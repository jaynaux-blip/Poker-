// riverline_ui: core-tableui's additions (next_segment_spec §7.6, §8.3), compiled into ui_test and called from main() in
// ui_main.cpp (the shared kit is UiSeg.h):
// - the table HUD's goldens (table_hud_turn_open, table_hud_allin_count, table_hud_break, table_hud_bubble,
//   table_hud_itm, table_hud_onfoot_marker, table_hud_rail_onair, table_hud_dees) at 1920, 2257 (2550x1220) and 1440,
//   each with its text inside the canvas and none of it under 13 units, at HUD sizes 80% and 130% too;
// - the plates (the AI stack sizes): on every live seat, always, faded off the player's turn, never over the faces,
//   cards and chips the host marks or each other, edge plates for seats off screen, the same layout every frame;
// - the table card's five pages (table_card_reads ... table_card_night), the room's clock screens, the desk and the cage
//   (desk_register_funding, cage_receipt_split);
// - ControlParity (no table action on B; all in is a hold on both devices; Y held 0.2 s raises, 0.5 s does nothing,
//   1.1 s goes all in, and a held Y never raises first), SettingsMigration (the AUDIO and GAMEPLAY rows), the caption
//   rules, and the copy lint over the table's sentences.
#include "UiSeg.h"

#include "CopyLint.h"
#include "ShortStack/Game/ChipSet.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/UI/Captions.h"
#include "ShortStack/UI/FrontEnd.h"
#include "ShortStack/UI/TableHud.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace ui_test
{
namespace tableui_seg
{
const TableMeasurer Measure;
const float Wide = 1080.0f * 2550.0f / 1220.0f;
const float Widths[3] = {1920.0f, Wide, 1440.0f};

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

/** The HUD's minimum (§1.7): 13 logical units. */
void Readable(const ss::ui::DrawList& L, float Min, const std::string& What)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && !Cmd.Text.Text.empty() && Cmd.Text.SizePx < Min - 0.01f)
		{
			Expect(false, (What + ": no text under " + std::to_string(static_cast<int>(Min)) + " units (\"" + Cmd.Text.Text + "\" is " +
							  std::to_string(Cmd.Text.SizePx) + ")")
							  .c_str());
			return;
		}
	}
}

ss::ui::DrawList Frame(const ss::ui::TableHudInfo& I, float Width, double Now)
{
	ss::ui::DrawList L;
	ss::ui::Canvas C(L, Measure, Width, 1080.0f, 1.0f);
	ss::ui::DrawTableHud(C, I, Now);
	return L;
}

// ------------------------------------------------------------------ the scenes (§6.1)

const double Now = 500.0;

struct Scene
{
	ss::live::ClockFacts Clock;
	ss::ui::TableHudInfo I;
	ss::life::State Life;
	ss::recall::Record Last;
	std::vector<ss::ui::Rect> Faces;
};

/** Where a seat's stack lands on screen at this width (the vertical field of view is fixed: wider windows see more). */
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

/** The faces, cards and chips at each seat (what the host projects for KeepClear). */
void MarkSeat(Scene& S, const ss::ui::SeatPlate& P)
{
	if (!P.OnScreen)
	{
		return;
	}
	S.I.KeepClear.push_back({P.X - 50.0f, P.Y - 35.0f, 100.0f, 50.0f});   // the stack
	S.I.KeepClear.push_back({P.X + 52.0f, P.Y - 42.0f, 70.0f, 44.0f});   // the cards
	S.I.KeepClear.push_back({P.X - 62.0f, P.Y - 240.0f, 124.0f, 160.0f}); // the face
}

/** Tuesday Nightly, level 9 (800/1,600, ante 1,600), 41 of 94 left; the player on the button with 49,600. */
void Seated(Scene& S, float Width)
{
	ss::live::ClockFacts& F = S.Clock;
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
	F.NextJumpAt = 14;
	F.NextJumpCents = 24300;
	F.Funding = "BACKED \xC2\xB7 BIG LOU 100%";
	F.ColorUp.clear();
	F.TableId = 12;
	F.Seat = 4;
	ss::ui::TableHudInfo& I = S.I;
	I = ss::ui::TableHudInfo();
	I.Where = ss::ui::Venue::CardRoom;
	I.Clock = &S.Clock;
	I.Event = "Nightly $120";
	I.HeroStack = 49600;
	I.Bb = 1600;
	I.Pot = 4000;
	I.PotAt = {Width * 0.5f, 560.0f};
	I.PotOnScreen = true;
	I.OnFoot.Clock = "9:47 PM";
	S.Life.Energy = 26.0;
	I.OnFoot.Life = &S.Life;
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
		MarkSeat(S, P);
	}
	I.KeepClear.push_back({Width * 0.5f - 150.0f, 840.0f, 300.0f, 220.0f}); // the player's own cards
	I.Bpm = 88.0f;
	for (int K = 0; K < 480; ++K)
	{
		const float Ph = std::fmod(static_cast<float>(K) / 240.0f * 1.47f, 1.0f);
		I.Trace.push_back(Ph < 0.06f ? Ph / 0.06f : Ph < 0.1f ? 1.0f - (Ph - 0.06f) / 0.04f : 0.0f);
	}
	I.Focus = 0.62f;
	I.CaptionLook = ss::ui::CaptionStyle();
	ss::ui::Caption Pa;
	Pa.Style = 1;
	Pa.Line = "Players, the blinds are now 800 and 1,600, with a big blind ante of 1,600.";
	Pa.At = Now - 1.0;
	ss::ui::Caption Talk;
	Talk.Speaker = "Sal";
	Talk.Line = "Average is about forty-six thousand. Somebody's got all of mine.";
	Talk.At = Now - 0.5;
	I.Captions = {Pa, Talk};
	I.Gamepad = false;
}

void TurnOpen(Scene& S, float Width)
{
	Seated(S, Width);
	ss::ui::TableHudInfo& I = S.I;
	I.YourTurn = true;
	I.CanRaise = true;
	I.ToCall = 1600;
	ss::sizing::Spot& Sp = I.Spot;
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
	Sp.InPosition = true;
	I.Presets = ss::sizing::Presets(Sp);
	I.RaiseTo = ss::sizing::Default(Sp);
	I.Preset = ss::sizing::Snap(Sp, I.RaiseTo, 0);
	I.HeroCards = {48, 41}; // A♠ Q♣... as dealt
	I.Plates[0].LastAction = "Fold";
	I.Plates[0].Folded = true;
	I.Plates[0].ActedAt = Now - 3.0;
	ss::ui::Toast Lou;
	Lou.From = "Big Lou";
	Lou.Line = "how many chips. dont round up";
	Lou.At = Now - 1.0;
	I.Toasts = {Lou}; // held: it's the player's decision
}

void AllInCount(Scene& S, float Width)
{
	Seated(S, Width);
	ss::ui::TableHudInfo& I = S.I;
	I.Readout = ss::ui::StackReadout::Room;
	const std::vector<ss::chipset::Denom> Chips = ss::chipset::InPlay(ss::chipset::Room::Embercrest, ss::live::DeepstackLevels(), 8, 6);
	for (ss::ui::SeatPlate& P : I.Plates)
	{
		P.Read = ss::chipset::ReadStack(P.Stack, Chips, false);
	}
	ss::ui::SeatPlate& Rick = I.Plates[0];
	Rick.LastAction = "All in";
	Rick.LastTo = 18400;
	Rick.Bet = 18400;
	Rick.Stack = 0;
	Rick.AllIn = true;
	Rick.ActedAt = Now - 0.8;
	Rick.Read = ss::chipset::ReadStack(18400, Chips, true);
	I.Plates[1].Read = ss::chipset::ReadStack(21000, Chips, true); // a long look counted Mrs. Park
	I.YourTurn = true;
	I.CanRaise = true;
	I.ToCall = 18400;
	I.Pot = 22400;
	ss::sizing::Spot& Sp = I.Spot;
	Sp.On = ss::Street::Preflop;
	Sp.Pot = 22400;
	Sp.ToCall = 18400;
	Sp.CurrentBet = 18400;
	Sp.Bb = 1600;
	Sp.MinTo = 35200;
	Sp.MaxTo = 49600;
	Sp.Effective = 18400;
	Sp.Raises = 1;
	Sp.Ante = 1600;
	Sp.Unit = 100;
	I.Presets = ss::sizing::Presets(Sp);
	I.RaiseTo = ss::sizing::Default(Sp);
	I.Preset = ss::sizing::Snap(Sp, I.RaiseTo, 0);
	I.AllInHold = 0.5f;
	ss::ui::Caption Count;
	Count.Speaker = "Dealer";
	Count.Line = "All in. " + ss::chipset::Spoken(18400);
	Count.At = Now - 0.4;
	I.Captions.push_back(Count);
}

