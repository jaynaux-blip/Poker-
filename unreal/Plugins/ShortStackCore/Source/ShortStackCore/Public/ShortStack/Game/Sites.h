#pragma once

#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/SiteData.h"

#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
struct SaveData;
struct HistoryEntry;
namespace life
{
struct State;
struct Mark;
}
namespace rep
{
struct Public;
}
namespace world
{
class World;
}

/**
 * The poker sites (update 2 §5.8): RiverLine and its rivals, each a net::Network built from its SiteDef. The routing
 * (Network.cpp: KeyOf, Site, SiteOf, ForEvent, AttachAll, SetHeroAll) is live since A3: a "<key>." id is that site's, and
 * every site has its own network in the registry, empty until core-sites registers the site's SiteDef in Sites() and its
 * schedule in NetworkSites.cpp (G3). RiverLine's schedule, ids, seeds, rake, fields and payouts never change (U9).
 * Data: core-sites (Game/Sites.cpp).
 */
namespace net
{
/** A site's rake: Kind 0 is RiverLine's FeeOf with 0.915 of the rest to the pool; others a flat share or steps by buy-in. */
struct RakeRule
{
	int Kind = 0;
	double Flat = 0;
	std::vector<std::pair<Chips, double>> Steps;
	SHORTSTACKCORE_API Chips Fee(Chips BuyIn) const;
	SHORTSTACKCORE_API double PrizeShare(const EventTemplate& T) const;
};
/** How busy a site is (RiverLine's: the tickers as today). */
struct TrafficRule
{
	double Scale = 1.0, Sunday = 1.38;
	int PeakHour = 21;
	int OnlineBase = 318000, OnlineSwing = 94000;
};
/** A way to move money in or out (the Cashier). */
struct CashMethod
{
	std::string Id, Name;
	int FeePermille = 0;
	Chips MinCents = 0, WithdrawFeeCents = 0;
	double DepositHours = 0;
	bool Withdraws = false;
	double WithdrawHoursMin = 0, WithdrawHoursMax = 0;
	bool Reversible = false;
	bool P1 = false;
};
/** What a site's invitation waits for (§5.4). */
struct InviteGate
{
	int MinEvents = 0;
	int MinRentsPaid = 0;
	Chips MinAbi100 = 0;
	bool SeriesTitle = false, KastPartner = false, BraceletOrRing = false;
	int SponsorTier = 0;
};
/** One site: its brand, its palette (pal:: order, 16 tokens), rake, traffic, fields, cashier, gate and launch. */
struct SiteDef
{
	SiteId Id = SiteId::RiverLine;
	std::string Key, Name, Tagline, Rep, RepTitle, TeamName;
	std::array<uint32_t, 16> Palette{}; // Bg, Bg2, Panel, Panel2, Line, Ink, Muted, Dim, Accent, Accent2, Gold, Red, Orange, Green, Felt, FeltDark
	RakeRule Rake;
	TrafficRule Traffic;
	std::array<double, 5> Softness{};
	std::array<std::string, 5> Population;
	double SecondsPerHand[2] = {36, 42};
	bool FieldFloor = true;
	double GtdOfPool = 0.9, LaunchGtd = 1.0;
	int TeamSize = 0;
	std::vector<CashMethod> Cashier;
	Chips MinWithdraw = 0;
	InviteGate Gate;
	int LaunchDay = 0;
	bool Linked = false; // RiverLine: linked to the bank (no wallet, no fees)
	bool P1 = false;
};
/** Every registered site, RiverLine first (Phase A: only RiverLine). */
SHORTSTACKCORE_API const std::vector<SiteDef>& Sites();
/** A site's definition (one not registered yet: RiverLine's). */
SHORTSTACKCORE_API const SiteDef& Def(SiteId S);
/**
 * A site's network (RiverLine: Shared()). Every site has its own: a registered rival's is built from its SiteDef, one not
 * registered yet is empty (no schedule), so an id never reaches another site's events. The rivals' are built once, all
 * together, the first time one is asked for (AttachAll does it on the game thread, before the desk's worker runs: §5.8's
 * threading contract), and never move.
 */
SHORTSTACKCORE_API Network& Site(SiteId S);
/**
 * The site an id belongs to (an event, template, series or ticket id): the prefix before the first '.', when it is a site's
 * key and comes before any '@' ("runittwice.sunday-100k@1500" is RunItTwice's); anything else, every bare id, RiverLine's.
 */
SHORTSTACKCORE_API SiteId SiteOf(const std::string& Id);
/** Site(SiteOf(Id)): the network an event, template, series or ticket id lives on. */
SHORTSTACKCORE_API Network& ForEvent(const std::string& Id);
/** Network::Attach and SetHero on every site's network (they all show the same world; each picks out its own finishes). */
SHORTSTACKCORE_API void AttachAll(const world::World* W);
SHORTSTACKCORE_API void SetHeroAll(const std::string& Name, const std::vector<HistoryEntry>& History);
/**
 * A site's key, the prefix of its ids: "riverline", "ignite", "runittwice", "halcyon" ("" for a SiteId out of range). Fixed
 * with SiteId, since saved ids carry it; a registered SiteDef's Key must be the same (sites_test checks it).
 */
SHORTSTACKCORE_API const char* KeyOf(SiteId S);
} // namespace net

