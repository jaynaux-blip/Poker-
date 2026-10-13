#include "ShortStack/UI/VideoArt.h"

// Rerun's art (update 2 §4.2, §4.5). Phase A, day 1 (the lead): stubs that draw nothing. core-video owns this file from
// here.

namespace ss
{
namespace ui
{
namespace videoart
{
void Logo(Canvas& C, Rect R)
{
	(void)C;
	(void)R;
}

void Thumbnail(Canvas& C, Rect R, const video::Thumb& T)
{
	(void)C;
	(void)R;
	(void)T;
}

void Tile(Canvas& C, Rect R, const video::Video& V, double World)
{
	(void)C;
	(void)R;
	(void)V;
	(void)World;
}
} // namespace videoart
} // namespace ui
} // namespace ss
