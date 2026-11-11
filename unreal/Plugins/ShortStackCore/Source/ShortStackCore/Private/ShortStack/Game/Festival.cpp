// Update 4, Festival Season (SPEC v1.7 §2.1-§2.4, §2.9.1, §2.10): fest::, the ten festivals as pure functions of code and the clock.
// F-net's file from G1 (the calendar, phases, All/FestivalOf/LiveOn/LastEnd/SeatsWon/RecordOf/Marks, the Pass's festival).
//
// Phase A (§6 PA-6, the lead): stubs that keep today's behaviour. All() is empty, so no series is a festival and every query
// answers "none": FestivalOf, LiveOn, Next and PassFestival null, PhaseOf Off, Marks empty, RecordOf false, SeatsWon 0,
// IsTitleTemplate false. The brand table and IsChampionship are the spec's fixed facts (nothing reads them before G1). Step A-2 (the
// lead; LEAD_CALLS Q-5): YourLine is not shown and Inventory is empty. The three other inventories' stubs sit in their G1 owners'
// files, where their bodies land: TextInventory in G/SessionFest.cpp (F-world), FestUiInventory in UI/RiverLineFestival.cpp
// (F-hubs) and FestArtInventory in UI/FestivalArt.cpp (F-look).
#include "ShortStack/Game/Festival.h"

#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sites.h"

#include <string>
#include <vector>

namespace ss
{
namespace fest
{
namespace fest_detail
{
/** The brand keys in Brand's order (None first). */
const char* const FestBrandKeys[] = {"", "rcop", "ring", "tco", "slam", "feign", "push", "writ", "grit", "pennant", "reign"};
static_assert(sizeof(FestBrandKeys) / sizeof(FestBrandKeys[0]) == static_cast<size_t>(Brand::Count), "one key a brand");
} // namespace fest_detail

Brand BrandOf(const std::string& Key)
{
	for (int B = 1; B < static_cast<int>(Brand::Count); ++B)
	{
		if (Key == fest_detail::FestBrandKeys[B])
		{
			return static_cast<Brand>(B);
		}
	}
	return Brand::None;
}

const char* BrandKey(Brand B)
{
	return B > Brand::None && B < Brand::Count ? fest_detail::FestBrandKeys[static_cast<int>(B)] : "";
}

bool IsChampionship(Brand B)
{
	return B == Brand::Rcop || B == Brand::Pennant || B == Brand::Reign;
}

bool IsTitleTemplate(const std::string& /*TemplateId*/)
{
	// Phase A: All() is empty, so no edition has TitleIds to hold the template (G1: F-net, through FestivalOf).
	return false;
}

const std::vector<Festival>& All()
{
	// Phase A: no festival yet (G1 builds the list from net::Sites(), every series with SeriesInfo::Festival).
	static const std::vector<Festival> FestNone;
	return FestNone;
}

std::vector<const Festival*> Festivals(int /*FromDay*/, int /*ToDay*/)
{
	return {};
}

const Festival* FestivalOf(const std::string& /*SeriesId*/)
{
	return nullptr;
}

const Festival* LiveOn(net::SiteId /*Site*/, int /*Day*/)
{
	return nullptr;
}

const Festival* Next(int /*Day*/)
{
	return nullptr;
}

double LastEnd(const Festival& F)
{
	// Phase A: the Main's start stands in (no festival exists, so nothing asks).
	return F.MainStart;
}

Phase PhaseOf(const Festival& /*F*/, double /*WorldMin*/, const world::World* /*W*/)
{
	return Phase::Off;
}

FieldRecord RecordOf(const net::Network& /*N*/, const net::EventInstance& /*E*/, const world::World* /*W*/)
{
	return FieldRecord();
}

int SeatsWon(const Festival& /*F*/, double /*Now*/)
{
	return 0;
}

double RacePoints(int Place, int Entries, Chips BuyInCents, PayoutCurve /*Curve*/)
{
	// Phase A: net::Points (every Standard event's points but the four smallest fields'); G1 cuts it at PaidPlaces(Entries, Curve).
	return net::Points(Place, Entries, BuyInCents);
}

std::vector<life::Mark> Marks(double /*From*/, double /*To*/, double /*Now*/)
{
	return {};
}

const Festival* PassFestival(net::SiteId /*Site*/, double /*World*/)
{
	return nullptr;
}

int PassNeeds(const Festival& F, const net::InviteGate& Gate)
{
	// §2.10 item 1: a brand's first edition invites on the two rents alone; from the second, the gate's FestivalEvents.
	return F.Edition == 1 ? 0 : Gate.FestivalEvents;
}

YourLineView YourLine(const Festival& /*F*/, const world::World* /*W*/)
{
	return YourLineView();
}

std::vector<std::string> Inventory()
{
	return {};
}
} // namespace fest
} // namespace ss
