#pragma once

#include "ShortStack/Hand.h"

namespace ss
{
/** "12,345" (en-US grouping). */
std::string Grouped(int64_t N);
/** "$2.37", "$1,225.00", "-$1.00". Port of money() in web/src/client/canvasui.ts. */
std::string Money(Chips Cents);
/** Chip counts: "12,345", "1.25M", "12.3M". */
std::string ChipsText(double N);
/** "1st", "22nd", "1,000th". */
std::string Ordinal(int N);
/** Minutes after midnight as "2:07 AM". */
std::string ClockString(double Minutes);
/** printf-style fixed decimals for display ("12.5"). */
std::string Fixed(double V, int Digits);
/** "Flop", "Turn", ... */
const char* StreetTitle(Street S);

/** Easing curves shared by the client animations. */
double EaseOutCubic(double T);
double EaseInOut(double T);
double EaseOutBack(double T);
double Clamp01(double V);
} // namespace ss
