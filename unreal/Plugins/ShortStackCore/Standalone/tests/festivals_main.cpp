// festivals (update 4 SPEC v1.7 §2.15.1, §8.1; the 25th suite): Festival Season's core. The calendar to 2040 (one stage, ten clear
// days, no window over a series), the phases, the six rival festivals' plans, counts and codes, the pins' appends, Added money and
// the Main curve, the race, the crown and its tickets, the roll, the marks, News, the Pass, saves and copy (56 checks at G1), then
// G3's RivalCapsLifted and SlamSatellitesG3 (§4.14). Default run <= 30 s alone (the cap is its TIMEOUT). F-net's file (B-session's at
// G3); F-world's checks are in festivals_world.cpp.
// Phase A (§6 PA-48, the lead): the suite is registered and runs empty.
//
// G1, FN-1 (F-net; PLAN_FNET §4 FN-1): the kit's one-check filter ("festivals_test <Check>" runs that check alone, both files'
// halves of it: Only(), which F-world's SegWorld reads too; a name no check answers to fails the run, so a typo can't pass a planted
// bug), SegNet (F-net's checks, each half and helper in the anonymous namespace below, since festivals_test links both files), and
// the calendar's three checks on RiverLine's four festivals: FestCalendarOneStage, FestCalendarFacts (F-net's half: no world; the
// crown minute, crown + 3 days and the never-crowned edition are F-world's FW-2b) and FestChampTitles (F-net's half). The rival
// festivals' rows join each check as FN-2a, FN-3 and FN-2b build them. fest::All()'s first build is timed and printed (budget 1 ms,
// SPEC §2.14; asserted only under SS_PERF=1, the perf label's run).
//
// G1, FN-2a (F-net; PLAN_FNET's Corrections items 1, 5 and 13): FEIGN and GRIT, the first rival festivals. Their rows join
// FestCalendarOneStage and FestCalendarFacts (which also asserts Site(Ignite).FindSeries("ignite.feign26"), PLAN_G1 item 38, and that
// a rival's "a week out" News skips a festival series, §2.9.2), and seven checks land on their samples: FestTemplatesAppended,
// FestStartsApart, FestRivalCapG1, FestNamesThemed (with the tier and design-word half, PLAN_G1 item 19), FestCodes (Network::NameOf's
// "-E"/"-L"), FestNamesScreened and FestCopy (fest::Inventory's strings by kind, FestChampWord, the glyph rule, and the bodies of
// TrophyName, NameAfterArticle and DateInSentence). Each grows as PENNANT and REIGN (FN-3), PUSH and WRIT (FN-2b) and RiverLine's
// dress (FN-4a, FN-4b) land.
//
// G1, FN-3 (F-net; PLAN_FNET §4 FN-3 and its Corrections): PENNANT and REIGN, the yearly championships. Their rows join every
// check above (the calendar, facts and titles; the appended templates, walked plan by plan in each site's order, FEIGN then PENNANT,
// GRIT then REIGN; the starts and the championship days' instances; the caps; the room words, codes and title forms; the screens and
// the copy), and three checks land with F-net's halves: FestMajorFloor (T.Main only over the Wildfire Main's median pool), FestLadderSeats
// (the step ladder's keys, seats, values and walk to the Main ticket) and FestMainTicket (the year-free Main tickets through Takes).
//
// G1, FN-2b (F-net; PLAN_FNET's Corrections item 1, PLAN_G1 item 39): PUSH and WRIT, the siblings' last two, inserted after FEIGN and
// before GRIT. Their rows join every check above: the calendar (and "ten a year from 2027", one of each brand, in §2.1.3's chain),
// the facts and the draws (PUSH 28, WRIT 14 with HighRoller, both titled), the appended templates walked in the six plans' order
// (FEIGN, PUSH, PENNANT; WRIT, GRIT, REIGN: Ignite 2,198 and RunItTwice 2,487, PENNANT's, GRIT's and REIGN's indices shifted once),
// the starts (WRIT's numbered never at its Main's 18:00), the caps (Standing Room Only 2,875, The Summons 2,963), the room words,
// the codes (SPEC's PUSH and WRIT samples and PIE's "WRIT 03: $2,100 Strong Room PKO"), the screens and the copy.
//
// G1, FN-4a (F-net; PLAN_FNET §4 FN-4 and its Corrections item 1): RiverLine's dress. FestTemplatesAppended walks RiverLine's 84
// appended satellites (8,537 templates; the first 8,453 still 5bc7d44's), two checks land with F-net's halves, FestRcopGtdSum (RCOP
// 2026's computed guarantee in its series, its tagline and the schedule post) and FestBlurbsFilled (no blurb spells a number; every
// figure from the template: the rival festivals' B1-B15 and RiverLine's three W5 blurbs), and fest::Inventory's new RiverLine strings
// reach FestNamesScreened and FestCopy (which also holds X2: "festival" names the tier only, so the five renamed series keep out of
// any series name). RiverLineFestivalPinned, the appended block's pin, is in rival_sites (tests/sites_main.cpp).
//
// G1, FN-4b (F-net; PLAN_FNET §4 FN-4 and its Corrections item 1; FW-1 merged): the copy, complete. FestNamesScreened asserts
// fest::Inventory complete (P1-P12 through TrophyName, C1 and the six plans' 123 room words); FestCopy runs every core inventory
// (fest::Inventory, F-world's fest::TextInventory and the sites' copy table, site::Copy and site::CopyLabels, F-world's 27 festival rows
// found by FW-1's key list) through one set of rules by kind (FestNetCopyWhy: the festival-only rules on festival strings), with
// FestChampWord and §2.11's championship rule by site (no Ignite string holds "championship"; RunItTwice's only as "the club's
// championship"; a shared festival template is judged on its PENNANT fill as Ignite's and on its REIGN fill as RunItTwice's; no
// headline title holds it), also over every series and template on Ignite and RunItTwice; and FestRcopKept lands: RCOP's strings and the six
// tournament names the game already prints, kept and screened. SitesCopy (rival_sites) screens SeriesInfo::Title too.
//
// G1, FN-5 (F-net; PLAN_FNET §4 FN-5 and its Corrections items 4 and 10; R-9): the juice. PayoutFor's Main body landed in
// E/Structure.cpp and fest::RacePoints' cut in G/Festival.cpp, and eight checks land: FestCurveShape (§2.4.3's table, the sweeps on
// the rival Mains' pools, spec-check6's edge setups with N-1's scaled floors, Standard == PayoutTable), FestCurveAgrees (the engine,
// the network and the world to the cent on every rival Main of 2026-2028, the written lift's figures, a Blank world through FEIGN
// I's Main), FestAddedMoneyAgrees (Added end to end, the Mains only, the world's P.Pool), FestGuarantees, FestNoRiverLineJuice,
// FestOverlayIgnoresAdded, FestRacePoints and FestTalkBubbleCurve. SoftBound's line with PENNANT's Main is in rival_sites.
//
// G1, FN-6 (F-net; PLAN_FNET §4 FN-6 and its Corrections item 9; LEAD_CALLS R-14, R-15): tickets and seats. Network::NextFor walks a
// one-off's repeats (G/Network.cpp) and fest::SeatsWon has its body (G/Festival.cpp), and two checks land with FestLadderSeats'
// SeatsWon half: FestNextForRepeats (every run of the championships' steps through NextFor, Step 1's 13:30 run after the 12:00 one
// on day 8, a Step 2 ticket good until day 11's last run; R-15's person half: WorldSim's ExpireTickets, through the forward
// declaration below, credits no person's Step 2 ticket while runs remain and credits a dead one; and no other ticket a satellite
// seats names a repeating event, so no other NextFor answer moves) and FestSeats (SeatsWon over every edition equals the sum over
// its Main-ticket satellites at three moments; R-14's session half: a seat won in Ring Rush 2027's trio, planted as the session
// plants a won seat, registers the hero into its Main through the session's path with the clock at day 165, no world simulated).
//
// G1, FN-7 (F-net; PLAN_FNET §4 FN-7): marks, News and the record field. fest::Marks, Network::News' festival posts and
// fest::RecordOf (with Network's DayMax walk) have their bodies, and three checks land with F-net's halves: FestMarks (every edition
// to 2028 marked FESTIVAL and MAIN from T-30, never before, inside [From, To), all ten brands; the session's wall on Night One and
// on day 26 with no Ignite account), FestNews (the announcements, opening nights, the first-festival and championship lines,
// PENNANT I's sample, RCOP 2026's scripted posts, no festival a week out, and the record posts against RecordOf on every 2026-2027
// rival edition) and FestRecordOf (the planted Big Top and 365-day cases, a brute force over Window on 200 samples, RiverLine
// claiming none, and the walk's budget printed: SPEC §2.14, asserted under SS_PERF=1). session_seams.cpp:724 ("seams: no marks,
// badges or sponsors yet") fails in F-net's tree from here, as SPEC §8.4 predicts (Q8; F-world's patch to the lead, A16).
#include "../../Source/ShortStackCore/Private/ShortStack/Game/SitesDetail.h"
#include "../../Source/ShortStackCore/Private/ShortStack/Game/WorldSim.h"
#include "CopyLint.h"
#include "Trademarks.h"
#include "ShortStack/Common.h"
#include "ShortStack/Game/Festival.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Lobby.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/SiteShow.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/TableTalk.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Structure.h"
#include "ShortStack/Tournament.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace world
{
namespace worldsim_detail
{
// G1, FN-6 (LEAD_CALLS R-15): G/WorldSim.cpp's (F-world's; no header): a person's tickets whose event and seat chain have nothing left
// are credited at their seat value. FestNextForRepeats' person half calls it on a constructed Npc.
void ExpireTickets(Npc& N, double Now);
} // namespace worldsim_detail
} // namespace world
} // namespace ss

