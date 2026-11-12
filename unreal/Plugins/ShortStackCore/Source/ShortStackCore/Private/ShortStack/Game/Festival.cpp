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
#include "ShortStack/Game/Festival.h"

#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Game/WorldFest.h"

#include <algorithm>
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
} // namespace netsites_detail
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
	// §2.11's core strings, kind first: every rival festival edition's name, Short, Title and tagline, and every event's printed name,
	// blurb and seats text (the satellites through Festival::Satellites), each string once, in All()'s order. FestCopy and
	// FestNamesScreened (festivals_test) run the lint and both trademark tiers over them.
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
		if (F.Site == net::SiteId::RiverLine)
		{
			continue; // RiverLine's four join at FN-4a (§2.2.2's strings, the renames, the trio)
		}
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
