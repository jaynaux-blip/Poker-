#pragma once

#include "ShortStack/AI/Grading.h"
#include "ShortStack/Game/Chat.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Lobby.h"
#include "ShortStack/Rng.h"
#include "ShortStack/Tournament.h"

#include <functional>
#include <map>
#include <memory>
#include <set>

namespace ss
{
enum class Pace : int
{
	Full,
	Smart,
	Sprint,
};

enum class Screen : int
{
	Boot,
	Lobby,
	Table,
	Results,
};

enum class RightTab : int
{
	Info,
	Chat,
	Payouts,
};

enum class SoundId : int
{
	Chip,
	ChipStack,
	Deal,
	Flip,
	Check,
	Fold,
	Turn,
	Win,
	Level,
	Bust,
	Click,
	Alert,
	Move,
	AllIn,
	Bubble,
	Cash,
};

/** What the viewer sees at one seat of the hero's table. */
struct SeatVis
{
	bool Present = false;
	int Seat = 0;
	std::string Id;
	std::string Name;
	bool IsHero = false;
	bool IsRival = false;
	std::string Country; // ISO 3166 alpha-2 ("" when unknown)
	bool Regular = false; // one of the network's regulars (the leaderboards know them)
	bool Pro = false;     // Team RiverLine
	Chips Stack = 0;
	Chips Bet = 0;
	std::vector<Card> Hole; // known to the viewer (hero, or revealed)
	bool HasCards = false;
	bool Folded = false;
	bool AllIn = false;
	std::string LastAction;
	double LastActionAt = 0.0;
	bool Acting = false;
	double ActStart = 0.0;
	double ActEnd = 0.0;
	bool HasEquity = false;
	double Equity = 0.0;
	bool Winner = false;
	std::string HandLabel;
	double DealtAt = 0.0;
};

/** Where an animated chip stack or card flies from or to. */
struct FlightEnd
{
	enum class Kind : int
	{
		Seat,
		Pot,
		Bet, // a seat's bet spot
		Deck,
		Muck,
	};
	Kind Type = Kind::Seat;
	int Seat = 0;

	static FlightEnd AtSeat(int S) { return {Kind::Seat, S}; }
	static FlightEnd AtBet(int S) { return {Kind::Bet, S}; }
	static FlightEnd AtPot() { return {Kind::Pot, 0}; }
	static FlightEnd AtDeck() { return {Kind::Deck, 0}; }
	static FlightEnd AtMuck() { return {Kind::Muck, 0}; }
};

struct Flight
{
	bool IsChips = true;
	FlightEnd From;
	FlightEnd To;
	Chips Amount = 0;
	double Start = 0.0;
	double Dur = 0.0;
};

struct HeroPrompt
{
	DecisionAnalysis Analysis;
	double OpenedAt = 0.0;
	double Deadline = 0.0;
	double TimeBankUntil = 0.0;
	Chips RaiseTo = 0;
	Chips MinRaise = 0;
	Chips MaxRaise = 0;
	Chips ToCall = 0;
	bool CanCheck = false;
	bool CanRaise = false;
	bool IsBet = false;
	Chips Pot = 0;
	Chips BigBlind = 0;
	Street OnStreet = Street::Preflop;
};

struct GradeBadge
{
	DecisionGrade G;
	double At = 0.0;
};

struct Results
{
	std::string EventName;
	int Place = 0;
	int Entrants = 0;
	Chips PrizeCents = 0;
	Chips BuyInCents = 0;
	int Hands = 0;
	std::vector<DecisionGrade> Grades;
	double AccuracyPct = 0.0;
	Chips BiggestPot = 0;
	bool Won = false;
	Chips BountyCents = 0; // collected bounties (PKO and mystery)
	int Knockouts = 0;
	std::string SeatWon;   // satellites: the seat's event ("" when none)
	Chips SeatValueCents = 0;
	// The whole sitting, when it was more than one table (multi-tabling).
	int SessionEvents = 1;
	Chips SessionNetCents = 0; // bankroll now minus before the first buy-in
};

/** One tournament out of several finished while the others were still running. */
struct FinishedTable
{
	ss::Results Result;
	double At = 0.0;
};

/** A look at one open table without bringing it to the front (table tabs, tiles). */
struct TableGlance
{
	std::string EventId;
	std::string Name;
	bool YourTurn = false;
	double TurnOpenedAt = 0.0;
	double Deadline = 0.0;      // the action clock, or the time bank once it runs
	double DeadlineStart = 0.0; // when that clock started
	Chips Stack = 0;
	double StackBb = 0.0;
	int Rank = 0;
	int Remaining = 0;
	bool InMoney = false;
	bool Busted = false;  // waiting for the results to post
	bool Sprinting = false;
	bool AllIn = false;   // the hero's chips are in the middle
};

struct Banner
{
	bool Active = false;
	std::string Title;
	std::string Sub;
	double At = 0.0;
	uint32_t Color = 0xffffff; // 0xRRGGBB
};

struct HistoryEntry
{
	std::string Name;
	int Place = 0;
	int Entrants = 0;
	Chips Prize = 0;
	double AccuracyPct = 0.0;
	Chips BuyInCents = -1;  // -1 in saves from before the network schedule
	std::string EventId;    // the scheduled instance ("mm-26@1530"), if known
};

/** Everything that persists between runs. */
struct SaveData
{
	Chips BankrollCents = 237;
	std::string HeroName = "grinder_3c";
	std::vector<HistoryEntry> History;
	std::vector<std::string> TextsSeen;
	double ClockMinutes = 2.0 * 60.0 + 7.0; // the lobby clock
	life::State Life;

