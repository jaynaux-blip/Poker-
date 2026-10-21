#include "ShortStack/Audio/Synth.h"
#include "../StrictFloat.h"

#include <algorithm>
#include <cmath>

#include "VoiceEngine.h"

// The voice engine keeps its own copy of the rate (it doesn't pull in Session.h); the two must agree.
static_assert(static_cast<int>(ss::audio::vox::OutRate) == ss::audio::SampleRate, "VoiceEngine.h OutRate must equal Synth.h SampleRate");

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

// ------------------------------------------------------------------ Update 3: the revamped sites' ten cues (SPEC §1.11)
//
// Ignite's five are the arcade on the boardwalk: a bright chirp, a party cracker, a plastic flapper, a burning fuse and a
// firework. RunItTwice's five are the exchange after the bell: a terminal's tick, split-flaps settling, the desk bell, a
// rubber stamp, an odometer's ratchet and two chrome notes. The host renders a cue on the game thread when the UI asks for
// it (UNightOneAudio::Play), as often as 20 times a second during a count-up, so every voice here runs in one pass with
// running envelopes and turning phasors (no pow() or sin() a sample where it can be helped).
//
// UiSound passes only an id and a volume (§7.2), and the volumes stay within 0.25-0.6 (§10), so two cues take their
// variant from the volume asked for (UiSound's default, 0.4, is the everyday one of each):
//   IgBoom   0.25-0.45: the multiplier lands (about 0.8 s); 0.5-0.6: the long tail, for x25 and up (about 2.3 s).
//   RitRank  0.25-0.4: one tick per place gained; 0.45: two notes falling (a relegation); 0.5-0.6: two notes rising (a
//            promotion). The better the news, the louder.
// IgFuse is a loop segment: 0.52 s, its level steady between equal-power fades of 20 ms; fired again every 0.5 s while the
// fuse burns, the segments cross-fade into one fuse (each crackles differently: the host seeds every render afresh).

/** The volume from which IgBoom renders its long tail. */
const double CueBoomLongVol = 0.5;
/** RitRank's variants: the tick under CueRankFallVol, the falling notes up to CueRankRiseVol, the rising ones from it (the
 *  edges sit between the volumes callers use, clear of float rounding: 0.4f is 0.4000000060 as a double). */
const double CueRankFallVol = 0.425;
const double CueRankRiseVol = 0.5;
/** IgFuse's segment and its fades; fire it every CueFuseLen - CueFuseFade seconds to hold a longer fuse. */
const double CueFuseLen = 0.52;
const double CueFuseFade = 0.02;
/** No cue peaks past the top of §10's volume range, whatever seed the host renders it with: the booms' noise can line up a
 *  little past it (about 1 render in 20 of the long tail at 0.6), and such a render is scaled just under it (1 dB at most). */
const float CueCeiling = 0.59f;

/** The cues' white noise, uniform on [-1, 1): xorshift32 scaled by a multiply (Noise::Next divides, a sample at a time),
 *  seeded by hashing the render's Noise, which steps on so the next voice's noise is its own. */
struct CueWhite
{
	uint32_t State = 1u;
	explicit CueWhite(Noise& R)
	{
		uint32_t H = R.S + 0x9e3779b9u;
		H ^= H >> 16;
		H *= 0x85ebca6bu;
		H ^= H >> 13;
		H *= 0xc2b2ae35u;
		H ^= H >> 16;
		State = H != 0u ? H : 1u;
		R.Next();
	}
	double Next()
	{
		State ^= State << 13;
		State ^= State >> 17;
		State ^= State << 5;
		return static_cast<double>(static_cast<int32_t>(State)) * (1.0 / 2147483648.0);
	}
};

/** 1 / the RMS of uniform noise (variance 1/3) through a filter, roughly: a resonator passes pi/2 of its -3 dB width, a
 *  lowpass its cutoff, a highpass the rest. */
double CueNoiseNorm(FilterType Type, double Freq, double Q)
{
	const double Nyquist = SynthRate * 0.5;
	double Share = 1.0;
	if (Type == FilterType::BandPass)
	{
		Share = 0.5 * SynthPi * Freq / (Q > 0.1 ? Q : 0.1) / Nyquist;
	}
	else if (Type == FilterType::LowPass)
	{
		Share = Freq / Nyquist;
	}
	else
	{
		Share = 1.0 - Freq / Nyquist;
	}
	Share = Share < 0.002 ? 0.002 : Share > 1.0 ? 1.0 : Share;
	return 1.0 / std::sqrt(Share / 3.0);
}

