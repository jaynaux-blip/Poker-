#pragma once

#include "ShortStack/Common.h"
#include "ShortStack/Game/World.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
class Session;
namespace life
{
struct State;
struct RoomLeader;
}

/**
 * Pro badges (next_segment_spec §5): eleven badges in seven families, all earned in public. One glyph per name on
 * every board, seat and stream plate (rules C1 and C9), a badge rail on the player card, progress cards on Career,
 * the Players Club card at the desk, the month's leaders on the room's screens, and a Hall of Fame. Badges come only
 * from public facts and are outputs, never gates: nothing reads them to decide a price, a deal or a hand (§1.1).
 * Holder never carries a backer, makeup or a bankroll (rule C6). Pure and deterministic: no RNG.
 *
 * Bodies: core-badges (Badges.cpp; the session's side in SessionBadges.cpp). Thread use: everything here is pure but
 * Shared() (the game thread's index) and HowEarned (it names an online series through net::Shared()); FieldBadges and
 * FieldLeaders run on the desk's worker too, so they never touch Shared().
 */
namespace badge
{
enum class Kind : uint8_t
{
	HallOfFame,
	Bracelet,
	Ring,
	PlayerOfYear,
	LivePlayerOfYear,
	TeamRiverLine,
	Sponsored,
	CircuitCard,
	VerifiedPro,
	KastPartner,
	OnTheWall,
	Club,
	Count,
};
/** Where a badge shows (bits). */
enum Scope : uint8_t
{
	Network = 1,
	Room = 2,
	Stream = 4,
	Card = 8,
};
enum Frame : uint8_t
{
	FrameNone = 0,
	FrameChampion = 1,
	FrameGold = 2,
};
/**
 * One badge held. Tier: the club (1 Regular, 2 Ember, 3 Crest), the circuit card (1 Card, 2 Gold, 3 Black), a Player of
 * the Year (1 while reigning: the year after the title). Count: bracelets, rings, wall wins, titles, club visits in 90
 * days. Year: the class, the season, the latest title's. Main: a Main Event's bracelet or ring. Color: the enamel, the
 * stone or the brand's colour (0xRRGGBB). Brand: the sponsor's brand id (its display name when the catalog doesn't know it).
 */
struct Badge
{
	Kind K = Kind::Count;
	int Tier = 0;
	int Count = 0;
	int Year = 0;
	bool Main = false;
	uint32_t Color = 0;
	std::string Brand;
};
struct Set
{
	std::vector<Badge> Held; // in Kind order
	/** What the set was built from (the session's cache of the player's own; never packed or compared). */
	uint64_t Stamp = 0;
	SHORTSTACKCORE_API const Badge* Find(Kind K) const;
	SHORTSTACKCORE_API uint32_t Mask() const;
};
constexpr int ClubWindowDays = 90, ClubRegular = 6, ClubEmber = 18, ClubCrest = 36;
constexpr int CardTop[3] = {200, 50, 10}, CardFromWeek = 13;
constexpr int VerifiedEvents = 1000, VerifiedBuyIns = 150, VerifiedQuietDays = 30;
constexpr double VerifiedRoi = 0.15;
constexpr int KastPartnerFollowers = 5000, HallAge = 40, HallYears = 10, HallClass = 2;
constexpr double HallBar = 12.0;
/** The player's monthly review (§5.3): the 1st of the month at 6 AM. */
constexpr double ReviewMinute = 6.0 * 60.0;
/** Everything the rules read about one person: public facts only. */
struct Holder
{
	int Age = 0;
	int YearsOn = 0;
	int DaysSincePlayed = 9999;
	bool Professional = false;
	bool TeamRiverLine = false;
	bool KastPartner = false;
	std::string Sponsor; // the world's: the brand's display name; the player's: the brand id
	uint32_t SponsorColor = 0;
	const world::Tracker* Stats = nullptr;
	const std::vector<world::Award>* Awards = nullptr;
	int Majors = 0;
	std::vector<world::Honor> Honors;
	int CardBest = 0;
	int CardLast = 0;
	int ClubVisitsNow = 0;
	int ClubVisitsAtMonth = 0;
	int WallWins = 0;
	bool VerifiedHeld = false;
	/** The latest win on the wall: its day (-1: unknown) and the event's short name ("" when the record doesn't say). */
	int WallLast = -1;
	std::string WallLastEvent;
	/** The player's last ShiftLink shift (world minutes; -1e9 none) and the days since it (the Verified card's line). */
	double LastShiftAt = -1.0e9;
};
SHORTSTACKCORE_API Holder Of(const world::World& W, int Npc, int Day);
/** The Sponsored and Team RiverLine badges come from sponsor::Badges. */
SHORTSTACKCORE_API Holder OfHero(const Session& S);
SHORTSTACKCORE_API Set Earned(const Holder& H, int Day);
/** The one glyph a name shows in a context (§5.4): the first held that the avatar frame doesn't already say; null: none. */
SHORTSTACKCORE_API const Badge* Top(const Set& S, uint8_t Context, uint8_t Frame);
/** Everything held that a context shows, in its slot order (Card: all of it, the card rail's order). */
SHORTSTACKCORE_API std::vector<Badge> InOrder(const Set& S, uint8_t Context, uint8_t Frame);
SHORTSTACKCORE_API int ClubTier(int VisitsNow, int VisitsAtMonthStart);
/** Distinct days in the Window days up to and including Day (Day - Window < d <= Day): a day counts once. */
SHORTSTACKCORE_API int VisitsIn(const std::vector<int>& Days, int Day, int Window = ClubWindowDays);
/** The player's Embercrest days from the live record (a finished entry, never a refund; one a day). */
SHORTSTACKCORE_API std::vector<int> HeroRoomDays(const life::State& L);
/** The player's club tier on Day, from the save alone (the card room has no session): upgrades at once, drops on the 1st. */
SHORTSTACKCORE_API int HeroClubTier(const life::State& L, int Day);
SHORTSTACKCORE_API bool VerifiedNow(const Holder& H);
SHORTSTACKCORE_API double HallScore(const world::Npc& N, const world::World& W, int Year);
/** The weekly standings' card for a circuit rank (1-based; 0: no points) in a season week (none before CardFromWeek). */
SHORTSTACKCORE_API int CardFor(int Rank, int Week);
/** "VERIFIED PRO", "3× RING", "GOLD CARD '27": the small-caps label (hover, the card rail, the HUD caption). */
SHORTSTACKCORE_API std::string Label(const Badge& B);
/** "Verified Pro", "Three-time ring winner": the tooltip's title. */
SHORTSTACKCORE_API std::string Title(const Badge& B);
/** The hover line (§5.5), with the holder's numbers where it has them (a packed badge from the room has none). */
SHORTSTACKCORE_API std::string HowEarned(const Badge& B, const Holder& H);
/** One honorific for a name ("two-time Embercrest champion", "Team RiverLine's"; "" for none): intros, the caption. */
SHORTSTACKCORE_API std::string Honorific(const Set& S, uint8_t Context);
/** The honorific with the name, as said aloud: "two-time Embercrest champion, Mei", "Team RiverLine's Kenji", or the name. */
SHORTSTACKCORE_API std::string Introduce(const Set& S, uint8_t Context, const std::string& Name);
/** The studying caption's dim line: the first Max badges of a context in words ("Twice on the wall · Ember member"). */
SHORTSTACKCORE_API std::string Caption(const Set& S, uint8_t Context, int Max = 3);
/** The desk's line for a club card (§5.5), said once per upgrade; Kept (not Upgraded) greets a member. Addr: ", grinder". */
SHORTSTACKCORE_API std::string DeskLine(int Tier, bool Upgraded, const std::string& Addr);
/** Kast chat's one reaction to a badge earned live (§5.6): seeded by Salt, the same line for the same badge and salt. */
SHORTSTACKCORE_API std::string ChatReaction(const Badge& B, uint32_t Salt);
/** The TextsSeen key for one earning (C10): "badge:<kind>:<tier>:<count>". */
SHORTSTACKCORE_API std::string SeenKey(const Badge& B);
/** The desk's own key for a club card printed (the card room's SaveData::TextsSeen): "badge-desk:club:<tier>". Apart from
 *  SeenKey, so a toast on the laptop never takes the desk's upgrade line away. */
SHORTSTACKCORE_API std::string DeskKey(int Tier);
struct Progress
{
	Kind K = Kind::Count;
	int Tier = 0;
	double Have = 0.0;
	double Need = 0.0;
	std::string Line;   // "Ember · 12 of 18 events in 90 days"
	std::string Detail; // "ROI +18% · 96 of 150 buy-ins up · 12 of 30 days clear of the day job"
};
SHORTSTACKCORE_API std::vector<Progress> Next(const Session& S, int Max);
/** The field's badges (name -> Pack) for live::DescribeField: the Room scope's, only people who hold any. */
SHORTSTACKCORE_API std::vector<std::pair<std::string, std::string>> FieldBadges(const world::World& W, const std::vector<int>& Field, int Day);
/** The month's room leaders at Day (at most 10, each with its Pack) for live::DescribeField: the leaders the room's
 *  screens show that night (§5). The player's row carries what the world knows of them (no sponsor, no Verified). */
SHORTSTACKCORE_API std::vector<life::RoomLeader> FieldLeaders(const world::World& W, int Day);
/** The player's own row for the night's snapshot (live::DescribeField; the director reads it for the player's intro):
 *  their Room badges from the world and the save alone, packed like FieldBadges' rows. Worker-safe (no Shared()). */
SHORTSTACKCORE_API std::string HeroFieldPack(const world::World& W, const life::State& L, double World);
/** Every NPC's set, built once per world revision and day (each person when first asked for). Game thread only. */
class Index
{
public:
	SHORTSTACKCORE_API const Set& Of(const world::World& W, int Npc, int Day);
	/** How many sets were built since the index last moved on (a bench and the tests read it). */
	int BuiltCount() const { return Builds; }

private:
	int Rev = -1;
	int Built = -9999;
	const world::World* For = nullptr;
	uint32_t Seed = 0;
	size_t People = 0;
	int Builds = 0;
	std::vector<Set> Sets;
	std::vector<uint8_t> Ready;
	std::vector<std::vector<world::Honor>> HonorsBy;
};
SHORTSTACKCORE_API Index& Shared();
/** A set as one line-safe string ([0-9A-Za-z.;%_-]): livebadge, liveleader. Unpack reads what it can and skips the rest. */
SHORTSTACKCORE_API std::string Pack(const Set& S);
SHORTSTACKCORE_API Set Unpack(const std::string& Packed);
/** badge\treview */
SHORTSTACKCORE_API void WriteLines(const life::State& L, std::string& Out);
SHORTSTACKCORE_API bool ReadLine(const std::vector<std::string>& Fields, life::State& L);
/** Every line of badge copy, by key, for the copy lint: hover lines, desk lines, honorifics, chat reactions. */
SHORTSTACKCORE_API std::vector<std::pair<std::string, std::string>> Inventory();
/** Every label, title, caption word and progress line (no end punctuation: the UI's), for the label checks. */
SHORTSTACKCORE_API std::vector<std::pair<std::string, std::string>> Labels();
} // namespace badge
} // namespace ss
