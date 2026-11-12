// Update 4, the festival hub (SPEC v1.7 §2.8): one shared module worn in each site's frame (THE FESTIVAL YEAR, the banner, the Main
// card, the day strip and rows, the ticker, THE RACE, CHAMPIONS, YOUR <FESTIVAL>), Main Event Day, the champion's, the belt's, a
// mark's and a title's moments, the strip's popover, the Career page's pins and the hub's strings (FestUiInventory). F-hubs' file
// from G1. It reaches the client's private state through rlfest_detail::Access (RiverLine.h's friend), forwarders defined here.
//
// Phase A (§6 PA-46; the lead): stubs that draw nothing. Draw calls FestivalOverlays at its overlay stage; no page routes to
// FestivalHub or CareerPins before G1. Access is empty.
//
// G1 FH-1a (F-hubs' stream A; SPEC v1.7 §2.8.1-§2.8.3, §2.1.4, §2.2.3, §2.11, §2.12; PLAN_G1 §2.1, PLAN_FHUBS §4 and its
// Corrections): the hub's skeleton. Stream B's FH-1c routes a festival series to FestivalHub on the three hosts (UI/RiverLineNet.cpp:
// NetHubEdition, NetHubRect); here:
// - rlfest_detail::Gather reads every fact the hub prints into one HubFacts value, once a frame: every digit a sim fact (§2.8.3) and
//   every world read the hub makes (the const views of H/WorldFest.h, FestRaceKey, FestTop, FestRoll and HeroFestPins, and
//   fest::PhaseOf's crown minute), so G2 can point it at the held view. It calls no HoldGuarded world method (PLAN_FHUBS Q17) and
//   draws no random number: every presentation clock (the countdown, the banner's motions) is real time and feeds no outcome (§2.12).
// - rlfest_detail::HubFrameOf: the per-host layout table (§2.8.2's rects: RiverLine's, Ignite's under L7, RunItTwice's bracketed
//   ones; FH-2 proves the last two with its pages).
// - The banner (the look's FestBanner and the FestWordMark overload, F-look's bodies from FL-7; today's crest at 150; the title, the
//   tagline and RCOP's second line; the four stats), the Main card (the date and start, "{Gtd} GUARANTEED", the buy-in, "+ {Added}
//   ADDED", the lock line, "{N} SEATS WON BY SATELLITE", Details `fsmain`), the right column's tabs `fstab0..2` with the phase's
//   preselection, and each tab's empty state (U6, U9, U11). The strip, the day strip, the rows, the ticker and the popover are FH-1b's;
//   the race's rows, the roll's rows and the YOUR panel FH-4A's; Main Event Day, Closing and the Afterglow FH-5b's.
// - The UI side state (the IgniteStates precedent): the tab the player picked and the hub's open time, kept by laptop and started
//   afresh whenever the client opens a page or a series, or shows another edition.
// - FestUiInventory: the strings this file draws, each with its kind (§2.11), then stream B's through rlfest_detail::HostUiStrings.
//
// G1 FH-1b (stream A; SPEC v1.7 §2.8.2, §2.8.3, §2.8.6, §2.4.4, §2.2.3, §2.1.4, §2.10 item 6, §2.11; PLAN_G1 §2.1): the rest of the
// hub's left half and the strip.
// - THE FESTIVAL YEAR (`fsy0..9` in calendar order, WRIT ... FEIGN): each brand's on-stage edition, else its next (PLAN_FHUBS Q24),
//   from fest::All() across the three sites (a brand with no edition yet leaves its cell empty); the low-detail crest at 26, the
//   Short, the opening date, "BY INVITATION" on a rival cell without an account (FH-2 adds the Pass's count, U30); an edition
//   inside its Announced-to-Afterglow window is lit, and the one on stage (else the next to open) carries the Accent underline.
// - The strip's popover (§2.8.6), drawn in FestivalOverlays: `{cell.X, 128, 360, 168}` clamped inside the hub's content rect, the
//   crest at 46, the name and its full title (N17-N19), the host and the window, U16's phase and account lines, and one button
//   (U17: OPEN THE HUB on the host in front, GO TO {SITE} for another site's edition with an account open; none without one). It
//   holds the pointer (house law 3): while it is open the whole hub draws under SitesBlockUnder's modal hold, so a click on it
//   reaches nothing beneath and a click elsewhere closes it, swallowed; the frame it opens takes no input of its own
//   (FestOneFrameClicks). Esc is the client's key handler's (UI/RiverLine.cpp), which F-hubs' files don't reach at G1.
// - The day strip (`fsd<day>`: today lit, a star on Main day, an outline star on a championship's other title days) and the day's
//   rows (`ev:<id>`, written to Listed): the edition's events that day, then one row for each satellite into its Main that runs
//   that day (the ladder up to it included: RCOP's step1-4 and mega-rcop, §2.2.3), seven in view and the rest under the wheel;
//   each row's button is the client's own RegisterBlock (SitesTwoPress, the clause's second press and core's strike gate, W3;
//   FestStrikeGate), shown on every row until one is armed and then on that one alone (Session::ConfirmRegister is one per
//   session, the RunItTwice Board's rule); a satellite into a Main that isn't joinable adds the Main's lock line and U31.
// - The overlay ticker (§2.4.4; none on Ignite's hub, L7): U26's forms for each of the edition's guaranteed headliners and the
//   Main's registrations so far, rebuilt once a world minute, one clipped run.
#include "ShortStack/UI/RiverLine.h"

#include "RiverLineShared.h"
#include "ShortStack/Game/Festival.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/SiteShow.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Game/WorldFest.h"
#include "ShortStack/UI/EventArt.h"
#include "ShortStack/UI/FestivalArt.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace ui
{
namespace rlfest_detail
{
// UI/RiverLineNet.cpp (stream B, FH-1c; LEAD_CALLS R-1): the host files' festival strings, joined to FestUiInventory below.
std::vector<std::string> HostUiStrings();
} // namespace rlfest_detail
} // namespace ui

namespace fest
{
// Step A-2 (LEAD_CALLS Q-5; moved here from G/Festival.cpp by the integrator): the hubs', the register sheets' and the takeovers' UI
// strings. G1 (FH-1a on): every string this file draws, each "<kind>\t<text>" (§2.11: a label or a sentence, which FestCopyUi maps to
// the stop rule and the cap), its placeholders filled from the sim when drawn; then stream B's (rlfest_detail::HostUiStrings).
std::vector<std::string> FestUiInventory()
{
	std::vector<std::string> Out = {
		// U2: the banner's fourth stat, a value over its label.
		"label\tOPENS IN",
		"label\t{D}D {H}H {M}M",
		"label\tPROGRESS",
		"label\tDAY {N} OF {Days}",
		"label\tMAIN EVENT TODAY",
		"label\tCOMPLETE",
		// U3: the banner's other stats and the Main card.
		"label\tGUARANTEED",
		"label\tEVENTS",
		"label\tDATES",
		"label\t{FirstDate} \xE2\x80\x93 {LastDate}",
		"label\tMAIN EVENT \xC2\xB7 {Weekday} {Date} \xC2\xB7 {Time}",
		"label\t{Gtd} GUARANTEED",
		"label\t{BuyIn} buy-in \xC2\xB7 {Stack} chips",
		"label\t+ {Added} ADDED",
		"label\t{N} SEATS WON BY SATELLITE",
		"label\t1 SEAT WON BY SATELLITE",
		"label\tDetails",
		// U5: the right column's tabs and THE RACE's title.
		"label\tTHE RACE",
		"label\tCHAMPIONS",
		"label\tYOUR {FESTIVAL}",
		"label\tPLAYER OF THE SERIES",
		// U6: THE RACE's lines.
		"sentence\tNo points yet.",
		"sentence\tEvery cash scores, yours and every regular's. First place takes the belt.",
		"sentence\tThis edition's race began before your update: no Player of the Series this time.",
		// U9: CHAMPIONS' two honest lines while the roll is empty.
		"sentence\tNo champion yet. The first name goes up on {Date}.",
		"sentence\tThe record books opened {Date}, {Year}.",
		// U11: YOUR <FESTIVAL> before the hero's first event of the edition.
		"sentence\tPlay any {Festival} event for your pin.",
		// G7's second line (the hub banner only, beside RCOP's tagline and never in its place; fest::Inventory lists G7 too).
		"sentence\tThe river runs highest in the fall.",
		// FH-1b. U1: the strip's label and a rival cell without an account.
		"label\tTHE FESTIVAL YEAR",
		"label\tBY INVITATION",
		// The day's rows (§2.8.2): GTD or pool, the entries and the status, the pills (U3's MAIN EVENT, U34's titles); U31 beside a
		// satellite into a Main that isn't joinable yet (§2.2.3).
		"label\t{Gtd} GTD",
		"label\t{Entries} entries",
		"label\t1 entry",
		"label\tIN {Countdown}",
		"label\tLATE {Clock}",
		"label\tMAIN EVENT",
		"label\tMINI MAIN EVENT",
		"label\tMICRO MAIN EVENT",
		"label\tSATELLITE",
		"sentence\tA seat won here waits in your wallet until big fields open.",
		// The overlay ticker (§2.4.4): U26's three forms, and the Main's registrations so far (U3's MAIN EVENT with U4's "registered").
		"sentence\t{Site} added {Overlay} to cover {Event}'s guarantee.",
		"sentence\t{Event} is projected {Amount} short.",
		"sentence\t{Event} is projected {Amount} over.",
		"label\tMAIN EVENT \xC2\xB7 {Entries} registered",
		// The strip's popover (§2.8.6): the host and the window, U16's phase and account lines, U17's buttons.
		"label\t{Site} \xC2\xB7 {FirstDate} \xE2\x80\x93 {LastDate}",
		"sentence\tOpens {Date}.",
		"sentence\tDay {N} of {Days}.",
		"sentence\tCrowned {Date}.",
		"sentence\tBy invitation.",
		"sentence\tYour account is open.",
		"label\tOPEN THE HUB",
		"label\tGO TO {SITE}",
	};
	for (std::string& Line : ui::rlfest_detail::HostUiStrings())
	{
		Out.push_back(std::move(Line));
	}
	return Out;
}
} // namespace fest

