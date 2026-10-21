// RiverLine's autoplay (update 2 SPEC §2.6.1, §2.11): the panel over a table ("AI AUTOPLAY": the style tiles, Sharpness
// and Form, the watch speeds, the skip's target and the stop rules, "Make this my default"), the watch strip under the
// felt (the style, the think bar that never names a move before it's played, the plan's last action, the speeds and
// Take over [K]), the skip card over the dimmed table, and the keys (K, Backspace, an action key during the plan's
// think). Everything the session knows comes from Session::AutoPanel (core-autoplay's Panel); every change goes through
// SetAutoplay, SkipAhead, StopSkip and TakeOver. The words "AI autoplay" are system UI here (U1): nothing in the world
// ever says them. Side state lives beside the class, by laptop, as Mail's does. ui-laptop's file.
//
// Update 3, G1 fix 2 (fix2-auto; g1-audit N1; SPEC §2.3-§2.5, §3.3-§3.5, §9.6 "share no chrome"): at a rival's table the panel,
// the strip and the skip card speak the site's own language through its SiteLook and its skin. RiverLine's look draws exactly
// what it always drew (its branches below are the old code, untouched). Ignite's: candy pills and round cards in the neon,
// BlackItalic titles and numbers, the think as a burning fuse. RunItTwice's: slabs and panels with one cut, BoldCondensed
// caps, the section slab and the twice print, red as light (fills, edges, glows; never letters), Hot White on red, Mono for
// the machine's lines. Same rects, ids and presses everywhere, so the keys, the clicks and the tests read one panel.
#include "ShortStack/UI/RiverLine.h"
#include "../StrictFloat.h"
#include "IgniteArt.h"
#include "RiverLineShared.h"
#include "RitArt.h"

#include "ShortStack/AI/Style.h"
#include "ShortStack/Game/Autoplay.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/SiteShow.h"

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

// ---- Update 3, G1 fix 2 (fix2-auto; g1-audit N1; §2.3-§2.5, §3.3-§3.5, §9.6): the rivals' language for the panel, the
// strip and the skip card. Nothing here draws for RiverLine's look.

/** Whose language the autoplay chrome speaks: RiverLine's (the code as it was), Ignite's pills, RunItTwice's one cut. */
enum class AutoLook : uint8_t
{
	Home,
	Ignite,
	Rit,
};

AutoLook AutoLookOf(const rlsites_detail::SiteLook& L)
{
	return L.Cut > 0.0f ? AutoLook::Rit : L.Button == rlsites_detail::ButtonShape::Pill ? AutoLook::Ignite : AutoLook::Home;
}

/** RunItTwice's rail at a table (RiverLineRit.cpp's RitRailMini): its chrome, which the panel and the skip card keep clear of. */
const float AutoRitRail = 64.0f;

/** The tiles are up (RunItTwice draws them in RiverLine's frame: no rail there, and RiverLine's 64-unit top bar). */
bool AutoTiled(const Session& S)
{
	return S.Tiled && S.TableCount() >= 2;
}

/** Ignite's hour (Afterburner Hours swaps the coral family for violet, §2.3). */
igniteart::IgniteTones AutoIgTones(double World)
{
	return igniteart::IgniteTonesFor(site::show::IgniteMood(World));
}

/** A run in the look's display face: Ignite's BlackItalic, RunItTwice's BoldCondensed. */
TextStyle AutoFace(const rlsites_detail::SiteLook& L, float Size, const Color& Col, Align A = Align::Left, Baseline B = Baseline::Alphabetic)
{
	TextStyle St = Ts(Size, L.Cut > 0.0f ? 700 : 900, Col, A, B);
	St.Face = L.TitleFace;
	return St;
}

/** RunItTwice's Mono: the machine's print (the skip's line, the counts, the keys, a time). */
TextStyle AutoMono(float Size, const Color& Col, Align A = Align::Left)
{
	return Ts(Size, 500, Col, A, Baseline::Alphabetic, true);
}

/** A label as the look writes it (RunItTwice's in caps). */
std::string AutoCaps(const rlsites_detail::SiteLook& L, const std::string& Text)
{
	return L.LabelCaps ? rlnet_detail::NetUpper(Text) : Text;
}

/** Text's width in St's face. */
float AutoWidth(const Canvas& Cv, const std::string& Text, const TextStyle& St)
{
	return Cv.Measure(Text, St.Size, St.Face >= 0 ? static_cast<Font>(St.Face) : Canvas::FaceFor(St.Weight, St.Mono));
}

/** St a size down until Text fits MaxW: a rival's lines are never cut (the plan's words stay whole, §2.11). */
TextStyle AutoFit(const Canvas& Cv, const std::string& Text, TextStyle St, float MaxW)
{
	const float W = AutoWidth(Cv, Text, St);
	if (W > MaxW && W > 0.0f)
	{
		St.Size = std::max(St.Size * 0.5f, St.Size * MaxW / W);
	}
	return St;
}

/** Ignite's plum ink: a label on a lit candy fill. */
Color AutoIgInk()
{
	return Hex(rlsites_detail::SitesIgPlumInk);
}

/** Ignite's candy pill: a fill, a gloss across its top when lit, and a rim. */
void AutoIgPill(Canvas& Cv, const Rect& R, const Paint& Fill, const Color& Rim, float RimW, bool Gloss)
{
	const float Rr = R.H / 2.0f;
	Cv.FillRoundRect(R, Rr, Fill);
	if (Gloss && R.W > Rr * 1.2f + 4.0f)
	{
		Cv.FillRoundRect({R.X + Rr * 0.6f, R.Y + 3.0f, R.W - Rr * 1.2f, R.H * 0.28f}, R.H * 0.14f, Rgba(255, 255, 255, 0.22f));
	}
	if (RimW > 0.0f)
	{
		Cv.StrokeRoundRect(R, Rr, Rim, RimW);
	}
}

/** Ignite's candy meter (Composure's): a Night track, the fill with its gloss and a cream spark at its head. */
void AutoIgMeter(Canvas& Cv, const Rect& R, float Frac, const Paint& Fill)
{
	const float Rr = R.H / 2.0f;
	Cv.FillRoundRect(R, Rr, Hex(rlsites_detail::SitesIgNight));
	Cv.StrokeRoundRect(R, Rr, pal::Line, 1.0f);
	const float Fw = std::max(R.H, R.W * std::min(1.0f, std::max(0.0f, Frac)));
	Cv.FillRoundRect({R.X, R.Y, Fw, R.H}, Rr, Fill);
	Cv.FillRoundRect({R.X + 4.0f, R.Y + 2.0f, std::max(2.0f, Fw - 8.0f), R.H * 0.25f}, R.H * 0.125f, Rgba(255, 255, 255, 0.3f));
	rlsites_detail::SitesSpark(Cv, R.X + Fw - 3.0f, R.Y + Rr, std::max(6.0f, R.H * 0.7f), pal::Ink);
}

/**
 * Ignite's time bank as a fuse (§2.5) along the strip: a wavy rope from X0 to X1, burnt from the start up to T (0..1), the
 * spark crackling at the burn's head (sunflower with a white core in a tangerine glow). Lighter than the booth's rope (a point
 * every 16 units, square ends): the strip draws over a busy table (§10).
 */
void AutoIgFuse(Canvas& Cv, float X0, float X1, float Y, double T, double Now)
{
	const int N = std::max(8, static_cast<int>((X1 - X0) / 16.0f));
	const float Tt = static_cast<float>(Clamp01(T));
	const Vec2 Head{X0 + (X1 - X0) * Tt, Y + 2.5f * std::sin(Tt * 14.0f)};
	std::vector<Vec2> Burnt;
	std::vector<Vec2> Rope{Head};
	for (int I = 0; I <= N; ++I)
	{
		const float U = static_cast<float>(I) / static_cast<float>(N);
		const Vec2 At{X0 + (X1 - X0) * U, Y + 2.5f * std::sin(U * 14.0f)};
		if (U < Tt)
		{
			Burnt.push_back(At);
		}
		else if (U > Tt)
		{
			Rope.push_back(At);
		}
	}
	Burnt.push_back(Head);
	if (Burnt.size() >= 2)
	{
		Cv.StrokePolyline(Burnt, false, Hex(0x5a4060), 3.0f);
	}
	if (Rope.size() >= 2)
	{
		Cv.StrokePolyline(Rope, false, Hex(0xc9a46a), 3.0f);
	}
	if (Tt < 1.0f)
	{
		const float Flick = 0.8f + 0.2f * static_cast<float>(std::sin(Now * 23.0));
		Cv.GlowRoundRect({Head.X - 9.0f, Head.Y - 9.0f, 18.0f, 18.0f}, 9.0f, Hex(rlsites_detail::SitesIgTang, 0.45f * Flick), 8.0f);
		rlsites_detail::SitesSpark(Cv, Head.X, Head.Y, 8.0f * Flick, Hex(rlsites_detail::SitesIgSun));
		rlsites_detail::SitesSpark(Cv, Head.X, Head.Y, 3.5f, Hex(0xffffff));
	}
}

/** RunItTwice's terminal bar (Composure's, §3.4): 20 square cells, as many lit in Col as Frac says, the rest Panel2. */
void AutoRitCells(Canvas& Cv, const Rect& R, float Frac, const Color& Col)
{
	const int Cells = 20;
	const float Cw = (R.W - static_cast<float>(Cells - 1)) / static_cast<float>(Cells);
	const int Lit = static_cast<int>(JsRound(static_cast<double>(std::min(1.0f, std::max(0.0f, Frac))) * Cells));
	for (int I = 0; I < Cells; ++I)
	{
		Cv.FillRect({R.X + static_cast<float>(I) * (Cw + 1.0f), R.Y, Cw, R.H}, I < Lit ? Col : pal::Panel2);
	}
}

/** RunItTwice's timer (the data card's, §3.5): a 2-unit Signal Red line over a hairline, draining, with an LED Red head. */
void AutoRitTimer(Canvas& Cv, float X, float Y, float W, float Frac)
{
	Cv.FillRect({X, Y, W, 1.0f}, pal::Line);
	const float Fw = W * std::min(1.0f, std::max(0.0f, Frac));
	if (Fw > 0.5f)
	{
		Cv.FillRect({X, Y - 0.5f, Fw, 2.0f}, pal::Accent);
		Cv.FillRect({std::max(X, X + Fw - 4.0f), Y - 1.5f, std::min(4.0f, Fw), 4.0f}, Hex(rlsites_detail::SitesRitLed));
	}
}

/** What a lit choice is lit with: the accent (a watch speed, a target), the second accent (SKIP, a mode of its own). */
enum class AutoLit : uint8_t
{
	Accent,
	Second,
};

