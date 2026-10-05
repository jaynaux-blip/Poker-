#pragma once

#include "CoreMinimal.h"

/**
 * Fifth Street, synthesized like the apartment's room tone and the card room's (no audio assets): out on the street the
 * rain hissing on the pavement and drumming on the awnings, the gutters running and drips off the fire escapes, wet
 * tyres hissing past on Market now and then, once in a long while a siren or a bus a few blocks off (echoing down the
 * street), and the transformer's buzz under a lit sign; inside the Lucky Penny the coolers' fans with the compressor
 * cycling under them, the ballasts' hum, the rain on the glass, and the street muffled through it. Across the street,
 * inside the Wash & Fold, the washers sloshing and churning (one winding up into a spin now and then), the dryers'
 * rumble with the clothes tumbling and the zips ticking on the drums, a machine's buzzer now and then, the TV's voice up
 * on the back wall; heard faintly through its glass from the sidewalk out front. Each room crossfades with the street as
 * the listener crosses its threshold, more of the street coming in while its door stands open. The doors' own sounds
 * ring in it too: the laundromat's bells, a lock that won't turn, a buzzer pressed, the steel back door. Mono, 48 kHz,
 * for UNightOneAudio::StartAmbience; the setters, the one-shots and Render are all called on the game thread.
 */
class FStreetAmbience
{
public:
	explicit FStreetAmbience(uint32 Seed);

	/** How far inside the store the listener is (AStreetStage::CameraInside, 0..1) and how far open its doors are (0..1). */
	void SetInside(float InInside, float InDoorsOpen);
	/**
	 * How far inside the Wash & Fold the listener is (AStreetStage::CameraInWash, 0..1), how far open its door is (0..1), and
	 * how near its front they stand out on the sidewalk (AStreetStage::WashNear, 0..1).
	 */
	void SetWash(float InInside, float InDoorOpen, float InNear);
	/** The Wash & Fold's bells on their strap: Hard 1 as the door swings open, less as its closer brings it shut. */
	void RingBell(float Hard);
	/** A key that won't turn: its pins in the new cylinder, the door shaken in its frame. */
	void LockRattle();
	/** A locked door's handle tried: the latch's play, the door knocking against its stop. */
	void TryHandle();
	/** An intercom's buzzer pressed and held (the tenement's, by its street door). */
	void PressBuzzer();
	/** The steel back door: the latch, the heavy door swinging, its closer pulling it to. */
	void SteelDoor();
	/** The rain's strength as it shows (AStreetStage::RainStrength) and how wet the street is (0 dry .. 1 puddles everywhere). */
	void SetRain(float InRain, float InWetness);
	/** 0 night .. 1 full day (Session::Daylight): the traffic is busier by day. */
	void SetDaylight(float InDaylight);
	/** How near the listener is to a buzzing sign (AStreetStage::SignHum, 0..1). */
	void SetHum(float InHum);
	/** While paused nothing new happens on the street (no car, no siren, the compressor holds); the rain goes on. */
	void SetPaused(bool bInPaused);
	/** Fades everything out over Seconds (the building's door closing behind the player on the way home). */
	void FadeOut(float Seconds);

	void Render(float* Out, int32 N);

private:
	/** An RBJ biquad (direct form I). */
	struct FBiquad
	{
		double B0 = 1.0, B1 = 0.0, B2 = 0.0, A1 = 0.0, A2 = 0.0;
		double X1 = 0.0, X2 = 0.0, Y1 = 0.0, Y2 = 0.0;
		void LowPass(double Hz, double Q);
		void HighPass(double Hz, double Q);
		void BandPass(double Hz, double Q);
		double Run(double X);

	private:
		void Set(double B0In, double B1In, double B2In, double A0, double A1In, double A2In);
	};

