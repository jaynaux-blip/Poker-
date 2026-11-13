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
#include "UiSeg.h"

#include "CopyLint.h"
#include "Trademarks.h"
#include "ShortStack/Game/Festival.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/UI/EventArt.h"
#include "ShortStack/UI/RiverLine.h"
#include "../../Source/ShortStackCore/Private/ShortStack/Game/SitesDetail.h"
#include "../../Source/ShortStackCore/Private/ShortStack/UI/RitArt.h"

#include <algorithm>
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
} // namespace rlnet_detail
namespace rlignite_detail
{
// UI/RiverLineIgnite.cpp (FH-3b): the bandstand's festival pick and the Lighthouse's festival line.
bool IgniteBandstandFest(double World, net::EventInstance& Out, std::string& Label);
std::string IgniteLighthouseAt(double World);
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
	const std::string Card = HostsRunAt(During, 230.0f, 196.0f);
	const std::string Code = Short + " #" + std::to_string(HasPick ? Net.TemplateOf(Pick).EventNo : 0) + ":";
	ui_test::Expect(HasPick && Card.compare(0, Code.size(), Code) == 0, ("FestBandstandPick: the bandstand's card is the picked event (" + Code + "), not " + Card).c_str());
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
	const std::string EveCard = HostsRunAt(Eve, 230.0f, 196.0f);
	const std::string EveCode = Short + " #" + std::to_string(HasEve ? Net.TemplateOf(EvePick).EventNo : 0) + ":";
	ui_test::Expect(HasEve && EveCard.compare(0, EveCode.size(), EveCode) == 0, ("FestBandstandPick: at 19:00 the bandstand's card is the picked event (" + EveCode + "), not " + EveCard).c_str());
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
		{"Prize", "$795.32"}, {"Date", "Oct 6"}};
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
}
} // namespace

namespace u4_ui_test
{
void SegFestHosts()
{
	FestHostsRouting();
	FestBandstandPick();
	FestCopyUiHosts();
	FestRitCodesAndHeld();
	FestHonestLines();
}
} // namespace u4_ui_test