/** A voice's release, in samples: the last quarter of its length, Shortest to 250 ms, so a ring cut off early fades out
 *  rather than stops. */
double CueFadeN(double Len, double Shortest)
{
	const double Quarter = 0.25 * Len;
	return (Quarter < Shortest ? Shortest : Quarter > 0.25 ? 0.25 : Quarter) * SynthRate;
}

/**
 * A partial: its frequency glides exponentially from F0 to F1 over Glide seconds (no glide when F1 is 0), under a linear
 * attack and then an exponential decay with time constant Tau, cut at Len after a release (CueFadeN, 3 ms at least). A
 * sine turns a phasor.
 */
void CueOsc(Buffer& Out, double At, double F0, double F1, double Glide, double Peak, double Attack, double Tau, double Len, Wave W = Wave::Sine)
{
	if (!(Peak > 0.0) || !(Len > 0.0) || !(F0 > 0.0) || !(Tau > 0.0))
	{
		return;
	}
	Out.Ensure(At + Len);
	const size_t Start = static_cast<size_t>(At * SynthRate);
	const size_t N = std::min(static_cast<size_t>(Len * SynthRate), Out.Data.size() - Start);
	const double AttackN = Attack * SynthRate > 1.0 ? Attack * SynthRate : 1.0;
	const double FadeN = CueFadeN(Len, 0.003);
	const double Decay = std::exp(-1.0 / (Tau * SynthRate));
	const bool Glides = F1 > 0.0 && Glide > 0.0;
	const double GlideN = Glides ? Glide * SynthRate + 16.0 : 0.0;
	double Held = Peak;
	double Fq = F0;
	double Re = 1.0, Im = 0.0, StepRe = 1.0, StepIm = 0.0, Phase = 0.0;
	for (size_t I = 0; I < N; ++I)
	{
		// The step turns with the glide every 16 samples (and once without one); the phasor's length is held every 256.
		if (I == 0 || (Glides && I % 16 == 0 && static_cast<double>(I) < GlideN))
		{
			const double T = static_cast<double>(I) / SynthRate;
			Fq = Glides ? F0 * std::pow(F1 / F0, T < Glide ? T / Glide : 1.0) : F0;
			const double W0 = 2.0 * SynthPi * Fq / SynthRate;
			StepRe = std::cos(W0);
			StepIm = std::sin(W0);
		}
		if (I % 256 == 0)
		{
			const double Mag = std::sqrt(Re * Re + Im * Im);
			Re /= Mag;
			Im /= Mag;
		}
		double Wv = 0.0;
		if (W == Wave::Sine)
		{
			Wv = Im;
			const double NextRe = Re * StepRe - Im * StepIm;
			Im = Im * StepRe + Re * StepIm;
			Re = NextRe;
		}
		else
		{
			Phase += Fq / SynthRate;
			Phase -= Phase >= 1.0 ? 1.0 : 0.0;
			Wv = W == Wave::Square ? (Phase < 0.5 ? 1.0 : -1.0) : W == Wave::Triangle ? 1.0 - 4.0 * std::fabs(Phase - 0.5) : 2.0 * Phase - 1.0;
		}
		const double D = static_cast<double>(I);
		double Gain = Held;
		if (D < AttackN)
		{
			Gain = Peak * D / AttackN;
		}
		else
		{
			Held *= Decay;
		}
		const double Left = static_cast<double>(N - I);
		if (Left < FadeN)
		{
			Gain *= Left / FadeN;
		}
		Out.Data[Start + I] += static_cast<float>(Wv * Gain);
	}
}

/**
 * Noise through a biquad (sweeping F0 to F1 over Sweep seconds when F1 is set), scaled to unit RMS for its band, under a
 * linear attack and an exponential decay with time constant Tau; cut at Len after a release (CueFadeN, 2 ms at least).
 * Peak is the RMS at the top.
 */