void Break(Scene& S, float Width)
{
	Seated(S, Width);
	ss::live::ClockFacts& F = S.Clock;
	F.OnBreak = true;
	F.BreakSecondsLeft = 9.0 * 60.0 + 40.0;
	F.Level = 13;
	F.Now = {2000, 4000, 4000};
	F.Next = {2500, 5000, 5000};
	F.LevelSecondsLeft = 1200.0;
	F.BreakBeforeLevel = 19;
	F.SecondsToBreak = F.BreakSecondsLeft + 6.0 * 1200.0;
	F.Remaining = 33;
	F.Average = 56969;
	F.ColorUp = "COLORING UP THE BLACK 100s";
	S.I.Bb = 4000;
	ss::ui::Toast Lou;
	Lou.From = "Big Lou";
	Lou.Line = "how many chips. dont round up";
	Lou.At = Now - 1.0;
	S.I.Toasts = {Lou};
	ss::ui::Caption Pa;
	Pa.Style = 1;
	Pa.Line = "Players, on this break we're coloring up the black hundreds. Leave them out on the felt and the dealers will race them off.";
	Pa.At = Now - 2.0;
	S.I.Captions = {Pa};
}

void Bubble(Scene& S, float Width)
{
	Seated(S, Width);
	ss::live::ClockFacts& F = S.Clock;
	F.Level = 15;
	F.Now = {3000, 6000, 6000};
	F.Next = {4000, 8000, 8000};
	F.Remaining = 15;
	F.ToTheMoney = 1;
	F.HandForHand = true;
	F.Average = 125333;
	F.BreakBeforeLevel = 19;
	F.NextJumpAt = 14;
	S.I.Bb = 6000;
	S.I.HeroStack = 58800;
	S.I.Plates[1].Stack = 21000;
	S.I.Plates[1].Looked = true;
	S.I.Plates[3].Acting = true;
}

void Itm(Scene& S, float Width)
{
	Bubble(S, Width);
	ss::live::ClockFacts& F = S.Clock;
	F.Remaining = 14;
	F.HandForHand = false;
	F.InTheMoney = true;
	F.ToTheMoney = 0;
	F.NextJumpAt = 12;
	F.NextJumpCents = 5300;
}

void OnFootMarker(Scene& S, float Width)
{
	Break(S, Width);
	ss::ui::TableHudInfo& I = S.I;
	I.Walking = true;
	I.Toasts.clear();
	I.OnFoot.Place = "THE EMBERCREST \xC2\xB7 CARD ROOM";
	I.OnFoot.Clock = "11:24 PM";
	I.OnFoot.World = 2.0 * 1440.0 + 23.0 * 60.0 + 24.0;
	I.OnFoot.BankrollCents = 41260;
	I.OnFoot.Prompt = "Talk to the floor";
	I.HintsAt = Now - 1.0;
	I.SeatMarker.Text = "TABLE 12 \xC2\xB7 SEAT 4";
	I.SeatMarker.Sub = "BREAK 9:40";
	I.SeatMarker.X = Width * 0.5f + 420.0f;
	I.SeatMarker.Y = 520.0f;
	I.SeatMarker.Metres = 18.0f;
	I.SeatMarker.OnScreen = true;
	I.Plates.clear();
	I.KeepClear.clear();
}

void RailOnAir(Scene& S, float Width)
{
	Seated(S, Width);
	ss::ui::TableHudInfo& I = S.I;
	I.OnAir = true;
	I.HeroStack = 0;
	I.Event = "Embercrest Sunday $150";
	I.NextOutCents = 35600;
	S.Clock.Remaining = 6;
	S.Clock.InTheMoney = true;
	I.Plates.push_back(Seat(5, "Corinne Vasquez", "", 412000, 0.0f, 700.0f, Width));
}

void Dees(Scene& S, float Width)
{
	Seated(S, Width);
	ss::ui::TableHudInfo& I = S.I;
	I.Where = ss::ui::Venue::BackRoom;
	I.Clock = nullptr;
	I.Event.clear();
	I.Bb = 2;
	I.HeroStack = 248;
	I.Pot = 46;
	const ss::Chips Stacks[5] = {212, 340, 96, 455, 180};
	const char* const Names[5] = {"Sal", "Big Lou", "Twitch", "Mei", "Dee"};
	for (size_t K = 0; K < I.Plates.size(); ++K)
	{
		I.Plates[K].Stack = Stacks[K];
		I.Plates[K].Name = Names[K];
	}
	I.YourTurn = true;
	I.CanRaise = true;
	I.ToCall = 10;
	ss::sizing::Spot& Sp = I.Spot;
	Sp.On = ss::Street::Flop;
	Sp.Pot = 46;
	Sp.ToCall = 10;
	Sp.CurrentBet = 10;
	Sp.Bb = 2;
	Sp.MinTo = 20;
	Sp.MaxTo = 248;
	Sp.Effective = 248;
	Sp.Raises = 1;
	Sp.Unit = 1;
	I.Presets = ss::sizing::Presets(Sp);
	I.RaiseTo = ss::sizing::Default(Sp);
	I.Preset = ss::sizing::Snap(Sp, I.RaiseTo, 0);
}

// ------------------------------------------------------------------ the checks

void Golden(const char* Name, void (*Build)(Scene&, float), const std::vector<std::string>& Must, const std::vector<std::string>& MustNot, bool Seated_ = true)
{
	for (const float Width : Widths)
	{
		Scene S;
		Build(S, Width);
		const ss::ui::DrawList L = Frame(S.I, Width, Now);
		const std::string What = std::string(Name) + " at " + W(Width);
		Expect(!L.Cmds.empty() && L.Vertices.size() > 200, (What + ": drew").c_str());
		for (const std::string& N : Must)
		{
			Expect(Says(L, N), (What + ": says \"" + N + "\"").c_str());
		}
		for (const std::string& N : MustNot)
		{
			Expect(!Says(L, N), (What + ": doesn't say \"" + N + "\"").c_str());
		}
		Fits(L, Width, What);
		if (Seated_)
		{
			Readable(L, 13.0f, What);
		}
		if (Width == Widths[0])
		{
			WriteList(Name, L);
		}
		else if (Width == Widths[1])
		{
			WriteList(std::string(Name) + "_2257", L);
		}
		// The HUD size setting: panels scale about their anchors and still fit.
		for (const float Sc : {0.8f, 1.3f})
		{
			Scene T;
			Build(T, Width);
			T.I.Scale = Sc;
			Fits(Frame(T.I, Width, Now), Width, What + " at HUD size " + std::to_string(static_cast<int>(Sc * 100.0f)) + "%");
		}
	}
}

void HudGoldens()
{
	Golden("table_hud_turn_open", TurnOpen,
		{"LEVEL 9", "14:32", "BACKED \xC2\xB7 BIG LOU 100%", "NIGHTLY $120", "TABLE 12 \xC2\xB7 SEAT 4", "800 / 1,600 \xC2\xB7 ANTE 1,600", "BEFORE LEVEL 13", "41 / 94",
			"45,854 \xC2\xB7 28.7 BB", "$9,870 \xC2\xB7 14 PAID", "11.5 BB", "18,400", "13.1 BB", "21,000", "40.1 BB", "64,200", "1,600 to call", "RAISE TO", "2.2x", "ALL IN", "HOLD",
			"31 BB", "49,600", "FOLD", "CALL", "TIRED", "FLOOR"},
		{"how many chips"});
	Golden("table_hud_allin_count", AllInCount, {"18,400 to call", "EFFECTIVE 11.5 BB", "POT 22,400", "ALL IN 18,400", "COUNTED", "LET GO TO CANCEL", "Eighteen thousand four hundred.", "\xE2\x89\x88"},
		{});
	Golden("table_hud_break", Break, {"BREAK", "9:40", "COLORING UP THE BLACK 100s", "2,000 / 4,000 \xC2\xB7 ANTE 4,000", "BIG LOU", "how many chips. dont round up"}, {"RAISE TO"});
	Golden("table_hud_bubble", Bubble, {"HAND FOR HAND \xC2\xB7 1 FROM THE MONEY", "9.8 BB", "58,800", "3.5 BB", "1 FROM THE MONEY \xC2\xB7 MIN CASH $243"}, {});
	Golden("table_hud_itm", Itm, {"IN THE MONEY \xC2\xB7 NEXT JUMP AT 12 LEFT +$53", "NEXT JUMP AT 12 LEFT \xC2\xB7 +$53"}, {"FROM THE MONEY"});
	Golden("table_hud_onfoot_marker", OnFootMarker, {"TABLE 12 \xC2\xB7 SEAT 4", "18 m", "BREAK 9:40", "11:24 PM", "BREAK 9:40 \xC2\xB7 LEVEL 13 2,000/4,000", "INTERACT", "PAUSE"},
		{"YOUR STACK", "EAT OR DRINK"}, false);
	Golden("table_hud_rail_onair", RailOnAir, {"ON AIR", "EMBERCREST SUNDAY $150 \xC2\xB7 FEATURE TABLE", "NEXT OUT $356", "Corinne Vasquez", "412,000"}, {"YOUR STACK"});
	Golden("table_hud_dees", Dees, {"$212", "$340", "$248", "$10 to call", "POT $46", "RAISE TO"}, {"LEVEL", "PRIZE POOL", "BB \xC2\xB7"});
	// The bubble's stack is under the 15-BB line: ember gold, said with its number.
	{
		Scene S;
		Bubble(S, 1920.0f);
		const ss::ui::DrawList L = Frame(S.I, 1920.0f, Now);
		bool Gold = false;
		for (const ss::ui::DrawCmd& Cmd : L.Cmds)
		{
			if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == "9.8 BB" && Cmd.Text.SizePx >= 39.0f)
			{
				Gold = std::fabs(Cmd.Text.Col.R - 0xf6 / 255.0f) < 0.02f && std::fabs(Cmd.Text.Col.G - 0xb0 / 255.0f) < 0.02f;
			}
		}
		Expect(Gold, "table HUD: 9.8 BB is ember gold in the hero block");
	}
	// The toast lane holds while it's the player's decision, and shows once it isn't.
	{
		Scene S;
		TurnOpen(S, 1920.0f);
		Expect(!Says(Frame(S.I, 1920.0f, Now), "dont round up"), "table HUD: a text waits while the action's on the player");
		S.I.YourTurn = false;
		Expect(Says(Frame(S.I, 1920.0f, Now), "dont round up"), "table HUD: the text shows once the decision's made");
	}
	// YOUR EYES at the rail: the lower third names the table, and the counts stay on the stage's monitors.
	{
		Scene S;
		RailOnAir(S, 1920.0f);
		S.I.Readout = ss::ui::StackReadout::Eyes;
		const ss::ui::DrawList L = Frame(S.I, 1920.0f, Now);
		Expect(Says(L, "ON AIR") && Says(L, "Corinne Vasquez") && !Says(L, "412,000") && !Says(L, "64,200"), "table HUD: YOUR EYES keeps the counts off the lower third");
	}
}

