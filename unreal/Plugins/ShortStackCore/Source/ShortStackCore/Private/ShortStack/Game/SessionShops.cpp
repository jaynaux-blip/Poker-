#include "ShortStack/Game/Session.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Body.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Shop.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/Store.h"
#include "ShortStack/Game/Town.h"
#include "ShortStack/Game/Wardrobe.h"
#include "ShortStack/Game/World.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <set>

// The Session's Northside seam (the realism update's spec, §7.2): the shops (pay, service, settle, wait, wear, donate,
// board) and Northside Pawn & Loan (§5.8), in their own translation unit so Session.cpp stays closed after Phase A.
//
// How the pieces fit:
//  - Every charge goes through Life.Record as ledger kind 9, labeled with the shop's name; goods and meals are taxed
//    (store::Tax), services, passes, the membership and the pawn's loans and fees aren't. A charge over the bankroll is
//    "Card declined." and costs nothing.
//  - A service (a cut, a workout, a plate at Ruby's) is charged, then played as a TimeSkip "shop:<venue>:<offer ids>"
//    with Rest 1.0; SettleShop lands its effects when it ends. A "shop:" skip is asleep to the calendar (Update passes
//    Awake only for an empty activity id): the needs climb by the hours, and energy moves as the offers say
//    (Outcome.Energy, whole). Waiting (Ruby's "sit a while", the barber's bench, the shelter, the laundromat) is a skip
//    with an empty activity id instead, awake, so the drain (3 an hour with Ruby's coffee) and fitness apply on their own;
//    ShopStep pours Ruby's refills as they come.
//  - A tip at the barber's or Ruby's goes on the same receipt and is reported to Tipped (the keeper on shift): the
//    pages don't call Tipped themselves.
//  - Fitness changes call RefreshGear (BodyFx), as does anything that takes or returns gear.
//  - The keeper on shift sees the player at every paid visit (ShopVisit): a point of familiarity once a day, and the
//    look (Bond::LookSeen), so a new look is remarked on until they've seen it. The Lucky Penny's counter reads the
//    same (ShopClerk: Ray by day, Benny by night, a fresh cut of Lonnie's, a look the clerk hasn't rung up).
//  - The migration (shop::AfterParse) goes by the lines the save had (Life.ShopLinesRead), never by fields at their
//    defaults: a new career's fitness and grooming are still at their defaults when first saved.

