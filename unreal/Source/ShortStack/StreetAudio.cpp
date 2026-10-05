#include "StreetAudio.h"

#include <cmath>

namespace StreetAudioDetail
{
constexpr double Rate = 48000.0;
constexpr double Dt = 1.0 / Rate;
constexpr double TwoPi = 2.0 * UE_DOUBLE_PI;

/** A one-pole coefficient with its corner at Hz. */
double Corner(double Hz)
{
	return 1.0 - FMath::Exp(-TwoPi * Hz * Dt);
}

/** A one-pole coefficient that settles with a time constant of Seconds. */
double Settle(double Seconds)
{
	return 1.0 - FMath::Exp(-Dt / Seconds);
}

/** Eases Value toward Target, landing on it exactly (no endless tail of ever smaller steps). */
void Ease(double& Value, double Target, double K)
{
	Value += (Target - Value) * K;
	if (FMath::Abs(Target - Value) < 1e-7)
	{
		Value = Target;
	}
}

/** A phase kept within one turn (it's added to every sample, for hours). */
void Turn(double& Phase, double Hz)
{
	Phase += TwoPi * Hz * Dt;
	if (Phase > TwoPi)
	{
		Phase -= TwoPi;
	}
}

/** The first Count harmonics of a sine at Phase, weighted (sin nP by the Chebyshev recursion: one sine, one cosine). */
double Harmonics(double Phase, const double* Weights, int32 Count)
{
	const double C2 = 2.0 * FMath::Cos(Phase);
	double Prev = 0.0;
	double Cur = FMath::Sin(Phase);
	double Sum = 0.0;
	for (int32 H = 0; H < Count; ++H)
	{
		Sum += Weights[H] * Cur;
		const double Next = C2 * Cur - Prev;
		Prev = Cur;
		Cur = Next;
	}
	return Sum;
}
} // namespace StreetAudioDetail

// ------------------------------------------------------------------ building blocks

void FStreetAmbience::FBiquad::Set(double B0In, double B1In, double B2In, double A0, double A1In, double A2In)
{
	B0 = B0In / A0;
	B1 = B1In / A0;
	B2 = B2In / A0;
	A1 = A1In / A0;
	A2 = A2In / A0;
}

void FStreetAmbience::FBiquad::LowPass(double Hz, double Q)
{
	const double W = StreetAudioDetail::TwoPi * Hz * StreetAudioDetail::Dt;
	const double Cw = FMath::Cos(W);
	const double Alpha = FMath::Sin(W) / (2.0 * Q);
	Set((1.0 - Cw) * 0.5, 1.0 - Cw, (1.0 - Cw) * 0.5, 1.0 + Alpha, -2.0 * Cw, 1.0 - Alpha);
}

void FStreetAmbience::FBiquad::HighPass(double Hz, double Q)
{
	const double W = StreetAudioDetail::TwoPi * Hz * StreetAudioDetail::Dt;
	const double Cw = FMath::Cos(W);
	const double Alpha = FMath::Sin(W) / (2.0 * Q);
	Set((1.0 + Cw) * 0.5, -(1.0 + Cw), (1.0 + Cw) * 0.5, 1.0 + Alpha, -2.0 * Cw, 1.0 - Alpha);
}

void FStreetAmbience::FBiquad::BandPass(double Hz, double Q)
{
	// Unity gain at the center.
	const double W = StreetAudioDetail::TwoPi * Hz * StreetAudioDetail::Dt;
	const double Cw = FMath::Cos(W);
	const double Alpha = FMath::Sin(W) / (2.0 * Q);
	Set(Alpha, 0.0, -Alpha, 1.0 + Alpha, -2.0 * Cw, 1.0 - Alpha);
}

double FStreetAmbience::FBiquad::Run(double X)
{
	const double Y = B0 * X + B1 * X1 + B2 * X2 - A1 * Y1 - A2 * Y2;
	X2 = X1;
	X1 = X;
	Y2 = Y1;
	Y1 = Y;
	return Y;
}

void FStreetAmbience::FRing::Strike(double Hz, double Seconds, double Amp)
{
	const double W = StreetAudioDetail::TwoPi * Hz * StreetAudioDetail::Dt;
	const double R = FMath::Exp(-StreetAudioDetail::Dt / Seconds);
	C = 2.0 * R * FMath::Cos(W);
	R2 = R * R;
	// y[n] = Amp r^n sin(w n): y[0] = 0, y[1] = Amp r sin(w), and the recursion carries it on.
	Y2 = 0.0;
	Y1 = Amp * R * FMath::Sin(W);
}

