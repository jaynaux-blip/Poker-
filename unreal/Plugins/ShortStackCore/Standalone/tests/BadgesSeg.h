// The pro_badges suite's kit for update 2's per-track files (update 2 SPEC §6.7): each track's checks live in its own
// tests/badges_seg_<track>.cpp, compiled into badges_test and called from main() in badges_main.cpp after the suite's own
// checks. Failures is the suite's count; helpers go in a namespace of its own inside badges_test.
#pragma once

namespace badges_test
{
extern int Failures;
void SegFresh(bool Quick); // core-world: tests/badges_seg_fresh.cpp
} // namespace badges_test
