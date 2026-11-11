// Update 4, the Story Director (SPEC v1.7 §3.15, §3.16): scoring, budgets, Names to Watch, Dee's tip, PASSED YOU, Rookie of the
// Year, sponsors on results, the follow list and the StoryNote queue. W-story's file from G2.
//
// Phase A (§6 PA-19, PA-23; the lead): the Director tells nothing: every hook is a no-op, the follow list stays empty and
// TakeStoryNotes hands over nothing. story::Access is defined here (§6.2).
#include "ShortStack/Game/WorldStory.h"

#include "ShortStack/Game/World.h"

#include <array>
#include <deque>
#include <string>
#include <vector>

namespace ss
{
namespace world
{
namespace story
{
/** World's friend for the story modules (§6.2): the books World keeps for them (W-story's reads and writes from G2). */
struct Access
{
	static TpiBook& Book(World& W) { return W.Tpi; }
	static TpiRec& Hero(World& W) { return W.HeroTpi; }
	static ArcBook& Arcs(World& W) { return W.Arcs; }
	static std::vector<FollowRec>& Follows(World& W) { return W.Follows; }
	static std::array<WatchPick, 12>& Watch(World& W) { return W.Watch; }
	static std::deque<FeedItem>& Feed(World& W) { return W.Feed; }
	static DeeTip& Tip(World& W) { return W.Tip; }
};

void Day(World& /*W*/, int /*Today*/)
{
	// Phase A: no urgent pass (titles, PASSED YOU).
}

void Week(World& /*W*/, int /*Today*/)
{
	// Phase A: no re-rank, no arc week, no Director week.
}

void Year(World& /*W*/, int /*Calendar*/, int /*Today*/)
{
	// Phase A: no Rookie of the Year (G2: the FIRST-YEARS leader's Honor, §3.16 R2).
}

void SponsorOnResults(World& /*W*/, Npc& /*N*/, int /*Today*/)
{
	// Phase A: today's follower path alone signs sponsors (G2: Run100 >= 8 professionals, §3.16 R3).
}
} // namespace story

// ---- the session's calls (§3.15 R6; each drains the hold from G2)

void World::Follow(int /*Npc*/)
{
}

void World::Unfollow(int /*Npc*/)
{
}

void World::Follow(const std::string& /*CrowdName*/)
{
}

std::vector<StoryNote> World::TakeStoryNotes()
{
	return {};
}
} // namespace world
} // namespace ss
