// Update 4, the scene's words (SPEC v1.7 §3.2.2, §3.4, §3.19, §3.20): arrival lines for every moment kind, NEW FACES, the Summer's
// lines and the CAME FOR labels (scene::ArrivalLine, scene::Inventory). W-arrive's file from G2.
//
// Phase A (§6 PA-19, PA-30; the lead): nobody has a moment (Npc::Moment is 0), so ArrivalText never falls through to ArrivalLine;
// the inventory is empty (G2 fills it, copy_lint_world reads it).
#include "ShortStack/Game/WorldScene.h"

#include "ShortStack/Game/World.h"

#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace world
{
namespace scene
{
std::string ArrivalLine(const World& /*W*/, const Npc& /*N*/)
{
	// Phase A: unreachable (Moment 0 for everyone); G2's lines by cause and arrival day (§3.2.2's copy table).
	return std::string();
}

std::vector<std::pair<std::string, std::string>> Inventory()
{
	return {};
}
} // namespace scene
} // namespace world
} // namespace ss