	/** Line-based text, safe to store in any save system. */
	SHORTSTACKCORE_API std::string Serialize() const;
	static SHORTSTACKCORE_API bool Parse(const std::string& Text, SaveData& Out);
};

/** How the session reaches the world: sounds, the phone, the heartbeat, the desk. */
class SessionHooks
{
public:
	virtual ~SessionHooks() = default;
	virtual void Sound(SoundId /*Id*/, double /*Volume*/) {}
	virtual void Text(const std::string& /*From*/, const std::string& /*Body*/) {}
	virtual void Heartbeat(bool /*On*/) {}
	virtual void AddCan() {}
	virtual void Celebrate() {}
	virtual void Save(const SaveData& /*Data*/) {}
	/** Leaves the apartment for a place the host plays out itself (Dee's game). False when it can't. */
	virtual bool GoOut(const std::string& /*ActivityId*/, Chips /*BuyInCents*/) { return false; }
};

/**
 * Night One game session: bankroll, lobby, and the moment-to-moment flow of
 * a tournament. Hands from the engine are replayed event by event with
 * realistic pacing so the client can animate them; the hero's decisions are
 * graded; tournament milestones trigger story beats. Port of
 * web/src/game/session.ts. Time is in seconds of game time.
 */
class Session
{
public:
	SHORTSTACKCORE_API Session(SessionHooks& InHooks, const std::string& Seed, const SaveData* Loaded = nullptr);

	// ------------------------------------------------------------ persistent
	Chips BankrollCents = 237;
	std::string HeroName = "grinder_3c";
	std::vector<HistoryEntry> History;

	// ------------------------------------------------------------ ui state
	ss::Screen CurrentScreen = ss::Screen::Boot;
	int Selected = 0;
	ss::Pace CurrentPace = ss::Pace::Smart;
	bool Hud = true;
	ss::RightTab Tab = ss::RightTab::Chat;
	bool ConfirmRegister = false;
	bool HasResults = false;
	ss::Results LastResults;
	double ResultsAt = 0.0;
	ss::Banner CurrentBanner;

