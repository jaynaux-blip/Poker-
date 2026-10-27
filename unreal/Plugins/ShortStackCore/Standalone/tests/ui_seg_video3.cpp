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
//
// C3 (ui-rerun, Rerun 2.0's pages; update 3 SPEC §7.7, SPEC_CREATORS §9.1's R1-R4 surfaces), on a channel the test builds
// through the core (uploads with recipes, a Duel, a Cut, a Live Drop, a breakout and its Binge wave, the world stepped
// between them): the header's four pages; Studio (the Cut and Loud renames, the lengths, three titles, six options and the
// Duel picker with its real cost, the settings, PUBLISH); the launch card (likes and dislikes, the ranking, "would have
// earned ... (not paid)" before Partner, the carry and Live Drop's lines, the four launch actions); every Analytics page
// and one video's four tabs (a Cut's "Shown in feed" and "Stayed to watch", the legacy "est."); the Ad market before
// Partner; Channel and Home (RECOMMENDED; no dislike on either); the Outlier shelf and the tags; the bell's lines; at most
// two uncached thumbnails painted a frame (Canvas::IsCached); every page inside the laptop at both scales and inside the
// page budget (printed); the copy lint and the trademark screen over the pages' inventories.
#include "UiSeg.h"

#include "../../Source/ShortStackCore/Private/ShortStack/UI/RiverLineVideoShared.h"
#include "../../Source/ShortStackCore/Private/ShortStack/Game/VideoAlgoPriv.h"
#include "CopyLint.h"
#include "Trademarks.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/Game/VideoAlgo.h"
#include "ShortStack/Game/VideoStats.h"
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

// ================================================================== C3: Rerun 2.0's pages (ui-rerun)

namespace video3_c3
{
using ss::ui::DrawList;
namespace rr = ss::ui::rlvideo_detail;
namespace video = ss::video;

int Checks = 0;
void Check(bool Condition, const char* What)
{
	++Checks;
	Expect(Condition, What);
}
void Check(bool Condition, const std::string& What)
{
	Check(Condition, What.c_str());
}

/** A laptop on the desk with Rerun's session, the pointer driven by hand, a frame drawn for every step. */
struct Lap
{
	QuietHooks H;
	ss::Session S;
	ss::ui::RiverLine RL;
	TableMeasurer M;
	double Now = 1.0;
	explicit Lap(const std::string& Seed) : S(H, Seed), RL(S)
	{
		S.CurrentScreen = ss::Screen::Lobby;
		Now = Step(S, Now, 0.2);
		Rest();
	}
	void Rest()
	{
		S.Life.Energy = 90.0;
		S.Life.Hunger = 5.0;
		S.Life.Thirst = 5.0;
	}
	DrawList Frame(float X = -1.0f, float Y = -1.0f, bool Down = false, bool Pressed = false, bool Released = false, float Wheel = 0.0f, float Scale = 1.0f)
	{
		RL.UI.Ptr.Active = X >= 0.0f;
		RL.UI.Ptr.X = X;
		RL.UI.Ptr.Y = Y;
		RL.UI.Ptr.Down = Down;
		RL.UI.Ptr.Pressed = Pressed;
		RL.UI.Ptr.Released = Released;
		RL.UI.Ptr.Wheel = Wheel;
		DrawList L;
		ss::ui::Canvas C(L, M, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, Scale);
		RL.Draw(C, Now);
		RL.UI.Ptr.EndFrame();
		Now += 1.0 / 30.0;
		return L;
	}
	/** Frames until every thumbnail on the page is cached (two paint a frame), then one more: the page at rest. */
	DrawList Settled(float Scale = 1.0f)
	{
		for (int K = 0; K < 80; ++K)
		{
			Frame(-1.0f, -1.0f, false, false, false, 0.0f, Scale);
			if (U().Paint.Painted == 0 && U().Paint.Waiting == 0)
			{
				break;
			}
		}
		return Frame(-1.0f, -1.0f, false, false, false, 0.0f, Scale);
	}
	void Click(float X, float Y)
	{
		Frame(X, Y);
		Frame(X, Y, true, true, false);
		Frame(X, Y, false, false, true);
	}
	void Open(int Page) { rr::VideoOpen(RL, Page, Now); }
	rr::VideoUi& U() { return rr::VideoUiOf(&RL); }
	video::Channel& Ch() { return S.Life.Video; }
};

/** The run that is exactly Text (or holds it), with its place and width. */
struct Run
{
	bool Found = false;
	float X = 0.0f, Base = 0.0f, W = 0.0f, Size = 0.0f;
};
Run RunOf(const DrawList& L, const std::string& Text, bool Has = false, float MinY = -1.0e9f)
{
	TableMeasurer M;
	Run Out;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.BaselineY >= MinY && (Has ? Cmd.Text.Text.find(Text) != std::string::npos : Cmd.Text.Text == Text))
		{
			Out.Found = true;
			Out.X = Cmd.Text.X;
			Out.Base = Cmd.Text.BaselineY;
			Out.Size = Cmd.Text.SizePx;
			Out.W = M.Width(Cmd.Text.Text, Cmd.Text.Face, Cmd.Text.SizePx); // X is the box's left (the alignment applied)
			return Out;
		}
	}
	return Out;
}

/** Clicks the middle of a run (a tab, a chip, a button's label): false when it isn't drawn. */
bool ClickRun(Lap& L, const DrawList& D, const std::string& Text, float MinY = -1.0e9f)
{
	const Run R = RunOf(D, Text, false, MinY);
	if (R.Found)
	{
		L.Click(R.X + R.W / 2.0f, R.Base - R.Size * 0.35f);
	}
	return R.Found;
}

std::string Joined(const DrawList& L)
{
	std::string Out;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text)
		{
			Out += Cmd.Text.Text + "\n";
		}
	}
	return Out;
}

/** Whether NetSpaced's tracked capitals (one run a glyph on one baseline) spell Word. */
bool Spelled(const DrawList& L, const std::string& Word)
{
	std::string Cur;
	float Base = -1.0e9f;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text)
		{
			continue;
		}
		const std::string& T = Cmd.Text.Text;
		if (T.size() == 1 && Cmd.Text.BaselineY == Base)
		{
			Cur += T;
		}
		else
		{
			Cur = T.size() == 1 ? T : std::string();
			Base = Cmd.Text.BaselineY;
		}
		if (Cur.find(Word) != std::string::npos)
		{
			return true;
		}
	}
	return false;
}

/** Text runs whose ink leaves the laptop's page (x outside 0..1600, y outside 0..962): their count, each printed. */
int Outside(const DrawList& L, const char* What)
{
	TableMeasurer M;
	int Bad = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text || Cmd.Text.BaselineY > 962.0f)
		{
			continue;
		}
		// The run as drawn (Canvas::Text resolves the alignment and the ellipsis): its box's left and its width.
		const float W = M.Width(Cmd.Text.Text, Cmd.Text.Face, Cmd.Text.SizePx);
		const float X0 = Cmd.Text.X;
		if (X0 < -0.5f || X0 + W > 1600.5f || Cmd.Text.BaselineY < 0.0f)
		{
			++Bad;
			std::printf("  %s: \"%s\" at %.1f..%.1f, %.1f\n", What, Cmd.Text.Text.c_str(), X0, X0 + W, Cmd.Text.BaselineY);
		}
	}
	return Bad;
}

/** Pairs of different text runs inside R whose boxes share a line and come within Gap of each other across it (review-c3f-ui: a top
 *  earner's revenue ran into LIKES), each printed. */
int Crowded(const DrawList& L, const ss::ui::Rect& R, const char* What, float Gap)
{
	TableMeasurer M;
	struct Box
	{
		std::string T;
		float X0, X1, Y0, Y1;
	};
	std::vector<Box> B;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		const ss::ui::TextItem& Tx = Cmd.Text;
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Tx.Text.size() > 1 && Tx.X >= R.X && Tx.X <= R.X + R.W && Tx.BaselineY >= R.Y && Tx.BaselineY <= R.Y + R.H)
		{
			B.push_back({Tx.Text, Tx.X, Tx.X + M.Width(Tx.Text, Tx.Face, Tx.SizePx), Tx.BaselineY - 0.72f * Tx.SizePx, Tx.BaselineY + 0.18f * Tx.SizePx});
		}
	}
	int N = 0;
	for (size_t I = 0; I < B.size(); ++I)
	{
		for (size_t J = I + 1; J < B.size(); ++J)
		{
			if (B[I].T != B[J].T && B[I].X0 < B[J].X1 + Gap && B[J].X0 < B[I].X1 + Gap && B[I].Y0 < B[J].Y1 && B[J].Y0 < B[I].Y1)
			{
				++N;
				std::printf("  %s: \"%s\" %.1f..%.1f runs into \"%s\" %.1f..%.1f\n", What, B[I].T.c_str(), B[I].X0, B[I].X1, B[J].T.c_str(), B[J].X0, B[J].X1);
			}
		}
	}
	return N;
}

/** A recipe as the library would deal one (the archetype, a palette, a face): the compositor draws it (ui-thumbs). */
video::ThumbSpec Spec(int I, int K)
{
	video::ThumbSpec S;
	S.Arch = static_cast<int16_t>((I * 3 + K * 7) % 35);
	S.Layout = static_cast<int16_t>((I + K) % 18);
	S.Bg = static_cast<int16_t>(K % 8);
	S.Pal = static_cast<int16_t>((I + 2 * K) % 11);
	S.Expr = static_cast<int16_t>((I + K) % 12 == 7 ? 4 : (I + K) % 12);
	S.CapStyle = static_cast<int16_t>(K % 8);
	S.Variant = static_cast<uint32_t>(1000 * I + K);
	S.Caption = "82%";
	return S;
}

/** The newest open candidate. */
std::string Newest(const ss::Session& S)
{
	const video::Candidate* Best = nullptr;
	for (const video::Candidate& C : S.Life.Video.Open)
	{
		Best = !Best || C.At >= Best->At ? &C : Best;
	}
	return Best ? Best->Id : std::string();
}

/**
 * A Rerun 2.0 channel's history through the core: N uploads three days apart, every format and style in turn, recipes on
 * most (a Duel every fourth, a Cut every fifth), the world stepped between them so each video has its verdict, its check-ins,
 * its parts and the rings their days; then a Live Drop on the newest, a breakout on the third newest (its Binge wave), the
 * Momentum and the streak a hit leaves.
 */
void Build(Lap& L, int N, bool Partner)
{
	video::Channel& Ch = L.Ch();
	Ch.X.On = true;
	L.S.Life.Update3LinesRead |= static_cast<uint32_t>(ss::life::U3Video);
	if (Partner)
	{
		Ch.PartnerAt = L.S.WorldMinutes() - 1.0;
	}
	const char* Kinds[8] = {"badbeat", "bluff", "cooler", "herocall", "bigpot", "deeprun", "suckout", "win"};
	for (int I = 0; I < N; ++I)
	{
		L.S.DebugVideo(std::string("fake ") + Kinds[I % 8] + " online");
		video::Choice C;
		C.Candidates = {Newest(L.S)};
		C.F = I % 5 == 3 ? video::Format::Short : I % 7 == 6 ? video::Format::Recap : video::Format::Breakdown;
		if (C.F == video::Format::Recap && Kinds[I % 8] != std::string("deeprun"))
		{
			C.F = video::Format::Breakdown;
		}
		C.T = static_cast<video::TitleStyle>(I % 5);
		C.Len = static_cast<video::LengthOpt>(I % 3);
		C.Ads = I % 6 == 5 ? video::AdLoad::Heavy : video::AdLoad::Standard;
		if (C.F != video::Format::Short && I % 3 != 2)
		{
			C.Thumbs = {Spec(I, 0)};
			if (I % 4 == 1)
			{
				C.Thumbs.push_back(Spec(I, 1));
				C.Thumbs.push_back(Spec(I, 2));
			}
		}
		video::UploadFacts Fa;
		Fa.Energy = 90.0;
		Fa.RentsPaid = 2;
		Fa.GearQuality = 0.5;
		video::Upload(Ch, C, L.S.WorldMinutes(), "rerun3-build", Fa);
		L.S.DebugVideo("advance 3");
		L.Rest();
	}
	// A breakout on the third newest (a surge into Next up: Breakout, Outlier tags), a hit's Momentum and the streak.
	std::vector<video::Video*> Up;
	for (video::Video& V : Ch.Videos)
	{
		Up.push_back(&V);
	}
	std::stable_sort(Up.begin(), Up.end(), [](const video::Video* A, const video::Video* B) { return A->At > B->At; });
	if (Up.size() >= 3)
	{
		video::Video& B = *Up[2];
		video::Comp K;
		K.T = B.At + 2.0 * 1440.0;
		K.P = 60.0 * std::max(50.0, B.X.P0);
		K.A = 0.3;
		K.Kind = video::CompKind::Breakout;
		K.SrcMix = 4;
		B.X.Comps.push_back(K);
		// Its Binge wave on the three Update 3 uploads before it (SPEC_CREATORS §3.3: 0.04 x the hit's excess, A 0.5).
		for (size_t I = 3; I < Up.size() && I < 6; ++I)
		{
			video::Comp W;
			W.T = K.T;
			W.P = 0.04 * K.P;
			W.A = 0.5;
			W.Kind = video::CompKind::Binge;
			W.SrcMix = 10;
			Up[I]->X.Comps.push_back(W);
		}
	}
	Ch.X.MomH = 0.7;
	Ch.X.Momentum = 1.38;
	Ch.X.MomShown = 1.2;
	Ch.X.StreakW = 6;
	Ch.X.RainChecks = 1;
}