namespace festivals_test
{
int Failures = 0;
int Checks = 0;
/** The check named on the command line (nullptr: run every check), and how many checks answered to it. */
const char* FestFilter = nullptr;
int FestFilterHits = 0;

void Expect(bool Condition, const char* What)
{
	++Checks;
	if (!Condition)
	{
		++Failures;
		std::printf("FAILED: %s\n", What);
	}
}

void Expect(bool Condition, const std::string& What)
{
	Expect(Condition, What.c_str());
}

/** The one-check filter (PLAN_FNET Q22): true when Check should run (no filter, or the filter names it). F-world's halves call it
 *  with the same names, so "festivals_test FestCalendarFacts" runs both halves of a shared check. */
bool Only(const char* Check)
{
	const bool Run = !FestFilter || std::strcmp(FestFilter, Check) == 0;
	FestFilterHits += Run && FestFilter ? 1 : 0;
	return Run;
}

void SegNet();   // below (F-net)
void SegWorld(); // festivals_world.cpp (F-world)

namespace
{
const double FestNetDay = ss::net::MinutesPerDay;

/** A world day's first minute. */
double FestNetAt(int Day, int Minute = 0)
{
	return static_cast<double>(Day) * FestNetDay + static_cast<double>(Minute);
}

/** "Oct 18, 2026" for a failure message. */
std::string FestNetDate(int Day)
{
	return ss::net::DateLabel(Day) + ", " + std::to_string(ss::world::YearOf(Day));
}

/** A series id's plan stem: the site's "<key>." and a trailing two-digit year dropped ("rcop27" -> "rcop", "ignite.wildfeb27" ->
 *  "wildfeb", "mm" -> "mm"). */
std::string FestNetStem(const std::string& Id)
{
	std::string S = Id;
	const size_t Dot = S.find('.');
	if (Dot != std::string::npos)
	{
		S = S.substr(Dot + 1);
	}
	const size_t N = S.size();
	if (N > 2 && S[N - 1] >= '0' && S[N - 1] <= '9' && S[N - 2] >= '0' && S[N - 2] <= '9')
	{
		S = S.substr(0, N - 2);
	}
	return S;
}

const char* FestNetPhaseName(ss::fest::Phase P)
{
	switch (P)
	{
	case ss::fest::Phase::Off: return "Off";
	case ss::fest::Phase::Announced: return "Announced";
	case ss::fest::Phase::Buildup: return "Buildup";
	case ss::fest::Phase::Live: return "Live";
	case ss::fest::Phase::MainDay: return "MainDay";
	case ss::fest::Phase::Closing: return "Closing";
	case ss::fest::Phase::Afterglow: return "Afterglow";
	}
	return "?";
}

/** The latest Start + Duration over a series' templates, each through Network::Next from its own day (an independent walk of what
 *  fest::LastEnd reads through Window); Last names that event and Events counts the series' events. */
double FestNetLastEndByTemplates(const ss::net::Network& N, const std::string& SeriesId, std::string* Last = nullptr, int* Events = nullptr)
{
	double End = -1.0;
	int Count = 0;
	for (const ss::net::EventTemplate& T : N.Templates())
	{
		ss::net::EventInstance E;
		if (T.Series != SeriesId || T.OnlyDay == -9999)
		{
			continue;
		}
		// Every run of the template on its day (G1: GRIT's shows run twice a night, the 10 PM run finishing last).
		for (double From = FestNetAt(T.OnlyDay); N.Next(T.Id, From, E) && E.Start < FestNetAt(T.OnlyDay + 1); From = E.Start + 1.0)
		{
			++Count;
			if (E.Start + E.Duration > End && Last)
			{
				*Last = N.NameOf(E) + " (" + std::to_string(E.Entries) + " runners from " + ss::net::TimeLabel(E.Start) + " on day " + std::to_string(T.OnlyDay) + ")";
			}
			End = std::max(End, E.Start + E.Duration);
		}
	}
	if (Events)
	{
		*Events = Count;
	}
	return End;
}

// ------------------------------------------------------------------ FestCalendarOneStage (F-net)

/** §2.1.1 rules 2-3: one stage across the three sites to 2040. FN-1: RiverLine's four (RCOP alone in 2026); the rival rows join as
 *  FN-2a, FN-3 and FN-2b build them ("ten a year from 2027" is FN-2b's: every year 2027-2040 holds the ten brands once each, in
 *  §2.1.3's chain, and 2026 RCOP and FEIGN). Planted: RCOP 2027 opening Oct 14 (inside Micro Madness); PENNANT opening Mar 29 (9
 *  clear days after Ring Rush); REIGN opening Jul 25 (over RiverLine's high-roller list); PUSH moved to Sep 10 (4 clear days after
 *  Summer Slam, FN-2b); WRIT's first edition in 2028 (nine in 2027, FN-2b). */
void FestCalendarOneStageNet()
{
	namespace fest = ss::fest;
	namespace net = ss::net;
	const std::vector<fest::Festival>& All = fest::All();
	bool Sorted = !All.empty();
	bool Years = true;
	for (size_t I = 0; I < All.size(); ++I)
	{
		Sorted = Sorted && (I == 0 || All[I - 1].FirstDay < All[I].FirstDay);
		const int Year = ss::world::YearOf(All[I].FirstDay);
		Years = Years && Year >= fest::kFirstYear && Year <= fest::kLastYear && Year == All[I].Year && All[I].LastDay >= All[I].FirstDay &&
			ss::world::YearOf(All[I].LastDay) == Year;
	}
	Expect(Sorted && Years, "fest::All() holds " + std::to_string(All.size()) + " editions in FirstDay order, each inside one year of 2026-2040");

	int Overlaps = 0;
	int Short = 0;
	int Tightest = 1 << 30;
	std::string TightestPair;
	std::string FirstBad;
	for (size_t I = 0; I < All.size(); ++I)
	{
		for (size_t J = I + 1; J < All.size(); ++J)
		{
			const fest::Festival& A = All[I].FirstDay <= All[J].FirstDay ? All[I] : All[J];
			const fest::Festival& B = All[I].FirstDay <= All[J].FirstDay ? All[J] : All[I];
			const int Clear = B.FirstDay - A.LastDay - 1;
			if (B.FirstDay <= A.LastDay)
			{
				++Overlaps;
			}
			else if (Clear < fest::kClearDays)
			{
				++Short;
			}
			if ((B.FirstDay <= A.LastDay || Clear < fest::kClearDays) && FirstBad.empty())
			{
				FirstBad = "; first: " + A.Name + " -> " + B.Name + ", " + std::to_string(Clear) + " clear days";
			}
			if (J == I + 1 && Clear < Tightest)
			{
				Tightest = Clear;
				TightestPair = A.Name + " -> " + B.Name;
			}
		}
	}
	Expect(Overlaps == 0 && Short == 0,
		"no two festival windows overlap and every gap is at least " + std::to_string(fest::kClearDays) + " clear days (" + std::to_string(Overlaps) + " overlaps, " +
			std::to_string(Short) + " short gaps; tightest " + std::to_string(Tightest) + " clear days, " + TightestPair + FirstBad + ")");

	// No festival window over any series window on any site (§2.1.3: "the rival row moves", never a RiverLine one).
	int OverSeries = 0;
	int Series = 0;
	std::string FirstOver;
	for (int S = 0; S < static_cast<int>(net::SiteId::Count); ++S)
	{
		for (const net::SeriesInfo& Sr : net::Site(static_cast<net::SiteId>(S)).Series())
		{
			if (Sr.Festival)
			{
				continue;
			}
			++Series;
			for (const fest::Festival& F : All)
			{
				if (F.FirstDay <= Sr.LastDay && Sr.FirstDay <= F.LastDay)
				{
					++OverSeries;
					FirstOver = FirstOver.empty() ? "; first: " + F.Name + " (" + FestNetDate(F.FirstDay) + ") over " + Sr.Name + " (" + FestNetDate(Sr.FirstDay) + " - " +
							FestNetDate(Sr.LastDay) + ")" : FirstOver;
				}
			}
		}
	}
	Expect(OverSeries == 0 && Series > 0,
		"no festival window meets a series window on any site (" + std::to_string(Series) + " series, " + std::to_string(OverSeries) + " overlaps" + FirstOver + ")");

	// RiverLine's four every year from 2027, RCOP alone in 2026; no brand twice in a year (any site).
	std::map<int, std::vector<std::string>> RiverLineByYear;
	std::map<std::pair<int, int>, int> BrandYear;
	for (const fest::Festival& F : All)
	{
		if (F.Site == net::SiteId::RiverLine)
		{
			RiverLineByYear[F.Year].push_back(fest::BrandKey(F.Which));
		}
		++BrandYear[{static_cast<int>(F.Which), F.Year}];
	}
	bool Four = RiverLineByYear.size() == static_cast<size_t>(fest::kLastYear - fest::kFirstYear + 1) && RiverLineByYear[2026] == std::vector<std::string>{"rcop"};
	for (int Year = 2027; Year <= fest::kLastYear; ++Year)
	{
		Four = Four && RiverLineByYear[Year] == std::vector<std::string>{"ring", "tco", "slam", "rcop"};
	}
	bool Once = true;
	for (const auto& It : BrandYear)
	{
		Once = Once && It.second == 1 && It.first.first != static_cast<int>(fest::Brand::None);
	}
	Expect(Four && Once, "RiverLine's four every year 2027-2040 (Ring Rush, The Championship Online, Summer Slam, RCOP, in that order) and RCOP alone in 2026; "
						 "every edition has a brand and no brand runs twice in a year");

	// FN-2a, FN-3, FN-2b: the rival festivals, each on its host every year from its first edition to 2040 (FEIGN on Ignite from 2026,
	// GRIT on RunItTwice from 2027; FN-3: PENNANT on Ignite and REIGN on RunItTwice from 2027, whose windows have zero slack against the
	// series beside them; FN-2b: PUSH on Ignite and WRIT on RunItTwice from 2027).
	struct Rival
	{
		fest::Brand Which;
		net::SiteId Site;
		int FirstYear;
	};
	const Rival Rivals[] = {{fest::Brand::Feign, net::SiteId::Ignite, 2026}, {fest::Brand::Grit, net::SiteId::RunItTwice, 2027}, {fest::Brand::Pennant, net::SiteId::Ignite, 2027},
		{fest::Brand::Reign, net::SiteId::RunItTwice, 2027}, {fest::Brand::Push, net::SiteId::Ignite, 2027}, {fest::Brand::Writ, net::SiteId::RunItTwice, 2027}};
	std::string RivalBad;
	for (const Rival& R : Rivals)
	{
		std::vector<int> RivalYears;
		bool OnSite = true;
		for (const fest::Festival& F : All)
		{
			if (F.Which == R.Which)
			{
				RivalYears.push_back(F.Year);
				OnSite = OnSite && F.Site == R.Site;
			}
		}
		bool Every = RivalYears.size() == static_cast<size_t>(fest::kLastYear - R.FirstYear + 1);
		for (size_t I = 0; I < RivalYears.size(); ++I)
		{
			Every = Every && RivalYears[I] == R.FirstYear + static_cast<int>(I);
		}
		if (!(Every && OnSite) && RivalBad.empty())
		{
			RivalBad = std::string(": ") + fest::BrandKey(R.Which) + " has " + std::to_string(RivalYears.size()) + " editions" + (OnSite ? "" : ", not all on its host");
		}
	}
	Expect(RivalBad.empty(), "FEIGN on Ignite every year 2026-2040, GRIT on RunItTwice, PENNANT on Ignite, REIGN on RunItTwice, PUSH on Ignite and WRIT on RunItTwice every "
							 "year 2027-2040, one edition a year" + RivalBad);

	// FN-2b, "ten a year from 2027" (SPEC.md §2.15.1, PLAN_G1 item 39): every year 2027-2040 holds exactly the ten brands, once each, in
	// §2.1.3's chain (WRIT, Ring Rush, PENNANT, GRIT, The Championship Online, REIGN, Summer Slam, PUSH, RCOP, FEIGN), and 2026 holds
	// RCOP and FEIGN only.
	const std::vector<std::string> Chain = {"writ", "ring", "pennant", "grit", "tco", "reign", "slam", "push", "rcop", "feign"};
	std::map<int, std::vector<std::string>> ByYear;
	for (const fest::Festival& F : All)
	{
		ByYear[F.Year].push_back(fest::BrandKey(F.Which));
	}
	std::string TenBad = ByYear[2026] == std::vector<std::string>{"rcop", "feign"} ? std::string() : "; 2026 holds " + std::to_string(ByYear[2026].size());
	for (int Year = 2027; Year <= fest::kLastYear; ++Year)
	{
		if (ByYear[Year] != Chain && TenBad.empty())
		{
			TenBad = "; " + std::to_string(Year) + " holds " + std::to_string(ByYear[Year].size()) + " editions" + (ByYear[Year].size() == Chain.size() ? " out of the chain's order" : "");
		}
	}
	Expect(TenBad.empty() && ByYear.size() == static_cast<size_t>(fest::kLastYear - fest::kFirstYear + 1),
		"ten a year from 2027: every year 2027-2040 holds the ten brands once each in the chain's order (WRIT, Ring Rush, PENNANT, GRIT, The Championship Online, REIGN, Summer "
		"Slam, PUSH, RCOP, FEIGN), 2026 RCOP and FEIGN" + TenBad);
}

// ------------------------------------------------------------------ FestCalendarFacts (F-net's half: no world)

struct FestNetFacts
{
	ss::fest::Brand Which;
	ss::net::SiteId Site;
	int Month, Day, Days; // the window (§2.1.3)
	int FirstYear;        // the first edition
	int MainNo;           // the Main's day of the window (1 = FirstDay)
	int MainMinute;       // its start
	const char* Name;     // the name before the year
	const char* Stem;     // the series id's stem (and the brand key)
	int Draw;
	bool Flagship;
};

/** §2.1.3's rows: RiverLine's four (FN-1), FEIGN and GRIT (FN-2a), PENNANT and REIGN (FN-3), PUSH and WRIT (FN-2b). */
const FestNetFacts FestNetRiverLine[] = {
	{ss::fest::Brand::RingRush, ss::net::SiteId::RiverLine, 3, 6, 14, 2027, 14, 16 * 60, "Ring Rush", "ring", 40, false},
	{ss::fest::Brand::Tco, ss::net::SiteId::RiverLine, 6, 4, 19, 2027, 19, 18 * 60, "The Championship Online", "tco", 70, false},
	{ss::fest::Brand::Slam, ss::net::SiteId::RiverLine, 8, 20, 17, 2027, 17, 15 * 60, "Summer Slam", "slam", 80, true},
	{ss::fest::Brand::Rcop, ss::net::SiteId::RiverLine, 10, 18, 22, 2026, 22, 15 * 60, "RCOP", "rcop", 110, true},
	{ss::fest::Brand::Grit, ss::net::SiteId::RunItTwice, 5, 12, 12, 2027, 12, 18 * 60, "GRIT", "grit", 32, false},
	{ss::fest::Brand::Feign, ss::net::SiteId::Ignite, 11, 30, 10, 2026, 10, 18 * 60, "FEIGN", "feign", 32, false},
	{ss::fest::Brand::Pennant, ss::net::SiteId::Ignite, 3, 30, 13, 2027, 13, 18 * 60, "PENNANT", "pennant", 50, false},
	{ss::fest::Brand::Reign, ss::net::SiteId::RunItTwice, 7, 26, 13, 2027, 13, 18 * 60, "REIGN", "reign", 45, false},
	{ss::fest::Brand::Push, ss::net::SiteId::Ignite, 9, 16, 10, 2027, 10, 18 * 60, "PUSH", "push", 28, false},
	{ss::fest::Brand::Writ, ss::net::SiteId::RunItTwice, 1, 22, 10, 2027, 10, 18 * 60, "WRIT", "writ", 14, false},
};

/** A championship edition's satellites in ladder order (§2.1.2, §2.3.2): Step 1's two templates, Step 2's, Step 3's, the Mega and
 *  the Last Chance. */
std::vector<std::string> FestNetLadder(const std::string& Id)
{
	std::vector<std::string> Out;
	for (const char* Stem : {"-s1a", "-s1b", "-s2a", "-s2b", "-s3a", "-s3b", "-sat1", "-sat2"})
	{
		Out.push_back(Id + Stem);
	}
	return Out;
}

/** One edition's phases with no world, minute by minute at each boundary (§2.1.4), and PassFestival beside them (§2.10 item 1). */
void FestNetPhases(const ss::fest::Festival& F, std::string& Bad)
{
	namespace fest = ss::fest;
	const double End = fest::LastEnd(F);
	const double Last = FestNetAt(F.LastDay + 1);
	struct At
	{
		double Min;
		fest::Phase Want;
		const char* Where;
	};
	std::vector<At> Steps = {
		{FestNetAt(F.FirstDay - fest::kAnnounceDays) - 1.0, fest::Phase::Off, "T-30 less a minute"},
		{FestNetAt(F.FirstDay - fest::kAnnounceDays), fest::Phase::Announced, "T-30 00:00"},
		{FestNetAt(F.FirstDay - fest::kBuildupDays) - 1.0, fest::Phase::Announced, "T-7 less a minute"},
		{FestNetAt(F.FirstDay - fest::kBuildupDays), fest::Phase::Buildup, "T-7 00:00"},
		{FestNetAt(F.FirstDay) - 1.0, fest::Phase::Buildup, "FirstDay less a minute"},
		{FestNetAt(F.FirstDay), F.MainDay == F.FirstDay ? fest::Phase::MainDay : fest::Phase::Live, "FirstDay 00:00"},
		{FestNetAt(F.MainDay) - 1.0, F.MainDay == F.FirstDay ? fest::Phase::Buildup : fest::Phase::Live, "MainDay less a minute"},
		{FestNetAt(F.MainDay), fest::Phase::MainDay, "MainDay 00:00"},
		{F.MainStart, fest::Phase::MainDay, "the Main's start"},
		{Last - 1.0, F.MainDay == F.LastDay ? fest::Phase::MainDay : fest::Phase::Live, "LastDay 23:59"},
	};
	if (End > Last)
	{
		Steps.push_back({Last, fest::Phase::Closing, "LastDay 24:00"});
		Steps.push_back({End - 1.0, fest::Phase::Closing, "LastEnd less a minute"});
	}
	Steps.push_back({std::max(End, Last), fest::Phase::Afterglow, "LastEnd"});
	Steps.push_back({End + fest::kAfterglowDays * FestNetDay - 1.0, fest::Phase::Afterglow, "LastEnd + 3 days less a minute"});
	Steps.push_back({End + fest::kAfterglowDays * FestNetDay, fest::Phase::Off, "LastEnd + 3 days"});
	for (const At& S : Steps)
	{
		const fest::Phase Got = fest::PhaseOf(F, S.Min, nullptr);
		// The Pass's festival: the site's edition in Announced, Buildup or Live (MainDay is Live's), never Closing or the Afterglow.
		const bool PassOpen = S.Want == fest::Phase::Announced || S.Want == fest::Phase::Buildup || S.Want == fest::Phase::Live || S.Want == fest::Phase::MainDay;
		const fest::Festival* Pass = fest::PassFestival(F.Site, S.Min);
		if (Got != S.Want || (PassOpen ? Pass != &F : Pass == &F))
		{
			Bad += Bad.empty() ? "" : "; ";
			Bad += F.Name + " at " + S.Where + ": " + FestNetPhaseName(Got) + " (want " + FestNetPhaseName(S.Want) + "), PassFestival " +
				(Pass ? Pass->Name : std::string("none"));
		}
	}
}

/** §2.1.2-§2.1.4, §2.10 item 1 with no world. Planted: Buildup starting at T-6; PassFestival answering in Closing; RCOP's Flagship
 *  false; Micro Madness 2026 left at Draw 0; Top Floor Series without HighRoller; PENNANT's first edition at 2026 (FN-3); WRIT without
 *  HighRoller (FN-2b). */
void FestCalendarFactsNet()
{
	namespace fest = ss::fest;
	namespace net = ss::net;
	const std::vector<fest::Festival>& All = fest::All();

	// Each brand's site, window, Main day and start, first edition and edition numbers, ids, names and flags (a rival's ids are its
	// site's key, the brand and the year: "ignite.feign26"; its satellites the edition's Mega and Last Chance, WRIT's The Summons; Ring
	// Rush's and the Championship Online's the trio FN-4a appends).
	for (const FestNetFacts& B : FestNetRiverLine)
	{
		std::vector<const fest::Festival*> Rows;
		for (const fest::Festival& F : All)
		{
			if (F.Which == B.Which)
			{
				Rows.push_back(&F);
			}
		}
		std::string Bad;
		for (size_t I = 0; I < Rows.size(); ++I)
		{
			const fest::Festival& F = *Rows[I];
			const int Year = B.FirstYear + static_cast<int>(I);
			const std::string Yy = std::to_string(Year % 100);
			const std::string Key = B.Site == net::SiteId::RiverLine ? std::string() : std::string(net::KeyOf(B.Site)) + ".";
			const std::string Id = B.Which == fest::Brand::Rcop && Year == 2026 ? std::string("rcop") : Key + B.Stem + (Yy.size() < 2 ? "0" : "") + Yy;
			const int First = ss::world::DayOn(Year, B.Month, B.Day);
			const int MainDay = First + B.MainNo - 1;
			const std::vector<std::string> Seats = B.Which == fest::Brand::Rcop ? std::vector<std::string>{"step4", "mega-rcop"}
				: fest::IsChampionship(B.Which) ? FestNetLadder(Id)
				: B.Which == fest::Brand::Writ ? std::vector<std::string>{Id + "-sat1"}
				: B.Site != net::SiteId::RiverLine ? std::vector<std::string>{Id + "-sat1", Id + "-sat2"}
				: B.Which == fest::Brand::RingRush || B.Which == fest::Brand::Tco ? std::vector<std::string>{Id + "-step", Id + "-mega", Id + "-last"} // FN-4a's trio
												   : std::vector<std::string>{};
			const bool Ok = F.Site == B.Site && F.Year == Year && F.Edition == Year - B.FirstYear + 1 && F.FirstDay == First && F.LastDay == First + B.Days - 1 &&
				F.MainDay == MainDay && F.MainStart == FestNetAt(MainDay, B.MainMinute) && F.SeriesId == Id && F.Name == std::string(B.Name) + " " + std::to_string(Year) &&
				F.MainId == Id + "-main" && F.Draw == B.Draw && F.Flagship == B.Flagship && F.Satellites == Seats && std::string(fest::BrandKey(F.Which)) == B.Stem;
			if (!Ok && Bad.empty())
			{
				Bad = ": first wrong " + F.SeriesId + " (" + F.Name + ", edition " + std::to_string(F.Edition) + ", " + FestNetDate(F.FirstDay) + " - " + FestNetDate(F.LastDay) +
					", Main day " + std::to_string(F.MainDay) + " at " + net::TimeLabel(F.MainStart) + ", Draw " + std::to_string(F.Draw) + (F.Flagship ? ", flagship" : "") +
					", " + std::to_string(F.Satellites.size()) + " satellites)";
			}
		}
		const bool Count = Rows.size() == static_cast<size_t>(fest::kLastYear - B.FirstYear + 1);
		Expect(Count && Bad.empty(),
			std::string(B.Name) + ": " + std::to_string(Rows.size()) + " editions " + std::to_string(B.FirstYear) + "-2040 on " + net::Def(B.Site).Name + ", " + std::to_string(B.Days) + " days from " +
				net::DateLabel(ss::world::DayOn(B.FirstYear, B.Month, B.Day)) + ", the Main on day " + std::to_string(B.MainNo) + " at " + net::TimeLabel(B.MainMinute) +
				", edition I in " + std::to_string(B.FirstYear) + ", Draw " + std::to_string(B.Draw) + (B.Flagship ? ", flagship" : "") + Bad);
	}
	const fest::Festival* Rcop = fest::FestivalOf("rcop");
	Expect(Rcop && Rcop->Name == "RCOP 2026" && Rcop->Edition == 1 && Rcop->FirstDay == 13 && Rcop->LastDay == 34 && Rcop->MainDay == 34 &&
			   Rcop->MainStart == FestNetAt(34, 15 * 60) && Rcop->MainId == "rcop-main" && Rcop->Draw == 110 && Rcop->Flagship,
		"RCOP 2026: days 13-34 (Oct 18 - Nov 8), its Main rcop-main on day 34 at 3:00 PM, edition I, Draw 110, flagship");
	// FN-2a: the first rival editions, and Network::FindSeries on a festival id (PLAN_G1 item 38: Phase A's SeriesIndex, filled in
	// Index() after BuildFestivals).
	const fest::Festival* Feign = fest::FestivalOf("ignite.feign26");
	const fest::Festival* Grit = fest::FestivalOf("runittwice.grit27");
	const net::SeriesInfo* FeignSr = net::Site(net::SiteId::Ignite).FindSeries("ignite.feign26");
	const net::SeriesInfo* GritSr = net::Site(net::SiteId::RunItTwice).FindSeries("runittwice.grit27");
	Expect(Feign && Feign->Site == net::SiteId::Ignite && Feign->Name == "FEIGN 2026" && Feign->Edition == 1 && Feign->FirstDay == 56 && Feign->LastDay == 65 &&
			   Feign->MainStart == FestNetAt(65, 18 * 60) && Feign->MainId == "ignite.feign26-main" && Feign->TitleIds == std::vector<std::string>{"ignite.feign26-main"} && FeignSr &&
			   FeignSr->Id == "ignite.feign26" && FeignSr->Festival && FeignSr->Brand == "feign" && FeignSr->Events == 27 && FeignSr->MainEvent == Feign->MainId &&
			   !net::Site(net::SiteId::RunItTwice).FindSeries("ignite.feign26") && !net::Shared().FindSeries("ignite.feign26"),
		"FEIGN 2026: Ignite, days 56-65 (Nov 30 - Dec 9), its Main ignite.feign26-main on day 65 at 6:00 PM, edition I, its one title the Main; "
		"Site(Ignite).FindSeries(\"ignite.feign26\") finds the series (27 events), no other site's does");
	Expect(Grit && Grit->Site == net::SiteId::RunItTwice && Grit->Name == "GRIT 2027" && Grit->Edition == 1 && Grit->FirstDay == ss::world::DayOn(2027, 5, 12) &&
			   Grit->LastDay == ss::world::DayOn(2027, 5, 23) && Grit->MainStart == FestNetAt(Grit->LastDay, 18 * 60) && GritSr && GritSr->Events == 27 &&
			   GritSr->Title == "The Grinders' RunItTwice" && !fest::FestivalOf("runittwice.grit26"),
		"GRIT 2027: RunItTwice, May 12-23, its Main on night 12 at 6:00 PM, edition I (no 2026 edition), 27 events (24 shows and 3 headliners), "
		"titled The Grinders' RunItTwice");
	// FN-2b: the siblings' last two (§2.1.3): PUSH 2027 on Ignite (Sep 16-25, 21 events, "Pier's Upbeat Summer Hold'em") and WRIT 2027 on
	// RunItTwice (Jan 22-31, 13 events: twelve numbered and the Main; "The Winter RunItTwice"; its one satellite The Summons), each
	// edition I, no 2026 edition.
	const fest::Festival* Push = fest::FestivalOf("ignite.push27");
	const fest::Festival* Writ = fest::FestivalOf("runittwice.writ27");
	const net::SeriesInfo* PushSr = net::Site(net::SiteId::Ignite).FindSeries("ignite.push27");
	const net::SeriesInfo* WritSr = net::Site(net::SiteId::RunItTwice).FindSeries("runittwice.writ27");
	Expect(Push && Push->Site == net::SiteId::Ignite && Push->Name == "PUSH 2027" && Push->Edition == 1 && Push->FirstDay == ss::world::DayOn(2027, 9, 16) &&
			   Push->LastDay == ss::world::DayOn(2027, 9, 25) && Push->MainStart == FestNetAt(Push->LastDay, 18 * 60) && Push->MainId == "ignite.push27-main" && PushSr &&
			   PushSr->Events == 21 && PushSr->Title == "Pier's Upbeat Summer Hold'em" && !PushSr->HighRoller && !fest::FestivalOf("ignite.push26") && Writ &&
			   Writ->Site == net::SiteId::RunItTwice && Writ->Name == "WRIT 2027" && Writ->Edition == 1 && Writ->FirstDay == ss::world::DayOn(2027, 1, 22) && Writ->FirstDay == 109 &&
			   Writ->LastDay == ss::world::DayOn(2027, 1, 31) && Writ->MainStart == FestNetAt(Writ->LastDay, 18 * 60) && WritSr && WritSr->Events == 13 &&
			   WritSr->Title == "The Winter RunItTwice" && WritSr->HighRoller && Writ->Satellites == std::vector<std::string>{"runittwice.writ27-sat1"} &&
			   !fest::FestivalOf("runittwice.writ26"),
		"PUSH 2027: Ignite, Sep 16-25, its Main ignite.push27-main on day 10 at 6:00 PM, 21 events, titled Pier's Upbeat Summer Hold'em; WRIT 2027: RunItTwice, Jan 22-31 "
		"(days 109-118), its Main on day 10 at 6:00 PM, 13 events, titled The Winter RunItTwice, a high-roller series, its one satellite The Summons; each edition I (no 2026 "
		"edition)");
	// FN-3: the championships (§2.1.3's v1.6 rows): the Main on day 13 at 18:00, the Micro Main on day 11 and the Mini on day 12, both
	// 18:00; edition I in 2027 (2026's windows fell before Night One); no full title.
	const net::Network& IgNet = net::Site(net::SiteId::Ignite);
	const net::Network& RtNet = net::Site(net::SiteId::RunItTwice);
	std::string ChampBad;
	struct ChampFacts
	{
		const net::Network* N;
		const char* Id;
		const char* Name;
		int Month, Day, Events;
	};
	for (const ChampFacts& C : {ChampFacts{&IgNet, "ignite.pennant27", "PENNANT 2027", 3, 30, 39}, ChampFacts{&RtNet, "runittwice.reign27", "REIGN 2027", 7, 26, 30}})
	{
		const fest::Festival* F = fest::FestivalOf(C.Id);
		const net::SeriesInfo* Sr = C.N->FindSeries(C.Id);
		const net::EventTemplate* Micro = C.N->FindTemplate(std::string(C.Id) + "-micro");
		const net::EventTemplate* Mini = C.N->FindTemplate(std::string(C.Id) + "-mini");
		const int First = ss::world::DayOn(2027, C.Month, C.Day);
		const bool Ok = F && F->Site == C.N->Site() && F->Name == C.Name && F->Edition == 1 && F->FirstDay == First && F->LastDay == First + 12 && F->MainDay == F->LastDay &&
			F->MainStart == FestNetAt(F->LastDay, 18 * 60) && Sr && Sr->Events == C.Events && Sr->Title.empty() && Micro && Micro->OnlyDay == First + 10 &&
			Micro->StartMinute == 18 * 60 && Mini && Mini->OnlyDay == First + 11 && Mini->StartMinute == 18 * 60;
		ChampBad += Ok ? "" : std::string(" ") + C.Id;
	}
	Expect(ChampBad.empty() && !fest::FestivalOf("ignite.pennant26") && !fest::FestivalOf("runittwice.reign26") && !IgNet.FindSeries("ignite.pennant26"),
		"PENNANT 2027: Ignite, Mar 30 - Apr 11, 39 events; REIGN 2027: RunItTwice, Jul 26 - Aug 7, 30 events; each edition I (no 2026 edition), the Micro Main on day "
		"11, the Mini on day 12 and the Main on day 13, all at 6:00 PM, no full title" + ChampBad);

	// FestivalOf, LiveOn, Next and Festivals round-trip on every edition; a series is not a festival.
	std::string Trip;
	for (const fest::Festival& F : All)
	{
		const net::SeriesInfo* Sr = net::Site(F.Site).FindSeries(F.SeriesId);
		const std::vector<const fest::Festival*> In = fest::Festivals(F.FirstDay, F.LastDay);
		const bool Ok = fest::FestivalOf(F.SeriesId) == &F && fest::LiveOn(F.Site, F.FirstDay) == &F && fest::LiveOn(F.Site, F.MainDay) == &F &&
			fest::LiveOn(F.Site, F.LastDay) == &F && fest::LiveOn(F.Site, F.FirstDay - 1) != &F && fest::LiveOn(F.Site, F.LastDay + 1) != &F && fest::Next(F.FirstDay - 1) == &F &&
			In.size() == 1 && In[0] == &F && Sr && Sr->Festival && Sr->Brand == fest::BrandKey(F.Which) && Sr->FirstDay == F.FirstDay && Sr->MainEvent == F.MainId;
		if (!Ok && Trip.empty())
		{
			Trip = ": first wrong " + F.SeriesId;
		}
	}
	const bool Plain = !fest::FestivalOf("mm") && !fest::FestivalOf("slam") && !fest::FestivalOf("mm27") && !fest::FestivalOf("win27") && !fest::FestivalOf("hrs27") &&
		!fest::FestivalOf("hol26") && !fest::FestivalOf("ignite.wildfeb27") && !fest::FestivalOf("runittwice.open26") && !fest::FestivalOf("") && !fest::FestivalOf("rcop-main");
	const bool Edges = !All.empty() && !fest::Next(All.back().FirstDay) && fest::Festivals(-400, 12).empty() && !fest::LiveOn(net::SiteId::RiverLine, 12) &&
		!fest::LiveOn(net::SiteId::RiverLine, 35) && !fest::LiveOn(net::SiteId::Count, 20) && fest::Next(0) == Rcop;
	Expect(Trip.empty() && Plain && Edges,
		"FestivalOf, LiveOn, Next and Festivals round-trip on all " + std::to_string(All.size()) + " editions (each its series' flags); Micro Madness, Summer Slam 2026 "
		"(history), the seasonal series and the rivals' plain series are not festivals; nothing before RCOP 2026" + Trip);

	// LastEnd (§2.6.1): the scheduled end of the edition's last event, from Make's durations. RCOP 2026's falls in the early hours of
	// day 35 (LastDay + 1), at or after its Main's finish (6,052 runners from 15:00: 669 minutes, to 02:09).
	const net::Network& River = net::Shared();
	net::EventInstance Main;
	const bool HaveMain = River.Next("rcop-main", FestNetAt(34), Main);
	const double RcopEnd = Rcop ? fest::LastEnd(*Rcop) : -1.0;
	std::string LastEvent;
	int RcopEvents = 0;
	const double ByTemplates = FestNetLastEndByTemplates(River, "rcop", &LastEvent, &RcopEvents);
	const double MainEnd = HaveMain ? Main.Start + Main.Duration : -1.0;
	std::printf("festivals: LastEnd(RCOP 2026) = day %d %s, %s; its Main (%d runners from %s) ends day %d %s\n", net::DayOf(RcopEnd), net::TimeLabel(RcopEnd).c_str(),
		LastEvent.c_str(), HaveMain ? Main.Entries : 0, HaveMain ? net::TimeLabel(Main.Start).c_str() : "-", net::DayOf(MainEnd), net::TimeLabel(MainEnd).c_str());
	Expect(HaveMain && RcopEvents == 154 && RcopEnd == ByTemplates && RcopEnd >= MainEnd && MainEnd > FestNetAt(35) && RcopEnd < FestNetAt(35, 6 * 60),
		"LastEnd(RCOP 2026) is the latest scheduled finish of its " + std::to_string(RcopEvents) + " events (" + net::TimeLabel(RcopEnd) + " on day " +
			std::to_string(net::DayOf(RcopEnd)) + ": " + LastEvent + "), at or after its Main's (" + net::TimeLabel(MainEnd) + "), in the early hours of day 35");
	bool Ends = true;
	for (const char* Id : {"ring27", "tco27", "slam27", "rcop27", "ring40", "rcop40", "ignite.feign26", "runittwice.grit27", "ignite.feign40", "ignite.pennant27",
			 "runittwice.reign27", "ignite.pennant40", "runittwice.reign40", "ignite.push27", "runittwice.writ27", "ignite.push40", "runittwice.writ40"})
	{
		const fest::Festival* F = fest::FestivalOf(Id);
		Ends = Ends && F && fest::LastEnd(*F) == FestNetLastEndByTemplates(net::Site(F->Site), Id) && fest::LastEnd(*F) >= F->MainStart;
	}
	Expect(Ends, "LastEnd agrees with each event's own scheduled finish (every run) on Ring Rush, The Championship Online, Summer Slam and RCOP 2027 and on 2040's, "
				 "FEIGN 2026 and 2040, GRIT 2027, PENNANT, REIGN, PUSH and WRIT 2027 and 2040");

	// PhaseOf with no world at every boundary, and PassFestival only in Announced, Buildup and Live.
	std::string Phases;
	for (const char* Id : {"rcop", "ring27", "tco27", "slam27", "rcop27", "slam40", "ignite.feign26", "runittwice.grit27", "ignite.feign40", "ignite.pennant27",
			 "runittwice.reign27", "ignite.push27", "runittwice.writ27"})
	{
		if (const fest::Festival* F = fest::FestivalOf(Id))
		{
			FestNetPhases(*F, Phases);
		}
		else
		{
			Phases += std::string(Phases.empty() ? "" : "; ") + Id + " missing";
		}
	}
	Expect(Phases.empty(), "PhaseOf (no world) at T-30 00:00, T-7, FirstDay 00:00, MainDay, LastDay 23:59, LastEnd and LastEnd + 3 days; PassFestival names the edition only "
						   "while it is Announced, in Buildup or Live" + (Phases.empty() ? std::string() : ": " + Phases));

	// Draw, Flagship, HighRoller, Festival and Brand on every series of every site (§3.2.1, §12.1 I-3; no Title on these).
	struct DrawRow
	{
		const char* Stem;
		int Draw;
		bool HighRoller;
		bool Festival;
		bool Flagship;
		const char* Title = ""; // SeriesInfo::Title (§2.11 N17-N19): GRIT's (FN-2a), PUSH's and WRIT's (FN-2b); "" on every other
	};
	const DrawRow Draws[] = {
		{"rcop", 110, false, true, true}, {"slam", 80, false, true, true}, {"tco", 70, false, true, false}, {"ring", 40, false, true, false},
		{"feign", 32, false, true, false}, {"grit", 32, false, true, false, "The Grinders' RunItTwice"}, {"pennant", 50, false, true, false}, {"reign", 45, false, true, false},
		{"push", 28, false, true, false, "Pier's Upbeat Summer Hold'em"}, {"writ", 14, true, true, false, "The Winter RunItTwice"},
		{"mm", 22, false, false, false}, {"win", 12, false, false, false}, {"spr", 12, false, false, false}, {"hol", 12, false, false, false},
		{"hrs", 4, true, false, false}, {"openspring", 10, false, false, false}, {"open", 10, false, false, false}, {"wildfeb", 8, false, false, false},
		{"wildaug", 8, false, false, false}, {"ironworks", 6, false, false, false},
	};
	int Seen = 0;
	std::string Wrong;
	for (int S = 0; S < static_cast<int>(net::SiteId::Count); ++S)
	{
		for (const net::SeriesInfo& Sr : net::Site(static_cast<net::SiteId>(S)).Series())
		{
			++Seen;
			const std::string Stem = FestNetStem(Sr.Id);
			const DrawRow* Row = nullptr;
			for (const DrawRow& D : Draws)
			{
				Row = Stem == D.Stem ? &D : Row;
			}
			// Summer Slam 2026 ("slam", day -30) is history: Summer Slam's Draw, but a series (§2.1.3: first edition 2027).
			const bool History = Sr.Id == "slam";
			const bool Ok = Row && Sr.Draw == Row->Draw && Sr.HighRoller == Row->HighRoller && Sr.Festival == (Row->Festival && !History) &&
				Sr.Flagship == (Row->Flagship && !History) && Sr.Brand == (Sr.Festival ? Stem : std::string()) && Sr.Title == Row->Title;
			if (!Ok && Wrong.empty())
			{
				Wrong = ": first wrong " + Sr.Id + " (Draw " + std::to_string(Sr.Draw) + (Sr.HighRoller ? ", high roller" : "") + (Sr.Festival ? ", festival " + Sr.Brand : std::string()) +
					(Sr.Flagship ? ", flagship" : "") + ")";
			}
		}
	}
	Expect(Seen > 0 && Wrong.empty(),
		"every series on every site (" + std::to_string(Seen) + ") carries the SPEC's 3.2.1 Draw (RCOP 110, Summer Slam 80, The Championship Online 70, PENNANT 50, REIGN 45, "
		"Ring Rush 40, FEIGN 32, GRIT 32, PUSH 28, Micro Madness 22, WRIT 14 with HighRoller, the seasonal 12, the Open 10, Wildfire 8, Ironworks 6, Top Floor 4 with "
		"HighRoller); Festival and Brand on the festivals, Flagship on RCOP and Summer Slam (none of the six rivals: L2), GRIT's, PUSH's and WRIT's Titles" + Wrong);

	// §2.9.2 (FN-2a): a rival's "series a week out" post skips a festival series (it is announced at T-30, FN-7's posts), while a
	// plain series keeps it: Ignite's News a day after FEIGN 2026's T-7 holds no FEIGN post, and a day after Wildfire Winter 2027's
	// T-7 holds its week-out post. FN-7: the week-out post's own form ("FEIGN 2026 starts Nov 30", tagged with the series' Short);
	// FEIGN's T-30 announcement ("FEIGN is coming to Ignite", tagged FESTIVAL) is FestNews'.
	const net::Network& Ig = net::Site(net::SiteId::Ignite);
	net::HeroStats NoHero;
	int FeignPosts = 0;
	for (const net::NewsItem& It : Ig.News(FestNetAt(56 - 7 + 1, 12 * 60), NoHero, 60))
	{
		FeignPosts += It.Kind == net::NewsKind::Series && (It.Tag == "FEIGN" || It.Title.rfind("FEIGN 2026 starts", 0) == 0) ? 1 : 0;
	}
	const net::SeriesInfo* Wild = Ig.FindSeries("ignite.wildfeb27");
	int WildPosts = 0;
	if (Wild)
	{
		for (const net::NewsItem& It : Ig.News(FestNetAt(Wild->FirstDay - 7 + 1, 12 * 60), NoHero, 60))
		{
			WildPosts += It.Kind == net::NewsKind::Series && It.Tag == Wild->Short ? 1 : 0;
		}
	}
	Expect(FeignPosts == 0 && WildPosts == 1,
		"a rival's week-out News skips a festival (" + std::to_string(FeignPosts) + " FEIGN 2026 posts on day 50) and keeps a plain series' (" + std::to_string(WildPosts) +
			" for Wildfire Winter 2027)");
}

// ------------------------------------------------------------------ FestChampTitles (F-net's half)

/** §2.6.2: a championship edition's titles are its Main, Mini and Micro Main (RCOP's too: rcop-main, rcop-mini, rcop-micro in 2026),
 *  every other edition's its Main alone (Summer Slam's though its Mini is a major); IsTitleTemplate agrees. FN-1: RiverLine's rows;
 *  FN-3: PENNANT's and REIGN's 28 editions (a rival's Mini and Micro are titles as its plan flags them Title; its other headliners,
 *  steps and satellites and the year-free Main ticket are not). Planted: RCOP left at {MainId}; the Mini Main row without Title
 *  (FN-3, REIGN's and PENNANT's). */
void FestChampTitlesNet()
{
	namespace fest = ss::fest;
	namespace net = ss::net;
	size_t Titles = 0;
	int Championships = 0;
	std::string Bad;
	bool Agree = true;
	bool Majors = true;
	for (const fest::Festival& F : fest::All())
	{
		const net::Network& N = net::Site(F.Site);
		std::vector<std::string> Want = {F.MainId};
		if (fest::IsChampionship(F.Which))
		{
			++Championships;
			const std::string Stem = F.MainId.substr(0, F.MainId.size() - 4);
			Want.push_back(Stem + "mini");
			Want.push_back(Stem + "micro");
		}
		bool Exist = true;
		for (const std::string& Id : F.TitleIds)
		{
			const net::EventTemplate* T = N.FindTemplate(Id);
			Exist = Exist && T && T->Series == F.SeriesId;
			Agree = Agree && fest::IsTitleTemplate(Id);
			// RiverLine's titles are all majors, so WorldHeroMinorTitle still answers false for them (no world fact moves at FN-1).
			Majors = Majors && (F.Site != net::SiteId::RiverLine || (T && T->Main));
		}
		Titles += F.TitleIds.size();
		if ((F.TitleIds != Want || !Exist) && Bad.empty())
		{
			Bad = ": first wrong " + F.SeriesId + " (" + std::to_string(F.TitleIds.size()) + " titles)";
		}
	}
	const fest::Festival* Rcop = fest::FestivalOf("rcop");
	const bool Rcop26 = Rcop && Rcop->TitleIds == std::vector<std::string>{"rcop-main", "rcop-mini", "rcop-micro"};
	const fest::Festival* Slam = fest::FestivalOf("slam27");
	const net::EventTemplate* SlamMini = net::Shared().FindTemplate("slam27-mini");
	const bool SlamOne = Slam && Slam->TitleIds == std::vector<std::string>{"slam27-main"} && SlamMini && SlamMini->Main;
	const bool NotTitles = !fest::IsTitleTemplate("slam27-mini") && !fest::IsTitleTemplate("slam27-hr") && !fest::IsTitleTemplate("rcop-1") &&
		!fest::IsTitleTemplate("rcop27-hr") && !fest::IsTitleTemplate("rcop27-shr") && !fest::IsTitleTemplate("mm27-main") && !fest::IsTitleTemplate("mm-main") &&
		!fest::IsTitleTemplate("slam-main") && !fest::IsTitleTemplate("step4") && !fest::IsTitleTemplate("mega-rcop") && !fest::IsTitleTemplate("") &&
		!fest::IsTitleTemplate("rcop");
	// FN-3: the rival championships' samples.
	const fest::Festival* Pen27 = fest::FestivalOf("ignite.pennant27");
	const fest::Festival* Reign28 = fest::FestivalOf("runittwice.reign28");
	const bool Champs = Pen27 && Pen27->TitleIds == std::vector<std::string>{"ignite.pennant27-main", "ignite.pennant27-mini", "ignite.pennant27-micro"} && Reign28 &&
		Reign28->TitleIds == std::vector<std::string>{"runittwice.reign28-main", "runittwice.reign28-mini", "runittwice.reign28-micro"} &&
		!fest::IsTitleTemplate("ignite.pennant27-hr") && !fest::IsTitleTemplate("ignite.pennant27-on") && !fest::IsTitleTemplate("ignite.pennant27-mega") &&
		!fest::IsTitleTemplate("ignite.pennant27-33") && !fest::IsTitleTemplate("ignite.pennant27-s3a") && !fest::IsTitleTemplate("ignite.pennant27-sat1") &&
		!fest::IsTitleTemplate("ignite.pennant-main") && !fest::IsTitleTemplate("runittwice.reign-main") && !fest::IsTitleTemplate("runittwice.reign27-27") &&
		!fest::IsTitleTemplate("runittwice.reign40-s1b") && !fest::IsTitleTemplate("ignite.feign27-hr") && fest::IsTitleTemplate("ignite.push27-main") &&
		fest::IsTitleTemplate("runittwice.writ27-main") && !fest::IsTitleTemplate("ignite.push27-mega") && !fest::IsTitleTemplate("runittwice.writ27-sat1");
	Expect(Bad.empty() && Rcop26 && SlamOne && Agree && NotTitles && Majors && Champs && Championships == 15 + 14 + 14,
		"every championship edition (" + std::to_string(Championships) + ": RCOP's 15, rcop-main, rcop-mini, rcop-micro in 2026; PENNANT's and REIGN's 14 each) has TitleIds "
		"{main, mini, micro}, every other {MainId} (Summer Slam's, though its Mini is a major); IsTitleTemplate holds exactly the " + std::to_string(Titles) +
		" title ids (no High Roller, Opener, mega, step, satellite or Main ticket); RiverLine's titles are majors" + Bad);
}

// ------------------------------------------------------------------ the rival festivals (FN-2a: FEIGN and GRIT)

/** A rival festival edition's templates on its host, in template order: its series' events and the satellites seating its Main. */
std::vector<const ss::net::EventTemplate*> FestNetEditionTemplates(const ss::fest::Festival& F)
{
	const ss::net::Network& N = ss::net::Site(F.Site);
	std::vector<const ss::net::EventTemplate*> Out;
	for (const ss::net::EventTemplate& T : N.Templates())
	{
		const bool Satellite = std::find(F.Satellites.begin(), F.Satellites.end(), T.Id) != F.Satellites.end();
		if (T.Series == F.SeriesId || Satellite)
		{
			Out.push_back(&T);
		}
	}
	return Out;
}

/** Every rival festival edition in All() (FN-2a: FEIGN and GRIT). */
std::vector<const ss::fest::Festival*> FestNetRivalEditions()
{
	std::vector<const ss::fest::Festival*> Out;
	for (const ss::fest::Festival& F : ss::fest::All())
	{
		if (F.Site != ss::net::SiteId::RiverLine)
		{
			Out.push_back(&F);
		}
	}
	return Out;
}

/** A template's start minutes inside its day, every run (GRIT's shows: 19:00 and 22:00). */
std::vector<int> FestNetRuns(const ss::net::EventTemplate& T)
{
	std::vector<int> Out;
	for (int M = T.StartMinute; M <= (T.EveryMinutes > 0 ? T.LastMinute : T.StartMinute); M += std::max(1, T.EveryMinutes))
	{
		Out.push_back(M);
	}
	return Out;
}

/** The brand's room words (SPEC v1.7 §2.3.2's lists, in their order: the order is part of the data; PENNANT's and REIGN's with the
 *  lead's L6 swaps at their own indices). */
std::vector<std::string> FestNetRooms(ss::fest::Brand B)
{
	if (B == ss::fest::Brand::Pennant)
	{
		return {"Bleachers", "Can of Corn", "Box Seats", "On Deck", "Mound Visit", "Warning Track", "Foul Pole", "Dinger", "Rally Cap", "Bat Rack", "Rosin Bag", "Base Path",
			"Line Drive", "Knuckleball", "Curveball", "Changeup", "Screwball", "Walk-Off", "Extra Innings", "Pickoff", "Turnstile", "Fair Ball", "Pinch Hitter", "Batting Cage",
			"Southpaw", "Bobblehead", "Concourse", "Fungo", "Hot Corner", "Bases Loaded", "Tag Up", "Chin Music", "Dugout"};
	}
	if (B == ss::fest::Brand::Reign)
	{
		return {"Cloakroom", "Night Porter", "Spyhole", "Latchkey", "Mortise", "Escutcheon", "Door Chain", "Secret Panel", "Doorknocker", "Fanlight", "Antechamber",
			"Newel Post", "Alcove", "Foyer", "Lift Cage", "Sash Window", "Ceiling Rose", "Parquet", "Speaking Tube", "Mantelpiece", "Wingback", "Hat Stand", "Dumbwaiter",
			"Bolthole"};
	}
	if (B == ss::fest::Brand::Feign)
	{
		return {"Wonky Stairs", "Air Jets", "Big Wheel", "Toffee Apple", "Spinning Disc", "Rope Bridge", "Lantern Walk", "Hook-a-Duck", "Swing Boats", "Ticket Booth",
			"Stilt Walk", "Barrel Organ", "Trapdoor", "Wobble Room", "Candy Floss", "Mirror Maze", "Rolling Barrel", "Crooked House", "Tilt Floor", "Ghost Train", "Coconut Shy",
			"Ring Toss", "Carousel", "Laughing Clown"};
	}
	if (B == ss::fest::Brand::Grit)
	{
		return {"Encore", "Second House", "Footlights", "Stage Door", "Dress Circle", "Box Office", "Opening Act", "Overture", "Interval", "Front Row", "Spotlight",
			"Understudy"};
	}
	if (B == ss::fest::Brand::Push)
	{
		return {"Sea Spray", "Promenade", "Striker", "Boardwalk", "Saltwater", "Iron Pier", "Gull's Rest", "Tide Clock", "Ferris Light", "Lifebuoy", "Harbour Lamp",
			"Deckchair", "Rock Candy", "Breakwater", "Pier End", "Seawall", "Anchor Rope", "Shell Cove"};
	}
	if (B == ss::fest::Brand::Writ)
	{
		return {"Long Gallery", "Seal Room", "Map Room", "Reading Room", "Strong Room", "Writing Room", "Clerk's Office", "Hilary Term", "Gatehouse", "Back Stair",
			"Morning Room", "Witness Box"};
	}
	return {};
}

/** The plan facts the checks hold the rival festivals to (SPEC v1.7 §2.3.2): numbered events, the most a day, headliners and
 *  satellite templates (a championship's steps run on two days: 3 x 2 + 2), each site's in its plan order (§2.3.1: FEIGN, PUSH and
 *  PENNANT on Ignite, WRIT, GRIT and REIGN on RunItTwice; FN-2b inserted PUSH after FEIGN and WRIT before GRIT). */
struct FestNetPlan
{
	ss::fest::Brand Which;
	int Numbered, PerDay, Headliners, Satellites;
	ss::net::SiteId Site;
};
const FestNetPlan FestNetPlans[] = {{ss::fest::Brand::Feign, 24, 3, 3, 2, ss::net::SiteId::Ignite}, {ss::fest::Brand::Push, 18, 2, 3, 2, ss::net::SiteId::Ignite},
	{ss::fest::Brand::Pennant, 33, 3, 6, 8, ss::net::SiteId::Ignite}, {ss::fest::Brand::Writ, 12, 2, 1, 1, ss::net::SiteId::RunItTwice},
	{ss::fest::Brand::Grit, 12, 1, 3, 2, ss::net::SiteId::RunItTwice}, {ss::fest::Brand::Reign, 24, 2, 6, 8, ss::net::SiteId::RunItTwice}};

const FestNetPlan* FestNetPlanOf(ss::fest::Brand B)
{
	for (const FestNetPlan& P : FestNetPlans)
	{
		if (P.Which == B)
		{
			return &P;
		}
	}
	return nullptr;
}

/** RiverLinePinned's FNV over a network's first Count templates (tests/sites_main.cpp's SitesTemplatesFnv, the same fields). */
uint64_t FestNetPrefixFnv(const ss::net::Network& N, size_t Count)
{
	uint64_t H = 1469598103934665603ull;
	for (size_t I = 0; I < Count && I < N.Templates().size(); ++I)
	{
		const ss::net::EventTemplate& T = N.Templates()[I];
		const std::string Row = T.Id + "|" + T.Series + "|" + T.Ticket + "|" + std::to_string(T.BuyInCents) + "|" + std::to_string(ss::net::FeeOf(T.BuyInCents)) + "|" +
			std::to_string(T.GtdCents) + "|" + std::to_string(T.Field) + "|" + std::to_string(static_cast<int>(T.Fmt)) + "|" + std::to_string(T.StartMinute) + "|" +
			std::to_string(T.EveryMinutes) + "|" + std::to_string(T.LastMinute) + "|" + std::to_string(T.Days) + "|" + std::to_string(T.OnlyDay) + "|" + std::to_string(T.TableSize) +
			"|" + std::to_string(T.StartStack) + "|" + std::to_string(T.LateRegMinutes) + "|" + std::to_string(T.SeatValueCents) + "\n";
		for (const char C : Row)
		{
			H ^= static_cast<uint64_t>(static_cast<unsigned char>(C));
			H *= 1099511628211ull;
		}
	}
	return H;
}

/** "0x38f22b3af5ae3e70" (sites_main.cpp's SitesHex, for the messages). */
std::string SitesHexNet(uint64_t H)
{
	char B[32];
	std::snprintf(B, sizeof(B), "0x%016llx", static_cast<unsigned long long>(H));
	return B;
}

// ------------------------------------------------------------------ FestTemplatesAppended (F-net; FN-2a's siblings)

/** FinalTarget's rule (G/WorldSim.cpp:76-90, F-world's; restated for the checks): from a ticket, template by template while it is a
 *  satellite with a seat, to where the chain of seats ends (a Step 1 ticket walks Step 2, Step 3, to the Main ticket). */
std::string FestNetFinalTarget(const ss::net::Network& N, const std::string& Ticket)
{
	std::string At = Ticket;
	for (int Guard = 0; Guard < 8; ++Guard)
	{
		const ss::net::EventTemplate* T = N.FindTemplate(At);
		if (!T || T->Fmt != ss::net::Format::Satellite || T->SeatTicket.empty())
		{
			break;
		}
		At = T->SeatTicket;
	}
	return At;
}

/** §2.3.1, §2.3.4: the festivals append after every existing rival template (the prefixes hash to Phase A's constants,
 *  tests/sites_main.cpp:362-363), plan by plan in each site's order, edition by edition: FEIGN's 15 x 29 = 435, PUSH's 14 x 23 = 322
 *  and PENNANT's 14 x 47 = 658 on Ignite (2,198), WRIT's 14 x 14 = 196, GRIT's 14 x 17 = 238 and REIGN's 14 x 38 = 532 on RunItTwice
 *  (2,487) (FEIGN 2026 at Ignite's 783-811, the fixture's FEIGN I, PUSH 2027 at 1,218, PENNANT 2027 at 1,540; WRIT 2027 at
 *  RunItTwice's 1,521, GRIT 2027 at 1,717, REIGN 2027 at 1,955); each edition in the fixed order (numbered No 1..N, the headliners in
 *  calendar order, the satellites in ladder order); ids unique across the three sites; every satellite's seat chains to its
 *  edition's Main ticket, valued at the buy-in of the rung it seats. Planted: the festival loop placed before the series loop; the
 *  championship plan placed before FEIGN; a Step 2 row seating into a ticket no template takes; PUSH appended after PENNANT (FN-2b). */
void FestTemplatesAppendedNet()
{
	namespace fest = ss::fest;
	namespace net = ss::net;
	const net::Network& Ig = net::Site(net::SiteId::Ignite);
	const net::Network& Rt = net::Site(net::SiteId::RunItTwice);
	const uint64_t IgFnv = FestNetPrefixFnv(Ig, 783);
	const uint64_t RtFnv = FestNetPrefixFnv(Rt, 1521);
	Expect(IgFnv == 0x4658347bd97f6bfcull && RtFnv == 0x4e4bb2627a9641d9ull,
		"the rival prefixes are Phase A's: Ignite's first 783 and RunItTwice's first 1,521 templates hash to sites_main.cpp's constants (" + std::to_string(IgFnv) + ", " +
			std::to_string(RtFnv) + ")");

	// Every template from the prefix on belongs to a festival edition, edition by edition in template order, 29 / 17 an edition.
	struct SiteRow
	{
		const net::Network* N;
		net::SiteId Id;
		size_t Prefix, Appended;
	};
	const SiteRow Sites[] = {{&Ig, net::SiteId::Ignite, 783, 435 + 322 + 658}, {&Rt, net::SiteId::RunItTwice, 1521, 196 + 238 + 532}};
	std::string Bad;
	for (const SiteRow& S : Sites)
	{
		size_t At = S.Prefix;
		// Plan by plan in the site's order, each brand's editions by year (All() is in FirstDay order).
		std::vector<const fest::Festival*> Editions;
		for (const FestNetPlan& Pl : FestNetPlans)
		{
			for (const fest::Festival* F : FestNetRivalEditions())
			{
				if (Pl.Site == S.Id && F->Which == Pl.Which)
				{
					Editions.push_back(F);
				}
			}
		}
		for (const fest::Festival* F : Editions)
		{
			if (F->Site != S.Id)
			{
				continue;
			}
			const FestNetPlan* Plan = FestNetPlanOf(F->Which);
			const std::vector<const net::EventTemplate*> Mine = FestNetEditionTemplates(*F);
			const size_t Want = Plan ? static_cast<size_t>(Plan->Numbered + Plan->Headliners + Plan->Satellites) : 0;
			bool Order = Mine.size() == Want && Plan;
			int LastDay = -1, LastStart = -1;
			for (size_t K = 0; Order && K < Mine.size(); ++K)
			{
				const net::EventTemplate& T = *Mine[K];
				Order = Order && At + K < S.N->Templates().size() && &T == &S.N->Templates()[At + K];
				if (K < static_cast<size_t>(Plan->Numbered))
				{
					Order = Order && T.EventNo == static_cast<int>(K) + 1 && T.Id == F->SeriesId + "-" + std::to_string(K + 1);
				}
				else if (K < static_cast<size_t>(Plan->Numbered + Plan->Headliners))
				{
					// The headliners in calendar order, numbered after the numbered events.
					Order = Order && T.EventNo == static_cast<int>(K) + 1 && (T.OnlyDay > LastDay || (T.OnlyDay == LastDay && T.StartMinute > LastStart)) && T.Series == F->SeriesId &&
						T.Fmt != net::Format::Satellite;
					LastDay = T.OnlyDay;
					LastStart = T.StartMinute;
				}
				else
				{
					// A satellite: its seat chains to the edition's Main ticket (directly, or up a championship's ladder), valued at the
					// buy-in of the rung it seats (the Main's, or the next step's first template), which is one of this edition's own.
					const net::EventTemplate* Main = S.N->FindTemplate(F->MainId);
					const std::string MainTicket = S.N->TicketOf(F->MainId);
					const net::EventTemplate* Rung = T.SeatTicket == MainTicket ? Main : S.N->FindTemplate(T.SeatTicket);
					const bool Own = Rung == Main || (Rung && std::find(F->Satellites.begin(), F->Satellites.end(), Rung->Id) != F->Satellites.end());
					Order = Order && T.Fmt == net::Format::Satellite && T.Series.empty() && T.EventNo == 0 && Main && FestNetFinalTarget(*S.N, T.SeatTicket) == MainTicket &&
						Rung && Own && T.SeatValueCents == Rung->BuyInCents && T.GtdCents == 0;
				}
			}
			if (!Order && Bad.empty())
			{
				Bad = ": first out of place " + F->SeriesId + " (" + std::to_string(Mine.size()) + " templates from index " + std::to_string(At) + ")";
			}
			At += Mine.size();
		}
		if (S.N->Templates().size() != S.Prefix + S.Appended || At != S.N->Templates().size())
		{
			Bad += Bad.empty() ? std::string(": ") + net::KeyOf(S.Id) + " has " + std::to_string(S.N->Templates().size()) + " templates, editions end at " + std::to_string(At) : "";
		}
	}
	const auto IdAt = [](const net::Network& N, size_t I) { return I < N.Templates().size() ? N.Templates()[I].Id : std::string(); };
	const bool Feign26 = IdAt(Ig, 783) == "ignite.feign26-1" && IdAt(Ig, 811) == "ignite.feign26-sat2" && IdAt(Ig, 1218) == "ignite.push27-1" &&
		IdAt(Ig, 1240) == "ignite.push27-sat2" && IdAt(Ig, 1540) == "ignite.pennant27-1" && IdAt(Rt, 1521) == "runittwice.writ27-1" && IdAt(Rt, 1534) == "runittwice.writ27-sat1" &&
		IdAt(Rt, 1717) == "runittwice.grit27-1" && IdAt(Rt, 1955) == "runittwice.reign27-1";
	Expect(Bad.empty() && Feign26,
		"Ignite gains FEIGN's 15 x 29 = 435, PUSH's 14 x 23 = 322 and PENNANT's 14 x 47 = 658 templates (2,198) and RunItTwice WRIT's 14 x 14 = 196, GRIT's 14 x 17 = 238 and "
		"REIGN's 14 x 38 = 532 (2,487), after every existing template, plan by plan (FEIGN 2026 at Ignite's 783-811, PUSH 2027 at 1,218-1,240, PENNANT 2027 from 1,540; WRIT "
		"2027 at RunItTwice's 1,521-1,534, GRIT 2027 from 1,717, REIGN 2027 from 1,955), each edition in order: numbered No 1..N, the headliners in calendar order numbered "
		"after them, the satellites in ladder order, each seat chaining to its Main's ticket at the rung's buy-in (no series, no number, no seat guarantee)" + Bad);

	// FN-4a (§2.2.3, §2.2.4): RiverLine's 84 satellites after every existing template (its first 8,453 hash to RiverLinePinned's
	// 5bc7d44 constant), year by year 2027-2040, Ring Rush's then the Championship Online's, each Main's Satellite, Mega and Last
	// Chance with §2.2.3's facts; each edition's Festival::Satellites is exactly its three, RCOP 2026's stays its step4 and Mega, and
	// Summer Slam's editions have none until G3.
	struct FestNetTrioWant
	{
		const char* Stem;
		const char* Name;
		ss::Chips BuyIn;
		int DaysOut;
		int Start[2]; // Ring Rush, the Championship Online (minutes)
		const char* Speed;
		double Level;
		int Stack, LateReg;
		int Field[2];
		int Seats, Opens;
	};
	const FestNetTrioWant Trio[] = {{"step", "Main Satellite", 5500, 2, {20 * 60, 20 * 60}, "Turbo", 6.0, 20000, 60, {120, 300}, 0, 72},
		{"mega", "Main Mega Satellite", 10900, 1, {20 * 60, 20 * 60}, "Turbo", 6.0, 20000, 60, {130, 650}, 10, 168},
		{"last", "Main Last Chance", 21500, 0, {14 * 60, 16 * 60}, "Hyper", 3.0, 10000, 15, {60, 90}, 0, 72}};
	const net::Network& Rl = net::Site(net::SiteId::RiverLine);
	const uint64_t RlFnv = FestNetPrefixFnv(Rl, 8453);
	std::string RlBad;
	size_t RlAt = 8453;
	for (int Year = 2027; Year <= 2040; ++Year)
	{
		for (int P = 0; P < 2; ++P)
		{
			const std::string Series = std::string(P == 0 ? "ring" : "tco") + std::to_string(Year % 100);
			const std::string Festival = P == 0 ? "Ring Rush" : "The Championship Online";
			const fest::Festival* F = fest::FestivalOf(Series);
			const net::EventTemplate* Main = Rl.FindTemplate(Series + "-main");
			std::vector<std::string> Want;
			bool Ok = F && Main;
			for (const FestNetTrioWant& W : Trio)
			{
				const net::EventTemplate* T = RlAt < Rl.Templates().size() ? &Rl.Templates()[RlAt] : nullptr;
				++RlAt;
				Want.push_back(Series + "-" + W.Stem);
				Ok = Ok && T && T->Id == Want.back() && T->Name == Festival + " " + W.Name && T->Series.empty() && T->EventNo == 0 && T->Fmt == net::Format::Satellite &&
					!T->Main && T->Ticket.empty() && T->Blurb.empty() && T->BuyInCents == W.BuyIn && T->Speed == W.Speed && T->LevelMinutes == W.Level &&
					T->StartStack == W.Stack && T->LateRegMinutes == W.LateReg && T->TableSize == 8 && T->Field == W.Field[P] && T->OnlyDay == Main->OnlyDay - W.DaysOut &&
					T->StartMinute == W.Start[P] && T->OpensHours == W.Opens && T->GtdCents == static_cast<ss::Chips>(W.Seats) * Main->BuyInCents &&
					T->SeatTicket == Rl.TicketOf(Main->Id) && T->SeatValueCents == Main->BuyInCents && T->Seats == std::string(P == 0 ? "RING" : "TCO") + " Main Event seats" &&
					T->Days == 0x7f && T->EveryMinutes == 0;
			}
			Ok = Ok && Main->StartMinute == (P == 0 ? 16 * 60 : 18 * 60) && F->Satellites == Want;
			if (!Ok && RlBad.empty())
			{
				RlBad = ": first out of place " + Series;
			}
		}
	}
	const fest::Festival* Rcop26 = fest::FestivalOf("rcop");
	const bool RcopSats = Rcop26 && Rcop26->Satellites == std::vector<std::string>{"step4", "mega-rcop"};
	bool SlamNone = true;
	for (int Year = 2027; Year <= 2040; ++Year)
	{
		const fest::Festival* Slam = fest::FestivalOf("slam" + std::to_string(Year % 100));
		SlamNone = SlamNone && Slam && Slam->Satellites.empty();
	}
	Expect(Rl.Templates().size() == 8537 && RlAt == 8537 && RlFnv == 0x38f22b3af5ae3e70ull && RlBad.empty() && RcopSats && SlamNone,
		"RiverLine gains 84 satellites (8,537 templates) after its first 8,453, which still hash to 5bc7d44's constant (" + SitesHexNet(RlFnv) +
			"): 2027-2040, Ring Rush's then the Championship Online's, each Main's Satellite ($55, two days out at 20:00, Turbo, 120/300), Mega ($109, the night "
			"before at 20:00, Turbo, 130/650, ten seats at the Main's buy-in) and Last Chance ($215, two hours before the Main: 14:00 / 16:00, Hyper, 60/90), no "
			"series, no number, seating the Main's ticket at its buy-in; each edition's Satellites are its three, RCOP 2026's its step4 and Mega, Summer Slam's none (" +
			std::to_string(Rl.Templates().size()) + " templates)" + RlBad);

	// Ids unique across the three sites.
	std::set<std::string> Ids;
	size_t Total = 0;
	for (int S = 0; S < static_cast<int>(net::SiteId::Count); ++S)
	{
		for (const net::EventTemplate& T : net::Site(static_cast<net::SiteId>(S)).Templates())
		{
			Ids.insert(T.Id);
			++Total;
		}
	}
	Expect(Ids.size() == Total, "every template id is unique across the three sites (" + std::to_string(Total) + " templates, " + std::to_string(Total - Ids.size()) + " repeats)");
}

// ------------------------------------------------------------------ FestStartsApart (F-net)

/** §2.3.2: inside a day the numbered starts rise with No (FEIGN's No 16 at 18:00, No 17 at 21:30); no two headliners and no numbered
 *  event share a minute on one day (every run counted); no satellite starts at a headliner's minute (one beside a numbered event is
 *  allowed); no day holds more numbered events than the plan's PerDay. FN-2a: FEIGN and GRIT, every edition; FN-3: PENNANT and REIGN
 *  (0 violations 2027-2040; REIGN's numbered starts 14:00 and 17:00, PENNANT's 15:00, 17:30 and 20:00), and the championship days'
 *  instances §2.14 gives world_u4's row (PENNANT's day 10 holds 14 of its own, REIGN's 11); FN-2b: PUSH (15:00 and 17:30; its #18 and
 *  Main Last Chance together at 15:00 on day 10) and WRIT (15:00 and 18:00; its #12 alone on Main day at 15:00, never at the Main's
 *  18:00). Planted: v1.0's (No - 1) % PerDay slot; REIGN numbered at 15:00 + Slot x 180 (18:00 meets the Micro Main on day 11); WRIT
 *  numbered at 18:00 + Slot x 180 (its #12 meets the Main, FN-2b). */
void FestStartsApartNet()
{
	namespace fest = ss::fest;
	namespace net = ss::net;
	int Days = 0;
	std::string Bad;
	for (const fest::Festival* F : FestNetRivalEditions())
	{
		const FestNetPlan* Plan = FestNetPlanOf(F->Which);
		std::map<int, std::vector<const net::EventTemplate*>> ByDay;
		for (const net::EventTemplate* T : FestNetEditionTemplates(*F))
		{
			ByDay[T->OnlyDay].push_back(T);
		}
		for (const auto& Day : ByDay)
		{
			++Days;
			int LastNo = 0, LastStart = -1, Numbered = 0;
			std::map<int, int> Heads;    // minute -> headliners there
			std::map<int, int> Numbers;  // minute -> numbered runs there
			std::vector<int> Satellites; // minutes
			std::string Why;
			for (const net::EventTemplate* T : Day.second)
			{
				const bool Satellite = T->Fmt == net::Format::Satellite;
				const bool Number = !Satellite && Plan && T->EventNo <= Plan->Numbered;
				if (Number)
				{
					++Numbered;
					if (T->EventNo < LastNo || (T->EventNo > LastNo && T->StartMinute <= LastStart))
					{
						Why = "No " + std::to_string(T->EventNo) + " at " + net::TimeLabel(T->StartMinute) + " after No " + std::to_string(LastNo) + " at " + net::TimeLabel(LastStart);
					}
					LastNo = T->EventNo;
					LastStart = T->StartMinute;
				}
				for (int M : FestNetRuns(*T))
				{
					if (Satellite)
					{
						Satellites.push_back(M);
					}
					else
					{
						++(Number ? Numbers : Heads)[M];
					}
				}
			}
			for (const auto& H : Heads)
			{
				Why = H.second > 1 || Numbers.count(H.first) ? "a headliner shares " + net::TimeLabel(H.first) : Why;
			}
			for (const auto& Nm : Numbers)
			{
				Why = Nm.second > 1 ? "two numbered runs at " + net::TimeLabel(Nm.first) : Why;
			}
			for (int M : Satellites)
			{
				Why = Heads.count(M) ? "a satellite at a headliner's " + net::TimeLabel(M) : Why;
			}
			Why = Plan && Numbered > Plan->PerDay ? std::to_string(Numbered) + " numbered events on one day" : Why;
			if (!Why.empty() && Bad.empty())
			{
				Bad = ": " + F->Name + " on " + FestNetDate(Day.first) + ": " + Why;
			}
		}
	}
	// The samples: FEIGN 2026's headline pair and No 17; GRIT 2027's shows at 7 and 10 PM.
	const net::Network& Ig = net::Site(net::SiteId::Ignite);
	const net::Network& Rt = net::Site(net::SiteId::RunItTwice);
	const net::EventTemplate* F8 = Ig.FindTemplate("ignite.feign26-8");
	const net::EventTemplate* F9 = Ig.FindTemplate("ignite.feign26-9");
	const net::EventTemplate* F10 = Ig.FindTemplate("ignite.feign26-10");
	const net::EventTemplate* F16 = Ig.FindTemplate("ignite.feign26-16");
	const net::EventTemplate* F17 = Ig.FindTemplate("ignite.feign26-17");
	const net::EventTemplate* Top = Ig.FindTemplate("ignite.feign26-mega");
	const net::EventTemplate* G7 = Rt.FindTemplate("runittwice.grit27-7");
	const bool Samples = F8 && F9 && F10 && F16 && F17 && Top && G7 && F8->StartMinute == 20 * 60 && F9->StartMinute == 15 * 60 && F10->StartMinute == 17 * 60 + 30 &&
		F16->StartMinute == 18 * 60 && F17->StartMinute == 21 * 60 + 30 && F16->OnlyDay == Top->OnlyDay && Top->StartMinute == 20 * 60 && F16->OnlyDay == 62 &&
		FestNetRuns(*G7) == std::vector<int>{19 * 60, 22 * 60} && G7->OnlyDay == ss::world::DayOn(2027, 5, 18);
	// FN-2b: PUSH's and WRIT's numbered starts, every edition, and their Main days.
	std::set<int> PushStarts, WritStarts;
	for (const fest::Festival* F : FestNetRivalEditions())
	{
		const FestNetPlan* Plan = FestNetPlanOf(F->Which);
		for (const net::EventTemplate* T : FestNetEditionTemplates(*F))
		{
			if (Plan && T->Fmt != net::Format::Satellite && T->EventNo <= Plan->Numbered && (F->Which == fest::Brand::Push || F->Which == fest::Brand::Writ))
			{
				(F->Which == fest::Brand::Push ? PushStarts : WritStarts).insert(T->StartMinute);
			}
		}
	}
	const net::EventTemplate* W12 = Rt.FindTemplate("runittwice.writ27-12");
	const net::EventTemplate* WMain = Rt.FindTemplate("runittwice.writ27-main");
	const net::EventTemplate* W2 = Rt.FindTemplate("runittwice.writ27-2");
	const net::EventTemplate* P18 = Ig.FindTemplate("ignite.push27-18");
	const net::EventTemplate* PLast = Ig.FindTemplate("ignite.push27-sat2");
	const bool Siblings = PushStarts == std::set<int>{15 * 60, 17 * 60 + 30} && WritStarts == std::set<int>{15 * 60, 18 * 60} && W12 && WMain && W2 && P18 && PLast &&
		W12->OnlyDay == WMain->OnlyDay && W12->StartMinute == 15 * 60 && WMain->StartMinute == 18 * 60 && W2->OnlyDay == 109 && W2->StartMinute == 18 * 60 &&
		P18->OnlyDay == PLast->OnlyDay && P18->StartMinute == 15 * 60 && PLast->StartMinute == 15 * 60;
	Expect(Siblings, "PUSH's numbered start at 3:00 and 5:30 PM, WRIT's at 3:00 and 6:00 PM; WRIT 2027's No 12 is alone on its Main day at 3:00 PM, never at the Main's "
					 "6:00 PM (its No 2 takes 6:00 PM on Jan 22, a day with no headliner); PUSH 2027's No 18 and its Main Last Chance share 3:00 PM on day 10");
	// FN-3: the championships' numbered starts, every edition, and their day 10 (§2.14).
	std::set<int> PennantStarts, ReignStarts;
	for (const fest::Festival* F : FestNetRivalEditions())
	{
		const FestNetPlan* Plan = FestNetPlanOf(F->Which);
		for (const net::EventTemplate* T : FestNetEditionTemplates(*F))
		{
			if (Plan && T->Fmt != net::Format::Satellite && T->EventNo <= Plan->Numbered && fest::IsChampionship(F->Which))
			{
				(F->Which == fest::Brand::Pennant ? PennantStarts : ReignStarts).insert(T->StartMinute);
			}
		}
	}
	const auto OwnOnDay = [](const fest::Festival* F, int DayNo) {
		int Count = 0;
		if (!F)
		{
			return -1;
		}
		const net::Network& N = net::Site(F->Site);
		const int Day = F->FirstDay + DayNo - 1;
		for (const net::EventInstance& E : N.Window(FestNetAt(Day), FestNetAt(Day + 1)))
		{
			const net::EventTemplate& T = N.TemplateOf(E);
			Count += T.Series == F->SeriesId || std::find(F->Satellites.begin(), F->Satellites.end(), T.Id) != F->Satellites.end() ? 1 : 0;
		}
		return Count;
	};
	const fest::Festival* Pen27 = fest::FestivalOf("ignite.pennant27");
	const fest::Festival* Reign27 = fest::FestivalOf("runittwice.reign27");
	const int Pen10 = OwnOnDay(Pen27, 10);
	const int Reign10 = OwnOnDay(Reign27, 10);
	std::printf("festivals: PENNANT 2027's day 10 (world day %d) holds %d of its instances, REIGN 2027's day 10 (world day %d) %d\n", Pen27 ? Pen27->FirstDay + 9 : 0, Pen10,
		Reign27 ? Reign27->FirstDay + 9 : 0, Reign10);
	const bool Champs = PennantStarts == std::set<int>{15 * 60, 17 * 60 + 30, 20 * 60} && ReignStarts == std::set<int>{14 * 60, 17 * 60} && Pen10 == 14 && Reign10 == 11 &&
		Pen27 && Pen27->FirstDay + 9 == 185 && Reign27 && Reign27->FirstDay + 9 == 303;
	Expect(Days > 0 && Bad.empty() && Samples && Champs,
		"FEIGN, PUSH, PENNANT, WRIT, GRIT and REIGN, every edition (" + std::to_string(Days) + " festival days): numbered starts rise with No inside a day, no headliner shares a minute with another "
		"event, no two numbered runs share one, no satellite starts at a headliner's minute, no day over its PerDay; FEIGN 2026's No 8 at 8:00 PM, No 9 and 10 at 3:00 and 5:30 PM, "
		"No 16 at 6:00 PM on Dec 6 (The Big Top's 8:00 PM) and No 17 at 9:30 PM; GRIT 2027's No 7 on May 18 at 7:00 and 10:00 PM; PENNANT's numbered at 3:00, 5:30 and 8:00 PM "
		"and REIGN's at 2:00 and 5:00 PM only; PENNANT 2027's day 10 (world day 185) holds 14 of its instances (3 numbered, the High Roller, Step 1 x 8, Step 3 x 2) and "
		"REIGN 2027's (world day 303) 11" + Bad);
}

// ------------------------------------------------------------------ FestRivalCapG1 (F-net)

/** §2.3.3: until G3 every rival festival template's largest draw stays under 3,000, every edition to 2040: a series event's
 *  Field x Scale x 1.15, a satellite's Field x Scale x the site's largest day factor x 1.25 (Make's draws). FN-2a: FEIGN and GRIT
 *  (The Big Top 2,760 and The Matinee 2,875); FN-3: PENNANT and REIGN, whose megas are the tightest (General Admission 2,990.1, The
 *  Side Door 2,989.8); FN-2b: PUSH and WRIT (Standing Room Only 2,875, The Summons 2,963 at the Sunday factor 1.5). Planted: The Big
 *  Top at 2,700 entries; General Admission at 2,610 entries (3,001.5); The Summons at 1,700 entries (3,187, FN-2b). */
void FestRivalCapG1Net()
{
	namespace fest = ss::fest;
	namespace net = ss::net;
	double Largest = 0.0;
	std::string Which;
	int Templates = 0;
	int Over = 0;
	std::string FirstOver;
	for (const fest::Festival* F : FestNetRivalEditions())
	{
		const net::SiteDef& D = net::Def(F->Site);
		double DayMax = 0.0;
		for (int Wd = 0; Wd < 7; ++Wd)
		{
			DayMax = std::max(DayMax, net::netsites_detail::NetSitesDayFactor(D, Wd));
		}
		for (const net::EventTemplate* T : FestNetEditionTemplates(*F))
		{
			++Templates;
			const double Draw = static_cast<double>(T->Field) * D.Traffic.Scale * (T->Series.empty() ? DayMax * 1.25 : 1.15);
			if (Draw > Largest)
			{
				Largest = Draw;
				Which = T->Id;
			}
			if (Draw >= 3000.0)
			{
				++Over;
				FirstOver = FirstOver.empty() ? "; first: " + T->Id + " " + std::to_string(Draw) : FirstOver;
			}
		}
	}
	std::printf("festivals: the rival festivals' largest draw %.1f (%s), %d templates\n", Largest, Which.c_str(), Templates);
	const auto Largest1 = [](net::SiteId S, const char* Id) {
		const net::EventTemplate* T = net::Site(S).FindTemplate(Id);
		return T ? std::round(static_cast<double>(T->Field) * net::Def(S).Traffic.Scale * 1.15 * 10.0) / 10.0 : -1.0;
	};
	const bool Megas = Largest1(net::SiteId::Ignite, "ignite.pennant27-mega") == 2990.1 && Largest1(net::SiteId::RunItTwice, "runittwice.reign27-mega") == 2989.8 &&
		Largest1(net::SiteId::Ignite, "ignite.pennant40-mega") == 2990.1;
	Expect(Megas, "the championship megas' largest draws are General Admission's 2,990.1 and The Side Door's 2,989.8, every edition");
	// FN-2b: Standing Room Only (a series event, x 1.15) and The Summons (a satellite: RunItTwice's Sunday factor x 1.25), every edition.
	double SummonsDay = 0.0;
	for (int Wd = 0; Wd < 7; ++Wd)
	{
		SummonsDay = std::max(SummonsDay, net::netsites_detail::NetSitesDayFactor(net::Def(net::SiteId::RunItTwice), Wd));
	}
	const auto Summons = [SummonsDay](const char* Id) {
		const net::EventTemplate* T = net::Site(net::SiteId::RunItTwice).FindTemplate(Id);
		return T ? std::round(static_cast<double>(T->Field) * net::Def(net::SiteId::RunItTwice).Traffic.Scale * SummonsDay * 1.25 * 10.0) / 10.0 : -1.0;
	};
	std::printf("festivals: Standing Room Only's largest draw %.1f, The Summons' %.1f (day factor %.2f)\n", Largest1(net::SiteId::Ignite, "ignite.push27-mega"),
		Summons("runittwice.writ27-sat1"), SummonsDay);
	const bool Siblings = std::lround(Largest1(net::SiteId::Ignite, "ignite.push27-mega")) == 2875 && std::lround(Largest1(net::SiteId::Ignite, "ignite.push40-mega")) == 2875 &&
		std::lround(Summons("runittwice.writ27-sat1")) == 2963 && std::lround(Summons("runittwice.writ40-sat1")) == 2963 && SummonsDay == 1.5;
	Expect(Siblings, "Standing Room Only's largest draw rounds to 2,875 and The Summons' to 2,963 (1,580 entries x RunItTwice's Sunday 1.5 x 1.25), every edition");
	Expect(Templates > 0 && Over == 0,
		"every rival festival template's largest draw is under 3,000 every edition (" + std::to_string(Templates) + " templates; largest " + std::to_string(Largest) + ", " + Which +
			")" + FirstOver);
}

// ------------------------------------------------------------------ FestNamesThemed (F-net)

/** §2.3.2 Names: every numbered event's name carries its brand's room word, event No taking the list's entry at
 *  (Fnv1a(SeriesId) + (No - 1) x 7) % RoomCount, skipping words already used; no word repeats inside an edition; FEIGN 2026's #7 Tilt
 *  Floor, #8 Air Jets, #9 Swing Boats, #10 Mirror Maze, #11 Carousel, #12 Rope Bridge, #16 Ticket Booth; GRIT 2027's #7 Dress Circle
 *  and #10 Second House; FN-3: PENNANT 2027's #10 Can of Corn and REIGN 2027's 03 Sash Window, and a championship's three titles in
 *  caps on Ignite (MICRO MAIN EVENT, MINI MAIN EVENT, MAIN EVENT) and in the bracket on RunItTwice; FN-2b: PUSH 2027's #4 Breakwater
 *  and WRIT 2027's 11 Long Gallery (rooms_check.py's); every word passes both trademark tiers and the design-word lint (PLAN_G1 item
 *  19). Planted: the k-th word drawn with repeats; "Main Event" printed mixed-case on Ignite (FN-3); two of PUSH's words swapped in
 *  the plan's list (FN-2b). */
void FestNamesThemedNet()
{
	namespace fest = ss::fest;
	namespace net = ss::net;
	int Names = 0;
	std::string Bad;
	for (const fest::Festival* F : FestNetRivalEditions())
	{
		const std::vector<std::string> Rooms = FestNetRooms(F->Which);
		const FestNetPlan* Plan = FestNetPlanOf(F->Which);
		if (Rooms.empty() || !Plan)
		{
			Bad = Bad.empty() ? ": no room list for " + F->Name : Bad;
			continue;
		}
		std::vector<bool> Used(Rooms.size(), false);
		std::set<std::string> Seen;
		const uint32_t Key = ss::Fnv1a(F->SeriesId);
		for (const net::EventTemplate* T : FestNetEditionTemplates(*F))
		{
			if (T->Fmt == net::Format::Satellite || T->EventNo > Plan->Numbered)
			{
				continue;
			}
			++Names;
			uint32_t K = (Key + static_cast<uint32_t>(T->EventNo - 1) * 7u) % static_cast<uint32_t>(Rooms.size());
			while (Used[K])
			{
				K = (K + 1u) % static_cast<uint32_t>(Rooms.size());
			}
			Used[K] = true;
			// The word sits between the buy-in and the kind: "<code>: <buy-in> <Room> <Kind>".
			const size_t After = T->Name.find(' ', T->Name.find(": ") + 2);
			const bool Carries = After != std::string::npos && T->Name.compare(After + 1, Rooms[K].size(), Rooms[K]) == 0;
			if ((!Carries || !Seen.insert(Rooms[K]).second) && Bad.empty())
			{
				Bad = ": " + T->Name + " (want " + Rooms[K] + ")";
			}
		}
	}
	const net::Network& Ig = net::Site(net::SiteId::Ignite);
	const net::Network& Rt = net::Site(net::SiteId::RunItTwice);
	auto Has = [](const net::Network& N, const char* Id, const char* Word) {
		const net::EventTemplate* T = N.FindTemplate(Id);
		return T && T->Name.find(std::string(" ") + Word + " ") != std::string::npos;
	};
	const bool Samples = Has(Ig, "ignite.push27-4", "Breakwater") && Has(Rt, "runittwice.writ27-11", "Long Gallery") && Has(Rt, "runittwice.writ27-3", "Strong Room") &&
		Has(Ig, "ignite.push27-1", "Harbour Lamp") && Has(Ig, "ignite.feign26-7", "Tilt Floor") && Has(Ig, "ignite.feign26-8", "Air Jets") && Has(Ig, "ignite.feign26-9", "Swing Boats") &&
		Has(Ig, "ignite.feign26-10", "Mirror Maze") && Has(Ig, "ignite.feign26-11", "Carousel") && Has(Ig, "ignite.feign26-12", "Rope Bridge") &&
		Has(Ig, "ignite.feign26-16", "Ticket Booth") && Has(Rt, "runittwice.grit27-7", "Dress Circle") && Has(Rt, "runittwice.grit27-10", "Second House") &&
		Has(Ig, "ignite.pennant27-10", "Can of Corn") && Has(Rt, "runittwice.reign27-3", "Sash Window");
	// FN-3: a championship's titles print in its site's title form, every edition (Ignite's caps, RunItTwice's bracket).
	int TitleRows = 0;
	std::string TitleBad;
	for (const fest::Festival* F : FestNetRivalEditions())
	{
		const net::Network& N = net::Site(F->Site);
		for (const std::string& Id : fest::IsChampionship(F->Which) ? F->TitleIds : std::vector<std::string>{})
		{
			const net::EventTemplate* T = N.FindTemplate(Id);
			const bool Micro = Id.size() > 6 && Id.compare(Id.size() - 6, 6, "-micro") == 0;
			const bool Mini = Id.size() > 5 && Id.compare(Id.size() - 5, 5, "-mini") == 0;
			const std::string Word = Micro ? "Micro Main Event" : Mini ? "Mini Main Event" : "Main Event";
			const std::string Caps = Micro ? "MICRO MAIN EVENT" : Mini ? "MINI MAIN EVENT" : "MAIN EVENT";
			const std::string Tail = T ? ": " + net::BuyIn(T->BuyInCents) + (F->Site == net::SiteId::RunItTwice ? " NLHE [" + Word + "]" : " " + Caps) : std::string();
			++TitleRows;
			if ((!T || T->Name.size() < Tail.size() || T->Name.compare(T->Name.size() - Tail.size(), Tail.size(), Tail) != 0) && TitleBad.empty())
			{
				TitleBad = ": " + (T ? T->Name : Id) + " (want \"..." + Tail + "\")";
			}
		}
	}
	Expect(TitleRows == 28 * 3 && TitleBad.empty(),
		"PENNANT's and REIGN's " + std::to_string(TitleRows) + " title rows print in their site's form: \"PENNANT #37: $2.20 MICRO MAIN EVENT\", \"PENNANT #38: $22 MINI MAIN "
		"EVENT\", \"PENNANT #39: $215 MAIN EVENT\" on Ignite, \"REIGN 28: $22 NLHE [Micro Main Event]\" ... \"REIGN 30: $2,100 NLHE [Main Event]\" on RunItTwice" + TitleBad);
	std::string Hits;
	for (fest::Brand B : {fest::Brand::Feign, fest::Brand::Push, fest::Brand::Writ, fest::Brand::Grit, fest::Brand::Pennant, fest::Brand::Reign})
	{
		for (const std::string& W : FestNetRooms(B))
		{
			const char* Hit = trademarks::Find(W, trademarks::kSourceScan);
			Hit = Hit ? Hit : trademarks::Find(W, trademarks::kCopyScan);
			Hit = Hit ? Hit : trademarks::Find(W, trademarks::kDesignWords);
			Hits += Hit ? " " + W + " (" + Hit + ")" : std::string();
		}
	}
	Expect(Names > 0 && Bad.empty() && Samples && Hits.empty(),
		"every FEIGN, PUSH, WRIT, GRIT, PENNANT and REIGN numbered name (" + std::to_string(Names) + ") carries its list's word at (Fnv1a(SeriesId) + (No - 1) x 7) % "
		"RoomCount, none twice in an edition; FEIGN 2026's #7 Tilt Floor ... #12 Rope Bridge and #16 Ticket Booth, PUSH 2027's #1 Harbour Lamp and #4 Breakwater, WRIT "
		"2027's 03 Strong Room and 11 Long Gallery, GRIT 2027's #7 Dress Circle and #10 Second House, PENNANT 2027's #10 Can of Corn, REIGN 2027's 03 Sash Window; every word "
		"passes both trademark tiers and the design words" + Bad + Hits);
}

// ------------------------------------------------------------------ FestCodes (F-net)

/** §2.3.2, §2.11 H14, §6 PA-4: Ignite's lines "<Short> #<No>: <BuyIn> <Room> <Kind>" and its Main in caps; RunItTwice's with no "#"
 *  and two digits, its headliners "NLHE [<Headliner>]"; Network::NameOf gives each GRIT show's 19:00 instance "-E" and its 22:00
 *  instance "-L" and every other instance its template's name, and the listing and News read it; no tier letter (GRIT's runs aside),
 *  "[Day" or "RIT-"; SeriesInfo::Title GRIT's, PUSH's and WRIT's, "" for FEIGN, PENNANT and REIGN; FN-3: the championships' print
 *  (SPEC's samples and PIE's lines); FN-2b: PUSH's and WRIT's (SPEC's samples, WRIT's ladder and PIE's "WRIT 03: $2,100 Strong Room
 *  PKO" on day 110). Planted: NameOf giving GRIT's 22:00 run the 19:00 run's "-E"; "Main Event" printed mixed-case on Ignite (FN-3);
 *  WRIT's ladder read at No % 5 (FN-2b). */
void FestCodesNet()
{
	namespace fest = ss::fest;
	namespace net = ss::net;
	const net::Network& Ig = net::Site(net::SiteId::Ignite);
	const net::Network& Rt = net::Site(net::SiteId::RunItTwice);
	auto NameIs = [](const net::Network& N, const char* Id, const char* Want) {
		const net::EventTemplate* T = N.FindTemplate(Id);
		return T && T->Name == Want;
	};
	const bool Lines = NameIs(Ig, "ignite.feign26-9", "FEIGN #9: $16.50 Swing Boats Mystery") && NameIs(Ig, "ignite.feign26-1", "FEIGN #1: $3.30 Wonky Stairs Flip & Run") &&
		NameIs(Ig, "ignite.feign26-mega", "FEIGN #25: $2.20 The Big Top") && NameIs(Ig, "ignite.feign26-hr", "FEIGN #26: $109 High Roller") &&
		NameIs(Ig, "ignite.feign26-main", "FEIGN #27: $55 MAIN EVENT") && NameIs(Rt, "runittwice.grit27-main", "GRIT 15: $530 NLHE [Main Event]") &&
		NameIs(Rt, "runittwice.grit27-on", "GRIT 13: $109 NLHE [Opening Night]") && NameIs(Rt, "runittwice.grit27-mega", "GRIT 14: $3.30 NLHE [The Matinee]") &&
		NameIs(Rt, "runittwice.grit27-7", "GRIT 07: $33 Dress Circle Turbo") && NameIs(Ig, "ignite.feign26-sat1", "Main Mega Satellite") &&
		NameIs(Rt, "runittwice.grit27-sat2", "Main Last Chance");
	// FN-3 (§2.3.2's championships' print; PIE's lines among them).
	const bool Champs = !Ig.FindTemplate("ignite.pennant27-39") && NameIs(Ig, "ignite.pennant27-main", "PENNANT #39: $215 MAIN EVENT") &&
		NameIs(Ig, "ignite.pennant27-10", "PENNANT #10: $3.30 Can of Corn PKO") && NameIs(Ig, "ignite.pennant27-4", "PENNANT #4: $44 Bobblehead No-Limit Hold'em") &&
		NameIs(Ig, "ignite.pennant27-on", "PENNANT #34: $11 Play Ball") && NameIs(Ig, "ignite.pennant27-mega", "PENNANT #35: $1.10 General Admission") &&
		NameIs(Ig, "ignite.pennant27-hr", "PENNANT #36: $530 High Roller") && NameIs(Ig, "ignite.pennant27-micro", "PENNANT #37: $2.20 MICRO MAIN EVENT") &&
		NameIs(Ig, "ignite.pennant27-mini", "PENNANT #38: $22 MINI MAIN EVENT") && NameIs(Rt, "runittwice.reign27-main", "REIGN 30: $2,100 NLHE [Main Event]") &&
		NameIs(Rt, "runittwice.reign27-3", "REIGN 03: $530 Sash Window NLHE") && NameIs(Rt, "runittwice.reign27-4", "REIGN 04: $33 Dumbwaiter NLHE") &&
		NameIs(Rt, "runittwice.reign27-hr", "REIGN 27: $5,250 NLHE [High Roller]") && NameIs(Rt, "runittwice.reign27-on", "REIGN 25: $215 NLHE [Coats Off]") &&
		NameIs(Rt, "runittwice.reign27-mega", "REIGN 26: $5.50 NLHE [The Side Door]") && NameIs(Rt, "runittwice.reign27-micro", "REIGN 28: $22 NLHE [Micro Main Event]") &&
		NameIs(Rt, "runittwice.reign27-mini", "REIGN 29: $215 NLHE [Mini Main Event]") && NameIs(Ig, "ignite.pennant27-s1a", "Step 1 \xC2\xB7 PENNANT Main") &&
		NameIs(Ig, "ignite.pennant27-s2b", "Step 2 \xC2\xB7 PENNANT Main") && NameIs(Rt, "runittwice.reign27-s3a", "Step 3 \xC2\xB7 REIGN Main") &&
		NameIs(Rt, "runittwice.reign27-sat1", "Main Mega Satellite") && NameIs(Ig, "ignite.pennant27-sat2", "Main Last Chance");
	Expect(Champs, "the championships' lines: \"PENNANT #39: $215 MAIN EVENT\", \"PENNANT #10: $3.30 Can of Corn PKO\", \"PENNANT #4: $44 Bobblehead No-Limit Hold'em\", "
				   "\"PENNANT #34: $11 Play Ball\" ... \"PENNANT #38: $22 MINI MAIN EVENT\", \"REIGN 30: $2,100 NLHE [Main Event]\", \"REIGN 03: $530 Sash Window NLHE\", "
				   "\"REIGN 04: $33 Dumbwaiter NLHE\", \"REIGN 27: $5,250 NLHE [High Roller]\", \"REIGN 25: $215 NLHE [Coats Off]\" ... \"REIGN 29: $215 NLHE [Mini Main "
				   "Event]\"; the satellites \"Step 1 · PENNANT Main\" ... \"Step 3 · REIGN Main\", \"Main Mega Satellite\", \"Main Last Chance\"");
	// FN-2b (§2.3.2's PUSH and WRIT samples; WRIT's ladder, Ladder[(No x 3) % 5]; PIE's "WRIT 03" line on day 110).
	const net::EventTemplate* W3 = Rt.FindTemplate("runittwice.writ27-3");
	const net::EventTemplate* Summons = Rt.FindTemplate("runittwice.writ27-sat1");
	bool Ladder = true;
	for (int No = 1; No <= 12; ++No)
	{
		const net::EventTemplate* T = Rt.FindTemplate("runittwice.writ28-" + std::to_string(No));
		const ss::Chips Want[] = {21500, 32000, 53000, 105000, 210000};
		Ladder = Ladder && T && T->BuyInCents == Want[(No * 3) % 5];
	}
	const bool Siblings = NameIs(Ig, "ignite.push27-4", "PUSH #4: $16.50 Breakwater No-Limit Hold'em") && !Ig.FindTemplate("ignite.push27-21") &&
		NameIs(Ig, "ignite.push27-main", "PUSH #21: $109 MAIN EVENT") && NameIs(Ig, "ignite.push27-mega", "PUSH #19: $1.10 Standing Room Only") &&
		NameIs(Ig, "ignite.push27-hr", "PUSH #20: $215 High Roller") && NameIs(Ig, "ignite.push27-3", "PUSH #3: $4.40 Gull's Rest Turbo") &&
		NameIs(Ig, "ignite.push27-2", "PUSH #2: $27.50 Shell Cove PKO") && NameIs(Ig, "ignite.push27-sat1", "Main Mega Satellite") &&
		NameIs(Ig, "ignite.push27-sat2", "Main Last Chance") && NameIs(Rt, "runittwice.writ27-11", "WRIT 11: $1,050 Long Gallery PKO") &&
		NameIs(Rt, "runittwice.writ27-main", "WRIT 13: $1,050 NLHE [Main Event]") && NameIs(Rt, "runittwice.writ27-3", "WRIT 03: $2,100 Strong Room PKO") && W3 &&
		W3->OnlyDay == 110 && NameIs(Rt, "runittwice.writ27-1", "WRIT 01: $1,050 Map Room NLHE") && NameIs(Rt, "runittwice.writ27-12", "WRIT 12: $320 Hilary Term NLHE") &&
		Summons && Summons->Name == "The Summons" && Summons->Seats == "WRIT Main Event seats" && Summons->SeatTicket == "runittwice.writ27-main" &&
		Summons->SeatValueCents == 105000 && Summons->GtdCents == 0 && Summons->Blurb == "$22 for a seat in the Main Event: one for every $1,050 in the pool." && Ladder;
	Expect(Siblings, "PUSH's and WRIT's lines: \"PUSH #4: $16.50 Breakwater No-Limit Hold'em\", \"PUSH #19: $1.10 Standing Room Only\", \"PUSH #20: $215 High Roller\", "
					 "\"PUSH #21: $109 MAIN EVENT\", \"WRIT 11: $1,050 Long Gallery PKO\", \"WRIT 13: $1,050 NLHE [Main Event]\", PIE's \"WRIT 03: $2,100 Strong Room PKO\" "
					 "(day 110); WRIT's numbered buy-ins Ladder[(No x 3) % 5] of $215, $320, $530, $1,050, $2,100; The Summons seats the Main ($1,050, \"WRIT Main Event seats\", "
					 "no seat guarantee, blurb B6 filled)");
	Expect(Lines, "the codes in each site's form: \"FEIGN #9: $16.50 Swing Boats Mystery\", \"FEIGN #1: $3.30 Wonky Stairs Flip & Run\", \"FEIGN #25: $2.20 The Big Top\", "
				  "\"FEIGN #27: $55 MAIN EVENT\", \"GRIT 13: $109 NLHE [Opening Night]\", \"GRIT 15: $530 NLHE [Main Event]\", \"GRIT 07: $33 Dress Circle Turbo\"; the satellites "
				  "\"Main Mega Satellite\" and \"Main Last Chance\"");

	// NameOf over GRIT 2027's nights (every instance from the schedule): the 7 PM run "-E", the 10 PM run "-L", the headliners and every
	// other instance their template's name; the listing's name and spec read it, and News' event names with it.
	int Shows = 0;
	std::string Bad;
	std::string Pie;
	const fest::Festival* Grit = fest::FestivalOf("runittwice.grit27");
	for (int Day = Grit ? Grit->FirstDay : 0; Grit && Day <= Grit->LastDay; ++Day)
	{
		std::set<std::string> Night;
		for (const net::EventInstance& E : Rt.Window(FestNetAt(Day), FestNetAt(Day + 1)))
		{
			const net::EventTemplate& T = Rt.TemplateOf(E);
			const std::string Name = Rt.NameOf(E);
			const ss::LobbyEvent L = Rt.Listing(E, nullptr, 7);
			bool Ok = L.Name == Name && L.Spec.Name == (T.Series.empty() ? net::BuyIn(T.BuyInCents) + " " + Name : Name);
			if (T.Series == Grit->SeriesId && T.EveryMinutes > 0)
			{
				++Shows;
				const int Minute = static_cast<int>(E.Start - FestNetAt(Day));
				const std::string Code = T.Name.substr(0, T.Name.find(": "));
				const std::string Want = Code + (Minute == 19 * 60 ? "-E" : Minute == 22 * 60 ? "-L" : "-?") + T.Name.substr(T.Name.find(": "));
				Ok = Ok && Name == Want && Night.insert(Name).second;
				Pie = E.Start == FestNetAt(ss::world::DayOn(2027, 5, 13), 19 * 60) ? Name : Pie;
			}
			else
			{
				Ok = Ok && Name == T.Name;
			}
			if (!Ok && Bad.empty())
			{
				Bad = ": " + T.Id + " at " + net::TimeLabel(E.Start) + " reads \"" + Name + "\"";
			}
		}
	}
	// Every Ignite instance over FEIGN 2026 and PENNANT 2027, and every RunItTwice instance over REIGN 2027 (the steps' runs among
	// them), reads its template's name.
	const fest::Festival* Pen = fest::FestivalOf("ignite.pennant27");
	const fest::Festival* Reign = fest::FestivalOf("runittwice.reign27");
	struct Span
	{
		const net::Network* N;
		int From, To;
	};
	const fest::Festival* Push = fest::FestivalOf("ignite.push27");
	const fest::Festival* Writ = fest::FestivalOf("runittwice.writ27");
	for (const Span& Sp : {Span{&Ig, 56, 66}, Span{&Ig, Pen ? Pen->FirstDay : 0, Pen ? Pen->LastDay + 1 : 0}, Span{&Rt, Reign ? Reign->FirstDay : 0, Reign ? Reign->LastDay + 1 : 0},
			 Span{&Ig, Push ? Push->FirstDay : 0, Push ? Push->LastDay + 1 : 0}, Span{&Rt, Writ ? Writ->FirstDay : 0, Writ ? Writ->LastDay + 1 : 0}})
	{
		for (const net::EventInstance& E : Sp.N->Window(FestNetAt(Sp.From), FestNetAt(Sp.To)))
		{
			if (Sp.N->NameOf(E) != Sp.N->TemplateOf(E).Name && Bad.empty())
			{
				Bad = ": " + Sp.N->TemplateOf(E).Id + " reads \"" + Sp.N->NameOf(E) + "\"";
			}
		}
	}
	Expect(Shows == 24 && Bad.empty() && Pie == "GRIT 02-E: $16.50 Box Office PKO",
		"Network::NameOf names GRIT 2027's " + std::to_string(Shows) + " shows by their run (\"GRIT 07-E\" at 7 PM, \"GRIT 07-L\" at 10 PM, one of each a night; the PIE line "
		"\"GRIT 02-E: $16.50 Box Office PKO\" on May 13), every other instance (FEIGN's, PUSH's, WRIT's, PENNANT's and REIGN's with their steps' runs, the headliners, the dailies) its "
		"template's name; the listing reads it (" + Pie + ")" + Bad);

	// The forms: Ignite "<Short> #<No>: ", RunItTwice "<Short> <NN>: " (and "-E"/"-L" from NameOf only); no tier letter, "[Day" or RIT-.
	std::string Form;
	for (const fest::Festival* F : FestNetRivalEditions())
	{
		const net::SeriesInfo* Sr = net::Site(F->Site).FindSeries(F->SeriesId);
		const std::string Short = Sr ? Sr->Short : std::string();
		for (const net::EventTemplate* T : FestNetEditionTemplates(*F))
		{
			if (T->Fmt == net::Format::Satellite)
			{
				continue;
			}
			const std::string No = std::to_string(T->EventNo);
			const std::string Code = F->Site == net::SiteId::RunItTwice ? Short + " " + (T->EventNo < 10 ? "0" : "") + No : Short + " #" + No;
			const bool Ok = T->Name.compare(0, Code.size() + 2, Code + ": ") == 0 && T->Name.find("[Day") == std::string::npos && T->Name.find("RIT-") == std::string::npos;
			if (!Ok && Form.empty())
			{
				Form = ": " + T->Name + " (want \"" + Code + ": \")";
			}
		}
	}
	const net::SeriesInfo* GritSr = Rt.FindSeries("runittwice.grit27");
	const net::SeriesInfo* FeignSr = Ig.FindSeries("ignite.feign26");
	const net::SeriesInfo* PenSr = Ig.FindSeries("ignite.pennant27");
	const net::SeriesInfo* ReignSr = Rt.FindSeries("runittwice.reign27");
	const net::SeriesInfo* PushSr = Ig.FindSeries("ignite.push27");
	const net::SeriesInfo* WritSr = Rt.FindSeries("runittwice.writ27");
	Expect(Form.empty() && GritSr && GritSr->Title == "The Grinders' RunItTwice" && FeignSr && FeignSr->Title.empty() && GritSr->Short == "GRIT" && FeignSr->Short == "FEIGN" &&
			   PenSr && PenSr->Title.empty() && PenSr->Short == "PENNANT" && ReignSr && ReignSr->Title.empty() && ReignSr->Short == "REIGN" && PushSr &&
			   PushSr->Title == "Pier's Upbeat Summer Hold'em" && PushSr->Short == "PUSH" && WritSr && WritSr->Title == "The Winter RunItTwice" && WritSr->Short == "WRIT",
		"every FEIGN, PUSH, PENNANT, WRIT, GRIT and REIGN event's code is its site's form (Ignite \"FEIGN #<No>: \", \"PUSH #<No>: \", \"PENNANT #<No>: \", RunItTwice "
		"\"WRIT <NN>: \", \"GRIT <NN>: \", \"REIGN <NN>: \", no tier letter, \"[Day\" or RIT-); GRIT's Title is \"The Grinders' RunItTwice\", PUSH's \"Pier's Upbeat "
		"Summer Hold'em\", WRIT's \"The Winter RunItTwice\"; FEIGN, PENNANT and REIGN have none" + Form);
}

// ------------------------------------------------------------------ FestMajorFloor (F-net's half)

/** A title row's median pool in dollars, by §2.4.2's W5 arithmetic (not the generator's NetSitesGtd): the median entries
 *  T.Field x Scale x 1.025 (a series event's median draw) times the contribution llround(BuyIn x the site's prize share). */
double FestNetMedianPool(const ss::net::SiteDef& D, const ss::net::EventTemplate& T)
{
	const double Contribution = static_cast<double>(std::llround(static_cast<double>(T.BuyInCents) * D.Rake.PrizeShare(T))) / 100.0;
	return static_cast<double>(T.Field) * D.Traffic.Scale * 1.025 * Contribution;
}

/** §2.6.2's major floor (v1.6): a title row is a major (T.Main) only when its median pool reaches the smallest major the game made
 *  before G1, the Wildfire Main's ($33 x 600 entries: $18,063): every rival title row with T.Main clears it and every one under it
 *  has T.Main false (PENNANT's Micro Main, $5,022: a title, not a major); the table's 2027 pools to the dollar ($5,022, $24,083,
 *  $78,434; $37,346, $184,413, $711,533); RiverLine's titles are majors (RCOP's trio with $2M-$25M guarantees). Lasting and HallScore
 *  are F-world's half. Planted: PENNANT's Micro with T.Main = true. */
void FestMajorFloorNet()
{
	namespace fest = ss::fest;
	namespace net = ss::net;
	const net::EventTemplate* Wild = net::Site(net::SiteId::Ignite).FindTemplate("ignite.wildfeb27-main");
	const double Floor = Wild ? FestNetMedianPool(net::Def(net::SiteId::Ignite), *Wild) : 0.0;
	// The smallest major on the rival sites outside the festivals: the majors the game made before G1.
	double Smallest = 1.0e18;
	std::string SmallestId;
	for (net::SiteId S : {net::SiteId::Ignite, net::SiteId::RunItTwice})
	{
		const net::Network& N = net::Site(S);
		for (const net::EventTemplate& T : N.Templates())
		{
			const net::SeriesInfo* Sr = T.Series.empty() ? nullptr : N.FindSeries(T.Series);
			const double Pool = T.Main && Sr && !Sr->Festival ? FestNetMedianPool(net::Def(S), T) : 1.0e18;
			if (Pool < Smallest)
			{
				Smallest = Pool;
				SmallestId = T.Id;
			}
		}
	}
	std::printf("festivals: the major floor $%.2f (%s); the smallest pre-G1 rival major $%.2f (%s)\n", Floor, Wild ? Wild->Id.c_str() : "-", Smallest, SmallestId.c_str());
	Expect(Wild && std::llround(Floor) == 18063 && std::fabs(Smallest - Floor) < 0.005,
		"the major floor is the Wildfire Main's median pool, $18,063 ($33 x 600 entries), the smallest major the rival sites made before G1 (" + SmallestId + ")");

	int Rows = 0;
	int Majors = 0;
	std::string Bad;
	for (const fest::Festival& F : fest::All())
	{
		const net::Network& N = net::Site(F.Site);
		for (const std::string& Id : F.TitleIds)
		{
			const net::EventTemplate* T = N.FindTemplate(Id);
			++Rows;
			Majors += T && T->Main ? 1 : 0;
			const double Pool = T && F.Site != net::SiteId::RiverLine ? FestNetMedianPool(net::Def(F.Site), *T) : 0.0;
			const bool Ok = T && (F.Site == net::SiteId::RiverLine ? T->Main : T->Main == (Pool >= Floor));
			if (!Ok && Bad.empty())
			{
				Bad = ": " + Id + (T && T->Main ? " is a major" : " is not a major") + " at a median pool of $" + std::to_string(std::llround(Pool));
			}
		}
	}
	// §2.6.2's table, 2027 (every edition's rows are the same plan): the six rival title rows' median pools, to the dollar.
	struct TableRow
	{
		net::SiteId Site;
		const char* Id;
		long long Pool;
		bool Main;
	};
	const TableRow Table[] = {{net::SiteId::Ignite, "ignite.pennant27-micro", 5022, false}, {net::SiteId::Ignite, "ignite.pennant27-mini", 24083, true},
		{net::SiteId::Ignite, "ignite.pennant27-main", 78434, true}, {net::SiteId::RunItTwice, "runittwice.reign27-micro", 37346, true},
		{net::SiteId::RunItTwice, "runittwice.reign27-mini", 184413, true}, {net::SiteId::RunItTwice, "runittwice.reign27-main", 711533, true}};
	std::string TableBad;
	for (const TableRow& R : Table)
	{
		const net::EventTemplate* T = net::Site(R.Site).FindTemplate(R.Id);
		const long long Pool = T ? std::llround(FestNetMedianPool(net::Def(R.Site), *T)) : -1;
		TableBad += T && Pool == R.Pool && T->Main == R.Main ? "" : std::string(" ") + R.Id + " $" + std::to_string(Pool) + (T && T->Main ? " major" : "");
	}
	bool Rcop = true;
	for (const char* Id : {"rcop-main", "rcop-mini", "rcop-micro"})
	{
		const net::EventTemplate* T = net::Shared().FindTemplate(Id);
		Rcop = Rcop && T && T->Main && T->GtdCents >= 100000000;
	}
	Expect(Rows > 0 && Bad.empty() && TableBad.empty() && Rcop,
		"every title row (" + std::to_string(Rows) + ", " + std::to_string(Majors) + " majors) is a major exactly when its median pool reaches the floor: PENNANT's Micro Main "
		"$5,022 is a title, not a major; its Mini $24,083 and Main $78,434, REIGN's Micro $37,346, Mini $184,413 and Main $711,533 are majors; RiverLine's titles are majors "
		"(RCOP's trio over $1M each)" + Bad + TableBad);
}

// ------------------------------------------------------------------ FN-6's walks (F-net)

/** Every run of template T starting in [From, To), through Network::Next (Instances' own list, never NextFor), in start order: the
 *  independent walk FN-6's checks hold NextFor and SeatsWon to. */
std::vector<ss::net::EventInstance> FestNetRunsOf(const ss::net::Network& N, const ss::net::EventTemplate& T, double From, double To)
{
	std::vector<ss::net::EventInstance> Out;
	ss::net::EventInstance E;
	for (double At = From; N.Next(T.Id, At, E) && E.Start < To; At = E.Start + 1.0)
	{
		Out.push_back(E);
	}
	return Out;
}

/** The world's seat rule (G/WorldSim.cpp's P.Seats) over every run of the one-offs Temps on their days finished by At:
 *  Σ max(1, Pool / SeatValue). */
int FestNetSeatsOf(const ss::net::Network& N, const std::vector<const ss::net::EventTemplate*>& Temps, double At)
{
	int Seats = 0;
	for (const ss::net::EventTemplate* T : Temps)
	{
		for (const ss::net::EventInstance& E : FestNetRunsOf(N, *T, FestNetAt(T->OnlyDay), FestNetAt(T->OnlyDay + 1)))
		{
			Seats += E.Start + E.Duration <= At && T->SeatValueCents > 0 ? std::max<int>(1, static_cast<int>(E.Pool / T->SeatValueCents)) : 0;
		}
	}
	return Seats;
}

// ------------------------------------------------------------------ FestLadderSeats (F-net)

/** §2.3.2's ladder (v1.6), every PENNANT and REIGN edition: Step k's SeatTicket is Step k+1's ticket key; both of a step's templates
 *  take one key, the first run's template id (-s1a, -s2a, -s3a); Step 3, the Mega and the Last Chance seat the Main ticket; the seats
 *  texts and values follow the next rung ("Step 2 tickets ($5.50)" at $5.50, "PENNANT Main Event seats" at $215) and SeatValue reads
 *  them by ticket; walked with FinalTarget's rule every satellite's seat ends at the Main ticket, which NextFor resolves through
 *  Takes to the edition's Main; a Step 2 ticket won on day 8 registers on day 9's Step 2 and, unused, on day 11's; Festival::Satellites
 *  holds the eight in ladder order; FN-6: once the edition is over, SeatsWon counts the seats of Step 3, the Mega and the Last Chance
 *  only (the satellites seating the Main ticket), never Step 1's or Step 2's step tickets. Planted: a step keyed <series>-s2
 *  (FinalTarget stops at Step 2); SeatsWon counting Step 1's seats. */
void FestLadderSeatsNet()
{
	namespace fest = ss::fest;
	namespace net = ss::net;
	int Editions = 0;
	std::string Bad;
	for (const fest::Festival* F : FestNetRivalEditions())
	{
		if (!fest::IsChampionship(F->Which))
		{
			continue;
		}
		++Editions;
		const net::Network& N = net::Site(F->Site);
		const net::SeriesInfo* Sr = N.FindSeries(F->SeriesId);
		const std::string Short = Sr ? Sr->Short : std::string();
		const std::string MainTicket = N.TicketOf(F->MainId);
		const auto Tm = [&N, F](const char* Stem) { return N.FindTemplate(F->SeriesId + Stem); };
		const net::EventTemplate* Main = Tm("-main");
		const net::EventTemplate* S1a = Tm("-s1a");
		const net::EventTemplate* S1b = Tm("-s1b");
		const net::EventTemplate* S2a = Tm("-s2a");
		const net::EventTemplate* S2b = Tm("-s2b");
		const net::EventTemplate* S3a = Tm("-s3a");
		const net::EventTemplate* S3b = Tm("-s3b");
		const net::EventTemplate* Mega = Tm("-sat1");
		const net::EventTemplate* Last = Tm("-sat2");
		std::string Why;
		if (!(Main && S1a && S1b && S2a && S2b && S3a && S3b && Mega && Last))
		{
			Why = "a rung is missing";
		}
		else
		{
			const std::string Step1 = "Step 1 \xC2\xB7 ";
			// The keys: both of a step's templates take the first run's id.
			Why = N.TicketOf(S1a->Id) == S1a->Id && N.TicketOf(S1b->Id) == S1a->Id && N.TicketOf(S2a->Id) == S2a->Id && N.TicketOf(S2b->Id) == S2a->Id &&
					N.TicketOf(S3a->Id) == S3a->Id && N.TicketOf(S3b->Id) == S3a->Id
				? Why
				: "a step's two templates take different tickets";
			// The seats: Step 1 -> Step 2's key -> Step 3's key -> the Main ticket; the Mega and the Last Chance seat the Main ticket.
			Why = S1a->SeatTicket == S2a->Id && S1b->SeatTicket == S2a->Id && S2a->SeatTicket == S3a->Id && S2b->SeatTicket == S3a->Id && S3a->SeatTicket == MainTicket &&
					S3b->SeatTicket == MainTicket && Mega->SeatTicket == MainTicket && Last->SeatTicket == MainTicket
				? Why
				: "a seat goes to the wrong rung (Step 1 seats " + S1a->SeatTicket + ")";
			// The values and texts (RCOP's ladder's form), and SeatValue by ticket.
			const bool Values = S1a->SeatValueCents == S2a->BuyInCents && S1b->SeatValueCents == S2a->BuyInCents && S2a->SeatValueCents == S3a->BuyInCents &&
				S2b->SeatValueCents == S3a->BuyInCents && S3a->SeatValueCents == Main->BuyInCents && Mega->SeatValueCents == Main->BuyInCents &&
				Last->SeatValueCents == Main->BuyInCents && N.SeatValue(S2a->Id) == S2a->BuyInCents && N.SeatValue(S3a->Id) == S3a->BuyInCents &&
				N.SeatValue(MainTicket) == Main->BuyInCents;
			const bool Texts = S1a->Seats == "Step 2 tickets (" + net::BuyIn(S2a->BuyInCents) + ")" && S2b->Seats == "Step 3 tickets (" + net::BuyIn(S3a->BuyInCents) + ")" &&
				S3b->Seats == Short + " Main Event seats" && Mega->Seats == Short + " Main Event seats" && S1a->Name == Step1 + Short + " Main" &&
				S1a->Blurb == "Win a ticket to Step 2." && S2a->Blurb == "Win a ticket to Step 3." && S3a->Blurb == "Win a seat in the " + net::BuyIn(Main->BuyInCents) + " Main Event.";
			Why = Values && Texts ? Why : "a seat's value or text (" + S1a->Seats + ", " + S3a->Seats + ")";
			// The walk: every satellite of the edition, by FinalTarget's rule, ends at the Main ticket.
			for (const std::string& Id : F->Satellites)
			{
				const net::EventTemplate* T = N.FindTemplate(Id);
				const std::string End = T ? FestNetFinalTarget(N, T->SeatTicket) : std::string();
				Why = End == MainTicket ? Why : Id + "'s seats end at " + End;
			}
			Why = F->Satellites == FestNetLadder(F->SeriesId) ? Why : "Festival::Satellites is not the ladder in order";
			// NextFor: the Main ticket from Step 1's day is this edition's Main; a Step 2 ticket won on day 8 registers on day 9's
			// Step 2 and, from day 10, on day 11's.
			net::EventInstance E;
			const bool MainNext = N.NextFor(MainTicket, FestNetAt(S1a->OnlyDay, 12 * 60), E) && &N.TemplateOf(E) == Main;
			const bool Day9 = N.NextFor(S2a->Id, FestNetAt(S1a->OnlyDay, 23 * 60), E) && &N.TemplateOf(E) == S2a && E.Start == FestNetAt(S2a->OnlyDay, S2a->StartMinute);
			const bool Day11 = N.NextFor(S2a->Id, FestNetAt(S2a->OnlyDay + 1), E) && &N.TemplateOf(E) == S2b && E.Start == FestNetAt(S2b->OnlyDay, S2b->StartMinute);
			const bool Days = S1a->OnlyDay == F->FirstDay + 7 && S2a->OnlyDay == F->FirstDay + 8 && S2b->OnlyDay == F->FirstDay + 10;
			Why = MainNext && Day9 && Day11 && Days ? Why : "NextFor misses a rung (Main " + std::to_string(MainNext) + ", day 9 " + std::to_string(Day9) + ", day 11 " +
					std::to_string(Day11) + ")";
			// SeatsWon (FN-6): over the finished edition, the Main-ticket satellites' seats alone; the steps' tickets are seats too
			// (max(1, Pool / SeatValue) a run), but not Main seats.
			const double Over = std::floor(fest::LastEnd(*F)) + FestNetDay;
			const int MainSeats = FestNetSeatsOf(N, {S3a, S3b, Mega, Last}, Over);
			const int StepSeats = FestNetSeatsOf(N, {S1a, S1b, S2a, S2b}, Over);
			const int Won = fest::SeatsWon(*F, Over);
			Why = Won == MainSeats && MainSeats > 0 && StepSeats > 0 ? Why
				: "SeatsWon " + std::to_string(Won) + " against the Main-ticket satellites' " + std::to_string(MainSeats) + " (the steps' tickets " + std::to_string(StepSeats) + ")";
		}
		if (!Why.empty() && Bad.empty())
		{
			Bad = ": " + F->Name + ": " + Why;
		}
	}
	Expect(Editions == 28 && Bad.empty(),
		"every PENNANT and REIGN edition's ladder (" + std::to_string(Editions) + "): Step 1 seats Step 2's key (-s2a), Step 2 Step 3's (-s3a), Step 3, the Mega and the Last "
		"Chance the Main ticket; a step's two templates take its first run's id; seats texts and values from the next rung (\"Step 2 tickets ($5.50)\"); FinalTarget walks every "
		"seat to the Main ticket and NextFor finds the Main through it; a Step 2 ticket from day 8 plays day 9's or day 11's Step 2; Satellites in ladder order; SeatsWon "
		"over a finished edition counts Step 3's, the Mega's and the Last Chance's seats, never the steps' tickets" + Bad);
}

// ------------------------------------------------------------------ FestMainTicket (F-net's half)

/** §2.3.2's tickets (v1.6), §2.13: each championship Main takes one key that never changes with the year, ignite.pennant-main and
 *  runittwice.reign-main (RCOP's Mains share rcop-main), in every edition 2027-2040; NextFor of that key resolves through Takes to
 *  the edition's Main, and after the 2028 Main has run to the 2029 Main, so a seat covered late registers on the next one; the key is
 *  at home on its site, seats at the Main's buy-in and is no template's id; the other festivals' Mains take their own ids. (THE
 *  BELT's seat registering is F-world's half.) Planted: the Main keyed by its edition's id. */
void FestMainTicketNet()
{
	namespace fest = ss::fest;
	namespace net = ss::net;
	struct Champ
	{
		fest::Brand Which;
		net::SiteId Site;
		const char* Key;
		const char* Prefix;
	};
	std::string Bad;
	int Editions = 0;
	for (const Champ& C : {Champ{fest::Brand::Pennant, net::SiteId::Ignite, "ignite.pennant-main", "ignite.pennant"},
			 Champ{fest::Brand::Reign, net::SiteId::RunItTwice, "runittwice.reign-main", "runittwice.reign"}})
	{
		const net::Network& N = net::Site(C.Site);
		for (const fest::Festival& F : fest::All())
		{
			if (F.Which != C.Which)
			{
				continue;
			}
			++Editions;
			const net::EventTemplate* Main = N.FindTemplate(F.MainId);
			net::EventInstance E;
			const bool Ok = Main && Main->Ticket == C.Key && N.TicketOf(F.MainId) == C.Key && N.NextFor(C.Key, FestNetAt(F.FirstDay), E) && N.TemplateOf(E).Id == F.MainId &&
				E.Start == F.MainStart;
			if (!Ok && Bad.empty())
			{
				Bad = ": " + F.Name + "'s Main takes " + (Main ? N.TicketOf(F.MainId) : std::string("nothing"));
			}
		}
		const fest::Festival* F28 = fest::FestivalOf(std::string(C.Prefix) + "28");
		const fest::Festival* F29 = fest::FestivalOf(std::string(C.Prefix) + "29");
		net::EventInstance E;
		const bool Late = F28 && F29 && N.NextFor(C.Key, F28->MainStart + 1.0, E) && N.TemplateOf(E).Id == F29->MainId && E.Start == F29->MainStart;
		const net::EventTemplate* Main28 = F28 ? N.FindTemplate(F28->MainId) : nullptr;
		const bool Home = net::SiteOf(C.Key) == C.Site && &net::ForEvent(C.Key) == &N && !N.FindTemplate(C.Key) && Main28 && N.SeatValue(C.Key) == Main28->BuyInCents;
		if (!(Late && Home) && Bad.empty())
		{
			Bad = std::string(": ") + C.Key + (Late ? " is not at home on its site" : " after the 2028 Main finds no 2029 Main");
		}
	}
	const bool Own = net::Site(net::SiteId::Ignite).TicketOf("ignite.feign27-main") == "ignite.feign27-main" &&
		net::Site(net::SiteId::RunItTwice).TicketOf("runittwice.grit27-main") == "runittwice.grit27-main" &&
		net::Site(net::SiteId::Ignite).TicketOf("ignite.push27-main") == "ignite.push27-main" &&
		net::Site(net::SiteId::RunItTwice).TicketOf("runittwice.writ27-main") == "runittwice.writ27-main";
	Expect(Editions == 28 && Bad.empty() && Own,
		"each championship Main takes its year-free ticket (ignite.pennant-main, runittwice.reign-main) in every edition 2027-2040 (" + std::to_string(Editions) +
			"); NextFor of it finds that edition's Main, and after the 2028 Main the 2029 Main; the key is at home on its site, seats at the Main's buy-in and is no "
			"template's id; FEIGN's, GRIT's, PUSH's and WRIT's Mains take their own ids" + Bad);
}

// ------------------------------------------------------------------ FestNextForRepeats (F-net, FN-6)

/** §2.3.2 (v1.6), §2.15.1, LEAD_CALLS R-15: NextFor walks a one-off's repeats. In every PENNANT and REIGN edition, walked from the
 *  step's first day, NextFor of each step's ticket meets every run of the step's two templates in order (Network::Next's list: Step 1's
 *  8 + 8 or 6 + 6, Step 2's 4 + 4 or 3 + 3, Step 3's 2 + 2), the same instances, and nothing after the last: Step 1's 13:30 run
 *  (PENNANT; REIGN's 14:00) answers once the 12:00 run has started on day 8, and a Step 2 ticket finds day 11's last run (23:00 on
 *  PENNANT, 21:00 on REIGN) a minute before it. The person half (R-15): a constructed Npc holding a Step 2 ticket keeps it, its
 *  bankroll untouched, through ExpireTickets at days 8-11's boundaries and between day 11's runs, while ExpireTickets credits a 2040
 *  Step 2 ticket at its value once that edition's Main has started (the ticket and its Main are spent). No other ticket moves: no
 *  ticket a satellite seats on any site names a repeating one-off, outside the championships' Step 2 and Step 3 keys (GRIT's shows
 *  take none). Planted: NextFor reading T.StartMinute only. */
void FestNextForRepeatsNet()
{
	namespace fest = ss::fest;
	namespace net = ss::net;
	namespace world = ss::world;
	int Editions = 0;
	int Runs = 0;
	std::string Bad;
	std::string PersonBad;
	std::string Sample;
	for (const fest::Festival* F : FestNetRivalEditions())
	{
		if (!fest::IsChampionship(F->Which))
		{
			continue;
		}
		++Editions;
		const net::Network& N = net::Site(F->Site);
		const auto Tm = [&N, F](const char* Stem) { return N.FindTemplate(F->SeriesId + Stem); };
		const net::EventTemplate* Steps[3][2] = {{Tm("-s1a"), Tm("-s1b")}, {Tm("-s2a"), Tm("-s2b")}, {Tm("-s3a"), Tm("-s3b")}};
		std::string Why;
		for (int K = 0; K < 3 && Why.empty(); ++K)
		{
			const net::EventTemplate* A = Steps[K][0];
			const net::EventTemplate* B = Steps[K][1];
			if (!A || !B)
			{
				Why = "Step " + std::to_string(K + 1) + " is missing";
				break;
			}
			// Every run, as Instances lists them, against NextFor's walk of the step's ticket.
			std::vector<net::EventInstance> Want = FestNetRunsOf(N, *A, FestNetAt(A->OnlyDay), FestNetAt(A->OnlyDay + 1));
			const std::vector<net::EventInstance> Second = FestNetRunsOf(N, *B, FestNetAt(B->OnlyDay), FestNetAt(B->OnlyDay + 1));
			Want.insert(Want.end(), Second.begin(), Second.end());
			std::vector<std::string> Got;
			net::EventInstance E;
			for (double From = FestNetAt(A->OnlyDay); N.NextFor(A->Id, From, E) && Got.size() < 64; From = E.Start + 1.0)
			{
				Got.push_back(E.Id + "/" + std::to_string(E.Entries) + "/" + std::to_string(E.Pool));
			}
			std::vector<std::string> Listed;
			for (const net::EventInstance& W : Want)
			{
				Listed.push_back(W.Id + "/" + std::to_string(W.Entries) + "/" + std::to_string(W.Pool));
			}
			const size_t Expected = F->Which == fest::Brand::Pennant ? (K == 0 ? 16u : K == 1 ? 8u : 4u) : (K == 0 ? 12u : K == 1 ? 6u : 4u);
			Runs += static_cast<int>(Got.size());
			if (Got != Listed || Listed.size() != Expected)
			{
				Why = "Step " + std::to_string(K + 1) + "'s ticket meets " + std::to_string(Got.size()) + " runs through NextFor, " + std::to_string(Listed.size()) +
					" listed (" + std::to_string(Expected) + " expected)";
			}
		}
		if (Steps[0][0] && Steps[1][0] && Steps[1][1])
		{
			const net::EventTemplate* S1a = Steps[0][0];
			const net::EventTemplate* S2a = Steps[1][0];
			const net::EventTemplate* S2b = Steps[1][1];
			const int Second = F->Which == fest::Brand::Pennant ? 13 * 60 + 30 : 14 * 60;
			const int LastRun = F->Which == fest::Brand::Pennant ? 23 * 60 : 21 * 60;
			net::EventInstance E;
			const bool Day8 = N.NextFor(S1a->Id, FestNetAt(S1a->OnlyDay, 12 * 60) + 1.0, E) && &N.TemplateOf(E) == S1a && E.Start == FestNetAt(S1a->OnlyDay, Second);
			const bool Day11 = N.NextFor(S2a->Id, FestNetAt(S2b->OnlyDay, LastRun) - 1.0, E) && &N.TemplateOf(E) == S2b && E.Start == FestNetAt(S2b->OnlyDay, LastRun);
			const bool After = !N.NextFor(S2a->Id, FestNetAt(S2b->OnlyDay, LastRun) + 1.0, E);
			const bool Days = S1a->OnlyDay == F->FirstDay + 7 && S2b->OnlyDay == F->FirstDay + 10;
			Why = (Day8 && Day11 && After && Days) || !Why.empty() ? Why
				: "Step 1's second run on day 8 " + std::to_string(Day8) + ", day 11's last Step 2 " + std::to_string(Day11) + ", nothing after it " + std::to_string(After);
			if (Why.empty() && Sample.empty())
			{
				Sample = "; " + F->Name + ": Step 1 from 12:01 on day 8 -> " + net::TimeLabel(FestNetAt(S1a->OnlyDay, Second)) + ", a Step 2 ticket from " +
					net::TimeLabel(FestNetAt(S2b->OnlyDay, LastRun) - 1.0) + " on day 11 -> " + net::TimeLabel(FestNetAt(S2b->OnlyDay, LastRun));
			}
			// The person half (R-15): a Step 2 ticket held from day 8 is never credited while day 11's runs remain.
			std::string Person;
			world::Npc P;
			P.Tickets[S2a->Id] = 1;
			const ss::Chips Roll = P.Bankroll;
			const double Moments[] = {FestNetAt(F->FirstDay + 7), FestNetAt(F->FirstDay + 8), FestNetAt(F->FirstDay + 9), FestNetAt(F->FirstDay + 10),
				FestNetAt(S2b->OnlyDay, LastRun) - 60.0};
			for (double Now : Moments)
			{
				world::worldsim_detail::ExpireTickets(P, Now);
				if (Person.empty() && (P.Tickets.count(S2a->Id) == 0 || P.Bankroll != Roll))
				{
					Person = "ExpireTickets credited a Step 2 ticket at " + net::TimeLabel(Now) + " on day " + std::to_string(net::DayOf(Now) - F->FirstDay + 1);
				}
			}
			// The contrast: the last edition's Step 2 ticket, once its Main has started, has nothing left and is credited at its value.
			if (F->Year == fest::kLastYear)
			{
				world::Npc Late;
				Late.Tickets[S2a->Id] = 1;
				const ss::Chips Before = Late.Bankroll;
				world::worldsim_detail::ExpireTickets(Late, F->MainStart + 1.0);
				Person = Person.empty() && (Late.Tickets.count(S2a->Id) != 0 || Late.Bankroll != Before + N.SeatValue(S2a->Id) || N.SeatValue(S2a->Id) != S2a->BuyInCents)
					? "the spent Step 2 ticket is not credited at its value"
					: Person;
			}
			if (!Person.empty() && PersonBad.empty())
			{
				PersonBad = "; the person half: " + F->Name + ": " + Person;
			}
		}
		if (!Why.empty() && Bad.empty())
		{
			Bad = ": " + F->Name + ": " + Why;
		}
	}
	// No other ticket moves: every ticket a satellite seats that names a repeating one-off is a championship's Step 2 or Step 3 key.
	int Seated = 0;
	std::string Other;
	for (int S = 0; S < static_cast<int>(net::SiteId::Count); ++S)
	{
		const net::Network& N = net::Site(static_cast<net::SiteId>(S));
		std::set<std::string> Repeating;
		for (const net::EventTemplate& T : N.Templates())
		{
			if (T.OnlyDay != -9999 && T.EveryMinutes > 0)
			{
				Repeating.insert(N.TicketOf(T.Id));
			}
		}
		for (const net::EventTemplate& T : N.Templates())
		{
			if (T.Fmt != net::Format::Satellite || T.SeatTicket.empty())
			{
				continue;
			}
			++Seated;
			const size_t Dash = T.SeatTicket.rfind('-');
			const std::string Key = Dash == std::string::npos ? std::string() : T.SeatTicket.substr(Dash);
			const fest::Festival* Of = fest::FestivalOf(Dash == std::string::npos ? std::string() : T.SeatTicket.substr(0, Dash));
			const bool Step = (Key == "-s2a" || Key == "-s3a") && Of && fest::IsChampionship(Of->Which) && Of->Site != net::SiteId::RiverLine;
			if (Repeating.count(T.SeatTicket) > 0 && !Step && Other.empty())
			{
				Other = "; " + T.Id + " seats " + T.SeatTicket + ", a repeating event";
			}
		}
	}
	std::printf("  FestNextForRepeats: %d championship editions, %d step runs met through NextFor, %d satellite seats checked for repeats%s\n", Editions, Runs, Seated,
		Sample.c_str());
	Expect(Editions == 28 && Bad.empty() && PersonBad.empty() && Runs == 14 * (16 + 8 + 4) + 14 * (12 + 6 + 4) && Seated > 0 && Other.empty(),
		"NextFor walks a one-off's repeats: in every PENNANT and REIGN edition (" + std::to_string(Editions) + ") each step's ticket meets every run of its two templates (" +
			std::to_string(Runs) + " runs, the instances Network::Next lists) and nothing after; Step 1's next run after 12:00 on day 8 is 13:30 (REIGN 14:00); a Step 2 "
			"ticket finds day 11's last run; ExpireTickets keeps a person's Step 2 ticket at days 8-11's boundaries and between day 11's runs, and credits a spent one at "
			"its value; no other ticket a satellite seats (" + std::to_string(Seated) + ") names a repeating event" + Sample + Bad + PersonBad + Other);
}

// ------------------------------------------------------------------ FestSeats (F-net, FN-6)

/** §2.8.3, §2.15.1, LEAD_CALLS R-14: SeatsWon. Over every edition (all three sites, 2026-2040), at Buildup's start, at the Main's
 *  start and a day after its last event, SeatsWon equals Σ max(1, Pool / SeatValue) over the runs of its satellites whose SeatTicket
 *  is its Main's ticket finished by then (each run walked by Network::Next: a one-off's on its day, RCOP's recurring step4 and
 *  mega-rcop from Buildup to the Main's start); a championship's Step 1 and 2 seats are step tickets, left out (PENNANT and REIGN:
 *  their seats exist and the figure is below the all-satellite sum), while every other festival's satellites all seat its Main, so
 *  its figure is the plain sum (unchanged); a finished edition with satellites has seats. The session half (R-14): Ring Rush 2027's
 *  Main Satellite's seat, planted as a won seat is (++Life.Tickets[SeatTicket], G/Session.cpp's seat line), registers the hero into
 *  that Main through the session's path (Listing, CanAfford through Session::TicketsFor, RegisterEvent) with the clock at day 165,
 *  2 hours before the Main, no world simulated: the table opens on the Main, the ticket is spent and the bankroll untouched.
 *  Planted: seats counted from tracked winners only (one a finished satellite); Step 1's seats counted. */
void FestSeatsNet()
{
	namespace fest = ss::fest;
	namespace net = ss::net;
	struct Run
	{
		double End;
		int Seats;
		bool Main;
	};
	int Editions = 0;
	int Champs = 0;
	int Seated = 0;
	std::string Bad;
	std::string Sample;
	for (const fest::Festival& F : fest::All())
	{
		++Editions;
		const net::Network& N = net::Site(F.Site);
		const std::string MainTicket = N.TicketOf(F.MainId);
		const double Buildup = FestNetAt(F.FirstDay - fest::kBuildupDays);
		std::vector<Run> Runs;
		bool AllMain = true;
		for (const std::string& Id : F.Satellites)
		{
			const net::EventTemplate* T = N.FindTemplate(Id);
			if (!T)
			{
				AllMain = false;
				continue;
			}
			const bool Main = T->SeatTicket == MainTicket;
			const bool Recurring = T->OnlyDay == -9999;
			AllMain = AllMain && Main;
			for (const net::EventInstance& E : FestNetRunsOf(N, *T, Recurring ? Buildup : FestNetAt(T->OnlyDay), Recurring ? F.MainStart : FestNetAt(T->OnlyDay + 1)))
			{
				Runs.push_back({E.Start + E.Duration, T->SeatValueCents > 0 ? std::max<int>(1, static_cast<int>(E.Pool / T->SeatValueCents)) : 0, Main});
			}
		}
		const auto Sum = [&Runs](double At, bool MainOnly) {
			int S = 0;
			for (const Run& R : Runs)
			{
				S += R.End <= At && (R.Main || !MainOnly) ? R.Seats : 0;
			}
			return S;
		};
		const double Done = std::floor(fest::LastEnd(F)) + FestNetDay;
		const double Moments[] = {Buildup, std::floor(F.MainStart), Done};
		std::string Why;
		for (double At : Moments)
		{
			const int Won = fest::SeatsWon(F, At);
			Why = Why.empty() && Won != Sum(At, true)
				? "SeatsWon " + std::to_string(Won) + " at " + net::TimeLabel(At) + " on " + FestNetDate(net::DayOf(At)) + ", the Main-ticket satellites' " +
					std::to_string(Sum(At, true))
				: Why;
		}
		const bool Ladder = fest::IsChampionship(F.Which) && F.Site != net::SiteId::RiverLine;
		Champs += Ladder ? 1 : 0;
		const int Won = fest::SeatsWon(F, Done);
		const int Every = Sum(Done, false);
		Seated += Won > 0 ? 1 : 0;
		if (Why.empty() && Ladder && (AllMain || Every <= Won))
		{
			Why = "the steps' tickets (" + std::to_string(Every - Won) + ") are not left out";
		}
		if (Why.empty() && !Ladder && !AllMain)
		{
			Why = "a satellite seats something other than the Main";
		}
		if (Why.empty() && !Runs.empty() && Won <= 0)
		{
			Why = "no seats after the edition";
		}
		if (!Why.empty() && Bad.empty())
		{
			Bad = ": " + F.Name + ": " + Why;
		}
		const bool Shown = F.Year == 2027 && (F.Which == fest::Brand::Pennant || F.Which == fest::Brand::Reign || F.Which == fest::Brand::RingRush) ||
			(F.Year == 2026 && F.Which == fest::Brand::Rcop);
		if (Shown)
		{
			Sample += "; " + F.Name + " " + std::to_string(Won) + (Ladder ? " (the steps' " + std::to_string(Every - Won) + " left out)" : std::string());
		}
	}
	// The session half (R-14): a seat won in the RiverLine trio registers the hero into the Main through SeatTicket.
	const fest::Festival* Ring = nullptr;
	for (const fest::Festival& F : fest::All())
	{
		Ring = !Ring && F.Which == fest::Brand::RingRush && F.Year == 2027 ? &F : Ring;
	}
	const net::Network& Home = net::Site(net::SiteId::RiverLine);
	const net::EventTemplate* Sat = Ring ? Home.FindTemplate(Ring->SeriesId + "-step") : nullptr;
	net::EventInstance MainE;
	std::string Session = "no Ring Rush 2027, its Main Satellite or its Main";
	if (Ring && Sat && Home.Next(Ring->MainId, FestNetAt(Ring->MainDay), MainE) && Home.TemplateOf(MainE).Id == Ring->MainId)
	{
		ss::SessionHooks Quiet;
		ss::Session S(Quiet, "festseats");
		S.LobbyMinutes = MainE.Start - 120.0 - FestNetAt(net::NightOneDay);
		S.BankrollCents = 0;
		++S.Life.Tickets[Sat->SeatTicket];
		const ss::LobbyEvent L = Home.Listing(MainE, nullptr, S.Unlocks());
		const bool Afford = S.CanAfford(L);
		const std::string Why = S.RegisterEvent(L);
		const bool In = Why.empty() && S.T && S.T->Spec.Id == MainE.Id && S.Life.TicketsFor(Sat->SeatTicket) == 0 && S.BankrollCents == 0;
		Session = Ring->MainDay == 165 && net::DayOf(S.WorldMinutes()) == 165 && Afford && In ? std::string()
			: "the hero with " + Sat->Id + "'s seat (" + Sat->SeatTicket + ") on day " + std::to_string(net::DayOf(S.WorldMinutes())) + ": " + (Afford ? "" : "can't afford; ") +
				(Why.empty() ? (In ? "" : "registered, but not into the Main with the ticket spent") : Why);
	}
	std::printf("  FestSeats: %d editions (%d championships with ladders, %d with seats)%s; the session half: %s\n", Editions, Champs, Seated, Sample.c_str(),
		Session.empty() ? "Ring Rush 2027's Main registered with the trio's seat on day 165" : Session.c_str());
	Expect(Editions > 100 && Champs == 28 && Seated > 100 && Bad.empty() && Session.empty(),
		"SeatsWon over every edition (" + std::to_string(Editions) + ", " + std::to_string(Seated) + " with seats) equals Σ max(1, Pool / SeatValue) over its Main-ticket "
			"satellites finished by then (at Buildup, the Main's start and after the last event); PENNANT's and REIGN's Step 1 and 2 tickets are left out, every other "
			"festival's satellites all seat its Main; a seat won in Ring Rush 2027's trio registers the hero into its Main through the session on day 165 (ticket spent, "
			"bankroll untouched)" + Sample + Bad + (Session.empty() ? std::string() : ": " + Session));
}

// ------------------------------------------------------------------ FestNamesScreened (F-net)

/** §2.11, §2.15.1: the festival names, titles, taglines, trophies (TrophyName: P1-P12 exactly), satellites, headliners and every
 *  string fest::Inventory lists (the six rival festivals' from FN-2b), every room word and the ten brands' keys (the rival names, §2.1.3: the
 *  ids follow the names) pass both trademark tiers with PA-1's entries; lowercase "push" is never flagged; no inventory string holds
 *  "Ignite Series" or "Ignite Championship". FN-4b: the inventory is complete (P1-P12, C1 and the 123 room words). Planted: a deny
 *  entry "writ" added to the source tier (scratch Trademarks.h); the PENNANT trophy answered for its title object; FN-4b: the room
 *  words left out of fest::Inventory; C1 listed without its year. */
void FestNamesScreenedNet()
{
	namespace fest = ss::fest;
	std::vector<std::string> Strings;
	for (const std::string& Entry : fest::Inventory())
	{
		Strings.push_back(Entry.substr(Entry.find('\t') + 1));
	}
	const size_t Inventory = Strings.size();
	for (int B = 0; B <= static_cast<int>(fest::Brand::Count); ++B)
	{
		Strings.push_back(fest::BrandKey(static_cast<fest::Brand>(B)));
		Strings.push_back(fest::TrophyName(static_cast<fest::Brand>(B), 0));
		Strings.push_back(fest::TrophyName(static_cast<fest::Brand>(B), 1));
	}
	for (fest::Brand B : {fest::Brand::Feign, fest::Brand::Push, fest::Brand::Writ, fest::Brand::Grit, fest::Brand::Pennant, fest::Brand::Reign})
	{
		for (const std::string& W : FestNetRooms(B))
		{
			Strings.push_back(W);
		}
	}
	std::string Hits;
	int Screened = 0;
	for (const std::string& S : Strings)
	{
		if (S.empty())
		{
			continue;
		}
		++Screened;
		const char* Hit = trademarks::Find(S, trademarks::kSourceScan);
		Hit = Hit ? Hit : trademarks::Find(S, trademarks::kCopyScan);
		const std::string Low = trademarks::Lower(S);
		const bool Series = Low.find("ignite series") != std::string::npos || Low.find("ignite championship") != std::string::npos;
		if ((Hit || Series) && Hits.size() < 300)
		{
			Hits += " \"" + S + "\" (" + (Hit ? Hit : "an Ignite series name") + ")";
		}
	}
	const bool Push = !trademarks::Find("push", trademarks::kSourceScan) && !trademarks::Find("push", trademarks::kCopyScan) &&
		!trademarks::Find("Last summer on the pier. Push.", trademarks::kSourceScan) && !trademarks::Find("Last summer on the pier. Push.", trademarks::kCopyScan);
	Expect(Inventory > 0 && Hits.empty() && Push,
		"the festival strings (" + std::to_string(Inventory) + " inventory entries, the ten brand keys, the trophies, the room words: " + std::to_string(Screened) +
			" screened) pass both trademark tiers; lowercase \"push\" is never flagged; no string names an Ignite series or championship" + Hits);

	// TrophyName (§2.11 P1-P12; LEAD_CALLS R-3): the trophies by brand, the championships' title objects at Tier 1, "" otherwise.
	const char* const Want[] = {"", "The RCOP Cup", "the ring", "the bracelet", "The Slam Plate", "The Funhouse Mirror", "The Big Red Button", "The Seal", "The Grindstone",
		"The Gold Pennant", "The Sceptre"};
	bool Trophies = std::string(fest::TrophyName(fest::Brand::Count)).empty() && std::string(fest::TrophyName(fest::Brand::Pennant, 1)) == "The Pennant Ribbon" &&
		std::string(fest::TrophyName(fest::Brand::Reign, 1)) == "The Spade Finial" && std::string(fest::TrophyName(fest::Brand::Rcop, 1)).empty() &&
		std::string(fest::TrophyName(fest::Brand::Feign, 1)).empty() && std::string(fest::TrophyName(fest::Brand::Pennant, 2)).empty();
	for (int B = 0; B < static_cast<int>(fest::Brand::Count); ++B)
	{
		Trophies = Trophies && std::string(fest::TrophyName(static_cast<fest::Brand>(B))) == Want[B];
	}
	Expect(Trophies, "TrophyName prints §2.11's P1-P12: The Seal, the ring, The Grindstone, the bracelet, The Slam Plate, The Big Red Button, The RCOP Cup, The Funhouse Mirror, "
					 "The Gold Pennant, The Sceptre; at Tier 1 The Pennant Ribbon and The Spade Finial; \"\" for None, RCOP's Tier 1 and any other tier");

	// FN-4b (§2.11; PLAN_FNET's Corrections item 1): the inventory is complete: it lists P1-P12 as TrophyName prints them, C1 (the crown
	// Honor's title) and every room word of the six plans (this file's lists, FestNetRooms), each a label.
	std::set<std::string> Listed;
	for (const std::string& Entry : fest::Inventory())
	{
		Listed.insert(Entry);
	}
	int Needed = 0;
	std::string Missing;
	const auto Need = [&Listed, &Needed, &Missing](const std::string& Text) {
		++Needed;
		if (!Listed.count("label\t" + Text) && Missing.size() < 300)
		{
			Missing += " \"" + Text + "\"";
		}
	};
	for (int B = 1; B < static_cast<int>(fest::Brand::Count); ++B)
	{
		Need(fest::TrophyName(static_cast<fest::Brand>(B)));
		if (*fest::TrophyName(static_cast<fest::Brand>(B), 1))
		{
			Need(fest::TrophyName(static_cast<fest::Brand>(B), 1));
		}
	}
	Need("{Festival} {Year} Player of the Series");
	for (fest::Brand B : {fest::Brand::Feign, fest::Brand::Push, fest::Brand::Writ, fest::Brand::Grit, fest::Brand::Pennant, fest::Brand::Reign})
	{
		for (const std::string& W : FestNetRooms(B))
		{
			Need(W);
		}
	}
	Expect(Needed == 10 + 2 + 1 + 123 && Missing.empty(),
		"fest::Inventory is complete: the ten trophies and the two title objects (P1-P12), C1 \"{Festival} {Year} Player of the Series\" and the six plans' 123 room "
		"words, each a label (" + std::to_string(Needed) + " wanted)" + (Missing.empty() ? std::string() : "; missing:" + Missing));
}

// ------------------------------------------------------------------ FestCopy (F-net)

/** FestChampWord (§2.11): "Ignite championship", "Ignite's championship", or the site name Ignite within two words of
 *  "championship". */
bool FestNetChampWord(const std::string& Text)
{
	const std::string Low = trademarks::Lower(Text);
	if (Low.find("ignite championship") != std::string::npos || Low.find("ignite's championship") != std::string::npos)
	{
		return true;
	}
	std::vector<std::string> Words;
	std::string W;
	for (const char C : Low + " ")
	{
		if ((C >= 'a' && C <= 'z') || (C >= '0' && C <= '9') || C == '\'')
		{
			W += C;
		}
		else if (!W.empty())
		{
			Words.push_back(W);
			W.clear();
		}
	}
	for (size_t I = 0; I < Words.size(); ++I)
	{
		if (Words[I].rfind("championship", 0) != 0)
		{
			continue;
		}
		for (size_t J = I >= 3 ? I - 3 : 0; J < std::min(Words.size(), I + 4); ++J)
		{
			if (J != I && Words[J].rfind("ignite", 0) == 0)
			{
				return true;
			}
		}
	}
	return false;
}

/** A filled template with §2.11's article rule: "{Festival}" after "the ", "The " or "next " goes through fest::NameAfterArticle. */
std::string FestNetFill(const std::string& Template, std::map<std::string, std::string> Vars)
{
	std::string Out = Template;
	for (const char* Art : {"the {Festival}", "The {Festival}", "next {Festival}"})
	{
		const std::string A = Art;
		const size_t Brace = A.find('{');
		for (size_t At = Out.find(A); At != std::string::npos; At = Out.find(A, At + 1))
		{
			Out.replace(At + Brace, std::string("{Festival}").size(), ss::fest::NameAfterArticle(Vars["Festival"]));
		}
	}
	return copylint::Fill(Out, Vars);
}

/** DESIGN §2.6's avoided words (§1.2 item 5; FestCopyWords: festival strings only, so the sites' older copy keeps "The Spark"), with
 *  "marquee" and "pavilion" (§2.11: never in player copy). */
const char* const FestNetAvoided[] = {"spark", "flame", "blaze", "wildfire", "ember", "crest", "heat", "cold", "stamina", "momentum", "energy", "board", "watchlist",
	"headliners", "beef", "hall", "class", "boom", "millions", "jackpot", "run good", "supernova", "premier", "marquee", "pavilion"};

/** §2.11's "championship" rule on one printed string (FN-4b): an Ignite string (one that names Ignite, or a row of Ignite's) holds no
 *  "championship" at all; a RunItTwice string (one that names RunItTwice, or a row of RunItTwice's) holds it only as "the club's
 *  championship". "" when the string keeps the rule, else the reason. */
std::string FestNetChampSite(const std::string& Text, bool IgniteRow, bool RitRow)
{
	const std::string Low = trademarks::Lower(Text);
	const std::string Club = "the club's ";
	int Count = 0;
	int Clubs = 0;
	for (size_t At = Low.find("championship"); At != std::string::npos; At = Low.find("championship", At + 1))
	{
		++Count;
		Clubs += At >= Club.size() && Low.compare(At - Club.size(), Club.size(), Club) == 0 ? 1 : 0;
	}
	if (Count == 0)
	{
		return std::string();
	}
	if (IgniteRow || trademarks::HasWord(Low, "ignite"))
	{
		return " an Ignite string holds \"championship\"";
	}
	if ((RitRow || trademarks::HasWord(Low, "runittwice")) && Clubs < Count)
	{
		return " a RunItTwice string holds \"championship\" outside \"the club's championship\"";
	}
	return std::string();
}

/** A shared festival template (FN-4b, fix round 1): one that names its festival through {Festival}, {FESTIVAL}, {Short} or {SHORT},
 *  so it prints for any festival's edition, Ignite's PENNANT and RunItTwice's REIGN among them. */
bool FestNetHoldsFestival(const std::string& Template)
{
	for (const char* Var : {"{Festival}", "{FESTIVAL}", "{Short}", "{SHORT}"})
	{
		if (Template.find(Var) != std::string::npos)
		{
			return true;
		}
	}
	return false;
}

/** FestCopy's rules for one inventory string (FN-2a's, shared by every core inventory from FN-4b): its kind's length cap and stop rule
 *  (label and subject: no closing stop, 200; sentence: a stop, 600; text: 84; headtitle: 120; headbody: 200 and a stop; journey: 120
 *  and a stop), filled from the samples (Filled gets the fill) through CopyLint, both trademark tiers, U1, FestChampWord, §2.11's
 *  championship rule by site (on the Ignite fill and on a RunItTwice fill; a shared festival template that is no row of one site's,
 *  OneSite false, is an Ignite string on its PENNANT fill and a RunItTwice string on its REIGN fill), no "championship" in a headline
 *  title (HeadTitle: a head:*:title row of the copy table or a "headtitle" entry, §2.11's FestCopy row), the glyph rule (ASCII and
 *  · — – … é ’), the article rule (no "the The" with The Championship Online 2027) and the year (a next year's {Date} prints it); a
 *  festival string also the design words, X1 ("players"), DESIGN §2.6's avoided words with "marquee" and "pavilion", and no *COOP
 *  form (§1.2 items 1, 3-5: the sites' older copy keeps its "The Spark", "tier" and "players"). "" when it passes, else the reasons. */
std::string FestNetCopyWhy(const std::string& Kind, const std::string& Text, const std::map<std::string, std::string>& Vars, bool Festival, bool IgniteRow,
	bool RitRow, bool OneSite, bool HeadTitle, const std::string& NextYear, std::string& Filled)
{
	size_t Cap = copylint::LetterBody;
	bool Stop = true;
	if (Kind == "label" || Kind == "subject")
	{
		Cap = copylint::MailSubject;
		Stop = false;
	}
	else if (Kind == "headtitle")
	{
		Cap = copylint::kHeadTitleMax;
		Stop = false;
	}
	else if (Kind == "text")
	{
		Cap = copylint::TableTalk;
		Stop = false;
	}
	else if (Kind == "headbody")
	{
		Cap = copylint::kHeadBodyMax;
	}
	else if (Kind == "journey")
	{
		Cap = copylint::kJourneyMax;
	}
	const bool Known = Kind == "label" || Kind == "sentence" || Kind == "text" || Kind == "subject" || Kind == "letter" || Kind == "headtitle" || Kind == "headbody" ||
		Kind == "journey";
	Filled = FestNetFill(Text, Vars);
	const bool Ends = copylint::EndsWith(Filled, ".") || copylint::EndsWith(Filled, "!") || copylint::EndsWith(Filled, "?");
	std::string Why = Known ? std::string() : " an unknown kind \"" + Kind + "\"";
	const std::string Lint = copylint::Check(Filled + (Stop || Ends ? "" : "."), Cap + (Stop || Ends ? 0 : 1), {});
	Why += Lint.empty() ? std::string() : " " + Lint;
	Why += !Stop && Ends && Kind != "text" ? std::string(" a label ends in a stop") : std::string();
	for (const char* Hit : {trademarks::Find(Filled, trademarks::kSourceScan), trademarks::Find(Filled, trademarks::kCopyScan),
			 Festival ? trademarks::Find(Filled, trademarks::kDesignWords) : nullptr})
	{
		Why += Hit ? std::string(" trademark or design word: ") + Hit : std::string();
	}
	const std::string Low = trademarks::Lower(Filled);
	for (const char* W : {"ai", "bot", "bots", "autoplay", "autopilot", "players"})
	{
		const bool Rule = Festival || std::string(W) != "players";
		Why += Rule && trademarks::HasWord(Low, W) ? std::string(" U1/X1: ") + W : std::string();
	}
	for (const char* W : FestNetAvoided)
	{
		Why += Festival && trademarks::HasWord(Low, W) ? std::string(" avoided: ") + W : std::string();
	}
	Why += Festival && Low.find("coop") != std::string::npos ? " a *COOP form" : "";
	// The championship rule by site: Vars is the PENNANT fill (Ignite's); a shared festival template is an Ignite string there and a
	// RunItTwice string on its REIGN fill.
	const bool Shared = !OneSite && FestNetHoldsFestival(Text);
	Why += FestNetChampWord(Filled) ? " FestChampWord" : "";
	Why += FestNetChampSite(Filled, IgniteRow || Shared, RitRow);
	std::map<std::string, std::string> Rit = Vars;
	Rit["Site"] = "RunItTwice";
	for (const char* Var : {"Festival", "FESTIVAL", "Short", "SHORT"})
	{
		Rit[Var] = "REIGN";
	}
	const std::string RitFilled = FestNetFill(Text, Rit);
	Why += FestNetChampWord(RitFilled) ? " FestChampWord (RunItTwice fill)" : "";
	Why += FestNetChampSite(RitFilled, IgniteRow, RitRow || Shared);
	// No headline title holds "championship" (§2.11), in its template or either fill.
	const bool HeadChamp = trademarks::Lower(Text).find("championship") != std::string::npos ||
		trademarks::Lower(Filled).find("championship") != std::string::npos || trademarks::Lower(RitFilled).find("championship") != std::string::npos;
	Why += HeadTitle && HeadChamp ? " a headline title holds \"championship\"" : "";
	// The glyph rule: ASCII plus · — – … é ’ (no ×).
	for (size_t K = 0; K < Filled.size(); ++K)
	{
		if (static_cast<unsigned char>(Filled[K]) < 0x80)
		{
			continue;
		}
		const std::string G2 = Filled.substr(K, 2);
		const std::string G3 = Filled.substr(K, 3);
		if (G2 == "\xC2\xB7" || G2 == "\xC3\xA9")
		{
			++K;
		}
		else if (G3 == "\xE2\x80\x94" || G3 == "\xE2\x80\x93" || G3 == "\xE2\x80\xA6" || G3 == "\xE2\x80\x99")
		{
			K += 2;
		}
		else
		{
			Why += " a glyph outside the rule";
			break;
		}
	}
	// The article rule (no "the The") and the year (a next year's date prints it).
	std::map<std::string, std::string> Again = Vars;
	Again["Festival"] = "The Championship Online 2027";
	Again["Date"] = NextYear;
	const std::string Refilled = trademarks::Lower(FestNetFill(Text, Again));
	Why += Refilled.find("the the") != std::string::npos ? " \"the The\" with The Championship Online" : "";
	Why += Text.find("{Date}") != std::string::npos && Refilled.find(", 2027") == std::string::npos ? " a next year's {Date} without its year" : "";
	return Why;
}

/** F-world's festival rows in the sites' copy table (FW-1's key list, the contract F-net's News and F-world's headlines read; §2.11). */
const char* const FestNetSiteKeys[] = {"news:fest:announce:title", "news:fest:announce", "news:fest:announce:first", "news:fest:announce:champ:ignite",
	"news:fest:announce:champ:runittwice", "news:fest:announce:rl", "news:fest:open:title", "news:fest:open", "news:fest:open:grit", "news:fest:record:title",
	"news:fest:record:ever", "news:fest:record:year", "head:fest:main:title", "head:fest:main", "head:fest:title:title", "head:fest:title", "head:fest:crown:title",
	"head:fest:crown", "tag:fest:crown", "tag:fest:festival", "invite:ignite:fest:subject", "invite:ignite:fest", "invite:runittwice:fest:subject",
	"invite:runittwice:fest", "letter:fest:crown:subject", "letter:fest:crown", "ledger:festcredit",
	// G1 polish (g1p-copy): the crown line's singular (item 11) and the Pass letters' first-day and later forms (item 10).
	"head:fest:crown:one", "invite:ignite:fest:today", "invite:ignite:fest:opened", "invite:runittwice:fest:today", "invite:runittwice:fest:opened"};

/**
 * FestCountRows (G1 polish, g1p-copy; the lead's PIE item 11, "6,350 points from 1 cashes."): every festival row of the sites' copy
 * table that writes a count before its plural noun ("{Cashes} cashes", "{Points} points", "{Events} events", "{Entries} runners") is
 * one of the nine known, each safe at one: the crown's {Cashes} has its singular twin head:fest:crown:one ("from 1 cash.", no
 * {Cashes}, picked by World::Headline at one); {Points} is a race total, and one cash scores at least 14 (net::Points' smallest,
 * found here over every field to 20,000); {Events} is a series' event count, at least 2 for every edition fest::All() holds
 * (2026-2028); a record field's {Entries} is judged against every field since the site's launch weeks before (fest::RecordOf), never
 * 1. A new count row fails here until it is classified. The festival UI's own counts (U10's "1 event · 1 cash", "1 SEAT WON BY
 * SATELLITE", "1 entry", "1 regular won their seat by satellite.", "First title on record.") are singular-aware in
 * UI/RiverLineFestival.cpp and UI/RiverLineNet.cpp, and WHO'S IN's "{N} {Festival} titles" is written only from two (G/WorldFest.cpp).
 * Planted bugs: head:fest:crown:one deleted; a festival row "{N} seats" added.
 */
void FestCountRowsNet()
{
	namespace fest = ss::fest;
	namespace net = ss::net;
	// The copy's number fills (a count, a total, a place): a word after one of these that ends in "s" is a counted plural noun.
	const std::set<std::string> Numbers = {"Cashes", "Points", "Events", "Entries", "Days", "Nights", "N", "K", "Rank", "Scorers", "Seats", "Left", "Count", "Titles"};
	const std::set<std::pair<std::string, std::string>> Known = {{"head:fest:crown", "Cashes"}, {"head:fest:crown", "Points"}, {"head:fest:crown:one", "Points"},
		{"letter:fest:crown", "Points"}, {"news:fest:announce", "Events"}, {"news:fest:announce:rl", "Events"}, {"news:fest:open", "Events"},
		{"news:fest:record:ever", "Entries"}, {"news:fest:record:year", "Entries"}};
	std::set<std::pair<std::string, std::string>> Found;
	for (const char* Key : FestNetSiteKeys)
	{
		const std::string Text = ss::site::CopyText(Key);
		for (size_t Open = Text.find('{'); Open != std::string::npos; Open = Text.find('{', Open + 1))
		{
			const size_t Close = Text.find('}', Open);
			const std::string Name = Close == std::string::npos ? std::string() : Text.substr(Open + 1, Close - Open - 1);
			if (!Numbers.count(Name) || Close + 2 >= Text.size() || Text[Close + 1] != ' ')
			{
				continue;
			}
			size_t End = Close + 2;
			while (End < Text.size() && Text[End] >= 'a' && Text[End] <= 'z')
			{
				++End;
			}
			const std::string Noun = Text.substr(Close + 2, End - Close - 2);
			if (Noun.size() >= 2 && Noun.back() == 's')
			{
				Found.insert({Key, Name});
			}
		}
	}
	const std::string One = ss::site::CopyText("head:fest:crown:one");
	const bool Twin = One.find("points from 1 cash.") != std::string::npos && One.find("{Cashes}") == std::string::npos && One.find("cashes") == std::string::npos;
	int Editions = 0, Fewest = 1 << 30;
	for (const fest::Festival& F : fest::All())
	{
		const net::SeriesInfo* Sr = net::Site(F.Site).FindSeries(F.SeriesId);
		Fewest = std::min(Fewest, Sr ? Sr->Events : 0);
		++Editions;
	}
	// One cash's fewest points: for each field, its last paid place (net::Points pays round(15%), at least one) at a free buy-in.
	double Least = 1e9;
	for (int Entries = 2; Entries <= 20000; ++Entries)
	{
		const int Paid = std::max(1, static_cast<int>(std::lround(static_cast<double>(Entries) * 0.15)));
		Least = std::min(Least, net::Points(Paid, Entries, 0));
	}
	std::string Odd;
	for (const auto& P : Found)
	{
		Odd += Known.count(P) ? std::string() : " new: " + P.first + " {" + P.second + "}";
	}
	for (const auto& P : Known)
	{
		Odd += Found.count(P) ? std::string() : " gone: " + P.first + " {" + P.second + "}";
	}
	std::printf("  FestCountRows: %zu count rows in the festival copy, the crown's singular twin \"%s\", the fewest events of %d editions %d, one cash's fewest points %.0f\n",
		Found.size(), One.c_str(), Editions, Fewest, Least);
	Expect(Odd.empty() && Twin && Editions > 0 && Fewest >= 2 && Least >= 2.0,
		"FestCountRows: the festival copy's count rows are the nine known, the crown's {Cashes} with its singular twin (\"from 1 cash.\"), every edition's {Events} at "
		"least 2 (" + std::to_string(Fewest) + ") and one cash's {Points} at least 2 (" + std::to_string(static_cast<int>(Least)) + ")" + Odd);
}

/** §1.2 items 2-5, 9-10, 12-13, §2.11: every core inventory (FN-4b, PLAN_FNET's Corrections item 1): fest::Inventory (F-net's names,
 *  Shorts, Titles, taglines, event names, blurbs, seats texts, the trophies, C1 and the room words), fest::TextInventory (F-world's
 *  Dee lines, WHO'S IN's reasons, the story line and Rerun's titles) and the sites' copy table (site::Copy's sentences and
 *  site::CopyLabels' titles, subjects and labels; F-world's 27 festival rows found by FW-1's key list, every key answering), each
 *  string by its kind through FestNetCopyWhy (CopyLint, both trademark tiers, U1, FestChampWord, the championship rule by site, the
 *  glyphs, the article and the year for every string; the design words, X1, the avoided words and *COOP for the festival strings),
 *  filled from the samples (copylint's, sites_main's SitesCopy values and fworld_to_fnet_1's); no rival festival name, Short, Title,
 *  tagline, event name, blurb, seats text, trophy or headline title (a "headtitle" entry, or a copy-table title row keyed head:*:title)
 *  holds "championship"; no Ignite string on Ignite's site (its series, templates and page words) holds it at all, nor a shared
 *  festival template ({Festival}, {FESTIVAL}, {Short} or {SHORT}, no row of one site's) on its PENNANT fill, and RunItTwice's (and a
 *  shared template on its REIGN fill) only as "the club's championship" in body copy, never in a name; every
 *  {Festival} template filled again with "The Championship Online 2027" holds no "the The" and every {Date} a next year's date with
 *  its year; NameAfterArticle and DateInSentence answer §1.2 item 13's forms. Planted: "players" in a count; "GRIT Championship" as a
 *  Title; "Ignite's championship" in a tagline; NameAfterArticle and DateInSentence left at H0's stubs; PUSH's tagline without its
 *  closing stop (FN-2b); FN-4b: "players" in the record News sentence; "PUSH Championship" as a Title; "Ignite's championship" in
 *  Ignite's championship announcement (News); "RunItTwice's championship" in RunItTwice's; "marquee" in a Dee text; "championship"
 *  in an Ignite weekly's blurb; FN-4b's fix round 1: "Championship" in head:fest:main:title, "the club's championship" in
 *  head:fest:crown:title (the headline rule alone) and "championship" in Dee's shared T9 (the PENNANT fill's rule alone). */
void FestCopyNet()
{
	namespace fest = ss::fest;
	namespace net = ss::net;
	std::map<std::string, std::string> Vars = copylint::Samples();
	const std::map<std::string, std::string> Extra = {{"Site", "Ignite"}, {"Month", "November"}, {"Tier", "Mini"}, {"Gtd", "$25K"}, {"Entries", "410"}, {"Events", "40"},
		{"Days", "17"}, {"Series", "RunItTwice Open 2026"}, {"Festival", "PENNANT"}, {"Date", "Mar 30"}, {"MainBuyIn", "$215"}, {"Added", "$10,000"}, {"MicroBuyIn", "$2.20"},
		{"MiniBuyIn", "$22"}, {"Step1", "$1.10"}, {"MainDate", "Apr 11"}, {"Nights", "14"}, {"Show1", "7 PM"}, {"Show2", "10 PM"}, {"Prize", "$41,280"},
		{"Trophy", "The Gold Pennant"}, {"Points", "1,284"}, {"Cashes", "9"}, {"Year", "2027"}, {"K", "2"}, {"MARK", "FINALIST"}, {"Rank", "14"}, {"Scorers", "608"},
		{"SatBuyIn", "$22"}, {"TROPHY", "THE GOLD PENNANT"}, {"BuyIn", "$2.20"}, {"N", "3"}, {"A", "PENNANT"}, {"B", "REIGN"}, {"C", "RCOP"}, {"MonthA", "April"},
		{"MonthB", "August"}, {"FESTIVAL", "PENNANT"}, {"Short", "PENNANT"}, {"SHORT", "PENNANT"}};
	for (const auto& Kv : Extra)
	{
		Vars[Kv.first] = Kv.second;
	}
	// FN-4b: the sites' copy table's other variables, for its older rows (SitesCopy's samples, tests/sites_main.cpp).
	Vars.insert({{"Rate", "14%"}, {"Rake", "$212.40"}, {"Released", "$35.00"}, {"Min", "$10.00"}, {"Fee", "$1.50"}, {"Why", "A series title gets noticed around here."},
		{"Money", "$197.00 is on its way from Ignite (Mon to Wed)."}, {"Buyin", "$530"}, {"Window", "Mon to Wed"}, {"Method", "card"}});
	// Today (Nov 12, 2026's world day) and a day in the next world year, for the second fill's {Date}.
	const int Today = ss::world::DayOn(2026, 11, 12);
	const std::string NextYear = fest::DateInSentence(ss::world::DayOn(2027, 3, 30), Today);

	// The core inventories, each entry with its source, kind and rules (FN-4b).
	// OneSite: a copy-table row of one site's (its key names Ignite, RunItTwice or RiverLine), never a shared festival template;
	// HeadTitle: a headline title (a "headtitle" entry, or a copy-table title row whose key starts with head: and ends with :title).
	struct FestNetCore
	{
		const char* Source;
		std::string Kind, Text;
		bool Festival, IgniteRow, RitRow, OneSite, HeadTitle;
	};
	std::vector<FestNetCore> Core;
	for (const char* Source : {"fest::Inventory", "fest::TextInventory"})
	{
		for (const std::string& Entry : std::string(Source) == "fest::Inventory" ? fest::Inventory() : fest::TextInventory())
		{
			const size_t Tab = Entry.find('\t');
			const std::string Kind = Tab == std::string::npos ? std::string() : Entry.substr(0, Tab);
			Core.push_back({Source, Kind, Tab == std::string::npos ? Entry : Entry.substr(Tab + 1), true, false, false, false, Kind == "headtitle"});
		}
	}
	std::map<std::string, std::string> FestRows; // text -> its key
	std::string Unkeyed;
	int Keys = 0;
	std::set<std::string> KeyWords;
	for (const char* Key : FestNetSiteKeys)
	{
		const std::string Text = ss::site::CopyText(Key);
		Keys += Text.empty() ? 0 : 1;
		Unkeyed += Text.empty() ? std::string(" ") + Key : std::string();
		if (!Text.empty())
		{
			FestRows[Text] = Key;
		}
		std::string Word;
		for (const char C : std::string(Key) + ":")
		{
			if (C != ':')
			{
				Word += C;
				continue;
			}
			KeyWords.insert(Word);
			Word.clear();
		}
	}
	// The headline titles of the copy table (it answers by key alone, site::CopyText): FW-1's head:*:title rows and any other
	// head:<a>:title or head:<a>:<b>:title row the key list's words reach, each a title row (CopyLabels): head:fest:title is the
	// title headline's body, a sentence.
	std::set<std::string> HeadKeys;
	for (const char* Key : FestNetSiteKeys)
	{
		const std::string K = Key;
		if (K.rfind("head:", 0) == 0 && copylint::EndsWith(K, ":title"))
		{
			HeadKeys.insert(K);
		}
	}
	for (const std::string& A : KeyWords)
	{
		HeadKeys.insert("head:" + A + ":title");
		for (const std::string& B : KeyWords)
		{
			HeadKeys.insert("head:" + A + ":" + B + ":title");
		}
	}
	std::set<std::string> HeadTexts;
	for (const std::string& K : HeadKeys)
	{
		const std::string Text = ss::site::CopyText(K);
		if (!Text.empty())
		{
			HeadTexts.insert(Text);
		}
	}
	int SiteFest = 0;
	for (const char* Source : {"site::Copy", "site::CopyLabels"})
	{
		const bool Sentences = std::string(Source) == "site::Copy";
		for (const std::string& S : Sentences ? ss::site::Copy() : ss::site::CopyLabels())
		{
			const auto It = FestRows.find(S);
			const std::string Key = It == FestRows.end() ? std::string() : It->second;
			SiteFest += Key.empty() ? 0 : 1;
			const bool Ig = Key.find(":ignite") != std::string::npos;
			const bool Rit = Key.find(":runittwice") != std::string::npos;
			const bool Rl = copylint::EndsWith(Key, ":rl") || Key.find(":rl:") != std::string::npos;
			Core.push_back({Source, Sentences ? "sentence" : "label", S, !Key.empty(), Ig, Rit, Ig || Rit || Rl, !Sentences && HeadTexts.count(S) > 0});
		}
	}
	std::map<std::string, int> Entries;
	int Problems = 0;
	int Heads = 0;
	std::string First;
	for (const FestNetCore& C : Core)
	{
		++Entries[C.Source];
		Heads += C.HeadTitle ? 1 : 0;
		std::string Filled;
		const std::string Why = FestNetCopyWhy(C.Kind, C.Text, Vars, C.Festival, C.IgniteRow, C.RitRow, C.OneSite, C.HeadTitle, NextYear, Filled);
		if (!Why.empty())
		{
			++Problems;
			First = First.empty() ? ": \"" + Filled + "\" [" + C.Source + ", " + C.Kind + "]" + Why : First;
			std::printf("COPY (festivals) \"%s\" [%s, %s]:%s\n", Filled.c_str(), C.Source, C.Kind.c_str(), Why.c_str());
		}
	}
	const int Ours = Entries["fest::Inventory"];
	std::printf("  FestCopy: fest::Inventory %d, fest::TextInventory %d, site::Copy %d, site::CopyLabels %d entries (%d festival rows by FW-1's keys, %d "
				"headline titles)\n",
		Ours, Entries["fest::TextInventory"], Entries["site::Copy"], Entries["site::CopyLabels"], SiteFest, Heads);
	Expect(Ours > 0 && Entries["fest::TextInventory"] > 0 && Entries["site::Copy"] > 0 && Entries["site::CopyLabels"] > 0 && Problems == 0,
		"every core inventory passes the copy lint by kind, both trademark tiers, U1, FestChampWord, the championship rule by site (a shared festival template on "
		"its PENNANT and REIGN fills), no \"championship\" in a headline title (" + std::to_string(Heads) + "), the glyph rule and the article and year rules, "
		"and its festival strings the design words, X1 and the avoided words (fest::Inventory " + std::to_string(Ours) + ", fest::TextInventory " +
			std::to_string(Entries["fest::TextInventory"]) + ", site::Copy " + std::to_string(Entries["site::Copy"]) + ", site::CopyLabels " +
			std::to_string(Entries["site::CopyLabels"]) + "; " + std::to_string(Problems) + " problems)" + First);
	Expect(Keys == static_cast<int>(sizeof(FestNetSiteKeys) / sizeof(FestNetSiteKeys[0])) && SiteFest == Keys,
		"FW-1's " + std::to_string(Keys) + " festival keys each answer site::CopyText and their rows are in site::Copy or site::CopyLabels (" + std::to_string(SiteFest) +
			" found)" + (Unkeyed.empty() ? std::string() : ": missing" + Unkeyed));

	// FestChampWord over every rival festival string the tables hold (FN-2a: FEIGN's and GRIT's): no "championship" at all.
	int Rival = 0;
	std::string Champ;
	for (const fest::Festival* F : FestNetRivalEditions())
	{
		const net::Network& N = net::Site(F->Site);
		const net::SeriesInfo* Sr = N.FindSeries(F->SeriesId);
		std::vector<std::string> Texts = {fest::TrophyName(F->Which), fest::TrophyName(F->Which, 1)};
		if (Sr)
		{
			Texts.insert(Texts.end(), {Sr->Name, Sr->Short, Sr->Title, Sr->Tagline});
		}
		for (const net::EventTemplate* T : FestNetEditionTemplates(*F))
		{
			Texts.insert(Texts.end(), {T->Name, T->Blurb, T->Seats});
		}
		for (const std::string& X : Texts)
		{
			++Rival;
			if ((trademarks::Lower(X).find("championship") != std::string::npos || FestNetChampWord(X)) && Champ.empty())
			{
				Champ = ": \"" + X + "\" (" + F->Name + ")";
			}
		}
	}
	Expect(Rival > 0 && Champ.empty(), "no rival festival name, Short, Title, tagline, event name, blurb, seats text or trophy holds \"championship\" (" + std::to_string(Rival) +
										   " strings; FestChampWord, §2.11)" + Champ);

	// FN-4b (§2.11): no Ignite string holds "championship" at all, and RunItTwice's only as "the club's championship" in body copy
	// (taglines, blurbs), never in a name, Short, Title or seats text: every series and template on both sites, festival or not, and
	// their pages' own words (the site's name, tagline, rep and team).
	int SiteStrings = 0;
	std::string SiteChamp;
	for (const net::SiteId S : {net::SiteId::Ignite, net::SiteId::RunItTwice})
	{
		const bool Ig = S == net::SiteId::Ignite;
		const net::SiteDef& D = net::Def(S);
		std::vector<std::pair<std::string, bool>> Texts = {{D.Name, false}, {D.Tagline, true}, {D.Rep, false}, {D.RepTitle, false}, {D.TeamName, false}};
		for (const net::SeriesInfo& Sr : net::Site(S).Series())
		{
			Texts.insert(Texts.end(), {{Sr.Name, false}, {Sr.Short, false}, {Sr.Title, false}, {Sr.Tagline, true}});
		}
		for (const net::EventTemplate& T : net::Site(S).Templates())
		{
			Texts.insert(Texts.end(), {{T.Name, false}, {T.Seats, false}, {T.Blurb, true}});
		}
		for (const std::pair<std::string, bool>& X : Texts)
		{
			if (X.first.empty())
			{
				continue;
			}
			++SiteStrings;
			const bool Holds = trademarks::Lower(X.first).find("championship") != std::string::npos;
			const std::string Why = !Holds ? std::string() : !X.second ? std::string(" a name holds \"championship\"") : FestNetChampSite(X.first, Ig, !Ig);
			if (!Why.empty() && SiteChamp.empty())
			{
				SiteChamp = ": \"" + X.first + "\" (" + D.Name + ")" + Why;
			}
		}
	}
	Expect(SiteStrings > 0 && SiteChamp.empty(),
		"no Ignite string holds \"championship\" and RunItTwice's hold it only as \"the club's championship\" in body copy (" + std::to_string(SiteStrings) +
			" series, template and page strings on the two sites; §2.11)" + SiteChamp);

	// FN-4a (§2.2.2, X2): "festival" names the tier only. The five renamed RiverLine seasonal series print their new names, each a series
	// in the calendar and an entry of fest::Inventory, and no series on any site that is not a festival edition has "festival" in its name.
	std::set<std::string> Listed;
	for (const std::string& Entry : fest::Inventory())
	{
		Listed.insert(Entry);
	}
	std::set<std::string> RiverLineNames;
	std::string Tier;
	int Series = 0;
	for (int S = 0; S < static_cast<int>(net::SiteId::Count); ++S)
	{
		for (const net::SeriesInfo& Sr : net::Site(static_cast<net::SiteId>(S)).Series())
		{
			++Series;
			if (S == static_cast<int>(net::SiteId::RiverLine))
			{
				RiverLineNames.insert(Sr.Name);
			}
			if (!Sr.Festival && trademarks::HasWord(trademarks::Lower(Sr.Name), "festival") && Tier.empty())
			{
				Tier = ": \"" + Sr.Name + "\" (" + Sr.Id + ")";
			}
		}
	}
	std::string Renamed;
	for (const char* Name : {"Polar Night Series", "Deep Freeze Classic", "Spring Fever Series", "Greenlight Open", "Fireside Classic"})
	{
		Renamed += RiverLineNames.count(Name) && Listed.count(std::string("label\t") + Name) ? "" : std::string(" missing: ") + Name;
	}
	Expect(Series > 0 && Tier.empty() && Renamed.empty(),
		"X2: the five RiverLine seasonal series renamed off \"Festival\" (Polar Night Series, Deep Freeze Classic, Spring Fever Series, Greenlight Open, Fireside "
		"Classic) are in the calendar and fest::Inventory, and none of " + std::to_string(Series) + " series that is not a festival holds the word" + Tier + Renamed);

	// The helpers (§1.2 item 13; H0's declarations, F-net's bodies): the article and the date forms, and the fill that uses them.
	const bool Article = fest::NameAfterArticle("The Championship Online 2027") == "Championship Online 2027" && fest::NameAfterArticle("FEIGN 2026") == "FEIGN 2026" &&
		fest::NameAfterArticle("RCOP 2026") == "RCOP 2026" && fest::NameAfterArticle("Theatre Royal") == "Theatre Royal" && fest::NameAfterArticle("The") == "The" &&
		fest::NameAfterArticle("") == "";
	std::map<std::string, std::string> Tco = Vars;
	Tco["Festival"] = "The Championship Online 2027";
	const std::string Final = FestNetFill("Final table of the {Festival} Main Event. The {Festival} returns next {Festival} season.", Tco);
	const bool Fill = Final == "Final table of the Championship Online 2027 Main Event. The Championship Online 2027 returns next Championship Online 2027 season.";
	const bool Dates = fest::DateInSentence(ss::world::DayOn(2026, 11, 30), Today) == "Nov 30" && fest::DateInSentence(1, Today) == "Oct 6" &&
		fest::DateInSentence(ss::world::DayOn(2027, 3, 30), Today) == "Mar 30, 2027" && fest::DateInSentence(ss::world::DayOn(2026, 1, 2), Today) == "Jan 2" && fest::DateInSentence(ss::world::DayOn(2025, 12, 31), Today) == "Dec 31, 2025" &&
		fest::DateInSentence(ss::world::DayOn(2027, 12, 31), ss::world::DayOn(2027, 1, 1)) == "Dec 31" &&
		fest::DateInSentence(ss::world::DayOn(2028, 1, 1), ss::world::DayOn(2027, 12, 31)) == "Jan 1, 2028" &&
		fest::DateInSentence(ss::world::DayOn(2028, 2, 29), ss::world::DayOn(2028, 7, 1)) == "Feb 29";
	Expect(Article && Fill && Dates,
		"NameAfterArticle drops a leading \"The \" only (\"The Championship Online 2027\" -> \"Championship Online 2027\"), so \"the {Festival}\" never prints \"the The\"; "
		"DateInSentence prints \"Nov 30\" inside Today's world year and \"Mar 30, 2027\" outside it (" + Final + ")");
}

// ------------------------------------------------------------------ FestRcopKept (F-net)

/** §1.2 item 1, §2.2.2, §2.11, §2.15.1 (the user's decisions of 2026-11-09 and 2026-11-10; FN-4b): RCOP's existing strings are never
 *  renamed and the game still prints them ("RCOP 2026", "RCOP", "RCOP #150: $5,250 Main Event", "RCOP '27 #132", "Step 1 · RCOP Main"
 *  ... "Step 4 · RCOP Main", "RCOP Main Mega Satellite", the tagline "The RiverLine Championship of Online Poker.", which RCOP 2026's
 *  extends with its guarantee and RCOP 2027's with its events and guarantee), and they pass both trademark tiers, FestCopyWords (the avoided words) and the
 *  themed-name lint (the design words); no RiverLine tagline (every year's) or post (the News to Night One's day 2 and to Nov 12, 2027,
 *  RCOP 2027 done) types "$100,000,000"; the five seasonal renames pass both tiers; the six tournament names the game already prints
 *  are never renamed (Sunday Storm, RiverLine Millions and Friday Night Fight on RiverLine, Flip & Run and Flip & Run Bounty on
 *  Ignite, The Closer on RunItTwice, each still a template's name; FEIGN's No 1 a Flip & Run) and pass both tiers in every form the
 *  game prints ("Platinum Pass", "Platinum Passes", "Friday Night Fight", "Flip & Run", "Flip & Runs", "FLIP & RUN", "Flip & Run
 *  Bounty", "The Closer", "Sunday Storm", "RiverLine Millions"; the two "Platinum Pass" forms as strings, FD-4). Planted: "championship
 *  of online poker" added to the source tier; "flip & run" added to the copy tier (scratch Trademarks.h). */
void FestRcopKeptNet()
{
	namespace net = ss::net;
	const net::Network& Rl = net::Site(net::SiteId::RiverLine);
	const std::string Tagline = "The RiverLine Championship of Online Poker.";
	const net::SeriesInfo* Rc = Rl.FindSeries("rcop");
	const net::SeriesInfo* Rc27 = Rl.FindSeries("rcop27");
	const auto NameOf = [&Rl](const char* Id) {
		const net::EventTemplate* T = Rl.FindTemplate(Id);
		return T ? T->Name : std::string();
	};
	std::string No132;
	for (const net::EventTemplate& T : Rl.Templates())
	{
		No132 = T.Series == "rcop27" && T.Name.rfind("RCOP '27 #132: ", 0) == 0 ? T.Name : No132;
	}
	const std::string Rung = " \xC2\xB7 RCOP Main";
	std::string Moved;
	const auto As = [&Moved](const std::string& Got, const std::string& Want, bool Prefix = false) {
		const bool Same = Prefix ? Got.rfind(Want, 0) == 0 : Got == Want;
		Moved += Same ? std::string() : " \"" + Got + "\" for \"" + Want + "\"";
	};
	As(Rc ? Rc->Name : std::string(), "RCOP 2026");
	As(Rc ? Rc->Short : std::string(), "RCOP");
	As(Rc ? Rc->Tagline : std::string(), Tagline + " ", true);
	As(Rc27 ? Rc27->Tagline : std::string(), Tagline + " ", true);
	As(NameOf("rcop-main"), "RCOP #150: $5,250 Main Event");
	As(No132, "RCOP '27 #132: ", true);
	for (const char* K : {"1", "2", "3", "4"})
	{
		As(NameOf((std::string("step") + K).c_str()), std::string("Step ") + K + Rung);
	}
	As(NameOf("mega-rcop"), "RCOP Main Mega Satellite");
	const bool Printed = Moved.empty();
	std::vector<std::string> Rcop = {"RCOP 2026", "RCOP", "RCOP #150: $5,250 Main Event", "RCOP '27 #132", "Step 1" + Rung, "Step 2" + Rung, "Step 3" + Rung,
		"Step 4" + Rung, "RCOP Main Mega Satellite", Tagline};
	if (Rc)
	{
		Rcop.push_back(Rc->Tagline);
	}
	std::string Hits;
	for (const std::string& S : Rcop)
	{
		const char* Hit = trademarks::Find(S, trademarks::kSourceScan);
		Hit = Hit ? Hit : trademarks::Find(S, trademarks::kCopyScan);
		Hit = Hit ? Hit : trademarks::Find(S, trademarks::kDesignWords);
		const std::string Low = trademarks::Lower(S);
		for (const char* W : FestNetAvoided)
		{
			Hit = Hit ? Hit : trademarks::HasWord(Low, W) ? W : nullptr;
		}
		Hits += Hit ? " \"" + S + "\" (" + Hit + ")" : std::string();
	}
	Expect(Printed && Hits.empty(),
		"RCOP's strings are untouched (\"RCOP 2026\", \"RCOP\", \"RCOP #150: $5,250 Main Event\", \"" + No132 + "\", Step 1 to Step 4 \xC2\xB7 RCOP Main, \"RCOP Main Mega "
		"Satellite\", the tagline \"" + Tagline + "\" leading RCOP 2026's and RCOP 2027's) and pass both trademark tiers, the avoided words and the design words" + Moved + Hits);

	// No RiverLine tagline or post types $100,000,000 (RCOP 2026's figure is its computed guarantee, §2.2.2).
	net::HeroStats NoHero;
	std::string Typed;
	int Posts = 0;
	for (const double At : {FestNetAt(2, 12 * 60), FestNetAt(ss::world::DayOn(2027, 11, 12), 12 * 60)})
	{
		for (const net::NewsItem& It : Rl.News(At, NoHero, 400))
		{
			++Posts;
			Typed = It.Body.find("$100,000,000") != std::string::npos || It.Title.find("$100,000,000") != std::string::npos ? It.Title : Typed;
		}
	}
	int Taglines = 0;
	for (const net::SeriesInfo& Sr : Rl.Series())
	{
		++Taglines;
		Typed = Sr.Tagline.find("$100,000,000") != std::string::npos ? Sr.Name : Typed;
	}
	Expect(Posts > 0 && Taglines > 0 && Typed.empty(), "no RiverLine tagline (" + std::to_string(Taglines) + " series) or post (" + std::to_string(Posts) +
														   " at Night One and after RCOP 2027) types $100,000,000" + (Typed.empty() ? std::string() : ": " + Typed));

	// The five seasonal renames (§2.2.2, X2) and the six tournament names in every printed form (the user, 2026-11-10): both tiers.
	const auto Prints = [](net::SiteId S, const char* Name) {
		for (const net::EventTemplate& T : net::Site(S).Templates())
		{
			if (T.Name == Name)
			{
				return true;
			}
		}
		return false;
	};
	const net::EventTemplate* FlipOpener = net::Site(net::SiteId::Ignite).FindTemplate("ignite.feign26-1");
	const std::string Flip = " Flip & Run";
	const bool Kept = Prints(net::SiteId::RiverLine, "Sunday Storm") && Prints(net::SiteId::RiverLine, "RiverLine Millions") &&
		Prints(net::SiteId::RiverLine, "Friday Night Fight") && Prints(net::SiteId::Ignite, "Flip & Run") && Prints(net::SiteId::Ignite, "Flip & Run Bounty") &&
		Prints(net::SiteId::RunItTwice, "The Closer") && FlipOpener && FlipOpener->Name.size() > Flip.size() &&
		FlipOpener->Name.compare(FlipOpener->Name.size() - Flip.size(), Flip.size(), Flip) == 0;
	std::string Screened;
	int Forms = 0;
	for (const char* S : {"Polar Night Series", "Deep Freeze Classic", "Spring Fever Series", "Greenlight Open", "Fireside Classic", "Platinum Pass", "Platinum Passes",
			 "Friday Night Fight", "Flip & Run", "Flip & Runs", "FLIP & RUN", "Flip & Run Bounty", "The Closer", "Sunday Storm", "RiverLine Millions"})
	{
		++Forms;
		const char* Hit = trademarks::Find(S, trademarks::kSourceScan);
		Hit = Hit ? Hit : trademarks::Find(S, trademarks::kCopyScan);
		Screened += Hit ? std::string(" \"") + S + "\" (" + Hit + ")" : std::string();
	}
	Expect(Kept && Screened.empty(),
		"the six tournament names the game already prints are never renamed (Sunday Storm, RiverLine Millions, Friday Night Fight, Flip & Run, Flip & Run Bounty, The "
		"Closer; FEIGN 2026's No 1 \"" + (FlipOpener ? FlipOpener->Name : std::string("missing")) + "\"), and they and the five seasonal renames pass both trademark tiers (" +
			std::to_string(Forms) + " forms)" + Screened);
}

// ------------------------------------------------------------------ FestRcopGtdSum (F-net's half)

/** §2.2.2 (the W5 fixes; PLAN_FNET Q12), §2.15.1 (F-net's half; slide 0 is F-hubs' FestHonestLines): RCOP 2026's SeriesInfo::GtdCents
 *  is the sum of its 154 templates' guarantees ($78,203,400) in both RiverLine construction paths (Network() and Network(RiverLine's
 *  SiteDef)); its tagline prints it ("The RiverLine Championship of Online Poker. $78,203,400 guaranteed."), and so does the day -6
 *  schedule post; its name, Short and Events stay; no RiverLine tagline or News body types "$100,000,000". Planted: Cents(1.0e8) kept. */
void FestRcopGtdSumNet()
{
	namespace net = ss::net;
	const net::Network& Rl = net::Site(net::SiteId::RiverLine);
	ss::Chips Sum = 0;
	int Events = 0;
	for (const net::EventTemplate& T : Rl.Templates())
	{
		if (T.Series == "rcop")
		{
			Sum += T.GtdCents;
			++Events;
		}
	}
	const std::string Tagline = "The RiverLine Championship of Online Poker. $78,203,400 guaranteed.";
	const net::Network ByDef(net::Def(net::SiteId::RiverLine));
	const net::SeriesInfo* Rc = Rl.FindSeries("rcop");
	const net::SeriesInfo* RcDef = ByDef.FindSeries("rcop");
	const bool Series = Rc && RcDef && Rc->GtdCents == Sum && RcDef->GtdCents == Sum && Rc->Tagline == Tagline && RcDef->Tagline == Tagline && Rc->Name == "RCOP 2026" &&
		Rc->Short == "RCOP" && Rc->Events == 154;
	Expect(Events == 154 && Sum == static_cast<ss::Chips>(7820340000ll) && Series,
		"RCOP 2026's guarantee is its 154 templates' sum, $78,203,400, in both RiverLine networks, and its tagline prints it (" + std::to_string(Events) + " templates, " +
			ss::net::BuyIn(Sum) + "; series " + (Rc ? ss::net::BuyIn(Rc->GtdCents) + " \"" + Rc->Tagline + "\"" : std::string("missing")) + ")");

	// The schedule post (day -6, 10:00) and every RiverLine headline to day 2 (Night One's News): the figure, never $100,000,000.
	net::HeroStats NoHero;
	std::string Post, Typed;
	for (const net::NewsItem& It : Rl.News(FestNetAt(2, 12 * 60), NoHero, 400))
	{
		Post = It.Title == "RCOP 2026: the schedule" ? It.Body : Post;
		Typed = It.Body.find("$100,000,000") != std::string::npos || It.Title.find("$100,000,000") != std::string::npos ? It.Title : Typed;
	}
	for (const net::SeriesInfo& Sr : Rl.Series())
	{
		Typed = Sr.Tagline.find("$100,000,000") != std::string::npos ? Sr.Name : Typed;
	}
	Expect(Post == "154 events. $78,203,400 guaranteed. October 18 to November 8. Satellites run every day from $2.20." && Typed.empty(),
		"the RCOP 2026 schedule post reads the computed guarantee (\"" + Post + "\") and no RiverLine tagline or headline types $100,000,000" +
			(Typed.empty() ? std::string() : ": " + Typed));
}

// ------------------------------------------------------------------ FestBlurbsFilled (F-net's half)

/** The figures a blurb prints: "$2.20", "$1,050", "$1M", "3:00" (a time's digits), "2" (a step); each maximal run of $ , . : and
 *  digits holding a digit (a K or M right after it joins), trailing punctuation dropped. */
std::vector<std::string> FestNetFigures(const std::string& Text)
{
	std::vector<std::string> Out;
	const auto Part = [&Text](size_t K) {
		const char C = Text[K];
		return (C >= '0' && C <= '9') || C == '$' || C == ',' || C == '.' || C == ':';
	};
	size_t I = 0;
	while (I < Text.size())
	{
		if (!Part(I))
		{
			++I;
			continue;
		}
		size_t J = I;
		while (J < Text.size() && Part(J))
		{
			++J;
		}
		if (J < Text.size() && (Text[J] == 'K' || Text[J] == 'M') && (J + 1 == Text.size() || !std::isalnum(static_cast<unsigned char>(Text[J + 1]))))
		{
			++J;
		}
		std::string Tok = Text.substr(I, J - I);
		while (!Tok.empty() && (Tok.back() == '.' || Tok.back() == ',' || Tok.back() == ':'))
		{
			Tok.pop_back();
		}
		if (Tok.find_first_of("0123456789") != std::string::npos)
		{
			Out.push_back(Tok);
		}
		I = J;
	}
	return Out;
}

/** Why a blurb fails §2.11's rule ("" if it passes): a number spelled out (two to twenty, the tens, hundred, thousand, million,
 *  billion, dozen; "one for every" and the ordinals are words, not figures), a typed past, or a figure that isn't one of the
 *  template's own (its buy-in, guarantee, added money and seat value; its Main's buy-in; its start time; the step its seat climbs to). */
std::string FestNetBlurbWhy(const ss::net::EventTemplate& T, ss::Chips MainBuyIn)
{
	namespace net = ss::net;
	if (T.Blurb.empty())
	{
		return std::string();
	}
	std::string Why;
	const std::string Low = trademarks::Lower(T.Blurb);
	for (const char* W : {"two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen",
			 "seventeen", "eighteen", "nineteen", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety", "hundred", "thousand", "million",
			 "billion", "dozen"})
	{
		Why += trademarks::HasWord(Low, W) ? std::string(" spells \"") + W + "\"" : std::string();
	}
	for (const char* Past : {"last year", "last week", "last night", "champion won"})
	{
		Why += Low.find(Past) != std::string::npos ? std::string(" a typed past: ") + Past : std::string();
	}
	std::set<std::string> Own = {net::BuyIn(T.BuyInCents), net::BuyIn(MainBuyIn)};
	for (const ss::Chips C : {T.GtdCents, T.AddedCents, T.SeatValueCents})
	{
		if (C > 0)
		{
			Own.insert(net::BuyIn(C));
			Own.insert(net::MoneyShort(C));
		}
	}
	const std::string Start = net::TimeLabel(static_cast<double>(T.StartMinute));
	Own.insert(Start.substr(0, Start.find(' ')));
	// A step's seat climbs to "Step K" (its seats text names the rung: "Step 2 tickets ($5.50)").
	if (T.Seats.rfind("Step ", 0) == 0)
	{
		Own.insert(T.Seats.substr(5, T.Seats.find(' ', 5) - 5));
	}
	for (const std::string& Fig : FestNetFigures(T.Blurb))
	{
		Why += Own.count(Fig) ? std::string() : " a figure not the template's: " + Fig;
	}
	return Why;
}

/** §2.11 (B1-B15; rev-plan minor 11), §2.2.2, §2.15.1 (F-net's half; U2/U15 are F-hubs' FestCopyUi): no festival blurb spells a number
 *  out and every figure in one comes from its template: every rival festival event's blurb (B1-B15: its buy-in, its start, its Main's
 *  buy-in, the step its seat climbs to), and RiverLine's three W5 blurbs (rcop-main's "$25,000,000 guaranteed." from its guarantee, the
 *  Mega's "$5,250" twice from its seat's value and the Main's buy-in, Sunday Showdown's "$1M" from its guarantee), which type no past
 *  either. RiverLine's other blurbs are today's strings (§2.2.2: untouched, U9). Planted: "Twenty-two dollars" restored (The Summons). */
void FestBlurbsFilledNet()
{
	namespace fest = ss::fest;
	namespace net = ss::net;
	int Blurbs = 0, Bad = 0;
	std::string First;
	for (const fest::Festival* F : FestNetRivalEditions())
	{
		const net::Network& N = net::Site(F->Site);
		const net::EventTemplate* Main = N.FindTemplate(F->MainId);
		for (const net::EventTemplate* T : FestNetEditionTemplates(*F))
		{
			Blurbs += T->Blurb.empty() ? 0 : 1;
			const std::string Why = FestNetBlurbWhy(*T, Main ? Main->BuyInCents : 0);
			if (!Why.empty())
			{
				++Bad;
				First = First.empty() ? ": \"" + T->Blurb + "\" (" + T->Id + ")" + Why : First;
			}
		}
	}
	const net::Network& Rl = net::Site(net::SiteId::RiverLine);
	const net::EventTemplate* RcopMain = Rl.FindTemplate("rcop-main");
	const std::pair<const char*, const char*> W5[] = {{"rcop-main", "The one that changes a life. $25,000,000 guaranteed."},
		{"mega-rcop", "Seats to the $5,250 RCOP Main Event: one for every $5,250 in the pool."}, {"sunday-showdown", "The people's major. $1M guaranteed every Sunday."}};
	int Fixed = 0;
	for (const auto& Row : W5)
	{
		const net::EventTemplate* T = Rl.FindTemplate(Row.first);
		const std::string Why = !T ? std::string(" missing") : FestNetBlurbWhy(*T, RcopMain ? RcopMain->BuyInCents : 0) + (T->Blurb == Row.second ? "" : " not §2.2.2's text");
		Fixed += T ? 1 : 0;
		if (!Why.empty())
		{
			++Bad;
			First = First.empty() ? ": \"" + (T ? T->Blurb : std::string()) + "\" (" + Row.first + ")" + Why : First;
		}
	}
	const net::EventTemplate* Mega = Rl.FindTemplate("mega-rcop");
	const bool MegaSeats = Mega && Mega->Seats == "RCOP Main Event seats" && Mega->GtdCents == 0 && RcopMain && Mega->SeatValueCents == RcopMain->BuyInCents;
	Expect(Blurbs > 0 && Fixed == 3 && Bad == 0 && MegaSeats,
		"no festival blurb spells a number out, and every figure in one is its template's (" + std::to_string(Blurbs) +
			" rival festival blurbs, B1-B15, and RiverLine's three W5 blurbs; the Mega's seats text is \"RCOP Main Event seats\", no seat guaranteed)" + First);
}

// ------------------------------------------------------------------ the juice (FN-5): the Main curve, Added money, guarantees, overlays

/** Night One's first world minute (tests/world_seg_sites.cpp's NightOne): where a Blank world starts. */
const double FestNetNightOne = 1440.0 + 127.0;

/** A rival festival edition's Main with its instance (SPEC v1.7 §2.4: the six rival Mains carry Added and Curve::Main). */
struct FestNetMain
{
	const ss::fest::Festival* F = nullptr;
	const ss::net::Network* N = nullptr;
	const ss::net::EventTemplate* T = nullptr;
	ss::net::EventInstance E;
	ss::Chips Contribution = 0; // llround(BuyIn x the site's prize share): what an entry puts in the pool
};

/** Every rival festival edition's Main from FromYear to ToYear, in All()'s order (a Main with no instance at its MainStart fails). */
std::vector<FestNetMain> FestNetMains(int FromYear, int ToYear)
{
	std::vector<FestNetMain> Out;
	for (const ss::fest::Festival* F : FestNetRivalEditions())
	{
		if (F->Year < FromYear || F->Year > ToYear)
		{
			continue;
		}
		FestNetMain M;
		M.F = F;
		M.N = &ss::net::Site(F->Site);
		if (!M.N->Next(F->MainId, F->MainStart - 1.0, M.E) || std::fabs(M.E.Start - F->MainStart) > 0.5)
		{
			Expect(false, F->Name + "'s Main " + F->MainId + " has an instance at its MainStart");
			continue;
		}
		M.T = &M.N->TemplateOf(M.E);
		M.Contribution = static_cast<ss::Chips>(std::llround(static_cast<double>(M.T->BuyInCents) * M.N->Definition().Rake.PrizeShare(*M.T)));
		Out.push_back(M);
	}
	return Out;
}

/** "" when a prize table is non-increasing (no place pays more than the one above it), never below 0 and sums to Pool; else its
 *  first fault. */
std::string FestNetTableFault(const std::vector<ss::Chips>& T, ss::Chips Pool)
{
	ss::Chips Sum = 0;
	for (size_t I = 0; I < T.size(); ++I)
	{
		Sum += T[I];
		if (T[I] < 0)
		{
			return "place " + std::to_string(I + 1) + " pays " + std::to_string(T[I]) + " cents";
		}
		if (I > 0 && T[I] > T[I - 1])
		{
			return "place " + std::to_string(I + 1) + " pays " + std::to_string(T[I]) + " cents over place " + std::to_string(I) + "'s " + std::to_string(T[I - 1]);
		}
	}
	return Sum == Pool ? std::string() : "it sums to " + std::to_string(Sum) + " of " + std::to_string(Pool) + " cents";
}

/** Whether §2.4.3's step 5 (b) scales the floors (they sum past the pool once made non-increasing): the check's own copy of steps 1,
 *  4 and 5 in PayoutFor's arithmetic, so the edge sweep can say how many of its tables took the scaled path (N-1). */
bool FestNetFloorsScaled(ss::Chips PoolCents, int Entrants, ss::Chips MinCashCents)
{
	if (Entrants <= 3)
	{
		return false;
	}
	const int Paid = ss::PaidPlaces(Entrants, ss::PayoutCurve::Main);
	const double Pool = static_cast<double>(PoolCents);
	const double F0 = ss::Min(static_cast<double>(MinCashCents), Pool / Paid);
	std::vector<double> F(static_cast<size_t>(Paid));
	for (int I = 1; I <= Paid; ++I)
	{
		double Fi = F0;
		if (I > Paid / 2)
		{
			const double B = static_cast<double>(Paid - I) / static_cast<double>(Paid);
			const int K = B < ss::payout::kMainFloorFlat ? 0 : 1 + static_cast<int>(std::floor((B - ss::payout::kMainFloorFlat) / ss::payout::kMainFloorBlock));
			Fi = F0 * ss::PowInt(ss::payout::kMainFloorStep, K);
		}
		F[static_cast<size_t>(I - 1)] = Fi;
	}
	for (int I = Paid - 2; I >= 0; --I)
	{
		F[static_cast<size_t>(I)] = ss::Max(F[static_cast<size_t>(I)], F[static_cast<size_t>(I + 1)]);
	}
	double Sum = 0.0;
	for (int I = 0; I < Paid; ++I)
	{
		Sum += F[static_cast<size_t>(I)];
	}
	return Sum > Pool;
}

/** §2.4.3's table "What the curve does", the Main columns (model/spec-fix3/examples_out.txt §1): a $11 event at RiverLine's share,
 *  E.Pool = N x $10.07, min cash $16.50. Paid, 1st's share, the final table's share, the share of paid places flat at the min cash. */
struct FestNetShapeRow
{
	int Entries, Paid;
	double First, Final, Flat;
};
const FestNetShapeRow FestNetShapeTable[] = {{260, 31, 26.1, 72.2, 0.0}, {700, 84, 23.0, 61.1, 0.0}, {1000, 120, 21.6, 57.3, 10.0}, {3000, 360, 17.9, 47.4, 20.0},
	{10000, 1200, 14.0, 37.9, 19.0}, {26662, 3199, 11.5, 31.9, 19.0}, {124466, 14936, 9.1, 25.7, 20.0}};

/** SPEC v1.7 §2.4.3, §2.15.1: PayoutFor(Main) matches the printed table at 260 ... 124,466 entries (paid exact, 1st and the final table
 *  +-0.2 points of the pool, the flat band +-1 point); every table is non-increasing, never below 0 and sums to its pool there, at every
 *  101st N from 4 to 130,000 (1,288 fields) on each rival Main's E.Pool with and without its added money (2027's six), and on
 *  spec-check6's 52 edge setups at every 1,001st N (6,760 tables, 3,096 of them with step 5 (b) scaling the floors) and the three
 *  field sizes where step 6 run on scaled floors rose (N-1); Standard is PayoutTable byte for byte on 2,000 sampled (pool, N <= 200,000).
 *  Planted: the lift not tapered; v1.6's step 5 (place 577 over place 576 at 10,000 entries); step 6 on scaled floors (the text
 *  before N-1: place 28 over place 27 at 3,822 on GRIT's contribution). */
void FestCurveShapeNet()
{
	using ss::Chips;
	const ss::PayoutCurve Main = ss::PayoutCurve::Main;
	const auto T0 = std::chrono::steady_clock::now();
	// The printed table.
	std::string Bad;
	for (const FestNetShapeRow& R : FestNetShapeTable)
	{
		const Chips Pool = 1007 * static_cast<Chips>(R.Entries);
		const std::vector<Chips> T = ss::PayoutFor(Pool, R.Entries, 1650, Main);
		const int Paid = static_cast<int>(T.size());
		Chips Final = 0;
		int Above = -1; // the last place paying more than the min cash (examples.py's metrics: > 1.0005 x the min cash)
		for (int I = 0; I < Paid; ++I)
		{
			Final += I < 9 ? T[static_cast<size_t>(I)] : 0;
			Above = static_cast<double>(T[static_cast<size_t>(I)]) > 1650.0 * 1.0005 ? I : Above;
		}
		const double First = Paid > 0 ? 100.0 * static_cast<double>(T[0]) / static_cast<double>(Pool) : 0.0;
		const double FinalShare = 100.0 * static_cast<double>(Final) / static_cast<double>(Pool);
		const double Flat = Paid > 0 ? 100.0 * static_cast<double>(Paid - (Above + 1)) / static_cast<double>(Paid) : 0.0;
		std::printf("festivals: the Main curve at %d entries (pool $%.2f, min cash $16.50): %d paid, 1st %.2f%%, final table %.2f%%, flat at the min cash %.1f%% "
					"(SPEC: %d / %.1f%% / %.1f%% / %.0f%%)\n",
			R.Entries, static_cast<double>(Pool) / 100.0, Paid, First, FinalShare, Flat, R.Paid, R.First, R.Final, R.Flat);
		const std::string Fault = FestNetTableFault(T, Pool);
		if (Paid != R.Paid || Paid != ss::PaidPlaces(R.Entries, Main) || std::fabs(First - R.First) > 0.2 || std::fabs(FinalShare - R.Final) > 0.2 ||
			std::fabs(Flat - R.Flat) > 1.0 || !Fault.empty())
		{
			Bad += " " + std::to_string(R.Entries) + " entries (" + std::to_string(Paid) + " paid, 1st " + std::to_string(First) + "%, final table " +
				std::to_string(FinalShare) + "%, flat " + std::to_string(Flat) + "%" + (Fault.empty() ? std::string() : "; " + Fault) + ")";
		}
	}
	Expect(Bad.empty(), "the Main curve matches SPEC §2.4.3's table at 260, 700, 1,000, 3,000, 10,000, 26,662 and 124,466 entries (paid exact, 1st and the final table "
						"within 0.2 points, the flat band within 1), each table non-increasing and summing to its pool" + (Bad.empty() ? std::string() : ":" + Bad));

	// Every 101st N from 4 to 130,000 on each rival Main's E.Pool (max(GTD, C x N) + Added, Network::Make's), with and without Added.
	const std::vector<FestNetMain> Mains = FestNetMains(2027, 2027);
	int Tables = 0;
	long long Places = 0;
	std::string SweepBad;
	for (const FestNetMain& M : Mains)
	{
		for (int WithAdded = 1; WithAdded >= 0; --WithAdded)
		{
			for (int N = 4; N <= 130000; N += 101)
			{
				const Chips Pool = std::max(M.T->GtdCents, M.Contribution * static_cast<Chips>(N)) + (WithAdded ? M.T->AddedCents : 0);
				const std::vector<Chips> T = ss::PayoutFor(Pool, N, M.T->BuyInCents * 3 / 2, Main);
				++Tables;
				Places += static_cast<long long>(T.size());
				const std::string Fault = static_cast<int>(T.size()) == ss::PaidPlaces(N, Main) ? FestNetTableFault(T, Pool) : std::string("PaidPlaces differs from the table");
				if (!Fault.empty() && SweepBad.empty())
				{
					SweepBad = ": " + M.F->Name + "'s Main at " + std::to_string(N) + " entries" + (WithAdded ? "" : " without its added money") + ", " + Fault;
				}
			}
		}
	}
	Expect(Mains.size() == 6 && Tables == 6 * 2 * 1288 && SweepBad.empty(),
		"the Main curve is non-increasing, never below 0 and sums to the pool at every 101st N from 4 to 130,000 on each of 2027's six rival Mains' pools, with and without "
		"Added (" + std::to_string(Tables) + " tables, " + std::to_string(Places) + " places)" + SweepBad);

	// spec-check6's 52 edge setups (each pool kind with a min cash from Pool / (2 x Paid) to the pool), every 1,001st N, and the three
	// named field sizes (N-1, model/spec-pass/n1_edge_out.txt).
	struct FestNetEdgePool
	{
		const char* Name;
		Chips Contribution; // < 0: a flat pool of -Contribution
		Chips Added;
	};
	const FestNetEdgePool Edges[] = {{"REIGN's contribution + $50,000", 198450, 5000000}, {"GRIT's contribution", 50085, 0}, {"the $11 pool", 1007, 0},
		{"a flat $1,000,000,000", -100000000000LL, 0}};
	const int Ratios[] = {1000, 1200, 1300, 1340, 1350, 1360, 1370, 1380, 1400, 1450, 1520, 2000, -1}; // min cash Pool / (Paid x r / 1000); -1: the pool
	const auto EdgePool = [](const FestNetEdgePool& P, int N) { return (P.Contribution < 0 ? -P.Contribution : P.Contribution * static_cast<Chips>(N)) + P.Added; };
	const auto EdgeMin = [Main](Chips Pool, int N, int R) {
		return R < 0 ? Pool
					 : static_cast<Chips>(static_cast<double>(Pool) * 1000.0 / (static_cast<double>(ss::PaidPlaces(N, Main)) * static_cast<double>(R)));
	};
	int EdgeTables = 0;
	int Scaled = 0;
	std::string EdgeBad;
	for (const FestNetEdgePool& P : Edges)
	{
		for (const int R : Ratios)
		{
			for (int N = 4; N <= 130000; N += 1001)
			{
				const Chips Pool = EdgePool(P, N);
				const Chips Min = EdgeMin(Pool, N, R);
				const std::string Fault = FestNetTableFault(ss::PayoutFor(Pool, N, Min, Main), Pool);
				++EdgeTables;
				Scaled += FestNetFloorsScaled(Pool, N, Min) ? 1 : 0;
				if (!Fault.empty() && EdgeBad.empty())
				{
					EdgeBad = ": " + std::string(P.Name) + " at " + std::to_string(N) + " entries, min cash " + std::to_string(Min) + " cents, " + Fault;
				}
			}
		}
	}
	struct FestNetNamed
	{
		int Edge, Entries, Ratio;
	};
	const FestNetNamed Named[] = {{1, 3822, 1000}, {0, 63995, 1000}, {3, 26355, -1}};
	int NamedScaled = 0;
	for (const FestNetNamed& Nm : Named)
	{
		const Chips Pool = EdgePool(Edges[Nm.Edge], Nm.Entries);
		const Chips Min = EdgeMin(Pool, Nm.Entries, Nm.Ratio);
		const std::string Fault = FestNetTableFault(ss::PayoutFor(Pool, Nm.Entries, Min, Main), Pool);
		NamedScaled += FestNetFloorsScaled(Pool, Nm.Entries, Min) ? 1 : 0;
		if (!Fault.empty())
		{
			EdgeBad += "; " + std::string(Edges[Nm.Edge].Name) + " at " + std::to_string(Nm.Entries) + " entries (N-1's named size), " + Fault;
		}
	}
	std::printf("festivals: the Main curve's edge sweep: %d tables on spec-check6's 52 setups at every 1,001st N, %d with step 5 (b) scaling the floors; the three "
				"named sizes (3,822, 63,995, 26,355) %d of 3 scaled\n",
		EdgeTables, Scaled, NamedScaled);
	Expect(EdgeTables == 6760 && Scaled == 3096 && NamedScaled == 3 && EdgeBad.empty(),
		"the Main curve keeps the scaled floors (N-1): spec-check6's 52 edge setups at every 1,001st N (6,760 tables, 3,096 scaled) and its three named field sizes "
		"are non-increasing, never below 0 and sum to the pool (" + std::to_string(EdgeTables) + " tables, " + std::to_string(Scaled) + " scaled)" + EdgeBad);

	// Standard is PayoutTable exactly, on 2,000 sampled (pool, N <= 200,000, min cash).
	uint64_t Seed = 0x9e3779b97f4a7c15ull;
	const auto Draw = [&Seed]() {
		Seed = Seed * 6364136223846793005ull + 1442695040888963407ull;
		return Seed >> 33;
	};
	int Same = 0;
	std::string StdBad;
	for (int K = 0; K < 2000; ++K)
	{
		const int N = 1 + static_cast<int>(Draw() % 200000);
		const Chips Each = 1 + static_cast<Chips>(Draw() % 500000);
		const Chips Pool = K % 10 == 0 ? 1 + static_cast<Chips>(Draw()) * 47 : Each * static_cast<Chips>(N);
		const Chips Min = K % 7 == 0 ? static_cast<Chips>(Draw() % static_cast<uint64_t>(Pool + 1)) : Each * 3 / 2;
		const std::vector<Chips> A = ss::PayoutFor(Pool, N, Min, ss::PayoutCurve::Standard);
		const bool Ok = A == ss::PayoutTable(Pool, N, Min) && static_cast<int>(A.size()) == ss::PaidPlaces(N, ss::PayoutCurve::Standard);
		Same += Ok ? 1 : 0;
		if (!Ok && StdBad.empty())
		{
			StdBad = ": pool " + std::to_string(Pool) + ", " + std::to_string(N) + " entries, min cash " + std::to_string(Min);
		}
	}
	const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count();
	std::printf("festivals: FestCurveShape: %d + %d tables swept, Standard == PayoutTable on %d of 2,000 samples; %.2fs\n", Tables, EdgeTables, Same, Seconds);
	Expect(Same == 2000, "PayoutFor(Standard) is PayoutTable byte for byte, PaidPlaces its size, on 2,000 sampled pools, fields up to 200,000 and min cashes" + StdBad);
}

/** The world's payout of an online festival Main: Sim::OnlineEvent's pending event (P.Pool = E.Pool) through Sim::CurveOf and
 *  WorldSim.cpp's PayoutFor form (a freezeout: the prize pool is the pool). */
std::vector<ss::Chips> FestNetWorldPayouts(ss::world::World& W, const ss::net::EventInstance& E, ss::Chips* Pool = nullptr)
{
	const ss::world::Pending P = ss::world::Sim::OnlineEvent(W, E);
	if (Pool)
	{
		*Pool = P.Pool;
	}
	return ss::PayoutFor(P.Pool, P.Entries, std::max<ss::Chips>(1, P.BuyIn * 3 / 2), ss::world::Sim::CurveOf(P));
}

/** The nine top places of a table in whole dollars, 9th first (SPEC §2.4.3's "pays 9th to 1st" lines). */
std::vector<long long> FestNetTopDollars(const std::vector<ss::Chips>& T)
{
	std::vector<long long> Out;
	for (int I = std::min<int>(9, static_cast<int>(T.size())) - 1; I >= 0; --I)
	{
		Out.push_back(std::llround(static_cast<double>(T[static_cast<size_t>(I)]) / 100.0));
	}
	return Out;
}

/** SPEC v1.7 §2.4.3, §2.15.1: the engine (a Tournament from the listing's spec), the network (Network::Payouts) and the world (Sim's
 *  form) pay every rival festival Main of 2026-2028 to the cent; PaidPlaces is the table's size; every table is non-increasing and
 *  never below 0; the written lift's figures hold (FEIGN's and GRIT's lines, the championships' first prizes); and one Blank world run
 *  through FEIGN I's Main (day 66) recorded its prizes from Network::Payouts. Planted: the engine's 1.4x min cash kept under Main; the
 *  0-based lift (1st 1 + 9L/8); step 7's remainder from the last place up (all four still agree: only the monotone half fails). */
void FestCurveAgreesNet()
{
	using ss::Chips;
	const ss::PayoutCurve Main = ss::PayoutCurve::Main;
	const std::vector<FestNetMain> Mains = FestNetMains(2026, 2028);
	ss::world::World W;
	W.Create(7u, FestNetNightOne, ss::world::Start::Blank);
	std::string Bad;
	long long Places = 0;
	for (const FestNetMain& M : Mains)
	{
		const std::vector<Chips> Net = M.N->Payouts(M.E);
		const ss::Tournament Engine(M.N->Listing(M.E).Spec, "FestNet", "fest-curve:" + M.E.Id);
		const std::vector<Chips> World = FestNetWorldPayouts(W, M.E);
		Places += static_cast<long long>(Net.size());
		const std::string Fault = FestNetTableFault(Net, M.E.Pool);
		const std::string EngineFault = FestNetTableFault(Engine.Payouts, Engine.PrizePoolCents);
		if (M.T->Curve != Main || Engine.Payouts != Net || World != Net || static_cast<int>(Net.size()) != ss::PaidPlaces(M.E.Entries, Main) || !Fault.empty() ||
			!EngineFault.empty())
		{
			Bad += " " + M.F->Name + " (" + std::to_string(M.E.Entries) + " entries: network " + std::to_string(Net.size()) + " places, 1st " +
				std::to_string(Net.empty() ? 0 : Net[0]) + "; engine " + std::to_string(Engine.Payouts.size()) + ", 1st " +
				std::to_string(Engine.Payouts.empty() ? 0 : Engine.Payouts[0]) + "; world " + std::to_string(World.size()) + ", 1st " +
				std::to_string(World.empty() ? 0 : World[0]) + (Fault.empty() ? std::string() : "; " + Fault) + (EngineFault.empty() ? std::string() : "; engine: " + EngineFault) + ")";
		}
	}
	Expect(Mains.size() == 13 && Bad.empty(), "the engine, the network and the world pay every rival festival Main of 2026-2028 to the cent on the Main curve, PaidPlaces is "
											  "the table's size and no place pays more than the one above it or below 0 (" +
												  std::to_string(Mains.size()) + " Mains, " + std::to_string(Places) + " places)" + Bad);

	// The written lift's figures (SPEC §2.4.3; model/spec-fix3/examples_out.txt §3-5), each on its own pool and min cash.
	struct FestNetLine
	{
		const char* What;
		Chips Pool;
		int Entries;
		Chips MinCash;
		int Paid;
		Chips First;  // exact cents, or 0 when only the whole dollars are printed
		long long FirstDollars;
		Chips Last;   // the min cash paid, exact cents (0: not printed)
		std::vector<long long> Top; // 9th to 1st in whole dollars (empty: not printed)
	};
	const FestNetLine Lines[] = {
		{"the FEIGN Main (700 runners, $37,500)", 3750000, 700, 8250, 84, 865491, 8655, 8558, {740, 863, 1022, 1234, 1531, 1977, 2719, 4203, 8655}},
		{"GRIT's Main Event (1,000, $510,850)", 51085000, 1000, 79500, 120, 11073771, 110738, 79500, {9467, 11049, 13084, 15796, 19594, 25290, 34785, 53773, 110738}},
		{"PENNANT's Main at its median (410, $88,433.89)", 8843389, 410, 32250, 49, 0, 20836, 0, {}},
		{"REIGN's Main at its median (359, $761,532.55)", 76153255, 359, 315000, 43, 18466307, 184663, 0, {}},
		{"REIGN's Main at its largest draw (402, $847,769)", 84776900, 402, 315000, 48, 0, 200644, 0, {}},
		{"PENNANT 2027's 409-runner Main ($88,262.15)", 8826215, 409, 32250, 49, 2079538, 20795, 0, {}},
	};
	std::string LineBad;
	for (const FestNetLine& Ln : Lines)
	{
		const std::vector<Chips> T = ss::PayoutFor(Ln.Pool, Ln.Entries, Ln.MinCash, Main);
		const long long FirstDollars = T.empty() ? 0 : std::llround(static_cast<double>(T[0]) / 100.0);
		std::printf("festivals: %s pays 1st %.2f of %d paid, min cash %.2f\n", Ln.What, T.empty() ? 0.0 : static_cast<double>(T[0]) / 100.0, static_cast<int>(T.size()),
			T.empty() ? 0.0 : static_cast<double>(T.back()) / 100.0);
		if (static_cast<int>(T.size()) != Ln.Paid || FirstDollars != Ln.FirstDollars || (Ln.First > 0 && T[0] != Ln.First) || (Ln.Last > 0 && T.back() != Ln.Last) ||
			(!Ln.Top.empty() && FestNetTopDollars(T) != Ln.Top) || !FestNetTableFault(T, Ln.Pool).empty())
		{
			LineBad += " " + std::string(Ln.What) + " (" + std::to_string(T.size()) + " paid, 1st " + std::to_string(T.empty() ? 0 : T[0]) + " cents)";
		}
	}
	// PENNANT 2027's Main as the game draws it: 409 runners, so it is the line above.
	for (const FestNetMain& M : Mains)
	{
		if (M.F->Which == ss::fest::Brand::Pennant && M.F->Year == 2027 && M.E.Entries == 409)
		{
			const std::vector<Chips> Net = M.N->Payouts(M.E);
			LineBad += M.E.Pool == 8826215 && !Net.empty() && Net[0] == 2079538 ? "" : " PENNANT 2027's instance (pool " + std::to_string(M.E.Pool) + ")";
		}
	}
	Expect(LineBad.empty(), "the written lift's figures hold: FEIGN's Main pays $740 ... $8,655 (1st $8,654.91, min cash $85.58 of 84), GRIT's $9,467 ... $110,738 (min cash "
							"$795 of 120), PENNANT's median Main 1st $20,836, REIGN's $184,663 and $200,644, PENNANT 2027's instance $20,795.38" + LineBad);

	// One Blank world through FEIGN I's Main (day 66): every result it recorded for that Main is Network::Payouts' place.
	const FestNetMain* Feign = nullptr;
	for (const FestNetMain& M : Mains)
	{
		Feign = !Feign && M.F->Which == ss::fest::Brand::Feign && M.F->Year == 2026 ? &M : Feign;
	}
	if (!Feign)
	{
		Expect(false, "FEIGN 2026's Main is among the rival Mains");
		return;
	}
	const auto T1 = std::chrono::steady_clock::now();
	// The lead's carrier (LEAD-INT): the world simulates an online event only when someone it follows registers, and on seed 7 nobody
	// it follows registers for FEIGN I's Main once F-world's world runs; so the world read is the first Blank world from seed 7 that
	// played the Main (World::ResultOf), at most four (seeds 7-10; the last is read either way, so a world that never plays it fails below).
	std::unique_ptr<ss::world::World> Played;
	uint32_t Seed = 7u;
	for (;;)
	{
		Played = std::make_unique<ss::world::World>();
		Played->Create(Seed, FestNetNightOne, ss::world::Start::Blank);
		Played->AdvanceTo(Feign->E.Start + Feign->E.Duration + 60.0);
		if (Played->ResultOf(Feign->E.Id) || Seed == 10u)
		{
			break;
		}
		++Seed;
	}
	const ss::world::World& Run = *Played;
	const std::vector<Chips> Pay = Feign->N->Payouts(Feign->E);
	int Records = 0, Cashes = 0, Honors = 0;
	std::string RecBad;
	const auto PayAt = [&Pay](int Place) { return Place >= 1 && Place <= static_cast<int>(Pay.size()) ? Pay[static_cast<size_t>(Place - 1)] : Chips(0); };
	// The title (the world's history keeps every rival major's winner, known or not) and the final table posts of the people it follows.
	for (const ss::world::Honor& H : Run.Honors())
	{
		if (H.EventId == Feign->E.Id)
		{
			++Honors;
			++Records;
			RecBad += H.Prize != PayAt(1) || H.Entries != Feign->E.Entries ? " the title: " + std::to_string(H.Prize) + " cents (Network::Payouts " + std::to_string(PayAt(1)) + ")" : "";
		}
	}
	for (const ss::world::WorldEvent& Ev : Run.Events())
	{
		if (Ev.Kind == ss::world::EventKind::FinalTable && Ev.What == Feign->T->Name)
		{
			++Records;
			RecBad += Ev.Amount != PayAt(Ev.Value) ? " the final table's " + std::to_string(Ev.Value) + ": " + std::to_string(Ev.Amount) + " cents" : "";
		}
	}
	for (size_t I = 0; I < Run.People().size(); ++I)
	{
		for (const ss::world::Finish& Fi : Run.People()[I].Recent)
		{
			if (Fi.Event != Feign->T->Name || Fi.Entries != Feign->E.Entries)
			{
				continue;
			}
			++Records;
			const Chips Want = PayAt(Fi.Place);
			Cashes += Fi.Prize > 0 ? 1 : 0;
			if (Fi.Prize != Want || Fi.Curve != Main)
			{
				RecBad += " place " + std::to_string(Fi.Place) + ": " + std::to_string(Fi.Prize) + " cents (Network::Payouts " + std::to_string(Want) + ")";
			}
		}
	}
	std::printf("festivals: a Blank world (seed %u) through FEIGN I's Main (day %d, %d entries) in %.2fs: %d results recorded for it (the title %d; %d followed players' records, %d cashes)\n",
		Seed, static_cast<int>((Feign->E.Start + Feign->E.Duration) / ss::net::MinutesPerDay), Feign->E.Entries,
		std::chrono::duration<double>(std::chrono::steady_clock::now() - T1).count(), Records, Honors, Records - Honors, Cashes);
	Expect(Honors == 1 && RecBad.empty(), "a Blank world run through FEIGN I's Main recorded its title, final tables and finishes at Network::Payouts' prizes on the Main "
										  "curve (" + std::to_string(Records) + " results)" + RecBad);
}

/** SPEC v1.7 §2.4.1, §2.15.1: every rival festival Main of 2026-2028 carries its brand's added money and the Main curve, and only the
 *  Mains do (never a championship's Mini or Micro Main or a headliner); Make's pool is max(GTD, C x N) + Added; the listing's spec carries
 *  Added and the curve; a Tournament built from it has PrizePoolCents == E.Pool; the world's pending event has P.Pool == E.Pool (a Blank
 *  world's Sim::OnlineEvent); Live at Finished reports E.Pool and judges overlay on the base (E.Pool - Added). Planted: + Spec.AddedCents
 *  dropped in the engine (E/Tournament.cpp:34). */
void FestAddedMoneyAgreesNet()
{
	using ss::Chips;
	const ss::PayoutCurve Main = ss::PayoutCurve::Main;
	const std::map<ss::fest::Brand, Chips> Added = {{ss::fest::Brand::Feign, 250000}, {ss::fest::Brand::Push, 500000}, {ss::fest::Brand::Grit, 1000000},
		{ss::fest::Brand::Writ, 2500000}, {ss::fest::Brand::Pennant, 1000000}, {ss::fest::Brand::Reign, 5000000}};
	// Only the Mains: every template of every rival festival edition.
	int Juiced = 0;
	std::string OnlyBad;
	for (const ss::fest::Festival* F : FestNetRivalEditions())
	{
		for (const ss::net::EventTemplate* T : FestNetEditionTemplates(*F))
		{
			const bool IsMain = T->Id == F->MainId;
			const auto It = Added.find(F->Which);
			const Chips Want = IsMain && It != Added.end() ? It->second : 0;
			Juiced += T->AddedCents > 0 ? 1 : 0;
			if (T->AddedCents != Want || (T->Curve == Main) != IsMain)
			{
				OnlyBad += " " + T->Id + " (Added " + std::to_string(T->AddedCents) + ", curve " + std::to_string(static_cast<int>(T->Curve)) + ")";
			}
		}
	}
	Expect(OnlyBad.empty() && Juiced > 0, "only the rival festival Mains carry added money (FEIGN $2,500, PUSH $5,000, GRIT $10,000, WRIT $25,000, PENNANT $10,000, REIGN "
										  "$50,000) and the Main curve, never a Mini or Micro Main, a headliner or a satellite (" +
											  std::to_string(Juiced) + " Mains)" + OnlyBad);

	ss::world::World W;
	W.Create(7u, FestNetNightOne, ss::world::Start::Blank);
	const std::vector<FestNetMain> Mains = FestNetMains(2026, 2028);
	int Short = 0, ShortSeen = 0;
	std::string Bad;
	for (const FestNetMain& M : Mains)
	{
		const Chips Base = std::max(M.T->GtdCents, M.Contribution * static_cast<Chips>(M.E.Entries));
		const auto L = M.N->Listing(M.E);
		const ss::Tournament Engine(L.Spec, "FestNet", "fest-added:" + M.E.Id);
		Chips WorldPool = 0;
		FestNetWorldPayouts(W, M.E, &WorldPool);
		const double After = M.E.Start + M.E.Duration + 1.0;
		const ss::net::LiveState Live = M.N->Live(M.E, After);
		const Chips PerEntry = (M.E.Pool - M.T->AddedCents) / std::max(1, M.E.Entries);
		const bool ShortField = M.Contribution * static_cast<Chips>(M.E.Entries) < M.T->GtdCents;
		Short += ShortField ? 1 : 0;
		ShortSeen += ShortField && Live.Overlay ? 1 : 0;
		if (M.T->AddedCents <= 0 || M.E.Pool != Base + M.T->AddedCents || L.Spec.AddedCents != M.T->AddedCents || L.Spec.Curve != M.T->Curve ||
			Engine.PrizePoolCents != M.E.Pool || WorldPool != M.E.Pool || Live.St != ss::net::Status::Finished || Live.Pool != M.E.Pool ||
			Live.Overlay != (PerEntry * static_cast<Chips>(M.E.Entries) < M.T->GtdCents) || (Live.Overlay && !ShortField))
		{
			Bad += " " + M.F->Name + " (pool " + std::to_string(M.E.Pool) + ", max(GTD, C x N) + Added " + std::to_string(Base + M.T->AddedCents) + "; spec Added " +
				std::to_string(L.Spec.AddedCents) + "; engine " + std::to_string(Engine.PrizePoolCents) + "; world " + std::to_string(WorldPool) + "; Live " +
				std::to_string(Live.Pool) + (Live.Overlay ? ", overlay" : "") + ")";
		}
	}
	std::printf("festivals: the added money end to end on %zu rival Mains of 2026-2028 (%d closed short of the guarantee, %d of them an overlay at Finished)\n", Mains.size(), Short,
		ShortSeen);
	Expect(Mains.size() == 13 && Bad.empty(), "every rival festival Main of 2026-2028: Make's pool is max(GTD, C x N) + Added, the listing's spec carries Added and the curve, "
											  "the engine's PrizePoolCents and the world's P.Pool are E.Pool, and Live at Finished reports E.Pool with the overlay judged on the base" +
												  Bad);
}

/** A guarantee as a lobby writes it (G/NetworkSites.cpp's NetSitesNice, the check's own copy): two significant figures, rounded down. */
double FestNetNice(double Dollars)
{
	if (Dollars < 100.0)
	{
		return std::floor(Dollars + 1e-9);
	}
	const double Step = std::pow(10.0, std::floor(std::log10(Dollars)) - 1.0);
	return std::floor(Dollars / Step + 1e-9) * Step;
}

/** True when Text promises a seat count: a number (digits or a number word) right before "seat", or a seat guarantee. */
bool FestNetSeatCount(const std::string& Text)
{
	std::string Low;
	for (const char Ch : Text)
	{
		Low.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(Ch))));
	}
	if (Low.find("guarantee") != std::string::npos || Low.find("gtd") != std::string::npos)
	{
		return true;
	}
	static const char* const Words[] = {"one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve", "fifteen", "twenty", "fifty", "hundred"};
	for (size_t At = Low.find("seat"); At != std::string::npos; At = Low.find("seat", At + 1))
	{
		size_t End = At;
		while (End > 0 && Low[End - 1] == ' ')
		{
			--End;
		}
		size_t Begin = End;
		while (Begin > 0 && (std::isalnum(static_cast<unsigned char>(Low[Begin - 1])) || Low[Begin - 1] == ','))
		{
			--Begin;
		}
		const std::string Word = Low.substr(Begin, End - Begin);
		bool Number = !Word.empty() && std::isdigit(static_cast<unsigned char>(Word[0]));
		for (const char* W : Words)
		{
			Number = Number || Word == W;
		}
		if (Number)
		{
			return true;
		}
	}
	return false;
}

