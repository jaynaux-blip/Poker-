#include "ShortStack/Game/Body.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Network.h"

#include <algorithm>
#include <cmath>

// Fitness and grooming (the realism update's spec, §5.1 and §5.3). Everything is linear in f = Fitness / 100:
//  - online: the awake drain x(1 - 0.15 f), sleep's restore + 0.10 f, "exhausted" at 15 energy from Fitness 50 (20
//    below), tilt calm + 0.20 f. The membership's own sauna (calm 0.10) is the GearDrop item's (Gear.cpp), so it's
//    counted once, by gear::Sum, member or not here.
//  - at the table: resting heart 66 - 6 f bpm, settling x(1 + 0.5 f), Focus refilling from 1/14 to 1/11 a second; the
//    heavy bag's clear head (tilt gain x0.85 for 8 hours); Box Breathing after three sessions with Rosa (steadying 20 bpm
//    instead of 16, for three-quarters of the Focus).
//  - a counted workout: Base x (1 - F/100), halved under 25 energy; one every 16 hours (the first any time). Three days
//    after the last one, fitness fades a point a day, continuously.
//  - the barber: a rank of length a cut can reach every 21 days since the last cut (to 6), a rank of beard every 5 days
//    since the last shave (to 4); always shorter. A career starts with its last cut and shave a week before night one
//    (Session::NewCareer, and shop::AfterParse for a save from before the shops), so the hair grows whether or not the
//    hero ever sits in Lonnie's chair. With nothing on record at all (a bare Grooming) nothing has grown yet: he cuts
//    what's there or shorter. Sharp is 1 on the day of a cut in Lonnie's chair and 0 three weeks on, by the day; the
//    week-old cut a career starts with isn't his work (Cuts 0), so it isn't sharp.

namespace ss
{
namespace body
{
namespace body_detail
{
constexpr double Day = 1440.0;
constexpr double DecayGrace = 3.0 * Day;
constexpr double WorkoutGap = 16.0 * 60.0;
constexpr double Never = -1.0e8; // Grooming's defaults (-1e9) sit below this: no cut or shave on record

double Unit(const Fitness& F)
{
	return std::clamp(F.Level / 100.0, 0.0, 1.0);
}
} // namespace body_detail

using namespace body_detail;

Effects EffectsOf(const Fitness& F, bool /*Member*/)
{
	const double U = Unit(F);
	Effects E;
	E.DrainScale = 1.0 - 0.15 * U;
	E.RestBonus = 0.10 * U;
	E.ExhaustedAt = F.Level >= 50.0 ? 15.0 : 20.0;
	E.Calm = 0.20 * U;
	return E;
}

TableEffects TableOf(const Fitness& F, double World)
{
	const double U = Unit(F);
	TableEffects T;
	T.RestBpm = 66.0 - 6.0 * U;
	T.SettleScale = 1.0 + 0.5 * U;
	T.FocusRefill = 1.0 / 14.0 + (1.0 / 11.0 - 1.0 / 14.0) * U;
	if (F.Sessions >= 3)
	{
		// Box Breathing: four in, hold four, four out.
		T.SteadyBpm = 20.0;
		T.FocusCostScale = 0.75;
	}
	if (World < F.ClearHeadUntil)
	{
		T.TiltGainScale = 0.85;
	}
	return T;
}

double Gain(const Fitness& F, double Base, double Energy, double World)
{
	// The gap runs from the last workout that counted. Before the first, there's none to wait on: a member migrated
	// from the old passive membership has LastCounted set to the migration (when the fading clock starts) but no
	// workout behind it.
	if (F.Workouts > 0 && World - F.LastCounted < WorkoutGap)
	{
		return 0.0;
	}
	const double G = (1.0 - std::clamp(F.Level, 0.0, 100.0) / 100.0) * (Energy < 25.0 ? 0.5 : 1.0);
	return std::max(0.0, Base * G);
}

void Decay(Fitness& F, double World)
{
	const double Start = std::max(F.LastCounted + DecayGrace, F.LastDecay);
	if (World > Start)
	{
		F.Level = std::max(0.0, F.Level - (World - Start) / Day);
		F.LastDecay = World;
	}
}

int HairRank(int HairStyle)
{
	// hero::Look's Hair: Shaved, Buzz Cut, Crew Cut, Textured Crop, Side Part, Undercut, Curls, Afro, Shoulder Length,
	// Ponytail, Braids (it takes at least an Afro's length to braid), Bob.
	static const int Ranks[] = {0, 1, 2, 3, 3, 3, 4, 5, 6, 6, 5, 5};
	return HairStyle >= 0 && HairStyle < 12 ? Ranks[HairStyle] : 3;
}

int BeardRank(int FacialHair)
{
	// None, Stubble, Mustache, Goatee, Short Beard, Full Beard.
	static const int Ranks[] = {0, 1, 2, 2, 3, 4};
	return FacialHair >= 0 && FacialHair < 6 ? Ranks[FacialHair] : 0;
}

int HairAllowed(const Grooming& G, const hero::Look& L, double World)
{
	// Nothing on record (a bare Grooming: every career, new or migrated, starts with a cut a week back): nothing grown.
	const double Grown = G.CutAt < Never ? 0.0 : std::floor(std::max(0.0, World - G.CutAt) / (21.0 * Day));
	const int Now = HairRank(L.Hair);
	return static_cast<int>(std::min(6.0, static_cast<double>(Now) + Grown));
}

int BeardAllowed(const Grooming& G, const hero::Look& L, double World)
{
	const double Grown = G.ShaveAt < Never ? 0.0 : std::floor(std::max(0.0, World - G.ShaveAt) / (5.0 * Day));
	const int Now = BeardRank(L.FacialHair);
	return static_cast<int>(std::min(4.0, static_cast<double>(Now) + Grown));
}

double Sharp(const Grooming& G, double World)
{
	if (G.Cuts <= 0 || G.CutAt < Never)
	{
		return 0.0; // never in Lonnie's chair (the cut a career starts with is from before the city)
	}
	const double Days = static_cast<double>(net::DayOf(World) - net::DayOf(G.CutAt));
	return std::clamp(1.0 - Days / 21.0, 0.0, 1.0);
}
} // namespace body
} // namespace ss
