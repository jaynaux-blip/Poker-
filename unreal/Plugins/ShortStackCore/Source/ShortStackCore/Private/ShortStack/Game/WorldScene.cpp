// Update 4, the Scene Book (SPEC v1.7 §3.2-§3.5, §3.18): posting, the 09:00 step, the Curve Keeper, CapNow and its anchors, Summers
// and ripples, the wake, NEW FACES and the pinned Embercrest regulars (H/WorldScene.h). W-arrive's file from G2 (S-crowd adds the
// crowd's arrivals, the scene count and NEW FACES' crowd count at G4).
//
// Phase A (§6 PA-19, PA-23, PA-24, PA-52; the lead): the book never opens, so every hook is a no-op, except scene::Week, which runs
// today's Sim::Newcomers in its place (the same draws from the week's stream R, in the same order). PinRegulars is present and
// disabled: nothing calls it, so World::Regulars stays empty and EmbercrestRegulars keeps today's per-day sort.
#include "ShortStack/Game/WorldScene.h"

#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Rng.h"
#include "WorldSim.h"

#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace world
{
namespace scene
{
/** World's friend for the scene modules (§6.2): the store World keeps for them (W-arrive's reads and writes from G2). */
struct Access
{
	static SceneBook& Book(World& W) { return W.Scene; }
	static const SceneBook& Book(const World& W) { return W.Scene; }
	static std::vector<int>& Regulars(World& W) { return W.Regulars; }
};

void Day(World& /*W*/, int /*Today*/)
{
	// Phase A: the book hasn't opened (AnchorDay -1): nothing is posted.
}

void Hour(World& /*W*/, double /*Now*/)
{
	// Phase A: no moment carries anything, so the 09:00 step emits nobody.
}

void Week(World& W, int Today, Rng& R)
{
	// Phase A: today's newcomers, exactly (G2: the keeper's weekly step takes Newcomers' slot and draws nothing from R; §3.2 R1).
	Sim::Newcomers(W, Today, R);
}

void Month(World& /*W*/, int /*Today*/)
{
	// Phase A: nobody has GFHoneyLeft, so the quick fade has nobody to fade.
}

void OnResolved(World& /*W*/, const Pending& /*P*/, const net::EventResult& /*Res*/)
{
	// Phase A: no ripple and no Summer is posted.
}

void OnHeroResult(World& /*W*/, const std::string& /*EventId*/, int /*Place*/, Chips /*Prize*/)
{
	// Phase A: no wake (the book is shut; results before it opens post nothing, WI-1).
}

void OnRoom(World& /*W*/, const std::string& /*EventId*/, int /*HeroPlace*/, int /*Field*/)
{
	// Phase A: no Room moment.
}

void Left(World& /*W*/, Npc& /*N*/, int /*Today*/, bool /*Broke*/)
{
	// Phase A: LeftWhy stays 0 (written only when non-zero, so no npc line moves).
}

void Honeymoon(World& /*W*/, Npc& /*N*/, int /*Today*/)
{
	// Phase A: nobody arrived under the book (Moment 0), so nobody is in a honeymoon.
}

void OnStreamMilestone(World& /*W*/, Npc& /*N*/, int /*Today*/, int /*Followers*/)
{
	// Phase A: no Stream moment.
}

int SceneCount(const World& /*W*/)
{
	// Phase A: unread (G2: Active or on a Break and not Faded; from G4 the crowd's too).
	return 0;
}

double Planned(const SceneBook& /*B*/, int /*Today*/)
{
	// Phase A: the book is shut (G2: the keeper's S-curve, §3.3).
	return 0.0;
}

double Audience(int /*Scene*/)
{
	// Phase A: unread (G2: sqrt(Scene / 1657) x max(0.15, 1 - Scene / 10000), §3.2).
	return 0.0;
}

void PinRegulars(World& W)
{
	// The pin (§3.18 R1), disabled in Phase A: nothing calls it. G2 calls it when the book opens, so a new career's list equals
	// today's: EmbercrestRegulars reads today's rule while Regulars is empty.
	std::vector<int>& Pinned = Access::Regulars(W);
	Pinned.clear();
	Pinned = W.EmbercrestRegulars();
}
} // namespace scene

// ---- the session's calls (§3.5 R1, §3.19.4)

void World::NoteReach(int /*Day*/, int /*Followers*/)
{
	// Phase A: no You moment (W-arrive at G2: drains the hold, then posts each threshold crossed once, ReachDone).
}

std::vector<std::pair<int, int>> World::NewFacesWeek(net::SiteId /*Site*/, int /*Monday*/) const
{
	// Phase A: nobody arrived under the book (W-arrive at G2; from G4 the crowd's arrivals too).
	return {};
}
} // namespace world

// ---- ss.Scene.* and ss.Turn.* (game/Update4Commands.cpp, §3.26, §6 PA-50): W-arrive's from G2 (the held turn's switch is theirs)

std::string Session::DebugScene(const std::string& /*Args*/)
{
	return "Not built yet: the Scene and the held turn come with Update 4's G2.";
}
} // namespace ss