void CueHiss(Buffer& Out, Noise& R, double At, FilterType Type, double F0, double F1, double Sweep, double Q, double Peak, double Attack, double Tau, double Len)
{
	if (!(Peak > 0.0) || !(Len > 0.0) || !(F0 > 0.0) || !(Tau > 0.0))
	{
		return;
	}
	Out.Ensure(At + Len);
	const size_t Start = static_cast<size_t>(At * SynthRate);
	const size_t N = std::min(static_cast<size_t>(Len * SynthRate), Out.Data.size() - Start);
	const double AttackN = Attack * SynthRate > 1.0 ? Attack * SynthRate : 1.0;
	const double FadeN = CueFadeN(Len, 0.002);
	const double Decay = std::exp(-1.0 / (Tau * SynthRate));
	const bool Sweeps = F1 > 0.0 && Sweep > 0.0;
	CueWhite White(R);
	Biquad F;
	double Norm = 1.0;
	double Held = Peak;
	for (size_t I = 0; I < N; ++I)
	{
		if (I == 0 || (Sweeps && I % 32 == 0))
		{
			const double T = static_cast<double>(I) / SynthRate;
			const double Fq = Sweeps ? F0 * std::pow(F1 / F0, T < Sweep ? T / Sweep : 1.0) : F0;
			F.Set(Type, Fq, Q);
			Norm = CueNoiseNorm(Type, Fq, Q);
		}
		const double D = static_cast<double>(I);
		double Gain = Held;
		if (D < AttackN)
		{
			Gain = Peak * D / AttackN;
		}
		else
		{
			Held *= Decay;
		}
		const double Left = static_cast<double>(N - I);
		if (Left < FadeN)
		{
			Gain *= Left / FadeN;
		}
		Out.Data[Start + I] += static_cast<float>(F.Process(White.Next()) * Norm * Gain);
	}
}

/** One crackle: a click of noise in a band (a spark, a scrap of paper, a firework's star). */
void CueCrack(Buffer& Out, Noise& R, double At, double Freq, double Q, double Peak, double Tau)
{
	CueHiss(Out, R, At, FilterType::BandPass, Freq, 0.0, 0.0, Q, Peak, 0.0002, Tau, Tau * 6.0 + 0.002);
}

/**
 * A two-operator FM note, the chrome bell: the carrier's phase moved by a modulator at Ratio times its frequency, whose index
 * falls from Index with time constant IndexTau (bright on the strike, purer as it rings); a linear attack, then a decay of
 * Tau, cut at Len after a release (CueFadeN, 3 ms at least).
 */
void CueFm(Buffer& Out, double At, double Carrier, double Ratio, double Index, double IndexTau, double Peak, double Attack, double Tau, double Len)
{
	if (!(Peak > 0.0) || !(Len > 0.0) || !(Carrier > 0.0) || !(Tau > 0.0) || !(IndexTau > 0.0))
	{
		return;
	}
	Out.Ensure(At + Len);
	const size_t Start = static_cast<size_t>(At * SynthRate);
	const size_t N = std::min(static_cast<size_t>(Len * SynthRate), Out.Data.size() - Start);
	const double AttackN = Attack * SynthRate > 1.0 ? Attack * SynthRate : 1.0;
	const double FadeN = CueFadeN(Len, 0.003);
	const double Decay = std::exp(-1.0 / (Tau * SynthRate));
	const double IndexDecay = std::exp(-1.0 / (IndexTau * SynthRate));
	const double W0 = 2.0 * SynthPi * Carrier * Ratio / SynthRate;
	const double StepRe = std::cos(W0), StepIm = std::sin(W0);
	double ModRe = 1.0, ModIm = 0.0, Phase = 0.0, Depth = Index, Held = Peak;
	for (size_t I = 0; I < N; ++I)
	{
		if (I % 256 == 0)
		{
			const double Mag = std::sqrt(ModRe * ModRe + ModIm * ModIm);
			ModRe /= Mag;
			ModIm /= Mag;
		}
		const double Wv = std::sin(2.0 * SynthPi * Phase + Depth * ModIm);
		Phase += Carrier / SynthRate;
		Phase -= Phase >= 1.0 ? 1.0 : 0.0;
		const double NextRe = ModRe * StepRe - ModIm * StepIm;
		ModIm = ModIm * StepRe + ModRe * StepIm;
		ModRe = NextRe;
		Depth *= IndexDecay;
		const double D = static_cast<double>(I);
		double Gain = Held;
		if (D < AttackN)
		{
			Gain = Peak * D / AttackN;
		}
		else
		{
			Held *= Decay;
		}
		const double Left = static_cast<double>(N - I);
		if (Left < FadeN)
		{
			Gain *= Left / FadeN;
		}
		Out.Data[Start + I] += static_cast<float>(Wv * Gain);
	}
}

