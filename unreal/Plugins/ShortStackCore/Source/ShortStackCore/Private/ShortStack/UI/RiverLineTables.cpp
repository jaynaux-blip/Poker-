// RiverLine multi-tabling: the table tabs in the top bar, the tile view (every open table at once, each with its own
// buttons) and the toasts for tournaments that end while others play on.
#include "ShortStack/UI/RiverLine.h"
#include "../StrictFloat.h"
#include "RiverLineShared.h"

#include "ShortStack/Evaluator.h"
#include "ShortStack/Game/Chat.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/UI/Avatars.h"

#include <algorithm>
#include <cmath>

namespace ss
{
namespace ui
{
namespace rltables_detail
{
const float TilesTop = 72.0f;
const float TilesBottom = RiverLine::Height - 46.0f; // above the taskbar
const float TileGap = 8.0f;

Color ClockColor(double SecondsLeft)
{
	return SecondsLeft < 5.0 ? pal::Red : pal::Orange;
}

float Pulse(double Now, double Rate)
{
	return 0.5f + 0.5f * static_cast<float>(std::sin(Now * Rate));
}

/** A countdown ring: the share of the clock left, from twelve o'clock. */
void ClockRing(Canvas& Cv, float Cx, float Cy, float R, float Frac, const Color& Col, float Width)
{
	Cv.StrokeArc(Cx, Cy, R, 0.0f, 2.0f * Pi, Color{Col.R, Col.G, Col.B, 0.2f}, Width);
	if (Frac > 0.002f)
	{
		Cv.StrokeArc(Cx, Cy, R, -Pi / 2.0f, -Pi / 2.0f + 2.0f * Pi * std::min(1.0f, Frac), Col, Width, true);
	}
}

std::string SecondsLabel(double Seconds)
{
	return std::to_string(static_cast<int>(std::ceil(std::max(0.0, Seconds)))) + "s";
}

std::string BbLabel(double Bb)
{
	return Fixed(Bb, Bb < 10.0 ? 1 : 0) + " BB";
}
} // namespace rltables_detail

using namespace rltables_detail;
using namespace rlnet_detail;

// ------------------------------------------------------------------ table tabs

void RiverLine::TableStrip(float X, double Now)
{
	const int Count = S.TableCount();
	const bool AtTables = S.CurrentScreen == Screen::Table;
	if (Count == 0 || (!AtTables && S.CurrentScreen != Screen::Lobby))
	{
		return;
	}
	const std::string Bal = "Balance " + Money(S.BankrollCents);
	// Room for the balance (and Kast's LIVE pill beside it while streaming).
	const float Pill = S.Streaming() ? UI.Measure(std::to_string(static_cast<long long>(std::round(S.Stream.Viewers))), 13.0f, 900, true) + 80.0f + 24.0f : 0.0f;
	const float Right = Width - 140.0f - (UI.Measure(Bal, 17.0f, 700) + 28.0f) - 14.0f - Pill;
	const bool CanAdd = AtTables && Count < S.MaxTables();
	const bool CanTile = AtTables && Count >= 2;
	float Px = X + 2.0f;
	std::string Hint;
	float HintX = 0.0f;

	if (Count == 1 && AtTables)
	{
		// One table: the live pill, as it always was.
		const std::string Name = S.T ? S.T->Spec.Name : std::string();
		const float W = std::min(UI.Measure(Name, 14.0f, 700) + 64.0f, Right - Px - 46.0f);
		UI.RRect({Px, 17.0f, W, 30.0f}, 15.0f, Rgba(239, 77, 90, 0.12f), NetA(pal::Red, 0.5f));
		C->FillCircle(Px + 16.0f, 32.0f, 4.5f, NetA(pal::Red, 0.6f + 0.4f * Nf(std::sin(Now * 4.0))));
		UI.Text("LIVE", Px + 26.0f, 36.5f, Ts(11.0f, 800, pal::Red));
		UI.Text(Name, Px + 58.0f, 37.0f, Ts(14.0f, 700, pal::Ink, Align::Left, Baseline::Alphabetic, false, W - 66.0f));
		Px += W + 8.0f;
	}
	else
	{
		const float Extras = (CanAdd ? 40.0f : 0.0f) + (CanTile ? 40.0f : 0.0f);
		const float TabW = std::min(176.0f, (Right - Px - Extras) / Nf(Count) - 6.0f);
		const int Front = S.FocusedTable();
		for (int I = 0; I < Count; ++I)
		{
			const TableGlance G = S.Glance(I);
			const Rect R{Px, 12.0f, TabW, 40.0f};
			const Ui::ClickState St = UI.Clickable("tab" + std::to_string(I), R);
			if (St.Clicked)
			{
				S.ShowTables();
				S.FocusTable(I);
			}
			const bool InFront = AtTables && I == Front;
			const double Remain = G.Deadline - Now;
			const Color Turn = ClockColor(Remain);
			if (G.YourTurn)
			{
				C->GlowRoundRect(R, 10.0f, NetA(Turn, 0.2f + 0.25f * Pulse(Now, 6.0)), 12.0f);
			}
			const Color Fill = G.YourTurn ? Mix(Hex(0x1a1410), Turn, 0.18f + 0.1f * Pulse(Now, 6.0)) : InFront ? Hex(0x15304a) : St.Hover ? Hex(0x132339) : Hex(0x0e1a2c);
			const Color Edge = G.YourTurn ? Turn : InFront ? NetA(pal::Accent, 0.8f) : pal::Line;
			UI.RRect(R, 10.0f, Fill, Edge, G.YourTurn || InFront ? 1.5f : 1.0f);
			const float Cx = R.X + 18.0f;
			const float Cy = R.Y + 20.0f;
			if (G.YourTurn)
			{
				const double Span = std::max(1.0, G.Deadline - G.DeadlineStart);
				ClockRing(*C, Cx, Cy, 8.5f, static_cast<float>(Remain / Span), Turn, 3.0f);
			}
			else
			{
				const Color Dot = G.Busted ? pal::Dim : G.Sprinting ? pal::Accent2 : G.AllIn ? pal::Gold : pal::Green;
				C->FillCircle(Cx, Cy, 4.5f, Dot);
				if (!G.Busted)
				{
					C->StrokeEllipse(Cx, Cy, 7.0f + 2.0f * Pulse(Now + Nf(I), 2.0), 7.0f + 2.0f * Pulse(Now + Nf(I), 2.0), NetA(Dot, 0.35f), 1.0f);
				}
			}
			UI.Text(NetShortName(G.Name), R.X + 33.0f, R.Y + 17.0f, Ts(12.5f, 700, pal::Ink, Align::Left, Baseline::Alphabetic, false, R.W - 40.0f));
			std::string Line;
			Color LineCol = pal::Muted;
			if (G.YourTurn)
			{
				Line = SecondsLabel(Remain) + " to act";
				LineCol = Turn;
			}
			else if (G.Busted)
			{
				Line = "Finishing\xE2\x80\xA6";
			}
			else if (G.Sprinting)
			{
				Line = "Sprinting \xC2\xB7 " + BbLabel(G.StackBb);
				LineCol = pal::Accent2;
			}
			else
			{
				Line = BbLabel(G.StackBb) + " \xC2\xB7 " + Grouped(G.Rank) + "/" + Grouped(G.Remaining);
				LineCol = G.InMoney ? pal::Gold : pal::Muted;
			}
			UI.Text(Line, R.X + 33.0f, R.Y + 32.0f, Ts(11.0f, 600, LineCol, Align::Left, Baseline::Alphabetic, true, R.W - 40.0f));
			if (InFront && !S.Tiled)
			{
				UI.RRect({R.X + 10.0f, R.Y + R.H - 3.0f, R.W - 20.0f, 3.0f}, 1.5f, pal::Accent);
			}
			Px += TabW + 6.0f;
		}
	}

	// Add a table, and switch between one table in front and all of them tiled.
	auto IconButton = [&](const std::string& Id, const std::string& Label, bool Lit) {
		const Rect R{Px, 14.0f, 36.0f, 36.0f};
		const Ui::ClickState St = UI.Clickable(Id, R);
		UI.RRect(R, 18.0f, Lit ? Hex(0x16354a) : St.Hover ? Hex(0x172a42) : Hex(0x0e1a2c), Lit ? pal::Accent : pal::Line);
		if (St.Hover)
		{
			Hint = Label;
			HintX = R.X + R.W / 2.0f;
		}
		Px += 40.0f;
		return St.Clicked;
	};
	if (CanAdd)
	{
		const float Bx = Px + 18.0f;
		if (IconButton("addtable", "Add a table (" + std::to_string(Count) + " of " + std::to_string(S.MaxTables()) + ")", false))
		{
			S.ShowLobby();
			OpenPage(Page::Lobby, Now);
		}
		C->FillRect({Bx - 7.0f, 31.0f, 14.0f, 2.2f}, pal::Ink);
		C->FillRect({Bx - 1.1f, 25.0f, 2.2f, 14.0f}, pal::Ink);
	}
	if (CanTile)
	{
		const float Bx = Px + 18.0f;
		if (IconButton("tileview", S.Tiled ? "One table at a time" : "Tile all tables", S.Tiled))
		{
			S.Tiled = !S.Tiled;
		}
		const Color Ic = S.Tiled ? pal::Accent : pal::Ink;
		for (int K = 0; K < 4; ++K)
		{
			C->FillRoundRect({Bx - 8.0f + Nf(K % 2) * 9.0f, 24.0f + Nf(K / 2) * 9.0f, 7.0f, 7.0f}, 1.5f, Ic);
		}
	}
	if (!Hint.empty())
	{
		const float Hw = UI.Measure(Hint, 12.0f, 700) + 20.0f;
		const float Hx = std::min(Width - Hw - 8.0f, HintX - Hw / 2.0f);
		UI.RRect({Hx, 58.0f, Hw, 24.0f}, 6.0f, Hex(0x0b1422), pal::Line);
		UI.Text(Hint, Hx + Hw / 2.0f, 74.5f, Ts(12.0f, 700, pal::Ink, Align::Center));
	}
}

// ------------------------------------------------------------------ tile view

void RiverLine::Tiles(double Now)
{
	const int Count = S.TableCount();
	const int Front = S.FocusedTable();
	const float TileW = (Width - 16.0f - TileGap) / 2.0f;
	const float Avail = TilesBottom - TilesTop;
	const float TileH = Count <= 2 ? 446.0f : (Avail - TileGap) / 2.0f;
	const float Y0 = Count <= 2 ? TilesTop + (Avail - TileH) / 2.0f : TilesTop;
	if (Count <= 2)
	{
		const int Waiting = S.TablesWaiting();
		UI.Text("TILE VIEW", 12.0f, Y0 - 16.0f, Ts(11.0f, 800, pal::Muted));
		UI.Text(std::to_string(Count) + " tables" + (Waiting > 0 ? " \xC2\xB7 your turn at " + std::to_string(Waiting) : std::string()), 86.0f, Y0 - 16.0f,
			Ts(12.0f, 600, Waiting > 0 ? pal::Orange : pal::Dim));
	}
	const int Slots = Count <= 2 ? 2 : 4;
	TileClicked = -1;
	for (int I = 0; I < Slots; ++I)
	{
		const Rect R{8.0f + Nf(I % 2) * (TileW + TileGap), Y0 + Nf(I / 2) * (TileH + TileGap), TileW, TileH};
		if (I < Count)
		{
			S.WithTable(I, [&]() { Tile(I, R, I == Front, Now); });
			continue;
		}
		// An empty slot: one more table fits.
		if (Count < S.MaxTables())
		{
			const Ui::ClickState St = UI.Clickable("tileadd" + std::to_string(I), R);
			UI.RRect(R, 14.0f, St.Hover ? Hex(0x0f1c2f) : Hex(0x0b1523), NetA(pal::Line, St.Hover ? 1.0f : 0.6f));
			C->StrokeRoundRect({R.X + 10.0f, R.Y + 10.0f, R.W - 20.0f, R.H - 20.0f}, 10.0f, NetA(pal::Muted, 0.2f), 1.5f);
			const float Cx = R.X + R.W / 2.0f;
			const float Cy = R.Y + R.H / 2.0f - 14.0f;
			C->FillCircle(Cx, Cy, 26.0f, St.Hover ? Hex(0x16354a) : Hex(0x111f33));
			C->FillRect({Cx - 10.0f, Cy - 1.5f, 20.0f, 3.0f}, pal::Accent);
			C->FillRect({Cx - 1.5f, Cy - 10.0f, 3.0f, 20.0f}, pal::Accent);
			UI.Text("Add a table", Cx, Cy + 52.0f, Ts(16.0f, 800, pal::Ink, Align::Center));
			UI.Text("Pick another tournament in the lobby", Cx, Cy + 74.0f, Ts(13.0f, 500, pal::Muted, Align::Center));
			if (St.Clicked)
			{
				S.ShowLobby();
				OpenPage(Page::Lobby, Now);
			}
		}
	}
	if (TileClicked >= 0)
	{
		S.FocusTable(TileClicked);
	}
}

void RiverLine::Tile(int Index, const Rect& R, bool Front, double Now)
{
	if (!S.T)
	{
		return;
	}
	const Tournament& Tn = *S.T;
	const std::string Id = "t" + std::to_string(Index);
	const bool Turn = S.HasPrompt;
	const double ClockEnd = S.Prompt.TimeBankUntil > 0.0 ? S.Prompt.TimeBankUntil : S.Prompt.Deadline;
	const double Remain = ClockEnd - Now;
	const Color TurnCol = ClockColor(Remain);

	// Frame. A press anywhere on the tile brings it forward (the keyboard acts on the table in front).
	if (UI.Clickable(Id + "bg", R).Clicked)
	{
		TileClicked = Index;
	}
	if (Turn)
	{
		C->GlowRoundRect(R, 14.0f, NetA(TurnCol, 0.22f + 0.22f * Pulse(Now, 6.0)), 16.0f);
	}
	UI.RRect(R, 14.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, Hex(0x0f1a2b), Hex(0x0a121e)), Turn ? TurnCol : Front ? NetA(pal::Accent, 0.75f) : pal::Line, Turn || Front ? 2.0f : 1.0f);