/** Uploads the newest candidate now as a long video with a recipe (and a Duel of two), through the core's Upload (the launch
 *  card's subject; Session::Publish's glue carrying the Update 3 choice is core-videocontent's, checked in its suite). */
std::string PublishOne(Lap& L, bool Duel)
{
	L.S.DebugVideo("fake badbeat online");
	video::Choice C;
	C.Candidates = {Newest(L.S)};
	C.F = video::Format::Breakdown;
	C.T = video::TitleStyle::Story;
	C.Thumbs = {Spec(40, 0)};
	if (Duel)
	{
		C.Thumbs.push_back(Spec(40, 1));
	}
	video::UploadFacts Fa;
	Fa.Energy = 90.0;
	Fa.RentsPaid = 2;
	Fa.GearQuality = 0.5;
	return video::Upload(L.Ch(), C, L.S.WorldMinutes(), "rerun3-publish", Fa);
}

int CountRuns(const DrawList& L)
{
	int N = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		N += Cmd.Type == ss::ui::DrawCmd::Kind::Text ? 1 : 0;
	}
	return N;
}

/** A page's warm draw (the median of 9 frames once its thumbnails and charts are cached), printed with its size. */
void Measure(Lap& L, const std::string& Name, double MaxMs = 6.0)
{
	const DrawList D = L.Settled();
	std::vector<double> Ms;
	for (int K = 0; K < 9; ++K)
	{
		const auto T0 = std::chrono::steady_clock::now();
		L.Frame();
		Ms.push_back(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count());
	}
	std::sort(Ms.begin(), Ms.end());
	const int Runs = CountRuns(D);
	std::printf("  rerun page     %-28s %6zu vertices, %3d runs, %.3f ms warm\n", Name.c_str(), D.Vertices.size(), Runs, Ms[4]);
	Check(D.Vertices.size() <= 35000 && Runs <= 600, Name + ": inside a laptop page's budget (35k vertices, 600 runs)");
	Check(Ms[4] < MaxMs, Name + ": draws warm in a few milliseconds (printed; the editor's 1.5 ms is checked in PIE)");
}

/** No dislike on a public page: no run mentions dislikes and none is a video's dislike count. */
bool NoDislikes(Lap& L, const DrawList& D)
{
	const std::string All = trademarks::Lower(Joined(D));
	if (All.find("dislike") != std::string::npos)
	{
		return false;
	}
	for (const video::Video& V : L.Ch().Videos)
	{
		const video::VideoStats St = video::StatsOf(L.Ch(), V, L.S.WorldMinutes());
		if (St.Dislikes >= 10.0 && DrewRun(D, ss::Grouped(static_cast<int64_t>(std::floor(St.Dislikes)))))
		{
			return false;
		}
	}
	// review-ui-rerun: a tile's or the banner's line ("629 views · 6 days ago") puts a word beside every number; a bare count
	// beside the views is a count the page doesn't name (a dislike count slipped in passes the checks above).
	for (const ss::ui::DrawCmd& Cmd : D.Cmds)
	{
		const std::string& T = Cmd.Text.Text;
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text || T.find(" view") == std::string::npos)
		{
			continue;
		}
		size_t From = 0;
		while (From <= T.size())
		{
			const size_t Dot = T.find(" \xC2\xB7 ", From);
			const std::string Seg = T.substr(From, Dot == std::string::npos ? std::string::npos : Dot - From);
			bool Bare = !Seg.empty();
			for (const char C : Seg)
			{
				Bare = Bare && ((C >= '0' && C <= '9') || C == ',');
			}
			if (Bare)
			{
				std::printf("  public line with a bare count: \"%s\"\n", T.c_str());
				return false;
			}
			if (Dot == std::string::npos)
			{
				break;
			}
			From = Dot + 4;
		}
	}
	return true;
}

// ------------------------------------------------------------------ the shell

void Shell()
{
	Lap L("rerun3-shell");
	Build(L, 6, false);
	L.Open(0);
	DrawList D = L.Settled();
	Check(DrewRun(D, "HOME") && DrewRun(D, "STUDIO") && DrewRun(D, "CHANNEL") && DrewRun(D, "ANALYTICS"), "shell: HOME · STUDIO · CHANNEL · ANALYTICS");
	Check(RunOf(D, "HOME").X < RunOf(D, "STUDIO").X && RunOf(D, "STUDIO").X < RunOf(D, "CHANNEL").X && RunOf(D, "CHANNEL").X < RunOf(D, "ANALYTICS").X, "shell: the tabs in the spec's order");
	// The indices callers use (Mail opens 1): 0 Studio, 1 Channel, 2 Analytics, 3 Home; names for ss.Laptop.Open.
	Check(rr::VideoPageNamed("studio") == 0 && rr::VideoPageNamed("channel") == 1 && rr::VideoPageNamed("analytics") == 2 && rr::VideoPageNamed("home") == 3 &&
			rr::VideoPageNamed("nope") == -1,
		"shell: VideoPageNamed knows the four pages");
	for (int P = 0; P < 4; ++P)
	{
		L.Open(P);
		L.Frame();
		Check(L.U().Page == P, "shell: VideoOpen opens page " + std::to_string(P));
	}
	L.Open(9);
	Check(L.U().Page == 3, "shell: VideoOpen clamps to the last page");
	// The glance is public (rounded); the second line names the channel.
	L.Ch().Subs = 4210.0;
	L.Open(1);
	D = L.Frame();
	Check(DrewRun(D, "4.2K subscribers") && Drew(D, L.S.HeroName), "shell: the glance rounds subscribers in public (4.2K)");
	// Clicking the tabs moves the pages.
	Check(ClickRun(L, L.Frame(), "ANALYTICS") && L.U().Page == 2, "shell: the ANALYTICS tab");
	Check(ClickRun(L, L.Frame(), "HOME") && L.U().Page == 3, "shell: the HOME tab");
	Check(ClickRun(L, L.Frame(), "STUDIO") && L.U().Page == 0, "shell: the STUDIO tab");
	// The bell: the core's Rerun 2.0 lines, newest first, their accents (a breakout marigold).
	video::Channel& Ch = L.Ch();
	Ch.Bell.push_back({L.S.WorldMinutes(), "\xE2\x80\x9Ckings, 82% to win\xE2\x80\x9D is breaking out: it's showing up next to other videos."});
	Ch.Bell.push_back({L.S.WorldMinutes(), "New viewers are bingeing your channel: three older videos are getting views again."});
	L.Frame();
	const Run Bell = RunOf(L.Frame(), "4.2K subscribers");
	L.Click(1556.0f, 30.0f);
	D = L.Frame();
	Check(L.U().BellOpen && DrewRun(D, "Notifications") && Drew(D, "bingeing your channel") && Drew(D, "is breaking out"), "bell: Rerun 2.0's lines in the bell");
	Check(Bell.Found, "bell: the glance beside it");
	WriteList("rerun3_bell", D);
	L.Click(800.0f, 500.0f);
	Check(!L.U().BellOpen, "bell: a click off it closes it");
}

// ------------------------------------------------------------------ Studio

void Studio()
{
	Lap L("rerun3-studio");
	Build(L, 5, false);
	L.U().LaunchHidden = true;
	L.S.DebugVideo("fake badbeat online");
	L.S.DebugVideo("fake bluff online");
	L.Open(0);
	DrawList D = L.Settled();
	// The renames (§1.10, §6.2): a Cut, Loud; the lengths with their real minutes; three columns.
	Check(DrewRun(D, "Cut") && !DrewRun(D, "Short"), "studio: the Short format shows as Cut");
	Check(DrewRun(D, "Loud") && !DrewRun(D, "Hype"), "studio: the Hype style shows as Loud");
	Check(Drew(D, "Tight  6:00") && Drew(D, "Standard  8:00") && Drew(D, "Extended  12:00"), "studio: Breakdown's three lengths, 6:00, 8:00, 12:00");
	Check(Drew(D, "MOMENTS") && Drew(D, "IDEAS") && Drew(D, "SERIES"), "studio: MOMENTS · IDEAS · SERIES");
	Check(DrewRun(D, "PUBLISH") && Spelled(D, "PACKAGING READ") && Spelled(D, "SETTINGS"), "studio: PUBLISH, the packaging read and the settings");
	Check(Drew(D, "Goes up ") && Drew(D, "when the edit ends") && Drew(D, "Your viewers peak around"), "studio: when it goes up against the audience's peak (§6.6.3)");
	Check(Drew(D, "Edit 1 h 30 m") && Drew(D, "8 energy"), "studio: a Standard Breakdown's edit, 1 h 30 m and 8 energy");
	Check(Drew(D, "Riding your breakout: this upload starts with +38% reach"), "studio: the carry while Momentum is over 1.1 (R1, the core's line)");
	Check(Drew(D, "Upload streak: 6 weeks \xC2\xB7 1 Rain Check"), "studio: the upload streak with its Rain Check (R2)");
	Check(Drew(D, "Promise: ") && !Drew(D, "Your viewers are on at"), "studio: core-videocontent's packaging read (promise against payoff), the hour once");
	Check(Outside(D, "studio") == 0, "studio: every run inside the laptop");
	WriteList("rerun3_studio", D);
	// Extended: the edit's minutes follow the length (§4.5.5).
	Check(ClickRun(L, D, "Extended  12:00"), "studio: Extended picked");
	D = L.Frame();
	Check(Drew(D, "Edit 2 h") && Drew(D, "10 energy"), "studio: Extended's edit, 2 h and 10 energy");
	// A Cut: no thumbnail, fixed length, the feed's ads (§4.5.6, §5.4).
	Check(ClickRun(L, D, "Cut"), "studio: Cut picked");
	D = L.Frame();
	Check(DrewRun(D, "45 s, fixed") && Drew(D, "A Cut plays in the feed") && DrewRun(D, "Cuts share the feed's ads"), "studio: a Cut has no thumbnail and no ad load of its own");
	Check(ClickRun(L, D, "Breakdown"), "studio: back to a Breakdown");
	// The library's options as it deals them (three titles, six recipes): the Duel picker and its cost.
	rr::VideoUi& U = L.U();
	U.Titles = {{"badbeat.story.1", "kings, 82% to win, and then the nine of hearts", 1, 0}, {"badbeat.question.1", "Would you get kings in here? 58 big blinds", 0, 0},
		{"badbeat.plain.1", "Night Owl, hand 214: kings against ace-nine all in", 0, 0}};
	U.Thumbs.clear();
	for (int K = 0; K < 6; ++K)
	{
		U.Thumbs.push_back(Spec(50, K));
	}
	U.OptKey = rr::RrOptionKey(U);
	D = L.Settled();
	Check(Drew(D, "kings, 82% to win, and then the nine of hearts") && Drew(D, "Would you get kings in here?") && Drew(D, "Night Owl, hand 214"), "studio: the three title options");
	const video::Choice One = rr::RrChoice(U);
	Check(One.TitleTpl == "badbeat.story.1" && One.Thumbs.size() == 1 && One.Len == video::LengthOpt::Extended, "studio: the choice carries the template, the recipe and the length");
	const int PickedArch = U.Thumbs[static_cast<size_t>(std::clamp(U.ThumbPick, 0, 5))].Arch;
	const char* PickedName = video::ArchName(PickedArch);
	const std::string PickedRead = video::ArchRead(PickedArch);
	Check(PickedName && DrewRun(D, PickedName) && !PickedRead.empty() && Drew(D, PickedRead.substr(0, std::min<size_t>(12, PickedRead.size()))),
		"studio: the picked recipe's archetype and its read under the options (ArchName, ArchRead)");
	Check(ClickRun(L, D, "DUEL"), "studio: DUEL");
	D = L.Frame();
	// The options at x 912 / 1040 from the grid's top (below the day's verdict line when it shows): the second and the fifth.
	const float Oy = U.OptTop;
	Check(U.DuelOn && Oy >= 452.0f, "studio: the Duel picker is on");
	L.Click(rr::RrEdX + rr::RrThumbW + 12.0f + rr::RrOptW + 8.0f + 60.0f, Oy + 30.0f);
	L.Click(rr::RrEdX + rr::RrThumbW + 12.0f + 60.0f, Oy + 2.0f * (rr::RrOptH + 8.0f) + 30.0f);
	D = L.Settled();
	const video::Choice Duel = rr::RrChoice(U);
	Check(U.DuelOn && Duel.Thumbs.size() == 3, "studio: a Duel of three (the selected and two more)");
	Check(Drew(D, "DUEL: A + B + C (+30 min)") && Drew(D, "(+Duel)") && Drew(D, "Edit 2 h 30 m") && Drew(D, "12 energy"), "studio: the Duel's cost, +15 minutes and +1 energy a contender (§4.6.8)");
	WriteList("rerun3_studio_duel", D);
	// Shuffle advances the deal (the library's: here the stub deals none, so the injected options give way).
	const int Before = U.ThumbShuffle;
	Check(ClickRun(L, D, "SHUFFLE", 420.0f) && U.ThumbShuffle == Before + 1, "studio: the thumbnails' SHUFFLE advances the deal");
	// Ads: a segmented setting; Heavy rides in the choice.
	D = L.Frame();
	Check(ClickRun(L, D, "Heavy") && rr::RrChoice(U).Ads == video::AdLoad::Heavy, "studio: Heavy ads ride in the choice");
	Measure(L, "studio");
}

// ------------------------------------------------------------------ the launch card

