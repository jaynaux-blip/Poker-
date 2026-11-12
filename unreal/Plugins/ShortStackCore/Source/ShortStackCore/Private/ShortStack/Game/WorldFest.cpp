// Update 4, Festival Season (SPEC v1.7 §2.5-§2.6): the world's festival race, its crown, the hero's marks and the views the hubs,
// the card and the roll read (H/WorldFest.h), and WHO'S IN (World::FieldStoriesOf, §4.7.3). F-world's file from G1 (S-promote adds
// the crowd's race scores at G4).
//
// Phase A (§6 PA-19, PA-23-PA-25, PA-27; the lead): stubs that keep today's behaviour. fest::All() is empty, so no race opens,
// nothing scores, nothing is crowned and every view answers "none". IsCrownHonor is the spec's predicate (no Honor's id ends in
// "#pos" before G1, so its two skips change nothing).
#include "ShortStack/Game/WorldFest.h"

#include "ShortStack/Game/World.h"

#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace fest
{
/** World's friend for the festival modules (§6.2): the store World keeps for them (F-world's reads and writes from G1). */
struct WorldAccess
{
	static world::FestRace& Race(world::World& W) { return W.Fest; }
	static const world::FestRace& Race(const world::World& W) { return W.Fest; }
	static world::FestFinal& Last(world::World& W) { return W.FestLast; }
	static const world::FestFinal& Last(const world::World& W) { return W.FestLast; }
	static std::vector<world::HeroFestMark>& Marks(world::World& W) { return W.HeroFest; }
	static const std::vector<world::HeroFestMark>& Marks(const world::World& W) { return W.HeroFest; }
};

double MarkCutoff(const world::World& /*W*/, world::FestMark /*Mark*/)
{
	// Phase A: no race runs (G1: the points of the scorer holding the mark's last rank, cached per world Rev).
	return 0.0;
}

/** World::FestViews' contents (H0, LEAD_CALLS R-10): empty, and never made, until F-world's per-Rev views (FW-2a). */
struct WorldViews
{
};
} // namespace fest

namespace world
{
// ---- the race and the crown (§2.5.2, §2.6.1)

void FestDay(World& /*W*/, int /*Today*/)
{
	// Phase A: no festival opens (fest::All() is empty).
}

void FestHour(World& /*W*/, double /*Now*/)
{
	// Phase A: no race to crown, no Afterglow to end.
}

void FestScoreResult(World& /*W*/, const Pending& /*P*/, int /*Npc*/, int /*Place*/)
{
	// Phase A: Fest.Key is never set, so no finish scores.
}

void FestScoreHero(World& /*W*/, const Pending& /*Ev*/, int /*Place*/)
{
	// Phase A: as FestScoreResult, for the hero.
}

std::vector<std::pair<int, FestScore>> FestRank(const FestRace& /*Race*/)
{
	// Phase A: no race (G1: points descending, a person ahead of the hero on a tie, then the lower npc id).
	return {};
}

// ---- the views (§2.6.6)

std::vector<std::pair<int, FestScore>> FestTop(const World& /*W*/, int /*Count*/)
{
	return {};
}

std::pair<int, int> FestStanding(const World& /*W*/, int /*Npc*/)
{
	return {0, 0};
}

std::vector<std::pair<int, int>> FestKnown(const World& /*W*/)
{
	return {};
}

double FestCrownedAt(const World& /*W*/, const std::string& /*Key*/)
{
	return -1.0;
}

std::vector<FestRollRow> FestRoll(const World& /*W*/, fest::Brand /*B*/)
{
	return {};
}

const Honor* FestDefending(const World& /*W*/, fest::Brand /*B*/, bool /*Crown*/, int /*Day*/)
{
	return nullptr;
}

bool IsDefending(const World& /*W*/, int /*Npc*/, int /*Day*/)
{
	return false;
}

bool IsCrownHonor(const Honor& H)
{
	// The Player of the Series belt (§2.6.1 step 2: EventId = the series id + "#pos"). No event id ends so: they are
	// "<template>@<minute>", "roty#<year>" from G2.
	static const std::string FestPosTail = "#pos";
	return H.EventId.size() >= FestPosTail.size() && H.EventId.compare(H.EventId.size() - FestPosTail.size(), FestPosTail.size(), FestPosTail) == 0;
}

int TitleRank(const Honor& /*H*/)
{
	// Phase A: no festival, so no Honor is a festival title (G1: the class from the brand, the template and IsCrownHonor).
	return 0;
}

bool FestivalTitle(const Pending& /*P*/)
{
	// Phase A: no edition has TitleIds (G1: fest::IsTitleTemplate of the pending event's template).
	return false;
}

std::vector<HeroFestPin> HeroFestPins(const World& /*W*/)
{
	return {};
}

FestMark HeroMark(const World& /*W*/, const std::string& /*Key*/)
{
	return FestMark::None;
}

// ---- G1's appends (the lead's pre-wave commit H0, PLAN_G1 §4.2): stubs until F-world's bodies (FestRaceKey FW-2a, HeroMarks FW-2b,
// HeroFestResults FW-3, FestWhyLine FW-4). No race, mark or festival result exists before them, so each answers "none".

const std::vector<HeroFestMark>& HeroMarks(const World& /*W*/)
{
	static const std::vector<HeroFestMark> FestMarksNone;
	return FestMarksNone;
}

const std::string& FestRaceKey(const World& /*W*/)
{
	static const std::string FestRaceNone;
	return FestRaceNone;
}

std::string FestWhyLine(const World& /*W*/, const StoryPerson& /*Person*/, const std::string& /*EventId*/)
{
	return std::string();
}

std::vector<HeroFestResult> HeroFestResults(const World& /*W*/, const std::string& /*Key*/)
{
	return {};
}

// ---- WHO'S IN (§4.7.3, PA-27)

const FieldStories& World::FieldStoriesOf(const std::string& /*EventId*/) const
{
	// Phase A: nobody's story is told yet (G1: F-world's People and the Qualifiers group, cached per world revision).
	static const FieldStories FestStoriesNone;
	return FestStoriesNone;
}
} // namespace world
} // namespace ss
