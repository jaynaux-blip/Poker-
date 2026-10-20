// RiverLine's autoplay (update 2 SPEC §2.6.1, §2.11): the panel over a table ("AI AUTOPLAY": the style tiles, Sharpness
// and Form, the watch speeds, the skip's target and the stop rules, "Make this my default"), the watch strip under the
// felt (the style, the think bar that never names a move before it's played, the plan's last action, the speeds and
// Take over [K]), the skip card over the dimmed table, and the keys (K, Backspace, an action key during the plan's
// think). Everything the session knows comes from Session::AutoPanel (core-autoplay's Panel); every change goes through
// SetAutoplay, SkipAhead, StopSkip and TakeOver. The words "AI autoplay" are system UI here (U1): nothing in the world
// ever says them. Side state lives beside the class, by laptop, as Mail's does. ui-laptop's file.
#include "ShortStack/UI/RiverLine.h"
#include "../StrictFloat.h"
#include "RiverLineShared.h"

#include "ShortStack/AI/Style.h"
#include "ShortStack/Game/Autoplay.h"
#include "ShortStack/Game/Format.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <vector>

namespace ss
{
namespace ui
{
namespace rlauto_detail
{
/** One laptop's autoplay panel: open over a table (by index, and its event so a closed table never takes the panel). */
struct AutoUi
{
	bool Open = false;
	int Table = -1;
	std::string EventId;
	double OpenedAt = -100.0;
	bool SkipPicked = false; // autoplay off, SKIP picked on the speed row: the panel's button starts a skip
	std::string Note;        // the session's last answer ("" when it said yes), or a confirmation
	bool NoteBad = false;
	double NoteAt = -100.0;
	std::map<std::string, std::pair<std::string, double>> Stops; // by event id: the skip's stop reason and when it was first seen
};

std::map<const RiverLine*, AutoUi>& AutoUiStates()
{
	static std::map<const RiverLine*, AutoUi> ByLaptop;
	return ByLaptop;
}

AutoUi& AutoUiOf(const RiverLine* RL)
{
	return AutoUiStates()[RL];
}

const float AutoTileW = 252.0f;
const float AutoTileH = 130.0f;
const float AutoTileGap = 12.0f;
const float AutoEdgeSize = 12.0f;
const float AutoEdgeLine = 15.0f;
const int AutoEdgeLines = 4;

Color AutoSwingColor(style::Swing V)
{
	switch (V)
	{
	case style::Swing::Steady: return pal::Green;
	case style::Swing::Even: return pal::Accent2;
	case style::Swing::Swingy: return pal::Orange;
	case style::Swing::Wild: return pal::Red;
	}
	return pal::Muted;
}

/**
 * The styles a panel shows: the whole catalog, eleven tiles (§2.6.1, AP1). One that can't be picked here is greyed with
 * the Panel's reason: Bounty Hunter outside knockout events, and a style the ship rule holds back (§2.5; Bounty Hunter
 * until its module lands, P1) with core-autoplay's "Not on the menu".
 */
std::vector<int> AutoShownStyles(const autoplay::Panel& P)
{
	std::vector<int> Out;
	for (size_t I = 0; I < P.Styles.size(); ++I)
	{
		Out.push_back(static_cast<int>(I));
	}
	return Out;
}

/** The 4-3-4 grid (§2.6.1): how many tiles each row holds, for N tiles shown (the last row takes what's left). */
std::vector<int> AutoRows(int N)
{
	std::vector<int> Rows;
	const int Pattern[3] = {4, 3, 4};
	for (int K = 0; N > 0; ++K)
	{
		const int Take = std::min(N, Pattern[K % 3]);
		Rows.push_back(Take);
		N -= Take;
	}
	return Rows;
}

/** Where tile K of the grid sits inside the panel R (Top: the grid's top). */
Rect AutoTileRect(const Rect& R, float Top, const std::vector<int>& Rows, int K)
{
	int Row = 0;
	while (Row + 1 < static_cast<int>(Rows.size()) && K >= Rows[static_cast<size_t>(Row)])
	{
		K -= Rows[static_cast<size_t>(Row)];
		++Row;
	}
	const int In = Rows[static_cast<size_t>(Row)];
	const float RowW = static_cast<float>(In) * AutoTileW + static_cast<float>(In - 1) * AutoTileGap;
	const float X0 = R.X + (R.W - RowW) / 2.0f;
	return {X0 + static_cast<float>(K) * (AutoTileW + AutoTileGap), Top + static_cast<float>(Row) * (AutoTileH + AutoTileGap), AutoTileW, AutoTileH};
}

/** NetSpaced's width for Str, without drawing it. */
float AutoSpacedW(Canvas& Cv, const std::string& Str, float Size, int Weight, float Track)
{
	float Total = 0.0f;
	int Glyphs = 0;
	for (size_t I = 0; I < Str.size();)
	{
		const unsigned char B = static_cast<unsigned char>(Str[I]);
		const size_t Len = B >= 0xF0 ? 4 : B >= 0xE0 ? 3 : B >= 0xC0 ? 2 : 1;
		Total += Cv.Measure(Str.substr(I, Len), Size, Weight) + Track;
		++Glyphs;
		I += Len;
	}
	return Glyphs > 0 ? Total - Track : 0.0f;
}

/** Text split by words: as much as fits in FirstW, then the rest ("" when it all fits). */
std::pair<std::string, std::string> AutoSplitAt(Canvas& Cv, const std::string& Text, float FirstW, float Size, int Weight)
{
	if (Cv.Measure(Text, Size, Weight) <= FirstW)
	{
		return {Text, std::string()};
	}
	size_t Cut = 0;
	for (size_t Space = Text.find(' '); Space != std::string::npos; Space = Text.find(' ', Space + 1))
	{
		if (Cv.Measure(Text.substr(0, Space), Size, Weight) > FirstW)
		{
			break;
		}
		Cut = Space;
	}
	if (Cut == 0)
	{
		return {Text, std::string()};
	}
	return {Text.substr(0, Cut), Text.substr(Cut + 1)};
}

/** The skip's target as the panel's main button says it: "Skip to the bubble", "Skip 50 hands". */
std::string AutoSkipButton(autoplay::Target To, bool Breaks)
{
	switch (To)
	{
	case autoplay::Target::NextBreak: return Breaks ? "Skip to the next break" : "Skip to the next level";
	case autoplay::Target::Bubble: return "Skip to the bubble";
	case autoplay::Target::FinalTable: return "Skip to the final table";
	case autoplay::Target::Hands: return "Skip 50 hands";
	default: return "Skip to the result";
	}
}

/** "SKIPPING · PUSH/FOLD · hand 214 · level 11 · 31 to the bubble" in two: the head (the first two parts) and the rest. */
std::pair<std::string, std::string> AutoSplitSkipLine(const std::string& Line)
{
	const std::string Dot = " \xC2\xB7 ";
	const size_t First = Line.find(Dot);
	const size_t Second = First == std::string::npos ? std::string::npos : Line.find(Dot, First + Dot.size());
	if (Second == std::string::npos)
	{
		return {Line, std::string()};
	}
	return {Line.substr(0, Second), Line.substr(Second + Dot.size())};
}

/** A toggle chip (a box with a check, then its label): returns true when clicked. */
bool AutoToggle(Ui& UI, Canvas& Cv, const std::string& Id, const Rect& R, const std::string& Label, bool On, bool Enabled)
{
	const Ui::ClickState St = UI.Clickable(Id, R, Enabled);
	const float A0 = Cv.GetAlpha();
	Cv.SetAlpha(A0 * (Enabled ? 1.0f : 0.45f));
	UI.RRect(R, 8.0f, On ? rlnet_detail::NetA(pal::Accent, 0.14f) : St.Hover ? rlsites_detail::SitesTone(0x172a42) : rlsites_detail::SitesTone(0x0e1a2c), On ? rlnet_detail::NetA(pal::Accent, 0.7f) : pal::Line);
	const Rect Box{R.X + 10.0f, R.Y + R.H / 2.0f - 8.0f, 16.0f, 16.0f};
	UI.RRect(Box, 4.0f, On ? pal::Accent : rlsites_detail::SitesTone(0x0b1422), On ? pal::Accent : pal::Muted);
	if (On)
	{
		rlnet_detail::NetCheck(Cv, Box.X + 8.0f, Box.Y + 8.0f, 10.0f, rlsites_detail::SitesOnAccent());
	}
	UI.Text(Label, R.X + 34.0f, R.Y + R.H / 2.0f + 0.5f, Ts(13.0f, 700, On ? pal::Ink : rlsites_detail::SitesTone(0xc3cedf), Align::Left, Baseline::Middle, false, R.W - 42.0f));
	Cv.SetAlpha(A0);
	return St.Clicked && Enabled;
}

/** A segment of a choice row (a speed, a target): lit when chosen. Returns true when clicked. */
bool AutoSegment(Ui& UI, Canvas& Cv, const std::string& Id, const Rect& R, const std::string& Label, bool Lit, bool Enabled)
{
	const Ui::ClickState St = UI.Clickable(Id, R, Enabled);
	const float A0 = Cv.GetAlpha();
	Cv.SetAlpha(A0 * (Enabled ? 1.0f : 0.4f));
	UI.RRect(R, 8.0f, Lit ? rlsites_detail::SitesTone(0x1f4d63) : St.Hover && Enabled ? rlsites_detail::SitesTone(0x172a42) : pal::Panel, Lit ? pal::Accent : pal::Line);
	UI.Text(Label, R.X + R.W / 2.0f, R.Y + R.H / 2.0f + 0.5f, Ts(13.5f, 700, Lit ? pal::Ink : pal::Muted, Align::Center, Baseline::Middle));
	Cv.SetAlpha(A0);
	return St.Clicked && Enabled;
}

/** A segment's width for its label. */
float AutoSegmentW(Ui& UI, const std::string& Label)
{
	return UI.Measure(Label, 13.5f, 700) + 28.0f;
}

/** The hero's plate as the table shows it (the plan's think runs its timer), or null. */
const SeatVis* AutoHeroSeat(const Session& S)
{
	for (const SeatVis& V : S.Seats)
	{
		if (V.Present && V.IsHero)
		{
			return &V;
		}
	}
	return nullptr;
}

/** The event a finished result was (its history entry: the newest with that name, place and field). */
const HistoryEntry* AutoHistoryOf(const Session& S, const Results& R)
{
	for (const HistoryEntry& H : S.History)
	{
		if (H.Name == R.EventName && H.Place == R.Place && H.Entrants == R.Entrants)
		{
			return &H;
		}
	}
	return nullptr;
}

bool AutoPanelOpen(const RiverLine* RL)
{
	const auto It = AutoUiStates().find(RL);
	return It != AutoUiStates().end() && It->second.Open;
}

void AutoOpenPanel(const RiverLine* RL, Session& S, int Table, double Now)
{
	if (Table < 0 || Table >= S.TableCount())
	{
		return;
	}
	// The panel shows the career's plan and Form: from here the save keeps them (§6.2's U2AutoPref rule).
	S.Life.Update2LinesRead |= life::U2AutoPref;
	AutoUi& U = AutoUiOf(RL);
	U.Open = true;
	U.Table = Table;
	U.EventId = S.Glance(Table).EventId;
	U.OpenedAt = Now;
	U.SkipPicked = false;
	U.Note.clear();
}

void AutoClosePanel(const RiverLine* RL)
{
	const auto It = AutoUiStates().find(RL);
	if (It != AutoUiStates().end())
	{
		It->second.Open = false;
		It->second.SkipPicked = false;
	}
}

void ForgetAuto(const RiverLine* RL)
{
	AutoUiStates().erase(RL);
}

bool AutoKey(const RiverLine* RL, Session& S, const std::string& Key)
{
	const bool KeyK = Key == "k" || Key == "K";
	const bool Back = Key == "Backspace";
	// The panel: K or Backspace closes it (Esc is the laptop's pause, §2.6.1). It's modal: no other key reaches the table
	// under it (an action key there would take the wheel unseen).
	if (AutoPanelOpen(RL))
	{
		if (S.CurrentScreen == Screen::Table)
		{
			if (KeyK || Back)
			{
				AutoClosePanel(RL);
			}
			return true;
		}
		AutoClosePanel(RL);
	}
	if (S.CurrentScreen != Screen::Table || S.TableCount() == 0)
	{
		return false;
	}
	const int Front = S.FocusedTable();
	if (KeyK || Back)
	{
		const autoplay::Panel P = S.AutoPanel(Front);
		if (Back)
		{
			// Backspace stops a skip (it ends between hands; the plan keeps the seat at 1x).
			if (P.Skipping)
			{
				S.StopSkip(Front);
				return true;
			}
			return false;
		}
		if (!P.On)
		{
			// K with autoplay off: the plan the panel shows takes the seat (watch), from the next decision.
			autoplay::TableAuto A = P.Now;
			A.M = autoplay::Mode::Watch;
			S.SetAutoplay(Front, A);
			return true;
		}
		// K with it on (§2.6.5): mid-skip the skip stops (the plan watches at 1x); in the resume window or in a hand that's
		// already the player's, autoplay goes off; otherwise this hand is the player's.
		S.TakeOver(Front);
		return true;
	}
	// An action key during the plan's think takes the wheel and does nothing else; the next press acts (§2.6.1).
	if (!S.HasPrompt && (Key == "f" || Key == "c" || Key == "x" || Key == "r" || Key == "b" || Key == "a" || Key == "F" || Key == "C" || Key == "X" || Key == "R" || Key == "B" || Key == "A"))
	{
		const autoplay::Panel P = S.AutoPanel(Front);
		if (P.Thinking)
		{
			S.TakeOver(Front);
			return true;
		}
	}
	return false;
}

const autoplay::Played* AutoPlayedFor(const Session& S, const Results& R)
{
	const HistoryEntry* H = AutoHistoryOf(S, R);
	if (!H || H->EventId.empty())
	{
		return nullptr;
	}
	const autoplay::Played* P = autoplay::PlayedFor(S.Life.Auto, H->EventId);
	return P && (P->AiDecisions > 0 || P->AiHands > 0) ? P : nullptr;
}

const autoplay::Played* AutoPlayedFor(const Session& S, const HistoryEntry& H)
{
	if (H.EventId.empty())
	{
		return nullptr;
	}
	const autoplay::Played* P = autoplay::PlayedFor(S.Life.Auto, H.EventId);
	return P && (P->AiDecisions > 0 || P->AiHands > 0) ? P : nullptr;
}

std::string AutoTileLine(const autoplay::Panel& P)
{
	if (P.Skipping)
	{
		return P.SkipLine.empty() ? P.Status : P.SkipLine;
	}
	if (!P.On)
	{
		return std::string();
	}
	if (P.TakenOver)
	{
		return P.Status + " \xC2\xB7 your hand";
	}
	if (P.Thinking)
	{
		return P.Status + " \xC2\xB7 thinking\xE2\x80\xA6";
	}
	return P.LastAction.empty() ? P.Status : P.Status + " \xC2\xB7 " + P.LastAction;
}

std::string AutoTabLine(Canvas& Cv, const autoplay::Panel& P, const std::string& Status, float MaxW, float& Size)
{
	// Four tabs leave about a hundred units for the line: "SKIP · 799 to the bubble" or "AUTO · Balanced Reg 8x" would
	// lose their ends to an ellipsis there, so the line steps down a size, then to its short form, never cut.
	// (The laptop's tables are online, where no event takes a break: the first target is the next level.)
	const std::string Where = P.Now.To == autoplay::Target::NextBreak ? std::string("Next level") : std::string(autoplay::TargetName(P.Now.To));
	const std::string Short = P.Skipping ? "SKIP \xC2\xB7 " + Where : "AUTO \xC2\xB7 " + autoplay::SpeedName(P.Now.Speed);
	for (const std::string* Line : {&Status, &Short})
	{
		for (const float Try : {11.0f, 10.0f})
		{
			if (Cv.Measure(*Line, Try, 600) <= MaxW)
			{
				Size = Try;
				return *Line;
			}
		}
	}
	Size = 10.0f;
	return Short;
}

std::vector<std::string> AutoUiInventory()
{
	return {AutoSubtitle, AutoCloseHint, AutoSharpOff, AutoFormNote, AutoSloppy, AutoThinkHint, AutoYourHand, AutoYourHandKeys, AutoYourDecision, AutoPending,
		AutoDefaultSaved, AutoEveryDecision, AutoXpOwn};
}
} // namespace rlauto_detail

// ------------------------------------------------------------------ the panel

void RiverLine::AutoPanel(const Rect& R, double Now)
{
	rlauto_detail::AutoUi& U = rlauto_detail::AutoUiOf(this);
	const int Table = U.Table;
	if (!U.Open || Table < 0 || Table >= S.TableCount() || S.Glance(Table).EventId != U.EventId)
	{
		rlauto_detail::AutoClosePanel(this);
		return;
	}
	const autoplay::Panel P = S.AutoPanel(Table);
	// What the table is (the skip's targets that can't be reached there now are greyed).
	std::string Event;
	bool InMoney = false;
	bool Final = false;
	bool Breaks = false;
	bool Done = true;
	S.WithTable(Table, [&]() {
		if (S.T)
		{
			Event = S.T->Spec.Name;
			InMoney = S.T->InTheMoney();
			Final = S.T->Tables.size() <= 1;
			Breaks = S.T->Spec.BreakEvery > 0;
			Done = S.Finishing() || S.T->bFinished || S.T->Hero().Busted;
		}
	});
	const autoplay::Mode Shown = P.Skipping ? autoplay::Mode::Skip : P.Now.M;
	auto Say = [&](const std::string& Why, bool Bad) {
		U.Note = Why;
		U.NoteBad = Bad;
		U.NoteAt = Now;
	};
	auto Apply = [&](const autoplay::TableAuto& A) {
		const std::string Why = S.SetAutoplay(Table, A);
		if (!Why.empty())
		{
			Say(Why, true);
		}
		return Why.empty();
	};

	// A click anywhere off the panel closes it (the taskbar still does what it's asked).
	const bool Inside = UI.Ptr.X >= R.X && UI.Ptr.X <= R.X + R.W && UI.Ptr.Y >= R.Y && UI.Ptr.Y <= R.Y + R.H;
	if (UI.Ptr.Active && UI.Ptr.Pressed && !Inside)
	{
		rlauto_detail::AutoClosePanel(this);
		return;
	}

	const float In = 0.4f + 0.6f * rlnet_detail::NetEase((Now - U.OpenedAt) / 0.25);
	const float A0 = C->GetAlpha();
	// The table dims behind it (the top bar and the taskbar stay as they are).
	C->FillRect({0.0f, 64.0f, Width, Height - 64.0f - 38.0f}, Rgba(5, 9, 16, 0.6f * In));
	C->SetAlpha(A0 * In);
	C->GlowRoundRect({R.X, R.Y + 10.0f, R.W, R.H}, 18.0f, Rgba(0, 0, 0, 0.6f), 28.0f);
	UI.RRect(R, 18.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, rlsites_detail::SitesTone(0x13213a), rlsites_detail::SitesTone(0x0c1524)), rlsites_detail::SitesTone(0x2a3f60), 1.5f);

