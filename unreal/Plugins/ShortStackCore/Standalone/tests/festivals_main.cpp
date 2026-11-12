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
#include <chrono>
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
 *  FN-2a, FN-3 and FN-2b build them ("ten a year from 2027" is FN-2b's). Planted: RCOP 2027 opening Oct 14 (inside Micro Madness). */
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

	// FN-2a: the rival festivals built so far, each on its host every year from its first edition to 2040 (FEIGN on Ignite from 2026,
	// GRIT on RunItTwice from 2027; PUSH, WRIT, PENNANT and REIGN join at FN-2b and FN-3, and "ten a year from 2027" with them).
	struct Rival
	{
		fest::Brand Which;
		net::SiteId Site;
		int FirstYear;
	};
	const Rival Rivals[] = {{fest::Brand::Feign, net::SiteId::Ignite, 2026}, {fest::Brand::Grit, net::SiteId::RunItTwice, 2027}};
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
	Expect(RivalBad.empty(), "FEIGN on Ignite every year 2026-2040 and GRIT on RunItTwice every year 2027-2040, one edition a year" + RivalBad);
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

/** §2.1.3's rows: RiverLine's four (FN-1), FEIGN and GRIT (FN-2a; the rest join at FN-3 and FN-2b). */
const FestNetFacts FestNetRiverLine[] = {
	{ss::fest::Brand::RingRush, ss::net::SiteId::RiverLine, 3, 6, 14, 2027, 14, 16 * 60, "Ring Rush", "ring", 40, false},
	{ss::fest::Brand::Tco, ss::net::SiteId::RiverLine, 6, 4, 19, 2027, 19, 18 * 60, "The Championship Online", "tco", 70, false},
	{ss::fest::Brand::Slam, ss::net::SiteId::RiverLine, 8, 20, 17, 2027, 17, 15 * 60, "Summer Slam", "slam", 80, true},
	{ss::fest::Brand::Rcop, ss::net::SiteId::RiverLine, 10, 18, 22, 2026, 22, 15 * 60, "RCOP", "rcop", 110, true},
	{ss::fest::Brand::Grit, ss::net::SiteId::RunItTwice, 5, 12, 12, 2027, 12, 18 * 60, "GRIT", "grit", 32, false},
	{ss::fest::Brand::Feign, ss::net::SiteId::Ignite, 11, 30, 10, 2026, 10, 18 * 60, "FEIGN", "feign", 32, false},
};

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
 *  false; Micro Madness 2026 left at Draw 0; Top Floor Series without HighRoller. */
void FestCalendarFactsNet()
{
	namespace fest = ss::fest;
	namespace net = ss::net;
	const std::vector<fest::Festival>& All = fest::All();

	// Each brand's site, window, Main day and start, first edition and edition numbers, ids, names and flags (a rival's ids are its
	// site's key, the brand and the year: "ignite.feign26"; its satellites the edition's Mega and Last Chance).
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
				: B.Site != net::SiteId::RiverLine ? std::vector<std::string>{Id + "-sat1", Id + "-sat2"}
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
	for (const char* Id : {"ring27", "tco27", "slam27", "rcop27", "ring40", "rcop40", "ignite.feign26", "runittwice.grit27", "ignite.feign40"})
	{
		const fest::Festival* F = fest::FestivalOf(Id);
		Ends = Ends && F && fest::LastEnd(*F) == FestNetLastEndByTemplates(net::Site(F->Site), Id) && fest::LastEnd(*F) >= F->MainStart;
	}
	Expect(Ends, "LastEnd agrees with each event's own scheduled finish (every run) on Ring Rush, The Championship Online, Summer Slam and RCOP 2027 and on 2040's, "
				 "FEIGN 2026 and 2040 and GRIT 2027");

	// PhaseOf with no world at every boundary, and PassFestival only in Announced, Buildup and Live.
	std::string Phases;
	for (const char* Id : {"rcop", "ring27", "tco27", "slam27", "rcop27", "slam40", "ignite.feign26", "runittwice.grit27", "ignite.feign40"})
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
		const char* Title = ""; // SeriesInfo::Title (§2.11 N17-N19): GRIT's "The Grinders' RunItTwice" (FN-2a); "" on every other
	};
	const DrawRow Draws[] = {
		{"rcop", 110, false, true, true}, {"slam", 80, false, true, true}, {"tco", 70, false, true, false}, {"ring", 40, false, true, false},
		{"feign", 32, false, true, false}, {"grit", 32, false, true, false, "The Grinders' RunItTwice"},
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
		"every series on every site (" + std::to_string(Seen) + ") carries the SPEC's 3.2.1 Draw (RCOP 110, Summer Slam 80, The Championship Online 70, Ring Rush 40, FEIGN 32, "
		"GRIT 32, Micro Madness 22, the seasonal 12, the Open 10, Wildfire 8, Ironworks 6, Top Floor 4 with HighRoller); Festival and Brand on the festivals, Flagship on RCOP "
		"and Summer Slam, GRIT's Title" + Wrong);

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
 *  every other edition's its Main alone (Summer Slam's though its Mini is a major); IsTitleTemplate agrees. FN-1: RiverLine's rows
 *  (PENNANT and REIGN join at FN-3). Planted: RCOP left at {MainId}. */
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
	Expect(Bad.empty() && Rcop26 && SlamOne && Agree && NotTitles && Majors && Championships > 0,
		"every championship edition (" + std::to_string(Championships) + ", RCOP's included: rcop-main, rcop-mini, rcop-micro in 2026) has TitleIds {main, mini, micro}, every "
		"other {MainId} (Summer Slam's, though its Mini is a major); IsTitleTemplate holds exactly the " + std::to_string(Titles) + " title ids; RiverLine's titles are majors" + Bad);
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