void Plates()
{
	for (const float Width : Widths)
	{
		Scene S;
		TurnOpen(S, Width);
		const std::string What = "plates at " + W(Width);
		const std::vector<ss::ui::PlacedPlate> P = ss::ui::LayoutPlates(S.I, Width, Now);
		Expect(P.size() == 5, (What + ": a plate on every one of the five AI stacks").c_str());
		bool Apart = true;
		bool Clear = true;
		bool Inside = true;
		for (size_t A = 0; A < P.size(); ++A)
		{
			const ss::ui::Rect& R = P[A].Footprint;
			Inside = Inside && R.X >= 57.0f && R.X + R.W <= Width - 57.0f && R.Y >= 329.0f && R.Y <= 761.0f;
			for (size_t B = A + 1; B < P.size(); ++B)
			{
				const ss::ui::Rect& Q = P[B].Footprint;
				Apart = Apart && !(R.X < Q.X + Q.W && Q.X < R.X + R.W && R.Y < Q.Y + Q.H && Q.Y < R.Y + R.H);
			}
			for (const ss::ui::Rect& K : S.I.KeepClear)
			{
				Clear = Clear && !(R.X < K.X + K.W && K.X < R.X + R.W && R.Y < K.Y + K.H && K.Y < R.Y + R.H);
			}
		}
		Expect(Apart, (What + ": no plate over another").c_str());
		Expect(Clear, (What + ": no plate over a face, the cards or the chips").c_str());
		Expect(Inside, (What + ": plates inside x 58..W-58 and y 330..760").c_str());
		// The same info, the same layout: plates never jump while nothing moves.
		const std::vector<ss::ui::PlacedPlate> Again = ss::ui::LayoutPlates(S.I, Width, Now + 0.5);
		bool Same = Again.size() == P.size();
		for (size_t K = 0; K < P.size() && Same; ++K)
		{
			Same = P[K].Box.X == Again[K].Box.X && P[K].Box.Y == Again[K].Box.Y;
		}
		Expect(Same, (What + ": the layout is the same every frame").c_str());
		// Strength: full on the player's decision; 70% off it; the seat looked at and one that just acted at full.
		float Folded = 0.0f;
		for (const ss::ui::PlacedPlate& Pl : P)
		{
			const ss::ui::SeatPlate& Sp = S.I.Plates[static_cast<size_t>(Pl.Index)];
			if (Sp.Folded)
			{
				Folded = Pl.Alpha;
			}
			else
			{
				Expect(Pl.Alpha > 0.99f, (What + ": full strength on the player's decision").c_str());
			}
		}
		Expect(std::fabs(Folded - 0.55f) < 0.01f, (What + ": a folded seat at 55%").c_str());
		S.I.YourTurn = false;
		S.I.Plates[2].Looked = true;
		S.I.Plates[3].ActedAt = Now - 1.0;
		S.I.Plates[4].ActedAt = Now - 2.5;
		for (const ss::ui::PlacedPlate& Pl : ss::ui::LayoutPlates(S.I, Width, Now))
		{
			const int Seat = Pl.Index;
			const float Want = Seat == 0 ? 0.7f * 0.55f : Seat == 2 || Seat == 3 ? 1.0f : 0.7f;
			Expect(std::fabs(Pl.Alpha - Want) < 0.01f, (What + ": seat " + std::to_string(Seat) + " at its strength off the player's turn").c_str());
		}
		// Always on: off the player's turn every plate still draws its stack.
		const ss::ui::DrawList L = Frame(S.I, Width, Now);
		Expect(Says(L, "13.1 BB") && Says(L, "17.2 BB") && Says(L, "40.1 BB") && Says(L, "19.9 BB"), (What + ": every stack shows off the player's turn").c_str());
		// U flips the units: chips the big number, big blinds beside.
		S.I.Units = ss::ui::StackUnits::Chips;
		const ss::ui::DrawList Lc = Frame(S.I, Width, Now);
		bool ChipsBig = false;
		for (const ss::ui::DrawCmd& Cmd : Lc.Cmds)
		{
			ChipsBig = ChipsBig || (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == "21,000" && Cmd.Text.SizePx >= 16.0f);
		}
		Expect(ChipsBig, (What + ": chips as the big number").c_str());
		// YOUR EYES: no numbers on a plate.
		S.I.Readout = ss::ui::StackReadout::Eyes;
		const ss::ui::DrawList Le = Frame(S.I, Width, Now);
		Expect(!Says(Le, "21,000") && !Says(Le, "13.1 BB") && Says(Le, "Mrs. Park"), (What + ": YOUR EYES shows names and no numbers").c_str());
	}
	// At the rail the nearest seat sits low, where the captions are (above the lower third): its plate goes where a line
	// said won't cover it.
	for (const float Width : Widths)
	{
		Scene S;
		RailOnAir(S, Width);
		const float Mw = std::min(980.0f, Width - 840.0f);
		const ss::ui::Rect Said = {Width * 0.5f - Mw * 0.5f, 1080.0f - 58.0f - 132.0f - 16.0f - 152.0f, Mw, 152.0f};
		bool Clear = true;
		for (const ss::ui::PlacedPlate& Pl : ss::ui::LayoutPlates(S.I, Width, Now))
		{
			const ss::ui::Rect& R = Pl.Box;
			Clear = Clear && !(R.X < Said.X + Said.W && Said.X < R.X + R.W && R.Y < Said.Y + Said.H && Said.Y < R.Y + R.H);
		}
		Expect(Clear, ("plates: at the rail no plate sits under the captions at " + W(Width)).c_str());
	}
	// A seat off a 16:9 screen gets an edge plate, a chevron toward it.
	{
		Scene S;
		TurnOpen(S, 1440.0f);
		S.I.Plates[4].X = 1500.0f;
		S.I.Plates[4].OnScreen = false;
		const std::vector<ss::ui::PlacedPlate> P = ss::ui::LayoutPlates(S.I, 1440.0f, Now);
		bool Edge = false;
		for (const ss::ui::PlacedPlate& Pl : P)
		{
			if (Pl.Index == 4)
			{
				Edge = Pl.Edge && Pl.Toward == 1 && Pl.Box.X + Pl.Box.W <= 1440.0f - 57.0f;
			}
		}
		Expect(Edge, "plates: a seat off screen gets an edge plate pointing at it");
	}
	// Crowded: plates whose stacks sit close still come apart.
	{
		Scene S;
		TurnOpen(S, 1920.0f);
		for (size_t K = 0; K < S.I.Plates.size(); ++K)
		{
			S.I.Plates[K].X = 900.0f + static_cast<float>(K) * 30.0f;
			S.I.Plates[K].Y = 520.0f;
		}
		S.I.KeepClear.clear();
		const std::vector<ss::ui::PlacedPlate> P = ss::ui::LayoutPlates(S.I, 1920.0f, Now);
		bool Apart = true;
		for (size_t A = 0; A < P.size(); ++A)
		{
			for (size_t B = A + 1; B < P.size(); ++B)
			{
				const ss::ui::Rect& R = P[A].Footprint;
				const ss::ui::Rect& Q = P[B].Footprint;
				Apart = Apart && !(R.X < Q.X + Q.W && Q.X < R.X + R.W && R.Y < Q.Y + Q.H && Q.Y < R.Y + R.H);
			}
		}
		Expect(Apart, "plates: five stacks bunched together still get five clear plates");
	}
	// The badge glyph's place for the host's atlas cell, and the studying caption's row.
	{
		Scene S;
		TurnOpen(S, 1920.0f);
		S.I.Study.Name = "Mrs. Park";
		S.I.Study.Label = "Regular";
		S.I.Study.Honorific = "a Crest member";
		S.I.Study.BadgeCells = {3, 7};
		S.I.Study.Alpha = 1.0f;
		const std::vector<ss::ui::GlyphSpot> G = ss::ui::HudGlyphs(S.I, 1920.0f, Now);
		Expect(G.size() == 3 && G[0].Cell == 3 && G[0].Size == 16.0f && G[1].Size == 26.0f, "plates: the host draws one glyph on a plate and the studying caption's row");
		const ss::ui::DrawList L = Frame(S.I, 1920.0f, Now);
		Expect(Says(L, "MRS. PARK") && Says(L, "a Crest member"), "table HUD: the studying caption with its honorific");
	}
}

