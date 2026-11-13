// Update 4, the festival hub (SPEC v1.7 §2.8): one shared module worn in each site's frame (THE FESTIVAL YEAR, the banner, the Main
// card, the day strip and rows, the ticker, THE RACE, CHAMPIONS, YOUR <FESTIVAL>), Main Event Day, the champion's, the belt's, a
// mark's and a title's moments, the strip's popover, the Career page's pins and the hub's strings (FestUiInventory). F-hubs' file
// from G1. It reaches the client's private state through rlfest_detail::Access (RiverLine.h's friend), forwarders defined here.
//
// Phase A (§6 PA-46; the lead): stubs that draw nothing. Draw calls FestivalOverlays at its overlay stage; no page routes to
// FestivalHub or CareerPins before G1. Access is empty.
//
// G1 FH-1a (F-hubs' stream A; SPEC v1.7 §2.8.1-§2.8.3, §2.1.4, §2.2.3, §2.11, §2.12; PLAN_G1 §2.1, PLAN_FHUBS §4 and its
// Corrections): the hub's skeleton. Stream B's FH-1c routes a festival series to FestivalHub on the three hosts (UI/RiverLineNet.cpp:
// NetHubEdition, NetHubRect); here:
// - rlfest_detail::Gather reads every fact the hub prints into one HubFacts value, once a frame: every digit a sim fact (§2.8.3) and
//   every world read the hub makes (the const views of H/WorldFest.h, FestRaceKey, FestTop, FestRoll and HeroFestPins, and
//   fest::PhaseOf's crown minute), so G2 can point it at the held view. It calls no HoldGuarded world method (PLAN_FHUBS Q17) and
//   draws no random number: every presentation clock (the countdown, the banner's motions) is real time and feeds no outcome (§2.12).
// - rlfest_detail::HubFrameOf: the per-host layout table (§2.8.2's rects: RiverLine's, Ignite's under L7, RunItTwice's bracketed
//   ones; FH-2 proves the last two with its pages).
// - The banner (the look's FestBanner and the FestWordMark overload, F-look's bodies from FL-7; today's crest at 150; the title, the
//   tagline and RCOP's second line; the four stats), the Main card (the date and start, "{Gtd} GUARANTEED", the buy-in, "+ {Added}
//   ADDED", the lock line, "{N} SEATS WON BY SATELLITE", Details `fsmain`), the right column's tabs `fstab0..2` with the phase's
//   preselection, and each tab's empty state (U6, U9, U11). The strip, the day strip, the rows, the ticker and the popover are FH-1b's;
//   the race's rows, the roll's rows and the YOUR panel FH-4A's; Main Event Day, Closing and the Afterglow FH-5b's.
// - The UI side state (the IgniteStates precedent): the tab the player picked and the hub's open time, kept by laptop and started
//   afresh whenever the client opens a page or a series, or shows another edition.
// - FestUiInventory: the strings this file draws, each with its kind (§2.11), then stream B's through rlfest_detail::HostUiStrings.
//
// G1 FH-1b (stream A; SPEC v1.7 §2.8.2, §2.8.3, §2.8.6, §2.4.4, §2.2.3, §2.1.4, §2.10 item 6, §2.11; PLAN_G1 §2.1): the rest of the
// hub's left half and the strip.
// - THE FESTIVAL YEAR (`fsy0..9` in calendar order, WRIT ... FEIGN): each brand's on-stage edition, else its next (PLAN_FHUBS Q24),
//   from fest::All() across the three sites (a brand with no edition yet leaves its cell empty); the low-detail crest at 26, the
//   Short, the opening date, "BY INVITATION" on a rival cell without an account (FH-2 adds the Pass's count, U30); an edition
//   inside its Announced-to-Afterglow window is lit, and the one on stage (else the next to open) carries the Accent underline.
// - The strip's popover (§2.8.6), drawn in FestivalOverlays: `{cell.X, 128, 360, 168}` clamped inside the hub's content rect, the
//   crest at 46, the name and its full title (N17-N19), the host and the window, U16's phase and account lines, and one button
//   (U17: OPEN THE HUB on the host in front, GO TO {SITE} for another site's edition with an account open; none without one). It
//   holds the pointer (house law 3): while it is open the whole hub draws under SitesBlockUnder's modal hold, so a click on it
//   reaches nothing beneath and a click elsewhere closes it, swallowed; the frame it opens takes no input of its own
//   (FestOneFrameClicks). Esc is the client's key handler's (UI/RiverLine.cpp), which F-hubs' files don't reach at G1.
// - The day strip (`fsd<day>`: today lit, a star on Main day, an outline star on a championship's other title days) and the day's
//   rows (`ev:<id>`, written to Listed): the edition's events that day, then one row for each satellite into its Main that runs
//   that day (the ladder up to it included: RCOP's step1-4 and mega-rcop, §2.2.3), seven in view and the rest under the wheel;
//   each row's button is the client's own RegisterBlock (SitesTwoPress, the clause's second press and core's strike gate, W3;
//   FestStrikeGate), shown on every row until one is armed and then on that one alone (Session::ConfirmRegister is one per
//   session, the RunItTwice Board's rule); a satellite into a Main that isn't joinable adds the Main's lock line and U31.
// - The overlay ticker (§2.4.4; none on Ignite's hub, L7): U26's forms for each of the edition's guaranteed headliners and the
//   Main's registrations so far, rebuilt once a world minute, one clipped run.
//
// G1 FH-2 (stream A, after F-net's FN-3 and FN-2b: the real calendar, all ten brands; SPEC v1.7 §2.1.3, §2.1.4, §2.8.1-§2.8.3,
// §2.8.6, §2.10 items 1 and 6, §2.2.3, §2.11; PLAN_G1 §2.1, PLAN_FHUBS §4 FH-2 and its Corrections): the hub in all three frames.
// - Ignite's rows (L7): {24, 408, 1000, 462}, six whole and the seventh's top under a fade from y 826 to the frame's foot, with
//   screen 12's scroll bar at x 1014 while the day holds more rows than show; no ticker; the body {1044, 392, 532, 478}.
// - GRIT (RunItTwice, "nights"): the day strip's cells read NIGHT {N}, the banner's state NIGHT {N} OF {Nights} (U2), and each show's
//   two runs that night share one row: its two buttons, "7 PM" and "10 PM" (the runs' own starts), beside the runs' codes from
//   Network::NameOf ("GRIT 07-E", "GRIT 07-L"), pick the run the row's RegisterBlock registers (the strike gate unchanged); the
//   ticker leads with U15's GRIT line while the edition is on stage.
// - The Summons (WRIT's satellite, no seat guarantee): U33, "{N} SEATS AT THE POOL SO FAR" while it runs (max(1, Pool / SeatValue),
//   the world's own seat rule, G/WorldSim.cpp:513) and "{N} SEATS WON" once finished (WRIT's one satellite, so fest::SeatsWon's sum).
// - THE FESTIVAL YEAR's Pass (§2.10 items 1 and 6): a rival cell without an account reads "PASS {N}/{Need}" while the Festival Pass
//   counts and "BY INVITATION" otherwise, from GateMet's own inputs (G/SiteMoney.cpp:512-538: the site's Gate.FestivalEvents, its
//   launch, Gate.MinRentsPaid, fest::PassFestival, fest::PassNeeds and max(P.OnlineEvents, P.Paid100)); the popover says it in words
//   (U30). Every site's FestivalEvents is -1 until F-world's FW-7, so every such cell reads BY INVITATION in this tree.
// - The locked Main (§2.2.3): the Main card's lock line, the ladder rows' lock line and U31 beside their buttons, THE RACE first while
//   on stage (FestLockedMain).
//
// G1 FH-4A (stream A, after F-world's FW-2b and FW-3 and F-net's commit with fest::YourLine's body; SPEC v1.7 §2.5.3-§2.5.5, §2.6.2,
// §2.6.4, §2.6.6, §2.8.2, §2.11, M-4; PLAN_G1 §2.1, PLAN_FHUBS §4 FH-4 and its Corrections; LEAD_CALLS R-18, R-33): the right column's
// three bodies, each reading only the const views (Gather; no HoldGuarded call, no random number).
// - THE RACE: "PLAYER OF THE SERIES" with EVERYONE / PEOPLE YOU KNOW (`fsrace0/1`, U5) over world::FestTop / world::FestKnown; rows at
//   40 (rank, the scorer's mark band as a drawn marker, an r 12 avatar, the name through PlayerName, points, and the movement since
//   yesterday as a drawn triangle beside an ASCII number); the YOU row (U7) at the body's foot and the marks legend (U8's mark labels
//   beside their markers, 12/600 Muted). The movement reads HubRaceMemo, the UI's own record per laptop and edition: the race as the
//   hub last read it, and the race as last read on the day before today, so nothing shows until the hub has seen a day turn after a
//   load (never a guess) and nothing after the crown, when FestTop holds FestLast's final ten. Ten rows on RiverLine and RunItTwice,
//   eight on Ignite with the YOU row at y 776 and the legend at y 834 (R-33, Q13).
// - CHAMPIONS over world::FestRoll, newest first: the edition (14/800), then MAIN EVENT and PLAYER OF THE SERIES, a championship's
//   edition (RCOP's included) with MINI MAIN EVENT and MICRO MAIN EVENT between them (U34) in 120 units, any other in 88; prizes in
//   NetMoney ("{Prize} · 1st of {Entries}", M-4), the belt's points ("{Points} pts"); as many editions as the body holds (six at 88 or
//   four at 120 in 558, five or three in Ignite's 478); U9's two lines while the roll is empty (FH-1a).
// - YOUR <FESTIVAL> (once the hero holds the edition's pin, else U11): the pin (FestPin at 96, 72 on Ignite) with the kept mark's
//   medal and U12's three grades beside it, the trophy, a title object and the belt when won, the ladder (§2.5.3: U8's seven rungs at
//   64 x 22, those reached filled, the next lit, U27's "{N} pts to {MARK} · right now" from world::FestPointsTo, none before the first
//   cash), the mark and "A seat in the next Main Event" after the crown (U28's line, a FINALIST or better), U10's line from
//   fest::YourLine (singular at one), U37's "Good on {Dates}." while a step ticket of the edition is in the wallet (R-18: the YOUR
//   panel's half), and the hero's results in the edition from world::HeroFestResults (six, four on Ignite).
// - FH-4A fix round 1 (U35, §2.8.2): a championship's step rows (PENNANT's and REIGN's Step 1-3, the edition's own one-off ladder,
//   HubSteps) carry "STEP {K}" in the number's place on the row's first line (a satellite has no number) and "FOR A STEP {K} TICKET"
//   or "FOR A MAIN EVENT SEAT" in SATELLITE's place, each figure from the ladder itself (the rung counted from the bottom; the rung its
//   seat is), never typed. RCOP's recurring step1-4 are no edition's own ladder and keep SATELLITE (U35 lists STEP 1-3).
//
// G1 FH-5b (stream A, after F-world's FW-4: WHO'S IN; SPEC v1.7 §2.8.4, §2.8.2, §2.1.4, §2.6.3, §2.6.7, §4.7.3, §2.11 U4, U13, U14,
// U38; PLAN_G1 §2.1, PLAN_FHUBS §4 FH-5 and its Corrections; LEAD_CALLS R-16, R-21, R-33): Main Event Day, the Afterglow and WHO'S IN.
// - The broadcast (Phase::MainDay, and Closing until the crown): the banner becomes the hero band (the crest, the word-mark at 34,
//   MAIN EVENT DAY, a split-flap row of 44 x 64 cells counting down to the Main's start and then flipping to its state, REGISTERING,
//   LATE REG, RUNNING, FINAL TABLE or COMPLETE, and "LIVE · {Left} LEFT OF {Entries}" from Network::Live); the opening bell (the look's
//   sting once per laptop and Main, and "The Main Event is under way." on an Accent band for 3 s, when the hub draws within three world
//   minutes of the Main's start); the guarantee meter inside the Main card (THE GUARANTEE, the pool so far against a tick at the
//   guarantee, PROJ SHORT or PROJ OVER from site::show::OverlayOf while registering, OVERLAY once registration closed short); the
//   schedule's place taken by REGISTRATIONS (site::show::Registrations as ChartArt's LineChart in the skin's theme) and WHO'S IN.
// - WHO'S IN from World::FieldStoriesOf(<the Main's instance>) (FW-4; a const view, cached per world state): the first twelve people
//   in the query's one order (eight on Ignite, R-33's two columns at 48), each an r 12 avatar, PlayerName and its line: the three
//   championship reasons through FestWhyLine (R-16, R-21), the others U38's labels ("Met {N}x" from the bond's encounters, "Defending
//   champion", "Won a seat in {Satellite}" naming the edition's one satellite into the Main, else "a satellite", "Team pro", "Streams on
//   Kast") and a roll champion's "{Festival} champion" (§4.7.3's "slychip · RCOP 2026 champion"); the Qualifiers group's count line
//   (U14, "regulars", singular at one) only when it is 1 or more. Empty (after the event, or before anyone registered): not drawn.
// - The reel and the champion card, once the Main's result exists (Network::Result): the reel takes REGISTRATIONS' place, FINAL TABLE
//   from 9th to 1st, one row every 0.6 s from the laptop's first draw after the result (PlayerName, prizes in NetMoney: the world's
//   result, or the engine's when the hero played); the champion card takes the Main card's (CHAMPION, the trophy at 64, the name or
//   YOU, the prize, U14's line "{Name} won {Trophy} and {Prize}." and the journey from World::ProfileOf: the arrival line and the
//   titles on record). The champion card stays through the Afterglow, where the hub is the schedule again (the banner's COMPLETE,
//   CHAMPIONS preselected).
// - The Main card's U4 line, "DEFENDING CHAMPION · {Name} · registered", while FieldStoriesOf lists a DefendingChampion (no Registered
//   call, Q17). A championship's Mini and Micro rows are pinned at the top of their day's rows (the broadcast is the Main's day only).
// - Previews (FestPreviewStill's flap, bell and reel half): every flap cell its settled character, the reel whole, no bell, no sound;
//   nothing a still draws touches the side state.
//
// G1 FH-6a (stream A; SPEC v1.7 §2.8.5, §2.5.3, §2.8.7, §2.7.6, §2.11; LEAD_CALLS R-7; PLAN_G1 §2.1 and item 16; PLAN_FHUBS §4 FH-6 and
// its Corrections): the moments, the pin wall's body and the stings.
// - FestivalOverlays plays Session::PeekFestMoment once, over the poker client in front (the lobby or a result; never at a table, at
//   the title screen, under a time skip or an outcome): the scrim {0, 0, 1600, 962} (Bg at 0.82) and, for Kinds 1, 2, 3 and 5, the card
//   {500, 170, 600, 580}: the crest slams in (Spring, 0-0.5 s), CHAMPION or PLAYER OF THE SERIES staged (0.4-1.0 s), U18's line, the
//   trophy (220; 260 for a championship's, the RCOP Cup included), the belt, the gold pin (Kind 3) or the title object (Kind 5; the gold
//   pin on RCOP) rising (0.8-1.6 s), the prize counting up (the belt's points), festival confetti (Accent, Accent2, Gold; 160 quads,
//   behind the card), FestChampion with CrowdApplause once, TAKE A BOW (`fsmoment`) and U19 after 1.6 s (a click anywhere on the scrim
//   too); beneath a Main's card U29's shelf line (kHubShelfShips, Q20) and, while its race is uncrowned, the belt line; from a
//   championship's second edition in this world the Main's line adds "{Ordinal} edition" (U36). Kind 4 is the mark card
//   {560, 250, 480, 420}: the edition's pin rises, the mark's medal drops onto it (Spring), the mark, the edition, U28's rank line and
//   the crown ticket's line when one was earned; no confetti and no sting, the Bubble cue. A DEV moment (R-7) prints DEV beside its
//   numbers; Session::DoneFestMoment on dismiss keys only the belt and the marks. A still while previewing: the settled frame, no cue,
//   no input, nothing granted. No sponsor mark is drawn on any moment (REIGN's title moment included).
// - The pointer: the overlay holds it from the app's frame through rlfest_detail::FestMomentHold (SitesBlockUnder over the scrim),
//   which RiverLine::Draw calls before the app (the lead's line, patch fhubs_to_lead_4); FestivalOverlays hands it back for the moment's
//   own clicks. Without that line a press beneath still never completes (the scrim takes the press), but a one-frame click would.
// - The stings (§2.7.6): the host's sting (HubStingOf: the look's, else the site's FestRiver, FestIgnite or FestRit) on the first live
//   hub open of a world day per festival on this laptop, and once when a festival registration is confirmed and core accepts it (a
//   new open table whose event is a festival's, whichever register button made it); the opening bell shares the open's sting when both
//   fall in one frame. Never while previewing.
// - CareerPins (§2.8.7), {1092, 656, 484, 104} on RiverLine's Career page (stream B's FH-6b calls it): hidden until the hero holds a
//   festival pin (World::HeroFestPins); "FESTIVAL PINS · {Year}" with ‹ › (`fpy-`, `fpy+`, vector marks) over the years holding a pin,
//   ten slots of 40 at x = R.X + 12 + 47i, y R.Y + 44 in calendar order: FestPin by grade and mark, an empty socket for an edition this
//   world holds that the hero skipped, a faint dash for one it never held (before Night One, or not founded); hover prints the facts.
#include "ShortStack/UI/RiverLine.h"

#include "../Game/SitesDetail.h"
#include "../Game/WorldFestDetail.h"
#include "RiverLineShared.h"
#include "ShortStack/Game/Festival.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/SiteShow.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Game/WorldFest.h"
#include "ShortStack/UI/ChartArt.h"
#include "ShortStack/UI/EventArt.h"
#include "ShortStack/UI/FestivalArt.h"
#include "ShortStack/UI/SiteMotion.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace ui
{
namespace rlfest_detail
{
// UI/RiverLineNet.cpp (stream B, FH-1c; LEAD_CALLS R-1): the host files' festival strings, joined to FestUiInventory below.
std::vector<std::string> HostUiStrings();
} // namespace rlfest_detail
} // namespace ui

namespace fest
{
// Step A-2 (LEAD_CALLS Q-5; moved here from G/Festival.cpp by the integrator): the hubs', the register sheets' and the takeovers' UI
// strings. G1 (FH-1a on): every string this file draws, each "<kind>\t<text>" (§2.11: a label or a sentence, which FestCopyUi maps to
// the stop rule and the cap), its placeholders filled from the sim when drawn; then stream B's (rlfest_detail::HostUiStrings).
std::vector<std::string> FestUiInventory()
{
	std::vector<std::string> Out = {
		// U2: the banner's fourth stat, a value over its label.
		"label\tOPENS IN",
		"label\t{D}D {H}H {M}M",
		"label\tPROGRESS",
		"label\tDAY {N} OF {Days}",
		"label\tMAIN EVENT TODAY",
		"label\tCOMPLETE",
		// U3: the banner's other stats and the Main card.
		"label\tGUARANTEED",
		"label\tEVENTS",
		"label\tDATES",
		"label\t{FirstDate} \xE2\x80\x93 {LastDate}",
		"label\tMAIN EVENT \xC2\xB7 {Weekday} {Date} \xC2\xB7 {Time}",
		"label\t{Gtd} GUARANTEED",
		"label\t{BuyIn} buy-in \xC2\xB7 {Stack} chips",
		"label\t+ {Added} ADDED",
		"label\t{N} SEATS WON BY SATELLITE",
		"label\t1 SEAT WON BY SATELLITE",
		"label\tDetails",
		// U5: the right column's tabs and THE RACE's title.
		"label\tTHE RACE",
		"label\tCHAMPIONS",
		"label\tYOUR {FESTIVAL}",
		"label\tPLAYER OF THE SERIES",
		// U6: THE RACE's lines.
		"sentence\tNo points yet.",
		"sentence\tEvery cash scores, yours and every regular's. First place takes the belt.",
		"sentence\tThis edition's race began before your update: no Player of the Series this time.",
		// U9: CHAMPIONS' two honest lines while the roll is empty.
		"sentence\tNo champion yet. The first name goes up on {Date}.",
		"sentence\tThe record books opened {Date}, {Year}.",
		// U11: YOUR <FESTIVAL> before the hero's first event of the edition.
		"sentence\tPlay any {Festival} event for your pin.",
		// G7's second line (the hub banner only, beside RCOP's tagline and never in its place; fest::Inventory lists G7 too).
		"sentence\tThe river runs highest in the fall.",
		// FH-1b. U1: the strip's label and a rival cell without an account.
		"label\tTHE FESTIVAL YEAR",
		"label\tBY INVITATION",
		// The day's rows (§2.8.2): GTD or pool, the entries and the status, the pills (U3's MAIN EVENT, U34's titles); U31 beside a
		// satellite into a Main that isn't joinable yet (§2.2.3).
		"label\t{Gtd} GTD",
		"label\t{Entries} entries",
		"label\t1 entry",
		"label\tIN {Countdown}",
		"label\tLATE {Clock}",
		"label\tMAIN EVENT",
		"label\tMINI MAIN EVENT",
		"label\tMICRO MAIN EVENT",
		"label\tSATELLITE",
		// U35 (FH-4A fix round 1): a championship's step rows, the pill numbers the ladder's own (HubStepPills).
		"label\tSTEP 1",
		"label\tSTEP 2",
		"label\tSTEP 3",
		"label\tFOR A STEP {K} TICKET",
		"label\tFOR A MAIN EVENT SEAT",
		"sentence\tA seat won here waits in your wallet until big fields open.",
		// The overlay ticker (§2.4.4): U26's three forms, and the Main's registrations so far (U3's MAIN EVENT with U4's "registered").
		"sentence\t{Site} added {Overlay} to cover {Event}'s guarantee.",
		"sentence\t{Event} is projected {Amount} short.",
		"sentence\t{Event} is projected {Amount} over.",
		"label\tMAIN EVENT \xC2\xB7 {Entries} registered",
		// The strip's popover (§2.8.6): the host and the window, U16's phase and account lines, U17's buttons.
		"label\t{Site} \xC2\xB7 {FirstDate} \xE2\x80\x93 {LastDate}",
		"sentence\tOpens {Date}.",
		"sentence\tDay {N} of {Days}.",
		"sentence\tCrowned {Date}.",
		"sentence\tBy invitation.",
		"sentence\tYour account is open.",
		"label\tOPEN THE HUB",
		"label\tGO TO {SITE}",
		// FH-2. U2's NIGHT form (GRIT's banner) and GRIT's day strip; U15's GRIT line (the ticker's lead on GRIT's hub, its Short in
		// {FESTIVAL}: "GRIT · NIGHT 2 OF 12 · SHOWS AT 7 PM AND 10 PM").
		"label\tNIGHT {N} OF {Nights}",
		"label\tNIGHT {N}",
		"label\t{FESTIVAL} \xC2\xB7 NIGHT {N} OF {Nights} \xC2\xB7 SHOWS AT {Show1} AND {Show2}",
		// U30: the strip cell's count and the popover's words for the Festival Pass.
		"label\tPASS {N}/{Need}",
		"sentence\tFestival Pass: {N} of {Need} online events.",
		"sentence\tOpens to you after your second rent.",
		// U33: The Summons' seats (§2.8.3), singular at one.
		"label\t{N} SEATS AT THE POOL SO FAR",
		"label\t1 SEAT AT THE POOL SO FAR",
		"label\t{N} SEATS WON",
		"label\t1 SEAT WON",
		// FH-4A. U5: THE RACE's toggles. U7: the YOU row. U8: the ladder's rungs (the race's legend draws its five mark labels).
		"label\tEVERYONE",
		"label\tPEOPLE YOU KNOW",
		"label\tYou \xC2\xB7 {Rank} of {Scorers} in the race \xC2\xB7 {Points} pts",
		"label\tPLAY 1",
		"label\tCASH 1",
		"label\tTOP 25%",
		"label\tTOP 10%",
		"label\tFINALIST",
		"label\tPODIUM",
		"label\tTHE BELT",
		// U10 (singular at one) and its parts as the panel joins them; U12: the pin's three grades; U27: the ladder's line.
		"label\t{Events} events \xC2\xB7 {Cashes} cashes \xC2\xB7 best {Best} of {Field} \xC2\xB7 race {Rank} of {Scorers}",
		"label\t1 event",
		"label\t1 cash",
		"label\tGOLD \xC2\xB7 won an event",
		"label\tSILVER \xC2\xB7 cashed",
		"label\tOUTLINE \xC2\xB7 played",
		"label\t{N} pts to {MARK} \xC2\xB7 right now",
		// U28's ticket line on the panel after the crown; U37: a step ticket's runs left.
		"label\tA seat in the next Main Event",
		"sentence\tGood on {Dates}.",
		// CHAMPIONS' figures (M-4: NetMoney) and the belt's points; the panel's result rows.
		"label\t{Prize} \xC2\xB7 1st of {Entries}",
		"label\t{Points} pts",
		"label\t{Place} of {Entries}",
		// FH-5b. U4: the Main card's defending champion while registered. U13: the broadcast's labels, the guarantee meter's forms and
		// the reel's and the champion card's titles; the Main's states on the flap (§2.8.4, §2.1.4's Closing).
		"label\tDEFENDING CHAMPION \xC2\xB7 {Name} \xC2\xB7 registered",
		"label\tMAIN EVENT DAY",
		"label\tREGISTRATIONS",
		"label\tTHE GUARANTEE",
		"label\tPROJ SHORT \xC2\xB7 {Amount}",
		"label\tPROJ OVER \xC2\xB7 {Amount}",
		"label\tOVERLAY \xC2\xB7 {Amount}",
		"label\tWHO'S IN",
		"label\tLIVE \xC2\xB7 {Left} LEFT OF {Entries}",
		"label\tFINAL TABLE",
		"label\tCHAMPION",
		"label\tREGISTERING",
		"label\tLATE REG",
		"label\tRUNNING",
		"label\t{H}:{M}:{S}",
		// U14: the opening bell's band, the champion card's line and WHO'S IN's Qualifiers count (singular at one); the journey's parts
		// from the person's card (World::ProfileOf: the arrival line, the titles on record).
		"sentence\tThe Main Event is under way.",
		"sentence\t{Name} won {Trophy} and {Prize}. {Journey}",
		"sentence\t{N} regulars won their seat by satellite.",
		"sentence\t1 regular won their seat by satellite.",
		"sentence\tOn {Site} since {Year}.",
		"sentence\tOn the scene since {Year}.",
		"sentence\tFirst title on record.",
		"sentence\t{N} titles on record.",
		"label\tYOU",
		// U38: WHO'S IN's other labels (the three who:fest:* lines print through F-world's FestWhyLine, R-16, R-21), and a roll champion's
		// line (§4.7.3: "slychip · RCOP 2026 champion"; a belt holder's "Player of the Series, {Festival}", no year before "Player": X1).
		"label\tMet {N}x",
		"label\tDefending champion",
		"label\tWon a seat in {Satellite}",
		"label\tTeam pro",
		"label\tStreams on Kast",
		"label\t{Festival} champion",
		"label\tPlayer of the Series, {Festival}",
		// FH-6a. U18: the moments' lines under CHAMPION and PLAYER OF THE SERIES (above): a Main's (U36 from a championship's second
		// edition in this world), another festival event's, the belt's and a title's; U38: a DEV moment's mark beside its numbers.
		"label\t{Festival} Main Event",
		"label\t{Festival} Main Event \xC2\xB7 {Ordinal} edition",
		"label\t{Event}",
		"label\t{Festival} {Year}",
		"label\t{Festival} {Year} \xC2\xB7 {Tier} Main Event",
		"label\t{Ordinal} edition",
		"label\t{Prize}",
		"label\tDEV",
		// U19 and U29: the moment's close, and the lines beneath a Main's card (the shelf line while kHubShelfShips, Q20).
		"sentence\tClick anywhere to continue.",
		"label\tTAKE A BOW",
		"sentence\tThe trophy goes on the shelf at home.",
		"sentence\tThe belt is decided at the crown, later tonight.",
		// U28: the mark card's rank line (its mark is U8's word, its edition U18's line, its ticket line the YOUR panel's).
		"label\t{Rank} of {Scorers} in the race",
		// U20's pin wall half: the Career page's title (the hover joins U12's grade and U10's line after the edition's name).
		"label\tFESTIVAL PINS \xC2\xB7 {Year}",
		"label\t{Festival} \xC2\xB7 {Grade} \xC2\xB7 {Events} events \xC2\xB7 {Cashes} cashes",
	};
	for (std::string& Line : ui::rlfest_detail::HostUiStrings())
	{
		Out.push_back(std::move(Line));
	}
	return Out;
}
} // namespace fest

