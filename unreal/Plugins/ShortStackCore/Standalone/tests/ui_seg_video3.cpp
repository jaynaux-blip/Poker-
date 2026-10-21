// rerun_ui: ui-rerun's checks (update 3 SPEC §7.7): every Analytics page and card at both scales, likes and dislikes on the
// Content table, the one-video Overview and the launch card, dislikes on no public surface, the Ad market card and the
// "would have earned" line before Partner, the Duel picker, Home's RECOMMENDED tile, the Cut rename, the legacy "est." rows,
// at most 2 uncached thumbnails a frame (by Canvas::IsCached). Compiled into rerun_ui_test and called from
// rerun_ui_main.cpp (the kit is UiSeg.h); the helpers go in ui_test::video3_seg.
//
// G1 (ui-rerun's first cut, SPEC §5.2, §8.4 "charts before the Desk"): the chart kit, UI/ChartArt.h. Its scales and axis
// words; its three looks (Rerun's, a skin's, RunItTwice's red and black) and their contrast; every chart inside its rect at
// both scales (1 and 1.5 pixels a unit) for empty data, one point, negative values, gaps, a year of points and a flat line;
// the hover, the reveal, the markers, the last point; the cache (a replay draws exactly what the first draw drew, moved
// with its rect, and new data or a new look never replays the old); RunItTwice's "red is light" (a red run is only a twice
// print's under-copy) and no RiverLine colour in either look; the budgets (vertices, runs, warm milliseconds, printed);
// and two pages, chart_kit_rerun and chart_kit_rit, written with an out-dir for the rasterizer.
#include "UiSeg.h"

#include "../../Source/ShortStackCore/Private/ShortStack/UI/RiverLineVideoShared.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/UI/ChartArt.h"
#include "ShortStack/UI/Ui.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <limits>
#include <set>
#include <string>
#include <vector>

namespace ui_test
{
namespace video3_seg
{
using ss::ui::Canvas;
using ss::ui::Color;
using ss::ui::DrawList;
using ss::ui::Rect;
using ss::ui::Vec2;
namespace chart = ss::ui::chart;

const double ChartNaN = std::numeric_limits<double>::quiet_NaN();

/** RunItTwice's 16 tokens as SPEC v1.2 §3.3 sets them (the user's red and black), so the kit's look is checked whatever
 * Sites.cpp carries while core-sites-look's palette lands. */
ss::ui::Skin RitSkin()
{
	return ss::ui::Skin::FromHex({0x0c0c0f, 0x111114, 0x16161a, 0x1e1e23, 0x2c2c33, 0xececef, 0xa0a0aa, 0x62626c, 0xd2141a, 0xb6bbc5, 0xffffff, 0xff3d3d, 0xf5a524, 0x48c77a,
		0x1a1a1f, 0x09090b});
}

DrawList DrawWith(float Scale, const std::function<void(Canvas&)>& Fn)
{
	TableMeasurer M;
	DrawList L;
	Canvas C(L, M, 1600.0f, 1000.0f, Scale);
	Fn(C);
	return L;
}

/** What a draw list covers: visible vertices (alpha over 0.01) and every text run's ink (cap top to descender). */
struct InkBox
{
	float X0 = 1e9f, Y0 = 1e9f, X1 = -1e9f, Y1 = -1e9f;
	size_t Vertices = 0;
	int Runs = 0;
	bool Finite = true;
	bool Empty() const { return X1 < X0; }
};

InkBox InkOf(const DrawList& L)
{
	TableMeasurer M;
	InkBox I;
	I.Vertices = L.Vertices.size();
	for (const ss::ui::Vertex& V : L.Vertices)
	{
		if (!std::isfinite(V.X) || !std::isfinite(V.Y) || !std::isfinite(V.Col.A))
		{
			I.Finite = false;
			continue;
		}
		if (V.Col.A <= 0.01f)
		{
			continue;
		}
		I.X0 = std::min(I.X0, V.X);
		I.Y0 = std::min(I.Y0, V.Y);
		I.X1 = std::max(I.X1, V.X);
		I.Y1 = std::max(I.Y1, V.Y);
	}
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text)
		{
			continue;
		}
		++I.Runs;
		const ss::ui::TextItem& T = Cmd.Text;
		if (!std::isfinite(T.X) || !std::isfinite(T.BaselineY))
		{
			I.Finite = false;
			continue;
		}
		const float W = M.Width(T.Text, T.Face, T.SizePx);
		I.X0 = std::min(I.X0, T.X);
		I.X1 = std::max(I.X1, T.X + W);
		I.Y0 = std::min(I.Y0, T.BaselineY - 0.75f * T.SizePx);
		I.Y1 = std::max(I.Y1, T.BaselineY + 0.22f * T.SizePx);
	}
	return I;
}

bool InsideOf(const InkBox& I, const Rect& R, float Tol = 1.0f)
{
	return I.Finite && (I.Empty() || (I.X0 >= R.X - Tol && I.Y0 >= R.Y - Tol && I.X1 <= R.X + R.W + Tol && I.Y1 <= R.Y + R.H + Tol));
}

std::string BoxText(const InkBox& I)
{
	char Buf[160];
	std::snprintf(Buf, sizeof(Buf), "ink %.1f,%.1f to %.1f,%.1f%s", I.X0, I.Y0, I.X1, I.Y1, I.Finite ? "" : " (not finite)");
	return Buf;
}

uint32_t HexOf(const Color& C)
{
	auto B = [](float V) { return static_cast<uint32_t>(std::lround(std::max(0.0f, std::min(1.0f, V)) * 255.0f)); };
	return (B(C.R) << 16) | (B(C.G) << 8) | B(C.B);
}

double Contrast(const Color& A, const Color& B)
{
	auto Lum = [](const Color& C) {
		auto Ch = [](float V) {
			const double X = static_cast<double>(V);
			return X <= 0.03928 ? X / 12.92 : std::pow((X + 0.055) / 1.055, 2.4);
		};
		return 0.2126 * Ch(C.R) + 0.7152 * Ch(C.G) + 0.0722 * Ch(C.B);
	};
	const double La = Lum(A);
	const double Lb = Lum(B);
	return (std::max(La, Lb) + 0.05) / (std::min(La, Lb) + 0.05);
}

bool SameColor(const Color& A, const Color& B)
{
	return HexOf(A) == HexOf(B) && std::fabs(A.A - B.A) < 0.002f;
}

/** A deterministic wave of N values around Base (no draw from any game Rng). */
std::vector<double> Wave(int N, double Base, double Amp, double Phase = 0.0)
{
	std::vector<double> V;
	for (int K = 0; K < N; ++K)
	{
		const double T = static_cast<double>(K);
		V.push_back(Base + Amp * (0.6 * std::sin(T * 0.37 + Phase) + 0.3 * std::sin(T * 1.9 + Phase * 2.0) + 0.1 * std::cos(T * 0.11)));
	}
	return V;
}

std::vector<double> Cumulative(const std::vector<double>& Steps)
{
	std::vector<double> Out;
	double Acc = 0.0;
	for (const double S : Steps)
	{
		Acc += S;
		Out.push_back(Acc);
	}
	return Out;
}

// ------------------------------------------------------------------ scales and words

