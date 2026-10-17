// riverline_ui: ui-laptop's autoplay additions (update 2 SPEC §2.11, §6.7): auto_panel, auto_panel_greyed, auto_watch_strip
// (no move shown before it's played), table_skip (replacing table_sprint), auto_tile_status, results_auto,
// career_auto_row. Compiled into ui_test and called from main() in ui_main.cpp (the kit is UiSeg.h). Phase A, day 1 (the
// lead): the table_sprint draw list, moved here from ui_main.cpp's Screens; ui-laptop fills the rest, its helpers in
// ui_test::auto_seg.
//
// What's checked, through the draw lists and the session (no private header):
// - the pace row reads FULL · SMART · AUTO; AUTO opens the panel ("AI AUTOPLAY"); K, Backspace and a click off it close it;
// - the panel: eleven tiles in a 4-3-4 grid, each edge line whole inside its tile, greyed tiles saying why; picking a style,
//   a speed, a target and the stop rules goes to the session; "Make this my default"; the main button turns it on;
// - the watch strip: the think line, never a move before it's played; the action buttons during the think take the
//   wheel and do nothing else, the next press acts; the last move once played; AUTO RESUMES and K in it;
// - the skip card (table_skip) and K, Backspace and its buttons; the tile and tab lines (auto_tile_status);
// - the results line and the "—" accuracy (results_auto); Career's AUTO tag and "—" (career_auto_row);
// - everything fits the laptop's 1600-unit width, and the copy lint over this track's autoplay sentences.
#include "UiSeg.h"

#include "CopyLint.h"
#include "ShortStack/AI/Style.h"
#include "ShortStack/Game/Autoplay.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <set>

namespace ui_test
{
namespace auto_seg
{
using ss::ui::DrawList;

/** A session at a table and its laptop, the pointer driven by hand, a frame drawn for every step. */
struct AutoDesk
{
	QuietHooks H;
	ss::Session S;
	ss::ui::RiverLine RL;
	TableMeasurer M;
	double Now = 1.0;
	explicit AutoDesk(const std::string& Seed) : S(H, Seed), RL(S)
	{
		S.CurrentScreen = ss::Screen::Lobby;
		Now = Step(S, Now, 0.2);
	}
	DrawList Frame(float X = -1.0f, float Y = -1.0f, bool Down = false, bool Pressed = false, bool Released = false)
	{
		RL.UI.Ptr.Active = X >= 0.0f;
		RL.UI.Ptr.X = X;
		RL.UI.Ptr.Y = Y;
		RL.UI.Ptr.Down = Down;
		RL.UI.Ptr.Pressed = Pressed;
		RL.UI.Ptr.Released = Released;
		DrawList L;
		ss::ui::Canvas C(L, M, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, 1.0f);
		RL.Draw(C, Now);
		RL.UI.Ptr.EndFrame();
		return L;
	}
	void Click(float X, float Y)
	{
		Frame(X, Y, true, true, false);
		Now += 0.05;
		Frame(X, Y, false, false, true);
		Now += 0.05;
	}
	/** The session run for some seconds, a frame drawn every step (the pointer away). */
	void Run(double Seconds)
	{
		const double End = Now + Seconds;
		while (Now < End)
		{
			Now += 1.0 / 30.0;
			S.Update(Now);
			Frame();
		}
	}
	/** Runs until Done() (a frame each step) or the guard; true when it happened. */
	template <typename F>
	bool Until(F Done, double MaxSeconds)
	{
		const double End = Now + MaxSeconds;
		while (!Done() && Now < End)
		{
			Now += 1.0 / 30.0;
			S.Update(Now);
			Frame();
		}
		return Done();
	}
	ss::autoplay::Panel Plan() const { return S.AutoPanel(S.FocusedTable()); }
	/** Seated at the Lobby's first event with a hand being dealt. */
	bool Seat()
	{
		S.Register(0);
		return Until([&]() { return S.CurrentScreen == ss::Screen::Table && S.CurHand && !S.CurHand->bComplete; }, 30.0);
	}
};

/** A text run that is exactly Text: its left, baseline and width (the measurer's). */
struct AutoRun
{
	bool Found = false;
	float X = 0.0f;
	float Base = 0.0f;
	float W = 0.0f;
	float Size = 0.0f;
	size_t Index = 0;
};
AutoRun RunOf(const DrawList& L, const std::string& Text, const TableMeasurer& M, size_t From = 0)
{
	AutoRun Out;
	for (size_t I = From; I < L.Cmds.size(); ++I)
	{
		const ss::ui::DrawCmd& Cmd = L.Cmds[I];
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == Text)
		{
			Out.Found = true;
			Out.X = Cmd.Text.X;
			Out.Base = Cmd.Text.BaselineY;
			Out.Size = Cmd.Text.SizePx;
			Out.W = M.Width(Text, Cmd.Text.Face, Cmd.Text.SizePx);
			Out.Index = I;
			return Out;
		}
	}
	return Out;
}

/** NetSpaced's tracked capitals (one run a glyph), joined up per line. */
std::vector<std::pair<std::string, size_t>> SpacedRuns(const DrawList& L)
{
	std::vector<std::pair<std::string, size_t>> Out;
	std::string Cur;
	size_t At = 0;
	float Base = -1.0e9f;
	for (size_t I = 0; I < L.Cmds.size(); ++I)
	{
		const ss::ui::DrawCmd& Cmd = L.Cmds[I];
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text)
		{
			continue;
		}
		const std::string& T = Cmd.Text.Text;
		const bool Glyph = !T.empty() && (T.size() == 1 || (static_cast<unsigned char>(T[0]) >= 0xC0 && T.size() <= 4));
		if (Glyph && Cmd.Text.BaselineY == Base)
		{
			Cur += T;
			continue;
		}
		if (!Cur.empty())
		{
			Out.push_back({Cur, At});
		}
		Cur = Glyph ? T : std::string();
		At = I;
		Base = Glyph ? Cmd.Text.BaselineY : -1.0e9f;
	}
	if (!Cur.empty())
	{
		Out.push_back({Cur, At});
	}
	return Out;
}

