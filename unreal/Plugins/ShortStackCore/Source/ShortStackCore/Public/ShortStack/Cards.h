#pragma once

#include "ShortStack/Common.h"

namespace ss
{
inline int RankOf(Card C) { return C >> 2; }
inline int SuitOf(Card C) { return C & 3; }
inline Card MakeCard(int Rank, int Suit) { return (Rank << 2) | Suit; }

extern const char* const RankChars; // "23456789TJQKA"
extern const char* const SuitChars; // "cdhs"
extern const char* const RankNames[13];
extern const char* const RankPlurals[13];

std::string CardToString(Card C);
/** Parse "As" style text; returns -1 on bad input. */
Card ParseCard(const std::string& Text);
/** Parse "AsKd" or "As Kd". */
std::vector<Card> ParseCards(const std::string& Text);
std::vector<Card> FullDeck();

/**
 * Starting-hand class 0..168 on a 13x13 grid: pairs on the diagonal, suited
 * hands above it (row = high rank), offsuit below.
 */
int HandClass(Card A, Card B);
std::string HandClassLabel(int Cls);
int HandClassCombos(int Cls);
} // namespace ss
