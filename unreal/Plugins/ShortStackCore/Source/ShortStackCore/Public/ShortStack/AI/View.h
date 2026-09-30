#pragma once

#include "ShortStack/Hand.h"

namespace ss
{
/** Public information about one seat. */
struct PublicSeat
{
	int Seat = 0;
	std::string Id;
	Chips Stack = 0;
	Chips StreetBet = 0;
	Chips Committed = 0;
	bool Folded = false;
	bool AllIn = false;
};

/**
 * Everything a player may know when deciding: public table state plus their
 * own hole cards. Bots and the grader only ever see this, which is how the
 * "AI never sees hidden cards" guarantee is enforced.
 */
struct PlayerView
{
	int Me = 0; // index into Seats
	std::vector<Card> Hole;
	std::vector<PublicSeat> Seats;
	std::vector<Card> Board;
	Street CurrentStreet = Street::Preflop;
	std::vector<ActionRecord> History;
	int ButtonIdx = 0;
	int BbIdx = 0;
	int SbIdx = 0;
	Chips SmallBlind = 0;
	Chips BigBlind = 0;
	Chips Ante = 0;
	Chips Pot = 0;
	Chips CurrentBet = 0;
	Chips ToCall = 0;
	Chips MinRaiseTo = 0;
	Chips MaxRaiseTo = 0;
	bool CanRaise = false;
	bool CanCheck = false;
};

PlayerView MakeView(const Hand& H, int SeatIdx);

enum class Position : int
{
	BTN,
	SB,
	BB,
	UTG,
	UTG1,
	MP,
	LJ,
	HJ,
	CO,
};

const char* PositionName(Position P);
Position PositionOf(const PlayerView& View, int Idx);
/** Players still to act behind Idx preflop who have not folded. */
int PlayersBehind(const PlayerView& View, int Idx);

struct StreetSummary
{
	int Raises = 0;
	int Limpers = 0;
	int LastRaiserSeat = -1; // -1 when nobody raised
	Chips LastRaiseTo = 0;
};

StreetSummary PreflopSummary(const PlayerView& View);
int SeatIndex(const PlayerView& View, int Seat);
} // namespace ss