namespace ui
{
namespace rlfest_detail
{
using rlnet_detail::NetA;
using rlnet_detail::NetMoney;
using rlnet_detail::NetUpper;

/** The client's private state the hub's helpers read (RiverLine.h's friend). */
struct Access
{
	using Page = RiverLine::Page;
	static const Session& SessionOf(const RiverLine& RL) { return RL.S; }
	static double PageAt(const RiverLine& RL) { return RL.PageAt; }
	static double SeriesAt(const RiverLine& RL) { return RL.SeriesAt; }
	static void OpenPage(RiverLine& RL, Page P, double Now) { RL.OpenPage(P, Now); }
	static void SelectEvent(RiverLine& RL, const std::string& Id) { RL.SelectEvent(Id); }
	// FH-1b: the rows' list (the arrow keys walk it, as the lobby's schedule), the client's own register button, a winner's name.
	static void SetListed(RiverLine& RL, std::vector<std::string> Ids) { RL.Listed = std::move(Ids); }
	static void RegisterBlock(RiverLine& RL, const net::EventInstance& E, const net::LiveState& L, const Rect& R, double Now) { RL.RegisterBlock(E, L, R, Now); }
	static std::string PlacingName(const RiverLine& RL, const net::Placing& P) { return RL.PlacingName(P); }
	static double WorldOf(const RiverLine& RL) { return RL.World; }
};

/** THE FESTIVAL YEAR's ten brands in calendar order (§2.8.2: the cells' ids fsy0..9). */
constexpr fest::Brand HubYearOrder[10] = {fest::Brand::Writ, fest::Brand::RingRush, fest::Brand::Pennant, fest::Brand::Grit, fest::Brand::Tco,
	fest::Brand::Reign, fest::Brand::Slam, fest::Brand::Push, fest::Brand::Rcop, fest::Brand::Feign};

/** One cell of THE FESTIVAL YEAR (§2.8.2, §2.1.4): a brand's on-stage edition, else its next (PLAN_FHUBS Q24). */
struct HubYearCell
{
	const fest::Festival* F = nullptr;       // null: the brand has no edition yet (F-net's rows land package by package)
	const net::SeriesInfo* Series = nullptr; // the edition's series on its host
	fest::Phase Ph = fest::Phase::Off;
	bool OnStage = false;                    // Live, Main day, Closing or the Afterglow
	bool Lit = false;                        // inside its Announced-to-Afterglow window, or underlined
	bool Underline = false;                  // the edition on stage (else the next to open): the 2-unit Accent underline
	bool Account = true;                     // the player holds an account on the host (RiverLine always)
	double CrownedAt = -1.0;                 // the crown minute (world::FestCrownedAt), -1 before it
};

/** Every fact the hub prints (§2.8.3), read once a frame by Gather. */
struct HubFacts
{
	const fest::Festival* F = nullptr;
	const net::SeriesInfo* Series = nullptr; // the edition's series on its host (null: the hub draws nothing)
	fest::Phase Ph = fest::Phase::Off;
	double World = 0.0;
	int Today = 0;
	int DayCount = 0;          // the window's days, LastDay - FirstDay + 1
	double OpensIn = 0.0;      // world minutes to FirstDay 00:00 (the lobby runs one world minute a real minute)
	float DayProgress = 0.0f;  // the world day's fraction, changed once a world minute (FestBanner, the word-mark's Gauge)
	bool HasMain = false;      // the Main's instance: Network::Next(MainId) from the edition's first day
	net::EventInstance Main;
	Chips MainBuyIn = 0, MainGtd = 0, MainAdded = 0;
	int MainStack = 0;
	std::string MainLock;      // the Main's lock line while it isn't joinable (RCOP and Summer Slam until G3, §2.2.3); "" if it is
	int SeatsWon = 0;          // fest::SeatsWon: Main-ticket seats only
	bool RaceOpen = false;     // the race the world's views read is this edition's (world::FestRaceKey)
	int RaceRows = 0;          // the rows world::FestTop holds for it (THE RACE's rows: FH-4A)
	bool RollEmpty = true;     // world::FestRoll holds no edition of the brand (U9)
	bool HasPin = false;       // the hero holds this edition's pin (world::HeroFestPins)
	HubYearCell Year[10];      // THE FESTIVAL YEAR, in HubYearOrder (FH-1b)
	std::vector<int> TitleDays; // the edition's other title days (a championship's Micro and Mini: the day strip's outline stars)
};

/** The world minute a day begins. */
double HubMinute(int Day)
{
	return static_cast<double>(Day) * net::MinutesPerDay;
}

/** THE FESTIVAL YEAR's cells (§2.8.2, §2.1.4; FH-1b): for each brand, the edition on stage (Live to the Afterglow), else the next to
 *  open; lit inside the Announced-to-Afterglow window; the on-stage editions underlined, or the next to open when none is. World
 *  reads: PhaseOf's crown minute and FestCrownedAt, both const views. */
void HubYear(const Session& S, double World, const world::World* W, HubFacts& H)
{
	const int Today = net::DayOf(World);
	// A fortnight back holds every tail (Closing, an Afterglow, an uncrowned edition's two days, R-13); a year and a month ahead
	// holds every brand's next edition.
	constexpr int TailDays = 14;
	const std::vector<const fest::Festival*> Near = fest::Festivals(Today - TailDays, Today + 400);
	bool AnyOnStage = false;
	for (int I = 0; I < 10; ++I)
	{
		HubYearCell& YC = H.Year[I];
		YC = HubYearCell();
		for (const fest::Festival* F : Near)
		{
			if (F->Which != HubYearOrder[I])
			{
				continue;
			}
			if (F->FirstDay <= Today)
			{
				const fest::Phase Ph = fest::PhaseOf(*F, World, W);
				if (Ph == fest::Phase::Live || Ph == fest::Phase::MainDay || Ph == fest::Phase::Closing || Ph == fest::Phase::Afterglow)
				{
					YC.F = F;
					YC.Ph = Ph;
					YC.OnStage = true;
					break;
				}
				continue;
			}
			if (!YC.F)
			{
				YC.F = F; // Near is in FirstDay order: the brand's first edition still to open
				YC.Ph = fest::PhaseOf(*F, World, W);
			}
		}
		if (!YC.F)
		{
			continue;
		}
		YC.Series = net::Site(YC.F->Site).FindSeries(YC.F->SeriesId);
		YC.Lit = YC.Ph != fest::Phase::Off;
		YC.Account = YC.F->Site == net::SiteId::RiverLine || rlsites_detail::SitesAccount(S, YC.F->Site) != nullptr;
		if (YC.Ph == fest::Phase::Afterglow && W)
		{
			YC.CrownedAt = world::FestCrownedAt(*W, YC.F->SeriesId);
		}
		AnyOnStage = AnyOnStage || YC.OnStage;
	}
	int Next = -1;
	for (int I = 0; I < 10; ++I)
	{
		HubYearCell& YC = H.Year[I];
		if (!YC.F || !YC.Series)
		{
			YC.F = nullptr;
			continue;
		}
		if (AnyOnStage)
		{
			YC.Underline = YC.OnStage;
		}
		else if (Next < 0 || YC.F->FirstDay < H.Year[Next].F->FirstDay)
		{
			Next = I;
		}
	}
	if (Next >= 0)
	{
		H.Year[Next].Underline = true;
	}
	for (HubYearCell& YC : H.Year)
	{
		YC.Lit = YC.Lit || YC.Underline;
	}
}

HubFacts Gather(const Session& S, const fest::Festival& F, double World)
{
	HubFacts H;
	H.F = &F;
	H.World = World;
	H.Today = net::DayOf(World);
	const net::Network& Net = net::Site(F.Site);
	H.Series = Net.FindSeries(F.SeriesId);
	const world::World* W = Net.Attached();
	H.Ph = fest::PhaseOf(F, World, W);
	H.DayCount = F.LastDay - F.FirstDay + 1;
	H.OpensIn = HubMinute(F.FirstDay) - World;
	H.DayProgress = static_cast<float>(std::floor(World - HubMinute(H.Today)) / net::MinutesPerDay);
	H.HasMain = !F.MainId.empty() && Net.Next(F.MainId, HubMinute(F.FirstDay), H.Main);
	if (H.HasMain)
	{
		const net::EventTemplate& T = Net.TemplateOf(H.Main);
		H.MainBuyIn = T.BuyInCents;
		H.MainGtd = T.GtdCents;
		H.MainAdded = T.AddedCents;
		H.MainStack = T.StartStack;
		if (Net.Joinable(H.Main, &H.MainLock, S.Unlocks()))
		{
			H.MainLock.clear();
		}
	}
	H.SeatsWon = fest::SeatsWon(F, World);
	if (W)
	{
		H.RaceOpen = world::FestRaceKey(*W) == F.SeriesId;
		H.RaceRows = H.RaceOpen ? static_cast<int>(world::FestTop(*W, 10).size()) : 0;
		H.RollEmpty = world::FestRoll(*W, F.Which).empty();
		for (const world::HeroFestPin& Pin : world::HeroFestPins(*W))
		{
			H.HasPin = H.HasPin || Pin.Key == F.SeriesId;
		}
	}
	HubYear(S, World, W, H);
	for (size_t K = 1; K < F.TitleIds.size(); ++K)
	{
		net::EventInstance Title;
		if (Net.Next(F.TitleIds[K], HubMinute(F.FirstDay), Title))
		{
			H.TitleDays.push_back(net::DayOf(Title.Start));
		}
	}
	return H;
}

/** One host's regions (§2.8.2, in RiverLine::Width x Height units). */
struct HubFrame
{
	Rect Year, Banner, Main, Days, Rows, Ticker, Tabs, Body;
	float CrestX = 0.0f, CrestY = 0.0f; // the banner crest's centre (150 across)
	float WordX = 0.0f;                 // the word-mark's, the title's, the tagline's and the stats' left edge
	float StatGap = 200.0f;             // the four stats' pitch
	bool HasTicker = true;              // Ignite's hub has none (L7)
	float YearX = 260.0f;               // THE FESTIVAL YEAR's first cell (v1.6: ten cells of 128 from x 260; RunItTwice 116 from x 368)
	float YearPitch = 128.0f;
	float YearText = 92.0f;             // a cell's text column: a Short gets 92 units (80 on RunItTwice)
};

const HubFrame& HubFrameOf(net::SiteId Site)
{
	static const HubFrame HubFrameRiverLine = [] {
		HubFrame Fr;
		Fr.Year = {24.0f, 84.0f, 1552.0f, 40.0f};
		Fr.Banner = {24.0f, 132.0f, 1552.0f, 204.0f};
		Fr.Main = {1166.0f, 152.0f, 386.0f, 164.0f};
		Fr.Days = {24.0f, 348.0f, 1000.0f, 52.0f};
		Fr.Rows = {24.0f, 408.0f, 1000.0f, 504.0f};
		Fr.Ticker = {24.0f, 920.0f, 1000.0f, 30.0f};
		Fr.Tabs = {1044.0f, 348.0f, 532.0f, 36.0f};
		Fr.Body = {1044.0f, 392.0f, 532.0f, 558.0f};
		Fr.CrestX = 114.0f;
		Fr.CrestY = 234.0f;
		Fr.WordX = 210.0f;
		return Fr;
	}();
	// Ignite (L7): RiverLine's rects but the rows' (cut at y 870, above the dock) and the body's (to y 870), and no overlay ticker.
	static const HubFrame HubFrameIgnite = [] {
		HubFrame Fr = HubFrameRiverLine;
		Fr.Rows = {24.0f, 408.0f, 1000.0f, 462.0f};
		Fr.Ticker = {24.0f, 920.0f, 0.0f, 0.0f};
		Fr.Body = {1044.0f, 392.0f, 532.0f, 478.0f};
		Fr.HasTicker = false;
		return Fr;
	}();
	// RunItTwice: §2.8.2's bracketed rects, right of the rail; the stats a little closer, so the fourth clears the Main card at x 1170.
	static const HubFrame HubFrameRunItTwice = [] {
		HubFrame Fr;
		Fr.Year = {232.0f, 96.0f, 1344.0f, 40.0f};
		Fr.Banner = {232.0f, 144.0f, 1344.0f, 196.0f};
		Fr.Main = {1170.0f, 160.0f, 390.0f, 164.0f};
		Fr.Days = {232.0f, 352.0f, 860.0f, 52.0f};
		Fr.Rows = {232.0f, 412.0f, 860.0f, 504.0f};
		Fr.Ticker = {232.0f, 924.0f, 860.0f, 26.0f};
		Fr.Tabs = {1108.0f, 352.0f, 468.0f, 36.0f};
		Fr.Body = {1108.0f, 396.0f, 468.0f, 554.0f};
		Fr.CrestX = 322.0f;
		Fr.CrestY = 242.0f;
		Fr.WordX = 418.0f;
		Fr.StatGap = 180.0f;
		Fr.YearX = 368.0f;
		Fr.YearPitch = 116.0f;
		Fr.YearText = 80.0f;
		return Fr;
	}();
	switch (Site)
	{
	case net::SiteId::Ignite: return HubFrameIgnite;
	case net::SiteId::RunItTwice: return HubFrameRunItTwice;
	default: break;
	}
	return HubFrameRiverLine;
}

/** The hub's side state on one laptop (the IgniteStates precedent): what the player picked and when the hub opened. */
struct HubUi
{
	const Session* Of = nullptr;
	std::string Key;          // the edition shown
	double PageAt = 0.0;      // the client's page and series clocks when it opened: a new open (or a new laptop's) starts afresh
	double SeriesAt = 0.0;
	double OpenedAt = 0.0;    // real seconds: the banner's and the word-mark's motions run from here
	int Tab = -1;             // the tab the player picked, -1 the phase's (HubAutoTab)
	// FH-1b.
	int Day = -1;             // the day strip's pick (-1: today, held inside the window)
	int RowFirst = 0;         // the first row in view (the wheel scrolls the day's rows)
	bool Drew = false;        // the hub drew in this frame (FestivalOverlays draws the popover only over it, then clears it)
	struct Pop
	{
		bool Open = false;
		bool Fresh = false;   // opened in this frame: drawn, but it takes no input until the next (FestOneFrameClicks)
		int Cell = -1;        // fsy<Cell>
		Rect R;               // {cell.X, 128, 360, 168}, clamped inside the hub's content rect
		HubYearCell C;        // the cell's facts as Gather read them (FestivalOverlays reads no world)
		int Today = 0;
	} Popover;
	// The day's rows, rebuilt once a world minute or when the edition or the day changes.
	std::vector<std::string> Ladder; // the edition's satellites and the ladder up to them (built once an edition)
	bool LadderBuilt = false;
	long long RowsMinute = -1;
	int RowsDay = -9999;
	std::vector<net::EventInstance> Rows;
	// The overlay ticker's run, rebuilt once a world minute.
	long long TickerMinute = -1;
	std::string Ticker;
};

std::map<const RiverLine*, HubUi>& HubStates()
{
	static std::map<const RiverLine*, HubUi> FestHubByLaptop;
	return FestHubByLaptop;
}

HubUi& HubOf(const RiverLine& RL, const fest::Festival& F, double Now)
{
	HubUi& U = HubStates()[&RL];
	const Session* Of = &Access::SessionOf(RL);
	const double PageAt = Access::PageAt(RL);
	const double SeriesAt = Access::SeriesAt(RL);
	if (U.Of != Of || U.Key != F.SeriesId || U.PageAt != PageAt || U.SeriesAt != SeriesAt)
	{
		U = HubUi();
		U.Of = Of;
		U.Key = F.SeriesId;
		U.PageAt = PageAt;
		U.SeriesAt = SeriesAt;
		U.OpenedAt = Now;
	}
	return U;
}

/** The tab the hub opens on (§2.8.2): THE RACE while the edition is on stage (Live, Main day, Closing), CHAMPIONS during Announced,
 *  Buildup and the Afterglow, YOUR <FESTIVAL> once the hero holds the edition's pin, but never past THE RACE while the Main is locked
 *  and the edition is on stage (§2.2.3: the hub opens on THE RACE, never on a locked Main). */
int HubAutoTab(const HubFacts& H)
{
	const bool OnStage = H.Ph == fest::Phase::Live || H.Ph == fest::Phase::MainDay || H.Ph == fest::Phase::Closing;
	if (H.HasPin && !(OnStage && !H.MainLock.empty()))
	{
		return 2;
	}
	return OnStage ? 0 : 1;
}

/** U2's countdown value, "12D 07H 41M" (world minutes; the lobby's world minute is a real minute). */
std::string HubCountdown(double Minutes)
{
	const long long M = std::max(0LL, static_cast<long long>(std::floor(Minutes)));
	return std::to_string(M / 1440) + "D " + rlnet_detail::NetPad2((M / 60) % 24) + "H " + rlnet_detail::NetPad2(M % 60) + "M";
}

/** The banner's fourth stat (§2.8.2, U2): the countdown over OPENS IN before the window, then DAY {N} OF {Days}, MAIN EVENT TODAY
 *  and COMPLETE over PROGRESS. */
std::pair<std::string, std::string> HubStateStat(const HubFacts& H)
{
	if (H.OpensIn > 0.0)
	{
		return {HubCountdown(H.OpensIn), "OPENS IN"};
	}
	if (H.Ph == fest::Phase::MainDay)
	{
		return {"MAIN EVENT TODAY", "PROGRESS"};
	}
	if (H.Today > H.F->LastDay)
	{
		return {"COMPLETE", "PROGRESS"};
	}
	return {"DAY " + std::to_string(H.Today - H.F->FirstDay + 1) + " OF " + std::to_string(H.DayCount), "PROGRESS"};
}

/** One frame of the hub: the client, its canvas, the facts and the host's frame. */
struct HubCtx
{
	RiverLine& RL;
	Canvas& C;
	Ui& UI;
	const HubFacts& H;
	const HubFrame& Fr;
	const festart::FestivalLook& Look;
	double Now = 0.0;
	double SinceOpen = 0.0; // real seconds since the hub opened
	bool Still = false;     // the stream preview's settled frame (no motion, no input)
};

/** The banner (§2.8.2): the look's gradient and motif (FestBanner), today's crest at 150, the word-mark (46), the full title, the
 *  tagline and RCOP's second line, and the four stats from WordX, StatGap apart, each a value over its label. */
void HubBanner(HubCtx& X)
{
	const HubFacts& H = X.H;
	const Rect& B = X.Fr.Banner;
	// A ground under the look (FestBanner paints the Bg2 -> Felt gradient over it once F-look's body lands, FL-7).
	X.C.FillRoundRect(B, 18.0f, Paint::Linear({B.X, 0.0f}, {B.X + B.W, 0.0f}, rlsites_detail::SitesTone(0x0e1726), rlsites_detail::SitesTone(0x111c2e)));
	festart::FestBanner(X.C, X.Look, B, X.SinceOpen, H.DayProgress, H.Ph == fest::Phase::MainDay, X.Still);
	X.C.StrokeRoundRect(B, 18.0f, NetA(pal::Line, 0.9f), 1.0f);
	eventart::SeriesCrest(X.C, *H.Series, X.Fr.CrestX, X.Fr.CrestY, 150.0f, X.Still ? 0.0 : X.Now);
	// PC-10: the overload with DayProgress and Still (an eight-argument call ending in a float would bind the old one).
	festart::FestWordMark(X.C, X.Look, H.F->Name, X.Fr.WordX, 196.0f, 46.0f, X.SinceOpen, H.DayProgress, X.Still);
	if (X.Look.Which == fest::Brand::None)
	{
		// No look yet (F-look's table, FL-1): the name in the site's title face, where the word-mark stands.
		X.UI.Text(H.F->Name, X.Fr.WordX, 196.0f, Ts(46.0f, 900, pal::Ink));
	}
	const float TextW = X.Fr.Main.X - X.Fr.WordX - 24.0f;
	float TagY = 228.0f;
	if (!H.Series->Title.empty())
	{
		X.UI.Text(NetUpper(H.Series->Title), X.Fr.WordX, 214.0f, Ts(13.0f, 700, pal::Accent2, Align::Left, Baseline::Alphabetic, false, TextW));
		TagY = 236.0f;
	}
	X.UI.Text(H.Series->Tagline, X.Fr.WordX, TagY, Ts(16.0f, 500, rlsites_detail::SitesTone(0xc3cedf), Align::Left, Baseline::Alphabetic, false, TextW));
	if (H.F->Which == fest::Brand::Rcop)
	{
		X.UI.Text("The river runs highest in the fall.", X.Fr.WordX, 246.0f, Ts(13.0f, 500, pal::Accent2, Align::Left, Baseline::Alphabetic, false, TextW));
	}
	const std::pair<std::string, std::string> State = HubStateStat(H);
	const std::pair<std::string, std::string> Stats[4] = {
		{NetMoney(H.Series->GtdCents), "GUARANTEED"},
		{std::to_string(H.Series->Events), "EVENTS"},
		{net::DateLabel(H.F->FirstDay) + " \xE2\x80\x93 " + net::DateLabel(H.F->LastDay), "DATES"},
		State,
	};
	for (int I = 0; I < 4; ++I)
	{
		const float Sx = X.Fr.WordX + X.Fr.StatGap * static_cast<float>(I);
		const float MaxW = I == 3 ? X.Fr.Main.X - Sx - 16.0f : X.Fr.StatGap - 12.0f;
		X.UI.Text(Stats[I].first, Sx, 292.0f, Ts(24.0f, 900, I == 0 ? pal::Gold : pal::Ink, Align::Left, Baseline::Alphabetic, false, MaxW));
		X.UI.Text(Stats[I].second, Sx, 314.0f, Ts(11.0f, 800, pal::Muted));
	}
}

/** The Main card (§2.8.2): MAIN EVENT with the date and start, "{Gtd} GUARANTEED", the buy-in and stack, "+ {Added} ADDED" (the rival
 *  Mains), the lock line while the Main isn't joinable (§2.2.3), "{N} SEATS WON BY SATELLITE" once a seat is won, and Details
 *  (`fsmain`), which opens the lobby's event panel on the Main. The defending champion (U4) and "Last year: {Prize}." (U23) are
 *  FH-4A's and FH-3a's; the Main's state through Closing FH-5b's. */
void HubMainCard(HubCtx& X)
{
	const HubFacts& H = X.H;
	if (!H.HasMain)
	{
		return;
	}
	const Rect& M = X.Fr.Main;
	X.UI.RRect(M, 14.0f, Rgba(4, 8, 16, 0.6f), NetA(pal::Gold, 0.55f));
	const int Day = net::DayOf(H.Main.Start);
	rlnet_detail::NetSpaced(X.C, "MAIN EVENT \xC2\xB7 " + NetUpper(net::WeekdayName(Day)) + " " + NetUpper(net::DateLabel(Day)) + " \xC2\xB7 " + net::TimeLabel(H.Main.Start),
		M.X + 18.0f, M.Y + 26.0f, 10.5f, 800, pal::Gold, 1.5f);
	if (H.MainGtd > 0)
	{
		X.UI.Text(NetMoney(H.MainGtd) + " GUARANTEED", M.X + 18.0f, M.Y + 58.0f, Ts(22.0f, 900, pal::Gold, Align::Left, Baseline::Alphabetic, false, M.W - 36.0f));
	}
	float Y = M.Y + 80.0f;
	X.UI.Text(net::BuyIn(H.MainBuyIn) + " buy-in \xC2\xB7 " + Grouped(H.MainStack) + " chips", M.X + 18.0f, Y, Ts(13.0f, 600, rlsites_detail::SitesTone(0xc3cedf)));
	if (!H.MainLock.empty())
	{
		Y += 20.0f;
		rlnet_detail::NetLockIcon(X.C, M.X + 18.0f, Y - 10.0f, 11.0f, pal::Orange);
		X.UI.Text(H.MainLock, M.X + 34.0f, Y, Ts(12.0f, 600, pal::Orange, Align::Left, Baseline::Alphabetic, false, M.W - 52.0f));
	}
	if (H.MainAdded > 0)
	{
		Y += 20.0f;
		X.UI.Text("+ " + NetMoney(H.MainAdded) + " ADDED", M.X + 18.0f, Y, Ts(12.0f, 800, pal::Accent));
	}
	if (H.SeatsWon > 0)
	{
		Y += 20.0f;
		X.UI.Text(H.SeatsWon == 1 ? std::string("1 SEAT WON BY SATELLITE") : std::to_string(H.SeatsWon) + " SEATS WON BY SATELLITE", M.X + 18.0f, Y, Ts(11.0f, 800, pal::Muted));
	}
	ButtonOpts O;
	O.Size = 14.0f;
	if (rlsites_detail::SitesButton(X.UI, X.C, "fsmain", {M.X + M.W - 114.0f, M.Y + M.H - 44.0f, 96.0f, 32.0f}, "Details", O) && !X.Still)
	{
		Access::SelectEvent(X.RL, H.Main.Id);
		Access::OpenPage(X.RL, RiverLine::Page::Lobby, X.Now);
	}
}

/** THE FESTIVAL YEAR (§2.8.2, U1; FH-1b): the label in one NetSpaced line, then the ten cells in calendar order (`fsy0..9`), each its
 *  brand's on-stage or next edition: the low-detail crest at 26, the Short (13/800 in its 92-unit column, 80 on RunItTwice), the
 *  opening date (11/600 Muted), "BY INVITATION" (9.5/800) on a rival cell without an account; a crest outside its window rests at
 *  0.45; the underlined one carries a 2-unit Accent line. A click opens the cell's popover, clamped inside the content rect R. */
void HubYearStrip(HubCtx& X, HubUi& U, const Rect& R)
{
	const HubFrame& Fr = X.Fr;
	const Rect& Yr = Fr.Year;
	// The label, narrowed to the room before the first cell (RunItTwice's 136 units).
	const std::string YearLabel = "THE FESTIVAL YEAR";
	const float Track = 1.6f;
	const float Room = Fr.YearX - Yr.X - 14.0f;
	float Size = 10.5f;
	while (Size > 8.0f && X.C.Measure(YearLabel, Size, 800) + Track * static_cast<float>(YearLabel.size() - 1) > Room)
	{
		Size -= 0.5f;
	}
	X.C.FillRoundRect({Yr.X, Yr.Y + 12.0f, 3.0f, 16.0f}, 1.5f, pal::Accent);
	rlnet_detail::NetSpaced(X.C, YearLabel, Yr.X + 11.0f, Yr.Y + 25.0f, Size, 800, pal::Muted, Track);
	for (int I = 0; I < 10; ++I)
	{
		const HubYearCell& Cl = X.H.Year[I];
		if (!Cl.F)
		{
			continue;
		}
		const Rect Cell{Fr.YearX + Fr.YearPitch * static_cast<float>(I), Yr.Y, Fr.YearPitch - 6.0f, Yr.H};
		const Ui::ClickState St = X.UI.Clickable("fsy" + std::to_string(I), Cell);
		if (St.Clicked && !X.Still)
		{
			// §2.8.6: {cell.X, 128, 360, 168} on RiverLine and Ignite (under the strip; RunItTwice's strip sits lower), inside R.
			HubUi::Pop P;
			P.Open = true;
			P.Fresh = true;
			P.Cell = I;
			P.R = {std::max(R.X, std::min(Cell.X, R.X + R.W - 360.0f)), Yr.Y + Yr.H + 4.0f, 360.0f, 168.0f};
			P.C = Cl;
			P.Today = X.H.Today;
			U.Popover = P;
		}
		const bool PopHere = U.Popover.Open && U.Popover.Cell == I;
		X.UI.RRect(Cell, 10.0f, PopHere ? NetA(pal::Accent, 0.18f) : St.Hover ? rlsites_detail::SitesTone(0x172a42) : rlsites_detail::SitesTone(0x101a2b),
			PopHere ? pal::Accent : NetA(pal::Line, Cl.Lit ? 0.9f : 0.5f));
		const float A0 = X.C.GetAlpha();
		X.C.SetAlpha(A0 * (Cl.Lit ? 1.0f : 0.45f));
		eventart::SeriesCrest(X.C, *Cl.Series, Cell.X + 19.0f, Cell.Y + 20.0f, 26.0f, X.Still ? 0.0 : X.Now);
		const float Tx = Cell.X + 36.0f;
		const bool IsInvite = !Cl.Account;
		X.UI.Text(NetUpper(Cl.Series->Short), Tx, Cell.Y + (IsInvite ? 14.0f : 17.0f), Ts(13.0f, 800, Cl.Lit ? pal::Ink : pal::Muted, Align::Left, Baseline::Alphabetic, false, Fr.YearText));
		X.UI.Text(net::DateLabel(Cl.F->FirstDay), Tx, Cell.Y + (IsInvite ? 26.0f : 31.0f), Ts(11.0f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, false, Fr.YearText));
		if (IsInvite)
		{
			X.UI.Text("BY INVITATION", Tx, Cell.Y + 37.0f, Ts(9.5f, 800, pal::Orange, Align::Left, Baseline::Alphabetic, false, Fr.YearText));
		}
		X.C.SetAlpha(A0);
		if (Cl.Underline)
		{
			X.C.FillRect({Cell.X + 6.0f, Cell.Y + Cell.H - 2.0f, Cell.W - 12.0f, 2.0f}, pal::Accent);
		}
	}
}

/** An outline star (the day strip's mark on a championship's other title days, §2.8.2). */
void HubStarOutline(Canvas& C, float Cx, float Cy, float R, const Color& Col)
{
	std::vector<Vec2> P;
	for (int I = 0; I < 10; ++I)
	{
		const float A = -Pi / 2.0f + static_cast<float>(I) * Pi / 5.0f;
		const float Rr = I % 2 == 0 ? R : R * 0.42f;
		P.push_back({Cx + Rr * std::cos(A), Cy + Rr * std::sin(A)});
	}
	C.StrokePolyline(P, true, Col, 1.2f);
}

/** The day strip (§2.8.2; FH-1b): one cell a day of the window (`fsd<day>`), today lit, the day shown outlined, a star on Main day and
 *  an outline star on the edition's other title days. Returns the day shown: the player's pick, else today held inside the window. */
int HubDays(HubCtx& X, HubUi& U)
{
	const HubFacts& H = X.H;
	const fest::Festival& F = *H.F;
	const Rect& D = X.Fr.Days;
	int Shown = U.Day >= F.FirstDay && U.Day <= F.LastDay ? U.Day : std::min(std::max(H.Today, F.FirstDay), F.LastDay);
	const int Count = std::max(1, H.DayCount);
	const float Gap = 4.0f;
	const float Cw = (D.W - Gap * static_cast<float>(Count - 1)) / static_cast<float>(Count);
	const int MainDay = H.HasMain ? net::DayOf(H.Main.Start) : F.MainDay;
	for (int Day = F.FirstDay; Day <= F.LastDay; ++Day)
	{
		const Rect R{D.X + static_cast<float>(Day - F.FirstDay) * (Cw + Gap), D.Y, Cw, D.H};
		const Ui::ClickState St = X.UI.Clickable("fsd" + std::to_string(Day), R);
		if (St.Clicked && !X.Still)
		{
			U.Day = Day;
			U.RowFirst = 0;
			Shown = Day;
		}
		const bool IsToday = Day == H.Today;
		const bool On = Day == Shown;
		X.UI.RRect(R, 8.0f, IsToday ? NetA(pal::Accent, 0.22f) : St.Hover ? rlsites_detail::SitesTone(0x172a42) : rlsites_detail::SitesTone(0x101a2b),
			On ? pal::Accent : IsToday ? NetA(pal::Accent, 0.6f) : NetA(pal::Line, 0.6f), On ? 1.5f : 1.0f);
		const float Mx = R.X + R.W / 2.0f;
		X.UI.Text(std::string(net::WeekdayName(Day)).substr(0, 1), Mx, R.Y + 17.0f, Ts(10.0f, 800, IsToday ? pal::Accent : pal::Muted, Align::Center));
		const std::string Date = net::DateLabel(Day);
		X.UI.Text(Date.substr(Date.find(' ') + 1), Mx, R.Y + 40.0f, Ts(17.0f, 900, Day < H.Today && !On ? pal::Dim : pal::Ink, Align::Center));
		if (Day == MainDay)
		{
			rlnet_detail::NetStar(X.C, R.X + R.W - 7.0f, R.Y + 8.0f, 4.5f, pal::Gold);
		}
		else if (std::find(H.TitleDays.begin(), H.TitleDays.end(), Day) != H.TitleDays.end())
		{
			HubStarOutline(X.C, R.X + R.W - 7.0f, R.Y + 8.0f, 4.5f, pal::Gold);
		}
	}
	return Shown;
}

/** The edition's satellites and the ladder up to them (§2.2.3: RCOP's step1-4 and mega-rcop): F.Satellites, then every satellite
 *  whose seat is a ticket of one already in the set, recurring or a one-off inside the edition's window (FestCalRow's rule). Built
 *  once an edition (HubUi::Ladder). */
std::vector<std::string> HubLadder(const net::Network& Net, const fest::Festival& F)
{
	std::vector<std::string> Out = F.Satellites;
	std::vector<std::string> Tickets;
	for (const std::string& Id : Out)
	{
		Tickets.push_back(Net.TicketOf(Id));
	}
	for (int Pass = 0; Pass < 6; ++Pass)
	{
		bool Grew = false;
		for (const net::EventTemplate& T : Net.Templates())
		{
			if (T.Fmt != net::Format::Satellite || T.SeatTicket.empty() || std::find(Out.begin(), Out.end(), T.Id) != Out.end())
			{
				continue;
			}
			if (T.OnlyDay != -9999 && (T.OnlyDay < F.FirstDay - fest::kAnnounceDays || T.OnlyDay > F.MainDay))
			{
				continue;
			}
			if (std::find(Tickets.begin(), Tickets.end(), T.SeatTicket) != Tickets.end())
			{
				Out.push_back(T.Id);
				Tickets.push_back(Net.TicketOf(T.Id));
				Grew = true;
			}
		}
		if (!Grew)
		{
			break;
		}
	}
	return Out;
}

/** The day's rows (§2.8.2): the edition's events that day in start order, then one row for each satellite into its Main running that
 *  day (its first instance not yet finished, else the day's last), rebuilt once a world minute or on a new day. */
void HubDayRows(const net::Network& Net, const fest::Festival& F, HubUi& U, int Day, double World)
{
	const long long Minute = static_cast<long long>(std::floor(World));
	if (U.RowsDay == Day && U.RowsMinute == Minute)
	{
		return;
	}
	if (!U.LadderBuilt)
	{
		U.Ladder = HubLadder(Net, F);
		U.LadderBuilt = true;
	}
	U.RowsDay = Day;
	U.RowsMinute = Minute;
	U.Rows.clear();
	std::vector<net::EventInstance> Sats;
	for (net::EventInstance& E : Net.Window(HubMinute(Day), HubMinute(Day + 1)))
	{
		const net::EventTemplate& T = Net.TemplateOf(E);
		if (T.Series == F.SeriesId)
		{
			U.Rows.push_back(std::move(E));
			continue;
		}
		if (T.Fmt != net::Format::Satellite || std::find(U.Ladder.begin(), U.Ladder.end(), T.Id) == U.Ladder.end())
		{
			continue;
		}
		const auto Same = std::find_if(Sats.begin(), Sats.end(), [&E](const net::EventInstance& O) { return O.Template == E.Template; });
		if (Same == Sats.end())
		{
			Sats.push_back(std::move(E));
		}
		else if (Net.Live(*Same, World).St == net::Status::Finished)
		{
			*Same = std::move(E);
		}
	}
	for (net::EventInstance& E : Sats)
	{
		U.Rows.push_back(std::move(E));
	}
}

/** A satellite's seat followed up its ladder to the Main it ends in: that Main's lock line while it isn't joinable (§2.2.3), else "". */
std::string HubSeatLock(const net::Network& Net, const net::EventTemplate& T, double From, int Unlocks)
{
	const net::EventTemplate* At = &T;
	for (int K = 0; K < 8 && At->Fmt == net::Format::Satellite && !At->SeatTicket.empty(); ++K)
	{
		const net::EventTemplate* Up = Net.FindTemplate(At->SeatTicket);
		if (!Up)
		{
			return std::string();
		}
		At = Up;
	}
	net::EventInstance Target;
	std::string SeatLock;
	if (At == &T || At->Fmt == net::Format::Satellite || !Net.Next(At->Id, From, Target) || Net.Joinable(Target, &SeatLock, Unlocks))
	{
		return std::string();
	}
	return SeatLock;
}

/** The day's rows (§2.8.2; FH-1b): seven at 72 in view (the wheel scrolls the rest; Ignite's seventh peeks under its frame), each
 *  `ev:<id>` (a click opens the lobby's event panel on it), written to Listed: the start, the low-detail emblem at 30, the number and
 *  the name, GTD or pool and the buy-in, the pills (MAIN EVENT, a title's MINI or MICRO MAIN EVENT, SATELLITE), the status and the
 *  entries; a satellite into a Main that isn't joinable shows the Main's lock line and U31 instead of its money line. The button is
 *  RegisterBlock's (SitesTwoPress, the clause's second press, core's strike gate: FestStrikeGate): on every unfinished row until one
 *  is armed, then on that row alone, which a press on its button selects first (Session::ConfirmRegister is one per session); a
 *  finished row shows its winner in the button's place. */
void HubRows(HubCtx& X, HubUi& U, int Day)
{
	const HubFacts& H = X.H;
	const fest::Festival& F = *H.F;
	const net::Network& Net = net::Site(F.Site);
	const Session& S = Access::SessionOf(X.RL);
	const Rect& Rr = X.Fr.Rows;
	HubDayRows(Net, F, U, Day, H.World);
	constexpr float Pitch = 72.0f;
	constexpr float RowH = 66.0f;
	const int Visible = std::max(1, static_cast<int>(Rr.H / Pitch));
	const int Count = static_cast<int>(U.Rows.size());
	if (!X.Still && X.UI.Hover(Rr) && X.UI.Ptr.Wheel != 0.0f)
	{
		U.RowFirst += X.UI.Ptr.Wheel > 0.0f ? 1 : -1;
	}
	U.RowFirst = std::max(0, std::min(U.RowFirst, Count - Visible));
	if (!X.Still)
	{
		std::vector<std::string> Ids;
		for (const net::EventInstance& E : U.Rows)
		{
			Ids.push_back(E.Id);
		}
		Access::SetListed(X.RL, std::move(Ids));
	}
	const bool Armed = S.ConfirmRegister;
	const Color SoftCol = rlsites_detail::SitesTone(0xc3cedf);
	const Color EdgeCol = Hex(H.Series->Color);
	X.C.PushClip(Rr);
	for (int I = U.RowFirst; I < Count && Pitch * static_cast<float>(I - U.RowFirst) < Rr.H; ++I)
	{
		const net::EventInstance& E = U.Rows[static_cast<size_t>(I)];
		const net::EventTemplate& T = Net.TemplateOf(E);
		const net::LiveState L = Net.Live(E, H.World);
		const Rect R{Rr.X, Rr.Y + Pitch * static_cast<float>(I - U.RowFirst), Rr.W, RowH};
		const Rect Rb{R.X + R.W - 372.0f, R.Y + 10.0f, 360.0f, 46.0f};
		const bool IsMine = X.RL.SelectedEvent() == E.Id;
		const bool OverButton = X.UI.Hover(Rb);
		const Ui::ClickState St = X.UI.Clickable("ev:" + E.Id, R, !OverButton);
		if (St.Clicked && !X.Still)
		{
			Access::SelectEvent(X.RL, E.Id);
			Access::OpenPage(X.RL, RiverLine::Page::Lobby, X.Now);
		}
		const auto TitleIt = std::find(F.TitleIds.begin(), F.TitleIds.end(), T.Id);
		const bool IsMain = T.Id == F.MainId;
		const bool TitleEvent = !IsMain && TitleIt != F.TitleIds.end();
		const bool IsSat = T.Fmt == net::Format::Satellite;
		X.UI.RRect(R, 10.0f, St.Hover ? rlsites_detail::SitesTone(0x15253d) : rlsites_detail::SitesTone(0x101a2b),
			IsMain ? NetA(pal::Gold, 0.55f) : IsMine ? NetA(pal::Accent, 0.6f) : NetA(pal::Line, 0.6f));
		X.C.FillRoundRect({R.X + 1.0f, R.Y + 10.0f, 4.0f, R.H - 20.0f}, 2.0f, IsMain || TitleEvent ? pal::Gold : EdgeCol);
		X.UI.Text(net::TimeLabel(E.Start), R.X + 18.0f, R.Y + 28.0f, Ts(14.0f, 700, pal::Muted));
		eventart::Emblem(X.C, T, R.X + 104.0f, R.Y + 33.0f, 30.0f, X.Still ? 0.0 : X.Now);
		const float StatusX = Rb.X - 138.0f;
		float Nx = R.X + 128.0f;
		if (T.EventNo > 0)
		{
			Nx += rlnet_detail::NetPill(X.C, "#" + std::to_string(T.EventNo), Nx, R.Y + 14.0f, IsMain || TitleEvent ? pal::Gold : EdgeCol, true, 10.0f) + 8.0f;
		}
		X.UI.Text(rlnet_detail::NetShortName(T.Name), Nx, R.Y + 28.0f, Ts(15.0f, 700, pal::Ink, Align::Left, Baseline::Alphabetic, false, std::max(40.0f, StatusX - 12.0f - Nx)));
		const std::string SeatLock = IsSat ? HubSeatLock(Net, T, E.Start, S.Unlocks()) : std::string();
		float Lx = R.X + 128.0f;
		if (!SeatLock.empty())
		{
			// §2.2.3: the button stays, and the row says the truth beside it.
			rlnet_detail::NetLockIcon(X.C, Lx, R.Y + 36.0f, 11.0f, pal::Orange);
			X.UI.Text(SeatLock, Lx + 16.0f, R.Y + 46.0f, Ts(12.0f, 600, pal::Orange, Align::Left, Baseline::Alphabetic, false, StatusX - 28.0f - Lx));
			X.UI.Text("A seat won here waits in your wallet until big fields open.", Lx, R.Y + 61.0f, Ts(11.0f, 500, pal::Muted, Align::Left, Baseline::Alphabetic, false, StatusX - 12.0f - Lx));
		}
		else
		{
			std::string PotLine;
			if (T.GtdCents > 0)
			{
				PotLine = NetMoney(std::max(L.Pool, T.GtdCents)) + (L.St == net::Status::Finished ? "" : " GTD");
			}
			else if (L.Pool > 0)
			{
				PotLine = NetMoney(L.Pool);
			}
			if (!PotLine.empty())
			{
				Lx += X.UI.Text(PotLine, Lx, R.Y + 51.0f, Ts(13.0f, 700, SoftCol)) + 14.0f;
			}
			if (T.BuyInCents > 0)
			{
				Lx += X.UI.Text(net::BuyIn(T.BuyInCents), Lx, R.Y + 51.0f, Ts(13.0f, 800, pal::Gold)) + 14.0f;
			}
			const bool IsMini = T.Id.size() >= 4 && T.Id.compare(T.Id.size() - 4, 4, "mini") == 0;
			const std::string PillText = IsMain ? std::string("MAIN EVENT")
				: TitleEvent                     ? std::string(IsMini ? "MINI MAIN EVENT" : "MICRO MAIN EVENT")
				: IsSat                          ? std::string("SATELLITE")
												 : std::string();
			if (!PillText.empty() && Lx < StatusX - 40.0f)
			{
				rlnet_detail::NetPill(X.C, PillText, Lx, R.Y + 39.0f, IsMain || TitleEvent ? pal::Gold : pal::Accent2, false, 9.5f);
			}
		}
		// The status (the series page's own forms) and the entries.
		const std::string StatusLine = L.St == net::Status::LateReg ? "LATE " + rlnet_detail::NetHms(L.LateRegLeft)
			: L.St == net::Status::Registering || L.St == net::Status::Announced ? "IN " + NetUpper(net::Countdown(L.StartsIn))
																				   : NetUpper(net::StatusName(L.St));
		rlnet_detail::NetPill(X.C, StatusLine, StatusX, R.Y + 14.0f, rlnet_detail::NetStatus(L.St), false, 10.0f);
		const int Entries = L.St == net::Status::Finished ? E.Entries : L.Entries;
		if (Entries > 0)
		{
			X.UI.Text(Entries == 1 ? std::string("1 entry") : Grouped(Entries) + " entries", StatusX, R.Y + 51.0f, Ts(12.0f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, false, 128.0f));
		}
		if (L.St == net::Status::Finished)
		{
			const net::EventResult& Res = Net.Result(E);
			if (!Res.FinalTable.empty())
			{
				const net::Placing& W = Res.FinalTable.front();
				rlnet_detail::NetTrophy(X.C, Rb.X + 18.0f, Rb.Y + 9.0f, 26.0f, Hex(0xffe08a), Hex(0xc9962b));
				X.UI.Text(Access::PlacingName(X.RL, W), Rb.X + 40.0f, Rb.Y + 20.0f, Ts(14.0f, 700, W.Player == -1 ? pal::Accent : pal::Ink, Align::Left, Baseline::Alphabetic, false, Rb.W - 44.0f));
				X.UI.Text(Money(W.Prize), Rb.X + 40.0f, Rb.Y + 38.0f, Ts(12.0f, 700, pal::Gold, Align::Left, Baseline::Alphabetic, true));
			}
		}
		else if (!Armed || IsMine)
		{
			// The press that arms the step first selects this row, so the one confirm step is this row's (a two-frame click too).
			if (!Armed && !IsMine && !X.Still && OverButton && (X.UI.Ptr.Pressed || X.UI.Ptr.Released))
			{
				Access::SelectEvent(X.RL, E.Id);
			}
			Access::RegisterBlock(X.RL, E, L, Rb, X.Now);
		}
	}
	X.C.PopClip();
}

/** The overlay ticker's run (§2.4.4, U26; FH-1b): for each of the edition's guaranteed headliners (its featured events with a
 *  guarantee, the Main among them) from yesterday to tomorrow, and for the Main while its registration runs: the overlay its site added
 *  once registration closed, else the projection short or over while registering with someone in (site::show::OverlayOf: the
 *  guarantee against the pool, so added money is never an overlay); then the Main's registrations so far. "" when there's nothing. */
std::string HubTickerRun(const net::Network& Net, const HubFacts& H)
{
	const fest::Festival& F = *H.F;
	std::vector<std::string> TickItems;
	std::vector<std::string> TickSeen;
	const auto TickAdd = [&](const net::EventInstance& E) {
		const net::EventTemplate& T = Net.TemplateOf(E);
		if (T.GtdCents <= 0 || std::find(TickSeen.begin(), TickSeen.end(), E.Id) != TickSeen.end())
		{
			return;
		}
		TickSeen.push_back(E.Id);
		const ::ss::site::show::OverlayView V = ::ss::site::show::OverlayOf(Net, E, H.World);
		if (!V.Shown)
		{
			return;
		}
		const std::string Name = rlnet_detail::NetShortName(T.Name);
		if (V.Actual)
		{
			if (V.Cents > 0)
			{
				TickItems.push_back(rlsites_detail::SitesShortName(F.Site) + " added " + NetMoney(V.Cents) + " to cover " + Name + "'s guarantee.");
			}
			return;
		}
		if (V.Cents > 0)
		{
			TickItems.push_back(Name + " is projected " + NetMoney(V.Cents) + " short.");
			return;
		}
		const double Each = static_cast<double>(T.BuyInCents) * Net.Definition().Rake.PrizeShare(T);
		const Chips Over = static_cast<Chips>(std::llround(V.Entries * Each)) - T.GtdCents;
		if (Over > 0)
		{
			TickItems.push_back(Name + " is projected " + NetMoney(Over) + " over.");
		}
	};
	const int From = std::max(F.FirstDay, H.Today - 1);
	const int To = std::min(F.LastDay, H.Today + 1);
	if (From <= To)
	{
		for (const net::EventInstance& E : Net.Window(HubMinute(From), HubMinute(To + 1)))
		{
			const net::EventTemplate& T = Net.TemplateOf(E);
			if (T.Series == F.SeriesId && T.Featured)
			{
				TickAdd(E);
			}
		}
	}
	if (H.HasMain)
	{
		TickAdd(H.Main);
		const ::ss::site::show::OverlayView V = ::ss::site::show::OverlayOf(Net, H.Main, H.World);
		if (!V.Actual && V.Registered > 0)
		{
			TickItems.push_back("MAIN EVENT \xC2\xB7 " + Grouped(V.Registered) + " registered");
		}
	}
	std::string TickRun;
	for (const std::string& It : TickItems)
	{
		TickRun += (TickRun.empty() ? "" : "   \xE2\x80\x94   ") + It;
	}
	return TickRun;
}

/** The overlay ticker (§2.8.2; FH-1b): one clipped run in its rect, rebuilt once a world minute; it slides when wider than its lane
 *  (a still in the preview). Ignite's hub has none (L7). */
void HubTicker(HubCtx& X, HubUi& U)
{
	if (!X.Fr.HasTicker)
	{
		return;
	}
	const long long Minute = static_cast<long long>(std::floor(X.H.World));
	if (U.TickerMinute != Minute)
	{
		U.Ticker = HubTickerRun(net::Site(X.H.F->Site), X.H);
		U.TickerMinute = Minute;
	}
	if (U.Ticker.empty())
	{
		return;
	}
	const Rect& T = X.Fr.Ticker;
	X.UI.RRect(T, 8.0f, rlsites_detail::SitesTone(0x0b1220), NetA(pal::Line, 0.7f));
	X.C.FillRoundRect({T.X + 10.0f, T.Y + 8.0f, 4.0f, T.H - 16.0f}, 2.0f, pal::Accent);
	const Rect Lane{T.X + 22.0f, T.Y, T.W - 30.0f, T.H};
	const float W = X.UI.Measure(U.Ticker, 13.0f, 600);
	const float Span = W + 80.0f;
	const bool Slide = W > Lane.W && !X.Still;
	const float Tx = Slide ? Lane.X - static_cast<float>(std::fmod(X.SinceOpen * 40.0, static_cast<double>(Span))) : Lane.X;
	const TextStyle Style = Ts(13.0f, 600, rlsites_detail::SitesTone(0xb9c4d6), Align::Left, Baseline::Middle);
	X.C.PushClip(Lane);
	X.UI.Text(U.Ticker, Tx, T.Y + T.H / 2.0f + 1.0f, Style);
	if (Slide && Tx + Span < Lane.X + Lane.W)
	{
		X.UI.Text(U.Ticker, Tx + Span, T.Y + T.H / 2.0f + 1.0f, Style);
	}
	X.C.PopClip();
}

/** The right column's tabs (`fstab0..2`, U5): THE RACE · CHAMPIONS · YOUR <FESTIVAL>, one shown at a time. Returns the tab shown. */
int HubTabs(HubCtx& X, HubUi& U)
{
	const Rect& T = X.Fr.Tabs;
	const std::string Names[3] = {"THE RACE", "CHAMPIONS", "YOUR " + NetUpper(X.H.Series->Short)};
	const int Shown = U.Tab >= 0 ? U.Tab : HubAutoTab(X.H);
	const float Gap = 6.0f;
	const float W = (T.W - 2.0f * Gap) / 3.0f;
	for (int I = 0; I < 3; ++I)
	{
		const Rect R{T.X + static_cast<float>(I) * (W + Gap), T.Y, W, T.H};
		const Ui::ClickState St = X.UI.Clickable("fstab" + std::to_string(I), R);
		if (St.Clicked && !X.Still)
		{
			U.Tab = I;
		}
		const bool On = I == Shown;
		X.UI.RRect(R, 10.0f, On ? NetA(pal::Accent, 0.2f) : St.Hover ? rlsites_detail::SitesTone(0x172a42) : rlsites_detail::SitesTone(0x101a2b),
			On ? pal::Accent : NetA(pal::Line, 0.9f));
		X.UI.Text(Names[I], R.X + R.W / 2.0f, R.Y + R.H / 2.0f + 0.5f, Ts(12.0f, 800, On ? pal::Ink : pal::Muted, Align::Center, Baseline::Middle, false, R.W - 16.0f));
	}
	return Shown;
}

/** The right column's body for the tab shown, in its empty states (§2.8.2): THE RACE's lines (U6), CHAMPIONS' two honest lines
 *  while the roll is empty (U9), YOUR <FESTIVAL>'s invitation before the hero's first event of the edition (U11). The race's rows,
 *  the roll's rows and the YOUR panel are FH-4A's. */
void HubBody(HubCtx& X, int Tab)
{
	const HubFacts& H = X.H;
	const Rect& B = X.Fr.Body;
	X.UI.RRect(B, 14.0f, rlsites_detail::SitesTone(0x0e1726), NetA(pal::Line, 0.9f));
	const float Px = B.X + 22.0f;
	const float MaxW = B.W - 44.0f;
	const Color Soft = rlsites_detail::SitesTone(0xc3cedf);
	float Y = B.Y + 34.0f;
	switch (Tab)
	{
	case 0:
	{
		X.UI.Text("PLAYER OF THE SERIES", Px, Y, Ts(12.0f, 800, pal::Muted));
		Y += 32.0f;
		const bool Ahead = H.Ph == fest::Phase::Off || H.Ph == fest::Phase::Announced || H.Ph == fest::Phase::Buildup;
		if (H.RaceOpen || Ahead)
		{
			if (Ahead)
			{
				// Before the race opens on the edition's first day: what it is, then its empty state.
				Y = rlnet_detail::NetParagraph(X.C, "Every cash scores, yours and every regular's. First place takes the belt.", Px, Y, MaxW, 15.0f, 500, Soft, 22.0f) + 10.0f;
			}
			if (H.RaceRows == 0)
			{
				X.UI.Text("No points yet.", Px, Y, Ts(15.0f, 600, pal::Ink));
			}
		}
		else
		{
			// §2.5.5: the edition is on stage and the world holds no race for it (a save from before the update).
			rlnet_detail::NetParagraph(X.C, "This edition's race began before your update: no Player of the Series this time.", Px, Y, MaxW, 15.0f, 500, Soft, 22.0f);
		}
		break;
	}
	case 1:
		if (H.RollEmpty)
		{
			X.UI.Text("No champion yet. The first name goes up on " + fest::DateInSentence(H.F->MainDay, H.Today) + ".", Px, Y, Ts(15.0f, 600, pal::Ink, Align::Left, Baseline::Alphabetic, false, MaxW));
			// The roll's books opened on Night One in every world (no festival edition was held before it, fest::All()).
			X.UI.Text("The record books opened " + net::DateLabel(net::NightOneDay) + ", " + std::to_string(world::YearOf(net::NightOneDay)) + ".", Px, Y + 26.0f,
				Ts(13.0f, 500, pal::Muted, Align::Left, Baseline::Alphabetic, false, MaxW));
		}
		break;
	default:
		if (!H.HasPin)
		{
			rlnet_detail::NetParagraph(X.C, "Play any " + fest::NameAfterArticle(H.F->Name) + " event for your pin.", Px, Y, MaxW, 15.0f, 500, Soft, 22.0f);
		}
		break;
	}
}

/** The popover's phase line (§2.8.6, U16): "Opens {Date}." before the window, "Day {N} of {Days}." on stage, "Crowned {Date}." in
 *  the Afterglow (the crown's day, else the edition's last finish with no world). */
std::string HubPhaseLine(const HubYearCell& Cl, int Today)
{
	const fest::Festival& F = *Cl.F;
	switch (Cl.Ph)
	{
	case fest::Phase::Live:
	case fest::Phase::MainDay:
	case fest::Phase::Closing:
	{
		const int Days = F.LastDay - F.FirstDay + 1;
		return "Day " + std::to_string(std::max(1, std::min(Days, Today - F.FirstDay + 1))) + " of " + std::to_string(Days) + ".";
	}
	case fest::Phase::Afterglow:
		return "Crowned " + fest::DateInSentence(net::DayOf(Cl.CrownedAt >= 0.0 ? Cl.CrownedAt : fest::LastEnd(F)), Today) + ".";
	default: break;
	}
	return "Opens " + fest::DateInSentence(F.FirstDay, Today) + ".";
}

/** The strip's popover (§2.8.6; FH-1b), drawn by FestivalOverlays over the hub that opened it, in the client's skin: the crest at 46,
 *  the name and its full title (12/600 Muted, N17-N19), the host and the window, the phase line, "Your account is open." or "By
 *  invitation." (U16; FH-2 adds the Pass's count, U30), and one button (U17): OPEN THE HUB when the edition's host is the site in front
 *  (the edition's hub), GO TO {SITE} for another site's edition with an account open (the site tabs' path: RiverLine::SiteShown, then
 *  that site's SERIES page on the edition); none for a rival festival without an account. The hub held the pointer under it
 *  (FestivalHub); the frame it opens in takes no input. */
void HubPopover(RiverLine& RL, Canvas& C, Ui& UI, HubUi& U, double Now)
{
	HubUi::Pop& P = U.Popover;
	const HubYearCell Cl = P.C;
	if (!Cl.F || !Cl.Series)
	{
		P = HubUi::Pop();
		return;
	}
	const Session& S = Access::SessionOf(RL);
	const net::SiteId Front = rlsites_detail::SitesClient(RL, S);
	const SkinScope Worn(rlsites_detail::SitesSkin(Front, Access::WorldOf(RL)));
	const Rect R = P.R;
	const bool TakesInput = !P.Fresh;
	P.Fresh = false;
	UI.RRect(R, 14.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, rlsites_detail::SitesTone(0x131c2b), rlsites_detail::SitesTone(0x0b1220)), NetA(pal::Accent, 0.6f));
	eventart::SeriesCrest(C, *Cl.Series, R.X + 40.0f, R.Y + 42.0f, 46.0f, 0.0);
	const float Tx = R.X + 76.0f;
	const float Tw = R.W - 92.0f;
	UI.Text(Cl.F->Name, Tx, R.Y + 36.0f, Ts(18.0f, 800, pal::Ink, Align::Left, Baseline::Alphabetic, false, Tw));
	float Ly = R.Y + 54.0f;
	if (!Cl.Series->Title.empty())
	{
		UI.Text(Cl.Series->Title, Tx, Ly, Ts(12.0f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, false, Tw));
		Ly += 16.0f;
	}
	UI.Text(rlsites_detail::SitesShortName(Cl.F->Site) + " \xC2\xB7 " + net::DateLabel(Cl.F->FirstDay) + " \xE2\x80\x93 " + net::DateLabel(Cl.F->LastDay), Tx, Ly,
		Ts(12.0f, 600, rlsites_detail::SitesTone(0xc3cedf), Align::Left, Baseline::Alphabetic, false, Tw));
	UI.Text(HubPhaseLine(Cl, P.Today), R.X + 18.0f, R.Y + 102.0f, Ts(14.0f, 700, pal::Ink, Align::Left, Baseline::Alphabetic, false, R.W - 36.0f));
	UI.Text(Cl.Account ? "Your account is open." : "By invitation.", R.X + 18.0f, R.Y + 122.0f, Ts(13.0f, 500, Cl.Account ? pal::Green : pal::Muted, Align::Left, Baseline::Alphabetic, false, 160.0f));
	if (!Cl.Account)
	{
		return;
	}
	const bool Here = Cl.F->Site == Front;
	ButtonOpts O;
	O.Size = 13.0f;
	const Rect B{R.X + R.W - 172.0f, R.Y + R.H - 44.0f, 156.0f, 32.0f};
	if (rlsites_detail::SitesButton(UI, C, "fspopgo", B, Here ? std::string("OPEN THE HUB") : "GO TO " + rlsites_detail::SitesUpperName(Cl.F->Site), O, rlsites_detail::SitesLook(Front)) &&
		TakesInput)
	{
		if (!Here)
		{
			RL.SiteShown = Cl.F->Site;
			Access::OpenPage(RL, RiverLine::Page::Series, Now);
		}
		RL.ShowSeries(Cl.F->SeriesId, Now);
		P = HubUi::Pop();
	}
}
} // namespace rlfest_detail

