// The creator update (SPEC_CREATORS §2.9, §5.3 K2, K10-K12): Kast's chat games, core-kastplay's file. Hold or Crack, Nod,
// Welcome the raid, Set the scene, Rail Call's press and its named arrivals, First Follower's line, all through the RoomView
// Kast.cpp implements (KastPlay.h). Phase A-K: every verb refuses and every hook does nothing.
#include "KastPlay.h"

#include <string>

namespace ss
{
namespace kast
{
namespace play
{
bool Nod(PlayState& /*P*/, RoomView& /*Room*/, Channel& /*Ch*/, const std::string& /*Name*/)
{
	return false;
}

bool RailCall(PlayState& /*P*/, RoomView& /*Room*/, Channel& /*Ch*/, const Inputs& /*In*/)
{
	return false;
}

bool WelcomeRaid(PlayState& /*P*/, RoomView& /*Room*/, Channel& /*Ch*/)
{
	return false;
}

bool SetTheScene(PlayState& /*P*/, RoomView& /*Room*/, Channel& /*Ch*/)
{
	return false;
}

bool CallIt(PlayState& /*P*/, RoomView& /*Room*/, Channel& /*Ch*/, bool /*Holds*/)
{
	return false;
}

void Begin(PlayState& /*P*/, const Channel& /*Ch*/, const Inputs& /*In*/)
{
}

void OnMoment(PlayState& /*P*/, RoomView& /*Room*/, Channel& /*Ch*/, const Inputs& /*In*/, Moment /*M*/)
{
}

void OnMinute(PlayState& /*P*/, RoomView& /*Room*/, Channel& /*Ch*/, const Inputs& /*In*/, int /*Minute*/)
{
}

void OnChat(PlayState& /*P*/, RoomView& /*Room*/, const Inputs& /*In*/, double /*DReal*/, Rng& /*R*/)
{
}

void OnArrive(PlayState& /*P*/, RoomView& /*Room*/, Channel& /*Ch*/, const std::string& /*Name*/, View::Source /*From*/)
{
}

void OnFollow(PlayState& /*P*/, RoomView& /*Room*/, Channel& /*Ch*/, const std::string& /*Name*/, bool /*FirstFollower*/)
{
}

void Fill(const PlayState& /*P*/, View& /*V*/)
{
}

void Close(const PlayState& /*P*/, Summary& /*S*/)
{
}
} // namespace play
} // namespace kast
} // namespace ss
