// Update 4, Festival Season (SPEC v1.7 §2.5-§2.6): the world's festival race, its crown, the hero's marks and the views the hubs,
// the card and the roll read (H/WorldFest.h), and WHO'S IN (World::FieldStoriesOf, §4.7.3). F-world's file from G1 (S-promote adds
// the crowd's race scores at G4).
//
// Phase A (§6 PA-19, PA-23-PA-25, PA-27; the lead): stubs that keep today's behaviour. IsCrownHonor is the spec's predicate (no
// Honor's id ends in "#pos" before G1, so its two skips change nothing).
//
// G1, FW-2a (F-world; PLAN_FWORLD §4 FW-2, Corrections item 1): the race and its marks (§2.5.1-§2.5.4). FestDay opens a festival's
// race on its FirstDay (its site open in this world, no crown Honor for its key); FestScoreResult and FestScoreHero add a cash's
// fest::RacePoints to the scorer's row (online, the live key's series, never a satellite, Place >= 1, points > 0; the hero's result once,
// World::HeroFinished's !Again); FestRank orders a race (points, then a person ahead of the hero, then the lower id); MarkFor,
// FestLastRank and FestPointsTo (G/WorldFestDetail.h, R-4) and fest::MarkCutoff are the marks' rule and the ladder; FestTop,
// FestStanding, FestKnown and FestRaceKey read the live race, else the last crown's top ten (FestLast, the Afterglow). The ranked
// race is kept in World's FestViews (R-10; a copied world starts without it), rebuilt when the world's stamp (serial, Rev, HeroRev)
// moves or a race writer here drops it. No RNG (§2.12): sums and a sort with a total order. The crown (FestHour, FestCrownedAt,
// HeroMark, HeroMarks) is FW-2b's; until then a race stays open past its last event and a stale key is cleared, uncrowned, when the
// next festival opens (the calendar never meets that case once FestHour crowns).
#include "ShortStack/Game/WorldFest.h"

#include "ShortStack/Game/World.h"
#include "WorldFestDetail.h"
#include "WorldSim.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace fest
{
/** World::FestViews' contents (H0, LEAD_CALLS R-10): the festival views kept per world state. Never saved; a copied World starts
 *  without them (World::FestViewBox), so a snapshot builds its own on its one reader (the host's writer, which reads none). */
struct WorldViews
{
	/** The race the views read, ranked once per world state: the live race, else the last crown's top ten (the Afterglow). */
	struct RaceView
	{
		bool Valid = false;
		uint64_t Serial = 0; // the world state it was ranked at (WorldStamp: serial, Rev, HeroRev), and the race's key and rows then
		int Rev = -1, HeroRev = -1;
		std::string Key;
		size_t Rows = 0;
		bool Live = false;                                    // the live race (else FestLast's top ten, or nothing)
		int Scorers = 0;                                      // the race's scorers (FestLast.Scorers in the Afterglow)
		std::vector<std::pair<int, world::FestScore>> Order;  // rank order: every scorer live, the top ten after the crown
		std::vector<std::pair<int, int>> RankOf;              // (npc, rank) by npc, for the standing's lookups
		std::vector<std::pair<int, int>> Known;               // FestKnown: (npc, rank), race order
	};
	RaceView Race;
};

/** World's friend for the festival modules (§6.2): the store World keeps for them (F-world's reads and writes from G1). */
struct WorldAccess
{
	static world::FestRace& Race(world::World& W) { return W.Fest; }
	static const world::FestRace& Race(const world::World& W) { return W.Fest; }
	static world::FestFinal& Last(world::World& W) { return W.FestLast; }
	static const world::FestFinal& Last(const world::World& W) { return W.FestLast; }
	static std::vector<world::HeroFestMark>& Marks(world::World& W) { return W.HeroFest; }
	static const std::vector<world::HeroFestMark>& Marks(const world::World& W) { return W.HeroFest; }
	/** The views' cache, made on first use (a display read on the world's one reader thread: mutable, no lock, PLAN_G1 PC-9). */
	static WorldViews& Views(const world::World& W)
	{
		if (!W.FestViews.P)
		{
			W.FestViews.P = std::make_shared<WorldViews>();
		}
		return *W.FestViews.P;
	}
	/** A race writer changed the race: the ranked view is rebuilt on its next read (nothing is made when no view was read). */
	static void DropRace(world::World& W)
	{
		if (W.FestViews.P)
		{
			W.FestViews.P->Race.Valid = false;
		}
	}
};
} // namespace fest

