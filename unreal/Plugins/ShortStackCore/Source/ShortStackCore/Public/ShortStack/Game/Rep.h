#pragma once

#include "ShortStack/Common.h"

#include <string>
#include <vector>

namespace ss
{
namespace life
{
struct State;
}
namespace kast
{
struct Channel;
}
namespace world
{
class World;
}

/**
 * The public record (next_segment_spec §1.1): how good and how famous the player looks from outside, in one place.
 * Staking's prices, sponsorship's standing and the badges all read it; none of them reads the others. Built from
 * life::State, the Kast channel, the world (optional: the card room builds it from SaveData alone, world fields
 * empty) and the world minute. Private things (stakes, makeup, the bankroll, debts, tilt) never enter it (N3).
 *
 * Owned by the lead. Phase A, day 1: the contract with stub bodies (Rep.cpp) that nothing calls yet; the record and
 * town::Renown's move onto RenownOf land with the rest of Phase A's seams.
 */
namespace rep
{
struct Public
{
	// Online (world::Tracker for the player; freerolls excluded from the count and the ROI).
	int OnlineEvents = 0;
	double OnlineRoi = 0.0;
	double ProfitAbi = 0.0;
	Chips Abi = 0;
	// Live (Life's totals and the life\tliveday record: not LiveEntries, which keeps only the last sixty).
	int LiveEvents = 0;
	int LiveCashes = 0;
	int LiveBest = 0;
	int Titles = 0;
	int LiveSample = 0;
	double LiveRoi = 0.0;
	Chips LivePrizes365 = 0;
	int Embercrest60 = 0;
	int Cashes60 = 0;
	int Embercrest90 = 0;
	int Cashes90 = 0;
	int BestEmbercrestPlace = 0;
	std::vector<int> EmbercrestDays;
	// Reach (the channel).
	int Followers = 0;
	int Superfans = 0;
	double Avg30 = 0.0;
	bool Partner = false;
	// Honors.
	int Bracelets = 0;
	int Rings = 0;
	int Majors = 0;
	int Poy = 0;
	// Conduct the world can know: the paper (90 days), the site (180), word of mouth (365).
	bool PickedUp90 = false;
	bool Restricted180 = false;
	bool Welched180 = false;
	bool Welched365 = false;
	bool Walked365 = false;
	bool Late90 = false;
	/** 0..1, today's formula (Town.cpp:796-809) computed from the fields above. */
	double Renown = 0.0;
	// Update 2 (§4.7, §5.4; core-econ fills them in Rep.cpp): Rerun, and the rival sites' invitation gates (from History).
	// Restricted180 keeps its meaning: RiverLine's account.
	int VideoSubs = 0, Views30 = 0;
	bool VideoPartner = false;
	int SiteAccounts = 0;
	Chips Abi100 = 0; // the average buy-in of the last hundred tracked events
	int OnlineSeriesTitles = 0;
};

SHORTSTACKCORE_API Public Of(const life::State& L, const kast::Channel& C, const world::World* W, double World);
/** Today's renown formula (Town.cpp:796-809); town::Renown becomes a wrapper over it. */
SHORTSTACKCORE_API double RenownOf(const Public& P);
/** The mood line shown before a counter is sent (§1.9 #25): "Sounds open to it", "That's a stretch", "Don't push it". */
SHORTSTACKCORE_API const char* Mood(double Margin);
} // namespace rep
} // namespace ss
