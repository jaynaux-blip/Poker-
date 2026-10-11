// Scripted, repeatable nights (ReplayHooks.h): the lead's Phase A replays, driven from the console in PIE (or a -game
// standalone), each written to a text file another build's run is diffed against (tools/replay/replay_diff.py):
//
//   ss.Replay.Load <save.txt>                         the career becomes that save's text (a sandbox project only)
//   ss.Replay.Nightly <occurrence id> <out> [seed] [chips]
//       A Nightly from the desk to the cage: the room opened on an event the player isn't registered for (the desk writes
//       them in on arrival: FLiveDeskSignup and RegisterFromSignup), Chips added to their stack before the first hand, the
//       whole night played at once on autopilot (TestFastForward), then out past the cage and home.
//   ss.Replay.Dee <buy-in cents> <hands> <out> [seed]
//       A night at Dee's game: the hero checks when it's free, calls up to four big blinds and folds the rest, then racks
//       up after Hands hands.
//   ss.Replay.Perf <occurrence id> <label> <out> [warmup s] [measure s] [stats 0|1] [quit 0|1]
//       S-1, the card room's numbers: the room opened on a registered entry (a full field: every table running), the
//       hero on the same autopilot as Dee's night, Warmup seconds, then Measure seconds of frame, game thread, render
//       thread, RHI and GPU times (what stat unit shows), engine voices and the tables running, averaged with percentiles,
//       with csvprofile around the window (Saved/Profiling/CSV) for stat game's and stat gpu's breakdown. Stats 1 shows
//       stat unit, game, gpu and audio during the window (as the spec's S-1 asks; they cost a little). Quit 1 ends the
//       game after writing (a -game standalone run).
//   ss.Replay.Stop                                    ends the run and writes what it has
//
// While a replay runs the card table deals from the seed instead of the clock (ABackRoomTable::AddPlayer) and every line
// the table hands its HUD is recorded (ABackRoomTable::TakeLines), drained every frame so nothing depends on the HUD
// drawing. The file holds the new ledger lines, the entry the night wrote, the bankroll, the world's checksum and every
// caption in order. This file is the same in the build before the change (applied there as a patch), so both play alike.
#include "ReplayHooks.h"

#include "AudioDevice.h"
#include "BackRoomGameMode.h"
#include "BackRoomPlayer.h"
#include "BackRoomTable.h"
#include "CareerSave.h"
#include "Containers/Ticker.h"
#include "DynamicRHI.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RenderTimer.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Tournament.h"

#include <functional>
#include <string>

DEFINE_LOG_CATEGORY_STATIC(LogReplay, Log, All);

