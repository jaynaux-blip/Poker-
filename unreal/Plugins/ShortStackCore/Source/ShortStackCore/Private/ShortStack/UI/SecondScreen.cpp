// The GearDrop monitors' pictures: Kast's studio or the life tracker on the 24", study on the 27".
#include "ShortStack/UI/SecondScreen.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/UI/StreamArt.h"
#include "ShortStack/UI/Ui.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>

namespace ss
{
namespace ui
{
namespace secondscreen
{
namespace secondscreen_detail
{
const float W = Width;
const float H = Height;
const Color Lime = Hex(0x53fc18);
const Color LiveRed = Hex(0xeb0400);

Color Alpha(const Color& C, float A)
{
	return Color{C.R, C.G, C.B, C.A * A};
}

void Backdrop(Canvas& C)
{
	C.FillRect({0.0f, 0.0f, W, H}, Paint::Linear({0.0f, 0.0f}, {0.0f, H}, pal::Bg2, pal::Bg));
}

void Panel(Canvas& C, const Rect& R)
{
	C.FillRoundRect(R, 14.0f, Paint::Solid(pal::Panel));
	C.StrokeRoundRect(R, 14.0f, pal::Line, 1.5f);
}

/** "1:07" from minutes. */
std::string HoursMinutes(double Minutes)
{
	const int M = static_cast<int>(std::max(0.0, Minutes));
	const int Mm = M % 60;
	return std::to_string(M / 60) + (Mm < 10 ? ":0" : ":") + std::to_string(Mm);
}

std::string Signed(Chips Cents)
{
	return (Cents > 0 ? "+" : "") + Money(Cents);
}

/** A sparkline (area and line) of Values across R, scaled between Lo and Hi. */
void Graph(Canvas& C, const Rect& R, const std::vector<float>& Values, float Lo, float Hi, const Color& Col)
{
	if (Values.size() < 2)
	{
		C.StrokePolyline({{R.X, R.Y + R.H * 0.6f}, {R.X + R.W, R.Y + R.H * 0.6f}}, false, Alpha(Col, 0.5f), 2.0f);
		return;
	}
	const float Span = std::max(Hi - Lo, 1e-3f);
	std::vector<Vec2> Pts;
	Pts.reserve(Values.size());
	for (size_t I = 0; I < Values.size(); ++I)
	{
		const float X = R.X + R.W * static_cast<float>(I) / static_cast<float>(Values.size() - 1);
		const float Y = R.Y + R.H * (1.0f - (Values[I] - Lo) / Span);
		Pts.push_back({X, Y});
	}
	// The area under it, a strip of quads (each convex), fading downward.
	const float Bottom = R.Y + R.H;
	for (size_t I = 0; I + 1 < Pts.size(); ++I)
	{
		C.FillPolygon({Pts[I], Pts[I + 1], {Pts[I + 1].X, Bottom}, {Pts[I].X, Bottom}},
		              Paint::Linear({0.0f, R.Y}, {0.0f, Bottom}, Alpha(Col, 0.32f), Alpha(Col, 0.02f)));
	}
	C.StrokePolyline(Pts, false, Col, 3.0f, true);
	C.FillCircle(Pts.back().X, Pts.back().Y, 6.0f, Paint::Solid(Col));
	C.FillCircle(Pts.back().X, Pts.back().Y, 12.0f, Paint::Solid(Alpha(Col, 0.25f)));
}

// ------------------------------------------------------------------ the life tracker

void Stat(Canvas& C, const Rect& R, const std::string& Title, const std::string& Big, const Color& BigCol, const std::string& Sub)
{
	Panel(C, R);
	C.Text(Title, R.X + 22.0f, R.Y + 36.0f, Ts(15.0f, 800, pal::Muted));
	C.Text(Big, R.X + 22.0f, R.Y + 84.0f, Ts(36.0f, 800, BigCol, Align::Left, Baseline::Alphabetic, false, R.W - 44.0f));
	C.Text(Sub, R.X + 22.0f, R.Y + 118.0f, Ts(16.0f, 500, pal::Muted, Align::Left, Baseline::Alphabetic, false, R.W - 44.0f));
}

void Tracker(Canvas& C, const Session& S, double Now)
{
	Backdrop(C);
	const life::State& L = S.Life;
	const double World = S.WorldMinutes();
	C.Text("BANKROLL", 60.0f, 82.0f, Ts(18.0f, 800, pal::Muted));
	C.Text(Money(S.BankrollCents), 60.0f, 160.0f, Ts(72.0f, 800, pal::Ink));
	Chips Day = 0;
	for (const life::LedgerEntry& E : L.Ledger)
	{
		if (E.At < World - 1440.0)
		{
			break;
		}
		Day += E.Amount;
	}
	if (Day != 0)
	{
		C.Text(Signed(Day) + " in the last 24 hours", 64.0f, 200.0f, Ts(22.0f, 700, Day > 0 ? pal::Green : pal::Red));
	}
	C.Text(ClockString(S.ClockMinutes()), W - 60.0f, 82.0f, Ts(22.0f, 700, pal::Ink, Align::Right));
	C.Text(S.HeroName, W - 60.0f, 112.0f, Ts(16.0f, 600, pal::Muted, Align::Right));

	// The bankroll's story, from the ledger (newest first): walk back from today's balance.
	const Rect G{60.0f, 236.0f, W - 120.0f, 300.0f};
	Panel(C, {G.X - 20.0f, G.Y - 20.0f, G.W + 40.0f, G.H + 40.0f});
	std::vector<float> Curve;
	const size_t N = std::min<size_t>(L.Ledger.size(), 40);
	Chips B = S.BankrollCents;
	Curve.push_back(static_cast<float>(B));
	for (size_t I = 0; I < N; ++I)
	{
		B -= L.Ledger[I].Amount;
		Curve.push_back(static_cast<float>(B));
	}
	std::reverse(Curve.begin(), Curve.end());
	float Lo = *std::min_element(Curve.begin(), Curve.end());
	float Hi = *std::max_element(Curve.begin(), Curve.end());
	const float Pad = std::max((Hi - Lo) * 0.15f, 500.0f);
	Lo = std::max(0.0f, Lo - Pad);
	Hi += Pad;
	for (int K = 1; K < 4; ++K)
	{
		const float Y = G.Y + G.H * static_cast<float>(K) / 4.0f;
		C.StrokePolyline({{G.X, Y}, {G.X + G.W, Y}}, false, Alpha(pal::Line, 0.7f), 1.0f);
		C.Text(Money(static_cast<Chips>(Hi - (Hi - Lo) * static_cast<float>(K) / 4.0f)), G.X + G.W, Y - 6.0f, Ts(13.0f, 600, pal::Dim, Align::Right));
	}
	const bool Up = Curve.back() >= Curve.front();
	Graph(C, G, Curve, Lo, Hi, Up ? pal::Accent : pal::Red);
	if (Curve.size() < 3)
	{
		C.Text("The story starts tonight.", G.X + G.W / 2.0f, G.Y + G.H / 2.0f - 30.0f, Ts(22.0f, 600, pal::Muted, Align::Center));
	}

	// Rent, Dee's game and the Riverside.
	const float Ty = 586.0f;
	const float Tw = (W - 120.0f - 40.0f) / 3.0f;
	{
		std::string Big = Money(L.RentDueCents);
		std::string Sub;
		Color Col = pal::Ink;
		const double Left = L.RentDeadline - World;
		switch (L.RentStage)
		{
		case life::Rent::Paid: Big = "PAID"; Col = pal::Green; Sub = "Next: " + Money(L.RentDueCents) + " in " + std::to_string(static_cast<int>(std::max(0.0, Left) / 1440.0)) + " days"; break;
		case life::Rent::FinalNotice: Col = pal::Red; Sub = "FINAL NOTICE \xC2\xB7 " + HoursMinutes(Left) + " left"; break;
		case life::Rent::Evicted: Big = "EVICTED"; Col = pal::Red; Sub = "A key back: " + Money(L.RentDueCents); break;
		default:
			Col = Left < 1440.0 ? pal::Orange : pal::Ink;
			Sub = Left > 1440.0 ? "Due in " + std::to_string(static_cast<int>(Left / 1440.0)) + "d " + std::to_string(static_cast<int>(std::fmod(Left, 1440.0) / 60.0)) + "h"
			                    : "Due in " + HoursMinutes(std::max(0.0, Left));
			break;
		}
		Stat(C, {60.0f, Ty, Tw, 140.0f}, "RENT", Big, Col, Sub);
	}
	Stat(C, {60.0f + Tw + 20.0f, Ty, Tw, 140.0f}, "DEE'S GAME", L.BackRoomNights > 0 ? Signed(L.BackRoomNetCents) : std::string("\xE2\x80\x94"),
	     L.BackRoomNetCents > 0 ? pal::Green : (L.BackRoomNetCents < 0 ? pal::Red : pal::Ink),
	     L.BackRoomNights > 0 ? std::to_string(L.BackRoomNights) + (L.BackRoomNights == 1 ? " night in the back room" : " nights in the back room") : std::string("Laundromat, back room. Ask Dee."));
	Stat(C, {60.0f + 2.0f * (Tw + 20.0f), Ty, Tw, 140.0f}, "THE RIVERSIDE", L.LiveBestPlace > 0 ? Ordinal(L.LiveBestPlace) : std::string("\xE2\x80\x94"),
	     L.LiveBestPlace == 1 ? pal::Gold : pal::Ink,
	     L.LiveEvents > 0 ? std::to_string(L.LiveEvents) + " played \xC2\xB7 " + std::to_string(L.LiveCashes) + " cashed \xC2\xB7 " + Money(L.LiveWonCents) : std::string("Sunday tournament. Bring a buy-in."));

	// The latest entries.
	float Y = 776.0f;
	C.Text("LATELY", 60.0f, Y, Ts(15.0f, 800, pal::Muted));
	for (size_t I = 0; I < std::min<size_t>(L.Ledger.size(), 3); ++I)
	{
		const life::LedgerEntry& E = L.Ledger[I];
		const float Ly = Y + 34.0f + static_cast<float>(I) * 30.0f;
		C.Text(E.Label, 60.0f, Ly, Ts(18.0f, 500, pal::Ink, Align::Left, Baseline::Alphabetic, false, W - 400.0f));
		C.Text(Signed(E.Amount), W - 60.0f, Ly, Ts(18.0f, 700, E.Amount >= 0 ? pal::Green : pal::Red, Align::Right));
	}
	(void)Now;
}

// ------------------------------------------------------------------ Kast studio

void Studio(Canvas& C, const Session& S, double Now)
{
	C.FillRect({0.0f, 0.0f, W, H}, Paint::Solid(Hex(0x0e0e10)));
	const kast::Stream& St = S.Stream;
	const double World = S.WorldMinutes();
	// Header: LIVE, uptime, viewers, followers.
	const float Pulse = 0.65f + 0.35f * static_cast<float>(std::sin(Now * 3.0));
	C.FillRoundRect({40.0f, 28.0f, 96.0f, 40.0f}, 8.0f, Paint::Solid(Alpha(LiveRed, Pulse)));
	C.Text("LIVE", 88.0f, 56.0f, Ts(20.0f, 900, Hex(0xffffff), Align::Center));
	C.Text(HoursMinutes(St.Uptime(World)), 156.0f, 57.0f, Ts(22.0f, 700, pal::Ink, Align::Left, Baseline::Alphabetic, true));
	const std::string Viewers = std::to_string(static_cast<int>(std::round(St.Viewers)));
	C.Text(Viewers + " watching", 330.0f, 57.0f, Ts(22.0f, 800, Lime));
	C.Text(std::to_string(S.Channel.Followers) + " followers", 560.0f, 57.0f, Ts(20.0f, 600, pal::Muted));
	C.Text("KAST STUDIO", W - 40.0f, 57.0f, Ts(18.0f, 900, Hex(0x9b8cff), Align::Right));

	// The program: the facecam as it goes out, with the overlay's name strip.
	const Rect Pg{40.0f, 96.0f, 960.0f, 540.0f};
	C.Text("PROGRAM", Pg.X, Pg.Y - 8.0f, Ts(13.0f, 800, LiveRed));
	streamart::Cam Look;
	Look.Gear = S.GearFx();
	Look.Headphones = S.Owns("headphones");
	Look.Face = St.Face;
	Look.FaceAge = Now - St.FaceAt;
	Look.Talking = Now - St.ThankAt < 3.0;
	Look.Time = Now;
	Look.Live = St.Live;
	Look.Leds = S.RoomGlow(Now);
	C.Save();
	C.PushClip(Pg);
	streamart::Facecam(C, Pg, Look);
	C.PopClip();
	C.Restore();
	C.StrokeRoundRect(Pg, 4.0f, LiveRed, 3.0f);
	C.FillRect({Pg.X, Pg.Y + Pg.H - 56.0f, Pg.W, 56.0f}, Paint::Linear({0.0f, Pg.Y + Pg.H - 56.0f}, {0.0f, Pg.Y + Pg.H}, Alpha(Hex(0x000000), 0.0f), Alpha(Hex(0x000000), 0.75f)));
	C.Text(S.HeroName, Pg.X + 24.0f, Pg.Y + Pg.H - 18.0f, Ts(24.0f, 900, Hex(0xffffff)));
	C.Text("SHORT STACK \xC2\xB7 grinding the night", Pg.X + 28.0f + C.Measure(S.HeroName, 24.0f, 900), Pg.Y + Pg.H - 18.0f, Ts(18.0f, 600, Alpha(Hex(0xffffff), 0.7f)));
	// The latest alert, over the program's top.
	if (!St.Alerts.empty())
	{
		const kast::Alert& A = St.Alerts.front();
		const Rect Ar{Pg.X + Pg.W / 2.0f - 260.0f, Pg.Y + 20.0f, 520.0f, 64.0f};
		C.FillRoundRect(Ar, 12.0f, Paint::Solid(Alpha(Hex(0x1b1530), 0.92f)));
		C.StrokeRoundRect(Ar, 12.0f, Hex(0x9b8cff), 2.0f);
		C.Text(A.Who, Ar.X + Ar.W / 2.0f, Ar.Y + 30.0f, Ts(22.0f, 900, Lime, Align::Center, Baseline::Alphabetic, false, Ar.W - 30.0f));
		C.Text(A.Text, Ar.X + Ar.W / 2.0f, Ar.Y + 54.0f, Ts(15.0f, 500, pal::Ink, Align::Center, Baseline::Alphabetic, false, Ar.W - 30.0f));
	}

	// Viewers over the stream.
	const Rect Gr{40.0f, 676.0f, 960.0f, 170.0f};
	C.Text("VIEWERS", Gr.X, Gr.Y - 10.0f, Ts(13.0f, 800, pal::Muted));
	C.Text("peak " + std::to_string(St.Peak), Gr.X + Gr.W, Gr.Y - 10.0f, Ts(13.0f, 700, pal::Muted, Align::Right));
	std::vector<float> V(St.Graph.end() - std::min<ptrdiff_t>(static_cast<ptrdiff_t>(St.Graph.size()), 90), St.Graph.end());
	float Hi = 4.0f;
	for (float X : V)
	{
		Hi = std::max(Hi, X * 1.2f);
	}
	Graph(C, Gr, V, 0.0f, Hi, Lime);

	// Chat, newest at the bottom.
	const Rect Ch{1040.0f, 96.0f, W - 1080.0f, 750.0f};
	C.FillRoundRect(Ch, 10.0f, Paint::Solid(Hex(0x18181b)));
	C.Text("STREAM CHAT", Ch.X + 20.0f, Ch.Y + 36.0f, Ts(15.0f, 800, pal::Muted));
	C.PushClip({Ch.X, Ch.Y + 50.0f, Ch.W, Ch.H - 60.0f});
	float Y = Ch.Y + Ch.H - 22.0f;
	for (size_t I = St.Chat.size(); I-- > 0 && Y > Ch.Y + 60.0f;)
	{
		const kast::ChatMsg& M = St.Chat[I];
		if (M.Deleted)
		{
			continue;
		}
		const bool Named = M.Kind == kast::LineKind::Chat || M.Kind == kast::LineKind::Streamer || M.Kind == kast::LineKind::Question || M.Kind == kast::LineKind::Tip;
		float X = Ch.X + 20.0f;
		if (Named)
		{
			X += C.Text(M.Who + ":", X, Y, Ts(17.0f, 800, Hex(M.NameColor))) + 8.0f;
		}
		C.Text(M.Text, X, Y, Ts(17.0f, 500, Named ? pal::Ink : Hex(0xa78bfa), Align::Left, Baseline::Alphabetic, false, Ch.X + Ch.W - 20.0f - X));
		Y -= 30.0f;
	}
	C.PopClip();
}

// ------------------------------------------------------------------ study

struct Hand
{
	int Hi;
	int Lo;
	bool Suited;
};

/** The Chen formula: a quick score for a starting hand (ranks 2..14). */
double Chen(const Hand& Hd)
{
	auto Val = [](int R) { return R == 14 ? 10.0 : R == 13 ? 8.0 : R == 12 ? 7.0 : R == 11 ? 6.0 : R / 2.0; };
	double Sc = Val(Hd.Hi);
	if (Hd.Hi == Hd.Lo)
	{
		return std::max(5.0, Sc * 2.0);
	}
	if (Hd.Suited)
	{
		Sc += 2.0;
	}
	const int Gap = Hd.Hi - Hd.Lo - 1;
	Sc -= Gap == 0 ? 0.0 : Gap == 1 ? 1.0 : Gap == 2 ? 2.0 : Gap == 3 ? 4.0 : 5.0;
	if (Gap <= 1 && Hd.Hi < 12)
	{
		Sc += 1.0;
	}
	return std::ceil(Sc);
}

struct Position
{
	const char* Name;
	double Open; // share of hands opened
};
const Position Positions[5] = {{"UTG", 0.15}, {"HJ", 0.19}, {"CO", 0.27}, {"BTN", 0.43}, {"SB", 0.36}};

/** 0 fold, 1 mixed (the edge of the range), 2 raise; [row][col], row/col 0 = ace. */
std::array<std::array<int, 13>, 13> Chart(int Pos)
{
	struct Entry
	{
		int Row, Col;
		double Score;
		int Combos;
	};
	std::vector<Entry> All;
	for (int R = 0; R < 13; ++R)
	{
		for (int Cc = 0; Cc < 13; ++Cc)
		{
			const int A = 14 - R;
			const int B = 14 - Cc;
			const Hand Hd{std::max(A, B), std::min(A, B), Cc > R};
			const int Combos = R == Cc ? 6 : (Cc > R ? 4 : 12);
			// Ties: pairs, then suited, then the higher kicker.
			All.push_back({R, Cc, Chen(Hd) + (R == Cc ? 0.3 : 0.0) + (Hd.Suited ? 0.1 : 0.0) + Hd.Lo * 0.01, Combos});
		}
	}
	std::stable_sort(All.begin(), All.end(), [](const Entry& X, const Entry& Y) { return X.Score > Y.Score; });
	std::array<std::array<int, 13>, 13> Out{};
	const double Goal = Positions[Pos].Open * 1326.0;
	double Sum = 0.0;
	for (const Entry& E : All)
	{
		if (Sum >= Goal * 1.08)
		{
			break;
		}
		Out[E.Row][E.Col] = Sum < Goal * 0.94 ? 2 : 1;
		Sum += E.Combos;
	}
	return Out;
}

const char* RankChar(int Index)
{
	static const char* Ranks[13] = {"A", "K", "Q", "J", "T", "9", "8", "7", "6", "5", "4", "3", "2"};
	return Ranks[Index];
}

std::string TellPhrase(const std::string& Tell)
{
	static const std::map<std::string, std::string> Phrases = {
		{"ChipGlance", "the glance at the chips"}, {"BrowFlash", "the eyebrow flash"}, {"Swallow", "the dry swallow"}, {"LipPress", "the pressed lips"},
		{"Freeze", "going still"}, {"StareDown", "the stare"}, {"LookAway", "looking away"}, {"Tremble", "the shaking hands"},
		{"NeckTouch", "the hand to the neck"}, {"FalseSmile", "the mouth-only smile"}, {"RealSmile", "the real smile"}, {"BlinkBurst", "the blinking"},
		{"Sigh", "the big sigh"}, {"Recheck", "checking the cards again"}, {"PupilFlare", "the wide pupils"}, {"ChipReach", "reaching for chips"},
	};
	const auto It = Phrases.find(Tell);
	return It == Phrases.end() ? Tell : It->second;
}

void Study(Canvas& C, const Session& S, double Now)
{
	Backdrop(C);
	// The chart cycles through the positions; each change sweeps across the grid from the top left.
	const double Period = 9.0;
	const int Pos = static_cast<int>(std::fmod(Now / Period, 5.0));
	const int Prev = (Pos + 4) % 5;
	const double Since = std::fmod(Now, Period);
	static std::array<std::array<std::array<int, 13>, 13>, 5> Charts;
	static bool bCharts = false;
	if (!bCharts)
	{
		for (int P = 0; P < 5; ++P)
		{
			Charts[P] = Chart(P);
		}
		bCharts = true;
	}
	C.Text("PREFLOP \xC2\xB7 OPEN RAISE", 60.0f, 76.0f, Ts(18.0f, 800, pal::Muted));
	C.Text(std::string(Positions[Pos].Name) + " \xC2\xB7 " + std::to_string(static_cast<int>(Positions[Pos].Open * 100.0 + 0.5)) + "% of hands", 60.0f, 118.0f, Ts(32.0f, 800, pal::Ink));
	for (int P = 0; P < 5; ++P)
	{
		const float Px = 470.0f + static_cast<float>(P) * 80.0f;
		const bool On = P == Pos;
		C.FillRoundRect({Px, 90.0f, 70.0f, 34.0f}, 17.0f, Paint::Solid(On ? pal::Accent : pal::Panel2));
		C.Text(Positions[P].Name, Px + 35.0f, 113.0f, Ts(15.0f, 800, On ? Hex(0x04201c) : pal::Muted, Align::Center));
	}
	const float Cell = 52.0f;
	const float Gx = 60.0f;
	const float Gy = 150.0f;
	const Color Fold = Hex(0x1a2436);
	const Color Mixed = Hex(0xf28a3a);
	const Color Raise = Hex(0xe5484d);
	auto Shade = [&](int State, bool Pair) {
		const Color Base = State == 2 ? Raise : State == 1 ? Mixed : Fold;
		return Pair ? Mix(Base, Hex(0xffffff), State ? 0.08f : 0.03f) : Base;
	};
	for (int R = 0; R < 13; ++R)
	{
		for (int Cc = 0; Cc < 13; ++Cc)
		{
			const float T = static_cast<float>(Clamp01(Since * 3.0 - (R + Cc) * 0.06));
			const Color Col = Mix(Shade(Charts[Prev][R][Cc], R == Cc), Shade(Charts[Pos][R][Cc], R == Cc), T);
			const Rect Rc{Gx + static_cast<float>(Cc) * Cell, Gy + static_cast<float>(R) * Cell, Cell - 3.0f, Cell - 3.0f};
			C.FillRoundRect(Rc, 6.0f, Paint::Solid(Col));
			const int Hi = std::min(R, Cc);
			const int Lo = std::max(R, Cc);
			const std::string Label = std::string(RankChar(Hi)) + RankChar(Lo) + (R == Cc ? "" : (Cc > R ? "s" : "o"));
			C.Text(Label, Rc.X + Rc.W / 2.0f, Rc.Y + Rc.H / 2.0f + 6.0f, Ts(15.0f, 700, Charts[Pos][R][Cc] ? Hex(0xffffff) : pal::Muted, Align::Center));
		}
	}
	const float Ly = Gy + 13.0f * Cell + 30.0f;
	C.FillRoundRect({Gx, Ly - 16.0f, 20.0f, 20.0f}, 5.0f, Paint::Solid(Raise));
	C.Text("raise", Gx + 30.0f, Ly, Ts(16.0f, 600, pal::Muted));
	C.FillRoundRect({Gx + 120.0f, Ly - 16.0f, 20.0f, 20.0f}, 5.0f, Paint::Solid(Mixed));
	C.Text("sometimes", Gx + 150.0f, Ly, Ts(16.0f, 600, pal::Muted));
	C.FillRoundRect({Gx + 290.0f, Ly - 16.0f, 20.0f, 20.0f}, 5.0f, Paint::Solid(Fold));
	C.Text("fold", Gx + 320.0f, Ly, Ts(16.0f, 600, pal::Muted));

	// Reads: the tells the player has seen confirmed at showdown.
	const Rect Rd{800.0f, 60.0f, W - 860.0f, 560.0f};
	Panel(C, Rd);
	C.Text("READS", Rd.X + 28.0f, Rd.Y + 48.0f, Ts(18.0f, 800, pal::Gold));
	C.Text("from the back room", Rd.X + 110.0f, Rd.Y + 48.0f, Ts(16.0f, 500, pal::Muted));
	std::vector<std::pair<int, std::string>> Known;
	for (const auto& R : S.Life.Reads)
	{
		Known.push_back({R.second, R.first});
	}
	std::stable_sort(Known.begin(), Known.end(), [](const auto& X, const auto& Y) { return X.first > Y.first; });
	if (Known.empty())
	{
		C.Text("Nothing yet. Watch them at Dee's table:", Rd.X + 28.0f, Rd.Y + 110.0f, Ts(20.0f, 500, pal::Ink));
		C.Text("what they do with a big hand, and with nothing.", Rd.X + 28.0f, Rd.Y + 142.0f, Ts(20.0f, 500, pal::Muted));
	}
	float Y = Rd.Y + 108.0f;
	for (size_t I = 0; I < Known.size() && I < 8; ++I)
	{
		const std::string& Key = Known[I].second;
		const size_t Slash = Key.find('/');
		const std::string Who = Slash == std::string::npos ? Key : Key.substr(0, Slash);
		const std::string Tell = Slash == std::string::npos ? std::string() : TellPhrase(Key.substr(Slash + 1));
		const bool Learned = Known[I].first >= 2;
		C.FillCircle(Rd.X + 36.0f, Y - 7.0f, 6.0f, Paint::Solid(Learned ? pal::Accent : pal::Dim));
		const float Nx = Rd.X + 54.0f;
		const float Nw = C.Text(Who, Nx, Y, Ts(21.0f, 800, pal::Ink));
		C.Text(Tell, Nx + Nw + 12.0f, Y, Ts(20.0f, 500, Learned ? pal::Ink : pal::Muted, Align::Left, Baseline::Alphabetic, false, Rd.X + Rd.W - 120.0f - (Nx + Nw + 12.0f)));
		C.Text(Learned ? "read" : "seen " + std::to_string(Known[I].first) + "x", Rd.X + Rd.W - 28.0f, Y, Ts(16.0f, 700, Learned ? pal::Accent : pal::Dim, Align::Right));
		Y += 54.0f;
	}

	// Tonight at the tables.
	const Rect Tn{800.0f, 650.0f, W - 860.0f, 190.0f};
	Panel(C, Tn);
	C.Text("TONIGHT", Tn.X + 28.0f, Tn.Y + 44.0f, Ts(18.0f, 800, pal::Muted));
	C.Text(std::to_string(S.HandsPlayed), Tn.X + 28.0f, Tn.Y + 120.0f, Ts(56.0f, 800, pal::Ink));
	C.Text("hands played", Tn.X + 28.0f, Tn.Y + 160.0f, Ts(18.0f, 500, pal::Muted));
	C.Text(ClockString(S.ClockMinutes()), Tn.X + Tn.W - 28.0f, Tn.Y + 120.0f, Ts(40.0f, 700, pal::Ink, Align::Right));
	C.Text("and the rain hasn't stopped", Tn.X + Tn.W - 28.0f, Tn.Y + 160.0f, Ts(18.0f, 500, pal::Muted, Align::Right));
}
} // namespace secondscreen_detail

using namespace secondscreen_detail;

void Draw(Canvas& C, const Session& S, int Which, double Now)
{
	if (Which == 0)
	{
		if (S.Streaming())
		{
			Studio(C, S, Now);
		}
		else
		{
			Tracker(C, S, Now);
		}
	}
	else
	{
		Study(C, S, Now);
	}
}
} // namespace secondscreen
} // namespace ui
} // namespace ss
