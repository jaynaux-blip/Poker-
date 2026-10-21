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
/**
 * The mood at a world minute: Afterburner inside Ignite's SiteDef night window (NightFromMin..NightToMin, 00:00-06:00), then
 * Wake-Up to 12:00, Daytime to 18:00, Prime Time to midnight. The one place the hour is read (SitesSkin's Afterburner choice
 * agrees with it).
 */
SHORTSTACKCORE_API Mood IgniteMood(double World);
/**
 * The Board's OVERLAY column: max(0, GTD - RegisteredSoFar x the template's median late-reg multiplier x the per-entry prize
 * contribution), "PROJ. ±" while registration is open; once it has closed (Actual) the event's actual overlay. 0 for an event
 * with no guarantee or no buy-in, before registration opens, and while nobody is registered (nothing to project from).
 */
SHORTSTACKCORE_API Chips ProjectedOverlay(const net::Network& N, const net::EventInstance& E, int RegisteredSoFar, double World, bool& Actual);
/**
 * The Club card's live accrual: the tier, the next, the month's rake, what it has earned and what's left to the next. A pure
 * function of the account's RakeMonth through the Second Run Club's own table (the cents SiteCloseMonth pays on the 1st).
 * Tier 0 is "no tier"; at the top (Inner Circle) Next == Tier and ToNext == 0.
 */
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
	// G1 appends (core-sites-look, spike S-D §5; append-only):
	int Shared = 0;      // the board cards both runs share: 0 (all in preflop), 3 (on the flop) or 4 (on the turn)
	int Second = 0;      // the second run alone (the Luck Ledger's count): 0 lost, 1 part (a chop, or a main pot without the side), 2 won
	bool HeroIn = false; // the player was live in a pot at the all-in; Verdict and Second mean nothing without it
	// Fix append (secondboard, the lead's PIE 2026-10-21; append-only):
	int First = 0; // the real run alone, as the engine paid it (H.PotResults): 0 lost, 1 part, 2 won; Verdict is VerdictOf(First, Second)
};
/**
 * The Second Board from the hand's own deck stub (Hand::DeckForShow), else a keyed draw; draws nothing from H's Rng.
 * HeroSeat is an index into H.Seats (as autoplay::AllInEquity takes it), -1 when the player wasn't dealt in. True only for a
 * finished hand whose cards went on their backs with cards to come (the hand's first Reveal, at 0, 3 or 4 board cards):
 * never after a fold, a river all-in or an unfinished hand. The stub is walked as a dealer deals a second run (a burn before
 * each street still to come); FromStub is false only when the deck is too short for that (a fixed test deck), and then the
 * board is Rng("rit/second/" + HandId) over the cards nobody was dealt.
 */
SHORTSTACKCORE_API bool SecondRun(const Hand& H, int HeroSeat, const std::string& HandId, SecondBoard& Out);
/**
 * The Desk's numbers over the last Days days (Days <= 0: all of History), from History's rows on the account's site that
 * started by World: Events, ROI ((prizes - buy-ins) / buy-ins), ITM and FT as shares of Events (a final table is 1st to 9th,
 * as the boards count it), ABI over the paid entries, Net (prizes - buy-ins), Rake (the site's fee on each paid entry) and
 * Club (RunItTwice: this month's accrual, ClubAccrual's; 0 elsewhere).
 */
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
	// G1 appends (core-sites-look; append-only): what the row is, and who (a world::World id, -1: nobody the world follows).
	int Kind = 0; // LiveKind
	int Npc = -1;
};
/** LiveRow::Kind. Fact rows are the time-stamped feed; Team and Notable are the Story's notable entries; Podium the result. */
enum LiveKind : int
{
	LiveFact = 0,    // a world fact at its minute (registration, the start, late registration, the close, the overlay, the end)
	LiveTeam = 1,    // a pro of the site's own team registered (a TEAM tag)
	LiveNotable = 2, // a player with a following registered (the Kast directory's streamers)
	LivePodium = 3,  // a place paid at the end (1st to 3rd), from the world's result
};
/**
 * Sunday Live's rows for E up to World, oldest first: only what has happened by World and only the world's own numbers (the
 * network's registrations at past minutes, its pool and payouts, the world's planned entrants and its result). No NPC chip
 * counts, ever (W5). Bounded: at most about 30 rows.
 */
SHORTSTACKCORE_API std::vector<LiveRow> SundayLive(const net::Network& N, const world::World* W, const net::EventInstance& E, double World);
/** Ignite's welcome bonus as the REWARDS page draws it: pending, released, the next release and the needle. */
struct WelcomeView
{
	Chips Pending = 0, Released = 0, NextIn = 0;
	double NeedleT = 0;
	double Ends = 0;
};
/**
 * Pending and Released as the account holds them; NextIn the rake still to pay before the next $5 is released ($5 for every
 * $20 of rake, SiteMoney.cpp: 0 once nothing is pending or the 60 days ran out); NeedleT the Fuel Gauge's needle, the share
 * of the bonus released so far (0..1); Ends the world minute the 60 days run out (0: no bonus started).
 */
SHORTSTACKCORE_API WelcomeView Welcome(const site::Account& A, double World);

// ------------------------------------------------------------------ G1 appends (core-sites-look; append-only, §7.1)