/**
 * A choice in a rival's language (a watch speed, SKIP, a target): Ignite's candy pill (lit: the light with a gloss and plum
 * ink); RunItTwice's slab cut 6 (Carbon under a hairline; lit Signal Red with its LED top edge and Hot White type, SKIP in
 * the chrome ramp with Onyx type), the label in the look's face (RunItTwice's in caps), a size down rather than cut.
 * Returns true when clicked.
 */
bool AutoRivalChip(Ui& UI, Canvas& Cv, const rlsites_detail::SiteLook& L, const std::string& Id, const Rect& R, const std::string& Label, bool Lit, AutoLit Light, bool Enabled, float Size)
{
	const Ui::ClickState St = UI.Clickable(Id, R, Enabled);
	const bool Hover = St.Hover && Enabled;
	const float A0 = Cv.GetAlpha();
	Cv.SetAlpha(A0 * (Enabled ? 1.0f : 0.4f));
	Color Ink = Hover ? pal::Ink : pal::Muted;
	if (L.Cut > 0.0f)
	{
		const float Cut = std::min(R.H / 2.0f, 6.0f);
		const std::vector<Vec2> Path = rlsites_detail::SitesCutPath(R, Cut);
		if (Lit && Light == AutoLit::Accent)
		{
			Cv.FillPolygon(Path, Hover ? Hex(0xd62c31) : pal::Accent);
			Cv.FillRect({R.X, R.Y, R.W - Cut, 1.0f}, Hex(rlsites_detail::SitesRitLed));
			Ink = pal::Gold;
		}
		else if (Lit)
		{
			Cv.FillPolygon(Path, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, Hex(0xf3f4f6), Hex(0x8d929c)));
			Ink = Hex(rlsites_detail::SitesRitOnyx);
		}
		else
		{
			Cv.FillPolygon(Path, Hover ? pal::Panel2 : pal::Panel);
			Cv.StrokePolyline(Path, true, Hover ? Hex(rlsites_detail::SitesRitChrome, 0.35f) : pal::Line, 1.0f);
		}
	}
	else if (Lit)
	{
		AutoIgPill(Cv, R, Light == AutoLit::Second ? pal::Accent2 : pal::Accent, pal::Line, 0.0f, true);
		Ink = AutoIgInk();
	}
	else
	{
		// A plain candy well; its rim lights on hover (the rows of them stay light on vertices, §10).
		AutoIgPill(Cv, R, Hover ? pal::Line : pal::Panel2, pal::Accent, Hover ? 1.2f : 0.0f, false);
	}
	const std::string Text = AutoCaps(L, Label);
	Cv.Text(Text, R.X + R.W / 2.0f, R.Y + R.H / 2.0f + 1.0f, AutoFit(Cv, Text, AutoFace(L, Size, Ink, Align::Center, Baseline::Middle), R.W - 12.0f));
	Cv.SetAlpha(A0);
	return St.Clicked && Enabled;
}

/** AutoRivalChip's width for its label at Size. */
float AutoRivalChipW(const Canvas& Cv, const rlsites_detail::SiteLook& L, const std::string& Label, float Size)
{
	return AutoWidth(Cv, AutoCaps(L, Label), AutoFace(L, Size, pal::Ink)) + 28.0f;
}

/** A stop rule's label as a rival draws it: RunItTwice's BoldCondensed caps, Ignite's Bold. */
TextStyle AutoToggleStyle(const rlsites_detail::SiteLook& L, const Color& Col)
{
	return L.Cut > 0.0f ? AutoFace(L, 14.0f, Col, Align::Left, Baseline::Middle) : Ts(13.0f, 700, Col, Align::Left, Baseline::Middle);
}

/** AutoRivalToggle's width for its label. */
float AutoRivalToggleW(const Canvas& Cv, const rlsites_detail::SiteLook& L, const std::string& Label)
{
	return AutoWidth(Cv, AutoCaps(L, Label), AutoToggleStyle(L, pal::Ink)) + 46.0f;
}

/**
 * A stop rule in a rival's language: Ignite's pill with a round check bubble (on: a tint of the accent, the bubble lit with a
 * plum check); RunItTwice's slab cut 6 with a square box (on: edged in Signal Red light, the box lit red with a Hot White
 * vector check). Returns true when clicked.
 */
bool AutoRivalToggle(Ui& UI, Canvas& Cv, const rlsites_detail::SiteLook& L, const std::string& Id, const Rect& R, const std::string& Label, bool On, bool Enabled)
{
	const Ui::ClickState St = UI.Clickable(Id, R, Enabled);
	const bool Hover = St.Hover && Enabled;
	const float A0 = Cv.GetAlpha();
	Cv.SetAlpha(A0 * (Enabled ? 1.0f : 0.45f));
	const float Cy = R.Y + R.H / 2.0f;
	if (L.Cut > 0.0f)
	{
		const std::vector<Vec2> Path = rlsites_detail::SitesCutPath(R, std::min(R.H / 2.0f, 6.0f));
		Cv.FillPolygon(Path, On || Hover ? pal::Panel2 : pal::Panel);
		Cv.StrokePolyline(Path, true, On ? rlnet_detail::NetA(pal::Accent, 0.8f) : Hover ? Hex(rlsites_detail::SitesRitChrome, 0.35f) : pal::Line, 1.0f);
		const Rect Box{R.X + 10.0f, Cy - 8.0f, 16.0f, 16.0f};
		if (On)
		{
			Cv.FillRect(Box, pal::Accent);
			rlnet_detail::NetCheck(Cv, Box.X + 8.0f, Box.Y + 8.0f, 10.0f, pal::Gold);
		}
		else
		{
			Cv.FillRect(Box, pal::Bg);
			Cv.StrokePolyline({{Box.X + 0.5f, Box.Y + 0.5f}, {Box.X + Box.W - 0.5f, Box.Y + 0.5f}, {Box.X + Box.W - 0.5f, Box.Y + Box.H - 0.5f}, {Box.X + 0.5f, Box.Y + Box.H - 0.5f}}, true,
				Hex(rlsites_detail::SitesRitChrome, 0.5f), 1.0f);
		}
	}
	else
	{
		AutoIgPill(Cv, R, On ? Mix(pal::Panel2, pal::Accent, 0.22f) : Hover ? pal::Line : pal::Panel2, On ? pal::Accent : pal::Line, On ? 1.5f : 1.2f, false);
		const float Bx = R.X + 18.0f;
		Cv.FillCircle(Bx, Cy, 9.0f, On ? pal::Accent : Hex(rlsites_detail::SitesIgNight));
		if (On)
		{
			rlnet_detail::NetCheck(Cv, Bx, Cy, 10.0f, AutoIgInk());
		}
		else
		{
			Cv.StrokeEllipse(Bx, Cy, 9.0f, 9.0f, pal::Muted, 1.2f);
		}
	}
	const std::string Text = AutoCaps(L, Label);
	Cv.Text(Text, R.X + 34.0f, Cy + 0.5f, AutoFit(Cv, Text, AutoToggleStyle(L, On ? pal::Ink : Hover ? pal::Ink : pal::Muted), R.W - 42.0f));
	Cv.SetAlpha(A0);
	return St.Clicked && Enabled;
}

/** A rival button's role: go (watch, skip, take the wheel), gold (Ignite's sunflower: the skip, Take over), danger, quiet. */
enum class AutoRole : uint8_t
{
	Go,
	Gold,
	Danger,
	Quiet,
};

/** The role a RiverLine button kind plays at a rival's table (the panel's main button keeps its kinds' meanings). */
AutoRole AutoRoleOf(ButtonKind Kind)
{
	switch (Kind)
	{
	case ButtonKind::Secondary:
	case ButtonKind::Ghost: return AutoRole::Quiet;
	case ButtonKind::Danger: return AutoRole::Danger;
	case ButtonKind::Gold: return AutoRole::Gold;
	case ButtonKind::Primary: break;
	}
	return AutoRole::Go;
}

/**
 * A button in a rival's language: Ignite's pill (go: the sunset with its glow; gold: sunflower; both with a gloss and plum
 * ink; danger: grape with cream; quiet: Panel2 with a rim) in BlackItalic; RunItTwice's slab cut 8 (go and gold: the Signal
 * Red slab with its LED edge, Hot White type and the light sweep on hover; danger: Carbon edged in Signal Red light; quiet:
 * Carbon under a chrome hairline) in BoldCondensed caps. A size down rather than cut. Returns true when clicked.
 */
bool AutoRivalButton(Ui& UI, Canvas& Cv, const rlsites_detail::SiteLook& L, const igniteart::IgniteTones& Tones, const std::string& Id, const Rect& R, const std::string& Label, AutoRole Role,
	bool Enabled, float Size, double Now)
{
	const Ui::ClickState St = UI.Clickable(Id, R, Enabled);
	const bool Hover = St.Hover && Enabled;
	const Rect B{R.X, R.Y + (St.Down ? 2.0f : 0.0f), R.W, R.H};
	const float A0 = Cv.GetAlpha();
	Cv.SetAlpha(A0 * (Enabled ? 1.0f : 0.35f));
	Color Ink = pal::Ink;
	if (L.Cut > 0.0f)
	{
		const float Cut = std::min(R.H / 2.0f, L.Cut * 0.8f);
		if (Role == AutoRole::Go || Role == AutoRole::Gold)
		{
			rlrit_detail::RitSlab(Cv, B, Cut, Hover, Now);
			Ink = pal::Gold; // Hot White on Signal Red, 5.45:1
		}
		else
		{
			const std::vector<Vec2> Path = rlsites_detail::SitesCutPath(B, Cut);
			if (Role == AutoRole::Danger && Hover)
			{
				Cv.GlowRoundRect({B.X, B.Y + 6.0f, B.W, B.H - 6.0f}, 0.0f, Hex(rlsites_detail::SitesRitRed, 0.3f), 10.0f);
			}
			Cv.FillPolygon(Path, Hover ? pal::Panel2 : pal::Panel);
			Cv.StrokePolyline(Path, true, Role == AutoRole::Danger ? pal::Accent : Hex(rlsites_detail::SitesRitChrome, Hover ? 0.6f : 0.35f), 1.0f);
		}
	}
	else
	{
		switch (Role)
		{
		case AutoRole::Go:
		{
			const Color To = Hex(Tones.Night ? Tones.CoralHi : rlsites_detail::SitesIgPink);
			Cv.GlowRoundRect({B.X + 10.0f, B.Y + B.H - 14.0f, B.W - 20.0f, 16.0f}, 8.0f, rlnet_detail::NetA(To, Hover ? 0.6f : 0.45f), 12.0f);
			AutoIgPill(Cv, B, Paint::Linear({B.X, 0.0f}, {B.X + B.W, 0.0f}, Hex(Hover ? Tones.CoralHi : Tones.Coral), To), pal::Line, 0.0f, true);
			Ink = AutoIgInk();
			break;
		}
		case AutoRole::Gold:
			AutoIgPill(Cv, B, Hover ? Mix(pal::Gold, Hex(0xffffff), 0.15f) : pal::Gold, pal::Line, 0.0f, true);
			Ink = AutoIgInk();
			break;
		case AutoRole::Danger: AutoIgPill(Cv, B, Hex(Hover ? 0x4a2a72 : 0x3a1f5c), Hex(0x6a4a9a), 2.0f, false); break;
		case AutoRole::Quiet: AutoIgPill(Cv, B, Hover ? pal::Line : pal::Panel2, Hover ? pal::Accent : pal::Line, 1.2f, false); break;
		}
	}
	const std::string Text = AutoCaps(L, Label);
	Cv.Text(Text, B.X + B.W / 2.0f, B.Y + B.H / 2.0f + 1.0f, AutoFit(Cv, Text, AutoFace(L, Size, Ink, Align::Center, Baseline::Middle), B.W - 24.0f));
	Cv.SetAlpha(A0);
	return St.Clicked && Enabled;
}

