// The creator update (SPEC_CREATORS §2.9, §5.3 K2, K10-K12, §10.2): the chat games' private state and the narrow view of the
// Room they play through, core-kastplay's header (private to the core: Kast.cpp and KastPlay.cpp include it, and so may
// tests/kastgrowth_play.cpp by its relative path, with a RoomView of its own). Hold or Crack, Nod, Welcome the raid, Set the
// scene, Rail Call's press and its named arrivals, First Follower's line. Kast::Stream holds a PlayState by pointer and
// implements RoomView over its own state (core-kastroom); its verbs call the play functions, and the Room calls the hooks
// below at the points they name. The two tracks change this seam together (it isn't one of the lead's frozen files).
#pragma once

#include "ShortStack/Game/Kast.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ss
{
namespace kast
{
/** The chat games' state for one stream: per stream, never saved, never money (chat's points stay the predictions'). Phase
 *  A-K: empty; core-kastplay fills it (Hold or Crack's calls, the Nods left, the scene's banked Engage...). */
struct PlayState
{
};

/** The Room as the chat games see it: its clock and keys, who is here, a line from one of them, Engage and Hype, a follow,
 *  and the few room effects the games set off. Kast.cpp implements it (core-kastroom); the games never touch the Room's state
 *  any other way. Phase A-K: declared, implemented by nothing yet. */
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
	/** A line in chat from someone present (refused, false, for anyone who isn't); Who empty is a system line. */
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
	/** Welcome the raid on that raid's people: their stays x StayBy and follows x FollowBy (x1.3 and x1.5). */
	virtual void WelcomeRaiders(double StayBy, double FollowBy) = 0;
	/** Set the scene's bank: the next N arrivals stay x StayBy (3 of them, x1.2). */
	virtual void BankArrivals(int N, double StayBy) = 0;
};

namespace play
{
/** The verbs' bodies (the Stream's verbs call them). Each refuses (false) when it isn't open; Phase A-K: every one refuses. */
bool Nod(PlayState& P, RoomView& Room, Channel& Ch, const std::string& Name);
bool RailCall(PlayState& P, RoomView& Room, Channel& Ch, const Inputs& In);
bool WelcomeRaid(PlayState& P, RoomView& Room, Channel& Ch);
bool SetTheScene(PlayState& P, RoomView& Room, Channel& Ch);
bool CallIt(PlayState& P, RoomView& Room, Channel& Ch, bool Holds);

// The Room's calls into the games (Kast.cpp, core-kastroom), at the points each names. Phase A-K: each does nothing.
/** Start: a fresh night (three Nods, nothing open). */
void Begin(PlayState& P, const Channel& Ch, const Inputs& In);
/** OnMoment, after Recount: Hold or Crack opens on an all-in on the front table (In.ThisTable == In.FrontTable, cards to come,
 *  2+ watching, an attended minute) and reveals at WonAllIn, LostAllIn and BadBeat. */
void OnMoment(PlayState& P, RoomView& Room, Channel& Ch, const Inputs& In, Moment M);
/** Each whole world minute AdvanceTo runs: a Nod's glow and the raid's welcome window end, Rail Call's chance opens. */
void OnMinute(PlayState& P, RoomView& Room, Channel& Ch, const Inputs& In, int Minute);
/** ChatStep's display time: chat calls HOLDS or CRACKS while Hold or Crack is open, from the people present, words from the
 *  stream's own R. Changes no channel state. */
void OnChat(PlayState& P, RoomView& Room, const Inputs& In, double DReal, Rng& R);
/** Someone the games sent for arrived (Rail Call's named arrivals say so). */
void OnArrive(PlayState& P, RoomView& Room, Channel& Ch, const std::string& Name, View::Source From);
/** A guest followed; FirstFollower on stream 1's first stranger who stayed (their line, with KastShine). */
void OnFollow(PlayState& P, RoomView& Room, Channel& Ch, const std::string& Name, bool FirstFollower);
/** The games' part of the View, each Tick: Hold or Crack's bar and the streamer's call, the Nods left and the lit names,
 *  Set the scene; the Room arbitrates the action slot (Hold or Crack first). */
void Fill(const PlayState& P, View& V);
/** Stop: the end card's best caller (Summary::BestCaller, BestCalls, BestCallsOf). */
void Close(const PlayState& P, Summary& S);
} // namespace play
} // namespace kast
} // namespace ss
