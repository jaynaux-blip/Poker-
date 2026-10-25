#pragma once

#include "ShortStack/Common.h"
#include "ShortStack/Game/Autoplay.h"
#include "ShortStack/Game/Body.h"
#include "ShortStack/Game/Coach.h"
#include "ShortStack/Game/Hero.h"
#include "ShortStack/Game/Shop.h"
#include "ShortStack/Game/SiteData.h"
#include "ShortStack/Game/SitePlay.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/Stake.h"
#include "ShortStack/Game/Town.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/Game/Wardrobe.h"

#include <functional>
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
	Live, // Burner: the Embercrest's tournaments, up at the resort (the host takes the player there)
	Study, // coaching: a session with a coach (next_segment_spec §2.11; the catalog has none until paid coaching, P2)
};

/** Dee's game: $1/$2 no-limit, bought into from the bankroll. */
constexpr Chips GameMinBuyInCents = 4000;
constexpr Chips GameMaxBuyInCents = 20000;

struct Activity
{
	std::string Id;
	Kind Type = Kind::Job;
	std::string Title;   // "Night cashier"
	std::string Place;   // "Lucky Penny #212", or the contact
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
	Chips CostCents = 0; // what it costs up front (a coaching session); 0 for everything that pays
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
	/**
	 * 0 poker, 1 job, 2 hustle, 3 bills, 4 prizes, 5 gear, 6 streaming, 7 transfers, 8 the corner store, 9 Northside
	 * (the shops on the block: the barber, the thrift, the gym, Ruby's and the pawn; labeled with the shop's name),
	 * 10 backing (stakes, pieces, swaps, a package; next_segment_spec §1.2), 11 sponsors (covers, cuts, bonuses,
	 * stipends), 12 coaching, 13 video (Rerun's monthly payout; update 2 §1.2), 14 poker sites (the rival sites' cashier
	 * fees; deposits and withdrawals themselves are kind 7, moving the player's own money).
	 */
	int Kind = 0;
};
/** The segment's ledger kinds (§1.2, §1.9 #1). */
constexpr int LedgerBacking = 10;
constexpr int LedgerSponsors = 11;
constexpr int LedgerCoaching = 12;
/** Update 2's ledger kinds (§1.2): RiverLine Rewards pays as kind 4 (prizes), like the Night Shift. */
constexpr int LedgerVideo = 13;
constexpr int LedgerSites = 14;

// ------------------------------------------------------------------ the next segment's types (next_segment_spec §7.2)
// The data the segment's features keep on the player's life. Saved by their own lines: livefund, livevoice, liveroom,
// livebadge, liveleader and livepatch after each liveentry (live::WriteEntryLines), life\tliveday, life\tshift and
// life\tlive's 7th and 8th fields (Session.cpp), mail (WriteMail, below), and the features' own (stake::WriteLines,
// sponsor::WriteLines, badge::WriteLines). A save from before the segment parses and writes back byte for byte: these
// lines appear only once a feature has something to keep (§7.5).

/** Who pays for a live entry (§1.2): one source per entry. */
enum class Paid : int
{
	Own,
	Backer,  // a person holding the whole entry (Big Lou's stake)
	Package, // a package's roll (Lantern Row)
	Sponsor, // a covered entry
	Ticket,
};

/**
 * The money behind an entry (§1.2), kept on the entry and saved as livefund. Own: the player's money, as today (an entry
 * without a livefund line). Cents is what the source put in: a backer's or a package's buy-in, a sponsor's cover, or,
 * on the player's own entry, what the pieces they sold brought in at registration (Permille: the share sold).
 */
struct Funding
{
	Paid From = Paid::Own;
	std::string Ref; // the deal or contract id
	std::string Who; // "Big Lou", "Team Embercrest"
	int Permille = 0; // the backer's share of the action
	Chips Cents = 0;
	int CutPct = 0;     // a sponsor's cut of the prize
	bool Fares = false; // the bus fares are covered too
};
/** The player's own money, nothing sold: what an entry without a livefund line was paid with. */
inline bool IsOwnMoney(const Funding& F)
{
	return F.From == Paid::Own && F.Ref.empty() && F.Who.empty() && F.Permille == 0 && F.Cents == 0 && F.CutPct == 0 && !F.Fares;
}

