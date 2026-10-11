#pragma once

#include "ShortStack/Game/Badges.h"
#include "ShortStack/Game/ChipSet.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Recall.h"
#include "ShortStack/Game/Sizing.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/UI/BadgeArt.h"
#include "ShortStack/UI/Canvas.h"
#include "ShortStack/UI/Captions.h"
#include "ShortStack/UI/FrontEnd.h"
#include "ShortStack/UI/StoreCounter.h"
#include "ShortStack/UI/Ui.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace ui
{
/**
 * The table HUD (next_segment_spec §1.6, §6): one layout for the Embercrest's tables and Dee's, in core's visual
 * language, on a logical canvas 1,080 tall whose width follows the window (1,920 at 16:9, 2,257 at 2550x1220, 1,440 at
 * 4:3). Every feature fills a named slot: the clock card (with the funding row), the time block, the toast lane (held
 * while it's the player's decision), the banner and whispers, the plates at each stack (the AI stack sizes, one badge
 * glyph each), the studying caption, the captions, the hero block, the ladder and presets, and the table card. Panels
 * anchor to the canvas edges with 58-unit margins and never stretch. Also here: the room's clock screens, the desk and
 * the cage (fed by data: the card room has a SaveData, not a Session), and the one control map every HUD's glyphs read.
 *
 * The plates (§6.3) are the AI stack sizes: under SHOWN (the default) every live seat, always, exact, faded to 70% when
 * the action isn't on the player and full while it is, on the seat they look at, and for 2 s after a seat acts. A plate
 * sits by its stack and is laid out (LayoutPlates) so that it never covers what the host marks KeepClear (the faces,
 * the cards and the chips, projected), the HUD's own panels, the pot tag or another plate.
 */

/** What a glance at a stack tells (§6.2 A): YOUR EYES, THE ROOM'S WAY, SHOWN (the default). */
enum class StackReadout : int
{
	Eyes = 0,
	Room = 1,
	Shown = 2,
};
enum class StackUnits : int
{
	BigBlinds = 0,
	Chips = 1,
};
/** A plate at a stack on the felt (§6.3): the AI stack sizes. */
struct SeatPlate
{
	int Seat = 0;
	std::string Name;
	std::string Label;  // "Knows you", "Regular": small caps, teal
	int BadgeCell = -1; // badgeart::AtlasCell, Room scope, one glyph: the host draws the cell where HudGlyphs says
	badge::Badge Glyph; // or the badge itself (Glyph.K != Count): drawn here, as vector art
	Chips Stack = 0;    // the engine's stack once the chips have landed (StackPile): a plate never runs ahead of the chips
	Chips Bet = 0;
	chipset::Read Read; // THE ROOM'S WAY: what a glance gives (chipset::ReadStack); Exact once counted
	bool Hero = false;  // the player's own seat: the hero block says it, so no plate
	bool Folded = false;
	bool AllIn = false;
	bool Acting = false;
	bool Button = false;
	bool Away = false;  // sitting out (blinded off, or up from the table)
	std::string LastAction; // "RAISE", "CALL", "CHECK", "BET", "ALL IN", "FOLD": the host clears it when the street ends
	Chips LastTo = 0;       // the raise's or bet's total, for "RAISE 3,200 · 2 BB"
	double ActedAt = -100.0; // when the seat last acted (the HUD's clock): full strength for 2 s after
	bool Looked = false;     // the seat the player is looking at
	float X = 0.0f;          // the stack's spot projected, in the HUD's units
	float Y = 0.0f;
	bool OnScreen = true;    // off screen: an edge plate with a chevron toward the seat
	float Alpha = 1.0f;      // the host's own fade (a seat filling or emptying)
};
/** A text, a sponsor bonus, what the player's wearing at the sit-down: the top-right lane (§1.6). */
struct Toast
{
	std::string From;
	std::string Line;
	uint32_t Accent = 0; // 0xRRGGBB; 0: teal
	double At = 0.0;
};
/** The studying caption (§5): name, label, up to 3 badges and the honorific. */
struct Studied
{
	std::string Name;
	std::string Label;
	std::string Honorific;
	std::vector<int> BadgeCells;        // atlas cells (the host draws them where HudGlyphs says)
	std::vector<badge::Badge> Badges;   // or the badges, drawn here
	float Alpha = 0.0f;
};
/** A choice on screen, by its control's id (Controls): "[Q] Stay and watch". */
struct Choice
{
	std::string Control; // a Control::Id ("stand", "leave", "rebuy") or a key name ("R")
	std::string Label;
};
struct TableHudInfo
{
	Venue Where = Venue::CardRoom;           // CardRoom: tournament chips; BackRoom: Dee's dollars
	const live::ClockFacts* Clock = nullptr; // null at Dee's game (dollars, no clock card)
	std::string Event;                       // the clock card's header: "NIGHTLY $120"
	Chips HeroStack = 0;
	Chips Pot = 0;
	Chips Bb = 0;
	Vec2 PotAt;
	bool PotOnScreen = false;
	bool YourTurn = false;
	bool CanCheck = false;
	bool CanRaise = false;
	Chips ToCall = 0;
	Chips RaiseTo = 0;
	float AllInHold = 0.0f; // the ALL IN rung's fill (HoldButton::Progress)
	sizing::Spot Spot;
	std::vector<sizing::Preset> Presets;
	int Preset = -1;
	std::vector<SeatPlate> Plates;
	StackReadout Readout = StackReadout::Shown;
	StackUnits Units = StackUnits::BigBlinds;
	float Bpm = 68.0f;
	std::vector<float> Trace; // the heart's trace, oldest first, 0..1
	float Focus = 1.0f;       // Focus left, 0..1
	bool Studying = false;    // Focus held
	float Eyelids = 0.0f;
	bool Steadying = false;
	std::vector<Caption> Captions;
	CaptionStyle CaptionLook;
	std::vector<StreetHudInfo::Subtitle> Whispers;
	std::vector<Toast> Toasts; // the lane (§1.6): held by the host while YourTurn
	Studied Study;
	std::string Banner;
	double BannerAt = -100.0;
	std::string Prompt;
	StreetHudInfo OnFoot; // the time and the vitals seated; the street's whole block on foot
	bool Walking = false;
	struct Marker
	{
		std::string Text; // "TABLE 12 · SEAT 4"
		std::string Sub;  // "DEALT IN WHILE YOU'RE UP", "BREAK 9:40"
		float X = 0.0f;
		float Y = 0.0f;
		float Metres = 0.0f;
		bool OnScreen = false;
	} SeatMarker;
	const recall::Record* LastHand = nullptr;
	double LastHandAt = -100.0;
	bool Gamepad = false;
	double HintsAt = -100.0;
	bool OnAir = false; // the feature table on the stream: the lower third
	float Scale = 1.0f; // the HUD size setting (0.8 to 1.3): panels scale about their anchors

	// ---- what the legacy HUD drew, so the core HUD replaces it whole (BackRoomGameMode.cpp:1397-2131)
	/** Faces, cards and chips on screen (the HUD's units): plates never cover them. */
	std::vector<Rect> KeepClear;
	/** The player's cards once looked at (the hero block shows them small). */
	std::vector<Card> HeroCards;
	/** Out of the hand at a live table: the pace and the next-hand key (Pace -1 hides it). */
	bool InHand = true;
	int Pace = -1; // 0 LIVE, 1 BRISK, 2 FAST
	bool Skipping = false;
	/** Walking in: "EMBERCREST CASINO" over "the card room · embercrest nightly $120" and the day. */
	struct Arrival
	{
		std::string Title;
		std::string Sub;
		std::string Day;
		double At = -100.0;
	} Arrive;
	/** A note across the middle for a moment (the view, the pace, "You're felted."), with choices under it. */
	std::string Note;
	std::string NoteSub;
	double NoteAt = -100.0;
	bool NoteHolds = false; // stays until cleared (felted) instead of fading after 2.6 s
	std::vector<Choice> Choices;
	/** A line over the bottom ("Racking up after this hand") and a hold in progress (d-pad down: walk away). */
	std::string Status;
	std::string HoldLabel;
	float HoldProgress = 0.0f;
	/** On foot: how the night stands while up ("Up from table 12 · still dealt in"). */
	std::string FootNote;
	/** The night totted up, over the walk out (first line the title); Net colours the "Net" line. */
	std::vector<std::string> Summary;
	float SummaryAlpha = 0.0f;
	Chips Net = 0;
	/** The feature table's lower third (OnAir): the next place out pays this. */
	Chips NextOutCents = 0;
};
SHORTSTACKCORE_API void DrawTableHud(Canvas& C, const TableHudInfo& I, double Now);

/** Where a plate went (LayoutPlates): its box (the action tag below it included), and an edge plate's chevron. */
struct PlacedPlate
{
	int Index = -1; // into TableHudInfo::Plates
	Rect Box;       // the plate
	Rect Footprint; // the plate and the room its action tag takes under it
	bool Edge = false;
	int Toward = 0; // an edge plate's chevron: -1 left, 1 right
	float Alpha = 1.0f;
	Vec2 Glyph;     // the badge glyph's centre (16 units)
};
/**
 * The plates' layout on a canvas Width wide (1,080 tall): each by its stack (below it, else above, else beside), clamped
 * to x 58..W−58−188 and y 330..760 (at Scale 1), never over KeepClear, the HUD's panels (the clock card, the hero block,
 * the ladder's area, the toast lane), the pot tag or another plate when there's room for it; overlaps left over are
 * nudged vertically. Pure: the same info gives the same layout, so plates never jump while nothing moves.
 */
SHORTSTACKCORE_API std::vector<PlacedPlate> LayoutPlates(const TableHudInfo& I, float Width, double Now);
/** A badge glyph the host draws from the atlas (an atlas cell) at a centre and size, over the HUD. */
struct GlyphSpot
{
	int Cell = -1;
	float Cx = 0.0f;
	float Cy = 0.0f;
	float Size = 16.0f;
	float Alpha = 1.0f;
};
/** Where DrawTableHud left room for atlas cells (the plates' BadgeCell, the studying caption's BadgeCells). */
SHORTSTACKCORE_API std::vector<GlyphSpot> HudGlyphs(const TableHudInfo& I, float Width, double Now);

/** One field of the FIELD page: someone the player knows, still in, and their count as of the last break. */
struct FieldRow
{
	std::string Name;
	std::string Label;  // "Knows you", "Regular"
	int Table = 0;      // 0: not known
	Chips Count = -1;   // as of the last break (-1: not counted yet)
	bool Here = false;  // at the player's table (their plate is live)
};
/**
 * The FIELD page's rows the way a floor posts counts (§6.3): Known is who the player knows (name, label). FieldCounts is
 * the count at a break (call it when the break starts and keep the rows); FieldNow is the page between breaks: who is
 * still in and at which table now, each with the count from AtBreak (or -1, not counted yet), the player's own table
 * marked Here (its plates are live). Both sorted by the count, the biggest first.
 */
SHORTSTACKCORE_API std::vector<FieldRow> FieldCounts(const Tournament& T, const std::vector<std::pair<std::string, std::string>>& Known);
SHORTSTACKCORE_API std::vector<FieldRow> FieldNow(const Tournament& T, const std::vector<std::pair<std::string, std::string>>& Known, const std::vector<FieldRow>& AtBreak);

/** The table card (hold Tab or d-pad up; modal): READS, HANDS, PAYOUTS, FIELD, NIGHT. */
struct TableCardInfo
{
	int Page = 0; // READS, HANDS, PAYOUTS, FIELD, NIGHT
	std::vector<std::string> Reads;
	std::vector<recall::Record> Hands; // newest first
	std::vector<live::PayRow> Pay;
	const live::ClockFacts* Clock = nullptr;
	std::vector<std::pair<std::string, std::string>> Known; // name, detail ("TABLE 7 · 41,200"): used when Field is empty
	std::vector<Caption> Night;
	std::string NightTitle;
	StackUnits Units = StackUnits::BigBlinds;
	bool Gamepad = false;
	// ---- additions
	std::vector<FieldRow> Field;
	std::string CountsAt;    // "9:00": the break the counts are from; empty before the first
	int HeroPlace = 0;       // the player's place if they went out now (PAYOUTS' YOU row); 0: finished or not known
	Chips HeroStack = 0;
	Venue Where = Venue::CardRoom;
	std::string HeroName = "You";
};
SHORTSTACKCORE_API void DrawTableCard(Canvas& C, const TableCardInfo& I, double Now);
/** The table card's page names, in order. */
SHORTSTACKCORE_API const std::vector<std::string>& TableCardPages();
/**
 * The room's clock screens (§6.5), 1,024×576 (any size: it scales). Page: 0 CLOCK, 1 PAYOUTS, 2 TODAY, 3 LEADERS.
 * Today: the day's schedule, one event a line ("7:00 PM\tNIGHTLY $120\tRUNNING"). Event names the clock ("NIGHTLY $120").
 */
SHORTSTACKCORE_API void DrawRoomClock(Canvas& C, const live::ClockFacts& F, int Page, const std::vector<std::string>& Today,
	const std::vector<life::RoomLeader>& Leaders, const std::string& HeroName, double Now, const std::string& Event = std::string(),
	const std::vector<live::PayRow>& Pay = {});

/** An event at the desk, with who could pay for it (the options, and why not: "" when it can). */
struct DeskEvent
{
	live::Occurrence O;
	std::vector<std::pair<life::Funding, std::string>> Pay;
};
/** The desk and the cage (§6.6), fed by data: the card room has a SaveData, not a Session. */
struct DeskInfo
{
	std::vector<DeskEvent> Open;
	std::string Why;
	const life::LiveEntry* Entry = nullptr;
	Chips Bankroll = 0;
	int PastEvents = 0;
	std::string Address;
	std::string ClerkLine;
	badgeart::ClubCardInfo Card;
	bool CardPrinting = false;
	std::vector<sponsor::Window> Duties;
	std::vector<life::LedgerEntry> Receipt; // the cage: the entry's lines past the prize (a backer's share, a sponsor's cut or
	                                        // bonus); a prize line (kind 4) is the PRIZE row already and isn't counted twice
	// ---- additions
	double World = 0.0;    // world minutes (the desk's clock: late registration, the meet's window)
	std::vector<int> Fields; // per event in Open's order, the field so far (0 or missing: not known)
	Chips PrizeCents = 0;  // the cage: what the player cashed for
	int Place = 0;
	int Entrants = 0;
	double FinishedAt = 0.0;
	std::string Seat;      // after registering: "TABLE 12 · SEAT 4"
};
struct DeskRequest
{
	enum Kind
	{
		Register,
		Unregister,
		Collect,
		Tip,
		Appear,
		Leave,
	} K = Register;
	std::string Id;
	int Percent = 0;
	int PayOption = 0;   // Register: the index into the event's Pay as drawn
	life::Funding Fund;  // Register: that option itself, so the host registers with what the player saw (it matches From and
	                     // Ref against the options it builds now, and refuses if they've moved: an offer lapsed, a cover used)
};
class DeskScreen
{
public:
	/** Window: "desk" | "cage". */
	SHORTSTACKCORE_API void Open(double Now, const std::string& Window);
	/** "Up", "Down", "Left", "Right", "Enter" (or "A", "Space"), "Escape" (or "B", "Back"), "Q", "E", "TabPrev", "TabNext";
	 *  "R" (or "Y") leaves and gets refunded; "F" (or "X", "PutBack") answers a sponsor's appearance. */
	SHORTSTACKCORE_API void Key(const std::string& Name, double Now);
	SHORTSTACKCORE_API void Draw(Canvas& C, const DeskInfo& I, double Now);
	SHORTSTACKCORE_API std::vector<DeskRequest> TakeRequests();
	bool IsOpen() const { return !Which.empty(); }
	const std::string& Window() const { return Which; }
	int Selected() const { return Sel; }
	int PayChoice() const { return Pay; }
	bool Gamepad = false;
	Pointer Ptr;

private:
	std::string Which;
	double OpenedAt = -10.0;
	std::vector<DeskRequest> Pending;
	int Sel = 0;     // the event (desk) or the action (cage)
	int Pay = 0;     // who pays, among the event's options
	int Focus = 0;   // 0 the events, 1 who pays, 2 the actions
	double ActedAt = -10.0;
	std::string Said;
	double SaidAt = -10.0;
	DeskInfo Last;   // what the keys act on (the last Draw's; its Entry pointer is dropped: EntryId and EntryOpen say it)
	std::string EntryId;
	bool EntryOpen = false;  // registered (not yet settled or refunded)
	bool EntryFresh = false; // registered and not a hand played yet (no checkpoint): the refund is still on offer
	std::vector<std::pair<Rect, int>> Hits; // pointer targets: rect, action code
	float PressX = -1.0f;
	float PressY = -1.0f;
	void Act(int Code, double Now);
};

/** One control map (§6.4), read by every HUD's glyphs. On foot in the card room, the controls match the street. */
enum class ControlContext : int
{
	Street,
	CardRoomOnFoot,
	Table,
	TableCard,
};
struct Control
{
	const char* Id = nullptr;    // "fold", "call", "raise", "allin", "presets", "steps", "peek", "study", "breathe", "card", "units", "stand", "pace", "leave", "back", "view", "move", "run", "use", "eat", "pause", "page", "close"
	const char* Key = nullptr;   // the keyboard's glyph ("F", "SPACE", "TAB")
	const char* Pad = nullptr;   // the gamepad's ("X", "LT", "UP"); several are space-separated ("LB RB")
	bool Hold = false;           // held, not tapped
	const char* Label = nullptr; // "FOLD", "ALL IN"
};
SHORTSTACKCORE_API const std::vector<Control>& Controls(ControlContext C);
/** The control with that id in C (nullptr: none). */
SHORTSTACKCORE_API const Control* FindControl(ControlContext C, const std::string& Id);
/** The one-time line at the first live table after the controls moved (§6.4). */
SHORTSTACKCORE_API std::string ControlsNotice(bool Gamepad);

/**
 * Tap and hold on one button (§6.4). Y at the table raises on release if it was held under TapSeconds; past that the
 * ALL IN rung fills for FillSeconds and fires (once) when full; let go between the two and nothing happens. A held
 * button never taps first. The keyboard's all-in is a hold of A with no tap (TapSeconds 0).
 */
struct HoldButton
{
	double TapSeconds = 0.35;
	double FillSeconds = 0.6;
	bool CanTap = true;
	enum class Out : int
	{
		None,
		Tap,  // released early: raise
		Hold, // held to full: all in
	};
	SHORTSTACKCORE_API void Press(double Now);
	/** Call every frame while held; returns Hold once, the frame the fill completes. */
	SHORTSTACKCORE_API Out Tick(double Now);
	SHORTSTACKCORE_API Out Release(double Now);
	/** The ALL IN rung's fill, 0..1 (0 before the tap window ends). */
	SHORTSTACKCORE_API float Progress(double Now) const;
	/** Past the tap window and not yet full: letting go cancels ("Let go to cancel"). */
	SHORTSTACKCORE_API bool Cancelling(double Now) const;
	bool IsDown() const { return DownAt >= 0.0; }
	void Reset()
	{
		DownAt = -1.0;
		Fired = false;
	}

private:
	double DownAt = -1.0;
	bool Fired = false;
};

/**
 * The help under a setting the card room reads (the front end's AUDIO and GAMEPLAY tabs, §4.9, §6.7), by its save key:
 * "voices", "captions", "captionsize", "captionbg", "speakernames", "tabletalk", "tablepace", "stackreadout",
 * "stackunits", "turncue", "hudscale". Empty for a key it doesn't know.
 */
SHORTSTACKCORE_API std::string TableSettingHelp(const std::string& Key, bool Gamepad);
/** The table HUD's lines in sentences (empty states, notices, the settings' help), for the copy lint. */
SHORTSTACKCORE_API std::vector<std::pair<std::string, std::string>> TableUiCopy();
} // namespace ui
} // namespace ss
