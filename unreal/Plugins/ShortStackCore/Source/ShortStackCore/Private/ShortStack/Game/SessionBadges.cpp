#include "ShortStack/Game/Session.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Badges.h"

// The session's side of pro badges (next_segment_spec §5, §7.2): the player's badges, the ones not shown yet, progress,
// the calendar's BadgeStep (the monthly review; also once on load, §7.5) and the debug command. core-badges.
//
// Phase A, days 2-3: stubs that keep today's behavior: the player holds no badge, nothing is unseen or in progress,
// and the review does nothing.

namespace ss
{
const badge::Set& Session::HeroBadges() const
{
	return HeroSet;
}

std::vector<badge::Badge> Session::Unseen()
{
	return {};
}

std::vector<badge::Progress> Session::BadgeProgress(int Max) const
{
	return badge::Next(*this, Max);
}

void Session::BadgeStep(double /*From*/, double /*To*/)
{
}

std::string Session::DebugBadge(const std::string& /*Args*/)
{
	return "ss.Badge: no badges yet.";
}
} // namespace ss
