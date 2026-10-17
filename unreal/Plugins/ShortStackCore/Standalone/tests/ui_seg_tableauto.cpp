// riverline_ui: core-tableui's additions (update 2 SPEC §2.6.2, §2.11, §6.7), compiled into ui_test and called from main()
// in ui_main.cpp (the kit is UiSeg.h):
// - the table card's PLAY page (table_card_play, _2257, _dees, _locked): "PLAY" appended to TableCardPages(), the "AI
//   AUTOPLAY" header, the styles (the shown one in full; greyed ones say why; a hidden one isn't there), Sharpness with
//   why, when it was read and the sloppy plan's warning, the watch speed (no 8x at Dee's), the skip's targets with the
//   keyboard's digits, the stops, Dee's first night (her line and nothing else), the footer's keys on each device, and
//   the same page fed by a real session's AutoPanel;
// - the plan's lines on the HUD (P0a; table_hud_auto_watch, _think, _resume, _skip): the status with [K] Take over, the
//   think that names no move (a stale last action included), the last action once played, the take-over window, the
//   host's own Status and Prompt winning their places, the ladder only on a decision handed back, the out-of-hand row
//   and the hints saying WATCH SPEED, and the live skip's card over the dimmed table (no plates, no glyph spots);
// - the plates' guarantees unchanged under autoplay (the same layout with the plan's fields set);
// - the control map's autoplay entries (K, T under autoplay, the PLAY page's A and X, nothing else on the held card's
//   keys) and the table card's footer qualifier (YOUR PLAN KEEPS PLAYING);
// - the copy-lint rule (no "always wait for you" without "autoplay") with TableSettingHelp's qualifier, over the table's,
//   autoplay's and the styles' inventories; U1 on the drawn pages ("AI" only in the system header, never "bot").
#include "UiSeg.h"

#include "CopyLint.h"
#include "ShortStack/AI/Style.h"
#include "ShortStack/Game/Autoplay.h"
#include "ShortStack/UI/TableHud.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

namespace ui_test
{
namespace tableauto_seg
{
const TableMeasurer Measure;
const float Wide = 1080.0f * 2550.0f / 1220.0f;
const float Widths[3] = {1920.0f, Wide, 1440.0f};
const double Now = 500.0;

std::string W(float Width)
{
	return std::to_string(static_cast<int>(Width));
}

/** Letter-spaced labels draw a glyph a run: the runs on one baseline, joined. */
std::string Joined(const ss::ui::DrawList& L)
{
	std::string Out;
	float Base = -1.0f;
	float End = 0.0f;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text)
		{
			continue;
		}
		const float Wd = Measure.Width(Cmd.Text.Text, Cmd.Text.Face, Cmd.Text.SizePx);
		const bool Same = std::fabs(Cmd.Text.BaselineY - Base) < 0.5f && Cmd.Text.X - End < Cmd.Text.SizePx * 0.6f && Cmd.Text.X >= End - 1.0f;
		Out += Same ? "" : "\n";
		Out += Cmd.Text.Text;
		Base = Cmd.Text.BaselineY;
		End = Cmd.Text.X + Wd;
	}
	return Out;
}

bool Says(const ss::ui::DrawList& L, const std::string& Needle)
{
	return Drew(L, Needle) || Joined(L).find(Needle) != std::string::npos;
}

/** How many runs are exactly Text. */
int Count(const ss::ui::DrawList& L, const std::string& Text)
{
	int N = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		N += Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == Text ? 1 : 0;
	}
	return N;
}

/** A key's glyph (a run exactly Text) on a centred line near baseline Y: letter-spaced labels elsewhere draw single
 *  letters too, so a glyph is looked for where its line is. */
bool KeyAt(const ss::ui::DrawList& L, const std::string& Text, float Width, float Y)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == Text && std::fabs(Cmd.Text.BaselineY - Y) < 26.0f && std::fabs(Cmd.Text.X - Width * 0.5f) < 420.0f)
		{
			return true;
		}
	}
	return false;
}

/** Every run starts and ends on the canvas. */
void Fits(const ss::ui::DrawList& L, float Width, const std::string& What)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text || Cmd.Text.Text.empty())
		{
			continue;
		}
		const float Wd = Measure.Width(Cmd.Text.Text, Cmd.Text.Face, Cmd.Text.SizePx);
		if (Cmd.Text.X < -1.0f || Cmd.Text.X + Wd > Width + 1.0f || Cmd.Text.Y < -1.0f || Cmd.Text.BaselineY > 1081.0f)
		{
			Expect(false, (What + ": text fits the screen (\"" + Cmd.Text.Text + "\" runs off)").c_str());
			return;
		}
	}
}

/** The HUD's minimum: 13 logical units. */
void Readable(const ss::ui::DrawList& L, const std::string& What)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && !Cmd.Text.Text.empty() && Cmd.Text.SizePx < 12.99f)
		{
			Expect(false, (What + ": no text under 13 units (\"" + Cmd.Text.Text + "\")").c_str());
			return;
		}
	}
}

/** U1: system UI may say "AI autoplay" (the PLAY page's header), and nothing on these screens ever says "bot". */
bool OnlySystemAi(const ss::ui::DrawList& L, int MaxAi)
{
	const std::string Text = copylint::Lower(Joined(L));
	int Ai = 0;
	std::string Word;
	bool Bot = false;
	for (size_t K = 0; K <= Text.size(); ++K)
	{
		const char Ch = K < Text.size() ? Text[K] : ' ';
		if ((Ch >= 'a' && Ch <= 'z') || (Ch >= '0' && Ch <= '9'))
		{
			Word.push_back(Ch);
			continue;
		}
		Ai += Word == "ai" ? 1 : 0;
		Bot = Bot || Word == "bot" || Word == "bots";
		Word.clear();
	}
	return Ai <= MaxAi && !Bot && (Ai == 0 || Text.find("ai autoplay") != std::string::npos);
}

