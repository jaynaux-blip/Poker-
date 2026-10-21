#pragma once

#include "ShortStack/Game/SiteData.h"
#include "ShortStack/UI/Canvas.h"

namespace ss
{
namespace ui
{
/**
 * The poker sites' marks (update 2 §1.8, §5.3): siteart::Logo is the one source of a site's logo (the poker button, the
 * lobby, the Cashier, Mail, a video's thumbnail), Strip the narrow band a table tile carries. RiverLine's is today's;
 * RunItTwice's (Update 3 G1, the 2026-10-21 restyle) is red and black: the turnstone's two cards in chrome and Signal Red on a
 * gloss-black tile, "Twice" in LED Red, and a red Strip with the name in Hot White. Fictional sites only.
 *
 * Bodies: SiteArt.cpp (core-sites-look from Update 3; this header is append-only).
 */
namespace siteart
{
SHORTSTACKCORE_API void Logo(Canvas& C, Rect R, net::SiteId S);
SHORTSTACKCORE_API void Strip(Canvas& C, Rect R, net::SiteId S);

// Update 3 (SPEC §2.5, §3.3-§3.5, §8.3 #4; G1, core-sites-look; appended):
/** RunItTwice's mark, the turnstone's two river cards (the second run dealt over the first), fitted and centred in R. */
SHORTSTACKCORE_API void TwinCards(Canvas& C, Rect R, const Color& First, const Color& Second, const Color& Edge);
/** The Twin Stripe card back's centre mark: the two cards in Hot White, the second edged in Onyx (§3.5's Cards). */
SHORTSTACKCORE_API void TwinStripeMark(Canvas& C, Rect R);
/**
 * Ignite's neon sign, the felt's watermark (§2.5): the flame and "Ignite" in BlackItalic as a lit tube, standing on Base from
 * X, Size its cap height. Three soft Hot Pink copies at 0.06, 0.10 and 0.16 under a crisp coral one at 0.22, each times
 * Strength (1 for the felt); only colours' own alpha, so the parent alpha carries through. Returns its width.
 */
SHORTSTACKCORE_API float NeonSign(Canvas& C, float X, float Base, float Size, float Strength);
/**
 * A League division's crest in its status metal (§3.3's status ramp), Size tall, centred on (Cx, Cy): Div 0 Open (a chrome
 * outline, 1 bar), 1 Iron (carbon, 2), 2 Steel (chrome, 3), 3 Gold (Status Gold, 4), 4 High Bridge (red lacquer and its
 * arch); Stars (0-5) for a group winner, in Hot White above it.
 */
SHORTSTACKCORE_API void DivisionShield(Canvas& C, float Cx, float Cy, float Size, int Div, int Stars = 0);
/** A brushed chrome plate with one 45-degree cut of Cut on its top-right corner (credentials, the Member File): engrave it in Onyx. */
SHORTSTACKCORE_API void ChromePlate(Canvas& C, Rect R, float Cut);
} // namespace siteart
} // namespace ui
} // namespace ss
