#pragma once

#include "ShortStack/Common.h"

#include <set>
#include <string>
#include <vector>

/**
 * The rival sites' data the player's life keeps (update 2 §5.5, §6.3): split from Sites.h (the registry) so Life.h can
 * embed it without pulling in the networks. Saved as site\t... lines once Update2LinesRead's U2Sites bit is set. Frozen
 * with Life.h after Phase A; the rules are core-sites' (Game/Sites.cpp, SiteMoney.cpp). Dormant until G3.
 */
namespace ss
{
namespace net
{
/** Saved: append-only. RiverLine stays the bank's site (U8): it has no wallet. */
enum class SiteId : int
{
	RiverLine,
	Flopjoy,
	Turnstone,
	Halcyon,
	Count,
};
} // namespace net

namespace site
{
/**
 * The kinds of the sites' letters (Mail's Source "site", and RiverLine Player Relations'): an invitation and an offer
 * want an answer, so Mail's trim keeps them while unread (life::WantsAnswer); everything routine is 10 and up.
 */
enum LetterKind : int
{
	LetterInvite = 1,
	LetterOffer = 2,
	LetterRoutine = 10,
};
/** One line of a site's own ledger (the last 40 a site). */
struct Line
{
	double At = 0;
	int Kind = 0;
	Chips Cents = 0;
	std::string Label;
};
/** The player's points season on a site. */
struct Season
{
	double Points = 0;
	Chips Won = 0;
	int Wins = 0, FinalTables = 0, Events = 0;
	int Year = 0;
};
/** A joined rival: its wallet, rake and VIP month, the welcome bonus, what went in and out, the player's record there. */
struct Account
{
	net::SiteId Site = net::SiteId::RiverLine;
	double JoinedAt = 0;
	Chips Cents = 0;
	std::string MonthKey;
	Chips RakeMonth = 0, RakeLife = 0;
	int Vip = 0;
	double DealUntil = 0; // P1
	Chips BonusPending = 0, BonusReleased = 0;
	double BonusExpires = 0;
	Chips Deposited = 0, Withdrawn = 0;
	Season Hero;
	Chips LifetimeWon = 0;
	int LifetimeEvents = 0;
	std::vector<Line> Lines; // the last 40
};
/** Money moving between the bank and a site. */
struct Transfer
{
	std::string Id;
	net::SiteId Site = net::SiteId::RiverLine;
	bool Out = false;
	std::string Method;
	Chips Cents = 0, FeeCents = 0;
	double RequestedAt = 0, ArrivesAt = 0;
	int State = 0; // 0 pending, 1 arrived, 2 reversed (P1)
};
struct Invitation
{
	net::SiteId Site = net::SiteId::RiverLine;
	double At = 0;
	int State = 0; // 0 sent, 1 accepted, 2 declined, 3 resent
};
/** RiverLine Rewards: the month's rake and tier, and the retention offer's boost. */
struct Rewards
{
	std::string MonthKey;
	Chips Rake = 0;
	int Tier = 0;
	double BoostUntil = 0;
};
/** Everything the sites keep on the career (life::State::Sites). */
struct Book
{
	std::vector<Account> Accounts;
	std::vector<Transfer> Transfers;
	std::vector<Invitation> Invites;
	Rewards RiverLine;
	std::set<std::string> Done;
};
} // namespace site
} // namespace ss
