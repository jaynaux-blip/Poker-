// card_room (next_segment_spec §7.6, suite 13): core-room's tests.
// RoomNightly (the cue log against Tests/room_nightly.txt; room_test --write regenerates it); the PA's order and
// numbers; honesty; talk rates; RoomCheckpoints; RoomTalkCopy (the copy lint, the Back Room's eight); RoomField;
// RoomPace; RoomEquityScope.
//
// Phase A: the copy lint's self-check, and RoomField (the lead's, tests/room_field.cpp: live::DescribeField against both
// old copies of the field code). core-room fills the rest in Phase B.
// Goldens and fixtures are read from SS_TESTS_DIR (unreal/Plugins/ShortStackCore/Tests, set by CMakeLists.txt).
#include "CopyLint.h"
#include "ShortStack/Audio/Voice.h"
#include "ShortStack/Game/Room.h"
#include "ShortStack/Game/TableTalk.h"

#include <cstdio>

namespace room_test
{
int Failures = 0;

void Expect(bool Condition, const char* What)
{
	if (!Condition)
	{
		++Failures;
		std::printf("FAILED: %s\n", What);
	}
}
} // namespace room_test

namespace room_field
{
/** RoomField (the lead's, Phase A): tests/room_field.cpp. The failures it found. */
int Run();
}

int main()
{
	room_test::Failures += copylint::SelfCheck();
	room_test::Failures += room_field::Run();
	if (room_test::Failures == 0)
	{
		std::printf("room tests: all passed\n");
	}
	return room_test::Failures == 0 ? 0 : 1;
}
