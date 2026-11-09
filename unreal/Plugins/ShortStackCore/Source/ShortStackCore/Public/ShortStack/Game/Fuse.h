#pragma once

#include "ShortStack/Common.h"
#include "ShortStack/Game/Lobby.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace ss
{
namespace world
{
class World;
}

/**
 * Update 3 (SPEC §2.6 F7, §7.3): FUSE, Ignite's three-player lottery hyper. LIGHT IT, the fuse burns, the reel lands on
 * the prize: x2 to x1,000 the buy-in at $1.10 (x250 at $3.30 and $5.50). The multiplier is drawn from a key on the tier
 * and its ordinal (every FUSE entered at the tier, paid or on a ticket), saved at entry before the reveal; the pool is
 * exactly the multiplier times the buy-in. Session-only: never planned by the world, never in its records or History.
 * Declared by the lead in Phase A; bodies by core-fuse (Fuse.cpp; SessionFuse.cpp's glue after spike S-J).
 */
namespace fuse
{
/** One row of the prize table: the multiplier and its probability at $1.10, and at $3.30 and $5.50. */
struct Row
{
	int M = 0;
	double P[2] = {0, 0};
	bool Split = false; // x25 and up: 80/12/8
};
/** Seven rows; the sum of p x m is 3 x (1 - edge) exactly per tier. */
SHORTSTACKCORE_API const std::array<Row, 7>& Table();
/** The house edge: S-J's knob, 0.08 to 0.12; the lead's calibration (G2) is 0.10 (S-J.md §3, with the "high" fillers and 300 chips). */
SHORTSTACKCORE_API double Edge();
/** The edge's share of the buy-in: what the rake hooks see (never the tournament spec's FeeCents). */
SHORTSTACKCORE_API Chips FeeOf(int Tier);
SHORTSTACKCORE_API bool IsFuse(const std::string& EventId);
/** The N-th multiplier at a tier (N counts every FUSE entered there, paid or on a ticket). */
SHORTSTACKCORE_API int Draw(uint64_t WorldSeed, int Tier, int N);
/** "ignite.fuse<tier>@<minute>#<n>". */
SHORTSTACKCORE_API std::string IdFor(int Tier, double World, int N);
/** The synthetic event: 3 entrants, Freezeout, "Hyper", TableSize 3, FeeCents = BuyIn, GTD = m x buy-in (the pool exact). */
SHORTSTACKCORE_API ss::LobbyEvent EventFor(int Tier, int M, double World, int N);
/** The places' prizes: winner takes all to x10; 80/12/8 from x25. */
SHORTSTACKCORE_API std::array<Chips, 3> Payout(int Tier, int M);
/** The two Ignite members online that hour who take the other seats (keyed, never NameField's own Rng). */
SHORTSTACKCORE_API std::vector<std::string> SeatsFor(const world::World* W, uint64_t WorldSeed, int Tier, int N, double World);
/** What the reveal shows (Session::FuseReveal). */
struct Reveal
{
	int M = 0;
	double PaidAt = 0;
	int Tier = 0;
	bool Seen = false;
};

// ---- Appended at G2 by core-fuse (append-only; every consumer told in STATUS_core-fuse.txt).
// Tiers are 0, 1, 2 ($1.10, $3.30, $5.50): the cabinets fuse0..2, FuseRec::N's index, the id's digit.
// Reveal (Session::FuseReveal(Table)): M and Tier are core's (the table's own spec); PaidAt is the session's presentation
// clock (Session::Now) when LIGHT IT went through, so a page rebuilt mid-reveal can carry on from Now - PaidAt; Seen turns
// true once RevealSeconds(M) + 1 s have passed (the table opens itself 1 s after the boom, §2.4 D), sooner on TakeSeat and
// later while the laptop still draws it (RevealShown; both below, G2 fix round). M is 0 when the table isn't a FUSE.
constexpr int TierCount = 3;
/** The table at any edge (§2.6 F7): the x5-and-up rows fixed, x2 and x3 re-solved so each column sums to 1 and to
 *  3 x (1 - edge). Table() is TableAt(Edge()). */
SHORTSTACKCORE_API std::array<Row, 7> TableAt(double Edge);
/** The tier's buy-in in cents: 110, 330, 550 (0 for a tier that isn't one). */
SHORTSTACKCORE_API Chips BuyInOf(int Tier);
/** The tier's top multiplier: x1000 at $1.10, x250 at $3.30 and $5.50 (Table()'s last row holds 1000). */
SHORTSTACKCORE_API int TopOf(int Tier);
/** A row's multiplier at a tier (the top row reads TopOf) and its probability there (P[0] at $1.10, P[1] above). */
SHORTSTACKCORE_API int MOf(const Row& R, int Tier);
SHORTSTACKCORE_API double POf(const Row& R, int Tier);
/** The starting stack (S-J's third knob; 300 chips at the lead's calibration) and the seat-fillers' population ("high"). */
SHORTSTACKCORE_API Chips StartingStack();
SHORTSTACKCORE_API const char* Fillers();
/** The blind levels: 10/20, 15/30, 20/40, 30/60, 40/80, 50/100, 60/120, 80/160, 100/200, then +25% a level (to 10). */
SHORTSTACKCORE_API std::vector<ss::Level> Levels();
/** "FUSE $1.10": the event's name, its wallet lines ("Buy-in: FUSE $1.10", "Ticket: FUSE $1.10") and its ticket's label. */
SHORTSTACKCORE_API std::string NameOf(int Tier);
/** A FUSE id's tier and ordinal ("ignite.fuse<tier>@<minute>#<n>"); -1 when it isn't a FUSE id. */
SHORTSTACKCORE_API int TierOf(const std::string& EventId);
SHORTSTACKCORE_API int OrdinalOf(const std::string& EventId);
/**
 * Where a FUSE ticket is held in life::State::Tickets: "ignite.fusetkt<tier>". Never the id RegisterEvent's ticket test
 * reads ("ignite.fuse<tier>"), so a paid LIGHT IT never spends one; LightFuse(Tier, true) moves one across for the entry.
 * Every loop that grants a FUSE ticket (the Prize Counter, the wheel, a Neon cracker) credits this id.
 */
SHORTSTACKCORE_API std::string TicketId(int Tier);
/** The tier of a FUSE ticket id (TicketId's), -1 for any other ticket. */
SHORTSTACKCORE_API int TierOfTicket(const std::string& TicketId);
/** How long the reveal plays before the boom has settled, by multiplier (§2.4 D): 1.5 s to x3, 3.4 s to x10, 6.3 s above. */
SHORTSTACKCORE_API double RevealSeconds(int M);
/** A FUSE table's multiplier (GTD / buy-in) and what a place there pays (Payout's; 0 past 3rd), from the tournament's own
 *  spec. The engine pays the winner the whole pool; the session pays these (ShowResults, the banners, Kast's moment). */
SHORTSTACKCORE_API int MultiplierOf(const ss::TournamentSpec& Spec);
SHORTSTACKCORE_API Chips PrizeAt(const ss::TournamentSpec& Spec, int Place);
/** FUSE's own copy (LightFuse's answers, the table's lines, the ticket labels), for the copy lint and the trademark screen. */
SHORTSTACKCORE_API std::vector<std::string> Inventory();
} // namespace fuse

namespace site
{
namespace play
{
struct FuseRec;
struct SettleFacts;
} // namespace play
} // namespace site
class Session;

namespace fuse
{
/**
 * The record after a FUSE's result (site::play::OnResult calls it when F.Fuse): Played, Won, the prize into NetCents,
 * the Fuse Wall (the player's own FUSEs of x10 and up, the biggest 10 kept) and Big hits (every FUSE of x10 and up the
 * player sat in, with its winner and first prize; the newest 10). The entry side (the ordinal, Last10, BestM, the paid
 * buy-in out of NetCents, RecentBuyIns) is LightFuse's, at entry. Day is the 6 AM site day of World.
 */
SHORTSTACKCORE_API void NoteResult(site::play::FuseRec& R, const site::play::SettleFacts& F, double World);
/** The entry side of the record, as LightFuse writes it (exposed for the save's tests and the dev tools). */
SHORTSTACKCORE_API void NoteEntry(site::play::FuseRec& R, int Tier, int M, bool Paid);
/** True when the record holds anything, BandOf's recent buy-ins included (an untouched book writes no line). */
SHORTSTACKCORE_API bool HasRecord(const site::play::FuseRec& R);
/**
 * The record's save line (§7.4), "site\tfuse\tignite\t" n1 n3 n5 played won netCents bestM wall last10 hits recent, with a
 * newline, appended to Out only when HasRecord (core-siteplay's site::play::WriteLines calls it); ReadLine takes the split
 * line (P[0] "site", P[1] "fuse") and fills the record (core-siteplay's ReadLine dispatches "fuse" to it). Round-trips byte
 * for byte.
 */
SHORTSTACKCORE_API void WriteLine(const site::play::FuseRec& R, std::string& Out);
SHORTSTACKCORE_API bool ReadLine(const std::vector<std::string>& P, site::play::FuseRec& R);
/** The record in a few lines, for ss.Site.Play describe (core-siteplay's DebugSitePlay prints it). */
SHORTSTACKCORE_API std::string Describe(const site::play::FuseRec& R);
/**
 * Dev only (ss.Site.Play fuse <tier> <m>; core-siteplay's DebugSitePlay forwards here): the next FUSE lit at the tier in
 * this session plays for M (one of the tier's rows) instead of its drawn multiplier; the ordinal still advances, and the
 * table's chat says the multiplier was set by hand. "" or why not.
 */
SHORTSTACKCORE_API std::string DebugForce(Session& S, int Tier, int M);
/** Drops a session's FUSE side state (the reveal's clocks, a forced multiplier): ~Session calls it (the lead's patch,
 *  patches/core-fuse_to_lead_1.diff); until then a dead session's few bytes stay, and a light always rewrites its own. */
SHORTSTACKCORE_API void Forget(const Session* S);

// ---- Appended in the G2 fix round by g2f-fuse (M3: the first deal waits for the reveal; append-only).
/**
 * TAKE YOUR SEAT (§2.4 D): the reveal of the FUSE with that event id counts as seen from now (FuseReveal's Seen), so its
 * first hand is dealt on the usual beat. The laptop calls it when the seat is taken, or when it opens the table itself;
 * without it the reveal is seen RevealSeconds(M) + 1 s after the light, once the laptop has stopped drawing it.
 */
SHORTSTACKCORE_API void TakeSeat(Session& S, const std::string& EventId);
/**
 * The laptop drew that FUSE's reveal this frame (its Odds drawer included): while it keeps drawing it, the reveal isn't
 * seen, even past its run, so the first deal never comes under it. A reveal not drawn for a second (the laptop shut
 * mid-reveal) goes by the clock alone.
 */
SHORTSTACKCORE_API void RevealShown(Session& S, const std::string& EventId);
/**
 * The table in front's first hand waits (Session::TableStep asks before its first event, patches/g2f-fuse_to_lead_1.diff):
 * true at a FUSE whose reveal isn't seen yet, so nothing is dealt, auto-folded or put on the player's clock under the reveal.
 * Never at another table, a later hand, or a table a plan or a skip has (autoplay and skips go on as ever). The first false
 * answer for a FUSE is when its first deal went (FirstDealAt).
 */
SHORTSTACKCORE_API bool HoldFirstDeal(const Session& S);
/** When the FUSE with that event id was let deal its first hand (the presentation clock); -1 until then, or for no such FUSE. */
SHORTSTACKCORE_API double FirstDealAt(const Session& S, const std::string& EventId);
} // namespace fuse
} // namespace ss
