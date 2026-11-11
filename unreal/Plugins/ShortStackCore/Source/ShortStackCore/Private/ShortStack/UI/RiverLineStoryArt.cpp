// Update 4, the stories' art (SPEC v1.7 §3.17 R4, §3.19): the seat plate's one pill, the TPI chip and crest, the Notebook's taskbar
// mark, the rank chevrons and the STORY graph's pins. W-ui's file from G2.
//
// Phase A (§6 PA-47; the lead): stubs that draw nothing. DrawSeat calls DrawSeatPill only for a seat whose SeatVis::Pill isn't 0, and
// no seat has one before G2; the taskbar never shows the Notebook, so NotebookIcon is never reached. Step A-2 (the lead; LEAD_CALLS
// Q-6): SeatBadgeRing, called by the plate only for a seat with a pill, leaves the badge where it is.
#include "ShortStack/UI/RiverLine.h"

namespace ss
{
namespace ui
{
void RiverLine::DrawSeatPill(const SeatVis& /*Seat*/, const Rect& /*Plate*/, double /*Now*/)
{
}

void RiverLine::NotebookIcon(const Rect& /*R*/)
{
}

bool RiverLine::SeatBadgeRing(const SeatVis& /*Seat*/, const Rect& /*Plate*/, float& /*X*/, float& /*Y*/)
{
	return false;
}
} // namespace ui
} // namespace ss