namespace ss
{
namespace session_shops_detail
{
constexpr double Day = net::MinutesPerDay;
const char* const Dot = " \xC2\xB7 ";
const char* const Minus = "\xE2\x88\x92";
const char* const Declined = "Card declined.";
const char* const PawnName = "Northside Pawn & Loan";
// The waits (Session::Skip::Label): ShopStep pours Ruby's refills during the first.
const char* const DinerWait = "Sitting a while at Ruby's";
const char* const BarberWait = "On the bench at Fifth St. Barbers";
const char* const ShelterWait = "Waiting for the 14";
const char* const LaundryWait = "Waiting at the Wash & Fold";
const char* const StorageRun = "To Ninth St. Storage and back";

bool IsService(const shop::Offer& O)
{
	return O.Minutes > 0;
}

bool TipVenue(const std::string& Venue)
{
	return Venue == "barber" || Venue == "diner";
}

bool PassToday(const Session& S, double World)
{
	const auto It = S.Life.Visits.find("gym/daypass");
	return It != S.Life.Visits.end() && It->second.Count > 0 && net::DayOf(It->second.LastAt) == net::DayOf(World);
}

/** Brows from thin to full: Thin and Arched (Arched wears Thin's groom), Natural, Thick. */
int BrowRank(int Brows)
{
	return Brows == 2 ? 2 : Brows == 1 ? 1 : 0;
}

/** "A", "A & B", "A, B & C". */
std::string JoinNames(const std::vector<std::string>& Names)
{
	std::string Out;
	for (size_t I = 0; I < Names.size(); ++I)
	{
		Out += (I == 0 ? "" : I + 1 == Names.size() ? " & " : ", ") + Names[I];
	}
	return Out;
}

/** A change on a meter, as UsedLine writes it: "hunger −45" (a real minus sign); "" for none. */
std::string Change(double V, const char* What)
{
	const long N = std::lround(V);
	if (N == 0)
	{
		return std::string();
	}
	return std::string(What) + " " + (N > 0 ? "+" : Minus) + std::to_string(N > 0 ? N : -N);
}

std::string Changes(double Hunger, double Thirst, double Energy)
{
	std::string Out;
	for (const std::string& C : {Change(Hunger, "hunger"), Change(Thirst, "thirst"), Change(Energy, "energy")})
	{
		if (!C.empty())
		{
			Out += (Out.empty() ? "" : ", ") + C;
		}
	}
	if (!Out.empty())
	{
		Out[0] = static_cast<char>(Out[0] - 'a' + 'A');
		Out += ".";
	}
	return Out;
}

/** "35 minutes", "an hour", "two hours", "eight hours". */
std::string Duration(int Minutes)
{
	static const char* const Hours[] = {"two", "three", "four", "five", "six", "seven", "eight"};
	if (Minutes == 60)
	{
		return "an hour";
	}
	if (Minutes > 60 && Minutes % 60 == 0)
	{
		const int H = Minutes / 60;
		return (H <= 8 ? std::string(Hours[H - 2]) : std::to_string(H)) + " hours";
	}
	return std::to_string(Minutes) + " minutes";
}

/** A line on the outcome card starts with a capital ("An hour at IronHouse."). */
std::string Sentence(std::string Text)
{
	if (!Text.empty())
	{
		Text[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(Text[0])));
	}
	return Text;
}

/** An offer as a receipt or the outcome card names it: the Style Book's COLOR and BROWS tabs list bare names ("Auburn",
 *  "Thin"), which say what they are only under their tab. */
std::string Called(const shop::Offer& Of)
{
	switch (Of.Type)
	{
	case shop::OfferKind::Color: return Of.Name + " color";
	case shop::OfferKind::Brows: return Of.Name + " brows";
	default: return Of.Name;
	}
}

Chips WholeDollars(Chips Cents)
{
	return (std::max<Chips>(0, Cents) + 50) / 100 * 100;
}

// ------------------------------------------------------------------ an order

struct Priced
{
	const shop::Venue* V = nullptr;
	std::vector<const shop::Offer*> Offers;
	std::vector<const wardrobe::Garment*> Garments;
	Chips Goods = 0;    // taxed, before tax
	Chips Services = 0; // not taxed
	Chips Discount = 0;
	Chips Tax = 0;
	Chips Tip = 0;
	Chips Total = 0;      // what the card is charged
	Chips Covered = 0;    // Big Lou's share: the food and its tax, when he's picked up the check
	Chips House = 0;      // a sponsor's service on the house (Fifth St. Barbers' cut every 14 days, next_segment_spec §3.6)
	int Minutes = 0;
	bool AnyService = false;
	bool HasPass = false; // a day pass bought with the workout
	bool Lou = false;     // Big Lou's got the check (town::PaysYourCheck): the tip is all the player pays
	std::string Why;      // the order itself is wrong ("" when it's well formed)
};

/** An order priced. Payer: work out who pays (Big Lou's check); ShopBlocked, asking only whether it can be had, doesn't. */
Priced Price(const Session& S, const Session::ShopOrder& O, bool Payer = true)
{
	Priced P;
	P.V = shop::FindVenue(O.Venue);
	if (!P.V)
	{
		P.Why = "Not on offer.";
		return P;
	}
	const double World = S.WorldMinutes();
	std::set<std::string> Seen;
	std::set<std::string> Tabs;
	for (const std::string& Id : O.Offers)
	{
		const shop::Offer* Of = shop::FindOffer(Id);
		if (!Of || Of->Venue != O.Venue)
		{
			P.Why = "Not on offer.";
			return P;
		}
		if (!Seen.insert(Id).second)
		{
			P.Why = "One of each.";
			return P;
		}
		if (O.Venue == "barber" && !Tabs.insert(Of->Tab).second)
		{
			// One cut, one beard, one color and one brow job to a sitting.
			P.Why = "One " + std::string(Of->Tab == "CUT" ? "cut" : Of->Tab == "BEARD" ? "beard" : Of->Tab == "COLOR" ? "color" : "brow job") + " at a time.";
			return P;
		}
		P.Offers.push_back(Of);
		(Of->Taxed ? P.Goods : P.Services) += Of->PriceCents;
		if (IsService(*Of))
		{
			P.AnyService = true;
			// At Ruby's the plates and the drinks come together; the barber's services and the workouts follow each other.
			P.Minutes = O.Venue == "diner" ? std::max(P.Minutes, Of->Minutes) : P.Minutes + Of->Minutes;
		}
		// A day pass or the membership bought with the workout lets the hero onto the floor with it.
		P.HasPass = P.HasPass || Of->Type == shop::OfferKind::DayPass || Of->Type == shop::OfferKind::Membership;
	}
	for (const std::string& Id : O.Garments)
	{
		const wardrobe::Garment* G = wardrobe::Find(Id);
		if (!G || G->Tier != 0 || O.Venue != "thrift")
		{
			P.Why = "Not on the rack.";
			return P;
		}
		if (!Seen.insert(Id).second)
		{
			P.Why = "One of each.";
			return P;
		}
		P.Garments.push_back(G);
		P.Goods += wardrobe::ThriftPrice(*G, World);
	}
	if (P.Offers.empty() && P.Garments.empty())
	{
		P.Why = "Nothing on the ticket.";
		return P;
	}
	// "Cut & beard" together: $5 off the sum.
	if (O.Venue == "barber" && Tabs.count("CUT") > 0 && Tabs.count("BEARD") > 0)
	{
		P.Discount = 500;
	}
	P.Tax = store::Tax(P.Goods);
	// The tip, on what the work cost before tax, to the cent (half up).
	if (TipVenue(O.Venue) && O.TipPercent > 0)
	{
		const Chips Base = std::max<Chips>(0, P.Goods + P.Services - P.Discount);
		P.Tip = (Base * std::min(100, O.TipPercent) + 50) / 100;
	}
	// A sponsor's service on the house (§3.6): the cut rings up at $0. The tip on what it would have cost is still the
	// player's to leave.
	if (O.Venue == "barber" && sponsor::ServiceCovered(S.Life.Sponsors, "barber", World))
	{
		for (const shop::Offer* Of : P.Offers)
		{
			if (Of->Type == shop::OfferKind::Cut && P.House == 0)
			{
				P.House = std::min<Chips>(Of->PriceCents, 3000);
			}
		}
	}
	P.Total = P.Goods + P.Services - P.Discount - P.House + P.Tax + P.Tip;
	// Big Lou on his Friday night, the player cashed lately (§6.3): the food and its tax go on his tab, once a night. The
	// tip is still the player's to leave, worked out on the check as it would have been.
	if (Payer && O.Venue == "diner" && town::PaysYourCheck(S, World) == "big-lou")
	{
		P.Lou = true;
		P.Covered = P.Total - P.Tip;
		P.Total = P.Tip;
	}
	return P;
}

std::string OfferBlocked(const Session& S, const shop::Venue& V, const shop::Offer& Of, double World, bool Pass)
{
	const bool Member = S.Owns("gym");
	if (Of.StaffedOnly && !shop::IsStaffed(V, World))
	{
		return "Nobody at the desk till " + net::TimeLabel(shop::NextOpen(V, World)) + ".";
	}
	const hero::Look& L = S.Person.Appearance;
	switch (Of.Type)
	{
	case shop::OfferKind::Cut:
		if (Of.LookValue >= 0 && body::HairRank(Of.LookValue) > body::HairAllowed(S.Life.Groom, L, World))
		{
			return "Give it three weeks.";
		}
		break;
	case shop::OfferKind::Beard:
		if (body::BeardRank(Of.LookValue) > body::BeardAllowed(S.Life.Groom, L, World))
		{
			return "Give it a few days.";
		}
		break;
	case shop::OfferKind::Brows:
		if (BrowRank(Of.LookValue) >= BrowRank(L.Brows) && Of.LookValue != L.Brows)
		{
			return "Can't put brows back on.";
		}
		if (Of.LookValue == L.Brows)
		{
			return "That's how they are already.";
		}
		break;
	case shop::OfferKind::Workout:
	case shop::OfferKind::Training:
		if (Of.MembersFree && !Member && !Pass)
		{
			return "Day pass or membership first.";
		}
		if (S.Life.Energy < -Of.Energy)
		{
			return "You don't have it in you. Eat or sleep first.";
		}
		break;
	case shop::OfferKind::Membership:
		if (Member)
		{
			return "You're already a member.";
		}
		break;
	case shop::OfferKind::DayPass:
		if (Member)
		{
			return "Members don't need one.";
		}
		if (PassToday(S, World))
		{
			return "You've got today's pass.";
		}
		break;
	case shop::OfferKind::Drink:
		// Ruby's coffee is bottomless: while a cup's going (Life.CoffeeUntil ahead) it isn't rung up again, and its first
		// cup's energy doesn't come twice. The refills are free while the hero sits (WaitAt, ShopStep).
		if (Of.Id == "diner/coffee" && S.Life.CoffeeUntil > World)
		{
			return shop::KeeperOf("diner", World) == "ruby" ? "You've got a cup going, baby. Refills are on the house."
															 : "It's bottomless, hon. Sit a while and I'll keep it coming.";
		}
		break;
	case shop::OfferKind::Color:
	case shop::OfferKind::Garment:
	case shop::OfferKind::Meal:
	case shop::OfferKind::Shake: break;
	}
	return std::string();
}

std::string OrderBlocked(const Session& S, const Priced& P, const Session::ShopOrder& O)
{
	if (!P.Why.empty())
	{
		return P.Why;
	}
	if (S.TimeSkip.Active)
	{
		return "You're busy.";
	}
	if (S.T)
	{
		return "Finish your tournament first.";
	}
	const double World = S.WorldMinutes();
	if (!S.ShopOpenFor(O.Venue))
	{
		return "Closed" + std::string(Dot) + shop::HoursLine(*P.V, World);
	}
	const bool Pass = P.HasPass || PassToday(S, World);
	double Effort = 0.0; // the energy the order's workouts take, together
	bool Joins = false;
	bool Passes = false;
	bool Staffed = false;
	for (const shop::Offer* Of : P.Offers)
	{
		const std::string Why = OfferBlocked(S, *P.V, *Of, World, Pass);
		if (!Why.empty())
		{
			return Why;
		}
		Effort += Of->Type == shop::OfferKind::Workout || Of->Type == shop::OfferKind::Training ? std::max(0.0, -Of->Energy) : 0.0;
		Joins = Joins || Of->Type == shop::OfferKind::Membership;
		Passes = Passes || Of->Type == shop::OfferKind::DayPass;
		Staffed = Staffed || (Of->StaffedOnly && IsService(*Of));
	}
	if (Joins && Passes)
	{
		// Never both on one receipt, whoever calls (the desk's page says the same: GymScreen).
		return "Membership covers today. Skip the pass.";
	}
	if (Effort > S.Life.Energy)
	{
		return "You don't have it in you. Eat or sleep first.";
	}
	for (const wardrobe::Garment* G : P.Garments)
	{
		if (S.Life.Closet.Owned.count(G->Id) > 0)
		{
			return "You already own that.";
		}
	}
	// A service finishes before the door's locked (members on the gym's fob have no closing time), and one with the
	// keeper (Rosa's hour) before the keeper's shift is over, fob or no fob.
	const bool Fob = P.V->MembersAnyHour && S.Owns("gym");
	if (P.AnyService && (Staffed || !Fob))
	{
		// The hours that matter: the keeper's shift for a service with the keeper, else the door's.
		shop::Venue Hours = *P.V;
		if (Staffed && !P.V->Staffed.empty())
		{
			Hours.Open = P.V->Staffed;
		}
		const double Shuts = shop::ClosesAt(Hours, World);
		if (Shuts < World + 7.0 * Day - 1.0 && World + static_cast<double>(P.Minutes) > Shuts)
		{
			return Staffed && Fob ? "Not enough time before the desk closes." : "Not enough time before closing.";
		}
	}
	return std::string();
}

/** The receipt's line in the Bank: "Fifth St. Barbers · Crew Cut & Short Beard", "Second Chance Thrift · 3 items". */
std::string Receipt(const Priced& P)
{
	std::vector<std::string> Names;
	for (const shop::Offer* Of : P.Offers)
	{
		Names.push_back(Called(*Of));
	}
	for (const wardrobe::Garment* G : P.Garments)
	{
		Names.push_back(G->Name);
	}
	const std::string House = P.House > 0 ? " (cut on the house)" : "";
	if (Names.size() > 2)
	{
		return P.V->Name + Dot + std::to_string(Names.size()) + " items" + House;
	}
	return P.V->Name + Dot + JoinNames(Names) + House;
}

/**
 * The card charged and the ledger told (kind 9), once. A member's free workout leaves no receipt. Big Lou's check is a
 * line for nothing, "Ruby's (Big Lou got it)", with the tip, still the player's, on a line of its own; and the night is
 * marked (Life.Said[town::CheckKey]) so he covers one check a night.
 */
void Charge(Session& S, const Priced& P, double World)
{
	S.BankrollCents -= P.Total;
	if (P.Lou)
	{
		if (P.Tip > 0)
		{
			S.Life.Record(World, P.V->Name + Dot + "Tip", -P.Tip, 9);
		}
		S.Life.Record(World, P.V->Name + " (Big Lou got it)", 0, 9);
		const std::string Key = town::CheckKey(S, World);
		if (!Key.empty())
		{
			S.Life.Said[Key] = net::DayOf(World);
		}
		return;
	}
	if (P.Total > 0 || P.House > 0)
	{
		S.Life.Record(World, Receipt(P), -P.Total, 9); // a cut on the house rings up at $0, "on the house"
	}
}

/** What was had: Visit.Usual (the first service, else the first thing) and the tip or the lack of one. */
void Remember(Session& S, const Priced& P, const Session::ShopOrder& O)
{
	shop::Visit& V = S.Life.Visits[O.Venue];
	const shop::Offer* Main = nullptr;
	for (const shop::Offer* Of : P.Offers)
	{
		if (!Main || (IsService(*Of) && !IsService(*Main)))
		{
			Main = Of;
		}
	}
	if (Main)
	{
		V.Usual = Main->Id;
	}
	else if (!P.Garments.empty())
	{
		V.Usual = P.Garments.front()->Id;
	}
	if (TipVenue(O.Venue) && P.AnyService)
	{
		(O.TipPercent > 0 ? V.Tips : V.Stiffs) += 1;
		const std::string Keeper = shop::KeeperOf(O.Venue, S.WorldMinutes());
		if (!Keeper.empty())
		{
			S.Tipped(Keeper, std::max(0, O.TipPercent));
		}
	}
}

/** Goods handed over with the receipt: the cooler's bottles into the bag, the shake drunk at the desk, today's pass. */
void HandOver(Session& S, const shop::Offer& Of, double World)
{
	switch (Of.Type)
	{
	case shop::OfferKind::Drink:
		if (!IsService(Of))
		{
			S.Life.Pantry[Of.Id.substr(Of.Id.find('/') + 1)] += 1;
		}
		break;
	case shop::OfferKind::Shake:
	{
		const double HungerWas = S.Life.Hunger;
		const double ThirstWas = S.Life.Thirst;
		const double EnergyWas = S.Life.Energy;
		S.Life.Hunger = std::clamp(S.Life.Hunger - Of.Hunger, 0.0, 100.0);
		S.Life.Thirst = std::clamp(S.Life.Thirst - Of.Thirst, 0.0, 100.0);
		S.Life.Energy = std::clamp(S.Life.Energy + Of.Energy * S.Life.Perks.MealBoost, 0.0, 100.0);
		store::Item Cup;
		Cup.Id = Of.Id;
		Cup.Name = Of.Name;
		Cup.Where = store::Shelf::Drinks;
		Cup.Look = store::Art::Cup;
		S.LastEaten = Session::Eaten();
		S.LastEaten.ItemId = Of.Id;
		S.LastEaten.Hunger = S.Life.Hunger - HungerWas;
		S.LastEaten.Thirst = S.Life.Thirst - ThirstWas;
		S.LastEaten.Energy = S.Life.Energy - EnergyWas;
		S.LastEaten.Line = store::UsedLine(Cup, S.LastEaten.Hunger, S.LastEaten.Thirst, S.LastEaten.Energy);
		break;
	}
	case shop::OfferKind::DayPass:
	{
		shop::Visit& Pass = S.Life.Visits["gym/daypass"];
		++Pass.Count;
		Pass.LastAt = World;
		Pass.SpentCents += Of.PriceCents;
		break;
	}
	default: break;
	}
}

/** The closet a first purchase lands in: the look the player wears, as things they own. */
void EnsureCloset(Session& S)
{
	if (!S.Life.Closet.Seeded && S.Life.Closet.Owned.empty())
	{
		S.Life.Closet = wardrobe::SeedFrom(S.Person.Appearance);
	}
}

/** Takes a garment off and out of the closet (sold, pawned, given away); the portrait follows. */
void Drop(Session& S, const std::string& GarmentId)
{
	wardrobe::Closet& C = S.Life.Closet;
	C.Owned.erase(GarmentId);
	for (std::string& W : C.Worn)
	{
		if (W == GarmentId)
		{
			W.clear();
		}
	}
	wardrobe::ApplyTo(C, S.Person.Appearance);
}

// ------------------------------------------------------------------ the pawn's lots

/** Gear as people say it on a ticket: "27\" monitor". */
std::string Short(const std::string& GearId, const std::string& Fallback)
{
	static const std::pair<const char*, const char*> Names[] = {{"monitor-24", "24\" monitor"}, {"monitor-27", "27\" monitor"}, {"tower-mid", "desktop PC"},
		{"tower-dual", "two-PC rig"}, {"macro-pad", "macro pad"}, {"headphones", "headphones"}, {"webcam-720", "720p webcam"}, {"webcam-1080", "1080p webcam"},
		{"mirrorless", "mirrorless camera"}, {"mic-usb", "USB mic"}, {"mic-xlr", "broadcast mic"}, {"ring-light", "ring light"}, {"key-lights", "key lights"},
		{"green-screen", "green screen"}, {"chair", "desk chair"}, {"espresso", "espresso machine"}};
	for (const auto& N : Names)
	{
		if (GearId == N.first)
		{
			return N.second;
		}
	}
	return Fallback;
}

std::string ShortName(const shop::Lot& L)
{
	return L.Kind == shop::LotKind::Gear ? Short(L.Id.substr(5), L.Name) : L.Name;
}

const world::Award* AwardFor(const Session& S, const std::string& Rest)
{
	const size_t Colon = Rest.find(':');
	if (Colon == std::string::npos)
	{
		return nullptr;
	}
	const int AwardDay = std::atoi(Rest.substr(0, Colon).c_str());
	const std::string Event = Rest.substr(Colon + 1);
	for (const world::Award& A : S.Living().HeroAwards())
	{
		if (A.Day == AwardDay && A.Event == Event)
		{
			return &A;
		}
	}
	return nullptr;
}

std::string AwardId(const world::Award& A)
{
	return "award:" + std::to_string(A.Day) + ":" + A.Event;
}

/** Any lot by its id, valued as Augie would (whether or not the hero has it). False for nothing he knows. */
bool LotFor(const Session& S, const std::string& Id, shop::Lot& L)
{
	L = shop::Lot();
	L.Id = Id;
	const size_t Colon = Id.find(':');
	if (Colon == std::string::npos)
	{
		return false;
	}
	const std::string Kind = Id.substr(0, Colon);
	const std::string Rest = Id.substr(Colon + 1);
	if (Kind == "gear")
	{
		const gear::Item* I = gear::Find(Rest);
		if (!I || !shop::Takes(*I))
		{
			return false;
		}
		L.Kind = shop::LotKind::Gear;
		L.Name = I->Name;
		L.ValueCents = shop::GearValue(*I);
	}
	else if (Kind == "memento" && Rest == "cup")
	{
		L.Kind = shop::LotKind::Memento;
		L.Name = "The Embercrest cup";
		L.ValueCents = 6000;
		L.Final = true;
	}
	else if (Kind == "memento" && Rest == "deechip")
	{
		L.Kind = shop::LotKind::Memento;
		L.Name = "Dee's $100 chip";
		L.Refused = "Dee's? Put that back in your pocket. She'd have my license.";
	}
	else if (Kind == "award")
	{
		const world::Award* A = AwardFor(S, Rest);
		if (!A)
		{
			return false;
		}
		L.Kind = shop::LotKind::Award;
		L.Name = A->Event + (A->Ring ? " ring" : " bracelet");
		L.ValueCents = shop::AwardValue(*A);
		L.Final = true;
	}
	else if (Kind == "keepsake")
	{
		const hero::BackgroundInfo* B = hero::FindBackground(Rest);
		if (!B)
		{
			return false;
		}
		L = shop::KeepsakeOf(B->Id);
	}
	else if (Kind == "garment")
	{
		const wardrobe::Garment* G = wardrobe::Find(Rest);
		if (!G)
		{
			return false;
		}
		L.Kind = shop::LotKind::Garment;
		L.Name = G->Name;
		L.ValueCents = WholeDollars(G->PriceCents * 40 / 100); // 40% of the thrift's price
	}
	else if (Kind == "curio")
	{
		const shop::Curio* C = shop::FindCurio(Rest);
		if (!C)
		{
			return false;
		}
		L.Kind = shop::LotKind::Curio;
		L.Name = C->Name;
		L.ValueCents = C->PriceCents;
	}
	else
	{
		return false;
	}
	L.SellCents = shop::SellOf(L.ValueCents);
	L.LoanCents = shop::LoanOf(L.ValueCents);
	return true;
}

bool OwnsCurio(const Session& S, const shop::Curio& C)
{
	return C.Garment.empty() ? S.Life.Unlocks.count("curio:" + C.Id) > 0 : S.Life.Closet.Owned.count(C.Garment) > 0;
}

/** The hero's thing leaves them (sold or pawned). True when it was gear (the desk, the tables and the stream change). */
bool Release(Session& S, const shop::Lot& L)
{
	const std::string Rest = L.Id.substr(L.Id.find(':') + 1);
	switch (L.Kind)
	{
	case shop::LotKind::Gear: return S.Gear.erase(Rest) > 0;
	case shop::LotKind::Garment: Drop(S, Rest); break;
	case shop::LotKind::Curio:
		if (const shop::Curio* C = shop::FindCurio(Rest))
		{
			if (C->Garment.empty())
			{
				S.Life.Unlocks.erase("curio:" + C->Id);
			}
			else
			{
				Drop(S, C->Garment);
			}
		}
		break;
	case shop::LotKind::Memento:
	case shop::LotKind::Award:
	case shop::LotKind::Keepsake: break; // where it is (the sill, the case, a pocket) is PawnHolds'
	}
	return false;
}

/** It comes back (redeemed or bought). True when it was gear. */
bool Restore(Session& S, const shop::Lot& L)
{
	const std::string Rest = L.Id.substr(L.Id.find(':') + 1);
	switch (L.Kind)
	{
	case shop::LotKind::Gear:
		if (S.Gear.count(Rest) == 0)
		{
			S.Gear[Rest] = 0.0;
		}
		return true;
	case shop::LotKind::Garment:
		EnsureCloset(S);
		S.Life.Closet.Owned.insert(Rest);
		break;
	case shop::LotKind::Curio:
		if (const shop::Curio* C = shop::FindCurio(Rest))
		{
			if (C->Garment.empty())
			{
				S.Life.Unlocks.insert("curio:" + C->Id);
			}
			else
			{
				EnsureCloset(S);
				S.Life.Closet.Owned.insert(C->Garment);
			}
		}
		break;
	case shop::LotKind::Memento:
	case shop::LotKind::Award:
	case shop::LotKind::Keepsake: break;
	}
	return false;
}

/** Augie's bond: the Owed flag while a ticket is open. */
void Owed(Session& S)
{
	const bool Open = !S.Life.Pawn.Tickets.empty();
	auto It = S.Life.Town.find("augie");
	if (Open)
	{
		S.Life.Town["augie"].Flags |= town::Owed;
	}
	else if (It != S.Life.Town.end())
	{
		It->second.Flags &= ~town::Owed;
	}
}

/** Sold to Augie: the cash, the ledger, the thing gone (gear and garments to his shelf, the rest gone for good). */
bool SellNow(Session& S, const shop::Lot& L, double At)
{
	S.BankrollCents += L.SellCents;
	S.Life.Record(At, std::string(PawnName) + " (sold: " + ShortName(L) + ")", L.SellCents, 9);
	const bool GearOut = Release(S, L);
	if (L.Kind == shop::LotKind::Gear || L.Kind == shop::LotKind::Garment)
	{
		shop::ShelfLot Shelf;
		Shelf.Lot = L.Id;
		Shelf.PriceCents = L.ValueCents;
		Shelf.From = At;
		Shelf.Until = At + shop::PawnShelfDays * Day;
		S.Life.Pawn.Shelf.push_back(Shelf);
	}
	else if (L.Final)
	{
		S.Life.Pawn.Away.insert(L.Id);
	}
	return GearOut;
}

/** Pawned: the loan in hand and a 30-day ticket; the thing waits in his cage. */
bool LoanNow(Session& S, const shop::Lot& L, double At, int& Number)
{
	shop::Ticket T;
	T.Number = S.Life.Pawn.NextTicket++;
	T.Lot = L.Id;
	T.LoanCents = L.LoanCents;
	T.TakenAt = At;
	T.DueAt = At + shop::PawnTermDays * Day;
	S.Life.Pawn.Tickets.push_back(T);
	Number = T.Number;
	S.BankrollCents += L.LoanCents;
	S.Life.Record(At, std::string(PawnName) + " (loan #" + shop::TicketNumber(T.Number) + ")", L.LoanCents, 9);
	Owed(S);
	return Release(S, L);
}

/** Dee hears when a bracelet or a ring goes over Augie's glass. */
std::string DeeOnAward(const shop::Lot& L, bool Sold)
{
	const std::string Piece = L.Name.size() >= 4 && L.Name.compare(L.Name.size() - 4, 4, "ring") == 0 ? "ring" : "bracelet";
	return Sold ? "you sold the " + Piece + "? ok. it's metal. the title's still yours. go win another one."
				: "you pawned the " + Piece + ". ok. it's metal. go win it back.";
}
} // namespace session_shops_detail


