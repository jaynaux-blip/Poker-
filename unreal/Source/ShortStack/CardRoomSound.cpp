// The card room, heard (CardRoomSound.h): the room rendered ahead on a worker, the speech jobs, the VoiceBank seam, the
// director's feed and the listening gate's recorder.

#include "CardRoomSound.h"

#include "CardRoomAmbience.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "NightOneAudio.h"
#include "Sound/SoundWave.h"
#include "ShortStack/Tournament.h"

#include <algorithm>
#include <cmath>

DEFINE_LOG_CATEGORY_STATIC(LogCardRoomSound, Log, All);

namespace CardRoomSoundDetail
{
constexpr int32 Rate = 48000;
/** The worker renders a 25 ms block at a time, and keeps 100 ms ahead of the stream. */
constexpr int32 BlockFrames = 1200;
constexpr int32 TargetFrames = 4800;
/** Half a second of ring: room for a stalled frame or two. */
constexpr int32 RingFrames = 24000;

/** 16-bit PCM WAV, interleaved. */
bool WriteWav(const FString& Path, const std::vector<float>& Samples, int32 Channels)
{
	TArray64<uint8> Bytes;
	const int64 DataBytes = static_cast<int64>(Samples.size()) * 2;
	Bytes.Reserve(44 + DataBytes);
	auto U32 = [&Bytes](uint32 V) {
		for (int32 I = 0; I < 4; ++I)
		{
			Bytes.Add(static_cast<uint8>((V >> (8 * I)) & 0xff));
		}
	};
	auto U16 = [&Bytes](uint16 V) {
		Bytes.Add(static_cast<uint8>(V & 0xff));
		Bytes.Add(static_cast<uint8>((V >> 8) & 0xff));
	};
	auto Tag = [&Bytes](const char* T) {
		for (int32 I = 0; I < 4; ++I)
		{
			Bytes.Add(static_cast<uint8>(T[I]));
		}
	};
	Tag("RIFF");
	U32(static_cast<uint32>(36 + DataBytes));
	Tag("WAVE");
	Tag("fmt ");
	U32(16);
	U16(1);
	U16(static_cast<uint16>(Channels));
	U32(Rate);
	U32(static_cast<uint32>(Rate * Channels * 2));
	U16(static_cast<uint16>(Channels * 2));
	U16(16);
	Tag("data");
	U32(static_cast<uint32>(DataBytes));
	for (const float S : Samples)
	{
		const int16 V = static_cast<int16>(FMath::Clamp(S, -1.0f, 1.0f) * 32767.0f);
		U16(static_cast<uint16>(V));
	}
	return FFileHelper::SaveArrayToFile(Bytes, *Path);
}

/** RMS over the line's speaking parts (10 ms frames within 30 dB of its loudest), in dBFS. */
float ActiveDb(const std::vector<float>& S)
{
	const int32 Frame = Rate / 100;
	std::vector<double> Powers;
	double Loudest = 0.0;
	for (size_t At = 0; At + static_cast<size_t>(Frame) <= S.size(); At += static_cast<size_t>(Frame))
	{
		double P = 0.0;
		for (int32 I = 0; I < Frame; ++I)
		{
			P += static_cast<double>(S[At + static_cast<size_t>(I)]) * S[At + static_cast<size_t>(I)];
		}
		P /= Frame;
		Powers.push_back(P);
		Loudest = FMath::Max(Loudest, P);
	}
	double Sum = 0.0;
	int32 N = 0;
	for (const double P : Powers)
	{
		if (P >= Loudest * 0.001)
		{
			Sum += P;
			++N;
		}
	}
	return N > 0 && Sum > 0.0 ? static_cast<float>(10.0 * std::log10(Sum / N)) : -120.0f;
}
} // namespace CardRoomSoundDetail

using namespace CardRoomSoundDetail;

// ------------------------------------------------------------------ the stream

struct FCardRoomSound::FRecording
{
	FString Name;
	FString Dir;
	int64 Target = 0; // frames
	float Crowd = 0.7f;
	std::vector<float> Stream; // interleaved, as the stream pulled it
	struct FOverlay
	{
		int64 At = 0; // stream frame it was heard at
		std::vector<float> Mono;
		float L = 0.7f;
		float R = 0.7f;
	};
	std::vector<FOverlay> Overlays;
};

