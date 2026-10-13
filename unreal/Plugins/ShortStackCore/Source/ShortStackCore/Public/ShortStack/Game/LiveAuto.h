#pragma once

#include "ShortStack/AI/Style.h"
#include "ShortStack/Game/Autoplay.h"
#include "ShortStack/Tournament.h"

#include <vector>

namespace ss
{
/**
 * The live skip (update 2 §2.12), in core so card_room can test it: the card room's FAutoHost calls SkipSlice once a
 * frame with its budget, and a sink that feeds the room (RoomFeed::Simulated, HandsPlayed, LiveMet) and makes the same
 * core calls OnHandEnded makes, from Tournament::LastHeroHand(). A slice stops only between hands; the budget never
 * changes what happens, only where a slice ends (LiveSkipSliceEqualsTicks). Bodies: core-autoplay (Game/LiveAuto.cpp).
 */
namespace live
{
struct SkipSink
{
	virtual ~SkipSink() = default;
	/** A hand the hero was dealt, finished (HeroSeat: the hero's index in H.Seats). The host's run takes it through
	 *  autoplay::NoteSkippedHand (the plan's tilt, its hand and decisions, the observed stats) before the next checkpoint. */
	virtual void HandDone(const Hand& H, int HeroSeat) = 0;
	/** A tick's tournament events (busts, levels, breaks, moves). */
	virtual void Events(const std::vector<TEvent>& E) = 0;
	/** Time for a checkpoint (every 5 ticks and at the end, R5). */
	virtual void Checkpoint(int Tick) = 0;
};
enum class SkipWhy : int
{
	Budget,
	Target,
	Bubble,
	HandForHand,
	FinalTable,
	Break,
	Busted,
	Stopped,
};
struct SkipOut
{
	int Ticks = 0;
	SkipWhy Why = SkipWhy::Budget;
	// Appended by core-autoplay: hands the player was dealt in the slice (a Hands target counts these; the host passes
	// what's left of it as A.Hands on the next slice).
	int Hands = 0;
};
/**
 * Steps T.SimulateTickWith(B) until the table's target or a stop, or until BudgetMs of real time has gone (at least one
 * tick a call: a budget of 0 steps tick by tick). Stops only between hands: no decision-level stop rule runs here
 * (§2.6.4), the final table always stops the skip after the tick that seats it (live: the stage move runs visibly), and the
 * bubble rule (A.Stops) stops it when the field first comes within Paid + 15 and at hand for hand. Targets: NextBreak at
 * the next break (an event with none: the next level), Bubble at the first of those or the bubble bursting, FinalTable,
 * Result (the player out or the event won), Hands after A.Hands dealt hands; Closing is Dee's and plays as Result here.
 * A target already behind the night (Bubble in the money, FinalTable at it, Hands at 0) ends the call at once with Target.
 * Per tick, in order: Sink.HandDone with LastHeroHand() when the player was dealt in, Sink.Events with the tick's events,
 * then Sink.Checkpoint when T.Tick is a multiple of 5 and again when the skip ends (any Why but Budget). B is read
 * afresh each tick, so a sink that updates the object B refers to after a hand (the plan's tilt: autoplay::NoteSkippedHand, then
 * the host's brain) is seen from the next hand on. Nothing here reads a clock but the budget's, so slices of any budget
 * play the same night (LiveSkipSliceEqualsTicks). A.M other than Skip: nothing is played (Stopped).
 */
SHORTSTACKCORE_API SkipOut SkipSlice(Tournament& T, const style::HeroBrain& B, const autoplay::TableAuto& A, double BudgetMs, SkipSink& Sink);
} // namespace live
} // namespace ss
