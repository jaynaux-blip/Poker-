// u4_ui: the festival half's host pages (update 4 SPEC v1.7 §2.15.2; F-hubs' stream B, LEAD_CALLS R-1): the hub's routing on the
// three hosts, the takeover's words on Ignite and RunItTwice, the register sheets' YOUR strip, slide 0, the tickers, the card's pill
// and the trophy case (UI/RiverLineNet.cpp, UI/RiverLineIgnite.cpp, UI/RiverLineRit.cpp, UI/RiverLineCard.cpp, UI/SecondScreen.cpp).
// Called from u4_ui_main.cpp (the kit), after SegFest.
// G1 (the lead's pre-wave commit H0, PLAN_G1 §4.2): registered, empty.
// G1 FH-1c: FestHostsRouting, the hub's routing (§2.8.1: rlnet_detail::NetHubPick, NetHubEdition and NetHubRect in
// UI/RiverLineNet.cpp). F-net's calendar (fest::All()) is empty before FN-1, so the choice is checked on constructed editions and the
// real path on every series the three hosts list; FestHubBudgets (stream A, FH-7) later draws every state on every host.
// G1 FH-3b: FestBandstandPick (§2.7.4's bandstand pick, rlignite_detail::IgniteBandstandFest, on F-net's real calendar and drawn on
// Ignite's PLAY across FEIGN 2026's Buildup and first day, with the Lighthouse, chip 10, the floor's shelf and the Prize Counter's
// hand-over) and FestCopyUi's B half (stream B's strings, rlfest_detail::HostUiStrings: their kinds, the hub's lint after a sample fill,
// U15's figures from the edition with no digit typed, U22's words through SecondScreen's Word()); the chip and label's cost printed
// (asserted at FH-7).
// G1 FH-5a: FestCopyUi's N-2 half (RunItTwice keeps a festival event's code on THE BOARD, the Prospectus and the table header, §2.3.2:
// WRIT 2027's day 2 and Main day, GRIT 2027's 02-E and 02-L, a REIGN 04 table), FestYourStrip's N-3 half (the Main card's Details holds the Prospectus on
// REIGN 2027's Main from its day 2, §2.8.2) and FestReignCopy's THE BOARD's tag half (no sponsor mark by the REIGN tag, §2.15.1).
// G1 FH-3a: FestHonestLines' B half (RiverLine's honest copy, UI/RiverLineNet.cpp: slide 0's computed guarantee, the wire's festival
// item from the Buildup, the boards' U25 and THE BELT, no Season prize, the Career goals' U24, the hub's RCOP Buildup panel's U23 through
// rlfest_detail::RcopBuildupPanel; no removed claim in its claim form) with the lobby's cost printed; FestCopyUi's B half gains U23-U25.
// G1 FH-4B: FestYourStrip's sheets half (§2.8.2: YOUR {FESTIVAL}'s two-line strip on RiverLine's event panel, Ignite's event sheet and
// RunItTwice's Prospectus once the hero has played the edition, none before and none on a plain series event; LEAD_CALLS R-18: U37 on
// a step event's sheet, Ignite's wallet ticket row and RunItTwice's step tickets apart from its WELCOME TICKETS); FestCopyUi's B half
// gains the strip's, U37's and the ticket rows' strings.
// G1 FH-6b: FestHonestLines' true-defender half (PLAN_FHUBS Q16: DEFENDING CHAMPION on the card and on the lobby's names, and the card's
// honorific line, only for a true defender, from the roll and the calendar; rlnet_detail::NetFestDefends, NetFestHonorific), the pin
// wall's band on the Career page (RiverLine Rewards keeps it while the wall is hidden) and the trophy case's festival pieces (U20,
// rlnet_detail::NetFestCase) with the Career page's cost printed (asserted at FH-7); FestCopyUi's B half gains U20's and U21's strings.
// G1 FH-5c: FestAfterglowHosts (§2.1.4's Afterglow on the hosts' front pages, U15, U16; §2.4.4's overlay watch on RiverLine's wire,
// U26; PLAN_FHUBS Q23), on FestHonestLines' career: the wire's overlay items through RCOP 2026's days against the test's own reading of
// site::show::OverlayOf, drawn on the lobby; RCOP 2026's Afterglow on slide 0 and the wire (the champion from the world's roll, nothing
// before the crown or after the three days); RunItTwice's first edition's Afterglow on the tape; FestCopyUi's B half gains their strings.
// G1 polish (g1p, key ignite; PIE_G1_NOTES' cosmetic list): FestTitleFits (the bandstand's two-line title for every Ignite festival
// event of 2026-2028, measured in its boxes at both scales; FestBandstandPick reads the drawn lines) and FestSignFits (the Lighthouse's
// Afterglow sign, "{SHORT} CHAMPION" over the champion's whole name, for every Ignite brand and the world's longest names).
#include "UiSeg.h"

#include "CopyLint.h"
#include "Trademarks.h"
#include "ShortStack/Game/Festival.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Handles.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/SiteShow.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/WorldFest.h"
#include "ShortStack/Rng.h"
#include "ShortStack/UI/EventArt.h"
#include "ShortStack/UI/RiverLine.h"
#include "../../Source/ShortStackCore/Private/ShortStack/Game/SitesDetail.h"
#include "../../Source/ShortStackCore/Private/ShortStack/Game/WorldFestDetail.h"
#include "../../Source/ShortStackCore/Private/ShortStack/UI/IgniteArt.h"
#include "../../Source/ShortStackCore/Private/ShortStack/UI/RitArt.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace ui
{
namespace rlnet_detail
{
// UI/RiverLineNet.cpp (FH-1c): the festival hub's routing.
Rect NetHubRect(net::SiteId Site);
const fest::Festival* NetHubPick(net::SiteId Site, bool HasPick, const fest::Festival* Picked, const std::vector<std::pair<const fest::Festival*, fest::Phase>>& Near);
const fest::Festival* NetHubEdition(net::SiteId Site, const std::string& SeriesId, double World);
// UI/RiverLineNet.cpp (FH-3a): RiverLine's festival on the taskbar's wire.
std::string NetTickerFest(double World, std::string& Series);
const net::SeriesInfo* NetWireSeries(const net::Network& Net, double World, const std::string& FestSeries);
// UI/RiverLineNet.cpp (FH-4B): the edition a register sheet belongs to, and a wallet ticket as a festival step ticket.
const fest::Festival* NetSheetEdition(const net::Network& Net, const net::EventInstance& E);
int NetStepTicket(net::SiteId Site, const std::string& Ticket, double World, const fest::Festival*& Edition, std::string& GoodOn);
// UI/RiverLineNet.cpp (FH-6b): the edition a person defends, the card's honorific line and a trophy case's pieces.
const fest::Festival* NetFestDefends(const world::World& W, int Npc, double WorldMin);
std::string NetFestHonorific(const world::World& W, int Npc, double WorldMin);
std::vector<std::pair<const world::Award*, const world::Honor*>> NetFestCase(const std::vector<world::Award>& Awards, const world::World& W, int Npc);
// UI/RiverLineNet.cpp (FH-5c): RiverLine's edition in its Afterglow (and its Main's champion) and the wire's overlay items (U26).
const fest::Festival* NetAfterglow(double World, const world::Honor*& Main);
std::vector<std::string> NetTickerOverlays(double World);
} // namespace rlnet_detail
namespace rlignite_detail
{
// UI/RiverLineIgnite.cpp: the event sheet over the floor, and the laptop's Ignite state dropped.
void IgniteOpenSheet(RiverLine& RL, const std::string& EventId, double Now);
void IgniteForget(const RiverLine* RL);
bool IgniteSheetOpen(const RiverLine* RL);
// UI/RiverLineIgnite.cpp (FH-3b): the bandstand's festival pick and the Lighthouse's festival line.
bool IgniteBandstandFest(double World, net::EventInstance& Out, std::string& Label);
std::string IgniteLighthouseAt(double World);
// UI/RiverLineIgnite.cpp (G1 polish g1p items 6 and 7): the bandstand title's and the Lighthouse sign's lines as the draw makes them
// (per line its words and {size, box, baseline, face}), and an edition's Afterglow line for a champion.
std::vector<std::pair<std::string, std::array<float, 4>>> IgniteBandTitleLines(const Canvas& C, const net::EventTemplate& T, bool Pv, float ClockW, float& FactsBase);
std::vector<std::pair<std::string, std::array<float, 4>>> IgniteLighthouseSignLines(const Canvas& C, const std::string& FestLine);
std::string IgniteLighthouseAfterglow(const fest::Festival& F, const std::string& Champion);
void IgniteFestChipFace(Canvas& At, const net::SeriesInfo& Sr, const Rect& R, bool On, bool Hot, uint32_t Fill, uint32_t Coral, uint32_t RimHi);
} // namespace rlignite_detail
namespace rlrit_detail
{
// UI/RiverLineRit.cpp (FH-3b): the tag after an event's name and the tape's festival countdown.
std::string RitTagOf(const net::EventTemplate& T);
std::string RitTapeCountdown(const fest::Festival& F, fest::Phase P, double World);
// UI/RiverLineRit.cpp (FH-5a): a festival event's code on RunItTwice's own surfaces (N-2) and the Main card's held selection (N-3).
std::string RitFestCode(const net::Network& Net, const net::EventInstance& E);
std::string RitTableCode(const std::string& EventId);
std::string RitEventName(const std::string& Name);
void RitHoldSelection(RiverLine& RL, const std::string& InstanceId);
// UI/RiverLineRit.cpp (FH-5c): the tape's festival at a minute (and its phase) and its Afterglow item.
const fest::Festival* RitTapeFest(double World, fest::Phase& P);
std::string RitTapeChampion(const fest::Festival& F);
} // namespace rlrit_detail
namespace agenda_detail
{
// UI/SecondScreen.cpp: a calendar square's word.
std::pair<int, std::string> Word(const life::Mark& M);
} // namespace agenda_detail
namespace rlfest_detail
{
// UI/RiverLineNet.cpp (FH-1c, FH-3b): stream B's strings for fest::FestUiInventory.
std::vector<std::string> HostUiStrings();
// UI/RiverLineNet.cpp (FH-3a): the hub's RCOP Buildup panel, "Last year: {Prize}." from the roll, a brand's next edition.
void RcopBuildupPanel(Canvas& Cv, Ui& UI, const Session& S, const fest::Festival& F, const Rect& Rt, double World, double Now, double Since);
std::string FestLastYear(const world::World& W, const fest::Festival& F);
const fest::Festival* FestNextEdition(const fest::Festival& F);
} // namespace rlfest_detail
} // namespace ui
} // namespace ss

namespace
{
using HostsNear = std::vector<std::pair<const ss::fest::Festival*, ss::fest::Phase>>;

ss::fest::Festival HostsEdition(ss::net::SiteId Site, ss::fest::Brand Which, const char* SeriesId, int FirstDay, int LastDay)
{
	ss::fest::Festival F;
	F.Site = Site;
	F.Which = Which;
	F.SeriesId = SeriesId;
	F.FirstDay = FirstDay;
	F.LastDay = LastDay;
	F.MainDay = LastDay;
	return F;
}

bool HostsSameRect(const ss::ui::Rect& A, const ss::ui::Rect& B)
{
	return A.X == B.X && A.Y == B.Y && A.W == B.W && A.H == B.H;
}

/** FestHostsRouting (FH-1c): a host's SERIES page is a festival edition's hub, in the host's rect, exactly when §2.8.1 says. Plants:
 *  NetHubPick's site test dropped (Ignite's FEIGN opens on RiverLine's page); a picked plain series falling through to the window (a
 *  live festival takes the page from the series the player picked). */
void FestHostsRouting()
{
	using ss::fest::Brand;
	using ss::fest::Phase;
	using ss::net::SiteId;
	namespace rn = ss::ui::rlnet_detail;
	const ss::fest::Festival Rcop = HostsEdition(SiteId::RiverLine, Brand::Rcop, "rcop", 13, 34);
	const ss::fest::Festival Tco = HostsEdition(SiteId::RiverLine, Brand::Tco, "tco27", 250, 263);
	const ss::fest::Festival Feign = HostsEdition(SiteId::Ignite, Brand::Feign, "ignite.feign26", 56, 65);
	const ss::fest::Festival Grit = HostsEdition(SiteId::RunItTwice, Brand::Grit, "runittwice.grit27", 222, 233);

	// A picked series of the host's decides alone.
	ui_test::Expect(rn::NetHubPick(SiteId::RiverLine, true, &Rcop, HostsNear{}) == &Rcop, "FestHostsRouting: a picked festival series opens its hub");
	ui_test::Expect(rn::NetHubPick(SiteId::RiverLine, true, &Rcop, HostsNear{{&Rcop, Phase::Off}}) == &Rcop, "FestHostsRouting: a picked festival series opens its hub in any phase");
	ui_test::Expect(rn::NetHubPick(SiteId::RiverLine, true, nullptr, HostsNear{{&Rcop, Phase::Live}}) == nullptr,
		"FestHostsRouting: a picked plain series keeps today's page while the host's festival is live");
	ui_test::Expect(rn::NetHubPick(SiteId::Ignite, true, &Rcop, HostsNear{}) == nullptr, "FestHostsRouting: another site's edition never opens on this host");

	// None picked: the host's current or next edition inside its Announced-to-Afterglow window, else today's page.
	for (const Phase P : {Phase::Announced, Phase::Buildup, Phase::Live, Phase::MainDay, Phase::Closing, Phase::Afterglow})
	{
		ui_test::Expect(rn::NetHubPick(SiteId::RiverLine, false, nullptr, HostsNear{{&Rcop, P}}) == &Rcop,
			("FestHostsRouting: every phase from Announced to Afterglow opens the host's hub (phase " + std::to_string(static_cast<int>(P)) + ")").c_str());
	}
	ui_test::Expect(rn::NetHubPick(SiteId::RiverLine, false, nullptr, HostsNear{{&Rcop, Phase::Off}}) == nullptr, "FestHostsRouting: an edition that is Off leaves today's page");
	ui_test::Expect(rn::NetHubPick(SiteId::RiverLine, false, nullptr, HostsNear{}) == nullptr, "FestHostsRouting: no edition near leaves today's page");
	ui_test::Expect(rn::NetHubPick(SiteId::RiverLine, false, nullptr, HostsNear{{&Feign, Phase::Live}}) == nullptr, "FestHostsRouting: Ignite's FEIGN never opens on RiverLine's page");
	ui_test::Expect(rn::NetHubPick(SiteId::Ignite, false, nullptr, HostsNear{{&Rcop, Phase::Afterglow}, {&Feign, Phase::Announced}}) == &Feign,
		"FestHostsRouting: Ignite shows FEIGN announced, not RiverLine's RCOP in its Afterglow");
	ui_test::Expect(rn::NetHubPick(SiteId::RiverLine, false, nullptr, HostsNear{{&Rcop, Phase::Afterglow}, {&Tco, Phase::Announced}}) == &Rcop,
		"FestHostsRouting: the edition on stage comes before the next one announced");
	ui_test::Expect(rn::NetHubPick(SiteId::RiverLine, false, nullptr, HostsNear{{&Rcop, Phase::Off}, {&Tco, Phase::Announced}}) == &Tco,
		"FestHostsRouting: after the Afterglow the next edition announced takes the page");
	ui_test::Expect(rn::NetHubPick(SiteId::RunItTwice, false, nullptr, HostsNear{{&Feign, Phase::Live}, {&Grit, Phase::MainDay}}) == &Grit, "FestHostsRouting: RunItTwice shows GRIT");

	// The hosts' content rects (§2.8.1; Ignite's ends at y 870, above the dock, L7).
	ui_test::Expect(HostsSameRect(rn::NetHubRect(SiteId::RiverLine), {24.0f, 84.0f, 1552.0f, 866.0f}), "FestHostsRouting: RiverLine's hub rect is {24, 84, 1552, 866}");
	ui_test::Expect(HostsSameRect(rn::NetHubRect(SiteId::Ignite), {24.0f, 84.0f, 1552.0f, 786.0f}), "FestHostsRouting: Ignite's hub rect is {24, 84, 1552, 786}");
	ui_test::Expect(HostsSameRect(rn::NetHubRect(SiteId::RunItTwice), {232.0f, 96.0f, 1344.0f, 854.0f}), "FestHostsRouting: RunItTwice's hub rect is {232, 96, 1344, 854}");

	// The real path: every series a host lists, picked, routes to exactly its own edition (fest::FestivalOf), a plain series (every
	// series before F-net's FN-1) to today's page.
	int SeriesSeen = 0;
	int HubsSeen = 0;
	for (const SiteId Host : {SiteId::RiverLine, SiteId::Ignite, SiteId::RunItTwice})
	{
		for (const ss::net::SeriesInfo& Sr : ss::net::Site(Host).Series())
		{
			const ss::fest::Festival* Of = ss::fest::FestivalOf(Sr.Id);
			const ss::fest::Festival* Want = Of && Of->Site == Host ? Of : nullptr;
			const ss::fest::Festival* Got = rn::NetHubEdition(Host, Sr.Id, 0.0);
			++SeriesSeen;
			HubsSeen += Got ? 1 : 0;
			if (Got != Want)
			{
				const std::string What = "FestHostsRouting: series " + Sr.Id + " routes to " + (Got ? Got->Name : std::string("today's page")) + ", not " +
					(Want ? Want->Name : std::string("today's page"));
				ui_test::Expect(false, What.c_str());
			}
		}
	}
	ui_test::Expect(SeriesSeen > 0, "FestHostsRouting: the three hosts list their series");
	std::printf("FestHostsRouting: %d series on the three hosts, %d of them routed to a festival hub\n", SeriesSeen, HubsSeen);
}

/** A laptop on Ignite (FH-3b): a session whose clock moves forward to a world minute (the world catches up), Ignite joined, PLAY
 *  open, drawn at scale 1 with the kit's metrics. */
struct HostsLaptop
{
	ui_test::QuietHooks H;
	ss::Session S;
	ss::ui::RiverLine RL;
	double Now = 1.0;
	explicit HostsLaptop(const std::string& Seed) : S(H, Seed), RL(S)
	{
		S.CurrentScreen = ss::Screen::Lobby;
		RL.UI.Ptr.Active = false;
		Now = ui_test::Step(S, Now, 0.2);
	}
	void At(double World)
	{
		if (World > S.WorldMinutes())
		{
			S.LobbyMinutes += World - S.WorldMinutes();
		}
		Now = ui_test::Step(S, Now, 0.2);
		S.Life.Hunger = 20.0;
		S.Life.Thirst = 20.0;
		S.Life.Energy = 80.0;
	}
	void OpenPlay()
	{
		if (!ss::site::AccountOf(S.Life.Sites, ss::net::SiteId::Ignite))
		{
			ss::site::Join(S.Life.Sites, ss::net::SiteId::Ignite, S.WorldMinutes());
		}
		if (ss::site::Account* A = ss::site::AccountOf(S.Life.Sites, ss::net::SiteId::Ignite))
		{
			A->Cents = 5000;
		}
		S.Life.Update2LinesRead |= ss::life::U2Sites;
		RL.SiteShown = ss::net::SiteId::Ignite;
		RL.OpenApp(ss::ui::RiverLine::App::RiverLine, Now);
		S.ShowLobby();
		RL.OpenPage(ss::ui::RiverLine::Page::Lobby, Now);
		Now = ui_test::Step(S, Now, 1.5);
	}
	/** RunItTwice joined (FH-5a) and one of its pages open: THE BOARD is Page::Lobby, a festival's hub Page::Series. */
	void OpenRit(ss::ui::RiverLine::Page Pg)
	{
		if (!ss::site::AccountOf(S.Life.Sites, ss::net::SiteId::RunItTwice))
		{
			ss::site::Join(S.Life.Sites, ss::net::SiteId::RunItTwice, S.WorldMinutes());
		}
		if (ss::site::Account* A = ss::site::AccountOf(S.Life.Sites, ss::net::SiteId::RunItTwice))
		{
			A->Cents = 50000;
		}
		S.Life.Update2LinesRead |= ss::life::U2Sites;
		RL.SiteShown = ss::net::SiteId::RunItTwice;
		RL.OpenApp(ss::ui::RiverLine::App::RiverLine, Now);
		S.ShowLobby();
		RL.OpenPage(Pg, Now);
		Now = ui_test::Step(S, Now, 1.5);
	}
	/** A click at (X, Y): down, up, then the pointer gone; the frame after. */
	ss::ui::DrawList Press(float X, float Y)
	{
		RL.UI.Ptr.Active = true;
		RL.UI.Ptr.X = X;
		RL.UI.Ptr.Y = Y;
		RL.UI.Ptr.Down = true;
		RL.UI.Ptr.Pressed = true;
		Frame();
		Now += 0.05;
		RL.UI.Ptr.Down = false;
		RL.UI.Ptr.Released = true;
		Frame();
		Now += 0.05;
		RL.UI.Ptr.Active = false;
		return Frame();
	}
	ss::ui::DrawList Frame()
	{
		ui_test::TableMeasurer M;
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, M, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, 1.0f);
		RL.Draw(C, Now);
		RL.UI.Ptr.EndFrame();
		return L;
	}
	/** A settled frame (every ease landed). */
	ss::ui::DrawList Settled()
	{
		Now += 1.5;
		Frame();
		Now += 0.05;
		return Frame();
	}
};

/** The bandstand's festival pick worked out from the schedule alone (§2.7.4): the festival's latest event starting later that day. */
bool HostsExpectedPick(const ss::fest::Festival& F, double World, ss::net::EventInstance& Out)
{
	const ss::net::Network& Net = ss::net::Site(ss::net::SiteId::Ignite);
	const double DayEnd = (static_cast<double>(ss::net::DayOf(World)) + 1.0) * ss::net::MinutesPerDay;
	bool Has = false;
	for (const ss::net::EventInstance& E : Net.Window(World - ss::net::MinutesPerDay, DayEnd + ss::net::MinutesPerDay))
	{
		if (Net.TemplateOf(E).Series == F.SeriesId && E.Start > World && E.Start < DayEnd && (!Has || E.Start > Out.Start))
		{
			Out = E;
			Has = true;
		}
	}
	return Has;
}

/** The text of the run whose box starts at (X, its baseline at Base), "" when none: the bandstand's card title sits at (230, 196). */
std::string HostsRunAt(const ss::ui::DrawList& L, float X, float Base)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && std::fabs(Cmd.Text.X - X) < 1.0f && std::fabs(Cmd.Text.BaselineY - Base) < 1.0f)
		{
			return Cmd.Text.Text;
		}
	}
	return std::string();
}

size_t HostsRuns(const ss::ui::DrawList& L)
{
	size_t Runs = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		Runs += Cmd.Type == ss::ui::DrawCmd::Kind::Text ? 1 : 0;
	}
	return Runs;
}

/** FestBandstandPick (§2.15.2, F-hubs' half of FestBandstand; PLAN_G1 item 3): during FEIGN the bandstand features the latest FEIGN
 *  event starting later that day ("FEIGN TONIGHT · #{No}", U32), else The Spark (or what's up next) as today. On F-net's calendar every
 *  FEIGN 2026 day at noon and late evening, the days either side; then drawn on Ignite's PLAY: FEIGN's Buildup (the Lighthouse's
 *  countdown, no takeover yet) and its first day (the pick on the bandstand, the Lighthouse's day line, chip 10, the floor's shelf, no
 *  Prize Counter under the arch) at noon, and again at 19:00 when The Spark's TONIGHT slide has an event of its own (the pick's label
 *  and card drawn, not The Spark's head), with the festival chip and label's cost printed (PLAN_FHUBS FH-3's budget row; FH-7
 *  asserts it).
 *  Plant: The Spark kept during the festival. */
