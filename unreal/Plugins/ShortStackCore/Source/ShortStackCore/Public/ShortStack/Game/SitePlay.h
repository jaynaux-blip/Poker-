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
	// Appended at G2 (core-siteplay): what Session::SitePlayHand resolves from the hand and the world, so the hook needs
	// neither. Knocked is everyone the player busted this hand (by name); the ids line up with it (-1: not in the world).
	bool Preflop = false;                // the all-in went in before the flop
	bool Lost = false;                   // the player lost the pot they were all in for (a split isn't a loss)
	std::string EventId;                 // the table's event instance
	std::string HandId;                  // "<event>#<table>#<hand>": keys a moment so it's paid once
	std::vector<int> KnockedNpcs;        // Knocked's world ids, in order
	std::vector<std::string> Headliners; // of Knocked: Ignite's Headliners this month
	std::vector<int> TeamPros;           // of KnockedNpcs: Team RunItTwice's pros
	std::string BustedBy;                // the player busted this hand, by this name ("": not busted)
	int BustedByNpc = -1;
	int8_t SecondVerdict = -1;           // RunItTwice's second run alone, read from the real stub (0 lost, 1 part, 2 won); -1 none
	bool WonFirst = false;               // the real board's result, for the second run's count
	std::vector<int> TeamSix;            // Team RunItTwice's pros now (a year's set fills from them)
	Chips BuyIn = 0;                     // the table's buy-in (a scalp's ticket scales with it)
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
 *
 * G2 (core-siteplay): Ignite's half is ui-ignite's proposal (patches/ui-ignite_to_core-siteplay_1.diff) taken whole but for
 * the notes on each field; RunItTwice's half and the Governor's are core-siteplay's. ViewOf fills the asked site's half and
 * its Gov; the other half stays at its defaults. Money in the view is in cents; Sparks are counts.
 */
