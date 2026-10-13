#pragma once

#include "ShortStack/AI/Profiles.h"
#include "ShortStack/AI/Style.h"
#include "ShortStack/Game/Recall.h"

#include <string>
#include <vector>

namespace ss
{
class Rng;

/**
 * Dee's skip (update 2 §2.6.3, P1): hands at a cash table played headless, the hero by the plan, until a felt, a
 * stop-loss or stop-win, closing or the energy floor. Declared in Phase A; bodies by core-autoplay (Game/CashSim.cpp) when
 * the P1 lands. Nothing calls it before then.
 */
namespace cash
{
struct Rules
{
	int MaxHands = 30;
	Chips StopLoss = 0, StopWin = 0;
	double ClosingAt = 0, MinutesPerHand = 4.5;
	int MinEnergy = 4;
};
struct Seat
{
	std::string Name;
	Profile Prof;
	Chips Stack = 0;
	bool Hero = false;
};
struct Out
{
	int Hands = 0;
	std::vector<Chips> Stacks;
	double Minutes = 0;
	std::string Why;
	std::vector<recall::Record> HeroHands;
};
SHORTSTACKCORE_API Out PlayHands(std::vector<Seat>& Seats, int Button, const style::HeroBrain& B, Rng& R, const Rules& Ru, Chips Small, Chips Big);
} // namespace cash
} // namespace ss
