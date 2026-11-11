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
 * Update 4 (SPEC v1.7 §2.4.3, §6 PA-11): a prize table's shape. Standard is PayoutTable exactly; Main is the festival Mains'
 * written curve (G1; nothing sets it before). Never saved (templates are code; online specs are not checkpointed). Declared with
 * A-1's first commit (PA-3 appends it to net::EventTemplate); PayoutFor, PaidPlaces and the payout:: constants with the engine's.
 */
enum class PayoutCurve : uint8_t
{
	Standard = 0,
	Main = 1,
};

/**
 * The one payout function for every consumer (§2.4.3; the engine, the network, the world and the table talk call it from A-2,
 * PA-13). Standard is PayoutTable(PoolCents, Entrants, MinCashCents) exactly (PaidShare 0.15). Main is the written curve, its body
 * F-net's at G1; until then it routes to PayoutTable too (nothing sets Main before G1).
 */
SHORTSTACKCORE_API std::vector<Chips> PayoutFor(Chips PoolCents, int Entrants, Chips MinCashCents, PayoutCurve Curve);

/** PayoutFor(...).size() without the arithmetic: Standard's (and, until G1, Main's) is PayoutTable's paid places. */
SHORTSTACKCORE_API int PaidPlaces(int Entrants, PayoutCurve Curve);

/** The Main curve's constants (§2.4.3; read by E/Structure.cpp's Main body from G1). */
namespace payout
{
constexpr double kMainPaidShare = 0.12;
constexpr double kMainLift = 0.30;                         // the final table's lift up to kMainLiftFull entries
constexpr int kMainLiftFull = 3000, kMainLiftZero = 50000; // the lift tapers in ln(entries) to 0 at kMainLiftZero
constexpr double kMainFloorFlat = 0.20;                    // the bottom 20% of paid places pay the min cash
constexpr double kMainFloorBlock = 0.10;                   // each next 10% block of the bottom half ...
constexpr double kMainFloorStep = 1.15;                    // ... floors at 1.15x the block below
} // namespace payout

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
