// Update 3 (SPEC §2.2-§2.5, §7.2): Ignite Poker, the Arcade on the Boardwalk, ui-ignite's file: the client frame (the
// marquee, the Boardwalk band, the dock), PLAY, the event sheet, BOARDS, BUZZ, WALLET, the Player Card, "Your run" and the
// capsule table's layer. Phase A: stubs. ClientFrame routes every Ignite screen but Boot here, and until this frame draws
// its own, Ignite keeps today's look through RiverLine's frame (HomeFrame, in Ignite's skin); the table layer draws nothing.
#include "ShortStack/UI/RiverLine.h"
#include "RiverLineShared.h"

namespace ss
{
namespace ui
{
void RiverLine::IgniteFrame(double Now, bool Preview)
{
	HomeFrame(Now, Preview);
}

void RiverLine::IgniteTableLayer(double /*Now*/)
{
}
} // namespace ui
} // namespace ss
