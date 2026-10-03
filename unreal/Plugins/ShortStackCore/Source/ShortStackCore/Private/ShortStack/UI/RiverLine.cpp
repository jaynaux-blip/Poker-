#include "ShortStack/UI/RiverLine.h"
#include "../StrictFloat.h"

#include "ShortStack/Cards.h"
#include "ShortStack/Evaluator.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Structure.h"
#include "ShortStack/UI/Avatars.h"
#include "RiverLineShared.h"

#include <algorithm>
#include <cmath>

namespace ss
{
namespace ui
{
namespace riverline_detail
{
const float RlW = RiverLine::Width;
const float RlH = RiverLine::Height;
const float TableCx = 590.0f;
const float TableCy = 430.0f;
const float TableRx = 420.0f;
const float TableRy = 226.0f;
const float RlTop = 64.0f;
const float RlSideX = 1188.0f;

Color GradeColor(Grade G)
{
	switch (G)
	{
	case Grade::Best: return Hex(0x3ecf6e);
	case Grade::Good: return Hex(0x27d3c3);
	case Grade::Inaccuracy: return Hex(0xf2c14e);
	case Grade::Mistake: return Hex(0xf28a3a);
	default: return Hex(0xef4d5a);
	}
}

const char* GradeIconText(Grade G)
{
	switch (G)
	{
	case Grade::Inaccuracy: return "?!";
	case Grade::Mistake: return "?";
	case Grade::Blunder: return "??";
	default: return "";
	}
}

float BaselineOf(float Y, const TextStyle& S)
{
	switch (S.VAlign)
	{
	case Baseline::Middle: return Y + 0.29f * S.Size;
	case Baseline::Top: return Y + 0.79f * S.Size;
	case Baseline::Bottom: return Y - 0.21f * S.Size;
	default: return Y;
	}
}

std::string Lower(const std::string& S)
{
	std::string Out = S;
	for (char& Ch : Out)
	{
		Ch = Ch >= 'A' && Ch <= 'Z' ? static_cast<char>(Ch - 'A' + 'a') : Ch;
	}
	return Out;
}

std::string Pad2(int V)
{
	return (V < 10 ? "0" : "") + std::to_string(V);
}

bool StartsWith(const std::string& S, const char* Prefix)
{
	return S.rfind(Prefix, 0) == 0;
}

Color NetChipFill(const Color& Cl)
{
	return {Cl.R, Cl.G, Cl.B, 0.12f};
}
} // namespace riverline_detail

using namespace riverline_detail;

// ------------------------------------------------------------------ frame

void RiverLine::Draw(Canvas& Cv, double Now)
{
	C = &Cv;
	UI.Begin(Cv, Now);
	NetFrame(Now);
	C->Save();
	C->FillRect({0.0f, 0.0f, RlW, RlH}, Paint::Linear({0.0f, 0.0f}, {0.0f, RlH}, pal::Bg2, pal::Bg));
	// While the clock races (a shift, a run, sleep) or a result is up, the screen underneath takes no input.
	const bool Modal = S.TimeSkip.Active || S.HasOutcome;
	const Pointer Real = UI.Ptr;
	if (Modal)
	{
		UI.Ptr.Pressed = false;
		UI.Ptr.Released = false;
		UI.Ptr.Wheel = 0.0f;
		UI.Ptr.Active = false;
	}
	if (AppShown == App::RiverLine)
	{
		TopBar(Now);
		switch (S.CurrentScreen)
		{
		case Screen::Boot: Boot(Now); break;
		case Screen::Lobby: LobbyPages(Now); break;
		case Screen::Table:
		{
			// A player card (a name at the table caught the player's eye) holds the table's input while it's up.
			const bool Card = CardOpen();
			const Pointer Held = UI.Ptr;
			if (Card)
			{
				UI.Ptr.Pressed = false;
				UI.Ptr.Released = false;
				UI.Ptr.Wheel = 0.0f;
				UI.Ptr.Active = false;
			}
			if (S.Tiled && S.TableCount() >= 2)
			{
				Tiles(Now);
			}
			else
			{
				Table(Now);
			}
			if (Card)
			{
				UI.Ptr = Held;
				PlayerCard(Now);
			}
			break;
		}
		case Screen::Results: ResultsScreen(Now); break;
		}
		FinishedToasts(Now);
	}
	else if (AppShown == App::ShiftLink)
	{
		ShiftLinkApp(Now);
	}
	else if (AppShown == App::Burner)
	{
		BurnerApp(Now);
	}
	else if (AppShown == App::GearDrop)
	{
		GearDropApp(Now);
	}
	else if (AppShown == App::Kast)
	{
		KastApp(Now);
	}
	else
	{
		BankApp(Now);
	}
	Taskbar({0.0f, RlH - 38.0f, RlW, 38.0f}, Now);
	if (SleepOpen)
	{
		SleepMenu(Now);
	}
	if (Modal)
	{
		const bool Cursor = UI.CursorIsPointer;
		UI.Ptr = Real;
		UI.CursorIsPointer = Cursor;
		if (S.TimeSkip.Active)
		{
			SkipOverlay(Now);
		}
		else
		{
			OutcomeCard(Now);
		}
	}
	DrawCursor();
	C->Restore();
	UI.End();
	C = nullptr;
}

// ------------------------------------------------------------------ small drawing helpers

float RiverLine::GradeLabel(Grade G, const std::string& Rest, float X, float Y, const TextStyle& Style, bool Draw)
{
	const float Sz = Style.Size;
	const bool Shape = G == Grade::Best || G == Grade::Good;
	const std::string IconText = GradeIconText(G);
	const float IconW = Shape ? Sz * 0.95f : C->Measure(IconText, Sz, Style.Weight, Style.Mono);
	const float Gap = C->Measure(" ", Sz, Style.Weight, Style.Mono);
	const float RestW = C->Measure(Rest, Sz, Style.Weight, Style.Mono);
	const float Total = IconW + Gap + RestW;
	if (!Draw)
	{
		return Total;
	}
	const float X0 = X - (Style.HAlign == Align::Center ? Total / 2.0f : Style.HAlign == Align::Right ? Total : 0.0f);
	const float Base = BaselineOf(Y, Style);
	TextStyle Left = Style;
	Left.HAlign = Align::Left;
	if (G == Grade::Best)
	{
		std::vector<Vec2> Star;
		const float Cx = X0 + Sz * 0.47f;
		const float Cy = Base - Sz * 0.37f;
		for (int I = 0; I < 10; ++I)
		{
			const float A = -Pi / 2.0f + static_cast<float>(I) * Pi / 5.0f;
			const float R = (I % 2 == 0) ? Sz * 0.46f : Sz * 0.19f;
			Star.push_back({Cx + R * std::cos(A), Cy + R * std::sin(A)});
		}
		C->FillPolygon(Star, Style.Col);
	}
	else if (G == Grade::Good)
	{
		C->StrokePolyline({{X0 + Sz * 0.1f, Base - Sz * 0.37f}, {X0 + Sz * 0.38f, Base - Sz * 0.1f}, {X0 + Sz * 0.9f, Base - Sz * 0.7f}}, false, Style.Col, Sz * 0.15f);
	}
	else
	{
		C->Text(IconText, X0, Y, Left);
	}
	C->Text(Rest, X0 + IconW + Gap, Y, Left);
	return Total;
}

float RiverLine::ArrowText(const std::string& LeftText, const std::string& RightText, float X, float Y, const TextStyle& Style)
{
	const float Sz = Style.Size;
	const float Lw = C->Measure(LeftText, Sz, Style.Weight, Style.Mono);
	const float Rw = C->Measure(RightText, Sz, Style.Weight, Style.Mono);
	const float Aw = Sz * 0.8f;
	const float Gap = Sz * 0.4f;
	const float Total = Lw + Gap + Aw + Gap + Rw;
	const float X0 = X - (Style.HAlign == Align::Center ? Total / 2.0f : Style.HAlign == Align::Right ? Total : 0.0f);
	TextStyle Left = Style;
	Left.HAlign = Align::Left;
	C->Text(LeftText, X0, Y, Left);
	const float Base = BaselineOf(Y, Style);
	const float Ay = Base - Sz * 0.33f;
	const float Ax = X0 + Lw + Gap;
	const float Lw2 = Sz * 0.09f > 1.2f ? Sz * 0.09f : 1.2f;
	C->StrokePolyline({{Ax, Ay}, {Ax + Aw, Ay}}, false, Style.Col, Lw2);
	C->StrokePolyline({{Ax + Aw - Sz * 0.28f, Ay - Sz * 0.24f}, {Ax + Aw, Ay}, {Ax + Aw - Sz * 0.28f, Ay + Sz * 0.24f}}, false, Style.Col, Lw2);
	C->Text(RightText, Ax + Aw + Gap, Y, Left);
	return Total;
}

void RiverLine::Ghost(float Cx, float Cy, float Sz)
{
	const Color Body = Rgba(255, 255, 255, 0.92f);
	C->FillCircle(Cx, Cy - Sz * 0.12f, Sz * 0.34f, Body);
	C->FillRect({Cx - Sz * 0.34f, Cy - Sz * 0.12f, Sz * 0.68f, Sz * 0.36f}, Body);
	for (int I = 0; I < 3; ++I)
	{
		const float X0 = Cx - Sz * 0.34f + static_cast<float>(I) * Sz * 0.2267f;
		C->FillPolygon({{X0, Cy + Sz * 0.23f}, {X0 + Sz * 0.2267f, Cy + Sz * 0.23f}, {X0 + Sz * 0.113f, Cy + Sz * 0.4f}}, Body);
	}
	C->FillEllipse(Cx - Sz * 0.12f, Cy - Sz * 0.14f, Sz * 0.06f, Sz * 0.09f, Hex(0x2a0d14));
	C->FillEllipse(Cx + Sz * 0.12f, Cy - Sz * 0.14f, Sz * 0.06f, Sz * 0.09f, Hex(0x2a0d14));
}

// ------------------------------------------------------------------ chrome

void RiverLine::Logo(float X, float Y, float Scale)
{
	C->Save();
	C->Translate(X, Y);
	C->Scale(Scale, Scale);
	const float Prev = C->GetAlpha();
	for (int K = 0; K < 2; ++K)
	{
		std::vector<Vec2> Pts;
		for (int I = 0; I <= 36; ++I)
		{
			Pts.push_back({static_cast<float>(I), 6.0f + static_cast<float>(K) * 11.0f + std::sin((static_cast<float>(I) / 36.0f) * Pi * 2.0f) * 5.0f});
		}
		C->SetAlpha(Prev * (K == 0 ? 1.0f : 0.55f));
		C->StrokePolyline(Pts, false, pal::Accent, 5.0f, true);
	}
	C->SetAlpha(Prev);
	UI.Text("River", 48.0f, 26.0f, Ts(28.0f, 800, pal::Ink));
	UI.Text("Line", 48.0f + UI.Measure("River", 28.0f, 800), 26.0f, Ts(28.0f, 400, pal::Accent));
	C->Restore();
}

void RiverLine::TopBar(double Now)
{
	C->FillRect({0.0f, 0.0f, RlW, RlTop}, Hex(0x08101b));
	C->FillRect({0.0f, RlTop - 1.0f, RlW, 1.0f}, pal::Line);
	Logo(24.0f, 16.0f, 1.0f);
	if (S.CurrentScreen != Screen::Boot)
	{
		NavTabs(Now);
	}
	// Balance and clock.
	UI.Text(ClockString(S.ClockMinutes()), RlW - 24.0f, 34.0f, Ts(17.0f, 700, pal::Ink, Align::Right, Baseline::Alphabetic, true));
	UI.Text(std::string(net::WeekdayName(net::DayOf(World))) + ", " + net::DateLabel(net::DayOf(World)), RlW - 24.0f, 52.0f, Ts(12.0f, 600, pal::Muted, Align::Right));
	const std::string Bal = "Balance " + Money(S.BankrollCents);
	const float Bw = UI.Measure(Bal, 17.0f, 700) + 28.0f;
	UI.RRect({RlW - 140.0f - Bw, 16.0f, Bw, 32.0f}, 16.0f, Hex(0x10213a), pal::Line);
	UI.Text(Bal, RlW - 140.0f - Bw / 2.0f, 38.0f, Ts(17.0f, 700, pal::Gold, Align::Center));
	if (S.Streaming())
	{
		LivePill(RlW - 150.0f - Bw, 17.0f, Now); // on Kast: viewers, a click away from the studio
	}
	// Connection dot.
	C->FillCircle(RlW - 128.0f, 32.0f, 4.0f, std::sin(Now * 2.0) > -0.9 ? pal::Green : pal::Dim);
}

Chips RiverLine::PotTotal() const
{
	Chips Total = S.PotChips;
	for (const SeatVis& Seat : S.Seats)
	{
		if (Seat.Present)
		{
			Total += Seat.Bet;
		}
	}
	return Total;
}

void RiverLine::DrawCursor()
{
	const Pointer& P = UI.Ptr;
	if (!P.Active)
	{
		return;
	}
	C->Save();
	C->Translate(P.X, P.Y);
	if (UI.CursorIsPointer)
	{
		C->FillCircle(0.0f, 0.0f, 9.0f, Rgba(39, 211, 195, 0.25f));
		C->StrokeArc(0.0f, 0.0f, 9.0f, 0.0f, 2.0f * Pi, Hex(0xffffff), 2.0f);
	}
	else
	{
		const std::vector<Vec2> Arrow = {{0, 0}, {0, 26}, {7, 20}, {12, 31}, {16, 29}, {11, 18}, {19, 18}};
		C->FillPolygon(Arrow, Hex(0xffffff));
		C->StrokePolyline(Arrow, true, Hex(0x000000), 1.5f);
	}
	C->Restore();
}

// ------------------------------------------------------------------ boot

void RiverLine::Boot(double Now)
{
	// Animated waves.
	const float Prev = C->GetAlpha();
	C->SetAlpha(Prev * 0.18f);
	for (int K = 0; K < 6; ++K)
	{
		std::vector<Vec2> Pts;
		for (float X = 0.0f; X <= RlW; X += 8.0f)
		{
			Pts.push_back({X, 640.0f + static_cast<float>(K) * 34.0f + static_cast<float>(std::sin(X / 140.0f + Now * 0.6 + K)) * 18.0f});
		}
		C->StrokePolyline(Pts, false, K % 2 ? pal::Accent : pal::Accent2, 2.0f);
	}
	C->SetAlpha(Prev);
	const Rect Card{RlW / 2.0f - 280.0f, 200.0f, 560.0f, 420.0f};
	UI.RRect(Card, 18.0f, Rgba(17, 28, 46, 0.92f), pal::Line);
	Logo(Card.X + 150.0f, Card.Y + 50.0f, 1.6f);
	UI.Text("Welcome back, " + S.HeroName, RlW / 2.0f, Card.Y + 175.0f, Ts(26.0f, 700, pal::Ink, Align::Center));
	UI.Text("Account balance", RlW / 2.0f, Card.Y + 225.0f, Ts(16.0f, 500, pal::Muted, Align::Center));
	UI.Text(Money(S.BankrollCents), RlW / 2.0f, Card.Y + 275.0f, Ts(44.0f, 800, pal::Gold, Align::Center, Baseline::Alphabetic, true));
	ButtonOpts Login;
	Login.Kind = ButtonKind::Primary;
	Login.Size = 24.0f;
	if (UI.Button("login", {Card.X + 60.0f, Card.Y + 320.0f, Card.W - 120.0f, 64.0f}, "Log in", Login))
	{
		S.CurrentScreen = Screen::Lobby;
		S.OnBoot();
	}
	UI.Text("Play responsibly. RiverLine is a fictional site.", RlW / 2.0f, Card.Y + Card.H + 40.0f, Ts(14.0f, 500, pal::Dim, Align::Center));
}

float RiverLine::Wrap(const std::string& Text, float X, float Y, float MaxW, float Size, const Color& Col, float LineHeight)
{
	float Yy = Y;
	for (const std::string& Line : WrapLines(Text, MaxW, Size))
	{
		UI.Text(Line, X, Yy, Ts(Size, 500, Col));
		Yy += LineHeight;
	}
	return Yy;
}

std::vector<std::string> RiverLine::WrapLines(const std::string& Text, float MaxW, float Size)
{
	std::vector<std::string> Out;
	std::string Line;
	size_t Start = 0;
	while (Start <= Text.size())
	{
		size_t End = Text.find(' ', Start);
		if (End == std::string::npos)
		{
			End = Text.size();
		}
		const std::string Word = Text.substr(Start, End - Start);
		const std::string Test = Line.empty() ? Word : Line + " " + Word;
		if (UI.Measure(Test, Size) > MaxW && !Line.empty())
		{
			Out.push_back(Line);
			Line = Word;
		}
		else
		{
			Line = Test;
		}
		Start = End + 1;
	}
	if (!Line.empty())
	{
		Out.push_back(Line);
	}
	return Out;
}

// ------------------------------------------------------------------ table

int RiverLine::HeroSeatNo() const
{
	for (size_t I = 0; I < S.Seats.size(); ++I)
	{
		if (S.Seats[I].Present && S.Seats[I].IsHero)
		{
			return static_cast<int>(I);
		}
	}
	return 0;
}

Vec2 RiverLine::SlotPos(int Seat) const
{
	// Seats around the oval from the hero at the bottom (nine-handed, or six-max).
	const int N = std::max(2, static_cast<int>(S.Seats.size()));
	const int Slot = (Seat - HeroSeatNo() + N) % N;
	const float A = (90.0f + static_cast<float>(Slot) * 360.0f / static_cast<float>(N)) * Pi / 180.0f;
	return {TableCx + (TableRx + 50.0f) * std::cos(A), TableCy + (TableRy + 64.0f) * std::sin(A)};
}

Vec2 RiverLine::BetPos(int Seat) const
{
	const Vec2 P = SlotPos(Seat);
	return {TableCx + (P.X - TableCx) * 0.56f, TableCy + (P.Y - TableCy) * 0.5f};
}

void RiverLine::Table(double Now)
{
	if (!S.T)
	{
		return;
	}
	const Tournament& T = *S.T;

	// Felt and rail.
	C->FillEllipse(TableCx, TableCy + 10.0f, TableRx + 34.0f, TableRy + 34.0f, Rgba(0, 0, 0, 0.5f));
	C->FillEllipse(TableCx, TableCy, TableRx + 30.0f, TableRy + 30.0f, Paint::Linear({0.0f, TableCy - TableRy}, {0.0f, TableCy + TableRy}, Hex(0x2a3342), Hex(0x10151d)));
	C->FillEllipse(TableCx, TableCy, TableRx, TableRy, Paint::Radial({TableCx, TableCy - 40.0f}, 30.0f, {TableCx, TableCy}, TableRx, Hex(0x15875f), 0.7f, pal::Felt, pal::FeltDark));
	C->StrokeEllipse(TableCx, TableCy, TableRx - 40.0f, TableRy - 34.0f, Rgba(255, 255, 255, 0.08f), 2.0f);
	C->SetAlpha(0.14f);
	Logo(TableCx - 95.0f, TableCy + 88.0f, 1.25f);
	C->SetAlpha(1.0f);

	// Pot and board: the stack is what's been gathered in; the label counts the bets out in front too.
	if (S.PotChips > 0)
	{
		DrawChipStack(*C, S.PotChips, TableCx, TableCy - 75.0f, 1.0f);
	}
	if (const Chips Total = PotTotal(); Total > 0)
	{
		UI.RRect({TableCx - 80.0f, TableCy - 128.0f, 160.0f, 30.0f}, 15.0f, Rgba(0, 0, 0, 0.45f));
		UI.Text("Pot " + ChipsText(static_cast<double>(Total)), TableCx, TableCy - 107.0f, Ts(17.0f, 700, pal::Ink, Align::Center));
	}
	const float Bw = 84.0f;
	const float Bh = 118.0f;
	const float Bx0 = TableCx - (5.0f * Bw + 4.0f * 10.0f) / 2.0f;
	const std::vector<Card> Winners = WinningCards();
	auto IsWinner = [&](Card Cd) { return std::find(Winners.begin(), Winners.end(), Cd) != Winners.end(); };
	for (size_t I = 0; I < S.Board.size(); ++I)
	{
		const double At = I < S.BoardShownAt.size() ? S.BoardShownAt[I] : 0.0;
		if (Now < At)
		{
			continue;
		}
		CardOpts O;
		O.Flip = static_cast<float>(std::min(1.0, (Now - At) / 0.25));
		O.Highlight = IsWinner(S.Board[I]);
		O.Dim = !Winners.empty() && !IsWinner(S.Board[I]);
		DrawCard(*C, S.Board[I], Bx0 + static_cast<float>(I) * (Bw + 10.0f), TableCy - 45.0f, Bw, Bh, O);
	}

	// Dealer button.
	const Vec2 Bp = SlotPos(S.ButtonSeat);
	const float Dbx = TableCx + (Bp.X - TableCx) * 0.74f + 34.0f;
	const float Dby = TableCy + (Bp.Y - TableCy) * 0.7f;
	C->FillCircle(Dbx, Dby, 15.0f, Hex(0xf4f4f4));
	UI.Text("D", Dbx, Dby + 1.0f, Ts(16.0f, 800, Hex(0x111111), Align::Center, Baseline::Middle));

	// Bets.
	for (const SeatVis& Seat : S.Seats)
	{
		if (!Seat.Present || Seat.Bet <= 0)
		{
			continue;
		}
		const Vec2 P = BetPos(Seat.Seat);
		DrawChipStack(*C, Seat.Bet, P.X, P.Y, 0.85f);
		UI.Text(ChipsText(static_cast<double>(Seat.Bet)), P.X, P.Y + 30.0f, Ts(15.0f, 700, Hex(0xffffff), Align::Center));
	}

	// Seats.
	for (const SeatVis& Seat : S.Seats)
	{
		if (Seat.Present)
		{
			DrawSeat(Seat, Now);
		}
	}
	for (size_t I = 0; I < S.Seats.size(); ++I)
	{
		if (S.Seats[I].Present)
		{
			continue;
		}
		const Vec2 P = SlotPos(static_cast<int>(I));
		UI.RRect({P.X - 60.0f, P.Y - 22.0f, 120.0f, 44.0f}, 22.0f, Rgba(255, 255, 255, 0.03f), Rgba(255, 255, 255, 0.08f));
		UI.Text("Empty", P.X, P.Y + 6.0f, Ts(14.0f, 500, pal::Dim, Align::Center));
	}

	// Flights.
	for (const Flight& F : S.Flights)
	{
		DrawFlight(F, Now);
	}

	// Hero hand strength.
	const SeatVis* Hero = nullptr;
	for (const SeatVis& Seat : S.Seats)
	{
		Hero = Seat.Present && Seat.IsHero ? &Seat : Hero;
	}
	if (Hero && Hero->Hole.size() == 2 && !Hero->Folded && S.Board.size() >= 3)
	{
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
			const std::string Label = Describe(EvaluateHand(Hero->Hole, Shown));
			const Vec2 Hp = SlotPos(Hero->Seat);
			UI.RRect({Hp.X - 110.0f, Hp.Y - 190.0f, 220.0f, 28.0f}, 14.0f, Rgba(0, 0, 0, 0.55f));
			UI.Text(Label, Hp.X, Hp.Y - 170.0f, Ts(15.0f, 700, pal::Gold, Align::Center));
		}
	}

