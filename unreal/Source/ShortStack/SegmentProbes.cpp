// The next segment's Phase A spikes (next_segment_spec 8.2), as console commands for the lead's editor session. Testing only:
// nothing here runs unless asked, and none of it writes a save.
//
//   ss.Probe.CardRoom <seconds> [talkers] [tables] [saves] [out file] [wait s]              S-2, the card room's audio cost
//       At a card table (the Embercrest or Dee's): the room tone is swapped for a stereo prototype of the planned
//       ss::audio::CardRoom (a walla of two noise layers a side, formant talkers at three bandpasses each with their
//       coefficients set per syllable, chip clicks at the near tables and the feature table, a reverb of four combs and two
//       allpasses a side), rendered where the room tone renders today (UNightOneAudio::TickComponent, game thread, 25 ms
//       blocks a quarter second ahead). It reports the synthesis' ms per frame (average and peak), the queue's low water and
//       underruns (the queue found empty), and Saves background saves emulated as the room's checkpoint makes them (the
//       save copied on the game thread, serialized on a worker; nothing written) with the game thread's cost and the worst
//       frame after each. Then the room tone comes back.
//   ss.Probe.Overlay [0|1]                                                                  S-4, a Slate draw list over the card room
//       A core ss::ui draw list on an SDrawListWidget over the view, beside the legacy canvas HUD: the 1,080-tall canvas's
//       frame and its 58-unit margins, a plate projected at each opponent's stack, and the facts (viewport, logical width,
//       scale, who has the keyboard focus, the draw list's cost) in its corner and the log.
//   ss.Probe.Patch <out cm|off> [up cm] [side cm] [thick mm]                                S-3, a patch card on spine_05
//       A 7.6 x 5.1 cm block-out card on the hero's spine_05 (the street's hero, or the hero's body at the table), Out cm in
//       front of the bone along the body's facing, Up and Side as given, carried by the bone: walk, run and reach at V.
#include "BackRoomGameMode.h"
#include "BackRoomPlayer.h"
#include "BackRoomTable.h"
#include "BackRoomAmbience.h"
#include "CardRoomAmbience.h"
#include "CareerSave.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "NightOneAudio.h"
#include "RenderTimer.h"
#include "SlateDrawList.h"
#include "Tasks/Task.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/UI/Canvas.h"

#include <cmath>
#include <memory>
#include <vector>

DEFINE_LOG_CATEGORY_STATIC(LogProbe, Log, All);

