// Update 3 (SPEC §3.3-§3.5, §13; v1.2's red and black): RunItTwice's private header, ui-rit's. The palette (the 16 skin
// tokens core-sites-look writes into Sites.cpp, and the extensions, which live only here: never in pal::, none a RiverLine
// hex), the two signatures (the twin stripe and the twice print), the shapes and marks the frame and its pages share, the
// screen state kept beside the class (RiverLine.h is frozen: the pointer-keyed map pattern, as rlsites_detail::SitesOf), the
// rlrit_detail::Access forwarders (RiverLine.h's friend; bodies in RiverLineRitLeague.cpp) and the copy.
//
// Red is light, never ink (§3.3): Signal Red fills, strokes and glows; text is Ink, Hot White, Chrome, Muted or a semantic
// colour. The only red letters are Signal Down (beside a minus or a vector triangle), the twice print's under-copies, the
// Settlement's stamp on paper and the logo's "Twice" (siteart::Logo, core-sites-look's).
#pragma once

#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/SiteShow.h"
#include "ShortStack/UI/RiverLine.h"
#include "ShortStack/UI/Ui.h"

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace ss
{
namespace ui
{
namespace rlrit_detail
{
// ------------------------------------------------------------------ the palette (§3.3 table, §13.2 #1-#2)

// The 16 skin tokens, as core-sites-look sets them in Sites.cpp (pal:: holds them inside RunItTwice's SkinScope).
constexpr uint32_t RitOnyx = 0x0c0c0f;    // Bg: the page's ground
constexpr uint32_t RitBg2 = 0x111114;     // the rail's and the header's ground
constexpr uint32_t RitCarbon = 0x16161a;  // Panel: rows, cards, data cards
constexpr uint32_t RitPanel2 = 0x1e1e23;  // the zebra's second row
constexpr uint32_t RitHair = 0x2c2c33;    // Line: 1-unit rules
constexpr uint32_t RitInk = 0xececef;     // Frost: text
constexpr uint32_t RitMuted = 0xa0a0aa;   // secondary text, labels
constexpr uint32_t RitDim = 0x62626c;     // disabled and decorative only (3.24:1: never information)
constexpr uint32_t RitRed = 0xd2141a;     // Signal Red (Accent): the one accent, light never ink
constexpr uint32_t RitChrome = 0xb6bbc5;  // Accent2: secondary data
constexpr uint32_t RitWhite = 0xffffff;   // Hot White (the Gold token): money, labels on red
constexpr uint32_t RitDown = 0xff3d3d;    // Signal Down (Red): losses, with a minus or a triangle
constexpr uint32_t RitAmber = 0xf5a524;   // Caution Amber (Orange): projected overlays
constexpr uint32_t RitUp = 0x48c77a;      // Signal Up (Green): gains
constexpr uint32_t RitCloth = 0x1a1a1f;   // Felt
constexpr uint32_t RitClothDark = 0x09090b; // FeltDark
// The extensions (RitArt's own).
constexpr uint32_t RitTape = 0x060607;      // Tape Black: the tape, scrims, the lower third
constexpr uint32_t RitGloss = 0x202026;     // a lacquer gradient's top (to Onyx)
constexpr uint32_t RitTwillA = 0x141417;    // the carbon twill's two tones
constexpr uint32_t RitTwillB = 0x18181c;
constexpr uint32_t RitDeep = 0x7a0a12;      // Deep Red: the twice print's under-copy on red grounds
constexpr uint32_t RitLed = 0xff2633;       // LED Red: 1-unit cores inside red light; never body text
constexpr uint32_t RitChromeDark = 0x8d929c; // the Chrome ramp
constexpr uint32_t RitChromeLight = 0xf3f4f6;
constexpr uint32_t RitChromeMid = 0xaab0ba;
constexpr uint32_t RitGoldDark = 0xa8822f;  // Status Gold: the Gold division and the High Table card only (never money)
constexpr uint32_t RitGoldLight = 0xe6c66a;
constexpr uint32_t RitGoldMid = 0xc9a24a;
constexpr uint32_t RitPool = 0x26262d;      // the cloth's downlight
constexpr uint32_t RitSelRow = 0x34161a;    // the selected Board row's fixed ground (Signal Red at 0.16 over Carbon)
constexpr uint32_t RitFinalDim = 0x7d151a;  // FINAL CALL's dim phase
constexpr uint32_t RitNightRow = 0x291d22;  // night service's Panel2 row (a 6% red cast)
constexpr uint32_t RitPaper = 0xf4f1ea;     // the Settlement's slip, Club Day
constexpr uint32_t RitPaperInk = 0x141416;
constexpr uint32_t RitPaperMuted = 0x5a5a62;
constexpr uint32_t RitPromo = 0x1c2b26;     // the promotion tint (League rows, G2)
constexpr uint32_t RitFlapTop = 0x2b2b32;   // a gloss flap tile's lit upper half
constexpr uint32_t RitFlapLow = 0x0d0d10;   // and its lower half
constexpr uint32_t RitAmberGround = 0x1a150d; // the projected-overlay box's ground
constexpr uint32_t RitIronBar = 0x3d3d46;   // Iron's carbon as a bar on a Carbon panel (the Pyramid's)

/** A palette hex as a colour. */
inline Color RitC(uint32_t Rgb, float A = 1.0f)
{
	return Hex(Rgb, A);
}

// ------------------------------------------------------------------ type (§3.3: upright and condensed)

/** The faces: BoldCondensed for display (titles, big numbers, the Board's cells, labels in caps), Regular body, Bold names, Mono. */
enum class RitFace : uint8_t
{
	Cond,
	Body,
	Name,
	Mono,
};
TextStyle RitStyle(float Size, const Color& Col, RitFace Face = RitFace::Cond, Align A = Align::Left, Baseline B = Baseline::Alphabetic, float MaxW = 0.0f);
/** Draws a run; returns its width. */
float RitText(Canvas& C, const std::string& S, float X, float Y, float Size, const Color& Col, RitFace Face = RitFace::Cond, Align A = Align::Left,
	Baseline B = Baseline::Alphabetic, float MaxW = 0.0f);
float RitMeasure(const Canvas& C, const std::string& S, float Size, RitFace Face = RitFace::Cond);
/** Capitals, ASCII only (names stay as typed elsewhere). */
std::string RitUpper(const std::string& S);
/** A label in BoldCondensed capitals (11-15). */
float RitLabel(Canvas& C, const std::string& S, float X, float Y, float Size, const Color& Col, Align A = Align::Left);
/** Tracked capitals drawn a glyph at a time (the kickers a test spells: "SECOND RUN CLUB", "WELCOME TICKETS"). */
float RitSpaced(Canvas& C, const std::string& S, float X, float Y, float Size, const Color& Col, float Track);
/** The largest size in [Min, Size] at which S fits MaxW (BoldCondensed). */
float RitFit(const Canvas& C, const std::string& S, float Size, float Min, float MaxW);
/** Body copy wrapped to MaxW, at most MaxLines lines (the last ellipsized); returns the y after the last line. */
float RitParagraph(Canvas& C, const std::string& Text, float X, float Y, float MaxW, float Size, const Color& Col, float LineH, int MaxLines = 99);

// ------------------------------------------------------------------ the twice print (§3.3)

/** Display lines from this size up print twice (the knob of §13.7: raise to 40 if the PIE read is blurry). */
constexpr float RitTwiceFrom = 28.0f;
/** At most this many twice-printed runs a page. */
constexpr int RitTwiceCap = 24;
/** The under-copy's offset: round(Size / 16), 2-6. */
float RitTwiceOffset(float Size);
/**
 * The twice print: the under-copy (Signal Red; Deep Red on a red ground) at (+o, +o), then the readable copy in Fill on top,
 * in BoldCondensed. Under 28 units, or past the page's 24, it prints once. Returns the width.
 */
float RitTwice(Canvas& C, const std::string& S, float X, float Y, float Size, const Color& Fill, uint32_t Under = RitRed, Align A = Align::Left);

// ------------------------------------------------------------------ shapes (§3.3: square corners with one 45° cut)

/** A rect with its top-right corner cut at 45 degrees (Cut 0: a plain rect). */
std::vector<Vec2> RitCutPath(const Rect& R, float Cut);
void RitCutFill(Canvas& C, const Rect& R, float Cut, const Paint& P);
void RitCutStroke(Canvas& C, const Rect& R, float Cut, const Color& Col, float W = 1.0f);
/** The gloss: a lacquer gradient from Gloss to Onyx, top to bottom (plates, flap tiles, data cards' top bands). */
Paint RitGlossPaint(const Rect& R);
/** A panel: Carbon (or Fill) under a 1-unit hairline, cut 10. Starts a new panel for the twin-stripe tally. */
void RitPanel(Canvas& C, const Rect& R, float Cut = 10.0f, uint32_t Fill = RitCarbon, uint32_t Line = RitHair);
/** A 1-unit LED Red core along a red fill's top edge. */
void RitLedEdge(Canvas& C, float X, float Y, float W);
/** A soft glow under a rect (the plate's light, the selection's underglow, a slab's halo). */
void RitGlow(Canvas& C, const Rect& R, uint32_t Rgb, float Alpha, float Blur);
/** The light sweep: a white diagonal band at 0.10 across R, its middle At (0..1) of the way along. */
void RitSweep(Canvas& C, const Rect& R, float At, float Band);
/** A slab: a Signal Red cut fill with its LED top edge and a top-half gloss at 0.05 (Hover: the light sweep). */
void RitSlab(Canvas& C, const Rect& R, float Cut, bool Hover, double Now);

// ------------------------------------------------------------------ the twin stripe (§3.3)

/** Horizontal: an 8-unit bar, a 4-unit gap, a 3-unit bar (scaled by K). Brand (Signal Red) stripes count a panel's one. */
void RitTwinH(Canvas& C, float X, float Y, float W, float K = 1.0f, uint32_t Rgb = RitRed);
void RitTwinV(Canvas& C, float X, float Y, float H, float K = 1.0f, uint32_t Rgb = RitRed);
/** Slashed at 18 degrees like the section slab's edge: X is the thick bar's top-left. */
void RitTwinSlash(Canvas& C, float X, float Y, float H, float K = 1.0f, uint32_t Rgb = RitRed);
/** The tape's separator: two 18-degree bars, 3 and 1 wide. */
void RitTwinTick(Canvas& C, float X, float Y, float H, const Color& Col);

/** What a page drew of the signatures (ui_seg_rit's checks): twice prints, and the most Signal Red twin stripes in a panel. */
struct RitTally
{
	int Twice = 0;
	int Stripes = 0;
	int MaxStripes = 0;
	int Panels = 0;
};
void RitPageBegin();
void RitPanelBegin();
const RitTally& RitTallyNow();

// ------------------------------------------------------------------ marks (vectors: Roboto has no arrows, triangles or stars)

/** The race-number box: an Onyx box outlined in Signal Red with an Ink numeral; active, white with an Onyx numeral. */
void RitRaceBox(Canvas& C, float X, float Y, int N, bool Active, float S = 22.0f, float Size = 15.0f);
/** A small cut tag in BoldCondensed caps; returns its width. Outline: a 1-unit frame, the ink the frame's colour. */
float RitTag(Canvas& C, float X, float Y, const std::string& S, uint32_t Fill, uint32_t Ink, float Size = 11.0f, float PadX = 5.0f, float H = 16.0f, float Cut = 4.0f,
	bool Outline = false);
/** Hot White caps on a Signal Red parallelogram (the Second Board's label); returns its width. */
float RitParaTag(Canvas& C, float X, float Y, const std::string& S, float Size = 13.0f, float H = 22.0f);
/** The Watchlist star: Chrome outline, Hot White when on. Never red (ui_seg_rit). */
void RitStar(Canvas& C, float Cx, float Cy, float R, bool On);
void RitLock(Canvas& C, float X, float Y, const Color& Col);
void RitBell(Canvas& C, float Cx, float Cy, float S, const Color& Col);
/** A move triangle (Up: Signal Up's ▲; else ▼). */
void RitTri(Canvas& C, float Cx, float Cy, float S, bool Up, const Color& Col);
/** A right arrow (the copy's "→"). */
void RitArrow(Canvas& C, float X, float Y, float W, const Color& Col, float Sw = 2.0f);
/** "±" after a projected figure, as a vector; returns its width. */
float RitPlusMinus(Canvas& C, float X, float Baseline, float Size, const Color& Col);
/** The carbon twill: 14-unit two-tone diagonal stripes (cached per skin key; at most 90 quads). */
void RitTwill(Canvas& C, const Rect& R);
/** Scanlines every 3 units at Alpha (the tape's; cached). */
void RitScanlines(Canvas& C, const Rect& R, uint32_t Rgb, float Alpha);
/** An octagon's points in R, its corners cut by Cut. */
std::vector<Vec2> RitOctagon(const Rect& R, float Cut);
/**
 * The Settlement's stamp on paper: an octagonal frame of twin lines in Signal Red ink (the frame and the ink-spread ring may
 * fade with Fade; the word never does: 4.83:1 on Paper at full alpha), the word in BoldCondensed caps, scaled by Scale.
 */
void RitStampPaper(Canvas& C, float Cx, float Cy, const std::string& Word, float Size, float Scale, float Spread);
/** On a dark ground a stamp is a filled Signal Red octagon with the word in Hot White (5.45:1), never red letters. */
void RitStampDark(Canvas& C, float Cx, float Cy, const std::string& Word, float Size);
/** A square avatar (DrawAvatar under a square clip) in a 1-unit frame. */
void RitSquareAvatar(Canvas& C, float X, float Y, float S, const std::string& Name, const Color& Frame);

// ------------------------------------------------------------------ the split-flap

/** A flap tile's ground: gloss black, Signal Red (FINAL CALL, and its dim phase), or chrome (SEATED). */
enum class RitTile : uint8_t
{
	Gloss,
	Red,
	RedDim,
	Chrome,
};
/**
 * A split-flap cell: tiles behind each character of Text from X (BoldCondensed at Size, H tall), the first Settled of them
 * lettered (blank panels while flipping: text never scales), a black split across, in W (the cell's width; 0: the text's).
 * One run a cell. Returns the width of the tiles.
 */
float RitFlap(Canvas& C, float X, float Y, const std::string& Text, float Size, float H, int Settled, RitTile Tile, const Color& Ink, float W = 0.0f);

// ------------------------------------------------------------------ G2: the loops' model (RunItTwice's half of the View)

/**
 * What RunItTwice's G2 surfaces draw (§3.4, §3.6 R1-R9), member for member the RitView proposed to core-siteplay
 * (patches/ui-rit_to_core-siteplay_1.diff). RitPlayOf fills it from Session::SitePlayView(RunItTwice) and nothing else, so
 * every figure is core's (the book, World::League, the Governor's ring): the pages rank, zone and score nothing themselves.
 * While core's half is empty every surface shows its honest empty state; ui_seg_rit drives the pages with fixture data
 * through RitPlayOverride, a test-only seam the game never calls.
 */
struct RitLeagueRow
{
	int Npc = -1; // -1: the player
	std::string Name, Country;
	int Rank = 0, Move = 0, Days = 0;
	float Points = 0.0f;
	std::array<float, 8> Best{};
	std::array<int8_t, 5> Form{{-1, -1, -1, -1, -1}}; // oldest first: -1 no entry, 0 blank, 1 cash, 2 final table
	std::array<int8_t, 3> Path{{-1, -1, -1}};         // oldest first: a division, -1 not known
	int8_t Zone = 0;                                  // +1 promotion, -1 relegation
	bool Team = false, Callout = false, CanCall = false;
};
struct RitFixture
{
	int Day = 0;
	std::string TemplateId, Name;
	float Mult = 1.5f;
	bool Deadline = false;
};
struct RitDivision
{
	int Members = 0;
	Chips Floor = 0;
	std::vector<std::string> Matchdays;
	std::string Reward;
};
struct RitCallout
{
	bool On = false;
	int Npc = -1;
	std::string Name, Country;
	bool ByThem = false;
	float Mine = 0.0f, Theirs = 0.0f;
	int Week = -1;
	double Ends = 0.0;
	bool Tonight = false;
	std::string EventId;
	int W = 0, L = 0;
	bool Seen = false; // G2 fix L6: this save has already slid this Matchweek's INCOMING CALLOUT in (RitView::IncomingSeen)
};
struct RitH2H
{
	std::string Name;
	int W = 0, L = 0, TheyKo = 0, YouKo = 0;
};
struct RitScalpPro
{
	int Npc = -1;
	std::string Name;
	bool Taken = false;
};
struct RitScalps
{
	std::vector<RitScalpPro> Six;
	int Taken = 0, Week = 0, WeekCap = 2, Lifetime = 0, Year = 0;
	bool FullSet = false, Bounty = false;
	std::vector<std::pair<Chips, std::string>> Ladder; // (the lowest buy-in, what a scalp there pays), ascending
};
struct RitStatementLine
{
	std::string Label;
	int MonthKey = -1;
	Chips Cents = 0;
	int Kind = 0;      // site::play::Kind
	std::string Month; // core's name for the earning month ("October"; MonthKey's when empty)
};
struct RitHold
{
	std::string What;
	Chips Cents = 0, Covered = 0, RakeToGo = 0;
	int Day = 0;
	bool SparksOwed = false;
};
struct RitStatement
{
	std::string Line; // core's sentence ("RunItTwice gave back $53.48 of your $212.40 rake in the last 30 days (25%).")
	Chips Rake30 = 0, Promo30 = 0;
	double Share = 0.0, Ceiling = 0.5;
	std::vector<RitStatementLine> Lines; // newest first
	std::vector<RitHold> Holds;          // oldest first
	int Day0 = -1;
	std::array<int32_t, 30> RakeByDay{};
};
struct RitLuck
{
	int AllIns = 0, Won = 0;
	double Expected = 0.0;
	std::array<int, 10> N{}, Wins{};
	std::array<double, 10> Exp{};
	int SecondWon = 0, SecondOf = 0;
	bool Seconds = false;
};
struct RitCred
{
	int Id = 0;
	std::string Name;
	bool Earned = false;
	int Day = -1;
	std::string Event;
};
struct RitBriefing
{
	bool On = false;
	int Day = -1, Events = 0;
	Chips Net = 0;
	int RankWas = 0, RankNow = 0;
	float ToPromotion = 0.0f;
	std::string Matchday, Callout;
	std::string Line; // core's composed line (when set, drawn as it is)
};
struct RitBellView
{
	std::string EventId, Name;
	Chips Short = 0;
	double Closes = 0.0, Rung = 0.0;
};
struct RitNight
{
	bool On = false;
	int Month = -1;
	int8_t From = 0, To = 0;
	int Rank = 0, Of = 0;
	std::vector<RitLeagueRow> Was, Final;
	std::string Ticket;
	bool Held = false;
	bool Seen = false; // G2 fix L6: this save has already played this month's League Night (RitView::NightSeen)
};
struct RitPlayHist
{
	int Month = 0;
	int8_t Div = 0;
	int Rank = 0, Of = 0;
	int8_t Move = 0;
};
struct RitPlay
{
	bool On = false; // core's loops are live (RitView::On): the LEAGUE plate shows; off, G1's 8 plates
	bool Member = false, Started = false, InLeague = false, Clause = false;
	bool Leave = false, Placement = false, Bounce = false, Deadline = false;
	int MonthKey = -1;
	int8_t Div = 0, Group = 0, Crest = 0;
	int Rank = 0, Of = 0;
	float Points = 0.0f, ToPromotion = 0.0f, ToSafety = 0.0f;
	int Up = 3, Down = 4;
	int Matchweek = 0, Matchweeks = 4;
	double Closes = 0.0;
	int Today = -1;
	std::vector<std::string> Counting;
	std::vector<std::pair<int, float>> HeroDays;
	std::vector<RitLeagueRow> Table;
	std::vector<RitFixture> Fixtures;
	std::array<RitDivision, 5> Divisions{};
	std::vector<RitPlayHist> History; // newest first
	std::vector<std::pair<std::string, float>> Scored;
	RitCallout Callout;
	std::vector<RitH2H> H2H;
	RitScalps Scalps;
	RitStatement Statement;
	RitLuck Luck;
	std::vector<RitCred> Creds;
	RitBriefing Briefing;
	std::vector<std::string> Watch;
	std::vector<RitBellView> Bells;
	RitNight Night;
};
/** The League's divisions, low to high (§3.6 R2). */
constexpr int RitDivisions = 5;
const char* RitDivName(int Div);
/** A division's status metal (§3.3's ramp: Open a chrome outline, Iron carbon, Steel chrome, Gold Status Gold, High Bridge red). */
uint32_t RitDivMetal(int Div);
/**
 * A division's crest (§3.6 R2): a shield of 1-5 bars in its status metal (High Bridge's an arch), up to 5 stars over it.
 * Split 0..1 draws the shield split along its chevron (League Night's re-forge: the halves apart by Split).
 */
void RitCrest(Canvas& C, float Cx, float Cy, float H, int Div, int Stars = 0, float Split = 0.0f);
/** A credential: a chrome plate engraved in Onyx (6.25:1 or more on the ramp); while not earned, an outline in Muted. */
void RitCredPlate(Canvas& C, const Rect& R, const std::string& Name, const std::string& Line, bool Earned);
/** "You're the bounty" (R4): a Signal Red crosshair, a vector. */
void RitCrosshair(Canvas& C, float Cx, float Cy, float R);
/** A watched player's chrome eye (§3.5 Seats). */
void RitEye(Canvas& C, float Cx, float Cy, float W, const Color& Col);

// ------------------------------------------------------------------ the screen state (beside the class: RiverLine.h is frozen)

/** A status cell the Board remembers, so it flips only when its value changes. */
struct RitFlapMemo
{
	std::string Text;
	double At = -100.0;   // when it started to flip
	bool Sounded = true;  // its RitFlap has played
};
/** One of the tape's items. */
struct RitTapeItem
{
	std::string Key;   // what it is about ("ov:<id>", "run", "left:<table>")
	std::string Text;  // in capitals
	uint32_t Rgb = RitInk;
	int Move = 0;      // a vector triangle before the last word: 1 up, -1 down
};
/**
 * A Second Board seen at this laptop's tables, for the Settlement's left column (UI state, never saved). Every field is the
 * finished hand's (site::show::SecondRecordOf), never the table's display state: the table reads the board at the showdown,
 * before the award has marked a winner, and by then its seats' equities have run to the river's 100 / 0.
 */
struct RitSecond
{
	std::string EventId;
	std::string HandId;
	std::array<int, 2> Hero{};
	std::array<int, 2> Villain{};
	std::vector<int> Ran;
	std::array<int, 5> Twice{};
	int Verdict = 0;       // 0 lost both, 1 split, 2 won both
	int First = 0;         // what the real run paid the player: 0 lost, 1 part (a chop, or a main pot without the side), 2 won
	double Equity = -1.0;  // the player's when the cards went on their backs (autoplay::AllInEquity); -1 unknown
	double At = 0.0;       // world minutes
};
/** The record of site::show's, as the column and the ghost draw it. */
RitSecond RitSecondOf(const site::show::SecondRecord& R);
/** A seat as the octagon's layer last saw it (the knockout slide's memory; UI state, never saved). */
struct RitSeatSeen
{
	bool Present = false;
	bool Hero = false;
	std::string Id;
	std::string Name;
	Chips Stack = 0;
};
/** A busted seat's card on its way out (§3.5 Knockouts): which seat, who, since when (real seconds). */
struct RitKnockout
{
	int Seat = 0;
	std::string Name;
	double At = 0.0;
	bool Stamped = false; // G2: a Team pro's scalp, its TAKEN thunked (RitStamp, once)
};
struct RitUi
{
	// The Board.
	bool GtdOnly = false;
	bool LateOnly = false;
	int Sort = 0; // 0 start, 1 buy-in, 2 GTD, 3 field, 4 overlay
	std::map<std::string, RitFlapMemo> Flaps;
	double WaveAt = -100.0;
	double WaveKey = -1.0e9; // the page-open stamp (RiverLine's PageAt) the last wave started from
	RiverLine::Page SeenPage = RiverLine::Page::Embercrest;
	std::string SeenEvent;
	double SelectAt = -100.0;
	int Tab = 0; // the Prospectus's tab
	// The tape.
	long long TapeKey = -1;
	std::vector<RitTapeItem> Tape;
	std::map<std::string, std::string> TapeSeen;
	std::vector<std::string> FlashQueue;
	std::string FlashKey;
	double FlashAt = -100.0;
	double TapeX = 0.0;
	// The table (bets in BB or chips: ui-tablestyle's SitesBetsInBb and SitesToggleBets).
	std::string SecondHand;
	double SecondAt = -100.0;
	bool SecondShown = false;
	bool SecondFirst = true; // "On RunItTwice, we run it twice." the first time
	double SecondNoteAt = -100.0;
	std::vector<RitSecond> Seconds; // <= 12, newest last: the table's own reads (its 4 s ghost); the Settlement reads Session::SecondBoards
	std::string WinFor;              // the hand whose win last swept
	double WinAt = -100.0;
	std::vector<RitTapeItem> Extra;  // the tables' own lines for the tape (wins, the pot of the session), <= 3, newest first
	std::string SeatsFor;                // the table the seats were seen at ("<event>#<table>")
	double SeatsAt = -100.0;             // when (a gap of a second or more: the player looked away, nothing slides)
	std::vector<RitSeatSeen> SeatsSeen;  // as last drawn
	std::vector<RitKnockout> Knockouts;  // cards sliding out (0.6 s each)
	// Sunday Live.
	int Live = 0;
	std::string BellFor;
	double BellAt = -100.0;
	size_t FeedSeen = 0;
	std::string FeedFor;
	// Rankings, the Newsdesk.
	std::map<int, int> RankSeen;
	int NewsPick = 0;
	// The Desk.
	int DeskTab = 0;
	int DeskAll = 0;
	// The Club.
	bool CardBack = false;
	double CardAt = -100.0;
	int CardTier = -1;
	double MintAt = -100.0;
	std::string ClubDayFor;        // the Club Day slip last printed ("<minute>#<cents>")
	double ClubDayAt = -100.0;     // when it started to feed out of the slot
	bool ClubDayStamped = true;    // its PAID has thunked (RitStamp, once)
	// The Settlement.
	std::string PrintFor;
	double PrintAt = -100.0;
	bool Stamped = false;
	// G2 (§3.6). The loops' screen state: what this laptop has seen and shown, never saved (core keeps the outcomes).
	bool HasOverride = false;   // RitPlayOverride's fixture model (ui_seg_rit only)
	RitPlay Override;
	bool Mine = false;          // the Board's "mine" filter (the Watchlist)
	int BriefingGone = -1;      // the 6 AM day whose Briefing was dismissed
	int LeagueTab = -1;         // the League's browsed division (-1: the player's own)
	int NightSeen = -1;         // the closed month whose League Night has played on this laptop
	double NightAt = -100.0;    // League Night's start (real seconds); NightOn while it plays
	bool NightOn = false;
	int NightRankTicks = 0;     // RitRank ticks played in this League Night
	int CallNpc = -1;           // the Callout drawer's member (-1: closed)
	double CallAt = -100.0;
	std::string CallSaid;       // core's answer to the last CALL OUT ("" called)
	double CallSaidAt = -100.0;
	int IncomingSeen = -1;      // the Matchweek whose incoming Callout alert has slid in
	double IncomingAt = -100.0;
	std::vector<std::string> BellsRung; // the desk bells rung on this laptop (instance ids, <= 24)
	std::string DeskBellFor;    // the last desk bell's event
	double DeskBellAt = -100.0;
	bool Pyramid = false;       // RANKINGS' PYRAMID tab
	int StatementTab = 0;       // the Club's Statement panel: 0 the Statement, 1 Club Day's printout
	std::vector<int> CredsSeen; // credentials earned when this laptop last looked (a new one stamps once)
	bool CredsInit = false;
	double CredAt = -100.0;
	int CredNew = -1;
	bool SideLeague = false;    // the table's LEAGUE tab (ui-tablestyle's SidePanel; patches/ui-rit_to_ui-tablestyle_3.diff)
	int RankWasFor = -1;        // the League place this laptop last saw (the Settlement's "11TH -> 7TH"), by month
	int RankWas = 0;
	float PointsWas = 0.0f;
	int SettleRankFrom = 0;     // the place before the Settlement on show, and after
	int SettleRankTo = 0;
	float SettlePointsFrom = 0.0f;
	bool SettleRanked = false;
	int SettleTicks = 0;        // its RitRank ticks played
	std::vector<std::pair<std::string, std::string>> TakenHere; // (event id, a Team pro's name) seen leave as a scalp (<= 8)
	std::string ScalpSeatsFor;  // the table (event # table) ScalpBefore was read at
	std::vector<std::string> ScalpBefore; // the Team pros already TAKEN when this laptop sat at that table (or stamped there):
	                                      // a knockout stamps TAKEN only for a scalp core took after (review-ui-rit)
	RitPlay PlayCache;          // this frame's RitPlayOf (the frame, its table layer and the LEAGUE tab read it once)
	double PlayAt = -1.0;       // the session clock and world minute it was read at
	double PlayWorld = -1.0;
};
/** This laptop's state (by address, as rlsites_detail::SitesOf). */
RitUi& RitOf(const RiverLine* RL);
/** Drops a laptop's state when it goes (~RiverLine, by ui-tablestyle's patch). */
void ForgetRit(const RiverLine* RL);
/** The loops' model for this laptop: core's (Session::SitePlayView, read once a frame), or the test's override. */
RitPlay RitPlayOf(const RiverLine* RL, const Session& S);
/** Core's view at a world minute as the model (RitPlayOf's uncached read). */
RitPlay RitPlayFrom(const Session& S, double World);
/** ui_seg_rit only: draw this laptop's G2 surfaces from P (null: core's again). The game never calls it. */
void RitPlayOverride(const RiverLine* RL, const RitPlay* P);

// ------------------------------------------------------------------ the page kit both files share (RiverLineRit.cpp's)

/** Everything a page draws with: the laptop, its session, the canvas and pointer, this laptop's state, the clocks, the loops. */
struct RitCtx
{
	RiverLine& RL;
	Session& S;
	Canvas& C;
	Ui& UI;
	RitUi& U;
	double Now = 0.0;
	double World = 0.0;
	bool Preview = false;
	RitPlay P; // this frame's loops (RitPlayOf)
};
/** RunItTwice. */
inline constexpr net::SiteId RitSite = net::SiteId::RunItTwice;
void RitSound(RitCtx& X, SoundId Id, float Vol);
Ui::ClickState RitClick(RitCtx& X, const std::string& Id, const Rect& R, bool Enabled = true);
std::string RitClock(double World);
std::string RitCountdown(double Minutes);
std::string RitBuyIn(Chips Cents);
std::string RitDate(int Day);
std::string RitTemplateOf(const std::string& EventId);
/** "1ST", "22ND": a place in capitals. */
std::string RitPlace(int N);
/** League points as the pages print them: "38.2". */
std::string RitPts(double V);
/** A League row's name in the world's letters, the player's handle as typed. */
const RitLeagueRow* RitHeroRow(const RitPlay& P);
/** High Bridge: nobody goes up; its top places win a Sunday 100K ticket (RiverLineRitLeague.cpp). */
bool RitTopDivision(const RitPlay& P);
/** The table's projection (§3.6 R2, Deadline Day): the place the player would hold if this event scored Score today. */
int RitRankIf(const RitPlay& P, double Score);
/** A term sheet's line: a Chrome label, a dotted leader, the value right-aligned in Mono. */
void RitTerm(RitCtx& X, float L, float R, float Y, const std::string& Key, const std::string& Value, uint32_t Rgb);
/** "NOVEMBER" or "NOV"; "November". A MonthKey is Year * 12 + Month - 1 (world::MonthKeyOf). */
std::string RitMonthName(int Key, bool Short);
std::string RitMonthTitle(int Key);
/** A multiplier as the copy writes it ("1.5", "1.25"). */
std::string RitMult(float M);
/** "STEEL G4": the group's short form. */
std::string RitGroupTag(const RitPlay& P);
/** A division's status-metal pip (Open's an outline; -1 a Dim outline: not known). */
void RitPip(Canvas& C, float X, float Y, float S, int Div);

// ------------------------------------------------------------------ G2's pages and panels (RiverLineRitLeague.cpp)

/** THE LEAGUE (Page::League, §3.4 C). */
void RitLeaguePage(RitCtx& X);
/**
 * League Night (§3.6 R2): the season finale, after the frame's pages (whose pointer it held from the top: SitesBlockUnder).
 * A click anywhere skips it (Scrim: the click on the scrim, swallowed). Returns true while it plays.
 */
bool RitLeagueNight(RitCtx& X, bool Scrim);
/** The Callout drawer (R3), after the frame's pages (pointer held from the top); Scrim closes it. */
void RitCalloutDrawer(RitCtx& X, bool Scrim);
/** The drawer's rect this frame (its slide in from the right). */
Rect RitCallRect(const RitUi& U, double Now, bool Preview);
/** INCOMING CALLOUT (R3): the Signal Red alert that slides in at the top of the client; its rect (the frame holds under it). */
void RitIncoming(RitCtx& X, float X0);
Rect RitIncomingRect(float X0);
/** The Statement (R5): given back as a share of rake against the 50% ceiling, the dated lines, the Guest List. */
void RitStatementPanel(RitCtx& X, const Rect& R);
/** Team Scalps (R4) on the Club: the year's six, TAKEN, the ticket ladder, FULL SET; "you're the bounty". */
void RitScalpsPanel(RitCtx& X, const Rect& R);
/** The Luck Ledger (R7) on the Desk's right panel. */
void RitLuckPanel(RitCtx& X, const Rect& R);
/** Credentials (R7) as chrome plates, in R (the Member File's left column). */
void RitCredentials(RitCtx& X, const Rect& R);
/** The Member File's League history, Callout record and head-to-head (R2, R3). */
void RitMemberLeague(RitCtx& X, const Rect& R);
/** The Settlement's League panel (RiverLineRit.cpp): the place it moved from and to, the group re-sorting, RitRank's ticks. */
void RitSettleLeague(RitCtx& X, const Rect& R, double Since);
/** The Briefing (R9) in R; returns false when there is none to show (the caller draws what was there). */
bool RitBriefingBand(RitCtx& X, const Rect& R, bool Pinned);
/** The Watchlist's feed (the League's side): the starred events' next instances. */
void RitWatchFeed(RitCtx& X, const Rect& R);
/** The table's LEAGUE tab (§3.5 Side panel): the group live and the projected place by outcome. */
void RitSideLeague(RiverLine& RL, Canvas& C, const Rect& Body, double Now);

// ------------------------------------------------------------------ the forwarders (RiverLine.h's friend; RiverLineRitLeague.cpp)

/** One row of the shared schedule (RiverLine::ScheduleRows, the gatekeeper), copied out of the class's private type. */
struct RitRow
{
	net::EventInstance E;
	net::LiveState L;
	bool Joinable = false;
	std::string Lock;
};
struct Access
{
	static Session& S(RiverLine& RL);
	static Canvas& C(RiverLine& RL);
	static Ui& UI(RiverLine& RL);
	static double World(const RiverLine& RL);
	static double Dt(const RiverLine& RL);
	static bool Previewing(const RiverLine& RL);
	static double PageAt(const RiverLine& RL);
	static void SetPage(RiverLine& RL, RiverLine::Page P);
	static double EventAt(const RiverLine& RL);
	static std::vector<std::string>& Listed(RiverLine& RL);
	static RiverLine::Filter FilterShown(const RiverLine& RL);
	static double FilterAt(const RiverLine& RL);
	static bool TakeScrollToSelected(RiverLine& RL);
	static float& ListScroll(RiverLine& RL);
	static float& ListScrollGoal(RiverLine& RL);
	static net::Board BoardShown(const RiverLine& RL);
	static double BoardAt(const RiverLine& RL);
	static double NewsSeenAt(const RiverLine& RL);
	static std::vector<RitRow> ScheduleRows(const RiverLine& RL);
	static void RegisterBlock(RiverLine& RL, const net::EventInstance& E, const net::LiveState& L, const Rect& R, double Now);
	static const std::vector<net::NewsItem>& NewsFeed(RiverLine& RL);
	static void PlayerCard(RiverLine& RL, double Now);
	static void FinishedToasts(RiverLine& RL, double Now);
};

// ------------------------------------------------------------------ the copy (every sentence in RitInventory, for the lint)

inline constexpr const char* RitNoTables = "No tables running.";
inline constexpr const char* RitBoardOpen = "The Board is open.";
inline constexpr const char* RitNoSpins = "No spins. No boxes.";
inline constexpr const char* RitRakeBack = "Rake back, in cash, on the 1st.";
inline constexpr const char* RitAttitude = "No spins. No boxes. Rake back, in cash, on the 1st.";
inline constexpr const char* RitBoardFoot = "Statuses settle on open. FINAL CALL is 10 minutes or less of late registration. OVERLAY projects until registration closes.";
inline constexpr const char* RitBoardEmpty = "Nothing on the Board for this filter right now.";
inline constexpr const char* RitSitePays = "The site pays the overlay if the field stays short.";
inline constexpr const char* RitNoOverlay = "No overlay projected: the field covers the guarantee so far.";
inline constexpr const char* RitNoOverlayYet = "No overlay projected: the field is on course to cover the guarantee.";
inline constexpr const char* RitNoGtd = "No guarantee: the pool is what the field puts in.";
inline constexpr const char* RitSettles = "Settles to your account.";
inline constexpr const char* RitNoRecord = "Nothing played here yet.";
inline constexpr const char* RitPickRow = "Pick a row on the Board to read its terms.";
inline constexpr const char* RitPaysGrow = "Paid places grow with the field: about one in seven cashes.";
inline constexpr const char* RitNoMajor = "No major on the Board today. The night's featured event is up instead.";
inline constexpr const char* RitNoChipCounts = "Chip counts stay at the tables. The Board reports what is final.";
inline constexpr const char* RitLiveOpen = "SUNDAY SESSION OPEN.";
inline constexpr const char* RitNoFeed = "Nothing in the feed yet. Lines land here as the night happens.";
inline constexpr const char* RitNoNotables = "No Team pros or followed players registered yet.";
inline constexpr const char* RitDeskEmpty = "No RunItTwice results yet. They land on the Desk as you play.";
inline constexpr const char* RitDeskTape = "Your RunItTwice hands kept for Rerun: open Studio to cut one.";
inline constexpr const char* RitDeskNoTape = "No RunItTwice hands kept yet. A big hand here lands on this tape.";
inline constexpr const char* RitSeriesNone = "No series on the calendar right now.";
inline constexpr const char* RitNewsEmpty = "No stories on the Newsdesk yet.";
inline constexpr const char* RitClubTurn = "Click the card to turn it.";
inline constexpr const char* RitProvisional = "Provisional until the 1st. It is paid into your balance here.";
inline constexpr const char* RitTopTier = "Inner Circle: the top of the Club.";
inline constexpr const char* RitNoLetters = "Nothing from them yet. Offers come by Mail.";
inline constexpr const char* RitStaysHere = "Money on a site plays there and nowhere else. It never pays the rent.";
inline constexpr const char* RitDepositHow = "From your bank, by card. It lands at once.";
inline constexpr const char* RitWithdrawHow = "Back to your bank. It takes a while, and they take their fee.";
inline constexpr const char* RitOnlyBank = "Money only moves between a site and your bank, never site to site.";
inline constexpr const char* RitNothingComing = "Nothing on its way.";
inline constexpr const char* RitNoLines = "Nothing here yet. Deposits, buy-ins and prizes show up as they happen.";
inline constexpr const char* RitSettled = "Settled. Paid to your RunItTwice account.";
inline constexpr const char* RitRunTwice = "On RunItTwice, we run it twice.";
inline constexpr const char* RitJustToSee = "Just to see.";
inline constexpr const char* RitForShow = "Dealt for show from the rest of the deck. It never moves a chip.";
inline constexpr const char* RitNoSeconds = "No all-in ran with cards to come in this event.";
inline constexpr const char* RitNoUpNext = "Nothing boarding right now. The Board has the night's schedule.";
inline constexpr const char* RitMemberNote = "Your file on RunItTwice: what you played here, and nothing else.";
inline constexpr const char* RitNoClub = "Not in the Club yet this month: $50 of rake makes you a Member.";
inline constexpr const char* RitTicketsRule = "A first deposit of $50 or more brings a $22 Sunday Grind ticket and two $5.50 daily tickets.";
inline constexpr const char* RitTicketsNone = "None held right now.";
inline constexpr const char* RitClubDayNone = "Club Day prints here on the 1st: the month's rake, your tier and what it paid.";
inline constexpr const char* RitClubPaid = "Paid into your balance here on the 1st.";
inline constexpr const char* RitNoClubMonths = "No Club months yet. Your first $50 of rake in a month makes one.";
inline constexpr const char* RitSecondHow = "The rest of the deck, dealt again. It never moves a chip.";
// G1's fix pass (fix-rit): ARRIVED's hover when the world has no result yet.
inline constexpr const char* RitNoWinner = "The result posts in a moment.";
// G2 (§3.4, §3.6): the League, Callouts, the Watchlist, Team Scalps, the Statement, the Luck Ledger, credentials, the Briefing.
inline constexpr const char* RitLeagueNone = "The League draws its groups from members with a paid entry here. Your first paid RunItTwice entry puts you in this month's draw.";
inline constexpr const char* RitLeagueNoGroup = "No group drawn for you this month yet. It's drawn at your first paid RunItTwice entry of the month, or on the 1st.";
inline constexpr const char* RitLeagueRule = "Your best 8 days count. Each day's best is your best result from your first entries of the day at your division's stakes.";
inline constexpr const char* RitLeagueOther = "Every division plays in groups of 20 each month. Only your own group's standings are kept.";
inline constexpr const char* RitDivNoCount = "Members are counted when the month's groups are drawn.";
inline constexpr const char* RitLeagueLeave = "On leave: no RunItTwice entry this month under your Team RiverLine deal. No relegation, no Callouts, and your division is kept.";
inline constexpr const char* RitLeagueBounce = "Bounce Back: cash 3 times in Matchweek 1 and start 30 points up.";
inline constexpr const char* RitNoFixtures = "No fixtures for your division this week.";
inline constexpr const char* RitNightSkip = "Click to skip.";
inline constexpr const char* RitCalloutNone = "No Callout this week. Call out a member of your group: the week's League points decide it.";
inline constexpr const char* RitCalloutNoGroup = "Callouts are played inside your League group, once a Matchweek.";
inline constexpr const char* RitCalloutLeave = "No Callouts while you're on leave.";
inline constexpr const char* RitCalloutClause = "No Callouts under your Team RiverLine deal.";
inline constexpr const char* RitCalloutRule = "Status only: no cash, no League points. A win goes on your Member File.";
inline constexpr const char* RitCallDrawerWhat = "A week-long duel inside your group: this Matchweek's League points, by the League's own scoring.";
inline constexpr const char* RitCallOnce = "One Callout a Matchweek, and they can call you out too.";
inline constexpr const char* RitWatchNone = "Star an event on the Board to put it on your Watchlist. Its next five starts show in the header.";
inline constexpr const char* RitWatchQuiet = "Nothing on your Watchlist is open in the next two days.";
inline constexpr const char* RitMineNone = "Nothing on your Watchlist is on the Board. Star a row to add its event.";
inline constexpr const char* RitClauseBanner = "Under your Team RiverLine deal, every table here is a strike.";
inline constexpr const char* RitBounty = "Signed with Team RunItTwice, you can't take a scalp: the crosshair is on you now, at every table here.";
inline constexpr const char* RitScalpsRule = "Knock one of Team RunItTwice's six out of a paid RunItTwice event and their scalp is yours, with a ticket for two a week at most.";
inline constexpr const char* RitScalpsNone = "No scalps yet this year. The set is the six on the team when you take your first.";
inline constexpr const char* RitScalpsLadder = "A ticket scaled to the event.";
inline constexpr const char* RitStatementNone = "Nothing given back in the last 30 days. Second Run pays on the 1st; League and Team Scalps tickets land as you earn them.";
inline constexpr const char* RitLuckNone = "No all-ins in paid RunItTwice events yet. Each one lands here against its equity.";
inline constexpr const char* RitLuckEven = "You are running at expectation.";
inline constexpr const char* RitLuckHow = "Expected wins are your equity when the chips went in, summed.";
inline constexpr const char* RitCredsNone = "Credentials are engraved here as you earn them.";
inline constexpr const char* RitHistNone = "No League months closed yet.";
inline constexpr const char* RitH2HNone = "No head-to-head history yet.";
inline constexpr const char* RitSideNoLeague = "Not in a League group this month. Your first paid RunItTwice entry of the month draws one.";
inline constexpr const char* RitSideNotCounting = "This event doesn't score today: it's past your first entries of the day, or under your division's floor.";
inline constexpr const char* RitSideFlip = "The tab flips to PROMOTION ZONE the moment you cash.";
inline constexpr const char* RitTodaysBest = "Today's best counts.";
inline constexpr const char* RitNotToday = "Not today's best.";
inline constexpr const char* RitNoLeagueYet = "Your first paid RunItTwice entry of the month draws your League group.";
inline constexpr const char* RitNoLuckHere = "No all-ins with cards to come.";
inline constexpr const char* RitRakeNone = "No rake paid here in the last 30 days.";

/** Every sentence RunItTwice's pages draw beyond core's own copy (the copy lint, the trademark screen). */
std::vector<std::string> RitInventory();
/** Its names and labels: the plates, the section slab's words, the statuses, the stamps (the trademark screen, not the lint). */
std::vector<std::string> RitLabels();
} // namespace rlrit_detail
} // namespace ui
} // namespace ss
