// Update 4, Festival Season (SPEC v1.7 §2.5.3; LEAD_CALLS R-4): the festival race's private rules, beside H/WorldFest.h's frozen
// views. F-world's header (G1; private, so not frozen, Q-5): the marks' rule as functions (FestMarksNested needs it callable: three
// thousand crowns are not testable) and the ladder's number (FW-2a); FestTicketPaid, the session's one write to the hero's marks
// (FW-2b, R-4); the titles' reads (FW-3): an Honor's festival and slot, a post's festival title (World::Headline's branch), and the
// title order's comparison and pick (§2.6.3: the card's honorific and bigfield::Honorific read FestTopTitle). Bodies in G/WorldFest.cpp.
#pragma once

#include "ShortStack/Game/WorldFest.h"

#include <string>

namespace ss
{
namespace world
{
class World;
struct WorldEvent;

/** FW-3: an Honor's slot on its festival's roll (FestOfHonor). */
enum class FestSlot : int
{
	None = -1,
	Main,  // the festival's Main (its MainId)
	Mini,  // a championship's Mini Main Event (in its TitleIds)
	Micro, // a championship's Micro Main Event
	Crown, // the Player of the Series belt ("<series>#pos", §2.6.1)
};
/** The festival edition an Honor belongs to and its slot (§2.6.2): a crown Honor's series, or a title template's (the EventId's template,
 *  before '@', in its edition's TitleIds); nullptr and FestSlot::None for every other Honor (a numbered event's, a series Main that is no
 *  festival's, the year's titles). */
const fest::Festival* FestOfHonor(const Honor& H, FestSlot& Slot);
/** The festival title a Champion or Won post names (World::Headline's festival branch, §2.9.2): the post's Honor (the same minute, the
 *  winner and the event name; Lasting keeps every title's Honor) read through FestOfHonor. nullptr unless its slot is Main, Mini or
 *  Micro. */
const fest::Festival* FestTitlePost(const World& W, const WorldEvent& E, FestSlot& Slot);
/** The title order (§2.6.3, FestTitleOrder): whether A outranks B. Class first (TitleRank: 1 the best; 0, not a title, last), then the
 *  bigger Prize (a belt's Entries: its points), then the newer date; an exact tie is not before. */
bool FestTitleBefore(const Honor& A, const Honor& B);
/** A person's top title by the title order (Npc -1: the hero's); nullptr when they hold none. The card's honorific line and
 *  bigfield::Honorific read it. */
const Honor* FestTopTitle(const World& W, int Npc);

/** FINALIST's width in a race of Scorers (§2.5.3's Kf): clamp(ceil(0.10 S), 3, 25) from 250 scorers, clamp(ceil(0.05 S), 3, 10)
 *  below (fest::kFinalistCap, kFinalistSmall, kFinalistSmallBelow, kFinalistSmallShare); 0 for an empty race. */
int FestFinalistRanks(int Scorers);
/** The mark the scorer at Rank meets at the crown, the best of (§2.5.3): Title at rank 1, Podium at 2-3, FINALIST within Kf, Top 10%
 *  within ceil(0.10 S), Top 25% within ceil(0.25 S), else None; None for a rank outside 1..Scorers. Non-increasing in rank, each
 *  grade's ranks contiguous (FestMarksNested). */
FestMark MarkFor(int Rank, int Scorers);
/** The last rank that meets Mark or better in a race of Scorers (MarkFor(r, S) >= Mark exactly for r in 1..this): the rank whose
 *  points fest::MarkCutoff reads. 0 for None (or Count) and for an empty race; never more than Scorers. */
int FestLastRank(FestMark Mark, int Scorers);
/** U27's "{N} pts to {MARK}" on YOUR <FESTIVAL>'s ladder (§2.5.3), from the live race right now: N = fest::MarkCutoff(W, Mark) − the
 *  hero's points + 1; 0 when the hero holds Mark or better now (the rung is filled); −1 when the rung shows no number: no live race,
 *  None, or before the hero's first cash (the lit rung is CASH 1). */
int FestPointsTo(const World& W, FestMark Mark);
/** FestStep's receipt (§2.6.1 step 7, §2.13; R-4): the hero's crown ticket for edition Key is paid (keyed festticket:<key> in
 *  site::Book::Done), so the mark's record drops its trailing t. The session has no other write to the world's marks. Moves HeroRev
 *  (the session's next save writes the world); nothing when the hero holds no unpaid ticket for Key. */
void FestTicketPaid(World& W, const std::string& Key);
} // namespace world
} // namespace ss
