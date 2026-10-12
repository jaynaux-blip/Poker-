#pragma once

#include "ShortStack/AI/Grading.h"
#include "ShortStack/Game/Badges.h"
#include "ShortStack/Game/Chat.h"
#include "ShortStack/Game/Gear.h"
#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Lobby.h"
#include "ShortStack/Game/Rep.h"
#include "ShortStack/Game/Store.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Rng.h"
#include "ShortStack/Tournament.h"

#include <functional>
#include <map>
#include <memory>
#include <set>

namespace ss
{
namespace town
{
struct Talk; // Talk.h
}

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
	Stream, // Kast chat, while live
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
	Mail, // a letter in the Mail app (next_segment_spec §3.9; audio-synth renders it)
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
	/** The one badge the seat shows on the network (§5, rule C1): badge::Top in its Network scope; K == Count for none. */
	badge::Badge Top;
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

/** What the player's tables saw of someone at a live tournament (SaveData::NoteLive). */
struct LiveSeen
{
	/** P busted before the player (Value: their place), S still in when the player finished, M sat with the player,
	 * K knocked the player out, H knocked out by the player, B a big pot with the player (Value: the player's net). */
	char Tag = 'M';
	std::string Name; // as the field knows them
	long long Value = 0;
};

/** Everything that persists between runs. */
struct SaveData
{
	Chips BankrollCents = 237;
	std::string HeroName = "grinder_3c";
	/** The person behind the screen name (the character creator's; "hero" lines). */
	hero::Character Person;
	std::vector<HistoryEntry> History;
	std::vector<std::string> TextsSeen;
	double ClockMinutes = 2.0 * 60.0 + 7.0; // the lobby clock
	life::State Life;
	gear::Owned Gear;
	gear::LedState Leds;
	kast::Channel Channel;
	/** The living world (its own "world" lines, passed through untouched by anything that edits a save). */
	std::string WorldText;
	/**
	 * For a host that saves in the background (SessionHooks::SavesInBackground): the world as it was at the save,
	 * frozen, for the host to write into WorldText off the game thread (WorldText is empty then). The same snapshot
	 * comes back on every save until the world moves on, so its text need only be written once.
	 */
	std::shared_ptr<const world::World> WorldSnapshot;
	/** What happened away from the desk for the world to hear about (written by the host, read by the next session). */
	std::vector<std::string> WorldNotes;

	/** Line-based text, safe to store in any save system. */
	SHORTSTACKCORE_API std::string Serialize() const;
	static SHORTSTACKCORE_API bool Parse(const std::string& Text, SaveData& Out);
	/** A night at Dee's game with these people (as the room knows them), and how it went. */
	SHORTSTACKCORE_API void NoteBackRoom(double World, const std::vector<std::string>& Names, Chips NetCents);
	/** A Embercrest Sunday: where the player and the people with faces finished (saves from before the daily schedule). */
	SHORTSTACKCORE_API void NoteEmbercrest(double World, int HeroPlace, int Field, const std::vector<std::pair<std::string, int>>& Places);
	/**
	 * A live tournament the host played: the player's finish and what their tables saw, for the world. The entry keeps
	 * who sat with the player and what passed between them (life::LiveEntry::Table): what they text about afterwards.
	 */
	SHORTSTACKCORE_API void NoteLive(double World, const std::string& EventId, int HeroPlace, Chips Prize, int Field, const std::vector<LiveSeen>& Seen);
	/**
	 * What the player saw of the rest of a live tournament after their own finish (railing it): who went out where, the
	 * winner too (P tags only). The world's result agrees with it. World: the player's own finish (the same as
	 * NoteLive's), so the places are in before the world resolves the event.
	 */
	SHORTSTACKCORE_API void NoteLivePlaces(double World, const std::string& EventId, const std::vector<LiveSeen>& Places);
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
	/**
	 * True when the host writes saves off the game thread: the session then hands it a frozen copy of the world
	 * (SaveData::WorldSnapshot, a few milliseconds) instead of the world's text (tens of milliseconds to write).
	 */
	virtual bool SavesInBackground() const { return false; }
	/** Leaves the apartment for a place the host plays out itself (Dee's game). False when it can't. */
	virtual bool GoOut(const std::string& /*ActivityId*/, Chips /*BuyInCents*/) { return false; }
	/** Something from GearDrop arrived (or a subscription ended): the desk can show it (a second monitor, a ring light). */
	virtual void GearChanged(const std::string& /*ItemId*/, bool /*Owned*/) {}
	/** The stream went live or ended (an ON AIR light, the ring light coming on). */
	virtual void OnAir(bool /*Live*/) {}
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
	SHORTSTACKCORE_API ~Session();
	Session(const Session&) = delete;
	Session& operator=(const Session&) = delete;

