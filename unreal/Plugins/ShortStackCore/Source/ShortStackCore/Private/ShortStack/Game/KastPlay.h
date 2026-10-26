// The creator update (SPEC_CREATORS §2.9, §5.3 K2, K10-K12, §10.2): the chat games' private state and the narrow view of the
// Room they play through, core-kastplay's header (private to the core: Kast.cpp and KastPlay.cpp include it, and so may
// tests/kastgrowth_play.cpp by its relative path, with a RoomView of its own). Hold or Crack, Nod, Welcome the raid, Set the
// scene, Rail Call's press and its named arrivals, First Follower's line. Kast::Stream holds a PlayState by pointer and
// implements RoomView over its own state (core-kastroom); its verbs call the play functions, and the Room calls the hooks
// below at the points they name. The two tracks change this seam together (it isn't one of the lead's frozen files).
//
// What the games change, and on which clock (R3, R4): the room effects (a Nodded guest's follow, Rail Call's arrivals, the
// raid's welcome, the scene's bank, Hold or Crack's Hype at the reveal, Engage from a call) happen only in the verbs (the
// player's clicks) and in OnMoment / OnMinute (the world-minute grid), from the inputs and keyed draws
// (growth::Unit(Room.Seed(), Room.StreamNo(), ...)). OnChat is display time: chat's calls, the replies, the toasts, words
// from the stream's own R; it gets no Channel and changes no room state, so a frame rate or a pace never moves the room.
#pragma once