void FestBandstandPick()
{
	namespace ri = ss::ui::rlignite_detail;
	const ss::fest::Festival* Feign = ss::fest::FestivalOf("ignite.feign26");
	ui_test::Expect(Feign && Feign->Site == ss::net::SiteId::Ignite && Feign->Which == ss::fest::Brand::Feign, "FestBandstandPick: FEIGN 2026 is on F-net's calendar, on Ignite");
	if (!Feign)
	{
		return;
	}
	const ss::net::Network& Net = ss::net::Site(ss::net::SiteId::Ignite);
	const ss::net::SeriesInfo* Sr = Net.FindSeries(Feign->SeriesId);
	ui_test::Expect(Sr && !Sr->Short.empty(), "FestBandstandPick: FEIGN 2026's series is Ignite's, with its Short");
	const std::string Short = Sr ? Sr->Short : std::string("FEIGN");
	int Picked = 0;
	int Spark = 0;
	for (int Day = Feign->FirstDay - 1; Day <= Feign->LastDay + 1; ++Day)
	{
		for (const double Hour : {12.0, 23.95})
		{
			const double World = static_cast<double>(Day) * ss::net::MinutesPerDay + Hour * 60.0;
			ss::net::EventInstance Want;
			const bool Live = Day >= Feign->FirstDay && Day <= Feign->LastDay;
			const bool HasWant = Live && HostsExpectedPick(*Feign, World, Want);
			ss::net::EventInstance Got;
			std::string Label;
			const bool HasGot = ri::IgniteBandstandFest(World, Got, Label);
			const std::string When = " (day " + std::to_string(Day) + " at " + std::to_string(static_cast<int>(Hour)) + ")";
			if (HasWant)
			{
				++Picked;
				const std::string WantLabel = Short + " TONIGHT \xC2\xB7 #" + std::to_string(Net.TemplateOf(Want).EventNo);
				ui_test::Expect(HasGot && Got.Id == Want.Id, ("FestBandstandPick: the bandstand features FEIGN's latest event starting later that day" + When).c_str());
				ui_test::Expect(Label == WantLabel, ("FestBandstandPick: the bandstand reads " + WantLabel + ", not " + Label + When).c_str());
			}
			else
			{
				++Spark;
				ui_test::Expect(!HasGot, ("FestBandstandPick: with no FEIGN event still to start that day (or no festival) the bandstand keeps The Spark" + When).c_str());
			}
		}
	}
	ui_test::Expect(Picked >= Feign->LastDay - Feign->FirstDay, "FestBandstandPick: FEIGN's evenings put a FEIGN event on the bandstand nearly every day");
	ui_test::Expect(Spark >= 4, "FestBandstandPick: the days either side and the late evenings keep The Spark");

	// Drawn on Ignite's PLAY: a festival-free day (the baseline), FEIGN's Buildup, FEIGN's first day.
	const auto Began = std::chrono::steady_clock::now();
	HostsLaptop P("u4-fest-hosts");
	const double Free = static_cast<double>(Feign->FirstDay - ss::fest::kBuildupDays - 2) * ss::net::MinutesPerDay + 12.0 * 60.0;
	P.At(Free);
	P.OpenPlay();
	const ss::ui::DrawList Before = P.Settled();
	ui_test::Expect(ui_test::Drew(Before, "BANDSTAND") && !ui_test::Drew(Before, "ON THE FLOOR"), "FestBandstandPick: a festival-free day keeps today's bandstand and floor");
	ui_test::Expect(ui_test::Drew(Before, "PRIZE COUNTER"), "FestBandstandPick: a festival-free day keeps the Prize Counter");
	const double Buildup = static_cast<double>(Feign->FirstDay - 3) * ss::net::MinutesPerDay + 12.0 * 60.0;
	P.At(Buildup);
	P.OpenPlay();
	const ss::ui::DrawList Soon = P.Settled();
	ui_test::Expect(ui_test::DrewRun(Soon, Short + " \xC2\xB7 OPENS IN 3D"), "FestBandstandPick: in FEIGN's Buildup the Lighthouse counts down (FEIGN · OPENS IN 3D)");
	ui_test::Expect(ui_test::Drew(Soon, "BANDSTAND") && ui_test::Drew(Soon, "PRIZE COUNTER") && !ui_test::Drew(Soon, "ON THE FLOOR"), "FestBandstandPick: the Buildup takes nothing else over");
	const double Open = static_cast<double>(Feign->FirstDay) * ss::net::MinutesPerDay + 12.0 * 60.0;
	P.At(Open);
	P.OpenPlay();
	const ss::ui::DrawList During = P.Settled();
	ss::net::EventInstance Pick;
	std::string Label;
	const bool HasPick = ri::IgniteBandstandFest(P.S.WorldMinutes(), Pick, Label);
	ui_test::Expect(HasPick, "FestBandstandPick: FEIGN's first day has an event still to start at noon");
	ui_test::Expect(HasPick && ui_test::DrewRun(During, Label), ("FestBandstandPick: the bandstand draws " + Label).c_str());
	ui_test::Expect(!ui_test::Drew(During, "TONIGHT \xC2\xB7 BANDSTAND") && !ui_test::Drew(During, "UP NEXT \xC2\xB7 BANDSTAND"), "FestBandstandPick: The Spark is not kept during the festival");
	// G1 polish (g1p item 6): the card's title on two lines, the code and buy-in at (230, 186), the words at (230, 204), both drawn whole
	// (no ellipsis) and together the picked event's name.
	const std::string Card = HostsRunAt(During, 230.0f, 186.0f);
	const std::string Code = Short + " #" + std::to_string(HasPick ? Net.TemplateOf(Pick).EventNo : 0) + ":";
	ui_test::Expect(HasPick && Card.compare(0, Code.size(), Code) == 0, ("FestBandstandPick: the bandstand's card is the picked event (" + Code + "), not " + Card).c_str());
	const std::string CardWords = HostsRunAt(During, 230.0f, 204.0f);
	const std::string PickName = HasPick ? Net.TemplateOf(Pick).Name : std::string();
	ui_test::Expect(HasPick && Card + " " + CardWords == PickName,
		("FestBandstandPick: the bandstand's title is the picked event's whole name on two lines (" + PickName + "), not " + Card + " / " + CardWords).c_str());
	const int Days = Feign->LastDay - Feign->FirstDay + 1;
	ui_test::Expect(ui_test::DrewRun(During, Short + " \xC2\xB7 DAY 1 OF " + std::to_string(Days)), "FestBandstandPick: the Lighthouse reads FEIGN · DAY 1 OF 10");
	ui_test::Expect(ui_test::DrewRun(During, Short), "FestBandstandPick: chip 10 reads the festival's Short");
	ui_test::Expect(ui_test::DrewRun(During, Short + " \xC2\xB7 ON THE FLOOR"), "FestBandstandPick: the floor's first shelf is FEIGN · ON THE FLOOR");
	ui_test::Expect(!ui_test::Drew(During, "PRIZE COUNTER") && !ui_test::Drew(During, "Welcome bonus"), "FestBandstandPick: the Prize Counter's lines are hidden under FEIGN's arch");
	// The same day at 19:00, when The Spark's own TONIGHT slide has an event too (at noon it has none): the festival's pick still wins
	// the bandstand, its label and its card drawn, The Spark's TONIGHT head not.
	const double Evening = static_cast<double>(Feign->FirstDay) * ss::net::MinutesPerDay + 19.0 * 60.0;
	P.At(Evening);
	P.OpenPlay();
	const ss::ui::DrawList Eve = P.Settled();
	ss::net::EventInstance EvePick;
	std::string EveLabel;
	const bool HasEve = ri::IgniteBandstandFest(P.S.WorldMinutes(), EvePick, EveLabel);
	ui_test::Expect(HasEve, "FestBandstandPick: FEIGN's first day has an event still to start at 19:00");
	ui_test::Expect(HasEve && ui_test::DrewRun(Eve, EveLabel), ("FestBandstandPick: at 19:00 the bandstand draws " + EveLabel).c_str());
	ui_test::Expect(!ui_test::Drew(Eve, "TONIGHT \xC2\xB7 BANDSTAND"), "FestBandstandPick: at 19:00 The Spark's TONIGHT slide is not kept during the festival");
	const std::string EveCard = HostsRunAt(Eve, 230.0f, 186.0f);
	const std::string EveCode = Short + " #" + std::to_string(HasEve ? Net.TemplateOf(EvePick).EventNo : 0) + ":";
	ui_test::Expect(HasEve && EveCard.compare(0, EveCode.size(), EveCode) == 0, ("FestBandstandPick: at 19:00 the bandstand's card is the picked event (" + EveCode + "), not " + EveCard).c_str());
	const std::string EveWords = HostsRunAt(Eve, 230.0f, 204.0f);
	const std::string EveName = HasEve ? Net.TemplateOf(EvePick).Name : std::string();
	ui_test::Expect(HasEve && EveCard + " " + EveWords == EveName,
		("FestBandstandPick: at 19:00 the bandstand's title is the picked event's whole name on two lines (" + EveName + "), not " + EveCard + " / " + EveWords).c_str());
	const double Took = std::chrono::duration<double>(std::chrono::steady_clock::now() - Began).count();
	// The budget row (PLAN_FHUBS FH-3: the Ignite floor's chip and label <= 1k; printed here, asserted in FH-7 with F-look's attraction
	// merged): chip 10's face drawn alone at both scales (on, the busiest) plus the shelf's label and sub-line (two text runs, no
	// vertices). The whole PLAY pages are printed beside it: their difference also holds each day's own schedule.
	size_t ChipV[2] = {0, 0};
	size_t ChipOff = 0;
	size_t CrestV = 0;
	size_t ChipRuns = 0;
	for (int Si = 0; Si < 2 && Sr; ++Si)
	{
		ui_test::TableMeasurer M;
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, M, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, Si == 0 ? 1.0f : 1.5f);
		ri::IgniteFestChipFace(C, *Sr, {120.0f, 440.0f, C.Measure(Short, 13.5f, 700) + 56.0f, 32.0f}, true, false, Sr->Color, 0xff6b57u, 0x5a4060u);
		ChipV[Si] = L.Vertices.size();
		ChipRuns = HostsRuns(L) + 2;
	}
	if (Sr)
	{
		// Its parts at scale 1: the chip off (no glow) and the crest at 22 alone (F-look's low-detail path draws it at <= 30 px).
		ui_test::TableMeasurer M;
		ss::ui::DrawList Off;
		ss::ui::Canvas C(Off, M, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, 1.0f);
		ri::IgniteFestChipFace(C, *Sr, {120.0f, 440.0f, C.Measure(Short, 13.5f, 700) + 56.0f, 32.0f}, false, false, Sr->Color, 0xff6b57u, 0x5a4060u);
		ChipOff = Off.Vertices.size();
		ss::ui::DrawList Crest;
		ss::ui::Canvas Cc(Crest, M, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, 1.0f);
		ss::ui::eventart::SeriesCrest(Cc, *Sr, 142.0f, 456.0f, 22.0f, 0.0);
		CrestV = Crest.Vertices.size();
	}
	std::printf("FestBandstandPick: the chip off %zu vertices, its crest at 22 alone %zu\n", ChipOff, CrestV);
	std::printf("FestBandstandPick: %d FEIGN pick(s), %d Spark case(s); the floor's chip and label: %zu / %zu vertices (scale 1 / 1.5), %zu runs (budget 1k, FH-7); "
				"PLAY %zu vertices / %zu runs on day %d, %zu / %zu on FEIGN's day 1; %.2f s\n",
		Picked, Spark, ChipV[0], ChipV[1], ChipRuns, Before.Vertices.size(), HostsRuns(Before), Feign->FirstDay - ss::fest::kBuildupDays - 2, During.Vertices.size(),
		HostsRuns(During), Took);
}

/** A line's measured width as the draw measures it: its own size and face ({size, box, baseline, face}). */
float HostsLineWidth(const ss::ui::Canvas& C, const std::pair<std::string, std::array<float, 4>>& Line)
{
	return C.Measure(Line.first, Line.second[0], static_cast<ss::ui::Font>(static_cast<int>(Line.second[3])));
}

/** The Ignite festivals of the 2026-2028 calendars (FEIGN, PUSH, PENNANT), in FirstDay order. */
std::vector<const ss::fest::Festival*> HostsIgniteEditions()
{
	std::vector<const ss::fest::Festival*> Out;
	for (const ss::fest::Festival& F : ss::fest::All())
	{
		if (F.Site == ss::net::SiteId::Ignite && F.Year >= 2026 && F.Year <= 2028)
		{
			Out.push_back(&F);
		}
	}
	return Out;
}

/**
 * FestTitleFits (G1 polish, g1p item 6; PIE s06_ignite_lobby, s06_ignite_wed and s08a_pennant_ignite read "FEIGN #5: $7…", "PENNANT #6:
 * $…"): every numbered and headline event of every Ignite festival in the 2026-2028 calendars (FEIGN 2026-2028, PUSH and PENNANT 2027-
 * 2028) takes the bandstand's two-line title (rlignite_detail::IgniteBandTitleLines, the layout the draw uses): its code and buy-in
 * over its words, the two together its whole name. At UI scales 1.0 and 1.5, on the boardwalk's bandstand and under PENNANT's pavilion,
 * with the widest clock pill the card can show: each line's measured width (the canvas's measure, the size and face the draw uses) fits
 * its box; line 1 ends 8 clear of the clock's pill and line 2 8 clear of the event's Sparks; line 1 stands below the head (the card's
 * eyebrow, or the pavilion screen's top), line 2 below line 1's baseline and the facts line below line 2's. Every other Ignite event
 * keeps today's one line (32 in 226 at 196; 26 in 214 at 210 under the pavilion). FestBandstandPick's PLAY frames hold the drawn half.
 * Plant: the festival events kept on today's one line (the whole name at 32 in 226: "FEIGN #5: $7…").
 */
void FestTitleFits()
{
	namespace ri = ss::ui::rlignite_detail;
	const auto Began = std::chrono::steady_clock::now();
	const ss::net::Network& Net = ss::net::Site(ss::net::SiteId::Ignite);
	const std::vector<const ss::fest::Festival*> Editions = HostsIgniteEditions();
	std::string Names;
	for (const ss::fest::Festival* F : Editions)
	{
		const ss::net::SeriesInfo* Sr = Net.FindSeries(F->SeriesId);
		Names += (Names.empty() ? "" : ", ") + (Sr ? Sr->Short : F->SeriesId) + " " + std::to_string(F->Year);
	}
	ui_test::Expect(Editions.size() == 7, ("FestTitleFits: the 2026-2028 calendars hold seven Ignite festivals (" + Names + ")").c_str());
	ui_test::TableMeasurer M;
	const char* const Clocks[] = {"STARTS 23:59:59", "LATE 23:59:59", "IN 23:59:59", "ON NOW"};
	int Events = 0;
	int Bad = 0;
	int Plain = 0;
	std::string First;
	std::string FirstPlain;
	float Fullest[2] = {0.0f, 0.0f};
	std::string FullestLine[2];
	float Smallest[2] = {99.0f, 99.0f};
	for (int Si = 0; Si < 2; ++Si)
	{
		const float Scale = Si == 0 ? 1.0f : 1.5f;
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, M, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, Scale);
		float ClockW = 110.0f;
		for (const char* Ck : Clocks)
		{
			ClockW = std::max(ClockW, C.Measure(Ck, 12.5f, 900) + 26.0f);
		}
		for (const ss::net::EventTemplate& T : Net.Templates())
		{
			const ss::fest::Festival* F = T.Series.empty() ? nullptr : ss::fest::FestivalOf(T.Series);
			const bool Mine = F && std::find(Editions.begin(), Editions.end(), F) != Editions.end();
			for (const bool Pv : {false, true})
			{
				float FactsBase = 0.0f;
				const std::vector<std::pair<std::string, std::array<float, 4>>> Lines = ri::IgniteBandTitleLines(C, T, Pv, ClockW, FactsBase);
				char Where[64];
				std::snprintf(Where, sizeof(Where), " (%s, scale %.1f)", Pv ? "pavilion" : "bandstand", static_cast<double>(Scale));
				if (!F)
				{
					// Today's one line, as before.
					const bool Same = Lines.size() == 1 && Lines[0].first == T.Name && Lines[0].second[0] == (Pv ? 26.0f : 32.0f) &&
						Lines[0].second[1] == (Pv ? 214.0f : 226.0f) && Lines[0].second[2] == (Pv ? 210.0f : 196.0f) && FactsBase == (Pv ? 231.0f : 218.0f);
					Plain += Same ? 0 : 1;
					if (!Same && FirstPlain.empty())
					{
						FirstPlain = "a non-festival event changed its one line: " + T.Name + Where;
					}
					continue;
				}
				if (!Mine)
				{
					continue;
				}
				Events += Si == 0 && !Pv ? 1 : 0;
				// Each line as drawn first (its measure in its box), then the two lines together its whole name.
				std::string Why;
				const float PillLeft = Pv ? 452.0f : 584.0f - ClockW;
				const float SparksLeft = 546.0f - C.Measure("+" + ss::Grouped(ri::IgniteBaseSparks(T.BuyInCents)), 17.0f, ss::ui::Font::BlackItalic);
				for (size_t I = 0; I < Lines.size() && I < 2 && Why.empty(); ++I)
				{
					const float W = HostsLineWidth(C, Lines[I]);
					const float Box = Lines[I].second[1];
					const float Size = Lines[I].second[0];
					const ss::ui::Font Face = static_cast<ss::ui::Font>(static_cast<int>(Lines[I].second[3]));
					const float Top = Lines[I].second[2] - M.Ascent(Face, Size);
					char Num[160];
					std::snprintf(Num, sizeof(Num), "line %zu '%s' measures %.1f at %.1f in a box of %.1f", I + 1, Lines[I].first.c_str(), static_cast<double>(W), static_cast<double>(Size),
						static_cast<double>(Box));
					if (W > Box + 0.01f || Size < 8.0f)
					{
						Why = Num;
					}
					else if (230.0f + W > (I == 0 ? PillLeft : SparksLeft) - 8.0f)
					{
						Why = std::string(Num) + (I == 0 ? ", into the clock's pill" : ", into the Sparks");
					}
					else if (Top < (I == 0 ? (Pv ? 182.0f : 160.0f) : Lines[0].second[2]))
					{
						Why = std::string(Num) + ", over the line above";
					}
					if (Box > 0.0f && W / Box > Fullest[I])
					{
						Fullest[I] = W / Box;
						FullestLine[I] = Num;
					}
					Smallest[I] = std::min(Smallest[I], Size);
				}
				if (Why.empty() && (Lines.size() != 2 || Lines[0].first + " " + Lines[1].first != T.Name))
				{
					Why = "not its whole name on two lines: " + (Lines.empty() ? std::string() : Lines[0].first) + (Lines.size() > 1 ? " / " + Lines[1].first : std::string());
				}
				if (Why.empty() && FactsBase - M.Ascent(ss::ui::Font::Bold, Pv ? 13.5f : 14.0f) < Lines[1].second[2])
				{
					Why = "the facts line runs into line 2";
				}
				if (!Why.empty())
				{
					++Bad;
					if (First.empty())
					{
						First = T.Name + ": " + Why + Where;
					}
				}
			}
		}
	}
	ui_test::Expect(Events >= 7 * 18, ("FestTitleFits: every Ignite festival's numbered and headline events are checked (" + std::to_string(Events) + ")").c_str());
	ui_test::Expect(Bad == 0, ("FestTitleFits: every festival event's bandstand title is its whole name, each line fitting its box clear of the pill and the Sparks (" +
								  std::to_string(Bad) + " bad; first: " + First + ")")
								  .c_str());
	ui_test::Expect(Plain == 0, ("FestTitleFits: every other Ignite event keeps today's one-line title (" + FirstPlain + ")").c_str());
	std::printf("FestTitleFits: %s; %d festival events x 2 forms x 2 scales; fullest line 1: %s (%.0f%%); fullest line 2: %s (%.0f%%); smallest sizes %.1f / %.1f; "
				"%.2f s\n",
		Names.c_str(), Events, FullestLine[0].c_str(), static_cast<double>(Fullest[0] * 100.0f), FullestLine[1].c_str(), static_cast<double>(Fullest[1] * 100.0f),
		static_cast<double>(Smallest[0]), static_cast<double>(Smallest[1]), std::chrono::duration<double>(std::chrono::steady_clock::now() - Began).count());
}

/**
 * FestSignFits (G1 polish, g1p item 7; PIE s08a_afterglow_ignite reads "PENNANT CHAMPION · J…"): in the Afterglow the Lighthouse's sign
 * shows the champion's whole name. For every Ignite festival of the 2026-2028 calendars (each brand's form: FEIGN, PUSH, PENNANT; U15's
 * words through rlignite_detail::IgniteLighthouseAfterglow) and the longest names the world can produce (the widest and the longest of
 * 20,000 handles::Make draws over the network's countries, regulars and pros, at most 17 characters as Sim::UniqueName keeps them, and
 * a 17-character name of the widest capital), the sign prints "{SHORT} CHAMPION" over the name (rlignite_detail::IgniteLighthouseSignLines,
 * the layout the draw uses): both whole, each line's measured width within the plate's 120 at UI scales 1.0 and 1.5, a generated name
 * at 9 or more, the name's line under the head's. The Buildup's and the days' lines keep today's one line at 10.
 * Plant: the Afterglow kept on today's one line at 10 in 120 ("PENNANT CHAMPION · J…").
 */
