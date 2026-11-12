// u4_ui (update 4 SPEC v1.7 §2.15.2, §3.24, §4.19.2, §5.15, §8.1; the 28th suite): Update 4's screens, apart from riverline_ui and
// rivals_ui (both near their caps already). The festival half (the hub, the moments, the looks: u4_ui_fest.cpp F-hubs,
// u4_ui_look.cpp F-look), the world's pages (u4_ui_world.cpp W-ui: copy_lint_world, the Notebook, the STORY tab, the pills), the big
// field's slice (u4_ui_bigfield.cpp B-ui) and the crowd's pages (u4_ui_crowd.cpp S-ui). Default run <= 60 s alone (the cap is its
// TIMEOUT): the festival half <= 30 s, the big-field slice <= 15 s, the world and crowd pages <= 15 s (§12.1 I-17). This file is the
// kit (the lead's): the counters and Expect here, the page dump and budget helpers as the gates need them.
// Phase A (§6 PA-48, the lead): the suite is registered and runs empty.
// G1 (the lead's pre-wave commit H0; LEAD_CALLS R-1, R-2; PLAN_G1 §4.2): the UI kit joins (tests/UiSeg.h and ui_seg_kit.cpp: the
// draw-list readers TextAt, Drew and DrewRun, WriteList, Emit, Step, TableMeasurer, QuietHooks), so the tracks share one set of
// page helpers. Its three counters are defined here (ui_seg_kit.cpp leaves them out under SS_UI_KIT_MAIN_COUNTERS): ui_test::OutDir
// from SS_U4_UI_OUT (pages are written there as <name>.json; nothing without it), ui_test::Expect forwarding to Expect below so every
// check counts once, and ui_test::Failures for a helper that counts by hand; main fails on either counter. F-hubs runs as two
// streams (Option B, R-1): the hub in u4_ui_fest.cpp (A), the host pages in u4_ui_fest_hosts.cpp (B).
#include "UiSeg.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
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

void SegFest();      // u4_ui_fest.cpp (F-hubs, stream A)
void SegFestHosts(); // u4_ui_fest_hosts.cpp (F-hubs, stream B)
void SegLook();      // u4_ui_look.cpp (F-look)
void SegWorld();     // u4_ui_world.cpp (W-ui)
void SegBigField();  // u4_ui_bigfield.cpp (B-ui)
void SegCrowd();     // u4_ui_crowd.cpp (S-ui)
} // namespace u4_ui_test

namespace ui_test
{
int Failures = 0;
std::string OutDir;

void Expect(bool Condition, const char* What)
{
	u4_ui_test::Expect(Condition, What);
}
} // namespace ui_test

int main(int /*Argc*/, char** /*Argv*/)
{
	if (const char* Out = std::getenv("SS_U4_UI_OUT"))
	{
		ui_test::OutDir = Out;
	}
	const auto T0 = std::chrono::steady_clock::now();
	u4_ui_test::SegFest();
	u4_ui_test::SegFestHosts();
	u4_ui_test::SegLook();
	u4_ui_test::SegWorld();
	u4_ui_test::SegBigField();
	u4_ui_test::SegCrowd();
	const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count();
	std::printf("u4 ui tests: %d checks, %d failed in %.1fs\n", u4_ui_test::Checks, u4_ui_test::Failures + ui_test::Failures, Seconds);
	return u4_ui_test::Failures == 0 && ui_test::Failures == 0 ? 0 : 1;
}
