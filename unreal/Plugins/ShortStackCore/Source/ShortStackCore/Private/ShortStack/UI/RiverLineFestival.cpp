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
#include "ShortStack/UI/RiverLine.h"

#include "RiverLineShared.h"
#include "ShortStack/Game/Festival.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
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
};

/** The world minute a day begins. */
double HubMinute(int Day)
{
	return static_cast<double>(Day) * net::MinutesPerDay;
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
} // namespace rlfest_detail

void RiverLine::FestivalHub(const Rect& R, const fest::Festival& F, double Now)
{
	// G1 FH-1a: the skeleton (the file's head says what each package adds). R is the host's content rect (FH-1c's NetHubRect): every
	// region of HubFrameOf(F.Site) lies inside it, and nothing draws past it.
	const rlfest_detail::HubFacts H = rlfest_detail::Gather(S, F, World);
	if (!H.Series)
	{
		return;
	}
	rlfest_detail::HubUi& U = rlfest_detail::HubOf(*this, F, Now);
	rlfest_detail::HubCtx X{*this, *C, UI, H, rlfest_detail::HubFrameOf(F.Site), festart::LookOf(F.Which), Now, Now - U.OpenedAt, Previewing};
	C->PushClip(R);
	rlfest_detail::HubBanner(X);
	rlfest_detail::HubMainCard(X);
	rlfest_detail::HubBody(X, rlfest_detail::HubTabs(X, U));
	C->PopClip();
}

void RiverLine::FestivalOverlays(double /*Now*/)
{
	// Phase A: no festival exists (fest::All() is empty), so no moment waits (Session::PeekFestMoment) and no popover is open. G1: the
	// moments are FH-6a's and the popover FH-1b's.
}

void RiverLine::CareerPins(const Rect& /*R*/, double /*Now*/)
{
}
} // namespace ui
} // namespace ss