namespace ProbeDetail
{
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

ABackRoomGameMode* TableMode()
{
	UWorld* W = GameWorld();
	return W ? W->GetAuthGameMode<ABackRoomGameMode>() : nullptr;
}

void Report(const FString& Text, const FString& OutFile)
{
	TArray<FString> Lines;
	Text.ParseIntoArrayLines(Lines, false);
	for (const FString& L : Lines)
	{
		UE_LOG(LogProbe, Display, TEXT("%s"), *L);
	}
	if (!OutFile.IsEmpty())
	{
		FFileHelper::SaveStringToFile(Text, *OutFile, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}
}

// ================================================================== S-2: the card room's audio cost

constexpr float Rate = 48000.0f;

/** A transposed direct form II biquad (bandpass and resonators). */
struct FBiquad
{
	float B0 = 0.0f, B1 = 0.0f, B2 = 0.0f, A1 = 0.0f, A2 = 0.0f, Z1 = 0.0f, Z2 = 0.0f;
	void BandPass(float Hz, float Q)
	{
		const float W = 2.0f * UE_PI * FMath::Clamp(Hz, 40.0f, Rate * 0.45f) / Rate;
		const float Alpha = FMath::Sin(W) / (2.0f * Q);
		const float A0 = 1.0f + Alpha;
		B0 = Alpha / A0;
		B1 = 0.0f;
		B2 = -Alpha / A0;
		A1 = -2.0f * FMath::Cos(W) / A0;
		A2 = (1.0f - Alpha) / A0;
	}
	FORCEINLINE float Run(float X)
	{
		const float Y = B0 * X + Z1;
		Z1 = B1 * X - A1 * Y + Z2;
		Z2 = B2 * X - A2 * Y;
		return Y;
	}
};

struct FComb
{
	std::vector<float> Buf;
	int32 I = 0;
	float Feedback = 0.78f;
	float Damp = 0.25f;
	float Store = 0.0f;
	FORCEINLINE float Run(float X)
	{
		const float Y = Buf[I];
		Store = Y * (1.0f - Damp) + Store * Damp;
		Buf[I] = X + Store * Feedback;
		I = I + 1 < static_cast<int32>(Buf.size()) ? I + 1 : 0;
		return Y;
	}
};

struct FAllpass
{
	std::vector<float> Buf;
	int32 I = 0;
	FORCEINLINE float Run(float X)
	{
		const float B = Buf[I];
		const float Y = B - X;
		Buf[I] = X + B * 0.5f;
		I = I + 1 < static_cast<int32>(Buf.size()) ? I + 1 : 0;
		return Y;
	}
};

/** The prototype: the planned CardRoom's voice counts at a cost close to what audio-synth will build (spec 4.4, 10). */
class FProbeRoom
{
public:
	FProbeRoom(int32 InTalkers, int32 InTables)
	{
		for (int32 K = 0; K < 2; ++K)
		{
			Walla[K][0].BandPass(520.0f, 0.8f);
			Walla[K][1].BandPass(1700.0f, 0.9f);
		}
		Talkers.SetNum(FMath::Clamp(InTalkers, 0, 16));
		for (int32 T = 0; T < Talkers.Num(); ++T)
		{
			FTalker& V = Talkers[T];
			V.F0 = 95.0f + 110.0f * Uniform();
			V.Pan = Uniform();
			NewSyllable(V);
		}
		Tables.SetNum(FMath::Clamp(InTables, 0, 16));
		for (FTable& Tb : Tables)
		{
			Tb.Pan = Uniform();
			Tb.Click.BandPass(3200.0f + 1800.0f * Uniform(), 9.0f);
			Tb.Next = 0.2f + Uniform();
		}
		static const int32 CombLen[4] = {1557, 1617, 1491, 1422};
		static const int32 PassLen[2] = {556, 441};
		for (int32 K = 0; K < 2; ++K)
		{
			for (int32 C = 0; C < 4; ++C)
			{
				Combs[K][C].Buf.assign(static_cast<size_t>(CombLen[C] + K * 23), 0.0f);
			}
			for (int32 A = 0; A < 2; ++A)
			{
				Passes[K][A].Buf.assign(static_cast<size_t>(PassLen[A] + K * 23), 0.0f);
			}
		}
	}

	/** Frames stereo frames, interleaved L/R. */
	void Render(float* Out, int32 Frames)
	{
		const float Dt = 1.0f / Rate;
		for (int32 F = 0; F < Frames; ++F)
		{
			Time += Dt;
			float L = 0.0f;
			float R = 0.0f;
			// The walla: two noise layers a side, breathing slowly.
			const float Breath = 0.6f + 0.4f * FMath::Sin(Time * 0.37f);
			for (int32 K = 0; K < 2; ++K)
			{
				const float N = Noise();
				const float W = (Walla[K][0].Run(N) * 0.6f + Walla[K][1].Run(N) * 0.3f) * Breath * 0.25f;
				(K == 0 ? L : R) += W;
			}
			// The talkers: a glottal saw through three formants; the formants move per syllable, never per sample.
			for (FTalker& V : Talkers)
			{
				V.Left -= Dt;
				if (V.Left <= 0.0f)
				{
					NewSyllable(V);
				}
				V.Phase += V.F0 * Dt;
				V.Phase -= FMath::FloorToFloat(V.Phase);
				const float G = (2.0f * V.Phase - 1.0f) * V.Env;
				const float S = (V.Formant[0].Run(G) + V.Formant[1].Run(G) * 0.6f + V.Formant[2].Run(G) * 0.35f) * 0.18f;
				L += S * (1.0f - V.Pan);
				R += S * V.Pan;
			}
			// The tables' chips: a click now and then through a resonance.
			for (FTable& Tb : Tables)
			{
				Tb.Next -= Dt;
				float Imp = 0.0f;
				if (Tb.Next <= 0.0f)
				{
					Imp = 0.7f;
					Tb.Next = 0.05f + 0.9f * Uniform();
				}
				const float C = Tb.Click.Run(Imp + Noise() * 0.0005f) * 0.4f;
				L += C * (1.0f - Tb.Pan);
				R += C * Tb.Pan;
			}
			// The room: four combs and two allpasses a side.
			const float SendL = L * 0.3f;
			const float SendR = R * 0.3f;
			float WetL = 0.0f;
			float WetR = 0.0f;
			for (int32 C = 0; C < 4; ++C)
			{
				WetL += Combs[0][C].Run(SendL);
				WetR += Combs[1][C].Run(SendR);
			}
			for (int32 A = 0; A < 2; ++A)
			{
				WetL = Passes[0][A].Run(WetL);
				WetR = Passes[1][A].Run(WetR);
			}
			Out[2 * F] = FMath::Clamp(L + WetL * 0.25f, -1.0f, 1.0f);
			Out[2 * F + 1] = FMath::Clamp(R + WetR * 0.25f, -1.0f, 1.0f);
		}
	}

private:
	struct FTalker
	{
		float F0 = 120.0f;
		float Phase = 0.0f;
		float Pan = 0.5f;
		float Left = 0.0f;
		float Env = 0.0f;
		FBiquad Formant[3];
	};
	struct FTable
	{
		float Pan = 0.5f;
		float Next = 0.0f;
		FBiquad Click;
	};

	void NewSyllable(FTalker& V)
	{
		static const float Vowels[5][3] = {{730, 1090, 2440}, {270, 2290, 3010}, {300, 870, 2240}, {530, 1840, 2480}, {570, 840, 2410}};
		const float* Fm = Vowels[FMath::Min(4, static_cast<int32>(Uniform() * 5.0f))];
		for (int32 K = 0; K < 3; ++K)
		{
			V.Formant[K].BandPass(Fm[K] * (0.9f + 0.2f * Uniform()), 6.0f + 4.0f * K);
		}
		V.Left = 0.12f + 0.2f * Uniform();
		V.Env = Uniform() < 0.25f ? 0.0f : 0.5f + 0.5f * Uniform();
	}
	FORCEINLINE float Uniform()
	{
		State = State * 1664525u + 1013904223u;
		return static_cast<float>(State >> 8) / 16777216.0f;
	}
	FORCEINLINE float Noise() { return 2.0f * Uniform() - 1.0f; }

	uint32 State = 0x5eed1234u;
	float Time = 0.0f;
	FBiquad Walla[2][2];
	TArray<FTalker> Talkers;
	TArray<FTable> Tables;
	FComb Combs[2][4];
	FAllpass Passes[2][2];
};

struct FAudioProbe
{
	TSharedPtr<FProbeRoom> Room;
	TWeakObjectPtr<UNightOneAudio> Audio;
	bool bLive = false;
	double Seconds = 30.0;
	double StartedAt = 0.0;
	FString Out;
	// Per frame.
	double FrameSynthMs = 0.0;
	int32 FrameBlocks = 0;
	// Totals.
	TArray<double> SynthMs;
	TArray<double> FrameMs;
	TArray<double> GameMs;
	int64 Blocks = 0;
	double BlockMsSum = 0.0;
	double BlockMsPeak = 0.0;
	int32 Underruns = 0;
	int32 Samples = 0;
	int32 MinQueued = TNumericLimits<int32>::Max();
	// The emulated saves.
	TSharedPtr<ss::SaveData> Saved;
	TArray<double> SaveAt;
	int32 NextSave = 0;
	TArray<double> SaveCopyMs;
	TArray<double> SaveWorstFrameMs;
	TArray<int32> SaveUnderruns;
	double WatchUntil = -1.0;
	FTSTicker::FDelegateHandle Ticker;
};

TSharedPtr<FAudioProbe> Probe;

FString Stats(const TArray<double>& V, const TCHAR* Unit)
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
	return FString::Printf(TEXT("avg %.3f %s, p50 %.3f, p95 %.3f, p99 %.3f, peak %.3f (n %d)"), Sum / S.Num(), Unit, S[S.Num() / 2], S[FMath::Min(S.Num() - 1, S.Num() * 95 / 100)],
		S[FMath::Min(S.Num() - 1, S.Num() * 99 / 100)], S.Last(), S.Num());
}

void FinishAudioProbe()
{
	if (!Probe)
	{
		return;
	}
	FAudioProbe& P = *Probe;
	FTSTicker::GetCoreTicker().RemoveTicker(P.Ticker);
	// The room tone back (a fresh one: the table's own keeps its crowd level, this starts at the default).
	if (UNightOneAudio* A = P.Audio.Get())
	{
		if (P.bLive)
		{
			TSharedPtr<FCardRoomAmbience> Tone = MakeShared<FCardRoomAmbience>(0xcafeu);
			A->SetAmbienceSource([Tone](float* Out, int32 N) { Tone->Render(Out, N); }, 0.4f);
		}
		else
		{
			TSharedPtr<FBackRoomAmbience> Tone = MakeShared<FBackRoomAmbience>(0x5eedu);
			A->SetAmbienceSource([Tone](float* Out, int32 N) { Tone->Render(Out, N); }, 0.35f);
		}
	}
	const double QueuedMs = P.MinQueued == TNumericLimits<int32>::Max() ? 0.0 : static_cast<double>(P.MinQueued) / (2.0 * 2.0 * Rate) * 1000.0;
	FString T = FString::Printf(TEXT("S-2 card room audio probe (%s, %.0f s, %d frames)\n"), P.bLive ? TEXT("the Embercrest") : TEXT("Dee's game"), P.Seconds, P.FrameMs.Num());
	T += FString::Printf(TEXT("synth per frame:   %s\n"), *Stats(P.SynthMs, TEXT("ms")));
	T += FString::Printf(TEXT("synth per block:   avg %.3f ms, peak %.3f ms over %lld blocks of 25 ms (budget: 0.35 ms avg, 0.8 ms peak per frame)\n"),
		P.Blocks > 0 ? P.BlockMsSum / static_cast<double>(P.Blocks) : 0.0, P.BlockMsPeak, P.Blocks);
	T += FString::Printf(TEXT("frame:             %s\n"), *Stats(P.FrameMs, TEXT("ms")));
	T += FString::Printf(TEXT("game thread:       %s\n"), *Stats(P.GameMs, TEXT("ms")));
	T += FString::Printf(TEXT("queue low water:   %.1f ms of stereo ahead (target 250); underruns (found empty) %d of %d samples\n"), QueuedMs, P.Underruns, P.Samples);
	for (int32 K = 0; K < P.SaveCopyMs.Num(); ++K)
	{
		T += FString::Printf(TEXT("save %d:            copy %.3f ms on the game thread, worst frame %.2f ms in the 2 s after, underruns %d\n"), K + 1, P.SaveCopyMs[K],
			P.SaveWorstFrameMs.IsValidIndex(K) ? P.SaveWorstFrameMs[K] : 0.0, P.SaveUnderruns.IsValidIndex(K) ? P.SaveUnderruns[K] : 0);
	}
	T += TEXT("Decision (spec 10): game-thread synthesis fits if the per-frame average is under 0.35 ms, the peak under 0.8 ms and no underrun; else ue-room takes the USynthComponent path.\n");
	Report(T, P.Out);
	Probe.Reset();
}

bool TickAudioProbe(float)
{
	if (!Probe)
	{
		return false;
	}
	FAudioProbe& P = *Probe;
	const double Now = FPlatformTime::Seconds();
	UNightOneAudio* A = P.Audio.Get();
	if (!A)
	{
		UE_LOG(LogProbe, Warning, TEXT("S-2: the table's audio went away (the level changed)"));
		Probe.Reset();
		return false;
	}
	// (The core ticker runs before the world's: the queue as the renderer left it, before this frame's top-up.)
	const int32 Queued = A->GetAmbienceQueuedBytes();
	const bool bWarm = Now - P.StartedAt > 1.0;
	if (bWarm)
	{
		++P.Samples;
		P.MinQueued = FMath::Min(P.MinQueued, Queued);
		if (Queued <= 0)
		{
			++P.Underruns;
			if (P.WatchUntil > Now && P.SaveUnderruns.Num() > 0)
			{
				++P.SaveUnderruns.Last();
			}
		}
		P.SynthMs.Add(P.FrameSynthMs);
		P.FrameMs.Add(FApp::GetDeltaTime() * 1000.0);
		P.GameMs.Add(FPlatformTime::ToMilliseconds(GGameThreadTime));
		if (P.WatchUntil > Now && P.SaveWorstFrameMs.Num() > 0)
		{
			P.SaveWorstFrameMs.Last() = FMath::Max(P.SaveWorstFrameMs.Last(), FApp::GetDeltaTime() * 1000.0);
		}
	}
	P.FrameSynthMs = 0.0;
	P.FrameBlocks = 0;
	// A background save, as the room's checkpoint makes one: the save copied here, serialized on a worker (never written).
	if (P.Saved && P.NextSave < P.SaveAt.Num() && Now - P.StartedAt >= P.SaveAt[P.NextSave])
	{
		++P.NextSave;
		const uint64 C0 = FPlatformTime::Cycles64();
		TSharedPtr<ss::SaveData> Copy = MakeShared<ss::SaveData>(*P.Saved);
		P.SaveCopyMs.Add(FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64() - C0));
		P.SaveWorstFrameMs.Add(0.0);
		P.SaveUnderruns.Add(0);
		P.WatchUntil = Now + 2.0;
		UE::Tasks::Launch(UE_SOURCE_LOCATION, [Copy]() { (void)Copy->Serialize(); });
	}
	if (Now - P.StartedAt >= P.Seconds + 1.0)
	{
		FinishAudioProbe();
		return false;
	}
	return true;
}