	// ------------------------------------------------------------ tournament
	const LobbyEvent* Event = nullptr; // the event being played (points at Joined)
	LobbyEvent Joined;
	std::unique_ptr<Tournament> T;
	std::unique_ptr<Hand> CurHand;
	int TableId = 0;
	std::vector<SeatVis> Seats;
	int ButtonSeat = 0;
	std::vector<Card> Board;
	std::vector<double> BoardShownAt;
	Chips PotChips = 0;
	std::vector<Flight> Flights;
	std::vector<ChatLine> Chat;
	bool HasPrompt = false;
	HeroPrompt Prompt;
	std::vector<GradeBadge> Badges;
	std::vector<DecisionGrade> Grades;
	double HeroTilt = 0.0;
	bool SitOutNext = false;
	bool SittingOutThisHand = false;
	bool Sprinting = false;
	std::string SprintStopReason;
	int HandsPlayed = 0;
	Chips BiggestPot = 0;
	double TimeBank = 30.0;
	bool Moving = false;
	int MovingFrom = 0;
	int MovingTo = 0;
	double MovingAt = 0.0;
	bool AutoFolded = false;
	std::vector<Card> AutoFoldedCards;
	double AutoFoldedAt = 0.0;
	double LastLevelUpAt = -100.0;
	bool HeroAllInReveal = false;
	double Now = 0.0;
	/** Real-time budget per update for Sprint simulation. */
	double SprintBudgetMs = 10.0;

	SHORTSTACKCORE_API void Save();
	SHORTSTACKCORE_API void ResetSave();
	void OnBoot();

	bool CanAfford(const LobbyEvent& Ev) const;
	/** Registers for a Lobby() event (tonight's hand-tuned listings). */
	void Register(int Index);
	/** Registers for any joinable listing (the network schedule builds them); pays the buy-in and opens the table. */
	SHORTSTACKCORE_API void RegisterEvent(const LobbyEvent& Listing);

	void DealerLine(const std::string& Text);
	void SystemLine(const std::string& Text);
	void Say(const std::string& Who, const std::string& Text);

	void StartNextHand();
	SHORTSTACKCORE_API void Update(double InNow);
	void HeroAct(const PlayerAction& Action, bool TimedOut = false);
	void RequestSitOut();
	void BeginSprint();
	void StopSprint(const std::string& Reason = "Stopped");
	void LeaveResults();

	// ------------------------------------------------------------ multi-tabling
	// Several tournaments at once. The table in front lives in the fields above (T, CurHand, Seats, Prompt, ...);
	// the others wait in Runs and take their turn in Update. Every open table keeps playing, wherever the player looks.
	static constexpr int TableLimit = 4;
	/** How many tables the player can handle: TableLimit, two when exhausted. */
	SHORTSTACKCORE_API int MaxTables() const;
	int TableCount() const { return static_cast<int>(Runs.size()); }
	/** The table in front (index into the open tables), -1 with none open. */
	int FocusedTable() const { return Active; }
	/** Brings an open table to the front. */
	SHORTSTACKCORE_API void FocusTable(int Index);
	/** Runs Fn with table Index in front (S.T, S.Seats, S.Prompt... are its), then puts the front table back. Fn must not open or close tables. */
	SHORTSTACKCORE_API void WithTable(int Index, const std::function<void()>& Fn);
	SHORTSTACKCORE_API TableGlance Glance(int Index) const;
	/** Acts at table Index (a tile's buttons): brings it to the front and acts there. */
	SHORTSTACKCORE_API void HeroActAt(int Index, const PlayerAction& Action);
	/** Already seated in this scheduled instance. */
	SHORTSTACKCORE_API bool IsPlaying(const std::string& EventId) const;
	/** Open tables where it's the player's turn. */
	SHORTSTACKCORE_API int TablesWaiting() const;
	/** To the lobby with tables still running (they keep playing), and back. */
	SHORTSTACKCORE_API void ShowLobby();
	SHORTSTACKCORE_API void ShowTables();
	/** All tables at once, each with its own buttons (the tile view), or one in front at a time. */
	bool Tiled = false;
	/** With one table in front: when it doesn't need the player and another does, that one comes forward. */
	bool AutoFocus = true;
	/** The player is out (or has won) and the table is about to post its result. */
	bool Finishing() const { return HasBustInfo; }
	/** Events of the current hand played out so far (tests watch it move). */
	size_t HandProgress() const { return Cursor; }
	/** Tournaments that ended while others were still running, newest last (toasts). */
	std::vector<FinishedTable> Finished;
	/** Tournament clock (drives the dawn outside), or the lobby clock. Minutes after midnight on Night One. */
	SHORTSTACKCORE_API double ClockMinutes() const;
	/** The clock on the network's calendar (net::DayOf, net::TimeLabel). */
	SHORTSTACKCORE_API double WorldMinutes() const;
	/** Back from somewhere the host played out (Dee's game): calendar events since World still happen. */
	void ResumeCalendarFrom(double World) { CalendarAt = World; }
	/** The clock between tournaments: 2:07 AM at first, running in real time, and where the last tournament ended. */
	double LobbyMinutes = 2.0 * 60.0 + 7.0;

