// Update 4, the crowd in the hero's fields (SPEC v1.7 §5.5, §4.11.2): naming the hero's fields (dealt and big), the hero-table
// preference, crowd entries in the hero's events and their dealt finishes. S-crowd's file from G4.
//
// Phase A (§6 PA-19; the lead): created empty (the crowd is empty; handles::Field names every unnamed seat, as today).
#include "ShortStack/Game/WorldCrowd.h"

#include "ShortStack/Game/World.h"

#include <string>
#include <vector>

namespace ss
{
namespace world
{
// Update 4 (LEAD_CALLS Q-5): the crowd's naming calls, declared before the freeze. Phase A: the crowd is empty, so no seat is named
// from it (every seat stays for handles::Field, as today).
std::vector<uint32_t> World::CrowdForField(const std::string& /*EventId*/, net::SiteId /*Site*/, int /*Tier*/, const std::vector<int>& RestArchetypes)
{
	return std::vector<uint32_t>(RestArchetypes.size(), 0u);
}

uint32_t World::CrowdNameForSeat(const std::string& /*EventId*/, int /*Index*/, int /*Archetype*/, net::SiteId /*Site*/, int /*Tier*/)
{
	return 0u;
}
} // namespace world
} // namespace ss