void FestSignFits()
{
	namespace ri = ss::ui::rlignite_detail;
	const auto Began = std::chrono::steady_clock::now();
	const std::vector<const ss::fest::Festival*> Editions = HostsIgniteEditions();
	ui_test::TableMeasurer M;
	// The world's longest names: handles::Make's widest and longest (capitals, as the sign prints them), at most 17 characters.
	ss::Rng R("u4-ui-sign-names");
	ss::ui::DrawList Probe;
	ss::ui::Canvas Pc(Probe, M, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, 1.0f);
	std::vector<std::pair<float, std::string>> Widest;
	std::string Longest;
	for (int I = 0; I < 20000; ++I)
	{
		const std::string Country = ss::handles::PickCountry(R);
		const std::string Name = ss::handles::Make(R, Country, I % 4 == 0);
		if (Name.empty() || Name.size() > 17)
		{
			continue;
		}
		std::string Caps = Name;
		for (char& Ch : Caps)
		{
			Ch = static_cast<char>(std::toupper(static_cast<unsigned char>(Ch)));
		}
		Widest.push_back({Pc.Measure(Caps, 11.0f, 900), Name});
		Longest = Name.size() > Longest.size() ? Name : Longest;
	}
	std::sort(Widest.begin(), Widest.end(), [](const auto& A, const auto& B) { return A.first > B.first || (A.first == B.first && A.second < B.second); });
	std::vector<std::string> Champions;
	for (size_t I = 0; I < Widest.size() && I < 8; ++I)
	{
		Champions.push_back(Widest[I].second);
	}
	Champions.push_back(Longest);
	const std::string Wall(17, 'W');
	Champions.push_back(Wall);
	int Lines = 0;
	int Bad = 0;
	std::string First;
	float Fullest = 0.0f;
	std::string FullestLine;
	float Smallest = 99.0f;
	for (int Si = 0; Si < 2; ++Si)
	{
		const float Scale = Si == 0 ? 1.0f : 1.5f;
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, M, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, Scale);
		for (const ss::fest::Festival* F : Editions)
		{
			const ss::net::SeriesInfo* Sr = ss::net::Site(ss::net::SiteId::Ignite).FindSeries(F->SeriesId);
			const std::string Short = Sr ? Sr->Short : std::string();
			// The Buildup's and the days' lines keep today's one line.
			for (const std::string& Today : {Short + " \xC2\xB7 OPENS IN 14D", Short + " \xC2\xB7 DAY 13 OF 13"})
			{
				const std::vector<std::pair<std::string, std::array<float, 4>>> One = ri::IgniteLighthouseSignLines(C, Today);
				if (One.size() != 1 || One[0].first != Today || One[0].second[0] != 10.0f || One[0].second[2] != 286.5f)
				{
					++Bad;
					First = First.empty() ? "the line '" + Today + "' left today's one line" : First;
				}
			}
			for (const std::string& Champ : Champions)
			{
				const std::string Line = ri::IgniteLighthouseAfterglow(*F, Champ);
				const std::vector<std::pair<std::string, std::array<float, 4>>> Two = ri::IgniteLighthouseSignLines(C, Line);
				char Where[96];
				std::snprintf(Where, sizeof(Where), " (%s %d, scale %.1f)", Short.c_str(), F->Year, static_cast<double>(Scale));
				// Each line as drawn first (its measure in the plate), then the head over the whole name.
				std::string Why = Line.empty() ? std::string("no Afterglow line") : std::string();
				for (size_t I = 0; I < Two.size() && Why.empty(); ++I)
				{
					++Lines;
					const float W = HostsLineWidth(C, Two[I]);
					const float Size = Two[I].second[0];
					char Num[160];
					std::snprintf(Num, sizeof(Num), "'%s' measures %.1f at %.1f in %.0f", Two[I].first.c_str(), static_cast<double>(W), static_cast<double>(Size),
						static_cast<double>(Two[I].second[1]));
					if (W > Two[I].second[1] + 0.01f || Size <= 0.0f)
					{
						Why = Num;
					}
					else if (I == 1 && Champ != Wall && Size < 9.0f)
					{
						Why = std::string(Num) + ", under 9";
					}
					else if (I == 1 && Two[1].second[2] - M.Ascent(ss::ui::Font::Black, Size) < Two[0].second[2])
					{
						Why = std::string(Num) + ", over the head's line";
					}
					if (I == 1 && Champ != Wall)
					{
						if (W / Two[I].second[1] > Fullest)
						{
							Fullest = W / Two[I].second[1];
							FullestLine = Num;
						}
						Smallest = std::min(Smallest, Size);
					}
				}
				if (Why.empty() && (Two.size() != 2 || Two[0].first != Short + " CHAMPION" || Two[0].first + " \xC2\xB7 " + Two[1].first != Line))
				{
					Why = "not '" + Short + " CHAMPION' over the whole name: " + (Two.empty() ? std::string() : Two[0].first) + (Two.size() > 1 ? " / " + Two[1].first : std::string());
				}
				if (!Why.empty())
				{
					++Bad;
					First = First.empty() ? Why + Where : First;
				}
			}
		}
	}
	ui_test::Expect(Editions.size() == 7 && Champions.size() >= 3, "FestSignFits: seven Ignite festivals and the world's longest names are checked");
	ui_test::Expect(Bad == 0, ("FestSignFits: the Afterglow sign prints {SHORT} CHAMPION over the champion's whole name, each line inside the plate (" + std::to_string(Bad) +
								  " bad; first: " + First + ")")
								  .c_str());
	std::printf("FestSignFits: %zu editions x %zu names x 2 scales, %d lines; widest generated '%s', longest '%s' (%zu); fullest name line: %s (%.0f%%), smallest %.1f; "
				"%.2f s\n",
		Editions.size(), Champions.size(), Lines, Widest.empty() ? "" : Widest.front().second.c_str(), Longest.c_str(), Longest.size(), FullestLine.c_str(),
		static_cast<double>(Fullest * 100.0f), static_cast<double>(Smallest), std::chrono::duration<double>(std::chrono::steady_clock::now() - Began).count());
}

std::vector<uint32_t> HostsCodePoints(const std::string& S)
{
	std::vector<uint32_t> Out;
	for (size_t I = 0; I < S.size();)
	{
		const unsigned char C0 = static_cast<unsigned char>(S[I]);
		uint32_t Cp = C0;
		size_t Len = 1;
		if (C0 >= 0xF0) { Len = 4; Cp = C0 & 0x07u; }
		else if (C0 >= 0xE0) { Len = 3; Cp = C0 & 0x0Fu; }
		else if (C0 >= 0xC0) { Len = 2; Cp = C0 & 0x1Fu; }
		for (size_t K = 1; K < Len && I + K < S.size(); ++K)
		{
			Cp = (Cp << 6) | (static_cast<unsigned char>(S[I + K]) & 0x3Fu);
		}
		I += Len;
		Out.push_back(Cp);
	}
	return Out;
}

/** What's wrong with one of stream B's entries ("" when nothing), filled from Vars (§2.11, the hub's rules): a kind (label: no closing
 *  stop, at most 84; sentence: CopyLint at 200), U1, both trademark tiers, the design words, the avoided words, X1 and the glyph rule. */
std::string HostsCopyWhy(const std::string& Kind, const std::string& Text, const std::map<std::string, std::string>& Vars)
{
	const std::string Line = copylint::Fill(Text, Vars);
	if (Kind == "sentence")
	{
		const std::string Why = copylint::Check(Text, copylint::Pa, Vars);
		if (!Why.empty())
		{
			return Why;
		}
	}
	else if (Kind == "label")
	{
		if (Line.empty() || Line.find('{') != std::string::npos || Line.find('}') != std::string::npos)
		{
			return "a variable with no sample: " + Line;
		}
		if (copylint::Characters(Line) > copylint::TableTalk)
		{
			return "a label longer than 84 characters";
		}
		if (Line.back() == '.' || Line.back() == '!' || Line.back() == '?' || Line.find("  ") != std::string::npos || Line.front() == ' ' || Line.back() == ' ')
		{
			return "a label with a closing stop or a stray space";
		}
	}
	else
	{
		return "no kind (label or sentence)";
	}
	const std::string Low = trademarks::Lower(Line);
	for (const char* W : {"ai", "bot", "bots", "autoplay", "autopilot"})
	{
		if (trademarks::HasWord(Low, W))
		{
			return std::string("U1: \"") + W + "\"";
		}
	}
	if (const char* Hit = trademarks::Find(Line, trademarks::kSourceScan))
	{
		return std::string("the source tier: ") + Hit;
	}
	if (const char* Hit = trademarks::Find(Line, trademarks::kCopyScan))
	{
		return std::string("the copy tier: ") + Hit;
	}
	if (const char* Hit = trademarks::Find(Line, trademarks::kDesignWords))
	{
		return std::string("a design word: ") + Hit;
	}
	static const char* const HostsAvoided[] = {"spark", "flame", "blaze", "wildfire", "ember", "crest", "heat", "cold", "stamina", "momentum", "energy", "board",
		"watchlist", "headliners", "beef", "hall", "class", "boom", "millions", "jackpot", "run good", "supernova", "premier"};
	for (const char* W : HostsAvoided)
	{
		if (trademarks::HasWord(Low, W))
		{
			return std::string("an avoided word: ") + W;
		}
	}
	if (trademarks::HasWord(Low, "players"))
	{
		return "X1: people counted as players";
	}
	for (const uint32_t Cp : HostsCodePoints(Line))
	{
		const bool Ok = (Cp >= 0x20 && Cp < 0x7F) || Cp == 0xB7 || Cp == 0x2014 || Cp == 0x2013 || Cp == 0x2026 || Cp == 0xE9 || Cp == 0x2019;
		if (!Ok)
		{
			char Buf[48];
			std::snprintf(Buf, sizeof(Buf), "a glyph outside the set: U+%04X", static_cast<unsigned>(Cp));
			return Buf;
		}
	}
	return std::string();
}

/** FestCopyUi, stream B's half (§2.15.2, §2.11; FH-3b): every rlfest_detail::HostUiStrings entry carries its kind and passes the hub's
 *  lint filled from the hub's sample names (stream A's FestCopyUi runs the same entries through fest::FestUiInventory once it merges
 *  them); no entry types a digit (FestBlurbsFilled' U15 half); the figures U15 prints come from the edition and the clock (the
 *  Lighthouse on FEIGN 2026's Buildup and its days, RunItTwice's tape on GRIT 2027's), RunItTwice's tag reads the festival's Short,
 *  never SERIES, and the wall calendar's words are U22's (Word()'s two cases on constructed marks: F-hubs never merges F-net's
 *  fest::Marks). Plants: "bot" in a line; a typed "22" in U15. */
void FestCopyUiHosts()
{
	namespace ri = ss::ui::rlignite_detail;
	namespace rr = ss::ui::rlrit_detail;
	const std::vector<std::string> Inv = ss::ui::rlfest_detail::HostUiStrings();
	std::map<std::string, std::string> Vars = copylint::Samples();
	const std::pair<const char*, const char*> Extra[] = {{"D", "12"}, {"N", "4"}, {"Days", "22"}, {"FirstDate", "Mar 30"}, {"LastDate", "Apr 11"}, {"Gtd", "$281,100"},
		{"Added", "$2,500"}, {"FESTIVAL", "PENNANT"}, {"Time", "9:00 PM"}, {"Name", "MRS. PARK"}, {"Festival", "RCOP 2027"}, {"BuyIn", "$5,250"},
		{"Prize", "$795.32"}, {"Date", "Oct 6"}, {"Rank", "38th"}, {"Scorers", "1,124"}, {"Events", "3"}, {"Cashes", "2"}, {"Best", "9th"},
		{"Field", "1,104"}, {"Dates", "Apr 7 and Apr 9"}, {"Year", "2027"}, {"Site", "RiverLine"}, {"Overlay", "$4,210"}};
	for (const auto& Kv : Extra)
	{
		Vars[Kv.first] = Kv.second;
	}
	int Bad = 0;
	for (const std::string& Entry : Inv)
	{
		const size_t Tab = Entry.find('\t');
		const std::string Kind = Tab == std::string::npos ? std::string() : Entry.substr(0, Tab);
		const std::string Text = Tab == std::string::npos ? Entry : Entry.substr(Tab + 1);
		std::string Why = HostsCopyWhy(Kind, Text, Vars);
		for (size_t I = 0; I < Text.size() && Why.empty(); ++I)
		{
			// A figure is a fill: no digit typed outside a placeholder (the singular "1 event" is a count's form and U23's "Step 1" the
			// ladder's first rung's name, not figures).
			if (std::isdigit(static_cast<unsigned char>(Text[I])) && Text.compare(0, 8, "1 event ") != 0 && Text != "Step 1 runs every four hours.")
			{
				Why = "a digit typed in the format";
			}
		}
		if (!Why.empty())
		{
			++Bad;
			ui_test::Expect(false, ("FestCopyUi (hosts): " + Text + ": " + Why).c_str());
		}
	}
	ui_test::Expect(Inv.size() >= 12, "FestCopyUi (hosts): stream B's strings reach fest::FestUiInventory through HostUiStrings");
	// FH-5c: the wire's overlay items in U26's three forms, in their current sense (SPEC minor 12): never v1.0's "paid {Overlay} over its
	// guarantee", which said the opposite of an overlay.
	int HostsU26 = 0;
	bool HostsOldSense = false;
	for (const std::string& Entry : Inv)
	{
		HostsU26 += Entry == "sentence	{Site} added {Overlay} to cover {Event}'s guarantee." || Entry == "sentence	{Event} is projected {Amount} short." ||
				Entry == "sentence	{Event} is projected {Amount} over."
			? 1
			: 0;
		HostsOldSense = HostsOldSense || Entry.find("over its guarantee") != std::string::npos;
	}
	ui_test::Expect(HostsU26 == 3 && !HostsOldSense, "FestCopyUi (hosts): the wire's overlay items are U26's three forms, never v1.0's \"paid {Overlay} over its guarantee\"");

	// U15's figures (FestBlurbsFilled' U15 half): the Lighthouse's countdown and day from FEIGN 2026's dates and the clock.
	const ss::fest::Festival* Feign = ss::fest::FestivalOf("ignite.feign26");
	const ss::net::SeriesInfo* FeignSr = Feign ? ss::net::Site(ss::net::SiteId::Ignite).FindSeries(Feign->SeriesId) : nullptr;
	if (Feign && FeignSr)
	{
		const int Days = Feign->LastDay - Feign->FirstDay + 1;
		for (int Ahead = 1; Ahead <= ss::fest::kBuildupDays - 1; ++Ahead)
		{
			const double World = static_cast<double>(Feign->FirstDay - Ahead) * ss::net::MinutesPerDay + 12.0 * 60.0;
			const std::string Want = FeignSr->Short + " \xC2\xB7 OPENS IN " + std::to_string(Ahead) + "D";
			ui_test::Expect(ri::IgniteLighthouseAt(World) == Want, ("FestCopyUi (hosts): the Lighthouse reads " + Want + ", not " + ri::IgniteLighthouseAt(World)).c_str());
		}
		for (int Day = 1; Day <= Days; ++Day)
		{
			const double World = static_cast<double>(Feign->FirstDay + Day - 1) * ss::net::MinutesPerDay + 15.0 * 60.0;
			const std::string Want = FeignSr->Short + " \xC2\xB7 DAY " + std::to_string(Day) + " OF " + std::to_string(Days);
			ui_test::Expect(ri::IgniteLighthouseAt(World) == Want, ("FestCopyUi (hosts): the Lighthouse reads " + Want + ", not " + ri::IgniteLighthouseAt(World)).c_str());
		}
		ui_test::Expect(ri::IgniteLighthouseAt(static_cast<double>(Feign->FirstDay - ss::fest::kAnnounceDays + 2) * ss::net::MinutesPerDay).empty(),
			"FestCopyUi (hosts): an Announced festival leaves the Lighthouse its own line");
	}
	else
	{
		ui_test::Expect(false, "FestCopyUi (hosts): FEIGN 2026 is on F-net's calendar");
	}
	// RunItTwice's tape on GRIT 2027 (its Buildup's countdown, its nights) and its tag: the festival's Short.
	const ss::fest::Festival* Grit = ss::fest::FestivalOf("runittwice.grit27");
	const ss::net::Network& Rit = ss::net::Site(ss::net::SiteId::RunItTwice);
	const ss::net::SeriesInfo* GritSr = Grit ? Rit.FindSeries(Grit->SeriesId) : nullptr;
	if (Grit && GritSr)
	{
		const int Nights = Grit->LastDay - Grit->FirstDay + 1;
		const std::string Soon = rr::RitTapeCountdown(*Grit, ss::fest::Phase::Buildup, static_cast<double>(Grit->FirstDay - 2) * ss::net::MinutesPerDay + 600.0);
		ui_test::Expect(Soon == GritSr->Short + " \xC2\xB7 OPENS IN 2D", ("FestCopyUi (hosts): the tape counts down to GRIT (" + Soon + ")").c_str());
		const std::string Night = rr::RitTapeCountdown(*Grit, ss::fest::Phase::Live, static_cast<double>(Grit->FirstDay + 2) * ss::net::MinutesPerDay + 600.0);
		const std::string Head = GritSr->Short + " \xC2\xB7 NIGHT 3 OF " + std::to_string(Nights);
		const std::string Plain = GritSr->Short + " \xC2\xB7 DAY 3 OF " + std::to_string(Nights);
		ui_test::Expect(Night.compare(0, Head.size(), Head) == 0 || Night == Plain, ("FestCopyUi (hosts): the tape reads GRIT's night of its nights (" + Night + ")").c_str());
		int Tagged = 0;
		for (const ss::net::EventTemplate& T : Rit.Templates())
		{
			if (T.Series == Grit->SeriesId)
			{
				++Tagged;
				ui_test::Expect(rr::RitTagOf(T) == GritSr->Short, ("FestCopyUi (hosts): " + T.Id + "'s tag reads " + GritSr->Short + ", never SERIES").c_str());
			}
		}
		ui_test::Expect(Tagged > 0, "FestCopyUi (hosts): GRIT 2027's events are on RunItTwice");
		std::printf("FestCopyUi (hosts): %zu entries, %d bad; GRIT's tape on night 3: %s\n", Inv.size(), Bad, Night.c_str());
	}
	else
	{
		ui_test::Expect(false, "FestCopyUi (hosts): GRIT 2027 is on F-net's calendar");
	}
	for (const ss::net::EventTemplate& T : Rit.Templates())
	{
		if (!T.Series.empty() && !ss::fest::FestivalOf(T.Series))
		{
			ui_test::Expect(rr::RitTagOf(T) == "SERIES", "FestCopyUi (hosts): a plain series' event keeps SERIES");
			break;
		}
	}
	// U22 on the wall calendar (§2.9.1): FESTIVAL ranks 7 and MAIN 6 in a crowded square (PAY 0 ... SERIES 8).
	ss::life::Mark Opens;
	Opens.Kind = ss::fest::MarkFestival;
	ss::life::Mark Main;
	Main.Kind = ss::fest::MarkFestMain;
	ui_test::Expect(ss::ui::agenda_detail::Word(Opens) == std::pair<int, std::string>(7, "FESTIVAL"), "FestCopyUi (hosts): the wall calendar's festival mark reads FESTIVAL (rank 7)");
	ui_test::Expect(ss::ui::agenda_detail::Word(Main) == std::pair<int, std::string>(6, "MAIN"), "FestCopyUi (hosts): the wall calendar's Main Event mark reads MAIN (rank 6)");
}

// THE BOARD's grid (UI/RiverLineRit.cpp's RitGrid, RitRowPitch, RitColEvent, RitColBuy): a row's name at x 322 on the row's y + 22, its
// tag after the name on y + 21.5 in the room up to RitColBuy - 74.
constexpr float HostsGridY = 252.0f;
constexpr float HostsGridH = 680.0f;
constexpr float HostsRowPitch = 34.0f;
constexpr float HostsColEvent = 322.0f;
constexpr float HostsTagEnd = 536.0f;

/** The first RunItTwice edition of a brand (2027's), nullptr when F-net's calendar has none. */
const ss::fest::Festival* HostsRitEdition(ss::fest::Brand Which)
{
	const ss::fest::Festival* First = nullptr;
	for (const ss::fest::Festival& F : ss::fest::All())
	{
		if (F.Site == ss::net::SiteId::RunItTwice && F.Which == Which && (!First || F.FirstDay < First->FirstDay))
		{
			First = &F;
		}
	}
	return First;
}

std::string HostsUpper(std::string S)
{
	for (char& Ch : S)
	{
		Ch = Ch >= 'a' && Ch <= 'z' ? static_cast<char>(Ch - 'a' + 'A') : Ch;
	}
	return S;
}

/** Today's short form of a name (rlnet_detail::NetShortName's rule): the text after its first ": ". */
std::string HostsShortName(const std::string& Name)
{
	const size_t Colon = Name.find(": ");
	return Colon == std::string::npos ? Name : Name.substr(Colon + 2);
}

/** One row of THE BOARD as the last frame drew it: the instance, the row's top and the run in its EVENT column. */
struct HostsBoardRow
{
	ss::net::EventInstance E;
	float Y = 0.0f;
	std::string Name;
	bool Selected = false;
};

/** THE BOARD's wholly visible rows in the grid's order (its Listed and scroll after the frame L). */
std::vector<HostsBoardRow> HostsBoardRows(HostsLaptop& P, const ss::ui::DrawList& L)
{
	namespace rr = ss::ui::rlrit_detail;
	const ss::net::Network& Net = ss::net::Site(ss::net::SiteId::RunItTwice);
	const float Scroll = rr::Access::ListScroll(P.RL);
	const std::vector<std::string>& Listed = rr::Access::Listed(P.RL);
	std::vector<HostsBoardRow> Out;
	for (size_t I = 0; I < Listed.size(); ++I)
	{
		HostsBoardRow R;
		R.Y = HostsGridY + static_cast<float>(I) * HostsRowPitch - Scroll;
		if (R.Y < HostsGridY - 0.5f || R.Y + HostsRowPitch > HostsGridY + HostsGridH + 0.5f || !Net.FindInstance(Listed[I], R.E))
		{
			continue;
		}
		R.Name = HostsRunAt(L, HostsColEvent, R.Y + 22.0f);
		R.Selected = Listed[I] == P.RL.SelectedEvent();
		Out.push_back(R);
	}
	return Out;
}

/** A Board run as drawn against the whole name: the same, or the name cut with an ellipsis after at least Keep bytes. */
bool HostsWholeOrCut(const std::string& Run, const std::string& Name, size_t Keep, bool& Cut)
{
	const std::string Ell = "\xE2\x80\xA6";
	const size_t N = Run.size() >= Ell.size() ? Run.size() - Ell.size() : 0;
	Cut = Run != Name && Run.size() > Ell.size() && Run.compare(N, Ell.size(), Ell) == 0 && N >= Keep && Name.compare(0, N, Run, 0, N) == 0;
	return Run == Name || Cut;
}

/** FestCopyUi's N-2 half (§2.3.2, the lead's N-2; §2.15.2's RunItTwice festival pages; FH-5a): on THE BOARD a festival event's row
 *  reads its instance's name, its code kept ("WRIT 13: $1,050 NLHE [Main Event]", whole: the row runs on to the buy-in figure, and only
 *  a name longer than that ends in an ellipsis, its code whole), and its Prospectus the same in capitals; every other row on the same
 *  Board keeps today's NetShortName form. WRIT 2027's day 2 and its Main day at noon.
 *  Plant: NetShortName left on THE BOARD's festival rows. */
void FestCopyUiRitBoard(HostsLaptop& P, const ss::fest::Festival& Writ)
{
	const ss::net::Network& Net = ss::net::Site(ss::net::SiteId::RunItTwice);
	const ss::net::SeriesInfo* Sr = Net.FindSeries(Writ.SeriesId);
	const std::string Short = Sr ? Sr->Short : std::string("WRIT");
	int FestRows = 0;
	int PlainRows = 0;
	int PlainSeries = 0;
	int Prospects = 0;
	int Cuts = 0;
	std::string MainRow;
	for (const int Day : {Writ.FirstDay + 1, Writ.MainDay})
	{
		P.At(static_cast<double>(Day) * ss::net::MinutesPerDay + 12.0 * 60.0);
		P.OpenRit(ss::ui::RiverLine::Page::Lobby);
		const ss::ui::DrawList L = P.Settled();
		const std::string When = " (day " + std::to_string(Day) + ")";
		std::string FestId;
		for (const HostsBoardRow& R : HostsBoardRows(P, L))
		{
			const ss::net::EventTemplate& T = Net.TemplateOf(R.E);
			if (!T.Series.empty() && ss::fest::FestivalOf(T.Series))
			{
				++FestRows;
				const std::string Want = Net.NameOf(R.E);
				bool Cut = false;
				const bool Ok = HostsWholeOrCut(R.Name, Want, Want.find(": ") + 2, Cut);
				Cuts += Cut ? 1 : 0;
				ui_test::Expect(Ok, ("FestCopyUi (N-2): THE BOARD's festival row reads " + Want + ", not " + R.Name + When).c_str());
				ui_test::Expect(Want.compare(0, Short.size() + 1, Short + " ") == 0 && Want.find(": ") != std::string::npos && Want.find('#') == std::string::npos,
					("FestCopyUi (N-2): RunItTwice's festival code has no '#' (" + Want + ")").c_str());
				FestId = FestId.empty() ? R.E.Id : FestId;
				MainRow = T.Id == Writ.MainId ? R.Name : MainRow;
			}
			else
			{
				++PlainRows;
				PlainSeries += T.Series.empty() ? 0 : 1;
				const std::string Want = HostsShortName(T.Name);
				ui_test::Expect(R.Name == Want, ("FestCopyUi (N-2): a plain row keeps today's form " + Want + ", not " + R.Name + When).c_str());
			}
		}
		// The Prospectus on a festival event (the Main on its day): the name in capitals, its code kept.
		ss::net::EventInstance Pick;
		const bool HasMain = Day == Writ.MainDay && Net.Next(Writ.MainId, P.S.WorldMinutes(), Pick);
		if (!HasMain && !FestId.empty())
		{
			Net.FindInstance(FestId, Pick);
		}
		if (HasMain || !FestId.empty())
		{
			P.RL.SelectEvent(Pick.Id);
			const ss::ui::DrawList Sel = P.Settled();
			const std::string Want = HostsUpper(Net.NameOf(Pick));
			ui_test::Expect(ui_test::DrewRun(Sel, Want), ("FestCopyUi (N-2): the Prospectus reads " + Want + When).c_str());
			++Prospects;
			if (HasMain)
			{
				ui_test::Expect(Want == "WRIT 13: $1,050 NLHE [MAIN EVENT]", ("FestCopyUi (N-2): WRIT 2027's Main is WRIT 13: $1,050 NLHE [MAIN EVENT] on the Prospectus, not " + Want).c_str());
			}
		}
	}
	ui_test::Expect(FestRows >= 2, "FestCopyUi (N-2): THE BOARD lists WRIT's events on its day 2 and Main day");
	ui_test::Expect(PlainRows >= 2, "FestCopyUi (N-2): THE BOARD lists RunItTwice's own events beside WRIT's");
	ui_test::Expect(Prospects == 2, "FestCopyUi (N-2): the Prospectus opened on a WRIT event both days");
	ui_test::Expect(MainRow == "WRIT 13: $1,050 NLHE [Main Event]", ("FestCopyUi (N-2): THE BOARD lists WRIT 2027's Main as WRIT 13: $1,050 NLHE [Main Event], not " + MainRow).c_str());
	std::printf("FestCopyUi (N-2): THE BOARD on WRIT's day 2 and Main day: %d festival rows (%d cut at the buy-in), %d plain rows (%d of a plain series), %d Prospectus; "
				"the Main's row: %s\n", FestRows, Cuts, PlainRows, PlainSeries, Prospects, MainRow.c_str());
}