double FStreetAmbience::FRing::Run()
{
	const double Y = C * Y1 - R2 * Y2;
	Y2 = Y1;
	Y1 = Y;
	// Rung out: stopped before the tail sinks toward denormals.
	if (FMath::Abs(Y1) < 1e-9 && FMath::Abs(Y2) < 1e-9)
	{
		Y1 = 0.0;
		Y2 = 0.0;
	}
	return Y;
}

// ------------------------------------------------------------------ the street

FStreetAmbience::FStreetAmbience(uint32 Seed)
	: State(Seed | 1u)
{
	using namespace StreetAudioDetail;
	HissHp.HighPass(450.0, 0.7);
	HissLp.LowPass(7000.0, 0.6);
	Gutter.BandPass(GutterHz, 5.0);
	Spray.BandPass(1150.0, 0.8);
	Sizzle.BandPass(6500.0, 1.5);
	CrackDecay = FMath::Exp(-Dt / 0.0015);
	// The echo off the fronts across the street and down the block: 0.42 s of the far sounds.
	Echo.SetNumZeroed(static_cast<int32>(Rate * 0.42));
	// The street was going before the door shut behind the player: the compressor partway through a run, the first car
	// and the first far sound some way off.
	NextCompressor = 20.0 + 90.0 * Uniform();
	NextCar = 4.0 + 10.0 * Uniform();
	NextFar = 45.0 + 60.0 * Uniform();
	NextGust = 2.0 + 3.0 * Uniform();
}

void FStreetAmbience::SetInside(float InInside, float InDoorsOpen)
{
	InsideTarget = FMath::Clamp(static_cast<double>(InInside), 0.0, 1.0);
	DoorsTarget = FMath::Clamp(static_cast<double>(InDoorsOpen), 0.0, 1.0);
}

void FStreetAmbience::SetRain(float InRain, float InWetness)
{
	RainTarget = FMath::Clamp(static_cast<double>(InRain), 0.0, 2.0);
	Wet = FMath::Clamp(static_cast<double>(InWetness), 0.0, 1.0);
}

void FStreetAmbience::SetDaylight(float InDaylight)
{
	Daylight = FMath::Clamp(static_cast<double>(InDaylight), 0.0, 1.0);
}

void FStreetAmbience::SetHum(float InHum)
{
	HumTarget = FMath::Clamp(static_cast<double>(InHum), 0.0, 1.0);
}

void FStreetAmbience::SetPaused(bool bInPaused)
{
	bPaused = bInPaused;
}

void FStreetAmbience::FadeOut(float Seconds)
{
	FadeTarget = 0.0;
	FadeStep = 1.0 / (FMath::Max(0.05, static_cast<double>(Seconds)) * StreetAudioDetail::Rate);
}

double FStreetAmbience::Noise()
{
	// xorshift32, -1..1
	State ^= State << 13;
	State ^= State >> 17;
	State ^= State << 5;
	return static_cast<double>(State) / 2147483648.0 - 1.0;
}

double FStreetAmbience::Scatter(double PerSecond)
{
	// Exponential gaps: a Poisson scattering, never quite regular.
	return -FMath::Loge(FMath::Max(Uniform(), 1e-6)) / FMath::Max(PerSecond, 0.05);
}

