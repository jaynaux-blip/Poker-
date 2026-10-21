#pragma once

#include "ShortStack/Common.h"
#include "ShortStack/Game/SiteData.h"

#include <array>
#include <cstdint>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
struct HistoryEntry;
namespace life
{
struct State;
}
namespace sponsor
{
struct Book;
}
namespace world
{
class World;
}

/**
 * Update 3 (SPEC §2.6, §3.6, §1.4, §7.3): the rival sites' loops. Ignite's Sparks, the Prize Counter, the Flame, Dares, the
 * Spark Wheel, Crackers, the Wristband, Ignite Levels, looks, the Fuse record, Beef and the Sticker Book; RunItTwice's
 * Watchlist, the League, Callouts, H2H, Team Scalps, the Luck Ledger and credentials; and the Value Governor that pays every
 * cash-valued grant on either site (at most 50% of the 30-day rake, the Guest List for the rest). Every outcome is core's,
 * keyed by a hash or a last-day integer; nothing is created until a paid entry or a Clicked claim (S10).
 *
 * The structs below are embedded in life::State (Life.h) and freeze with it. Saved as site\t... sub-tags (§7.4) once
 * Update3LinesRead's U3SitePlay bit is set. Declared by the lead in Phase A; bodies by core-siteplay (SitePlay.cpp,
 * SitePlayIgnite.cpp, SitePlayRit.cpp). Dormant until G2.
 */
