// rivals_ui (update 3 SPEC §7.7, suite 21): the revamped rival sites' screens, Ignite's (tests/ui_seg_ignite.cpp, ui-ignite)
// and RunItTwice's (tests/ui_seg_rit.cpp, ui-rit), each track's checks in its own file. Budget: 60 s for the default run.
// Usage: rivals_ui_test [out-dir] (with an out-dir, the pages' draw lists are written as JSON). The lead's file.
#include "UiSeg.h"

#include <chrono>
#include <cstdio>

int main(int Argc, char** Argv)
{
	if (Argc > 1)
	{
		ui_test::OutDir = Argv[1];
	}
	const auto T0 = std::chrono::steady_clock::now();
	ui_test::SegSeams(); // Phase A's seams first: the tracks' pages draw through them
	ui_test::SegIgnite();
	ui_test::SegRit();
	const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count();
	if (ui_test::Failures == 0)
	{
		std::printf("rivals ui tests: all passed in %.1fs\n", Seconds);
	}
	return ui_test::Failures == 0 ? 0 : 1;
}
