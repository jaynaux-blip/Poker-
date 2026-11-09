// The site_play suite's kit (update 3 SPEC §7.7, suite 18): core-siteplay's checks in siteplay_main.cpp and
// siteplay_session.cpp, core-fuse's in siteplay_fuse.cpp, each file's entry point called from main() in siteplay_main.cpp.
// Helpers go in a namespace of their own inside siteplay_test (siteplay_test::fuse_seg, ...), so two files never define the
// same name. Expect counts a check and a failure. core-siteplay's file (with siteplay_main.cpp).
#pragma once

#include <string>

namespace siteplay_test
{
extern int Failures;
extern int Checks;
/** "siteplay_test long": the slow modes (more seeds; the League's group-pace case). */
extern bool Long;
void Expect(bool Condition, const char* What);
void Expect(bool Condition, const std::string& What);

// The per-track files' entry points, called from main() in this order.
void SegCore();    // core-siteplay: tests/siteplay_main.cpp (the book: catalogs, the Governor, the loops, the model harness)
void SegSession(); // core-siteplay: tests/siteplay_session.cpp (the Session's site play, the strike gate's refusals)
void SegFuse();    // core-fuse: tests/siteplay_fuse.cpp (FUSE's tables, draws, pool and Session glue)
/** "siteplay_test beef [days] [fuses]" (G2 fix M1): update3_siteplay's career run long, how often a Beef forms in play. */
void SegBeef(int Days, int Fuses);
} // namespace siteplay_test
