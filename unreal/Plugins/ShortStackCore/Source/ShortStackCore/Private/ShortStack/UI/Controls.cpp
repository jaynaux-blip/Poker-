#include "ShortStack/UI/TableHud.h"
#include "../StrictFloat.h"

#include <algorithm>
#include <string>
#include <vector>

// The one control map (next_segment_spec §6.4; declared in TableHud.h). core-tableui.
//
// Every HUD's glyphs read it, and the host binds by it, so a prompt never names a button that does something else.
// Seated: X fold, A check or call, Y raise on release (held past 0.35 s it fills ALL IN and goes all in at 0.95 s), B
// backs out and nothing else; the keyboard's all in is a hold of A. On foot in the card room the controls are the
// street's: the left stick moves, LB (or L3) runs, Y is the view, A uses, B pauses.
//
// Update 2 (SPEC §2.6.2): autoplay. At the table K puts the plan in charge with the style the PLAY page shows, and takes
// it back (the keyboard's: a pad turns it on and off from the PLAY page); any action key during the plan's think takes the
// wheel (the first press only takes it, the next one acts); T (right on the d-pad) steps the watch speed while the plan
// holds the seat; K or B stops a skip at the next gap between hands (backing out of it: B still does nothing else). The
// PLAY page uses only what's free while the card is held: the arrows (A on a pad) pick the style, K (A held on a pad)
// turns the plan on or off, 1 to 5 (X on a pad) pick the skip's target and Enter held (X held) skips. A control with no
// glyph for a device ("auto" has none on a pad, "autohold" none on the keyboard) isn't that device's, and no row draws it.
// "keep" (C1): in the AUTO RESUMES window (the first 3 s of the hand after a take-over) K keeps the seat yours, autoplay off.
// A pad has no K at the table and B only ever backs out, so there X, A or Y does it: an action button pressed then means
// you're playing. Like taking the wheel, that press only takes the seat; the next one acts.

