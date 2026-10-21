// Update 3 (SPEC §3.4, §3.6, §7.2): RunItTwice's League page, League Night, the Callout drawer, the Watchlist, the
// Statement, the Luck Ledger panel and credentials, ui-rit's file. This file has no RiverLine member function of its own:
// it reaches the client's private state and its gatekeepers through rlrit_detail::Access (RiverLine.h's friend),
// forwarders defined here. Phase A, day 1: Access is empty.
#include "ShortStack/UI/RiverLine.h"
#include "RiverLineShared.h"

namespace ss
{
namespace ui
{
namespace rlrit_detail
{
struct Access
{
};
} // namespace rlrit_detail
} // namespace ui
} // namespace ss
