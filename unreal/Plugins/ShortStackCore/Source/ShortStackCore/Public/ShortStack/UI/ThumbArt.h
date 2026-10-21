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
 * Update 3 (SPEC §6.1, §7.3): Rerun 2.0's thumbnails. Compose draws a ThumbSpec (a recipe core-videocontent generated) into
 * R: the archetype's layout, background, palette, the subject with an expression and a pose, props, the caption in its
 * style, a sticker. Every caption is the facts' own (U7). Lod draws the small-tile version (S-T decides where faces drop).
 * Cached per spec, size and Look in Rerun's cache pool (Canvas::Cached with CachePool::Rerun). Bodies: ui-thumbs
 * (ThumbArt.cpp). Phase A, day 1: a stub that draws nothing.
 */
namespace thumbart
{
SHORTSTACKCORE_API void Compose(Canvas& C, Rect R, const video::ThumbSpec& S, const hero::Character& Look, bool Lod);
} // namespace thumbart
} // namespace ui
} // namespace ss
