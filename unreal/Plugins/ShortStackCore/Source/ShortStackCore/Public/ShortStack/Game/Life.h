#pragma once

#include "ShortStack/Common.h"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace ss
{
class Rng;

/**
 * Life away from the tables: the shifts, the hustles and the bills that keep a broke grinder in the game
 * long enough to make the rent. Times are world minutes (see net::DayOf): Monday, October 5, 00:00 is 0.
 */
namespace life
{
enum class Kind : int
{
	Job,    // ShiftLink: legal, low pay, no risk
	Hustle, // Burner: Marcus's runs; good money, police heat
	Ghost,  // Burner: playing Sam's RiverLine account; risks the player's own account
	Sleep,
	Game, // Burner: Dee's live game in the laundromat's back room (the host takes the player there)
	Live, // Burner: the Riverside's Sunday tournament (the host takes the player there)
};

/** Dee's game: $1/$2 no-limit, bought into from the bankroll. */
constexpr Chips GameMinBuyInCents = 4000;
constexpr Chips GameMaxBuyInCents = 20000;

struct Activity
{
	std::string Id;
	Kind Type = Kind::Job;
	std::string Title;   // "Night cashier"
	std::string Place;   // "Quik Stop #212", or the contact
	std::string Blurb;
	double Hours = 6.0;
	Chips WageCents = 0; // per hour (jobs)
	Chips PayMin = 0;    // flat pay on top (tips, a cut)
	Chips PayMax = 0;
	double Energy = 0.0; // spent (negative: restored by sleep)
	double Heat = 0.0;   // police attention gained
	double Risk = 0.0;   // base chance it goes wrong
	int Opens = 0;       // start window, minutes after midnight (wraps past midnight)
	int Closes = 1440;
	int Days = 0x7f;     // weekdays it runs (bit 0 Monday .. bit 6 Sunday), by the day its window opens
	uint32_t Color = 0x27d3c3;
};

/** Everything on offer, in app order: ShiftLink jobs, then the Burner contacts, then sleep. */
SHORTSTACKCORE_API const std::vector<Activity>& Catalog();
SHORTSTACKCORE_API const Activity* Find(const std::string& Id);
/** Whether the activity can start at this time of day. */
SHORTSTACKCORE_API bool InWindow(const Activity& A, double World);
/** When the start window next opens (World itself when open now). */
SHORTSTACKCORE_API double NextOpen(const Activity& A, double World);

struct LedgerEntry
{
	double At = 0.0;
	std::string Label;
	Chips Amount = 0;
	int Kind = 0; // 0 poker, 1 job, 2 hustle, 3 bills, 4 prizes
};

/**
 * The player's entry in a live tournament (live::Register): one per occurrence, keyed by its id, so paying,
 * refunding and settling each happen once however often they're asked for.
 */
struct LiveEntry
{
	enum Stage : int
	{
		Registered,
		Finished,
		Refunded,
	};
	std::string Id;   // the occurrence ("riverside-nightly@31")
	std::string Name; // "Riverside Nightly $120"
	int State = Registered;
	Chips PaidCents = 0; // the buy-in, fee included
	Chips FeeCents = 0;
	double RegisteredAt = 0.0;
	int Entrants = 0; // the field, the player included
	/** The people the world registered (name, how they play: an ss::Archetype): the field is fixed at registration. */
	std::vector<std::pair<std::string, int>> Roster;
	/** The champions' board by the desk as it stood that night (newest first: "SAT NIGHTLY\tMei\t221500"). */
	std::vector<std::string> Board;
	bool FareThere = false;
	bool FareHome = false;
	int Place = 0;
	Chips PrizeCents = 0;
	double FinishedAt = 0.0;
	/**
	 * The night so far, for a game closed in the middle of it (cleared once settled): when the player got to the room
	 * (world minutes, 0 until they did), where the tournament stood after the last hand they saw (Tournament::Checkpoint,
	 * empty before the first), the world clock then, and what the room keeps of its own (the host's text).
	 */
	double ArrivedAt = 0.0;
	std::string Checkpoint;
	double CheckpointAt = 0.0;
	std::string CheckpointHost;
};

/** Rent stages. */
enum class Rent : int
{
	Due,         // $1,225 by Friday midnight
	Paid,        // next month's rent is the new deadline
	FinalNotice, // missed: late fee added, three more days
	Evicted,
};

/** What persists about the player's life (saved with the session). */
struct State
{
	double Energy = 45.0; // 0..100
	double Heat = 0.0;    // 0..100
	Rent RentStage = Rent::Due;
	Chips RentDueCents = 122500;
	double RentDeadline = 5.0 * 1440.0; // Friday, October 9, midnight
	int RentsPaid = 0;
	Chips DebtCents = 0;      // owed to Marcus
	double BannedUntil = 0.0; // RiverLine account restricted until
	int Shifts = 0;
	int Runs = 0;   // successful runs for Marcus
	int Busts = 0;  // times picked up
	int Ghosts = 0; // sessions on Sam's account
	int Bans = 0;
	Chips EarnedJobs = 0;
	Chips EarnedHustles = 0;
	std::set<std::string> Unlocks;      // "bounty", "satellite", "sixmax"
	std::map<std::string, int> Tickets; // event template id -> tickets held
	std::set<int> NightsPaid;           // Night Shift payouts (the day of the 6 AM finish)
	std::vector<LedgerEntry> Ledger;    // newest first
	// Dee's game.
	int BackRoomNights = 0;
	Chips BackRoomNetCents = 0;
	/** Tells seen and confirmed at showdown, by "Player/Tell" (2 or more: learned). */
	std::map<std::string, int> Reads;
	// Live tournaments (the Riverside).
	int LiveEvents = 0;
	int LiveCashes = 0;
	int LiveBestPlace = 0; // 0: none yet
	Chips LiveWonCents = 0; // prizes, before buy-ins
	std::vector<LiveEntry> LiveEntries; // oldest first (the last sixty)

	SHORTSTACKCORE_API void Record(double At, const std::string& Label, Chips Amount, int Kind);
	int TicketsFor(const std::string& TemplateId) const;
};

/** The player's situation when choosing (what the catalog needs to know). */
struct Context
{
	double World = 0.0;
	bool InTournament = false;
	Chips Bankroll = 0;
	int Cashes = 0;
};

/** Why an activity can't start now ("" when it can). */
SHORTSTACKCORE_API std::string Blocked(const Activity& A, const State& L, const Context& Ctx);
/** Chance it goes wrong (busted, or the account flagged). */
SHORTSTACKCORE_API double RiskOf(const Activity& A, const State& L);

/** How an activity turned out. Money is the net change (a fine is negative); Session applies it. */
struct Outcome
{
	std::string ActivityId;
	double Start = 0.0;
	double End = 0.0;
	Chips Money = 0;
	double Energy = 0.0; // change
	double Heat = 0.0;   // change
	Chips Debt = 0;      // added
	double BanUntil = 0.0;
	bool Bad = false;
	std::string Title;
	std::string Body;
};
SHORTSTACKCORE_API Outcome Resolve(const Activity& A, const State& L, double Start, Rng& R, Chips Bankroll);

/** Bounty envelopes in mystery bounty events, as multiples of the bounty part of the buy-in. */
SHORTSTACKCORE_API Chips MysteryEnvelope(Chips BountyCents, Rng& R);

/** The Night Shift that contains World (or the one that just ended, in the daytime): its start. */
SHORTSTACKCORE_API double NightShiftStart(double World);
/** 0 at night, 1 in daylight (dawn 4:40 to 6:20, dusk 18:30 to 20:00). */
SHORTSTACKCORE_API double Daylight(double World);
} // namespace life
} // namespace ss