/** SPEC v1.7 §2.4.2, §2.15.1: headline guarantees are NetSitesNice(share x the median pool) at 1.0 on Ignite and 0.95 on RunItTwice (the
 *  Main, the headliners and the micro-mega), numbered events at 0.9 (FEIGN's two $11 headline events, No 8 and 16, at 1.0); The Summons
 *  and mega-rcop carry none and their blurbs and seats text promise no seat count; the championships' twelve headline guarantees match
 *  §2.4.2's W5 table to the dollar for 2027 and 2028, and the editions' GtdCents are $281,100 and $1,991,300. Planted: a headliner at 0.9;
 *  a championship headliner at 0.9x. */
void FestGuaranteesNet()
{
	using ss::Chips;
	int Headliners = 0, Numbered = 0;
	std::string Bad;
	for (const ss::fest::Festival* F : FestNetRivalEditions())
	{
		const ss::net::Network& N = ss::net::Site(F->Site);
		const ss::net::SiteDef& D = N.Definition();
		const double HeadShare = F->Site == ss::net::SiteId::RunItTwice ? 0.95 : 1.0;
		for (const ss::net::EventTemplate* T : FestNetEditionTemplates(*F))
		{
			if (T->Fmt == ss::net::Format::Satellite || T->Series != F->SeriesId)
			{
				continue;
			}
			const std::string Tail = T->Id.substr(std::min(T->Id.size(), F->SeriesId.size() + 1));
			const bool IsNumbered = !Tail.empty() && std::all_of(Tail.begin(), Tail.end(), [](char Ch) { return Ch >= '0' && Ch <= '9'; });
			const bool HeadlineNo = IsNumbered && F->Which == ss::fest::Brand::Feign && (T->EventNo == 8 || T->EventNo == 16);
			const double Share = !IsNumbered || HeadlineNo ? HeadShare : 0.9;
			const double Contribution = static_cast<double>(std::llround(static_cast<double>(T->BuyInCents) * D.Rake.PrizeShare(*T))) / 100.0;
			const double Median = static_cast<double>(T->Field) * D.Traffic.Scale * 1.025 * Contribution;
			const Chips Want = static_cast<Chips>(std::llround(FestNetNice(Share * Median) * 100.0));
			(IsNumbered ? Numbered : Headliners) += 1;
			if (T->GtdCents != Want)
			{
				Bad += " " + T->Id + " (" + std::to_string(T->GtdCents) + " cents, " + std::to_string(Share) + "x the median pool is " + std::to_string(Want) + ")";
			}
		}
	}
	Expect(Bad.empty() && Headliners > 0 && Numbered > 0, "every rival festival guarantee is NetSitesNice(share x the median pool): headliners 1.0x on Ignite and 0.95x on "
														  "RunItTwice, numbered events 0.9x (FEIGN's No 8 and 16 at 1.0x) (" +
															  std::to_string(Headliners) + " headliners, " + std::to_string(Numbered) + " numbered events)" + Bad);

	// The championships' twelve headline guarantees and their editions' totals (§2.4.2's W5 table), 2027 and 2028.
	struct FestNetChampRow
	{
		ss::fest::Brand Which;
		const char* Stem;
		Chips Gtd;
	};
	const FestNetChampRow Rows[] = {{ss::fest::Brand::Pennant, "on", 800000}, {ss::fest::Brand::Pennant, "mega", 260000}, {ss::fest::Brand::Pennant, "hr", 2100000},
		{ss::fest::Brand::Pennant, "micro", 500000}, {ss::fest::Brand::Pennant, "mini", 2400000}, {ss::fest::Brand::Pennant, "main", 7800000},
		{ss::fest::Brand::Reign, "on", 8700000}, {ss::fest::Brand::Reign, "mega", 1200000}, {ss::fest::Brand::Reign, "hr", 19000000},
		{ss::fest::Brand::Reign, "micro", 3500000}, {ss::fest::Brand::Reign, "mini", 17000000}, {ss::fest::Brand::Reign, "main", 67000000}};
	int Matched = 0, Editions = 0;
	std::string ChampBad;
	for (const ss::fest::Festival* F : FestNetRivalEditions())
	{
		if ((F->Which != ss::fest::Brand::Pennant && F->Which != ss::fest::Brand::Reign) || F->Year < 2027 || F->Year > 2028)
		{
			continue;
		}
		++Editions;
		const ss::net::Network& N = ss::net::Site(F->Site);
		for (const FestNetChampRow& R : Rows)
		{
			if (R.Which != F->Which)
			{
				continue;
			}
			const std::string Id = F->SeriesId + "-" + R.Stem;
			const ss::net::EventTemplate* T = nullptr;
			for (const ss::net::EventTemplate& Tm : N.Templates())
			{
				T = !T && Tm.Id == Id ? &Tm : T;
			}
			Matched += T && T->GtdCents == R.Gtd ? 1 : 0;
			if (!T || T->GtdCents != R.Gtd)
			{
				ChampBad += " " + Id + " (" + (T ? std::to_string(T->GtdCents) : std::string("missing")) + ", the table's " + std::to_string(R.Gtd) + ")";
			}
		}
		const Chips Total = F->Which == ss::fest::Brand::Pennant ? 28110000 : 199130000;
		Chips Sum = 0, Listed = -1;
		for (const ss::net::EventTemplate* T : FestNetEditionTemplates(*F))
		{
			Sum += T->Series == F->SeriesId ? T->GtdCents * static_cast<Chips>(FestNetRuns(*T).size()) : 0;
		}
		for (const ss::net::SeriesInfo& S : N.Series())
		{
			Listed = S.Id == F->SeriesId ? S.GtdCents : Listed;
		}
		if (Sum != Total || Listed != Total)
		{
			ChampBad += " " + F->Name + "'s edition GtdCents " + std::to_string(Listed) + " (its templates sum to " + std::to_string(Sum) + ", the table's " + std::to_string(Total) + ")";
		}
	}
	Expect(Editions == 4 && Matched == 24 && ChampBad.empty(), "the championships' headline guarantees match §2.4.2's W5 table to the dollar in 2027 and 2028 (PENNANT $8,000 "
															   "$2,600 $21,000 $5,000 $24,000 $78,000; REIGN $87,000 $12,000 $190,000 $35,000 $170,000 $670,000) and the "
															   "editions' GtdCents are $281,100 and $1,991,300" + ChampBad);

	// The Summons and mega-rcop: no guarantee, and no seat count promised in their blurbs, seats text or listing.
	int Doors = 0;
	std::string DoorBad;
	for (const ss::net::SiteId S : {ss::net::SiteId::RunItTwice, ss::net::SiteId::RiverLine})
	{
		const ss::net::Network& N = ss::net::Site(S);
		for (const ss::net::EventTemplate& T : N.Templates())
		{
			const bool Summons = S == ss::net::SiteId::RunItTwice && T.Fmt == ss::net::Format::Satellite && T.Name.find("The Summons") != std::string::npos;
			if (!Summons && T.Id != "mega-rcop")
			{
				continue;
			}
			++Doors;
			ss::net::EventInstance E;
			const bool Listed = N.Next(T.Id, T.OnlyDay >= 0 ? static_cast<double>(T.OnlyDay) * ss::net::MinutesPerDay - 1.0 : FestNetNightOne, E);
			const std::string Label = Listed ? N.Listing(E).GuaranteeLabel : std::string();
			if (T.GtdCents != 0 || FestNetSeatCount(T.Blurb) || FestNetSeatCount(T.Seats) || Label.find("GTD") != std::string::npos)
			{
				DoorBad += " " + T.Id + " (GTD " + std::to_string(T.GtdCents) + ", blurb \"" + T.Blurb + "\", seats \"" + T.Seats + "\", label \"" + Label + "\")";
			}
		}
	}
	std::printf("festivals: guarantees: %d rival festival headliners and %d numbered events at their shares of the median pool; the championships' %d rows in %d editions; "
				"%d no-guarantee doors (The Summons, mega-rcop)\n",
		Headliners, Numbered, Matched, Editions, Doors);
	Expect(Doors >= 2 && DoorBad.empty(), "The Summons and mega-rcop carry no guarantee and promise no seat count in their blurbs, seats text or listing (" + std::to_string(Doors) +
											  " templates)" + DoorBad);
}

