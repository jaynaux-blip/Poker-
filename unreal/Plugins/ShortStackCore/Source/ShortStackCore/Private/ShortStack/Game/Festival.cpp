// Update 4, Festival Season (SPEC v1.7 §2.1-§2.4, §2.9.1, §2.10): fest::, the ten festivals as pure functions of code and the clock.
// F-net's file from G1 (the calendar, phases, All/FestivalOf/LiveOn/LastEnd/SeatsWon/RecordOf/Marks, the Pass's festival).
//
// Phase A (§6 PA-6, the lead): stubs that keep today's behaviour. All() is empty, so no series is a festival and every query
// answers "none": FestivalOf, LiveOn, Next and PassFestival null, PhaseOf Off, Marks empty, RecordOf false, SeatsWon 0,
// IsTitleTemplate false. The brand table and IsChampionship are the spec's fixed facts (nothing reads them before G1). Step A-2 (the
// lead; LEAD_CALLS Q-5): YourLine is not shown and Inventory is empty. The three other inventories' stubs sit in their G1 owners'
// files, where their bodies land: TextInventory in G/SessionFest.cpp (F-world), FestUiInventory in UI/RiverLineFestival.cpp
// (F-hubs) and FestArtInventory in UI/FestivalArt.cpp (F-look).
//
// G1, FN-1 (F-net; SPEC v1.7 §2.1.1-§2.1.4, §2.2.1, §2.6.2, §2.10 item 1, §2.12; LEAD_CALLS R-13): the calendar. All() is built
// once, from every site's network (net::Site, RiverLine's being Shared()), out of the series flagged SeriesInfo::Festival (RiverLine's
// four from FN-1: G/Network.cpp's BuildSeries for RCOP 2026, G/NetworkSeries.cpp's dress table for every later year; the rival six
// from G/NetworkFestival.cpp). Its indexes are built with it (series id -> row, the sorted title ids, each site's rows), so
// FestivalOf, IsTitleTemplate, LiveOn, Next, Festivals and PassFestival are pure reads of a list that never changes once made.
// Prewarm builds it on the game thread (G/Network.cpp), before anything else asks. LastEnd is the one lazy value: the latest
// scheduled finish of the edition's instances (Make's durations, through Network::Window over the edition's days), worked out the
// first time a phase past LastDay asks and kept per series id under a lock. Nothing here draws a random number or writes a save.
//
// G1, FN-2a (F-net; SPEC v1.7 §1.2 item 13, §2.11; LEAD_CALLS R-3; PLAN_FNET's Corrections items 1 and 5): the rival festivals' rows
// come from G/NetworkFestival.cpp (FEIGN and GRIT first), with no change here. The bodies of H0's three helpers: TrophyName (§2.11's
// P1-P12 by brand and tier), NameAfterArticle (a leading "The " dropped after an article the template writes) and DateInSentence
// (the year printed outside Today's world year). Inventory lists the rival festivals' strings, each with its kind ("label\t" or
// "sentence\t", the G/VideoCopy.cpp precedent): every edition's name, Short, Title and tagline, and every event's printed name
// (GRIT's shows as their two runs, through Network::NameOf), blurb and seats text, once each. It grows as rows land (PENNANT and
// REIGN at FN-3, PUSH and WRIT at FN-2b; RiverLine's dress at FN-4a; the trophies, C1 and the room words at FN-4b).
//
// G1, FN-3 (F-net; SPEC v1.7 §2.1.1 rule 6, §2.1.2, §2.3.2, §2.6.2): PENNANT and REIGN, the rival championships. A championship's
// TitleIds are its Main, Mini and Micro Main Event; a rival's Mini and Micro join only as its plan flags them Title
// (G/NetworkFestival.cpp's NetSitesFestTitle), RCOP's as its templates exist (rcop-mini, rcop-micro). Festival::Satellites walks a
// championship's ladder: the satellites seating the Main's ticket, and those seating a ticket one of the edition's own one-off
// satellites takes (Step 1 seats a Step 2 ticket, Step 2 a Step 3 ticket), in template order, which is ladder order (Step 1's two
// templates, Step 2's, Step 3's, the Mega, the Last Chance). RCOP's recurring steps are not an edition's own: its list stays its
// step4 and mega-rcop.
//
// G1, FN-4a (F-net; SPEC v1.7 §2.2.2, §2.2.3, §2.11; PLAN_G1 item 26): RiverLine's dress. The trio G/NetworkSeries.cpp appends
// (a Main Satellite, a Mega and a Last Chance into each Ring Rush and Championship Online Main, 2027-2040) joins those editions'
// Satellites through the same ticket walk, with no change here; Inventory adds RiverLine's four festivals (names, Shorts, taglines,
// satellites), §2.2.2's other G1 strings and the five renamed seasonal series.
//
// G1, FN-4b (F-net; SPEC v1.7 §2.11; PLAN_FNET's Corrections item 1): Inventory is complete: the trophies and the championships' title
// objects (P1-P12, through TrophyName), the crown Honor's title (C1) and every room word of the six rival plans
// (G/NetworkFestival.cpp's NetSitesFestRoomWords), after the strings above.
//
// G1, FN-6 (F-net; SPEC v1.7 §2.8.3, §2.3.2; PLAN_FNET §4 FN-6): SeatsWon's body. The seats into the edition's Main won by satellite
// so far: max(1, Pool / SeatValue) an instance, the world's own seat rule, over the satellites in Festival::Satellites whose SeatTicket
// is the Main's ticket (a championship's Step 1 and 2 seat step tickets, which chain to the Main through the ladder, so they are left
// out; every other festival's satellites all seat its Main, so its figure is the plain sum), each one-off's runs on its day and a
// recurring satellite's (RCOP's step4 and mega-rcop) from Buildup to the Main's start, counted once finished. Worked out per world
// minute and kept per series under a lock (the hub and a desk worker may both ask), the LastEnd precedent.
//
// G1, FN-7 (F-net; SPEC v1.7 §2.9.1, §2.4.5; PLAN_FNET §4 FN-7): Marks and RecordOf. Marks lists every festival from its Announced
// phase (DayOf(Now) >= FirstDay - 30), on every site and whatever accounts the hero holds: MarkFestival at FirstDay 00:00 ("FEIGN
// 2026 opens") and MarkFestMain at the Main's start ("FEIGN 2026 Main Event", U22's labels with the edition's name), Where the host
// site's name, each inside [From, To). RecordOf measures E against every instance its site started from the launch in this world (W's
// SiteLaunched, else the site's LaunchDay) up to E's start: the launch day from its launch minute and E's own day up to E's start
// walked through the site's Window (E itself left out), every day between from Network::DayMaxOn (G/NetworkFestival.cpp's walk,
// filled in Prewarm and lazily after). Ever: E's field beats them all; Year: it beats every one since Jan 1 of E's year. RiverLine
// never claims a record (§2.4.5). Network::News posts the record line only when RecordOf says so.
#include "ShortStack/Game/Festival.h"

