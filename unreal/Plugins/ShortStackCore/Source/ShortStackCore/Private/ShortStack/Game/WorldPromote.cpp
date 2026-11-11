// Update 4, promotion (SPEC v1.7 §5.7, §5.8): the triggers, the budget, Sim::Promote, the regulars' top-up from the crowd, crowd
// scoring in festival races and the follow-promotes rule. S-promote's file from G4.
//
// Phase A (§6 PA-19, PA-23; the lead): nobody is promoted. Resolve's Discover branch asks DiscoverFromCrowd first, which answers false,
// so Sim::Discover makes the person as today. Sim::Promote is declared (G/WorldSim.h) with a stub body.
#include "ShortStack/Game/WorldCrowd.h"

#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/World.h"
#include "WorldSim.h"

namespace ss
{
namespace world
{
namespace crowd
{
bool DiscoverFromCrowd(World& /*W*/, const Pending& /*P*/, const net::Placing& /*Winner*/, int /*Day*/, int& /*Id*/)
{
	return false;
}
} // namespace crowd

int Sim::Promote(World& /*W*/, int /*Crowd*/, int /*Day*/, PromoteWhy /*Why*/)
{
	// Phase A: the crowd is empty (G4: the full record built from the crowd record, its history carried, §5.7).
	return -1;
}
} // namespace world
} // namespace ss