FCardRoomSound::FCardRoomSound(uint32 Seed, int32 Talkers)
{
	Room = MakeUnique<ss::audio::CardRoom>(Seed);
	Budget = FMath::Clamp(Talkers, 0, 12);
	Room->SetVoiceBudget(Budget);
	Ring.SetNumZeroed(RingFrames * 2);
}

FCardRoomSound::~FCardRoomSound()
{
	// No wait here: a render in flight holds its own reference, so the room outlives it (and this may be that render's
	// last reference, on its worker).
}

void FCardRoomSound::Queue(TFunction<void(ss::audio::CardRoom&)> Fn)
{
	Pending.Add(MoveTemp(Fn));
}

void FCardRoomSound::SetEmitters(const std::vector<Emitter>& Emitters)
{
	Queue([Emitters](ss::audio::CardRoom& R) { R.SetEmitters(Emitters); });
}

void FCardRoomSound::SetListener(float X, float Y, float Yaw)
{
	Queue([X, Y, Yaw](ss::audio::CardRoom& R) { R.SetListener(X, Y, Yaw); });
}

void FCardRoomSound::SetMix(const ss::room::Mix& Mix, float CallsOffsetDb)
{
	const ss::room::Mix M = Mix;
	const float Calls = M.Calls <= -59.0f ? M.Calls : M.Calls + CallsOffsetDb;
	Queue([M, Calls](ss::audio::CardRoom& R) { R.SetMix(M.Bed, M.Talk, M.Chips, Calls, M.Rail, M.Crowd); });
}

void FCardRoomSound::SetPaLevel(float Db)
{
	Queue([Db](ss::audio::CardRoom& R) { R.SetPaLevel(Db); });
}

void FCardRoomSound::SetMuffle(float CutoffHz, float Db)
{
	Queue([CutoffHz, Db](ss::audio::CardRoom& R) { R.SetMuffle(CutoffHz, Db); });
}

void FCardRoomSound::SetPace(float EventClock)
{
	Queue([EventClock](ss::audio::CardRoom& R) { R.SetPace(EventClock); });
}

void FCardRoomSound::Duck(float Db, float Seconds)
{
	Queue([Db, Seconds](ss::audio::CardRoom& R) { R.Duck(Db, Seconds); });
}

void FCardRoomSound::TableHand(int32 Id, float Seconds, bool bAllIn, bool bCalled, float PotBb)
{
	Queue([Id, Seconds, bAllIn, bCalled, PotBb](ss::audio::CardRoom& R) { R.TableHand(Id, Seconds, bAllIn, bCalled, PotBb); });
}

void FCardRoomSound::Say(const ss::audio::SpeechPlan& Plan, const ss::audio::Timbre& Voice, Spot Where, int32 Id, float Effort, bool bMurmur)
{
	Queue([Plan, Voice, Where, Id, Effort, bMurmur](ss::audio::CardRoom& R) { R.Say(Plan, Voice, Where, Id, Effort, bMurmur); });
}

void FCardRoomSound::Crowd(ss::audio::Vocal V, int32 Voices, Spot Where, int32 Id, float Level)
{
	Queue([V, Voices, Where, Id, Level](ss::audio::CardRoom& R) { R.Crowd(V, Voices, Where, Id, Level); });
}

void FCardRoomSound::SetVoiceBudget(int32 Talkers)
{
	Budget = FMath::Clamp(Talkers, 0, 12);
	const int32 B = Budget;
	Queue([B](ss::audio::CardRoom& R) { R.SetVoiceBudget(B); });
}

void FCardRoomSound::RenderAhead(TArray<TFunction<void(ss::audio::CardRoom&)>>& Batch)
{
	FScopeLock Lock(&RoomLock);
	for (TFunction<void(ss::audio::CardRoom&)>& Fn : Batch)
	{
		Fn(*Room);
	}
	float Block[BlockFrames * 2];
	for (int32 Guard = 0; Guard < RingFrames / BlockFrames; ++Guard)
	{
		{
			FScopeLock R(&RingLock);
			if (RingCount + BlockFrames > TargetFrames || RingCount + BlockFrames > RingFrames)
			{
				break;
			}
		}
		const uint64 C0 = FPlatformTime::Cycles64();
		Room->RenderStereo(Block, BlockFrames);
		const double Ms = FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64() - C0);
		FScopeLock R(&RingLock);
		int32 Write = (RingRead + RingCount) % RingFrames;
		for (int32 I = 0; I < BlockFrames; ++I)
		{
			Ring[2 * Write] = Block[2 * I];
			Ring[2 * Write + 1] = Block[2 * I + 1];
			Write = (Write + 1) % RingFrames;
		}
		RingCount += BlockFrames;
		BlockMsSum += Ms;
		BlockMsPeak = FMath::Max(BlockMsPeak, Ms);
		++Blocks;
	}
}

