#include "ShortStack/UI/RiverLine.h"
#include "../StrictFloat.h"

// RiverLine's Backing page (next_segment_spec §2.12): stakes, pieces, swaps and packages. core-stake.
//
// Phase A: a stub that keeps today's behavior. Nothing opens Page::Backing until the Backing tab lands (ui-laptop's nav,
// RiverLineNet.cpp), so the page draws nothing; the body is here so the tab can call it from the day it's wired.

namespace ss
{
namespace ui
{
void RiverLine::BackingPage(double /*Now*/)
{
}
} // namespace ui
} // namespace ss