	// ------------------------------------------------------------ life (Life.h)
	life::State Life;
	/** A shift, a hustle or sleep in progress: the clock races from From to To while the room plays it out. */
	struct Skip
	{
		bool Active = false;
		double From = 0.0; // world minutes
		double To = 0.0;
		double RealStart = 0.0;
		double RealSeconds = 3.0;
		std::string Label;
		life::Outcome Result;
	};
	Skip TimeSkip;
	bool HasOutcome = false; // the result card is up
	life::Outcome LastOutcome;
	/** Starts an activity from life::Catalog(); returns why not, or "" when it started. */
	SHORTSTACKCORE_API std::string StartActivity(const std::string& Id);
	/** Heads out to a live game (life::Kind::Game, a buy-in from the bankroll) or a live tournament (life::Kind::Live,
	 * its fixed buy-in); returns why not, or "". */
	SHORTSTACKCORE_API std::string GoToGame(const std::string& Id, Chips BuyInCents);
	SHORTSTACKCORE_API bool PayRent();
	SHORTSTACKCORE_API bool PayDebt();
	SHORTSTACKCORE_API life::Context LifeContext() const;
	/** Formats unlocked so far (net::Unlock bits). */
	SHORTSTACKCORE_API int Unlocks() const;
	/** 0 at night, 1 by day, for the room's lighting. */
	SHORTSTACKCORE_API double Daylight() const;
	/** RiverLine has restricted the account (ghosting). */
	bool Restricted() const { return WorldMinutes() < Life.BannedUntil; }
	/** Tickets that pay for this listing (satellite seats). */
	int TicketsFor(const LobbyEvent& Ev) const;

	// Who is who in the tournament being played (player id -> country; regulars from the network).
	std::map<std::string, std::string> FieldCountry;
	std::set<std::string> FieldRegulars;
	std::set<std::string> FieldPros;
	// Bounties and seats in the tournament being played.
	std::map<std::string, Chips> Bounties; // player id -> bounty on their head (PKO)
	Chips BountyWon = 0;
	int Knockouts = 0;
	double LastBountyAt = -100.0;
	Chips LastBountyCents = 0;
	std::string LastBountyName;
	bool SeatWon = false;
	/** Satellites: seats the prize pool buys (0 otherwise). */
	int SeatsInPlay() const;

