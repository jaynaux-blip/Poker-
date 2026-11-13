// u4_ui: the festival half's hub and moments (update 4 SPEC v1.7 §2.15.2; F-hubs' file): the hub in three frames, Main Event Day,
// the moments, the strip and popover, the pin wall. Called from u4_ui_main.cpp (the kit).
// Phase A (update 4 SPEC v1.7 §6 PA-48): registered, empty.
// G1 FH-1a (F-hubs' stream A; PLAN_G1 §2.1, §3.2; PLAN_FHUBS §4 and its Corrections): RiverLine's hub on RCOP 2026 (F-net's FN-1 row,
// routed by stream B's FH-1c), its first two pages and FestHonestLines' A half. Every page is emitted at scale 1.0 (the kit's Emit,
// under SS_U4_UI_OUT) and measured at 1.0 and 1.5 (vertices, text runs, NetSpaced lines, commands; Q18); SS_UI_BENCH=1 adds the warm
// draw (100 draws after one cold). §2.8.10's hub row is printed only here: the art is F-look's stubs until FH-7 asserts it. Helpers
// sit in an anonymous namespace and carry "Hubs" (the hosts' file, u4_ui_fest_hosts.cpp, and F-look's share the binary).
// G1 FH-1b (PLAN_G1 §2.1, §3.2): the strip, the day's rows and the popover: the page fest_popover (RCOP's cell on Night One), and
// FestStrikeGate, FestOneFrameClicks' popover half and FestCopyUi's A half (with FestBlurbsFilled' U2 half, PLAN_G1 §1 item 9). The
// popover's budget row (<= 3k, <= 20 runs) is printed, asserted at FH-7 with the hub's.
// G1 FH-2 (PLAN_G1 §2.1, §3.2; PLAN_FHUBS §4 FH-2 and its Corrections): the real calendar in all three frames, after F-net's FN-3 and
// FN-2b. Two careers walked forward in day order (the plan's A and B): A opens Ignite and draws FEIGN 2026's hub (fest_hub_ig_live);
// B never opens Ignite and draws Night One's strip (fest_strip_night_one), RCOP 2026's first day (fest_hub_rl_live) and its ladder
// (fest_hub_rl_locked), GRIT 2027's second night (fest_hub_rit_twice) and FEIGN II's popover on day 395 (fest_popover_pass, its BY
// INVITATION form: every site's Pass is -1 until F-world's FW-7; FH-7 emits the counting form). Checks: FestStripTen, FestLockedMain,
// FestCopyUi extended (U30, U33, U2's NIGHT form, U15's GRIT line and its drawn figures).
// G1 FH-4A (PLAN_G1 §2.1, §3.2; PLAN_FHUBS §4 FH-4 and its Corrections): THE RACE, CHAMPIONS and YOUR <FESTIVAL>, inside career B
// (no new career: the pages' time budget). On RCOP 2026's first day the YOUR panel invites before the hero plays, then the hero's first
// RCOP event (a cash told through World::HeroEntered and HeroFinished, the public path, item 15) fills it: fest_hub_your_ladder; a
// plain series result adds nothing (FestYourStrip, the hub's half). Days 14 and 15 walk THE RACE's movement (none before the hub
// has seen a day turn); day 19 keeps THE RACE first with the pin (FestLockedMain); day 35 shows RCOP 2026's four title lines and its
// Mini and Micro winners, and day 400 RCOP 2026 and 2027 on one roll: fest_hub_champ_roll (FestRollTitles). FestCopyUi extended (U5,
// U7, U8, U10, U12, U27, U28's seat line, U34, U37 and the roll's figures: listed, no digit typed, U37's and U10's forms).
#include "UiSeg.h"

#include "CopyLint.h"
#include "Trademarks.h"
#include "ShortStack/Game/Festival.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/UI/RiverLine.h"
#include "../../Source/ShortStackCore/Private/ShortStack/Game/WorldFestDetail.h"
#include "../../Source/ShortStackCore/Private/ShortStack/UI/RiverLineShared.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
// UI/RiverLineFestival.cpp (FH-2): the Festival Pass's form on a rival cell without an account, pure (FestStripTen calls it).
int HubPassForm(const net::InviteGate& Gate, bool Open, int RentsPaid, const fest::Festival* PassFest, const fest::Festival& F, int Events, int& N, int& Need);
// FH-4A: U10's line, U37's sentence, a roll edition's height, and the movement marks THE RACE drew in a laptop's last hub frame.
std::string HubYourCounts(const fest::YourLineView& V, bool Race);
std::string HubGoodOn(const std::vector<int>& Days, int Today);
float HubRollPitch(bool Championship);
int HubRaceMoves(const RiverLine& RL);
// FH-4A fix round 1: U35's two pills for a step row (its rung, the rung its seat is; 0 the Main).
std::pair<std::string, std::string> HubStepPills(int K, int Next);
} // namespace rlfest_detail
} // namespace ui
} // namespace ss

namespace
{
using ss::ui::DrawList;
using ss::ui::RiverLine;

/** A session and its laptop on RiverLine's SERIES page, the pointer away. */
struct HubsLaptop
{
	ui_test::QuietHooks H;
	ss::Session S;
	RiverLine RL;
	double Now = 1.0;
	explicit HubsLaptop(const std::string& Seed) : S(H, Seed), RL(S)
	{
		S.CurrentScreen = ss::Screen::Lobby;
		RL.UI.Ptr.Active = false;
		Now = ui_test::Step(S, Now, 0.2);
	}
	DrawList Frame(float Scale)
	{
		ui_test::TableMeasurer M;
		DrawList L;
		ss::ui::Canvas C(L, M, RiverLine::Width, RiverLine::Height, Scale);
		RL.Draw(C, Now);
		RL.UI.Ptr.EndFrame();
		return L;
	}
	/** The clock forward to World (the lobby clock; the world catches up), the player fed and rested. */
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
	/** RiverLine's SERIES page with no series picked: the host's current or next festival (§2.8.1). */
	void OpenHub()
	{
		RL.SiteShown = ss::net::SiteId::RiverLine;
		RL.ShowSeries(std::string(), Now);
		RL.OpenPage(RiverLine::Page::Series, Now);
		Now = ui_test::Step(S, Now, 1.5);
	}
	/** FH-1b: the pointer at (X, Y) for one frame, then a settled frame; One: pressed and released inside that one frame (a quick tap at
	 *  30 Hz), else a press frame and a release frame. Returns the settled frame. */
	DrawList Click(float X, float Y, bool One = false)
	{
		RL.UI.Ptr.Active = true;
		RL.UI.Ptr.X = X;
		RL.UI.Ptr.Y = Y;
		RL.UI.Ptr.Pressed = true;
		RL.UI.Ptr.Down = !One;
		RL.UI.Ptr.Released = One;
		Frame(1.0f);
		if (!One)
		{
			RL.UI.Ptr.Down = false;
			RL.UI.Ptr.Released = true;
			Frame(1.0f);
		}
		Away();
		return Frame(1.0f);
	}
	/** The pointer off the screen, still. */
	void Away()
	{
		RL.UI.Ptr.Active = false;
		RL.UI.Ptr.X = -1.0f;
		RL.UI.Ptr.Y = -1.0f;
		RL.UI.Ptr.Down = false;
		RL.UI.Ptr.Pressed = false;
		RL.UI.Ptr.Released = false;
	}
};

/** How many text runs are exactly Text. */
int HubsRuns(const DrawList& L, const std::string& Text)
{
	int N = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		N += Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == Text ? 1 : 0;
	}
	return N;
}

/** The first run exactly Text: its box's left and its baseline ({-1, -1} when none). */
std::pair<float, float> HubsRunAt(const DrawList& L, const std::string& Text)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == Text)
		{
			return {Cmd.Text.X, Cmd.Text.BaselineY};
		}
	}
	return {-1.0f, -1.0f};
}

/** A page's cost (§2.8.10's columns): vertices, text runs, NetSpaced lines (a baseline holding three or more one-glyph runs in a
 *  row, as NetSpaced draws them), commands. */
struct HubsCost
{
	size_t Vertices = 0, Runs = 0, Cmds = 0;
	int Spaced = 0;
};

bool HubsGlyph(const std::string& T)
{
	return !T.empty() && (T.size() == 1 || (static_cast<unsigned char>(T[0]) >= 0xC0 && T.size() <= 4));
}

HubsCost HubsCostOf(const DrawList& L)
{
	HubsCost K;
	K.Vertices = L.Vertices.size();
	K.Cmds = L.Cmds.size();
	float Base = -1.0e9f;
	int Run = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text)
		{
			continue;
		}
		++K.Runs;
		if (HubsGlyph(Cmd.Text.Text) && Cmd.Text.BaselineY == Base)
		{
			if (++Run == 3)
			{
				++K.Spaced;
			}
			continue;
		}
		Run = HubsGlyph(Cmd.Text.Text) ? 1 : 0;
		Base = Run ? Cmd.Text.BaselineY : -1.0e9f;
	}
	return K;
}

/** Emits Name (scale 1.0) and prints its cost at both scales, with the warm draw under SS_UI_BENCH=1. Returns the 1.0 list. */
DrawList HubsPage(const std::string& Name, HubsLaptop& P)
{
	ui_test::Emit(Name, P.RL, P.Now);
	DrawList One;
	for (const float Scale : {1.0f, 1.5f})
	{
		const DrawList L = P.Frame(Scale);
		const HubsCost K = HubsCostOf(L);
		std::printf("  %-24s x%.1f: %6zu vertices (hub <= 35000), %3zu runs (<= 600), %d NetSpaced (<= 4), %4zu commands\n", Name.c_str(), Scale, K.Vertices, K.Runs, K.Spaced, K.Cmds);
		if (Scale == 1.0f)
		{
			One = L;
		}
	}
	if (std::getenv("SS_UI_BENCH"))
	{
		P.Frame(1.0f);
		const auto T0 = std::chrono::steady_clock::now();
		for (int I = 0; I < 100; ++I)
		{
			P.Frame(1.0f);
		}
		const double Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count() / 100.0;
		std::printf("  %-24s warm draw %.3f ms (hub <= 1.5 ms; printed, FH-7 asserts)\n", Name.c_str(), Ms);
	}
	return One;
}

/** FestHonestLines (§2.15.2; F-hubs' A half, FH-1a: U9's empty roll): Night One's hub opens on CHAMPIONS (RCOP 2026 is Announced) and
 *  shows the two honest lines while the roll is empty, "No champion yet. The first name goes up on {Date}." with the Main's date
 *  (Nov 8) and "The record books opened Oct 6, 2026."; neither hub page holds the old typed champion's prize ("4,108,220", "4.1M").
 *  Stream B's half (FH-3a, FH-6b) adds the slide, the claim forms, the goals and the true defenders. Plant: CHAMPIONS' empty state
 *  printing the old claim "Last year's champion won $4,108,220." in U9's place. */
void FestHonestLinesHubs(const DrawList& NightOne, const DrawList& Buildup)
{
	const ss::fest::Festival* Rcop = ss::fest::FestivalOf("rcop");
	ui_test::Expect(Rcop != nullptr && Rcop->MainDay == 34, "FestHonestLines: RCOP 2026 is a festival with its Main on day 34 (F-net's FN-1 row)");
	const std::string FirstName = "No champion yet. The first name goes up on " + (Rcop ? ss::net::DateLabel(Rcop->MainDay) : std::string("?")) + ".";
	ui_test::Expect(FirstName == "No champion yet. The first name goes up on Nov 8.", ("FestHonestLines: U9's date is the Main's, Nov 8 (" + FirstName + ")").c_str());
	ui_test::Expect(ui_test::DrewRun(NightOne, "CHAMPIONS") && ui_test::DrewRun(NightOne, "THE RACE"), "FestHonestLines: Night One's SERIES page is RCOP 2026's hub, its tabs drawn");
	ui_test::Expect(ui_test::DrewRun(NightOne, FirstName), ("FestHonestLines: Night One's hub shows \"" + FirstName + "\"").c_str());
	ui_test::Expect(ui_test::DrewRun(NightOne, "The record books opened Oct 6, 2026."), "FestHonestLines: Night One's hub shows \"The record books opened Oct 6, 2026.\"");
	ui_test::Expect(ui_test::DrewRun(Buildup, FirstName), "FestHonestLines: the Buildup hub (day 6) still opens on CHAMPIONS' honest lines");
	for (const DrawList* L : {&NightOne, &Buildup})
	{
		for (const char* Claim : {"4,108,220", "4.1M"})
		{
			ui_test::Expect(!ui_test::Drew(*L, Claim), (std::string("FestHonestLines: no hub page holds the typed past prize ") + Claim).c_str());
		}
	}
}

