#include "ShortStack/Game/Wardrobe.h"
#include "../StrictFloat.h"

// Phase A stubs (the realism update's spec, §7.1): every declaration links and returns its default (an empty catalog,
// an unseeded closet), so core-ui and looks build against Wardrobe.h from day 1. core-shops' Phase B replaces these
// bodies with the catalog, the closet's seeding and collapse, Style, and the thrift's racks and prices (§5.2).

namespace ss
{
namespace wardrobe
{
const std::vector<Garment>& Catalog()
{
	static const std::vector<Garment> None;
	return None;
}

const Garment* Find(const std::string& Id)
{
	for (const Garment& G : Catalog())
	{
		if (G.Id == Id)
		{
			return &G;
		}
	}
	return nullptr;
}

Closet SeedFrom(const hero::Look& /*L*/)
{
	return Closet();
}

void ApplyTo(const Closet& /*C*/, hero::Look& /*L*/)
{
}

double StyleOf(const Closet& /*C*/)
{
	return 0.0;
}

std::vector<std::string> ThriftStock(int /*Week*/, uint64_t /*Seed*/, uint64_t /*KindsAvailable*/)
{
	return {};
}

Chips ThriftPrice(const Garment& G, double /*World*/)
{
	return G.PriceCents;
}
} // namespace wardrobe
} // namespace ss
