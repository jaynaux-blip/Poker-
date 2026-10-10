#include "ShortStack/UI/Captions.h"
#include "../StrictFloat.h"

// Captions (next_segment_spec §4.9). core-tableui.
//
// Phase A, day 1: stubs that compile and draw nothing; a line holds as long as its speech.

namespace ss
{
namespace ui
{
float CaptionHold(const std::string&, float SpeechSeconds)
{
	return SpeechSeconds;
}

void DrawCaptions(Canvas&, const std::vector<Caption>&, const CaptionStyle&, float, float, double)
{
}
} // namespace ui
} // namespace ss