// ------------------------------------------------------------------ the shops

bool Session::ShopOpenFor(const std::string& Venue) const
{
	using namespace session_shops_detail;
	const shop::Venue* V = shop::FindVenue(Venue);
	if (!V)
	{
		return false;
	}
	// The gym's fob: a member gets in at any hour.
	return shop::IsOpen(*V, WorldMinutes()) || (V->MembersAnyHour && Owns("gym"));
}

std::string Session::ShopBlocked(const std::string& OfferId) const
{
	using namespace session_shops_detail;
	const shop::Offer* O = shop::FindOffer(OfferId);
	if (!O)
	{
		return "Not on offer.";
	}
	ShopOrder One;
	One.Venue = O->Venue;
	One.Offers.push_back(OfferId);
	return OrderBlocked(*this, Price(*this, One, false), One);
}

Chips Session::ShopTotal(const ShopOrder& O) const
{
	using namespace session_shops_detail;
	const Priced P = Price(*this, O);
	return P.Why.empty() ? P.Total : 0;
}

std::string Session::ShopPay(const ShopOrder& O)
{
	using namespace session_shops_detail;
	const Priced P = Price(*this, O);
	if (P.Why.empty() && P.AnyService)
	{
		return "That one takes a seat: order it as a service.";
	}
	const std::string Why = OrderBlocked(*this, P, O);
	if (!Why.empty())
	{
		return Why;
	}
	if (P.Total > BankrollCents)
	{
		Sound(SoundId::Fold, 0.5);
		return Declined;
	}
	const double World = WorldMinutes();
	Charge(*this, P, World);
	ShopVisit(O.Venue, P.Total);
	Remember(*this, P, O);
	for (const shop::Offer* Of : P.Offers)
	{
		HandOver(*this, *Of, World);
		if (Of->Type == shop::OfferKind::Membership)
		{
			// The same subscription as GearDrop's: renewed every 30 days by RenewGear, wherever it was bought.
			Gear["gym"] = World + 30.0 * Day;
			RefreshGear();
			Hooks.GearChanged("gym", true);
		}
	}
	if (!P.Garments.empty())
	{
		EnsureCloset(*this);
		for (const wardrobe::Garment* G : P.Garments)
		{
			Life.Closet.Owned.insert(G->Id);
		}
	}
	Sound(SoundId::Cash, 0.6);
	Save();
	return "";
}