// ------------------------------------------------------------------ the table card

void CardPages()
{
	Scene S;
	Bubble(S, 1920.0f);
	ss::ui::TableCardInfo C;
	C.Clock = &S.Clock;
	C.Reads = {"Rick: a long look at his chips means strength", "Mrs. Park: a quick call means a draw (seen once)"};
	for (int K = 0; K < 10; ++K)
	{
		ss::recall::Record R;
		R.Number = 212 - K;
		R.Level = 15;
		R.Blinds = {3000, 6000, 6000};
		R.Mine = {48 - K, 30 + K};
		if (K % 3 != 2)
		{
			R.Board = {4, 17, 33};
		}
		if (K % 2 == 0)
		{
			R.Board.push_back(9);
			R.Board.push_back(22);
		}
		R.Lines = {{ss::Street::Preflop, "Rick", "raises to", 13200}, {ss::Street::Preflop, "You", "calls", 13200}};
		if (K == 0)
		{
			R.Shown = {{"Rick", {47, 46}}};
		}
		R.HeroNet = K % 2 == 0 ? -18400 : 6400 * K;
		C.Hands.push_back(R);
	}
	// The ladder at the bubble: 14 paid of 94.
	const ss::Chips Pay[11] = {287300, 195100, 140200, 101300, 77800, 61500, 49600, 41200, 35600, 29600, 24300};
	const int From[11] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 13};
	const int To[11] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 14};
	for (int K = 0; K < 11; ++K)
	{
		C.Pay.push_back({From[K], To[K], Pay[K], false});
	}
	C.HeroPlace = 15;
	C.Field = {{"Rick", "Knows you", 7, 52300, false}, {"Mei", "", 3, 88100, false}, {"Mrs. Park", "Regular", 12, 21000, true}, {"Sal", "Regular", 9, -1, false}};
	C.CountsAt = "9:00";
	for (int K = 0; K < 40; ++K)
	{
		ss::ui::Caption Ln;
		Ln.Style = K % 5 == 0 ? 1 : K % 7 == 0 ? 2 : 0;
		Ln.Speaker = K % 2 == 0 ? "Mrs. Park" : "Rick";
		Ln.Line = Ln.Style == 1 ? "Players, the blinds are now 500 and 1,000, with a big blind ante of 1,000." : Ln.Style == 2 ? "[Applause.]" : "Seat four, it's on you. Take your time, we've got all night.";
		Ln.Clock = ss::ClockString(19.0 * 60.0 + static_cast<double>(K) * 4.0);
		C.Night.push_back(Ln);
	}
	C.NightTitle = "The night \xC2\xB7 Embercrest Nightly";
	static const char* const Names[5] = {"table_card_reads", "table_card_hands", "table_card_payouts", "table_card_field", "table_card_night"};
	const std::vector<std::vector<std::string>> Must = {
		{"READS", "a long look at his chips means strength"},
		{"#212", "L15 \xC2\xB7 3,000/6,000", "Rick showed down.", "-3.1 BB"},
		{"14 PAID \xC2\xB7 PRIZE POOL $9,870 \xC2\xB7 POSTED", "13\xE2\x80\x93" "14", "$243", "$2,873", "1st", "YOU \xC2\xB7 15TH IF YOU WENT OUT NOW \xC2\xB7 1 FROM THE MONEY"},
		{"15 OF 94 LEFT", "COUNTS AT THE 9:00 BREAK", "TABLE 7", "52,300 \xC2\xB7 8.7 BB", "YOUR TABLE", "NOT COUNTED YET"},
		{"THE NIGHT \xC2\xB7 EMBERCREST NIGHTLY", "FLOOR", "Players, the blinds are now 500 and 1,000", "[Applause.]"},
	};
	for (int Page = 0; Page < 5; ++Page)
	{
		for (const float Width : Widths)
		{
			C.Page = Page;
			C.Gamepad = Width == Widths[1];
			ss::ui::DrawList L;
			ss::ui::Canvas Cv(L, Measure, Width, 1080.0f, 1.0f);
			ss::ui::DrawTableCard(Cv, C, Now);
			const std::string What = std::string(Names[Page]) + " at " + W(Width);
			for (const std::string& N : Must[static_cast<size_t>(Page)])
			{
				Expect(Says(L, N), (What + ": says \"" + N + "\"").c_str());
			}
			Expect(Says(L, "READS") && Says(L, "NIGHT") && Says(L, "SHOWING BIG BLINDS"), (What + ": the tabs and the units").c_str());
			Fits(L, Width, What);
			Readable(L, 13.0f, What);
			if (Width == Widths[0])
			{
				WriteList(Names[Page], L);
			}
		}
	}
	// The night log holds the newest that fit, in order, ending with the newest.
	{
		C.Page = 4;
		C.Night.back().Line = "The last thing said tonight, so far.";
		ss::ui::DrawList L;
		ss::ui::Canvas Cv(L, Measure, 1920.0f, 1080.0f, 1.0f);
		ss::ui::DrawTableCard(Cv, C, Now);
		Expect(Says(L, "The last thing said tonight, so far."), "table card: the night log ends on the newest line");
	}
}

void RoomClocks()
{
	Scene S;
	Seated(S, 1920.0f);
	S.Clock.LateReg.clear();
	const std::vector<std::string> Today = {"12:00 PM\tNoon Deepstack $80\tDONE", "7:00 PM\tNightly $120\tRUNNING", "10:30 PM\tMidnight Turbo $60\tREG OPEN"};
	std::vector<ss::life::RoomLeader> Leaders;
	const char* const Names[10] = {"Mrs. Park", "Rick", "Mei", "Sal", "Corinne Vasquez", "Jesse Cole", "Big Lou", "Dana Whitfield", "Theo Marsh", "Ana Lucero"};
	for (int K = 0; K < 10; ++K)
	{
		Leaders.push_back({K + 1, Names[K], 412.0 - K * 31.0, std::string()});
	}
	std::vector<ss::live::PayRow> Pay = {{1, 1, 287300, false}, {2, 2, 195100, false}, {3, 3, 140200, false}, {10, 12, 29600, false}, {13, 14, 24300, false}};
	static const char* const Pages[4] = {"room_clock_clock", "room_clock_payouts", "room_clock_today", "room_clock_leaders"};
	const std::vector<std::vector<std::string>> Must = {
		{"LEVEL 9", "14:32", "800 / 1,600 \xC2\xB7 ANTE 1,600", "NEXT \xC2\xB7 1,000 / 2,000 \xC2\xB7 ANTE 2,000", "41 / 94", "$9,870", "THE EMBERCREST", "NIGHTLY $120"},
		{"14 PAID \xC2\xB7 $9,870 \xC2\xB7 POSTED", "$2,873", "13\xE2\x80\x93" "14"},
		{"TODAY AT THE EMBERCREST", "NIGHTLY $120", "RUNNING", "10:30 PM"},
		{},
	};
	for (int Page = 0; Page < 4; ++Page)
	{
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, Measure, 1024.0f, 576.0f, 1.0f);
		ss::ui::DrawRoomClock(C, S.Clock, Page, Today, Leaders, "Jesse Cole", Now, "Nightly $120", Pay);
		const std::string What = Pages[Page];
		for (const std::string& N : Must[static_cast<size_t>(Page)])
		{
			Expect(Says(L, N), (What + ": says \"" + N + "\"").c_str());
		}
		Fits(L, 1024.0f, What);
		if (Page < 3)
		{
			Readable(L, 13.0f, What); // (LEADERS is core-badges' art)
		}
		WriteList(Pages[Page], L);
	}
	// An empty month says so, in voice.
	{
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, Measure, 1024.0f, 576.0f, 1.0f);
		ss::ui::DrawRoomClock(C, S.Clock, 3, Today, {}, "Jesse Cole", Now);
		Expect(Says(L, "No results posted this month yet."), "room clock: LEADERS with nobody posted");
	}
}

// ------------------------------------------------------------------ the controls

