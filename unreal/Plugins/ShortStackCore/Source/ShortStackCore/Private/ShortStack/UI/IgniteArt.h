// Update 3 (SPEC §2.3-§2.5, §7.2): Ignite Poker's art kit and its client's private seams, ui-ignite's header (private to the
// UI sources). The palette constants (never in pal::, none a RiverLine hex: rival_sites' leak check would read one as a
// leak), the hour's tones, the drawing kit (IgniteArt.cpp), the client's per-laptop side state, and
// rlignite_detail::Access, RiverLine's friend: static forwarders to its private state and its gatekeepers (RegisterBlock,
// ScheduleRows, FeatureSlides), so the pages in RiverLineIgnitePlay.cpp reach them without a member function of their own
// and the gatekeepers stay single-sourced.
#pragma once

#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/SitePlay.h"
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

// ------------------------------------------------------------------ G2's art (SPEC §2.4 H, §2.6; IgniteArt.cpp)
/** The ring names (F4): GLOW, SHINE, DAZZLE; Ring 1..3. */
const char* IgniteRingName(int Ring);
/** The Flame by streak (F2): small 1-2 days, medium 3-6, big 7-13, roaring 14-29, blue from 30; on ice a frosted grey-blue.
 * X its centre, BaseY its foot, S the size the streak's band scales. */
void IgniteFlameLook(Canvas& C, float X, float BaseY, float S, int Streak, bool Ice);
/** The Spark Wheel face (REWARDS and the spin): N wedges in alternating grape and violet round a hub, its three rings
 * (GLOW outer, SHINE middle, DAZZLE inner) lit up to Lit, turned by Angle (radians). */
void IgniteWheelFace(Canvas& C, float Cx, float Cy, float R, int Wedges, int Lit, float Angle, const IgniteTones& K, float Glow);
/** The face's rings sit at R + 6 (GLOW, the rim), 0.7 R (SHINE) and 0.4 R (DAZZLE), a lit one stroked 4 wide. The prizes
 * ride the band between SHINE and the rim, off every ring (G2_AUDIT L9): its middle radius, and how far a label may reach
 * either side of it (the strokes' half widths and 3 px of clear felt, so "1,050" across the wheel never touches the rim). */