/** RiverLine's festival templates at FN-5 (G1's juice changes none of them): their count and the FNV-1a of "id TAB GtdCents LF". */
const int FestNetRiverLineFestCount = 4860;
const uint64_t FestNetRiverLineFestPin = 0x5d06510bd274e9afull;

/** SPEC v1.7 §2.4.1, §2.15.1: no RiverLine template carries added money or the Main curve, and RiverLine's festival templates keep
 *  today's guarantees (every RiverLine festival's series events and satellites, ids and GtdCents pinned). Planted: Added on the Ring
 *  Rush Main. */
void FestNoRiverLineJuiceNet()
{
	const ss::net::Network& R = ss::net::Site(ss::net::SiteId::RiverLine);
	int Juiced = 0;
	std::string First;
	for (const ss::net::EventTemplate& T : R.Templates())
	{
		if (T.AddedCents != 0 || T.Curve != ss::PayoutCurve::Standard)
		{
			++Juiced;
			First += First.size() < 300 ? " " + T.Id : "";
		}
	}
	std::set<std::string> Series, Satellites;
	for (const ss::fest::Festival& F : ss::fest::All())
	{
		if (F.Site == ss::net::SiteId::RiverLine)
		{
			Series.insert(F.SeriesId);
			Satellites.insert(F.Satellites.begin(), F.Satellites.end());
		}
	}
	uint64_t H = 14695981039346656037ull;
	int Count = 0;
	ss::Chips Sum = 0;
	for (const ss::net::EventTemplate& T : R.Templates())
	{
		if (!Series.count(T.Series) && !Satellites.count(T.Id))
		{
			continue;
		}
		++Count;
		Sum += T.GtdCents;
		for (const char Ch : T.Id + "\t" + std::to_string(T.GtdCents) + "\n")
		{
			H = (H ^ static_cast<unsigned char>(Ch)) * 1099511628211ull;
		}
	}
	std::printf("festivals: RiverLine's festival templates: %d (series events and satellites), guarantees $%.2f in all, pin %s\n", Count, static_cast<double>(Sum) / 100.0,
		SitesHexNet(H).c_str());
	Expect(Juiced == 0, "no RiverLine template carries added money or the Main curve (" + std::to_string(Juiced) + " do:" + First + ")");
	Expect(Count == FestNetRiverLineFestCount && H == FestNetRiverLineFestPin, "RiverLine's festival templates keep today's guarantees (" + std::to_string(Count) +
																				  " templates, pin " + SitesHexNet(H) + ")");
}

