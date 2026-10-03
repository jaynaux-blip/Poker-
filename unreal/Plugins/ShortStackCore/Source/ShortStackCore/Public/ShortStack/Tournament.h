#pragma once

#include "ShortStack/AI/Bot.h"
#include "ShortStack/AI/Profiles.h"
#include "ShortStack/Hand.h"
#include "ShortStack/Rng.h"
#include "ShortStack/Structure.h"

#include <map>
#include <memory>
#include <set>
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
	// Breaks (C++ only, the live rooms): after every BreakEvery levels the room takes BreakMinutes off, no hands dealt.
	int BreakEvery = 0;
	double BreakMinutes = 0.0;
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
	Break, // a break before the level in LevelNumber (C++ only)
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

extern SHORTSTACKCORE_API const char* const HeroId; // "hero"

/**
 * Multi-table tournament. Every table plays real hands with the same engine:
 * the hero's table with full-strength bots, the rest of the field with the
 * lightweight bots. One tick is one hand at every table (hand-for-hand
 * timing). Port of web/src/core/tournament.ts.
 */
class Tournament
{
public:
	SHORTSTACKCORE_API Tournament(const TournamentSpec& InSpec, const std::string& HeroName, const std::string& Seed, const std::vector<ReservedPlayer>& Reserved = {});

	// ---------------------------------------------------------------- queries
	const TPlayer& Hero() const { return Players[static_cast<size_t>(HeroIndex)]; }
	SHORTSTACKCORE_API const Level& CurrentLevel() const;
	SHORTSTACKCORE_API const Level& NextLevel() const;
	int PaidPlaces() const { return static_cast<int>(Payouts.size()); }
	bool InTheMoney() const { return Remaining <= PaidPlaces(); }
	SHORTSTACKCORE_API bool HandForHand() const;
	double ElapsedSeconds() const { return static_cast<double>(Tick) * Spec.SecondsPerHand; }
	SHORTSTACKCORE_API double LevelSecondsLeft() const;
	/** The room's clock: the start, the hands played, and the breaks taken. */
	double ClockMinutes() const { return Spec.StartClock + ElapsedSeconds() / 60.0 + static_cast<double>(BreaksTaken) * Spec.BreakMinutes; }
	SHORTSTACKCORE_API double AverageStack() const;
	SHORTSTACKCORE_API std::vector<const TPlayer*> AlivePlayers() const;
	SHORTSTACKCORE_API std::vector<const TPlayer*> Standings() const;
	SHORTSTACKCORE_API int HeroRank() const;
	SHORTSTACKCORE_API Chips PrizeFor(int Place) const;
	SHORTSTACKCORE_API double PressureFor(const TPlayer& P) const;
	SHORTSTACKCORE_API int PlayerIndex(const std::string& Id) const;

	// ---------------------------------------------------------------- play
	/** Rebalance tables, then deal the hero's next hand (nullptr when the hero is not playing). */
	SHORTSTACKCORE_API std::unique_ptr<Hand> StartTick(std::vector<TEvent>& OutEvents);
	/** Apply the hero's hand (if any), play every other table, process eliminations and the clock. */
	SHORTSTACKCORE_API std::vector<TEvent> FinishTick(Hand* HeroHand, const Profile* HeroAuto = nullptr);
	/**
	 * FinishTick in steps (C++ only), so a big field's other tables can be played over several frames: BeginFinish applies
	 * the hero's hand, FinishSome plays up to Count more tables (true once none are left), EndFinish processes eliminations
	 * and the clock. Same order and same results as FinishTick, which is these three in a row. Nothing else may touch the
	 * tournament between BeginFinish and EndFinish.
	 */
	SHORTSTACKCORE_API void BeginFinish(Hand* HeroHand, const Profile* HeroAuto = nullptr);
	SHORTSTACKCORE_API bool FinishSome(int Count);
	SHORTSTACKCORE_API std::vector<TEvent> EndFinish();
	bool FinishPending() const { return bFinishing; }
	/** A whole round with no interactive hand (hero busted or sprinting). */
	SHORTSTACKCORE_API std::vector<TEvent> SimulateTick(const Profile* HeroAuto = nullptr);
	/** Next bot decision at a live hand (full strength unless Fast). */
	SHORTSTACKCORE_API BotDecision BotDecisionFor(const Hand& H, bool Fast, const Profile* ProfileOverride = nullptr);
	/** Break tables that are no longer needed and even out table sizes. */
	SHORTSTACKCORE_API std::vector<TEvent> Balance();
	/** Scripted move (story beats), e.g. seating the rival at the hero's table. */
	SHORTSTACKCORE_API std::vector<TEvent> MoveToTable(const std::string& Id, int TableId);

	// ---------------------------------------------------------------- checkpoints (C++ only, the live rooms)
	/**
	 * The tournament between ticks as one line of text: everything play changes (stacks, seats, buttons, the clock,
	 * the flags, the draw's state), none of what the field was made with. Empty while a tick is being finished.
	 */
	SHORTSTACKCORE_API std::string Checkpoint() const;
	/**
	 * Carries on from a checkpoint: this tournament must have been made the same way as the one that wrote it (the
	 * same spec, hero, seed and reserved players), and then plays on exactly as that one would have. False, with
	 * nothing changed, for text that isn't a checkpoint of this field.
	 */
	SHORTSTACKCORE_API bool Restore(const std::string& Text);

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
	/** Breaks the room has taken (C++ only): each is Spec.BreakMinutes on the clock. */
	int BreaksTaken = 0;
	/** The hero's seat is held but not dealt in (late registration, C++ only): their table plays without them. */
	bool HeroAway = false;
	/** The hero has walked away (C++ only): dealt in, they check or fold every hand until blinded off. */
	bool HeroSitsOut = false;

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
	// A tick being finished in steps.
	bool bFinishing = false;
	bool FinishWithHero = false;
	int FinishHeroTable = -1;
	int FinishNextTable = 0; // the first table id not played yet
};
} // namespace ss
