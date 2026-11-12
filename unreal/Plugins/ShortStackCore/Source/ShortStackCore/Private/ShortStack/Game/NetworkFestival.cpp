// Update 4, Festival Season (SPEC v1.7 §2.3, §2.4.5): the rival festivals' templates and the record field's day maxima, Network's
// side of fest::. F-net's file from G1: the plans (const NetSitesFestival IgniteFestivals[], RunItTwiceFestivals[]; FEIGN, PUSH and
// PENNANT on Ignite, WRIT, GRIT and REIGN on RunItTwice, each appended last so every sibling template keeps its index), their
// numbered events, headliners and satellites, Added and the Main curve; DayMax's non-caching walk for fest::RecordOf.
//
// Phase A (§6 PA-4, the lead): both members are declared and called, and do nothing. BuildFestivals runs last in BuildSite and
// appends no template or series (every rival keeps its 783 and 1,521 templates); FillDayMax runs first in Prewarm and leaves DayMax
// empty (nothing reads it before G1). H0 (the lead's G1 pre-wave commit) adds DayMaxOn, fest::RecordOf's public read of DayMax,
// answering 0 until F-net's body (FN-7).
//
// G1, FN-2a (F-net; SPEC v1.7 §2.1.3, §2.3.1-§2.3.4, §2.4.1-§2.4.3 as data, §2.11; PLAN_FNET's Corrections item 1): FEIGN on Ignite
// and GRIT on RunItTwice, the first plan of each site (PUSH, WRIT, PENNANT and REIGN join at FN-3 and FN-2b). BuildFestivals walks
// each plan's editions, first year to 2040, and appends every edition's templates in one fixed order (template indices are a save
// fact from ship, G/WorldSave.cpp saves a Pending's template index): the numbered events No 1..N, the headliners in calendar order
// (numbered after the numbered events), then the satellites (no number, no series, seating the edition's Main). The numbered
// generator is §2.3.2's, a pure function of the plan and the series id (no RNG, §2.12): the day by integer division of the window,
// the start by the event's slot among its own day's numbered events, the buy-in from the series curve, the format from the plan's
// rotation, the room from the brand's own list keyed on Fnv1a of the series id, the name in each site's own code form (Ignite's
// "FEIGN #9: $16.50 Swing Boats Mystery", RunItTwice's "GRIT 07: $33 Dress Circle Turbo", whose two runs a night
// Network::NameOf prints as "GRIT 07-E" and "GRIT 07-L"). Guarantees: 0.9x the median pool on numbered events (the rivals'
// GtdOfPool), the plan's share on headliners (1.0 Ignite, 0.95 RunItTwice), none on satellites; Added money and the Main curve's
// flag on the Mains only (the curve's body is FN-5's, PayoutFor answers Standard's table until then). The names, taglines,
// blurbs and seats texts are §2.11's (N3, N8, N11, N16, N19, G3, G8, H1, H3, H4, H7-H10, H14, B1, B3-B5, B7-B10).
#include "ShortStack/Game/Network.h"

