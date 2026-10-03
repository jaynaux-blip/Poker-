#pragma once

#include "ShortStack/UI/Canvas.h"

#include <string>

namespace ss
{
namespace ui
{
/** Profile pictures on RiverLine: an icon on a colored disc, in a frame. */
enum class AvatarIcon : int
{
	Initials,
	Shark,
	Fish,
	Whale,
	Owl,
	Cat,
	Fox,
	Bear,
	Panda,
	Skull,
	Crown,
	Gem,
	Spade,
	Heart,
	Club,
	Diamond,
	Dice,
	Clover,
	Rocket,
	Robot,
	Alien,
	Shades,
	Flame,
	Bolt,
	Chip,
	Cowboy,
	Moon,
	AceCard,
	Coffee,
	Headphones,
	Pizza,
	EightBall,
	Wolf,
	Tiger,
	Ghost, // the rival's alone
	Count,
};

enum class AvatarFrame : int
{
	None,
	Ring,  // a colored rim
	Chip,  // a poker chip's edge spots
	Gold,  // Team RiverLine
	Neon,  // glowing: the player, the rival
	Bracelet, // a bracelet winner: gold links all the way round, a plaque at the bottom (and a ring winner's stone on top)
	Gem,      // a ring winner: a gold band with their ring's stone set on top
};

struct AvatarSpec
{
	AvatarIcon Icon = AvatarIcon::Initials;
	uint32_t Bg = 0x27d3c3;  // disc gradient, light side
	uint32_t Bg2 = 0x0e7490; // dark side
	uint32_t Ink = 0xffffff; // the icon
	AvatarFrame Frame = AvatarFrame::None;
	uint32_t Rim = 0xffffff; // Ring and Neon frames
	std::string Initials;
	// Champions (Bracelet and Gem frames).
	int Bracelets = 0;
	int Rings = 0;
	uint32_t Stone = 0xdff6ff; // the latest ring's stone
	uint32_t Plate = 0x2a2a35; // the latest bracelet's enamel
	bool Halo = false;         // the Neon glow (in Rim) under the frame: the player, the rival
};

/** The avatar a screen name picked: an icon that fits the name when one does ("ElTiburon" is a shark), else one of the set. */
SHORTSTACKCORE_API AvatarSpec AvatarFor(const std::string& Name);
/** Draws an avatar of radius R centered at (Cx, Cy). */
SHORTSTACKCORE_API void DrawAvatar(Canvas& C, float Cx, float Cy, float R, const AvatarSpec& A);
SHORTSTACKCORE_API const char* AvatarIconName(AvatarIcon I);
} // namespace ui
} // namespace ss
