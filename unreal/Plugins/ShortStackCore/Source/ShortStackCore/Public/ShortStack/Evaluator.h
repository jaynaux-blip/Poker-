#pragma once

#include "ShortStack/Common.h"

namespace ss
{
/**
 * Hand categories. Scores from Evaluate() are
 *   category << 20 | r1 << 16 | r2 << 12 | r3 << 8 | r4 << 4 | r5
 * so a higher score is always a stronger hand.
 */
enum class Category : int
{
	HighCard = 0,
	Pair = 1,
	TwoPair = 2,
	Trips = 3,
	Straight = 4,
	Flush = 5,
	FullHouse = 6,
	Quads = 7,
	StraightFlush = 8,
};

extern const char* const CategoryNames[9];

/** Score the best five-card hand among N (5..7) cards. */
int Evaluate(const Card* Cards, int N);
/** Score hole cards plus a board of 3..5 cards. */
SHORTSTACKCORE_API int EvaluateHand(const std::vector<Card>& Hole, const std::vector<Card>& Board);
inline int CategoryOf(int Score) { return Score >> 20; }
/** Plain-English description, e.g. "Two Pair, Kings and Nines". */
SHORTSTACKCORE_API std::string Describe(int Score);
/** Highest card of a straight within a 13-bit rank mask, or -1. */
int StraightHigh(int RankMask);
} // namespace ss