/** FH-1a's pages: fest_hub_rl_announced (Night One: RCOP 2026 Announced, the countdown over OPENS IN) and fest_hub_rl_buildup (day 6,
 *  RCOP's Buildup: FirstDay 13 - 7). */
void FestHubPagesRiverLine()
{
	HubsLaptop P("u4-fest-hub");
	const ss::fest::Festival* Rcop = ss::fest::FestivalOf("rcop");
	P.OpenHub();
	const int Day0 = ss::net::DayOf(P.S.WorldMinutes());
	ui_test::Expect(Rcop && ss::fest::PhaseOf(*Rcop, P.S.WorldMinutes(), nullptr) == ss::fest::Phase::Announced, "fest_hub_rl_announced: RCOP 2026 is Announced on Night One");
	const DrawList NightOne = HubsPage("fest_hub_rl_announced", P);
	ui_test::Expect(ui_test::DrewRun(NightOne, "OPENS IN"), "fest_hub_rl_announced: the countdown stands over OPENS IN");
	std::printf("  Night One is world day %d; RCOP 2026 opens on day %d\n", Day0, Rcop ? Rcop->FirstDay : -1);
	P.At(static_cast<double>(Rcop ? Rcop->FirstDay - ss::fest::kBuildupDays : 6) * ss::net::MinutesPerDay + 10.0 * 60.0);
	P.OpenHub();
	ui_test::Expect(Rcop && ss::fest::PhaseOf(*Rcop, P.S.WorldMinutes(), nullptr) == ss::fest::Phase::Buildup, "fest_hub_rl_buildup: RCOP 2026 is in its Buildup on day 6");
	const DrawList Buildup = HubsPage("fest_hub_rl_buildup", P);
	FestHonestLinesHubs(NightOne, Buildup);
	// FestCopyUi's U2 half (FestBlurbsFilled, PLAN_G1 §1 item 9): the banner's state figure is the edition's own, filled from the clock.
	if (Rcop)
	{
		const long long M = static_cast<long long>(std::floor(static_cast<double>(Rcop->FirstDay) * ss::net::MinutesPerDay - P.S.WorldMinutes()));
		char Want[32];
		std::snprintf(Want, sizeof(Want), "%lldD %02lldH %02lldM", M / 1440, (M / 60) % 24, M % 60);
		ui_test::Expect(ui_test::DrewRun(Buildup, Want), (std::string("FestCopyUi: the Buildup hub's countdown is RCOP 2026's own, ") + Want).c_str());
		const std::string Dates = ss::net::DateLabel(Rcop->FirstDay) + " \xE2\x80\x93 " + ss::net::DateLabel(Rcop->LastDay);
		ui_test::Expect(ui_test::DrewRun(Buildup, Dates), ("FestCopyUi: the banner's DATES are the edition's, " + Dates).c_str());
	}
	// FH-1b: THE FESTIVAL YEAR on Night One: RCOP 2026 (Announced, the next to open) lit and underlined in its calendar slot (fsy8); the
	// brands F-net has built so far each in theirs, the rest empty until their rows land.
	ui_test::Expect(ui_test::DrewRun(NightOne, "RCOP") && ui_test::DrewRun(NightOne, "Oct 18"), "THE FESTIVAL YEAR: RCOP 2026's cell, its Short and its opening date");
	ui_test::Expect(ui_test::TextAt(NightOne, "RCOP") > 84.0f && ui_test::TextAt(NightOne, "RCOP") < 124.0f, "THE FESTIVAL YEAR: RCOP's cell sits in the strip's rect");
}

/** The settings the clause half needs: a running deal that plays only on Ignite, so a RiverLine registration strikes (update 2 §5.7,
 *  the sites' SitesClause precedent); "" when the sponsors' core offers none to sign (the half is then skipped, said so). */
std::string HubsClauseOnIgnite(ss::Session& S)
{
	S.DebugSponsor("offer riverline");
	for (ss::sponsor::Contract& K : S.Life.Sponsors.Contracts)
	{
		if (K.Brand == "riverline")
		{
			K.T.Only = "ignite";
		}
	}
	if (!S.Life.Sponsors.Contracts.empty())
	{
		S.SignDeal(S.Life.Sponsors.Contracts.back().Id);
	}
	return ss::ui::rlsites_detail::SitesClauseWarning(S, ss::net::SiteId::RiverLine, false, false);
}

/** FestStrikeGate (§2.15.2; FH-1b): the hub's register buttons are the client's RegisterBlock, so a row registers only through
 *  SitesTwoPress's second press: off the clause the first press arms "Confirm" (Session::ConfirmRegister) and registers nothing, the
 *  second registers; under a "plays only on" clause the first press reads the strike, the second ("Take the strike") passes
 *  StrikeConfirm{true} to core's gate (W3). One confirm step at a time, the pressed row's. On RCOP 2026's first day, 30 minutes before
 *  the first joinable event, so its row is open (RegisterBlock opens an hour ahead). Plant: a hub row calling Session::RegisterEvent
 *  directly. Also FestCopyUi's live figure: the banner reads "DAY 1 OF 22". */
void FestStrikeGateHubs()
{
	const ss::fest::Festival* Rcop = ss::fest::FestivalOf("rcop");
	if (!Rcop)
	{
		ui_test::Expect(false, "FestStrikeGate: RCOP 2026 is a festival");
		return;
	}
	const ss::net::Network& Net = ss::net::Shared();
	for (const bool Clause : {false, true})
	{
		HubsLaptop P(Clause ? "u4-fest-strike-clause" : "u4-fest-strike");
		ss::Session& S = P.S;
		S.BankrollCents = 50000000;
		ss::net::EventInstance E;
		bool Found = false;
		for (const ss::net::EventInstance& I : Net.Window(static_cast<double>(Rcop->FirstDay) * ss::net::MinutesPerDay, static_cast<double>(Rcop->FirstDay + 1) * ss::net::MinutesPerDay))
		{
			const ss::net::EventTemplate& T = Net.TemplateOf(I);
			if (!Found && T.Series == "rcop" && T.BuyInCents <= 100000 && Net.Joinable(I, nullptr, S.Unlocks()))
			{
				E = I;
				Found = true;
			}
		}
		ui_test::Expect(Found, "FestStrikeGate: RCOP 2026's first day has a joinable event under $1,000");
		if (!Found)
		{
			return;
		}
		P.At(E.Start - 30.0);
		std::string Warn;
		if (Clause)
		{
			Warn = HubsClauseOnIgnite(S);
			if (Warn.empty())
			{
				std::printf("  FestStrikeGate: no deal to sign that plays only on Ignite; the clause half is skipped\n");
				return;
			}
		}
		P.OpenHub();
		P.Away();
		DrawList L = P.Frame(1.0f);
		const std::vector<std::string> Rows = P.RL.ListedEvents();
		const auto At = std::find(Rows.begin(), Rows.end(), E.Id);
		const int Index = At == Rows.end() ? -1 : static_cast<int>(At - Rows.begin());
		ui_test::Expect(Index >= 0 && Index < 7, ("FestStrikeGate: " + E.Id + " is one of the day's rows in view (written to Listed)").c_str());
		if (Index < 0 || Index >= 7)
		{
			return;
		}
		if (!Clause)
		{
			ui_test::Expect(ui_test::DrewRun(L, "DAY 1 OF 22"), "FestCopyUi: RCOP 2026's first day reads DAY 1 OF 22 (U2, the edition's days)");
		}
		const float RowY = 408.0f + 72.0f * static_cast<float>(Index);
		const float Bx = 24.0f + 1000.0f - 372.0f; // the row's button {R.X + R.W - 372, R.Y + 10, 360, 46}
		const std::string First = Clause ? Warn : "Register \xC2\xB7 " + ss::net::BuyIn(Net.TemplateOf(E).BuyInCents);
		std::pair<float, float> Btn = HubsRunAt(L, First);
		if (Clause && Btn.second < 0.0f)
		{
			Btn = HubsRunAt(L, ss::ui::rlsites_detail::SitesStrikeShort); // the warning shortened to fit the row's button (SitesFitButton)
		}
		ui_test::Expect(Btn.second > RowY && Btn.second < RowY + 66.0f, ("FestStrikeGate: the row's button is RegisterBlock's (\"" + First + "\")").c_str());
		L = P.Click(Bx + 180.0f, RowY + 33.0f);
		ui_test::Expect(!S.IsPlaying(E.Id), Clause ? "FestStrikeGate: under the clause the first press registers nothing" : "FestStrikeGate: the first press registers nothing");
		ui_test::Expect(S.ConfirmRegister && P.RL.SelectedEvent() == E.Id, "FestStrikeGate: the first press arms the confirm step, for the row pressed");
		const std::string Ok = Clause ? "Take the strike" : "Confirm";
		ui_test::Expect(HubsRuns(L, Ok) == 1 && ui_test::DrewRun(L, "Cancel"), ("FestStrikeGate: one confirm step (\"" + Ok + "\", Cancel), the pressed row's alone").c_str());
		if (Clause)
		{
			ui_test::Expect(ui_test::Drew(L, ss::ui::rlsites_detail::SitesStrikeConfirm), "FestStrikeGate: under the clause the confirm step says it's a strike");
		}
		P.Click(Bx + 60.0f, RowY + 33.0f); // the confirm step's left half ("regok")
		ui_test::Expect(S.IsPlaying(E.Id), Clause ? "FestStrikeGate: \"Take the strike\" registers through core's gate" : "FestStrikeGate: the second press registers");
	}
}

/** FH-1b's page fest_popover (RCOP's cell on Night One) and FestOneFrameClicks' popover half (§2.15.2): the click that opens the
 *  popover fires nothing beneath it and doesn't close it; a click on the popover reaches nothing under it (the Main card's Details,
 *  beneath its foot, stays shut); a click off it closes it and is swallowed (the hub stays on CHAMPIONS); OPEN THE HUB shows the cell's
 *  edition and closes it. The popover row of §2.8.10 (<= 3k vertices, <= 20 runs over the page) is printed. Plant: the popover drawn
 *  without SitesBlockUnder. */