void Scales()
{
	struct Case
	{
		double Lo, Hi;
		bool Zero, Integer;
		const char* Name;
	};
	const Case Cases[] = {
		{0.0, 61408.0, true, false, "a month of views"},
		{-340.0, 1250.0, true, false, "a net that dipped below 0"},
		{-500.0, -20.0, true, false, "losses only"},
		{5.0, 5.0, true, false, "one value"},
		{5.0, 5.0, false, false, "one value, no zero"},
		{0.0, 0.0, true, false, "nothing"},
		{0.0, 3.0, true, true, "three events"},
		{0.0, 1.2e6, true, false, "a million"},
		{0.031, 0.067, false, false, "click-through rates"},
		{ChartNaN, ChartNaN, true, false, "NaN"},
		{-std::numeric_limits<double>::infinity(), 12.0, true, false, "an infinite end"},
	};
	int Bad = 0;
	for (const Case& K : Cases)
	{
		const chart::AxisScale S = chart::NiceScale(K.Lo, K.Hi, 4, K.Zero, K.Integer);
		bool Ok = S.Ticks.size() >= 2 && S.Ticks.size() <= 12 && S.Step > 0.0 && S.Hi > S.Lo && std::isfinite(S.Lo) && std::isfinite(S.Hi);
		for (size_t I = 1; Ok && I < S.Ticks.size(); ++I)
		{
			Ok = std::fabs((S.Ticks[I] - S.Ticks[I - 1]) - S.Step) < S.Step * 1e-6;
		}
		const double Mag = std::pow(10.0, std::floor(std::log10(S.Step) + 1e-9));
		const double Norm = S.Step / Mag;
		Ok = Ok && (std::fabs(Norm - 1.0) < 1e-6 || std::fabs(Norm - 2.0) < 1e-6 || std::fabs(Norm - 2.5) < 1e-6 || std::fabs(Norm - 5.0) < 1e-6);
		if (std::isfinite(K.Lo) && std::isfinite(K.Hi))
		{
			Ok = Ok && S.Lo <= std::min(K.Lo, K.Hi) + 1e-9 && S.Hi >= std::max(K.Lo, K.Hi) - 1e-9;
		}
		if (K.Zero)
		{
			Ok = Ok && std::find(S.Ticks.begin(), S.Ticks.end(), 0.0) != S.Ticks.end();
		}
		if (K.Integer)
		{
			Ok = Ok && S.Step >= 1.0 && std::fabs(S.Step - std::round(S.Step)) < 1e-9;
		}
		if (!Ok)
		{
			++Bad;
			std::printf("  chart scale    %s: %g..%g step %g (%zu ticks)\n", K.Name, S.Lo, S.Hi, S.Step, S.Ticks.size());
		}
	}
	Expect(Bad == 0, "NiceScale: whole steps of 1, 2, 2.5 or 5, covering the data, 0 kept, NaN and one value safe");
	const chart::AxisScale Neg = chart::NiceScale(-340.0, 1250.0, 4, true, false);
	const auto ZeroTick = std::find(Neg.Ticks.begin(), Neg.Ticks.end(), 0.0);
	Expect(Neg.Lo < 0.0 && Neg.Hi > 0.0 && ZeroTick != Neg.Ticks.end() && !std::signbit(*ZeroTick), "a range across 0 ticks exactly 0 (never -0)");
	Expect(chart::NiceTicks(0.0, 100.0, 4) == std::vector<double>({0.0, 25.0, 50.0, 75.0, 100.0}), "NiceTicks(0, 100) is 0, 25, 50, 75, 100");
	const chart::AxisScale Peak = chart::NiceScale(0.0, 4001.0, 4, true, false);
	const chart::AxisScale Losses = chart::NiceScale(-500.0, -20.0, 4, true, false);
	Expect(Peak.Lo == 0.0 && Peak.Hi == 5000.0 && Losses.Lo == -500.0 && Losses.Hi == 0.0,
		"a peak just past a round number keeps the plot: 4,001 scales to 5K (not 6K), -500 to -500 (not -600)");

	struct Word
	{
		std::string Got, Want;
	};
	const Word Words[] = {
		{chart::AxisCount(0.0, 250.0), "0"},
		{chart::AxisCount(750.0, 250.0), "750"},
		{chart::AxisCount(1500.0, 500.0), "1.5K"},
		{chart::AxisCount(1250.0, 250.0), "1.25K"},
		{chart::AxisCount(2000.0, 1000.0), "2K"},
		{chart::AxisCount(61408.0), "61K"},
		{chart::AxisCount(1200.0), "1.2K"},
		{chart::AxisCount(412.0), "412"},
		{chart::AxisCount(1.2e6), "1.2M"},
		{chart::AxisCount(999950.0), "1M"},
		{chart::AxisCount(-1500.0, 500.0), "-1.5K"},
		{chart::AxisCount(0.5, 0.5), "0.5"},
		{chart::AxisMoney(0.0, 25000.0), "$0"},
		{chart::AxisMoney(250.0, 250.0), "$2.50"},
		{chart::AxisMoney(25000.0, 5000.0), "$250"},
		{chart::AxisMoney(120000.0), "$1.2K"},
		{chart::AxisMoney(-4000.0, 2000.0), "-$40"},
		{chart::AxisMoney(21240.0), "$212.40"},
		{chart::AxisMoney(16371.0), "$163.71"},
		{chart::AxisDuration(192.0), "3:12"},
		{chart::AxisDuration(30.0), "0:30"},
		{chart::AxisDuration(3723.0), "1:02:03"},
		{chart::AxisPercent(0.046), "4.6%"},
		{chart::AxisPercent(0.46), "46%"},
		{chart::AxisPercent(0.025, 0.025), "2.5%"},
		{chart::AxisPercent(0.05, 0.025), "5%"},
		{chart::AxisPercent(-0.076), "-7.6%"},
		{chart::AxisDate(0), "Oct 5"},
		{chart::AxisDate(1), "Oct 6"},
		{chart::AxisText(chart::Units::Plain, 2.5, 0.5), "2.5"},
	};
	int Wrong = 0;
	for (const Word& W : Words)
	{
		if (W.Got != W.Want)
		{
			++Wrong;
			std::printf("  chart words    got \"%s\", want \"%s\"\n", W.Got.c_str(), W.Want.c_str());
		}
	}
	Expect(Wrong == 0, "the axis words: counts with K and M, money in cents, durations, percents, dates");
	// Every word the axes write stays in the measured glyph set (plain ASCII here: S11).
	bool Ascii = true;
	for (int K = -60; K <= 60; ++K)
	{
		const double V = std::pow(10.0, K / 6.0) * (K % 2 == 0 ? 1.0 : -1.0);
		for (const std::string& S : {chart::AxisCount(V), chart::AxisMoney(V * 100.0), chart::AxisDuration(V), chart::AxisPercent(V / 100.0), chart::AxisCount(V, 0.25)})
		{
			for (const char Ch : S)
			{
				Ascii = Ascii && static_cast<unsigned char>(Ch) >= 32 && static_cast<unsigned char>(Ch) < 127;
			}
		}
	}
	Expect(Ascii, "the axis words are plain ASCII over 20 decades, signs and all");
}

// ------------------------------------------------------------------ the looks

void Looks()
{
	using namespace ss::ui::rlvideo_detail;
	const chart::Theme R = chart::Rerun();
	Expect(SameColor(R.Ground, RrGround) && SameColor(R.Panel, RrPanel) && SameColor(R.Tip, RrPanel2) && SameColor(R.Edge, RrLine) && SameColor(R.Ink, RrCream) &&
			SameColor(R.Muted, RrMuted) && SameColor(R.Dim, RrDim) && SameColor(R.Accent, RrGold) && SameColor(R.Amber, RrWarn) && SameColor(R.Series[1], RrSky) &&
			SameColor(R.Series[3], RrCash),
		"chart::Rerun() wears RiverLineVideoShared.h's look");
	const ss::ui::Skin Rs = RitSkin();
	const chart::Theme X = chart::RunItTwice(Rs);
	Expect(HexOf(X.Value) == 0xffffff && HexOf(X.Under) == 0xd2141a && HexOf(X.Accent) == 0xd2141a && HexOf(X.Down) == 0xff3d3d && HexOf(X.Up) == 0x48c77a &&
			HexOf(X.Amber) == 0xf5a524 && HexOf(X.Series[0]) == 0xffffff && HexOf(X.Series[1]) == 0xb6bbc5,
		"chart::RunItTwice: Hot White figures, Signal Red under-copies and light, Chrome second, Signal Up and Down, amber");
	Expect(X.LabelFace == static_cast<int8_t>(ss::ui::Font::BoldCondensed) && X.FigureFace == static_cast<int8_t>(ss::ui::Font::BoldCondensed) &&
			X.AxisFace == static_cast<int8_t>(ss::ui::Font::BoldCondensed) && X.Caps && X.Cut == 10.0f && X.Radius == 0.0f && X.BarRadius == 0.0f,
		"chart::RunItTwice: BoldCondensed caps, square tiles with the one cut");
	auto Near = [](uint32_t A, uint32_t B) {
		for (int Sh = 0; Sh <= 16; Sh += 8)
		{
			if (std::abs(static_cast<int>((A >> Sh) & 255u) - static_cast<int>((B >> Sh) & 255u)) > 2)
			{
				return false;
			}
		}
		return true;
	};
	Expect(Near(HexOf(X.Panel), 0x202026) && HexOf(X.Panel2) == 0x0c0c0f, "RunItTwice's tiles are gloss: 202026 over Onyx (within 2 a channel)");
	Expect(Near(HexOf(X.Tip), 0x060607), "RunItTwice's tooltip sits on Tape Black (060607)");
	Expect(chart::TwiceOffset(28.0f) == 2.0f && chart::TwiceOffset(39.0f) == 2.0f && chart::TwiceOffset(40.0f) == 3.0f && chart::TwiceOffset(56.0f) == 4.0f &&
			chart::TwiceOffset(72.0f) == 5.0f && chart::TwiceOffset(96.0f) == 6.0f && chart::TwiceOffset(12.0f) == 2.0f,
		"the twice print's offset: round(Size / 16) in 2..6");
	const chart::Theme Home = chart::FromSkin(ss::ui::Skin::Current());
	Expect(R.Key() != X.Key() && R.Key() != Home.Key() && X.Key() == chart::RunItTwice(Rs).Key(), "every look has a key of its own, and the same look the same key");

	// Contrast: every colour that carries a number reads at 4.5:1 or more on every ground it sits on.
	struct Pair
	{
		const char* Name;
		Color Fg, Bg;
	};
	int Low = 0;
	for (int Which = 0; Which < 3; ++Which)
	{
		const chart::Theme& T = Which == 0 ? R : Which == 1 ? X : Home;
		const char* Look = Which == 0 ? "Rerun" : Which == 1 ? "RunItTwice" : "RiverLine";
		const Pair Pairs[] = {
			{"Muted on Ground", T.Muted, T.Ground},
			{"Muted on the tile's top", T.Muted, T.Panel},
			{"Muted on the tile's foot", T.Muted, T.Panel2},
			{"Ink on Ground", T.Ink, T.Ground},
			{"Ink on the tooltip", T.Ink, T.Tip},
			{"Muted on the tooltip", T.Muted, T.Tip},
			{"the figure on the tile's top", T.Value, T.Panel},
			{"Up on the tile's top", T.Up, T.Panel},
			{"Down on the tile's top", T.Down, T.Panel},
			{"Up on Ground", T.Up, T.Ground},
			{"Down on Ground", T.Down, T.Ground},
			{"Amber on Ground", T.Amber, T.Ground},
		};
		for (const Pair& P : Pairs)
		{
			const double Cr = Contrast(P.Fg, P.Bg);
			if (Cr < 4.5)
			{
				++Low;
				std::printf("  chart contrast %s: %s %.2f\n", Look, P.Name, Cr);
			}
		}
	}
	Expect(Low == 0, "the looks: every figure, label and delta at 4.5:1 or more on its ground (Rerun, RunItTwice, RiverLine)");
	const double SignalRedOnOnyx = Contrast(X.Under, X.Ground);
	Expect(SignalRedOnOnyx < 4.5, "RunItTwice's Signal Red is light, never ink (3.59:1 on Onyx): the kit prints it only as an under-copy");
	// The one label the kit prints on a filled mark (a moment's numbered bubble) reads 4.5:1 or more in every look.
	double BubbleLow = 99.0;
	for (const chart::Theme* T : {&R, &X, &Home})
	{
		chart::Series S;
		S.Y = {100.0, 300.0, 200.0, 500.0, 400.0}; // no axis figure reads "1"
		const std::vector<chart::Mark> Bubble = {{2, "1", false, Color{0, 0, 0, 0}}};
		const DrawList L = DrawWith(1.0f, [&](Canvas& C) { chart::LineChart(C, {100.0f, 100.0f, 600.0f, 300.0f}, {S}, chart::LineOpts(), *T, nullptr, &Bubble); });
		for (const ss::ui::DrawCmd& Cmd : L.Cmds)
		{
			if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == "1")
			{
				BubbleLow = std::min(BubbleLow, Contrast(Cmd.Text.Col, T->Accent));
			}
		}
	}
	Expect(BubbleLow >= 4.5 && BubbleLow < 99.0, "a numbered bubble's figure reads 4.5:1 or more on its accent (RunItTwice: Hot White on Signal Red)");
}

