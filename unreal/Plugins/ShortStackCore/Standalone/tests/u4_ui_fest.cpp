// u4_ui: the festival half's hub and moments (update 4 SPEC v1.7 §2.15.2; F-hubs' file): the hub in three frames, Main Event Day,
// the moments, the strip and popover, the pin wall. Called from u4_ui_main.cpp (the kit).
// Phase A (update 4 SPEC v1.7 §6 PA-48): registered, empty.
// G1 FH-1a (F-hubs' stream A; PLAN_G1 §2.1, §3.2; PLAN_FHUBS §4 and its Corrections): RiverLine's hub on RCOP 2026 (F-net's FN-1 row,
// routed by stream B's FH-1c), its first two pages and FestHonestLines' A half. Every page is emitted at scale 1.0 (the kit's Emit,
// under SS_U4_UI_OUT) and measured at 1.0 and 1.5 (vertices, text runs, NetSpaced lines, commands; Q18); SS_UI_BENCH=1 adds the warm
// draw (100 draws after one cold). §2.8.10's hub row is printed only here: the art is F-look's stubs until FH-7 asserts it. Helpers
// sit in an anonymous namespace and carry "Hubs" (the hosts' file, u4_ui_fest_hosts.cpp, and F-look's share the binary).
#include "UiSeg.h"

#include "ShortStack/Game/Festival.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/UI/RiverLine.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
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
};

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
}
} // namespace

namespace u4_ui_test
{
void SegFest()
{
	const auto T0 = std::chrono::steady_clock::now();
	FestHubPagesRiverLine();
	std::printf("u4_ui SegFest (F-hubs A): %.1fs\n", std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count());
}
} // namespace u4_ui_test