#include "SitesDetail.h"
#include "ShortStack/Game/Festival.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/World.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace net
{
namespace netsites_detail
{
// G/NetworkSites.cpp's helpers (both files F-net's; external linkage, declared here as SPEC §2.3.1 reuses them).
Chips NetSitesCents(double Dollars);
int NetSitesAt(int H, int M);
double NetSitesStep(double Dollars, double Lo, double Hi);
void NetSitesShape(EventTemplate& T, const char* Speed);
Chips NetSitesGtd(const SiteDef& D, const EventTemplate& T, double Scale);

/** The numbered events' format rotation values (a plan's Formats, indexed by No % 4). */
enum NetSitesFestFmt : int
{
	NetSitesFestFreezeout,
	NetSitesFestReEntry,
	NetSitesFestBounty,
	NetSitesFestMystery,
	NetSitesFestReEntryTurbo, // a ReEntry in the rotation's turbo slot (Kind "Turbo")
};

/** A headliner or a satellite, by its place in the window (SPEC §2.3.1). */
struct NetSitesFestEvent
{
	const char* Stem;  // "main", "hr", "mega", "on", "sat1", "sat2"; v1.6: "micro", "mini", "s1", "s2", "s3"
	const char* Name;  // "Main Event", "High Roller", "The Big Top", "Opening Night", "Main Mega Satellite", ...
	double BuyIn;
	Format Fmt;
	const char* Speed; // NetSitesShape's
	int Entries;       // the site's entries at the median draw (T.Field = Entries / Traffic.Scale)
	int DayIndex;      // 0-based day of the window
	int Start;         // minutes after midnight
	double GtdShare;   // of the median pool (1.0 Ignite / 0.95 RunItTwice headliners; 0 = none; -1 = seats, below)
	int Seats;         // satellites: seats guaranteed (GtdCents = Seats x the Main's buy-in); 0 = none
	Chips AddedCents;  // added money (the Main only)
	bool Main, Satellite, Curve; // Curve: the Main curve (the Main only, §2.4.3)
	const char* Blurb; // {BuyIn}, {Start} and {MainBuyIn} filled from the template (no blurb spells a number, §2.11)
	// v1.6, appended (CHAMP_DESIGN §3.2-§3.3; the defaults keep the siblings' rows as they are; F-net's own struct, not a seam):
	bool Title = false;           // a title row: joins Festival::TitleIds and prints in the site's title form (§2.3.2)
	int DayIndex2 = -1;           // a second day: a second template, ids "<stem>a" / "<stem>b"
	int Every = 0, LastStart = 0; // repeats inside a day (one-offs honour them, G/Network.cpp:1038-1062, as GRIT's shows do)
	const char* SeatInto = "";    // the stem whose ticket a seat is ("" = the Main ticket)
	const char* TicketKey = "";   // the ticket this row takes: "" = its own id; a step's b row takes its a row's id; the Main
	                              // takes "<site>.<brand>-main"
};

/** A numbered event the plan sets apart (FEIGN's two $11 headline events at 1.0x, No 17 after No 16; F-net's, §2.3.2). A zero or
 *  negative field keeps the generator's value. */
struct NetSitesFestNumber
{
	int No;
	double BuyIn;    // 0: the curve's
	int Fmt;         // NetSitesFestFmt; -1: the rotation's
	int Entries;     // 0: the plan's
	int Start;       // -1: the slot's
	double GtdScale; // NetSitesGtd's scale; 0: 1.0 (0.9x the median pool)
};

/** One festival's plan (SPEC §2.3.1; F-net's columns appended after SlotStart/SlotStep). */
struct NetSitesFestival
{
	const char* Brand;  // fest::BrandKey
	const char* Name;   // "FEIGN" (the name is the tag)
	const char* Short;  // "FEIGN" (pills, the Lighthouse sign, the Board's tag, event codes)
	const char* Title;  // SeriesInfo::Title: "The Grinders' RunItTwice" (GRIT); "" for FEIGN, PENNANT and REIGN
	int Month, Day, Span; // the window
	int FirstYear;
	int Numbered;               // numbered events
	double MinBuyIn, MaxBuyIn;  // their ladder (standard buy-ins only)
	int Entries;                // a numbered event's site entries at the median draw
	int PerDay;                 // the most numbered events a day (FEIGN 3, GRIT 1; festivals_test asserts it)
	const char* const* Rooms;
	int RoomCount;              // the brand's own words for numbered event names (§2.3.2 Names; >= Numbered)
	const int* Formats;         // the numbered events' format rotation (NetSitesFestFmt values, by No % 4), length 4
	const NetSitesFestEvent* Events;
	int EventCount;
	uint32_t Color, Color2;     // SeriesInfo colours (the festival's Accent and Accent2, §2.7.2)
	int Draw;
	bool Flagship;
	const char* Tagline;
	int SlotStart, SlotStep;    // v1.6, appended: the numbered start rule in minutes (FEIGN 900/150, GRIT 1140/0)
	// F-net's, appended (FN-2a):
	int ShowEvery = 0, ShowLast = 0;         // the numbered events' repeats inside their night (GRIT: every 180 to 22:00, two runs)
	bool HighRoller = false;                 // SeriesInfo::HighRoller (WRIT, FN-2b)
	bool FlipOpener = false;                 // No 1 is Ignite's Flip & Run (FEIGN)
	const NetSitesFestNumber* Numbers = nullptr; // the numbered events set apart
	int NumberCount = 0;
};

// FEIGN (SPEC §2.1.3, §2.3.2): Ignite, Nov 30, ten days, from 2026. The funhouse words in the order that gives FEIGN 2026 v1.3's rooms.
const char* const NetSitesFeignRooms[] = {"Wonky Stairs", "Air Jets", "Big Wheel", "Toffee Apple", "Spinning Disc", "Rope Bridge", "Lantern Walk", "Hook-a-Duck",
	"Swing Boats", "Ticket Booth", "Stilt Walk", "Barrel Organ", "Trapdoor", "Wobble Room", "Candy Floss", "Mirror Maze", "Rolling Barrel", "Crooked House",
	"Tilt Floor", "Ghost Train", "Coconut Shy", "Ring Toss", "Carousel", "Laughing Clown"};
const int NetSitesFeignFormats[4] = {NetSitesFestFreezeout, NetSitesFestMystery, NetSitesFestBounty, NetSitesFestReEntryTurbo};
const NetSitesFestNumber NetSitesFeignNumbers[] = {
	{8, 11.0, NetSitesFestMystery, 420, 20 * 60, 1.0 / 0.9}, // the first $11 headline event, at 1.0x
	{16, 11.0, NetSitesFestBounty, 420, 18 * 60, 1.0 / 0.9}, // the second, at 18:00: The Big Top owns Dec 6's 20:00
	{17, 0.0, -1, 0, 21 * 60 + 30, 0.0},                     // that day's other numbered event follows No 16
};
const NetSitesFestEvent NetSitesFeignEvents[] = {
	{"mega", "The Big Top", 2.20, Format::ReEntry, "Regular", 2400, 6, 20 * 60, 1.0, 0, 0, false, false, false, "The biggest room in the funhouse."},
	{"hr", "High Roller", 109.0, Format::Freezeout, "Regular", 120, 8, 19 * 60, 1.0, 0, 0, false, false, false, "The festival's highest buy-in."},
	{"main", "Main Event", 55.0, Format::Freezeout, "Deep", 700, 9, 18 * 60, 1.0, 0, 250000, true, false, true, "The Main Event: a guarantee, money added and a top-heavy payout."},
	{"sat1", "Main Mega Satellite", 5.50, Format::Satellite, "Turbo", 200, 8, 20 * 60 + 30, 0.0, 0, 0, false, true, false, "Seats into the Main Event, the night before."},
	{"sat2", "Main Last Chance", 2.20, Format::Satellite, "Hyper", 150, 9, 15 * 60, 0.0, 0, 0, false, true, false, "The last way into the Main Event."},
};

// GRIT (SPEC §2.1.3, §2.3.2): RunItTwice, May 12, twelve nights, from 2027. Twelve show templates, one a night, each run twice.
const char* const NetSitesGritRooms[] = {"Encore", "Second House", "Footlights", "Stage Door", "Dress Circle", "Box Office", "Opening Act", "Overture", "Interval",
	"Front Row", "Spotlight", "Understudy"};
const int NetSitesGritFormats[4] = {NetSitesFestFreezeout, NetSitesFestReEntry, NetSitesFestBounty, NetSitesFestReEntryTurbo};
const NetSitesFestEvent NetSitesGritEvents[] = {
	{"on", "Opening Night", 109.0, Format::ReEntry, "Regular", 600, 0, 20 * 60 + 30, 0.95, 0, 0, false, false, false, "The first curtain. {BuyIn} and a long night ahead."},
	{"mega", "The Matinee", 3.30, Format::ReEntry, "Regular", 2500, 4, 15 * 60, 0.95, 0, 0, false, false, false, "The afternoon show: {Start}, {BuyIn} a seat."},
	{"main", "Main Event", 530.0, Format::Freezeout, "Deep", 1000, 11, 18 * 60, 0.95, 0, 1000000, true, false, true,
		"The last show of the run. Deep stacks, a top-heavy payout and money added."},
	{"sat1", "Main Mega Satellite", 55.0, Format::Satellite, "Turbo", 150, 10, 17 * 60, 0.0, 0, 0, false, true, false, "Seats into the Main Event, the night before."},
	{"sat2", "Main Last Chance", 22.0, Format::Satellite, "Hyper", 120, 11, 15 * 60, 0.0, 0, 0, false, true, false, "The last way into the Main Event."},
};

// The plans, each site's appended in this order (v1.6: PUSH after FEIGN and PENNANT last on Ignite; WRIT before GRIT and REIGN last
// on RunItTwice, FN-2b and FN-3), so every earlier plan's templates keep their indices. Flagship lives in each plan's own cell
// (LEAD_CALLS Q-16: none of the six is a flagship; S-W1 would edit REIGN's here).
const NetSitesFestival IgniteFestivals[] = {
	{"feign", "FEIGN", "FEIGN", "", 11, 30, 10, 2026, 24, 1.10, 22.0, 260, 3, NetSitesFeignRooms, 24, NetSitesFeignFormats, NetSitesFeignEvents, 5, 0xff4fa3, 0x5ff2c9, 32,
		false, "Nothing in here is what it looks like. Except the guarantee.", 15 * 60, 150, 0, 0, false, true, NetSitesFeignNumbers, 3},
};
const NetSitesFestival RunItTwiceFestivals[] = {
	{"grit", "GRIT", "GRIT", "The Grinders' RunItTwice", 5, 12, 12, 2027, 12, 5.50, 109.0, 300, 1, NetSitesGritRooms, 12, NetSitesGritFormats, NetSitesGritEvents, 5, 0xd2141a,
		0xf3e9d2, 32, false, "Every event runs twice a night. Seven o'clock, then ten. Bring grit.", 19 * 60, 0, 180, 22 * 60},
};

/** A blurb's figures from its template (§2.11: every figure from the template, none typed or spelled). */
std::string NetSitesFestBlurb(const char* Blurb, const EventTemplate& T, Chips MainBuyIn)
{
	std::string Out = Blurb;
	const std::pair<const char*, std::string> Vars[] = {{"{BuyIn}", BuyIn(T.BuyInCents)}, {"{Start}", TimeLabel(static_cast<double>(T.StartMinute))},
		{"{MainBuyIn}", BuyIn(MainBuyIn)}};
	for (const auto& V : Vars)
	{
		const std::string Key = V.first;
		for (size_t FillAt = Out.find(Key); FillAt != std::string::npos; FillAt = Out.find(Key, FillAt + V.second.size()))
		{
			Out.replace(FillAt, Key.size(), V.second);
		}
	}
	return Out;
}

/** An event's code in its site's form: Ignite "FEIGN #9", RunItTwice "GRIT 07" (two digits, no "#"; §2.3.2, §2.11 H14). */
std::string NetSitesFestCode(SiteId OnSite, const char* Short, int No)
{
	if (OnSite == SiteId::RunItTwice)
	{
		return std::string(Short) + " " + (No < 10 ? "0" : "") + std::to_string(No);
	}
	return std::string(Short) + " #" + std::to_string(No);
}

/** "Main Event" -> "MAIN EVENT" (Ignite's Main, its hero word; the plan row keeps the mixed case). */
std::string NetSitesFestCaps(const std::string& Name)
{
	std::string Out = Name;
	for (char& C : Out)
	{
		C = C >= 'a' && C <= 'z' ? static_cast<char>(C - 'a' + 'A') : C;
	}
	return Out;
}

/** The runs a template's day holds (its repeats counted: GRIT's shows run twice). */
int NetSitesFestRuns(const EventTemplate& T)
{
	return T.EveryMinutes > 0 && T.LastMinute >= T.StartMinute ? (T.LastMinute - T.StartMinute) / T.EveryMinutes + 1 : 1;
}
} // namespace netsites_detail

void Network::BuildFestivals()
{
	// SPEC v1.7 §2.3.1: last in BuildSite, so every template above keeps its index; rival sites only (RiverLine's four are dressed by
	// BuildSeries and BuildCalendar, §2.2.1).
	using namespace netsites_detail;
	const NetSitesFestival* Plans = nullptr;
	size_t PlanCount = 0;
	switch (NetSite)
	{
	case SiteId::Ignite:
		Plans = IgniteFestivals;
		PlanCount = sizeof(IgniteFestivals) / sizeof(IgniteFestivals[0]);
		break;
	case SiteId::RunItTwice:
		Plans = RunItTwiceFestivals;
		PlanCount = sizeof(RunItTwiceFestivals) / sizeof(RunItTwiceFestivals[0]);
		break;
	case SiteId::RiverLine:
	case SiteId::Halcyon: // P1
	case SiteId::Count: break;
	}
	if (!Plans || Rules->Key.empty())
	{
		return;
	}
	const SiteDef& D = *Rules;
	const std::string Prefix = std::string(KeyOf(NetSite)) + ".";
	const double Scale = D.Traffic.Scale > 0.0 ? D.Traffic.Scale : 1.0;
	const double GtdOfPool = D.GtdOfPool > 0.0 ? D.GtdOfPool : 0.9;
	const auto FestField = [Scale](int Planned) { return static_cast<int>(std::lround(static_cast<double>(Planned) / Scale)); };
	for (size_t P = 0; P < PlanCount; ++P)
	{
		const NetSitesFestival& Pl = Plans[P];
		// The headliners in calendar order (their numbers follow the numbered events'), then the satellites in the plan's order.
		std::vector<const NetSitesFestEvent*> Heads;
		std::vector<const NetSitesFestEvent*> Sats;
		const NetSitesFestEvent* MainRow = nullptr;
		for (int K = 0; K < Pl.EventCount; ++K)
		{
			const NetSitesFestEvent& Ev = Pl.Events[K];
			(Ev.Satellite ? Sats : Heads).push_back(&Ev);
			MainRow = Ev.Main ? &Ev : MainRow;
		}
		std::stable_sort(Heads.begin(), Heads.end(), [](const NetSitesFestEvent* A, const NetSitesFestEvent* B) {
			return A->DayIndex != B->DayIndex ? A->DayIndex < B->DayIndex : A->Start < B->Start;
		});
		const Chips MainBuyIn = MainRow ? NetSitesCents(MainRow->BuyIn) : 0;
		for (int Year = Pl.FirstYear; Year <= fest::kLastYear; ++Year)
		{
			const std::string Yy = std::to_string(Year % 100);
			SeriesInfo Sr;
			Sr.Id = Prefix + Pl.Brand + (Yy.size() < 2 ? "0" + Yy : Yy);
			Sr.Name = std::string(Pl.Name) + " " + std::to_string(Year);
			Sr.Short = Pl.Short;
			Sr.FirstDay = world::DayOn(Year, Pl.Month, Pl.Day);
			Sr.LastDay = Sr.FirstDay + Pl.Span - 1;
			Sr.Color = Pl.Color;
			Sr.Color2 = Pl.Color2;
			Sr.Tagline = Pl.Tagline;
			Sr.MainEvent = Sr.Id + "-main";
			Sr.Festival = true;
			Sr.Brand = Pl.Brand;
			Sr.Draw = Pl.Draw;
			Sr.Flagship = Pl.Flagship;
			Sr.HighRoller = Pl.HighRoller;
			Sr.Title = Pl.Title;
			Chips Gtd = 0;
			int Events = 0;

			// The numbered events: No 1..N, each on its day of the window, started by its slot among that day's numbered events.
			std::vector<int> DayCount(static_cast<size_t>(std::max(1, Pl.Span)), 0);
			std::vector<bool> Used(static_cast<size_t>(Pl.RoomCount), false);
			const uint32_t Key = Fnv1a(Sr.Id);
			for (int No = 1; No <= Pl.Numbered; ++No)
			{
				const NetSitesFestNumber* Set = nullptr;
				for (int K = 0; K < Pl.NumberCount; ++K)
				{
					Set = Pl.Numbers[K].No == No ? &Pl.Numbers[K] : Set;
				}
				const int DayIndex = (No - 1) * Pl.Span / Pl.Numbered;
				const int Slot = DayCount[static_cast<size_t>(DayIndex)]++;
				// The series loop's curve (G/NetworkSites.cpp's), each a standard buy-in.
				const double S = Pl.Numbered > 2 ? static_cast<double>((No * 7) % (Pl.Numbered - 1)) / static_cast<double>(Pl.Numbered - 2) : 0.0;
				double Dollars = NetSitesStep(Pl.MinBuyIn + (Pl.MaxBuyIn - Pl.MinBuyIn) * S * S, Pl.MinBuyIn, Pl.MaxBuyIn);
				int Fmt = Pl.Formats[No % 4];
				int NumEntries = Pl.Entries;
				int Start = Pl.SlotStart + Slot * Pl.SlotStep;
				double GtdScale = 1.0;
				if (Set)
				{
					Dollars = Set->BuyIn > 0.0 ? Set->BuyIn : Dollars;
					Fmt = Set->Fmt >= 0 ? Set->Fmt : Fmt;
					NumEntries = Set->Entries > 0 ? Set->Entries : NumEntries;
					Start = Set->Start >= 0 ? Set->Start : Start;
					GtdScale = Set->GtdScale > 0.0 ? Set->GtdScale : GtdScale;
				}
				const bool Flip = Pl.FlipOpener && No == 1;
				const bool Turbo = Fmt == NetSitesFestReEntryTurbo && !Flip;
				// The room: the list's entry at (Fnv1a(SeriesId) + (No - 1) x 7) % RoomCount, skipping words already used (uint32
				// arithmetic; 7 is coprime with every list's length, so the skip is only a guard).
				uint32_t Room = (Key + static_cast<uint32_t>(No - 1) * 7u) % static_cast<uint32_t>(Pl.RoomCount);
				for (int Guard = 0; Guard < Pl.RoomCount && Used[Room]; ++Guard)
				{
					Room = (Room + 1u) % static_cast<uint32_t>(Pl.RoomCount);
				}
				Used[Room] = true;
				EventTemplate T;
				T.Series = Sr.Id;
				T.EventNo = No;
				T.Id = Sr.Id + "-" + std::to_string(No);
				T.BuyInCents = NetSitesCents(Dollars);
				T.Fmt = Flip ? Format::Flip
					: Fmt == NetSitesFestMystery ? Format::Mystery
					: Fmt == NetSitesFestBounty ? Format::Bounty
					: Fmt == NetSitesFestFreezeout ? Format::Freezeout
					: Format::ReEntry;
				// Ignite's Flip & Run plays as an 8-max hyper (G/NetworkSites.cpp's daily); turbos by the rotation; the rest Regular.
				NetSitesShape(T, Flip ? "Hyper" : Turbo ? "Turbo" : "Regular");
				T.TableSize = No % 6 == 0 ? 6 : 8;
				T.Field = FestField(NumEntries);
				T.OnlyDay = Sr.FirstDay + DayIndex;
				T.StartMinute = Start;
				T.EveryMinutes = Pl.ShowEvery;
				T.LastMinute = Pl.ShowEvery > 0 ? Pl.ShowLast : 1439;
				T.OpensHours = 24 * 14;
				const char* KindWord = Flip ? "Flip & Run"
					: T.Fmt == Format::Mystery ? "Mystery"
					: T.Fmt == Format::Bounty ? "PKO"
					: Turbo ? "Turbo"
					: NetSite == SiteId::RunItTwice ? "NLHE"
					: "No-Limit Hold'em";
				T.Name = NetSitesFestCode(NetSite, Pl.Short, No) + ": " + BuyIn(T.BuyInCents) + " " + Pl.Rooms[Room] + " " + KindWord;
				T.GtdCents = NetSitesGtd(D, T, GtdScale);
				Gtd += T.GtdCents * NetSitesFestRuns(T);
				Events += NetSitesFestRuns(T);
				Temps.push_back(T);
			}

			// The headliners: numbers after the numbered events, the plan's share of the median pool, Added and the curve on the Main.
			int No = Pl.Numbered;
			for (const NetSitesFestEvent* Ev : Heads)
			{
				++No;
				EventTemplate T;
				T.Series = Sr.Id;
				T.EventNo = No;
				T.Main = Ev->Main;
				T.Id = Sr.Id + "-" + Ev->Stem;
				T.BuyInCents = NetSitesCents(Ev->BuyIn);
				T.Fmt = Ev->Fmt;
				NetSitesShape(T, Ev->Speed);
				T.TableSize = 8;
				T.Field = FestField(Ev->Entries);
				T.OnlyDay = Sr.FirstDay + Ev->DayIndex;
				T.StartMinute = Ev->Start;
				T.EveryMinutes = Ev->Every;
				T.LastMinute = Ev->Every > 0 ? Ev->LastStart : 1439;
				T.OpensHours = Ev->Main ? 24 * 30 : 24 * 14;
				T.Featured = Ev->Main;
				const std::string Headliner = Ev->Main && NetSite != SiteId::RunItTwice ? NetSitesFestCaps(Ev->Name) : std::string(Ev->Name);
				T.Name = NetSitesFestCode(NetSite, Pl.Short, No) + ": " + BuyIn(T.BuyInCents) + " " +
					(NetSite == SiteId::RunItTwice ? "NLHE [" + Headliner + "]" : Headliner);
				T.GtdCents = Ev->GtdShare > 0.0 ? NetSitesGtd(D, T, Ev->GtdShare / GtdOfPool) : 0;
				T.AddedCents = Ev->Main ? Ev->AddedCents : 0;
				T.Curve = Ev->Main && Ev->Curve ? PayoutCurve::Main : PayoutCurve::Standard;
				T.Blurb = NetSitesFestBlurb(Ev->Blurb, T, MainBuyIn);
				Gtd += T.GtdCents * NetSitesFestRuns(T);
				Events += NetSitesFestRuns(T);
				Temps.push_back(T);
			}

			// The satellites: no number and no series (the hub finds them through Festival::Satellites), each seating the edition's Main
			// at its buy-in, with no seat guarantee (FD-7: the seats are the pool's, max(1, Pool / SeatValue)).
			for (const NetSitesFestEvent* Ev : Sats)
			{
				EventTemplate T;
				T.Id = Sr.Id + "-" + Ev->Stem;
				T.Name = Ev->Name;
				T.BuyInCents = NetSitesCents(Ev->BuyIn);
				T.Fmt = Format::Satellite;
				NetSitesShape(T, Ev->Speed);
				T.TableSize = 8;
				T.Field = FestField(Ev->Entries);
				T.OnlyDay = Sr.FirstDay + Ev->DayIndex;
				T.StartMinute = Ev->Start;
				T.EveryMinutes = Ev->Every;
				T.LastMinute = Ev->Every > 0 ? Ev->LastStart : 1439;
				T.OpensHours = 24 * 14;
				T.Seats = std::string(Pl.Short) + " Main Event seats";
				T.SeatTicket = Sr.MainEvent;
				T.SeatValueCents = MainBuyIn;
				T.GtdCents = Ev->Seats > 0 ? static_cast<Chips>(Ev->Seats) * MainBuyIn : 0;
				T.Blurb = NetSitesFestBlurb(Ev->Blurb, T, MainBuyIn);
				Temps.push_back(T);
			}
			Sr.Events = Events;
			Sr.GtdCents = Gtd;
			AllSeries.push_back(Sr);
		}
	}
}

void Network::FillDayMax(double /*Now*/) const
{
}

int Network::DayMaxOn(int /*Day*/) const
{
	// H0 (PLAN_FNET P1): 0, as if FillDayMax had walked nothing. Nothing calls it before F-net's fest::RecordOf, which lands with
	// this body (DayMax's entry for Day) at FN-7.
	return 0;
}
} // namespace net
} // namespace ss
