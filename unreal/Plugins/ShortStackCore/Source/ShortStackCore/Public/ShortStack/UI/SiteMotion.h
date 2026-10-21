#pragma once

#include "ShortStack/UI/Canvas.h"

#include <cstdint>
#include <vector>

namespace ss
{
namespace ui
{
/**
 * Update 3 (SPEC §1.11, §7.3): the motion kit both revamped sites animate with. Every function is a closed form of Now - At
 * (real seconds; At is stamped in the UI's side state), so an animation never decides an outcome; anything drawn restores
 * the parent alpha (never SetAlpha(1.0f), which is absolute). While Previewing (the Kast stream preview) callers pass a
 * settled clock and everything draws as a still. Declared and bodied by the lead in Phase A (SiteMotion.cpp, tested in
 * rivals_ui's ui_seg_seams.cpp); frozen after.
 */
namespace motion
{
/** From to To over Seconds after At, eased out. */
SHORTSTACKCORE_API double CountUp(double From, double To, double Now, double At, double Seconds);
/** How many of Marks (seconds after At, ascending) the clock has passed: a reveal's stage. */
SHORTSTACKCORE_API int Stage(double Now, double At, const std::vector<double>& Marks);
/** A damped spring from 0 to 1 after At (overshoots a little). */
SHORTSTACKCORE_API double Spring(double Now, double At);
/** A short decaying shake after At, its direction hashed from Id. */
SHORTSTACKCORE_API Vec2 Shake(double Now, double At, uint32_t Id);
/** N pieces of confetti (at most 200 quads) falling through R after At, their paths hashed from Id. */
SHORTSTACKCORE_API void Confetti(Canvas& C, uint32_t Id, double At, double Now, int N, Rect R);
/** A split-flap row: how many of Chars have settled after At. */
SHORTSTACKCORE_API int Flap(double Now, double At, int Chars);
/** Chasing bulbs: bulb I of N's brightness (0..1) at Bpm. */
SHORTSTACKCORE_API double Chase(double Now, int I, int N, double Bpm);
/** The beat clock: 0..1 within the current beat at Bpm. */
SHORTSTACKCORE_API double Beat(double Now, double Bpm);
} // namespace motion
} // namespace ui
} // namespace ss
