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
#include "ShortStack/Game/Festival.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/World.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
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
		if (T.Series == SeriesId && T.OnlyDay != -9999 && N.Next(T.Id, FestNetAt(T.OnlyDay), E))
		{
			++Count;
			if (E.Start + E.Duration > End && Last)
			{
				*Last = T.Name + " (" + std::to_string(E.Entries) + " runners from " + ss::net::TimeLabel(E.Start) + " on day " + std::to_string(T.OnlyDay) + ")";
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

/** §2.1.3's rows for RiverLine's four (FN-1). The rival rows join from FN-2a. */
const FestNetFacts FestNetRiverLine[] = {
	{ss::fest::Brand::RingRush, ss::net::SiteId::RiverLine, 3, 6, 14, 2027, 14, 16 * 60, "Ring Rush", "ring", 40, false},
	{ss::fest::Brand::Tco, ss::net::SiteId::RiverLine, 6, 4, 19, 2027, 19, 18 * 60, "The Championship Online", "tco", 70, false},
	{ss::fest::Brand::Slam, ss::net::SiteId::RiverLine, 8, 20, 17, 2027, 17, 15 * 60, "Summer Slam", "slam", 80, true},
	{ss::fest::Brand::Rcop, ss::net::SiteId::RiverLine, 10, 18, 22, 2026, 22, 15 * 60, "RCOP", "rcop", 110, true},
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

	// Each RiverLine brand's site, window, Main day and start, first edition and edition numbers, ids, names and flags.
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
			const std::string Id = B.Which == fest::Brand::Rcop && Year == 2026 ? std::string("rcop") : std::string(B.Stem) + (Yy.size() < 2 ? "0" : "") + Yy;
			const int First = ss::world::DayOn(Year, B.Month, B.Day);
			const int MainDay = First + B.MainNo - 1;
			const std::vector<std::string> Seats = B.Which == fest::Brand::Rcop ? std::vector<std::string>{"step4", "mega-rcop"} : std::vector<std::string>{};
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
			std::string(B.Name) + ": " + std::to_string(Rows.size()) + " editions " + std::to_string(B.FirstYear) + "-2040 on RiverLine, " + std::to_string(B.Days) + " days from " +
				net::DateLabel(ss::world::DayOn(B.FirstYear, B.Month, B.Day)) + ", the Main on day " + std::to_string(B.MainNo) + " at " + net::TimeLabel(B.MainMinute) +
				", edition I in " + std::to_string(B.FirstYear) + ", Draw " + std::to_string(B.Draw) + (B.Flagship ? ", flagship" : "") + Bad);
	}
	const fest::Festival* Rcop = fest::FestivalOf("rcop");
	Expect(Rcop && Rcop->Name == "RCOP 2026" && Rcop->Edition == 1 && Rcop->FirstDay == 13 && Rcop->LastDay == 34 && Rcop->MainDay == 34 &&
			   Rcop->MainStart == FestNetAt(34, 15 * 60) && Rcop->MainId == "rcop-main" && Rcop->Draw == 110 && Rcop->Flagship,
		"RCOP 2026: days 13-34 (Oct 18 - Nov 8), its Main rcop-main on day 34 at 3:00 PM, edition I, Draw 110, flagship");

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
	for (const char* Id : {"ring27", "tco27", "slam27", "rcop27", "ring40", "rcop40"})
	{
		const fest::Festival* F = fest::FestivalOf(Id);
		Ends = Ends && F && fest::LastEnd(*F) == FestNetLastEndByTemplates(River, Id) && fest::LastEnd(*F) >= F->MainStart;
	}
	Expect(Ends, "LastEnd agrees with each event's own scheduled finish on Ring Rush, The Championship Online, Summer Slam and RCOP 2027 and on 2040's");

	// PhaseOf with no world at every boundary, and PassFestival only in Announced, Buildup and Live.
	std::string Phases;
	for (const char* Id : {"rcop", "ring27", "tco27", "slam27", "rcop27", "slam40"})
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
	};
	const DrawRow Draws[] = {
		{"rcop", 110, false, true, true}, {"slam", 80, false, true, true}, {"tco", 70, false, true, false}, {"ring", 40, false, true, false},
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
				Sr.Flagship == (Row->Flagship && !History) && Sr.Brand == (Sr.Festival ? Stem : std::string()) && Sr.Title.empty();
			if (!Ok && Wrong.empty())
			{
				Wrong = ": first wrong " + Sr.Id + " (Draw " + std::to_string(Sr.Draw) + (Sr.HighRoller ? ", high roller" : "") + (Sr.Festival ? ", festival " + Sr.Brand : std::string()) +
					(Sr.Flagship ? ", flagship" : "") + ")";
			}
		}
	}
	Expect(Seen > 0 && Wrong.empty(),
		"every series on every site (" + std::to_string(Seen) + ") carries the SPEC's 3.2.1 Draw (RCOP 110, Summer Slam 80, The Championship Online 70, Ring Rush 40, Micro Madness 22, "
		"the seasonal 12, the Open 10, Wildfire 8, Ironworks 6, Top Floor 4 with HighRoller); Festival and Brand on the four festivals, Flagship on RCOP and Summer Slam" + Wrong);
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