// ------------------------------------------------------------------ the scenes

struct Table
{
	ss::live::ClockFacts Clock;
	ss::ui::TableHudInfo I;
	ss::life::State Life;
};

ss::ui::SeatPlate Seat(int Number, const char* Name, const char* Label, ss::Chips Stack, float Dx, float Y, float Width)
{
	ss::ui::SeatPlate P;
	P.Seat = Number;
	P.Name = Name;
	P.Label = Label;
	P.Stack = Stack;
	P.X = Width * 0.5f + Dx;
	P.Y = Y;
	P.OnScreen = P.X > 0.0f && P.X < Width;
	return P;
}

/** The Tuesday Nightly, level 9 (800/1,600, ante 1,600), 41 of 94 left: GTO has the player's seat with 49,600. */
void Watching(Table& T, float Width)
{
	ss::live::ClockFacts& F = T.Clock;
	F.Level = 9;
	F.Now = {800, 1600, 1600};
	F.Next = {1000, 2000, 2000};
	F.LevelSecondsLeft = 14.0 * 60.0 + 32.0;
	F.BreakBeforeLevel = 13;
	F.SecondsToBreak = F.LevelSecondsLeft + 3.0 * 1200.0;
	F.Entrants = 94;
	F.Remaining = 41;
	F.Paid = 14;
	F.ToTheMoney = 27;
	F.PoolCents = 987000;
	F.MinCashCents = 24300;
	F.Average = 45854;
	F.AverageBb = 45854.0 / 1600.0;
	F.Projected = false;
	F.TableId = 12;
	F.Seat = 4;
	ss::ui::TableHudInfo& I = T.I;
	I = ss::ui::TableHudInfo();
	I.Where = ss::ui::Venue::CardRoom;
	I.Clock = &T.Clock;
	I.Event = "Nightly $120";
	I.HeroStack = 49600;
	I.Bb = 1600;
	I.Pot = 4000;
	I.PotAt = {Width * 0.5f, 560.0f};
	I.PotOnScreen = true;
	I.OnFoot.Clock = "9:47 PM";
	T.Life.Energy = 80.0;
	I.OnFoot.Life = &T.Life;
	I.Plates = {
		Seat(0, "Rick", "Knows you", 18400, -640.0f, 640.0f, Width),
		Seat(1, "Mrs. Park", "Regular", 21000, -410.0f, 470.0f, Width),
		Seat(2, "Mei", "", 64200, 0.0f, 420.0f, Width),
		Seat(3, "Sal", "Regular", 31800, 410.0f, 470.0f, Width),
		Seat(4, "Dana Whitfield", "", 27500, 640.0f, 640.0f, Width),
	};
	I.Plates[1].BadgeCell = 3;
	for (const ss::ui::SeatPlate& P : I.Plates)
	{
		I.KeepClear.push_back({P.X - 50.0f, P.Y - 35.0f, 100.0f, 50.0f});
		I.KeepClear.push_back({P.X + 52.0f, P.Y - 42.0f, 70.0f, 44.0f});
		I.KeepClear.push_back({P.X - 62.0f, P.Y - 240.0f, 124.0f, 160.0f});
	}
	I.KeepClear.push_back({Width * 0.5f - 150.0f, 840.0f, 300.0f, 220.0f});
	I.HeroCards = {48, 41}; // known without a peek while the plan holds the seat
	I.Bpm = 74.0f;
	for (int K = 0; K < 480; ++K)
	{
		const float Ph = std::fmod(static_cast<float>(K) / 240.0f * 1.23f, 1.0f);
		I.Trace.push_back(Ph < 0.06f ? Ph / 0.06f : Ph < 0.1f ? 1.0f - (Ph - 0.06f) / 0.04f : 0.0f);
	}
	ss::ui::Toast Lou;
	Lou.From = "Big Lou";
	Lou.Line = "how many chips. dont round up";
	Lou.At = Now - 1.0;
	I.Toasts = {Lou};
	// The plan's fields (ue-auto fills them): it holds the seat, watched at 1x.
	I.AutoOn = true;
	I.AutoStyle = "GTO";
	I.AutoSharp = 0.93;
	I.AutoSpeed = 1;
}

/** The plan's think on the river: the player sees whose think it is and how to take the wheel, never the move. */
void Thinking(Table& T, float Width)
{
	Watching(T, Width);
	T.I.AutoDeciding = true;
	T.I.AutoActsIn = 1.4f;
	T.I.AutoSpeed = 2;
}

/** The hand after a take-over: the plan's back unless K keeps the seat the player's. */
void Resuming(Table& T, float Width)
{
	Watching(T, Width);
	T.I.AutoResumeWindow = 2.1f;
}

/** A skip to the bubble at the Embercrest: the skip card over the dimmed table. */
void Skipping(Table& T, float Width)
{
	Watching(T, Width);
	T.I.SkipOn = true;
	T.I.AutoStyle = "Push/Fold";
	T.I.SkipLine = "Level 9 \xC2\xB7 31 left of 88 \xC2\xB7 you 42 BB";
	T.I.Plates[1].BadgeCell = 3;
}

ss::ui::DrawList Frame(const ss::ui::TableHudInfo& I, float Width)
{
	ss::ui::DrawList L;
	ss::ui::Canvas C(L, Measure, Width, 1080.0f, 1.0f);
	ss::ui::DrawTableHud(C, I, Now);
	return L;
}

int PlayIndex()
{
	const std::vector<std::string>& Pages = ss::ui::TableCardPages();
	return static_cast<int>(std::find(Pages.begin(), Pages.end(), std::string("PLAY")) - Pages.begin());
}

