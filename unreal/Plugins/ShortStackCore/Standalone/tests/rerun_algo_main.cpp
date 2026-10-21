// rerun_algo (update 3 SPEC §4.8, §7.7, suite 19): Rerun 2.0's model and money. The invariants I1-I9 over Update 2's choice
// sets, UploadU2 against Update 2's Upload, the CPM identity, the season's yearly mean, Update 2's RPMs to the cent, the
// analytics' consistency, the ad-load lock, burnout from extra editing only, the verdicts and check-ins, the Duel, trends
// from the world's log, the archetype bands against their Update 2 twins, the save budgets. Default run <= 30 s; "--write"
// writes video_vectors_u3.txt. core-videoalgo's file. Phase A: the suite is registered and runs empty.
#include <chrono>
#include <cstdio>

namespace rerun_algo_test
{
int Failures = 0;
int Checks = 0;
} // namespace rerun_algo_test

int main(int /*Argc*/, char** /*Argv*/)
{
	const auto T0 = std::chrono::steady_clock::now();
	const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count();
	std::printf("rerun algo tests: %d checks, %d failed in %.1fs\n", rerun_algo_test::Checks, rerun_algo_test::Failures, Seconds);
	return rerun_algo_test::Failures == 0 ? 0 : 1;
}