/** FestCopyUi's N-2 half, the plain series (§2.3.2: every other event unchanged): RunItTwice's own series between WRIT and REIGN (2027's
 *  Ironworks) keeps today's NetShortName form on THE BOARD, its code dropped as before G1, and the Prospectus likewise. */
void FestCopyUiRitPlainSeries(HostsLaptop& P, const ss::fest::Festival& Writ, const ss::fest::Festival& Reign)
{
	const ss::net::Network& Net = ss::net::Site(ss::net::SiteId::RunItTwice);
	const ss::net::SeriesInfo* Plain = nullptr;
	for (const ss::net::SeriesInfo& Sr : Net.Series())
	{
		const bool Between = Sr.FirstDay > Writ.MainDay + 1 && Sr.LastDay < Reign.FirstDay - 1;
		Plain = !Plain && !Sr.Festival && !ss::fest::FestivalOf(Sr.Id) && Between ? &Sr : Plain;
	}
	ui_test::Expect(Plain != nullptr, "FestCopyUi (N-2): RunItTwice runs a plain series between WRIT and REIGN");
	if (!Plain)
	{
		return;
	}
	P.At(static_cast<double>(Plain->FirstDay + 1) * ss::net::MinutesPerDay + 12.0 * 60.0);
	P.OpenRit(ss::ui::RiverLine::Page::Lobby);
	const ss::ui::DrawList L = P.Settled();
	int Rows = 0;
	int Coded = 0;
	std::string Id;
	for (const HostsBoardRow& R : HostsBoardRows(P, L))
	{
		const ss::net::EventTemplate& T = Net.TemplateOf(R.E);
		if (T.Series != Plain->Id)
		{
			continue;
		}
		++Rows;
		Coded += T.Name.find(": ") != std::string::npos ? 1 : 0;
		Id = Id.empty() ? R.E.Id : Id;
		const std::string Want = HostsShortName(T.Name);
		ui_test::Expect(R.Name == Want, ("FestCopyUi (N-2): a plain series' row keeps today's NetShortName form " + Want + ", not " + R.Name).c_str());
	}
	ui_test::Expect(Rows >= 1, ("FestCopyUi (N-2): THE BOARD lists " + Plain->Name + "'s events").c_str());
	if (!Id.empty())
	{
		ss::net::EventInstance E;
		Net.FindInstance(Id, E);
		P.RL.SelectEvent(Id);
		const ss::ui::DrawList Sel = P.Settled();
		const std::string Want = HostsUpper(HostsShortName(Net.TemplateOf(E).Name));
		ui_test::Expect(ui_test::DrewRun(Sel, Want), ("FestCopyUi (N-2): a plain series' Prospectus keeps today's form " + Want).c_str());
	}
	std::printf("FestCopyUi (N-2): %s on day %d: %d rows, %d with a code in the template's name, each drawn without it\n", Plain->Name.c_str(), Plain->FirstDay + 1, Rows, Coded);
}

/** The first instance of a festival whose name starts with Code ("GRIT 02-E: "), searched day by day through the edition. */
bool HostsFestRun(const ss::fest::Festival& F, const std::string& Code, ss::net::EventInstance& Out)
{
	const ss::net::Network& Net = ss::net::Site(F.Site);
	for (int Day = F.FirstDay; Day <= F.LastDay; ++Day)
	{
		for (const ss::net::EventInstance& E : Net.Window(static_cast<double>(Day) * ss::net::MinutesPerDay, static_cast<double>(Day + 1) * ss::net::MinutesPerDay))
		{
			if (Net.TemplateOf(E).Series == F.SeriesId && Net.NameOf(E).compare(0, Code.size(), Code) == 0)
			{
				Out = E;
				return true;
			}
		}
	}
	return false;
}

/** A table header as RunItTwice composes it for an instance (RitFrame's header: RitTableCode + RitEventName of the listing's name). */
std::string HostsRitHeader(const ss::net::EventInstance& E)
{
	namespace rr = ss::ui::rlrit_detail;
	const ss::net::Network& Net = ss::net::Site(ss::net::SiteId::RunItTwice);
	return rr::RitTableCode(E.Id) + rr::RitEventName(Net.Listing(E).Spec.Name);
}

/** FestCopyUi's N-2 half on GRIT's night (§2.3.2; PIE step 8): THE BOARD lists both runs of a show with their codes, "GRIT 02-E" and
 *  "GRIT 02-L" (Network::NameOf from the start minute), and a GRIT table's header keeps the code, the run's "-E" included ("GRIT 02-E:
 *  Box Office PKO"; today's header reads "Box Office PKO"). At 18:00 on the night of GRIT 2027's 02 runs. */
void FestCopyUiRitGrit(HostsLaptop& P, const ss::fest::Festival& Grit)
{
	const ss::net::Network& Net = ss::net::Site(ss::net::SiteId::RunItTwice);
	const ss::net::SeriesInfo* Sr = Net.FindSeries(Grit.SeriesId);
	const std::string Short = Sr ? Sr->Short : std::string("GRIT");
	ss::net::EventInstance Early;
	ss::net::EventInstance Late;
	const bool Found = HostsFestRun(Grit, Short + " 02-E: ", Early) && HostsFestRun(Grit, Short + " 02-L: ", Late);
	ui_test::Expect(Found && ss::net::DayOf(Early.Start) == ss::net::DayOf(Late.Start), "FestCopyUi (N-2): GRIT 2027's show 02 runs twice one night (02-E and 02-L)");
	if (!Found)
	{
		return;
	}
	const int Day = ss::net::DayOf(Early.Start);
	P.At(static_cast<double>(Day) * ss::net::MinutesPerDay + 18.0 * 60.0);
	P.OpenRit(ss::ui::RiverLine::Page::Lobby);
	const ss::ui::DrawList L = P.Settled();
	int Coded = 0;
	bool SawEarly = false;
	bool SawLate = false;
	for (const HostsBoardRow& R : HostsBoardRows(P, L))
	{
		const ss::net::EventTemplate& T = Net.TemplateOf(R.E);
		if (T.Series != Grit.SeriesId)
		{
			continue;
		}
		const std::string Want = Net.NameOf(R.E);
		bool Cut = false;
		ui_test::Expect(HostsWholeOrCut(R.Name, Want, Want.find(": ") + 2, Cut), ("FestCopyUi (N-2): THE BOARD's GRIT row reads " + Want + ", not " + R.Name).c_str());
		++Coded;
		SawEarly = SawEarly || R.E.Id == Early.Id;
		SawLate = SawLate || R.E.Id == Late.Id;
	}
	ui_test::Expect(SawEarly && SawLate, "FestCopyUi (N-2): THE BOARD lists both runs of GRIT's show 02 at 18:00");
	ui_test::Expect(Net.NameOf(Early) == "GRIT 02-E: $16.50 Box Office PKO" && Net.NameOf(Late) == "GRIT 02-L: $16.50 Box Office PKO",
		("FestCopyUi (N-2): GRIT 2027's show 02 is GRIT 02-E / 02-L: $16.50 Box Office PKO (" + Net.NameOf(Early) + ", " + Net.NameOf(Late) + ")").c_str());
	const std::string Head = HostsRitHeader(Early);
	ui_test::Expect(Head == "GRIT 02-E: Box Office PKO", ("FestCopyUi (N-2): a GRIT 02-E table's header reads GRIT 02-E: Box Office PKO, not " + Head).c_str());
	ui_test::Expect(HostsRitHeader(Late) == "GRIT 02-L: Box Office PKO", "FestCopyUi (N-2): a GRIT 02-L table's header keeps its own run");
	std::printf("FestCopyUi (N-2): GRIT's night (day %d): %d GRIT rows coded; the 02-E table's header %s\n", Day, Coded, Head.c_str());
}

/** FestCopyUi's N-2 half at the table, drawn (§2.3.2; PIE step 8b): sat at REIGN 04 in its late registration on REIGN 2027's day 2, the
 *  table's header keeps the code, whole ("REIGN 04: Dumbwaiter NLHE" in the section slab's capitals: a festival table's header fits
 *  down to 18, where every other name stops at 22 and ends in an ellipsis). The last step on the laptop (the seat stays). */
void FestCopyUiRitTable(HostsLaptop& P, const ss::fest::Festival& Reign)
{
	const ss::net::Network& Net = ss::net::Site(ss::net::SiteId::RunItTwice);
	const ss::net::SeriesInfo* Sr = Net.FindSeries(Reign.SeriesId);
	const std::string Code = (Sr ? Sr->Short : std::string("REIGN")) + " 04: ";
	ss::net::EventInstance Run;
	const bool Found = HostsFestRun(Reign, Code, Run);
	ui_test::Expect(Found && ss::net::DayOf(Run.Start) == Reign.FirstDay + 1, ("FestCopyUi (N-2): REIGN 2027 runs " + Code + "on its day 2").c_str());
	if (!Found || Run.Start + 20.0 < P.S.WorldMinutes())
	{
		return;
	}
	P.At(Run.Start + 20.0);
	P.OpenRit(ss::ui::RiverLine::Page::Lobby);
	P.S.BankrollCents = 200000;
	std::string Lock;
	const ss::LobbyEvent Ev = Net.Listing(Run, &Lock, P.S.Unlocks());
	const std::string Why = P.S.RegisterEvent(Ev);
	ui_test::Expect(Why.empty() && P.S.T, ("FestCopyUi (N-2): the player sits " + Net.NameOf(Run) + " in its late registration (" + Why + "; lock '" + Lock + "')").c_str());
	if (!P.S.T)
	{
		return;
	}
	P.S.CurrentPace = ss::Pace::Full;
	const ss::ui::DrawList L = P.Settled();
	const std::string Want = HostsRitHeader(Run);
	ui_test::Expect(Want == "REIGN 04: Dumbwaiter NLHE", ("FestCopyUi (N-2): REIGN 04's table is REIGN 04: Dumbwaiter NLHE, not " + Want).c_str());
	const std::string Head = HostsUpper(Want);
	bool Drew = false;
	std::string Seen;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text.compare(0, Code.size(), Code) == 0)
		{
			Drew = Drew || Cmd.Text.Text == Head;
			Seen = Cmd.Text.Text;
		}
	}
	ui_test::Expect(Drew, ("FestCopyUi (N-2): the table header keeps the code, " + Head + " (drawn: " + Seen + ")").c_str());
	std::printf("FestCopyUi (N-2): the REIGN table's header: %s\n", Seen.c_str());
}

/** FestYourStrip's N-3 half (§2.8.2, the lead's N-3; FH-5a): on REIGN 2027's day 2 (eleven days before its Main) the Main card's Details
 *  opens the Prospectus on the Main itself (REIGN 30: $2,100 NLHE [MAIN EVENT], RIT-1800) though THE BOARD's 24-hour window doesn't
 *  list it, and the Board's fallback leaves it there on the next frames; a filter pick, and a row pick, end the hold. Details is the
 *  hub's fsmain when this tree's hub draws it (stream A), else its RunItTwice half, rlrit_detail::RitHoldSelection.
 *  Plant: Details only selecting the Main's id (the Board swaps in its first row). */
void FestYourStripRitHeld(HostsLaptop& P, const ss::fest::Festival& Reign, ss::ui::DrawList& Board)
{
	namespace rr = ss::ui::rlrit_detail;
	const ss::net::Network& Net = ss::net::Site(ss::net::SiteId::RunItTwice);
	P.At(static_cast<double>(Reign.FirstDay + 1) * ss::net::MinutesPerDay + 12.0 * 60.0);
	ss::net::EventInstance Main;
	const bool HasMain = Net.Next(Reign.MainId, P.S.WorldMinutes(), Main);
	ui_test::Expect(HasMain && ss::net::DayOf(Main.Start) == Reign.MainDay, "FestYourStrip (N-3): REIGN 2027's Main is on its Main day");
	if (!HasMain)
	{
		return;
	}
	P.OpenRit(ss::ui::RiverLine::Page::Lobby);
	P.Settled();
	const std::vector<std::string>& Listed = rr::Access::Listed(P.RL);
	ui_test::Expect(std::find(Listed.begin(), Listed.end(), Main.Id) == Listed.end() && !Listed.empty(), "FestYourStrip (N-3): THE BOARD's window doesn't list the Main on REIGN's day 2");
	// Details: the hub's Main card when it draws one, else the RunItTwice half the card calls.
	P.RL.ShowSeries(Reign.SeriesId, P.Now);
	P.RL.OpenPage(ss::ui::RiverLine::Page::Series, P.Now);
	const ss::ui::DrawList Hub = P.Settled();
	const ss::ui::TextItem* Details = nullptr;
	for (const ss::ui::DrawCmd& Cmd : Hub.Cmds)
	{
		Details = Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == "Details" ? &Cmd.Text : Details;
	}
	const bool ViaHub = Details != nullptr;
	if (ViaHub)
	{
		P.Press(Details->X + 6.0f, Details->BaselineY - 4.0f);
	}
	else
	{
		rr::RitHoldSelection(P.RL, Main.Id);
		P.RL.OpenPage(ss::ui::RiverLine::Page::Lobby, P.Now);
	}
	const ss::ui::DrawList Opened = P.Settled();
	const std::string Want = HostsUpper(Net.NameOf(Main));
	ui_test::Expect(Want == "REIGN 30: $2,100 NLHE [MAIN EVENT]", ("FestYourStrip (N-3): REIGN 2027's Main reads REIGN 30: $2,100 NLHE [MAIN EVENT], not " + Want).c_str());
	ui_test::Expect(P.RL.SelectedEvent() == Main.Id, "FestYourStrip (N-3): Details selects the Main and the Board's fallback leaves it");
	ui_test::Expect(ui_test::DrewRun(Opened, Want) && ui_test::DrewRun(Opened, "RIT-1800"), ("FestYourStrip (N-3): the Prospectus opens on the Main itself, " + Want + " under RIT-1800").c_str());
	P.Now += 0.05;
	const ss::ui::DrawList Next = P.Frame();
	ui_test::Expect(P.RL.SelectedEvent() == Main.Id && ui_test::DrewRun(Next, Want), "FestYourStrip (N-3): the next frame leaves the Prospectus on the Main");
	Board = P.Settled();
	ui_test::Expect(P.RL.SelectedEvent() == Main.Id && ui_test::DrewRun(Board, Want), "FestYourStrip (N-3): the Prospectus stays on the Main");
	const std::vector<HostsBoardRow> Rows = HostsBoardRows(P, Board);
	// A filter pick ends the hold: the Board's fallback takes its own row.
	P.RL.SetFilter(ss::ui::RiverLine::Filter::Playable, P.Now);
	P.Settled();
	ui_test::Expect(P.RL.SelectedEvent() != Main.Id, "FestYourStrip (N-3): a filter pick ends the hold (the Board selects its own row)");
	P.RL.SetFilter(ss::ui::RiverLine::Filter::All, P.Now);
	P.Settled();
	// Held again, then a row pick ends it: the Main selected alone afterwards is swapped out as today.
	rr::RitHoldSelection(P.RL, Main.Id);
	P.Settled();
	ui_test::Expect(P.RL.SelectedEvent() == Main.Id, "FestYourStrip (N-3): held again");
	const std::vector<HostsBoardRow> Again = HostsBoardRows(P, P.Frame());
	std::string Picked;
	if (!Again.empty())
	{
		P.Press(700.0f, Again.front().Y + HostsRowPitch / 2.0f);
		Picked = P.RL.SelectedEvent();
		ui_test::Expect(Picked == Again.front().E.Id, "FestYourStrip (N-3): a row pick selects the row");
	}
	P.RL.SelectEvent(Main.Id);
	P.Settled();
	ui_test::Expect(P.RL.SelectedEvent() != Main.Id, "FestYourStrip (N-3): after a row pick the hold is gone (the Main alone is swapped out, as before G1)");
	std::printf("FestYourStrip (N-3): Details via %s; the Prospectus on %s from day %d (the Main on day %d), %zu Board rows, the row pick %s\n",
		ViaHub ? "the hub's fsmain" : "rlrit_detail::RitHoldSelection (this tree's hub has no Main card)", Want.c_str(), Reign.FirstDay + 1, Reign.MainDay, Rows.size(), Picked.c_str());
}

/** FestReignCopy's THE BOARD's tag half (§2.15.1; PLAN_G1 item 9; FH-5a): on REIGN's Board nothing but REIGN's tag (the Short alone,
 *  drawn beside a name under 186 units) is drawn in a REIGN row's room after its name, up to the buy-in figure (no vertex, no other
 *  run), and no sponsor brand's name (a non-site brand: Name or Short) is on the Board at all. Plants: a sponsor brand drawn by the
 *  REIGN tag (its mark; its name). */
void FestReignCopyBoardTag(const ss::fest::Festival& Reign, const ss::ui::DrawList& Board, const std::vector<HostsBoardRow>& Rows)
{
	const ss::net::Network& Net = ss::net::Site(ss::net::SiteId::RunItTwice);
	const ss::net::SeriesInfo* Sr = Net.FindSeries(Reign.SeriesId);
	const std::string Short = Sr ? Sr->Short : std::string("REIGN");
	int ReignRows = 0;
	int Tags = 0;
	float Widest = 0.0f;
	for (const HostsBoardRow& R : Rows)
	{
		if (Net.TemplateOf(R.E).Series != Reign.SeriesId)
		{
			continue;
		}
		++ReignRows;
		const ss::ui::TextItem* Tag = nullptr;
		for (const ss::ui::DrawCmd& Cmd : Board.Cmds)
		{
			const bool On = Cmd.Type == ss::ui::DrawCmd::Kind::Text && std::fabs(Cmd.Text.BaselineY - (R.Y + 21.5f)) < 1.0f && Cmd.Text.X > HostsColEvent + 1.0f &&
				Cmd.Text.X < HostsTagEnd;
			Tag = On && Cmd.Text.Text == Short ? &Cmd.Text : Tag;
		}
		Tags += Tag ? 1 : 0;
		// The room after the name, up to the buy-in figure (right-aligned at x 610): where the tag would sit, and nothing else.
		ui_test::TableMeasurer M;
		const float NameEnd = HostsColEvent + M.Width(R.Name, ss::ui::Font::Regular, 14.0f);
		const float BuyLeft = 610.0f - M.Width(ss::ui::rlrit_detail::RitBuyIn(Net.TemplateOf(R.E).BuyInCents), ss::ui::Font::BoldCondensed, 15.0f);
		const ss::ui::Rect Room{NameEnd + 2.0f, R.Y + 3.0f, std::max(0.0f, BuyLeft - NameEnd - 4.0f), HostsRowPitch - 6.0f};
		Widest = std::max(Widest, Room.W);
		const auto Inside = [&Room](float X, float Y) { return X >= Room.X && X <= Room.X + Room.W && Y >= Room.Y && Y <= Room.Y + Room.H; };
		size_t Marks = 0;
		for (const ss::ui::Vertex& V : Board.Vertices)
		{
			Marks += Inside(V.X, V.Y) ? 1 : 0;
		}
		size_t Runs = 0;
		for (const ss::ui::DrawCmd& Cmd : Board.Cmds)
		{
			Runs += Cmd.Type == ss::ui::DrawCmd::Kind::Text && &Cmd.Text != Tag && Inside(Cmd.Text.X, Cmd.Text.BaselineY - 4.0f) ? 1 : 0;
		}
		ui_test::Expect(Marks == 0 && Runs == 0, ("FestReignCopy (THE BOARD's tag): nothing but the tag is drawn after " + R.Name + " (" + std::to_string(Marks) + " vertices, " +
			std::to_string(Runs) + " runs)").c_str());
	}
	int Brands = 0;
	for (const ss::sponsor::Brand& B : ss::sponsor::Brands())
	{
		if (B.Site != -1)
		{
			continue; // a site's own brand (RunItTwice prints its own name on its pages)
		}
		++Brands;
		for (const std::string& Word : {B.Name, B.Short})
		{
			if (Word.empty())
			{
				continue;
			}
			const std::string Low = trademarks::Lower(Word);
			for (const ss::ui::DrawCmd& Cmd : Board.Cmds)
			{
				if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && trademarks::Lower(Cmd.Text.Text).find(Low) != std::string::npos)
				{
					ui_test::Expect(false, ("FestReignCopy (THE BOARD's tag): REIGN's Board draws no sponsor brand (" + Word + " in " + Cmd.Text.Text + ")").c_str());
				}
			}
		}
	}
	ui_test::Expect(ReignRows >= 2 && Widest >= 12.0f, "FestReignCopy (THE BOARD's tag): REIGN's Board lists REIGN's events, with room after a name");
	std::printf("FestReignCopy (THE BOARD's tag): %d REIGN rows, %d REIGN tags drawn (none beside a name past 186 units), the widest room after a name %.0f, %d sponsor "
				"brands looked for\n", ReignRows, Tags, Widest, Brands);
}

/** The RunItTwice half of FH-5a, on one laptop: WRIT 2027 (N-2 on THE BOARD and the Prospectus), RunItTwice Open Spring (a plain
 *  series unchanged), GRIT 2027's night of show 02 (the -E and -L codes; the header's code), REIGN 2027's day 2 (N-3 at noon, the REIGN
 *  tag), then sat at REIGN 04 (the drawn header). */
void FestRitCodesAndHeld()
{
	const auto Began = std::chrono::steady_clock::now();
	const ss::fest::Festival* Writ = HostsRitEdition(ss::fest::Brand::Writ);
	const ss::fest::Festival* Grit = HostsRitEdition(ss::fest::Brand::Grit);
	const ss::fest::Festival* Reign = HostsRitEdition(ss::fest::Brand::Reign);
	ui_test::Expect(Writ && Grit && Reign, "FestCopyUi (N-2): WRIT, GRIT and REIGN are on F-net's calendar, on RunItTwice");
	if (!Writ || !Grit || !Reign)
	{
		return;
	}
	HostsLaptop P("u4-fest-hosts-rit");
	FestCopyUiRitBoard(P, *Writ);
	FestCopyUiRitPlainSeries(P, *Writ, *Reign);
	FestCopyUiRitGrit(P, *Grit);
	ss::ui::DrawList Board;
	FestYourStripRitHeld(P, *Reign, Board);
	FestReignCopyBoardTag(*Reign, Board, HostsBoardRows(P, Board));
	FestCopyUiRitTable(P, *Reign);
	std::printf("FestRitCodesAndHeld: WRIT from day %d, GRIT from day %d, REIGN from day %d; %.2f s\n", Writ->FirstDay, Grit->FirstDay, Reign->FirstDay,
		std::chrono::duration<double>(std::chrono::steady_clock::now() - Began).count());
}
/** A whole-dollar figure as RiverLine prints a pool or a guarantee (NetMoney's whole form: the test's own, from the cents). */
std::string HostsWhole(int64_t Cents)
{
	return "$" + ss::Grouped((Cents + 50) / 100);
}

