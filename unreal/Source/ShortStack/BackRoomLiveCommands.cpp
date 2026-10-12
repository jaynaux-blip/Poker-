// The Embercrest's test hooks and console commands (ABackRoomGameMode's live tournaments; the flow is BackRoomLive.cpp):
// the fast-forwards every track and the lead's acceptance runs share (ss.Live.Bubble, Final, Break, Shove), a bust, the
// last call, the next event from the desk and its seating; the room's director and its sound (ss.Live.Room, ss.Live.Cue,
// ss.Live.Night) and the listening gate (ss.Live.Listen, ss.Live.Bed; the ss.Room.* tunables are in BackRoomLiveRoom.cpp).

#include "BackRoomLiveShared.h"

#include "AudioDevice.h"
#include "BackRoomGameMode.h"
#include "BackRoomStage.h"
#include "BackRoomTable.h"
#include "CardRoomSound.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/Paths.h"
#include "NightOneAudio.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Room.h"
#include "ShortStack/Game/TableTalk.h"
#include "ShortStack/Tournament.h"

namespace LiveDetail
{
/** A room console command's game mode: the Embercrest's, or nothing (Dee's game, practice, another map). */
ABackRoomGameMode* LiveMode(UWorld* World)
{
	ABackRoomGameMode* GM = World ? World->GetAuthGameMode<ABackRoomGameMode>() : nullptr;
	return GM && GM->IsLive() ? GM : nullptr;
}

FAutoConsoleCommandWithWorldAndArgs BustCmd(TEXT("ss.Live.Bust"),
	TEXT("At the Embercrest: the player busts at the next gap between hands (testing: a one-chip stack, blinded out by the engine)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World) {
		if (ABackRoomGameMode* GM = LiveMode(World))
		{
			GM->TestBustNow();
		}
	}));
FAutoConsoleCommandWithWorldAndArgs LastCallCmd(TEXT("ss.Live.LastCall"),
	TEXT("At the Embercrest: the room plays on (you on autopilot) to just before the floor's last call for registration."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World) {
		if (ABackRoomGameMode* GM = LiveMode(World))
		{
			GM->TestLastCall();
		}
	}));
FAutoConsoleCommandWithWorldAndArgs NextCmd(TEXT("ss.Live.Next"),
	TEXT("ss.Live.Next [occurrence id]: once the player's night is over, the desk writes them into the next event today (or that one), as E twice at the desk does."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (ABackRoomGameMode* GM = LiveMode(World))
		{
			GM->TestRegisterNext(Args.Num() > 0 ? Args[0] : FString());
		}
	}));
FAutoConsoleCommandWithWorldAndArgs SeatCmd(TEXT("ss.Live.Seat"),
	TEXT("At the Embercrest, registered for the next event: the wait passes (as the terrace does) and the floor calls its seats."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World) {
		if (ABackRoomGameMode* GM = LiveMode(World))
		{
			GM->TestSeatingNow();
		}
	}));

// The shared fast-forwards (next_segment_spec 8.2): every track's checks and the lead's acceptance run (9.1) start from them.
FAutoConsoleCommandWithWorldAndArgs BubbleCmd(TEXT("ss.Live.Bubble"),
	TEXT("At the Embercrest: the whole room plays on at once (you on autopilot) until it's hand for hand on the bubble."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World) {
		if (ABackRoomGameMode* GM = LiveMode(World))
		{
			GM->TestFastForwardTo(TEXT("bubble"));
		}
	}));
FAutoConsoleCommandWithWorldAndArgs FinalCmd(TEXT("ss.Live.Final"),
	TEXT("At the Embercrest: the whole room plays on at once (you on autopilot) until the final table."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World) {
		if (ABackRoomGameMode* GM = LiveMode(World))
		{
			GM->TestFastForwardTo(TEXT("final"));
		}
	}));
FAutoConsoleCommandWithWorldAndArgs BreakCmd(TEXT("ss.Live.Break"),
	TEXT("At the Embercrest: the whole room plays on at once (you on autopilot) until the next break."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World) {
		if (ABackRoomGameMode* GM = LiveMode(World))
		{
			GM->TestFastForwardTo(TEXT("break"));
		}
	}));
FAutoConsoleCommandWithWorldAndArgs ShoveCmd(TEXT("ss.Live.Shove"),
	TEXT("At the Embercrest: the next opponent to act at your table while you're still in the hand moves all in."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World) {
		if (ABackRoomGameMode* GM = LiveMode(World))
		{
			GM->TestShove();
		}
	}));
