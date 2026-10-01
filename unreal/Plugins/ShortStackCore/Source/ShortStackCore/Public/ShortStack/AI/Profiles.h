#pragma once

#include "ShortStack/Common.h"

namespace ss
{
class Rng;

enum class Archetype : int
{
	Fish,
	Station,
	Nit,
	Tag,
	Lag,
	Maniac,
	Reg,
	Crusher,
};

/** How decision time relates to hand strength: the online timing tell. */
enum class TimingStyle : int
{
	Honest,
	Reverse,
	Balanced,
};

struct Profile
{
	Archetype Type = Archetype::Tag;
	TimingStyle Timing = TimingStyle::Balanced;
	std::string Label;
	double OpenWidth = 1.0;  // multiplier on baseline opening ranges
	double LimpRate = 0.0;   // chance to limp instead of raising
	double ThreeBet = 0.0;   // share of hands 3-bet for value
	double CallWidth = 1.0;  // multiplier on flatting ranges
	double Aggression = 0.0; // chance to bet/raise a value hand
	double Bluff = 0.0;      // chance to bluff when plausible
	double Stickiness = 0.0; // how far below pot odds they keep calling
	double Cbet = 0.0;       // continuation-bet frequency
	double Sizing = 1.0;     // bet size multiplier
	double TiltProne = 0.0;
	double PushFold = 0.0;   // how closely short-stack play follows charts
	double IcmAware = 0.0;   // how much bubble pressure changes play
	double ThinkBase = 0.0;  // ms
	double ThinkVar = 0.0;   // ms
};

const char* ArchetypeName(Archetype A);
/** Parses "fish", "tag", ... ; returns false if unknown. */
bool ArchetypeFromName(const std::string& Name, Archetype& Out);
const char* TimingName(TimingStyle T);

/** Rolls an individual player of the given archetype (consumes the RNG exactly like the TypeScript build). */
SHORTSTACKCORE_API Profile MakeProfile(Archetype A, Rng& R);

/** Weighted opponent mix for a stake band ("freeroll", "micro", "low", "high"). Order matters for RNG parity. */
using Population = std::vector<std::pair<Archetype, int>>;
const Population& GetPopulation(const std::string& Name);
Archetype RollArchetype(const Population& Pop, Rng& R);

/** Solid TAG used when the game plays the hero's hands (Sprint). */
Profile SprintProfile();
} // namespace ss
