// The video_channel suite's kit for its per-track files (update 3 SPEC §7.7, §8.2). Update 2's title and copy checks moved
// verbatim from video_main.cpp to tests/video_seg_copy.cpp at Phase A (core-videocontent's from then on); they are compiled
// into video_test and called from main() in video_main.cpp in the order they always ran. The helpers they share stay
// video_main.cpp's own and are declared here. (Its thumbnail draws moved to tests/ui_seg_thumbs.cpp, in rerun_ui.)
//
// In a track's file: helpers go in a namespace of its own inside video_test, so two files never define the same name.
#pragma once

#include "ShortStack/Game/Recall.h"
#include "ShortStack/Game/Video.h"

#include <string>

namespace video_test
{
/** A world day in minutes. */
constexpr double Day = 1440.0;
extern int Failures;
extern int Checks;
void Expect(bool Condition, const char* What);
void Expect(bool Condition, const std::string& What);
std::string Lower(std::string S);
/** The scripted bad beat (AA against 77, all in preflop, a seven on the river), as the player saw it. */
ss::recall::Record BadBeatHand(bool& Showdown, bool Swap = false);
/** A Dee's hand: the player, Big Lou and Mei, a bad beat for 64 big blinds. */
ss::recall::Record DeesHand();
/** A candidate for the creator careers: a cooler at $5.50. */
ss::video::Candidate Moment(const std::string& Id, double World);

// video_seg_copy.cpp (core-videocontent from Phase A): Update 2's title and copy checks.
void Copy();
void Storytime();
void Rules();
void CoolerOutcome();
} // namespace video_test