/** IgSpark: a rising tick per count step (Sparks counting up, a Dare's check): a triangle chirp up a sixth, G6 to E7 in
 *  35 ms, with a spark of hiss on its front. 75 ms, so steps 50 ms apart stay ticks. */
void CueIgSpark(Buffer& B, Noise& R, double V)
{
	CueOsc(B, 0.0, 1568.0, 2637.0, 0.035, 0.07 * V, 0.0015, 0.022, 0.075, Wave::Triangle);
	CueHiss(B, R, 0.0, FilterType::HighPass, 7000.0, 0.0, 0.0, 0.0, 0.025 * V, 0.0003, 0.004, 0.016);
}

/** IgPop: a cracker, a sticker slam, a level-up: the snap, the pop's weight (a sine dropping 300 to 110 Hz), the arcade's
 *  bloop (A5 leaping to E6) and paper streamers fluttering down. About 0.4 s. */
void CueIgPop(Buffer& B, Noise& R, double V)
{
	CueHiss(B, R, 0.0, FilterType::HighPass, 1400.0, 0.0, 0.0, 0.0, 0.1 * V, 0.0002, 0.006, 0.03);
	CueOsc(B, 0.0, 300.0, 110.0, 0.05, 0.12 * V, 0.001, 0.035, 0.13);
	CueOsc(B, 0.008, 880.0, 1320.0, 0.04, 0.07 * V, 0.002, 0.05, 0.16);
	double At = 0.035;
	for (int K = 0; K < 12 && At < 0.36; ++K)
	{
		const double Thin = 1.0 - At / 0.4;
		CueCrack(B, R, At, 2600.0 + 1600.0 * Rand(R), 2.0, 0.035 * V * Thin * (0.5 + 0.5 * Rand(R)), 0.004);
		At += 0.016 + 0.022 * Rand(R);
	}
}

/** IgTick: a wheel's wedge, a reel's symbol: a plastic flapper snapping off a peg (a hollow tock dropping from A6 to about
 *  F#6 in 4 ms, its octave, and a bright click), and its smaller rebound 11 ms on. About 40 ms. */
void CueIgTick(Buffer& B, Noise& R, double V)
{
	for (int K = 0; K < 2; ++K)
	{
		const double At = 0.011 * K, Strike = K == 0 ? 1.0 : 0.35;
		CueOsc(B, At, 1760.0, 1480.0, 0.004, 0.07 * V * Strike, 0.0003, 0.006, 0.028);
		CueOsc(B, At, 3520.0, 2960.0, 0.004, 0.015 * V * Strike, 0.0003, 0.003, 0.014);
		CueHiss(B, R, At, FilterType::HighPass, 6000.0, 0.0, 0.0, 0.0, 0.03 * V * Strike, 0.0001, 0.0008, 0.005);
	}
}

/** IgFuse: one loop segment of a burning fuse: a sizzle in two bands (the top one sputtering) and crackles, most small and a
 *  few big, between equal-power fades (CueFuseLen, CueFuseFade). */
void CueIgFuse(Buffer& B, Noise& R, double V)
{
	B.Ensure(CueFuseLen);
	const size_t N = static_cast<size_t>(CueFuseLen * SynthRate);
	Biquad Top, Low;
	Top.Set(FilterType::BandPass, 3600.0, 0.9);
	Low.Set(FilterType::BandPass, 1300.0, 1.2);
	const double TopNorm = CueNoiseNorm(FilterType::BandPass, 3600.0, 0.9), LowNorm = CueNoiseNorm(FilterType::BandPass, 1300.0, 1.2);
	CueWhite White(R);
	// The sputter: the top band's level walks between random targets 25-60 ms apart.
	double From = 0.8, To = 0.8, SpanAt = 0.0, SpanLen = 0.03;
	for (size_t I = 0; I < N; ++I)
	{
		const double T = static_cast<double>(I) / SynthRate;
		if (T >= SpanAt + SpanLen)
		{
			SpanAt = T;
			SpanLen = 0.025 + 0.035 * Rand(R);
			From = To;
			To = 0.55 + 0.45 * Rand(R);
		}
		const double Sputter = From + (To - From) * (T - SpanAt) / SpanLen;
		const double Sizzle = Top.Process(White.Next()) * TopNorm * 0.014 * Sputter + Low.Process(White.Next()) * LowNorm * 0.006;
		B.Data[I] += static_cast<float>(Sizzle * V);
	}
	double At = 0.004 + 0.02 * Rand(R);
	while (At < CueFuseLen - 0.012)
	{
		const double Size = Rand(R);
		if (Rand(R) < 0.25)
		{
			CueCrack(B, R, At, 700.0 + 400.0 * Rand(R), 3.0, (0.03 + 0.1 * Size * Size * Size) * V, 0.003);
		}
		else
		{
			CueCrack(B, R, At, 1800.0 + 4200.0 * Rand(R), 1.5, (0.02 + 0.16 * Size * Size * Size) * V, 0.0005 + 0.001 * Rand(R));
		}
		const double Gap = Rand(R);
		At += 0.008 + 0.05 * Gap * Gap;
	}
	// The ends: equal-power fades, so two segments overlapping by CueFuseFade hold the level.
	B.Data.resize(N);
	const double FadeN = CueFuseFade * SynthRate;
	for (size_t I = 0; I < N; ++I)
	{
		const double D = static_cast<double>(I), Left = static_cast<double>(N - 1 - I);
		if (D < FadeN)
		{
			B.Data[I] *= static_cast<float>(std::sin(0.5 * SynthPi * D / FadeN));
		}
		else if (Left < FadeN)
		{
			B.Data[I] *= static_cast<float>(std::sin(0.5 * SynthPi * Left / FadeN));
		}
	}
}