	int HeroSeatIdx() const;
	const TPlayer* PlayerById(const std::string& Id) const;

private:
	/** Everything that belongs to one table, for the tables not in front (see SwapActive). */
	struct TableRun
	{
		ss::Pace CurrentPace = ss::Pace::Smart;
		ss::Banner CurrentBanner;
		const LobbyEvent* Event = nullptr;
		LobbyEvent Joined;
		std::unique_ptr<Tournament> T;
		std::unique_ptr<Hand> CurHand;
		int TableId = 0;
		std::vector<SeatVis> Seats;
		int ButtonSeat = 0;
		std::vector<Card> Board;
		std::vector<double> BoardShownAt;
		Chips PotChips = 0;
		std::vector<Flight> Flights;
		std::vector<ChatLine> Chat;
		bool HasPrompt = false;
		HeroPrompt Prompt;
		std::vector<GradeBadge> Badges;
		std::vector<DecisionGrade> Grades;
		bool SitOutNext = false;
		bool SittingOutThisHand = false;
		bool Sprinting = false;
		std::string SprintStopReason;
		int HandsPlayed = 0;
		Chips BiggestPot = 0;
		double TimeBank = 30.0;
		bool Moving = false;
		int MovingFrom = 0;
		int MovingTo = 0;
		double MovingAt = 0.0;
		bool AutoFolded = false;
		std::vector<Card> AutoFoldedCards;
		double AutoFoldedAt = 0.0;
		double LastLevelUpAt = -100.0;
		bool HeroAllInReveal = false;
		std::map<std::string, std::string> FieldCountry;
		std::set<std::string> FieldRegulars;
		std::set<std::string> FieldPros;
		std::map<std::string, Chips> Bounties;
		Chips BountyWon = 0;
		int Knockouts = 0;
		double LastBountyAt = -100.0;
		Chips LastBountyCents = 0;
		std::string LastBountyName;
		bool SeatWon = false;
		size_t Cursor = 0;
		double NextAt = 0.0;
		bool BotPending = false;
		PlayerAction BotAction;
		double BotAt = 0.0;
		int BotSeatIdx = -1;
		bool HandDone = false;
		bool Revealed = false;
		bool HeroFolded = false;
		Chips PotBeforeAward = 0;
		double BustAt = 0.0;
		bool HasBustInfo = false;
		int BustPlace = 0;
		Chips BustPrize = 0;
		double LastIdleChat = 0.0;
		bool RivalArrived = false;
		int CansShown = 1;
		bool SprintBubble = false;
		bool SprintFinal = false;
		bool Closing = false;
	};
	/** Trades the front table's fields with Run's: checks the front table in, or Run's out. */
	void SwapActive(TableRun& Run);
	template <typename Src>
	static TableGlance GlanceOf(const Src& From);
	void TableStep();
	void CloseFinishedTables();
	void AutoFocusStep();
	void Sound(SoundId Id, double Volume);
	void Heartbeat(bool On);

	std::vector<std::unique_ptr<TableRun>> Runs;
	int Active = -1;
	bool Background = false; // a table not in front is taking its turn: quieter, no heartbeat
	int BackgroundHeavy = 0; // heavy steps the tables behind may still take this frame
	bool Closing = false;    // the front table finished while others play on; Update closes it
	double FocusHoldUntil = 0.0;
	Chips SessionStartBankroll = 0;
	int SessionEvents = 0;

	void StoryText(const std::string& Key, const std::string& From, const std::string& Body, bool Once = true);
	void Push(const std::string& Who, const std::string& Text, ChatKind Kind);
	std::string SeatName(int Seat) const;
	void BuildSeats();
	double Speed() const;
	bool ShouldAutoFold();
	void OpenHeroTurn();
	double Consume(const HandEvent& Ev);
	bool CollectBets();
	void UpdateEquities();
	void AfterAward();
	void EndHand();
	void HandleTourneyEvents(const std::vector<TEvent>& Events);
	void SprintStep();
	void ShowResults();
	void HandleKnockout(const TEvent& E);
	void NameField();
	bool CheckSatellite();
	void CheckUnlocks();
	void CheckCalendar(double From, double To, bool Awake);
	void PayNightShift(double End);
	void RentDeadline();
	void FinishSkip();
	void BustBanner(const TPlayer& Hero, const char* NoCashSub);

	SessionHooks& Hooks;
	std::set<std::string> TextsSeen;
	std::string SeedBase;
	int RegisterCount = 0;
	double LastTick = -1.0;
	double CalendarAt = -1.0;
	Rng R;
	Rng LifeRng;
	size_t Cursor = 0;
	double NextAt = 0.0;
	bool BotPending = false;
	PlayerAction BotAction;
	double BotAt = 0.0;
	int BotSeatIdx = -1;
	bool HandDone = false;
	bool Revealed = false;
	bool HeroFolded = false;
	Chips PotBeforeAward = 0;
	double BustAt = 0.0;
	bool HasBustInfo = false;
	int BustPlace = 0;
	Chips BustPrize = 0;
	double LastIdleChat = 0.0;
	bool RivalArrived = false;
	int CansShown = 1;
	bool SprintBubble = false;
	bool SprintFinal = false;
};

const char* SoundName(SoundId Id);
} // namespace ss