void FStreetAmbience::Render(float* Out, int32 N)
{
	using namespace StreetAudioDetail;
	// The listener's step through the door eases over a few hundredths of a second (no zipper), the rain over a second
	// and a half (it picks up, it doesn't switch on), the sign's buzz over a tenth; the compressor spins up and runs down.
	static const double ListenK = Settle(0.05);
	static const double RainK = Settle(1.5);
	static const double HumK = Settle(0.12);
	static const double CompOnK = Settle(0.4);
	static const double CompOffK = Settle(0.25);
	for (int32 I = 0; I < N; ++I)
	{
		Time += Dt;
		if (!bPaused)
		{
			Clock += Dt;
		}
		Ease(Inside, InsideTarget, ListenK);
		Ease(Doors, DoorsTarget, ListenK);
		Ease(Rain, RainTarget, RainK);
		Ease(Hum, HumTarget, HumK);
		Fade = Fade < FadeTarget ? FMath::Min(FadeTarget, Fade + FadeStep) : FMath::Max(FadeTarget, Fade - FadeStep);

		// The street: its beds, what passes (and its echo down the block), the sign overhead.
		const double Away = RenderCar() + RenderFar();
		const double Street = RenderStreet() + Away + EchoOf(Away) + RenderNeon();
		// Heard through the store's glass it's quieter and the top is gone, from 16 kHz on the sidewalk to 650 Hz with the
		// doors shut; through the open doors much less so.
		const double Muffle = Inside * (1.0 - 0.6 * Doors);
		if ((I & 31) == 0)
		{
			MuffK = Corner(16000.0 * FMath::Pow(650.0 / 16000.0, Muffle));
		}
		Muff1 += (Street - Muff1) * MuffK;
		Muff2 += (Muff1 - Muff2) * MuffK;
		double S = FMath::Lerp(Street, Muff2, FMath::Min(1.0, Muffle * 1.5)) * (1.0 - 0.7 * Muffle);

		// The coolers' compressor keeps its own hours, heard or not: on for a minute or two, off for half a minute or more.
		// It knocks as it starts and clunks as it stops (only worth striking with someone inside to hear it).
		if (Clock >= NextCompressor)
		{
			bCompressor = !bCompressor;
			CompSwitchAt = Time;
			NextCompressor = Clock + (bCompressor ? 70.0 + 90.0 * Uniform() : 30.0 + 50.0 * Uniform());
			if (Inside > 0.001)
			{
				Thunk.Strike(bCompressor ? 62.0 : 48.0, bCompressor ? 0.07 : 0.05, bCompressor ? 0.09 : 0.05);
			}
		}
		Ease(CompLevel, bCompressor ? 1.0 : 0.0, bCompressor ? CompOnK : CompOffK);
		if (Inside > 0.001)
		{
			S += RenderStore() * Inside;
		}

		S *= Fade;
		Out[I] = static_cast<float>(std::tanh(S * 1.4) / 1.4);
	}
}

double FStreetAmbience::RenderStreet()
{
	using namespace StreetAudioDetail;
	static const double GustK = Settle(1.2);
	static const double CrackK = Corner(2500.0);
	static const double BodyK = Corner(900.0);
	static const double GutterAmK = Corner(12.0);
	// Gusts: the rain thickens and thins every few seconds.
	if (Time >= NextGust)
	{
		GustTarget = 0.72 + 0.5 * Uniform();
		NextGust = Time + 2.5 + 5.0 * Uniform();
	}
	Ease(Gust, GustTarget, GustK);
	// The rain falling (heard by its square root: half the rain isn't half as loud), and what keeps dripping off everything
	// while the street is wet, rain or not.
	const double Fall = FMath::Sqrt(Rain) * Gust;
	const double Drip = FMath::Max(FMath::Min(Rain, 1.0), 0.35 * Wet);

	// The hiss on the pavement, the crackle of the drops landing on it, and the body of the rain across the block.
	double S = HissLp.Run(HissHp.Run(Noise())) * 0.15 * Fall;
	if (Time >= NextCrack)
	{
		CrackEnv = 0.3 + 0.7 * Uniform();
		NextCrack = Time + Scatter(260.0 * Rain);
	}
	const double Grain = Noise();
	CrackLow += (Grain - CrackLow) * CrackK;
	S += (Grain - CrackLow) * CrackEnv * 0.05 * Fall;
	CrackEnv = CrackEnv < 1e-6 ? 0.0 : CrackEnv * CrackDecay;
	Brown = Brown * 0.97 + Noise() * 0.17;
	Body += (Brown - Body) * BodyK;
	S += Body * 0.12 * Fall;

	// Drops on the awnings: dull pats on the canvas, ticks on sheet metal and the air conditioners over the street.
	const double Patter = FMath::Max(Fall, Drip);
	if (Time >= NextCanvas)
	{
		Canvas.Strike(180.0 + 260.0 * Uniform(), 0.006 + 0.008 * Uniform(), (0.012 + 0.03 * Uniform()) * Patter);
		NextCanvas = Time + Scatter(14.0 * Rain + 0.6 * Drip);
	}
	if (Time >= NextMetal)
	{
		Metal.Strike(1700.0 + 2200.0 * Uniform(), 0.012 + 0.03 * Uniform(), (0.004 + 0.012 * Uniform()) * Patter);
		NextMetal = Time + Scatter(4.0 * Rain + 0.8 * Drip);
	}
	S += Canvas.Run() + Metal.Run();

	// Drips off the fire escapes and the awnings' edges into the puddles: a bright note gliding up as the drop's bubble
	// closes, gone in a few hundredths of a second.
	if (Time >= NextPlink)
	{
		PlinkAt = Time;
		PlinkHz = 1100.0 + 1600.0 * Uniform();
		PlinkGain = (0.006 + 0.016 * Uniform()) * Drip;
		PlinkPhase = 0.0;
		NextPlink = Time + Scatter(0.8 + 1.6 * Drip);
	}
	const double SincePlink = Time - PlinkAt;
	if (SincePlink < 0.08)
	{
		Turn(PlinkPhase, PlinkHz * (1.0 + 0.7 * (1.0 - FMath::Exp(-SincePlink / 0.01))));
		S += FMath::Sin(PlinkPhase) * FMath::Exp(-SincePlink / 0.018) * PlinkGain;
	}

	// The gutters and the downspouts running: water through a narrow throat, its pitch hopping as the bubbles form, the
	// flow surging and easing.
	if (Time >= NextBubble)
	{
		GutterTarget = 500.0 + 1100.0 * Uniform();
		NextBubble = Time + 0.03 + 0.12 * Uniform();
	}
	if ((GutterTick++ & 63) == 0)
	{
		GutterHz += (GutterTarget - GutterHz) * 0.5;
		Gutter.BandPass(GutterHz, 5.0);
	}
	GutterAm += (Uniform() - GutterAm) * GutterAmK;
	S += Gutter.Run(Noise()) * GutterAm * 0.22 * Drip;
	return S;
}