namespace ui
{
namespace rlfest_detail
{
using rlnet_detail::NetA;
using rlnet_detail::NetMoney;
using rlnet_detail::NetUpper;

/** The client's private state the hub's helpers read (RiverLine.h's friend). */
struct Access
{
	using Page = RiverLine::Page;
	static const Session& SessionOf(const RiverLine& RL) { return RL.S; }
	static double PageAt(const RiverLine& RL) { return RL.PageAt; }
	static double SeriesAt(const RiverLine& RL) { return RL.SeriesAt; }
	static void OpenPage(RiverLine& RL, Page P, double Now) { RL.OpenPage(P, Now); }
	static void SelectEvent(RiverLine& RL, const std::string& Id) { RL.SelectEvent(Id); }
	// FH-1b: the rows' list (the arrow keys walk it, as the lobby's schedule), the client's own register button, a winner's name.
	static void SetListed(RiverLine& RL, std::vector<std::string> Ids) { RL.Listed = std::move(Ids); }
	static void RegisterBlock(RiverLine& RL, const net::EventInstance& E, const net::LiveState& L, const Rect& R, double Now) { RL.RegisterBlock(E, L, R, Now); }
	static std::string PlacingName(const RiverLine& RL, const net::Placing& P) { return RL.PlacingName(P); }
	static double WorldOf(const RiverLine& RL) { return RL.World; }
	// FH-4A: a name as the lobby draws it (its flag, the name opening the card, one badge and one pill; the player's own in neon).
	static void PlayerName(RiverLine& RL, int Npc, float X, float Y, float Size, float MaxW, uint8_t Frame) { RL.PlayerName(Npc, X, Y, Size, MaxW, true, Frame); }
	// FH-5b: a placing's name as the lobby draws it (someone nobody follows: their flag and screen name), and a laptop cue (the opening
	// bell's sting; the session plays it once an id every 50 ms).
	static void PlacingPlayer(RiverLine& RL, const net::Placing& P, float X, float Y, float Size, float MaxW) { RL.PlayerName(P, X, Y, Size, MaxW, true); }
	static void Sound(RiverLine& RL, SoundId Id, float Vol) { RL.S.UiSound(Id, Vol); }
	static void SetPreviewing(RiverLine& RL, bool On) { RL.Previewing = On; }
	// FH-6a: the moment's pointer hold before the app's frame, and the pin wall drawn alone (u4_ui's FestPinsReveal).
	static bool Previewing(const RiverLine& RL) { return RL.Previewing; }
	static Session& SessionMut(RiverLine& RL) { return RL.S; }
	static void Pins(RiverLine& RL, Canvas& Cv, const Rect& R, double Now)
	{
		RL.C = &Cv;
		RL.UI.Begin(Cv, Now);
		RL.World = RL.S.WorldMinutes();
		RL.CareerPins(R, Now);
		RL.UI.Ptr.EndFrame();
	}
};

/** THE FESTIVAL YEAR's ten brands in calendar order (§2.8.2: the cells' ids fsy0..9). */
constexpr fest::Brand HubYearOrder[10] = {fest::Brand::Writ, fest::Brand::RingRush, fest::Brand::Pennant, fest::Brand::Grit, fest::Brand::Tco,
	fest::Brand::Reign, fest::Brand::Slam, fest::Brand::Push, fest::Brand::Rcop, fest::Brand::Feign};

/** One cell of THE FESTIVAL YEAR (§2.8.2, §2.1.4): a brand's on-stage edition, else its next (PLAN_FHUBS Q24). */
struct HubYearCell
{
	const fest::Festival* F = nullptr;       // null: the brand has no edition yet (F-net's rows land package by package)
	const net::SeriesInfo* Series = nullptr; // the edition's series on its host
	fest::Phase Ph = fest::Phase::Off;
	bool OnStage = false;                    // Live, Main day, Closing or the Afterglow
	bool Lit = false;                        // inside its Announced-to-Afterglow window, or underlined
	bool Underline = false;                  // the edition on stage (else the next to open): the 2-unit Accent underline
	bool Account = true;                     // the player holds an account on the host (RiverLine always)
	double CrownedAt = -1.0;                 // the crown minute (world::FestCrownedAt), -1 before it
	// FH-2: the Festival Pass on a cell without an account (HubPassForm): 0 by invitation, 1 after the second rent, 2 counting.
	int PassForm = 0;
	int PassN = 0, PassNeed = 0;
};

/** The Festival Pass's progress on a rival edition without an account (§2.10 items 1 and 6, §2.8.6, U30; FH-2), from GateMet's own
 *  inputs in its order (G/SiteMoney.cpp:512-538): 0 "BY INVITATION" (the site has no Pass, isn't open yet, or the path is closed:
 *  the edition isn't the site's festival in its Announced, Buildup or Live phase, or its first edition, which needs the two rents
 *  alone, so the letter comes at the next daily check); 1 before the second rent ("Opens to you after your second rent."); 2 while
 *  it counts, "PASS {N}/{Need}" with N the paid online events (max(P.OnlineEvents, P.Paid100)), shown at most Need. Events is read
 *  only for 2 (HubYear asks the session's record only then). Pure; u4_ui's FestStripTen calls it. */
int HubPassForm(const net::InviteGate& Gate, bool Open, int RentsPaid, const fest::Festival* PassFest, const fest::Festival& F, int Events, int& N, int& Need)
{
	N = 0;
	Need = 0;
	if (Gate.FestivalEvents < 0 || !Open)
	{
		return 0;
	}
	if (RentsPaid < Gate.MinRentsPaid)
	{
		return 1;
	}
	if (PassFest != &F)
	{
		return 0;
	}
	Need = fest::PassNeeds(F, Gate);
	if (Need <= 0)
	{
		Need = 0;
		return 0;
	}
	N = std::max(0, std::min(Events, Need));
	return 2;
}

/** THE RACE's side state on one laptop and edition (§2.8.2; FH-4A): the race as the hub last read it (re-read when the world's stamp
 *  or the day changes), and the race as last read on the day before PrevDay, the movement column's yesterday. Kept across the hub's
 *  opens (HubUi starts afresh on each) and dropped for another session or world (a load): no movement shows until the hub has seen a
 *  day turn, never a guess. Presentation only. */
struct HubRaceMemo
{
	world::WorldStamp Stamp;                           // Serial, Rev and HeroRev of the read (the clock is not compared)
	bool Valid = false;
	std::vector<std::pair<int, world::FestScore>> All; // the race in rank order (world::FestTop over every scorer)
	std::map<int, int> Ranks;                          // npc -> rank in All, read on Day
	int Day = -1;
	std::map<int, int> Prev;                           // npc -> rank as last read on PrevDay - 1
	int PrevDay = -1;
};
struct HubRaceBook
{
	const Session* Of = nullptr;
	std::map<std::string, HubRaceMemo> ByKey;
};

std::map<const RiverLine*, HubRaceBook>& HubRaceBooks()
{
	static std::map<const RiverLine*, HubRaceBook> FestRaceByLaptop;
	return FestRaceByLaptop;
}

/** Every fact the hub prints (§2.8.3), read once a frame by Gather. */
struct HubFacts
{
	const fest::Festival* F = nullptr;
	const net::SeriesInfo* Series = nullptr; // the edition's series on its host (null: the hub draws nothing)
	fest::Phase Ph = fest::Phase::Off;
	double World = 0.0;
	int Today = 0;
	int DayCount = 0;          // the window's days, LastDay - FirstDay + 1
	double OpensIn = 0.0;      // world minutes to FirstDay 00:00 (the lobby runs one world minute a real minute)
	float DayProgress = 0.0f;  // the world day's fraction, changed once a world minute (FestBanner, the word-mark's Gauge)
	bool HasMain = false;      // the Main's instance: Network::Next(MainId) from the edition's first day
	net::EventInstance Main;
	Chips MainBuyIn = 0, MainGtd = 0, MainAdded = 0;
	int MainStack = 0;
	std::string MainLock;      // the Main's lock line while it isn't joinable (RCOP and Summer Slam until G3, §2.2.3); "" if it is
	int SeatsWon = 0;          // fest::SeatsWon: Main-ticket seats only
	bool RaceOpen = false;     // the race the world's views read is this edition's (world::FestRaceKey)
	int RaceRows = 0;          // the rows world::FestTop holds for it (THE RACE's rows: FH-4A)
	bool RollEmpty = true;     // world::FestRoll holds no edition of the brand (U9)
	bool HasPin = false;       // the hero holds this edition's pin (world::HeroFestPins)
	HubYearCell Year[10];      // THE FESTIVAL YEAR, in HubYearOrder (FH-1b)
	std::vector<int> TitleDays; // the edition's other title days (a championship's Micro and Mini: the day strip's outline stars)
	// FH-4A: the right column's views (§2.5-§2.6), read here and nowhere else.
	const world::World* W = nullptr;        // the world they were read from (the names' lookups); null: none
	bool Crowned = false;                   // world::FestCrownedAt: the race is final (FestLast's ten through the Afterglow)
	const HubRaceMemo* Race = nullptr;      // the race as last read (RaceOpen only)
	int HeroRank = 0, Scorers = 0;          // world::FestStanding(-1): rank 0 while the hero isn't scoring
	double HeroPoints = 0.0;
	std::vector<std::pair<int, int>> Known; // world::FestKnown: (npc, rank), race order
	std::vector<world::FestRollRow> Roll;   // world::FestRoll, newest first
	fest::YourLineView Your;                // fest::YourLine: the YOUR panel's numbers (the sheets' strip reads the same view)
	world::HeroFestPin Pin;                 // the edition's pin (Grade 0: none)
	world::HeroFestMark Mark;               // the hero's kept mark for the edition (world::HeroMarks; None before the crown)
	std::vector<world::HeroFestResult> Results; // world::HeroFestResults, newest first
	// FH-5b: Main Event Day (§2.8.4), read here and nowhere else.
	bool Broadcast = false;                 // Phase::MainDay or Closing with the Main's instance: the hero band and the broadcast
	net::LiveState MainLive;                // Network::Live(Main, World)
	bool HasResult = false;                 // the Main has finished and its final table exists (the reel, the champion card)
	std::vector<net::Placing> FinalTable;   // Network::Result(Main), places 1..9
	::ss::site::show::OverlayView MainOverlay; // the guarantee meter (site::show::OverlayOf)
	double MainEach = 0.0;                  // cents an entry adds to the Main's pool (the rake's prize share)
	struct Who
	{
		int Npc = -1;
		std::string Line;                   // the person's reason line ("" for the rival: PlayerName's own pill says it)
	};
	std::vector<Who> WhoIn;                 // World::FieldStoriesOf(Main): its people in the query's order, at most twelve
	int Qualifiers = 0;                     // the Qualifiers group: registered regulars who won their seat by satellite
	int Defender = -1;                      // FieldStoriesOf's DefendingChampion (U4), -1 none
	std::vector<::ss::site::show::RegPoint> Regs; // site::show::Registrations(Main, 48) while broadcasting before the result
};

/** WHO'S IN's "Won a seat in {Satellite}" (U38): the edition's one satellite into its Main by name, else "a satellite" (the world keeps
 *  a seat's ticket, not the satellite it came from, so with several the line never guesses). */
std::string HubSeatFrom(const net::Network& Net, const fest::Festival& F)
{
	const std::string MainTicket = Net.TicketOf(F.MainId);
	const net::EventTemplate* Only = nullptr;
	int Count = 0;
	for (const std::string& Id : F.Satellites)
	{
		const net::EventTemplate* T = Net.FindTemplate(Id);
		if (T && T->SeatTicket == MainTicket)
		{
			Only = T;
			++Count;
		}
	}
	return Count == 1 && Only ? rlnet_detail::NetShortName(Only->Name) : std::string("a satellite");
}

/** WHO'S IN's line for one person (§2.8.4, §4.7.3, §2.6.7; U38): FestWhyLine's championship lines, U38's labels, a roll champion's
 *  "{Festival} champion" (the person's latest Main on the brand's roll, else "Player of the Series, {Festival}"); "" for the rival. */
std::string HubWhoLine(const world::World& W, const net::Network& Net, const fest::Festival& F, const std::vector<world::FestRollRow>& Roll, const world::StoryPerson& P,
	const std::string& MainId)
{
	switch (P.Why)
	{
	case world::StoryWhy::Met:
	{
		const auto It = W.Bonds().find(P.Npc);
		const int Met = It == W.Bonds().end() ? 1 : std::max<int>(1, std::max(It->second.Encounters, static_cast<int>(It->second.Memories.size())));
		return "Met " + std::to_string(Met) + "x";
	}
	case world::StoryWhy::DefendingChampion: return "Defending champion";
	case world::StoryWhy::DefendingSeries:
	case world::StoryWhy::YearChampion:
	case world::StoryWhy::TitleHolder: return world::FestWhyLine(W, P, MainId);
	case world::StoryWhy::Champion:
		for (const bool Belt : {false, true})
		{
			for (const world::FestRollRow& R : Roll)
			{
				const world::Honor* H = Belt ? R.Crown : R.Main;
				const fest::Festival* Ed = H && H->Npc == P.Npc ? fest::FestivalOf(R.Key) : nullptr;
				if (Ed)
				{
					return Belt ? "Player of the Series, " + Ed->Name : Ed->Name + " champion";
				}
			}
		}
		return std::string();
	case world::StoryWhy::TeamPro: return "Team pro";
	case world::StoryWhy::Streamer: return "Streams on Kast";
	case world::StoryWhy::Qualifier: return "Won a seat in " + HubSeatFrom(Net, F);
	default: break;
	}
	return std::string();
}

/** THE RACE's read (FH-4A): the hero's standing, the people the player knows and the whole race once per world state and day, kept in
 *  the laptop's HubRaceMemo with the movement column's yesterday. */
void HubRaceRead(const RiverLine& RL, const Session& S, const world::World& W, const fest::Festival& F, HubFacts& H)
{
	HubRaceBook& Book = HubRaceBooks()[&RL];
	if (Book.Of != &S)
	{
		Book = HubRaceBook();
		Book.Of = &S;
	}
	HubRaceMemo& M = Book.ByKey[F.SeriesId];
	const world::WorldStamp St = W.SnapshotStamp();
	if (M.Valid && St.Serial != M.Stamp.Serial)
	{
		M = HubRaceMemo();
	}
	const std::pair<int, int> Me = world::FestStanding(W, -1);
	H.HeroRank = Me.first;
	H.Scorers = Me.second;
	if (!M.Valid || St.Rev != M.Stamp.Rev || St.HeroRev != M.Stamp.HeroRev || H.Today != M.Day)
	{
		M.All = world::FestTop(W, 1 << 20);
		std::map<int, int> Ranks;
		for (size_t I = 0; I < M.All.size(); ++I)
		{
			Ranks[M.All[I].first] = static_cast<int>(I) + 1;
		}
		if (M.Day >= 0 && H.Today == M.Day + 1)
		{
			M.Prev = std::move(M.Ranks);
			M.PrevDay = H.Today;
		}
		else if (M.Day >= 0 && H.Today != M.Day)
		{
			M.Prev.clear();
			M.PrevDay = -1;
		}
		M.Ranks = std::move(Ranks);
		M.Day = H.Today;
		M.Stamp = St;
		M.Valid = true;
	}
	H.Race = &M;
	H.RaceRows = static_cast<int>(std::min<size_t>(10, M.All.size()));
	if (H.HeroRank > 0 && H.HeroRank <= static_cast<int>(M.All.size()))
	{
		H.HeroPoints = M.All[static_cast<size_t>(H.HeroRank - 1)].second.Points;
	}
	H.Known = world::FestKnown(W);
}

/** The world minute a day begins. */
double HubMinute(int Day)
{
	return static_cast<double>(Day) * net::MinutesPerDay;
}

/** THE FESTIVAL YEAR's cells (§2.8.2, §2.1.4; FH-1b): for each brand, the edition on stage (Live to the Afterglow), else the next to
 *  open; lit inside the Announced-to-Afterglow window; the on-stage editions underlined, or the next to open when none is. World
 *  reads: PhaseOf's crown minute and FestCrownedAt, both const views. */
void HubYear(const Session& S, double World, const world::World* W, HubFacts& H)
{
	const int Today = net::DayOf(World);
	// A fortnight back holds every tail (Closing, an Afterglow, an uncrowned edition's two days, R-13); a year and a month ahead
	// holds every brand's next edition.
	constexpr int TailDays = 14;
	const std::vector<const fest::Festival*> Near = fest::Festivals(Today - TailDays, Today + 400);
	bool AnyOnStage = false;
	int Events = -1; // FH-2: the paid online events GateMet counts, asked of the session's record once, only when a Pass counts
	for (int I = 0; I < 10; ++I)
	{
		HubYearCell& YC = H.Year[I];
		YC = HubYearCell();
		for (const fest::Festival* F : Near)
		{
			if (F->Which != HubYearOrder[I])
			{
				continue;
			}
			if (F->FirstDay <= Today)
			{
				const fest::Phase Ph = fest::PhaseOf(*F, World, W);
				if (Ph == fest::Phase::Live || Ph == fest::Phase::MainDay || Ph == fest::Phase::Closing || Ph == fest::Phase::Afterglow)
				{
					YC.F = F;
					YC.Ph = Ph;
					YC.OnStage = true;
					break;
				}
				continue;
			}
			if (!YC.F)
			{
				YC.F = F; // Near is in FirstDay order: the brand's first edition still to open
				YC.Ph = fest::PhaseOf(*F, World, W);
			}
		}
		if (!YC.F)
		{
			continue;
		}
		YC.Series = net::Site(YC.F->Site).FindSeries(YC.F->SeriesId);
		YC.Lit = YC.Ph != fest::Phase::Off;
		YC.Account = YC.F->Site == net::SiteId::RiverLine || rlsites_detail::SitesAccount(S, YC.F->Site) != nullptr;
		if (!YC.Account)
		{
			// FH-2 (§2.10): the Pass's progress, from GateMet's own inputs.
			const net::InviteGate& Gate = net::Def(YC.F->Site).Gate;
			const bool Open = World >= net::netsites_detail::NetSitesLaunch(net::Site(YC.F->Site));
			const fest::Festival* PassFest = Gate.FestivalEvents >= 0 && Open ? fest::PassFestival(YC.F->Site, World) : nullptr;
			if (PassFest == YC.F && S.Life.RentsPaid >= Gate.MinRentsPaid && Events < 0)
			{
				const rep::Public P = S.Record();
				Events = std::max(P.OnlineEvents, P.Paid100);
			}
			YC.PassForm = HubPassForm(Gate, Open, S.Life.RentsPaid, PassFest, *YC.F, std::max(0, Events), YC.PassN, YC.PassNeed);
		}
		if (YC.Ph == fest::Phase::Afterglow && W)
		{
			YC.CrownedAt = world::FestCrownedAt(*W, YC.F->SeriesId);
		}
		AnyOnStage = AnyOnStage || YC.OnStage;
	}
	int Next = -1;
	for (int I = 0; I < 10; ++I)
	{
		HubYearCell& YC = H.Year[I];
		if (!YC.F || !YC.Series)
		{
			YC.F = nullptr;
			continue;
		}
		if (AnyOnStage)
		{
			YC.Underline = YC.OnStage;
		}
		else if (Next < 0 || YC.F->FirstDay < H.Year[Next].F->FirstDay)
		{
			Next = I;
		}
	}
	if (Next >= 0)
	{
		H.Year[Next].Underline = true;
	}
	for (HubYearCell& YC : H.Year)
	{
		YC.Lit = YC.Lit || YC.Underline;
	}
}

HubFacts Gather(const RiverLine& RL, const Session& S, const fest::Festival& F, double World)
{
	HubFacts H;
	H.F = &F;
	H.World = World;
	H.Today = net::DayOf(World);
	const net::Network& Net = net::Site(F.Site);
	H.Series = Net.FindSeries(F.SeriesId);
	const world::World* W = Net.Attached();
	H.Ph = fest::PhaseOf(F, World, W);
	H.DayCount = F.LastDay - F.FirstDay + 1;
	H.OpensIn = HubMinute(F.FirstDay) - World;
	H.DayProgress = static_cast<float>(std::floor(World - HubMinute(H.Today)) / net::MinutesPerDay);
	H.HasMain = !F.MainId.empty() && Net.Next(F.MainId, HubMinute(F.FirstDay), H.Main);
	if (H.HasMain)
	{
		const net::EventTemplate& T = Net.TemplateOf(H.Main);
		H.MainBuyIn = T.BuyInCents;
		H.MainGtd = T.GtdCents;
		H.MainAdded = T.AddedCents;
		H.MainStack = T.StartStack;
		if (Net.Joinable(H.Main, &H.MainLock, S.Unlocks()))
		{
			H.MainLock.clear();
		}
	}
	H.SeatsWon = fest::SeatsWon(F, World);
	if (W)
	{
		H.W = W;
		H.RaceOpen = world::FestRaceKey(*W) == F.SeriesId;
		H.Crowned = world::FestCrownedAt(*W, F.SeriesId) >= 0.0;
		for (const world::HeroFestMark& Mk : world::HeroMarks(*W))
		{
			H.Mark = Mk.Key == F.SeriesId ? Mk : H.Mark;
		}
		if (H.RaceOpen)
		{
			HubRaceRead(RL, S, *W, F, H);
			if (H.HeroRank > 0 && H.HeroPoints <= 0.0 && H.Mark.Key == F.SeriesId)
			{
				H.HeroPoints = H.Mark.Points; // after the crown, outside FestLast's ten: the mark's record
			}
		}
		H.Roll = world::FestRoll(*W, F.Which);
		H.RollEmpty = H.Roll.empty();
		for (const world::HeroFestPin& Pin : world::HeroFestPins(*W))
		{
			if (Pin.Key == F.SeriesId)
			{
				H.HasPin = true;
				H.Pin = Pin;
			}
		}
		H.Your = fest::YourLine(F, W);
		if (H.HasPin)
		{
			H.Results = world::HeroFestResults(*W, F.SeriesId);
		}
		// FH-5b: WHO'S IN over the Main's registered field (FW-4's const view, cached per world state; the same read gives U4).
		if (H.HasMain)
		{
			const world::FieldStories& Field = W->FieldStoriesOf(H.Main.Id);
			H.Qualifiers = Field.Groups[static_cast<size_t>(world::StoryGroup::Qualifiers)];
			for (const world::StoryPerson& P : Field.People)
			{
				H.Defender = H.Defender < 0 && P.Why == world::StoryWhy::DefendingChampion ? P.Npc : H.Defender;
				if (H.WhoIn.size() < 12)
				{
					H.WhoIn.push_back({P.Npc, HubWhoLine(*W, Net, F, H.Roll, P, H.Main.Id)});
				}
			}
		}
	}
	// FH-5b: Main Event Day's facts (§2.8.4): the Main's state, its result, the guarantee meter's and the registrations' reads.
	if (H.HasMain)
	{
		H.MainLive = Net.Live(H.Main, World);
		if (H.MainLive.St == net::Status::Finished)
		{
			H.FinalTable = Net.Result(H.Main).FinalTable;
			H.HasResult = !H.FinalTable.empty();
		}
		H.Broadcast = H.Ph == fest::Phase::MainDay || H.Ph == fest::Phase::Closing;
		if (H.Broadcast && H.MainGtd > 0)
		{
			H.MainOverlay = ::ss::site::show::OverlayOf(Net, H.Main, World);
			H.MainEach = static_cast<double>(H.MainBuyIn) * Net.Definition().Rake.PrizeShare(Net.TemplateOf(H.Main));
		}
		if (H.Broadcast && !H.HasResult)
		{
			H.Regs = ::ss::site::show::Registrations(Net, H.Main, World, 48);
		}
	}
	HubYear(S, World, W, H);
	for (size_t K = 1; K < F.TitleIds.size(); ++K)
	{
		net::EventInstance Title;
		if (Net.Next(F.TitleIds[K], HubMinute(F.FirstDay), Title))
		{
			H.TitleDays.push_back(net::DayOf(Title.Start));
		}
	}
	return H;
}

/** One host's regions (§2.8.2, in RiverLine::Width x Height units). */
struct HubFrame
{
	Rect Year, Banner, Main, Days, Rows, Ticker, Tabs, Body;
	float CrestX = 0.0f, CrestY = 0.0f; // the banner crest's centre (150 across)
	float WordX = 0.0f;                 // the word-mark's, the title's, the tagline's and the stats' left edge
	float StatGap = 200.0f;             // the four stats' pitch
	bool HasTicker = true;              // Ignite's hub has none (L7)
	float YearX = 260.0f;               // THE FESTIVAL YEAR's first cell (v1.6: ten cells of 128 from x 260; RunItTwice 116 from x 368)
	float YearPitch = 128.0f;
	float YearText = 92.0f;             // a cell's text column: a Short gets 92 units (80 on RunItTwice)
	// FH-2, Ignite (L7, screen 12): the rows' foot fades from FadeY to the rows' end, and a scroll bar stands at BarX while the day
	// holds more rows than show whole; 0 for none.
	float FadeY = 0.0f;
	float BarX = 0.0f;
	// FH-4A (§2.8.2; Ignite's under R-33, F-hubs' Q13): THE RACE's rows (RaceRows at 40 from RaceY), the YOU row's top and the legend's
	// baseline; YOUR <FESTIVAL>'s pin and result rows.
	float RaceY = 452.0f;
	int RaceRows = 10;
	float YouY = 856.0f;
	float LegendY = 914.0f;
	float PinSize = 96.0f;
	int YourResults = 6;
	// FH-5b (§2.8.4; Ignite's WHO'S IN to y 870 under R-33, F-hubs' Q13: eight people in two columns at 48): REGISTRATIONS (the reel's
	// place) and WHO'S IN, where the day strip, the rows and the ticker stand on other days.
	Rect Regs, Who;
};

const HubFrame& HubFrameOf(net::SiteId Site)
{
	static const HubFrame HubFrameRiverLine = [] {
		HubFrame Fr;
		Fr.Year = {24.0f, 84.0f, 1552.0f, 40.0f};
		Fr.Banner = {24.0f, 132.0f, 1552.0f, 204.0f};
		Fr.Main = {1166.0f, 152.0f, 386.0f, 164.0f};
		Fr.Days = {24.0f, 348.0f, 1000.0f, 52.0f};
		Fr.Rows = {24.0f, 408.0f, 1000.0f, 504.0f};
		Fr.Ticker = {24.0f, 920.0f, 1000.0f, 30.0f};
		Fr.Tabs = {1044.0f, 348.0f, 532.0f, 36.0f};
		Fr.Body = {1044.0f, 392.0f, 532.0f, 558.0f};
		Fr.CrestX = 114.0f;
		Fr.CrestY = 234.0f;
		Fr.WordX = 210.0f;
		Fr.Regs = {24.0f, 348.0f, 1000.0f, 250.0f};
		Fr.Who = {24.0f, 610.0f, 1000.0f, 340.0f};
		return Fr;
	}();
	// Ignite (L7): RiverLine's rects but the rows' (cut at y 870, above the dock) and the body's (to y 870), and no overlay ticker.
	static const HubFrame HubFrameIgnite = [] {
		HubFrame Fr = HubFrameRiverLine;
		Fr.Rows = {24.0f, 408.0f, 1000.0f, 462.0f};
		Fr.Ticker = {24.0f, 920.0f, 0.0f, 0.0f};
		Fr.Body = {1044.0f, 392.0f, 532.0f, 478.0f};
		Fr.HasTicker = false;
		Fr.FadeY = 826.0f;
		Fr.BarX = 1014.0f;
		// R-33 (Q13): the race's top eight with the YOU row at y 776 and the legend at y 834; the pin at 72 and four result rows.
		Fr.RaceRows = 8;
		Fr.YouY = 776.0f;
		Fr.LegendY = 834.0f;
		Fr.PinSize = 72.0f;
		Fr.YourResults = 4;
		Fr.Who = {24.0f, 610.0f, 1000.0f, 260.0f};
		return Fr;
	}();
	// RunItTwice: §2.8.2's bracketed rects, right of the rail; the stats a little closer, so the fourth clears the Main card at x 1170.
	static const HubFrame HubFrameRunItTwice = [] {
		HubFrame Fr;
		Fr.Year = {232.0f, 96.0f, 1344.0f, 40.0f};
		Fr.Banner = {232.0f, 144.0f, 1344.0f, 196.0f};
		Fr.Main = {1170.0f, 160.0f, 390.0f, 164.0f};
		Fr.Days = {232.0f, 352.0f, 860.0f, 52.0f};
		Fr.Rows = {232.0f, 412.0f, 860.0f, 504.0f};
		Fr.Ticker = {232.0f, 924.0f, 860.0f, 26.0f};
		Fr.Tabs = {1108.0f, 352.0f, 468.0f, 36.0f};
		Fr.Body = {1108.0f, 396.0f, 468.0f, 554.0f};
		Fr.CrestX = 322.0f;
		Fr.CrestY = 242.0f;
		Fr.WordX = 418.0f;
		Fr.StatGap = 180.0f;
		Fr.YearX = 368.0f;
		Fr.YearPitch = 116.0f;
		Fr.YearText = 80.0f;
		// §2.8.2's [852]: the YOU row under the tenth row, the legend beneath it.
		Fr.YouY = 852.0f;
		Fr.LegendY = 910.0f;
		// §2.8.4's rects moved to the bracketed frame: the rows' column, ending where the frame does (y 950).
		Fr.Regs = {232.0f, 352.0f, 860.0f, 250.0f};
		Fr.Who = {232.0f, 614.0f, 860.0f, 336.0f};
		return Fr;
	}();
	switch (Site)
	{
	case net::SiteId::Ignite: return HubFrameIgnite;
	case net::SiteId::RunItTwice: return HubFrameRunItTwice;
	default: break;
	}
	return HubFrameRiverLine;
}

/** The hub's side state on one laptop (the IgniteStates precedent): what the player picked and when the hub opened. */
struct HubUi
{
	const Session* Of = nullptr;
	std::string Key;          // the edition shown
	double PageAt = 0.0;      // the client's page and series clocks when it opened: a new open (or a new laptop's) starts afresh
	double SeriesAt = 0.0;
	double OpenedAt = 0.0;    // real seconds: the banner's and the word-mark's motions run from here
	int Tab = -1;             // the tab the player picked, -1 the phase's (HubAutoTab)
	// FH-1b.
	int Day = -1;             // the day strip's pick (-1: today, held inside the window)
	int RowFirst = 0;         // the first row in view (the wheel scrolls the day's rows)
	bool Drew = false;        // the hub drew in this frame (FestivalOverlays draws the popover only over it, then clears it)
	struct Pop
	{
		bool Open = false;
		bool Fresh = false;   // opened in this frame: drawn, but it takes no input until the next (FestOneFrameClicks)
		int Cell = -1;        // fsy<Cell>
		Rect R;               // {cell.X, 128, 360, 168}, clamped inside the hub's content rect
		HubYearCell C;        // the cell's facts as Gather read them (FestivalOverlays reads no world)
		int Today = 0;
	} Popover;
	// The day's rows, rebuilt once a world minute or when the edition or the day changes.
	std::vector<std::string> Ladder; // the edition's satellites and the ladder up to them (built once an edition)
	std::map<std::string, std::pair<int, int>> Steps; // FH-4A fix round 1, U35: a step row's template -> {its rung, the rung its seat is (0: the Main)}
	bool LadderBuilt = false;
	long long RowsMinute = -1;
	int RowsDay = -9999;
	std::vector<net::EventInstance> Rows;
	std::vector<net::EventInstance> Late; // FH-2: beside each row, a GRIT show's second run that night (Id "" for none)
	std::map<std::string, int> RunPick;   // FH-2: the run the player picked on a GRIT row (its first run's id -> 0 or 1)
	// The overlay ticker's run, rebuilt once a world minute.
	long long TickerMinute = -1;
	std::string Ticker;
	// FH-4A: THE RACE's toggle (0 EVERYONE, 1 PEOPLE YOU KNOW) and the movement marks its last frame drew (HubRaceMoves).
	int RaceView = 0;
	int MovesShown = 0;
	// FH-5b: the hero band's flap row as last drawn (each cell's character and when it last changed; the first draw waves from the
	// hub's open, 50 ms a cell).
	std::string FlapText;
	std::vector<double> FlapAt;
	// FH-6a: this open's sting looked at (its first live frame), and when it played (the bell shares it in that frame).
	bool StingChecked = false;
	double StingAt = -100.0;
};

/** FH-5b: what Main Event Day keeps per laptop across hub opens (the HubRaceBooks precedent): the opening bell rung for one Main (and
 *  when), the reel's start per Main (the laptop's first draw after the result: it plays once), the champion's journey line. */
struct HubCast
{
	std::string BellFor;
	double BellAt = -100.0;
	std::map<std::string, double> ReelAt;
	std::string JourneyFor;
	std::string Journey;
};

std::map<const RiverLine*, HubCast>& HubCasts()
{
	static std::map<const RiverLine*, HubCast> FestCastByLaptop;
	return FestCastByLaptop;
}

std::map<const RiverLine*, HubUi>& HubStates()
{
	static std::map<const RiverLine*, HubUi> FestHubByLaptop;
	return FestHubByLaptop;
}

/** FH-6a: a laptop's festival cues and its moment (UI side, never saved; a new session starts afresh): the world day each festival's
 *  hub last stung on (§2.7.6), the open festival tables the registration sting watches, the moment on screen (its first live frame,
 *  its cue, the pointer FestMomentHold took) and the pin wall's year. */
struct HubCues
{
	const Session* Of = nullptr;
	double LastWorld = -1.0e18;           // the session's world minute when last looked
	std::map<std::string, int> StingDay; // series id -> world day
	int Tables = -1;                      // the open tables when last looked (-1: not yet)
	std::vector<std::string> Seats;       // their festival events' ids
	std::string MomentId;                 // "<Kind>:<Key>[:dev]", "" with none on screen
	double MomentAt = -1.0;               // real seconds of its first live frame (-1: not drawn live yet)
	bool Sung = false;                    // its cue played
	bool Held = false;                    // FestMomentHold took the pointer in this frame
	rlsites_detail::SitesBlocked Saved;   // what it took
	int PinsYear = 0;                     // the pin wall's year (0: the newest holding a pin)
};

HubCues& HubCuesOf(const RiverLine& RL)
{
	static std::map<const RiverLine*, HubCues> FestCuesByLaptop;
	HubCues& Q = FestCuesByLaptop[&RL];
	const Session* Of = &Access::SessionOf(RL);
	const double World = Access::SessionOf(RL).WorldMinutes();
	if (Q.Of != Of || World < Q.LastWorld)
	{
		// Another session, or the clock went back (a load, a new career, a laptop at a freed address): this laptop starts afresh.
		Q = HubCues();
		Q.Of = Of;
	}
	Q.LastWorld = World;
	return Q;
}

/** The host's sting (§2.7.6): the look's (F-look's FestRiver, FestIgnite or FestRit), else the host site's own; never a click. */
SoundId HubStingOf(const festart::FestivalLook& L, net::SiteId Site)
{
	if (L.Sting != SoundId::Click)
	{
		return L.Sting;
	}
	return Site == net::SiteId::Ignite ? SoundId::FestIgnite : Site == net::SiteId::RunItTwice ? SoundId::FestRit : SoundId::FestRiver;
}

HubUi& HubOf(const RiverLine& RL, const fest::Festival& F, double Now)
{
	HubUi& U = HubStates()[&RL];
	const Session* Of = &Access::SessionOf(RL);
	const double PageAt = Access::PageAt(RL);
	const double SeriesAt = Access::SeriesAt(RL);
	if (U.Of != Of || U.Key != F.SeriesId || U.PageAt != PageAt || U.SeriesAt != SeriesAt)
	{
		U = HubUi();
		U.Of = Of;
		U.Key = F.SeriesId;
		U.PageAt = PageAt;
		U.SeriesAt = SeriesAt;
		U.OpenedAt = Now;
	}
	return U;
}

/** The tab the hub opens on (§2.8.2): THE RACE while the edition is on stage (Live, Main day, Closing), CHAMPIONS during Announced,
 *  Buildup and the Afterglow, YOUR <FESTIVAL> once the hero holds the edition's pin, but never past THE RACE while the Main is locked
 *  and the edition is on stage (§2.2.3: the hub opens on THE RACE, never on a locked Main). */
int HubAutoTab(const HubFacts& H)
{
	const bool OnStage = H.Ph == fest::Phase::Live || H.Ph == fest::Phase::MainDay || H.Ph == fest::Phase::Closing;
	if (H.HasPin && !(OnStage && !H.MainLock.empty()))
	{
		return 2;
	}
	return OnStage ? 0 : 1;
}

/** U2's countdown value, "12D 07H 41M" (world minutes; the lobby's world minute is a real minute). */
std::string HubCountdown(double Minutes)
{
	const long long M = std::max(0LL, static_cast<long long>(std::floor(Minutes)));
	return std::to_string(M / 1440) + "D " + rlnet_detail::NetPad2((M / 60) % 24) + "H " + rlnet_detail::NetPad2(M % 60) + "M";
}

/** The banner's fourth stat (§2.8.2, U2): the countdown over OPENS IN before the window, then DAY {N} OF {Days} (NIGHT {N} OF
 *  {Nights} on GRIT, FH-2), MAIN EVENT TODAY and COMPLETE over PROGRESS. */
std::pair<std::string, std::string> HubStateStat(const HubFacts& H)
{
	if (H.OpensIn > 0.0)
	{
		return {HubCountdown(H.OpensIn), "OPENS IN"};
	}
	if (H.Ph == fest::Phase::MainDay)
	{
		return {"MAIN EVENT TODAY", "PROGRESS"};
	}
	if (H.Today > H.F->LastDay)
	{
		return {"COMPLETE", "PROGRESS"};
	}
	// FH-2: GRIT counts nights (U2's NIGHT form).
	return {std::string(H.F->Which == fest::Brand::Grit ? "NIGHT " : "DAY ") + std::to_string(H.Today - H.F->FirstDay + 1) + " OF " + std::to_string(H.DayCount), "PROGRESS"};
}

/** One frame of the hub: the client, its canvas, the facts and the host's frame. */
struct HubCtx
{
	RiverLine& RL;
	Canvas& C;
	Ui& UI;
	const HubFacts& H;
	const HubFrame& Fr;
	const festart::FestivalLook& Look;
	double Now = 0.0;
	double SinceOpen = 0.0; // real seconds since the hub opened
	bool Still = false;     // the stream preview's settled frame (no motion, no input)
};

/** The banner (§2.8.2): the look's gradient and motif (FestBanner), today's crest at 150, the word-mark (46), the full title, the
 *  tagline and RCOP's second line, and the four stats from WordX, StatGap apart, each a value over its label. */
void HubBanner(HubCtx& X)
{
	const HubFacts& H = X.H;
	const Rect& B = X.Fr.Banner;
	// A ground under the look (FestBanner paints the Bg2 -> Felt gradient over it once F-look's body lands, FL-7).
	X.C.FillRoundRect(B, 18.0f, Paint::Linear({B.X, 0.0f}, {B.X + B.W, 0.0f}, rlsites_detail::SitesTone(0x0e1726), rlsites_detail::SitesTone(0x111c2e)));
	festart::FestBanner(X.C, X.Look, B, X.SinceOpen, H.DayProgress, H.Ph == fest::Phase::MainDay, X.Still);
	X.C.StrokeRoundRect(B, 18.0f, NetA(pal::Line, 0.9f), 1.0f);
	eventart::SeriesCrest(X.C, *H.Series, X.Fr.CrestX, X.Fr.CrestY, 150.0f, X.Still ? 0.0 : X.Now);
	// PC-10: the overload with DayProgress and Still (an eight-argument call ending in a float would bind the old one).
	festart::FestWordMark(X.C, X.Look, H.F->Name, X.Fr.WordX, 196.0f, 46.0f, X.SinceOpen, H.DayProgress, X.Still);
	if (X.Look.Which == fest::Brand::None)
	{
		// No look yet (F-look's table, FL-1): the name in the site's title face, where the word-mark stands.
		X.UI.Text(H.F->Name, X.Fr.WordX, 196.0f, Ts(46.0f, 900, pal::Ink));
	}
	const float TextW = X.Fr.Main.X - X.Fr.WordX - 24.0f;
	float TagY = 228.0f;
	if (!H.Series->Title.empty())
	{
		X.UI.Text(NetUpper(H.Series->Title), X.Fr.WordX, 214.0f, Ts(13.0f, 700, pal::Accent2, Align::Left, Baseline::Alphabetic, false, TextW));
		TagY = 236.0f;
	}
	X.UI.Text(H.Series->Tagline, X.Fr.WordX, TagY, Ts(16.0f, 500, rlsites_detail::SitesTone(0xc3cedf), Align::Left, Baseline::Alphabetic, false, TextW));
	if (H.F->Which == fest::Brand::Rcop)
	{
		X.UI.Text("The river runs highest in the fall.", X.Fr.WordX, 246.0f, Ts(13.0f, 500, pal::Accent2, Align::Left, Baseline::Alphabetic, false, TextW));
	}
	const std::pair<std::string, std::string> State = HubStateStat(H);
	const std::pair<std::string, std::string> Stats[4] = {
		{NetMoney(H.Series->GtdCents), "GUARANTEED"},
		{std::to_string(H.Series->Events), "EVENTS"},
		{net::DateLabel(H.F->FirstDay) + " \xE2\x80\x93 " + net::DateLabel(H.F->LastDay), "DATES"},
		State,
	};
	for (int I = 0; I < 4; ++I)
	{
		const float Sx = X.Fr.WordX + X.Fr.StatGap * static_cast<float>(I);
		const float MaxW = I == 3 ? X.Fr.Main.X - Sx - 16.0f : X.Fr.StatGap - 12.0f;
		X.UI.Text(Stats[I].first, Sx, 292.0f, Ts(24.0f, 900, I == 0 ? pal::Gold : pal::Ink, Align::Left, Baseline::Alphabetic, false, MaxW));
		X.UI.Text(Stats[I].second, Sx, 314.0f, Ts(11.0f, 800, pal::Muted));
	}
}

void HubName(HubCtx& X, int Npc, const std::string& Kept, float Ax, float Nx, float Base, float Size, float MaxW, float Ring);

/** The guarantee meter (§2.8.4, U13; FH-5b) in R inside the Main card: THE GUARANTEE, a track with the pool so far (the registered
 *  entries' prize share) against a gold tick at the guarantee and a hollow mark at the projection, and at the right PROJ SHORT or
 *  PROJ OVER while registering (site::show::OverlayOf: the guarantee against the pool), OVERLAY once registration closed short. A Main
 *  with no guarantee draws none. */
void HubMeter(HubCtx& X, const Rect& R)
{
	const HubFacts& H = X.H;
	if (H.MainGtd <= 0)
	{
		return;
	}
	const ::ss::site::show::OverlayView& V = H.MainOverlay;
	const bool Closed = V.Actual || H.MainLive.St >= net::Status::Running;
	const int Entries = Closed ? H.MainLive.Entries : V.Registered;
	const double Pool = static_cast<double>(Entries) * H.MainEach;
	const double Projected = Closed ? Pool : V.Entries * H.MainEach;
	const double Gtd = static_cast<double>(H.MainGtd);
	const double Scale = std::max({Gtd, Pool, Projected}) * 1.08;
	X.UI.Text("THE GUARANTEE", R.X, R.Y + 10.0f, Ts(10.0f, 800, pal::Muted));
	std::string Status;
	Color Col = pal::Orange;
	if (V.Shown && V.Actual && V.Cents > 0)
	{
		Status = "OVERLAY \xC2\xB7 " + NetMoney(V.Cents);
	}
	else if (V.Shown && !V.Actual && V.Cents > 0)
	{
		Status = "PROJ SHORT \xC2\xB7 " + NetMoney(V.Cents);
	}
	else if (V.Shown && !V.Actual && Projected > Gtd)
	{
		Status = "PROJ OVER \xC2\xB7 " + NetMoney(static_cast<Chips>(std::llround(Projected - Gtd)));
		Col = pal::Green;
	}
	if (!Status.empty())
	{
		X.UI.Text(Status, R.X + R.W, R.Y + 10.0f, Ts(11.0f, 800, Col, Align::Right));
	}
	const Rect Track{R.X, R.Y + 20.0f, R.W, 10.0f};
	X.C.FillRoundRect(Track, 5.0f, rlsites_detail::SitesTone(0x16233a));
	const float Fill = static_cast<float>(std::min(1.0, Pool / Scale));
	if (Fill > 0.0f)
	{
		X.C.FillRoundRect({Track.X, Track.Y, std::max(10.0f, Track.W * Fill), Track.H}, 5.0f, Pool >= Gtd ? pal::Green : pal::Accent);
	}
	if (!Closed && Projected > Pool)
	{
		const float Px = Track.X + Track.W * static_cast<float>(std::min(1.0, Projected / Scale));
		X.C.StrokeEllipse(Px, Track.Y + Track.H / 2.0f, 4.0f, 4.0f, pal::Ink, 1.5f);
	}
	const float Gx = Track.X + Track.W * static_cast<float>(Gtd / Scale);
	X.C.FillRect({Gx - 1.0f, Track.Y - 4.0f, 2.0f, Track.H + 8.0f}, pal::Gold);
	X.UI.Text(NetMoney(static_cast<Chips>(std::llround(Pool))), R.X, R.Y + 46.0f, Ts(11.0f, 700, pal::Ink, Align::Left, Baseline::Alphabetic, true));
	X.UI.Text(NetMoney(H.MainGtd), Gx, R.Y + 46.0f, Ts(11.0f, 700, pal::Gold, Gx > R.X + R.W - 60.0f ? Align::Right : Align::Center, Baseline::Alphabetic, true));
}

/** The split-flap row (§2.8.4; motion::Flap): Text in 44 x 64 cells from (X0, Y0), 4 apart. A cell shows its character once settled (0.25
 *  s after it last changed; the hub's first draw waves in from the page's or the series' open, 50 ms a cell), and before that a glyph that turns with real
 *  time (a closed form of the clock, no random number). A still (the preview) draws every cell settled and keeps no memo. */
void HubFlap(HubCtx& X, HubUi& U, const std::string& Text, float X0, float Y0)
{
	static const char HubFlapTurn[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	if (!X.Still)
	{
		const bool Wave = U.FlapText.empty();
		const double Opened = std::max(Access::PageAt(X.RL), Access::SeriesAt(X.RL)); // the page's or the series' open, not the first draw
		U.FlapAt.resize(Text.size(), X.Now);
		for (size_t I = 0; I < Text.size(); ++I)
		{
			if (Wave)
			{
				U.FlapAt[I] = std::min(X.Now, Opened) + 0.05 * static_cast<double>(I);
			}
			else if (I >= U.FlapText.size() || U.FlapText[I] != Text[I])
			{
				U.FlapAt[I] = X.Now;
			}
		}
		U.FlapText = Text;
	}
	for (size_t I = 0; I < Text.size(); ++I)
	{
		const Rect Cell{X0 + 48.0f * static_cast<float>(I), Y0, 44.0f, 64.0f};
		X.C.FillRoundRect(Cell, 6.0f, Paint::Linear({0.0f, Cell.Y}, {0.0f, Cell.Y + Cell.H}, rlsites_detail::SitesTone(0x1b2638), rlsites_detail::SitesTone(0x0c121d)));
		X.C.FillRect({Cell.X, Cell.Y + Cell.H / 2.0f - 0.5f, Cell.W, 1.0f}, Rgba(0, 0, 0, 0.55f));
		const bool Settled = X.Still || I >= U.FlapAt.size() || motion::Flap(X.Now, U.FlapAt[I], 1) > 0;
		char Ch = Text[I];
		if (!Settled)
		{
			const double Turned = (X.Now - U.FlapAt[I]) * 24.0 + 7.0 * static_cast<double>(I);
			Ch = HubFlapTurn[static_cast<size_t>(std::max(0.0, std::floor(Turned))) % (sizeof(HubFlapTurn) - 1)];
		}
		if (Ch != ' ')
		{
			X.UI.Text(std::string(1, Ch), Cell.X + Cell.W / 2.0f, Cell.Y + Cell.H / 2.0f + 1.0f, Ts(38.0f, 900, Settled ? pal::Ink : pal::Muted, Align::Center, Baseline::Middle));
		}
	}
}

/** The Main's state on the flap (§2.8.4, §2.1.4's Closing): a countdown to the start ("15:00:00", world seconds: the lobby's world
 *  second is a real second) while it hasn't started, then REGISTERING, LATE REG, RUNNING, FINAL TABLE or COMPLETE. */
std::string HubFlapText(const HubFacts& H)
{
	const net::LiveState& L = H.MainLive;
	if ((L.St == net::Status::Announced || L.St == net::Status::Registering) && H.World < H.Main.Start)
	{
		const long long S = std::max(0LL, static_cast<long long>(std::ceil((H.Main.Start - H.World) * 60.0 - 1.0e-6)));
		return rlnet_detail::NetPad2(S / 3600) + ":" + rlnet_detail::NetPad2((S / 60) % 60) + ":" + rlnet_detail::NetPad2(S % 60);
	}
	switch (L.St)
	{
	case net::Status::Announced:
	case net::Status::Registering: return "REGISTERING";
	case net::Status::LateReg: return "LATE REG";
	case net::Status::Running: return "RUNNING";
	case net::Status::FinalTable: return "FINAL TABLE";
	default: break;
	}
	return "COMPLETE";
}

/** The hero band (§2.8.4; FH-5b) in the banner's rect on Main Event Day and through Closing: the look's banner with its Main-day light,
 *  today's crest at 150, the word-mark at 34, MAIN EVENT DAY, the flap row, and under it "LIVE · {Left} LEFT OF {Entries}" while the
 *  Main runs (REGISTERING's word before its start); the opening bell's band over the flap for 3 s. */
void HubHeroBand(HubCtx& X, HubUi& U)
{
	const HubFacts& H = X.H;
	const Rect& B = X.Fr.Banner;
	X.C.FillRoundRect(B, 18.0f, Paint::Linear({B.X, 0.0f}, {B.X + B.W, 0.0f}, rlsites_detail::SitesTone(0x0e1726), rlsites_detail::SitesTone(0x111c2e)));
	festart::FestBanner(X.C, X.Look, B, X.SinceOpen, H.DayProgress, true, X.Still);
	X.C.StrokeRoundRect(B, 18.0f, NetA(pal::Line, 0.9f), 1.0f);
	eventart::SeriesCrest(X.C, *H.Series, X.Fr.CrestX, X.Fr.CrestY, 150.0f, X.Still ? 0.0 : X.Now);
	// PC-10: the overload with DayProgress and Still.
	festart::FestWordMark(X.C, X.Look, H.F->Name, X.Fr.WordX, B.Y + 48.0f, 34.0f, X.SinceOpen, H.DayProgress, X.Still);
	if (X.Look.Which == fest::Brand::None)
	{
		X.UI.Text(H.F->Name, X.Fr.WordX, B.Y + 48.0f, Ts(34.0f, 900, pal::Ink));
	}
	X.UI.Text("MAIN EVENT DAY", X.Fr.WordX, B.Y + 72.0f, Ts(12.0f, 800, pal::Accent));
	const float FlapY = B.Y + 84.0f;
	HubFlap(X, U, HubFlapText(H), X.Fr.WordX, FlapY);
	const net::Status St = H.MainLive.St;
	if (St == net::Status::LateReg || St == net::Status::Running || St == net::Status::FinalTable)
	{
		X.UI.Text("LIVE \xC2\xB7 " + Grouped(H.MainLive.Left) + " LEFT OF " + Grouped(H.MainLive.Entries), X.Fr.WordX, B.Y + 176.0f,
			Ts(18.0f, 800, pal::Ink, Align::Left, Baseline::Alphabetic, false, X.Fr.Main.X - X.Fr.WordX - 24.0f));
		X.C.FillCircle(X.Fr.WordX - 12.0f, B.Y + 170.0f, 4.0f, pal::Red);
	}
	else if (St == net::Status::Announced || St == net::Status::Registering)
	{
		X.UI.Text("REGISTERING", X.Fr.WordX, B.Y + 176.0f, Ts(14.0f, 800, pal::Muted));
	}
	// The opening bell (§2.8.4): once per laptop and Main, when the hub draws within three world minutes of the start (never in a still).
	HubCast& Cast = HubCasts()[&X.RL];
	if (!X.Still && H.World >= H.Main.Start && H.World - H.Main.Start < 3.0 && Cast.BellFor != H.Main.Id)
	{
		Cast.BellFor = H.Main.Id;
		Cast.BellAt = X.Now;
		if (U.StingAt != X.Now)
		{
			Access::Sound(X.RL, HubStingOf(X.Look, H.F->Site), 0.5f); // FH-6a: the host's sting, shared with the open's in one frame
		}
	}
	if (!X.Still && Cast.BellFor == H.Main.Id && X.Now - Cast.BellAt < 3.0)
	{
		const Rect Band{X.Fr.WordX - 14.0f, FlapY - 6.0f, X.Fr.Main.X - X.Fr.WordX - 10.0f, 76.0f};
		X.C.FillRoundRect(Band, 10.0f, pal::Accent);
		X.UI.Text("The Main Event is under way.", Band.X + Band.W / 2.0f, Band.Y + Band.H / 2.0f + 1.0f,
			Ts(26.0f, 800, rlsites_detail::SitesLabelInk(Hex(0x0b1220), pal::Accent, pal::Accent), Align::Center, Baseline::Middle, false, Band.W - 32.0f));
	}
}

/** The champion card (§2.8.4; FH-5b) in the Main card's rect once the Main's result exists, through the Afterglow: CHAMPION, the trophy
 *  (FestTrophy at 64), the name (PlayerName with its flag; the player's "YOU"), the prize (NetMoney), U14's "{Name} won {Trophy} and
 *  {Prize}." and the journey from the person's card (World::ProfileOf: "On {Site} since {Year}." or "On the scene since {Year}.", then
 *  "First title on record." or "{N} titles on record."), kept per laptop for the Main. */
void HubChampCard(HubCtx& X)
{
	const HubFacts& H = X.H;
	const Rect& M = X.Fr.Main;
	const net::Placing& P = H.FinalTable.front();
	X.UI.RRect(M, 14.0f, Rgba(4, 8, 16, 0.6f), pal::Gold);
	X.UI.Text("CHAMPION", M.X + 18.0f, M.Y + 26.0f, Ts(11.0f, 800, pal::Gold));
	festart::FestTrophy(X.C, H.F->Which, M.X + 58.0f, M.Y + 82.0f, 64.0f, X.Still ? 0.0 : X.Now, false);
	const bool Hero = P.Player == -1;
	if (Hero)
	{
		X.UI.Text("YOU", M.X + 106.0f, M.Y + 58.0f, Ts(20.0f, 900, pal::Ink));
	}
	else
	{
		Access::PlacingPlayer(X.RL, P, M.X + 106.0f, M.Y + 58.0f, 18.0f, M.W - 124.0f);
	}
	X.UI.Text(NetMoney(P.Prize), M.X + 106.0f, M.Y + 90.0f, Ts(22.0f, 900, pal::Gold, Align::Left, Baseline::Alphabetic, true, M.W - 124.0f));
	HubCast& Cast = HubCasts()[&X.RL];
	if (Cast.JourneyFor != H.Main.Id + "/" + std::to_string(P.Player))
	{
		std::string Journey;
		const world::Npc* N = !Hero && H.W && P.Player >= 0 ? H.W->Get(P.Player) : nullptr;
		if (N)
		{
			const world::Profile Pr = H.W->ProfileOf(P.Player);
			const std::string Site = !N->Path.empty() && N->Path.front().Kind == world::StepKind::Joined && !N->Path.front().What.empty() ? N->Path.front().What : std::string("RiverLine");
			Journey = Pr.Arrived >= 0 ? "On " + Site + " since " + std::to_string(world::YearOf(Pr.Arrived)) + "." : "On the scene since " + std::to_string(Pr.Since) + ".";
			Journey += Pr.Titles == 1 ? std::string(" First title on record.") : Pr.Titles > 1 ? " " + std::to_string(Pr.Titles) + " titles on record." : std::string();
		}
		Cast.JourneyFor = H.Main.Id + "/" + std::to_string(P.Player);
		Cast.Journey = Journey;
	}
	const std::string Name = Hero ? std::string("You") : Access::PlacingName(X.RL, P);
	X.UI.Text(Name + " won " + fest::TrophyName(H.F->Which) + " and " + NetMoney(P.Prize) + ".", M.X + 18.0f, M.Y + 130.0f,
		Ts(12.0f, 600, pal::Ink, Align::Left, Baseline::Alphabetic, false, M.W - 36.0f));
	if (!Cast.Journey.empty())
	{
		X.UI.Text(Cast.Journey, M.X + 18.0f, M.Y + 150.0f, Ts(12.0f, 500, pal::Muted, Align::Left, Baseline::Alphabetic, false, M.W - 36.0f));
	}
}

/** REGISTRATIONS (§2.8.4; FH-5b): site::show::Registrations(Main, 48) as ChartArt's LineChart in the skin's theme, the Main's count so
 *  far at the right ("MAIN EVENT · {Entries} registered"), times under the line. */
void HubRegsChart(HubCtx& X)
{
	const HubFacts& H = X.H;
	const Rect& R = X.Fr.Regs;
	X.UI.RRect(R, 14.0f, rlsites_detail::SitesTone(0x0e1726), NetA(pal::Line, 0.9f));
	X.UI.Text("REGISTRATIONS", R.X + 20.0f, R.Y + 28.0f, Ts(12.0f, 800, pal::Accent));
	if (!H.Regs.empty())
	{
		X.UI.Text("MAIN EVENT \xC2\xB7 " + Grouped(H.Regs.back().Entries) + " registered", R.X + R.W - 20.0f, R.Y + 28.0f, Ts(13.0f, 700, pal::Ink, Align::Right));
	}
	chart::Series Line;
	chart::LineOpts O;
	for (size_t I = 0; I < H.Regs.size(); ++I)
	{
		Line.Y.push_back(static_cast<double>(H.Regs[I].Entries));
		O.X.Labels.push_back(I % 8 == 0 || I + 1 == H.Regs.size() ? net::TimeLabel(H.Regs[I].At) : std::string());
	}
	Line.Col = pal::Accent;
	Line.Width = 2.0f;
	Line.Area = 0.18f;
	Line.Last = true;
	Line.Name = "Registered";
	O.Y = chart::Units::Count;
	O.Integer = true;
	O.Zero = true;
	O.X.Count = 6;
	O.Reveal = X.Still ? 1.0f : static_cast<float>(std::min(1.0, std::max(0.0, X.SinceOpen / 0.6)));
	O.Mouse = Vec2{-1.0f, -1.0f};
	if (H.Regs.size() >= 2)
	{
		chart::LineChart(X.C, {R.X + 16.0f, R.Y + 40.0f, R.W - 32.0f, R.H - 50.0f}, {Line}, O, chart::FromSkin(Skin::Current()));
	}
}

/** The reel (§2.8.4; FH-5b) in REGISTRATIONS' place once the Main's result exists: FINAL TABLE, 9th to 1st, one row every 0.6 s from
 *  the laptop's first draw after the result (kept per Main: it plays once), each the place, PlayerName and the prize (NetMoney: the
 *  world's result, or the engine's when the player played it); a still draws the reel whole. */
void HubReel(HubCtx& X)
{
	const HubFacts& H = X.H;
	const Rect& R = X.Fr.Regs;
	X.UI.RRect(R, 14.0f, rlsites_detail::SitesTone(0x0e1726), NetA(pal::Line, 0.9f));
	X.UI.Text("FINAL TABLE", R.X + 20.0f, R.Y + 28.0f, Ts(12.0f, 800, pal::Accent));
	const int N = static_cast<int>(H.FinalTable.size());
	int Shown = N;
	if (!X.Still)
	{
		HubCast& Cast = HubCasts()[&X.RL];
		const auto It = Cast.ReelAt.emplace(H.Main.Id, X.Now).first;
		Shown = std::min(N, static_cast<int>(std::floor((X.Now - It->second) / 0.6)) + 1);
	}
	const float Pitch = std::min(22.0f, (R.H - 44.0f) / static_cast<float>(std::max(1, N)));
	for (int K = 0; K < Shown; ++K)
	{
		// K counts the reel's rows from the top: the last place first, the champion last.
		const net::Placing& P = H.FinalTable[static_cast<size_t>(N - 1 - K)];
		const float Base = R.Y + 44.0f + Pitch * static_cast<float>(K) + Pitch * 0.72f;
		const bool First = P.Place == 1;
		if (First)
		{
			X.C.FillRoundRect({R.X + 10.0f, Base - Pitch * 0.72f, R.W - 20.0f, Pitch}, 6.0f, NetA(pal::Gold, 0.14f));
		}
		X.UI.Text(ss::Ordinal(P.Place), R.X + 60.0f, Base, Ts(13.0f, 800, First ? pal::Gold : pal::Muted, Align::Right, Baseline::Alphabetic, true));
		if (P.Player == -1)
		{
			X.UI.Text("YOU", R.X + 76.0f, Base, Ts(13.0f, 900, pal::Ink));
		}
		else
		{
			Access::PlacingPlayer(X.RL, P, R.X + 76.0f, Base, 13.0f, R.W - 300.0f);
		}
		X.UI.Text(NetMoney(P.Prize), R.X + R.W - 20.0f, Base, Ts(13.0f, 800, First ? pal::Gold : pal::Ink, Align::Right, Baseline::Alphabetic, true));
	}
}

/** WHO'S IN (§2.8.4, §4.7.3; FH-5b): the people of FieldStoriesOf(Main) in its one order, two columns of rows at 48 (twelve people;
 *  eight in Ignite's 260, R-33), each an r 12 avatar, PlayerName and its line; the Qualifiers group's line at the right of the title
 *  when it is 1 or more (U14, "regulars"). Nothing when the field holds no story. */
void HubWhoIn(HubCtx& X)
{
	const HubFacts& H = X.H;
	const Rect& R = X.Fr.Who;
	if (H.WhoIn.empty() && H.Qualifiers <= 0)
	{
		return;
	}
	X.UI.RRect(R, 14.0f, rlsites_detail::SitesTone(0x0e1726), NetA(pal::Line, 0.9f));
	X.UI.Text("WHO'S IN", R.X + 20.0f, R.Y + 28.0f, Ts(12.0f, 800, pal::Accent));
	if (H.Qualifiers > 0)
	{
		X.UI.Text(H.Qualifiers == 1 ? std::string("1 regular won their seat by satellite.") : Grouped(H.Qualifiers) + " regulars won their seat by satellite.", R.X + R.W - 20.0f,
			R.Y + 28.0f, Ts(13.0f, 600, pal::Muted, Align::Right));
	}
	const int Rows = static_cast<int>(std::floor((R.H - 44.0f) / 48.0f));
	const float ColW = (R.W - 40.0f) / 2.0f;
	const int Max = std::min(static_cast<int>(H.WhoIn.size()), Rows * 2);
	for (int I = 0; I < Max; ++I)
	{
		const HubFacts::Who& P = H.WhoIn[static_cast<size_t>(I)];
		const float Cx = R.X + 20.0f + ColW * static_cast<float>(I / Rows);
		const float Top = R.Y + 44.0f + 48.0f * static_cast<float>(I % Rows);
		HubName(X, P.Npc, std::string(), Cx + 14.0f, Cx + 36.0f, Top + 20.0f, 14.0f, ColW - 52.0f, 12.0f);
		if (!P.Line.empty())
		{
			X.UI.Text(P.Line, Cx + 36.0f, Top + 38.0f, Ts(12.0f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, false, ColW - 52.0f));
		}
	}
}

/** The Main card (§2.8.2): MAIN EVENT with the date and start, "{Gtd} GUARANTEED", the buy-in and stack, "+ {Added} ADDED" (the rival
 *  Mains), the lock line while the Main isn't joinable (§2.2.3), "{N} SEATS WON BY SATELLITE" once a seat is won, and Details
 *  (`fsmain`), which opens the lobby's event panel on the Main. The defending champion (U4) and "Last year: {Prize}." (U23) are
 *  FH-4A's and FH-3a's; the Main's state through Closing FH-5b's. */
void HubMainCard(HubCtx& X)
{
	const HubFacts& H = X.H;
	if (!H.HasMain)
	{
		return;
	}
	const Rect& M = X.Fr.Main;
	X.UI.RRect(M, 14.0f, Rgba(4, 8, 16, 0.6f), NetA(pal::Gold, 0.55f));
	const int Day = net::DayOf(H.Main.Start);
	rlnet_detail::NetSpaced(X.C, "MAIN EVENT \xC2\xB7 " + NetUpper(net::WeekdayName(Day)) + " " + NetUpper(net::DateLabel(Day)) + " \xC2\xB7 " + net::TimeLabel(H.Main.Start),
		M.X + 18.0f, M.Y + 26.0f, 10.5f, 800, pal::Gold, 1.5f);
	// FH-5b, U4: the defending champion while FieldStoriesOf lists them in the Main's field (no Registered call, Q17).
	const world::Npc* DefNpc = H.Defender >= 0 && H.W ? H.W->Get(H.Defender) : nullptr;
	const std::string Defends = DefNpc ? "DEFENDING CHAMPION \xC2\xB7 " + DefNpc->Name + " \xC2\xB7 registered" : std::string();
	ButtonOpts O;
	O.Size = 14.0f;
	if (H.Broadcast)
	{
		// Main Event Day (§2.8.4): Details beside the guarantee, one line over the meter (the lock line, else the buy-in) and one
		// under it (U4, else the added money, else the seats won).
		if (H.MainGtd > 0)
		{
			X.UI.Text(NetMoney(H.MainGtd) + " GUARANTEED", M.X + 18.0f, M.Y + 58.0f, Ts(22.0f, 900, pal::Gold, Align::Left, Baseline::Alphabetic, false, M.W - 132.0f));
		}
		if (!H.MainLock.empty())
		{
			rlnet_detail::NetLockIcon(X.C, M.X + 18.0f, M.Y + 70.0f, 11.0f, pal::Orange);
			X.UI.Text(H.MainLock, M.X + 34.0f, M.Y + 80.0f, Ts(12.0f, 600, pal::Orange, Align::Left, Baseline::Alphabetic, false, M.W - 52.0f));
		}
		else
		{
			X.UI.Text(net::BuyIn(H.MainBuyIn) + " buy-in \xC2\xB7 " + Grouped(H.MainStack) + " chips", M.X + 18.0f, M.Y + 80.0f, Ts(13.0f, 600, rlsites_detail::SitesTone(0xc3cedf)));
		}
		HubMeter(X, {M.X + 16.0f, M.Y + 98.0f, M.W - 32.0f, 48.0f});
		const std::string Foot = !Defends.empty() ? Defends
			: H.MainAdded > 0                     ? "+ " + NetMoney(H.MainAdded) + " ADDED"
			: H.SeatsWon > 1                      ? std::to_string(H.SeatsWon) + " SEATS WON BY SATELLITE"
			: H.SeatsWon == 1                     ? std::string("1 SEAT WON BY SATELLITE")
												  : std::string();
		if (!Foot.empty())
		{
			X.UI.Text(Foot, M.X + 18.0f, M.Y + 158.0f, Ts(11.0f, 800, !Defends.empty() ? pal::Gold : H.MainAdded > 0 ? pal::Accent : pal::Muted, Align::Left, Baseline::Alphabetic, false,
				M.W - 36.0f));
		}
		if (rlsites_detail::SitesButton(X.UI, X.C, "fsmain", {M.X + M.W - 104.0f, M.Y + 36.0f, 88.0f, 28.0f}, "Details", O) && !X.Still)
		{
			Access::SelectEvent(X.RL, H.Main.Id);
			Access::OpenPage(X.RL, RiverLine::Page::Lobby, X.Now);
		}
		return;
	}
	if (H.MainGtd > 0)
	{
		X.UI.Text(NetMoney(H.MainGtd) + " GUARANTEED", M.X + 18.0f, M.Y + 58.0f, Ts(22.0f, 900, pal::Gold, Align::Left, Baseline::Alphabetic, false, M.W - 36.0f));
	}
	float Y = M.Y + 80.0f;
	X.UI.Text(net::BuyIn(H.MainBuyIn) + " buy-in \xC2\xB7 " + Grouped(H.MainStack) + " chips", M.X + 18.0f, Y, Ts(13.0f, 600, rlsites_detail::SitesTone(0xc3cedf)));
	if (!H.MainLock.empty())
	{
		Y += 20.0f;
		rlnet_detail::NetLockIcon(X.C, M.X + 18.0f, Y - 10.0f, 11.0f, pal::Orange);
		X.UI.Text(H.MainLock, M.X + 34.0f, Y, Ts(12.0f, 600, pal::Orange, Align::Left, Baseline::Alphabetic, false, M.W - 52.0f));
	}
	if (H.MainAdded > 0)
	{
		Y += 20.0f;
		X.UI.Text("+ " + NetMoney(H.MainAdded) + " ADDED", M.X + 18.0f, Y, Ts(12.0f, 800, pal::Accent));
	}
	if (H.SeatsWon > 0)
	{
		Y += 20.0f;
		X.UI.Text(H.SeatsWon == 1 ? std::string("1 SEAT WON BY SATELLITE") : std::to_string(H.SeatsWon) + " SEATS WON BY SATELLITE", M.X + 18.0f, Y, Ts(11.0f, 800, pal::Muted));
	}
	if (!Defends.empty())
	{
		// FH-5b, U4 (§2.6.3): under the card's other lines, clear of Details once it reaches the button's band.
		Y += 20.0f;
		X.UI.Text(Defends, M.X + 18.0f, Y, Ts(11.0f, 800, pal::Gold, Align::Left, Baseline::Alphabetic, false, Y > M.Y + M.H - 50.0f ? M.W - 140.0f : M.W - 36.0f));
	}
	if (rlsites_detail::SitesButton(X.UI, X.C, "fsmain", {M.X + M.W - 114.0f, M.Y + M.H - 44.0f, 96.0f, 32.0f}, "Details", O) && !X.Still)
	{
		Access::SelectEvent(X.RL, H.Main.Id);
		Access::OpenPage(X.RL, RiverLine::Page::Lobby, X.Now);
	}
}

/** THE FESTIVAL YEAR (§2.8.2, U1; FH-1b): the label in one NetSpaced line, then the ten cells in calendar order (`fsy0..9`), each its
 *  brand's on-stage or next edition: the low-detail crest at 26, the Short (13/800 in its 92-unit column, 80 on RunItTwice), the
 *  opening date (11/600 Muted), "BY INVITATION" (9.5/800) on a rival cell without an account, "PASS {N}/{Need}" there while the
 *  Festival Pass counts (U30, FH-2); a crest outside its window rests at
 *  0.45; the underlined one carries a 2-unit Accent line. A click opens the cell's popover, clamped inside the content rect R. */
void HubYearStrip(HubCtx& X, HubUi& U, const Rect& R)
{
	const HubFrame& Fr = X.Fr;
	const Rect& Yr = Fr.Year;
	// The label, narrowed to the room before the first cell (RunItTwice's 136 units).
	const std::string YearLabel = "THE FESTIVAL YEAR";
	const float Track = 1.6f;
	const float Room = Fr.YearX - Yr.X - 14.0f;
	float Size = 10.5f;
	while (Size > 8.0f && X.C.Measure(YearLabel, Size, 800) + Track * static_cast<float>(YearLabel.size() - 1) > Room)
	{
		Size -= 0.5f;
	}
	X.C.FillRoundRect({Yr.X, Yr.Y + 12.0f, 3.0f, 16.0f}, 1.5f, pal::Accent);
	rlnet_detail::NetSpaced(X.C, YearLabel, Yr.X + 11.0f, Yr.Y + 25.0f, Size, 800, pal::Muted, Track);
	for (int I = 0; I < 10; ++I)
	{
		const HubYearCell& Cl = X.H.Year[I];
		if (!Cl.F)
		{
			continue;
		}
		const Rect Cell{Fr.YearX + Fr.YearPitch * static_cast<float>(I), Yr.Y, Fr.YearPitch - 6.0f, Yr.H};
		const Ui::ClickState St = X.UI.Clickable("fsy" + std::to_string(I), Cell);
		if (St.Clicked && !X.Still)
		{
			// §2.8.6: {cell.X, 128, 360, 168} on RiverLine and Ignite (under the strip; RunItTwice's strip sits lower), inside R.
			HubUi::Pop P;
			P.Open = true;
			P.Fresh = true;
			P.Cell = I;
			P.R = {std::max(R.X, std::min(Cell.X, R.X + R.W - 360.0f)), Yr.Y + Yr.H + 4.0f, 360.0f, 168.0f};
			P.C = Cl;
			P.Today = X.H.Today;
			U.Popover = P;
		}
		const bool PopHere = U.Popover.Open && U.Popover.Cell == I;
		X.UI.RRect(Cell, 10.0f, PopHere ? NetA(pal::Accent, 0.18f) : St.Hover ? rlsites_detail::SitesTone(0x172a42) : rlsites_detail::SitesTone(0x101a2b),
			PopHere ? pal::Accent : NetA(pal::Line, Cl.Lit ? 0.9f : 0.5f));
		const float A0 = X.C.GetAlpha();
		X.C.SetAlpha(A0 * (Cl.Lit ? 1.0f : 0.45f));
		eventart::SeriesCrest(X.C, *Cl.Series, Cell.X + 19.0f, Cell.Y + 20.0f, 26.0f, X.Still ? 0.0 : X.Now);
		const float Tx = Cell.X + 36.0f;
		const bool IsInvite = !Cl.Account; // a third line: BY INVITATION, or the Pass's count (U30, FH-2)
		X.UI.Text(NetUpper(Cl.Series->Short), Tx, Cell.Y + (IsInvite ? 14.0f : 17.0f), Ts(13.0f, 800, Cl.Lit ? pal::Ink : pal::Muted, Align::Left, Baseline::Alphabetic, false, Fr.YearText));
		X.UI.Text(net::DateLabel(Cl.F->FirstDay), Tx, Cell.Y + (IsInvite ? 26.0f : 31.0f), Ts(11.0f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, false, Fr.YearText));
		if (IsInvite)
		{
			const bool Counting = Cl.PassForm == 2;
			X.UI.Text(Counting ? "PASS " + std::to_string(Cl.PassN) + "/" + std::to_string(Cl.PassNeed) : std::string("BY INVITATION"), Tx, Cell.Y + 37.0f,
				Ts(9.5f, 800, Counting ? pal::Accent : pal::Orange, Align::Left, Baseline::Alphabetic, false, Fr.YearText));
		}
		X.C.SetAlpha(A0);
		if (Cl.Underline)
		{
			X.C.FillRect({Cell.X + 6.0f, Cell.Y + Cell.H - 2.0f, Cell.W - 12.0f, 2.0f}, pal::Accent);
		}
	}
}

/** An outline star (the day strip's mark on a championship's other title days, §2.8.2). */
void HubStarOutline(Canvas& C, float Cx, float Cy, float R, const Color& Col)
{
	std::vector<Vec2> P;
	for (int I = 0; I < 10; ++I)
	{
		const float A = -Pi / 2.0f + static_cast<float>(I) * Pi / 5.0f;
		const float Rr = I % 2 == 0 ? R : R * 0.42f;
		P.push_back({Cx + Rr * std::cos(A), Cy + Rr * std::sin(A)});
	}
	C.StrokePolyline(P, true, Col, 1.2f);
}

/** The day strip (§2.8.2; FH-1b): one cell a day of the window (`fsd<day>`; a night on GRIT, FH-2), today lit, the day shown
 *  outlined, a star on Main day and an outline star on the edition's other title days. Returns the day shown: the player's pick, else today held inside the window. */
int HubDays(HubCtx& X, HubUi& U)
{
	const HubFacts& H = X.H;
	const fest::Festival& F = *H.F;
	const Rect& D = X.Fr.Days;
	int Shown = U.Day >= F.FirstDay && U.Day <= F.LastDay ? U.Day : std::min(std::max(H.Today, F.FirstDay), F.LastDay);
	const int Count = std::max(1, H.DayCount);
	const float Gap = 4.0f;
	const float Cw = (D.W - Gap * static_cast<float>(Count - 1)) / static_cast<float>(Count);
	const int MainDay = H.HasMain ? net::DayOf(H.Main.Start) : F.MainDay;
	const bool Nights = F.Which == fest::Brand::Grit;
	for (int Day = F.FirstDay; Day <= F.LastDay; ++Day)
	{
		const Rect R{D.X + static_cast<float>(Day - F.FirstDay) * (Cw + Gap), D.Y, Cw, D.H};
		const Ui::ClickState St = X.UI.Clickable("fsd" + std::to_string(Day), R);
		if (St.Clicked && !X.Still)
		{
			U.Day = Day;
			U.RowFirst = 0;
			Shown = Day;
		}
		const bool IsToday = Day == H.Today;
		const bool On = Day == Shown;
		X.UI.RRect(R, 8.0f, IsToday ? NetA(pal::Accent, 0.22f) : St.Hover ? rlsites_detail::SitesTone(0x172a42) : rlsites_detail::SitesTone(0x101a2b),
			On ? pal::Accent : IsToday ? NetA(pal::Accent, 0.6f) : NetA(pal::Line, 0.6f), On ? 1.5f : 1.0f);
		const float Mx = R.X + R.W / 2.0f;
		// FH-2: GRIT's strip reads nights ("NIGHT 4"), every other edition's the weekday's initial.
		const std::string Top = Nights ? "NIGHT " + std::to_string(Day - F.FirstDay + 1) : std::string(net::WeekdayName(Day)).substr(0, 1);
		X.UI.Text(Top, Mx, R.Y + 17.0f, Ts(Nights ? 9.0f : 10.0f, 800, IsToday ? pal::Accent : pal::Muted, Align::Center, Baseline::Alphabetic, false, R.W - 6.0f));
		const std::string Date = net::DateLabel(Day);
		X.UI.Text(Date.substr(Date.find(' ') + 1), Mx, R.Y + 40.0f, Ts(17.0f, 900, Day < H.Today && !On ? pal::Dim : pal::Ink, Align::Center));
		if (Day == MainDay)
		{
			rlnet_detail::NetStar(X.C, R.X + R.W - 7.0f, R.Y + 8.0f, 4.5f, pal::Gold);
		}
		else if (std::find(H.TitleDays.begin(), H.TitleDays.end(), Day) != H.TitleDays.end())
		{
			HubStarOutline(X.C, R.X + R.W - 7.0f, R.Y + 8.0f, 4.5f, pal::Gold);
		}
	}
	return Shown;
}

/** The edition's satellites and the ladder up to them (§2.2.3: RCOP's step1-4 and mega-rcop): F.Satellites, then every satellite
 *  whose seat is a ticket of one already in the set, recurring or a one-off inside the edition's window (FestCalRow's rule). Built
 *  once an edition (HubUi::Ladder). */
std::vector<std::string> HubLadder(const net::Network& Net, const fest::Festival& F)
{
	std::vector<std::string> Out = F.Satellites;
	std::vector<std::string> Tickets;
	for (const std::string& Id : Out)
	{
		Tickets.push_back(Net.TicketOf(Id));
	}
	for (int Pass = 0; Pass < 6; ++Pass)
	{
		bool Grew = false;
		for (const net::EventTemplate& T : Net.Templates())
		{
			if (T.Fmt != net::Format::Satellite || T.SeatTicket.empty() || std::find(Out.begin(), Out.end(), T.Id) != Out.end())
			{
				continue;
			}
			if (T.OnlyDay != -9999 && (T.OnlyDay < F.FirstDay - fest::kAnnounceDays || T.OnlyDay > F.MainDay))
			{
				continue;
			}
			if (std::find(Tickets.begin(), Tickets.end(), T.SeatTicket) != Tickets.end())
			{
				Out.push_back(T.Id);
				Tickets.push_back(Net.TicketOf(T.Id));
				Grew = true;
			}
		}
		if (!Grew)
		{
			break;
		}
	}
	return Out;
}

/** A championship's step ladder (§2.8.2, U35; FH-4A fix round 1): the edition's own one-off satellites (F.Satellites) that seat
 *  another of them or that another of them seats (Step 1 seats a Step 2 ticket, Step 2 a Step 3 ticket, Step 3 the Main's), each
 *  with its rung counted from the bottom and the rung its seat is (0: the Main); a step's two runs take one ticket, so they share
 *  their rung. The Mega and the Last Chance seat the Main with no rung below them, and a recurring ladder (RCOP's step1-4, every
 *  four hours) is no edition's own: none of them is a step row. Built once an edition (HubUi::Steps). */
std::map<std::string, std::pair<int, int>> HubSteps(const net::Network& Net, const fest::Festival& F)
{
	std::vector<const net::EventTemplate*> Own;
	for (const std::string& Id : F.Satellites)
	{
		const net::EventTemplate* T = Net.FindTemplate(Id);
		if (T && T->Fmt == net::Format::Satellite && T->OnlyDay != -9999 && !T->SeatTicket.empty())
		{
			Own.push_back(T);
		}
	}
	// The rung below T (one seating T's ticket) and the rung T's seat is (one taking T's seat ticket).
	auto Below = [&Net, &Own](const net::EventTemplate* T) -> const net::EventTemplate* {
		const std::string Ticket = Net.TicketOf(T->Id);
		for (const net::EventTemplate* V : Own)
		{
			if (V != T && V->SeatTicket == Ticket)
			{
				return V;
			}
		}
		return nullptr;
	};
	auto Into = [&Net, &Own](const net::EventTemplate* T) -> const net::EventTemplate* {
		for (const net::EventTemplate* V : Own)
		{
			if (V != T && Net.TicketOf(V->Id) == T->SeatTicket)
			{
				return V;
			}
		}
		return nullptr;
	};
	auto Rung = [&Below](const net::EventTemplate* T) {
		int K = 1;
		for (const net::EventTemplate* V = Below(T); V && K < 8; V = Below(V))
		{
			++K;
		}
		return K;
	};
	std::map<std::string, std::pair<int, int>> Out;
	for (const net::EventTemplate* T : Own)
	{
		const net::EventTemplate* Up = Into(T);
		if (Up || Below(T))
		{
			Out[T->Id] = {Rung(T), Up ? Rung(Up) : 0};
		}
	}
	return Out;
}

/** U35's two pills for a step row (§2.8.2, §2.11): "STEP {K}" and "FOR A STEP {Next} TICKET", or "FOR A MAIN EVENT SEAT" when its
 *  seat is the Main's (Next 0). Pure (u4_ui's FestCopyUi calls it). */
std::pair<std::string, std::string> HubStepPills(int K, int Next)
{
	return {"STEP " + std::to_string(K), Next > 0 ? "FOR A STEP " + std::to_string(Next) + " TICKET" : std::string("FOR A MAIN EVENT SEAT")};
}

/** The day's rows (§2.8.2): the edition's events that day in start order (a GRIT show's two runs in one row, FH-2), then one row for
 *  each satellite into its Main running that day (its first instance not yet finished, else the day's last), rebuilt once a world
 *  minute or on a new day. */
void HubDayRows(const net::Network& Net, const fest::Festival& F, HubUi& U, int Day, double World)
{
	const long long Minute = static_cast<long long>(std::floor(World));
	if (U.RowsDay == Day && U.RowsMinute == Minute)
	{
		return;
	}
	if (!U.LadderBuilt)
	{
		U.Ladder = HubLadder(Net, F);
		U.Steps = HubSteps(Net, F);
		U.LadderBuilt = true;
	}
	U.RowsDay = Day;
	U.RowsMinute = Minute;
	U.Rows.clear();
	U.Late.clear();
	std::vector<net::EventInstance> Sats;
	const bool Nights = F.Which == fest::Brand::Grit;
	for (net::EventInstance& E : Net.Window(HubMinute(Day), HubMinute(Day + 1)))
	{
		const net::EventTemplate& T = Net.TemplateOf(E);
		if (T.Series == F.SeriesId)
		{
			if (Nights)
			{
				// FH-2: a GRIT show's second run that night (one template, run at 7 PM and 10 PM) joins its first run's row.
				const auto First = std::find_if(U.Rows.begin(), U.Rows.end(), [&E](const net::EventInstance& O) { return O.Template == E.Template; });
				if (First != U.Rows.end())
				{
					U.Late[static_cast<size_t>(First - U.Rows.begin())] = std::move(E);
					continue;
				}
			}
			U.Rows.push_back(std::move(E));
			U.Late.emplace_back();
			continue;
		}
		if (T.Fmt != net::Format::Satellite || std::find(U.Ladder.begin(), U.Ladder.end(), T.Id) == U.Ladder.end())
		{
			continue;
		}
		const auto Same = std::find_if(Sats.begin(), Sats.end(), [&E](const net::EventInstance& O) { return O.Template == E.Template; });
		if (Same == Sats.end())
		{
			Sats.push_back(std::move(E));
		}
		else if (Net.Live(*Same, World).St == net::Status::Finished)
		{
			*Same = std::move(E);
		}
	}
	for (net::EventInstance& E : Sats)
	{
		U.Rows.push_back(std::move(E));
		U.Late.emplace_back();
	}
	// FH-5b (§2.8.4): a championship's Mini and Micro Main Event rows pinned at the top of their day (the edition's other title
	// templates; the Main's day is the broadcast), each keeping its place among the rest.
	std::vector<size_t> Order;
	for (const bool Title : {true, false})
	{
		for (size_t I = 0; I < U.Rows.size(); ++I)
		{
			const std::string& Tid = Net.TemplateOf(U.Rows[I]).Id;
			if ((std::find(F.TitleIds.begin() + (F.TitleIds.empty() ? 0 : 1), F.TitleIds.end(), Tid) != F.TitleIds.end()) == Title)
			{
				Order.push_back(I);
			}
		}
	}
	std::vector<net::EventInstance> Rows;
	std::vector<net::EventInstance> Late;
	for (const size_t I : Order)
	{
		Rows.push_back(std::move(U.Rows[I]));
		Late.push_back(std::move(U.Late[I]));
	}
	U.Rows = std::move(Rows);
	U.Late = std::move(Late);
}

/** A satellite's seat followed up its ladder to the Main it ends in: that Main's lock line while it isn't joinable (§2.2.3), else "". */
std::string HubSeatLock(const net::Network& Net, const net::EventTemplate& T, double From, int Unlocks)
{
	const net::EventTemplate* At = &T;
	for (int K = 0; K < 8 && At->Fmt == net::Format::Satellite && !At->SeatTicket.empty(); ++K)
	{
		const net::EventTemplate* Up = Net.FindTemplate(At->SeatTicket);
		if (!Up)
		{
			return std::string();
		}
		At = Up;
	}
	net::EventInstance Target;
	std::string SeatLock;
	if (At == &T || At->Fmt == net::Format::Satellite || !Net.Next(At->Id, From, Target) || Net.Joinable(Target, &SeatLock, Unlocks))
	{
		return std::string();
	}
	return SeatLock;
}

/** A show's start as GRIT's buttons print it (§2.8.2: "7 PM", "10 PM"; "7:30 PM" off the hour). */
std::string HubShowTime(double Start)
{
	const long long M = (static_cast<long long>(std::llround(Start - std::floor(Start / net::MinutesPerDay) * net::MinutesPerDay)) % 1440 + 1440) % 1440;
	const long long H = M / 60;
	const long long H12 = H % 12 == 0 ? 12 : H % 12;
	return std::to_string(H12) + (M % 60 != 0 ? ":" + rlnet_detail::NetPad2(M % 60) : std::string()) + (H < 12 ? " AM" : " PM");
}

/** The day's rows (§2.8.2; FH-1b): seven at 72 in view (the wheel scrolls the rest; Ignite's seventh peeks under its frame), each
 *  `ev:<id>` (a click opens the lobby's event panel on it), written to Listed: the start, the low-detail emblem at 30, the number and
 *  the name, GTD or pool and the buy-in, the pills (MAIN EVENT, a title's MINI or MICRO MAIN EVENT, SATELLITE; a championship's step
 *  row its U35 pair, STEP {K} in the number's place and its seat in SATELLITE's, FH-4A fix round 1), the status and the entries;
 *  a satellite into a Main that isn't joinable shows the Main's lock line and U31 instead of its money line. The button is
 *  RegisterBlock's (SitesTwoPress, the clause's second press, core's strike gate: FestStrikeGate): on every unfinished row until one
 *  is armed, then on that row alone, which a press on its button selects first (Session::ConfirmRegister is one per session); a
 *  finished row shows its winner in the button's place.
 *  FH-2: a GRIT row holds its show's two runs: two buttons, "7 PM" and "10 PM", beside the runs' codes (Network::NameOf) in the start's
 *  column, pick the run its narrower RegisterBlock (300) registers (the run the player is in, else the one selected, else the player's pick, else the first
 *  still taking entries); while the confirm step is armed the pick holds. The Summons' money line is U33's seats. On Ignite (L7) the
 *  rows' foot fades from y 826 and screen 12's bar shows where the view sits while the day holds more rows than show whole. */
void HubRows(HubCtx& X, HubUi& U, int Day)
{
	const HubFacts& H = X.H;
	const fest::Festival& F = *H.F;
	const net::Network& Net = net::Site(F.Site);
	const Session& S = Access::SessionOf(X.RL);
	const Rect& Rr = X.Fr.Rows;
	HubDayRows(Net, F, U, Day, H.World);
	constexpr float Pitch = 72.0f;
	constexpr float RowH = 66.0f;
	const int Visible = std::max(1, static_cast<int>(Rr.H / Pitch));
	const int Count = static_cast<int>(U.Rows.size());
	if (!X.Still && X.UI.Hover(Rr) && X.UI.Ptr.Wheel != 0.0f)
	{
		U.RowFirst += X.UI.Ptr.Wheel > 0.0f ? 1 : -1;
	}
	U.RowFirst = std::max(0, std::min(U.RowFirst, Count - Visible));
	if (!X.Still)
	{
		std::vector<std::string> Ids;
		for (size_t I = 0; I < U.Rows.size(); ++I)
		{
			Ids.push_back(U.Rows[I].Id);
			if (!U.Late[I].Id.empty())
			{
				Ids.push_back(U.Late[I].Id);
			}
		}
		Access::SetListed(X.RL, std::move(Ids));
	}
	const bool Armed = S.ConfirmRegister;
	const std::string Selected = X.RL.SelectedEvent();
	const Color SoftCol = rlsites_detail::SitesTone(0xc3cedf);
	const Color EdgeCol = Hex(H.Series->Color);
	X.C.PushClip(Rr);
	for (int I = U.RowFirst; I < Count && Pitch * static_cast<float>(I - U.RowFirst) < Rr.H; ++I)
	{
		const net::EventInstance& E0 = U.Rows[static_cast<size_t>(I)];
		const net::EventInstance* E1 = U.Late[static_cast<size_t>(I)].Id.empty() ? nullptr : &U.Late[static_cast<size_t>(I)];
		const bool Pair = E1 != nullptr;
		// FH-2: the run this row shows and registers.
		int Pick = 0;
		if (Pair)
		{
			const auto Picked = U.RunPick.find(E0.Id);
			const net::Status St0 = Net.Live(E0, H.World).St;
			const bool FirstShut = St0 == net::Status::Running || St0 == net::Status::FinalTable || St0 == net::Status::Finished;
			Pick = S.IsPlaying(E0.Id)       ? 0
				: S.IsPlaying(E1->Id)       ? 1
				: Selected == E1->Id        ? 1
				: Selected == E0.Id         ? 0
				: Picked != U.RunPick.end() ? Picked->second
				: FirstShut                 ? 1
											: 0;
		}
		const net::EventInstance& E = Pick == 1 ? *E1 : E0;
		const net::EventTemplate& T = Net.TemplateOf(E);
		const net::LiveState L = Net.Live(E, H.World);
		const Rect R{Rr.X, Rr.Y + Pitch * static_cast<float>(I - U.RowFirst), Rr.W, RowH};
		const Rect Rb = Pair ? Rect{R.X + R.W - 312.0f, R.Y + 10.0f, 300.0f, 46.0f} : Rect{R.X + R.W - 372.0f, R.Y + 10.0f, 360.0f, 46.0f};
		// GRIT's runs take the start's column: the codes right-aligned to x + 66, the buttons from x + 72; the emblem and the name move
		// right by 54.
		const float Shift = Pair ? 54.0f : 0.0f;
		const Rect RunsR{R.X + 8.0f, R.Y + 6.0f, 132.0f, 56.0f};
		const bool IsMine = Selected == E0.Id || (Pair && Selected == E1->Id);
		const bool OverReg = X.UI.Hover(Rb);
		const bool OverButton = OverReg || (Pair && X.UI.Hover(RunsR));
		const Ui::ClickState St = X.UI.Clickable("ev:" + E.Id, R, !OverButton);
		if (St.Clicked && !X.Still)
		{
			Access::SelectEvent(X.RL, E.Id);
			Access::OpenPage(X.RL, RiverLine::Page::Lobby, X.Now);
		}
		const auto TitleIt = std::find(F.TitleIds.begin(), F.TitleIds.end(), T.Id);
		const bool IsMain = T.Id == F.MainId;
		const bool TitleEvent = !IsMain && TitleIt != F.TitleIds.end();
		const bool IsSat = T.Fmt == net::Format::Satellite;
		// FH-4A fix round 1, U35: a championship's step row (HubSteps) and its two pills.
		const auto StepIt = IsSat ? U.Steps.find(T.Id) : U.Steps.end();
		const bool IsStep = StepIt != U.Steps.end();
		const std::pair<std::string, std::string> StepPills = IsStep ? HubStepPills(StepIt->second.first, StepIt->second.second) : std::pair<std::string, std::string>();
		X.UI.RRect(R, 10.0f, St.Hover ? rlsites_detail::SitesTone(0x15253d) : rlsites_detail::SitesTone(0x101a2b),
			IsMain ? NetA(pal::Gold, 0.55f) : IsMine ? NetA(pal::Accent, 0.6f) : NetA(pal::Line, 0.6f));
		X.C.FillRoundRect({R.X + 1.0f, R.Y + 10.0f, 4.0f, R.H - 20.0f}, 2.0f, IsMain || TitleEvent ? pal::Gold : EdgeCol);
		if (!Pair)
		{
			X.UI.Text(net::TimeLabel(E.Start), R.X + 18.0f, R.Y + 28.0f, Ts(14.0f, 700, pal::Muted));
		}
		eventart::Emblem(X.C, T, R.X + 104.0f + Shift, R.Y + 33.0f, 30.0f, X.Still ? 0.0 : X.Now);
		const float StatusX = Rb.X - 138.0f;
		float Nx = R.X + 128.0f + Shift;
		if (T.EventNo > 0)
		{
			Nx += rlnet_detail::NetPill(X.C, "#" + std::to_string(T.EventNo), Nx, R.Y + 14.0f, IsMain || TitleEvent ? pal::Gold : EdgeCol, true, 10.0f) + 8.0f;
		}
		else if (IsStep)
		{
			// U35's STEP {K} in the number's place (a satellite has none).
			Nx += rlnet_detail::NetPill(X.C, StepPills.first, Nx, R.Y + 14.0f, pal::Accent2, true, 10.0f) + 8.0f;
		}
		X.UI.Text(rlnet_detail::NetShortName(T.Name), Nx, R.Y + 28.0f, Ts(15.0f, 700, pal::Ink, Align::Left, Baseline::Alphabetic, false, std::max(40.0f, StatusX - 12.0f - Nx)));
		const std::string SeatLock = IsSat ? HubSeatLock(Net, T, E.Start, S.Unlocks()) : std::string();
		float Lx = R.X + 128.0f + Shift;
		if (!SeatLock.empty())
		{
			// §2.2.3: the button stays, and the row says the truth beside it.
			rlnet_detail::NetLockIcon(X.C, Lx, R.Y + 36.0f, 11.0f, pal::Orange);
			X.UI.Text(SeatLock, Lx + 16.0f, R.Y + 46.0f, Ts(12.0f, 600, pal::Orange, Align::Left, Baseline::Alphabetic, false, StatusX - 28.0f - Lx));
			X.UI.Text("A seat won here waits in your wallet until big fields open.", Lx, R.Y + 61.0f, Ts(11.0f, 500, pal::Muted, Align::Left, Baseline::Alphabetic, false, StatusX - 12.0f - Lx));
		}
		else
		{
			std::string PotLine;
			// FH-2, U33: The Summons (WRIT's satellite, no seat guarantee) prints its seats, max(1, Pool / SeatValue) at the pool so far,
			// then the seats it won (§2.8.3; the world's seat rule, G/WorldSim.cpp:492).
			const bool Summons = IsSat && F.Which == fest::Brand::Writ && T.SeatValueCents > 0 &&
				std::find(F.Satellites.begin(), F.Satellites.end(), T.Id) != F.Satellites.end();
			const bool Done = L.St == net::Status::Finished;
			const Chips SeatPool = Done ? E.Pool : L.Pool;
			if (Summons && SeatPool > 0)
			{
				const long long Seats = std::max<long long>(1, SeatPool / T.SeatValueCents);
				PotLine = Seats == 1 ? std::string(Done ? "1 SEAT WON" : "1 SEAT AT THE POOL SO FAR") : std::to_string(Seats) + (Done ? " SEATS WON" : " SEATS AT THE POOL SO FAR");
			}
			else if (T.GtdCents > 0)
			{
				PotLine = NetMoney(std::max(L.Pool, T.GtdCents)) + (Done ? "" : " GTD");
			}
			else if (L.Pool > 0)
			{
				PotLine = NetMoney(L.Pool);
			}
			if (!PotLine.empty())
			{
				Lx += X.UI.Text(PotLine, Lx, R.Y + 51.0f, Ts(13.0f, 700, SoftCol)) + 14.0f;
			}
			if (T.BuyInCents > 0)
			{
				Lx += X.UI.Text(net::BuyIn(T.BuyInCents), Lx, R.Y + 51.0f, Ts(13.0f, 800, pal::Gold)) + 14.0f;
			}
			const bool IsMini = T.Id.size() >= 4 && T.Id.compare(T.Id.size() - 4, 4, "mini") == 0;
			const std::string PillText = IsMain ? std::string("MAIN EVENT")
				: TitleEvent                     ? std::string(IsMini ? "MINI MAIN EVENT" : "MICRO MAIN EVENT")
				: IsStep                         ? StepPills.second
				: IsSat                          ? std::string("SATELLITE")
												 : std::string();
			if (!PillText.empty() && Lx < StatusX - 40.0f)
			{
				// U35's seat pill is the row's longest: a size smaller where it would reach the entries' column (RunItTwice's 860).
				const float PillSize = IsStep && Lx + X.C.Measure(PillText, 9.5f, 700) + 9.5f * 1.3f > StatusX - 6.0f ? 8.5f : 9.5f;
				rlnet_detail::NetPill(X.C, PillText, Lx, R.Y + 39.0f, IsMain || TitleEvent ? pal::Gold : pal::Accent2, false, PillSize);
			}
		}
		// The status (the series page's own forms) and the entries.
		const std::string StatusLine = L.St == net::Status::LateReg ? "LATE " + rlnet_detail::NetHms(L.LateRegLeft)
			: L.St == net::Status::Registering || L.St == net::Status::Announced ? "IN " + NetUpper(net::Countdown(L.StartsIn))
																				   : NetUpper(net::StatusName(L.St));
		rlnet_detail::NetPill(X.C, StatusLine, StatusX, R.Y + 14.0f, rlnet_detail::NetStatus(L.St), false, 10.0f);
		const int Entries = L.St == net::Status::Finished ? E.Entries : L.Entries;
		if (Entries > 0)
		{
			X.UI.Text(Entries == 1 ? std::string("1 entry") : Grouped(Entries) + " entries", StatusX, R.Y + 51.0f, Ts(12.0f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, false, 128.0f));
		}
		if (Pair)
		{
			// FH-2: GRIT's two runs, each its code and its button; a press picks the run (held while the confirm step is armed).
			for (int K = 0; K < 2; ++K)
			{
				const net::EventInstance& Run = K == 1 ? *E1 : E0;
				const Rect Bk{R.X + 72.0f, R.Y + 8.0f + 28.0f * static_cast<float>(K), 64.0f, 24.0f};
				const std::string RunName = Net.NameOf(Run);
				X.UI.Text(RunName.substr(0, RunName.find(": ")), R.X + 66.0f, Bk.Y + 16.0f,
					Ts(10.5f, 700, K == Pick ? pal::Ink : pal::Muted, Align::Right, Baseline::Alphabetic, false, 56.0f));
				ButtonOpts O;
				O.Size = 12.0f;
				O.Kind = K == Pick ? ButtonKind::Primary : ButtonKind::Secondary;
				if (rlsites_detail::SitesButton(X.UI, X.C, "fsrun" + std::to_string(K) + ":" + E0.Id, Bk, HubShowTime(Run.Start), O) && !X.Still && !Armed)
				{
					U.RunPick[E0.Id] = K;
					Access::SelectEvent(X.RL, Run.Id);
				}
			}
		}
		if (L.St == net::Status::Finished)
		{
			const net::EventResult& Res = Net.Result(E);
			if (!Res.FinalTable.empty())
			{
				const net::Placing& W = Res.FinalTable.front();
				rlnet_detail::NetTrophy(X.C, Rb.X + 18.0f, Rb.Y + 9.0f, 26.0f, Hex(0xffe08a), Hex(0xc9962b));
				X.UI.Text(Access::PlacingName(X.RL, W), Rb.X + 40.0f, Rb.Y + 20.0f, Ts(14.0f, 700, W.Player == -1 ? pal::Accent : pal::Ink, Align::Left, Baseline::Alphabetic, false, Rb.W - 44.0f));
				X.UI.Text(Money(W.Prize), Rb.X + 40.0f, Rb.Y + 38.0f, Ts(12.0f, 700, pal::Gold, Align::Left, Baseline::Alphabetic, true));
			}
		}
		else if (!Armed || IsMine)
		{
			// The press that arms the step first selects this row, so the one confirm step is this row's (a two-frame click too).
			if (!Armed && Selected != E.Id && !X.Still && OverReg && (X.UI.Ptr.Pressed || X.UI.Ptr.Released))
			{
				Access::SelectEvent(X.RL, E.Id);
			}
			Access::RegisterBlock(X.RL, E, L, Rb, X.Now);
		}
	}
	if (X.Fr.FadeY > 0.0f && X.Fr.FadeY < Rr.Y + Rr.H)
	{
		// FH-2, Ignite (L7): the seventh row's top shows under a fade to the frame's ground.
		const Rect Fade{Rr.X, X.Fr.FadeY, Rr.W, Rr.Y + Rr.H - X.Fr.FadeY};
		X.C.FillRect(Fade, Paint::Linear({0.0f, Fade.Y}, {0.0f, Fade.Y + Fade.H}, NetA(pal::Bg, 0.0f), NetA(pal::Bg, 0.92f)));
	}
	X.C.PopClip();
	if (X.Fr.BarX > 0.0f && Count > Visible)
	{
		// Screen 12's scroll bar (x 1014, the rows' height less 4): the thumb's share and place are the view's.
		const float TrackH = Rr.H - 4.0f;
		const float ThumbH = std::max(24.0f, TrackH * static_cast<float>(Visible) / static_cast<float>(Count));
		const float ThumbY = Rr.Y + (TrackH - ThumbH) * static_cast<float>(U.RowFirst) / static_cast<float>(std::max(1, Count - Visible));
		X.C.FillRoundRect({X.Fr.BarX, Rr.Y, 4.0f, TrackH}, 2.0f, NetA(pal::Line, 0.6f));
		X.C.FillRoundRect({X.Fr.BarX, ThumbY, 4.0f, ThumbH}, 2.0f, NetA(pal::Accent, 0.8f));
	}
}

/** The overlay ticker's run (§2.4.4, U26; FH-1b; on GRIT's hub U15's GRIT line leads while it runs, FH-2): for each of the edition's
 *  guaranteed headliners (its featured events with a
 *  guarantee, the Main among them) from yesterday to tomorrow, and for the Main while its registration runs: the overlay its site added
 *  once registration closed, else the projection short or over while registering with someone in (site::show::OverlayOf: the
 *  guarantee against the pool, so added money is never an overlay); then the Main's registrations so far. "" when there's nothing. */
std::string HubTickerRun(const net::Network& Net, const HubFacts& H)
{
	const fest::Festival& F = *H.F;
	std::vector<std::string> TickItems;
	std::vector<std::string> TickSeen;
	if (F.Which == fest::Brand::Grit && (H.Ph == fest::Phase::Live || H.Ph == fest::Phase::MainDay) && H.Today >= F.FirstDay && H.Today <= F.LastDay)
	{
		// FH-2, U15's GRIT line: tonight's show and its two runs' starts (the first show running twice tonight).
		const std::vector<net::EventInstance> Tonight = Net.Window(HubMinute(H.Today), HubMinute(H.Today + 1));
		for (size_t A = 0; A < Tonight.size() && TickItems.empty(); ++A)
		{
			if (Net.TemplateOf(Tonight[A]).Series != F.SeriesId)
			{
				continue;
			}
			for (size_t B = A + 1; B < Tonight.size(); ++B)
			{
				if (Tonight[B].Template == Tonight[A].Template)
				{
					TickItems.push_back(NetUpper(H.Series->Short) + " \xC2\xB7 NIGHT " + std::to_string(H.Today - F.FirstDay + 1) + " OF " + std::to_string(H.DayCount) +
						" \xC2\xB7 SHOWS AT " + HubShowTime(Tonight[A].Start) + " AND " + HubShowTime(Tonight[B].Start));
					break;
				}
			}
		}
	}
	const auto TickAdd = [&](const net::EventInstance& E) {
		const net::EventTemplate& T = Net.TemplateOf(E);
		if (T.GtdCents <= 0 || std::find(TickSeen.begin(), TickSeen.end(), E.Id) != TickSeen.end())
		{
			return;
		}
		TickSeen.push_back(E.Id);
		const ::ss::site::show::OverlayView V = ::ss::site::show::OverlayOf(Net, E, H.World);
		if (!V.Shown)
		{
			return;
		}
		const std::string Name = rlnet_detail::NetShortName(T.Name);
		if (V.Actual)
		{
			if (V.Cents > 0)
			{
				TickItems.push_back(rlsites_detail::SitesShortName(F.Site) + " added " + NetMoney(V.Cents) + " to cover " + Name + "'s guarantee.");
			}
			return;
		}
		if (V.Cents > 0)
		{
			TickItems.push_back(Name + " is projected " + NetMoney(V.Cents) + " short.");
			return;
		}
		const double Each = static_cast<double>(T.BuyInCents) * Net.Definition().Rake.PrizeShare(T);
		const Chips Over = static_cast<Chips>(std::llround(V.Entries * Each)) - T.GtdCents;
		if (Over > 0)
		{
			TickItems.push_back(Name + " is projected " + NetMoney(Over) + " over.");
		}
	};
	const int From = std::max(F.FirstDay, H.Today - 1);
	const int To = std::min(F.LastDay, H.Today + 1);
	if (From <= To)
	{
		for (const net::EventInstance& E : Net.Window(HubMinute(From), HubMinute(To + 1)))
		{
			const net::EventTemplate& T = Net.TemplateOf(E);
			if (T.Series == F.SeriesId && T.Featured)
			{
				TickAdd(E);
			}
		}
	}
	if (H.HasMain)
	{
		TickAdd(H.Main);
		const ::ss::site::show::OverlayView V = ::ss::site::show::OverlayOf(Net, H.Main, H.World);
		if (!V.Actual && V.Registered > 0)
		{
			TickItems.push_back("MAIN EVENT \xC2\xB7 " + Grouped(V.Registered) + " registered");
		}
	}
	std::string TickRun;
	for (const std::string& It : TickItems)
	{
		TickRun += (TickRun.empty() ? "" : "   \xE2\x80\x94   ") + It;
	}
	return TickRun;
}

/** The overlay ticker (§2.8.2; FH-1b): one clipped run in its rect, rebuilt once a world minute; it slides when wider than its lane
 *  (a still in the preview). Ignite's hub has none (L7). */
void HubTicker(HubCtx& X, HubUi& U)
{
	if (!X.Fr.HasTicker)
	{
		return;
	}
	const long long Minute = static_cast<long long>(std::floor(X.H.World));
	if (U.TickerMinute != Minute)
	{
		U.Ticker = HubTickerRun(net::Site(X.H.F->Site), X.H);
		U.TickerMinute = Minute;
	}
	if (U.Ticker.empty())
	{
		return;
	}
	const Rect& T = X.Fr.Ticker;
	X.UI.RRect(T, 8.0f, rlsites_detail::SitesTone(0x0b1220), NetA(pal::Line, 0.7f));
	X.C.FillRoundRect({T.X + 10.0f, T.Y + 8.0f, 4.0f, T.H - 16.0f}, 2.0f, pal::Accent);
	const Rect Lane{T.X + 22.0f, T.Y, T.W - 30.0f, T.H};
	const float W = X.UI.Measure(U.Ticker, 13.0f, 600);
	const float Span = W + 80.0f;
	const bool Slide = W > Lane.W && !X.Still;
	const float Tx = Slide ? Lane.X - static_cast<float>(std::fmod(X.SinceOpen * 40.0, static_cast<double>(Span))) : Lane.X;
	const TextStyle Style = Ts(13.0f, 600, rlsites_detail::SitesTone(0xb9c4d6), Align::Left, Baseline::Middle);
	X.C.PushClip(Lane);
	X.UI.Text(U.Ticker, Tx, T.Y + T.H / 2.0f + 1.0f, Style);
	if (Slide && Tx + Span < Lane.X + Lane.W)
	{
		X.UI.Text(U.Ticker, Tx + Span, T.Y + T.H / 2.0f + 1.0f, Style);
	}
	X.C.PopClip();
}

/** The right column's tabs (`fstab0..2`, U5): THE RACE · CHAMPIONS · YOUR <FESTIVAL>, one shown at a time. Returns the tab shown. */
int HubTabs(HubCtx& X, HubUi& U)
{
	const Rect& T = X.Fr.Tabs;
	const std::string Names[3] = {"THE RACE", "CHAMPIONS", "YOUR " + NetUpper(X.H.Series->Short)};
	const int Shown = U.Tab >= 0 ? U.Tab : HubAutoTab(X.H);
	const float Gap = 6.0f;
	const float W = (T.W - 2.0f * Gap) / 3.0f;
	for (int I = 0; I < 3; ++I)
	{
		const Rect R{T.X + static_cast<float>(I) * (W + Gap), T.Y, W, T.H};
		const Ui::ClickState St = X.UI.Clickable("fstab" + std::to_string(I), R);
		if (St.Clicked && !X.Still)
		{
			U.Tab = I;
		}
		const bool On = I == Shown;
		X.UI.RRect(R, 10.0f, On ? NetA(pal::Accent, 0.2f) : St.Hover ? rlsites_detail::SitesTone(0x172a42) : rlsites_detail::SitesTone(0x101a2b),
			On ? pal::Accent : NetA(pal::Line, 0.9f));
		X.UI.Text(Names[I], R.X + R.W / 2.0f, R.Y + R.H / 2.0f + 0.5f, Ts(12.0f, 800, On ? pal::Ink : pal::Muted, Align::Center, Baseline::Middle, false, R.W - 16.0f));
	}
	return Shown;
}

/** A name in the right column (FH-4A): the lobby's PlayerName for the player and for a world person (a world npc id is the network's
 *  player index), after an r Ring avatar centred at (Ax, Base - 0.4 Size) when Ring > 0; a name the world no longer holds prints as
 *  the Honor kept it. */
void HubName(HubCtx& X, int Npc, const std::string& Kept, float Ax, float Nx, float Base, float Size, float MaxW, float Ring)
{
	const Session& S = Access::SessionOf(X.RL);
	const world::Npc* P = X.H.W && Npc >= 0 && Npc < static_cast<int>(net::Shared().Players().size()) ? X.H.W->Get(Npc) : nullptr;
	const bool Hero = Npc == -1;
	const std::string& Nm = Hero ? S.HeroName : P ? P->Name : Kept;
	uint8_t Frame = 0;
	if (Ring > 0.0f)
	{
		Frame = rlnet_detail::NetBadgeFrame(rlnet_detail::NetAvatar(X.C, Ax, Base - Size * 0.4f, Ring, Nm, Color{0.0f, 0.0f, 0.0f, 0.0f}, Hero));
	}
	if (Hero || P)
	{
		Access::PlayerName(X.RL, Npc, Nx, Base, Size, MaxW, Frame);
	}
	else
	{
		X.UI.Text(Nm, Nx, Base, Ts(Size, 700, pal::Ink, Align::Left, Baseline::Alphabetic, false, MaxW));
	}
}

/** THE RACE's marker for a mark band (§2.5.3; the legend draws the same five beside their labels): the belt gold, the podium silver,
 *  FINALIST the Accent and TOP 10% bronze, all filled, TOP 25% an outline; nothing for None. */
void HubMarkDot(Canvas& C, world::FestMark Mark, float Cx, float Cy, float R)
{
	switch (Mark)
	{
	case world::FestMark::Title: C.FillCircle(Cx, Cy, R, pal::Gold); break;
	case world::FestMark::Podium: C.FillCircle(Cx, Cy, R, Hex(0xc9d3e0)); break;
	case world::FestMark::Finalist: C.FillCircle(Cx, Cy, R, pal::Accent); break;
	case world::FestMark::Top10: C.FillCircle(Cx, Cy, R, Hex(0xd08c4f)); break;
	case world::FestMark::Top25: C.StrokeEllipse(Cx, Cy, R - 0.75f, R - 0.75f, pal::Muted, 1.5f); break;
	default: break;
	}
}

/** The movement since yesterday (§2.8.2, §2.11): the ASCII number right-aligned at Right, a vector triangle left of it (up, green; down,
 *  red), never a character. */
void HubMove(HubCtx& X, int Delta, float Right, float Base)
{
	const Color Col = Delta > 0 ? pal::Green : pal::Red;
	const float W = X.UI.Text(std::to_string(Delta > 0 ? Delta : -Delta), Right, Base, Ts(12.0f, 700, Col, Align::Right, Baseline::Alphabetic, true));
	const float Tx = Right - W - 9.0f;
	const float Ty = Base - 4.5f;
	if (Delta > 0)
	{
		X.C.FillPolygon({{Tx - 4.5f, Ty + 4.0f}, {Tx + 4.5f, Ty + 4.0f}, {Tx, Ty - 4.0f}}, Col);
	}
	else
	{
		X.C.FillPolygon({{Tx - 4.5f, Ty - 4.0f}, {Tx + 4.5f, Ty - 4.0f}, {Tx, Ty + 4.0f}}, Col);
	}
}

/** U8's seven rungs (§2.5.3) and the marks they stand for (None for PLAY 1 and CASH 1). */
const char* const HubRungNames[7] = {"PLAY 1", "CASH 1", "TOP 25%", "TOP 10%", "FINALIST", "PODIUM", "THE BELT"};
constexpr world::FestMark HubRungMarks[7] = {world::FestMark::None, world::FestMark::None, world::FestMark::Top25, world::FestMark::Top10,
	world::FestMark::Finalist, world::FestMark::Podium, world::FestMark::Title};

/** A mark's U8 label (the title's "THE BELT"); "" for None. */
const char* HubMarkName(world::FestMark Mark)
{
	for (int K = 2; K < 7; ++K)
	{
		if (HubRungMarks[K] == Mark)
		{
			return HubRungNames[K];
		}
	}
	return "";
}

/** THE RACE (§2.8.2, §2.5.2-§2.5.5; FH-4A): the title with EVERYONE / PEOPLE YOU KNOW (`fsrace0/1`, U5), the frame's rows at 40, the
 *  YOU row (U7) and the marks legend. Before the race opens: what it is and "No points yet." (U6); an edition on stage with no race:
 *  the old-save line (§2.5.5); after the crown, FestLast's final ten (FestTop) with no movement. */
void HubRace(HubCtx& X, HubUi& U)
{
	const HubFacts& H = X.H;
	const Rect& B = X.Fr.Body;
	const float Px = B.X + 22.0f;
	const float MaxW = B.W - 44.0f;
	const Color Soft = rlsites_detail::SitesTone(0xc3cedf);
	U.MovesShown = 0;
	X.UI.Text("PLAYER OF THE SERIES", Px, B.Y + 34.0f, Ts(12.0f, 800, pal::Muted));
	const bool Ahead = H.Ph == fest::Phase::Off || H.Ph == fest::Phase::Announced || H.Ph == fest::Phase::Buildup;
	float Y = B.Y + 66.0f;
	if (!H.RaceOpen && !Ahead)
	{
		// §2.5.5: the edition is on stage and the world holds no race for it (a save from before the update).
		rlnet_detail::NetParagraph(X.C, "This edition's race began before your update: no Player of the Series this time.", Px, Y, MaxW, 15.0f, 500, Soft, 22.0f);
		return;
	}
	if (Ahead || !H.Race || H.RaceRows == 0)
	{
		if (Ahead)
		{
			// Before the race opens on the edition's first day: what it is, then its empty state.
			Y = rlnet_detail::NetParagraph(X.C, "Every cash scores, yours and every regular's. First place takes the belt.", Px, Y, MaxW, 15.0f, 500, Soft, 22.0f) + 10.0f;
		}
		X.UI.Text("No points yet.", Px, Y, Ts(15.0f, 600, pal::Ink));
		return;
	}
	// EVERYONE / PEOPLE YOU KNOW, right of the title.
	const char* const Views[2] = {"EVERYONE", "PEOPLE YOU KNOW"};
	float Tx = B.X + B.W - 22.0f;
	for (int I = 1; I >= 0; --I)
	{
		const float Tw = X.UI.Measure(Views[I], 10.0f, 800) + 18.0f;
		Tx -= Tw;
		const Rect R{Tx, B.Y + 17.0f, Tw, 22.0f};
		const Ui::ClickState St = X.UI.Clickable("fsrace" + std::to_string(I), R);
		if (St.Clicked && !X.Still)
		{
			U.RaceView = I;
		}
		const bool On = U.RaceView == I;
		X.UI.RRect(R, 11.0f, On ? NetA(pal::Accent, 0.22f) : St.Hover ? rlsites_detail::SitesTone(0x172a42) : rlsites_detail::SitesTone(0x101a2b),
			On ? pal::Accent : NetA(pal::Line, 0.9f));
		X.UI.Text(Views[I], R.X + Tw / 2.0f, R.Y + 11.5f, Ts(10.0f, 800, On ? pal::Ink : pal::Muted, Align::Center, Baseline::Middle));
		Tx -= 6.0f;
	}
	// The rows: the race's top, or the people the player knows in race order with their rank in the whole race.
	struct HubRaceRow
	{
		int Npc = 0;
		int Rank = 0;
		double Points = -1.0; // -1: not in the race the world kept (FestLast holds ten)
	};
	const HubRaceMemo& M = *H.Race;
	std::vector<HubRaceRow> Rows;
	if (U.RaceView == 0)
	{
		for (size_t I = 0; I < M.All.size() && static_cast<int>(Rows.size()) < X.Fr.RaceRows; ++I)
		{
			Rows.push_back({M.All[I].first, static_cast<int>(I) + 1, M.All[I].second.Points});
		}
	}
	else
	{
		for (const std::pair<int, int>& K : H.Known)
		{
			if (static_cast<int>(Rows.size()) >= X.Fr.RaceRows)
			{
				break;
			}
			const bool In = K.second >= 1 && K.second <= static_cast<int>(M.All.size());
			Rows.push_back({K.first, K.second, In ? M.All[static_cast<size_t>(K.second - 1)].second.Points : -1.0});
		}
		if (Rows.empty())
		{
			X.UI.Text("No points yet.", Px, Y, Ts(15.0f, 600, pal::Ink));
		}
	}
	const bool Moves = !H.Crowned && M.PrevDay == H.Today;
	const float Right = B.X + B.W - 22.0f;
	for (size_t I = 0; I < Rows.size(); ++I)
	{
		const HubRaceRow& Rw = Rows[I];
		const float Top = X.Fr.RaceY + static_cast<float>(I) * 40.0f;
		const float Base = Top + 26.0f;
		if (Rw.Npc < 0)
		{
			X.UI.RRect({B.X + 12.0f, Top + 2.0f, B.W - 24.0f, 36.0f}, 8.0f, NetA(pal::Accent, 0.12f), NetA(pal::Accent, 0.5f));
		}
		const Color Medal = Rw.Rank == 1 ? pal::Gold : Rw.Rank == 2 ? Hex(0xc9d3e0) : Rw.Rank == 3 ? Hex(0xd08c4f) : pal::Muted;
		X.UI.Text(Grouped(Rw.Rank), Px + 34.0f, Base, Ts(14.0f, 800, Medal, Align::Right, Baseline::Alphabetic, true));
		HubMarkDot(X.C, world::MarkFor(Rw.Rank, H.Scorers), Px + 46.0f, Base - 5.0f, 4.0f);
		HubName(X, Rw.Npc, std::string(), Px + 70.0f, Px + 90.0f, Base, 14.0f, Right - 112.0f - (Px + 90.0f), 12.0f);
		if (Rw.Points >= 0.0)
		{
			X.UI.Text(Grouped(static_cast<int64_t>(std::llround(Rw.Points))), Right - 44.0f, Base, Ts(15.0f, 800, pal::Ink, Align::Right, Baseline::Alphabetic, true));
		}
		if (Moves)
		{
			const auto It = M.Prev.find(Rw.Npc);
			const int Delta = It == M.Prev.end() ? 0 : It->second - Rw.Rank;
			if (Delta != 0)
			{
				HubMove(X, Delta, Right, Base);
				++U.MovesShown;
			}
		}
	}
	// The YOU row (U7) at the body's foot, then the marks legend.
	if (H.HeroRank > 0)
	{
		const Rect R{B.X + 12.0f, X.Fr.YouY, B.W - 24.0f, 36.0f};
		X.UI.RRect(R, 8.0f, NetA(pal::Accent, 0.12f), NetA(pal::Accent, 0.6f));
		X.UI.Text("You \xC2\xB7 " + ss::Ordinal(H.HeroRank) + " of " + Grouped(H.Scorers) + " in the race \xC2\xB7 " + Grouped(static_cast<int64_t>(std::llround(H.HeroPoints))) + " pts",
			Px, R.Y + 24.0f, Ts(14.0f, 700, pal::Accent, Align::Left, Baseline::Alphabetic, false, MaxW));
	}
	float Lx = Px;
	for (int K = 2; K < 7; ++K)
	{
		HubMarkDot(X.C, HubRungMarks[K], Lx + 4.0f, X.Fr.LegendY - 4.0f, 4.0f);
		Lx += 12.0f + X.UI.Text(HubRungNames[K], Lx + 12.0f, X.Fr.LegendY, Ts(12.0f, 600, pal::Muted)) + 14.0f;
	}
}

/** A roll edition's height (§2.8.2, v1.6): a championship's (RCOP's included) four lines take 120 units, any other edition's two 88. */
float HubRollPitch(bool Championship)
{
	return Championship ? 120.0f : 88.0f;
}

/** CHAMPIONS (§2.6.2, §2.8.2; FH-4A): world::FestRoll's editions, newest first, as many as the body holds (six at most): the edition
 *  (14/800), then MAIN EVENT, a championship's MINI MAIN EVENT and MICRO MAIN EVENT (U34), and PLAYER OF THE SERIES, each with its name
 *  and figure (a title's prize in NetMoney and its field, M-4; the belt's points). A line whose Honor this world hasn't made yet (the
 *  crown before it lands) keeps its slot empty. U9's two honest lines while the roll is empty (FH-1a). */
void HubRoll(HubCtx& X)
{
	const HubFacts& H = X.H;
	const Rect& B = X.Fr.Body;
	const float Px = B.X + 22.0f;
	const float MaxW = B.W - 44.0f;
	if (H.RollEmpty)
	{
		const float Y = B.Y + 34.0f;
		X.UI.Text("No champion yet. The first name goes up on " + fest::DateInSentence(H.F->MainDay, H.Today) + ".", Px, Y, Ts(15.0f, 600, pal::Ink, Align::Left, Baseline::Alphabetic, false, MaxW));
		// The roll's books opened on Night One in every world (no festival edition was held before it, fest::All()).
		X.UI.Text("The record books opened " + net::DateLabel(net::NightOneDay) + ", " + std::to_string(world::YearOf(net::NightOneDay)) + ".", Px, Y + 26.0f,
			Ts(13.0f, 500, pal::Muted, Align::Left, Baseline::Alphabetic, false, MaxW));
		return;
	}
	const Color Soft = rlsites_detail::SitesTone(0xc3cedf);
	const float Right = B.X + B.W - 22.0f;
	float Y = B.Y + 12.0f;
	int Shown = 0;
	for (const world::FestRollRow& Row : H.Roll)
	{
		const fest::Festival* Ed = fest::FestivalOf(Row.Key);
		const bool Champ = Ed && fest::IsChampionship(Ed->Which);
		const float Pitch = HubRollPitch(Champ);
		if (Shown == 6 || Y + Pitch > B.Y + B.H)
		{
			break;
		}
		if (Shown > 0)
		{
			X.C.FillRect({Px, Y - 1.0f, MaxW, 1.0f}, NetA(pal::Line, 0.8f));
		}
		X.UI.Text(Ed ? Ed->Name : Row.Key, Px, Y + 22.0f, Ts(14.0f, 800, pal::Ink, Align::Left, Baseline::Alphabetic, false, MaxW));
		const std::pair<const char*, const world::Honor*> Lines[4] = {{"MAIN EVENT", Row.Main}, {"MINI MAIN EVENT", Row.Mini}, {"MICRO MAIN EVENT", Row.Micro},
			{"PLAYER OF THE SERIES", Row.Crown}};
		int Slot = 0;
		for (int K = 0; K < 4; ++K)
		{
			if (!Champ && (K == 1 || K == 2))
			{
				continue;
			}
			const float Base = Y + 45.0f + 24.0f * static_cast<float>(Slot++);
			const world::Honor* Hn = Lines[K].second;
			if (!Hn)
			{
				continue;
			}
			X.UI.Text(Lines[K].first, Px, Base, Ts(10.0f, 800, pal::Muted));
			const std::string Fig = K == 3 ? Grouped(Hn->Entries) + " pts" : NetMoney(Hn->Prize) + " \xC2\xB7 1st of " + Grouped(Hn->Entries);
			const float FigW = X.UI.Text(Fig, Right, Base, Ts(12.0f, 700, Soft, Align::Right, Baseline::Alphabetic, true));
			HubName(X, Hn->Npc, Hn->Name, 0.0f, Px + 140.0f, Base, 13.0f, std::max(40.0f, Right - FigW - 12.0f - (Px + 140.0f)), 0.0f);
		}
		Y += Pitch;
		++Shown;
	}
}

/** U10's line from fest::YourLine's view (§2.8.2; FH-4A): "{Events} events · {Cashes} cashes · best {Best} of {Field}", singular at
 *  one ("1 event · 1 cash"), then " · race {Rank} of {Scorers}" when Race and the hero scores. The YOUR panel draws it whole; the
 *  register sheets' strip (stream B's FH-4B) can call it without the race clause, which the strip prints at its right. Pure. */
std::string HubYourCounts(const fest::YourLineView& V, bool Race)
{
	std::string Out = (V.Events == 1 ? std::string("1 event") : Grouped(V.Events) + " events") + " \xC2\xB7 " + (V.Cashes == 1 ? std::string("1 cash") : Grouped(V.Cashes) + " cashes");
	if (V.Best > 0 && V.Field > 0)
	{
		Out += " \xC2\xB7 best " + ss::Ordinal(V.Best) + " of " + Grouped(V.Field);
	}
	if (Race && V.Rank > 0 && V.Scorers > 0)
	{
		Out += " \xC2\xB7 race " + ss::Ordinal(V.Rank) + " of " + Grouped(V.Scorers);
	}
	return Out;
}

/** U37's sentence (§2.8.2, LEAD_CALLS R-18; FH-4A): "Good on {Dates}." over a step ticket's days left, each in a sentence's form
 *  (fest::DateInSentence): "Good on Apr 7.", "Good on Apr 7 and Apr 9.", "Good on Apr 6, Apr 7 and Apr 9."; "" for none. Pure (u4_ui's
 *  FestCopyUi calls it). */
std::string HubGoodOn(const std::vector<int>& Days, int Today)
{
	if (Days.empty())
	{
		return std::string();
	}
	std::string Out = "Good on ";
	for (size_t I = 0; I < Days.size(); ++I)
	{
		if (I > 0)
		{
			Out += I + 1 == Days.size() ? " and " : ", ";
		}
		Out += fest::DateInSentence(Days[I], Today);
	}
	return Out + ".";
}

/** The days a step ticket of edition F is still good on (§2.2.3, U37): for each step ticket the wallet holds (a ladder satellite's
 *  SeatTicket that isn't the Main's), the days of the edition's one-off runs that take it and still take entries, in order. A recurring
 *  step (RCOP's step1-4, every four hours) shows none: its next run is always hours away. */
std::vector<int> HubStepDays(const Session& S, const net::Network& Net, const fest::Festival& F, const std::vector<std::string>& Ladder, double World)
{
	const std::string MainTicket = F.MainId.empty() ? std::string() : Net.TicketOf(F.MainId);
	std::vector<std::string> Seen;
	std::vector<int> Days;
	for (const std::string& Id : Ladder)
	{
		const net::EventTemplate* T = Net.FindTemplate(Id);
		if (!T || T->SeatTicket.empty() || T->SeatTicket == MainTicket || std::find(Seen.begin(), Seen.end(), T->SeatTicket) != Seen.end())
		{
			continue;
		}
		Seen.push_back(T->SeatTicket);
		if (S.Life.TicketsFor(T->SeatTicket) <= 0)
		{
			continue;
		}
		for (const std::string& Run : Ladder)
		{
			const net::EventTemplate* R = Net.FindTemplate(Run);
			net::EventInstance E;
			if (!R || R->OnlyDay == -9999 || Net.TicketOf(Run) != T->SeatTicket || !Net.Next(Run, HubMinute(F.FirstDay - fest::kAnnounceDays), E))
			{
				continue;
			}
			const net::Status St = Net.Live(E, World).St;
			if (St == net::Status::Announced || St == net::Status::Registering || St == net::Status::LateReg)
			{
				Days.push_back(net::DayOf(E.Start));
			}
		}
	}
	std::sort(Days.begin(), Days.end());
	Days.erase(std::unique(Days.begin(), Days.end()), Days.end());
	return Days;
}

/** YOUR <FESTIVAL> (§2.8.2, §2.5.3, §2.6.4; FH-4A): before the hero's first event of the edition, U11; then the pin with its mark's
 *  medal and U12's grades beside it, the trophy, a title object and the belt when won (F-look's art, cached), the ladder with U27's
 *  line (live) or the mark and its seat (after the crown), U10's line, U37 while a step ticket is held, and the results, newest first. */
void HubYour(HubCtx& X, HubUi& U)
{
	const HubFacts& H = X.H;
	const fest::Festival& F = *H.F;
	const Rect& B = X.Fr.Body;
	const float Px = B.X + 22.0f;
	const float MaxW = B.W - 44.0f;
	const Color Soft = rlsites_detail::SitesTone(0xc3cedf);
	if (!H.HasPin)
	{
		rlnet_detail::NetParagraph(X.C, "Play any " + fest::NameAfterArticle(F.Name) + " event for your pin.", Px, B.Y + 34.0f, MaxW, 15.0f, 500, Soft, 22.0f);
		return;
	}
	const net::Network& Net = net::Site(F.Site);
	const float Pin = X.Fr.PinSize;
	const float Top = B.Y + 18.0f;
	const world::FestMark Kept = H.Mark.Key == F.SeriesId ? H.Mark.Mark : world::FestMark::None;
	festart::FestPin(X.C, F.Which, H.Pin.Grade, Kept, Px + Pin / 2.0f, Top + Pin / 2.0f, Pin);
	// Beside the pin: the hero's points while they score, then U12's three grades with the pin's own lit.
	const float Rx = Px + Pin + 18.0f;
	float Ry = Top + 22.0f;
	if (H.RaceOpen && H.HeroRank > 0)
	{
		X.UI.Text(Grouped(static_cast<int64_t>(std::llround(H.HeroPoints))) + " pts", Rx, Ry, Ts(18.0f, 900, pal::Ink));
	}
	Ry += 22.0f;
	const char* const Grades[3] = {"GOLD \xC2\xB7 won an event", "SILVER \xC2\xB7 cashed", "OUTLINE \xC2\xB7 played"};
	for (int G = 0; G < 3; ++G)
	{
		X.UI.Text(Grades[G], Rx, Ry, Ts(11.0f, 700, H.Pin.Grade == 3 - G ? pal::Ink : pal::Muted));
		Ry += 16.0f;
	}
	// The trophy, a championship's title object and the belt when won, from the panel's right edge inwards.
	const double Time = X.Still ? 0.0 : X.SinceOpen;
	float Ax = B.X + B.W - 22.0f - 28.0f;
	for (const world::FestRollRow& Row : H.Roll)
	{
		if (Row.Key != F.SeriesId)
		{
			continue;
		}
		if (Row.Main && Row.Main->Npc == -1)
		{
			festart::FestTrophy(X.C, F.Which, Ax, Top + 30.0f, 56.0f, Time, false);
			Ax -= 64.0f;
		}
		if ((Row.Mini && Row.Mini->Npc == -1) || (Row.Micro && Row.Micro->Npc == -1))
		{
			festart::FestTrophy(X.C, F.Which, Ax, Top + 30.0f, 56.0f, Time, false, {}, 1);
			Ax -= 64.0f;
		}
		if (Row.Crown && Row.Crown->Npc == -1)
		{
			festart::FestBelt(X.C, F.Which, Ax, Top + 30.0f, 56.0f, Time, false);
		}
	}
	// The ladder (§2.5.3): the rungs reached filled (the race's mark right now while it runs, the kept mark after the crown), the next lit.
	const bool Live = H.RaceOpen && !H.Crowned;
	const world::FestMark Held = Live && H.HeroRank > 0 ? world::MarkFor(H.HeroRank, H.Scorers) : Kept;
	bool Reached[7];
	Reached[0] = true;
	Reached[1] = H.Your.Cashes > 0;
	for (int K = 2; K < 7; ++K)
	{
		Reached[K] = Held != world::FestMark::None && Held >= HubRungMarks[K];
	}
	int Next = -1;
	for (int K = 0; K < 7 && Next < 0; ++K)
	{
		Next = Reached[K] ? -1 : K;
	}
	const float Gap = 4.0f;
	const float Rw = std::min(64.0f, (MaxW - 6.0f * Gap) / 7.0f);
	const float Ly = Top + Pin + 14.0f;
	for (int K = 0; K < 7; ++K)
	{
		const Rect R{Px + static_cast<float>(K) * (Rw + Gap), Ly, Rw, 22.0f};
		const bool Lit = K == Next;
		X.UI.RRect(R, 6.0f, Reached[K] ? NetA(pal::Accent, 0.85f) : Lit ? NetA(pal::Accent, 0.16f) : rlsites_detail::SitesTone(0x101a2b),
			Reached[K] || Lit ? pal::Accent : NetA(pal::Line, 0.9f));
		X.UI.Text(HubRungNames[K], R.X + Rw / 2.0f, R.Y + 11.5f, Ts(9.0f, 800, Reached[K] ? pal::Bg : Lit ? pal::Ink : pal::Muted, Align::Center, Baseline::Middle, false, Rw - 6.0f));
	}
	float Y = Ly + 22.0f + 24.0f;
	if (Live && Next >= 2 && H.W)
	{
		// U27, from F-world's cut-off right now (fest::MarkCutoff − the hero's points + 1); none before the first cash (CASH 1 is lit).
		const int N = world::FestPointsTo(*H.W, HubRungMarks[Next]);
		if (N > 0)
		{
			X.UI.Text(Grouped(N) + " pts to " + HubRungNames[Next] + " \xC2\xB7 right now", Px, Y, Ts(13.0f, 700, pal::Accent, Align::Left, Baseline::Alphabetic, false, MaxW));
			Y += 24.0f;
		}
	}
	else if (!Live && Kept != world::FestMark::None)
	{
		// After the crown: the mark, and the seat a FINALIST or better earned (fest::kCrownTickets; U28's line).
		const float Mw = X.UI.Text(HubMarkName(Kept), Px, Y, Ts(15.0f, 900, pal::Gold));
		if (fest::kCrownTickets && Kept >= world::FestMark::Finalist)
		{
			X.UI.Text("A seat in the next Main Event", Px + Mw + 14.0f, Y, Ts(13.0f, 600, Soft, Align::Left, Baseline::Alphabetic, false, MaxW - Mw - 14.0f));
		}
		Y += 24.0f;
	}
	X.UI.Text(HubYourCounts(H.Your, true), Px, Y, Ts(13.0f, 600, Soft, Align::Left, Baseline::Alphabetic, false, MaxW));
	Y += 22.0f;
	if (!U.LadderBuilt)
	{
		U.Ladder = HubLadder(Net, F);
		U.Steps = HubSteps(Net, F);
		U.LadderBuilt = true;
	}
	const std::string GoodOn = HubGoodOn(HubStepDays(Access::SessionOf(X.RL), Net, F, U.Ladder, H.World), H.Today);
	if (!GoodOn.empty())
	{
		X.UI.Text(GoodOn, Px, Y, Ts(13.0f, 600, pal::Accent, Align::Left, Baseline::Alphabetic, false, MaxW));
		Y += 22.0f;
	}
	// The hero's results in the edition, newest first (the frame's count).
	Y += 6.0f;
	int Rows = 0;
	for (const world::HeroFestResult& R : H.Results)
	{
		if (Rows == X.Fr.YourResults || Y + 30.0f > B.Y + B.H)
		{
			break;
		}
		X.C.FillRect({Px, Y, MaxW, 1.0f}, NetA(pal::Line, 0.7f));
		const net::EventTemplate* T = Net.FindTemplate(R.EventId.substr(0, R.EventId.find('@')));
		const std::string Place = ss::Ordinal(R.Place) + (R.Entries > 0 ? " of " + Grouped(R.Entries) : std::string());
		const float Pw = R.Prize > 0 ? X.UI.Text(NetMoney(R.Prize), Px + MaxW, Y + 21.0f, Ts(13.0f, 800, pal::Accent, Align::Right, Baseline::Alphabetic, true)) + 14.0f : 0.0f;
		const float Lw = X.UI.Text(Place, Px + MaxW - Pw, Y + 21.0f, Ts(13.0f, 600, Soft, Align::Right, Baseline::Alphabetic, true)) + 14.0f;
		X.UI.Text(T ? rlnet_detail::NetShortName(T->Name) : R.EventId, Px, Y + 21.0f, Ts(13.0f, 700, pal::Ink, Align::Left, Baseline::Alphabetic, false, std::max(40.0f, MaxW - Pw - Lw)));
		Y += 30.0f;
		++Rows;
	}
}

/** The right column's body for the tab shown (§2.8.2): THE RACE, CHAMPIONS or YOUR <FESTIVAL>, each in its empty state until its view
 *  holds something (U6, U9, U11). */
void HubBody(HubCtx& X, HubUi& U, int Tab)
{
	X.UI.RRect(X.Fr.Body, 14.0f, rlsites_detail::SitesTone(0x0e1726), NetA(pal::Line, 0.9f));
	switch (Tab)
	{
	case 0: HubRace(X, U); break;
	case 1: HubRoll(X); break;
	default: HubYour(X, U); break;
	}
}

/** u4_ui (FH-5b, FestPreviewStill): the laptop drawn as the stream preview draws it (Previewing: no input, no side effects). */
void HubPreviewing(RiverLine& RL, bool On)
{
	Access::SetPreviewing(RL, On);
}

/** u4_ui (FH-4A): the movement marks THE RACE drew in the laptop's last hub frame (0 before any). */
int HubRaceMoves(const RiverLine& RL)
{
	const auto It = HubStates().find(&RL);
	return It == HubStates().end() ? 0 : It->second.MovesShown;
}

/** The popover's phase line (§2.8.6, U16): "Opens {Date}." before the window, "Day {N} of {Days}." on stage, "Crowned {Date}." in
 *  the Afterglow (the crown's day, else the edition's last finish with no world). */
std::string HubPhaseLine(const HubYearCell& Cl, int Today)
{
	const fest::Festival& F = *Cl.F;
	switch (Cl.Ph)
	{
	case fest::Phase::Live:
	case fest::Phase::MainDay:
	case fest::Phase::Closing:
	{
		const int Days = F.LastDay - F.FirstDay + 1;
		return "Day " + std::to_string(std::max(1, std::min(Days, Today - F.FirstDay + 1))) + " of " + std::to_string(Days) + ".";
	}
	case fest::Phase::Afterglow:
		return "Crowned " + fest::DateInSentence(net::DayOf(Cl.CrownedAt >= 0.0 ? Cl.CrownedAt : fest::LastEnd(F)), Today) + ".";
	default: break;
	}
	return "Opens " + fest::DateInSentence(F.FirstDay, Today) + ".";
}

/** The strip's popover (§2.8.6; FH-1b), drawn by FestivalOverlays over the hub that opened it, in the client's skin: the crest at 46,
 *  the name and its full title (12/600 Muted, N17-N19), the host and the window, the phase line, "Your account is open." or "By
 *  invitation." (U16), or the Pass's words (U30, FH-2: "Festival Pass: {N} of {Need} online events.", "Opens to you after your
 *  second rent."), and one button (U17): OPEN THE HUB when the edition's host is the site in front
 *  (the edition's hub), GO TO {SITE} for another site's edition with an account open (the site tabs' path: RiverLine::SiteShown, then
 *  that site's SERIES page on the edition); none for a rival festival without an account. The hub held the pointer under it
 *  (FestivalHub); the frame it opens in takes no input. */
void HubPopover(RiverLine& RL, Canvas& C, Ui& UI, HubUi& U, double Now)
{
	HubUi::Pop& P = U.Popover;
	const HubYearCell Cl = P.C;
	if (!Cl.F || !Cl.Series)
	{
		P = HubUi::Pop();
		return;
	}
	const Session& S = Access::SessionOf(RL);
	const net::SiteId Front = rlsites_detail::SitesClient(RL, S);
	const SkinScope Worn(rlsites_detail::SitesSkin(Front, Access::WorldOf(RL)));
	const Rect R = P.R;
	const bool TakesInput = !P.Fresh;
	P.Fresh = false;
	UI.RRect(R, 14.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, rlsites_detail::SitesTone(0x131c2b), rlsites_detail::SitesTone(0x0b1220)), NetA(pal::Accent, 0.6f));
	eventart::SeriesCrest(C, *Cl.Series, R.X + 40.0f, R.Y + 42.0f, 46.0f, 0.0);
	const float Tx = R.X + 76.0f;
	const float Tw = R.W - 92.0f;
	UI.Text(Cl.F->Name, Tx, R.Y + 36.0f, Ts(18.0f, 800, pal::Ink, Align::Left, Baseline::Alphabetic, false, Tw));
	float Ly = R.Y + 54.0f;
	if (!Cl.Series->Title.empty())
	{
		UI.Text(Cl.Series->Title, Tx, Ly, Ts(12.0f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, false, Tw));
		Ly += 16.0f;
	}
	UI.Text(rlsites_detail::SitesShortName(Cl.F->Site) + " \xC2\xB7 " + net::DateLabel(Cl.F->FirstDay) + " \xE2\x80\x93 " + net::DateLabel(Cl.F->LastDay), Tx, Ly,
		Ts(12.0f, 600, rlsites_detail::SitesTone(0xc3cedf), Align::Left, Baseline::Alphabetic, false, Tw));
	UI.Text(HubPhaseLine(Cl, P.Today), R.X + 18.0f, R.Y + 102.0f, Ts(14.0f, 700, pal::Ink, Align::Left, Baseline::Alphabetic, false, R.W - 36.0f));
	// FH-2, U30: the Pass's progress in words on a rival edition without an account (no button there, so the line takes the width).
	const std::string AccountLine = Cl.Account ? std::string("Your account is open.")
		: Cl.PassForm == 2                     ? "Festival Pass: " + std::to_string(Cl.PassN) + " of " + std::to_string(Cl.PassNeed) + " online events."
		: Cl.PassForm == 1                     ? std::string("Opens to you after your second rent.")
											   : std::string("By invitation.");
	UI.Text(AccountLine, R.X + 18.0f, R.Y + 122.0f, Ts(13.0f, 500, Cl.Account ? pal::Green : Cl.PassForm == 2 ? pal::Accent : pal::Muted, Align::Left, Baseline::Alphabetic, false,
		Cl.Account ? 160.0f : R.W - 36.0f));
	if (!Cl.Account)
	{
		return;
	}
	const bool Here = Cl.F->Site == Front;
	ButtonOpts O;
	O.Size = 13.0f;
	const Rect B{R.X + R.W - 172.0f, R.Y + R.H - 44.0f, 156.0f, 32.0f};
	if (rlsites_detail::SitesButton(UI, C, "fspopgo", B, Here ? std::string("OPEN THE HUB") : "GO TO " + rlsites_detail::SitesUpperName(Cl.F->Site), O, rlsites_detail::SitesLook(Front)) &&
		TakesInput)
	{
		if (!Here)
		{
			RL.SiteShown = Cl.F->Site;
			Access::OpenPage(RL, RiverLine::Page::Series, Now);
		}
		RL.ShowSeries(Cl.F->SeriesId, Now);
		P = HubUi::Pop();
	}
}

// ---- FH-6a: the stings (§2.7.6), the moments (§2.8.5) and the pin wall (§2.8.7).

/** Q20 (PLAN_FHUBS): U29's shelf line prints while the 3D shelf ships; true at G1, the lead flips it if cut 5 is taken. */
constexpr bool kHubShelfShips = true;

/** §2.7.6: the host's sting on the first live hub open of a world day per festival (this laptop's memory); never in a still. */
void HubOpenSting(HubCtx& X, HubUi& U)
{
	if (X.Still || U.StingChecked)
	{
		return;
	}
	U.StingChecked = true;
	HubCues& Q = HubCuesOf(X.RL);
	const int Day = net::DayOf(X.H.World);
	const auto It = Q.StingDay.find(X.H.F->SeriesId);
	if (It != Q.StingDay.end() && It->second == Day)
	{
		return;
	}
	Q.StingDay[X.H.F->SeriesId] = Day;
	U.StingAt = X.Now;
	Access::Sound(X.RL, HubStingOf(X.Look, X.H.F->Site), 0.5f);
}

/** The festival edition an event instance belongs to (its template's series), or null. */
const fest::Festival* HubEventFestival(const std::string& EventId)
{
	const size_t At = EventId.find('@');
	if (At == std::string::npos)
	{
		return nullptr;
	}
	const net::EventTemplate* T = net::ForEvent(EventId).FindTemplate(EventId.substr(0, At));
	return T ? fest::FestivalOf(T->Series) : nullptr;
}

/** §2.7.6: a festival registration's sting, once, when its press is confirmed and core accepts it: a new open table whose event is a
 *  festival's, whichever register button made it (the hub's rows, RegisterBlock, IgniteSheet, RitProspectus). The tables are looked at
 *  only when their count moves; a laptop's first look takes the tables it finds as heard. */
void HubSeatSting(Session& S, HubCues& Q)
{
	const int N = S.TableCount();
	if (N == Q.Tables)
	{
		return;
	}
	std::vector<std::string> Seats;
	for (int I = 0; I < N; ++I)
	{
		const TableGlance G = S.Glance(I);
		if (HubEventFestival(G.EventId))
		{
			Seats.push_back(G.EventId);
		}
	}
	for (const std::string& Id : Seats)
	{
		if (Q.Tables >= 0 && std::find(Q.Seats.begin(), Q.Seats.end(), Id) == Q.Seats.end())
		{
			const fest::Festival* F = HubEventFestival(Id);
			S.UiSound(HubStingOf(festart::LookOf(F->Which), F->Site), 0.5f);
			break;
		}
	}
	Q.Seats = std::move(Seats);
	Q.Tables = N;
}

/** Whether the laptop shows a moment now (§2.8.5): over the poker client in front, at the lobby or a result; never at a table, at the
 *  title screen, under a time skip or an outcome card. */
bool HubMomentRoom(const RiverLine& RL, const Session& S)
{
	return RL.CurrentApp() == RiverLine::App::RiverLine && S.CurrentScreen != Screen::Boot && S.CurrentScreen != Screen::Table && !S.TimeSkip.Active &&
		!S.HasOutcome;
}

/** The moment's identity on this laptop: a new one restarts its clocks and its cue. */
std::string HubMomentId(const FestMoment& M)
{
	return std::to_string(M.Kind) + ":" + M.Key + (M.Dev ? ":dev" : "");
}

/** A small FNV-1a over the moment's id: confetti's paths (presentation only, §2.12). */
uint32_t HubMomentHash(const std::string& Text)
{
	uint32_t H = 2166136261u;
	for (const char Ch : Text)
	{
		H ^= static_cast<uint8_t>(Ch);
		H *= 16777619u;
	}
	return H;
}

/** U8's word for a mark (world::FestMark as FestMoment::Mark carries it). */
const char* HubMarkWord(int Mark)
{
	switch (static_cast<world::FestMark>(Mark))
	{
	case world::FestMark::Top25: return "TOP 25%";
	case world::FestMark::Top10: return "TOP 10%";
	case world::FestMark::Finalist: return "FINALIST";
	case world::FestMark::Podium: return "PODIUM";
	case world::FestMark::Title: return "THE BELT";
	default: break;
	}
	return "";
}

/** U36: the edition's place among this world's editions of its brand (from Night One on, this one included). */
int HubWorldEdition(const fest::Festival& F)
{
	int N = 0;
	for (const fest::Festival& G : fest::All())
	{
		N += G.Which == F.Which && G.FirstDay >= 0 && G.FirstDay <= F.FirstDay ? 1 : 0;
	}
	return N;
}

/** U18's line under the card's title: a Main's ("{Festival} Main Event", U36's edition from a championship's second in this world),
 *  the belt's and a title's edition, another festival event's name (the win on the results screen; a DEV moment's is the edition's
 *  first numbered event, as ss.Fest.Moment's prize reads it; else the edition). */
std::string HubMomentLine(const Session& S, const FestMoment& M, const fest::Festival& F)
{
	switch (M.Kind)
	{
	case 1:
	{
		const int N = HubWorldEdition(F);
		return F.Name + " Main Event" + (fest::IsChampionship(F.Which) && N >= 2 ? " \xC2\xB7 " + ss::Ordinal(N) + " edition" : std::string());
	}
	case 3:
		if (M.Dev)
		{
			const net::Network& Net = net::ForEvent(F.MainId);
			for (const net::EventInstance& E : Net.Window(static_cast<double>(F.FirstDay) * net::MinutesPerDay, static_cast<double>(F.LastDay + 1) * net::MinutesPerDay))
			{
				const net::EventTemplate& T = Net.TemplateOf(E);
				if (T.Series == F.SeriesId && std::find(F.Satellites.begin(), F.Satellites.end(), T.Id) == F.Satellites.end())
				{
					return Net.NameOf(E);
				}
			}
		}
		return S.HasResults && S.LastResults.Won && !S.LastResults.EventName.empty() ? S.LastResults.EventName : F.Name;
	case 5: return F.Name + " \xC2\xB7 " + (M.Mark == 2 ? "Micro" : "Mini") + " Main Event";
	default: break;
	}
	return F.Name;
}

/** Kinds 1, 2, 3 and 5's card {500, 170, 600, 580} (§2.8.5), Tm seconds after its first live frame (a still: settled). */
void HubChampionCard(Canvas& C, Ui& UI, const Session& S, const FestMoment& M, const fest::Festival& F, double Tm, bool Still, double Now)
{
	const Rect K{500.0f, 170.0f, 600.0f, 580.0f};
	const float Cx = K.X + K.W / 2.0f;
	C.FillRoundRect(K, 18.0f, Paint::Linear({0.0f, K.Y}, {0.0f, K.Y + K.H}, pal::Bg2, pal::Bg));
	C.StrokeRoundRect(K, 18.0f, pal::Accent, 2.0f);
	const float A0 = C.GetAlpha();
	// The crest slams in (Spring, 0-0.5 s).
	const double Slam = Still ? 1.0 : motion::Spring(Tm, 0.0);
	const net::SeriesInfo* Series = net::Site(F.Site).FindSeries(F.SeriesId);
	if (Series && Slam > 0.02)
	{
		eventart::SeriesCrest(C, *Series, Cx, K.Y + 70.0f, static_cast<float>(84.0 * Slam), Still ? 0.0 : Now);
	}
	// CHAMPION (PLAYER OF THE SERIES for the belt) staged from 0.4 s to 1.0 s, with U18's line.
	const float Staged = Still ? 1.0f : static_cast<float>(std::clamp((Tm - 0.4) / 0.6, 0.0, 1.0));
	if (Staged > 0.0f)
	{
		C.SetAlpha(A0 * Staged);
		UI.Text(M.Kind == 2 ? "PLAYER OF THE SERIES" : "CHAMPION", Cx, K.Y + 166.0f, Ts(M.Kind == 2 ? 34.0f : 44.0f, 900, pal::Gold, Align::Center));
		UI.Text(HubMomentLine(S, M, F), Cx, K.Y + 198.0f, Ts(18.0f, 700, pal::Ink, Align::Center, Baseline::Alphabetic, false, K.W - 48.0f));
		C.SetAlpha(A0);
	}
	// The trophy (260 for a championship's, the RCOP Cup included), the belt, the gold pin or the title object rises (0.8-1.6 s).
	const double Rise = Still ? 1.0 : std::clamp((Tm - 0.8) / 0.8, 0.0, 1.0);
	if (Rise > 0.0)
	{
		const float E = static_cast<float>(1.0 - (1.0 - Rise) * (1.0 - Rise));
		const float Oy = K.Y + 318.0f + (1.0f - E) * 50.0f;
		const double T = Still ? 0.0 : Tm;
		C.SetAlpha(A0 * E);
		if (M.Kind == 1)
		{
			festart::FestTrophy(C, F.Which, Cx, Oy, fest::IsChampionship(F.Which) ? 260.0f : 220.0f, T, false);
		}
		else if (M.Kind == 2)
		{
			festart::FestBelt(C, F.Which, Cx, Oy, 220.0f, T, false);
		}
		else if (M.Kind == 5 && F.Which != fest::Brand::Rcop)
		{
			festart::FestTrophy(C, F.Which, Cx, Oy, 220.0f, T, false, {}, 1);
		}
		else
		{
			festart::FestPin(C, F.Which, 3, world::FestMark::None, Cx, Oy, 150.0f); // Kind 3, and RCOP's title (no title object)
		}
		C.SetAlpha(A0);
	}
	// The prize counts up (the engine's number; the belt shows its points), DEV beside a dev moment's.
	const double Shown = Still ? static_cast<double>(M.Prize) : motion::CountUp(0.0, static_cast<double>(M.Prize), Tm, 1.0, 1.0);
	const Chips V = static_cast<Chips>(std::llround(Shown));
	const std::string Figure = M.Kind == 2 ? Grouped(V) + " pts" : NetMoney(V);
	const float Fw = UI.Text(Figure, Cx, K.Y + 486.0f, Ts(34.0f, 900, pal::Gold, Align::Center, Baseline::Alphabetic, true));
	if (M.Dev)
	{
		UI.Text("DEV", Cx + Fw / 2.0f + 12.0f, K.Y + 486.0f, Ts(13.0f, 800, pal::Accent));
	}
}

/** Kind 4's card {560, 250, 480, 420} (§2.8.5, M2b): the edition's pin rises, the mark's medal drops onto it (Spring), the mark, the
 *  edition, "{Rank} of {Scorers} in the race" (U28) and the crown ticket's line when one was earned (FINALIST or PODIUM). */
void HubMarkCard(Canvas& C, Ui& UI, const Session& S, const FestMoment& M, const fest::Festival& F, double Tm, bool Still)
{
	const Rect K{560.0f, 250.0f, 480.0f, 420.0f};
	const float Cx = K.X + K.W / 2.0f;
	C.FillRoundRect(K, 16.0f, Paint::Linear({0.0f, K.Y}, {0.0f, K.Y + K.H}, pal::Bg2, pal::Bg));
	C.StrokeRoundRect(K, 16.0f, NetA(pal::Line, 0.9f), 1.5f);
	int Grade = 1;
	for (const world::HeroFestPin& P : world::HeroFestPins(S.Living()))
	{
		Grade = P.Key == M.Key ? std::max(1, P.Grade) : Grade;
	}
	const float A0 = C.GetAlpha();
	const double Rise = Still ? 1.0 : std::clamp(Tm / 0.6, 0.0, 1.0);
	const float E = static_cast<float>(1.0 - (1.0 - Rise) * (1.0 - Rise));
	const bool Medal = Still || Tm >= 0.6;
	const float Drop = !Medal ? 0.0f : Still ? 1.0f : static_cast<float>(motion::Spring(Tm, 0.6));
	C.SetAlpha(A0 * E);
	festart::FestPin(C, F.Which, Grade, Medal ? static_cast<world::FestMark>(M.Mark) : world::FestMark::None, Cx,
		K.Y + 120.0f + (1.0f - E) * 40.0f - (Medal ? (1.0f - Drop) * 16.0f : 0.0f), 110.0f);
	C.SetAlpha(A0);
	UI.Text(HubMarkWord(M.Mark), Cx, K.Y + 238.0f, Ts(30.0f, 900, pal::Gold, Align::Center));
	UI.Text(F.Name, Cx, K.Y + 268.0f, Ts(16.0f, 700, pal::Ink, Align::Center, Baseline::Alphabetic, false, K.W - 40.0f));
	const std::string Rank = ss::Ordinal(M.Rank) + " of " + Grouped(M.Scorers) + " in the race";
	const float Rw = UI.Text(Rank, Cx, K.Y + 294.0f, Ts(14.0f, 600, pal::Muted, Align::Center));
	if (M.Dev)
	{
		UI.Text("DEV", Cx + Rw / 2.0f + 10.0f, K.Y + 294.0f, Ts(12.0f, 800, pal::Accent));
	}
	const world::FestMark Mk = static_cast<world::FestMark>(M.Mark);
	if (fest::kCrownTickets && (Mk == world::FestMark::Finalist || Mk == world::FestMark::Podium))
	{
		UI.Text("A seat in the next Main Event", Cx, K.Y + 324.0f, Ts(14.0f, 700, pal::Green, Align::Center));
	}
}

/** u4_ui and RiverLine::Draw (FH-6a; the lead's line before the app's frame, patch fhubs_to_lead_4): a moment about to draw takes the
 *  pointer over its scrim (SitesBlockUnder), so nothing beneath sees this frame's press, release or wheel, its first frame included;
 *  FestivalOverlays hands it back for the moment's own clicks. Once a frame (a second call in the frame does nothing); never while
 *  previewing. */
void FestMomentHold(RiverLine& RL)
{
	HubCues& Q = HubCuesOf(RL);
	const Session& S = Access::SessionOf(RL);
	if (Q.Held || Access::Previewing(RL) || !HubMomentRoom(RL, S))
	{
		return;
	}
	const std::optional<FestMoment> M = S.PeekFestMoment();
	if (!M || !fest::FestivalOf(M->Key))
	{
		return;
	}
	Q.Saved = rlsites_detail::SitesBlockUnder(RL.UI, {0.0f, 0.0f, 1600.0f, 962.0f}, false);
	Q.Held = Q.Saved.Active;
}

/** The moment on screen (§2.8.5), drawn by FestivalOverlays over the client's frame: the scrim, the card, its cue once, and the close
 *  (TAKE A BOW or a click anywhere on the scrim after 1.6 s; Session::DoneFestMoment). A still while previewing: the settled card, no
 *  cue, no input, nothing granted, nothing in the side state. */
void HubMoment(RiverLine& RL, Canvas& C, Ui& UI, double World, bool Still, double Now)
{
	HubCues& Q = HubCuesOf(RL);
	if (Q.Held)
	{
		rlsites_detail::SitesUnblock(UI, Q.Saved);
		Q.Held = false;
	}
	Session& S = Access::SessionMut(RL);
	const std::optional<FestMoment> M = HubMomentRoom(RL, S) ? S.PeekFestMoment() : std::nullopt;
	const fest::Festival* F = M ? fest::FestivalOf(M->Key) : nullptr;
	if (!F)
	{
		if (!Still)
		{
			Q.MomentId.clear();
		}
		return;
	}
	double Tm = 30.0;
	if (!Still)
	{
		const std::string Id = HubMomentId(*M);
		if (Id != Q.MomentId)
		{
			Q.MomentId = Id;
			Q.MomentAt = Now;
			Q.Sung = false;
		}
		Tm = Now - Q.MomentAt;
		if (!Q.Sung)
		{
			// One cue a moment (§2.7.6): FestChampion with CrowdApplause under it, the mark's pencil-weight Bubble.
			Q.Sung = true;
			if (M->Kind == 4)
			{
				S.UiSound(SoundId::Bubble, 0.4f);
			}
			else
			{
				S.UiSound(SoundId::FestChampion, 0.6f);
				S.UiSound(SoundId::CrowdApplause, 0.4f);
			}
		}
	}
	const SkinScope Worn(festart::Dressed(rlsites_detail::SitesSkin(F->Site, World), festart::LookOf(F->Which)));
	const Rect Scrim{0.0f, 0.0f, 1600.0f, 962.0f};
	C.FillRect(Scrim, NetA(pal::Bg, 0.82f));
	const bool Ready = Still || Tm >= 1.6;
	// The scrim takes every press (a press beneath never completes); after 1.6 s a click anywhere on it closes the moment.
	bool Close = !Still && UI.Clickable(Ready ? "fsmomentany" : "fsmomentwait", Scrim).Clicked && Ready;
	if (M->Kind == 4)
	{
		HubMarkCard(C, UI, S, *M, *F, Tm, Still);
		if (Ready)
		{
			UI.Text("Click anywhere to continue.", 800.0f, 644.0f, Ts(13.0f, 600, pal::Muted, Align::Center));
		}
	}
	else
	{
		if (!Still)
		{
			motion::Confetti(C, HubMomentHash(Q.MomentId), Q.MomentAt, Now, 160, Scrim);
		}
		HubChampionCard(C, UI, S, *M, *F, Tm, Still, Now);
		if (Ready)
		{
			ButtonOpts O;
			O.Kind = ButtonKind::Gold;
			O.Size = 18.0f;
			const bool Bow = rlsites_detail::SitesButton(UI, C, "fsmoment", {700.0f, 680.0f, 200.0f, 46.0f}, "TAKE A BOW", O);
			Close = Close || (Bow && !Still);
			UI.Text("Click anywhere to continue.", 800.0f, 912.0f, Ts(13.0f, 600, pal::Muted, Align::Center));
		}
		// Beneath a Main's card (U29): the shelf line while the shelf ships, and the belt line while the edition's race is uncrowned.
		float Y = 788.0f;
		if (M->Kind == 1 && kHubShelfShips)
		{
			UI.Text("The trophy goes on the shelf at home.", 800.0f, Y, Ts(15.0f, 600, pal::Ink, Align::Center));
			Y += 26.0f;
		}
		const world::World& W = S.Living();
		if (M->Kind == 1 && world::FestRaceKey(W) == F->SeriesId && world::FestCrownedAt(W, F->SeriesId) < 0.0)
		{
			UI.Text("The belt is decided at the crown, later tonight.", 800.0f, Y, Ts(15.0f, 600, pal::Muted, Align::Center));
		}
	}
	if (Close)
	{
		S.DoneFestMoment();
		Q.MomentId.clear();
	}
}

/** The pin wall (§2.8.7) in R ({1092, 656, 484, 104} on RiverLine's Career page): hidden until the hero holds a festival pin. */
void HubPins(RiverLine& RL, Canvas& C, Ui& UI, const Session& S, const Rect& R, bool Still)
{
	const world::World& W = S.Living();
	const std::vector<world::HeroFestPin> Pins = world::HeroFestPins(W);
	std::vector<int> Years;
	for (const world::HeroFestPin& P : Pins)
	{
		if (const fest::Festival* F = fest::FestivalOf(P.Key))
		{
			Years.push_back(F->Year);
		}
	}
	if (Years.empty())
	{
		return;
	}
	std::sort(Years.begin(), Years.end());
	Years.erase(std::unique(Years.begin(), Years.end()), Years.end());
	HubCues& Q = HubCuesOf(RL);
	const auto Found = std::find(Years.begin(), Years.end(), Q.PinsYear);
	const size_t At = Found == Years.end() ? Years.size() - 1 : static_cast<size_t>(Found - Years.begin());
	const int Year = Years[At];
	UI.Text("FESTIVAL PINS \xC2\xB7 " + std::to_string(Year), R.X + 12.0f, R.Y + 26.0f, Ts(13.0f, 800, pal::Ink));
	// ‹ › over the years holding a pin, drawn as vector marks.
	const Rect Prev{R.X + R.W - 68.0f, R.Y + 8.0f, 26.0f, 26.0f};
	const Rect Next{R.X + R.W - 36.0f, R.Y + 8.0f, 26.0f, 26.0f};
	const bool HasPrev = At > 0;
	const bool HasNext = At + 1 < Years.size();
	for (int Side = 0; Side < 2; ++Side)
	{
		const Rect& B = Side == 0 ? Prev : Next;
		const float Mx = B.X + B.W / 2.0f;
		const float My = B.Y + B.H / 2.0f;
		const float D = Side == 0 ? 3.0f : -3.0f;
		C.StrokePolyline({{Mx + D, My - 6.0f}, {Mx - D, My}, {Mx + D, My + 6.0f}}, false, (Side == 0 ? HasPrev : HasNext) ? pal::Ink : NetA(pal::Muted, 0.45f), 2.0f, true);
	}
	if (!Still)
	{
		if (UI.Clickable("fpy-", Prev, HasPrev).Clicked)
		{
			Q.PinsYear = Years[At - 1];
		}
		if (UI.Clickable("fpy+", Next, HasNext).Clicked)
		{
			Q.PinsYear = Years[At + 1];
		}
	}
	// Ten slots of 40 in the year's calendar order (WRIT ... FEIGN).
	const char* const Grades[3] = {"OUTLINE \xC2\xB7 played", "SILVER \xC2\xB7 cashed", "GOLD \xC2\xB7 won an event"};
	std::string Facts;
	for (int I = 0; I < 10; ++I)
	{
		const fest::Brand B = HubYearOrder[I];
		const Rect Slot{R.X + 12.0f + 47.0f * static_cast<float>(I), R.Y + 44.0f, 40.0f, 40.0f};
		const float Cx = Slot.X + 20.0f;
		const float Cy = Slot.Y + 20.0f;
		const fest::Festival* F = nullptr;
		for (const fest::Festival& G : fest::All())
		{
			F = !F && G.Which == B && G.Year == Year ? &G : F;
		}
		const world::HeroFestPin* Pin = nullptr;
		for (const world::HeroFestPin& P : Pins)
		{
			Pin = F && P.Key == F->SeriesId ? &P : Pin;
		}
		if (Pin)
		{
			festart::FestPin(C, B, Pin->Grade, world::HeroMark(W, F->SeriesId), Cx, Cy, 40.0f);
		}
		else if (F && F->FirstDay >= 0)
		{
			C.FillCircle(Cx, Cy, 16.0f, NetA(pal::Bg, 0.7f)); // an empty socket: an edition this world holds, skipped
			C.StrokeEllipse(Cx, Cy, 17.0f, 17.0f, NetA(pal::Line, 0.9f), 1.5f);
		}
		else
		{
			C.FillRect({Cx - 8.0f, Cy - 1.0f, 16.0f, 2.0f}, NetA(pal::Muted, 0.35f)); // a faint dash: never held in this world
		}
		if (!Still && F && UI.Hover(Slot))
		{
			if (Pin)
			{
				fest::YourLineView V;
				V.Events = Pin->Events;
				V.Cashes = Pin->Cashes;
				V.Best = Pin->BestPlace;
				V.Field = Pin->BestEntries;
				for (const world::HeroFestMark& K : world::HeroMarks(W))
				{
					if (K.Key == F->SeriesId)
					{
						V.Rank = K.Rank;
						V.Scorers = K.Scorers;
					}
				}
				Facts = F->Name + " \xC2\xB7 " + Grades[std::clamp(Pin->Grade, 1, 3) - 1] + " \xC2\xB7 " + HubYourCounts(V, V.Rank > 0);
			}
			else
			{
				Facts = F->Name;
			}
		}
	}
	if (!Facts.empty())
	{
		UI.Text(Facts, R.X + 12.0f, R.Y + 98.0f, Ts(12.0f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, false, R.W - 24.0f));
	}
}

/** u4_ui (FH-6a, FestPinsReveal): the pin wall drawn alone into Cv in its Career-page rect. */
void HubPinsDraw(RiverLine& RL, Canvas& Cv, double Now)
{
	Access::Pins(RL, Cv, {1092.0f, 656.0f, 484.0f, 104.0f}, Now);
}
} // namespace rlfest_detail

void RiverLine::FestivalHub(const Rect& R, const fest::Festival& F, double Now)
{
	// G1 FH-1a's skeleton and FH-1b's strip, day strip, rows and ticker (the file's head says what each package adds). R is the
	// host's content rect (FH-1c's NetHubRect): every region of HubFrameOf(F.Site) lies inside it, and nothing draws past it.
	rlfest_detail::HubUi& U = rlfest_detail::HubOf(*this, F, Now);
	const rlfest_detail::HubFacts H = rlfest_detail::Gather(*this, S, F, World);
	if (!H.Series)
	{
		return;
	}
	rlfest_detail::HubCtx X{*this, *C, UI, H, rlfest_detail::HubFrameOf(F.Site), festart::LookOf(F.Which), Now, Now - U.OpenedAt, Previewing};
	rlfest_detail::HubOpenSting(X, U); // FH-6a: the host's sting, once a world day per festival on this laptop (§2.7.6)
	// FH-1b, house law 3: an open popover holds the pointer for the whole hub (modal: a click on it reaches nothing beneath, a click
	// anywhere else closes it and is swallowed); FestivalOverlays draws it once the hub has drawn, with the pointer handed back.
	const bool PopUp = U.Popover.Open && !X.Still;
	rlsites_detail::SitesBlocked Held;
	if (PopUp)
	{
		Held = rlsites_detail::SitesBlockUnder(UI, U.Popover.R, true);
	}
	C->PushClip(R);
	rlfest_detail::HubYearStrip(X, U, R);
	if (H.Broadcast)
	{
		// FH-5b, Main Event Day and Closing (§2.8.4): the hero band, the Main card with its meter (the champion card once the result
		// exists), REGISTRATIONS (the reel once the result exists) and WHO'S IN where the day strip, the rows and the ticker stand.
		rlfest_detail::HubHeroBand(X, U);
		if (H.HasResult)
		{
			rlfest_detail::HubChampCard(X);
			rlfest_detail::HubReel(X);
		}
		else
		{
			rlfest_detail::HubMainCard(X);
			rlfest_detail::HubRegsChart(X);
		}
		rlfest_detail::HubWhoIn(X);
		if (!X.Still)
		{
			rlfest_detail::Access::SetListed(*this, {});
		}
	}
	else
	{
		rlfest_detail::HubBanner(X);
		if (H.HasResult)
		{
			rlfest_detail::HubChampCard(X); // FH-5b: the Main's champion through the Afterglow
		}
		else
		{
			rlfest_detail::HubMainCard(X);
		}
		rlfest_detail::HubRows(X, U, rlfest_detail::HubDays(X, U));
		rlfest_detail::HubTicker(X, U);
	}
	rlfest_detail::HubBody(X, U, rlfest_detail::HubTabs(X, U));
	C->PopClip();
	if (PopUp)
	{
		rlsites_detail::SitesUnblock(UI, Held);
		if (Held.Scrim)
		{
			U.Popover = rlfest_detail::HubUi::Pop();
		}
	}
	if (!X.Still)
	{
		U.Drew = true;
	}
}

void RiverLine::FestivalOverlays(double Now)
{
	// G1 FH-1b: the strip's popover, over the hub that drew in this frame (another page, app or the stream preview leaves it
	// unseen; a new page or series closes it, HubOf).
	const auto It = rlfest_detail::HubStates().find(this);
	if (It != rlfest_detail::HubStates().end())
	{
		rlfest_detail::HubUi& U = It->second;
		const bool Drew = U.Drew;
		U.Drew = false;
		if (Drew && U.Popover.Open)
		{
			rlfest_detail::HubPopover(*this, *C, UI, U, Now);
		}
	}
	// G1 FH-6a: a festival registration's sting, then the moment over everything above (it hands back the pointer FestMomentHold took).
	if (!Previewing)
	{
		rlfest_detail::HubSeatSting(S, rlfest_detail::HubCuesOf(*this));
	}
	rlfest_detail::HubMoment(*this, *C, UI, World, Previewing, Now);
}

void RiverLine::CareerPins(const Rect& R, double /*Now*/)
{
	// G1 FH-6a (§2.8.7): the pin wall; stream B's CareerPage calls it in the band at y 656 (FH-6b).
	rlfest_detail::HubPins(*this, *C, UI, S, R, Previewing);
}
} // namespace ui
} // namespace ss
