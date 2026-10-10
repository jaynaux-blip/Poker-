#pragma once

#include "ShortStack/Game/Life.h"
#include "ShortStack/UI/Canvas.h"

#include <string>
#include <vector>

namespace ss
{
class Session;

namespace ui
{
/**
 * The dates the screens below show beside the session's own facts (next_segment_spec §1.3): chop days and payments due,
 * sponsor period ends, appearance windows and renewals, the 1st's badge review. Every feature's marks, merged in time
 * order (Session::Marks); the second screen and the wall calendar draw the one list (ui-laptop, SecondScreen.cpp).
 */
struct Agenda
{
	std::vector<life::Mark> Marks;
};
/** The agenda for the Days days from World. */
SHORTSTACKCORE_API Agenda AgendaOf(const Session& S, double World, double Days);

/**
 * The monitors GearDrop sells, as the apartment shows them beside the laptop: pictures, drawn without input.
 * - Which 0, the 24" on the right: Kast's studio while the player is live (the stream as it goes out, the chat,
 *   the viewers), otherwise the life tracker (the bankroll's story from the ledger, the rent, the Embercrest's next
 *   event or the player's entry there, the career so far).
 * - Which 1, the 27" on the left: study, a preflop chart that cycles through the positions beside the tells the
 *   player has learned at Dee's table.
 */
namespace secondscreen
{
constexpr float Width = 1600.0f;
constexpr float Height = 900.0f;

SHORTSTACKCORE_API void Draw(Canvas& C, const Session& S, int Which, double Now);
} // namespace secondscreen

/**
 * The Embercrest's wall calendar, the room's own giveaway, on the apartment's wall by the window: the month, today's
 * and tomorrow's events in type big enough to read from the desk chair, and what the player has written on it (their
 * entry, the rent's day). Printed paper, drawn without input; the host draws it again when Key changes.
 */
namespace wallcalendar
{
/** Canvas units: a 50 x 70 cm sheet. */
constexpr float Width = 600.0f;
constexpr float Height = 840.0f;

/** Everything the sheet shows, as a key: the day, the player's entries on it and tomorrow, the rent. */
SHORTSTACKCORE_API std::string Key(const Session& S);
SHORTSTACKCORE_API void Draw(Canvas& C, const Session& S);
} // namespace wallcalendar
} // namespace ui
} // namespace ss