// ------------------------------------------------------------------ bounds

struct Shape
{
	const char* Name;
	std::vector<double> V;
};

std::vector<Shape> Shapes()
{
	std::vector<Shape> S;
	S.push_back({"empty", {}});
	S.push_back({"one point", {4200.0}});
	S.push_back({"28 days", Wave(28, 2200.0, 1400.0)});
	S.push_back({"negative values", {-340.0, 120.0, -80.0, 560.0, 1250.0, -900.0, 30.0}});
	S.push_back({"losses only", {-5.0, -20.0, -12.0, -40.0}});
	S.push_back({"gaps", {1.0, ChartNaN, 3.0, 4.0, ChartNaN, ChartNaN, 2.0}});
	S.push_back({"a year, big", Wave(365, 1.0e6, 8.0e5, 0.4)});
	S.push_back({"flat", {7.0, 7.0, 7.0, 7.0}});
	S.push_back({"spiky", {0.0, 100.0, 0.0, 100.0, 0.0, 100.0, 0.0, 100.0}});
	return S;
}

void Bounds()
{
	const chart::Theme Looks[2] = {chart::Rerun(), chart::RunItTwice(RitSkin())};
	const float Scales[2] = {1.0f, 1.5f};
	const Rect Rects[3] = {{232.0f, 222.0f, 1344.0f, 300.0f}, {20.0f, 246.0f, 1120.0f, 320.0f}, {600.0f, 400.0f, 180.0f, 90.0f}};
	const std::vector<Shape> Data = Shapes();
	int Checked = 0;
	int Bad[6] = {0, 0, 0, 0, 0, 0};
	const char* Kinds[6] = {"LineChart", "Bars", "Bars (stacked)", "Sparkline", "LineChart (no axes)", "Bars (no axes)"};
	for (const chart::Theme& T : Looks)
	{
		for (const float Sc : Scales)
		{
			for (const Shape& D : Data)
			{
				for (const Rect& R : Rects)
				{
					// Every element on: the band, the area, dots, the last point, bubbles and pins, a hover in the middle.
					chart::Series S;
					S.Y = D.V;
					S.Area = 0.35f;
					S.AreaFade = 0.06f;
					S.Last = true;
					S.Dots = D.V.size() < 40;
					chart::Series Typ;
					Typ.Y = D.V;
					for (double& V : Typ.Y)
					{
						V *= 0.8;
					}
					Typ.Dash = 4.0f;
					Typ.Gap = 4.0f;
					chart::Band B;
					for (const double V : D.V)
					{
						B.Lo.push_back(V - std::fabs(V) * 0.2);
						B.Hi.push_back(V + std::fabs(V) * 0.2);
					}
					std::vector<chart::Mark> Marks;
					if (!D.V.empty())
					{
						Marks.push_back({0, "1", false, Color{0, 0, 0, 0}});
						Marks.push_back({static_cast<int>(D.V.size()) - 1, "2", false, Color{0, 0, 0, 0}});
						Marks.push_back({static_cast<int>(D.V.size() / 2), "", true, Color{0, 0, 0, 0}});
					}
					chart::LineOpts O;
					O.Y = chart::Units::Money;
					O.X.FirstDay = 120;
					O.Mouse = {R.X + R.W * 0.5f, R.Y + R.H * 0.5f};
					O.Empty = "No results yet";
					const DrawList L = DrawWith(Sc, [&](Canvas& C) { chart::LineChart(C, R, {S, Typ}, O, T, &B, &Marks); });
					const InkBox I = InkOf(L);
					++Checked;
					if (!InsideOf(I, R))
					{
						if (Bad[0]++ == 0)
						{
							std::printf("  chart bounds   LineChart, %s, scale %.1f, rect %.0fx%.0f: %s\n", D.Name, Sc, R.W, R.H, BoxText(I).c_str());
						}
					}
					chart::LineOpts Bare;
					Bare.YLabels = false;
					Bare.XLabels = false;
					Bare.Grid = false;
					Bare.Mouse = {R.X + 2.0f, R.Y + R.H * 0.5f};
					const InkBox Ib = InkOf(DrawWith(Sc, [&](Canvas& C) { chart::LineChart(C, R, {S}, Bare, T); }));
					if (!InsideOf(Ib, R))
					{
						if (Bad[4]++ == 0)
						{
							std::printf("  chart bounds   LineChart (no axes), %s, scale %.1f: %s\n", D.Name, Sc, BoxText(Ib).c_str());
						}
					}
					// Bars, side by side (a second set at half and the opposite sign) and stacked.
					chart::BarSet A;
					A.V = D.V;
					chart::BarSet Bs;
					for (const double V : D.V)
					{
						Bs.V.push_back(-V * 0.5);
					}
					chart::BarOpts Bo;
					Bo.X.FirstDay = 120;
					Bo.Mouse = O.Mouse;
					for (int Stack = 0; Stack < 2; ++Stack)
					{
						Bo.Stacked = Stack == 1;
						const InkBox Bi = InkOf(DrawWith(Sc, [&](Canvas& C) { chart::Bars(C, R, {A, Bs}, Bo, T); }));
						if (!InsideOf(Bi, R))
						{
							if (Bad[1 + Stack]++ == 0)
							{
								std::printf("  chart bounds   %s, %s, scale %.1f: %s\n", Kinds[1 + Stack], D.Name, Sc, BoxText(Bi).c_str());
							}
						}
					}
					chart::BarOpts Bb;
					Bb.YLabels = false;
					Bb.XLabels = false;
					Bb.Grid = false;
					const InkBox Bn = InkOf(DrawWith(Sc, [&](Canvas& C) { chart::Bars(C, R, {A}, Bb, T); }));
					if (!InsideOf(Bn, R))
					{
						if (Bad[5]++ == 0)
						{
							std::printf("  chart bounds   Bars (no axes), %s, scale %.1f: %s\n", D.Name, Sc, BoxText(Bn).c_str());
						}
					}
					// A sparkline the size of a Rankings cell and of a Kpi's corner.
					for (const Rect& Sr : {Rect{R.X, R.Y, 120.0f, 24.0f}, Rect{R.X + 10.0f, R.Y + 10.0f, 134.0f, 40.0f}})
					{
						chart::SparkOpts So;
						So.Last = true;
						So.Zero = D.V.size() % 2 == 0;
						const InkBox Si = InkOf(DrawWith(Sc, [&](Canvas& C) { chart::Sparkline(C, Sr, D.V, T.Series[1], So, T); }));
						if (!InsideOf(Si, Sr))
						{
							if (Bad[3]++ == 0)
							{
								std::printf("  chart bounds   Sparkline, %s, scale %.1f, %.0fx%.0f: %s\n", D.Name, Sc, Sr.W, Sr.H, BoxText(Si).c_str());
							}
						}
					}
				}
			}
		}
	}
	for (int K = 0; K < 6; ++K)
	{
		Expect(Bad[K] == 0, (std::string(Kinds[K]) + " stays inside its rect: both looks, both scales, three sizes, every data shape").c_str());
	}
	std::printf("  chart bounds   %d line charts, %d bar charts, %d sparklines checked\n", Checked * 2, Checked * 3, Checked * 2);

	// Kpi tiles: Rerun's 380 x 110 with its sparkline, RunItTwice's 212 x 72 and the Desk's 8 across, long words, a tiny tile.
	struct Tile
	{
		Rect R;
		chart::KpiCard K;
		float Size;
	};
	std::vector<Tile> Tiles;
	chart::KpiCard Views;
	Views.Label = "VIEWS";
	Views.Value = "61,408";
	Views.Delta = "18%";
	Views.DeltaSign = 1;
	Views.Note = "More than your typical 28 days";
	Views.Spark = Wave(28, 2200.0, 1400.0);
	Tiles.push_back({{20.0f, 120.0f, 380.0f, 110.0f}, Views, 34.0f});
	chart::KpiCard Subs = Views;
	Subs.Label = "SUBSCRIBERS";
	Subs.Value = "+412";
	Subs.Sub = "(31 lost)";
	Subs.Delta = "9%";
	Subs.DeltaSign = -1;
	Subs.Selected = true;
	Tiles.push_back({{804.0f, 120.0f, 380.0f, 110.0f}, Subs, 34.0f});
	chart::KpiCard Gtd;
	Gtd.Label = "Tonight GTD";
	Gtd.Value = "$92,300";
	Gtd.Note = "18 guaranteed events";
	Tiles.push_back({{232.0f, 96.0f, 212.0f, 72.0f}, Gtd, 34.0f});
	chart::KpiCard Net;
	Net.Label = "NET";
	Net.Value = "+$612";
	Net.Delta = "$84";
	Net.DeltaSign = 1;
	Tiles.push_back({{232.0f, 130.0f, 159.0f, 76.0f}, Net, 32.0f});
	chart::KpiCard Long;
	Long.Label = "A LABEL FAR TOO LONG FOR ITS TILE, TO SEE IT CUT";
	Long.Value = "$1,234,567,890.12";
	Long.Sub = "and a sub line that cannot fit";
	Long.Delta = "1,234%";
	Long.DeltaSign = -1;
	Long.Note = "A note that runs on and on well past the tile's right edge";
	Tiles.push_back({{400.0f, 400.0f, 159.0f, 76.0f}, Long, 32.0f});
	chart::KpiCard Over = Gtd;
	Over.ValueCol = ss::ui::Hex(0xf5a524);
	Tiles.push_back({{700.0f, 400.0f, 212.0f, 72.0f}, Over, 34.0f});
	Tiles.push_back({{900.0f, 600.0f, 40.0f, 20.0f}, Long, 32.0f});
	int KpiBad = 0;
	for (const chart::Theme& T : Looks)
	{
		for (const float Sc : Scales)
		{
			for (const Tile& Tl : Tiles)
			{
				chart::KpiOpts Ko;
				Ko.ValueSize = Tl.Size;
				const InkBox I = InkOf(DrawWith(Sc, [&](Canvas& C) { chart::Kpi(C, Tl.R, Tl.K, T, Ko); }));
				const bool Tiny = Tl.R.W < 60.0f;
				// A tile too small for its words draws what fits; its ground stays in the rect, text may not, so only check the ground.
				if (!Tiny && !InsideOf(I, Tl.R, 1.5f))
				{
					if (KpiBad++ == 0)
					{
						std::printf("  chart bounds   Kpi \"%s\" %.0fx%.0f, scale %.1f: %s\n", Tl.K.Label.c_str(), Tl.R.W, Tl.R.H, Sc, BoxText(I).c_str());
					}
				}
			}
		}
	}
	Expect(KpiBad == 0, "Kpi stays inside its tile: Rerun's, the Board's, the Desk's, long words cut, an amber figure");

	// Tooltips at every corner of the page stay on it.
	int TipBad = 0;
	const Rect Page{0.0f, 0.0f, 1600.0f, 962.0f};
	for (const chart::Theme& T : Looks)
	{
		for (const Vec2 At : {Vec2{5.0f, 5.0f}, Vec2{1595.0f, 5.0f}, Vec2{1595.0f, 957.0f}, Vec2{5.0f, 957.0f}, Vec2{800.0f, 480.0f}})
		{
			Rect Got;
			const std::vector<chart::TipRow> Rows = {{"Views", "2,184", T.Series[0], Color{0, 0, 0, 0}}, {"Typical", "1,900 to 2,600", T.Series[1], Color{0, 0, 0, 0}},
				{"Net", "-$40.00", Color{0, 0, 0, 0}, T.Down}};
			const InkBox I = InkOf(DrawWith(1.0f, [&](Canvas& C) { Got = chart::Tooltip(C, At, "Oct 28", Rows, T, Page); }));
			if (!InsideOf(I, Page, 0.5f) || !InsideOf(I, Got, 1.5f))
			{
				if (TipBad++ == 0)
				{
					std::printf("  chart bounds   Tooltip at %.0f,%.0f: %s\n", At.X, At.Y, BoxText(I).c_str());
				}
			}
		}
	}
	Expect(TipBad == 0, "Tooltip flips and clamps to stay on the page, its words inside its card");

	// The rest of the kit: a donut, a funnel, a heat map, a ratio bar, the icons.
	int RestBad = 0;
	auto Rest = [&RestBad](const char* What, const InkBox& I, const Rect& R) {
		if (!InsideOf(I, R, 1.5f))
		{
			if (RestBad++ == 0)
			{
				std::printf("  chart bounds   %s: %s\n", What, BoxText(I).c_str());
			}
		}
	};
	for (const chart::Theme& T : Looks)
	{
		for (const float Sc : Scales)
		{
			const std::vector<chart::Slice> Slices = {{38.0, Color{0, 0, 0, 0}, "Home"}, {27.0, Color{0, 0, 0, 0}, "Next up"}, {14.0, Color{0, 0, 0, 0}, "Subscriptions"},
				{6.0, Color{0, 0, 0, 0}, "Search"}, {0.2, Color{0, 0, 0, 0}, "Kast"}, {0.0, Color{0, 0, 0, 0}, "External"}};
			Rest("Donut", InkOf(DrawWith(Sc, [&](Canvas& C) { chart::Donut(C, 300.0f, 300.0f, 80.0f, 18.0f, Slices, T, "38%", "Home", {370.0f, 300.0f}); })),
				{220.0f, 220.0f, 160.0f, 160.0f});
			Rest("Donut, empty", InkOf(DrawWith(Sc, [&](Canvas& C) { chart::Donut(C, 300.0f, 300.0f, 80.0f, 18.0f, {}, T, "0", "No views"); })),
				{220.0f, 220.0f, 160.0f, 160.0f});
			const std::vector<chart::Stage> Stages = {
				{"Impressions", 101300.0, "101,300"}, {"Views", 5912.0, "5,912"}, {"Past 0:30", 4255.0, ""}, {"Past half", 2010.0, ""}, {"Liked", 216.0, ""}, {"Subscribed", 31.0, ""}};
			for (int Lg = 0; Lg < 2; ++Lg)
			{
				Rest("Funnel", InkOf(DrawWith(Sc, [&](Canvas& C) { chart::Funnel(C, {40.0f, 500.0f, 520.0f, 200.0f}, Stages, T, Color{0, 0, 0, 0}, Lg == 1); })),
					{40.0f, 500.0f, 520.0f, 200.0f});
			}
			std::vector<std::vector<double>> Grid;
			for (int D = 0; D < 7; ++D)
			{
				Grid.push_back(Wave(24, 50.0, 40.0, D * 0.7));
			}
			Grid[3][5] = ChartNaN;
			chart::HeatOpts Ho;
			Ho.Rows = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
			for (int H = 0; H < 24; ++H)
			{
				Ho.Cols.push_back(ss::ClockString(H * 60.0));
			}
			Ho.Ring = {{6, 20}, {2, 23}, {9, 9}};
			Ho.Mouse = {900.0f, 200.0f};
			Rest("Heatmap", InkOf(DrawWith(Sc, [&](Canvas& C) { chart::Heatmap(C, {796.0f, 150.0f, 784.0f, 270.0f}, Grid, Ho, T); })), {796.0f, 150.0f, 784.0f, 270.0f});
			Rest("RatioBar", InkOf(DrawWith(Sc, [&](Canvas& C) { chart::RatioBar(C, {40.0f, 800.0f, 360.0f, 18.0f}, 216.0, 16.0, T, 0.975); })), {40.0f, 800.0f, 360.0f, 18.0f});
			Rest("RatioBar, nothing", InkOf(DrawWith(Sc, [&](Canvas& C) { chart::RatioBar(C, {40.0f, 800.0f, 360.0f, 10.0f}, 0.0, 0.0, T); })), {40.0f, 800.0f, 360.0f, 10.0f});
			for (int K = 0; K < static_cast<int>(chart::Icon::Count); ++K)
			{
				const float Cx = 100.0f + 40.0f * static_cast<float>(K);
				const float Lw = std::max(1.2f, 24.0f * 0.09f);
				Rest("an icon", InkOf(DrawWith(Sc, [&](Canvas& C) { chart::DrawIcon(C, static_cast<chart::Icon>(K), Cx, 900.0f, 24.0f, T.Ink); })),
					{Cx - 12.0f - Lw, 888.0f - Lw, 24.0f + Lw * 2.0f, 24.0f + Lw * 2.0f});
			}
		}
	}
	Expect(RestBad == 0, "Donut, Funnel, Heatmap, RatioBar and every icon stay inside their rects");
}

