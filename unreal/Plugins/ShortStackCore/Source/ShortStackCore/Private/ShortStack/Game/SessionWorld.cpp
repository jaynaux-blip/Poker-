#include "ShortStack/Game/Session.h"

#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/World.h"
#include "WorldSim.h"

// The world's side of the session (update 2 §3, §6.2): how the career's world began, and the world's debug command
// (ss.World.Start). Phase A, day 1 (the lead). core-world owns this file from here.

namespace ss
{
world::Start Session::WorldStart() const
{
	return LivingWorld.Began();
}

std::string Session::DebugWorld(const std::string& Args)
{
	// ss.Site.Launch <site> (§6.6, testing; SessionSites.cpp forwards it as "sitelaunch <key>"): a rival opens in this world
	// now, its news with it, instead of on its own day.
	if (Args.rfind("sitelaunch", 0) == 0)
	{
		const std::string Key = Args.size() > 11 ? Args.substr(11) : std::string();
		net::SiteId Site = net::SiteId::Count;
		for (const net::SiteDef& D : net::Sites())
		{
			Site = D.Id != net::SiteId::RiverLine && D.Key == Key ? D.Id : Site;
		}
		if (Site == net::SiteId::Count || !LivingWorld.Ready())
		{
			return "ss.Site.Launch <ignite|runittwice>";
		}
		const double Was = LivingWorld.SiteLaunched(Site);
		if (!world::Sim::LaunchNow(LivingWorld, Site))
		{
			return net::Def(Site).Name + " is already open (since " + net::DateLabel(net::DayOf(Was)) + " " + std::to_string(world::YearOf(net::DayOf(Was))) + ").";
		}
		Save();
		return net::Def(Site).Name + " opens now. " + DebugWorld("start");
	}
	// ss.World.Start: the world's start mode, its version, when its books opened and Team RiverLine's size, then each rival
	// site's launch day, its regulars and its team.
	const bool Blank = LivingWorld.Began() == world::Start::Blank;
	int Team = 0;
	for (const world::Npc& N : LivingWorld.People())
	{
		Team += N.Pro ? 1 : 0;
	}
	std::string Out = std::string("World: ") + (Blank ? "Fresh start" : "Established") + ", version " + std::to_string(world::Version) + ", began " +
		net::DateLabel(net::DayOf(LivingWorld.StartedAt())) + " " + net::TimeLabel(LivingWorld.StartedAt()) + ".";
	Out += " Team RiverLine: " + std::to_string(Team) + (Team == 1 ? " pro" : " pros");
	if (Blank && Team == 0)
	{
		// A fresh world's first class signs on the 1st after its first month (update 2 §3.3 F3), or the next 1st when nobody
		// had results by then.
		const auto NextFirst = [](int From) {
			int Year = 0;
			int Month = 0;
			int Day = 0;
			world::CivilDate(From, Year, Month, Day);
			return Month == 12 ? world::DayOn(Year + 1, 1, 1) : world::DayOn(Year, Month + 1, 1);
		};
		const int Opened = NextFirst(net::DayOf(LivingWorld.StartedAt()));
		const int Today = net::DayOf(LivingWorld.Clock());
		Out += " (signs on results from " + net::DateLabel(Today < Opened ? Opened : NextFirst(Today)) + ")";
	}
	Out += ".";
	// The rival sites (update 2 §5.9, G3): when each opens in this world, how many play there, and its team.
	for (const net::SiteDef& D : net::Sites())
	{
		if (D.Id == net::SiteId::RiverLine)
		{
			continue;
		}
		const double Launch = LivingWorld.SiteLaunched(D.Id);
		if (Launch < 0.0)
		{
			Out += " " + D.Name + ": not in this world yet.";
			continue;
		}
		const int Day = net::DayOf(Launch);
		int Members = 0;
		int Pros = 0;
		for (const world::Npc& N : LivingWorld.People())
		{
			const bool There = N.Playing() && ((N.Sites >> static_cast<int>(D.Id)) & 1) != 0;
			Members += There ? 1 : 0;
			Pros += There && !D.TeamName.empty() && N.Sponsor == D.TeamName ? 1 : 0;
		}
		Out += " " + D.Name + (Launch <= LivingWorld.Clock() ? ": open since " : ": opens ") + net::DateLabel(Day) + " " + std::to_string(world::YearOf(Day)) + ", " +
			std::to_string(Members) + " regulars";
		if (!D.TeamName.empty() && (D.TeamSize > 0 || Pros > 0))
		{
			Out += ", " + D.TeamName + " " + std::to_string(Pros) + (Pros == 1 ? " pro" : " pros");
		}
		Out += ".";
	}
	return Out;
}
} // namespace ss