	// Header: the event, the blinds, where the player stands.
	const Level& Lv = Tn.CurrentLevel();
	const float Hy = R.Y + 23.0f;
	const float NameW = UI.Text(NetShortName(Tn.Spec.Name), R.X + 14.0f, Hy, Ts(14.0f, 800, pal::Ink, Align::Left, Baseline::Alphabetic, false, R.W * 0.42f));
	const float LvW = UI.Text("Lvl " + std::to_string(Tn.LevelIndex + 1) + " \xC2\xB7 " + ChipsText(static_cast<double>(Lv.Sb)) + "/" + ChipsText(static_cast<double>(Lv.Bb)), R.X + 26.0f + NameW, Hy,
		Ts(12.0f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, true));
	if (Front)
	{
		// The keyboard's table: F, C and R act here.
		NetPill(*C, "KEYS F \xC2\xB7 C \xC2\xB7 R", R.X + 38.0f + NameW + LvW, Hy - 13.0f, pal::Accent, false, 9.0f);
	}
	float Rx = R.X + R.W - 14.0f;
	if (S.TimeBank >= 1.0)
	{
		Rx -= UI.Text("TB " + SecondsLabel(S.TimeBank), Rx, Hy, Ts(11.5f, 700, pal::Dim, Align::Right, Baseline::Alphabetic, true)) + 12.0f;
	}
	const TPlayer& Me = Tn.Hero();
	Rx -= UI.Text(Grouped(Me.Busted ? Me.Place : Tn.HeroRank()) + " / " + Grouped(Tn.Remaining), Rx, Hy, Ts(12.5f, 700, pal::Ink, Align::Right, Baseline::Alphabetic, true)) + 8.0f;
	if (Tn.InTheMoney())
	{
		const float Pw = C->Measure("ITM", 9.0f, 700) + 9.0f * 1.3f;
		NetPill(*C, "ITM", Rx - Pw, Hy - 13.0f, pal::Gold, true, 9.0f);
	}
	C->FillRect({R.X + 1.0f, R.Y + 34.0f, R.W - 2.0f, 1.0f}, NetA(pal::Line, 0.7f));

