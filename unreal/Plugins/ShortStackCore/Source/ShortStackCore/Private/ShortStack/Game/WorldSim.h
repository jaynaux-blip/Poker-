// The living world's simulation, shared by its .cpp files (World, WorldSim, WorldHero, WorldSave, WorldText).
#pragma once

#include "ShortStack/Game/World.h"
#include "ShortStack/Rng.h"

#include <cmath>
#include <string>
#include <vector>

namespace ss
{
namespace world
{
/** What an event is like (bits): what it rewards, and what playing it practises. */
enum KindBit : int
{
	KindBounty = 1,
	KindSatellite = 2,
	KindLive = 4,
	KindTurbo = 8,
	KindDeep = 16,
	KindOmaha = 32,
	KindMystery = 64,
	KindHyper = 128,
	KindReEntry = 256,
	KindUnderground = 512,
	KindSixMax = 1024,
};

namespace sim
{
/** A typical buy-in at each online tier (net::Tier), in cents. */
constexpr Chips TierAbi[5] = {0, 300, 2200, 15000, 105000};
/** The buy-ins a player at each tier plays, in cents. */
constexpr Chips TierLow[5] = {0, 25, 330, 2200, 21500};
constexpr Chips TierHigh[5] = {0, 550, 5500, 53000, 525000};
/** The comfortable buy-in (cents) where each tier starts (Sim::TierFor). */
constexpr Chips TierFloor[5] = {0, 0, 600, 4500, 32000};
/** What it takes to travel for each live level (dollars a hundred buy-ins of discipline would want). */
constexpr double LiveNeed[6] = {0.0, 1000.0, 8000.0, 40000.0, 150000.0, 1500000.0};
/** Weekly living costs of a professional at each tier (dollars), before their own lifestyle. */
constexpr double Living[5] = {120.0, 160.0, 280.0, 550.0, 1200.0};
/** The active population on day one (the network's regulars, the cast, Kast's streamers). */
constexpr int FoundingPopulation = 1618;
/** Results the world keeps in memory: every event for this long, the big ones for a year. */
constexpr double KeepResults = 8.0 * 1440.0;
constexpr double KeepMajors = 400.0 * 1440.0;
constexpr size_t MaxLog = 6000;
/** World::RoiRank and RoiPlace rank nobody against fewer qualifying regulars than this: unknown (-1; update 2 bug 6). */
constexpr int RoiRankMinimum = 20;

// The rival sites (update 2 §5.9, G3): who has an account where, and how they choose between the sites.
/** RunItTwice from Night One: online regulars (OnlineShare at least this) at Low and up, and Micro ones. */
constexpr float RunItTwiceShare = 0.5f;
constexpr double RunItTwiceLowUp = 0.22;
constexpr double RunItTwiceMicro = 0.08;
/** Halcyon from Night One (P1's site, seeded now so P1 needs no world bump): High-tier online regulars. */
constexpr double HalcyonHigh = 0.30;
/** Ignite from its launch: over IgniteJoinDays, Micro and Low regulars (OnlineShare at least IgniteShare) join; from its
 *  launch, IgniteArrivals of the new online arrivals start with an account, IgniteOnly of those on Ignite alone. */
constexpr float IgniteShare = 0.4f;
constexpr double IgniteMicro = 0.30;
constexpr double IgniteLow = 0.10;
constexpr int IgniteJoinDays = 30;
constexpr double IgniteArrivals = 0.40;
constexpr double IgniteOnly = 0.20;
/** The skilled (Overall at least this) like Ignite's soft fields and an overlay; Mid and High players like RunItTwice's
 *  majors (§5.9's weights). */
constexpr float SkilledOverall = 0.70f;
constexpr double SkilledIgnite = 1.3;
constexpr double SkilledOverlay = 1.2;
constexpr double RunItTwiceMajors = 1.2;
/** Team RunItTwice's size (the SiteDef's TeamSize when it has one). */
constexpr int RunItTwiceTeam = 6;
/** An overlay worth a headline on the site's own news: at least this much, and a tenth of the guarantee. */
constexpr Chips OverlayNews = 50000;

inline double Clamp(double V, double Lo, double Hi)
{
	return V < Lo ? Lo : V > Hi ? Hi : V;
}
inline float Clampf(double V, double Lo, double Hi)
{
	return static_cast<float>(Clamp(V, Lo, Hi));
}
inline Chips Cents(double Dollars)
{
	return static_cast<Chips>(std::llround(Dollars * 100.0));
}
inline double Dollars(Chips C)
{
	return static_cast<double>(C) / 100.0;
}
inline int TierIndex(int T)
{
	return T < 0 ? 0 : T > 4 ? 4 : T;
}
/** Calendar helpers (WorldCalendar.cpp): year * 12 + month (0-based), the day of the year, the day of the month. */
int MonthKey(int Day);
int DayOfYear(int Day);
int MonthDay(int Day);
/** A calendar year's title events, as LiveCalendar has them: the Grand Circuit's ring events and the Championship's
 *  bracelet events (its Main Event aside), in date order. */
std::vector<LiveEvent> TitleEvents(int Year);

/** The world day of a time. */
inline int DayAt(double Minutes)
{
	return static_cast<int>(std::floor(Minutes / 1440.0));
}
} // namespace sim

/** The simulation: free functions with access to the World's state (friend). */
struct Sim
{
	// ---- the people (World.cpp)
	static void Found(World& W, double Start);
	/** The Embercrest's own regulars (live::RoomLocals), whoever of them a world doesn't have yet. */
	static void RoomLocals(World& W, int Today);
	/**
	 * A fresh start (update 2 §3.4): every record wiped from a world built as an established one, the people kept as they
	 * are (U6): skills, traits, habits, bankrolls and Kast audiences. The world's own books (honors, news, the series and
	 * Night Shift standings) go too.
	 */
	static void Wipe(World& W);
	/** One person's records, wiped (Wipe; a local a fresh world gains on load). Today: the day the books opened. */
	static void WipeRecord(Npc& N, int Today);
	/** The day a world's record books opened (its first day). */
	static int BooksOpened(const World& W) { return sim::DayAt(W.StartedAt()); }
	/**
	 * A fresh world's first weeks (update 2 §3.3 F9): an event family's first result in the new books, once a family, in
	 * its first FirstWinnerDays. Winner: an npc, -1 the player, -2 a name the world doesn't follow (kept, so the family's
	 * first stays its first, but never a headline).
	 */
	static void FirstWinner(World& W, const Pending& P, int Winner, Chips Prize);
	static constexpr int FirstWinnerDays = 60;
	/** A fresh world's board report: the Season leader and the biggest riser, the first seven mornings and the Mondays to
	 *  week four (F9). Called at the day's turn, before the ranks move. */
	static void BoardReport(World& W, int Day);
	/** The board report's riser is someone on the rows RiverLine's boards list (its top 60), never a climb to No. 800. */
	static constexpr int BoardReportRows = 60;
	/** A fresh world's team signs only players with results (F3): an online final table on record or a top-100 Season rank
	 *  (SeasonRank: BoardRanks' Season ranks at the signing). */
	static bool SignsOnResults(const Npc& N, const std::vector<int>& SeasonRank);
	/** A fresh world's Team RiverLine signing keeps what it signed on in its Value: the Season rank at the signing (1 to
	 *  100, else 0) plus SigningFinalTables times the online final tables on record. Never in Amount: the news and the
	 *  ticker read a world event's Amount as money. (The board report keeps its numbers in What for the same reason.) */
	static constexpr int SigningFinalTables = 1000;
	/** Where everyone stands on a board at a time, from 1 (0: nothing on it), ties by id: the arrows' ranks (Ranks). */
	static std::vector<int> BoardRanks(const World& W, net::Board B, double At);
	static int Add(World& W, Npc&& N);
	static std::string UniqueName(World& W, Rng& R, const std::string& Country, bool Pro);
	static void MakeSkills(Npc& N, Rng& R, double Base);
	static Npc Rookie(World& W, Rng& R, int Day);
	static Npc Discover(World& W, const std::string& Name, const std::string& Country, double Strength, int Day, uint32_t Salt);
	/** A buy-in they're comfortable with today (cents). */
	static Chips Comfort(const Npc& N);
	static double Need(const Npc& N); // buy-ins they want behind them
	static int TierFor(Chips Abi);
	/** Online events on a normal day for them (what their fatigue is measured against). */
	static double Habit(const Npc& N);