std::string Session::ShopService(const ShopOrder& O)
{
	using namespace session_shops_detail;
	const Priced P = Price(*this, O);
	if (P.Why.empty() && !P.AnyService)
	{
		return ShopPay(O); // nothing to sit for: goods, charged and handed over
	}
	const std::string Why = OrderBlocked(*this, P, O);
	if (!Why.empty())
	{
		return Why;
	}
	if (P.Total > BankrollCents)
	{
		Sound(SoundId::Fold, 0.5);
		return Declined;
	}
	const double World = WorldMinutes();
	Charge(*this, P, World);
	if (P.House > 0)
	{
		sponsor::UseService(Life.Sponsors, O.Venue, World); // the sponsor's cut for this fortnight is used
	}
	ShopVisit(O.Venue, P.Total);
	Remember(*this, P, O);
	std::string Ids;
	double Energy = 0.0;
	for (const shop::Offer* Of : P.Offers)
	{
		if (!IsService(*Of))
		{
			HandOver(*this, *Of, World);
			if (Of->Type == shop::OfferKind::Membership)
			{
				// Joined at the desk on the way to the floor: the same subscription ShopPay sells.
				Gear["gym"] = World + 30.0 * Day;
				RefreshGear();
				Hooks.GearChanged("gym", true);
			}
			continue;
		}
		Ids += (Ids.empty() ? "" : ",") + Of->Id;
		// A plate's energy goes as far as the line cook can make it (store::ReliefOf's rule).
		Energy += Of->Energy > 0.0 && O.Venue == "diner" ? Of->Energy * Life.Perks.MealBoost : Of->Energy;
	}
	if (Stream.Live)
	{
		EndStream();
	}
	// Played on the street as a short cinematic: the barber's 8 seconds, a workout's 5, a meal's 4 (§5.0).
	TimeSkip = Skip();
	TimeSkip.Active = true;
	TimeSkip.From = World;
	TimeSkip.To = World + static_cast<double>(P.Minutes);
	TimeSkip.RealStart = Now;
	TimeSkip.RealSeconds = O.Venue == "barber" ? 8.0 : O.Venue == "gym" ? 5.0 : 4.0;
	TimeSkip.Rest = 1.0;
	TimeSkip.Couch = Evicted();
	TimeSkip.Label = "At " + P.V->Name;
	TimeSkip.Result.ActivityId = "shop:" + O.Venue + ":" + Ids;
	TimeSkip.Result.Start = World;
	TimeSkip.Result.End = TimeSkip.To;
	TimeSkip.Result.Energy = Energy;
	HasOutcome = false;
	ConfirmRegister = false;
	Sound(SoundId::Cash, 0.6);
	return "";
}

std::string Session::WaitAt(const std::string& Venue, double UntilWorld)
{
	using namespace session_shops_detail;
	const double World = WorldMinutes();
	const bool Stop = Venue == "shelter" || Venue == "market-east" || Venue == "market-west";
	const shop::Venue* V = Stop ? nullptr : shop::FindVenue(Venue);
	std::string Label;
	if (Stop)
	{
		Label = ShelterWait;
	}
	else if (V && V->Id == "diner")
	{
		Label = DinerWait;
	}
	else if (V && V->Id == "barber")
	{
		Label = BarberWait;
	}
	else if (V && V->Id == "washfold")
	{
		Label = LaundryWait;
	}
	else
	{
		return "Nowhere to sit here.";
	}
	if (TimeSkip.Active)
	{
		return "You're busy.";
	}
	if (T)
	{
		return "Finish your tournament first.";
	}
	if (UntilWorld <= World + 0.5)
	{
		return "Nothing to wait for.";
	}
	if (UntilWorld - World > 8.0 * 60.0 + 0.5)
	{
		return "Eight hours, tops.";
	}
	if (V)
	{
		if (!ShopOpenFor(V->Id))
		{
			return "Closed" + std::string(Dot) + shop::HoursLine(*V, World);
		}
		const double Shuts = shop::ClosesAt(*V, World);
		if (Shuts < World + 7.0 * Day - 1.0 && UntilWorld > Shuts)
		{
			return "They close at " + net::TimeLabel(Shuts) + ".";
		}
	}
	if (Stream.Live)
	{
		EndStream();
	}
	const double Hours = (UntilWorld - World) / 60.0;
	TimeSkip = Skip();
	TimeSkip.Active = true;
	TimeSkip.From = World;
	TimeSkip.To = UntilWorld;
	TimeSkip.RealStart = Now;
	TimeSkip.RealSeconds = 3.0 + 0.5 * Hours;
	TimeSkip.Rest = 1.0;
	TimeSkip.Couch = Evicted();
	TimeSkip.Label = Label;
	// Awake (an empty activity id): the needs climb and the energy drains as the hours pass.
	TimeSkip.Result.Start = World;
	TimeSkip.Result.End = UntilWorld;
	const int Mins = static_cast<int>(std::lround(UntilWorld - World));
	if (Label == DinerWait)
	{
		// The coffee keeps coming while you sit (ShopStep pours the refills; the second and third cups give energy): the
		// drain stays at 3 an hour.
		const bool Coffee = Life.CoffeeUntil > World;
		int Refills = 0;
		if (Coffee)
		{
			Life.CoffeeUntil = std::max(Life.CoffeeUntil, UntilWorld + 120.0);
			const auto It = Life.Visits.find("diner");
			const double OrderAt = It != Life.Visits.end() ? It->second.LastAt : World;
			for (int K = 2; K < 400; ++K)
			{
				const double At = OrderAt + 30.0 * static_cast<double>(K - 1);
				if (At > UntilWorld)
				{
					break;
				}
				Refills += At > World ? 1 : 0;
			}
		}
		const std::string Who = shop::KeeperOf("diner", UntilWorld) == "ruby" ? "Ruby" : "Marlene";
		TimeSkip.Result.Title = "Sat a while";
		// The hours as the page asked for them (a host's clock can land a minute either side); anything else to the minute.
		const int Whole = static_cast<int>(std::lround(static_cast<double>(Mins) / 60.0)) * 60;
		const std::string Spent = Sentence(Duration(Mins >= 60 && std::abs(Mins - Whole) <= 2 ? Whole : Mins));
		TimeSkip.Result.Body = Spent + " in a booth at Ruby's, the window going from one color to another." +
			(Refills > 0 ? " " + Who + " topped up the coffee " + (Refills == 1 ? std::string("once.") : std::to_string(Refills) + " times.") : std::string(" Nobody hurried you."));
	}
	else if (Label == BarberWait)
	{
		TimeSkip.Result.Title = "Next";
		TimeSkip.Result.Body = "Lonnie sweeps off the chair and nods you over.";
	}
	else if (Label == ShelterWait)
	{
		TimeSkip.Result.Title = "The 14";
		TimeSkip.Result.Body = "Headlights at the corner of Fifth. The bus pulls into the bay, kneels, and opens its doors.";
	}
	else
	{
		TimeSkip.Result.Title = "Waited";
		TimeSkip.Result.Body = "The dryers turned. Inez folded. The time went.";
	}
	HasOutcome = false;
	Sound(SoundId::Click, 0.6);
	return "";
}