constexpr float IgniteWheelBandMid(float R)
{
	return (R * 0.7f + R + 6.0f) * 0.5f;
}
constexpr float IgniteWheelBandRoom(float R)
{
	return (R + 6.0f - R * 0.7f) * 0.5f - 5.0f;
}
/** A cracker (F5: Paper 0, Neon 1, Gold 2) centred at (Cx, Cy), W wide; Tug (0..1) pulls its ends apart. */
void IgniteCracker(Canvas& C, float Cx, float Cy, float W, int Tier, float Tug);
/** A matchstick (a Spare Match), Lit: its head burning; Spent: burnt down. */
void IgniteMatch(Canvas& C, float X, float Y, float H, bool Lit, bool Spent);
/** The Wristband's violet rope from (X0, Y) to (X1, Y), filled up to T (0..1) in lime. */
void IgniteRope(Canvas& C, float X0, float X1, float Y, float Amp, double T, uint32_t Seed);
/** A level's hexagon badge with its number (the ring's, the card's, the banner's). */
void IgniteLevelBadge(Canvas& C, float Cx, float Cy, float R, int Level);
/** FUSE's multiplier chip colours (§2.4 C): x2-x3 cream, x5-x10 sky, x25 coral, x100 and up sunflower. */
uint32_t IgniteFuseTint(int M, const IgniteTones& K);

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
// G2 (SPEC §2.3's voice: playful, warm, second person, short; every sentence in IgniteInventory)
inline constexpr const char* IgniteFlameNeeds = "Your flame needs one entry today.";
inline constexpr const char* IgniteOnIce = "On ice: your flame waits.";
inline constexpr const char* IgniteSpinReady = "Spin's ready.";
inline constexpr const char* IgniteSpinDone = "Today's spin is done. The wheel lights again at 6 AM.";
inline constexpr const char* IgniteSpinNone = "One paid entry today lights GLOW.";
inline constexpr const char* IgniteAllThree = "That's 3 of 3: pull your cracker.";
inline constexpr const char* IgniteNoCrackers = "No crackers on the shelf. Clearing all three Dares drops one.";
inline constexpr const char* IgniteShelfFull = "The shelf holds 12. A 13th opens itself into Sparks.";
inline constexpr const char* IgniteNeverSold = "Crackers are never sold. They drop from Dares, levels and the Wristband.";
inline constexpr const char* IgniteSparksRule = "10 Sparks for every cent of rake on a paid entry. Sparks never expire.";
inline constexpr const char* IgniteSparksStay = "Sparks buy tickets and looks here. Money never buys Sparks.";
inline constexpr const char* IgniteTicketsEarn = "Ticket and freeroll entries earn nothing.";
inline constexpr const char* IgniteDaresGrey = "Under your deal, a Dare's entry here is a strike.";
inline constexpr const char* IgniteMondayGood = "Past this week's cap your base Sparks still count, and they land on Monday at 6 AM.";
inline constexpr const char* IgniteGuestRule = "Nothing on the Guest List expires. The oldest item always moves first.";
inline constexpr const char* IgniteFineRule = "Ignite gives back at most half the rake you pay here, counted over 30 days. The rest waits on the Guest List.";
inline constexpr const char* IgniteFineWelcome = "Inside your 60 welcome days the ceiling is 75%: the welcome bonus counts in it.";
inline constexpr const char* IgniteNoHolds = "Nothing waiting. Every reward so far has landed.";
inline constexpr const char* IgniteFuseHow = "{Stack} chips \xC2\xB7 {Blinds} \xC2\xB7 3-minute levels \xC2\xB7 winner takes all to x10; x25 and up pay 80 / 12 / 8 \xC2\xB7 the fuse is the blind clock \xC2\xB7 it earns Sparks";
inline constexpr const char* IgniteFuseLine = "three players \xC2\xB7 one fuse \xC2\xB7 light it and see what you're playing for";
inline constexpr const char* IgniteFuseSkip = "A click skips to the boom. The table opens itself a second after.";
inline constexpr const char* IgniteFuseKeyed = "Drawn when you lit it. A reload replays this reveal.";
inline constexpr const char* IgniteFuseOdds = "Every FUSE is drawn from this table. The pool is exactly the multiplier times the buy-in.";
inline constexpr const char* IgniteNoWall = "No FUSE of x10 or more yet. Your first one goes up here.";
inline constexpr const char* IgniteWheelOdds = "The wheel stops where it was drawn. These are its chances at your stake.";
inline constexpr const char* IgniteCrackerOddsLine = "A cracker's prize is drawn the moment you pull it, from these chances. A reload never changes it.";
inline constexpr const char* IgniteCrackerOwned = "A look you already own comes as {N} Sparks instead.";
inline constexpr const char* IgniteBuyAsk = "Spend the Sparks? This can't be undone.";
inline constexpr const char* IgniteLockerEmpty = "Wear what you've got. Locked looks say how they're had.";
inline constexpr const char* IgniteOuchLine = "Ouch. That was {An} {Pct}% favourite. Have some Sparks.";
inline constexpr const char* IgniteBeefNone = "Nobody's got your number yet.";
inline constexpr const char* IgniteSecretSticker = "A secret. Nobody says how.";

/**
 * The loops as Ignite's G2 pages draw them (SPEC §2.6): Session::SitePlayView(Ignite)'s fields (core-siteplay's View) with
 * the Governor's figure (the Fine print), read through IgniteLoopsOf, the one adapter. Every number here is core's: the
 * pages only animate it (W2). While Live is false (no G2 View yet) every surface keeps its G1 state.
 */
