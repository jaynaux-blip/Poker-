// u4_ui (update 4 SPEC v1.7 §2.15.2, §3.24, §4.19.2, §5.15, §8.1; the 28th suite): Update 4's screens, apart from riverline_ui and
// rivals_ui (both near their caps already). The festival half (the hub, the moments, the looks: u4_ui_fest.cpp F-hubs,
// u4_ui_look.cpp F-look), the world's pages (u4_ui_world.cpp W-ui: copy_lint_world, the Notebook, the STORY tab, the pills), the big
// field's slice (u4_ui_bigfield.cpp B-ui) and the crowd's pages (u4_ui_crowd.cpp S-ui). Default run <= 60 s alone (the cap is its
// TIMEOUT): the festival half <= 30 s, the big-field slice <= 15 s, the world and crowd pages <= 15 s (§12.1 I-17). This file is the
// kit (the lead's): the counters and Expect here, the page dump and budget helpers as the gates need them.
// Phase A (§6 PA-48, the lead): the suite is registered and runs empty.
#include <chrono>
#include <cstdio>
#include <string>

namespace u4_ui_test
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

void SegFest();     // u4_ui_fest.cpp (F-hubs)
void SegLook();     // u4_ui_look.cpp (F-look)
void SegWorld();    // u4_ui_world.cpp (W-ui)
void SegBigField(); // u4_ui_bigfield.cpp (B-ui)
void SegCrowd();    // u4_ui_crowd.cpp (S-ui)
} // namespace u4_ui_test

int main(int /*Argc*/, char** /*Argv*/)
{
	const auto T0 = std::chrono::steady_clock::now();
	u4_ui_test::SegFest();
	u4_ui_test::SegLook();
	u4_ui_test::SegWorld();
	u4_ui_test::SegBigField();
	u4_ui_test::SegCrowd();
	const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count();
	std::printf("u4 ui tests: %d checks, %d failed in %.1fs\n", u4_ui_test::Checks, u4_ui_test::Failures, Seconds);
	return u4_ui_test::Failures == 0 ? 0 : 1;
}
