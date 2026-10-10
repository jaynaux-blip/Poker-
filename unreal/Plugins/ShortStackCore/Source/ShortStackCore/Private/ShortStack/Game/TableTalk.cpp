#include "ShortStack/Game/TableTalk.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Life.h"

// What the card room's people say (next_segment_spec §4.5 to §4.8). core-room.
//
// Phase A, day 1: stubs that compile and say nothing (every Said is empty, the inventory too). CaptionName hands back
// the name the room already knows.

namespace ss
{
namespace talk
{
Said Line(Moment, const Context&)
{
	return Said();
}

Said Dealer(const std::string&, const Context&)
{
	return Said();
}

Said Floor(const std::string&, const std::map<std::string, std::string>&, uint32_t)
{
	return Said();
}

Said Cast(const std::string&, Moment, uint32_t)
{
	return Said();
}

std::string SpokenHand(const std::vector<Card>&)
{
	return std::string();
}

std::string CaptionName(const std::string&, const std::string& Name, int)
{
	return Name;
}

std::vector<std::pair<std::string, std::string>> Inventory()
{
	return {};
}
} // namespace talk
} // namespace ss
