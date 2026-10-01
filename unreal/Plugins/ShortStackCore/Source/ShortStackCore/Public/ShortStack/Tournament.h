#pragma once

#include "ShortStack/AI/Bot.h"
#include "ShortStack/AI/Profiles.h"
#include "ShortStack/Hand.h"
#include "ShortStack/Rng.h"
#include "ShortStack/Structure.h"

#include <map>
#include <memory>
#include <unordered_map>

namespace ss
{
struct TournamentSpec
{
	std::string Id;
	std::string Name;
	Chips BuyInCents = 0; // total paid, including fee
	Chips FeeCents = 0;
	Chips GuaranteeCents = 0; // prize pool floor (fixed pool for freerolls)
	int Entrants = 0;
	Chips StartingStack = 10000;
	double LevelMinutes = 5.0;
	double SecondsPerHand = 42.0;
	std::string Population = "micro"; // freeroll, micro, low, high
	std::string Speed = "Turbo";
	double StartClock = 0.0; // game clock at the start, minutes after midnight
	int TableSize = 9;
	std::vector<Level> Levels; // empty = StandardLevels()
	// Formats on top of the TypeScript engine (C++ only; zero keeps the shared behavior).
	Chips BountyCents = 0;    // part of each buy-in that goes on players' heads (PKO), out of the prize pool
	Chips SeatValueCents = 0; // satellites: the prize pool is paid as seats of this value
	bool MysteryBounty = false; // bounties are drawn from envelopes once in the money (the session pays them)
	std::string SeatTicket;     // satellites: the event a seat enters (schedule template id)
};

struct TPlayer
{
	std::string Id;
	std::string Name;
	bool IsHero = false;
	bool HasProfile = false;
	Profile Prof;
	Chips Stack = 0;
	int TableId = -1;
	int Seat = -1;
	bool Busted = false;
	int Place = 0;
	Chips PrizeCents = 0;
	double Tilt = 0.0;
	int Hands = 0;
	int VpipHands = 0;
	int PfrHands = 0;
	std::string KnockedOutBy; // who won the pot that busted this player (C++ only)
};

struct TTable
{
	int Id = 0;
	std::vector<int> Seats; // player index or -1
	int ButtonSeat = -1;
};

enum class TEventType : int
{
	Bust,
	Level,
	Moved,
	TableBroken,
	HandForHand,
	Bubble,
	FinalTable,
	Finished,
};

struct TEvent
{
	TEventType Type = TEventType::Bust;
	std::string Id;   // Bust, Moved
	std::string Name; // Bust; bubble player for Bubble; winner for Finished
	int Place = 0;    // Bust
	Chips PrizeCents = 0;
	int TableId = 0;  // Bust, TableBroken
	int From = 0;     // Moved
	int To = 0;       // Moved
	bool IsHero = false;
	int LevelNumber = 0; // Level (1-based)
	Level Blinds;        // Level
	std::string EliminatedBy; // Bust: the player who won the pot (C++ only)
};

struct ReservedPlayer
{
	std::string Name;
	Archetype Type = Archetype::Crusher;
};

extern const char* const HeroId; // "hero"

/**
 * Multi-table tournament. Every table plays real hands with the same engine:
 * the hero's table with full-strength bots, the rest of the field with the
 * lightweight bots. One tick is one hand at every table (hand-for-hand
 * timing). Port of web/src/core/tournament.ts.
 */
class Tournament
{
public:
	Tournament(const TournamentSpec& InSpec, const std::string& HeroName, const std::string& Seed, const std::vector<ReservedPlayer>& Reserved = {});

	// ---------------------------------------------------------------- queries
	const TPlayer& Hero() const { return Players[static_cast<size_t>(HeroIndex)]; }
	const Level& CurrentLevel() const;
	const Level& NextLevel() const;
	int PaidPlaces() const { return static_cast<int>(Payouts.size()); }
	bool InTheMoney() const { return Remaining <= PaidPlaces(); }
	bool HandForHand() const;
	double ElapsedSeconds() const { return static_cast<double>(Tick) * Spec.SecondsPerHand; }
	double LevelSecondsLeft() const;
	double ClockMinutes() const { return Spec.StartClock + ElapsedSeconds() / 60.0; }
	double AverageStack() const;
	std::vector<const TPlayer*> AlivePlayers() const;
	std::vector<const TPlayer*> Standings() const;
	int HeroRank() const;
	Chips PrizeFor(int Place) const;
	double PressureFor(const TPlayer& P) const;
	int PlayerIndex(const std::string& Id) const;

	// ---------------------------------------------------------------- play
	/** Rebalance tables, then deal the hero's next hand (nullptr when the hero is not playing). */
	std::unique_ptr<Hand> StartTick(std::vector<TEvent>& OutEvents);
	/** Apply the hero's hand (if any), play every other table, process eliminations and the clock. */
	std::vector<TEvent> FinishTick(Hand* HeroHand, const Profile* HeroAuto = nullptr);
	/** A whole round with no interactive hand (hero busted or sprinting). */
	std::vector<TEvent> SimulateTick(const Profile* HeroAuto = nullptr);
	/** Next bot decision at a live hand (full strength unless Fast). */
	BotDecision BotDecisionFor(const Hand& H, bool Fast, const Profile* ProfileOverride = nullptr);
	/** Break tables that are no longer needed and even out table sizes. */
	std::vector<TEvent> Balance();
	/** Scripted move (story beats), e.g. seating the rival at the hero's table. */
	std::vector<TEvent> MoveToTable(const std::string& Id, int TableId);

	TournamentSpec Spec;
	Rng R;
	std::vector<TPlayer> Players; // insertion order: hero, reserved, then the field
	std::map<int, TTable> Tables; // ordered by id, like the TypeScript Map insertion order
	std::vector<Chips> Payouts;   // cents by place (index 0 = 1st)
	Chips PrizePoolCents = 0;
	std::vector<Level> Levels;
	int TableSize = 9;
	int Tick = 0;
	int LevelIndex = 0;
	int Remaining = 0;
	bool bFinished = false;

private:
	std::unique_ptr<Hand> MakeHand(TTable& Table);
	void PlayInstant(Hand& H, bool Fast, const Profile* HeroAuto);
	void ApplyHand(const Hand& H);
	std::vector<TEvent> ProcessEliminations();
	int Count(const TTable& T) const;
	void SeatPlayer(int PlayerIdx, TTable& Table);
	void MovePlayer(int PlayerIdx, TTable& To, std::vector<TEvent>& Events);

	std::unordered_map<std::string, int> IndexById;
	std::unordered_map<std::string, Chips> StartStacks;
	int HeroIndex = 0;
	bool bAnnouncedFinal = false;
	bool bAnnouncedH4H = false;
	bool bBurstBubble = false;
};
} // namespace ss