// ------------------------------------------------------------------ behaviour

void Behaviour()
{
	const chart::Theme T = chart::Rerun();
	const Rect R{20.0f, 246.0f, 1120.0f, 320.0f};
	// Empty data: no series geometry, no hover, the caller's empty note, nothing that isn't a number.
	{
		chart::LineOut Out;
		chart::LineOpts O;
		O.Empty = "No results yet";
		O.Mouse = {500.0f, 400.0f};
		const DrawList L = DrawWith(1.0f, [&](Canvas& C) { Out = chart::LineChart(C, R, {}, O, T); });
		Expect(Out.Points == 0 && Out.Hover == -1 && DrewRun(L, "No results yet") && InkOf(L).Finite, "LineChart with no data: the empty note, no hover");
		chart::Series None;
		None.Y = {ChartNaN, ChartNaN};
		const DrawList L2 = DrawWith(1.0f, [&](Canvas& C) { Out = chart::LineChart(C, R, {None}, O, T); });
		Expect(Out.Hover == -1 && DrewRun(L2, "No results yet"), "LineChart with only gaps reads as empty");
		chart::BarOut Bo;
		chart::BarOpts Bopt;
		Bopt.Empty = "No results yet";
		const DrawList L3 = DrawWith(1.0f, [&](Canvas& C) { Bo = chart::Bars(C, R, {}, Bopt, T); });
		Expect(Bo.Slots == 0 && Bo.Hover == -1 && DrewRun(L3, "No results yet"), "Bars with no data: the empty note");
		const DrawList L4 = DrawWith(1.0f, [&](Canvas& C) { chart::Sparkline(C, {10.0f, 10.0f, 120.0f, 24.0f}, {}, T.Accent, chart::SparkOpts(), T); });
		Expect(L4.Vertices.empty() && L4.Cmds.empty(), "a Sparkline with no data draws nothing");
		const DrawList L5 = DrawWith(1.0f, [&](Canvas& C) { chart::Heatmap(C, {10.0f, 10.0f, 300.0f, 100.0f}, {}, chart::HeatOpts(), T); });
		const DrawList L6 = DrawWith(1.0f, [&](Canvas& C) { chart::Funnel(C, {10.0f, 10.0f, 300.0f, 100.0f}, {}, T); });
		Expect(L5.Vertices.empty() && L6.Vertices.empty(), "Heatmap and Funnel with no data draw nothing");
	}
	// One point: a dot in the middle; a level sparkline; one bar in the middle of its slot.
	{
		chart::LineOut Out;
		chart::Series S;
		S.Y = {4200.0};
		const DrawList L = DrawWith(1.0f, [&](Canvas& C) { Out = chart::LineChart(C, R, {S}, chart::LineOpts(), T); });
		const float Mid = Out.Plot.X + Out.Plot.W * 0.5f;
		bool Dot = false;
		for (const ss::ui::Vertex& V : L.Vertices)
		{
			Dot = Dot || (SameColor(V.Col, T.Series[0]) && std::fabs(V.X - Mid) < 4.0f && std::fabs(V.Y - Out.Ys.Map(4200.0, Out.Plot.Y + Out.Plot.H, Out.Plot.Y)) < 4.0f);
		}
		Expect(Out.Points == 1 && Dot && Out.Ys.Lo == 0.0 && Out.Ys.Hi >= 4200.0, "LineChart with one point: a dot at its value in the middle");
		const DrawList Sp = DrawWith(1.0f, [&](Canvas& C) { chart::Sparkline(C, {100.0f, 100.0f, 120.0f, 24.0f}, {7.0}, T.Accent, chart::SparkOpts(), T); });
		const InkBox Si = InkOf(Sp);
		Expect(!Si.Empty() && Si.X1 - Si.X0 > 100.0f && Si.Y1 - Si.Y0 < 16.0f, "a Sparkline of one value is a level line across");
		chart::BarSet B;
		B.V = {12.0};
		chart::BarOut Bo;
		const DrawList Lb = DrawWith(1.0f, [&](Canvas& C) { Bo = chart::Bars(C, R, {B}, chart::BarOpts(), T); });
		float X0 = 1e9f;
		float X1 = -1e9f;
		for (const ss::ui::Vertex& V : Lb.Vertices)
		{
			if (SameColor(V.Col, T.Series[0]))
			{
				X0 = std::min(X0, V.X);
				X1 = std::max(X1, V.X);
			}
		}
		Expect(Bo.Slots == 1 && std::fabs((X0 + X1) * 0.5f - (Bo.Plot.X + Bo.Plot.W * 0.5f)) < 1.0f, "Bars with one value: one bar in the middle");
	}
	// Negative values: 0 inside the scale, the zero line drawn, bars hanging below it, an area on both sides.
	{
		chart::LineOut Out;
		chart::Series S;
		S.Y = {-340.0, 120.0, -80.0, 560.0, 1250.0, -900.0, 30.0};
		S.Area = 0.3f;
		const DrawList L = DrawWith(1.0f, [&](Canvas& C) { Out = chart::LineChart(C, R, {S}, chart::LineOpts(), T); });
		const float ZeroY = Out.Ys.Map(0.0, Out.Plot.Y + Out.Plot.H, Out.Plot.Y);
		bool ZeroLine = false;
		int Above = 0;
		int Below = 0;
		for (const ss::ui::Vertex& V : L.Vertices)
		{
			ZeroLine = ZeroLine || (SameColor(V.Col, T.Axis) && std::fabs(V.Y - ZeroY) < 1.0f);
			if (HexOf(V.Col) == HexOf(T.Series[0]) && V.Col.A > 0.0f && V.Col.A < 0.5f)
			{
				if (V.Y < ZeroY - 1.0f)
				{
					++Above;
				}
				else if (V.Y > ZeroY + 1.0f)
				{
					++Below;
				}
			}
		}
		Expect(Out.Ys.Lo < 0.0 && Out.Ys.Hi > 0.0 && ZeroLine, "LineChart across 0: a scale with 0 inside and the zero line drawn");
		Expect(Above > 0 && Below > 0, "the area fills to 0 from both sides");
		chart::BarSet B;
		B.V = {-50.0, -20.0};
		chart::BarOut Bo;
		const DrawList Lb = DrawWith(1.0f, [&](Canvas& C) { Bo = chart::Bars(C, R, {B}, chart::BarOpts(), T); });
		const float Top = Bo.Ys.Map(0.0, Bo.Plot.Y + Bo.Plot.H, Bo.Plot.Y);
		float MinY = 1e9f;
		for (const ss::ui::Vertex& V : Lb.Vertices)
		{
			if (SameColor(V.Col, T.Series[0]))
			{
				MinY = std::min(MinY, V.Y);
			}
		}
		Expect(Bo.Ys.Hi == 0.0 && Bo.Ys.Lo < -50.0 + 1e-9 && MinY >= Top - 0.6f, "negative bars hang from 0 at the top");
		chart::BarSet Up;
		Up.V = {30.0, 10.0};
		chart::BarOpts St;
		St.Stacked = true;
		const DrawList Ls = DrawWith(1.0f, [&](Canvas& C) { Bo = chart::Bars(C, R, {Up, B}, St, T); });
		Expect(Bo.Ys.Lo <= -50.0 && Bo.Ys.Hi >= 30.0, "stacked bars: positives stack up, negatives down, both in the scale");
	}
	// The hover: the index under the pointer, its point as the tooltip's anchor; outside, none.
	{
		chart::Series S;
		S.Y = Wave(28, 2200.0, 1400.0);
		chart::LineOpts O;
		chart::LineOut Out;
		DrawWith(1.0f, [&](Canvas& C) { Out = chart::LineChart(C, R, {S}, O, T); });
		O.Mouse = {Out.Plot.X + Out.Plot.W * 10.0f / 27.0f + 3.0f, Out.Plot.Y + 10.0f};
		DrawWith(1.0f, [&](Canvas& C) { Out = chart::LineChart(C, R, {S}, O, T); });
		const float WantY = Out.Ys.Map(S.Y[10], Out.Plot.Y + Out.Plot.H, Out.Plot.Y);
		Expect(Out.Hover == 10 && std::fabs(Out.HoverAt.Y - WantY) < 0.01f, "LineChart's hover: the nearest index and its point");
		O.Mouse = {R.X + R.W + 50.0f, R.Y};
		DrawWith(1.0f, [&](Canvas& C) { Out = chart::LineChart(C, R, {S}, O, T); });
		Expect(Out.Hover == -1, "no hover off the plot");
		O.Mouse = {Out.Plot.X + Out.Plot.W * 0.5f, Out.Plot.Y + 10.0f};
		O.Reveal = 0.5f;
		DrawWith(1.0f, [&](Canvas& C) { Out = chart::LineChart(C, R, {S}, O, T); });
		Expect(Out.Hover == -1, "no hover while the line is still drawing in");
		chart::BarSet B;
		B.V = Wave(48, 40.0, 30.0);
		chart::BarOpts Bo;
		chart::BarOut Bout;
		DrawWith(1.0f, [&](Canvas& C) { Bout = chart::Bars(C, R, {B}, Bo, T); });
		Bo.Mouse = {Bout.Plot.X + Bout.Plot.W * (47.5f / 48.0f), Bout.Plot.Y + 5.0f};
		DrawWith(1.0f, [&](Canvas& C) { Bout = chart::Bars(C, R, {B}, Bo, T); });
		Expect(Bout.Hover == 47 && Bout.HoverSlot.W > 0.0f, "Bars' hover: the slot under the pointer");
		int Slice = -2;
		const std::vector<chart::Slice> Slices = {{3.0, Color{0, 0, 0, 0}, "A"}, {1.0, Color{0, 0, 0, 0}, "B"}};
		DrawWith(1.0f, [&](Canvas& C) { Slice = chart::Donut(C, 300.0f, 300.0f, 80.0f, 18.0f, Slices, T, "", "", {300.0f - 71.0f, 305.0f}); });
		Expect(Slice == 0, "Donut's hover: just short of three quarters clockwise from 12 is still the first slice");
		DrawWith(1.0f, [&](Canvas& C) { Slice = chart::Donut(C, 300.0f, 300.0f, 80.0f, 18.0f, Slices, T, "", "", {300.0f - 50.0f, 300.0f - 50.0f}); });
		Expect(Slice == 1, "Donut's hover: the last quarter is the second slice");
		chart::HeatOpts Ho;
		Ho.Mouse = {10.0f + 1.5f * 102.0f / 1.0f, 15.0f};
		chart::HeatOut H;
		DrawWith(1.0f, [&](Canvas& C) { H = chart::Heatmap(C, {10.0f, 10.0f, 300.0f, 100.0f}, {{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}}, Ho, T); });
		Expect(H.Row == 0 && H.Col == 1, "Heatmap's hover: the cell under the pointer");
	}
	// The reveal: half drawn shows no line past the middle, no last point, no marks.
	{
		chart::Series S;
		S.Y = Wave(28, 2200.0, 1400.0);
		S.Col = T.Series[1]; // sky, so the lit last point (marigold) is the only accent
		S.Last = true;
		chart::LineOpts O;
		O.Reveal = 0.5f;
		chart::LineOut Out;
		const DrawList L = DrawWith(1.0f, [&](Canvas& C) { Out = chart::LineChart(C, R, {S}, O, T); });
		bool Clip = false;
		for (const ss::ui::DrawCmd& Cmd : L.Cmds)
		{
			Clip = Clip || (Cmd.Type == ss::ui::DrawCmd::Kind::PushClip && std::fabs(Cmd.Clip.X + Cmd.Clip.W - (Out.Plot.X + Out.Plot.W * 0.5f + 1.0f)) < 0.5f);
		}
		bool Lit = false;
		for (const ss::ui::Vertex& V : L.Vertices)
		{
			Lit = Lit || SameColor(V.Col, T.Accent);
		}
		Expect(Clip && !Lit, "LineChart's reveal clips the series at the reveal's x, and lights the last point only when drawn");
	}
}

