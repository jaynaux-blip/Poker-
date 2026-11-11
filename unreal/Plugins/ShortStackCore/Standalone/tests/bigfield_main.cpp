// big_field (update 4 SPEC v1.7 §4.19, §8.1; the 27th suite): the Big Field's engine. The field model (the ten CalibId checks against
// the dealt references), the ring, ModelBalance, merged eliminations, ck2, CatchUp, Reenter, the queries and the IcmPlay branch, a
// dozen 10,000-entry runs and the 125,000 builds. Default run <= 30 s alone (the cap is its TIMEOUT); "long" (the dealt re-plays,
// EstimateBands, the 125,000 bench) runs outside ctest. B-engine's file; B-session's checks are in bigfield_session.cpp.
// Phase A (§6 PA-48, the lead): the suite is registered and runs empty.
#include <chrono>
#include <cstdio>
#include <string>

namespace big_field_test
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

void SegSession(); // bigfield_session.cpp (B-session)
} // namespace big_field_test

int main(int /*Argc*/, char** /*Argv*/)
{
	const auto T0 = std::chrono::steady_clock::now();
	big_field_test::SegSession();
	const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count();
	std::printf("big field tests: %d checks, %d failed in %.1fs\n", big_field_test::Checks, big_field_test::Failures, Seconds);
	return big_field_test::Failures == 0 ? 0 : 1;
}