/** SPEC v1.7 §2.4.1, §2.4.4, §2.15.1: for a rival Main with added money and a short field, OverlayOf, ProjectedOverlay, the rival's
 *  News overlays and the world's SiteOverlay report the base overlay (the guarantee less what the field put in), never one from a pool
 *  that holds Added: every short rival Main of 2026-2040 as the game draws it, and FEIGN 2026's Main cut to a field 20% short by hand
 *  (resolved on a Blank world for SiteOverlay). Planted: ProjectedOverlay from E.Pool. */
void FestOverlayIgnoresAddedNet()
{
	using ss::Chips;
	namespace show = ss::site::show;
	ss::net::HeroStats Hero;
	int Short = 0, NewsSeen = 0;
	std::string Bad;
	const std::vector<FestNetMain> Mains = FestNetMains(2026, 2040);
	for (const FestNetMain& M : Mains)
	{
		const Chips Base = M.T->GtdCents - M.Contribution * static_cast<Chips>(M.E.Entries);
		if (Base <= 0)
		{
			continue;
		}
		++Short;
		const double After = M.E.Start + M.E.Duration + 1.0;
		bool Actual = false;
		const Chips Projected = show::ProjectedOverlay(*M.N, M.E, M.E.Entries, After, Actual);
		const show::OverlayView V = show::OverlayOf(*M.N, M.E, After);
		std::string Why = !Actual || Projected != Base ? "ProjectedOverlay " + std::to_string(Projected) : std::string();
		Why += !V.Actual || V.Cents != Base ? " OverlayOf " + std::to_string(V.Cents) : std::string();
		const std::string Name = M.N->NameOf(M.E);
		for (const ss::net::NewsItem& It : M.N->News(After, Hero, 1000))
		{
			if (It.Tag == "OVERLAY" && (It.Title.find(Name) != std::string::npos || It.Body.find(Name) != std::string::npos))
			{
				++NewsSeen;
				Why += It.Amount != std::llround(static_cast<double>(Base) / 100.0) * 100 ? " News " + std::to_string(It.Amount) : std::string();
			}
		}
		if (!Why.empty())
		{
			Bad += " " + M.F->Name + " (" + std::to_string(M.E.Entries) + " entries, base overlay " + std::to_string(Base) + ":" + Why + ")";
		}
	}
	// FEIGN 2026's Main 20% short by hand: the pool Make would give that field (the guarantee + Added), each of the four reads.
	ss::world::World W;
	W.Create(7u, FestNetNightOne, ss::world::Start::Blank);
	int Posts = 0;
	for (const FestNetMain& M : Mains)
	{
		if (M.F->Which != ss::fest::Brand::Feign || M.F->Year != 2026)
		{
			continue;
		}
		ss::net::EventInstance Cut = M.E;
		Cut.Entries = static_cast<int>(static_cast<double>(M.T->GtdCents) * 0.8 / static_cast<double>(M.Contribution));
		Cut.Pool = std::max(M.T->GtdCents, M.Contribution * static_cast<Chips>(Cut.Entries)) + M.T->AddedCents;
		const Chips Base = M.T->GtdCents - M.Contribution * static_cast<Chips>(Cut.Entries);
		const double After = Cut.Start + Cut.Duration + 1.0;
		bool Actual = false;
		const Chips Projected = show::ProjectedOverlay(*M.N, Cut, Cut.Entries, After, Actual);
		const show::OverlayView V = show::OverlayOf(*M.N, Cut, After);
		ss::world::Pending P = ss::world::Sim::OnlineEvent(W, Cut);
		ss::world::Sim::Resolve(W, P);
		Chips Posted = -1;
		for (const ss::world::WorldEvent& Ev : W.Events())
		{
			if (Ev.Kind == ss::world::EventKind::SiteOverlay && Ev.What == P.Name)
			{
				++Posts;
				Posted = Ev.Amount;
			}
		}
		std::printf("festivals: FEIGN 2026's Main cut to %d entries (pool $%.2f with Added): base overlay $%.2f; ProjectedOverlay $%.2f, OverlayOf $%.2f, the world's "
					"SiteOverlay $%.2f\n",
			Cut.Entries, static_cast<double>(Cut.Pool) / 100.0, static_cast<double>(Base) / 100.0, static_cast<double>(Projected) / 100.0, static_cast<double>(V.Cents) / 100.0,
			static_cast<double>(Posted) / 100.0);
		if (!Actual || Projected != Base || V.Cents != Base || Posted != Base)
		{
			Bad += " FEIGN 2026's Main cut to " + std::to_string(Cut.Entries) + " (base " + std::to_string(Base) + ": ProjectedOverlay " + std::to_string(Projected) +
				", OverlayOf " + std::to_string(V.Cents) + ", SiteOverlay " + std::to_string(Posted) + ")";
		}
	}
	std::printf("festivals: overlays on %zu rival Mains of 2026-2040: %d closed short, %d of them in their site's News\n", Mains.size(), Short, NewsSeen);
	Expect(Short > 0 && NewsSeen > 0 && Posts == 1 && Bad.empty(), "OverlayOf, ProjectedOverlay, the rival's News and the world's SiteOverlay report a short Main's base "
																   "overlay, never one from a pool holding Added (" +
																	   std::to_string(Short) + " short Mains, " + std::to_string(NewsSeen) + " in News)" + Bad);
}

