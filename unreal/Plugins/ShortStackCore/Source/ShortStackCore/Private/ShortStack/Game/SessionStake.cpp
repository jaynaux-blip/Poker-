#include "ShortStack/Game/Session.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Coach.h"
#include "ShortStack/Game/Stake.h"

// The session's side of backing and coaching (next_segment_spec §2, §7.2): the Session.h block's backing calls, the
// calendar's StakeStep and the hook sites' seams (StakeEntered, StakeWorld, StakeConduct, StudyDone). core-stake.
//
// Phase A, days 2-3: stubs that keep today's behavior. Nobody is backing the player: every entry is their own money
// (FundingOptions offers only that, so GoToLive registers as it always has), nothing moves at the online buy-in or in
// the world, and the player's actions are refused in the game's voice.

namespace ss
{
std::string Session::AcceptStake(const std::string& /*OfferId*/)
{
	return "Nobody's offered to back you.";
}

std::string Session::CounterStake(const std::string& /*OfferId*/, int /*Lever*/)
{
	return "Nobody's offered to back you.";
}

std::string Session::ListAction(const std::string& /*EventId*/, int /*Permille*/, int /*MarkupPermille*/)
{
	return "Nobody's buying pieces yet.";
}

std::string Session::CancelListing(const std::string& /*EventId*/)
{
	return "You haven't sold any pieces of that one.";
}

std::string Session::ProposeSwap(const std::string& /*EntryId*/, const std::string& /*Partner*/, int /*Permille*/)
{
	return "Nobody's swapping yet.";
}

std::string Session::PayOwed(const std::string& /*OwedId*/)
{
	return "You don't owe anyone.";
}

std::string Session::RequestChop(const std::string& /*DealId*/)
{
	return "There's no deal to chop.";
}

std::string Session::WalkAway(const std::string& /*DealId*/)
{
	return "There's no deal to walk away from.";
}

std::string Session::BookCoach(const std::string& /*ActivityId*/)
{
	return "No coach is taking students yet.";
}

std::vector<std::pair<life::Funding, std::string>> Session::FundingOptions(const std::string& /*EventOrOccurrenceId*/) const
{
	// The player's own money, as always (life::Funding's defaults).
	return {{life::Funding(), std::string()}};
}

void Session::StakeStep(double /*From*/, double /*To*/)
{
}

std::string Session::DebugStake(const std::string& /*Args*/)
{
	return "ss.Stake: no stakes, pieces, swaps or packages yet.";
}

void Session::StakeEntered(const std::string& /*EventId*/, Chips /*BuyInCents*/, const life::Funding& /*F*/)
{
}

void Session::StakeWorld()
{
}

void Session::StakeConduct(sponsor::Breach /*Why*/)
{
}

void Session::StudyDone(const life::Activity& /*A*/, const life::Outcome& /*O*/)
{
}
} // namespace ss
