// world_u4 (update 4 SPEC v1.7 §3.24, §5.15, §8.1; the 26th suite): the living world at Update 4's scale. The Scene Book and its
// arrivals, Growth, the TPI, the Arc Book and the Director, the seating rule, the crowd, promotion and the scale runs, with the
// held turn's slice invariance (0.5, 2 and unlimited ms over a 30-day run that crosses a Monday the 1st) and its structural checks.
// Default run <= 45 s alone (the cap is its TIMEOUT); slow modes "long" (365-730 days x 2-3 seeds), "big3k" (the grown 3,000
// fixture) and "big" (1,100 days to the scene's 10,000, then to day 1,460) run outside ctest. The lead's file (A3's slice harness);
// each track's checks are in its own file: _scene (W-arrive), _growth (W-growth), _story (W-story), u4_session.cpp (W-table, then
// S-crowd), _crowd (S-crowd), _promote (S-promote), _scale (S-scale).
// Phase A (§6 PA-48, the lead): the suite is registered and runs empty (A3 adds the slice harness in A-2). The story-surfaces seat
// harness (§6 PA-49, u4_seat_harness.h) is included so it compiles with the suite; W-table's fidelity_exact runs it at G2.
#include "u4_seat_harness.h"

#include <chrono>
#include <cstdio>
#include <string>

namespace world_u4_test
{
int Failures = 0;
int Checks = 0;

void Expect(bool Condition, const char* What)
{
	++Checks;
	if (!Condition)
	{
		++Failures;
		std::printf("FAILED: %s\n", What);
	}
}

void Expect(bool Condition, const std::string& What)
{
	Expect(Condition, What.c_str());
}

void SegScene();   // world_u4_scene.cpp (W-arrive)
void SegGrowth();  // world_u4_growth.cpp (W-growth)
void SegStory();   // world_u4_story.cpp (W-story)
void SegSession(); // u4_session.cpp (W-table; S-crowd at G4)
void SegCrowd();   // world_u4_crowd.cpp (S-crowd)
void SegPromote(); // world_u4_promote.cpp (S-promote)
void SegScale();   // world_u4_scale.cpp (S-scale)
} // namespace world_u4_test

int main(int /*Argc*/, char** /*Argv*/)
{
	const auto T0 = std::chrono::steady_clock::now();
	world_u4_test::SegScene();
	world_u4_test::SegGrowth();
	world_u4_test::SegStory();
	world_u4_test::SegSession();
	world_u4_test::SegCrowd();
	world_u4_test::SegPromote();
	world_u4_test::SegScale();
	const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count();
	std::printf("world u4 tests: %d checks, %d failed in %.1fs\n", world_u4_test::Checks, world_u4_test::Failures, Seconds);
	return world_u4_test::Failures == 0 ? 0 : 1;
}