double FStreetAmbience::RenderCar()
{
	using namespace StreetAudioDetail;
	static const double RoarK = Corner(220.0);
	if (!bCar && Clock >= NextCar)
	{
		bCar = true;
		CarAt = Time;
		CarLength = 4.5 + 3.5 * Uniform();
		CarGain = 0.5 + 0.5 * Uniform();
		CarEngine = 38.0 + 24.0 * Uniform();
		// Busier by day: a car every 4 to 14 seconds; at night every 10 to 36.
		NextCar = Clock + CarLength + FMath::Lerp(10.0, 4.0, Daylight) + FMath::Lerp(26.0, 10.0, Daylight) * Uniform();
	}
	if (!bCar)
	{
		return 0.0;
	}
	const double T = Time - CarAt;
	if (T >= CarLength)
	{
		bCar = false;
		return 0.0;
	}
	// Nearest halfway through: it swells and falls away as 1 / (1 + u^2), tapered to nothing at either end. Coming, the
	// spray off the tyres and the engine are higher; going, lower (the Doppler shift of a car at 40 km/h or so).
	const double U = (T - CarLength * 0.5) / (CarLength / 6.0);
	const double Env = CarGain * FMath::Sin(UE_DOUBLE_PI * T / CarLength) / (1.0 + U * U);
	const double Shift = 1.0 - 0.035 * std::tanh(U);
	if ((CarTick++ & 63) == 0)
	{
		Spray.BandPass(1150.0 * Shift * Shift, 0.8);
	}
	const double Hiss = Spray.Run(Noise()) * (0.1 + 0.26 * Wet);
	CarBrown = CarBrown * 0.985 + Noise() * 0.12;
	CarRoar += (CarBrown - CarRoar) * RoarK;
	Turn(CarPhase, CarEngine * Shift);
	const double Engine = FMath::Sin(CarPhase) + 0.5 * FMath::Sin(2.0 * CarPhase + 0.4);
	return Env * (Hiss + CarRoar * 0.1 + Engine * 0.02);
}