/** The PLAY page as ue-auto fills it: GTO watched at 2x, Sharpness 93% (Form 92, rested), skip to the bubble. */
ss::ui::TableCardInfo PlayCard(const ss::live::ClockFacts* Clock, bool Dees)
{
	ss::ui::TableCardInfo C;
	C.Clock = Clock;
	C.Where = Dees ? ss::ui::Venue::BackRoom : ss::ui::Venue::CardRoom;
	C.Page = PlayIndex();
	ss::autoplay::Panel& P = C.Play.Panel;
	P.Styles = ss::style::Catalog();
	for (const ss::style::Info& Inf : P.Styles)
	{
		P.Greyed.push_back(ss::autoplay::CantPick(Inf.S, Dees ? 2 : 1, false));
	}
	P.Form = 92.0;
	P.Sharp = 0.93;
	P.SharpWhy = ss::autoplay::SharpWhy(92.0, false, 0.0, false, false);
	P.SharpSetAt = 2.0 * 1440.0 + 21.0 * 60.0 + 47.0;
	P.Now.S = ss::style::Style::Gto;
	P.Now.M = ss::autoplay::Mode::Watch;
	P.Now.Speed = 2;
	P.Now.To = ss::autoplay::Target::Bubble;
	P.Now.Stops = ss::autoplay::StopFinalTable | ss::autoplay::StopBubble | ss::autoplay::StopMyAllIn;
	P.On = true;
	if (!Dees)
	{
		for (const ss::autoplay::Target T : {ss::autoplay::Target::NextBreak, ss::autoplay::Target::Bubble, ss::autoplay::Target::FinalTable, ss::autoplay::Target::Result,
				 ss::autoplay::Target::Hands})
		{
			C.Play.Targets.push_back(ss::autoplay::TargetName(T));
		}
		C.Play.Target = 1;
	}
	return C;
}

ss::ui::DrawList Card(const ss::ui::TableCardInfo& C, float Width)
{
	ss::ui::DrawList L;
	ss::ui::Canvas Cv(L, Measure, Width, 1080.0f, 1.0f);
	ss::ui::DrawTableCard(Cv, C, Now);
	return L;
}

// ------------------------------------------------------------------ the PLAY page

