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
#include "../../Source/ShortStackCore/Private/ShortStack/Game/SitesDetail.h"
#include "CopyLint.h"
#include "Trademarks.h"
#include "ShortStack/Common.h"
#include "ShortStack/Game/Festival.h"
#include "ShortStack/Game/Lobby.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Tournament.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

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
	// T-7 holds its week-out post.
	const net::Network& Ig = net::Site(net::SiteId::Ignite);
	net::HeroStats NoHero;
	int FeignPosts = 0;
	for (const net::NewsItem& It : Ig.News(FestNetAt(56 - 7 + 1, 12 * 60), NoHero, 60))
	{
		FeignPosts += It.Kind == net::NewsKind::Series && (It.Tag == "FEIGN" || It.Title.find("FEIGN") != std::string::npos) ? 1 : 0;
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

// ------------------------------------------------------------------ FestLadderSeats (F-net)

/** §2.3.2's ladder (v1.6), every PENNANT and REIGN edition: Step k's SeatTicket is Step k+1's ticket key; both of a step's templates
 *  take one key, the first run's template id (-s1a, -s2a, -s3a); Step 3, the Mega and the Last Chance seat the Main ticket; the seats
 *  texts and values follow the next rung ("Step 2 tickets ($5.50)" at $5.50, "PENNANT Main Event seats" at $215) and SeatValue reads
 *  them by ticket; walked with FinalTarget's rule every satellite's seat ends at the Main ticket, which NextFor resolves through
 *  Takes to the edition's Main; a Step 2 ticket won on day 8 registers on day 9's Step 2 and, unused, on day 11's; Festival::Satellites
 *  holds the eight in ladder order. (SeatsWon's Main-ticket filter is FN-6's half.) Planted: a step keyed <series>-s2 (FinalTarget
 *  stops at Step 2). */
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
		}
		if (!Why.empty() && Bad.empty())
		{
			Bad = ": " + F->Name + ": " + Why;
		}
	}
	Expect(Editions == 28 && Bad.empty(),
		"every PENNANT and REIGN edition's ladder (" + std::to_string(Editions) + "): Step 1 seats Step 2's key (-s2a), Step 2 Step 3's (-s3a), Step 3, the Mega and the Last "
		"Chance the Main ticket; a step's two templates take its first run's id; seats texts and values from the next rung (\"Step 2 tickets ($5.50)\"); FinalTarget walks every "
		"seat to the Main ticket and NextFor finds the Main through it; a Step 2 ticket from day 8 plays day 9's or day 11's Step 2; Satellites in ladder order" + Bad);
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

// ------------------------------------------------------------------ FestNamesScreened (F-net)

/** §2.11, §2.15.1: the festival names, titles, taglines, trophies (TrophyName: P1-P12 exactly), satellites, headliners and every
 *  string fest::Inventory lists (the six rival festivals' from FN-2b), every room word and the ten brands' keys (the rival names, §2.1.3: the
 *  ids follow the names) pass both trademark tiers with PA-1's entries; lowercase "push" is never flagged; no inventory string holds
 *  "Ignite Series" or "Ignite Championship". Planted: a deny entry "writ" added to the source tier (scratch Trademarks.h); the
 *  PENNANT trophy answered for its title object. */
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