// ---- The pages' view: Ignite (§2.4, §2.6)
/** One of today's Dares as the pages draw it (F3). */
struct DareView
{
	int Id = -1;       // DareCatalog()'s id; -1: the slot is empty
	int8_t Tier = 0;   // 0 Easy, 1 Medium, 2 Hard
	std::string Label; // the catalog's line ("Play a Flip & Run")
	int Prog = 0, Needs = 1; // progress against its own target (Needs here is the target: 1, 3 or 20)
	int Entries = 1;         // the paid entries today it needs before it can complete (1, 2, 3)
	int64_t Sparks = 0;      // its reward at today's band
	bool Done = false, Claimed = false, Held = false; // Held: the claim waits on the Guest List's "Sparks owed"
};
/** One wedge of a Spark Wheel ring at today's band (its Odds row; the spin lands on one). */
struct WedgeView
{
	std::string Label; // "200 Sparks", "FUSE $1.10 ticket"
	int64_t Sparks = 0;
	std::string TicketId;
	Chips FaceCents = 0;
	double P = 0;
};
/** One of the Wristband's 30 tiers this month (F6). */
struct TierView
{
	int Tier = 0;
	int64_t Needs = 0;  // the month's base Sparks that reach it
	std::string Reward; // "Flip & Run $1.10 ticket", "Paper Cracker", "Spare Match", "Kindling card back", "" (nothing)
	int8_t Kind = 0;    // 0 nothing, 1 ticket, 2 cracker, 3 Spare Match, 4 look, 5 the month's title and billing
	bool Reached = false, Claimed = false, Held = false;
};
/** A Prize Counter item (F1). */
struct PrizeView
{
	int Item = 0; // PrizeCounter()'s index (Session::BuyPrize)
	std::string Label;
	int64_t Sparks = 0;
	std::string TicketId;
	Chips FaceCents = 0;
	int LookSlot = -1, Look = -1;
	int8_t Tab = 0;     // 0 Tickets, 1 Looks, 2 Extras (the Spare Match)
	bool Owned = false; // a look already in the Locker; Spare Matches already at 2
	bool Afford = false;
};
/** A look in the Locker: owned, worn, or how it's had. */
struct LookView
{
	int8_t Slot = 0; // 0 card back, 1 felt, 2 chip set, 3 frame, 4 win effect
	int8_t Look = 0; // its index in the slot (Equip's Look)
	std::string Name, How; // "Sunset"; "Wristband T12", "Prize Counter 25,000"
	uint32_t ColorA = 0, ColorB = 0; // a Wristband month's two theme colours (0: the base design's own)
	int8_t Base = 0;                 // the base design it restyles (its index in the slot's base looks)
	bool Owned = false, Worn = false;
};
/** A sticker in the Sticker Book (F8): the 24 firsts and the 6 secrets (no hint until it's had). */
struct StickerView
{
	int Id = 0;
	std::string Name, How;
	bool Got = false, Secret = false;
};
/** A moment at the table the layer slams once, keyed by its hand: PAYBACK, SCALP, Ouch (F8, F9). UI state, not saved. */
struct MomentView
{
	int8_t Kind = 0; // 0 Payback, 1 Scalp, 2 Ouch
	std::string Name, HandId;
	int64_t Sparks = 0;
	double Equity = 0; // Ouch: the all-in's equity
	double World = 0;
	bool Held = false;
};
/** What a cracker opened (the open's prize card, in shelf order for OPEN ALL). UI state, not saved. */
struct OpenView
{
	std::string BoxId;
	int8_t Tier = 0;   // Paper 0, Neon 1, Gold 2
	std::string Prize; // "+600 Sparks", "FUSE $1.10 ticket", a look's name
	int64_t Sparks = 0;
	std::string TicketId;
	int LookSlot = -1, Look = -1;
	bool Sticker = false, Held = false;
};
/**
 * One row of a Cracker's Odds (F5, W5; G2 fix M2): a prize it can hold and its chance in one Cracker, read from the very
 * tables CrackerContents draws from. Part 0 rows are the Sparks (exactly one of them, chances summing to 1); a Part 1 row is a
 * ticket on top; Part 2 rows are the look pieces on top (each of the six looks' share of LookP).
 */
