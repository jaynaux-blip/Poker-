#pragma once

#include "ShortStack/Game/Lobby.h"

#include <array>
#include <cmath>
#include <map>

namespace ss
{
struct HistoryEntry;

/**
 * The RiverLine poker network around the player: a week-round tournament schedule in the style of the big
 * online sites, the series on its calendar, the regulars who play it, and what they have won.
 *
 * Everything is generated from fixed seeds and the world clock, so the network is the same on every run
 * and moves forward as the night does. Tournaments the player isn't in are not dealt hand by hand. They
 * are simulated statistically: entries build through registration, the field shrinks, and a final table
 * of regulars (weighted by skill, volume and stakes) takes the prizes. Those results feed the
 * leaderboards and the news.
 */
namespace net
{
// ------------------------------------------------------------------ calendar

/** World time is minutes since Monday, October 5, 2026, 00:00, on the player's clock. */
constexpr double MinutesPerDay = 1440.0;
/** Night One: the early hours of Tuesday, October 6. */
constexpr int NightOneDay = 1;

inline int DayOf(double WorldMinutes) { return static_cast<int>(std::floor(WorldMinutes / MinutesPerDay)); }
/** 0 = Monday. */
SHORTSTACKCORE_API int Weekday(int Day);
SHORTSTACKCORE_API const char* WeekdayName(int Day, bool Long = false);
/** "Oct 6". */
SHORTSTACKCORE_API std::string DateLabel(int Day);
/** "2:07 AM". */
SHORTSTACKCORE_API std::string TimeLabel(double WorldMinutes);
/** "42s", "12m", "1h 23m", "5d 14h". */
SHORTSTACKCORE_API std::string Countdown(double Minutes);
/** "$1M", "$250K", "$5.5K", "$750". */
SHORTSTACKCORE_API std::string MoneyShort(Chips Cents);
/** Buy-ins as players write them: "$1.10", "$215", "$1,050", "Free". */
SHORTSTACKCORE_API std::string BuyIn(Chips Cents);

// ------------------------------------------------------------------ schedule

enum class Tier : int
{
	Freeroll,
	Micro, // up to $5.50
	Low,   // up to $55
	Mid,   // up to $530
	High,
};
SHORTSTACKCORE_API Tier TierOf(Chips BuyInCents);
/** The site's fee inside a buy-in, the way lobbies split it: $2.20 = $2.00 + $0.20, $215 = $200 + $15. */
SHORTSTACKCORE_API Chips FeeOf(Chips BuyInCents);
SHORTSTACKCORE_API const char* TierName(Tier T);

enum class Format : int
{
	Freezeout,
	ReEntry,
	Bounty,  // progressive knockout
	Mystery, // mystery bounty
	Satellite,
	Flip, // flip & go
};

enum class Status : int
{
	Announced,
	Registering,
	LateReg,
	Running,
	FinalTable,
	Finished,
};
SHORTSTACKCORE_API const char* StatusName(Status S);

struct EventTemplate
{
	std::string Id;
	std::string Name;
	std::string Series; // series id, or empty
	int EventNo = 0;    // number within the series
	Chips BuyInCents = 0;
	Chips GtdCents = 0; // guarantee (the fixed pool of a freeroll)
	bool Omaha = false;
	Format Fmt = Format::Freezeout;
	std::string Speed = "Regular"; // Hyper, Turbo, Regular, Deep
	double LevelMinutes = 10.0;
	int StartStack = 20000;
	int TableSize = 8;
	int LateRegMinutes = 120;
	int Field = 500;       // typical entries
	int StartMinute = 0;   // minutes after midnight
	int EveryMinutes = 0;  // repeat interval within the day (0 = once)
	int LastMinute = 1439; // no repeats after this
	int Days = 0x7f;       // weekday mask, bit 0 = Monday
	int OnlyDay = -9999;   // a one-off on this world day
	int OpensHours = 18;   // registration opens this long before the start
	bool Featured = false;
	int JoinIndex = -1; // Lobby() index when the event can be played in this build
	std::string Seats;  // satellites: what the seats are for
	std::string SeatTicket; // satellites: the template a seat enters
	Chips SeatValueCents = 0;
	std::string Blurb;
};

struct EventInstance
{
	int Template = 0;
	std::string Id;
	double Start = 0.0; // world minutes
	uint32_t Seed = 0;
	int Entries = 0;     // where registration will end up
	Chips Pool = 0;      // final prize pool
	double Duration = 0; // minutes from the start to the winner
	double LateReg = 0;  // minutes of late registration (never past the middle of the event)
};

struct LiveState
{
	Status St = Status::Announced;
	int Entries = 0; // so far
	Chips Pool = 0;  // what is being played for (the guarantee until entries exceed it)
	bool Overlay = false;
	int Left = 0;
	int Level = 0;
	double LateRegLeft = 0.0; // minutes
	double RegOpensIn = 0.0;  // minutes (Announced)
	double StartsIn = 0.0;    // minutes (before the start)
	double Progress = 0.0;    // 0..1 from the start to the winner
};

// ------------------------------------------------------------------ people

struct Player
{
	std::string Name;
	std::string Country; // ISO 3166 alpha-2
	int Hue = 0;         // avatar color
	float Skill = 0.5f;
	float Volume = 0.5f;
	Tier Stake = Tier::Micro;
	bool Pro = false; // Team RiverLine
	bool Rival = false;
	// Lifetime before the simulated window.
	Chips Earnings = 0;
	int Wins = 0;
	int FinalTables = 0;
	int Cashes = 0;
	Chips Best = 0;
	// 2026 season before the simulated window.
	double SeasonPoints = 0.0;
	Chips SeasonEarnings = 0;
	int SeasonWins = 0;
	int SeasonFinalTables = 0;
	std::array<float, 8> Form{}; // weekly points, oldest first
};

struct Placing
{
	int Player = -1; // index into Players(), -1 for the player (an event they played), -2 for someone nobody follows
	int Place = 0;
	Chips Prize = 0;
	std::string Name;    // Player -2: their screen name
	std::string Country; // and flag
};

struct EventResult
{
	std::vector<Placing> FinalTable; // places 1..9
};

/** What the player has done, for the boards (built from the save's history). */
struct HeroStats
{
	std::string Name;
	Chips Earnings = 0;
	int Wins = 0;
	int FinalTables = 0;
	int Cashes = 0;
	Chips Best = 0;
	double SeasonPoints = 0.0;
	double NightPoints = 0.0;
	double SeriesPoints = 0.0;
	int SeriesTitles = 0;
	int Tournaments = 0;
};
/** Night points count the Night Shift that contains Now (life::NightShiftStart). */
SHORTSTACKCORE_API HeroStats StatsFrom(const std::string& Name, const std::vector<HistoryEntry>& History, double Now = MinutesPerDay + 127.0);
/** When a result's event started (world minutes), from its event id; Night One for older saves. */
SHORTSTACKCORE_API double EntryStart(const HistoryEntry& E);

/** Formats the player has unlocked (bits for Joinable and Listing). */
enum Unlock : int
{
	UnlockBounty = 1,    // first cash
	UnlockSatellite = 2, // first final table
	UnlockSixMax = 4,    // first title
};
/** Leaderboard points for a finish (the network's formula: field size, buy-in and place). */
SHORTSTACKCORE_API double Points(int Place, int Entries, Chips BuyInCents);

enum class Board : int
{
	Earnings,    // all time
	Season,      // Player of the Year points, 2026
	Wins,        // 2026
	FinalTables, // 2026
	Series,      // the running series
	NightShift,  // tonight's micro-stakes leaderboard
	Live,        // live Player of the Year points (a living world only)
};

struct BoardRow
{
	int Rank = 0;
	int Player = -1; // index into Players(), or -1 for the hero
	double Value = 0.0;
	int Move = 0; // places gained since yesterday
	std::array<float, 8> Form{};
	Chips Prize = 0; // leaderboard prize (Night Shift)
};

struct SeriesInfo
{
	std::string Id;
	std::string Name;
	std::string Short;
	int FirstDay = 0;
	int LastDay = 0;
	int Events = 0;
	Chips GtdCents = 0;
	uint32_t Color = 0x27d3c3;
	uint32_t Color2 = 0x3b82f6;
	std::string Tagline;
	std::string MainEvent; // template id
};

enum class NewsKind : int
{
	BigWin,
	Series,
	Schedule,
	Record,
	Hero,
};

struct NewsItem
{
	double At = 0.0;
	NewsKind Kind = NewsKind::BigWin;
	std::string Title;
	std::string Body;
	Chips Amount = 0;
	int Player = -1;
	std::string Tag;
};

class Network
{
public:
	SHORTSTACKCORE_API Network();

