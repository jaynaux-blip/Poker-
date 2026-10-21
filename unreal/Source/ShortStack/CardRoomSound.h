// The Embercrest's card room, heard (next_segment_spec 4, ue-room): the stereo room (ss::audio::CardRoom) rendered ahead
// on a worker and streamed through UNightOneAudio, the director's host state (FRoomHost: cues waiting for their time and
// their speech, what's being said, the rail), the VoiceBank seam, and the listening gate's recorder. ABackRoomGameMode's
// members that use these (RoomSound, RoomHost, VoiceBank) are declared in BackRoomGameMode.h; the code is in
// BackRoomLiveRoom.cpp (the host), BackRoomLive.cpp (the flow) and CardRoomSound.cpp (this header's).
#pragma once

#include "CoreMinimal.h"
#include "HAL/CriticalSection.h"
#include "Tasks/Task.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/WeakObjectPtr.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "ShortStack/Audio/CardRoom.h"
#include "ShortStack/Audio/Voice.h"
#include "ShortStack/Game/Room.h"
#include "ShortStack/UI/Captions.h"

#include <atomic>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

class ABackRoomPlayer;
class USoundWave;

/**
 * The card room's stereo sound (next_segment_spec 4.11): hosts ss::audio::CardRoom. The room renders on a worker
 * (UE::Tasks) about 100 ms ahead into a ring; UNightOneAudio's stream pulls from the ring on the game thread (Pull), so the
 * game thread only copies. Every setter is queued on the game thread and applied by whoever renders next, under one
 * lock: the room is never touched by two threads at once, and the setters land in the order they were made, before
 * the block that follows them. If the ring runs short (the very first fill, a stalled worker) Pull renders the rest
 * itself under the same lock, so the stream never underruns and never reorders.
 *
 * Latency: what the host hands the room is heard after the ring and the stream's queue (StreamSeconds on the host), so
 * a one-shot that should line up with the room (a near line and its duck) is held that long, and so is its caption.
 */
class FCardRoomSound : public TSharedFromThis<FCardRoomSound, ESPMode::ThreadSafe>
{
public:
	using Spot = ss::audio::CardRoom::Spot;
	using Emitter = ss::audio::CardRoom::Emitter;

	FCardRoomSound(uint32 Seed, int32 Talkers);
	~FCardRoomSound();

	// ---- setters (game thread): queued, applied before the next block the room renders
	void SetEmitters(const std::vector<Emitter>& Emitters);
	void SetListener(float X, float Y, float Yaw);
	/** room::MixFor's levels; CallsOffsetDb moves the dealers' calls with the Voices bus (against the stream's Ambience). */
	void SetMix(const ss::room::Mix& Mix, float CallsOffsetDb);
	/** The PA as heard, in dB (CardRoom::SetPaLevel): the Voices bus against the stream's Ambience; -120 silences it. */
	void SetPaLevel(float Db);
	void SetMuffle(float CutoffHz, float Db);
	void SetPace(float EventClock);
	void Duck(float Db, float Seconds);
	void TableHand(int32 Id, float Seconds, bool bAllIn, bool bCalled, float PotBb);
	void Say(const ss::audio::SpeechPlan& Plan, const ss::audio::Timbre& Voice, Spot Where, int32 Id, float Effort, bool bMurmur);
	void Crowd(ss::audio::Vocal V, int32 Voices, Spot Where, int32 Id, float Level);
	void SetVoiceBudget(int32 Talkers);
	int32 GetVoiceBudget() const { return Budget; }

	// ---- the stream
	/** UNightOneAudio's source (game thread): Frames stereo frames, interleaved. */
	void Pull(float* Out, int32 Frames);
	/** Each frame (game thread): the queued setters go to a worker, which tops the ring up. */
	void Pump();
	/** Seconds rendered ahead in the ring (not yet handed to the stream). */
	double RingSeconds() const;

	// ---- what ss.Live.Room reports
	ss::audio::CardRoom::Meter Measure() const;
	struct FStats
	{
		double BlockMsAvg = 0.0;  // a 25 ms block's render, on the worker
		double BlockMsPeak = 0.0;
		double WorkerMsPerSecond = 0.0; // render time per second of audio
		int32 Blocks = 0;
		int32 SyncRenders = 0; // blocks the game thread had to render itself (the ring ran short)
		double SyncMsPeak = 0.0;
	};
	FStats Stats() const;