/** Where tracked capitals spell Word: the command index of its first glyph, or -1. */
long SpelledAt(const DrawList& L, const std::string& Word)
{
	for (const auto& S : SpacedRuns(L))
	{
		if (S.first.find(Word) != std::string::npos)
		{
			return static_cast<long>(S.second);
		}
	}
	return -1;
}

/** The panel's rect at one table in front (RiverLine.cpp's AutoPanel call: over the felt). */
const ss::ui::Rect PanelRect{590.0f - 554.0f, 64.0f + 14.0f, 1108.0f, 870.0f};

bool Inside(const ss::ui::Rect& R, float X, float Y)
{
	return X >= R.X && X <= R.X + R.W && Y >= R.Y && Y <= R.Y + R.H;
}

/** Every text run from command From on whose baseline lies inside R keeps inside it, left and right: the count of those that don't (printed). */
int Overflows(const DrawList& L, const ss::ui::Rect& R, size_t From, const TableMeasurer& M, const char* What)
{
	int Bad = 0;
	for (size_t I = From; I < L.Cmds.size(); ++I)
	{
		const ss::ui::DrawCmd& Cmd = L.Cmds[I];
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text || !Inside(R, Cmd.Text.X + 1.0f, Cmd.Text.BaselineY))
		{
			continue;
		}
		const float W = M.Width(Cmd.Text.Text, Cmd.Text.Face, Cmd.Text.SizePx);
		if (Cmd.Text.X < R.X - 0.5f || Cmd.Text.X + W > R.X + R.W + 0.5f)
		{
			++Bad;
			std::printf("  %s: \"%s\" at x %.1f, %.1f wide, leaves %.0f..%.0f\n", What, Cmd.Text.Text.c_str(), Cmd.Text.X, W, R.X, R.X + R.W);
		}
	}
	return Bad;
}

/** Greedy word wrap with the test's metrics (NetWrap's rule): how many lines Text takes at MaxW. */
int WrappedLines(const std::string& Text, float MaxW, float Size, ss::ui::Font Face, const TableMeasurer& M)
{
	int Lines = 0;
	std::string Line;
	size_t Start = 0;
	while (Start <= Text.size())
	{
		size_t End = Text.find(' ', Start);
		End = End == std::string::npos ? Text.size() : End;
		const std::string Word = Text.substr(Start, End - Start);
		const std::string Test = Line.empty() ? Word : Line + " " + Word;
		if (M.Width(Test, Face, Size) > MaxW && !Line.empty())
		{
			++Lines;
			Line = Word;
		}
		else
		{
			Line = Test;
		}
		Start = End + 1;
	}
	return Lines + (Line.empty() ? 0 : 1);
}

/** Any of the plan's action words as the strip says them ("CALLED 3,200"): a move on screen. */
bool DrewAMove(const DrawList& L)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text)
		{
			continue;
		}
		const std::string& T = Cmd.Text.Text;
		for (const char* Word : {"FOLDED", "CHECKED", "CALLED ", "BET ", "RAISED TO ", "ALL IN "})
		{
			if (T.rfind(Word, 0) == 0)
			{
				return true;
			}
		}
	}
	return false;
}

/** The centre of the drawn button whose label is Label (its run), or {-1, -1}. */
ss::ui::Vec2 ButtonAt(const DrawList& L, const std::string& Label, const TableMeasurer& M)
{
	const AutoRun R = RunOf(L, Label, M);
	return R.Found ? ss::ui::Vec2{R.X + R.W / 2.0f, R.Base - R.Size * 0.3f} : ss::ui::Vec2{-1.0f, -1.0f};
}

/** As ButtonAt, for a label drawn inside Area (a seat's "Fold" tag isn't the Fold button). */
ss::ui::Vec2 ButtonIn(const DrawList& L, const std::string& Label, const ss::ui::Rect& Area, const TableMeasurer& M)
{
	for (size_t From = 0;;)
	{
		const AutoRun R = RunOf(L, Label, M, From);
		if (!R.Found)
		{
			return {-1.0f, -1.0f};
		}
		const ss::ui::Vec2 C{R.X + R.W / 2.0f, R.Base - R.Size * 0.3f};
		if (Inside(Area, C.X, C.Y))
		{
			return C;
		}
		From = R.Index + 1;
	}
}

/** Where the player's action buttons go (RiverLine::Controls: Fold, Call, Raise at y 862, 66 tall, from x 640). */
const ss::ui::Rect ActionRow{640.0f, 862.0f, 530.0f, 66.0f};

// ------------------------------------------------------------------ the pace row and the panel

