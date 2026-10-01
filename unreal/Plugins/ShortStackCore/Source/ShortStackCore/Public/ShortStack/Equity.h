#pragma once

#include "ShortStack/Common.h"

namespace ss
{
class Rng;

/** Hand classes ordered strongest to weakest (generated from the TypeScript build). */
extern const int PreflopOrder[169];
/** Heads-up equity of each hand class against a random hand. */
extern const double PreflopEquityHU[169];

/** Share of all 1326 combos at least as strong as each hand class (AA = 0.0045, 32o = 1.0). */
const double* ClassPercentile();
SHORTSTACKCORE_API double HandPercentile(Card A, Card B);

/** A range as a band of the preflop ranking: hands with percentile in (Min, Max]. */
struct RangeBand
{
	double Min = 0.0;
	double Max = 1.0;
};

/** Monte Carlo equity of Hero against opponents drawn from Ranges; ties split. */
SHORTSTACKCORE_API double EquityVsRanges(const std::vector<Card>& Hero, const std::vector<Card>& Board, const std::vector<RangeBand>& Ranges, int Iterations, Rng& R);

/**
 * Pot share of every known hand: exact enumeration from the flop on,
 * Monte Carlo (PreflopSamples runs) before it.
 */
std::vector<double> EquityKnownHands(const std::vector<std::vector<Card>>& Hands, const std::vector<Card>& Board, Rng& R, int PreflopSamples = 12000);
} // namespace ss