	// ---- the listening gate (next_segment_spec 4.10): the stream as heard, with the voices played over it
	/** Records the next Seconds of the stream into Dir/pie_<Name>.wav, and today's bed (CardRoomAmbience.h at its 0.4
	 *  send, the room as full as Crowd) beside it as pie_<Name>_bed.wav. False when one is under way. */
	bool StartRecording(const FString& Name, const FString& Dir, double Seconds, float Crowd);
	bool IsRecording() const { return Recording.IsValid(); }
	/** A one-shot (a near line, the chime) as the recording hears it: mono, panned, from now (QueuedFrames: the stream
	 *  frames pulled but not yet heard). */
	void RecordOverlay(const std::vector<float>& Mono, float GainL, float GainR, int32 QueuedFrames);

private:
	struct FRecording;
	void Kick();
	/** Under RoomLock: applies the queued setters handed over, then renders until the ring holds Target frames. */
	void RenderAhead(TArray<TFunction<void(ss::audio::CardRoom&)>>& Batch);
	void Queue(TFunction<void(ss::audio::CardRoom&)> Fn);
	void FinishRecording();

	TUniquePtr<ss::audio::CardRoom> Room;
	mutable FCriticalSection RoomLock;
	TArray<TFunction<void(ss::audio::CardRoom&)>> Pending; // game thread only
	UE::Tasks::FTask InFlight;
	// The ring (interleaved stereo), its own lock.
	mutable FCriticalSection RingLock;
	TArray<float> Ring;
	int32 RingRead = 0;
	int32 RingCount = 0; // frames
	int32 Budget = 0;
	// Stats (the worker writes under RingLock).
	double BlockMsSum = 0.0;
	double BlockMsPeak = 0.0;
	int32 Blocks = 0;
	int32 SyncRenders = 0;
	double SyncMsPeak = 0.0;
	TSharedPtr<FRecording, ESPMode::ThreadSafe> Recording;
};

/**
 * The VoiceBank seam (next_segment_spec 4.10): a recorded clip by a cue's Key (with its Part, for the PA's sentences) and
 * speaker, played instead of the synthesized murmur when there is one. Option B (a baked phrasebook) or D (the lead reads
 * the floor's copy) drop in here later without touching core. Empty this segment: Find always says no.
 */
class FVoiceBank
{
public:
	/** The clip for Key/Part said by Speaker ("Floor", "Dealer", a name), or null: the synthesized voice plays. */
	USoundWave* Find(const std::string& Key, int32 Part, const std::string& Speaker) const;
	/** A clip for every speaker (Speaker empty) or one; P2's import fills these. */
	void Add(const FString& Key, int32 Part, const FString& Speaker, const FSoftObjectPath& Clip);
	int32 Num() const { return Clips.Num(); }

private:
	static FString KeyOf(const FString& Key, int32 Part, const FString& Speaker);
	TMap<FString, FSoftObjectPath> Clips;
};

/** A line's speech, made off the game thread (next_segment_spec 4.11): its plan always, and its samples for a one-shot. */
struct FRoomSpeech
{
	// In.
	std::string Text;
	ss::audio::Timbre Voice;
	float Effort = 0.6f;
	uint32 Seed = 1;
	bool bRender = false; // a one-shot at the speaker's head: the samples too
	bool bMurmur = false; // the room's murmur (people), not a clear call (dealers)
	/** The fallback (talkers 0, what ships: spec 11): no formant voice, only a whispered breath under the caption; in the
	 *  room's stream it goes as a murmur, which the room breathes at budget 0. */
	bool bBreath = false;
	/** The jaw follows the plan (MouthAt): only with the dev flag's formant voices, never on the fallback's breath. */
	bool bLipSync = false;
	float TargetDb = -25.0f; // the one-shot's speaking level, dBFS RMS
	// Out (once bDone).
	ss::audio::SpeechPlan Plan;
	std::vector<float> Samples;
	double Ms = 0.0;
	std::atomic<bool> bDone{false};
	void Run();
};

