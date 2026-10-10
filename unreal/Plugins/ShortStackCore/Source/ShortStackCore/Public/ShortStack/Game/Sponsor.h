#pragma once

#include "ShortStack/Common.h"

#include <array>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
struct SaveData;
namespace life
{
struct State;
struct LiveEntry;
struct Mark;
}
namespace kast
{
struct Channel;
struct Deal;
}
namespace wardrobe
{
struct Closet;
}
namespace live
{
struct Occurrence;
}
namespace rep
{
struct Public;
}

/**
 * Sponsors (next_segment_spec §3): thirteen fictional brands in five tiers, from a Polaroid on the Lucky Penny's
 * corkboard to Turnstone Poker's chest patch, and the contract book. Asks are counted from things the player already
 * does; 30-day reviews pay stipends in arrears; strikes have a cure; categories are exclusive; a morals clause; counters
 * and renewals. Patches go on in the closet. Offers and letters go to life::State::Inbox (Thread = the contract id,
 * Source "sponsor"), which the Mail app reads. Kast's legacy deals become contracts (§3.8) and Kast works on a view
 * of the book (StreamView / AbsorbStream). The store credit is its own pot and never pays rent (N7); sponsor gear is
 * the brand's during the term, so the pawn refuses it.
 *
 * Forward-declares life::State, LiveEntry and SaveData (Shop.h's pattern) so that Life.h can hold a Book.
 *
 * Bodies: core-sponsor (Sponsor.cpp). Phase A, day 1: stubs that compile and change nothing: an empty catalog, no
 * contract active, nothing covered, no credit, nothing anyone's property, and every action refused.
 */