double FStreetAmbience::RenderFar()
{
	using namespace StreetAudioDetail;
	static const double SirenK = Corner(1500.0);
	if (Far == EFar::None && Clock >= NextFar)
	{
		Far = Uniform() < 0.4 ? EFar::Siren : EFar::Bus;
		FarAt = Time;
		FarLength = Far == EFar::Siren ? 18.0 + 8.0 * Uniform() : 14.0;
		FarGain = 0.6 + 0.4 * Uniform();
		FarPhase = 0.0;
		// Once in a long while: every two to four minutes at night, a little more often by day.
		NextFar = Clock + FarLength + FMath::Lerp(80.0, 50.0, Daylight) + FMath::Lerp(140.0, 90.0, Daylight) * Uniform();
	}
	if (Far == EFar::None)
	{
		return 0.0;
	}
	const double T = Time - FarAt;
	if (T >= FarLength)
	{
		Far = EFar::None;
		return 0.0;
	}
	if (Far == EFar::Bus)
	{
		return RenderBus(T) * FarGain;
	}
	// A siren blocks away, wailing up and down every four seconds: it comes up out of the city, holds, and moves off a
	// little lower than it came; the buildings between take the top off it.
	const double Along = T / FarLength;
	const double Env = FMath::SmoothStep(0.0, 0.35, Along) * (1.0 - FMath::SmoothStep(0.55, 1.0, Along));
	const double Wail = 0.5 - 0.5 * FMath::Cos(TwoPi * T / 3.9);
	Turn(FarPhase, (680.0 + 640.0 * Wail) * (1.0 + 0.025 * FMath::Cos(UE_DOUBLE_PI * Along)));
	static const double Voice[3] = {1.0, 0.35, 0.2};
	FarLow += (Harmonics(FarPhase, Voice, 3) - FarLow) * SirenK;
	return FarLow * Env * FarGain * 0.04;
}

double FStreetAmbience::RenderBus(double T)
{
	using namespace StreetAudioDetail;
	static const double KnockK = Corner(1200.0);
	static const double BrakeK = Corner(2000.0);
	static const double AirK = Corner(3500.0);
	// A city bus pulling in to the stop on Market and away again: the diesel slowing as it comes, the air brakes' sigh,
	// the doors' two, the engine idling, then revving off through a gear change as it goes.
	double Firing = 32.0;
	if (T < 3.5)
	{
		Firing = FMath::Lerp(48.0, 32.0, T / 3.5);
	}
	else if (T >= 8.0 && T < 10.6)
	{
		Firing = FMath::Lerp(32.0, 60.0, (T - 8.0) / 2.6);
	}
	else if (T >= 10.6 && T < 11.0)
	{
		Firing = FMath::Lerp(60.0, 44.0, (T - 10.6) / 0.4);
	}
	else if (T >= 11.0)
	{
		Firing = FMath::Lerp(44.0, 58.0, FMath::Min(1.0, (T - 11.0) / 3.0));
	}
	const double Load = T < 3.5 ? 0.6 : T < 8.0 ? 0.45 : 1.0;
	const double Env = FMath::SmoothStep(0.0, 3.0, T) * (1.0 - FMath::SmoothStep(10.0, 14.0, T));
	Turn(FarPhase, Firing);
	static const double Diesel[6] = {1.0, 0.574, 0.415, 0.33, 0.276, 0.238};
	// The diesel's knock: a burst of grit on every firing.
	const double Beat = 0.5 + 0.5 * FMath::Sin(FarPhase);
	const double Beat2 = Beat * Beat;
	const double Beat4 = Beat2 * Beat2;
	KnockLow += (Noise() * Beat4 * Beat4 - KnockLow) * KnockK;
	double S = (Harmonics(FarPhase, Diesel, 6) * 0.045 + KnockLow * 0.08) * Load;
	// The air brakes at the stop, then the doors opening and closing.
	struct FSigh
	{
		double At, Seconds, Gain;
	};
	static const FSigh Sighs[3] = {{3.5, 0.35, 1.0}, {4.3, 0.2, 0.5}, {7.4, 0.2, 0.5}};
	double Air = 0.0;
	for (const FSigh& Sigh : Sighs)
	{
		if (T >= Sigh.At && T < Sigh.At + Sigh.Seconds * 6.0)
		{
			Air += Sigh.Gain * FMath::Exp(-(T - Sigh.At) / Sigh.Seconds);
		}
	}
	const double Rush = Noise();
	BrakeLow += (Rush - BrakeLow) * BrakeK;
	S += (Rush - BrakeLow) * Air * 0.05;
	// From a block away the air takes the top off it.
	FarLow += (S - FarLow) * AirK;
	return FarLow * Env;
}

