#pragma once

#include "ShortStack/Common.h"
#include "ShortStack/Game/SiteData.h"

#include <string>
#include <vector>

namespace ss
{
namespace world
{
class World;
struct LeagueBook;
struct LeagueMember;

/**
 * Update 3 G2 (SPEC §3.6 R2, §2.6 F8, §8.3 #9): what the world tells core-siteplay about the rival sites' people, and the
 * League's world-side rules. core-world's header (World.h is frozen after Phase A, so these are free functions over the
 * world's public face, bodies in WorldSim.cpp). Nothing here writes: every answer is a pure function of the world's saved
 * state at that minute, so a reload, a watch or a skip reads the same.
 *
 * The League's clock (§1.1): world days turn at 6 AM and months on the 1st at 6 AM. A result belongs to the 6 AM day its
 * event ended on (LeagueDay(P.End)); the world records a member's result only in its own month's book.
 */

/** The 6 AM world day a world minute falls in (the day of the League, the Flame and the Dares). */
SHORTSTACKCORE_API int LeagueDay(double At);
/** The League month (MonthKeyOf, Year * 12 + Month - 1) of a world minute: the 1st turns at 6 AM. */
SHORTSTACKCORE_API int LeagueMonth(double At);

/**
 * SetLeagueRoster's Div carries the group's pace (World.h's signature is frozen): the division (0 Open .. 4 High Bridge) in
 * the low 3 bits and, when the draw couldn't find enough volume-matched members (§3.6 R2), the pace (1-3 entries a day) in
 * bits 3-4; 0 there is the first-3 rule. LeagueBook::Div keeps the packed value (saved as it is); read it through these.
 */
SHORTSTACKCORE_API int LeaguePack(int Division, int Pace);
SHORTSTACKCORE_API int LeagueDivision(int Packed);
/** How many of a member's first League entries of a day count: the group's pace, else 3. */
SHORTSTACKCORE_API int LeagueFirst(int Packed);

/** A member's month score: the sum of their best 8 daily bests, in points (the book keeps tenths). */
SHORTSTACKCORE_API double LeaguePoints(const LeagueMember& M);
/** The member's form, newest day first, up to 5 days with a League result: 3 final table, 2 cash, 1 blank. */
SHORTSTACKCORE_API std::vector<int> LeagueForm(const LeagueMember& M);
/** The book's member for an NPC (null: not in the group). */
SHORTSTACKCORE_API const LeagueMember* LeagueMemberOf(const LeagueBook& B, int Npc);

/**
 * Someone's monthly volume on a rival site from the world's records (§3.6 R2's draw): their entries there this season
 * (SiteRecord::Events, the calendar year) per 30 days since the season began for them (the later of January 1, the site's
 * launch and the day they joined the world). In the season's first week (every January 1 starts the counts again) the
 * entries the world has already planned for them (its registrations to the end of tomorrow) count too, over those days, so
 * January's draw reads a real rate. 0 for a member with no entries; -1 for someone who isn't a member, or a site not open.
 * S-6' (durable/update3/S-6p.md) measured it against the true monthly counts.
 */
SHORTSTACKCORE_API double MonthlyVolume(const World& W, net::SiteId Site, int Npc, double At);

/** A site's members (its Sites bit, playing) and their MonthlyVolume, by id. */
struct SiteMember
{
	int Npc = -1;
	int Tier = 1;         // their online home stakes (net::Tier)
	double Volume = 0.0;  // MonthlyVolume
	int Events = 0;       // this season's entries on the site
};
SHORTSTACKCORE_API std::vector<SiteMember> SiteMembers(const World& W, net::SiteId Site, double At);

/**
 * The group's pace (§3.6 R2): the median member's entries per playing day, max(1, volume / 30), rounded and held to 1-3
 * (the model's rule, design_model_v11.py). Members with no record count as 1.
 */
SHORTSTACKCORE_API int LeaguePaceOf(const World& W, const std::vector<int>& Npcs, double At);

/** A site team's pros right now (Npc::Sponsor is the team's name, Team RunItTwice's six), by id; fewer while it signs. */
SHORTSTACKCORE_API std::vector<int> SiteTeamNow(const World& W, net::SiteId Site);

/** One of Ignite's Headliners (§2.6 F8), and what put them there. */
struct Headliner
{
	int Npc = -1;
	int Followers = 0;      // Kast followers (an established world ranks by these first)
	double PointsBefore = 0; // this season's Ignite points before this month (World month, midnight)
	double PointsMonth = 0;  // this month's so far
	Chips WonBefore = 0;     // Ignite money won before this season (stays put all year: January has no points before the month)
};
/**
 * Ignite's Headliners for the month At is in: its members (playing) with the most stream followers, then by this season's
 * Ignite points before the month, then by the Ignite money they won before this season, then this month's points so far,
 * then by id; in a fresh world (Start::Blank) the same without the followers ("Season 1: the first headliners", on points
 * alone in its first season, when nobody has Ignite money from before it). Every key but the last
 * two stays put all month, so the six change only when an audience does (or someone stops playing). Up to Count (6); empty
 * before Ignite has launched in this world.
 */
SHORTSTACKCORE_API std::vector<Headliner> Headliners(const World& W, double At, int Count = 6);
} // namespace world
} // namespace ss
