// rerun_ui (update 3 SPEC §7.7, suite 22): Rerun 2.0's screens (tests/ui_seg_video3.cpp, ui-rerun) and its thumbnails
// (tests/ui_seg_thumbs.cpp, ui-thumbs, with video_main.cpp's Update 2 thumbnail draws moved there at Phase A), each track's
// checks in its own file. Budget: 60 s for the default run. Usage: rerun_ui_test [out-dir] (with an out-dir, the pages'
// draw lists are written as JSON). The lead's file.
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
	ui_test::SegCanvas(); // Phase A's seams first (the pools set their own budgets and put the defaults back)
	ui_test::SegVideo3();
	ui_test::SegThumbs();
	const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count();
	if (ui_test::Failures == 0)
	{
		std::printf("rerun ui tests: all passed in %.1fs\n", Seconds);
	}
	return ui_test::Failures == 0 ? 0 : 1;
}
