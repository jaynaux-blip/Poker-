// Update 3 (SPEC §2.6 F7, §7.3): FUSE's pure parts, core-fuse's file. The prize tables at any edge, the fee the rake hooks
// see, the keyed draw per tier ordinal (tickets counted), the id, the synthetic 3-entrant event with the exact pool, the
// seats and the payout. Phase A, day 1: stubs (no FUSE id exists, so nothing reaches them).
#include "ShortStack/Game/Fuse.h"

#include <array>
#include <string>
#include <vector>

namespace ss
{
namespace fuse
{
const std::array<Row, 7>& Table()
{
	static const std::array<Row, 7> FuseRows{};
	return FuseRows;
}

double Edge()
{
	return 0.08;
}

Chips FeeOf(int /*Tier*/)
{
	return 0;
}

bool IsFuse(const std::string& /*EventId*/)
{
	return false;
}

int Draw(uint64_t /*WorldSeed*/, int /*Tier*/, int /*N*/)
{
	return 0;
}

std::string IdFor(int /*Tier*/, double /*World*/, int /*N*/)
{
	return std::string();
}

ss::LobbyEvent EventFor(int /*Tier*/, int /*M*/, double /*World*/, int /*N*/)
{
	return ss::LobbyEvent();
}

std::array<Chips, 3> Payout(int /*Tier*/, int /*M*/)
{
	return {};
}

std::vector<std::string> SeatsFor(const world::World* /*W*/, uint64_t /*WorldSeed*/, int /*Tier*/, int /*N*/, double /*World*/)
{
	return {};
}
} // namespace fuse
} // namespace ss
