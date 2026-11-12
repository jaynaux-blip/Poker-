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
#include "UiSeg.h"

#include "CopyLint.h"
#include "Trademarks.h"
#include "ShortStack/Game/Festival.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/UI/RiverLine.h"
#include "../../Source/ShortStackCore/Private/ShortStack/UI/RiverLineShared.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <utility>
#include <vector>

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
		{"Overlay", "$412,600"}, {"Amount", "$1,280,000"}, {"Entries", "2,345"}, {"Countdown", "2H 10M"}, {"Clock", "0:41:12"}};
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
	// U2's formats (FestBlurbsFilled' U2 half): in the inventory, and no digit typed in any.
	for (const char* U2 : {"OPENS IN", "{D}D {H}H {M}M", "PROGRESS", "DAY {N} OF {Days}", "MAIN EVENT TODAY", "COMPLETE"})
	{
		const bool Listed = std::find(Inv.begin(), Inv.end(), std::string("label\t") + U2) != Inv.end();
		const bool Typed = std::string(U2).find_first_of("0123456789") != std::string::npos;
		ui_test::Expect(Listed && !Typed, (std::string("FestCopyUi: U2's \"") + U2 + "\" is in FestUiInventory with no digit typed").c_str());
	}
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
	std::printf("u4_ui SegFest (F-hubs A): %.1fs\n", std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count());
}
} // namespace u4_ui_test