void ControlParity()
{
	using ss::ui::ControlContext;
	// No table action on B but backing out; all in is a hold on both devices.
	bool OnlyBack = true;
	for (const ss::ui::Control& K : ss::ui::Controls(ControlContext::Table))
	{
		for (const std::string& Pad : {std::string(K.Pad)})
		{
			const bool B = Pad == "B" || Pad.rfind("B ", 0) == 0 || Pad.find(" B") != std::string::npos;
			OnlyBack = OnlyBack && (!B || std::string(K.Id) == "back");
		}
	}
	Expect(OnlyBack, "ControlParity: nothing at the table is on B but backing out");
	const ss::ui::Control* AllIn = ss::ui::FindControl(ControlContext::Table, "allin");
	Expect(AllIn && AllIn->Hold && std::string(AllIn->Key) == "A" && std::string(AllIn->Pad) == "Y", "ControlParity: all in is A held on the keyboard and Y held on the pad");
	const ss::ui::Control* Raise = ss::ui::FindControl(ControlContext::Table, "raise");
	Expect(Raise && !Raise->Hold && std::string(Raise->Pad) == "Y", "ControlParity: Y raises");
	// On foot in the card room, the street's own controls.
	for (const char* Id : {"move", "run", "view", "use", "pause"})
	{
		const ss::ui::Control* Street = ss::ui::FindControl(ControlContext::Street, Id);
		const ss::ui::Control* Room = ss::ui::FindControl(ControlContext::CardRoomOnFoot, Id);
		Expect(Street && Room && std::string(Street->Key) == Room->Key && std::string(Street->Pad) == Room->Pad && Street->Hold == Room->Hold,
			(std::string("ControlParity: on foot in the card room, ") + Id + " is the street's").c_str());
	}
	Expect(std::string(ss::ui::FindControl(ControlContext::CardRoomOnFoot, "pause")->Pad) == "B", "ControlParity: B pauses on foot");
	// Y: a tap raises, a half-second hold does nothing, a long hold goes all in, and a hold never raises first.
	using Out = ss::ui::HoldButton::Out;
	auto HoldFor = [](ss::ui::HoldButton Btn, double Seconds, bool& RaisedFirst) {
		Btn.Press(10.0);
		Out Fired = Out::None;
		for (double T = 10.0; T < 10.0 + Seconds; T += 1.0 / 60.0)
		{
			const Out O = Btn.Tick(T);
			RaisedFirst = RaisedFirst || O == Out::Tap;
			Fired = O != Out::None ? O : Fired;
		}
		const Out Last = Btn.Release(10.0 + Seconds);
		return Fired != Out::None ? Fired : Last;
	};
	ss::ui::HoldButton Y;
	bool RaisedFirst = false;
	Expect(HoldFor(Y, 0.2, RaisedFirst) == Out::Tap, "ControlParity: Y held 0.2 s raises");
	Expect(HoldFor(Y, 0.5, RaisedFirst) == Out::None, "ControlParity: Y held 0.5 s does nothing");
	Expect(HoldFor(Y, 1.1, RaisedFirst) == Out::Hold, "ControlParity: Y held 1.1 s goes all in");
	Expect(!RaisedFirst, "ControlParity: a held Y never raises first");
	{
		ss::ui::HoldButton Btn;
		Btn.Press(0.0);
		Expect(Btn.Progress(0.3) == 0.0f && Btn.Progress(0.65) > 0.49f && Btn.Progress(0.65) < 0.51f && Btn.Cancelling(0.5) && !Btn.Cancelling(0.2),
			"ControlParity: the ALL IN fill starts at 0.35 s and says let go to cancel");
		Expect(Btn.Tick(0.96) == Out::Hold && Btn.Tick(1.2) == Out::None && Btn.Release(1.3) == Out::None, "ControlParity: all in fires once, at 0.95 s");
	}
	ss::ui::HoldButton A;
	A.CanTap = false;
	bool Ignored = false;
	Expect(HoldFor(A, 0.2, Ignored) == Out::None && HoldFor(A, 0.7, Ignored) == Out::Hold, "ControlParity: the keyboard's A is a hold, and a tap does nothing");
	Expect(ss::ui::ControlsNotice(true) == "All in is Y held now. B backs out.", "ControlParity: the first live table's line");
	// The table's hints draw each device's glyphs.
	Scene S;
	Seated(S, 1920.0f);
	S.I.HintsAt = Now - 1.0;
	S.I.Gamepad = true;
	const ss::ui::DrawList L = Frame(S.I, 1920.0f, Now);
	Expect(Says(L, "STUDY A FACE") && Says(L, "RT") && Says(L, "TABLE CARD"), "ControlParity: the pad's hints at the table");
}

// ------------------------------------------------------------------ settings

struct SettingsHooks : ss::ui::FrontEndHooks
{
	int Changes = 0;
	void SettingsChanged(const ss::ui::GameSettings&) override { ++Changes; }
};

void SettingsMigration()
{
	// Today's settings (from before the segment) read to the new rows' defaults; a full string round-trips.
	ss::ui::GameSettings Old;
	Expect(ss::ui::GameSettings::Parse("shortstack.settings.v1\nquality=2\nmaster=70\ntablepace=1\n", Old) && Old.StackReadout == 2 && Old.StackUnits == 0 &&
			   Old.TurnCue == 2 && Old.HudScale == 100 && Old.Captions == 1 && Old.CaptionSize == 100 && Old.CaptionBackdrop == 72 && Old.SpeakerNames && Old.TableTalk == 1 &&
			   Old.VoicesVolume == 100 && Old.TablePace == 1,
		"SettingsMigration: an old string reads to the defaults");
	SettingsHooks H;
	ss::ui::FrontEnd Fe(H);
	Fe.Open(ss::ui::FrontEnd::Page::Pause, Now);
	Fe.Key("Down", Now);
	Fe.Key("Enter", Now);
	Expect(Fe.Current() == ss::ui::FrontEnd::Page::Settings, "settings: open from the pause menu");
	for (int K = 0; K < 4; ++K)
	{
		Fe.Key("E", Now);
	}
	// GAMEPLAY: table pace, table talk, stack sizes, stack units, turn cue, HUD size.
	Fe.Key("Down", Now);
	Fe.Key("Right", Now); // table talk: lively
	Fe.Key("Down", Now);
	Fe.Key("Right", Now); // stack sizes: SHOWN -> THE ROOM'S WAY
	Fe.Key("Down", Now);
	Fe.Key("Right", Now); // stack units: chips
	Fe.Key("Down", Now);
	Fe.Key("Left", Now); // turn cue: dealer
	Fe.Key("Down", Now);
	Fe.Key("Right", Now); // HUD size: 105%
	Expect(Fe.Settings.TableTalk == 2 && Fe.Settings.StackReadout == 1 && Fe.Settings.StackUnits == 1 && Fe.Settings.TurnCue == 1 && Fe.Settings.HudScale == 105,
		"settings: the GAMEPLAY rows change what they say");
	// AUDIO: voices, captions, size, background, names.
	Fe.Key("Q", Now);
	Fe.Key("Q", Now);
	for (int K = 0; K < 3; ++K)
	{
		Fe.Key("Down", Now);
	}
	Fe.Key("Left", Now); // voices 95
	Fe.Key("Down", Now);
	Fe.Key("Down", Now);
	Fe.Key("Right", Now); // captions: everything
	Fe.Key("Down", Now);
	Fe.Key("Right", Now); // caption size 125
	Fe.Key("Down", Now);
	Fe.Key("Left", Now); // caption background 68
	Fe.Key("Down", Now);
	Fe.Key("Left", Now); // speaker names off
	Expect(Fe.Settings.VoicesVolume == 95 && Fe.Settings.Captions == 2 && Fe.Settings.CaptionSize == 125 && Fe.Settings.CaptionBackdrop == 68 && !Fe.Settings.SpeakerNames,
		"settings: the AUDIO rows change what they say");
	ss::ui::GameSettings Back;
	Expect(ss::ui::GameSettings::Parse(Fe.Settings.Serialize(), Back) && Back.Serialize() == Fe.Settings.Serialize(), "SettingsMigration: the new rows round-trip");
	Expect(H.Changes >= 10, "settings: every change is reported");
	// Both tabs draw and fit at every width, on both devices (the AUDIO tab is up: E E is GAMEPLAY, Q Q back).
	for (const float Width : Widths)
	{
		for (int Tab = 0; Tab < 2; ++Tab)
		{
			ss::ui::DrawList L;
			ss::ui::Canvas C(L, Measure, Width, 1080.0f, 1.0f);
			Fe.Gamepad = Width == Widths[2];
			Fe.Draw(C, Now + 2.0);
			const std::string What = std::string(Tab == 0 ? "settings AUDIO" : "settings GAMEPLAY") + " at " + W(Width);
			if (Tab == 0)
			{
				Expect(Says(L, "Voices") && Says(L, "Captions") && Says(L, "Caption size") && Says(L, "Speaker names") && Says(L, "EVERYTHING"), (What + ": the rows").c_str());
				WriteList(Width == Widths[0] ? std::string("settings_audio") : "settings_audio_" + W(Width), L);
			}
			else
			{
				Expect(Says(L, "Stack sizes") && Says(L, "THE ROOM'S WAY") && Says(L, "Turn cue") && Says(L, "HUD size") && Says(L, "GAMEPLAY"), (What + ": the rows").c_str());
				WriteList(Width == Widths[0] ? std::string("settings_gameplay") : "settings_gameplay_" + W(Width), L);
			}
			Fits(L, Width, What);
			Fe.Key(Tab == 0 ? "E" : "Q", Now);
			Fe.Key(Tab == 0 ? "E" : "Q", Now);
		}
	}
}

// ------------------------------------------------------------------ captions

int Boxes(const ss::ui::DrawList& L, const std::vector<std::string>& Lines)
{
	int N = 0;
	for (const std::string& S : Lines)
	{
		N += Says(L, S) ? 1 : 0;
	}
	return N;
}