struct IgniteDare
{
	int Id = -1;
	int Tier = 0; // 0 Easy, 1 Medium, 2 Hard
	std::string Label;
	int Prog = 0, Needs = 1;
	int Entries = 1; // the paid entries today it needs before it can complete (1 Easy, 2 Medium, 3 Hard: core's floor)
	int64_t Sparks = 0;
	bool Done = false, Claimed = false, Held = false;
};
/** The paid entries a Dare still needs today before it completes, once its own target is met (0: none, or not yet met). */
inline int IgniteDareShort(const IgniteDare& D, int EntriesToday)
{
	return D.Id >= 0 && !D.Done && !D.Claimed && D.Prog >= D.Needs && EntriesToday < D.Entries ? D.Entries - EntriesToday : 0;
}
struct IgniteWedge
{
	std::string Label;
	int64_t Sparks = 0;
	std::string TicketId;
	Chips FaceCents = 0;
	double P = 0.0;
};
struct IgniteTier
{
	int Tier = 0;
	int64_t Needs = 0;
	std::string Reward;
	int Kind = 0; // 0 nothing, 1 ticket, 2 cracker, 3 Spare Match, 4 look, 5 the month's title
	bool Reached = false, Claimed = false, Held = false;
};
struct IgnitePrize
{
	int Item = 0;
	std::string Label;
	int64_t Sparks = 0;
	std::string TicketId;
	Chips FaceCents = 0;
	int LookSlot = -1, Look = -1;
	int Tab = 0; // 0 Tickets, 1 Looks, 2 Extras
	bool Owned = false;
};
struct IgniteLook
{
	int Slot = 0, Look = 0;
	std::string Name, How;
	uint32_t ColorA = 0, ColorB = 0; // a Wristband month's two theme colours (0: the base design's own)
	bool Owned = false, Worn = false;
};
struct IgniteStickerRow
{
	int Id = 0;
	std::string Name, How;
	bool Got = false, Secret = false;
};
struct IgniteMoment
{
	int Kind = 0; // 0 Payback, 1 Scalp, 2 Ouch
	std::string Name, HandId;
	int64_t Sparks = 0;
	double Equity = 0.0, World = 0.0;
	bool Held = false;
};
struct IgniteOpened
{
	std::string BoxId;
	int Tier = 0;
	std::string Prize;
	int64_t Sparks = 0;
	std::string TicketId;
	int LookSlot = -1, Look = -1;
	bool Sticker = false, Held = false;
};
struct IgniteEntry
{
	std::string EventId;
	int64_t Base = 0, Sparks = 0;
	double Flame = 1.0, Theme = 1.0;
	bool Banked = false;
	std::array<int, 3> DareTicks{};
	int RingLit = 0;
	int LevelFrom = 0, LevelTo = 0, TierFrom = 0, TierTo = 0;
};
/** A Cracker's Odds (F5, W5; G2_AUDIT M2): core's site::play::CrackerOddsView in the pages' shape. Part 0 rows are the
 * Sparks (exactly one, their chances summing to 1), a Part 1 row the ticket on top, Part 2 rows the look pieces on top. */
struct IgniteCrackerRow
{
	int Part = 0;
	std::string Label;
	int64_t Sparks = 0;
	std::string TicketId;
	Chips FaceCents = 0;
	int LookSlot = -1, Look = -1;
	double P = 0.0;
};
struct IgniteCrackerOdds
{
	int Tier = 0;
	std::string Name, From;
	std::vector<IgniteCrackerRow> Rows;
	double TicketP = 0.0, LookP = 0.0;
	int64_t DuplicateSparks = 0;
	double EvSparks = 0.0;
};
/** Core's view of a Cracker's Odds in the pages' shape (the adapter's copy; the drawer reads core's pure CrackerOdds when a
 * stand-in left them empty). */
