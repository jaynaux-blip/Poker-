// night_one_session: core-stake's additions (next_segment_spec §7.6, §8.3), compiled into session_test and called
// from main() in session_main.cpp (the shared kit is SessionSeg.h): Lou's stake through GoToLive shows the exact three
// lines; a cancelled event returns Lou's money; online buy-ins route to the roll; ResumeLive keeps the funding.
//
// Phase A, day 1: the track's own file, registered and empty. core-stake fills it in Phase B.
#include "SessionSeg.h"

#include "ShortStack/Game/Coach.h"
#include "ShortStack/Game/Stake.h"

namespace session_test
{
void SegStake()
{
}
} // namespace session_test
