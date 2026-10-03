// RiverLine Stats: a results tracker's dashboard on every player card (and the player's own, from the Career page).
// Profit, ROI and ITM up top; the profit graph with its peak, its worst downswing, the biggest score and the
// bracelets and rings where they were won; ROI by buy-in and by format; where the cashes finished; the records.
// Chart colours are a validated pair (profit teal, loss red; both clear 3:1 on the panel and stay apart under
// colour-blindness), always with a sign beside them.
#include "ShortStack/UI/RiverLine.h"
#include "../StrictFloat.h"
#include "RiverLineShared.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/UI/EventArt.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace ss
{
namespace ui
{
using namespace rlnet_detail;

namespace rlstats_detail
{
const Color StProfit = Hex(0x1aa99c);
const Color StLoss = Hex(0xe5484d);
const Color StBar = Hex(0x3987e5);
const Color StGold = Hex(0xf2c14e);
const Color StInk = Hex(0xe6edf7);
const Color StSoft = Hex(0xc3cedf);
const Color StGrid = Hex(0x1a2a42);
const Color StAxis = Hex(0x2e4361);
const Color StPanel = Hex(0x0d1828);
const Color StEdge = Hex(0x223352);
const char* const StMinus = "\xE2\x88\x92";

/** Money the way a tracker writes it: cents under $1,000, short above. */
std::string StAmount(Chips C)
{
	const Chips A = C < 0 ? -C : C;
	return A < 100000 ? NetMoney(A) : net::MoneyShort(A);
}

std::string StSigned(Chips C)
{
	return (C < 0 ? std::string(StMinus) : C > 0 ? std::string("+") : std::string()) + StAmount(C);
}

std::string StPct(double V, int Digits = 1)
{
	return Fixed(std::fabs(V) * 100.0, Digits) + "%";
}

std::string StSignedPct(double V, int Digits = 1)
{
	return (V < -0.0005 ? std::string(StMinus) : V > 0.0005 ? std::string("+") : std::string()) + StPct(V, Digits);
}

std::string StTally(int N)
{
	return N >= 100000 ? Fixed(N / 1000.0, 0) + "K" : Grouped(N);
}

/** A clean step for about Ticks gridlines over Span (1, 2, 2.5 or 5 times a power of ten). */
double StNiceStep(double Span, int Ticks)
{
	const double Raw = std::max(1e-9, Span / std::max(1, Ticks));
	const double Pow = std::pow(10.0, std::floor(std::log10(Raw)));
	const double F = Raw / Pow;
	return Pow * (F <= 1.0 ? 1.0 : F <= 2.0 ? 2.0 : F <= 2.5 ? 2.5 : F <= 5.0 ? 5.0 : 10.0);
}

/** An axis label: "$0", "$250K", "−$50K", "20K". */
std::string StAxisMoney(double Dollars)
{
	const double A = std::fabs(Dollars);
	if (A < 0.5)
	{
		return "$0";
	}
	const auto Trim = [](double V) {
		std::string S = Fixed(V, 1);
		return S.size() > 2 && S.compare(S.size() - 2, 2, ".0") == 0 ? S.substr(0, S.size() - 2) : S;
	};
	const std::string Body = A >= 1e6 ? Trim(A / 1e6) + "M" : A >= 1e3 ? Trim(A / 1e3) + "K" : Grouped(static_cast<int64_t>(std::llround(A)));
	return (Dollars < 0.0 ? std::string(StMinus) : std::string()) + "$" + Body;
}

std::string StAxisCount(double N)
{
	const long long V = std::llround(N);
	return V >= 10000 && V % 1000 == 0 ? Grouped(V / 1000) + "K" : Grouped(V);
}

/** A bar growing from a baseline at X0 to X1 (either way): rounded at the data end, square at the baseline. */
void StBarMark(Canvas& Cv, float X0, float X1, float Y, float H, const Color& Col)
{
	const float L = std::min(X0, X1);
	const float W = std::fabs(X1 - X0);
	if (W < 0.5f)
	{
		return;
	}
	const float Rr = std::min(4.0f, W * 0.5f);
	Cv.FillRoundRect({L, Y, W, H}, Rr, Col);
	// Square the baseline end.
	const float Sq = std::min(W, Rr);
	Cv.FillRect({X1 >= X0 ? X0 : X0 - Sq, Y, Sq, H}, Col);
}

/** A column from a baseline at Y0 to Y1 (up or down): rounded at the data end, square at the baseline. */
void StColumnMark(Canvas& Cv, float X, float Y0, float Y1, float W, const Color& Col)
{
	const float T = std::min(Y0, Y1);
	const float H = std::fabs(Y1 - Y0);
	if (H < 0.5f)
	{
		return;
	}
	const float Rr = std::min(4.0f, H * 0.5f);
	Cv.FillRoundRect({X, T, W, H}, Rr, Col);
	const float Sq = std::min(H, Rr);
	Cv.FillRect({X, Y1 <= Y0 ? Y0 - Sq : Y0, W, Sq}, Col);
}

/** An annotation on the graph: small text on a dark pill so it reads over the line and the wash. */
Rect StNote(Canvas& Cv, const std::string& Text, float X, float Y, bool Draw)
{
	TextStyle St = Ts(10.5f, 700, StInk);
	const float W = Cv.Measure(Text, 10.5f, 700) + 12.0f;
	const Rect Box{X - 6.0f, Y - 12.0f, W, 17.0f};
	if (Draw)
	{
		Cv.FillRoundRect(Box, 8.5f, Rgba(10, 19, 33, 0.88f));
		Cv.Text(Text, X, Y, St);
	}
	return Box;
}

bool StOverlap(const Rect& A, const Rect& B)
{
	return A.X < B.X + B.W && B.X < A.X + A.W && A.Y < B.Y + B.H && B.Y < A.Y + A.H;
}

/** The panels' plate. */
void StPlate(Canvas& Cv, const Rect& R)
{
	Cv.FillRoundRect(R, 14.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, Hex(0x0f1c2f), StPanel));
	Cv.StrokeRoundRect(R, 14.0f, StEdge, 1.0f);
}

/** A tooltip: the value first (strong), what it is after (muted). */
void StTip(Canvas& Cv, Ui& U, float X, float Y, const Rect& Within, const std::string& Value, const std::vector<std::string>& Lines)
{
	float W = U.Measure(Value, 15.0f, 800) + 28.0f;
	for (const std::string& L : Lines)
	{
		W = std::max(W, U.Measure(L, 12.0f, 600) + 28.0f);
	}
	const float H = 34.0f + 18.0f * static_cast<float>(Lines.size());
	float Tx = X + 16.0f;
	float Ty = Y - H - 10.0f;
	Tx = Tx + W > Within.X + Within.W - 6.0f ? X - W - 16.0f : Tx;
	Ty = Ty < Within.Y + 6.0f ? Y + 14.0f : Ty;
	Cv.GlowRoundRect({Tx, Ty + 4.0f, W, H}, 10.0f, Rgba(0, 0, 0, 0.5f), 18.0f);
	Cv.FillRoundRect({Tx, Ty, W, H}, 10.0f, Hex(0x15253d));
	Cv.StrokeRoundRect({Tx, Ty, W, H}, 10.0f, Hex(0x2e4361), 1.0f);
	Cv.Text(Value, Tx + 14.0f, Ty + 24.0f, Ts(15.0f, 800, StInk));
	for (size_t K = 0; K < Lines.size(); ++K)
	{
		Cv.Text(Lines[K], Tx + 14.0f, Ty + 43.0f + 18.0f * static_cast<float>(K), Ts(12.0f, 600, StSoft));
	}
}

/** The net at tournament E on the graph's points (straight between them). */
double StNetAt(const std::vector<std::pair<double, double>>& Pts, double E)
{
	if (Pts.empty())
	{
		return 0.0;
	}
	for (size_t K = 1; K < Pts.size(); ++K)
	{
		if (Pts[K].first >= E)
		{
			const double Span = Pts[K].first - Pts[K - 1].first;
			const double T = Span > 0.0 ? (E - Pts[K - 1].first) / Span : 1.0;
			return Pts[K - 1].second + (Pts[K].second - Pts[K - 1].second) * T;
		}
	}
	return Pts.back().second;
}

const char* const StStakeNames[world::TrackStakeCount] = {"Micro", "Low", "Mid", "High", "Live"};
const char* const StStakeLong[world::TrackStakeCount] = {"Micro stakes online", "Low stakes online", "Mid stakes online", "High stakes online", "Live tournaments"};
const char* const StFormatNames[world::TrackFormatCount] = {"Regular", "Deepstack", "Turbo", "Hyper", "Bounty", "Satellite"};
const char* const StFormatShort[world::TrackFormatCount] = {"Regular", "Deep", "Turbo", "Hyper", "Bounty", "Sats"};
} // namespace rlstats_detail

using namespace rlstats_detail;

void RiverLine::StatsKpis(const world::Tracker& T, const Rect& R, double Now)
{
	const float In = NetEase((Now - CardTabAt) / 0.5);
	const world::World* Wd = net::Shared().Attached();
	const double Roi = T.Roi();
	struct Kpi
	{
		const char* Label;
		std::string Value;
		std::string Sub;
		int Sign; // +1 good, -1 bad, 0 neither
	};
	const double Rank = Wd && T.Events >= 200 ? Wd->RoiRank(Roi) : -1.0;
	const int Place = Wd && T.Events >= 200 ? Wd->RoiPlace(Roi, &T == &Wd->HeroStats()) : 0;
	const int Own = static_cast<int>(std::max_element(T.ByStake.begin(), T.ByStake.end(), [](const world::TrackLine& A, const world::TrackLine& B) { return A.Events < B.Events; }) - T.ByStake.begin());
	const Kpi Tiles[6] = {
		{"PROFIT", StSigned(static_cast<Chips>(static_cast<double>(T.Net) * In)), "Prizes " + StAmount(T.Prizes) + "  \xC2\xB7  buy-ins " + StAmount(T.BuyIns), T.Net > 0 ? 1 : T.Net < 0 ? -1 : 0},
		{"ROI", StSignedPct(Roi * In, std::fabs(Roi) >= 1.0 ? 0 : 1),
			Place >= 1 && Place <= 10 ? "No. " + std::to_string(Place) + " on RiverLine" : Rank >= 0.0 ? "Beats " + Fixed(std::min(Rank * 100.0, 99.0), 0) + "% of regulars" : Grouped(T.Events) + " tournaments in",
			Roi > 0.0005 ? 1 : Roi < -0.0005 ? -1 : 0},
		{"ITM", StPct(T.Itm() * In), Grouped(T.Cashes) + (T.Cashes == 1 ? " cash" : " cashes"), 0},
		{"TOURNAMENTS", Grouped(static_cast<int>(std::lround(static_cast<double>(T.Events) * static_cast<double>(In)))), Grouped(T.Wins) + (T.Wins == 1 ? " win" : " wins") + "  \xC2\xB7  " + Grouped(T.FinalTables) + " FTs", 0},
		{"ABI", StAmount(T.AverageBuyIn()), T.Events > 0 ? std::string(Own == static_cast<int>(world::TrackStake::Live) ? "Average buy-in, mostly live" : "Average buy-in") : std::string("\xE2\x80\x94"), 0},
		{"AVG FINISH", T.Finished > 0 ? "Top " + Fixed(std::max(1.0, T.AverageFinish() * 100.0), 0) + "%" : std::string("\xE2\x80\x94"), "of the field", 0},
	};
	const float Gap = 10.0f;
	const float Hero = 248.0f;
	const float Tw = (R.W - Hero - 5.0f * Gap) / 5.0f;
	float X = R.X;
	for (int K = 0; K < 6; ++K)
	{
		const float W = K == 0 ? Hero : Tw;
		const Rect Tr{X, R.Y, W, R.H};
		C->FillRoundRect(Tr, 12.0f, Paint::Linear({0.0f, Tr.Y}, {0.0f, Tr.Y + Tr.H}, Hex(0x112036), Hex(0x0d1828)));
		C->StrokeRoundRect(Tr, 12.0f, StEdge, 1.0f);
		const Kpi& Kp = Tiles[K];
		NetSpaced(*C, Kp.Label, Tr.X + 16.0f, Tr.Y + 24.0f, 9.5f, 800, pal::Muted, 1.4f);
		float Vx = Tr.X + 16.0f;
		const float Vy = Tr.Y + (K == 0 ? 70.0f : 62.0f);
		if (Kp.Sign != 0)
		{
			// The direction, in colour and in shape (never colour alone): a triangle beside the signed value.
			const Color Dc = Kp.Sign > 0 ? StProfit : StLoss;
			const float Ts0 = K == 0 ? 9.0f : 7.0f;
			NetTriangle(*C, Vx + Ts0, Vy - (K == 0 ? 15.0f : 10.0f), Ts0 * 2.0f, Kp.Sign > 0, Dc);
			Vx += Ts0 * 2.0f + 8.0f;
		}
		UI.Text(Kp.Value, Vx, Vy, Ts(K == 0 ? 40.0f : 26.0f, 800, StInk, Align::Left, Baseline::Alphabetic, false, Tr.X + W - Vx - 12.0f));
		UI.Text(Kp.Sub, Tr.X + 16.0f, Tr.Y + R.H - 14.0f, Ts(11.5f, 600, StSoft, Align::Left, Baseline::Alphabetic, false, W - 26.0f));
		X += W + Gap;
	}
}

void RiverLine::StatsBoard(const world::Tracker& T, const std::vector<world::Award>& Awards, const Rect& R, double Now)
{
	const float In = NetEase((Now - CardTabAt) / 0.9);
	if (T.Events == 0)
	{
		StPlate(*C, R);
		UI.Text("No tournaments on record yet.", R.X + R.W / 2.0f, R.Y + R.H / 2.0f - 6.0f, Ts(18.0f, 700, StInk, Align::Center));
		UI.Text("Every buy-in and every cash shows up here: profit, ROI, ITM and the graph.", R.X + R.W / 2.0f, R.Y + R.H / 2.0f + 20.0f, Ts(13.0f, 500, pal::Muted, Align::Center));
		return;
	}
	const float Gap = 14.0f;
	const float Lw = 640.0f;
	const Rect Pg{R.X, R.Y, Lw, 292.0f};
	const Rect Rs{R.X, Pg.Y + Pg.H + Gap, (Lw - Gap) / 2.0f, R.Y + R.H - (Pg.Y + Pg.H + Gap)};
	const Rect Rf{Rs.X + Rs.W + Gap, Rs.Y, Rs.W, Rs.H};
	const Rect Fn{R.X + Lw + Gap, R.Y, R.W - Lw - Gap, 236.0f};
	const Rect Rc{Fn.X, Fn.Y + Fn.H + Gap, Fn.W, R.Y + R.H - (Fn.Y + Fn.H + Gap)};

	// ---------------------------------------------------------------- the profit graph
	StPlate(*C, Pg);
	{
		// The graph's two views: profit, or the ABI (how their buy-ins moved).
		const char* const Views[2] = {"PROFIT", "ABI"};
		float Tx = Pg.X + Pg.W - 14.0f;
		for (int K = 1; K >= 0; --K)
		{
			const float Tw = UI.Measure(Views[K], 10.5f, 800) + 22.0f;
			Tx -= Tw;
			const Rect Tr{Tx, Pg.Y + 12.0f, Tw, 22.0f};
			const Ui::ClickState St = UI.Clickable(std::string("statsgraph") + Views[K], Tr, CardOpen());
			if (St.Clicked && StatsGraph != K)
			{
				StatsGraph = K;
				CardTabAt = Now;
			}
			const bool On = StatsGraph == K;
			UI.RRect(Tr, 11.0f, On ? NetA(pal::Accent, 0.18f) : St.Hover ? Hex(0x172a42) : Rgba(255, 255, 255, 0.02f), On ? pal::Accent : StEdge);
			NetSpaced(*C, Views[K], Tr.X + Tw / 2.0f, Tr.Y + 15.0f, 10.5f, 800, On ? StInk : pal::Muted, 1.2f, Align::Center);
			Tx -= 6.0f;
		}
		NetSpaced(*C, StatsGraph == 1 ? "AVERAGE BUY-IN" : "PROFIT", Pg.X + 18.0f, Pg.Y + 28.0f, 11.0f, 800, pal::Muted, 1.6f);
		UI.Text(StatsGraph == 1 ? Grouped(T.Events) + " tournaments  \xC2\xB7  every bullet" : Grouped(T.Events) + " tournaments  \xC2\xB7  prizes after buy-ins", Tx - 10.0f, Pg.Y + 28.0f,
			Ts(11.5f, 600, pal::Muted, Align::Right));
	}
	const Rect Plot{Pg.X + 70.0f, Pg.Y + 48.0f, Pg.W - 70.0f - 84.0f, Pg.H - 48.0f - 34.0f};
	if (StatsGraph == 1)
	{
		StatsAbi(T, Pg, Plot, Now);
	}
	else
	{
		std::vector<std::pair<double, double>> Pts;
		Pts.push_back({0.0, 0.0});
		for (size_t K = 0; K < T.Curve.size(); ++K)
		{
			Pts.push_back({static_cast<double>((K + 1) * static_cast<size_t>(T.Stride)), static_cast<double>(T.Curve[K]) / 100.0});
		}
		if (Pts.back().first < static_cast<double>(T.Events))
		{
			Pts.push_back({static_cast<double>(T.Events), static_cast<double>(T.Net) / 100.0});
		}
		double Lo = 0.0;
		double Hi = 0.0;
		for (const auto& Q : Pts)
		{
			Lo = std::min(Lo, Q.second);
			Hi = std::max(Hi, Q.second);
		}
		Hi = std::max(Hi, static_cast<double>(T.Peak) / 100.0);
		if (Hi - Lo < 1.0)
		{
			Hi = Lo + 1.0;
		}
		const double Step = StNiceStep((Hi - Lo) * 1.1, 4);
		Lo = std::floor(Lo / Step) * Step;
		Hi = std::ceil(Hi * 1.06 / Step) * Step;
		const double Events = static_cast<double>(std::max(1, T.Events));
		auto Xof = [&](double E) { return Plot.X + Plot.W * Nf(E / Events); };
		auto Yof = [&](double V) { return Plot.Y + Plot.H * Nf((Hi - V) / (Hi - Lo)); };
		// Gridlines and their labels (solid hairlines, one step off the panel).
		for (double V = Lo; V <= Hi + Step * 0.5; V += Step)
		{
			const float Y = Yof(V);
			C->FillRect({Plot.X, Y, Plot.W, 1.0f}, std::fabs(V) < Step * 0.01 ? StAxis : StGrid);
			UI.Text(StAxisMoney(V), Plot.X - 10.0f, Y + 4.0f, Ts(11.0f, 600, pal::Muted, Align::Right, Baseline::Alphabetic, true));
		}
		{
			const double Xs = StNiceStep(Events, 5);
			for (double E = 0.0; E <= Events + 0.5; E += Xs)
			{
				const float X = Xof(E);
				C->FillRect({X, Plot.Y + Plot.H, 1.0f, 5.0f}, StAxis);
				UI.Text(StAxisCount(E), X, Plot.Y + Plot.H + 20.0f, Ts(11.0f, 600, pal::Muted, Align::Center, Baseline::Alphabetic, true));
			}
		}
		// The worst downswing: a band from the peak to the bottom, named at the top (the note goes on with the others).
		std::string DownText;
		float DownX = 0.0f;
		if (T.Downswing > 0 && T.DownTo > T.DownFrom)
		{
			const float X0 = Xof(T.DownFrom);
			const float X1 = Xof(T.DownTo);
			if (X1 - X0 >= 4.0f)
			{
				C->FillRect({X0, Plot.Y, X1 - X0, Plot.H}, NetA(StLoss, 0.07f * In));
				C->FillRect({X0, Plot.Y, X1 - X0, 2.0f}, NetA(StLoss, 0.7f * In));
				DownText = "Downswing " + std::string(StMinus) + StAmount(T.Downswing);
				const float Lw0 = UI.Measure(DownText, 10.5f, 700);
				DownX = std::min(std::max(X0 + (X1 - X0) / 2.0f - Lw0 / 2.0f, Plot.X + 8.0f), Plot.X + Plot.W - Lw0 - 8.0f);
			}
		}
		// The line, revealed left to right: teal above zero, red below, each with a light wash to the zero line.
		{
			std::vector<std::pair<float, double>> Path; // x, value (zero crossings put in)
			for (size_t K = 0; K < Pts.size(); ++K)
			{
				if (K > 0 && ((Pts[K - 1].second < 0.0) != (Pts[K].second < 0.0)) && Pts[K - 1].second != 0.0 && Pts[K].second != 0.0)
				{
					const double T0 = Pts[K - 1].second / (Pts[K - 1].second - Pts[K].second);
					Path.push_back({Xof(Pts[K - 1].first + (Pts[K].first - Pts[K - 1].first) * T0), 0.0});
				}
				Path.push_back({Xof(Pts[K].first), Pts[K].second});
			}
			const float Y0 = Yof(0.0);
			C->PushClip({Plot.X - 6.0f, Pg.Y, (Plot.W + 12.0f) * In, Pg.H});
			std::vector<Vec2> Up{{Path.front().first, Y0}};
			std::vector<Vec2> Down{{Path.front().first, Y0}};
			for (const auto& Q : Path)
			{
				Up.push_back({Q.first, Yof(std::max(0.0, Q.second))});
				Down.push_back({Q.first, Yof(std::min(0.0, Q.second))});
			}
			Up.push_back({Path.back().first, Y0});
			Down.push_back({Path.back().first, Y0});
			C->FillPolygon(Up, Paint::Linear({0.0f, Plot.Y}, {0.0f, Y0}, NetA(StProfit, 0.24f), NetA(StProfit, 0.02f)));
			C->FillPolygon(Down, Paint::Linear({0.0f, Y0}, {0.0f, Plot.Y + Plot.H}, NetA(StLoss, 0.02f), NetA(StLoss, 0.24f)));
			// The line in runs by sign.
			std::vector<Vec2> Run;
			bool Neg = Path.front().second < 0.0;
			for (size_t K = 0; K < Path.size(); ++K)
			{
				const bool N = Path[K].second < 0.0 || (Path[K].second == 0.0 && K + 1 < Path.size() && Path[K + 1].second < 0.0);
				const Vec2 P{Path[K].first, Yof(Path[K].second)};
				if (!Run.empty() && N != Neg && Path[K].second == 0.0)
				{
					Run.push_back(P);
					C->StrokePolyline(Run, false, Neg ? StLoss : StProfit, 2.0f, true);
					Run.clear();
					Neg = N;
				}
				Run.push_back(P);
			}
			if (Run.size() >= 2)
			{
				C->StrokePolyline(Run, false, Neg ? StLoss : StProfit, 2.0f, true);
			}
			// The peak, the biggest score, the titles: marked where they happened (labels only where there is room).
			const auto Dot = [&](float X, float Y, const Color& Col, float Rr) {
				C->FillCircle(X, Y, Rr + 2.0f, StPanel);
				C->FillCircle(X, Y, Rr, Col);
			};
			// Where the titles sit (their icons) and the downswing's note, so the other notes keep clear of them.
			std::vector<Rect> Taken;
			if (!DownText.empty())
			{
				Taken.push_back(StNote(*C, DownText, DownX, Plot.Y + 18.0f, true));
			}
			for (const world::Award& A : Awards)
			{
				if (A.Tourney > 0 && A.Tourney <= T.Events)
				{
					const float X = Xof(A.Tourney);
					const float Iy = std::max(Plot.Y + 14.0f, Yof(StNetAt(Pts, A.Tourney)) - 30.0f);
					Taken.push_back({X - 16.0f, Iy - 16.0f, 32.0f, 32.0f});
				}
			}
			const auto Place = [&](const std::string& Text, float X, float Y, bool Above) {
				const float Tw = C->Measure(Text, 10.5f, 700);
				const float Lx = std::min(std::max(X - Tw / 2.0f, Plot.X + 8.0f), Plot.X + Plot.W - Tw - 8.0f);
				const float Tries[2] = {Above ? Y - 12.0f : Y + 22.0f, Above ? Y + 22.0f : Y - 12.0f};
				for (float Ly : Tries)
				{
					if (Ly < Plot.Y + 14.0f || Ly > Plot.Y + Plot.H - 4.0f)
					{
						continue;
					}
					const Rect Box = StNote(*C, Text, Lx, Ly, false);
					bool Clear = true;
					for (const Rect& Q : Taken)
					{
						Clear = Clear && !StOverlap(Box, Q);
					}
					if (Clear)
					{
						StNote(*C, Text, Lx, Ly, true);
						Taken.push_back(Box);
						return;
					}
				}
			};
			float BestX = -1000.0f;
			float BestY = 0.0f;
			if (T.Best > 0 && T.BestAt > 0 && T.BestAt <= T.Events)
			{
				BestX = Xof(T.BestAt);
				BestY = Yof(StNetAt(Pts, T.BestAt));
				C->FillRect({BestX, BestY + 6.0f, 1.0f, std::max(0.0f, Plot.Y + Plot.H - BestY - 6.0f)}, NetA(StGold, 0.35f));
				Dot(BestX, BestY, StGold, 4.5f);
			}
			if (T.Peak > 0 && T.PeakAt > 0 && std::fabs(Xof(T.PeakAt) - BestX) > 90.0f)
			{
				const float X = Xof(T.PeakAt);
				const float Y = Yof(static_cast<double>(T.Peak) / 100.0);
				Dot(X, Y, StInk, 3.5f);
				Place("Peak " + StSigned(T.Peak), X, Y, true);
			}
			if (BestX > -1000.0f)
			{
				Place("Biggest score " + StAmount(T.Best), BestX, BestY, BestY < Plot.Y + Plot.H * 0.4f);
			}
			for (const world::Award& A : Awards)
			{
				if (A.Tourney <= 0 || A.Tourney > T.Events)
				{
					continue;
				}
				const float X = Xof(A.Tourney);
				const float Y = Yof(StNetAt(Pts, A.Tourney));
				const float Iy = std::max(Plot.Y + 14.0f, Y - 30.0f);
				C->FillRect({X, Iy + 10.0f, 1.0f, std::max(0.0f, Y - Iy - 14.0f)}, NetA(StGold, 0.6f));
				Dot(X, Y, StGold, 3.5f);
				eventart::Trophy(*C, A, X, Iy, 28.0f, Now);
			}
			const float Ex = Path.back().first;
			const float Ey = Yof(Path.back().second);
			Dot(Ex, Ey, T.Net < 0 ? StLoss : StProfit, 4.5f);
			C->PopClip();
			if (In > 0.98f)
			{
				UI.Text(StSigned(T.Net), Ex + 12.0f, Ey + 5.0f, Ts(13.5f, 800, StInk));
			}
		}
		// Hover: a crosshair that finds the nearest point, and what it says.
		if (UI.Ptr.Active && UI.Hover(Plot) && CardOpen())
		{
			const float Px = UI.Ptr.X;
			size_t Near = 0;
			for (size_t K = 1; K < Pts.size(); ++K)
			{
				Near = std::fabs(Xof(Pts[K].first) - Px) < std::fabs(Xof(Pts[Near].first) - Px) ? K : Near;
			}
			const float X = Xof(Pts[Near].first);
			const float Y = Yof(Pts[Near].second);
			C->FillRect({X, Plot.Y, 1.0f, Plot.H}, Hex(0x4a6280));
			C->FillCircle(X, Y, 6.0f, StPanel);
			C->FillCircle(X, Y, 4.5f, Pts[Near].second < 0.0 ? StLoss : StProfit);
			StTip(*C, UI, X, Y, Pg, StSigned(static_cast<Chips>(std::llround(Pts[Near].second * 100.0))),
				Near == 0 ? std::vector<std::string>{"The start"}
					  : std::vector<std::string>{"After " + Grouped(static_cast<int64_t>(Pts[Near].first)) + " tournaments", "ABI " + StAmount(T.StretchAbi(Near - 1)) + " over that stretch"});
		}

	}

	// ---------------------------------------------------------------- ROI by buy-in and by format
	// Columns around a zero line placed by the data: up for a profit, down for a loss, the ROI on each cap.
	const auto Columns = [&](const Rect& P, const char* Title, const char* const* Names, const char* const* Long, const world::TrackLine* Lines, int N) {
		StPlate(*C, P);
		NetSpaced(*C, Title, P.X + 18.0f, P.Y + 26.0f, 11.0f, 800, pal::Muted, 1.6f);
		UI.Text("ROI", P.X + P.W - 18.0f, P.Y + 26.0f, Ts(11.0f, 600, pal::Muted, Align::Right));
		std::vector<double> Roi(static_cast<size_t>(N), 0.0);
		double Lo0 = 0.0;
		double Hi0 = 0.0;
		for (int K = 0; K < N; ++K)
		{
			if (Lines[K].Events > 0 && Lines[K].BuyIns > 0)
			{
				Roi[static_cast<size_t>(K)] = static_cast<double>(Lines[K].Prizes - Lines[K].BuyIns) / static_cast<double>(Lines[K].BuyIns);
				const double Shown = std::max(-1.0, std::min(3.0, Roi[static_cast<size_t>(K)]));
				Lo0 = std::min(Lo0, Shown);
				Hi0 = std::max(Hi0, Shown);
			}
		}
		if (Hi0 - Lo0 < 0.2)
		{
			Hi0 += 0.1;
			Lo0 -= Lo0 < 0.0 ? 0.1 : 0.0;
		}
		// The columns stay clear of the header and of the names below, with room for each cap's label.
		const float Top = P.Y + 56.0f;
		const float Bottom = P.Y + P.H - 56.0f;
		const float Ph = Bottom - Top;
		const float Y0 = Top + Ph * Nf(Hi0 / (Hi0 - Lo0));
		const float Slot = (P.W - 32.0f) / Nf(N);
		C->FillRect({P.X + 16.0f, Y0, P.W - 32.0f, 1.0f}, StAxis);
		int Hovered = -1;
		for (int K = 0; K < N; ++K)
		{
			const world::TrackLine& L = Lines[K];
			const float Cx = P.X + 16.0f + Slot * (Nf(K) + 0.5f);
			const bool Any = L.Events > 0 && L.BuyIns > 0;
			UI.Text(Names[K], Cx, P.Y + P.H - 24.0f, Ts(11.5f, 700, Any ? StInk : pal::Dim, Align::Center));
			UI.Text(Any ? StTally(L.Events) : std::string("\xE2\x80\x94"), Cx, P.Y + P.H - 10.0f, Ts(10.0f, 600, pal::Muted, Align::Center, Baseline::Alphabetic, true));
			if (!Any)
			{
				continue;
			}
			const double R0 = Roi[static_cast<size_t>(K)];
			const double Shown = std::max(-1.0, std::min(3.0, R0));
			const float Ye = Y0 - (Y0 - (Shown >= 0.0 ? Top : Bottom)) * Nf(std::fabs(Shown) / (Shown >= 0.0 ? Hi0 : -Lo0)) * In;
			const bool Hov = UI.Hover({Cx - Slot / 2.0f, P.Y + 36.0f, Slot, P.H - 36.0f}) && CardOpen();
			Hovered = Hov ? K : Hovered;
			const Color Col = R0 < 0.0 ? StLoss : StProfit;
			StColumnMark(*C, Cx - 10.0f, Y0, Ye, 20.0f, Hov ? Mix(Col, Hex(0xffffff), 0.2f) : Col);
			UI.Text(StSignedPct(R0, 0), Cx, R0 < 0.0 ? Ye + 14.0f : Ye - 6.0f, Ts(11.0f, 700, StInk, Align::Center, Baseline::Alphabetic, true));
		}
		if (Hovered >= 0)
		{
			const world::TrackLine& L = Lines[Hovered];
			StTip(*C, UI, UI.Ptr.X, UI.Ptr.Y, R, "ROI " + StSignedPct(Roi[static_cast<size_t>(Hovered)]),
				{std::string(Long[Hovered]) + "  \xC2\xB7  " + Grouped(L.Events) + " tournaments",
					"ABI " + StAmount(L.BuyIns / std::max(1, L.Events)) + "  \xC2\xB7  ITM " + StPct(static_cast<double>(L.Cashes) / std::max(1, L.Events)),
					"Profit " + StSigned(L.Prizes - L.BuyIns)});
		}
	};
	Columns(Rs, "BY BUY-IN", StStakeNames, StStakeLong, T.ByStake.data(), world::TrackStakeCount);
	Columns(Rf, "BY FORMAT", StFormatShort, StFormatNames, T.ByFormat.data(), world::TrackFormatCount);

	// ---------------------------------------------------------------- finishes
	StPlate(*C, Fn);
	NetSpaced(*C, "FINISHES", Fn.X + 18.0f, Fn.Y + 26.0f, 11.0f, 800, pal::Muted, 1.6f);
	{
		const double Itm = T.Itm();
		UI.Text("In the money", Fn.X + 18.0f, Fn.Y + 56.0f, Ts(13.0f, 700, StSoft));
		UI.Text(StPct(Itm), Fn.X + Fn.W - 18.0f, Fn.Y + 58.0f, Ts(22.0f, 800, StInk, Align::Right));
		const Rect Track{Fn.X + 18.0f, Fn.Y + 70.0f, Fn.W - 36.0f, 8.0f};
		C->FillRoundRect(Track, 4.0f, NetA(StProfit, 0.18f));
		C->FillRoundRect({Track.X, Track.Y, std::max(8.0f, Track.W * Nf(Itm) * In), Track.H}, 4.0f, StProfit);
		UI.Text(Grouped(T.Cashes) + " of " + Grouped(T.Events) + " tournaments", Fn.X + 18.0f, Fn.Y + 96.0f, Ts(11.5f, 600, pal::Muted));
		// Where the cashes finished.
		const int Ft = std::max(0, T.FinalTables - T.Wins - T.Podiums);
		const int Other = std::max(0, T.Cashes - T.FinalTables);
		const std::pair<const char*, int> Parts[4] = {{"Won", T.Wins}, {"2nd or 3rd", T.Podiums}, {"Final table", Ft}, {"Other cashes", Other}};
		const int Cashes = std::max(1, T.Cashes);
		int Most = 1;
		for (const auto& Pt : Parts)
		{
			Most = std::max(Most, Pt.second);
		}
		const float Top = Fn.Y + 116.0f;
		const float Row = (Fn.Y + Fn.H - 12.0f - Top) / 4.0f;
		const float Bx = Fn.X + 112.0f;
		const float Bw = Fn.W - 112.0f - 116.0f;
		for (int K = 0; K < 4; ++K)
		{
			const float Cy = Top + Row * Nf(K) + Row / 2.0f;
			UI.Text(Parts[K].first, Fn.X + 18.0f, Cy + 4.5f, Ts(12.5f, 700, StInk));
			const float Len = Bw * Nf(static_cast<double>(Parts[K].second) / Most) * In;
			const bool Hov = UI.Hover({Fn.X + 8.0f, Cy - Row / 2.0f, Fn.W - 16.0f, Row}) && CardOpen();
			StBarMark(*C, Bx, Bx + std::max(Parts[K].second > 0 ? 3.0f : 0.0f, Len), Cy - 6.0f, 12.0f, Hov ? Mix(StBar, Hex(0xffffff), 0.2f) : StBar);
			UI.Text(Grouped(Parts[K].second), Fn.X + Fn.W - 58.0f, Cy + 4.5f, Ts(12.0f, 700, StInk, Align::Right, Baseline::Alphabetic, true));
			UI.Text(StPct(static_cast<double>(Parts[K].second) / Cashes, 0), Fn.X + Fn.W - 18.0f, Cy + 4.5f, Ts(11.5f, 600, pal::Muted, Align::Right, Baseline::Alphabetic, true));
			if (Hov)
			{
				StTip(*C, UI, UI.Ptr.X, UI.Ptr.Y, R, Grouped(Parts[K].second) + " " + Parts[K].first,
					{StPct(static_cast<double>(Parts[K].second) / Cashes) + " of their cashes", StPct(static_cast<double>(Parts[K].second) / std::max(1, T.Events), 2) + " of all their tournaments"});
			}
		}
	}

	// ---------------------------------------------------------------- records
	StPlate(*C, Rc);
	NetSpaced(*C, "RECORDS", Rc.X + 18.0f, Rc.Y + 26.0f, 11.0f, 800, pal::Muted, 1.6f);
	{
		const Chips PerEvent = T.Net / std::max(1, T.Events);
		const std::pair<std::string, std::string> Lines[6] = {
			{"Biggest score", T.Best > 0 ? StAmount(T.Best) : std::string("\xE2\x80\x94")},
			{"Peak profit", T.Peak > 0 ? StSigned(T.Peak) : std::string("\xE2\x80\x94")},
			{"Worst downswing", T.Downswing > 0 ? std::string(StMinus) + StAmount(T.Downswing) + "  \xC2\xB7  " + Grouped(std::max(1, T.DownTo - T.DownFrom)) + " events" : std::string("\xE2\x80\x94")},
			{"Longest dry run", Grouped(T.LongestDry) + (T.LongestDry == 1 ? " event" : " events") + " without a cash"},
			{"Profit per event", StSigned(PerEvent)},
			{"Current run", T.Dry == 0 ? std::string("Cashed last time out") : Grouped(T.Dry) + (T.Dry == 1 ? " event" : " events") + " since a cash"},
		};
		const float Top = Rc.Y + 40.0f;
		const float Row = (Rc.H - 48.0f) / 6.0f;
		for (int K = 0; K < 6; ++K)
		{
			const float Y = Top + Row * Nf(K) + Row / 2.0f + 4.5f;
			if (K > 0)
			{
				C->FillRect({Rc.X + 18.0f, Top + Row * Nf(K), Rc.W - 36.0f, 1.0f}, StGrid);
			}
			UI.Text(Lines[K].first, Rc.X + 18.0f, Y, Ts(12.5f, 600, StSoft));
			UI.Text(Lines[K].second, Rc.X + Rc.W - 18.0f, Y, Ts(12.5f, 800, StInk, Align::Right, Baseline::Alphabetic, false, Rc.W - 150.0f));
		}
	}
}

void RiverLine::StatsAbi(const world::Tracker& T, const Rect& Pg, const Rect& Plot, double Now)
{
	const float In = NetEase((Now - CardTabAt) / 0.9);
	// The stretches: Stride tournaments each, and the unfinished one at the end.
	struct Stretch
	{
		double From;
		double To;
		double Abi; // dollars
	};
	std::vector<Stretch> Parts;
	const size_t Count = T.Curve.size() + (T.Events > static_cast<int>(T.Curve.size()) * T.Stride ? 1 : 0);
	for (size_t K = 0; K < Count; ++K)
	{
		const double From = static_cast<double>(K * static_cast<size_t>(T.Stride));
		const double To = std::min(static_cast<double>(T.Events), From + T.Stride);
		Parts.push_back({From, To, static_cast<double>(T.StretchAbi(K)) / 100.0});
	}
	double Lo = 1e12;
	double Hi = 0.0;
	for (const Stretch& P : Parts)
	{
		Lo = std::min(Lo, std::max(0.1, P.Abi));
		Hi = std::max(Hi, P.Abi);
	}
	if (Parts.empty())
	{
		return;
	}
	// A log scale: a career from $1 to $1,000 stays readable, and each stake gets its own band.
	Lo = std::max(0.1, Lo / 1.6);
	Hi = std::max(Lo * 4.0, Hi * 1.6);
	const double L0 = std::log10(Lo);
	const double L1 = std::log10(Hi);
	const double Events = static_cast<double>(std::max(1, T.Events));
	auto Xof = [&](double E) { return Plot.X + Plot.W * Nf(E / Events); };
	auto Yof = [&](double V) { return Plot.Y + Plot.H * Nf((L1 - std::log10(std::max(0.1, V))) / (L1 - L0)); };
	// The stakes, as bands: micro up to $5.50, low to $55, mid to $530, high above (named after the line is drawn).
	std::vector<std::pair<const char*, float>> BandNames;
	{
		const double Edges[5] = {0.1, 5.5, 55.0, 530.0, 1e9};
		const char* const Names[4] = {"MICRO", "LOW", "MID", "HIGH"};
		for (int K = 0; K < 4; ++K)
		{
			const double A = std::max(Edges[K], Lo);
			const double B = std::min(Edges[K + 1], Hi);
			if (B <= A)
			{
				continue;
			}
			const float Ya = Yof(A);
			const float Yb = Yof(B);
			C->FillRect({Plot.X, Yb, Plot.W, Ya - Yb}, K % 2 == 0 ? Rgba(255, 255, 255, 0.025f) : Rgba(255, 255, 255, 0.0f));
			if (Edges[K + 1] < Hi)
			{
				C->FillRect({Plot.X, Yb, Plot.W, 1.0f}, StAxis);
			}
			if (Ya - Yb >= 18.0f)
			{
				BandNames.push_back({Names[K], Yb + 14.0f});
			}
		}
	}
	// Gridlines at clean amounts.
	{
		static const double Ticks[] = {0.25, 1.0, 2.5, 10.0, 25.0, 100.0, 250.0, 1000.0, 2500.0, 10000.0, 25000.0, 100000.0};
		int Shown = 0;
		for (double V : Ticks)
		{
			Shown += V >= Lo && V <= Hi ? 1 : 0;
		}
		int K = 0;
		for (double V : Ticks)
		{
			if (V < Lo || V > Hi)
			{
				continue;
			}
			if (Shown > 6 && (K++ % 2) == 1)
			{
				continue;
			}
			const float Y = Yof(V);
			C->FillRect({Plot.X, Y, Plot.W, 1.0f}, StGrid);
			UI.Text(V < 1.0 ? "$" + Fixed(V, 2) : StAxisMoney(V), Plot.X - 10.0f, Y + 4.0f, Ts(11.0f, 600, pal::Muted, Align::Right, Baseline::Alphabetic, true));
		}
		const double Xs = StNiceStep(Events, 5);
		for (double E = 0.0; E <= Events + 0.5; E += Xs)
		{
			const float X = Xof(E);
			C->FillRect({X, Plot.Y + Plot.H, 1.0f, 5.0f}, StAxis);
			UI.Text(StAxisCount(E), X, Plot.Y + Plot.H + 20.0f, Ts(11.0f, 600, pal::Muted, Align::Center, Baseline::Alphabetic, true));
		}
	}
	// The ABI, stretch by stretch: a stepped line with a light wash beneath, revealed left to right.
	{
		C->PushClip({Plot.X - 6.0f, Pg.Y, (Plot.W + 12.0f) * In, Pg.H});
		std::vector<Vec2> Stair;
		std::vector<Vec2> Area;
		Area.push_back({Xof(Parts.front().From), Plot.Y + Plot.H});
		for (const Stretch& P : Parts)
		{
			const float Y = Yof(P.Abi);
			Stair.push_back({Xof(P.From), Y});
			Stair.push_back({Xof(P.To), Y});
			Area.push_back({Xof(P.From), Y});
			Area.push_back({Xof(P.To), Y});
		}
		Area.push_back({Xof(Parts.back().To), Plot.Y + Plot.H});
		C->FillPolygon(Area, Paint::Linear({0.0f, Plot.Y}, {0.0f, Plot.Y + Plot.H}, NetA(StBar, 0.22f), NetA(StBar, 0.02f)));
		C->StrokePolyline(Stair, false, StBar, 2.0f, true);
		// Their lifetime ABI, as a reference line.
		const double Life = static_cast<double>(T.AverageBuyIn()) / 100.0;
		if (Life >= Lo && Life <= Hi)
		{
			const float Y = Yof(Life);
			C->FillRect({Plot.X, Y, Plot.W, 1.0f}, NetA(StGold, 0.55f));
		}
		const Vec2 End = Stair.back();
		C->FillCircle(End.X, End.Y, 6.5f, StPanel);
		C->FillCircle(End.X, End.Y, 4.5f, StBar);
		C->PopClip();
		if (In > 0.98f)
		{
			UI.Text(StAmount(T.StretchAbi(Parts.size() - 1)), End.X + 12.0f, End.Y + 5.0f, Ts(13.5f, 800, StInk));
			if (Life >= Lo && Life <= Hi)
			{
				const float Y = Yof(Life);
				const std::string Note = "Lifetime ABI " + StAmount(T.AverageBuyIn());
				StNote(*C, Note, Plot.X + Plot.W * 0.5f - C->Measure(Note, 10.5f, 700) * 0.5f, Y < Plot.Y + 30.0f ? Y + 22.0f : Y - 8.0f, true);
			}
		}
	}
	for (const auto& Band : BandNames)
	{
		float Wd = 0.0f;
		for (const char* Ch = Band.first; *Ch; ++Ch)
		{
			Wd += C->Measure(std::string(1, *Ch), 9.0f, 800) + 1.4f;
		}
		C->FillRoundRect({Plot.X + 4.0f, Band.second - 10.0f, Wd + 6.0f, 14.0f}, 4.0f, NetA(StPanel, 0.82f));
		NetSpaced(*C, Band.first, Plot.X + 8.0f, Band.second, 9.0f, 800, Hex(0x6d86a6), 1.4f);
	}
	// Hover: the stretch under the pointer.
	if (UI.Ptr.Active && UI.Hover(Plot) && CardOpen())
	{
		const double E = static_cast<double>(UI.Ptr.X - Plot.X) / static_cast<double>(Plot.W) * Events;
		size_t K = 0;
		while (K + 1 < Parts.size() && Parts[K].To < E)
		{
			++K;
		}
		const Stretch& P = Parts[K];
		const float X0 = Xof(P.From);
		const float X1 = Xof(P.To);
		C->FillRect({X0, Plot.Y, std::max(1.0f, X1 - X0), Plot.H}, Rgba(255, 255, 255, 0.05f));
		const float Y = Yof(P.Abi);
		C->FillRect({X0, Y - 1.0f, std::max(1.0f, X1 - X0), 3.0f}, Mix(StBar, Hex(0xffffff), 0.3f));
		const Chips Cents = static_cast<Chips>(std::llround(P.Abi * 100.0));
		const net::Tier Tr = net::TierOf(Cents);
		const char* const TierNames[5] = {"Freerolls", "Micro stakes", "Low stakes", "Mid stakes", "High stakes"};
		StTip(*C, UI, UI.Ptr.X, Y, Pg, "ABI " + StAmount(Cents),
			{"Tournaments " + Grouped(static_cast<int64_t>(P.From) + 1) + "\xE2\x80\x93" + Grouped(static_cast<int64_t>(P.To)), TierNames[static_cast<int>(Tr)]});
	}
}

/** The caption by the tabs: whose numbers these are. */
void RiverLine::StatsBrand(float X, float Y)
{
	for (int K = 0; K < 3; ++K)
	{
		const float H = 6.0f + 4.0f * Nf(K);
		C->FillRoundRect({X + Nf(K) * 6.0f, Y - H, 4.0f, H}, 1.5f, K == 2 ? StProfit : NetA(StProfit, 0.55f));
	}
	const float W = NetSpaced(*C, "RIVERLINE STATS", X + 26.0f, Y, 11.0f, 900, StInk, 1.8f);
	UI.Text("Tournament results  \xC2\xB7  every buy-in, every cash", X + 38.0f + W, Y, Ts(11.5f, 600, pal::Muted));
}

void RiverLine::HeroStatsCard(double Now)
{
	const world::World& W = S.Living();
	const float In = NetEase((Now - CardAt) / 0.3);
	C->FillRect({0.0f, NetTop, NetW, RiverLine::Height - NetTop}, Rgba(4, 8, 16, 0.76f * In));
	const Ui::ClickState Outside = UI.Clickable("herostatsbackdrop", {0.0f, NetTop, NetW, RiverLine::Height - NetTop});
	const Rect R{NetW / 2.0f - 530.0f, 96.0f + (1.0f - In) * 22.0f, 1060.0f, 806.0f};
	const Ui::ClickState Inside = UI.Clickable("herostatsbody", R);
	const float A0 = C->GetAlpha();
	C->SetAlpha(A0 * In);
	C->GlowRoundRect({R.X, R.Y + 8.0f, R.W, R.H}, 22.0f, Rgba(0, 0, 0, 0.5f), 36.0f);
	UI.RRect(R, 22.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, Hex(0x13223a), Hex(0x0c1626)), StEdge);
	C->FillRoundRect({R.X, R.Y, R.W, 6.0f}, 3.0f, pal::Accent);
	NetAvatar(*C, R.X + 76.0f, R.Y + 84.0f, 46.0f, S.HeroName, pal::Accent, true);
	UI.Text(S.HeroName, R.X + 142.0f, R.Y + 70.0f, Ts(34.0f, 900, StInk));
	UI.Text("Your tournaments on RiverLine since Night One", R.X + 142.0f, R.Y + 102.0f, Ts(16.0f, 600, StSoft));
	{
		const Rect X{R.X + R.W - 56.0f, R.Y + 22.0f, 36.0f, 36.0f};
		const Ui::ClickState Cx = UI.Clickable("herostatsclose", X);
		C->FillCircle(X.X + 18.0f, X.Y + 18.0f, 18.0f, Cx.Hover ? Hex(0x223352) : Hex(0x162540));
		C->StrokePolyline({{X.X + 12.0f, X.Y + 12.0f}, {X.X + 24.0f, X.Y + 24.0f}}, false, StSoft, 2.0f, true);
		C->StrokePolyline({{X.X + 24.0f, X.Y + 12.0f}, {X.X + 12.0f, X.Y + 24.0f}}, false, StSoft, 2.0f, true);
		if (Cx.Clicked)
		{
			CardShown = -1;
		}
	}
	StatsKpis(W.HeroStats(), {R.X + 24.0f, R.Y + 150.0f, R.W - 48.0f, 104.0f}, Now);
	StatsBrand(R.X + 26.0f, R.Y + 290.0f);
	StatsBoard(W.HeroStats(), W.HeroAwards(), {R.X + 24.0f, R.Y + 306.0f, R.W - 48.0f, R.H - 306.0f - 22.0f}, Now);
	C->SetAlpha(A0);
	if (Outside.Clicked && !Inside.Hover)
	{
		CardShown = -1;
	}
}
} // namespace ui
} // namespace ss
