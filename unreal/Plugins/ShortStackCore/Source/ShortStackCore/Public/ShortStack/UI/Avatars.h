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
};

/** The avatar a screen name picked: an icon that fits the name when one does ("ElTiburon" is a shark), else one of the set. */
SHORTSTACKCORE_API AvatarSpec AvatarFor(const std::string& Name);
/** Draws an avatar of radius R centered at (Cx, Cy). */
SHORTSTACKCORE_API void DrawAvatar(Canvas& C, float Cx, float Cy, float R, const AvatarSpec& A);
SHORTSTACKCORE_API const char* AvatarIconName(AvatarIcon I);
} // namespace ui
} // namespace ss