void PaceAndPanel()
{
	AutoDesk D("ui-auto-panel");
	ss::Session& S = D.S;
	Expect(D.Seat(), "autoplay: seated at a table");
	DrawList L = D.Frame();
	Expect(DrewRun(L, "Full") && DrewRun(L, "Smart") && DrewRun(L, "AUTO") && !DrewRun(L, "Sprint"), "autoplay: the pace row reads FULL · SMART · AUTO");
	Expect(SpelledAt(L, "AI AUTOPLAY") < 0, "autoplay: the panel starts closed");
	// AUTO opens the panel (the third pace button).
	const ss::ui::Vec2 Auto = ButtonAt(L, "AUTO", D.M);
	D.Click(Auto.X, Auto.Y);
	L = D.Frame();
	const long Title = SpelledAt(L, "AI AUTOPLAY");
	Expect(Title >= 0, "autoplay: AUTO opens the panel, titled AI AUTOPLAY");
	Expect(S.Autoplay(0).M == ss::autoplay::Mode::Off, "autoplay: opening the panel turns nothing on");
	// Eleven tiles in a 4-3-4 grid: every style's name, each edge line whole inside its tile.
	std::vector<AutoRun> Names;
	for (const ss::style::Info& I : ss::style::Catalog())
	{
		Names.push_back(RunOf(L, I.Name, D.M, static_cast<size_t>(std::max(0L, Title))));
	}
	const bool AllNames = std::all_of(Names.begin(), Names.end(), [](const AutoRun& R) { return R.Found; });
	Expect(AllNames && Names.size() == 11, "autoplay: the panel shows all eleven styles");
	if (AllNames)
	{
		std::set<int> RowsAt;
		for (const AutoRun& R : Names)
		{
			RowsAt.insert(static_cast<int>(std::lround(R.Base)));
		}
		std::vector<int> PerRow;
		for (const int Y : RowsAt)
		{
			PerRow.push_back(static_cast<int>(std::count_if(Names.begin(), Names.end(), [Y](const AutoRun& R) { return static_cast<int>(std::lround(R.Base)) == Y; })));
		}
		Expect(PerRow == std::vector<int>({4, 3, 4}), "autoplay: the tiles sit 4-3-4");
	}
	int Long = 0;
	for (const ss::style::Info& I : ss::style::Catalog())
	{
		// RiverLineAuto.cpp's tile: 252 wide, the edge at 12 units over 220, four lines at most.
		const int Lines = WrappedLines(I.Edge, 252.0f - 32.0f, 12.0f, ss::ui::Font::Regular, D.M);
		if (Lines > 4)
		{
			++Long;
			std::printf("  autoplay: %s's edge line takes %d lines\n", I.Name, Lines);
		}
	}
	Expect(Long == 0, "autoplay: every style's edge line fits its tile whole");
	// The rest of the panel: Sharpness and its causes, the speeds and SKIP, the targets, the stop rules, the default.
	const ss::autoplay::Panel P = D.Plan();
	Expect(SpelledAt(L, "SHARPNESS") >= 0 && DrewRun(L, P.SharpWhy) && Drew(L, "Read when your plan takes the seat."), "autoplay: Sharpness, its causes, and when it's read");
	Expect(DrewRun(L, "1x") && DrewRun(L, "2x") && DrewRun(L, "4x") && DrewRun(L, "8x") && DrewRun(L, "SKIP"), "autoplay: the speed row, 1x to 8x and SKIP");
	bool Targets = true;
	for (const ss::autoplay::Target To : {ss::autoplay::Target::NextBreak, ss::autoplay::Target::Bubble, ss::autoplay::Target::FinalTable, ss::autoplay::Target::Result, ss::autoplay::Target::Hands})
	{
		Targets = Targets && DrewRun(L, ss::autoplay::TargetName(To));
	}
	bool Stops = true;
	for (const uint32_t Bit : {ss::autoplay::StopFinalTable, ss::autoplay::StopBubble, ss::autoplay::StopMyAllIn, ss::autoplay::StopBigPot, ss::autoplay::StopPremium})
	{
		Stops = Stops && DrewRun(L, ss::autoplay::StopName(Bit));
	}
	Expect(Targets && Stops, "autoplay: the skip's five targets and the five stop rules");
	Expect(DrewRun(L, "Your default") && DrewRun(L, "Watch it play \xC2\xB7 1x"), "autoplay: a new career's plan is its default; the button watches at 1x");
	Expect(Overflows(L, PanelRect, static_cast<size_t>(std::max(0L, Title)), D.M, "panel") == 0, "autoplay: every line of the panel fits inside it");
	Expect(PanelRect.X >= 16.0f && PanelRect.X + PanelRect.W <= ss::ui::RiverLine::Width - 16.0f && PanelRect.Y + PanelRect.H <= ss::ui::RiverLine::Height - 46.0f,
		"autoplay: the panel sits on the laptop's screen, clear of the taskbar");

	// Pick GTO (still off: the table keeps it), 2x, then the main button hands over the seat; the panel closes.
	const AutoRun Gto = RunOf(L, "GTO", D.M, static_cast<size_t>(std::max(0L, Title)));
	D.Click(Gto.X + 30.0f, Gto.Base + 30.0f);
	Expect(S.AutoPanel(0).Now.S == ss::style::Style::Gto && S.Autoplay(0).M == ss::autoplay::Mode::Off, "autoplay: a tile picks the style; off, the table keeps it");
	L = D.Frame();
	const ss::ui::Vec2 Two = ButtonAt(L, "2x", D.M);
	D.Click(Two.X, Two.Y);
	Expect(S.AutoPanel(0).Now.Speed == 2 && S.Autoplay(0).M == ss::autoplay::Mode::Off, "autoplay: a speed is kept with it");
	L = D.Frame();
	Expect(DrewRun(L, "Make this my default") && DrewRun(L, "Watch it play \xC2\xB7 2x"), "autoplay: a changed plan offers to be the default");
	WriteList("auto_panel", L);
	std::printf("  auto_panel     %6zu vertices %5zu commands\n", L.Vertices.size(), L.Cmds.size());
	// A greyed tile: the hidden Bounty Hunter (the ship rule holds it back until its module lands) says why and can't be picked.
	const std::string Why = S.AutoPanel(0).Greyed[static_cast<size_t>(ss::style::Style::BountyHunter)];
	Expect(!Why.empty() && DrewRun(L, Why), "autoplay: a greyed tile says why");
	const AutoRun Bounty = RunOf(L, "Bounty Hunter", D.M, static_cast<size_t>(std::max(0L, Title)));
	D.Click(Bounty.X + 30.0f, Bounty.Base + 30.0f);
	Expect(S.AutoPanel(0).Now.S == ss::style::Style::Gto, "autoplay: a greyed tile can't be picked");
	// "Make this my default".
	const ss::ui::Vec2 Def = ButtonAt(L, "Make this my default", D.M);
	D.Click(Def.X, Def.Y);
	Expect(S.Life.Auto.Pref.S == ss::style::Style::Gto && S.Life.Auto.Pref.Speed == 2 && (S.Life.Update2LinesRead & ss::life::U2AutoPref) != 0,
		"autoplay: Make this my default saves the plan to the career");
	L = D.Frame();
	Expect(DrewRun(L, "Your default") && Drew(L, "Saved. New tables start with this plan."), "autoplay: and says so");
	// The stop rules toggle (My all-in decisions on, then off).
	const ss::ui::Vec2 AllIn = ButtonAt(L, ss::autoplay::StopName(ss::autoplay::StopMyAllIn), D.M);
	D.Click(AllIn.X, AllIn.Y);
	Expect((S.AutoPanel(0).Now.Stops & ss::autoplay::StopMyAllIn) != 0, "autoplay: a stop rule turns on");
	D.Click(AllIn.X, AllIn.Y);
	Expect((S.AutoPanel(0).Now.Stops & ss::autoplay::StopMyAllIn) == 0, "autoplay: and off");
	// The target (Final table), kept with the plan while it's off.
	L = D.Frame();
	const ss::ui::Vec2 Final = ButtonAt(L, ss::autoplay::TargetName(ss::autoplay::Target::FinalTable), D.M);
	D.Click(Final.X, Final.Y);
	Expect(S.AutoPanel(0).Now.To == ss::autoplay::Target::FinalTable, "autoplay: a target is kept with the plan");
	// The main button: watch at 2x; the panel closes.
	L = D.Frame();
	const ss::ui::Vec2 Go = ButtonAt(L, "Watch it play \xC2\xB7 2x", D.M);
	D.Click(Go.X, Go.Y);
	const ss::autoplay::TableAuto On = S.Autoplay(0);
	Expect(On.M == ss::autoplay::Mode::Watch && On.S == ss::style::Style::Gto && On.Speed == 2, "autoplay: the main button hands the seat to the plan");
	L = D.Frame();
	Expect(SpelledAt(L, "AI AUTOPLAY") < 0, "autoplay: and the panel closes");
	// Open again: K closes it, Backspace closes it, a click off it closes it; none of them changes the plan.
	for (int Way = 0; Way < 3; ++Way)
	{
		L = D.Frame();
		const ss::ui::Vec2 A = ButtonAt(L, "AUTO", D.M);
		D.Click(A.X, A.Y);
		Expect(SpelledAt(D.Frame(), "AI AUTOPLAY") >= 0, "autoplay: AUTO opens the panel again");
		if (Way == 0)
		{
			D.RL.Key("k");
		}
		else if (Way == 1)
		{
			D.RL.Key("Backspace");
		}
		else
		{
			D.Click(1400.0f, 500.0f);
		}
		Expect(SpelledAt(D.Frame(), "AI AUTOPLAY") < 0 && S.Autoplay(0).M == ss::autoplay::Mode::Watch, Way == 0 ? "autoplay: K closes the panel" : Way == 1 ? "autoplay: Backspace closes the panel" : "autoplay: a click off the panel closes it");
	}
	// While it plays, a tile changes the style from the next decision.
	L = D.Frame();
	const ss::ui::Vec2 A2 = ButtonAt(L, "AUTO", D.M);
	D.Click(A2.X, A2.Y);
	L = D.Frame();
	const AutoRun Lag = RunOf(L, "LAG", D.M, static_cast<size_t>(std::max(0L, SpelledAt(L, "AI AUTOPLAY"))));
	D.Click(Lag.X + 30.0f, Lag.Base + 30.0f);
	Expect(S.Autoplay(0).S == ss::style::Style::Lag && S.Autoplay(0).M == ss::autoplay::Mode::Watch, "autoplay: picked while it plays, the new style plays on");
	L = D.Frame();
	Expect(DrewRun(L, "WATCHING 2x"), "autoplay: the panel says it's watching");
	// "Take my seat back" turns it off.
	const ss::ui::Vec2 Back = ButtonAt(L, "Take my seat back", D.M);
	D.Click(Back.X, Back.Y);
	Expect(S.Autoplay(0).M == ss::autoplay::Mode::Off, "autoplay: Take my seat back turns it off");
	// FULL or SMART hands the seat back too.
	S.SetAutoplay(0, On);
	L = D.Frame();
	const ss::ui::Vec2 Full = ButtonAt(L, "Full", D.M);
	D.Click(Full.X, Full.Y);
	Expect(S.Autoplay(0).M == ss::autoplay::Mode::Off && S.CurrentPace == ss::Pace::Full, "autoplay: FULL takes the seat back and sets the pace");
	// K with autoplay off: the plan the panel shows takes the seat.
	D.RL.Key("k");
	Expect(S.Autoplay(0).M == ss::autoplay::Mode::Watch && S.Autoplay(0).S == ss::style::Style::Gto, "autoplay: K turns it on with the plan the panel shows");

	// auto_panel_greyed: worn out and starving, Sharpness drops under 70% and the panel says the plan gets sloppy.
	{
		AutoDesk G("ui-auto-greyed");
		Expect(G.Seat(), "autoplay: a second table");
		G.S.Life.Energy = 6.0;
		G.S.Life.Hunger = 99.0;
		G.S.Life.Thirst = 99.0;
		G.S.HeroTilt = 0.9;
		G.Frame();
		DrawList Lg = G.Frame();
		const ss::ui::Vec2 Ag = ButtonAt(Lg, "AUTO", G.M);
		G.Click(Ag.X, Ag.Y);
		G.Now += 0.5; // the panel's fade-in done
		Lg = G.Frame();
		const ss::autoplay::Panel Pg = G.Plan();
		Expect(Pg.Sharp < 0.70 && DrewRun(Lg, Pg.SharpWhy) && Drew(Lg, "Under 70% your plan gets sloppy"), "autoplay: a sloppy plan's Sharpness, its causes and the warning");
		Expect(DrewRun(Lg, Pg.Greyed[static_cast<size_t>(ss::style::Style::BountyHunter)]), "autoplay: the greyed tile's reason");
		Expect(Overflows(Lg, PanelRect, static_cast<size_t>(std::max(0L, SpelledAt(Lg, "AI AUTOPLAY"))), G.M, "panel (greyed)") == 0, "autoplay: the sloppy panel still fits");
		WriteList("auto_panel_greyed", Lg);
	}
}

