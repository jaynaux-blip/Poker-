#include "ShortStack/UI/TableHud.h"
#include "../StrictFloat.h"

// The table HUD, the table card and the room's clock screens (next_segment_spec §1.6, §6.3, §6.5). core-tableui.
//
// Phase A, day 1: stubs that compile and draw nothing. The card room keeps drawing its legacy HUD until this lands.

namespace ss
{
namespace ui
{
void DrawTableHud(Canvas&, const TableHudInfo&, double)
{
}

void DrawTableCard(Canvas&, const TableCardInfo&, double)
{
}

void DrawRoomClock(Canvas&, const live::ClockFacts&, int, const std::vector<std::string>&, const std::vector<life::RoomLeader>&, const std::string&, double)
{
}
} // namespace ui
} // namespace ss