/** A cue on its way: due on the host's cue clock, its speech being made. */
struct FRoomQueued
{
	ss::room::Cue Cue;
	double DueAt = 0.0;
	TSharedPtr<FRoomSpeech, ESPMode::ThreadSafe> Speech;
	TWeakObjectPtr<ABackRoomPlayer> Actor;
	/** A line from the host itself (a table's line routed through ABackRoomPlayer::Say), not the director's. */
	bool bHost = false;
	/** Said at the player's table by someone there (the dealer, a seat): a one-shot at their head, one line at a time. */
	bool bNear = false;
	/** Who says it, as one voice (ss::room::VoiceKey): their lines one at a time. Empty: nobody's own voice. */
	std::string Voice;
};

/** Something to do at a real time (FPlatformTime::Seconds): a one-shot held by the stream's latency, a caption with its sound. */
struct FRoomTimed
{
	double At = 0.0;
	TFunction<void()> Do;
};

/** ue-room's own state on ABackRoomGameMode (BackRoomGameMode.h's FRoomHost): made in BuildDirector. */
struct FRoomHost
{
	~FRoomHost();
	/** The cue clock: real seconds at the room's pace (Focus's dilation, a fast-forward), clamped to 0.45..1.5 (spec 4.9). */
	double Clock = 0.0;
	/** Cues taken from the director (or played by the host), not yet made into speech: TickRoom resolves the speaker and
	 *  starts the job, then they wait in Queue for their time. */
	std::vector<FRoomQueued> Incoming;
	std::vector<FRoomQueued> Queue;
	/** Tonight's field's ages by name (the entry's voices), for their timbre. */
	TMap<FString, int32> Ages;
	TArray<FRoomTimed> Timeline;
	/** A fast-forward or a wait being caught up: only the last tick's cues are kept (the room doesn't replay the hours). */
	bool bCatchingUp = false;
	std::vector<ss::room::Cue> CatchUp;
	/** The table's news this frame (LiveTick's TakeEvents), for the director. */
	std::vector<ss::TEvent> TableEvents;
	int32 LastHandNumber = -1;
	/** The player's table at the end of the last frame (Bug 2: a move names the table the player was moved from). */
	int32 HeroTable = -1;
	/** The engine's own word for it: the hero's last Moved event's From and the engine tick it came in (LiveMove trusts it
	 *  only in that tick, whichever came first in the frame, the event or the table's note). */
	int32 MovedFrom = -1;
	int32 MovedTick = -1;
	/** Real time the PA, and the player's table, are speaking until (lines wait their turn; small talk that would wait
	 *  long is left to the night log). */
	double PaFreeAt = 0.0;
	double TableFreeAt = 0.0;
	/**
	 * Each voice, one line at a time (ss::room::Turns), across Takes: a backer at the rail, a dealer, a seat never starts a
	 * line before their last one is done, at any pace. VoicesDue places every cue as it comes in, on the cue clock, after
	 * its speaker's line before it by the lines' own lengths (a hand's start and its end, a lean, RoomFeed::Simulated's
	 * start and finish taken at one moment). VoicesSpeaking is real time, each voice speaking until its line as heard is
	 * done (its speech or its clip): a line waits for it like the PA's and the table's.
	 */
	ss::room::Turns VoicesDue;
	ss::room::Turns VoicesSpeaking;
	/** Near speech one-shots playing (their end times): at most 2 engine voices (spec 10). */
	TArray<double> NearEnds;
	/** A table's hand in real seconds (the engine's hands elsewhere play over the next one), eased. */
	double HandSeconds = 45.0;
	double LastHandEndAt = -1.0;
	// The rail's backers (PlaceBackers): who, where, when it was last looked at.
	std::vector<std::pair<std::string, int>> RailSet;
	int32 RailAnchor = -2;
	int32 RailRemaining = -1; // the field's count at the last look (a bust is what changes who's out)
	double NextRailLook = 0.0;
	FVector RailAt = FVector::ZeroVector; // room space, where the backers are (the rail's line comes from there)
	bool bRailPlaced = false;
	bool bRailStanding = false; // on their feet at the bar line or the stage's edge (else in an empty table's chairs)
	/** How many of them stand in the bar's line of railbirds (their places, the line's last ones, are theirs: PlaceExtras). */
	int32 RailAtBar = 0;
	// The dealer's "Turn them up" (an all-in and a call at the player's table): the rail leans in mid-hand, once a hand.
	std::set<std::string> TurnUpLines;
	int32 LeanHand = -1;
	// The mix and the room's muffles, as last set.
	float TalkScale = 1.0f;
	int32 Talkers = -1;
	float FocusMuffle = 0.0f;
	bool bBedAB = false; // ss.Live.Bed 1: today's mono bed in the room's place (the listening gate's A/B)
	/** The room's stream is on the table's audio (from the walk in: the doorway behind the player as they come in). */
	bool bStreaming = false;
	// What the room was last told (only changes are queued), and this frame's list, built in place.
	std::vector<ss::audio::CardRoom::Emitter> LastEmitters;
	std::vector<ss::audio::CardRoom::Emitter> EmitterScratch;
	float LastMix[8] = {-999.0f, -999.0f, -999.0f, -999.0f, -999.0f, -999.0f, -999.0f, -999.0f};
	float LastMuffle[2] = {-1.0f, -1.0f};
	float LastPace = -1.0f;
	float LastListener[3] = {1.0e9f, 1.0e9f, 0.0f};
	// What ss.Live.Room reports of the host.
	double HostMsSum = 0.0;
	double HostMsPeak = 0.0;
	int32 HostFrames = 0;
	double SpeechMsSum = 0.0;
	double SpeechMsPeak = 0.0;
	int32 SpeechJobs = 0;
	int32 Skipped = 0; // lines left to the night log (small talk that would have talked over a line; a line whose speaker is busy past its moment)
	double LastLatency = 0.3;
};

