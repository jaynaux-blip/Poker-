#include "ShortStack/UI/TableHud.h"
#include "../StrictFloat.h"

// The desk and the cage (next_segment_spec §6.6; declared in TableHud.h). core-tableui.
//
// Phase A, day 1: a stub that compiles, draws nothing and takes no requests. The card room's desk keeps working as it
// does today (subtitles and E again) until this lands.

namespace ss
{
namespace ui
{
void DeskScreen::Open(double Now, const std::string& Window)
{
	Which = Window;
	OpenedAt = Now;
	Pending.clear();
}

void DeskScreen::Key(const std::string&, double)
{
}

void DeskScreen::Draw(Canvas&, const DeskInfo&, double)
{
}

std::vector<DeskRequest> DeskScreen::TakeRequests()
{
	std::vector<DeskRequest> Out;
	Out.swap(Pending);
	return Out;
}
} // namespace ui
} // namespace ss
