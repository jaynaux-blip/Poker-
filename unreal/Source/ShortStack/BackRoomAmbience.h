#pragma once

#include "CoreMinimal.h"

/**
 * The Back Room's room tone, synthesized: the running dryer through the wall (a low rumble swelling
 * with each turn of the drum, a zipper or a button clunking against it now and then) and the
 * fluorescent tube's ballast hum. Mono, 48 kHz, for UNightOneAudio::StartAmbience.
 */
class FBackRoomAmbience
{
public:
	explicit FBackRoomAmbience(uint32 Seed) : State(Seed | 1u) {}

	void Render(float* Out, int32 N)
	{
		constexpr double Rate = 48000.0;
		constexpr double Dt = 1.0 / Rate;
		// One-pole lowpass coefficients: the rumble around 140 Hz, the clunk's body around 400 Hz.
		const double RumbleK = 1.0 - FMath::Exp(-2.0 * UE_DOUBLE_PI * 140.0 * Dt);
		const double ThudK = 1.0 - FMath::Exp(-2.0 * UE_DOUBLE_PI * 400.0 * Dt);
		for (int32 I = 0; I < N; ++I)
		{
			Time += Dt;
			// The drum turns about 48 times a minute: the load lifts and falls with each turn.
			const double Turn = Time * 0.8;
			const double Swell = 0.62 + 0.38 * FMath::Sin(2.0 * UE_DOUBLE_PI * Turn) * FMath::Sin(2.0 * UE_DOUBLE_PI * Turn * 0.5 + 0.7);
			Brown = Brown * 0.995 + Noise() * 0.05;
			Rumble += (Brown - Rumble) * RumbleK;
			double S = Rumble * 0.9 * Swell;
			// Something hard in the load hits the drum once a turn or so.
			if (Time >= NextClunk)
			{
				ClunkAt = Time;
				ClunkGain = 0.25 + 0.35 * (0.5 + 0.5 * Noise());
				NextClunk = Time + (1.0 / 0.8) * (Noise() > 0.2 ? 1.0 : 2.0) + 0.05 * Noise();
			}
			const double Since = Time - ClunkAt;
			if (Since < 0.12)
			{
				const double Env = FMath::Exp(-Since * 45.0);
				Thud += (Noise() - Thud) * ThudK;
				S += ClunkGain * Env * (0.6 * Thud + 0.5 * FMath::Sin(2.0 * UE_DOUBLE_PI * 95.0 * Since));
			}
			// The ballast: 120 Hz and its harmonics, faint, wavering a little.
			const double Hum = 0.010 * FMath::Sin(2.0 * UE_DOUBLE_PI * 120.0 * Time) + 0.006 * FMath::Sin(2.0 * UE_DOUBLE_PI * 240.0 * Time + 0.4)
				+ 0.004 * FMath::Sin(2.0 * UE_DOUBLE_PI * 360.0 * Time + 1.1) + 0.0025 * FMath::Sin(2.0 * UE_DOUBLE_PI * 1080.0 * Time);
			S += Hum * (0.85 + 0.15 * FMath::Sin(2.0 * UE_DOUBLE_PI * 0.13 * Time));
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

	uint32 State = 1;
	double Time = 0.0;
	double Brown = 0.0;
	double Rumble = 0.0;
	double Thud = 0.0;
	double NextClunk = 0.6;
	double ClunkAt = -1.0;
	double ClunkGain = 0.0;
};