	// The format: knockouts and seats.
	if (T.Spec.BountyCents > 0 || T.Spec.SeatValueCents > 0)
	{
		std::string Chip;
		if (T.Spec.SeatValueCents > 0)
		{
			Chip = "SATELLITE \xC2\xB7 " + std::to_string(S.SeatsInPlay()) + " seats \xC2\xB7 " + Grouped(T.Remaining) + " left";
		}
		else if (T.Spec.MysteryBounty)
		{
			Chip = "MYSTERY BOUNTY \xC2\xB7 envelopes open in the money \xC2\xB7 " + std::to_string(S.Knockouts) + " KOs \xC2\xB7 " + Money(S.BountyWon);
		}
		else
		{
			const auto Mine = S.Bounties.find(HeroId);
			Chip = "PKO \xC2\xB7 your bounty " + Money(Mine == S.Bounties.end() ? 0 : Mine->second) + " \xC2\xB7 " + std::to_string(S.Knockouts) + " KOs \xC2\xB7 won " + Money(S.BountyWon);
		}
		const Color Cc = T.Spec.SeatValueCents > 0 ? pal::Gold : pal::Orange;
		const float W = UI.Measure(Chip, 13.0f, 800) + 28.0f;
		UI.RRect({24.0f, RlTop + 14.0f, W, 30.0f}, 15.0f, NetChipFill(Cc), Cc);
		UI.Text(Chip, 24.0f + W / 2.0f, RlTop + 30.0f, Ts(13.0f, 800, Cc, Align::Center, Baseline::Middle));
	}
	if (Now - S.LastBountyAt < 2.6)
	{
		const float K = static_cast<float>((Now - S.LastBountyAt) / 2.6);
		const Vec2 Hp = SlotPos(HeroSeatNo());
		C->SetAlpha(K < 0.75f ? 1.0f : (1.0f - K) * 4.0f);
		const float Y = Hp.Y - 40.0f - K * 50.0f;
		UI.Text("+" + Money(S.LastBountyCents), Hp.X + 140.0f, Y, Ts(34.0f, 900, pal::Gold, Align::Left, Baseline::Alphabetic, true));
		UI.Text((T.Spec.MysteryBounty ? "MYSTERY BOUNTY \xC2\xB7 " : "BOUNTY \xC2\xB7 ") + S.LastBountyName, Hp.X + 140.0f, Y + 22.0f, Ts(13.0f, 800, pal::Orange));
		C->SetAlpha(1.0f);
	}