/** The brand's room words (SPEC v1.7 §2.3.2's lists, in their order: the order is part of the data). */
std::vector<std::string> FestNetRooms(ss::fest::Brand B)
{
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
	return {};
}

/** The plan facts the checks hold FEIGN and GRIT to (SPEC v1.7 §2.3.2): numbered events, the most a day, headliners, satellites. */
struct FestNetPlan
{
	ss::fest::Brand Which;
	int Numbered, PerDay, Headliners, Satellites;
};
const FestNetPlan FestNetPlans[] = {{ss::fest::Brand::Feign, 24, 3, 3, 2}, {ss::fest::Brand::Grit, 12, 1, 3, 2}};

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

// ------------------------------------------------------------------ FestTemplatesAppended (F-net; FN-2a's siblings)

/** §2.3.1, §2.3.4: the festivals append after every existing rival template (the prefixes hash to Phase A's constants,
 *  tests/sites_main.cpp:362-363), FEIGN's 15 editions x 29 = 435 on Ignite and GRIT's 14 x 17 = 238 on RunItTwice, each edition in
 *  the fixed order (numbered No 1..N, the headliners in calendar order, the satellites); ids unique across the three sites; every
 *  satellite seats its edition's Main ticket at the Main's buy-in. Planted: the festival loop placed before the series loop. */
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
	const SiteRow Sites[] = {{&Ig, net::SiteId::Ignite, 783, 435}, {&Rt, net::SiteId::RunItTwice, 1521, 238}};
	std::string Bad;
	for (const SiteRow& S : Sites)
	{
		size_t At = S.Prefix;
		for (const fest::Festival* F : FestNetRivalEditions())
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
					const net::EventTemplate* Main = S.N->FindTemplate(F->MainId);
					Order = Order && T.Fmt == net::Format::Satellite && T.Series.empty() && T.EventNo == 0 && Main && T.SeatTicket == S.N->TicketOf(F->MainId) &&
						T.SeatValueCents == Main->BuyInCents && T.GtdCents == 0;
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
	Expect(Bad.empty(),
		"Ignite gains FEIGN's 15 x 29 = 435 templates (1,218) and RunItTwice GRIT's 14 x 17 = 238 (1,759), after every existing template, each edition in order: "
		"numbered No 1..N, the headliners in calendar order numbered after them, the satellites seating its Main (no series, no number, no seat guarantee)" + Bad);

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
 *  allowed); no day holds more numbered events than the plan's PerDay. FN-2a: FEIGN and GRIT, every edition. Planted: v1.0's
 *  (No - 1) % PerDay slot. */
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
	Expect(Days > 0 && Bad.empty() && Samples,
		"FEIGN and GRIT, every edition (" + std::to_string(Days) + " festival days): numbered starts rise with No inside a day, no headliner shares a minute with another "
		"event, no two numbered runs share one, no satellite starts at a headliner's minute, no day over its PerDay; FEIGN 2026's No 8 at 8:00 PM, No 9 and 10 at 3:00 and 5:30 PM, "
		"No 16 at 6:00 PM on Dec 6 (The Big Top's 8:00 PM) and No 17 at 9:30 PM; GRIT 2027's No 7 on May 18 at 7:00 and 10:00 PM" + Bad);
}