	// Felt.
	const float Top = R.Y + 34.0f;
	const float Bottom = R.Y + R.H - 56.0f;
	const float Cx = R.X + R.W / 2.0f;
	const float Cy = (Top + Bottom) / 2.0f + 6.0f;
	const float Ex = R.W * 0.31f;
	const float Ey = (Bottom - Top) * 0.29f;
	C->FillEllipse(Cx, Cy + 6.0f, Ex + 20.0f, Ey + 20.0f, Rgba(0, 0, 0, 0.45f));
	C->FillEllipse(Cx, Cy, Ex + 16.0f, Ey + 16.0f, Paint::Linear({0.0f, Cy - Ey}, {0.0f, Cy + Ey}, Hex(0x2a3342), Hex(0x10151d)));
	C->FillEllipse(Cx, Cy, Ex, Ey, Paint::Radial({Cx, Cy - Ey * 0.3f}, 10.0f, {Cx, Cy}, Ex, Hex(0x15875f), 0.7f, pal::Felt, pal::FeltDark));
	C->StrokeEllipse(Cx, Cy, Ex - 20.0f, Ey - 16.0f, Rgba(255, 255, 255, 0.07f), 1.5f);

	const int Seats = std::max(2, static_cast<int>(S.Seats.size()));
	const int HeroNo = HeroSeatNo();
	auto SeatAt = [&](int Seat) {
		const int Slot = (Seat - HeroNo + Seats) % Seats;
		const float A = (90.0f + Nf(Slot) * 360.0f / Nf(Seats)) * Pi / 180.0f;
		return Vec2{Cx + (Ex + 62.0f) * std::cos(A), Cy + (Ey + 40.0f) * std::sin(A)};
	};
	auto BetAt = [&](int Seat) {
		const Vec2 P = SeatAt(Seat);
		return Vec2{Cx + (P.X - Cx) * 0.62f, Cy + (P.Y - Cy) * (P.Y < Cy ? 0.62f : 0.36f)};
	};