	// Header: whose panel this is, the state it's in, and the way out.
	rlnet_detail::NetSpaced(*C, autoplay::Text(autoplay::Words::PanelTitle), R.X + 30.0f, R.Y + 38.0f, 12.0f, 800, pal::Accent, 2.0f);
	UI.Text(Event, R.X + 30.0f, R.Y + 68.0f, Ts(22.0f, 800, pal::Ink, Align::Left, Baseline::Alphabetic, false, 520.0f));
	UI.Text(rlauto_detail::AutoSubtitle, R.X + 30.0f, R.Y + 92.0f, Ts(13.5f, 500, pal::Muted, Align::Left, Baseline::Alphabetic, false, 640.0f));
	{
		const Rect X{R.X + R.W - 52.0f, R.Y + 20.0f, 34.0f, 34.0f};
		const Ui::ClickState Cx = UI.Clickable("autoclose", X);
		C->FillCircle(X.X + 17.0f, X.Y + 17.0f, 17.0f, Cx.Hover ? pal::Line : rlsites_detail::SitesTone(0x162540));
		C->StrokePolyline({{X.X + 11.0f, X.Y + 11.0f}, {X.X + 23.0f, X.Y + 23.0f}}, false, pal::Ink, 2.0f, true);
		C->StrokePolyline({{X.X + 23.0f, X.Y + 11.0f}, {X.X + 11.0f, X.Y + 23.0f}}, false, pal::Ink, 2.0f, true);
		const std::string State = P.Skipping ? std::string("SKIPPING") : P.On ? "WATCHING " + autoplay::SpeedName(P.Now.Speed) : std::string("OFF");
		const Color Sc = P.Skipping ? pal::Accent2 : P.On ? pal::Accent : pal::Muted;
		const float Sw = C->Measure(State, 11.0f, 700) + 11.0f * 1.3f;
		rlnet_detail::NetPill(*C, State, X.X - 14.0f - Sw, R.Y + 27.0f, Sc, P.On, 11.0f);
		UI.Text(rlauto_detail::AutoCloseHint, R.X + R.W - 30.0f, R.Y + 92.0f, Ts(12.5f, 600, pal::Dim, Align::Right));
		if (Cx.Clicked)
		{
			rlauto_detail::AutoClosePanel(this);
		}
	}

