// Update 3 (SPEC §6.3, §7.3): Rerun 2.0's video ideas, core-videocontent's file: 175 idea types drawn from the player's own
// career, each true to its facts and locked (saying why) until the facts exist. Phase A, day 1: stubs (no ideas).
#include "ShortStack/Game/VideoContent.h"

#include <vector>

namespace ss
{
namespace video
{
std::vector<IdeaCard> Ideas(const IdeaCtx& /*X*/, const Channel& /*C*/, double /*World*/)
{
	return {};
}

bool MakeIdeaCandidate(const IdeaCtx& /*X*/, int /*IdeaId*/, double /*World*/, Candidate& /*Out*/)
{
	return false;
}
} // namespace video
} // namespace ss
