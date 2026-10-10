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
 * Forward-declares life::State, LiveEntry and SaveData (Shop.h's pattern) so that Life.h can hold a Book.
 *
 * Bodies: core-stake (Stake.cpp). Phase A, day 1: stubs that compile and change nothing.
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
	int State = 0;
	std::vector<Line> Lines;
};
/** An online package entry, one per bullet (at most 3). */
struct Fund
{
	std::string EventId;
	std::string DealId;
	int Bullet = 1;
	Chips Cents = 0;
};
/** Pieces for sale in an event: the occurrence or online event id, not the entry (§1.2). Plan: drawn once at List(). */
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
};
/** Buyer "": the market. */
struct Piece
{
	std::string EntryId;
	std::string Buyer;
	int Permille = 0;
	Chips Paid = 0;
};
struct Swap
{
	std::string EntryId;
	std::string Partner;
	int Permille = 0;
	int State = 0;
	Chips Net = 0;
};
struct Owed
{
	std::string Id;
	std::string Who;
	std::string Label;
	Chips Cents = 0;
	double Due = 0.0;
	int State = 0;
	bool ToYou = false;
};
struct Offer
{
	std::string Id;
	Kind K = Kind::Event;
	Outfit By = Outfit::None;
	std::string Backer;
	std::string EntryId;
	Terms T;
	double Expires = 0.0;
	int Counters = 0;
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
/** Money and memories to the world, applied once (Session::WorldStep). */
struct Flow
{
	std::string Id;
	std::string Npc;
	std::string Where;
	Chips Cents = 0;
	int Memory = 0;
	double At = 0.0;
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
	std::set<std::string> Done;
	std::map<std::string, Chips> NetOnYou;      // Lou's running net on the player (§2.5)
	std::map<std::string, double> PausedUntil; // and his pause
	uint32_t LinesRead = 0;                     // not saved: which stake lines the save had (§7.5)
};
struct Regard
{
	float Trust = 0.0f;
	float Respect = 0.0f;
	float Resentment = 0.0f;
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
};

SHORTSTACKCORE_API double ShrunkRoi(int Events, double Observed, bool Live, double Prior);
SHORTSTACKCORE_API int Ceiling(const View& V, const Book& B, const std::string& Buyer, bool Live, Chips BuyIn);
/** Pure. */
SHORTSTACKCORE_API std::vector<Offer> OffersFor(const View& V, const Book& B, double World);
/** 1 yes, 0 stands, -1 withdrawn. */
SHORTSTACKCORE_API int Answer(const Offer& O, const Terms& Ask, int Day);
/** For rep::Mood. */
SHORTSTACKCORE_API double Margin(const Offer& O, const Terms& Ask);
SHORTSTACKCORE_API std::string Accept(Book& B, Chips& Bankroll, life::State& L, const Offer& O, double World);
SHORTSTACKCORE_API life::Funding FundFor(const Book& B, const std::string& EntryId, Chips BuyIn, bool Live, double World);
/** The backer's line, or the listing's one pieces line (collected here, §1.2). */
SHORTSTACKCORE_API void OnRegister(Book& B, Chips& Bankroll, life::State& L, const life::LiveEntry& E, double World);
SHORTSTACKCORE_API void OnSettle(Book& B, Chips& Bankroll, life::State& L, const life::LiveEntry& E, Chips Prize, double World);
SHORTSTACKCORE_API void OnRefund(Book& B, Chips& Bankroll, life::State& L, const life::LiveEntry& E, double World);
SHORTSTACKCORE_API void OnlineSettle(Book& B, Chips& Bankroll, life::State& L, const std::string& EventId, Chips Prize, double World);
/** Ask + swapped <= MaxSoldPermille. */
SHORTSTACKCORE_API std::string List(Book& B, const View& V, const std::string& EventId, int Ask, int Markup, double Opens, double Closes);
SHORTSTACKCORE_API std::string ProposeSwap(Book& B, const View& V, const std::string& EntryId, const std::string& Partner, int Permille);
SHORTSTACKCORE_API void SettleSwap(Book& B, Chips& Bankroll, life::State& L, const std::string& EntryId, const std::string& Partner, Chips TheirPrize, double World);
SHORTSTACKCORE_API void Step(Book& B, Chips& Bankroll, life::State& L, double From, double To, std::vector<std::pair<std::string, std::string>>& Texts);
SHORTSTACKCORE_API std::string Chop(Book& B, Chips& Bankroll, life::State& L, const std::string& DealId, double World);
SHORTSTACKCORE_API std::string WalkAway(Book& B, Chips& Bankroll, life::State& L, const std::string& DealId, double World);
SHORTSTACKCORE_API std::string Pay(Book& B, Chips& Bankroll, life::State& L, const std::string& OwedId, double World);
/** Who, permille: at most 2 faces, only people not still in tonight's field (§1.5). */
SHORTSTACKCORE_API std::vector<std::pair<std::string, int>> RailFor(const Book& B, const std::string& EntryId, const std::set<std::string>& OutTonight);
SHORTSTACKCORE_API std::string RailLine(const Book& B, const std::string& EntryId, const std::string& Who, int Moment, uint32_t Salt);
SHORTSTACKCORE_API std::vector<life::Mark> Marks(const Book& B, double From, double To);
SHORTSTACKCORE_API void WriteLines(const life::State& L, std::string& Out);
SHORTSTACKCORE_API bool ReadLine(const std::vector<std::string>& Fields, life::State& L);
SHORTSTACKCORE_API void AfterParse(SaveData& Data);
} // namespace stake
} // namespace ss
