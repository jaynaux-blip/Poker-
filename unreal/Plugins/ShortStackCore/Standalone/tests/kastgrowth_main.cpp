// kast_growth (SPEC_CREATORS §9.1, suite 23): Kast's growth model and the Room. The pure functions (Unit's independence, the
// tables, Heat's closed form and split invariance, the night's gain and its caps, the forecast's arithmetic, tokens, the rings,
// the grant mask, the pending payout and the sponsor read rule, the save tails), then the Room and the careers on AdvanceTo's
// minute grid (kastgrowth_session.cpp) and the chat games (kastgrowth_play.cpp). Default run <= 60 s; "long" runs every
// archetype at 12 seeds and the levers (the gates' run). core-kastmath's file (main() and SegPure).
// Phase A-K: the suite is registered and runs empty but for one smoke check.
#include "KastGrowthSeg.h"

#include "ShortStack/Game/KastGrowth.h"

#include <chrono>
#include <cstdio>
#include <string>

namespace kastgrowth_test
{
int Failures = 0;
int Checks = 0;
bool Long = false;

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

void SegPure()
{
	// Phase A-K's smoke check: the suite links the core's growth model and a keyed draw is a uniform in [0, 1).
	const double U = ss::kast::growth::Unit(1, 1, ss::kast::growth::Tag::Slot, 2, 3);
	Expect(U >= 0.0 && U < 1.0, "kast growth: a keyed draw is a uniform in [0, 1)");
}
} // namespace kastgrowth_test

int main(int Argc, char** Argv)
{
	kastgrowth_test::Long = Argc > 1 && std::string(Argv[1]) == "long";
	const auto T0 = std::chrono::steady_clock::now();
	kastgrowth_test::SegPure();
	kastgrowth_test::SegSession();
	kastgrowth_test::SegPlay();
	const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count();
	std::printf("kast growth tests: %d checks, %d failed in %.1fs\n", kastgrowth_test::Checks, kastgrowth_test::Failures, Seconds);
	return kastgrowth_test::Failures == 0 ? 0 : 1;
}
