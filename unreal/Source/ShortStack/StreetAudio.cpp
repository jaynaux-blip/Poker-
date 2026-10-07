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
double Harmonics(double Phase, const double* Weights, int32 Count);

/** Every street ambience alive (game thread only): the stage feeds the game mode's its positioned sounds. */
TArray<FStreetAmbience*>& LiveAmbiences()
{
	static TArray<FStreetAmbience*> List;
	return List;
}

/** A voice's distance falloff (spec 3.7): inverse from a 2 m reference, cut to nothing between 32 and 40 m. */
double Falloff(double Cm)
{
	return FMath::Min(1.0, 200.0 / FMath::Max(Cm, 1.0)) * (1.0 - FMath::SmoothStep(3200.0, 4000.0, Cm));
}

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
	LiveAmbiences().Add(this);
	SideState = (Seed * 2654435761u) | 1u;
	DogFormant.BandPass(700.0, 3.0);
	NextDog = 30.0 + 90.0 * Uniform();
	NextTrain = 180.0 + 240.0 * Uniform();
	HissHp.HighPass(450.0, 0.7);
	HissLp.LowPass(7000.0, 0.6);
	Gutter.BandPass(GutterHz, 5.0);
	Spray.BandPass(1150.0, 0.8);
	Sizzle.BandPass(6500.0, 1.5);
	CrackDecay = FMath::Exp(-Dt / 0.0015);
	// The echo off the fronts across the street and down the block: 0.42 s of the far sounds.
	Echo.SetNumZeroed(static_cast<int32>(Rate * 0.42));
	// The laundromat: the water's band in the washers, the blowers' air, the TV's voice (its vowels move the formants).
	Slosh.BandPass(520.0, 0.7);
	Blower.LowPass(1400.0, 0.7);
	Formant1.BandPass(600.0, 4.0);
	Formant2.BandPass(1700.0, 6.0);
	Pending.Reserve(64);
	// The street was going before the door shut behind the player: the compressor partway through a run, the first car
	// and the first far sound some way off, a spin and a buzzer somewhere ahead in the laundromat.
	NextCompressor = 20.0 + 90.0 * Uniform();
	NextCar = 4.0 + 10.0 * Uniform();
	NextFar = 45.0 + 60.0 * Uniform();
	NextGust = 2.0 + 3.0 * Uniform();
	NextSpin = 20.0 + 60.0 * Uniform();
	NextMachineBuzz = 30.0 + 60.0 * Uniform();
	// The shops' rooms: a small speaker's band (a radio's, the TV's), a jukebox's warmth, the voices' formants, the flat-top's
	// fat, the diner's chatter.
	Speaker.BandPass(1300.0, 0.55);
	SpeakerLow.LowPass(2400.0, 0.7);
	TalkF1.BandPass(650.0, 4.0);
	TalkF2.BandPass(1800.0, 5.0);
	SizzleBand.BandPass(5200.0, 0.7);
	ChatterBand.BandPass(720.0, 1.1);
}

void FStreetAmbience::SetInside(float InInside, float InDoorsOpen)
{
	InsideTarget = FMath::Clamp(static_cast<double>(InInside), 0.0, 1.0);
	DoorsTarget = FMath::Clamp(static_cast<double>(InDoorsOpen), 0.0, 1.0);
}

void FStreetAmbience::SetRoom(int32 InTone, float InDoorOpen)
{
	// A shop's room (3..7); the one being left keeps playing while it fades (Render switches when it's out).
	const bool bShop = InTone >= 3 && InTone <= 7;
	RoomWanted = bShop ? InTone : 0;
	RoomTarget = bShop ? 1.0 : 0.0;
	RoomDoorTarget = bShop ? FMath::Clamp(static_cast<double>(InDoorOpen), 0.0, 1.0) : RoomDoorTarget;
	if (bShop && (RoomTone == 0 || RoomIn < 1e-3))
	{
		RoomTone = InTone;
	}
}

void FStreetAmbience::SetWash(float InInside, float InDoorOpen, float InNear)
{
	WashTarget = FMath::Clamp(static_cast<double>(InInside), 0.0, 1.0);
	WashDoorTarget = FMath::Clamp(static_cast<double>(InDoorOpen), 0.0, 1.0);
	WashNearTarget = FMath::Clamp(static_cast<double>(InNear), 0.0, 1.0);
}

// ------------------------------------------------------------------ the doors

void FStreetAmbience::Strike(double Delay, double Hz, double Seconds, double Amp)
{
	if (Pending.Num() >= 96)
	{
		return; // (never this many at once; a stuck queue mustn't grow)
	}
	FStrike S;
	S.At = Time + FMath::Max(0.0, Delay);
	S.Hz = Hz;
	S.Seconds = Seconds;
	S.Amp = Amp;
	// Kept in the order they're due.
	int32 Index = Pending.Num();
	while (Index > 0 && Pending[Index - 1].At > S.At)
	{
		--Index;
	}
	Pending.Insert(S, Index);
}

double FStreetAmbience::RenderStrikes()
{
	while (Pending.Num() > 0 && Pending[0].At <= Time)
	{
		const FStrike& S = Pending[0];
		Voices[NextVoice].Strike(S.Hz, S.Seconds, S.Amp);
		NextVoice = (NextVoice + 1) % VoiceCount;
		Pending.RemoveAt(0, 1, EAllowShrinking::No);
	}
	double Out = 0.0;
	for (FRing& Voice : Voices)
	{
		Out += Voice.Run();
	}
	return Out;
}