void FestPopoverHubs()
{
	HubsLaptop P("u4-fest-popover");
	P.OpenHub();
	P.Away();
	const DrawList Before = P.Frame(1.0f);
	ui_test::Expect(!ui_test::DrewRun(Before, "OPEN THE HUB"), "fest_popover: no popover before a click");
	const float CellX = 260.0f + 8.0f * 128.0f + 50.0f; // RCOP's cell, fsy8
	const DrawList Opened = P.Click(CellX, 104.0f, true);
	ui_test::Expect(ui_test::DrewRun(Opened, "OPEN THE HUB") && ui_test::DrewRun(Opened, "RCOP 2026"), "fest_popover: a one-frame click on RCOP's cell opens its popover");
	ui_test::Expect(P.RL.CurrentPage() == RiverLine::Page::Series, "FestOneFrameClicks: the click that opens the popover fires nothing else");
	ui_test::Expect(ui_test::DrewRun(Opened, "Opens Oct 18.") && ui_test::DrewRun(Opened, "Your account is open.") && ui_test::DrewRun(Opened, "RiverLine \xC2\xB7 Oct 18 \xE2\x80\x93 Nov 8"),
		"fest_popover: the phase line, the account line and the host and window (U16)");
	const DrawList Page = HubsPage("fest_popover", P);
	const HubsCost With = HubsCostOf(Page);
	const HubsCost Without = HubsCostOf(Before);
	std::printf("  fest_popover: the popover %zu vertices (<= 3000), %d runs (<= 20) over the page (printed; FH-7 asserts)\n", With.Vertices - Without.Vertices,
		static_cast<int>(With.Runs) - static_cast<int>(Without.Runs));
	const std::string Picked = P.RL.SelectedEvent();
	const DrawList OnIt = P.Click(1500.0f, 290.0f); // on the popover's foot, over the Main card's Details
	ui_test::Expect(P.RL.CurrentPage() == RiverLine::Page::Series && P.RL.SelectedEvent() == Picked, "FestOneFrameClicks: a click on the popover reaches nothing beneath it (Details stays shut)");
	ui_test::Expect(ui_test::DrewRun(OnIt, "OPEN THE HUB"), "FestOneFrameClicks: a click on the popover keeps it open");
	const DrawList Off = P.Click(1100.0f, 366.0f); // off it, over THE RACE's tab
	ui_test::Expect(!ui_test::DrewRun(Off, "OPEN THE HUB"), "FestOneFrameClicks: a click off the popover closes it");
	ui_test::Expect(ui_test::DrewRun(Off, "No champion yet. The first name goes up on Nov 8.") && !ui_test::DrewRun(Off, "No points yet."),
		"FestOneFrameClicks: the click that closes it is swallowed (the hub stays on CHAMPIONS)");
	P.Click(CellX, 104.0f, true);
	const std::pair<float, float> Go = HubsRunAt(P.Frame(1.0f), "OPEN THE HUB");
	const DrawList Shown = P.Click(Go.first + 20.0f, Go.second - 4.0f);
	ui_test::Expect(Go.first > 0.0f && !ui_test::DrewRun(Shown, "OPEN THE HUB") && P.RL.CurrentPage() == RiverLine::Page::Series && ui_test::DrewRun(Shown, "RCOP 2026"),
		"fest_popover: OPEN THE HUB shows RCOP 2026's hub and closes the popover");
}

/** The UTF-8 code points of S. */
std::vector<uint32_t> HubsCodePoints(const std::string& S)
{
	std::vector<uint32_t> Out;
	for (size_t I = 0; I < S.size();)
	{
		const unsigned char B = static_cast<unsigned char>(S[I]);
		const size_t Len = B >= 0xF0 ? 4 : B >= 0xE0 ? 3 : B >= 0xC0 ? 2 : 1;
		uint32_t Cp = Len == 1 ? B : Len == 2 ? (B & 0x1Fu) : Len == 3 ? (B & 0x0Fu) : (B & 0x07u);
		for (size_t K = 1; K < Len && I + K < S.size(); ++K)
		{
			Cp = (Cp << 6) | (static_cast<unsigned char>(S[I + K]) & 0x3Fu);
		}
		Out.push_back(Cp);
		I += Len;
	}
	return Out;
}

/** The words of a lower-case line (letters, digits and ''' joined). */
std::vector<std::string> HubsWords(const std::string& Low)
{
	std::vector<std::string> Out(1);
	for (const char Ch : Low)
	{
		if (std::isalnum(static_cast<unsigned char>(Ch)) || Ch == '\'')
		{
			Out.back() += Ch;
		}
		else if (!Out.back().empty())
		{
			Out.emplace_back();
		}
	}
	if (Out.back().empty())
	{
		Out.pop_back();
	}
	return Out;
}

/** What's wrong with one FestUiInventory entry ("" when nothing), its kind and text, filled from Vars (§2.11): the kind's stop rule
 *  and cap (a sentence: CopyLint at 200; a label: no closing stop, at most 84), U1, both trademark tiers, the design words, the
 *  avoided words, X1 (no count of people as "players"), FestChampWord and the glyph rule, all after the fill. */
std::string HubsCopyWhy(const std::string& Kind, const std::string& Text, const std::map<std::string, std::string>& Vars)
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
	static const char* const HubsAvoided[] = {"spark", "flame", "blaze", "wildfire", "ember", "crest", "heat", "cold", "stamina", "momentum", "energy", "board",
		"watchlist", "headliners", "beef", "hall", "class", "boom", "millions", "jackpot", "run good", "supernova", "premier"};
	for (const char* W : HubsAvoided)
	{
		if (trademarks::HasWord(Low, W))
		{
			return std::string("an avoided word: ") + W;
		}
	}
	const std::vector<std::string> Words = HubsWords(Low);
	for (size_t I = 0; I < Words.size(); ++I)
	{
		const std::string& W = Words[I];
		if (W.size() >= 4 && W.compare(W.size() - 4, 4, "coop") == 0)
		{
			return "a *COOP acronym: " + W;
		}
		if ((W == "players" || W == "player") && I > 0 && std::isdigit(static_cast<unsigned char>(Words[I - 1].back())))
		{
			return "X1: a count of people as players";
		}
		if (W == "ignite" || W == "ignite's")
		{
			for (size_t J = I > 2 ? I - 2 : 0; J < Words.size() && J <= I + 2; ++J)
			{
				if (Words[J].rfind("championship", 0) == 0)
				{
					return "FestChampWord: Ignite beside \"championship\"";
				}
			}
		}
	}
	for (const uint32_t Cp : HubsCodePoints(Line))
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

/** FestCopyUi (§2.15.2, §2.11; F-hubs' A half, FH-1b): every fest::FestUiInventory entry (this file's and stream B's, through
 *  HostUiStrings) carries its kind and passes HubsCopyWhy filled from a sample edition; a second fill with "The Championship Online
 *  2027" (after "the " or "next ", through fest::NameAfterArticle) never prints "the The", and a next-year date prints its year once
 *  fest::DateInSentence has F-net's body (H0's stub prints today's form: FH-2 merges FN-2a's helpers); U2's formats type no digit
 *  (FestBlurbsFilled' U2 half; the drawn figures are checked on the pages above). Plant: "bot" in a line. */
void FestCopyUiHubs()
{
	const std::vector<std::string> Inv = ss::fest::FestUiInventory();
	std::map<std::string, std::string> Vars = copylint::Samples();
	const std::pair<const char*, const char*> Extra[] = {{"D", "12"}, {"H", "07"}, {"M", "41"}, {"N", "4"}, {"Days", "22"}, {"FirstDate", "Oct 18"},
		{"LastDate", "Nov 8"}, {"Weekday", "SUN"}, {"Date", "Nov 8"}, {"Time", "3:00 PM"}, {"Gtd", "$78,203,400"}, {"BuyIn", "$5,250"}, {"Stack", "100,000"},
		{"Added", "$2,500"}, {"FESTIVAL", "RCOP"}, {"Festival", "RCOP 2026"}, {"Year", "2026"}, {"Site", "RiverLine"}, {"SITE", "IGNITE"}, {"Event", "$5,250 Main Event"},
		{"Overlay", "$412,600"}, {"Amount", "$1,280,000"}, {"Entries", "2,345"}, {"Countdown", "2H 10M"}, {"Clock", "0:41:12"},
		{"Nights", "12"}, {"Show1", "7 PM"}, {"Show2", "10 PM"}, {"Need", "20"}, {"Rank", "164th"}, {"Scorers", "2,214"}, {"Points", "290"},
		{"Events", "9"}, {"Cashes", "3"}, {"Best", "31st"}, {"Field", "1,964"}, {"MARK", "FINALIST"}, {"Dates", "Apr 7 and Apr 9"}, {"Prize", "$795.32"},
		{"Place", "31st"}, {"K", "2"}};
	for (const auto& Kv : Extra)
	{
		Vars[Kv.first] = Kv.second;
	}
	const int Today = 1;
	const int NextYear = Today + 400;
	const bool DatesReal = ss::fest::DateInSentence(NextYear, Today) != ss::net::DateLabel(NextYear);
	int Bad = 0;
	int Labels = 0;
	int Sentences = 0;
	for (const std::string& Entry : Inv)
	{
		const size_t Tab = Entry.find('\t');
		const std::string Kind = Tab == std::string::npos ? std::string() : Entry.substr(0, Tab);
		const std::string Text = Tab == std::string::npos ? Entry : Entry.substr(Tab + 1);
		Labels += Kind == "label" ? 1 : 0;
		Sentences += Kind == "sentence" ? 1 : 0;
		std::string Why = HubsCopyWhy(Kind, Text, Vars);
		if (Why.empty())
		{
			// The second fill: The Championship Online 2027 (after "the " or "next ", as the hub prints it) and a next-year date.
			std::map<std::string, std::string> Second = Vars;
			Second["Date"] = ss::fest::DateInSentence(NextYear, Today);
			std::string Filled;
			for (size_t I = 0; I < Text.size();)
			{
				if (Text.compare(I, 10, "{Festival}") == 0)
				{
					const std::string Before = trademarks::Lower(Filled);
					const bool AfterArticle = (Before.size() >= 4 && Before.compare(Before.size() - 4, 4, "the ") == 0) || (Before.size() >= 5 && Before.compare(Before.size() - 5, 5, "next ") == 0);
					Filled += AfterArticle ? ss::fest::NameAfterArticle("The Championship Online 2027") : std::string("The Championship Online 2027");
					I += 10;
				}
				else
				{
					Filled += Text[I++];
				}
			}
			Filled = copylint::Fill(Filled, Second);
			if (trademarks::Lower(Filled).find("the the ") != std::string::npos)
			{
				Why = "\"the The\" in the second fill: " + Filled;
			}
			else if (DatesReal && Text.find("{Date}") != std::string::npos && Filled.find(std::to_string(ss::world::YearOf(NextYear))) == std::string::npos)
			{
				Why = "a next-year date without its year: " + Filled;
			}
		}
		if (!Why.empty())
		{
			++Bad;
			std::printf("COPY (FestUiInventory) \"%s\": %s\n", Entry.c_str(), Why.c_str());
		}
	}
	ui_test::Expect(Bad == 0, ("FestCopyUi: every FestUiInventory line passes the lint, both tiers, U1, the design and avoided words, X1, FestChampWord and the glyph rule (" +
		std::to_string(Labels) + " labels, " + std::to_string(Sentences) + " sentences, " + std::to_string(Bad) + " failing)").c_str());
	if (!DatesReal)
	{
		std::printf("  FestCopyUi: fest::DateInSentence is H0's stub (F-net's body lands in FN-2a, merged in FH-2): the next-year date check waits\n");
	}
	// U2's formats and U15's GRIT line (FestBlurbsFilled' U2/U15 half; FH-2 adds the NIGHT forms): in the inventory, no digit typed.
	for (const char* U2 : {"OPENS IN", "{D}D {H}H {M}M", "PROGRESS", "DAY {N} OF {Days}", "MAIN EVENT TODAY", "COMPLETE", "NIGHT {N} OF {Nights}", "NIGHT {N}",
			 "{FESTIVAL} \xC2\xB7 NIGHT {N} OF {Nights} \xC2\xB7 SHOWS AT {Show1} AND {Show2}"})
	{
		const bool Listed = std::find(Inv.begin(), Inv.end(), std::string("label\t") + U2) != Inv.end();
		std::string Bare; // the format without its placeholders' names ({Show1} is a variable, not a typed digit)
		for (const char* Ch = U2; *Ch; ++Ch)
		{
			if (*Ch == '{')
			{
				while (*Ch && *Ch != '}')
				{
					++Ch;
				}
				if (!*Ch)
				{
					break;
				}
				continue;
			}
			Bare += *Ch;
		}
		const bool Typed = Bare.find_first_of("0123456789") != std::string::npos;
		ui_test::Expect(Listed && !Typed, (std::string("FestCopyUi: U2's \"") + U2 + "\" is in FestUiInventory with no digit typed").c_str());
	}
	// FH-2: U30 and U33 are in the inventory with their kinds (so the lint above ran over them).
	for (const char* Line : {"label\tPASS {N}/{Need}", "sentence\tFestival Pass: {N} of {Need} online events.", "sentence\tOpens to you after your second rent.",
			 "label\t{N} SEATS AT THE POOL SO FAR", "label\t{N} SEATS WON"})
	{
		ui_test::Expect(std::find(Inv.begin(), Inv.end(), std::string(Line)) != Inv.end(), (std::string("FestCopyUi: FestUiInventory lists ") + Line).c_str());
	}
	// FH-4A: U5, U7, U10, U12, U27, U28's seat line, U37 and the roll's figures are listed with their kinds, every figure a placeholder
	// (U8's rungs print their own numbers: "PLAY 1", "TOP 25%").
	for (const char* Line : {"label\tEVERYONE", "label\tPEOPLE YOU KNOW", "label\tYou \xC2\xB7 {Rank} of {Scorers} in the race \xC2\xB7 {Points} pts",
			 "label\t{Events} events \xC2\xB7 {Cashes} cashes \xC2\xB7 best {Best} of {Field} \xC2\xB7 race {Rank} of {Scorers}", "label\tGOLD \xC2\xB7 won an event",
			 "label\tSILVER \xC2\xB7 cashed", "label\tOUTLINE \xC2\xB7 played", "label\t{N} pts to {MARK} \xC2\xB7 right now", "label\tA seat in the next Main Event",
			 "sentence\tGood on {Dates}.", "label\t{Prize} \xC2\xB7 1st of {Entries}", "label\t{Points} pts", "label\t{Place} of {Entries}"})
	{
		std::string Bare;
		for (const char* Ch = std::strchr(Line, '\t') + 1; *Ch; ++Ch)
		{
			if (*Ch == '{')
			{
				while (*Ch && *Ch != '}')
				{
					++Ch;
				}
				if (!*Ch)
				{
					break;
				}
				continue;
			}
			Bare += *Ch;
		}
		const bool Listed = std::find(Inv.begin(), Inv.end(), std::string(Line)) != Inv.end();
		const bool Typed = Bare.find_first_of("0123456789") != std::string::npos && std::string(Line).find("1st of") == std::string::npos;
		ui_test::Expect(Listed && !Typed, (std::string("FestCopyUi: FestUiInventory lists \"") + Line + "\" with no digit typed").c_str());
	}
	for (const char* Rung : {"PLAY 1", "CASH 1", "TOP 25%", "TOP 10%", "FINALIST", "PODIUM", "THE BELT", "MINI MAIN EVENT", "MICRO MAIN EVENT"})
	{
		ui_test::Expect(std::find(Inv.begin(), Inv.end(), std::string("label\t") + Rung) != Inv.end(), (std::string("FestCopyUi: FestUiInventory lists U8/U34's ") + Rung).c_str());
	}
	// U37 (SPEC v1.7 §2.16 step 8a: PENNANT 2027's Step 2 ticket on day 183, its runs on days 184 and 186): one date, two, three.
	namespace rf = ss::ui::rlfest_detail;
	const std::string Two = rf::HubGoodOn({184, 186}, 183);
	ui_test::Expect(Two == "Good on Apr 7 and Apr 9.", ("FestCopyUi: U37 reads \"Good on Apr 7 and Apr 9.\" for PENNANT 2027's Step 2 runs (" + Two + ")").c_str());
	ui_test::Expect(rf::HubGoodOn({186}, 185) == "Good on Apr 9." && rf::HubGoodOn({}, 185).empty() && rf::HubGoodOn({183, 184, 186}, 180) == "Good on Apr 6, Apr 7 and Apr 9.",
		"FestCopyUi: U37 with one day, none and three");
	// U10, singular at one ("1 event · 1 cash"), the race clause only while the hero scores.
	ss::fest::YourLineView V;
	V.Shown = true;
	V.Events = 1;
	V.Cashes = 1;
	V.Best = 3;
	V.Field = 1964;
	const std::string One = rf::HubYourCounts(V, true);
	V.Events = 9;
	V.Cashes = 0;
	V.Best = 31;
	V.Rank = 164;
	V.Scorers = 2214;
	const std::string Many = rf::HubYourCounts(V, true);
	ui_test::Expect(One == "1 event \xC2\xB7 1 cash \xC2\xB7 best 3rd of 1,964" && Many == "9 events \xC2\xB7 0 cashes \xC2\xB7 best 31st of 1,964 \xC2\xB7 race 164th of 2,214" &&
			rf::HubYourCounts(V, false) == "9 events \xC2\xB7 0 cashes \xC2\xB7 best 31st of 1,964",
		("FestCopyUi: U10's forms (" + One + " | " + Many + ")").c_str());
	// FH-4A fix round 1, U35 (§2.8.2): the five step-pill forms listed with their kinds, no digit typed but the pill numbers ({K} is
	// the next rung's, a placeholder); HubStepPills gives each rung its own pair, its seat form the inventory's filled.
	for (const char* Line : {"label\tSTEP 1", "label\tSTEP 2", "label\tSTEP 3", "label\tFOR A STEP {K} TICKET", "label\tFOR A MAIN EVENT SEAT"})
	{
		std::string Bare;
		for (const char* Ch = std::strchr(Line, '\t') + 1; *Ch; ++Ch)
		{
			if (*Ch == '{')
			{
				while (*Ch && *Ch != '}')
				{
					++Ch;
				}
				if (!*Ch)
				{
					break;
				}
				continue;
			}
			Bare += *Ch;
		}
		const bool PillNo = Bare.size() == 6 && Bare.compare(0, 5, "STEP ") == 0;
		const bool Listed = std::find(Inv.begin(), Inv.end(), std::string(Line)) != Inv.end();
		const bool Typed = !PillNo && Bare.find_first_of("0123456789") != std::string::npos;
		ui_test::Expect(Listed && !Typed, (std::string("FestCopyUi: FestUiInventory lists U35's \"") + Line + "\" with no digit typed but the pill number").c_str());
	}
	bool PillsOk = true;
	std::string PillsSeen;
	for (const std::pair<int, int>& Rung : {std::pair<int, int>{1, 2}, std::pair<int, int>{2, 3}, std::pair<int, int>{3, 0}})
	{
		const std::pair<std::string, std::string> Pp = rf::HubStepPills(Rung.first, Rung.second);
		const std::string Seat = Rung.second > 0 ? copylint::Fill("FOR A STEP {K} TICKET", {{"K", std::to_string(Rung.second)}}) : std::string("FOR A MAIN EVENT SEAT");
		PillsOk = PillsOk && Pp.first == "STEP " + std::to_string(Rung.first) && Pp.second == Seat &&
			std::find(Inv.begin(), Inv.end(), "label\t" + Pp.first) != Inv.end();
		PillsSeen += (PillsSeen.empty() ? "" : " | ") + Pp.first + " / " + Pp.second;
	}
	ui_test::Expect(PillsOk, ("FestCopyUi: U35's pills for Step 1, 2 and 3 (" + PillsSeen + ")").c_str());
}
// ------------------------------------------------------------------ FH-2: the real calendar in three frames

