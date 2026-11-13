// Update 4, Festival Season (SPEC v1.7 §2.8.5, §2.9.3, §2.9.4): the session's festival rules, beside H/Session.h's frozen members.
// F-world's private header (G1's FW-5; private, so not frozen, Q-5): what G/Session.cpp's ShowResults sends and plays for a finished
// festival event, the seat line, T12's fill, the cap, the moments the marks leave, and Rerun's festival stories (G/SessionVideo.cpp).
// FW-6: FestStep's crown tickets, step refunds and crown letter as functions of the career's state and the world (the session's step
// calls them; the checks call them at any minute without running a world there), and the ladder's seat line.
// Bodies in G/SessionFest.cpp; tests/festivals_world.cpp calls them directly.
#pragma once

#include "ShortStack/Game/Session.h"

#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace life
{
struct State;
}
namespace world
{
class World;
struct HeroFestMark;
}
namespace fest
{
struct Festival;
}

namespace session_fest_detail
{
/** ShowResults' festival branch (§2.9.3's order, §2.8.5): what one finished event sends and plays. */
struct FestResult
{
	/** A festival Main won or final-tabled (1st to 9th), or a championship's Mini or Micro Main won: the branch that comes first, so
	 *  today's bracelet, ring and series-title lines don't follow it (a Championship Online Main win sends T8, not "bracelet-first"). */
	bool Branch = false;
	/** Dee's text (StoryText, once per key): fest-main-<series id> (T1-T8, T15, T16), fest-title-<series id> (T17) or
	 *  fest-ft-<series id> (T9); both empty when the cap holds it back (one of win, final table or mark an edition, §2.9.3). */
	std::string Key, Body;
	/** Kind 1 (the Main), 5 (a championship's Mini or Micro; Mark carries the tier, 1 Mini, 2 Micro) or 3 (any other festival event
	 *  won; never a satellite or a seat), Key the edition's series id; set after the result is paid. */
	std::optional<FestMoment> Moment;
};
/** The festival branch for the hero's finish at Place in EventId ("<template>@<minute>"); SeatWon: the event seated the hero. */
FestResult FestResultOf(const std::string& EventId, int Place, bool SeatWon, Chips Prize, const std::set<std::string>& TextsSeen);

/** T11 (§2.9.3, v1.6) for a seat won in satellite EventId ("<template>@<minute>") whose ticket is an edition's Main ticket (its Main
 *  template's Ticket, or its id), filled with the edition whose Main that ticket enters next (a championship's Main ticket is
 *  year-free) and SatBuyInCents; "" for every other seat: RCOP's "rcop-main" keeps today's line, and a championship's Step 1 or Step 2
 *  seat (a step ticket) keeps the ladder line. */
std::string FestSeatText(const std::string& EventId, const std::string& SeatTicket, Chips SatBuyInCents);

/** T12's {Step1} (§2.2.2): RiverLine's step1 buy-in as players write it ("$2.20" today, so gh0stfold's line is byte-identical). */
std::string FestStep1Fill();

/** The cap (§2.9.3): edition SeriesId already sent its win, title, final-table or mark line (the belt's is the other one). */
bool FestWinLineSent(const std::set<std::string>& TextsSeen, const std::string& SeriesId);

/** The moment the hero's marks leave (§2.8.5): the first Title mark whose "festbelt:<key>" is not in Done (Kind 2), else the first
 *  TOP 25%, TOP 10%, FINALIST or PODIUM mark whose "festmark:<key>" is not (Kind 4), crown order; Prize carries the race points
 *  (rounded), Mark the grade, Rank and Scorers the race's. nullopt: none waits. */
std::optional<FestMoment> FestMarkMoment(const world::World& W, const std::set<std::string>& Done);

/** Rerun's festival stories (§2.9.4): a text key's prefix, 0 "fest-main-", 1 "fest-belt-", 2 "fest-ft-" (matched with
 *  rfind(prefix, 0) == 0); -1 for every other key. */
int FestStoryPrefix(const std::string& Key);
/** The story's facts at NoteStory time (§2.9.4, V1-V9), as Candidate::IdeaFacts' display facts: Festival (the edition's whole name;
 *  a title's fill drops its leading "The " after "the " or "next ", G/VideoCopy.cpp's FestStoryNameFill), and TROPHY and BuyIn (a
 *  Main), N (the belt: the hero's events in the edition) or Place (a final table: the hero's Main finish). Empty for a key that is no
 *  festival story. */
std::vector<std::pair<std::string, std::string>> FestStoryFacts(const world::World& W, const std::string& Key);

/** Dee's line for a seat a satellite won into a step ticket or any seat T11 doesn't name (the existing line, one literal, read by
 *  ShowResults and ss.Fest.Result's s1 seat). */
const char* FestLadderLine();

/** FW-6 (§2.6.1 step 7): the ticket a crown mark earns in the brand's next edition (the first edition of the brand after F), as the
 *  wallet keys it: THE BELT its Main's ticket (Network::TicketOf: a championship's year-free "<site>.<brand>-main", RCOP's
 *  "rcop-main"), PODIUM its Main Mega Satellite ("-sat1", RiverLine "-mega", RCOP "mega-rcop"), FINALIST its Last Chance ("-sat2",
 *  RiverLine "-last", RCOP "step4"; WRIT's one satellite, The Summons, for both). "" for any other mark, or when the brand has no next
 *  edition or that edition no such satellite (Summer Slam has none). */
std::string FestCrownTicket(const fest::Festival& F, int Mark);
/** FW-6: FestStep's ticket for mark K (§2.6.1 step 7, §2.13): when K carries an unpaid ticket (fest::kCrownTickets) and
 *  "festticket:<key>" is not in L.Sites.Done, the ticket FestCrownTicket names goes on RiverLine into L.Tickets, on Ignite and
 *  RunItTwice through site::play::Grant (Kind::Ticket, "<ticket>|festticket:<key>": the Value Governor, over the cap the Guest List);
 *  then the key and world::FestTicketPaid (the mark's t drops). True when it paid or queued now (nothing when K has no ticket left). */
bool FestPayCrownTicket(life::State& L, world::World& W, const world::HeroFestMark& K, double Now);
/** FW-6 (§2.6.1 item 8, v1.6): after a championship's crown, each step ticket of that edition the hero holds (a ticket one of its
 *  satellites seats, never the Main ticket) that Network::NextFor can no longer place is credited at its SeatValue (times the count)
 *  into the site's wallet once, keyed "festcredit:<ticket>" in L.Sites.Done, with one ledger line (ledger:festcredit), and leaves the
 *  wallet; not governed (a refund, not a promotion). The number of tickets credited now. */
int FestStepCredits(life::State& L, const world::World& W, double Now);
/** FW-6 (§2.9.3, L5-L6): the crown letter for edition F's belt won with Points race points, (subject, body): "{Festival} {Year}: the
 *  belt is yours" ({Festival} the edition's name without its year), its body's two {Festival} after "the " and "next " through
 *  fest::NameAfterArticle. */
std::pair<std::string, std::string> FestCrownLetter(const fest::Festival& F, double Points);
} // namespace session_fest_detail
} // namespace ss