namespace sponsor
{
enum class Category : int
{
	Local,
	Energy,
	Apparel,
	Gear,
	Privacy,
	Training,
	CardRoom,
	Site,
	Count,
};
enum class Spot : int
{
	Chest,
	Sleeve,
	Cap,
	Overlay,
	Count,
};
enum class Ask : int
{
	LiveHours,
	Reads,
	TeachingStreams,
	LiveEvents,
	EmbercrestEvents,
	Workouts,
	Appearances,
	CommunityStreams,
	Count,
};
enum class Gift : int
{
	StoreCredit,
	Product,
	Service,
	Gear,
	Garment,
	Ticket,
};
struct Perk
{
	Gift Kind = Gift::StoreCredit;
	std::string Id;
	int Count = 1;
	Chips Cents = 0;
	int EveryDays = 30;
};
struct Terms
{
	int Days = 90;
	Chips SigningCents = 0;
	Chips MonthlyCents = 0;
	Chips PerLiveHourCents = 0;
	Chips PerReadCents = 0;
	Chips FinalTableBonusCents = 0;
	Chips TitleBonusCents = 0;
	std::vector<Perk> Perks;
	int CoveredEntries = 0;
	Chips CoveredCapCents = 0;
	bool CoversFares = false;
	int PrizeCutPct = 0;
	std::array<double, static_cast<size_t>(Ask::Count)> Asks{};
	std::vector<Spot> Wants;
	bool Exclusive = false;
	bool ConductClause = true;
	int CureDays = 14;
};
struct Gate
{
	int Followers = 0;
	int AvgViewers = 0;
	bool Partner = false;
	int Tier = 0;
	std::string BlockVenue;
	int BlockVisits = 0;
};
struct Brand
{
	std::string Id;
	std::string Name;
	std::string Rep;
	std::string RepTitle;
	std::string Keeper;
	Category Cat = Category::Local;
	int Tier = 1;
	uint32_t Color = 0;
	uint32_t Ink = 0;
	int Mark = 0;
	Gate Needs;
	Terms Offer;
	std::vector<std::string> Pitch;
	bool Legacy = false;
};
/** The catalog: 13 brands (§3.4; the site is turnstone, not lantern). */
SHORTSTACKCORE_API const std::vector<Brand>& Brands();
SHORTSTACKCORE_API const Brand* FindBrand(const std::string& Id);
/** Npc::Sponsor holds display names. */
SHORTSTACKCORE_API const Brand* FindBrandByName(const std::string& Name);
/** The roads of §3.3, with the conduct limits. */
SHORTSTACKCORE_API int Tier(const rep::Public& P, const std::string& BrandId);

enum class State : int
{
	Offered,
	Countered,
	Active,
	Notice,
	Ended,
};
enum class End : int
{
	None,
	Declined,
	Withdrawn,
	Expired,
	Walked,
	Lapsed,
	ForCause,
	Conduct,
	NotRenewed,
};
struct Tally
{
	double LiveMinutes = 0.0;
	int Reads = 0;
	int Teaching = 0;
	int LiveEvents = 0;
	int Embercrest = 0;
	int Workouts = 0;
	int Appearances = 0;
	int CoveredUsed = 0;
};
struct Contract
{
	std::string Id;
	std::string Brand;
	State St = State::Offered;
	End Why = End::None;
	Terms T;
	double OfferedAt = 0.0;
	double ExpiresAt = 0.0;
	double Since = 0.0;
	double Until = 0.0;
	int Period = 0;
	int Strikes = 0;
	int Counters = 0;
	double CureUntil = 0.0;
	Tally Now;
	Chips PaidCents = 0;
	bool Legacy = false;
	int TierAtSigning = 0;
};
struct Book
{
	std::vector<Contract> Contracts; // ended: the last 24
	std::map<std::string, double> Cooldown;
	std::set<std::string> PaidKeys;
	Chips StoreCreditCents = 0;
	double SteppedTo = 0.0;
	uint32_t LinesRead = 0; // not saved: which sponsor lines the save had (§7.5)
};
SHORTSTACKCORE_API void Step(Book& B, life::State& L, Chips& Bankroll, const kast::Channel& C, const rep::Public& P, double From, double To, const std::string& Seed);
SHORTSTACKCORE_API std::string CanSign(const Book& B, const life::State& L, const std::string& ContractId, double World);
SHORTSTACKCORE_API std::string Sign(Book& B, life::State& L, Chips& Bankroll, const std::string& ContractId, double World);
enum class Lever : int
{
	Money,
	Lighter,
	Shorter,
	OneMoreEntry,
	NoExclusivity,
};
SHORTSTACKCORE_API double Leverage(const Book& B, const rep::Public& P, const std::string& BrandId);
SHORTSTACKCORE_API std::string Counter(Book& B, const rep::Public& P, const std::string& ContractId, Lever L, double World);
SHORTSTACKCORE_API void Decline(Book& B, const std::string& ContractId, double World);
SHORTSTACKCORE_API std::string GiveNotice(Book& B, const std::string& ContractId, double World);
SHORTSTACKCORE_API std::string AskForRaise(Book& B, const rep::Public& P, const std::string& ContractId, double World);
/** "" or what's missing. */
SHORTSTACKCORE_API std::string Fits(const Book& B, const wardrobe::Closet& C, bool AtEmbercrest, double World);
SHORTSTACKCORE_API void NoteWorn(life::LiveEntry& E, const std::string& Hero, const wardrobe::Closet& C, const Book& B, double World);
struct Window
{
	std::string Contract;
	std::string Venue;
	std::string Label;
	double From = 0.0;
	double To = 0.0;
	bool Done = false;
};
SHORTSTACKCORE_API std::vector<Window> Appearances(const Book& B, double From, double To);
SHORTSTACKCORE_API std::string Appear(Book& B, const std::string& Venue, double World);
SHORTSTACKCORE_API const Contract* Covering(const Book& B, const live::Occurrence& O, double World);
/** The cover line. */
SHORTSTACKCORE_API void OnRegister(Book& B, life::State& L, Chips& Bankroll, const life::LiveEntry& E, double World);
/** The cut, the bonuses. */
SHORTSTACKCORE_API void Settled(Book& B, life::State& L, Chips& Bankroll, const life::LiveEntry& E);
SHORTSTACKCORE_API void OnRefund(Book& B, life::State& L, Chips& Bankroll, const life::LiveEntry& E, double World);
/** Kast works on this; nothing it holds is saved. */
SHORTSTACKCORE_API std::vector<kast::Deal> StreamView(const Book& B, double World);
SHORTSTACKCORE_API void AbsorbStream(Book& B, const std::vector<kast::Deal>& Deals);
enum class Breach : int
{
	PickedUp,
	AccountRestricted,
};
SHORTSTACKCORE_API void Conduct(Book& B, life::State& L, Chips& Bankroll, Breach Why, double World);
SHORTSTACKCORE_API bool Active(const Book& B, const std::string& BrandId, double World);
/** The store credit first; returns what the bankroll still owes. */
SHORTSTACKCORE_API Chips SpendCredit(Book& B, Chips Due);
/** The pawn refuses it (§3.6). */
SHORTSTACKCORE_API bool IsProperty(const Book& B, const std::string& GearOrGarmentId, double World);
/** Brand, colour. */
SHORTSTACKCORE_API std::vector<std::pair<std::string, uint32_t>> Badges(const Book& B, double World);
SHORTSTACKCORE_API std::vector<life::Mark> Marks(const Book& B, double From, double To);
SHORTSTACKCORE_API Chips NpcWeekly(const std::string& BrandId);
/** Includes sponsor\tpatch for the closet. */
SHORTSTACKCORE_API void WriteLines(const life::State& L, std::string& Out);
SHORTSTACKCORE_API bool ReadLine(const std::vector<std::string>& Fields, life::State& L);
/** Kast's legacy deals (§3.8). */
SHORTSTACKCORE_API void AfterParse(SaveData& Data);
} // namespace sponsor
} // namespace ss