/** How one of the field sounds and what they can honestly talk about (§4), fixed at registration. */
struct LiveVoice
{
	std::string Name;
	int Style = 0; // an ss::Archetype
	float Social = 0.3f;
	float Aggro = 0.5f;
	float Grit = 0.5f;
	int Mood = 0;
	int Is = 0;
	int Age = 40;
	bool Pro = false;
	bool Staked = false;
	float Rivalry = 0.0f; // toward the player
	float Familiarity = 0.0f;
	float Respect = 0.0f;
	std::string Story; // what they can claim (their own world records); empty: nothing
	std::vector<std::pair<std::string, int>> Ties; // name, tie kind
};

/** A row of the month's room leaders (§5) as it stood at registration. */
struct RoomLeader
{
	int Rank = 0;
	std::string Name;
	double Points = 0.0;
	std::string Badge; // badge::Pack
};

/** A date the wall calendar and the second screen show (§1.3): a chop day, a payment due, a review. */
struct Mark
{
	double At = 0.0; // world minutes
	int Kind = 0;
	std::string Label;
	std::string Where;
};

/** One letter in the Mail app's shared inbox (§1.9 #27): brands (sponsor) and staking outfits (stake) post to it. */
struct Letter
{
	double At = 0.0;
	std::string Thread; // the contract or deal id
	std::string Source; // "sponsor" | "stake"
	std::string From;
	std::string Subject;
	std::string Body; // at most 600 characters
	int Kind = 0;
	bool Read = false;
};
/** The inbox keeps the last 80 letters; a body is at most 600 characters (the contract card is drawn from the terms). */
constexpr size_t InboxKeep = 80;
constexpr size_t LetterBodyMax = 600;

/** One day of live play (the record the sixty kept entries can't hold: Live.cpp:515-524), kept for 400 days. */
struct LiveDay
{
	int Day = 0;
	int Events = 0;
	int Cashes = 0;
	int Embercrest = 0;
	int EmbercrestCashes = 0;
	Chips SpentCents = 0;
	Chips WonCents = 0;
};
constexpr int LiveDaysKept = 400;

/**
 * Bits of State::SegmentLinesRead (§7.5): which of the segment's life lines the state carries, read from the save or
 * kept since by the segment's code. Migrations key on them, never on a value at its default (the ShopLinesRead
 * precedent), and Serialize writes a record only once its bit is set, so a save from before the segment writes back
 * byte for byte.
 */
enum SegmentLine : uint32_t
{
	SegLiveWins = 1u << 0,   // life\tlive's 7th field (LiveWins)
	SegLiveRecord = 1u << 1, // life\tlive's 8th field (LiveSpentCents), and with it the life\tliveday lines
	SegShift = 1u << 2,      // life\tshift (LastShiftAt)
	SegMail = 1u << 3,       // mail lines were read
	SegLiveLines = 1u << 4,  // any of livefund, livevoice, liveroom, livebadge, liveleader, livepatch were read
};

/** Someone in a live field the room knows (a regular), or who knows the player, as it stood at registration. */
struct LiveFace
{
	std::string Name;
	std::string Label; // "Embercrest regular", "Knows you", "Holds a grudge"...
	std::string Line;  // what they say sitting down with the player ("" for nothing)
};

/** Someone the world follows who was at the player's table at a live tournament, and what passed between them in the open. */
struct TableMate
{
	enum Saw : int
	{
		SatWith = 1,
		KnockedYouOut = 2,
		YouKnockedOut = 4,
		PotWon = 8,   // the player took a big pot off them
		PotLost = 16, // they took one off the player
	};
	std::string Name; // as the field knows them
	int Seen = 0;     // Saw bits
	Chips Pot = 0;    // the biggest pot between them in tournament chips, to the player (+) or to them (-)
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
	std::string Id;   // the occurrence ("embercrest-nightly@31")
	std::string Name; // "Embercrest Nightly $120"
	int State = Registered;
	Chips PaidCents = 0; // the buy-in, fee included
	Chips FeeCents = 0;
	double RegisteredAt = 0.0;
	int Entrants = 0; // the field, the player included
	/** The people the world registered (name, how they play: an ss::Archetype): the field is fixed at registration. */
	std::vector<std::pair<std::string, int>> Roster;
	/** The champions' board by the desk as it stood that night (newest first: "SAT NIGHTLY\tMei\t221500"). */
	std::vector<std::string> Board;
	/** The faces in the field: the room's regulars and whoever remembers the player. */
	std::vector<LiveFace> Faces;
	/** Who the player sat with that night and what passed between them (the room's report, SaveData::NoteLive). */
	std::vector<TableMate> Table;
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