	const std::vector<Player>& Players() const { return People; }
	/** The regulars as they were on Night One (the living world starts from these). */
	const std::vector<Player>& Founding() const { return People; }
	/**
	 * What each regular did in the network's own simulated past, from SimFirstDay up to To: season points,
	 * prize money, wins and final tables (the numbers the boards showed before a living world took over).
	 */
	SHORTSTACKCORE_API void FoundingTally(double To, std::vector<double>& Points, std::vector<Chips>& Money, std::vector<int>& Wins, std::vector<int>& FinalTables) const;
	const std::vector<EventTemplate>& Templates() const { return Temps; }
	const std::vector<SeriesInfo>& Series() const { return AllSeries; }
	const EventTemplate& TemplateOf(const EventInstance& E) const { return Temps[static_cast<size_t>(E.Template)]; }
	const SeriesInfo* FindSeries(const std::string& Id) const;
	/** The series running at this time (or the next one to start). */
	SHORTSTACKCORE_API const SeriesInfo* CurrentSeries(double Now) const;
	int RivalIndex() const { return Rival; }
	/** A regular by screen name (-1 when not one of the network's regulars). */
	SHORTSTACKCORE_API int FindPlayer(const std::string& Name) const;

	/** Events starting in [From, To), in start order. */
	SHORTSTACKCORE_API std::vector<EventInstance> Window(double From, double To) const;
	/** Where an event stands at Now. */
	SHORTSTACKCORE_API LiveState Live(const EventInstance& E, double Now) const;
	/** The final table of a finished event. */
	SHORTSTACKCORE_API const EventResult& Result(const EventInstance& E) const;
	/** Prize table for an event's final field (first place first). */
	SHORTSTACKCORE_API std::vector<Chips> Payouts(const EventInstance& E) const;
	/**
	 * The lobby listing for a scheduled event, with the tournament to play. Tonight's story events keep their
	 * hand-tuned spec; other Hold'em events get one built from the schedule. Joinable is false for events the
	 * tables can't run yet (bounties, satellites, Omaha, six-max, huge fields); Lock says why.
	 */
	SHORTSTACKCORE_API LobbyEvent Listing(const EventInstance& E, std::string* Lock = nullptr, int Unlocks = 0) const;
	/** Whether the tables can run this event (Listing's Joinable), without building the listing. */
	SHORTSTACKCORE_API bool Joinable(const EventInstance& E, std::string* Lock = nullptr, int Unlocks = 0) const;
	/** The instance a Listing's spec came from, by id ("night-owl@1560"); false when it isn't scheduled. */
	SHORTSTACKCORE_API bool FindInstance(const std::string& Id, EventInstance& Out) const;
	/** The first instance of a template starting at or after From (within two weeks). */
	SHORTSTACKCORE_API bool Next(const std::string& TemplateId, double From, EventInstance& Out) const;
	/** Featured events starting in (Now, Now + Horizon], soonest first. */
	SHORTSTACKCORE_API std::vector<EventInstance> Upcoming(double Now, double Horizon) const;
	/** Simulates every result up to Now ahead of time (a loading moment), so the first board or page doesn't stall. */
	SHORTSTACKCORE_API void Prewarm(double Now) const;
	/** The player's own results (from the save): their finishes replace a regular in those final tables. */
	SHORTSTACKCORE_API void SetHero(const std::string& Name, const std::vector<HistoryEntry>& History);
	const std::string& HeroName() const { return YouName; }