double FStreetAmbience::EchoOf(double In)
{
	using namespace StreetAudioDetail;
	static const double EchoK = Corner(3000.0);
	const int32 Len = Echo.Num();
	auto Reflection = [this, Len](double Seconds) {
		int32 J = EchoAt - static_cast<int32>(Seconds * Rate);
		if (J < 0)
		{
			J += Len;
		}
		return static_cast<double>(Echo[J]);
	};
	const double Last = Reflection(0.397);
	const double Back = Reflection(0.067) * 0.3 + Reflection(0.151) * 0.22 + Reflection(0.263) * 0.15 + Last * 0.1;
	// The last reflection fed back in (the street rings a little), the air taking a little more top off each time.
	EchoLow += (In + Last * 0.3 - EchoLow) * EchoK;
	if (FMath::Abs(EchoLow) < 1e-12)
	{
		EchoLow = 0.0;
	}
	Echo[EchoAt] = static_cast<float>(EchoLow);
	EchoAt = EchoAt + 1 < Len ? EchoAt + 1 : 0;
	return Back;
}

double FStreetAmbience::RenderNeon()
{
	using namespace StreetAudioDetail;
	static const double FlutterK = Corner(30.0);
	if (Hum < 1e-4)
	{
		return 0.0;
	}
	// The transformer: 120 Hz and its overtones, roughened as its core buzzes, and the tubes' sizzle chopped at the same
	// rate; an old tube flutters now and then.
	Turn(HumPhase, 120.0);
	static const double Core[5] = {1.0, 0.5, 0.3, 0.18, 0.1};
	const double Buzz = std::tanh(1.8 * Harmonics(HumPhase, Core, 5)) / 1.8;
	const double Pulse = 0.5 + 0.5 * FMath::Cos(HumPhase);
	const double Pulse2 = Pulse * Pulse;
	const double Fizz = Sizzle.Run(Noise()) * Pulse2 * Pulse2 * Pulse2;
	if (Time >= NextFlutter)
	{
		const bool bDip = Uniform() < 0.15;
		FlutterTarget = bDip ? 0.35 : 0.85 + 0.15 * Uniform();
		NextFlutter = Time + (bDip ? 0.08 : 0.5 + 2.5 * Uniform());
	}
	Flutter += (FlutterTarget - Flutter) * FlutterK;
	return (Buzz * 0.03 + Fizz * 0.1) * Hum * Flutter;
}

double FStreetAmbience::RenderStore()
{
	using namespace StreetAudioDetail;
	static const double FanK = Corner(500.0);
	static const double CompK = Corner(300.0);
	// The coolers' evaporator fans: a soft rush that never stops.
	const double W = Noise();
	FanLow1 += (W - FanLow1) * FanK;
	FanLow2 += (FanLow1 - FanLow2) * FanK;
	double S = FanLow2 * 0.4;
	// The compressor: its motor a little under 60 Hz loaded, the second harmonic strongest, some mechanical noise with it;
	// stopping, it shudders for a moment before it's still.
	Turn(CompPhase, 59.4);
	static const double Motor[4] = {0.6, 1.0, 0.35, 0.2};
	CompLow += (Noise() - CompLow) * CompK;
	const double Since = Time - CompSwitchAt;
	const double Shudder = !bCompressor && Since < 0.9 ? 1.0 + 1.5 * FMath::Sin(TwoPi * 7.0 * Since) * (1.0 - Since / 0.9) : 1.0;
	S += (Harmonics(CompPhase, Motor, 4) * 0.023 + CompLow * 0.05) * CompLevel * Shudder;
	S += Thunk.Run();
	// The ballasts: 120 Hz and its overtones, faint, wavering a little (as the Back Room's).
	Turn(BallastPhase, 120.0);
	static const double Ballast[3] = {0.010, 0.006, 0.004};
	S += Harmonics(BallastPhase, Ballast, 3) * (0.85 + 0.15 * FMath::Sin(TwoPi * 0.13 * Time));
	// Drops striking the windows.
	if (Time >= NextTap)
	{
		Tap.Strike(2600.0 + 1800.0 * Uniform(), 0.003 + 0.004 * Uniform(), (0.003 + 0.009 * Uniform()) * FMath::Sqrt(Rain));
		NextTap = Time + Scatter(28.0 * Rain);
	}
	S += Tap.Run();
	return S;
}