void Launch()
{
	Lap L("rerun3-launch");
	Build(L, 7, false);
	const std::string Id = PublishOne(L, true);
	Check(!Id.empty(), "launch: a Duel published through the session");
	// Live Drop (R3): a stream of an hour starting 20 minutes after the drop, 30 of its own viewers.
	const video::Video* V = rr::RrVideo(L.S, Id);
	Check(V && V->X.On && V->X.Duels == 2, "launch: the video is Rerun 2.0's, with its Duel");
	if (V)
	{
		video::algo::NoteStream(L.Ch(), V->At + 20.0, V->At + 80.0, 40, 30.0, 25.0);
	}
	L.S.DebugVideo("advance 1");
	L.Rest();
	L.Open(0);
	DrawList D = L.Settled();
	Check(Drew(D, "LAUNCH") || Drew(D, "L"), "launch: the card");
	Check(DrewRun(D, "VIEWS") && DrewRun(D, "CTR SO FAR") && DrewRun(D, "LIKES \xC2\xB7 DISLIKES") && DrewRun(D, "RANKING") && DrewRun(D, "IMPRESSIONS"), "launch: views, CTR so far, likes and dislikes, the ranking, impressions");
	Check(Drew(D, " OF ") && Drew(D, "typical "), "launch: the numbers against the typical upload at the same age");
	Check(Drew(D, "As a Partner, this video would have earned about") && Drew(D, "(not paid)"), "launch: before Partner, \"would have earned ... (not paid)\" (§4.5.8)");
	Check(Drew(D, "Live Drop: your stream gave it a push (30 watching)"), "launch: Live Drop's line with the stream's own viewers (R3)");
	Check(Drew(D, "Riding your breakout"), "launch: the carry it got at upload (R1)");
	Check(Drew(D, "Make a Cut") && DrewRun(D, "Go live on Kast") && Drew(D, "Post a teaser") && Drew(D, "Reply round") && Drew(D, "Viewers who arrive early count."), "launch: the four launch actions and the hint");
	Check(DrewRun(D, "THUMBNAIL DUEL") && Drew(D, "of watch time") && Drew(D, " CTR"), "launch: the Duel's running CTRs and watch-time shares");
	Check(Drew(D, "VIEWS AN HOUR SINCE THE DROP") && Drew(D, "The verdict comes at midnight"), "launch: views an hour and when the verdict comes");
	Check(Drew(D, "dislikes are only visible to you"), "launch: the dislikes' privacy note");
	Check(Outside(D, "launch") == 0, "launch: every run inside the laptop");
	WriteList("rerun3_launch", D);
	Measure(L, "launch card");
	// NEW VIDEO: the editor while the launch runs, and back.
	L.S.DebugVideo("fake bluff online");
	Check(ClickRun(L, D, "NEW VIDEO \xE2\x80\xBA"), "launch: NEW VIDEO");
	D = L.Frame();
	Check(DrewRun(D, "PUBLISH") && Drew(D, "BACK TO THE LAUNCH"), "launch: the editor, with the way back");
	const Run Back = RunOf(D, "BACK TO THE LAUNCH", true);
	if (Back.Found)
	{
		L.Click(Back.X + Back.W / 2.0f, Back.Base - Back.Size * 0.35f);
	}
	Check(Back.Found && !L.U().LaunchHidden && Drew(L.Frame(), "LAUNCH ACTIONS"), "launch: back to the card");
	// After the verdict: the day's one-line result at the top of Studio (the bell's sentence and the label).
	L.S.DebugVideo("advance 2");
	L.Rest();
	L.U().LaunchHidden = true;
	D = L.Frame();
	const video::Video* After = rr::RrVideo(L.S, Id);
	Check(After && After->X.Verdict >= 0, "launch: the verdict came");
	if (After && After->X.Verdict >= 0)
	{
		const std::string Label = ss::ui::rlnet_detail::NetUpper(rr::RrVerdictName(After->X.Verdict));
		Check(DrewRun(D, Label), "launch: the verdict's label at the top of Studio for a day (" + Label + ")");
	}
	WriteList("rerun3_studio_verdict", D);
}

// ------------------------------------------------------------------ Analytics

void Analytics(bool Partner)
{
	const std::string Tag = Partner ? "partner" : "before";
	Lap L("rerun3-analytics-" + Tag);
	Build(L, 14, Partner);
	L.Open(2);
	DrawList D = L.Settled();
	// Overview (§5.3).
	Check(DrewRun(D, "OVERVIEW") && DrewRun(D, "Content") && DrewRun(D, "Audience") && DrewRun(D, "Revenue") && DrewRun(D, "Realtime") && DrewRun(D, "Research"), "overview " + Tag + ": the six sub-tabs");
	Check(DrewRun(D, "7 d") && DrewRun(D, "28 d") && DrewRun(D, "90 d") && DrewRun(D, "365 d") && DrewRun(D, "Lifetime"), "overview " + Tag + ": the range picker");
	Check(Drew(D, "VIEWS") && Drew(D, "WATCH TIME (HOURS)") && Drew(D, "SUBSCRIBERS") && Drew(D, "ESTIMATED REVENUE"), "overview " + Tag + ": the four tiles");
	Check(Drew(D, " lost)") && Drew(D, " gained \xC2\xB7 "), "overview " + Tag + ": subscribers gained and lost (§5.1)");
	Check(Drew(D, "REALTIME") && Drew(D, "views \xC2\xB7 last 48 hours"), "overview " + Tag + ": the Realtime card");
	Check(Drew(D, "TOP CONTENT") && Drew(D, "LATEST") && Drew(D, "CHANNEL HEALTH") && Drew(D, "Momentum"), "overview " + Tag + ": top content, the latest and Channel health");
	Check(Drew(D, "Upload streak: 6 weeks \xC2\xB7 1 Rain Check"), "overview " + Tag + ": the upload streak with its Rain Check (R2)");
	Check(Partner ? Drew(D, "HOW THE MONEY ADDS UP") : Drew(D, "Ads start when you join the Partner Program."), "overview " + Tag + ": the money card");
	Check(Outside(D, ("overview " + Tag).c_str()) == 0, "overview " + Tag + ": every run inside the laptop");
	WriteList("rerun3_overview_" + Tag, D);
	Measure(L, "overview " + Tag);
	// The range picker moves the window.
	Check(ClickRun(L, D, "90 d") && L.U().RangeIx == 2, "overview " + Tag + ": 90 d");
	D = L.Settled();
	Check(Drew(D, "LAST 90 DAYS"), "overview " + Tag + ": the chart follows the window");
	Check(ClickRun(L, D, "Lifetime") && L.U().RangeIx == 4, "overview " + Tag + ": Lifetime");
	D = L.Settled();
	Check(Drew(D, "LIFETIME"), "overview " + Tag + ": lifetime");
	L.U().RangeIx = 1;
	// Content: the table.
	Check(ClickRun(L, L.Frame(), "Content"), "content " + Tag + ": the tab");
	D = L.Settled();
	Check(DrewRun(D, "LIKES \xC2\xB7 DISLIKES") && DrewRun(D, "CTR 48H") && DrewRun(D, "IMPRESSIONS") && DrewRun(D, "COIN"), "content " + Tag + ": the table's columns");
	Check(DrewRun(D, "shown in feed") && DrewRun(D, "stayed to watch"), "content " + Tag + ": a Cut's impressions and CTR relabelled (§5.4)");
	Check(DrewRun(D, "BREAKOUT") || DrewRun(D, "OUTLIER"), "content " + Tag + ": the breakout's tag (R1)");
	Check(DrewRun(D, "BINGE WAVE"), "content " + Tag + ": the Binge wave's videos marked (R2)");
	Check(Outside(D, ("content " + Tag).c_str()) == 0, "content " + Tag + ": every run inside the laptop");
	WriteList("rerun3_content_" + Tag, D);
	Measure(L, "content " + Tag);
	// Sorting by views.
	Check(ClickRun(L, D, "VIEWS", 150.0f) && L.U().SortCol == 0, "content " + Tag + ": sort by views");
	// One video: the breakout's.
	std::vector<const video::Video*> Up;
	for (const video::Video& V : L.Ch().Videos)
	{
		Up.push_back(&V);
	}
	std::stable_sort(Up.begin(), Up.end(), [](const video::Video* A, const video::Video* B) { return A->At > B->At; });
	const video::Video& Hit = *Up[2];
	L.U().One = Hit.Id;
	L.U().OneTab = 0;
	D = L.Settled();
	Check(DrewRun(D, "Not pushed") && DrewRun(D, "Held") && DrewRun(D, "Pushed") && DrewRun(D, "Pushed more") && DrewRun(D, "Pushed hard"), "video " + Tag + ": the five-step scale (R1)");
	Check(Drew(D, "WHAT RERUN DID WITH IT") && Drew(D, "INSIGHTS") && Drew(D, "your subscribers got it"), "video " + Tag + ": the log and the Insights");
	Check(Drew(D, "is showing up next to other videos"), "video " + Tag + ": the breakout's line");
	Check(Drew(D, "YOUR NEXT UPLOAD") && Drew(D, "Riding your breakout"), "video " + Tag + ": the next upload's carry (R1)");
	// C3 gate (update 3 §4.6.7): a video inside its 28 days shows CHANGE THE PACKAGING: its first 48 hours against the typical,
	// the library's three titles and six thumbnails for it.
	Check(Drew(D, "CHANGE THE PACKAGING") && Drew(D, "This video's first 48 hours: CTR ") && L.U().RepackT.size() == 3 && L.U().RepackS.size() == 6,
		"video " + Tag + ": the repackage card (the read, three titles, six thumbnails)");
	Check(Partner ? DrewRun(D, "REVENUE") && Drew(D, "RPM ") : DrewRun(D, "WOULD HAVE EARNED") && DrewRun(D, "not paid"), "video " + Tag + ": revenue, or what it would have earned (not paid)");
	Check(Outside(D, ("video overview " + Tag).c_str()) == 0, "video " + Tag + ": every run inside the laptop");
	WriteList("rerun3_video_overview_" + Tag, D);
	Measure(L, "video overview " + Tag);
	const char* Tabs[3] = {"Reach", "Engagement", "Revenue"};
	for (int K = 0; K < 3; ++K)
	{
		Check(ClickRun(L, L.Frame(), Tabs[K], 240.0f) && L.U().OneTab == K + 1, std::string("video ") + Tag + ": the " + Tabs[K] + " tab");
		D = L.Settled();
		if (K == 0)
		{
			Check(Drew(D, "HOW VIEWERS GOT THERE") && DrewRun(D, "Past half") && DrewRun(D, "Subscribed") && Drew(D, "TRAFFIC SOURCES"), "reach " + Tag + ": the funnel and the sources");
		}
		else if (K == 1)
		{
			Check(Drew(D, "AUDIENCE RETENTION") && Drew(D, "Moments that held them") && Drew(D, "Dip: "), "engagement " + Tag + ": retention with its moments and the dip");
			Check(Drew(D, "LIKES AND DISLIKES") && Drew(D, "dislikes are only visible to you"), "engagement " + Tag + ": likes against dislikes, private");
		}
		else
		{
			Check(Drew(D, "AD TYPES") && Drew(D, "pre-roll") && Drew(D, "RPM A DAY") && Drew(D, "SUITABILITY AND ADS"), "revenue " + Tag + ": the ad types, the coin and RPM a day");
		}
		Check(Outside(D, (std::string("video ") + Tabs[K] + " " + Tag).c_str()) == 0, std::string("video ") + Tabs[K] + " " + Tag + ": every run inside the laptop");
		WriteList(std::string("rerun3_video_") + Tabs[K] + "_" + Tag, D);
		Measure(L, std::string("video ") + Tabs[K] + " " + Tag);
	}
	// A Cut's Reach: "Shown in feed" and "Stayed to watch" (§5.4).
	for (const video::Video* V : Up)
	{
		if (V->F == video::Format::Short)
		{
			L.U().One = V->Id;
			L.U().OneTab = 1;
			D = L.Settled();
			Check(DrewRun(D, "SHOWN IN FEED") && DrewRun(D, "STAYED TO WATCH") && !DrewRun(D, "IMPRESSIONS"), "reach " + Tag + ": a Cut's Shown in feed and Stayed to watch");
			WriteList("rerun3_video_cut_" + Tag, D);
			break;
		}
	}
	// C3 gate (§4.6.7): a press on the card's second title repackages the breakout through Session::Repackage (its words change,
	// one change spent); a second press inside three days is refused in the core's words.
	{
		L.U().One = Hit.Id;
		L.U().OneTab = 0;
		L.Settled();
		const std::string Was = Hit.Title;
		const int Before = Hit.X.Repacks;
		const std::string Second = L.U().RepackT.size() > 1 ? L.U().RepackT[1].Text : std::string();
		L.Click(1300.0f, 510.0f); // the second title row (the card at 1176, 372; the read, the label, row 0 at 464, row 1 at 496)
		D = L.Settled();
		std::printf("  rerun repack   \"%s\" -> \"%s\" (the card's second title \"%s\"), %d change(s)\n", Was.c_str(), Hit.Title.c_str(), Second.c_str(), Hit.X.Repacks);
		Check(!Second.empty() && Hit.X.Repacks == Before + 1 && Hit.Title != Was && Hit.Title == Second && Drew(D, "Changed. Rerun skips its next check-in"),
			"video " + Tag + ": a title press repackages the video (its words change, one change spent)");
		WriteList("rerun3_video_repack_" + Tag, D);
		L.Click(1300.0f, 510.0f);
		D = L.Settled();
		Check(Hit.X.Repacks == Before + 1 && Drew(D, "Give the last change three days."), "video " + Tag + ": a second change inside three days is refused");
	}
	L.U().One.clear();
	// Audience, Revenue, Realtime, Research.
	const char* Pages[4] = {"Audience", "Revenue", "Realtime", "Research"};
	for (int K = 0; K < 4; ++K)
	{
		Check(ClickRun(L, L.Frame(), Pages[K], 0.0f) && L.U().Sub == K + 2, std::string("analytics ") + Tag + ": " + Pages[K]);
		D = L.Settled();
		switch (K)
		{
		case 0:
			Check(Drew(D, "WHEN YOUR VIEWERS ARE ON") && Drew(D, "Best: ") && Drew(D, "You post at") && Drew(D, "AGE") && Drew(D, "TOP GEOGRAPHIES"), "audience " + Tag + ": the heat map and its caption, ages, geography");
			break;
		case 1:
			if (Partner)
			{
				Check(Drew(D, "EST. REVENUE") && Drew(D, "PLAYBACK CPM") && Drew(D, "MONETIZED PLAYS") && Drew(D, "PAYOUTS") && Drew(D, "AD SETTINGS ON YOUR VIDEOS"), "revenue " + Tag + ": the tiles, payouts, the ad settings");
				// C3 gate: a payday that rolled over (nothing paid yet) never reads PAID.
				const bool Paydays = std::any_of(L.Ch().Paid.begin(), L.Ch().Paid.end(), [](const std::string& K) { return K.rfind("pay:", 0) == 0; });
				Check(L.Ch().PaidCents > 0 || (!Drew(D, " PAID") && (!Paydays || Drew(D, " ROLLED OVER"))), "revenue " + Tag + ": a rolled-over month reads ROLLED OVER, never PAID");
				WriteList(std::string("rerun3_") + Pages[K] + "_" + Tag, D);
				Check(ClickRun(L, D, "MANAGE") && L.U().Manage, "revenue " + Tag + ": MANAGE opens the list");
				const DrawList Mg = L.Frame();
				Check(Drew(Mg, "MANAGE ADS") && (Drew(Mg, "judged") || Drew(Mg, "check-in on day")), "revenue " + Tag + ": each video's load and whether it's judged");
				WriteList("rerun3_Revenue_manage", Mg);
				// A click outside the list closes it and clicks nothing under it (here: the Realtime tab's spot).
				L.Click(431.0f, 90.0f);
				Check(!L.U().Manage && L.U().Sub == 3, "revenue " + Tag + ": a click off MANAGE's list closes it, nothing under it");
				D = L.Settled();
			}
			else
			{
				Check(Drew(D, "THE AD MARKET THIS WEEK") && Drew(D, "Setup and gear") && Drew(D, "WHAT YOUR VIDEOS WOULD HAVE EARNED"), "revenue " + Tag + ": the Ad market card and \"would have earned\" (§4.5.8)");
			}
			break;
		case 2: Check(Drew(D, "VIEWS \xC2\xB7 LAST 48 HOURS") && Drew(D, "LAST 60 MINUTES") && Drew(D, "subscribers now"), "realtime " + Tag + ": the hours, the minutes, subscribers now"); break;
		default:
			// C3 fix round (M2): the search table beside the trends; before Partner the Ad market takes the help card's place
			// (its words behind the trends' "?").
			Check(Drew(D, "TRENDING IN POKER THIS WEEK") && Drew(D, "WHAT POKER VIEWERS SEARCH") && Drew(D, "YOUR NICHE") &&
					(Partner ? Drew(D, "HOW RERUN DECIDES") : Drew(D, "THE AD MARKET THIS WEEK")),
				"research " + Tag + ": the trends, the searches, the niche, the help card (the Ad market before Partner)");
			break;
		}
		Check(Outside(D, (std::string(Pages[K]) + " " + Tag).c_str()) == 0, std::string(Pages[K]) + " " + Tag + ": every run inside the laptop");
		if (!(K == 1 && Partner))
		{
			WriteList(std::string("rerun3_") + Pages[K] + "_" + Tag, D);
		}
		Measure(L, std::string(Pages[K]) + " " + Tag);
	}
}

