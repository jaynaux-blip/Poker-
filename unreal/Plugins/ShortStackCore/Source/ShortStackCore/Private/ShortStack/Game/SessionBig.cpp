// Update 4, the session's side of the Big Field (SPEC v1.7 §4.2-§4.15): the unlock and the reveal, the plans and the stops, the
// beats, CHIP COUNTS, the broadcast kit's view and the honorifics, the play-on and its delivery, late registration, re-entry, YOUR
// RUN's record and ss.Big.*. B-session's file from G3.
//
// Phase A (§6 PA-39, PA-42; the lead): stubs that keep today's behaviour. Re-entry refuses, there are no play-ons to drain, the two
// views are empty and off, and ss.Big.* answers that it isn't built yet.
#include "ShortStack/Game/BigField.h"
#include "ShortStack/Game/Session.h"

#include <string>

namespace ss
{
std::string Session::Reenter(const std::string& /*EventId*/)
{
	return "Not yet.";
}

void Session::DrainPlayOns()
{
	// Phase A: PlayOns is always empty (nothing moves a busted run there before G3).
}

const bigfield::CountsView& Session::CountsView() const
{
	static const bigfield::CountsView BigNoCounts;
	return BigNoCounts;
}

const bigfield::Broadcast& Session::BroadcastView() const
{
	static const bigfield::Broadcast BigNoBroadcast;
	return BigNoBroadcast;
}

std::string Session::DebugBig(const std::string& /*Args*/)
{
	return "Not built yet: the Big Field comes with Update 4's G3.";
}
} // namespace ss