void FCardRoomSound::Kick()
{
	if (!InFlight.IsCompleted())
	{
		return;
	}
	int32 Have = 0;
	{
		FScopeLock R(&RingLock);
		Have = RingCount;
	}
	if (Pending.Num() == 0 && Have + BlockFrames > TargetFrames)
	{
		return;
	}
	TSharedPtr<TArray<TFunction<void(ss::audio::CardRoom&)>>, ESPMode::ThreadSafe> Batch = MakeShared<TArray<TFunction<void(ss::audio::CardRoom&)>>, ESPMode::ThreadSafe>(MoveTemp(Pending));
	Pending.Reset();
	TSharedRef<FCardRoomSound, ESPMode::ThreadSafe> Self = AsShared();
	InFlight = UE::Tasks::Launch(UE_SOURCE_LOCATION, [Self, Batch]() { Self->RenderAhead(*Batch); });
}

void FCardRoomSound::Pump()
{
	Kick();
}

void FCardRoomSound::Pull(float* Out, int32 Frames)
{
	if (!Out || Frames <= 0)
	{
		return;
	}
	auto TakeRing = [this, Out, Frames](int32 Got) {
		FScopeLock R(&RingLock);
		const int32 N = FMath::Min(Frames - Got, RingCount);
		for (int32 I = 0; I < N; ++I)
		{
			Out[2 * (Got + I)] = Ring[2 * RingRead];
			Out[2 * (Got + I) + 1] = Ring[2 * RingRead + 1];
			RingRead = (RingRead + 1) % RingFrames;
		}
		RingCount -= N;
		return Got + N;
	};
	int32 Got = TakeRing(0);
	if (Got < Frames)
	{
		// Short: the render in flight finishes first (its setters are older than the ones queued since, and its samples come
		// first), then the rest is rendered here, after it, with the setters made since: nothing plays out of order.
		InFlight.Wait();
		FScopeLock Lock(&RoomLock);
		Got = TakeRing(Got);
		if (Got < Frames)
		{
			const uint64 C0 = FPlatformTime::Cycles64();
			for (TFunction<void(ss::audio::CardRoom&)>& Fn : Pending)
			{
				Fn(*Room);
			}
			Pending.Reset();
			Room->RenderStereo(Out + 2 * Got, Frames - Got);
			const double Ms = FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64() - C0);
			FScopeLock R(&RingLock);
			++SyncRenders;
			SyncMsPeak = FMath::Max(SyncMsPeak, Ms);
		}
	}
	if (Recording.IsValid())
	{
		FRecording& Rec = *Recording;
		Rec.Stream.insert(Rec.Stream.end(), Out, Out + 2 * Frames);
		if (static_cast<int64>(Rec.Stream.size()) >= Rec.Target * 2)
		{
			FinishRecording();
		}
	}
	Kick();
}

double FCardRoomSound::RingSeconds() const
{
	FScopeLock R(&RingLock);
	return static_cast<double>(RingCount) / Rate;
}

ss::audio::CardRoom::Meter FCardRoomSound::Measure() const
{
	FScopeLock Lock(&RoomLock);
	return Room->Measure();
}

FCardRoomSound::FStats FCardRoomSound::Stats() const
{
	FScopeLock R(&RingLock);
	FStats S;
	S.Blocks = Blocks;
	S.BlockMsAvg = Blocks > 0 ? BlockMsSum / Blocks : 0.0;
	S.BlockMsPeak = BlockMsPeak;
	S.WorkerMsPerSecond = S.BlockMsAvg * (static_cast<double>(Rate) / BlockFrames);
	S.SyncRenders = SyncRenders;
	S.SyncMsPeak = SyncMsPeak;
	return S;
}

// ------------------------------------------------------------------ the listening gate's recorder

