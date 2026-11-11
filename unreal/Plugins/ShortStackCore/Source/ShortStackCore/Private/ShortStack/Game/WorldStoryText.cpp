// Update 4, the stories' words (SPEC v1.7 §3.13-§3.16, §3.19, §3.20): STORY lines, Notebook beats, the monthly rankings, Dee's
// texts, NEWS RISING and NEW FACES, PA honorifics, town lines, the PASSED YOU greetings and Rerun keys. W-story's file from G2.
//
// Phase A (§6 PA-10, PA-19, PA-24, PA-30, PA-31; the lead): every reader falls through empty: Headline false, TownKnown empty, PaHonor
// and PassedGreeting "", NewsExtras adds nothing, the inventory is empty. World::NewsExtras (a1-net's stub, PA-10) lives here now,
// beside the posts it will make (W-story's at G2; G/WorldText.cpp is F-world's at G1 and W-ui's at G2).
#include "ShortStack/Game/WorldStory.h"

#include "ShortStack/Game/World.h"

#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace world
{
namespace story
{
bool Headline(const World& /*W*/, const WorldEvent& /*E*/, std::string& /*Title*/, std::string& /*Body*/, std::string& /*Tag*/)
{
	// Phase A: no TpiMark or SceneEra is ever posted, and nobody debuts under the book.
	return false;
}

std::vector<int> TownKnown(const World& /*W*/)
{
	return {};
}

std::string PaHonor(const std::string& /*Name*/, int& /*Rank*/)
{
	return std::string();
}

std::string PassedGreeting(const World& /*W*/, int /*Npc*/, uint32_t /*Salt*/, bool /*Spoken*/)
{
	// Phase A: nobody has passed the player (Bond::PassedDay is 0).
	return std::string();
}

std::vector<std::pair<std::string, std::string>> Inventory()
{
	return {};
}
} // namespace story

void World::NewsExtras(net::SiteId /*Site*/, double /*Now*/, std::vector<net::NewsItem>& /*Items*/) const
{
	// Update 4 (SPEC v1.7 §6 PA-10): Network::News calls this after its world-log blocks. A no-op until G2 (W-story's RISING posts and
	// NEW FACES, §3.15 R3, §3.19.4).
}
} // namespace world
} // namespace ss