// ------------------------------------------------------------------ Channel and Home (public)

void Public()
{
	Lap L("rerun3-public");
	Build(L, 9, false);
	L.Open(1);
	DrawList D = L.Settled();
	Check(DrewRun(D, "VIDEOS") && DrewRun(D, "CUTS") && DrewRun(D, "SERIES") && DrewRun(D, "ABOUT"), "channel: VIDEOS · CUTS · SERIES · ABOUT");
	Check(DrewRun(D, "Latest") && DrewRun(D, "Popular") && DrewRun(D, "Oldest"), "channel: the sorts");
	Check(Drew(D, "THE OUTLIER SHELF") && (DrewRun(D, "BREAKOUT") || DrewRun(D, "OUTLIER")), "channel: the Outlier shelf with the breakout (R1)");
	Check(Drew(D, "Upload streak: 6 weeks"), "channel: the upload streak in the banner (R2)");
	Check(Drew(D, "Partner Program") && Drew(D, " / 1,000"), "channel: before Partner, the gate's two bars");
	Check(NoDislikes(L, D), "channel: no dislike on the public page");
	Check(!Drew(D, "BALANCE") && !Drew(D, " earned"), "channel: no money on the public page");
	Check(Outside(D, "channel") == 0, "channel: every run inside the laptop");
	WriteList("rerun3_channel", D);
	Measure(L, "channel");
	Check(ClickRun(L, D, "CUTS") && L.U().ChanTab == 1, "channel: the CUTS tab");
	D = L.Settled();
	Check(Drew(D, "CUT"), "channel: Cuts marked");
	Check(ClickRun(L, D, "ABOUT") && L.U().ChanTab == 3, "channel: ABOUT");
	// Home: a video Rerun is testing on Home right now is RECOMMENDED; the world's creators' videos are the library's.
	const std::string Id = PublishOne(L, false);
	L.Open(3);
	D = L.Settled();
	Check(!Id.empty() && DrewRun(D, "RECOMMENDED"), "home: the new upload, RECOMMENDED while Rerun tests it on Home (§6.6.6)");
	Check(DrewRun(D, "All") && DrewRun(D, "Bad beats") && DrewRun(D, "Strategy"), "home: the chips");
	Check(NoDislikes(L, D), "home: no dislike on the public feed");
	Check(Outside(D, "home") == 0, "home: every run inside the laptop");
	WriteList("rerun3_home", D);
	Measure(L, "home");
	Check(ClickRun(L, D, "Strategy") && L.U().HomeChip == 2, "home: a chip");
	// The world's creators beside yours (core-videocontent's WorldVideos is a stub today, so these are fake: the library's
	// recipes under made-up channels, set for the day the way RrDaily keeps them).
	L.U().HomeChip = 0;
	const std::vector<video::ThumbSpec> Specs = video::ContactSheet(1, 9);
	static const char* const Who[] = {"Overlay Weekly", "deepstack dana", "Ray at the rail"};
	for (size_t K = 0; K < Specs.size(); ++K)
	{
		video::WorldVideo W;
		W.Creator = Who[K % 3];
		W.Title = "Fake world video " + std::to_string(K + 1) + ": a read from the cheap seats";
		W.Spec = Specs[K];
		W.At = L.S.WorldMinutes() - 90.0 * static_cast<double>(K + 1);
		W.Views = 1200.0 + 3100.0 * static_cast<double>(K);
		L.U().WorldVids.push_back(W);
	}
	D = L.Settled();
	Check(!Specs.empty() && Drew(D, "Fake world video 1:") && Drew(D, "Overlay Weekly") && RunOf(D, "RECOMMENDED").Found, "home: the world's videos after yours, newest first");
	Check(RunOf(D, "Fake world video 1:", true).Base < RunOf(D, "Fake world video 5:", true).Base || !Drew(D, "Fake world video 5:"), "home: newest first, four a row");
	Check(NoDislikes(L, D) && Outside(D, "home world") == 0, "home: the world's tiles stay inside the laptop, no dislikes");
	WriteList("rerun3_home_world", D);
	Measure(L, "home with the world");
}

// ------------------------------------------------------------------ ss.Video.Sheet's contact sheet (VT1)

void Sheet()
{
	Lap L("rerun3-sheet");
	Build(L, 2, false);
	Check(!rr::VideoOpenSheet(L.RL, "nothing", 24, L.Now), "sheet: an unknown group opens nothing");
	static const char* const Groups[] = {"beats", "reads", "pots", "runs", "rival", "stories", "ideas"};
	for (const char* G : Groups)
	{
		const bool Opened = rr::VideoOpenSheet(L.RL, G, 24, L.Now);
		const DrawList D = L.Settled();
		const std::vector<video::ThumbSpec>& Sh = L.U().Sheet;
		std::set<std::pair<int, int>> Pairs;
		int Named = 0;
		for (const video::ThumbSpec& T : Sh)
		{
			Pairs.insert({T.Arch, T.Pal});
			Named += video::ArchName(T.Arch) && DrewRun(D, video::ArchName(T.Arch)) ? 1 : 0;
		}
		const std::string Tag = std::string("sheet ") + G;
		Check(Opened && L.U().Page == rr::RrPageSheet && L.U().SheetGroup == G, Tag + ": opens on the sheet page");
		// As many as the library has for the group, up to 24 (core-videocontent's ContactSheet).
		std::printf("  sheet          %-8s %2zu recipes, %2zu (archetype, palette) pairs, %2d named\n", G, Sh.size(), Pairs.size(), Named);
		Check(!Sh.empty() && Sh.size() <= 24 && Pairs.size() == Sh.size() && Named == static_cast<int>(Sh.size()),
			Tag + ": up to 24 recipes, no (archetype, palette) twice, each named");
		Check(DrewRun(D, std::to_string(Sh.size()) + " for " + G + ", no two alike"), Tag + ": the count it drew is the sheet's");
		Check(L.U().Paint.Waiting == 0, Tag + ": every thumbnail painted once the sheet settles");
		Check(Outside(D, Tag.c_str()) == 0, Tag + ": every run inside the laptop");
		if (G == Groups[0])
		{
			WriteList("rerun3_sheet", D);
			// A testing page: 24 thumbnails replay their cached vertices, so its size is printed, not held to a page's 35k.
			std::vector<double> Ms;
			for (int K = 0; K < 9; ++K)
			{
				const auto T0 = std::chrono::steady_clock::now();
				L.Frame();
				Ms.push_back(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count());
			}
			std::sort(Ms.begin(), Ms.end());
			std::printf("  rerun page     %-28s %6zu vertices, %3d runs, %.3f ms warm\n", "sheet beats", D.Vertices.size(), CountRuns(D), Ms[4]);
			Check(Ms[4] < 12.0, "sheet: draws warm (printed)");
		}
	}
	Check(rr::VideoOpenSheet(L.RL, "pots", 3, L.Now) && L.U().Sheet.size() == 3, "sheet: n of them");
	L.Open(0);
	Check(L.U().Page == rr::RrPageStudio, "sheet: the tabs leave it");
}

// ------------------------------------------------------------------ thumbnails: two uncached a frame

void Progressive()
{
	Lap L("rerun3-progressive");
	Build(L, 12, true);
	// A fresh look: nothing of this channel is cached at this size yet.
	L.S.Person.Appearance.Hair = (L.S.Person.Appearance.Hair + 5) % 12;
	L.Open(1);
	L.Frame();
	Check(L.U().Paint.Painted == 2 && L.U().Paint.Waiting > 0, "thumbnails: the first frame paints two uncached ones, the rest skeletons (V12)");
	int Frames = 1, Most = 2;
	while ((L.U().Paint.Painted > 0 || L.U().Paint.Waiting > 0) && Frames < 60)
	{
		L.Frame();
		Most = std::max(Most, L.U().Paint.Painted);
		++Frames;
	}
	Check(Most <= 2 && L.U().Paint.Painted == 0 && L.U().Paint.Waiting == 0, "thumbnails: never more than two uncached a frame, and the page settles");
	std::printf("  rerun thumbs   the Channel's grid and shelf cached over %d frames (2 a frame)\n", Frames);
	// A skeleton until a tile's turn: a new look repaints, two a frame.
	L.S.Person.Appearance.Hair = (L.S.Person.Appearance.Hair + 1) % 12;
	const DrawList D = L.Frame();
	Check(L.U().Paint.Painted == 2, "thumbnails: a new look repaints, two a frame");
	WriteList("rerun3_channel_skeletons", D);
}

// ------------------------------------------------------------------ an old save (V6): Update 2's channel