/** §1.2 items 2-5, 9-10, 12-13, §2.11: every fest::Inventory entry (the six rival festivals' from FN-2b: names, titles, taglines, event names,
 *  blurbs and seats texts) by its kind (label: no closing stop, 200; sentence: a stop, 600; the F-world kinds as FW-1's lint), filled
 *  from the samples (copylint's, sites_main's SitesCopy values and fworld_to_fnet_1's) through CopyLint, both trademark tiers, the
 *  design words after the fill, U1, "players" (X1), DESIGN §2.6's avoided words, "marquee" and "pavilion", the glyph rule (ASCII and
 *  · — – … é ’) and FestChampWord; no rival festival name, Short, Title, tagline, event name, blurb, seats text or trophy holds
 *  "championship"; every {Festival} template filled again with "The Championship Online 2027" holds no "the The" and every {Date} a
 *  next year's date with its year; NameAfterArticle and DateInSentence answer §1.2 item 13's forms. Planted: "players" in a count;
 *  "GRIT Championship" as a Title; "Ignite's championship" in a tagline; NameAfterArticle and DateInSentence left at H0's stubs;
 *  PUSH's tagline without its closing stop (FN-2b). */
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
		{"MonthB", "August"}};
	for (const auto& Kv : Extra)
	{
		Vars[Kv.first] = Kv.second;
	}
	const char* const Avoided[] = {"spark", "flame", "blaze", "wildfire", "ember", "crest", "heat", "cold", "stamina", "momentum", "energy", "board", "watchlist", "headliners",
		"beef", "hall", "class", "boom", "millions", "jackpot", "run good", "supernova", "premier", "marquee", "pavilion"};
	int Entries = 0;
	int Problems = 0;
	std::string First;
	// Today (Nov 12, 2026's world day) and a day in the next world year, for the second fill's {Date}.
	const int Today = ss::world::DayOn(2026, 11, 12);
	const std::string NextYear = fest::DateInSentence(ss::world::DayOn(2027, 3, 30), Today);
	for (const std::string& Entry : fest::Inventory())
	{
		++Entries;
		const size_t Tab = Entry.find('\t');
		const std::string Kind = Tab == std::string::npos ? std::string() : Entry.substr(0, Tab);
		const std::string Text = Tab == std::string::npos ? Entry : Entry.substr(Tab + 1);
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
		const std::string Filled = FestNetFill(Text, Vars);
		const bool Ends = copylint::EndsWith(Filled, ".") || copylint::EndsWith(Filled, "!") || copylint::EndsWith(Filled, "?");
		std::string Why = Known ? std::string() : " an unknown kind \"" + Kind + "\"";
		const std::string Lint = copylint::Check(Filled + (Stop || Ends ? "" : "."), Cap + (Stop || Ends ? 0 : 1), {});
		Why += Lint.empty() ? std::string() : " " + Lint;
		Why += !Stop && Ends && Kind != "text" ? std::string(" a label ends in a stop") : std::string();
		for (const char* Hit : {trademarks::Find(Filled, trademarks::kSourceScan), trademarks::Find(Filled, trademarks::kCopyScan), trademarks::Find(Filled, trademarks::kDesignWords)})
		{
			Why += Hit ? std::string(" trademark or design word: ") + Hit : std::string();
		}
		const std::string Low = trademarks::Lower(Filled);
		for (const char* W : {"ai", "bot", "bots", "autoplay", "autopilot", "players"})
		{
			Why += trademarks::HasWord(Low, W) ? std::string(" U1/X1: ") + W : std::string();
		}
		for (const char* W : Avoided)
		{
			Why += trademarks::HasWord(Low, W) ? std::string(" avoided: ") + W : std::string();
		}
		Why += Low.find("coop") != std::string::npos ? " a *COOP form" : "";
		Why += FestNetChampWord(Filled) ? " FestChampWord" : "";
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
		if (!Why.empty())
		{
			++Problems;
			First = First.empty() ? ": \"" + Filled + "\" [" + Kind + "]" + Why : First;
			std::printf("COPY (festivals) \"%s\" [%s]:%s\n", Filled.c_str(), Kind.c_str(), Why.c_str());
		}
	}
	Expect(Entries > 0 && Problems == 0,
		"fest::Inventory's " + std::to_string(Entries) + " entries pass the copy lint by kind, both trademark tiers, the design words, U1, X1, the avoided words, the glyph rule, "
		"FestChampWord and the article and year rules (" + std::to_string(Problems) + " problems)" + First);

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
	// FN-4a: RiverLine's dress.
	if (Only("FestRcopGtdSum"))
	{
		FestRcopGtdSumNet();
	}
	if (Only("FestBlurbsFilled"))
	{
		FestBlurbsFilledNet();
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
