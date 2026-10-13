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
	/** A hand the hero was dealt, finished (HeroSeat: the hero's index in H.Seats). */
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
};
/** Steps T->SimulateTickWith(B) until the table's target or a stop, or until BudgetMs of real time has gone. */
SHORTSTACKCORE_API SkipOut SkipSlice(Tournament& T, const style::HeroBrain& B, const autoplay::TableAuto& A, double BudgetMs, SkipSink& Sink);
} // namespace live
} // namespace ss