// ------------------------------------------------------------------ FestRivalCapG1 (F-net)

/** §2.3.3: until G3 every rival festival template's largest draw stays under 3,000, every edition to 2040: a series event's
 *  Field x Scale x 1.15, a satellite's Field x Scale x the site's largest day factor x 1.25 (Make's draws). FN-2a: FEIGN and GRIT
 *  (The Big Top 2,760 and The Matinee 2,875 the largest). Planted: The Big Top at 2,700 entries. */
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
	Expect(Templates > 0 && Over == 0,
		"every rival festival template's largest draw is under 3,000 every edition (" + std::to_string(Templates) + " templates; largest " + std::to_string(Largest) + ", " + Which +
			")" + FirstOver);
}

// ------------------------------------------------------------------ FestNamesThemed (F-net)

/** §2.3.2 Names: every numbered event's name carries its brand's room word, event No taking the list's entry at
 *  (Fnv1a(SeriesId) + (No - 1) x 7) % RoomCount, skipping words already used; no word repeats inside an edition; FEIGN 2026's #7 Tilt
 *  Floor, #8 Air Jets, #9 Swing Boats, #10 Mirror Maze, #11 Carousel, #12 Rope Bridge, #16 Ticket Booth; GRIT 2027's #7 Dress Circle
 *  and #10 Second House; every word passes both trademark tiers and the design-word lint (PLAN_G1 item 19). Planted: the k-th word
 *  drawn with repeats. */
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
	const bool Samples = Has(Ig, "ignite.feign26-7", "Tilt Floor") && Has(Ig, "ignite.feign26-8", "Air Jets") && Has(Ig, "ignite.feign26-9", "Swing Boats") &&
		Has(Ig, "ignite.feign26-10", "Mirror Maze") && Has(Ig, "ignite.feign26-11", "Carousel") && Has(Ig, "ignite.feign26-12", "Rope Bridge") &&
		Has(Ig, "ignite.feign26-16", "Ticket Booth") && Has(Rt, "runittwice.grit27-7", "Dress Circle") && Has(Rt, "runittwice.grit27-10", "Second House");
	std::string Hits;
	for (fest::Brand B : {fest::Brand::Feign, fest::Brand::Grit})
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
		"every FEIGN and GRIT numbered name (" + std::to_string(Names) + ") carries its list's word at (Fnv1a(SeriesId) + (No - 1) x 7) % RoomCount, none twice in an edition; "
		"FEIGN 2026's #7 Tilt Floor ... #12 Rope Bridge and #16 Ticket Booth, GRIT 2027's #7 Dress Circle and #10 Second House; every word passes both trademark tiers and the "
		"design words" + Bad + Hits);
}