// ------------------------------------------------------------------ the cache

std::string JsonOf(const DrawList& L)
{
	return L.ToJson();
}

void Cache()
{
	const chart::Theme T = chart::Rerun();
	const chart::Theme X = chart::RunItTwice(RitSkin());
	const Rect R{232.0f, 222.0f, 1344.0f, 300.0f};
	chart::Series S;
	S.Y = Cumulative(Wave(60, 4.0, 900.0, 1.3));
	S.Area = 0.06f;
	S.Last = true;
	chart::LineOpts O;
	O.Y = chart::Units::Money;
	O.X.FirstDay = 340;
	auto Draw = [&](const Rect& At, const chart::Series& Sr, const chart::Theme& Th, bool Cache) {
		chart::LineOpts Oc = O;
		Oc.Cache = Cache;
		return DrawWith(1.0f, [&](Canvas& C) { chart::LineChart(C, At, {Sr}, Oc, Th); });
	};
	const DrawList First = Draw(R, S, T, true);
	const DrawList Again = Draw(R, S, T, true);
	Expect(JsonOf(First) == JsonOf(Again), "a cached LineChart replays exactly what it drew");
	const DrawList Straight = Draw(R, S, T, false);
	bool Same = Straight.Vertices.size() == First.Vertices.size();
	for (size_t K = 0; Same && K < First.Vertices.size(); ++K)
	{
		Same = std::fabs(First.Vertices[K].X - Straight.Vertices[K].X) < 1e-3f && std::fabs(First.Vertices[K].Y - Straight.Vertices[K].Y) < 1e-3f &&
			SameColor(First.Vertices[K].Col, Straight.Vertices[K].Col);
	}
	Expect(Same, "the cached layers draw the same geometry as the straight draw");
	const Rect Moved{R.X + 37.0f, R.Y + 11.0f, R.W, R.H};
	const DrawList Shifted = Draw(Moved, S, T, true);
	bool Follows = Shifted.Vertices.size() == First.Vertices.size();
	for (size_t K = 0; Follows && K < First.Vertices.size(); ++K)
	{
		Follows = std::fabs(Shifted.Vertices[K].X - First.Vertices[K].X - 37.0f) < 1e-3f && std::fabs(Shifted.Vertices[K].Y - First.Vertices[K].Y - 11.0f) < 1e-3f;
	}
	Expect(Follows, "a replay moves with its rect");
	chart::Series Changed = S;
	Changed.Y[30] += 500.0;
	Expect(JsonOf(Draw(R, Changed, T, true)) != JsonOf(First), "new data never replays the old drawing");
	const DrawList Rit = Draw(R, S, X, true);
	bool Marigold = false;
	for (const ss::ui::Vertex& V : Rit.Vertices)
	{
		Marigold = Marigold || HexOf(V.Col) == 0xF2A33Au;
	}
	Expect(JsonOf(Rit) != JsonOf(First) && !Marigold, "a new look never replays the old one's colours");
	const DrawList Sp1 = DrawWith(1.0f, [&](Canvas& C) { chart::Sparkline(C, {300.0f, 600.0f, 120.0f, 24.0f}, S.Y, X.Series[1], chart::SparkOpts(), X); });
	const DrawList Sp2 = DrawWith(1.0f, [&](Canvas& C) { chart::Sparkline(C, {300.0f, 600.0f, 120.0f, 24.0f}, S.Y, X.Series[1], chart::SparkOpts(), X); });
	Expect(JsonOf(Sp1) == JsonOf(Sp2) && !Sp1.Vertices.empty(), "a cached Sparkline replays exactly");
}

