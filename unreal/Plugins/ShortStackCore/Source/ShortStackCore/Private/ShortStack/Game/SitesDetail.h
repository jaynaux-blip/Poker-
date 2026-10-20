#pragma once

// The rival sites' internals shared between core-sites' files (update 2 §5): Sites.cpp, SiteMoney.cpp, SessionSites.cpp,
// NetworkSites.cpp, Network.cpp and Rep.cpp's G3 fields. Private to the module: the public contract is Sites.h and SiteData.h
// (frozen). The laptop's pages (ui-laptop) may include it from UI/ ("../Game/SitesDetail.h") for the tier tables, the mark
// kinds and the player's numbers by site; nothing here is saved, so it can change with the code.

#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/SiteData.h"
#include "ShortStack/Game/Sites.h"

#include <string>
#include <utility>
#include <vector>

namespace ss
{
struct HistoryEntry;
namespace life
{
struct State;
}
namespace rep
{
struct Public;
/**
 * The record's invitation fields from the player's History (it holds exactly the last hundred results): Abi100, the average
 * buy-in of the paid events among them (freerolls and unknown buy-ins left out), and OnlineSeriesTitles, the online series
 * events among them the player won (any site's: a series is a template's, looked up on the event's own site). Rep.cpp
 * (core-econ's G3 fields, written by core-sites). rep::Of can't see History, so Session::Record() calls this after it, and
 * SiteStep does before the gates.
 */
void FromHistory(Public& P, const std::vector<HistoryEntry>& History);
} // namespace rep
namespace world
{
class World;
}

namespace site
{
/** The marks the sites add to the wall calendar and the second screen (life::Mark::Kind; stake uses 30s, sponsor 20s, video 40s). */
enum SiteMarkKind : int
{
	MarkTransfer = 50, // "Ignite withdrawal $197.00 arrives"
	MarkRewards = 51,  // "RiverLine Rewards" on the 1st
	MarkBonus = 52,    // "Ignite welcome bonus ends"
	MarkSeries = 53,   // a joined rival's series starts
};

/** A promotion tier: its name, the month's rake it starts at, and what it pays back (per mille of that rake). */
struct TierInfo
{
	const char* Name;
	Chips From;
	int Permille;
};
/** RiverLine Rewards (§5.7): Bronze under $40 (nothing back), Silver $40 (8%), Gold $200 (14%), Platinum $800 (20%), Diamond $3,000 (27%). */
const std::vector<TierInfo>& RewardsTiers();
/** RunItTwice's Second Run Club (§5.6): from $50 of a month's rake 15%, $250 20%, $1,000 25%, $4,000 30% (tier 0: none). */
const std::vector<TierInfo>& SecondRunTiers();
/** The tier a month's rake reaches in a table (0 = the first). */
int TierFor(const std::vector<TierInfo>& Tiers, Chips Rake);
/** "2026-11": the calendar month of a world minute (the site book's month keys). */
std::string MonthKeyAt(double World);
/** "November": the month's name for letters and lines. */
std::string MonthName(const std::string& Key);
/** The world minute the next calendar month starts after World (its 1st, midnight). */
double NextMonthStart(double World);

/** The player's account on a site (null: none, or RiverLine, which has no wallet). */
Account* AccountOf(Book& B, net::SiteId S);
const Account* AccountOf(const Book& B, net::SiteId S);
/** A registered rival: one the player could hold an account with (P0: Ignite and RunItTwice). */
bool IsRival(net::SiteId S);
/** Where Mail's letters from a site come from: "Bree Okafor, Ignite Partnerships", "RiverLine Player Relations". */
std::string FromLine(net::SiteId S);
/** A site's deposit or withdrawal method by id ("" = its first one that way), or null. */
const net::CashMethod* MethodFor(net::SiteId S, const std::string& Id, bool Out);

/**
 * The invitation gate (§5.4) against the public record: the site launched by World, Act I over (RentsPaid), and the site's
 * own door (Ignite: 150 paid online events; RunItTwice: 300 with an $11 average over the last hundred, or an online series
 * title, or Kast Partner). Why (when met): 0 events, 1 a series title, 2 Kast Partner.
 */
bool GateMet(net::SiteId S, const rep::Public& P, int RentsPaid, double World, int* Why = nullptr);
/** Sends a site's invitation now (State 0, or 3 for the second letter 30 days after a decline). Forced by ss.Site.Invite. */
void Invite(Book& B, life::State& L, net::SiteId S, double World, int Why, bool Second);
/** Mail from a site (Source "site", the thread its key): LetterInvite, or LetterRoutine (at most one a week a company, §1.7). */
void PostLetter(life::State& L, net::SiteId S, double World, int Kind, const std::string& Subject, const std::string& Body);
/** A routine letter from that company went out less than a week before World. */
bool WroteThisWeek(const life::State& L, net::SiteId S, double World);
/** Fills a letter's {Vars}. */
std::string FillCopy(const std::string& Template, const std::vector<std::pair<std::string, std::string>>& Vars);

/**
 * The player's numbers for a site's boards (§5.8, bug 7). RiverLine: StatsFrom's (its own events only), and with a world
 * and a full History (the last 100 can't be the whole record) the lifetime numbers by subtraction: Earnings = the world's
 * online prizes less every rival account's LifetimeWon, Season = the world's HeroPoints, Tournaments the same way, each never below
 * StatsFrom's (a career older than the tracker, or a save without a world, C3). A rival:
 * its account's record (Season, Wins, Final tables this year, LifetimeWon as Earnings), its running series' points and this
 * month's points from History, and SiteBoard = this month's points (Ignite's Month board).
 */
net::HeroStats HeroOn(net::SiteId S, const std::string& Name, const std::vector<HistoryEntry>& History, const Book& B, const world::World* W, double Now);

/** The sites' copy by key (letters, news, errors, the rent line; "" for a key it doesn't have). */
const char* CopyText(const std::string& Key);
/**
 * Every sentence the sites can say (letters, news bodies, errors, the rent line, texts), for the copy lint: site::Inventory()
 * is these. Titles, subjects and ledger labels are in CopyLabels(): no closing stop, held to the lint's other rules.
 */
std::vector<std::string> Copy();
std::vector<std::string> CopyLabels();
} // namespace site

namespace net
{
namespace netsites_detail
{
/** A rival's weekday traffic (RiverLine's day factors, its own Sunday), Monday 0. */
double NetSitesDayFactor(const SiteDef& D, int Weekday);
/** The world minute a site's lobby opened on this network's attached world (its SiteDef's launch day when none says). */
double NetSitesLaunch(const Network& N);
/** A launch-week one-off's template id: the daily's own id and this suffix and the day of the week (1 to 7). */
constexpr const char* NetSitesLaunchTag = "-launch";
/** The daily a launch-week one-off stands in for ("" when Id isn't one). */
std::string NetSitesLaunchBase(const std::string& Id);
} // namespace netsites_detail
} // namespace net
} // namespace ss