/** The text runs inside {X0, Y0}-{X1, Y1} (box left and baseline). */
int HubsRunsIn(const DrawList& L, float X0, float Y0, float X1, float Y1)
{
	int N = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		N += Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.X >= X0 && Cmd.Text.X < X1 && Cmd.Text.BaselineY >= Y0 && Cmd.Text.BaselineY < Y1 ? 1 : 0;
	}
	return N;
}

/** The run exactly Text with its baseline in [Y0, Y1): its box's left and baseline ({-1, -1} when none). */
std::pair<float, float> HubsRunBetween(const DrawList& L, const std::string& Text, float Y0, float Y1)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == Text && Cmd.Text.BaselineY >= Y0 && Cmd.Text.BaselineY < Y1)
		{
			return {Cmd.Text.X, Cmd.Text.BaselineY};
		}
	}
	return {-1.0f, -1.0f};
}

/** An instance by its id (an empty one when the site has none). */
ss::net::EventInstance HubsInstance(const ss::net::Network& Net, const std::string& Id)
{
	ss::net::EventInstance E;
	if (!Net.FindInstance(Id, E))
	{
		E = ss::net::EventInstance();
	}
	return E;
}

/** The brands of THE FESTIVAL YEAR in calendar order (§2.8.2's fsy0..9) and the Short each edition's series prints. */
const ss::fest::Brand HubsYearBrands[10] = {ss::fest::Brand::Writ, ss::fest::Brand::RingRush, ss::fest::Brand::Pennant, ss::fest::Brand::Grit, ss::fest::Brand::Tco,
	ss::fest::Brand::Reign, ss::fest::Brand::Slam, ss::fest::Brand::Push, ss::fest::Brand::Rcop, ss::fest::Brand::Feign};

std::string HubsShortOf(const ss::fest::Festival& F)
{
	const ss::net::SeriesInfo* Sr = ss::net::Site(F.Site).FindSeries(F.SeriesId);
	std::string Out = Sr ? Sr->Short : std::string();
	for (char& Ch : Out)
	{
		Ch = static_cast<char>(std::toupper(static_cast<unsigned char>(Ch)));
	}
	return Out;
}

/** A frame's strip (§2.8.2): the first cell's x, the pitch, the text column and the strip's top. */
struct HubsStrip
{
	const char* Host;
	float X0, Pitch, Text, Top;
};
constexpr HubsStrip HubsStripRiverLine{"RiverLine", 260.0f, 128.0f, 92.0f, 84.0f};
constexpr HubsStrip HubsStripRunItTwice{"RunItTwice", 368.0f, 116.0f, 80.0f, 96.0f};

/** FestStripTen's page half (§2.15.2, v1.6; FH-2) on one frame's hub: every brand of the ten that has an edition in reach draws its
 *  Short in its calendar slot (the run's box at X0 + Pitch * i + 36, i the brand's place in WRIT ... FEIGN), every cell (Pitch - 6
 *  wide) inside the content rect's right edge (1576 on all three hosts), a rival cell without an account drawing BY INVITATION in its
 *  text column. Returns the cells found. */
int FestStripTenPage(const DrawList& L, const HubsStrip& Sp, double World)
{
	const int Today = ss::net::DayOf(World);
	int Found = 0;
	for (int I = 0; I < 10; ++I)
	{
		// The brand's on-stage or next edition (PLAN_FHUBS Q24): the first edition of the brand whose window ends on or after today.
		const ss::fest::Festival* Ed = nullptr;
		for (const ss::fest::Festival* F : ss::fest::Festivals(Today - 14, Today + 400))
		{
			if (!Ed && F->Which == HubsYearBrands[I] && F->LastDay + 9 >= Today)
			{
				Ed = F;
			}
		}
		if (!Ed)
		{
			continue;
		}
		const std::string Short = HubsShortOf(*Ed);
		const std::pair<float, float> At = HubsRunBetween(L, Short, Sp.Top, Sp.Top + 40.0f);
		const float Want = Sp.X0 + Sp.Pitch * static_cast<float>(I) + 36.0f;
		ui_test::Expect(At.first >= 0.0f && std::fabs(At.first - Want) < 0.5f,
			(std::string("FestStripTen: ") + Sp.Host + "'s cell fsy" + std::to_string(I) + " holds " + Short + " (" + Ed->Name + ") at x " + std::to_string(static_cast<int>(Want))).c_str());
		const float Right = Sp.X0 + Sp.Pitch * static_cast<float>(I) + Sp.Pitch - 6.0f;
		ui_test::Expect(Right <= 1576.0f, (std::string("FestStripTen: ") + Sp.Host + "'s cell fsy" + std::to_string(I) + " ends inside the content rect").c_str());
		Found += At.first >= 0.0f ? 1 : 0;
	}
	return Found;
}

/** FestStripTen (§2.15.2, v1.6; FH-2): the ten cells fit both frames (128 from x 260 on RiverLine and Ignite, 116 from x 368 on
 *  RunItTwice: FestStripTenPage on each frame's page); every Short any edition prints fits its cell's text column (92 units at 13/800,
 *  80 on RunItTwice); "BY INVITATION" fits 80 at 9.5/800; the ids fsy0..9 are in calendar order (each cell's click opens its own
 *  brand's popover: HubsStripClicks). Also the Pass's three forms (§2.10 items 1 and 6, HubPassForm) on constructed gates. Plant: a
 *  ten-letter Short (it overflows the 80-unit column). */