void Legacy()
{
	Lap L("rerun3-legacy");
	// Update 2 videos: the channel's X off, uploads through Update 2's choice sets.
	for (int I = 0; I < 4; ++I)
	{
		L.S.DebugVideo("fake badbeat online");
		video::Choice C;
		C.Candidates = {Newest(L.S)};
		C.F = video::Format::Breakdown;
		C.T = video::TitleStyle::Story;
		video::UploadFacts Fa;
		Fa.Energy = 90.0;
		video::Upload(L.Ch(), C, L.S.WorldMinutes(), "rerun3-legacy", Fa);
		L.S.DebugVideo("advance 2");
		L.Rest();
	}
	Check(!L.Ch().X.On && !L.Ch().Videos.empty() && !L.Ch().Videos.front().X.On, "legacy: an Update 2 channel and videos");
	L.Open(2);
	L.U().Sub = 1;
	DrawList D = L.Settled();
	Check(Drew(D, " est."), "legacy: the table marks Update 2's impressions and CTR \"est.\" (V6)");
	L.U().One = L.Ch().Videos.front().Id;
	D = L.Settled();
	Check(Drew(D, "Published before Rerun's new analytics"), "legacy: one video says it predates the log");
	WriteList("rerun3_legacy_video", D);
	L.Open(1);
	D = L.Settled();
	Check(DrewRun(D, "VIDEOS") && !L.Ch().X.On, "legacy: Channel renders and nothing turns Rerun 2.0 on");
	L.U().One.clear();
	L.S.DebugVideo("fake cooler online");
	L.Open(0);
	D = L.Settled();
	Check(DrewRun(D, "PUBLISH") && !L.Ch().X.On, "legacy: Studio renders; looking changes nothing");
	// C3 (the gate's rewrite): the library deals for an Update 2 channel too, so Studio's choice carries Rerun 2.0's packaging (a
	// template and a recipe), and looking turns nothing on: the first PUBLISH of it is the channel's migration (§7.5 item 3).
	const video::Choice C = rr::RrChoice(L.U());
	Check(!C.TitleTpl.empty() && C.Thumbs.size() == 1 && !L.Ch().X.On, "legacy: Studio deals Rerun 2.0's packaging on an Update 2 channel; looking turns nothing on");
	WriteList("rerun3_legacy_studio", D);
	// The migration (§7.5, V6, I7): that PUBLISH turns the channel on and makes a Rerun 2.0 video; every Update 2 video keeps its
	// numbers, stays Update 2 and gains no component as the days go by.
	struct Was
	{
		std::string Id, Title;
		double P, Retention, Rpm, Conv, At;
	};
	std::vector<Was> Old;
	for (const video::Video& V : L.Ch().Videos)
	{
		Old.push_back({V.Id, V.Title, V.P, V.Retention, V.Rpm, V.Conv, V.At});
	}
	L.Rest();
	const std::string Why = L.S.Publish(C);
	for (int K = 0; K < 1800 && L.S.TimeSkip.Active; ++K)
	{
		L.Now += 1.0 / 30.0;
		L.S.Update(L.Now);
	}
	L.S.DebugVideo("advance 7");
	int Kept = 0, New = 0;
	bool NewOn = false;
	for (const video::Video& V : L.Ch().Videos)
	{
		const auto W = std::find_if(Old.begin(), Old.end(), [&V](const Was& X) { return X.Id == V.Id; });
		if (W == Old.end())
		{
			++New;
			NewOn = V.X.On && V.X.TitleTpl == C.TitleTpl && V.X.Spec[0].Arch == C.Thumbs.front().Arch;
			continue;
		}
		Kept += !V.X.On && V.X.Comps.empty() && V.Title == W->Title && V.P == W->P && V.Retention == W->Retention && V.Rpm == W->Rpm && V.Conv == W->Conv && V.At == W->At ? 1 : 0;
	}
	Check(Why.empty() && L.Ch().X.On && New == 1 && NewOn && (L.S.Life.Update3LinesRead & ss::life::U3Video) != 0,
		"legacy: its first Rerun 2.0 upload migrates the channel (TurnOnU3): the channel on, the new video Rerun 2.0's, the save's bit set");
	Check(Kept == static_cast<int>(Old.size()) && !Old.empty(), "legacy: every Update 2 video keeps its numbers, stays Update 2 and gains no component a week on (V6, I7)");
}

// ------------------------------------------------------------------ copy

void Copy()
{
	std::vector<std::pair<std::string, std::string>> Lines;
	for (const std::string& S : rr::VideoUiInventory())
	{
		Lines.push_back({"rerun", S});
	}
	Failures += copylint::Run("ui-rerun", Lines, copylint::LetterBody);
	++Checks;
	int Bad = 0;
	std::vector<std::string> All = rr::VideoUiInventory();
	const std::vector<std::string> Frags = rr::VideoUiFragments();
	All.insert(All.end(), Frags.begin(), Frags.end());
	for (const std::string& S : All)
	{
		const char* Hit = trademarks::Find(S, trademarks::kSourceScan);
		const char* Hit2 = trademarks::Find(S, trademarks::kCopyScan);
		if (Hit || Hit2)
		{
			++Bad;
			std::printf("  copy: \"%s\" names \"%s\"\n", S.c_str(), Hit ? Hit : Hit2);
		}
		// U1: nothing in the world says AI, bot or autoplay.
		for (const char* Banned : {"ai", "bot", "bots", "autoplay", "autopilot"})
		{
			if (trademarks::HasWord(trademarks::Lower(S), Banned))
			{
				++Bad;
				std::printf("  copy: \"%s\" says \"%s\"\n", S.c_str(), Banned);
			}
		}
		// C9: no fire or burn words on Rerun's pages (Ignite owns the Flame).
		for (const char* Burn : {"fire", "flame", "blaze", "burn", "burnt", "burning", "spark", "heat"})
		{
			if (trademarks::HasWord(trademarks::Lower(S), Burn))
			{
				++Bad;
				std::printf("  copy: \"%s\" says \"%s\" (C9)\n", S.c_str(), Burn);
			}
		}
	}
	Check(Bad == 0, "copy: Rerun's pages name no real brand, never say AI, bot or autoplay, and no fire or burn word (U1, C9)");
	Check(Frags.size() > 200 && rr::VideoUiInventory().size() > 40, "copy: the pages' inventories are whole");
}

// ------------------------------------------------------------------ review-ui-rerun (C3): the gaps its mutants went through

/** The runs exactly Text. */
int RunsOf(const DrawList& L, const std::string& Text)
{
	int N = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		N += Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == Text ? 1 : 0;
	}
	return N;
}

video::Video* VideoById(Lap& L, const std::string& Id)
{
	for (video::Video& V : L.Ch().Videos)
	{
		if (V.Id == Id)
		{
			return &V;
		}
	}
	return nullptr;
}

void Review()
{
	// R1's carry in Studio only while Momentum is over 1.1, its number the channel's MomC - 1 (m3).
	{
		Lap L("rerun3-review-carry");
		Build(L, 5, false);
		L.U().LaunchHidden = true;
		L.S.DebugVideo("fake badbeat online");
		L.Open(0);
		L.Ch().X.Momentum = 1.05;
		DrawList D = L.Settled();
		Check(DrewRun(D, "PUBLISH") && !Drew(D, "Riding your breakout"), "review: no carry in Studio at Momentum 1.05 (R1: only over 1.1)");
		L.Ch().X.Momentum = 1.12;
		D = L.Frame();
		Check(Drew(D, "Riding your breakout: this upload starts with +12% reach"), "review: the carry at Momentum 1.12 reads +12%");
	}
	// The launch card: the carry the upload got (SettledS), not the channel's Momentum now (m4); before Partner its money tile
	// is what it would have earned, never revenue (m5).
	{
		Lap L("rerun3-review-launch");
		Build(L, 6, false);
		const std::string Id = PublishOne(L, false);
		video::Video* V = VideoById(L, Id);
		Check(V && V->X.On && V->X.SettledS > 1.1f, "review: the launch's upload got a carry over 1.1");
		L.S.DebugVideo("advance 1");
		L.Rest();
		L.Open(0);
		V = VideoById(L, Id);
		const std::string Got = V ? "Riding your breakout: this upload starts with +" + std::to_string(std::llround(100.0 * (static_cast<double>(V->X.SettledS) - 1.0))) + "% reach" : std::string("?");
		L.Ch().X.Momentum = 0.9; // a flop since: the meter falls, the card keeps what the upload got
		L.U().Stats.clear();
		DrawList D = L.Settled();
		Check(DrewRun(D, "LAUNCH ACTIONS") || Drew(D, "LAUNCH ACTIONS"), "review: the launch card is up");
		Check(Drew(D, Got), "review: the launch card's carry is the upload's own (" + Got + "), whatever Momentum does after");
		Check(Drew(D, "WOULD HAVE EARNED") && DrewRun(D, "not paid") && !Drew(D, "EST. REVENUE"), "review: before Partner the money tile reads WOULD HAVE EARNED, not paid");
		if (V)
		{
			V->X.SettledS = 1.05f;
		}
		L.Ch().X.Momentum = 1.5;
		L.U().Stats.clear();
		D = L.Frame();
		Check(!Drew(D, "Riding your breakout"), "review: an upload that got no carry shows none, however high Momentum is now");
		L.Ch().PartnerAt = L.S.WorldMinutes() - 1.0;
		D = L.Frame();
		Check(Drew(D, "EST. REVENUE") && !Drew(D, "WOULD HAVE EARNED"), "review: after Partner the tile is revenue");
	}
	// An Update 2 channel's thumbnails wait their turn too: two painted a frame, the rest skeletons (V12; m2).
	{
		Lap L("rerun3-review-legacy-paint");
		const char* Kinds[6] = {"badbeat", "bluff", "cooler", "herocall", "bigpot", "suckout"};
		for (int I = 0; I < 6; ++I)
		{
			L.S.DebugVideo(std::string("fake ") + Kinds[I] + " online");
			video::Choice C;
			C.Candidates = {Newest(L.S)};
			C.F = video::Format::Breakdown;
			C.T = static_cast<video::TitleStyle>(I % 5);
			C.Fr = static_cast<video::Frame>(I % 5); // six different frames: none replays another's cache
			video::UploadFacts Fa;
			Fa.Energy = 90.0;
			video::Upload(L.Ch(), C, L.S.WorldMinutes(), "rerun3-review-legacy", Fa);
			L.S.DebugVideo("advance 1");
			L.Rest();
		}
		Check(!L.Ch().X.On, "review: an Update 2 channel");
		L.S.Person.Appearance.Hair = (L.S.Person.Appearance.Hair + 7) % 12; // a look nothing is cached at
		L.Open(1);
		L.Frame();
		Check(L.U().Paint.Painted == 2 && L.U().Paint.Waiting > 0, "review: Update 2 frames paint two a frame, the rest wait as skeletons");
		int Frames = 1;
		while ((L.U().Paint.Painted > 0 || L.U().Paint.Waiting > 0) && Frames < 40)
		{
			L.Frame();
			++Frames;
		}
		Check(Frames >= 3 && L.U().Paint.Waiting == 0, "review: the Update 2 grid settles over several frames");
	}
	// RECOMMENDED is the video Rerun is testing on Home, not every upload sharing its title (Realtime's OnHomeNow lists titles).
	{
		Lap L("rerun3-review-home");
		Build(L, 6, false);
		const std::string NewId = PublishOne(L, false);
		const double W = L.S.WorldMinutes();
		video::Video* New = VideoById(L, NewId);
		video::Video* Old = nullptr;
		for (video::Video& V : L.Ch().Videos)
		{
			Old = V.Id != NewId && V.X.On && !rr::RrHomeTested(V, W) && (!Old || V.At < Old->At) ? &V : Old;
		}
		Check(New && Old && rr::RrHomeTested(*New, W), "review: a new upload is tested on Home, an old one isn't");
		// The UI's reading agrees with the core's (by title, where titles are unique).
		const video::RealtimeView Rt = video::Realtime(L.Ch(), W);
		int Agree = 0, Differ = 0;
		for (const video::Video& V : L.Ch().Videos)
		{
			int Same = 0;
			for (const video::Video& O : L.Ch().Videos)
			{
				Same += O.Title == V.Title ? 1 : 0;
			}
			if (Same == 1 && V.X.On)
			{
				const bool Core = std::find(Rt.OnHomeNow.begin(), Rt.OnHomeNow.end(), V.Title) != Rt.OnHomeNow.end();
				(Core == rr::RrHomeTested(V, W) ? Agree : Differ) += 1;
			}
		}
		Check(Differ == 0 && Agree >= 3,"review: RrHomeTested reads Home as the core's Realtime does (" + std::to_string(Agree) + " agree)");
		L.Open(3);
		DrawList D = L.Settled();
		const int Before = RunsOf(D, "RECOMMENDED");
		if (New && Old)
		{
			Old->Title = New->Title;
		}
		D = L.Settled();
		Check(Before >= 1 && RunsOf(D, "RECOMMENDED") == Before, "review: an old upload with the new one's title isn't RECOMMENDED with it");
		if (Old && New)
		{
			L.U().One = Old->Id;
			L.Open(2);
			L.U().Sub = 1;
			L.U().One = Old->Id;
			L.U().OneTab = 0;
			D = L.Settled();
			Check(Drew(D, "THE VERDICT") && !DrewRun(D, "RECOMMENDED ON HOME"), "review: the old one's page isn't RECOMMENDED ON HOME");
			L.U().One = New->Id;
			D = L.Settled();
			Check(DrewRun(D, "RECOMMENDED ON HOME"), "review: the new one's page is");
		}
	}
	// What the pages print adds up: a like ratio from the counts beside it, subscribers gained - lost = net; the Overview's
	// balance clear of Channel health's card.
	{
		Lap L("rerun3-review-sums");
		Build(L, 14, true);
		L.Open(2);
		DrawList D = L.Settled();
		const video::Range& Rg = L.U().Range;
		double G = 0.0, Lo = 0.0;
		for (const double X : Rg.SubsGained)
		{
			G += X;
		}
		for (const double X : Rg.SubsLost)
		{
			Lo += X;
		}
		const double NetS = std::floor(G - Lo), LostS = std::floor(Lo);
		Check(G >= Lo && Drew(D, "+" + ss::Grouped(static_cast<int64_t>(NetS))) && Drew(D,ss::Grouped(static_cast<int64_t>(NetS + LostS)) + " gained \xC2\xB7 " + ss::Grouped(static_cast<int64_t>(LostS)) + " lost"),
			"review: Overview's subscribers: gained - lost = the net it shows");
		const Run Bal = RunOf(D, "Balance ", true);
		Check(Bal.Found && Bal.Base + 3.0f < 582.0f + 230.0f, "review: the balance sits above Channel health's card");
		L.U().Sub = 1;
		D = L.Settled();
		int Rows = 0, Off = 0;
		for (const video::Video& V : L.Ch().Videos)
		{
			const video::VideoStats St = video::StatsOf(L.Ch(), V, L.S.WorldMinutes());
			const double Lf = std::floor(St.Likes), Df = std::floor(St.Dislikes);
			const std::string Counts = ss::Grouped(static_cast<int64_t>(Lf)) + " \xC2\xB7 " + ss::Grouped(static_cast<int64_t>(Df)) + "  ";
			if (!Drew(D, Counts))
			{
				continue;
			}
			++Rows;
			const double Ratio = Lf + Df > 0.0 ? Lf / (Lf + Df) : 1.0;
			Off += Drew(D, Counts + ss::Fixed(100.0 * Ratio, 1) + "%") ? 0 : 1;
		}
		Check(Rows >= 5 && Off == 0, "review: the Content table's like ratio is the counts' own (" + std::to_string(Rows) + " rows)");
		// Reach's CTR LIFETIME says "colder viewers later" only when it fell below the first 48 hours'.
		int Rose = 0, Fell = 0, Wrong = 0;
		for (const video::Video& V : L.Ch().Videos)
		{
			const video::VideoStats St = video::StatsOf(L.Ch(), V, L.S.WorldMinutes());
			if (V.F == video::Format::Short || St.Estimated || (St.CtrLife >= St.Ctr48 ? Rose : Fell) >= 2)
			{
				continue;
			}
			(St.CtrLife >= St.Ctr48 ? Rose : Fell) += 1;
			L.U().One = V.Id;
			L.U().OneTab = 1;
			D = L.Settled();
			Wrong += Drew(D, "colder viewers later") == (St.CtrLife < St.Ctr48) ? 0 : 1;
		}
		L.U().One.clear();
		Check(Rose + Fell >= 2 && Wrong == 0, "review: Reach's lifetime CTR note follows the numbers (" + std::to_string(Rose) + " rose, " + std::to_string(Fell) + " fell)");
	}
	// After 21+ dark days Studio's carry is the one the upload gets: Upload adds welcome back's +0.2 to H first (R1, §3.3).
	{
		Lap L("rerun3-review-welcome");
		Build(L, 4, false);
		L.U().LaunchHidden = true;
		video::Channel& Ch = L.Ch();
		L.S.DebugVideo("advance 25"); // 25 dark days
		L.Rest();
		Ch.X.MomH = 0.2;
		Ch.X.Momentum = std::clamp(std::exp(0.5 * 0.2), 0.75, 1.8) / 1.049; // MomCOf(0.2): about 1.05, no carry by itself
		L.S.DebugVideo("fake badbeat online");
		L.Open(0);
		const DrawList D = L.Settled();
		const int Dark = static_cast<int>(std::floor(L.S.WorldMinutes() / 1440.0)) - Ch.X.LastUploadDay;
		const std::string Id = PublishOne(L, false);
		const video::Video* V = VideoById(L, Id);
		Check(Dark >= 21 && Ch.X.WelcomeBack, "review: the upload after " + std::to_string(Dark) + " dark days got welcome back");
		const std::string Got = V ? "Riding your breakout: this upload starts with +" + std::to_string(std::llround(100.0 * (static_cast<double>(V->X.SettledS) - 1.0))) + "% reach" : std::string("?");
		Check(V && V->X.SettledS > 1.1f && Drew(D, Got), "review: Studio's carry after a break is the upload's own (" + Got + ")");
	}
	// Live Drop's hint under Studio's preview only for an upload it can push (a Rerun 2.0 one).
	{
		Lap L("rerun3-review-legacy-drop");
		L.S.DebugVideo("fake cooler online");
		L.Open(0);
		const DrawList D = L.Settled();
		const video::Choice C = rr::RrChoice(L.U());
		const bool U3 = L.Ch().X.On || !C.TitleTpl.empty() || !C.Thumbs.empty();
		Check(DrewRun(D, "PUBLISH") && Drew(D, "Go live within 6 hours") == U3, std::string("review: Studio's Live Drop hint only for a Rerun 2.0 upload (this one ") + (U3 ? "is" : "isn't") + ")");
	}
	// Kast's "Make it a Cut" lands in the editor as a Cut even when Studio hasn't shown the running launch yet.
	{
		Lap L("rerun3-review-cut");
		Build(L, 5, false);
		const std::string Id = PublishOne(L, false);
		L.S.DebugVideo("fake bluff online");
		const double W = L.S.WorldMinutes();
		rr::VideoOpenCut(L.RL, W - 5.0, W + 5.0, L.Now);
		const DrawList D = L.Settled();
		Check(!Id.empty() && L.U().F == video::Format::Short && DrewRun(D, "PUBLISH") && !Drew(D, "LAUNCH ACTIONS") && Drew(D, "BACK TO THE LAUNCH"),
			"review: Make it a Cut opens the Cut editor, the running launch one link away");
		Check(!L.U().NextUp.empty() && rr::RrChoice(L.U()).NextUp == L.U().NextUp, "review: the Cut links the newest long video as its Next-up card");
	}
}