// The room's director (ue-room fills DescribeRoom and TestCue).
FAutoConsoleCommandWithWorldAndArgs RoomCmd(TEXT("ss.Live.Room"),
	TEXT("At the Embercrest: the room's mood, each layer's level, the voices in use, the synth's cost, engine voices and the last 10 cues."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World) {
		if (ABackRoomGameMode* GM = LiveMode(World))
		{
			TArray<FString> Lines;
			GM->DescribeRoom().ParseIntoArrayLines(Lines, false);
			for (const FString& Line : Lines)
			{
				UE_LOG(LogEmbercrest, Display, TEXT("%s"), *Line);
			}
		}
	}));
FAutoConsoleCommandWithWorldAndArgs CueCmd(TEXT("ss.Live.Cue"),
	TEXT("ss.Live.Cue <moment> [seat]: at the Embercrest, the room's director says that moment now (a talk::Moment's name), from that seat at your table."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (ABackRoomGameMode* GM = LiveMode(World))
		{
			GM->TestCue(Args.Num() > 0 ? Args[0] : FString(), Args.Num() > 1 ? FCString::Atoi(*Args[1]) : -1);
		}
	}));

/** How full the room is now (the share of its tables running), for today's bed beside it. */
float RoomCrowd(const ABackRoomGameMode& GM)
{
	const ss::Tournament* T = GM.GetTourney();
	if (!T)
	{
		return 0.7f;
	}
	const int32 All = FMath::Max(1, (T->Spec.Entrants + T->TableSize - 1) / FMath::Max(1, T->TableSize));
	return FMath::Clamp(static_cast<float>(T->Tables.size()) / static_cast<float>(All), 0.15f, 1.0f);
}

// The listening gate (next_segment_spec 4.10, 9.1 step 4): the room recorded in PIE beside today's bed, and an A/B live.
FAutoConsoleCommandWithWorldAndArgs ListenCmd(TEXT("ss.Live.Listen"),
	TEXT("ss.Live.Listen <name> [seconds=60] [dir]: at the Embercrest, records the room as heard from here (the stereo stream and the near voices over it) into ")
		TEXT("<dir>/pie_<name>.wav, and today's mono bed beside it as pie_<name>_bed.wav (dir defaults to Saved/Listening)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		ABackRoomGameMode* GM = LiveMode(World);
		FCardRoomSound* Sound = GM ? GM->GetRoomSound() : nullptr;
		if (!Sound)
		{
			UE_LOG(LogEmbercrest, Warning, TEXT("ss.Live.Listen: only at the Embercrest, with the room's sound up"));
			return;
		}
		FString Name = Args.Num() > 0 ? Args[0] : FString(TEXT("room"));
		for (TCHAR& Ch : Name)
		{
			Ch = FChar::IsAlnum(Ch) || Ch == TEXT('_') || Ch == TEXT('-') ? Ch : TEXT('_');
		}
		const double Seconds = Args.Num() > 1 ? FMath::Clamp(FCString::Atod(*Args[1]), 1.0, 600.0) : 60.0;
		const FString Dir = Args.Num() > 2 ? Args[2] : FPaths::ProjectSavedDir() / TEXT("Listening");
		if (!Sound->StartRecording(Name, Dir, Seconds, RoomCrowd(*GM)))
		{
			UE_LOG(LogEmbercrest, Warning, TEXT("ss.Live.Listen: a recording is under way already"));
			return;
		}
		UE_LOG(LogEmbercrest, Display, TEXT("ss.Live.Listen: recording %.0f s of the room (%s) into %s/pie_%s.wav, today's bed beside it"), Seconds,
			GM->GetTourney() ? UTF8_TO_TCHAR(ss::room::MoodName(ss::room::MoodOf(*GM->GetTourney(), false, true))) : TEXT("?"), *Dir, *Name);
	}));