	// The style tiles, 4-3-4 (§2.6.1): name, swing chip, edge line, best in; a greyed tile says why.
	const std::vector<int> Order = rlauto_detail::AutoShownStyles(P);
	const std::vector<int> Rows = rlauto_detail::AutoRows(static_cast<int>(Order.size()));
	const float GridTop = R.Y + 112.0f;
	for (size_t K = 0; K < Order.size(); ++K)
	{
		const style::Info& I = P.Styles[static_cast<size_t>(Order[K])];
		const std::string Why = static_cast<size_t>(Order[K]) < P.Greyed.size() ? P.Greyed[static_cast<size_t>(Order[K])] : std::string();
		const bool Greyed = !Why.empty();
		const bool Picked = I.S == P.Now.S;
		const Rect T = rlauto_detail::AutoTileRect(R, GridTop, Rows, static_cast<int>(K));
		const Ui::ClickState St = UI.Clickable(std::string("autostyle") + I.Id, T, !Greyed && !Done);
		const float Ta = C->GetAlpha();
		C->SetAlpha(Ta * (Greyed ? 0.5f : 1.0f));
		if (Picked && !Greyed)
		{
			C->GlowRoundRect(T, 12.0f, rlnet_detail::NetA(pal::Accent, 0.28f), 12.0f);
		}
		UI.RRect(T, 12.0f, Paint::Linear({0.0f, T.Y}, {0.0f, T.Y + T.H}, Picked ? rlsites_detail::SitesTone(0x163a4c) : St.Hover ? rlsites_detail::SitesTone(0x16263d) : rlsites_detail::SitesTone(0x111d31), Picked ? rlsites_detail::SitesTone(0x0f2433) : rlsites_detail::SitesTone(0x0d1729)),
			Picked && !Greyed ? pal::Accent : pal::Line, Picked && !Greyed ? 2.0f : 1.0f);
		const Color Sw = rlauto_detail::AutoSwingColor(I.Var);
		const std::string Swing = style::SwingName(I.Var);
		const float SwW = C->Measure(Swing, 9.5f, 700) + 9.5f * 1.3f;
		rlnet_detail::NetPill(*C, Swing, T.X + T.W - 14.0f - SwW, T.Y + 13.0f, Sw, false, 9.5f);
		UI.Text(I.Name, T.X + 16.0f, T.Y + 30.0f, Ts(17.0f, 800, Picked ? pal::Ink : rlsites_detail::SitesTone(0xdbe4f0), Align::Left, Baseline::Alphabetic, false, T.W - 32.0f - SwW - 8.0f));
		rlnet_detail::NetParagraph(*C, I.Edge, T.X + 16.0f, T.Y + 50.0f, T.W - 32.0f, rlauto_detail::AutoEdgeSize, 500, rlsites_detail::SitesTone(0xb6c2d4), rlauto_detail::AutoEdgeLine, rlauto_detail::AutoEdgeLines);
		if (Greyed)
		{
			rlnet_detail::NetLockIcon(*C, T.X + 15.0f, T.Y + T.H - 25.0f, 13.0f, pal::Orange);
			UI.Text(Why, T.X + 34.0f, T.Y + T.H - 14.0f, Ts(12.0f, 700, pal::Orange, Align::Left, Baseline::Alphabetic, false, T.W - 50.0f));
		}
		else
		{
			// BEST IN and the line after it; a long one carries on below, under the label (never cut short).
			const std::pair<std::string, std::string> Best = rlauto_detail::AutoSplitAt(*C, I.BestIn, T.W - 40.0f - rlauto_detail::AutoSpacedW(*C, "BEST IN", 9.0f, 800, 1.2f), 12.0f, 600);
			const float Ly = T.Y + T.H - (Best.second.empty() ? 14.0f : 28.0f);
			const float Lw = rlnet_detail::NetSpaced(*C, "BEST IN", T.X + 16.0f, Ly, 9.0f, 800, pal::Dim, 1.2f);
			UI.Text(Best.first, T.X + 24.0f + Lw, Ly, Ts(12.0f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, false, T.W - 40.0f - Lw));
			if (!Best.second.empty())
			{
				UI.Text(Best.second, T.X + 16.0f, Ly + 14.0f, Ts(12.0f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, false, T.W - 32.0f));
			}
		}
		if (Picked && !Greyed)
		{
			// On the tile's top edge, so it never sits on the tile's words.
			rlnet_detail::NetPill(*C, P.On ? std::string("PLAYING") : std::string("PICKED"), T.X + 16.0f, T.Y - 8.5f, pal::Accent, true, 9.0f);
		}
		C->SetAlpha(Ta);
		if (St.Clicked && !Greyed && I.S != P.Now.S)
		{
			autoplay::TableAuto A = P.Now;
			A.S = I.S;
			A.M = Shown;
			Apply(A);
		}
	}
	const float Lower = GridTop + static_cast<float>(Rows.size()) * (rlauto_detail::AutoTileH + rlauto_detail::AutoTileGap) + 14.0f;