std::string Session::Wear(const std::string& GarmentId)
{
	using namespace session_shops_detail;
	const wardrobe::Garment* G = wardrobe::Find(GarmentId);
	if (!G || Life.Closet.Owned.count(GarmentId) == 0)
	{
		return "You don't own that.";
	}
	if (TimeSkip.Active)
	{
		return "You're busy.";
	}
	std::string& Slot = Life.Closet.Worn[static_cast<size_t>(G->Where)];
	if (Slot != GarmentId)
	{
		Slot = GarmentId;
		Life.Closet.Seeded = true;
		wardrobe::ApplyTo(Life.Closet, Person.Appearance);
		Save();
	}
	return "";
}

std::string Session::TakeOff(int WardrobeSlot)
{
	using namespace session_shops_detail;
	if (WardrobeSlot < 0 || WardrobeSlot >= wardrobe::SlotCount)
	{
		return "Nothing there.";
	}
	std::string& Slot = Life.Closet.Worn[static_cast<size_t>(WardrobeSlot)];
	if (Slot.empty())
	{
		return "Nothing to take off.";
	}
	if (TimeSkip.Active)
	{
		return "You're busy.";
	}
	Slot.clear();
	wardrobe::ApplyTo(Life.Closet, Person.Appearance);
	Save();
	return "";
}

std::string Session::Donate(const std::string& GarmentId)
{
	using namespace session_shops_detail;
	const wardrobe::Garment* G = wardrobe::Find(GarmentId);
	if (!G || Life.Closet.Owned.count(GarmentId) == 0)
	{
		return "You don't own that.";
	}
	if (TimeSkip.Active)
	{
		return "You're busy.";
	}
	if (!ShopOpenFor("thrift"))
	{
		const shop::Venue* V = shop::FindVenue("thrift");
		return "Closed" + std::string(Dot) + (V ? shop::HoursLine(*V, WorldMinutes()) : std::string());
	}
	const double World = WorldMinutes();
	Drop(*this, GarmentId);
	// Bea writes a receipt "for your taxes" (a line in the Bank, for nothing), and remembers.
	Life.Record(World, "Second Chance Thrift (donation: " + G->Name + ")", 0, 9);
	ShopVisit("thrift", 0);
	town::Bond& Bea = Life.Town["bea"];
	Bea.Flags |= town::Donor;
	Bea.Familiarity = std::min(100, Bea.Familiarity + 1);
	Save();
	return "";
}

std::string Session::BoardBus(const std::string& LiveId)
{
	using namespace session_shops_detail;
	const char* const NoEntry = "The 14 \xC2\xB7 Embercrest. Register for an event first (Burner).";
	const life::LiveEntry* E = LiveId.empty() ? live::ActiveEntry(Life) : live::EntryFor(Life, LiveId);
	if (!E || E->State != life::LiveEntry::Registered)
	{
		return NoEntry;
	}
	const live::Occurrence O = live::FindOccurrence(E->Id);
	if (!O.Valid())
	{
		return NoEntry;
	}
	if (E->ArrivedAt > 0.0)
	{
		return ResumeLive(); // a night already under way: straight back to the seat
	}
	const double World = WorldMinutes();
	if (World + live::TravelMinutes > O.LateRegEnds)
	{
		return "Late registration's over by the time the 14 gets up the ridge.";
	}
	if (World + live::TravelMinutes < O.DeskOpens)
	{
		return "The desk opens at " + net::TimeLabel(O.DeskOpens) + ". Take a later 14.";
	}
	// As the desk's Live activity does (Session::GoToGame): the host opens Embercrest?Live=<id>, the fare is paid.
	return GoToLive(E->Id);
}

uint64_t Session::WorldSeed() const
{
	using namespace session_shops_detail;
	// The living world's seed (saved with the career), spread over 64 bits (splitmix64's finalizer).
	uint64_t Z = static_cast<uint64_t>(LivingWorld.Seed()) + 0x9e3779b97f4a7c15ull;
	Z = (Z ^ (Z >> 30)) * 0xbf58476d1ce4e5b9ull;
	Z = (Z ^ (Z >> 27)) * 0x94d049bb133111ebull;
	return Z ^ (Z >> 31);
}

double Session::Fitness() const
{
	using namespace session_shops_detail;
	return Life.Fit.Level;
}

double Session::Sharp() const
{
	using namespace session_shops_detail;
	return body::Sharp(Life.Groom, WorldMinutes());
}

// ------------------------------------------------------------------ the pawn

std::vector<shop::Lot> Session::PawnLots() const
{
	using namespace session_shops_detail;
	std::vector<shop::Lot> Out;
	const double World = WorldMinutes();
	// The gear: at home, or in Ninth St. Storage while the unit's paid up ("Bring it over": an hour's round trip).
	const bool Stored = Evicted() && Life.StorageDue > 0.0;
	if (!Stored || Life.StorageDue > World)
	{
		for (const gear::Item& I : gear::Catalog())
		{
			shop::Lot L;
			if (Owns(I.Id) && LotFor(*this, "gear:" + I.Id, L))
			{
				L.Stored = Stored;
				Out.push_back(L);
			}
		}
	}
	shop::Lot L;
	// The cup on the sill.
	if (Life.LiveBestPlace == 1 && !PawnHolds("memento:cup") && LotFor(*this, "memento:cup", L))
	{
		Out.push_back(L);
	}
	// The trophy case: the piece leaves, the title stays in the records.
	for (const world::Award& A : Living().HeroAwards())
	{
		const std::string Id = AwardId(A);
		if (!PawnHolds(Id) && LotFor(*this, Id, L))
		{
			Out.push_back(L);
		}
	}
	// The keepsake in the hero's pocket.
	if (Person.Created)
	{
		const std::string Id = std::string("keepsake:") + hero::InfoOf(Person.Story).Key;
		if (!PawnHolds(Id) && LotFor(*this, Id, L))
		{
			Out.push_back(L);
		}
	}
	// The thrift's rare finds ("Leather I'll take. The rest, try Bea.") and the vintage pieces from his own case.
	for (const std::string& Id : Life.Closet.Owned)
	{
		const wardrobe::Garment* G = wardrobe::Find(Id);
		if (G && G->Rare && LotFor(*this, "garment:" + Id, L))
		{
			Out.push_back(L);
		}
	}
	for (const shop::Curio& C : shop::Curios())
	{
		if (OwnsCurio(*this, C) && LotFor(*this, "curio:" + C.Id, L))
		{
			Out.push_back(L);
		}
	}
	// Dee's chip, which he won't touch.
	if (Life.BackRoomNetCents > 0 && LotFor(*this, "memento:deechip", L))
	{
		Out.push_back(L);
	}
	// A sponsor's gear or kit is the brand's while the deal runs (next_segment_spec §3.6): he won't buy it or lend on it.
	for (shop::Lot& Each : Out)
	{
		if (sponsor::IsProperty(Life.Sponsors, Each.Id, World))
		{
			Each.Refused = "That's got a brand's sticker on it. I don't buy loaners.";
		}
	}
	return Out;
}

