// Update 4, at the table (SPEC v1.7 §3.17): the seating rule, seat fidelity, the one pill, the hero's notes (World::NoteObserved) and
// the ForYou greeting. W-table's file from G2 (S-crowd's crowd seats, the crowd's pills and ss.Crowd.* at G4, §5.5).
//
// Phase A (§6 PA-19, PA-24, PA-38, PA-50; the lead): HeroFinished hands TableReport::Seen to NoteObserved, which keeps nothing yet
// (the bonds' note fields stay 0, written only when non-zero). The session's hooks into this file keep today's behaviour:
// SeatStoryRule is NameField's fresh-faces block verbatim, the fidelity overwrite and the crowd's names and meetings do nothing,
// every pill is none and every note "", and ss.Crowd.* answers that it isn't built yet.
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/World.h"

#include <set>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace world
{
void World::NoteObserved(const std::vector<Observed>& /*Seen*/)
{
	// Phase A: nothing folded (G2: the First and Recent blocks of each bond, §3.17 R5; drains the hold).
}
} // namespace world

void Session::SeatStoryRule(std::vector<int>& Who, int Today)
{
	// Phase A: today's fresh-faces block, verbatim (G/Session.cpp:5952-5965 at 8db6d21): a name or two the player has never seen
	// before, at their own table, swapped to the front of Who (no draw). G2 (W-table, §3.17 R2): the seating rule from day 2.
	int Fresh = 0;
	for (size_t K = 0; K < Who.size() && Fresh < 2; ++K)
	{
		const world::Npc* N = LivingWorld.Get(Who[K]);
		if (N && N->Came != world::Arrival::None && Today - N->Arrived <= 60 && !LivingWorld.BondWith(N->Id))
		{
			std::swap(Who[static_cast<size_t>(Fresh)], Who[K]);
			++Fresh;
		}
	}
}

void Session::ApplyFidelity(TPlayer& /*P*/, const world::Npc& /*N*/)
{
	// Phase A: the seat keeps the engine's profile (G2: the person's own from day 2, Rng("prof/" + Name), never Nr; §3.17 R1).
}

void Session::NameCrowdSeats(std::vector<size_t>& /*Rest*/, std::set<std::string>& /*Taken*/)
{
	// Phase A: no crowd, so every remaining seat is handles::Field's (G4: §5.5 R1-R2, from world/<seed>/crowdname/<eventId>).
}

uint8_t Session::PillFor(const std::string& /*PlayerId*/, int /*Today*/, int16_t& PillN)
{
	PillN = 0;
	return static_cast<uint8_t>(SeatPill::None);
}

std::string Session::NoteFor(const std::string& /*PlayerId*/, int /*Today*/)
{
	return std::string();
}

void Session::CrowdMet(const std::string& /*PlayerId*/, int /*Today*/)
{
	// Phase A: FieldCrowd is empty (G4: World::CrowdMet the first time a crowd seat sits at the player's table in an event).
}

std::string Session::DebugCrowd(const std::string& /*Args*/)
{
	return "Not built yet: the crowd comes with Update 4's G4.";
}
} // namespace ss
