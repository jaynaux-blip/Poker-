#include "ShortStack/Game/Live.h"
#include "../StrictFloat.h"

// The clock facts and the payout ladder (next_segment_spec §6.2 C; declared in Live.h). core-tableui.
//
// Phase A, day 1: stubs that compile and change nothing (nothing reads them yet): the timers the host passes in, and
// no ladder.

namespace ss
{
namespace live
{
ClockFacts Clock(const Tournament&, const Occurrence&, const life::LiveEntry*, double, double LevelSecondsLeft, bool OnBreak, double BreakSecondsLeft)
{
	ClockFacts F;
	F.LevelSecondsLeft = LevelSecondsLeft;
	F.OnBreak = OnBreak;
	F.BreakSecondsLeft = BreakSecondsLeft;
	return F;
}

std::vector<PayRow> PayLadder(const Tournament&)
{
	return {};
}
} // namespace live
} // namespace ss
