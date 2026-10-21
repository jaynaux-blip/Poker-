// Update 3 (SPEC §2.6 F7, §7.2): FUSE's Session glue, core-fuse's file: LIGHT IT through the strike gate with the rake
// override, the ordinal advanced and the save committed at entry before the reveal, the 3-max table, the reveal's state.
// Lands after spike S-J passes. Phase A, day 1: "not yet".
#include "ShortStack/Game/Session.h"

#include "ShortStack/Game/Fuse.h"

#include <string>

namespace ss
{
std::string Session::LightFuse(int /*Tier*/, bool /*Ticket*/, StrikeConfirm /*Confirm*/)
{
	return "Not yet.";
}

fuse::Reveal Session::FuseReveal(int /*Table*/) const
{
	return fuse::Reveal();
}
} // namespace ss
