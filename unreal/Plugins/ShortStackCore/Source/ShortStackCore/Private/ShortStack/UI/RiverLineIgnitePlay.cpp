// Update 3 (SPEC §2.4, §2.6, §7.2): Ignite's REWARDS and FUSE pages, ui-ignite's file: the Prize Counter, the Flame, Dares,
// the Spark Wheel, Crackers, the Wristband, the level-up banner, the Locker and the Sticker Book, the FUSE booth, its
// reveal and the 3-max table's marquee. This file has no RiverLine member function of its own: it reaches the client's
// private state and its gatekeepers through rlignite_detail::Access (RiverLine.h's friend), forwarders defined here.
// Phase A, day 1: Access is empty.
#include "ShortStack/UI/RiverLine.h"
#include "RiverLineShared.h"

namespace ss
{
namespace ui
{
namespace rlignite_detail
{
struct Access
{
};
} // namespace rlignite_detail
} // namespace ui
} // namespace ss
