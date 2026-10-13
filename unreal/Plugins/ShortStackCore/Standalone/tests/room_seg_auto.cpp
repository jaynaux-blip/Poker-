// card_room: core-autoplay's additions (update 2 SPEC §6.7, §2.12): LiveSkipSliceEqualsTicks (live::SkipSlice in slices
// of any budget gives the same checkpoints as stepping the night tick by tick), and RoomPace with the plan in the seat at
// 1x, 4x and 8x (the cue log by hand is the same: no frame clock). Compiled into room_test and called from main() in
// room_main.cpp, as tests/room_field.cpp is. Phase A (the lead): the entry point, empty; core-autoplay fills it, its
// helpers in room_seg_auto's own namespace.
#include "ShortStack/Game/LiveAuto.h"

namespace room_seg_auto
{
/** core-autoplay's card_room checks. The failures they found. */
int Run()
{
	return 0;
}
} // namespace room_seg_auto
