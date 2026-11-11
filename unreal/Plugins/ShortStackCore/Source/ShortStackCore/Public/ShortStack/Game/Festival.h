#pragma once

#include "ShortStack/Common.h"
#include "ShortStack/Game/SiteData.h"
#include "ShortStack/Structure.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ss
{
namespace life
{
struct Mark;
}
namespace world
{
class World;
}
namespace net
{
class Network;
struct EventInstance;
struct InviteGate;
} // namespace net

/**
 * Update 4, Festival Season (SPEC v1.7 §2.1-§2.4, §2.9.1, §2.10; Phase A's declarations, §6 PA-6): the ten branded festivals across
 * the three sites, as pure functions of code and the clock. Nothing here is saved. Bodies in G/Festival.cpp (F-net's at G1); the
 * rival festivals' templates are built by Network::BuildFestivals (G/NetworkFestival.cpp). Frozen after Phase A (append-only, by
 * patch to the lead).
 *
 * Phase A: every body is a stub that keeps today's behaviour. All() is empty, so nothing has a festival: FestivalOf, LiveOn, Next
 * and PassFestival are null, PhaseOf is Off, Marks is empty, RecordOf is false, SeatsWon 0, IsTitleTemplate false.
 */
namespace fest
{
// The knobs and cross-gate constants (§1.4; each defined here and nowhere else).
constexpr int kClearDays = 10, kAnnounceDays = 30, kBuildupDays = 7, kAfterglowDays = 3;
constexpr int kFirstYear = 2026, kLastYear = 2040;
/** [ASK] 4, answered "the Festival Pass" (2026-11-10): paid online events a rival's Pass needs from a festival's second edition
 *  (its first needs the two rents alone). F-world copies it into Ignite's and RunItTwice's InviteGate::FestivalEvents at G1. */
constexpr int kPassEvents = 20;
/** The title, the podium and FINALIST earn seats into the next edition (§2.6.1; the lead's decision, 2026-11-09). */
constexpr bool kCrownTickets = true;
/** The race's marks (§2.5.3). */
constexpr int kFinalistCap = 25, kFinalistSmall = 10, kFinalistSmallBelow = 250;
constexpr double kFinalistSmallShare = 0.05, kTop10 = 0.10, kTop25 = 0.25;
/** Two mark kinds in a new range (sponsor 20s, stake 30s, video 40s, sites 50s, G/SitesDetail.h): §2.9.1. */
constexpr int MarkFestival = 60; // "{Festival} opens" on FirstDay, 00:00
constexpr int MarkFestMain = 61; // "{Festival} Main Event" at the Main's start

/** The ten brands. Never saved (v1.6 appended Pennant and Reign). */
enum class Brand : uint8_t
{
	None,
	Rcop,
	RingRush,
	Tco,
	Slam,
	Feign,
	Push,
	Writ,
	Grit,
	Pennant,
	Reign,
	Count,
};
/** "rcop" -> Rcop; "" (or any other key) -> None. */
SHORTSTACKCORE_API Brand BrandOf(const std::string& Key);
/** "rcop", "ring", "tco", "slam", "feign", "push", "writ", "grit", "pennant", "reign"; "" for None. */
SHORTSTACKCORE_API const char* BrandKey(Brand B);
/** v1.6: a site's yearly championship (Rcop, Pennant, Reign): three titles, the title order, the storylines, the Director's boost and
 *  the trophy's height read it (§2.6). The one place "a site's yearly championship" is known; no SeriesInfo field. */
SHORTSTACKCORE_API bool IsChampionship(Brand B);
/** v1.6: the template is in its festival's TitleIds (computed from the id through FestivalOf, so nothing is added to Pending and the
 *  save format doesn't change). A title is not always a major: the major floor decides T.Main (§2.6.2). */
SHORTSTACKCORE_API bool IsTitleTemplate(const std::string& TemplateId);

/** One festival edition (§2.1.2). */
struct Festival
{
	net::SiteId Site = net::SiteId::RiverLine;
	Brand Which = Brand::None;
	std::string SeriesId;      // "rcop", "rcop27", "ring27", "tco27", "slam27", "ignite.feign26", "runittwice.grit27", ...
	std::string Name;          // "RCOP 2026", "FEIGN 2026", "PUSH 2027", ...
	int Year = 0, Edition = 0; // Edition = Year - the brand's first year + 1 (FEIGN 2026 is I; RCOP 2026 is 2026's)
	int FirstDay = 0, LastDay = 0, MainDay = 0;
	std::string MainId;                  // the Main's template id ("rcop-main", "ring27-main", "ignite.feign26-main", ...)
	std::vector<std::string> TitleIds;   // v1.6: the edition's title templates, the Main first: {main, mini, micro} for every
	                                     // championship (RCOP's too: "rcop-main", "rcop-mini", "rcop-micro" in 2026), {MainId}
	                                     // otherwise (Summer Slam's included: its Mini is a major but not a title)
	double MainStart = 0.0;              // world minutes
	std::vector<std::string> Satellites; // template ids seating into this edition's Main (§2.2.3, §2.3.4); RCOP's recurring step4
	                                     // and mega-rcop are listed for RCOP editions; a championship's eight in ladder order (v1.6):
	                                     // its Step 1 and 2 seat a step ticket that chains to the Main ticket, the rest the Main ticket
	int Draw = 0;
	bool Flagship = false;
};
/** Every festival, all three sites, in FirstDay order (built once from net::Sites() on first use, cached: ~150 rows). Empty in
 *  Phase A. */
SHORTSTACKCORE_API const std::vector<Festival>& All();
/** Every festival whose window meets [FromDay, ToDay], in FirstDay order. */
SHORTSTACKCORE_API std::vector<const Festival*> Festivals(int FromDay, int ToDay);
/** The festival a series id belongs to (nullptr: not a festival). O(log n). */
SHORTSTACKCORE_API const Festival* FestivalOf(const std::string& SeriesId);
/** The festival whose window holds Day on Site (nullptr: none). Cached per (site, day). */
SHORTSTACKCORE_API const Festival* LiveOn(net::SiteId Site, int Day);
/** The next festival to open after Day (any site). */
SHORTSTACKCORE_API const Festival* Next(int Day);
/** The scheduled end (world minutes) of the edition's last event, from Make's durations (lazy, cached per series id). */
SHORTSTACKCORE_API double LastEnd(const Festival& F);

enum class Phase : uint8_t
{
	Off,
	Announced,
	Buildup,
	Live,
	MainDay,
	Closing,
	Afterglow,
};
/** Pure: the phase at a world minute (§2.1.4). Closing = after LastDay until the world's crown; Afterglow = kAfterglowDays from the
 *  crown minute (W->FestCrownedAt(F), §2.6); with no world, Afterglow runs kAfterglowDays from LastEnd(F). */
SHORTSTACKCORE_API Phase PhaseOf(const Festival& F, double WorldMin, const world::World* W);

/** "Biggest ever", only when true (§2.4.5). */
struct FieldRecord
{
	bool Ever = false, Year = false;
};
/** E against every instance on E's site that started from the site's opening in this world (a rival's launch, W->SiteLaunched, or
 *  its LaunchDay with no world) up to E's start: Ever = E.Entries beats them all; Year = beats every one since Jan 1 of E's year.
 *  Reads Network::DayMax (filled in Prewarm). */
SHORTSTACKCORE_API FieldRecord RecordOf(const net::Network& N, const net::EventInstance& E, const world::World* W);
/** Seats won by satellite into the edition's Main so far (§2.8.3): the sum of max(1, Pool / SeatValue) over the edition's finished
 *  satellites whose SeatTicket is the Main ticket; cached per world minute. */
SHORTSTACKCORE_API int SeatsWon(const Festival& F, double Now);
/** The race's points for a finish (§2.4.3): net::Points for places inside PaidPlaces(Entries, Curve), 0 past it (under Main only
 *  cashes score). Phase A: net::Points (PaidPlaces comes with PA-11; nothing calls this before G1). */
SHORTSTACKCORE_API double RacePoints(int Place, int Entries, Chips BuyInCents, PayoutCurve Curve);
/** The wall calendar's and the agenda's festival marks in [From, To) (§2.9.1): every festival from its Announced phase, account or
 *  not; MarkFestival on FirstDay and MarkFestMain at the Main's start, Where the host site's name. Session::Marks merges them. */
SHORTSTACKCORE_API std::vector<life::Mark> Marks(double From, double To, double Now);

/** The Festival Pass (§2.10): the site's festival in its Announced, Buildup or Live phase at World (nullptr: none, so the path is
 *  closed outside festival weeks). */
SHORTSTACKCORE_API const Festival* PassFestival(net::SiteId Site, double World);
/** The paid online events the Pass needs for F: 0 for the brand's first edition (F.Edition == 1), Gate.FestivalEvents after. */
SHORTSTACKCORE_API int PassNeeds(const Festival& F, const net::InviteGate& Gate);
} // namespace fest
} // namespace ss