void FestStripTenMeasures()
{
	ui_test::TableMeasurer M;
	DrawList Scratch;
	ss::ui::Canvas C(Scratch, M, RiverLine::Width, RiverLine::Height, 1.0f);
	int Shorts = 0;
	float Widest = 0.0f;
	std::string WidestShort;
	for (const ss::fest::Festival& F : ss::fest::All())
	{
		const std::string Short = HubsShortOf(F);
		const float W = C.Measure(Short, 13.0f, 800);
		++Shorts;
		if (W > Widest)
		{
			Widest = W;
			WidestShort = Short;
		}
		if (W > 80.0f)
		{
			ui_test::Expect(false, ("FestStripTen: " + F.Name + "'s Short " + Short + " measures " + std::to_string(W) + " at 13/800, past RunItTwice's 80-unit column").c_str());
		}
	}
	ui_test::Expect(Shorts > 0, "FestStripTen: F-net's calendar lists editions");
	std::printf("  FestStripTen: %d editions' Shorts measured, the widest %s at %.1f (columns 92 and 80)\n", Shorts, WidestShort.c_str(), Widest);
	const float Invite = C.Measure("BY INVITATION", 9.5f, 800);
	ui_test::Expect(Invite <= 80.0f, ("FestStripTen: BY INVITATION fits the 80-unit column at 9.5/800 (" + std::to_string(Invite) + ")").c_str());
	const float Pass = C.Measure("PASS 20/20", 9.5f, 800);
	ui_test::Expect(Pass <= 80.0f, ("FestStripTen: PASS 20/20 fits the 80-unit column at 9.5/800 (" + std::to_string(Pass) + ")").c_str());
	// The Pass's forms (GateMet's inputs, in its order): no Pass, a closed site, before the second rent, outside the festival weeks,
	// the first edition (two rents alone), and counting (N held at Need).
	const ss::fest::Festival* Feign2 = nullptr;
	const ss::fest::Festival* Feign1 = nullptr;
	for (const ss::fest::Festival& F : ss::fest::All())
	{
		Feign1 = !Feign1 && F.Which == ss::fest::Brand::Feign && F.Edition == 1 ? &F : Feign1;
		Feign2 = !Feign2 && F.Which == ss::fest::Brand::Feign && F.Edition == 2 ? &F : Feign2;
	}
	ui_test::Expect(Feign1 && Feign2, "FestStripTen: FEIGN's first and second editions are on the calendar");
	if (Feign1 && Feign2)
	{
		namespace rf = ss::ui::rlfest_detail;
		ss::net::InviteGate G;
		G.MinRentsPaid = 2;
		int N = 0;
		int Need = 0;
		ui_test::Expect(rf::HubPassForm(G, true, 2, Feign2, *Feign2, 30, N, Need) == 0, "FestStripTen: no Pass on the site (FestivalEvents -1) reads BY INVITATION");
		G.FestivalEvents = ss::fest::kPassEvents;
		ui_test::Expect(rf::HubPassForm(G, false, 2, Feign2, *Feign2, 30, N, Need) == 0, "FestStripTen: a site not open yet reads BY INVITATION");
		ui_test::Expect(rf::HubPassForm(G, true, 1, Feign2, *Feign2, 30, N, Need) == 1, "FestStripTen: before the second rent the popover says so (U30)");
		ui_test::Expect(rf::HubPassForm(G, true, 2, nullptr, *Feign2, 30, N, Need) == 0, "FestStripTen: outside the festival weeks the path is closed (BY INVITATION)");
		ui_test::Expect(rf::HubPassForm(G, true, 2, Feign1, *Feign1, 3, N, Need) == 0 && Need == 0, "FestStripTen: a first edition needs the two rents alone (no count shown)");
		ui_test::Expect(rf::HubPassForm(G, true, 2, Feign2, *Feign2, 7, N, Need) == 2 && N == 7 && Need == ss::fest::kPassEvents, "FestStripTen: the second edition counts, PASS 7/20");
		ui_test::Expect(rf::HubPassForm(G, true, 2, Feign2, *Feign2, 31, N, Need) == 2 && N == Need, "FestStripTen: the count holds at its Need once met");
	}
}

/** FestStripTen's ids (FH-2): a one-frame click on each drawn cell opens the popover of that cell's brand (its edition's name drawn as
 *  the popover's title), then a click on the scrim closes it. */
void HubsStripClicks(HubsLaptop& P, const HubsStrip& Sp)
{
	const int Today = ss::net::DayOf(P.S.WorldMinutes());
	for (int I = 0; I < 10; ++I)
	{
		const ss::fest::Festival* Ed = nullptr;
		for (const ss::fest::Festival* F : ss::fest::Festivals(Today - 14, Today + 400))
		{
			Ed = !Ed && F->Which == HubsYearBrands[I] && F->LastDay + 9 >= Today ? F : Ed;
		}
		if (!Ed)
		{
			continue;
		}
		const DrawList Open = P.Click(Sp.X0 + Sp.Pitch * static_cast<float>(I) + 50.0f, Sp.Top + 20.0f, true);
		const std::pair<float, float> Title = HubsRunBetween(Open, Ed->Name, Sp.Top + 40.0f, Sp.Top + 220.0f);
		ui_test::Expect(Title.first >= 0.0f, (std::string("FestStripTen: fsy") + std::to_string(I) + " on " + Sp.Host + " opens " + Ed->Name + "'s popover (calendar order)").c_str());
		P.Click(800.0f, 700.0f); // off it: the scrim closes it
	}
}

/** The Main's lock line on the hub's edition (Network::Joinable's own words), "" when it is joinable. */
std::string HubsMainLock(const ss::Session& S, const ss::fest::Festival& F)
{
	const ss::net::Network& Net = ss::net::Site(F.Site);
	ss::net::EventInstance Main;
	std::string Lock;
	if (!Net.Next(F.MainId, static_cast<double>(F.FirstDay) * ss::net::MinutesPerDay, Main) || Net.Joinable(Main, &Lock, S.Unlocks()))
	{
		return std::string();
	}
	return Lock;
}

/** FestLockedMain (§2.15.2, §2.2.3; FH-2), the ladder half on RCOP 2026's day 19 (a Saturday: step1-4 and mega-rcop all run): the day's
 *  rows scrolled to their end with the hub's wheel, each ladder row keeps its button (RegisterBlock's, in the row's button rect) and
 *  shows the Main's lock line and U31 beside it. Plant: the lock line dropped for mega-rcop. */
void FestLockedMainRows(HubsLaptop& P, const ss::fest::Festival& Rcop, const DrawList& L)
{
	const std::string Lock = HubsMainLock(P.S, Rcop);
	const std::string U31 = "A seat won here waits in your wallet until big fields open.";
	const std::vector<std::string> Rows = P.RL.ListedEvents();
	const ss::net::Network& Net = ss::net::Site(Rcop.Site);
	int Ladder = 0;
	for (const char* Id : {"step1", "step2", "step3", "step4", "mega-rcop"})
	{
		int Index = -1;
		for (size_t I = 0; I < Rows.size(); ++I)
		{
			Index = Index < 0 && Net.TemplateOf(HubsInstance(Net, Rows[I])).Id == Id ? static_cast<int>(I) : Index;
		}
		ui_test::Expect(Index >= 0, (std::string("FestLockedMain: ") + Id + " is one of day 19's rows").c_str());
		const int First = std::max(0, static_cast<int>(Rows.size()) - 7);
		if (Index < First)
		{
			continue;
		}
		const float RowY = 408.0f + 72.0f * static_cast<float>(Index - First);
		const bool HasLock = HubsRunBetween(L, Lock, RowY, RowY + 66.0f).first >= 0.0f;
		const bool HasU31 = HubsRunBetween(L, U31, RowY, RowY + 66.0f).first >= 0.0f;
		const bool HasButton = HubsRunsIn(L, 24.0f + 1000.0f - 372.0f, RowY, 24.0f + 1000.0f, RowY + 66.0f) > 0;
		ui_test::Expect(HasLock && HasU31 && HasButton, (std::string("FestLockedMain: ") + Id + "'s row keeps its button with the Main's lock line and U31 beside it").c_str());
		++Ladder;
	}
	ui_test::Expect(Ladder == 5, "FestLockedMain: the five ladder rows are in view once the rows are scrolled to their end");
}

/** FestLockedMain's wallet half: a won seat into the locked Main (a rcop-main ticket in the hero's wallet) survives a save and a
 *  reload (§2.2.3: the seat waits until big fields open). */
void FestLockedMainWallet()
{
	struct HubsSaveHooks : ui_test::QuietHooks
	{
		std::string Last;
		void Save(const ss::SaveData& Data) override { Last = Data.Serialize(); }
	};
	const std::string Ticket = ss::net::Shared().TicketOf("rcop-main");
	HubsSaveHooks H;
	std::string Saved;
	{
		ss::Session S(H, "u4-fest-locked-wallet");
		S.Life.Tickets[Ticket] += 1;
		S.Save();
		Saved = H.Last;
	}
	ss::SaveData Data;
	const bool Parsed = !Saved.empty() && ss::SaveData::Parse(Saved, Data);
	ui_test::Expect(Parsed, "FestLockedMain: the session with a rcop-main ticket saves and parses");
	if (Parsed)
	{
		HubsSaveHooks H2;
		ss::Session Back(H2, "u4-fest-locked-wallet", &Data);
		ui_test::Expect(Back.Life.TicketsFor(Ticket) == 1, ("FestLockedMain: the rcop-main ticket (" + Ticket + ") is in the wallet after the reload").c_str());
	}
}

/** The clock forward to Day at Hour (world time), the hub's laptop fed and rested. */
void HubsTo(HubsLaptop& P, int Day, double Hour)
{
	P.At(static_cast<double>(Day) * ss::net::MinutesPerDay + Hour * 60.0);
}

/** A host's SERIES page with no series picked: its current or next festival (§2.8.1), the pointer away. */
void HubsOpenHost(HubsLaptop& P, ss::net::SiteId Site)
{
	P.RL.SiteShown = Site;
	P.RL.ShowSeries(std::string(), P.Now);
	P.RL.OpenPage(RiverLine::Page::Series, P.Now);
	P.Now = ui_test::Step(P.S, P.Now, 1.5);
	P.Away();
}

/** A rival account through its letter (the invitation, then Accept). */
bool HubsAccept(HubsLaptop& P, ss::net::SiteId Site)
{
	const std::string Key = ss::net::KeyOf(Site);
	P.S.DebugSite("invite " + Key);
	const std::string Why = P.S.AnswerInvite(Site, true);
	const bool Ok = ss::ui::rlsites_detail::SitesAccount(P.S, Site) != nullptr;
	if (!Ok)
	{
		std::printf("  %s: no account (%s)\n", Key.c_str(), Why.c_str());
	}
	return Ok;
}

/** FH-4A: the right column's tab I on RiverLine's hub (fstab0..2 in {1044, 348, 532, 36}), clicked; the settled frame. */
DrawList HubsTab(HubsLaptop& P, int I)
{
	const float W = (532.0f - 12.0f) / 3.0f;
	return P.Click(1044.0f + static_cast<float>(I) * (W + 6.0f) + W / 2.0f, 366.0f);
}

/** FH-4A: the first run exactly Text whose box starts at X0 or right of it: its baseline (-1 when none). */
float HubsBaseRight(const DrawList& L, const std::string& Text, float X0)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == Text && Cmd.Text.X >= X0)
		{
			return Cmd.Text.BaselineY;
		}
	}
	return -1.0f;
}

/** FH-4A: a running event of edition F (a numbered event, headliner or Main; Plain: a RiverLine event of no festival instead) that
 *  started today, still in play: its id ("" for none). */
std::string HubsRunning(HubsLaptop& P, const ss::fest::Festival* F, bool Plain)
{
	const ss::net::Network& Net = ss::net::Site(ss::net::SiteId::RiverLine);
	const double World = P.S.WorldMinutes();
	for (const ss::net::EventInstance& E : Net.Window(static_cast<double>(ss::net::DayOf(World)) * ss::net::MinutesPerDay, World))
	{
		const ss::net::EventTemplate& T = Net.TemplateOf(E);
		const ss::net::Status St = Net.Live(E, World).St;
		const bool InPlay = St == ss::net::Status::LateReg || St == ss::net::Status::Running || St == ss::net::Status::FinalTable;
		const bool Ours = Plain ? ss::fest::FestivalOf(T.Series) == nullptr : (F && T.Series == F->SeriesId && T.Fmt != ss::net::Format::Satellite);
		if (InPlay && Ours && T.BuyInCents > 0)
		{
			return E.Id;
		}
	}
	return std::string();
}

