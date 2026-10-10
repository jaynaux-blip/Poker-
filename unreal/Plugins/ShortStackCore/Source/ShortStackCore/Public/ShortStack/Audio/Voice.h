#pragma once

#include "ShortStack/Common.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ss
{
namespace audio
{
/**
 * Voices for the card room (next_segment_spec §4.10): a synthesized murmur that carries a line's rhythm and pitch
 * without words (the captions carry the words), non-word sounds, and the PA's chain. No audio assets. Speech renders
 * off the game thread. Every coefficient is computed per syllable or per event, never per sample.
 *
 * Bodies: audio-synth (Voice.cpp). Phase A, day 1: stubs that compile and render silence.
 */
enum class Vocal : int
{
	None,
	Laugh,
	Sigh,
	Scoff,
	Groan,
	Ooh,
	Gasp,
	Cheer,
	Clap,
	Applause,
};
struct Timbre
{
	float F0 = 120.0f;
	float Tract = 1.0f;
	float Rate = 4.6f;
	float Breath = 0.15f;
	float Rasp = 0.0f;
};
SHORTSTACKCORE_API Timbre TimbreFor(bool Woman, int Age, uint32_t Seed);
SHORTSTACKCORE_API Timbre PaTimbre(uint32_t Seed);
struct Syllable
{
	float Start = 0.0f;
	float Length = 0.0f;
	float F1 = 500.0f;
	float F2 = 1500.0f;
	float F3 = 2500.0f;
	float Open = 0.5f;
	float Gain = 1.0f;
	float Pitch = 1.0f;
};
struct SpeechPlan
{
	std::vector<Syllable> Syllables;
	float Seconds = 0.0f;
};
SHORTSTACKCORE_API SpeechPlan PlanSpeech(const std::string& Line, const Timbre& T, float Effort, uint32_t Seed);
/** The jaw (0 closed .. 1 open) T seconds in: the host's lip sync reads this. */
SHORTSTACKCORE_API float MouthAt(const SpeechPlan& P, float T);
SHORTSTACKCORE_API std::vector<float> RenderSpeech(const SpeechPlan& P, const Timbre& T, bool Murmur, uint32_t Seed);
SHORTSTACKCORE_API std::vector<float> RenderVocal(Vocal V, const Timbre& T, int Voices, uint32_t Seed);
SHORTSTACKCORE_API void PaChain(std::vector<float>& Samples, uint32_t Seed);
} // namespace audio
} // namespace ss
