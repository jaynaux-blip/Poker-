// kast_ui (SPEC_CREATORS §9.1, §9.3, suite 24): Kast's screens under the creator update (tests/ui_seg_kast.cpp, ui-kast), apart
// from riverline_ui (64 s of its 75 s already): every surface of §5.3 (one count on every surface, "N watching · M chatting",
// Heat, Event Mode, the action slot, Hold or Crack, the peak-first end card), and the Kast pages' frame budgets (<= 1.5 ms warm,
// <= 2.0 ms during a reveal or a final table with the Broadcast; SS_UI_BENCH prints them). Budget: 60 s for the default run.
// Usage: kast_ui_test [out-dir] (with an out-dir, the pages' draw lists are written as JSON). The lead's file.
#include "UiSeg.h"

#include "ShortStack/Game/Session.h"
#include "ShortStack/UI/RiverLine.h"

#include <chrono>
#include <cstdio>

namespace ui_test
{
namespace kast_ui_smoke
{
/** Phase A-K's smoke check: Kast's Studio draws through the kit on a fresh session. */
void Smoke()
{
	QuietHooks H;
	ss::Session S(H, "kast-ui-smoke");
	S.CurrentScreen = ss::Screen::Lobby;
	ss::ui::RiverLine RL(S);
	RL.OpenApp(ss::ui::RiverLine::App::Kast, 1.0);
	RL.ShowKastPage(ss::ui::RiverLine::KastPage::Studio);
	Emit("kast_ui_smoke", RL, 2.0);
}
} // namespace kast_ui_smoke
} // namespace ui_test

int main(int Argc, char** Argv)
{
	if (Argc > 1)
	{
		ui_test::OutDir = Argv[1];
	}
	const auto T0 = std::chrono::steady_clock::now();
	ui_test::kast_ui_smoke::Smoke();
	ui_test::SegKast();
	const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count();
	if (ui_test::Failures == 0)
	{
		std::printf("kast ui tests: all passed in %.1fs\n", Seconds);
	}
	return ui_test::Failures == 0 ? 0 : 1;
}
