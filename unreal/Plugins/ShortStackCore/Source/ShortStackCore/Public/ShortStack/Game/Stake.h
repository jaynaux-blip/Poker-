#pragma once

#include "ShortStack/Common.h"

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace ss
{
struct SaveData;
namespace life
{
struct State;
struct LiveEntry;
struct Funding;
struct Mark;
}
namespace live
{
struct Occurrence;
}
namespace rep
{
struct Public;
}

/**
 * Backing (next_segment_spec §2): event stakes, pieces at a markup, swaps and packages, from the people the player
 * knows, coaches who back them, and Lantern Row Staking. Every dollar moves once, here, keyed by an id and recorded in
 * Book::Done (N2): buy-in back first, 50/50 after makeup, markup priced on a shrunk ROI, makeup that isn't a debt.
 * Listings key by the event (the occurrence or online event id), never the entry: fills are commitments until
 * registration, when the money moves in one line (§1.2). The staked roll is its own pot and never pays rent (N7).
 * Pure functions on the Book; the Session calls them (SessionStake.cpp). Stakes are private (N3): nothing here is
 * public, and badges, sponsors and table talk never read it.
 *
 * Who backs (§2.5): Big Lou (event stakes at most once a week, paused 30 days at -$360 net on the player), Mrs. Park,
 * Sal and Dre (pieces at their price), Mei and Rick (swaps), Dee (10% of the player's first Sunday, at par, "for the
 * lessons"), Lantern Row Staking (FoldEquityFred's stable: the online package with his monthly review), Sam (the
 * Burner's package, makeup a debt if you quit, spelled out before signing) and the world's regulars who respect the
 * player (a 60-day package). Finch School is P2.
 *
 * Money: integer cents; shares: integer permille. Other people's shares round down, so leftover cents stay with the
 * player, who holds the prize (S7). Every draw is seeded from ids ("stake:" + id + ":" + day), so a reload never rerolls.
 *
 * Forward-declares life::State, LiveEntry and SaveData (Shop.h's pattern) so that Life.h can hold a Book.
 */
namespace stake
{
enum class Kind : int
{
	Event,
	Pieces,
	Swap,
	Package,
};
/** None: a person in the world, by name. */
enum class Outfit : int
{
	None,
	LanternRow,
	Sam,
	FinchSchool,
};
/** Pieces and swaps together: real horses keep 20 to 30% of their own action. */
constexpr int MaxSoldPermille = 700;
constexpr double RentReserveDays = 7.0;
constexpr double DueHours = 72.0;
/** Big Lou (§2.5): stakes of $200 or less, one a week, a 30-day pause at -$360 net on the player, 14 days to pay him. */
constexpr Chips LouMaxStakeCents = 20000;
constexpr double LouWeekDays = 7.0;
/** And never more than four in any 30 days (§1.11: his stakes move at most about $250 of EV a month to the player). */
constexpr int LouMonthCap = 4;
constexpr Chips LouPauseAtCents = -36000;
constexpr double LouPauseDays = 30.0;
constexpr double LouGraceDays = 14.0;
/** A package pays for an entry and two re-entries of one event; a fourth bullet is the player's own money (§2.7). */
constexpr int PackageBullets = 3;
/** Lantern Row's door (§2.5, Q36): 500 tracked online events (freerolls excluded) at +5% shrunk ROI or better. */
constexpr int LanternRowEvents = 500;
constexpr double LanternRowFloor = 0.05;
/** How long an offer stands, and what a refused counter leaves (§2.10). */
constexpr double OfferDays = 3.0;
/** Welched: this long past due (Lou: LouGraceDays). */
constexpr double WelchDays = 7.0;

/** The terms of a deal or an offer. Cut: the player's share of the profit (a package) or after the buy-in (a stake). */
struct Terms
{
	int CutPermille = 500;
	int MarkupPermille = 1000;
	Chips CapOnline = 0;
	Chips CapLive = 0;
	Chips Roll = 0;
	int PeriodDays = 30;
	int Volume = 0;
	int DueDays = 3;
	int GhostsPerWeek = 0;
	bool BuyInBack = true;
	bool MakeupIsDebt = false;
};
/** A line in a package's own ledger (the last 40). */
struct Line
{
	double At = 0.0;
	Chips Amount = 0;
	std::string Label;
};
/** Deal::State. */
enum DealState : int
{
	DealOpen = 0,     // an event stake waiting for its event; a package running
	DealPlaying = 1,  // an event stake registered
	DealSettled = 2,  // an event stake settled
	DealEnded = 3,    // a package ended (cut loose, its term up, or tiered out)
	DealWalked = 4,   // the player walked away from the makeup
	DealRefunded = 5, // the event was cancelled and the backer repaid
	DealLapsed = 6,   // the event went by without the player
};
struct Deal
{
	std::string Id;
	Kind K = Kind::Event;
	Outfit By = Outfit::None;
	std::string Backer;
	Terms T;
	double Since = 0.0;
	double NextChop = 0.0;
	Chips RollNow = 0;
	Chips Funded = 0;
	Chips Makeup = 0;
	Chips Debt = 0;
	int Events = 0;
	int Periods = 0;
	int State = DealOpen;
	std::vector<Line> Lines;
	// Event stakes: the occurrence it pays for, when the offer runs out, the stake and the prize; whether the backer
	// was in the field (only people out of it stand at the rail, §1.5).
	std::string EventId;
	double Until = 0.0;
	Chips Stake = 0;
	Chips Prize = 0;
	Chips TheirShare = 0;
	bool InField = false;
	// Packages: the tier (1..3, Lantern Row), losing periods in a row, periods short of the volume, this period's
	// events, the Burner's ghosting count at the period's start, and when the deal ended.
	int Tier = 0;
	int Losing = 0;
	int Missed = 0;
	int PeriodEvents = 0;
	int GhostsAtStart = 0;
	double EndedAt = 0.0;
};
/** An online package entry, one per bullet (at most PackageBullets); a live package entry too (bullet 1). */
struct Fund
{
	std::string EventId;
	std::string DealId;
	int Bullet = 1;
	Chips Cents = 0;
	bool Settled = false;
	std::string Name; // the event's name, for the roll's lines
	double At = 0.0;
};
/**
 * Pieces for sale in an event: the occurrence or online event id, not the entry (§1.2). Plan: drawn once at List(), who
 * buys how much and when (time, buyer, permille; buyer "": someone off the market). Filled: what the plan has filled
 * by the last step. Collected: the money moved at registration (one line); a listing never collected lapses at Closes.
 */
struct Listing
{
	std::string EventId;
	int Ask = 0;
	int Markup = 1000;
	int Filled = 0;
	double Opens = 0.0;
	double Closes = 0.0;
	bool Collected = false;
	std::vector<std::tuple<double, std::string, int>> Plan;
	Chips BuyIn = 0;   // the event's buy-in when listed
	bool Live = true;  // the Embercrest (5% pieces) or RiverLine (1%)
	int Told = 0;      // plan entries already texted about
};
/** Buyer "": the market. Share: what the result paid them (set at settlement). */
struct Piece
{
	std::string EntryId;
	std::string Buyer;
	int Permille = 0;
	Chips Paid = 0;
	bool InField = false;
	Chips Share = 0;
	bool Settled = false;
};
/** Swap::State. */
enum SwapState : int
{
	SwapAgreed = 0,  // a commitment until both register
	SwapLive = 1,    // the player registered
	SwapSettled = 2, // both results in, the difference owed
	SwapLapsed = 3,  // one of them didn't play
};
/** A swap in one event (EntryId: the event id). Net: what the partner owes the player (+) or the player owes them (-). */
struct Swap
{
	std::string EntryId;
	std::string Partner;
	int Permille = 0;
	int State = SwapAgreed;
	Chips Net = 0;
	Chips Mine = -1;   // the player's prize once settled (-1: not yet)
	Chips Theirs = -1; // the partner's (-1: not known yet)
	double Closes = 0.0;
	double Since = 0.0;
};
/** Owed::State. */
enum OwedState : int
{
	OwedOpen = 0,
	OwedPaid = 1,
	OwedLate = 2,
	OwedWelched = 3,
	OwedVoid = 4,
};
struct Owed
{
	std::string Id;
	std::string Who;
	std::string Label;
	Chips Cents = 0;
	double Due = 0.0;
	int State = OwedOpen;
	bool ToYou = false;
	double Since = 0.0; // when it was owed (the result)
	std::string EntryId;
	double PaidAt = 0.0;
	bool Reminded = false;
};
/** Offer::State. */
enum OfferState : int
{
	OfferStanding = 0,
	OfferTaken = 1,
	OfferDeclined = 2,
	OfferWithdrawn = 3,
};
struct Offer
{
	std::string Id;
	Kind K = Kind::Event;
	Outfit By = Outfit::None;
	std::string Backer;
	std::string EntryId; // the event it's for (an occurrence id; "" for a package)
	Terms T;
	double Expires = 0.0;
	int Counters = 0;
	double At = 0.0;
	int Permille = 1000; // the share it's for (Dee's piece: 100; a swap: 50 or 100)
	Chips BuyIn = 0;     // the event's buy-in, or a package's cap
	int State = OfferStanding;
	int Tier = 0;          // a package's tier
	int CounteredDay = -1; // the day of the last counter (one a day)
	int Lever = -1;        // the last counter's lever
	std::string Line;      // what was said when it was made
};
/** Conduct::Marks kinds (the time and the kind). Rep reads 1 to 3 (word of mouth). */
enum MarkKind : int
{
	MarkLate = 1,
	MarkWelched = 2,
	MarkWalked = 3,
	MarkPickedUp = 4,
	MarkRestricted = 5,
	MarkEvicted = 6,
	MarkShot = 7,
	MarkTilt = 8,
	MarkCutLoose = 9,
};
/**
 * The letters the book posts to the shared inbox (life::Letter::Kind, Source "stake", From "Lantern Row Staking"; Fred's
 * review comes from "FoldEquityFred"). Thread is the offer id for an offer and for the answer to a counter, the deal id
 * otherwise. On an offer's letter, Accept is Session::AcceptStake(Thread) and Decline is stake::Decline(Book, Thread,
 * World) followed by a save. Sponsors' letters are sponsor::LetterKind (1 to 11).
 */
enum LetterKind : int
{
	LetterOffer = 20,   // a package offered
	LetterWelcome = 21, // the roll is live
	LetterChop = 22,    // a chop, or a period's end with the makeup carried
	LetterReview = 23,  // Fred's review, with the chop
	LetterEnded = 24,   // the package is over: cut loose, closed with the roll up ("Package closed"), or walked away from
	LetterNote = 25,    // a note: the answer to a counter ("Your counter"), volume, a tier move, chop day coming, a big result
};
struct Conduct
{
	int Late = 0;
	int Welched = 0;
	int Walked = 0;
	int Shots = 0;
	int Tilts = 0;
	int Chops = 0;
	int Honored = 0;
	bool Seeded = false;
	std::vector<std::pair<double, int>> Marks;
};
/** What a Flow is to the person: World::HeroMoney's memory (core-world's MemoryKind of the same name). */
enum class Memory : int
{
	Staked,
	BoughtPiece,
	Swapped,
	PaidUp,
	PaidLate,
	Welched,
	WalkedOut,
};
/** Money and memories to the world, applied once (Session::WorldStep). Cents: to the person (+) or from them (-). */
struct Flow
{
	std::string Id;
	std::string Npc;
	std::string Where;
	Chips Cents = 0;
	int Memory = 0;
	double At = 0.0;
	bool Applied = false;
	bool Early = false; // paid inside a day of the result (World::HeroMoney's Early: Trust +0.06 and Respect +0.02, not +0.04)
};
struct Book
{
	std::vector<Deal> Deals;
	std::vector<Fund> Funds;
	std::vector<Listing> Listings;
	std::vector<Piece> Pieces;
	std::vector<Swap> Swaps;
	std::vector<Owed> Owes;
	std::vector<Offer> Offers;
	Conduct Record;
	std::vector<Flow> ToWorld;
	/** Every movement's key, and when it moved (pruned after 120 days once its record is closed). */
	std::map<std::string, double> Done;
	std::map<std::string, Chips> NetOnYou;      // Lou's running net on the player (§2.5)
	std::map<std::string, double> PausedUntil; // his pause; Mrs. Park's after a late payment; an outfit's door
	/** Sam's thread on the Burner (he never texts the phone, §1.4): when and what, the last 20. */
	std::vector<std::pair<double, std::string>> Burner;
	double SteppedTo = 0.0;      // the calendar's last step (not saved: the session's clock decides)
	double LookedAt = -1.0e9;    // not saved: when the session last asked who'd make an offer
	uint32_t LinesRead = 0;      // not saved: which stake lines the save had (§7.5)
};
struct Regard
{
	float Trust = 0.0f;
	float Respect = 0.0f;
	float Resentment = 0.0f;
	float Familiarity = 0.0f;
	bool Known = false;      // their bond has a label ("Knows you" or better)
	bool FinalTable = false; // they've been at a final table with the player
};
/** Someone in the world who could back an online package (the rule at WorldSim.cpp:2063): a roll, horses, regard. */
struct Patron
{
	std::string Name;
	Chips Roll = 0;
	int Horses = 0;
	bool Respects = false; // "Respects you" or "Trusts you"
};
/** What backers can know (S1), built by the session. */
struct View
{
	const rep::Public* Rec = nullptr;
	std::map<std::string, Regard> Regards;
	Chips Bankroll = 0;
	Chips RentDue = 0;
	double RentDeadline = 0.0;
	int Day = 0;
	double World = 0.0;
	double RoiRank = 0.0;                                     // the online ROI's rank among the regulars (0..1)
	std::map<std::string, Chips> Rolls;                       // the backers' bankrolls
	std::map<std::string, std::vector<std::string>> Playing; // event id -> the people the world has in it
	std::vector<Patron> Patrons;
	int Cashes = 0;            // tournaments cashed, online and live (Sam hires players who cash)
	bool Restricted = false;   // the RiverLine account
	bool SundayPlayed = false; // a finished Embercrest Sunday among the kept entries (Dee's piece is of the first one)
};

// ------------------------------------------------------------------ prices (§2.6)

/** Prior + (Observed - Prior) x n / (n + K), K 600 online and 60 live (n and Observed exclude freerolls, S9). */
SHORTSTACKCORE_API double ShrunkRoi(int Events, double Observed, bool Live, double Prior);
/** The rake at an online buy-in (-FeeOf / buy-in), and a live event's fee over its total. */
SHORTSTACKCORE_API double OnlinePrior(Chips Abi);
SHORTSTACKCORE_API double LivePrior(Chips BuyIn);
/** What buyers believe the player's ROI is, online or live, from the public record. */
SHORTSTACKCORE_API double ExpRoiOf(const View& V, bool Live, Chips BuyIn);
/** The highest markup Buyer ("" the market) would pay, permille rounded to 0.01, clamped to [1.00, the cap]. */
SHORTSTACKCORE_API int Ceiling(const View& V, const Book& B, const std::string& Buyer, bool Live, Chips BuyIn);
/** Why buyers expect what they do, in a line: "31 live events at +44%: they believe +8% of it." */
SHORTSTACKCORE_API std::string CeilingWhy(const View& V, bool Live, Chips BuyIn);
/** The cap on a markup: 1.20 at a local event, 1.35 online, 1.50 at a regional main and up. */
SHORTSTACKCORE_API int MarkupCap(bool Live, Chips BuyIn);
/** Lantern Row's package at a tier (§2.7): 1 is +5% at 35% ($11 cap, $1,100 roll, 120 events), 2 is +8% at 45% ($22, $2,200, 120), 3 is +15% at 50% ($55, $5,500, 100). */
SHORTSTACKCORE_API Terms LanternRowTerms(int Tier);
/** The tier a shrunk online ROI earns (0: under the +5% floor). */
SHORTSTACKCORE_API int LanternRowTier(double ExpRoi);

// ------------------------------------------------------------------ offers and negotiation (§2.5, §2.10)

/** Pure: the offers the people and outfits would make now that the book doesn't hold yet (each id made once). */
SHORTSTACKCORE_API std::vector<Offer> OffersFor(const View& V, const Book& B, double World);
/**
 * A new offer into the book, said the way its maker says things: a text from a person (Texts), a letter from Lantern Row
 * (the inbox), Sam's lines on the Burner (Book::Burner, with the fine print). Each offer is announced once.
 */
SHORTSTACKCORE_API void Announce(Book& B, life::State& L, const Offer& O, double World, std::vector<std::pair<std::string, std::string>>& Texts);
/** Sam's thread on the Burner gets a line (the last 20 kept). */
SHORTSTACKCORE_API void ToBurner(Book& B, double World, const std::string& Line);
/** 1 yes, 0 stands, -1 withdrawn. Drawn once per offer and day. */
SHORTSTACKCORE_API int Answer(const Offer& O, const Terms& Ask, int Day);
/** For rep::Mood: how far inside what they'd take the ask is (positive inside). */
SHORTSTACKCORE_API double Margin(const Offer& O, const Terms& Ask);
/** The terms a counter asks for (Lever 0: cut +5 points, 1: markup -0.05, 2: the cap up a step). */
SHORTSTACKCORE_API Terms CounterTerms(const Offer& O, int Lever);
/** One counter per offer per day: the answer, the offer updated (taken, standing 24 hours, or withdrawn for 7 days). */
SHORTSTACKCORE_API std::string Counter(Book& B, Chips& Bankroll, life::State& L, const std::string& OfferId, int Lever, double World);
SHORTSTACKCORE_API std::string Accept(Book& B, Chips& Bankroll, life::State& L, const Offer& O, double World);
SHORTSTACKCORE_API std::string Decline(Book& B, const std::string& OfferId, double World);

// ------------------------------------------------------------------ who pays (§1.2)

/** Who pays for an entry: a backer's stake, a package's roll, or the player (with what their pieces brought in). */
SHORTSTACKCORE_API life::Funding FundFor(const Book& B, const std::string& EntryId, Chips BuyIn, bool Live, double World);
/**
 * The funding policy (§1.2), in the order the desk and GoToLive try it: a backer's stake (or Lou's standing offer for
 * that event), the sponsors' covered entries (sponsor::CoverOptions: every deal that covers entries), a package's roll,
 * then the player's own money with what the pieces sold on it bring in. Only the sources that exist for the event are
 * listed, each with why it won't pay this one ("" when it will); the player's own money is always last and always ""
 * (what it can afford is CanRegister's, which also refuses anyone else's money that isn't on this list as it stands).
 * Session::FundingOptions wraps it; the card room's desk calls it on its SaveData. Cheap: the Burner's row runs it every
 * frame.
 */
SHORTSTACKCORE_API std::vector<std::pair<life::Funding, std::string>> FundingOptions(const life::State& L, const live::Occurrence& O, double World);
/** The same for an online event: the package's roll (up to its cap and its bullets), then the player's own money. */
SHORTSTACKCORE_API std::vector<std::pair<life::Funding, std::string>> OnlineFundingOptions(const life::State& L, const std::string& EventId, Chips BuyIn, double World);
/**
 * Whether F is a backer's stake the player has already said yes to (an accepted deal), not a standing offer still to
 * take: the desk starts on it, as GoToLive would.
 */
SHORTSTACKCORE_API bool Accepted(const life::Funding& F);

// ------------------------------------------------------------------ money (§1.2, §2.4)

/** The backer's line, or the listing's one pieces line (collected here, §1.2). */
SHORTSTACKCORE_API void OnRegister(Book& B, Chips& Bankroll, life::State& L, const life::LiveEntry& E, double World);
SHORTSTACKCORE_API void OnSettle(Book& B, Chips& Bankroll, life::State& L, const life::LiveEntry& E, Chips Prize, double World);
SHORTSTACKCORE_API void OnRefund(Book& B, Chips& Bankroll, life::State& L, const life::LiveEntry& E, double World);
/**
 * An online registration (Session::RegisterEvent, after the buy-in): a package's roll pays it (its own ledger), a
 * listing's pieces are collected in one line; and the record against the player (a shot, a tilted registration).
 * BankrollBefore: the bankroll before the buy-in; Tilt: the hero's tilt then.
 */
SHORTSTACKCORE_API void OnlineEntered(Book& B, Chips& Bankroll, life::State& L, const std::string& EventId, const std::string& Name, Chips BuyIn,
	const life::Funding& F, Chips BankrollBefore, double Tilt, double World);
SHORTSTACKCORE_API void OnlineSettle(Book& B, Chips& Bankroll, life::State& L, const std::string& EventId, Chips Prize, double World);
/** Ask + swapped <= MaxSoldPermille. "" when listed (Projection says what's expected to fill). */
SHORTSTACKCORE_API std::string List(Book& B, const View& V, const std::string& EventId, int Ask, int Markup, double Opens, double Closes);
/** Cancels the listings on an event that haven't been collected: nobody has paid, so nothing moves. */
SHORTSTACKCORE_API std::string CancelListing(Book& B, const std::string& EventId, double World);
/** What a listing is expected to sell, in a line: "Projected: 35% by Saturday night (Mrs. Park, Big Lou and 2 others)." */
SHORTSTACKCORE_API std::string Projection(const Book& B, const std::string& EventId, double World);
/** Sold on an event so far (the listings' asks, and what's filled by World) and swapped. */
SHORTSTACKCORE_API int AskedOn(const Book& B, const std::string& EventId);
SHORTSTACKCORE_API int FilledOn(const Book& B, const std::string& EventId, double World);
SHORTSTACKCORE_API int SwappedOn(const Book& B, const std::string& EventId);
/** "" when agreed; else the partner's answer. EntryId: the event (a commitment until both register). */
SHORTSTACKCORE_API std::string ProposeSwap(Book& B, const View& V, const std::string& EntryId, const std::string& Partner, int Permille);
/** The partner's result (TheirPrize < 0: they didn't play, the swap lapses); settles once both are in. */
SHORTSTACKCORE_API void SettleSwap(Book& B, Chips& Bankroll, life::State& L, const std::string& EntryId, const std::string& Partner, Chips TheirPrize, double World);
/**
 * The calendar (§1.3), From..To: pieces filled (a text each from the people who buy), listings and swaps lapsing, due
 * dates (auto-pay keeping the rent reserve, late, welched), what people owe the player paid, package chops and reviews
 * (letters to the inbox), results and reminders texted, offers expiring, old records pruned. V: the record, for a
 * package's review (none: the reviews wait for the next step that has one).
 */
SHORTSTACKCORE_API void Step(Book& B, Chips& Bankroll, life::State& L, double From, double To, std::vector<std::pair<std::string, std::string>>& Texts,
	const View* V = nullptr);
SHORTSTACKCORE_API std::string Chop(Book& B, Chips& Bankroll, life::State& L, const std::string& DealId, double World);
SHORTSTACKCORE_API std::string WalkAway(Book& B, Chips& Bankroll, life::State& L, const std::string& DealId, double World);
SHORTSTACKCORE_API std::string Pay(Book& B, Chips& Bankroll, life::State& L, const std::string& OwedId, double World);
/** A pickup, a restriction or an eviction: what the record against the player keeps (§2.9). */
SHORTSTACKCORE_API void NoteConduct(Book& B, int Kind, double World);

// ------------------------------------------------------------------ the room (§1.5)

/** Who, permille: at most 2 faces, only people not still in tonight's field (§1.5). */
SHORTSTACKCORE_API std::vector<std::pair<std::string, int>> RailFor(const Book& B, const std::string& EntryId, const std::set<std::string>& OutTonight);
/**
 * What a backer at the rail says (Moment: a talk::Moment: Bubble, InTheMoney, DoubleUp, FinalTable, Lose, Win, BadBeat,
 * Rail), or texts at a break (Break: a phone line). In their own voice, never a number of chips or a card; "" for
 * nothing. Works without the book (the director has only names): the backer's stake picks the line when it's there.
 */
SHORTSTACKCORE_API std::string RailLine(const Book& B, const std::string& EntryId, const std::string& Who, int Moment, uint32_t Salt);
/**
 * The life::Mark kinds of the book's dates (Marks), so the calendar tells them apart by kind, never by their words.
 * Sponsors' dates are sponsor::MarkKind (20 to 24).
 */
enum DateKind : int
{
	DatePay = 30,    // a payment the player owes ("$119.00 to Mrs. Park")
	DateOwed = 31,   // money owed to the player ("Mei owes you $40.00")
	DateChop = 32,   // a package's chop day
	DateBacked = 33, // an event a backer is putting the player in
	DateSold = 34,   // an event the player sold pieces of
};
/** The dates for the wall calendar and the second screen: payments due, owed to the player, chop days, staked events. */
SHORTSTACKCORE_API std::vector<life::Mark> Marks(const Book& B, double From, double To);

// ------------------------------------------------------------------ what the pages show

/** The open deals: a package's makeup and roll, what the player owes and is owed. */
SHORTSTACKCORE_API Chips StakedRoll(const Book& B);
SHORTSTACKCORE_API Chips MakeupOf(const Book& B);
SHORTSTACKCORE_API Chips YouOwe(const Book& B);
SHORTSTACKCORE_API Chips OwedToYou(const Book& B);
SHORTSTACKCORE_API const Deal* ActivePackage(const Book& B);
/** The terms in one line: "100% of the Embercrest Nightly $120 · buy-in back first, then 50/50 · nothing carried over · one a week." */
SHORTSTACKCORE_API std::string TermsLine(const Offer& O);
SHORTSTACKCORE_API std::string DealLine(const Deal& D);
/** The backer's display name ("Lantern Row Staking", "Big Lou"). */
SHORTSTACKCORE_API std::string BackerName(const Deal& D);
SHORTSTACKCORE_API std::string OfferFrom(const Offer& O);
/** Anything to show: an offer, a deal, a listing, a swap, something owed (the Backing tab stays hidden until then). */
SHORTSTACKCORE_API bool Anything(const Book& B);
/**
 * A line of backer copy in the speaker's voice (Who: "Big Lou", "Mrs. Park", "Sal", "Dre", "Mei", "Rick", "Dee", "Sam",
 * "Lantern Row Staking", or anyone else for the generic backer's), by kind ("offer", "taken", "result.cash", "late"...),
 * one of its variants by Salt, with {Vars} filled. "" when they have nothing for it.
 */
SHORTSTACKCORE_API std::string Speak(const std::string& Who, const std::string& Kind, uint32_t Salt, const std::map<std::string, std::string>& Vars = {});
/** Every line of backer copy, by speaker and kind ("Big Lou/offer"), for the copy lint; letters are LetterInventory's. */
SHORTSTACKCORE_API std::vector<std::pair<std::string, std::string>> Inventory();
SHORTSTACKCORE_API std::vector<std::pair<std::string, std::string>> LetterInventory();

// ------------------------------------------------------------------ the save (§7.4, §7.5)

SHORTSTACKCORE_API void WriteLines(const life::State& L, std::string& Out);
SHORTSTACKCORE_API bool ReadLine(const std::vector<std::string>& Fields, life::State& L);
/**
 * An older save starts with an empty book. Nothing is written on load: the record before this segment (pickups, bans,
 * evictions) stays the life's own, which the public record reads (rep::Of); the book's record keeps what happens from
 * here on (Session::StakeConduct, shots, tilted registrations, late payments). So a save writes back as it was read.
 */
SHORTSTACKCORE_API void AfterParse(SaveData& Data);
} // namespace stake
} // namespace ss