/** The action bar's roles, for the buttons that take the wheel during the plan's think. */
enum class AutoAct : uint8_t
{
	Fold,
	Call,
	Raise,
};

/**
 * A button that takes the wheel, where the player's own sits and as big (the look's ButtonH), in the look's language but
 * hollow: Ignite's pill ringed in its role's light (FOLD grape, CALL sky, RAISE the accent) over a tint; RunItTwice's slab cut
 * 8 on Carbon (RAISE edged in Signal Red light with its LED top edge, the others in hairlines; Chrome type). The label (and
 * CALL's amount) where the player's own button prints them (RiverLine.cpp's TblActionButton). Returns true when clicked.
 */
bool AutoRivalGhost(Ui& UI, Canvas& Cv, const rlsites_detail::SiteLook& L, const std::string& Id, const Rect& R, const std::string& Label, const std::string& Sub, AutoAct Role)
{
	const Ui::ClickState St = UI.Clickable(Id, R);
	const Rect B{R.X, R.Y + (St.Down ? 2.0f : 0.0f), R.W, R.H};
	const bool Slab = L.Cut > 0.0f;
	Color Ink = pal::Ink;
	if (Slab)
	{
		const float Cut = std::min(R.H / 2.0f, L.Cut * 0.8f);
		const std::vector<Vec2> Path = rlsites_detail::SitesCutPath(B, Cut);
		if (Role == AutoAct::Raise && St.Hover)
		{
			Cv.GlowRoundRect({B.X, B.Y + 8.0f, B.W, B.H - 8.0f}, 0.0f, Hex(rlsites_detail::SitesRitRed, 0.3f), 12.0f);
		}
		Cv.FillPolygon(Path, St.Hover ? pal::Panel2 : pal::Panel);
		Cv.StrokePolyline(Path, true, Role == AutoAct::Raise ? pal::Accent : Role == AutoAct::Call ? Hex(rlsites_detail::SitesRitChrome, 0.6f) : pal::Line, 1.0f);
		if (Role == AutoAct::Raise)
		{
			Cv.FillRect({B.X, B.Y, B.W - Cut, 1.0f}, Hex(rlsites_detail::SitesRitLed));
		}
		Ink = Role == AutoAct::Raise ? pal::Ink : pal::Accent2;
	}
	else
	{
		const Color Col = Role == AutoAct::Fold ? Hex(0x6a4a9a) : Role == AutoAct::Call ? pal::Accent2 : pal::Accent;
		AutoIgPill(Cv, B, rlnet_detail::NetA(Col, St.Hover ? 0.24f : 0.1f), Col, 2.0f, false);
		Ink = Role == AutoAct::Fold ? pal::Ink : Col;
	}
	TextStyle Big = AutoFace(L, Slab ? 20.0f : 22.0f, Ink, Align::Center, Sub.empty() ? Baseline::Middle : Baseline::Alphabetic);
	Big.Weight = 900;
	const std::string Caption = rlnet_detail::NetUpper(Label);
	const float Cy = B.Y + B.H / 2.0f;
	if (Sub.empty())
	{
		Cv.Text(Caption, B.X + B.W / 2.0f, Cy + 1.0f, Big);
	}
	else
	{
		Cv.Text(Caption, B.X + B.W / 2.0f, Cy - 3.0f, Big);
		TextStyle Amount = Big;
		Amount.Size = Slab ? 15.0f : 16.0f;
		Cv.Text(Sub, B.X + B.W / 2.0f, Cy + 18.0f, Amount);
	}
	return St.Clicked;
}

/**
 * A style's tile in a rival's language (AutoTileRect's rect; the press is the caller's): Ignite's round card in the hour's
 * tones (picked: a tint of the accent in its neon, a PLAYING pill on the top edge), the name in BlackItalic and the swing as
 * a candy pill; RunItTwice's slab cut 6 under a gloss band and a chrome hairline (picked: the Board's selection, Signal Red at
 * 0.16 with a 4-unit red bar, an LED top edge and a red underglow; a red tag), the name in BoldCondensed caps and the swing as
 * a cut tag (Wild lit red with Hot White: red is light, never ink).
 */
void AutoRivalTile(Ui& UI, Canvas& Cv, const rlsites_detail::SiteLook& L, const igniteart::IgniteTones& Tones, const Rect& T, const style::Info& I, const std::string& Why, bool Picked, bool Hover,
	bool On)
{
	const bool Greyed = !Why.empty();
	const bool Lit = Picked && !Greyed;
	const bool Rit = L.Cut > 0.0f;
	const std::string Swing = rlnet_detail::NetUpper(style::SwingName(I.Var));
	if (Rit)
	{
		const float Cut = 6.0f;
		const std::vector<Vec2> Path = rlsites_detail::SitesCutPath(T, Cut);
		if (Lit)
		{
			rlrit_detail::RitGlow(Cv, {T.X, T.Y + T.H - 8.0f, T.W, 10.0f}, rlrit_detail::RitRed, 0.3f, 8.0f);
		}
		Cv.FillPolygon(Path, Lit ? Hex(rlrit_detail::RitSelRow) : Hover ? pal::Panel2 : pal::Panel);
		Cv.FillPolygon(rlsites_detail::SitesCutPath({T.X, T.Y, T.W, 34.0f}, Cut), Paint::Linear({0.0f, T.Y}, {0.0f, T.Y + 34.0f}, Hex(rlrit_detail::RitGloss, 0.9f), Hex(rlrit_detail::RitGloss, 0.0f)));
		Cv.StrokePolyline(Path, true, Lit ? rlnet_detail::NetA(pal::Accent, 0.7f) : Hex(rlsites_detail::SitesRitChrome, Hover ? 0.5f : 0.3f), 1.0f);
		if (Lit)
		{
			Cv.FillRect({T.X, T.Y, 4.0f, T.H}, pal::Accent);
			Cv.FillRect({T.X, T.Y, T.W - Cut, 1.0f}, Hex(rlsites_detail::SitesRitLed));
		}
		const bool Wild = I.Var == style::Swing::Wild;
		const uint32_t Col = I.Var == style::Swing::Steady ? rlrit_detail::RitUp : I.Var == style::Swing::Even ? rlrit_detail::RitChrome : I.Var == style::Swing::Swingy ? rlrit_detail::RitAmber : rlrit_detail::RitRed;
		const float SwW = rlrit_detail::RitMeasure(Cv, Swing, 10.0f) + 10.0f;
		rlrit_detail::RitTag(Cv, T.X + T.W - 14.0f - SwW, T.Y + 12.0f, Swing, Col, Wild ? rlrit_detail::RitWhite : Col, 10.0f, 5.0f, 16.0f, 4.0f, !Wild);
		rlrit_detail::RitText(Cv, rlnet_detail::NetUpper(I.Name), T.X + 16.0f, T.Y + 31.0f, 19.0f, pal::Ink, rlrit_detail::RitFace::Cond, Align::Left, Baseline::Alphabetic, T.W - 40.0f - SwW);
	}
	else
	{
		const float Rr = L.Radius;
		if (Lit)
		{
			Cv.GlowRoundRect(T, Rr, rlnet_detail::NetA(pal::Accent, 0.35f), 14.0f);
		}
		const Color Top = Lit ? Mix(Hex(Tones.PanelTop), pal::Accent, 0.16f) : Hover ? Hex(Tones.CabTop) : Hex(Tones.PanelTop);
		Cv.FillRoundRect(T, Rr, Paint::Linear({0.0f, T.Y}, {0.0f, T.Y + T.H}, Top, Hex(Tones.PanelBot)));
		Cv.StrokeRoundRect(T, Rr, Lit ? pal::Accent : Hover ? Hex(Tones.RimHi) : Hex(Tones.Rim), Lit ? 2.0f : 1.0f);
		const TextStyle Sw = AutoFace(L, 9.5f, AutoIgInk(), Align::Center, Baseline::Middle);
		const float SwW = AutoWidth(Cv, Swing, Sw) + 14.0f;
		const Rect Chip{T.X + T.W - 14.0f - SwW, T.Y + 12.0f, SwW, 18.0f};
		AutoIgPill(Cv, Chip, AutoSwingColor(I.Var), pal::Line, 0.0f, false);
		Cv.Text(Swing, Chip.X + Chip.W / 2.0f, Chip.Y + Chip.H / 2.0f + 0.5f, Sw);
		TextStyle Name = AutoFace(L, 18.0f, Lit ? pal::Ink : Mix(pal::Ink, pal::Muted, 0.2f));
		Name.MaxWidth = T.W - 40.0f - SwW;
		UI.Text(I.Name, T.X + 16.0f, T.Y + 31.0f, Name);
	}
	rlnet_detail::NetParagraph(Cv, I.Edge, T.X + 16.0f, T.Y + 50.0f, T.W - 32.0f, AutoEdgeSize, 500, rlsites_detail::SitesTone(0xb6c2d4), AutoEdgeLine, AutoEdgeLines);
	if (Greyed)
	{
		if (Rit)
		{
			rlrit_detail::RitLock(Cv, T.X + 16.0f, T.Y + T.H - 27.0f, pal::Orange);
		}
		else
		{
			rlnet_detail::NetLockIcon(Cv, T.X + 15.0f, T.Y + T.H - 25.0f, 13.0f, pal::Orange);
		}
		UI.Text(Why, T.X + 34.0f, T.Y + T.H - 14.0f, Ts(12.0f, 700, pal::Orange, Align::Left, Baseline::Alphabetic, false, T.W - 50.0f));
	}
	else
	{
		// BEST IN in the look's face, then where (a long one carries on below, under the label: never cut short).
		const TextStyle Kick = AutoFace(L, Rit ? 11.0f : 9.5f, pal::Muted);
		const float Lw = AutoWidth(Cv, "BEST IN", Kick);
		const std::pair<std::string, std::string> Best = AutoSplitAt(Cv, I.BestIn, T.W - 40.0f - Lw, 12.0f, 600);
		const float Ly = T.Y + T.H - (Best.second.empty() ? 14.0f : 28.0f);
		Cv.Text("BEST IN", T.X + 16.0f, Ly, Kick);
		UI.Text(Best.first, T.X + 24.0f + Lw, Ly, Ts(12.0f, 600, pal::Ink, Align::Left, Baseline::Alphabetic, false, T.W - 40.0f - Lw));
		if (!Best.second.empty())
		{
			UI.Text(Best.second, T.X + 16.0f, Ly + 14.0f, Ts(12.0f, 600, pal::Ink, Align::Left, Baseline::Alphabetic, false, T.W - 32.0f));
		}
	}
	if (Lit)
	{
		// On the tile's top edge, so it never sits on the tile's words.
		const std::string Tag = On ? std::string("PLAYING") : std::string("PICKED");
		if (Rit)
		{
			rlrit_detail::RitTag(Cv, T.X + 16.0f, T.Y - 9.0f, Tag, rlrit_detail::RitRed, rlrit_detail::RitWhite, 10.0f, 6.0f, 18.0f, 4.0f);
		}
		else
		{
			const TextStyle Ts9 = AutoFace(L, 9.5f, AutoIgInk(), Align::Center, Baseline::Middle);
			const float Pw = AutoWidth(Cv, Tag, Ts9) + 30.0f;
			const Rect Pill{T.X + 14.0f, T.Y - 10.0f, Pw, 20.0f};
			AutoIgPill(Cv, Pill, pal::Accent, pal::Line, 0.0f, true);
			rlsites_detail::SitesSpark(Cv, Pill.X + 11.0f, Pill.Y + 10.0f, 6.0f, AutoIgInk());
			Cv.Text(Tag, Pill.X + Pill.W / 2.0f + 6.0f, Pill.Y + 10.5f, Ts9);
		}
	}
}

