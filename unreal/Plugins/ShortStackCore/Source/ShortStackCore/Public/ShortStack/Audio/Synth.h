#pragma once

#include "ShortStack/Game/Session.h"

namespace ss
{
namespace audio
{
/**
 * Procedural audio, port of web/src/audio/sound.ts: every sound is synthesized
 * (no samples). One-shots render to mono float buffers; the ambience (rain on
 * glass, heavy rain, fridge hum, laptop fan, drips) streams block by block.
 */
constexpr int SampleRate = 48000;

enum class Effect : int
{
	Buzz,     // phone vibrating on the desk
	CanOpen,  // a fresh energy drink
	Thump,    // one heartbeat (lub-dub)
};

/** Renders a game sound (volume 0..1 as the session requests it). */
std::vector<float> Render(SoundId Id, float Volume, uint32_t Seed);
std::vector<float> Render(Effect Id, uint32_t Seed);
/** Distant thunder, 5 s. */
std::vector<float> RenderThunder(float Strength, uint32_t Seed);

/** Continuous room ambience. Not thread-safe; own one per audio stream. */
class Ambience
{
public:
	explicit Ambience(uint32_t Seed);
	/** Fills N mono samples. */
	void Render(float* Out, int N);

private:
	struct Filter
	{
		double B0 = 1.0, B1 = 0.0, B2 = 0.0, A1 = 0.0, A2 = 0.0;
		double X1 = 0.0, X2 = 0.0, Y1 = 0.0, Y2 = 0.0;
		double Process(double X);
	};
	uint32_t State = 1;
	double NextNoise();
	Filter HissBp;
	Filter HissLp;
	Filter BodyLp;
	Filter FanLp;
	Filter DripBp;
	double Brown = 0.0;
	double Time = 0.0;
	double NextDrip = 0.1;
	double DripUntil = 0.0;
	double DripStart = 0.0;
	double DripGain = 0.0;
};
} // namespace audio
} // namespace ss