// ------------------------------------------------------------------ the C3 fix round (c3f-ui): L9, M1, M2, H1

/** The app away for a while and back on Studio: a new visit. */
DrawList Revisit(Lap& L, int Page = 0)
{
	L.RL.OpenApp(ss::ui::RiverLine::App::Bank, L.Now);
	L.Frame();
	L.Now += 2.0;
	L.Open(Page);
	return L.Frame();
}

/** The ideas Studio reads this frame, replaced (RrIdeas' cache: its ten-minute bucket and the moments' count). */
void SetIdeas(Lap& L, const std::vector<video::IdeaCard>& Ideas)
{
	L.U().Ideas = Ideas;
	L.U().IdeasMinute = std::floor(L.S.WorldMinutes() / 10.0);
	L.U().IdeasOpen = L.Ch().Open.size();
}

video::IdeaCard FakeIdea(int Id, bool Ready, bool Trending)
{
	video::IdeaCard I;
	I.Id = Id;
	I.Name = Trending ? "Sunday grind" : "Bankroll update";
	I.Fact = "214 events, up $412";
	I.Hint = Ready ? std::string() : "Play 20 events this month first.";
	I.Formats = {video::Format::Breakdown};
	I.EditMin = 90;
	I.Energy = 8;
	I.Ready = Ready;
	I.Trending = Trending;
	return I;
}

void FixVisit()
{
	// L9 (update 3 §4.1): Studio opens on IDEAS when a new or trending idea waits, else on MOMENTS.
	Lap L("rerun3-fix-visit");
	Build(L, 3, false);
	L.U().LaunchHidden = true;
	L.S.DebugVideo("fake badbeat online");
	L.S.DebugVideo("fake bluff online");
	// The first look: the ready ideas then aren't new (a load's), a trending one waits until IDEAS shows it.
	std::vector<video::IdeaCard> Base = L.S.VideoIdeas();
	bool Trend = false;
	for (const video::IdeaCard& I : Base)
	{
		Trend = Trend || (I.Ready && I.Trending);
	}
	L.Open(0);
	L.Frame();
	Check(L.U().ListTab == (Trend ? 1 : 0), std::string("visit: the first visit opens on ") + (Trend ? "IDEAS (a trending idea waits)" : "MOMENTS (nothing new or trending)"));
	// Every idea seen (the list shown): the next visit opens on MOMENTS.
	L.U().ListTab = 1;
	L.Frame();
	DrawList D = Revisit(L);
	Check(L.U().ListTab == 0 && Drew(D, "BAD BEAT"), "visit: nothing new or trending: Studio opens on MOMENTS");
	// A new ready idea since the last visit: IDEAS, its card marked NEW. (The board from here is a short one of its own, so
	// every card shows: the real ideas were all seen above, and an idea that leaves the board is never new again.)
	std::vector<video::IdeaCard> More = {FakeIdea(9001, true, false)};
	L.RL.OpenApp(ss::ui::RiverLine::App::Bank, L.Now);
	L.Frame();
	L.Now += 2.0;
	SetIdeas(L, More);
	L.Open(0);
	D = L.Frame();
	Check(L.U().ListTab == 1 && L.U().NewIdeas.count(9001) == 1 && DrewRun(D, "NEW") && DrewRun(D, "Bankroll update"), "visit: a new ready idea opens Studio on IDEAS, marked NEW");
	WriteList("rerun3_fix_studio_ideas", D);
	// Seen on the list: the visit after opens on MOMENTS again, and the NEW chip is gone.
	SetIdeas(L, More);
	D = Revisit(L);
	SetIdeas(L, More);
	D = L.Frame();
	Check(L.U().ListTab == 0 && L.U().NewIdeas.empty(), "visit: once seen, the idea isn't new: MOMENTS");
	// A trending idea not seen yet: IDEAS.
	More.push_back(FakeIdea(9002, true, true));
	L.RL.OpenApp(ss::ui::RiverLine::App::Bank, L.Now);
	L.Frame();
	L.Now += 2.0;
	SetIdeas(L, More);
	L.Open(0);
	D = L.Frame();
	Check(L.U().ListTab == 1 && DrewRun(D, "TRENDING"), "visit: an unseen trending idea opens Studio on IDEAS");
	// A locked idea never opens IDEAS by itself.
	More.push_back(FakeIdea(9003, false, true));
	SetIdeas(L, More);
	L.Frame();
	D = Revisit(L);
	SetIdeas(L, More);
	L.Frame();
	Check(L.U().ListTab == 0, "visit: a locked idea, even trending, leaves Studio on MOMENTS");
	// The header's STUDIO tab from another page is a visit too; inside one visit the player's tab stays.
	More.push_back(FakeIdea(9004, true, false));
	SetIdeas(L, More);
	L.U().Page = rr::RrPageChannel;
	L.Frame();
	SetIdeas(L, More);
	Check(ClickRun(L, L.Frame(), "STUDIO") && L.U().Page == 0 && L.U().ListTab == 1, "visit: the STUDIO tab from the Channel follows the rule (a new idea: IDEAS)");
	L.Click(rr::RrListX + 40.0f, 90.0f);
	SetIdeas(L, More);
	L.Frame();
	L.Frame();
	Check(L.U().ListTab == 0, "visit: the player's MOMENTS click holds for the visit");
	// Callers that mean a tab: Kast's Cut highlights open MOMENTS, Research's links open IDEAS, whatever waits.
	More.push_back(FakeIdea(9005, true, true));
	SetIdeas(L, More);
	rr::VideoOpenCut(L.RL, L.S.WorldMinutes() - 90.0, L.S.WorldMinutes(), L.Now + 2.0);
	L.Now += 2.0;
	L.Frame();
	Check(L.U().ListTab == 0, "visit: Kast's Make it a Cut opens MOMENTS");
}

/** A dealt recipe for the newest moment (Studio's own deal), with the pickers' allowed values: false when the library deals
 *  none for it. */
bool StudioWithRecipes(Lap& L)
{
	L.U().LaunchHidden = true;
	L.Open(0);
	L.Frame();
	L.Click(rr::RrListX + 40.0f, 90.0f); // MOMENTS (L9 may open IDEAS)
	L.Settled();
	return L.U().Thumbs.size() == 6 && !L.U().Pick.empty();
}