FAutoConsoleCommandWithWorldAndArgs BedCmd(TEXT("ss.Live.Bed"),
	TEXT("ss.Live.Bed 1|0: at the Embercrest, today's mono bed (CardRoomAmbience.h) plays in the room's place (1) or the stereo room again (0): the listening gate's A/B."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		ABackRoomGameMode* GM = LiveMode(World);
		FCardRoomSound* Sound = GM ? GM->GetRoomSound() : nullptr;
		UNightOneAudio* Audio = GM && GM->Table ? GM->Table->GetAudio() : nullptr;
		if (!Sound || !Audio)
		{
			UE_LOG(LogEmbercrest, Warning, TEXT("ss.Live.Bed: only at the Embercrest, with the room's sound up"));
			return;
		}
		const bool bBed = Args.Num() > 0 && FCString::Atoi(*Args[0]) != 0;
		if (bBed)
		{
			RoomStream::TodaysBed(*Audio, RoomCrowd(*GM));
		}
		else
		{
			RoomStream::Room(*Audio, Sound->AsShared());
		}
		UE_LOG(LogEmbercrest, Display, TEXT("ss.Live.Bed: %s"), bBed ? TEXT("today's mono bed (A)") : TEXT("the stereo room (B)"));
	}));
FAutoConsoleCommandWithWorldAndArgs NightCmd(TEXT("ss.Live.Night"),
	TEXT("ss.Live.Night [lines=40]: at the Embercrest, the night's log (the table card's NIGHT page): every line the room said, with the room's clock."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		ABackRoomGameMode* GM = LiveMode(World);
		if (!GM)
		{
			return;
		}
		const std::vector<ss::ui::Caption>& Log = GM->GetNightLog();
		const int32 N = Args.Num() > 0 ? FMath::Max(1, FCString::Atoi(*Args[0])) : 40;
		UE_LOG(LogEmbercrest, Display, TEXT("THE NIGHT  %s  (%d lines kept, %d on screen now)"), *GM->GetLiveName(), static_cast<int32>(Log.size()), static_cast<int32>(GM->GetCaptions().size()));
		for (size_t I = Log.size() > static_cast<size_t>(N) ? Log.size() - static_cast<size_t>(N) : 0; I < Log.size(); ++I)
		{
			const ss::ui::Caption& C = Log[I];
			static const TCHAR* Styles[] = {TEXT(""), TEXT("FLOOR "), TEXT("sound "), TEXT("to you ")};
			UE_LOG(LogEmbercrest, Display, TEXT("  %8s  %s%s%s%s"), UTF8_TO_TCHAR(C.Clock.c_str()), Styles[FMath::Clamp(C.Style, 0, 3)],
				C.Speaker.empty() ? TEXT("") : UTF8_TO_TCHAR((C.Speaker + ": ").c_str()), UTF8_TO_TCHAR(C.Line.c_str()), C.Room ? TEXT("   [room]") : TEXT(""));
		}
	}));
} // namespace LiveDetail

using namespace LiveDetail;

// ------------------------------------------------------------------ test hooks

void ABackRoomGameMode::TestFastForward(int32 Hands)
{
	// -1: the floor moves you to another table (at the next gap between hands).
	if (Hands < 0)
	{
		bTestMove = true;
		return;
	}
	FastForwardLeft = Hands;
}

void ABackRoomGameMode::TestHeroChips(int32 Chips)
{
	TestChips += Chips;
}

void ABackRoomGameMode::TestBustNow()
{
	if (!bLive || !Tourney || bLiveOver || bCancelled || Tourney->Hero().Busted || Tourney->bFinished)
	{
		UE_LOG(LogEmbercrest, Warning, TEXT("Test bust: the player isn't in a tournament here"));
		return;
	}
	bTestBust = true;
	UE_LOG(LogEmbercrest, Log, TEXT("Test bust: at the next gap between hands"));
}

void ABackRoomGameMode::TestLastCall()
{
	if (!bLive || !Tourney || bCancelled)
	{
		return;
	}
	const double At = LastCallAt(LiveLateRegEnds);
	if (Minutes >= LiveLateRegEnds)
	{
		UE_LOG(LogEmbercrest, Warning, TEXT("Test last call: registration closed at %s"), *Str(ss::net::TimeLabel(LiveLateRegEnds)));
		return;
	}
	if (Minutes >= At)
	{
		bLastCalled = false; // inside the window: the floor calls it again, now
		return;
	}
	if (bLiveOver)
	{
		RailCatchUp(At - 0.5);
		return;
	}
	const int32 Hands = FMath::Max(0, FMath::FloorToInt((At - 0.5 - (LiveDayStart + Tourney->ClockMinutes())) * 60.0 / Tourney->Spec.SecondsPerHand));
	UE_LOG(LogEmbercrest, Log, TEXT("Test last call: %d hands on to %s"), Hands, *Str(ss::net::TimeLabel(At)));
	TestFastForward(Hands);
}