void CaptionRules()
{
	using ss::ui::Caption;
	Expect(std::fabs(ss::ui::CaptionHold("Seat four, it's on you.", 0.0f) - 3.03f) < 0.05f, "captions: 1.5 s plus a second per 15 characters");
	Expect(ss::ui::CaptionHold("Ok.", 0.0f) == 2.6f && ss::ui::CaptionHold(std::string(200, 'a'), 0.0f) == 8.0f && ss::ui::CaptionHold("Ok.", 5.0f) == 5.0f,
		"captions: held 2.6 to 8 s, never shorter than the speech");
	std::vector<Caption> Lines;
	auto Add = [&](int Style, const char* Who, const char* Line, double At, bool Room, int Side) {
		Caption C;
		C.Style = Style;
		C.Speaker = Who;
		C.Line = Line;
		C.At = At;
		C.Room = Room;
		C.Side = Side;
		Lines.push_back(C);
	};
	Add(1, "", "Players, we are on the bubble. We're going hand for hand.", Now - 3.0, false, 0);
	Add(0, "Rick", "Hand for hand. Here we go.", Now - 2.5, false, 0);
	Add(0, "Mei", "All in and a call over here!", Now - 2.0, true, -1);
	Add(2, "", "[The rail groans.]", Now - 1.5, true, 1);
	Add(3, "Mrs. Park", "You going to make me wait all night?", Now - 1.0, false, 0);
	const std::vector<std::string> Texts = {"Players, we are on the bubble.", "Hand for hand. Here we go.", "All in and a call over here!", "[The rail groans.]", "You going to make me wait"};
	for (const float Width : Widths)
	{
		for (int Mode = 0; Mode < 3; ++Mode)
		{
			for (const int Size : {100, 150})
			{
				ss::ui::CaptionStyle Look;
				Look.Mode = Mode;
				Look.Size = Size;
				ss::ui::DrawList L;
				ss::ui::Canvas C(L, Measure, Width, 1080.0f, 1.0f);
				ss::ui::DrawCaptions(C, Lines, Look, 904.0f, std::min(980.0f, Width - 840.0f), Now);
				const std::string What = "captions mode " + std::to_string(Mode) + " at " + std::to_string(Size) + "% at " + W(Width);
				const int Shown = Boxes(L, Texts);
				if (Mode == 0)
				{
					Expect(L.Cmds.empty(), (What + ": off draws nothing").c_str());
				}
				else if (Size > 100 && Width < 1600.0f)
				{
					// 4:3 at 150%: the lines wrap (two rows a box); the floor still leads.
					Expect(Says(L, "FLOOR") && !Says(L, "[The rail groans.]") == (Mode == 1), (What + ": the floor kept").c_str());
				}
				else if (Mode == 1)
				{
					Expect(Shown == 3 && !Says(L, "[The rail groans.]") && !Says(L, "All in and a call over here!") && Says(L, "FLOOR"), (What + ": dialogue: the table, the floor and lines to you").c_str());
				}
				else
				{
					// Five live, three boxes: the oldest room lines go first, the floor stays.
					Expect(Shown == 3 && Says(L, "Players, we are on the bubble.") && Says(L, "You going to make me wait"), (What + ": everything, capped at three, the floor kept").c_str());
				}
				Fits(L, Width, What);
			}
		}
	}
	// Names off: no speaker, the floor still marked.
	{
		ss::ui::CaptionStyle Look;
		Look.Names = false;
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, Measure, 1920.0f, 1080.0f, 1.0f);
		ss::ui::DrawCaptions(C, Lines, Look, 904.0f, 980.0f, Now);
		Expect(!Says(L, "Rick") && Says(L, "FLOOR"), "captions: speaker names off");
	}
	// Everything mode shows where an off-screen speaker is.
	{
		ss::ui::CaptionStyle Look;
		Look.Mode = 2;
		Look.MaxShown = 5;
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, Measure, 1920.0f, 1080.0f, 1.0f);
		ss::ui::DrawCaptions(C, Lines, Look, 904.0f, 980.0f, Now);
		Expect(Says(L, "[The rail groans.]") && Says(L, "All in and a call over here!") && Says(L, "MEI") == false && Says(L, "Mei"), "captions: everything at five boxes");
	}
}

// ------------------------------------------------------------------ the desk and the cage

ss::ui::DrawList DeskFrame(ss::ui::DeskScreen& S, const ss::ui::DeskInfo& D, float Width, double At)
{
	ss::ui::DrawList L;
	ss::ui::Canvas C(L, Measure, Width, 1080.0f, 1.0f);
	S.Draw(C, D, At);
	return L;
}