void FixPickers()
{
	// M1 (update 3 §6.6.2): CAPTION, FACE and PALETTE on the selected recipe, inside its archetype's values; live; saved.
	Lap L("rerun3-fix-pickers");
	Build(L, 4, false);
	L.S.DebugVideo("fake badbeat online");
	Check(StudioWithRecipes(L), "pickers: the library deals six recipes for a bad beat");
	if (L.U().Thumbs.size() != 6)
	{
		return;
	}
	rr::VideoUi& U = L.U();
	DrawList D = L.Settled();
	const video::Choice Ch0 = rr::RrChoice(U);
	const video::ThumbChoices P = video::ThumbChoicesOf(U.Thumbs[static_cast<size_t>(U.ThumbPick)], Ch0, L.Ch().Open, L.Ch());
	std::printf("  rerun pickers  %s: %zu captions, %zu faces, %zu palettes, %zu caption styles\n", video::ArchName(U.Thumbs[static_cast<size_t>(U.ThumbPick)].Arch), P.Captions.size(),
		P.Exprs.size(), P.Pals.size(), P.CapStyles.size());
	Check(Spelled(D, "CAPTION") && Spelled(D, "PALETTE") && (P.Exprs.empty() || Spelled(D, "FACE")), "pickers: CAPTION, FACE and PALETTE under the preview");
	bool Chips = !P.Captions.empty();
	for (const auto& C : P.Captions)
	{
		Chips = Chips && (Drew(D, C.first) || Drew(D, C.first.substr(0, std::min<size_t>(6, C.first.size()))));
	}
	Check(Chips, "pickers: the archetype's caption rows on this moment, each a chip");
	Check(Outside(D, "pickers") == 0, "pickers: every run inside the laptop");
	WriteList("rerun3_fix_studio_pickers", D);
	// PALETTE: a press on another swatch recolours the selected recipe (the preview and its option at once).
	const video::ThumbSpec Was = U.Thumbs[static_cast<size_t>(U.ThumbPick)];
	int Other = -1;
	for (const int16_t Pl : P.Pals)
	{
		Other = Other < 0 && Pl != Was.Pal ? Pl : Other;
	}
	if (Other >= 0)
	{
		// The swatches sit on the second row (FACE's chips, then PALETTE's): find this one's by its clickable's spot (a press
		// that moved anything else is put back).
		const auto Same = [](const video::ThumbSpec& A, const video::ThumbSpec& B) {
			return A.Arch == B.Arch && A.Pal == B.Pal && A.Expr == B.Expr && A.CapStyle == B.CapStyle && A.Caption == B.Caption && A.Caption2 == B.Caption2;
		};
		bool Hit = false;
		for (float X = rr::RrEdX + 80.0f; X < rr::RrEdX + rr::RrEdW && !Hit; X += 22.0f)
		{
			for (float Y = U.OptTop + rr::RrThumbH + 50.0f; Y < U.OptTop + rr::RrThumbH + 90.0f && !Hit; Y += 10.0f)
			{
				L.Click(X, Y);
				const video::ThumbSpec& Now = U.Thumbs[static_cast<size_t>(U.ThumbPick)];
				Hit = Now.Pal == Other;
				if (!Hit && !Same(Now, Was))
				{
					U.Thumbs[static_cast<size_t>(U.ThumbPick)] = Was;
				}
			}
		}
		Check(Hit, "pickers: a swatch press recolours the selected recipe to that palette");
	}
	const video::ThumbSpec Pal = U.Thumbs[static_cast<size_t>(U.ThumbPick)];
	Check(Pal.Arch == Was.Arch && Pal.Expr == Was.Expr && Pal.Caption == Was.Caption && video::ThumbWithin(Pal, rr::RrChoice(U), L.Ch().Open, L.Ch()),
		"pickers: the palette moved alone, inside the archetype's values (ThumbWithin)");
	// FACE and CAPTION: their chips by their words.
	int Face = -1;
	for (const int16_t E : P.Exprs)
	{
		Face = Face < 0 && E != Pal.Expr ? E : Face;
	}
	const char* const Faces[12] = {"Neutral", "Smile", "Grin", "Laugh", "Shocked", "Scream", "Devastated", "Tilted", "Smug", "Skeptical", "Thinking", "Exhausted"};
	if (Face >= 0)
	{
		Check(ClickRun(L, L.Frame(), Faces[Face], U.OptTop + rr::RrThumbH) && U.Thumbs[static_cast<size_t>(U.ThumbPick)].Expr == Face, std::string("pickers: FACE ") + Faces[Face]);
	}
	int Cap = -1;
	for (size_t K = 0; K < P.Captions.size(); ++K)
	{
		Cap = Cap < 0 && P.Captions[K].first != Pal.Caption ? static_cast<int>(K) : Cap;
	}
	if (Cap >= 0)
	{
		const std::string Label = P.Captions[static_cast<size_t>(Cap)].second.empty() ? P.Captions[static_cast<size_t>(Cap)].first
																					: P.Captions[static_cast<size_t>(Cap)].first + " " + P.Captions[static_cast<size_t>(Cap)].second;
		Check(ClickRun(L, L.Frame(), Label, U.OptTop + rr::RrThumbH) && U.Thumbs[static_cast<size_t>(U.ThumbPick)].Caption == P.Captions[static_cast<size_t>(Cap)].first,
			"pickers: CAPTION \"" + Label + "\"");
		D = L.Settled();
		Check(Drew(D, P.Captions[static_cast<size_t>(Cap)].first), "pickers: the preview draws the picked caption (live)");
	}
	// The titles' SHUFFLE keeps the edits too: only the thumbnails' own SHUFFLE deals the six again (review-c3f-ui).
	{
		const video::ThumbSpec Was2 = U.Thumbs[static_cast<size_t>(U.ThumbPick)];
		const int Shuffles = U.TitleShuffle;
		bool Hit = false;
		const DrawList Dt = L.Frame();
		for (const ss::ui::DrawCmd& Cmd : Dt.Cmds)
		{
			if (!Hit && Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == "SHUFFLE" && Cmd.Text.BaselineY < U.OptTop - 40.0f)
			{
				L.Click(Cmd.Text.X + 10.0f, Cmd.Text.BaselineY - 5.0f);
				Hit = true;
			}
		}
		L.Settled();
		const video::ThumbSpec& Now = U.Thumbs[static_cast<size_t>(U.ThumbPick)];
		Check(Hit && U.TitleShuffle == Shuffles + 1 && Now.Arch == Was2.Arch && Now.Pal == Was2.Pal && Now.Expr == Was2.Expr && Now.Caption == Was2.Caption &&
				Now.Caption2 == Was2.Caption2,
			"pickers: the titles' SHUFFLE keeps the edited recipe");
	}
	// A title's style keeps the edits (the six aren't dealt again); the choice carries the edited recipe.
	const video::ThumbSpec Edited = U.Thumbs[static_cast<size_t>(U.ThumbPick)];
	Check(ClickRun(L, L.Frame(), "Question", 200.0f), "pickers: another title style");
	L.Settled();
	const video::Choice Ch = rr::RrChoice(U);
	Check(Ch.Thumbs.size() == 1 && Ch.Thumbs[0].Pal == Edited.Pal && Ch.Thumbs[0].Expr == Edited.Expr && Ch.Thumbs[0].Caption == Edited.Caption,
		"pickers: a title's style keeps the edited recipe, and the choice carries it");
	// A value the archetype doesn't allow leaves the recipe as it was (the core's guard).
	int Banned = -1;
	for (int Pl = 0; Pl < video::thumb::PlCount && Banned < 0; ++Pl)
	{
		Banned = std::find(P.Pals.begin(), P.Pals.end(), static_cast<int16_t>(Pl)) == P.Pals.end() ? Pl : -1;
	}
	if (Banned >= 0)
	{
		const video::ThumbSpec Same = video::ThumbEdit(Edited, Ch, L.Ch().Open, L.Ch(), video::TfPalette, Banned);
		Check(Same.Pal == Edited.Pal, "pickers: a palette outside the archetype's values changes nothing");
	}
	// PUBLISH: the edited recipe goes up as it was shown.
	const size_t Before = L.Ch().Videos.size();
	const std::string Why = L.S.Publish(Ch);
	for (int K = 0; K < 2000 && L.S.TimeSkip.Active; ++K)
	{
		L.Now += 1.0 / 30.0;
		L.S.Update(L.Now);
	}
	const video::Video* V = nullptr;
	for (const video::Video& X : L.Ch().Videos)
	{
		V = !V || X.At > V->At ? &X : V;
	}
	Check(Why.empty() && L.Ch().Videos.size() == Before + 1 && V && V->X.On && V->X.Spec[0].Arch == Edited.Arch && V->X.Spec[0].Pal == Edited.Pal && V->X.Spec[0].Expr == Edited.Expr &&
			V->X.Spec[0].Caption == Edited.Caption && V->X.Spec[0].Caption2 == Edited.Caption2,
		"pickers: PUBLISH saves the edited recipe with the upload (" + (Why.empty() ? std::string("published") : Why) + ")");
}

void FixDuelPickers()
{
	// The Duel's contenders are edited one at a time: the selected one only.
	Lap L("rerun3-fix-duel");
	Build(L, 4, false);
	L.S.DebugVideo("fake bluff online");
	if (!StudioWithRecipes(L))
	{
		Check(false, "duel pickers: the library deals six recipes for a bluff");
		return;
	}
	rr::VideoUi& U = L.U();
	Check(ClickRun(L, L.Frame(), "DUEL"), "duel pickers: DUEL on");
	L.Click(rr::RrEdX + rr::RrThumbW + 12.0f + rr::RrOptW + 8.0f + 60.0f, U.OptTop + 30.0f); // the second option joins as B
	L.Settled();
	const std::vector<video::ThumbSpec> Was = U.Thumbs;
	const int Sel = U.ThumbPick;
	const video::ThumbChoices P = video::ThumbChoicesOf(U.Thumbs[static_cast<size_t>(Sel)], rr::RrChoice(U), L.Ch().Open, L.Ch());
	int Other = -1;
	for (const int16_t Pl : P.Pals)
	{
		Other = Other < 0 && Pl != Was[static_cast<size_t>(Sel)].Pal ? Pl : Other;
	}
	if (Other >= 0)
	{
		const video::ThumbSpec E = video::ThumbEdit(U.Thumbs[static_cast<size_t>(Sel)], rr::RrChoice(U), L.Ch().Open, L.Ch(), video::TfPalette, Other);
		U.Thumbs[static_cast<size_t>(Sel)] = E;
	}
	bool OthersSame = true;
	for (size_t K = 0; K < Was.size(); ++K)
	{
		OthersSame = OthersSame && (static_cast<int>(K) == Sel || (U.Thumbs[K].Pal == Was[K].Pal && U.Thumbs[K].Caption == Was[K].Caption));
	}
	const video::Choice Ch = rr::RrChoice(U);
	bool Carried = false;
	for (const video::ThumbSpec& S : Ch.Thumbs)
	{
		Carried = Carried || (S.Arch == U.Thumbs[static_cast<size_t>(Sel)].Arch && S.Pal == U.Thumbs[static_cast<size_t>(Sel)].Pal);
	}
	Check(U.DuelOn && Ch.Thumbs.size() == 2 && OthersSame && Carried, "duel pickers: an edit moves the selected contender only, and the Duel carries it");
	WriteList("rerun3_fix_studio_duel", L.Settled());
	// Through the pickers themselves (review-c3f-ui): a press edits the contender shown (B) alone; a press on A, a contender that
	// isn't shown, selects it and keeps it in the Duel, and the next press edits A alone; a press on the one shown takes it out.
	const auto Press = [&L, &U]() {
		// Another caption row, else another face (by their chips' words, under the preview): true when one was pressed.
		const size_t S = static_cast<size_t>(U.ThumbPick);
		const video::ThumbChoices C = video::ThumbChoicesOf(U.Thumbs[S], rr::RrChoice(U), L.Ch().Open, L.Ch());
		const float Below = U.OptTop + rr::RrThumbH;
		for (const auto& Cp : C.Captions)
		{
			if (Cp.first != U.Thumbs[S].Caption && ClickRun(L, L.Frame(), Cp.second.empty() ? Cp.first : Cp.first + " " + Cp.second, Below))
			{
				return true;
			}
		}
		const char* const Faces[12] = {"Neutral", "Smile", "Grin", "Laugh", "Shocked", "Scream", "Devastated", "Tilted", "Smug", "Skeptical", "Thinking", "Exhausted"};
		for (const int16_t E : C.Exprs)
		{
			if (E != U.Thumbs[S].Expr && E >= 0 && E < 12 && ClickRun(L, L.Frame(), Faces[E], Below))
			{
				return true;
			}
		}
		return false;
	};
	const auto Moved = [](const video::ThumbSpec& X, const video::ThumbSpec& Y) {
		return X.Pal != Y.Pal || X.Expr != Y.Expr || X.CapStyle != Y.CapStyle || X.Caption != Y.Caption || X.Caption2 != Y.Caption2;
	};
	const int A = U.Duel.size() == 2 ? U.Duel.front() : -1, B = U.ThumbPick;
	const auto TileOf = [&U](int K) {
		return ss::ui::Vec2{rr::RrEdX + rr::RrThumbW + 12.0f + static_cast<float>(K % 2) * (rr::RrOptW + 8.0f) + 60.0f, U.OptTop + static_cast<float>(K / 2) * (rr::RrOptH + 8.0f) + 30.0f};
	};
	if (A >= 0 && A != B)
	{
		std::vector<video::ThumbSpec> Was2 = U.Thumbs;
		const bool PressB = Press();
		L.Settled();
		Check(PressB && Moved(U.Thumbs[static_cast<size_t>(B)], Was2[static_cast<size_t>(B)]) && !Moved(U.Thumbs[static_cast<size_t>(A)], Was2[static_cast<size_t>(A)]),
			"duel pickers: a picker press edits the contender shown (B) alone");
		L.Click(TileOf(A).X, TileOf(A).Y);
		L.Settled();
		Check(U.ThumbPick == A && U.Duel.size() == 2 && U.DuelOn, "duel pickers: a press on A, not the one shown, selects it and keeps it in the Duel");
		Was2 = U.Thumbs;
		const bool PressA = Press();
		L.Settled();
		Check(PressA && Moved(U.Thumbs[static_cast<size_t>(A)], Was2[static_cast<size_t>(A)]) && !Moved(U.Thumbs[static_cast<size_t>(B)], Was2[static_cast<size_t>(B)]),
			"duel pickers: then a picker press edits A alone");
		L.Click(TileOf(A).X, TileOf(A).Y);
		L.Settled();
		Check(U.Duel.size() == 1 && U.Duel.front() == B, "duel pickers: a press on the contender shown takes it out of the Duel");
	}
	else
	{
		Check(false, "duel pickers: a Duel of two, B shown");
	}
}

