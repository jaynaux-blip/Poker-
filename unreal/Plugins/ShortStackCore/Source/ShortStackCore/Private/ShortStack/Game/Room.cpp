#include "ShortStack/Game/Room.h"
#include "../StrictFloat.h"

// The room's one voice (next_segment_spec §4). core-room.
//
// Phase A, day 1: stubs that compile and say nothing: the director hands back no cues and keeps no state, the mood
// stays Running with today's flat mix, and no voices are made.

namespace ss
{
namespace room
{
Mood MoodOf(const Tournament&, bool, bool)
{
	return Mood::Running;
}

Mix MixFor(Mood, float)
{
	return Mix();
}

Director::Director(const life::LiveEntry&, const live::Occurrence&, const std::string&, const std::string&)
{
}

void Director::Resume(const Tournament&, const std::string&, double)
{
}

void Director::SetRail(const std::vector<RailBacker>&)
{
}

void Director::Events(const Tournament&, const std::vector<TEvent>&, double)
{
}

void Director::Rounds(const Tournament&, const std::vector<TableRound>&)
{
}

void Director::Hand(const Tournament&, const HandNote&)
{
}

void Director::Clock(const Tournament&, double, double)
{
}

std::vector<Cue> Director::Take()
{
	return {};
}

std::string Director::Write() const
{
	return std::string();
}

bool Director::Read(const std::string&)
{
	return true;
}

std::vector<life::LiveVoice> VoicesFor(const life::LiveEntry&)
{
	return {};
}

life::LiveVoice VoiceOf(const world::World&, int, const std::string&)
{
	return life::LiveVoice();
}
} // namespace room
} // namespace ss