/** The money rules on the rival sites (§5.5, §5.6, §5.7). Bodies: core-sites (Sites.cpp, SiteMoney.cpp); dormant until G3. */
namespace site
{
SHORTSTACKCORE_API std::string Join(Book& B, net::SiteId S, double World);
/** A line in a rival's own ledger (stake::Pot's rival side writes here, never the bank's). */
SHORTSTACKCORE_API void AddLine(Book& B, net::SiteId S, double At, int Kind, Chips Cents, const std::string& Label);
SHORTSTACKCORE_API std::string Deposit(Book& B, life::State& L, Chips& Bankroll, net::SiteId S, Chips Cents, const std::string& Method, double World);
SHORTSTACKCORE_API std::string Withdraw(Book& B, net::SiteId S, Chips Cents, const std::string& Method, double World);
SHORTSTACKCORE_API std::string Reverse(Book& B, const std::string& TransferId, double World); // P1
/** A rival's wallet (null for RiverLine: the bank). */
SHORTSTACKCORE_API Chips* WalletOf(Book& B, net::SiteId S);
SHORTSTACKCORE_API void OnBuyIn(Book& B, net::SiteId S, Chips BuyIn, Chips Fee, const std::string& Label, double World);
SHORTSTACKCORE_API void OnPrize(Book& B, net::SiteId S, Chips Cents, const std::string& Label, double World, const Season& Add);
/** The calendar's step (CheckCalendar, after VideoStep): launches, invitations, arrivals, VIP months, Rewards, expiry. */
SHORTSTACKCORE_API void Step(Book& B, life::State& L, Chips& Bankroll, const rep::Public& P, int SponsorTier, double From, double To,
	std::vector<std::pair<std::string, std::string>>& Mail);
/** The rent card's warning when the bank alone can't cover the rent but the bank and the sites could. */
SHORTSTACKCORE_API bool RentWarning(const Book& B, Chips Bankroll, Chips RentDue, double Deadline, double World, std::string& Line);
SHORTSTACKCORE_API std::vector<life::Mark> Marks(const Book& B, double From, double To);
SHORTSTACKCORE_API std::vector<std::string> Inventory();
SHORTSTACKCORE_API void WriteLines(const life::State& L, std::string& Out);
SHORTSTACKCORE_API bool ReadLine(const std::vector<std::string>& P, life::State& L);
SHORTSTACKCORE_API void AfterParse(SaveData& D);
} // namespace site
} // namespace ss