	// ---- the days (WorldSim.cpp)
	static void PlanDay(World& W, int Day);
	static Pending OnlineEvent(World& W, const net::EventInstance& E);
	static Pending LiveEventOf(World& W, const LiveEvent& E);
	static void Resolve(World& W, Pending& P);
	static void NewDay(World& W, int Day);
	static void NewWeek(World& W, int Day);
	static void NewMonth(World& W, int Day);
	static void NewYear(World& W, int Calendar, int Day);
	static double Score(const Npc& N, int Kinds);
	static void Post(World& W, double At, EventKind K, int Who, const std::string& What, Chips Amount = 0, int Value = 0, int Other = -1);
	/** A step on someone's journey (their card keeps the first few and the latest). */
	static void Mark(Npc& N, int Day, StepKind K, const std::string& What, int Place = 0, int Of = 0, Chips Amount = 0);
	/** The bracelet or ring an event's winner takes home. */
	static Award AwardOf(const Pending& P, int Day, Chips Prize);

	// ---- the stats pages (WorldStats.cpp)
	static TrackStake StakeOf(const Pending& P);
	static TrackFormat FormatOf(const Pending& P);
	/** A regular's tracked history before the story began, from their lifetime numbers (once; not for the faded). */
	static void SeedStats(Npc& N);
	static void Reputation(Npc& N);
	static void SetMood(Npc& N, Momentum M, int Day);
	static Year& YearRow(Npc& N, int Day);
	static void RefreshField(World& W);
	static void Ranks(World& W);
	static bool Major(const Pending& P);

