// site_play (update 3 SPEC §7.7, suite 18): the revamped sites' loops, the Value Governor and FUSE. Every Ignite pattern and
// RunItTwice profile under its bound, no grant past the Governor's share at its instant, the Guest List's holds clearing,
// keyed draws that reproduce across reloads and watch and skip, nothing paid twice, the League, FUSE's exact pool and
// odds, the strike gate's refusals. Default run <= 30 s; "long" runs the slow modes. core-siteplay's file.
// Phase A: the suite is registered and runs empty.
#include "SitePlaySeg.h"

#include <chrono>
#include <cstdio>
#include <string>

namespace siteplay_test
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
} // namespace siteplay_test

int main(int /*Argc*/, char** /*Argv*/)
{
	const auto T0 = std::chrono::steady_clock::now();
	siteplay_test::SegSession();
	siteplay_test::SegFuse();
	const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count();
	std::printf("site play tests: %d checks, %d failed in %.1fs\n", siteplay_test::Checks, siteplay_test::Failures, Seconds);
	return siteplay_test::Failures == 0 ? 0 : 1;
}
