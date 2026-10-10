#pragma once

#include "ShortStack/Common.h"

namespace ss
{
struct Level
{
	Chips Sb = 0;
	Chips Bb = 0;
	Chips Ante = 0; // big-blind ante
};

// Exported (SHORTSTACKCORE_API) since the next segment: the game module's clock card, payouts and room clocks read the
// structure the core plays (next_segment_spec §7.1).

/** Modern online structure with a big-blind ante from level 1; 10,000 starting stack = 100 BB. */
SHORTSTACKCORE_API const std::vector<Level>& StandardLevels();

/**
 * Payout table in cents: about 15% of the field paid, power-law prizes, a
 * min-cash floor and banded places after 9th. Sums exactly to the pool.
 */
SHORTSTACKCORE_API std::vector<Chips> PayoutTable(Chips PoolCents, int Entrants, Chips MinCashCents, double PaidShare = 0.15);

/** Inclusive place ranges that share a payout, e.g. {10, 12}. */
SHORTSTACKCORE_API std::vector<std::pair<int, int>> PayoutBands(int Paid);

/** Independent Chip Model (Malmuth-Harville) equity per stack. */
SHORTSTACKCORE_API std::vector<double> Icm(const std::vector<double>& Stacks, const std::vector<double>& Prizes);
} // namespace ss