	// Sharpness and Form (§2.5), on the left: what it is, why, and when it was read.
	{
		const float X = R.X + 30.0f;
		const float W = 430.0f;
		rlnet_detail::NetSpaced(*C, "SHARPNESS", X, Lower + 12.0f, 10.5f, 800, pal::Muted, 1.6f);
		const Color Sc = P.Sharp >= 0.9 ? pal::Green : P.Sharp >= 0.7 ? pal::Accent : pal::Orange;
		const float Nw = UI.Text(std::to_string(static_cast<int>(std::lround(P.Sharp * 100.0))) + "%", X, Lower + 56.0f, Ts(38.0f, 900, Sc, Align::Left, Baseline::Alphabetic, true));
		const Rect Bar{X + Nw + 18.0f, Lower + 40.0f, W - Nw - 18.0f, 8.0f};
		UI.RRect(Bar, 4.0f, Rgba(255, 255, 255, 0.08f));
		const float Frac = static_cast<float>(Clamp01((P.Sharp - 0.5) / 0.5));
		UI.RRect({Bar.X, Bar.Y, std::max(8.0f, Bar.W * Frac), Bar.H}, 4.0f, Sc);
		UI.Text(P.SharpWhy, X, Lower + 82.0f, Ts(15.0f, 700, pal::Ink, Align::Left, Baseline::Alphabetic, false, W));
		const std::string When = P.On ? "Held for this run since " + net::TimeLabel(P.SharpSetAt) + "." : std::string(rlauto_detail::AutoSharpOff);
		UI.Text(When, X, Lower + 104.0f, Ts(12.5f, 600, pal::Dim, Align::Left, Baseline::Alphabetic, false, W));
		float Y = Lower + 132.0f;
		if (P.Sharp < 0.70)
		{
			Y = rlnet_detail::NetParagraph(*C, rlauto_detail::AutoSloppy, X, Y, W, 13.0f, 700, pal::Orange, 18.0f, 2) + 4.0f;
		}
		rlnet_detail::NetParagraph(*C, rlauto_detail::AutoFormNote, X, Y, W, 13.0f, 500, pal::Muted, 18.0f, 2);
	}

