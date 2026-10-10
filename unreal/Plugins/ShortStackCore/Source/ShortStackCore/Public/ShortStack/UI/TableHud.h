#pragma once

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
 * Bodies: core-tableui (TableHud.cpp, DeskScreen.cpp, Controls.cpp). Phase A, day 1: stubs that compile and draw
 * nothing; the desk takes no requests and the control maps are empty.
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
	std::string Label;
	int BadgeCell = -1; // badgeart::AtlasCell, Room scope, one glyph
	Chips Stack = 0;
	Chips Bet = 0;
	chipset::Read Read;
	bool Hero = false;
	bool Folded = false;
	bool AllIn = false;
	bool Acting = false;
	bool Button = false;
	bool Away = false;
	std::string LastAction;
	Chips LastTo = 0;
	float X = 0.0f; // projected, in the HUD's units
	float Y = 0.0f;
	bool OnScreen = true;
	float Alpha = 1.0f;
};
/** A text, a sponsor bonus, what the player's wearing at the sit-down: the top-right lane (§1.6). */
struct Toast
{
	std::string From;
	std::string Line;
	uint32_t Accent = 0;
	double At = 0.0;
};
/** The studying caption (§5): name, label, up to 3 badges and the honorific. */
struct Studied
{
	std::string Name;
	std::string Label;
	std::string Honorific;
	std::vector<int> BadgeCells;
	float Alpha = 0.0f;
};
struct TableHudInfo
{
	Venue Where = Venue::CardRoom;
	const live::ClockFacts* Clock = nullptr; // null at Dee's game (dollars, no clock card)
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
	float AllInHold = 0.0f;
	sizing::Spot Spot;
	std::vector<sizing::Preset> Presets;
	int Preset = -1;
	std::vector<SeatPlate> Plates;
	StackReadout Readout = StackReadout::Shown;
	StackUnits Units = StackUnits::BigBlinds;
	float Bpm = 68.0f;
	std::vector<float> Trace;
	float Focus = 1.0f;
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
	StreetHudInfo OnFoot;
	bool Walking = false;
	struct Marker
	{
		std::string Text;
		std::string Sub;
		float X = 0.0f;
		float Y = 0.0f;
		float Metres = 0.0f;
		bool OnScreen = false;
	} SeatMarker;
	const recall::Record* LastHand = nullptr;
	double LastHandAt = -100.0;
	bool Gamepad = false;
	double HintsAt = -100.0;
	bool OnAir = false;
	float Scale = 1.0f;
};
SHORTSTACKCORE_API void DrawTableHud(Canvas& C, const TableHudInfo& I, double Now);

/** The table card (hold Tab or d-pad up; modal): READS, HANDS, PAYOUTS, FIELD, NIGHT. */
struct TableCardInfo
{
	int Page = 0; // READS, HANDS, PAYOUTS, FIELD, NIGHT
	std::vector<std::string> Reads;
	std::vector<recall::Record> Hands;
	std::vector<live::PayRow> Pay;
	const live::ClockFacts* Clock = nullptr;
	std::vector<std::pair<std::string, std::string>> Known;
	std::vector<Caption> Night;
	std::string NightTitle;
	StackUnits Units = StackUnits::BigBlinds;
	bool Gamepad = false;
};
SHORTSTACKCORE_API void DrawTableCard(Canvas& C, const TableCardInfo& I, double Now);
/** The room's clock screens (§6.5). Page: CLOCK, PAYOUTS, TODAY, LEADERS. */
SHORTSTACKCORE_API void DrawRoomClock(Canvas& C, const live::ClockFacts& F, int Page, const std::vector<std::string>& Today,
	const std::vector<life::RoomLeader>& Leaders, const std::string& HeroName, double Now);

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
	std::vector<life::LedgerEntry> Receipt;
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
	int PayOption = 0;
};
class DeskScreen
{
public:
	/** Window: "desk" | "cage". */
	SHORTSTACKCORE_API void Open(double Now, const std::string& Window);
	SHORTSTACKCORE_API void Key(const std::string& Name, double Now);
	SHORTSTACKCORE_API void Draw(Canvas& C, const DeskInfo& I, double Now);
	SHORTSTACKCORE_API std::vector<DeskRequest> TakeRequests();
	bool Gamepad = false;
	Pointer Ptr;

private:
	std::string Which;
	double OpenedAt = -10.0;
	std::vector<DeskRequest> Pending;
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
	const char* Id = nullptr;
	const char* Key = nullptr;
	const char* Pad = nullptr;
	bool Hold = false;
	const char* Label = nullptr;
};
SHORTSTACKCORE_API const std::vector<Control>& Controls(ControlContext C);
} // namespace ui
} // namespace ss