/** U24's and U23's "Last year: {Prize}." as the test reads it from the world's roll itself (world::FestRoll of F's brand: a row of the
 *  year before F's with a Main that paid), "" when the roll holds none: the oracle the panel and the Career goal answer to, never
 *  the function under test (FIX ROUND 1 (d)). */
std::string HostsLastYearWant(const ss::world::World& W, const ss::fest::Festival& F)
{
	for (const ss::world::FestRollRow& Row : ss::world::FestRoll(W, F.Which))
	{
		if (Row.Year == F.Year - 1 && Row.Main && Row.Main->Prize > 0)
		{
			return "Last year: " + HostsWhole(Row.Main->Prize) + ".";
		}
	}
	return std::string();
}

/** Every text run of a frame joined by single spaces (a wrapped paragraph reads back as its sentence). */
std::string HostsText(const ss::ui::DrawList& L)
{
	std::string Out;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text)
		{
			Out += (Out.empty() ? "" : " ") + Cmd.Text.Text;
		}
	}
	return Out;
}

/** The first run holding a removed or typed money claim in its claim form (§2.2.2, §2.8.8, §2.8.9; PLAN_G1 item 29: "7,412 entries",
 *  never a bare "7,412", which Phase 0's "$7,412.30" mail prints), "" when none. Boards adds the boards' never-paid prizes. */
std::string HostsClaim(const ss::ui::DrawList& L, bool Boards)
{
	static const char* const Forms[] = {"4,108,220", "4.1M", "$100,000,000", "7,412 entries", "Last year's champion won"};
	static const char* const BoardForms[] = {"Platinum Pass", "$6,500", "RCOP Main package", "Main Event packages", "worth $25,000"};
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text)
		{
			continue;
		}
		for (const char* F : Forms)
		{
			if (Cmd.Text.Text.find(F) != std::string::npos)
			{
				return Cmd.Text.Text;
			}
		}
		for (const char* F : BoardForms)
		{
			if (Boards && Cmd.Text.Text.find(F) != std::string::npos)
			{
				return Cmd.Text.Text;
			}
		}
	}
	return std::string();
}

/** RiverLine's own lobby (or another of its pages) on the laptop (FH-3a): RiverLine in front, the page open, eased in. */
void HostsOpenRiverLine(HostsLaptop& P, ss::ui::RiverLine::Page Pg)
{
	P.S.Life.Update2LinesRead |= ss::life::U2Sites;
	P.RL.SiteShown = ss::net::SiteId::RiverLine;
	P.RL.OpenApp(ss::ui::RiverLine::App::RiverLine, P.Now);
	P.S.ShowLobby();
	P.RL.OpenPage(Pg, P.Now);
	P.Now = ui_test::Step(P.S, P.Now, 1.5);
}

/** The wire's festival item worked out from the edition's dates alone (U15, R-32): the Buildup counts down, Live counts the days. */
std::string HostsWantWire(const ss::fest::Festival& F, const std::string& Short, int Day)
{
	if (Day >= F.FirstDay - ss::fest::kBuildupDays && Day < F.FirstDay)
	{
		return Short + " \xC2\xB7 OPENS IN " + std::to_string(F.FirstDay - Day) + "D";
	}
	if (Day >= F.FirstDay && Day <= F.LastDay)
	{
		return Short + " \xC2\xB7 DAY " + std::to_string(Day - F.FirstDay + 1) + " OF " + std::to_string(F.LastDay - F.FirstDay + 1);
	}
	return std::string();
}

/** FH-5c: NetMoney's form (RiverLine's money: whole dollars from $1,000 or on a whole dollar, else the cents). */
std::string HostsNetMoney(ss::Chips Cents)
{
	return Cents >= 100000 || Cents % 100 == 0 ? HostsWhole(Cents) : ss::Money(Cents);
}

/** FH-5c: the wire's overlay items as the test reads them (§2.4.4, U26), from the network's own figures (site::show::OverlayOf) and
 *  U26's three forms: while F is the edition Live on its site, its guaranteed headliners (featured, or its Main) that started in the
 *  last day or start in the next twelve hours; the paid overlay once registration closed, else the projection short, else over (the
 *  projected entries' pool past the guarantee); the two latest starts, in start order. The oracle NetTickerOverlays answers to. */
std::vector<std::string> HostsWantOverlays(const ss::fest::Festival& F, double World)
{
	std::vector<std::string> Out;
	const ss::fest::Festival* Live = ss::fest::LiveOn(F.Site, ss::net::DayOf(World));
	if (!Live || Live->SeriesId != F.SeriesId)
	{
		return Out;
	}
	const ss::net::Network& Net = ss::net::Site(F.Site);
	std::vector<std::pair<double, std::string>> All;
	for (const ss::net::EventInstance& E : Net.Window(World - ss::net::MinutesPerDay, World + 12.0 * 60.0))
	{
		const ss::net::EventTemplate& T = Net.TemplateOf(E);
		if (T.Series != F.SeriesId || T.GtdCents <= 0 || !(T.Featured || T.Id == F.MainId))
		{
			continue;
		}
		const ss::site::show::OverlayView V = ss::site::show::OverlayOf(Net, E, World);
		if (!V.Shown)
		{
			continue;
		}
		const std::string Name = HostsShortName(T.Name);
		const ss::Chips Over = static_cast<ss::Chips>(std::llround(V.Entries * static_cast<double>(T.BuyInCents) * Net.Definition().Rake.PrizeShare(T))) - T.GtdCents;
		if (V.Actual && V.Cents > 0)
		{
			All.push_back({E.Start, "RiverLine added " + HostsNetMoney(V.Cents) + " to cover " + Name + "'s guarantee."});
		}
		else if (!V.Actual && V.Cents > 0)
		{
			All.push_back({E.Start, Name + " is projected " + HostsNetMoney(V.Cents) + " short."});
		}
		else if (!V.Actual && Over > 0)
		{
			All.push_back({E.Start, Name + " is projected " + HostsNetMoney(Over) + " over."});
		}
	}
	std::stable_sort(All.begin(), All.end(), [](const std::pair<double, std::string>& A, const std::pair<double, std::string>& B) { return A.first < B.first; });
	for (size_t I = All.size() > 2 ? All.size() - 2 : 0; I < All.size(); ++I)
	{
		Out.push_back(All[I].second);
	}
	return Out;
}

/** FH-5c: a tracked kicker read back (NetSpaced draws a glyph at a time): every run on the baseline Base from X0 on, in x order, its
 *  spaces dropped. */
std::string HostsSpacedAt(const ss::ui::DrawList& L, float Base, float X0)
{
	std::vector<std::pair<float, std::string>> Glyphs;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && std::fabs(Cmd.Text.BaselineY - Base) < 1.0f && Cmd.Text.X >= X0 - 1.0f)
		{
			Glyphs.push_back({Cmd.Text.X, Cmd.Text.Text});
		}
	}
	std::stable_sort(Glyphs.begin(), Glyphs.end(), [](const std::pair<float, std::string>& A, const std::pair<float, std::string>& B) { return A.first < B.first; });
	std::string Out;
	for (const std::pair<float, std::string>& G : Glyphs)
	{
		for (const char Ch : G.second)
		{
			Out += Ch == ' ' ? std::string() : std::string(1, Ch);
		}
	}
	return Out;
}

/** FH-5c: S with its spaces dropped. */
std::string HostsNoSpaces(const std::string& S)
{
	std::string Out;
	for (const char Ch : S)
	{
		Out += Ch == ' ' ? std::string() : std::string(1, Ch);
	}
	return Out;
}

/** FH-5c: edition F's Main Event champion on the world's roll (world::FestRoll), nullptr while it holds none. */
const ss::world::Honor* HostsRollMain(const ss::world::World& W, const ss::fest::Festival& F)
{
	for (const ss::world::FestRollRow& Row : ss::world::FestRoll(W, F.Which))
	{
		if (Row.Key == F.SeriesId && Row.Main)
		{
			return Row.Main;
		}
	}
	return nullptr;
}

/** FH-5c: the laptop's world caught up to its clock, and moved on past edition F's crown when the crown is still ahead; F's crown minute
 *  (-1 while the world hasn't crowned it). */
double HostsAfterCrown(HostsLaptop& P, const ss::fest::Festival& F)
{
	P.S.DrainWorld();
	double Crowned = ss::world::FestCrownedAt(P.S.Living(), F.SeriesId);
	if (Crowned > P.S.WorldMinutes())
	{
		P.At(Crowned + 60.0);
		P.S.DrainWorld();
		Crowned = ss::world::FestCrownedAt(P.S.Living(), F.SeriesId);
	}
	return Crowned;
}

/**
 * FestAfterglowHosts, its RiverLine half (§2.1.4, §2.4.4, §2.7.4; U13, U15, U16, U26; F-hubs FH-5c), on FestHonestLines' career at
 * RCOP 2026's Live day: (1) every three hours from the day before each RiverLine edition through 2027 to the day after it, the wire's
 * overlay items (rlnet_detail::NetTickerOverlays) equal the test's own reading (HostsWantOverlays: OverlayOf in U26's forms, none
 * outside its days), and at the first of those minutes still ahead in RCOP 2026's days the lobby's wire draws them; (2) two days after
 * LastDay, once
 * the world has crowned RCOP 2026 and in its Afterglow: the wire's festival item is U15's "RCOP CHAMPION · {NAME}" (the Main's champion
 * on the roll), nothing in the Closing before the crown and nothing once the three days are over, and slide 0 takes its Afterglow form
 * (the kicker's champion line, "Crowned {Date}." and the champion's prize).
 * Plants: the wire's U26 items in v1.0's "paid {Overlay} over its guarantee"; the slide's Afterglow form dropped; the wire's Afterglow
 * item kept to the Closing's "".
 */
void FestAfterglowWire(HostsLaptop& P, const ss::fest::Festival& Rcop, const ss::net::SeriesInfo& Rc)
{
	namespace rn = ss::ui::rlnet_detail;
	const auto Began = std::chrono::steady_clock::now();
	int Bad = 0, Added = 0, Shorts = 0, Overs = 0, Items = 0, Editions = 0;
	std::vector<double> Ahead; // RCOP 2026's minutes still ahead of the laptop's clock whose wire has an overlay item
	for (const ss::fest::Festival& Ed : ss::fest::All())
	{
		if (Ed.Site != ss::net::SiteId::RiverLine || Ed.FirstDay > Rcop.FirstDay + 450)
		{
			continue;
		}
		++Editions;
		for (int Day = Ed.FirstDay - 1; Day <= Ed.LastDay + 1; ++Day)
		{
			for (int Hour = 0; Hour < 24; Hour += 3)
			{
				const double World = static_cast<double>(Day) * ss::net::MinutesPerDay + static_cast<double>(Hour) * 60.0 + 30.0;
				const std::vector<std::string> Got = rn::NetTickerOverlays(World);
				const std::vector<std::string> Want = HostsWantOverlays(Ed, World);
				if (&Ed == &Rcop && !Got.empty() && World > P.S.WorldMinutes() && Day <= Rcop.LastDay)
				{
					Ahead.push_back(World);
				}
				if (Got != Want && ++Bad <= 3)
				{
				std::string G, Wt;
				for (const std::string& S : Got)
				{
					G += "[" + S + "]";
				}
				for (const std::string& S : Want)
				{
					Wt += "[" + S + "]";
				}
				ui_test::Expect(false, ("FestAfterglowHosts: " + Ed.Name + "'s day " + std::to_string(Day) + " " + std::to_string(Hour) + ":30's wire overlays read " + G + ", not " + Wt).c_str());
				}
				for (const std::string& S : Got)
				{
					++Items;
					Added += S.find(" to cover ") != std::string::npos ? 1 : 0;
					Shorts += S.size() > 7 && S.compare(S.size() - 7, 7, " short.") == 0 ? 1 : 0;
					Overs += S.size() > 6 && S.compare(S.size() - 6, 6, " over.") == 0 ? 1 : 0;
				}
			}
		}
	}
	ui_test::Expect(Bad == 0, "FestAfterglowHosts: the wire's overlay items are U26's forms of OverlayOf's figures, the edition's headliners only, on its days only");
	ui_test::Expect(Items > 0 && Added + Shorts + Overs == Items, "FestAfterglowHosts: the wire carries RiverLine's festivals' overlay items in U26's forms through their days");
	// Drawn at the first minute still ahead in RCOP 2026's days whose wire has an overlay item.
	ui_test::Expect(!Ahead.empty(), "FestAfterglowHosts: RCOP 2026's days ahead of the career's clock have a minute with an overlay item on the wire");
	if (!Ahead.empty())
	{
		P.At(Ahead.front());
	}
	HostsOpenRiverLine(P, ss::ui::RiverLine::Page::Lobby);
	const ss::ui::DrawList Lobby = P.Settled();
	int Drawn = 0;
	for (const std::string& S : rn::NetTickerOverlays(std::floor(P.S.WorldMinutes()))) // the wire reads its whole minute
	{
		Drawn += ui_test::DrewRun(Lobby, S) ? 1 : 0;
		ui_test::Expect(ui_test::DrewRun(Lobby, S), ("FestAfterglowHosts: the lobby's wire draws \"" + S + "\"").c_str());
	}
	ui_test::Expect(Drawn > 0, "FestAfterglowHosts: RCOP's lobby draws an overlay item on its wire");

	// RCOP 2026's Afterglow: two days after LastDay at noon, past the crown.
	P.At(static_cast<double>(Rcop.LastDay + 2) * ss::net::MinutesPerDay + 12.0 * 60.0);
	const double Crowned = HostsAfterCrown(P, Rcop);
	const ss::world::World& W = P.S.Living();
	const double At = P.S.WorldMinutes();
	const ss::world::Honor* Champ = HostsRollMain(W, Rcop);
	ui_test::Expect(Crowned > 0.0 && ss::fest::PhaseOf(Rcop, At, &W) == ss::fest::Phase::Afterglow && Champ,
		"FestAfterglowHosts: the world has crowned RCOP 2026 two days after its last day, its Main's champion on the roll, and it is in its Afterglow");
	if (!(Crowned > 0.0 && Champ))
	{
		return;
	}
	const std::string WantWire = Rc.Short + " CHAMPION \xC2\xB7 " + HostsUpper(Champ->Name);
	std::string Series;
	const std::string Wire = rn::NetTickerFest(At, Series);
	ui_test::Expect(Wire == WantWire && Series == Rcop.SeriesId, ("FestAfterglowHosts: the wire's Afterglow item reads \"" + WantWire + "\", not \"" + Wire + "\"").c_str());
	const ss::world::Honor* Held = nullptr;
	ui_test::Expect(rn::NetAfterglow(At, Held) == &Rcop && Held == Champ, "FestAfterglowHosts: RCOP 2026 is RiverLine's edition in its Afterglow, with its Main's champion");
	std::string Before, After;
	const std::string Closing = ss::net::DayOf(Crowned - 30.0) > Rcop.LastDay ? rn::NetTickerFest(Crowned - 30.0, Before) : std::string();
	const std::string Over = rn::NetTickerFest(Crowned + static_cast<double>(ss::fest::kAfterglowDays) * ss::net::MinutesPerDay + 60.0, After);
	ui_test::Expect(Closing.empty() && Over != WantWire && After != Rcop.SeriesId,
		("FestAfterglowHosts: no champion on the wire in the Closing before the crown (\"" + Closing + "\") nor once the Afterglow is over (\"" + Over + "\")").c_str());
	HostsOpenRiverLine(P, ss::ui::RiverLine::Page::Lobby);
	const std::string Sub = "Crowned " + ss::fest::DateInSentence(ss::net::DayOf(Crowned), ss::net::DayOf(At)) + ".";
	const std::string Prize = HostsNetMoney(Champ->Prize);
	const std::string Kicker = HostsNoSpaces(WantWire);
	bool WireDrawn = false, SubDrawn = false, KickerDrawn = false, PrizeDrawn = false;
	for (int Turn = 0; Turn < 12; ++Turn)
	{
		P.Now += 8.0;
		P.Frame();
		P.Now += 1.0;
		const ss::ui::DrawList L = P.Frame();
		WireDrawn = WireDrawn || ui_test::DrewRun(L, WantWire);
		const bool Slide = ui_test::DrewRun(L, Sub);
		SubDrawn = SubDrawn || Slide;
		KickerDrawn = KickerDrawn || (Slide && HostsSpacedAt(L, 126.0f, 60.0f).find(Kicker) != std::string::npos);
		PrizeDrawn = PrizeDrawn || (Slide && ui_test::DrewRun(L, Prize));
	}
	ui_test::Expect(WireDrawn, ("FestAfterglowHosts: the lobby's wire draws \"" + WantWire + "\"").c_str());
	ui_test::Expect(SubDrawn && KickerDrawn && PrizeDrawn, ("FestAfterglowHosts: slide 0's Afterglow form draws \"" + WantWire + "\" over " + Rc.Name + ", \"" + Sub + "\" and " + Prize +
		" (" + (SubDrawn ? "" : "no crown date ") + (KickerDrawn ? "" : "no champion line ") + (PrizeDrawn ? "" : "no prize") + ")").c_str());
	std::printf("FestAfterglowHosts (wire): %d overlay items over %d RiverLine editions' days (%d added, %d short, %d over), %d drawn on RCOP 2026's day %d; crowned day %d; %s; "
				"%s; %.2f s\n",
		Items, Editions, Added, Shorts, Overs, Drawn, Ahead.empty() ? -1 : ss::net::DayOf(Ahead.front()) - Rcop.FirstDay + 1, ss::net::DayOf(Crowned), WantWire.c_str(), Sub.c_str(),
		std::chrono::duration<double>(std::chrono::steady_clock::now() - Began).count());
}

/**
 * FestAfterglowHosts, its RunItTwice half (§2.1.4, U15; PLAN_FHUBS Q23; F-hubs FH-5c), on the same career: RunItTwice's first edition
 * whose Afterglow is still ahead, two days after its last day (past its crown): the tape's festival is it in its Afterglow, its item
 * U15's "{SHORT} CHAMPION · {NAME}" (the Main's champion on the roll) drawn on THE BOARD's tape, and in the Closing before the crown
 * the tape's festival is Closing (no champion yet).
 * Plant: the tape's Afterglow item dropped.
 */
void FestAfterglowTape(HostsLaptop& P)
{
	namespace rr = ss::ui::rlrit_detail;
	const int Today = ss::net::DayOf(P.S.WorldMinutes());
	const ss::fest::Festival* F = nullptr;
	for (const ss::fest::Festival& E : ss::fest::All())
	{
		if (!F && E.Site == ss::net::SiteId::RunItTwice && E.LastDay + 2 > Today)
		{
			F = &E;
		}
	}
	ui_test::Expect(F != nullptr, "FestAfterglowHosts: RunItTwice has an edition ahead of the career's clock");
	if (!F)
	{
		return;
	}
	P.At(static_cast<double>(F->LastDay + 2) * ss::net::MinutesPerDay + 12.0 * 60.0);
	const double Crowned = HostsAfterCrown(P, *F);
	const ss::world::World& W = P.S.Living();
	const double At = P.S.WorldMinutes();
	const ss::world::Honor* Champ = HostsRollMain(W, *F);
	const ss::net::SeriesInfo* Sr = ss::net::Site(F->Site).FindSeries(F->SeriesId);
	ui_test::Expect(Crowned > 0.0 && Champ && Sr && ss::fest::PhaseOf(*F, At, &W) == ss::fest::Phase::Afterglow,
		("FestAfterglowHosts: the world has crowned " + F->Name + ", its Main's champion on the roll, and it is in its Afterglow").c_str());
	if (!(Crowned > 0.0 && Champ && Sr))
	{
		return;
	}
	const std::string Want = Sr->Short + " CHAMPION \xC2\xB7 " + ss::ui::rlrit_detail::RitUpper(Champ->Name);
	ss::fest::Phase Ph = ss::fest::Phase::Off;
	ss::fest::Phase Early = ss::fest::Phase::Off;
	const ss::fest::Festival* OnTape = rr::RitTapeFest(At, Ph);
	const ss::fest::Festival* Before = rr::RitTapeFest(Crowned - 30.0, Early);
	ui_test::Expect(OnTape == F && Ph == ss::fest::Phase::Afterglow && rr::RitTapeChampion(*F) == Want,
		("FestAfterglowHosts: the tape's Afterglow item reads \"" + Want + "\", not \"" + rr::RitTapeChampion(*F) + "\"").c_str());
	ui_test::Expect(Before == F && Early != ss::fest::Phase::Afterglow, ("FestAfterglowHosts: before " + F->Name + "'s crown the tape's festival is not in its Afterglow").c_str());
	P.OpenRit(ss::ui::RiverLine::Page::Lobby);
	bool Drawn = false;
	for (int K = 0; K < 40 && !Drawn; ++K)
	{
		P.Now += 0.5;
		Drawn = ui_test::DrewRun(P.Frame(), Want);
	}
	ui_test::Expect(Drawn, ("FestAfterglowHosts: RunItTwice's tape draws \"" + Want + "\" in " + F->Name + "'s Afterglow").c_str());
	std::printf("FestAfterglowHosts (tape): %s crowned day %d; %s %s\n", F->Name.c_str(), ss::net::DayOf(Crowned), Want.c_str(), Drawn ? "drawn" : "not drawn");
}

void FestHonestDefenders(HostsLaptop& P); // FH-6b: below, after the YOUR strip's helpers it plays the hero's event with

/**
 * FestHonestLines (§2.15.2; F-hubs' stream B half, FH-3a; PLAN_G1 items 9 and 29): RiverLine's copy reads the sim and types no past.
 * Slide 0 prints RCOP 2026's $78,203,400 from SeriesInfo::GtdCents and the Main's buy-in and guarantee from its template
 * (FestRcopGtdSum's slide half); the taskbar's wire carries the festival from its Buildup, never on Night One (R-32); the Series board
 * says what scores (U25) with THE BELT for a festival and no prize panel for a plain series, the Season board claims no Platinum Pass
 * (FD-4); the Career goals read U24; the hub's RCOP Buildup panel prints U23 from step1..4 and the Main, its "Last year" only from the
 * roll. No frame drawn here holds a removed claim in its claim form. The lobby's cost on RCOP's day 8 against Night One's is printed
 * (PLAN_FHUBS FH-3's budget row: the lobby inside 29.9k + 2k; asserted at FH-7).
 * Plants: the old goal restored; Cents(1.0e8) kept on the slide.
 */
