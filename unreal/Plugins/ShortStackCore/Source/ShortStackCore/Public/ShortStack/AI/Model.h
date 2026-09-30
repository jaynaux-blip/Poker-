#pragma once

#include "ShortStack/AI/View.h"
#include "ShortStack/Equity.h"

namespace ss
{
class Rng;

/**
 * What one player believes about an opponent's holding, from public actions
 * only: a preflop range band and how much postflop aggression or calling the
 * opponent has shown.
 */
struct OpponentModel
{
	int Seat = 0;
	RangeBand Band;
	int Aggr = 0;  // postflop bets and raises
	int Calls = 0; // postflop calls
};

std::vector<OpponentModel> ModelOpponents(const PlayerView& View);
OpponentModel ModelSeat(const PlayerView& View, int Seat);

/** 0 = nothing, 1 = one pair using a hole card, 2 = two pair or better using a hole card. */
int MadeLevel(Card A, Card B, const std::vector<Card>& Board, int BoardCat);
/** Flush draw or open-ended straight draw (flop and turn only). */
bool HasDraw(Card A, Card B, const std::vector<Card>& Board);
int BoardCategory(const std::vector<Card>& Board);

/** Monte Carlo equity against modelled opponents (ranges narrowed by their actions). */
double EquityVsModels(const std::vector<Card>& Hole, const std::vector<Card>& Board, const std::vector<OpponentModel>& Models, int Iterations, Rng& R);

/** Cheap made-hand/draw estimate used by the fast bots that play the rest of the field. */
double HeuristicEquity(const std::vector<Card>& Hole, const std::vector<Card>& Board, int Opponents);
} // namespace ss
