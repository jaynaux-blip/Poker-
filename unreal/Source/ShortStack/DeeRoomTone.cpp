#include "DeeRoomTone.h"

#include <cmath>

namespace DeeRoomToneDetail
{
constexpr double Rate = static_cast<double>(ss::audio::SampleRate);
constexpr double TwoPi = 6.283185307179586;

/** A level that eases toward Goal over about Seconds (one step per sample). */
double Glide(double Value, double Goal, double Seconds)
{
	return Value + (Goal - Value) * (1.0 - std::exp(-1.0 / (Rate * Seconds)));
}
} // namespace DeeRoomToneDetail

using namespace DeeRoomToneDetail;

// ------------------------------------------------------------------ filters

void FDeeRoomTone::FBiquad::LowPass(double Hz, double Q)
{
	const double W = TwoPi * Hz / Rate;
	const double Alpha = std::sin(W) / (2.0 * Q);
	const double C = std::cos(W);
	const double A0 = 1.0 + Alpha;
	B0 = (1.0 - C) * 0.5 / A0;
	B1 = (1.0 - C) / A0;
	B2 = B0;
	A1 = -2.0 * C / A0;
	A2 = (1.0 - Alpha) / A0;
}

void FDeeRoomTone::FBiquad::HighPass(double Hz, double Q)
{
	const double W = TwoPi * Hz / Rate;
	const double Alpha = std::sin(W) / (2.0 * Q);
	const double C = std::cos(W);
	const double A0 = 1.0 + Alpha;
	B0 = (1.0 + C) * 0.5 / A0;
	B1 = -(1.0 + C) / A0;
	B2 = B0;
	A1 = -2.0 * C / A0;
	A2 = (1.0 - Alpha) / A0;
}

void FDeeRoomTone::FBiquad::BandPass(double Hz, double Q)
{
	const double W = TwoPi * Hz / Rate;
	const double Alpha = std::sin(W) / (2.0 * Q);
	const double C = std::cos(W);
	const double A0 = 1.0 + Alpha;
	B0 = Alpha / A0;
	B1 = 0.0;
	B2 = -Alpha / A0;
	A1 = -2.0 * C / A0;
	A2 = (1.0 - Alpha) / A0;
}

double FDeeRoomTone::FBiquad::Process(double X)
{
	const double Y = B0 * X + B1 * X1 + B2 * X2 - A1 * Y1 - A2 * Y2;
	X2 = X1;
	X1 = X;
	Y2 = Y1;
	Y1 = Y;
	return Y;
}

// ------------------------------------------------------------------ the room

FDeeRoomTone::FDeeRoomTone(uint32 Seed)
	: Room(Seed), State(Seed * 2654435761u + 1u)
{
	HissBand.BandPass(5200.0, 0.7);
	// A small TV's speaker: no bass, nothing much over 3.5 kHz.
	SpeakerHigh.HighPass(260.0, 0.7);
	SpeakerLow.LowPass(3400.0, 0.7);
	Voice.BandPass(900.0, 0.9);
	CrowdBand.BandPass(1300.0, 0.6);
	NextTick = 1.0 + 3.0 * (Noise() * 0.5 + 0.5);
}

double FDeeRoomTone::Noise()
{
	State ^= State << 13;
	State ^= State >> 17;
	State ^= State << 5;
	return static_cast<double>(State) / 2147483647.5 - 1.0;
}

double FDeeRoomTone::Random()
{
	return Noise() * 0.5 + 0.5;
}

void FDeeRoomTone::Ping(double Hz, double Amp, double Seconds)
{
	if (Rings.Num() >= 24)
	{
		Rings.RemoveAt(0);
	}
	FRing R;
	R.Step = TwoPi * Hz / Rate;
	R.Amp = Amp;
	// To a thousandth of its strength over Seconds.
	R.Decay = std::exp(std::log(0.001) / (Seconds * Rate));
	Rings.Add(R);
}

void FDeeRoomTone::ScheduleRadiator()
{
	// A cast-iron radiator heating up: ticks in little runs as the iron moves (a bright ping, a second partial over it),
	// and now and then the knock of water in the pipe. Less of both as the steam backs off by day.
	const double Warm = 0.25 + 0.75 * static_cast<double>(Heat);
	if (Clock >= NextTick)
	{
		const double Hz = 1700.0 + 2600.0 * Random();
		const double Amp = (0.012 + 0.03 * Random()) * Warm;
		Ping(Hz, Amp, 0.03 + 0.05 * Random());
		Ping(Hz * (2.3 + 0.4 * Random()), Amp * 0.4, 0.02 + 0.02 * Random());
		if (BurstLeft > 0)
		{
			--BurstLeft;
			NextTick = Clock + 0.08 + 0.5 * Random();
		}
		else
		{
			BurstLeft = static_cast<int32>(Random() * 5.0);
			NextTick = Clock + (2.5 + 11.0 * Random()) / Warm;
		}
	}
	if (Clock >= NextKnock)
	{
		const double Amp = (0.05 + 0.05 * Random()) * Warm;
		Ping(70.0 + 50.0 * Random(), Amp, 0.14);
		Ping(620.0 + 300.0 * Random(), Amp * 0.35, 0.32);
		Ping(1450.0 + 400.0 * Random(), Amp * 0.15, 0.2);
		NextKnock = Clock + (30.0 + 70.0 * Random()) / Warm;
	}
}