void FestHonestLines()
{
	namespace rn = ss::ui::rlnet_detail;
	namespace rf = ss::ui::rlfest_detail;
	const auto Began = std::chrono::steady_clock::now();
	const ss::net::Network& Net = ss::net::Site(ss::net::SiteId::RiverLine);
	const ss::net::SeriesInfo* Rc = Net.FindSeries("rcop");
	const ss::fest::Festival* Rcop = ss::fest::FestivalOf("rcop");
	ui_test::Expect(Rc && Rcop && Rcop->Which == ss::fest::Brand::Rcop && Rcop->Site == ss::net::SiteId::RiverLine, "FestHonestLines: RCOP 2026 is F-net's row on RiverLine");
	const ss::net::EventTemplate* Main = Rc ? Net.FindTemplate(Rc->MainEvent) : nullptr;
	const ss::net::EventTemplate* Step1 = Net.FindTemplate("step1");
	if (!Rc || !Rcop || !Main || !Step1)
	{
		ui_test::Expect(false, "FestHonestLines: RCOP 2026's series, Main and step 1 are RiverLine's");
		return;
	}
	const std::string Gtd = HostsWhole(Rc->GtdCents);
	ui_test::Expect(Gtd == "$78,203,400", ("FestHonestLines: RCOP 2026's guarantee is the sum of its templates' ($78,203,400), not " + Gtd).c_str());
	const std::string Sub = std::to_string(Rc->Events) + " events \xC2\xB7 " + ss::net::BuyIn(Main->BuyInCents) + " Main Event with " + HostsWhole(Main->GtdCents) + " guaranteed";

	// Night One (day 0): slide 0 over the carousel's turns; the wire keeps today's items.
	HostsLaptop P("u4-fest-hosts-rl");
	HostsOpenRiverLine(P, ss::ui::RiverLine::Page::Lobby);
	const ss::ui::DrawList Night = P.Settled();
	bool SlideGtd = false;
	bool SlideSub = false;
	std::string Claim;
	for (int Turn = 0; Turn < 12; ++Turn)
	{
		const ss::ui::DrawList L = Turn == 0 ? Night : (P.Now += 8.0, P.Frame());
		SlideGtd = SlideGtd || ui_test::DrewRun(L, Gtd);
		SlideSub = SlideSub || ui_test::DrewRun(L, Sub);
		Claim = Claim.empty() ? HostsClaim(L, false) : Claim;
	}
	ui_test::Expect(SlideGtd, ("FestHonestLines: slide 0 prints RCOP 2026's " + Gtd + " from SeriesInfo::GtdCents").c_str());
	ui_test::Expect(SlideSub, ("FestHonestLines: slide 0's line reads the Main's template: " + Sub).c_str());
	ui_test::Expect(Claim.empty(), ("FestHonestLines: Night One's lobby holds no removed claim (" + Claim + ")").c_str());
	ui_test::Expect(ss::net::DayOf(P.S.WorldMinutes()) < Rcop->FirstDay - ss::fest::kBuildupDays && !ui_test::Drew(Night, Rc->Short + " \xC2\xB7 "),
		"FestHonestLines: Night One's wire carries no festival item (the Buildup starts it, R-32)");

	// The wire's festival item on every day from Night One to the day after RCOP 2026 (U15's forms).
	int WireBad = 0;
	for (int Day = 0; Day <= Rcop->LastDay + 1; ++Day)
	{
		const double World = static_cast<double>(Day) * ss::net::MinutesPerDay + 12.0 * 60.0;
		std::string Series;
		const std::string Got = rn::NetTickerFest(World, Series);
		const std::string Want = HostsWantWire(*Rcop, Rc->Short, Day);
		if (Got != Want || (!Want.empty() && Series != Rcop->SeriesId))
		{
			++WireBad;
			ui_test::Expect(false, ("FestHonestLines: day " + std::to_string(Day) + "'s wire reads \"" + Got + "\", not \"" + Want + "\"").c_str());
		}
	}
	ui_test::Expect(WireBad == 0, "FestHonestLines: the wire counts RCOP 2026 down through its Buildup and its days while Live");

	// RCOP's Buildup (day FirstDay - 4): the wire's item drawn; the hub's RCOP Buildup panel (U23).
	const int BuildDay = Rcop->FirstDay - 4;
	P.At(static_cast<double>(BuildDay) * ss::net::MinutesPerDay + 12.0 * 60.0);
	HostsOpenRiverLine(P, ss::ui::RiverLine::Page::Lobby);
	const ss::ui::DrawList Soon = P.Settled();
	ui_test::Expect(ui_test::DrewRun(Soon, HostsWantWire(*Rcop, Rc->Short, BuildDay)), ("FestHonestLines: RCOP's Buildup puts " + HostsWantWire(*Rcop, Rc->Short, BuildDay) + " on the wire").c_str());
	{
		// The festival's item stands in for its series' own: on RCOP's days its series is the wire's, and it is named once.
		const double LiveAt = static_cast<double>(Rcop->FirstDay + 7) * ss::net::MinutesPerDay + 12.0 * 60.0;
		std::string LiveSeries;
		rn::NetTickerFest(LiveAt, LiveSeries);
		const ss::net::SeriesInfo* Cur = Net.CurrentSeries(LiveAt);
		ui_test::Expect(Cur && Cur->Id == Rcop->SeriesId && LiveSeries == Rcop->SeriesId && !rn::NetWireSeries(Net, LiveAt, LiveSeries) && rn::NetWireSeries(Net, LiveAt, std::string()) == Cur,
			"FestHonestLines: the festival's item stands in for its series' own (no second RCOP item on the wire)");
	}
	{
		ui_test::TableMeasurer M;
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, M, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, 1.0f);
		ss::ui::Ui U;
		U.Begin(C, P.Now);
		rf::RcopBuildupPanel(C, U, P.S, *Rcop, {1044.0f, 392.0f, 532.0f, 478.0f}, P.S.WorldMinutes(), P.Now, 5.0);
		U.End();
		const std::string Text = HostsText(L);
		const bool Sats = (P.S.Unlocks() & ss::net::UnlockSatellite) != 0;
		const std::string Lead = ss::net::BuyIn(Step1->BuyInCents) + " to a " + ss::net::BuyIn(Main->BuyInCents) + " seat.";
		const std::string Five = "Five steps to the Main Event and its " + HostsWhole(Main->GtdCents) + " guarantee." + (Sats ? " Step 1 runs every four hours." : " Make a final table to unlock satellites.");
		ui_test::Expect(ui_test::DrewRun(L, Lead), ("FestHonestLines: the RCOP Buildup panel reads " + Lead + " from step1 and the Main").c_str());
		ui_test::Expect(Text.find(Five) != std::string::npos, ("FestHonestLines: the RCOP Buildup panel reads \"" + Five + "\"").c_str());
		bool Buys = true;
		for (const char* Id : {"step1", "step2", "step3", "step4"})
		{
			const ss::net::EventTemplate* T = Net.FindTemplate(Id);
			Buys = Buys && T && ui_test::DrewRun(L, ss::net::BuyIn(T->BuyInCents));
		}
		ui_test::Expect(Buys && ui_test::DrewRun(L, ss::net::BuyIn(Main->BuyInCents)), "FestHonestLines: the ladder's buy-ins are step1..4's and the Main's");
		const std::string LastYear = HostsLastYearWant(P.S.Living(), *Rcop);
		ui_test::Expect(LastYear.empty() ? Text.find("Last year") == std::string::npos : Text.find(LastYear) != std::string::npos,
			(LastYear.empty() ? std::string("FestHonestLines: the RCOP Buildup panel draws no \"Last year\" while the roll holds no RCOP Main of last year")
							  : "FestHonestLines: the RCOP Buildup panel reads \"" + LastYear + "\" from the roll").c_str());
		ui_test::Expect(HostsClaim(L, true).empty(), ("FestHonestLines: the RCOP Buildup panel holds no typed past (" + HostsClaim(L, true) + ")").c_str());
	}

	// RCOP Live (day FirstDay + 7): the wire's day; the boards; the Career goals; the lobby's cost against Night One's.
	const int LiveDay = Rcop->FirstDay + 7;
	P.At(static_cast<double>(LiveDay) * ss::net::MinutesPerDay + 12.0 * 60.0);
	HostsOpenRiverLine(P, ss::ui::RiverLine::Page::Lobby);
	const ss::ui::DrawList Live = P.Settled();
	ui_test::Expect(ui_test::DrewRun(Live, HostsWantWire(*Rcop, Rc->Short, LiveDay)), ("FestHonestLines: RCOP Live puts " + HostsWantWire(*Rcop, Rc->Short, LiveDay) + " on the wire").c_str());
	ui_test::Expect(HostsClaim(Live, false).empty(), ("FestHonestLines: RCOP's lobby holds no removed claim (" + HostsClaim(Live, false) + ")").c_str());
	HostsOpenRiverLine(P, ss::ui::RiverLine::Page::Leaderboards);
	P.RL.ShowBoard(ss::net::Board::Series, P.Now);
	const ss::ui::DrawList SeriesBoard = P.Settled();
	const ss::fest::Festival* NextRcop = rf::FestNextEdition(*Rcop);
	const std::string Belt = "Points from every cash at " + Rc->Name + "." +
		(ss::fest::kCrownTickets && NextRcop ? " The leader takes the belt and a seat in the next " + ss::fest::NameAfterArticle(NextRcop->Name) + " Main Event." : std::string(" The leader takes the belt."));
	ui_test::Expect(HostsText(SeriesBoard).find(Belt) != std::string::npos, ("FestHonestLines: RCOP's Series board reads \"" + Belt + "\" (U25)").c_str());
	int SeatLines = 0;
	for (const ss::ui::DrawCmd& Cmd : SeriesBoard.Cmds)
	{
		SeatLines += Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == "A seat in the next Main Event" ? 1 : 0;
	}
	ui_test::Expect(SeatLines == (NextRcop ? 3 : 0), "FestHonestLines: THE BELT puts the crown's seat beside the belt, the podium and FINALIST");
	ui_test::Expect(HostsClaim(SeriesBoard, true).empty(), ("FestHonestLines: RCOP's Series board claims no prize (" + HostsClaim(SeriesBoard, true) + ")").c_str());
	P.RL.ShowBoard(ss::net::Board::Season, P.Now);
	const ss::ui::DrawList Season = P.Settled();
	ui_test::Expect(HostsClaim(Season, true).empty() && !ui_test::Drew(Season, "A seat in the next"), ("FestHonestLines: the Season board claims no Platinum Pass or prize (" + HostsClaim(Season, true) + ")").c_str());
	HostsOpenRiverLine(P, ss::ui::RiverLine::Page::Career);
	const ss::ui::DrawList Career = P.Settled();
	const std::string CareerText = HostsText(Career);
	const std::string SeatGoal = "Win a " + ss::net::BuyIn(Main->BuyInCents) + " seat to the RCOP Main Event.";
	const std::string CareerLast = HostsLastYearWant(P.S.Living(), *Rcop);
	const std::string WinGoal = "Win the RCOP Main Event." + (CareerLast.empty() ? std::string() : " " + CareerLast);
	ui_test::Expect(CareerText.find(SeatGoal) != std::string::npos, ("FestHonestLines: the Career goal reads \"" + SeatGoal + "\" (U24)").c_str());
	ui_test::Expect(CareerText.find(WinGoal) != std::string::npos && (!CareerLast.empty() || CareerText.find("Last year") == std::string::npos),
		("FestHonestLines: the Career goal reads \"" + WinGoal + "\" (U24, \"Last year\" only from the roll)").c_str());
	ui_test::Expect(HostsClaim(Career, false).empty(), ("FestHonestLines: the Career page holds no typed past (" + HostsClaim(Career, false) + ")").c_str());
	// FH-5c: the wire's overlay items through RCOP's days, then RCOP 2026's Afterglow on slide 0 and the wire (two days after LastDay).
	FestAfterglowWire(P, *Rcop, *Rc);

	// A plain series after RCOP: its Series board says what scores and draws no prize panel.
	const ss::net::SeriesInfo* Plain = nullptr;
	for (const ss::net::SeriesInfo& Sr : Net.Series())
	{
		if (!Sr.Festival && Sr.FirstDay > Rcop->LastDay + 1 && Sr.FirstDay < Rcop->LastDay + 120 && (!Plain || Sr.FirstDay < Plain->FirstDay))
		{
			Plain = &Sr;
		}
	}
	ui_test::Expect(Plain != nullptr, "FestHonestLines: RiverLine runs a plain series after RCOP 2026");
	if (Plain)
	{
		P.At(static_cast<double>(Plain->FirstDay) * ss::net::MinutesPerDay + 12.0 * 60.0);
		HostsOpenRiverLine(P, ss::ui::RiverLine::Page::Leaderboards);
		P.RL.ShowBoard(ss::net::Board::Series, P.Now);
		const ss::ui::DrawList PlainBoard = P.Settled();
		const ss::net::SeriesInfo* Shown = Net.CurrentSeries(P.S.WorldMinutes());
		const std::string Want = "Points from every cash at " + (Shown ? Shown->Name : Plain->Name) + ".";
		const std::string Text = HostsText(PlainBoard);
		ui_test::Expect(Shown && !Shown->Festival && Text.find(Want) != std::string::npos && Text.find("The leader takes the belt") == std::string::npos,
			("FestHonestLines: a plain series' board reads \"" + Want + "\" (U25) and nothing more").c_str());
		ui_test::Expect(!ui_test::Drew(PlainBoard, "A seat in the next") && HostsClaim(PlainBoard, true).empty(), "FestHonestLines: a plain series' board draws no prize panel");
	}
	std::printf("FestHonestLines: slide 0 %s; the lobby %zu vertices / %zu runs on Night One, %zu / %zu on RCOP's day %d (%+lld; the lobby inside 29.9k + 2k, FH-7); "
				"plain series %s; %.2f s\n",
		Gtd.c_str(), Night.Vertices.size(), HostsRuns(Night), Live.Vertices.size(), HostsRuns(Live), LiveDay - Rcop->FirstDay + 1,
		static_cast<long long>(Live.Vertices.size()) - static_cast<long long>(Night.Vertices.size()), Plain ? Plain->Name.c_str() : "none",
		std::chrono::duration<double>(std::chrono::steady_clock::now() - Began).count());
	// FH-5c: RunItTwice's first edition's Afterglow on the tape; FH-6b: the true defenders, the pin wall's band and the trophy case, on the
	// same career a year on.
	FestAfterglowTape(P);
	FestHonestDefenders(P);
}
/** FH-4B: one of edition F's runs on F's site at the laptop's clock: Kind 0 a numbered event (its series F's) in play now (late
 *  registration, running, the final table), 1 a numbered event still taking entries later today, 2 a satellite of F (Festival::
 *  Satellites) still taking entries inside F's seat window; the earliest such instance, false when there is none. */
bool HostsStripRun(const ss::fest::Festival& F, double World, int Kind, ss::net::EventInstance& Out, const std::string& Except = std::string())
{
	const ss::net::Network& Net = ss::net::Site(F.Site);
	bool Found = false;
	for (const ss::net::EventTemplate& T : Net.Templates())
	{
		const bool Sat = std::find(F.Satellites.begin(), F.Satellites.end(), T.Id) != F.Satellites.end();
		if (Kind == 2 ? !Sat : T.Series != F.SeriesId)
		{
			continue;
		}
		ss::net::EventInstance E;
		if (!Net.Next(T.Id, Kind == 0 ? World - 12.0 * 60.0 : World, E))
		{
			continue;
		}
		const ss::net::Status St = Net.Live(E, World).St;
		const int Day = ss::net::DayOf(E.Start);
		const bool Ok = Kind == 0 ? (St == ss::net::Status::LateReg || St == ss::net::Status::Running || St == ss::net::Status::FinalTable)
			: Kind == 1			  ? (St == ss::net::Status::Registering || St == ss::net::Status::Announced) && Day == ss::net::DayOf(World)
								  : (St == ss::net::Status::Registering || St == ss::net::Status::Announced) && Day >= F.FirstDay - ss::fest::kAnnounceDays && Day <= F.MainDay;
		if (Ok && E.Id != Except && (!Found || E.Start < Out.Start))
		{
			Out = E;
			Found = true;
		}
	}
	return Found;
}

/** FH-4B: the first plain series event (a series that is no festival edition) on Site starting after World, false when none. */
bool HostsStripPlain(ss::net::SiteId Site, double World, ss::net::EventInstance& Out)
{
	const ss::net::Network& Net = ss::net::Site(Site);
	bool Found = false;
	for (const ss::net::EventTemplate& T : Net.Templates())
	{
		ss::net::EventInstance E;
		if (T.Series.empty() || ss::fest::FestivalOf(T.Series) || !Net.Next(T.Id, World, E))
		{
			continue;
		}
		if (!Found || E.Start < Out.Start)
		{
			Out = E;
			Found = true;
		}
	}
	return Found;
}

/** FH-4B: U10's line without its race clause as FestYourStrip expects it from fest::YourLine's numbers (written here, apart from
 *  rlnet_detail::NetYourCounts), and the race at the head's right ("" while the hero doesn't score). */
std::string HostsStripCounts(const ss::fest::YourLineView& V)
{
	std::string Want = (V.Events == 1 ? std::string("1 event") : ss::Grouped(V.Events) + " events") + " \xC2\xB7 " + (V.Cashes == 1 ? std::string("1 cash") : ss::Grouped(V.Cashes) + " cashes");
	return V.Best > 0 && V.Field > 0 ? Want + " \xC2\xB7 best " + ss::Ordinal(V.Best) + " of " + ss::Grouped(V.Field) : Want;
}

std::string HostsStripRace(const ss::fest::YourLineView& V)
{
	return V.Rank > 0 && V.Scorers > 0 ? "race " + ss::Ordinal(V.Rank) + " of " + ss::Grouped(V.Scorers) : std::string();
}

/** FH-4B: event E's register sheet on its host, settled: RiverLine's lobby with E selected (its event panel), Ignite's sheet over the
 *  floor, RunItTwice's Prospectus held on E (rlrit_detail::RitHoldSelection, which draws it off THE BOARD's window). Ignite's floor is
 *  drawn once before the sheet opens: the laptop's first Ignite frame claims its state for the session (RiverLine::IgniteFrame starts
 *  it afresh for a new owner), which would drop a sheet opened before it (fix round 1). */
ss::ui::DrawList HostsStripSheet(HostsLaptop& P, const ss::net::EventInstance& E)
{
	const ss::net::SiteId Site = ss::net::SiteOf(E.Id);
	if (Site == ss::net::SiteId::Ignite)
	{
		P.OpenPlay();
		P.Frame();
		ss::ui::rlignite_detail::IgniteOpenSheet(P.RL, E.Id, P.Now);
	}
	else if (Site == ss::net::SiteId::RunItTwice)
	{
		P.OpenRit(ss::ui::RiverLine::Page::Lobby);
		ss::ui::rlrit_detail::RitHoldSelection(P.RL, E.Id);
	}
	else
	{
		HostsOpenRiverLine(P, ss::ui::RiverLine::Page::Lobby);
		P.RL.SelectEvent(E.Id);
	}
	return P.Settled();
}

/** FH-4B: no line of a YOUR strip on L (the head "YOUR " + the Short, the race clause, U10's counts: a run naming events and cashes). */
bool HostsNoStrip(const ss::ui::DrawList& L, const std::string& Head)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		const std::string& T = Cmd.Text.Text;
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && (T == Head || T.compare(0, 5, "race ") == 0 || (T.find(" event") != std::string::npos && T.find(" cash") != std::string::npos)))
		{
			return false;
		}
	}
	return true;
}

/** FH-4B (fix round 1): L is E's own register sheet, so a "no strip" on it tests something: the laptop's selection is E and the sheet
 *  draws a line only it draws: Ignite's sheet up (rlignite_detail::IgniteSheetOpen) with its "YOUR IGNITE BALANCE" line; RiverLine's
 *  event panel with E's sub line ("NL Hold'em · {Speed} · {N}-min levels · {N}-max"); RunItTwice's Prospectus with E's "RIT-{HHMM}". */
bool HostsOnSheet(const HostsLaptop& P, const ss::net::EventInstance& E, const ss::ui::DrawList& L)
{
	const ss::net::SiteId Site = ss::net::SiteOf(E.Id);
	if (P.RL.SelectedEvent() != E.Id)
	{
		return false;
	}
	if (Site == ss::net::SiteId::Ignite)
	{
		return ss::ui::rlignite_detail::IgniteSheetOpen(&P.RL) && ui_test::DrewRun(L, "YOUR IGNITE BALANCE");
	}
	if (Site == ss::net::SiteId::RunItTwice)
	{
		const int Min = static_cast<int>(std::fmod(E.Start, ss::net::MinutesPerDay));
		return ui_test::DrewRun(L, "RIT-" + std::to_string(Min / 60 * 100 + Min % 60));
	}
	const ss::net::EventTemplate& T = ss::net::Site(Site).TemplateOf(E);
	return ui_test::DrewRun(L, std::string(T.Omaha ? "PL Omaha" : "NL Hold'em") + " \xC2\xB7 " + T.Speed + " \xC2\xB7 " + std::to_string(static_cast<int>(T.LevelMinutes)) + "-min levels \xC2\xB7 " +
		std::to_string(T.TableSize) + "-max");
}

/** FH-4B: the runs drawn as Lines cross no other text run on L from the first run reading After on ("" from the first run; Ignite's
 *  sheet sits over its floor): each run's box, from its top to a fifth of its size under its baseline and as wide as its measured text,
 *  meets no other run's. Hit names the first crossing. */
bool HostsStripClear(const ss::ui::DrawList& L, const std::vector<std::string>& Lines, const std::string& After, std::string& Hit)
{
	ui_test::TableMeasurer M;
	std::vector<const ss::ui::TextItem*> Runs;
	bool On = After.empty();
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text)
		{
			On = On || Cmd.Text.Text == After;
			if (On && !Cmd.Text.Text.empty())
			{
				Runs.push_back(&Cmd.Text);
			}
		}
	}
	const auto Meets = [&M](const ss::ui::TextItem& A, const ss::ui::TextItem& B) {
		const float Ax1 = A.X + M.Width(A.Text, A.Face, A.SizePx);
		const float Bx1 = B.X + M.Width(B.Text, B.Face, B.SizePx);
		return A.X < Bx1 && B.X < Ax1 && A.Y < B.BaselineY + 0.2f * B.SizePx && B.Y < A.BaselineY + 0.2f * A.SizePx;
	};
	for (const ss::ui::TextItem* S : Runs)
	{
		if (std::find(Lines.begin(), Lines.end(), S->Text) == Lines.end())
		{
			continue;
		}
		for (const ss::ui::TextItem* O : Runs)
		{
			if (O != S && O->Text != S->Text && Meets(*S, *O))
			{
				Hit = "\"" + S->Text + "\" meets \"" + O->Text + "\"";
				return false;
			}
		}
	}
	return true;
}

/** FH-4B: the hero plays Id to Place for Prize through the world's public calls (World::HeroEntered, HeroFinished; PLAN_FHUBS
 *  Corrections item 15), the world drained first as the session drains it before telling a finish. */
void HostsStripPlay(HostsLaptop& P, const std::string& Id, int Place, ss::Chips Prize)
{
	P.S.DrainWorld();
	ss::world::World& W = P.S.Living();
	W.HeroEntered(Id, std::string());
	W.HeroFinished(Id, Place, Prize, ss::world::World::TableReport());
	P.Now = ui_test::Step(P.S, P.Now, 0.2);
}

/** FestYourStrip's sheets half on one host (§2.8.2; FH-4B): edition F on its first day at 15:00: before the hero plays F, a numbered
 *  event's sheet draws no strip (that list checked to be the event's own sheet first, HostsOnSheet: fix round 1); after a cash in one of F's events in play, the same sheet and a satellite's draw "YOUR {SHORT}", U10's
 *  line in its singular ("1 event · 1 cash · best ...") and the race at the head's right while the hero scores, from fest::YourLine;
 *  a plain series event's sheet draws none. Second (Plural): an unpaid finish makes it "2 events · 1 cash". */