	// Watch speeds and SKIP, the skip's target, the stop rules (§2.6.1, §2.6.4), on the right.
	{
		const float X = R.X + 500.0f;
		const float Right = R.X + R.W - 30.0f;
		rlnet_detail::NetSpaced(*C, "WATCH", X, Lower + 12.0f, 10.5f, 800, pal::Muted, 1.6f);
		float Sx = X;
		for (const int Speed : {1, 2, 4, 8})
		{
			const Rect B{Sx, Lower + 22.0f, 56.0f, 32.0f};
			const bool Lit = !P.Skipping && !U.SkipPicked && P.Now.Speed == Speed;
			if (rlauto_detail::AutoSegment(UI, *C, "autospeed" + std::to_string(Speed), B, autoplay::SpeedName(Speed), Lit, !Done))
			{
				autoplay::TableAuto A = P.Now;
				A.Speed = Speed;
				A.M = P.On ? autoplay::Mode::Watch : autoplay::Mode::Off;
				U.SkipPicked = false;
				Apply(A);
			}
			Sx += 62.0f;
		}
		{
			const Rect B{Sx + 10.0f, Lower + 22.0f, 74.0f, 32.0f};
			if (rlauto_detail::AutoSegment(UI, *C, "autoskip", B, "SKIP", P.Skipping || (!P.On && U.SkipPicked), !Done) && !P.Skipping)
			{
				if (P.On)
				{
					const std::string Why = S.SkipAhead(Table, P.Now.To, P.Now.To == autoplay::Target::Hands ? 50 : 0);
					if (!Why.empty())
					{
						Say(Why, true);
					}
				}
				else
				{
					U.SkipPicked = true;
				}
			}
		}
		rlnet_detail::NetSpaced(*C, "SKIP TO", X, Lower + 80.0f, 10.5f, 800, pal::Muted, 1.6f);
		float Tx = X;
		for (const autoplay::Target To : {autoplay::Target::NextBreak, autoplay::Target::Bubble, autoplay::Target::FinalTable, autoplay::Target::Result, autoplay::Target::Hands})
		{
			// Online events take no breaks: there the first target stops at the next level, and says so (as its button does).
			const std::string Label = To == autoplay::Target::NextBreak && !Breaks ? std::string("Next level") : std::string(autoplay::TargetName(To));
			const float W = rlauto_detail::AutoSegmentW(UI, Label);
			const bool Reachable = !(To == autoplay::Target::Bubble && InMoney) && !(To == autoplay::Target::FinalTable && Final);
			const Rect B{Tx, Lower + 90.0f, W, 32.0f};
			if (rlauto_detail::AutoSegment(UI, *C, std::string("autoto") + Label, B, Label, P.Now.To == To, Reachable && !Done))
			{
				autoplay::TableAuto A = P.Now;
				A.To = To;
				A.Hands = To == autoplay::Target::Hands ? 50 : 0;
				if (P.Skipping)
				{
					const std::string Why = S.SkipAhead(Table, To, A.Hands);
					if (!Why.empty())
					{
						Say(Why, true);
					}
				}
				else
				{
					A.M = Shown;
					Apply(A);
				}
			}
			Tx += W + 8.0f;
		}
		rlnet_detail::NetSpaced(*C, "STOP RULES", X, Lower + 148.0f, 10.5f, 800, pal::Muted, 1.6f);
		float Gx = X;
		float Gy = Lower + 158.0f;
		for (const uint32_t Bit : {autoplay::StopFinalTable, autoplay::StopBubble, autoplay::StopMyAllIn, autoplay::StopBigPot, autoplay::StopPremium})
		{
			const std::string Label = autoplay::StopName(Bit);
			const float W = UI.Measure(Label, 13.0f, 700) + 46.0f;
			if (Gx + W > Right)
			{
				Gx = X;
				Gy += 36.0f;
			}
			if (rlauto_detail::AutoToggle(UI, *C, "autostop" + std::to_string(Bit), {Gx, Gy, W, 32.0f}, Label, (P.Now.Stops & Bit) != 0, !Done))
			{
				autoplay::TableAuto A = P.Now;
				A.Stops ^= Bit;
				A.M = Shown;
				Apply(A);
			}
			Gx += W + 8.0f;
		}
	}