struct CrackerOddsRow
{
	int8_t Part = 0;
	std::string Label; // "600 Sparks", "FUSE $1.10 ticket", "Neon Grid card back"
	int64_t Sparks = 0;
	std::string TicketId;
	Chips FaceCents = 0;
	int LookSlot = -1, Look = -1;
	double P = 0;
};
/** A Cracker's Odds drawer (F5, W5; G2 fix M2): what it holds and how likely, where it comes from, its average worth. */
struct CrackerOddsView
{
	int8_t Tier = 0;  // Paper 0, Neon 1, Gold 2
	std::string Name; // "Paper Cracker"
	std::string From; // "Clear all three Dares, any level-up, Wristband T5"
	std::vector<CrackerOddsRow> Rows;
	double TicketP = 0;            // the chance it also holds a ticket (Neon 2%, Gold 4%)
	double LookP = 0;              // the chance it also holds a look piece (Neon 30%, Gold always)
	int64_t DuplicateSparks = 500; // a look piece already in the Locker comes as these Sparks instead
	double EvSparks = 0;           // its Sparks on average, a ticket at its Prize Counter price (Paper 162.5, Neon 820, Gold 4,100)
};
/** What the last paid entry did (the results' rewards ticker). UI state, not saved: empty after a reload. */
struct EntryView
{
	std::string EventId;
	int64_t Base = 0, Sparks = 0; // base at 10 a cent of rake; credited after the Flame and the theme night
	double Flame = 1.0, Theme = 1.0;
	bool Banked = false; // past the week's band cap: banked for Monday
	std::array<int8_t, 3> DareTicks{};
	int8_t RingLit = 0; // the wheel ring it lit (0 none, 1 GLOW, 2 SHINE, 3 DAZZLE)
	int LevelFrom = 0, LevelTo = 0, TierFrom = 0, TierTo = 0;
};
/** A Guest List line (§1.4): oldest first, the "Sparks owed" line where it queued. */
struct HoldView
{
	std::string What;
	Chips Cents = 0, Covered = 0;
	Chips RakeLeft = 0; // the rake still to pay before it clears, at today's spare share (0: it clears at the next step)
	int Entries = 0;    // about how many of the site's cheapest paid entries that rake is
	int Day = 0;
	int64_t Sparks = 0; // the "Sparks owed" line's Sparks
	bool SparksOwed = false, Reset = false;
};
/** One month the ring still holds: its rake and what it gave back (the Statement's lines, dated by the earning month). */
struct GovMonth
{
	int MonthKey = -1;
	std::string Name; // "October"
	Chips Rake = 0, Promo = 0;
};
/** The Governor's figure on a site at this instant (the Fine print's and the Statement's, §1.4). */
struct GovView
{
	Chips Rake30 = 0, Promo30 = 0;
	double Share = 0, Ceiling = 0.5; // Promo30 / Rake30 (GivenBack30); the ceiling at this instant
	Chips Room = 0;                  // what the next grant could take now (cents; the senior holds take it first)
	bool Welcome = false;            // inside Ignite's 60-day welcome window (the 75% ceiling)
	Chips WeekFace = 0, WeekCap = 0; // this week's promotional value and the stake band's cap (0: uncapped)
	int64_t MondayBank = 0;          // base Sparks banked for Monday ("Monday bonus: +2,140 Sparks banked")
	std::vector<HoldView> Holds;
	int ItemsFree = 0;               // Guest List slots a claimed ticket can still take (0: a new one waits in place)
	std::vector<GovMonth> Months;    // oldest first
	std::array<Chips, 30> RakeByDay{}; // the ring's rake by day, oldest first (the Club's "this month's rake by day")
	int FirstDay = 0;                // the play day of RakeByDay[0]
	std::string Line;                // "Ignite gave back $3.65 of your $8.72 rake in the last 30 days (42%)."
};
/** Ignite's half of the view (§2.4, §2.6). */
struct IgniteView
{
	bool On = false;      // the loops are live (G2); false keeps G1's states
	bool Started = false; // the book exists (a paid entry or a claim made it)
	bool FuseOn = false;  // FUSE is lit (S-J passed); false keeps FLIP
	bool OnIce = false;   // a clause strikes Ignite: the Flame waits, the Dares grey (derived from WouldStrike while empty)
	int8_t Band = 0;      // 0 A, 1 B, 2 C
	int64_t Sparks = 0;
	// The Flame
	int Streak = 0, Best = 0, Matches = 0;
	double Mult = 1.0;     // the multiplier the next paid entry today gets
	bool LitToday = false;
	bool Nudge = false;    // after 6 PM with no flame day yet (the poker button's flicker, "Your flame needs one entry today")
	int8_t Look = 0;       // 0 none, 1 small (1-2), 2 medium (3-6), 3 big (7-13), 4 roaring (14-29), 5 blue (30 and up)
	std::array<int8_t, 7> Week{}; // Mon..Sun: 0 ahead, 1 lit, 2 rest day, 3 missed, 4 today (unlit), 5 on ice, 6 a Spare Match burnt
	int RestLeft = 2;             // free rest days left this week
	// Dares
	std::array<DareView, 3> Dares{};
	double DaresReset = 0; // the world minute they reset (6 AM)
	int WeekClears = 0;
	int8_t ClearCracker = 0; // what clearing all three drops today (Paper 0, Neon 1)
	bool ClearPaid = false;
	// The Spark Wheel
	int EntriesToday = 0;
	int8_t RingsLit = 0; // 0 .. 3 (GLOW, SHINE, DAZZLE)
	bool SpinReady = false, Spun = false;
	int8_t SpunRing = 0; // the ring the spin used (1..3)
	int SpunWedge = -1;  // its wedge in Rings[SpunRing - 1]
	bool SpunHeld = false;
	int MoreForNext = 0; // paid entries to the next ring (1 or 2; 0 at DAZZLE)
	std::string NearGoal; // "1 more entry lights SHINE", "2 more light DAZZLE", "" at DAZZLE
	std::array<std::vector<WedgeView>, 3> Rings;
	// Crackers
	std::vector<Box> Crackers;
	std::vector<OpenView> Opened; // the last open (UI state)
	// The Wristband and the Level
	int Tier = 0;
	int64_t BandSparks = 0;
	std::string Theme; // "Kindling"
	int DaysLeft = 0;
	std::array<TierView, 30> Tiers{};
	int Level = 1;
	std::string Title; // "Rookie"
	int64_t LevelSparks = 0, LevelFrom = 0, LevelTo = 0;
	// The Prize Counter, the Locker, the Sticker Book
	std::vector<PrizeView> Prizes;
	Looks Worn;
	std::vector<LookView> Locker;
	std::vector<StickerView> Book;
	// FUSE
	FuseRec Fuse;
	std::array<std::string, 3> FuseTicketIds; // Life.Tickets' ids for a FUSE ticket at each tier
	// People
	std::vector<std::string> Headliners;
	std::vector<Beef> Beefs;
	std::string BeefName; // the current Beef ("" none)
	int ScalpLife = 0, ScalpWeek = 0, ScalpCap = 3;
	bool Guest = false;   // billed SPECIAL GUEST this week
	int GuestUntil = -1;  // the play day the billing ends
	bool CrowdKnows = false; // a first Ignite cash of $100 or more: shouts can use the handle
	int OuchToday = 0;
	std::vector<MomentView> Moments; // the last few at the tables, newest last (UI state)
	// The night
	int8_t ThemeNight = 0; // 0 Mon .. 6 Sun
	std::string ThemeName, ThemeTwist; // "FUSE NIGHT", "FUSE x1.5 Sparks"
	EntryView Last;
	int64_t MondayBank = 0; // base Sparks banked for Monday
	// G2 fix M2 (W5): every Cracker's Odds, Paper, Neon and Gold (CrackerOdds(Tier)), for the Odds drawer one click away.
	std::array<CrackerOddsView, 3> CrackerOdds{};
};

