#pragma once

#include "ShortStack/Common.h"

#include <map>
#include <string>

namespace ss
{
class Hand;

/**
 * What the hero has seen of the other players (update 2 §2.4.3): counts of their actions in hands the hero was dealt,
 * kept outside the engine (Tournament.cpp counts nothing), keyed by the player's id (SeatInput::Id). Fed after every
 * hand by the post-hand hooks on both paths (Session::AutoAfterHand online, FAutoHost::OnHandEnded and live::SkipSlice
 * at the Embercrest, a per-night table at Dee's, the harness's own table), so watching and skipping see the same stats;
 * a hand's counts are read only from the next hand on. Exploitative reads them through Of, never a player's hidden
 * Profile or the live tell stream (U2). Bodies: core-styles (AI/Reads.cpp).
 */
namespace reads
{
struct Stats
{
	int Hands = 0, Vpip = 0, Pfr = 0, ThreeBetOpp = 0, ThreeBet = 0, FoldTo3BetOpp = 0, FoldTo3Bet = 0, CbetOpp = 0, Cbet = 0, FoldToCbetOpp = 0,
		FoldToCbet = 0, Flops = 0, Wtsd = 0, AggBets = 0, AggCalls = 0;
};
/** By player id (SeatInput::Id). */
using Table = std::map<std::string, Stats>;
/** One hand the hero was dealt (HeroSeat: the hero's index in H.Seats), counted into Out. Draws nothing. */
SHORTSTACKCORE_API void Count(const Hand& H, int HeroSeat, Table& Out);
/** An event's table folded into the notes across events at Weight (half weight: §2.4.3). */
SHORTSTACKCORE_API void Fold(Table& Notes, const Table& Event, double Weight);
/** The estimates (each shrunk toward its population prior) and C, the confidence in them, 0..1. */
struct Read
{
	double Vpip, Pfr, ThreeBet, FoldTo3Bet, Cbet, FoldToCbet, Wtsd, Af;
	double C = 0;
};
/** From what was seen tonight and the notes (either may be null); a learned tell on the player halves K. */
SHORTSTACKCORE_API Read Of(const Stats* Seen, const Stats* Note, int LearnedTells);
/** The 15 counts as tab-separated text (the autoseen host line and autoplay\tnote), and back. */
SHORTSTACKCORE_API std::string Write(const Stats& S);
SHORTSTACKCORE_API bool Parse(const std::string& Text, Stats& Out);
} // namespace reads
} // namespace ss
