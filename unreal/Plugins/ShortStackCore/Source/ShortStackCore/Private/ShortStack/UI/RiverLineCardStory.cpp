// Update 4, the player card's STORY tab (SPEC v1.7 §3.19.2): the TPI line and the pill, the rank graph with its pins, THE STORY SO
// FAR, YOU AND THEM with the hero's notes, and the FOLLOW toggle. W-ui's file from G2.
//
// Phase A (§6 PA-47; the lead): a stub that draws nothing. The card shows its fifth tab only while Session::StoryTabShown(), never
// before G2, so CardStory is never reached. Step A-2 (the lead; LEAD_CALLS Q-6): the hooks for the card's other G2 changes (§3.19.3)
// and the results screen's FOLLOW toggles, each drawing nothing and answering false (or 0) until G2.
#include "ShortStack/UI/RiverLine.h"

namespace ss
{
namespace ui
{
void RiverLine::CardStory(const world::Profile& /*P*/, const Rect& /*Card*/, double /*Now*/)
{
}

bool RiverLine::CardFollow(const world::Profile& /*P*/, const Rect& /*Card*/, double /*Now*/)
{
	return false;
}

bool RiverLine::CardTpiLine(const world::Profile& /*P*/, const Rect& /*Card*/, double /*Now*/)
{
	return false;
}

bool RiverLine::HeroCardTpiLine(const Rect& /*Card*/, double /*Now*/)
{
	return false;
}

float RiverLine::CardCameFor(const world::Profile& /*P*/, float /*X*/, float /*Y*/, double /*Now*/)
{
	return 0.0f;
}

bool RiverLine::ResultsFollow(const Rect& /*Area*/, double /*Now*/)
{
	return false;
}
} // namespace ui
} // namespace ss
