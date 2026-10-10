#include "ShortStack/Game/Sizing.h"
#include "../StrictFloat.h"

// Bet sizing (next_segment_spec §6.2 D). core-tableui.
//
// Phase A, day 1: stubs that compile and change nothing: no presets, the minimum as the default, steps that stay put.

namespace ss
{
namespace sizing
{
Spot SpotOf(const Hand&, int)
{
	return Spot();
}

std::vector<Preset> Presets(const Spot&)
{
	return {};
}

Chips Default(const Spot& S)
{
	return S.MinTo;
}

Chips Step(const Spot&, Chips To, int)
{
	return To;
}

int Snap(const Spot&, Chips, int)
{
	return -1;
}

std::string Describe(const Spot&, Chips)
{
	return std::string();
}
} // namespace sizing
} // namespace ss