// ---- The pages' view: RunItTwice (§3.4, §3.6)
/** A row of the League table (20 rows): the world's recorded days for a member, the book's for the player (W5). */
struct LeagueRowView
{
	int Npc = -1; // -1: the player
	std::string Name, Country;
	bool Hero = false, Team = false;
	int Rank = 0, Move = 0; // overnight move: places up (+) or down (-) since the end of yesterday
	int Days = 0;           // days with a result
	double Points = 0;      // the best 8 days
	std::array<int8_t, 5> Form{}; // newest first: 0 no result, 1 blank, 2 cash, 3 final table
	std::array<int8_t, 3> Path{{-1, -1, -1}}; // the last 3 months' divisions, oldest first (-1: not known)
	int8_t Zone = 0;        // 1 promotion, -1 relegation, 0 safe
	bool Callout = false;   // this week's Callout
	bool CanCall = false;   // CALL is open for this row
	std::array<double, 8> Best{}; // ui-rit: the 8 counting daily bests, highest first (0: an empty slot); the best-8 meter's cells
};
/** A starred template's next instance (the WATCH chips, the mine filter). */
struct WatchView
{
	std::string TemplateId, EventId, Name;
	double Start = 0;
	Chips BuyIn = 0;
	bool Bell = false;  // the desk bell rang for it (keyed in BellKeys)
	double BellAt = -1; // the world minute it rang (the UI plays RitBell once for a fresh one)
};
/** One of the year's six Team pros (R4). */
struct ScalpView
{
	int Npc = -1;
	std::string Name;
	bool Taken = false;
};
/** A credential plate (R7). */
struct CredView
{
	int Id = 0;
	std::string Name;
	bool Got = false;
	int Day = -1;      // ui-rit: the world day it was earned (-1: not known; the book keeps the bits only), engraved on the plate
	std::string Event; // ui-rit: where ("The Nightcap"; "" when it isn't an event's, or not known)
};
/** The Luck Ledger (R7): all-ins against their equity, in 10 buckets. */
struct LuckView
{
	int AllIns = 0, Won = 0;
	double Expected = 0;          // expected wins
	double GapPct = 0;            // (won - expected) / expected, in percent
	std::array<int, 10> Of{}, Wins{}; // by equity decile
	std::array<double, 10> Exp{};     // expected wins in each decile (from the bucket midpoints)
	int SecondWon = 0, SecondOf = 0;
	std::string Line; // "You are running 7.6% below expectation."
};
/** A Statement line (R5): what the site gave back, dated by the month that earned it. */
struct StatementRow
{
	std::string Label, Month; // "Second Run Club", "October"
	Chips Cents = 0;
};
/** RunItTwice's half of the view (§3.4, §3.6). */
struct RitView
{
	bool On = false, Started = false;
	bool OnLeave = false; // a clause strikes RunItTwice now, or did at a step this month (RitClose's leave, with no day played)
	bool Bounty = false;  // signed with Team RunItTwice: "you're the bounty" (scalps off)
	// The League
	bool League = false;  // a group is drawn this month
	bool DrawPending = false; // the first paid entry asked for a draw; the next step draws it
	int Month = -1;
	int8_t Div = 0;
	std::string DivName; // "Steel"
	int8_t Group = 0;
	bool Placement = false, Bounce = false;
	int Rank = 0, Of = 0;
	double Points = 0, BounceBonus = 0;
	int Days = 0;
	double ToPromotion = 0, ToSafety = 0; // points to the promotion line, and above the relegation line (0: there)
	int Up = 3, Down = 4;
	int Matchweek = 1, Matchweeks = 4;
	double DeadlineAt = 0; // the world minute the month closes (the 1st, 6 AM)
	bool DeadlineDay = false;
	std::vector<std::pair<std::string, int>> Matchdays; // the division's fixtures: name, weekday (0 Monday)
	std::string MatchdayTonight;                        // "" or the fixture's name today
	int8_t Pace = 3;
	std::vector<LeagueRowView> Table;
	std::vector<LeagueHist> Hist;
	// Callouts and head to head
	bool CalloutOn = false, CalloutByThem = false, CanCallOut = false;
	int CalloutNpc = -1;
	std::string CalloutName;
	double CalloutHero = 0, CalloutThem = 0; // this week's League points each since the call
	std::vector<H2H> Heads;
	// The Watchlist
	std::vector<std::string> Stars;
	std::vector<WatchView> Watch;
	// Team Scalps
	std::array<ScalpView, 6> Scalps{};
	int Taken = 0, WeekCount = 0, WeekCap = 2, Lifetime = 0;
	bool FullSet = false;
	// The Desk, the Member File
	LuckView Luck;
	std::vector<CredView> Creds;
	std::vector<StatementRow> Statement;
	// The Briefing (R9)
	std::string Briefing;
	// ui-rit (patches/ui-rit_to_core-siteplay_2.diff): what the table's LEAGUE tab, the Desk, the Settlement and RANKINGS'
	// PYRAMID need beyond the above. Each empty until filled; the pages hide what's empty (they never estimate it).
	std::vector<std::pair<int, double>> HeroDays;            // the player's daily bests this month (play day, points): the projection
	std::vector<std::string> Counting;                       // the open tables' events that would score if they finished now (the
	                                                         // first Pace a day at the floor: Session::SitePlayView fills it)
	std::vector<std::pair<std::string, double>> Scored;      // this sitting's RunItTwice results (event id) and the points each
	                                                         // scored (0: it didn't count); UI state, empty after a reload
	std::array<int, 5> Members{};                            // RunItTwice members whose tier fits each division (the world's): PYRAMID
	std::vector<std::pair<Chips, std::string>> ScalpLadder;  // R4 as data: (the lowest buy-in, "$11 The Sweat ticket"), ascending
	std::array<std::string, 5> Rewards;                      // what promotion into each division pays ("" for Open)
	// G2 fix L6: what this save has already played (saved, so a relaunch doesn't replay it; SeeLeagueNight, SeeIncomingCallout):
	// the closed month whose League Night played, and the Matchweek (PlayMonth x 8 + (day of month - 1) / 7) whose INCOMING
	// CALLOUT slid in; -1 none.
	int NightSeen = -1;
	int IncomingSeen = -1;
};
struct View
{
	IgniteView Ig;
	GovView Gov; // the site asked for
	RitView Rt;
};