void DeskAndCage()
{
	using namespace ss;
	live::Occurrence Nightly;
	live::Occurrence Turbo;
	for (int Day = 1; Day < 9 && !Turbo.Valid(); ++Day)
	{
		for (const live::Occurrence& O : live::Occurrences(Day))
		{
			if (std::string(O.T->Key) == "nightly")
			{
				Nightly = O;
			}
			if (std::string(O.T->Key) == "turbo" && Nightly.Valid() && Nightly.Day == Day)
			{
				Turbo = O;
			}
		}
	}
	Expect(Nightly.Valid() && Turbo.Valid(), "desk: a night with the Nightly and the Midnight Turbo");
	ui::DeskInfo D;
	D.World = Nightly.Start - 100.0;
	D.Bankroll = 41260;
	D.ClerkLine = "Evening. Nightly's got seats, and Lou left word he's good for it if you want it.";
	D.Card.Name = "Jesse Cole";
	D.Card.ScreenName = "grinder_3c";
	D.Card.Tier = 1;
	D.Card.Visits = 6;
	D.Card.New = true;
	D.CardPrinting = true;
	life::Funding Lou;
	Lou.From = life::Paid::Backer;
	Lou.Who = "Big Lou";
	Lou.Permille = 1000;
	Lou.Cents = 12000;
	life::Funding Cover;
	Cover.From = life::Paid::Sponsor;
	Cover.Who = "Team Embercrest";
	ui::DeskEvent E1;
	E1.O = Nightly;
	E1.Pay = {{Lou, ""}, {Cover, "Team Embercrest covers the Noon Deepstack and the Warm-Up, not the Nightly."}, {life::Funding(), ""}};
	ui::DeskEvent E2;
	E2.O = Turbo;
	E2.Pay = {{life::Funding(), ""}};
	D.Open = {E1, E2};
	D.Fields = {57, 0};
	for (const float Width : Widths)
	{
		ui::DeskScreen S;
		S.Gamepad = Width == Widths[1];
		S.Open(Now, "desk");
		const ui::DrawList L = DeskFrame(S, D, Width, Now + 2.0);
		const std::string What = "desk at " + W(Width);
		for (const char* N : {"THE DESK", "Embercrest Nightly $120", "$105 + $15 \xC2\xB7 20,000 CHIPS", "LOU'LL PUT YOU IN", "Big Lou is putting you in the Nightly.", "COVERED BY TEAM EMBERCREST",
				 "YOUR MONEY", "REGISTER", "57 IN SO FAR", "Midnight Turbo $60", "Lou left word"})
		{
			Expect(Says(L, N), (What + ": says \"" + N + "\"").c_str());
		}
		Fits(L, Width, What);
		(void)0; // (the club card is core-badges' art: its own sizes)
		if (Width == Widths[0])
		{
			WriteList("desk_register_funding", L);
		}
	}
	// The keys: it starts on the player's own money; a cover that doesn't apply says why and registers nothing; Lou's
	// stake registers with his option.
	ui::DeskScreen S;
	S.Open(Now, "desk");
	DeskFrame(S, D, 1920.0f, Now + 1.0);
	Expect(S.Selected() == 0 && S.PayChoice() == -1, "desk: the first event, nobody else's money preselected");
	S.Key("Left", Now + 1.0);
	S.Key("Enter", Now + 1.1);
	Expect(S.TakeRequests().empty(), "desk: a cover that doesn't apply registers nothing");
	Expect(Says(DeskFrame(S, D, 1920.0f, Now + 1.2), "Team Embercrest covers"), "desk: and the clerk says why");
	S.Key("Left", Now + 1.3);
	S.Key("Enter", Now + 1.4);
	std::vector<ui::DeskRequest> R = S.TakeRequests();
	Expect(R.size() == 1 && R[0].K == ui::DeskRequest::Register && R[0].Id == Nightly.Id && R[0].PayOption == 0 && R[0].Fund.From == life::Paid::Backer &&
			   R[0].Fund.Who == "Big Lou",
		"desk: REGISTER with Lou's stake, the option itself handed over");
	// Registered: the seat card prints; the refund is offered until the player sits.
	life::LiveEntry Mine;
	Mine.Id = Nightly.Id;
	Mine.Name = Nightly.T->Name;
	Mine.State = life::LiveEntry::Registered;
	Mine.RegisteredAt = D.World;
	Mine.ArrivedAt = D.World; // a desk registration is in the room already (FinishSignup): it hasn't sat, all the same
	D.Entry = &Mine;
	D.Seat = "TABLE 12 \xC2\xB7 SEAT 4 \xC2\xB7 20,000 CHIPS";
	const ui::DrawList In = DeskFrame(S, D, 1920.0f, Now + 2.0);
	Expect(Says(In, "YOU'RE IN") && Says(In, "EMBERCREST NIGHTLY $120 \xC2\xB7 TABLE 12 \xC2\xB7 SEAT 4 \xC2\xB7 20,000 CHIPS") && Says(In, "Leave and get refunded"), "desk: the seat card and the refund");
	Fits(In, 1920.0f, "desk registered");
	S.Key("R", Now + 2.1);
	R = S.TakeRequests();
	Expect(R.size() == 1 && R[0].K == ui::DeskRequest::Unregister && R[0].Id == Nightly.Id, "desk: leave and get refunded");
	S.Key("Y", Now + 2.15);
	R = S.TakeRequests();
	Expect(R.size() == 1 && R[0].K == ui::DeskRequest::Unregister, "desk: Y on the pad is the refund's button too");
	// Once a hand's been dealt to the player (the entry has a checkpoint), the money's in the game: no refund.
	{
		life::LiveEntry Playing = Mine;
		Playing.Checkpoint = "t1";
		ui::DeskInfo Dp = D;
		Dp.Entry = &Playing;
		const ui::DrawList Lp = DeskFrame(S, Dp, 1920.0f, Now + 2.16);
		S.Key("R", Now + 2.17);
		S.Key("Y", Now + 2.18);
		Expect(!Says(Lp, "Leave and get refunded") && Says(Lp, "YOU'RE IN") && S.TakeRequests().empty(), "desk: no refund once the player has played a hand");
		DeskFrame(S, D, 1920.0f, Now + 2.19);
	}
	// Nobody's offer is taken for the player: a package's roll is the default (the deal plays every event in range), else
	// their own money; Lou's stake and a cover are a move they make.
	{
		life::Funding Roll;
		Roll.From = life::Paid::Package;
		Roll.Who = "Lantern Row Staking";
		Roll.Permille = 650;
		ui::DeskInfo Dk = D;
		Dk.Entry = nullptr;
		Dk.Open = {E1};
		Dk.Open[0].Pay = {{Lou, ""}, {Roll, ""}, {life::Funding(), ""}};
		ui::DeskScreen Pk;
		Pk.Open(Now, "desk");
		DeskFrame(Pk, Dk, 1920.0f, Now + 1.0);
		Pk.Key("Enter", Now + 1.0);
		std::vector<ui::DeskRequest> Rk = Pk.TakeRequests();
		Expect(Rk.size() == 1 && Rk[0].PayOption == 1, "desk: an active package's roll is the default");
		Dk.Open[0].Pay = {{Lou, ""}, {life::Funding(), ""}};
		ui::DeskScreen Ow;
		Ow.Open(Now, "desk");
		DeskFrame(Ow, Dk, 1920.0f, Now + 1.0);
		Ow.Key("A", Now + 1.0);
		Rk = Ow.TakeRequests();
		Expect(Rk.size() == 1 && Rk[0].PayOption == 1, "desk: otherwise the player's own money, never Lou's offer by default");
		// A stake the player already shook on (an accepted deal) is theirs: the desk starts on it, as GoToLive would, so
		// registering by default never calls it off.
		life::Funding Shook = Lou;
		Shook.Ref = "deal:lou:" + Nightly.Id;
		Dk.Open[0].Pay = {{Roll, ""}, {Shook, ""}, {life::Funding(), ""}};
		ui::DeskScreen Ac;
		Ac.Open(Now, "desk");
		DeskFrame(Ac, Dk, 1920.0f, Now + 1.0);
		Ac.Key("A", Now + 1.0);
		Rk = Ac.TakeRequests();
		Expect(Rk.size() == 1 && Rk[0].PayOption == 1 && Rk[0].Fund.Ref == Shook.Ref, "desk: a stake the player accepted is the default");
		// More ways to pay than rows (Lou's offer, two sponsors' covers, the roll, the player's own money): four show at a
		// time and the window follows the choice, so the row the player steps onto is drawn, and that's what registers.
		life::Funding Penny = Cover;
		Penny.Who = "Lucky Penny";
		Dk.Open[0].Pay = {{Lou, ""}, {Penny, "Lucky Penny doesn't cover the Nightly."}, {Cover, "Team Embercrest's covered entries are used this month."}, {Roll, ""},
			{life::Funding(), ""}};
		ui::DeskScreen Fv;
		Fv.Open(Now, "desk");
		const bool RollFirst = Says(DeskFrame(Fv, Dk, 1920.0f, Now + 1.0), "FROM THE PACKAGE'S ROLL");
		Fv.Key("Right", Now + 1.05);
		const ui::DrawList Lf = DeskFrame(Fv, Dk, 1920.0f, Now + 1.1);
		Fv.Key("Enter", Now + 1.15);
		Rk = Fv.TakeRequests();
		Expect(RollFirst && Says(Lf, "YOUR MONEY") && !Says(Lf, "LOU'LL PUT YOU IN") && Rk.size() == 1 && Rk[0].PayOption == 4 && Rk[0].Fund.From == life::Paid::Own,
			"desk: past four ways to pay, the rows follow the choice (the fifth is drawn when chosen)");
		Fits(Lf, 1920.0f, "desk with five ways to pay");
	}
	// Meet the regulars: the appearance whose window is open, on F (X on the pad); a closed or done one isn't offered.
	{
		sponsor::Window Gone;
		Gone.Venue = "luckypenny";
		Gone.Label = "Lucky Penny: an hour behind the counter";
		Gone.From = D.World - 300.0;
		Gone.To = D.World - 240.0;
		sponsor::Window Meet;
		Meet.Venue = "embercrest";
		Meet.Label = "Meet the regulars (20 min)";
		Meet.From = D.World - 10.0;
		Meet.To = D.World + 50.0;
		ui::DeskInfo Dm = D;
		Dm.Entry = nullptr;
		Dm.Duties = {Gone, Meet};
		ui::DeskScreen Ms;
		Ms.Open(Now, "desk");
		const ui::DrawList Lm = DeskFrame(Ms, Dm, 1920.0f, Now + 1.0);
		Ms.Key("F", Now + 1.0);
		std::vector<ui::DeskRequest> Rm = Ms.TakeRequests();
		Expect(Says(Lm, "Meet the regulars (20 min)") && !Says(Lm, "behind the counter") && Rm.size() == 1 && Rm[0].K == ui::DeskRequest::Appear && Rm[0].Id == "embercrest",
			"desk: F answers the appearance that's open now");
		Dm.Duties = {Gone};
		DeskFrame(Ms, Dm, 1920.0f, Now + 1.1);
		Ms.Key("X", Now + 1.1);
		Expect(Ms.TakeRequests().empty(), "desk: no appearance offered, none answered");
	}
	S.Key("Escape", Now + 2.2);
	R = S.TakeRequests();
	Expect(R.size() == 1 && R[0].K == ui::DeskRequest::Leave && !S.IsOpen(), "desk: Escape steps away");
	// The cage: the receipt, the split, the W-2G note over $5,000, COLLECT once.
	life::LiveEntry Won;
	Won.Id = "embercrest@7";
	Won.Name = "Embercrest Sunday $150";
	Won.State = life::LiveEntry::Finished;
	Won.PaidCents = 15000;
	ui::DeskInfo Cage;
	Cage.Entry = &Won;
	Cage.Place = 1;
	Cage.Entrants = 114;
	Cage.PrizeCents = 573600;
	Cage.FinishedAt = 7.0 * 1440.0 + 23.0 * 60.0 + 51.0;
	Cage.ClerkLine = "Congratulations. Count it with me.";
	life::LedgerEntry Share;
	Share.Label = "Big Lou: his share, Embercrest Sunday $150";
	Share.Amount = -294300; // the $150 back first, then half of the $5,586 past it (Stake.cpp OnSettle)
	Share.Kind = 10;
	life::LedgerEntry Bonus;
	Bonus.Label = "Team Embercrest: the title bonus";
	Bonus.Amount = 5000;
	Bonus.Kind = 11;
	Cage.Receipt = {Share, Bonus};
	for (const float Width : Widths)
	{
		ui::DeskScreen Cs;
		Cs.Open(Now, "cage");
		const ui::DrawList L = DeskFrame(Cs, Cage, Width, Now + 2.0);
		const std::string What = "cage at " + W(Width);
		for (const char* N : {"THE CAGE", "Embercrest Sunday $150", "1st of 114", "11:51 PM", "$5,736.00", "Big Lou: his share, Embercrest Sunday $150", "-$2,943.00", "$50.00", "YOURS", "$2,843.00",
				 "W-2G", "COLLECT $2,843"})
		{
			Expect(Says(L, N), (What + ": says \"" + N + "\"").c_str());
		}
		Fits(L, Width, What);
		Readable(L, 13.0f, What);
		if (Width == Widths[0])
		{
			WriteList("cage_receipt_split", L);
		}
	}
	ui::DeskScreen Cs;
	Cs.Open(Now, "cage");
	DeskFrame(Cs, Cage, 1920.0f, Now + 1.0);
	Cs.Key("Enter", Now + 1.0);
	Cs.Key("Enter", Now + 1.2);
	R = Cs.TakeRequests();
	Expect(R.size() == 1 && R[0].K == ui::DeskRequest::Collect && R[0].Id == Won.Id, "cage: COLLECT, once");
	// Still counted out a minute later: the button doesn't offer the money again.
	{
		const ui::DrawList Later = DeskFrame(Cs, Cage, 1920.0f, Now + 61.0);
		Cs.Key("Enter", Now + 61.0);
		Expect(Says(Later, "COUNTED OUT") && !Says(Later, "COLLECT $") && Cs.TakeRequests().empty(), "cage: counted out stays counted out");
	}
	// The host may hand over every line the entry wrote: the prize line is the PRIZE row, counted once.
	{
		life::LedgerEntry Prize;
		Prize.Label = "Embercrest Sunday $150: 1st of 114";
		Prize.Amount = 573600;
		Prize.Kind = 4;
		ui::DeskInfo Full = Cage;
		Full.Receipt = {Prize, Share, Bonus};
		ui::DeskScreen Cf;
		Cf.Open(Now, "cage");
		const ui::DrawList L = DeskFrame(Cf, Full, 1920.0f, Now + 2.0);
		Expect(Says(L, "COLLECT $2,843") && !Says(L, "$8,579") && !Says(L, "Embercrest Sunday $150: 1st of 114"), "cage: the prize isn't counted twice");
		Fits(L, 1920.0f, "cage with the prize line");
	}
}

