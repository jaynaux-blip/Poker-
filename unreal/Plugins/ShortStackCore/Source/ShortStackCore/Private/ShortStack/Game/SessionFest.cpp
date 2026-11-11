// Update 4, the session's side of Festival Season (SPEC v1.7 §2.8.5, §2.9): the champion's, the belt's, a mark's and a title's
// moments, FestStep's texts, letters, crown tickets and refunds, Dee's lines and Rerun's titles (fest::TextInventory) and ss.Fest.*.
// F-world's file from G1.
//
// Phase A (§6 PA-37; the lead): stubs that keep today's behaviour. No festival exists (fest::All() is empty), so no moment waits,
// FestStep (CheckCalendar's, after SitePlayStep) has nothing to send and ss.Fest.* answers that it isn't built yet.
#include "ShortStack/Game/Festival.h"
#include "ShortStack/Game/Session.h"

#include <optional>
#include <string>

namespace ss
{
std::optional<FestMoment> Session::PeekFestMoment() const
{
	return std::nullopt;
}

void Session::DoneFestMoment()
{
}

void Session::FestStep(double /*From*/, double /*To*/)
{
}

std::string Session::DebugFest(const std::string& /*Args*/)
{
	return "Not built yet: Festival Season comes with Update 4's G1.";
}
} // namespace ss
