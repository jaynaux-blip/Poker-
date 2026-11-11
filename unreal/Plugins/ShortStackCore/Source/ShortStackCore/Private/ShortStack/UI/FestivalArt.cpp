// Update 4, Festival Season's look (SPEC v1.7 §2.7): the ten looks, the palette swap, the trophies, belts, pins, word-marks and felt
// marks (UH/FestivalArt.h), and the art's strings (FestArtInventory). F-look's file from G1.
//
// Phase A (§6 PA-44; the lead): stubs that keep today's behaviour. Every look is empty, LookFor finds none, Dressed returns its base
// and the art draws nothing; nothing calls them before G1.
#include "ShortStack/UI/FestivalArt.h"

namespace ss
{
namespace ui
{
namespace festart
{
const FestivalLook& LookOf(fest::Brand /*B*/)
{
	static const FestivalLook FestNoLook;
	return FestNoLook;
}

const FestivalLook* LookFor(const net::SeriesInfo& /*S*/)
{
	return nullptr;
}

Skin Dressed(const Skin& Base, const FestivalLook& /*L*/)
{
	return Base;
}

void FestTrophy(Canvas& /*C*/, fest::Brand /*B*/, float /*Cx*/, float /*Cy*/, float /*Size*/, double /*Time*/, bool /*Lod*/, const std::vector<int>& /*Fields*/, int /*Tier*/)
{
}

void FestBelt(Canvas& /*C*/, fest::Brand /*B*/, float /*Cx*/, float /*Cy*/, float /*Size*/, double /*Time*/, bool /*Lod*/)
{
}

void FestPin(Canvas& /*C*/, fest::Brand /*B*/, int /*Grade*/, world::FestMark /*Mark*/, float /*Cx*/, float /*Cy*/, float /*Size*/)
{
}

void FestWordMark(Canvas& /*C*/, const FestivalLook& /*L*/, const std::string& /*Text*/, float /*X*/, float /*Y*/, float /*Size*/, double /*Time*/, bool /*Still*/)
{
}

void FeltMark(Canvas& /*C*/, const FestivalLook& /*L*/, float /*Cx*/, float /*Cy*/, float /*Size*/, bool /*Suppressed*/)
{
}
} // namespace festart
} // namespace ui
} // namespace ss