IgniteCrackerOdds IgniteCrackerOddsFrom(const site::play::CrackerOddsView& V);
struct IgniteHold
{
	std::string What;
	Chips Cents = 0, Covered = 0, RakeLeft = 0;
	int Entries = 0; // about how many of the site's cheapest paid entries the rake left is (core's)
	int Day = 0;
	int64_t Sparks = 0; // the "Sparks owed" line's Sparks
	bool SparksOwed = false, Reset = false;
};
struct IgniteLoops
{
	bool Live = false;    // G2's View is here; false: every surface keeps its G1 state
	bool Started = false; // the book exists
	bool FuseOn = false;  // FUSE is lit; false: the centre button reads FLIP
	bool OnIce = false;   // a clause strikes Ignite
	int Band = 0;
	int64_t Sparks = 0;
	// The Flame
	int Streak = 0, Best = 0, Matches = 0;
	double Mult = 1.0;
	bool LitToday = false;
	bool Nudge = false; // after 6 PM with no flame day yet (core's)
	std::array<int, 7> Week{};
	// Dares
	std::array<IgniteDare, 3> Dares{};
	double DaresReset = 0.0;
	int WeekClears = 0;
	int ClearCracker = 0;
	bool ClearPaid = false;
	// The Spark Wheel
	int EntriesToday = 0, RingsLit = 0;
	bool SpinReady = false, Spun = false, SpunHeld = false;
	int SpunRing = 0, SpunWedge = -1, MoreForNext = 1;
	std::string NearRing; // core's "1 more entry lights SHINE" ("" at DAZZLE; IgniteNearRing words it when core hasn't)
	std::array<std::vector<IgniteWedge>, 3> Rings;
	// Crackers
	std::vector<site::play::Box> Crackers;
	std::vector<IgniteOpened> Opened;
	std::array<IgniteCrackerOdds, 3> CrackerOdds{}; // Paper, Neon, Gold: core's Odds (W5, G2_AUDIT M2)
	// The Wristband and the Level
	int Tier = 0;
	int64_t BandSparks = 0;
	std::string Theme;
	int DaysLeft = 0;
	std::array<IgniteTier, 30> Tiers{};
	int Level = 1;
	std::string Title = "Rookie";
	int64_t LevelSparks = 0, LevelFrom = 0, LevelTo = 100;
	// The Prize Counter, the Locker, the Sticker Book
	std::vector<IgnitePrize> Prizes;
	site::play::Looks Worn;
	std::vector<IgniteLook> Locker;
	std::vector<IgniteStickerRow> Book;
	// FUSE
	site::play::FuseRec Fuse;
	std::array<std::string, 3> FuseTicketIds;
	// People
	std::vector<std::string> Headliners;
	std::vector<site::play::Beef> Beefs;
	std::string BeefName;
	int ScalpLife = 0;
	bool Guest = false;
	bool CrowdKnows = false; // a first Ignite cash of $100 or more: shouts may use the handle
	std::vector<IgniteMoment> Moments;
	// The night
	int ThemeNight = 0;
	std::string ThemeName, ThemeTwist;
	IgniteEntry Last;
	// The Governor on Ignite (the Fine print)
	Chips Rake30 = 0, Promo30 = 0;
	double Share = 0.0, Ceiling = 0.5;
	bool Welcome = false;
	Chips WeekFace = 0, WeekCap = 0;
	int64_t MondayBank = 0;
	std::vector<IgniteHold> Holds;
	std::vector<std::pair<std::string, Chips>> Months;
	std::string GovLine; // core's "Ignite gave back $3.65 of your $8.72 rake in the last 30 days (42%)."
};

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

	// ---- G2 (SPEC §2.4, §2.6): the loops' side state. Every outcome here is core's; these are only stages and choices.
	/** The loops this frame (IgniteLoopsOf reads the View once a frame), and a test's stand-in (IgniteFakeLoops). */
	IgniteLoops Loops;
	double LoopsAt = -1.0;
	bool Faked = false;
	IgniteLoops Fake;
	/** REWARDS' sub-page (0 Today, 1 Wristband, 2 Prize Counter, 3 Welcome bonus) and the Prize Counter's tab. */
	int PrizeTab = 0;
	/** The Prize Counter's confirm card (an item, -1 none): a BUY opens it, so a one-frame click never spends Sparks. */
	int BuyItem = -1;
	double BuyAt = -100.0;
	/** The overlays: the wheel's spin, a cracker's open (OPEN ALL chains them), the level-up banner, the drawers. */
	bool Spin = false;
	double SpinAt = -100.0;
	int SpinRing = 0, SpinWedge = -1;
	bool Crack = false;
	std::string CrackId;     // the cracker up ("*": OPEN ALL); pulled when CrackAt is stamped
	double CrackAt = -100.0; // when core opened it (the tug and the bang run from here); < 0: waiting on PULL
	size_t CrackShown = 0; // OPEN ALL: which of the opened is up
	std::vector<IgniteOpened> Cracked;
	int LevelSeen = 0;
	double LevelAt = -100.0;
	int LevelShown = 0;
	int64_t SparksSeen = -1;
	int64_t SparksDelta = 0;
	double SparksAt = -100.0;
	int Drawer = 0; // 0 none, 1 the wheel's odds, 2 FUSE's odds, 3 the Fine print, 4 the Crackers' odds
	double DrawerAt = -100.0;
	/** The FUSE reveal (§2.4 D): the FUSE's event id, the table it sat at, when it started; RevealSkip: when the odds drawer
	 * opened over it closed (-1 none), the table opening itself a second after. */
	std::string RevealEvent;
	int RevealTable = -1;
	double RevealAt = -100.0;
	double RevealSkip = -1.0;
	std::set<std::string> RevealsDone;
	/** The FUSE table's blind clock: the level it last slammed "BLINDS UP!" for, and when. */
	std::string BlindsTable;
	int BlindsLevel = 0;
	double BlindsAt = -100.0;
	/** A claim core refused ("" none): its reason under the button for a few seconds. */
	std::string Note;
	std::string NoteId;
	double NoteAt = -100.0;
	/** The Player Card's Locker slot shown. */
	int LockerSlot = 0;
	/** BOARDS: the Fuse Wall tab is up (G2). */
	bool BoardWall = false;
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
/** REWARDS (Page::Promotions; RiverLineIgnitePlay.cpp): Today (Dares, the Flame, the Spark Wheel, Crackers, the Wristband's
 * next tiers, the theme nights), the Wristband, the Prize Counter and the Welcome bonus with its Fuel Gauge (SPEC §2.4 H). */