std::vector<shop::Lot> Session::PawnShelf() const
{
	using namespace session_shops_detail;
	const double World = WorldMinutes();
	std::vector<std::pair<std::string, Chips>> Stock;
	auto Add = [&Stock](const std::string& Id, Chips Price) {
		if (std::none_of(Stock.begin(), Stock.end(), [&Id](const std::pair<std::string, Chips>& S) { return S.first == Id; }))
		{
			Stock.push_back({Id, Price});
		}
	};
	// The hero's own (sold, forfeited) and Unit 114's pieces first, then Monday's used gear, then the curio case.
	for (const shop::ShelfLot& S : Life.Pawn.Shelf)
	{
		if (World >= S.From && World < S.Until)
		{
			Add(S.Lot, S.PriceCents);
		}
	}
	for (const shop::ShelfLot& S : shop::UsedStock(shop::UsedWeek(World), WorldSeed(), ~0ull))
	{
		Add(S.Lot, S.PriceCents);
	}
	for (const shop::Curio& C : shop::Curios())
	{
		Add("curio:" + C.Id, C.PriceCents);
	}
	std::vector<shop::Lot> Out;
	for (const auto& S : Stock)
	{
		shop::Lot L;
		if (!LotFor(*this, S.first, L))
		{
			continue;
		}
		L.ValueCents = S.second;
		L.Refused.clear();
		const std::string Rest = S.first.substr(S.first.find(':') + 1);
		if (L.Kind == shop::LotKind::Gear)
		{
			const gear::Item* I = gear::Find(Rest);
			if (Owns(Rest))
			{
				L.Refused = "Owned.";
			}
			else if (Evicted())
			{
				L.Refused = "Needs a room."; // "Where you gonna plug it in, the couch?"
			}
			else if (I && !I->Requires.empty() && !Owns(I->Requires))
			{
				// GearDrop's upgrade path holds here too: the 27-inch goes beside a 24, the two-PC rig beside a desktop.
				const gear::Item* Need = gear::Find(I->Requires);
				L.Refused = "Needs the " + Short(I->Requires, Need ? Need->Name : I->Requires) + " first.";
			}
			else if (I && I->Where != gear::Slot::None)
			{
				for (const auto& G : Gear)
				{
					const gear::Item* Have = gear::Find(G.first);
					if (Have && Have->Where == I->Where && Have->PriceCents > I->PriceCents)
					{
						L.Refused = "You have better.";
					}
				}
			}
		}
		else if (L.Kind == shop::LotKind::Garment && Life.Closet.Owned.count(Rest) > 0)
		{
			L.Refused = "Owned.";
		}
		else if (L.Kind == shop::LotKind::Curio)
		{
			const shop::Curio* C = shop::FindCurio(Rest);
			if (C && OwnsCurio(*this, *C))
			{
				L.Refused = "Owned.";
			}
		}
		Out.push_back(L);
	}
	return Out;
}

std::string Session::PawnSell(const std::string& LotId)
{
	using namespace session_shops_detail;
	shop::Lot L;
	bool Found = false;
	for (const shop::Lot& Each : PawnLots())
	{
		if (Each.Id == LotId)
		{
			L = Each;
			Found = true;
		}
	}
	if (!Found)
	{
		return "That's not yours to sell.";
	}
	if (!L.Refused.empty())
	{
		return L.Refused;
	}
	if (TimeSkip.Active)
	{
		return "You're busy.";
	}
	if (T)
	{
		return "Finish your tournament first.";
	}
	if (!ShopOpenFor("pawn"))
	{
		return "Closed" + std::string(Dot) + shop::HoursLine(*shop::FindVenue("pawn"), WorldMinutes());
	}
	const double World = WorldMinutes();
	if (L.Stored)
	{
		// "Bring it over": an hour to Ninth St. and back, then the sale.
		const double Shuts = shop::ClosesAt(*shop::FindVenue("pawn"), World);
		if (Shuts - World < 60.0)
		{
			return "He closes before you'd be back from Ninth St.";
		}
		TimeSkip = Skip();
		TimeSkip.Active = true;
		TimeSkip.From = World;
		TimeSkip.To = World + 60.0;
		TimeSkip.RealStart = Now;
		TimeSkip.RealSeconds = 3.5;
		TimeSkip.Rest = 1.0;
		TimeSkip.Couch = Evicted();
		TimeSkip.Label = StorageRun;
		TimeSkip.Result.ActivityId = "shop:pawn:sell:" + LotId;
		TimeSkip.Result.Start = World;
		TimeSkip.Result.End = World + 60.0;
		HasOutcome = false;
		Sound(SoundId::Click, 0.6);
		return "";
	}
	if (SellNow(*this, L, World))
	{
		RefreshGear();
		Hooks.GearChanged(LotId.substr(5), false);
	}
	if (L.Kind == shop::LotKind::Award)
	{
		StoryText("pawn-award-sold:" + LotId, "Dee", DeeOnAward(L, true));
	}
	ShopVisit("pawn", 0);
	Sound(SoundId::Cash, 0.7);
	Save();
	return "";
}

std::string Session::PawnLoan(const std::string& LotId)
{
	using namespace session_shops_detail;
	shop::Lot L;
	bool Found = false;
	for (const shop::Lot& Each : PawnLots())
	{
		if (Each.Id == LotId)
		{
			L = Each;
			Found = true;
		}
	}
	if (!Found)
	{
		return "That's not yours to pawn.";
	}
	if (!L.Refused.empty())
	{
		return L.Refused;
	}
	if (TimeSkip.Active)
	{
		return "You're busy.";
	}
	if (T)
	{
		return "Finish your tournament first.";
	}
	if (!ShopOpenFor("pawn"))
	{
		return "Closed" + std::string(Dot) + shop::HoursLine(*shop::FindVenue("pawn"), WorldMinutes());
	}
	const double World = WorldMinutes();
	if (L.Stored)
	{
		const double Shuts = shop::ClosesAt(*shop::FindVenue("pawn"), World);
		if (Shuts - World < 60.0)
		{
			return "He closes before you'd be back from Ninth St.";
		}
		TimeSkip = Skip();
		TimeSkip.Active = true;
		TimeSkip.From = World;
		TimeSkip.To = World + 60.0;
		TimeSkip.RealStart = Now;
		TimeSkip.RealSeconds = 3.5;
		TimeSkip.Rest = 1.0;
		TimeSkip.Couch = Evicted();
		TimeSkip.Label = StorageRun;
		TimeSkip.Result.ActivityId = "shop:pawn:loan:" + LotId;
		TimeSkip.Result.Start = World;
		TimeSkip.Result.End = World + 60.0;
		HasOutcome = false;
		Sound(SoundId::Click, 0.6);
		return "";
	}
	int Number = 0;
	if (LoanNow(*this, L, World, Number))
	{
		RefreshGear();
		Hooks.GearChanged(LotId.substr(5), false);
	}
	if (L.Kind == shop::LotKind::Award)
	{
		StoryText("pawn-award-loan:" + LotId, "Dee", DeeOnAward(L, false));
	}
	ShopVisit("pawn", 0);
	Sound(SoundId::Cash, 0.7);
	Save();
	return "";
}

std::string Session::PawnRedeem(int Ticket)
{
	using namespace session_shops_detail;
	auto It = std::find_if(Life.Pawn.Tickets.begin(), Life.Pawn.Tickets.end(), [Ticket](const shop::Ticket& Tk) { return Tk.Number == Ticket; });
	if (It == Life.Pawn.Tickets.end())
	{
		return "No such ticket.";
	}
	if (TimeSkip.Active)
	{
		return "You're busy.";
	}
	if (T)
	{
		return "Finish your tournament first.";
	}
	const double World = WorldMinutes();
	if (!ShopOpenFor("pawn"))
	{
		return "Closed" + std::string(Dot) + shop::HoursLine(*shop::FindVenue("pawn"), World);
	}
	const Chips Price = shop::RedeemCents(*It, World);
	if (Price <= 0)
	{
		return "Thirty days and a week. That one's his now.";
	}
	if (Price > BankrollCents)
	{
		Sound(SoundId::Fold, 0.5);
		return Declined;
	}
	const shop::Ticket Tk = *It;
	BankrollCents -= Price;
	Life.Record(World, std::string(PawnName) + " (redeemed #" + shop::TicketNumber(Tk.Number) + ")", -Price, 9);
	Life.Pawn.Tickets.erase(It);
	Owed(*this);
	shop::Lot L;
	if (LotFor(*this, Tk.Lot, L) && Restore(*this, L))
	{
		RefreshGear();
		Hooks.GearChanged(Tk.Lot.substr(5), true);
	}
	ShopVisit("pawn", Price);
	Sound(SoundId::Cash, 0.7);
	Save();
	return "";
}

std::string Session::PawnRenew(int Ticket)
{
	using namespace session_shops_detail;
	auto It = std::find_if(Life.Pawn.Tickets.begin(), Life.Pawn.Tickets.end(), [Ticket](const shop::Ticket& Tk) { return Tk.Number == Ticket; });
	if (It == Life.Pawn.Tickets.end())
	{
		return "No such ticket.";
	}
	if (TimeSkip.Active)
	{
		return "You're busy.";
	}
	if (T)
	{
		return "Finish your tournament first.";
	}
	const double World = WorldMinutes();
	if (!ShopOpenFor("pawn"))
	{
		return "Closed" + std::string(Dot) + shop::HoursLine(*shop::FindVenue("pawn"), World);
	}
	if (shop::RedeemCents(*It, World) <= 0)
	{
		return "Thirty days and a week. That one's his now.";
	}
	// A renewal buys thirty days from today: not on a ticket that's already good that long (a fresh one, or a second
	// press the same day, would pay again for nothing).
	if (!shop::Renewable(*It, World))
	{
		return "Nothing to renew yet. It's good till " + net::DateLabel(net::DayOf(It->DueAt)) + ".";
	}
	// What the ticket owes today: the 20% in term, the 25% in the week of grace (redeeming then costs the same 25%, so a
	// renewal in the grace is never the cheaper way out).
	const Chips Fee = shop::RenewCents(*It, World);
	if (Fee > BankrollCents)
	{
		Sound(SoundId::Fold, 0.5);
		return Declined;
	}
	BankrollCents -= Fee;
	Life.Record(World, std::string(PawnName) + " (renewed #" + shop::TicketNumber(It->Number) + ")", -Fee, 9);
	// Another thirty days from today.
	It->DueAt = World + shop::PawnTermDays * Day;
	++It->Renewals;
	ShopVisit("pawn", Fee);
	Sound(SoundId::Cash, 0.6);
	Save();
	return "";
}