	// ------------------------------------------------------------ persistent
	Chips BankrollCents = 237;
	std::string HeroName = "grinder_3c";
	std::vector<HistoryEntry> History;
	/** Who the player made in the character creator (defaults, not Created, for a career from before it). */
	hero::Character Person;

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
	// The Lucky Penny #212 (the corner store): pay at the counter, then eat and drink from the bag.
	/** Pays for the basket from the bankroll and puts it in the bag; "" or why not ("Card declined."). */
	SHORTSTACKCORE_API std::string Checkout(const store::Basket& B);
	/** Eats or drinks one of an item in the bag; "" or why not. LastEaten says what it did. */
	SHORTSTACKCORE_API std::string Consume(const std::string& ItemId);
	/** The last thing eaten or drunk: the item, what each meter did (hunger and thirst negative: relief), and a line for a toast. */
	struct Eaten
	{
		std::string ItemId;
		double Hunger = 0.0;
		double Thirst = 0.0;
		double Energy = 0.0;
		std::string Line; // "Drank the Cascade. Thirst -45, energy +2."
	};
	Eaten LastEaten;
	/**
	 * What to reach for in the bag: what does the most for the worst need without wasting much or making another
	 * worse (an id). "" when the bag is empty, nothing is worth it now (fed, watered, awake), or all of it would hurt.
	 */
	SHORTSTACKCORE_API std::string BagPick() const;
	/**
	 * The eat key: BagPick's choice, eaten or drunk (LastEaten.Line for the toast). "" or why not, in a line for the toast:
	 * an empty bag, "You're not hungry or thirsty." (or just tired), "Nothing in here for that thirst.", "Not hungry enough
	 * for the Deli Wedge yet.", "The Hilltop chips would only make you thirstier."
	 */
	SHORTSTACKCORE_API std::string EatFromBag();
	/** What the clerk says right now. */
	SHORTSTACKCORE_API std::string ClerkSays(const store::Basket& B) const;
	/**
	 * Penny Drop (the laptop): orders the basket to the door at the app's prices, plus the fee and tax; it
	 * arrives 25 to 45 minutes later (the clock brings it, into the bag). "" or why not.
	 */
	SHORTSTACKCORE_API std::string PlaceOrder(const store::Basket& B);
	/** Where Penny Drop brings an order: the apartment, or Dee's place while the locks are changed. */
	SHORTSTACKCORE_API std::string DeliveryAddress() const;
	/** A glass of water from the kitchen tap (at home): free, a little at a time, and never past TapFloor. "" or why not. */
	SHORTSTACKCORE_API std::string DrinkTapWater();
	static constexpr double TapFloor = 40.0;
	/** Minutes until the tap is worth another glass (0: now). */
	SHORTSTACKCORE_API double TapWait() const;
	/** A new career for this character: a fresh save, then what their background starts them with. */
	SHORTSTACKCORE_API void NewCareer(const hero::Character& Who);
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
	/**
	 * Back from somewhere the host played out (Dee's game, the Embercrest, the street): calendar events since World
	 * still happen (the needs climb, the bills come due, orders arrive). Every host keeps the player's energy itself
	 * while they're there (bEnergyKept), so the hours away aren't charged to it a second time.
	 */
	void ResumeCalendarFrom(double World, bool bEnergyKept = true)
	{
		CalendarAt = World;
		EnergyKeptUntil = bEnergyKept ? WorldMinutes() : -1.0;
	}
	/**
	 * A host away from the apartment's door (the street) sets this: Penny Drop orders that come due wait at the door
	 * (they stay in Life.Deliveries, and are saved) for the first calendar check back home.
	 */
	bool DeferDeliveries = false;
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
		// The bed it began in: the locks changing in the middle of a night's sleep don't make it the couch's.
		double Rest = 1.0;
		bool Couch = false;
	};
	Skip TimeSkip;
	bool HasOutcome = false; // the result card is up
	life::Outcome LastOutcome;
	/** Starts an activity from life::Catalog(); returns why not, or "" when it started. */
	SHORTSTACKCORE_API std::string StartActivity(const std::string& Id);
	/** Heads out to a live game (life::Kind::Game, a buy-in from the bankroll) or a live tournament (life::Kind::Live,
	 * its fixed buy-in); returns why not, or "". */
	SHORTSTACKCORE_API std::string GoToGame(const std::string& Id, Chips BuyInCents);
	/**
	 * Registers for one of the Embercrest's events (live::Occurrence id; "" for the next one open) with the field the
	 * world has for it, pays the bus and heads out. Registering again for an event already entered just goes back.
	 */
	SHORTSTACKCORE_API std::string GoToLive(const std::string& OccurrenceId);
	/** The live tournament the player is in the middle of (the game closed during it): its occurrence id, or empty. */
	SHORTSTACKCORE_API std::string LiveInProgress() const;
	/** Back to that tournament's room, straight to the seat (no bus, nothing dealt without them while the game was closed). */
	SHORTSTACKCORE_API std::string ResumeLive();
	/** Pays what's due now (or, paid up, next month early). Late payment doesn't move the next month's due date. */
	SHORTSTACKCORE_API bool PayRent();
	/** The locks changed (life::Rent::Evicted): on Dee's couch, the gear in storage, no stream. */
	bool Evicted() const { return Life.RentStage == life::Rent::Evicted; }
	/** Evicted: pays the back rent and a month up front (Life.RentDueCents), gets the key and the gear back. "" or why not. */
	SHORTSTACKCORE_API std::string MoveBackIn();
	/** Evicted with gear in storage: pays the unit's next month ahead (no more than one month ahead). "" or why not. */
	SHORTSTACKCORE_API std::string PayStorage();
	/** What a night's sleep gives back, as a multiple (a better bed and dark curtains add; Dee's couch takes away). */
	SHORTSTACKCORE_API double RestFactor() const;
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