// ---- The loops' clock (§1.1): the 6 AM world day, weeks from Monday 6 AM, months on the 1st at 6 AM.
/** The play day of a world minute: net::DayOf(World - 360). */
SHORTSTACKCORE_API int PlayDay(double World);
/** The play week (Monday 0) of a play day. */
SHORTSTACKCORE_API int PlayWeek(int Day);
/** The calendar month (world::MonthKeyOf) of a play day. */
SHORTSTACKCORE_API int PlayMonth(int Day);
/** The world minute a play day starts (6 AM). */
SHORTSTACKCORE_API double PlayDayStart(int Day);

// ---- Value: the ring and the Guest List count in thousandths of a cent, so a Spark (0.0157 cents) is exact enough.
constexpr int64_t ValuePerCent = 1000;
/** Sparks at Prize Counter face (7,000 for $1.10), in the ring's units. */
SHORTSTACKCORE_API int64_t SparksValue(int64_t Sparks);
/** The ring's rake and promo over the 30 days to Day, and the promo booked on one play day, in cents (IncomeU3's tally). */
SHORTSTACKCORE_API Chips RakeCents30(const Book& B, net::SiteId S);
SHORTSTACKCORE_API Chips PromoCents30(const Book& B, net::SiteId S);
SHORTSTACKCORE_API Chips PromoCentsOn(const Book& B, net::SiteId S, int Day);

