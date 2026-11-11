// Update 4, the festival hub (SPEC v1.7 §2.8): one shared module worn in each site's frame (THE FESTIVAL YEAR, the banner, the Main
// card, the day strip and rows, the ticker, THE RACE, CHAMPIONS, YOUR <FESTIVAL>), Main Event Day, the champion's, the belt's, a
// mark's and a title's moments, the strip's popover, the Career page's pins and the hub's strings (FestUiInventory). F-hubs' file
// from G1. It reaches the client's private state through rlfest_detail::Access (RiverLine.h's friend), forwarders defined here.
//
// Phase A (§6 PA-46; the lead): stubs that draw nothing. Draw calls FestivalOverlays at its overlay stage; no page routes to
// FestivalHub or CareerPins before G1. Access is empty.
#include "ShortStack/UI/RiverLine.h"

#include "ShortStack/Game/Festival.h"

#include <string>
#include <vector>

namespace ss
{
namespace fest
{
// Step A-2 (LEAD_CALLS Q-5; moved here from G/Festival.cpp by the integrator): the hubs', the register sheets' and the takeovers' UI
// strings, empty in Phase A. F-hubs writes the body at G1.
std::vector<std::string> FestUiInventory()
{
	return {};
}
} // namespace fest

namespace ui
{
namespace rlfest_detail
{
struct Access
{
};
} // namespace rlfest_detail

void RiverLine::FestivalHub(const Rect& /*R*/, const fest::Festival& /*F*/, double /*Now*/)
{
}

void RiverLine::FestivalOverlays(double /*Now*/)
{
	// Phase A: no festival exists (fest::All() is empty), so no moment waits (Session::PeekFestMoment) and no popover is open.
}

void RiverLine::CareerPins(const Rect& /*R*/, double /*Now*/)
{
}
} // namespace ui
} // namespace ss
