#include "ShortStack/Game/ChipSet.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Live.h"

// The chips on the felt (next_segment_spec §6.2 B). core-tableui.
//
// Phase A, day 1: stubs that compile and change nothing: no set, no layout, and every stack read exactly.

namespace ss
{
namespace chipset
{
const std::vector<Denom>& Set(Room)
{
	static const std::vector<Denom> Empty{};
	return Empty;
}

std::vector<Denom> InPlay(Room, const std::vector<Level>&, int, int)
{
	return {};
}

std::vector<Column> Lay(Chips, const std::vector<Denom>&, Style)
{
	return {};
}

std::vector<Column> StartingBag(const live::EventTemplate&)
{
	return {};
}

Read ReadStack(Chips Exact, const std::vector<Denom>&, bool)
{
	Read Out;
	Out.Low = Exact;
	Out.High = Exact;
	Out.Shown = Exact;
	Out.Exact = true;
	return Out;
}

std::string Spoken(Chips)
{
	return std::string();
}

std::string InBb(Chips, Chips)
{
	return std::string();
}
} // namespace chipset
} // namespace ss