void StartAudioProbe(const TArray<FString>& Args);

/** A probe asked for before the table is up (Wait s after the hero is seated at one): its arguments, and since when seated. */
TArray<FString> PendingArgs;
double PendingSeatedAt = -1.0;
FTSTicker::FDelegateHandle PendingTicker;

bool TickPendingProbe(float)
{
	ABackRoomGameMode* GM = TableMode();
	const double Now = FPlatformTime::Seconds();
	if (!GM || GM->GetPhase() != EBackRoomPhase::Playing || !GM->GetWorld() || !GM->GetWorld()->HasBegunPlay())
	{
		PendingSeatedAt = -1.0;
		return true;
	}
	if (PendingSeatedAt < 0.0)
	{
		PendingSeatedAt = Now;
	}
	const double Wait = PendingArgs.Num() > 5 ? FCString::Atod(*PendingArgs[5]) : 0.0;
	if (Now - PendingSeatedAt < Wait)
	{
		return true;
	}
	TArray<FString> Args = PendingArgs;
	Args.SetNum(FMath::Min(Args.Num(), 5));
	PendingArgs.Reset();
	StartAudioProbe(Args);
	return false;
}

void StartAudioProbe(const TArray<FString>& Args)
{
	if (Probe)
	{
		UE_LOG(LogProbe, Warning, TEXT("S-2 is already running"));
		return;
	}
	if (Args.Num() > 5)
	{
		// Wait: start once the hero has sat that long at a card table (from -ExecCmds, before the room is open).
		PendingArgs = Args;
		PendingSeatedAt = -1.0;
		PendingTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TickPendingProbe), 0.0f);
		UE_LOG(LogProbe, Display, TEXT("S-2 waits for a seated card table (%s s after)"), *Args[5]);
		return;
	}
	ABackRoomGameMode* GM = TableMode();
	UNightOneAudio* Audio = GM && GM->Table ? GM->Table->GetAudio() : nullptr;
	if (!Audio)
	{
		UE_LOG(LogProbe, Warning, TEXT("S-2: only at a card table (the Embercrest or Dee's game)"));
		return;
	}
	Probe = MakeShared<FAudioProbe>();
	FAudioProbe& P = *Probe;
	P.Seconds = Args.Num() > 0 ? FMath::Max(5.0, FCString::Atod(*Args[0])) : 30.0;
	const int32 Talkers = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 6;
	const int32 Tables = Args.Num() > 2 ? FCString::Atoi(*Args[2]) : 5;
	const int32 Saves = Args.Num() > 3 ? FMath::Clamp(FCString::Atoi(*Args[3]), 0, 8) : 2;
	P.Out = Args.Num() > 4 ? Args[4] : FPaths::ProjectSavedDir() / TEXT("Profiling/ShortStack/S2_cardroom_audio.txt");
	P.Room = MakeShared<FProbeRoom>(Talkers, Tables);
	P.Audio = Audio;
	P.bLive = GM->IsLive();
	// The save the emulated checkpoints copy (read now, before the clock starts).
	if (Saves > 0)
	{
		std::string Text;
		TSharedPtr<ss::SaveData> D = MakeShared<ss::SaveData>();
		if (CareerSave::LoadText(Text) && ss::SaveData::Parse(Text, *D))
		{
			P.Saved = D;
			for (int32 K = 0; K < Saves; ++K)
			{
				P.SaveAt.Add(1.0 + P.Seconds * (K + 1) / (Saves + 1));
			}
		}
	}
	TWeakPtr<FAudioProbe> Weak = Probe;
	TSharedPtr<FProbeRoom> Room = P.Room;
	Audio->SetAmbienceSourceStereo(
		[Weak, Room](float* Out, int32 Frames) {
			const uint64 C0 = FPlatformTime::Cycles64();
			Room->Render(Out, Frames);
			const double Ms = FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64() - C0);
			if (TSharedPtr<FAudioProbe> Pr = Weak.Pin())
			{
				Pr->FrameSynthMs += Ms;
				++Pr->FrameBlocks;
				++Pr->Blocks;
				Pr->BlockMsSum += Ms;
				Pr->BlockMsPeak = FMath::Max(Pr->BlockMsPeak, Ms);
			}
		},
		0.4f);
	P.StartedAt = FPlatformTime::Seconds();
	P.Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TickAudioProbe), 0.0f);
	UE_LOG(LogProbe, Display, TEXT("S-2: %d talkers, %d tables, %d saves, %.0f s; writing %s"), Talkers, Tables, Saves, P.Seconds, *P.Out);
}

