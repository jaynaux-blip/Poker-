// sponsor_deals (next_segment_spec §7.6, suite 12): core-sponsor's tests.
// Catalog integrity; tiers per road and the conduct limits; offer cadence; exclusivity and slots; an idempotent Step;
// measurement; strikes and cures; conduct; the counter table; renewals; live money; the round trip and the Kast
// migration fixtures.
//
// Phase A, day 1: an empty main that passes (it checks only that the copy lint works). core-sponsor fills it in Phase B.
// Goldens and fixtures are read from SS_TESTS_DIR (unreal/Plugins/ShortStackCore/Tests, set by CMakeLists.txt).
#include "CopyLint.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/UI/SponsorArt.h"

#include <cstdio>

namespace sponsor_test
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
} // namespace sponsor_test

int main()
{
	sponsor_test::Failures += copylint::SelfCheck();
	if (sponsor_test::Failures == 0)
	{
		std::printf("sponsor tests: all passed\n");
	}
	return sponsor_test::Failures == 0 ? 0 : 1;
}