// ------------------------------------------------------------------ the watch strip

void WatchStrip()
{
	AutoDesk D("ui-auto-watch");
	ss::Session& S = D.S;
	Expect(D.Seat(), "autoplay: seated to watch");
	ss::autoplay::TableAuto A;
	A.S = ss::style::Style::Gto;
	A.M = ss::autoplay::Mode::Watch;
	A.Speed = 2;
	Expect(S.SetAutoplay(0, A).empty(), "autoplay: GTO at 2x");
	// Until the plan thinks: the think line, the action buttons that take the wheel, and never a move.
	int MovesShown = 0;
	int Thinks = 0;
	bool Emitted = false;
	for (int Guard = 0; Guard < 30 * 240 && Thinks < 6; ++Guard)
	{
		D.Now += 1.0 / 30.0;
		S.Update(D.Now);
		const DrawList L = D.Frame();
		const ss::autoplay::Panel P = D.Plan();
		if (P.Thinking)
		{
			++Thinks;
			MovesShown += DrewAMove(L) ? 1 : 0;
			Expect(DrewRun(L, ss::autoplay::ThinkingLine(ss::style::Style::Gto)) && DrewRun(L, "Any action takes the wheel.") && DrewRun(L, "Fold"),
				"autoplay: during the think, the think line and the buttons that take the wheel");
			if (!Emitted)
			{
				Emitted = true;
				WriteList("auto_watch_strip", L);
				std::printf("  auto_watch_strip %5zu vertices %5zu commands\n", L.Vertices.size(), L.Cmds.size());
				const ss::ui::Rect Strip{640.0f, 778.0f, 530.0f, 154.0f};
				Expect(Overflows(L, Strip, 0, D.M, "strip") == 0, "autoplay: the strip's lines fit it");
			}
			// Let this think play out (the plan acts), then look for the next.
			D.Until([&]() { return !D.Plan().Thinking; }, 30.0);
		}
	}
	Expect(Thinks >= 3 && MovesShown == 0, "autoplay: the plan's move is never on screen while it thinks");
	Expect(S.AutoPanel(0).AiDecisions > 0, "autoplay: the plan made its decisions");
	// Once played, the strip shows the plan's last action (when there's one this hand and no think on).
	bool SawLast = false;
	for (int Guard = 0; Guard < 30 * 120 && !SawLast; ++Guard)
	{
		D.Now += 1.0 / 30.0;
		S.Update(D.Now);
		const DrawList L = D.Frame();
		const ss::autoplay::Panel P = D.Plan();
		SawLast = !P.Thinking && !P.LastAction.empty() && DrewRun(L, P.LastAction) && SpelledAt(L, "LAST MOVE") >= 0;
	}
	Expect(SawLast, "autoplay: the strip shows the plan's last move once it's played");
	// The style chip opens the panel; the panel is modal: an action key under it takes nothing, and K closes it.
	Expect(D.Until([&]() { return D.Plan().Thinking; }, 120.0), "autoplay: a think to try the panel over");
	{
		const DrawList Lc = D.Frame();
		const ss::ui::Vec2 Chip = ButtonIn(Lc, "GTO", {640.0f, 778.0f, 530.0f, 50.0f}, D.M);
		D.Click(Chip.X, Chip.Y);
		Expect(SpelledAt(D.Frame(), "AI AUTOPLAY") >= 0, "autoplay: the strip's style chip opens the panel");
		D.RL.Key("f");
		Expect(!S.AutoPanel(0).TakenOver && SpelledAt(D.Frame(), "AI AUTOPLAY") >= 0, "autoplay: an action key under the open panel does nothing");
		D.RL.Key("k");
		Expect(!S.AutoPanel(0).TakenOver && SpelledAt(D.Frame(), "AI AUTOPLAY") < 0, "autoplay: K closes the panel and takes nothing");
	}
	// The strip's speed buttons change the watch speed.
	{
		const DrawList L = D.Frame();
		const ss::ui::Vec2 Four = ButtonAt(L, "4x", D.M);
		D.Click(Four.X, Four.Y);
		Expect(S.Autoplay(0).Speed == 4 && S.Autoplay(0).M == ss::autoplay::Mode::Watch, "autoplay: the strip's 4x");
	}
	// A press on an action button during the think takes the wheel and does nothing else; the next press acts.
	Expect(D.Until([&]() { return D.Plan().Thinking; }, 120.0), "autoplay: the plan thinks again");
	const size_t Acted = S.CurHand ? S.CurHand->Events.size() : 0;
	DrawList L = D.Frame();
	const ss::ui::Vec2 Ghost = ButtonIn(L, "Fold", ActionRow, D.M);
	D.Click(Ghost.X, Ghost.Y);
	Expect(S.AutoPanel(0).TakenOver && (S.CurHand ? S.CurHand->Events.size() : 0) == Acted, "autoplay: the first press takes the wheel and acts on nothing");
	Expect(D.Until([&]() { return S.HasPrompt; }, 5.0), "autoplay: the player's prompt opens at once");
	L = D.Frame();
	Expect(DrewRun(L, "YOUR HAND"), "autoplay: the strip says whose hand it is");
	Expect(DrewRun(L, "Your plan's back next hand. K turns autoplay off."), "autoplay: and how to keep it");
	const ss::ui::Vec2 Real = ButtonIn(L, "Fold", ActionRow, D.M);
	Expect(std::fabs(Real.X - Ghost.X) < 1.0f && std::fabs(Real.Y - Ghost.Y) < 6.0f, "autoplay: the buttons that take the wheel sit where the player's own do");
	D.Click(Real.X, Real.Y);
	Expect(!S.HasPrompt, "autoplay: the second press acts");
	// The next hand: AUTO RESUMES for a few seconds; K inside it turns autoplay off.
	Expect(D.Until([&]() { return D.Plan().ResumeWindow; }, 120.0), "autoplay: AUTO RESUMES after a taken hand");
	L = D.Frame();
	Expect(DrewRun(L, ss::autoplay::Text(ss::autoplay::Words::Resumes)), "autoplay: the strip says AUTO RESUMES · [K] keep playing");
	D.RL.Key("k");
	Expect(S.Autoplay(0).M == ss::autoplay::Mode::Off, "autoplay: K in the window keeps the seat (autoplay off)");
	// An action key (C) during a think takes the wheel too.
	A.Speed = 1;
	S.SetAutoplay(0, A);
	Expect(D.Until([&]() { return D.Plan().Thinking; }, 120.0), "autoplay: on again, it thinks");
	D.RL.Key("c");
	Expect(S.AutoPanel(0).TakenOver && !S.HasPrompt, "autoplay: C during the think takes the wheel without acting");
}