	/** A struck resonance (a drop on canvas, a tick on glass): a decaying sine from a two-pole recursion. */
	struct FRing
	{
		double C = 0.0, R2 = 0.0, Y1 = 0.0, Y2 = 0.0;
		void Strike(double Hz, double Seconds, double Amp);
		double Run();
	};

	/** One of the rare far sounds down the street. */
	enum class EFar : uint8
	{
		None,
		Siren,
		Bus,
	};

	/** A short bright sound waiting its turn (a bell's partial, a pin, a knock): a ring struck Delay after it was asked for. */
	struct FStrike
	{
		double At = 0.0;
		double Hz = 1000.0;
		double Seconds = 0.1;
		double Amp = 0.0;
	};

	/** A buzzer: Hz and its overtones through a saturated core, for Length seconds from At (its own clock: Time). */
	struct FBuzzer
	{
		double At = -100.0;
		double Length = 0.0;
		double Hz = 120.0;
		double Gain = 0.0;
		double Phase = 0.0;
		double Run(double Now);
	};

	double Noise();
	double Uniform() { return 0.5 + 0.5 * Noise(); }
	/** Seconds until the next of a scattering of events Rate times a second. */
	double Scatter(double Rate);
	/** The rain, the awnings, the drips and the gutters. */
	double RenderStreet();
	double RenderCar();
	/** The siren or the bus, when one's going. */
	double RenderFar();
	/** The bus, T seconds after it was first heard. */
	double RenderBus(double T);
	/** What comes back off the fronts down the block of the passing sounds In. */
	double EchoOf(double In);
	double RenderNeon();
	double RenderStore();
	/** The Wash & Fold's room: the washers, the dryers, the buzzer, the TV, the ballasts and the rain on its glass. */
	double RenderWash();
	/** Queues a strike Delay seconds from now. */
	void Strike(double Delay, double Hz, double Seconds, double Amp);
	/** The strikes that are due, and every ring still sounding. */
	double RenderStrikes();

	uint32 State = 1;
	/** Always runs (the beds, and an event's own envelope once it has begun). */
	double Time = 0.0;
	/** The street's own clock: holds while paused, and every event is scheduled on it. */
	double Clock = 0.0;
	bool bPaused = false;

	// The listener and the weather, as set (targets) and as eased per sample.
	double InsideTarget = 0.0;
	double DoorsTarget = 0.0;
	double Inside = 0.0;
	double Doors = 0.0;
	double RainTarget = 1.0;
	double Rain = 1.0;
	double Wet = 0.85;
	double Daylight = 0.0;
	double HumTarget = 0.0;
	double Hum = 0.0;
	double Fade = 0.0;
	double FadeTarget = 1.0;
	double FadeStep = 1.0 / (2.5 * 48000.0);

	// The rain: its hiss on the pavement, the droplets' crackle, the body of it across the block, and the gusts.
	FBiquad HissHp;
	FBiquad HissLp;
	double CrackLow = 0.0;
	double CrackEnv = 0.0;
	double CrackDecay = 0.0;
	double NextCrack = 0.0;
	double Brown = 0.0;
	double Body = 0.0;
	double Gust = 1.0;
	double GustTarget = 1.0;
	double NextGust = 4.0;
	// Drops on the awnings (canvas and sheet metal), drips off the edges, the gutters running.
	FRing Canvas;
	FRing Metal;
	double NextCanvas = 0.1;
	double NextMetal = 0.3;
	double PlinkAt = -10.0;
	double PlinkHz = 1800.0;
	double PlinkGain = 0.0;
	double PlinkPhase = 0.0;
	double NextPlink = 0.5;
	FBiquad Gutter;
	double GutterHz = 900.0;
	double GutterTarget = 900.0;
	double GutterAm = 0.0;
	double NextBubble = 0.0;
	int32 GutterTick = 0;