	// Pot and board.
	if (S.PotChips > 0)
	{
		const std::string Pot = "Pot " + ChipsText(static_cast<double>(S.PotChips));
		const float Pw = UI.Measure(Pot, 12.5f, 700) + 20.0f;
		UI.RRect({Cx - Pw / 2.0f, Cy - 60.0f, Pw, 21.0f}, 10.5f, Rgba(0, 0, 0, 0.45f));
		UI.Text(Pot, Cx, Cy - 45.0f, Ts(12.5f, 700, pal::Ink, Align::Center));
	}
	const float Bw = 38.0f;
	const float Bh = 53.0f;
	const float Bx0 = Cx - (5.0f * Bw + 4.0f * 5.0f) / 2.0f;
	for (size_t I = 0; I < S.Board.size(); ++I)
	{
		const double At = I < S.BoardShownAt.size() ? S.BoardShownAt[I] : 0.0;
		if (Now < At)
		{
			continue;
		}
		CardOpts O;
		O.Flip = static_cast<float>(std::min(1.0, (Now - At) / 0.25));
		DrawCard(*C, S.Board[I], Bx0 + Nf(I) * (Bw + 5.0f), Cy - 34.0f, Bw, Bh, O);
	}

	// Dealer button and bets.
	{
		const Vec2 Bp = SeatAt(S.ButtonSeat);
		const float Dx = Cx + (Bp.X - Cx) * 0.8f + 24.0f;
		const float Dy = Cy + (Bp.Y - Cy) * 0.78f;
		C->FillCircle(Dx, Dy, 8.0f, Hex(0xf4f4f4));
		UI.Text("D", Dx, Dy + 0.5f, Ts(10.0f, 800, Hex(0x111111), Align::Center, Baseline::Middle));
	}
	for (const SeatVis& Sv : S.Seats)
	{
		if (!Sv.Present || Sv.Bet <= 0)
		{
			continue;
		}
		const Vec2 B = BetAt(Sv.Seat);
		const std::string Amt = ChipsText(static_cast<double>(Sv.Bet));
		const float Aw = UI.Measure(Amt, 11.5f, 700, true);
		const float X0 = B.X - (Aw + 14.0f) / 2.0f;
		C->FillCircle(X0 + 5.0f, B.Y - 4.0f, 5.5f, Hex(0xc8323c));
		C->StrokeEllipse(X0 + 5.0f, B.Y - 4.0f, 3.5f, 3.5f, Rgba(255, 255, 255, 0.85f), 1.2f, 2.0f);
		UI.Text(Amt, X0 + 14.0f, B.Y, Ts(11.5f, 700, Hex(0xffffff), Align::Left, Baseline::Alphabetic, true));
	}

