#pragma once

#include "ShortStack/AI/Bot.h"
#include "ShortStack/AI/Profiles.h"
#include "ShortStack/AI/Reads.h"
#include "ShortStack/AI/View.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace ss
{
class Rng;
class Tournament;

/**
 * Autoplay's styles (update 2 §2.3, §2.4): the plan that plays the hero's seat when the player hands it over. Every
 * style decides through Decide, from public information only (the PlayerView, the observed stats in reads::, the learned
 * read book), and anything it calls from the engine draws from the tournament's R exactly as a bot would, in the same
 * order on the watch and the skip paths (§2.8). A style's own mixes come from Mix, a stateless hash. Autopilot is today's
 * Sprint profile exactly (session_detail::Autopilot forwards to ProfileOf(Style::Autopilot)): tilt 0, no slips.
 * Declared by the lead in Phase A; bodies by core-styles (AI/Style.cpp, StyleGto.cpp, StyleExploit.cpp, StylePushFold.cpp).
 */
namespace style
{
/** Saved (autoplay\tpref, autoplay\tplayed, the auto host line): append-only, in this order. */
enum class Style : int
{
	Autopilot,
	Tag,
	Lag,
	Nit,
	Maniac,
	Reg,
	Gto,
	Exploit,
	PushFold,
	IcmShark,
	BountyHunter,
	Count,
};
enum class Swing : int
{
	Steady,
	Even,
	Swingy,
	Wild,
};
/** One tile of the panel and the PLAY page. */
struct Info
{
	Style S;
	const char* Id;     // "gto": saved, and the console's name for it
	const char* Name;   // "GTO"
	const char* Edge;   // the tile's edge line
	Swing Var;
	const char* BestIn; // "tough fields"
	bool TourneyOnly;   // no meaning at a cash table (Push/Fold, ICM Shark, Bounty Hunter): AtCash plays instead
	bool BountyOnly;    // knockout events only
	Style AtCash;       // what plays at Dee's
	double Tempo;       // seconds: ThinkSeconds' base (§2.4.7)
	bool Hidden;        // the ship rule (§2.5): a style that missed calibration twice
};
SHORTSTACKCORE_API const std::vector<Info>& Catalog();
SHORTSTACKCORE_API const Info& InfoOf(Style S);
SHORTSTACKCORE_API bool FromId(const std::string& Id, Style& Out);
/** The profile a style starts from (§2.4.1), built from the archetype bases with no RNG. */
SHORTSTACKCORE_API Profile ProfileOf(Style S);
/** The learned read book (Life.Reads >= 2) as a snapshot at seating: name -> learned tells. */
struct Learned
{
	std::map<std::string, int> Tells;
};
/** Everything the plan decides with for a run: the style, how cleanly it plays (Sharp), its tilt, what it has read. */
struct HeroBrain
{
	Style S = Style::Autopilot;
	double Sharp = 0.80;
	double Tilt = 0;
	double TiltGain = 1.0;
	const reads::Table* Seen = nullptr;  // tonight's observed stats
	const reads::Table* Notes = nullptr; // the notes across events
	const Learned* Book = nullptr;
	int Where = 0; // 0 online, 1 Embercrest, 2 cash
};
/** One decision's facts: the view, the bubble pressure the bots get (PressureFor), the tournament, the mix keys. */
struct HeroContext
{
	PlayerView View;
	double Pressure = 0;
	const Tournament* T = nullptr;
	int HeroIdx = 0;
	uint64_t MixSeed = 0;
	int Tick = 0;
};
/** The plan's decision at a tournament (the tournament's R; the slip is drawn here, with Mix). */
SHORTSTACKCORE_API BotDecision Decide(const HeroContext& C, const HeroBrain& B, Rng& R);
/** The plan's decision at Dee's (the table's Rng; MixSeed and HandNo key its own mixes). */
SHORTSTACKCORE_API BotDecision DecideCash(const PlayerView& V, const HeroBrain& B, Rng& R, uint64_t MixSeed, int HandNo);
/** Stateless splitmix64 over the keys: [0, 1). Draws nothing from any Rng. */
SHORTSTACKCORE_API double Mix(uint64_t Seed, int Tick, int Seat, int Action, uint32_t Salt);
/** How long the plan seems to think (presentation only: it never feeds a decision). Speed: the watch speed (1 to 8). */
SHORTSTACKCORE_API double ThinkSeconds(const HeroBrain& B, const HeroContext& C, int Speed);
/** Every player-facing string the styles have (names, edge lines, best-in lines), for the copy lint. */
SHORTSTACKCORE_API std::vector<std::string> Inventory();

// ---------------------------------------------------------------------------------------------------------- appended by
// core-styles in G1 (append-only from here)

/** The swing chip's word: "STEADY", "EVEN", "SWINGY", "WILD" (never colour alone). */
SHORTSTACKCORE_API const char* SwingName(Swing V);
/**
 * Why a tile is greyed where the plan would play (§2.3, §2.6.3), or "" when it can be picked there. Where: 0 online,
 * 1 the Embercrest, 2 a cash game (Dee's). Knockout: the event pays bounties. A hidden style (the ship rule) is never
 * shown at all, so it has no reason.
 */
SHORTSTACKCORE_API std::string Unavailable(Style S, int Where, bool Knockout);
/**
 * GTO's balance over its own range at a river decision (the harness's assertion 14, §2.10): of the range, the share that
 * bets each size and the share of those bets that are bluffs (q outside the value region), or, facing a bet of Faced x
 * pot, the share that continues. Valid is false when the spot isn't a heads-up river decision GTO plays from its range.
 */
struct RangeAudit
{
	bool Valid = false;
	double Bet75 = 0, Bluff75 = 0, Bet100 = 0, Bluff100 = 0;
	double Faced = 0, Continue = 0;
};
SHORTSTACKCORE_API RangeAudit AuditGtoRiver(const HeroContext& C);
} // namespace style
} // namespace ss
