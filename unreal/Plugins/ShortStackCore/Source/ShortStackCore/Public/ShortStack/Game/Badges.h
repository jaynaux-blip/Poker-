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
 * Bodies: core-badges (Badges.cpp; the session's side in SessionBadges.cpp). Phase A, day 1: stubs that compile and
 * change nothing: nobody holds a badge, Pack is empty, and the save hooks read and write nothing.
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
	std::vector<Badge> Held;
	SHORTSTACKCORE_API const Badge* Find(Kind K) const;
	SHORTSTACKCORE_API uint32_t Mask() const;
};
constexpr int ClubWindowDays = 90, ClubRegular = 6, ClubEmber = 18, ClubCrest = 36;
constexpr int CardTop[3] = {200, 50, 10}, CardFromWeek = 13;
constexpr int VerifiedEvents = 1000, VerifiedBuyIns = 150, VerifiedQuietDays = 30;
constexpr double VerifiedRoi = 0.15;
constexpr int KastPartnerFollowers = 5000, HallAge = 40, HallYears = 10, HallClass = 2;
constexpr double HallBar = 12.0;
/** Everything the rules read about one person: public facts only. */
struct Holder
{
	int Age = 0;
	int YearsOn = 0;
	int DaysSincePlayed = 9999;
	bool Professional = false;
	bool TeamRiverLine = false;
	bool KastPartner = false;
	std::string Sponsor;
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
};
SHORTSTACKCORE_API Holder Of(const world::World& W, int Npc, int Day);
/** The Sponsored and Team RiverLine badges come from sponsor::Badges. */
SHORTSTACKCORE_API Holder OfHero(const Session& S);
SHORTSTACKCORE_API Set Earned(const Holder& H, int Day);
SHORTSTACKCORE_API const Badge* Top(const Set& S, uint8_t Context, uint8_t Frame);
SHORTSTACKCORE_API int ClubTier(int VisitsNow, int VisitsAtMonthStart);
SHORTSTACKCORE_API int VisitsIn(const std::vector<int>& Days, int Day, int Window = ClubWindowDays);
SHORTSTACKCORE_API bool VerifiedNow(const Holder& H);
SHORTSTACKCORE_API double HallScore(const world::Npc& N, const world::World& W, int Year);
SHORTSTACKCORE_API std::string Label(const Badge& B);
SHORTSTACKCORE_API std::string Title(const Badge& B);
SHORTSTACKCORE_API std::string HowEarned(const Badge& B, const Holder& H);
SHORTSTACKCORE_API std::string Honorific(const Set& S, uint8_t Context);
SHORTSTACKCORE_API std::string DeskLine(int Tier, bool Upgraded, const std::string& Addr);
struct Progress
{
	Kind K = Kind::Count;
	int Tier = 0;
	double Have = 0.0;
	double Need = 0.0;
	std::string Line;
};
SHORTSTACKCORE_API std::vector<Progress> Next(const Session& S, int Max);
/** The field's badges (name -> Pack) for live::DescribeField. */
SHORTSTACKCORE_API std::vector<std::pair<std::string, std::string>> FieldBadges(const world::World& W, const std::vector<int>& Field, int Day);
/** The month's room leaders at Day (at most 10, each with its Pack) for live::DescribeField: the leaders the room's
 *  screens show that night (§5). Declared beside FieldBadges in Phase A, days 2-3, so DescribeField has the call. */
SHORTSTACKCORE_API std::vector<life::RoomLeader> FieldLeaders(const world::World& W, int Day);
/** Every NPC's set, built once per world revision and day. */
class Index
{
public:
	SHORTSTACKCORE_API const Set& Of(const world::World& W, int Npc, int Day);

private:
	int Rev = -1;
	int Built = -9999;
	std::vector<Set> Sets;
};
SHORTSTACKCORE_API Index& Shared();
SHORTSTACKCORE_API std::string Pack(const Set& S);
SHORTSTACKCORE_API Set Unpack(const std::string& Packed);
/** badge\treview */
SHORTSTACKCORE_API void WriteLines(const life::State& L, std::string& Out);
SHORTSTACKCORE_API bool ReadLine(const std::vector<std::string>& Fields, life::State& L);
} // namespace badge
} // namespace ss
