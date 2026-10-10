#pragma once

#include "ShortStack/Common.h"
#include "ShortStack/Hand.h"

#include <string>
#include <vector>

namespace ss
{
/**
 * Bet sizing for big-blind-ante structures (next_segment_spec §6.2 D): the presets for each spot (unopened, facing a
 * raise, postflop unbet, facing a bet), the default, the fine steps and the labels ("RAISE TO 3,600 · 2.2x · 2.3 BB",
 * "BET 8,000 · 67% POT"). A pot-sized raise is the current bet + pot + to-call, as RiverLine::PotRaise. The card room
 * and RiverLine's online table share it.
 *
 * Bodies: core-tableui (Sizing.cpp). Phase A, day 1: stubs that compile and change nothing.
 */
namespace sizing
{
struct Spot
{
	Street On = Street::Preflop;
	Chips Pot = 0;
	Chips ToCall = 0;
	Chips CurrentBet = 0;
	Chips Bb = 0;
	Chips MinTo = 0;
	Chips MaxTo = 0;
	Chips Effective = 0;
	bool IsBet = false;
	bool InPosition = false;
	int Raises = 0;
};
struct Preset
{
	std::string Label;
	Chips To = 0;
	bool AllIn = false;
};
SHORTSTACKCORE_API Spot SpotOf(const Hand& H, int Seat);
SHORTSTACKCORE_API std::vector<Preset> Presets(const Spot& S);
SHORTSTACKCORE_API Chips Default(const Spot& S);
SHORTSTACKCORE_API Chips Step(const Spot& S, Chips To, int Steps);
SHORTSTACKCORE_API int Snap(const Spot& S, Chips To, int Dir);
SHORTSTACKCORE_API std::string Describe(const Spot& S, Chips To);
} // namespace sizing
} // namespace ss
