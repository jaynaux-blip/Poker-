// Update 3 (SPEC §6.2, §7.3): Rerun 2.0's titles, core-videocontent's file: about 2,000 templates with fact slots, promise
// tags and hidden traits behind the stable rendezvous-hash picker (it never offers a template one of the last 20 titles
// used), and the library's inventory for the copy lint. Phase A, day 1: stubs (no options; Update 2's picker untouched).
#include "ShortStack/Game/VideoContent.h"

#include <string>
#include <vector>

namespace ss
{
namespace video
{
std::vector<TitleOption> TitleOptions(const Choice& /*Ch*/, const std::vector<Candidate>& /*Cands*/, const Channel& /*C*/, int /*Shuffle*/, int /*Count*/)
{
	return {};
}

std::vector<std::string> InventoryU3()
{
	return {};
}
} // namespace video
} // namespace ss
