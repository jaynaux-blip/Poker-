// The kast_growth suite's kit (SPEC_CREATORS §9.1, the 23rd standalone suite): core-kastmath's checks in kastgrowth_main.cpp
// (the pure functions, the save tails), core-kastroom's in kastgrowth_session.cpp (the Room and the careers through the real
// kast::Channel and Stream on AdvanceTo's minute grid), core-kastplay's in kastgrowth_play.cpp (the chat games), each file's
// entry point called from main() in kastgrowth_main.cpp. Helpers go in a namespace of their own inside kastgrowth_test
// (kastgrowth_test::room_seg, ...), so two files never define the same name. Expect counts a check and a failure. "long" (the
// gates' run: every archetype at 12 seeds, the levers) is behind an argument. The lead's file.
#pragma once

#include <string>

namespace kastgrowth_test
{
extern int Failures;
extern int Checks;
/** main()'s argument: "long" runs the slow modes. */
extern bool Long;
void Expect(bool Condition, const char* What);
void Expect(bool Condition, const std::string& What);

// The per-track files' entry points, called from main() in this order.
void SegPure();    // core-kastmath: tests/kastgrowth_main.cpp (Pure, Forecast, the save tails)
void SegSession(); // core-kastroom: tests/kastgrowth_session.cpp (HonestCount, PaceInvariance, FrameRate, FirstMinutes, LongMtt,
                   // Momentum, Careers, Levers, Saves)
void SegPlay();    // core-kastplay: tests/kastgrowth_play.cpp (Hold or Crack, Nod, Welcome the raid, Set the scene, Rail Call)
} // namespace kastgrowth_test
