#pragma once

#include "ShortStack/Common.h"
#include "ShortStack/Game/Gear.h"

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
struct Funding;
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
 * Everything is a pure function of the book, the life, the channel and the world minute. The only seeded choices are a
 * letter's wording and its arrival minute (Seed + ":sponsor:" + brand + ":" + day); every answer, review and payment is
 * a rule. Money moves once: each stipend, cover, cut, bonus and grant is keyed in Book::PaidKeys, so a Step run twice
 * over one span (or after a save in the middle of it) pays once. Owned by core-sponsor.
 */
namespace sponsor
{
struct Book;

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
/** The patch spots on a jacket, tee or hat (the closet's Patches row): Chest, Sleeve and Cap. */
constexpr int PatchSpots = 3;
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
	std::string Id; // the store item (Product), the venue (Service), the gear or garment, the event template (Ticket)
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
	/** The Embercrest events a cover takes (EventTemplate keys: "noon", "nightly"); empty: any up to the cap. */
	std::vector<std::string> CoveredEvents;
	int PrizeCutPct = 0;
	/** Rangewise: a bonus per teaching stream, at most TeachingCap a period. */
	Chips PerTeachingCents = 0;
	int TeachingCap = 0;
	std::array<double, static_cast<size_t>(Ask::Count)> Asks{};
	std::vector<Spot> Wants;
	bool Exclusive = false;
	bool ConductClause = true;
	int CureDays = 14;
	// Update 2 (§5.7; core-deals). Written as key=value only when set (never a new Ask: ask= is positional).
	std::string Only;       // only=<site>: plays, streams and posts only on that site
	std::string OnlyStream; // onlystream=<site>: streams only on that site (P1, Flopjoy Squad)
	std::string OnSite;     // onsite=<site>: live hours count only with that site's table in front (P1)
	Chips VideoIntCents = 0; // vi=: a video integration's fee (P1)
	int VideoInts = 0;       // via=: integrations a period (P1)
};
struct Gate
{
	int Followers = 0;
	int AvgViewers = 0;
	bool Partner = false;
	int Tier = 0;
	std::string BlockVenue;
	int BlockVisits = 0;
	/** Offers come at all (Turnstone's tier needs venues that aren't playable yet: only ss.Sponsor.Offer brings it). */
	bool Open = true;
};
struct Brand
{
	std::string Id;
	std::string Name;     // "Lucky Penny #212"
	std::string Short;    // "Lucky Penny": funding lines, the patch row, the toast
	std::string Kast;     // the name Kast's overlay, chat and reads use ("RiverLine"); "" for a brand that never reads on stream
	std::string Product;  // what the stream reads about
	std::string ReadLine; // the streamer's read
	std::string Rep;
	std::string RepTitle;
	std::string Keeper; // town::Person id of the one who makes the offer in person ("ray"); "" for a brand that mails
	std::string Venue;  // where the keeper's shop is ("store"), or the Embercrest's desk ("embercrest")
	Category Cat = Category::Local;
	int Tier = 1;
	uint32_t Color = 0;
	uint32_t Ink = 0;
	int Mark = 0;
	Gate Needs;
	Terms Offer;
	std::vector<std::string> Pitch; // the offer letter's wordings (one is picked by the seed)
	bool Legacy = false;            // one of Kast's five (its id is in old saves)
	int Site = -1;                  // update 2: a Site-category brand's net::SiteId ("riverline", "turnstone", "flopjoy"); -1 for the rest
};
/** The catalog: 13 brands (§3.4; the site is turnstone, not lantern). */
SHORTSTACKCORE_API const std::vector<Brand>& Brands();
SHORTSTACKCORE_API const Brand* FindBrand(const std::string& Id);
/** Npc::Sponsor holds display names: the brand's Name, its Short or its Kast name ("RiverLine" and "Team RiverLine"). */
SHORTSTACKCORE_API const Brand* FindBrandByName(const std::string& Name);
/** Energy, Apparel, Training, Card room and Poker site: one brand at a time. */
SHORTSTACKCORE_API bool ExclusiveCategory(Category C);
/** "energy-drink", "apparel", ... as a letter says it. */
SHORTSTACKCORE_API const char* CategoryWord(Category C);
/** "chest", "sleeve", "cap", "overlay". */
SHORTSTACKCORE_API const char* SpotWord(Spot S);