/**
 * The skip's runner (a skip promises no end: the runner says it's moving, not how far): Ignite's Night track with a candy
 * pill and a cream spark at its head; RunItTwice's hairline with a Signal Red bar, its LED core and a trail (the scan line's).
 */
void AutoSkipRunner(Canvas& Cv, bool Rit, const Rect& Track, float At, float Len, const Color& Tail)
{
	const float X = Track.X + At * (Track.W - Len);
	const float Mid = Track.Y + Track.H / 2.0f;
	if (Rit)
	{
		Cv.FillRect({Track.X, Mid - 0.5f, Track.W, 1.0f}, pal::Line);
		const float T0 = std::max(Track.X, X - 32.0f);
		if (X > T0)
		{
			Cv.FillRect({T0, Mid - 1.5f, X - T0, 3.0f}, Paint::Linear({T0, 0.0f}, {X, 0.0f}, rlnet_detail::NetA(pal::Accent, 0.0f), rlnet_detail::NetA(pal::Accent, 0.22f)));
		}
		Cv.FillRect({X, Mid - 1.5f, Len, 3.0f}, pal::Accent);
		Cv.FillRect({X, Mid - 0.5f, Len, 1.0f}, Hex(rlsites_detail::SitesRitLed));
		return;
	}
	Cv.FillRoundRect(Track, Track.H / 2.0f, Hex(rlsites_detail::SitesIgNight));
	Cv.StrokeRoundRect(Track, Track.H / 2.0f, pal::Line, 1.0f);
	Cv.FillRoundRect({X, Track.Y, Len, Track.H}, Track.H / 2.0f, Paint::Linear({X, 0.0f}, {X + Len, 0.0f}, pal::Accent, Tail));
	rlsites_detail::SitesSpark(Cv, X + Len - 2.0f, Mid, std::max(7.0f, Track.H * 1.3f), pal::Ink);
}

/**
 * The skip card in a rival's language (§2.6.1's card: SKIPPING · <STYLE> over the dimmed table, where the skip is, its
 * buttons and keys; a tile's short form): Ignite's neon BlackItalic title over Night with a candy runner and pills;
 * RunItTwice's twice-printed BoldCondensed over Tape Black, its line and keys in Mono, the scan-line runner and slabs. A size
 * down, never cut. The presses are RiverLine's, under the same ids.
 */
void AutoRivalSkip(Ui& UI, Canvas& Cv, Session& S, const rlsites_detail::SiteLook& L, const Tournament& T, const autoplay::Panel& P, int Table, const Rect& R, double Now, double World)
{
	const bool Rit = L.Cut > 0.0f;
	const bool Compact = R.W < 800.0f;
	const igniteart::IgniteTones Tones = AutoIgTones(World);
	const Color Tail = Hex(Tones.Night ? Tones.CoralHi : rlsites_detail::SitesIgPink);
	const std::pair<std::string, std::string> Line = AutoSplitSkipLine(P.SkipLine.empty() ? std::string("SKIPPING") : P.SkipLine);
	const float Cx = R.X + R.W / 2.0f;
	const float Bar = static_cast<float>(std::fmod(Now * 0.8, 1.0));
	// The felt dims in the look's scrim (Night; Tape Black, RunItTwice's rail left lit beside a table).
	const float Left = Rit && !Compact && !AutoTiled(S) ? std::max(R.X, AutoRitRail) : R.X;
	Cv.FillRect({Left, R.Y, R.X + R.W - Left, R.H}, Hex(Rit ? rlsites_detail::SitesRitTape : rlsites_detail::SitesIgNight, Rit ? 0.8f : 0.78f));
	if (Compact)
	{
		const float Cy = R.Y + R.H / 2.0f;
		const TextStyle Head = AutoFace(L, 24.0f, Rit ? pal::Ink : pal::Accent, Align::Center);
		const TextStyle Sub = Rit ? AutoMono(13.0f, pal::Ink, Align::Center) : Ts(14.0f, 700, pal::Ink, Align::Center);
		const float Pw = std::min(R.W - 24.0f, std::max(380.0f, std::max(AutoWidth(Cv, Line.first, Head), AutoWidth(Cv, Line.second, Sub)) + 80.0f));
		const Rect Plate{Cx - Pw / 2.0f, Cy - 58.0f, Pw, 140.0f};
		if (Rit)
		{
			rlrit_detail::RitPanel(Cv, Plate, L.Cut, rlrit_detail::RitTape, rlrit_detail::RitHair);
			Cv.FillRect({Plate.X, Plate.Y, Plate.W - L.Cut, 3.0f}, pal::Accent);
		}
		else
		{
			Cv.GlowRoundRect(Plate, L.Radius, rlnet_detail::NetA(pal::Accent, 0.25f), 14.0f);
			Cv.FillRoundRect(Plate, L.Radius, Hex(rlsites_detail::SitesIgNight, 0.94f));
			Cv.StrokeRoundRect(Plate, L.Radius, rlnet_detail::NetA(pal::Accent, 0.7f), 1.5f);
		}
		UI.Text(Line.first, Cx, Cy - 24.0f, AutoFit(Cv, Line.first, Head, Pw - 40.0f));
		UI.Text(Line.second, Cx, Cy + 2.0f, AutoFit(Cv, Line.second, Sub, Pw - 40.0f));
		AutoSkipRunner(Cv, Rit, {Cx - 160.0f, Cy + 18.0f, 320.0f, Rit ? 5.0f : 6.0f}, Bar, 64.0f, Tail);
		if (AutoRivalButton(UI, Cv, L, Tones, "autoskipstop" + T.Spec.Id, {Cx - 70.0f, Cy + 40.0f, 140.0f, 32.0f}, "Stop", AutoRole::Quiet, true, 13.0f, Now))
		{
			S.StopSkip(Table);
		}
		return;
	}
	if (Rit)
	{
		// The twice print (§3.3): Hot White over its Signal Red under-copy; the card's one twin stripe under it; the line in Mono.
		rlrit_detail::RitPanelBegin();
		const float Sz = rlrit_detail::RitFit(Cv, Line.first, 44.0f, 28.0f, R.W - 80.0f);
		if (rlrit_detail::RitMeasure(Cv, Line.first, Sz) <= R.W - 80.0f)
		{
			rlrit_detail::RitTwice(Cv, Line.first, Cx, R.Y + 266.0f, Sz, pal::Gold, rlrit_detail::RitRed, Align::Center);
		}
		else
		{
			UI.Text(Line.first, Cx, R.Y + 266.0f, AutoFit(Cv, Line.first, AutoFace(L, 28.0f, pal::Gold, Align::Center), R.W - 80.0f));
		}
		rlrit_detail::RitTwinH(Cv, Cx - 120.0f, R.Y + 281.0f, 240.0f, 0.6f);
		const TextStyle Ls = AutoFit(Cv, Line.second, AutoMono(17.0f, pal::Ink, Align::Center), R.W - 80.0f);
		if (!Line.second.empty())
		{
			// A rival's board sits 25 units higher than RiverLine's (its cards from 296 under the card's top): the line would print
			// on the cards, so it prints on a strip of tape (review-fix2-auto).
			const float Lw = AutoWidth(Cv, Line.second, Ls);
			Cv.FillPolygon(rlsites_detail::SitesCutPath({Cx - Lw / 2.0f - 16.0f, R.Y + 299.0f, Lw + 32.0f, 27.0f}, 6.0f), Hex(rlsites_detail::SitesRitTape, 0.9f));
		}
		UI.Text(Line.second, Cx, R.Y + 318.0f, Ls);
	}
	else
	{
		// The neon: the title in BlackItalic in the accent over its glow, the line in Bold cream.
		Cv.GlowRoundRect({Cx - 280.0f, R.Y + 224.0f, 560.0f, 60.0f}, 30.0f, rlnet_detail::NetA(pal::Accent, 0.22f), 34.0f);
		UI.Text(Line.first, Cx, R.Y + 266.0f, AutoFit(Cv, Line.first, AutoFace(L, 44.0f, pal::Accent, Align::Center), R.W - 80.0f));
		const TextStyle Ls = AutoFit(Cv, Line.second, Ts(19.0f, 700, pal::Ink, Align::Center), R.W - 80.0f);
		if (!Line.second.empty())
		{
			// As at RunItTwice, the board's top lies under the line: it prints on a Night pill (review-fix2-auto).
			const float Lw = AutoWidth(Cv, Line.second, Ls);
			Cv.FillRoundRect({Cx - Lw / 2.0f - 18.0f, R.Y + 286.0f, Lw + 36.0f, 28.0f}, 14.0f, Hex(rlsites_detail::SitesIgNight, 0.9f));
		}
		UI.Text(Line.second, Cx, R.Y + 306.0f, Ls);
	}
	const Level& Lv = T.CurrentLevel();
	const std::pair<std::string, std::string> Stats[4] = {
		{"Level", std::to_string(T.LevelIndex + 1) + " \xC2\xB7 " + ChipsText(static_cast<double>(Lv.Sb)) + "/" + ChipsText(static_cast<double>(Lv.Bb))},
		{"Players left", ChipsText(T.Remaining)},
		{"Your stack", ChipsText(static_cast<double>(T.Hero().Stack)) + " (" + std::to_string(static_cast<int>(JsRound(static_cast<double>(T.Hero().Stack) / static_cast<double>(Lv.Bb)))) + " BB)"},
		{"Your rank", Ordinal(T.HeroRank())},
	};
	for (int I = 0; I < 4; ++I)
	{
		const float X = Cx - 330.0f + static_cast<float>(I) * 220.0f;
		const std::string Label = Rit ? rlnet_detail::NetUpper(Stats[I].first) : Stats[I].first;
		UI.Text(Label, X + 100.0f, R.Y + 386.0f, Rit ? AutoFace(L, 14.0f, pal::Muted, Align::Center) : Ts(15.0f, 700, pal::Muted, Align::Center));
		UI.Text(Stats[I].second, X + 100.0f, R.Y + 420.0f, AutoFit(Cv, Stats[I].second, AutoFace(L, Rit ? 26.0f : 24.0f, pal::Ink, Align::Center), 210.0f));
	}
	AutoSkipRunner(Cv, Rit, {Cx - 300.0f, R.Y + 466.0f, 600.0f, Rit ? 8.0f : 10.0f}, Bar, 120.0f, Tail);
	// K takes the wheel in two presses (§2.6.5); the button does both, so its words hold. Stop leaves the plan the seat at 1x.
	if (AutoRivalButton(UI, Cv, L, Tones, "stopsprint", {Cx - 250.0f, R.Y + 516.0f, 240.0f, 60.0f}, "Take the wheel", Rit ? AutoRole::Go : AutoRole::Gold, true, 20.0f, Now))
	{
		S.StopSkip(Table);
		S.TakeOver(Table);
	}
	if (AutoRivalButton(UI, Cv, L, Tones, "autoskipstop", {Cx + 10.0f, R.Y + 516.0f, 240.0f, 60.0f}, "Stop", AutoRole::Quiet, true, 20.0f, Now))
	{
		S.StopSkip(Table);
	}
	const std::string Keys = autoplay::Text(autoplay::Words::SkipKeys);
	UI.Text(Keys, Cx, R.Y + 610.0f, AutoFit(Cv, Keys, Rit ? AutoMono(14.0f, pal::Muted, Align::Center) : Ts(14.0f, 600, pal::Muted, Align::Center), R.W - 80.0f));
}