	// ------------------------------------------------------------ gear and streaming (Gear.h, Kast.h)
	/** Everything bought on GearDrop (id -> when a subscription renews; 0 for things owned outright). */
	gear::Owned Gear;
	/** The Kast channel (persists) and the stream (live state). */
	kast::Channel Channel;
	kast::Stream Stream;
	/** What the owned gear adds up to, with fitness's drain and calm folded in (body::EffectsOf, RefreshGear). */
	const gear::Effects& GearFx() const { return Fx; }
	bool Owns(const std::string& Id) const { return Gear.count(Id) > 0; }
	/** Why an item can't be bought now ("" when it can). */
	SHORTSTACKCORE_API std::string CanBuy(const std::string& Id) const;
	/** Buys from GearDrop (the first month, for subscriptions); returns why not, or "". */
	SHORTSTACKCORE_API std::string Buy(const std::string& Id);
	/** Ends a subscription (no refund; it stops now). */
	SHORTSTACKCORE_API bool Cancel(const std::string& Id);
	/** The LED room kit: its colour (gear::LedPresets), on or off, and whether it flashes with the stream. */
	gear::LedState Leds;
	SHORTSTACKCORE_API void SetLedPreset(int Preset);
	SHORTSTACKCORE_API void SetLedsOn(bool On);
	SHORTSTACKCORE_API void SetLedSync(bool Sync);
	/** What the LEDs show at Now (the time passed to Update): the room's lights and the facecam follow it. */
	SHORTSTACKCORE_API gear::Glow RoomGlow(double Now) const;
	/** Goes live on Kast; returns why not, or "". */
	SHORTSTACKCORE_API std::string GoLive();
	/** Ends the stream: the channel's balance goes to the bank and the summary card comes up. Raid: a small channel
	 * (kast::Network) to send the viewers to on the way out; it raids back on another night. */
	SHORTSTACKCORE_API void EndStream(const std::string& Raid = std::string());
	/** The posted schedule: weekdays (bit 0 Monday, 0 for none) and the start, minutes after midnight. Regulars come
	 * when they know when; missing a posted day costs loyalty. */
	SHORTSTACKCORE_API void StreamSchedule(int Weekdays, int StartMinute);
	bool Streaming() const { return Stream.Live; }
	/** The summary card of the stream that just ended is up. */
	bool StreamCard = false;
	/** The stream's view of the game right now. */
	SHORTSTACKCORE_API kast::Inputs StreamInputs() const;
	/** Studio actions (see kast::Stream). */
	SHORTSTACKCORE_API bool StreamAd(int Seconds);
	SHORTSTACKCORE_API bool StreamThank();
	SHORTSTACKCORE_API bool StreamAnswer(int MsgId);
	SHORTSTACKCORE_API bool StreamTimeout(int MsgId);
	SHORTSTACKCORE_API bool StreamPromote(const std::string& Name);
	SHORTSTACKCORE_API bool StreamDemote(const std::string& Name);
	SHORTSTACKCORE_API Chips StreamRead(const std::string& SponsorId);
	SHORTSTACKCORE_API void StreamTitle(int Title);
	/** Sponsor offers. Accepting takes one of the overlay's slots (kast::MaxDeals). */
	SHORTSTACKCORE_API std::string AcceptDeal(const std::string& Id);
	SHORTSTACKCORE_API void DeclineDeal(const std::string& Id);
	/** Sends the channel's balance to the bank; returns the amount. */
	SHORTSTACKCORE_API Chips CashOut();
	// ------------------------------------------------------------ the living world (World.h)
	/** Everyone else's careers, going on whether the player is watching or not. */
	world::World& Living() { return LivingWorld; }
	const world::World& Living() const { return LivingWorld; }
	/** Debug: how the world would look Days from now (a copy is played forward; nothing changes). */
	SHORTSTACKCORE_API std::string WorldPreview(int DayCount) const;
	/** Debug: the player sleeps through Days (the clock and the world move on; nothing else happens to the player). */
	SHORTSTACKCORE_API void WorldSkip(int DayCount);