/**
 * Standing (§3.3), 0 to 5, from the public record, the best of its roads: the live road (the Embercrest's record), the
 * stream road (the channel) and, from update 2 (SPEC §4.7), the creator road (Rerun's subscribers, its views in the
 * last 30 days and Partner); tiers 4 and 5 by the live road open when their venues are playable, so only the stream
 * and Rerun reach them now.
 * The conduct limits: a pickup in the last 90 days holds every brand to tier 1, and a restriction in the last 180 keeps
 * poker sites and training sites at 0. Never saved; computed every time. (Dee's road and the block road need the life:
 * TierOf.)
 */
SHORTSTACKCORE_API int Tier(const rep::Public& P, const std::string& BrandId);
/** Tier with Dee's road (her game up $200 lifetime) and, for a local brand, the block road at its own shop. */
SHORTSTACKCORE_API int TierOf(const rep::Public& P, const Book& B, const life::State& L, const std::string& BrandId, double World);
/** The words for a standing ("A face in the room"): Career shows these, never the thresholds. */
SHORTSTACKCORE_API const char* StandingWords(int Tier);
/** Distinct days with a paid visit to Venue in the 60 days before World (the block road). */
SHORTSTACKCORE_API int VisitDays(const Book& B, const std::string& Venue, double World);

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
	double LiveMinutes = 0;
	int Reads = 0;
	int Teaching = 0;
	int LiveEvents = 0;
	int Embercrest = 0;
	int Workouts = 0;
	int Appearances = 0;
	int CoveredUsed = 0;
	int Community = 0;
};
/** A counter or a raise waiting on its answer (Contract::Pending). */
constexpr int PendingRaise = 5;
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
	int Period = 0; // reviews done
	int Strikes = 0;
	int Counters = 0;
	double CureUntil = 0.0; // a strike's cure runs to here (0: none open)
	Tally Now;              // this period's counted asks (reads, appearances, covered entries; the rest is measured)
	Chips PaidCents = 0;    // everything paid under it (stipends, Kast's hourly and reads, bonuses; less a cut)
	bool Legacy = false;
	int TierAtSigning = 0;
	// ---- kept by the book
	double ReadAt = -1.0e9;   // the last stream read (Kast's one a stream)
	int WorkoutBase = 0;      // Fit.Workouts when the period began
	Chips HeldCents = 0;      // half a stipend, held for the cure
	double CureFrom = 0.0;    // the review that struck
	Tally Short;              // what the cure has to make up
	Tally Cure;               // made up so far: reads and appearances (the rest is measured from CureFrom)
	int CureWorkoutBase = 0;
	bool CompsHalved = false; // a strike holds half of this period's comps
	double ReplyAt = 0.0;     // a counter's or a raise's answer arrives (next business morning)
	int Pending = -1;         // the lever waiting on that answer (Lever, or PendingRaise)
	bool Raised = false;      // the raise has been asked for this term
	std::string Renews;       // the contract this one renews ("" for a first term)
	double MetSum = 0.0;      // the share of asks met, summed over the term's reviews
	std::vector<std::string> Property; // gear and garments the brand sent ("gear:<id>", "garment:<id>")
	bool Kept = false;        // the property is the player's now (a clean end)
};
/** Mail letter kinds (life::Letter::Kind) the book posts. */
enum LetterKind : int
{
	LetterOffer = 1,
	LetterWelcome = 2,
	LetterReview = 3,
	LetterStrike = 4,
	LetterCured = 5,
	LetterEnded = 6,
	LetterNotice = 7,
	LetterReply = 8,
	LetterRenewal = 9,
	LetterConduct = 10,
	LetterHeard = 11,
};
/** life::Mark kinds the book's dates use (the wall calendar, the second screen). */
enum MarkKind : int
{
	MarkReview = 20,
	MarkAppearance = 21,
	MarkRenewal = 22,
	MarkCure = 23,
	MarkOfferEnds = 24,
};
// Letters go to life::State::Inbox (Thread = the contract id, Source "sponsor").
struct Book
{
	std::vector<Contract> Contracts; // ended: the last 24
	std::map<std::string, double> Cooldown; // brand id, "cat:<Category>" or "*" -> until
	std::set<std::string> PaidKeys;
	Chips StoreCreditCents = 0;
	double SteppedTo = 0.0;
	/** The block road (§3.3): venue -> the days with a paid visit there, the last 60 (twelve gum packs on one day are one). */
	std::map<std::string, std::vector<int>> Days;
	/** When strikes were given (leverage counts the last 180 days). */
	std::vector<double> Strikes;
	/** The Lucky Penny let a pickup slide (once). */
	bool PennyPass = false;
	uint32_t LinesRead = 0; // not saved: which sponsor lines the save had (§7.5)
	/** Not saved: the next time Step has anything to do (a look, a review, a reply, a perk); the session skips the step before
	 *  it. Every change made outside Step resets it to 0. */
	double NextAt = 0.0;
	/** Not saved: a standing to test with (ss.Sponsor.Standing): TierOf never reads lower. */
	int DebugTier = 0;
};
/** Book::LinesRead bits. */
enum BookLine : uint32_t
{
	LineBook = 1u << 0,
	LineDeal = 1u << 1,
	LinePatch = 1u << 2,
};