bool FCardRoomSound::StartRecording(const FString& Name, const FString& Dir, double Seconds, float Crowd)
{
	if (Recording.IsValid())
	{
		return false;
	}
	Recording = MakeShared<FRecording, ESPMode::ThreadSafe>();
	Recording->Name = Name;
	Recording->Dir = Dir;
	Recording->Target = static_cast<int64>(FMath::Clamp(Seconds, 1.0, 600.0) * Rate);
	Recording->Crowd = Crowd;
	Recording->Stream.reserve(static_cast<size_t>(Recording->Target * 2 + BlockFrames * 32));
	return true;
}

void FCardRoomSound::RecordOverlay(const std::vector<float>& Mono, float GainL, float GainR, int32 QueuedFrames)
{
	if (!Recording.IsValid() || Mono.empty())
	{
		return;
	}
	FRecording::FOverlay O;
	O.At = FMath::Max<int64>(0, static_cast<int64>(Recording->Stream.size() / 2) - QueuedFrames);
	O.Mono = Mono;
	O.L = GainL;
	O.R = GainR;
	Recording->Overlays.push_back(MoveTemp(O));
}

void FCardRoomSound::FinishRecording()
{
	TSharedPtr<FRecording, ESPMode::ThreadSafe> Rec = Recording;
	Recording.Reset();
	// Mixed and written off the game thread, with today's bed rendered beside it.
	UE::Tasks::Launch(UE_SOURCE_LOCATION, [Rec]() {
		std::vector<float>& S = Rec->Stream;
		S.resize(static_cast<size_t>(Rec->Target * 2));
		for (const FRecording::FOverlay& O : Rec->Overlays)
		{
			for (size_t I = 0; I < O.Mono.size(); ++I)
			{
				const int64 F = O.At + static_cast<int64>(I);
				if (F >= Rec->Target)
				{
					break;
				}
				S[static_cast<size_t>(2 * F)] += O.Mono[I] * O.L;
				S[static_cast<size_t>(2 * F + 1)] += O.Mono[I] * O.R;
			}
		}
		IFileManager::Get().MakeDirectory(*Rec->Dir, true);
		const FString Room = FPaths::Combine(Rec->Dir, FString::Printf(TEXT("pie_%s.wav"), *Rec->Name));
		const bool bRoom = WriteWav(Room, S, 2);
		// Today's bed (CardRoomAmbience.h at its 0.4 send, both ears), the same length, for the A/B.
		FCardRoomAmbience Bed(0xcafeu);
		Bed.SetCrowd(Rec->Crowd);
		std::vector<float> Mono(static_cast<size_t>(Rec->Target));
		for (int64 At = 0; At < Rec->Target; At += BlockFrames)
		{
			Bed.Render(Mono.data() + At, static_cast<int32>(FMath::Min<int64>(BlockFrames, Rec->Target - At)));
		}
		std::vector<float> Stereo(Mono.size() * 2);
		for (size_t I = 0; I < Mono.size(); ++I)
		{
			Stereo[2 * I] = Stereo[2 * I + 1] = Mono[I] * 0.4f;
		}
		const FString BedPath = FPaths::Combine(Rec->Dir, FString::Printf(TEXT("pie_%s_bed.wav"), *Rec->Name));
		const bool bBed = WriteWav(BedPath, Stereo, 2);
		UE_LOG(LogCardRoomSound, Display, TEXT("Listening gate: %s (%s), %s (%s); %.1f s, %d voices over the stream"), *Room, bRoom ? TEXT("written") : TEXT("FAILED"), *BedPath,
			bBed ? TEXT("written") : TEXT("FAILED"), static_cast<double>(Rec->Target) / Rate, static_cast<int32>(Rec->Overlays.size()));
	});
}

// ------------------------------------------------------------------ the VoiceBank seam

FString FVoiceBank::KeyOf(const FString& Key, int32 Part, const FString& Speaker)
{
	return FString::Printf(TEXT("%s#%d@%s"), *Key, Part, *Speaker);
}

void FVoiceBank::Add(const FString& Key, int32 Part, const FString& Speaker, const FSoftObjectPath& Clip)
{
	Clips.Add(KeyOf(Key, Part, Speaker), Clip);
}