// ------------------------------------------------------------------ FestCodes (F-net)

/** §2.3.2, §2.11 H14, §6 PA-4: Ignite's lines "<Short> #<No>: <BuyIn> <Room> <Kind>" and its Main in caps; RunItTwice's with no "#"
 *  and two digits, its headliners "NLHE [<Headliner>]"; Network::NameOf gives each GRIT show's 19:00 instance "-E" and its 22:00
 *  instance "-L" and every other instance its template's name, and the listing and News read it; no tier letter (GRIT's runs aside),
 *  "[Day" or "RIT-"; SeriesInfo::Title GRIT's, "" for FEIGN. Planted: NameOf giving GRIT's 22:00 run the 19:00 run's "-E". */
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
	// Every Ignite instance over FEIGN 2026 reads its template's name.
	for (const net::EventInstance& E : Ig.Window(FestNetAt(56), FestNetAt(66)))
	{
		if (Ig.NameOf(E) != Ig.TemplateOf(E).Name && Bad.empty())
		{
			Bad = ": " + Ig.TemplateOf(E).Id + " reads \"" + Ig.NameOf(E) + "\"";
		}
	}
	Expect(Shows == 24 && Bad.empty() && Pie == "GRIT 02-E: $16.50 Box Office PKO",
		"Network::NameOf names GRIT 2027's " + std::to_string(Shows) + " shows by their run (\"GRIT 07-E\" at 7 PM, \"GRIT 07-L\" at 10 PM, one of each a night; the PIE line "
		"\"GRIT 02-E: $16.50 Box Office PKO\" on May 13), every other instance (FEIGN's, the headliners, the dailies) its template's name; the listing reads it (" + Pie + ")" + Bad);

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
	Expect(Form.empty() && GritSr && GritSr->Title == "The Grinders' RunItTwice" && FeignSr && FeignSr->Title.empty() && GritSr->Short == "GRIT" && FeignSr->Short == "FEIGN",
		"every FEIGN and GRIT event's code is its site's form (Ignite \"FEIGN #<No>: \", RunItTwice \"GRIT <NN>: \", no tier letter, \"[Day\" or RIT-); GRIT's Title is "
		"\"The Grinders' RunItTwice\", FEIGN has none" + Form);
}

// ------------------------------------------------------------------ FestNamesScreened (F-net)

/** §2.11, §2.15.1: the festival names, titles, taglines, trophies (TrophyName: P1-P12 exactly), satellites, headliners and every
 *  string fest::Inventory lists (FN-2a: FEIGN's and GRIT's), every room word and the ten brands' keys (the rival names, §2.1.3: the
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
	for (fest::Brand B : {fest::Brand::Feign, fest::Brand::Grit})
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

/** §1.2 items 2-5, 9-10, 12-13, §2.11: every fest::Inventory entry (FN-2a: FEIGN's and GRIT's names, titles, taglines, event names,
 *  blurbs and seats texts) by its kind (label: no closing stop, 200; sentence: a stop, 600; the F-world kinds as FW-1's lint), filled
 *  from the samples (copylint's, sites_main's SitesCopy values and fworld_to_fnet_1's) through CopyLint, both trademark tiers, the
 *  design words after the fill, U1, "players" (X1), DESIGN §2.6's avoided words, "marquee" and "pavilion", the glyph rule (ASCII and
 *  · — – … é ’) and FestChampWord; no rival festival name, Short, Title, tagline, event name, blurb, seats text or trophy holds
 *  "championship"; every {Festival} template filled again with "The Championship Online 2027" holds no "the The" and every {Date} a
 *  next year's date with its year; NameAfterArticle and DateInSentence answer §1.2 item 13's forms. Planted: "players" in a count;
 *  "GRIT Championship" as a Title; "Ignite's championship" in a tagline; NameAfterArticle and DateInSentence left at H0's stubs. */
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
