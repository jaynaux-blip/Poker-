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
 * Effects are linear in f = Level / 100. A fitness of 0 changes nothing (Effects{} and TableEffects{}), outside the
 * heavy bag's clear head and Box Breathing, which are their own.
 */
namespace body
{
struct Fitness
{
	double Level = 0.0;          // 0..100
	double LastCounted = -1.0e9; // the last workout that counted (world minutes): one every 16 hours. A migrated member's
	                             // (no Workouts yet) is the migration's: the fading counts from it
	int Workouts = 0;            // counted workouts, ever
	int Sessions = 0;            // sessions with Rosa: three teach Box Breathing
	double ClearHeadUntil = 0.0; // the heavy bag's 8 hours
	double LastDecay = 0.0;      // fading is applied up to here
};

struct Grooming
{
	double CutAt = -1.0e9;   // the last cut: any CUT service, or a week before night one for a career that's had none
	double ShaveAt = -1.0e9; // the last beard service (the same week-old start); -1e9: nothing on record
	int Cuts = 0;            // cuts in Lonnie's chair: Sharp needs one
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

/** Fitness's own effects. The membership's sauna (tilt calm 0.10) is the GearDrop item's, counted by gear::Sum; Member
 *  is kept for the contract and changes nothing here. */
SHORTSTACKCORE_API Effects EffectsOf(const Fitness& F, bool Member);
SHORTSTACKCORE_API TableEffects TableOf(const Fitness& F, double World); // Clear head, Box Breathing
SHORTSTACKCORE_API double Gain(const Fitness& F, double Base, double Energy, double World); // 0 within 16 h of the last counted
SHORTSTACKCORE_API void Decay(Fitness& F, double World);
SHORTSTACKCORE_API int HairRank(int HairStyle); // §5.1's ranks
SHORTSTACKCORE_API int BeardRank(int FacialHair);
SHORTSTACKCORE_API int HairAllowed(const Grooming& G, const hero::Look& L, double World);
SHORTSTACKCORE_API int BeardAllowed(const Grooming& G, const hero::Look& L, double World);
/** 1 on the day of a cut in Lonnie's chair, 0 three weeks on (by the day); 0 before the first (Cuts 0). */
SHORTSTACKCORE_API double Sharp(const Grooming& G, double World);
} // namespace body
} // namespace ss