// ================================================================== S-4: a Slate draw list over the card room

struct FOverlay
{
	TSharedPtr<SDrawListWidget> Widget;
	TWeakObjectPtr<UGameViewportClient> Viewport;
	TWeakObjectPtr<UWorld> ForWorld;
	FSlateTextMeasurer Measurer;
	FDrawListHint Hint;
	FTSTicker::FDelegateHandle Ticker;
	TArray<double> BuildMs;
	double LastLog = 0.0;
};

TSharedPtr<FOverlay> Overlay;

void StopOverlay()
{
	if (!Overlay)
	{
		return;
	}
	FTSTicker::GetCoreTicker().RemoveTicker(Overlay->Ticker);
	if (UGameViewportClient* V = Overlay->Viewport.Get())
	{
		if (Overlay->Widget)
		{
			V->RemoveViewportWidgetContent(Overlay->Widget.ToSharedRef());
		}
	}
	UE_LOG(LogProbe, Display, TEXT("S-4 overlay off; building its draw list: %s"), *Stats(Overlay->BuildMs, TEXT("ms")));
	Overlay.Reset();
}

bool TickOverlay(float)
{
	if (!Overlay)
	{
		return false;
	}
	FOverlay& O = *Overlay;
	// A level change takes the viewport's widgets off: put it back on the new world's viewport.
	UWorld* World = GameWorld();
	if (!World)
	{
		StopOverlay();
		return false;
	}
	if (World != O.ForWorld.Get())
	{
		if (UGameViewportClient* Old = O.Viewport.Get())
		{
			Old->RemoveViewportWidgetContent(O.Widget.ToSharedRef());
		}
		O.Viewport = World->GetGameViewport();
		O.ForWorld = World;
		if (UGameViewportClient* New = O.Viewport.Get())
		{
			New->AddViewportWidgetContent(O.Widget.ToSharedRef(), 10);
		}
	}
	UGameViewportClient* V = O.Viewport.Get();
	ABackRoomGameMode* GM = TableMode();
	if (!V || !GM || !World->HasBegunPlay())
	{
		return true; // (between levels)
	}
	const uint64 C0 = FPlatformTime::Cycles64();
	FVector2D Size;
	V->GetViewportSize(Size);
	if (Size.X < 8.0 || Size.Y < 8.0)
	{
		return true;
	}
	// The HUD's canvas: 1,080 tall, as wide as the window's aspect (1,920 at 16:9, 2,257 at 2552 x 1222).
	const float H = 1080.0f;
	const float Scale = static_cast<float>(Size.Y) / H;
	const float W = static_cast<float>(Size.X) / Scale;
	TSharedPtr<ss::ui::DrawList> List = O.Hint.Make();
	ss::ui::Canvas C(*List, O.Measurer, W, H, Scale);
	const ss::ui::Color Ember = ss::ui::Hex(0xe8703a);
	const ss::ui::Color Ash = ss::ui::Hex(0xe6e1d8);
	const ss::ui::Color Navy = ss::ui::Hex(0x0e1726, 0.82f);
	// The canvas and its margins.
	C.StrokeRoundRect({2.0f, 2.0f, W - 4.0f, H - 4.0f}, 0.0f, ss::ui::Hex(0xe8703a, 0.5f), 2.0f);
	C.StrokeRoundRect({58.0f, 58.0f, W - 116.0f, H - 116.0f}, 6.0f, ss::ui::Hex(0x5fb3b3, 0.45f), 1.5f);
	// The plates' clamp (spec 6.3: x 58..W-58-188, y 330..760).
	C.StrokeRoundRect({58.0f, 330.0f, W - 116.0f, 430.0f}, 4.0f, ss::ui::Hex(0x5fb3b3, 0.25f), 1.0f);
	// A plate at each opponent's stack, projected.
	APlayerController* PC = GM->GetWorld() ? GM->GetWorld()->GetFirstPlayerController() : nullptr;
	int32 Plates = 0;
	if (PC && GM->Table)
	{
		for (ABackRoomPlayer* P : GM->Table->GetOpponents())
		{
			if (!P || P->IsHidden())
			{
				continue;
			}
			FVector2D Px;
			const bool bOn = PC->ProjectWorldLocationToScreen(P->Spots.Stack + FVector(0.0, 0.0, 4.0), Px, true);
			const float X = FMath::Clamp(static_cast<float>(Px.X) / Scale - 94.0f, 58.0f, W - 58.0f - 188.0f);
			const float Y = FMath::Clamp(static_cast<float>(Px.Y) / Scale - 23.0f, 330.0f, 760.0f);
			C.FillRoundRect({X, Y, 188.0f, 46.0f}, 6.0f, ss::ui::Paint(Navy));
			C.StrokeRoundRect({X, Y, 188.0f, 46.0f}, 6.0f, bOn ? Ember : ss::ui::Hex(0x888888), 1.5f);
			ss::ui::TextStyle Name;
			Name.Size = 17.0f;
			Name.Weight = 800;
			Name.Col = Ash;
			Name.MaxWidth = 172.0f;
			C.Text(std::string(TCHAR_TO_UTF8(*P->Persona.Name)), X + 8.0f, Y + 20.0f, Name);
			ss::ui::TextStyle Stack;
			Stack.Size = 16.0f;
			Stack.Weight = 700;
			Stack.Mono = true;
			Stack.Col = Ember;
			C.Text(bOn ? "PLATE (S-4)" : "OFF SCREEN", X + 8.0f, Y + 40.0f, Stack);
			++Plates;
		}
	}
	// The facts.
	const TSharedPtr<SWidget> Focus = FSlateApplication::IsInitialized() ? FSlateApplication::Get().GetUserFocusedWidget(0) : nullptr;
	const FString Facts = FString::Printf(TEXT("S-4  viewport %.0f x %.0f  canvas %.0f x 1080  scale %.4f  plates %d  focus %s"), Size.X, Size.Y, W, Scale, Plates,
		Focus ? *Focus->GetTypeAsString() : TEXT("none"));
	ss::ui::TextStyle Info;
	Info.Size = 15.0f;
	Info.Mono = true;
	Info.Col = Ash;
	Info.HAlign = ss::ui::Align::Right;
	C.FillRoundRect({W - 58.0f - 760.0f, H - 58.0f - 34.0f, 760.0f, 30.0f}, 4.0f, ss::ui::Paint(Navy));
	C.Text(std::string(TCHAR_TO_UTF8(*Facts)), W - 58.0f - 10.0f, H - 58.0f - 13.0f, Info);
	O.Hint.Note(*List);
	O.Widget->SetDrawList(List);
	O.BuildMs.Add(FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64() - C0));
	const double Now = FPlatformTime::Seconds();
	if (Now - O.LastLog > 5.0)
	{
		O.LastLog = Now;
		UE_LOG(LogProbe, Display, TEXT("%s; draw list %s"), *Facts, *Stats(O.BuildMs, TEXT("ms")));
	}
	return true;
}

