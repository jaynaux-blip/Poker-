// Update 3 (SPEC §6.1, §7.3): Rerun 2.0's thumbnail compositor, ui-thumbs' file: a ThumbSpec drawn at any size, cached in
// Rerun's pool, the LOD for small tiles. Phase A, day 1: a stub that draws nothing.
#include "ShortStack/UI/ThumbArt.h"

#include "ShortStack/Game/Hero.h"

namespace ss
{
namespace ui
{
namespace thumbart
{
void Compose(Canvas& /*C*/, Rect /*R*/, const video::ThumbSpec& /*S*/, const hero::Character& /*Look*/, bool /*Lod*/)
{
}
} // namespace thumbart
} // namespace ui
} // namespace ss
