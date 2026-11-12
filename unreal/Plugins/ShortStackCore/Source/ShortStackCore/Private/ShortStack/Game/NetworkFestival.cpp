// Update 4, Festival Season (SPEC v1.7 §2.3, §2.4.5): the rival festivals' templates and the record field's day maxima, Network's
// side of fest::. F-net's file from G1: the plans (const NetSitesFestival IgniteFestivals[], RunItTwiceFestivals[]; FEIGN, PUSH and
// PENNANT on Ignite, WRIT, GRIT and REIGN on RunItTwice, each appended last so every sibling template keeps its index), their
// numbered events, headliners and satellites, Added and the Main curve; DayMax's non-caching walk for fest::RecordOf.
//
// Phase A (§6 PA-4, the lead): both members are declared and called, and do nothing. BuildFestivals runs last in BuildSite and
// appends no template or series (every rival keeps its 783 and 1,521 templates); FillDayMax runs first in Prewarm and leaves DayMax
// empty (nothing reads it before G1). H0 (the lead's G1 pre-wave commit) adds DayMaxOn, fest::RecordOf's public read of DayMax,
// answering 0 until F-net's body (FN-7).
#include "ShortStack/Game/Network.h"

namespace ss
{
namespace net
{
void Network::BuildFestivals()
{
}

void Network::FillDayMax(double /*Now*/) const
{
}

int Network::DayMaxOn(int /*Day*/) const
{
	// H0 (PLAN_FNET P1): 0, as if FillDayMax had walked nothing. Nothing calls it before F-net's fest::RecordOf, which lands with
	// this body (DayMax's entry for Day) at FN-7.
	return 0;
}
} // namespace net
} // namespace ss