namespace site
{
namespace play
{
/** What a governed grant is (the Governor's ledger and the Guest List). Saved: append-only. */
enum class Kind : uint8_t
{
	BaseSparks,
	DareSparks,
	WheelSparks,
	CrackerSparks,
	OuchSparks,
	PaybackSparks,
	ScalpSparks,
	Ticket,
	WalletPromo,
	SecondRun,
	Welcome,
	LeagueTicket,
	ScalpTicket,
	Count,
};

/** One site's 30-slot ring of (rake, promo) cents by 6 AM world day, and the week's stake-band cap (§1.4). */
struct GovRing
{
	int Day0 = -1;
	std::array<int32_t, 30> Rake{}, Promo{};
	int WeekKey = -1;
	int32_t WeekFace = 0;
	int32_t Banked = 0; // base Sparks held for Monday (their Room reserved at once)
};

/** A grant waiting on the Guest List: <= 12 items, one "Sparks owed" line and 3 reset slots (§1.4). */
struct Hold
{
	std::string Key, What;
	Kind K = Kind::Ticket;
	Chips Cents = 0, Covered = 0;
	int Day = 0;
	bool SparksOwed = false, Reset = false;
};

/** Ignite's currency (F1): 10 Sparks a cent of paid rake, the week's band cap and the Monday bank, the wheel's day. */
struct Spark
{
	int64_t Balance = 0, BaseLife = 0, BaseMonth = 0, SpentLife = 0;
	int MonthKey = -1;
	int WeekKey = -1;
	Chips WeekFaceCents = 0;
	int64_t Deferred = 0; // banked for Monday
	int DayKey = -1;
	Chips DayRakeCents = 0;
	int DayEntries = 0;
	int WheelDay = -1;
	int8_t WheelRing = 0;
	int OuchDay = -1, OuchCount = 0;
};

/** The Flame (F2): a daily streak with two rest days a week and a Spare Match per 5-day run. */
struct Flame
{
	int Streak = 0, Best = 0, LastDay = -1, Matches = 0, RestWeek = -1, IceSince = -1;
	int8_t RestUsed = 0;
	int RunDays = 0;
};

/** Today's three Dares (F3): their ids derive from the day key and the band. */
struct Dares
{
	int DayKey = -1;
	int8_t Band = 0;
	std::array<int16_t, 3> Prog{};
	uint8_t Claimed = 0;
	int WeekKey = -1, WeekClears = 0;
};

/** A Cracker on the shelf (F5): Paper 0, Neon 1, Gold 2; <= 12. */
struct Box
{
	std::string Id;
	int8_t Tier = 0;
	int EarnedAt = 0;
};

/** The looks owned and worn (the Locker). */
struct Looks
{
	std::array<uint32_t, 2> Owned{};
	int8_t Back = 0, Felt = 0, Chips = 0, Frame = 0, Fx = 0;
};

/** A row of the Fuse Wall: a FUSE of the player's, by multiplier. */
struct WallRow
{
	int16_t M = 0;
	int8_t Place = 0;
	Chips Prize = 0;
	int Day = 0;
};

/** A FUSE of x10 and up the player sat in (BUZZ's Big hits). */
struct HitRow
{
	int16_t M = 0;
	std::string Winner;
	Chips Prize = 0;
	int Day = 0;
};

/** FUSE's record (F7): the tier ordinals count every FUSE entered, paid or on a ticket. */
struct FuseRec
{
	std::array<int, 3> N{}; // every FUSE entered per tier, paid or ticket
	int Played = 0, Won = 0;
	Chips NetCents = 0;
	int BestM = 0;
	std::vector<WallRow> Wall; // <= 10
	std::array<int16_t, 10> Last10{};
	std::vector<HitRow> Hits;             // <= 10, BUZZ's Big hits
	std::array<Chips, 20> RecentBuyIns{}; // BandOf
};

/** Someone who keeps knocking the player out, or the other way round (F8): <= 5. */
struct Beef
{
	std::string Name;
	int TheyKo = 0, YouKo = 0, LastDay = -1;
};

/** The Sticker Book's bits and the counters its secrets read. */
struct Stickers
{
	uint64_t Bits = 0;
	int ScalpWeek = -1, ScalpCount = 0, ScalpLife = 0, GuestUntil = -1;
	bool Cashed100 = false;
};

/** Everything Ignite keeps for the player. */
struct Ignite
{
	Spark S;
	Flame F;
	Dares D;
	std::vector<Box> Boxes;
	int BandMonth = -1;
	uint32_t BandClaimed = 0;
	int LevelClaimed = 1;
	Looks L;
	FuseRec Fuse;
	std::vector<Beef> Beefs;
	Stickers St;
};

/** The player's League month on RunItTwice (R2). */
struct League
{
	int Month = -1;
	int8_t Div = 2, Group = 0;
	bool Placement = true, Leave = false, Bounce = false;
	std::array<int, 19> Npcs{};
	std::vector<std::pair<int, float>> Days; // <= 31
	int TodayKey = -1;
	int8_t TodayCount = 0, Pace = 3; // the first-3 rule (or the group's pace, 1-3)
};

/** A closed League month: <= 12. */
struct LeagueHist
{
	int Month = 0;
	int8_t Div = 0;
	int Rank = 0, Of = 0;
	int8_t Move = 0;
};

/** This week's Callout (R3). */
struct Callout
{
	int Week = -1, Npc = -1;
	float NpcPts0 = 0, HeroPts0 = 0;
	bool ByThem = false;
};

/** Head to head with someone at the player's tables: <= 8. */
struct H2H
{
	std::string Name;
	int W = 0, L = 0, TheyKo = 0, YouKo = 0;
};

/** Team Scalps (R4): the year's six house pros and who the player has knocked out. */
struct Scalps
{
	int Year = 0;
	std::array<int, 6> Set{};
	uint8_t Taken = 0;
	int WeekKey = -1, WeekCount = 0, SetPaidYear = 0, Lifetime = 0;
};

/** The Luck Ledger (R7): all-ins against their equity. */
struct Luck
{
	int AllIns = 0;
	int64_t ExpWinsX1000 = 0;
	int Wins = 0;
	std::array<int, 20> Buckets{}; // exp, won x10
	int SecondWon = 0, SecondOf = 0; // second runs: counted from the real stub only
};

/** Everything RunItTwice keeps for the player. */
struct Rit
{
	std::vector<std::string> Stars; // <= 12
	std::set<std::string> BellKeys;
	League L;
	std::vector<LeagueHist> Hist;
	Callout C;
	std::vector<H2H> H;
	Scalps Sc;
	Luck Lk;
	uint32_t Creds = 0;
};

/** The play book (life::State::SitePlay). */
struct Book
{
	Ignite Ig;
	Rit Rt;
	std::array<GovRing, 3> Gov{};
	std::vector<Hold> Holds; // <= 15
};

/** What the post-hand hook hands the book (OnHand). */
struct HandNote
{
	bool Paid = false, AllIn = false, Won = false, Showdown = false, Own = false;
	double Equity = -1;
	double PotBb = 0;
	std::vector<std::string> Knocked;
	std::string HoleCards;
	int Graded = 0, GoodGrades = 0;
};

/** A finished event on a site, as SiteSettle knows it (OnResult). */
struct SettleFacts
{
	net::SiteId Site{};
	std::string EventId, Template;
	int Place = 0, Entrants = 0;
	Chips Prize = 0, BuyIn = 0;
	double Points = 0;
	bool Final = false, Win = false;
	std::vector<std::string> Knocked;
	std::string BustedBy;
	int Format = 0;
	bool Featured = false, Series = false, Fuse = false;
	int FuseM = 0;
	std::string FuseWinner;
};

/**
 * Everything the G2 pages draw, by value, computed (const) from the book, or from its defaults while the book is empty:
 * balances, the Flame, three Dares with progress and rewards, the wheel's lit rings and "1 more lights SHINE", crackers,
 * the Wristband's 30 tiers, the Level, looks, the Fuse record, Wall and Big hits, Headliners, Beef, stickers, the Guest
 * List (items, Covered, "Sparks owed"), the Monday bonus, the Governor's share, theme of the night, the League table (20
 * rows with the world's scores), the Callout, H2H, scalps, the Luck Ledger, credentials, the Briefing's line, the
 * under-the-clause flags (derived from WouldStrike while the book is empty). core-siteplay appends the fields.
 */
struct View
{
};

// ---- The Governor (§1.4)
/** Pays a cash-valued grant if the ring has Room (true), else holds it on the Guest List (false). Keyed: pays once. */
SHORTSTACKCORE_API bool Grant(Book& B, life::State& L, net::SiteId S, Kind K, const std::string& What, Chips ValueCents, const std::string& Key, int Day, double World);
/** Promo30 / Rake30 on a site: the Statement's and the Fine print's figure. */
SHORTSTACKCORE_API double GivenBack30(const Book& B, net::SiteId S);

// ---- The hooks (Session.cpp calls them; each keeps today's behaviour until G2)
SHORTSTACKCORE_API void OnPaidEntry(Book& B, life::State& L, net::SiteId S, Chips Fee, Chips BuyIn, int Format, const std::string& EventId, double World);
SHORTSTACKCORE_API void OnResult(Book& B, life::State& L, const SettleFacts& F, double World);
SHORTSTACKCORE_API void OnHand(Book& B, net::SiteId S, const HandNote& N, double World);
/** What a step hands back: letters to post, and a League roster the Session gives World::SetLeagueRoster. */
struct StepOut
{
	std::vector<std::pair<std::string, std::string>> Mail;
	bool Roster = false;
	int MonthKey = -1;
	int8_t Div = 0;
	std::vector<int> Npcs;
};
/** Steps an existing book from From to To (idempotent over any span); never seeds one. */
SHORTSTACKCORE_API StepOut Step(Book& B, life::State& L, const site::Book& Sites, const world::World* W, const sponsor::Book& Deals, uint64_t WorldSeed, double From, double To);
/** The League's one scoring rule, shared with the world (core-world calls it in Sim::SiteResult): 0 below the division's floor. */
SHORTSTACKCORE_API double LeagueScore(int Div, const std::string& TemplateId, Chips BuyIn, int WorldDay, double RawPoints, bool Series, bool Deadline);

// ---- The claims (the Session's methods call these; each keyed and idempotent; "" or why not)
SHORTSTACKCORE_API std::string Spin(Book& B, life::State& L, uint64_t WorldSeed, double World);
SHORTSTACKCORE_API std::string Open(Book& B, life::State& L, uint64_t WorldSeed, const std::string& BoxId, double World);
SHORTSTACKCORE_API std::string ClaimDare(Book& B, life::State& L, int Slot, double World);
SHORTSTACKCORE_API std::string ClaimTier(Book& B, life::State& L, int Tier, double World);
SHORTSTACKCORE_API std::string Buy(Book& B, life::State& L, int Item, double World);
SHORTSTACKCORE_API std::string Equip(Book& B, int Slot, int Look);
SHORTSTACKCORE_API std::string Star(Book& B, const std::string& TemplateId, bool On);
SHORTSTACKCORE_API std::string CallOut(Book& B, int Npc, double World);

// ---- Catalogs (static data; house rules 2 and 3 are tested over them)
struct DareDef
{
	int Id;
	int8_t Tier;
	const char* Label;
	int8_t Needs;
	int Kind;
};
SHORTSTACKCORE_API const std::vector<DareDef>& DareCatalog();
/** A ring's wedges at a band: (prize, probability). */
SHORTSTACKCORE_API const std::vector<std::pair<int, double>>& WheelTable(int Ring, int8_t Band);
struct PrizeDef
{
	int Id;
	const char* Label;
	int64_t Sparks;
	std::string TicketId;
	int LookSlot, Look;
};
SHORTSTACKCORE_API const std::vector<PrizeDef>& PrizeCounter();
/** Sparks to reach an Ignite Level: 100 x (n - 1)^2.4. */
SHORTSTACKCORE_API int64_t LevelNeeds(int Level);
/** The band (A, B, C) from the last 20 paid Ignite entries, FUSE's included. */
SHORTSTACKCORE_API int8_t BandOf(const Book& B, const std::vector<HistoryEntry>& History);
SHORTSTACKCORE_API View ViewOf(const Book& B, const life::State& L, const site::Book& Sites, const world::World* W, net::SiteId S, double World);
/** Every sentence the loops say, for the copy lint and the trademark screen. */
SHORTSTACKCORE_API std::vector<std::string> Inventory();
/** The book's site\t sub-tags (pure const; runs on the save worker): nothing while U3SitePlay is unset. */
SHORTSTACKCORE_API void WriteLines(const life::State& L, std::string& Out);
/** One of the book's site\t sub-tags split on tabs; false when it isn't one. */
SHORTSTACKCORE_API bool ReadLine(const std::vector<std::string>& P, life::State& L);
} // namespace play
} // namespace site
} // namespace ss