double FStreetAmbience::FBuzzer::Run(double Now)
{
	const double T = Now - At;
	if (T < 0.0 || T >= Length)
	{
		return 0.0;
	}
	// Its coil's hum and every overtone of it, driven hard: the harsh edge of an old buzzer.
	StreetAudioDetail::Turn(Phase, Hz);
	static const double Core[5] = {1.0, 0.6, 0.45, 0.3, 0.2};
	const double Buzz = std::tanh(2.5 * StreetAudioDetail::Harmonics(Phase, Core, 5)) / 2.5;
	const double Env = FMath::Min(1.0, T / 0.01) * FMath::Min(1.0, (Length - T) / 0.02);
	return Buzz * Env * Gain;
}

void FStreetAmbience::RingBell(float Hard)
{
	// Two small brass bells on a strap: each knock rings their partials (inharmonic, the high ones dying first), and the
	// strap jangles them a few more times, softer, as it settles.
	static const double Partials[4] = {1.0, 1.59, 2.36, 3.02};
	static const double Decays[4] = {1.1, 0.7, 0.45, 0.3};
	static const double Levels[4] = {1.0, 0.55, 0.4, 0.25};
	const double H = FMath::Clamp(static_cast<double>(Hard), 0.0, 1.0);
	const int32 Jangles = H > 0.7 ? 5 : 3;
	double At = 0.0;
	for (int32 K = 0; K < Jangles; ++K)
	{
		const double Amp = 0.055 * H * FMath::Pow(0.62, static_cast<double>(K)) * (0.8 + 0.4 * Uniform());
		const double Bell = (K % 2 == 0 ? 2050.0 : 2480.0) * (1.0 + 0.01 * Noise());
		for (int32 P = 0; P < 4; ++P)
		{
			Strike(At, Bell * Partials[P], Decays[P], Amp * Levels[P]);
		}
		At += 0.07 + 0.09 * Uniform();
	}
}

void FStreetAmbience::LockRattle()
{
	// The key going in and meeting pins it doesn't know, jiggled; then the knob tried, the door knocking in its frame twice.
	Strike(0.0, 3400.0, 0.012, 0.05);
	Strike(0.05, 4100.0, 0.008, 0.035);
	Strike(0.11, 2900.0, 0.01, 0.04);
	for (int32 K = 0; K < 5; ++K)
	{
		Strike(0.18 + 0.035 * K + 0.015 * Uniform(), 2600.0 + 2200.0 * Uniform(), 0.006 + 0.006 * Uniform(), 0.03 + 0.02 * Uniform());
	}
	for (const double At : {0.42, 0.61})
	{
		Strike(At, 82.0, 0.09, 0.22);
		Strike(At, 140.0, 0.06, 0.1);
		Strike(At + 0.004, 950.0, 0.03, 0.05);
	}
}

void FStreetAmbience::TryHandle()
{
	// The lever pressed down against the lock, its spring; the steel door pulled twice against its stop (heavier than a
	// wooden one, and it barely gives).
	Strike(0.0, 1800.0, 0.02, 0.045);
	Strike(0.03, 2700.0, 0.01, 0.03);
	for (const double At : {0.12, 0.3})
	{
		Strike(At, 58.0, 0.12, 0.24);
		Strike(At, 96.0, 0.08, 0.12);
		Strike(At + 0.003, 760.0, 0.04, 0.05);
	}
}

void FStreetAmbience::PressBuzzer()
{
	// The button's click, the buzz held a second, the click letting go.
	Strike(0.0, 2300.0, 0.006, 0.04);
	DoorBuzzer.At = Time + 0.03;
	DoorBuzzer.Length = 1.1;
	DoorBuzzer.Hz = 118.0;
	DoorBuzzer.Gain = 0.05;
	DoorBuzzer.Phase = 0.0;
	Strike(1.14, 1900.0, 0.005, 0.025);
}