/**
 * The watch strip in a rival's language (RiverLine::AutoStrip's rows, rects, ids and presses): Ignite's candy card (the style
 * as a lit pill, candy speeds, Take over in sunflower, the think as a fuse burning down, the buttons that take the wheel as
 * ringed pills as big as the player's own); RunItTwice's Tape Black slab under its Signal Red edge (the style on a red-barred
 * slab, cut speeds, Take over on the red slab, the think as the data card's draining red line, Chrome slabs for the wheel,
 * Mono for the counts). R.H under 40: the one line over the player's own controls.
 */
void AutoRivalStrip(Ui& UI, Canvas& Cv, Session& S, const RiverLine& RL, const rlsites_detail::SiteLook& L, const autoplay::Panel& P, const style::Info& Info, int Table, const std::string& Stop,
	const Rect& R, double Now, double World)
{
	const bool Rit = L.Cut > 0.0f;
	const igniteart::IgniteTones Tones = AutoIgTones(World);
	if (R.H < 40.0f)
	{
		// Over the player's own controls (a hand taken over, the prompt open): whose hand it is, in one line.
		const std::string Line = !Stop.empty() ? Stop : std::string(P.TakenOver ? AutoYourHandKeys : AutoYourDecision);
		float Tw = 0.0f;
		if (Rit)
		{
			Tw = rlrit_detail::RitTag(Cv, R.X, R.Y + 3.0f, "YOUR HAND", rlrit_detail::RitRed, rlrit_detail::RitWhite, 12.0f, 7.0f, 20.0f, 4.0f);
		}
		else
		{
			const TextStyle Tag = AutoFace(L, 11.0f, AutoIgInk(), Align::Center, Baseline::Middle);
			Tw = AutoWidth(Cv, "YOUR HAND", Tag) + 20.0f;
			AutoIgPill(Cv, {R.X, R.Y + 2.0f, Tw, 22.0f}, pal::Gold, pal::Line, 0.0f, true);
			Cv.Text("YOUR HAND", R.X + Tw / 2.0f, R.Y + 13.5f, Tag);
		}
		const TextStyle St = Rit ? Ts(13.5f, 500, Stop.empty() ? pal::Ink : pal::Gold) : Ts(13.0f, 700, Stop.empty() ? pal::Ink : pal::Gold);
		UI.Text(Line, R.X + Tw + 10.0f, R.Y + 17.0f, AutoFit(Cv, Line, St, R.W - Tw - 10.0f));
		return;
	}
	// The card, 8 units round the rows and as tall as the buttons that take the wheel need (the look's ButtonH: Ignite's 72).
	const Rect Card{R.X - 8.0f, R.Y, R.W + 16.0f, std::max(R.H, 84.0f + L.ButtonH + 8.0f)};
	if (Rit)
	{
		const std::vector<Vec2> Path = rlsites_detail::SitesCutPath(Card, L.Cut);
		Cv.FillPolygon(Path, Hex(rlsites_detail::SitesRitTape, 0.94f));
		Cv.StrokePolyline(Path, true, pal::Line, 1.0f);
		Cv.FillRect({Card.X, Card.Y, Card.W - L.Cut, 3.0f}, pal::Accent);
		Cv.FillRect({Card.X, Card.Y + 1.0f, Card.W - L.Cut, 1.0f}, Hex(rlsites_detail::SitesRitLed));
	}
	else
	{
		// The card in the neon's rim (no glow: the strip draws over a busy table, §10).
		Cv.FillRoundRect(Card, L.Radius, Paint::Linear({0.0f, Card.Y}, {0.0f, Card.Y + Card.H}, Hex(Tones.PanelTop), Hex(Tones.Plum)));
		Cv.StrokeRoundRect(Card, L.Radius, rlnet_detail::NetA(pal::Accent, 0.6f), 1.5f);
	}

	// Row 1: the style (it opens the panel), the speeds, Take over [K].
	const float Ry = R.Y + 10.0f;
	{
		const std::string Name = rlnet_detail::NetUpper(Info.Name);
		const TextStyle Ns = AutoFace(L, Rit ? 15.0f : 13.0f, Rit ? pal::Ink : AutoIgInk(), Align::Left, Baseline::Middle);
		const Rect Chip{R.X + 10.0f, Ry, AutoWidth(Cv, Name, Ns) + (Rit ? 44.0f : 38.0f), 30.0f};
		const Ui::ClickState St = UI.Clickable("autochip", Chip);
		if (Rit)
		{
			const std::vector<Vec2> Path = rlsites_detail::SitesCutPath(Chip, 6.0f);
			Cv.FillPolygon(Path, St.Hover ? pal::Panel2 : pal::Panel);
			Cv.StrokePolyline(Path, true, Hex(rlsites_detail::SitesRitChrome, St.Hover ? 0.6f : 0.35f), 1.0f);
			Cv.FillRect({Chip.X, Chip.Y, 4.0f, Chip.H}, pal::Accent);
			Cv.Text(Name, Chip.X + 16.0f, Chip.Y + 15.5f, Ns);
			rlnet_detail::NetChevron(Cv, Chip.X + Chip.W - 14.0f, Chip.Y + 15.0f, 9.0f, 2, pal::Accent2);
		}
		else
		{
			AutoIgPill(Cv, Chip, St.Hover ? Mix(pal::Accent, Hex(0xffffff), 0.15f) : pal::Accent, pal::Line, 0.0f, true);
			Cv.Text(Name, Chip.X + 14.0f, Chip.Y + 15.5f, Ns);
			rlnet_detail::NetChevron(Cv, Chip.X + Chip.W - 14.0f, Chip.Y + 15.0f, 9.0f, 2, AutoIgInk()); // the panel opens over the table
		}
		if (St.Clicked)
		{
			AutoOpenPanel(&RL, S, Table, Now);
		}
	}
	const float TakeW = 150.0f;
	float Sx = R.X + R.W - 10.0f - TakeW - 12.0f - 4.0f * 44.0f + 4.0f;
	for (const int Speed : {1, 2, 4, 8})
	{
		if (AutoRivalChip(UI, Cv, L, "autostrip" + std::to_string(Speed), {Sx, Ry, 40.0f, 30.0f}, autoplay::SpeedName(Speed), P.Now.Speed == Speed && !P.Skipping, AutoLit::Accent, !P.Skipping, 14.0f) &&
			P.Now.Speed != Speed)
		{
			autoplay::TableAuto A = P.Now;
			A.Speed = Speed;
			A.M = autoplay::Mode::Watch;
			S.SetAutoplay(Table, A);
		}
		Sx += 44.0f;
	}
	{
		const std::string Label = P.TakenOver || P.ResumeWindow ? std::string("Autoplay off [K]") : std::string(autoplay::Text(autoplay::Words::TakeOverKey));
		if (AutoRivalButton(UI, Cv, L, Tones, "autotake", {R.X + R.W - 10.0f - TakeW, Ry, TakeW, 30.0f}, Label, P.TakenOver ? AutoRole::Quiet : AutoRole::Gold, true, 13.5f, Now))
		{
			S.TakeOver(Table);
		}
	}

	// Row 2: the plan's think (never its move), else its last action once played, else whose hand it is.
	const float Ly = R.Y + 64.0f;
	const float X = R.X + 14.0f;
	const float W = R.W - 28.0f;
	const SeatVis* Hero = AutoHeroSeat(S);
	if (P.ResumeWindow)
	{
		const std::string Said = Rit ? rlnet_detail::NetUpper(autoplay::Text(autoplay::Words::Resumes)) : std::string(autoplay::Text(autoplay::Words::Resumes));
		const float Frac = static_cast<float>(Clamp01(P.ResumeLeft / autoplay::ResumeWindowSeconds));
		UI.Text(Said, X, Ly, AutoFit(Cv, Said, AutoFace(L, Rit ? 17.0f : 16.0f, pal::Gold), W));
		if (Rit)
		{
			AutoRitTimer(Cv, X, Ly + 11.0f, W, Frac);
		}
		else
		{
			AutoIgMeter(Cv, {X, Ly + 8.0f, W, 7.0f}, Frac, pal::Gold);
		}
	}
	else if (P.Skipping)
	{
		UI.Text(AutoPending, X, Ly, Ts(15.0f, Rit ? 500 : 700, pal::Accent2));
		UI.Text(P.Status, R.X + R.W - 14.0f, Ly, Rit ? AutoMono(12.5f, pal::Muted, Align::Right) : Ts(12.5f, 700, pal::Muted, Align::Right));
	}
	else if (P.TakenOver)
	{
		const std::string Yours = AutoYourHand;
		UI.Text(Yours, X, Ly, AutoFit(Cv, Yours, Rit ? Ts(15.0f, 700, pal::Gold) : AutoFace(L, 16.0f, pal::Gold), W));
	}
	else if (P.Thinking)
	{
		const float Tw = UI.Text(autoplay::ThinkingLine(P.Now.S), X, Ly, Rit ? Ts(15.0f, 700, pal::Ink) : AutoFace(L, 17.0f, pal::Ink));
		const std::string Hint = AutoThinkHint;
		UI.Text(Hint, R.X + R.W - 14.0f, Ly, AutoFit(Cv, Hint, Ts(12.5f, Rit ? 500 : 700, pal::Muted, Align::Right), W - Tw - 26.0f));
		double Frac = 0.0;
		if (Hero && Hero->ActEnd > Hero->ActStart)
		{
			Frac = Clamp01((Hero->ActEnd - Now) / (Hero->ActEnd - Hero->ActStart));
		}
		if (Rit)
		{
			AutoRitTimer(Cv, X, Ly + 11.0f, W, static_cast<float>(Frac));
		}
		else
		{
			// Ignite's time bank is a fuse (§2.5): it burns down from the start as the think runs out.
			AutoIgFuse(Cv, X, X + W, Ly + 13.0f, 1.0 - Frac, Now);
		}
	}
	else if (!P.LastAction.empty())
	{
		const float Lw = UI.Text("LAST MOVE", X, Ly, AutoFace(L, Rit ? 13.0f : 12.0f, pal::Muted));
		UI.Text(P.LastAction, X + 12.0f + Lw, Ly, AutoFit(Cv, P.LastAction, AutoFace(L, Rit ? 18.0f : 17.0f, pal::Ink), W - 12.0f - Lw));
	}
	else
	{
		const std::string Line = autoplay::StatusLine(P.Now, P.Sharp);
		UI.Text(Line, X, Ly, AutoFit(Cv, Line, Rit ? AutoMono(13.0f, pal::Muted) : Ts(13.5f, 700, pal::Muted), W));
	}

	// Row 3: during the think, the buttons that take the wheel where the player's own sit and as big: the first press takes
	// the wheel and nothing else (§2.6.1), the next one acts. Otherwise why a skip stopped, and what the plan has done here.
	const float By = R.Y + 84.0f;
	if (P.Thinking && S.CurHand && !S.CurHand->bComplete)
	{
		const LegalActions Legal = S.CurHand->GetLegalActions();
		const float Bw = (R.W - 20.0f) / 3.0f;
		const float Bh = L.ButtonH;
		// CALL's amount as the table prints its bets (RunItTwice's in big blinds, unless the player switched them to chips).
		const Chips Bb = std::max<Chips>(1, S.T ? S.T->CurrentLevel().Bb : 1);
		const std::string CallSub = Legal.CanCheck					   ? std::string()
			: rlsites_detail::SitesBetsInBb(RL, L) ? rlsites_detail::SitesBbText(Legal.CallAmount, Bb)
												   : ChipsText(static_cast<double>(Legal.CallAmount));
		bool Take = AutoRivalGhost(UI, Cv, L, "autofold", {R.X, By, Bw, Bh}, "Fold", std::string(), AutoAct::Fold);
		Take = AutoRivalGhost(UI, Cv, L, "autocall", {R.X + Bw + 10.0f, By, Bw, Bh}, Legal.CanCheck ? "Check" : "Call", CallSub, AutoAct::Call) || Take;
		if (Legal.CanRaise)
		{
			Take = AutoRivalGhost(UI, Cv, L, "autoraise", {R.X + (Bw + 10.0f) * 2.0f, By, Bw, Bh}, Legal.IsBet ? "Bet" : "Raise", std::string(), AutoAct::Raise) || Take;
		}
		if (Take)
		{
			S.TakeOver(Table);
		}
		return;
	}
	if (!Stop.empty())
	{
		UI.Text(Stop, X, By + 22.0f, AutoFit(Cv, Stop, Rit ? Ts(14.0f, 700, pal::Gold) : AutoFace(L, 15.0f, pal::Gold), W));
	}
	const std::string Counts = "This event so far: " + Grouped(P.AiDecisions) + (P.AiDecisions == 1 ? " decision" : " decisions") + " by your plan, " + Grouped(P.OwnDecisions) + " by you.";
	UI.Text(Counts, X, By + (Stop.empty() ? 22.0f : 46.0f), AutoFit(Cv, Counts, Rit ? AutoMono(12.5f, pal::Muted) : Ts(13.0f, 600, pal::Muted), W));
}
} // namespace rlauto_detail