/** FH-4A: the hero plays Id to Place for Prize through the world's public calls (World::HeroEntered, HeroFinished; PLAN_FHUBS
 *  Corrections item 15), the world drained first as the session drains it before telling a finish. */
void HubsHeroFinish(HubsLaptop& P, const std::string& Id, int Place, ss::Chips Prize)
{
	P.S.DrainWorld();
	ss::world::World& W = P.S.Living();
	W.HeroEntered(Id, std::string());
	W.HeroFinished(Id, Place, Prize, ss::world::World::TableReport());
	P.Now = ui_test::Step(P.S, P.Now, 0.2);
}

/** U10's line as FestYourStrip expects it from fest::YourLine's numbers (written here, apart from the hub's HubYourCounts). */
std::string HubsYourWant(const ss::fest::YourLineView& V)
{
	std::string Want = (V.Events == 1 ? std::string("1 event") : ss::Grouped(V.Events) + " events") + " \xC2\xB7 " + (V.Cashes == 1 ? std::string("1 cash") : ss::Grouped(V.Cashes) + " cashes");
	if (V.Best > 0 && V.Field > 0)
	{
		Want += " \xC2\xB7 best " + ss::Ordinal(V.Best) + " of " + ss::Grouped(V.Field);
	}
	if (V.Rank > 0)
	{
		Want += " \xC2\xB7 race " + ss::Ordinal(V.Rank) + " of " + ss::Grouped(V.Scorers);
	}
	return Want;
}

/** FestYourStrip (§2.8.2, §2.15.2; F-hubs' hub half, FH-4A) and fest_hub_your_ladder, on RCOP 2026's first day: before the hero plays,
 *  YOUR RCOP invites (U11) and prints no U10 line; after one cash, the panel prints fest::YourLine's numbers in U10's singular ("1 event
 *  · 1 cash · best ... · race ..."), the seven rungs with PLAY 1 and CASH 1 reached and U27's "{N} pts to {MARK} · right now" for the
 *  next one from fest::MarkCutoff and the hero's points, the YOU row (U7) on THE RACE and the result row; a plain series result adds
 *  nothing. Plant: the U10 line drawn before the hero plays. */
void FestYourStripHub(HubsLaptop& P, const ss::fest::Festival& Rcop)
{
	const DrawList Before = HubsTab(P, 2);
	ui_test::Expect(ui_test::DrewRun(Before, "Play any RCOP 2026 event for your pin.") && !ui_test::Drew(Before, "0 events") && !ui_test::Drew(Before, "1 event"),
		"FestYourStrip: before the hero plays an RCOP 2026 event, YOUR RCOP invites (U11) and prints no U10 line");
	const std::string Id = HubsRunning(P, &Rcop, false);
	ui_test::Expect(!Id.empty(), "FestYourStrip: an RCOP 2026 event is in play on its first day");
	if (Id.empty())
	{
		return;
	}
	const ss::net::Network& Net = ss::net::Site(ss::net::SiteId::RiverLine);
	const ss::net::EventInstance E = HubsInstance(Net, Id);
	HubsHeroFinish(P, Id, 3, std::max<ss::Chips>(100, Net.TemplateOf(E).BuyInCents * 6));
	// The page late on the first day, once the day's first events have finished and the race holds the regulars' points too.
	HubsTo(P, Rcop.FirstDay, 23.5);
	P.OpenHub();
	HubsTab(P, 2);
	const ss::world::World& W = P.S.Living();
	const ss::fest::YourLineView V = ss::fest::YourLine(Rcop, &W);
	const std::pair<int, int> Me = ss::world::FestStanding(W, -1);
	ui_test::Expect(V.Shown && V.Events == 1 && V.Cashes == 1 && Me.first > 0, ("FestYourStrip: the hero's cash in " + Id + " gives RCOP 2026's pin and a place in the race").c_str());
	const DrawList L = HubsPage("fest_hub_your_ladder", P);
	const std::string Want = HubsYourWant(V);
	ui_test::Expect(ui_test::DrewRun(L, Want), ("FestYourStrip: YOUR RCOP prints fest::YourLine's numbers in U10's singular (\"" + Want + "\")").c_str());
	ui_test::Expect(Want == ss::ui::rlfest_detail::HubYourCounts(V, true), "FestYourStrip: the hub's U10 line is the expected one");
	for (const char* Rung : {"PLAY 1", "CASH 1", "TOP 25%", "TOP 10%", "FINALIST", "PODIUM", "THE BELT"})
	{
		ui_test::Expect(HubsBaseRight(L, Rung, 1044.0f) > 392.0f, (std::string("YOUR RCOP: the ladder's rung ") + Rung + " is drawn").c_str());
	}
	// U27: the next mark the hero doesn't hold right now, its cut-off from the live race.
	const std::vector<std::pair<int, ss::world::FestScore>> Race = ss::world::FestTop(W, Me.first);
	const double Mine = Race.empty() ? 0.0 : Race.back().second.Points;
	const ss::world::FestMark Marks[5] = {ss::world::FestMark::Top25, ss::world::FestMark::Top10, ss::world::FestMark::Finalist, ss::world::FestMark::Podium, ss::world::FestMark::Title};
	const char* const Names[5] = {"TOP 25%", "TOP 10%", "FINALIST", "PODIUM", "THE BELT"};
	std::string Line;
	const ss::world::FestMark Held = ss::world::MarkFor(Me.first, Me.second);
	for (int K = 0; K < 5; ++K)
	{
		if (Held == ss::world::FestMark::None || Held < Marks[K])
		{
			const long long N = std::llround(ss::fest::MarkCutoff(W, Marks[K]) - Mine) + 1;
			Line = N > 0 ? ss::Grouped(N) + " pts to " + Names[K] + " \xC2\xB7 right now" : std::string();
			break;
		}
	}
	std::printf("  fest_hub_your_ladder: the hero %s of %d in the race with %.0f pts; U27: %s\n", ss::Ordinal(Me.first).c_str(), Me.second, Mine, Line.empty() ? "(none)" : Line.c_str());
	ui_test::Expect(Line.empty() || ui_test::DrewRun(L, Line), ("YOUR RCOP: U27 reads \"" + Line + "\" (fest::MarkCutoff - the hero's points + 1)").c_str());
	const std::string TName = ss::ui::rlnet_detail::NetShortName(Net.TemplateOf(E).Name);
	ui_test::Expect(ui_test::Drew(L, TName) && ui_test::Drew(L, "3rd"), ("YOUR RCOP: the result row names " + TName + " and the place").c_str());
	// THE RACE: the YOU row (U7).
	const DrawList RaceTab = HubsTab(P, 0);
	const std::string You = "You \xC2\xB7 " + ss::Ordinal(Me.first) + " of " + ss::Grouped(Me.second) + " in the race \xC2\xB7 " + ss::Grouped(static_cast<int64_t>(std::llround(Mine))) + " pts";
	ui_test::Expect(ui_test::DrewRun(RaceTab, You) && HubsBaseRight(RaceTab, You, 1044.0f) > 856.0f, ("THE RACE: the YOU row at the body's foot reads \"" + You + "\" (U7)").c_str());
	ui_test::Expect(ss::ui::rlfest_detail::HubRaceMoves(P.RL) == 0, "THE RACE: no movement before the hub has seen a day turn (never a guess)");
	// PEOPLE YOU KNOW (fsrace1): world::FestKnown's people in race order with their rank in the whole race, else "No points yet.".
	const std::pair<float, float> Known = HubsRunAt(RaceTab, "PEOPLE YOU KNOW");
	const DrawList KnownTab = P.Click(Known.first + 20.0f, Known.second - 4.0f);
	const std::vector<std::pair<int, int>> People = ss::world::FestKnown(W);
	const ss::world::Npc* First = People.empty() ? nullptr : W.Get(People.front().first);
	ui_test::Expect(Known.first > 1044.0f && (First ? ui_test::Drew(KnownTab, First->Name) && ui_test::DrewRun(KnownTab, ss::Grouped(People.front().second))
												 : ui_test::DrewRun(KnownTab, "No points yet.")),
		("THE RACE: PEOPLE YOU KNOW lists world::FestKnown in race order (" + std::to_string(People.size()) + " people)").c_str());
	// None on a plain series event: a plain RiverLine result leaves the panel's numbers as they were.
	const std::string Plain = HubsRunning(P, nullptr, true);
	if (!Plain.empty())
	{
		HubsHeroFinish(P, Plain, 2, 500);
		const ss::fest::YourLineView After = ss::fest::YourLine(Rcop, &W);
		const DrawList Again = HubsTab(P, 2);
		ui_test::Expect(After.Events == 1 && ui_test::Drew(Again, "1 event \xC2\xB7 1 cash"), ("FestYourStrip: a plain series result (" + Plain + ") adds nothing to YOUR RCOP").c_str());
	}
	else
	{
		std::printf("  FestYourStrip: no plain RiverLine event in play on day %d (the plain case skipped)\n", ss::net::DayOf(P.S.WorldMinutes()));
	}
}

/** FestRollTitles (§2.6.2, §2.8.2; FH-4A), day 35: RCOP 2026's roll edition shows MAIN EVENT, MINI MAIN EVENT, MICRO MAIN EVENT and
 *  PLAYER OF THE SERIES in that order inside its 120 units, with the rcop-mini and rcop-micro winners (world::FestRoll's Honors: the
 *  names and "{Prize} · 1st of {Entries}" in NetMoney). Plant: a championship's edition at 88. */
void FestRollTitlesDay35(HubsLaptop& P, const ss::fest::Festival& Rcop)
{
	P.RL.SiteShown = ss::net::SiteId::RiverLine;
	P.RL.ShowSeries("rcop", P.Now);
	P.RL.OpenPage(RiverLine::Page::Series, P.Now);
	P.Now = ui_test::Step(P.S, P.Now, 1.5);
	const DrawList L = HubsTab(P, 1);
	const ss::world::World& W = P.S.Living();
	const ss::world::FestRollRow* Row = nullptr;
	const std::vector<ss::world::FestRollRow> Roll = ss::world::FestRoll(W, ss::fest::Brand::Rcop);
	for (const ss::world::FestRollRow& R : Roll)
	{
		Row = R.Key == Rcop.SeriesId ? &R : Row;
	}
	ui_test::Expect(Row && Row->Mini && Row->Micro, "FestRollTitles: RCOP 2026's roll row holds its Mini and Micro winners on day 35");
	if (!Row || !Row->Mini || !Row->Micro)
	{
		return;
	}
	const float Title = HubsBaseRight(L, "RCOP 2026", 1044.0f);
	const float Main = HubsBaseRight(L, "MAIN EVENT", 1044.0f);
	const float Mini = HubsBaseRight(L, "MINI MAIN EVENT", 1044.0f);
	const float Micro = HubsBaseRight(L, "MICRO MAIN EVENT", 1044.0f);
	const float Pos = Row->Crown ? HubsBaseRight(L, "PLAYER OF THE SERIES", 1044.0f) : Micro + 24.0f;
	ui_test::Expect(Title > 392.0f && Title < Main && Main < Mini && Mini < Micro && Micro < Pos && Pos <= Title - 22.0f + ss::ui::rlfest_detail::HubRollPitch(true),
		("FestRollTitles: RCOP 2026's four lines in order inside 120 units (title " + std::to_string(Title) + ", last line " + std::to_string(Pos) + ")").c_str());
	for (const ss::world::Honor* Hn : {Row->Mini, Row->Micro})
	{
		const std::string Fig = ss::ui::rlnet_detail::NetMoney(Hn->Prize) + " \xC2\xB7 1st of " + ss::Grouped(Hn->Entries);
		ui_test::Expect(ui_test::Drew(L, Hn->Name) && ui_test::DrewRun(L, Fig), ("FestRollTitles: " + Hn->EventId + "'s winner " + Hn->Name + " and \"" + Fig + "\" on the roll").c_str());
	}
}

