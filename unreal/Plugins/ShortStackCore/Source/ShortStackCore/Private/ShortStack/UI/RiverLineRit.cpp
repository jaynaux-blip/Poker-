// Update 3 (SPEC §3.2-§3.5, §7.2; v1.2's red and black, §13): RunItTwice, the Exchange, ui-rit's file: the client frame (the
// red-lit carbon rail of numbered plates, the red tape, the header with its section slab), the Board, the Prospectus, Sunday Live, RANKINGS, the Desk, SERIES, the Newsdesk, the Club, the
// Cashier, the Settlement, the Member File and the octagon table's layer. Phase A: stubs. ClientFrame routes every
// RunItTwice screen but Boot here, and until this frame draws its own, RunItTwice keeps today's look through RiverLine's
// frame (HomeFrame, in RunItTwice's skin); the table layer draws nothing.
#include "ShortStack/UI/RiverLine.h"
#include "RiverLineShared.h"

namespace ss
{
namespace ui
{
void RiverLine::RitFrame(double Now, bool Preview)
{
	HomeFrame(Now, Preview);
}

void RiverLine::RitTableLayer(double /*Now*/)
{
}
} // namespace ui
} // namespace ss