void FDeeRoomTone::Render(float* Out, int32 N)
{
	if (!Out || N <= 0)
	{
		return;
	}
	Room.Render(Out, N);
	const double Dt = 1.0 / Rate;
	for (int32 I = 0; I < N; ++I)
	{
		Clock += Dt;
		ScheduleRadiator();
		double Radiator = 0.0;
		for (FRing& R : Rings)
		{
			Radiator += std::sin(R.Phase) * R.Amp;
			R.Phase += R.Step;
			R.Amp *= R.Decay;
		}
		// The vent valve's hiss, breathing slowly while the steam's up.
		HissLevel = Glide(HissLevel, 0.0035 * static_cast<double>(Heat) * (0.6 + 0.4 * std::sin(Clock * 0.21) * std::sin(Clock * 0.077)), 0.5);
		Radiator += HissBand.Process(Noise()) * HissLevel;

		// The TV across the room: someone talking (a voiced buzz through the mouth's band, the consonants as hiss), in
		// syllables and phrases, now and then a studio audience laughing, a little music under it; the set's own speaker.
		TvGain = Glide(TvGain, bTvOn ? 1.0 : 0.0, 0.35);
		double Tv = 0.0;
		if (TvGain > 0.0005)
		{
			if (Clock >= NextSyllable)
			{
				if (Clock >= PhraseEnd)
				{
					// A new phrase after a pause, the voice's pitch somewhere new (another speaker, another line).
					PhraseEnd = Clock + 0.8 + 2.4 * Random();
					NextSyllable = Clock + 0.2 + 0.9 * Random();
					PitchGoal = 110.0 + 130.0 * Random();
				}
				else
				{
					SyllableEnd = Clock + 0.07 + 0.18 * Random();
					NextSyllable = SyllableEnd + 0.02 + 0.12 * Random();
					PitchGoal *= 0.94 + 0.12 * Random();
				}
			}
			Syllable = Glide(Syllable, Clock < SyllableEnd ? 1.0 : 0.0, 0.025);
			Pitch = Glide(Pitch, PitchGoal, 0.08);
			GlottalPhase += Pitch / Rate;
			GlottalPhase -= std::floor(GlottalPhase);
			const double Buzz = (GlottalPhase * 2.0 - 1.0) * 0.6 + Noise() * 0.25;
			double Talk = Voice.Process(Buzz) * Syllable * 0.08;
			if (Clock >= NextLaugh)
			{
				LaughStart = Clock;
				LaughEnd = Clock + 1.4 + 1.6 * Random();
				NextLaugh = LaughEnd + 35.0 + 80.0 * Random();
			}
			if (Clock < LaughEnd)
			{
				const double T = (Clock - LaughStart) / FMath::Max(0.01, LaughEnd - LaughStart);
				const double Swell = std::sin(T * 3.14159265) * (0.7 + 0.3 * std::sin(Clock * TwoPi * 5.5));
				Talk += CrowdBand.Process(Noise()) * Swell * 0.05;
			}
			Bed = Glide(Bed, std::fmod(Clock, 47.0) < 19.0 ? 1.0 : 0.0, 2.0);
			Talk += (std::sin(Clock * TwoPi * 220.0) + 0.6 * std::sin(Clock * TwoPi * 277.2) + 0.5 * std::sin(Clock * TwoPi * 329.6)) * Bed * 0.004 *
					(0.6 + 0.4 * std::sin(Clock * TwoPi * 0.5));
			Tv = SpeakerLow.Process(SpeakerHigh.Process(Talk)) * TvGain;
		}
		Out[I] += static_cast<float>(Radiator + Tv);
	}
	Rings.RemoveAll([](const FRing& R) { return R.Amp < 1.0e-5; });
}

std::vector<float> FDeeRoomTone::RenderDoor(uint32 Seed)
{
	// The door swings into its frame: a dull thud in the wood (low, short), the frame's rattle, then the latch.
	FDeeRoomTone Gen(Seed);
	const int32 Count = static_cast<int32>(Rate * 0.7);
	std::vector<float> Out(static_cast<size_t>(Count), 0.0f);
	FBiquad Body;
	Body.LowPass(220.0, 0.8);
	FBiquad Latch;
	Latch.BandPass(3200.0, 3.0);
	for (int32 I = 0; I < Count; ++I)
	{
		const double T = static_cast<double>(I) / Rate;
		double S = 0.0;
		S += std::sin(TwoPi * 68.0 * T) * std::exp(-T / 0.07) * 0.5;
		S += Body.Process(Gen.Noise()) * std::exp(-T / 0.05) * 1.4;
		const double Rattle = T - 0.03;
		if (Rattle > 0.0)
		{
			S += std::sin(TwoPi * 410.0 * Rattle) * std::exp(-Rattle / 0.09) * 0.07;
		}
		const double Click = T - 0.11;
		if (Click > 0.0)
		{
			S += Latch.Process(Gen.Noise()) * std::exp(-Click / 0.012) * 0.6;
		}
		Out[static_cast<size_t>(I)] = static_cast<float>(FMath::Clamp(S, -1.0, 1.0));
	}
	return Out;
}
