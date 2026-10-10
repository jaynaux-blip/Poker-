#include "ShortStack/Audio/Voice.h"
#include "../StrictFloat.h"

// Voices for the card room (next_segment_spec §4.10). audio-synth.
//
// Phase A, day 1: stubs that compile and render silence (empty buffers, a closed mouth).

namespace ss
{
namespace audio
{
Timbre TimbreFor(bool, int, uint32_t)
{
	return Timbre();
}

Timbre PaTimbre(uint32_t)
{
	return Timbre();
}

SpeechPlan PlanSpeech(const std::string&, const Timbre&, float, uint32_t)
{
	return SpeechPlan();
}

float MouthAt(const SpeechPlan&, float)
{
	return 0.0f;
}

std::vector<float> RenderSpeech(const SpeechPlan&, const Timbre&, bool, uint32_t)
{
	return {};
}

std::vector<float> RenderVocal(Vocal, const Timbre&, int, uint32_t)
{
	return {};
}

void PaChain(std::vector<float>&, uint32_t)
{
}
} // namespace audio
} // namespace ss
