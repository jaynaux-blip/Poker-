#pragma once

#include "ShortStack/AI/Grading.h"
#include "ShortStack/Game/Chat.h"
#include "ShortStack/Game/Lobby.h"
#include "ShortStack/Rng.h"
#include "ShortStack/Tournament.h"

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
};

/** Everything that persists between runs. */
struct SaveData
{
	Chips BankrollCents = 237;
	std::string HeroName = "grinder_3c";
	std::vector<HistoryEntry> History;
	std::vector<std::string> TextsSeen;

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
	const LobbyEvent* Event = nullptr;
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
	void Register(int Index);

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
	/** Tournament clock (drives the dawn outside), or 2:07 AM in the lobby. */
	SHORTSTACKCORE_API double ClockMinutes() const;

	int HeroSeatIdx() const;
	const TPlayer* PlayerById(const std::string& Id) const;

private:
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
	void BustBanner(const TPlayer& Hero, const char* NoCashSub);

	SessionHooks& Hooks;
	std::set<std::string> TextsSeen;
	std::string SeedBase;
	int RegisterCount = 0;
	Rng R;
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