// ------------------------------------------------------------------ the pages

const uint32_t RiverLineHexes[16] = {0x0a111c, 0x0e1726, 0x111c2e, 0x16233a, 0x223352, 0xe6edf7, 0x7b8aa3, 0x4d5b73, 0x27d3c3, 0x3b82f6, 0xf2c14e, 0xef4d5a, 0xf28a3a, 0x3ecf6e,
	0x0f6a4e, 0x083a2b};

/** No RiverLine token, and none of Banned, in any visible vertex or text run. */
bool NoLeak(const DrawList& L, const std::vector<uint32_t>& Banned, std::string& Found)
{
	std::set<uint32_t> Bad(std::begin(RiverLineHexes), std::end(RiverLineHexes));
	Bad.insert(Banned.begin(), Banned.end());
	for (const ss::ui::Vertex& V : L.Vertices)
	{
		if (V.Col.A > 0.01f && Bad.count(HexOf(V.Col)) > 0)
		{
			char Buf[32];
			std::snprintf(Buf, sizeof(Buf), "%06x", HexOf(V.Col));
			Found = Buf;
			return false;
		}
	}
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Bad.count(HexOf(Cmd.Text.Col)) > 0)
		{
			Found = Cmd.Text.Text;
			return false;
		}
	}
	return true;
}

/**
 * RunItTwice's "red is light" (SPEC §3.3, §7.7): every run in Signal Red, Deep Red or LED Red is a twice print's under-copy,
 * the same words in Ink or Hot White at (-o, -o). Counts the twice prints.
 */
bool RedIsLight(const DrawList& L, int& Twice, std::string& Stray)
{
	Twice = 0;
	std::vector<const ss::ui::TextItem*> Runs;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text)
		{
			Runs.push_back(&Cmd.Text);
		}
	}
	for (const ss::ui::TextItem* T : Runs)
	{
		const uint32_t H = HexOf(T->Col);
		if (H != 0xd2141a && H != 0x7a0a12 && H != 0xff2633)
		{
			continue;
		}
		const float O = chart::TwiceOffset(T->SizePx);
		bool Over = false;
		for (const ss::ui::TextItem* U : Runs)
		{
			const uint32_t Hu = HexOf(U->Col);
			Over = Over || (U->Text == T->Text && (Hu == 0xffffff || Hu == 0xececef) && std::fabs(U->X - (T->X - O)) < 0.01f && std::fabs(U->BaselineY - (T->BaselineY - O)) < 0.01f);
		}
		if (!Over)
		{
			Stray = T->Text;
			return false;
		}
		++Twice;
	}
	return true;
}

void Ground(Canvas& C, const Color& Col)
{
	C.FillRect({0.0f, 0.0f, 1600.0f, 962.0f}, Col);
}

void Card(Canvas& C, const Rect& R, const chart::Theme& T, const std::string& Title, const std::string& Right = std::string())
{
	if (T.Cut > 0.0f)
	{
		const float K = T.Cut;
		C.FillPolygon({{R.X, R.Y}, {R.X + R.W - K, R.Y}, {R.X + R.W, R.Y + K}, {R.X + R.W, R.Y + R.H}, {R.X, R.Y + R.H}}, ss::ui::Paint::Linear({R.X, R.Y}, {R.X, R.Y + R.H}, T.Panel, T.Panel2));
		C.StrokePolyline({{R.X + 0.5f, R.Y + 0.5f}, {R.X + R.W - K, R.Y + 0.5f}, {R.X + R.W - 0.5f, R.Y + K}, {R.X + R.W - 0.5f, R.Y + R.H - 0.5f}, {R.X + 0.5f, R.Y + R.H - 0.5f}}, true,
			T.Edge, 1.0f);
	}
	else
	{
		C.FillRoundRect(R, T.Radius, T.Panel);
		C.StrokeRoundRect({R.X + 0.5f, R.Y + 0.5f, R.W - 1.0f, R.H - 1.0f}, T.Radius - 0.5f, T.Edge, 1.0f);
	}
	ss::ui::TextStyle St = ss::ui::Ts(T.LabelSize, T.LabelWeight, T.Muted);
	St.Face = T.LabelFace;
	C.Text(Title, R.X + 18.0f, R.Y + 26.0f, St);
	if (!Right.empty())
	{
		ss::ui::TextStyle Rs = ss::ui::Ts(T.LabelSize, 600, T.Muted, ss::ui::Align::Right);
		Rs.Face = T.AxisFace;
		C.Text(Right, R.X + R.W - 18.0f - T.Cut, R.Y + 26.0f, Rs);
	}
}

/** Rerun's Analytics Overview drawn with the kit (SPEC §5.3; mockups/rerun.py's numbers): the page the rasterizer shows. */
DrawList RerunPage(float Scale, double& Ms)
{
	const chart::Theme T = chart::Rerun();
	std::vector<double> Views = Wave(28, 2190.0, 900.0, 0.2);
	for (size_t K = 23; K < Views.size(); ++K)
	{
		Views[K] += 1500.0 * std::exp(-static_cast<double>(K - 23) / 2.2);
	}
	const auto T0 = std::chrono::steady_clock::now();
	DrawList L = DrawWith(Scale, [&](Canvas& C) {
		Ground(C, T.Ground);
		C.FillRect({0.0f, 0.0f, 1600.0f, 60.0f}, ss::ui::Hex(0x0f1114));
		const char* Labels[4] = {"VIEWS", "WATCH TIME (HOURS)", "SUBSCRIBERS", "ESTIMATED REVENUE"};
		const char* Values[4] = {"61,408", "3,912", "+412", "$163.71"};
		const char* Deltas[4] = {"18%", "9%", "9%", "24%"};
		const char* Notes[4] = {"More than your typical 28 days", "About typical", "443 gained, 31 lost", "October ads pay more"};
		for (int K = 0; K < 4; ++K)
		{
			chart::KpiCard Kc;
			Kc.Label = Labels[K];
			Kc.Value = Values[K];
			Kc.Delta = Deltas[K];
			Kc.DeltaSign = 1;
			Kc.Note = Notes[K];
			Kc.Sub = K == 2 ? "(31 lost)" : "";
			Kc.Spark = Wave(28, 2190.0, 900.0, 0.2 + K * 0.5);
			Kc.SparkCol = T.Series[static_cast<size_t>(K)];
			Kc.Selected = K == 0;
			chart::Kpi(C, {20.0f + 392.0f * static_cast<float>(K), 120.0f, 380.0f, 110.0f}, Kc, T);
		}
		// The main chart: the views by day over the typical band, two numbered moments and the upload pins, a hover.
		Card(C, {20.0f, 246.0f, 1120.0f, 320.0f}, T, "VIEWS · LAST 28 DAYS", "shaded: your typical range");
		chart::Series Sv;
		Sv.Y = Views;
		Sv.Width = 3.0f;
		Sv.Area = 0.35f;
		Sv.AreaFade = 0.06f;
		Sv.Name = "Views";
		chart::Band Typ;
		for (int K = 0; K < 28; ++K)
		{
			Typ.Lo.push_back(1500.0 + 300.0 * std::sin(K * 0.4));
			Typ.Hi.push_back(2350.0 + 300.0 * std::sin(K * 0.4 + 0.3));
		}
		std::vector<chart::Mark> Marks = {{8, "1", false, Color{0, 0, 0, 0}}, {23, "2", false, Color{0, 0, 0, 0}}, {19, "3", false, Color{0, 0, 0, 0}}};
		for (const int U : {2, 5, 8, 11, 13, 16, 19, 22, 25, 27})
		{
			Marks.push_back({U, "", true, Color{0, 0, 0, 0}});
		}
		chart::LineOpts O;
		O.X.FirstDay = 361;
		O.X.Count = 9;
		const Rect Main{30.0f, 286.0f, 1100.0f, 270.0f};
		chart::LineOut Lo;
		Lo = chart::LineChart(C, Main, {Sv}, O, T, &Typ, &Marks);
		O.Mouse = {Lo.Plot.X + Lo.Plot.W * 23.0f / 27.0f, Lo.Plot.Y + 40.0f};
		Lo = chart::LineChart(C, Main, {Sv}, O, T, &Typ, &Marks);
		if (Lo.Hover >= 0)
		{
			chart::Tooltip(C, Lo.HoverAt, chart::AxisDate(361 + Lo.Hover),
				{{"Views", ss::Grouped(static_cast<int64_t>(Views[static_cast<size_t>(Lo.Hover)])), T.Series[0], Color{0, 0, 0, 0}},
					{"Typical", ss::Grouped(1900) + " to " + ss::Grouped(2600), ss::ui::Hex(0xffffff, 0.3f), Color{0, 0, 0, 0}}},
				T, Main);
		}
		// Realtime: 48 hourly bars (the last 8 lit) and 60 minute bars.
		Card(C, {1156.0f, 246.0f, 424.0f, 320.0f}, T, "REALTIME", "updating live");
		chart::BarSet Hours;
		Hours.V = Wave(48, 45.0, 30.0, 2.0);
		Hours.Col = ss::ui::Hex(0x8a6a3a);
		Hours.Each.assign(48, Color{0, 0, 0, 0});
		for (int K = 40; K < 48; ++K)
		{
			Hours.Each[static_cast<size_t>(K)] = T.Accent;
		}
		chart::BarOpts Bo;
		Bo.YLabels = false;
		Bo.Grid = false;
		Bo.Gap = 0.3f;
		Bo.X.Labels.assign(48, "");
		Bo.X.Labels[0] = "48 h ago";
		Bo.X.Labels[47] = "now";
		chart::Bars(C, {1166.0f, 300.0f, 404.0f, 110.0f}, {Hours}, Bo, T);
		chart::BarSet Mins;
		Mins.V = Wave(60, 0.9, 0.5, 5.0);
		Mins.Col = T.Series[1];
		chart::BarOpts Bm;
		Bm.YLabels = false;
		Bm.XLabels = false;
		Bm.Grid = false;
		chart::Bars(C, {1166.0f, 430.0f, 404.0f, 60.0f}, {Mins}, Bm, T);
		chart::DrawIcon(C, chart::Icon::Eye, 1180.0f, 520.0f, 18.0f, T.Muted);
		chart::DrawIcon(C, chart::Icon::PersonPlus, 1210.0f, 520.0f, 18.0f, T.Muted);
		chart::DrawIcon(C, chart::Icon::Clock, 1240.0f, 520.0f, 18.0f, T.Muted);
		// Traffic sources, the funnel, likes against dislikes, when viewers are on.
		Card(C, {20.0f, 582.0f, 380.0f, 368.0f}, T, "TRAFFIC SOURCES");
		chart::Donut(C, 210.0f, 760.0f, 120.0f, 26.0f,
			{{38.0, Color{0, 0, 0, 0}, "Home"}, {27.0, Color{0, 0, 0, 0}, "Next up"}, {14.0, Color{0, 0, 0, 0}, "Subscriptions"}, {6.0, Color{0, 0, 0, 0}, "Search"},
				{5.0, Color{0, 0, 0, 0}, "Notifications"}, {4.0, Color{0, 0, 0, 0}, "Kast"}, {6.0, T.Dim, "Other"}},
			T, "38%", "Home", {210.0f + 105.0f, 760.0f - 10.0f});
		Card(C, {416.0f, 582.0f, 360.0f, 220.0f}, T, "HOW VIEWERS GOT THERE");
		chart::Funnel(C, {434.0f, 614.0f, 324.0f, 176.0f},
			{{"Impressions", 101300.0, "101,300"}, {"Views", 5912.0, "5,912"}, {"Past 0:30", 4255.0, "4,255"}, {"Past half", 2010.0, "2,010"}, {"Liked", 216.0, "216"},
				{"Subscribed", 31.0, "31"}},
			T, Color{0, 0, 0, 0}, true);
		Card(C, {416.0f, 818.0f, 360.0f, 132.0f}, T, "LIKES AND DISLIKES");
		chart::DrawIcon(C, chart::Icon::ThumbUp, 446.0f, 872.0f, 20.0f, T.Up);
		chart::DrawIcon(C, chart::Icon::ThumbDown, 520.0f, 874.0f, 20.0f, T.Down);
		ss::ui::TextStyle Fig = ss::ui::Ts(15.0f, 800, T.Ink);
		C.Text("216", 462.0f, 878.0f, Fig);
		C.Text("16", 536.0f, 878.0f, Fig);
		chart::RatioBar(C, {434.0f, 892.0f, 324.0f, 18.0f}, 216.0, 16.0, T, 0.975);
		Card(C, {792.0f, 582.0f, 788.0f, 368.0f}, T, "WHEN YOUR VIEWERS ARE ON");
		std::vector<std::vector<double>> Week;
		for (int D = 0; D < 7; ++D)
		{
			std::vector<double> Day;
			for (int H = 0; H < 24; ++H)
			{
				const double Peak = std::exp(-std::pow((H - 21.5) / 3.0, 2.0)) + 0.5 * std::exp(-std::pow((H - 1.0) / 2.0, 2.0));
				Day.push_back(20.0 + 80.0 * Peak * (D >= 5 ? 1.2 : 1.0) + 6.0 * std::sin(D * 1.3 + H));
			}
			Week.push_back(Day);
		}
		chart::HeatOpts Ho;
		Ho.Rows = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
		for (int H = 0; H < 24; ++H)
		{
			Ho.Cols.push_back(ss::ClockString(H * 60.0));
		}
		Ho.ColEvery = 4;
		Ho.Ring = {{0, 4}, {2, 4}, {5, 4}};
		chart::Heatmap(C, {810.0f, 622.0f, 752.0f, 300.0f}, Week, Ho, T);
	});
	Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count();
	return L;
}