// ------------------------------------------------------------------ the skip card

void SkipCard()
{
	AutoDesk D("ui-auto-skip");
	ss::Session& S = D.S;
	Expect(D.Seat(), "autoplay: seated to skip");
	ss::autoplay::TableAuto A;
	A.S = ss::style::Style::PushFold;
	A.M = ss::autoplay::Mode::Watch;
	S.SetAutoplay(0, A);
	S.SprintBudgetMs = 0.0; // one tick an update: the card stays up to be looked at
	Expect(S.SkipAhead(0, ss::autoplay::Target::Bubble).empty(), "autoplay: skip to the bubble with Push/Fold");
	Expect(D.Until([&]() { return S.Sprinting; }, 120.0), "autoplay: the skip starts at the next hand");
	D.Run(0.5);
	DrawList L = D.Frame();
	const ss::autoplay::Panel P = D.Plan();
	Expect(P.Skipping && P.SkipLine.rfind("SKIPPING \xC2\xB7 PUSH/FOLD \xC2\xB7 hand ", 0) == 0, "autoplay: the skip line names the style and the hand");
	Expect(DrewRun(L, "SKIPPING \xC2\xB7 PUSH/FOLD") && Drew(L, "to the bubble") && Drew(L, "level "), "autoplay: the skip card: SKIPPING · PUSH/FOLD, then where it is");
	Expect(DrewRun(L, "Take the wheel") && DrewRun(L, "Stop") && DrewRun(L, ss::autoplay::Text(ss::autoplay::Words::SkipKeys)), "autoplay: the card's buttons and keys");
	Expect(!Drew(L, "SPRINTING AHEAD"), "autoplay: Sprint's card is gone");
	const AutoRun Head = RunOf(L, "SKIPPING \xC2\xB7 PUSH/FOLD", D.M);
	Expect(Head.Found && Overflows(L, {0.0f, 64.0f, 1180.0f, 936.0f}, Head.Index, D.M, "skip card") == 0, "autoplay: the card's lines fit the table's side");
	Emit("table_skip", D.RL, D.Now);
	// K stops the skip between hands; the plan keeps the seat at 1x (§2.6.5).
	D.RL.Key("k");
	Expect(!S.Sprinting && S.Autoplay(0).M == ss::autoplay::Mode::Watch && S.Autoplay(0).Speed == 1 && !S.AutoPanel(0).TakenOver, "autoplay: K stops the skip, the plan watches at 1x");
	L = D.Frame();
	Expect(DrewRun(L, "Stopped. Your plan has the seat at 1x.") || Drew(L, "Stopped."), "autoplay: the strip says why it stopped");
	// Backspace stops a skip the same way.
	S.SkipAhead(0, ss::autoplay::Target::Bubble);
	Expect(D.Until([&]() { return S.Sprinting; }, 120.0), "autoplay: skipping again");
	D.RL.Key("Backspace");
	Expect(!S.Sprinting && S.Autoplay(0).M == ss::autoplay::Mode::Watch, "autoplay: Backspace stops the skip");
	// The card's Take the wheel: the skip stops and this hand is the player's.
	S.SkipAhead(0, ss::autoplay::Target::Bubble);
	Expect(D.Until([&]() { return S.Sprinting; }, 120.0), "autoplay: and again");
	L = D.Frame();
	const ss::ui::Vec2 Wheel = ButtonAt(L, "Take the wheel", D.M);
	D.Click(Wheel.X, Wheel.Y);
	Expect(!S.Sprinting && S.AutoPanel(0).TakenOver, "autoplay: Take the wheel stops the skip and hands the player this hand");
	S.SprintBudgetMs = 10.0;
}

