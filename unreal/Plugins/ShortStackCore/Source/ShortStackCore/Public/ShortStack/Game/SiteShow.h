#pragma once

#include "ShortStack/Common.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/SiteData.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace ss
{
class Hand;
struct HistoryEntry;
namespace world
{
class World;
}

/**
 * Update 3 (SPEC §2.4, §3.4-§3.6, §7.3): the revamped sites' presentation helpers, pure functions with no save. Ignite's
 * mood by the world hour, the Board's projected overlay, the Club card's live accrual, the Second Board after an all-in,
 * the Desk's KPIs, Sunday Live's story rows and the welcome bonus's needle. Declared by the lead in Phase A; bodies by
 * core-sites-look (SiteShow.cpp) at G1.
 */
namespace site
{
namespace show
{
/** Ignite's time of day: Afterburner Hours (00:00-06:00), the wake-up, daytime, prime time. */
enum class Mood : uint8_t
{
	Afterburner,
	WakeUp,
	Daytime,
	PrimeTime,
};
SHORTSTACKCORE_API Mood IgniteMood(double World);
/** The Board's OVERLAY column: registered so far x the template's median late-reg multiplier (Actual once it closed). */
SHORTSTACKCORE_API Chips ProjectedOverlay(const net::Network& N, const net::EventInstance& E, int RegisteredSoFar, double World, bool& Actual);
/** The Club card's live accrual: the tier, the next, the month's rake, what it has earned and what's left to the next. */
struct Accrual
{
	int Tier = 0, Next = 0;
	Chips Rake = 0, Accrued = 0, ToNext = 0;
};
SHORTSTACKCORE_API Accrual ClubAccrual(const site::Account& A);
/** What the second run of the board would have been (for show): 0 lost both, 1 split, 2 won both. */
struct SecondBoard
{
	std::array<int, 5> Cards{};
	int Verdict = 0;
	bool FromStub = false;
};
/** The Second Board from the hand's own deck stub (Hand::DeckForShow), else a keyed draw; draws nothing from H's Rng. */
SHORTSTACKCORE_API bool SecondRun(const Hand& H, int HeroSeat, const std::string& HandId, SecondBoard& Out);
/** The Desk's numbers over the last Days days. */
struct DeskKpis
{
	int Events = 0;
	double Roi = 0, Itm = 0, Ft = 0;
	Chips Abi = 0, Net = 0, Rake = 0, Club = 0;
};
SHORTSTACKCORE_API DeskKpis Desk(const std::vector<HistoryEntry>& H, const site::Account& A, int Days, double World);
/** One line of Sunday Live's story (the world's own results, as they happen). */
struct LiveRow
{
	double At = 0;
	std::string Text;
};
SHORTSTACKCORE_API std::vector<LiveRow> SundayLive(const net::Network& N, const world::World* W, const net::EventInstance& E, double World);
/** Ignite's welcome bonus as the REWARDS page draws it: pending, released, the next release and the needle. */
struct WelcomeView
{
	Chips Pending = 0, Released = 0, NextIn = 0;
	double NeedleT = 0;
	double Ends = 0;
};
SHORTSTACKCORE_API WelcomeView Welcome(const site::Account& A, double World);
} // namespace show
} // namespace site
} // namespace ss
