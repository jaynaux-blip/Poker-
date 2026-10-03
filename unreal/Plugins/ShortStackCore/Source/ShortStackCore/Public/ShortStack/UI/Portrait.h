#pragma once

#include "ShortStack/Game/Hero.h"
#include "ShortStack/UI/Canvas.h"

namespace ss
{
namespace ui
{
/** How the portrait is lit: a warm key from the left, a colored rim from behind on the right. */
struct PortraitLight
{
	Color Key = Hex(0xffe2c4);
	Color Rim = Hex(0x27d3c3);
	float RimStrength = 0.75f;
};

/**
 * The character as an illustrated head-and-shoulders portrait (the creator's live preview, the ID card,
 * the HUD). Everything in the look shows: face shape, skin, eyes, brows, the twelve hairstyles, facial
 * hair, build, the jacket and its color, glasses and headwear, with age lines and greying from the
 * character's age. Centered on X with the eye line at Y; Scale 1 draws the head about 210 units tall
 * and the shoulders down to about 330 units below the eyes (clip the bottom where the frame ends).
 * Time animates breathing, a blink and the eyes (pass 0 for a still).
 */
SHORTSTACKCORE_API void DrawPortrait(Canvas& C, const hero::Character& Who, float X, float Y, float Scale, double Time, const PortraitLight& Light = PortraitLight());

/** A full-length silhouette for the height chart: feet on FeetY, PxPerCm units per centimeter. */
SHORTSTACKCORE_API void DrawFigure(Canvas& C, const hero::Look& L, float X, float FeetY, float PxPerCm, const Color& Fill, const Color& Edge);
} // namespace ui
} // namespace ss