void PlayPage()
{
	const std::vector<std::string>& Pages = ss::ui::TableCardPages();
	const std::vector<std::string> Before = {"READS", "HANDS", "PAYOUTS", "FIELD", "NIGHT"};
	Expect(Pages.size() == 6 && Pages.back() == "PLAY" && std::equal(Before.begin(), Before.end(), Pages.begin()),
		"PLAY page: appended to TableCardPages() as the sixth (the hosts index the first five by number)");
	Table T;
	Watching(T, 1920.0f);
	const ss::style::Info& Gto = ss::style::InfoOf(ss::style::Style::Gto);
	const std::vector<std::string> Must = {"PLAY", "READS", "AI AUTOPLAY", "WATCHING \xC2\xB7 2x", "STYLE", "GTO", "TAG", "Balanced Reg", ss::style::SwingName(Gto.Var),
		"BEST IN", Gto.BestIn, "SHARPNESS", "93%", "Form 92, rested", "Set when your plan took the seat, at 9:47 PM.", "WATCH SPEED", "1x", "4x", "8x", "AT THE TABLE",
		"SKIP TO", "Next break", "Bubble", "50 hands", "STOPS", "Final table \xC2\xB7 Bubble \xC2\xB7 My all-in decisions", "STYLE", "AUTOPLAY ON / OFF", "SKIP TO", "SKIP",
		"14:32", "LEVEL 9 \xC2\xB7 800 / 1,600 \xC2\xB7 ANTE 1,600"};
	for (const float Width : Widths)
	{
		for (const bool Pad : {false, true})
		{
			ss::ui::TableCardInfo C = PlayCard(&T.Clock, false);
			C.Gamepad = Pad;
			const ss::ui::DrawList L = Card(C, Width);
			const std::string What = std::string("table_card_play at ") + W(Width) + (Pad ? " (pad)" : " (keys)");
			for (const std::string& N : Must)
			{
				Expect(Says(L, N), (What + ": says \"" + N + "\"").c_str());
			}
			// The shown style in full: its edge line's start (the rest wraps); and how the seat changes hands, in this device's keys.
			const std::string Edge = Gto.Edge;
			Expect(Says(L, Edge.substr(0, Edge.find(' ', 20))), (What + ": GTO's edge line").c_str());
			Expect(Says(L, Pad ? "Hold A and your plan takes the seat" : "Press K and your plan takes the seat") && !Says(L, Pad ? "Press K" : "Hold A and"),
				(What + ": how the plan takes the seat and gives it back, in this device's keys").c_str());
			// Each device's keys: the keyboard's arrows, K, digits and Enter; the pad's A and X (held for the two holds).
			if (Pad)
			{
				Expect(!DrewRun(L, "ENTER") && !DrewRun(L, "1-5") && Count(L, "HOLD") == 2, (What + ": the pad's A held to turn it on or off, X held to skip").c_str());
			}
			else
			{
				Expect(DrewRun(L, "ENTER") && DrewRun(L, "1-5") && Count(L, "HOLD") == 1, (What + ": the keyboard's K, 1 to 5, and Enter held").c_str());
				ss::ui::TableCardInfo P = C;
				P.Gamepad = true;
				const ss::ui::DrawList Lp = Card(P, Width);
				Expect(Count(L, "5") == Count(Lp, "5") + 1 && Count(L, "1") == Count(Lp, "1") + 1,
					(What + ": the keyboard's digits over the first five targets (none on a pad)").c_str());
			}
			Expect(!Says(L, "YOUR DECISIONS WAIT") && !Says(L, "YOUR PLAN KEEPS PLAYING"), (What + ": the footer's right is the page's keys").c_str());
			Expect(OnlySystemAi(L, 1), (What + ": \"AI\" only in the header; never \"bot\" (U1)").c_str());
			Fits(L, Width, What);
			Readable(L, What);
			if (!Pad && Width == Widths[0])
			{
				WriteList("table_card_play", L);
			}
			else if (Pad && Width == Widths[1])
			{
				WriteList("table_card_play_2257", L);
			}
		}
	}
	// The selected style follows the plan; a hidden one isn't on the menu, and a plan on one (an old save's) shows it with why.
	{
		ss::ui::TableCardInfo C = PlayCard(&T.Clock, false);
		const ss::style::Info& Bounty = ss::style::InfoOf(ss::style::Style::BountyHunter);
		const std::string Why = ss::autoplay::CantPick(ss::style::Style::BountyHunter, 1, false);
		Expect(!Card(C, 1440.0f).Cmds.empty() && (!Bounty.Hidden || !Says(Card(C, 1440.0f), Bounty.Name)), "PLAY page: a hidden style isn't among the tiles");
		C.Play.Panel.Now.S = ss::style::Style::BountyHunter;
		const ss::ui::DrawList L = Card(C, 1440.0f);
		Expect(Says(L, Bounty.Name) && !Why.empty() && Says(L, Why), ("PLAY page: a plan whose style can't be picked here shows it with why (" + Why + ")").c_str());
		C.Play.Panel.Now.S = ss::style::Style::Lag;
		for (ss::style::Info& Inf : C.Play.Panel.Styles)
		{
			Inf.Hidden = Inf.Hidden || Inf.S == ss::style::Style::Maniac;
		}
		const ss::ui::DrawList H = Card(C, 1440.0f);
		Expect(!Says(H, "Maniac") && Says(H, "LAG") && Says(H, ss::style::SwingName(ss::style::InfoOf(ss::style::Style::Lag).Var)),
			"PLAY page: a hidden style isn't on the menu (the ship rule); LAG shown with its swing");
	}
	// Off: the state says so, and Sharpness is what the plan would get now (no "set when" line).
	{
		ss::ui::TableCardInfo C = PlayCard(&T.Clock, false);
		C.Play.Panel.On = false;
		C.Play.Panel.Now.M = ss::autoplay::Mode::Off;
		const ss::ui::DrawList L = Card(C, 1920.0f);
		Expect(DrewRun(L, "OFF") && !Says(L, "WATCHING") && !Says(L, "Set when your plan took the seat"), "PLAY page: off says OFF, with no set time");
		C.Play.Panel.On = true;
		C.Play.Panel.Now.M = ss::autoplay::Mode::Skip;
		Expect(DrewRun(Card(C, 1920.0f), "SKIPPING"), "PLAY page: a skip says SKIPPING");
	}
	// A sloppy plan: the warning in autoplay's own words, its causes read back off SharpWhy.
	for (const bool Tilted : {false, true})
	{
		ss::ui::TableCardInfo C = PlayCard(&T.Clock, false);
		ss::autoplay::Panel& P = C.Play.Panel;
		P.Form = 70.0;
		P.Sharp = 0.63;
		P.SharpWhy = ss::autoplay::SharpWhy(70.0, false, Tilted ? 0.6 : 0.0, true, true);
		const std::string Warn = ss::autoplay::SharpWarning(0.63, 70.0, Tilted ? 0.6 : 0.0, true, true);
		ss::ui::DrawList L;
		ss::ui::Canvas Cv(L, Measure, 1920.0f, 1080.0f, 1.0f);
		ss::ui::DrawTableCard(Cv, C, Now);
		const std::string Text = Joined(L);
		std::string Flat;
		for (const char Ch : Text)
		{
			Flat.push_back(Ch == '\n' ? ' ' : Ch);
		}
		Expect(!Warn.empty() && Flat.find(Warn) != std::string::npos && Says(L, "63%") && (Warn.find("on tilt") != std::string::npos) == Tilted,
			(std::string("PLAY page: the sloppy plan's warning, word for word") + (Tilted ? " (on tilt)" : "")).c_str());
		Fits(L, 1920.0f, "PLAY page with the warning");
	}
	// Dee's: no 8x (a room of people you know), the tournament styles play as their cash style, no skip yet.
	for (const float Width : Widths)
	{
		ss::ui::TableCardInfo C = PlayCard(nullptr, true);
		C.Play.Panel.Now.S = ss::style::Style::PushFold;
		C.Play.Panel.Now.Speed = 4;
		C.Gamepad = Width == Widths[2];
		const ss::ui::DrawList L = Card(C, Width);
		const std::string What = "table_card_play_dees at " + W(Width);
		Expect(Says(L, "Push/Fold") && Says(L, "Plays as TAG here") && Says(L, "WATCHING \xC2\xB7 4x") && Says(L, "4x") && !DrewRun(L, "8x"),
			(What + ": Push/Fold plays as TAG here; 1x to 4x").c_str());
		Expect(!Says(L, "SKIP TO") && !Says(L, "STOPS") && !DrewRun(L, "1-5") && !Says(L, "LEVEL"), (What + ": no skip at a cash game yet, no clock").c_str());
		Fits(L, Width, What);
		Readable(L, What);
		if (Width == Widths[0])
		{
			WriteList("table_card_play_dees", L);
		}
	}
	// Dee's first night: her line, and nothing starts (no keys, no styles).
	for (const float Width : Widths)
	{
		ss::ui::TableCardInfo C = PlayCard(nullptr, true);
		C.Play.Panel.On = false;
		C.Play.Panel.Now.M = ss::autoplay::Mode::Off;
		C.Play.Locked = ss::autoplay::Text(ss::autoplay::Words::DeeFirstNight);
		const ss::ui::DrawList L = Card(C, Width);
		const std::string What = "table_card_play_locked at " + W(Width);
		Expect(Says(L, "AI AUTOPLAY") && Says(L, "First night, you play your own cards.") && Says(L, "Dee:"), (What + ": Dee's line").c_str());
		Expect(!Says(L, "STYLE") && !Says(L, "SHARPNESS") && !Says(L, "AUTOPLAY ON / OFF") && !Says(L, "WATCH SPEED"), (What + ": and nothing to start").c_str());
		Fits(L, Width, What);
		Readable(L, What);
		if (Width == Widths[0])
		{
			WriteList("table_card_play_locked", L);
		}
	}
	// The other pages' footer: the player's decisions wait while the card is up; a plan holding the seat keeps playing.
	{
		ss::ui::TableCardInfo C = PlayCard(&T.Clock, false);
		C.Page = 0;
		Expect(Says(Card(C, 1920.0f), "YOUR PLAN KEEPS PLAYING") && !Says(Card(C, 1920.0f), "YOUR DECISIONS WAIT"), "table card: with the plan on, YOUR PLAN KEEPS PLAYING");
		C.Play.Panel.TakenOver = true;
		Expect(Says(Card(C, 1920.0f), "YOUR DECISIONS WAIT"), "table card: a hand taken over is the player's: YOUR DECISIONS WAIT");
		C.Play.Panel.TakenOver = false;
		C.Play.Panel.On = false;
		Expect(Says(Card(C, 1920.0f), "YOUR DECISIONS WAIT") && !Says(Card(C, 1920.0f), "YOUR PLAN"), "table card: with the plan off, YOUR DECISIONS WAIT");
		// The tabs still fit with the sixth, the level beside them at 4:3.
		C.Page = PlayIndex();
		Expect(Says(Card(C, 1440.0f), "LEVEL 9 \xC2\xB7 800 / 1,600 \xC2\xB7 ANTE 1,600") && Says(Card(C, 1440.0f), "PLAY"), "table card: six tabs and the level fit at 1440");
	}
}

