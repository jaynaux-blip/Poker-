#pragma once

#include "ShortStack/Common.h"
#include "ShortStack/Game/Handles.h"
#include "ShortStack/Structure.h"

#include <cstdint>
#include <deque>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace life
{
struct State;
}
namespace world
{
class World;
}
struct LobbyEvent;
struct TournamentSpec;
class Tournament;
class Session;

/**
 * Update 4, the Big Field (SPEC v1.7 §4.2-§4.15; Phase A's declarations, §6 PA-42): fields over 3,000 played three ways (every
 * hand, key spots, jump to the final table), the deep-run beats, CHIP COUNTS, the final-table broadcast kit, the keyed mystery deck
 * and YOUR RUN. Bodies in G/BigField.cpp and G/SessionBig.cpp (B-session's at G3), the draws in UI/RiverLineBig.cpp (B-ui's). Frozen
 * after Phase A (append-only, by patch to the lead).
 *
 * Phase A: every body is a stub that keeps today's behaviour. Nothing calls them: Estimate is zero, LockLine "", MysteryDeck and
 * Inventory empty, ReadLine claims run\t lines and keeps nothing (as any unknown line was dropped), WriteLines writes nothing.
 */
namespace bigfield
{
// The knobs and cross-gate constants (§1.4; each defined here and nowhere else). The dealt-field limit, the ring and the model's
// own constants are fieldmodel::'s (EH/FieldModel.h).
/** [ASK] 3, answered "after the first rent" (2026-11-10): rents paid before net::Unlock::UnlockBigField opens (§4.2). */
constexpr int kBigFieldRents = 1;
/** The broadcast kit and the new beats play in fields of this many entries or more (§4.5.2, §4.8.1). */
constexpr int kBroadcastEntries = 1000;
/** "Discovered at your table" needs a field of this many (§4.11.1). */
constexpr int kDiscoverEntries = 5000;
/** The play-on's and the catch-up's budgets a frame, real milliseconds (§4.10.1, §4.12.1): 2 ms, 8 ms in a time skip's ease. */
constexpr double kModelSliceMs = 2.0, kModelSkipSliceMs = 8.0, kCatchUpSliceMs = 8.0;
/** At most this many play-ons at once (§4.10.1). */
constexpr int kPlayOnMax = 4;
/** The mystery deck sums to this share of the bounty pool (§4.9.4; the lead's decision of 2026-11-09). */
constexpr double kMysteryDeckShare = 1.0;
/** YOUR RUN (§4.15): kept for results of this many entries or more; the best 12 by place over entries plus the last 3. */
constexpr int kRunMinEntries = 1000, kRunsBest = 12, kRunsLast = 3;
/** KEY SPOTS pauses this long on a beat, the banner's life (§4.3.3). */
constexpr double kBeatHoldSeconds = 3.2;
/** The session watches at most this many seats of a modelled field for WHO'S IN (§4.7.3, Tournament::SetWatch). */
constexpr int kWatchMax = 12;
/** Step A-2 (LEAD_CALLS Q-5; §4.1.2's game-side list): CHIP COUNTS' top rows and a PKO's biggest heads (§4.7.2), and YOUR RUN's
 *  samples kept per run (§4.15.1: 32 buffered, decimated to 16). */
constexpr int kLeaders = 10, kBiggestHeads = 5, kRunSamples = 16;

/** The register strip's three tiles (§4.3.1; autoplay::Prefs::BigPick holds the last one, 0 for never). None: today's table. */
enum class Pick : uint8_t
{
	None,
	EveryHand, // Mode::Off at Smart: today's table
	KeySpots,  // the skip to the result, stopping for the big decisions, the bubble, the money and the final table
	Jump,      // the skip to the final table, the all-in stop on unless dropped for this registration
};

/** The strip's estimate (§4.4.2): real minutes, rounded to 5, at least 5. */
struct Pace
{
	int EveryHand = 0, KeySpots = 0, Jump = 0;
};
/** From the pacing table's rows, interpolated in ln(entries) and scaled by the level length (§4.4.2). Phase A: zeros. */
SHORTSTACKCORE_API Pace Estimate(int Entries, double LevelMinutes);

/** The lock line on a big field before the bit is on ("Big fields open once your first rent is paid.", §4.2.2). Phase A: "". */
SHORTSTACKCORE_API std::string LockLine();

// The broadcast kit's view (§4.8.2; Session::BroadcastView builds it in G/SessionBig.cpp). Presentation only: never saved.
struct IntroSeat
{
	int Seat = 0;
	std::string Name, Country, Honorific;
	Chips Stack = 0;
	bool Hero = false;
	int Npc = -1;
};
struct LowerThird
{
	int Place = 0;
	std::string Name, Country, By;
	Chips Prize = 0, Next = 0;
	double At = 0;
};
struct TaleOfTape
{
	std::string Them;
	Chips You = 0, ThemChips = 0;
	int YourTitles = 0, TheirTitles = 0;
	std::string Record;
};
struct Broadcast
{
	bool On = false;
	uint8_t Phase = 0; // 0 the seat diagram, 1 the seats, 2 the pay ladder, 3 over (play resumes)
	int SeatAt = 0;
	double PhaseAt = 0; // real seconds (Session::Now)
	std::vector<IntroSeat> Seats; // clockwise from the button's left
	std::vector<Chips> Ladder;     // the final table's places, 1st first (8 at 8-max, 9 at 9-max), real Payouts
	std::string Brand;             // the festival's brand (SeriesInfo::Brand) or ""
	std::deque<LowerThird> Queue;  // at most 2 waiting
	bool HeadsUp = false;
	TaleOfTape Tape;
};

// CHIP COUNTS (§4.7.2; Session::CountsView, refreshed once a tick, cached by the tick). Never saved.
struct CountsRow
{
	int Rank = 0;
	std::string Name, Country;
	Chips Stack = 0;
	bool Hero = false;
	int Npc = -1;
};
struct CountsView
{
	int Tick = -1;                 // the engine tick it was built at (-1: never)
	int HeroRank = 0, Remaining = 0;
	std::string Ladder;            // the pay ladder's line (§4.6)
	std::vector<CountsRow> Top;    // the top 10, then the hero's row when outside them
	std::vector<std::pair<std::string, Chips>> Heads;  // PKO only: the 5 biggest heads
	std::vector<std::pair<Chips, int>> Envelopes;      // mystery only: the top two values left with their counts
	int EnvelopesMore = 0;                              // and how many more are left
};

/** The keyed mystery deck (§4.9.2): Paid - 1 envelopes summing to round(Share x Entries x Bounty), keyed by place. Phase A: empty. */
SHORTSTACKCORE_API std::vector<Chips> MysteryDeck(uint64_t WorldSeed, const std::string& EventId, int Entries, Chips Bounty, int Paid, double Share);

/** G4's crowd namer for a modelled seat's first naming (§4.11.2): true when it named the seat (Out, CrowdSerial). Unset before G4. */
using SeatNamer = std::function<bool(int Field, uint8_t Arch, int Tier, handles::Identity& Out, int& CrowdSerial)>;

/** Every string the Big Field prints (the lock line, the reveal, the strip, the beats, CHIP COUNTS, the kit, YOUR RUN), for the
 *  copy lint and both trademark tiers (BigCopy). Phase A: empty. */
SHORTSTACKCORE_API std::vector<std::string> Inventory();

/** YOUR RUN's save line (§4.15.2): run\t<eventId>\t<entries>\t... Read by the session's Parse dispatch; true when Fields is one. */
SHORTSTACKCORE_API bool ReadLine(const std::vector<std::string>& Fields, life::State& L);
/** The kept runs as run\t lines, only once Update4LinesRead & U4Run is set (§4.15.2). Phase A: nothing. */
SHORTSTACKCORE_API void WriteLines(const life::State& L, std::string& Out);

// Step A-2 (LEAD_CALLS Q-5): every function the Big Field's sections name in this header, declared before the freeze with a stub
// that answers empty, none or false (bodies in G/BigField.cpp, B-session's at G3; nothing calls them before G3).

/** The extended ladder (§4.1.10): Base (the listing's levels: the template's, doubled for stacks over 10,000) continued past its
 *  last level in the ladder's decade pattern (1M, 1.2M, 1.6M, 2M, 2.5M, 3M, 4M, 5M, 6M, 8M, 10M ...; small blind half, ante = big
 *  blind) up to fieldmodel::kLadderCap. Network::Listing sets it as Spec.Levels for a listing over kBigFieldEntries (per listing,
 *  never a template field). Phase A: empty. */
SHORTSTACKCORE_API std::vector<Level> Ladder(const std::vector<Level>& Base);

/** One tile of the register strip (§4.3.1): its pick, the title ("KEY SPOTS"), the estimate line ("about 55 min") and the sub line. */
struct StripTile
{
	Pick Which = Pick::None;
	std::string Title, Estimate, Sub;
};
/** The register strip (§4.3.1): RiverLine's RegisterBlock, Ignite's event sheet and RunItTwice's Prospectus each draw it in their
 *  own skin from this one view, one layout rule. Presentation only: never saved. */
struct Strip
{
	bool Shown = false;          // a listing over kBigFieldEntries while net::Unlock::UnlockBigField is on
	Pick Selected = Pick::None;  // the preselected tile: Prefs::BigPick's last pick, KEY SPOTS when it's 0
	StripTile Tiles[3];          // EVERY HAND, KEY SPOTS, JUMP TO THE FINAL TABLE
	bool AllInStop = true;       // tile 3's toggle ("ALL-IN STOP ON" / "OFF"): on for every registration until dropped for it
	std::string Clock;           // under the strip: "About 11 hours on the clock." (the instance's Duration, to the hour, >= 1)
};
/** The strip for a listing (§4.3.1): Unlocks the career's net::Unlock bits, BigPick autoplay::Prefs::BigPick, AllInStop tile 3's
 *  toggle for this registration. Phase A: not shown (an empty view). */
SHORTSTACKCORE_API Strip StripView(const LobbyEvent& Listing, int Unlocks, uint8_t BigPick, bool AllInStop);

/** The deep-run beats (§4.5.2), by priority (a lower value wins; one banner a tick). FinalTable, InTheMoney and Bubble (THE BUBBLE /
 *  HAND FOR HAND) are today's engine events; the rest are new, in fields of kBroadcastEntries or more, from day 2, with the bit on,
 *  while the hero is in. */
enum class Beat : uint8_t
{
	None,
	FinalTable, // 1, today's
	HeadsUp,    // 2 HEADS-UP FOR THE TITLE: 2 left, the hero one of them
	InTheMoney, // 3, today's (a modelled field's sub line is new)
	Bubble,     // 4, today's THE BUBBLE / HAND FOR HAND
	LastThree,  // 5 LAST THREE TABLES: TableCountAll() falls from 4 or more to 3 or fewer
	Left100,    // 6 100 LEFT: Remaining crosses 100; entries over 200
	Left500,    // 7 500 LEFT: crosses 500; entries over 1,000
	Left1000,   // 8 1,000 LEFT: crosses 1,000; entries over 2,000
	HalfGone,   // 9 HALF THE FIELD IS GONE: crosses entries / 2; entries 2,000 or more
	Count,
};
/** What the beats compare, taken before and after each engine tick (§4.5.2). Fired holds the beats this event has already shown
 *  (bit 1 << Beat), so each fires at most once per event. */
struct BeatState
{
	int Entries = 0, Remaining = 0, Tables = 0; // Tables: Tournament::TableCountAll()
	bool HeroIn = false;
	int HeroRank = 0;
	uint32_t Fired = 0;
};
/** The highest-priority new beat crossed between two ticks (§4.5.2; G/SessionBig.cpp calls it after each tick at G3). Phase A: None. */
SHORTSTACKCORE_API Beat BeatsCrossed(const BeatState& Before, const BeatState& After);

/** The skip card's line in a modelled field (§4.3.4): "SKIPPING · KEY SPOTS · 4,112 left · you're 640th · 34 to the money." before
 *  the money, "... · next jump at 1,080th." in it (autoplay::SkipLine elsewhere). Phase A: "". */
SHORTSTACKCORE_API std::string SkipLine(const Tournament& T, Pick P);

/** A watched seat's status while the hero's run lives (§4.7.3, WHO'S IN's lines): in, with the rank from FieldView().WatchRank, or
 *  out, with the place. Known false: the engine doesn't know the seat (then the world's result speaks, after the event). */
struct SeatStatus
{
	bool Known = false;
	bool In = false;
	int Rank = 0;  // in: the seat's rank in chips
	int Place = 0; // out: where it finished
};
/** Field is the seat's field index (TLeader::Field, fieldmodel's). Phase A: unknown. */
SHORTSTACKCORE_API SeatStatus StatusOf(const Tournament& T, int Field);

/** Late registration into a modelled field closes before the estimated money (§4.12.1): the start plus the calibration's median
 *  money tick (by CalibId, scaled by LevelMinutes) less one level, in clock minutes. Phase A: 0 (no estimate). */
SHORTSTACKCORE_API double MoneyMinutes(const TournamentSpec& Spec);

/** The broadcast's honorific for a seat (§4.8.4): public facts only, the first match wins (titles, team or badge, a rank, an arc
 *  fact, history with you, this event's facts, then "Plays from {country}."); Npc -1 for an anonymous seat. Phase A: "". */
SHORTSTACKCORE_API std::string Honorific(const world::World& W, const Session& S, int Npc, const std::string& EventId);
} // namespace bigfield
} // namespace ss