USoundWave* FVoiceBank::Find(const std::string& Key, int32 Part, const std::string& Speaker) const
{
	if (Clips.Num() == 0)
	{
		return nullptr; // this segment: the murmur, always
	}
	const FString K = UTF8_TO_TCHAR(Key.c_str());
	const FSoftObjectPath* Path = Clips.Find(KeyOf(K, Part, UTF8_TO_TCHAR(Speaker.c_str())));
	Path = Path ? Path : Clips.Find(KeyOf(K, Part, FString()));
	return Path ? Cast<USoundWave>(Path->TryLoad()) : nullptr;
}

// ------------------------------------------------------------------ speech, off the game thread

void FRoomSpeech::Run()
{
	const uint64 C0 = FPlatformTime::Cycles64();
	Plan = ss::audio::PlanSpeech(Text, Voice, Effort, Seed);
	if (bRender && !Plan.Syllables.empty())
	{
		ss::audio::SpeechPlan Said = Plan;
		if (bBreath)
		{
			// The fallback (spec 4.10): no voice, only the breath of the line under its caption.
			for (ss::audio::Syllable& S : Said.Syllables)
			{
				S.Air = 1.0f;
				S.Gain *= 0.7f;
			}
		}
		Samples = ss::audio::RenderSpeech(Said, Voice, bMurmur || bBreath, Seed);
		const float Db = ActiveDb(Samples);
		if (Db > -100.0f)
		{
			const float Gain = FMath::Pow(10.0f, (TargetDb - Db) / 20.0f);
			float Peak = 0.0f;
			for (float& S : Samples)
			{
				S *= Gain;
				Peak = FMath::Max(Peak, FMath::Abs(S));
			}
			if (Peak > 0.95f)
			{
				const float Down = 0.95f / Peak;
				for (float& S : Samples)
				{
					S *= Down;
				}
			}
		}
	}
	Ms = FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64() - C0);
	bDone = true;
}

// ------------------------------------------------------------------ the host

FRoomHost::~FRoomHost()
{
	RoomVoices::Clear();
}

namespace RoomFeed
{
bool IsStart(const ss::TEvent& E)
{
	return E.Type == ss::TEventType::TableBroken || E.Type == ss::TEventType::Moved || E.Type == ss::TEventType::FinalTable;
}

void Take(FRoomHost& H, ss::room::Director& D)
{
	std::vector<ss::room::Cue> Got = D.Take();
	if (H.bCatchingUp)
	{
		H.CatchUp.insert(H.CatchUp.end(), Got.begin(), Got.end());
		return;
	}
	for (ss::room::Cue& C : Got)
	{
		FRoomQueued Q;
		Q.DueAt = H.Clock + C.Delay;
		Q.Cue = MoveTemp(C);
		H.Incoming.push_back(MoveTemp(Q));
	}
}

namespace
{
void Hands(FRoomHost& H, FCardRoomSound* Sound, const std::vector<ss::TableRound>& Rounds)
{
	if (!Sound || H.bCatchingUp)
	{
		return;
	}
	// The engine's hands at the other tables, each played out over the next hand here (the player's own table is the
	// table actor's to sound: the room plays nothing for it).
	for (const ss::TableRound& R : Rounds)
	{
		const float Bb = R.BigBlind > 0 ? static_cast<float>(R.Pot) / static_cast<float>(R.BigBlind) : 0.0f;
		Sound->TableHand(R.TableId, static_cast<float>(H.HandSeconds), R.AllIn, R.Called, Bb);
	}
}
} // namespace

void Simulated(FRoomHost& H, ss::room::Director& D, FCardRoomSound* Sound, const ss::Tournament& T, const std::vector<ss::TEvent>& Events, double World)
{
	if (H.bCatchingUp)
	{
		H.CatchUp.clear(); // only the last tick's news is news
	}
	std::vector<ss::TEvent> Start, Finish;
	for (const ss::TEvent& E : Events)
	{
		(IsStart(E) ? Start : Finish).push_back(E);
	}
	// The start's Take and the finish's come at one moment, each one's Delays from it: whoever speaks in both (a backer at
	// the rail, Dee) says the finish's line after the start's, not over it. TickRoom places every cue that comes in, in
	// this order, one voice at a time (FRoomHost::VoicesDue), so the catch-up's last tick (EndCatchUp) is spaced the same.
	D.Events(T, Start, World);
	Take(H, D);
	D.Rounds(T, T.Rounds());
	D.Events(T, Finish, World);
	Take(H, D);
	Hands(H, Sound, T.Rounds());
}

void Table(FRoomHost& H, ss::room::Director& D, FCardRoomSound* Sound, const ss::Tournament& T, const std::vector<ss::TEvent>& Events, const ss::room::HandNote* Note,
	const std::vector<ss::TableRound>& Rounds, bool bNewHand, double World)
{
	const bool bFinish = Note != nullptr || !Rounds.empty();
	bool bFinishFed = false;
	bool bStartFed = false;
	auto Finish = [&](const std::vector<ss::TEvent>& Run) {
		if (Note)
		{
			D.Hand(T, *Note);
		}
		D.Rounds(T, Rounds);
		D.Events(T, Run, World);
		Take(H, D);
		Hands(H, Sound, Rounds);
		bFinishFed = true;
	};
	const bool bFinishEvents = std::any_of(Events.begin(), Events.end(), [](const ss::TEvent& E) { return !IsStart(E); });
	if (bFinish && !bFinishEvents)
	{
		Finish({}); // the hand ended with no news of its own (it came before any start that follows)
	}
	for (size_t I = 0; I < Events.size();)
	{
		const bool bStart = IsStart(Events[I]);
		std::vector<ss::TEvent> Run;
		while (I < Events.size() && IsStart(Events[I]) == bStart)
		{
			Run.push_back(Events[I++]);
		}
		if (bStart)
		{
			D.Events(T, Run, World);
			Take(H, D);
			bStartFed = true;
		}
		else if (!bFinishFed)
		{
			Finish(Run);
		}
		else
		{
			D.Events(T, Run, World);
			Take(H, D);
		}
	}
	if (bNewHand && !bStartFed)
	{
		// A hand dealt with no news of its own: the director still marks its start (the stacks it'll judge it by).
		D.Events(T, {}, World);
		Take(H, D);
	}
}

void BeginCatchUp(FRoomHost& H)
{
	H.bCatchingUp = true;
	H.CatchUp.clear();
}

void EndCatchUp(FRoomHost& H)
{
	H.bCatchingUp = false;
	for (ss::room::Cue& C : H.CatchUp)
	{
		FRoomQueued Q;
		Q.DueAt = H.Clock + C.Delay;
		Q.Cue = MoveTemp(C);
		H.Incoming.push_back(MoveTemp(Q));
	}
	H.CatchUp.clear();
}
} // namespace RoomFeed

