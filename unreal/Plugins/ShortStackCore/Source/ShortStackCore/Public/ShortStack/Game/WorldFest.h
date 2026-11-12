#pragma once

#include "ShortStack/Common.h"
#include "ShortStack/Game/Festival.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace world
{
class World;
struct Honor;
struct Pending;

/**
 * Update 4, Festival Season (SPEC v1.7 §2.5-§2.6; Phase A's declarations, §6 PA-19): the world's festival race (Player of the
 * Series), its crown, the marks the hero keeps, and the views the hubs, the card and the roll read. It includes nothing of World.h
 * (World.h includes it and appends the store to World: Fest, FestLast, HeroFest; the WorldLeague.h precedent). Bodies in
 * G/WorldFest.cpp (F-world's at G1); the private reads and writes go through fest::WorldAccess, World's friend (§6.2). Frozen after
 * Phase A (append-only, by patch to the lead).
 *
 * Phase A: every body is a stub that keeps today's behaviour. No festival exists (fest::All() is empty), so no race opens, nothing
 * scores, nothing is crowned, and every view answers "none": FestTop and FestKnown empty, FestStanding (0, 0), FestCrownedAt -1,
 * FestRoll empty, FestDefending null, IsDefending false, TitleRank 0, FestivalTitle false, HeroFestPins empty, HeroMark None.
 */
struct FestScore
{
	double Points = 0.0; // net::Points' integers kept as doubles
	int Cashes = 0;
};
struct FestRace
{
	std::string Key;                     // the live festival's series id; "" when none is running
	std::map<int, FestScore> Scores;     // npc id -> score; -1 the hero (a row exists once Points > 0)
	/** G4's crowd scorers by crowd name (§5.8 R1): declared empty now so this header never changes after the freeze (§6.7); folded into
	 *  world\tfest\trace as a second record list written only when non-empty. */
	std::map<std::string, FestScore> CrowdScores;
};
/** The last crown, kept kAfterglowDays (§2.6.1). */
struct FestFinal
{
	std::string Key;
	double At = 0.0; // the crown's world minute
	int Scorers = 0;
	std::vector<std::pair<int, FestScore>> Top; // up to 10, rank order; -1 the hero
};
/** Saved as int: append-only, clamped on read. */
enum class FestMark : int
{
	None,
	Top25,
	Top10,
	Finalist,
	Podium,
	Title,
	Count,
};
struct HeroFestMark
{
	std::string Key;
	FestMark Mark = FestMark::None;
	int Rank = 0, Scorers = 0;
	double Points = 0.0;
	/** A crown ticket still unpaid (§2.6.1 step 7, fest::kCrownTickets; the record's trailing `t`, §2.13): FINALIST or better earns a
	 *  seat in the next edition, paid once by the session's FestStep. */
	bool Ticket = false;
};
/** Derived (§2.6.4): one pin per edition the hero played; Grade 1 outline (played), 2 silver (cashed), 3 gold (won an event). */
struct HeroFestPin
{
	std::string Key;
	int Grade = 0;
	int Events = 0, Cashes = 0, Wins = 0, BestPlace = 0, BestEntries = 0;
};
/** A champions' roll row, by edition, newest first (§2.6.2). */
struct FestRollRow
{
	std::string Key;
	int Year = 0, Edition = 0;
	const Honor* Main = nullptr;
	const Honor* Crown = nullptr;
	const Honor* Mini = nullptr;  // v1.6: a championship's Mini and Micro (else null)
	const Honor* Micro = nullptr;
};

// ---- the race (§2.5.2) and the crown (§2.6.1): the world's own hooks (G/WorldSim.cpp, G/WorldHero.cpp; Phase A's calls, §6 PA-23,
// PA-24). Each a no-op in Phase A.
/** Sim::NewDay's first step: opens the race of a festival whose FirstDay is Day on a site open in this world. */
SHORTSTACKCORE_API void FestDay(World& W, int Day);
/** World::AdvanceTo's hourly block: crowns the race once its last event has resolved (and clears FestLast after the Afterglow). */
SHORTSTACKCORE_API void FestHour(World& W, double Now);
/** Sim::Apply (beside RiverLine's series points): a person's finish in a live festival's numbered event, headliner or Main. */
SHORTSTACKCORE_API void FestScoreResult(World& W, const Pending& P, int Npc, int Place);
/** World::HeroFinished (inside !Again and Online): the hero's finish, scored as a person's with Npc -1. */
SHORTSTACKCORE_API void FestScoreHero(World& W, const Pending& Ev, int Place);
/** The race in rank order (§2.5.2 rule 5): points descending, a person ahead of the hero on a tie, then the lower npc id. */
SHORTSTACKCORE_API std::vector<std::pair<int, FestScore>> FestRank(const FestRace& Race);

// ---- views the UI reads (§2.6.6; all const, cached per world Rev).
SHORTSTACKCORE_API std::vector<std::pair<int, FestScore>> FestTop(const World& W, int Count); // the live race, else FestLast.Top
SHORTSTACKCORE_API std::pair<int, int> FestStanding(const World& W, int Npc);                 // (rank, scorers); rank 0: not scoring
SHORTSTACKCORE_API std::vector<std::pair<int, int>> FestKnown(const World& W);                // (npc, rank), race order
SHORTSTACKCORE_API double FestCrownedAt(const World& W, const std::string& Key);              // -1: not crowned
SHORTSTACKCORE_API std::vector<FestRollRow> FestRoll(const World& W, fest::Brand B);
SHORTSTACKCORE_API const Honor* FestDefending(const World& W, fest::Brand B, bool Crown, int Day);
SHORTSTACKCORE_API bool IsDefending(const World& W, int Npc, int Day);
/** A crown Honor (the Player of the Series belt): its EventId ends in "#pos" (§2.6.1). HallScore and the award rebuild skip them. */
SHORTSTACKCORE_API bool IsCrownHonor(const Honor& H);
/** v1.6: the title order's class, 1 (a championship Main) to 5 (a championship Mini or Micro), 0 not a title; callers then order by
 *  Prize (belts by points), then At (§2.6.3). */
SHORTSTACKCORE_API int TitleRank(const Honor& H);
/** v1.6: the template is in its festival's TitleIds (Discover, §2.6.5). */
SHORTSTACKCORE_API bool FestivalTitle(const Pending& P);
SHORTSTACKCORE_API std::vector<HeroFestPin> HeroFestPins(const World& W);
SHORTSTACKCORE_API FestMark HeroMark(const World& W, const std::string& Key);
// WHO'S IN is one query for the hub, CHIP COUNTS and the broadcast: World::FieldStoriesOf(EventId) (§4.7.3, declared in Phase A,
// §6 PA-27). At G1 F-world fills its People (rival, met, the defending champion and the roll's champions, team pros, streamers,
// satellite qualifiers) and the Qualifiers group; G2 and G3 add the rest (§4.7.3's ownership list).

// ---- G1's appends (the lead's pre-wave commit H0; LEAD_CALLS R-2; PLAN_G1 §4.2): views F-hubs, F-net and the session read, each a
// stub in G/WorldFest.cpp answering "none" until F-world's bodies (FestRaceKey FW-2a, HeroMarks FW-2b, HeroFestResults FW-3,
// FestWhyLine FW-4).
struct StoryPerson;
/** The hero's marks, one per edition scored in (World's HeroFest: Key, Mark, Rank, Scorers, Points and the unpaid crown Ticket):
 *  T14, the mark moment (U28) and FestStep need the rank, scorers and ticket HeroMark does not carry. */
SHORTSTACKCORE_API const std::vector<HeroFestMark>& HeroMarks(const World& W);
/** The series id of the race FestTop, FestStanding, FestKnown and fest::MarkCutoff read right now ("" when they read none), set from
 *  the race's opening (FestDay), before anyone scores: the hub's line (§2.5.5) and fest::YourLine tell a fresh race with no scorers
 *  from no race, and edition F's race from another's. */
SHORTSTACKCORE_API const std::string& FestRaceKey(const World& W);
/** WHO'S IN's festival line for a person in EventId's field (§2.11's who:fest:* lines, F-world's inventory; at G1 the three
 *  championship reasons only, LEAD_CALLS R-16, R-21); "" when the person has none. */
SHORTSTACKCORE_API std::string FestWhyLine(const World& W, const StoryPerson& Person, const std::string& EventId);
/** One of YOUR <FESTIVAL>'s result rows (§2.8.2): a festival event the hero finished (World's HeroResults, which is private). */
struct HeroFestResult
{
	std::string EventId; // "<template>@<minute>"
	int Place = 0;
	Chips Prize = 0;
	int Entries = 0;     // the instance's field (FindInstance), 0 when it can't be found
};
/** The hero's results in edition Key (its numbered events, headliners and Main, as HeroFestPins counts them), newest first: the YOUR
 *  panel draws up to six. */
SHORTSTACKCORE_API std::vector<HeroFestResult> HeroFestResults(const World& W, const std::string& Key);
} // namespace world

namespace fest
{
/** World's friend for the festival modules (§6.2: defined in G/WorldFest.cpp, holding the module's private reads and writes). */
struct WorldAccess;
/** The ladder's "{N} pts to {MARK}" (§2.5.3, U27): the points of the scorer holding the mark's last rank in the live race right now
 *  (cached per world Rev); 0 when no race is running. */
SHORTSTACKCORE_API double MarkCutoff(const world::World& W, world::FestMark Mark);
/** G1 (the lead's pre-wave commit H0; LEAD_CALLS R-10): the festival views' cache World keeps for them (its private FestViews: the
 *  per-Rev views and FieldStoriesOf's returned reference), defined in G/WorldFest.cpp (F-world's) and reached through WorldAccess.
 *  Never saved; a copied World starts without one. */
struct WorldViews;
} // namespace fest
} // namespace ss