	// Seats.
	const bool HandOver = S.CurHand && S.CurHand->bComplete;
	const SeatVis* HeroVis = nullptr;
	for (const SeatVis& Sv : S.Seats)
	{
		const Vec2 P = SeatAt(Sv.Seat);
		const Rect Plate{P.X - 58.0f, P.Y - 17.0f, 116.0f, 34.0f};
		if (!Sv.Present)
		{
			UI.RRect(Plate, 9.0f, Rgba(255, 255, 255, 0.025f), Rgba(255, 255, 255, 0.06f));
			continue;
		}
		HeroVis = Sv.IsHero ? &Sv : HeroVis;
		const bool Dim = Sv.Folded && !Sv.Winner;
		// Hole cards toward the middle of the table: below the plate for the seats along the top.
		if (!Sv.IsHero && Sv.HasCards && !Sv.Folded)
		{
			const bool Below = P.Y < Cy - 20.0f;
			const bool Faces = Sv.Hole.size() == 2;
			const float Cw = Faces ? 26.0f : 17.0f;
			const float Ch = Faces ? 36.0f : 24.0f;
			const float Yc = Below ? Plate.Y + Plate.H - 6.0f : Plate.Y - Ch + 6.0f;
			for (int K = 0; K < 2; ++K)
			{
				CardOpts O;
				O.Rotate = Faces ? 0.0f : (Nf(K) - 0.5f) * 0.18f;
				O.Highlight = Sv.Winner && HandOver;
				DrawCard(*C, Faces ? Sv.Hole[static_cast<size_t>(K)] : -1, P.X + 18.0f + Nf(K) * (Faces ? Cw + 2.0f : 11.0f), Yc, Cw, Ch, O);
			}
		}
		const float A0 = C->GetAlpha();
		C->SetAlpha(A0 * (Dim ? 0.45f : 1.0f));
		if (Sv.Winner && HandOver)
		{
			C->GlowRoundRect(Plate, 9.0f, Rgba(242, 193, 78, 0.8f), 14.0f);
		}
		const Color Edge = Sv.Acting ? pal::Accent : Sv.IsRival ? Hex(0xb8324a) : Sv.Winner && HandOver ? pal::Gold : Rgba(255, 255, 255, 0.1f);
		UI.RRect(Plate, 9.0f, Paint::Linear({0.0f, Plate.Y}, {0.0f, Plate.Y + Plate.H}, Sv.IsHero ? Hex(0x1b3552) : Hex(0x1a2536), Sv.IsHero ? Hex(0x10223a) : Hex(0x0f1726)), Edge,
			Sv.Acting ? 1.5f : 1.0f);
		AvatarSpec Pic = AvatarFor(Sv.Name);
		if (Sv.IsHero)
		{
			Pic.Frame = AvatarFrame::Neon;
			Pic.Rim = 0x27d3c3;
		}
		else if (Sv.Pro)
		{
			Pic.Frame = AvatarFrame::Gold;
		}
		DrawAvatar(*C, Plate.X + 17.0f, P.Y, 11.5f, Pic);
		UI.Text(Sv.Name, Plate.X + 34.0f, P.Y - 2.0f, Ts(12.0f, 700, Sv.IsRival ? Hex(0xff8da0) : pal::Ink, Align::Left, Baseline::Alphabetic, false, Plate.W - 40.0f));
		std::string Under = Sv.Stack <= 0 && Sv.AllIn ? std::string("ALL-IN") : ChipsText(static_cast<double>(Sv.Stack));
		Color UnderCol = Sv.AllIn ? pal::Red : pal::Gold;
		if (Sv.Winner && HandOver && !Sv.HandLabel.empty())
		{
			Under = Sv.HandLabel;
			UnderCol = pal::Gold;
		}
		else if (!Sv.LastAction.empty() && Now - Sv.LastActionAt < 2.2 && !Sv.Folded)
		{
			Under = Sv.LastAction;
			UnderCol = pal::Accent;
		}
		UI.Text(Under, Plate.X + 34.0f, P.Y + 12.0f, Ts(11.0f, 700, UnderCol, Align::Left, Baseline::Alphabetic, true, Plate.W - 40.0f));
		if (Sv.Acting && Sv.ActEnd > Sv.ActStart)
		{
			const float Frac = static_cast<float>(Clamp01((Sv.ActEnd - Now) / (Sv.ActEnd - Sv.ActStart)));
			UI.RRect({Plate.X + 6.0f, Plate.Y + Plate.H + 3.0f, (Plate.W - 12.0f) * Frac, 3.0f}, 1.5f, Frac < 0.3f ? pal::Red : pal::Accent);
		}
		C->SetAlpha(A0);
	}

