#pragma once

#include "ShortStack/Common.h"

namespace ss
{
class Tournament;

/**
 * ICM Shark's payout ladder (update 2 §2.4.5): where chips stop being worth their face, and the bubble factor between two
 * players, from the exact Icm() with 7 or fewer left, else a six-player model with the ladder block-compressed.
 * Bodies: core-styles (AI/IcmPlay.cpp).
 */
namespace icmplay
{
/** Within 30% of the money, at the final table, or in a satellite's last three times the seats. */
SHORTSTACKCORE_API bool InZone(const Tournament& T);
/** ICM equity lost if Player loses the pot to Against over equity gained if they win it. */
SHORTSTACKCORE_API double BubbleFactor(const Tournament& T, int Player, int Against);
/** The equity a call needs: Bf × Risk / (Bf × Risk + Pot). */
SHORTSTACKCORE_API double RequiredEquity(double Bf, Chips Risk, Chips Pot);
} // namespace icmplay
} // namespace ss