/** SPEC v1.7 §2.4.3, §2.15.1: RacePoints is net::Points under Standard (every field from 5 entries) and, under Main, net::Points inside
 *  PaidPlaces and 0 past it (net::Points' places 12-15% score nothing), on fields 2-3,000 at four buy-ins and on every rival Main of
 *  2026-2028. Planted: points for places 12-15%. */
void FestRacePointsNet()
{
	using ss::Chips;
	const ss::PayoutCurve Main = ss::PayoutCurve::Main;
	const Chips BuyIns[] = {110, 5500, 21500, 210000};
	int Places = 0, Cut = 0, SmallDiff = 0;
	std::string Bad;
	for (int N = 2; N <= 3000; N += N < 200 ? 1 : 37)
	{
		const int Paid = ss::PaidPlaces(N, Main);
		for (const Chips B : BuyIns)
		{
			for (int P = 1; P <= std::min(N, N / 5 + 3); ++P)
			{
				++Places;
				const double Points = ss::net::Points(P, N, B);
				const double Std = ss::fest::RacePoints(P, N, B, ss::PayoutCurve::Standard);
				const double Race = ss::fest::RacePoints(P, N, B, Main);
				SmallDiff += Std != Points && N < 5 ? 1 : 0;
				Cut += P > Paid && Points > 0.0 ? 1 : 0;
				if ((Std != Points && N >= 5) || Race != (P <= Paid ? Points : 0.0))
				{
					Bad += Bad.size() < 400 ? " " + std::to_string(P) + " of " + std::to_string(N) + " at " + std::to_string(B) + " (net::Points " + std::to_string(Points) +
							", Standard " + std::to_string(Std) + ", Main " + std::to_string(Race) + ")"
											: std::string();
				}
			}
		}
	}
	for (const FestNetMain& M : FestNetMains(2026, 2028))
	{
		const int Paid = ss::PaidPlaces(M.E.Entries, Main);
		const double Last = ss::fest::RacePoints(Paid, M.E.Entries, M.T->BuyInCents, M.T->Curve);
		const double Past = ss::fest::RacePoints(Paid + 1, M.E.Entries, M.T->BuyInCents, M.T->Curve);
		if (Last <= 0.0 || Past != 0.0 || ss::net::Points(Paid + 1, M.E.Entries, M.T->BuyInCents) <= 0.0)
		{
			Bad += " " + M.F->Name + "'s Main (place " + std::to_string(Paid) + " " + std::to_string(Last) + ", " + std::to_string(Paid + 1) + " " + std::to_string(Past) + ")";
		}
	}
	std::printf("festivals: race points on %d places (fields 2-3,000, four buy-ins): %d past the Main curve's paid places that net::Points scores; %d Standard differences under 5 "
				"entries\n",
		Places, Cut, SmallDiff);
	Expect(Cut > 0 && Bad.empty(), "RacePoints is net::Points under Standard and, under the Main curve, only for the paid places (0 past PaidPlaces; every rival Main's first "
								   "place out of the money scores nothing)" + Bad);
}

