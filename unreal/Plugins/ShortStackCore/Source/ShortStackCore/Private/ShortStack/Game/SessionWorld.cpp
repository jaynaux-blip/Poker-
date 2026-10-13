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
	// ss.World.Start: the world's start mode, its version and when it began (core-world adds launch days and team sizes).
	(void)Args;
	const bool Blank = LivingWorld.Began() == world::Start::Blank;
	return std::string("World: ") + (Blank ? "Fresh start" : "Established") + ", version " + std::to_string(world::Version) + ", began " +
		net::DateLabel(net::DayOf(LivingWorld.StartedAt())) + " " + net::TimeLabel(LivingWorld.StartedAt()) + ".";
}
} // namespace ss