	GradeBadges(Now);
	Controls(Now);
	SidePanel(Now);
	Overlays(Now);
	if (S.Streaming() && !Previewing)
	{
		StreamAlert(TableCx, TableCy - TableRy + 24.0f, 0.72f, Now); // follows, subs, tips as they come in, on the felt
	}
}

std::vector<Card> RiverLine::WinningCards() const
{
	std::vector<Card> Out;
	if (!S.CurHand || !S.CurHand->bComplete || S.Board.size() < 5)
	{
		return Out;
	}
	const SeatVis* Winner = nullptr;
	for (const SeatVis& Seat : S.Seats)
	{
		if (!Winner && Seat.Present && Seat.Winner && !Seat.HandLabel.empty())
		{
			Winner = &Seat;
		}
	}
	if (!Winner || Winner->Hole.size() != 2)
	{
		return Out;
	}
	// Highlight the five cards that make the winning hand.
	std::vector<Card> All = Winner->Hole;
	All.insert(All.end(), S.Board.begin(), S.Board.end());
	const int Target = EvaluateHand(Winner->Hole, S.Board);
	for (int A = 0; A < 7; ++A)
	{
		for (int B = A + 1; B < 7; ++B)
		{
			std::vector<Card> Five;
			for (int I = 0; I < 7; ++I)
			{
				if (I != A && I != B)
				{
					Five.push_back(All[static_cast<size_t>(I)]);
				}
			}
			if (Evaluate(Five.data(), 5) == Target)
			{
				return Five;
			}
		}
	}
	return Out;
}

void RiverLine::DrawSeat(const SeatVis& Seat, double Now)
{
	const Vec2 P = SlotPos(Seat.Seat);
	const bool IsHero = Seat.IsHero;
	const float Pw = IsHero ? 230.0f : 190.0f;
	const float Ph = 64.0f;
	const Rect Plate{P.X - Pw / 2.0f, P.Y - Ph / 2.0f, Pw, Ph};
	const bool Dim = Seat.Folded && !Seat.Winner;
	const bool HandOver = S.CurHand && S.CurHand->bComplete;

	// Cards.
	if (Seat.HasCards && !Seat.Folded)
	{
		const double Dealt = Now - Seat.DealtAt;
		if (IsHero && Seat.Hole.size() == 2)
		{
			const float Cw = 80.0f;
			const float Ch = 112.0f;
			for (size_t I = 0; I < 2; ++I)
			{
				CardOpts O;
				O.Flip = static_cast<float>(Clamp01((Dealt - 0.35 - static_cast<double>(I) * 0.1) / 0.25));
				O.Rotate = (static_cast<float>(I) - 0.5f) * 0.06f;
				O.Highlight = Seat.Winner;
				DrawCard(*C, Seat.Hole[I], P.X - Cw - 4.0f + static_cast<float>(I) * (Cw + 8.0f), P.Y - Ph / 2.0f - Ch + 16.0f, Cw, Ch, O);
			}
		}
		else if (Seat.Hole.size() == 2)
		{
			const float Cw = 58.0f;
			const float Ch = 82.0f;
			for (size_t I = 0; I < 2; ++I)
			{
				CardOpts O;
				O.Highlight = Seat.Winner;
				DrawCard(*C, Seat.Hole[I], P.X - Cw - 2.0f + static_cast<float>(I) * (Cw + 4.0f), P.Y - Ph / 2.0f - Ch + 14.0f, Cw, Ch, O);
			}
		}
		else if (Dealt > 0.4)
		{
			const float Cw = 44.0f;
			const float Ch = 62.0f;
			for (int I = 0; I < 2; ++I)
			{
				CardOpts O;
				O.Rotate = (static_cast<float>(I) - 0.5f) * 0.15f;
				DrawCard(*C, -1, P.X - 30.0f + static_cast<float>(I) * 18.0f, P.Y - Ph / 2.0f - Ch + 18.0f, Cw, Ch, O);
			}
		}
	}
	else if (IsHero && Seat.Folded && S.AutoFolded && Now - S.AutoFoldedAt < 1.5)
	{
		for (size_t I = 0; I < S.AutoFoldedCards.size(); ++I)
		{
			CardOpts O;
			O.Alpha = static_cast<float>(1.0 - (Now - S.AutoFoldedAt) / 1.5);
			O.Dim = true;
			DrawCard(*C, S.AutoFoldedCards[I], P.X - 84.0f + static_cast<float>(I) * 88.0f, P.Y - Ph / 2.0f - 100.0f, 80.0f, 112.0f, O);
		}
	}

	// Plate.
	C->SetAlpha(Dim ? 0.55f : 1.0f);
	if (Seat.Acting)
	{
		UI.RRectStroke({Plate.X - 4.0f, Plate.Y - 4.0f, Plate.W + 8.0f, Plate.H + 8.0f}, 16.0f, pal::Accent, 2.5f);
	}
	if (Seat.Winner && HandOver)
	{
		C->GlowRoundRect(Plate, 14.0f, Rgba(242, 193, 78, 0.9f), 24.0f);
		UI.RRect(Plate, 14.0f, Hex(0x2d2410));
	}
	const Paint PlateBg = Paint::Linear({0.0f, Plate.Y}, {0.0f, Plate.Y + Plate.H}, IsHero ? Hex(0x1b3552) : Hex(0x1a2536), IsHero ? Hex(0x10223a) : Hex(0x0f1726));
	const Color Edge = Seat.IsRival ? Hex(0xb8324a) : Seat.Winner && HandOver ? pal::Gold : Rgba(255, 255, 255, 0.1f);
	UI.RRect(Plate, 14.0f, PlateBg, Edge, Seat.IsRival ? 2.0f : 1.0f);
	// Avatar, with the country flag pinned to it.
	AvatarSpec Pic = AvatarFor(Seat.Name);
	if (IsHero)
	{
		Pic.Frame = AvatarFrame::Neon;
		Pic.Rim = 0x27d3c3;
	}
	else if (Seat.Pro)
	{
		Pic.Frame = AvatarFrame::Gold;
	}
	rlnet_detail::NetChampion(Pic, Seat.Name, IsHero);
	DrawAvatar(*C, Plate.X + 32.0f, P.Y, 21.0f, Pic);
	if (!Seat.Country.empty())
	{
		const Rect Fl{Plate.X + 40.0f, P.Y + 10.0f, 15.0f, 10.0f};
		C->FillRoundRect({Fl.X - 2.0f, Fl.Y - 2.0f, Fl.W + 4.0f, Fl.H + 4.0f}, 3.5f, Hex(0x0b1220));
		rlnet_detail::NetFlag(*C, Seat.Country, Fl.X, Fl.Y, Fl.W, Fl.H);
	}
	// Someone the world knows: their name opens their card.
	const auto Known = IsHero ? S.FieldNpc.end() : S.FieldNpc.find(Seat.Id);
	bool NameHover = false;
	if (Known != S.FieldNpc.end())
	{
		const Ui::ClickState St = UI.Clickable("seatcard" + Seat.Id, Plate);
		NameHover = St.Hover;
		if (St.Clicked)
		{
			ShowPlayer(Known->second, Now);
		}
	}
	UI.Text(Seat.Name, Plate.X + 62.0f, P.Y - 6.0f, Ts(16.0f, 700, Seat.IsRival ? Hex(0xff8da0) : NameHover ? pal::Accent : pal::Ink, Align::Left, Baseline::Alphabetic, false, Pw - 72.0f));
	if (S.T && S.T->Spec.BountyCents > 0 && !S.T->Spec.MysteryBounty)
	{
		const auto Head = S.Bounties.find(Seat.Id);
		const std::string Tag = Money(Head == S.Bounties.end() ? 0 : Head->second);
		const float Tw = UI.Measure(Tag, 12.0f, 800) + 16.0f;
		UI.RRect({Plate.X + Pw - Tw - 6.0f, Plate.Y - 11.0f, Tw, 22.0f}, 11.0f, Hex(0x3a1f08), pal::Orange);
		UI.Text(Tag, Plate.X + Pw - Tw / 2.0f - 6.0f, Plate.Y + 0.5f, Ts(12.0f, 800, pal::Orange, Align::Center, Baseline::Middle, true));
	}
	const double Bb = S.CurHand ? static_cast<double>(S.CurHand->BigBlind) : 1.0;
	const std::string StackText = Seat.Stack <= 0 && Seat.AllIn ? std::string("ALL-IN") : ChipsText(static_cast<double>(Seat.Stack));
	UI.Text(StackText, Plate.X + 62.0f, P.Y + 19.0f, Ts(16.0f, 700, Seat.AllIn ? pal::Red : pal::Gold, Align::Left, Baseline::Alphabetic, true));
	if (Seat.Stack > 0)
	{
		const double InBB = static_cast<double>(Seat.Stack) / Bb;
		UI.Text(Fixed(InBB, InBB < 10.0 ? 1 : 0) + " BB", Plate.X + Pw - 12.0f, P.Y + 19.0f, Ts(13.0f, 500, pal::Muted, Align::Right));
	}
	C->SetAlpha(1.0f);

	// Timer bar.
	if (Seat.Acting)
	{
		const double Total = Seat.ActEnd - Seat.ActStart;
		const double Left = std::max(0.0, Seat.ActEnd - Now);
		double Frac = Total > 0.0 ? Left / Total : 0.0;
		UI.RRect({Plate.X + 8.0f, Plate.Y + Plate.H + 5.0f, Plate.W - 16.0f, 6.0f}, 3.0f, Rgba(0, 0, 0, 0.5f));
		const bool Bank = IsHero && S.HasPrompt && S.Prompt.TimeBankUntil > 0.0;
		const Color Col = Bank ? pal::Orange : Frac < 0.25 ? pal::Red : pal::Accent;
		if (Bank)
		{
			Frac = std::max(0.0, (S.Prompt.TimeBankUntil - Now) / std::max(1.0, S.TimeBank));
		}
		UI.RRect({Plate.X + 8.0f, Plate.Y + Plate.H + 5.0f, (Plate.W - 16.0f) * static_cast<float>(Frac), 6.0f}, 3.0f, Col);
	}

	// Last action tag.
	if (!Seat.LastAction.empty() && Now - Seat.LastActionAt < 30.0)
	{
		const std::string& Tag = Seat.LastAction;
		const Color Col = Tag == "Fold" ? pal::Dim : Tag == "Check" ? Hex(0x5f7fa6) : StartsWith(Tag, "Call") ? pal::Accent2 : Tag == "All-in" ? pal::Red : pal::Orange;
		const float Tw = UI.Measure(Tag, 14.0f, 800) + 20.0f;
		const float Ty = Plate.Y + Plate.H + (Seat.Acting ? 16.0f : 8.0f);
		UI.RRect({P.X - Tw / 2.0f, Ty, Tw, 24.0f}, 12.0f, Col);
		UI.Text(Tag, P.X, Ty + 17.0f, Ts(14.0f, 800, Hex(0xffffff), Align::Center));
	}

	// All-in equity (broadcast style).
	if (Seat.HasEquity && !Seat.Folded && S.CurHand && !S.CurHand->bComplete)
	{
		const double Eq = Seat.Equity;
		const float Ey = Plate.Y - 28.0f;
		const float Ex = P.X + (IsHero ? 128.0f : 100.0f);
		UI.RRect({Ex - 36.0f, Ey - 18.0f, 72.0f, 30.0f}, 8.0f, Rgba(0, 0, 0, 0.7f), Eq >= 0.5 ? pal::Green : pal::Red, 1.5f);
		UI.Text(std::to_string(static_cast<int>(JsRound(Eq * 100.0))) + "%", Ex, Ey + 4.0f, Ts(17.0f, 800, Eq >= 0.5 ? pal::Green : Hex(0xff9aa4), Align::Center, Baseline::Alphabetic, true));
	}
	if (!Seat.HandLabel.empty() && HandOver && !Seat.Folded)
	{
		const float Tw = UI.Measure(Seat.HandLabel, 13.0f, 700) + 16.0f;
		UI.RRect({P.X - Tw / 2.0f, Plate.Y - 20.0f, Tw, 22.0f}, 11.0f, Seat.Winner ? pal::Gold : Rgba(0, 0, 0, 0.7f));
		UI.Text(Seat.HandLabel, P.X, Plate.Y - 4.0f, Ts(13.0f, 700, Seat.Winner ? Hex(0x231704) : pal::Ink, Align::Center));
	}

	// HUD.
	if (S.Hud && !IsHero)
	{
		const TPlayer* Pl = S.PlayerById(Seat.Id);
		if (Pl && Pl->Hands >= 3)
		{
			const int Vp = static_cast<int>(JsRound(static_cast<double>(Pl->VpipHands) / Pl->Hands * 100.0));
			const int Pf = static_cast<int>(JsRound(static_cast<double>(Pl->PfrHands) / Pl->Hands * 100.0));
			const std::string Hud = std::to_string(Vp) + "/" + std::to_string(Pf) + " \xC2\xB7 " + std::to_string(Pl->Hands) + "h";
			const Color Col = Vp >= 45 ? Hex(0xff9aa4) : Vp <= 14 ? Hex(0x8fb7ff) : pal::Muted;
			float Hy = Plate.Y - (Seat.HasCards && !Seat.Folded ? 72.0f : 8.0f);
			float Hx = P.X;
			Align Ha = Align::Center;
			if (Hy < RlTop + 20.0f)
			{
				// The seats along the top: beside their cards, toward the middle, clear of the top bar.
				Hy = Plate.Y - 30.0f;
				Hx = P.X + (P.X < TableCx ? 66.0f : -66.0f);
				Ha = P.X < TableCx ? Align::Left : Align::Right;
			}
			UI.Text(Hud, Hx, Hy, Ts(13.0f, 700, Col, Ha, Baseline::Alphabetic, true));
		}
	}
}

Vec2 RiverLine::FlightPoint(const FlightEnd& End) const
{
	switch (End.Type)
	{
	case FlightEnd::Kind::Pot: return {TableCx, TableCy - 75.0f};
	case FlightEnd::Kind::Deck: return {TableCx, TableCy - 160.0f};
	case FlightEnd::Kind::Muck: return {TableCx, TableCy - 20.0f};
	case FlightEnd::Kind::Bet: return BetPos(End.Seat);
	default:
	{
		const Vec2 P = SlotPos(End.Seat);
		return {P.X, P.Y - 30.0f};
	}
	}
}

void RiverLine::DrawFlight(const Flight& F, double Now)
{
	const double T = (Now - F.Start) / F.Dur;
	if (T < 0.0 || T > 1.0)
	{
		return;
	}
	const float K = static_cast<float>(EaseOutCubic(T));
	const Vec2 A = FlightPoint(F.From);
	const Vec2 B = FlightPoint(F.To);
	const float X = A.X + (B.X - A.X) * K;
	const float Y = A.Y + (B.Y - A.Y) * K - std::sin(K * Pi) * 20.0f;
	if (F.IsChips)
	{
		DrawChipStack(*C, F.Amount, X, Y, 0.75f, static_cast<float>(1.0 - std::max(0.0, T - 0.85) * 5.0));
	}
	else
	{
		CardOpts O;
		O.Rotate = K * 3.0f;
		O.Alpha = static_cast<float>(1.0 - std::max(0.0, T - 0.8) * 4.0);
		DrawCard(*C, -1, X - 20.0f, Y - 28.0f, 40.0f, 56.0f, O);
	}
}

void RiverLine::GradeBadges(double Now)
{
	const SeatVis* Hero = nullptr;
	for (const SeatVis& Seat : S.Seats)
	{
		Hero = Seat.Present && Seat.IsHero ? &Seat : Hero;
	}
	if (!Hero)
	{
		return;
	}
	const Vec2 P = SlotPos(Hero->Seat);
	for (size_t I = 0; I < S.Badges.size(); ++I)
	{
		const GradeBadge& Bd = S.Badges[I];
		const double Age = Now - Bd.At;
		const double Alpha = std::min(1.0, Age * 5.0) * (1.0 - std::max(0.0, Age - 2.6) / 0.6);
		const float Y = P.Y - 70.0f - static_cast<float>(I) * 6.0f - static_cast<float>(EaseOutCubic(Age * 2.0)) * 30.0f;
		C->SetAlpha(static_cast<float>(std::max(0.0, Alpha)));
		const TextStyle St = Ts(17.0f, 800, Hex(0x08101b), Align::Center, Baseline::Middle);
		const std::string Name = GradeName(Bd.G.Result);
		const float Wd = GradeLabel(Bd.G.Result, Name, 0.0f, 0.0f, St, false) + 28.0f;
		const float X = P.X + 150.0f;
		UI.RRect({X, Y - 20.0f, Wd, 34.0f}, 17.0f, GradeColor(Bd.G.Result));
		GradeLabel(Bd.G.Result, Name, X + Wd / 2.0f, Y + 3.0f, St);
		C->SetAlpha(1.0f);
	}
	// Explanation of the latest graded decision under the action area.
	if (!S.Grades.empty() && !S.Badges.empty() && Now - S.Badges.back().At < 3.2 && !S.HasPrompt)
	{
		const DecisionGrade& Last = S.Grades.back();
		const std::vector<std::string> Lines = WrapLines(Last.Note, 520.0f, 16.0f);
		for (size_t I = 0; I < Lines.size() && I < 2; ++I)
		{
			UI.Text(Lines[I], 1170.0f, 930.0f + static_cast<float>(I) * 24.0f, Ts(16.0f, 600, GradeColor(Last.Result), Align::Right));
		}
	}
}

// ------------------------------------------------------------------ controls

void RiverLine::Key(const std::string& K)
{
	if (S.CurrentScreen == Screen::Lobby && PageShown == Page::Lobby && (K == "ArrowUp" || K == "ArrowDown") && !Listed.empty())
	{
		const auto Found = std::find(Listed.begin(), Listed.end(), EventId);
		int I = Found == Listed.end() ? -1 : static_cast<int>(Found - Listed.begin());
		I = std::min(std::max(I + (K == "ArrowUp" ? -1 : 1), 0), static_cast<int>(Listed.size()) - 1);
		SelectEvent(Listed[static_cast<size_t>(I)]);
		return;
	}
	if (!S.HasPrompt)
	{
		return;
	}
	HeroPrompt& P = S.Prompt;
	const std::string Lk = Lower(K);
	if (Lk == "f")
	{
		S.HeroAct(PlayerAction::Fold());
	}
	else if (Lk == "c" || Lk == "x")
	{
		S.HeroAct(P.CanCheck ? PlayerAction::Check() : PlayerAction::Call());
	}
	else if (Lk == "r" || Lk == "b")
	{
		S.HeroAct(PlayerAction::RaiseTo(static_cast<double>(P.RaiseTo)));
	}
	else if (Lk == "a")
	{
		S.HeroAct(PlayerAction::RaiseTo(static_cast<double>(P.MaxRaise)));
	}
	else if (K == "ArrowUp")
	{
		P.RaiseTo = MinChips(P.MaxRaise, P.RaiseTo + P.BigBlind);
	}
	else if (K == "ArrowDown")
	{
		P.RaiseTo = MaxChips(P.MinRaise, P.RaiseTo - P.BigBlind);
	}
}

double RiverLine::PotFrac(const HeroPrompt& P, double F) const
{
	const double Cur = S.CurHand ? static_cast<double>(S.CurHand->CurrentBet) : 0.0;
	return JsRound(Cur + static_cast<double>(P.Pot + P.ToCall) * F);
}

double RiverLine::PotRaise(const HeroPrompt& P) const
{
	const double Cur = S.CurHand ? static_cast<double>(S.CurHand->CurrentBet) : 0.0;
	return JsRound(Cur + static_cast<double>(P.Pot + P.ToCall));
}

void RiverLine::Controls(double Now)
{
	const float Ax = 640.0f;
	const float Aw = 1170.0f - Ax;

	// Pace and HUD (bottom-left).
	const std::pair<Pace, const char*> Paces[3] = {{Pace::Full, "Full"}, {Pace::Smart, "Smart"}, {Pace::Sprint, "Sprint"}};
	UI.Text("Pace", 24.0f, 866.0f, Ts(13.0f, 700, pal::Muted));
	for (int I = 0; I < 3; ++I)
	{
		const Rect R{24.0f + static_cast<float>(I) * 96.0f, 874.0f, 92.0f, 36.0f};
		const Ui::ClickState St = UI.Clickable(std::string("pace") + Paces[I].second, R);
		const bool Active = S.CurrentPace == Paces[I].first;
		UI.RRect(R, 9.0f, Active ? Hex(0x1f4d63) : St.Hover ? Hex(0x172a42) : pal::Panel, Active ? pal::Accent : pal::Line);
		UI.Text(Paces[I].second, R.X + R.W / 2.0f, R.Y + 24.0f, Ts(15.0f, 700, Active ? pal::Ink : pal::Muted, Align::Center));
		if (St.Clicked && !Active)
		{
			S.CurrentPace = Paces[I].first;
			if (Paces[I].first == Pace::Sprint)
			{
				S.SystemLine("Sprint: autopilot plays your hands until the bubble or the final table. Safe, a bit passive, a step behind your best.");
			}
		}
	}
	const Rect HudR{24.0f, 918.0f, 130.0f, 30.0f};
	if (UI.Clickable("hud", HudR).Clicked)
	{
		S.Hud = !S.Hud;
	}
	UI.RRect(HudR, 8.0f, S.Hud ? Hex(0x16354a) : pal::Panel, S.Hud ? pal::Accent : pal::Line);
	UI.Text(S.Hud ? "HUD ON" : "HUD OFF", HudR.X + HudR.W / 2.0f, HudR.Y + 21.0f, Ts(13.0f, 700, S.Hud ? pal::Ink : pal::Muted, Align::Center));
	const Rect LeanR{164.0f, 918.0f, 148.0f, 30.0f};
	if (UI.Clickable("lean", LeanR).Clicked)
	{
		LeanBackRequested = true;
	}
	UI.RRect(LeanR, 8.0f, pal::Panel, pal::Line);
	UI.Text("Lean back (Space)", LeanR.X + LeanR.W / 2.0f, LeanR.Y + 21.0f, Ts(13.0f, 700, pal::Muted, Align::Center));

	// Composure meter.
	const double Tilt = S.HeroTilt;
	UI.Text("Composure", 24.0f, 812.0f, Ts(13.0f, 700, pal::Muted));
	UI.RRect({112.0f, 802.0f, 170.0f, 10.0f}, 5.0f, Hex(0x0b1422), pal::Line);
	const float Comp = static_cast<float>(1.0 - Tilt);
	UI.RRect({112.0f, 802.0f, 170.0f * Comp, 10.0f}, 5.0f, Comp > 0.6f ? pal::Green : Comp > 0.35f ? pal::Gold : pal::Red);
	if (Tilt > 0.3 && !S.SitOutNext)
	{
		ButtonOpts O;
		O.Kind = ButtonKind::Ghost;
		O.Size = 13.0f;
		if (UI.Button("sitout", {24.0f, 824.0f, 258.0f, 26.0f}, "Step away: sit out next hand", O))
		{
			S.RequestSitOut();
		}
	}

	if (!S.HasPrompt)
	{
		// Waiting state.
		const bool NoteUp = !S.Grades.empty() && !S.Badges.empty() && Now - S.Badges.back().At < 3.2; // GradeBadges explains the last decision here
		if (S.CurHand && !S.CurHand->bComplete && !S.Sprinting && S.CurHand->ToAct >= 0 && !NoteUp)
		{
			const HandSeat& ToAct = S.CurHand->Seats[static_cast<size_t>(S.CurHand->ToAct)];
			if (ToAct.Id != HeroId)
			{
				const SeatVis& V = S.Seats[static_cast<size_t>(ToAct.Seat)];
				UI.Text("Waiting for " + (V.Present ? V.Name : std::string("\xE2\x80\xA6")), 1170.0f, 930.0f, Ts(15.0f, 500, pal::Dim, Align::Right));
			}
		}
		return;
	}
	HeroPrompt& P = S.Prompt;

	// Bet sizing.
	if (P.CanRaise)
	{
		const double Cur = S.CurHand ? static_cast<double>(S.CurHand->CurrentBet) : static_cast<double>(P.BigBlind);
		std::vector<std::pair<std::string, double>> Presets;
		if (P.OnStreet == Street::Preflop)
		{
			Presets = {{"Min", static_cast<double>(P.MinRaise)}, {"2.5x", JsRound(Cur * 2.5)}, {"3x", JsRound(Cur * 3.0)}, {"Pot", PotRaise(P)}, {"All-in", static_cast<double>(P.MaxRaise)}};
		}
		else
		{
			Presets = {{"1/3", PotFrac(P, 1.0 / 3.0)}, {"1/2", PotFrac(P, 0.5)}, {"3/4", PotFrac(P, 0.75)}, {"Pot", PotRaise(P)}, {"All-in", static_cast<double>(P.MaxRaise)}};
		}
		for (size_t I = 0; I < Presets.size(); ++I)
		{
			const Rect R{Ax + static_cast<float>(I) * 72.0f, 786.0f, 66.0f, 34.0f};
			const Chips Val = static_cast<Chips>(std::max(static_cast<double>(P.MinRaise), std::min(static_cast<double>(P.MaxRaise), Presets[I].second)));
			ButtonOpts O;
			O.Kind = P.RaiseTo == Val ? ButtonKind::Primary : ButtonKind::Secondary;
			O.Size = 15.0f;
			if (UI.Button("pre" + std::to_string(I), R, Presets[I].first, O))
			{
				P.RaiseTo = Val;
			}
		}
		const Rect Box{Ax + 370.0f, 786.0f, Aw - 370.0f, 34.0f};
		UI.RRect(Box, 8.0f, Hex(0x0b1422), pal::Line);
		UI.Text(ChipsText(static_cast<double>(P.RaiseTo)), Box.X + Box.W - 12.0f, Box.Y + 24.0f, Ts(18.0f, 700, pal::Ink, Align::Right, Baseline::Alphabetic, true));
		UI.Text(Fixed(static_cast<double>(P.RaiseTo) / static_cast<double>(P.BigBlind), 1) + " BB", Box.X + 10.0f, Box.Y + 23.0f, Ts(13.0f, 500, pal::Muted));
		const double Step = std::max(1.0, JsRound(static_cast<double>(P.BigBlind) / 4.0));
		P.RaiseTo = static_cast<Chips>(UI.Slider("raise", {Ax + 10.0f, 826.0f, Aw - 20.0f, 26.0f}, static_cast<double>(P.RaiseTo), static_cast<double>(P.MinRaise), static_cast<double>(P.MaxRaise), Step));
	}

	// Main buttons (each may end the prompt, so re-check it).
	const float Bw = (Aw - 20.0f) / 3.0f;
	const float By = 862.0f;
	ButtonOpts FoldO;
	FoldO.Kind = ButtonKind::Danger;
	FoldO.Hotkey = "F";
	const bool CanCheck = P.CanCheck;
	const bool CanRaise = P.CanRaise;
	const Chips ToCall = P.ToCall;
	const Chips RaiseTo = P.RaiseTo;
	const Chips MaxRaise = P.MaxRaise;
	const bool IsBet = P.IsBet;
	const Chips PotNow = P.Pot;
	if (UI.Button("fold", {Ax, By, Bw, 66.0f}, "Fold", FoldO))
	{
		S.HeroAct(PlayerAction::Fold());
	}
	Chips HeroStack = 0;
	if (S.CurHand)
	{
		for (const HandSeat& Hs : S.CurHand->Seats)
		{
			HeroStack = Hs.Id == HeroId ? Hs.Stack : HeroStack;
		}
	}
	const std::string CallLabel = CanCheck ? "Check" : ToCall >= HeroStack ? "Call all-in" : "Call";
	ButtonOpts CallO;
	CallO.Hotkey = "C";
	CallO.Sub = CanCheck ? std::string() : ChipsText(static_cast<double>(ToCall));
	if (UI.Button("call", {Ax + Bw + 10.0f, By, Bw, 66.0f}, CallLabel, CallO) && S.HasPrompt)
	{
		S.HeroAct(CanCheck ? PlayerAction::Check() : PlayerAction::Call());
	}
	if (CanRaise)
	{
		const bool AllIn = RaiseTo >= MaxRaise;
		ButtonOpts RaiseO;
		RaiseO.Kind = AllIn ? ButtonKind::Gold : ButtonKind::Primary;
		RaiseO.Sub = ChipsText(static_cast<double>(RaiseTo));
		RaiseO.Hotkey = "R";
		if (UI.Button("raise", {Ax + (Bw + 10.0f) * 2.0f, By, Bw, 66.0f}, AllIn ? "All-in" : IsBet ? "Bet" : "Raise to", RaiseO) && S.HasPrompt)
		{
			S.HeroAct(PlayerAction::RaiseTo(static_cast<double>(RaiseTo)));
		}
	}
	// Pot odds readout (Math skill, level 1).
	if (!CanCheck && ToCall > 0)
	{
		const double Odds = static_cast<double>(ToCall) / static_cast<double>(PotNow + ToCall);
		UI.Text("Call " + ChipsText(static_cast<double>(ToCall)) + " to win " + ChipsText(static_cast<double>(PotNow)) + " \xC2\xB7 you need " + std::to_string(static_cast<int>(JsRound(Odds * 100.0))) + "% equity", 1170.0f, 948.0f,
			Ts(15.0f, 600, pal::Muted, Align::Right));
	}
}

// ------------------------------------------------------------------ side panel

void RiverLine::SidePanel(double Now)
{
	const Tournament& T = *S.T;
	const float X = RlSideX;
	const float Wd = RlW - X - 16.0f;
	const Rect Card{X, RlTop + 14.0f, Wd, 300.0f};
	UI.RRect(Card, 14.0f, pal::Panel, pal::Line);
	UI.Text(T.Spec.Name, X + 18.0f, Card.Y + 34.0f, Ts(19.0f, 800, pal::Ink, Align::Left, Baseline::Alphabetic, false, Wd - 120.0f));
	UI.Text("Table " + std::to_string(S.TableId), X + Wd - 18.0f, Card.Y + 34.0f, Ts(15.0f, 700, pal::Muted, Align::Right));
	const Level& Lvl = T.CurrentLevel();
	const int Secs = static_cast<int>(std::max(0.0, JsRound(T.LevelSecondsLeft())));
	const Color Flash = Now - S.LastLevelUpAt < 2.0 ? pal::Gold : pal::Ink;
	UI.Text("Level " + std::to_string(T.LevelIndex + 1), X + 18.0f, Card.Y + 68.0f, Ts(15.0f, 500, pal::Muted));
	UI.Text(ChipsText(static_cast<double>(Lvl.Sb)) + "/" + ChipsText(static_cast<double>(Lvl.Bb)) + " \xC2\xB7 ante " + ChipsText(static_cast<double>(Lvl.Ante)), X + Wd - 18.0f, Card.Y + 68.0f,
		Ts(17.0f, 700, Flash, Align::Right, Baseline::Alphabetic, true));
	UI.Text("Next level", X + 18.0f, Card.Y + 96.0f, Ts(15.0f, 500, pal::Muted));
	ArrowText(std::to_string(Secs / 60) + ":" + Pad2(Secs % 60), ChipsText(static_cast<double>(T.NextLevel().Sb)) + "/" + ChipsText(static_cast<double>(T.NextLevel().Bb)), X + Wd - 18.0f, Card.Y + 96.0f,
		Ts(15.0f, 600, pal::Ink, Align::Right, Baseline::Alphabetic, true));
	const double Avg = T.AverageStack();
	const Hand* Hh = S.CurHand.get();
	(void)Hh;
	struct Row
	{
		std::string K;
		std::string V;
		bool HasColor;
		Color Col;
	};
	std::vector<Row> Rows;
	Rows.push_back({"Players left", ChipsText(T.Remaining) + " / " + ChipsText(T.Spec.Entrants), false, pal::Ink});
	Rows.push_back({"Average stack", ChipsText(JsRound(Avg)) + " (" + std::to_string(static_cast<int>(JsRound(Avg / static_cast<double>(Lvl.Bb)))) + " BB)", false, pal::Ink});
	Rows.push_back({"Your rank", T.Hero().Busted ? std::string("\xE2\x80\x94") : Ordinal(T.HeroRank()) + " of " + ChipsText(T.Remaining), false, pal::Ink});
	if (T.InTheMoney())
	{
		const Chips Jump = T.PrizeFor(T.Remaining - 1) != 0 ? T.PrizeFor(T.Remaining - 1) : T.PrizeFor(1);
		Rows.push_back({"In the money", "next jump " + Money(Jump), true, pal::Green});
	}
	else
	{
		Rows.push_back({"To the money", ChipsText(T.Remaining - T.PaidPlaces()) + " players", T.HandForHand(), pal::Orange});
	}
	Rows.push_back({"Min cash / 1st", Money(T.Payouts.back()) + " / " + Money(T.Payouts.front()), false, pal::Ink});
	for (size_t I = 0; I < Rows.size(); ++I)
	{
		const float Y = Card.Y + 134.0f + static_cast<float>(I) * 32.0f;
		UI.Text(Rows[I].K, X + 18.0f, Y, Ts(15.0f, 500, pal::Muted));
		UI.Text(Rows[I].V, X + Wd - 18.0f, Y, Ts(15.0f, 700, Rows[I].HasColor ? Rows[I].Col : pal::Ink, Align::Right));
	}

	// Tabs.
	// Live on Kast: the stream gets a tab of its own.
	const bool Live = S.Streaming();
	const RightTab Shown = !Live && S.Tab == RightTab::Stream ? RightTab::Chat : S.Tab;
	const std::pair<RightTab, const char*> Tabs[4] = {{RightTab::Chat, "Chat"}, {RightTab::Stream, "Stream"}, {RightTab::Payouts, "Payouts"}, {RightTab::Info, "Standings"}};
	const int TabCount = Live ? 4 : 3;
	const float Ty = Card.Y + Card.H + 14.0f;
	for (int I = 0, Slot = 0; I < 4; ++I)
	{
		if (!Live && Tabs[I].first == RightTab::Stream)
		{
			continue;
		}
		const float TabW = Wd / static_cast<float>(TabCount);
		const Rect R{X + static_cast<float>(Slot++) * TabW, Ty, TabW - 6.0f, 36.0f};
		const Ui::ClickState St = UI.Clickable(std::string("tab") + Tabs[I].second, R);
		if (St.Clicked)
		{
			S.Tab = Tabs[I].first;
		}
		const bool Active = Shown == Tabs[I].first;
		if (Active)
		{
			UI.RRect(R, 9.0f, pal::Panel2, pal::Line);
		}
		UI.Text(Tabs[I].second, R.X + R.W / 2.0f, R.Y + 24.0f, Ts(15.0f, 700, Active ? pal::Ink : pal::Muted, Align::Center));
		if (Tabs[I].first == RightTab::Stream)
		{
			C->FillCircle(R.X + 12.0f, R.Y + 18.0f, 4.0f, rlnet_detail::NetA(pal::Red, 0.6f + 0.4f * static_cast<float>(std::sin(Now * 4.0))));
		}
	}
	const Rect Box{X, Ty + 44.0f, Wd, RlH - 38.0f - (Ty + 44.0f) - 12.0f};
	if (Shown == RightTab::Stream)
	{
		StreamSide(Box, Now);
		return;
	}
	UI.RRect(Box, 14.0f, Hex(0x0c1524), pal::Line);
	C->PushClip(Box);
	if (Shown == RightTab::Chat)
	{
		// Wrap from the bottom up.
		float Y = Box.Y + Box.H - 14.0f;
		for (size_t Ii = S.Chat.size(); Ii-- > 0 && Y > Box.Y + 10.0f;)
		{
			const ChatLine& L = S.Chat[Ii];
			const Color Col = L.Kind == ChatKind::Dealer ? Hex(0x8fa0bb) : L.Kind == ChatKind::System ? pal::Gold : L.Kind == ChatKind::Rival ? Hex(0xff8da0) : pal::Ink;
			const Color WhoCol = L.Kind == ChatKind::Rival ? Hex(0xff5c7a) : L.Kind == ChatKind::Dealer ? Hex(0x5f6f88) : pal::Accent;
			const std::string Prefix = L.Who.empty() ? std::string() : L.Who + ": ";
			const std::vector<std::string> Wrapped = WrapLines(Prefix + L.Text, Box.W - 28.0f, 14.0f);
			for (size_t J = Wrapped.size(); J-- > 0 && Y > Box.Y + 10.0f;)
			{
				if (J == 0 && !Prefix.empty())
				{
					const float Pw = UI.Text(Prefix, Box.X + 14.0f, Y, Ts(14.0f, 700, WhoCol));
					UI.Text(Wrapped[0].size() > Prefix.size() ? Wrapped[0].substr(Prefix.size()) : std::string(), Box.X + 14.0f + Pw, Y, Ts(14.0f, 500, Col));
				}
				else
				{
					UI.Text(Wrapped[J], Box.X + 14.0f, Y, Ts(14.0f, 500, Col));
				}
				Y -= 20.0f;
			}
		}
	}
	else if (S.Tab == RightTab::Payouts)
	{
		float Y = Box.Y + 30.0f;
		for (const std::pair<int, int>& Band : PayoutBands(static_cast<int>(T.Payouts.size())))
		{
			if (Y > Box.Y + Box.H - 10.0f)
			{
				break;
			}
			const std::string Label = Band.first == Band.second ? Ordinal(Band.first) : Ordinal(Band.first) + "\xE2\x80\x93" + Ordinal(Band.second);
			const bool Reached = T.Remaining <= Band.second;
			UI.Text(Label, Box.X + 16.0f, Y, Ts(14.0f, 500, Reached ? pal::Ink : pal::Muted));
			UI.Text(Money(T.Payouts[static_cast<size_t>(Band.first - 1)]), Box.X + Box.W - 16.0f, Y, Ts(14.0f, 700, Reached ? pal::Gold : pal::Muted, Align::Right, Baseline::Alphabetic, true));
			Y += 22.0f;
		}
	}
	else
	{
		const std::vector<const TPlayer*> St = T.Standings();
		const int HeroRank = T.HeroRank();
		float Y = Box.Y + 30.0f;
		for (size_t I = 0; I < St.size() && I < 12; ++I)
		{
			const TPlayer& Pl = *St[I];
			UI.Text(std::to_string(I + 1) + ".", Box.X + 16.0f, Y, Ts(14.0f, 500, pal::Muted, Align::Left, Baseline::Alphabetic, true));
			UI.Text(Pl.Name, Box.X + 56.0f, Y, Ts(14.0f, Pl.IsHero ? 800 : 500, Pl.IsHero ? pal::Accent : Pl.Name == RivalName ? Hex(0xff8da0) : pal::Ink, Align::Left, Baseline::Alphabetic, false, 190.0f));
			UI.Text(ChipsText(static_cast<double>(Pl.Stack)), Box.X + Box.W - 16.0f, Y, Ts(14.0f, 700, pal::Ink, Align::Right, Baseline::Alphabetic, true));
			Y += 22.0f;
		}
		if (!T.Hero().Busted && HeroRank > 12)
		{
			Y += 8.0f;
			UI.Text(std::to_string(HeroRank) + ".", Box.X + 16.0f, Y, Ts(14.0f, 500, pal::Muted, Align::Left, Baseline::Alphabetic, true));
			UI.Text(T.Hero().Name, Box.X + 56.0f, Y, Ts(14.0f, 800, pal::Accent));
			UI.Text(ChipsText(static_cast<double>(T.Hero().Stack)), Box.X + Box.W - 16.0f, Y, Ts(14.0f, 700, pal::Ink, Align::Right, Baseline::Alphabetic, true));
		}
	}
	C->PopClip();
}

// ------------------------------------------------------------------ overlays

void RiverLine::Overlays(double Now)
{
	if (S.Moving)
	{
		const double Age = Now - S.MovingAt;
		const double A = std::min(1.0, Age * 4.0) * (1.0 - std::max(0.0, Age - 1.6) / 0.6);
		C->SetAlpha(static_cast<float>(std::max(0.0, A)));
		UI.RRect({TableCx - 230.0f, TableCy - 70.0f, 460.0f, 110.0f}, 16.0f, Rgba(8, 16, 27, 0.94f), pal::Accent2);
		UI.Text("Table balancing", TableCx, TableCy - 30.0f, Ts(17.0f, 500, pal::Muted, Align::Center));
		ArrowText("Table " + std::to_string(S.MovingFrom), "Table " + std::to_string(S.MovingTo), TableCx, TableCy + 12.0f, Ts(28.0f, 800, pal::Ink, Align::Center));
		C->SetAlpha(1.0f);
	}
	if (S.CurrentBanner.Active)
	{
		const Banner& B = S.CurrentBanner;
		const double Age = Now - B.At;
		const double A = std::min(1.0, Age * 3.0) * (1.0 - std::max(0.0, Age - 2.6) / 0.6);
		const float Sc = static_cast<float>(0.9 + 0.1 * EaseOutBack(std::min(1.0, Age * 2.5)));
		const Color Col = Hex(B.Color);
		C->Save();
		C->SetAlpha(static_cast<float>(std::max(0.0, A)));
		C->Translate(TableCx, TableCy - 10.0f);
		C->Scale(Sc, Sc);
		UI.RRect({-330.0f, -80.0f, 660.0f, 150.0f}, 20.0f, Rgba(6, 10, 18, 0.92f), Col, 2.0f);
		UI.Text(B.Title, 0.0f, -12.0f, Ts(42.0f, 900, Col, Align::Center));
		UI.Text(B.Sub, 0.0f, 36.0f, Ts(20.0f, 600, pal::Ink, Align::Center));
		C->Restore();
	}
	if (S.Sprinting && S.T)
	{
		const Tournament& T = *S.T;
		C->FillRect({0.0f, RlTop, RlSideX - 8.0f, RlH - RlTop}, Rgba(5, 9, 16, 0.78f));
		UI.Text("SPRINTING AHEAD", TableCx, 330.0f, Ts(40.0f, 900, pal::Accent, Align::Center));
		UI.Text("The game is playing your hands in a solid, tight-aggressive style.", TableCx, 372.0f, Ts(17.0f, 500, pal::Muted, Align::Center));
		const Level& L = T.CurrentLevel();
		const std::pair<std::string, std::string> Stats[4] = {
			{"Level", std::to_string(T.LevelIndex + 1) + " \xC2\xB7 " + ChipsText(static_cast<double>(L.Sb)) + "/" + ChipsText(static_cast<double>(L.Bb))},
			{"Players left", ChipsText(T.Remaining)},
			{"Your stack", ChipsText(static_cast<double>(T.Hero().Stack)) + " (" + std::to_string(static_cast<int>(JsRound(static_cast<double>(T.Hero().Stack) / static_cast<double>(L.Bb)))) + " BB)"},
			{"Your rank", Ordinal(T.HeroRank())},
		};
		for (int I = 0; I < 4; ++I)
		{
			const float X = TableCx - 330.0f + static_cast<float>(I) * 220.0f;
			UI.Text(Stats[I].first, X + 100.0f, 450.0f, Ts(15.0f, 500, pal::Muted, Align::Center));
			UI.Text(Stats[I].second, X + 100.0f, 482.0f, Ts(21.0f, 800, pal::Ink, Align::Center, Baseline::Alphabetic, true));
		}
		const float Bar = static_cast<float>(std::fmod(Now * 0.8, 1.0));
		UI.RRect({TableCx - 300.0f, 530.0f, 600.0f, 8.0f}, 4.0f, Hex(0x0b1422));
		UI.RRect({TableCx - 300.0f + Bar * 480.0f, 530.0f, 120.0f, 8.0f}, 4.0f, pal::Accent);
		ButtonOpts O;
		O.Kind = ButtonKind::Gold;
		O.Size = 20.0f;
		if (UI.Button("stopsprint", {TableCx - 120.0f, 580.0f, 240.0f, 60.0f}, "Take the wheel", O))
		{
			S.StopSprint("you took over");
		}
	}
}

// ------------------------------------------------------------------ results

void RiverLine::ResultsScreen(double Now)
{
	if (!S.HasResults)
	{
		return;
	}
	const Results& R = S.LastResults;
	const bool Seat = !R.SeatWon.empty();
	const bool Cashed = R.PrizeCents > 0 || R.BountyCents > 0 || Seat;
	const Color Col = R.Won || Seat ? pal::Gold : Cashed ? pal::Green : pal::Red;
	UI.Text(R.EventName, 60.0f, RlTop + 60.0f, Ts(20.0f, 600, pal::Muted));
	UI.Text(Seat ? std::string("Seat won.") : R.Won ? std::string("Champion.") : "You finished " + Ordinal(R.Place), 60.0f, RlTop + 130.0f, Ts(60.0f, 900, Col));
	UI.Text("of " + ChipsText(R.Entrants) + " players", 60.0f, RlTop + 172.0f, Ts(22.0f, 500, pal::Muted));
	if (Seat)
	{
		const std::string To = R.SeatWon == "rcop-main" ? "the RCOP Main Event" : R.SeatWon == "step2" ? "Step 2" : R.SeatWon == "step3" ? "Step 3" : R.SeatWon == "step4" ? "Step 4" : R.SeatWon;
		UI.Text("A " + Money(R.SeatValueCents) + " ticket to " + To, 60.0f, RlTop + 250.0f, Ts(40.0f, 800, pal::Gold, Align::Left, Baseline::Alphabetic, false, 1060.0f));
	}
	else if (R.PrizeCents > 0 || R.BountyCents > 0)
	{
		const float Pw = UI.Text("+" + Money(R.PrizeCents + R.BountyCents), 60.0f, RlTop + 250.0f, Ts(46.0f, 800, pal::Gold, Align::Left, Baseline::Alphabetic, true));
		if (R.BountyCents > 0)
		{
			UI.Text(Money(R.BountyCents) + " in bounties \xC2\xB7 " + std::to_string(R.Knockouts) + (R.Knockouts == 1 ? " knockout" : " knockouts"), 80.0f + Pw, RlTop + 248.0f, Ts(18.0f, 700, pal::Orange));
		}
	}
	else
	{
		UI.Text("No cash this time", 60.0f, RlTop + 250.0f, Ts(40.0f, 800, pal::Muted));
	}
	UI.Text("Buy-in " + (R.BuyInCents ? Money(R.BuyInCents) : std::string("free")) + " \xC2\xB7 " + std::to_string(R.Hands) + " hands \xC2\xB7 biggest pot won " + ChipsText(static_cast<double>(R.BiggestPot)), 60.0f, RlTop + 292.0f, Ts(17.0f, 500, pal::Muted));
	const float BalW = UI.Text("Balance now " + Money(S.BankrollCents), 60.0f, RlTop + 330.0f, Ts(20.0f, 700, pal::Ink));
	if (R.SessionEvents > 1)
	{
		// Multi-tabling: the whole sitting, every buy-in against every prize.
		const std::string Net = (R.SessionNetCents >= 0 ? "+" : "") + Money(R.SessionNetCents);
		const float Lw = UI.Text("This sitting: " + std::to_string(R.SessionEvents) + " tournaments \xC2\xB7 net ", 84.0f + BalW, RlTop + 330.0f, Ts(17.0f, 600, pal::Muted));
		UI.Text(Net, 84.0f + BalW + Lw, RlTop + 330.0f, Ts(17.0f, 800, R.SessionNetCents >= 0 ? pal::Green : pal::Red, Align::Left, Baseline::Alphabetic, true));
	}

	// Accuracy gauge.
	const float Gx = 1240.0f;
	const float Gy = RlTop + 200.0f;
	const double Acc = R.AccuracyPct;
	const double Shown = Acc * (Now >= S.ResultsAt ? EaseOutCubic((Now - S.ResultsAt) / 1.4) : 1.0);
	C->StrokeArc(Gx, Gy, 110.0f, Pi * 0.75f, Pi * 2.25f, Hex(0x13213a), 18.0f, true);
	if (Shown > 0.1)
	{
		const Color GaugeCol = Acc >= 85.0 ? pal::Green : Acc >= 70.0 ? pal::Accent : Acc >= 55.0 ? pal::Gold : pal::Red;
		C->StrokeArc(Gx, Gy, 110.0f, Pi * 0.75f, Pi * 0.75f + (Pi * 1.5f * static_cast<float>(Shown)) / 100.0f, GaugeCol, 18.0f, true);
	}
	UI.Text(R.Grades.empty() ? std::string("\xE2\x80\x94") : Fixed(Acc, 1), Gx, Gy + 14.0f, Ts(52.0f, 900, pal::Ink, Align::Center, Baseline::Alphabetic, true));
	UI.Text("Decision accuracy", Gx, Gy + 60.0f, Ts(16.0f, 500, pal::Muted, Align::Center));
	const int Xp = static_cast<int>(JsRound(static_cast<double>(R.Grades.size()) * (Acc / 100.0) * 12.0 + (Cashed ? 60.0 : 0.0)));
	UI.Text("Skill XP +" + std::to_string(Xp), Gx, Gy + 150.0f, Ts(22.0f, 800, pal::Accent, Align::Center));
	UI.Text("XP comes from decisions, not results.", Gx, Gy + 178.0f, Ts(14.0f, 500, pal::Muted, Align::Center));

	// Grade distribution.
	const Grade Order[5] = {Grade::Best, Grade::Good, Grade::Inaccuracy, Grade::Mistake, Grade::Blunder};
	int Counts[5] = {0, 0, 0, 0, 0};
	for (const DecisionGrade& G : R.Grades)
	{
		++Counts[static_cast<int>(G.Result)];
	}
	const float Total = static_cast<float>(R.Grades.empty() ? 1 : R.Grades.size());
	float Bx = 60.0f;
	const float By = RlTop + 390.0f;
	for (int I = 0; I < 5; ++I)
	{
		const float Wd = (static_cast<float>(Counts[I]) / Total) * 1000.0f;
		if (Wd > 0.0f)
		{
			UI.RRect({Bx, By, std::max(4.0f, Wd - 3.0f), 22.0f}, 5.0f, GradeColor(Order[I]));
		}
		Bx += Wd;
	}
	for (int I = 0; I < 5; ++I)
	{
		GradeLabel(Order[I], std::string(GradeName(Order[I])) + " " + std::to_string(Counts[I]), 60.0f + static_cast<float>(I) * 200.0f, By + 52.0f, Ts(16.0f, 700, GradeColor(Order[I])));
	}

	// Biggest mistakes.
	std::vector<DecisionGrade> Worst;
	for (const DecisionGrade& G : R.Grades)
	{
		if (G.Result != Grade::Best && G.Result != Grade::Good)
		{
			Worst.push_back(G);
		}
	}
	std::stable_sort(Worst.begin(), Worst.end(), [](const DecisionGrade& A, const DecisionGrade& B) { return A.LossBB > B.LossBB; });
	if (Worst.size() > 4)
	{
		Worst.resize(4);
	}
	UI.Text("Hand review", 60.0f, By + 110.0f, Ts(22.0f, 800));
	if (Worst.empty())
	{
		UI.Text(R.Grades.empty() ? "No decisions graded." : "Clean game. No significant mistakes found.", 60.0f, By + 144.0f, Ts(17.0f, 500, pal::Muted));
	}
	for (size_t I = 0; I < Worst.size(); ++I)
	{
		const DecisionGrade& G = Worst[I];
		const float Y = By + 144.0f + static_cast<float>(I) * 56.0f;
		UI.RRect({60.0f, Y - 26.0f, 1000.0f, 48.0f}, 10.0f, pal::Panel, pal::Line);
		GradeLabel(G.Result, GradeName(G.Result), 78.0f, Y + 4.0f, Ts(16.0f, 800, GradeColor(G.Result)));
		UI.Text(std::string(StreetTitle(G.OnStreet)) + " \xC2\xB7 you chose " + Lower(G.Chosen.Label), 220.0f, Y + 4.0f, Ts(16.0f, 600));
		UI.Text(G.Note, 1045.0f, Y + 4.0f, Ts(15.0f, 500, pal::Muted, Align::Right, Baseline::Alphabetic, false, 480.0f));
	}

	ButtonOpts Back;
	Back.Kind = ButtonKind::Primary;
	if (UI.Button("backlobby", {1120.0f, RlH - 120.0f, 420.0f, 70.0f}, "Back to lobby", Back))
	{
		S.LeaveResults();
	}
}
} // namespace ui
} // namespace ss