/** SPEC v1.7 §2.4.3 (the table talk's row), R-9, §2.15.1: a Main-curve result at PaidPlaces + 1 with no prize may claim the bubble and
 *  one at 15% + 1 may not (and the reverse under Standard); StoryText writes the 7th field only for Main and StoryOf reads it back, old
 *  six-field stories parse as Standard. Planted: the talk rule left at 15% (G/TableTalk.cpp:1419). */
void FestTalkBubbleCurveNet()
{
	namespace talk = ss::talk;
	const int N = 700;
	const int MainPaid = ss::PaidPlaces(N, ss::PayoutCurve::Main);
	const int StdPaid = ss::PaidPlaces(N, ss::PayoutCurve::Standard);
	const auto Result = [N](int Place, ss::PayoutCurve Curve) {
		talk::Result R;
		R.Day = 65;
		R.Event = "FEIGN 10: $55 MAIN EVENT";
		R.Place = Place;
		R.Entries = N;
		R.Prize = 0;
		R.Online = true;
		R.Curve = Curve;
		return R;
	};
	const auto Claims = [](const talk::Result& R) {
		ss::life::LiveVoice V;
		V.Name = "Festnet Voice";
		V.Story = talk::StoryText({R});
		talk::Context C;
		C.Self = &V;
		C.Vars = {{"Topic", "bubble"}, {"Record", "0"}, {"Day", "66"}};
		return !talk::Line(talk::Moment::Small, C).Text.empty();
	};
	const bool MainBubble = Claims(Result(MainPaid + 1, ss::PayoutCurve::Main));
	const bool MainAtStd = Claims(Result(StdPaid + 1, ss::PayoutCurve::Main));
	const bool StdBubble = Claims(Result(StdPaid + 1, ss::PayoutCurve::Standard));
	const bool StdAtMain = Claims(Result(MainPaid + 1, ss::PayoutCurve::Standard));
	std::printf("festivals: the bubble talk at %d entries: the Main curve pays %d (place %d claims it: %s; place %d: %s), Standard %d (place %d: %s; place %d: %s)\n", N, MainPaid,
		MainPaid + 1, MainBubble ? "yes" : "no", StdPaid + 1, MainAtStd ? "yes" : "no", StdPaid, StdPaid + 1, StdBubble ? "yes" : "no", MainPaid + 1, StdAtMain ? "yes" : "no");
	Expect(MainPaid == 84 && StdPaid == 105 && MainBubble && !MainAtStd && StdBubble && !StdAtMain,
		"a Main-curve result one past PaidPlaces with no prize may claim the bubble and one at 15% + 1 may not (Standard the other way round)");

	// The story's 7th field: written only for Main, read back; old stories (six fields) and a 7th of 0 read Standard.
	talk::Result A = Result(12, ss::PayoutCurve::Main);
	A.Prize = 41500;
	const talk::Result B = Result(StdPaid + 1, ss::PayoutCurve::Standard);
	const std::string Text = talk::StoryText({A, B});
	const std::vector<talk::Result> Back = talk::StoryOf(Text);
	const size_t Bar = Text.find('|');
	const std::string First = Text.substr(0, Bar), Second = Bar == std::string::npos ? std::string() : Text.substr(Bar + 1);
	const bool Fields = std::count(First.begin(), First.end(), '^') == 6 && std::count(Second.begin(), Second.end(), '^') == 5;
	const auto Same = [](const talk::Result& X, const talk::Result& Y) {
		return X.Day == Y.Day && X.Event == Y.Event && X.Place == Y.Place && X.Entries == Y.Entries && X.Prize == Y.Prize && X.Online == Y.Online && X.Curve == Y.Curve;
	};
	const std::vector<talk::Result> Old = talk::StoryOf("12^Embercrest Nightly $120^2^88^214000^0|9^RiverLine Daily $22^7^410^61000^1^0");
	const bool OldOk = Old.size() == 2 && Old[0].Curve == ss::PayoutCurve::Standard && Old[0].Place == 2 && Old[0].Prize == 214000 && Old[1].Curve == ss::PayoutCurve::Standard &&
		Old[1].Online && Old[1].Entries == 410;
	Expect(Fields && Back.size() == 2 && Same(Back[0], A) && Same(Back[1], B) && OldOk,
		"StoryText writes the curve as a 7th field only when it isn't Standard and StoryOf reads it back; old six-field stories (and a 7th of 0) read Standard (\"" + Text + "\")");
}