/** The director fed, in its order (Room.h): free functions over the host's state, for every place the host plays a tick. */
namespace RoomFeed
{
/** StartTick's news (the balance: tables breaking, moves, the final table); FinishTick's is the rest. */
bool IsStart(const ss::TEvent& E);
/** Cues out of the director into the host's queue (DueAt on its cue clock), or, catching up, kept for the last tick only. */
void Take(FRoomHost& H, ss::room::Director& D);
/**
 * A whole tick the host played itself (Tournament::SimulateTick: the rail, a fast-forward, a blind-off, a dead hand): the
 * start's events, the rounds, the finish's events. The engine's hands at the other tables go to the room (Sound).
 */
void Simulated(FRoomHost& H, ss::room::Director& D, FCardRoomSound* Sound, const ss::Tournament& T, const std::vector<ss::TEvent>& Events, double World);
/**
 * The player's table this frame: its events in the order they came (a start's and a finish's runs), the hand it finished
 * (Note), the rounds that finish played, and whether a new hand was dealt with no news of its own.
 */
void Table(FRoomHost& H, ss::room::Director& D, FCardRoomSound* Sound, const ss::Tournament& T, const std::vector<ss::TEvent>& Events, const ss::room::HandNote* Note,
	const std::vector<ss::TableRound>& Rounds, bool bNewHand, double World);
void BeginCatchUp(FRoomHost& H);
/** The last tick's cues go out as news (the PA doesn't replay what passed). */
void EndCatchUp(FRoomHost& H);
} // namespace RoomFeed

class UNightOneAudio;
/** What UNightOneAudio streams in the card room. */
namespace RoomStream
{
/** The stereo room in place of the old mono bed: the stream pulls from Sound's ring (silence once it's gone). */
void Room(UNightOneAudio& Audio, const TSharedPtr<FCardRoomSound, ESPMode::ThreadSafe>& Sound);
/** Today's mono bed (CardRoomAmbience.h at its 0.4 send, the room as full as Crowd) in the room's place: the listening
 *  gate's A/B in PIE (ss.Live.Bed 1). */
void TodaysBed(UNightOneAudio& Audio, float Crowd);
} // namespace RoomStream

/**
 * Who in tonight's field sounds how (their LiveVoice), for the people the save knows nothing about: CastLooks gives a
 * stranger's persona the chatter their voice has (next_segment_spec 4.11: strangers' Chatter from LiveVoice.Social).
 * Set while the card room is open, cleared when it closes.
 */
namespace RoomVoices
{
void Set(const std::vector<ss::life::LiveVoice>& Voices);
void Clear();
/** The voice's Social for Name, if tonight's field has one. */
bool SocialOf(const FString& Name, float& Out);
} // namespace RoomVoices
