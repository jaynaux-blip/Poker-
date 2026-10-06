#include "ShortStack/Game/Talk.h"
#include "../StrictFloat.h"

// Phase A stubs (the realism update's spec, §7.1), written by core-shops and handed to people-core: every declaration
// links and returns its default (a context that knows who and where, and nothing said), so people-ue and core-ui build
// against Talk.h from day 1. people-core replaces these bodies (and TalkContext) with §6.4's talk.

namespace ss
{
namespace town
{
struct TalkContext
{
	std::string Speaker;
	std::string Venue;
};

std::shared_ptr<const TalkContext> ContextFor(const Session& /*S*/, const std::string& Speaker, const std::string& Venue)
{
	auto C = std::make_shared<TalkContext>();
	C->Speaker = Speaker;
	C->Venue = Venue;
	return C;
}

Talk Open(const TalkContext& /*C*/)
{
	return Talk();
}

Talk Answer(const TalkContext& /*C*/, const std::string& /*ChoiceId*/)
{
	return Talk();
}

std::string KeeperLine(const TalkContext& /*C*/, const std::string& /*Moment*/)
{
	return std::string();
}

std::string Bark(const TalkContext& /*C*/, const std::string& /*Trigger*/)
{
	return std::string();
}
} // namespace town
} // namespace ss
