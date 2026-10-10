#pragma once

#include "ShortStack/Game/Badges.h"
#include "ShortStack/UI/Canvas.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ss
{
namespace life
{
struct RoomLeader;
}

namespace ui
{
/**
 * The badges' art (next_segment_spec §5.5, §5.6): the glyphs (a tier changes the mark, not just its colour), the
 * badge rail, the atlas the room's plates sample (one cell per variant), the Players Club card the desk prints, and the
 * month's leaders for the room's screens. Badge colours are named tokens here.
 *
 * Bodies: core-badges (BadgeArt.cpp). Phase A, day 1: stubs that compile and draw nothing.
 */
namespace badgeart
{
SHORTSTACKCORE_API void Draw(Canvas& C, const badge::Badge& B, float Cx, float Cy, float Size, double Time, bool Glint = false);
/** Returns the width drawn. */
SHORTSTACKCORE_API float Rail(Canvas& C, const badge::Set& S, uint8_t Context, float X, float Cy, float Size, int Max, bool Labels);
/** Draws every variant into cells of Cell units; returns how many. */
SHORTSTACKCORE_API int Atlas(Canvas& C, float Cell);
/** The badge's cell in the atlas (-1: none). */
SHORTSTACKCORE_API int AtlasCell(const badge::Badge& B);
struct ClubCardInfo
{
	std::string Name;
	std::string ScreenName;
	std::string Country;
	int Tier = 0;
	int Visits = 0;
	int Wins = 0;
	int SinceDay = 0;
	bool New = false;
};
SHORTSTACKCORE_API void ClubCard(Canvas& C, const Rect& R, const ClubCardInfo& Info, double Now);
SHORTSTACKCORE_API void RoomLeaders(Canvas& C, const Rect& R, const std::vector<life::RoomLeader>& Rows, const std::string& Month, const std::string& Hero, double Now);
} // namespace badgeart
} // namespace ui
} // namespace ss
