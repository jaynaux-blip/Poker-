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
#include "ShortStack/Game/Festival.h"

#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Game/WorldFest.h"

#include <algorithm>
#include <map>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
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
 *  Main, §2.6.2: "<stem>main", "<stem>mini", "<stem>micro", RCOP 2026's "rcop-main", "rcop-mini", "rcop-micro"), and the
 *  satellites seating its Main's ticket (Seats: the site's satellite templates by the ticket they seat; a recurring one always, a
 *  one-off only from the edition's Announced phase to its Main day). Edition is set by FestCalBuild, once every row is known. */
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
			if (N.FindTemplate(Stem + Title))
			{
				F.TitleIds.push_back(Stem + Title);
			}
		}
	}
	const auto Seated = Seats.find(N.TicketOf(F.MainId));
	if (Seated != Seats.end())
	{
		for (int K : Seated->second)
		{
			const net::EventTemplate& T = N.Templates()[static_cast<size_t>(K)];
			if (T.OnlyDay == -9999 || (T.OnlyDay >= F.FirstDay - kAnnounceDays && T.OnlyDay <= F.MainDay))
			{
				F.Satellites.push_back(T.Id);
			}
		}
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

FieldRecord RecordOf(const net::Network& /*N*/, const net::EventInstance& /*E*/, const world::World* /*W*/)
{
	return FieldRecord();
}

int SeatsWon(const Festival& /*F*/, double /*Now*/)
{
	return 0;
}

double RacePoints(int Place, int Entries, Chips BuyInCents, PayoutCurve /*Curve*/)
{
	// Phase A: net::Points (every Standard event's points but the four smallest fields'); G1 cuts it at PaidPlaces(Entries, Curve).
	return net::Points(Place, Entries, BuyInCents);
}

std::vector<life::Mark> Marks(double /*From*/, double /*To*/, double /*Now*/)
{
	return {};
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

YourLineView YourLine(const Festival& /*F*/, const world::World* /*W*/)
{
	return YourLineView();
}

std::vector<std::string> Inventory()
{
	return {};
}

// G1's appends (the lead's pre-wave commit H0, PLAN_G1 §4.2): stubs until F-net's bodies (FN-2a). Nothing calls them before G1's
// packages; each answers empty, or as today's text prints.

const char* TrophyName(Brand /*B*/, int /*Tier*/)
{
	// H0: no trophy is named yet (FN-2a: §2.11's P1-P12 by brand and tier).
	return "";
}

std::string NameAfterArticle(const std::string& Name)
{
	// H0: the name as it is (FN-2a drops a leading "The ").
	return Name;
}

std::string DateInSentence(int Day, int /*Today*/)
{
	// H0: today's date label, "Oct 6" (FN-2a adds the year outside Today's world year).
	return net::DateLabel(Day);
}
} // namespace fest
} // namespace ss