// ------------------------------------------------------------------ the panel

void RiverLine::AutoPanel(const Rect& Asked, double Now)
{
	// The table's look (update 3, G1 fix 2): RiverLine's draws the panel as it always has (the Home branches); a rival's in
	// its own language, through the same rects, ids and presses.
	const rlsites_detail::SiteLook& Look = rlsites_detail::SitesTableLook(S);
	const rlauto_detail::AutoLook Lk = rlauto_detail::AutoLookOf(Look);
	const bool Home = Lk == rlauto_detail::AutoLook::Home;
	const bool Rit = Lk == rlauto_detail::AutoLook::Rit;
	const bool Tiled = rlauto_detail::AutoTiled(S);
	// RunItTwice centres the panel on its octagon (RiverLineRit.cpp), 28 units over its rail: it moves clear of the rail.
	const float Clear = rlauto_detail::AutoRitRail + 8.0f;
	const Rect R = Rit && !Tiled && Asked.X < Clear ? Rect{Clear, Asked.Y, Asked.W, Asked.H} : Asked;
	const igniteart::IgniteTones Tones = rlauto_detail::AutoIgTones(World);
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
	if (Home)
	{
		// The table dims behind it (the top bar and the taskbar stay as they are).
		C->FillRect({0.0f, 64.0f, Width, Height - 64.0f - 38.0f}, Rgba(5, 9, 16, 0.6f * In));
		C->SetAlpha(A0 * In);
		C->GlowRoundRect({R.X, R.Y + 10.0f, R.W, R.H}, 18.0f, Rgba(0, 0, 0, 0.6f), 28.0f);
		UI.RRect(R, 18.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, rlsites_detail::SitesTone(0x13213a), rlsites_detail::SitesTone(0x0c1524)), rlsites_detail::SitesTone(0x2a3f60), 1.5f);
	}
	else
	{
		// The table dims under the look's own chrome (Ignite's marquee; RunItTwice's tape, header and rail, but in the tiles,
		// which RiverLine's frame draws) in the look's scrim: Night, or Tape Black.
		const float Top = Rit && Tiled ? 64.0f : Look.ChromeH;
		const float Left = Rit && !Tiled ? rlauto_detail::AutoRitRail : 0.0f;
		C->FillRect({Left, Top, Width - Left, Height - Top - 38.0f}, Hex(Rit ? rlsites_detail::SitesRitTape : rlsites_detail::SitesIgNight, (Rit ? 0.8f : 0.72f) * In));
		C->SetAlpha(A0 * In);
		C->GlowRoundRect({R.X, R.Y + 10.0f, R.W, R.H}, Look.PanelRadius, Rgba(0, 0, 0, 0.6f), 28.0f);
		if (Rit)
		{
			// The Exchange's panel: Carbon cut 10 under a hairline, a 4-unit Signal Red top bar with its LED core, its light
			// spilling above it.
			rlrit_detail::RitGlow(*C, {R.X, R.Y - 2.0f, R.W - Look.Cut, 16.0f}, rlrit_detail::RitRed, 0.4f, 14.0f);
			rlrit_detail::RitPanel(*C, R, Look.Cut, rlrit_detail::RitCarbon, rlrit_detail::RitHair);
			C->FillRect({R.X, R.Y, R.W - Look.Cut, 4.0f}, pal::Accent);
			C->FillRect({R.X, R.Y, R.W - Look.Cut, 1.0f}, Hex(rlsites_detail::SitesRitLed));
		}
		else
		{
			// The arcade's card: round, in the hour's panel tones, ringed in the neon (the accent's tube and a cream line).
			C->GlowRoundRect(R, Look.PanelRadius, rlnet_detail::NetA(pal::Accent, 0.32f), 18.0f);
			C->FillRoundRect(R, Look.PanelRadius, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, Hex(Tones.PanelTop), Hex(Tones.Plum)));
			igniteart::IgniteNeonRoundRect(*C, R, Look.PanelRadius, pal::Accent, 2.0f, 6.0f);
			C->StrokeRoundRect({R.X + 6.0f, R.Y + 6.0f, R.W - 12.0f, R.H - 12.0f}, Look.PanelRadius - 6.0f, rlnet_detail::NetA(pal::Ink, 0.14f), 1.0f);
		}
	}

	// Header: whose panel this is, the state it's in, and the way out.
	const std::string Title = autoplay::Text(autoplay::Words::PanelTitle);
	if (Home)
	{
		rlnet_detail::NetSpaced(*C, Title, R.X + 30.0f, R.Y + 38.0f, 12.0f, 800, pal::Accent, 2.0f);
		UI.Text(Event, R.X + 30.0f, R.Y + 68.0f, Ts(22.0f, 800, pal::Ink, Align::Left, Baseline::Alphabetic, false, 520.0f));
		UI.Text(rlauto_detail::AutoSubtitle, R.X + 30.0f, R.Y + 92.0f, Ts(13.5f, 500, pal::Muted, Align::Left, Baseline::Alphabetic, false, 640.0f));
	}
	else if (Rit)
	{
		// The section slab (§3.4): Signal Red, its right edge leaning 18 degrees, AI AUTOPLAY in Hot White; the panel's one twin
		// stripe slashed after it; then the event, twice-printed in BoldCondensed caps (fit down to 22, where it prints once).
		const float Sy = R.Y + 18.0f;
		const float Sh = 40.0f;
		const float SlabW = 206.0f;
		C->FillPolygon({{R.X, Sy}, {R.X + SlabW, Sy}, {R.X + SlabW - Sh * 0.3249197f, Sy + Sh}, {R.X, Sy + Sh}}, pal::Accent);
		C->FillRect({R.X, Sy, SlabW, 1.0f}, Hex(rlsites_detail::SitesRitLed));
		C->Text(Title, R.X + 22.0f, Sy + Sh / 2.0f + 1.0f, rlauto_detail::AutoFace(Look, 22.0f, pal::Gold, Align::Left, Baseline::Middle));
		rlrit_detail::RitTwinSlash(*C, R.X + SlabW + 4.0f, Sy, Sh, 1.0f);
		const std::string Name = rlnet_detail::NetUpper(Event);
		const float NameX = R.X + SlabW + 36.0f;
		const float NameW = R.X + R.W - 52.0f - 14.0f - 150.0f - NameX;
		const float Sz = rlrit_detail::RitFit(*C, Name, 30.0f, 22.0f, NameW);
		if (rlrit_detail::RitMeasure(*C, Name, Sz) <= NameW)
		{
			rlrit_detail::RitTwice(*C, Name, NameX, Sy + 31.0f, Sz, pal::Ink);
		}
		else
		{
			rlrit_detail::RitText(*C, Name, NameX, Sy + 31.0f, Sz, pal::Ink, rlrit_detail::RitFace::Cond, Align::Left, Baseline::Alphabetic, NameW);
		}
		UI.Text(rlauto_detail::AutoSubtitle, R.X + 30.0f, R.Y + 90.0f, Ts(14.0f, 500, pal::Muted, Align::Left, Baseline::Alphabetic, false, 720.0f));
	}
	else
	{
		// The arcade's head: a spark and AI AUTOPLAY in the accent, the event as a cabinet's head (BlackItalic 28).
		igniteart::IgniteSpark(*C, R.X + 37.0f, R.Y + 33.0f, 8.0f, pal::Accent);
		C->Text(Title, R.X + 50.0f, R.Y + 38.0f, rlauto_detail::AutoFace(Look, 14.0f, pal::Accent));
		UI.Text(Event, R.X + 30.0f, R.Y + 72.0f, igniteart::IgniteBi(28.0f, pal::Ink, Align::Left, Baseline::Alphabetic, 520.0f));
		UI.Text(rlauto_detail::AutoSubtitle, R.X + 30.0f, R.Y + 95.0f, Ts(14.0f, 500, pal::Muted, Align::Left, Baseline::Alphabetic, false, 640.0f));
	}
	{
		const Rect X{R.X + R.W - 52.0f, R.Y + 20.0f, 34.0f, 34.0f};
		const Ui::ClickState Cx = UI.Clickable("autoclose", X);
		const std::string State = P.Skipping ? std::string("SKIPPING") : P.On ? "WATCHING " + autoplay::SpeedName(P.Now.Speed) : std::string("OFF");
		const Color Sc = P.Skipping ? pal::Accent2 : P.On ? pal::Accent : pal::Muted;
		if (Home)
		{
			C->FillCircle(X.X + 17.0f, X.Y + 17.0f, 17.0f, Cx.Hover ? pal::Line : rlsites_detail::SitesTone(0x162540));
			C->StrokePolyline({{X.X + 11.0f, X.Y + 11.0f}, {X.X + 23.0f, X.Y + 23.0f}}, false, pal::Ink, 2.0f, true);
			C->StrokePolyline({{X.X + 23.0f, X.Y + 11.0f}, {X.X + 11.0f, X.Y + 23.0f}}, false, pal::Ink, 2.0f, true);
			const float Sw = C->Measure(State, 11.0f, 700) + 11.0f * 1.3f;
			rlnet_detail::NetPill(*C, State, X.X - 14.0f - Sw, R.Y + 27.0f, Sc, P.On, 11.0f);
			UI.Text(rlauto_detail::AutoCloseHint, R.X + R.W - 30.0f, R.Y + 92.0f, Ts(12.5f, 600, pal::Dim, Align::Right));
		}
		else if (Rit)
		{
			// A cut Carbon key with a chrome cross; the state as a tag: WATCHING lit red with Hot White, SKIPPING on chrome with
			// Onyx, OFF outlined in Muted.
			const std::vector<Vec2> Shut = rlsites_detail::SitesCutPath(X, 6.0f);
			C->FillPolygon(Shut, Cx.Hover ? pal::Panel2 : pal::Panel);
			C->StrokePolyline(Shut, true, Hex(rlsites_detail::SitesRitChrome, Cx.Hover ? 0.6f : 0.35f), 1.0f);
			C->StrokePolyline({{X.X + 11.0f, X.Y + 11.0f}, {X.X + 23.0f, X.Y + 23.0f}}, false, pal::Accent2, 1.6f);
			C->StrokePolyline({{X.X + 23.0f, X.Y + 11.0f}, {X.X + 11.0f, X.Y + 23.0f}}, false, pal::Accent2, 1.6f);
			const float Tw = rlrit_detail::RitMeasure(*C, rlnet_detail::NetUpper(State), 12.0f) + 14.0f;
			const uint32_t Fill = P.Skipping ? rlrit_detail::RitChrome : P.On ? rlrit_detail::RitRed : rlrit_detail::RitMuted;
			const uint32_t Ink = P.Skipping ? rlrit_detail::RitOnyx : P.On ? rlrit_detail::RitWhite : rlrit_detail::RitMuted;
			rlrit_detail::RitTag(*C, X.X - 14.0f - Tw, R.Y + 27.0f, State, Fill, Ink, 12.0f, 7.0f, 20.0f, 4.0f, !P.Skipping && !P.On);
			UI.Text(rlauto_detail::AutoCloseHint, R.X + R.W - 30.0f, R.Y + 90.0f, Ts(13.0f, 500, pal::Muted, Align::Right));
		}
		else
		{
			// A candy button with a cream cross; the state as a candy pill (lit in its light with plum ink).
			C->FillCircle(X.X + 17.0f, X.Y + 17.0f, 17.0f, Cx.Hover ? pal::Line : pal::Panel2);
			C->StrokeEllipse(X.X + 17.0f, X.Y + 17.0f, 17.0f, 17.0f, Cx.Hover ? pal::Accent : pal::Line, 1.2f);
			C->StrokePolyline({{X.X + 11.0f, X.Y + 11.0f}, {X.X + 23.0f, X.Y + 23.0f}}, false, pal::Ink, 2.5f, true);
			C->StrokePolyline({{X.X + 23.0f, X.Y + 11.0f}, {X.X + 11.0f, X.Y + 23.0f}}, false, pal::Ink, 2.5f, true);
			const bool Lit = P.Skipping || P.On;
			const TextStyle St = rlauto_detail::AutoFace(Look, 11.0f, Lit ? rlauto_detail::AutoIgInk() : pal::Muted, Align::Center, Baseline::Middle);
			const float Sw = rlauto_detail::AutoWidth(*C, State, St) + 24.0f;
			const Rect Pill{X.X - 14.0f - Sw, R.Y + 26.0f, Sw, 22.0f};
			rlauto_detail::AutoIgPill(*C, Pill, Lit ? Sc : pal::Panel2, pal::Line, Lit ? 0.0f : 1.2f, Lit);
			C->Text(State, Pill.X + Pill.W / 2.0f, Pill.Y + Pill.H / 2.0f + 0.5f, St);
			UI.Text(rlauto_detail::AutoCloseHint, R.X + R.W - 30.0f, R.Y + 95.0f, Ts(12.5f, 600, pal::Muted, Align::Right));
		}
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
		if (!Home)
		{
			rlauto_detail::AutoRivalTile(UI, *C, Look, Tones, T, I, Why, Picked, St.Hover, P.On);
		}
		else
		{
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
		const std::string Pct = std::to_string(static_cast<int>(std::lround(P.Sharp * 100.0))) + "%";
		const float Frac = static_cast<float>(Clamp01((P.Sharp - 0.5) / 0.5));
		const std::string When = P.On ? "Held for this run since " + net::TimeLabel(P.SharpSetAt) + "." : std::string(rlauto_detail::AutoSharpOff);
		if (Home)
		{
			rlnet_detail::NetSpaced(*C, "SHARPNESS", X, Lower + 12.0f, 10.5f, 800, pal::Muted, 1.6f);
			const Color Sc = P.Sharp >= 0.9 ? pal::Green : P.Sharp >= 0.7 ? pal::Accent : pal::Orange;
			const float Nw = UI.Text(Pct, X, Lower + 56.0f, Ts(38.0f, 900, Sc, Align::Left, Baseline::Alphabetic, true));
			const Rect Bar{X + Nw + 18.0f, Lower + 40.0f, W - Nw - 18.0f, 8.0f};
			UI.RRect(Bar, 4.0f, Rgba(255, 255, 255, 0.08f));
			UI.RRect({Bar.X, Bar.Y, std::max(8.0f, Bar.W * Frac), Bar.H}, 4.0f, Sc);
			UI.Text(P.SharpWhy, X, Lower + 82.0f, Ts(15.0f, 700, pal::Ink, Align::Left, Baseline::Alphabetic, false, W));
			UI.Text(When, X, Lower + 104.0f, Ts(12.5f, 600, pal::Dim, Align::Left, Baseline::Alphabetic, false, W));
		}
		else if (Rit)
		{
			// The figure in BoldCondensed: Hot White twice-printed over Signal Red; Signal Up at 90% and Caution Amber under
			// 70% print once (§3.3: coloured figures do). The meter is the terminal's 20 cells, lit red as light.
			UI.Text("SHARPNESS", X, Lower + 12.0f, rlauto_detail::AutoFace(Look, 13.0f, pal::Muted));
			const bool Once = P.Sharp >= 0.9 || P.Sharp < 0.7;
			const Color Fig = P.Sharp >= 0.9 ? pal::Green : P.Sharp >= 0.7 ? pal::Gold : pal::Orange;
			const float Nw = Once ? rlrit_detail::RitText(*C, Pct, X, Lower + 58.0f, 44.0f, Fig) : rlrit_detail::RitTwice(*C, Pct, X, Lower + 58.0f, 44.0f, Fig);
			rlauto_detail::AutoRitCells(*C, {X + Nw + 18.0f, Lower + 38.0f, W - Nw - 18.0f, 10.0f}, Frac, P.Sharp >= 0.9 ? pal::Green : P.Sharp >= 0.7 ? pal::Accent : pal::Orange);
			UI.Text(P.SharpWhy, X, Lower + 84.0f, Ts(15.0f, 700, pal::Ink, Align::Left, Baseline::Alphabetic, false, W));
			UI.Text(When, X, Lower + 106.0f, rlauto_detail::AutoFit(*C, When, rlauto_detail::AutoMono(12.5f, pal::Muted), W));
		}
		else
		{
			// The figure in BlackItalic, the meter a candy pill with its spark.
			UI.Text("SHARPNESS", X, Lower + 12.0f, rlauto_detail::AutoFace(Look, 12.0f, pal::Muted));
			const Color Sc = P.Sharp >= 0.9 ? pal::Green : P.Sharp >= 0.7 ? pal::Accent : pal::Orange;
			const float Nw = UI.Text(Pct, X, Lower + 58.0f, igniteart::IgniteBi(42.0f, Sc));
			const Rect Bar{X + Nw + 18.0f, Lower + 38.0f, W - Nw - 18.0f, 12.0f};
			rlauto_detail::AutoIgMeter(*C, Bar, Frac, Paint::Linear({Bar.X, 0.0f}, {Bar.X + Bar.W, 0.0f}, Mix(Sc, Hex(0xffffff), 0.3f), Sc));
			UI.Text(P.SharpWhy, X, Lower + 84.0f, Ts(15.0f, 700, pal::Ink, Align::Left, Baseline::Alphabetic, false, W));
			UI.Text(When, X, Lower + 106.0f, Ts(12.5f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, false, W));
		}
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
		// The rows (RiverLine's and Ignite's 32 tall; RunItTwice's panel is 20 shorter, 850, so its rows sit tighter, 30 tall).
		const float Rh = Rit ? 30.0f : 32.0f;
		const float WatchY = Rit ? Lower + 18.0f : Lower + 22.0f;
		const float ToY = Rit ? Lower + 76.0f : Lower + 90.0f;
		const float StopY = Rit ? Lower + 134.0f : Lower + 158.0f;
		const float StopPitch = Rit ? 34.0f : 36.0f;
		const auto Kicker = [&](const char* Word, float Y) {
			if (Home)
			{
				rlnet_detail::NetSpaced(*C, Word, X, Y, 10.5f, 800, pal::Muted, 1.6f);
			}
			else
			{
				UI.Text(Word, X, Rit ? Y - 2.0f : Y, rlauto_detail::AutoFace(Look, Rit ? 13.0f : 12.0f, pal::Muted));
			}
		};
		Kicker("WATCH", Lower + 12.0f);
		float Sx = X;
		for (const int Speed : {1, 2, 4, 8})
		{
			const Rect B{Sx, WatchY, 56.0f, Rh};
			const bool Lit = !P.Skipping && !U.SkipPicked && P.Now.Speed == Speed;
			if (Home ? rlauto_detail::AutoSegment(UI, *C, "autospeed" + std::to_string(Speed), B, autoplay::SpeedName(Speed), Lit, !Done)
					 : rlauto_detail::AutoRivalChip(UI, *C, Look, "autospeed" + std::to_string(Speed), B, autoplay::SpeedName(Speed), Lit, rlauto_detail::AutoLit::Accent, !Done, 15.0f))
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
			const Rect B{Sx + 10.0f, WatchY, 74.0f, Rh};
			const bool Lit = P.Skipping || (!P.On && U.SkipPicked);
			if ((Home ? rlauto_detail::AutoSegment(UI, *C, "autoskip", B, "SKIP", Lit, !Done) : rlauto_detail::AutoRivalChip(UI, *C, Look, "autoskip", B, "SKIP", Lit, rlauto_detail::AutoLit::Second, !Done, 15.0f)) &&
				!P.Skipping)
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
		Kicker("SKIP TO", Rit ? Lower + 70.0f : Lower + 80.0f);
		const autoplay::Target Targets[5] = {autoplay::Target::NextBreak, autoplay::Target::Bubble, autoplay::Target::FinalTable, autoplay::Target::Result, autoplay::Target::Hands};
		// Online events take no breaks: there the first target stops at the next level, and says so (as its button does).
		const auto TargetLabel = [Breaks](autoplay::Target To) { return To == autoplay::Target::NextBreak && !Breaks ? std::string("Next level") : std::string(autoplay::TargetName(To)); };
		// A rival's face runs wider (Ignite's BlackItalic): its five targets take a size down rather than leave the panel.
		float ToSize = 13.5f;
		if (!Home)
		{
			float TextW = 0.0f;
			for (const autoplay::Target To : Targets)
			{
				TextW += rlauto_detail::AutoRivalChipW(*C, Look, TargetLabel(To), ToSize) - 28.0f;
			}
			const float Room = Right - X - 5.0f * 28.0f - 4.0f * 8.0f;
			ToSize = TextW > Room ? ToSize * Room / TextW : ToSize;
		}
		float Tx = X;
		for (const autoplay::Target To : Targets)
		{
			const std::string Label = TargetLabel(To);
			const float W = Home ? rlauto_detail::AutoSegmentW(UI, Label) : rlauto_detail::AutoRivalChipW(*C, Look, Label, ToSize);
			const bool Reachable = !(To == autoplay::Target::Bubble && InMoney) && !(To == autoplay::Target::FinalTable && Final);
			const Rect B{Tx, ToY, W, Rh};
			if (Home ? rlauto_detail::AutoSegment(UI, *C, std::string("autoto") + Label, B, Label, P.Now.To == To, Reachable && !Done)
					 : rlauto_detail::AutoRivalChip(UI, *C, Look, std::string("autoto") + Label, B, Label, P.Now.To == To, rlauto_detail::AutoLit::Accent, Reachable && !Done, ToSize))
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
		Kicker("STOP RULES", Rit ? Lower + 128.0f : Lower + 148.0f);
		float Gx = X;
		float Gy = StopY;
		for (const uint32_t Bit : {autoplay::StopFinalTable, autoplay::StopBubble, autoplay::StopMyAllIn, autoplay::StopBigPot, autoplay::StopPremium})
		{
			const std::string Label = autoplay::StopName(Bit);
			const float W = Home ? UI.Measure(Label, 13.0f, 700) + 46.0f : rlauto_detail::AutoRivalToggleW(*C, Look, Label);
			if (Gx + W > Right)
			{
				Gx = X;
				Gy += StopPitch;
			}
			const Rect B{Gx, Gy, W, Rh};
			const bool On = (P.Now.Stops & Bit) != 0;
			if (Home ? rlauto_detail::AutoToggle(UI, *C, "autostop" + std::to_string(Bit), B, Label, On, !Done)
					 : rlauto_detail::AutoRivalToggle(UI, *C, Look, "autostop" + std::to_string(Bit), B, Label, On, !Done))
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
		if (Home || Rit)
		{
			C->FillRect({R.X + 24.0f, Fy - 14.0f, R.W - 48.0f, 1.0f}, pal::Line);
		}
		else
		{
			// Ignite's rule is the marquee's band of the hour.
			C->FillRoundRect({R.X + 24.0f, Fy - 15.0f, R.W - 48.0f, 3.0f}, 1.5f, Paint::Linear({R.X + 24.0f, 0.0f}, {R.X + R.W - 24.0f, 0.0f}, Hex(Tones.BandA), Hex(Tones.BandB)));
		}
		const autoplay::TableAuto Def = autoplay::DefaultPlan(S.Life.Auto);
		const bool IsDefault = Def.S == P.Now.S && Def.Speed == P.Now.Speed && Def.To == P.Now.To && Def.Stops == P.Now.Stops;
		const std::string DefLabel = IsDefault ? std::string("Your default") : std::string(autoplay::Text(autoplay::Words::MakeDefault));
		bool MakeIt = false;
		if (Home)
		{
			ButtonOpts D;
			D.Kind = ButtonKind::Secondary;
			D.Size = 15.0f;
			D.Enabled = !IsDefault;
			MakeIt = rlsites_detail::SitesButton(UI, *C, "autodefault", {R.X + 30.0f, Fy, 240.0f, 46.0f}, DefLabel, D);
		}
		else
		{
			MakeIt = rlauto_detail::AutoRivalButton(UI, *C, Look, Tones, "autodefault", {R.X + 30.0f, Fy, 240.0f, 46.0f}, DefLabel, rlauto_detail::AutoRole::Quiet, !IsDefault, 15.0f, Now);
		}
		if (MakeIt && !IsDefault)
		{
			autoplay::SetDefault(S.Life.Auto, S.Life, P.Now);
			Say(rlauto_detail::AutoDefaultSaved, false);
		}
		if (IsDefault)
		{
			rlnet_detail::NetCheck(*C, R.X + 52.0f, Fy + 23.0f, 12.0f, Rit ? pal::Accent2 : pal::Accent);
		}
		if (!U.Note.empty() && Now - U.NoteAt < 5.0)
		{
			// RunItTwice's good word is Chrome (red is light, never ink).
			UI.Text(U.Note, R.X + 290.0f, Fy + 28.0f, Ts(13.5f, Rit ? 500 : 700, U.NoteBad ? pal::Orange : Rit ? pal::Accent2 : pal::Accent, Align::Left, Baseline::Alphabetic, false, R.W - 290.0f - 300.0f));
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
		const Rect GoR{R.X + R.W - 290.0f, Fy, 260.0f, 46.0f};
		if ((Home ? rlsites_detail::SitesButton(UI, *C, "autogo", GoR, Label, Go)
				  : rlauto_detail::AutoRivalButton(UI, *C, Look, Tones, "autogo", GoR, Label, rlauto_detail::AutoRoleOf(Go.Kind), !Done, 16.0f, Now)) &&
			!Done)
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
	// The table's look (update 3, G1 fix 2): a rival's strip in its own language, the same rows, ids and presses; RiverLine's
	// below, as it always was.
	const rlsites_detail::SiteLook& Look = rlsites_detail::SitesTableLook(S);
	if (rlauto_detail::AutoLookOf(Look) != rlauto_detail::AutoLook::Home)
	{
		rlauto_detail::AutoRivalStrip(UI, *C, S, *this, Look, P, Info, Table, Stop, R, Now, World);
		return;
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
	// The table's look (update 3, G1 fix 2): a rival's card in its own language, the same ids and presses; RiverLine's below,
	// as it always was.
	const rlsites_detail::SiteLook& Look = rlsites_detail::SitesTableLook(S);
	if (rlauto_detail::AutoLookOf(Look) != rlauto_detail::AutoLook::Home)
	{
		rlauto_detail::AutoRivalSkip(UI, *C, S, Look, T, P, Table, R, Now, World);
		return;
	}
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
