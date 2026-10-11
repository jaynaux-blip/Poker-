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
 * Voices for the card room (next_segment_spec §4.10): a synthesized murmur that carries a line's rhythm, vowels and
 * pitch without words (the captions carry the words), non-word sounds, and the PA's chain. No audio assets. Speech
 * renders off the game thread: every function here is pure and safe to call from any thread. Every coefficient is
 * computed per syllable or per event, never per sample.
 *
 * Bodies: audio-synth (Voice.cpp, with the engine in VoiceEngine.cpp).
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
/** A voice: its pitch (Hz), its tract's formant scale (1.0 men, about 1.17 women), syllables a second, breath, rasp (0..1). */
struct Timbre
{
	float F0 = 120.0f;
	float Tract = 1.0f;
	float Rate = 4.6f;
	float Breath = 0.15f;
	float Rasp = 0.0f;
};
/** Men 85 to 155 Hz, women 165 to 255 Hz, drifting with age; 4 to 5 syllables a second. */
SHORTSTACKCORE_API Timbre TimbreFor(bool Woman, int Age, uint32_t Seed);
/** The floor's announcer: 3.3 to 3.8 syllables a second, steady and projected. */
SHORTSTACKCORE_API Timbre PaTimbre(uint32_t Seed);
/**
 * One syllable of a plan, in seconds from the line's start. F1..F3 are the vowel's formants in Hz (already scaled by
 * the timbre); Pitch scales the timbre's F0; Open is the jaw's opening at the vowel (0..1). The rest shape the sound:
 * Onset and Coda are consonant classes (0 none; the engine's Manner list: 1 p/t/k, 2 b/d/g, 3 s, 4 sh, 5 f/th, 6 h,
 * 7 m/n, 8 l, 9 r, 10 w/y, 11 z/v), Locus and CodaLocus their F2 loci in Hz, G1/G2 a diphthong's glide targets (0: none),
 * Air the breath added to this syllable (0 voiced .. 1 whispered).
 */
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
	int Onset = 0;
	int Coda = 0;
	float Locus = 0.0f;
	float CodaLocus = 0.0f;
	float G1 = 0.0f;
	float G2 = 0.0f;
	float Air = 0.0f;
};
struct SpeechPlan
{
	std::vector<Syllable> Syllables;
	float Seconds = 0.0f;
};
/**
 * The line's syllables, read from its spelling (numbers spoken out): stress, timing at the timbre's rate, pauses at the
 * punctuation, declination and accents, a fall at a full stop and a rise at a question. Effort 0.3 (a mutter) to 1.0 (a
 * shout); 0.6 is talk, 0.9 a dealer's call across the room. Deterministic in its inputs.
 */
SHORTSTACKCORE_API SpeechPlan PlanSpeech(const std::string& Line, const Timbre& T, float Effort, uint32_t Seed);
/** The jaw (0 closed .. 1 open) T seconds in: the host's lip sync reads this. Zero outside every syllable. */
SHORTSTACKCORE_API float MouthAt(const SpeechPlan& P, float T);
/**
 * The plan spoken at 48 kHz, mono: a glottal source through the formants, with its consonants. Murmur rounds it off
 * like talk heard from across a room (low-passed, consonants softened). About 1.5 ms of work a second of speech.
 */
SHORTSTACKCORE_API std::vector<float> RenderSpeech(const SpeechPlan& P, const Timbre& T, bool Murmur, uint32_t Seed);
/**
 * A non-word sound, 48 kHz mono: one person (Voices 1) or a crowd, each with their own pitch and timing. Clap is about
 * 2 s of light claps; Applause runs 3.2 s, longer for a big crowd (up to 6 s).
 */
SHORTSTACKCORE_API std::vector<float> RenderVocal(Vocal V, const Timbre& T, int Voices, uint32_t Seed);
/** The PA, in place at 48 kHz: 250 Hz to 5 kHz through a small horn, then the ceiling's speakers; adds a 0.3 s tail. */
SHORTSTACKCORE_API void PaChain(std::vector<float>& Samples, uint32_t Seed);
} // namespace audio
} // namespace ss
