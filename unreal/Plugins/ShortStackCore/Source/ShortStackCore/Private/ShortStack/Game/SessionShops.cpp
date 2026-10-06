#include "ShortStack/Game/Session.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Body.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Shop.h"
#include "ShortStack/Game/Wardrobe.h"

#include <algorithm>

// The Session's Northside seam (the realism update's spec, §7.2): the shops (pay, service, settle, wait, wear, donate,
// board) and Northside Pawn & Loan (§5.8), in their own translation unit so Session.cpp stays closed after Phase A.
//
// Phase A: the calls exist and link. What can be had says why not ("Closed."), nothing is charged and no skip starts;
// the hooks Session.cpp calls (SettleShop, ShopStep) do nothing yet. What is already real: ShopOpenFor (on Shop.h's
// hours), the store's visits (ShopVisit, ShopClerk), the look on camera (ShopStream: no change while Sharp and Style
// are 0), WorldSeed, Fitness, Sharp and PawnHolds. core-shops' Phase B fills the rest.
//
// Notes for Phase B (Session.cpp is closed): a "shop:" skip is asleep to the calendar (Update passes Awake only for an
// empty activity id), so its energy is the offers' (Outcome.Energy, taken whole: FinishSkip uses Rest 1.0 for it);
// a wait that should drain as awake (Ruby's "sit a while", the bench, the shelter) starts its skip with an empty
// activity id instead, and the coffee and fitness apply on their own. A change of fitness calls RefreshGear (BodyFx).
// The migration (shop::AfterParse) goes by the lines the save had (Life.ShopLinesRead, set by shop::ReadLine), never by
// fields at their defaults: a new career's fitness and grooming are still at their defaults when first saved.

namespace ss
{
namespace session_shops_detail
{
/** Phase A's answer to anything that would buy, wear or wait: nothing on the block is open for business yet. */
const char* const NotYet = "Closed.";
} // namespace session_shops_detail

// ------------------------------------------------------------------ the shops

bool Session::ShopOpenFor(const std::string& Venue) const
{
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
	const shop::Offer* O = shop::FindOffer(OfferId);
	if (!O)
	{
		return "Not on offer.";
	}
	return session_shops_detail::NotYet;
}

Chips Session::ShopTotal(const ShopOrder& /*O*/) const
{
	return 0;
}

std::string Session::ShopPay(const ShopOrder& /*O*/)
{
	return session_shops_detail::NotYet;
}

std::string Session::ShopService(const ShopOrder& /*O*/)
{
	return session_shops_detail::NotYet;
}

std::string Session::WaitAt(const std::string& /*Venue*/, double /*UntilWorld*/)
{
	return session_shops_detail::NotYet;
}

std::string Session::Wear(const std::string& GarmentId)
{
	return Life.Closet.Owned.count(GarmentId) > 0 ? session_shops_detail::NotYet : "You don't own that.";
}

std::string Session::TakeOff(int WardrobeSlot)
{
	return WardrobeSlot >= 0 && WardrobeSlot < wardrobe::SlotCount ? session_shops_detail::NotYet : "Nothing there.";
}

std::string Session::Donate(const std::string& GarmentId)
{
	return Life.Closet.Owned.count(GarmentId) > 0 ? session_shops_detail::NotYet : "You don't own that.";
}

std::string Session::BoardBus(const std::string& /*LiveId*/)
{
	return "The 14 \xC2\xB7 Embercrest. Register for an event first (Burner).";
}

uint64_t Session::WorldSeed() const
{
	// The living world's seed (saved with the career), spread over 64 bits (splitmix64's finalizer).
	uint64_t Z = static_cast<uint64_t>(LivingWorld.Seed()) + 0x9e3779b97f4a7c15ull;
	Z = (Z ^ (Z >> 30)) * 0xbf58476d1ce4e5b9ull;
	Z = (Z ^ (Z >> 27)) * 0x94d049bb133111ebull;
	return Z ^ (Z >> 31);
}

double Session::Fitness() const
{
	return Life.Fit.Level;
}

double Session::Sharp() const
{
	return body::Sharp(Life.Groom, WorldMinutes());
}

// ------------------------------------------------------------------ the pawn

std::vector<shop::Lot> Session::PawnLots() const
{
	return {};
}

std::vector<shop::Lot> Session::PawnShelf() const
{
	return {};
}

std::string Session::PawnSell(const std::string& /*LotId*/)
{
	return session_shops_detail::NotYet;
}

std::string Session::PawnLoan(const std::string& /*LotId*/)
{
	return session_shops_detail::NotYet;
}

std::string Session::PawnRedeem(int /*Ticket*/)
{
	return session_shops_detail::NotYet;
}

std::string Session::PawnRenew(int /*Ticket*/)
{
	return session_shops_detail::NotYet;
}

std::string Session::PawnBuy(const std::string& /*LotId*/)
{
	return session_shops_detail::NotYet;
}

bool Session::PawnHolds(const std::string& LotId) const
{
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
	// FinishSkip, for a "shop:<venue>:<offer ids>" skip (TimeSkip.Result still holds it): the look, the fitness, the
	// meal's relief and the outcome line. Phase A: no shop starts a skip yet.
}

void Session::ShopStep(double /*From*/, double /*To*/)
{
	// CheckCalendar, every stretch of the clock: fitness decay, the pawn's reminders, forfeits and shelf expiry.
	// Phase A: nothing to do yet.
}

void Session::ShopVisit(const std::string& Venue, Chips SpentCents)
{
	shop::Visit& V = Life.Visits[Venue];
	++V.Count;
	V.LastAt = WorldMinutes();
	V.SpentCents += SpentCents;
}

void Session::ShopClerk(store::ClerkContext& Ctx) const
{
	// The Lucky Penny's counter remembers the player by their visits (Checkout records them), not by the ledger's
	// labels; a career from before the visits were kept keeps the ledger's answer (ClerkSays reads it first).
	const auto It = Life.Visits.find("store");
	if (It != Life.Visits.end() && It->second.Count > 0 && It->second.LastAt >= 0.0)
	{
		Ctx.SinceVisit = std::max(0.0, Ctx.World - It->second.LastAt);
	}
}

void Session::ShopStream(kast::Inputs& In) const
{
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