void FestYourStripHost(HostsLaptop& P, const ss::fest::Festival& F, bool Plural, std::string& Report)
{
	namespace rn = ss::ui::rlnet_detail;
	const ss::net::Network& Net = ss::net::Site(F.Site);
	const ss::net::SeriesInfo* Sr = Net.FindSeries(F.SeriesId);
	const std::string Head = "YOUR " + HostsUpper(Sr ? Sr->Short : F.Name);
	P.At(static_cast<double>(F.FirstDay) * ss::net::MinutesPerDay + 15.0 * 60.0);
	ss::net::EventInstance Playing;
	ss::net::EventInstance Later;
	ss::net::EventInstance Sat;
	const bool HasPlaying = HostsStripRun(F, P.S.WorldMinutes(), 0, Playing);
	const bool HasLater = HostsStripRun(F, P.S.WorldMinutes(), 1, Later);
	const bool HasSat = HostsStripRun(F, P.S.WorldMinutes(), 2, Sat);
	ui_test::Expect(HasPlaying && HasLater && HasSat, ("FestYourStrip (sheets): " + F.Name + " has an event in play, one later today and a satellite on its first day").c_str());
	if (!HasPlaying || !HasLater || !HasSat)
	{
		return;
	}
	ui_test::Expect(rn::NetSheetEdition(Net, Later) == &F && rn::NetSheetEdition(Net, Sat) == &F, ("FestYourStrip (sheets): " + F.Name + "'s event and satellite belong to it").c_str());
	const ss::ui::DrawList Before = HostsStripSheet(P, Later);
	ui_test::Expect(HostsOnSheet(P, Later, Before), ("FestYourStrip (sheets): before the hero plays " + F.Name + ", " + Net.NameOf(Later) + "'s own sheet is up").c_str());
	ui_test::Expect(!ss::fest::YourLine(F, &P.S.Living()).Shown && HostsNoStrip(Before, Head),
		("FestYourStrip (sheets): before the hero plays " + F.Name + ", " + Net.NameOf(Later) + "'s sheet draws no YOUR strip").c_str());
	HostsStripPlay(P, Playing.Id, 3, std::max<ss::Chips>(100, Net.TemplateOf(Playing).BuyInCents * 6));
	const ss::fest::YourLineView V = ss::fest::YourLine(F, &P.S.Living());
	ui_test::Expect(V.Shown && V.Events == 1 && V.Cashes == 1 && V.Best == 3, ("FestYourStrip (sheets): the hero's cash in " + Playing.Id + " gives " + F.Name + "'s pin").c_str());
	const std::string Counts = HostsStripCounts(V);
	const std::string Race = HostsStripRace(V);
	ui_test::Expect(Counts.compare(0, 17, "1 event \xC2\xB7 1 cash") == 0, ("FestYourStrip (sheets): U10 is singular at one (" + Counts + ")").c_str());
	const ss::ui::DrawList After = HostsStripSheet(P, Later);
	ui_test::Expect(ui_test::DrewRun(After, Head) && ui_test::DrewRun(After, Counts) && (Race.empty() ? !ui_test::Drew(After, "race ") : ui_test::DrewRun(After, Race)),
		("FestYourStrip (sheets): " + Net.NameOf(Later) + "'s sheet draws " + Head + (Race.empty() ? std::string() : " | " + Race) + " | " + Counts).c_str());
	const std::string Anchor = F.Site == ss::net::SiteId::Ignite ? std::string("YOUR IGNITE BALANCE") : std::string();
	std::string Hit;
	const bool ClearLater = HostsStripClear(After, {Head, Race, Counts}, Anchor, Hit);
	ui_test::Expect(ClearLater, ("FestYourStrip (sheets): the strip on " + Net.NameOf(Later) + "'s sheet crosses no other line (" + Hit + ")").c_str());
	const ss::ui::DrawList OnSat = HostsStripSheet(P, Sat);
	ui_test::Expect(ui_test::DrewRun(OnSat, Head) && ui_test::DrewRun(OnSat, Counts), ("FestYourStrip (sheets): the satellite " + Net.NameOf(Sat) + "'s sheet draws the strip").c_str());
	const bool ClearSat = HostsStripClear(OnSat, {Head, Race, Counts}, Anchor, Hit);
	ui_test::Expect(ClearSat, ("FestYourStrip (sheets): the strip on " + Net.NameOf(Sat) + "'s sheet crosses no other line (" + Hit + ")").c_str());
	ss::net::EventInstance Plain;
	const bool HasPlain = HostsStripPlain(F.Site, P.S.WorldMinutes(), Plain);
	ui_test::Expect(HasPlain, ("FestYourStrip (sheets): " + ss::net::Def(F.Site).Name + " runs a plain series event after " + F.Name).c_str());
	if (HasPlain)
	{
		const ss::ui::DrawList OnPlain = HostsStripSheet(P, Plain);
		ui_test::Expect(HostsOnSheet(P, Plain, OnPlain), ("FestYourStrip (sheets): the plain series event " + Net.NameOf(Plain) + "'s own sheet is up").c_str());
		ui_test::Expect(rn::NetSheetEdition(Net, Plain) == nullptr && HostsNoStrip(OnPlain, Head),
			("FestYourStrip (sheets): the plain series event " + Net.NameOf(Plain) + "'s sheet draws no YOUR strip").c_str());
	}
	std::string Second;
	if (Plural)
	{
		ss::net::EventInstance Again;
		P.At(P.S.WorldMinutes() + 3.0 * 60.0);
		if (HostsStripRun(F, P.S.WorldMinutes(), 0, Again, Playing.Id))
		{
			HostsStripPlay(P, Again.Id, std::max(40, ss::net::Site(F.Site).Live(Again, P.S.WorldMinutes()).Entries), 0);
			const ss::fest::YourLineView V2 = ss::fest::YourLine(F, &P.S.Living());
			Second = HostsStripCounts(V2);
			ss::net::EventInstance Next;
			const ss::ui::DrawList L2 = HostsStripSheet(P, HostsStripRun(F, P.S.WorldMinutes(), 2, Next) ? Next : Sat);
			ui_test::Expect(V2.Events == 2 && V2.Cashes == 1 && Second.compare(0, 18, "2 events \xC2\xB7 1 cash") == 0 && ui_test::DrewRun(L2, Second),
				("FestYourStrip (sheets): an unpaid finish makes it plural (" + Second + ")").c_str());
		}
		else
		{
			ui_test::Expect(false, ("FestYourStrip (sheets): a second " + F.Name + " event in play three hours on").c_str());
		}
	}
	Report += " " + F.Name + ": " + Head + (Race.empty() ? std::string() : " | " + Race) + " | " + Counts + (Second.empty() ? std::string() : " then " + Second) + ";";
}

/** U37 and the wallets (§2.8.2, §2.16 step 8a, LEAD_CALLS R-18; FH-4B): on PENNANT 2027's Step 1 day 8 (day 183, Tue Apr 6) with a
 *  Step 2 ticket held, the Step 2 event's sheet on Ignite reads "Good on Apr 7 and Apr 9." under its ticket line, Ignite's wallet
 *  lists the ticket as a row (STEP 2, PENNANT 2027, TICKET, U37), and a plain event's sheet reads no U37; on REIGN 2027's day 2
 *  RunItTwice's THE CLUB lists a held REIGN step ticket apart from its WELCOME TICKETS (STEP {K}, REIGN 2027 · Good on ...), never
 *  under them. Plants: U37 keeping a run that no longer takes entries; the step ticket left among the welcome tickets. */
void FestYourStripTickets(HostsLaptop& P, std::string& Report)
{
	namespace rn = ss::ui::rlnet_detail;
	const ss::fest::Festival* Pennant = nullptr;
	const ss::fest::Festival* Reign = nullptr;
	for (const ss::fest::Festival& F : ss::fest::All())
	{
		Pennant = !Pennant && F.Which == ss::fest::Brand::Pennant ? &F : Pennant;
		Reign = !Reign && F.Which == ss::fest::Brand::Reign ? &F : Reign;
	}
	ui_test::Expect(Pennant && Reign, "FestYourStrip (U37): F-net's calendar holds PENNANT and REIGN");
	if (!Pennant || !Reign)
	{
		return;
	}
	// A step's ticket: a one-off satellite of the edition seating another one-off's ticket (Step 1 seats Step 2's).
	const auto StepTwo = [](const ss::fest::Festival& F, std::string& Ticket, std::string& RunId) {
		const ss::net::Network& Net = ss::net::Site(F.Site);
		for (const std::string& Id : F.Satellites)
		{
			const ss::net::EventTemplate* S1 = Net.FindTemplate(Id);
			for (const std::string& Up : F.Satellites)
			{
				const ss::net::EventTemplate* S2 = Net.FindTemplate(Up);
				if (S1 && S2 && S1->OnlyDay != -9999 && S2->OnlyDay != -9999 && S1->SeatTicket == Net.TicketOf(Up) && Ticket.empty())
				{
					Ticket = S1->SeatTicket;
					RunId = Up;
				}
			}
		}
		return !Ticket.empty();
	};
	std::string PTicket;
	std::string PRun;
	ui_test::Expect(StepTwo(*Pennant, PTicket, PRun), "FestYourStrip (U37): PENNANT 2027 has a Step 2 ticket");
	if (PTicket.empty())
	{
		return;
	}
	const ss::net::Network& Ig = ss::net::Site(ss::net::SiteId::Ignite);
	P.At(183.0 * ss::net::MinutesPerDay + 12.0 * 60.0);
	ui_test::Expect(ss::net::DateLabel(183) == "Apr 6", ("FestYourStrip (U37): day 183 is Apr 6, not " + ss::net::DateLabel(183)).c_str());
	P.S.Life.Tickets[PTicket] = 1;
	// The Step 2 runs taking the ticket, from the calendar: the days of those still taking entries.
	std::vector<int> Days;
	ss::net::EventInstance Run;
	bool HasRun = false;
	for (const std::string& Id : Pennant->Satellites)
	{
		const ss::net::EventTemplate* R = Ig.FindTemplate(Id);
		ss::net::EventInstance E;
		if (R && R->OnlyDay != -9999 && Ig.TicketOf(Id) == PTicket && Ig.Next(Id, static_cast<double>(R->OnlyDay) * ss::net::MinutesPerDay, E))
		{
			Days.push_back(ss::net::DayOf(E.Start));
			if (!HasRun || E.Start < Run.Start)
			{
				Run = E;
				HasRun = true;
			}
		}
	}
	std::sort(Days.begin(), Days.end());
	std::string Want = "Good on ";
	for (size_t I = 0; I < Days.size(); ++I)
	{
		Want += (I == 0 ? "" : I + 1 == Days.size() ? " and " : ", ") + ss::net::DateLabel(Days[I]);
	}
	Want += ".";
	ui_test::Expect(HasRun && Want == "Good on Apr 7 and Apr 9.", ("FestYourStrip (U37): PENNANT's Step 2 runs on Apr 7 and Apr 9 (" + Want + ", §2.16 step 8a)").c_str());
	if (!HasRun)
	{
		return;
	}
	const ss::ui::DrawList Sheet = HostsStripSheet(P, Run);
	ui_test::Expect(ui_test::DrewRun(Sheet, Want) && ui_test::Drew(Sheet, "1 ticket held"), ("FestYourStrip (U37): the Step 2 event's sheet on Ignite reads " + Want + " under its ticket line").c_str());
	std::string Hit;
	const bool ClearSheet = HostsStripClear(Sheet, {Want}, "YOUR IGNITE BALANCE", Hit);
	ui_test::Expect(ClearSheet, ("FestYourStrip (U37): U37 on the sheet crosses no other line (" + Hit + ")").c_str());
	// g1p-hub item 14 (§2.16 step 8a, R-18; PIE s08a_step2_ticket_your): the hub's YOUR panel lists the ticket too, here before the hero's
	// first PENNANT event (no pin: U11's line, then U37 under it), in the panel's body {1044, 392, 532, 478}. Plant: the panel returning
	// after U11 with no U37 (the old early return). The sheet above is closed first (Ignite's side state forgotten, as the wallet's half does).
	{
		ss::ui::rlignite_detail::IgniteForget(&P.RL);
		// The moments this career earned on its way (FEIGN 2026's mark) are seen first: an overlay holds the pointer over the hub.
		while (P.S.PeekFestMoment())
		{
			P.S.DoneFestMoment();
		}
		P.OpenPlay();
		P.RL.ShowSeries(std::string(), P.Now);
		P.RL.OpenPage(ss::ui::RiverLine::Page::Series, P.Now);
		P.Settled();
		const float Tw = (532.0f - 12.0f) / 3.0f;
		const ss::ui::DrawList Your = P.Press(1044.0f + 2.0f * (Tw + 6.0f) + Tw / 2.0f, 366.0f);
		const bool NoPin = !ss::fest::YourLine(*Pennant, &P.S.Living()).Shown;
		const std::string U11 = "Play any " + ss::fest::NameAfterArticle(Pennant->Name) + " event for your pin.";
		float U37Y = -1.0f;
		for (const ss::ui::DrawCmd& Cmd : Your.Cmds)
		{
			U37Y = Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == Want && Cmd.Text.X >= 1044.0f && Cmd.Text.BaselineY > 392.0f && Cmd.Text.BaselineY < 870.0f ? Cmd.Text.BaselineY : U37Y;
		}
		ui_test::Expect(NoPin && ui_test::DrewRun(Your, U11) && U37Y > 0.0f,
			("FestYourStrip (U37): the hub's YOUR panel on Ignite reads " + Want + " under U11 before the hero's first " + Pennant->Name + " event (no pin: " + (NoPin ? "yes" : "no") + ")").c_str());
	}
	ss::net::EventInstance Plain;
	bool HasPlain = false;
	for (const ss::net::EventTemplate& T : Ig.Templates())
	{
		ss::net::EventInstance E;
		if (!HasPlain && T.Series.empty() && T.Fmt != ss::net::Format::Satellite && Ig.Next(T.Id, P.S.WorldMinutes() + 60.0, E))
		{
			Plain = E;
			HasPlain = true;
		}
	}
	if (HasPlain)
	{
		P.S.Life.Tickets[Ig.TicketOf(Ig.TemplateOf(Plain).Id)] = 1;
		const ss::ui::DrawList PlainSheet = HostsStripSheet(P, Plain);
		ui_test::Expect(HostsOnSheet(P, Plain, PlainSheet) && !ui_test::Drew(PlainSheet, "Good on "), ("FestYourStrip (U37): a plain event's sheet with its own ticket held reads no U37 (" + Ig.NameOf(Plain) + ")").c_str());
		P.S.Life.Tickets.erase(Ig.TicketOf(Ig.TemplateOf(Plain).Id));
	}
	ss::ui::rlignite_detail::IgniteForget(&P.RL);
	P.OpenPlay();
	P.RL.OpenPage(ss::ui::RiverLine::Page::Cashier, P.Now);
	const ss::ui::DrawList Wallet = P.Settled();
	ui_test::Expect(ui_test::DrewRun(Wallet, "STEP 2") && ui_test::DrewRun(Wallet, Pennant->Name) && ui_test::DrewRun(Wallet, "TICKET") && ui_test::DrewRun(Wallet, Want),
		("FestYourStrip (U37): Ignite's wallet lists the Step 2 ticket as a row: STEP 2, " + Pennant->Name + ", TICKET, " + Want).c_str());
	const bool ClearWallet = HostsStripClear(Wallet, {"STEP 2", Pennant->Name, "TICKET", Want}, "RECEIPTS", Hit);
	ui_test::Expect(ClearWallet, ("FestYourStrip (U37): the wallet's ticket row crosses no other line (" + Hit + ")").c_str());
	// Apr 8, after Step 2's first run has closed: one day left.
	P.At(185.0 * ss::net::MinutesPerDay + 12.0 * 60.0);
	const std::string Left = "Good on " + ss::net::DateLabel(Days.back()) + ".";
	const ss::ui::DrawList Later = P.Settled();
	ui_test::Expect(Days.size() == 2 && ui_test::DrewRun(Later, Left) && !ui_test::DrewRun(Later, Want), ("FestYourStrip (U37): on Apr 8 the wallet's row reads " + Left).c_str());
	P.S.Life.Tickets.erase(PTicket);
	Report += " U37 " + Want + " then " + Left + " (Ignite's sheet and wallet);";

	// RunItTwice: a REIGN step ticket on THE CLUB, on REIGN 2027's day 2.
	std::string RTicket;
	std::string RRun;
	ui_test::Expect(StepTwo(*Reign, RTicket, RRun), "FestYourStrip (U37): REIGN 2027 has a Step 2 ticket");
	if (RTicket.empty())
	{
		return;
	}
	P.At(static_cast<double>(Reign->FirstDay + 1) * ss::net::MinutesPerDay + 12.0 * 60.0);
	P.S.Life.Tickets[RTicket] = 1;
	const ss::fest::Festival* Ed = nullptr;
	std::string Good;
	const int Step = rn::NetStepTicket(ss::net::SiteId::RunItTwice, RTicket, P.S.WorldMinutes(), Ed, Good);
	P.OpenRit(ss::ui::RiverLine::Page::Promotions);
	const ss::ui::DrawList Club = P.Settled();
	// What the welcome list would print for it (the list's own rule: the ticket's template's short name, else the key past its site).
	const ss::net::EventTemplate* RT = ss::net::Site(ss::net::SiteId::RunItTwice).FindTemplate(RTicket);
	const std::string AsWelcome = RT ? HostsShortName(RT->Name) : RTicket.substr(RTicket.find('.') + 1);
	const std::string Line = Reign->Name + (Good.empty() ? std::string() : " \xC2\xB7 " + Good);
	ui_test::Expect(Step == 2 && Ed == Reign && !Good.empty(), ("FestYourStrip (U37): REIGN's Step 2 ticket is a step ticket on its day 2 (" + Good + ")").c_str());
	ui_test::Expect(ui_test::DrewRun(Club, "STEP 2") && ui_test::DrewRun(Club, Line) && !ui_test::DrewRun(Club, AsWelcome) && ui_test::DrewRun(Club, "None held right now."),
		("FestYourStrip (U37): THE CLUB lists REIGN's Step 2 ticket apart (STEP 2 | " + Line + "), never as the welcome ticket " + AsWelcome).c_str());
	const bool ClearClub = HostsStripClear(Club, {"STEP 2", Line}, std::string(), Hit);
	ui_test::Expect(ClearClub, ("FestYourStrip (U37): THE CLUB's step ticket row crosses no other line (" + Hit + ")").c_str());
	// The Step 2 event's own sheet on RunItTwice: its Prospectus, held on the run, reads U37 under the festival lines.
	const ss::net::Network& Rn = ss::net::Site(ss::net::SiteId::RunItTwice);
	const ss::net::EventTemplate* RunT = Rn.FindTemplate(RRun);
	ss::net::EventInstance RE;
	const bool HasRE = RunT && Rn.Next(RRun, static_cast<double>(RunT->OnlyDay) * ss::net::MinutesPerDay, RE);
	ui_test::Expect(HasRE, "FestYourStrip (U37): REIGN's Step 2 run is on the calendar");
	if (HasRE)
	{
		const ss::ui::DrawList Pro = HostsStripSheet(P, RE);
		const bool ClearPro = HostsStripClear(Pro, {Good}, std::string(), Hit);
		ui_test::Expect(P.RL.SelectedEvent() == RE.Id && ui_test::DrewRun(Pro, Good) && ClearPro,
			("FestYourStrip (U37): the Step 2 run's Prospectus reads " + Good + " clear of every other line (" + Hit + ")").c_str());
	}
	P.S.Life.Tickets.erase(RTicket);
	Report += " THE CLUB: STEP 2 | " + Line + ";";
}

/** FestYourStrip's sheets half (§2.15.2, §2.8.2; F-hubs' stream B, FH-4B): YOUR {FESTIVAL} on each host's register sheet, from
 *  fest::YourLine, the same numbers as the hub's panel (stream A's half): RCOP 2026 on RiverLine's event panel (with the plural after a
 *  second, unpaid finish), FEIGN 2026 on Ignite's event sheet, WRIT 2027 on RunItTwice's Prospectus; then U37 and the wallets (R-18).
 *  One laptop walks the year. Plants: the strip on a plain series event (the SPEC's); U10 plural at one; U37 keeping a closed run; the
 *  step ticket left among RunItTwice's welcome tickets. */
void FestYourStripSheets()
{
	const auto Began = std::chrono::steady_clock::now();
	const ss::fest::Festival* Rcop = ss::fest::FestivalOf("rcop");
	const ss::fest::Festival* Feign = ss::fest::FestivalOf("ignite.feign26");
	const ss::fest::Festival* Writ = HostsRitEdition(ss::fest::Brand::Writ);
	ui_test::Expect(Rcop && Feign && Writ, "FestYourStrip (sheets): F-net's calendar holds RCOP 2026, FEIGN 2026 and WRIT 2027");
	if (!Rcop || !Feign || !Writ)
	{
		return;
	}
	HostsLaptop P("u4-fest-hosts-strip");
	std::string Report;
	FestYourStripHost(P, *Rcop, true, Report);
	FestYourStripHost(P, *Feign, false, Report);
	FestYourStripHost(P, *Writ, false, Report);
	FestYourStripTickets(P, Report);
	std::printf("FestYourStrip (sheets):%s %.2f s\n", Report.c_str(), std::chrono::duration<double>(std::chrono::steady_clock::now() - Began).count());
}

/** FH-6b: brand B's edition of Year (nullptr when the calendar has none). */
const ss::fest::Festival* HostsEditionOf(ss::fest::Brand B, int Year)
{
	for (const ss::fest::Festival& F : ss::fest::All())
	{
		if (F.Which == B && F.Year == Year)
		{
			return &F;
		}
	}
	return nullptr;
}

/** FH-6b: the edition E's champion defends, the earliest of E's brand opening after E's last day: the oracle's own walk of fest::All()
 *  (never rlfest_detail::FestNextEdition, which the code under test reads). */
const ss::fest::Festival* HostsAfter(const ss::fest::Festival& E)
{
	const ss::fest::Festival* Best = nullptr;
	for (const ss::fest::Festival& G : ss::fest::All())
	{
		if (G.Which == E.Which && G.FirstDay > E.LastDay && (!Best || G.FirstDay < Best->FirstDay))
		{
			Best = &G;
		}
	}
	return Best;
}

/** FH-6b: U21's {Festival} worked out from the edition: its name less " {Year}". */
std::string HostsBrandWord(const ss::fest::Festival& F)
{
	const std::string Tail = " " + std::to_string(F.Year);
	return F.Name.size() > Tail.size() && F.Name.compare(F.Name.size() - Tail.size(), Tail.size(), Tail) == 0 ? F.Name.substr(0, F.Name.size() - Tail.size()) : F.Name;
}

/** FH-6b: who defends at a world minute, from the roll and the calendar alone (§2.6.3): a roll row's Main champion (a person) while the
 *  brand's next edition is Announced, building up, Live or on its Main day, so up to its crown. Unsure: the Main champion of a row whose
 *  next edition is Closing (after its last event, before its crown, the world's latest roll entry takes over: no assertion either way).
 *  People: every person on any roll (Main, belt, Mini, Micro). */
struct HostsDefenders
{
	std::map<int, const ss::fest::Festival*> Defends;
	std::vector<int> People;
	std::vector<int> Unsure;
};
HostsDefenders HostsDefendersAt(const ss::world::World& W, double World)
{
	using ss::fest::Phase;
	HostsDefenders D;
	for (int B = static_cast<int>(ss::fest::Brand::None) + 1; B < static_cast<int>(ss::fest::Brand::Count); ++B)
	{
		for (const ss::world::FestRollRow& Row : ss::world::FestRoll(W, static_cast<ss::fest::Brand>(B)))
		{
			for (const ss::world::Honor* H : {Row.Main, Row.Crown, Row.Mini, Row.Micro})
			{
				if (H && H->Npc >= 0)
				{
					D.People.push_back(H->Npc);
				}
			}
			const ss::fest::Festival* E = ss::fest::FestivalOf(Row.Key);
			const ss::fest::Festival* Next = E ? HostsAfter(*E) : nullptr;
			const Phase Ph = Next ? ss::fest::PhaseOf(*Next, World, &W) : Phase::Off;
			if (!Row.Main || Row.Main->Npc < 0)
			{
				continue;
			}
			if (Ph == Phase::Closing)
			{
				D.Unsure.push_back(Row.Main->Npc);
			}
			else if (Ph == Phase::Announced || Ph == Phase::Buildup || Ph == Phase::Live || Ph == Phase::MainDay)
			{
				D.Defends[Row.Main->Npc] = Next;
			}
		}
	}
	for (std::vector<int>* V : {&D.People, &D.Unsure})
	{
		std::sort(V->begin(), V->end());
		V->erase(std::unique(V->begin(), V->end()), V->end());
	}
	return D;
}

