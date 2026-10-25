// The creator update (SPEC_CREATORS §10.2): Kast's Session glue, core-kastglue's file. Every body behind the lead's hooks in
// Session.cpp: KastInputsExtra (the deepest run over all tables with its event's facts, the front table, prestige, series,
// buy-in, the cached all-in equity, Attended, Rerun's Heat bumps, the live night, the seed, the rents paid, the deals' reads),
// the new studio verbs, KastBeforeStart (the migration's seeding), KastAfterStart (U3Kast, the one-time letter), KastNewCareer
// (U3Kast and Heat as of now for a new career), KastAfterStop (sponsor pay as LedgerSponsors lines, the pending release, the
// milestone stings' notices), KastDaily, the forecast and the rank Studio shows, and DebugKast's body for ss.Kast.* (§10.5).
// Phase A-K: stubs that change nothing (the verbs refuse, the readouts are empty, and ss.Kast.* describes the channel as it
// stands and says the rest isn't built yet).
#include "ShortStack/Game/Session.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/KastGrowth.h"

#include <cctype>
#include <cmath>
#include <string>
#include <vector>

namespace ss
{
namespace session_kast_detail
{
/** ss.Kast.*'s words: the verb as Update3Commands passes it (in lower case) and its arguments as typed. */
std::vector<std::string> KastWords(const std::string& Args)
{
	std::vector<std::string> Out;
	std::string Word;
	for (const char C : Args)
	{
		if (std::isspace(static_cast<unsigned char>(C)))
		{
			if (!Word.empty())
			{
				Out.push_back(Word);
				Word.clear();
			}
			continue;
		}
		Word.push_back(C);
	}
	if (!Word.empty())
	{
		Out.push_back(Word);
	}
	return Out;
}
} // namespace session_kast_detail

bool Session::StreamNod(const std::string& /*Name*/)
{
	return false;
}

bool Session::StreamRailCall()
{
	return false;
}

bool Session::StreamWelcomeRaid()
{
	return false;
}

bool Session::StreamSetScene()
{
	return false;
}

bool Session::StreamCallIt(bool /*Holds*/)
{
	return false;
}

bool Session::StreamClipIt()
{
	return false;
}

kast::growth::Band Session::KastForecast() const
{
	return kast::growth::Band();
}

kast::growth::Standing Session::KastRank() const
{
	return kast::growth::Standing();
}

std::string Session::DebugKast(const std::string& Args)
{
	const std::vector<std::string> W = session_kast_detail::KastWords(Args);
	const std::string Verb = W.empty() ? std::string("describe") : W[0];
	const double World = WorldMinutes();
	const kast::Channel& Ch = Channel;
	if (Verb == "describe")
	{
		// The channel as today's model keeps it. Heat and its why lines, the forecast, the lurker pool, Stamina, the LiveMin
		// ring, the tokens and PendingCents join with core-kastglue's body, and the room by source with core-kastroom's.
		int Casual = 0;
		int Regular = 0;
		int Superfan = 0;
		for (const kast::Member& M : Ch.Members)
		{
			Superfan += M.Loyalty >= kast::SuperfanLoyalty ? 1 : 0;
			Regular += M.Loyalty >= kast::RegularLoyalty && M.Loyalty < kast::SuperfanLoyalty ? 1 : 0;
			Casual += M.Loyalty < kast::RegularLoyalty ? 1 : 0;
		}
		std::string Out = "Kast: " + Grouped(Ch.Followers) + " followers, " + Grouped(static_cast<int64_t>(Ch.Members.size())) + " in the community (" + Grouped(Casual) + " casual, " +
			Grouped(Regular) + " regulars, " + Grouped(Superfan) + " superfans).";
		Out += "\nStreams " + std::to_string(Ch.Streams) + ", " + Fixed(Ch.MinutesLive / 60.0, 1) + " hours live; peak " + std::to_string(Ch.Peak) + ", average " +
			Fixed(Ch.AvgViewers(), 1) + " over the last ten.";
		Out += "\nLevel " + std::to_string(Ch.Level()) + ", stage " + std::to_string(Ch.Stage) + ", streak " + std::to_string(Ch.Streak) + "; " +
			(Ch.Partner ? std::string("Partner") : Ch.Affiliate ? std::string("Affiliate") : std::string("not Affiliate yet")) + ".";
		Out += "\nSubs " + std::to_string(Ch.ActiveSubs(World)) + " active, mods " + std::to_string(Ch.Mods.size()) + ", clips " + std::to_string(Ch.Clips.size()) + "; balance " +
			Money(Ch.UnpaidCents) + ", paid out " + Money(Ch.PaidCents) + ".";
		if (Stream.Live)
		{
			Out += "\nLive " + Fixed(Stream.Uptime(World), 0) + " world minutes: " + std::to_string(std::lround(Stream.Viewers)) + " watching, peak " + std::to_string(Stream.Peak) + ".";
		}
		Out += "\nHeat, the forecast, the lurker pool, Stamina and pending money: not built yet.";
		return Out;
	}
	static const char* const Stubs[][2] = {{"grant", "Grant"}, {"heat", "Heat"}, {"moment", "Moment"}, {"raid", "Raid"}, {"room", "Room"}, {"advance", "Advance"}, {"rent", "Rent"},
		{"forecast", "Forecast"}};
	for (const auto& Stub : Stubs)
	{
		if (Verb == Stub[0])
		{
			return std::string("ss.Kast.") + Stub[1] + ": not built yet (it comes with the creator update's Kast).";
		}
	}
	return "ss.Kast.<Describe | Grant <followers> [regulars] [subs] | Heat <h> | Moment <bubble|itm|ft|top3|win|allin> [prestige] [entrants] | Raid <size> [broadcast] | Room | "
		   "Advance <days> | Rent | Forecast>";
}

void Session::KastInputsExtra(kast::Inputs& /*In*/) const
{
}

void Session::KastBeforeStart()
{
}

void Session::KastAfterStart()
{
}

void Session::KastNewCareer()
{
}

void Session::KastAfterStop()
{
}

void Session::KastDaily(double /*From*/, double /*To*/)
{
}
} // namespace ss