void FStreetAmbience::SteelDoor()
{
	// The latch and the push bar; the heavy door swinging, its weight in the frame; its closer pulling it to, and the latch
	// (rung out before the street's level gives way to the Back Room's, 1.4 s on, a quarter second of it already queued).
	Strike(0.0, 2200.0, 0.012, 0.07);
	Strike(0.02, 1350.0, 0.025, 0.05);
	Strike(0.15, 64.0, 0.35, 0.32);
	Strike(0.15, 108.0, 0.22, 0.14);
	Strike(0.17, 420.0, 0.08, 0.05);
	Strike(0.85, 55.0, 0.28, 0.26);
	Strike(0.85, 2600.0, 0.008, 0.05);
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
	static const double RoomK = Settle(0.12);
	for (int32 I = 0; I < N; ++I)
	{
		Time += Dt;
		if (!bPaused)
		{
			Clock += Dt;
		}
		Ease(Inside, InsideTarget, ListenK);
		Ease(Doors, DoorsTarget, ListenK);
		Ease(WashIn, WashTarget, ListenK);
		Ease(WashDoor, WashDoorTarget, ListenK);
		Ease(WashNear, WashNearTarget, ListenK);
		// A shop's room: in over a third of a second at the door (a different shop's waits for the last one to fade).
		if (RoomWanted != RoomTone && RoomIn < 1e-3)
		{
			RoomTone = RoomWanted;
		}
		Ease(RoomIn, RoomWanted == RoomTone ? RoomTarget : 0.0, RoomK);
		Ease(RoomDoor, RoomDoorTarget, ListenK);
		Ease(Rain, RainTarget, RainK);
		Ease(Hum, HumTarget, HumK);
		Fade = Fade < FadeTarget ? FMath::Min(FadeTarget, Fade + FadeStep) : FMath::Max(FadeTarget, Fade - FadeStep);

		// The street: its beds, what passes (and its echo down the block), the sign overhead.
		const double Away = RenderCar() + RenderFar();
		const double Street = RenderStreet() + Away + EchoOf(Away) + RenderNeon();
		// Heard through the store's glass (or the laundromat's) it's quieter and the top is gone, from 16 kHz on the
		// sidewalk to 650 Hz with the door shut; through an open door much less so.
		const double Muffle = FMath::Max3(Inside * (1.0 - 0.6 * Doors), WashIn * (1.0 - 0.6 * WashDoor), RoomIn * (1.0 - 0.6 * RoomDoor));
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
		// The Wash & Fold: all round the listener inside it; out on the sidewalk in front of it faintly through its glass,
		// more with its door open.
		const double WashLevel = WashIn + (1.0 - WashIn) * WashNear * (0.06 + 0.3 * WashDoor);
		if (WashLevel > 0.001)
		{
			S += RenderWash() * WashLevel;
		}
		// A shop's room all round the listener inside it (its open door's leak outside is the stage's positioned voice).
		if (RoomIn > 0.001 && RoomTone != 0)
		{
			S += RenderRoom() * RoomIn;
		}

		// The doors' own sounds ring on as the street falls away behind the player.
		S = S * Fade + RenderStrikes() + DoorBuzzer.Run(Time);
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

// ------------------------------------------------------------------ the Wash & Fold

double FStreetAmbience::RenderWash()
{
	using namespace StreetAudioDetail;
	static const double RumbleK = Corner(90.0);
	static const double SpinK = Corner(800.0);
	static const double SyllableK = Corner(18.0);
	// The washers: three drums turning back and forth, the water thrown up and slapped down as each turns (a band of
	// noise swelling with the cube of the turn), the motors' hum under them.
	double Throw = 0.0;
	for (int32 K = 0; K < 3; ++K)
	{
		Turn(SloshPhase[K], SloshRate[K]);
		const double Sv = FMath::Abs(FMath::Sin(SloshPhase[K]));
		Throw += Sv * Sv * Sv;
	}
	double S = Slosh.Run(Noise()) * Throw * 0.06;
	Turn(MotorPhase, 47.0);
	static const double Motor[4] = {0.5, 1.0, 0.4, 0.2};
	S += Harmonics(MotorPhase, Motor, 4) * 0.006;
	// Now and then one winds up into its spin: a whine climbing to speed and the drum's roar with it, holding, coasting down.
	if (Clock >= NextSpin)
	{
		SpinAt = Time;
		SpinLength = 25.0 + 20.0 * Uniform();
		NextSpin = Clock + SpinLength + 40.0 + 80.0 * Uniform();
	}
	const double Spun = Time - SpinAt;
	if (Spun >= 0.0 && Spun < SpinLength)
	{
		const double Speed = FMath::SmoothStep(0.0, 8.0, Spun) * (1.0 - FMath::SmoothStep(SpinLength - 6.0, SpinLength, Spun));
		Turn(SpinPhase, (30.0 + 120.0 * Speed) * 6.0);
		static const double Whine[3] = {1.0, 0.3, 0.15};
		SpinLow += (Noise() - SpinLow) * SpinK;
		S += (Harmonics(SpinPhase, Whine, 3) * 0.012 + SpinLow * 0.06) * Speed;
	}
	// The dryers: a low rumble breathing with the drums, the clothes falling as they turn, zips and buttons ticking on the
	// steel, the blowers' air through the lint screens.
	RumbleBrown = RumbleBrown * 0.985 + Noise() * 0.15;
	Rumble1 += (RumbleBrown - Rumble1) * RumbleK;
	Rumble2 += (Rumble1 - Rumble2) * RumbleK;
	S += Rumble2 * 0.5 * (0.85 + 0.15 * FMath::Sin(TwoPi * 0.8 * Time));
	if (Time >= NextTumble)
	{
		Tumble.Strike(55.0 + 40.0 * Uniform(), 0.05 + 0.05 * Uniform(), 0.03 + 0.05 * Uniform());
		NextTumble = Time + 0.18 + 0.5 * Uniform();
	}
	S += Tumble.Run();
	if (Time >= NextClink)
	{
		Clink.Strike(2300.0 + 2800.0 * Uniform(), 0.008 + 0.02 * Uniform(), 0.004 + 0.009 * Uniform());
		NextClink = Time + Scatter(2.2);
	}
	S += Clink.Run();
	S += Blower.Run(Noise()) * 0.03;
	// A machine done: its buzzer, now and then (one that came due with nobody near enough to hear it is let go).
	if (Clock >= NextMachineBuzz)
	{
		if (Clock - NextMachineBuzz < 2.0)
		{
			MachineBuzzer.At = Time;
			MachineBuzzer.Length = 1.2 + 0.8 * Uniform();
			MachineBuzzer.Hz = Uniform() < 0.5 ? 120.0 : 150.0;
			MachineBuzzer.Gain = 0.035;
			MachineBuzzer.Phase = 0.0;
		}
		NextMachineBuzz = Clock + 45.0 + 110.0 * Uniform();
	}
	S += MachineBuzzer.Run(Time);
	// The TV up on the back wall: a voice through its little speaker, syllable by syllable in phrases, its pitch wandering,
	// each syllable's vowel moving the formants.
	if (Clock >= NextPhrase)
	{
		bTalking = !bTalking;
		NextPhrase = Clock + (bTalking ? 1.5 + 4.5 * Uniform() : 0.25 + 1.0 * Uniform());
	}
	if (Time >= NextSyllable)
	{
		SyllableTarget = bTalking ? 0.35 + 0.65 * Uniform() : 0.0;
		NextSyllable = Time + 0.08 + 0.15 * Uniform();
		Formant1.BandPass(350.0 + 500.0 * Uniform(), 4.0);
		Formant2.BandPass(1000.0 + 1300.0 * Uniform(), 6.0);
		VoiceHz = 110.0 + 70.0 * Uniform();
	}
	Syllable += (SyllableTarget - Syllable) * SyllableK;
	Turn(VoicePhase, VoiceHz);
	static const double Glottal[8] = {1.0, 0.5, 0.33, 0.25, 0.2, 0.16, 0.14, 0.12};
	const double Throat = Harmonics(VoicePhase, Glottal, 8) * 0.7 + Noise() * 0.3;
	S += (Formant1.Run(Throat) + 0.6 * Formant2.Run(Throat)) * Syllable * 0.02;
	// Its ballasts, and the rain on its window.
	Turn(WashBallastPhase, 120.0);
	static const double Ballast[3] = {0.008, 0.005, 0.003};
	S += Harmonics(WashBallastPhase, Ballast, 3);
	if (Time >= NextWashTap)
	{
		WashTap.Strike(2400.0 + 1800.0 * Uniform(), 0.003 + 0.004 * Uniform(), (0.002 + 0.007 * Uniform()) * FMath::Sqrt(Rain));
		NextWashTap = Time + Scatter(24.0 * Rain);
	}
	S += WashTap.Run();
	return S;
}

// ------------------------------------------------------------------ the shops' rooms (spec 3.7's room tones by zone)

double FStreetAmbience::RenderBallast(double Level)
{
	using namespace StreetAudioDetail;
	// The tubes' ballasts: 120 Hz with its odd-heavy harmonics, a little rasp riding on it.
	Turn(RoomBallastPhase, 120.0);
	static const double Weights[5] = {0.5, 0.18, 0.3, 0.08, 0.12};
	return Harmonics(RoomBallastPhase, Weights, 5) * 0.006 * Level + Sizzle.Run(Noise()) * 0.0015 * Level;
}

double FStreetAmbience::RenderMusic(double Bpm, double Small)
{
	using namespace StreetAudioDetail;
	// Four chords, a bar each (I vi IV V in C), three notes and a root under them, a kick on the beat and a hat off it.
	static const double Chords[4][3] = {{261.63, 329.63, 392.0}, {220.0, 261.63, 329.63}, {174.61, 220.0, 261.63}, {196.0, 246.94, 293.66}};
	static const double Roots[4] = {65.41, 55.0, 43.65, 49.0};
	const double BeatLength = 60.0 / FMath::Max(40.0, Bpm);
	const double Before = BeatClock;
	BeatClock += Dt;
	if (BeatClock >= BeatLength)
	{
		BeatClock -= BeatLength;
		MusicBeat = (MusicBeat + 1) % 16;
		KickEnv = 1.0;
		KickHz = 120.0;
	}
	if (BeatClock >= BeatLength * 0.5 && Before < BeatLength * 0.5)
	{
		HatEnv = 1.0;
	}
	const int32 Chord = (MusicBeat / 4) % 4;
	double Pad = 0.0;
	for (int32 K = 0; K < 3; ++K)
	{
		Turn(MusicPhase[K], Chords[Chord][K] * (1.0 + 0.0015 * K));
		Pad += FMath::Sin(MusicPhase[K]) + 0.3 * FMath::Sin(2.0 * MusicPhase[K]);
	}
	// The pad swells with each beat a little (a strummed feel), the bass plucked on the beat.
	const double Strum = 0.55 + 0.45 * FMath::Exp(-BeatClock / (BeatLength * 0.6));
	Turn(BassPhase, Roots[Chord]);
	static const double BassW[3] = {0.8, 0.35, 0.15};
	const double Bass = Harmonics(BassPhase, BassW, 3) * FMath::Exp(-BeatClock / (BeatLength * 0.9));
	// The kick: a sine dropping from 120 to 50 Hz in a few hundredths of a second.
	KickHz = 50.0 + (KickHz - 50.0) * 0.9992;
	Turn(KickPhase, KickHz);
	KickEnv *= 0.99975;
	const double Kick = FMath::Sin(KickPhase) * KickEnv * KickEnv;
	HatEnv *= 0.9985;
	const double Hiss = Noise();
	HatLow += (Hiss - HatLow) * 0.35;
	const double Hat = (Hiss - HatLow) * HatEnv;
	const double Mix = std::tanh(Pad * 0.06 * Strum) + Bass * 0.05 + Kick * 0.07 + Hat * 0.02;
	// Through a small speaker (a radio's band, no bottom), or warm and full (a jukebox, the gym's).
	const double Narrow = Speaker.Run(Mix) * 1.6;
	const double Warm = SpeakerLow.Run(Mix);
	return FMath::Lerp(Warm, Narrow, Small);
}

double FStreetAmbience::RenderTalk(double Pitch)
{
	using namespace StreetAudioDetail;
	// Someone talking on the air: phrases of syllables with pauses, the pitch wandering, the vowels moving the formants.
	if (Time >= TalkNextPhrase)
	{
		bTalkOn = !bTalkOn;
		TalkNextPhrase = Time + (bTalkOn ? 1.5 + 3.5 * Uniform() : 0.3 + 0.9 * Uniform());
	}
	if (Time >= TalkNextSyl)
	{
		TalkSylTarget = bTalkOn ? 0.4 + 0.6 * Uniform() : 0.0;
		TalkNextSyl = Time + 0.09 + 0.14 * Uniform();
		TalkHz = Pitch * (0.85 + 0.3 * Uniform());
		TalkF1.BandPass(450.0 + 450.0 * Uniform(), 4.0);
		TalkF2.BandPass(1200.0 + 1100.0 * Uniform(), 5.0);
	}
	static const double SylK = Settle(0.025);
	Ease(TalkSyl, TalkSylTarget, SylK);
	Turn(TalkPhase, TalkHz);
	// A buzzy glottal pulse (a few harmonics) and a little breath, through the two formants.
	static const double Glottal[6] = {1.0, 0.7, 0.5, 0.35, 0.25, 0.18};
	const double Source = Harmonics(TalkPhase, Glottal, 6) + Noise() * 0.15;
	return (TalkF1.Run(Source) + 0.6 * TalkF2.Run(Source)) * TalkSyl * 0.05;
}

double FStreetAmbience::RenderRoom()
{
	using namespace StreetAudioDetail;
	double S = 0.0;
	switch (RoomTone)
	{
	case 3:
	{
		// Fifth St. Barbers: the radio on the backbar (oldies through its little speaker), the clippers in bursts, the TV
		// up on its bracket talking low, the tube lights.
		S += RenderMusic(96.0, 1.0) * 0.55;
		S += RenderTalk(130.0) * 0.35;
		if (Clock >= NextClip)
		{
			ClipAt = Time;
			ClipLength = 2.0 + 5.0 * Uniform();
			NextClip = Clock + ClipLength + 3.0 + 9.0 * Uniform();
		}
		const double SinceClip = Time - ClipAt;
		if (SinceClip >= 0.0 && SinceClip < ClipLength)
		{
			// The blade's buzz (a saturated 120 Hz with its overtones), up and down at the ends, wavering as it moves on the head.
			const double Env = FMath::Min(1.0, SinceClip / 0.08) * FMath::Min(1.0, (ClipLength - SinceClip) / 0.12) * (0.8 + 0.2 * FMath::Sin(SinceClip * 7.0));
			Turn(ClipPhase, 121.0);
			static const double Blade[6] = {0.6, 0.45, 0.4, 0.3, 0.25, 0.2};
			S += std::tanh(Harmonics(ClipPhase, Blade, 6) * 2.5) * Env * 0.018;
		}
		S += RenderBallast(0.5);
		break;
	}
	case 4:
	{
		// Second Chance Thrift: the fluorescents loud over the racks, a church station's choir and preacher on the radio by the
		// register, hangers knocking on the rails.
		S += RenderBallast(1.6);
		S += RenderMusic(72.0, 1.0) * 0.3;
		S += RenderTalk(110.0) * 0.25;
		if (Clock >= NextRoomStrike)
		{
			RoomRing.Strike(2400.0 + 1800.0 * Uniform(), 0.05, 0.012 + 0.01 * Uniform());
			NextRoomStrike = Clock + 1.5 + 6.0 * Uniform();
		}
		S += RoomRing.Run();
		break;
	}
	case 5:
	{
		// IronHouse: the music up (128), plates set down and clanking on the bars, the treadmills' belts and motors whirring
		// with a runner's footfalls on them.
		S += RenderMusic(128.0, 0.15) * 0.7;
		if (Clock >= NextRoomStrike)
		{
			RoomRing.Strike(380.0 + 520.0 * Uniform(), 0.35, 0.02 + 0.02 * Uniform());
			NextRoomStrike = Clock + 2.0 + 7.0 * Uniform();
		}
		if (Clock >= NextRoomStrike2)
		{
			RoomRing2.Strike(1600.0 + 900.0 * Uniform(), 0.25, 0.008);
			NextRoomStrike2 = Clock + 0.8 + 4.0 * Uniform();
		}
		S += RoomRing.Run() + RoomRing2.Run();
		Turn(WhirPhase, 186.0);
		static const double Motor[3] = {0.6, 0.3, 0.2};
		static const double BeltK = Corner(300.0);
		WhirLow += (Noise() - WhirLow) * BeltK;
		Turn(StepPhase, 2.7);
		const double Step = FMath::Pow(FMath::Max(0.0, FMath::Sin(StepPhase)), 24.0);
		S += Harmonics(WhirPhase, Motor, 3) * 0.004 + WhirLow * 0.03 + WhirLow * Step * 0.25;
		S += RenderBallast(0.3);
		break;
	}
	case 6:
	{
		// Ruby's: the flat-top sizzling behind the pass, dishes and cups, the low talk at the booths, the jukebox.
		if (Time >= NextSizzle)
		{
			SizzleEnv = 0.4 + 0.6 * Uniform();
			NextSizzle = Time + Scatter(40.0);
		}
		SizzleEnv *= 0.9996;
		S += SizzleBand.Run(Noise()) * (0.012 + 0.03 * SizzleEnv);
		if (Clock >= NextRoomStrike)
		{
			RoomRing.Strike(2800.0 + 2200.0 * Uniform(), 0.12, 0.01 + 0.012 * Uniform());
			NextRoomStrike = Clock + 0.6 + 3.5 * Uniform();
		}
		S += RoomRing.Run();
		if (Time >= NextChatter)
		{
			ChatterEnv = 0.2 + 0.8 * Uniform();
			NextChatter = Time + 0.1 + 0.25 * Uniform();
		}
		static const double ChatK = Corner(6.0);
		ChatterLow += (ChatterEnv - ChatterLow) * ChatK;
		S += ChatterBand.Run(Noise()) * ChatterLow * 0.035;
		S += RenderMusic(84.0, 0.35) * 0.4;
		break;
	}
	case 7:
	{
		// Northside Pawn & Loan: a ball game on the radio in the cage (the announcer over the crowd), the ceiling fan
		// ticking round, the tubes, the wall of clocks.
		if (Time >= NextCrowd)
		{
			CrowdTarget = Uniform() < 0.12 ? 1.0 : 0.25 + 0.3 * Uniform();
			NextCrowd = Time + 1.0 + 4.0 * Uniform();
		}
		static const double SwellK = Settle(0.6);
		Ease(CrowdSwell, CrowdTarget, SwellK);
		static const double CrowdK1 = Corner(2500.0);
		static const double CrowdK2 = Corner(500.0);
		const double Air = Noise();
		CrowdLow1 += (Air - CrowdLow1) * CrowdK1;
		CrowdLow2 += (CrowdLow1 - CrowdLow2) * CrowdK2;
		S += Speaker.Run((CrowdLow1 - CrowdLow2) * CrowdSwell * 0.08 + RenderTalk(150.0) * 1.2) * 0.9;
		// The fan's blades ticking round once a turn, its air a soft whoosh with them.
		Turn(FanPhase, 1.3);
		const double Tick = FMath::Pow(FMath::Max(0.0, FMath::Sin(FanPhase)), 60.0);
		static const double FanAirK = Corner(400.0);
		WhirLow += (Noise() - WhirLow) * FanAirK;
		S += Tick * 0.01 * Noise() + WhirLow * (0.008 + 0.006 * FMath::Sin(FanPhase * 3.0));
		// The clocks on the wall: their ticks never quite together.
		ClockTick += Dt;
		if (ClockTick >= 0.5)
		{
			ClockTick -= 0.5;
			RoomRing2.Strike(3200.0 + 1400.0 * Uniform(), 0.012, 0.006 + 0.004 * Uniform());
		}
		S += RoomRing2.Run();
		S += RenderBallast(1.0);
		break;
	}
	default:
		break;
	}
	return S;
}

// ------------------------------------------------------------------ stereo, and the positioned voices (spec 3.7)

FStreetAmbience::~FStreetAmbience()
{
	StreetAudioDetail::LiveAmbiences().Remove(this);
}

TSharedPtr<FStreetAmbience> FStreetAmbience::Live()
{
	TArray<FStreetAmbience*>& List = StreetAudioDetail::LiveAmbiences();
	// The newest: an old one (the last visit's, still going while its stream winds down) gives way to it.
	for (int32 I = List.Num() - 1; I >= 0; --I)
	{
		if (List[I] && List[I]->DoesSharedInstanceExist())
		{
			return List[I]->AsShared();
		}
	}
	return nullptr;
}

void FStreetAmbience::SetListener(const FVector& At, const FVector& Forward, const FVector& Right)
{
	ListenAt = At;
	ListenRight = Right.GetSafeNormal2D();
	if (ListenRight.IsNearlyZero())
	{
		ListenRight = FVector(0.0, 1.0, 0.0);
	}
}

void FStreetAmbience::SetWind(float InWind)
{
	WindTarget = FMath::Clamp(static_cast<double>(InWind), 0.0, 1.0);
}

void FStreetAmbience::SetEmitters(const TArray<FStreetEmitter>& InEmitters)
{
	using namespace StreetAudioDetail;
	// The nearest twelve, by how loud they'd be here.
	TArray<TPair<double, int32>> Order;
	for (int32 I = 0; I < InEmitters.Num(); ++I)
	{
		const double Heard = Falloff(FVector::Dist(InEmitters[I].At, ListenAt)) * InEmitters[I].Level;
		if (Heard > 0.002)
		{
			Order.Emplace(-Heard, I);
		}
	}
	Order.Sort([](const TPair<double, int32>& A, const TPair<double, int32>& B) { return A.Key < B.Key; });
	if (Order.Num() > MaxVoices)
	{
		Order.SetNum(MaxVoices);
	}
	// Each keeps its voice by its id (its noise and filters carry on); the gone go quiet; the new take free voices.
	bool bKeep[MaxVoices] = {};
	TArray<int32> Fresh;
	for (const TPair<double, int32>& O : Order)
	{
		const FStreetEmitter& E = InEmitters[O.Value];
		int32 Found = INDEX_NONE;
		for (int32 V = 0; V < MaxVoices; ++V)
		{
			if (Voices12[V].bActive && Voices12[V].Id == E.Id)
			{
				Found = V;
				break;
			}
		}
		if (Found == INDEX_NONE)
		{
			Fresh.Add(O.Value);
			continue;
		}
		FVoice& V = Voices12[Found];
		V.At = E.At;
		V.Level = E.Level;
		V.Hz = E.Hz;
		bKeep[Found] = true;
	}
	for (int32 V = 0; V < MaxVoices; ++V)
	{
		if (!bKeep[V])
		{
			// Faded out by its gains (they ease to zero), then free.
			Voices12[V].Level = 0.0f;
		}
	}
	for (const int32 Index : Fresh)
	{
		const FStreetEmitter& E = InEmitters[Index];
		for (int32 V = 0; V < MaxVoices; ++V)
		{
			FVoice& Free = Voices12[V];
			if (bKeep[V] || (Free.bActive && (Free.GainL + Free.GainR) > 1e-4))
			{
				continue;
			}
			Free = FVoice();
			Free.bActive = true;
			Free.Id = E.Id;
			Free.Kind = E.Kind;
			Free.At = E.At;
			Free.Level = E.Level;
			Free.Hz = E.Hz;
			Free.Rng = (E.Id * 2654435761u) | 1u;
			switch (E.Kind)
			{
			case EStreetEmitter::Gutter:
				Free.Band.BandPass(1300.0, 1.2);
				break;
			case EStreetEmitter::Steam:
				Free.Band.HighPass(2600.0, 0.7);
				break;
			case EStreetEmitter::RoomLeak:
				Free.Band.LowPass(700.0, 0.7);
				break;
			case EStreetEmitter::Engine:
				Free.Band.BandPass(1900.0, 0.8);
				break;
			default:
				Free.Band.BandPass(5500.0, 1.5);
				break;
			}
			bKeep[V] = true;
			break;
		}
	}
}

void FStreetAmbience::AimVoices()
{
	// Constant-power pan by where each is across the listener's ears, its fall-off with distance, its kind's own level.
	for (FVoice& V : Voices12)
	{
		if (!V.bActive)
		{
			continue;
		}
		const FVector To = V.At - ListenAt;
		const double Dist = To.Size();
		const double Across = Dist > 1.0 ? FMath::Clamp(FVector::DotProduct(To / Dist, ListenRight), -1.0, 1.0) : 0.0;
		const double Angle = (Across + 1.0) * UE_DOUBLE_PI * 0.25;
		const double G = StreetAudioDetail::Falloff(Dist) * V.Level;
		V.TargetL = G * FMath::Cos(Angle);
		V.TargetR = G * FMath::Sin(Angle);
	}
}

double FStreetAmbience::VoiceNoise(FVoice& V)
{
	V.Rng ^= V.Rng << 13;
	V.Rng ^= V.Rng >> 17;
	V.Rng ^= V.Rng << 5;
	return static_cast<double>(V.Rng) / 2147483648.0 - 1.0;
}

double FStreetAmbience::RenderVoice(FVoice& V)
{
	using namespace StreetAudioDetail;
	switch (V.Kind)
	{
	case EStreetEmitter::Hum:
	{
		// A transformer's buzz on the mains and its harmonics, a little sizzle over it.
		Turn(V.Phase, V.Hz);
		static const double Buzz[4] = {0.6, 0.35, 0.22, 0.12};
		const double B = Harmonics(V.Phase, Buzz, 4);
		return std::tanh(B * 2.0) * 0.022 + V.Band.Run(VoiceNoise(V)) * 0.004;
	}
	case EStreetEmitter::Ac:
	{
		// The fan's air beating at its blades' rate, the compressor's hum under it.
		static const double FanK = Corner(420.0);
		V.Low1 += (VoiceNoise(V) - V.Low1) * FanK;
		Turn(V.Phase2, 9.0);
		Turn(V.Phase, V.Hz);
		return V.Low1 * (0.75 + 0.25 * FMath::Sin(V.Phase2)) * 0.09 + FMath::Sin(V.Phase) * 0.01 + FMath::Sin(V.Phase * 2.0) * 0.005;
	}
	case EStreetEmitter::Gutter:
	{
		// Water down the spout and splashing at its foot: a band of noise, bubbles struck in it.
		if (Time >= V.Next)
		{
			V.Ring.Strike(500.0 + 1100.0 * (0.5 + 0.5 * VoiceNoise(V)), 0.02, 0.04);
			V.Next = Time + 0.02 + 0.08 * (0.5 + 0.5 * VoiceNoise(V));
		}
		return V.Band.Run(VoiceNoise(V)) * 0.06 + V.Ring.Run();
	}
	case EStreetEmitter::Steam:
		return V.Band.Run(VoiceNoise(V)) * 0.05;
	case EStreetEmitter::RoomLeak:
	{
		// A room heard through its open door: its murmur (syllables of voices), a radio's beat in some, all of it muffled.
		static const double MurK = Corner(5.0);
		if (Time >= V.Next)
		{
			V.Env = 0.3 + 0.7 * (0.5 + 0.5 * VoiceNoise(V));
			V.Next = Time + 0.12 + 0.2 * (0.5 + 0.5 * VoiceNoise(V));
		}
		V.Low2 += (V.Env - V.Low2) * MurK;
		const int32 Room = static_cast<int32>(V.Hz);
		Turn(V.Phase, 2.0);
		const double Pulse = (Room % 3 == 1) ? FMath::Pow(FMath::Max(0.0, FMath::Sin(V.Phase)), 8.0) * 0.6 : 0.0;
		V.Phase2 += TwoPi * 55.0 * Dt;
		const double Bass = FMath::Sin(V.Phase2) * Pulse * 0.03;
		return V.Band.Run(VoiceNoise(V)) * V.Low2 * 0.05 + Bass;
	}
	case EStreetEmitter::RainRoof:
	{
		// Drops drumming on canvas or sheet metal close by.
		if (Time >= V.Next)
		{
			V.Ring.Strike(900.0 + 1600.0 * (0.5 + 0.5 * VoiceNoise(V)), 0.006 + 0.01 * (0.5 + 0.5 * VoiceNoise(V)), 0.02 + 0.03 * (0.5 + 0.5 * VoiceNoise(V)));
			V.Next = Time + Scatter(60.0 * V.Level + 10.0);
		}
		static const double PatterK = Corner(1800.0);
		V.Low1 += (VoiceNoise(V) - V.Low1) * PatterK;
		return V.Ring.Run() + V.Low1 * 0.02;
	}
	case EStreetEmitter::Engine:
	{
		// The engine (a low pulse, its pitch with its speed), the tyres' roar, their spray when the road is wet (Rain).
		const double Speed = FMath::Max(1.0, static_cast<double>(V.Hz));
		Turn(V.Phase, 28.0 + Speed * 1.6);
		static const double Cyl[3] = {0.5, 0.3, 0.2};
		const double Engine = Harmonics(V.Phase, Cyl, 3) * 0.02;
		static const double RoarK = Corner(240.0);
		V.Low1 += (VoiceNoise(V) - V.Low1) * RoarK;
		const double Wash = V.Band.Run(VoiceNoise(V)) * 0.05 * Rain;
		return Engine + V.Low1 * 0.08 + Wash;
	}
	default:
		return 0.0;
	}
}

double FStreetAmbience::RenderSide()
{
	using namespace StreetAudioDetail;
	// Another hiss of the rain, uncorrelated with the bed's, and the wind's rush: added to one ear and taken from the other.
	SideState ^= SideState << 13;
	SideState ^= SideState >> 17;
	SideState ^= SideState << 5;
	const double N = static_cast<double>(SideState) / 2147483648.0 - 1.0;
	static const double HiK = Corner(6000.0);
	static const double LoK = Corner(450.0);
	SideHigh += (N - SideHigh) * HiK;
	SideLow += (SideHigh - SideLow) * LoK;
	const double Hiss = (SideHigh - SideLow) * 0.06 * Rain;
	// The wind: a slow swell of low noise.
	static const double WindK = Corner(260.0);
	static const double SwellK = Settle(2.5);
	Ease(WindLevel, WindTarget, Settle(3.0));
	WindLow1 += (N - WindLow1) * WindK;
	WindLow2 += (WindLow1 - WindLow2) * WindK;
	if ((static_cast<int64>(Time * 4.0) & 1) == 0 && FMath::Frac(Time * 0.37) < 0.0005)
	{
		SideGust = 0.6 + 0.8 * Uniform();
	}
	Ease(WindSwell, SideGust, SwellK);
	return Hiss + WindLow2 * (0.02 + 0.09 * WindLevel) * WindSwell;
}

double FStreetAmbience::RenderDistant(double& OutPan)
{
	using namespace StreetAudioDetail;
	OutPan = 0.0;
	double S = 0.0;
	// A dog a few yards over: two to four barks, a formant's bark through the rain.
	if (Clock >= NextDog)
	{
		DogAt = Time;
		DogBarks = 2 + static_cast<int32>(Uniform() * 3.0);
		DogPan = Noise() * 0.8;
		NextDog = Clock + 70.0 + 200.0 * Uniform();
	}
	const double SinceDog = Time - DogAt;
	if (SinceDog >= 0.0 && SinceDog < DogBarks * 0.45)
	{
		const double Within = FMath::Fmod(SinceDog, 0.45);
		const double Env = Within < 0.16 ? FMath::Sin(Within / 0.16 * UE_DOUBLE_PI) : 0.0;
		S += DogFormant.Run(Noise()) * Env * 0.035;
		OutPan = DogPan;
	}
	// A train's horn across the river: two notes a minor third apart, swelling and dying away, far down and dull.
	if (Clock >= NextTrain)
	{
		TrainAt = Time;
		TrainPan = 0.6;
		NextTrain = Clock + 300.0 + 420.0 * Uniform();
	}
	const double SinceTrain = Time - TrainAt;
	if (SinceTrain >= 0.0 && SinceTrain < 4.0)
	{
		const double Env = FMath::Sin(FMath::Min(1.0, SinceTrain / 3.6) * UE_DOUBLE_PI) * (SinceTrain < 1.6 || SinceTrain > 2.0 ? 1.0 : 0.4);
		Turn(TrainPhase1, 311.1);
		Turn(TrainPhase2, 370.0);
		const double Horn = std::tanh((FMath::Sin(TrainPhase1) + FMath::Sin(TrainPhase2)) * 1.6);
		static const double DullK = Corner(700.0);
		TrainLow += (Horn - TrainLow) * DullK;
		S += TrainLow * Env * 0.012;
		OutPan = TrainPan;
	}
	return S;
}

void FStreetAmbience::RenderStereo(float* Out, int32 Frames)
{
	using namespace StreetAudioDetail;
	// The bed, as the mono street makes it (its rooms, its doors, its far sounds), then the width, the far sounds and the
	// voices: outside only, muffled as the bed is through a room's glass.
	if (Scratch.Num() < Frames)
	{
		Scratch.SetNumUninitialized(Frames);
	}
	Render(Scratch.GetData(), Frames);
	AimVoices();
	const double Outdoors = 1.0 - FMath::Max3(Inside * (1.0 - 0.6 * Doors), WashIn * (1.0 - 0.6 * WashDoor), RoomIn * (1.0 - 0.6 * RoomDoor));
	const double Step = Frames > 0 ? 1.0 / Frames : 1.0;
	for (int32 F = 0; F < Frames; ++F)
	{
		double L = Scratch[F];
		double R = Scratch[F];
		const double Side = RenderSide() * Outdoors * Fade;
		L += Side;
		R -= Side;
		double Pan = 0.0;
		const double Distant = RenderDistant(Pan) * Outdoors * Fade;
		L += Distant * (1.0 - 0.5 * Pan);
		R += Distant * (1.0 + 0.5 * Pan);
		for (FVoice& V : Voices12)
		{
			if (!V.bActive)
			{
				continue;
			}
			// The gains glide to their aim across the block (no zipper); a voice faded right out is freed.
			V.GainL += (V.TargetL - V.GainL) * Step * 4.0;
			V.GainR += (V.TargetR - V.GainR) * Step * 4.0;
			if (V.Level <= 0.0f && V.GainL + V.GainR < 1e-5)
			{
				V.bActive = false;
				continue;
			}
			const double X = RenderVoice(V) * Outdoors * Fade;
			L += X * V.GainL;
			R += X * V.GainR;
		}
		Out[2 * F] = static_cast<float>(std::tanh(L * 1.2) / 1.2);
		Out[2 * F + 1] = static_cast<float>(std::tanh(R * 1.2) / 1.2);
	}
}