void ABackRoomGameMode::TestRegisterNext(const FString& OccurrenceId)
{
	if (!bLive || !bLiveOver || bCancelled || !NextEntryId.IsEmpty() || Signup.IsValid())
	{
		UE_LOG(LogEmbercrest, Warning, TEXT("Test register: only once the player's night is over, with nothing registered yet"));
		return;
	}
	if (!bRailing)
	{
		LiveStay();
	}
	FString WhyNot;
	const FString Id = OccurrenceId.IsEmpty() ? NextOpenEvent(WhyNot) : OccurrenceId;
	if (Id.IsEmpty())
	{
		UE_LOG(LogEmbercrest, Warning, TEXT("Test register: nothing the desk can sell now (%s)"), WhyNot.IsEmpty() ? TEXT("no event left today") : *WhyNot);
		return;
	}
	UE_CLOG(!bRailing, LogEmbercrest, Warning, TEXT("Test register: not on the rail yet (Q stays): leaving takes the player off the list"));
	DeskSignup(Id);
}

void ABackRoomGameMode::TestSeatingNow()
{
	if (NextEntryId.IsEmpty() || bNextCalled || bOpeningNext)
	{
		UE_LOG(LogEmbercrest, Warning, TEXT("Test seating: no next event waiting"));
		return;
	}
	RailCatchUp(NextSeatingAt);
}

// ------------------------------------------------------------------ the shared fast-forwards

void ABackRoomGameMode::TestFastForwardTo(const FString& Moment)
{
	const uint8 Stop = Moment == TEXT("bubble") ? 1 : Moment == TEXT("final") ? 2 : Moment == TEXT("break") ? 3 : 0;
	if (Stop == 0 || !bLive || !Tourney || bLiveOver || bCancelled || Tourney->Hero().Busted || Tourney->bFinished)
	{
		UE_LOG(LogEmbercrest, Warning, TEXT("Fast-forward to \"%s\": only at the Embercrest with the player still in it, to bubble, final or break"), *Moment);
		return;
	}
	FastForwardStop = Stop;
	if (FastForwardReached())
	{
		FastForwardStop = 0;
		UE_LOG(LogEmbercrest, Warning, TEXT("Fast-forward to %s: the room's there already (hand %d, %d left, %d tables)"), *Moment, Tourney->Tick, Tourney->Remaining,
			static_cast<int32>(Tourney->Tables.size()));
		return;
	}
	// As TestFastForward (LiveTick plays them at the next gap between hands), with a stop instead of a count: the guard is
	// a whole night's hands.
	FastForwardLeft = 4000;
	UE_LOG(LogEmbercrest, Log, TEXT("Fast-forward to %s: the room plays on from hand %d (%d left)"), *Moment, Tourney->Tick, Tourney->Remaining);
}

bool ABackRoomGameMode::FastForwardReached() const
{
	if (!Tourney)
	{
		return true;
	}
	switch (FastForwardStop)
	{
	case 1: return Tourney->HandForHand() || Tourney->InTheMoney();
	case 2: return Tourney->Tables.size() <= 1;
	case 3: return bOnBreak;
	default: return false;
	}
}

void ABackRoomGameMode::TestShove()
{
	if (!bLive || !Table || !Tourney || bLiveOver || bCancelled || Tourney->Hero().Busted || Tourney->bFinished)
	{
		UE_LOG(LogEmbercrest, Warning, TEXT("Test shove: the player isn't in a tournament here"));
		return;
	}
	Table->TestShoveNext();
	UE_LOG(LogEmbercrest, Log, TEXT("Test shove: the next opponent to act while you're in the hand moves all in"));
}

// ------------------------------------------------------------------ the room's director (ue-room)

