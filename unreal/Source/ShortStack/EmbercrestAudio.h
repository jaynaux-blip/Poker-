#pragma once

#include "CoreMinimal.h"

/**
 * The Embercrest's air, synthesized like the street's and the card room's (no audio assets): the mountain wind in soft
 * beds and gusts through the spruces, a raven now and then by day, the city's faint hum up from the valley at the
 * parapet; the fire bowls' gas roar and crackle (following their flicker), the cascades running over their weirs, the
 * revolving door's brushes and the casino's slot chimes muffled through it; the 14's hybrid idle and its air, the hiss of
 * its brakes, its kneeling and its doors, and its motor winding up as it pulls away; the hero's steps crunching on snow or
 * splashing on the wet stone. No rain: up here it's always clear. Mono, 48 kHz, for UNightOneAudio::StartAmbience; the
 * setters and Render are all called on the game thread.
 */
class FEmbercrestAmbience
{
public:
	explicit FEmbercrestAmbience(uint32 Seed);

	/** Where the listener is: how near the fire bowls (0..1) and their flicker, the cascades, the doors, the parapet. */
	void SetPlace(float InFire, float InFlicker, float InWater, float InDoors, float InValley);
	/** The revolving door's speed (degrees a second: 12 idle, 24 with someone going through) and the casino's swell (0..1). */
	void SetDoorway(float InSpinDegrees, float InSwell);
	/** How near the bus is (0..1), whether its engine runs, and how hard it's pulling away (0..1). */
	void SetBus(float InNear, bool bInRunning, float InPull);
	/** The sun's elevation (degrees): ravens by day, the valley's traffic busier then. */
	void SetSun(float InElevation);
	/** Nothing new starts while paused (the wind goes on). */
	void SetPaused(bool bInPaused) { bPaused = bInPaused; }
	/** The air brakes, a kneel, a door (Seconds long, Strength 0..1), heard at the bus's distance. */
	void Hiss(float Seconds, float Strength);
	/** A footstep in snow (Volume 0..1). */
	void Crunch(float Volume);
	/** A footstep's splash on wet stone (Volume 0..1). */
	void Splash(float Volume);
	/** Fades everything out over Seconds (through the doors, onto the bus). */
	void FadeOut(float Seconds);

	void Render(float* Out, int32 N);

private:
	double Noise();
	double Uniform() { return 0.5 + 0.5 * Noise(); }

	uint32 State = 1;
	double Time = 0.0;
	double Clock = 0.0;
	bool bPaused = false;

	// What the listener hears from where they are (targets, and eased per sample).
	double FireT = 0.0, Fire = 0.0;
	double FlickerT = 1.0, Flicker = 1.0;
	double WaterT = 0.0, Water = 0.0;
	double DoorsT = 0.0, Doors = 0.0;
	double ValleyT = 0.0, Valley = 0.0;
	double SpinT = 12.0, Spin = 12.0;
	double SwellT = 0.0, Swell = 0.0;
	double BusT = 0.0, Bus = 0.0;
	double PullT = 0.0, Pull = 0.0;
	bool bBusRunning = false;
	double Sun = 0.0;
	double Fade = 0.0;
	double FadeTarget = 1.0;
	double FadeStep = 1.0 / (1.5 * 48000.0);

	// The wind: a low bed, the gusts' envelope, the hiss of needles in them.
	double WindLow1 = 0.0, WindLow2 = 0.0;
	double Gust = 0.4, GustTarget = 0.4, NextGust = 2.0;
	double NeedleHp = 0.0, NeedleLp = 0.0, NeedleLp2 = 0.0;
	// A raven, far off.
	double RavenAt = -10.0, NextRaven = 30.0, RavenPhase = 0.0, RavenBand = 0.0, RavenBand2 = 0.0;
	int32 RavenCalls = 0;
	// The valley's hum.
	double HumLow = 0.0, HumLow2 = 0.0;
	// The fire: the roar's body, the crackle's clicks.
	double RoarLow = 0.0, RoarLow2 = 0.0, RoarHp = 0.0;
	double CrackEnv = 0.0, CrackDecay = 0.0, NextCrack = 0.1, CrackLp = 0.0;
	// The water: a broad wash, a bubbling band.
	double WashLp = 0.0, WashHp = 0.0, BubbleLp = 0.0, BubbleAm = 0.0, BubbleTarget = 0.0, NextBubble = 0.0;
	// The doorway: the brushes on each quarter turn, the slot chimes inside (muffled).
	double SpinAngle = 0.0, BrushEnv = 0.0, BrushLp = 0.0;
	double ChimeAt[4] = {-10.0, -10.0, -10.0, -10.0};
	double ChimeHz[4] = {880.0, 1320.0, 1047.0, 1568.0};
	double ChimePhase[4] = {0.0, 0.0, 0.0, 0.0};
	int32 NextChimeVoice = 0;
	double NextChime = 1.0;
	double MuffLp = 0.0, MuffLp2 = 0.0;
	double CrowdLp = 0.0, CrowdHp = 0.0;
	// The bus: its motor's whine and drone, the HVAC's rush, the air.
	double MotorPhase = 0.0, DronePhase = 0.0, HvacLp = 0.0, HvacLp2 = 0.0;
	double HissLeft = 0.0, HissLen = 0.0, HissGain = 0.0, HissHp = 0.0, HissLp = 0.0;
	// Footsteps.
	double CrunchLeft = 0.0, CrunchGain = 0.0, CrunchLp = 0.0, CrunchGrain = 0.0;
	double SplashLeft = 0.0, SplashGain = 0.0, SplashHp = 0.0, SplashLp = 0.0;
};
