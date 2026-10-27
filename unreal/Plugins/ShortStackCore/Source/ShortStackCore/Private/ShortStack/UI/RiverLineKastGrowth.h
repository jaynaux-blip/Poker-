// The creator update (SPEC_CREATORS §5.1, §5.3): Kast's new widgets, ui-kast's header (private to the core; RiverLine.h is frozen,
// so the widgets are free functions here rather than members). RiverLineKast.cpp's members hand each one a KastFrame (the
// laptop's canvas, pointer, session and clock) and the widget draws from the stream's const kast::View, the stream's last
// Summary or the Channel offline, and acts only through the Session's verbs. The per-laptop state (the Nod menu, the toast
// line, the forecast cache, chat's pause) lives in RiverLineKastGrowth.cpp by laptop, and ~RiverLine drops it (rlkast_detail::Forget,
// declared in RiverLineShared.h). While the laptop is Previewing every widget draws a still and changes nothing.
#pragma once

#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/UI/Canvas.h"
#include "ShortStack/UI/Ui.h"

#include <array>
#include <functional>
#include <string>
#include <vector>

namespace ss
{
namespace ui
{
class RiverLine;

namespace rlkast_detail
{
// ------------------------------------------------------------------ Kast's look (RiverLineKast.cpp's, shared by both files)

const float KastH = 962.0f;
const Color KBg = Hex(0x0e0a16);
const Color KPanel = Hex(0x17121f);
const Color KPanel2 = Hex(0x201832);
const Color KLine = Hex(0x2d2442);
const Color KInk = Hex(0xf1ecff);
const Color KMuted = Hex(0x9d93b8);
const Color KDim = Hex(0x625a7c);
const Color KViolet = Hex(kast::Violet);
const Color KLime = Hex(kast::Lime);
const Color KRed = Hex(0xff3b5c);
const Color KGold = Hex(0xf5c542);
// The creator update's additions: Event Mode's second gold, the lavender of a lit name, the sky of the directory, the deep ink
// on lime and gold, the button grey.
const Color KGold2 = Hex(0xd99a1e);
const Color KLav = Hex(0xd8ccff);
const Color KSky = Hex(0x38bdf8);
const Color KPink = Hex(0xf472b6);
const Color KDeep = Hex(0x1b1036);
const Color KBtn = Hex(0x3a2f55);
const Color KGreen = Hex(0x22c55e);

/** "1,040", "12.4K", "312K" (RiverLineKast.cpp). */
std::string KastCount(double V);
/** "3:52:10" (RiverLineKast.cpp). */
std::string KastClock(double Minutes);
/** The viewer graph's spark line (RiverLineKast.cpp). */
void KastSpark(Canvas& Cv, const Rect& R, const std::vector<float>& G, const Color& Col);

// ------------------------------------------------------------------ the frame a widget draws in

/** What a widget needs from the laptop drawing it: RiverLine's privates, handed over by its Kast members. */
struct KastFrame
{
	RiverLine& RL;
	Canvas& C;
	Ui& UI;
	Session& S;
	double World = 0.0;
	double Now = 0.0;
	bool Previewing = false;
};

// ------------------------------------------------------------------ the count (K1, §1.4)

/**
 * The integer every surface draws: View::Count, counting up over 0.3 real s from the last one shown (or the Room's floor,
 * View::ShownCount, when that is higher) on one clock a session, so the pill, StreamSide, the VIEWERS tile, the directory card
 * and the 24-inch Studio agree. Still: the target itself, and the clock untouched (the stream preview).
 */
int KastShown(const Session& S, double Now, bool Still);
/** The LIVE pill's width in every site's top bar (the tile strip reserves it: RiverLineTables.cpp): measured on the target. */
float KastPillWidth(Canvas& Cv, const Session& S);
/** The KAST pill in every site's top bar (RiverLine::LivePill): the count, gold in Event Mode, a click away from the studio. */
void KastPill(KastFrame& F, float Right, float Y);

// ------------------------------------------------------------------ Heat (K4)

/** Heat as a surface shows it: live, the View's (HeatShown, tonight's ghost, the tier, the why lines); offline, the Channel's
 *  through growth::HeatShown and growth::Tier, with the why lines Kast can say from the channel. Before the night has settled
 *  (KastGhostSettled) the ghost is 0 and Settled false: no number and no ghost fill, "tonight: settling in". */
struct KastHeat
{
	double Shown = 0.0;
	double Ghost = 0.0;
	int Tier = 0;
	std::array<std::string, 2> Why;
	bool Live = false;
	bool Settled = true;
};
KastHeat KastHeatNow(const Session& S, double World);
/**
 * Whether tonight's ghost has a number yet (the C-K audit's M4, the lead's ruling): not in a stream's first 15 world minutes, and
 * not until the forecast has a fair read, which is tonight's running average (the night's graph, a sample a world minute) inside
 * or above the forecast band with someone watching, or 60 world minutes live (a night counts toward Heat from 60, §2.4). The
 * running average starts at 0, so the ghost (perf on that average against the forecast) opened every stream at -9 before this.
 * Once settled it stays settled: the average as it stood at any world minute from 15 on counts (the graph's prefix).
 */
bool KastGhostSettled(const Session& S, double World);
/** Tile 4's sub line while live: "tonight: settling in" before the night has settled, then "tonight +N so far" (or -N). */
std::string KastGhostLine(const KastHeat& H);
/** Cold, Warming up, Running hot, On a heater, Sun run (growth::Tier's 0-4). */
const char* KastTierWord(int Tier);
/** The tier's tint: Running hot amber, On a heater orange, Sun run white-gold with a slow shimmer (warm light, never fire). */
Color KastTierTint(int Tier, double Now);
/** The thermometer glyph (a vector; never a flame): the fill is h, a lighter ghost above it. */
void KastThermo(Canvas& Cv, float X, float Y, float H, double Fill, double Ghost, const Color& Col);
/** The five-segment bar, one segment a tier, the ghost lighter past the committed level. */
void KastHeatBar(Canvas& Cv, const Rect& R, double Heat, double GhostTo);
/** The Kast header's 200 x 32 chip left of the LIVE pill. */
void KastHeatChip(Canvas& Cv, float X, float Y, const KastHeat& H, double Now);
/** "#23 of 49 in Poker" in the header, right-aligned at Right; returns its width (0 when there's no rank). */
float KastRankChip(Canvas& Cv, float Right, float Y, int Rank, int Of);

// ------------------------------------------------------------------ Studio (K4 K8 K9 K12, K2's Set the scene, K3's Rail Call)

/** Tile 0, VIEWERS: "12 watching", the top four sources, "5 chatting", the night's graph with the forecast band. */
void KastViewersTile(KastFrame& F, const Rect& R);
/**
 * The band's label on the VIEWERS tile's graph (the C-K PIE round's F5): the go-live forecast as it was ("forecast was 8-14",
 * the digits ForecastShort's), until the room has beaten it by 2x tonight (Count, the count shown, or Peak, the night's peak,
 * at least twice the band's top as shown); from then, while Count is above the band, KastCopy's verdict "above your forecast".
 */
std::string KastForecastLabel(const kast::growth::Band& Fc, int Count, int Peak);
/** Tile 4, HEAT: the tier word, tonight's ghost (live) or the top why line, the five-segment bar, the hype line. */
void KastHeatTile(KastFrame& F, const Rect& R);
/** The FOLLOWERS tile's foot before Affiliate ("Affiliate at 25", the real bar) and its bar's goal; Have the followers. */
std::string KastFollowerFoot(const kast::Channel& Ch, int& Goal);
/** Go live with the forecast under it ("Tonight: likely 8-14", "Tuesday · on schedule · Running hot"); true when pressed. */
bool KastGoBlock(KastFrame& F, const Rect& R, bool Open, bool Enabled, const std::string& Locked);
/** The action beside Go while live (R: the 220 x 32 under the raid button): Hold or Crack, Welcome the raid, Rail Call or Set
 *  the scene, else the rail call's reach. */
void KastStudioSlot(KastFrame& F, const Rect& R);
/** The Studio toast over the top of chat (the go-live's ping, Front Row, Welcome back): one a real minute. */
void KastStudioToast(KastFrame& F, const Rect& Chat);

// ------------------------------------------------------------------ the preview's overlay (K5 K11 K12)

/** Event Mode's EVENT bar, the Hold or Crack bar and the names a Nod lit, over the preview (Cam: the facecam's frame). */
void KastOverlay(KastFrame& F, const Rect& Prev, float Scale, const Rect& Cam);
/** Where the stream alert sits on the preview: under the EVENT bar and clear of the Hold or Crack bar when either is up. */
float KastAlertY(const Session& S, const Rect& Prev, float Scale);
/** Whether a raid alert is the site's Broadcast (K6): its line is one of the site voices' (copy::Broadcast) for its size. */
bool KastIsBroadcast(const kast::Alert& A);
/** The Broadcast alert's headline, "THE BROADCAST" (the mockup's; other raids read "RAID!"). */
const char* KastBroadcastHeadline();
/** The facecam's mood: Hyped through Event Mode when the stream's face would be Focus. */
kast::Mood KastCamMood(const Session& S);
/** Event Mode's stage word for the deepest run: "FINAL TABLE", else "LAST TWO TABLES". */
std::string KastStageWord(const Session& S, double World);

// ------------------------------------------------------------------ the stream preview's capture (§9.3's budget)

/** What the preview's client picture is drawn for beyond the clock; any change re-captures at once. */
struct KastCaptureKey
{
	Rect Frame;          // the preview's rect on the laptop
	int Client = 0;      // the poker client the stream shows (net::SiteId)
	int Screen = 0;      // Session::CurrentScreen
	int Tables = 0;      // tables open
	float PxScale = 0.0f; // the laptop canvas's pixel scale and transform
	Affine Xf;
	bool Same(const KastCaptureKey& O) const;
};
/** The capture's frame time: the stream's picture is re-captured 15 times a real second at a table (the laptop redraws at 30),
 *  10 on the lobby and the results (pages that barely move, and the lobby is the heaviest picture the preview takes). */
constexpr double KastCaptureFrame = 1.0 / 15.0;
constexpr double KastCaptureFrameStill = 1.0 / 10.0;
/** The frame time for a key: KastCaptureFrame at a table, else KastCaptureFrameStill. */
double KastCaptureFrameFor(const KastCaptureKey& Key);
/**
 * The stream preview's client picture, like the capture a stream encoder shows: Draw paints it (Previewing: a still that grants
 * nothing) and what it adds to the draw list is kept as the laptop's capture; a later draw with the same key, less than its frame
 * time (KastCaptureFrameFor) after the capture, copies the capture in instead of drawing the client again. A redraw at the instant
 * the preview last drew (or a clock that ran backwards) always re-captures, so a frame drawn again after a change shows it.
 */
void KastPreviewCapture(Canvas& Cv, const RiverLine* RL, const KastCaptureKey& Key, double Now, const std::function<void()>& Draw);
/** How many of the laptop's preview draws were captures and how many copies (kast_ui reads them). */
void KastCaptureCounts(const RiverLine* RL, long long& Captures, long long& Copies);

// ------------------------------------------------------------------ StreamSide (K1 K3 K4 K5 K10 K12)

/** The strip ("12 watching · 5 chatting", the tier and tonight, the Run Alert counter, Hold or Crack's bar), the one action
 *  slot, the waiting chips and the toast line; returns the y chat starts at. */
float KastSideTop(KastFrame& F, const Rect& R);

// ------------------------------------------------------------------ chat (K1 K11, C12)

/** "12 watching · 5 chatting" right-aligned at Right (Studio's chat header). */
void KastWatchingChatting(KastFrame& F, float Right, float Y);
/** The Mods tab's "2 of 3 here" (present mods of the channel's). */
std::string KastModsHere(const Session& S);
/** Whether a mod is present now (the Mods tab's dot). */
bool KastModHere(const Session& S, const std::string& Name);
/** Talk to chat's label: "(chat heard you)" after two thanks in ten world minutes. */
std::string KastThankLabel(const Session& S);
/** Whether Name is lit by a Nod right now (their lines glow). */
bool KastLit(const Session& S, const std::string& Name);
/** A chatter's name was pressed: the menu (Nod, Timeout, Promote to mod) opens on that line at X, Y. */
void KastOpenNameMenu(KastFrame& F, const kast::ChatMsg& M, float X, float Y);
/** The open name menu, drawn over chat (Bounds: the chat panel); true while it holds the pointer. */
bool KastNameMenu(KastFrame& F, const Rect& Bounds);
/** Whether the name menu is open (chat lines don't take presses under it). */
bool KastNameMenuOpen(const RiverLine* RL);
/**
 * Chat pauses under the pointer, as real platforms' chat does (the C-K PIE round's F3). While the pointer is over chat's lines
 * (or the name menu is open on one of them), lines keep arriving underneath but the list stops moving: it shows only the lines
 * that had landed when the pause began, so a name or an Answer stays where it was pressed. "Chat paused · 3 new" sits at the
 * list's foot. Moving off resumes and jumps to the newest; so does a press on the pill, and then chat runs live until the pointer
 * moves again over the lines. If every line the pause holds has scrolled out of chat's 140 (a long hover in a busy room), the
 * pause takes the newest again. UI-only, per laptop (KwLaptop); while the laptop is Previewing chat never pauses and nothing
 * here changes. Studio's chat and StreamSide's (every site's Stream tab) are RiverLine::StreamChat's, so both pause.
 */
struct KastChatHold
{
	bool Paused = false;
	double At = 0.0; // the clock when the pause began: lines that had landed by then show...
	int UpTo = 0;    // ...if they were in chat then (by id)
	int New = 0;     // lines that have landed since, underneath (the pill's N)
	Rect Pill;       // the pill at the list's foot (W 0 when not paused)
};
/** The pause for this draw of chat's list (Lines: the list's rect); once a draw, before the lines are laid out. */
KastChatHold KastChatPause(KastFrame& F, const Rect& Lines, bool Interactive);
/** Whether a line draws in the list: it has landed and, while paused, it is one the pause holds. */
bool KastChatShows(const KastChatHold& H, const kast::ChatMsg& M, double Now);
/** The pill ("Chat paused", "Chat paused · 3 new") over the list's foot; a press on it resumes and jumps to the newest. */
void KastChatPausePill(KastFrame& F, const KastChatHold& H);
/** The pill's words for N lines underneath. */
std::string KastChatPausedLabel(int New);
/** Whether the pointer is on the pill now (the line under it takes no press: the pill does). */
bool KastChatPillHeld(const RiverLine* RL);

// ------------------------------------------------------------------ the end card (K15 K17 K20, K8 K9 K10 K12)

/** The peak-first card's body between its head and its buttons, from Stream.Last; returns the height it takes. Draw false
 *  measures only. */
float KastEndCardBody(KastFrame& F, float X, float Y, float W, bool Draw);
/** The end card's clip slot: "Clip of the night: ..." with Clip it and Make it a Cut; returns its height. */
float KastEndCardClip(KastFrame& F, float X, float Y, float W, bool Draw);

// ------------------------------------------------------------------ the Community page (K3 K4 K10 K16 K20, C8 C13 C14)

void KastCommunityHeat(KastFrame& F, const Rect& R);
void KastCommunityPing(KastFrame& F, const Rect& R);
/** Building a community: the stage ladder with the Affiliate bar at the real platform's numbers (C8) and Partner's. */
void KastCommunityPath(KastFrame& F, const Rect& R);
void KastCommunityRaids(KastFrame& F, const Rect& R);
/** This week: the streak, the week's goals, Stamina from 24 hours live in 7 days. */
void KastCommunityWeek(KastFrame& F, const Rect& R);
/** Kast payouts and sponsor pay apart (the ledger's streaming and sponsor lines), the first payout and what waits. */
void KastCommunityPayouts(KastFrame& F, const Rect& R);

// ------------------------------------------------------------------ Browse (K6 K7)

/** The player's own card's chips (FINAL TABLE, NEW ON KAST, RISING IN POKER) after the viewers tag; returns the width used. */
float KastOwnCardChips(KastFrame& F, const Rect& Thumb, float X);
/** The three shelves under the cards: Sweating now, Fresh on Kast, Rising in Poker. */
void KastShelves(KastFrame& F, const Rect& R);

// ------------------------------------------------------------------ the Channel page (K15)

/** Top clips' flag for a clip in the Clip Lane (Clip::Lane): "in the Kast feed", KastCopy's own words (copy::ClipOfTheNight's). */
std::string KastClipLaneTag();

// ------------------------------------------------------------------ the copy

/** Every sentence and label the Kast widgets draw beyond core's copy, for the copy lint and the trademark screen. */
std::vector<std::string> KastUiInventory();
} // namespace rlkast_detail
} // namespace ui
} // namespace ss