/** RunItTwice's Desk (SPEC §3.4 E) and the Board's KPI strip (§3.4 A), Rankings' sparklines (§3.4 D): the red and black page. */
DrawList RitPage(float Scale, double& Ms)
{
	const ss::ui::Skin Rs = RitSkin();
	const chart::Theme T = chart::RunItTwice(Rs);
	const auto T0 = std::chrono::steady_clock::now();
	DrawList L = DrawWith(Scale, [&](Canvas& C) {
		Ground(C, T.Ground);
		C.FillRect({0.0f, 0.0f, 216.0f, 962.0f}, Rs.Bg2);
		C.FillRect({215.0f, 0.0f, 1.0f, 962.0f}, Rs.Line);
		// The Desk's eight KPI tiles: figures in BoldCondensed 32, twice-printed; a 30-day delta in Signal Up or Down.
		const char* Labels[8] = {"Events", "ROI", "ITM", "FT", "ABI", "Net", "Rake", "Club"};
		const char* Values[8] = {"412", "+11.4%", "17.2%", "3.4%", "$14.20", "+$612", "$361", "$63"};
		const char* Deltas[8] = {"38", "2.1%", "0.8%", "0.4%", "$1.10", "$84", "$41", "$7"};
		const int Signs[8] = {1, 1, -1, 1, 1, 1, 1, 0};
		const float TileW = (1344.0f - 70.0f) / 8.0f;
		for (int K = 0; K < 8; ++K)
		{
			chart::KpiCard Kc;
			Kc.Label = Labels[K];
			Kc.Value = Values[K];
			Kc.Delta = Deltas[K];
			Kc.DeltaSign = Signs[K];
			chart::KpiOpts Ko;
			Ko.ValueSize = 32.0f;
			chart::Kpi(C, {232.0f + (TileW + 10.0f) * static_cast<float>(K), 130.0f, TileW, 76.0f}, Kc, T, Ko);
		}
		// The chart: cumulative net, a 2-unit Hot White line over a white area at 0.06, the last point a Signal Red dot with its tick.
		chart::Series Net;
		Net.Y = Cumulative(Wave(60, 1020.0, 9000.0, 1.3)); // cents: a grinder's 60 days, about +$10 a day with $90 swings
		Net.Col = T.Value;
		Net.Area = 0.06f;
		Net.AreaFade = 1.0f;
		Net.Last = true;
		Net.Name = "Net";
		chart::LineOpts O;
		O.Y = chart::Units::Money;
		O.X.FirstDay = 340;
		O.X.Count = 5;
		O.YTicks = 4;
		chart::LineOut Lo = chart::LineChart(C, {232.0f, 222.0f, 1344.0f, 300.0f}, {Net}, O, T);
		O.Mouse = {Lo.Plot.X + Lo.Plot.W * 41.0f / 59.0f, Lo.Plot.Y + 30.0f};
		Lo = chart::LineChart(C, {232.0f, 222.0f, 1344.0f, 300.0f}, {Net}, O, T);
		if (Lo.Hover >= 0)
		{
			const double V = Net.Y[static_cast<size_t>(Lo.Hover)];
			chart::Tooltip(C, Lo.HoverAt, chart::AxisDate(340 + Lo.Hover), {{"Net", ss::Money(static_cast<ss::Chips>(std::llround(V))), Color{0, 0, 0, 0}, V < 0.0 ? T.Down : T.Value}}, T,
				{232.0f, 222.0f, 1344.0f, 300.0f});
		}
		// Rankings: ten rows with a 10-point sparkline each, Chrome with the last point Signal Red.
		for (int K = 0; K < 10; ++K)
		{
			const float Y = 538.0f + 34.0f * static_cast<float>(K);
			C.FillRect({232.0f, Y, 664.0f, 34.0f}, K % 2 == 0 ? Rs.Panel : Rs.Panel2);
			ss::ui::TextStyle Rank = ss::ui::Ts(18.0f, 700, Rs.Ink);
			Rank.Face = static_cast<int8_t>(ss::ui::Font::BoldCondensed);
			C.Text(std::to_string(K + 1), 248.0f, Y + 24.0f, Rank);
			ss::ui::TextStyle Val = Rank;
			Val.Size = 15.0f;
			Val.HAlign = ss::ui::Align::Right;
			Val.Col = Rs.Gold;
			C.Text(ss::Grouped(4210 - K * 287), 640.0f, Y + 22.0f, Val);
			chart::SparkOpts So;
			So.Last = true;
			So.Area = 0.0f;
			So.Width = 1.5f;
			chart::Sparkline(C, {700.0f, Y + 5.0f, 180.0f, 24.0f}, Cumulative(Wave(10, 3.0, 6.0, K * 0.9)), T.Series[1], So, T);
		}
		// The Board's KPI strip (212 x 72, the amber figure printed once) and stacked bars on the right.
		const char* Bl[4] = {"Tonight GTD", "Overlay watch", "Running", "Your rake · Nov"};
		const char* Bv[4] = {"$92,300", "$3,273", "21", "$212.40"};
		const char* Bn[4] = {"18 guaranteed events", "3 events projected short", "4,812 players seated", "$37.60 more to Regular (20%)"};
		for (int K = 0; K < 4; ++K)
		{
			chart::KpiCard Kc;
			Kc.Label = Bl[K];
			Kc.Value = Bv[K];
			Kc.Note = Bn[K];
			Kc.Sub = K == 2 ? "events" : "";
			Kc.ValueCol = K == 1 ? T.Amber : Color{0, 0, 0, 0};
			Kc.Selected = K == 0;
			chart::KpiOpts Ko;
			Ko.NoteSize = 11.0f;
			chart::Kpi(C, {912.0f + 166.0f * static_cast<float>(K % 2) * 2.0f, 538.0f + 84.0f * static_cast<float>(K / 2), 212.0f, 72.0f}, Kc, T, Ko);
		}
		chart::BarSet Won;
		Won.V = {120.0, 340.0, -60.0, 210.0, -180.0, 90.0, 410.0, -30.0, 150.0, 260.0, -120.0, 75.0};
		Won.Col = T.Value;
		chart::BarSet Lost;
		Lost.V = {-40.0, -60.0, -20.0, -90.0, -10.0, -50.0, -30.0, -80.0, -20.0, -40.0, -60.0, -10.0};
		Lost.Col = T.Series[1];
		chart::BarOpts Bo;
		Bo.Stacked = true;
		Bo.Y = chart::Units::Money;
		Bo.X.Labels = {"Jan", "", "Mar", "", "May", "", "Jul", "", "Sep", "", "Nov", ""};
		chart::Bars(C, {912.0f, 720.0f, 664.0f, 230.0f}, {Won, Lost}, Bo, T);
		chart::RatioBar(C, {1470.0f, 548.0f, 106.0f, 14.0f}, 412.0, 1873.0, T, 0.2, T.Series[1], T.Grid);
		chart::DrawIcon(C, chart::Icon::Coin, 1480.0f, 590.0f, 18.0f, T.Series[1]);
		chart::DrawIcon(C, chart::Icon::Magnifier, 1504.0f, 590.0f, 18.0f, T.Series[1]);
		chart::DrawIcon(C, chart::Icon::Globe, 1528.0f, 590.0f, 18.0f, T.Series[1]);
		chart::DrawIcon(C, chart::Icon::Comment, 1552.0f, 590.0f, 18.0f, T.Series[1]);
		chart::DrawIcon(C, chart::Icon::Share, 1480.0f, 620.0f, 18.0f, T.Series[1]);
		chart::DrawIcon(C, chart::Icon::ThumbUp, 1504.0f, 620.0f, 18.0f, T.Up);
		chart::DrawIcon(C, chart::Icon::ThumbDown, 1528.0f, 622.0f, 18.0f, T.Down);
		chart::DrawIcon(C, chart::Icon::Eye, 1552.0f, 620.0f, 18.0f, T.Series[1]);
	});
	Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count();
	return L;
}