namespace ReplayDetail
{
enum class EKind : uint8
{
	None,
	Nightly,
	Dee,
	Perf,
};

/** The run under way (one at a time). */
struct FRun
{
	EKind Kind = EKind::None;
	uint32 Seed = 0;
	FString Occurrence;
	int64 BuyIn = 0;
	int32 Hands = 0;
	int32 Chips = 0;
	FString Out;
	int32 Step = 0;
	double StartedAt = 0.0;
	double StepAt = 0.0;
	TWeakObjectPtr<UWorld> From;
	TWeakObjectPtr<UWorld> Room;
	bool bLeaveAsked = false;
	FString Error;
	// What the night wrote, taken as the room's night ends (the save is settled by then).
	FString Ledger;
	FString Entry;
	FString Bankroll;
	FString World;
	/** The engine's night as it ended: paid places, the field, ticks (the diff sets aside busts called far from the money). */
	FString Tournament;
	// The ledger before the night, newest first (the night's lines are the ones in front of it).
	std::vector<ss::life::LedgerEntry> Before;
	TArray<FString> Captions;
	TArray<FString> Dealers;
	FTSTicker::FDelegateHandle Ticker;
	// S-1 (Perf).
	FString Label;
	double Warmup = 20.0;
	double Measure = 30.0;
	bool bStats = false;
	bool bQuit = false;
	TArray<double> FrameMs, GameMs, RenderMs, RhiMs, GpuMs;
	TArray<int32> Voices;
	int32 TablesMin = 0, TablesMax = 0;
};

FRun Run;

FString Str(const std::string& S)
{
	return FString(UTF8_TO_TCHAR(S.c_str()));
}

/** The game's world now: PIE's, or a standalone game's. */
UWorld* GameWorld()
{
	if (!GEngine)
	{
		return nullptr;
	}
	for (const FWorldContext& C : GEngine->GetWorldContexts())
	{
		if ((C.WorldType == EWorldType::PIE || C.WorldType == EWorldType::Game) && C.World())
		{
			return C.World();
		}
	}
	return nullptr;
}

/** The card room's (or Dee's) game mode in the game's world, if that's where it is. */
ABackRoomGameMode* RoomMode(UWorld*& OutWorld)
{
	OutWorld = GameWorld();
	return OutWorld ? OutWorld->GetAuthGameMode<ABackRoomGameMode>() : nullptr;
}

/** Only a project made for replays (tools/replay/make_trees.sh writes the marker) ever has its career overwritten. */
bool IsSandbox()
{
	return FPaths::FileExists(FPaths::ProjectDir() / TEXT("REPLAY_SANDBOX.txt"));
}

bool LoadSave(ss::SaveData& D)
{
	std::string Text;
	return CareerSave::LoadText(Text) && ss::SaveData::Parse(Text, D);
}

bool SameLine(const ss::life::LedgerEntry& A, const ss::life::LedgerEntry& B)
{
	return A.At == B.At && A.Label == B.Label && A.Amount == B.Amount && A.Kind == B.Kind;
}

/** The night's ledger lines (oldest first), its entry, the bankroll and the world's checksum, from the save on disk. */
void TakeSave()
{
	ss::SaveData D;
	if (!LoadSave(D))
	{
		Run.Error = TEXT("no career save to read at the night's end");
		return;
	}
	// The night's lines are the fewest in front for which the rest is the ledger as it was (the ledger keeps the last 60,
	// so the old one may have lost its oldest lines meanwhile); a new line can repeat an old one exactly, so no search.
	const std::vector<ss::life::LedgerEntry>& After = D.Life.Ledger;
	size_t New = After.size();
	for (size_t K = 0; K <= After.size(); ++K)
	{
		bool bRest = After.size() - K <= Run.Before.size();
		for (size_t J = K; bRest && J < After.size(); ++J)
		{
			bRest = SameLine(After[J], Run.Before[J - K]);
		}
		if (bRest)
		{
			New = K;
			break;
		}
	}
	TArray<FString> Lines;
	for (size_t K = 0; K < New; ++K)
	{
		const ss::life::LedgerEntry& L = After[K];
		Lines.Insert(FString::Printf(TEXT("%.2f\t%d\t%lld\t%s"), L.At, L.Kind, static_cast<long long>(L.Amount), *Str(L.Label)), 0);
	}
	Run.Ledger = FString::Join(Lines, TEXT("\n"));
	Run.Bankroll = FString::Printf(TEXT("%lld"), static_cast<long long>(D.BankrollCents));
	Run.World = FString::Printf(TEXT("%08x %d bytes"), FCrc::MemCrc32(D.WorldText.data(), static_cast<int32>(D.WorldText.size())), static_cast<int32>(D.WorldText.size()));
	if (const ss::life::LiveEntry* E = Run.Occurrence.IsEmpty() ? nullptr : ss::live::EntryFor(D.Life, std::string(TCHAR_TO_UTF8(*Run.Occurrence))))
	{
		FString S = FString::Printf(TEXT("%s\tstate %d\tplace %d of %d\tprize %lld\tpaid %lld\tfee %lld\tbus %d/%d"), *Str(E->Id), E->State, E->Place, E->Entrants,
			static_cast<long long>(E->PrizeCents), static_cast<long long>(E->PaidCents), static_cast<long long>(E->FeeCents), E->FareThere ? 1 : 0, E->FareHome ? 1 : 0);
		for (const std::pair<std::string, int>& R : E->Roster)
		{
			S += FString::Printf(TEXT("\nroster\t%s\t%d"), *Str(R.first), R.second);
		}
		for (const ss::life::LiveFace& F : E->Faces)
		{
			S += FString::Printf(TEXT("\nface\t%s\t%s\t%s"), *Str(F.Name), *Str(F.Label), *Str(F.Line));
		}
		for (const std::string& B : E->Board)
		{
			S += TEXT("\nboard\t") + Str(B);
		}
		Run.Entry = S;
	}
}

void Write(const TCHAR* Result)
{
	FString T = FString::Printf(TEXT("# ShortStack replay: %s %s, seed %u, project %s, %s\n"), Run.Kind == EKind::Nightly ? TEXT("nightly") : Run.Kind == EKind::Perf ? TEXT("perf") : TEXT("dee"),
		Run.Kind == EKind::Nightly ? *Run.Occurrence : *FString::Printf(TEXT("buy-in %lld, %d hands"), Run.BuyIn, Run.Hands), Run.Seed, *FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()),
		*FDateTime::Now().ToString());
	T += FString::Printf(TEXT("[result]\n%s%s\n"), Result, Run.Error.IsEmpty() ? TEXT("") : *(TEXT(": ") + Run.Error));
	T += FString::Printf(TEXT("[ledger]\n%s\n"), *Run.Ledger);
	T += FString::Printf(TEXT("[entry]\n%s\n"), *Run.Entry);
	T += FString::Printf(TEXT("[bankroll]\n%s\n"), *Run.Bankroll);
	T += FString::Printf(TEXT("[world]\n%s\n"), *Run.World);
	T += FString::Printf(TEXT("[tournament]\n%s\n"), *Run.Tournament);
	T += FString::Printf(TEXT("[dealers]\n%s\n"), *FString::Join(Run.Dealers, TEXT("\n")));
	T += FString::Printf(TEXT("[captions]\n%s\n"), *FString::Join(Run.Captions, TEXT("\n")));
	FFileHelper::SaveStringToFile(T, *Run.Out, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	UE_LOG(LogReplay, Display, TEXT("Replay %s: %d captions, written to %s"), Result, Run.Captions.Num(), *Run.Out);
}

void Finish(const TCHAR* Result)
{
	Write(Result);
	if (Run.Ticker.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(Run.Ticker);
	}
	Run = FRun();
}

void NoteDealer(ABackRoomGameMode* GM)
{
	const FString Name = GM && GM->Dealer ? GM->Dealer->Persona.Name : FString();
	if (!Name.IsEmpty())
	{
		Run.Dealers.AddUnique(Name);
	}
}

/** The replays' hero at a table: checks when it's free, calls up to four big blinds, folds the rest. */
void Autopilot(ABackRoomTable* T)
{
	const FBackRoomPrompt P = T->GetPrompt();
	if (!P.bYourTurn)
	{
		return;
	}
	if (P.bCanCheck || P.ToCall <= 4 * FMath::Max<int64>(1, P.BigBlind))
	{
		T->HeroCheckCall();
	}
	else
	{
		T->HeroFold();
	}
}

FString Spread(const TArray<double>& V)
{
	if (V.Num() == 0)
	{
		return TEXT("(none)");
	}
	TArray<double> S = V;
	S.Sort();
	double Sum = 0.0;
	for (double X : S)
	{
		Sum += X;
	}
	return FString::Printf(TEXT("avg %.2f  p50 %.2f  p95 %.2f  p99 %.2f  max %.2f"), Sum / S.Num(), S[S.Num() / 2], S[FMath::Min(S.Num() - 1, S.Num() * 95 / 100)],
		S[FMath::Min(S.Num() - 1, S.Num() * 99 / 100)], S.Last());
}

/** S-1's numbers, written; then, from a standalone run, the game ends. */
void FinishPerf(UWorld* World)
{
	if (GEngine && World)
	{
		GEngine->Exec(World, TEXT("csvprofile stop"));
		if (Run.bStats)
		{
			GEngine->Exec(World, TEXT("stat none"));
		}
	}
	FVector2D Size(0.0, 0.0);
	if (World && World->GetGameViewport())
	{
		World->GetGameViewport()->GetViewportSize(Size);
	}
	TArray<double> Vo;
	for (int32 V : Run.Voices)
	{
		Vo.Add(V);
	}
	FString T = FString::Printf(TEXT("# S-1 card room baseline: %s, %s, project %s, %s\n"), *Run.Label, *Run.Occurrence, *FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()),
		*FDateTime::Now().ToString());
	T += FString::Printf(TEXT("viewport %.0f x %.0f, %d frames over %.0f s after %.0f s warm-up, stats on screen %d\n"), Size.X, Size.Y, Run.FrameMs.Num(), Run.Measure, Run.Warmup,
		Run.bStats ? 1 : 0);
	T += FString::Printf(TEXT("tables running %d..%d (the room has 20 and the stage)\n"), Run.TablesMin, Run.TablesMax);
	T += FString::Printf(TEXT("frame ms   %s\n"), *Spread(Run.FrameMs));
	T += FString::Printf(TEXT("game ms    %s\n"), *Spread(Run.GameMs));
	T += FString::Printf(TEXT("render ms  %s\n"), *Spread(Run.RenderMs));
	T += FString::Printf(TEXT("rhi ms     %s\n"), *Spread(Run.RhiMs));
	T += FString::Printf(TEXT("gpu ms     %s\n"), *Spread(Run.GpuMs));
	T += FString::Printf(TEXT("voices     %s (active engine sources)\n"), *Spread(Vo));
	T += TEXT("csvprofile: the window's per-frame stats are the newest file in Saved/Profiling/CSV (csv_summary.py averages them)\n");
	FFileHelper::SaveStringToFile(T, *Run.Out, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	UE_LOG(LogReplay, Display, TEXT("S-1 written to %s"), *Run.Out);
	const bool bQuit = Run.bQuit;
	if (Run.Ticker.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(Run.Ticker);
	}
	Run = FRun();
	if (bQuit)
	{
		FPlatformMisc::RequestExit(false);
	}
}

/** Every frame of a run: drain the table's lines (the hook records them) and move the script on. */
bool Tick(float)
{
	if (Run.Kind == EKind::None)
	{
		return false;
	}
	const double Now = FPlatformTime::Seconds();
	if (Now - Run.StartedAt > 1800.0)
	{
		Run.Error = TEXT("timed out (30 minutes)");
		Finish(TEXT("error"));
		return false;
	}
	UWorld* World = nullptr;
	ABackRoomGameMode* GM = RoomMode(World);
	if (GM && GM->Table && World != Run.From.Get())
	{
		GM->Table->TakeLines();
		NoteDealer(GM);
	}
	switch (Run.Step)
	{
	case 0:
		// The room opening (a new world with the card table's game mode).
		if (GM && World != Run.From.Get() && World->HasBegunPlay())
		{
			if (!GM->IsCareer() || (Run.Kind != EKind::Dee) != GM->IsLive())
			{
				Run.Error = TEXT("the room opened as practice (the save can't cover it, or the desk refused: see the log)");
				Finish(TEXT("error"));
				return false;
			}
			Run.Room = World;
			Run.Step = 1;
			Run.StepAt = Now;
			if (Run.Kind == EKind::Nightly)
			{
				// Before the first hand: the chips, and the whole night at once (both wait for the first gap between hands).
				GM->TestHeroChips(Run.Chips);
				GM->TestFastForward(4000);
			}
			else if (Run.Kind == EKind::Dee)
			{
				while (GM->GetTablePace() != 2)
				{
					GM->CycleTablePace();
				}
			}
			UE_LOG(LogReplay, Display, TEXT("Replay: the room is open (%s)"), *GM->DescribeNight());
		}
		else if (Now - Run.StartedAt > 120.0)
		{
			Run.Error = TEXT("the room never opened");
			Finish(TEXT("error"));
			return false;
		}
		break;
	case 1:
		if (!GM || World != Run.Room.Get())
		{
			Run.Error = TEXT("the room closed before the night ended");
			Finish(TEXT("error"));
			return false;
		}
		if (Run.Kind == EKind::Perf)
		{
			if (GM->Table && GM->GetPhase() == EBackRoomPhase::Playing)
			{
				Autopilot(GM->Table);
			}
			// Seated and dealt: the warm-up, then the window.
			if (Run.Step == 1 && GM->GetPhase() == EBackRoomPhase::Playing)
			{
				Run.Step = 3;
				Run.StepAt = Now;
			}
			break;
		}
		if (GM->GetPhase() == EBackRoomPhase::Leaving)
		{
			// The night is settled and saved: what it wrote.
			TakeSave();
			if (const ss::Tournament* Tn = GM->GetTourney())
			{
				Run.Tournament = FString::Printf(TEXT("paid %d\nentrants %d\nticks %d\nremaining %d\nfinished %d"), Tn->PaidPlaces(), Tn->Spec.Entrants, Tn->Tick, Tn->Remaining,
					Tn->bFinished ? 1 : 0);
			}
			if (Run.Kind == EKind::Nightly)
			{
				GM->LiveLeaveNow();
			}
			Run.Step = 2;
			Run.StepAt = Now;
			UE_LOG(LogReplay, Display, TEXT("Replay: the night is over (%s)"), *GM->DescribeNight());
			break;
		}
		if (Run.Kind == EKind::Nightly && (GM->GetPhase() == EBackRoomPhase::Playing || GM->GetPhase() == EBackRoomPhase::Moving))
		{
			// A fast-forward stops at a table move (the walk to the new seat) and plays on only when asked again: asked every
			// frame, it picks up at the first gap between hands at the new table, the same gap in every build. The autopilot
			// only acts should a hand ever come to the hero first.
			GM->TestFastForward(4000);
			if (GM->Table && GM->GetPhase() == EBackRoomPhase::Playing)
			{
				Autopilot(GM->Table);
			}
		}
		if (Run.Kind == EKind::Dee && GM->Table)
		{
			ABackRoomTable* T = GM->Table;
			if (GM->GetPhase() == EBackRoomPhase::Busted && !Run.bLeaveAsked)
			{
				Run.bLeaveAsked = true;
				GM->RequestLeave();
			}
			else if (GM->GetPhase() == EBackRoomPhase::Playing)
			{
				Autopilot(T);
				if (!Run.bLeaveAsked && T->GetHandNumber() >= Run.Hands)
				{
					Run.bLeaveAsked = true;
					GM->RequestLeave();
				}
			}
		}
		break;
	case 3:
	case 4:
		// S-1: the warm-up (3), then the window (4).
		if (!GM || World != Run.Room.Get())
		{
			Run.Error = TEXT("the room closed during the measurement");
			Finish(TEXT("error"));
			return false;
		}
		if (GM->Table && GM->GetPhase() == EBackRoomPhase::Playing)
		{
			Autopilot(GM->Table);
		}
		if (Run.Step == 3 && Now - Run.StepAt >= Run.Warmup)
		{
			Run.Step = 4;
			Run.StepAt = Now;
			GEngine->Exec(World, TEXT("r.GPUCsvStatsEnabled 1"));
			GEngine->Exec(World, TEXT("csvprofile start"));
			if (Run.bStats)
			{
				for (const TCHAR* Cmd : {TEXT("stat unit"), TEXT("stat game"), TEXT("stat gpu"), TEXT("stat audio")})
				{
					GEngine->Exec(World, Cmd);
				}
			}
			UE_LOG(LogReplay, Display, TEXT("S-1: measuring %.0f s (%s)"), Run.Measure, *GM->DescribeNight());
		}
		else if (Run.Step == 4)
		{
			Run.FrameMs.Add(FApp::GetDeltaTime() * 1000.0);
			Run.GameMs.Add(FPlatformTime::ToMilliseconds(GGameThreadTime));
			Run.RenderMs.Add(FPlatformTime::ToMilliseconds(GRenderThreadTime));
			Run.RhiMs.Add(FPlatformTime::ToMilliseconds(GRHIThreadTime));
			Run.GpuMs.Add(FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles(0)));
			FAudioDeviceHandle Device = World->GetAudioDevice();
			Run.Voices.Add(Device.IsValid() ? Device->GetNumActiveSources() : 0);
			const int32 Tables = GM->GetTourney() ? static_cast<int32>(GM->GetTourney()->Tables.size()) : 0;
			Run.TablesMin = Run.FrameMs.Num() == 1 ? Tables : FMath::Min(Run.TablesMin, Tables);
			Run.TablesMax = FMath::Max(Run.TablesMax, Tables);
			if (Now - Run.StepAt >= Run.Measure)
			{
				FinishPerf(World);
				return false;
			}
		}
		break;
	case 2:
		// Out of the room (the cage on the way at the Embercrest), or long enough after.
		if (!GM || World != Run.Room.Get() || Now - Run.StepAt > 120.0)
		{
			Finish(TEXT("done"));
			return false;
		}
		break;
	default:
		break;
	}
	return true;
}