	static void Apply(World& W, Npc& N, const Pending& P, const Entry& E, int Place, Chips Prize, bool Seat);
	static void PlanTrips(World& W, int Day, Rng& R);
	static void Finances(World& W, Npc& N, int Day, Rng& R);
	static void Stakes(World& W, Npc& N, int Day, Rng& R);
	static void Form(Npc& N, int Day);
	static void Learn(World& W, Npc& N, int Day);
	static void Broadcast(World& W, Npc& N, int Day, Rng& R);
	static void Lifecycle(World& W, Npc& N, int Day, Rng& R);
	static void Connect(World& W, int Day, Rng& R);
	static void Newcomers(World& W, int Day, Rng& R);
	static void TeamRiverLine(World& W, int Day);
	static void Bind(World& W, int A, int B, TieKind K, float Strength, int Day);
	static void TurnPro(World& W, Npc& N, int Day, Rng& R);

	// ---- the player (WorldHero.cpp)
	static void Feel(Bond& B, MemoryKind K, float Emotion);
	static void Fade(World& W);
	static void Fade(World& W, Npc& N);

	// ---- the badges' calendar (WorldSim.cpp; next_segment_spec §5.3)
	/** A finished entry at the room (its start day): the counts, the visit (once a day) and the month's points. */
	static void NoteRoom(RoomRecord& R, int Day, int Place, int FinalSize, Chips Prize, double Points);
	/** The circuit's standings for a season (Mondays from week badge::CardFromWeek, and the season's last), plus the
	 *  card a ring or bracelet won that season grants. Ranked: whether the week is one the standings post on. */
	static void Standings(World& W, int Year, bool Ranked);
	/** The Verified review on the 1st (VerifiedRecord). */
	static void Review(World& W, int Day);
	/** July 15: the Hall of Fame's class (at most badge::HallClass, scoring badge::HallBar or more). */
	static void HallOfFame(World& W, int Day);
	/** The room's last RoomDaysKept days for its regulars, locals and cast, from their habits (a new world, a version 1 one). */
	static void SeedRoomHistory(World& W, int Today);
	/** Bracelets and rings won before the story began, by the founding regulars who travel (§11 Q23). */
	static void SeedChampions(World& W);
	/** What Team RiverLine pays one of its ten a week (sponsor::NpcWeekly("riverline"); $1,500 until the catalog lands). */
	static Chips TeamWeekly();
	/** A world saved at an older version, brought up to this one (WorldSave.cpp; once, in Finish). */
	static void Migrate(World& W, int From);

