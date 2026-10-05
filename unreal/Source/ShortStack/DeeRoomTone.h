#pragma once

#include "CoreMinimal.h"
#include "ShortStack/Audio/Synth.h"

#include <vector>

/**
 * Dee's front room as it sounds: the rain on the glass and the room's hum (ss::audio::Ambience), her radiator's steam
 * ticking and knocking under the window, and the TV on low while it's on (voices through a cheap speaker, a laugh
 * track now and then). Streamed by UNightOneAudio as the hub's ambience, a block at a time on the game thread, like the
 * ambience it wraps; everything synthesized, as the rest of the game's sound is.
 */
class FDeeRoomTone
{
public:
	explicit FDeeRoomTone(uint32 Seed);

	/** Fills N mono samples at ss::audio::SampleRate. */
	void Render(float* Out, int32 N);
	/** The TV on or off (it fades, as a set does when the volume's turned down). */
	void SetTv(bool bOn) { bTvOn = bOn; }
	/** The radiator's steam: 1 the night's heat, lower on a warm afternoon (it ticks less, never quite stops). */
	void SetHeat(float InHeat) { Heat = FMath::Clamp(InHeat, 0.0f, 1.0f); }

	/** The apartment door closing (Dee coming in or going out): a thud in the frame and the latch. */
	static std::vector<float> RenderDoor(uint32 Seed);

private:
	/** A biquad (RBJ's cookbook): low pass, high pass or band pass. */
	struct FBiquad
	{
		double B0 = 1.0, B1 = 0.0, B2 = 0.0, A1 = 0.0, A2 = 0.0;
		double X1 = 0.0, X2 = 0.0, Y1 = 0.0, Y2 = 0.0;
		void LowPass(double Hz, double Q);
		void HighPass(double Hz, double Q);
		void BandPass(double Hz, double Q);
		double Process(double X);
	};
	/** A decaying sine: the radiator's pings and knocks. */
	struct FRing
	{
		double Phase = 0.0;
		double Step = 0.0;
		double Amp = 0.0;
		double Decay = 0.0;
	};

	/** White noise, -1..1; Random is the same, 0..1. */
	double Noise();
	double Random();
	void Ping(double Hz, double Amp, double Seconds);
	void ScheduleRadiator();

	ss::audio::Ambience Room;
	uint32 State = 1;
	double Clock = 0.0;
	float Heat = 1.0f;

	// The radiator.
	TArray<FRing> Rings;
	double NextTick = 2.0;
	int32 BurstLeft = 0;
	double NextKnock = 25.0;
	FBiquad HissBand;
	double HissLevel = 0.0;

	// The TV.
	bool bTvOn = false;
	double TvGain = 0.0;
	FBiquad SpeakerHigh;
	FBiquad SpeakerLow;
	FBiquad Voice;
	FBiquad CrowdBand;
	double Pitch = 160.0;
	double PitchGoal = 160.0;
	double GlottalPhase = 0.0;
	double Syllable = 0.0;    // the envelope of the syllable being said
	double SyllableEnd = 0.0; // when it ends (then a gap, or the next)
	double NextSyllable = 0.0;
	double PhraseEnd = 0.0;
	double NextLaugh = 50.0;
	double LaughEnd = 0.0;
	double LaughStart = 0.0;
	double Bed = 0.0; // a low music bed under the talk, on and off
};