/**
 * The Board's OVERLAY cell for E at World, everything a row and the Prospectus need: Cents is ProjectedOverlay's figure (the
 * actual overlay once registration closed: Actual), Entries the field it projects (registered so far x the template's median
 * late-registration multiplier; the final field once closed), Registered the network's count so far. Shown: a guaranteed,
 * paid event whose registration is open with someone registered, or closed. Amber: $100 or more and 5% or more of the
 * guarantee (§3.6 R1).
 */
struct OverlayView
{
	Chips Cents = 0;
	double Entries = 0;
	int Registered = 0;
	bool Actual = false, Shown = false, Amber = false;
};
SHORTSTACKCORE_API OverlayView OverlayOf(const net::Network& N, const net::EventInstance& E, double World);
/**
 * The registration multiplier the projection uses at World: the template's median final field over its field so far at
 * this point of registration (the network's registration curve at its median), 0 before registration opens, 1 once it has
 * closed. ProjectedOverlay's entries are RegisteredSoFar times this.
 */
SHORTSTACKCORE_API double LateRegMultiplier(const net::Network& N, const net::EventInstance& E, double World);
/** The Desk's net line: the cumulative net (prizes less buy-ins) after each of the site's results in the range, oldest first. */
struct NetPoint
{
	double At = 0; // the event's start (world minutes)
	Chips Net = 0;
};
SHORTSTACKCORE_API std::vector<NetPoint> DeskNet(const std::vector<HistoryEntry>& H, net::SiteId Site, int Days, double World);
/** Sunday Live's registrations by minute: Points samples of the network's count from registration's opening to World (or its close). */
struct RegPoint
{
	double At = 0;
	int Entries = 0;
};
SHORTSTACKCORE_API std::vector<RegPoint> Registrations(const net::Network& N, const net::EventInstance& E, double World, int Points);
/** Every sentence site::show writes (Sunday Live's rows, with their {Vars}), for the copy lint and the trademark screen. */
SHORTSTACKCORE_API std::vector<std::string> Inventory();

// ------------------------------------------------------------------ G1 fix appends (fix-rit, G1_AUDIT L5; append-only)

/**
 * A final table's seats for an event id ("runittwice.the-sweat@1530"): its template's TableSize on its own network (8 at the
 * rivals' default, 6 at six-max), 9 when the template isn't known (a bare or pre-schedule id). MadeFinalTable: the place is
 * 1st to that, so on an 8-max event the final table is the last 8, not 9. Desk's FT counts by it from G1's fix pass (its own
 * comment above still says 1st to 9th), as RunItTwice's Ledger and Member File do; RiverLine's boards keep their own rule.
 */
SHORTSTACKCORE_API int FinalTableSeats(const std::string& EventId);
SHORTSTACKCORE_API bool MadeFinalTable(const HistoryEntry& Row);
/**
 * The rake the player actually paid on a site: the account's own tally (Account::RakeLife: site::OnBuyIn adds the fee of
 * every entry paid from the wallet). A ticket's entry pays no rake, so a ticket's face value never counts as rake, nor does
 * a freeroll. Desk's Rake is this from G1's fix pass, whatever its Days (History can't tell a ticket's entry from a paid
 * one), so the Desk's RAKE tile and RunItTwice's Ledger (RAKE PAID) always agree.
 */
SHORTSTACKCORE_API Chips RakePaid(const site::Account& A);

// ------------------------------------------------------------------ fix appends (secondboard, the lead's PIE 2026-10-21; append-only)

/** The verdict for a pair of runs (each 0 lost, 1 part, 2 won): 2 won both, 0 lost both, otherwise 1 ("split it"). SecondRun's rule. */
SHORTSTACKCORE_API int VerdictOf(int First, int Second);
/**
 * One Second Board as RunItTwice's Settlement keeps it (§3.4 I, §3.6 R8), every fact from the finished hand and none from
 * the table's display state, which runs behind the engine: the table reads the board at the showdown, a beat before the
 * award marks the seats' Winner, and the seats' equities have followed the run-out to the river's 1, 0 or a half by then.
 * Hero: the player's hole cards; Villain: the first opponent still in at the end, in H.Seats order; Ran: the real board;
 * Board: SecondRun's (the second run, Shared, First, Second, Verdict); Equity: the player's share when the cards went on their
 * backs (autoplay::AllInEquity, HandFacts::EquityAtAllIn's number on both paths). EventId and At are the caller's. A pure
 * function of the hand: the watch path's CurHand and the skip path's LastHeroHand give the same record.
 */
struct SecondRecord
{
	std::string EventId, HandId;
	double At = 0; // world minutes
	std::array<int, 2> Hero{{-1, -1}}, Villain{{-1, -1}};
	std::array<int, 5> Ran{{-1, -1, -1, -1, -1}};
	SecondBoard Board;
	double Equity = -1;
};
/** SecondRun plus the hands, the real board and the all-in's equity: true only when SecondRun is and the player was in it (HeroIn). Draws nothing. */
SHORTSTACKCORE_API bool SecondRecordOf(const Hand& H, int HeroSeat, const std::string& HandId, SecondRecord& Out);
/**
 * A log of Second Boards on both paths (the Settlement's; the session's post-hand hook calls it with every hand the player
 * was dealt): H's record, when it has one, appended with EventId and At (the player's seat found by HeroId), the oldest
 * dropped past Keep. True when one was added. Draws nothing.
 */
SHORTSTACKCORE_API bool NoteSecond(std::vector<SecondRecord>& Log, const Hand& H, const std::string& EventId, const std::string& HandId, double At, size_t Keep = 12);
} // namespace show
} // namespace site
} // namespace ss