void RiverLine::FestivalHub(const Rect& R, const fest::Festival& F, double Now)
{
	// G1 FH-1a's skeleton and FH-1b's strip, day strip, rows and ticker (the file's head says what each package adds). R is the
	// host's content rect (FH-1c's NetHubRect): every region of HubFrameOf(F.Site) lies inside it, and nothing draws past it.
	const rlfest_detail::HubFacts H = rlfest_detail::Gather(S, F, World);
	if (!H.Series)
	{
		return;
	}
	rlfest_detail::HubUi& U = rlfest_detail::HubOf(*this, F, Now);
	rlfest_detail::HubCtx X{*this, *C, UI, H, rlfest_detail::HubFrameOf(F.Site), festart::LookOf(F.Which), Now, Now - U.OpenedAt, Previewing};
	// FH-1b, house law 3: an open popover holds the pointer for the whole hub (modal: a click on it reaches nothing beneath, a click
	// anywhere else closes it and is swallowed); FestivalOverlays draws it once the hub has drawn, with the pointer handed back.
	const bool PopUp = U.Popover.Open && !X.Still;
	rlsites_detail::SitesBlocked Held;
	if (PopUp)
	{
		Held = rlsites_detail::SitesBlockUnder(UI, U.Popover.R, true);
	}
	C->PushClip(R);
	rlfest_detail::HubYearStrip(X, U, R);
	rlfest_detail::HubBanner(X);
	rlfest_detail::HubMainCard(X);
	rlfest_detail::HubRows(X, U, rlfest_detail::HubDays(X, U));
	rlfest_detail::HubTicker(X, U);
	rlfest_detail::HubBody(X, rlfest_detail::HubTabs(X, U));
	C->PopClip();
	if (PopUp)
	{
		rlsites_detail::SitesUnblock(UI, Held);
		if (Held.Scrim)
		{
			U.Popover = rlfest_detail::HubUi::Pop();
		}
	}
	if (!X.Still)
	{
		U.Drew = true;
	}
}

void RiverLine::FestivalOverlays(double Now)
{
	// G1 FH-1b: the strip's popover, over the hub that drew in this frame (another page, app or the stream preview leaves it
	// unseen; a new page or series closes it, HubOf). The moments (Kinds 1-5, the mark card) are FH-6a's.
	const auto It = rlfest_detail::HubStates().find(this);
	if (It == rlfest_detail::HubStates().end())
	{
		return;
	}
	rlfest_detail::HubUi& U = It->second;
	const bool Drew = U.Drew;
	U.Drew = false;
	if (Drew && U.Popover.Open)
	{
		rlfest_detail::HubPopover(*this, *C, UI, U, Now);
	}
}

void RiverLine::CareerPins(const Rect& /*R*/, double /*Now*/)
{
}
} // namespace ui
} // namespace ss