// ------------------------------------------------------------------ FestMarks, FestNews, FestRecordOf (F-net; FN-7)

/** The first mark of Kind at At labelled Label in Ms (nullptr: none). */
const ss::life::Mark* FestNetMark(const std::vector<ss::life::Mark>& Ms, int Kind, double At, const std::string& Label)
{
	for (const ss::life::Mark& M : Ms)
	{
		if (M.Kind == Kind && M.At == At && M.Label == Label)
		{
			return &M;
		}
	}
	return nullptr;
}

/** §2.9.1, §2.15.1 FestMarks (F-net's half; the "no SERIES mark for a festival series" half is F-world's FW-7, site::Marks' skip):
 *  fest::Marks draws every festival's FESTIVAL mark at FirstDay 00:00 and its MAIN mark at the Main's start, labelled "<Name> opens"
 *  and "<Name> Main Event" (U22), Where the host site's name, from T-30 (DayOf(Now) >= FirstDay - 30) and not a minute before, inside
 *  [From, To) only, for all ten brands (every edition to 2028 on the three sites); and Session::Marks merges them whatever accounts
 *  the hero holds: on Night One RCOP 2026's two (day 13 00:00, the Main's start on day 34) and nothing of FEIGN's, and on day 26 with
 *  no Ignite account FEIGN 2026's two as well. Planted: account-only marks (a festival marked only on a site the hero holds an
 *  account with: Night One's career holds RiverLine's alone). */
void FestMarksNet()
{
	namespace fest = ss::fest;
	namespace net = ss::net;
	std::string Bad;
	std::set<int> Brands;
	int Editions = 0;
	const double From = FestNetAt(-60);
	const double To = FestNetAt(ss::world::DayOn(2029, 1, 1));
	for (const fest::Festival& F : fest::All())
	{
		if (F.Year > 2028)
		{
			continue;
		}
		++Editions;
		Brands.insert(static_cast<int>(F.Which));
		const std::string Where = net::Def(F.Site).Name;
		const std::string Opens = F.Name + " opens";
		const std::string Main = F.Name + " Main Event";
		const double Day1 = FestNetAt(F.FirstDay);
		const double T30 = FestNetAt(F.FirstDay - fest::kAnnounceDays);
		const std::vector<ss::life::Mark> On = fest::Marks(From, To, T30);
		const std::vector<ss::life::Mark> Before = fest::Marks(From, To, T30 - 1.0);
		const ss::life::Mark* O = FestNetMark(On, fest::MarkFestival, Day1, Opens);
		const ss::life::Mark* M = FestNetMark(On, fest::MarkFestMain, F.MainStart, Main);
		const bool Early = FestNetMark(Before, fest::MarkFestival, Day1, Opens) || FestNetMark(Before, fest::MarkFestMain, F.MainStart, Main);
		// [From, To): the opening mark sits in a window that starts on its minute and not in one that ends there.
		const bool Edges = FestNetMark(fest::Marks(Day1, Day1 + 1.0, T30), fest::MarkFestival, Day1, Opens) && !FestNetMark(fest::Marks(Day1 - 1.0, Day1, T30), fest::MarkFestival, Day1, Opens);
		if ((!O || !M || O->Where != Where || M->Where != Where || Early || !Edges) && Bad.size() < 400)
		{
			Bad += "; " + F.Name + (O ? "" : " no FESTIVAL") + (M ? "" : " no MAIN") + (O && O->Where != Where ? " FESTIVAL at " + O->Where : "") + (M && M->Where != Where ? " MAIN at " + M->Where : "") +
				(Early ? " marked before T-30" : "") + (Edges ? "" : " outside [From, To)");
		}
	}
	Expect(Editions == 22 && Brands.size() == 10 && Bad.empty(),
		"FestMarks: every edition to 2028 (" + std::to_string(Editions) + ", " + std::to_string(Brands.size()) + " brands) is marked FESTIVAL on FirstDay 00:00 and MAIN at its Main's start from T-30, never before, at its host's name" + Bad);
	// Session::Marks: account or not.
	const fest::Festival* Rcop = fest::FestivalOf("rcop");
	const fest::Festival* Feign = fest::FestivalOf("ignite.feign26");
	std::string Session = "no RCOP 2026 or FEIGN 2026";
	if (Rcop && Feign)
	{
		ss::SessionHooks Quiet;
		ss::Session S(Quiet, "festmarks");
		const double Night = S.WorldMinutes();
		const std::vector<ss::life::Mark> First = S.Marks(Night, Night + 60.0 * FestNetDay);
		const bool RcopTwo = FestNetMark(First, fest::MarkFestival, FestNetAt(13), "RCOP 2026 opens") && FestNetMark(First, fest::MarkFestMain, Rcop->MainStart, "RCOP 2026 Main Event") &&
			Rcop->FirstDay == 13 && Rcop->MainDay == 34 && net::DayOf(Rcop->MainStart) == 34;
		// F-world's rewrite of session_seams.cpp:724 (fworld_to_lead_3, the lead's at integration) expects exactly these two in a new
		// career's first 60 days: FESTIVAL at day 13 00:00 and MAIN at day 34 15:00, both at "RiverLine".
		const std::vector<ss::life::Mark> Seams = S.Marks(0.0, 60.0 * FestNetDay);
		const bool SeamsTwo = Seams.size() == 2 && Seams[0].Kind == fest::MarkFestival && Seams[0].At == FestNetAt(13) && Seams[0].Where == "RiverLine" &&
			Seams[1].Kind == fest::MarkFestMain && Seams[1].At == FestNetAt(34, 15 * 60) && Seams[1].Where == "RiverLine";
		const bool NoFeign = SeamsTwo && !FestNetMark(First, fest::MarkFestival, FestNetAt(Feign->FirstDay), "FEIGN 2026 opens");
		bool Account = false;
		for (const ss::site::Account& A : S.Life.Sites.Accounts)
		{
			Account = Account || A.Site == net::SiteId::Ignite;
		}
		S.LobbyMinutes = FestNetAt(Feign->FirstDay - fest::kAnnounceDays) - FestNetAt(net::NightOneDay);
		const std::vector<ss::life::Mark> Later = S.Marks(S.WorldMinutes(), S.WorldMinutes() + 60.0 * FestNetDay);
		const bool FeignTwo = FestNetMark(Later, fest::MarkFestival, FestNetAt(Feign->FirstDay), "FEIGN 2026 opens") && FestNetMark(Later, fest::MarkFestMain, Feign->MainStart, "FEIGN 2026 Main Event");
		Session = RcopTwo && NoFeign && !Account && FeignTwo ? std::string()
			: std::string(RcopTwo ? "" : " RCOP 2026's two missing on Night One") + (NoFeign ? "" : " FEIGN marked on Night One, or not exactly RCOP 2026's two in days 0-60 (" + std::to_string(Seams.size()) + ")") + (Account ? " an Ignite account" : "") +
				(FeignTwo ? "" : " FEIGN 2026's two missing on day " + std::to_string(net::DayOf(S.WorldMinutes())));
	}
	Expect(Session.empty(), "FestMarks: the session's wall sees RCOP 2026's FESTIVAL (day 13 00:00) and MAIN (day 34) on Night One, and FEIGN 2026's two from day 26 with no Ignite account:" + Session);
}

/** The news items on N at Now titled Title. */
std::vector<ss::net::NewsItem> FestNetTitled(const ss::net::Network& N, double Now, const std::string& Title)
{
	std::vector<ss::net::NewsItem> Out;
	for (const ss::net::NewsItem& It : N.News(Now, ss::net::HeroStats(), 1000000))
	{
		if (It.Title == Title)
		{
			Out.push_back(It);
		}
	}
	return Out;
}

/** A festival's {Festival} in its News posts: the edition's name without its year (§2.11's PENNANT I sample). */
std::string FestNetBrandName(const ss::fest::Festival& F)
{
	const std::string Tail = " " + std::to_string(F.Year);
	return F.Name.size() > Tail.size() && F.Name.compare(F.Name.size() - Tail.size(), Tail.size(), Tail) == 0 ? F.Name.substr(0, F.Name.size() - Tail.size()) : F.Name;
}

/** Text ends with Tail. */
bool FestNetEndsWith(const std::string& Text, const std::string& Tail)
{
	return Text.size() >= Tail.size() && Text.compare(Text.size() - Tail.size(), Tail.size(), Tail) == 0;
}

/** Whole dollars, grouped ("$281,100"). */
std::string FestNetDollars(ss::Chips Cents)
{
	return "$" + ss::Grouped(static_cast<int64_t>(std::llround(static_cast<double>(Cents) / 100.0)));
}

/** §2.9.2, §2.11, §2.15.1 FestNews (F-net's half; a Mini win's head:fest:title is F-world's FestHeadline), on fresh networks with no
 *  world (a rival's launch is its LaunchDay): every edition to 2028 posts one announcement at T-30 10:00 on its host's page (a rival's
 *  never before its launch: FEIGN I's on day 28, its launch day, at 10:00), "<Brand> is coming to <Site>", none a minute before,
 *  RiverLine's in the ":rl" form ("is back on"); RCOP 2026 posts none and keeps its two scripted posts; the first-festival line only
 *  on each rival's first festival (FEIGN I, WRIT I), never on a championship; a championship's announcement carries its site's
 *  sentence (PENNANT I's body equals §2.11's sample word for word); opening night at FirstDay 09:00 (GRIT's nights form with its two
 *  show times); the rivals' "a week out" post never names a festival; and a rival's record posts are exactly the instances fest::
 *  RecordOf says set a record (the "ever" line only when Ever), every 2026-2027 rival edition walked. Planted: the record post without
 *  RecordOf; "It's {Site}'s first festival." appended to PENNANT I's announcement. */
void FestNewsNet()
{
	namespace fest = ss::fest;
	namespace net = ss::net;
	const net::Network Ig(net::Def(net::SiteId::Ignite));
	const net::Network Rit(net::Def(net::SiteId::RunItTwice));
	const net::Network Rl;
	auto NetOf = [&](net::SiteId S) -> const net::Network& { return S == net::SiteId::Ignite ? Ig : S == net::SiteId::RunItTwice ? Rit : Rl; };
	const std::vector<fest::Festival>& All = fest::All();
	// Each rival's first festival in a world that opened it on its LaunchDay.
	std::map<int, const fest::Festival*> FirstOf;
	for (const fest::Festival& F : All)
	{
		if (F.Site != net::SiteId::RiverLine && F.FirstDay >= net::Def(F.Site).LaunchDay && !FirstOf.count(static_cast<int>(F.Site)))
		{
			FirstOf[static_cast<int>(F.Site)] = &F;
		}
	}
	std::string Bad;
	std::string Pennant1, Reign1, GritOpen;
	int Announced = 0, Opened = 0, Firsts = 0, Champs = 0;
	for (const fest::Festival& F : All)
	{
		if (F.Year > 2028)
		{
			continue;
		}
		const net::Network& N = NetOf(F.Site);
		const net::SeriesInfo* Sr = N.FindSeries(F.SeriesId);
		const net::EventTemplate* Main = N.FindTemplate(F.MainId);
		if (!Sr || !Main)
		{
			Bad += "; " + F.Name + " not on its network";
			continue;
		}
		const bool Home = F.Site == net::SiteId::RiverLine;
		const std::string Brand = FestNetBrandName(F);
		const std::string Site = net::Def(F.Site).Name;
		const int Days = F.LastDay - F.FirstDay + 1;
		if (F.SeriesId != "rcop")
		{
			const double At = std::max(FestNetAt(F.FirstDay - fest::kAnnounceDays, 600), Home ? FestNetAt(F.FirstDay - fest::kAnnounceDays, 600) : FestNetAt(net::Def(F.Site).LaunchDay, 600));
			const std::string Title = Brand + " is coming to " + Site;
			const std::vector<net::NewsItem> Got = FestNetTitled(N, At, Title);
			const bool Early = !FestNetTitled(N, At - 1.0, Title).empty();
			const std::string Body = Got.size() == 1 ? Got[0].Body : std::string();
			const std::string Lead = Home ? Brand + " is back on " + ss::fest::DateInSentence(F.FirstDay, net::DayOf(At)) + ": " : Brand + " opens on " + ss::fest::DateInSentence(F.FirstDay, net::DayOf(At)) + ": ";
			const bool First = Body.find("first festival") != std::string::npos;
			const bool WantFirst = !Home && FirstOf[static_cast<int>(F.Site)] == &F && !fest::IsChampionship(F.Which);
			const std::string ChampLead = F.Site == net::SiteId::Ignite ? " Main Events at " : " It's the club's championship: Main Events at ";
			const bool Champ = Body.find(ChampLead) != std::string::npos;
			const bool WantChamp = !Home && fest::IsChampionship(F.Which);
			const bool Ok = Got.size() == 1 && Got[0].At == At && !Early && Got[0].Tag == "FESTIVAL" && Got[0].Kind == net::NewsKind::Series && Body.rfind(Lead, 0) == 0 &&
				First == WantFirst && Champ == WantChamp && (!WantFirst || FestNetEndsWith(Body, " It's " + Site + "'s first festival."));
			if (!Ok && Bad.size() < 600)
			{
				Bad += "; " + F.Name + " announcement at " + FestNetDate(net::DayOf(At)) + ": " + std::to_string(Got.size()) + " post(s)" + (Early ? ", one a minute early" : "") + " \"" + Body + "\"";
			}
			Announced += Got.size() == 1 ? 1 : 0;
			Firsts += First ? 1 : 0;
			Champs += Champ ? 1 : 0;
			Pennant1 = F.SeriesId == "ignite.pennant27" ? Body : Pennant1;
			Reign1 = F.SeriesId == "runittwice.reign27" ? Body : Reign1;
		}
		// Opening night.
		const double Open = FestNetAt(F.FirstDay, 540);
		const std::string OpenTitle = Brand + " opens tonight";
		const std::vector<net::NewsItem> Got = FestNetTitled(N, Open, OpenTitle);
		const bool Early = !FestNetTitled(N, Open - 1.0, OpenTitle).empty();
		const std::string Body = Got.size() == 1 ? Got[0].Body : std::string();
		const std::string Want = F.Which == fest::Brand::Grit ? "Night 1 of " + std::to_string(Days) + ". Two shows a night, at "
			: "Day 1 of " + std::to_string(Days) + ". " + std::to_string(Sr->Events) + " events, " + FestNetDollars(Sr->GtdCents) + " guaranteed. The Main Event is on " +
				ss::fest::DateInSentence(F.MainDay, F.FirstDay) + ".";
		const bool OpenOk = Got.size() == 1 && Got[0].At == Open && !Early && Got[0].Tag == "FESTIVAL" && (F.Which == fest::Brand::Grit ? Body.rfind(Want, 0) == 0 : Body == Want);
		if (!OpenOk && Bad.size() < 600)
		{
			Bad += "; " + F.Name + " opening night: " + std::to_string(Got.size()) + " post(s)" + (Early ? ", one a minute early" : "") + " \"" + Body + "\"";
		}
		Opened += Got.size() == 1 ? 1 : 0;
		GritOpen = F.SeriesId == "runittwice.grit27" ? Body : GritOpen;
		// A week out: the rivals' series post never names a festival (it was announced at T-30).
		if (!Home)
		{
			for (const net::NewsItem& It : N.News(FestNetAt(F.FirstDay - 7, 600), net::HeroStats(), 1000000))
			{
				if (It.Title.rfind(F.Name + " starts", 0) == 0 && Bad.size() < 600)
				{
					Bad += "; " + F.Name + " a week out: \"" + It.Title + "\"";
				}
			}
		}
	}
	Expect(Bad.empty() && Firsts == 2 && Champs == 4 && Announced > 20 && Opened > 20,
		"FestNews: one announcement an edition at T-30 10:00 (a rival's never before its launch), RiverLine's \"is back on\", the first-festival line on FEIGN I and WRIT I only (" + std::to_string(Firsts) +
			"), a championship's sentence on PENNANT and REIGN (" + std::to_string(Champs) + "), opening night at 09:00, no festival a week out (" + std::to_string(Announced) + " announced, " +
			std::to_string(Opened) + " opened)" + Bad);
	Expect(Pennant1 == "PENNANT opens on Mar 30: 39 events, $281,100 guaranteed, and a $215 Main Event with $10,000 added. Main Events at $2.20, $22 and $215 close it. Satellites start at $1.10.",
		"FestNews: PENNANT I's announcement equals §2.11's sample (\"" + Pennant1 + "\")");
	Expect(Reign1.find(" It's the club's championship: Main Events at ") != std::string::npos && Reign1.find("first festival") == std::string::npos && Reign1.find(", and satellites from $") != std::string::npos,
		"FestNews: REIGN I's announcement carries RunItTwice's championship sentence and no first-festival line (\"" + Reign1 + "\")");
	Expect(GritOpen.rfind("Night 1 of ", 0) == 0 && GritOpen.find(". Two shows a night, at 7 PM and 10 PM.") != std::string::npos, "FestNews: GRIT 2027's opening night names its two shows (\"" + GritOpen + "\")");
	// RCOP 2026 keeps its two scripted posts and has no announcement; its opening night is §2.2.2's computed guarantee.
	{
		std::string Got;
		bool Schedule = false, Raised = false, Announce = false;
		for (const net::NewsItem& It : Rl.News(FestNetAt(12, 23 * 60), net::HeroStats(), 1000000))
		{
			Schedule = Schedule || It.Title == "RCOP 2026: the schedule";
			Raised = Raised || It.Title == "RCOP Main Event guarantee raised to $25M";
			Announce = Announce || (It.Tag == "FESTIVAL" && It.Title.find("RCOP") != std::string::npos);
		}
		const std::vector<net::NewsItem> Open = FestNetTitled(Rl, FestNetAt(13, 9 * 60), "RCOP opens tonight");
		Got = Open.size() == 1 ? Open[0].Body : std::to_string(Open.size()) + " posts";
		Expect(Schedule && Raised && !Announce && Got == "Day 1 of 22. 154 events, $78,203,400 guaranteed. The Main Event is on Nov 8.",
			"FestNews: RCOP 2026 keeps its two scripted posts, posts no announcement, and opens on day 13 at 09:00 (\"" + Got + "\")");
	}
	// Records: on every 2026-2027 rival edition, the record posts are exactly the instances RecordOf names.
	int Instances = 0, Posts = 0, Ever = 0, Year = 0;
	std::string RecBad;
	for (const fest::Festival& F : All)
	{
		if (F.Site == net::SiteId::RiverLine || F.Year > 2027)
		{
			continue;
		}
		const net::Network& N = NetOf(F.Site);
		const std::string Site = net::Def(F.Site).Name;
		std::set<std::pair<double, std::string>> Want;
		for (const net::EventInstance& E : N.Window(FestNetAt(F.FirstDay), FestNetAt(F.LastDay + 1)))
		{
			if (N.TemplateOf(E).Series != F.SeriesId)
			{
				continue;
			}
			++Instances;
			const fest::FieldRecord R = fest::RecordOf(N, E, nullptr);
			if (R.Ever || R.Year)
			{
				Want.insert({E.Start + E.LateReg, N.NameOf(E) + " draws a record field|" + ss::Grouped(E.Entries) +
					(R.Ever ? " runners: the biggest field " + Site + " has ever had." : " runners: " + Site + "'s biggest field this year.")});
				Ever += R.Ever ? 1 : 0;
				Year += R.Ever ? 0 : 1;
			}
		}
		std::set<std::pair<double, std::string>> Seen;
		for (double Q = FestNetAt(F.FirstDay) + 3.0 * FestNetDay; Q < FestNetAt(F.LastDay + 5); Q += 3.0 * FestNetDay)
		{
			for (const net::NewsItem& It : N.News(Q, net::HeroStats(), 1000000))
			{
				if (It.Kind == net::NewsKind::Record && It.Tag == "FESTIVAL" && It.At >= FestNetAt(F.FirstDay) && It.At < FestNetAt(F.LastDay + 2))
				{
					Seen.insert({It.At, It.Title + "|" + It.Body});
				}
			}
		}
		Posts += static_cast<int>(Seen.size());
		if (Seen != Want && RecBad.size() < 400)
		{
			RecBad += "; " + F.Name + ": " + std::to_string(Seen.size()) + " posted, " + std::to_string(Want.size()) + " by RecordOf";
		}
	}
	std::printf("festivals: FestNews: %d rival festival instances 2026-2027, %d record posts (%d ever, %d this year)\n", Instances, Posts, Ever, Year);
	Expect(RecBad.empty() && Posts > 0 && Posts < Instances && Ever > 0 && Year > 0,
		"FestNews: a rival's record posts are exactly the instances fest::RecordOf names, the ever line only when Ever (" + std::to_string(Posts) + " of " + std::to_string(Instances) + ")" + RecBad);
}

/** One sample's brute force (FestRecordOf): the largest field among Sorted's instances (Window's, start order) that started from
 *  From up to E's start, E left out. */
int FestNetBruteMax(const std::vector<ss::net::EventInstance>& Sorted, const ss::net::EventInstance& E, double From)
{
	int Most = 0;
	for (const ss::net::EventInstance& X : Sorted)
	{
		if (X.Start > E.Start)
		{
			break;
		}
		Most = X.Start >= From && X.Id != E.Id ? std::max(Most, X.Entries) : Most;
	}
	return Most;
}

/** §2.4.5, §2.14, §2.15.1 FestRecordOf, on fresh rival networks with no world (the launch is the LaunchDay): planted, a field that
 *  beats its year but not Ignite's first Big Top reads Year true, Ever false: since v1.6 PENNANT 2027's General Admission (2,600
 *  planned) sets 2027's best before Ignite's second Big Top, so FEIGN 2027's Big Top reads neither (printed) and the case is planted on
 *  2027's first mega, General Admission, between 2027's best before it and FEIGN 2026's Big Top (§2.4.5's v1.6 note); a 2028 instance planted
 *  between the best of its last 365 days and the best ever reads Ever false; a brute force over Window from the launch agrees with
 *  RecordOf on 200 sampled instances (100 a rival, 2026-2028, festival and plain); RiverLine claims none. The budget (§2.14,
 *  RecordOf <= 80 ms once a session per rival, in Prewarm): FillDayMax's walk to day 730 a rival is timed and printed, and the cost
 *  at day 0 (Ignite opens on day 28); asserted under SS_PERF=1 only. Planted: a 365-day window. */
void FestRecordOfNet()
{
	namespace fest = ss::fest;
	namespace net = ss::net;
	std::string Bad;
	int Sampled = 0, SampleEver = 0, SampleYear = 0;
	std::string Budget;
	bool PlantedYear = false, Planted365 = false;
	std::string PlantNote;
	for (const net::SiteId S : {net::SiteId::Ignite, net::SiteId::RunItTwice})
	{
		const auto T0 = std::chrono::steady_clock::now();
		const net::Network N(net::Def(S));
		const auto T1 = std::chrono::steady_clock::now();
		N.Prewarm(FestNetAt(0));
		const auto T2 = std::chrono::steady_clock::now();
		N.Prewarm(FestNetAt(730));
		const auto T3 = std::chrono::steady_clock::now();
		const double Day0 = std::chrono::duration<double, std::milli>(T2 - T1).count();
		const double Walk = std::chrono::duration<double, std::milli>(T3 - T2).count();
		std::printf("festivals: FestRecordOf: %s FillDayMax in Prewarm: day 0 %.3f ms, the walk to day 730 %.1f ms (budget 80 ms; the network built in %.0f ms)\n", net::Def(S).Name.c_str(), Day0, Walk,
			std::chrono::duration<double, std::milli>(T1 - T0).count());
		Budget += " " + std::string(net::Def(S).Name) + " " + std::to_string(Walk) + " ms";
		if (std::getenv("SS_PERF"))
		{
			Expect(Walk <= 80.0 && Day0 <= 1.0, "FestRecordOf: RecordOf's day maxima cost <= 80 ms a rival to day 730 in Prewarm and nothing at day 0 (" + std::string(net::Def(S).Name) + ": " +
				std::to_string(Walk) + " ms, " + std::to_string(Day0) + " ms)");
		}
		const double Launch = FestNetAt(net::Def(S).LaunchDay);
		const std::vector<net::EventInstance> Sorted = N.Window(Launch, FestNetAt(ss::world::DayOn(2029, 1, 1)));
		auto Brute = [&](const net::EventInstance& E) {
			fest::FieldRecord R;
			const double Jan = std::max(Launch, FestNetAt(ss::world::DayOn(ss::world::YearOf(net::DayOf(E.Start)), 1, 1)));
			R.Ever = E.Entries > FestNetBruteMax(Sorted, E, Launch);
			R.Year = E.Entries > FestNetBruteMax(Sorted, E, Jan);
			return R;
		};
		// 200 samples: 60 festival and 40 plain instances a rival, by stride over 2026-2028.
		std::vector<size_t> Fest, Plain;
		for (size_t I = 0; I < Sorted.size(); ++I)
		{
			const std::string& Series = N.TemplateOf(Sorted[I]).Series;
			(fest::FestivalOf(Series) ? Fest : Plain).push_back(I);
		}
		std::vector<size_t> Picks;
		for (size_t K = 0; K < 60 && !Fest.empty(); ++K)
		{
			Picks.push_back(Fest[K * Fest.size() / 60]);
		}
		for (size_t K = 0; K < 40 && !Plain.empty(); ++K)
		{
			Picks.push_back(Plain[K * Plain.size() / 40]);
		}
		for (size_t I : Picks)
		{
			const net::EventInstance& E = Sorted[I];
			const fest::FieldRecord Got = fest::RecordOf(N, E, nullptr);
			const fest::FieldRecord Want = Brute(E);
			++Sampled;
			SampleEver += Got.Ever ? 1 : 0;
			SampleYear += Got.Year ? 1 : 0;
			if ((Got.Ever != Want.Ever || Got.Year != Want.Year) && Bad.size() < 400)
			{
				Bad += "; " + N.NameOf(E) + " on " + FestNetDate(net::DayOf(E.Start)) + " (" + std::to_string(E.Entries) + "): RecordOf " + (Got.Ever ? "ever" : "-") + "/" + (Got.Year ? "year" : "-") +
					", brute " + (Want.Ever ? "ever" : "-") + "/" + (Want.Year ? "year" : "-");
			}
		}
		if (S != net::SiteId::Ignite)
		{
			continue;
		}
		// Planted (§2.15.1, v1.6): a field between its year's best and Ignite's first Big Top (FEIGN 2026's), on 2027's first mega.
		auto Find = [&](const char* Series, const char* Name) -> const net::EventInstance* {
			for (const net::EventInstance& E : Sorted)
			{
				if (N.TemplateOf(E).Series == Series && N.TemplateOf(E).Name.find(Name) != std::string::npos)
				{
					return &E;
				}
			}
			return nullptr;
		};
		const net::EventInstance* Top1 = Find("ignite.feign26", "The Big Top");
		const net::EventInstance* Top2 = Find("ignite.feign27", "The Big Top");
		const net::EventInstance* Mega = Find("ignite.pennant27", "General Admission");
		if (Top1 && Top2 && Mega)
		{
			const fest::FieldRecord Second = fest::RecordOf(N, *Top2, nullptr);
			const int YearMax = FestNetBruteMax(Sorted, *Mega, FestNetAt(ss::world::DayOn(2027, 1, 1)));
			const int EverMax = FestNetBruteMax(Sorted, *Mega, Launch);
			net::EventInstance P = *Mega;
			P.Entries = YearMax + 1;
			const fest::FieldRecord R = fest::RecordOf(N, P, nullptr);
			PlantedYear = YearMax + 1 <= Top1->Entries && Top1->Entries <= EverMax && R.Year && !R.Ever;
			PlantNote += " FEIGN 2026's Big Top " + std::to_string(Top1->Entries) + ", FEIGN 2027's " + std::to_string(Top2->Entries) + " (" + (Second.Ever ? "ever" : "-") + "/" +
				(Second.Year ? "year" : "-") + ", 2027's best before it " + std::to_string(FestNetBruteMax(Sorted, *Top2, FestNetAt(ss::world::DayOn(2027, 1, 1)))) + "); PENNANT 2027's General Admission " +
				std::to_string(Mega->Entries) + " planted at " + std::to_string(P.Entries) + " (2027's best before it " + std::to_string(YearMax) + ", the best ever " + std::to_string(EverMax) + "): " +
				(R.Ever ? "ever" : "-") + "/" + (R.Year ? "year" : "-") + ";";
		}
		// Planted: the latest 2028 festival instance whose best ever was set more than 365 days before it.
		for (auto It = Sorted.rbegin(); It != Sorted.rend() && !Planted365; ++It)
		{
			const net::EventInstance& E = *It;
			if (!fest::FestivalOf(N.TemplateOf(E).Series))
			{
				continue;
			}
			const int EverMax = FestNetBruteMax(Sorted, E, Launch);
			const int Last365 = FestNetBruteMax(Sorted, E, E.Start - 365.0 * FestNetDay);
			if (Last365 + 1 >= EverMax)
			{
				continue;
			}
			net::EventInstance P = E;
			P.Entries = Last365 + 1;
			const fest::FieldRecord R = fest::RecordOf(N, P, nullptr);
			Planted365 = !R.Ever;
			PlantNote += " " + N.NameOf(E) + " on " + FestNetDate(net::DayOf(E.Start)) + " planted at " + std::to_string(P.Entries) + " (its last 365 days' best " + std::to_string(Last365) +
				", the best ever " + std::to_string(EverMax) + "): " + (R.Ever ? "ever" : "-") + ";";
			break;
		}
	}
	std::printf("festivals: FestRecordOf: %d samples (%d ever, %d this year);%s\n", Sampled, SampleEver, SampleYear, PlantNote.c_str());
	Expect(Sampled == 200 && Bad.empty() && SampleEver > 0 && SampleYear > SampleEver, "FestRecordOf: RecordOf agrees with a brute force over Window from the launch on 200 sampled instances" + Bad);
	Expect(PlantedYear, "FestRecordOf: a field that beats its year but not Ignite's first Big Top (planted on PENNANT 2027's General Admission) reads Year true, Ever false;" + PlantNote);
	Expect(Planted365, "FestRecordOf: a 2028 field above its last 365 days' best but below the best ever is no \"ever\";" + PlantNote);
	// RiverLine never claims a record (§2.4.5).
	{
		const net::Network& Rl = net::Shared();
		const fest::Festival* Rcop = fest::FestivalOf("rcop");
		net::EventInstance Main;
		const bool Found = Rcop && Rl.Next(Rcop->MainId, FestNetAt(Rcop->MainDay), Main);
		Main.Entries = 10000000;
		const fest::FieldRecord R = Found ? fest::RecordOf(Rl, Main, nullptr) : fest::FieldRecord();
		Expect(Found && !R.Ever && !R.Year, "FestRecordOf: RiverLine claims no record (RCOP 2026's Main at 10,000,000 runners)");
	}
	(void)Budget;
}

/** fest::All()'s first build, timed once the networks exist (SPEC §2.14: built once, <= 1 ms; asserted under SS_PERF=1 only). */
void FestNetAllBudget()
{
	namespace net = ss::net;
	for (int S = 0; S < static_cast<int>(net::SiteId::Count); ++S)
	{
		net::Site(static_cast<net::SiteId>(S));
	}
	const auto T0 = std::chrono::steady_clock::now();
	const std::vector<ss::fest::Festival>& All = ss::fest::All();
	const double Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count();
	int BySite[static_cast<int>(net::SiteId::Count)] = {};
	for (const ss::fest::Festival& F : All)
	{
		++BySite[static_cast<int>(F.Site)];
	}
	std::printf("festivals: fest::All() built in %.3f ms (budget 1 ms): %zu editions (RiverLine %d, Ignite %d, RunItTwice %d)\n", Ms, All.size(),
		BySite[static_cast<int>(net::SiteId::RiverLine)], BySite[static_cast<int>(net::SiteId::Ignite)], BySite[static_cast<int>(net::SiteId::RunItTwice)]);
	if (std::getenv("SS_PERF"))
	{
		Expect(Ms <= 1.0, "fest::All() is built once in at most 1 ms (" + std::to_string(Ms) + " ms)");
	}
}
} // namespace

void SegNet()
{
	FestNetAllBudget();
	if (Only("FestCalendarOneStage"))
	{
		FestCalendarOneStageNet();
	}
	if (Only("FestCalendarFacts"))
	{
		FestCalendarFactsNet();
	}
	if (Only("FestChampTitles"))
	{
		FestChampTitlesNet();
	}
	// FN-3: the championships' three checks (F-net's halves).
	if (Only("FestMajorFloor"))
	{
		FestMajorFloorNet();
	}
	if (Only("FestLadderSeats"))
	{
		FestLadderSeatsNet();
	}
	if (Only("FestMainTicket"))
	{
		FestMainTicketNet();
	}
	// G1, FN-6: tickets and seats.
	if (Only("FestNextForRepeats"))
	{
		FestNextForRepeatsNet();
	}
	if (Only("FestSeats"))
	{
		FestSeatsNet();
	}
	// G1, FN-7: marks, News and the record field.
	if (Only("FestMarks"))
	{
		FestMarksNet();
	}
	if (Only("FestNews"))
	{
		FestNewsNet();
	}
	if (Only("FestRecordOf"))
	{
		FestRecordOfNet();
	}
	// FN-2a: the rival festivals (FEIGN and GRIT) and the copy.
	if (Only("FestTemplatesAppended"))
	{
		FestTemplatesAppendedNet();
	}
	if (Only("FestStartsApart"))
	{
		FestStartsApartNet();
	}
	if (Only("FestRivalCapG1"))
	{
		FestRivalCapG1Net();
	}
	if (Only("FestNamesThemed"))
	{
		FestNamesThemedNet();
	}
	if (Only("FestCodes"))
	{
		FestCodesNet();
	}
	if (Only("FestNamesScreened"))
	{
		FestNamesScreenedNet();
	}
	if (Only("FestCopy"))
	{
		FestCopyNet();
	}
	// G1 polish (g1p-copy, item 11).
	if (Only("FestCountRows"))
	{
		FestCountRowsNet();
	}
	// FN-4a: RiverLine's dress.
	if (Only("FestRcopGtdSum"))
	{
		FestRcopGtdSumNet();
	}
	if (Only("FestBlurbsFilled"))
	{
		FestBlurbsFilledNet();
	}
	// FN-4b: RCOP's and the six names' strings kept.
	if (Only("FestRcopKept"))
	{
		FestRcopKeptNet();
	}
	// FN-5: the juice.
	if (Only("FestCurveShape"))
	{
		FestCurveShapeNet();
	}
	if (Only("FestCurveAgrees"))
	{
		FestCurveAgreesNet();
	}
	if (Only("FestAddedMoneyAgrees"))
	{
		FestAddedMoneyAgreesNet();
	}
	if (Only("FestGuarantees"))
	{
		FestGuaranteesNet();
	}
	if (Only("FestNoRiverLineJuice"))
	{
		FestNoRiverLineJuiceNet();
	}
	if (Only("FestOverlayIgnoresAdded"))
	{
		FestOverlayIgnoresAddedNet();
	}
	if (Only("FestRacePoints"))
	{
		FestRacePointsNet();
	}
	if (Only("FestTalkBubbleCurve"))
	{
		FestTalkBubbleCurveNet();
	}
}
} // namespace festivals_test

int main(int Argc, char** Argv)
{
	festivals_test::FestFilter = Argc > 1 ? Argv[1] : nullptr;
	const auto T0 = std::chrono::steady_clock::now();
	festivals_test::SegNet();
	festivals_test::SegWorld();
	const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count();
	if (festivals_test::FestFilter && festivals_test::FestFilterHits == 0)
	{
		std::printf("FAILED: no check is named %s\n", festivals_test::FestFilter);
		return 2;
	}
	std::printf("festivals tests%s%s%s: %d checks, %d failed in %.1fs\n", festivals_test::FestFilter ? " (only " : "", festivals_test::FestFilter ? festivals_test::FestFilter : "",
		festivals_test::FestFilter ? ")" : "", festivals_test::Checks, festivals_test::Failures, Seconds);
	return festivals_test::Failures == 0 ? 0 : 1;
}
