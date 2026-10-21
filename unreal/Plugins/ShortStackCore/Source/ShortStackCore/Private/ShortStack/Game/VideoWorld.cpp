// Update 3 (SPEC §6.5, §7.3): the world's creators on Rerun (Home, Next up, Research), core-videocontent's file: their
// videos from the world log and the calendar, telling only the world's truth (U5). Phase A, day 1: a stub (none).
#include "ShortStack/Game/VideoContent.h"

#include "ShortStack/Game/World.h"

#include <vector>

namespace ss
{
namespace video
{
std::vector<WorldVideo> WorldVideos(const world::World& /*W*/, double /*World*/, int /*Count*/)
{
	return {};
}
} // namespace video
} // namespace ss
