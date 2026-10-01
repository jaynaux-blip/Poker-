#pragma once

#include "ShortStack/Common.h"

namespace ss
{
class Rng;

enum class Street : int
{
	Preflop,
	Flop,
	Turn,
	River,
	Showdown,
};

/** What a player did, as recorded (a call of zero is recorded as a check). */
enum class ActionType : int
{
	Fold,
	Check,
	Call,
	Bet,
	Raise,
};

/** What a player asks to do. `To` is the total street bet for a raise (may be fractional; floored). */
struct PlayerAction
{
	enum class Kind : int
	{
		Fold,
		Check,
		Call,
		Raise,
	};
	Kind Type = Kind::Fold;
	double To = 0.0;

	static PlayerAction Fold() { return {Kind::Fold, 0.0}; }
	static PlayerAction Check() { return {Kind::Check, 0.0}; }
	static PlayerAction Call() { return {Kind::Call, 0.0}; }
	static PlayerAction RaiseTo(double InTo) { return {Kind::Raise, InTo}; }
};

struct SeatInput
{
	int Seat = 0; // physical seat number, 0..8
	std::string Id;
	Chips Stack = 0;
};

struct HandSeat
{
	int Seat = 0;
	std::string Id;
	Chips StartStack = 0;
	Chips Stack = 0;
	std::vector<Card> Hole;
	Chips StreetBet = 0;
	Chips Committed = 0; // total put in this hand, including ante
	Chips AnteCommitted = 0;
	bool Folded = false;
	bool AllIn = false;
	bool HasActed = false;
	bool RaiseLocked = false;
};

struct ActionRecord
{
	Street OnStreet = Street::Preflop;
	int Seat = 0;
	ActionType Type = ActionType::Fold;
	Chips Added = 0;  // chips put in by this action
	Chips To = 0;     // player's street bet after the action
	bool AllIn = false;
	Chips PotBefore = 0;
	Chips Facing = 0; // amount to call before acting
};

struct PotResult
{
	Chips Amount = 0;
	std::vector<int> Eligible; // seat numbers
	std::vector<int> Winners;
	std::vector<Chips> Shares;
};

struct ShowdownHand
{
	int Seat = 0;
	std::vector<Card> Hole;
	int Score = 0;
};

enum class EventType : int
{
	Ante,
	Blind,
	Deal,
	Action,
	Street,
	Return,
	Reveal,
	Showdown,
	Award,
	End,
};

/** One thing that happened in a hand; the UI animates these and replays are built from them. */
struct HandEvent
{
	EventType Type = EventType::End;
	int Seat = 0;
	Chips Amount = 0;
	bool IsSmallBlind = false;            // Blind
	std::vector<int> Seats;               // Deal (dealing order), Reveal
	ActionRecord Action;                  // Action
	Street NewStreet = Street::Preflop;   // Street
	std::vector<Card> Cards;              // Street: cards dealt now
	std::vector<Card> Board;              // Street: full board
	std::vector<ShowdownHand> Hands;      // Showdown
	PotResult Pot;                        // Award
	int PotIndex = 0;                     // Award
	int PotCount = 0;                     // Award
};

struct LegalActions
{
	bool CanFold = true;
	bool CanCheck = false;
	Chips CallAmount = 0; // capped by stack
	bool CanRaise = false;
	Chips MinRaiseTo = 0;
	Chips MaxRaiseTo = 0; // all-in
	bool IsBet = false;   // no bet yet this street
};

struct HandConfig
{
	std::vector<SeatInput> Players;
	int ButtonSeat = 0;
	Chips SmallBlind = 0;
	Chips BigBlind = 0;
	Chips Ante = 0; // big-blind ante, 0 for none
	/** Optional fixed deck (top first) for tests; otherwise shuffled from the RNG. */
	std::vector<Card> Deck;
};

/**
 * One hand of No-Limit Texas Hold'em: moving button with heads-up blinds,
 * big-blind ante as dead money, min-raise rules, incomplete all-in raises that
 * do not reopen action, uncalled-bet returns, side pots and odd chips to the
 * first winner left of the button. Port of web/src/core/hand.ts.
 */
class Hand
{
public:
	SHORTSTACKCORE_API Hand(const HandConfig& Config, Rng& R);

	/** False if the config was invalid (fewer than two players, button on an empty seat). */
	bool IsValid() const { return bValid; }

	/** Applies an action for the player to act. Returns false (and changes nothing) if it is illegal. */
	SHORTSTACKCORE_API bool Act(const PlayerAction& Action);
	SHORTSTACKCORE_API LegalActions GetLegalActions() const;

	SHORTSTACKCORE_API Chips Pot() const;
	SHORTSTACKCORE_API Chips PotBeforeStreet() const;
	SHORTSTACKCORE_API const HandSeat* SeatByNumber(int SeatNumber) const;
	SHORTSTACKCORE_API int ActiveCount() const;
	SHORTSTACKCORE_API std::vector<PotResult> BuildPots() const;

	std::vector<HandSeat> Seats;
	std::vector<HandEvent> Events;
	std::vector<ActionRecord> History;
	std::vector<Card> Board;
	Chips SmallBlind = 0;
	Chips BigBlind = 0;
	Chips Ante = 0;
	int ButtonSeat = 0;

	Street CurrentStreet = Street::Preflop;
	int ToAct = -1; // index into Seats, -1 when nobody
	Chips CurrentBet = 0;
	Chips MinRaiseInc = 0;
	int LastAggressor = -1;
	bool bComplete = false;
	std::vector<PotResult> PotResults;

	int ButtonIndex() const { return ButtonIdx; }
	int SmallBlindIndex() const { return SbIdx; }
	int BigBlindIndex() const { return BbIdx; }

private:
	Chips Commit(HandSeat& S, Chips Amount);
	int NextIdx(int From) const;
	void Start();
	Card Draw();
	int FindNextToAct(int From) const;
	int AbleCount() const;
	void Advance();
	void FinishStreet();
	void DealNextStreet();
	void ReturnUncalled();
	void AwardUncontested();
	void DoShowdown();
	void End();

	std::vector<Card> Deck;
	int DeckPos = 0;
	int ButtonIdx = 0;
	int SbIdx = -1;
	int BbIdx = -1;
	bool bValid = true;
	bool bRevealed = false;
};
} // namespace ss