/** Starts a run: the seed, the save's newest ledger line, and the room opened from the game's world. */
bool Start(EKind Kind, const FString& Options)
{
	if (Run.Kind != EKind::None)
	{
		UE_LOG(LogReplay, Warning, TEXT("A replay is already running (ss.Replay.Stop ends it)"));
		return false;
	}
	UWorld* World = GameWorld();
	if (!World)
	{
		UE_LOG(LogReplay, Warning, TEXT("Replay: start the game (PIE) first"));
		return false;
	}
	ss::SaveData D;
	if (!LoadSave(D))
	{
		UE_LOG(LogReplay, Warning, TEXT("Replay: no career save (ss.Replay.Load one)"));
		return false;
	}
	Run.Kind = Kind;
	Run.Before = D.Life.Ledger;
	Run.StartedAt = FPlatformTime::Seconds();
	Run.From = World;
	Run.Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&Tick), 0.0f);
	UE_LOG(LogReplay, Display, TEXT("Replay: seed %u, bankroll %lld, clock %.2f; opening BackRoom?%s"), Run.Seed, static_cast<long long>(D.BankrollCents), D.ClockMinutes, *Options);
	UGameplayStatics::OpenLevel(World, FName(TEXT("BackRoom")), true, Options);
	return true;
}

/** A replay under way: another command that would start one (or rewrite the career) is refused before it touches it. */
bool Busy()
{
	if (Run.Kind == EKind::None)
	{
		return false;
	}
	UE_LOG(LogReplay, Warning, TEXT("A replay is already running (ss.Replay.Stop ends it)"));
	return true;
}

