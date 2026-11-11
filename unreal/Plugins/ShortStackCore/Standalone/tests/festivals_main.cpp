// festivals (update 4 SPEC v1.7 §2.15.1, §8.1; the 25th suite): Festival Season's core. The calendar to 2040 (one stage, ten clear
// days, no window over a series), the phases, the six rival festivals' plans, counts and codes, the pins' appends, Added money and
// the Main curve, the race, the crown and its tickets, the roll, the marks, News, the Pass, saves and copy (56 checks at G1), then
// G3's RivalCapsLifted and SlamSatellitesG3 (§4.14). Default run <= 30 s alone (the cap is its TIMEOUT). F-net's file (B-session's at
// G3); F-world's checks are in festivals_world.cpp.
// Phase A (§6 PA-48, the lead): the suite is registered and runs empty.
#include <chrono>
#include <cstdio>
#include <string>

namespace festivals_test
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

void SegWorld(); // festivals_world.cpp (F-world)
} // namespace festivals_test

int main(int /*Argc*/, char** /*Argv*/)
{
	const auto T0 = std::chrono::steady_clock::now();
	festivals_test::SegWorld();
	const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count();
	std::printf("festivals tests: %d checks, %d failed in %.1fs\n", festivals_test::Checks, festivals_test::Failures, Seconds);
	return festivals_test::Failures == 0 ? 0 : 1;
}
