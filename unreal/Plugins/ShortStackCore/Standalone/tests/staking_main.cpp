// staking_backing (next_segment_spec §7.6, suite 11): core-stake's tests.
// Event-stake vectors (Tests/staking_vectors.txt); pieces; swaps; a scripted 40-event package; conservation to the
// cent; idempotence across a save round trip; due dates; negotiation; ROI shrinkage and ceilings; backer EV (the
// fixed-seed Monte Carlo); the save round trip and migration; coaching tallies; the exploits.
//
// Phase A, day 1: an empty main that passes (it checks only that the copy lint works). core-stake fills it in Phase B.
// Goldens and fixtures are read from SS_TESTS_DIR (unreal/Plugins/ShortStackCore/Tests, set by CMakeLists.txt).
#include "CopyLint.h"
#include "ShortStack/Game/Coach.h"
#include "ShortStack/Game/Stake.h"

#include <cstdio>

namespace stake_test
{
int Failures = 0;

void Expect(bool Condition, const char* What)
{
	if (!Condition)
	{
		++Failures;
		std::printf("FAILED: %s\n", What);
	}
}
} // namespace stake_test

int main()
{
	stake_test::Failures += copylint::SelfCheck();
	if (stake_test::Failures == 0)
	{
		std::printf("staking tests: all passed\n");
	}
	return stake_test::Failures == 0 ? 0 : 1;
}