	// ---- the next segment (§1.2, §1.8, §7.4): each on its own line after the entry's (live::WriteEntryLines)
	/** Who paid (livefund; kept). No line: the player's own money. */
	Funding Fund;
	/** The field's voices as they stood at registration (livevoice; only while Registered: cleared at settle and refund). */
	std::vector<LiveVoice> Voices;
	/** The room director's state (room::Director::Write), saved with the checkpoint (liveroom; written and cleared with it). */
	std::string Room;
	/** The field's badges, name -> badge::Pack (livebadge; only while Registered). */
	std::vector<std::pair<std::string, std::string>> Badges;
	/** The month's room leaders at registration, at most 10 (liveleader; only while Registered). */
	std::vector<RoomLeader> Leaders;
	/**
	 * Who wore which brand, name -> brand id (livepatch): the world's sponsored people (live::DescribeField) only while
	 * Registered; the player's own rows (sponsor::NoteWorn: anyone not in the Roster) stay on a finished entry, since
	 * the sponsor's asks count them.
	 */
	std::vector<std::pair<std::string, std::string>> Patches;
};

/**
 * Rent stages. The first month is $1,225 (already late: the fee is in it) by Friday midnight; after that it's
 * $1,075 a month, due at midnight on October 31 and every thirty days after. A week out, a paid month turns due
 * again and the landlord starts texting. Miss it and there's a final notice: $150 more and three days. Miss that
 * and the locks change: the gear goes into storage (auctioned if the unit isn't paid after a month), the bed is
 * Dee's couch, and the way back is the back rent plus a month up front.
 */
enum class Rent : int
{
	Due,         // the landlord collects at the deadline if the money is there
	Paid,        // until a week before the next deadline
	FinalNotice, // missed: late fee added, three more days
	Evicted,     // RentDueCents is what it takes to move back in
};
constexpr Chips MonthlyRentCents = 107500;
constexpr Chips LateFeeCents = 15000;
constexpr double RentGraceDays = 3.0;
constexpr double RentNoticeDays = 7.0;
/** On Dee's couch: what a night's sleep gives back, and a daily chip-in for groceries. */
constexpr double CouchRest = 0.7;
constexpr Chips CouchChipInCents = 1000;
/** The storage unit the gear goes to: the deposit covers the first thirty days; unpaid after that, it's auctioned. */
constexpr Chips StorageCents = 6000;
constexpr double StorageDays = 30.0;

