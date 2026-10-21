// Update 3 (SPEC §4-§6, §7.2): Rerun 2.0's Session methods, core-videocontent's file: the ideas from the player's own career
// (the IdeaCtx the Session builds), Studio's title and thumbnail options, repackaging, the ad load, the launch actions and
// their TimeSkip ("video-act:"), and one video's analytics. Phase A, day 1: stubs (nothing offered, every lever refused).
#include "ShortStack/Game/Session.h"

#include "ShortStack/Game/VideoContent.h"
#include "ShortStack/Game/VideoStats.h"

#include <string>
#include <vector>

namespace ss
{
std::vector<video::IdeaCard> Session::VideoIdeas() const
{
	return {};
}

std::string Session::ChooseIdea(int /*IdeaId*/)
{
	return "Not yet.";
}

std::vector<video::TitleOption> Session::TitleOptions(const video::Choice& /*C*/, int /*Shuffle*/) const
{
	return {};
}

std::vector<video::ThumbSpec> Session::ThumbOptions(const video::Choice& /*C*/, int /*Shuffle*/) const
{
	return {};
}

std::string Session::Repackage(const std::string& /*VideoId*/, const video::Repack& /*R*/)
{
	return "Not yet.";
}

std::string Session::SetAdLoad(const std::string& /*VideoId*/, video::AdLoad /*L*/)
{
	return "Not yet.";
}

std::string Session::LaunchAction(const std::string& /*VideoId*/, video::Launch /*A*/)
{
	return "Not yet.";
}

std::string Session::SettleVideoAct(const std::string& /*ActivityId*/)
{
	return "Not yet.";
}

video::VideoStats Session::VideoStatsOf(const std::string& /*VideoId*/) const
{
	return video::VideoStats();
}
} // namespace ss