/** IgBoom: the multiplier lands. The shell's crack, the boom (a sine falling from 150 to 55 Hz with its octave, and a blast
 *  of noise whose lowpass closes), the arcade's sparkle (an E major triad high up, E7, in triangles 30 ms apart) and trailing
 *  crackles; about 0.8 s. From CueBoomLongVol, the long tail: a deeper boom, its echo off the boardwalk at 0.3 s, a rumble,
 *  and a willow of crackles swelling to 0.8 s and thinning out by 2.3 s. */
void CueIgBoom(Buffer& B, Noise& R, double V)
{
	const bool Big = V >= CueBoomLongVol;
	CueHiss(B, R, 0.0, FilterType::HighPass, 1500.0, 0.0, 0.0, 0.0, 0.16 * V, 0.0002, 0.005, 0.025);
	CueOsc(B, 0.0, Big ? 130.0 : 150.0, Big ? 45.0 : 55.0, Big ? 0.25 : 0.15, (Big ? 0.26 : 0.34) * V, 0.002, Big ? 0.22 : 0.12, Big ? 1.1 : 0.7);
	CueOsc(B, 0.0, Big ? 260.0 : 300.0, Big ? 90.0 : 110.0, Big ? 0.25 : 0.15, (Big ? 0.08 : 0.1) * V, 0.002, Big ? 0.12 : 0.07, Big ? 0.6 : 0.4);
	CueHiss(B, R, 0.0, FilterType::LowPass, Big ? 5000.0 : 4000.0, Big ? 180.0 : 250.0, Big ? 0.8 : 0.25, 1.0, (Big ? 0.14 : 0.16) * V, 0.001, Big ? 0.22 : 0.09, Big ? 1.0 : 0.6);
	const double Triad[3] = {2637.0, 3322.4, 3951.1};
	for (int K = 0; K < 3; ++K)
	{
		CueOsc(B, 0.04 + 0.03 * K, Triad[K], 0.0, 0.0, 0.03 * V, 0.002, Big ? 0.3 : 0.15, Big ? 0.8 : 0.5, Wave::Triangle);
	}
	if (!Big)
	{
		double At = 0.12;
		while (At < 0.8)
		{
			const double Thin = 1.0 - (At - 0.12) / 0.7;
			CueCrack(B, R, At, 2500.0 + 4000.0 * Rand(R), 1.5, 0.1 * V * Thin * (0.4 + 0.6 * Rand(R)), 0.0008);
			At += 0.03 + 0.07 * Rand(R);
		}
		return;
	}
	CueOsc(B, 0.3, 80.0, 40.0, 0.2, 0.18 * V, 0.01, 0.2, 0.7);
	CueHiss(B, R, 0.3, FilterType::LowPass, 900.0, 300.0, 0.5, 1.0, 0.05 * V, 0.01, 0.2, 0.7);
	CueHiss(B, R, 0.0, FilterType::LowPass, 220.0, 0.0, 0.0, 1.0, 0.06 * V, 0.04, 0.3, 1.0);
	double At = 0.3;
	while (At < 2.3)
	{
		// The willow: densest and loudest about 0.8 s in, thinning to the end.
		const double Swell = At < 0.8 ? (At - 0.3) / 0.5 : 1.0 - (At - 0.8) / 1.5;
		CueCrack(B, R, At, 2500.0 + 4500.0 * Rand(R), 1.5, 0.14 * V * (0.15 + 0.85 * Swell) * (0.3 + 0.7 * Rand(R)), 0.0007);
		At += 0.008 + 0.04 * (1.0 - Swell) * Rand(R) + 0.008 * Rand(R);
	}
}