/** What Step reads beyond the book (all optional). */
struct StepContext
{
	const gear::Owned* Gear = nullptr; // Halo's gate (a webcam owned)
	double StreamStart = -1.0;         // the stream in progress began (its minutes count); -1: off air
	std::vector<std::pair<std::string, std::string>>* Texts = nullptr; // phone texts (from, body) for the session to send
};
/**
 * The calendar's step (Session::SponsorStep), From..To in world minutes, in time order: the brands' weekly looks (an
 * offer when the gate is met), offers lapsing after seven days, counters answered the next business morning, the
 * weekly perks (store credit, product at the door), each period's review (asks measured, the stipend paid in arrears,
 * a strike and its cure, notice taking effect, the term's end), renewals fourteen days out, cures lapsing. Idempotent:
 * Book::SteppedTo and PaidKeys make a span stepped twice pay once.
 */
SHORTSTACKCORE_API void Step(Book& B, life::State& L, Chips& Bankroll, const kast::Channel& C, const rep::Public& P, double From, double To, const std::string& Seed,
	const StepContext& Ctx = StepContext());
/** Puts Brand's offer on the table now (the weekly look's own, or ss.Sponsor.Offer): its contract id, "" when it can't. */
SHORTSTACKCORE_API std::string MakeOffer(Book& B, life::State& L, const rep::Public& P, const std::string& BrandId, double World, const std::string& Seed, bool Force);
/** Why Brand wouldn't offer now ("" when it would): the gate, the conduct limits, a cooldown, a deal already running. */
SHORTSTACKCORE_API std::string Eligible(const Book& B, const life::State& L, const rep::Public& P, const std::string& BrandId, double World, const gear::Owned* Gear = nullptr);
/** A local brand's offer made in person (the keeper's talk card): the same terms, now in writing in Mail (once). */
SHORTSTACKCORE_API void HeardOffer(Book& B, life::State& L, const std::string& ContractId, double World);
/** The open offer from Brand (its contract id), "" for none. */
SHORTSTACKCORE_API std::string OpenOfferFor(const Book& B, const std::string& BrandId, double World);
SHORTSTACKCORE_API std::string CanSign(const Book& B, const life::State& L, const std::string& ContractId, double World);
SHORTSTACKCORE_API std::string Sign(Book& B, life::State& L, Chips& Bankroll, const std::string& ContractId, double World);
/** What signing would collide with, in red over Sign ("Volt Rush has your energy-drink category until Apr 12."). */
SHORTSTACKCORE_API std::vector<std::string> Conflicts(const Book& B, const life::State& L, const std::string& ContractId, double World);
enum class Lever : int
{
	Money,
	Lighter,
	Shorter,
	OneMoreEntry,
	NoExclusivity,
};
SHORTSTACKCORE_API double Leverage(const Book& B, const rep::Public& P, const std::string& BrandId);
/** How far inside what the brand would take a lever is (rep::Mood reads it): positive inside, negative past it. */
SHORTSTACKCORE_API double Margin(const Book& B, const rep::Public& P, const std::string& ContractId, Lever L);
/** What the lever would change, in a line ("+25% on the stipend and the rates."). */
SHORTSTACKCORE_API std::string LeverLabel(Lever L);
/** The rule (§3.7, no randomness): 2 they meet it, 1 they meet you halfway (more money, lighter asks), 0 they pass. */
SHORTSTACKCORE_API int CounterOutcome(Lever L, double Leverage);
/** Why a lever can't be pulled on this offer ("" when it can): nothing to raise, nothing to lighten, already short. */
SHORTSTACKCORE_API std::string LeverBlocked(const Contract& C, Lever L);
SHORTSTACKCORE_API std::string Counter(Book& B, const rep::Public& P, const std::string& ContractId, Lever L, double World);
SHORTSTACKCORE_API void Decline(Book& B, const std::string& ContractId, double World);
SHORTSTACKCORE_API std::string GiveNotice(Book& B, life::State& L, const std::string& ContractId, double World);
SHORTSTACKCORE_API std::string AskForRaise(Book& B, const rep::Public& P, const std::string& ContractId, double World);
/** "" or what's missing (the asks' patch rules, read against what's sewn and worn). */
SHORTSTACKCORE_API std::string Fits(const Book& B, const wardrobe::Closet& C, bool AtEmbercrest, double World);
SHORTSTACKCORE_API void NoteWorn(life::LiveEntry& E, const std::string& Hero, const wardrobe::Closet& C, const Book& B, double World);
/** Sews Brand's patch on Spot (Brand "" takes it off): "" or why not. Only an active contract's patch, on a spot it takes. */
SHORTSTACKCORE_API std::string Sew(wardrobe::Closet& C, const Book& B, int SpotIndex, const std::string& BrandId, double World);
/** The brands whose patch can go on Spot now (active contracts that take it). */
SHORTSTACKCORE_API std::vector<std::string> PatchesFor(const Book& B, int SpotIndex, double World);
/** "Wearing Lucky Penny (chest) · Volt Rush (cap)": the sit-down toast, "" with nothing sewn that counts. */
SHORTSTACKCORE_API std::string WearingLine(const wardrobe::Closet& C, const Book& B, double World);
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
/** The funding a covering contract gives O (From Sponsor, Ref, Who, Cents the buy-in, the fares, a cut). */
SHORTSTACKCORE_API life::Funding CoverFunding(const Contract& C, const live::Occurrence& O);
/** Every contract that covers entries, as a funding option for O and why not ("" when it can): the desk and FundingOptions. */
SHORTSTACKCORE_API std::vector<std::pair<life::Funding, std::string>> CoverOptions(const Book& B, const live::Occurrence& O, double World);
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
// Update 2 (§5.7; core-deals fills them): a contract's "plays only on" clause. Conduct(AccountRestricted) ends only the
// contracts whose brand's Site is RiverLine (§5.5).
/** Whether playing (or streaming, or posting a video) on Site would be a strike under a running clause: the warning first. */
SHORTSTACKCORE_API bool WouldStrike(const Book& B, int Site, bool Streaming, bool Video);
/** The player played (streamed, posted) on Site: a strike under a running "only" clause. */
SHORTSTACKCORE_API void NoteSitePlay(Book& B, life::State& L, int Site, double World, bool Streaming, bool Video);
SHORTSTACKCORE_API bool Active(const Book& B, const std::string& BrandId, double World);
/** The contract running for Brand at World (Active or under notice, inside its term), or nullptr. */
SHORTSTACKCORE_API const Contract* Running(const Book& B, const std::string& BrandId, double World);
SHORTSTACKCORE_API const Contract* Find(const Book& B, const std::string& ContractId);
SHORTSTACKCORE_API Contract* Find(Book& B, const std::string& ContractId);
/** The store credit first; returns what the bankroll still owes. */
SHORTSTACKCORE_API Chips SpendCredit(Book& B, Chips Due);
/** A paid visit to a venue on the block (Session::ShopVisit): the block road's day. */
SHORTSTACKCORE_API void NoteVisit(Book& B, const std::string& Venue, double World);
/** A service on the house now (Fifth St. Barbers' cut every 14 days): the venue's offer is rung up at $0. */
SHORTSTACKCORE_API bool ServiceCovered(const Book& B, const std::string& Venue, double World);
SHORTSTACKCORE_API void UseService(Book& B, const std::string& Venue, double World);
/** A subscription the brand pays for (IronHouse's membership): RenewGear skips it. */
SHORTSTACKCORE_API bool CoversGear(const Book& B, const std::string& GearId, double World);
/** The pawn refuses it (§3.6). Ids: a gear id, or "gear:<id>" / "garment:<id>". */
SHORTSTACKCORE_API bool IsProperty(const Book& B, const std::string& GearOrGarmentId, double World);
/** Brand, colour. */
SHORTSTACKCORE_API std::vector<std::pair<std::string, uint32_t>> Badges(const Book& B, double World);
SHORTSTACKCORE_API std::vector<life::Mark> Marks(const Book& B, double From, double To);
SHORTSTACKCORE_API Chips NpcWeekly(const std::string& BrandId);