// ------------------------------------------------------------------ the tile and tab lines

void TileStatus()
{
	AutoDesk D("ui-auto-tiles");
	ss::Session& S = D.S;
	S.BankrollCents = 3000;
	const ss::net::Network& Net = ss::net::Shared();
	std::vector<ss::LobbyEvent> Two;
	std::set<std::string> Names;
	for (const ss::net::EventInstance& E : Net.Window(S.WorldMinutes() - 180.0, S.WorldMinutes() + 60.0))
	{
		const ss::LobbyEvent L = Net.Listing(E, nullptr, S.Unlocks());
		if (L.Joinable && L.BuyInCents <= 1100 && L.Spec.Entrants <= 1500 && Two.size() < 2 && Names.insert(L.Spec.Name).second)
		{
			Two.push_back(L);
		}
	}
	Expect(Two.size() == 2, "autoplay: two small events to tile");
	if (Two.size() < 2)
	{
		return;
	}
	S.RegisterEvent(Two[0]);
	S.RegisterEvent(Two[1]);
	S.Tiled = true;
	D.Run(1.0);
	ss::autoplay::TableAuto Watch;
	Watch.S = ss::style::Style::Gto;
	Watch.M = ss::autoplay::Mode::Watch;
	Watch.Speed = 2;
	Expect(S.SetAutoplay(0, Watch).empty(), "autoplay: the first tile watched at 2x");
	ss::autoplay::TableAuto Skip;
	Skip.S = ss::style::Style::PushFold;
	Skip.M = ss::autoplay::Mode::Watch;
	S.SetAutoplay(1, Skip);
	Expect(S.SkipAhead(1, ss::autoplay::Target::Bubble).empty(), "autoplay: the second skips to the bubble");
	S.SprintBudgetMs = 0.0;
	bool Skipping = false;
	for (int Guard = 0; Guard < 30 * 60 && !Skipping; ++Guard)
	{
		D.Now += 1.0 / 30.0;
		S.Update(D.Now);
		D.Frame();
		S.WithTable(1, [&]() { Skipping = S.Sprinting; });
	}
	Expect(Skipping, "autoplay: the second tile is skipping");
	D.Run(0.3);
	const DrawList L = D.Frame();
	const ss::autoplay::Panel P0 = S.AutoPanel(0);
	const ss::autoplay::Panel P1 = S.AutoPanel(1);
	Expect(P0.Status == "AUTO \xC2\xB7 GTO 2x" && Drew(L, "AUTO \xC2\xB7 GTO 2x"), "autoplay: the watched tile says AUTO · GTO 2x");
	Expect(P1.Skipping && Drew(L, "SKIPPING \xC2\xB7 PUSH/FOLD \xC2\xB7 hand ") && Drew(L, "SKIP \xC2\xB7 "), "autoplay: the skipping tile's line, and its tab's SKIP line");
	int AutoPaces = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		AutoPaces += Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == "AUTO" ? 1 : 0;
	}
	Expect(AutoPaces == 2 && !DrewRun(L, "Sprint"), "autoplay: each tile's pace row reads FULL · SMART · AUTO");
	Expect(Overflows(L, {0.0f, 0.0f, ss::ui::RiverLine::Width, 64.0f}, 0, D.M, "tabs") == 0, "autoplay: the tab lines fit the laptop");
	WriteList("auto_tile_status", L);
	std::printf("  auto_tile_status %5zu vertices %5zu commands\n", L.Vertices.size(), L.Cmds.size());
	// A tile's AUTO brings its table forward with the panel over it.
	const AutoRun Second = [&]() {
		AutoRun Last;
		for (size_t From = 0;;)
		{
			const AutoRun R = RunOf(L, "AUTO", D.M, From);
			if (!R.Found)
			{
				return Last;
			}
			Last = R;
			From = R.Index + 1;
		}
	}();
	D.Click(Second.X + Second.W / 2.0f, Second.Base - 4.0f);
	const DrawList Lp = D.Frame();
	Expect(SpelledAt(Lp, "AI AUTOPLAY") >= 0 && S.FocusedTable() == 1, "autoplay: a tile's AUTO opens the panel for that table");
	S.SprintBudgetMs = 10.0;
}