/** The same page from a real session's AutoPanel (core-autoplay's Panel as it comes): off, then GTO watched at 2x. */
void PlayFromSession()
{
	QuietHooks H;
	ss::Session S(H, "tableauto-play");
	S.CurrentScreen = ss::Screen::Lobby;
	S.Register(0);
	Expect(S.CurrentScreen == ss::Screen::Table, "PLAY page from a session: a table");
	ss::ui::TableCardInfo C;
	C.Where = ss::ui::Venue::CardRoom;
	C.Page = PlayIndex();
	C.Play.Panel = S.AutoPanel(0);
	ss::ui::DrawList L = Card(C, 1920.0f);
	const std::string Default = ss::style::InfoOf(C.Play.Panel.Now.S).Name;
	Expect(DrewRun(L, "OFF") && Says(L, Default) && Says(L, C.Play.Panel.SharpWhy) && !C.Play.Panel.SharpWhy.empty(), "PLAY page from a session: off, the default style, Sharpness's why");
	ss::autoplay::TableAuto A;
	A.S = ss::style::Style::Gto;
	A.M = ss::autoplay::Mode::Watch;
	A.Speed = 2;
	const std::string Why = S.SetAutoplay(0, A);
	Expect(Why.empty(), ("PLAY page from a session: GTO turns on (" + Why + ")").c_str());
	C.Play.Panel = S.AutoPanel(0);
	L = Card(C, 1920.0f);
	Expect(Says(L, "WATCHING \xC2\xB7 2x") && Says(L, "GTO") && Says(L, "Set when your plan took the seat, at ") && Says(L, ss::style::InfoOf(ss::style::Style::Gto).BestIn),
		"PLAY page from a session: GTO watched at 2x, its Sharpness held since it took the seat");
	Fits(L, 1920.0f, "PLAY page from a session");
	Readable(L, "PLAY page from a session");
}

// ------------------------------------------------------------------ the plan's lines on the HUD (P0a)

void Golden(const char* Name, void (*Build)(Table&, float), const std::vector<std::string>& Must, const std::vector<std::string>& MustNot,
	const std::vector<std::string>& KeysOnly, const std::vector<std::string>& PadOnly)
{
	for (const float Width : Widths)
	{
		for (const bool Pad : {false, true})
		{
			Table T;
			Build(T, Width);
			T.I.Gamepad = Pad;
			const ss::ui::DrawList L = Frame(T.I, Width);
			const std::string What = std::string(Name) + " at " + W(Width) + (Pad ? " (pad)" : " (keys)");
			for (const std::string& N : Must)
			{
				Expect(Says(L, N), (What + ": says \"" + N + "\"").c_str());
			}
			for (const std::string& N : MustNot)
			{
				Expect(!Says(L, N), (What + ": doesn't say \"" + N + "\"").c_str());
			}
			for (const std::string& N : Pad ? PadOnly : KeysOnly)
			{
				Expect(Says(L, N), (What + ": says \"" + N + "\" on this device").c_str());
			}
			for (const std::string& N : Pad ? KeysOnly : PadOnly)
			{
				Expect(!Says(L, N), (What + ": doesn't say \"" + N + "\" on this device").c_str());
			}
			Expect(OnlySystemAi(L, 0), (What + ": never \"AI\" or \"bot\" on the table HUD").c_str());
			Fits(L, Width, What);
			Readable(L, What);
			for (const float Sc : {0.8f, 1.3f})
			{
				Table S;
				Build(S, Width);
				S.I.Gamepad = Pad;
				S.I.Scale = Sc;
				Fits(Frame(S.I, Width), Width, What + " at HUD size " + std::to_string(static_cast<int>(Sc * 100.0f)) + "%");
			}
			if (!Pad && Width == Widths[0])
			{
				WriteList(Name, L);
			}
			else if (Pad && Width == Widths[1])
			{
				WriteList(std::string(Name) + "_2257", L);
			}
		}
	}
}

