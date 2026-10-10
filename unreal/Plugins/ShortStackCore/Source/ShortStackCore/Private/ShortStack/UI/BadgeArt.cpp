#include "ShortStack/UI/BadgeArt.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Life.h"

// The badges' art (next_segment_spec §5.5). core-badges.
//
// Phase A, day 1: stubs that compile and draw nothing.

namespace ss
{
namespace ui
{
namespace badgeart
{
void Draw(Canvas&, const badge::Badge&, float, float, float, double, bool)
{
}

float Rail(Canvas&, const badge::Set&, uint8_t, float, float, float, int, bool)
{
	return 0.0f;
}

int Atlas(Canvas&, float)
{
	return 0;
}

int AtlasCell(const badge::Badge&)
{
	return -1;
}

void ClubCard(Canvas&, const Rect&, const ClubCardInfo&, double)
{
}

void RoomLeaders(Canvas&, const Rect&, const std::vector<life::RoomLeader>&, const std::string&, const std::string&, double)
{
}
} // namespace badgeart
} // namespace ui
} // namespace ss