void Pages()
{
	for (const float Sc : {1.0f, 1.5f})
	{
		double ColdR = 0.0, ColdX = 0.0, WarmR = 0.0, WarmX = 0.0;
		const DrawList Rr = RerunPage(Sc, ColdR);
		const DrawList Rx = RitPage(Sc, ColdX);
		// Warm: the static layers replay (the median of 9 draws).
		std::vector<double> Wr;
		std::vector<double> Wx;
		for (int K = 0; K < 9; ++K)
		{
			double Ms = 0.0;
			RerunPage(Sc, Ms);
			Wr.push_back(Ms);
			RitPage(Sc, Ms);
			Wx.push_back(Ms);
		}
		std::sort(Wr.begin(), Wr.end());
		std::sort(Wx.begin(), Wx.end());
		WarmR = Wr[4];
		WarmX = Wx[4];
		const InkBox Ir = InkOf(Rr);
		const InkBox Ix = InkOf(Rx);
		std::printf("  chart page     chart_kit_rerun at %.1f: %zu vertices, %d runs, %.3f ms cold, %.3f ms warm\n", Sc, Ir.Vertices, Ir.Runs, ColdR, WarmR);
		std::printf("  chart page     chart_kit_rit   at %.1f: %zu vertices, %d runs, %.3f ms cold, %.3f ms warm\n", Sc, Ix.Vertices, Ix.Runs, ColdX, WarmX);
		// A full Analytics page of charts stays inside a laptop page's budget (§10: 35k vertices, 600 runs, 1.5 ms warm); the
		// milliseconds are this machine's, loose enough for a shared PC.
		Expect(Ir.Vertices <= 35000 && Ir.Runs <= 600 && Ix.Vertices <= 35000 && Ix.Runs <= 600, "both chart pages inside a page's vertex and run budget");
		Expect(WarmR < 6.0 && WarmX < 6.0, "both chart pages draw warm in a few milliseconds (printed; the editor's budget is checked in PIE)");
		Expect(InsideOf(Ir, {0.0f, 0.0f, 1600.0f, 962.0f}, 0.5f) && InsideOf(Ix, {0.0f, 0.0f, 1600.0f, 962.0f}, 0.5f), "both chart pages inside the site's client area");
		std::string Found;
		Expect(NoLeak(Rr, {0xd2141a, 0x7a0a12, 0xff2633}, Found), ("chart_kit_rerun: no RiverLine token and no RunItTwice red (" + Found + ")").c_str());
		Found.clear();
		Expect(NoLeak(Rx, {0xF2A33A, 0xFFF4E0, 0x16181D}, Found), ("chart_kit_rit: no RiverLine token and no Rerun colour (" + Found + ")").c_str());
		int Twice = 0;
		std::string Stray;
		Expect(RedIsLight(Rx, Twice, Stray), ("chart_kit_rit: red is light, every red run a twice print's under-copy (" + Stray + ")").c_str());
		Expect(Twice >= 8 && Twice <= 24, "chart_kit_rit: the Desk's figures print twice, 24 or fewer a page");
		int RerunTwice = 0;
		Expect(RedIsLight(Rr, RerunTwice, Stray) && RerunTwice == 0, "Rerun's look prints every figure once");
		bool AmberOnce = true;
		for (const ss::ui::DrawCmd& Cmd : Rx.Cmds)
		{
			if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == "$3,273")
			{
				AmberOnce = AmberOnce && HexOf(Cmd.Text.Col) == 0xf5a524;
			}
		}
		Expect(AmberOnce && DrewRun(Rx, "$3,273"), "a coloured figure (the Board's amber overlay) prints once");
		Expect(DrewRun(Rx, "EVENTS") && DrewRun(Rx, "OVERLAY WATCH") && !DrewRun(Rx, "Events"), "RunItTwice's labels in capitals");
		if (Sc == 1.0f)
		{
			WriteList("chart_kit_rerun", Rr);
			WriteList("chart_kit_rit", Rx);
		}
	}
}

// ------------------------------------------------------------------ budgets

void Budgets()
{
	const chart::Theme T = chart::Rerun();
	const chart::Theme X = chart::RunItTwice(RitSkin());
	struct Case
	{
		const char* Name;
		std::function<void(Canvas&)> Draw;
		size_t MaxVertices;
		int MaxRuns;
	};
	chart::Series Month;
	Month.Y = Wave(28, 2200.0, 1400.0);
	Month.Area = 0.35f;
	Month.AreaFade = 0.06f;
	chart::Series Year;
	Year.Y = Wave(365, 2200.0, 1400.0);
	Year.Area = 0.35f;
	Year.AreaFade = 0.06f;
	chart::Series Typical = Month;
	Typical.Area = 0.0f;
	Typical.Dash = 4.0f;
	Typical.Gap = 4.0f;
	chart::BarSet Hours;
	Hours.V = Wave(48, 40.0, 30.0);
	chart::KpiCard K;
	K.Label = "VIEWS";
	K.Value = "61,408";
	K.Delta = "18%";
	K.DeltaSign = 1;
	K.Note = "More than your typical 28 days";
	K.Spark = Month.Y;
	const Rect R{20.0f, 246.0f, 1120.0f, 320.0f};
	const std::vector<Case> Cases = {
		{"LineChart, 28 days with its typical", [&](Canvas& C) { chart::LineChart(C, R, {Month, Typical}, chart::LineOpts(), T); }, 3500, 20},
		{"LineChart, 365 days", [&](Canvas& C) { chart::LineChart(C, R, {Year}, chart::LineOpts(), T); }, 6000, 20},
		{"Bars, 48 hours (rounded, 22 wide)", [&](Canvas& C) { chart::Bars(C, R, {Hours}, chart::BarOpts(), T); }, 1600, 12},
		{"Sparkline, 10 points", [&](Canvas& C) { chart::Sparkline(C, {10.0f, 10.0f, 120.0f, 24.0f}, Wave(10, 5.0, 3.0), X.Series[1], chart::SparkOpts(), X); }, 200, 0},
		{"Kpi, Rerun's with a sparkline", [&](Canvas& C) { chart::Kpi(C, {20.0f, 120.0f, 380.0f, 110.0f}, K, T); }, 700, 6},
		{"Kpi, the Desk's (twice-printed)", [&](Canvas& C) { chart::Kpi(C, {232.0f, 130.0f, 159.0f, 76.0f}, K, X); }, 300, 6},
		{"Tooltip, three rows", [&](Canvas& C) { chart::Tooltip(C, {500.0f, 300.0f}, "Oct 28", {{"A", "1", T.Accent, Color{0, 0, 0, 0}}, {"B", "2", T.Accent, Color{0, 0, 0, 0}}, {"C", "3", T.Accent, Color{0, 0, 0, 0}}}, T, R); }, 300, 7},
	};
	int Over = 0;
	for (const Case& Cs : Cases)
	{
		const DrawList L = DrawWith(1.0f, Cs.Draw);
		const InkBox I = InkOf(L);
		// Warm: the mean of 200 draws once the static layers are cached.
		const auto T0 = std::chrono::steady_clock::now();
		for (int N = 0; N < 200; ++N)
		{
			DrawWith(1.0f, Cs.Draw);
		}
		const double Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count() / 200.0;
		std::printf("  chart budget   %-38s %5zu vertices (<= %zu), %2d runs (<= %d), %.4f ms warm\n", Cs.Name, I.Vertices, Cs.MaxVertices, I.Runs, Cs.MaxRuns, Ms);
		if (I.Vertices > Cs.MaxVertices || I.Runs > Cs.MaxRuns || Ms > 1.0)
		{
			++Over;
		}
	}
	Expect(Over == 0, "every chart inside its vertex and run budget and under 1 ms warm");
}
} // namespace video3_seg

void SegVideo3()
{
	// G1: the chart kit (ChartArt), for RunItTwice's Desk and Rankings now and Rerun's Analytics at G3.
	video3_seg::Scales();
	video3_seg::Looks();
	video3_seg::Bounds();
	video3_seg::Behaviour();
	video3_seg::Cache();
	video3_seg::Budgets();
	video3_seg::Pages();
}
} // namespace ui_test
