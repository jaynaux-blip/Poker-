// riverline_ui: ui-laptop's autoplay additions (update 2 SPEC §2.11, §6.7): auto_panel, auto_panel_greyed, auto_watch_strip
// (no move shown before it's played), table_skip (replacing table_sprint), auto_tile_status, results_auto,
// career_auto_row. Compiled into ui_test and called from main() in ui_main.cpp (the kit is UiSeg.h). Phase A, day 1 (the
// lead): the table_sprint draw list, moved here from ui_main.cpp's Screens; ui-laptop fills the rest, its helpers in
// ui_test::auto_seg.
#include "UiSeg.h"

namespace ui_test
{
namespace auto_seg
{
/** A table at a postflop decision, as Screens reaches it (Full pace, calling to the flop). */
double AutoSegToFlop(ss::Session& S, double Now)
{
	S.CurrentScreen = ss::Screen::Lobby;
	S.Register(0);
	S.CurrentPace = ss::Pace::Full;
	for (int Guard = 0; ++Guard < 400000 && S.CurrentScreen == ss::Screen::Table;)
	{
		Now = Step(S, Now, 1.0 / 30.0);
		if (S.HasPrompt && S.Board.size() >= 3 && Now - S.Prompt.OpenedAt > 1.2)
		{
			break;
		}
		if (S.HasPrompt && S.Board.size() < 3 && Now - S.Prompt.OpenedAt > 0.3)
		{
			S.HeroAct(S.Prompt.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Call());
		}
	}
	return Now;
}
} // namespace auto_seg

void SegAuto()
{
	// The skip card (Sprint's today; table_skip when ui-laptop's card lands).
	QuietHooks H;
	ss::Session S(H, "ui-shots");
	ss::ui::RiverLine RL(S);
	const double Now = auto_seg::AutoSegToFlop(S, 0.0);
	Expect(S.CurrentScreen == ss::Screen::Table, "autoplay: a table to show the skip card on");
	RL.UI.Ptr.Active = false;
	S.Tab = ss::RightTab::Payouts;
	S.Sprinting = true;
	Emit("table_sprint", RL, Now + 1.5);
	S.Sprinting = false;
}
} // namespace ui_test
