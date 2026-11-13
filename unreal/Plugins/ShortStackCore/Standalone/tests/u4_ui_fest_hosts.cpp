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
#include "UiSeg.h"

#include "CopyLint.h"
#include "Trademarks.h"
#include "ShortStack/Game/Festival.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/UI/EventArt.h"
#include "ShortStack/UI/RiverLine.h"
#include "../../Source/ShortStackCore/Private/ShortStack/Game/SitesDetail.h"

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
		{"Added", "$2,500"}, {"FESTIVAL", "PENNANT"}, {"Time", "9:00 PM"}, {"Name", "MRS. PARK"}};
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
			// A figure is a fill: no digit typed outside a placeholder (the singular "1 event" is a count's form, not a figure).
			if (std::isdigit(static_cast<unsigned char>(Text[I])) && Text.compare(0, 8, "1 event ") != 0)
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
} // namespace

namespace u4_ui_test
{
void SegFestHosts()
{
	FestHostsRouting();
	FestBandstandPick();
	FestCopyUiHosts();
}
} // namespace u4_ui_test
