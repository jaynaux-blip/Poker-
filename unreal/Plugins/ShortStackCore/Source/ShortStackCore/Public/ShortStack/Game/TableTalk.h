#pragma once

#include "ShortStack/Common.h"

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace life
{
struct LiveVoice;
}

/**
 * What the card room's people say (next_segment_spec §4.5 to §4.8): the field's table talk by personality, the
 * dealers' calls, the floor's PA copy, and the Back Room's eight word for word. Every line claims only what the engine
 * or the save knows that tick (N4): a speaker's results come from their own Story, nobody talks about anyone's money
 * but their own (N3), and an empty Text means there's nothing they'd honestly say. Templates fill {Vars}. Every
 * inventory passes the copy lint (tests/CopyLint.h).
 *
 * Bodies: core-room (TableTalk.cpp). Phase A, day 1: stubs that compile and say nothing.
 */
namespace talk
{
enum class Moment : int
{
	SitDown,
	Small,
	Ask,
	Answer,
	Needle,
	NiceHand,
	Suckout,
	BadBeat,
	Cooler,
	ShowedBluff,
	DoubleUp,
	Goodbye,
	Tank,
	Bubble,
	InTheMoney,
	FinalTable,
	Break,
	Noticed,
	Win,
	Lose,
	Rail,
};
struct Context
{
	const life::LiveVoice* Self = nullptr;
	const life::LiveVoice* To = nullptr;
	std::map<std::string, std::string> Vars;
	uint32_t Salt = 0;
	const std::set<std::string>* Told = nullptr;
};
/** Empty Text: nothing they'd say. Key: what it spends (told once); Topic: what it's about. */
struct Said
{
	std::string Text;
	std::string Key;
	std::string Topic;
};
SHORTSTACKCORE_API Said Line(Moment M, const Context& C);
SHORTSTACKCORE_API Said Dealer(const std::string& Call, const Context& C);
SHORTSTACKCORE_API Said Floor(const std::string& Call, const std::map<std::string, std::string>& Vars, uint32_t Salt);
/** The Back Room's eight, word for word. */
SHORTSTACKCORE_API Said Cast(const std::string& Name, Moment M, uint32_t Salt);
/** "aces", "seven-four", "ace-king suited". */
SHORTSTACKCORE_API std::string SpokenHand(const std::vector<Card>& Hole);
SHORTSTACKCORE_API std::string CaptionName(const std::string& Id, const std::string& Name, int Seat);
/** Every template and variant, by speaker (the copy lint reads it). */
SHORTSTACKCORE_API std::vector<std::pair<std::string, std::string>> Inventory();
} // namespace talk
} // namespace ss
