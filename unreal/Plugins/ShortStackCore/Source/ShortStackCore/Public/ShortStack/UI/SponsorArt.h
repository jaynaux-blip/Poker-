#pragma once

#include "ShortStack/UI/Canvas.h"

#include <string>

namespace ss
{
namespace hero
{
struct Look;
}

namespace ui
{
/**
 * The brands' art (next_segment_spec §3.9, §1.7): sponsorart::Mark is the one source of a brand's logo, for the Kast
 * overlay, Mail, the patches, the ambassador banner and can labels. Fictional brands only.
 *
 * Bodies: core-sponsor (SponsorArt.cpp). Phase A, day 1: stubs that compile and draw nothing.
 */
namespace sponsorart
{
SHORTSTACKCORE_API void Mark(Canvas& C, const std::string& BrandId, const Rect& R);
/** A 512 x 340 sheet. */
SHORTSTACKCORE_API void Patch(Canvas& C, const std::string& BrandId);
/**
 * A brand's patch colours (0xRRGGBB) as Patch draws them and the art's atlas stitches them: the twill's ground, and the
 * thread of the merrowed edge and the word mark, always the one of the brand's two colours that reads on that ground.
 * False (and nothing set) for an unknown brand. The game's block-out patches take theirs from here.
 */
SHORTSTACKCORE_API bool PatchColors(const std::string& BrandId, uint32_t& Ground, uint32_t& Thread);
SHORTSTACKCORE_API void Polaroid(Canvas& C, const hero::Look& L, const std::string& Caption);
SHORTSTACKCORE_API void AmbassadorBanner(Canvas& C, const std::string& Name, const std::string& Record);
} // namespace sponsorart
} // namespace ui
} // namespace ss
