#include "ShortStack/Game/Coach.h"
#include "../StrictFloat.h"

#include "ShortStack/AI/Grading.h"
#include "ShortStack/AI/View.h"

// Coaching (next_segment_spec §2.11). core-stake.
//
// Phase A, day 1: stubs that compile and change nothing. TiltGainNow and ReadWeightNow return the background's value,
// so the perk sites that will call them (Session.cpp:900, :1225) keep today's numbers.

namespace ss
{
namespace coach
{
Spot SpotOf(const PlayerView&, const DecisionGrade&, bool, bool)
{
	return Spot::Open;
}

void Add(Notes&, Spot, const DecisionGrade&)
{
}

Review ReviewOf(const Notes&, const std::string&)
{
	return Review();
}

double TiltGainNow(const Notes&, double Background, double)
{
	return Background;
}

int ReadWeightNow(const Notes&, int Background, double)
{
	return Background;
}
} // namespace coach
} // namespace ss
