// Update 3 (SPEC §6.1, §7.3): Rerun 2.0's thumbnail recipes, core-videocontent's file: 36 archetypes over the layouts,
// expressions, poses, captions and palettes, dealt in hash order with a fit that depends on the moment; captions only
// from facts (U7). Phase A, day 1: stubs (no recipes; every archetype maps to Update 2's Reaction frame).
#include "ShortStack/Game/VideoContent.h"

#include <vector>

namespace ss
{
namespace video
{
std::vector<ThumbSpec> ThumbOptions(const Choice& /*Ch*/, const std::vector<Candidate>& /*Cands*/, const Channel& /*C*/, int /*Shuffle*/, int /*Count*/)
{
	return {};
}

double ArchetypeFit(int /*Arch*/, int /*KindGroup*/, const Candidate& /*Lead*/)
{
	return 1.0;
}

Frame FrameOf(int /*Arch*/)
{
	return Frame::Reaction;
}

double TraitMean(int /*KindGroup*/)
{
	return 1.0;
}

double ThumbTraits(const ThumbSpec& /*S*/, const Candidate& /*Lead*/, const Channel& /*C*/)
{
	return 1.0;
}

uint8_t ThumbPromise(const ThumbSpec& /*S*/)
{
	return 0;
}

uint64_t ThumbSignature(const ThumbSpec& /*S*/)
{
	return 0;
}
} // namespace video
} // namespace ss