void HudLines()
{
	const std::string Think = ss::autoplay::ThinkingLine(ss::style::Style::Gto);
	Golden("table_hud_auto_watch", Watching, {"AUTOPLAY \xC2\xB7 GTO \xC2\xB7 Sharpness 93%", "YOUR STACK", "31 BB", "how many chips. dont round up", "PRIZE POOL", "13.1 BB"},
		{"RAISE TO", "to call", "is thinking", "Take the wheel"}, {"Take over"}, {});
	Golden("table_hud_auto_think", Thinking, {"AUTOPLAY \xC2\xB7 GTO 2x \xC2\xB7 Sharpness 93%", Think, "Take the wheel"},
		{"RAISE TO", "to call", "CALLED", "RAISED", "CHECKED", "FOLDED"}, {"Take over"}, {});
	Golden("table_hud_auto_resume", Resuming, {"AUTO RESUMES"}, {"AUTOPLAY \xC2\xB7 GTO", "RAISE TO"}, {"Keep playing"}, {});
	Golden("table_hud_auto_skip", Skipping, {"SKIPPING \xC2\xB7 PUSH/FOLD", "Level 9 \xC2\xB7 31 left of 88 \xC2\xB7 you 42 BB", "Stop the skip", "PRIZE POOL", "YOUR STACK"},
		{"Dana Whitfield", "Mrs. Park", "13.1 BB", "POT 4,000", "AUTOPLAY \xC2\xB7", "RAISE TO", "is thinking"}, {}, {});
	// The think's keys on each device: the keyboard's F C R A, the pad's X A Y (the first press only takes the wheel).
	{
		Table T;
		Thinking(T, 1920.0f);
		const ss::ui::DrawList Keys = Frame(T.I, 1920.0f);
		const float ThinkY = 1080.0f - 120.0f;
		const float StatusY = 1080.0f - 190.0f;
		Expect(KeyAt(Keys, "F", 1920.0f, ThinkY) && KeyAt(Keys, "C", 1920.0f, ThinkY) && KeyAt(Keys, "R", 1920.0f, ThinkY) && KeyAt(Keys, "A", 1920.0f, ThinkY) &&
				   KeyAt(Keys, "K", 1920.0f, StatusY),
			"HUD: the think's keys F C R A, and K on the status line to take over");
		T.I.Gamepad = true;
		const ss::ui::DrawList Pad = Frame(T.I, 1920.0f);
		Expect(KeyAt(Pad, "X", 1920.0f, ThinkY) && KeyAt(Pad, "A", 1920.0f, ThinkY) && KeyAt(Pad, "Y", 1920.0f, ThinkY) && !KeyAt(Pad, "F", 1920.0f, ThinkY) &&
				   !KeyAt(Pad, "K", 1920.0f, StatusY),
			"HUD: the pad's X A Y take the wheel (and no K)");
	}
	// U1: the plan's move is never on screen before it's played, even when the host left the last one up.
	{
		Table T;
		Thinking(T, 1920.0f);
		T.I.AutoLastAction = "CALLED 3,200";
		Expect(!Says(Frame(T.I, 1920.0f), "CALLED 3,200"), "HUD: during the think, no action shows (not even the last street's)");
		T.I.AutoDeciding = false;
		T.I.AutoLastAction = "RAISED TO 9,000";
		const ss::ui::DrawList L = Frame(T.I, 1920.0f);
		Expect(Says(L, "GTO \xC2\xB7 RAISED TO 9,000") && !Says(L, Think), "HUD: once played, the plan's last action");
	}
	// The host's own lines win their places: a Status (racking up) and a Prompt.
	{
		Table T;
		Thinking(T, 1920.0f);
		T.I.Status = "Racking up after this hand";
		T.I.Prompt = "Your seat's on hold";
		const ss::ui::DrawList L = Frame(T.I, 1920.0f);
		Expect(Says(L, "Racking up after this hand") && Says(L, "Your seat's on hold") && !Says(L, "AUTOPLAY \xC2\xB7") && !Says(L, Think), "HUD: the host's Status and Prompt win their places");
	}
	// A decision handed back (a stop rule, a take-over): the ladder is up, and the plan's lines step aside.
	{
		Table T;
		Watching(T, 1920.0f);
		T.I.YourTurn = true;
		T.I.CanRaise = true;
		T.I.ToCall = 1600;
		ss::sizing::Spot& Sp = T.I.Spot;
		Sp.On = ss::Street::Preflop;
		Sp.Pot = 4000;
		Sp.ToCall = 1600;
		Sp.CurrentBet = 1600;
		Sp.Bb = 1600;
		Sp.MinTo = 3200;
		Sp.MaxTo = 49600;
		Sp.Effective = 49600;
		Sp.Ante = 1600;
		Sp.Unit = 100;
		T.I.Presets = ss::sizing::Presets(Sp);
		T.I.RaiseTo = ss::sizing::Default(Sp);
		const ss::ui::DrawList L = Frame(T.I, 1920.0f);
		Expect(Says(L, "1,600 to call") && Says(L, "RAISE TO") && !Says(L, "AUTOPLAY \xC2\xB7") && !Says(L, "Take the wheel") && !Says(L, "dont round up"),
			"HUD: a decision handed back gets the ladder (and holds the texts), not the plan's lines");
	}
	// Out of the hand, T is the plan's watch speed (the table's pace is untouched), and the hints say so.
	{
		Table T;
		Watching(T, 1920.0f);
		T.I.AutoSpeed = 4;
		T.I.InHand = false;
		T.I.Pace = 0;
		T.I.HintsAt = Now - 1.0;
		const ss::ui::DrawList L = Frame(T.I, 1920.0f);
		Expect(Says(L, "WATCH SPEED") && KeyAt(L, "T", 1920.0f, 1080.0f - 62.0f) && Says(L, "4x") && !Says(L, "TABLE PACE") && !Says(L, "LIVE") && !Says(L, "NEXT HAND"),
			"HUD: out of the hand, T steps the plan's WATCH SPEED (4x), and the hints say WATCH SPEED");
		T.I.AutoOn = false;
		const ss::ui::DrawList Off = Frame(T.I, 1920.0f);
		Expect(Says(Off, "TABLE PACE") && Says(Off, "LIVE") && !Says(Off, "WATCH SPEED"), "HUD: with the plan off, T is the table's pace as before");
	}
	// The skip: no plates on the dimmed felt, and no glyph spots for the host's atlas cells either.
	{
		Table T;
		Skipping(T, 1920.0f);
		T.I.Study.Name = "Mrs. Park";
		T.I.Study.BadgeCells = {3};
		T.I.Study.Alpha = 1.0f;
		Expect(ss::ui::HudGlyphs(T.I, 1920.0f, Now).empty(), "HUD: a live skip leaves no glyph spots (no plates, no studying caption)");
		T.I.Gamepad = true;
		const ss::ui::DrawList L = Frame(T.I, 1920.0f);
		Expect(KeyAt(L, "B", 1920.0f, 1080.0f * 0.4f + 148.0f) && Says(L, "Stop the skip"), "HUD: the pad's B stops the skip");
		T.I.Gamepad = false;
		Expect(KeyAt(Frame(T.I, 1920.0f), "K", 1920.0f, 1080.0f * 0.4f + 148.0f), "HUD: the keyboard's K stops the skip");
		T.I.SkipOn = false;
		Expect(!ss::ui::HudGlyphs(T.I, 1920.0f, Now).empty(), "HUD: the plates' glyph spots come back after the skip");
	}
	// The plates keep last segment's guarantees under the plan: the same layout with its fields set as without.
	for (const float Width : Widths)
	{
		Table T;
		Thinking(T, Width);
		const std::vector<ss::ui::PlacedPlate> With = ss::ui::LayoutPlates(T.I, Width, Now);
		ss::ui::TableHudInfo Plain = T.I;
		Plain.AutoOn = false;
		Plain.AutoDeciding = false;
		Plain.AutoStyle.clear();
		const std::vector<ss::ui::PlacedPlate> Without = ss::ui::LayoutPlates(Plain, Width, Now);
		bool Same = With.size() == Without.size() && With.size() == 5;
		for (size_t K = 0; K < With.size() && Same; ++K)
		{
			Same = With[K].Box.X == Without[K].Box.X && With[K].Box.Y == Without[K].Box.Y;
		}
		bool Clear = true;
		for (const ss::ui::PlacedPlate& Pl : With)
		{
			for (const ss::ui::Rect& K : T.I.KeepClear)
			{
				const ss::ui::Rect& R = Pl.Footprint;
				Clear = Clear && !(R.X < K.X + K.W && K.X < R.X + R.W && R.Y < K.Y + K.H && K.Y < R.Y + R.H);
			}
		}
		Expect(Same && Clear, ("HUD: under the plan the plates lay out as without it, off the faces, cards and chips, at " + W(Width)).c_str());
		const ss::ui::DrawList L = Frame(T.I, Width);
		Expect(Says(L, "13.1 BB") && Says(L, "40.1 BB"), ("HUD: under the plan every stack still shows at " + W(Width)).c_str());
	}
}

