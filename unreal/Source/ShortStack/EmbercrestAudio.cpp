#include "EmbercrestAudio.h"

namespace EmbercrestAudioDetail
{
constexpr double Rate = 48000.0;
constexpr double Dt = 1.0 / Rate;
constexpr double TwoPi = 2.0 * UE_DOUBLE_PI;

/** A one-pole coefficient with its corner at Hz. */
double Corner(double Hz)
{
	return 1.0 - FMath::Exp(-TwoPi * Hz * Dt);
}

/** A target eased toward over about Seconds, per sample. */
void Ease(double& Value, double Target, double Seconds)
{
	Value += (Target - Value) * (1.0 - FMath::Exp(-Dt / Seconds));
}

void Turn(double& Phase, double Hz)
{
	Phase += TwoPi * Hz * Dt;
	if (Phase > TwoPi)
	{
		Phase -= TwoPi;
	}
}
} // namespace EmbercrestAudioDetail

using namespace EmbercrestAudioDetail;

FEmbercrestAmbience::FEmbercrestAmbience(uint32 Seed) : State(Seed | 1u)
{
	NextRaven = 20.0 + 40.0 * Uniform();
	NextGust = 1.0 + 3.0 * Uniform();
}

double FEmbercrestAmbience::Noise()
{
	// xorshift32, -1..1.
	State ^= State << 13;
	State ^= State >> 17;
	State ^= State << 5;
	return static_cast<double>(State) / 2147483648.0 - 1.0;
}

void FEmbercrestAmbience::SetPlace(float InFire, float InFlicker, float InWater, float InDoors, float InValley)
{
	FireT = FMath::Clamp(InFire, 0.0f, 1.0f);
	FlickerT = FMath::Clamp(InFlicker, 0.4f, 1.4f);
	WaterT = FMath::Clamp(InWater, 0.0f, 1.0f);
	DoorsT = FMath::Clamp(InDoors, 0.0f, 1.0f);
	ValleyT = FMath::Clamp(InValley, 0.0f, 1.0f);
}

void FEmbercrestAmbience::SetDoorway(float InSpinDegrees, float InSwell)
{
	SpinT = FMath::Max(0.0f, InSpinDegrees);
	SwellT = FMath::Clamp(InSwell, 0.0f, 1.0f);
}

void FEmbercrestAmbience::SetBus(float InNear, bool bInRunning, float InPull)
{
	BusT = FMath::Clamp(InNear, 0.0f, 1.0f);
	bBusRunning = bInRunning;
	PullT = FMath::Clamp(InPull, 0.0f, 1.0f);
}

void FEmbercrestAmbience::SetSun(float InElevation)
{
	Sun = InElevation;
}

void FEmbercrestAmbience::Hiss(float Seconds, float Strength)
{
	HissLen = FMath::Max(0.05, static_cast<double>(Seconds));
	HissLeft = HissLen;
	HissGain = FMath::Clamp(static_cast<double>(Strength), 0.0, 1.0);
}

void FEmbercrestAmbience::Crunch(float Volume)
{
	CrunchLeft = 0.22;
	CrunchGain = FMath::Clamp(static_cast<double>(Volume), 0.0, 1.0);
}

void FEmbercrestAmbience::Splash(float Volume)
{
	SplashLeft = 0.12;
	SplashGain = FMath::Clamp(static_cast<double>(Volume), 0.0, 1.0);
}

void FEmbercrestAmbience::FadeOut(float Seconds)
{
	FadeTarget = 0.0;
	FadeStep = 1.0 / (FMath::Max(0.05, static_cast<double>(Seconds)) * Rate);
}