// ------------------------------------------------------------------ results and Career

void ResultsAndCareer()
{
	AutoDesk D("ui-auto-results");
	ss::Session& S = D.S;
	// A finished event the plan played whole: the results line, the "—" accuracy, nothing of the player's to review.
	ss::HistoryEntry H;
	H.Name = "Night Owl Turbo";
	H.Place = 37;
	H.Entrants = 1000;
	H.Prize = 1210;
	H.EventId = "night-owl@1410";
	S.History.insert(S.History.begin(), H);
	ss::autoplay::TableAuto Lag;
	Lag.S = ss::style::Style::Lag;
	ss::autoplay::Run Run;
	Run.AiDecisions = 214;
	Run.OwnDecisions = 0;
	Run.AiHands = 260;
	ss::autoplay::Record(S.Life.Auto, H.EventId, 0, 0, Lag, Run);
	S.LastResults = ss::Results();
	S.LastResults.EventName = H.Name;
	S.LastResults.Place = H.Place;
	S.LastResults.Entrants = H.Entrants;
	S.LastResults.PrizeCents = H.Prize;
	S.LastResults.BuyInCents = 550;
	S.LastResults.Hands = 260;
	S.HasResults = true;
	S.CurrentScreen = ss::Screen::Results;
	D.Now += 2.0;
	DrawList L = D.Frame();
	Expect(DrewRun(L, "Played by your plan: 214 of 214 decisions (LAG)") && DrewRun(L, "\xE2\x80\x94"), "results: played by the plan, and the player's accuracy is a dash");
	Expect(DrewRun(L, "Your plan made every decision, so there's nothing of yours to review.") && DrewRun(L, "XP comes from your own decisions, never your plan's."),
		"results: nothing of the player's to grade, and XP is the player's own");
	Expect(Overflows(L, {0.0f, 64.0f, ss::ui::RiverLine::Width, 900.0f}, 0, D.M, "results") == 0, "results: every line fits the laptop");
	WriteList("results_auto", L);
	// A shared event: the player made some decisions.
	Run.OwnDecisions = 46;
	ss::autoplay::Record(S.Life.Auto, H.EventId, 0, 0, Lag, Run);
	S.LastResults.Grades.resize(46);
	S.LastResults.AccuracyPct = 88.0;
	L = D.Frame();
	Expect(DrewRun(L, "Played by your plan: 214 of 260 decisions (LAG)") && DrewRun(L, "88.0"), "results: the plan's share, and the player's own accuracy");
	S.LastResults.Grades.clear();
	// A closed table's toast while others play: "played by your plan" where "no decisions" was.
	S.HasResults = false;
	S.CurrentScreen = ss::Screen::Lobby;
	Run.OwnDecisions = 0;
	ss::autoplay::Record(S.Life.Auto, H.EventId, 0, 0, Lag, Run);
	ss::FinishedTable Done;
	Done.Result = S.LastResults;
	Done.At = D.Now - 1.0;
	S.Finished.push_back(Done);
	L = D.Frame();
	Expect(DrewRun(L, "played by your plan") && !DrewRun(L, "no decisions"), "results: the closed table's toast says the plan played it");
	S.Finished.clear();

	// Career: the AUTO tag and the "—" for the plan's event; a played event keeps its bar and percentage.
	ss::HistoryEntry Mine;
	Mine.Name = "Daily Grind";
	Mine.Place = 120;
	Mine.Entrants = 700;
	Mine.AccuracyPct = 91.0;
	Mine.EventId = "daily-grind@1200";
	S.History.insert(S.History.begin() + 1, Mine);
	ss::HistoryEntry Shared;
	Shared.Name = "Hyper Hour";
	Shared.Place = 12;
	Shared.Entrants = 180;
	Shared.AccuracyPct = 84.0;
	Shared.EventId = "hyper-hour@1300";
	S.History.insert(S.History.begin(), Shared);
	ss::autoplay::TableAuto Gto;
	Gto.S = ss::style::Style::Gto;
	ss::autoplay::Run Half;
	Half.AiDecisions = 82;
	Half.OwnDecisions = 18;
	Half.AiHands = 120;
	Half.OwnHands = 30;
	ss::autoplay::Record(S.Life.Auto, Shared.EventId, 0, 0, Gto, Half);
	// The career's default (the longest name, 8x) and a Form that's slipped for two weeks without play.
	S.Life.Auto.Pref.S = ss::style::Style::Exploit;
	S.Life.Auto.Pref.Speed = 8;
	S.Life.Auto.Pref.Form = 88.0;
	S.Life.Auto.Pref.FormWeek = ss::autoplay::WeekOf(S.WorldMinutes());
	S.Life.Auto.Pref.WeekDecisions = 0;
	S.LobbyMinutes += 15.0 * 1440.0; // two weeks and a day without a hand
	D.Now = Step(S, D.Now, 0.1);
	bool Rusty = false;
	const double Form = ss::autoplay::FormNow(S.Life.Auto, S.WorldMinutes(), &Rusty);
	D.RL.OpenPage(ss::ui::RiverLine::Page::Career, D.Now);
	D.Now += 2.0;
	D.Frame();
	D.Now += 0.1;
	L = D.Frame();
	const AutoRun Tag = RunOf(L, "AUTO \xC2\xB7 LAG 100%", D.M);
	const AutoRun Tag2 = RunOf(L, "AUTO \xC2\xB7 GTO 82%", D.M);
	const AutoRun Row = RunOf(L, "Night Owl Turbo", D.M);
	Expect(Tag.Found && Tag2.Found && Row.Found && std::fabs(Tag.Base - Row.Base) < 20.0f, "career: the plan's events carry AUTO · LAG 100% and AUTO · GTO 82%");
	const AutoRun Place = RunOf(L, "37th of 1,000", D.M);
	Expect(Tag.Found && Place.Found && Tag.X + Tag.W < Place.X, "career: the tag ends before the place");
	int Dashes = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		Dashes += Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == "\xE2\x80\x94" && std::fabs(Cmd.Text.BaselineY - Row.Base) < 2.0f ? 1 : 0;
	}
	Expect(Dashes >= 1 && DrewRun(L, "91%") && DrewRun(L, "84%") && !DrewRun(L, "0%"), "career: \"—\" for the event the plan played whole (never 0%); the others keep their percentage");
	// Career's autoplay card (§2.11): the panel's default style and the player's Form, whole, inside the card.
	const ss::ui::Rect Card{1076.0f - 168.0f - 12.0f - 420.0f, 640.0f, 420.0f, 32.0f};
	Expect(SpelledAt(L, "AUTOPLAY") >= 0 && SpelledAt(L, "FORM") >= 0 && DrewRun(L, "Exploitative") && Drew(L, "8x \xC2\xB7 your default"),
		"career: the autoplay card shows the default plan");
	Expect(Rusty && DrewRun(L, std::to_string(static_cast<int>(std::lround(Form))) + ", rusty"), "career: and the player's Form, rusty after two weeks off");
	Expect(Overflows(L, Card, 0, D.M, "career card") == 0, "career: the autoplay card's words fit it");
	WriteList("career_auto_row", L);
	std::printf("  career_auto_row %5zu vertices %5zu commands\n", L.Vertices.size(), L.Cmds.size());
}