FAutoConsoleCommand LoadCmd(TEXT("ss.Replay.Load"), TEXT("ss.Replay.Load <save text file>: the career becomes that save (a replay sandbox project only)."),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args) {
		if (!IsSandbox())
		{
			UE_LOG(LogReplay, Error, TEXT("ss.Replay.Load: this project isn't a replay sandbox (no REPLAY_SANDBOX.txt in %s); the career here is left alone"),
				*FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()));
			return;
		}
		if (Busy())
		{
			return;
		}
		FString Text;
		if (Args.Num() < 1 || !FFileHelper::LoadFileToString(Text, *Args[0]))
		{
			UE_LOG(LogReplay, Error, TEXT("ss.Replay.Load: can't read %s"), Args.Num() > 0 ? *Args[0] : TEXT("(no file)"));
			return;
		}
		const std::string Utf8(TCHAR_TO_UTF8(*Text.Replace(TEXT("\r\n"), TEXT("\n"))));
		ss::SaveData D;
		if (!ss::SaveData::Parse(Utf8, D))
		{
			UE_LOG(LogReplay, Error, TEXT("ss.Replay.Load: %s isn't a career save"), *Args[0]);
			return;
		}
		CareerSave::SaveNow(Utf8, false);
		UE_LOG(LogReplay, Display, TEXT("ss.Replay.Load: %s (bankroll %lld, clock %.2f, %d ledger lines, %d live entries, world %d bytes)"), *Args[0],
			static_cast<long long>(D.BankrollCents), D.ClockMinutes, static_cast<int32>(D.Life.Ledger.size()), static_cast<int32>(D.Life.LiveEntries.size()),
			static_cast<int32>(D.WorldText.size()));
	}));