void FEmbercrestAmbience::Render(float* Out, int32 N)
{
	const double WindK1 = Corner(220.0);
	const double WindK2 = Corner(90.0);
	const double NeedleHpK = Corner(900.0);
	const double NeedleLpK = Corner(3200.0);
	const double HumK = Corner(110.0);
	const double RoarK = Corner(260.0);
	const double RoarHpK = Corner(40.0);
	const double CrackK = Corner(3500.0);
	const double WashK = Corner(2600.0);
	const double WashHpK = Corner(300.0);
	const double BubbleK = Corner(900.0);
	const double MuffK = Corner(700.0);
	const double CrowdK = Corner(900.0);
	const double CrowdHpK = Corner(250.0);
	const double HvacK = Corner(1300.0);
	const double HissHpK = Corner(2500.0);
	const double HissLpK = Corner(9000.0);
	const double CrunchK = Corner(2400.0);
	const double SplashHpK = Corner(1500.0);
	const double SplashLpK = Corner(7000.0);
	const double RavenK = Corner(1400.0);
	const double RavenHpK = Corner(500.0);
	for (int32 I = 0; I < N; ++I)
	{
		Time += Dt;
		if (!bPaused)
		{
			Clock += Dt;
		}
		Ease(Fire, FireT, 0.25);
		Ease(Flicker, FlickerT, 0.05);
		Ease(Water, WaterT, 0.25);
		Ease(Doors, DoorsT, 0.25);
		Ease(Valley, ValleyT, 0.5);
		Ease(Spin, SpinT, 0.4);
		Ease(Swell, SwellT, 0.6);
		Ease(Bus, BusT, 0.3);
		Ease(Pull, PullT, 0.5);
		if (Fade != FadeTarget)
		{
			Fade = Fade < FadeTarget ? FMath::Min(FadeTarget, Fade + FadeStep) : FMath::Max(FadeTarget, Fade - FadeStep);
		}
		double S = 0.0;

		// The wind: a low bed that never stops, gusts that come and go, the needles hissing in them.
		if (Clock >= NextGust)
		{
			GustTarget = 0.15 + 0.85 * Uniform() * Uniform();
			NextGust = Clock + 1.5 + 5.0 * Uniform();
		}
		Ease(Gust, GustTarget, 1.4);
		const double Wn = Noise();
		WindLow1 += (Wn - WindLow1) * WindK1;
		WindLow2 += (WindLow1 - WindLow2) * WindK2;
		S += WindLow2 * (0.9 + 1.6 * Gust) * 0.55;
		const double Nn = Noise();
		NeedleLp += (Nn - NeedleLp) * NeedleLpK;
		NeedleHp += (NeedleLp - NeedleHp) * NeedleHpK;
		NeedleLp2 += ((NeedleLp - NeedleHp) - NeedleLp2) * NeedleLpK;
		S += NeedleLp2 * Gust * Gust * 0.09;

		// A raven by day: two or three calls, far off.
		if (Sun > 3.0 && Clock >= NextRaven)
		{
			RavenAt = Clock;
			RavenCalls = 2 + static_cast<int32>(2.0 * Uniform());
			NextRaven = Clock + 40.0 + 60.0 * Uniform();
		}
		const double SinceRaven = Clock - RavenAt;
		if (SinceRaven >= 0.0 && SinceRaven < RavenCalls * 0.55)
		{
			const double Call = FMath::Fmod(SinceRaven, 0.55);
			if (Call < 0.32)
			{
				const double Env = FMath::Sin(UE_DOUBLE_PI * Call / 0.32);
				Turn(RavenPhase, 310.0 - 60.0 * Call / 0.32);
				// A harsh buzzy saw through a throat-like band.
				const double Saw = RavenPhase / UE_DOUBLE_PI - 1.0 + 0.3 * Noise();
				RavenBand += (Saw - RavenBand) * RavenK;
				RavenBand2 += (RavenBand - RavenBand2) * RavenHpK;
				S += (RavenBand - RavenBand2) * Env * Env * 0.05;
			}
		}

		// The valley's hum at the parapet: traffic and the city, nine hundred meters down.
		const double Hn = Noise();
		HumLow += (Hn - HumLow) * HumK;
		HumLow2 += (HumLow - HumLow2) * HumK;
		S += HumLow2 * Valley * (Sun > 0.0 ? 0.5 : 0.32) * 0.6;

		// The fire bowls: the gas roar's body and its hiss, the crackle of the lava rock, all with the flicker.
		const double Fn = Noise();
		RoarLow += (Fn - RoarLow) * RoarK;
		RoarLow2 += (RoarLow - RoarLow2) * RoarK;
		RoarHp += (RoarLow2 - RoarHp) * RoarHpK;
		S += (RoarLow2 - RoarHp) * Fire * Flicker * 1.4;
		if (Clock >= NextCrack)
		{
			CrackEnv = 0.4 + 0.6 * Uniform();
			CrackDecay = FMath::Exp(-Dt / (0.004 + 0.01 * Uniform()));
			NextCrack = Clock + 0.02 + 0.25 * Uniform() * Uniform();
		}
		CrackLp += (Noise() * CrackEnv - CrackLp) * CrackK;
		CrackEnv *= CrackDecay;
		S += CrackLp * Fire * 0.35;

		// The cascades: water over the weirs, a wash with a bubbling band in it.
		const double Wa = Noise();
		WashLp += (Wa - WashLp) * WashK;
		WashHp += (WashLp - WashHp) * WashHpK;
		if (Clock >= NextBubble)
		{
			BubbleTarget = Uniform();
			NextBubble = Clock + 0.03 + 0.08 * Uniform();
		}
		Ease(BubbleAm, BubbleTarget, 0.02);
		BubbleLp += (Noise() - BubbleLp) * BubbleK;
		S += ((WashLp - WashHp) * 0.32 + BubbleLp * BubbleAm * 0.12) * Water;

		// The doorway: the brushes sweeping as each wing passes, and the casino through the glass when it opens.
		const double Was = SpinAngle;
		SpinAngle = FMath::Fmod(SpinAngle + Spin * Dt, 90.0);
		if (SpinAngle < Was)
		{
			BrushEnv = 1.0;
		}
		BrushEnv *= 0.99985;
		BrushLp += (Noise() - BrushLp) * CrowdK;
		S += BrushLp * BrushEnv * Doors * 0.05;
		const double Open = FMath::Clamp(Doors * (0.25 + 0.75 * Swell + 0.3 * FMath::Max(0.0, Spin - 12.0) / 12.0), 0.0, 1.0);
		if (Clock >= NextChime)
		{
			ChimeAt[NextChimeVoice] = Clock;
			static const double Notes[8] = {784.0, 880.0, 988.0, 1175.0, 1319.0, 1568.0, 1760.0, 2093.0};
			ChimeHz[NextChimeVoice] = Notes[static_cast<int32>(Uniform() * 7.99)];
			NextChimeVoice = (NextChimeVoice + 1) % 4;
			NextChime = Clock + 0.15 + 0.9 * Uniform();
		}
		double Chimes = 0.0;
		for (int32 V = 0; V < 4; ++V)
		{
			const double Age = Clock - ChimeAt[V];
			if (Age >= 0.0 && Age < 1.2)
			{
				Turn(ChimePhase[V], ChimeHz[V]);
				Chimes += FMath::Sin(ChimePhase[V]) * FMath::Exp(-Age * 4.0) * 0.5;
			}
		}
		CrowdLp += (Noise() - CrowdLp) * CrowdK;
		CrowdHp += (CrowdLp - CrowdHp) * CrowdHpK;
		const double Casino = Chimes * 0.6 + (CrowdLp - CrowdHp) * 0.5;
		MuffLp += (Casino - MuffLp) * MuffK;
		MuffLp2 += (MuffLp - MuffLp2) * MuffK;
		S += FMath::Lerp(MuffLp2, MuffLp, Swell) * Open * 0.35;

		// The 14: its electric drive's whine, a low drone, the HVAC's rush; rising as it pulls away.
		if (bBusRunning && Bus > 0.0005)
		{
			Turn(MotorPhase, 380.0 + 900.0 * Pull);
			Turn(DronePhase, 42.0 + 30.0 * Pull);
			const double Hv = Noise();
			HvacLp += (Hv - HvacLp) * HvacK;
			HvacLp2 += (HvacLp - HvacLp2) * Corner(120.0);
			const double Motor = FMath::Sin(MotorPhase) * 0.04 * (0.4 + Pull) + (FMath::Sin(DronePhase) + 0.4 * FMath::Sin(2.0 * DronePhase)) * 0.12;
			S += (Motor + (HvacLp - HvacLp2) * 0.35) * Bus;
		}
		if (HissLeft > 0.0)
		{
			HissLeft -= Dt;
			const double Age = HissLen - HissLeft;
			const double Env = FMath::Min(1.0, Age / 0.02) * FMath::Clamp(HissLeft / (0.4 * HissLen), 0.0, 1.0);
			const double X = Noise();
			HissLp += (X - HissLp) * HissLpK;
			HissHp += (HissLp - HissHp) * HissHpK;
			S += (HissLp - HissHp) * Env * HissGain * (0.25 + 0.75 * Bus) * 0.9;
		}

		// Footsteps: snow's crunch (grains in a lowpassed burst), a splash on the wet stone.
		if (CrunchLeft > 0.0)
		{
			CrunchLeft -= Dt;
			if (Uniform() < 0.08)
			{
				CrunchGrain = Noise();
			}
			CrunchGrain *= 0.97;
			CrunchLp += (CrunchGrain + 0.3 * Noise() - CrunchLp) * CrunchK;
			const double Env = FMath::Sin(UE_DOUBLE_PI * FMath::Clamp(1.0 - CrunchLeft / 0.22, 0.0, 1.0));
			S += CrunchLp * Env * CrunchGain * 0.9;
		}
		if (SplashLeft > 0.0)
		{
			SplashLeft -= Dt;
			const double X = Noise();
			SplashLp += (X - SplashLp) * SplashLpK;
			SplashHp += (SplashLp - SplashHp) * SplashHpK;
			const double Env = FMath::Exp(-(0.12 - SplashLeft) * 30.0);
			S += (SplashLp - SplashHp) * Env * SplashGain * 0.5;
		}
		Out[I] = static_cast<float>(FMath::Clamp(S * Fade * 0.7, -1.0, 1.0));
	}
}
