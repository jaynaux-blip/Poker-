#pragma once

#include "ShortStack/Common.h"
#include "ShortStack/Structure.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ss
{
namespace live
{
struct EventTemplate;
}

/**
 * The chips on the felt (next_segment_spec §6.2 B): the Embercrest's tournament set (100, 500, 1,000, 5,000, 25,000)
 * and Dee's cash chips, laid out the way a real room stacks them (columns of at most 20, one denomination each, at most
 * one short column per denomination, the biggest chips at the front; a bet slides the fewest chips), the starting bags,
 * the colour-ups, and what a glance at a stack can honestly tell (the TDA's countable-stack norm: full columns count
 * exactly, a short one to the nearest five chips). Spoken says an amount the way a dealer does.
 *
 * Pure and deterministic: no randomness, no clock. Dee's game keeps today's mix (ABackRoomChips::Break, exactly).
 */
namespace chipset
{
struct Denom
{
	Chips Value = 0;
	uint32_t Body = 0;   // the chip's colour (0xRRGGBB)
	uint32_t Insert = 0; // the edge spots and the inlay's ring
	uint32_t Ink = 0;    // the number on the inlay
	const char* Mesh = nullptr; // "SM_Chip_T100" (the Embercrest), "SM_Chip_25" (Dee's)
};
enum class Room : int
{
	BackRoom,
	Embercrest,
};
/** Every chip the room has, smallest first. */
SHORTSTACKCORE_API const std::vector<Denom>& Set(Room R);
/** The chip of that value in the room's set (nullptr: none). */
SHORTSTACKCORE_API const Denom* Find(Room R, Chips Value);
/**
 * The chips in play at LevelIndex (0-based): the Embercrest races a chip off at the first break after which every
 * blind and ante is a multiple of the next chip up (the black 100s before level 13, the purple 500s before level 19,
 * the yellow 1,000s before level 25 on the deepstack structure). Dee's game uses all four, always.
 */
SHORTSTACKCORE_API std::vector<Denom> InPlay(Room R, const std::vector<Level>& Levels, int LevelIndex, int BreakEvery);
enum class Style : int
{
	Stack, // a player's chips: working chips in the small denominations, the rest in the big ones
	Bet,   // what a player slides out: the fewest chips
	Pot,   // the middle: as a stack, with deeper small columns
};
/** Count chips of InPlay[Denom] (Value: that chip's value). Denom is -1 for a chip that has left play (the race is off:
 *  a stack's odd hundreds stay on the felt as one short column of the old chip); look it up by Value with Find. */
struct Column
{
	int Denom = 0;
	int Count = 0;
	Chips Value = 0;
};
/**
 * Amount in columns, front to back (the biggest chips first; each denomination's short column last). Exact whenever
 * the room's chips can make it: what InPlay can't make is laid in the room's smaller chips as an old-chip remainder
 * (Denom -1). Below the room's smallest chip nothing is laid (the engine's stacks are whole hundreds at the Embercrest).
 * Replaces ABackRoomChips::Break.
 */
SHORTSTACKCORE_API std::vector<Column> Lay(Chips Amount, const std::vector<Denom>& InPlay, Style S);
/** What the columns hold. */
SHORTSTACKCORE_API Chips Sum(const std::vector<Column>& Columns);
/** The bag at the seat when the cards go in the air (Noon and Warm-Up 15,000; Nightly and Sunday 20,000; Big Stack
 *  30,000; Turbo 10,000), indexed into InPlay at level 1. */
SHORTSTACKCORE_API std::vector<Column> StartingBag(const live::EventTemplate& T);
/** A stack as a glance reads it: Shown is the estimate, the truth is between Low and High; Exact when it was counted
 *  (or every column is full). */
struct Read
{
	Chips Low = 0;
	Chips High = 0;
	Chips Shown = 0;
	bool Exact = false;
};
/** Counted: a long look (1.5 s of Focus), or the dealer's count facing an all-in. */
SHORTSTACKCORE_API Read ReadStack(Chips Exact, const std::vector<Denom>& InPlay, bool Counted);
/** "Eighteen thousand four hundred.", "Sixteen hundred.", "One million.": an amount as the dealer says it. */
SHORTSTACKCORE_API std::string Spoken(Chips Amount);
/** "11.5 BB", "9.8 BB", "31 BB", "124 BB": one decimal under 100 big blinds (a trailing .0 dropped; never rounded up to
 *  the next whole big blind, so 9.96 is 9.9 and 14.97 is 14.9), whole from 100. Empty without a big blind. */
SHORTSTACKCORE_API std::string InBb(Chips Amount, Chips Bb);
/** The number InBb prints, without " BB". */
SHORTSTACKCORE_API std::string BbNumber(Chips Amount, Chips Bb);
} // namespace chipset
} // namespace ss