namespace world
{
namespace worldfest_detail
{
/** ceil(Share × Scorers) in whole ranks (§2.5.3; FestMarksNested holds it to the whole-number rule for every S to 3,000). */
int FestShareRanks(double Share, int Scorers)
{
	return static_cast<int>(std::ceil(Share * static_cast<double>(Scorers)));
}

/** Whether a crown Honor already names the series (§2.5.2 rule 1: a crowned edition never reopens; its EventId is "<series>#pos"). */
bool FestCrownedKey(const World& W, const std::string& Key)
{
	const std::string Id = Key + "#pos";
	for (const Honor& H : W.Honors())
	{
		if (H.EventId == Id)
		{
			return true;
		}
	}
	return false;
}

/** One finish into the live race (§2.5.2 rules 2-3): online, the live key's series, not a satellite, a place; a row once it scores. */
void FestAddFinish(World& W, const Pending& P, int Npc, int Place)
{
	FestRace& Race = fest::WorldAccess::Race(W);
	if (Race.Key.empty() || !P.Online || P.Series != Race.Key || Place < 1 || (P.Kinds & KindSatellite) != 0)
	{
		return;
	}
	const double Pts = fest::RacePoints(Place, P.Entries, P.BuyIn, Sim::CurveOf(P));
	if (!(Pts > 0.0))
	{
		return;
	}
	FestScore& Row = Race.Scores[Npc];
	Row.Points += Pts;
	++Row.Cashes;
	fest::WorldAccess::DropRace(W);
}

/** The ranked race the views read, rebuilt when the world's stamp moved or a writer dropped it (§2.6.6: cached per world Rev). */
const fest::WorldViews::RaceView& FestRaceView(const World& W)
{
	fest::WorldViews::RaceView& V = fest::WorldAccess::Views(W).Race;
	const WorldStamp Stamp = W.SnapshotStamp();
	const FestRace& Race = fest::WorldAccess::Race(W);
	const FestFinal& Last = fest::WorldAccess::Last(W);
	const std::string& Key = !Race.Key.empty() ? Race.Key : Last.Key;
	const size_t Rows = !Race.Key.empty() ? Race.Scores.size() : Last.Top.size();
	if (V.Valid && V.Serial == Stamp.Serial && V.Rev == Stamp.Rev && V.HeroRev == Stamp.HeroRev && V.Rows == Rows && V.Key == Key)
	{
		return V;
	}
	V = fest::WorldViews::RaceView();
	if (!Race.Key.empty())
	{
		V.Live = true;
		V.Order = FestRank(Race);
		V.Scorers = static_cast<int>(V.Order.size());
	}
	else if (!Last.Key.empty())
	{
		V.Order = Last.Top;
		V.Scorers = Last.Scorers;
	}
	V.RankOf.reserve(V.Order.size());
	const std::map<int, Bond>& Bonds = W.Bonds();
	for (size_t K = 0; K < V.Order.size(); ++K)
	{
		const int Id = V.Order[K].first;
		const int Rank = static_cast<int>(K) + 1;
		V.RankOf.emplace_back(Id, Rank);
		if (Id < 0)
		{
			continue;
		}
		// PEOPLE YOU KNOW (§2.5.4): the player's rival, or a bond holding a memory.
		const Npc* Person = W.Get(Id);
		const auto Held = Bonds.find(Id);
		if ((Person && Person->Rival) || (Held != Bonds.end() && !Held->second.Memories.empty()))
		{
			V.Known.emplace_back(Id, Rank);
		}
	}
	std::sort(V.RankOf.begin(), V.RankOf.end());
	V.Key = Key;
	V.Rows = Rows;
	V.Serial = Stamp.Serial;
	V.Rev = Stamp.Rev;
	V.HeroRev = Stamp.HeroRev;
	V.Valid = true;
	return V;
}

/** A scorer's rank in the view (0: not in it). */
int FestRankIn(const fest::WorldViews::RaceView& V, int Npc)
{
	const auto It = std::lower_bound(V.RankOf.begin(), V.RankOf.end(), std::make_pair(Npc, 0));
	return It != V.RankOf.end() && It->first == Npc ? It->second : 0;
}
} // namespace worldfest_detail

// ---- the marks (§2.5.3; G/WorldFestDetail.h)

int FestFinalistRanks(int Scorers)
{
	if (Scorers <= 0)
	{
		return 0;
	}
	if (Scorers >= fest::kFinalistSmallBelow)
	{
		return std::clamp(worldfest_detail::FestShareRanks(fest::kTop10, Scorers), 3, fest::kFinalistCap);
	}
	return std::clamp(worldfest_detail::FestShareRanks(fest::kFinalistSmallShare, Scorers), 3, fest::kFinalistSmall);
}

FestMark MarkFor(int Rank, int Scorers)
{
	if (Rank < 1 || Rank > Scorers)
	{
		return FestMark::None;
	}
	if (Rank == 1)
	{
		return FestMark::Title;
	}
	if (Rank <= 3)
	{
		return FestMark::Podium;
	}
	if (Rank <= FestFinalistRanks(Scorers))
	{
		return FestMark::Finalist;
	}
	if (Rank <= worldfest_detail::FestShareRanks(fest::kTop10, Scorers))
	{
		return FestMark::Top10;
	}
	if (Rank <= worldfest_detail::FestShareRanks(fest::kTop25, Scorers))
	{
		return FestMark::Top25;
	}
	return FestMark::None;
}

int FestLastRank(FestMark Mark, int Scorers)
{
	if (Scorers <= 0 || Mark <= FestMark::None || Mark >= FestMark::Count)
	{
		return 0;
	}
	// The widest rank among the grades at or above Mark (MarkFor takes the best grade a rank reaches).
	int Last = 1;
	if (Mark <= FestMark::Podium)
	{
		Last = std::max(Last, 3);
	}
	if (Mark <= FestMark::Finalist)
	{
		Last = std::max(Last, FestFinalistRanks(Scorers));
	}
	if (Mark <= FestMark::Top10)
	{
		Last = std::max(Last, worldfest_detail::FestShareRanks(fest::kTop10, Scorers));
	}
	if (Mark <= FestMark::Top25)
	{
		Last = std::max(Last, worldfest_detail::FestShareRanks(fest::kTop25, Scorers));
	}
	return std::min(Last, Scorers);
}

int FestPointsTo(const World& W, FestMark Mark)
{
	const fest::WorldViews::RaceView& V = worldfest_detail::FestRaceView(W);
	const int Hero = V.Live ? worldfest_detail::FestRankIn(V, -1) : 0;
	const int Last = V.Live ? FestLastRank(Mark, V.Scorers) : 0;
	if (Hero == 0 || Last == 0)
	{
		return -1; // no live race, no such mark, or no cash yet: CASH 1 is the lit rung, with no number
	}
	if (Hero <= Last)
	{
		return 0;
	}
	const double Cutoff = V.Order[static_cast<size_t>(Last - 1)].second.Points;
	const double Mine = V.Order[static_cast<size_t>(Hero - 1)].second.Points;
	return static_cast<int>(std::llround(Cutoff - Mine)) + 1;
}

// ---- the race and the crown (§2.5.2, §2.6.1)

void FestDay(World& W, int Today)
{
	for (const fest::Festival* F : fest::Festivals(Today, Today))
	{
		const double Launched = W.SiteLaunched(F->Site);
		if (F->FirstDay != Today || Launched < 0.0 || Launched > W.Clock() || worldfest_detail::FestCrownedKey(W, F->SeriesId))
		{
			continue;
		}
		FestRace& Race = fest::WorldAccess::Race(W);
		if (Race.Key == F->SeriesId)
		{
			continue; // open already
		}
		// A stale key (an earlier festival's race still open; impossible under the calendar once FestHour crowns, FW-2b) is crowned
		// first there; until then it closes uncrowned.
		Race = FestRace();
		Race.Key = F->SeriesId;
		fest::WorldAccess::DropRace(W);
	}
}

void FestHour(World& /*W*/, double /*Now*/)
{
	// Phase A: no race to crown, no Afterglow to end (FW-2b: the crown, §2.6.1).
}

void FestScoreResult(World& W, const Pending& P, int Npc, int Place)
{
	worldfest_detail::FestAddFinish(W, P, Npc, Place);
}

void FestScoreHero(World& W, const Pending& Ev, int Place)
{
	// The hero's row is -1; World::HeroFinished calls this once per result (inside !Again: a result told twice is one result).
	worldfest_detail::FestAddFinish(W, Ev, -1, Place);
}

std::vector<std::pair<int, FestScore>> FestRank(const FestRace& Race)
{
	std::vector<std::pair<int, FestScore>> Order(Race.Scores.begin(), Race.Scores.end());
	// A total order (§2.12): points descending; on a tie a person ahead of the hero (-1: the hero must be strictly ahead, the Player
	// of the Year precedent), then the lower npc id.
	std::sort(Order.begin(), Order.end(), [](const std::pair<int, FestScore>& A, const std::pair<int, FestScore>& B) {
		if (A.second.Points != B.second.Points)
		{
			return A.second.Points > B.second.Points;
		}
		if ((A.first < 0) != (B.first < 0))
		{
			return B.first < 0;
		}
		return A.first < B.first;
	});
	return Order;
}

// ---- the views (§2.6.6)

std::vector<std::pair<int, FestScore>> FestTop(const World& W, int Count)
{
	const fest::WorldViews::RaceView& V = worldfest_detail::FestRaceView(W);
	const size_t N = std::min(V.Order.size(), static_cast<size_t>(std::max(0, Count)));
	return std::vector<std::pair<int, FestScore>>(V.Order.begin(), V.Order.begin() + static_cast<std::ptrdiff_t>(N));
}

std::pair<int, int> FestStanding(const World& W, int Npc)
{
	const fest::WorldViews::RaceView& V = worldfest_detail::FestRaceView(W);
	const int Rank = worldfest_detail::FestRankIn(V, Npc);
	if (Rank == 0 && !V.Live && Npc < 0)
	{
		// After the crown the hero's place outside the top ten is their mark's (FW-2b writes it at the crown).
		const std::string& Key = fest::WorldAccess::Last(W).Key;
		for (const HeroFestMark& M : fest::WorldAccess::Marks(W))
		{
			if (!Key.empty() && M.Key == Key)
			{
				return {M.Rank, V.Scorers};
			}
		}
	}
	return {Rank, V.Scorers};
}

std::vector<std::pair<int, int>> FestKnown(const World& W)
{
	return worldfest_detail::FestRaceView(W).Known;
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

const std::string& FestRaceKey(const World& W)
{
	// The race FestTop, FestStanding, FestKnown and MarkCutoff read: the live one from its opening, else the last crown's through the
	// Afterglow (FW-2b clears FestLast after it), else none.
	const std::string& Live = fest::WorldAccess::Race(W).Key;
	return !Live.empty() ? Live : fest::WorldAccess::Last(W).Key;
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

namespace fest
{
double MarkCutoff(const world::World& W, world::FestMark Mark)
{
	// The ladder's cut-off (§2.5.3, U27): the points of the scorer holding the mark's last rank in the live race right now (the
	// ranked view, kept per world state); 0 when no race runs or no scorer holds that rank yet.
	const WorldViews::RaceView& V = world::worldfest_detail::FestRaceView(W);
	const int Last = V.Live ? world::FestLastRank(Mark, V.Scorers) : 0;
	return Last > 0 ? V.Order[static_cast<size_t>(Last - 1)].second.Points : 0.0;
}
} // namespace fest
} // namespace ss
