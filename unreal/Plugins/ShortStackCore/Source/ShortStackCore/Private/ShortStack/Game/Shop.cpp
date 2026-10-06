#include "ShortStack/Game/Shop.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/World.h"

// Phase A stubs (the realism update's spec, §7.1): every declaration links and returns its default, so core-ui,
// people-core and ue-frame build against Shop.h from day 1. core-shops' Phase B replaces these bodies with §5's venues,
// hours and offers, the pawn's rules (§5.8), and the save lines and migration (§7.3).

namespace ss
{
namespace shop
{
const std::vector<Venue>& Venues()
{
	static const std::vector<Venue> None;
	return None;
}

const Venue* FindVenue(const std::string& Id)
{
	for (const Venue& V : Venues())
	{
		if (V.Id == Id)
		{
			return &V;
		}
	}
	return nullptr;
}

bool IsOpen(const Venue& /*V*/, double /*World*/)
{
	return false;
}

bool IsStaffed(const Venue& /*V*/, double /*World*/)
{
	return false;
}

double NextOpen(const Venue& /*V*/, double World)
{
	return World;
}

double ClosesAt(const Venue& /*V*/, double World)
{
	return World;
}

std::string HoursLine(const Venue& /*V*/, double /*World*/)
{
	return std::string();
}

std::string HoursSign(const Venue& /*V*/)
{
	return std::string();
}

const std::vector<Offer>& Offers(const std::string& /*Venue*/)
{
	static const std::vector<Offer> None;
	return None;
}

const Offer* FindOffer(const std::string& /*Id*/)
{
	return nullptr;
}

Chips GearValue(const gear::Item& /*I*/)
{
	return 0;
}

Chips AwardValue(const world::Award& /*A*/)
{
	return 0;
}

Lot KeepsakeOf(hero::Background /*B*/)
{
	return Lot();
}

Chips RedeemCents(const Ticket& /*T*/, double /*World*/)
{
	return 0;
}

Chips RenewCents(const Ticket& /*T*/)
{
	return 0;
}

std::vector<ShelfLot> UsedStock(int /*Week*/, uint64_t /*Seed*/, uint64_t /*GearKinds*/)
{
	return {};
}

void Consign(PawnBook& /*B*/, const std::vector<std::string>& /*LostGearIds*/, double /*World*/, uint64_t /*Seed*/)
{
}

void WriteLines(const life::State& /*Life*/, std::string& /*Out*/)
{
}

bool ReadLine(const std::vector<std::string>& /*Fields*/, life::State& /*Life*/)
{
	return false;
}

void AfterParse(SaveData& /*Data*/)
{
}
} // namespace shop
} // namespace ss
