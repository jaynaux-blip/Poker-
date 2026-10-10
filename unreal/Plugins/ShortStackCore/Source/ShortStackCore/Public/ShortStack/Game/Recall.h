#pragma once

#include "ShortStack/Common.h"
#include "ShortStack/Hand.h"
#include "ShortStack/Structure.h"

#include <string>
#include <utility>
#include <vector>

namespace ss
{
/**
 * The hands the player played, honestly (next_segment_spec §6.3): the table card's HANDS page and the last-hand card
 * above the hero block. A record holds only what the player saw: their own cards, the board, the actions and the
 * cards shown down. Written into the checkpoint's host text (recall\t<n>\t<escaped>, the last 10 hands).
 *
 * Bodies: core-tableui (Recall.cpp). Phase A, day 1: stubs that compile and change nothing.
 */
namespace recall
{
struct Line
{
	Street On = Street::Preflop;
	std::string Who;
	std::string What;
	Chips To = 0;
};
struct Record
{
	int Number = 0;
	int Level = 0;
	ss::Level Blinds;
	std::vector<Card> Mine;
	std::vector<Card> Board;
	std::vector<Line> Lines;
	std::vector<std::pair<std::string, std::vector<Card>>> Shown;
	Chips Pot = 0;
	Chips HeroNet = 0;
};
SHORTSTACKCORE_API Record Of(const Hand& H, int HeroSeat, const std::vector<std::string>& NameBySeat, const std::vector<int>& ShownSeats, int Number, int Level);
SHORTSTACKCORE_API std::string Write(const Record& R);
SHORTSTACKCORE_API bool Read(const std::string& Text, Record& Out);
} // namespace recall
} // namespace ss
