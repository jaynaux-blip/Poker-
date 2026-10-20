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
 * RunItTwice's is its patch's (art/blender/sponsors.py). Fictional sites only.
 *
 * Bodies: ui-laptop (SiteArt.cpp). Phase A, day 1: stubs that compile and draw nothing.
 */
namespace siteart
{
SHORTSTACKCORE_API void Logo(Canvas& C, Rect R, net::SiteId S);
SHORTSTACKCORE_API void Strip(Canvas& C, Rect R, net::SiteId S);
} // namespace siteart
} // namespace ui
} // namespace ss