void IgniteRewards(RiverLine& RL, IgniteUi& U, const igniteart::IgniteTones& K, double Now, bool Preview);

// ------------------------------------------------------------------ G2: the loops (RiverLineIgnitePlay.cpp)
/** The loops this frame: core's View through the one adapter (once a frame), or a test's stand-in. */
const IgniteLoops& IgniteLoopsOf(RiverLine& RL, double Now);
/** A test's stand-in for the View (until core-siteplay's lands, and for states a fresh career can't reach); null drops it. */
void IgniteFakeLoops(const RiverLine* RL, const IgniteLoops* L);
/** Copies core's View into the pages' shape (the adapter; Live stays false while the View is G1's empty struct). */
IgniteLoops IgniteLoopsFrom(const Session& S, double World);
/** The base Sparks an entry earns at this buy-in on Ignite (10 a cent of its rake, SPEC §2.6 F1): the cabinets' "+610". */
int64_t IgniteBaseSparks(Chips BuyInCents);
/** A Wristband tier that waits on CLAIM: reached, not claimed, and with something on it (core refuses a claim on a tier
 * with nothing, Kind 0, and never marks one claimed, so such a tier is simply done once reached). */
inline bool IgniteTierClaimable(const IgniteTier& T)
{
	return T.Reached && !T.Claimed && T.Kind != 0;
}
/** A multiplier as the glyph rule draws it: a vector cross, then the number in BlackItalic ("x25"); returns the width. */
float IgniteTimes(Canvas& C, float X, float Y, float Size, int M, const Color& Col, Align A);
/** The same with a figure as written ("1.20", "1.5"). */
float IgniteTimesText(Canvas& C, float X, float Y, float Size, const std::string& N, const Color& Col, Align A);
/**
 * The glyph rule for copy that carries a multiplier (SPEC §1.10, G2_AUDIT L9): strings write "x25", and a drawn label draws
 * the vector cross in its place. IgniteHasTimes says whether S has one (an 'x' starting a word, then a digit); IgniteLineTimes
 * draws a line with each one crossed (a line without draws exactly as C.Text; one past St.MaxWidth falls back to it), and
 * IgniteParagraphTimes is NetParagraph's wrap with each line drawn so. Both return what their plain forms return.
 */