	// The player's cards, big, and what they make.
	if (HeroVis && HeroVis->HasCards && !HeroVis->Folded && HeroVis->Hole.size() == 2)
	{
		const Vec2 P = SeatAt(HeroVis->Seat);
		const float Cw = 44.0f;
		const float Ch = 62.0f;
		for (size_t K = 0; K < 2; ++K)
		{
			CardOpts O;
			O.Flip = static_cast<float>(Clamp01((Now - HeroVis->DealtAt - 0.3 - static_cast<double>(K) * 0.1) / 0.25));
			O.Rotate = (Nf(K) - 0.5f) * 0.06f;
			O.Highlight = HeroVis->Winner && HandOver;
			DrawCard(*C, HeroVis->Hole[K], P.X - Cw - 2.0f + Nf(K) * (Cw + 4.0f), P.Y - 17.0f - Ch + 4.0f, Cw, Ch, O);
		}
		std::vector<Card> Shown;
		for (size_t I = 0; I < S.Board.size(); ++I)
		{
			if (Now >= (I < S.BoardShownAt.size() ? S.BoardShownAt[I] : 0.0))
			{
				Shown.push_back(S.Board[I]);
			}
		}
		if (Shown.size() >= 3)
		{
			UI.Text(Describe(EvaluateHand(HeroVis->Hole, Shown)), P.X - Cw - 12.0f, P.Y - 40.0f, Ts(12.0f, 700, pal::Gold, Align::Right));
		}
	}
	else if (S.AutoFolded && Now - S.AutoFoldedAt < 1.2 && S.AutoFoldedCards.size() == 2 && HeroVis)
	{
		const Vec2 P = SeatAt(HeroVis->Seat);
		for (size_t K = 0; K < 2; ++K)
		{
			CardOpts O;
			O.Alpha = static_cast<float>(1.0 - (Now - S.AutoFoldedAt) / 1.2);
			O.Dim = true;
			DrawCard(*C, S.AutoFoldedCards[K], P.X - 46.0f + Nf(K) * 48.0f, P.Y - 75.0f, 44.0f, 62.0f, O);
		}
	}

	// Banners: the bubble, the money, the final table, the end.
	const Banner& Bn = S.CurrentBanner;
	if (Bn.Active || S.Moving)
	{
		const double Age = Bn.Active ? Now - Bn.At : Now - S.MovingAt;
		const float Fade = static_cast<float>(std::min(Clamp01(Age / 0.25), Clamp01((3.2 - Age) / 0.4)));
		const std::string Title = Bn.Active ? Bn.Title : "MOVING TO TABLE " + std::to_string(S.MovingTo);
		const std::string Sub = Bn.Active ? Bn.Sub : "Table " + std::to_string(S.MovingFrom) + " is breaking";
		const Color Bc = Bn.Active ? Hex(Bn.Color) : pal::Accent;
		const float A0 = C->GetAlpha();
		C->SetAlpha(A0 * Fade);
		const float Bw2 = std::max(UI.Measure(Title, 22.0f, 900), UI.Measure(Sub, 13.0f, 600)) + 48.0f;
		UI.RRect({Cx - Bw2 / 2.0f, Cy - 44.0f, Bw2, 74.0f}, 12.0f, Rgba(6, 10, 18, 0.88f), NetA(Bc, 0.8f), 1.5f);
		UI.Text(Title, Cx, Cy - 8.0f, Ts(22.0f, 900, Bc, Align::Center));
		UI.Text(Sub, Cx, Cy + 15.0f, Ts(13.0f, 600, pal::Ink, Align::Center));
		C->SetAlpha(A0);
	}

