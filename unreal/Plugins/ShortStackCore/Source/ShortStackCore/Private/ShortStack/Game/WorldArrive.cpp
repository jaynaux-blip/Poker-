// Update 4, arrivals (SPEC v1.7 §3.2.2, §3.6, §3.9, §3.18 R2): Sim::Arrive (the cause shapes, the wealthy-amateur overlay), the
// honeymoon, LeftWhy, the quick fade and the pinned regulars' top-up. W-arrive's file from G2 (S-crowd routes arrivals into the crowd
// at G4).
//
// Phase A (§6 PA-19; the lead): Sim::Arrive is declared (G/WorldSim.h) with a stub body; nothing calls it before G2 (Sim::Newcomers
// still brings today's rookies, through scene::Week).
#include "ShortStack/Game/World.h"

#include "WorldSim.h"

namespace ss
{
namespace world
{
Npc Sim::Arrive(World& /*W*/, const SceneMoment& /*M*/, int /*Day*/, int /*K*/)
{
	// Phase A: nobody arrives through the book (G2, R9: the cause from the moment's mix, then UniqueName, RookieBody and the overlay,
	// all from the fork world/<seed>/arrive/<Day>/<M.Id>/<K>).
	return Npc();
}
} // namespace world
} // namespace ss
