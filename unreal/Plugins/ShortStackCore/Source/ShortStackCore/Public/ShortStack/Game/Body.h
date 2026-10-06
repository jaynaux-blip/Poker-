#pragma once

#include "ShortStack/Common.h"
#include "ShortStack/Game/Hero.h"

namespace ss
{
/**
 * Fitness and grooming (the realism update's spec, §5.1 and §5.3): what IronHouse's workouts build and what fades
 * without them, what that does online (the drain, sleep, the exhausted threshold, tilt) and at the table (the heart,
 * Focus), and how long the hair and beard have grown since the barber last saw them. Saved (§7.3).
 *
 * Phase A: the contract only. The bodies return defaults (no effect: Effects{} and TableEffects{}, everything allowed
 * at the barber) until core-shops' Phase B fills them.
 */
namespace body
{
struct Fitness
{
	double Level = 0.0;
	double LastCounted = -1.0e9;
	int Workouts = 0;
	int Sessions = 0;
	double ClearHeadUntil = 0.0;
	double LastDecay = 0.0;
};

struct Grooming
{
	double CutAt = -1.0e9;
	double ShaveAt = -1.0e9;
	int Cuts = 0;
};

/** Online and at home: the awake drain's scale, a bonus on sleep's restore, where "exhausted" starts, tilt calm. */
struct Effects
{
	double DrainScale = 1.0;
	double RestBonus = 0.0;
	double ExhaustedAt = 20.0;
	double Calm = 0.0;
};

/** At a live table (BackRoomTable, BackRoomGameMode). */
struct TableEffects
{
	double RestBpm = 66.0;
	double SettleScale = 1.0;
	double SteadyBpm = 16.0;
	double FocusRefill = 1.0 / 14.0;
	double FocusCostScale = 1.0;
	double TiltGainScale = 1.0;
};

SHORTSTACKCORE_API Effects EffectsOf(const Fitness& F, bool Member);
SHORTSTACKCORE_API TableEffects TableOf(const Fitness& F, double World); // Clear head, Box Breathing
SHORTSTACKCORE_API double Gain(const Fitness& F, double Base, double Energy, double World); // 0 within 16 h of the last counted
SHORTSTACKCORE_API void Decay(Fitness& F, double World);
SHORTSTACKCORE_API int HairRank(int HairStyle); // §5.1's ranks
SHORTSTACKCORE_API int BeardRank(int FacialHair);
SHORTSTACKCORE_API int HairAllowed(const Grooming& G, const hero::Look& L, double World);
SHORTSTACKCORE_API int BeardAllowed(const Grooming& G, const hero::Look& L, double World);
SHORTSTACKCORE_API double Sharp(const Grooming& G, double World);
} // namespace body
} // namespace ss
