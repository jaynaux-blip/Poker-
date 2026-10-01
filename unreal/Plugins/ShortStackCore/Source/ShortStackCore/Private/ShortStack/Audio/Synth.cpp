#include "ShortStack/Audio/Synth.h"
#include "../StrictFloat.h"

#include <cmath>

namespace ss
{
namespace audio
{
namespace synth_detail
{
const double SynthPi = 3.14159265358979323846;
const double SynthRate = static_cast<double>(SampleRate);

enum class FilterType : int
{
	LowPass,
	HighPass,
	BandPass,
};

enum class Wave : int
{
	Sine,
	Square,
	Triangle,
	Sawtooth,
};

/** RBJ biquad matching Web Audio's BiquadFilterNode (lowpass/highpass Q in dB). */
struct Biquad
{
	double B0 = 1.0, B1 = 0.0, B2 = 0.0, A1 = 0.0, A2 = 0.0;
	double X1 = 0.0, X2 = 0.0, Y1 = 0.0, Y2 = 0.0;

	void Set(FilterType T, double Freq, double Q)
	{
		const double F = Freq < 10.0 ? 10.0 : Freq > SynthRate * 0.49 ? SynthRate * 0.49 : Freq;
		const double W0 = 2.0 * SynthPi * F / SynthRate;
		const double Cw = std::cos(W0);
		const double Sw = std::sin(W0);
		double Alpha = 0.0;
		double A0 = 1.0;
		if (T == FilterType::BandPass)
		{
			Alpha = Sw / (2.0 * (Q > 1e-4 ? Q : 1e-4));
			B0 = Alpha;
			B1 = 0.0;
			B2 = -Alpha;
			A0 = 1.0 + Alpha;
			A1 = -2.0 * Cw;
			A2 = 1.0 - Alpha;
		}
		else
		{
			const double Ql = std::pow(10.0, Q / 20.0);
			Alpha = Sw / (2.0 * Ql);
			if (T == FilterType::LowPass)
			{
				B0 = (1.0 - Cw) / 2.0;
				B1 = 1.0 - Cw;
				B2 = (1.0 - Cw) / 2.0;
			}
			else
			{
				B0 = (1.0 + Cw) / 2.0;
				B1 = -(1.0 + Cw);
				B2 = (1.0 + Cw) / 2.0;
			}
			A0 = 1.0 + Alpha;
			A1 = -2.0 * Cw;
			A2 = 1.0 - Alpha;
		}
		B0 /= A0;
		B1 /= A0;
		B2 /= A0;
		A1 /= A0;
		A2 /= A0;
	}

