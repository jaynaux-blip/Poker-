#pragma once

#include "CoreMinimal.h"

/**
 * The Riverside's poker room, synthesized: the murmur of a room full of people talking at their
 * tables (a handful of voices, each a band of noise opening and closing in syllables and phrases),
 * chips riffled and stacked at the other tables, a dealer's shuffle now and then, the slot floor's
 * chimes through the doorway, and the building's air handling under it all. Busier while the room is
 * full (SetCrowd). Mono, 48 kHz, for UNightOneAudio::StartAmbience.
 */
class FCardRoomAmbience
{
public:
	explicit FCardRoomAmbience(uint32 Seed) : State(Seed | 1u)
	{
		for (FVoice& V : Voices)
		{
			V.Center = 380.0 + 900.0 * Uniform();
			V.NextPhrase = 4.0 * Uniform();
			V.Pan = 0.4 + 0.6 * Uniform();
		}
	}

	/** 0..1: how full the room is (tables still running), which drives the murmur and the chips. */
	void SetCrowd(float InCrowd) { Crowd = FMath::Clamp(static_cast<double>(InCrowd), 0.15, 1.0); }

	void Render(float* Out, int32 N)
	{
		constexpr double Rate = 48000.0;
		constexpr double Dt = 1.0 / Rate;
		const double AirK = 1.0 - FMath::Exp(-2.0 * UE_DOUBLE_PI * 90.0 * Dt);
		const double ClickK = 1.0 - FMath::Exp(-2.0 * UE_DOUBLE_PI * 5200.0 * Dt);
		for (int32 I = 0; I < N; ++I)
		{
			Time += Dt;
			double S = 0.0;

			// The air handling: a steady low rush.
			Brown = Brown * 0.996 + Noise() * 0.04;
			Air += (Brown - Air) * AirK;
			S += Air * 0.55;

			// Voices: each talks in phrases of syllables, through a band like a voice across a room.
			const int32 Talking = FMath::Clamp(static_cast<int32>(3.0 + 7.0 * Crowd), 3, NumVoices);
			for (int32 K = 0; K < NumVoices; ++K)
			{
				FVoice& V = Voices[K];
				if (Time >= V.NextPhrase)
				{
					V.bOn = !V.bOn && K < Talking;
					V.NextPhrase = Time + (V.bOn ? 1.5 + 4.5 * Uniform() : 1.0 + 7.0 * Uniform());
				}
				if (Time >= V.NextSyllable)
				{
					V.Target = V.bOn ? 0.3 + 0.7 * Uniform() : 0.0;
					V.NextSyllable = Time + 0.07 + 0.18 * Uniform();
					// The pitch of speech wanders.
					V.Center = FMath::Clamp(V.Center * (0.93 + 0.14 * Uniform()), 300.0, 1500.0);
				}
				V.Gain += (V.Target - V.Gain) * 0.0012;
				const double Lk = 1.0 - FMath::Exp(-2.0 * UE_DOUBLE_PI * V.Center * 1.6 * Dt);
				const double Hk = 1.0 - FMath::Exp(-2.0 * UE_DOUBLE_PI * V.Center * 0.55 * Dt);
				const double X = Noise();
				V.Low += (X - V.Low) * Lk;
				V.High += (V.Low - V.High) * Hk;
				S += (V.Low - V.High) * V.Gain * 0.11 * V.Pan;
			}
			// A laugh across the room now and then.
			if (Time >= NextLaugh)
			{
				LaughAt = Time;
				NextLaugh = Time + (8.0 + 22.0 * Uniform()) / Crowd;
			}
			const double SinceLaugh = Time - LaughAt;
			if (SinceLaugh < 1.1)
			{
				const double Ha = FMath::Max(0.0, FMath::Sin(2.0 * UE_DOUBLE_PI * 5.5 * SinceLaugh));
				LaughBand += (Noise() - LaughBand) * 0.08;
				S += LaughBand * Ha * Ha * 0.05 * (1.0 - SinceLaugh / 1.1);
			}

			// Chips: a riffle (one player's fingers working a stack), or a few chips set down.
			if (Time >= NextRiffle)
			{
				RiffleLeft = Uniform() < 0.6 ? 10 + static_cast<int32>(14.0 * Uniform()) : 2 + static_cast<int32>(4.0 * Uniform());
				RiffleGap = RiffleLeft > 6 ? 0.028 + 0.02 * Uniform() : 0.06 + 0.05 * Uniform();
				RiffleGain = 0.05 + 0.12 * Uniform();
				NextClick = Time;
				NextRiffle = Time + (0.4 + 2.2 * Uniform()) / Crowd;
			}
			if (RiffleLeft > 0 && Time >= NextClick)
			{
				ClickAt = Time;
				--RiffleLeft;
				NextClick = Time + RiffleGap * (0.8 + 0.4 * Uniform());
			}
			const double SinceClick = Time - ClickAt;
			if (SinceClick < 0.012)
			{
				Click += (Noise() - Click) * ClickK;
				S += (Noise() - Click) * FMath::Exp(-SinceClick * 600.0) * RiffleGain;
			}

			// A dealer's shuffle: two bursts of soft card flutter.
			if (Time >= NextShuffle)
			{
				ShuffleAt = Time;
				NextShuffle = Time + (14.0 + 30.0 * Uniform()) / Crowd;
			}
			const double SinceShuffle = Time - ShuffleAt;
			if (SinceShuffle < 1.3)
			{
				const double Burst = (SinceShuffle < 0.55 ? 1.0 : 0.0) + (SinceShuffle > 0.75 ? 1.0 : 0.0);
				const double Flutter = 0.5 + 0.5 * FMath::Sin(2.0 * UE_DOUBLE_PI * 38.0 * SinceShuffle);
				Shuffle += (Noise() - Shuffle) * 0.35;
				S += (Noise() - Shuffle) * Burst * Flutter * 0.02;
			}

			// The slot floor through the doorway: a soft chord bed and a winner's arpeggio sometimes.
			S += 0.0035 * (FMath::Sin(2.0 * UE_DOUBLE_PI * 523.25 * Time) + FMath::Sin(2.0 * UE_DOUBLE_PI * 659.25 * Time + 0.3) + FMath::Sin(2.0 * UE_DOUBLE_PI * 783.99 * Time + 1.2))
				* (0.55 + 0.45 * FMath::Sin(2.0 * UE_DOUBLE_PI * 0.07 * Time));
			if (Time >= NextJingle)
			{
				JingleAt = Time;
				JingleRoot = Uniform() < 0.5 ? 1046.5 : 880.0;
				NextJingle = Time + 9.0 + 25.0 * Uniform();
			}
			const double SinceJingle = Time - JingleAt;
			if (SinceJingle < 1.6)
			{
				static const double Steps[8] = {1.0, 1.25, 1.5, 2.0, 1.5, 2.0, 2.5, 3.0};
				const int32 Note = FMath::Min(7, static_cast<int32>(SinceJingle / 0.12));
				const double Local = SinceJingle - Note * 0.12;
				S += 0.012 * FMath::Sin(2.0 * UE_DOUBLE_PI * JingleRoot * Steps[Note] * SinceJingle) * FMath::Exp(-Local * 9.0) * (1.0 - SinceJingle / 1.6);
			}

			Out[I] = static_cast<float>(FMath::Clamp(S, -1.0, 1.0));
		}
	}

private:
	double Noise()
	{
		// xorshift32, -1..1
		State ^= State << 13;
		State ^= State >> 17;
		State ^= State << 5;
		return static_cast<double>(State) / 2147483648.0 - 1.0;
	}
	double Uniform() { return 0.5 + 0.5 * Noise(); }

	struct FVoice
	{
		bool bOn = false;
		double NextPhrase = 0.0;
		double NextSyllable = 0.0;
		double Target = 0.0;
		double Gain = 0.0;
		double Center = 700.0;
		double Pan = 1.0;
		double Low = 0.0;
		double High = 0.0;
	};
	static constexpr int32 NumVoices = 10;
	FVoice Voices[NumVoices];

	uint32 State = 1;
	double Time = 0.0;
	double Crowd = 1.0;
	double Brown = 0.0;
	double Air = 0.0;
	double NextLaugh = 6.0;
	double LaughAt = -10.0;
	double LaughBand = 0.0;
	double NextRiffle = 0.3;
	int32 RiffleLeft = 0;
	double RiffleGap = 0.03;
	double RiffleGain = 0.1;
	double NextClick = 0.0;
	double ClickAt = -1.0;
	double Click = 0.0;
	double NextShuffle = 5.0;
	double ShuffleAt = -10.0;
	double Shuffle = 0.0;
	double NextJingle = 4.0;
	double JingleAt = -10.0;
	double JingleRoot = 880.0;
};