// ------------------------------------------------------------------ the stream

namespace RoomStream
{
void Room(UNightOneAudio& Audio, const TSharedPtr<FCardRoomSound, ESPMode::ThreadSafe>& Sound)
{
	TWeakPtr<FCardRoomSound, ESPMode::ThreadSafe> Weak = Sound;
	// The room's level is its own (0 dB is the bed at about -30 dBFS, today's 0.4 send): unity here.
	Audio.SetAmbienceSourceStereo(
		[Weak](float* Out, int32 Frames) {
			if (TSharedPtr<FCardRoomSound, ESPMode::ThreadSafe> S = Weak.Pin())
			{
				S->Pull(Out, Frames);
			}
			else
			{
				FMemory::Memzero(Out, sizeof(float) * 2 * static_cast<size_t>(Frames));
			}
		},
		1.0f);
}

void TodaysBed(UNightOneAudio& Audio, float Crowd)
{
	TSharedPtr<FCardRoomAmbience> Bed = MakeShared<FCardRoomAmbience>(0xcafeu);
	Bed->SetCrowd(Crowd);
	Audio.SetAmbienceSource([Bed](float* Out, int32 N) { Bed->Render(Out, N); }, 0.4f);
}
} // namespace RoomStream

// ------------------------------------------------------------------ tonight's voices, for CastLooks

namespace RoomVoices
{
namespace
{
TMap<FString, float>& Socials()
{
	static TMap<FString, float> Map;
	return Map;
}
} // namespace

void Set(const std::vector<ss::life::LiveVoice>& Voices)
{
	check(IsInGameThread());
	TMap<FString, float>& M = Socials();
	M.Reset();
	for (const ss::life::LiveVoice& V : Voices)
	{
		M.Add(FString(UTF8_TO_TCHAR(V.Name.c_str())), V.Social);
	}
}

void Clear()
{
	Socials().Reset();
}

bool SocialOf(const FString& Name, float& Out)
{
	const float* S = Socials().Find(Name);
	if (!S)
	{
		return false;
	}
	Out = *S;
	return true;
}
} // namespace RoomVoices
