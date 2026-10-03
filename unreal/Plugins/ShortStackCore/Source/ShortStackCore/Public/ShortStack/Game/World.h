#pragma once

#include "ShortStack/Game/Network.h"

#include <array>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace ss
{
/**
 * The living poker world: everyone the player can meet or read about has a career that goes on without them.
 *
 * net::Network stays the calendar (the schedule, registrations, prize pools, listings). The World is the people
 * and what happens to them. Each save has its own World (its own seed and history): on Night One it holds the
 * network's 1,600 regulars, the Back Room and Riverside regulars, Kast's big streamers and the rival, and from
 * then on they choose events by bankroll, form and schedule, win and lose, move up and down, take breaks, go
 * broke, retire and come back, while new players join every week.
 *
 * Fidelity:
 * - High: the events the player plays. The player's own tables are dealt hand by hand; the World seats the
 *   people it registered there and takes their real finishing places back afterwards.
 * - Medium: majors (series main events, the Championship, the Summit, ring and bracelet events). Final tables
 *   are played out between the people at them (heads-up battles and eliminations become rivalries and news).
 * - Light: every other event. Each tracked entrant's finish is drawn from a skill-weighted distribution
 *   against the field's strength; nobody else in the field is simulated.
 * - Long: days with nobody to watch cost one planning pass and one resolution per event; weeks, months and
 *   seasons are maintenance passes (stakes, form, reputations, retirements, new players).
 *
 * The cards are never touched. Form, confidence and fatigue change what people choose to play, how much,
 * and how well they decide (tilt and burnout cost them focus), never the luck they get.
 */
namespace world
{
constexpr int Version = 1;

// ------------------------------------------------------------------ who they are

/** What a player is good at (0..1). Two players with the same average still play very differently. */
enum class Skill : int
{
	Preflop,
	Postflop,
	Aggression, // applying pressure well
	Bluffing,
	Value,
	Icm,
	ShortStack,
	DeepStack,
	HeadsUp,
	Bounty,
	Satellite,
	Live, // reads, tells, comfort at a real table
	Adjust, // exploiting what they see
	Emotion, // control after a bad beat
	Count,
};
constexpr int SkillCount = static_cast<int>(Skill::Count);
SHORTSTACKCORE_API const char* SkillName(Skill S);

/** Personality (0..1): how they behave, not how well. */
enum class Trait : int
{
	Discipline, // bankroll management
	Ambition,   // moving up, studying, big events
	Risk,       // shots, re-entries, gambling
	Patience,
	Sociability, // streaming, friends, table talk
	Grit,        // grinding through a downswing instead of quitting
	Aggro,       // style: loose and aggressive (1) or tight and passive (0)
	Count,
};
constexpr int TraitCount = static_cast<int>(Trait::Count);

/** The label the world would put on them right now. Not a class: it is worked out again every month. */
enum class Identity : int
{
	OnlineGrinder,
	LiveRegular,
	Recreational,
	YoungCrusher,
	DisciplinedPro,
	SatelliteSpecialist,
	BountySpecialist,
	Streamer,
	HighStakesGambler,
	BankrollNit,
	WealthyAmateur,
	UndergroundSpecialist,
	Veteran,
	RisingProspect,
	Count,
};
SHORTSTACKCORE_API const char* IdentityName(Identity I);

enum class Status : int
{
	Active,
	Break,   // back later (weeks to months)
	Broke,   // out of money: rebuilding, looking for a stake, or gone
	Retired, // may still come back for the big one
};
SHORTSTACKCORE_API const char* StatusName(Status S);

enum class Momentum : int
{
	Normal,
	Heater,
	Downswing,
	MajorDownswing,
	Breakout,
	Rebuilding,
	Confident, // after a title
	Burnout,
	Count,
};
SHORTSTACKCORE_API const char* MomentumName(Momentum M);

enum class Venue : int
{
	Online,
	Live,
	Underground,
	Count,
};
constexpr int VenueCount = static_cast<int>(Venue::Count);

enum class Rep : int
{
	Online,
	Live,
	Underground,
	Streaming,
	HighStakes,
	Overall,
	Count,
};
constexpr int RepCount = static_cast<int>(Rep::Count);
SHORTSTACKCORE_API const char* RepName(Rep R);

enum class Origin : int
{
	Founding,   // the network's regulars on Night One
	Cast,       // the Back Room and the Riverside
	Directory,  // Kast's established streamers
	Rookie,     // joined later
	Discovered, // an unknown who made a name with one result
};

/** How a newcomer found their way to RiverLine (the first line of their story). */
enum class Arrival : int
{
	None,          // they were here before the story began
	FirstTimer,    // downloaded it one night and stayed
	CameOfAge,     // old enough at last, registered that night
	SiteClosed,    // their old site shut down
	LiveCrossover, // a live regular trying the online grind
	Comeback,      // played years ago, back after a long time away
	HomeGame,      // talked into it by a home game's regulars
	Watched,       // signed up after watching someone (a stream, a Main Event)
	Streamer,      // started a Kast channel on day one
	Count,
};
SHORTSTACKCORE_API const char* ArrivalName(Arrival A);

/** When they play. */
enum class Plan : int
{
	Daily,      // a grinder: most days, many tables
	Nights,     // after work, weeknights
	Weekends,
	Occasional, // a night here and there
	SeriesFocus, // light all year, everything during a series
	MajorsOnly,  // Sundays and the big events
	LiveCircuit, // travels for live series
	Count,
};
SHORTSTACKCORE_API const char* PlanName(Plan P);

enum class Region : int
{
	Americas,
	Europe,
	AsiaPacific,
	Count,
};
SHORTSTACKCORE_API Region RegionOf(const std::string& Country);

/** Formats a player likes (bits). */
enum Likes : int
{
	LikeBounty = 1,
	LikeSatellite = 2,
	LikeTurbo = 4,
	LikeDeep = 8,
	LikeOmaha = 16,
	LikeMystery = 32,
};

/** Live poker's ladder (a player's comfort level, and an event's level). */
enum class LiveLevel : int
{
	None,
	Local,        // the weekly $150 at the casino
	Regional,     // a regional series, $550 mains
	Circuit,      // tour stops, $1,700 to $5,300 mains, rings
	Championship, // the summer Championship, $10,000 Main Event, bracelets
	HighRoller,   // $25,000 and up
};

// ------------------------------------------------------------------ what they've done

struct Ledger
{
	Chips Won = 0;   // prizes (and bounties)
	Chips Spent = 0; // buy-ins
	int Events = 0;
	int Cashes = 0;
	int FinalTables = 0;
	int Wins = 0;
	Chips Best = 0;
};

struct Season
{
	int Year = 2026;
	double Points = 0.0; // online Player of the Year points
	Chips Won = 0;       // online
	int Wins = 0;
	int FinalTables = 0;
	int Cashes = 0;
	double LivePoints = 0.0;
	Chips LiveWon = 0;
};

/** A result worth remembering (the player card's recent results). */
struct Finish
{
	int Day = 0;
	std::string Event;
	int Place = 0;
	int Entries = 0;
	Chips Prize = 0;
	int Where = 0; // Venue
	bool Major = false;
	Chips Seat = 0; // a satellite seat won (its value)
};

/** A year of a career (the career graph and the profile's years). */
struct Year
{
	int Number = 2026;
	Chips Online = 0; // won (prizes)
	Chips Live = 0;
	Chips Net = 0;    // after buy-ins, everywhere
	int Wins = 0;
	int Events = 0;
};

enum class TieKind : int
{
	Friend,
	Rival,
	TrainingPartner,
	Backer, // stakes Other
	Backed, // staked by Other
	TravelPartner,
	StreamCollab,
	HomeGame,
	Count,
};
SHORTSTACKCORE_API const char* TieName(TieKind K);

/** Someone they know in the world (a handful each, so it scales). */
struct Tie
{
	int Other = -1;
	TieKind Kind = TieKind::Friend;
	float Strength = 0.0f;
	int Since = 0; // day
};

/** A step on someone's way (the player card's journey). */
enum class StepKind : int
{
	Joined,          // arrived on RiverLine
	FirstEvent,      // their first tournament
	FirstCash,
	FirstFinalTable,
	FirstWin,
	FirstLive,       // their first live event
	FirstSeries,     // a series title
	Bracelet,
	Ring,
	Major,           // a Main Event, the Summit
	BigScore,        // a new career-best score of note
	MovedUp,
	MovedDown,
	TurnedPro,
	Sponsored,
	StartedStreaming,
	WentBroke,
	Break,
	Returned,
	Retired,
	PlayerOfYear,
	Count,
};

struct Step
{
	int Day = 0;
	StepKind Kind = StepKind::Joined;
	std::string What; // the event, the stakes, the sponsor
	int Place = 0;    // a finish (or a count: followers, a tier)
	int Of = 0;       // entries
	Chips Amount = 0;
};

/** A stats page's breakdowns: tournaments by buy-in and by format. */
enum class TrackStake : int
{
	Micro, // online, up to $5.50 (freerolls too)
	Low,
	Mid,
	High,
	Live,
	Count,
};
constexpr int TrackStakeCount = static_cast<int>(TrackStake::Count);

enum class TrackFormat : int
{
	Regular,
	Deep,
	Turbo,
	Hyper,
	Bounty,
	Satellite,
	Count,
};
constexpr int TrackFormatCount = static_cast<int>(TrackFormat::Count);

struct TrackLine
{
	int Events = 0;
	Chips BuyIns = 0;
	Chips Prizes = 0; // satellite seats at their value
	int Cashes = 0;
};

/**
 * What a results tracker sees of someone: every tournament's buy-in and prize (as public as the results themselves,
 * whoever was backing them), kept as totals, breakdowns, records and a profit graph. Cash games don't count.
 * The regulars' years before the story began are filled in from their lifetime numbers, so a veteran's graph is
 * a veteran's.
 */
struct Tracker
{
	static constexpr int CurveMax = 64;
	int Events = 0;
	Chips BuyIns = 0;
	Chips Prizes = 0;
	int Cashes = 0;
	int FinalTables = 0;
	int Podiums = 0; // finished 2nd or 3rd
	int Wins = 0;
	std::array<TrackLine, TrackStakeCount> ByStake{};
	std::array<TrackLine, TrackFormatCount> ByFormat{};
	// The profit graph: the running net (prizes after buy-ins) after every Stride tournaments; when it fills, every
	// other point goes and the stride doubles. Net is the running net now (Events in).
	std::vector<Chips> Curve;
	std::vector<Chips> Spend; // buy-ins so far at each of the graph's points (the ABI graph: how their stakes moved)
	int Stride = 1;
	Chips Net = 0;
	// Records.
	Chips Peak = 0;
	int PeakAt = 0;
	Chips Downswing = 0; // the deepest fall from a peak
	int DownFrom = 0;    // tournament numbers (1-based) of that peak and the bottom
	int DownTo = 0;
	Chips Best = 0; // the biggest single prize
	int BestAt = 0;
	int Dry = 0;        // tournaments since the last cash
	int LongestDry = 0; // the longest run without one
	double FinishSum = 0.0; // place / field, summed (lower is better)
	int Finished = 0;

	/** One tournament: what it cost (all bullets), what it paid (a seat at its value), where they finished. */
	SHORTSTACKCORE_API void Add(Chips BuyIn, Chips Prize, int Place, int Field, int FinalSize, TrackStake Stake, TrackFormat Format);
	double Roi() const { return BuyIns > 0 ? static_cast<double>(Prizes - BuyIns) / static_cast<double>(BuyIns) : 0.0; }
	double Itm() const { return Events > 0 ? static_cast<double>(Cashes) / static_cast<double>(Events) : 0.0; }
	/** The average finish as a share of the field (0.25: the top quarter). */
	double AverageFinish() const { return Finished > 0 ? FinishSum / static_cast<double>(Finished) : 0.0; }
	Chips AverageBuyIn() const { return Events > 0 ? BuyIns / Events : 0; }
	/** The ABI over the K-th stretch of the graph (Stride tournaments; the last, unfinished one when K == Curve.size()). */
	SHORTSTACKCORE_API Chips StretchAbi(size_t K) const;
};

/** A bracelet or a ring someone won: the trophy case on their card, and the frame around their picture. */
struct Award
{
	int Day = 0;
	bool Ring = false;   // a ring (Ring Rush, the Grand Circuit); else a bracelet (The Championship, online or in Las Vegas)
	bool Online = true;
	bool Main = false;   // the Main Event's
	std::string Series;  // online: the series' id ("tco27", "ring28"); live: the festival ("The Championship 2027")
	std::string Event;   // the event's name
	Chips Prize = 0;
	int Entries = 0;
	int Tourney = 0;     // their tournament count when they won it (where it sits on their profit graph; 0: unknown)
};

struct Npc
{
	int Id = 0;
	std::string Name;
	std::string Country;
	int Hue = 0;
	int Born = 1995;
	Origin From = Origin::Founding;
	Region Home = Region::Americas;
	// Ability, personality, preferences.
	std::array<float, SkillCount> Skills{};
	float Peak = 0.6f;      // how far the skills can still grow
	float Potential = 0.5f; // how fast
	std::array<float, TraitCount> Traits{};
	float OnlineShare = 0.9f; // online vs live
	int Formats = 0;          // Likes
	Plan Schedule = Plan::Nights;
	bool Professional = false; // lives off poker (no job)
	// Money. Bankroll is private: the world never shows it.
	Chips Bankroll = 0;
	Chips PeakRoll = 0;
	Chips Income = 0; // a week, from a job (or streaming, a sponsor)
	Chips Living = 0; // a week, out of the bankroll (professionals)
	int Backer = -1;  // staked by (-2: a staking stable)
	float BackerShare = 0.0f;
	Chips Makeup = 0; // owed to the backer before any profit
	int BackedSince = 0;
	// Where they play.
	int Tier = 1; // online home stakes (net::Tier)
	LiveLevel Live = LiveLevel::None;
	int TierSince = 0; // day
	// Career.
	std::array<Ledger, VenueCount> Totals{};
	Season ThisSeason;
	std::vector<Year> Years;   // oldest first
	std::vector<Finish> Recent; // newest first, at most 10
	std::array<float, 8> Weekly{}; // board points a week, oldest first
	float WeekPoints = 0.0f;
	std::string BestEvent;
	int BestDay = 0;
	int Bracelets = 0;
	int Rings = 0;
	int Titles = 0; // online series titles
	int Majors = 0; // main events (online series mains, the Championship, the Summit)
	// How it's going.
	Momentum Mood = Momentum::Normal;
	int MoodSince = 0;
	float Confidence = 0.0f; // -1..1
	float Fatigue = 0.0f;    // 0..1
	float Swing = 0.0f;      // recent results, in buy-ins (smoothed)
	float Tilt = 0.0f;       // 0..1, fades
	int WeekEvents = 0;
	std::array<int, 4> WeekKinds{}; // bounty, satellite, live, turbo events this week (what they practise)
	Chips WeekNet = 0;    // won minus spent this week
	Chips MonthNet = 0;   // the last four weeks (smoothed)
	int WeeksBroke = 0;
	std::array<float, RepCount> Fame{};  // accumulated, decaying (Reps come from these)
	std::array<float, RepCount> Reps{};  // 0..100
	// Streaming.
	bool Streams = false;
	int Followers = 0;
	std::string Sponsor; // brand, or ""
	// Story.
	bool Pro = false;      // Team RiverLine
	bool Rival = false;    // the player's rival
	bool Anchored = false; // the people the story needs (they never quit for good)
	bool Faded = false;    // long retired and little remembered: only the name and the record are kept
	Status St = Status::Active;
	int Until = 0; // break: back on this day
	int Joined = 0;
	int LastDay = -9999; // last played
	int Left = 0;        // retired or broke on this day
	int TripFrom = 0;    // travelling for a live series from this day
	int TripUntil = -1;  // until this day
	std::string Trip;    // the series
	Identity Is = Identity::OnlineGrinder;
	Identity Began = Identity::OnlineGrinder; // what they were on their first day
	// A newcomer's arrival (the card's journey starts here) and the steps since, oldest first (a few kept).
	Arrival Came = Arrival::None;
	int Arrived = 0;    // the day they arrived (Came != None)
	int CameWith = -1;  // who brought them, or who they watched (-1: nobody)
	std::vector<Step> Path;
	std::vector<Award> Awards; // every bracelet and ring, oldest first
	Tracker Stats;             // the stats page
	std::vector<Tie> Ties;
	std::map<std::string, int> Tickets; // event template id -> seats won

	float SkillOf(world::Skill S) const { return Skills[static_cast<size_t>(S)]; }
	float TraitOf(world::Trait T) const { return Traits[static_cast<size_t>(T)]; }
	float RepOf(world::Rep R) const { return Reps[static_cast<size_t>(R)]; }
	SHORTSTACKCORE_API float Overall() const; // average skill
	SHORTSTACKCORE_API int Age(int Day) const;
	SHORTSTACKCORE_API Chips CareerWon() const;
	bool Playing() const { return St == Status::Active; }
};

// ------------------------------------------------------------------ the player in their memory

enum class MemoryKind : int
{
	Met,            // first time at a table together
	KnockedOutHero, // they sent the player home
	HeroKnockedOut, // the player sent them home
	BigPotWon,      // the player won a big pot against them
	BigPotLost,
	FinalTable, // both at a final table
	HeadsUpWon, // the player beat them heads-up for a title
	HeadsUpLost,
	BackRoom,  // a night at Dee's game together
	Riverside, // the Sunday tournament together
	ShowedBluff, // they showed the player a bluff
	Count,
};
SHORTSTACKCORE_API const char* MemoryText(MemoryKind K);

struct Memory
{
	int Day = 0;
	MemoryKind Kind = MemoryKind::Met;
	std::string Where; // the event
	Chips Amount = 0;
};

/** What one person thinks of the player, and why. Several dimensions, never a single friendship meter. */
struct Bond
{
	float Familiarity = 0.0f; // how well they know the player
	float Respect = 0.0f;
	float Trust = 0.0f;
	float Rivalry = 0.0f;
	float Resentment = 0.0f;
	int Encounters = 0;
	int LastDay = -9999;
	std::vector<Memory> Memories; // oldest first, the first meeting always kept
	/** "Knows you", "Rival", "Holds a grudge", "Respects you", ... (empty: a stranger). */
	SHORTSTACKCORE_API std::string Label() const;
};

// ------------------------------------------------------------------ what happened

enum class EventKind : int
{
	Won,          // a title worth reporting
	FinalTable,   // at a major
	WentBroke,
	MovedUp,
	MovedDown,
	Retired,
	Returned,
	Breakout,
	Qualified,    // won a seat to a major
	Sponsored,
	Milestone,    // career milestone (first title, $1M, 100 final tables, ...)
	StartedStreaming,
	StreamMilestone,
	Debut,        // a new name on the circuit
	Discovered,   // an unknown made a name with one result
	Backed,       // took a stake
	Rivalry,      // two players keep meeting at final tables
	Champion,     // a major title (also in Honors)
	PlayerOfYear,
	Break,        // stepped away
	Count,
};
SHORTSTACKCORE_API const char* EventKindName(EventKind K);

/** A world event (simulation output): news, phone texts, commentary and the player cards read these. */
struct WorldEvent
{
	double At = 0.0;
	EventKind Kind = EventKind::Won;
	int Npc = -1;
	int Other = -1;
	Chips Amount = 0;
	std::string What; // the event, the stakes, the milestone
	int Value = 0;    // place, entries, tier, followers
	int Flags = 0;    // EventFlag bits
};

/** What made a world event special (bits). */
enum EventFlag : int
{
	FlagQualifier = 1, // played on a satellite seat
	FlagStaked = 2,    // someone else's money
	FlagFirst = 4,     // a first (title, final table, cash of this size)
	FlagHero = 8,      // the player was there
	FlagLive = 16,     // a live event
	FlagUnknown = 32,  // the world hadn't been following them
};

/** A title that stays in history. */
struct Honor
{
	int Year = 2026;
	std::string Title;   // "RCOP Main Event", "Championship Main Event", "Player of the Year"
	std::string EventId;
	int Npc = -1; // -1: the player
	std::string Name; // who (kept even if they leave the world)
	Chips Prize = 0;
	int Entries = 0;
	double At = 0.0;
};

/**
 * A player card: what anyone can see about someone. No bankroll, no plans, no state of mind; reputations,
 * results, titles, who they're known to run with, and what they think of the player (and why).
 */
struct Profile
{
	int Id = -1;
	std::string Name;
	std::string Country;
	int Age = 0;
	std::string Known;  // what the world calls them ("Online grinder")
	std::string Status; // "Active", "On a break", "Retired", "Away from the tables"
	std::string Stakes; // "Mid stakes online"
	std::string Live;   // "Travels the Grand Circuit"
	std::string Style;  // "Loose and aggressive"
	int Since = 2026;   // first year on the scene
	std::array<int, RepCount> Reps{};
	std::array<Ledger, VenueCount> Totals{}; // buy-ins are private (Spent is zero)
	int Bracelets = 0;
	int Rings = 0;
	int Titles = 0;
	int Majors = 0;
	std::vector<Award> Awards; // oldest first
	Tracker Stats;
	std::string BestEvent;
	Chips Best = 0;
	int BestDay = 0;
	std::vector<Finish> Recent;
	std::vector<Year> Years; // Net is private (zero)
	std::array<float, 8> Form{};
	Season ThisSeason;
	bool Streams = false;
	int Followers = 0;
	std::string Sponsor;
	bool Pro = false;
	bool Rival = false;
	std::vector<std::pair<std::string, std::string>> Knows; // what (Rival, Training partner, ...), who
	std::string Bond;                  // what they think of the player ("" for a stranger)
	std::vector<std::string> Memories; // why, newest first
	std::vector<std::string> Story;    // headlines about them, newest first
	// The journey: how they got here and the steps since.
	std::string Came; // "Came over when NorthPot closed its doors, after six years there." ("" for an old hand)
	int Arrived = -1;    // the day they arrived on RiverLine (-1: before the story began)
	bool New = false;    // arrived in the last month
	struct Moment
	{
		int Day = 0;
		StepKind Kind = StepKind::Joined;
		std::string Text;
	};
	std::vector<Moment> Journey; // oldest first
};

// ------------------------------------------------------------------ the live calendar

enum class LiveKind : int
{
	Local,        // the Riverside's Sunday $150 and rooms like it
	Underground,  // back-room cash games
	Regional,
	Circuit,
	HighRoller,
	Championship, // the summer Championship's bracelet events
	ChampionshipMain,
	Summit,       // the invitation-only $1,000,000 buy-in finale of the year
};

/** A live event (the world plays these; the player meets them at the Riverside and the Back Room). */
struct LiveEvent
{
	std::string Id;
	std::string Name;
	std::string City;
	std::string Series; // the festival it belongs to ("" for a local weekly)
	Region Where = Region::Americas;
	LiveKind Kind = LiveKind::Local;
	LiveLevel Level = LiveLevel::Local;
	Chips BuyIn = 0;
	Chips Stakes = 0; // underground: the big blind
	int Field = 0;
	double Start = 0.0; // world minutes
	double Duration = 0.0;
	bool Bracelet = false;
	bool Ring = false;
	bool Main = false;
	std::string Ticket; // satellites into this event win this ticket ("" for none)
};
/** The live events starting on Day, in start order (the same calendar in every save). */
SHORTSTACKCORE_API std::vector<LiveEvent> LiveCalendar(int Day);
/** The festivals running on or after Day within Span days (for travel plans). */
SHORTSTACKCORE_API std::vector<LiveEvent> LiveFestivals(int Day, int Span);

// ------------------------------------------------------------------ the world

/** The calendar year of a world day (Day 0 is October 5, 2026) and the day each year begins. */
SHORTSTACKCORE_API int YearOf(int Day);
SHORTSTACKCORE_API int YearStart(int Year);
/** The calendar date of a world day (Month and DayOfMonth from 1). */
SHORTSTACKCORE_API void CivilDate(int Day, int& Year, int& Month, int& DayOfMonth);
/** The world day of a calendar date. */
SHORTSTACKCORE_API int DayOn(int Year, int Month, int DayOfMonth);

/** Someone entered in an event. */
struct Entry
{
	int Npc = -1;
	bool Ticket = false; // a seat won in a satellite
	bool Staked = false; // the buy-in came from a backer
	int Bullets = 1;     // re-entries fired
	float Share = 1.0f;  // of the prize they keep (the rest was sold)
	int Known = 0;  // the player's tables: their real place (0: unknown)
	int Better = 0; // the player's tables: still in when the player left, so better than this place (0: no bound)
};

/** An event the world has planned (people registered) and will resolve when it ends. */
struct Pending
{
	std::string Id;
	std::string Name;
	bool Online = true;
	int Template = -1; // net::Network template (online)
	int Kind = 0;      // LiveKind (live)
	double Start = 0.0;
	double End = 0.0;
	int Entries = 0;
	Chips Pool = 0;
	Chips BuyIn = 0;
	int Tier = 1;
	int Format = 0; // net::Format (online)
	int TableSize = 9;
	bool Major = false;
	bool Bracelet = false;
	bool Ring = false;
	std::string Series;
	std::string Ticket;     // satellites: seats are tickets to this
	Chips SeatValue = 0;
	Chips Stakes = 0;       // underground: the big blind
	int HeroPlace = 0;      // the player's finish (0: not playing, -1: still playing)
	Chips HeroPrize = 0;
	double Strength = 0.4; // the field's skill (anonymous players included)
	double Luck = 1.0;     // how much skill shows (hypers and flips: less)
	int Kinds = 0;         // format bits (sim)
	int Seats = 0;         // satellites: seats awarded
	int FinalSize = 9;     // places at the final table
	std::vector<Entry> Who;
};

class World
{
public:
	SHORTSTACKCORE_API World();

	/** A new world: the network's regulars, the people you can meet, and this save's hidden traits. */
	SHORTSTACKCORE_API void Create(uint32_t Seed, double StartWorld);
	bool Ready() const { return Created; }
	/** Changes when something involving the player happens (the session saves the world then, not every frame). */
	int HeroRevision() const { return HeroRev; }
	uint32_t Seed() const { return WorldSeed; }
	/** When this world began (world minutes): earlier results are the network's history. */
	double StartedAt() const { return Origin; }
	double Clock() const { return Now; }
	int Revision() const { return Rev; }

	/** Plays the world forward to this time (events that end by then are decided, days and weeks roll over). */
	SHORTSTACKCORE_API void AdvanceTo(double World);
	/** Debug: plays the world forward by whole days on its own (the player's clock doesn't move). */
	SHORTSTACKCORE_API void Simulate(int DayCount);

	// People.
	const std::vector<Npc>& People() const { return Roster; }
	SHORTSTACKCORE_API int Find(const std::string& Name) const;
	const Npc* Get(int Id) const { return Id >= 0 && Id < static_cast<int>(Roster.size()) ? &Roster[static_cast<size_t>(Id)] : nullptr; }
	/** The network's view of everyone (net::Player rows, the same indices): what the RiverLine screens show. */
	const std::vector<net::Player>& View() const { return Rows; }
	SHORTSTACKCORE_API int ActiveCount() const;

	// Events.
	/** The result of a finished event this world played (nullptr: not one of its events). */
	SHORTSTACKCORE_API const net::EventResult* ResultOf(const std::string& EventId) const;
	/** Who the world has registered for an event (planned for its day). */
	SHORTSTACKCORE_API std::vector<int> Registered(const std::string& EventId);
	/** Makes sure the events starting on Day have their registrations (the player is looking at tomorrow). */
	SHORTSTACKCORE_API void EnsurePlanned(int Day);
	/** Board value for a person (the network's leaderboards, live data), and their rank a day (an hour) ago. */
	SHORTSTACKCORE_API double BoardValue(net::Board B, int Npc, double At) const;
	SHORTSTACKCORE_API int RankBefore(net::Board B, int Npc) const;

	// The player.
	/** Someone takes a seat in an event (the player's tables seat a regular the world hadn't planned there). */
	SHORTSTACKCORE_API void Join(const std::string& EventId, int Npc);
	/** The player registered (their result decides where everyone else finishes around them). */
	SHORTSTACKCORE_API void HeroEntered(const std::string& EventId, const std::string& EventName);
	/** The player's finish, and what their tables saw: who busted where, who was still in, who knocked whom out. */
	struct TableReport
	{
		std::vector<std::pair<int, int>> Places; // npc, place (busted before the player)
		std::vector<int> StillIn;                // npcs still in when the player finished
		std::vector<int> Met;                    // at the player's table at some point
		int KnockedOutHero = -1;                 // npc
		std::vector<int> HeroKnockedOut;         // npcs
		std::vector<std::pair<int, Chips>> BigPots; // npc, net to the player (+ won, - lost)
	};
	SHORTSTACKCORE_API void HeroFinished(const std::string& EventId, int Place, Chips Prize, const TableReport& Report);
	/** A night at Dee's game with these people (names as the Back Room knows them). */
	SHORTSTACKCORE_API void BackRoomNight(double At, const std::vector<std::string>& Names, Chips HeroNet);
	/** A Riverside Sunday: who finished where (names as the field knows them), and the player's place. */
	SHORTSTACKCORE_API void RiversideDone(double At, const std::vector<std::pair<std::string, int>>& Places, int HeroPlace, int Field);
	SHORTSTACKCORE_API void Remember(int Npc, MemoryKind Kind, const std::string& Where, Chips Amount, double At);
	const std::map<int, Bond>& Bonds() const { return HeroBonds; }
	SHORTSTACKCORE_API const Bond* BondWith(int Npc) const;
	/** A line for a table chat when the player sits down with someone who remembers them ("" for nothing to say). */
	SHORTSTACKCORE_API std::string Greeting(int Npc, uint32_t Salt) const;
	/** The player's name, as the world knows it (honors, headlines). */
	std::string HeroName;
	/** The bracelets and rings the player has won, oldest first. */
	const std::vector<Award>& HeroAwards() const { return HeroTrophies; }
	/** The player's own stats page (their RiverLine tournaments since Night One). */
	const Tracker& HeroStats() const { return HeroBook; }
	/** Where a stats page's ROI ranks among the regulars with at least Min tournaments (0..1: the share it beats). */
	SHORTSTACKCORE_API double RoiRank(double Roi, int Min = 200) const;
	/**
	 * Debug: a bracelet (The Championship Online's) or a ring (Ring Rush's) for someone (-1: the player), from the
	 * latest of those series to have started, its Main Event's when Main. Their card and their frame show it.
	 */
	SHORTSTACKCORE_API void GrantAward(int Npc, bool Ring, bool Main);
	/** Debug: Count micro- and low-stakes tournaments on the player's stats page (a small winner's luck), to preview it. */
	SHORTSTACKCORE_API void GrantHeroResults(int Count);
	/** The player's Player of the Year points this calendar year (online, live). */
	double HeroSeasonPoints() const { return HeroPoints; }
	double HeroSeasonLivePoints() const { return HeroLivePoints; }

	// History.
	const std::vector<WorldEvent>& Events() const { return Log; }
	const std::vector<Honor>& Honors() const { return Titles; }
	/** Who leads each board when the world is attached, by recorded value. */
	SHORTSTACKCORE_API std::vector<int> Leaders(Rep R, int Count) const;

	// Presentation (WorldText.cpp): the simulation's events in words. Private events (going broke, taking a stake)
	// never make a headline.
	SHORTSTACKCORE_API Profile ProfileOf(int Npc) const;
	SHORTSTACKCORE_API bool Headline(const WorldEvent& E, std::string& Title, std::string& Body, std::string& Tag) const;

	// Debug.
	SHORTSTACKCORE_API std::string Describe(int Npc) const;
	SHORTSTACKCORE_API std::string Report() const;
	/** Where the money went since the last reset (dollars): prizes, buy-ins, deposits and pay, living costs, cash-outs, travel. */
	const std::array<double, 6>& Flows() const { return Flow; }
	/** Prizes, buy-ins and entries by kind of event since the last reset (dollars). */
	const std::map<std::string, std::array<double, 3>>& FlowsByKind() const { return FlowBy; }
	void ResetFlows()
	{
		Flow = {};
		FlowBy.clear();
	}

	// Save.
	SHORTSTACKCORE_API void Write(std::string& Out) const;
	/** One save line split on tabs (the "world" prefix included); false when it isn't a world line. */
	SHORTSTACKCORE_API bool Read(const std::vector<std::string>& Fields);
	/** After the last line: rebuilds what isn't saved (indexes, the network view). */
	SHORTSTACKCORE_API void Finish();

private:
	friend struct Sim;
	bool Created = false;
	uint32_t WorldSeed = 0;
	double Now = 0.0;
	double Origin = 0.0;
	int Planned = -9999; // last day planned
	int Rev = 0;
	int HeroRev = 0;
	int Founding = 0; // the network's regulars, at the start of the roster
	std::vector<Npc> Roster;
	std::map<std::string, int> ByName;
	std::vector<net::Player> Rows;
	std::vector<Pending> Queue; // planned events, by end time
	std::map<std::string, net::EventResult> Results; // the last few days' final tables
	std::map<std::string, double> ResultEnds;
	std::vector<WorldEvent> Log;  // newest last (capped)
	std::vector<Honor> Titles;    // forever
	std::map<int, Bond> HeroBonds;
	std::map<std::string, std::pair<int, Chips>> HeroResults; // event id -> place, prize (the player's finished events)
	std::vector<Award> HeroTrophies;
	Tracker HeroBook;
	std::set<std::string> HeroIn; // events the player is in right now
	// The boards' running numbers.
	std::string SeriesKey;
	std::map<int, double> SeriesPoints;
	double NightKey = -1.0;
	std::map<int, double> NightPoints;
	std::map<int, double> NightPointsHourAgo;
	double NightHour = -1.0;
	std::array<std::vector<int>, 7> RanksYesterday; // by net::Board
	int Week = -9999;
	int Month = -9999;
	int DiscoveredThisWeek = 0;
	std::vector<std::pair<int, int>> WeekPairs; // people who met at final tables this week
	int NextAnon = 0;
	std::array<double, 5> Field{};    // online field strength by tier (moves with the regulars' skill)
	std::array<double, 5> FieldRef{}; // the regulars' average skill by tier when the world began
	int Target = 0;                   // the active population the world keeps (it grows slowly)
	double HeroPoints = 0.0;          // the player's online season points (Player of the Year)
	std::array<double, 6> Flow{};     // diagnostics only (not saved)
	std::map<std::string, std::array<double, 3>> FlowBy;
	double HeroLivePoints = 0.0;

	void Refresh(int Id);
	void RefreshAll();
};
} // namespace world
} // namespace ss