/** RitBlip: a terminal's tick (selection, a new feed line; the site's one navigation sound): a sine pip settling from 1,480
 *  to 1,318 Hz (E6) in 6 ms, a faint chrome partial at 2.5x and an edge of air on its front. 40 ms, dry. */
void CueRitBlip(Buffer& B, Noise& R, double V)
{
	CueOsc(B, 0.0, 1480.0, 1318.5, 0.006, 0.06 * V, 0.0008, 0.009, 0.04);
	CueOsc(B, 0.0, 3296.0, 0.0, 0.0, 0.014 * V, 0.0005, 0.004, 0.02);
	CueHiss(B, R, 0.0, FilterType::HighPass, 8000.0, 0.0, 0.0, 0.0, 0.012 * V, 0.0001, 0.0005, 0.003);
}

/** RitFlap: a split-flap settling: six flaps falling, faster then slower (16 to 36 ms apart, each gap jittered), each a
 *  papery clack in a band jittered round 2.2 kHz with a small knock; the last, the one that stays, heavier. About 0.15 s. */
void CueRitFlap(Buffer& B, Noise& R, double V)
{
	const int Count = 6;
	double At = 0.0;
	for (int K = 0; K < Count; ++K)
	{
		const bool Stays = K == Count - 1;
		const double Hit = (Stays ? 1.25 : 1.0 - 0.07 * K) * (0.8 + 0.2 * Rand(R));
		CueHiss(B, R, At, FilterType::BandPass, 2200.0 * (0.85 + 0.3 * Rand(R)), 0.0, 0.0, 1.6, 0.06 * V * Hit, 0.0002, 0.003, 0.014);
		CueOsc(B, At, Stays ? 380.0 : 520.0, 0.0, 0.0, (Stays ? 0.05 : 0.025) * V, 0.0005, Stays ? 0.007 : 0.004, 0.03);
		At += (0.016 + 0.004 * K) * (0.9 + 0.2 * Rand(R));
	}
}

/** RitBell: the desk bell: a struck dome at 2,350 Hz beating against its twin (7 Hz), its inharmonic partials (1.48x,
 *  2.61x) dying first, and the striker's tick. About 1.6 s. */
void CueRitBell(Buffer& B, Noise& R, double V)
{
	CueHiss(B, R, 0.0, FilterType::HighPass, 4000.0, 0.0, 0.0, 0.0, 0.045 * V, 0.0001, 0.001, 0.006);
	CueOsc(B, 0.0, 2350.0, 0.0, 0.0, 0.09 * V, 0.001, 0.42, 1.6);
	CueOsc(B, 0.0, 2357.0, 0.0, 0.0, 0.05 * V, 0.001, 0.4, 1.6);
	CueOsc(B, 0.0, 3478.0, 0.0, 0.0, 0.022 * V, 0.001, 0.25, 1.2);
	CueOsc(B, 0.0, 6134.0, 0.0, 0.0, 0.035 * V, 0.001, 0.16, 0.9);
}

/** RitStamp: a scalp, a stamp, a credential, a printout: the stamp coming down (a thud falling 140 to 70 Hz and the handle's
 *  knock), rubber slapping paper, the ink pressed in, and a small tick as it lifts at 0.17 s. About 0.25 s. */
void CueRitStamp(Buffer& B, Noise& R, double V)
{
	CueOsc(B, 0.0, 140.0, 70.0, 0.04, 0.16 * V, 0.001, 0.045, 0.22);
	CueOsc(B, 0.0, 260.0, 0.0, 0.0, 0.07 * V, 0.0005, 0.014, 0.06);
	CueHiss(B, R, 0.0, FilterType::BandPass, 1100.0, 0.0, 0.0, 0.8, 0.06 * V, 0.0003, 0.009, 0.04);
	CueHiss(B, R, 0.004, FilterType::LowPass, 600.0, 0.0, 0.0, 1.0, 0.015 * V, 0.01, 0.03, 0.08);
	CueHiss(B, R, 0.17, FilterType::BandPass, 2800.0, 0.0, 0.0, 3.0, 0.012 * V, 0.0002, 0.002, 0.008);
}