	/** A board's leading rows; HeroRow (if set) gets the player's own standing. */
	SHORTSTACKCORE_API std::vector<BoardRow> Leaderboard(Board B, double Now, const HeroStats& Hero, int Count, BoardRow* HeroRow) const;
	/** Night Shift leaderboard: prize for a rank (top 20 share $1,000). */
	static SHORTSTACKCORE_API Chips NightShiftPrize(int Rank);
	/** Headlines up to Now, newest first. */
	SHORTSTACKCORE_API std::vector<NewsItem> News(double Now, const HeroStats& Hero, int Count) const;

private:
	void BuildPlayers();
	void BuildSchedule();
	void BuildSeries();
	void Instances(int Day, std::vector<EventInstance>& Out) const;
	EventInstance Make(int TemplateIndex, double Start) const;
	/** Finished events in [From, To) by finish time, with results. */
	std::vector<EventInstance> Finished(double From, double To) const;
	struct Totals
	{
		std::vector<double> Points;
		std::vector<Chips> Money;
		std::vector<int> Wins;
		std::vector<int> FinalTables;
	};
	/** Totals for events finishing in [From, To); whole days before To's are cached. */
	Totals Tally(double From, double To, const std::string& SeriesId, bool MicroOnly) const;
	void Accumulate(Totals& T, double From, double To, const std::string& SeriesId, bool MicroOnly) const;
	struct Ranking
	{
		std::vector<double> Values;
		std::vector<int> Order;
		std::vector<int> RankBefore;
	};
	const Ranking& Ranked(Board B, double Now) const;

	std::vector<Player> People;
	std::map<std::string, int> ByName;
	std::array<std::vector<double>, 5> TierWeights; // final-table odds of each player by the event's tier
	std::vector<EventTemplate> Temps;
	std::vector<SeriesInfo> AllSeries;
	int Rival = -1;
	mutable std::map<std::string, EventResult> Results;
	mutable std::map<int, std::vector<EventInstance>> DayCache;
	mutable std::map<std::pair<int, long long>, Ranking> RankCache; // (board, minute)
	mutable std::map<std::string, Totals> TallyCache;              // whole days, by range and filter
	std::string YouName;
	std::map<std::string, std::pair<int, Chips>> HeroFinishes; // instance id -> (place, prize)
};

/** The network every screen shows (built on first use). */
SHORTSTACKCORE_API Network& Shared();
} // namespace net
} // namespace ss