FString ABackRoomGameMode::DescribeRoom() const
{
	if (!RoomDirector.IsValid() || !RoomHost.IsValid() || !Tourney)
	{
		return TEXT("No director in this room (ss.Live.Room: the Embercrest with the field seated).");
	}
	const FRoomHost& H = *RoomHost;
	FString Out;
	const int32 AllTables = FMath::Max(1, (Tourney->Spec.Entrants + Tourney->TableSize - 1) / FMath::Max(1, Tourney->TableSize));
	const float Share = FMath::Clamp(static_cast<float>(Tourney->Tables.size()) / static_cast<float>(AllTables), 0.0f, 1.0f);
	const ss::room::Mix Mix = ss::room::MixFor(RoomDirector->Now(), Share);
	Out += FString::Printf(TEXT("ROOM  %s  mood %s  hand %d  level %d  %d left  %d of %d tables open  table talk x%.1f\n"), *LiveName,
		UTF8_TO_TCHAR(ss::room::MoodName(RoomDirector->Now())), Tourney->Tick, Tourney->LevelIndex + 1, Tourney->Remaining, static_cast<int32>(Tourney->Tables.size()), AllTables,
		H.TalkScale);
	// Talk carries the crowd (10 log10 of the open share) and the Lombard lift (half of what the room rose, against a full
	// room Running): the arc's own number (spec 4.3's table) is what's left.
	const float CrowdDb = 10.0f * FMath::LogX(10.0f, FMath::Clamp(Share, 0.02f, 1.0f));
	const float ArcTalk = (Mix.Talk - 0.5f) / 1.5f - CrowdDb;
	Out += FString::Printf(TEXT("MIX (dB, room::MixFor)   bed %+.1f  talk %+.1f (the arc %+.1f, crowd %+.1f, Lombard %+.1f)  chips %+.1f  calls %+.1f  rail %+.1f  crowd %.2f\n"), Mix.Bed,
		Mix.Talk, ArcTalk, CrowdDb, Mix.Talk - ArcTalk - CrowdDb, Mix.Chips, Mix.Calls, Mix.Rail, Mix.Crowd);
	if (const FCardRoomSound* Sound = RoomSound.Get())
	{
		const ss::audio::CardRoom::Meter M = Sound->Measure();
		Out += FString::Printf(TEXT("HEARD (dBFS, last 0.5 s)  bed %.1f  talk %.1f  chips %.1f  calls %.1f  pa %.1f  rail %.1f  out %.1f\n"), M.BedDb, M.TalkDb, M.ChipsDb, M.CallsDb,
			M.PaDb, M.RailDb, M.OutDb);
		Out += FString::Printf(TEXT("VOICES  %d in use (%d formant voices speaking, %d tables voiced)  talkers %d%s  state %.1f KB\n"), M.Voiced, M.Talkers, M.Tables,
			Sound->GetVoiceBudget(), Sound->GetVoiceBudget() == 0 ? TEXT(" (the fallback: what ships)") : TEXT(" (DEV ss.Room.Talkers: the talkers the user turned down)"), M.Bytes / 1024.0);
		const FCardRoomSound::FStats S = Sound->Stats();
		Out += FString::Printf(TEXT("SYNTH (worker)  25 ms block avg %.3f ms, peak %.3f ms (%d blocks; %.2f ms a second of audio); ring %.0f ms ahead; game-thread renders %d (peak %.3f ms)\n"),
			S.BlockMsAvg, S.BlockMsPeak, S.Blocks, S.WorkerMsPerSecond, Sound->RingSeconds() * 1000.0, S.SyncRenders, S.SyncMsPeak);
		Out += FString::Printf(TEXT("MUFFLE  low-pass %.0f Hz, %+.1f dB (Focus %.2f, heart %.0f bpm%s);  pace x%.2f;  terrace doors %s\n"), H.LastMuffle[0], H.LastMuffle[1], H.FocusMuffle,
			Table ? Table->Composure.Bpm : 0.0f, bWaitingOut ? TEXT(", out on the terrace") : TEXT(""), H.LastPace, bWaitingOut || CurrentSpot() == ECardRoomSpot::Terrace ? TEXT("open") : TEXT("shut"));
		Out += FString::Printf(TEXT("RECORDING  %s\n"), Sound->IsRecording() ? TEXT("under way (ss.Live.Listen)") : TEXT("none"));
	}
	Out += FString::Printf(TEXT("HOST (game thread)  avg %.3f ms, peak %.3f ms a frame over %d frames;  speech jobs %d (worker avg %.2f ms, peak %.2f ms);  small talk left to the log %d\n"),
		H.HostFrames > 0 ? H.HostMsSum / H.HostFrames : 0.0, H.HostMsPeak, H.HostFrames, H.SpeechJobs, H.SpeechJobs > 0 ? H.SpeechMsSum / H.SpeechJobs : 0.0, H.SpeechMsPeak, H.Skipped);
	int32 Engine = 0;
	if (UWorld* World = GetWorld())
	{
		FAudioDeviceHandle Device = World->GetAudioDevice();
		Engine = Device.IsValid() ? Device->GetNumActiveSources() : 0;
	}
	const UNightOneAudio* Audio = Table ? Table->GetAudio() : nullptr;
	Out += FString::Printf(TEXT("ENGINE VOICES  %d active (budget 16); one-shots %d, near speech %d; stream latency %.0f ms; queued cues %d, timeline %d\n"), Engine,
		Audio ? Audio->NumOneShots() : 0, H.NearEnds.Num(), H.LastLatency * 1000.0, static_cast<int32>(H.Queue.size() + H.Incoming.size()), H.Timeline.Num());
	FString Rail;
	for (const std::pair<std::string, int>& R : H.RailSet)
	{
		Rail += FString::Printf(TEXT("%s%s %d%%"), Rail.IsEmpty() ? TEXT("") : TEXT(", "), UTF8_TO_TCHAR(R.first.c_str()), R.second / 10);
	}
	Out += FString::Printf(TEXT("RAIL  %s%s\n"), Rail.IsEmpty() ? TEXT("nobody backing the player out tonight") : *Rail,
		!H.bRailPlaced ? TEXT("") : (H.bRailStanding ? (H.RailAtBar > 0 ? TEXT(" (standing at the bar line)") : TEXT(" (standing at the stage's edge)")) : TEXT(" (watching from an empty table's chairs)")));
	Out += TEXT("LAST CUES\n");
	static const TCHAR* Kinds[] = {TEXT("LINE"), TEXT("CALL"), TEXT("PA"), TEXT("VOCAL"), TEXT("PHONE")};
	static const TCHAR* Places[] = {TEXT("your table"), TEXT("table"), TEXT("feature"), TEXT("rail"), TEXT("bar"), TEXT("desk"), TEXT("ceiling")};
	for (const ss::room::Cue& C : RecentCues)
	{
		const int32 K = FMath::Clamp(static_cast<int32>(C.What), 0, 4);
		const int32 P = FMath::Clamp(static_cast<int32>(C.Where), 0, 6);
		Out += FString::Printf(TEXT("  [hand %d] %-5s %-10s t%-2d %s: %s%s\n"), C.Tick, Kinds[K], Places[P], C.TableId, UTF8_TO_TCHAR(C.Speaker.c_str()),
			UTF8_TO_TCHAR((C.What == ss::room::Kind::Vocal ? C.Sound : C.Text).c_str()), C.Key.empty() ? TEXT("") : *FString::Printf(TEXT("  {%s}"), UTF8_TO_TCHAR(C.Key.c_str())));
	}
	return Out;
}

