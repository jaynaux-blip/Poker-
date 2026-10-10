#include "ShortStack/Game/Recall.h"
#include "../StrictFloat.h"

// The hands the player played (next_segment_spec §6.3). core-tableui.
//
// Phase A, day 1: stubs that compile and change nothing: an empty record, nothing written, nothing read.

namespace ss
{
namespace recall
{
Record Of(const Hand&, int, const std::vector<std::string>&, const std::vector<int>&, int, int)
{
	return Record();
}

std::string Write(const Record&)
{
	return std::string();
}

bool Read(const std::string&, Record&)
{
	return false;
}
} // namespace recall
} // namespace ss