// ------------------------------------------------------------------ the street's additions (captions, the duty chip)

void StreetDuty()
{
	ss::life::State Life;
	ss::ui::StreetHudInfo Info;
	Info.Place = "FIFTH STREET";
	Info.Clock = "1:20 PM";
	Info.World = 3.0 * 1440.0 + 13.0 * 60.0 + 20.0;
	Info.BankrollCents = 41260;
	Info.Life = &Life;
	Info.Due.Label = "Lucky Penny: an hour behind the counter";
	Info.Due.From = 3.0 * 1440.0 + 14.0 * 60.0;
	Info.Due.To = Info.Due.From + 60.0;
	for (const float Width : Widths)
	{
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, Measure, Width, 1080.0f, 1.0f);
		ss::ui::DrawStreetHud(C, Info, Now);
		Expect(Says(L, "LUCKY PENNY: AN HOUR BEHIND THE COUNTER \xC2\xB7 IN 40 MIN"), ("street: the duty chip counts down at " + W(Width)).c_str());
		Fits(L, Width, "street duty at " + W(Width));
		if (Width == Widths[0])
		{
			WriteList("street_hud_duty", L);
		}
	}
	Info.World = Info.Due.From + 35.0;
	{
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, Measure, 1920.0f, 1080.0f, 1.0f);
		ss::ui::DrawStreetHud(C, Info, Now);
		Expect(Says(L, "NOW \xC2\xB7 25 MIN LEFT"), "street: the duty chip during its window");
	}
	Info.World = Info.Due.To + 1.0;
	{
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, Measure, 1920.0f, 1080.0f, 1.0f);
		ss::ui::DrawStreetHud(C, Info, Now);
		Expect(!Says(L, "LUCKY PENNY"), "street: no chip once the window has closed");
	}
	// The street's subtitles go through the captions now: two boxes, the newest at the bottom.
	Info.Subtitles = {{"Marlene", "Coffee, hon?", Now - 2.0}, {"Sal", "Dee's got a seat open.", Now - 1.0}, {"Lonnie", "You hear about the game upstairs?", Now - 0.5}};
	ss::ui::DrawList L;
	ss::ui::Canvas C(L, Measure, 1920.0f, 1080.0f, 1.0f);
	ss::ui::DrawStreetHud(C, Info, Now);
	Expect(!Says(L, "Coffee, hon?") && Says(L, "Dee's got a seat open.") && Says(L, "You hear about the game upstairs?") && TextAt(L, "upstairs") > TextAt(L, "seat open"),
		"street: two caption boxes, the newest at the bottom");
	// With the player's caption settings: off shows none; Everything at 150% still keeps the street's two.
	{
		ss::ui::StreetHudInfo Off = Info;
		Off.CaptionLook.Mode = 0;
		ss::ui::DrawList Lo;
		ss::ui::Canvas Co(Lo, Measure, 1920.0f, 1080.0f, 1.0f);
		ss::ui::DrawStreetHud(Co, Off, Now);
		ss::ui::StreetHudInfo Big = Info;
		Big.CaptionLook.Mode = 2;
		Big.CaptionLook.Size = 150;
		Big.CaptionLook.MaxShown = 3;
		ss::ui::DrawList Lb;
		ss::ui::Canvas Cb(Lb, Measure, 1920.0f, 1080.0f, 1.0f);
		ss::ui::DrawStreetHud(Cb, Big, Now);
		Expect(!Says(Lo, "seat open") && !Says(Lo, "upstairs") && Says(Lo, "FIFTH STREET"), "street: captions off, no subtitles (the rest of the HUD as ever)");
		Expect(!Says(Lb, "Coffee, hon?") && Says(Lb, "Dee's got a seat open.") && Says(Lb, "You hear about the game upstairs?"), "street: the player's caption size, still two boxes");
		Fits(Lb, 1920.0f, "street captions at 150%");
	}
}

// ------------------------------------------------------------------ the CONTINUE card's marks (ui-laptop's patch)

void ContinueMarks()
{
	// What the career has earned in public (next_segment_spec §5.6) under the bankroll on the CONTINUE card: the club's
	// crest with its word, and the network's one badge. A career with neither has no EARNED row.
	ss::badge::Badge Ember;
	Ember.K = ss::badge::Kind::Club;
	Ember.Tier = 2;
	for (const int Tier : {0, 2})
	{
		SettingsHooks H;
		ss::ui::FrontEnd Fe(H);
		Fe.Info.HasSave = true;
		Fe.Info.HeroName = "grinder_3c";
		Fe.Info.Person.Created = true;
		Fe.Info.BankrollCents = 41260;
		Fe.Info.Tournaments = 12;
		Fe.Info.ClubTier = Tier;
		double At = Now;
		Fe.Open(ss::ui::FrontEnd::Page::Attract, At);
		At += 3.0;
		Fe.Key("Any", At);
		ss::ui::DrawList L;
		for (int K = 0; K < 45; ++K)
		{
			L.Clear();
			ss::ui::Canvas C(L, Measure, 1920.0f, ss::ui::FrontEnd::Height, 1.0f);
			At += 1.0 / 30.0;
			Fe.Draw(C, At);
		}
		const bool Row = Says(L, "EARNED") && Says(L, ss::badge::Label(Ember));
		Expect(Says(L, "BANKROLL") && (Tier > 0 ? Row : !Says(L, "EARNED")),
			Tier > 0 ? "continue: the card shows the club's crest the career earned" : "continue: no EARNED row before anything's earned");
		Fits(L, 1920.0f, "the CONTINUE card");
	}
}

void Copy()
{
	int Problems = 0;
	for (const std::pair<std::string, std::string>& Line : ss::ui::TableUiCopy())
	{
		Problems += copylint::Run("table ui", {Line}, Line.first.rfind("controls.", 0) == 0 ? copylint::TableTalk : copylint::LetterBody);
	}
	Expect(Problems == 0, "copy: the table's sentences pass the lint");
	Expect(ss::ui::TableUiCopy().size() > 30, "copy: the inventory holds the settings' help and the empty states");
}
} // namespace tableui_seg

/** SS_UI_BENCH=1: the table HUD's draw time (budget §10: 0.3 ms with its submit). */
void Bench()
{
	if (!std::getenv("SS_UI_BENCH"))
	{
		return;
	}
	auto Time = [](const char* What, const ss::ui::TableHudInfo& I) {
		// As the host draws: one list, cleared each frame (its buffers kept).
		ss::ui::DrawList L;
		const auto T0 = std::chrono::steady_clock::now();
		for (int K = 0; K < 200; ++K)
		{
			L.Clear();
			ss::ui::Canvas C(L, tableui_seg::Measure, 1920.0f, 1080.0f, 1.0f);
			ss::ui::DrawTableHud(C, I, tableui_seg::Now + K * 0.016);
		}
		const size_t Verts = L.Vertices.size();
		std::printf("UI BENCH %-28s %6.3f ms/draw %6zu vertices\n", What, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count() / 200.0, Verts);
	};
	tableui_seg::Scene S;
	tableui_seg::TurnOpen(S, 1920.0f);
	Time("table_hud_turn_open", S.I);
	ss::ui::TableHudInfo Part = S.I;
	Part.Plates.clear();
	Time("  without the plates", Part);
	Part = S.I;
	Part.Clock = nullptr;
	Time("  without the clock card", Part);
	Part = S.I;
	Part.YourTurn = false;
	Time("  without the ladder", Part);
	Part = S.I;
	Part.Captions.clear();
	Time("  without the captions", Part);
	Part = S.I;
	Part.Trace.clear();
	Time("  without the heart's trace", Part);
}

void SegTableUi()
{
	Bench();
	tableui_seg::HudGoldens();
	tableui_seg::Plates();
	tableui_seg::CardPages();
	tableui_seg::RoomClocks();
	tableui_seg::ControlParity();
	tableui_seg::SettingsMigration();
	tableui_seg::CaptionRules();
	tableui_seg::DeskAndCage();
	tableui_seg::StreetDuty();
	tableui_seg::ContinueMarks();
	tableui_seg::Copy();
}
} // namespace ui_test