	// ------------------------------------------------------------ Northside (the realism update's spec, §7.2)
	// The shops, the pawn and the people on the block. The calls live in SessionShops.cpp and SessionTown.cpp; each
	// "" or why not, like the rest of the session's. Money goes through the ledger as kind 9.
	// ---- Northside: shops (SessionShops.cpp) ----
	struct ShopOrder
	{
		std::string Venue;
		std::vector<std::string> Offers;
		std::vector<std::string> Garments;
		int TipPercent = 0;
	};
	SHORTSTACKCORE_API bool ShopOpenFor(const std::string& Venue) const;          // open now, or a member at the gym
	SHORTSTACKCORE_API std::string ShopBlocked(const std::string& OfferId) const; // "" when it can be had now
	SHORTSTACKCORE_API Chips ShopTotal(const ShopOrder& O) const;                 // tax where it applies, then the tip
	SHORTSTACKCORE_API std::string ShopPay(const ShopOrder& O);     // goods, passes, the membership, shakes: charged now
	SHORTSTACKCORE_API std::string ShopService(const ShopOrder& O); // cuts, workouts, meals: charged, then a TimeSkip "shop:<venue>:<ids>"
	SHORTSTACKCORE_API std::string WaitAt(const std::string& Venue, double UntilWorld); // sit a while, the bench, the shelter
	SHORTSTACKCORE_API std::string Wear(const std::string& GarmentId);
	SHORTSTACKCORE_API std::string TakeOff(int WardrobeSlot);
	SHORTSTACKCORE_API std::string Donate(const std::string& GarmentId);
	SHORTSTACKCORE_API std::string BoardBus(const std::string& LiveId); // the 14 at Market St (§7.6)
	SHORTSTACKCORE_API uint64_t WorldSeed() const;
	SHORTSTACKCORE_API double Fitness() const;
	SHORTSTACKCORE_API double Sharp() const;
	// ---- Northside: the pawn (SessionShops.cpp, §5.8) ----
	SHORTSTACKCORE_API std::vector<shop::Lot> PawnLots() const;        // what the hero could bring Augie now (stored gear while the unit's paid)
	SHORTSTACKCORE_API std::vector<shop::Lot> PawnShelf() const;       // for sale now: used gear, curios, forfeits, auction lots
	SHORTSTACKCORE_API std::string PawnSell(const std::string& LotId); // 50% of V, cash now (ledger 9); stored gear: a 60-minute skip first
	SHORTSTACKCORE_API std::string PawnLoan(const std::string& LotId); // 40% of V now, and a 30-day ticket
	SHORTSTACKCORE_API std::string PawnRedeem(int Ticket);
	SHORTSTACKCORE_API std::string PawnRenew(int Ticket);              // P1
	SHORTSTACKCORE_API std::string PawnBuy(const std::string& LotId);  // taxed; gear while evicted only if it needs no room
	SHORTSTACKCORE_API bool PawnHolds(const std::string& LotId) const; // in his cage, his case or gone: the apartment hides the cup, the trophy case stamps an award
	// ---- Northside: people (SessionTown.cpp) ----
	SHORTSTACKCORE_API void Heard(const std::string& Speaker, const town::Talk& Told); // said keys, familiarity, WorldNotes, MemoryKind::Street (not "T": that's the tournament)
	SHORTSTACKCORE_API std::string Lend(const std::string& Speaker, Chips Cents);  // Twitch's twenty
	SHORTSTACKCORE_API void Tipped(const std::string& Speaker, int Percent);

