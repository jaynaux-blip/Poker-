#pragma once

#include "ShortStack/Common.h"

#include <cstdint>

namespace ss
{
/**
 * The weather over Northside (the realism update's spec, §3.3): clear, overcast, drizzle, rain and storms that come
 * and go by the world clock, the same for every look at it (the street's sky and puddles, the apartment's window, the
 * people's umbrellas and talk).
 *
 * Owned by people-core. Phase A (core-shops): the contract only; At returns a dry, clear default until people-core
 * fills it.
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
};

/** Deterministic from the world minutes and the career's seed; RealSeconds animates the lightning envelope. One endless
 *  chain read by a bounded look-back (48-96 steps, cached per slot) and wetness over a fixed 24 h window (§3.3). */
SHORTSTACKCORE_API Now At(double World, uint64_t Seed, double RealSeconds = 0.0);
SHORTSTACKCORE_API const char* Word(Sky S); // "Rain"
} // namespace weather
} // namespace ss