void ABackRoomGameMode::TestCue(const FString& Moment, int32 Seat)
{
	if (!RoomDirector.IsValid() || !RoomHost.IsValid() || !Tourney)
	{
		UE_LOG(LogEmbercrest, Warning, TEXT("ss.Live.Cue: only at the Embercrest with the field seated"));
		return;
	}
	ss::talk::Moment M = ss::talk::Moment::Small;
	if (!ss::talk::MomentFrom(std::string(TCHAR_TO_UTF8(*Moment)), M))
	{
		FString Names;
		for (int32 I = 0; I <= static_cast<int32>(ss::talk::Moment::Rail); ++I)
		{
			Names += (I > 0 ? TEXT(" ") : TEXT("")) + FString(UTF8_TO_TCHAR(ss::talk::MomentName(static_cast<ss::talk::Moment>(I))));
		}
		UE_LOG(LogEmbercrest, Warning, TEXT("ss.Live.Cue %s: not a moment (%s)"), *Moment, *Names);
		return;
	}
	// Said now, past the rates and caps but never the honesty gates; nothing the night keeps changes.
	if (!RoomDirector->Prompt(*Tourney, M, Seat))
	{
		UE_LOG(LogEmbercrest, Warning, TEXT("ss.Live.Cue %s %d: nobody would honestly say that now"), *Moment, Seat);
		return;
	}
	RoomFeed::Take(*RoomHost, *RoomDirector);
	UE_LOG(LogEmbercrest, Display, TEXT("ss.Live.Cue %s %d: said"), *Moment, Seat);
}