// ---- Ignite's helpers the other tracks read
/** The Ignite Level from lifetime base Sparks (GateMet's IgniteLevel path reads IgniteLevel). */
SHORTSTACKCORE_API int LevelOf(int64_t BaseLife);
SHORTSTACKCORE_API int IgniteLevel(const Book& B);
/** A themed event's base Sparks multiplier on its night (F10): 1.5, else 1 (the cabinets' x1.5 sticker). */
SHORTSTACKCORE_API double ThemeMult(const std::string& TemplateId, double Start);
/** Whether an entry in this template at Start can move Dare DareId (the sheet's "counts for"). */
SHORTSTACKCORE_API bool DareMayCount(int DareId, const std::string& TemplateId, double Start);
/** A Box's contents, drawn from its key (§1.9: the same before and after a reload; Open pays exactly this). */
SHORTSTACKCORE_API OpenView CrackerContents(uint64_t WorldSeed, const Box& B);
/** A Cracker's Odds (Paper 0, Neon 1, Gold 2): the tables CrackerContents draws from, as rows (G2 fix M2, W5). Pure. */
SHORTSTACKCORE_API CrackerOddsView CrackerOdds(int Tier);
/** A look's name, how it's had and its colours (the Locker; SitesLookFor draws an equipped one from it). */
SHORTSTACKCORE_API LookView LookOf(int Slot, int Look);
/** How many looks a slot has (0 card back, 1 felt, 2 chip set, 3 frame, 4 win effect). */
SHORTSTACKCORE_API int LookCount(int Slot);
/** This month's Headliners (F8): the six Ignite members with the most followers, then by Ignite points; ids, best first. */
SHORTSTACKCORE_API std::vector<int> Headliners(const world::World* W);