void FixResearch()
{
	// M2 (§5.8): "What poker viewers search": the phrases, their interest, the week's arrow, yours, MAKE IT.
	for (const bool Partner : {false, true})
	{
		const std::string Tag = Partner ? "partner" : "before";
		Lap L("rerun3-fix-research-" + Tag);
		Build(L, 8, Partner);
		L.Open(2);
		L.U().Sub = 5;
		DrawList D = L.Settled();
		const std::vector<video::SearchRow> Rows = video::SearchesOf(L.S, L.S.VideoIdeas(), 10);
		std::string Interest;
		for (const video::SearchRow& R : Rows)
		{
			Interest += (Interest.empty() ? "" : " ") + std::to_string(R.Interest) + (R.Arrow > 0 ? "+" : R.Arrow < 0 ? "-" : "=");
		}
		std::printf("  rerun search   %zu rows%s, interest %s\n", Rows.size(), Rows.empty() ? "" : (", top \"" + Rows.front().Phrase + "\"").c_str(), Interest.c_str());
		Check(Drew(D, "WHAT POKER VIEWERS SEARCH") && DrewRun(D, "INTEREST") && DrewRun(D, "YOURS"), "research " + Tag + ": the search table and its columns");
		bool All = !Rows.empty();
		int Shown = 0, Make = 0;
		for (const video::SearchRow& R : Rows)
		{
			const bool In = Drew(D, R.Phrase.substr(0, std::min<size_t>(R.Phrase.size(), 18)));
			Shown += In ? 1 : 0;
			Make += R.Idea > 0 ? 1 : 0;
		}
		All = All && Shown >= std::min<int>(8, static_cast<int>(Rows.size()));
		Check(All, "research " + Tag + ": the phrases, the most searched first (" + std::to_string(Shown) + " of " + std::to_string(Rows.size()) + " drawn)");
		Check(Make == 0 || RunsOf(D, "MAKE IT \xE2\x80\xBA") >= std::min(Make, 8), "research " + Tag + ": MAKE IT on each row with an idea");
		Check(Drew(D, "TRENDING IN POKER THIS WEEK") && Drew(D, "YOUR NICHE") && (Partner ? Drew(D, "HOW RERUN DECIDES") : Drew(D, "THE AD MARKET THIS WEEK")),
			"research " + Tag + ": the trends, the niche, and the help card (Partner) or the Ad market");
		Check(Outside(D, ("research fix " + Tag).c_str()) == 0, "research " + Tag + ": every run inside the laptop");
		WriteList("rerun3_fix_research_" + Tag, D);
		Measure(L, "research (search) " + Tag);
		if (!Partner)
		{
			// The help behind its "?": opens over the page, a click off it closes it.
			L.Click(20.0f + 760.0f - 28.0f, 142.0f);
			D = L.Frame();
			Check(L.U().HelpOpen == "research" && Drew(D, "HOW RERUN DECIDES") && Drew(D, "Nobody can promise a number"), "research before: the \"?\" opens how Rerun decides");
			WriteList("rerun3_fix_research_help", D);
			L.Click(400.0f, 900.0f);
			Check(L.U().HelpOpen.empty(), "research before: a click off the card closes it");
		}
		// MAKE IT on a ready idea: Studio, the idea's candidate picked; on a locked one: IDEAS and what to do.
		const video::SearchRow* Ready = nullptr;
		const video::SearchRow* Locked = nullptr;
		for (const video::SearchRow& R : Rows)
		{
			Ready = !Ready && R.Idea > 0 && R.IdeaReady ? &R : Ready;
			Locked = !Locked && R.Idea > 0 && !R.IdeaReady ? &R : Locked;
		}
		for (const video::SearchRow* R : {Locked, Ready})
		{
			if (!R)
			{
				continue;
			}
			L.U().Page = 2;
			L.U().Sub = 5;
			D = L.Settled();
			const Run Phrase = RunOf(D, R->Phrase.substr(0, std::min<size_t>(R->Phrase.size(), 18)), true, 150.0f);
			bool Clicked = false;
			for (const ss::ui::DrawCmd& Cmd : D.Cmds)
			{
				if (!Clicked && Phrase.Found && Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == "MAKE IT \xE2\x80\xBA" && std::fabs(Cmd.Text.BaselineY - Phrase.Base) < 2.0f)
				{
					L.Click(Cmd.Text.X + 20.0f, Cmd.Text.BaselineY - 4.0f);
					Clicked = true;
				}
			}
			L.Frame();
			if (R == Ready)
			{
				const video::Candidate* C = rr::RrFind(L.Ch().Open, L.U().Pick.empty() ? std::string() : L.U().Pick.front());
				Check(Clicked && L.U().Page == 0 && L.U().ListTab == 1 && C && C->Source == 5 && C->Idea == R->Idea, "research " + Tag + ": MAKE IT on a ready idea: Studio with it picked (" + R->IdeaName + ")");
			}
			else
			{
				Check(Clicked && L.U().Page == 0 && L.U().ListTab == 1 && L.U().Note == (R->IdeaHint.empty() ? R->IdeaName : R->IdeaHint), "research " + Tag + ": MAKE IT on a locked idea: IDEAS, and what to do first");
			}
		}
	}
}

/**
 * The pay month's days before today, paid past the top-earner rate's start (the rings PaidThisMonth reads): one big day, the
 * 28 days' own mix of views, watch time, ads and the Cuts feed k times over, paid as the core pays it (the day's row, the
 * videos' earnings in their own shares, the balance), so the page's other numbers (RPM, CPM, HOW YOU EARNED, the balance)
 * agree with the rate's line. The page's caches are read again (a day's step moves their keys in play).
 */
bool PastTopRate(Lap& L)
{
	video::Channel& Ch = L.Ch();
	const int Today = ss::net::DayOf(L.S.WorldMinutes());
	const int Month = video::valgo::PayMonthOf(Today);
	video::DayRow Mix;
	for (const video::DayRow& D : Ch.X.Days)
	{
		if (D.Key >= Today - 28 && D.Key < Today)
		{
			Mix.Views += D.Views;
			Mix.WatchH += D.WatchH;
			Mix.Impr += D.Impr;
			Mix.Cents += D.Cents;
			Mix.CutCents += D.CutCents;
			Mix.CutViews += D.CutViews;
			Mix.AdImpr += D.AdImpr;
			Mix.Monetized += D.Monetized;
		}
	}
	double Earned = 0.0;
	for (const video::Video& V : Ch.Videos)
	{
		Earned += V.X.On ? V.X.Earned : 0.0;
	}
	if (Mix.Cents <= 0 || Earned <= 0.0)
	{
		return false;
	}
	for (auto It = Ch.X.Days.rbegin(); It != Ch.X.Days.rend(); ++It)
	{
		if (It->Key < Today && video::valgo::PayMonthOf(It->Key) == Month)
		{
			const ss::Chips Big = video::valgo::TopRateFrom() + 120000;
			const double K = static_cast<double>(Big) / static_cast<double>(Mix.Cents);
			const auto Times = [K](float X) { return static_cast<float>(static_cast<double>(X) * K); };
			It->Cents += Big;
			It->CutCents += static_cast<ss::Chips>(std::llround(static_cast<double>(Mix.CutCents) * K));
			It->Views += Times(Mix.Views);
			It->WatchH += Times(Mix.WatchH);
			It->Impr += Times(Mix.Impr);
			It->CutViews += Times(Mix.CutViews);
			It->AdImpr += Times(Mix.AdImpr);
			It->Monetized += Times(Mix.Monetized);
			const double Scale = 1.0 + static_cast<double>(Big) / Earned;
			for (video::Video& V : Ch.Videos)
			{
				V.X.Earned *= V.X.On ? Scale : 1.0;
			}
			Ch.BalanceCents += Big;
			L.U().RangeMinute = -1.0;
			L.U().Stats.clear();
			return true;
		}
	}
	return false;
}

void FixTopRate()
{
	// H1 (the user's soft ceiling, 2026-10-27): the Revenue page and the Overview say so when the month passed it.
	Lap L("rerun3-fix-toprate");
	Build(L, 14, true);
	L.Open(2);
	L.U().Sub = 3;
	DrawList D = L.Settled();
	Check(!Drew(D, "Top-earner rate") && !Drew(D, "top-earner") && !Drew(D, "full rate"), "top rate: a month under it says nothing of it");
	// A video's RPM A DAY (review-c3f-ui, review-c3f-algo's F2): under the rate, every day at the full rate.
	const video::Video* Long = nullptr;
	for (const video::Video& V : L.Ch().Videos)
	{
		Long = !Long && V.F != video::Format::Short && V.X.On ? &V : Long;
	}
	const int Today = ss::net::DayOf(L.S.WorldMinutes());
	if (Long)
	{
		const std::vector<double> Under = rr::RrRpmADay(L.Ch(), *Long, Today, 28, L.S.WorldMinutes());
		bool Full = Under.size() == 28;
		for (size_t K = 0; K < Under.size(); ++K)
		{
			Full = Full && Under[K] == video::algo::MoneyOf(*Long, Today - 27 + static_cast<int>(K)).Rpm * 100.0;
		}
		Check(Full, "top rate: under it, a video's RPM A DAY is the full rate every day");
	}
	Check(PastTopRate(L), "top rate: the month's paid cents past 6 rents");
	D = L.Settled();
	const video::valgo::TopRateView Top = video::valgo::TopRateOf(L.Ch(), L.S.WorldMinutes());
	const double Share = Top.Share;
	if (Long)
	{
		// Past it, today's point is what the RPM tile above it reads ("per 1,000 views today"), not the full rate.
		const std::vector<double> Past = rr::RrRpmADay(L.Ch(), *Long, Today, 28, L.S.WorldMinutes());
		const video::VideoStats St = video::StatsOf(L.Ch(), *Long, L.S.WorldMinutes());
		const double FullToday = video::algo::MoneyOf(*Long, Today).Rpm * 100.0;
		Check(!Past.empty() && std::fabs(Past.back() - St.Rpm * 100.0) < 1e-6 && Past.back() < 0.9 * FullToday,
			"top rate: a video's RPM A DAY ends on the reduced share its RPM tile reads (" + ss::Money(static_cast<ss::Chips>(std::llround(Past.empty() ? 0.0 : Past.back()))) + " of " +
				ss::Money(static_cast<ss::Chips>(std::llround(FullToday))) + ")");
	}
	Check(Top.Applies && Drew(D, video::valgo::TopRateLine().substr(0, 30)), "top rate: Revenue's HOW YOU EARNED carries the rate's line (the core's words)");
	Check(Top.Held > 0 ? Drew(D, "Held back so far this month: " + ss::Money(Top.Held)) : Drew(D, "The next views pay " + rr::RrPct(Share, 0) + " of the full rate."),
		"top rate: what it held back (or what the next views pay), the core's numbers");
	Check(Outside(D, "revenue top rate") == 0, "top rate: every run inside the laptop");
	WriteList("rerun3_fix_revenue_toprate", D);
	// The "?" card: the rule in words, every amount the game's rent's.
	L.Click(1156.0f + 12.0f + 400.0f - 20.0f, 246.0f + 146.0f + 20.0f); // the block's "?" (HOW YOU EARNED at 1156, 246)
	D = L.Frame();
	Check(L.U().HelpOpen == "toprate" && Drew(D, video::valgo::TopRateHelp().substr(0, 40)), "top rate: the \"?\" opens its card with the core's words");
	{
		// Under the block it explains: the rate's line and what it means stay readable (review-c3f-ui).
		const Run Means = RunOf(D, Top.Held > 0 ? "Held back so far this month" : "The next views pay", true);
		Check(Means.Found && L.U().HelpBox.Y > Means.Base + 2.0f, "top rate: the \"?\" card opens below the rate's lines");
	}
	WriteList("rerun3_fix_revenue_toprate_help", D);
	// A click inside the card clicks nothing under it; a click off it closes it.
	const int Sub = L.U().Sub;
	L.Click(L.U().HelpBox.X + 40.0f, L.U().HelpBox.Y + 40.0f);
	Check(L.U().HelpOpen == "toprate" && L.U().Sub == Sub, "top rate: a click inside the card stays in it");
	L.Click(300.0f, 400.0f);
	Check(L.U().HelpOpen.empty(), "top rate: a click off the card closes it");
	// Overview's HOW THE MONEY ADDS UP: the share the next view pays, named.
	L.U().Sub = 0;
	D = L.Settled();
	Check(Drew(D, "top-earner share") && Drew(D, "\xC3\x97 " + rr::RrPct(0.55 * Share, 0)), "top rate: the Overview's money card shows the reduced share");
	// A top earner's TOP CONTENT: revenues in the thousands stay in their column, clear of LIKES (review-c3f-ui; the titles and
	// the values, right of the thumbnails).
	Check(Crowded(D, {20.0f + 136.0f, 600.0f, 700.0f - 136.0f, 330.0f}, "overview top content", 4.0f) == 0, "top rate: TOP CONTENT's values keep apart (4 units or more)");
	WriteList("rerun3_fix_overview_toprate", D);
	// Videos that run to millions of views: the titles give way to the counts (the closed form scaled; the caches read again).
	for (video::Video& V : L.Ch().Videos)
	{
		V.P *= 2000.0;
		V.E *= 2000.0;
		V.X.P0 *= 2000.0;
		for (video::Comp& K : V.X.Comps)
		{
			K.P *= 2000.0;
		}
	}
	L.U().RangeMinute = -1.0;
	L.U().Stats.clear();
	D = L.Settled();
	Check(Crowded(D, {20.0f + 136.0f, 600.0f, 700.0f - 136.0f, 330.0f}, "overview top content, millions", 4.0f) == 0, "top rate: TOP CONTENT's titles give way to view counts in the millions");
}
} // namespace video3_c3

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
	// C3: Rerun 2.0's pages.
	video3_c3::Shell();
	video3_c3::Studio();
	video3_c3::Launch();
	video3_c3::Analytics(false);
	video3_c3::Analytics(true);
	video3_c3::Public();
	video3_c3::Sheet();
	video3_c3::Progressive();
	video3_c3::Legacy();
	video3_c3::Copy();
	video3_c3::Review();
	// The C3 fix round (c3f-ui): Studio's visit rule (L9), the pickers (M1), Research's search table (M2), the top-earner
	// rate (H1).
	video3_c3::FixVisit();
	video3_c3::FixPickers();
	video3_c3::FixDuelPickers();
	video3_c3::FixResearch();
	video3_c3::FixTopRate();
	std::printf("  ui-rerun (C3): %d checks\n", video3_c3::Checks);
	std::fflush(stdout); // ui-rerun's lines reach the log even when a later segment stops the run
}
} // namespace ui_test
