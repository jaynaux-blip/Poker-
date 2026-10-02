#pragma once

#include "ShortStack/UI/Canvas.h"

namespace ss
{
class Session;

namespace ui
{
/**
 * The monitors GearDrop sells, as the apartment shows them beside the laptop: pictures, drawn without input.
 * - Which 0, the 24" on the right: Kast's studio while the player is live (the stream as it goes out, the chat,
 *   the viewers), otherwise the life tracker (the bankroll's story from the ledger, the rent, the career so far).
 * - Which 1, the 27" on the left: study, a preflop chart that cycles through the positions beside the tells the
 *   player has learned at Dee's table.
 */
namespace secondscreen
{
constexpr float Width = 1600.0f;
constexpr float Height = 900.0f;

SHORTSTACKCORE_API void Draw(Canvas& C, const Session& S, int Which, double Now);
} // namespace secondscreen
} // namespace ui
} // namespace ss