/** What persists about the player's life (saved with the session). */
struct State
{
	double Energy = 45.0; // 0..100
	double Heat = 0.0;    // 0..100
	/** Needs, 0 (fed, watered) to 100 (starving, parched): they climb with the hours and drag energy down past 70. */
	double Hunger = 35.0;
	double Thirst = 30.0;
	/** What's in the bag from the corner store (item id -> count). */
	std::map<std::string, int> Pantry;
	/** Everything ever bought from the Lucky Penny, at the counter or on the app (item id -> count): Benny's "the usual". */
	std::map<std::string, int> Bought;
	/** Penny Drop orders on their way to the door. */
	struct Delivery
	{
		std::vector<std::pair<std::string, int>> Lines;
		double PlacedAt = 0.0; // world minutes
		double ArriveAt = 0.0;
		Chips PaidCents = 0;
	};
	std::vector<Delivery> Deliveries;
	int Orders = 0;            // Penny Drop orders placed, ever
	double TapAt = -1.0e9;     // the last glass of water from the kitchen tap (world minutes)
	Rent RentStage = Rent::Due;
	Chips RentDueCents = 122500;
	double RentDeadline = 5.0 * 1440.0; // Friday, October 9, midnight
	int RentsPaid = 0;
	int Evictions = 0;
	double EvictedAt = 0.0;   // when the locks last changed (world minutes)
	Chips CouchCents = 0;     // chipped in at Dee's, this stay
	double StorageDue = 0.0;  // when the storage unit renews (0: no unit)
	int Auctions = 0;         // storage units lost to auction
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
	// Live tournaments (the Embercrest).
	int LiveEvents = 0;
	int LiveCashes = 0;
	int LiveBestPlace = 0; // 0: none yet
	Chips LiveWonCents = 0; // prizes, before buy-ins
	std::vector<LiveEntry> LiveEntries; // oldest first (the last sixty)
	// Northside: the shops and the people on the block (Shop.h, Wardrobe.h, Body.h, Town.h; saved by shop::WriteLines
	// and town::WriteLines). Copied whole with the rest of the state wherever the career moves.
	wardrobe::Closet Closet;
	body::Fitness Fit;
	body::Grooming Groom;
	std::map<std::string, shop::Visit> Visits; // by venue id
	std::map<std::string, town::Bond> Town;    // by town person id
	std::map<std::string, int> Said;           // rumours and one-time lines told -> the day told (pruned after 90)
	shop::PawnBook Pawn;                       // tickets, the shelf, what's gone (§5.8)
	double CoffeeUntil = 0.0;                  // Ruby's coffee: the awake drain x0.75 until then (3/h, not 4/h)
	double RealMealAt = -1.0e9;                // a meal of >= 40 hunger relief: Mom's "eat something real" waits 24 h
	/**
	 * Not saved: which of Northside's lines the save being read had (shop::ReadLine sets a bit per line key, its own
	 * choice of bits), so shop::AfterParse migrates only what a save from before the shops lacked (§7.3). A value at its
	 * default can't say that: a new career's untouched fitness would read as "no fit line", and one that joined the gym
	 * before its first workout would load with the old members' Fitness 60 (shops_test's "migration" checks).
	 */
	uint32_t ShopLinesRead = 0;
	/** The character's background at work (not saved here: the session sets it from the character). */
	hero::Perks Perks;

	// ---- the next segment (next_segment_spec §7.2)
	stake::Book Stakes;      // backing: deals, pieces, swaps, what's owed (stake\t... lines, core-stake)
	coach::Notes Study;      // coaching: graded spots, the last review, composure (coach\t... lines, core-stake)
	sponsor::Book Sponsors;  // contracts, the store credit (sponsor\t... lines, core-sponsor)
	/** The Mail app's shared inbox (Post, WriteMail): oldest first, the last InboxKeep. */
	std::vector<Letter> Inbox;
	/** The live record the kept entries can't hold (live::Settle keeps it; rep::Of reads it). */
	int LiveWins = 0;            // titles: finished first, lifetime
	Chips LiveSpentCents = 0;    // lifetime buy-ins and fees of the entries played (a refund was never played)
	std::vector<LiveDay> LiveDays; // oldest first, the last LiveDaysKept days with live play
	/** When the last ShiftLink shift ended (world minutes; -1e9: none known). */
	double LastShiftAt = -1.0e9;
	/** The badges' monthly review (badge\treview, core-badges): the month key it last ran, and Verified held then. */
	int BadgeMonth = -1;
	bool VerifiedHeld = false;
	/** Not saved: SegmentLine bits, which of this segment's life lines the state carries (§7.5). */
	uint32_t SegmentLinesRead = 0;

	// ---- update 2 (update 2 SPEC §6.2): each feature's data, saved by its own lines (autoplay::, video::, site::
	// WriteLines, after the segment's block) once its Update2LinesRead bit is set, so an untouched old save writes back as
	// it was read.
	autoplay::Book Auto; // autoplay\t... lines: the defaults, Form, what each event's plan played, the notes on regulars
	video::Channel Video; // video\t... lines: Rerun
	site::Book Sites;     // site\t... lines: the rival sites' accounts, transfers, invitations, RiverLine Rewards
	/**
	 * Not saved: Update2Line bits. A bit is set when the save had the line, or once this update's code has state to keep
	 * for it: a graded decision of the player's own (autoplay::NoteOwnDecision), opening the panel and any autoplay use set
	 * U2AutoPref; NewCareer sets every bit (the SegShift precedent).
	 */
	uint32_t Update2LinesRead = 0;