bool IgniteHasTimes(const std::string& S);
float IgniteLineTimes(Canvas& C, const std::string& S, float X, float Y, const TextStyle& St);
float IgniteParagraphTimes(Canvas& C, const std::string& Text, float X, float Y, float MaxW, float Size, int Weight, const Color& Col, float LineH, int MaxLines = 99);
/** FUSE (Page::Fuse, SPEC §2.4 C): the three cabinets, LIGHT IT through the two presses, the last ten, the Fuse Wall. */
void IgniteFusePage(RiverLine& RL, IgniteUi& U, const igniteart::IgniteTones& K, double Now, bool Preview);
/** The Fuse Wall (your FUSEs of x10 and up) with your FUSE numbers: the FUSE page's right card, BOARDS' Fuse Wall tab. */
void IgniteFuseWallCard(RiverLine& RL, const IgniteLoops& L, const igniteart::IgniteTones& K, const Rect& R);
/** Which overlay is up (one at a time): 0 none, 1 the FUSE reveal, 2 the wheel's spin, 3 a cracker, 4 the Prize Counter's
 * confirm, 5 a drawer (odds, the Fine print). */
int IgniteOverlay(const RiverLine& RL, const IgniteUi& U, double Now);
/** The overlay's rect (its SitesBlockUnder: modal ones hold the whole client). */
Rect IgniteOverlayRect(int Overlay, const IgniteUi& U);
/** Draws the overlay that's up, after the page; a scrim click (B.Scrim) closes or skips it. */
void IgniteOverlayDraw(RiverLine& RL, IgniteUi& U, const igniteart::IgniteTones& K, double Now, bool Preview, bool Scrim);
/** The level-up banner (SPEC §2.4 H: drops from the marquee, blocks its rect, dismisses itself at 3.5 s). */
constexpr Rect IgniteBannerRect{500.0f, 90.0f, 600.0f, 150.0f};
bool IgniteBannerUp(const IgniteUi& U, double Now);
void IgniteBanner(RiverLine& RL, IgniteUi& U, const igniteart::IgniteTones& K, double Now, bool Preview);
/** Starts the wheel's spin overlay on core's draw (after Session::SpinWheel said ""). */
void IgniteStartSpin(IgniteUi& U, const IgniteLoops& L, double Now);
/** Starts a cracker's open overlay on what core opened (after Session::OpenCracker said ""). */
void IgniteStartCrack(IgniteUi& U, const IgniteLoops& L, double Now);
/** Starts the FUSE reveal for the FUSE at table Table (after Session::LightFuse said ""). */
void IgniteStartReveal(IgniteUi& U, const Session& S, int Table, double Now);
/** The reveal's stages for a multiplier (§2.4 D): when the fuse is out, the reel lands, the boom ends (seconds). */
struct IgniteRevealTimes
{
	double FuseEnd = 0.6, ReelEnd = 1.2, BoomEnd = 1.5;
};
IgniteRevealTimes IgniteRevealTimesFor(int M);
/** How rare multiplier M is at a tier (the reveal's line under BIG FUSE, W5), from its own row of core's table: "1 in 250 at
 * $1.10" where the chance is a whole one-in to within 0.5% ("1 in 1,667 at $3.30" for 0.06%), else "6.5% at $1.10"; "" for a
 * multiplier not on the tier's table. */
