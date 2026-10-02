#pragma once

#include "ShortStack/Game/Gear.h"
#include "ShortStack/Game/Kast.h"
#include "ShortStack/UI/Canvas.h"

namespace ss
{
namespace ui
{
/**
 * Pictures for GearDrop and Kast, all vector: the products on the store's cards, the "your setup" desk that fills
 * up as the player buys things, Kast's emotes, and the facecam: the player on stream, lit (or not) by whatever
 * lights they own, reacting to the cards.
 */
namespace streamart
{
/** A product, centered in R. */
SHORTSTACKCORE_API void Product(Canvas& C, gear::Art A, const Rect& R, uint32_t Accent, double Time);
/** The desk with what's owned on it (monitors, the tower, camera, mic, lights, plant, coffee), lit by the LED kit's Leds. */
SHORTSTACKCORE_API void Desk(Canvas& C, const Rect& R, const gear::Owned& Owned, bool Live, double Time, const gear::Glow& Leds = gear::Glow());
/** An emote (kast::Emote) in the square at (X, Y). */
SHORTSTACKCORE_API void Emote(Canvas& C, int E, float X, float Y, float Size);

struct Cam
{
	gear::Effects Gear;
	bool Headphones = false;
	kast::Mood Face = kast::Mood::Focus;
	double FaceAge = 10.0; // seconds since the face changed
	bool Talking = false;
	double Time = 0.0;
	uint32_t Hoodie = 0x9b5cff;
	bool Live = true;
	gear::Glow Leds; // the room's LEDs: a coloured wall behind, a rim of light around the streamer
};
/** The facecam: the streamer at the desk. With a green screen the background is left out (it sits over the game). */
SHORTSTACKCORE_API void Facecam(Canvas& C, const Rect& R, const Cam& Look);
} // namespace streamart
} // namespace ui
} // namespace ss