/** RitRank: under CueRankFallVol, one place gained: an odometer's ratchet (two pawl clicks 7 ms apart) and a small chrome
 *  ping (E7). From it, two chrome notes (FM with a metallic 1.4x modulator): falling a fourth for a relegation (D5 to A4,
 *  darker); from CueRankRiseVol, rising a fifth for a promotion (B-flat 4 to F5, brighter). */
void CueRitRank(Buffer& B, Noise& R, double V)
{
	if (V < CueRankFallVol)
	{
		CueHiss(B, R, 0.0, FilterType::BandPass, 3400.0, 0.0, 0.0, 4.0, 0.05 * V, 0.0001, 0.0012, 0.006);
		CueHiss(B, R, 0.007, FilterType::BandPass, 3000.0, 0.0, 0.0, 4.0, 0.035 * V, 0.0001, 0.0012, 0.006);
		CueOsc(B, 0.002, 2637.0, 0.0, 0.0, 0.04 * V, 0.001, 0.02, 0.07);
		return;
	}
	const bool Rise = V >= CueRankRiseVol;
	CueFm(B, 0.0, Rise ? 466.16 : 587.33, 1.4, Rise ? 2.2 : 1.4, 0.1, 0.09 * V, 0.002, 0.2, 0.5);
	CueFm(B, 0.14, Rise ? 698.46 : 440.0, 1.4, Rise ? 2.2 : 1.4, 0.1, 0.1 * V, 0.002, Rise ? 0.28 : 0.24, 0.6);
}

/** Finish's bus and limiter, tanh(1.728 x) / 1.2, with the quiet samples (most of a cue's tail) on a series that agrees with
 *  tanh to 1e-6 under 0.3, so a 2.3 s cue doesn't spend its render on tanh. */
float CueLimit(float S)
{
	const double Y = static_cast<double>(S) * 0.72 * 2.0 * 1.2;
	if (std::fabs(Y) < 0.3)
	{
		const double Y2 = Y * Y;
		return static_cast<float>(Y * (1.0 - Y2 * (1.0 / 3.0 - Y2 * (2.0 / 15.0 - Y2 * (17.0 / 315.0)))) / 1.2);
	}
	return static_cast<float>(std::tanh(Y) / 1.2);
}

/** The ten cues: the volume clamped to 0..1 (a non-number is silence), then the bus and the limiter as every one-shot, held
 *  under CueCeiling, with the trailing silence trimmed. */
std::vector<float> CueRender(SoundId Id, double Volume, Noise& R)
{
	const double V = std::isfinite(Volume) ? (Volume < 0.0 ? 0.0 : Volume > 1.0 ? 1.0 : Volume) : 0.0;
	Buffer B;
	switch (Id)
	{
	case SoundId::IgSpark: CueIgSpark(B, R, V); break;
	case SoundId::IgPop: CueIgPop(B, R, V); break;
	case SoundId::IgTick: CueIgTick(B, R, V); break;
	case SoundId::IgFuse: CueIgFuse(B, R, V); break;
	case SoundId::IgBoom: CueIgBoom(B, R, V); break;
	case SoundId::RitBlip: CueRitBlip(B, R, V); break;
	case SoundId::RitFlap: CueRitFlap(B, R, V); break;
	case SoundId::RitBell: CueRitBell(B, R, V); break;
	case SoundId::RitStamp: CueRitStamp(B, R, V); break;
	case SoundId::RitRank: CueRitRank(B, R, V); break;
	default: break;
	}
	B.Ensure(0.002);
	std::vector<float> Out = std::move(B.Data);
	float Top = 0.0f;
	for (float& S : Out)
	{
		S = CueLimit(S);
		Top = std::max(Top, std::fabs(S));
	}
	if (Top > CueCeiling)
	{
		const float Under = CueCeiling / Top;
		for (float& S : Out)
		{
			S *= Under;
		}
	}
	size_t Keep = Out.size();
	while (Keep > 1 && std::fabs(Out[Keep - 1]) < 1.0e-5f)
	{
		--Keep;
	}
	Out.resize(Keep);
	return Out;
}
} // namespace synth_detail