	// The footer: the career's default, the session's last word, and the one button that starts or stops it.
	{
		const float Fy = R.Y + R.H - 66.0f;
		C->FillRect({R.X + 24.0f, Fy - 14.0f, R.W - 48.0f, 1.0f}, pal::Line);
		const autoplay::TableAuto Def = autoplay::DefaultPlan(S.Life.Auto);
		const bool IsDefault = Def.S == P.Now.S && Def.Speed == P.Now.Speed && Def.To == P.Now.To && Def.Stops == P.Now.Stops;
		ButtonOpts D;
		D.Kind = ButtonKind::Secondary;
		D.Size = 15.0f;
		D.Enabled = !IsDefault;
		if (rlsites_detail::SitesButton(UI, *C, "autodefault", {R.X + 30.0f, Fy, 240.0f, 46.0f}, IsDefault ? std::string("Your default") : std::string(autoplay::Text(autoplay::Words::MakeDefault)), D) && !IsDefault)
		{
			autoplay::SetDefault(S.Life.Auto, S.Life, P.Now);
			Say(rlauto_detail::AutoDefaultSaved, false);
		}
		if (IsDefault)
		{
			rlnet_detail::NetCheck(*C, R.X + 52.0f, Fy + 23.0f, 12.0f, pal::Accent);
		}
		if (!U.Note.empty() && Now - U.NoteAt < 5.0)
		{
			UI.Text(U.Note, R.X + 290.0f, Fy + 28.0f, Ts(13.5f, 700, U.NoteBad ? pal::Orange : pal::Accent, Align::Left, Baseline::Alphabetic, false, R.W - 290.0f - 300.0f));
		}
		ButtonOpts Go;
		Go.Size = 16.0f;
		Go.Enabled = !Done;
		std::string Label;
		if (P.Skipping)
		{
			Go.Kind = ButtonKind::Secondary;
			Label = "Stop skipping";
		}
		else if (P.On)
		{
			Go.Kind = ButtonKind::Danger;
			Label = "Take my seat back";
		}
		else
		{
			Go.Kind = U.SkipPicked ? ButtonKind::Gold : ButtonKind::Primary;
			Label = U.SkipPicked ? rlauto_detail::AutoSkipButton(P.Now.To, Breaks) : "Watch it play \xC2\xB7 " + autoplay::SpeedName(P.Now.Speed);
		}
		if (rlsites_detail::SitesButton(UI, *C, "autogo", {R.X + R.W - 290.0f, Fy, 260.0f, 46.0f}, Label, Go) && !Done)
		{
			bool Closed = false;
			if (P.Skipping)
			{
				S.StopSkip(Table);
				Closed = true;
			}
			else if (P.On)
			{
				autoplay::TableAuto A = P.Now;
				A.M = autoplay::Mode::Off;
				Closed = Apply(A);
			}
			else if (U.SkipPicked)
			{
				const std::string Why = S.SkipAhead(Table, P.Now.To, P.Now.To == autoplay::Target::Hands ? 50 : 0);
				Closed = Why.empty();
				if (!Closed)
				{
					Say(Why, true);
				}
			}
			else
			{
				autoplay::TableAuto A = P.Now;
				A.M = autoplay::Mode::Watch;
				Closed = Apply(A);
			}
			if (Closed)
			{
				rlauto_detail::AutoClosePanel(this);
			}
		}
	}
	C->SetAlpha(A0);
}

// ------------------------------------------------------------------ the watch strip

void RiverLine::AutoStrip(const Rect& R, double Now)
{
	const int Table = S.FocusedTable();
	const autoplay::Panel P = S.AutoPanel(Table);
	if (!P.On || !S.T)
	{
		return;
	}
	const style::Info& Info = style::InfoOf(P.Now.S);
	rlauto_detail::AutoUi& U = rlauto_detail::AutoUiOf(this);
	// The skip's stop reason, shown for a while after the skip ends ("The bubble.", "A stop rule: this decision's yours.").
	std::string Stop;
	{
		std::pair<std::string, double>& Seen = U.Stops[S.T->Spec.Id];
		if (P.StopReason != Seen.first)
		{
			Seen = {P.StopReason, Now};
		}
		Stop = !Seen.first.empty() && Now - Seen.second < 8.0 ? Seen.first : std::string();
	}
	if (R.H < 40.0f)
	{
		// Over the player's own controls (a hand taken over, the prompt open): one line that says whose hand it is.
		const float Pw = rlnet_detail::NetPill(*C, "YOUR HAND", R.X, R.Y + 2.0f, pal::Gold, true, 10.0f);
		UI.Text(!Stop.empty() ? Stop : std::string(P.TakenOver ? rlauto_detail::AutoYourHandKeys : rlauto_detail::AutoYourDecision), R.X + Pw + 10.0f, R.Y + 15.0f,
			Ts(13.0f, 700, Stop.empty() ? rlsites_detail::SitesTone(0xc3cedf) : pal::Gold, Align::Left, Baseline::Alphabetic, false, R.W - Pw - 10.0f));
		return;
	}
	UI.RRect(R, 12.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, rlsites_detail::SitesTone(0x102036), rlsites_detail::SitesTone(0x0b1524)), rlnet_detail::NetA(pal::Accent, 0.35f));

