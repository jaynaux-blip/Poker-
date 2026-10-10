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
 * and Dee's cash chips, laid out the way a real room stacks them (columns of at most 20, one denomination each, the
 * biggest at the front), the starting bags, the colour-ups, and what a glance at a stack can honestly tell (the TDA's
 * countable-stack norm). Spoken says an amount the way a dealer does.
 *
 * Bodies: core-tableui (ChipSet.cpp). Phase A, day 1: stubs that compile and change nothing; ReadStack reads every
 * stack exactly (what the plates show under SHOWN).
 */
namespace chipset
{
struct Denom
{
	Chips Value = 0;
	uint32_t Body = 0;
	uint32_t Insert = 0;
	uint32_t Ink = 0;
	const char* Mesh = nullptr;
};
enum class Room : int
{
	BackRoom,
	Embercrest,
};
SHORTSTACKCORE_API const std::vector<Denom>& Set(Room R);
SHORTSTACKCORE_API std::vector<Denom> InPlay(Room R, const std::vector<Level>& Levels, int LevelIndex, int BreakEvery);
enum class Style : int
{
	Stack,
	Bet,
	Pot,
};
/** Count chips of InPlay[Denom]. */
struct Column
{
	int Denom = 0;
	int Count = 0;
};
/** Replaces ABackRoomChips::Break. */
SHORTSTACKCORE_API std::vector<Column> Lay(Chips Amount, const std::vector<Denom>& InPlay, Style S);
SHORTSTACKCORE_API std::vector<Column> StartingBag(const live::EventTemplate& T);
struct Read
{
	Chips Low = 0;
	Chips High = 0;
	Chips Shown = 0;
	bool Exact = false;
};
SHORTSTACKCORE_API Read ReadStack(Chips Exact, const std::vector<Denom>& InPlay, bool Counted);
/** "Eighteen thousand four hundred." */
SHORTSTACKCORE_API std::string Spoken(Chips Amount);
SHORTSTACKCORE_API std::string InBb(Chips Amount, Chips Bb);
} // namespace chipset
} // namespace ss
