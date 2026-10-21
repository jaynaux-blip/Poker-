// Update 3 (SPEC §5, §7.3): Rerun 2.0's analytics, core-videoalgo's file: every number the Analytics pages draw, the
// model's own, derived from the saved fields and the rings (an Update 2 video's from Fate = Fnv1a(Id), marked Estimated).
// Phase A, day 1: empty stubs.
#include "ShortStack/Game/VideoStats.h"

#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/World.h"

#include <vector>

namespace ss
{
namespace video
{
VideoStats StatsOf(const Channel& /*C*/, const Video& /*V*/, double /*World*/)
{
	return VideoStats();
}

Range StatsRange(const Channel& /*C*/, double /*World*/, int /*Days*/)
{
	return Range();
}

AudienceView AudienceOf(const Channel& /*C*/, double /*World*/)
{
	return AudienceView();
}

RealtimeView Realtime(const Channel& /*C*/, double /*World*/)
{
	return RealtimeView();
}

Health HealthOf(const Channel& /*C*/, double /*World*/)
{
	return Health();
}

std::vector<Trend> Trends(const life::State& /*L*/, const world::World& /*W*/, double /*World*/)
{
	return {};
}
} // namespace video
} // namespace ss
