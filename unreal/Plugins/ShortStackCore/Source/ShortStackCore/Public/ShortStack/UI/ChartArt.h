#pragma once

#include "ShortStack/UI/Canvas.h"

#include <string>
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
 * clock, person-plus, coin, magnifier, comment, share, globe). Every number a chart shows is the core's.
 *
 * Declared by ui-rerun (it owns this header after Phase A and lands LineChart, Bars, Sparkline, Kpi and Tooltip by B1 day 3,
 * for ui-rit); bodies in ChartArt.cpp. Phase A, day 1: the namespace only.
 */
namespace chart
{
} // namespace chart
} // namespace ui
} // namespace ss