// ------------------------------------------------------------------ the control map

void ControlMap()
{
	using ss::ui::ControlContext;
	auto Is = [](ControlContext Ctx, const char* Id, const char* Key, const char* Pad, bool Hold) {
		const ss::ui::Control* K = ss::ui::FindControl(Ctx, Id);
		return K && std::string(K->Key) == Key && std::string(K->Pad) == Pad && K->Hold == Hold;
	};
	Expect(Is(ControlContext::Table, "auto", "K", "", false), "controls: K turns the plan on and off at the table (a pad does it on the PLAY page)");
	Expect(Is(ControlContext::Table, "wheel", "F C R A", "X A Y", false), "controls: any action key takes the wheel during the plan's think");
	Expect(Is(ControlContext::Table, "stopskip", "K", "B", false), "controls: K or B stops a skip");
	const ss::ui::Control* Pace = ss::ui::FindControl(ControlContext::Table, "pace");
	const ss::ui::Control* Watch = ss::ui::FindControl(ControlContext::Table, "watch");
	Expect(Pace && Watch && std::string(Watch->Key) == Pace->Key && std::string(Watch->Pad) == Pace->Pad && std::string(Watch->Label) == "WATCH SPEED",
		"controls: T (right on the d-pad) is the watch speed while the plan holds the seat");
	Expect(Is(ControlContext::TableCard, "style", "LEFT RIGHT", "A", false) && Is(ControlContext::TableCard, "auto", "K", "", false) &&
			   Is(ControlContext::TableCard, "autohold", "", "A", true) && Is(ControlContext::TableCard, "target", "1-5", "X", false) &&
			   Is(ControlContext::TableCard, "skip", "ENTER", "X", true),
		"controls: the PLAY page's arrows or A, K or A held, 1 to 5 or X, Enter or X held");
	// The PLAY page uses only what's free while the card is held: none of its keys is the card's own.
	std::vector<std::string> Taken;
	for (const char* Id : {"page", "units", "card", "back"})
	{
		const ss::ui::Control* K = ss::ui::FindControl(ControlContext::TableCard, Id);
		for (const char* Keys : {K->Key, K->Pad})
		{
			std::string Cur;
			for (const char* P = Keys;; ++P)
			{
				if (*P == ' ' || *P == 0)
				{
					if (!Cur.empty())
					{
						Taken.push_back(Cur);
					}
					Cur.clear();
					if (*P == 0)
					{
						break;
					}
				}
				else
				{
					Cur.push_back(*P);
				}
			}
		}
	}
	bool Free = true;
	for (const char* Id : {"style", "auto", "autohold", "target", "skip"})
	{
		const ss::ui::Control* K = ss::ui::FindControl(ControlContext::TableCard, Id);
		for (const std::string& T : Taken)
		{
			for (const char* Keys : {K->Key, K->Pad})
			{
				const std::string S = std::string(" ") + Keys + " ";
				Free = Free && S.find(" " + T + " ") == std::string::npos;
			}
		}
	}
	Expect(Free, "controls: the PLAY page's keys are free under the held card (not Q E U TAB ESC, LB RB Y UP B)");
}