	// ------------------------------------------------------------ the next segment (next_segment_spec §7.2)
	// Backing, sponsors and badges. Declared in Phase A; the bodies are the feature tracks' (SessionStake.cpp,
	// SessionSponsor.cpp, SessionBadges.cpp, the precedent SessionShops.cpp), stubs that keep today's behavior until they
	// land. Each "" or why not, like the rest of the session's.
	/** The public record (§1.1): rep::Of on the life, the channel and the world, now. */
	SHORTSTACKCORE_API rep::Public Record() const;
	// ---- Backing (§2: SessionStake.cpp)
	SHORTSTACKCORE_API std::string AcceptStake(const std::string& OfferId);
	SHORTSTACKCORE_API std::string CounterStake(const std::string& OfferId, int Lever);
	/** Pieces of an event for sale: EventId is the occurrence or the online event, never the entry (§1.2). */
	SHORTSTACKCORE_API std::string ListAction(const std::string& EventId, int Permille, int MarkupPermille);
	SHORTSTACKCORE_API std::string CancelListing(const std::string& EventId);
	SHORTSTACKCORE_API std::string ProposeSwap(const std::string& EntryId, const std::string& Partner, int Permille);
	SHORTSTACKCORE_API std::string PayOwed(const std::string& OwedId);
	SHORTSTACKCORE_API std::string RequestChop(const std::string& DealId);
	SHORTSTACKCORE_API std::string WalkAway(const std::string& DealId);
	SHORTSTACKCORE_API std::string BookCoach(const std::string& ActivityId);
	/** Who could pay for an event (an occurrence or an online event id): each option, and why not ("" when it can). */
	SHORTSTACKCORE_API std::vector<std::pair<life::Funding, std::string>> FundingOptions(const std::string& EventOrOccurrenceId) const;
	// ---- Sponsors (§3: SessionSponsor.cpp). AcceptDeal and DeclineDeal (Kast's offers, above) forward here.
	SHORTSTACKCORE_API std::string SignDeal(const std::string& ContractId);
	SHORTSTACKCORE_API std::string CounterDeal(const std::string& ContractId, int Lever);
	// (DeclineDeal(ContractId) is the one declared with Kast's offers above.)
	SHORTSTACKCORE_API std::string GiveNotice(const std::string& ContractId);
	SHORTSTACKCORE_API std::string AskForRaise(const std::string& ContractId);
	SHORTSTACKCORE_API std::string SewPatch(int Spot, const std::string& BrandId);
	SHORTSTACKCORE_API std::string Appear(const std::string& Venue);
	SHORTSTACKCORE_API int SponsorTier() const;
	/** On Team RiverLine: the patch on the hero's avatar. It was the Kast deal (Session.h:642 at c09bc31); now the book's. */
	SHORTSTACKCORE_API bool TeamRiverLine() const;
	// ---- Badges (§5: SessionBadges.cpp)
	SHORTSTACKCORE_API const badge::Set& HeroBadges() const;
	/**
	 * Badges earned that the player hasn't been shown yet (each once: text\tbadge:<kind>:<tier>:<count>). It consumes: one
	 * caller (RiverLine, once a frame) toasts them; the call plays the toast's SoundId::Alert and, on air, gives Kast's chat
	 * its one reaction to each (badge::ChatReaction).
	 */
	SHORTSTACKCORE_API std::vector<badge::Badge> Unseen();
	SHORTSTACKCORE_API std::vector<badge::Progress> BadgeProgress(int Max) const;
	// ---- The calendar's steps (CheckCalendar runs them in this order, beside ShopStep) and debug (game/SegmentCommands.cpp)
	SHORTSTACKCORE_API void SponsorStep(double From, double To);
	SHORTSTACKCORE_API void StakeStep(double From, double To);
	SHORTSTACKCORE_API void BadgeStep(double From, double To);
	SHORTSTACKCORE_API std::string DebugStake(const std::string& Args);
	SHORTSTACKCORE_API std::string DebugSponsor(const std::string& Args);
	SHORTSTACKCORE_API std::string DebugBadge(const std::string& Args);
	/** The dates the second screen and the wall calendar show (§1.3): every feature's marks from From to To, merged in time order. */
	SHORTSTACKCORE_API std::vector<life::Mark> Marks(double From, double To) const;