std::string Session::PawnBuy(const std::string& LotId)
{
	using namespace session_shops_detail;
	shop::Lot L;
	bool Found = false;
	for (const shop::Lot& Each : PawnShelf())
	{
		if (Each.Id == LotId)
		{
			L = Each;
			Found = true;
		}
	}
	if (!Found)
	{
		return "Not on his shelf.";
	}
	if (!L.Refused.empty())
	{
		return L.Refused;
	}
	if (TimeSkip.Active)
	{
		return "You're busy.";
	}
	if (T)
	{
		return "Finish your tournament first.";
	}
	const double World = WorldMinutes();
	if (!ShopOpenFor("pawn"))
	{
		return "Closed" + std::string(Dot) + shop::HoursLine(*shop::FindVenue("pawn"), World);
	}
	// Used goods are taxed like any goods.
	const Chips Total = L.ValueCents + store::Tax(L.ValueCents);
	if (Total > BankrollCents)
	{
		Sound(SoundId::Fold, 0.5);
		return Declined;
	}
	BankrollCents -= Total;
	Life.Record(World, std::string(PawnName) + " (bought: " + ShortName(L) + ")", -Total, 9);
	std::vector<shop::ShelfLot>& Shelf = Life.Pawn.Shelf;
	const auto On = std::find_if(Shelf.begin(), Shelf.end(), [&LotId, World](const shop::ShelfLot& S) { return S.Lot == LotId && World >= S.From && World < S.Until; });
	if (On != Shelf.end())
	{
		Shelf.erase(On); // the hero's own, or Unit 114's: home again
	}
	if (Restore(*this, L))
	{
		RefreshGear();
		Hooks.GearChanged(LotId.substr(5), true);
	}
	ShopVisit("pawn", Total);
	Sound(SoundId::Cash, 0.7);
	Save();
	return "";
}

bool Session::PawnHolds(const std::string& LotId) const
{
	using namespace session_shops_detail;
	// In his cage (an open ticket), in his case or on his shelf (sold to him, or forfeited, for 21 days), or gone for good
	// (sold on). A forfeited cup sits in his case, not on the apartment's sill.
	const shop::PawnBook& B = Life.Pawn;
	if (B.Away.count(LotId) > 0)
	{
		return true;
	}
	return std::any_of(B.Tickets.begin(), B.Tickets.end(), [&](const shop::Ticket& Loan) { return Loan.Lot == LotId; }) ||
		   std::any_of(B.Shelf.begin(), B.Shelf.end(), [&](const shop::ShelfLot& OnSale) { return OnSale.Lot == LotId; });
}

// ------------------------------------------------------------------ Session.cpp's hooks

void Session::SettleShop()
{
	using namespace session_shops_detail;
	// FinishSkip, for a "shop:<venue>:<offer ids>" skip (TimeSkip.Result still holds it; LastOutcome has its energy).
	const life::Outcome& O = TimeSkip.Result;
	const std::string& Id = O.ActivityId;
	const size_t VenueEnd = Id.find(':', 5);
	if (Id.rfind("shop:", 0) != 0 || VenueEnd == std::string::npos)
	{
		return;
	}
	const std::string Venue = Id.substr(5, VenueEnd - 5);
	const std::string Rest = Id.substr(VenueEnd + 1);
	const double End = O.End;
	const shop::Venue* V = shop::FindVenue(Venue);

	// A sponsor's appearance (Session::Appear): time with the keeper, counted by FinishSkip (sponsor::Appear) as it ends.
	if (Rest == "appear")
	{
		LastOutcome.Title = Venue == "store" ? "Penny's Pick" : "The window chair";
		LastOutcome.Body = Venue == "store" ? "Ray holds the Polaroid up to the light until it comes in. It goes on the corkboard behind the register, by the lotto board."
											: "Thirty minutes in Lonnie's window chair. Two people slow down to look. One comes in.";
		return;
	}

	// The pawn, back from Ninth St. with the thing under an arm.
	if (Venue == "pawn")
	{
		const bool Sell = Rest.rfind("sell:", 0) == 0;
		const std::string LotId = Rest.size() > 5 ? Rest.substr(5) : std::string();
		shop::Lot L;
		if (!LotFor(*this, LotId, L) || L.Kind != shop::LotKind::Gear || !Owns(LotId.substr(5)))
		{
			LastOutcome.Title = "Back from Ninth St.";
			LastOutcome.Body = "The unit didn't have it. Augie shrugs and goes back to his paper.";
			return;
		}
		int Number = 0;
		const bool GearOut = Sell ? SellNow(*this, L, End) : LoanNow(*this, L, End, Number);
		if (GearOut)
		{
			RefreshGear();
			Hooks.GearChanged(LotId.substr(5), false);
		}
		ShopVisit("pawn", 0);
		LastOutcome.Money = Sell ? L.SellCents : L.LoanCents;
		LastOutcome.Title = Sell ? "Sold" : "Pawned";
		LastOutcome.Body = Sell ? "An hour to Ninth St. and back with the " + ShortName(L) + ". Augie counts out " + Money(L.SellCents) + " on the glass."
								: "An hour to Ninth St. and back with the " + ShortName(L) + ". Ticket #" + shop::TicketNumber(Number) + ": " + Money(L.LoanCents) +
				  " now, " + Money(L.LoanCents * 120 / 100) + " to have it back inside thirty days.";
		return;
	}

	// Everything else: the services that were ordered.
	std::vector<const shop::Offer*> Offers;
	for (size_t From = 0; From <= Rest.size();)
	{
		const size_t Comma = Rest.find(',', From);
		const std::string One = Rest.substr(From, Comma == std::string::npos ? std::string::npos : Comma - From);
		if (const shop::Offer* Of = shop::FindOffer(One))
		{
			Offers.push_back(Of);
		}
		if (Comma == std::string::npos)
		{
			break;
		}
		From = Comma + 1;
	}
	std::vector<std::string> Names;
	for (const shop::Offer* Of : Offers)
	{
		Names.push_back(Called(*Of));
	}
	LastOutcome.Title = Names.empty() ? (V ? V->Name : std::string("Done")) : JoinNames(Names);
	const int Minutes = static_cast<int>(std::lround(End - O.Start));

	if (Venue == "barber")
	{
		hero::Look& Look = Person.Appearance;
		for (const shop::Offer* Of : Offers)
		{
			switch (Of->Type)
			{
			case shop::OfferKind::Cut:
				if (Of->LookValue >= 0)
				{
					Look.Hair = Of->LookValue;
				}
				Life.Groom.CutAt = End;
				++Life.Groom.Cuts;
				break;
			case shop::OfferKind::Beard:
				Look.FacialHair = Of->LookValue;
				Life.Groom.ShaveAt = End;
				break;
			case shop::OfferKind::Color: Look.HairColor = Of->LookValue; break;
			case shop::OfferKind::Brows: Look.Brows = Of->LookValue; break;
			default: break;
			}
		}
		// The barber made the new look, so it's the one he's seen (his own work isn't a "new look" to him).
		const auto Barber = Life.Town.find(shop::KeeperOf("barber", O.Start));
		if (Barber != Life.Town.end())
		{
			Barber->second.LookSeen = town::LookHash(Look, Life.Closet);
		}
		static const char* const Closers[] = {
			"He spins the chair round to the mirror and waits for you to say it.",
			"He holds up the hand mirror behind your head, then brushes off your neck.",
			"He snaps the cape off, and the radio goes back to the ballgame.",
		};
		LastOutcome.Body = Sentence(Duration(Minutes)) + " in Lonnie's chair. " + Closers[static_cast<size_t>(net::DayOf(End)) % 3];
		return;
	}

	if (Venue == "gym")
	{
		// The workouts: one counted every 16 hours, its gain by how fit (and how tired) the hero came in.
		const double EnergyBefore = Life.Energy - LastOutcome.Energy;
		const double Was = Life.Fit.Level;
		bool Counted = false;
		bool Tried = false;
		bool Bag = false;
		bool Trainer = false;
		for (const shop::Offer* Of : Offers)
		{
			if (Of->Type != shop::OfferKind::Workout && Of->Type != shop::OfferKind::Training)
			{
				continue;
			}
			Tried = true;
			Life.Hunger = std::clamp(Life.Hunger - Of->Hunger, 0.0, 100.0);
			Life.Thirst = std::clamp(Life.Thirst - Of->Thirst, 0.0, 100.0);
			const double Gained = Counted ? 0.0 : body::Gain(Life.Fit, Of->FitnessBase, EnergyBefore, End);
			if (Gained > 0.0)
			{
				Life.Fit.Level = std::min(100.0, Life.Fit.Level + Gained);
				Life.Fit.LastCounted = End;
				Life.Fit.LastDecay = std::max(Life.Fit.LastDecay, End);
				++Life.Fit.Workouts;
				Counted = true;
			}
			if (Of->Id == "gym/bag")
			{
				Life.Fit.ClearHeadUntil = std::max(Life.Fit.ClearHeadUntil, End + 8.0 * 60.0);
				Bag = true;
			}
			if (Of->Type == shop::OfferKind::Training)
			{
				++Life.Fit.Sessions;
				Trainer = true;
			}
		}
		RefreshGear();
		std::string Body = Sentence(Duration(Minutes)) + " at IronHouse. ";
		if (Counted)
		{
			Body += "Fitness " + std::to_string(static_cast<int>(std::floor(Was))) + " \xE2\x86\x92 " + std::to_string(static_cast<int>(std::floor(Life.Fit.Level))) + ".";
		}
		else if (Tried)
		{
			Body += "No fitness from this one: the last workout that counted was under 16 hours ago.";
		}
		if (Bag)
		{
			Body += " A clear head for eight hours: bad beats land softer.";
		}
		if (Trainer)
		{
			Body += Life.Fit.Sessions == 3 ? " Rosa teaches you Box Breathing: four in, hold four, four out. Steadying at the table goes deeper and costs less Focus."
				: Life.Fit.Sessions < 3	   ? " Session " + std::to_string(Life.Fit.Sessions) + " of 3 with Rosa."
										   : " Rosa works you harder every time.";
		}
		LastOutcome.Body = Body;
		return;
	}

	if (Venue == "diner")
	{
		// The plates and the drinks: relief as the line cook makes it go (store::ReliefOf); the energy came with the skip.
		const double HungerWas = Life.Hunger;
		const double ThirstWas = Life.Thirst;
		double Relief = 0.0;
		bool Coffee = false;
		for (const shop::Offer* Of : Offers)
		{
			const double Hunger = Of->Type == shop::OfferKind::Meal && Of->Hunger > 0.0 ? Of->Hunger * Life.Perks.MealBoost : Of->Hunger;
			Relief += Hunger;
			Life.Hunger = std::clamp(Life.Hunger - Hunger, 0.0, 100.0);
			Life.Thirst = std::clamp(Life.Thirst - Of->Thirst, 0.0, 100.0);
			Coffee = Coffee || Of->Id == "diner/coffee";
		}
		if (Relief >= 40.0)
		{
			Life.RealMealAt = End; // Mom's "eat something real" waits a day (§5.4)
		}
		if (Coffee)
		{
			Life.CoffeeUntil = std::max(Life.CoffeeUntil, End + 120.0);
		}
		LastOutcome.Body = Changes(Life.Hunger - HungerWas, Life.Thirst - ThirstWas, LastOutcome.Energy) + (Relief >= 40.0 ? " A real meal, for once." : "");
		return;
	}
	LastOutcome.Body = Sentence(Duration(Minutes)) + (V ? " at " + V->Name + "." : ".");
}

