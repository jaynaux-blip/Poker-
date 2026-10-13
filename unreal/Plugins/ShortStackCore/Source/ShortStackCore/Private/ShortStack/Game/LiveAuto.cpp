#include "ShortStack/Game/LiveAuto.h"

#include <algorithm>
#include <chrono>

// The live skip (update 2 §2.12): the plan plays the night on headless, a budgeted slice a frame, between hands only.
// FAutoHost::TickSkip calls it with its sink (the room's catch-up feed, HandsPlayed, LiveMet, the same core calls
// OnHandEnded makes); card_room tests it (LiveSkipSliceEqualsTicks). core-autoplay's file.

namespace ss
{
namespace live
{
namespace live_auto_detail
{
bool LiveAutoHas(const std::vector<TEvent>& Events, TEventType Type)
{
	return std::any_of(Events.begin(), Events.end(), [Type](const TEvent& E) { return E.Type == Type; });
}

int LiveAutoHeroSeat(const Hand& H)
{
	for (size_t I = 0; I < H.Seats.size(); ++I)
	{
		if (H.Seats[I].Id == HeroId)
		{
			return static_cast<int>(I);
		}
	}
	return -1;
}
} // namespace live_auto_detail

SkipOut SkipSlice(Tournament& T, const style::HeroBrain& B, const autoplay::TableAuto& A, double BudgetMs, SkipSink& Sink)
{
	using live_auto_detail::LiveAutoHas;
	SkipOut Out;
	if (A.M != autoplay::Mode::Skip)
	{
		Out.Why = SkipWhy::Stopped;
		return Out;
	}
	if (T.FinishPending())
	{
		return Out; // a round is still being finished over frames: nothing may touch the tournament (the next frame)
	}
	if (T.Hero().Busted)
	{
		Out.Why = SkipWhy::Busted;
		return Out;
	}
	const bool BehindAlready = T.bFinished || (A.To == autoplay::Target::Bubble && T.InTheMoney()) || (A.To == autoplay::Target::FinalTable && T.Tables.size() <= 1) ||
		(A.To == autoplay::Target::Hands && A.Hands <= 0);
	if (BehindAlready)
	{
		Out.Why = SkipWhy::Target;
		return Out;
	}
	const auto Start = std::chrono::steady_clock::now();
	for (;;)
	{
		const int Zone = T.PaidPlaces() + 15;
		const bool OutsideZone = T.Remaining > Zone;
		const Tournament::TickResult Tick = T.SimulateTickWith(B);
		++Out.Ticks;
		if (const Hand* Last = T.LastHeroHand())
		{
			++Out.Hands;
			Sink.HandDone(*Last, live_auto_detail::LiveAutoHeroSeat(*Last));
		}
		Sink.Events(Tick.Events);
		// Where it ends (between hands, after this tick), first match wins.
		const bool IntoZone = OutsideZone && T.Remaining <= Zone; // (a big tick may jump it into the money: still the bubble)
		const bool HandForHand = LiveAutoHas(Tick.Events, TEventType::HandForHand);
		bool Ends = true;
		if (T.Hero().Busted)
		{
			Out.Why = SkipWhy::Busted;
		}
		else if (T.bFinished)
		{
			Out.Why = SkipWhy::Target; // the night is won: the result is in
		}
		else if (A.To == autoplay::Target::NextBreak && LiveAutoHas(Tick.Events, T.Spec.BreakEvery > 0 ? TEventType::Break : TEventType::Level))
		{
			Out.Why = SkipWhy::Break;
		}
		else if (A.To == autoplay::Target::Bubble && (IntoZone || HandForHand || LiveAutoHas(Tick.Events, TEventType::Bubble)))
		{
			Out.Why = IntoZone ? SkipWhy::Bubble : HandForHand ? SkipWhy::HandForHand : SkipWhy::Target;
		}
		else if (A.To == autoplay::Target::Hands && Out.Hands >= A.Hands)
		{
			Out.Why = SkipWhy::Target;
		}
		else if (LiveAutoHas(Tick.Events, TEventType::FinalTable))
		{
			Out.Why = SkipWhy::FinalTable; // always, live: the stage move then runs visibly
		}
		else if ((A.Stops & autoplay::StopBubble) && (IntoZone || HandForHand))
		{
			Out.Why = IntoZone ? SkipWhy::Bubble : SkipWhy::HandForHand;
		}
		else
		{
			Ends = false;
		}
		if (Ends || T.Tick % 5 == 0)
		{
			Sink.Checkpoint(T.Tick);
		}
		if (Ends)
		{
			return Out;
		}
		const double Spent = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - Start).count();
		if (Spent >= BudgetMs)
		{
			Out.Why = SkipWhy::Budget;
			return Out;
		}
	}
}
} // namespace live
} // namespace ss
