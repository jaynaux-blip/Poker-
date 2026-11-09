#pragma once

// Update 3 (SPEC §1.4, §2.6, §3.6): the rival sites' loops, shared between core-siteplay's three files (SitePlay.cpp: the
// Governor, the hooks, the step, the save lines and the view; SitePlayIgnite.cpp: Ignite's loops; SitePlayRit.cpp:
// RunItTwice's). Private to the module; the contract is SitePlay.h. Every name here starts with SitePlay, Ignite or Rit
// (the unity rules).

#include "ShortStack/Game/SitePlay.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ss
{
namespace site
{
namespace play
{
namespace siteplay_detail
{
// ------------------------------------------------------------------ the shared kit (SitePlay.cpp)
constexpr double SitePlayDayMinutes = 1440.0;
constexpr double SitePlayDayStartsAt = 360.0; // 6 AM: the Night Shift's close (life::NightShiftStart)
constexpr int SitePlayHoldItems = 12;         // claim lines (tickets) and the "Sparks owed" line together
constexpr int SitePlayHoldResets = 3;         // and the slots the resets that credit by themselves can take
constexpr int SitePlayHoldMax = 15;
constexpr int SitePlayPaidSlack = 2;          // a paid-entry marker outlives a day it never finished on (the 120-day prune)

/** What a grant pays when it pays: Sparks to the balance, a ticket to Life.Tickets, or cents to the site's wallet. */
struct SitePlayPayload
{
	int64_t Sparks = 0;
	std::string TicketId;
	Chips WalletCents = 0;
	std::string Label; // the ticket's or the wallet line's words ("Night Glow $2.20 ticket")
};

/** UI state the pages read through the view and nothing else (never saved, never an outcome): keyed by the book. */
struct SitePlayScratch
{
	const Book* For = nullptr;
	int64_t Stamp = -1;
	EntryView Last;
	std::vector<OpenView> Opened;
	std::vector<MomentView> Moments;
	std::string GradeEvent; // the hand hook's count of the event's graded decisions already seen
	size_t GradeSeen = 0;
	std::vector<std::pair<std::string, double>> Scored; // RunItTwice results this sitting and their League points (RitView::Scored)
};
SitePlayScratch& SitePlayScratchOf(const Book& B);

/**
 * Minutes since the play day's 6 AM start, [0, 1440): the one reading of "when in the day" the loops share (G2 PIE P3). Under
 * 360 is the morning (the Early bird's 6 AM to noon); 720 and up is the evening and the night through the 6 AM rollover (the
 * "after 6 PM" Dare, the Flame's nudge).
 */
double SitePlayIntoDay(double World);
constexpr double SitePlayNoonIn = 360.0;    // noon, in SitePlayIntoDay's minutes
constexpr double SitePlayEveningIn = 720.0; // 6 PM, in SitePlayIntoDay's minutes
/** The Governor's ring for a site (RiverLine's is never used). */
int SitePlayGovIndex(net::SiteId S);
/** Rolls a site's ring forward to Day (clearing the slots it passes) and its week to Day's week. */
void SitePlayRoll(Book& B, net::SiteId S, int Day);
int64_t SitePlayRake30(const GovRing& G);
int64_t SitePlayPromo30(const GovRing& G);
/** The ceiling at World: 0.75 inside Ignite's 60-day welcome window, else 0.50. */
double SitePlayCeiling(const life::State& L, net::SiteId S, double World);
bool SitePlayWelcome(const life::State& L, net::SiteId S, double World);
/** The week's stake-band cap in value units (0: uncapped), keyed on the week's average paid buy-in on that site. */
int64_t SitePlayWeekCap(const life::State& L, net::SiteId S, double World);
/** The Room a grant could take now (value units, never below 0): the ring's share less the week's cap, whichever is less. */
int64_t SitePlayRoom(const Book& B, const life::State& L, net::SiteId S, double World);
/** Books value into the ring (Spread: over its days in proportion to their rake; else today's slot) and the week's face. */
void SitePlayBook(Book& B, net::SiteId S, int Day, int64_t Value, bool CountsInWeek, bool Spread);
/**
 * The Governor's door for every cash-valued grant (§1.4): pays now when nothing waits and the Room and the week's cap take
 * it; else it queues on the Guest List (a Sparks grant on the one "Sparks owed" line). Reset marks a credit that came by
 * itself (a 6 AM Dare, a Wristband tier on the 1st, a League or Scalp ticket), which may take the reserved slots. Returns
 * true when paid now. A claim that can't queue (the claim lines full) isn't the Governor's to refuse: the claims ask
 * SitePlayHasItemRoom first.
 */
bool SitePlayGrant(Book& B, life::State& L, net::SiteId S, Kind K, const std::string& What, int64_t Value, const SitePlayPayload& Pay,
	const std::string& Key, int Day, double World, bool Reset);
/** Grant()'s done key for a key's idempotent part ("play:g:<site>:<key>"): set once that key has paid or queued. */
std::string SitePlayGrantDoneKey(net::SiteId S, const std::string& Idem);
/** Whether a claimed ticket could still queue (the claim lines aren't all taken). */
bool SitePlayHasItemRoom(const Book& B);
/** The day's spare Room into the oldest hold's Covered; clears what's covered, then any later hold that fits whole. */
void SitePlayClearHolds(Book& B, life::State& L, net::SiteId S, int Day, double World);
/** Pays a payload (the grant's or a cleared hold's): the balance, a ticket and its daily line, or the wallet. */
void SitePlayPay(Book& B, life::State& L, net::SiteId S, const SitePlayPayload& Pay, double World);
/** One aggregated 0-cent line a day per site: "Rewards: 2 tickets", "Prize Counter: Flip & Run $1.10 ticket". */
void SitePlayDailyLine(life::State& L, net::SiteId S, double World, const std::string& Prefix, const std::string& One);
/** The play book now has state (S2, S10): its lines are written from here on. */
void SitePlayTouch(life::State& L);
/** The book has state for a site (a paid entry or a claim made it): only then does Step move it (S10). */
bool SitePlayIgniteStarted(const Book& B);
bool SitePlayRitStarted(const Book& B);
/** A dated done key in the site book (SiteDated's form, so Update 2's 120-day prune covers it). */
std::string SitePlayDated(const std::string& Key, double World);
bool SitePlayDone(const site::Book& Sites, const std::string& Key);
void SitePlayMarkDone(life::State& L, const std::string& Key, double World);
/** Hash a key to [0, 1): Fnv1a(WorldSeed + ":" + What) / 2^32 (§1.9). */
double SitePlayUnit(uint64_t WorldSeed, const std::string& What);
/**
 * A prize draw's unit (the Spark Wheel, Crackers: the rewards with an Odds view, W5): §1.9's keyed Fnv1a, mixed by the murmur3
 * finalizer so its chances are the table's (G2 fix M2). The League draw and the NPC Callouts keep SitePlayUnit.
 */
double SitePlayDraw(uint64_t WorldSeed, const std::string& What);
/** "1,240". */
std::string SitePlayCount(int64_t N);
/** A paid entry's marker (OnPaidEntry sets it, OnResult takes it): the entry was wallet money, so the loops see it. */
void SitePlayMarkPaid(life::State& L, const std::string& EventId, double World);
bool SitePlayTakePaid(life::State& L, const std::string& EventId);
/** The site's cheapest paid entry's rake in cents (the Guest List's "about 4 Flip & Runs"), and its name. */
Chips SitePlayCheapRake(net::SiteId S);
const char* SitePlayCheapName(net::SiteId S);
/** The template part of an event id, its site's prefix taken off ("runittwice.the-sweat@1234" -> "the-sweat"). */
std::string SitePlayTemplate(const std::string& EventId);
/** The start minute in an event id ("...@1234" -> 1234; FUSE's "...@1234#5" too), or -1. */
double SitePlayStartOf(const std::string& EventId);
/** Escape and number helpers for the save lines (liveface's escape: backslash, tab, newline). */
std::string SitePlayEscape(const std::string& In);
std::string SitePlayUnescape(const std::string& In);
int64_t SitePlayInt(const std::string& S);
double SitePlayNum(const std::string& S);
std::string SitePlayNumText(double X);
std::vector<std::string> SitePlaySplit(const std::string& S, char Sep);

// ------------------------------------------------------------------ Ignite (SitePlayIgnite.cpp)
void IgnitePaidEntry(Book& B, life::State& L, Chips Fee, Chips BuyIn, bool Fuse, const std::string& EventId, double World);
void IgniteResult(Book& B, life::State& L, const SettleFacts& F, bool Paid, double World);
void IgniteHand(Book& B, life::State* L, const HandNote& N, double World);
/** Ignite's "Sparks owed" line cleared (SitePlayClearHolds): the Dares' and the spin's owed bits go (G2 fix M4). */
void IgniteOwedCleared(Book& B);
/** Brings Ignite's day, week and month up to Day (misses, the 6 AM Dare credit, the Monday bank, the Wristband's 1st). */
void IgniteRoll(Book& B, life::State& L, const sponsor::Book* Deals, uint64_t WorldSeed, int Day, double World);
void IgniteFillView(const Book& B, const life::State& L, const sponsor::Book* Deals, const world::World* W, uint64_t WorldSeed, double World, play::IgniteView& Out);
void IgniteWrite(const Book& B, std::string& Out);
bool IgniteRead(const std::vector<std::string>& P, Book& B);
void IgniteCopy(std::vector<std::string>& Sentences, std::vector<std::string>& Labels);
std::string IgniteDescribe(const Book& B, double World);
int8_t IgniteBand(const Book& B);
/** A Box onto the shelf (the 13th opens itself into Sparks). */
void IgniteDrop(Book& B, life::State& L, uint64_t WorldSeed, int8_t Tier, const std::string& Id, int Day, double World);
std::string IgniteSpin(Book& B, life::State& L, uint64_t WorldSeed, double World);
std::string IgniteOpen(Book& B, life::State& L, uint64_t WorldSeed, const std::string& BoxId, double World);
std::string IgniteClaimDare(Book& B, life::State& L, int Slot, double World);
std::string IgniteClaimTier(Book& B, life::State& L, int Tier, double World);
std::string IgniteBuy(Book& B, life::State& L, int Item, double World);
std::string IgniteEquip(Book& B, int Slot, int Look);
/** Dev: grant Sparks (governed), set a streak, set a level. */
std::string IgniteDebug(Book& B, life::State& L, const std::string& Verb, int64_t N, double World);

// ------------------------------------------------------------------ RunItTwice (SitePlayRit.cpp)
void RitPaidEntry(Book& B, life::State& L, Chips Fee, Chips BuyIn, const std::string& EventId, double World);
void RitResult(Book& B, life::State& L, const SettleFacts& F, bool Paid, double World);
void RitHand(Book& B, life::State* L, const HandNote& N, double World);
/**
 * The League's month (close on the 1st, the draw), the Matchweek's Callouts, on leave, and (End: the step's own end, not a
 * 6 AM it crossed) the desk bell. Forward only: an instant before what the book has stepped changes nothing. Fills Out's roster.
 */
void RitStep(Book& B, life::State& L, const world::World* W, const sponsor::Book* Deals, uint64_t WorldSeed, double From, double To, bool End, StepOut& Out);
/** The Matchweek's Callouts (RitStep's part): the baseline, a settled week, the NPCs' own draw (none while on leave). */
void RitCallouts(Book& B, const world::World* W, uint64_t WorldSeed, int Day, bool Leave);
/** The desk bell over the Watchlist at To (RitStep's part, a League month or not). */
void RitBell(Rit& R, double To);
/**
 * Why a Callout can't be made now ("" when it can): on leave (W7, §1.5), a clause that strikes RunItTwice in a month with no
 * RunItTwice day played, so no Callouts out.
 */
std::string RitCallOutBlocked(const Book& B, const sponsor::Book* Deals, double World);
/** Books the Second Run Club paid on a 1st into the ring's slots of the month that earned it (pro rata by its daily rake). */
void RitBookSecondRun(Book& B, life::State& L, double From, double To);
/** The same for one payment (Grant(Kind::SecondRun)'s door): count only, once per earning month. */
void RitBookSecondRunPay(Book& B, life::State& L, Chips Pay, int Earned, double MonthRake, double World);
void RitFillView(const Book& B, const life::State& L, const sponsor::Book* Deals, const world::World* W, uint64_t WorldSeed, double World, play::RitView& Out);
void RitWrite(const Book& B, std::string& Out);
bool RitRead(const std::vector<std::string>& P, Book& B);
void RitCopy(std::vector<std::string>& Sentences, std::vector<std::string>& Labels);
std::string RitDescribe(const Book& B, const world::World* W, double World);
std::string RitCallOut(Book& B, int Npc, double World);
std::string RitStar(Book& B, const std::string& TemplateId, bool On);
/** The League's month now: draws the group (the debug verb), or closes it (League Night next open). */
std::string RitDebug(Book& B, life::State& L, const world::World* W, const sponsor::Book* Deals, uint64_t WorldSeed, const std::string& Verb, int Npc, double World,
	StepOut& Out);
/** The player's League points this month (the best 8 days and the Bounce Back's 30). */
double RitHeroPoints(const League& Lg);
} // namespace siteplay_detail
} // namespace play
} // namespace site
} // namespace ss
