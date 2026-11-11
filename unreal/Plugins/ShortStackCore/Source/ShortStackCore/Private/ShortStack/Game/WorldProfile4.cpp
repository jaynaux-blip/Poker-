// Update 4, the card's story (SPEC v1.7 §3.19): World::StoryOf (the STORY tab's data), the Notebook's views, the ProfileOf cache and
// World::SceneChart. W-ui's file from G2.
//
// Phase A (§6 PA-19, PA-30; the lead): ProfileOf's fall-through, story::ProfileExtras, adds nothing.
#include "ShortStack/Game/WorldStory.h"

#include "ShortStack/Game/World.h"

namespace ss
{
namespace world
{
namespace story
{
void ProfileExtras(const World& /*W*/, const Npc& /*N*/, Profile& /*Pr*/)
{
	// Phase A: the card is today's.
}
} // namespace story
} // namespace world
} // namespace ss
