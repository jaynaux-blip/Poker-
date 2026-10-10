#include "ShortStack/UI/SponsorArt.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Hero.h"

// The brands' art (next_segment_spec §3.9). core-sponsor.
//
// Phase A, day 1: stubs that compile and draw nothing.

namespace ss
{
namespace ui
{
namespace sponsorart
{
void Mark(Canvas&, const std::string&, const Rect&)
{
}

void Patch(Canvas&, const std::string&)
{
}

void Polaroid(Canvas&, const hero::Look&, const std::string&)
{
}

void AmbassadorBanner(Canvas&, const std::string&, const std::string&)
{
}
} // namespace sponsorart
} // namespace ui
} // namespace ss