void Session::ShopStep(double From, double To)
{
	using namespace session_shops_detail;
	// CheckCalendar, every stretch of the clock.
	// Ruby's refills while the hero sits a while with coffee, every half hour from when it was ordered (§5.4): the first
	// cup's +8 came with the order, the second cup gives +4 and the third +2 (and 10 off the thirst each). After that
	// the pot keeps coming and keeps the drain at 3 an hour (Life.CoffeeUntil, held by WaitAt), and that's all it does:
	// the hours at the counter cost what an awake hour with coffee costs, and the thirst climbs as awake.
	if (TimeSkip.Active && TimeSkip.Result.ActivityId.empty() && TimeSkip.Label == DinerWait && Life.CoffeeUntil > From)
	{
		const auto It = Life.Visits.find("diner");
		const double OrderAt = It != Life.Visits.end() ? It->second.LastAt : TimeSkip.From;
		const double Upto = std::min(To, TimeSkip.To);
		for (int K = 2; K <= 3; ++K)
		{
			const double At = OrderAt + 30.0 * static_cast<double>(K - 1);
			if (At > Upto)
			{
				break;
			}
			if (At > From)
			{
				Life.Energy = std::clamp(Life.Energy + (K == 2 ? 4.0 : 2.0), 0.0, 100.0);
				Life.Thirst = std::clamp(Life.Thirst - 10.0, 0.0, 100.0);
			}
		}
	}

	// Fitness fades without workouts (and an edit from outside, a test hook, lands here too).
	body::Decay(Life.Fit, To);
	const body::Effects Fresh = body::EffectsOf(Life.Fit, Owns("gym"));
	if (Fresh.DrainScale != BodyFx.DrainScale || Fresh.RestBonus != BodyFx.RestBonus || Fresh.ExhaustedAt != BodyFx.ExhaustedAt || Fresh.Calm != BodyFx.Calm)
	{
		RefreshGear();
	}

	// The pawn: its reminders as real pawnbrokers send them, then forfeits, then what's sold on.
	const double Slack = 1.0;
	auto Crosses = [&](double At) { return At > From - Slack && At <= To; };
	const double Grace = shop::PawnGraceDays * Day;
	std::vector<std::pair<std::string, std::string>> Texts;
	for (const shop::Ticket& Tk : Life.Pawn.Tickets)
	{
		if (Tk.DueAt + Grace < To)
		{
			continue; // forfeited below: no reminder for a ticket already gone
		}
		const std::string Key = shop::TicketNumber(Tk.Number) + ":" + std::to_string(Tk.Renewals);
		shop::Lot L;
		const std::string What = LotFor(*this, Tk.Lot, L) ? ShortName(L) : std::string("pledge");
		if (Crosses(Tk.DueAt - 3.0 * Day))
		{
			Texts.push_back({"pawn-due:" + Key, "Ticket #" + shop::TicketNumber(Tk.Number) + " (" + What + ") is due " + net::WeekdayName(net::DayOf(Tk.DueAt)) + ". " +
				Money(Tk.LoanCents * 120 / 100) + " redeems it."});
		}
		if (Crosses(Tk.DueAt))
		{
			Texts.push_back({"pawn-grace:" + Key, "Ticket #" + shop::TicketNumber(Tk.Number) + " is in grace until " + net::WeekdayName(net::DayOf(Tk.DueAt + Grace)) +
				". After that it's ours."});
		}
	}
	bool Changed = false;
	for (auto It = Life.Pawn.Tickets.begin(); It != Life.Pawn.Tickets.end();)
	{
		if (It->DueAt + Grace < To)
		{
			// Forfeited: his now, on his shelf (gear, garments) or in his case (the rest) at V, for 21 days.
			shop::Lot L;
			if (LotFor(*this, It->Lot, L))
			{
				shop::ShelfLot S;
				S.Lot = It->Lot;
				S.PriceCents = L.ValueCents;
				S.From = It->DueAt + Grace;
				S.Until = S.From + shop::PawnShelfDays * Day;
				Life.Pawn.Shelf.push_back(S);
			}
			It = Life.Pawn.Tickets.erase(It);
			Changed = true;
		}
		else
		{
			++It;
		}
	}
	for (auto It = Life.Pawn.Shelf.begin(); It != Life.Pawn.Shelf.end();)
	{
		if (It->Until <= To)
		{
			// Sold to someone else. The hero's mementos, awards and keepsakes are gone for good.
			const std::string Kind = It->Lot.substr(0, It->Lot.find(':'));
			if (!It->Auction && (Kind == "memento" || Kind == "award" || Kind == "keepsake"))
			{
				Life.Pawn.Away.insert(It->Lot);
			}
			It = Life.Pawn.Shelf.erase(It);
			Changed = true;
		}
		else
		{
			++It;
		}
	}
	if (Changed)
	{
		Owed(*this);
	}
	for (const auto& Tx : Texts)
	{
		StoryText(Tx.first, PawnName, Tx.second);
	}
}

void Session::ShopVisit(const std::string& Venue, Chips SpentCents)
{
	using namespace session_shops_detail;
	const double World = WorldMinutes();
	shop::Visit& V = Life.Visits[Venue];
	++V.Count;
	V.LastAt = World;
	V.SpentCents += SpentCents;
	// The block road counts the days a shop saw the player, not the purchases (next_segment_spec §3.3).
	sponsor::NoteVisit(Life.Sponsors, Venue, World);
	if (Venue == "store")
	{
		// The Lucky Penny's usual: what's been bought there most.
		int Most = 0;
		for (const auto& Had : Life.Bought)
		{
			if (Had.second > Most)
			{
				Most = Had.second;
				V.Usual = Had.first;
			}
		}
	}
	// The keeper on shift knows the face a little better (once a day), and has seen how the player looks today.
	const std::string Keeper = shop::KeeperOf(Venue, World);
	if (!Keeper.empty())
	{
		town::Bond& B = Life.Town[Keeper];
		const int Today = net::DayOf(World);
		if (B.LastDay != Today)
		{
			B.Familiarity = std::min(100, B.Familiarity + 1);
			++B.Visits;
			B.LastDay = Today;
		}
		B.LookSeen = town::LookHash(Person.Appearance, Life.Closet);
	}
}

void Session::ShopClerk(store::ClerkContext& Ctx) const
{
	using namespace session_shops_detail;
	// The Lucky Penny's counter remembers the player by their visits (Checkout records them), not by the ledger's
	// labels; a career from before the visits were kept keeps the ledger's answer (ClerkSays reads it first).
	const auto It = Life.Visits.find("store");
	if (It != Life.Visits.end() && It->second.Count > 0 && It->second.LastAt >= 0.0)
	{
		Ctx.SinceVisit = std::max(0.0, Ctx.World - It->second.LastAt);
	}
	// Who's on the register by the roster (Ray days, Benny nights), and what he notices: a cut of Lonnie's in the last
	// three days, or a look he hasn't rung up before (his bond's LookSeen, kept by ShopVisit at every checkout).
	Ctx.Clerk = shop::KeeperOf("store", Ctx.World);
	Ctx.FreshCut = Life.Groom.Cuts > 0 && Ctx.World >= Life.Groom.CutAt && Ctx.World - Life.Groom.CutAt < 3.0 * Day;
	const auto Seen = Life.Town.find(Ctx.Clerk);
	Ctx.NewLook = !Ctx.FreshCut && Seen != Life.Town.end() && Seen->second.LookSeen != 0 && Seen->second.LookSeen != town::LookHash(Person.Appearance, Life.Closet);
}

void Session::ShopStream(kast::Inputs& In) const
{
	using namespace session_shops_detail;
	// Kast (§5.1, §5.2): follows x(1 + 0.05 Sharp); tips x(1 + 0.03 Sharp) x(1 + 0.02 Style). The stream reads the gear's
	// Follow and Tips as (1 + x) multipliers (Kast.cpp), so they're folded in there. Fresh from nowhere: no change.
	const double Fresh = Sharp();
	const double Style = wardrobe::StyleOf(Life.Closet);
	if (Fresh > 0.0)
	{
		In.Gear.Follow = (1.0 + In.Gear.Follow) * (1.0 + 0.05 * Fresh) - 1.0;
	}
	if (Fresh > 0.0 || Style > 0.0)
	{
		In.Gear.Tips = (1.0 + In.Gear.Tips) * (1.0 + 0.03 * Fresh) * (1.0 + 0.02 * Style) - 1.0;
	}
}
} // namespace ss