/** FH-6b: the honorific line a person's card must read (§2.6.3, U21): their top title by the title order (F-world's FestTopTitle), as
 *  "Defending {Festival} champion" when the oracle has them defending that Main, "{Festival} {Year} champion" for another Main title,
 *  "{Festival} {Year} Player of the Series" for a belt; "" for none and for a Mini or Micro title. */
std::string HostsHonorificWant(const ss::world::World& W, int Npc, const HostsDefenders& D)
{
	const ss::world::Honor* Top = ss::world::FestTopTitle(W, Npc);
	ss::world::FestSlot Slot = ss::world::FestSlot::None;
	const ss::fest::Festival* F = Top ? ss::world::FestOfHonor(*Top, Slot) : nullptr;
	if (!F)
	{
		return std::string();
	}
	if (Slot == ss::world::FestSlot::Crown)
	{
		return HostsBrandWord(*F) + " " + std::to_string(F->Year) + " Player of the Series";
	}
	if (Slot != ss::world::FestSlot::Main)
	{
		return std::string();
	}
	const auto It = D.Defends.find(Npc);
	return It != D.Defends.end() && It->second == HostsAfter(*F) ? "Defending " + HostsBrandWord(*F) + " champion" : HostsBrandWord(*F) + " " + std::to_string(F->Year) + " champion";
}

/** FH-6b: any run in an honorific's form (U21's three) on a frame: the first such text, "" when none. */
std::string HostsHonorificRun(const ss::ui::DrawList& L)
{
	auto Ends = [](const std::string& S, const std::string& Tail) { return S.size() >= Tail.size() && S.compare(S.size() - Tail.size(), Tail.size(), Tail) == 0; };
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		const std::string& T = Cmd.Text.Text;
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && (T.compare(0, 10, "Defending ") == 0 || Ends(T, " champion") || Ends(T, " Player of the Series")))
		{
			return T;
		}
	}
	return std::string();
}

/** FH-6b: on a frame, whether the first run reading Name has a DEFENDING CHAMPION run beside it on its row (to its right, its baseline
 *  within the name's height: PlayerName's one pill); Found false when no run reads Name. */
bool HostsPillBeside(const ss::ui::DrawList& L, const std::string& Name, bool& Found)
{
	Found = false;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text || Cmd.Text.Text != Name)
		{
			continue;
		}
		Found = true;
		for (const ss::ui::DrawCmd& Pc : L.Cmds)
		{
			if (Pc.Type == ss::ui::DrawCmd::Kind::Text && Pc.Text.Text == "DEFENDING CHAMPION" && Pc.Text.X > Cmd.Text.X && Pc.Text.X < Cmd.Text.X + 700.0f &&
				std::fabs(Pc.Text.BaselineY - Cmd.Text.BaselineY) < 14.0f)
			{
				return true;
			}
		}
		return false;
	}
	return false;
}

/**
 * FestHonestLines' true-defender half (§2.6.3, §2.6.4, §2.8.7; F-hubs FH-6b; PLAN_FHUBS Q16, accepted): DEFENDING CHAMPION (the card's
 * pill beside RIVAL, PlayerName's pill on the lobby's names) and the card's honorific line belong to true defenders only. On RCOP
 * 2027's third day (Live: RCOP 2026's Main champion defends; WRIT 2028 not yet Announced, so WRIT 2027's does not) and on WRIT 2028's
 * second Announced day (RCOP 2027 crowned: RCOP 2026's champion no longer defends, WRIT 2027's does), every person on any roll is held to
 * HostsDefendersAt (the roll and the calendar alone): rlnet_detail::NetFestDefends, NetFestHonorific, and their card drawn (the pill; the
 * honorific line from the title order in U21's words, "Defending {Festival} champion" only for the defended Main, a belt's "Player of the
 * Series", none for a Mini or Micro title); the boards' rows and an event panel's Players tab (PlayerName) carry the pill beside a
 * defender's name and no one else's. The Career page on the first day: RiverLine Rewards keeps the pin wall's band while the wall is
 * hidden (the hero has played no festival event) and yields it after the hero's first (an RCOP 2027 event); on the second, the trophy case
 * shows the hero's festival pieces from their Honors (planted through World::Read: FEIGN 2026's Main a festival trophy, RCOP 2026's belt,
 * PENNANT 2027's Mini a title object counted as one; the Championship Online 2027's Main stays its bracelet, RCOP 2026's Mini has no
 * title object), by date, with U20's count line; the Career page's cost before and after is printed (the page within its measured cost
 * + 4k, FH-7).
 * Plants: the pill for any roll champion; the pill from the Honor on (no Announced phase); "Defending" for any Main title; the
 * Championship Online's Main counted as a festival trophy too; the Rewards card kept under the wall.
 */
void FestHonestDefenders(HostsLaptop& P)
{
	namespace rn = ss::ui::rlnet_detail;
	namespace world = ss::world;
	using ss::fest::Brand;
	const auto Began = std::chrono::steady_clock::now();
	const ss::fest::Festival* R26 = HostsEditionOf(Brand::Rcop, 2026);
	const ss::fest::Festival* R27 = HostsEditionOf(Brand::Rcop, 2027);
	if (!R26 || !R27)
	{
		ui_test::Expect(false, "FestHonestLines (defenders): RCOP 2026 and RCOP 2027 are in fest::All()");
		return;
	}
	// The first day: FEIGN 2027's second Announced day while RCOP 2027 is Live (else RCOP 2027's third day). The second: WRIT 2028's second
	// Announced day (RCOP 2027 and FEIGN 2027 crowned weeks before), so WRIT 2027's champion defends.
	const ss::fest::Festival* Feign26 = HostsEditionOf(Brand::Feign, 2026);
	const ss::fest::Festival* Feign27 = HostsEditionOf(Brand::Feign, 2027);
	const ss::fest::Festival* Writ28 = HostsEditionOf(Brand::Writ, 2028);
	const int FeignOn = Feign27 ? Feign27->FirstDay - ss::fest::kAnnounceDays + 1 : -1;
	const int Days[2] = {FeignOn >= R27->FirstDay + 2 && FeignOn < R27->LastDay ? FeignOn : R27->FirstDay + 2,
		Writ28 && Writ28->FirstDay - ss::fest::kAnnounceDays + 1 > R27->LastDay + 3 ? Writ28->FirstDay - ss::fest::kAnnounceDays + 1 : R27->LastDay + 3};
	int Planted = -1;
	int Cards = 0, CardPills = 0, CardLines = 0, Helper = 0, NameRows = 0, NamePills = 0;
	std::string Report;
	ss::ui::DrawList Before, After;
	for (int K = 0; K < 2; ++K)
	{
		P.At(static_cast<double>(Days[K]) * ss::net::MinutesPerDay + 12.0 * 60.0);
		HostsOpenRiverLine(P, ss::ui::RiverLine::Page::Lobby);
		const world::World& W = P.S.Living();
		const double Now = P.S.WorldMinutes();
		const int Day = ss::net::DayOf(Now);
		const std::string On = " (day " + std::to_string(Day) + ")";
		if (K == 0 && Feign26 && Day == FeignOn)
		{
			// PlayerName's pill on a real row: when this world's FEIGN 2026 roll holds no Main (no person won it), its Main title is planted
			// (World::Read's honor line, as the world keeps titles) for the Money List's fourth, a row PlayerName draws, so FEIGN 2027's
			// Announced phase makes them a defender on the board.
			bool HasMain = false;
			for (const world::FestRollRow& Row : world::FestRoll(W, Brand::Feign))
			{
				HasMain = HasMain || (Row.Key == Feign26->SeriesId && Row.Main);
			}
			const std::vector<ss::net::BoardRow> Money = ss::net::Shared().Leaderboard(ss::net::Board::Earnings, Now, ss::net::HeroStats(), 60, nullptr);
			const int Fourth = Money.size() > 3 ? Money[3].Player : -1;
			if (!HasMain && Fourth >= 0 && W.Get(Fourth) && !ss::net::Shared().Players()[static_cast<size_t>(Fourth)].Rival)
			{
				P.S.DrainWorld();
				char AtText[32];
				std::snprintf(AtText, sizeof(AtText), "%.1f", Feign26->MainStart + 600.0);
				P.S.Living().Read({"world", "honor", "2026", Feign26->Name + " Main Event", Feign26->MainId + "@" + std::to_string(static_cast<long long>(Feign26->MainStart)),
					std::to_string(Fourth), W.Get(Fourth)->Name, "2500000", "1400", AtText});
				Planted = Fourth;
			}
		}
		const HostsDefenders D = HostsDefendersAt(W, Now);
		auto Unsure = [&D](int Npc) { return std::binary_search(D.Unsure.begin(), D.Unsure.end(), Npc); };
		int Champions = 0;
		for (int Npc : D.People)
		{
			Champions += D.Defends.count(Npc) == 0 && !Unsure(Npc) ? 1 : 0;
		}
		ui_test::Expect(!D.Defends.empty() && Champions > 0, ("FestHonestLines (defenders): the roll holds a defender and a champion who is not one" + On).c_str());
		const world::Honor* Last = nullptr;
		for (const world::FestRollRow& Row : world::FestRoll(W, Brand::Rcop))
		{
			Last = Row.Key == R26->SeriesId ? Row.Main : Last;
		}
		ui_test::Expect(Last && Last->Npc >= 0 && (D.Defends.count(Last->Npc) == 1) == (K == 0),
			(std::string("FestHonestLines (defenders): RCOP 2026's Main champion ") + (K == 0 ? "defends while RCOP 2027 is Live" : "no longer defends once RCOP 2027 is crowned") + On).c_str());
		// Every person on a roll: the helpers, then their card. A card is drawn for each defender and up to twelve others a day.
		int Others = 0;
		for (int Npc : D.People)
		{
			if (Unsure(Npc))
			{
				continue;
			}
			const auto Def = D.Defends.find(Npc);
			const bool Want = Def != D.Defends.end();
			const world::Npc* N = W.Get(Npc);
			const std::string Who = (N ? N->Name : std::string("npc ") + std::to_string(Npc)) + On;
			const ss::fest::Festival* Got = rn::NetFestDefends(W, Npc, Now);
			const std::string Line = HostsHonorificWant(W, Npc, D);
			const std::string GotLine = rn::NetFestHonorific(W, Npc, Now);
			ui_test::Expect((Got != nullptr) == Want && (!Want || Got == Def->second), ("FestHonestLines (defenders): " + Who + (Want ? " defends " + Def->second->Name : std::string(" defends nothing")) +
				", NetFestDefends reads " + (Got ? Got->Name : std::string("none"))).c_str());
			ui_test::Expect(!Want || world::IsDefending(W, Npc, Day), ("FestHonestLines (defenders): " + Who + " is world::IsDefending's").c_str());
			ui_test::Expect(GotLine == Line, ("FestHonestLines (defenders): " + Who + "'s honorific reads \"" + Line + "\", not \"" + GotLine + "\"").c_str());
			++Helper;
			if (!Want && Others >= 12)
			{
				continue;
			}
			Others += Want ? 0 : 1;
			P.RL.ShowPlayer(Npc, P.Now);
			const ss::ui::DrawList Card = P.Settled();
			const bool Pill = ui_test::DrewRun(Card, "DEFENDING CHAMPION");
			const std::string Drawn = HostsHonorificRun(Card);
			ui_test::Expect(P.RL.CardOpen() && Pill == Want, ("FestHonestLines (defenders): " + Who + "'s card " + (Want ? "carries" : "carries no") + " DEFENDING CHAMPION").c_str());
			ui_test::Expect(Line.empty() ? Drawn.empty() : ui_test::DrewRun(Card, Line),
				("FestHonestLines (defenders): " + Who + "'s card draws the honorific line \"" + Line + "\" (drawn: \"" + Drawn + "\")").c_str());
			P.RL.ShowPlayer(-1, P.Now);
			P.Settled();
			++Cards;
			CardPills += Pill ? 1 : 0;
			CardLines += Line.empty() ? 0 : 1;
		}
		// The boards' names (PlayerName): the pill beside a defender's name, beside no other roll champion's.
		HostsOpenRiverLine(P, ss::ui::RiverLine::Page::Leaderboards);
		for (ss::net::Board B : {ss::net::Board::Earnings, ss::net::Board::Season, ss::net::Board::Wins, ss::net::Board::FinalTables, ss::net::Board::Series})
		{
			P.RL.ShowBoard(B, P.Now);
			const ss::ui::DrawList Board = P.Settled();
			for (int Npc : D.People)
			{
				const world::Npc* N = W.Get(Npc);
				bool Found = false;
				const bool Pill = N && !Unsure(Npc) && HostsPillBeside(Board, N->Name, Found);
				if (!N || Unsure(Npc) || !Found)
				{
					continue;
				}
				const bool Want = D.Defends.count(Npc) > 0;
				ui_test::Expect(Pill == Want, ("FestHonestLines (defenders): board " + std::to_string(static_cast<int>(B)) + " draws " + N->Name + (Want ? " with" : " without") + " DEFENDING CHAMPION" + On).c_str());
				NameRows += Want ? 1 : 0;
				NamePills += Pill ? 1 : 0;
			}
		}
		// An event panel's Players tab (PlayerName): an event of the next hours with a defender among its first six regulars (the panel lists
		// the best known first), the tab opened by a press on its label; the pill beside the defender and beside no other roll champion.
		{
			const ss::net::Network& Net = ss::net::Site(ss::net::SiteId::RiverLine);
			world::World& Wm = P.S.Living();
			std::string Pick;
			for (const ss::net::EventInstance& E : Net.Window(Now - 3.0 * 60.0, Now + 20.0 * 60.0))
			{
				if (Net.Live(E, Now).St == ss::net::Status::Finished)
				{
					continue;
				}
				std::vector<int> Who = Wm.Registered(E.Id);
				std::stable_sort(Who.begin(), Who.end(), [&Wm](int A, int B) { return Wm.Get(A)->RepOf(world::Rep::Overall) > Wm.Get(B)->RepOf(world::Rep::Overall); });
				for (size_t I = 0; I < Who.size() && I < 6 && Pick.empty(); ++I)
				{
					Pick = D.Defends.count(Who[I]) ? E.Id : Pick;
				}
				if (!Pick.empty())
				{
					break;
				}
			}
			if (!Pick.empty())
			{
				HostsOpenRiverLine(P, ss::ui::RiverLine::Page::Lobby);
				P.RL.SelectEvent(Pick);
				const ss::ui::DrawList Sel = P.Settled();
				float Tx = -1.0f, Ty = -1.0f;
				for (const ss::ui::DrawCmd& Cmd : Sel.Cmds)
				{
					if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == "Players")
					{
						Tx = Cmd.Text.X;
						Ty = Cmd.Text.BaselineY;
					}
				}
				ui_test::Expect(Tx >= 0.0f, ("FestHonestLines (defenders): " + Pick + "'s panel has its Players tab" + On).c_str());
				P.Press(Tx + 12.0f, Ty - 5.0f);
				const ss::ui::DrawList Tab = P.Settled();
				for (int Npc : D.People)
				{
					const world::Npc* N = W.Get(Npc);
					bool Found = false;
					const bool Pill = N && !Unsure(Npc) && HostsPillBeside(Tab, N->Name, Found);
					if (!N || Unsure(Npc) || !Found)
					{
						continue;
					}
					const bool Want = D.Defends.count(Npc) > 0;
					ui_test::Expect(Pill == Want, ("FestHonestLines (defenders): " + Pick + "'s Players tab draws " + N->Name + (Want ? " with" : " without") + " DEFENDING CHAMPION" + On).c_str());
					NameRows += Want ? 1 : 0;
					NamePills += Pill ? 1 : 0;
				}
				P.RL.SelectEvent(std::string());
			}
			Report += Pick.empty() ? std::string() : " " + Pick + "'s Players tab;";
		}
		Report += " day " + std::to_string(Day) + ": " + std::to_string(D.Defends.size()) + " defending of " + std::to_string(D.People.size()) + " on the rolls;";
		if (K != 0)
		{
			continue;
		}
		// The Career page on RCOP 2027's third day: the wall hidden, RiverLine Rewards keeps its band.
		ui_test::Expect(world::HeroFestPins(W).empty(), "FestHonestLines (Career): this career's hero has played no festival event yet");
		HostsOpenRiverLine(P, ss::ui::RiverLine::Page::Career);
		const ss::ui::DrawList Hidden = P.Settled();
		ui_test::Expect(ui_test::Drew(Hidden, " of RiverLine rake this month"), "FestHonestLines (Career): RiverLine Rewards keeps the pin wall's band while the wall is hidden");
		ss::net::EventInstance Ev;
		const bool InPlay = HostsStripRun(*R27, Now, 0, Ev);
		ui_test::Expect(InPlay, "FestHonestLines (Career): an RCOP 2027 event is in play on its third day");
		if (InPlay)
		{
			HostsStripPlay(P, Ev.Id, std::max(1, Ev.Entries), 0);
		}
		ui_test::Expect(!world::HeroFestPins(P.S.Living()).empty(), "FestHonestLines (Career): the hero's first festival event gives them a pin");
		HostsOpenRiverLine(P, ss::ui::RiverLine::Page::Career);
		const ss::ui::DrawList Played = P.Settled();
		ui_test::Expect(!ui_test::Drew(Played, " of RiverLine rake this month"), "FestHonestLines (Career): once the wall shows, RiverLine Rewards yields its band");
	}
	// The hero's festival pieces on the second day, planted as the world keeps titles (World::Read's honor lines, Npc -1); the Career page
	// before and after them.
	const ss::fest::Festival* Pen27 = HostsEditionOf(Brand::Pennant, 2027);
	const ss::fest::Festival* Tco27 = HostsEditionOf(Brand::Tco, 2027);
	if (!Feign26 || !Pen27 || !Tco27 || Pen27->TitleIds.size() < 2 || R26->TitleIds.size() < 2)
	{
		ui_test::Expect(false, "FestHonestLines (Career): FEIGN 2026, PENNANT 2027 (with its Mini) and the Championship Online 2027 are in fest::All()");
	}
	else
	{
		HostsOpenRiverLine(P, ss::ui::RiverLine::Page::Career);
		Before = P.Settled();
		P.S.DrainWorld();
		auto Plant = [&P](int Year, const std::string& Title, const std::string& EventId, long long Prize, int Entries, double At) {
			char AtText[32];
			std::snprintf(AtText, sizeof(AtText), "%.1f", At);
			P.S.Living().Read({"world", "honor", std::to_string(Year), Title, EventId, "-1", P.S.HeroName, std::to_string(Prize), std::to_string(Entries), AtText});
		};
		auto Minute = [](double At) { return std::to_string(static_cast<long long>(At)); };
		const double BeltAt = static_cast<double>(R26->LastDay) * ss::net::MinutesPerDay + 23.0 * 60.0;
		const double PenAt = static_cast<double>(Pen27->FirstDay + 1) * ss::net::MinutesPerDay + 20.0 * 60.0;
		Plant(2026, R26->Name + " Player of the Series", R26->SeriesId + "#pos", 0, 1210, BeltAt);
		Plant(2026, R26->Name + " Mini Main Event", R26->TitleIds[1] + "@" + Minute(BeltAt - 600.0), 1200000, 900, BeltAt - 300.0);
		Plant(2026, Feign26->Name + " Main Event", Feign26->MainId + "@" + Minute(Feign26->MainStart), 2500000, 1400, Feign26->MainStart + 600.0);
		Plant(2027, Pen27->Name + " Mini Main Event", Pen27->TitleIds[1] + "@" + Minute(PenAt), 79532, 2766, PenAt + 300.0);
		Plant(2027, Tco27->Name + " Main Event", Tco27->MainId + "@" + Minute(Tco27->MainStart), 9000000, 5000, Tco27->MainStart + 600.0);
		const std::vector<std::pair<const world::Award*, const world::Honor*>> Case = rn::NetFestCase(P.S.Living().HeroAwards(), P.S.Living(), -1);
		std::vector<std::string> Pieces;
		for (const std::pair<const world::Award*, const world::Honor*>& Pc : Case)
		{
			Pieces.push_back(Pc.second ? Pc.second->EventId : std::string("award"));
		}
		const std::vector<std::string> WantTail = {R26->SeriesId + "#pos", Feign26->MainId + "@" + Minute(Feign26->MainStart), Pen27->TitleIds[1] + "@" + Minute(PenAt)};
		const bool Tail = Pieces.size() >= 3 && std::equal(WantTail.begin(), WantTail.end(), Pieces.end() - 3) && std::count(Pieces.begin(), Pieces.end(), std::string("award")) == static_cast<std::ptrdiff_t>(Pieces.size()) - 3;
		ui_test::Expect(Tail, "FestHonestLines (Career): the trophy case holds RCOP 2026's belt, FEIGN 2026's trophy and the Pennant Ribbon by date, never the Championship Online's Main (its bracelet) or RCOP's Mini (no title object)");
		int Bracelets = 0, Rings = 0;
		for (const world::Award& A : P.S.Living().HeroAwards())
		{
			Bracelets += A.Ring ? 0 : 1;
			Rings += A.Ring ? 1 : 0;
		}
		std::string Count = Bracelets > 0 ? std::to_string(Bracelets) + (Bracelets == 1 ? " bracelet" : " bracelets") : std::string();
		Count += Rings > 0 ? (Count.empty() ? "" : " \xC2\xB7 ") + std::to_string(Rings) + (Rings == 1 ? " ring" : " rings") : "";
		Count += (Count.empty() ? "" : " \xC2\xB7 ") + std::string("2 festival trophies \xC2\xB7 1 belt");
		HostsOpenRiverLine(P, ss::ui::RiverLine::Page::Career);
		After = P.Settled();
		ui_test::Expect(ui_test::DrewRun(After, Count), ("FestHonestLines (Career): the trophy case's count line reads \"" + Count + "\" (U20)").c_str());
		ui_test::Expect(!ui_test::Drew(After, " of RiverLine rake this month"), "FestHonestLines (Career): the trophy case's pieces leave the wall's band to the wall");
		Report += " the trophy case \"" + Count + "\";";
	}
	ui_test::Expect(Planted < 0 || NameRows > 0, "FestHonestLines (defenders): a board or a Players tab draws a defender's name (PlayerName's pill checked on a real row)");
	Report += Planted >= 0 ? " FEIGN 2026's Main planted for the Money List's fourth (npc " + std::to_string(Planted) + ");" : std::string(" no FEIGN 2026 Main planted (its roll holds one);");
	std::printf("FestHonestLines (defenders):%s %d roll people read through the helpers, %d cards drawn (%d with DEFENDING CHAMPION, %d with an honorific line), "
				"%d defender rows on the boards (%d pills); the Career page %zu vertices / %zu runs before the hero's festival pieces, %zu / %zu with them "
				"(%+lld; the page within its measured cost + 4k, FH-7); %.2f s\n",
		Report.c_str(), Helper, Cards, CardPills, CardLines, NameRows, NamePills, Before.Vertices.size(), HostsRuns(Before), After.Vertices.size(), HostsRuns(After),
		static_cast<long long>(After.Vertices.size()) - static_cast<long long>(Before.Vertices.size()), std::chrono::duration<double>(std::chrono::steady_clock::now() - Began).count());
}
} // namespace

namespace u4_ui_test
{
void SegFestHosts()
{
	FestHostsRouting();
	FestBandstandPick();
	FestTitleFits();
	FestSignFits();
	FestCopyUiHosts();
	FestRitCodesAndHeld();
	FestHonestLines();
	FestYourStripSheets();
}
} // namespace u4_ui_test
