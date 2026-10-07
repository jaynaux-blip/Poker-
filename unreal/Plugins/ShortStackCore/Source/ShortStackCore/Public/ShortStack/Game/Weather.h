#pragma once

#include "ShortStack/Common.h"

#include <cstdint>

namespace ss
{
/**
 * The weather over Northside (the realism update's spec, §3.3): clear, overcast, drizzle, rain and storms that come
 * and go by the world clock, the same for every look at it (the street's sky and puddles, the apartment's window, the
 * people's umbrellas and talk). Pure core, deterministic from the career's seed (Session::WorldSeed) and the world
 * minutes; owned by people-core.
 *
 * The model:
 * - **One endless chain.** Each 30-minute slot is one step of a Markov chain over the five states in order. A step moves
 *   at most one state (Clear <-> Overcast <-> Drizzle <-> Rain <-> Storm) and stays put with p >= 0.8, reading one
 *   uniform per slot (u_k from the seed and k): down if u_k < p_down, up if u_k > 1 - p_up. The step is monotone, so a
 *   higher state never ends below a lower one, and it never restarts: nothing jumps at midnight.
 * - **Regimes.** Each day is dry, mixed or wet, and the regime sets that day's step odds: dry days break up to clear
 *   with cloud now and then, mixed days cloud over with spells of drizzle (and ease a wet day's rain off), wet days
 *   build to rain and storms. Every state reverts toward its regime's sky, so the look-back's two runs meet fast. The
 *   days of each ten are ranked by value noise over the day index and the seed (3 dry, 4 mixed, 3 wet), so a run of
 *   weather lasts a few days and any month gets its share. Over October they give slot shares of about Clear 25%,
 *   Overcast 35%, Drizzle 15%, Rain 20%, Storm 5% (town_test holds 90 days to within 5 points for three seeds).
 * - **A bounded look-back.** At finds a slot's state by running the chain from both ends of the order (Clear and
 *   Storm) from 48 slots back with the same u_k: if the two runs have met, that's the state any history gives. If not,
 *   it reaches back 96, 192 and 384 slots (the runs' maps compose, so each extension costs only the new slots); past
 *   that, the Clear run's value. 48 slots settle 99.5% of cold reads and 96 all but about 3 in a million. Slot n + 1
 *   is always slot n's state plus one step.
 * - **Storms** show for at most 2 h: after 4 Storm slots in a row the state reads Rain until the chain leaves Storm.
 * - **Night One** (the career's first minute, 2:07 AM on Tuesday, for 12 world hours) is the opening's rainy night:
 *   for those slots the step maps every state to Rain, so the chain carries on from Rain afterwards. The rain began at
 *   10 PM, before the career did, so the opening's street is already soaked.
 * - **Outputs**, eased over the first 10 minutes of each slot: Rain 0, 0, 0.3, 0.75, 1.0; Cloud 0.2, 0.85, 0.9, 0.95,
 *   1.0; Wind 0.1-0.6 and Fog 0.1-0.6 by state with a slow drift. Wetness integrates the rain over a fixed 24 h window
 *   ending now, from 0 at the window's start: up 1.2 x Rain an hour to 1; while Rain < 0.1, down 0.08 + 0.12 x
 *   Daylight an hour (dry in 5-12 h). Puddle is smoothstep(0.55, 1.0, Wetness), held 2 h on the way down. In a storm a
 *   flash comes every 20-90 real seconds (seeded) with its thunder 1-6 s behind.
 * - **The cache.** Each thread keeps the slots it has settled (per seed): the street, the window and the crowd ask
 *   many times a second, and the next slot costs one step. A read costs about 8 us (the 24 h wetness window in
 *   5-minute steps); the same read again (a frame's callers, the same clock) is the last answer.
 */
namespace weather
{
enum class Sky : int
{
	Clear,
	Overcast,
	Drizzle,
	Rain,
	Storm,
};

struct Now
{
	Sky State = Sky::Clear;
	double Rain = 0.0, Cloud = 0.2, Wetness = 0.0, Puddle = 0.0, Wind = 0.1, WindDir = 0.0, Fog = 0.1, Flash = 0.0, ThunderIn = -1.0;
	// Rain, Cloud: 0..1 as above. Wetness, Puddle: 0..1. Wind: 0.1..0.6. WindDir: radians in the stage frame, the way the
	// wind blows (0 toward +X, west; pi/2 toward +Y, north): mostly from the west, so near pi. Fog: 0.1..0.6. Flash: the
	// lightning's envelope 0..1 (MPC_NS.Lightning). ThunderIn: seconds until the last flash's thunder, -1 when none is due.
};

constexpr double SlotMinutes = 30.0;

/** Deterministic from the world minutes and the career's seed; RealSeconds animates the lightning envelope. One endless
 *  chain read by a bounded look-back (48-96 steps, cached per slot) and wetness over a fixed 24 h window (§3.3). */
SHORTSTACKCORE_API Now At(double World, uint64_t Seed, double RealSeconds = 0.0);
SHORTSTACKCORE_API const char* Word(Sky S); // "Rain"
/** The state slot Slot (World / SlotMinutes, floored) shows, storms capped at 2 h: At(World).State without the rest. */
SHORTSTACKCORE_API Sky SlotSky(int64_t Slot, uint64_t Seed);

/**
 * What the last At or SlotSky on this thread cost (town_test counts it). Miss: the 24 h window's first slot wasn't in the
 * cache, so the look-back ran; LookBack its coupled steps (48, or 96 when the two runs hadn't met; 192 about 3 reads
 * in a million); Roll the single steps from there to the slot asked (one per slot; 1 for the next slot of a warm cache).
 */
struct Cost
{
	bool Miss = false;
	int LookBack = 0;
	int Roll = 0;
};
SHORTSTACKCORE_API Cost LastCost();
/** Forgets this thread's cache (a test's cold read). */
SHORTSTACKCORE_API void Forget();
} // namespace weather
} // namespace ss