	double Process(double X)
	{
		const double Y = B0 * X + B1 * X1 + B2 * X2 - A1 * Y1 - A2 * Y2;
		X2 = X1;
		X1 = X;
		Y2 = Y1;
		Y1 = Y;
		return Y;
	}
};

struct Noise
{
	uint32_t S = 1;
	explicit Noise(uint32_t Seed) : S(Seed * 2654435761u + 12345u) {}
	double Next()
	{
		S ^= S << 13;
		S ^= S >> 17;
		S ^= S << 5;
		return (static_cast<double>(S) / 4294967295.0) * 2.0 - 1.0;
	}
};

/** Web Audio style envelope: linear attack to Peak, then an exponential ramp to 0.0001 at Dur. */
double Envelope(double T, double Attack, double Peak, double Dur)
{
	if (T < 0.0 || Peak <= 0.0)
	{
		return 0.0;
	}
	if (T < Attack)
	{
		return Peak * T / Attack;
	}
	if (T >= Dur)
	{
		return 0.0;
	}
	return Peak * std::pow(0.0001 / Peak, (T - Attack) / (Dur - Attack));
}

struct Buffer
{
	std::vector<float> Data;
	void Ensure(double Seconds)
	{
		const size_t N = static_cast<size_t>(Seconds * SynthRate) + 1;
		if (Data.size() < N)
		{
			Data.resize(N, 0.0f);
		}
	}
};

/** Filtered noise burst (sound.ts burst()). */
void Burst(Buffer& Out, Noise& Rng, double Delay, double Freq, double Q, double Dur, double Gain, FilterType Type = FilterType::BandPass, double SweepTo = 0.0)
{
	if (Gain <= 0.0)
	{
		return;
	}
	Out.Ensure(Delay + Dur + 0.06);
	Biquad F;
	const size_t Start = static_cast<size_t>(Delay * SynthRate);
	const size_t N = static_cast<size_t>((Dur + 0.05) * SynthRate);
	for (size_t I = 0; I < N; ++I)
	{
		const double T = static_cast<double>(I) / SynthRate;
		const double Fq = SweepTo > 0.0 ? Freq * std::pow(SweepTo / Freq, T < Dur ? T / Dur : 1.0) : Freq;
		if (I % 32 == 0)
		{
			F.Set(Type, Fq, Q);
		}
		const double V = F.Process(Rng.Next()) * Envelope(T, 0.002, Gain, Dur);
		Out.Data[Start + I] += static_cast<float>(V);
	}
}

/** Oscillator tone (sound.ts tone()). */
void Tone(Buffer& Out, double Freq, double Dur, double Gain, double Delay = 0.0, Wave W = Wave::Sine, double SlideTo = 0.0)
{
	Out.Ensure(Delay + Dur + 0.06);
	const size_t Start = static_cast<size_t>(Delay * SynthRate);
	const size_t N = static_cast<size_t>((Dur + 0.05) * SynthRate);
	double Phase = 0.0;
	for (size_t I = 0; I < N; ++I)
	{
		const double T = static_cast<double>(I) / SynthRate;
		const double Fq = SlideTo > 0.0 ? Freq * std::pow(SlideTo / Freq, T < Dur ? T / Dur : 1.0) : Freq;
		Phase += Fq / SynthRate;
		Phase -= std::floor(Phase);
		double S = 0.0;
		switch (W)
		{
		case Wave::Square: S = Phase < 0.5 ? 1.0 : -1.0; break;
		case Wave::Triangle: S = 1.0 - 4.0 * std::fabs(Phase - 0.5); break;
		case Wave::Sawtooth: S = 2.0 * Phase - 1.0; break;
		default: S = std::sin(2.0 * SynthPi * Phase); break;
		}
		Out.Data[Start + I] += static_cast<float>(S * Envelope(T, 0.01, Gain, Dur));
	}
}

double Rand(Noise& R)
{
	return (R.Next() + 1.0) * 0.5;
}

void Clack(Buffer& Out, Noise& R, double Delay, double Gain)
{
	const double F1 = 3200.0 + Rand(R) * 1400.0;
	Burst(Out, R, Delay, F1, 9.0, 0.035, Gain);
	const double F2 = 900.0 + Rand(R) * 300.0;
	Burst(Out, R, Delay, F2, 4.0, 0.02, Gain * 0.4);
}

void Thump(Buffer& Out, double Delay, double Freq, double Gain)
{
	Tone(Out, Freq, 0.18, Gain, Delay, Wave::Sine, Freq * 0.6);
}

std::vector<float> Finish(Buffer& B)
{
	// sfx bus 0.8 and master 0.9, x2 for the compressor's makeup gain, then a soft limiter.
	for (float& S : B.Data)
	{
		S = static_cast<float>(std::tanh(static_cast<double>(S) * 0.72 * 2.0 * 1.2) / 1.2);
	}
	return B.Data;
}
} // namespace synth_detail

using namespace synth_detail;

std::vector<float> Render(SoundId Id, float Volume, uint32_t Seed)
{
	Buffer B;
	Noise R(Seed);
	const double V = Volume;
	switch (Id)
	{
	case SoundId::Chip:
		Clack(B, R, 0.0, 0.18 * V);
		break;
	case SoundId::ChipStack:
		for (int I = 0; I < 4; ++I)
		{
			Clack(B, R, I * (0.025 + Rand(R) * 0.03), 0.16 * V);
		}
		break;
	case SoundId::AllIn:
		for (int I = 0; I < 9; ++I)
		{
			Clack(B, R, I * 0.03 + Rand(R) * 0.02, 0.2 * V);
		}
		Tone(B, 70.0, 0.6, 0.35 * V, 0.02, Wave::Sine, 45.0);
		break;
	case SoundId::Deal:
		for (int I = 0; I < 4; ++I)
		{
			Burst(B, R, I * 0.09, 2200.0, 1.2, 0.07, 0.08 * V, FilterType::BandPass, 6000.0);
		}
		break;
	case SoundId::Flip:
		Burst(B, R, 0.0, 4200.0, 1.5, 0.03, 0.14 * V, FilterType::HighPass);
		break;
	case SoundId::Check:
		Tone(B, 190.0, 0.06, 0.2 * V, 0.0, Wave::Sine, 120.0);
		Tone(B, 190.0, 0.06, 0.2 * V, 0.1, Wave::Sine, 120.0);
		break;
	case SoundId::Fold:
		Burst(B, R, 0.0, 1500.0, 1.0, 0.12, 0.07 * V, FilterType::BandPass, 600.0);
		break;
	case SoundId::Turn:
		Tone(B, 880.0, 0.25, 0.12 * V);
		Tone(B, 1318.0, 0.35, 0.1 * V, 0.12);
		break;
	case SoundId::Alert:
		Tone(B, 1046.0, 0.3, 0.1 * V);
		break;
	case SoundId::Click:
		Tone(B, 1900.0, 0.02, 0.05 * V, 0.0, Wave::Square);
		break;
	case SoundId::Win:
		for (int I = 0; I < 8; ++I)
		{
			Clack(B, R, 0.1 + I * 0.035, 0.15 * V);
		}
		Tone(B, 523.0, 0.4, 0.07 * V, 0.0);
		Tone(B, 659.0, 0.4, 0.07 * V, 0.08);
		Tone(B, 784.0, 0.4, 0.07 * V, 0.16);
		break;
	case SoundId::Level:
		Tone(B, 1046.0, 1.2, 0.09 * V);
		Tone(B, 1568.0, 1.0, 0.05 * V);
		break;
	case SoundId::Bust:
		Tone(B, 220.0, 0.9, 0.18 * V, 0.0, Wave::Triangle, 98.0);
		break;
	case SoundId::Move:
		Burst(B, R, 0.0, 400.0, 0.7, 0.5, 0.08 * V, FilterType::BandPass, 3000.0);
		break;
	case SoundId::Bubble:
		Tone(B, 110.0, 2.2, 0.12 * V, 0.0, Wave::Sawtooth, 116.0);
		Tone(B, 165.0, 2.2, 0.06 * V, 0.05);
		break;
	case SoundId::Cash:
		Tone(B, 1568.0, 0.25, 0.1 * V);
		Tone(B, 2093.0, 0.4, 0.08 * V, 0.08);
		for (int I = 0; I < 6; ++I)
		{
			Clack(B, R, 0.15 + I * 0.03, 0.12 * V);
		}
		break;
	}
	return Finish(B);
}

std::vector<float> Render(Effect Id, uint32_t Seed)
{
	Buffer B;
	Noise R(Seed);
	switch (Id)
	{
	case Effect::Buzz:
		for (int K = 0; K < 2; ++K)
		{
			// Square 155 Hz through a 400 Hz lowpass, 0.3 s on, twice.
			Biquad Lp;
			Lp.Set(FilterType::LowPass, 400.0, 1.0);
			B.Ensure(K * 0.45 + 0.4);
			const size_t Start = static_cast<size_t>(K * 0.45 * SynthRate);
			for (size_t I = 0; I < static_cast<size_t>(0.35 * SynthRate); ++I)
			{
				const double T = static_cast<double>(I) / SynthRate;
				const double Ph = std::fmod(T * 155.0, 1.0);
				const double G = T < 0.02 ? 0.18 * T / 0.02 : T < 0.28 ? 0.18 : T < 0.32 ? 0.18 * (0.32 - T) / 0.04 : 0.0;
				B.Data[Start + I] += static_cast<float>(Lp.Process(Ph < 0.5 ? 1.0 : -1.0) * G);
			}
		}
		break;
	case Effect::CanOpen:
		Burst(B, R, 0.0, 900.0, 3.0, 0.05, 0.25);
		Burst(B, R, 0.04, 6000.0, 0.8, 0.45, 0.12, FilterType::HighPass);
		break;
	case Effect::Thump:
		Thump(B, 0.0, 62.0, 0.35);
		Thump(B, 0.16, 52.0, 0.25);
		break;
	case Effect::Step:
		// The heel's low thud, the tap of the sole, a little grit underfoot.
		Burst(B, R, 0.0, 150.0, 0.9, 0.08, 0.34, FilterType::LowPass);
		Burst(B, R, 0.012, 1800.0 + Rand(R) * 700.0, 3.0, 0.03, 0.07);
		Burst(B, R, 0.035, 5200.0, 0.7, 0.08, 0.02 + 0.015 * Rand(R), FilterType::HighPass);
		break;
	case Effect::Scrape:
		// A rough, rising groan from the legs, the frame's rattle, and the weight shifting.
		Burst(B, R, 0.0, 480.0, 7.0, 0.42, 0.22, FilterType::BandPass, 760.0);
		Burst(B, R, 0.03, 1400.0, 5.0, 0.34, 0.07, FilterType::BandPass, 2000.0);
		Burst(B, R, 0.0, 170.0, 1.0, 0.3, 0.14, FilterType::LowPass);
		break;
	case Effect::Breath:
	{
		// Air through the nose: band-limited noise that swells and fades over a second and a half.
		const double Dur = 1.6;
		B.Ensure(Dur + 0.05);
		Biquad F;
		for (size_t I = 0; I < static_cast<size_t>(Dur * SynthRate); ++I)
		{
			const double T = static_cast<double>(I) / SynthRate;
			if (I % 32 == 0)
			{
				F.Set(FilterType::BandPass, 900.0 - 350.0 * T / Dur, 0.7);
			}
			const double Env = std::pow(std::sin(SynthPi * T / Dur), 2.0) * (1.0 - 0.3 * T / Dur);
			B.Data[I] += static_cast<float>(F.Process(R.Next()) * 0.07 * Env);
		}
		break;
	}
	}
	return Finish(B);
}

std::vector<float> RenderThunder(float Strength, uint32_t Seed)
{
	Buffer B;
	B.Ensure(5.2);
	Noise R(Seed);
	Biquad Lp;
	double Brown = 0.0;
	for (size_t I = 0; I < B.Data.size(); ++I)
	{
		const double T = static_cast<double>(I) / SynthRate;
		if (I % 32 == 0)
		{
			Lp.Set(FilterType::LowPass, 420.0 * std::pow(90.0 / 420.0, T < 4.0 ? T / 4.0 : 1.0), 1.0);
		}
		Brown = (Brown + 0.02 * R.Next()) / 1.02;
		double G = 0.0;
		if (T < 0.25)
		{
			G = 0.9 * Strength * T / 0.25;
		}
		else if (T < 1.2)
		{
			G = 0.9 * Strength * std::pow(0.4 / 0.9, (T - 0.25) / 0.95);
		}
		else if (T < 5.0)
		{
			G = 0.4 * Strength * std::pow(0.0001 / 0.4, (T - 1.2) / 3.8);
		}
		B.Data[I] = static_cast<float>(Lp.Process(Brown * 3.5) * G);
	}
	for (float& S : B.Data)
	{
		S = static_cast<float>(std::tanh(static_cast<double>(S) * 0.9));
	}
	return B.Data;
}

// ------------------------------------------------------------------ ambience

double Ambience::Filter::Process(double X)
{
	const double Y = B0 * X + B1 * X1 + B2 * X2 - A1 * Y1 - A2 * Y2;
	X2 = X1;
	X1 = X;
	Y2 = Y1;
	Y1 = Y;
	return Y;
}

namespace synth_detail
{
template <typename T>
void Configure(T& F, FilterType Type, double Freq, double Q)
{
	Biquad B;
	B.Set(Type, Freq, Q);
	F.B0 = B.B0;
	F.B1 = B.B1;
	F.B2 = B.B2;
	F.A1 = B.A1;
	F.A2 = B.A2;
}
} // namespace synth_detail

Ambience::Ambience(uint32_t Seed)
	: State(Seed * 2654435761u + 7u)
{
	Configure(HissBp, FilterType::BandPass, 2400.0, 0.5);
	Configure(HissLp, FilterType::LowPass, 5200.0, 1.0);
	Configure(BodyLp, FilterType::LowPass, 700.0, 1.0);
	Configure(FanLp, FilterType::LowPass, 380.0, 1.0);
	Configure(DripBp, FilterType::BandPass, 3000.0, 6.0);
}

double Ambience::NextNoise()
{
	State ^= State << 13;
	State ^= State >> 17;
	State ^= State << 5;
	return (static_cast<double>(State) / 4294967295.0) * 2.0 - 1.0;
}

void Ambience::Render(float* Out, int N)
{
	const double Dt = 1.0 / SynthRate;
	for (int I = 0; I < N; ++I)
	{
		const double Fade = Time < 3.0 ? Time / 3.0 : 1.0; // ambience fades in over 3 s
		// Rain hiss on glass (muffled by the window).
		const double Hiss = HissLp.Process(HissBp.Process(NextNoise())) * 0.05;
		// Heavy rain body with a slow swell.
		Brown = (Brown + 0.02 * NextNoise()) / 1.02;
		const double Swell = 0.12 + 0.05 * std::sin(2.0 * SynthPi * 0.07 * Time);
		const double Body = BodyLp.Process(Brown * 3.5) * Swell;
		// Room tone: fridge hum and the laptop fan.
		const double Hum = std::sin(2.0 * SynthPi * 58.0 * Time) * 0.008;
		const double Fan = FanLp.Process(NextNoise()) * 0.025;
		// Drips: short resonant ticks every 50-300 ms.
		if (Time >= NextDrip)
		{
			NextDrip = Time + 0.05 + (NextNoise() + 1.0) * 0.125;
			DripStart = Time;
			DripUntil = Time + 0.02;
			DripGain = 0.02 + (NextNoise() + 1.0) * 0.015;
			Configure(DripBp, FilterType::BandPass, 1800.0 + (NextNoise() + 1.0) * 1750.0, 6.0);
		}
		double Drip = 0.0;
		const double Dn = NextNoise();
		if (Time < DripUntil + 0.05)
		{
			Drip = DripBp.Process(Dn) * Envelope(Time - DripStart, 0.002, DripGain, 0.02);
		}
		const double S = (Hiss + Body + Hum + Fan + Drip) * 0.9 * Fade * 0.9 * 2.0;
		Out[I] = static_cast<float>(std::tanh(S * 1.2) / 1.2);
		Time += Dt;
	}
}
} // namespace audio
} // namespace ss