	// Row 1: the style (it opens the panel), the speeds, Take over [K].
	const float Ry = R.Y + 10.0f;
	{
		const std::string Name = rlnet_detail::NetUpper(Info.Name);
		const float W = UI.Measure(Name, 13.0f, 800) + 38.0f;
		const Rect Chip{R.X + 10.0f, Ry, W, 30.0f};
		const Ui::ClickState St = UI.Clickable("autochip", Chip);
		UI.RRect(Chip, 15.0f, St.Hover ? rlnet_detail::NetA(pal::Accent, 0.3f) : rlnet_detail::NetA(pal::Accent, 0.16f), rlnet_detail::NetA(pal::Accent, 0.7f));
		UI.Text(Name, Chip.X + 13.0f, Chip.Y + 15.5f, Ts(13.0f, 800, pal::Accent, Align::Left, Baseline::Middle));
		rlnet_detail::NetChevron(*C, Chip.X + Chip.W - 14.0f, Chip.Y + 15.0f, 9.0f, 2, pal::Accent); // the panel opens over the table
		if (St.Clicked)
		{
			rlauto_detail::AutoOpenPanel(this, S, Table, Now);
		}
	}
	const float TakeW = 150.0f;
	float Sx = R.X + R.W - 10.0f - TakeW - 12.0f - 4.0f * 44.0f + 4.0f;
	for (const int Speed : {1, 2, 4, 8})
	{
		const Rect B{Sx, Ry, 40.0f, 30.0f};
		if (rlauto_detail::AutoSegment(UI, *C, "autostrip" + std::to_string(Speed), B, autoplay::SpeedName(Speed), P.Now.Speed == Speed && !P.Skipping, !P.Skipping) && P.Now.Speed != Speed)
		{
			autoplay::TableAuto A = P.Now;
			A.Speed = Speed;
			A.M = autoplay::Mode::Watch;
			S.SetAutoplay(Table, A);
		}
		Sx += 44.0f;
	}
	{
		ButtonOpts O;
		O.Kind = P.TakenOver ? ButtonKind::Secondary : ButtonKind::Gold;
		O.Size = 13.5f;
		const std::string Label = P.TakenOver || P.ResumeWindow ? std::string("Autoplay off [K]") : std::string(autoplay::Text(autoplay::Words::TakeOverKey));
		if (rlsites_detail::SitesButton(UI, *C, "autotake", {R.X + R.W - 10.0f - TakeW, Ry, TakeW, 30.0f}, Label, O))
		{
			S.TakeOver(Table);
		}
	}

	// Row 2: the plan's think (never its move), else its last action once played, else whose hand it is.
	const float Ly = R.Y + 64.0f;
	const SeatVis* Hero = rlauto_detail::AutoHeroSeat(S);
	if (P.ResumeWindow)
	{
		UI.Text(autoplay::Text(autoplay::Words::Resumes), R.X + 14.0f, Ly, Ts(15.0f, 800, pal::Gold));
		const float Frac = static_cast<float>(Clamp01(P.ResumeLeft / autoplay::ResumeWindowSeconds));
		UI.RRect({R.X + 14.0f, Ly + 10.0f, R.W - 28.0f, 4.0f}, 2.0f, Rgba(255, 255, 255, 0.08f));
		UI.RRect({R.X + 14.0f, Ly + 10.0f, (R.W - 28.0f) * Frac, 4.0f}, 2.0f, pal::Gold);
	}
	else if (P.Skipping)
	{
		UI.Text(rlauto_detail::AutoPending, R.X + 14.0f, Ly, Ts(15.0f, 700, pal::Accent2));
		UI.Text(P.Status, R.X + R.W - 14.0f, Ly, Ts(12.5f, 700, pal::Muted, Align::Right, Baseline::Alphabetic, true));
	}
	else if (P.TakenOver)
	{
		UI.Text(rlauto_detail::AutoYourHand, R.X + 14.0f, Ly, Ts(15.0f, 700, pal::Gold, Align::Left, Baseline::Alphabetic, false, R.W - 28.0f));
	}
	else if (P.Thinking)
	{
		const float Tw = UI.Text(autoplay::ThinkingLine(P.Now.S), R.X + 14.0f, Ly, Ts(15.0f, 700, pal::Ink));
		UI.Text(rlauto_detail::AutoThinkHint, R.X + R.W - 14.0f, Ly, Ts(12.5f, 600, pal::Muted, Align::Right, Baseline::Alphabetic, false, R.W - Tw - 40.0f));
		double Frac = 0.0;
		if (Hero && Hero->ActEnd > Hero->ActStart)
		{
			Frac = Clamp01((Hero->ActEnd - Now) / (Hero->ActEnd - Hero->ActStart));
		}
		UI.RRect({R.X + 14.0f, Ly + 10.0f, R.W - 28.0f, 4.0f}, 2.0f, Rgba(255, 255, 255, 0.08f));
		UI.RRect({R.X + 14.0f, Ly + 10.0f, std::max(4.0f, (R.W - 28.0f) * static_cast<float>(Frac)), 4.0f}, 2.0f, pal::Accent);
	}
	else if (!P.LastAction.empty())
	{
		const float Lw = rlnet_detail::NetSpaced(*C, "LAST MOVE", R.X + 14.0f, Ly, 10.0f, 800, pal::Muted, 1.4f);
		UI.Text(P.LastAction, R.X + 26.0f + Lw, Ly, Ts(15.0f, 800, pal::Ink, Align::Left, Baseline::Alphabetic, true));
	}
	else
	{
		UI.Text(autoplay::StatusLine(P.Now, P.Sharp), R.X + 14.0f, Ly, Ts(13.5f, 700, pal::Muted, Align::Left, Baseline::Alphabetic, false, R.W - 28.0f));
	}

	// Row 3: during the think, the action buttons where they always are: the first press takes the wheel and nothing
	// else (§2.6.1), the next one acts. Otherwise what the plan has done here, and why a skip stopped.
	const float Ax = R.X;
	const float Aw = R.W;
	const float By = R.Y + 84.0f;
	if (P.Thinking && S.CurHand && !S.CurHand->bComplete)
	{
		const LegalActions Legal = S.CurHand->GetLegalActions();
		const float Bw = (Aw - 20.0f) / 3.0f;
		ButtonOpts O;
		O.Kind = ButtonKind::Ghost;
		O.Size = 22.0f;
		bool Take = rlsites_detail::SitesButton(UI, *C, "autofold", {Ax, By, Bw, 66.0f}, "Fold", O);
		ButtonOpts Co = O;
		Co.Sub = Legal.CanCheck ? std::string() : ChipsText(static_cast<double>(Legal.CallAmount));
		Take = rlsites_detail::SitesButton(UI, *C, "autocall", {Ax + Bw + 10.0f, By, Bw, 66.0f}, Legal.CanCheck ? "Check" : "Call", Co) || Take;
		if (Legal.CanRaise)
		{
			Take = rlsites_detail::SitesButton(UI, *C, "autoraise", {Ax + (Bw + 10.0f) * 2.0f, By, Bw, 66.0f}, Legal.IsBet ? "Bet" : "Raise", O) || Take;
		}
		if (Take)
		{
			S.TakeOver(Table);
		}
		return;
	}
	if (!Stop.empty())
	{
		UI.Text(Stop, R.X + 14.0f, By + 22.0f, Ts(14.0f, 800, pal::Gold, Align::Left, Baseline::Alphabetic, false, R.W - 28.0f));
	}
	const std::string Counts = "This event so far: " + Grouped(P.AiDecisions) + (P.AiDecisions == 1 ? " decision" : " decisions") + " by your plan, " + Grouped(P.OwnDecisions) + " by you.";
	UI.Text(Counts, R.X + 14.0f, By + (Stop.empty() ? 22.0f : 46.0f), Ts(13.0f, 600, pal::Dim, Align::Left, Baseline::Alphabetic, false, R.W - 28.0f));
}

