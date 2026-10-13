// The living_world suite's kit for update 2's per-track files (update 2 SPEC §6.7): each track's checks live in its own
// tests/world_seg_<track>.cpp, compiled into world_test and called from main() in world_main.cpp after the suite's own
// checks. Each entry point counts its own failures and returns them; helpers go in a namespace of its own inside
// world_test (world_test::fresh_seg, ...), so two files never define the same name.
#pragma once

namespace world_test
{
int SegFresh(); // core-world (G1): tests/world_seg_fresh.cpp
int SegSites(); // core-world (G3): tests/world_seg_sites.cpp
} // namespace world_test