/** FestRollTitles' pitch and fest_hub_champ_roll, day 400: RCOP 2027's hub (CHAMPIONS) holds RCOP 2027 and RCOP 2026, newest first; the
 *  drawn pitch between their titles is 120 and four of it fit 558 [554] and three Ignite's 478. */
void FestRollTitlesDay400(HubsLaptop& P)
{
	P.RL.SiteShown = ss::net::SiteId::RiverLine;
	P.RL.ShowSeries("rcop27", P.Now);
	P.RL.OpenPage(RiverLine::Page::Series, P.Now);
	P.Now = ui_test::Step(P.S, P.Now, 1.5);
	HubsTab(P, 1);
	const DrawList L = HubsPage("fest_hub_champ_roll", P);
	const float New = HubsBaseRight(L, "RCOP 2027", 1044.0f);
	const float Old = HubsBaseRight(L, "RCOP 2026", 1044.0f);
	const float Pitch = Old - New;
	ui_test::Expect(New > 392.0f && Old > New, "fest_hub_champ_roll: RCOP 2027 above RCOP 2026 on one roll (newest first)");
	ui_test::Expect(Pitch == 120.0f && 4.0f * Pitch <= 554.0f && 3.0f * Pitch <= 478.0f,
		("FestRollTitles: a championship edition's drawn pitch is 120, four fit 558 [554] and three Ignite's 478 (" + std::to_string(Pitch) + ")").c_str());
}

/** Career A (FH-2; PLAN_FHUBS FH-2's two careers): FEIGN 2026 on Ignite, day 56, an Ignite account accepted: fest_hub_ig_live. The
 *  hub in Ignite's frame (L7): the strip and banner in RiverLine's rects, the rows cut at y 870 (no row text below it; six whole and a
 *  seventh under the fade), no overlay ticker, THE RACE preselected while Live. */
void FestCareerIgnite()
{
	HubsLaptop P("u4-fest-career-a");
	const ss::fest::Festival* Feign = ss::fest::FestivalOf("ignite.feign26");
	ui_test::Expect(Feign != nullptr && Feign->FirstDay == 56, "fest_hub_ig_live: FEIGN 2026 opens on day 56 (F-net's FN-2a row)");
	if (!Feign)
	{
		return;
	}
	HubsTo(P, 40, 12.0);
	ui_test::Expect(HubsAccept(P, ss::net::SiteId::Ignite), "fest_hub_ig_live: Ignite's invitation accepted");
	HubsTo(P, 56, 18.0);
	HubsOpenHost(P, ss::net::SiteId::Ignite);
	const DrawList L = HubsPage("fest_hub_ig_live", P);
	ui_test::Expect(ui_test::DrewRun(L, "FEIGN") && ui_test::DrewRun(L, "DAY 1 OF 10"), "fest_hub_ig_live: Ignite's SERIES page is FEIGN 2026's hub on its first day (DAY 1 OF 10)");
	ui_test::Expect(ui_test::DrewRun(L, "PLAYER OF THE SERIES"), "fest_hub_ig_live: THE RACE is preselected while Live");
	ui_test::Expect(HubsRunsIn(L, 24.0f, 870.0f, 1024.0f, 884.0f) == 0, "fest_hub_ig_live: no row text below y 870 (the rows end above Ignite's dock)");
	ui_test::Expect(HubsRunsIn(L, 1044.0f, 870.0f, 1576.0f, 884.0f) == 0, "fest_hub_ig_live: the right column's body ends at y 870");
	FestStripTenPage(L, HubsStripRiverLine, P.S.WorldMinutes());
	const std::vector<std::string> Rows = P.RL.ListedEvents();
	std::printf("  fest_hub_ig_live: %zu rows on FEIGN 2026's first day (six whole, the seventh under the fade)\n", Rows.size());
}

/** FestCopyUi's drawn U35 half (§2.8.2; FH-4A fix round 1): REIGN 2027's step rows on RunItTwice's hub (rows {232, 412, 860, 504}) on
 *  day 302 (Step 2's first day) and day 303 (Step 1's second, Step 3's first), at noon. Each step row of the edition draws STEP {K} on
 *  its first line and its seat pill on its second, ending 4 short of the entries' column (x 582), and no SATELLITE; K and the seat are read
 *  here from the row's own template (H22's "Step {K} · REIGN Main" and its seats text, "Step {K} tickets (...)" or "REIGN Main Event
 *  seats"), not from the hub's ladder. Adds each form drawn to Seen. Plants: FOR A MAIN EVENT SEAT on Step 1; the STEP {K} pill
 *  dropped; the seat pill kept at 9.5 in RunItTwice's narrower column. */
int FestStepPillsReignDay(HubsLaptop& P, const ss::fest::Festival& Reign, int Day, std::vector<std::string>& Seen)
{
	HubsTo(P, Day, 12.0);
	HubsOpenHost(P, ss::net::SiteId::RunItTwice);
	const DrawList L = P.Frame(1.0f);
	const ss::net::Network& Rit = ss::net::Site(ss::net::SiteId::RunItTwice);
	ui_test::TableMeasurer M;
	DrawList Scratch;
	ss::ui::Canvas C(Scratch, M, RiverLine::Width, RiverLine::Height, 1.0f);
	const std::vector<std::string> Rows = P.RL.ListedEvents();
	int Steps = 0;
	for (size_t I = 0; I < Rows.size() && I < 7; ++I)
	{
		const ss::net::EventTemplate& T = Rit.TemplateOf(HubsInstance(Rit, Rows[I]));
		const bool Own = std::find(Reign.Satellites.begin(), Reign.Satellites.end(), T.Id) != Reign.Satellites.end();
		if (!Own || T.Fmt != ss::net::Format::Satellite || T.Name.size() < 6 || T.Name.compare(0, 5, "Step ") != 0)
		{
			continue;
		}
		const std::string Rung = "STEP " + T.Name.substr(5, 1);
		const std::string SeatPill = T.Seats.compare(0, 5, "Step ") == 0 ? "FOR A STEP " + T.Seats.substr(5, 1) + " TICKET" : std::string("FOR A MAIN EVENT SEAT");
		const float RowY = 412.0f + 72.0f * static_cast<float>(I);
		const bool HasRung = HubsRunBetween(L, Rung, RowY, RowY + 32.0f).first >= 0.0f;
		const ss::ui::TextItem* SeatRun = nullptr;
		bool Sat = false;
		for (const ss::ui::DrawCmd& Cmd : L.Cmds)
		{
			if (Cmd.Type != ss::ui::DrawCmd::Kind::Text || Cmd.Text.BaselineY < RowY || Cmd.Text.BaselineY >= RowY + 66.0f)
			{
				continue;
			}
			SeatRun = Cmd.Text.Text == SeatPill && Cmd.Text.BaselineY >= RowY + 32.0f ? &Cmd.Text : SeatRun;
			Sat = Sat || Cmd.Text.Text == "SATELLITE";
		}
		const float SeatRight = SeatRun ? SeatRun->X + C.Measure(SeatPill, SeatRun->SizePx, SeatRun->Face) + SeatRun->SizePx * 0.65f : -1.0f;
		ui_test::Expect(HasRung && SeatRun && SeatRight <= 578.0f && !Sat,
			("FestCopyUi: U35 on day " + std::to_string(Day) + ": " + T.Name + "'s row draws " + Rung + " and " + SeatPill + " (its pill ends at x " +
				std::to_string(static_cast<int>(SeatRight)) + ", by 578: 4 short of the entries at 582), no SATELLITE")
				.c_str());
		if (HasRung && SeatRun)
		{
			Seen.push_back(Rung + " / " + SeatPill);
			std::printf("  U35 on day %d, row %zu (%s): %s / %s, the seat pill at %.1f ends at x %.1f (by 578, the entries at 582)\n", Day, I, Rows[I].c_str(), Rung.c_str(),
				SeatPill.c_str(), static_cast<double>(SeatRun->SizePx), static_cast<double>(SeatRight));
		}
		++Steps;
	}
	return Steps;
}

void FestStepPillsReign(HubsLaptop& P)
{
	const ss::fest::Festival* Reign = ss::fest::FestivalOf("runittwice.reign27");
	ui_test::Expect(Reign && Reign->FirstDay == 294, "FestCopyUi: REIGN 2027 opens on day 294 (F-net's FN-3 row)");
	if (!Reign)
	{
		return;
	}
	std::vector<std::string> Seen;
	const int On302 = FestStepPillsReignDay(P, *Reign, 302, Seen);
	const int On303 = FestStepPillsReignDay(P, *Reign, 303, Seen);
	std::string List;
	for (const std::string& S : Seen)
	{
		List += (List.empty() ? "" : " | ") + S;
	}
	const auto Has = [&Seen](const char* Form) { return std::find(Seen.begin(), Seen.end(), std::string(Form)) != Seen.end(); };
	ui_test::Expect(On302 == 1 && On303 == 2 && Has("STEP 2 / FOR A STEP 3 TICKET") && Has("STEP 1 / FOR A STEP 2 TICKET") && Has("STEP 3 / FOR A MAIN EVENT SEAT"),
		("FestCopyUi: U35's three rungs drawn on REIGN 2027's days 302 and 303 (" + std::to_string(On302) + " + " + std::to_string(On303) + " step rows: " + List + ")").c_str());
}

/** Career B (FH-2): RiverLine's and RunItTwice's hubs walked forward in day order, never opening Ignite: fest_strip_night_one (Night
 *  One, the pointer resting on FEIGN's cell), fest_hub_rl_live (RCOP 2026's first day), fest_hub_rl_locked (day 19, the rows
 *  scrolled to the ladder), The Summons' seats on WRIT 2027 (U33, no page), fest_hub_rit_twice (GRIT 2027, day 220: two runs a
 *  night), REIGN 2027's step rows (U35, days 302 and 303, no page; FH-4A fix round 1), Summer Slam 2027's locked Main (no page), and fest_popover_pass (FEIGN II's cell on day 395, BY INVITATION until F-world's
 *  FW-7 sets the Pass; FH-7 emits the counting form). */