void StartOverlay()
{
	if (Overlay)
	{
		return;
	}
	UWorld* World = GameWorld();
	UGameViewportClient* V = World ? World->GetGameViewport() : nullptr;
	if (!V || !FSlateApplication::IsInitialized())
	{
		UE_LOG(LogProbe, Warning, TEXT("S-4: start the game first (it draws at a card table, and follows the game into one)"));
		return;
	}
	Overlay = MakeShared<FOverlay>();
	Overlay->Viewport = V;
	Overlay->ForWorld = World;
	Overlay->Widget = SNew(SDrawListWidget);
	// As the street's HUD: never under the pointer, never focused; the pawn keeps the keys (z 10, over the canvas HUD).
	Overlay->Widget->SetVisibility(EVisibility::HitTestInvisible);
	V->AddViewportWidgetContent(Overlay->Widget.ToSharedRef(), 10);
	Overlay->Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TickOverlay), 0.0f);
	UE_LOG(LogProbe, Display, TEXT("S-4 overlay on (ss.Probe.Overlay 0 takes it off)"));
}

// ================================================================== S-3: a patch card on spine_05

TWeakObjectPtr<UStaticMeshComponent> PatchCard;

/** The hero's body (a skeletal mesh with spine_05): the street's character, or at the table the hero's MetaHuman. */
USkeletalMeshComponent* HeroBody(AActor*& OutOwner)
{
	UWorld* World = GameWorld();
	OutOwner = nullptr;
	if (!World)
	{
		return nullptr;
	}
	TArray<AActor*> Look;
	if (ABackRoomGameMode* GM = World->GetAuthGameMode<ABackRoomGameMode>())
	{
		Look.Add(GM->Hero.Get());
	}
	else
	{
		Look.Add(UGameplayStatics::GetPlayerPawn(World, 0));
	}
	for (int32 I = 0; I < Look.Num(); ++I)
	{
		AActor* A = Look[I];
		if (!A)
		{
			continue;
		}
		TArray<USkeletalMeshComponent*> Meshes;
		A->GetComponents(Meshes);
		USkeletalMeshComponent* Best = nullptr;
		for (USkeletalMeshComponent* M : Meshes)
		{
			if (M && M->GetSkeletalMeshAsset() && M->GetBoneIndex(TEXT("spine_05")) != INDEX_NONE && (!Best || M->GetName().Contains(TEXT("Body"))))
			{
				Best = M;
			}
		}
		if (Best)
		{
			OutOwner = Look[0];
			return Best;
		}
		TArray<AActor*> Attached;
		A->GetAttachedActors(Attached);
		Look.Append(Attached);
	}
	return nullptr;
}

