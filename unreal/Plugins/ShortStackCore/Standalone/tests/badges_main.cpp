// pro_badges (next_segment_spec §7.6, suite 14): core-badges's tests.
// The edges (club, Verified, the card, the POY reign, the Hall); the priority golden for 3 scopes x 2 frames;
// privacy; determinism over 400 days; distribution over 3 seeds x 2 years; migration from world v1; farming.
//
// Phase A, day 1: an empty main that passes (it checks only that the copy lint works). core-badges fills it in Phase B.
// Goldens and fixtures are read from SS_TESTS_DIR (unreal/Plugins/ShortStackCore/Tests, set by CMakeLists.txt).
#include "CopyLint.h"
#include "ShortStack/Game/Badges.h"
#include "ShortStack/UI/BadgeArt.h"

#include <cstdio>

namespace badges_test
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
} // namespace badges_test

int main()
{
	badges_test::Failures += copylint::SelfCheck();
	if (badges_test::Failures == 0)
	{
		std::printf("badges tests: all passed\n");
	}
	return badges_test::Failures == 0 ? 0 : 1;
}