FAutoConsoleCommand NightlyCmd(TEXT("ss.Replay.Nightly"),
	TEXT("ss.Replay.Nightly <occurrence id> <out file> [seed] [chips]: a Nightly from the desk to the cage, played at once, written to the file."),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args) {
		if (Args.Num() < 2)
		{
			UE_LOG(LogReplay, Warning, TEXT("ss.Replay.Nightly <occurrence id> <out file> [seed] [chips]"));
			return;
		}
		if (!ss::live::FindOccurrence(std::string(TCHAR_TO_UTF8(*Args[0]))).Valid())
		{
			UE_LOG(LogReplay, Warning, TEXT("ss.Replay.Nightly: %s isn't one of the Embercrest's events"), *Args[0]);
			return;
		}
		if (Busy())
		{
			return;
		}
		Run.Seed = Args.Num() > 2 ? static_cast<uint32>(FCString::Atoi64(*Args[2])) : 1u;
		Run.Chips = Args.Num() > 3 ? FCString::Atoi(*Args[3]) : 1000000;
		Run.Occurrence = Args[0];
		Run.Out = Args[1];
		if (!Start(EKind::Nightly, TEXT("Live=") + Args[0]))
		{
			Run = FRun();
		}
	}));

FAutoConsoleCommand DeeCmd(TEXT("ss.Replay.Dee"), TEXT("ss.Replay.Dee <buy-in cents> <hands> <out file> [seed]: a night at Dee's game on a fixed policy, written to the file."),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args) {
		if (Args.Num() < 3)
		{
			UE_LOG(LogReplay, Warning, TEXT("ss.Replay.Dee <buy-in cents> <hands> <out file> [seed]"));
			return;
		}
		if (Busy())
		{
			return;
		}
		Run.BuyIn = FCString::Atoi64(*Args[0]);
		Run.Hands = FMath::Max(1, FCString::Atoi(*Args[1]));
		Run.Out = Args[2];
		Run.Seed = Args.Num() > 3 ? static_cast<uint32>(FCString::Atoi64(*Args[3])) : 1u;
		if (!Start(EKind::Dee, FString::Printf(TEXT("BuyIn=%lld"), Run.BuyIn)))
		{
			Run = FRun();
		}
	}));