std::string IgniteRevealOdds(int M, int Tier);
/** The reel's strip (W5): the symbol at position I (it lands at IgniteReelFinal), the six before the result the rungs below it. */
int IgniteReelFinal(int M, int Tier);
int IgniteReelAt(int M, int Tier, int I);
/** The 3-max FUSE table's prize marquee and fuse clock, over the shared table (RiverLine::IgniteTableLayer calls it). */
void IgniteFuseTable(RiverLine& RL, IgniteUi& U, const igniteart::IgniteTones& K, double Now, bool Preview);
/** The Player Card's G2 tabs: the Locker and the Sticker Book (RiverLineIgnitePlay.cpp). */
void IgniteLocker(RiverLine& RL, IgniteUi& U, const igniteart::IgniteTones& K, const Rect& Box, double Now, bool Preview);
void IgniteStickerBook(RiverLine& RL, IgniteUi& U, const igniteart::IgniteTones& K, const Rect& Box, double Now, bool Preview);
/** A claim core refused: its reason under the button (Id) for 3 s. */
void IgniteNoteRefusal(IgniteUi& U, const std::string& Id, const std::string& Why, double Now);
/** "1 more entry lights SHINE" (always "1 more" or "2 more"); "" at DAZZLE. Core's words when it has them. */
std::string IgniteNearRing(const IgniteLoops& L);
/** The same in the pages' own short words (the dock's strip and any line core's words don't fit). */
std::string IgniteNearRingShort(const IgniteLoops& L);
/** The marquee's Flame pill and Sparks pill, and the dock's Wristband and wheel strips (RiverLineIgnitePlay.cpp). */
void IgniteSparksPill(RiverLine& RL, IgniteUi& U, const IgniteLoops& L, const igniteart::IgniteTones& K, double Now, bool Preview, double World);
void IgniteFlamePill(RiverLine& RL, IgniteUi& U, const IgniteLoops& L, const igniteart::IgniteTones& K, double Now, bool Preview, double World);
void IgniteDockBand(RiverLine& RL, IgniteUi& U, const IgniteLoops& L, const igniteart::IgniteTones& K, double Now, bool Preview);
void IgniteDockWheel(RiverLine& RL, IgniteUi& U, const IgniteLoops& L, const igniteart::IgniteTones& K, double Now, bool Preview);
/** The Today strip at G2 (the Flame, three Dare chips, the near-goal line; SPEC §2.4 A). */
void IgniteTodayLoops(RiverLine& RL, IgniteUi& U, const IgniteLoops& L, const igniteart::IgniteTones& K, double Now, bool Preview, double World);
/** The near-goal line (the most useful one this minute): "1 more entry lights SHINE", "380 Sparks to Level 8", ... */
std::string IgniteNearGoal(const IgniteLoops& L, double World);
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
/** The chips each fountain flies while Flying fountains are in the air at once (G2_AUDIT M5: a split pot's fountains fly over
 * the showdown's glows, the table's busiest frames): 6 for one, 5 each for two, 4 each for three or more; only the first
 * of them trails sparklets. */
constexpr int IgniteFountainFor(int Flying)
{
	return Flying <= 1 ? IgniteFountainChips : Flying == 2 ? 5 : 4;
}
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
	static const Session& SessionOf(const RiverLine& RL) { return RL.S; }
	static Canvas& C(RiverLine& RL) { return *RL.C; }
	static double World(const RiverLine& RL) { return RL.World; }
	static double Dt(const RiverLine& RL) { return RL.Dt; }
	static double PageAt(const RiverLine& RL) { return RL.PageAt; }
	static bool Previewing(const RiverLine& RL) { return RL.Previewing; }
	/** The stream preview's flag, for rivals_ui's stills (the client draws a settled still and asks core for nothing). */
	static void SetPreviewing(RiverLine& RL, bool On) { RL.Previewing = On; }
	static Page Shown(const RiverLine& RL) { return RL.PageShown; }
	/** The page a frame settles on (one Ignite hasn't got goes to PLAY): no OpenPage side effects. */
	static void SetPage(RiverLine& RL, Page P) { RL.PageShown = P; }
	static const std::string& EventId(const RiverLine& RL) { return RL.EventId; }
	static double EventAt(const RiverLine& RL) { return RL.EventAt; }
	static void OpenPage(RiverLine& RL, Page P, double Now) { RL.OpenPage(P, Now); }
	/** The lobby's site (as RunItTwice's results set it): Ignite's pages stay Ignite's once the results close. */
	static void ShowSite(RiverLine& RL, net::SiteId Site) { RL.SiteShown = Site; }
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