	// ---- the rival sites (WorldSim.cpp; update 2 §5.9, G3)
	/** A site's bit in Npc::Sites. */
	static int SiteBit(net::SiteId S) { return 1 << static_cast<int>(S); }
	/** The site an event is on (its id's prefix; live events are nobody's). */
	static net::SiteId SiteOf(const Pending& P);
	/** A world event's flags for something that happened on a site: the site in FlagSiteBits (RiverLine's: none). */
	static int SiteFlags(net::SiteId S) { return S == net::SiteId::RiverLine ? 0 : (static_cast<int>(S) << FlagSiteShift) & FlagSiteBits; }
	/** Whether a rival is registered (its SiteDef in net::Sites()); RiverLine always is. */
	static bool Registered(net::SiteId S);
	/** The day a rival opens its lobby in this world (its launch, as this world keeps it), -1 for none. */
	static int LaunchDay(const World& W, net::SiteId S);
	/** The sites with a lobby open on Day, as bits: RiverLine's always, a registered rival's from its launch day. */
	static int OpenOn(const World& W, int Day);
	/**
	 * Someone's accounts as §5.9's rule has them on Day, from their own fork ("world/<seed>/sites/<name>", so no other
	 * draw moves): RunItTwice and Halcyon by their stakes, Ignite's joiners over its first month and its share of the
	 * arrivals since. RiverLine's bit for everyone but Ignite's own arrivals; nothing but RiverLine's in a world the sites
	 * haven't opened in (an older save before its migration). Nothing about the teams (SiteTeams).
	 */
	static int SeedSites(const World& W, const Npc& N, int Day);
	/** Night One and a world brought up to version 4: every launch day this world doesn't have yet, and everyone's accounts as
	 *  of Today (Load: a past launch other than the first night's moves to the first day after loading, §6.5). Npc::Sites is
	 *  where someone plays: a team pro's is the team's site alone ("plays only on", §1.9 #11). */
	static void OpenSites(World& W, int Today, bool Load);
	/** NewDay: a rival's launch (its news), and Ignite's joiners for the day after (tomorrow's registrations). */
	static void SitesDay(World& W, int Day, bool News = true);
	/**
	 * ss.Site.Launch (testing, §6.6; Session::DebugWorld's "sitelaunch"): a rival not yet open in this world opens this minute,
	 * with its news and, for Ignite, its first month of joiners from today; nothing before this minute (no history). False for
	 * RiverLine, a site not registered, or one already open.
	 */
	static bool LaunchNow(World& W, net::SiteId S);
	/** NewMonth: the people who rose to RunItTwice's stakes open their accounts. */
	static void SitesMonth(World& W, int Day);
	/**
	 * Team RunItTwice (§5.9): leaves as Team RiverLine's do; signs the best-known members on the 1st (a fresh world: on
	 * results, from the 1st after its first month). Founding: an established world's first night, its team already signed
	 * (no news, as Team RiverLine's ten have none).
	 */
	static void SiteTeams(World& W, int Day, bool Founding = false);
	/** A finish on a rival: its sparse record and its running race (Apply). */
	static void SiteResult(World& W, Npc& N, const Pending& P, int Place, Chips Prize, double Points);
	/** A rival's running points race: RunItTwice's series, Ignite's month ("" when the event doesn't count). */
	static std::string RaceKey(net::SiteId S, const Pending& P);
	static std::string MonthRace(int Day);
	/** A board's value on a site (World::BoardValue by site): RiverLine's is the whole record less the rivals'. */
	static double SiteValue(const World& W, net::SiteId S, net::Board B, int Npc, double At);
	/** Where everyone stands on a site's board (BoardRanks by site). */
	static std::vector<int> BoardRanks(const World& W, net::SiteId S, net::Board B, double At);
	/** A rival's boards that keep yesterday's ranks (SiteRanksYesterday). */
	static const std::vector<net::Board>& SiteBoards();
	/** The new year: the rivals' seasons start again (lifetime money stays). */
	static void SitesNewYear(World& W);
};
} // namespace world
} // namespace ss
