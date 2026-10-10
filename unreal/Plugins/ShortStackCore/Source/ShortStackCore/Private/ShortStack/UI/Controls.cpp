#include "ShortStack/UI/TableHud.h"
#include "../StrictFloat.h"

// The one control map (next_segment_spec §6.4; declared in TableHud.h). core-tableui.
//
// Phase A, day 1: a stub that compiles: every context's map is empty, and the HUDs keep their own glyphs until this
// lands.

namespace ss
{
namespace ui
{
const std::vector<Control>& Controls(ControlContext)
{
	static const std::vector<Control> Empty{};
	return Empty;
}
} // namespace ui
} // namespace ss
