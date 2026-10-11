#pragma once

#include "ShortStack/Game/Badges.h"
#include "ShortStack/UI/Canvas.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ss
{
struct SaveData;
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
 * Seven silhouettes, one per family: the medallion (Hall of Fame, Player of the Year), the object (a bracelet, a ring),
 * the stitched five-sided patch (Team RiverLine, a brand), the card (the circuit's), the hexagon (Verified Pro), Kast's
 * check, and the Embercrest's crest (the wall's flame, the club's card). Every glyph reads at 12 units: small sizes
 * drop detail rather than shrink it (a count medal, a crown, the stitching and the year come in as there's room).
 *
 * Bodies: core-badges (BadgeArt.cpp).
 */
namespace badgeart
{
/** The badges' colours (§5.5), 0xRRGGBB. */
namespace tok
{
constexpr uint32_t Onyx = 0x141416;
constexpr uint32_t Gold = 0xf2c14e;       // the Hall's rim, Team RiverLine, the Gold card
constexpr uint32_t Midnight = 0x0a1020;   // the Player of the Year's field
constexpr uint32_t Teal = 0x27d3c3;       // online Player of the Year, Verified Pro
constexpr uint32_t LiveGold = 0xf6b04a;   // live Player of the Year, the Crest
constexpr uint32_t CardSilver = 0xc9d3e0; // the circuit's Card
constexpr uint32_t Violet = 0x9b5cff;     // Kast's check
constexpr uint32_t EmberHot = 0xf2832e;   // the wall
constexpr uint32_t Flame = 0xffd78a;
constexpr uint32_t Ash = 0xebe7df;        // a Regular's outline
constexpr uint32_t Copper = 0xd9531e;     // Ember
constexpr uint32_t Navy = 0x121c33;       // the Crest's ground
constexpr uint32_t Ice = 0x8d9ab4;
constexpr uint32_t Ink = 0x0a111c;        // marks on a light ground
} // namespace tok

SHORTSTACKCORE_API void Draw(Canvas& C, const badge::Badge& B, float Cx, float Cy, float Size, double Time, bool Glint = false);
/** Returns the width drawn. Labels: the first three carry their label (the card's rail); past Max, "+N". */
SHORTSTACKCORE_API float Rail(Canvas& C, const badge::Set& S, uint8_t Context, float X, float Cy, float Size, int Max, bool Labels);
/** Draws every variant into cells of Cell units, eight to a row (the small sizes' art, drawn large); returns how many. */
SHORTSTACKCORE_API int Atlas(Canvas& C, float Cell);
/** The badge's cell in the atlas (-1: none). Counts past the atlas's (5+ trophies, 9+ wins) share the last cell. */
SHORTSTACKCORE_API int AtlasCell(const badge::Badge& B);
/** The atlas's variants, in cell order (ss.Badge.Atlas lists them). */
SHORTSTACKCORE_API const std::vector<badge::Badge>& AtlasBadges();
struct ClubCardInfo
{
	std::string Name;
	std::string ScreenName;
	std::string Country;
	int Tier = 0;     // 0 a new member, 1 Regular, 2 Ember, 3 Crest
	int Visits = 0;   // events in the last 90 days
	int Wins = 0;     // on the wall
	int SinceDay = 0; // member since (world day)
	bool New = false; // just printed: the foil catches the light once
};
/** The Players Club card, any size at 1.586:1 (the desk's, Career's mini card at 220 x 138, the creator's review). */
SHORTSTACKCORE_API void ClubCard(Canvas& C, const Rect& R, const ClubCardInfo& Info, double Now);
/**
 * The player's card from the save alone (the card room has no session): the tier and the nights in the last 90 days from
 * the live record, the wall, the name. New: no card at this tier or above printed yet (badge::DeskKey in TextsSeen): the desk
 * prints it with the upgrade line (badge::DeskLine(Tier, true, ...)), then adds the key; otherwise it greets a member.
 */
SHORTSTACKCORE_API ClubCardInfo ClubCardOf(const SaveData& D, double World);
/** THE EMBERCREST's month: ten rows, one glyph a line (C8), the player's row in ember. Month: "NOVEMBER" or "THIS MONTH". */
SHORTSTACKCORE_API void RoomLeaders(Canvas& C, const Rect& R, const std::vector<life::RoomLeader>& Rows, const std::string& Month, const std::string& Hero, double Now);
} // namespace badgeart
} // namespace ui
} // namespace ss