// ------------------------------------------------------------------ the skip card

void RiverLine::SkipCard(const Rect& R, double Now)
{
	if (!S.T)
	{
		return;
	}
	const Tournament& T = *S.T;
	const int Table = S.FocusedTable();
	const autoplay::Panel P = S.AutoPanel(Table);
	const bool Compact = R.W < 800.0f;
	C->FillRect(R, Rgba(5, 9, 16, 0.78f));
	// "SKIPPING · PUSH/FOLD" over "hand 214 · level 11 · 31 to the bubble" (§2.6.1).
	const std::pair<std::string, std::string> Line = rlauto_detail::AutoSplitSkipLine(P.SkipLine.empty() ? std::string("SKIPPING") : P.SkipLine);
	const float Cx = R.X + R.W / 2.0f;
	const float Bar = static_cast<float>(std::fmod(Now * 0.8, 1.0));
	if (Compact)
	{
		const float Cy = R.Y + R.H / 2.0f;
		// A plate under the words: in a tile the board sits right behind them, and shows through the dim.
		const float Pw = std::min(R.W - 24.0f, std::max(380.0f, std::max(UI.Measure(Line.first, 24.0f, 900), UI.Measure(Line.second, 14.0f, 600)) + 80.0f));
		UI.RRect({Cx - Pw / 2.0f, Cy - 58.0f, Pw, 140.0f}, 12.0f, Rgba(6, 10, 18, 0.9f), rlnet_detail::NetA(pal::Accent, 0.35f));
		UI.Text(Line.first, Cx, Cy - 26.0f, Ts(24.0f, 900, pal::Accent, Align::Center, Baseline::Alphabetic, false, R.W - 40.0f));
		UI.Text(Line.second, Cx, Cy + 2.0f, Ts(14.0f, 600, pal::Ink, Align::Center, Baseline::Alphabetic, false, R.W - 40.0f));
		UI.RRect({Cx - 160.0f, Cy + 20.0f, 320.0f, 5.0f}, 2.5f, rlsites_detail::SitesTone(0x0b1422));
		UI.RRect({Cx - 160.0f + Bar * 256.0f, Cy + 20.0f, 64.0f, 5.0f}, 2.5f, pal::Accent);
		ButtonOpts O;
		O.Kind = ButtonKind::Secondary;
		O.Size = 13.0f;
		if (rlsites_detail::SitesButton(UI, *C, "autoskipstop" + T.Spec.Id, {Cx - 70.0f, Cy + 40.0f, 140.0f, 32.0f}, "Stop", O))
		{
			S.StopSkip(Table);
		}
		return;
	}
	UI.Text(Line.first, Cx, R.Y + 266.0f, Ts(40.0f, 900, pal::Accent, Align::Center, Baseline::Alphabetic, false, R.W - 80.0f));
	UI.Text(Line.second, Cx, R.Y + 306.0f, Ts(19.0f, 600, pal::Ink, Align::Center, Baseline::Alphabetic, false, R.W - 80.0f));
	const Level& L = T.CurrentLevel();
	const std::pair<std::string, std::string> Stats[4] = {
		{"Level", std::to_string(T.LevelIndex + 1) + " \xC2\xB7 " + ChipsText(static_cast<double>(L.Sb)) + "/" + ChipsText(static_cast<double>(L.Bb))},
		{"Players left", ChipsText(T.Remaining)},
		{"Your stack", ChipsText(static_cast<double>(T.Hero().Stack)) + " (" + std::to_string(static_cast<int>(JsRound(static_cast<double>(T.Hero().Stack) / static_cast<double>(L.Bb)))) + " BB)"},
		{"Your rank", Ordinal(T.HeroRank())},
	};
	for (int I = 0; I < 4; ++I)
	{
		const float X = Cx - 330.0f + static_cast<float>(I) * 220.0f;
		UI.Text(Stats[I].first, X + 100.0f, R.Y + 386.0f, Ts(15.0f, 500, pal::Muted, Align::Center));
		UI.Text(Stats[I].second, X + 100.0f, R.Y + 418.0f, Ts(21.0f, 800, pal::Ink, Align::Center, Baseline::Alphabetic, true));
	}
	UI.RRect({Cx - 300.0f, R.Y + 466.0f, 600.0f, 8.0f}, 4.0f, rlsites_detail::SitesTone(0x0b1422));
	UI.RRect({Cx - 300.0f + Bar * 480.0f, R.Y + 466.0f, 120.0f, 8.0f}, 4.0f, pal::Accent);
	// K takes the wheel in two presses (§2.6.5: the skip stops with the plan at 1x, then the hand is yours); the button does
	// both, so its words hold. Stop leaves the plan the seat at 1x, as Backspace does.
	ButtonOpts O;
	O.Kind = ButtonKind::Gold;
	O.Size = 20.0f;
	if (rlsites_detail::SitesButton(UI, *C, "stopsprint", {Cx - 250.0f, R.Y + 516.0f, 240.0f, 60.0f}, "Take the wheel", O))
	{
		S.StopSkip(Table);
		S.TakeOver(Table);
	}
	ButtonOpts So;
	So.Kind = ButtonKind::Secondary;
	So.Size = 20.0f;
	if (rlsites_detail::SitesButton(UI, *C, "autoskipstop", {Cx + 10.0f, R.Y + 516.0f, 240.0f, 60.0f}, "Stop", So))
	{
		S.StopSkip(Table);
	}
	UI.Text(autoplay::Text(autoplay::Words::SkipKeys), Cx, R.Y + 610.0f, Ts(14.0f, 600, pal::Muted, Align::Center));
}
} // namespace ui
} // namespace ss
