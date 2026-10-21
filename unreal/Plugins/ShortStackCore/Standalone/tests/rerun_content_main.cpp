// rerun_content (update 3 SPEC §6, §7.7, suite 20): Rerun 2.0's library. The title templates (at least 1,600, every cell
// filled, linted, every digit a fact, the stable picker), 500 generated thumbnails giving at least 300 distinct looks, the
// 147 P0 ideas unlocking from their facts, 10,000 comments through the lint, the world creators telling only the world's
// facts, the trademark screen (tests/Trademarks.h) over InventoryU3. Default run <= 20 s; "--write" pins the worked
// thumbnails, TraitMean and the worked titles. core-videocontent's file. Phase A: the suite is registered and runs empty.
#include <chrono>
#include <cstdio>

namespace rerun_content_test
{
int Failures = 0;
int Checks = 0;
} // namespace rerun_content_test

int main(int /*Argc*/, char** /*Argv*/)
{
	const auto T0 = std::chrono::steady_clock::now();
	const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count();
	std::printf("rerun content tests: %d checks, %d failed in %.1fs\n", rerun_content_test::Checks, rerun_content_test::Failures, Seconds);
	return rerun_content_test::Failures == 0 ? 0 : 1;
}