	// Who is who in the tournament being played (player id -> country; regulars from the network).
	std::map<std::string, std::string> FieldCountry;
	std::set<std::string> FieldRegulars;
	std::set<std::string> FieldPros;
	// The living world's people in the tournament being played: player id -> who, who sat with the player, who said hello.
	std::map<std::string, int> FieldNpc;
	std::set<int> FieldMet;
	std::set<int> FieldGreeted;
	std::vector<std::pair<int, Chips>> FieldPots;
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
		std::map<std::string, int> FieldNpc;
		std::set<int> FieldMet;
		std::set<int> FieldGreeted;
		std::vector<std::pair<int, Chips>> FieldPots;
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
	// The living world: started or loaded with the session, saved with it, played forward with the clock.
	world::World LivingWorld;
	double WorldNewsAt = 0.0; // world events up to here have been considered for the phone
	// The world's save text, rewritten when something involving the player happened or an hour has passed (writing
	// a couple of megabytes on every save would hitch).
	std::string WorldSaved;
	std::shared_ptr<const world::World> WorldSnap; // the same, frozen, for a host that saves in the background
	double WorldSavedAt = -1.0e9;
	int WorldSavedRev = -1;
	void StartWorld(const SaveData* Loaded);
	void WorldStep();
	void WorldNote(const std::vector<std::string>& Fields);
	void ReportToWorld();
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
	/** Live entries left registered after their event ended (the game closed mid-tournament) are resolved. */
	void ResolveAbandonedLive();
	/**
	 * Live events short of their minimum field (R6): an entry the player isn't in the room for is refunded once the cards
	 * should have gone in the air, and the world records every such event as called off (one the room refunded too).
	 */
	void SettleCancelled();
	/** The desk's word on an entry the session refunded (the player wasn't there to hear the floor). */
	void CancelledTexts();
	/** After a night at the Embercrest, the people the player sat with text over the next day or two (From..To crossed). */
	void BondTexts(double From, double To);
	bool CheckSatellite();
	void CheckUnlocks();
	void CheckCalendar(double From, double To, bool Awake);
	/** Penny Drop orders due by Until: into the bag, in the order they came, a text each. */
	void DeliverOrders(double Until);
	/** How much harder a bad beat lands on an empty stomach (starving or parched). */
	double NeedsTilt() const;
	void PayNightShift(double End);
	void RentDeadline();
	void SettleRent();
	void Evict();
	void Auction();
	void FinishSkip();
	void BustBanner(const TPlayer& Hero, const char* NoCashSub);
	void RefreshGear();
	void RenewGear(double From, double To);
	void StreamStep();
	void StreamMoment(kast::Moment M, const std::string& Detail = std::string(), double Size = 1.0);
	void PlayStreamNotices();
	void PayOut(const std::string& Label);
	// Northside's hooks in Session.cpp, bodies in SessionShops.cpp (§7.2).
	void SettleShop();                     // FinishSkip's hook for "shop:" activity ids: the look, the fitness, the meal's relief, the outcome line
	void ShopStep(double From, double To); // CheckCalendar's hook: fitness decay; the pawn's reminders, forfeits and shelf expiry
	/** A visit paid for at a venue (the Lucky Penny's counter included): Life.Visits[Venue]'s count, time and spend. */
	void ShopVisit(const std::string& Venue, Chips SpentCents);
	/** ClerkSays' hook: what the counter knows of the player from their visits (and, later, the clerk's bond). */
	void ShopClerk(store::ClerkContext& Ctx) const;
	/** StreamInputs' hook: the look on camera (Sharp and Style) in the stream's follow and tip multipliers. */
	void ShopStream(kast::Inputs& In) const;
	// The next segment's seams in Session.cpp (Phase A, §7.2), where a hook site needs more than a core call: the bodies
	// are the tracks' (stubs that keep today's behavior until they land).
	/** RegisterEvent's hook, after the buy-in (none when a package's roll paid): the roll's line, the online fund, a listing's pieces (SessionStake.cpp). */
	void StakeEntered(const std::string& EventId, Chips BuyInCents, const life::Funding& F);
	/** WorldStep's hook: the book's flows to the world (World::HeroMoney), swaps settled through World::FinishIn (SessionStake.cpp). */
	void StakeWorld();
	/** Conduct's hook: what the stake record keeps of a pickup or a restriction (SessionStake.cpp). */
	void StakeConduct(sponsor::Breach Why);
	/** FinishSkip's hook: a coaching session (life::Kind::Study, paid coaching: P2) ends (SessionStake.cpp). */
	void StudyDone(const life::Activity& A, const life::Outcome& O);
	/** StreamInputs' hook: the deals Kast works on (sponsor::StreamView) (SessionSponsor.cpp). */
	void SponsorStream(kast::Inputs& In) const;
	/** After Kast's step, a read, the offline turnover and the stream's end: what it did with the deals (sponsor::AbsorbStream) (SessionSponsor.cpp). */
	void SponsorStreamDone();
	/** RenewGear's hook: a subscription the sponsor pays for this period (its renewal is skipped) (SessionSponsor.cpp). */
	bool SponsorCovers(const std::string& GearId) const;
	/** A pickup (++Life.Busts) or a restriction (++Life.Bans): sponsor::Conduct, then the stake record (Session.cpp). */
	void Conduct(sponsor::Breach Why, double World);
	/** The player's badges (HeroBadges; SessionBadges.cpp keeps them). */
	badge::Set HeroSet;

	SessionHooks& Hooks;
	std::set<std::string> TextsSeen;
	std::string SeedBase;
	int RegisterCount = 0;
	double LastTick = -1.0;
	double CalendarAt = -1.0;
	/** A host that played the night out kept the energy up to here (ResumeCalendarFrom); the calendar drains it after. */
	double EnergyKeptUntil = -1.0;
	/** When the card last said no at the Lucky Penny's counter (world minutes, -1: it hasn't; not saved). */
	double DeclinedAt = -1.0;
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
	gear::Effects Fx;
	/** Fitness's effects (body::EffectsOf, refreshed with the gear): its drain and calm are folded into Fx; its rest bonus
	 * and exhausted threshold are read where sleep and the tables need them. */
	body::Effects BodyFx;
	double StreamWorldAt = -1.0;
	double StreamRealAt = -1.0;
	double StreamSoundAt = -100.0;
	double OfflineAt = -1.0;
	Rng KastRng;
};

const char* SoundName(SoundId Id);
} // namespace ss
