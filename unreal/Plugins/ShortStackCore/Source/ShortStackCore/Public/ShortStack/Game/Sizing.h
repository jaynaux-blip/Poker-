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
 * | Spot              | Presets                          | Default                                   |
 * | Unopened preflop  | MIN 2.2x 2.5x 3x POT ALL IN      | 2.2x, or 2x under 25 BB effective         |
 * |   (no ante: cash) | MIN 2.5x 3x 4x POT ALL IN        | 3x                                        |
 * | Facing a raise    | MIN 3x 4x POT ALL IN             | 3x in position, 4x out                    |
 * | Postflop, unbet   | 1/3 1/2 2/3 3/4 POT ALL IN       | 2/3 pot                                   |
 * | Facing a bet      | MIN 2.5x 3x POT ALL IN           | 3x the bet                                |
 *
 * Sizes round to Unit (the smallest chip that makes every blind and ante: 100 at the Embercrest's level 1, 500 from
 * level 12, $1 at Dee's); a size at or past the stack is the ALL IN preset. Pure: nothing here changes the hand.
 */
namespace sizing
{
struct Spot
{
	Street On = Street::Preflop;
	Chips Pot = 0;        // everything in the middle, the street's bets included
	Chips ToCall = 0;
	Chips CurrentBet = 0; // the street's bet to match
	Chips Bb = 0;
	Chips MinTo = 0;      // the smallest legal raise (or bet), as the total street bet
	Chips MaxTo = 0;      // all in
	Chips Effective = 0;  // the smaller of the player's chips and the biggest other stack still in the hand
	bool IsBet = false;   // nobody has bet this street
	bool InPosition = false; // the player acts last after the flop among those still in
	int Raises = 0;       // bets and raises this street (preflop: the blinds aren't raises)
	Chips Ante = 0;       // the hand's big-blind ante (0: a cash game)
	Chips Unit = 1;       // sizes round to this
	bool CanRaise() const { return MaxTo > CurrentBet && MaxTo > 0; }
};
struct Preset
{
	std::string Label; // "MIN", "2.2x", "1/3", "POT", "ALL IN"
	Chips To = 0;
	bool AllIn = false;
};
/** The spot for the player in physical seat Seat (HandSeat::Seat). */
SHORTSTACKCORE_API Spot SpotOf(const Hand& H, int Seat);
/** The presets, smallest first, ALL IN last (empty when the player can't raise). */
SHORTSTACKCORE_API std::vector<Preset> Presets(const Spot& S);
SHORTSTACKCORE_API Chips Default(const Spot& S);
/** Fine steps as the card room always had them: a big blind under 20 BB, five above. */
SHORTSTACKCORE_API Chips Step(const Spot& S, Chips To, int Steps);
/** The preset a bumper goes to from To (Dir +1 the next bigger, -1 the next smaller, 0 the nearest); -1: none. */
SHORTSTACKCORE_API int Snap(const Spot& S, Chips To, int Dir);
/** "RAISE TO 3,600 · 2.2x · 2.3 BB", "BET 8,000 · 67% POT", "ALL IN 49,600 · 31 BB". */
SHORTSTACKCORE_API std::string Describe(const Spot& S, Chips To);
/** Describe without the verb and the amount: "2.2x · 2.3 BB", "67% POT", "31 BB". */
SHORTSTACKCORE_API std::string Detail(const Spot& S, Chips To);
/** The verb: "BET", "RAISE TO" or "ALL IN". */
SHORTSTACKCORE_API const char* Verb(const Spot& S, Chips To);
} // namespace sizing
} // namespace ss
