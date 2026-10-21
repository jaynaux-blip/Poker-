#pragma once

#include "ShortStack/UI/Canvas.h"
#include "ShortStack/UI/Ui.h"

#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace ui
{
/**
 * Update 3 (SPEC §5.2, §7.3): the chart kit Rerun's Analytics draws with, and RunItTwice's Desk, League and Rankings reuse:
 * NiceTicks; the axes (AxisCount, AxisMoney, AxisDuration "3:12", AxisPercent, AxisDate); LineChart (areas, a typical band,
 * dashed series through Canvas::StrokeDashed, markers, the hover index, the reveal); Bars (stacked); Sparkline; Donut
 * (StrokeArc with butt caps per slice); Funnel; Heatmap; RatioBar; Kpi; Tooltip; and the icons (thumbs up and down, eye,
 * clock, person-plus, coin, magnifier, comment, share, globe). Every number a chart shows is the core's: the kit draws and
 * formats the values it is handed, never computes one.
 *
 * ui-rerun's header after Phase A (append-only; each change goes to ui-rit as a STATUS note); bodies in ChartArt.cpp.
 *
 * Looks. Every chart takes a Theme: Rerun() (marigold on ink, drawn outside every SkinScope), FromSkin(S) (a site's 16
 * tokens) or RunItTwice(S) (the Exchange's red and black: BoldCondensed caps, the twice print, the one-cut corner; S is the
 * client's skin, Skin::Current() inside its SkinScope). Series carry their own colours; alpha 0 means "the theme's".
 *
 * Caching. A chart's static layers (grid, axes, areas, lines, bars) go through Canvas::Cached (the Shared pool) keyed by
 * everything they draw, the theme's colours included, so a page that caches its core view by world minute or day replays
 * them; the hover, the reveal's clip, the last point and the markers draw live. Pass Cache = false for data that changes
 * every frame. Nothing here mutates anything but the canvas.
 */
namespace chart
{
// ------------------------------------------------------------------ the look

struct Theme
{
	Color Ground;    // the page under the chart
	Color Panel;     // a tile's ground (Kpi), the top of its gradient
	Color Panel2;    // the bottom of a tile's gradient (Panel for a flat tile)
	Color Edge;      // a tile's 1-unit edge (alpha 0: none)
	Color Tip;       // the tooltip's ground
	Color Grid;      // grid lines
	Color Axis;      // the zero line and the baseline
	Color Ink;       // values, tooltip text
	Color Muted;     // labels and axis figures (>= 4.5:1 on Ground and Panel in every shipped theme)
	Color Dim;       // decorative only: never carries a number
	Color Up;        // gains, a delta up
	Color Down;      // losses, a delta down (always beside a vector triangle or a minus)
	Color Amber;     // projected, cautionary
	Color Accent;    // the brand's light: the hover, the last point, a selected tile (Rerun marigold; RunItTwice Signal Red)
	Color Value;     // a Kpi's figure (Rerun cream; RunItTwice Hot White)
	Color Under;     // the twice print's under-copy (alpha 0: everything prints once); RunItTwice's Signal Red
	std::array<Color, 6> Series; // the default colours of series 0..5
	int8_t LabelFace = -1;  // a Font, or -1 for the face LabelWeight maps to
	int LabelWeight = 800;
	bool Caps = false;      // labels drawn in capitals whatever they're given
	int8_t FigureFace = -1; // Kpi figures, tooltip values, donut centres
	int FigureWeight = 800;
	int8_t AxisFace = -1;   // axis figures
	int AxisWeight = 600;
	int BodyWeight = 600;   // notes, tooltip rows, stage names (always the weight's face)
	float AxisSize = 11.0f;
	float LabelSize = 11.5f;
	float Radius = 10.0f;   // a tile's corners (0: square)
	float Cut = 0.0f;       // RunItTwice: square corners with one 45-degree cut on the top-right (overrides Radius)
	float BarRadius = 1.0f; // a bar's corners
	float TwiceFrom = 28.0f; // figures at least this big print twice when Under is set
	/** A hash of everything above (cache keys mix it in). */
	SHORTSTACKCORE_API uint64_t Key() const;
};
/** Rerun's look (RiverLineVideoShared.h's constants): cream on ink, marigold light, rounded cards. */
SHORTSTACKCORE_API Theme Rerun();
/** A site's 16 tokens with the house faces (RiverLine's, Ignite's or any skin's). */
SHORTSTACKCORE_API Theme FromSkin(const Skin& S);
/** RunItTwice's Exchange (SPEC §3.3): S's red and black, BoldCondensed caps, figures twice-printed, tiles in gloss with one cut. */
SHORTSTACKCORE_API Theme RunItTwice(const Skin& S = Skin::Current());
/** The twice print's offset for a size (SPEC §3.3: round(Size / 16) clamped to 2..6). */
SHORTSTACKCORE_API float TwiceOffset(float Size);

// ------------------------------------------------------------------ scales and axes

/** A "nice" axis: Lo and Hi widened to whole Steps (1, 2, 2.5 or 5 times a power of ten) and the ticks between. */
struct AxisScale
{
	double Lo = 0.0;
	double Hi = 1.0;
	double Step = 0.25;
	std::vector<double> Ticks;
	/** V's position between Px0 (at Lo) and Px1 (at Hi). */
	float Map(double V, float Px0, float Px1) const { return Hi > Lo ? Px0 + static_cast<float>((V - Lo) / (Hi - Lo)) * (Px1 - Px0) : Px0; }
};
/** About Target intervals over [Lo, Hi] (with 0 inside when Zero; whole steps when Integer). Empty, one value and NaN are safe. */
SHORTSTACKCORE_API AxisScale NiceScale(double Lo, double Hi, int Target = 4, bool Zero = true, bool Integer = false);
/** NiceScale's ticks. */
SHORTSTACKCORE_API std::vector<double> NiceTicks(double Lo, double Hi, int Target = 4);

/** How an axis writes its figures. Step (the axis's tick step, 0 for a lone figure) sets the decimals. */
enum class Units : uint8_t
{
	Count,    // "0", "750", "1.5K", "61K", "1.2M"
	Money,    // cents: "$0", "$2.50", "$250", "$1.2K", "-$40"
	Duration, // seconds: "0:30", "3:12", "1:02:03"
	Percent,  // a fraction: "4.6%", "46%"
	Plain,    // the number with Step's decimals
};
SHORTSTACKCORE_API std::string AxisCount(double V, double Step = 0.0);
SHORTSTACKCORE_API std::string AxisMoney(double Cents, double Step = 0.0);
SHORTSTACKCORE_API std::string AxisDuration(double Seconds);
SHORTSTACKCORE_API std::string AxisPercent(double Fraction, double Step = 0.0);
/** A world day as "Oct 6" (net::DateLabel). */
SHORTSTACKCORE_API std::string AxisDate(int Day);
SHORTSTACKCORE_API std::string AxisText(Units U, double V, double Step = 0.0);

/** The x axis: one label per point index, chosen by the chart so labels never touch. */
struct XAxis
{
	std::vector<std::string> Labels; // one per index ("" for none); wins over Days
	int FirstDay = std::numeric_limits<int>::min(); // else: index i is world day FirstDay + i * DayStep, written "Oct 6"
	int DayStep = 1;
	int Count = 6; // about this many labels
};

// ------------------------------------------------------------------ LineChart

struct Series
{
	std::vector<double> Y;       // one value per index (NaN: a gap)
	Color Col{0, 0, 0, 0};       // alpha 0: Theme::Series[its place]
	float Width = 2.0f;
	float Area = 0.0f;           // the area to the baseline: alpha at the line's far edge (0: none)
	float AreaFade = 1.0f;       // the area's alpha at the baseline, as a share of Area (Rerun 0.06; RunItTwice's flat 1)
	float Dash = 0.0f, Gap = 0.0f; // dashed (a typical curve; RunItTwice's "adjusted")
	bool Dots = false;           // a dot at every point
	float DotR = 2.5f;
	bool Last = false;           // the last point lit: a dot and a tick on the value axis (the Desk's red dot)
	Color LastCol{0, 0, 0, 0};   // alpha 0: Theme::Accent
	std::string Name;            // its line in a tooltip
};

/** The typical range (Rerun: the p25-p75 of the last 10 uploads), drawn under the series. */
struct Band
{
	std::vector<double> Lo, Hi;
	Color Col{1.0f, 1.0f, 1.0f, 0.09f};
};

/** A moment on the x axis: a numbered bubble over the plot (Tag "1", "2"), or a pin on the baseline (an upload). */
struct Mark
{
	int At = 0;
	std::string Tag;
	bool Pin = false;
	Color Col{0, 0, 0, 0}; // alpha 0: Accent for a bubble, Muted for a pin
};

struct LineOpts
{
	Units Y = Units::Count;
	int YTicks = 4;
	bool Zero = true;       // the scale includes 0
	bool Integer = false;   // whole-number ticks (counts of events)
	double YMin = std::numeric_limits<double>::quiet_NaN(); // a fixed range (NaN: from the data)
	double YMax = std::numeric_limits<double>::quiet_NaN();
	XAxis X;
	bool Grid = true;
	bool YLabels = true;
	bool XLabels = true;
	float Reveal = 1.0f;    // 0..1: the series draw in from the left (a closed form of Now - At, the caller's)
	Vec2 Mouse{-1.0f, -1.0f}; // the pointer, for the hover index
	std::string Empty;      // drawn in the middle when there is nothing to plot (the caller's copy)
	bool Cache = true;
};

struct LineOut
{
	Rect Plot;          // the plotting area inside R
	AxisScale Ys;
	int Hover = -1;     // the index under the pointer
	Vec2 HoverAt{0.0f, 0.0f}; // the first series' point there (a tooltip's anchor)
	int Points = 0;     // the longest series' length
};

/** Lines over a value axis in R (labels and all inside it). Band and Marks may be null. */
SHORTSTACKCORE_API LineOut LineChart(Canvas& C, const Rect& R, const std::vector<Series>& Lines, const LineOpts& O, const Theme& T,
	const Band* B = nullptr, const std::vector<Mark>* Marks = nullptr);

// ------------------------------------------------------------------ Bars

struct BarSet
{
	std::vector<double> V;     // one value per slot (negative bars hang from 0)
	Color Col{0, 0, 0, 0};     // alpha 0: Theme::Series[its place]
	std::vector<Color> Each;   // per-slot colours (alpha 0 entries: Col); Realtime's last hours lit
	std::string Name;
};

struct BarOpts
{
	bool Stacked = false;   // sets stack in one bar a slot (positives up, negatives down); else side by side
	float Gap = 0.25f;      // the space between slots, as a share of a slot
	Units Y = Units::Count;
	int YTicks = 4;
	bool Integer = false;
	double YMax = std::numeric_limits<double>::quiet_NaN();
	XAxis X;
	bool Grid = true;
	bool YLabels = true;
	bool XLabels = true;
	float Reveal = 1.0f;    // 0..1: the bars grow from 0
	Vec2 Mouse{-1.0f, -1.0f};
	std::string Empty;
	bool Cache = true;
};

struct BarOut
{
	Rect Plot;
	AxisScale Ys;
	int Hover = -1;       // the slot under the pointer
	Rect HoverSlot{0, 0, 0, 0};
	int Slots = 0;
};

SHORTSTACKCORE_API BarOut Bars(Canvas& C, const Rect& R, const std::vector<BarSet>& Sets, const BarOpts& O, const Theme& T);

// ------------------------------------------------------------------ Sparkline

struct SparkOpts
{
	float Width = 2.0f;
	float Area = 0.15f;     // the area under it (0: none)
	bool Last = false;      // the last point lit (Rankings: Chrome with the last point Signal Red)
	Color LastCol{0, 0, 0, 0}; // alpha 0: Theme::Accent
	float LastR = 2.5f;
	bool Zero = false;      // the range includes 0
	bool Cache = true;
};

/** A line with no axes filling R (one value draws a level line, none draws nothing). */
SHORTSTACKCORE_API void Sparkline(Canvas& C, const Rect& R, const std::vector<double>& V, const Color& Col, const SparkOpts& O, const Theme& T);

// ------------------------------------------------------------------ Kpi

struct KpiCard
{
	std::string Label;          // "VIEWS" (in capitals in a Caps theme)
	std::string Value;          // the figure, formatted by the caller from the core's number (a count-up passes its rolled value)
	std::string Sub;            // Muted, after the figure: "(31 lost)", "events"
	Color ValueCol{0, 0, 0, 0}; // alpha 0: Theme::Value (twice-printed in RunItTwice's look); a colour of its own prints once
	std::string Delta;          // "18%", beside a vector triangle (Up or Down by DeltaSign); "" for none
	int DeltaSign = 0;          // +1 up, -1 down, 0 flat (Muted, no triangle)
	std::string Note;           // the line under it: "More than your typical 28 days"
	std::vector<double> Spark;  // a sparkline in the tile's lower right (wide tiles)
	Color SparkCol{0, 0, 0, 0};
	bool Selected = false;      // the tile drives the chart under it: an Accent bar along its top
};

struct KpiOpts
{
	bool Ground = true;     // the tile itself (false: the caller drew it)
	float ValueSize = 34.0f;
	float NoteSize = 12.5f;
};

SHORTSTACKCORE_API void Kpi(Canvas& C, const Rect& R, const KpiCard& K, const Theme& T, const KpiOpts& O = KpiOpts());

// ------------------------------------------------------------------ Tooltip

struct TipRow
{
	std::string Label;
	std::string Value;
	Color Swatch{0, 0, 0, 0};   // a series' colour chip (alpha 0: none)
	Color ValueCol{0, 0, 0, 0}; // alpha 0: Theme::Ink
};

/** A floating card beside At (right of it, flipped left or clamped to stay inside Within). Returns where it drew. */
SHORTSTACKCORE_API Rect Tooltip(Canvas& C, Vec2 At, const std::string& Title, const std::vector<TipRow>& Rows, const Theme& T, const Rect& Within);

// ------------------------------------------------------------------ Donut, Funnel, Heatmap, RatioBar

struct Slice
{
	double V = 0.0;
	Color Col{0, 0, 0, 0}; // alpha 0: Theme::Series[its place]
	std::string Name;
};

/** A ring of slices (butt-capped arcs, clockwise from 12 o'clock) with a figure and a caption in its hole. Returns the hovered slice or -1. */
SHORTSTACKCORE_API int Donut(Canvas& C, float Cx, float Cy, float Radius, float Thick, const std::vector<Slice>& Slices, const Theme& T,
	const std::string& Figure = std::string(), const std::string& Caption = std::string(), Vec2 Mouse = {-1.0f, -1.0f}, float Reveal = 1.0f);

struct Stage
{
	std::string Name;  // "Impressions"
	double V = 0.0;
	std::string Value; // the figure as the caller writes it ("61,408")
};

/** Stages as bars from the widest down, each with its figure and its share of the stage before. Log widths on request. */
SHORTSTACKCORE_API void Funnel(Canvas& C, const Rect& R, const std::vector<Stage>& Stages, const Theme& T, const Color& Col = Color{0, 0, 0, 0}, bool Log = false);

struct HeatOpts
{
	std::vector<std::string> Rows; // row labels on the left ("Mon")
	std::vector<std::string> Cols; // column labels under the grid, every ColEvery-th
	int ColEvery = 3;
	float Gap = 2.0f;
	Color Hot{0, 0, 0, 0};        // the highest cell (alpha 0: Accent); the lowest is Panel
	std::vector<std::pair<int, int>> Ring; // cells ringed in Ink (the hours your uploads went up)
	Vec2 Mouse{-1.0f, -1.0f};
};

struct HeatOut
{
	int Row = -1;
	int Col = -1;
	Rect Cell{0, 0, 0, 0};
};

/** Rows x columns of cells shaded from Panel to Hot by value. */
SHORTSTACKCORE_API HeatOut Heatmap(Canvas& C, const Rect& R, const std::vector<std::vector<double>>& V, const HeatOpts& O, const Theme& T);

/** A share bar: A's part (Up) against B's (Down), with a tick at Typical (a share 0..1, or < 0 for none). */
SHORTSTACKCORE_API void RatioBar(Canvas& C, const Rect& R, double A, double B, const Theme& T, double Typical = -1.0, const Color& ColA = Color{0, 0, 0, 0},
	const Color& ColB = Color{0, 0, 0, 0});

// ------------------------------------------------------------------ icons

enum class Icon : uint8_t
{
	ThumbUp,
	ThumbDown,
	Eye,
	Clock,
	PersonPlus,
	Coin,
	Magnifier,
	Comment,
	Share,
	Globe,
	Count,
};

/** A vector icon filling the square of side Size centred at (Cx, Cy). */
SHORTSTACKCORE_API void DrawIcon(Canvas& C, Icon I, float Cx, float Cy, float Size, const Color& Col);
} // namespace chart
} // namespace ui
} // namespace ss