	// ---- update 3 (update 3 SPEC §7.2): the rival sites' loops and the Governor (SitePlay.h), saved as site\t sub-tags
	// (spark, flame, dare, box, band, level, look, fuse, beef, stick, star, league, lhist, callout, h2h, scalp, luck, cred,
	// gov, hold) by site::play::WriteLines once U3SitePlay is set. Ledger kinds unchanged (0-14): no new kind, no new pot.
	site::play::Book SitePlay;
	/**
	 * Not saved: Update3Line bits. A bit is set when the save had a line of its family, or when the feature first has state
	 * (the first paid rival entry or Clicked claim; the first Update 3 upload); NewCareer sets every bit (the SegShift
	 * precedent). Never in AfterParse (S10).
	 */
	uint32_t Update3LinesRead = 0;

	SHORTSTACKCORE_API void Record(double At, const std::string& Label, Chips Amount, int Kind);
	int TicketsFor(const std::string& TemplateId) const;
};

/**
 * Bits of State::Update2LinesRead (update 2 §6.2, §6.5): which of the update's lines the state carries. Migrations key
 * on them, never on a value at its default, and each feature writes its lines only once its bit is set.
 */
enum Update2Line : uint32_t
{
	U2AutoPref = 1,   // autoplay\tpref
	U2AutoPlayed = 2, // autoplay\tplayed
	U2Video = 4,      // video\t...
	U2Sites = 8,      // site\t... (accounts, transfers, invitations)
	U2Rewards = 16,   // site\trewards
	U2Notes = 32,     // autoplay\tnote
};

/** Bits of State::Update3LinesRead (update 3 §1.8 S2, §7.2): each feature writes its lines only once its bit is set. */
enum Update3Line : uint32_t
{
	U3SitePlay = 1, // site\t play sub-tags (site::play::WriteLines)
	U3Video = 2,    // video\t Update 3 sub-tags and vid's appended fields (video::WriteLinesU3)
	// The creator update (SPEC_CREATORS C11, §8.1): kast\tmo and the kast\tmem, log and goodwill tails (kast::growth). Set when
	// a kast\tmo line was read, by NewCareer, or at the first Stream::Start under the new model (Session::KastAfterStart);
	// never in AfterParse, by the stream preview, by opening a Kast page or by Offline. Phase A-K: nothing sets it yet.
	U3Kast = 4,
};

/**
 * Posts a letter to the shared inbox (the body cut to LetterBodyMax), keeping the last InboxKeep. Over the keep, the
 * oldest letter that's read or wants no answer goes first; an unread offer or invitation (WantsAnswer) goes only when all
 * of them are (update 2 §1.7).
 */
SHORTSTACKCORE_API void Post(State& L, const Letter& Mail);
/** A letter that wants an answer: a sponsor's offer or renewal, a staking outfit's package, a site's invitation or offer. */
SHORTSTACKCORE_API bool WantsAnswer(const Letter& Mail);
/** The inbox as save lines: "mail\tat\tthread\tsource\tkind\tread\tfrom\tsubject\tbody", oldest first (strings escaped). */
SHORTSTACKCORE_API void WriteMail(const State& L, std::string& Out);
/** Reads one mail line split on tabs (at least 9 fields); false when it isn't one. */
SHORTSTACKCORE_API bool ReadMail(const std::vector<std::string>& Fields, State& L);

/** Hours of being awake (or asleep) added to hunger and thirst. */
constexpr double HungerPerHour = 3.0;
constexpr double ThirstPerHour = 4.2;
constexpr double SleepNeedsRate = 0.35;
/** Energy drained per hour on top of the usual, by how far past 70 hunger and thirst are. */
SHORTSTACKCORE_API double NeedsDrain(const State& L);
/** "Starving", "Hungry", "Peckish", "Fed" (and the same for thirst). */
SHORTSTACKCORE_API const char* HungerWord(double Hunger);
SHORTSTACKCORE_API const char* ThirstWord(double Thirst);

/** The player's situation when choosing (what the catalog needs to know). */
struct Context
{
	double World = 0.0;
	bool InTournament = false;
	Chips Bankroll = 0;
	int Cashes = 0;
	/**
	 * Who would pay for an Embercrest occurrence, for the Burner's Embercrest row (next_segment_spec §1.2: a backer's
	 * stake, a covered entry, a package, or the player's own money). Session::LifeContext gives GoToLive's own choice
	 * (the first of FundingOptions that can); empty: the player's own money.
	 */
	std::function<Funding(const std::string& OccurrenceId)> LiveFunding;
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