#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Game/WorldFest.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace net
{
namespace netsites_detail
{
// G/NetworkFestival.cpp's (F-net's; no header): a rival festival plan row's Title flag (§2.3.1), by brand key and row stem.
bool NetSitesFestTitle(const std::string& Brand, const std::string& Stem);
/** G1, FN-4b (G/NetworkFestival.cpp): the six rival plans' room words, plan by plan (FEIGN, PUSH, PENNANT; WRIT, GRIT, REIGN). */
std::vector<std::string> NetSitesFestRoomWords();
} // namespace netsites_detail
namespace calendar_detail
{
/** G1, FN-4a (G/NetworkSeries.cpp): the five RiverLine seasonal series renamed off "Festival" (§2.2.2, X2), as shipped. */
std::vector<std::string> FestRenamedNames();
/** G1, FN-4a (G/Network.cpp): the "RCOP 2026: the schedule" post's body, its figure read from RCOP 2026's GtdCents (§2.2.2). */
std::string FestRcopPost(const SeriesInfo& Rcop);
} // namespace calendar_detail
} // namespace net

namespace fest
{
namespace fest_detail
{
/** The brand keys in Brand's order (None first). */
const char* const FestBrandKeys[] = {"", "rcop", "ring", "tco", "slam", "feign", "push", "writ", "grit", "pennant", "reign"};
static_assert(sizeof(FestBrandKeys) / sizeof(FestBrandKeys[0]) == static_cast<size_t>(Brand::Count), "one key a brand");

/** All() and the indexes built with it (FestCalBuild): nothing in here changes after it is made. */
struct FestCalIndex
{
	std::vector<Festival> Rows;                        // every edition, FirstDay order (All())
	std::vector<std::pair<std::string, int>> BySeries; // series id -> row, sorted by id (FestivalOf: a binary search)
	std::vector<std::string> Titles;                   // every edition's TitleIds, sorted (IsTitleTemplate)
	std::vector<std::vector<int>> BySite;              // each site's rows (net::SiteId order), FirstDay order (LiveOn, PassFestival)
};

/** The world minute a day begins. */
double FestCalMinute(int Day)
{
	return static_cast<double>(Day) * net::MinutesPerDay;
}

/** One edition from its series on network N (Site): the Main, its day and start, the titles (a championship's Main, Mini and Micro
 *  Main, §2.6.2: "<stem>main", "<stem>mini", "<stem>micro", RCOP 2026's "rcop-main", "rcop-mini", "rcop-micro"; a rival's Mini and
 *  Micro as its plan flags them Title), and the satellites seating its Main's ticket, directly or up its ladder (Seats: the site's
 *  satellite templates by the ticket they seat; a recurring one always, a one-off only from the edition's Announced phase to its
 *  Main day). Edition is set by FestCalBuild, once every row is known. */
Festival FestCalRow(const net::Network& N, net::SiteId Site, const net::SeriesInfo& Sr, const std::map<std::string, std::vector<int>>& Seats)
{
	Festival F;
	F.Site = Site;
	F.Which = BrandOf(Sr.Brand);
	F.SeriesId = Sr.Id;
	F.Name = Sr.Name;
	F.Year = world::YearOf(Sr.FirstDay);
	F.FirstDay = Sr.FirstDay;
	F.LastDay = Sr.LastDay;
	F.MainId = Sr.MainEvent;
	F.MainDay = Sr.LastDay;
	F.MainStart = FestCalMinute(Sr.LastDay);
	if (const net::EventTemplate* Main = N.FindTemplate(F.MainId))
	{
		F.MainDay = Main->OnlyDay != -9999 ? Main->OnlyDay : Sr.LastDay;
		F.MainStart = FestCalMinute(F.MainDay) + static_cast<double>(Main->StartMinute);
	}
	F.TitleIds.push_back(F.MainId);
	const std::string MainStem = "main";
	if (IsChampionship(F.Which) && F.MainId.size() > MainStem.size() && F.MainId.compare(F.MainId.size() - MainStem.size(), MainStem.size(), MainStem) == 0)
	{
		const std::string Stem = F.MainId.substr(0, F.MainId.size() - MainStem.size());
		for (const char* Title : {"mini", "micro"})
		{
			// RiverLine's titles are its templates (RCOP's rcop-mini, rcop-micro); a rival's are the rows its plan flags Title (§2.3.1).
			if (N.FindTemplate(Stem + Title) && (Site == net::SiteId::RiverLine || net::netsites_detail::NetSitesFestTitle(Sr.Brand, Title)))
			{
				F.TitleIds.push_back(Stem + Title);
			}
		}
	}
	// The satellites seating the Main's ticket, then (a championship's ladder) those seating a ticket one of the edition's own
	// one-off satellites takes, until none joins; each once, in template order (ladder order).
	std::vector<int> Mine;
	std::vector<std::string> Open = {N.TicketOf(F.MainId)};
	while (!Open.empty())
	{
		const auto Seated = Seats.find(Open.back());
		Open.pop_back();
		for (size_t J = 0; Seated != Seats.end() && J < Seated->second.size(); ++J)
		{
			const int K = Seated->second[J];
			const net::EventTemplate& T = N.Templates()[static_cast<size_t>(K)];
			const bool InWindow = T.OnlyDay == -9999 || (T.OnlyDay >= F.FirstDay - kAnnounceDays && T.OnlyDay <= F.MainDay);
			if (InWindow && std::find(Mine.begin(), Mine.end(), K) == Mine.end())
			{
				Mine.push_back(K);
				if (T.OnlyDay != -9999)
				{
					Open.push_back(N.TicketOf(T.Id));
				}
			}
		}
	}
	std::sort(Mine.begin(), Mine.end());
	for (int K : Mine)
	{
		F.Satellites.push_back(N.Templates()[static_cast<size_t>(K)].Id);
	}
	F.Draw = Sr.Draw;
	F.Flagship = Sr.Flagship;
	return F;
}

/** Builds All() and its indexes from every site's network (the registry builds the rivals' on first use). */
FestCalIndex FestCalBuild()
{
	FestCalIndex X;
	const int SiteCount = static_cast<int>(net::SiteId::Count);
	X.BySite.resize(static_cast<size_t>(SiteCount));
	for (int S = 0; S < SiteCount; ++S)
	{
		const net::SiteId Site = static_cast<net::SiteId>(S);
		const net::Network& N = net::Site(Site);
		std::map<std::string, std::vector<int>> Seats;
		const std::vector<net::EventTemplate>& Temps = N.Templates();
		for (size_t I = 0; I < Temps.size(); ++I)
		{
			if (Temps[I].Fmt == net::Format::Satellite && !Temps[I].SeatTicket.empty())
			{
				Seats[Temps[I].SeatTicket].push_back(static_cast<int>(I));
			}
		}
		for (const net::SeriesInfo& Sr : N.Series())
		{
			if (Sr.Festival)
			{
				X.Rows.push_back(FestCalRow(N, Site, Sr, Seats));
			}
		}
	}
	std::stable_sort(X.Rows.begin(), X.Rows.end(), [](const Festival& A, const Festival& B) { return A.FirstDay < B.FirstDay; });
	// Edition = Year - the brand's first year + 1 (§2.1.2): the first year is the brand's earliest row (RCOP and FEIGN 2026, the
	// rest 2027; Summer Slam 2026 is history and stays a series).
	int FirstYear[static_cast<int>(Brand::Count)] = {};
	for (const Festival& F : X.Rows)
	{
		int& First = FirstYear[static_cast<int>(F.Which)];
		First = First == 0 ? F.Year : std::min(First, F.Year);
	}
	for (size_t I = 0; I < X.Rows.size(); ++I)
	{
		Festival& F = X.Rows[I];
		F.Edition = F.Year - FirstYear[static_cast<int>(F.Which)] + 1;
		X.BySeries.emplace_back(F.SeriesId, static_cast<int>(I));
		X.Titles.insert(X.Titles.end(), F.TitleIds.begin(), F.TitleIds.end());
		X.BySite[static_cast<size_t>(F.Site)].push_back(static_cast<int>(I));
	}
	std::sort(X.BySeries.begin(), X.BySeries.end());
	std::sort(X.Titles.begin(), X.Titles.end());
	return X;
}

/** Built once, on first use (a function-local static: thread-safe; Prewarm makes it on the game thread). */
const FestCalIndex& FestCal()
{
	static const FestCalIndex FestCalBuilt = FestCalBuild();
	return FestCalBuilt;
}

/** The site's rows (none for a site out of range). */
const std::vector<int>& FestCalSiteRows(net::SiteId Site)
{
	static const std::vector<int> FestCalNoRows;
	const FestCalIndex& X = FestCal();
	const size_t S = static_cast<size_t>(Site);
	return S < X.BySite.size() ? X.BySite[S] : FestCalNoRows;
}

/** The phase before LastDay ends (Off, Announced, Buildup, Live, MainDay; §2.1.4): a function of the calendar alone. */
Phase FestCalEarlyPhase(const Festival& F, double WorldMin)
{
	if (WorldMin < FestCalMinute(F.FirstDay - kAnnounceDays))
	{
		return Phase::Off;
	}
	if (WorldMin < FestCalMinute(F.FirstDay - kBuildupDays))
	{
		return Phase::Announced;
	}
	if (WorldMin < FestCalMinute(F.FirstDay))
	{
		return Phase::Buildup;
	}
	return WorldMin >= FestCalMinute(F.MainDay) && WorldMin < FestCalMinute(F.MainDay + 1) ? Phase::MainDay : Phase::Live;
}

/** LastEnd's cache: series id -> the latest scheduled finish, under FestCalEndLock (the precedent: SessionFuse's side lock). */
std::mutex& FestCalEndLock()
{
	static std::mutex* const FestCalEndMutex = new std::mutex();
	return *FestCalEndMutex;
}

std::map<std::string, double>& FestCalEnds()
{
	static std::map<std::string, double>* const FestCalEndMap = new std::map<std::string, double>();
	return *FestCalEndMap;
}

/** SeatsWon's cache (FN-6): series id -> (the world minute, the seats won by then), under FestSeatsLock. */
std::mutex& FestSeatsLock()
{
	static std::mutex* const FestSeatsMutex = new std::mutex();
	return *FestSeatsMutex;
}

std::map<std::string, std::pair<double, int>>& FestSeatsCache()
{
	static std::map<std::string, std::pair<double, int>>* const FestSeatsMap = new std::map<std::string, std::pair<double, int>>();
	return *FestSeatsMap;
}

/** The seats into F's Main won by satellites finished by the world minute At (§2.8.3): Σ max(1, Pool / SeatValue) over the instances
 *  of F.Satellites whose SeatTicket is the Main's ticket and whose seat has a value (G/WorldSim.cpp's P.Seats), a one-off's on its
 *  day, a recurring one's from Buildup to the Main's start; one Window over the days they run. */
int FestSeatsCount(const Festival& F, double At)
{
	const net::Network& N = net::Site(F.Site);
	const std::string MainTicket = N.TicketOf(F.MainId);
	const double Buildup = FestCalMinute(F.FirstDay - kBuildupDays);
	std::map<int, bool> Seating; // template index -> recurring
	double From = 0.0;
	double To = 0.0;
	for (const std::string& Id : F.Satellites)
	{
		const net::EventTemplate* T = N.FindTemplate(Id);
		if (!T || T->SeatTicket != MainTicket || T->SeatValueCents <= 0)
		{
			continue;
		}
		const bool Recurring = T->OnlyDay == -9999;
		const double First = Recurring ? Buildup : FestCalMinute(T->OnlyDay);
		const double Last = Recurring ? F.MainStart : FestCalMinute(T->OnlyDay + 1);
		From = Seating.empty() ? First : std::min(From, First);
		To = Seating.empty() ? Last : std::max(To, Last);
		Seating.emplace(static_cast<int>(T - N.Templates().data()), Recurring);
	}
	int Seats = 0;
	if (Seating.empty() || At <= From)
	{
		return Seats;
	}
	for (const net::EventInstance& E : N.Window(From, std::min(To, At)))
	{
		const auto S = Seating.find(E.Template);
		if (S == Seating.end() || E.Start + E.Duration > At || (S->second && (E.Start < Buildup || E.Start >= F.MainStart)))
		{
			continue;
		}
		const Chips Value = N.Templates()[static_cast<size_t>(E.Template)].SeatValueCents;
		Seats += std::max<int>(1, static_cast<int>(E.Pool / Value));
	}
	return Seats;
}
} // namespace fest_detail

Brand BrandOf(const std::string& Key)
{
	for (int B = 1; B < static_cast<int>(Brand::Count); ++B)
	{
		if (Key == fest_detail::FestBrandKeys[B])
		{
			return static_cast<Brand>(B);
		}
	}
	return Brand::None;
}

const char* BrandKey(Brand B)
{
	return B > Brand::None && B < Brand::Count ? fest_detail::FestBrandKeys[static_cast<int>(B)] : "";
}

bool IsChampionship(Brand B)
{
	return B == Brand::Rcop || B == Brand::Pennant || B == Brand::Reign;
}

bool IsTitleTemplate(const std::string& TemplateId)
{
	// §2.6.2: in one edition's TitleIds (every edition's, sorted once with All()).
	const std::vector<std::string>& Titles = fest_detail::FestCal().Titles;
	return std::binary_search(Titles.begin(), Titles.end(), TemplateId);
}

const std::vector<Festival>& All()
{
	return fest_detail::FestCal().Rows;
}

std::vector<const Festival*> Festivals(int FromDay, int ToDay)
{
	std::vector<const Festival*> Out;
	for (const Festival& F : All())
	{
		if (F.FirstDay <= ToDay && F.LastDay >= FromDay)
		{
			Out.push_back(&F);
		}
	}
	return Out;
}

const Festival* FestivalOf(const std::string& SeriesId)
{
	const fest_detail::FestCalIndex& X = fest_detail::FestCal();
	const auto It = std::lower_bound(X.BySeries.begin(), X.BySeries.end(), SeriesId,
		[](const std::pair<std::string, int>& Row, const std::string& Id) { return Row.first < Id; });
	return It != X.BySeries.end() && It->first == SeriesId ? &X.Rows[static_cast<size_t>(It->second)] : nullptr;
}

const Festival* LiveOn(net::SiteId Site, int Day)
{
	// The site's last edition opening on or before Day, if its window still holds Day (a site's windows never overlap, §2.1.1).
	const std::vector<Festival>& Rows = All();
	const std::vector<int>& Mine = fest_detail::FestCalSiteRows(Site);
	const auto It = std::upper_bound(Mine.begin(), Mine.end(), Day, [&Rows](int D, int Row) { return D < Rows[static_cast<size_t>(Row)].FirstDay; });
	if (It == Mine.begin())
	{
		return nullptr;
	}
	const Festival& F = Rows[static_cast<size_t>(*(It - 1))];
	return Day <= F.LastDay ? &F : nullptr;
}

const Festival* Next(int Day)
{
	const std::vector<Festival>& Rows = All();
	const auto It = std::upper_bound(Rows.begin(), Rows.end(), Day, [](int D, const Festival& F) { return D < F.FirstDay; });
	return It == Rows.end() ? nullptr : &*It;
}

double LastEnd(const Festival& F)
{
	// The latest Start + Duration over the edition's instances (Make's, through the host's Window over the edition's days: an
	// edition's events all start inside its window), worked out once per series id (§2.6.1). RCOP 2026's falls at 03:57 on day 35,
	// RCOP #153's ($22 Omaha, 2,109 runners from 20:00 on day 34), after its Main's 02:09. An edition with no instance (none
	// exists) answers its Main's start.
	std::lock_guard<std::mutex> Guard(fest_detail::FestCalEndLock());
	std::map<std::string, double>& Ends = fest_detail::FestCalEnds();
	const auto Found = Ends.find(F.SeriesId);
	if (Found != Ends.end())
	{
		return Found->second;
	}
	const net::Network& N = net::Site(F.Site);
	double End = F.MainStart;
	for (const net::EventInstance& E : N.Window(fest_detail::FestCalMinute(F.FirstDay), fest_detail::FestCalMinute(F.LastDay + 1)))
	{
		if (N.TemplateOf(E).Series == F.SeriesId)
		{
			End = std::max(End, E.Start + E.Duration);
		}
	}
	Ends.emplace(F.SeriesId, End);
	return End;
}

Phase PhaseOf(const Festival& F, double WorldMin, const world::World* W)
{
	// §2.1.4. Up to LastDay 24:00 the calendar alone decides; then Closing until the crown and the Afterglow for three days from it.
	if (WorldMin < fest_detail::FestCalMinute(F.LastDay + 1))
	{
		return fest_detail::FestCalEarlyPhase(F, WorldMin);
	}
	const double Day = net::MinutesPerDay;
	if (W)
	{
		const double Crown = world::FestCrownedAt(*W, F.SeriesId);
		if (Crown < 0.0)
		{
			// LEAD_CALLS R-13: an edition never crowned (no scorer, a race begun before the update, an old save) closes for the
			// crown's longest wait, two days past LastEnd, then reads Off: no champion, so no Afterglow.
			return WorldMin < LastEnd(F) + 2.0 * Day ? Phase::Closing : Phase::Off;
		}
		return WorldMin < Crown ? Phase::Closing : WorldMin < Crown + kAfterglowDays * Day ? Phase::Afterglow : Phase::Off;
	}
	// No world: the Afterglow runs kAfterglowDays from LastEnd.
	const double End = LastEnd(F);
	return WorldMin < End ? Phase::Closing : WorldMin < End + kAfterglowDays * Day ? Phase::Afterglow : Phase::Off;
}

FieldRecord RecordOf(const net::Network& N, const net::EventInstance& E, const world::World* W)
{
	// §2.4.5 (FN-7): a rival's festival news only; RiverLine has a past the network doesn't keep.
	FieldRecord Rec;
	if (N.Site() == net::SiteId::RiverLine)
	{
		return Rec;
	}
	const double Said = W ? W->SiteLaunched(N.Site()) : -1.0;
	const double Launch = Said >= 0.0 ? Said : fest_detail::FestCalMinute(N.Definition().LaunchDay);
	if (E.Start < Launch)
	{
		return Rec;
	}
	const int LaunchDay = net::DayOf(Launch);
	const int Day = net::DayOf(E.Start);
	const int JanFirst = world::DayOn(world::YearOf(Day), 1, 1);
	int Ever = 0, Year = 0;
	auto Count = [&](int On, int Entries) {
		Ever = std::max(Ever, Entries);
		Year = On >= JanFirst ? std::max(Year, Entries) : Year;
	};
	// The launch day from the lobby's opening minute (it can open mid-day), when it comes before E's day.
	if (LaunchDay < Day)
	{
		for (const net::EventInstance& X : N.Window(Launch, fest_detail::FestCalMinute(LaunchDay + 1) - 0.5))
		{
			Count(LaunchDay, X.Entries);
		}
	}
	for (int D = LaunchDay + 1; D < Day; ++D)
	{
		Count(D, N.DayMaxOn(D));
	}
	// E's own day, every instance that started up to E's start but E.
	for (const net::EventInstance& X : N.Window(std::max(Launch, fest_detail::FestCalMinute(Day)), E.Start + 0.5))
	{
		if (X.Id != E.Id)
		{
			Count(Day, X.Entries);
		}
	}
	Rec.Ever = E.Entries > Ever;
	Rec.Year = E.Entries > Year;
	return Rec;
}

int SeatsWon(const Festival& F, double Now)
{
	// Update 4 G1 (SPEC v1.7 §2.8.3; F-net, FN-6): the satellites finished by Now's world minute, worked out once a minute per series
	// (FestSeatsCount: one Window over the days the satellites run) and kept under the lock.
	const double Minute = std::floor(Now);
	std::lock_guard<std::mutex> Guard(fest_detail::FestSeatsLock());
	std::map<std::string, std::pair<double, int>>& Cache = fest_detail::FestSeatsCache();
	const auto Found = Cache.find(F.SeriesId);
	if (Found != Cache.end() && Found->second.first == Minute)
	{
		return Found->second.second;
	}
	const int Seats = fest_detail::FestSeatsCount(F, Minute);
	Cache[F.SeriesId] = std::make_pair(Minute, Seats);
	return Seats;
}

double RacePoints(int Place, int Entries, Chips BuyInCents, PayoutCurve Curve)
{
	// Update 4 G1 (SPEC v1.7 §2.4.3; F-net, FN-5): race points follow the curve. net::Points scores its own top 15%; the race
	// counts only the places the field paid (PaidPlaces, PayoutFor(...).size()), so under Main only cashes score ("every cash
	// scores" is true) and under Standard it is net::Points for every field but the four smallest (Paid is min(N - 1, ...)).
	return Place <= PaidPlaces(Entries, Curve) ? net::Points(Place, Entries, BuyInCents) : 0.0;
}

std::vector<life::Mark> Marks(double From, double To, double Now)
{
	// §2.9.1 (FN-7): every festival from its Announced phase, account or not; All() is in FirstDay order, so the first edition not
	// yet Announced ends the walk.
	std::vector<life::Mark> Out;
	const int Today = net::DayOf(Now);
	auto Add = [&Out, From, To](double At, int Kind, const std::string& Label, const std::string& Where) {
		if (At >= From && At < To)
		{
			life::Mark M;
			M.At = At;
			M.Kind = Kind;
			M.Label = Label;
			M.Where = Where;
			Out.push_back(M);
		}
	};
	for (const Festival& F : All())
	{
		if (Today < F.FirstDay - kAnnounceDays)
		{
			break;
		}
		const std::string Where = net::Def(F.Site).Name;
		Add(fest_detail::FestCalMinute(F.FirstDay), MarkFestival, F.Name + " opens", Where);
		Add(F.MainStart, MarkFestMain, F.Name + " Main Event", Where);
	}
	std::stable_sort(Out.begin(), Out.end(), [](const life::Mark& A, const life::Mark& B) { return A.At < B.At; });
	return Out;
}

const Festival* PassFestival(net::SiteId Site, double World)
{
	// §2.10 item 1: the site's edition in its Announced, Buildup or Live phase (MainDay is Live's), else none: closed outside
	// festival weeks, Closing and the Afterglow included.
	const std::vector<Festival>& Rows = All();
	for (int Row : fest_detail::FestCalSiteRows(Site))
	{
		const Festival& F = Rows[static_cast<size_t>(Row)];
		if (World < fest_detail::FestCalMinute(F.LastDay + 1))
		{
			return fest_detail::FestCalEarlyPhase(F, World) != Phase::Off ? &F : nullptr;
		}
	}
	return nullptr;
}

int PassNeeds(const Festival& F, const net::InviteGate& Gate)
{
	// §2.10 item 1: a brand's first edition invites on the two rents alone; from the second, the gate's FestivalEvents.
	return F.Edition == 1 ? 0 : Gate.FestivalEvents;
}

YourLineView YourLine(const Festival& F, const world::World* W)
{
	// G1 (F-hubs' body, sent as fhubs_to_fnet_1; LEAD_CALLS R-8; SPEC v1.7 §2.8.2): the hero's line in edition F, the YOUR panel's
	// numbers and the register sheets' two-line strip. Shown once the hero has played F: the pin the world derives for it
	// (world::HeroFestPins, one per edition played) gives the events, the cashes and the best finish with its field. "race {Rank} of
	// {Scorers}" reads the race while the race the world's views read is F's (world::FestRaceKey: the live race, or its final
	// standings until they clear), else the hero's kept mark for F (world::HeroMarks: the rank and scorers at the crown); Rank 0 leaves
	// the clause off (not scoring yet, or no mark). Presentation only: never saved, no random number, no HoldGuarded call.
	YourLineView V;
	if (!W)
	{
		return V;
	}
	for (const world::HeroFestPin& Pin : world::HeroFestPins(*W))
	{
		if (Pin.Key == F.SeriesId)
		{
			V.Shown = true;
			V.Name = F.Name;
			V.Events = Pin.Events;
			V.Cashes = Pin.Cashes;
			V.Best = Pin.BestPlace;
			V.Field = Pin.BestEntries;
			break;
		}
	}
	if (!V.Shown)
	{
		return V;
	}
	if (world::FestRaceKey(*W) == F.SeriesId)
	{
		const std::pair<int, int> Standing = world::FestStanding(*W, -1);
		V.Rank = Standing.first;
		V.Scorers = Standing.second;
		return V;
	}
	for (const world::HeroFestMark& M : world::HeroMarks(*W))
	{
		if (M.Key == F.SeriesId)
		{
			V.Rank = M.Rank;
			V.Scorers = M.Scorers;
			break;
		}
	}
	return V;
}

std::vector<std::string> Inventory()
{
	// §2.11's core strings, kind first: every rival festival edition's name, Short, Title and tagline, and every event's printed name,
	// blurb and seats text (the satellites through Festival::Satellites), each string once, in All()'s order. FestCopy and
	// FestNamesScreened (festivals_test) run the lint and both trademark tiers over them. FN-4a (§2.2.2, §2.2.3; PLAN_G1 item 26):
	// RiverLine's four festivals' names, Shorts and taglines (RCOP 2026's with its computed guarantee) and their satellites (RCOP's step4
	// and Mega, the trio into Ring Rush's and the Championship Online's Mains, H11-H13), then §2.2.2's other G1 strings (the rcop-main,
	// Mega and Sunday Showdown blurbs, the Mega's seats text, the schedule post's body) and the five renamed seasonal series.
	// RiverLine's numbered events keep today's strings (U9), so they are not listed. FN-4b (§2.11): the trophies and the two title
	// objects (P1-P12, as TrophyName prints them), the crown Honor's title (C1, a template: WorldFest's crown fills it with the
	// edition's name) and the six rival plans' room words, every word a numbered name can draw.
	std::vector<std::string> Out;
	std::set<std::string> Seen;
	const auto Add = [&Out, &Seen](const char* Kind, const std::string& Text) {
		const std::string Entry = std::string(Kind) + "\t" + Text;
		if (!Text.empty() && Seen.insert(Entry).second)
		{
			Out.push_back(Entry);
		}
	};
	for (const Festival& F : All())
	{
		const net::Network& N = net::Site(F.Site);
		const net::SeriesInfo* Sr = N.FindSeries(F.SeriesId);
		if (!Sr)
		{
			continue;
		}
		Add("label", Sr->Name);
		Add("label", Sr->Short);
		Add("label", Sr->Title);
		Add("sentence", Sr->Tagline);
		if (F.Site == net::SiteId::RiverLine)
		{
			for (const std::string& Id : F.Satellites)
			{
				if (const net::EventTemplate* T = N.FindTemplate(Id))
				{
					Add("label", T->Name);
					Add("sentence", T->Blurb);
					Add("label", T->Seats);
				}
			}
			continue;
		}
		const std::vector<net::EventTemplate>& Temps = N.Templates();
		for (size_t I = 0; I < Temps.size(); ++I)
		{
			const net::EventTemplate& T = Temps[I];
			const bool Satellite = std::find(F.Satellites.begin(), F.Satellites.end(), T.Id) != F.Satellites.end();
			if (T.Series != F.SeriesId && !Satellite)
			{
				continue;
			}
			// The printed name of each run (GRIT's two runs a night: "GRIT 07-E", "GRIT 07-L"; one run for every other event).
			net::EventInstance E;
			E.Template = static_cast<int>(I);
			for (int M = T.StartMinute; M <= (T.EveryMinutes > 0 ? T.LastMinute : T.StartMinute); M += std::max(1, T.EveryMinutes))
			{
				E.Start = fest_detail::FestCalMinute(T.OnlyDay) + static_cast<double>(M);
				Add("label", N.NameOf(E));
			}
			Add("sentence", T.Blurb);
			Add("label", T.Seats);
		}
	}
	const net::Network& Home = net::Site(net::SiteId::RiverLine);
	for (const char* Id : {"rcop-main", "mega-rcop", "sunday-showdown"})
	{
		if (const net::EventTemplate* T = Home.FindTemplate(Id))
		{
			Add("sentence", T->Blurb);
			Add("label", T->Seats);
		}
	}
	if (const net::SeriesInfo* Rcop = Home.FindSeries("rcop"))
	{
		Add("sentence", net::calendar_detail::FestRcopPost(*Rcop));
	}
	for (const std::string& Name : net::calendar_detail::FestRenamedNames())
	{
		Add("label", Name);
	}
	for (int B = static_cast<int>(Brand::None) + 1; B < static_cast<int>(Brand::Count); ++B)
	{
		Add("label", TrophyName(static_cast<Brand>(B)));
		Add("label", TrophyName(static_cast<Brand>(B), 1));
	}
	Add("label", "{Festival} {Year} Player of the Series");
	for (const std::string& Word : net::netsites_detail::NetSitesFestRoomWords())
	{
		Add("label", Word);
	}
	return Out;
}

// G1's appends (the lead's pre-wave commit H0, PLAN_G1 §4.2): stubs until F-net's bodies (FN-2a). Nothing calls them before G1's
// packages; each answers empty, or as today's text prints.

const char* TrophyName(Brand B, int Tier)
{
	// §2.11 P1-P12, as printed (FN-2a; LEAD_CALLS R-3): the trophies by brand (P1-P10), and the two championships' title objects at
	// Tier 1 (P11-P12; RCOP, a championship too, has none). "" for None, Count or a tier the brand lacks.
	static const char* const FestTrophies[] = {"", "The RCOP Cup", "the ring", "the bracelet", "The Slam Plate", "The Funhouse Mirror", "The Big Red Button",
		"The Seal", "The Grindstone", "The Gold Pennant", "The Sceptre"};
	static_assert(sizeof(FestTrophies) / sizeof(FestTrophies[0]) == static_cast<size_t>(Brand::Count), "one trophy a brand");
	if (B <= Brand::None || B >= Brand::Count)
	{
		return "";
	}
	if (Tier == 0)
	{
		return FestTrophies[static_cast<int>(B)];
	}
	if (Tier == 1)
	{
		return B == Brand::Pennant ? "The Pennant Ribbon" : B == Brand::Reign ? "The Spade Finial" : "";
	}
	return "";
}

std::string NameAfterArticle(const std::string& Name)
{
	// §1.2 item 13, §2.11 (FN-2a): after an article the template writes ("the {Festival} Main Event", "next {Festival}"), a name that
	// starts with "The " drops it, so no fill prints "the The": "The Championship Online 2027" -> "Championship Online 2027". Every
	// other name ("FEIGN 2026", "RCOP 2026", "Ring Rush 2027") prints as it is.
	const std::string Article = "The ";
	return Name.compare(0, Article.size(), Article) == 0 && Name.size() > Article.size() ? Name.substr(Article.size()) : Name;
}

std::string DateInSentence(int Day, int Today)
{
	// §1.2 item 13 (FN-2a): "Oct 6" when Day falls in Today's world year, "Oct 6, 2026" in any other (both world days).
	const int Year = world::YearOf(Day);
	return Year == world::YearOf(Today) ? net::DateLabel(Day) : net::DateLabel(Day) + ", " + std::to_string(Year);
}
} // namespace fest
} // namespace ss
