// Update 3 (SPEC §2.3-§2.5, §7.2): Ignite Poker's art kit and its client's private seams, ui-ignite's header (private to the
// UI sources). The palette constants (never in pal::, none a RiverLine hex: rival_sites' leak check would read one as a
// leak), the hour's tones, the drawing kit (IgniteArt.cpp), the client's per-laptop side state, and
// rlignite_detail::Access, RiverLine's friend: static forwarders to its private state and its gatekeepers (RegisterBlock,
// ScheduleRows, FeatureSlides), so the pages in RiverLineIgnitePlay.cpp reach them without a member function of their own
// and the gatekeepers stay single-sourced.
#pragma once

#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/SiteShow.h"
#include "ShortStack/UI/Canvas.h"
#include "ShortStack/UI/RiverLine.h"
#include "ShortStack/UI/Ui.h"

#include <array>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace ss
{
namespace ui
{
namespace igniteart
{
// ------------------------------------------------------------------ the palette (SPEC §2.3)
constexpr uint32_t IgNight = 0x120a18;  // the dock well, scrims, the deepest sky
constexpr uint32_t IgPlum = 0x1a1220;   // token Bg (the day's page ground)
constexpr uint32_t IgGrape = 0x3a1f5c;  // cabinet bodies, wheel wedges
constexpr uint32_t IgCoral = 0xff6b57;  // token Accent (day)
constexpr uint32_t IgPink = 0xff3d8b;   // the neon tube's end, stickers, confetti
constexpr uint32_t IgViolet = 0x8b5cff; // levels, the Wristband rope
constexpr uint32_t IgSky = 0x4fc3f7;    // token Accent2 (day)
constexpr uint32_t IgLime = 0xc8ff4a;   // Sparks, "ready" pulses
constexpr uint32_t IgSun = 0xffc94a;    // token Gold: money, medals
constexpr uint32_t IgTang = 0xff9e3d;   // token Orange: the wake-up sky, the fuse's spark
constexpr uint32_t IgCream = 0xfff4ec;  // token Ink (day)
constexpr uint32_t IgRed = 0xff4d6d;    // token Red: BEEF, PAYBACK, KO (never a crimson near RunItTwice's Signal Red)
constexpr uint32_t IgDarkInk = 0x1a0f2a; // a label on a light fill (stickers, badges): cream on tangerine, sky or red reads under 3:1
// Afterburner Hours' own (SPEC §2.3: the night palette's Accent, Accent2, Ink, Bg and Panel).
constexpr uint32_t IgNightAccent = 0xb57bff;
constexpr uint32_t IgNightAccent2 = 0x3ee6d0;

/**
 * The hour's tones: the marquee's band and the boardwalk's sky by Ignite's mood (site::show::IgniteMood), and the shades
 * the art draws around the skin's tokens. Afterburner Hours swaps the coral family for violet (the mockup's swap), so a page
 * drawn at 2 AM never shows the day's coral beside the night skin.
 */
struct IgniteTones
{
	site::show::Mood Mood = site::show::Mood::PrimeTime;
	bool Night = false; // Afterburner Hours
	double Bpm = 124.0;
	const char* Label = "PRIME TIME";
	uint32_t BandA = 0, BandB = 0;              // the marquee's gradient band
	uint32_t SkyTop = 0, SkyMid = 0, SkyLow = 0; // the boardwalk's sky
	uint32_t Glow = 0;                          // the horizon's glow
	uint32_t Coral = 0, CoralHi = 0, CoralDeep = 0, CoralLight = 0, CoralDark = 0; // the accent family
	uint32_t Sky = 0, Cream = 0, Plum = 0;
	uint32_t Panel = 0, PanelTop = 0, PanelBot = 0; // card grounds
	uint32_t Well = 0;                              // a pill's ground
	uint32_t Rim = 0, RimHi = 0;                    // a card's and a pill's rims
	uint32_t CabTop = 0, CabBot = 0;                // a cabinet's body
};
IgniteTones IgniteTonesFor(site::show::Mood Mood);
/** The beat at world minute World (SPEC §2.3): 96 BPM in Afterburner Hours (midnight to 6 AM), 124 the rest of the day.
 * Everything that pulses on Ignite takes it from here, the final table's neon tube included. */
double IgniteBpmAt(double World);

// ------------------------------------------------------------------ type
/** BlackItalic (SPEC §1.11, §2.3): every title and number. */
TextStyle IgniteBi(float Size, const Color& Col, Align A = Align::Left, Baseline B = Baseline::Alphabetic, float MaxW = 0.0f);
/** Bold or regular body text (the shared Ts, kept here so the pages read one way). */
TextStyle IgniteTx(float Size, int Weight, const Color& Col, Align A = Align::Left, Baseline B = Baseline::Alphabetic, float MaxW = 0.0f);
/** Caps with letter spacing (NetSpaced's way, one glyph at a time); returns the width. */
float IgniteCaps(Canvas& C, const std::string& Text, float X, float Y, float Size, int Weight, const Color& Col, float Track, Align A = Align::Left);
Color IgniteA(const Color& C, float A);

// ------------------------------------------------------------------ the kit (IgniteArt.cpp)
/** A 4-point spark (Sparks, sparklets, the fuse's head). */
void IgniteSpark(Canvas& C, float Cx, float Cy, float R, const Color& Col, float Thin = 0.28f);
/** The layered flame (siteart::Logo's 24-point flame): X its centre, BaseY its foot, S its size (about 1.42 S tall). */
void IgniteFlame(Canvas& C, float X, float BaseY, float S, const Color& Outer, const Color& Mid, const Color& Core);
/** A star polygon of N points (stickers, medals, the GTD star). */
std::vector<Vec2> IgniteStar(float Cx, float Cy, float Ro, float Ri, int N, float RotDeg = -90.0f);
/** A starburst sticker with a cream outline and upright BlackItalic text (lines split at '\n'). Scale grows it from its centre. */
void IgniteSticker(Canvas& C, float Cx, float Cy, float R, const std::string& Label, const Color& Fill, const Color& Ink, float Size, int N = 16, float Ri = 0.8f,
	float Scale = 1.0f);
/** One candy chip in flight (the table's chips, SPEC §2.5): Kind picks its colours (0-5), R its radius; no shadow. */
void IgniteCandyChip(Canvas& C, float X, float Y, float R, int Kind, float Alpha = 1.0f);
/** A flat-topped hexagon (the level badge's shape). */
void IgniteHexagon(Canvas& C, float Cx, float Cy, float R, const Paint& Fill);
/** A pill (round ends) with an optional rim. */
void IgnitePill(Canvas& C, const Rect& R, const Paint& Fill, const Color& Rim, float RimW);
/** A ticket stub with semicircle notches, its two labels and a spark. */
void IgniteTicket(Canvas& C, float X, float Y, float W, float H, const std::string& Top, const std::string& Bottom, const Color& Col, const Color& Ink);
/** One marquee bulb at brightness Lit (0..1). */
void IgniteBulb(Canvas& C, float X, float Y, float R, const Color& On, const Color& Off, float Lit);
/** The bell (BUZZ), the dock's icons (0 play, 1 gift, 2 trophy, 3 wallet), a clock. */
void IgniteBell(Canvas& C, float Cx, float Cy, float S, const Color& Col);
void IgniteDockIcon(Canvas& C, int Kind, float Cx, float Cy, const Color& Col, const Color& Cut);
void IgniteClock(Canvas& C, float Cx, float Cy, float R, const Color& Col, double Minutes);
/** The Fuel Gauge (SPEC §2.6 F11): a half dial from empty to full with its needle at T (0..1). */
void IgniteFuelGauge(Canvas& C, float Cx, float Cy, float R, double T, const IgniteTones& K);
/** A wavy fuse from (X0, Y) to (X1, Y), burnt from the start up to T (0..1), the spark at the burn's head. */
void IgniteFuseRope(Canvas& C, float X0, float X1, float Y, float Amp, double T, const Color& Rope, const Color& Burnt, double Now, bool Spark);
/** Where the fuse's burn has reached (the rope's point index), and the spark there alone: a rope drawn without its spark
 * replays from the cache by its head, the spark crackles live. */
int IgniteFuseHead(float X0, float X1, double T);
void IgniteFuseSpark(Canvas& C, float X0, float X1, float Y, float Amp, double T, double Now);
/** A glow under a neon stroke: a few widening strokes at falling alpha (a cheap bloom). */
void IgniteNeonRoundRect(Canvas& C, const Rect& R, float Radius, const Color& Col, float Width, float Bloom);

// ------------------------------------------------------------------ the Boardwalk (SPEC §2.4 A; IgniteArt.cpp)
/** What the band's live layer and signs know this frame. */
struct IgniteBand
{
	IgniteTones K;
	double Now = 0.0;   // the presentation clock (a settled still while Previewing)
	double World = 0.0; // world minutes
	bool Snow = false;       // December to February
	bool Fireworks = false;  // the Sunday Blaze is running (Sun 7-9 PM)
	bool Wildfire = false;   // the series is running: bunting and bonfires
	uint64_t SkinKey = 0;    // Skin::Current().Key(), mixed into the static layer's cache key
};
/** The static skyline (the sky, the bay's far shore, the attractions' bodies, the deck and the water), cached per time bucket and skin. */
void IgniteBoardwalkStatic(Canvas& C, const IgniteBand& B);
/** The live layer: the bulbs on the beat, the lighthouse's beam, the water's shimmer, snow, fireworks, the series' bunting. */
void IgniteBoardwalkLive(Canvas& C, const IgniteBand& B);
/** The Spark Wheel on the boardwalk (SPEC §2.4: centre (790,214), r 118) and the sign at its foot. */
constexpr float IgniteWheelCx = 790.0f;
constexpr float IgniteWheelCy = 214.0f;
constexpr float IgniteWheelR = 118.0f;
constexpr Rect IgniteWheelSign{712.0f, 292.0f, 156.0f, 24.0f};
/** How far above the wheel's sign a placard (its rim included) must stay to show at all (G1_AUDIT L6). */
constexpr float IgnitePlacardGap = 3.0f;
/** Where label L's placard hangs at Angle (W its width): its car, its rect, and how much of it shows (0 tucked away). */
struct IgnitePlacard
{
	int Car = 0;
	Rect Tag;
	float Show = 1.0f;
};
IgnitePlacard IgnitePlacardAt(float Cx, float Cy, float R, float Angle, size_t L, float W, double Now);
/** The Ferris wheel (the Spark Wheel) turned by Angle; Labels ride three of its gondolas upright (G1: tonight's starts). */
void IgniteFerrisWheel(Canvas& C, float Cx, float Cy, float R, float Angle, const std::vector<std::string>& Labels, const IgniteTones& K, double Now);
/** A cabinet body (180 x 120, radius 18): its marquee band in Col, the rim lit when Selected or Hot (hovered). */
void IgniteCabinetBody(Canvas& C, const Rect& R, const Color& Col, bool Selected, float Glow, const IgniteTones& K, bool Hot = false);
/**
 * An event's mark on the floor: its family's glyph (EventArt's, the same the emblem tiles carry) in cream on a disc in the
 * family's colour, R its radius. The full EventArt tile is ~870 vertices; a floor of them would spend PLAY's whole budget
 * (SPEC §10: 34k), so the floor carries the glyph and the event sheet the full emblem.
 */
void IgniteEventMark(Canvas& C, const net::EventTemplate& T, float Cx, float Cy, float R, const IgniteTones& K);
/** The event's colour on the floor (its family's; a series' own). */
Color IgniteEventTint(const net::EventTemplate& T, const IgniteTones& K);

// ------------------------------------------------------------------ hashing (keyed, never rand())
/** fmix32 over a 32-bit hash (the lead's ruling on keyed draws: every Update 3 draw mixes Fnv1a). */
uint32_t IgniteMix(uint32_t H);
/** A keyed unit in [0, 1). */
double IgniteUnit(const std::string& Key);
} // namespace igniteart

namespace rlignite_detail
{
// ------------------------------------------------------------------ the copy (SPEC §1.10: every sentence is in IgniteInventory)
inline constexpr const char* IgniteStaysHere = "It stays on Ignite: it never pays rent.";
inline constexpr const char* IgniteStartsIt = "Your first deposit starts it: 100% of it, up to $100.";
inline constexpr const char* IgniteAllYours = "All of it's yours now.";
inline constexpr const char* IgniteWindowShut = "The 60 days ran out on {Date}. What wasn't released by then went with them.";
inline constexpr const char* IgniteBonusRule = "$5 is released for every $20 of rake you pay here, inside 60 days.";
inline constexpr const char* IgniteNoneOpen = "Nothing on the floor for this chip right now. Try All.";
inline constexpr const char* IgniteSeatsOpen = "Seats open an hour before the start.";
inline constexpr const char* IgniteNoRegulars = "No regulars in yet. The usual crowd turns up late.";
inline constexpr const char* IgniteClauseLine = "Under your {Team} deal, every table here is a strike.";
inline constexpr const char* IgniteNoNews = "Quiet on the boardwalk. News lands here as it happens.";
inline constexpr const char* IgniteNoCash = "No cash this time.";
inline constexpr const char* IgniteNoRecords = "No Ignite results yet. Your first one lands here.";
inline constexpr const char* IgniteNoLines = "Nothing here yet. Deposits, buy-ins and prizes show up as they happen.";
inline constexpr const char* IgniteNothingComing = "Nothing on its way.";
inline constexpr const char* IgniteDepositHow = "From your bank, by card. It lands at once.";
inline constexpr const char* IgniteWithdrawHow = "Back to your bank. It takes a while, and there's a fee.";
inline constexpr const char* IgniteOnlyBank = "Money moves between Ignite and your bank, never site to site.";
inline constexpr const char* IgniteNoBoard = "Nobody on this board yet. The first final table puts someone here.";
inline constexpr const char* IgniteCleanGame = "Clean game. No real mistakes found.";
inline constexpr const char* IgniteNoGrades = "No decisions graded.";
inline constexpr const char* IgniteDecisionsXp = "XP comes from decisions, not results.";
inline constexpr const char* IgniteNoShouts = "The boardwalk's quiet right now.";

/** The client's own small choices, by laptop (RiverLine.h is frozen: the SitesStates pattern). */
struct IgniteUi
{
	const Session* Owner = nullptr; // a laptop made at the same address for another career starts fresh
	// PLAY
	int Chip = 0;               // the filter chip (0 All .. 9 My tickets)
	double ChipAt = -100.0;
	std::array<float, 8> ShelfX{};    // each shelf's sideways scroll (eased), in cabinets
	std::array<float, 8> ShelfGoal{}; // where it's heading
	float FloorScroll = 0.0f;   // the shelves' vertical scroll (eased)
	float FloorGoal = 0.0f;
	std::string FollowId;       // the selection the shelves last scrolled to (arrow keys walk Listed)
	// The event sheet
	bool Sheet = false;
	double SheetAt = -100.0;
	int SheetTab = 0;           // Overview, Structure, Payouts, Players
	// REWARDS (G1: the Welcome bonus sub-page)
	int Rewards = 3;
	// BUZZ
	int Buzz = 0;               // Everything, Big hits (G2), Promos, Shouts
	float BuzzScroll = 0.0f;
	float BuzzGoal = 0.0f;
	// The Player Card
	int CardTab = 2;            // RECORDS at G1
	// BOARDS
	double BoardAt = -100.0;
	// "Your run"
	std::string ResultKey;      // the result on screen (its stage animations start when it changes)
	double ResultAt = -100.0;
	// Sounds already played for a stage (keyed, so a stage plays once)
	std::set<std::string> Played;
	// The table layer: when each seat's sticker appeared (its pop and its slam)
	std::map<std::string, double> Stickers;
	/**
	 * The schedule's picks (G1_AUDIT M3: each is a scan of a day or more of the schedule, and PLAY asked for them seven times a
	 * frame): the featured starts in IgniteFeatured's order before its cut, the next Sunday Blaze and the next Flip & Run. Taken
	 * once a world minute (the lobby's world runs at a minute a real minute; a status changes on a whole minute) and checked
	 * against Live every frame, so a pick that stops qualifying mid-minute is taken again at once.
	 */
	struct SchedulePicks
	{
		long long Minute = -(1LL << 62);
		std::vector<net::EventInstance> Featured;
		bool HasBlaze = false;
		net::EventInstance Blaze;
		bool HasFlip = false;
		net::EventInstance Flip;
	} Picks;
	/** A pot won at the table (SPEC §2.5 "Bets and pots"): its chips fountain from the pot to the winner, then "+4,200" floats up. */
	struct PotWin
	{
		int Seat = 0;
		Chips Amount = 0;
		double Start = 0.0; // the pot's flight (Session's Award: 0.6 s from the pot to the seat)
		double Dur = 0.6;
	};
	std::map<std::string, PotWin> Pots;
	/** The pot pill (SPEC §2.5 "the pot pill pulses as it grows", G1_AUDIT N4): the pot it last showed in this hand, the hand
	 * (the table's latest deal), and when and by how much (a share of the pot, 0..1) it last grew. */
	Chips PotSeen = 0;
	double PotHand = -1.0;
	double PotGrewAt = -100.0;
	float PotKick = 0.0f;
	/** The chat panel (G1_AUDIT N3): each item's wrapped lines, measured once (bounded), and the last frame's count of lines
	 * drawn and of bubbles among them. */
	struct ChatFit
	{
		std::vector<std::string> Lines;
		float W = 0.0f; // the bubble's width (a player's), or the widest line
		float H = 0.0f;
	};
	std::map<uint64_t, ChatFit> ChatFits;
	int ChatLines = 0;
	int ChatBubbles = 0;
	long long ChatFrames = 0;
};
IgniteUi& IgniteOf(const RiverLine* RL);
/** Drops a laptop's Ignite state (~RiverLine: ui-tablestyle's patch adds the call). */
void IgniteForget(const RiverLine* RL);
/** Opens the event sheet on an instance id (a cabinet, a sign, Play again; the tests, ui_seg_sites' clause rewrite). */
void IgniteOpenSheet(RiverLine& RL, const std::string& EventId, double Now);
/** Whether the event sheet is up. */
bool IgniteSheetOpen(const RiverLine* RL);
/** Plays an Ignite cue once for Key (a stage crossed), never while previewing. */
void IgniteCue(Session& S, IgniteUi& U, const std::string& Key, SoundId Id, float Vol, bool Preview);
/** REWARDS (Page::Promotions; RiverLineIgnitePlay.cpp). G1: the Welcome bonus sub-page with the Fuel Gauge (SPEC §2.6 F11). */
void IgniteRewards(RiverLine& RL, IgniteUi& U, const igniteart::IgniteTones& K, double Now, bool Preview);
/** The welcome bonus as the Fuel Gauge reads it (core-sites-look's site::show::Welcome, the account's own figures behind it). */
struct IgniteWelcome
{
	bool Started = false, Closed = false, Done = false;
	Chips Pending = 0, Released = 0, NextIn = 0;
	double Needle = 0.0;
	double Ends = 0.0;
};
IgniteWelcome IgniteWelcomeOf(const Session& S, double World);
/** Whether a start is tonight's (today's by the calendar, or before the coming 6 AM): what a sign or shout may call tonight. */
bool IgniteTonight(double Start, double World);

/** The copy Ignite's client draws (SPEC §1.10): every sentence (linted), the labels (screened), the 150 BUZZ shouts. */
std::vector<std::string> IgniteInventory();
std::vector<std::string> IgniteLabels();
const std::vector<std::string>& IgniteShouts();
/** The shout on BUZZ at world minute World (one every 10 world minutes, a keyed cycle: no line twice within 24 hours). */
int IgniteShoutAt(uint64_t WorldSeed, double World);

/** A pot won at the table (SPEC §2.5): six candy chips fountain from the pot to the winner, then "+4,200" floats for 1.4 s. */
constexpr int IgniteFountainChips = 6;
constexpr double IgniteFloatFor = 1.4;
/** Where fountain chip K is Since seconds into a pot's flight of Dur: how far along (0 the pot, 1 the winner), how high
 * over the straight line (its parabola, then one bounce where it lands), its sideways fan and its alpha; not shown before
 * its launch or once it has settled into the stack. Seed (the pot's) keys each chip's arc. */
struct IgniteHop
{
	bool Shown = false;
	float Along = 0.0f, Lift = 0.0f, Spread = 0.0f, Alpha = 1.0f;
};
IgniteHop IgniteHopAt(double Since, double Dur, int K, uint32_t Seed);

// ------------------------------------------------------------------ the capsule table's side panel and pot (RiverLineIgnite.cpp)
// SPEC §2.5 in Ignite's look, G1_AUDIT N3-N5: RiverLine::SidePanel and Table() call these at an Ignite table (patch
// fix2-ignite_to_lead_1). Each draws into C and keeps its state in the laptop's IgniteUi.

/** At most this many chat items (a player's run of lines, a room line) draw as bubbles, the newest; the older ones above them
 * stay as plain lines (the table's 22k vertices, SPEC §10, with a full chat at its busiest frame). */
constexpr int IgniteChatBubbles = 8;
/** The CHAT tab: rounded bubbles beside an avatar in the speaker's light (a speaker's lines in a row share one), the room's
 * lines on grape with a lime spark, the dealer's small and bare; newest at the bottom. Each bubble's wraps are measured once and
 * its geometry replays from the Boardwalk pool. */
void IgniteTableChat(Canvas& C, const RiverLine* RL, const std::vector<ChatLine>& Chat, const Rect& Box);
/** The STANDINGS tab: rank, each player's bubble in their light inside a level ring (its track; the level's arc is G2's), the
 * name and the stack in bb; your row glows and your ring is lit. */
void IgniteTableStandings(Canvas& C, const Tournament& T, const Rect& Box);
/** The pot pill under the board: Night with a coral rim, "POT 1,200" in BlackItalic; when the pot grows in a hand it pops
 * (scale and glow), the figure flashing sunflower. A still while previewing. */
void IgniteTablePotPill(Canvas& C, RiverLine& RL, const Rect& Pill, Chips Total, double Now);
/** The pot pill's pulse Since seconds after the pot grew by Kick (its growth's share of the pot, 0..1): the pill's scale, its
 * glow's alpha and how far the figure is lit (0 cream .. 1 sunflower). Settled (1, 0, 0) from IgnitePotPulseFor on. */
struct IgnitePotPulse
{
	float Scale = 1.0f, Glow = 0.0f, Flash = 0.0f;
};
constexpr double IgnitePotPulseFor = 0.45;
IgnitePotPulse IgnitePotPulseAt(double Since, float Kick);
/** A speaker's light in the chat and the standings: a hash into the arcade's six lights, the rival's Ignite red, yours the accent. */
Color IgniteNameLight(const std::string& Name, bool Rival, bool Hero);
/** Whether a BUZZ shout may be heard at world minute World (G1_AUDIT L6): a line that names a time of day only in its
 * hours ("Evening, boardwalk!" from 5 to 11 PM), any other "tonight" from 3 PM into the small hours, a Sunday line on Sundays. */
bool IgniteShoutFits(const std::string& Line, double World);
/** The time-of-day shouts and their hours (From to To, wrapping past midnight; Sunday: on Sundays only), for the tests. */
struct IgniteShoutHours
{
	const char* Line;
	int From, To;
	bool Sunday;
};
const std::vector<IgniteShoutHours>& IgniteShoutWindows();

/** The dock (SPEC §2.4): its pill, the raised centre button, its five items' ids and pages. */
constexpr float IgniteDockX = 380.0f;
constexpr float IgniteDockY = 884.0f;
constexpr float IgniteDockW = 840.0f;
constexpr float IgniteDockH = 64.0f;
constexpr float IgniteRaisedCx = 800.0f;
constexpr float IgniteRaisedCy = 892.0f;
constexpr float IgniteRaisedR = 56.0f;
/** The marquee's height (the preview crops it: SiteLook::ChromeH, ui-tablestyle's). */
constexpr float IgniteMarqueeH = 72.0f;

/**
 * RiverLine's friend (RiverLine.h: friend struct rlignite_detail::Access): static forwarders to the client's private state
 * and its gatekeepers, for the pages that live in files with no member function of their own (RiverLineIgnitePlay.cpp).
 */
struct Access
{
	using Page = RiverLine::Page;
	using ListRow = RiverLine::ListRow;
	using FeatureSlide = RiverLine::FeatureSlide;
	static Session& S(RiverLine& RL) { return RL.S; }
	static Canvas& C(RiverLine& RL) { return *RL.C; }
	static double World(const RiverLine& RL) { return RL.World; }
	static double Dt(const RiverLine& RL) { return RL.Dt; }
	static double PageAt(const RiverLine& RL) { return RL.PageAt; }
	static bool Previewing(const RiverLine& RL) { return RL.Previewing; }
	static Page Shown(const RiverLine& RL) { return RL.PageShown; }
	/** The page a frame settles on (one Ignite hasn't got goes to PLAY): no OpenPage side effects. */
	static void SetPage(RiverLine& RL, Page P) { RL.PageShown = P; }
	static const std::string& EventId(const RiverLine& RL) { return RL.EventId; }
	static double EventAt(const RiverLine& RL) { return RL.EventAt; }
	static void OpenPage(RiverLine& RL, Page P, double Now) { RL.OpenPage(P, Now); }
	static void SelectEvent(RiverLine& RL, const std::string& Id) { RL.SelectEvent(Id); }
	/** An empty selection taken up by "always select something" (NetLobby's rule), without SelectEvent's scroll request. */
	static void TakeEventId(RiverLine& RL, const std::string& Id) { RL.EventId = Id; }
	static bool TakeScrollToSelected(RiverLine& RL)
	{
		const bool Was = RL.ScrollToSelected;
		RL.ScrollToSelected = false;
		return Was;
	}
	static std::vector<std::string>& Listed(RiverLine& RL) { return RL.Listed; }
	/** The shared schedule (the gatekeeper: Joinable and its lock, the open statuses) with every filter off. */
	static std::vector<ListRow> ScheduleRows(RiverLine& RL)
	{
		const RiverLine::Filter Was = RL.FilterShown;
		RL.FilterShown = RiverLine::Filter::All;
		std::vector<ListRow> Out = RL.ScheduleRows();
		RL.FilterShown = Was;
		return Out;
	}
	static const std::vector<FeatureSlide>& FeatureSlides(RiverLine& RL) { return RL.FeatureSlides(); }
	static const std::vector<net::NewsItem>& NewsFeed(RiverLine& RL) { return RL.NewsFeed(); }
	static double NewsSeenAt(const RiverLine& RL) { return RL.NewsSeenAt; }
	static net::Board BoardShown(const RiverLine& RL) { return RL.BoardShown; }
	static void ShowBoard(RiverLine& RL, net::Board B, double Now) { RL.ShowBoard(B, Now); }
	static Vec2 SlotPos(const RiverLine& RL, int Seat) { return RL.SlotPos(Seat); }
	static bool CardOpen(const RiverLine& RL) { return RL.CardOpen(); }
	/** The shared gatekeepers (single-sourced: the strike gate, the money, the clause's two presses). */
	static void RegisterBlock(RiverLine& RL, const net::EventInstance& E, const net::LiveState& L, const Rect& R, double Now) { RL.RegisterBlock(E, L, R, Now); }
	static void Panel(RiverLine& RL, const Rect& R, float Radius) { RL.Panel(R, Radius); }
	/** The shared screens Ignite's frame holds (the table's logic, the series page, the player card, the toasts). */
	static void Table(RiverLine& RL, double Now) { RL.Table(Now); }
	static void Tiles(RiverLine& RL, double Now) { RL.Tiles(Now); }
	static void SeriesPage(RiverLine& RL, double Now) { RL.SeriesPage(Now); }
	static void PlayerCard(RiverLine& RL, double Now) { RL.PlayerCard(Now); }
	static void AutoPanel(RiverLine& RL, const Rect& R, double Now) { RL.AutoPanel(R, Now); }
	static void FinishedToasts(RiverLine& RL, double Now) { RL.FinishedToasts(Now); }
};
} // namespace rlignite_detail
} // namespace ui
} // namespace ss
