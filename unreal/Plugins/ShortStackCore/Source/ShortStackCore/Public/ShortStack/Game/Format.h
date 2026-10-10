#pragma once

#include "ShortStack/Hand.h"

// Exported (SHORTSTACKCORE_API) since the next segment: the game module's table HUD, desk and room clocks format money,
// places and clocks the way the core does (next_segment_spec §7.1).
namespace ss
{
/** "12,345" (en-US grouping). */
SHORTSTACKCORE_API std::string Grouped(int64_t N);
/** "$2.37", "$1,225.00", "-$1.00". Port of money() in web/src/client/canvasui.ts. */
SHORTSTACKCORE_API std::string Money(Chips Cents);
/** Chip counts: "12,345", "1.25M", "12.3M". */
SHORTSTACKCORE_API std::string ChipsText(double N);
/** "1st", "22nd", "1,000th". */
SHORTSTACKCORE_API std::string Ordinal(int N);
/** Minutes after midnight as "2:07 AM". */
SHORTSTACKCORE_API std::string ClockString(double Minutes);
/** printf-style fixed decimals for display ("12.5"). */
SHORTSTACKCORE_API std::string Fixed(double V, int Digits);
/** "Flop", "Turn", ... */
SHORTSTACKCORE_API const char* StreetTitle(Street S);

/** Easing curves shared by the client animations. */
SHORTSTACKCORE_API double EaseOutCubic(double T);
SHORTSTACKCORE_API double EaseInOut(double T);
SHORTSTACKCORE_API double EaseOutBack(double T);
SHORTSTACKCORE_API double Clamp01(double V);
} // namespace ss
