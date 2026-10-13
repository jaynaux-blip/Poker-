#include "ShortStack/Game/Session.h"

#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/World.h"

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
	// ss.World.Start: the world's start mode, its version, when its books opened and Team RiverLine's size (the rival
	// sites' launch days join at G3).
	(void)Args;
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
	return Out;
}
} // namespace ss