// ------------------------------------------------------------------ the copy

void Copy()
{
	// Every autoplay sentence this track draws beyond core's own (RiverLineShared.h's rlauto_detail), as drawn.
	const std::vector<std::pair<std::string, std::string>> Lines = {
		{"panel", "Your plan plays your seat in the style you pick. Any hand is yours again with one press of K."},
		{"panel", "K or Backspace closes this."},
		{"panel", "Read when your plan takes the seat."},
		{"panel", "Form is your own graded play online. From 80 it sharpens your plan; at 95 it's as sharp as it gets."},
		{"panel", "Under 70% your plan gets sloppy: eat, rest, play some hands yourself."},
		{"panel", "Saved. New tables start with this plan."},
		{"panel", "Held for this run since 2:14 AM."},
		{"strip", "Any action takes the wheel."},
		{"strip", "This hand is yours. Your plan's back on the next one."},
		{"strip", "Your plan's back next hand. K turns autoplay off."},
		{"strip", "Skipping from the next hand."},
		{"strip", "This event so far: 41 decisions by your plan, 3 by you."},
		{"results", "Your plan made every decision, so there's nothing of yours to review."},
		{"results", "XP comes from your own decisions, never your plan's."},
	};
	Failures += copylint::Run("ui-laptop autoplay", Lines, copylint::LetterBody);
	// System UI may say "autoplay" (U1); none of these lines claims anything happened in the world.
	int Bots = 0;
	for (const auto& L : Lines)
	{
		Bots += L.second.find(" bot") != std::string::npos || L.second.find("Bot") != std::string::npos ? 1 : 0;
	}
	Expect(Bots == 0, "autoplay copy: the plan is never a bot");
}
} // namespace auto_seg

void SegAuto()
{
	auto_seg::PaceAndPanel();
	auto_seg::WatchStrip();
	auto_seg::SkipCard();
	auto_seg::TileStatus();
	auto_seg::ResultsAndCareer();
	auto_seg::Copy();
}
} // namespace ui_test