	// Footer: the player's buttons when it's their turn, else what the table is doing.
	const float Fy = R.Y + R.H - 50.0f;
	if (Turn)
	{
		const double Span = std::max(1.0, ClockEnd - (S.Prompt.TimeBankUntil > 0.0 ? S.Prompt.Deadline : S.Prompt.OpenedAt));
		const float Frac = static_cast<float>(Clamp01(Remain / Span));
		UI.RRect({R.X + 12.0f, Fy - 8.0f, R.W - 24.0f, 4.0f}, 2.0f, Hex(0x0b1422));
		UI.RRect({R.X + 12.0f, Fy - 8.0f, (R.W - 24.0f) * Frac, 4.0f}, 2.0f, TurnCol);
		if (S.Prompt.TimeBankUntil > 0.0)
		{
			UI.Text("TIME BANK", R.X + R.W - 14.0f, Fy - 12.0f, Ts(10.0f, 800, TurnCol, Align::Right));
		}
		const HeroPrompt P = S.Prompt;
		Chips HeroStack = 0;
		if (S.CurHand)
		{
			for (const HandSeat& Hs : S.CurHand->Seats)
			{
				HeroStack = Hs.Id == HeroId ? Hs.Stack : HeroStack;
			}
		}
		const float Gap = 8.0f;
		const int Buttons = P.CanRaise ? 4 : 2;
		const float Bw3 = (R.W - 24.0f - Gap * Nf(Buttons - 1)) / Nf(Buttons);
		auto Slot = [&](int K) { return Rect{R.X + 12.0f + Nf(K) * (Bw3 + Gap), Fy, Bw3, 40.0f}; };
		ButtonOpts O;
		O.Size = 15.0f;
		bool Acted = false;
		O.Kind = ButtonKind::Danger;
		if (UI.Button(Id + "fold", Slot(0), "Fold", O))
		{
			S.HeroAct(PlayerAction::Fold());
			Acted = true;
		}
		O.Kind = ButtonKind::Secondary;
		const std::string CallLabel = P.CanCheck ? std::string("Check") : P.ToCall >= HeroStack ? "Call all-in" : "Call " + ChipsText(static_cast<double>(P.ToCall));
		if (UI.Button(Id + "call", Slot(1), CallLabel, O) && !Acted)
		{
			S.HeroAct(P.CanCheck ? PlayerAction::Check() : PlayerAction::Call());
			Acted = true;
		}
		if (P.CanRaise)
		{
			O.Kind = ButtonKind::Primary;
			const bool Shove = P.RaiseTo >= P.MaxRaise;
			if (!Shove && UI.Button(Id + "raise", Slot(2), (P.IsBet ? "Bet " : "Raise ") + ChipsText(static_cast<double>(P.RaiseTo)), O) && !Acted)
			{
				S.HeroAct(PlayerAction::RaiseTo(static_cast<double>(P.RaiseTo)));
				Acted = true;
			}
			O.Kind = ButtonKind::Gold;
			if (UI.Button(Id + "allin", Shove ? Rect{Slot(2).X, Fy, Bw3 * 2.0f + Gap, 40.0f} : Slot(3), "All-in " + ChipsText(static_cast<double>(P.MaxRaise)), O) && !Acted)
			{
				S.HeroAct(PlayerAction::RaiseTo(static_cast<double>(P.MaxRaise)));
				Acted = true;
			}
		}
		if (Acted)
		{
			TileClicked = Index;
		}
	}
	else
	{
		std::string Status;
		if (S.Finishing())
		{
			Status = Me.PrizeCents > 0 ? "Finished " + Ordinal(Me.Busted ? Me.Place : 1) + " \xC2\xB7 " + Money(Me.PrizeCents) : "Eliminated \xC2\xB7 the table closes in a moment";
		}
		else if (S.Sprinting)
		{
			Status = "Sprinting \xC2\xB7 the game plays your hands to the next milestone";
		}
		else if (S.SittingOutThisHand)
		{
			Status = "Sitting out this hand";
		}
		else if (HeroVis && HeroVis->Folded)
		{
			Status = "Folded \xC2\xB7 next hand soon";
		}
		else if (S.CurHand && !S.CurHand->bComplete && S.CurHand->ToAct >= 0)
		{
			const HandSeat& ToAct = S.CurHand->Seats[static_cast<size_t>(S.CurHand->ToAct)];
			const SeatVis& V = S.Seats[static_cast<size_t>(ToAct.Seat)];
			Status = ToAct.Id == HeroId ? std::string("Your turn") : "Waiting for " + (V.Present ? V.Name : std::string("\xE2\x80\xA6"));
		}
		else
		{
			Status = "Shuffling up";
		}
		UI.Text(Status, R.X + 16.0f, Fy + 25.0f, Ts(13.0f, 600, pal::Dim, Align::Left, Baseline::Alphabetic, false, R.W - 270.0f));
		// Pace for this table.
		const std::pair<Pace, const char*> Paces[3] = {{Pace::Full, "Full"}, {Pace::Smart, "Smart"}, {Pace::Sprint, "Sprint"}};
		for (int K = 0; K < 3; ++K)
		{
			const Rect Pr{R.X + R.W - 12.0f - Nf(3 - K) * 76.0f, Fy + 6.0f, 72.0f, 28.0f};
			const Ui::ClickState St = UI.Clickable(Id + "pace" + Paces[K].second, Pr, !S.Finishing());
			const bool Lit = S.CurrentPace == Paces[K].first;
			UI.RRect(Pr, 8.0f, Lit ? Hex(0x1f4d63) : St.Hover ? Hex(0x172a42) : pal::Panel, Lit ? pal::Accent : pal::Line);
			UI.Text(Paces[K].second, Pr.X + Pr.W / 2.0f, Pr.Y + 19.0f, Ts(12.5f, 700, Lit ? pal::Ink : pal::Muted, Align::Center));
			if (St.Clicked && !Lit)
			{
				S.CurrentPace = Paces[K].first;
				TileClicked = Index;
			}
		}
	}
}

// ------------------------------------------------------------------ closed tables

void RiverLine::FinishedToasts(double Now)
{
	float Y = 72.0f;
	for (const FinishedTable& F : S.Finished)
	{
		const double Age = Now - F.At;
		if (Age < 0.0 || Age > 9.0)
		{
			continue;
		}
		const Results& Res = F.Result;
		const float In = NetEase(Age / 0.35);
		const float Out = static_cast<float>(Clamp01((9.0 - Age) / 0.6));
		const float W = 384.0f;
		const float H = 78.0f;
		const Rect R{Width - W - 16.0f + (1.0f - In) * 60.0f, Y, W, H};
		const bool Seat = !Res.SeatWon.empty();
		const Chips Won = Res.PrizeCents + Res.BountyCents;
		const Color Col = Res.Won || Seat ? pal::Gold : Won > 0 ? pal::Green : pal::Red;
		const float A0 = C->GetAlpha();
		C->SetAlpha(A0 * In * Out);
		C->GlowRoundRect({R.X, R.Y + 6.0f, R.W, R.H}, 12.0f, Rgba(0, 0, 0, 0.5f), 16.0f);
		UI.RRect(R, 12.0f, Paint::Linear({R.X, 0.0f}, {R.X + R.W, 0.0f}, Mix(Hex(0x0f1a2b), Col, 0.12f), Hex(0x0d1626)), NetA(Col, 0.55f));
		UI.RRect({R.X, R.Y, 5.0f, R.H}, 2.5f, Col);
		NetSpaced(*C, "TABLE CLOSED", R.X + 20.0f, R.Y + 22.0f, 10.0f, 800, pal::Muted, 1.2f);
		const std::string Head = Seat ? std::string("Seat won") : Res.Won ? std::string("Champion") : "Finished " + Ordinal(Res.Place) + " of " + Grouped(Res.Entrants);
		UI.Text(Head, R.X + 20.0f, R.Y + 46.0f, Ts(17.0f, 800, Won > 0 || Res.Won || Seat ? Col : pal::Ink, Align::Left, Baseline::Alphabetic, false, R.W - 150.0f));
		UI.Text(Res.EventName, R.X + 20.0f, R.Y + 66.0f, Ts(12.5f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, false, R.W - 150.0f));
		UI.Text(Won > 0 ? "+" + Money(Won) : Seat ? Money(Res.SeatValueCents) : std::string("No cash"), R.X + R.W - 18.0f, R.Y + 46.0f,
			Ts(18.0f, 800, Won > 0 || Seat ? pal::Gold : pal::Dim, Align::Right, Baseline::Alphabetic, true));
		UI.Text(Res.Grades.empty() ? std::string("no decisions") : Fixed(Res.AccuracyPct, 1) + " accuracy", R.X + R.W - 18.0f, R.Y + 66.0f, Ts(12.0f, 600, pal::Muted, Align::Right));
		C->SetAlpha(A0);
		Y += (H + 10.0f) * Out;
	}
}
} // namespace ui
} // namespace ss
