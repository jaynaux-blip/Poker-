#include "ShortStack/Game/Session.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Talk.h"
#include "ShortStack/Game/Town.h"

// The Session's side of the people on the block (the realism update's spec, §7.2): what was said and heard, Twitch's
// twenty, the tips. Written by core-shops as Phase A stubs and handed to people-core, who fills them (said keys,
// familiarity, WorldNotes, MemoryKind::Street) in this file, so Session.cpp stays closed after Phase A.

namespace ss
{
void Session::Heard(const std::string& /*Speaker*/, const town::Talk& /*Told*/) // (not "T": Session::T is the tournament)
{
}

std::string Session::Lend(const std::string& /*Speaker*/, Chips /*Cents*/)
{
	return "Not now.";
}

void Session::Tipped(const std::string& /*Speaker*/, int /*Percent*/)
{
}
} // namespace ss
