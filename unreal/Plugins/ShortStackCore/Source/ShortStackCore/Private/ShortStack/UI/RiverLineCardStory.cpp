// Update 4, the player card's STORY tab (SPEC v1.7 §3.19.2): the TPI line and the pill, the rank graph with its pins, THE STORY SO
// FAR, YOU AND THEM with the hero's notes, and the FOLLOW toggle. W-ui's file from G2.
//
// Phase A (§6 PA-47; the lead): a stub that draws nothing. The card shows its fifth tab only while Session::StoryTabShown(), never
// before G2, so CardStory is never reached.
#include "ShortStack/UI/RiverLine.h"

namespace ss
{
namespace ui
{
void RiverLine::CardStory(const world::Profile& /*P*/, const Rect& /*Card*/, double /*Now*/)
{
}
} // namespace ui
} // namespace ss