FAutoConsoleCommand PerfCmd(TEXT("ss.Replay.Perf"),
	TEXT("ss.Replay.Perf <occurrence id> <label> <out file> [warmup s] [measure s] [stats 0|1] [quit 0|1]: S-1, the card room measured on a registered entry."),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args) {
		if (Args.Num() < 3)
		{
			UE_LOG(LogReplay, Warning, TEXT("ss.Replay.Perf <occurrence id> <label> <out file> [warmup s] [measure s] [stats 0|1] [quit 0|1]"));
			return;
		}
		if (Busy())
		{
			return;
		}
		Run.Occurrence = Args[0];
		Run.Label = Args[1];
		Run.Out = Args[2];
		Run.Warmup = Args.Num() > 3 ? FMath::Max(0.0, FCString::Atod(*Args[3])) : 20.0;
		Run.Measure = Args.Num() > 4 ? FMath::Max(1.0, FCString::Atod(*Args[4])) : 30.0;
		Run.bStats = Args.Num() > 5 && FCString::Atoi(*Args[5]) != 0;
		Run.bQuit = Args.Num() > 6 && FCString::Atoi(*Args[6]) != 0;
		Run.Seed = 1u;
		if (!Start(EKind::Perf, TEXT("Live=") + Args[0]))
		{
			Run = FRun();
		}
	}));

FAutoConsoleCommand StopCmd(TEXT("ss.Replay.Stop"), TEXT("Ends the replay under way and writes what it has."), FConsoleCommandDelegate::CreateLambda([]() {
	if (Run.Kind != EKind::None)
	{
		Run.Error = TEXT("stopped");
		Finish(TEXT("error"));
	}
}));
} // namespace ReplayDetail

uint32 ReplayHooks::Seed()
{
	return ReplayDetail::Run.Kind != ReplayDetail::EKind::None ? FMath::Max(1u, ReplayDetail::Run.Seed) : 0u;
}

void ReplayHooks::Caption(const FString& Speaker, const FString& Text, bool bRead)
{
	if (ReplayDetail::Run.Kind != ReplayDetail::EKind::None)
	{
		ReplayDetail::Run.Captions.Add(bRead ? TEXT("~read\t") + Text : Speaker + TEXT("\t") + Text);
	}
}
