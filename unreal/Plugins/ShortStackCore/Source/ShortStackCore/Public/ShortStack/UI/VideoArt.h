#pragma once

#include "ShortStack/Game/Video.h"
#include "ShortStack/UI/Canvas.h"

namespace ss
{
namespace hero
{
struct Character;
}
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

// core-video's additions (G2; append-only, §6.1).
/** The thumbnail with the player's own look on the portrait frames (live hands); Who null draws a plain figure. */
SHORTSTACKCORE_API void Thumbnail(Canvas& C, Rect R, const video::Thumb& T, const hero::Character* Who);
/** Studio's kind glyph: a small mark for the candidate list, centered at (Cx, Cy). */
SHORTSTACKCORE_API void KindGlyph(Canvas& C, float Cx, float Cy, float Size, video::Kind K);
/** Rerun's palette: marigold on ink, cream text (§1.8). */
constexpr uint32_t Marigold = 0xF2A33A;
constexpr uint32_t Ink = 0x16181D;
constexpr uint32_t Cream = 0xFFF4E0;
} // namespace videoart
} // namespace ui
} // namespace ss
