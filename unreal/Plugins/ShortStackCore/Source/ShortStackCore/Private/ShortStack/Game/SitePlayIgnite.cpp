// Update 3 (SPEC §2.6, §7.3): Ignite's loops, core-siteplay's file. Sparks and the Prize Counter, the Flame, Dares, the
// Spark Wheel, Crackers, the Wristband, Ignite Levels, looks, Headliners, Beef, the Ouch Bonus, the Sticker Book and theme
// nights. Phase A, day 1: stubs (every claim refused, the catalogs empty).
#include "ShortStack/Game/SitePlay.h"

#include "ShortStack/Game/Life.h"

#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace site
{
namespace play
{
std::string Spin(Book& /*B*/, life::State& /*L*/, uint64_t /*WorldSeed*/, double /*World*/)
{
	return "Not yet.";
}

std::string Open(Book& /*B*/, life::State& /*L*/, uint64_t /*WorldSeed*/, const std::string& /*BoxId*/, double /*World*/)
{
	return "Not yet.";
}

std::string ClaimDare(Book& /*B*/, life::State& /*L*/, int /*Slot*/, double /*World*/)
{
	return "Not yet.";
}

std::string ClaimTier(Book& /*B*/, life::State& /*L*/, int /*Tier*/, double /*World*/)
{
	return "Not yet.";
}

std::string Buy(Book& /*B*/, life::State& /*L*/, int /*Item*/, double /*World*/)
{
	return "Not yet.";
}

std::string Equip(Book& /*B*/, int /*Slot*/, int /*Look*/)
{
	return "Not yet.";
}

const std::vector<DareDef>& DareCatalog()
{
	static const std::vector<DareDef> IgniteDares;
	return IgniteDares;
}

const std::vector<std::pair<int, double>>& WheelTable(int /*Ring*/, int8_t /*Band*/)
{
	static const std::vector<std::pair<int, double>> IgniteWheel;
	return IgniteWheel;
}

const std::vector<PrizeDef>& PrizeCounter()
{
	static const std::vector<PrizeDef> IgnitePrizes;
	return IgnitePrizes;
}

int64_t LevelNeeds(int /*Level*/)
{
	return 0;
}

int8_t BandOf(const Book& /*B*/, const std::vector<HistoryEntry>& /*History*/)
{
	return 0;
}
} // namespace play
} // namespace site
} // namespace ss