// ---- RunItTwice's helpers (core-world and the pages read them)
/** "Open", "Iron", "Steel", "Gold", "High Bridge". */
SHORTSTACKCORE_API const char* DivisionName(int Div);
/** The division's stake floor in cents: Open 0, Iron 550, Steel 1100, Gold 2200, High Bridge 5500. */
SHORTSTACKCORE_API Chips DivisionFloor(int Div);
/** The Matchday multiplier of a template on a play day for a division (1.5 on its fixtures, else 1). */
SHORTSTACKCORE_API double MatchdayMult(int Div, const std::string& TemplateId, int Day);
/**
 * LeagueBook::Div, as the Session hands it to World::SetLeagueRoster, packs the group's pace with the division (World.h is
 * frozen): Div & 7 is the division, Div >> 3 the "first k" entries a day that count (1-3; 0 reads as 3). core-world scores
 * a member's result only while their TodayCount is under the pace.
 */
inline int LeagueDivOf(int8_t Packed) { return Packed & 7; }
inline int LeaguePaceOf(int8_t Packed) { return (Packed >> 3) >= 1 && (Packed >> 3) <= 3 ? (Packed >> 3) : 3; }
inline int8_t LeaguePack(int Div, int Pace) { return static_cast<int8_t>((Div & 7) | ((Pace >= 1 && Pace <= 3 ? Pace : 3) << 3)); }
/**
 * LeagueMember::Form (core-world writes it, the League page reads it): the last 5 days with a RunItTwice result, newest in
 * bits 0-1, each 1 blank, 2 cash, 3 final table (0: no day there yet). The result's day is PlayDay(the result's minute).
 */
inline int LeagueFormAt(uint16_t Form, int K) { return (Form >> (2 * K)) & 3; }
/** Team RunItTwice's pros in the world (Npc::Sponsor), by id: the six a year's set is taken from. */
SHORTSTACKCORE_API std::vector<int> TeamPros(const world::World* W);
/** The division a first League month places you in, by Abi100: under $5.50 Open ... $55 or more High Bridge. */
SHORTSTACKCORE_API int PlacementDivision(Chips Abi100);
/** The hook with the player's life (Session::SitePlayHand): OnHand plus every grant a hand pays (Ouch, Payback, scalps). */
SHORTSTACKCORE_API void OnHandPaid(Book& B, life::State& L, net::SiteId S, const HandNote& N, double World);
/** A paid entry's marker: true while the entry is the player's own wallet money (OnPaidEntry set it, OnResult clears it). */
SHORTSTACKCORE_API bool PaidEntry(const life::State& L, const std::string& EventId);
/** Every label the loops draw (dares, rings, crackers, looks, stickers, tiers, themes, divisions): no closing stop. */
SHORTSTACKCORE_API std::vector<std::string> InventoryLabels();
/**
 * The taskbar poker button's flame nudge (F2, ui-tablestyle's G2 part), cheap enough for every frame: after 6 PM world time,
 * a started Ignite book with no flame day yet today and not on ice. (IgniteView::Nudge is the same answer.)
 */
SHORTSTACKCORE_API bool FlameNudge(const Book& B, const sponsor::Book& Deals, double World);

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
/**
 * G2 fix L6: the seen marks (RitView::NightSeen, IncomingSeen). League Night for the closed Month, or the incoming Callout of
 * Matchweek Week, has played on this save: one dated site\tdone key each ("play:seen:night:<Month>", "play:seen:call:<Week>"),
 * the older mark of its kind dropped. True when it wrote (the caller saves). Nothing for a save with no RunItTwice book (S10),
 * and nothing when the mark is already there, so a page that calls it every frame writes once.
 */
SHORTSTACKCORE_API bool SeeLeagueNight(life::State& L, int Month, double World);
SHORTSTACKCORE_API bool SeeIncomingCallout(life::State& L, int Week, double World);

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