namespace ss
{
namespace ui
{
const std::vector<Control>& Controls(ControlContext C)
{
	static const std::vector<Control> Street = {
		{"move", "WASD", "LS", false, "MOVE"},
		{"run", "SHIFT", "LB", false, "RUN"},
		{"view", "V", "Y", false, "1ST / 3RD PERSON"},
		{"use", "E", "A", false, "INTERACT"},
		{"eat", "F", "X", false, "EAT OR DRINK"},
		{"pause", "ESC", "B", false, "PAUSE"},
	};
	static const std::vector<Control> OnFoot = {
		{"move", "WASD", "LS", false, "MOVE"},
		{"run", "SHIFT", "LB", false, "RUN"},
		{"view", "V", "Y", false, "1ST / 3RD PERSON"},
		{"use", "E", "A", false, "INTERACT"},
		{"card", "TAB", "UP", true, "TABLE CARD"},
		{"pause", "ESC", "B", false, "PAUSE"},
	};
	static const std::vector<Control> Table = {
		{"fold", "F", "X", false, "FOLD"},
		{"call", "C", "A", false, "CHECK / CALL"},
		{"raise", "R", "Y", false, "RAISE"},
		{"allin", "A", "Y", true, "ALL IN"},
		{"presets", "1-6", "LB RB", false, "PRESETS"},
		{"steps", "WHEEL", "LB RB", true, "FINE STEPS"},
		{"next", "N", "A", false, "NEXT HAND"},
		{"peek", "SPACE", "LT", false, "LOOK AT YOUR CARDS"},
		{"study", "RMB", "RT", true, "STUDY A FACE"},
		{"breathe", "SHIFT", "L3", true, "BREATHE"},
		{"card", "TAB", "UP", true, "TABLE CARD"},
		{"stand", "Q", "LEFT", false, "STAND UP"},
		{"pace", "T", "RIGHT", false, "TABLE PACE"},
		{"leave", "L", "DOWN", true, "WALK AWAY"},
		{"view", "V", "VIEW", false, "VIEW"},
		{"shoulder", "B", "R3", false, "OTHER SHOULDER"},
		{"back", "ESC", "B", false, "BACK"},
		{"pause", "ESC", "MENU", false, "PAUSE"},
		// Update 2: autoplay at the table.
		{"auto", "K", "", false, "AUTOPLAY"},
		{"wheel", "F C R A", "X A Y", false, "TAKE THE WHEEL"},
		{"keep", "K", "X A Y", false, "KEEP PLAYING"},
		{"watch", "T", "RIGHT", false, "WATCH SPEED"},
		{"stopskip", "K", "B", false, "STOP THE SKIP"},
	};
	static const std::vector<Control> Card = {
		{"page", "Q E", "LB RB", false, "PAGE"},
		{"units", "U", "Y", false, "CHIPS / BB"},
		{"card", "TAB", "UP", true, "LET GO TO CLOSE"},
		{"back", "ESC", "B", false, "CLOSE"},
		// Update 2: the PLAY page.
		{"style", "LEFT RIGHT", "A", false, "STYLE"},
		{"auto", "K", "", false, "AUTOPLAY ON / OFF"},
		{"autohold", "", "A", true, "AUTOPLAY ON / OFF"},
		{"target", "1-5", "X", false, "SKIP TO"},
		{"skip", "ENTER", "X", true, "SKIP"},
	};
	switch (C)
	{
	case ControlContext::Street: return Street;
	case ControlContext::CardRoomOnFoot: return OnFoot;
	case ControlContext::Table: return Table;
	case ControlContext::TableCard: return Card;
	}
	return Street;
}

const Control* FindControl(ControlContext C, const std::string& Id)
{
	for (const Control& K : Controls(C))
	{
		if (Id == K.Id)
		{
			return &K;
		}
	}
	return nullptr;
}

std::string ControlsNotice(bool Gamepad)
{
	return Gamepad ? "All in is Y held now. B backs out." : "All in is A held now. Esc backs out.";
}

// ------------------------------------------------------------------ tap and hold

void HoldButton::Press(double Now)
{
	DownAt = Now;
	Fired = false;
}

HoldButton::Out HoldButton::Tick(double Now)
{
	if (DownAt < 0.0 || Fired)
	{
		return Out::None;
	}
	const double Delay = CanTap ? TapSeconds : 0.0;
	if (Now - DownAt >= Delay + FillSeconds)
	{
		Fired = true;
		return Out::Hold;
	}
	return Out::None;
}

HoldButton::Out HoldButton::Release(double Now)
{
	if (DownAt < 0.0)
	{
		return Out::None;
	}
	const double Held = Now - DownAt;
	const double Delay = CanTap ? TapSeconds : 0.0;
	Out Result = Out::None;
	if (!Fired)
	{
		if (CanTap && Held < TapSeconds)
		{
			Result = Out::Tap;
		}
		else if (Held >= Delay + FillSeconds)
		{
			Result = Out::Hold; // full on the frame it was let go (no Tick saw it)
		}
	}
	DownAt = -1.0;
	Fired = false;
	return Result;
}

float HoldButton::Progress(double Now) const
{
	if (DownAt < 0.0)
	{
		return 0.0f;
	}
	if (Fired)
	{
		return 1.0f;
	}
	const double Delay = CanTap ? TapSeconds : 0.0;
	return static_cast<float>(std::clamp((Now - DownAt - Delay) / std::max(0.01, FillSeconds), 0.0, 1.0));
}

bool HoldButton::Cancelling(double Now) const
{
	if (DownAt < 0.0 || Fired)
	{
		return false;
	}
	const double Held = Now - DownAt;
	const double Delay = CanTap ? TapSeconds : 0.0;
	return Held >= Delay && Held < Delay + FillSeconds;
}
} // namespace ui
} // namespace ss
