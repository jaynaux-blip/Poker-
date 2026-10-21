// Update 3 (SPEC §3.6, §7.3): RunItTwice's loops, core-siteplay's file. The League with LeagueScore (the rule the world
// scores its members by), Callouts, H2H, Team Scalps, the Watchlist and the desk bell's keys, the Luck Ledger, credentials
// and the Briefing. Phase A, day 1: stubs (nothing scores, every claim refused).
#include "ShortStack/Game/SitePlay.h"

#include <string>

namespace ss
{
namespace site
{
namespace play
{
double LeagueScore(int /*Div*/, const std::string& /*TemplateId*/, Chips /*BuyIn*/, int /*WorldDay*/, double /*RawPoints*/, bool /*Series*/, bool /*Deadline*/)
{
	return 0.0;
}

std::string Star(Book& /*B*/, const std::string& /*TemplateId*/, bool /*On*/)
{
	return "Not yet.";
}

std::string CallOut(Book& /*B*/, int /*Npc*/, double /*World*/)
{
	return "Not yet.";
}
} // namespace play
} // namespace site
} // namespace ss