	// A car passing on Market: the spray off its tyres, the roar of the road, the engine under it (Doppler-shifted).
	bool bCar = false;
	double CarAt = 0.0;
	double CarLength = 6.0;
	double CarGain = 1.0;
	double CarEngine = 50.0;
	double CarPhase = 0.0;
	double NextCar = 9.0;
	FBiquad Spray;
	double CarBrown = 0.0;
	double CarRoar = 0.0;
	int32 CarTick = 0;

	// A siren or a bus a few blocks off, and the street's echo of the far sounds.
	EFar Far = EFar::None;
	double FarAt = 0.0;
	double FarLength = 20.0;
	double FarGain = 1.0;
	double FarPhase = 0.0;
	double FarLow = 0.0;
	double BrakeLow = 0.0;
	double KnockLow = 0.0;
	double NextFar = 70.0;
	TArray<float> Echo;
	int32 EchoAt = 0;
	double EchoLow = 0.0;

	// The transformer's buzz under a lit sign.
	double HumPhase = 0.0;
	FBiquad Sizzle;
	double Flutter = 1.0;
	double FlutterTarget = 1.0;
	double NextFlutter = 0.0;

	// The store: the coolers' fans, the compressor (on and off), the ballasts, drops on the glass.
	double FanLow1 = 0.0;
	double FanLow2 = 0.0;
	bool bCompressor = true;
	double CompLevel = 1.0;
	double CompSwitchAt = -100.0;
	double NextCompressor = 60.0;
	double CompPhase = 0.0;
	double CompLow = 0.0;
	double BallastPhase = 0.0;
	FRing Thunk;
	FRing Tap;
	double NextTap = 0.2;

	// The street as heard through the glass: two one-pole lowpasses whose corner falls as the listener goes in.
	double Muff1 = 0.0;
	double Muff2 = 0.0;
	double MuffK = 1.0;

	// The Wash & Fold: where the listener is (targets, and as eased per sample).
	double WashTarget = 0.0;
	double WashDoorTarget = 0.0;
	double WashNearTarget = 0.0;
	double WashIn = 0.0;
	double WashDoor = 0.0;
	double WashNear = 0.0;
	// The washers: the water thrown about in three drums turning back and forth, their motors, a spin now and then.
	FBiquad Slosh;
	double SloshPhase[3] = {0.0, 2.1, 4.4};
	double SloshRate[3] = {0.55, 0.47, 0.62};
	double MotorPhase = 0.0;
	double SpinAt = -100.0;
	double SpinLength = 40.0;
	double SpinPhase = 0.0;
	double NextSpin = 30.0;
	double SpinLow = 0.0;
	// The dryers: the rumble breathing with the drums, the clothes falling, zips and buttons ticking on the steel, the blowers.
	double RumbleBrown = 0.0;
	double Rumble1 = 0.0;
	double Rumble2 = 0.0;
	FRing Tumble;
	double NextTumble = 0.3;
	FRing Clink;
	double NextClink = 0.2;
	FBiquad Blower;
	// A machine's buzzer at the end of its cycle, now and then.
	FBuzzer MachineBuzzer;
	double NextMachineBuzz = 50.0;
	// The TV's voice through its little speaker: syllables in phrases, its pitch wandering, the vowels' formants.
	FBiquad Formant1;
	FBiquad Formant2;
	double VoicePhase = 0.0;
	double VoiceHz = 140.0;
	double Syllable = 0.0;
	double SyllableTarget = 0.0;
	double NextSyllable = 0.0;
	bool bTalking = true;
	double NextPhrase = 2.0;
	// The laundromat's own ballasts and the rain on its window.
	double WashBallastPhase = 0.0;
	FRing WashTap;
	double NextWashTap = 0.3;

	// The doors' sounds: strikes waiting their turn (in order), the rings they're struck on, the intercom's buzzer.
	TArray<FStrike> Pending;
	static constexpr int32 VoiceCount = 24;
	FRing Voices[VoiceCount];
	int32 NextVoice = 0;
	FBuzzer DoorBuzzer;
};