// ---- the contract, as the Mail app's card and Career show it
/** Which period World falls in (1-based; 0 before the start) and its bounds. */
SHORTSTACKCORE_API int PeriodAt(const Contract& C, double World);
SHORTSTACKCORE_API double PeriodStart(const Contract& C, int Period);
SHORTSTACKCORE_API double PeriodEnd(const Contract& C, int Period);
SHORTSTACKCORE_API int Periods(const Contract& C);
/** This period's asks so far: the counted ones from the book, the rest measured from the records (streams, entries, workouts). */
SHORTSTACKCORE_API Tally Measure(const Contract& C, const life::State& L, const kast::Channel& Ch, double World, double StreamStart = -1.0);
/** One ask's target this period (scaled for a short last period and a lighter counter). */
SHORTSTACKCORE_API double Need(const Contract& C, Ask A, int Period);
SHORTSTACKCORE_API double Have(const Tally& T, Ask A);
SHORTSTACKCORE_API const char* AskLabel(Ask A);
/** OFFER, ACTIVE, STRIKE, NOTICE, ENDED. */
SHORTSTACKCORE_API const char* Pill(const Contract& C, double World);
struct Card
{
	std::vector<std::string> Get;   // what you get
	struct Row
	{
		std::string Label;
		double Have = 0, Need = 0;
	};
	std::vector<Row> Asks;          // what they ask, with this period's progress
	std::string Deadline;           // "Period ends Mar 3 · 9 days"
	std::vector<std::string> Fine;  // the fine print
};
SHORTSTACKCORE_API Card CardOf(const Contract& C, const life::State& L, const kast::Channel& Ch, double World);

// ---- copy (every line passes the copy lint: CopyLint.h)
/** Every letter body (Kind 0), subject (1) and spoken line (2) the book can write, as (key, template). */
SHORTSTACKCORE_API std::vector<std::pair<std::string, std::string>> Inventory(int Kind);
/** The variables a template may use, with a sample of realistic length for each (the lint fills them). */
SHORTSTACKCORE_API std::map<std::string, std::string> InventorySamples();

/** Includes sponsor\tpatch for the closet. */
SHORTSTACKCORE_API void WriteLines(const life::State& L, std::string& Out);
SHORTSTACKCORE_API bool ReadLine(const std::vector<std::string>& Fields, life::State& L);
/** Kast's legacy deals (§3.8). */
SHORTSTACKCORE_API void AfterParse(SaveData& Data);
} // namespace sponsor
} // namespace ss