// ------------------------------------------------------------------ the copy

/**
 * The copy lint's update-2 rule (SPEC §2.11, §6.7): a line that promises the player's decisions "always wait for you"
 * says "autoplay" too, since autoplay is the one thing that ever acts for them. "" when the line is fine. It lands here
 * with TableSettingHelp's qualifier (never before it); patches/core-tableui_to_lead_1.diff moves it into
 * tests/CopyLint.h's Check, where every suite's lint runs it.
 */
std::string AutoplayWaitRule(const std::string& Line)
{
	const std::string Low = copylint::Lower(Line);
	if (Low.find("always wait for you") != std::string::npos && Low.find("autoplay") == std::string::npos)
	{
		return "promises \"always wait for you\" without \"autoplay\"";
	}
	return std::string();
}

void Copy()
{
	// The rule catches today's lines and passes the qualified ones.
	Expect(!AutoplayWaitRule("Your own decisions always wait for you.").empty() && !AutoplayWaitRule("Quicker decisions around you. Yours always wait for you.").empty() &&
			   !AutoplayWaitRule("your own decisions always wait for you").empty(),
		"copy rule: \"always wait for you\" without \"autoplay\" is caught");
	Expect(AutoplayWaitRule("Your own decisions wait for you, unless you've handed the seat to autoplay.").empty() &&
			   AutoplayWaitRule("Your decisions always wait for you, unless autoplay has the seat.").empty() && AutoplayWaitRule("YOUR DECISIONS WAIT").empty(),
		"copy rule: a qualified line passes");
	// The qualifier (§2.11's words) in both devices' help.
	for (const bool Pad : {false, true})
	{
		const std::string Help = ss::ui::TableSettingHelp("tablepace", Pad);
		Expect(Help.find("Your own decisions wait for you, unless you've handed the seat to autoplay.") != std::string::npos && AutoplayWaitRule(Help).empty() &&
				   copylint::Check(Help, copylint::LetterBody, copylint::Samples()).empty(),
			"copy: TableSettingHelp(\"tablepace\") carries autoplay's qualifier");
	}
	// The rule over every inventory these screens draw from: the table's, autoplay's and the styles'.
	int Problems = 0;
	for (const std::pair<std::string, std::string>& Line : ss::ui::TableUiCopy())
	{
		const std::string Why = AutoplayWaitRule(Line.second);
		Problems += Why.empty() ? 0 : 1;
		if (!Why.empty())
		{
			std::printf("COPY (table ui) %s: \"%s\": %s\n", Line.first.c_str(), Line.second.c_str(), Why.c_str());
		}
	}
	std::vector<std::string> Lines = ss::autoplay::Inventory();
	const std::vector<std::string> Styles = ss::style::Inventory();
	Lines.insert(Lines.end(), Styles.begin(), Styles.end());
	for (const std::string& Line : Lines)
	{
		const std::string Why = AutoplayWaitRule(Line);
		Problems += Why.empty() ? 0 : 1;
		if (!Why.empty())
		{
			std::printf("COPY (autoplay) \"%s\": %s\n", Line.c_str(), Why.c_str());
		}
	}
	Expect(Problems == 0, "copy rule: no inventory promises the decisions always wait without autoplay");
	// The PLAY page's own sentences are in the table's inventory and pass the lint.
	int Found = 0;
	for (const std::pair<std::string, std::string>& Line : ss::ui::TableUiCopy())
	{
		const bool Clean = copylint::Check(Line.second, copylint::LetterBody, copylint::Samples()).empty();
		Found += Line.first == "play.sharpset" && Line.second == "Set when your plan took the seat, at 9:47 PM." && Clean ? 1 : 0;
		Found += (Line.first == "play.how.pad" || Line.first == "play.how.keys") && Clean && Line.second.find("It never shows a move before it makes it.") != std::string::npos ? 1 : 0;
	}
	Expect(Found == 3, "copy: the PLAY page's sentences are in TableUiCopy and pass the lint");
}

/** SS_UI_BENCH=1: the PLAY page's and the plan's HUD's draw time (the HUD's budget: 0.3 ms). */
void Bench()
{
	if (!std::getenv("SS_UI_BENCH"))
	{
		return;
	}
	Table T;
	Thinking(T, 1920.0f);
	auto Time = [](const char* What, const std::function<void(ss::ui::Canvas&, double)>& Draw) {
		ss::ui::DrawList L;
		const auto T0 = std::chrono::steady_clock::now();
		for (int K = 0; K < 200; ++K)
		{
			L.Clear();
			ss::ui::Canvas C(L, Measure, 1920.0f, 1080.0f, 1.0f);
			Draw(C, Now + K * 0.016);
		}
		std::printf("UI BENCH %-28s %6.3f ms/draw %6zu vertices\n", What, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count() / 200.0,
			L.Vertices.size());
	};
	Time("table_hud_auto_think", [&T](ss::ui::Canvas& C, double At) { ss::ui::DrawTableHud(C, T.I, At); });
	Table S;
	Skipping(S, 1920.0f);
	Time("table_hud_auto_skip", [&S](ss::ui::Canvas& C, double At) { ss::ui::DrawTableHud(C, S.I, At); });
	const ss::ui::TableCardInfo Card = PlayCard(&T.Clock, false);
	Time("table_card_play", [&Card](ss::ui::Canvas& C, double At) { ss::ui::DrawTableCard(C, Card, At); });
}
} // namespace tableauto_seg

void SegTableAuto()
{
	tableauto_seg::Bench();
	tableauto_seg::PlayPage();
	tableauto_seg::PlayFromSession();
	tableauto_seg::HudLines();
	tableauto_seg::ControlMap();
	tableauto_seg::Copy();
}
} // namespace ui_test