#include "ShortStack/Game/Kast.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ss
{
namespace kast
{
/** The chat games' state for one stream: per stream, never saved, never money (chat's points are chat's, as the
 *  predictions' are). Begin resets it; the games write it, the Room only reads it (play::Fill hands the UI its part). */
struct PlayState
{
	// ---- the stream and its clocks
	bool Live = false;          // Begin to Close
	double StartWorld = 0.0;    // In.World at Begin
	double World = 0.0;         // the newest world minute a hook saw (In.World, or the Room's grid minute)
	double RealT = 0.0;         // display time since Begin, real seconds (OnChat's DReal, summed)
	bool Attended = true;       // the last Inputs::Attended a hook saw: Fill draws the attended buttons only while it holds
	bool SceneOpen = false;     // Set the scene can be pressed now (OnChat: a true 0, streams 1-20, attended, unused)

	// ---- Nod (§2.9, §5.3 K11): three a stream, one a person
	int NodsLeft = 3;
	std::vector<std::string> Nodded; // tonight's, in order
	struct Glow
	{
		std::string Name;
		double Until = 0.0; // world minute: the name lights on the overlay for one
	};
	std::vector<Glow> Lit;

	// ---- Hold or Crack (§5.3 K12)
	struct Call
	{
		std::string Who;
		bool Holds = true;
		int Points = 0; // chat's points on the call (never money)
	};
	struct HoldCrack
	{
		bool Open = false;
		int Table = -1;         // the front table it opened on (its reveal must come from the same table)
		double Equity = -1.0;   // Inputs::AllInEquity at the open (-1 unknown)
		double OpenedAt = 0.0;  // world minute
		int Called = 0;         // the streamer's call: 0 none yet, 1 HOLDS, 2 CRACKS
		int Target = 0;         // how many of the named people present will call (OnChat brings them in)
		bool Queued = false;    // OnChat drew its callers (Queue, shuffled by the stream's R) at its first step after the open
		std::vector<std::string> Queue;
		double Debt = 0.0;      // OnChat's call clock
		std::vector<Call> Calls;
	};
	HoldCrack HoC;
	/** Tonight's tally of chat's callers: the end card's best caller ("kat_v called 4 of 5"). */
	struct Caller
	{
		std::string Name;
		int Right = 0;
		int Of = 0;
	};
	std::vector<Caller> Callers;

	// ---- Welcome the raid (§2.7.1, §5.3 K10)
	double RaidAt = -1.0;     // the newest raid the games saw (RoomView::RaidAt)
	double WelcomedAt = -1.0; // the raid already welcomed (one welcome a raid)

	// ---- Rail Call (§2.3.6, §5.3 K3)
	bool RailOpen = false;    // RAIL CALL is offered (opened at an attended minute; stays until pressed or the run ends)
	bool RailCalled = false;  // pressed tonight
	bool WholeRail = false;   // every follower is attending already: no button, "Your whole rail is here"
	int RailReach = 0;        // the followers the call reached...
	int RailArrived = 0;      // ...and the ones it brought who have turned up
	std::vector<std::string> RailSent; // the followers on their way (OnArrive counts and voices each once)

	// ---- Set the scene (§5.3 K2)
	bool SceneUsed = false;

	// ---- display time: the games' chat lines waiting for OnChat (replies, arrivals, the reveal's burst)
	struct Line
	{
		int Kind = 0;                       // kastplay_detail's line kinds (KastPlay.cpp)
		std::vector<std::string> Speakers;  // who may say it, in order: the first one present at Due says it
		double Due = 0.0;                   // RealT
		std::string Name;                   // a name the line carries, if any
	};
	std::vector<Line> Pending;

	// ---- the toast line (§5.1: one toast a real minute, none while Hold or Crack is open)
	double ToastAt = -1.0e9; // RealT of the last toast
	struct Toast
	{
		Alert A;
		Notice N;
	};
	std::vector<Toast> Toasts; // the games' toasts waiting for the line to free
};

/** The Room as the chat games see it: its clock and keys, who is here, a line from one of them, Engage and Hype, a follow,
 *  and the few room effects the games set off. Kast.cpp implements it (core-kastroom); the games never touch the Room's state
 *  any other way. */
class RoomView
{
public:
	virtual ~RoomView() = default;
	// ---- the clock and the keys: the games' keyed draws are growth::Unit(Seed(), StreamNo(), Tag, ...) (Nod's is Tag::Nod)
	/** The room's world minute: the last whole one AdvanceTo ran. */
	virtual double Minute() const = 0;
	virtual uint64_t Seed() const = 0;
	/** Channel::Streams after Start's increment. */
	virtual uint64_t StreamNo() const = 0;
	/** Inputs::Attended as the Room last had it (for the verbs that take no Inputs: Nod, Welcome the raid, Set the scene). */
	virtual bool Attended() const = 0;
	/** The Ping's turnout terms now (§2.3.1), which Rail Call's second roll reads (growth::RailCallChance): Sched is the
	 *  stream's schedule factor as Start decided it (1.3 on schedule, 0.75 no posted schedule, 0.6 posted but off it), ToD
	 *  clamp(TimeOfDay(World) / 1.25, 0.4, 1). The defaults are neutral only so the seam compiles before the Room overrides
	 *  them (core-kastroom: override both). */
	virtual double Sched() const { return 1.0; }
	virtual double ToD() const { return 1.0; }
	// ---- who is here
	/** Whole people watching now. */
	virtual int Watching() const = 0;
	/** Whether a named person is in the room now (R1: only they may speak). */
	virtual bool Present(const std::string& Name) const = 0;
	/** The named people in the room now (members, guests, named raiders): Hold or Crack's callers come from them. */
	virtual std::vector<std::string> PresentNames() const = 0;
	/** Whether a member is attending tonight, here or away on the slot process (Rail Call rolls every follower who isn't). */
	virtual bool Attending(const std::string& Name) const = 0;
	// ---- what the games do to the room
	/** A line in chat from someone present (refused, false, for anyone who isn't); Who empty is a system line (Kind System).
	 *  Kind Streamer is the streamer's own line: Who is ignored (the games pass ""), the Room writes it as the hero's, with the
	 *  streamer's badges and colour, as Thank and Answer do. */
	virtual bool Speak(const std::string& Who, const std::string& Text, LineKind Kind) = 0;
	/** An alert on the overlay and the toast line, and a notice for the session to play (its Sound through the cue queue). */
	virtual void PostAlert(const Alert& A) = 0;
	virtual void PostNotice(const Notice& N) = 0;
	virtual double Engage() const = 0;
	virtual void AddEngage(double By) = 0;
	/** Hype, 0..100 (Hold or Crack's reveal: +8). */
	virtual void AddHype(double By) = 0;
	/** A present guest follows: they become a member on the spot (one person, one count). */
	virtual bool Follow(const std::string& Name) = 0;
	/** A member the streamer Nodded at or answered: loyalty x1.6 for the rest of tonight (Member::Noticed and its one-time
	 *  Affinity bump are the Channel's, which the games set). */
	virtual void MarkNoticed(const std::string& Name) = 0;
	/** A member not attending turns up at world minute At (Rail Call's arrivals), with where they came from; when they do, the
	 *  Room calls play::OnArrive. */
	virtual void ArriveAt(const std::string& Name, double At, View::Source From) = 0;
	/** The newest raid in tonight: its world minute (-1: none). */
	virtual double RaidAt() const = 0;
	/** That raid's named raiders in the room now (the ones who thank a welcome). The default is nobody, so the seam compiles
	 *  before the Room overrides it (core-kastroom: override it; patches/core-kastplay_to_core-kastroom_1.diff). */
	virtual std::vector<std::string> RaidNamesHere() const { return {}; }
	/** Welcome the raid on that raid's people: their stays x StayBy and follows x FollowBy (x1.3 and x1.5). */
	virtual void WelcomeRaiders(double StayBy, double FollowBy) = 0;
	/** Set the scene's bank: the next N arrivals stay x StayBy (3 of them, x1.2). */
	virtual void BankArrivals(int N, double StayBy) = 0;
};

namespace play
{
/** The verbs' bodies (the Stream's verbs call them while it's live). Each refuses (false) when it isn't open:
 *  - Nod: a present person not Nodded tonight, three a stream, at every pace. Their name lights for a world minute and they
 *    answer; a member is Noticed (once ever: Affinity +3% of its gap) and gets loyalty x1.6 tonight (MarkNoticed); a guest
 *    follows when growth::Unit(Seed, StreamNo, Tag::Nod, NameKey(Name), the room's minute) < 0.10 x growth::Sat(Followers).
 *  - RailCall: while RAIL CALL is offered and the minute is attended, once a world day (growth::GrantRailCall); every
 *    follower not attending, Dee and Mei included, rolls growth::RailCallChance on Unit(..., RailCall, NameKey) and comes at
 *    In.World + ExpAt(6, Unit(..., RailCall, NameKey, 1)); "Your rail call reached N followers" (N the followers rolled).
 *  - WelcomeRaid: within 2 world minutes of the newest raid, attended, once a raid: WelcomeRaiders(1.3, 1.5).
 *  - SetTheScene: a true 0 on streams 1-20, attended, once a stream: BankArrivals(3, 1.2).
 *  - CallIt: while Hold or Crack is open, attended, once a reveal: Engage +0.3 (heads-down the buttons aren't drawn and a
 *    press is refused).
 *  §2.9's table: Nod draws at every pace; Welcome the raid, Rail Call, Set the scene and Hold or Crack only while attended. */
bool Nod(PlayState& P, RoomView& Room, Channel& Ch, const std::string& Name);
bool RailCall(PlayState& P, RoomView& Room, Channel& Ch, const Inputs& In);
bool WelcomeRaid(PlayState& P, RoomView& Room, Channel& Ch);
bool SetTheScene(PlayState& P, RoomView& Room, Channel& Ch);
bool CallIt(PlayState& P, RoomView& Room, Channel& Ch, bool Holds);

// The Room's calls into the games (Kast.cpp, core-kastroom), at the points each names.
/** Start: a fresh night (three Nods, nothing open). */
void Begin(PlayState& P, const Channel& Ch, const Inputs& In);
/** OnMoment, after Recount: Hold or Crack opens on an all-in on the front table (In.ThisTable == In.FrontTable, cards to come:
 *  In.AllInEquity unknown or strictly between 0 and 1, 2+ watching, an attended minute) and reveals at WonAllIn, LostAllIn and
 *  BadBeat from the same table (Hype +8, KastCall, chat's result; the reveal's Hype never reads chat's calls, nor the pace: one
 *  that opened attended reveals at any pace, the streamer's say in it is CallIt's). A moment whose inputs carry no armed run at
 *  depth 0.4 (a bust) withdraws RAIL CALL at once. */
void OnMoment(PlayState& P, RoomView& Room, Channel& Ch, const Inputs& In, Moment M);
/** Each whole world minute AdvanceTo runs (Minute is the grid's index, the stream minute: its world minute is the stream's
 *  StartWorld + Minute, Begin's In.World + Minute): a Nod's glow ends, Rail Call is offered (the first attended minute an
 *  armed run is at depth 0.4: growth::RailCallOpen, while today's grant is unused) or withdrawn (the run ended), and a Hold or
 *  Crack whose reveal never came closes after 5 world minutes. */
void OnMinute(PlayState& P, RoomView& Room, Channel& Ch, const Inputs& In, int Minute);
/** ChatStep's display time: chat calls HOLDS or CRACKS while Hold or Crack is open, from the people present, words from the
 *  stream's own R; the games' waiting lines and toasts go up. Changes no channel state and no room state. */
void OnChat(PlayState& P, RoomView& Room, const Inputs& In, double DReal, Rng& R);
/** Someone the games sent for arrived (Rail Call's named arrivals say so). */
void OnArrive(PlayState& P, RoomView& Room, Channel& Ch, const std::string& Name, View::Source From);
/** A guest followed; FirstFollower on stream 1's first stranger who stayed. For that one the games post the follower's alert
 *  ("your first follower who isn't family", through the toast line), its system line, the KastShine notice and Dee's cheer:
 *  the Room books the follow and skips its own follow alert for them. */
void OnFollow(PlayState& P, RoomView& Room, Channel& Ch, const std::string& Name, bool FirstFollower);
/** The games' part of the View, each Tick, after the Room's own fill: Hold or Crack's bar and the streamer's call, the Nods
 *  left and the lit names, Set the scene, Rail Call's reach and arrivals and "Your whole rail is here", and the action slot:
 *  Action is the first of Hold or Crack > Welcome the raid > Rail Call that's open, Waiting the others (bits 1 << Slot). All
 *  three, Hold or Crack's bar and "Your whole rail is here" show only while attended (PlayState::Attended, the last hook's):
 *  heads-down mid-runout the bar goes and chat's game runs on to its reveal. */
void Fill(const PlayState& P, View& V);
/** Stop: an open Hold or Crack closes without a reveal; the end card's best caller (Summary::BestCaller, BestCalls,
 *  BestCallsOf) and Rail Call's facts (RailCalled, RailReach, RailArrived). The games are over until the next Begin. */
void Close(PlayState& P, Summary& S);

/** Whether Name was Nodded tonight (View::MemberFlags' bit 4, the Room's per-member cache). */
bool WasNodded(const PlayState& P, const std::string& Name);
/** The toast line's arbiter (§5.1): whether a toast may go up now (a real minute since the last, Hold or Crack closed), and
 *  the note that one did. The games' own toasts go through it; the Room's (milestones, Front Row, the Ping) should too. */
bool ToastFree(const PlayState& P);
void ToastShown(PlayState& P);
} // namespace play
} // namespace kast
} // namespace ss