void FestCareerRiverLine()
{
	HubsLaptop P("u4-fest-career-b");
	const ss::fest::Festival* Rcop = ss::fest::FestivalOf("rcop");
	const ss::fest::Festival* Writ = ss::fest::FestivalOf("runittwice.writ27");
	const ss::fest::Festival* Grit = ss::fest::FestivalOf("runittwice.grit27");
	const ss::fest::Festival* Slam = ss::fest::FestivalOf("slam27");
	ui_test::Expect(Rcop && Writ && Grit && Slam, "FH-2: RCOP 2026, WRIT 2027, GRIT 2027 and Summer Slam 2027 are F-net's rows");
	if (!Rcop || !Writ || !Grit || !Slam)
	{
		return;
	}
	// Night One: the ten-cell strip.
	P.OpenHub();
	P.RL.UI.Ptr.Active = true;
	P.RL.UI.Ptr.X = 260.0f + 9.0f * 128.0f + 50.0f; // resting on FEIGN's cell (fsy9: BY INVITATION)
	P.RL.UI.Ptr.Y = 104.0f;
	const DrawList NightOne = HubsPage("fest_strip_night_one", P);
	P.Away();
	const int Cells = FestStripTenPage(NightOne, HubsStripRiverLine, P.S.WorldMinutes());
	ui_test::Expect(Cells == 10, ("FestStripTen: Night One's strip holds all ten brands (" + std::to_string(Cells) + ")").c_str());
	ui_test::Expect(HubsRunsIn(NightOne, 260.0f + 9.0f * 128.0f, 120.0f, 260.0f + 10.0f * 128.0f, 124.0f) == 1 && ui_test::DrewRun(NightOne, "BY INVITATION"),
		"FestStripTen: FEIGN's cell without an Ignite account reads BY INVITATION (no Pass before F-world's FW-7)");
	HubsStripClicks(P, HubsStripRiverLine);
	// RCOP 2026's first day: the Main card's lock line, THE RACE first.
	HubsTo(P, Rcop->FirstDay, 12.0);
	P.OpenHub();
	const DrawList Live = HubsPage("fest_hub_rl_live", P);
	const std::string Lock = HubsMainLock(P.S, *Rcop);
	ui_test::Expect(!Lock.empty() && ui_test::DrewRun(Live, Lock), ("FestLockedMain: RCOP 2026's Main card shows the lock line (\"" + Lock + "\")").c_str());
	ui_test::Expect(ui_test::DrewRun(Live, "PLAYER OF THE SERIES") && !ui_test::Drew(Live, "No champion yet."), "FestLockedMain: the hub opens on THE RACE while Live, never past it");
	ui_test::Expect(ui_test::DrewRun(Live, "DAY 1 OF 22"), "fest_hub_rl_live: RCOP 2026's first day reads DAY 1 OF 22");
	// FH-4A: the YOUR panel before and after the hero's first RCOP event (fest_hub_your_ladder), then THE RACE's movement on days 14-15.
	FestYourStripHub(P, *Rcop);
	for (const int Day : {14, 15})
	{
		HubsTo(P, Day, 12.0);
		P.OpenHub();
		P.Frame(1.0f);
		std::printf("  THE RACE on day %d: %d movement marks (the hub saw the day before)\n", Day, ss::ui::rlfest_detail::HubRaceMoves(P.RL));
	}
	// Day 19 (a Saturday): the ladder in view.
	HubsTo(P, 19, 12.0);
	P.OpenHub();
	for (int K = 0; K < 12; ++K)
	{
		P.RL.UI.Ptr.Active = true;
		P.RL.UI.Ptr.X = 300.0f;
		P.RL.UI.Ptr.Y = 600.0f;
		P.RL.UI.Ptr.Wheel = 1.0f;
		P.Frame(1.0f);
	}
	P.Away();
	const DrawList Locked = HubsPage("fest_hub_rl_locked", P);
	FestLockedMainRows(P, *Rcop, Locked);
	ui_test::Expect(ui_test::DrewRun(Locked, Lock), "FestLockedMain: the Main card keeps its lock line on day 19");
	ui_test::Expect(ss::fest::YourLine(*Rcop, &P.S.Living()).Shown && ui_test::DrewRun(Locked, "PLAYER OF THE SERIES"),
		"FestLockedMain: with the hero's RCOP pin the hub still opens on THE RACE while the Main is locked");
	// FH-4A, FestRollTitles: RCOP 2026's title lines on day 35.
	HubsTo(P, 35, 12.0);
	FestRollTitlesDay35(P, *Rcop);
	// WRIT 2027: a RunItTwice account, then The Summons' seats (U33) on its day.
	HubsTo(P, Writ->FirstDay, 12.0);
	ui_test::Expect(HubsAccept(P, ss::net::SiteId::RunItTwice), "FH-2: RunItTwice's invitation accepted");
	{
		const ss::net::Network& Rit = ss::net::Site(ss::net::SiteId::RunItTwice);
		ss::net::EventInstance Sum;
		const bool Has = !Writ->Satellites.empty() && Rit.Next(Writ->Satellites.front(), static_cast<double>(Writ->FirstDay) * ss::net::MinutesPerDay, Sum);
		ui_test::Expect(Has, "U33: The Summons runs inside WRIT 2027");
		if (Has)
		{
			P.At(Sum.Start - 20.0);
			HubsOpenHost(P, ss::net::SiteId::RunItTwice);
			P.RL.UI.Ptr.Active = false;
			const ss::net::LiveState Ls = Rit.Live(Sum, P.S.WorldMinutes());
			const ss::net::EventTemplate& T = Rit.TemplateOf(Sum);
			const long long Seats = std::max<long long>(1, Ls.Pool / std::max<ss::Chips>(1, T.SeatValueCents));
			const DrawList W = P.Frame(1.0f);
			const std::string Want = Seats == 1 ? std::string("1 SEAT AT THE POOL SO FAR") : std::to_string(Seats) + " SEATS AT THE POOL SO FAR";
			ui_test::Expect(Ls.Pool > 0 && ui_test::DrewRun(W, Want), ("U33: The Summons' row reads \"" + Want + "\" (max(1, Pool / SeatValue) while it registers)").c_str());
			P.At(Sum.Start + Sum.Duration + 30.0);
			// Past midnight the hub shows the next day: its day strip's cell for The Summons' day brings it back (fsd<day>).
			const int SumDay = ss::net::DayOf(Sum.Start);
			const float Cw = (860.0f - 4.0f * static_cast<float>(Writ->LastDay - Writ->FirstDay)) / static_cast<float>(Writ->LastDay - Writ->FirstDay + 1);
			P.Click(232.0f + static_cast<float>(SumDay - Writ->FirstDay) * (Cw + 4.0f) + Cw / 2.0f, 378.0f);
			const DrawList Done = P.Frame(1.0f);
			const ss::net::EventInstance Fin = HubsInstance(Rit, Sum.Id);
			const long long Won = std::max<long long>(1, Fin.Pool / std::max<ss::Chips>(1, T.SeatValueCents));
			const std::string WonText = Won == 1 ? std::string("1 SEAT WON") : std::to_string(Won) + " SEATS WON";
			ui_test::Expect(Rit.Live(Sum, P.S.WorldMinutes()).St != ss::net::Status::Finished || ui_test::DrewRun(Done, WonText),
				("U33: once finished The Summons' row reads \"" + WonText + "\"").c_str());
		}
	}
	// GRIT 2027's second night: two runs a night.
	HubsTo(P, Grit->FirstDay + 1, 18.0);
	HubsOpenHost(P, ss::net::SiteId::RunItTwice);
	const DrawList Twice = HubsPage("fest_hub_rit_twice", P);
	FestStripTenPage(Twice, HubsStripRunItTwice, P.S.WorldMinutes());
	ui_test::Expect(ui_test::DrewRun(Twice, "NIGHT 2 OF 12"), "fest_hub_rit_twice: GRIT's banner counts nights (U2: NIGHT 2 OF 12)");
	ui_test::Expect(ui_test::DrewRun(Twice, "NIGHT 1") && ui_test::DrewRun(Twice, "NIGHT 12"), "fest_hub_rit_twice: GRIT's day strip reads nights");
	ui_test::Expect(ui_test::DrewRun(Twice, "7 PM") && ui_test::DrewRun(Twice, "10 PM"), "fest_hub_rit_twice: a show's row carries its two buttons, 7 PM and 10 PM");
	const std::vector<std::string> Listed = P.RL.ListedEvents();
	const ss::net::Network& Rit = ss::net::Site(ss::net::SiteId::RunItTwice);
	std::string EarlyId;
	std::string LateId;
	for (size_t I = 0; I + 1 < Listed.size() && EarlyId.empty(); ++I)
	{
		const ss::net::EventInstance A = HubsInstance(Rit, Listed[I]);
		const ss::net::EventInstance B = HubsInstance(Rit, Listed[I + 1]);
		if (A.Template == B.Template && Rit.TemplateOf(A).Series == Grit->SeriesId)
		{
			EarlyId = A.Id;
			LateId = B.Id;
		}
	}
	ui_test::Expect(!EarlyId.empty(), "fest_hub_rit_twice: the night's show lists its two runs");
	if (!EarlyId.empty())
	{
		const std::string CodeE = Rit.NameOf(HubsInstance(Rit, EarlyId)).substr(0, Rit.NameOf(HubsInstance(Rit, EarlyId)).find(": "));
		const std::string CodeL = Rit.NameOf(HubsInstance(Rit, LateId)).substr(0, Rit.NameOf(HubsInstance(Rit, LateId)).find(": "));
		ui_test::Expect(CodeE.size() > 2 && CodeE.compare(CodeE.size() - 2, 2, "-E") == 0 && CodeL.compare(CodeL.size() - 2, 2, "-L") == 0 && ui_test::DrewRun(Twice, CodeE) &&
				ui_test::DrewRun(Twice, CodeL),
			("fest_hub_rit_twice: the buttons stand beside the runs' codes (" + CodeE + ", " + CodeL + ")").c_str());
		const std::pair<float, float> Ten = HubsRunAt(Twice, "10 PM");
		P.Click(Ten.first + 8.0f, Ten.second - 4.0f);
		ui_test::Expect(P.RL.SelectedEvent() == LateId, "fest_hub_rit_twice: 10 PM picks the late run (its register button the late run's)");
		ui_test::Expect(!P.S.IsPlaying(LateId) && !P.S.ConfirmRegister, "fest_hub_rit_twice: picking a run registers nothing");
		// FestCopyUi's U15 half (FestBlurbsFilled): the ticker's GRIT line is the edition's own night, nights and show times.
		const ss::net::EventInstance E = HubsInstance(Rit, EarlyId);
		const ss::net::EventInstance Lt = HubsInstance(Rit, LateId);
		auto ShowOf = [](double Start) {
			const long long M = static_cast<long long>(std::llround(Start)) % 1440;
			const long long H = M / 60;
			return std::to_string(H % 12 == 0 ? 12 : H % 12) + (H < 12 ? " AM" : " PM");
		};
		const std::string Line = "GRIT \xC2\xB7 NIGHT 2 OF " + std::to_string(Grit->LastDay - Grit->FirstDay + 1) + " \xC2\xB7 SHOWS AT " + ShowOf(E.Start) + " AND " + ShowOf(Lt.Start);
		const DrawList Again = P.Frame(1.0f);
		ui_test::Expect(ui_test::Drew(Again, Line), ("FestCopyUi: GRIT's ticker line is the edition's own (\"" + Line + "\", U15)").c_str());
	}
	// FH-4A fix round 1: REIGN 2027's step rows (U35) on days 302 and 303.
	FestStepPillsReign(P);
	// Summer Slam 2027 (its Main locked until G3, §2.2.3): the Main card's lock line.
	HubsTo(P, Slam->FirstDay + 1, 12.0);
	P.OpenHub();
	const DrawList SlamL = P.Frame(1.0f);
	const std::string SlamLock = HubsMainLock(P.S, *Slam);
	ui_test::Expect(!SlamLock.empty() && ui_test::DrewRun(SlamL, SlamLock) && ui_test::DrewRun(SlamL, "YOUR " + HubsShortOf(*Slam)),
		("FestLockedMain: Summer Slam 2027's Main card shows the lock line (\"" + SlamLock + "\")").c_str());
	// Day 395: FEIGN II's cell (Announced from day 391), never an Ignite account in this career.
	HubsTo(P, 395, 12.0);
	P.OpenHub();
	const DrawList Before = P.Frame(1.0f);
	const ss::fest::Festival* Feign2 = ss::fest::FestivalOf("ignite.feign27");
	ui_test::Expect(Feign2 && Feign2->Edition == 2 && ss::fest::PhaseOf(*Feign2, P.S.WorldMinutes(), nullptr) == ss::fest::Phase::Announced,
		"fest_popover_pass: FEIGN 2027 (edition II) is Announced on day 395");
	ui_test::Expect(HubsRunsIn(Before, 260.0f + 9.0f * 128.0f, 120.0f, 260.0f + 10.0f * 128.0f, 124.0f) == 1, "fest_popover_pass: FEIGN II's cell carries its third line");
	P.Click(260.0f + 9.0f * 128.0f + 50.0f, 104.0f, true);
	const DrawList Pop = HubsPage("fest_popover_pass", P);
	ui_test::Expect(ui_test::DrewRun(Pop, "FEIGN 2027") && ui_test::DrewRun(Pop, "By invitation.") && !ui_test::DrewRun(Pop, "OPEN THE HUB"),
		"fest_popover_pass: FEIGN II's popover reads By invitation. with no button (BY INVITATION until F-world's Pass, FW-7)");
	// FH-4A: day 400, RCOP 2026 and 2027 on one roll.
	HubsTo(P, 400, 12.0);
	FestRollTitlesDay400(P);
}
} // namespace

namespace u4_ui_test
{
void SegFest()
{
	const auto T0 = std::chrono::steady_clock::now();
	FestHubPagesRiverLine();
	FestPopoverHubs();
	FestStrikeGateHubs();
	FestCopyUiHubs();
	// FH-2: the real calendar in three frames.
	FestStripTenMeasures();
	FestLockedMainWallet();
	FestCareerIgnite();
	FestCareerRiverLine();
	std::printf("u4_ui SegFest (F-hubs A): %.1fs\n", std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count());
}
} // namespace u4_ui_test