std::vector<float> Render(SoundId Id, float Volume, uint32_t Seed)
{
	using namespace synth_detail;
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
	// Update 3 (§1.11): the revamped sites' ten cues, each its own (CueRender above; IgBoom and RitRank pick their variant
	// by the volume asked for).
	case SoundId::IgSpark:
	case SoundId::IgPop:
	case SoundId::IgTick:
	case SoundId::IgFuse:
	case SoundId::IgBoom:
	case SoundId::RitBlip:
	case SoundId::RitFlap:
	case SoundId::RitBell:
	case SoundId::RitStamp:
	case SoundId::RitRank:
		return CueRender(Id, V, R);
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
	case SoundId::Mail:
		// A letter in the Mail app: paper sliding through a slot, landing, and a soft two-note bell (E6, B5), warmer
		// than the table's alerts and quieter than a text.
		Burst(B, R, 0.0, 1800.0, 0.9, 0.16, 0.06 * V, FilterType::BandPass, 3600.0);
		Burst(B, R, 0.15, 900.0, 0.8, 0.05, 0.08 * V, FilterType::LowPass);
		Tone(B, 1318.5, 0.5, 0.055 * V, 0.22);
		Tone(B, 2637.0, 0.18, 0.008 * V, 0.22);
		Tone(B, 987.77, 0.7, 0.05 * V, 0.36);
		Tone(B, 1975.5, 0.22, 0.007 * V, 0.36);
		break;
	}
	return Finish(B);
}

std::vector<float> Render(Effect Id, uint32_t Seed)
{
	using namespace synth_detail;
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
	case Effect::Chime:
		// Ding-dong: two soft bell tones, a sixth apart, with a little shimmer.
		Tone(B, 784.0, 1.1, 0.16, 0.0);
		Tone(B, 1568.0, 0.5, 0.03, 0.0);
		Tone(B, 622.25, 1.4, 0.16, 0.42);
		Tone(B, 1244.5, 0.6, 0.03, 0.42);
		break;
	case Effect::Applause:
	{
		// Many pairs of hands, each clapping at its own pace, swelling and dying away over three seconds.
		const double Dur = 3.2;
		B.Ensure(Dur + 0.1);
		for (int Hand = 0; Hand < 28; ++Hand)
		{
			const double Rate = 3.2 + 2.6 * Rand(R);
			double At = 0.05 + 0.35 * Rand(R);
			while (At < Dur)
			{
				const double Swell = std::sin(SynthPi * std::min(1.0, At / Dur)) * (1.0 - 0.4 * At / Dur);
				Burst(B, R, At, 900.0 + 1600.0 * Rand(R), 1.2, 0.025, 0.05 * Swell);
				At += 1.0 / Rate * (0.85 + 0.3 * Rand(R));
			}
		}
		break;
	}
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
	case Effect::CardPrint:
	{
		// The Players Club card coming out of the desk's printer: the relay, the motor and its stepper ticking the card
		// past the head, the card sliding out, and the clack as it drops into the tray. About 1.8 s.
		Burst(B, R, 0.0, 2600.0, 5.0, 0.008, 0.05);
		Tone(B, 95.0, 0.25, 0.03, 0.02, Wave::Triangle, 150.0);
		Tone(B, 150.0, 1.0, 0.028, 0.22, Wave::Triangle);
		Tone(B, 300.0, 1.0, 0.007, 0.22);
		for (double At = 0.25; At < 1.2; At += 0.0055 * (0.9 + 0.2 * Rand(R)))
		{
			const double Pass = std::sin(SynthPi * (At - 0.25) / 0.95);
			Burst(B, R, At, 1900.0 + 400.0 * Rand(R), 4.0, 0.004, 0.012 + 0.012 * Pass);
		}
		Burst(B, R, 1.22, 2400.0, 1.5, 0.3, 0.06, FilterType::BandPass, 4600.0);
		Burst(B, R, 1.58, 600.0, 0.9, 0.035, 0.09, FilterType::LowPass);
		Burst(B, R, 1.58, 3600.0, 6.0, 0.012, 0.07);
		Burst(B, R, 1.64, 3000.0, 6.0, 0.008, 0.025);
		break;
	}
	}
	return Finish(B);
}

std::vector<float> RenderThunder(float Strength, uint32_t Seed)
{
	using namespace synth_detail;
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
	using namespace synth_detail;
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
	using namespace synth_detail;
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
