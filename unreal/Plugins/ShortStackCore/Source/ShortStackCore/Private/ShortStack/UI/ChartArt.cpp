// Update 3 (SPEC §5.2, §7.3): the chart kit, ui-rerun's file (Rerun's Analytics; RunItTwice's Desk, League and Rankings).
//
// One look per product, through chart::Theme: Rerun's marigold on ink (drawn outside every SkinScope), any site's 16 tokens,
// and RunItTwice's Exchange (SPEC §3.3: Signal Red light on black, BoldCondensed caps, every figure of 28 units or more
// printed twice, square tiles with one 45-degree cut). The kit draws and formats the numbers it is handed and computes none
// (§5.10's data sources are the core's). Static layers (grid and axes; areas, lines and bars) are cached through
// Canvas::Cached by everything they draw; the reveal's clip, the hover, the last point and the markers draw live.
//
// Areas and bands are quad strips written straight into the draw list (no ear clipping, so a 365-point area stays linear,
// and no fringe: the line drawn over an area carries its edge). Private names carry the Chart prefix (the unity rules, §7).
#include "ShortStack/UI/ChartArt.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/UI/VideoArt.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace ui
{
namespace chart
{
// ------------------------------------------------------------------ private helpers (not in the header; Chart-prefixed)

/** FNV-1a over everything a cached layer draws. */
struct ChartHash
{
	uint64_t H = 0xcbf29ce484222325ull;
	void Bytes(const void* P, size_t N)
	{
		const unsigned char* B = static_cast<const unsigned char*>(P);
		for (size_t K = 0; K < N; ++K)
		{
			H = (H ^ B[K]) * 0x100000001b3ull;
		}
	}
	void Fl(float V) { Bytes(&V, sizeof(V)); }
	void Db(double V) { Bytes(&V, sizeof(V)); }
	void In(int64_t V) { Bytes(&V, sizeof(V)); }
	void St(const std::string& V)
	{
		In(static_cast<int64_t>(V.size()));
		Bytes(V.data(), V.size());
	}
	void Co(const Color& V)
	{
		Fl(V.R);
		Fl(V.G);
		Fl(V.B);
		Fl(V.A);
	}
	void Dv(const std::vector<double>& V)
	{
		In(static_cast<int64_t>(V.size()));
		if (!V.empty())
		{
			Bytes(V.data(), V.size() * sizeof(double));
		}
	}
	void Re(const Rect& V)
	{
		Fl(V.X);
		Fl(V.Y);
		Fl(V.W);
		Fl(V.H);
	}
};

enum class ChartRole : uint8_t
{
	Label,
	Figure,
	Axis,
	Body,
};

Color ChartOr(const Color& C, const Color& Fallback)
{
	return C.A > 0.0f ? C : Fallback;
}

Color ChartFade(Color C, float A)
{
	C.A *= A;
	return C;
}

bool ChartSame(const Color& A, const Color& B)
{
	return A.R == B.R && A.G == B.G && A.B == B.B && A.A == B.A;
}

/** WCAG 2.x relative luminance of one channel and of a colour (the formula the sites' contrast tests use). */
double ChartLinear(float V)
{
	const double Ch = static_cast<double>(V);
	return Ch <= 0.03928 ? Ch / 12.92 : std::pow((Ch + 0.055) / 1.055, 2.4);
}

double ChartLuminance(const Color& C)
{
	return 0.2126 * ChartLinear(C.R) + 0.7152 * ChartLinear(C.G) + 0.0722 * ChartLinear(C.B);
}

/** The words on a filled mark: the theme's ground or its figure colour, whichever reads higher on it (Rerun's ink on
 * marigold as drawn; RunItTwice's Hot White on Signal Red, 5.45:1, where the ground would read 3.59:1). */
Color ChartInkOn(const chart::Theme& T, const Color& Fill)
{
	const double Lf = ChartLuminance(Fill);
	auto Ratio = [Lf](const Color& Words) {
		const double Lw = ChartLuminance(Words);
		return (std::max(Lf, Lw) + 0.05) / (std::min(Lf, Lw) + 0.05);
	};
	return Ratio(T.Value) > Ratio(T.Ground) ? T.Value : T.Ground;
}

std::string ChartCaps(const std::string& S)
{
	std::string Out = S;
	for (char& Ch : Out)
	{
		if (Ch >= 'a' && Ch <= 'z')
		{
			Ch = static_cast<char>(Ch - 'a' + 'A');
		}
	}
	return Out;
}

TextStyle ChartStyle(const chart::Theme& T, ChartRole Role, float Size, const Color& Col, Align A, Baseline B)
{
	TextStyle St;
	St.Size = Size;
	St.Col = Col;
	St.HAlign = A;
	St.VAlign = B;
	switch (Role)
	{
	case ChartRole::Label:
		St.Weight = T.LabelWeight;
		St.Face = T.LabelFace;
		break;
	case ChartRole::Figure:
		St.Weight = T.FigureWeight;
		St.Face = T.FigureFace;
		break;
	case ChartRole::Axis:
		St.Weight = T.AxisWeight;
		St.Face = T.AxisFace;
		break;
	case ChartRole::Body:
		St.Weight = T.BodyWeight;
		St.Face = -1;
		break;
	}
	return St;
}

std::string ChartWords(const chart::Theme& T, ChartRole Role, const std::string& S)
{
	return Role == ChartRole::Label && T.Caps ? ChartCaps(S) : S;
}

float ChartMeasure(const Canvas& C, const chart::Theme& T, ChartRole Role, const std::string& S, float Size)
{
	const TextStyle St = ChartStyle(T, Role, Size, Color(), Align::Left, Baseline::Alphabetic);
	const std::string Words = ChartWords(T, Role, S);
	return St.Face >= 0 ? C.Measure(Words, Size, static_cast<Font>(St.Face)) : C.Measure(Words, Size, St.Weight, false);
}

float ChartPrint(Canvas& C, const chart::Theme& T, ChartRole Role, const std::string& S, float X, float Y, float Size, const Color& Col,
	Align A = Align::Left, Baseline B = Baseline::Alphabetic, float MaxW = 0.0f)
{
	TextStyle St = ChartStyle(T, Role, Size, Col, A, B);
	St.MaxWidth = MaxW;
	return C.Text(ChartWords(T, Role, S), X, Y, St);
}

/** A figure, printed twice in a theme with an under-copy (SPEC §3.3): Under's copy at (+o, +o), the readable copy on top. */
float ChartFigure(Canvas& C, const chart::Theme& T, const std::string& S, float X, float Y, float Size, const Color& Col, bool Twice, Align A, float MaxW)
{
	if (Twice && T.Under.A > 0.0f && Size >= T.TwiceFrom)
	{
		const float O = chart::TwiceOffset(Size);
		ChartPrint(C, T, ChartRole::Figure, S, X + O, Y + O, Size, T.Under, A, Baseline::Alphabetic, MaxW);
	}
	return ChartPrint(C, T, ChartRole::Figure, S, X, Y, Size, Col, A, Baseline::Alphabetic, MaxW);
}

/** A tile's outline: square with one 45-degree cut on the top-right corner. */
std::vector<Vec2> ChartCutShape(const Rect& R, float Cut)
{
	const float K = std::min(Cut, std::min(R.W, R.H) * 0.5f);
	return {{R.X, R.Y}, {R.X + R.W - K, R.Y}, {R.X + R.W, R.Y + K}, {R.X + R.W, R.Y + R.H}, {R.X, R.Y + R.H}};
}

/** A tile's ground (a top-to-bottom gradient) and its 1-unit edge, drawn inside R. */
void ChartPanel(Canvas& C, const Rect& R, const Color& Top, const Color& Bottom, const Color& Edge, float Radius, float Cut)
{
	if (R.W <= 1.0f || R.H <= 1.0f)
	{
		return;
	}
	const Paint P = ChartSame(Top, Bottom) ? Paint(Top) : Paint::Linear({R.X, R.Y}, {R.X, R.Y + R.H}, Top, Bottom);
	const Rect In{R.X + 0.5f, R.Y + 0.5f, R.W - 1.0f, R.H - 1.0f};
	if (Cut > 0.0f)
	{
		C.FillPolygon(ChartCutShape(R, Cut), P);
		if (Edge.A > 0.0f)
		{
			C.StrokePolyline(ChartCutShape(In, Cut - 0.2f), true, Edge, 1.0f);
		}
		return;
	}
	const float Rad = std::min(Radius, std::min(R.W, R.H) * 0.5f);
	C.FillRoundRect(R, Rad, P);
	if (Edge.A > 0.0f)
	{
		C.StrokeRoundRect(In, std::max(0.0f, Rad - 0.5f), Edge, 1.0f);
	}
}

/**
 * Quads between two polylines of one length, straight into the list: no fringe (the line drawn over it carries the edge)
 * and no ear clipping (a 365-point area stays linear). Colours per vertex; the canvas's transform and alpha apply.
 */
void ChartStrip(Canvas& C, const std::vector<Vec2>& Upper, const std::vector<Color>& UpperCol, const std::vector<Vec2>& Lower,
	const std::vector<Color>& LowerCol)
{
	const size_t N = std::min(std::min(Upper.size(), Lower.size()), std::min(UpperCol.size(), LowerCol.size()));
	const float Al = C.GetAlpha();
	if (N < 2 || Al <= 0.001f)
	{
		return;
	}
	DrawList& L = C.List();
	const Affine& Xf = C.Transform();
	size_t From = 0;
	while (From + 1 < N)
	{
		const size_t To = std::min(N, From + 4096); // a run stays far under the list's 60k-vertex command
		const uint32_t Base = L.BeginTriangles((To - From) * 2);
		for (size_t K = From; K < To; ++K)
		{
			const Vec2 U = Xf.Apply(Upper[K]);
			const Vec2 D = Xf.Apply(Lower[K]);
			L.AddVertex(U.X, U.Y, ChartFade(UpperCol[K], Al));
			L.AddVertex(D.X, D.Y, ChartFade(LowerCol[K], Al));
		}
		for (size_t K = 0; K + 1 < To - From; ++K)
		{
			const uint32_t A0 = Base + static_cast<uint32_t>(K * 2);
			L.AddTriangle(A0, A0 + 1, A0 + 2);
			L.AddTriangle(A0 + 1, A0 + 3, A0 + 2);
		}
		From = To - 1;
	}
}

/**
 * One run of points as an area down (or up) to BaseY: a point is inserted wherever a segment crosses the base, so every
 * quad sits on one side of it; each point's alpha runs from Area x Fade at the base to Area at Reach units from it.
 */
void ChartArea(Canvas& C, const std::vector<Vec2>& Run, float BaseY, const Color& Col, float Area, float Fade, float Reach)
{
	if (Run.size() < 2 || Area <= 0.0f)
	{
		return;
	}
	std::vector<Vec2> Up;
	Up.reserve(Run.size() * 2);
	for (size_t K = 0; K < Run.size(); ++K)
	{
		if (K > 0)
		{
			const Vec2 P0 = Run[K - 1];
			const Vec2 P1 = Run[K];
			const float D0 = P0.Y - BaseY;
			const float D1 = P1.Y - BaseY;
			if ((D0 < 0.0f && D1 > 0.0f) || (D0 > 0.0f && D1 < 0.0f))
			{
				const float At = D0 / (D0 - D1);
				Up.push_back({P0.X + (P1.X - P0.X) * At, BaseY});
			}
		}
		Up.push_back(Run[K]);
	}
	std::vector<Vec2> Down;
	std::vector<Color> UpCol;
	std::vector<Color> DownCol;
	Down.reserve(Up.size());
	UpCol.reserve(Up.size());
	DownCol.reserve(Up.size());
	const float F = std::max(0.0f, std::min(1.0f, Fade));
	for (const Vec2& P : Up)
	{
		Down.push_back({P.X, BaseY});
		const float Dist = Reach > 0.0f ? std::min(1.0f, std::fabs(P.Y - BaseY) / Reach) : 1.0f;
		UpCol.push_back(ChartFade(Col, Area * (F + (1.0f - F) * Dist)));
		DownCol.push_back(ChartFade(Col, Area * F));
	}
	ChartStrip(C, Up, UpCol, Down, DownCol);
}

/** Draws Paint at R's origin, through the cache under Key or straight (Cache false). */
void ChartLayer(Canvas& C, bool Cache, uint64_t Key, const Rect& R, const std::function<void(Canvas&)>& PaintFn)
{
	if (Cache)
	{
		C.Cached(Key, R.X, R.Y, PaintFn);
		return;
	}
	C.Save();
	C.Translate(R.X, R.Y);
	PaintFn(C);
	C.Restore();
}

/** Decimals that write every multiple of Step exactly (at most Max). */
int ChartDecimals(double Step, int Max)
{
	if (!(Step > 0.0) || !std::isfinite(Step))
	{
		return 0;
	}
	for (int D = 0; D < Max; ++D)
	{
		const double Scaled = Step * std::pow(10.0, D);
		if (std::fabs(Scaled - std::round(Scaled)) < 1e-6 * std::max(1.0, Scaled))
		{
			return D;
		}
	}
	return Max;
}

/** "1.50" -> "1.5", "2.00" -> "2". */
std::string ChartTrim(std::string S)
{
	if (S.find('.') == std::string::npos)
	{
		return S;
	}
	while (!S.empty() && S.back() == '0')
	{
		S.pop_back();
	}
	if (!S.empty() && S.back() == '.')
	{
		S.pop_back();
	}
	return S;
}

/** A magnitude with a K, M or B suffix past a thousand (Step: the axis's step, for the decimals). */
std::string ChartShort(double A, double Step)
{
	struct ChartSuffix
	{
		double Size;
		const char* Tag;
	};
	const ChartSuffix Suffixes[4] = {{1e9, "B"}, {1e6, "M"}, {1e3, "K"}, {1.0, ""}};
	for (const ChartSuffix& U : Suffixes)
	{
		if (A >= U.Size * 0.9995 || U.Size == 1.0)
		{
			const double V = A / U.Size;
			int D = 0;
			if (Step > 0.0)
			{
				D = ChartDecimals(Step / U.Size, 3);
			}
			else
			{
				D = U.Size == 1.0 ? (std::fabs(V - std::round(V)) < 1e-9 ? 0 : 1) : (V < 10.0 ? 1 : 0);
			}
			std::string Out = ChartTrim(Fixed(V, D));
			if (Out == "1000" && U.Size < 1e9)
			{
				continue; // 999,950 is "1M", not "1000K"
			}
			return Out + U.Tag;
		}
	}
	return "0";
}

/** The layout every axis chart shares, in R's own space (R's origin at 0, 0). */
struct ChartFrame
{
	Rect Plot;
	chart::AxisScale Ys;
	std::vector<std::string> YText;
	std::vector<std::pair<float, std::string>> XText; // each label's left edge and its words
	float XBase = 0.0f;                                // the x labels' baseline
	int N = 0;
	bool Any = false;
	bool Slots = false;
};

struct ChartAxesIn
{
	int N = 0;
	bool Any = false;
	double Lo = 0.0;
	double Hi = 0.0;
	chart::Units U = chart::Units::Count;
	int Ticks = 4;
	bool Zero = true;
	bool Integer = false;
	bool YLabels = true;
	bool XLabels = true;
	bool Slots = false;
	float TopPad = 0.0f;
	float PinPad = 0.0f;
	float SidePad = 6.0f; // room left and right of the plot (a numbered bubble's radius at either end)
	const chart::XAxis* X = nullptr;
};

float ChartXAt(const ChartFrame& F, double K)
{
	if (F.N <= 0)
	{
		return F.Plot.X;
	}
	if (F.Slots)
	{
		return F.Plot.X + static_cast<float>((K + 0.5) / static_cast<double>(F.N)) * F.Plot.W;
	}
	if (F.N == 1)
	{
		return F.Plot.X + F.Plot.W * 0.5f;
	}
	return F.Plot.X + static_cast<float>(K / static_cast<double>(F.N - 1)) * F.Plot.W;
}

float ChartYAt(const ChartFrame& F, double V)
{
	const double Clamped = std::max(F.Ys.Lo, std::min(F.Ys.Hi, V));
	return F.Ys.Map(Clamped, F.Plot.Y + F.Plot.H, F.Plot.Y);
}

/** The baseline areas and bars hang from: 0 inside the range, else its nearer end. */
float ChartBaseY(const ChartFrame& F)
{
	const double Base = F.Ys.Lo > 0.0 ? F.Ys.Lo : F.Ys.Hi < 0.0 ? F.Ys.Hi : 0.0;
	return ChartYAt(F, Base);
}

ChartFrame ChartLayout(const Canvas& C, const chart::Theme& T, const Rect& R, const ChartAxesIn& In)
{
	ChartFrame F;
	F.N = In.N;
	F.Any = In.Any;
	F.Slots = In.Slots;
	F.Ys = chart::NiceScale(In.Any ? In.Lo : 0.0, In.Any ? In.Hi : 0.0, In.Ticks, In.Zero, In.Integer);
	float LabelW = 0.0f;
	if (In.YLabels)
	{
		for (const double V : F.Ys.Ticks)
		{
			F.YText.push_back(chart::AxisText(In.U, V, F.Ys.Step));
			LabelW = std::max(LabelW, ChartMeasure(C, T, ChartRole::Axis, F.YText.back(), T.AxisSize));
		}
	}
	const chart::XAxis* X = In.X;
	const bool XWords = In.XLabels && X != nullptr && In.N > 0 && (!X->Labels.empty() || X->FirstDay != std::numeric_limits<int>::min());
	// Room on every side for what reaches past the plot: the labels, a 2-unit line's mitred joints (up to 4 units), the
	// hover's and the last point's ringed dots (5.75), the round caps.
	const float Gutter = std::max(In.SidePad, In.YLabels ? std::ceil(LabelW) + 9.0f : 6.0f);
	const float TopRoom = std::max(In.YLabels ? std::max(6.0f, std::ceil(T.AxisSize * 0.75f)) : 6.0f, In.TopPad);
	float BottomRoom = In.PinPad + (XWords ? T.AxisSize + 10.0f : 0.0f);
	BottomRoom = std::max(BottomRoom, In.YLabels ? std::max(6.0f, std::ceil(T.AxisSize * 0.6f)) : 6.0f);
	F.Plot = {Gutter, TopRoom, std::max(0.0f, R.W - Gutter - std::max(6.0f, In.SidePad)), std::max(0.0f, R.H - TopRoom - BottomRoom)};
	F.XBase = F.Plot.Y + F.Plot.H + In.PinPad + T.AxisSize + 5.0f;
	if (XWords)
	{
		const bool Explicit = !X->Labels.empty();
		const int Want = std::max(1, X->Count);
		const int Every = std::max(1, (In.N + Want - 1) / Want);
		float Right = -1e9f;
		for (int K = 0; K < In.N; ++K)
		{
			std::string Words;
			if (Explicit)
			{
				if (K < static_cast<int>(X->Labels.size()))
				{
					Words = X->Labels[static_cast<size_t>(K)];
				}
			}
			else if (K % Every == 0)
			{
				Words = chart::AxisDate(X->FirstDay + K * X->DayStep);
			}
			if (Words.empty())
			{
				continue;
			}
			const float Wd = ChartMeasure(C, T, ChartRole::Axis, Words, T.AxisSize);
			const float Left = std::max(0.0f, std::min(ChartXAt(F, K) - Wd * 0.5f, R.W - Wd));
			if (Left < Right + 10.0f)
			{
				continue;
			}
			F.XText.push_back({Left, Words});
			Right = Left + Wd;
		}
	}
	return F;
}

/** The grid, the zero line, the axis figures and the empty note, in R's space. */
void ChartAxesPaint(Canvas& C, const chart::Theme& T, const ChartFrame& F, bool Grid, const std::string& Empty)
{
	if (F.Plot.W <= 0.0f || F.Plot.H <= 0.0f)
	{
		return;
	}
	const float X0 = F.Plot.X;
	const float X1 = F.Plot.X + F.Plot.W;
	for (size_t K = 0; K < F.Ys.Ticks.size(); ++K)
	{
		const double V = F.Ys.Ticks[K];
		const float Y = ChartYAt(F, V);
		const bool ZeroLine = V == 0.0;
		if (Grid || ZeroLine)
		{
			C.FillRect({X0, Y - 0.5f, X1 - X0, 1.0f}, ZeroLine ? T.Axis : T.Grid);
		}
		if (K < F.YText.size())
		{
			ChartPrint(C, T, ChartRole::Axis, F.YText[K], X0 - 8.0f, Y, T.AxisSize, T.Muted, Align::Right, Baseline::Middle);
		}
	}
	if (!Grid && F.Ys.Lo > 0.0)
	{
		C.FillRect({X0, F.Plot.Y + F.Plot.H - 0.5f, X1 - X0, 1.0f}, T.Axis);
	}
	for (const auto& Lab : F.XText)
	{
		ChartPrint(C, T, ChartRole::Axis, Lab.second, Lab.first, F.XBase, T.AxisSize, T.Muted);
	}
	if (!F.Any && !Empty.empty())
	{
		ChartPrint(C, T, ChartRole::Body, Empty, X0 + F.Plot.W * 0.5f, F.Plot.Y + F.Plot.H * 0.5f, 13.0f, T.Muted, Align::Center, Baseline::Middle,
			F.Plot.W - 16.0f);
	}
}

void ChartFrameKey(ChartHash& H, const chart::Theme& T, const Rect& R, const ChartFrame& F)
{
	H.In(static_cast<int64_t>(T.Key() & 0x7fffffffffffffffull));
	H.Fl(R.W);
	H.Fl(R.H);
	H.Re(F.Plot);
	H.Db(F.Ys.Lo);
	H.Db(F.Ys.Hi);
	H.Db(F.Ys.Step);
	H.In(F.N);
	H.In(F.Any ? 1 : 0);
	H.In(F.Slots ? 1 : 0);
	for (const std::string& S : F.YText)
	{
		H.St(S);
	}
	for (const auto& Lab : F.XText)
	{
		H.Fl(Lab.first);
		H.St(Lab.second);
	}
	H.Fl(F.XBase);
}

/** Each series' resolved colour. */
Color ChartSeriesCol(const chart::Theme& T, const Color& Own, size_t Place)
{
	return ChartOr(Own, T.Series[Place % T.Series.size()]);
}

/** The runs of finite values of one series as points. */
std::vector<std::vector<Vec2>> ChartRuns(const ChartFrame& F, const std::vector<double>& Y)
{
	std::vector<std::vector<Vec2>> Runs;
	std::vector<Vec2> Run;
	for (size_t K = 0; K < Y.size(); ++K)
	{
		if (!std::isfinite(Y[K]))
		{
			if (!Run.empty())
			{
				Runs.push_back(std::move(Run));
				Run.clear();
			}
			continue;
		}
		Run.push_back({ChartXAt(F, static_cast<double>(K)), ChartYAt(F, Y[K])});
	}
	if (!Run.empty())
	{
		Runs.push_back(std::move(Run));
	}
	return Runs;
}

void ChartSeriesPaint(Canvas& C, const chart::Theme& T, const ChartFrame& F, const std::vector<chart::Series>& Lines, const chart::Band* B)
{
	if (F.Plot.W <= 0.0f || F.Plot.H <= 0.0f)
	{
		return;
	}
	if (B != nullptr && B->Col.A > 0.0f)
	{
		const size_t N = std::min(B->Lo.size(), B->Hi.size());
		std::vector<Vec2> Up;
		std::vector<Vec2> Dn;
		auto Flush = [&]() {
			if (Up.size() >= 2)
			{
				const std::vector<Color> Cols(Up.size(), B->Col);
				ChartStrip(C, Up, Cols, Dn, Cols);
			}
			Up.clear();
			Dn.clear();
		};
		for (size_t K = 0; K < N; ++K)
		{
			const double Lo = B->Lo[K];
			const double Hi = B->Hi[K];
			if (!std::isfinite(Lo) || !std::isfinite(Hi))
			{
				Flush();
				continue;
			}
			const float X = ChartXAt(F, static_cast<double>(K));
			Up.push_back({X, ChartYAt(F, std::max(Lo, Hi))});
			Dn.push_back({X, ChartYAt(F, std::min(Lo, Hi))});
		}
		Flush();
	}
	const float BaseY = ChartBaseY(F);
	const float Reach = std::max(BaseY - F.Plot.Y, F.Plot.Y + F.Plot.H - BaseY);
	for (size_t S = 0; S < Lines.size(); ++S)
	{
		const chart::Series& L = Lines[S];
		const Color Col = ChartSeriesCol(T, L.Col, S);
		const std::vector<std::vector<Vec2>> Runs = ChartRuns(F, L.Y);
		for (const std::vector<Vec2>& Run : Runs)
		{
			ChartArea(C, Run, BaseY, Col, L.Area, L.AreaFade, Reach);
		}
		for (const std::vector<Vec2>& Run : Runs)
		{
			if (Run.size() == 1)
			{
				C.FillCircle(Run[0].X, Run[0].Y, std::max(3.0f, L.Width * 1.5f), Col);
				continue;
			}
			if (L.Dash > 0.0f && L.Gap > 0.0f)
			{
				C.StrokeDashed(Run, Col, L.Width, L.Dash, L.Gap);
			}
			else if (L.Width > 0.0f)
			{
				C.StrokePolyline(Run, false, Col, L.Width, true);
			}
			if (L.Dots)
			{
				for (const Vec2& P : Run)
				{
					C.FillCircle(P.X, P.Y, L.DotR, Col);
				}
			}
		}
	}
}

void ChartSeriesKey(ChartHash& H, const chart::Theme& T, const std::vector<chart::Series>& Lines, const chart::Band* B)
{
	for (size_t S = 0; S < Lines.size(); ++S)
	{
		const chart::Series& L = Lines[S];
		H.Dv(L.Y);
		H.Co(ChartSeriesCol(T, L.Col, S));
		H.Fl(L.Width);
		H.Fl(L.Area);
		H.Fl(L.AreaFade);
		H.Fl(L.Dash);
		H.Fl(L.Gap);
		H.In(L.Dots ? 1 : 0);
		H.Fl(L.DotR);
	}
	if (B != nullptr)
	{
		H.Dv(B->Lo);
		H.Dv(B->Hi);
		H.Co(B->Col);
	}
}

/** The pointer's index along the x axis, or -1 outside the plot (Plot in the caller's space). */
int ChartHoverIndex(const Rect& Plot, int N, bool Slots, Vec2 Mouse)
{
	if (N <= 0 || Mouse.X < Plot.X - 4.0f || Mouse.X > Plot.X + Plot.W + 4.0f || Mouse.Y < Plot.Y - 4.0f || Mouse.Y > Plot.Y + Plot.H + 4.0f || Plot.W <= 0.0f)
	{
		return -1;
	}
	const float T = (Mouse.X - Plot.X) / Plot.W;
	int K = 0;
	if (Slots)
	{
		K = static_cast<int>(std::floor(T * static_cast<float>(N)));
	}
	else
	{
		K = N == 1 ? 0 : static_cast<int>(std::lround(T * static_cast<float>(N - 1)));
	}
	return std::max(0, std::min(N - 1, K));
}

/** A dot with a ground ring (a hover point, the last point). */
void ChartDot(Canvas& C, float X, float Y, float Rad, const Color& Col, const Color& Ring)
{
	C.FillCircle(X, Y, Rad + 1.75f, Ring);
	C.FillCircle(X, Y, Rad, Col);
}

/** The Bars layer: every slot's bar or stack, grown by Grow (0..1), in R's space. */
void ChartBarsPaint(Canvas& C, const chart::Theme& T, const ChartFrame& F, const std::vector<chart::BarSet>& Sets, bool Stacked, float Gap, float Grow)
{
	if (F.Plot.W <= 0.0f || F.Plot.H <= 0.0f || F.N <= 0 || Sets.empty())
	{
		return;
	}
	const float SlotW = F.Plot.W / static_cast<float>(F.N);
	const float GroupW = std::max(0.5f, SlotW * (1.0f - std::max(0.0f, std::min(0.9f, Gap))));
	const size_t Count = Sets.size();
	const float Each = Stacked ? GroupW : GroupW / static_cast<float>(Count);
	const float Inner = !Stacked && Count > 1 && Each > 5.0f ? 1.0f : 0.0f;
	auto Fill = [&](float X, float Y0, float Y1, float Wd, const Color& Col) {
		const float Top = std::min(Y0, Y1);
		const float Ht = std::fabs(Y1 - Y0);
		if (Ht < 0.01f || Wd <= 0.0f)
		{
			return;
		}
		// A thin bar's rounded corners can't be seen and cost 16 vertices: they round from 10 units wide.
		const float Rad = Wd >= 10.0f ? std::min(T.BarRadius, std::min(Wd, Ht) * 0.5f) : 0.0f;
		if (Rad > 0.05f)
		{
			C.FillRoundRect({X, Top, Wd, Ht}, Rad, Col);
		}
		else
		{
			C.FillRect({X, Top, Wd, Ht}, Col);
		}
	};
	for (int K = 0; K < F.N; ++K)
	{
		const float SlotX = F.Plot.X + SlotW * static_cast<float>(K) + (SlotW - GroupW) * 0.5f;
		double Pos = 0.0;
		double Neg = 0.0;
		for (size_t S = 0; S < Count; ++S)
		{
			const chart::BarSet& Set = Sets[S];
			if (static_cast<size_t>(K) >= Set.V.size() || !std::isfinite(Set.V[static_cast<size_t>(K)]))
			{
				continue;
			}
			const double V = Set.V[static_cast<size_t>(K)];
			Color Col = ChartSeriesCol(T, Set.Col, S);
			if (static_cast<size_t>(K) < Set.Each.size())
			{
				Col = ChartOr(Set.Each[static_cast<size_t>(K)], Col);
			}
			if (Stacked)
			{
				double& Acc = V >= 0.0 ? Pos : Neg;
				const float Y0 = ChartYAt(F, Acc * Grow);
				Acc += V;
				const float Y1 = ChartYAt(F, Acc * Grow);
				Fill(SlotX, Y0, Y1, Each, Col);
			}
			else
			{
				const float X = SlotX + Each * static_cast<float>(S) + Inner * 0.5f;
				Fill(X, ChartYAt(F, 0.0 > F.Ys.Lo ? 0.0 : F.Ys.Lo), ChartYAt(F, V * Grow), Each - Inner, Col);
			}
		}
	}
}
// ------------------------------------------------------------------ the look

uint64_t Theme::Key() const
{
	ChartHash H;
	for (const Color* C : {&Ground, &Panel, &Panel2, &Edge, &Tip, &Grid, &Axis, &Ink, &Muted, &Dim, &Up, &Down, &Amber, &Accent, &Value, &Under})
	{
		H.Co(*C);
	}
	for (const Color& C : Series)
	{
		H.Co(C);
	}
	H.In(LabelFace);
	H.In(LabelWeight);
	H.In(Caps ? 1 : 0);
	H.In(FigureFace);
	H.In(FigureWeight);
	H.In(AxisFace);
	H.In(AxisWeight);
	H.In(BodyWeight);
	H.Fl(AxisSize);
	H.Fl(LabelSize);
	H.Fl(Radius);
	H.Fl(Cut);
	H.Fl(BarRadius);
	H.Fl(TwiceFrom);
	return H.H;
}

Theme Rerun()
{
	// RiverLineVideoShared.h's look (RrGround, RrPanel, RrPanel2, RrLine, RrCream, RrMuted, RrDim, RrGold, RrWarn, RrSky,
	// RrCash) and the analytics mockup's gains and losses; rerun_ui checks the two stay one.
	Theme T;
	T.Ground = Hex(videoart::Ink);
	T.Panel = Hex(0x1f2229);
	T.Panel2 = T.Panel;
	T.Edge = Hex(0x363b47);
	T.Tip = Hex(0x2a2e37);
	T.Grid = Hex(0x363b47, 0.6f);
	T.Axis = Hex(0x363b47);
	T.Ink = Hex(videoart::Cream);
	T.Muted = Hex(0xb0a794);
	T.Dim = Hex(0x767064);
	T.Up = Hex(0x7fd99a);
	T.Down = Hex(0xf06a6a);
	T.Amber = Hex(0xf39a6b);
	T.Accent = Hex(videoart::Marigold);
	T.Value = Hex(videoart::Cream);
	T.Under = Color{0.0f, 0.0f, 0.0f, 0.0f};
	T.Series = {Hex(videoart::Marigold), Hex(0x8ccdfa), Hex(0x7fd99a), Hex(0xffd479), Hex(0xc8a2ff), Hex(0xff8fa3)};
	T.LabelWeight = 800;
	T.FigureWeight = 800;
	T.AxisWeight = 600;
	T.BodyWeight = 600;
	T.AxisSize = 11.0f;
	T.LabelSize = 11.5f;
	T.Radius = 10.0f;
	T.Cut = 0.0f;
	T.BarRadius = 1.0f;
	return T;
}

Theme FromSkin(const Skin& S)
{
	Theme T;
	T.Ground = S.Bg;
	T.Panel = S.Panel;
	T.Panel2 = S.Panel;
	T.Edge = S.Line;
	T.Tip = S.Panel; // Panel2 holds Muted at 4.50 on RiverLine's tokens: the card sits a step darker
	T.Grid = S.Line;
	T.Axis = Mix(S.Line, S.Muted, 0.35f);
	T.Ink = S.Ink;
	T.Muted = S.Muted;
	T.Dim = S.Dim;
	T.Up = S.Green;
	T.Down = S.Red;
	T.Amber = S.Orange;
	T.Accent = S.Accent;
	T.Value = S.Ink;
	T.Under = Color{0.0f, 0.0f, 0.0f, 0.0f};
	T.Series = {S.Accent, S.Accent2, S.Gold, S.Green, S.Orange, S.Muted};
	T.LabelWeight = 800;
	T.FigureWeight = 800;
	T.AxisWeight = 600;
	T.BodyWeight = 500;
	T.AxisSize = 11.0f;
	T.LabelSize = 11.5f;
	T.Radius = 6.0f;
	T.Cut = 0.0f;
	T.BarRadius = 1.0f;
	return T;
}

Theme RunItTwice(const Skin& S)
{
	// SPEC §3.3: Signal Red (Accent) is light, never ink; Hot White (the Gold token) is money and the figures; tiles are
	// gloss (Gloss 202026 over Onyx: Panel2 lifted an eighth toward the Hairline) under a Hairline edge, square with one cut;
	// the tooltip sits on Tape Black (Onyx at half); labels, figures and axes in BoldCondensed, labels in capitals.
	Theme T = FromSkin(S);
	T.Panel = Mix(S.Panel2, S.Line, 0.125f);
	T.Panel2 = S.Bg;
	T.Edge = S.Line;
	T.Tip = Mix(S.Bg, Color{0.0f, 0.0f, 0.0f, 1.0f}, 0.5f);
	T.Grid = S.Line;
	T.Axis = Mix(S.Line, S.Muted, 0.35f);
	T.Value = S.Gold;
	T.Under = S.Accent;
	T.Series = {S.Gold, S.Accent2, S.Orange, S.Green, S.Accent, S.Muted};
	T.LabelFace = static_cast<int8_t>(Font::BoldCondensed);
	T.FigureFace = static_cast<int8_t>(Font::BoldCondensed);
	T.AxisFace = static_cast<int8_t>(Font::BoldCondensed);
	T.Caps = true;
	T.AxisSize = 12.0f;
	T.LabelSize = 12.0f;
	T.BodyWeight = 500;
	T.Radius = 0.0f;
	T.Cut = 10.0f;
	T.BarRadius = 0.0f;
	T.TwiceFrom = 28.0f;
	return T;
}

float TwiceOffset(float Size)
{
	const int O = static_cast<int>(Size / 16.0f + 0.5f);
	return static_cast<float>(std::max(2, std::min(6, O)));
}

// ------------------------------------------------------------------ scales and axes

AxisScale NiceScale(double Lo, double Hi, int Target, bool Zero, bool Integer)
{
	if (!std::isfinite(Lo) && !std::isfinite(Hi))
	{
		Lo = 0.0;
		Hi = 0.0;
	}
	else if (!std::isfinite(Lo))
	{
		Lo = Hi;
	}
	else if (!std::isfinite(Hi))
	{
		Hi = Lo;
	}
	if (Lo > Hi)
	{
		std::swap(Lo, Hi);
	}
	if (Zero)
	{
		Lo = std::min(Lo, 0.0);
		Hi = std::max(Hi, 0.0);
	}
	if (Hi - Lo <= 1e-12 * std::max(1.0, std::fabs(Hi)))
	{
		// One value (or none): a range around it that keeps it off the frame's edge.
		if (Hi == 0.0)
		{
			Hi = 1.0;
		}
		else
		{
			// (with Zero the range already holds 0, so only a lone 0 lands here, above)
			const double Widen = std::max(Integer ? 1.0 : 1e-9, std::fabs(Hi) * 0.1);
			Lo -= Widen;
			Hi += Widen;
		}
	}
	// The nice step for Want intervals; then the tighter of Want and Want + 1 intervals (a tie keeps the fewer ticks), so a
	// peak just past a round number (4,001) gets 0-5K, not 0-6K with a third of the plot empty.
	auto StepFor = [Lo, Hi, Integer](int Intervals) {
		const double Raw = (Hi - Lo) / static_cast<double>(Intervals);
		const double Mag = std::pow(10.0, std::floor(std::log10(Raw)));
		static const double Nice[5] = {1.0, 2.0, 2.5, 5.0, 10.0};
		double Pick = Mag * 10.0;
		for (const double M : Nice)
		{
			if (Integer && M == 2.5 && Mag < 10.0)
			{
				continue;
			}
			if (M * Mag >= Raw * (1.0 - 1e-9))
			{
				Pick = M * Mag;
				break;
			}
		}
		return Integer ? std::max(1.0, std::round(Pick)) : Pick;
	};
	auto SpanFor = [Lo, Hi](double Each) { return (std::ceil(Hi / Each - 1e-9) - std::floor(Lo / Each + 1e-9)) * Each; };
	const int Want = std::max(1, Target);
	double Step = StepFor(Want);
	const double Finer = StepFor(Want + 1);
	if (SpanFor(Finer) < SpanFor(Step) * (1.0 - 1e-9))
	{
		Step = Finer;
	}
	AxisScale S;
	S.Step = Step;
	S.Lo = std::floor(Lo / Step + 1e-9) * Step;
	S.Hi = std::ceil(Hi / Step - 1e-9) * Step;
	if (S.Hi <= S.Lo)
	{
		S.Hi = S.Lo + Step;
	}
	const long long Count = std::min(100LL, std::llround((S.Hi - S.Lo) / Step));
	for (long long K = 0; K <= Count; ++K)
	{
		double V = S.Lo + Step * static_cast<double>(K);
		if (std::fabs(V) < Step * 1e-9)
		{
			V = 0.0;
		}
		S.Ticks.push_back(V);
	}
	if (S.Lo == 0.0)
	{
		S.Lo = 0.0; // never -0
	}
	return S;
}

std::vector<double> NiceTicks(double Lo, double Hi, int Target)
{
	return NiceScale(Lo, Hi, Target, false, false).Ticks;
}

std::string AxisCount(double V, double Step)
{
	if (!std::isfinite(V))
	{
		return "0";
	}
	const double A = std::fabs(V);
	const std::string Body = ChartShort(A, Step);
	return V < 0.0 && Body != "0" ? "-" + Body : Body;
}

std::string AxisMoney(double Cents, double Step)
{
	if (!std::isfinite(Cents))
	{
		return "$0";
	}
	const double D = std::fabs(Cents) / 100.0;
	std::string Body;
	if (D >= 999.5)
	{
		Body = ChartShort(D, Step > 0.0 ? Step / 100.0 : 0.0);
	}
	else
	{
		bool Cents2 = false;
		if (Step > 0.0)
		{
			Cents2 = ChartDecimals(Step / 100.0, 2) > 0;
		}
		else
		{
			Cents2 = std::fabs(D - std::round(D)) > 0.004;
		}
		Body = Cents2 ? Fixed(D, 2) : Fixed(std::round(D), 0);
	}
	if (Body == "0" || Body == "0.00")
	{
		return "$" + Body;
	}
	return (Cents < 0.0 ? "-$" : "$") + Body;
}

std::string AxisDuration(double Seconds)
{
	if (!std::isfinite(Seconds))
	{
		return "0:00";
	}
	const long long S = std::llround(std::fabs(Seconds));
	const long long H = S / 3600;
	const long long M = (S % 3600) / 60;
	const long long Sec = S % 60;
	char Buf[48];
	if (H > 0)
	{
		std::snprintf(Buf, sizeof(Buf), "%lld:%02lld:%02lld", H, M, Sec);
	}
	else
	{
		std::snprintf(Buf, sizeof(Buf), "%lld:%02lld", M, Sec);
	}
	return (Seconds < 0.0 && S > 0 ? "-" : "") + std::string(Buf);
}

std::string AxisPercent(double Fraction, double Step)
{
	if (!std::isfinite(Fraction))
	{
		return "0%";
	}
	const double P = Fraction * 100.0;
	const int D = Step > 0.0 ? ChartDecimals(Step * 100.0, 2) : (std::fabs(P) < 10.0 ? 1 : 0);
	std::string Body = ChartTrim(Fixed(P, D));
	if (Body == "-0")
	{
		Body = "0";
	}
	return Body + "%";
}

std::string AxisDate(int Day)
{
	return net::DateLabel(Day);
}

std::string AxisText(Units U, double V, double Step)
{
	switch (U)
	{
	case Units::Count: return AxisCount(V, Step);
	case Units::Money: return AxisMoney(V, Step);
	case Units::Duration: return AxisDuration(V);
	case Units::Percent: return AxisPercent(V, Step);
	case Units::Plain:
	{
		std::string Body = ChartTrim(Fixed(V, ChartDecimals(Step, 3)));
		return Body == "-0" ? "0" : Body;
	}
	}
	return AxisCount(V, Step);
}

// ------------------------------------------------------------------ LineChart

LineOut LineChart(Canvas& C, const Rect& R, const std::vector<Series>& Lines, const LineOpts& O, const Theme& T, const Band* B, const std::vector<Mark>* Marks)
{
	LineOut Out;
	ChartAxesIn In;
	In.Lo = std::numeric_limits<double>::infinity();
	In.Hi = -std::numeric_limits<double>::infinity();
	auto Take = [&In](double V) {
		if (std::isfinite(V))
		{
			In.Lo = std::min(In.Lo, V);
			In.Hi = std::max(In.Hi, V);
			In.Any = true;
		}
	};
	for (const Series& S : Lines)
	{
		In.N = std::max(In.N, static_cast<int>(S.Y.size()));
		for (const double V : S.Y)
		{
			Take(V);
		}
	}
	if (B != nullptr)
	{
		const size_t N = std::min(B->Lo.size(), B->Hi.size());
		In.N = std::max(In.N, static_cast<int>(N));
		for (size_t K = 0; K < N; ++K)
		{
			if (std::isfinite(B->Lo[K]) && std::isfinite(B->Hi[K]))
			{
				Take(B->Lo[K]);
				Take(B->Hi[K]);
			}
		}
	}
	if (std::isfinite(O.YMin))
	{
		In.Lo = O.YMin;
	}
	if (std::isfinite(O.YMax))
	{
		In.Hi = O.YMax;
	}
	bool Bubbles = false;
	bool Pins = false;
	if (Marks != nullptr)
	{
		for (const Mark& M : *Marks)
		{
			Bubbles = Bubbles || (!M.Pin && !M.Tag.empty());
			Pins = Pins || M.Pin;
		}
	}
	In.U = O.Y;
	In.Ticks = O.YTicks;
	In.Zero = O.Zero;
	In.Integer = O.Integer;
	In.YLabels = O.YLabels;
	In.XLabels = O.XLabels;
	In.Slots = false;
	In.TopPad = Bubbles ? 24.0f : 0.0f;
	In.SidePad = Bubbles ? 10.0f : 6.0f;
	In.PinPad = Pins ? 8.0f : 0.0f;
	In.X = &O.X;
	const ChartFrame F = ChartLayout(C, T, R, In);
	Out.Ys = F.Ys;
	Out.Points = F.N;
	Out.Plot = {R.X + F.Plot.X, R.Y + F.Plot.Y, F.Plot.W, F.Plot.H};

	ChartHash Hf;
	Hf.St("chart:line:frame");
	ChartFrameKey(Hf, T, R, F);
	Hf.In(O.Grid ? 1 : 0);
	Hf.St(F.Any ? std::string() : O.Empty);
	const bool Grid = O.Grid;
	const std::string Empty = O.Empty;
	ChartLayer(C, O.Cache, Hf.H, R, [&T, &F, Grid, &Empty](Canvas& Cv) { ChartAxesPaint(Cv, T, F, Grid, Empty); });

	const float Reveal = std::max(0.0f, std::min(1.0f, O.Reveal));
	if (F.Any && F.Plot.W > 0.0f && F.Plot.H > 0.0f && Reveal > 0.0f)
	{
		ChartHash Hs;
		Hs.St("chart:line:series");
		ChartFrameKey(Hs, T, R, F);
		ChartSeriesKey(Hs, T, Lines, B);
		const bool Clip = Reveal < 1.0f;
		if (Clip)
		{
			C.PushClip({R.X, R.Y, F.Plot.X + F.Plot.W * Reveal + 1.0f, R.H});
		}
		ChartLayer(C, O.Cache, Hs.H, R, [&T, &F, &Lines, B](Canvas& Cv) { ChartSeriesPaint(Cv, T, F, Lines, B); });
		if (Clip)
		{
			C.PopClip();
		}
	}
	if (!F.Any || Reveal < 1.0f || F.Plot.W <= 0.0f || F.Plot.H <= 0.0f)
	{
		return Out;
	}
	const Rect& P = Out.Plot;
	ChartFrame Abs = F;
	Abs.Plot = P;
	// The moments: numbered bubbles over the plot, upload pins on the baseline.
	if (Marks != nullptr)
	{
		for (const Mark& M : *Marks)
		{
			if (M.At < 0 || M.At >= F.N)
			{
				continue;
			}
			const float X = ChartXAt(Abs, static_cast<double>(M.At));
			if (M.Pin)
			{
				const Color Col = ChartOr(M.Col, T.Muted);
				C.FillRect({X - 0.75f, P.Y + P.H, 1.5f, 6.0f}, Col);
				C.FillCircle(X, P.Y + P.H + 2.0f, 3.5f, Col);
				continue;
			}
			if (M.Tag.empty())
			{
				continue;
			}
			const Color Col = ChartOr(M.Col, T.Accent);
			float To = P.Y + P.H;
			if (!Lines.empty() && static_cast<size_t>(M.At) < Lines[0].Y.size() && std::isfinite(Lines[0].Y[static_cast<size_t>(M.At)]))
			{
				To = ChartYAt(Abs, Lines[0].Y[static_cast<size_t>(M.At)]) - 6.0f;
			}
			if (To > P.Y - 2.0f)
			{
				C.StrokeDashed({{X, P.Y - 3.0f}, {X, To}}, ChartFade(T.Ink, 0.35f), 1.0f, 3.0f, 3.0f);
			}
			C.FillCircle(X, P.Y - 12.0f, 9.0f, Col);
			ChartPrint(C, T, ChartRole::Figure, M.Tag, X, P.Y - 12.0f, 11.5f, ChartInkOn(T, Col), Align::Center, Baseline::Middle);
		}
	}
	// The last point lit, with its tick on the value axis.
	for (size_t S = 0; S < Lines.size(); ++S)
	{
		const Series& L = Lines[S];
		if (!L.Last)
		{
			continue;
		}
		for (size_t K = L.Y.size(); K-- > 0;)
		{
			if (std::isfinite(L.Y[K]))
			{
				const Color Col = ChartOr(L.LastCol, T.Accent);
				const float X = ChartXAt(Abs, static_cast<double>(K));
				const float Y = ChartYAt(Abs, L.Y[K]);
				C.FillRect({P.X - 6.0f, Y - 1.0f, 6.0f, 2.0f}, Col);
				ChartDot(C, X, Y, 4.0f, Col, T.Ground);
				break;
			}
		}
	}
	// The hover: a hair line and each series' point.
	Out.Hover = ChartHoverIndex(P, F.N, false, O.Mouse);
	if (Out.Hover >= 0)
	{
		const float X = ChartXAt(Abs, static_cast<double>(Out.Hover));
		Out.HoverAt = {X, P.Y + P.H * 0.5f};
		C.FillRect({X - 0.5f, P.Y, 1.0f, P.H}, ChartFade(T.Muted, 0.45f));
		bool First = true;
		for (size_t S = 0; S < Lines.size(); ++S)
		{
			const Series& L = Lines[S];
			if (static_cast<size_t>(Out.Hover) >= L.Y.size() || !std::isfinite(L.Y[static_cast<size_t>(Out.Hover)]))
			{
				continue;
			}
			const float Y = ChartYAt(Abs, L.Y[static_cast<size_t>(Out.Hover)]);
			ChartDot(C, X, Y, 4.0f, ChartSeriesCol(T, L.Col, S), T.Ground);
			if (First)
			{
				Out.HoverAt = {X, Y};
				First = false;
			}
		}
	}
	return Out;
}

// ------------------------------------------------------------------ Bars

BarOut Bars(Canvas& C, const Rect& R, const std::vector<BarSet>& Sets, const BarOpts& O, const Theme& T)
{
	BarOut Out;
	ChartAxesIn In;
	In.Lo = 0.0;
	In.Hi = 0.0;
	for (const BarSet& S : Sets)
	{
		In.N = std::max(In.N, static_cast<int>(S.V.size()));
	}
	for (int K = 0; K < In.N; ++K)
	{
		double Pos = 0.0;
		double Neg = 0.0;
		for (const BarSet& S : Sets)
		{
			if (static_cast<size_t>(K) >= S.V.size() || !std::isfinite(S.V[static_cast<size_t>(K)]))
			{
				continue;
			}
			const double V = S.V[static_cast<size_t>(K)];
			In.Any = true;
			if (O.Stacked)
			{
				(V >= 0.0 ? Pos : Neg) += V;
			}
			else
			{
				In.Lo = std::min(In.Lo, V);
				In.Hi = std::max(In.Hi, V);
			}
		}
		if (O.Stacked)
		{
			In.Lo = std::min(In.Lo, Neg);
			In.Hi = std::max(In.Hi, Pos);
		}
	}
	if (std::isfinite(O.YMax))
	{
		In.Hi = O.YMax;
	}
	In.U = O.Y;
	In.Ticks = O.YTicks;
	In.Zero = true;
	In.Integer = O.Integer;
	In.YLabels = O.YLabels;
	In.XLabels = O.XLabels;
	In.Slots = true;
	In.X = &O.X;
	const ChartFrame F = ChartLayout(C, T, R, In);
	Out.Ys = F.Ys;
	Out.Slots = F.N;
	Out.Plot = {R.X + F.Plot.X, R.Y + F.Plot.Y, F.Plot.W, F.Plot.H};

	ChartHash Hf;
	Hf.St("chart:bars:frame");
	ChartFrameKey(Hf, T, R, F);
	Hf.In(O.Grid ? 1 : 0);
	Hf.St(F.Any ? std::string() : O.Empty);
	const bool Grid = O.Grid;
	const std::string Empty = O.Empty;
	ChartLayer(C, O.Cache, Hf.H, R, [&T, &F, Grid, &Empty](Canvas& Cv) { ChartAxesPaint(Cv, T, F, Grid, Empty); });

	const float Grow = std::max(0.0f, std::min(1.0f, O.Reveal));
	if (F.Any && Grow > 0.0f)
	{
		ChartHash Hb;
		Hb.St("chart:bars:bars");
		ChartFrameKey(Hb, T, R, F);
		Hb.In(O.Stacked ? 1 : 0);
		Hb.Fl(O.Gap);
		for (size_t S = 0; S < Sets.size(); ++S)
		{
			Hb.Dv(Sets[S].V);
			Hb.Co(ChartSeriesCol(T, Sets[S].Col, S));
			Hb.In(static_cast<int64_t>(Sets[S].Each.size()));
			for (const Color& E : Sets[S].Each)
			{
				Hb.Co(E);
			}
		}
		const bool Stacked = O.Stacked;
		const float Gap = O.Gap;
		// While growing the layer changes every frame: drawn straight, never cached.
		ChartLayer(C, O.Cache && Grow >= 1.0f, Hb.H, R, [&T, &F, &Sets, Stacked, Gap, Grow](Canvas& Cv) { ChartBarsPaint(Cv, T, F, Sets, Stacked, Gap, Grow); });
	}
	if (F.Any && Grow >= 1.0f && F.N > 0 && F.Plot.W > 0.0f)
	{
		Out.Hover = ChartHoverIndex(Out.Plot, F.N, true, O.Mouse);
		if (Out.Hover >= 0)
		{
			const float SlotW = Out.Plot.W / static_cast<float>(F.N);
			Out.HoverSlot = {Out.Plot.X + SlotW * static_cast<float>(Out.Hover), Out.Plot.Y, SlotW, Out.Plot.H};
			C.FillRect(Out.HoverSlot, ChartFade(T.Ink, 0.07f));
		}
	}
	return Out;
}

// ------------------------------------------------------------------ Sparkline

void Sparkline(Canvas& C, const Rect& R, const std::vector<double>& V, const Color& Col, const SparkOpts& O, const Theme& T)
{
	double Lo = std::numeric_limits<double>::infinity();
	double Hi = -std::numeric_limits<double>::infinity();
	for (const double X : V)
	{
		if (std::isfinite(X))
		{
			Lo = std::min(Lo, X);
			Hi = std::max(Hi, X);
		}
	}
	if (!(Lo <= Hi) || R.W <= 2.0f || R.H <= 2.0f)
	{
		return; // nothing to draw
	}
	if (O.Zero)
	{
		Lo = std::min(Lo, 0.0);
		Hi = std::max(Hi, 0.0);
	}
	const Color LineCol = ChartOr(Col, T.Series[0]);
	const Color LastCol = ChartOr(O.LastCol, T.Accent);
	ChartHash H;
	H.St("chart:spark");
	H.Fl(R.W);
	H.Fl(R.H);
	H.Dv(V);
	H.Co(LineCol);
	H.Co(LastCol);
	H.Co(T.Ground);
	H.Fl(O.Width);
	H.Fl(O.Area);
	H.In(O.Last ? 1 : 0);
	H.Fl(O.LastR);
	H.In(O.Zero ? 1 : 0);
	const SparkOpts Opt = O;
	const Rect Size = R;
	const double Low = Lo;
	const double High = Hi;
	ChartLayer(C, O.Cache, H.H, R, [&V, Opt, Size, Low, High, LineCol, LastCol, &T](Canvas& Cv) {
		// Room for a mitred joint (up to twice the width) and the lit last point's ring.
		const float Inset = std::max(Opt.Width * 2.0f + 0.5f, Opt.Last ? Opt.LastR + 2.25f : 0.0f);
		const Rect In{Inset, Inset, std::max(0.0f, Size.W - Inset * 2.0f), std::max(0.0f, Size.H - Inset * 2.0f)};
		const size_t N = V.size();
		auto Px = [&](size_t K) { return N <= 1 ? In.X : In.X + In.W * static_cast<float>(static_cast<double>(K) / static_cast<double>(N - 1)); };
		auto Py = [&](double X) {
			const double U = High > Low ? (X - Low) / (High - Low) : 0.5;
			return In.Y + In.H - static_cast<float>(U) * In.H;
		};
		std::vector<std::vector<Vec2>> Runs;
		std::vector<Vec2> Run;
		for (size_t K = 0; K < N; ++K)
		{
			if (!std::isfinite(V[K]))
			{
				if (!Run.empty())
				{
					Runs.push_back(Run);
					Run.clear();
				}
				continue;
			}
			Run.push_back({Px(K), Py(V[K])});
		}
		if (!Run.empty())
		{
			Runs.push_back(Run);
		}
		if (N == 1 && !Runs.empty())
		{
			// One value: a level line across.
			Runs[0] = {{In.X, Runs[0][0].Y}, {In.X + In.W, Runs[0][0].Y}};
		}
		const float Base = In.Y + In.H;
		for (const std::vector<Vec2>& Each : Runs)
		{
			ChartArea(Cv, Each, Base, LineCol, Opt.Area, 1.0f, 0.0f);
		}
		Vec2 LastAt{-1.0f, -1.0f};
		for (const std::vector<Vec2>& Each : Runs)
		{
			if (Each.size() == 1)
			{
				Cv.FillCircle(Each[0].X, Each[0].Y, std::max(1.5f, Opt.Width), LineCol);
			}
			else
			{
				Cv.StrokePolyline(Each, false, LineCol, Opt.Width, true);
			}
			LastAt = Each.back();
		}
		if (Opt.Last && LastAt.X >= 0.0f)
		{
			ChartDot(Cv, LastAt.X, LastAt.Y, Opt.LastR, LastCol, T.Ground);
		}
	});
}

// ------------------------------------------------------------------ Kpi

void Kpi(Canvas& C, const Rect& R, const KpiCard& K, const Theme& T, const KpiOpts& O)
{
	if (R.W <= 8.0f || R.H <= 8.0f)
	{
		return;
	}
	if (O.Ground)
	{
		ChartPanel(C, R, T.Panel, T.Panel2, T.Edge, T.Radius, T.Cut);
	}
	if (K.Selected)
	{
		const float Inset = T.Cut > 0.0f ? 0.0f : std::min(T.Radius, R.W * 0.25f);
		const float Right = T.Cut > 0.0f ? std::min(T.Cut, R.H * 0.5f) : Inset;
		C.FillRect({R.X + Inset, R.Y, R.W - Inset - Right, 3.0f}, T.Accent);
	}
	const float PadX = R.W >= 300.0f ? 18.0f : 12.0f;
	const float LabelBase = R.Y + std::max(T.LabelSize + 4.0f, R.H * 0.235f);
	const float BottomGap = std::max(4.0f, std::min(16.0f, (R.H - 72.0f) * 0.32f + 4.0f));
	const float NoteBase = R.Y + R.H - BottomGap;
	const bool HasNote = !K.Note.empty();
	const float ValueBase = HasNote ? (LabelBase + NoteBase) * 0.5f + O.ValueSize * 0.36f : (LabelBase + R.Y + R.H) * 0.5f + O.ValueSize * 0.36f - 2.0f;
	const bool HasSpark = K.Spark.size() >= 2 && R.W >= 260.0f && R.H >= 90.0f;
	const Rect SparkR{R.X + R.W - PadX - R.W * 0.35f, R.Y + R.H * 0.47f, R.W * 0.35f, R.H * 0.36f};
	const float TextRight = HasSpark ? SparkR.X - 10.0f : R.X + R.W - PadX;

	// The delta, top right, beside its triangle.
	float LabelRoom = R.X + R.W - PadX;
	if (!K.Delta.empty())
	{
		const float Size = 13.0f;
		const Color Col = K.DeltaSign > 0 ? T.Up : K.DeltaSign < 0 ? T.Down : T.Muted;
		const float Wd = ChartMeasure(C, T, ChartRole::Figure, K.Delta, Size);
		const float X1 = R.X + R.W - PadX - (T.Cut > 0.0f ? std::min(T.Cut, R.H * 0.5f) * 0.5f : 0.0f);
		ChartPrint(C, T, ChartRole::Figure, K.Delta, X1, LabelBase, Size, Col, Align::Right);
		float Left = X1 - Wd;
		if (K.DeltaSign != 0)
		{
			const float Tx = Left - 10.0f;
			const float Cy = LabelBase - Size * 0.36f;
			if (K.DeltaSign > 0)
			{
				C.FillPolygon({{Tx, Cy - 3.5f}, {Tx + 4.0f, Cy + 3.0f}, {Tx - 4.0f, Cy + 3.0f}}, Col);
			}
			else
			{
				C.FillPolygon({{Tx - 4.0f, Cy - 3.0f}, {Tx + 4.0f, Cy - 3.0f}, {Tx, Cy + 3.5f}}, Col);
			}
			Left = Tx - 5.0f;
		}
		LabelRoom = Left - 8.0f;
	}
	if (!K.Label.empty())
	{
		ChartPrint(C, T, ChartRole::Label, K.Label, R.X + PadX, LabelBase, T.LabelSize, T.Muted, Align::Left, Baseline::Alphabetic,
			std::max(10.0f, LabelRoom - R.X - PadX));
	}
	// The figure, twice-printed in a theme with an under-copy unless it carries a colour of its own.
	const bool Own = K.ValueCol.A > 0.0f;
	const float ValueRoom = std::max(10.0f, TextRight - R.X - PadX);
	float ValueW = 0.0f;
	if (!K.Value.empty())
	{
		ValueW = ChartFigure(C, T, K.Value, R.X + PadX, ValueBase, O.ValueSize, Own ? K.ValueCol : T.Value, !Own, Align::Left, ValueRoom);
	}
	if (!K.Sub.empty())
	{
		const float SubX = R.X + PadX + ValueW + 8.0f;
		const float Room = TextRight - SubX;
		if (Room > 16.0f)
		{
			ChartPrint(C, T, ChartRole::Label, K.Sub, SubX, ValueBase - 2.0f, T.LabelSize + 1.5f, T.Muted, Align::Left, Baseline::Alphabetic, Room);
		}
	}
	if (HasNote)
	{
		ChartPrint(C, T, ChartRole::Body, K.Note, R.X + PadX, NoteBase, O.NoteSize, T.Muted, Align::Left, Baseline::Alphabetic, std::max(10.0f, TextRight - R.X - PadX));
	}
	if (HasSpark)
	{
		SparkOpts So;
		So.Area = 0.15f;
		Sparkline(C, SparkR, K.Spark, ChartOr(K.SparkCol, T.Series[0]), So, T);
	}
}

// ------------------------------------------------------------------ Tooltip

Rect Tooltip(Canvas& C, Vec2 At, const std::string& Title, const std::vector<TipRow>& Rows, const Theme& T, const Rect& Within)
{
	const float PadIn = 10.0f;
	const float RowH = 18.0f;
	const float TitleH = Title.empty() ? 0.0f : T.LabelSize + 8.0f;
	float Wd = Title.empty() ? 0.0f : ChartMeasure(C, T, ChartRole::Label, Title, T.LabelSize);
	bool AnySwatch = false;
	for (const TipRow& Rw : Rows)
	{
		AnySwatch = AnySwatch || Rw.Swatch.A > 0.0f;
	}
	const float SwatchW = AnySwatch ? 14.0f : 0.0f;
	for (const TipRow& Rw : Rows)
	{
		const float Lw = ChartMeasure(C, T, ChartRole::Body, Rw.Label, 12.5f);
		const float Vw = ChartMeasure(C, T, ChartRole::Figure, Rw.Value, 13.0f);
		Wd = std::max(Wd, SwatchW + Lw + (Rw.Label.empty() || Rw.Value.empty() ? 0.0f : 18.0f) + Vw);
	}
	Wd = std::min(std::max(Wd + PadIn * 2.0f, 96.0f), std::max(96.0f, Within.W));
	const float Ht = PadIn * 2.0f + TitleH + RowH * static_cast<float>(Rows.size()) - (Rows.empty() ? 0.0f : 4.0f);
	float X = At.X + 14.0f;
	if (X + Wd > Within.X + Within.W)
	{
		X = At.X - 14.0f - Wd;
	}
	X = std::max(Within.X, std::min(X, Within.X + Within.W - Wd));
	float Y = At.Y - Ht * 0.5f;
	Y = std::max(Within.Y, std::min(Y, Within.Y + Within.H - Ht));
	const Rect Box{X, Y, Wd, Ht};
	ChartPanel(C, Box, T.Tip, T.Tip, T.Edge, std::min(T.Radius, 6.0f), T.Cut > 0.0f ? 6.0f : 0.0f);
	float Cursor = Y + PadIn;
	if (!Title.empty())
	{
		ChartPrint(C, T, ChartRole::Label, Title, X + PadIn, Cursor + T.LabelSize * 0.8f, T.LabelSize, T.Muted, Align::Left, Baseline::Alphabetic, Wd - PadIn * 2.0f);
		Cursor += TitleH;
	}
	for (const TipRow& Rw : Rows)
	{
		const float Base = Cursor + 13.0f;
		float Lx = X + PadIn;
		if (Rw.Swatch.A > 0.0f)
		{
			if (T.Radius > 0.0f)
			{
				C.FillCircle(Lx + 4.0f, Base - 4.5f, 4.0f, Rw.Swatch);
			}
			else
			{
				C.FillRect({Lx, Base - 8.5f, 8.0f, 8.0f}, Rw.Swatch);
			}
		}
		Lx += SwatchW;
		const float Vw = Rw.Value.empty() ? 0.0f : ChartPrint(C, T, ChartRole::Figure, Rw.Value, X + Wd - PadIn, Base, 13.0f, ChartOr(Rw.ValueCol, T.Ink), Align::Right);
		if (!Rw.Label.empty())
		{
			ChartPrint(C, T, ChartRole::Body, Rw.Label, Lx, Base, 12.5f, T.Ink, Align::Left, Baseline::Alphabetic, std::max(10.0f, X + Wd - PadIn - Vw - 10.0f - Lx));
		}
		Cursor += RowH;
	}
	return Box;
}

// ------------------------------------------------------------------ Donut

int Donut(Canvas& C, float Cx, float Cy, float Radius, float Thick, const std::vector<Slice>& Slices, const Theme& T, const std::string& Figure,
	const std::string& Caption, Vec2 Mouse, float Reveal)
{
	const float Th = std::max(1.0f, std::min(Thick, Radius));
	const float Mid = Radius - Th * 0.5f;
	double Total = 0.0;
	for (const Slice& S : Slices)
	{
		Total += std::isfinite(S.V) && S.V > 0.0 ? S.V : 0.0;
	}
	int Hover = -1;
	const float Sweep = 2.0f * Pi * std::max(0.0f, std::min(1.0f, Reveal));
	if (Total <= 0.0)
	{
		C.StrokeArc(Cx, Cy, Mid, -Pi * 0.5f, Pi * 1.5f, T.Grid, Th, false);
	}
	else
	{
		const float Dx = Mouse.X - Cx;
		const float Dy = Mouse.Y - Cy;
		const float Dist = std::sqrt(Dx * Dx + Dy * Dy);
		float MouseA = std::atan2(Dy, Dx) + Pi * 0.5f; // from 12 o'clock, clockwise
		if (MouseA < 0.0f)
		{
			MouseA += 2.0f * Pi;
		}
		const bool OnRing = Mouse.X >= 0.0f && Dist >= Radius - Th - 2.0f && Dist <= Radius + 2.0f && Reveal >= 1.0f;
		const float GapA = Slices.size() > 1 ? std::min(0.04f, 1.5f / std::max(1.0f, Mid)) : 0.0f;
		double Acc = 0.0;
		for (size_t S = 0; S < Slices.size(); ++S)
		{
			const double V = std::isfinite(Slices[S].V) && Slices[S].V > 0.0 ? Slices[S].V : 0.0;
			const float A0 = static_cast<float>(Acc / Total) * 2.0f * Pi;
			Acc += V;
			const float A1 = static_cast<float>(Acc / Total) * 2.0f * Pi;
			if (V <= 0.0)
			{
				continue;
			}
			if (OnRing && MouseA >= A0 && MouseA < A1)
			{
				Hover = static_cast<int>(S);
			}
			float From = A0 + GapA * 0.5f;
			float To = std::min(A1 - GapA * 0.5f, Sweep);
			if (A1 - A0 <= GapA)
			{
				From = A0;
				To = std::min(A1, Sweep);
			}
			if (To <= From)
			{
				continue;
			}
			const bool Lit = Hover == static_cast<int>(S);
			const float W = Lit ? Th + 4.0f : Th;
			const float Rm = Lit ? Mid + 1.0f : Mid;
			C.StrokeArc(Cx, Cy, std::min(Rm, Radius - W * 0.5f), From - Pi * 0.5f, To - Pi * 0.5f, ChartSeriesCol(T, Slices[S].Col, S), W, false);
		}
	}
	const float Hole = Radius - Th;
	if (!Figure.empty())
	{
		const float Size = std::max(10.0f, std::min(30.0f, Hole * 0.62f));
		ChartFigure(C, T, Figure, Cx, Caption.empty() ? Cy + Size * 0.36f : Cy + Size * 0.18f, Size, T.Value, true, Align::Center, Hole * 1.7f);
	}
	if (!Caption.empty())
	{
		ChartPrint(C, T, ChartRole::Label, Caption, Cx, Cy + (Figure.empty() ? 4.0f : std::max(10.0f, std::min(30.0f, Hole * 0.62f)) * 0.18f + T.LabelSize + 4.0f),
			T.LabelSize, T.Muted, Align::Center, Baseline::Alphabetic, Hole * 1.6f);
	}
	return Hover;
}

// ------------------------------------------------------------------ Funnel

void Funnel(Canvas& C, const Rect& R, const std::vector<Stage>& Stages, const Theme& T, const Color& Col, bool Log)
{
	if (Stages.empty() || R.W <= 20.0f || R.H <= 8.0f)
	{
		return;
	}
	const Color Fill = ChartOr(Col, T.Accent);
	const size_t N = Stages.size();
	const float RowH = R.H / static_cast<float>(N);
	const float BarH = std::max(4.0f, std::min(RowH - 6.0f, 26.0f));
	float NameW = 0.0f;
	float FigW = 0.0f;
	std::vector<std::string> Figures;
	std::vector<std::string> Shares;
	for (size_t K = 0; K < N; ++K)
	{
		NameW = std::max(NameW, ChartMeasure(C, T, ChartRole::Body, Stages[K].Name, 12.5f));
		Figures.push_back(Stages[K].Value.empty() ? AxisCount(Stages[K].V) : Stages[K].Value);
		std::string Share;
		if (K > 0 && std::isfinite(Stages[K - 1].V) && Stages[K - 1].V > 0.0 && std::isfinite(Stages[K].V))
		{
			Share = AxisPercent(Stages[K].V / Stages[K - 1].V);
		}
		Shares.push_back(Share);
		FigW = std::max(FigW, ChartMeasure(C, T, ChartRole::Figure, Figures.back(), 13.0f) + 8.0f + ChartMeasure(C, T, ChartRole::Body, Share, 11.5f));
	}
	NameW = std::min(NameW, R.W * 0.32f);
	FigW = std::min(FigW, R.W * 0.32f);
	const float X0 = R.X + NameW + 12.0f;
	const float X1 = R.X + R.W - FigW - 12.0f;
	const float Span = std::max(4.0f, X1 - X0);
	const double Top = std::isfinite(Stages[0].V) ? std::max(Stages[0].V, 0.0) : 0.0;
	double Widest = Top;
	for (const Stage& S : Stages)
	{
		Widest = std::max(Widest, std::isfinite(S.V) ? S.V : 0.0);
	}
	for (size_t K = 0; K < N; ++K)
	{
		const float Cy = R.Y + RowH * (static_cast<float>(K) + 0.5f);
		const double V = std::isfinite(Stages[K].V) ? std::max(0.0, Stages[K].V) : 0.0;
		double Frac = 0.0;
		if (Widest > 0.0)
		{
			Frac = Log ? std::log10(1.0 + V) / std::log10(1.0 + Widest) : V / Widest;
		}
		const float Wd = std::max(V > 0.0 ? 2.0f : 0.0f, static_cast<float>(Frac) * Span);
		const float Bx = X0 + (Span - Wd) * 0.5f;
		C.FillRect({X0, Cy - BarH * 0.5f, Span, BarH}, ChartFade(T.Grid, 0.35f));
		if (Wd > 0.0f)
		{
			const float Rad = std::min(T.BarRadius * 2.0f, std::min(Wd, BarH) * 0.5f);
			C.FillRoundRect({Bx, Cy - BarH * 0.5f, Wd, BarH}, Rad, ChartFade(Fill, 1.0f - 0.45f * static_cast<float>(K) / static_cast<float>(std::max<size_t>(1, N - 1))));
		}
		ChartPrint(C, T, ChartRole::Body, Stages[K].Name, R.X, Cy, 12.5f, T.Muted, Align::Left, Baseline::Middle, NameW);
		const float Fw = ChartPrint(C, T, ChartRole::Figure, Figures[K], X1 + 12.0f, Cy, 13.0f, T.Ink, Align::Left, Baseline::Middle, FigW);
		if (!Shares[K].empty())
		{
			ChartPrint(C, T, ChartRole::Body, Shares[K], X1 + 12.0f + Fw + 8.0f, Cy, 11.5f, T.Muted, Align::Left, Baseline::Middle);
		}
	}
}

// ------------------------------------------------------------------ Heatmap

HeatOut Heatmap(Canvas& C, const Rect& R, const std::vector<std::vector<double>>& V, const HeatOpts& O, const Theme& T)
{
	HeatOut Out;
	const int NRow = static_cast<int>(V.size());
	int NCol = 0;
	double Lo = std::numeric_limits<double>::infinity();
	double Hi = -std::numeric_limits<double>::infinity();
	for (const std::vector<double>& Rw : V)
	{
		NCol = std::max(NCol, static_cast<int>(Rw.size()));
		for (const double X : Rw)
		{
			if (std::isfinite(X))
			{
				Lo = std::min(Lo, X);
				Hi = std::max(Hi, X);
			}
		}
	}
	if (NRow == 0 || NCol == 0)
	{
		return Out;
	}
	float LabelW = 0.0f;
	for (const std::string& S : O.Rows)
	{
		LabelW = std::max(LabelW, ChartMeasure(C, T, ChartRole::Axis, S, T.AxisSize));
	}
	const float Gutter = O.Rows.empty() ? 0.0f : std::ceil(LabelW) + 8.0f;
	const float Under = O.Cols.empty() ? 0.0f : T.AxisSize + 8.0f;
	const float Gp = std::max(0.0f, O.Gap);
	const float CellW = (R.W - Gutter - Gp * static_cast<float>(NCol - 1)) / static_cast<float>(NCol);
	const float CellH = (R.H - Under - Gp * static_cast<float>(NRow - 1)) / static_cast<float>(NRow);
	if (CellW <= 0.5f || CellH <= 0.5f)
	{
		return Out;
	}
	const Color Hot = ChartOr(O.Hot, T.Accent);
	const Color Cold = Mix(T.Panel, Hot, 0.08f);
	auto CellAt = [&](int Rw, int Cl) { return Rect{R.X + Gutter + (CellW + Gp) * static_cast<float>(Cl), R.Y + (CellH + Gp) * static_cast<float>(Rw), CellW, CellH}; };
	ChartHash H;
	H.St("chart:heat");
	H.In(static_cast<int64_t>(T.Key() & 0x7fffffffffffffffull));
	H.Fl(R.W);
	H.Fl(R.H);
	H.Co(Hot);
	H.Fl(Gp);
	H.In(O.ColEvery);
	for (const std::vector<double>& Rw : V)
	{
		H.Dv(Rw);
	}
	for (const std::string& S : O.Rows)
	{
		H.St(S);
	}
	for (const std::string& S : O.Cols)
	{
		H.St(S);
	}
	for (const auto& Ring : O.Ring)
	{
		H.In(Ring.first);
		H.In(Ring.second);
	}
	const float Rad = std::min(3.0f, T.Radius * 0.3f);
	const Rect Org = R;
	ChartLayer(C, true, H.H, R, [&](Canvas& Cv) {
		for (int Rw = 0; Rw < NRow; ++Rw)
		{
			if (static_cast<size_t>(Rw) < O.Rows.size())
			{
				ChartPrint(Cv, T, ChartRole::Axis, O.Rows[static_cast<size_t>(Rw)], Gutter - 8.0f, CellAt(Rw, 0).Y - Org.Y + CellH * 0.5f, T.AxisSize, T.Muted, Align::Right,
					Baseline::Middle);
			}
			for (int Cl = 0; Cl < NCol; ++Cl)
			{
				Rect Cell = CellAt(Rw, Cl);
				Cell.X -= Org.X;
				Cell.Y -= Org.Y;
				const std::vector<double>& Vals = V[static_cast<size_t>(Rw)];
				Color Col = ChartFade(T.Grid, 0.5f);
				if (static_cast<size_t>(Cl) < Vals.size() && std::isfinite(Vals[static_cast<size_t>(Cl)]))
				{
					const double U = Hi > Lo ? (Vals[static_cast<size_t>(Cl)] - Lo) / (Hi - Lo) : (Hi > 0.0 ? 1.0 : 0.0);
					Col = Mix(Cold, Hot, static_cast<float>(std::pow(std::max(0.0, std::min(1.0, U)), 0.9)));
				}
				if (Rad > 0.05f)
				{
					Cv.FillRoundRect(Cell, Rad, Col);
				}
				else
				{
					Cv.FillRect(Cell, Col);
				}
			}
		}
		for (const auto& Ring : O.Ring)
		{
			if (Ring.first < 0 || Ring.first >= NRow || Ring.second < 0 || Ring.second >= NCol)
			{
				continue;
			}
			Rect Cell = CellAt(Ring.first, Ring.second);
			Cv.StrokeRoundRect({Cell.X - Org.X + 1.0f, Cell.Y - Org.Y + 1.0f, Cell.W - 2.0f, Cell.H - 2.0f}, Rad, T.Ink, 1.5f);
		}
		const int Every = std::max(1, O.ColEvery);
		for (int Cl = 0; Cl < NCol && static_cast<size_t>(Cl) < O.Cols.size(); Cl += Every)
		{
			const std::string& S = O.Cols[static_cast<size_t>(Cl)];
			if (S.empty())
			{
				continue;
			}
			const float Wd = ChartMeasure(Cv, T, ChartRole::Axis, S, T.AxisSize);
			const float X = std::max(Gutter, std::min(CellAt(0, Cl).X - Org.X, Org.W - Wd));
			ChartPrint(Cv, T, ChartRole::Axis, S, X, Org.H - 3.0f, T.AxisSize, T.Muted);
		}
	});
	if (O.Mouse.X >= R.X + Gutter && O.Mouse.Y >= R.Y && O.Mouse.Y < R.Y + R.H - Under)
	{
		const int Cl = static_cast<int>(std::floor((O.Mouse.X - R.X - Gutter) / (CellW + Gp)));
		const int Rw = static_cast<int>(std::floor((O.Mouse.Y - R.Y) / (CellH + Gp)));
		if (Cl >= 0 && Cl < NCol && Rw >= 0 && Rw < NRow)
		{
			Out.Row = Rw;
			Out.Col = Cl;
			Out.Cell = CellAt(Rw, Cl);
			C.StrokeRoundRect({Out.Cell.X + 0.5f, Out.Cell.Y + 0.5f, Out.Cell.W - 1.0f, Out.Cell.H - 1.0f}, Rad, ChartFade(T.Ink, 0.85f), 1.0f);
		}
	}
	return Out;
}

// ------------------------------------------------------------------ RatioBar

void RatioBar(Canvas& C, const Rect& R, double A, double B, const Theme& T, double Typical, const Color& ColA, const Color& ColB)
{
	if (R.W <= 2.0f || R.H <= 1.0f)
	{
		return;
	}
	const bool Tick = Typical >= 0.0 && Typical <= 1.0;
	const float Inset = Tick ? std::min(4.0f, R.H * 0.25f) : 0.0f;
	const Rect Bar{R.X, R.Y + Inset, R.W, std::max(1.0f, R.H - Inset * 2.0f)};
	const float Rad = T.Radius > 0.0f ? Bar.H * 0.5f : 0.0f;
	const double Va = std::isfinite(A) ? std::max(0.0, A) : 0.0;
	const double Vb = std::isfinite(B) ? std::max(0.0, B) : 0.0;
	const double Total = Va + Vb;
	if (Total <= 0.0)
	{
		C.FillRoundRect(Bar, Rad, T.Grid);
	}
	else
	{
		const float Share = static_cast<float>(Va / Total);
		C.FillRoundRect(Bar, Rad, ChartOr(ColB, T.Down));
		if (Share > 0.0f)
		{
			const float Wd = std::max(Bar.H > Bar.W * Share ? std::min(Bar.W, Rad * 2.0f) : 0.0f, Bar.W * Share);
			C.FillRoundRect({Bar.X, Bar.Y, Wd, Bar.H}, std::min(Rad, Wd * 0.5f), ChartOr(ColA, T.Up));
		}
	}
	if (Tick)
	{
		const float X = R.X + R.W * static_cast<float>(Typical);
		C.FillRect({std::max(R.X, std::min(X - 0.75f, R.X + R.W - 1.5f)), R.Y, 1.5f, R.H}, T.Ink);
	}
}

// ------------------------------------------------------------------ icons

void DrawIcon(Canvas& C, Icon I, float Cx, float Cy, float Size, const Color& Col)
{
	const float S = Size;
	const float Lw = std::max(1.2f, S * 0.09f);
	const float X0 = Cx - S * 0.5f;
	const float Y0 = Cy - S * 0.5f;
	auto P = [X0, Y0, S](float U, float V) { return Vec2{X0 + U * S, Y0 + V * S}; };
	switch (I)
	{
	case Icon::ThumbUp:
	case Icon::ThumbDown:
	{
		const bool Down = I == Icon::ThumbDown;
		auto Q = [&](float U, float V) { return Down ? P(1.0f - U, 1.0f - V) : P(U, V); };
		// The cuff, then the hand: a palm with the thumb raised.
		const Vec2 C0 = Q(0.08f, 0.44f);
		const Vec2 C1 = Q(0.28f, 0.92f);
		C.FillRect({std::min(C0.X, C1.X), std::min(C0.Y, C1.Y), std::fabs(C1.X - C0.X), std::fabs(C1.Y - C0.Y)}, Col);
		C.FillPolygon({Q(0.34f, 0.44f), Q(0.50f, 0.20f), Q(0.54f, 0.06f), Q(0.64f, 0.08f), Q(0.66f, 0.22f), Q(0.62f, 0.38f), Q(0.88f, 0.38f), Q(0.94f, 0.46f),
						  Q(0.86f, 0.90f), Q(0.80f, 0.92f), Q(0.34f, 0.92f)},
			Col);
		break;
	}
	case Icon::Eye:
	{
		std::vector<Vec2> Lid;
		for (int K = 0; K <= 16; ++K)
		{
			const float U = static_cast<float>(K) / 16.0f;
			Lid.push_back(P(0.06f + 0.88f * U, 0.5f - 0.30f * std::sin(U * Pi)));
		}
		for (int K = 15; K >= 1; --K)
		{
			const float U = static_cast<float>(K) / 16.0f;
			Lid.push_back(P(0.06f + 0.88f * U, 0.5f + 0.30f * std::sin(U * Pi)));
		}
		C.StrokePolyline(Lid, true, Col, Lw);
		C.FillCircle(Cx, Cy, S * 0.15f, Col);
		break;
	}
	case Icon::Clock:
	{
		C.StrokeEllipse(Cx, Cy, S * 0.42f, S * 0.42f, Col, Lw);
		C.StrokePolyline({P(0.5f, 0.24f), P(0.5f, 0.5f), P(0.68f, 0.62f)}, false, Col, Lw, true);
		break;
	}
	case Icon::PersonPlus:
	{
		C.FillCircle(X0 + S * 0.38f, Y0 + S * 0.30f, S * 0.17f, Col);
		std::vector<Vec2> Body;
		for (int K = 0; K <= 12; ++K)
		{
			const float A = Pi + Pi * static_cast<float>(K) / 12.0f;
			Body.push_back({X0 + S * 0.38f + S * 0.30f * std::cos(A), Y0 + S * 0.88f + S * 0.34f * std::sin(A)});
		}
		C.FillPolygon(Body, Col);
		C.FillRect({X0 + S * 0.74f, Y0 + S * 0.40f - Lw * 0.5f, S * 0.24f, Lw}, Col);
		C.FillRect({X0 + S * 0.86f - Lw * 0.5f, Y0 + S * 0.28f, Lw, S * 0.24f}, Col);
		break;
	}
	case Icon::Coin:
	{
		C.FillCircle(Cx, Cy, S * 0.42f, Col);
		C.StrokeEllipse(Cx, Cy, S * 0.30f, S * 0.30f, Mix(Col, Color{0.0f, 0.0f, 0.0f, Col.A}, 0.35f), std::max(1.0f, Lw * 0.8f));
		C.FillRect({Cx - Lw * 0.5f, Cy - S * 0.17f, Lw, S * 0.34f}, Mix(Col, Color{0.0f, 0.0f, 0.0f, Col.A}, 0.35f));
		break;
	}
	case Icon::Magnifier:
	{
		C.StrokeEllipse(X0 + S * 0.42f, Y0 + S * 0.42f, S * 0.28f, S * 0.28f, Col, Lw);
		C.StrokePolyline({P(0.63f, 0.63f), P(0.90f, 0.90f)}, false, Col, Lw * 1.4f, true);
		break;
	}
	case Icon::Comment:
	{
		C.StrokeRoundRect({X0 + S * 0.08f, Y0 + S * 0.14f, S * 0.84f, S * 0.56f}, S * 0.12f, Col, Lw);
		C.FillPolygon({P(0.26f, 0.68f), P(0.44f, 0.68f), P(0.24f, 0.88f)}, Col);
		break;
	}
	case Icon::Share:
	{
		const Vec2 A = P(0.76f, 0.20f);
		const Vec2 B = P(0.24f, 0.50f);
		const Vec2 D = P(0.76f, 0.80f);
		C.StrokePolyline({A, B, D}, false, Col, Lw);
		C.FillCircle(A.X, A.Y, S * 0.13f, Col);
		C.FillCircle(B.X, B.Y, S * 0.13f, Col);
		C.FillCircle(D.X, D.Y, S * 0.13f, Col);
		break;
	}
	case Icon::Globe:
	{
		C.StrokeEllipse(Cx, Cy, S * 0.42f, S * 0.42f, Col, Lw);
		C.StrokeEllipse(Cx, Cy, S * 0.18f, S * 0.42f, Col, Lw * 0.8f);
		C.FillRect({Cx - S * 0.42f, Cy - Lw * 0.4f, S * 0.84f, Lw * 0.8f}, Col);
		C.StrokePolyline({P(0.16f, 0.30f), P(0.84f, 0.30f)}, false, Col, Lw * 0.7f);
		C.StrokePolyline({P(0.16f, 0.70f), P(0.84f, 0.70f)}, false, Col, Lw * 0.7f);
		break;
	}
	case Icon::Count:
		break;
	}
}
} // namespace chart
} // namespace ui
} // namespace ss
