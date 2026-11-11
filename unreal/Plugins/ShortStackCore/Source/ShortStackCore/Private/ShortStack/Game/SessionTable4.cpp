// Update 4, at the table (SPEC v1.7 §3.17): the seating rule, seat fidelity, the one pill, the hero's notes (World::NoteObserved) and
// the ForYou greeting. W-table's file from G2 (S-crowd's crowd seats at G4).
//
// Phase A (§6 PA-19, PA-24; the lead): HeroFinished hands TableReport::Seen to NoteObserved, which keeps nothing yet (the bonds'
// note fields stay 0, written only when non-zero). The session's hooks into this file (SeatStoryRule, the fidelity overwrite,
// PillFor, §6 PA-38) land with the session's group (a1-rest).
#include "ShortStack/Game/World.h"

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
} // namespace ss