void Patch(const TArray<FString>& Args)
{
	if (Args.Num() > 0 && Args[0] == TEXT("off"))
	{
		if (UStaticMeshComponent* C = PatchCard.Get())
		{
			C->DestroyComponent();
		}
		PatchCard.Reset();
		UE_LOG(LogProbe, Display, TEXT("S-3: the card is off"));
		return;
	}
	AActor* Owner = nullptr;
	USkeletalMeshComponent* Body = HeroBody(Owner);
	if (!Body || !Owner)
	{
		UE_LOG(LogProbe, Warning, TEXT("S-3: no hero body with spine_05 here (the street, or a card table)"));
		return;
	}
	const float Out = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 12.0f;
	const float Up = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 0.0f;
	const float Side = Args.Num() > 2 ? FCString::Atof(*Args[2]) : 0.0f;
	const float Thick = Args.Num() > 3 ? FCString::Atof(*Args[3]) : 2.5f;
	UStaticMeshComponent* Card = PatchCard.Get();
	if (!Card || Card->GetOwner() != Body->GetOwner())
	{
		Card = NewObject<UStaticMeshComponent>(Body->GetOwner());
		Card->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		Card->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Card->SetCastShadow(false);
		Card->RegisterComponent();
		if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
		{
			UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Base, Card);
			M->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 0.35f, 0.05f));
			Card->SetMaterial(0, M);
		}
		PatchCard = Card;
	}
	// Placed in the body's facing (the actor's forward, as the pose stands now), then carried by the bone. The cube is a metre a side.
	const FTransform Bone = Body->GetSocketTransform(TEXT("spine_05"), RTS_World);
	const FVector Forward = Owner->GetActorForwardVector();
	const FVector Right = Owner->GetActorRightVector();
	const FVector At = Bone.GetLocation() + Forward * Out + FVector::UpVector * Up + Right * Side;
	Card->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	Card->SetWorldTransform(FTransform(Forward.Rotation(), At, FVector(Thick / 1000.0f, 0.076f, 0.051f)));
	Card->AttachToComponent(Body, FAttachmentTransformRules::KeepWorldTransform, TEXT("spine_05"));
	UE_LOG(LogProbe, Display, TEXT("S-3: card on %s.%s (spine_05) out %.1f cm, up %.1f, side %.1f, %.1f mm thick; relative %s"), *Body->GetOwner()->GetName(), *Body->GetName(), Out, Up,
		Side, Thick, *Card->GetRelativeTransform().ToString());
}

FAutoConsoleCommand CardRoomCmd(TEXT("ss.Probe.CardRoom"),
	TEXT("ss.Probe.CardRoom <seconds> [talkers=6] [tables=5] [saves=2] [out file] [wait s]: S-2, a prototype of the card room's stereo synthesis rendered where the room tone renders, timed (with Wait: once the hero has sat that long)."),
	FConsoleCommandWithArgsDelegate::CreateStatic(&StartAudioProbe));
FAutoConsoleCommand OverlayCmd(TEXT("ss.Probe.Overlay"), TEXT("ss.Probe.Overlay [0|1]: S-4, a core draw list on Slate over the card room beside the canvas HUD."),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args) {
		if (Args.Num() > 0 && Args[0] == TEXT("0"))
		{
			StopOverlay();
		}
		else
		{
			StartOverlay();
		}
	}));
FAutoConsoleCommand PatchCmd(TEXT("ss.Probe.Patch"), TEXT("ss.Probe.Patch <out cm|off> [up cm] [side cm] [thick mm]: S-3, a block-out patch card carried by the hero's spine_05."),
	FConsoleCommandWithArgsDelegate::CreateStatic(&Patch));
} // namespace ProbeDetail
