#include "ShortStack/Audio/CardRoom.h"
#include "../StrictFloat.h"

// The card room in stereo (next_segment_spec §4.4). audio-synth.
//
// Phase A, day 1: a stub that compiles and renders silence; nothing hosts it yet.

namespace ss
{
namespace audio
{
CardRoom::CardRoom(uint32_t InSeed) : State(InSeed == 0 ? 1u : InSeed)
{
}

void CardRoom::SetEmitters(const std::vector<Emitter>&)
{
}

void CardRoom::SetListener(float, float, float)
{
}

void CardRoom::SetMix(float, float, float, float, float, float)
{
}

void CardRoom::SetMuffle(float, float)
{
}

void CardRoom::SetPace(float)
{
}

void CardRoom::Duck(float, float)
{
}

void CardRoom::TableHand(int, float, bool, bool, float)
{
}

void CardRoom::Say(const SpeechPlan&, const Timbre&, Spot, int, float, bool)
{
}

void CardRoom::Crowd(Vocal, int, Spot, int, float)
{
}

void CardRoom::RenderStereo(float* Out, int Frames)
{
	for (int I = 0; Out && I < Frames * 2; ++I)
	{
		Out[I] = 0.0f;
	}
}

CardRoom::Meter CardRoom::Measure() const
{
	return Meter();
}
} // namespace audio
} // namespace ss
