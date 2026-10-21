// Update 3 (SPEC §2.4, §3.4-§3.6, §7.3): the revamped sites' presentation helpers, core-sites-look's file. Pure functions
// over the world's and the player's own numbers; no save. Phase A, day 1: stubs (nothing projected, accrued or drawn).
#include "ShortStack/Game/SiteShow.h"

#include <string>
#include <vector>

namespace ss
{
namespace site
{
namespace show
{
Mood IgniteMood(double /*World*/)
{
	return Mood::Daytime;
}

Chips ProjectedOverlay(const net::Network& /*N*/, const net::EventInstance& /*E*/, int /*RegisteredSoFar*/, double /*World*/, bool& Actual)
{
	Actual = false;
	return 0;
}

Accrual ClubAccrual(const site::Account& /*A*/)
{
	return Accrual();
}

bool SecondRun(const Hand& /*H*/, int /*HeroSeat*/, const std::string& /*HandId*/, SecondBoard& Out)
{
	Out = SecondBoard();
	return false;
}

DeskKpis Desk(const std::vector<HistoryEntry>& /*H*/, const site::Account& /*A*/, int /*Days*/, double /*World*/)
{
	return DeskKpis();
}

std::vector<LiveRow> SundayLive(const net::Network& /*N*/, const world::World* /*W*/, const net::EventInstance& /*E*/, double /*World*/)
{
	return {};
}

WelcomeView Welcome(const site::Account& /*A*/, double /*World*/)
{
	return WelcomeView();
}
} // namespace show
} // namespace site
} // namespace ss
