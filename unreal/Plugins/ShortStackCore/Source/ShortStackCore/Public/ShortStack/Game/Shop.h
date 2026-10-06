#pragma once

#include "ShortStack/Common.h"
#include "ShortStack/Game/Gear.h"
#include "ShortStack/Game/Hero.h"

#include <cstdint>
#include <set>
#include <string>
#include <vector>

namespace ss
{
struct SaveData;
namespace life
{
struct State;
}
namespace world
{
struct Award;
}

/**
 * Northside's shops (the realism update's spec, §5 and §7.1): the venues on the block and their real hours, what each
 * sells (cuts, garments, meals, workouts), the visits the player has made, and Northside Pawn & Loan's book (§5.8).
 * Pure data and rules; the Session charges, skips and settles (SessionShops.cpp, §7.2). Money in the ledger is kind 9.
 *
 * Phase A: the contract only. The bodies return defaults (no venues, no offers, nothing open) until core-shops'
 * Phase B fills them; consumers (core-ui, people-core, ue-frame) build against these declarations from day 1.
 */
namespace shop
{
/** One opening window (life::Activity's semantics): minutes after midnight, Closes <= Opens wraps past midnight,
 *  Days bit 0 Monday .. bit 6 Sunday by the day the window opens. */
struct Window
{
	int Opens = 0;
	int Closes = 1440;
	int Days = 0x7f;
};

enum class Kind : int
{
	Store,
	Barber,
	Thrift,
	Gym,
	Diner,
	Laundry,
	Pawn,
	Display,
};

struct Venue
{
	std::string Id;      // "barber"
	std::string Name;    // "Fifth St. Barbers"
	std::string Address; // "1818 Fifth St."
	Kind Type = Kind::Display;
	bool Enterable = false;
	std::vector<Window> Open;
	std::vector<Window> Staffed; // empty: the same as Open
	bool MembersAnyHour = false; // the gym's fob
	uint32_t Color = 0;
};
SHORTSTACKCORE_API const std::vector<Venue>& Venues(); // §5.0's table plus every display shop of §2.3
SHORTSTACKCORE_API const Venue* FindVenue(const std::string& Id);
SHORTSTACKCORE_API bool IsOpen(const Venue& V, double World);
SHORTSTACKCORE_API bool IsStaffed(const Venue& V, double World);
SHORTSTACKCORE_API double NextOpen(const Venue& V, double World);        // World itself when open
SHORTSTACKCORE_API double ClosesAt(const Venue& V, double World);        // the end of the current window (World when shut)
SHORTSTACKCORE_API std::string HoursLine(const Venue& V, double World);  // "Open until 10 PM" / "Opens Tue 10 AM" / "Open 24 hours"
SHORTSTACKCORE_API std::string HoursSign(const Venue& V);                // the door decal, §4.9

enum class OfferKind : int
{
	Cut,
	Beard,
	Color,
	Brows,
	Garment,
	Meal,
	Drink,
	DayPass,
	Membership,
	Workout,
	Training,
	Shake,
};

struct Offer
{
	std::string Id;    // "barber/crew", "diner/short-stack", "gym/treadmill"
	std::string Venue;
	std::string Tab;   // "CUT", "BEARD", "COLOR", "BROWS", "MENU", "WORKOUTS", "DESK"
	std::string Name;
	std::string Blurb;
	OfferKind Type = OfferKind::Meal;
	Chips PriceCents = 0; // before tax
	bool Taxed = false;
	int Minutes = 0;
	int LookSlot = -1;  // hero::Slot a service sets (Hair, FacialHair, HairColor, Brows)
	int LookValue = -1; // -1 keeps the value (a line-up)
	double Hunger = 0.0, Thirst = 0.0, Energy = 0.0; // relief, as store::Item (negative makes it worse: workouts)
	double FitnessBase = 0.0;
	bool StaffedOnly = false;
	bool MembersFree = false;
};
SHORTSTACKCORE_API const std::vector<Offer>& Offers(const std::string& Venue);
SHORTSTACKCORE_API const Offer* FindOffer(const std::string& Id);

/** The player's history with a venue (life::State::Visits, by venue id). */
struct Visit
{
	int Count = 0;
	double LastAt = -1.0;
	Chips SpentCents = 0;
	std::string Usual;
	int Tips = 0;
	int Stiffs = 0;
};

// ---- Northside Pawn & Loan (§5.8). Shop.h forward-declares world::Award and includes Gear.h and Hero.h. ----
enum class LotKind : int
{
	Gear,
	Memento,
	Award,
	Keepsake,
	Garment,
	Curio,
};

/** One thing Augie will look at or has for sale. Ids: "gear:tower-mid", "memento:cup", "award:<day>:<event>",
 *  "keepsake:<background key>", "garment:<garment id>", "curio:aviators". */
struct Lot
{
	std::string Id;
	LotKind Kind = LotKind::Gear;
	std::string Name;
	Chips ValueCents = 0; // V, his resale price, whole dollars
	Chips SellCents = 0;  // 50% of V, rounded down to whole dollars
	Chips LoanCents = 0;  // 40% of V, rounded down to whole dollars
	bool Stored = false;  // gear in Ninth St. Storage: selling or pawning it adds a 60-minute skip
	bool Final = false;   // can't be bought again anywhere (an award, the keepsake, the cup): SELL asks twice
	std::string Refused;  // "" when he'll take it; else his line (Dee's chip)
};

struct Ticket
{
	int Number = 0;
	std::string Lot;
	Chips LoanCents = 0;
	double TakenAt = 0.0;
	double DueAt = 0.0;
	int Renewals = 0;
};

struct ShelfLot
{
	std::string Lot;
	Chips PriceCents = 0;
	double From = 0.0;
	double Until = 0.0;
	bool Auction = false;
};

struct PawnBook
{
	std::vector<Ticket> Tickets; // open loans
	std::vector<ShelfLot> Shelf; // the hero's sold gear, forfeits and storage-auction lots, for sale From..Until
	std::set<std::string> Away;  // the hero's things gone for good: sold, or forfeited and then sold on
	int NextTicket = 412;
};

constexpr double PawnTermDays = 30.0, PawnGraceDays = 7.0, PawnShelfDays = 21.0;
SHORTSTACKCORE_API Chips GearValue(const gear::Item& I);    // 55% of GearDrop's price
SHORTSTACKCORE_API Chips AwardValue(const world::Award& A); // §5.8's bracelet and ring rule
SHORTSTACKCORE_API Lot KeepsakeOf(hero::Background B);      // §5.8's table
SHORTSTACKCORE_API Chips RedeemCents(const Ticket& T, double World); // + 20% in term, + 25% in grace, 0 once forfeited
SHORTSTACKCORE_API Chips RenewCents(const Ticket& T);                // the 20%
/** Monday's used gear (4-8 lots), only gear the host can draw on the shelf (a bit per gear::Catalog index). */
SHORTSTACKCORE_API std::vector<ShelfLot> UsedStock(int Week, uint64_t Seed, uint64_t GearKinds);
/** The storage auction's 1-3 lots (seeded among what was lost), on the shelf 2 days later for 21. */
SHORTSTACKCORE_API void Consign(PawnBook& B, const std::vector<std::string>& LostGearIds, double World, uint64_t Seed);

// ---- Save lines (§7.3) ----
/**
 * The closet, fit, groom, visit, pawn and meal lines for the save ("closet\tseeded", "closet\town\t<id>", ...), appended
 * to Out, tab-separated, one per line. SaveData::Serialize calls it (it may run off the game thread); the town's lines
 * are town::WriteLines'. It escapes its own strings: Session.cpp's Escape is private to that file. A save this build
 * writes must read back to the same text (shops_test and session_test check it).
 */
SHORTSTACKCORE_API void WriteLines(const life::State& Life, std::string& Out);
/**
 * Reads one save line split on tabs (Fields[0] is its key; a blank line is one empty field) into Life, and marks the
 * line's key in Life.ShopLinesRead; false when it isn't one of WriteLines' (Parse then offers it to town::ReadLine).
 */
SHORTSTACKCORE_API bool ReadLine(const std::vector<std::string>& Fields, life::State& Life);
/**
 * After a save is read (SaveData::Parse, beside the eviction migration): seeds what an older save didn't have (the
 * closet from the creator's look, fitness for an existing gym member, the store's visits from the ledger, grooming
 * and an empty pawn book), §7.3's migration. It goes by Data.Life.ShopLinesRead, never by values left at their
 * defaults, so it runs once per career: what it seeds is written, and the next load reads it back.
 */
SHORTSTACKCORE_API void AfterParse(SaveData& Data);
} // namespace shop
} // namespace ss
