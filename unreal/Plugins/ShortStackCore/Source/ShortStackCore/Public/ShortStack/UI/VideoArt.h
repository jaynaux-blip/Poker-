#pragma once

#include "ShortStack/Game/Video.h"
#include "ShortStack/UI/Canvas.h"

namespace ss
{
namespace ui
{
/**
 * Rerun's art (update 2 §4.2, §4.5): the logo (marigold on ink, clear of Kast's violet and lime and any platform's red),
 * a video's thumbnail from its frame (vector, in core: no scene capture), and a video's tile on the Channel page. Every
 * number on a thumbnail is the hand's own (U7).
 *
 * Bodies: core-video (VideoArt.cpp). Phase A, day 1: stubs that compile and draw nothing.
 */
namespace videoart
{
SHORTSTACKCORE_API void Logo(Canvas& C, Rect R);
SHORTSTACKCORE_API void Thumbnail(Canvas& C, Rect R, const video::Thumb& T);
SHORTSTACKCORE_API void Tile(Canvas& C, Rect R, const video::Video& V, double World);
} // namespace videoart
} // namespace ui
} // namespace ss
