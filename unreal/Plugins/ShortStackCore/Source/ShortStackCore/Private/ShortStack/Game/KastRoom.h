// The creator update (SPEC_CREATORS §2.2, §10.2, §10.7): the Room's private state, core-kastroom's header (private to the
// core: Kast.cpp includes it, and so may tests/kastgrowth_session.cpp by its relative path, so the Room can change without a
// Kast.h patch). Kast::Stream holds one by pointer, and names RoomState its friend: RoomState's own members (static helpers
// taking the Stream, and the RoomView core-kastroom implements for the chat games, KastPlay.h) may use the stream's private
// parts (Say, Queue, Earn, R, the debts).
//
// What lands here with the Room: the visits on the slot process (here or away, the slot's index, first and last in, the
// visit number, the source), the guests (a Pool index, arrive, leave, source, talker; at most 96, the rest Crowd with their
// own leave times), the named raiders (at most 24 a raid), the sources now and tonight, the deepest run and its event facts
// (ss.Kast.Moment's held run too: Stream::DebugRoom), the Ping's reach, the forecast (once a world minute; Start's is the one
// Heat reads), the directory rank (once a world minute), tonight's extras for the end card, and the View the UI reads, filled
// once a Tick. Phase A-K: the View alone.
#pragma once

#include "ShortStack/Game/Kast.h"

namespace ss
{
namespace kast
{
struct RoomState
{
	View Shown; // what Stream::GetView hands the UI (empty until the Room fills it)
};
} // namespace kast
} // namespace ss
