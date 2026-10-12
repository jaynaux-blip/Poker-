#include "ShortStack/Audio/CardRoom.h"
#include "VoiceEngine.h"
#include "../StrictFloat.h"

#include <algorithm>
#include <cmath>
#include <cstring>

// The card room in stereo (next_segment_spec §4.4, §4.9, §10). audio-synth.
//
// The stream renders in 5 ms blocks. Each block first runs the control work: the mix eases, the ducks, the listener's
// geometry (only when it moved), and every event due on the room's clock (a chip, a card, a talker's next phrase).
// That is where every coefficient is worked out. Then the per-sample loops run on three buses: everything that lives
// under 4 kHz (the room's talkers and their murmur, the rail's vocals, the walla, the air, the slot floor and the reverb)
// at 8 kHz; clear lines and the PA at 16 kHz; chips, cards and claps at 48 kHz. The two slow buses are lifted to 48 kHz
// by polyphase filters (the 16 kHz one only while something is on it). The loops keep their filters' state in locals and
// write through no-alias pointers, so the recursions stay in registers.
//
// Space: direct sound falls 1/r from 1 m; the reverberant send is flat to 3 m (the critical distance: the room's
// reverberation equals the direct sound there) and falls as (3/r)^0.75 past it, as a big room full of bodies does.
// Air and bodies darken a source from 16 kHz at 1 m and 12 kHz at 3 m to 3.5 kHz at 20 m. Behind the head: -3 dB and a
// 6 kHz low-pass. Panning is constant power, 96% wide, no HRTF (the street's precedent).
//
// Audio runs in float, not the engine's doubles: it has no golden vectors to match, and the stream's budget is a
// fraction of a millisecond a block. It is still deterministic: the same calls give the same samples.

namespace ss
{
namespace audio
{
namespace room_audio
{
constexpr int Block = 240; // 5 ms at 48 kHz
constexpr int Ratio = 3;
constexpr int Bus = Block / Ratio; // 80 at 16 kHz: clear lines and the PA
constexpr int Low = Block / 6;     // 40 at 8 kHz: the murmur, the walla, the air, the slot floor, the reverb
constexpr float Rate48 = 48000.0f;
constexpr float Rate16 = 16000.0f;
constexpr float Rate8 = 8000.0f;
constexpr float BlockSeconds = static_cast<float>(Block) / Rate48;
constexpr int MaxTables = 24;
constexpr int MaxSpots = 4;
constexpr int MaxAmbient = 12;
constexpr int MaxLines = 4;
constexpr int MaxRail = 8;
constexpr int MaxHands = 32;
constexpr int MaxClaps = 10;
constexpr int MaxDucks = 8;
constexpr int VoicedTables = 4;
constexpr int FoleyPool = 6;

enum Layer : int
{
	LBed,
	LTalk,
	LChips,
	LCalls,
	LRail,
	LPa,
	LCount,
};

float Clamp(float X, float Lo, float Hi)
{
	return X < Lo ? Lo : X > Hi ? Hi : X;
}

// ------------------------------------------------------------------ small parts

struct SineTable
{
	float T[1025];
	SineTable()
	{
		for (int I = 0; I <= 1024; ++I)
		{
			T[I] = std::sin(vox::TwoPi * static_cast<float>(I) / 1024.0f);
		}
	}
};

const SineTable& Sines()
{
	static const SineTable S;
	return S;
}

/** A struck resonance: chips, glass, a knuckle on felt. Coefficients set per strike. */
struct Ring
{
	float B = 0.0f;
	float C = 0.0f;
	float Y1 = 0.0f;
	float Y2 = 0.0f;
	float In = 0.0f;
	void Strike(float Hz, float Decay, float Amp, float Rate)
	{
		const float R = std::exp(-1.0f / (std::max(0.0002f, Decay) * Rate));
		const float W = vox::TwoPi * std::min(Hz, Rate * 0.45f) / Rate;
		B = 2.0f * R * std::cos(W);
		C = -R * R;
		In += Amp * std::sin(W);
	}
	void Clear() { Y1 = Y2 = In = 0.0f; }
};

/** A positioned source's geometry and gains, aimed once per block when the listener moves. */
struct Place
{
	float X = 0.0f, Y = 0.0f;
	float Dist = 1.0f;
	float GL = 0.0f, GR = 0.0f, GS = 0.0f;
	float TL = 0.0f, TR = 0.0f, TS = 0.0f;
	float K48 = 1.0f, K16 = 1.0f, K8 = 1.0f;
	float Lp = 0.0f;
	bool Fresh = true;
	void At(float InX, float InY)
	{
		X = InX;
		Y = InY;
		Fresh = true;
	}
};

struct Listener
{
	float X = 0.0f, Y = 0.0f, Cos = 1.0f, Sin = 0.0f;
};

/** Distance, direction and the reverb's share for one place (spec §4.4). */
void Aim(Place& P, const Listener& L)
{
	const float Dx = (P.X - L.X) * 0.01f;
	const float Dy = (P.Y - L.Y) * 0.01f;
	const float R = std::sqrt(Dx * Dx + Dy * Dy);
	const float Ahead = Dx * L.Cos + Dy * L.Sin;
	const float Right = -Dx * L.Sin + Dy * L.Cos;
	const float Norm = std::max(R, 0.3f);
	const float Lateral = R < 0.05f ? 0.0f : Clamp(Right / Norm, -1.0f, 1.0f);
	const float Behind = R < 0.05f ? 0.0f : Clamp(-Ahead / Norm, 0.0f, 1.0f);
	const float Direct = 1.0f / std::max(R, 1.0f) * vox::DbToGain(-3.0f * Behind);
	P.Dist = R;
	P.TL = Direct * std::sqrt(0.5f * (1.0f - 0.96f * Lateral));
	P.TR = Direct * std::sqrt(0.5f * (1.0f + 0.96f * Lateral));
	// Per channel, the reverb carries half of 1/9 of the power at 3 m and falls past it.
	P.TS = 0.2357f * (R <= 3.0f ? 1.0f : std::pow(3.0f / R, 0.75f));
	float Air = 16000.0f;
	if (R > 20.0f)
	{
		Air = 3500.0f * std::pow(2500.0f / 3500.0f, std::min(1.0f, (R - 20.0f) / 20.0f));
	}
	else if (R > 3.0f)
	{
		Air = 12000.0f * std::pow(3500.0f / 12000.0f, (R - 3.0f) / 17.0f);
	}
	else if (R > 1.0f)
	{
		Air = 16000.0f * std::pow(12000.0f / 16000.0f, (R - 1.0f) / 2.0f);
	}
	const float Shadow = 20000.0f * std::pow(6000.0f / 20000.0f, Behind);
	const float Cut = std::min(Air, Shadow);
	P.K48 = Cut >= 19000.0f ? 1.0f : vox::OnePoleK(Cut, Rate48);
	P.K16 = Cut >= 7500.0f ? 1.0f : vox::OnePoleK(Cut, Rate16);
	P.K8 = Cut >= 3800.0f ? 1.0f : vox::OnePoleK(Cut, Rate8);
	if (P.Fresh)
	{
		P.GL = P.TL;
		P.GR = P.TR;
		P.GS = P.TS;
		P.Fresh = false;
	}
}

/**
 * Mono In through a place into a stereo bus and the reverb's send; K is the place's air low-pass at the bus's rate.
 * G0..G1 is the layer's gain across the block. Returns the power it added, as heard (for the meter).
 */
float Spatial(Place& P, const float* SS_VOX_RESTRICT In, int N, float K, float G0, float G1, float* SS_VOX_RESTRICT L, float* SS_VOX_RESTRICT R, float* SS_VOX_RESTRICT S)
{
	const float Inv = 1.0f / static_cast<float>(N);
	const float L0 = P.GL * G0, L1 = P.TL * G1;
	const float R0 = P.GR * G0, R1 = P.TR * G1;
	const float S0 = P.GS * G0, S1 = P.TS * G1;
	const float DL = (L1 - L0) * Inv, DR = (R1 - R0) * Inv, DS = (S1 - S0) * Inv;
	float Gl = L0, Gr = R0, Gs = S0;
	float Lp = P.Lp;
	float Power = 0.0f;
	if (K >= 1.0f)
	{
		for (int I = 0; I < N; ++I)
		{
			const float X = In[I];
			L[I] += X * Gl;
			R[I] += X * Gr;
			S[I] += X * Gs;
			Power += X * X;
			Gl += DL;
			Gr += DR;
			Gs += DS;
		}
		Lp = In[N - 1];
	}
	else
	{
		for (int I = 0; I < N; ++I)
		{
			Lp += K * (In[I] - Lp);
			L[I] += Lp * Gl;
			R[I] += Lp * Gr;
			S[I] += Lp * Gs;
			Power += Lp * Lp;
			Gl += DL;
			Gr += DR;
			Gs += DS;
		}
	}
	P.Lp = Lp + 1.0e-18f;
	P.GL = P.TL;
	P.GR = P.TR;
	P.GS = P.TS;
	const float Ml = 0.5f * (L0 + L1), Mr = 0.5f * (R0 + R1);
	return Power * 0.5f * (Ml * Ml + Mr * Mr);
}

// ------------------------------------------------------------------ Foley: chips, cards, glass

enum class Fx : uint8_t
{
	Click,      // a chip on a chip
	Clicks,     // a bet set down, chips stacked (N clicks)
	ChipRiffle, // a player's fingers working a stack (N clicks, two pitches)
	Card,       // a card pitched (N cards)
	Shuffle,    // a riffle shuffle and its bridge
	Tap,        // the deck squared or cut
	Knock,      // a check (two knocks)
	Slide,      // chips pushed across the felt
	Scrape,     // a chair
	Glass,      // the bar: a glass set down
	Bottle,     // the bar: a bottle on the wood
	Rack,       // the cage: a rack of chips set down
	Printer,    // the desk: a receipt printing
};

/** A queued strike inside the current block. */
struct Pending
{
	int At = 0;
	Fx Kind = Fx::Click;
	float Gain = 0.0f;
	float P1 = 0.0f;
};

/** The noise voice: a band of noise with an envelope, and a tick train for the shuffle. */
struct Hiss
{
	vox::Biquad Band;
	float Env = 0.0f;
	float Rise = 0.0f;
	float Fall = 0.999f;
	int Attack = 0;
	int Hold = 0;
	int Phase = 0; // 0 idle, 1 attack, 2 hold, 3 decay
	float TickPeriod = 0.0f;
	float TickPos = 0.0f;
	float TickEnv = 0.0f;
	float TickDecay = 0.9f;
	// The shuffle's bridge, queued behind its riffle.
	bool HasNext = false;
	float NextHz = 0.0f, NextGain = 0.0f, NextHold = 0.0f, NextTick = 0.0f;
	void Start(float Hz, float Q, float Gain, float AttackS, float HoldS, float DecayS, float TickS, float Rate)
	{
		Band.BandPass(Hz, Q, Rate);
		Attack = std::max(1, static_cast<int>(AttackS * Rate));
		Rise = (Gain - Env) / static_cast<float>(Attack);
		Hold = std::max(1, static_cast<int>(HoldS * Rate));
		Fall = std::exp(-1.0f / (std::max(0.002f, DecayS) * Rate));
		Phase = 1;
		TickPeriod = TickS * Rate;
		TickPos = 0.0f;
		TickEnv = 1.0f;
		TickDecay = TickS > 0.0f ? std::exp(-1.0f / (0.0007f * Rate)) : 1.0f;
	}
};

/** A Foley voice: two struck rings, the edge's tick, a noise voice; strikes queued at offsets inside the block. */
struct Foley
{
	Ring Hi, Lo;
	float Spike = 0.0f;
	Hiss Noise;
	int Busy = 0; // samples of sound left
	Pending Queue[8];
	int Queued = 0;
	int Owner = -1; // the table it plays for (pool voices)
	vox::Rng R;

	void Add(int At, Fx Kind, float Gain, float P1)
	{
		if (Queued < 8)
		{
			Queue[Queued++] = Pending{At, Kind, Gain, P1};
		}
	}
	void Silence()
	{
		Hi.Clear();
		Lo.Clear();
		Spike = 0.0f;
		Noise.Phase = 0;
		Noise.Env = 0.0f;
		Noise.HasNext = false;
		Noise.Band.Clear();
		Busy = 0;
		Queued = 0;
	}
	void Fire(const Pending& E, float Rate)
	{
		const float G = E.Gain;
		switch (E.Kind)
		{
		case Fx::Click:
		case Fx::Clicks:
		case Fx::ChipRiffle:
		{
			// Clay on clay: a bright ring and a body under it, with the edge's tick.
			const float Pitch = E.P1 > 0.0f ? E.P1 : R.Range(3000.0f, 4800.0f);
			Hi.Strike(Pitch, R.Range(0.003f, 0.006f), 0.55f * G, Rate);
			Lo.Strike(Pitch * R.Range(0.32f, 0.42f), 0.0022f, 0.3f * G, Rate);
			Spike += 0.35f * G;
			Busy = std::max(Busy, static_cast<int>(0.04f * Rate));
			break;
		}
		case Fx::Card:
			// The pitch's swish, and the card landing.
			Noise.Start(R.Range(2800.0f, 4200.0f), 0.9f, 0.32f * G, 0.003f, 0.012f, 0.012f, 0.0f, Rate);
			Lo.Strike(R.Range(800.0f, 1100.0f), 0.002f, 0.12f * G, Rate);
			Busy = std::max(Busy, static_cast<int>(0.08f * Rate));
			break;
		case Fx::Shuffle:
			Noise.Start(R.Range(3200.0f, 4200.0f), 0.7f, 0.5f * G, 0.04f, R.Range(0.55f, 0.75f), 0.05f, R.Range(0.0028f, 0.0036f), Rate);
			Noise.HasNext = true;
			Noise.NextHz = 2600.0f;
			Noise.NextGain = 0.22f * G;
			Noise.NextHold = R.Range(0.22f, 0.32f);
			Noise.NextTick = 0.0065f;
			Busy = std::max(Busy, static_cast<int>(1.4f * Rate));
			break;
		case Fx::Tap:
			Lo.Strike(R.Range(330.0f, 480.0f), 0.009f, 0.35f * G, Rate);
			Spike += 0.08f * G;
			Busy = std::max(Busy, static_cast<int>(0.06f * Rate));
			break;
		case Fx::Knock:
			Lo.Strike(R.Range(170.0f, 240.0f), 0.016f, 0.55f * G, Rate);
			Hi.Strike(R.Range(900.0f, 1300.0f), 0.002f, 0.15f * G, Rate);
			Spike += 0.1f * G;
			Busy = std::max(Busy, static_cast<int>(0.1f * Rate));
			break;
		case Fx::Slide:
			Noise.Start(R.Range(900.0f, 1500.0f), 0.55f, 0.22f * G, 0.04f, std::max(0.1f, E.P1), 0.08f, 0.0f, Rate);
			Busy = std::max(Busy, static_cast<int>((E.P1 + 0.5f) * Rate));
			break;
		case Fx::Scrape:
			Noise.Start(R.Range(480.0f, 640.0f), 4.0f, 0.5f * G, 0.03f, R.Range(0.22f, 0.38f), 0.06f, 0.0f, Rate);
			Busy = std::max(Busy, static_cast<int>(0.7f * Rate));
			break;
		case Fx::Glass:
		{
			const float F = R.Range(2600.0f, 3900.0f);
			Hi.Strike(F, R.Range(0.06f, 0.16f), 0.18f * G, Rate);
			Lo.Strike(F * R.Range(0.36f, 0.42f), 0.04f, 0.1f * G, Rate);
			Spike += 0.05f * G;
			Busy = std::max(Busy, static_cast<int>(0.9f * Rate));
			break;
		}
		case Fx::Bottle:
			Lo.Strike(R.Range(150.0f, 210.0f), 0.03f, 0.5f * G, Rate);
			Hi.Strike(R.Range(1800.0f, 2400.0f), 0.02f, 0.08f * G, Rate);
			Busy = std::max(Busy, static_cast<int>(0.25f * Rate));
			break;
		case Fx::Rack:
			Lo.Strike(R.Range(260.0f, 340.0f), 0.02f, 0.45f * G, Rate);
			Noise.Start(1400.0f, 0.6f, 0.1f * G, 0.005f, 0.12f, 0.05f, 0.0f, Rate);
			Busy = std::max(Busy, static_cast<int>(0.35f * Rate));
			break;
		case Fx::Printer:
			Noise.Start(R.Range(1800.0f, 2600.0f), 1.6f, 0.2f * G, 0.01f, R.Range(0.5f, 0.9f), 0.03f, 1.0f / R.Range(28.0f, 36.0f), Rate);
			Busy = std::max(Busy, static_cast<int>(1.2f * Rate));
			break;
		}
	}
	/** The rings and the tick from A to B, state in locals. */
	void RunRings(float* SS_VOX_RESTRICT Out, int A, int B)
	{
		float H1 = Hi.Y1, H2 = Hi.Y2, Hb = Hi.B, Hc = Hi.C;
		float L1 = Lo.Y1, L2 = Lo.Y2, Lb = Lo.B, Lc = Lo.C;
		float Sp = Spike;
		if (A < B)
		{
			// A strike's input lands on the first sample.
			const float H = Hb * H1 + Hc * H2 + Hi.In;
			const float L = Lb * L1 + Lc * L2 + Lo.In;
			H2 = H1;
			H1 = H;
			L2 = L1;
			L1 = L;
			Out[A] = H + L + Sp;
			Sp *= -0.45f;
			Hi.In = Lo.In = 0.0f;
			for (int I = A + 1; I < B; ++I)
			{
				const float Hn = Hb * H1 + Hc * H2;
				const float Ln = Lb * L1 + Lc * L2;
				H2 = H1;
				H1 = Hn;
				L2 = L1;
				L1 = Ln;
				Out[I] = Hn + Ln + Sp;
				Sp *= -0.45f;
			}
		}
		Hi.Y1 = H1;
		Hi.Y2 = H2;
		Lo.Y1 = L1;
		Lo.Y2 = L2;
		Spike = Sp;
	}
	/** The noise voice from A to B, added. */
	void RunNoise(float* SS_VOX_RESTRICT Out, int A, int B, float Rate)
	{
		int I = A;
		while (I < B && Noise.Phase != 0)
		{
			vox::Biquad Band = Noise.Band;
			float Env = Noise.Env;
			uint32_t Ns = R.S;
			const bool Ticks = Noise.TickPeriod > 0.0f;
			int Phase = Noise.Phase;
			for (; I < B; ++I)
			{
				if (Phase == 1)
				{
					Env += Noise.Rise;
					if (--Noise.Attack <= 0)
					{
						Phase = 2;
					}
				}
				else if (Phase == 2)
				{
					if (--Noise.Hold <= 0)
					{
						Phase = 3;
					}
				}
				else
				{
					Env *= Noise.Fall;
					if (Env < 1.0e-4f)
					{
						Phase = 0;
						Env = 0.0f;
						++I;
						break;
					}
				}
				Ns ^= Ns << 13;
				Ns ^= Ns >> 17;
				Ns ^= Ns << 5;
				float V = Band.Run(static_cast<float>(Ns >> 8) * (2.0f / 16777216.0f) - 1.0f) * Env;
				if (Ticks)
				{
					Noise.TickPos += 1.0f;
					if (Noise.TickPos >= Noise.TickPeriod)
					{
						R.S = Ns;
						Noise.TickPos -= Noise.TickPeriod * R.Range(0.75f, 1.25f);
						Ns = R.S;
						Noise.TickEnv = 1.0f;
					}
					V *= Noise.TickEnv * 2.2f;
					Noise.TickEnv *= Noise.TickDecay;
				}
				Out[I] += V;
			}
			R.S = Ns;
			Noise.Band.Z1 = Band.Z1;
			Noise.Band.Z2 = Band.Z2;
			Noise.Env = Env;
			Noise.Phase = Phase;
			if (Phase == 0 && Noise.HasNext)
			{
				Noise.HasNext = false;
				Noise.Start(Noise.NextHz, 0.8f, Noise.NextGain, 0.01f, Noise.NextHold, 0.06f, Noise.NextTick, Rate);
			}
		}
	}
	/** Renders N samples (writes Out). Returns false, and leaves Out alone, when there's nothing to hear. */
	bool Render(float* SS_VOX_RESTRICT Out, int N, float Rate)
	{
		if (Busy <= 0 && Queued == 0)
		{
			return false;
		}
		// The queue is in time order: the block's events are added as the clock reaches them.
		int From = 0;
		for (int Q = 0; Q <= Queued; ++Q)
		{
			const int To = Q < Queued ? std::min(N, std::max(From, Queue[Q].At)) : N;
			RunRings(Out, From, To);
			RunNoise(Out, From, To, Rate);
			From = To;
			if (Q < Queued)
			{
				Fire(Queue[Q], Rate);
			}
		}
		Queued = 0;
		Busy -= N;
		if (Busy <= 0 && Noise.Phase == 0)
		{
			Busy = 0;
			Hi.Clear();
			Lo.Clear();
			Spike = 0.0f;
		}
		return true;
	}
};

/** A compound event playing out: N clicks or cards at a gap. */
struct Seq
{
	Fx Kind = Fx::Click;
	int Left = 0;
	double Next = 0.0;
	float Gap = 0.05f;
	float Gain = 1.0f;
	float PitchA = 3600.0f, PitchB = 4200.0f;
	int Count = 0;
};

/** One step of a hand's plan, from the hand's start on the room's clock. */
struct Step
{
	float At = 0.0f;
	Fx Kind = Fx::Click;
	uint8_t N = 1;
	uint8_t Gain = 200;
	uint8_t Gap = 10; // 5 ms units
};

struct TableState
{
	int Id = 0;
	bool Exists = false;
	bool Open = true;
	bool Feature = false;
	bool Voiced = false;
	int Players = 6;
	int Voice = -1; // its Foley voice from the pool, while it's voiced
	Place Where;
	Step Steps[24];
	int StepCount = 0;
	int StepAt = 0;
	double HandStart = 0.0;
	Seq Runs[2];
	double IdleNext = 0.0;
	uint32_t Hands = 0;
	vox::Rng R;
};

/** The bar, the cage, the desk: a Foley voice with its own habits. */
struct SpotState
{
	CardRoom::Spot Kind = CardRoom::Spot::Bar;
	int Id = 0;
	Place Where;
	Foley Sound;
	double Next = 0.0;
	vox::Rng R;
};

/** Far tables' chips: a diffuse scatter of clicks across the stereo field. */
struct FarChips
{
	Ring L, R;
	float LpL = 0.0f, LpR = 0.0f;
	double Next = 0.0;
	Seq Run;
	float RunPan = 0.0f;
	Pending Queue[8];
	float QueuePan[8];
	int Queued = 0;
	int Requests = 0;
	int Ringing = 0;
	vox::Rng Rng;
};

// ------------------------------------------------------------------ voices in the room

struct Ambient
{
	vox::Talker Voice;
	Syllable Phrase[16];
	int Count = 0;
	Place Where;
	Timbre Who;
	uint32_t Person = 0;
	int Table = -1;
	bool Speaking = false;
	bool Known = false;
	double NextAt = 0.0;
	float Gain = 1.0f;
	int Layer = LTalk;
};

struct LineSlot
{
	vox::Talker Voice;
	std::vector<Syllable> Plan;
	Place Where;
	bool Pa = false;
	bool Low = false; // murmured: the 8 kHz bus
	bool Near = false; // said at your table: it ducks the room, never itself (the layer's level without the ducks)
	bool Active = false;
	int Layer = LCalls;
	float Gain = 1.0f;
	uint32_t Order = 0;
};

struct RailSlot
{
	vox::Talker Voice;
	Syllable Plan[8];
	int Count = 0;
	Place Where;
	bool Active = false;
	int Delay = 0; // samples at 8 kHz before it starts
	float Gain = 1.0f;
	int Layer = LRail;
	uint32_t Order = 0;
};

struct HandSpot
{
	float GL = 0.0f, GR = 0.0f, GS = 0.0f;
	int Layer = LRail;
};

struct WallaVoice
{
	float Level = 0.0f;
	float Target = 0.0f;
	double Next = 0.0;
	double PhraseUntil = 0.0;
	bool On = false;
};

struct WallaSide
{
	vox::Biquad Shape;
	float Lp = 0.0f;
	float EnvA = 0.0f, EnvB = 0.0f;
	WallaVoice A[4];
	WallaVoice B[10];
	vox::Rng Noise;
};

struct SlotFloor
{
	Place Where;
	bool Exists = false;
	float Phase[4] = {0, 0, 0, 0};
	double JingleAt = -100.0;
	double NextJingle = 6.0;
	float JingleRoot = 880.0f;
	float JinglePhase = 0.0f;
	double NextCoin = 3.0;
	Ring Coin;
	vox::Rng R;
};

struct Wind
{
	Place Where;
	bool Exists = false;
	bool Open = false;
	float Lp1 = 0.0f, Lp2 = 0.0f;
	float Gust = 0.3f, GustTarget = 0.3f;
	double NextGust = 0.0;
	vox::Rng R;
};

struct DuckState
{
	bool Live = false;
	double Start = 0.0;
	float Attack = 0.06f, Hold = 0.0f, Release = 0.5f;
	float Depth[LCount] = {0, 0, 0, 0, 0, 0};
};

inline float TwoPiOver(float Rate)
{
	return vox::TwoPi / Rate;
}

/** The room's reverb: an 8-line feedback delay network at 8 kHz, Householder mixing, damped, RT60 0.7 s. */
struct Fdn
{
	std::vector<float> Buf;
	int Off[8] = {0}, Len[8] = {0}, Pos[8] = {0};
	float G[8] = {0};
	float Damp[8] = {0};
	std::vector<float> Ap;
	int ApLen[2] = {0, 0}, ApPos[2] = {0, 0};
	float WetGain = 0.2f;
	float Keep = 0.15f; // the damping: high frequencies die first (a 2.5 kHz pole each pass)
	void Init(float Rt60, float Rate)
	{
		// 15 to 27 ms (this room's mean free path is about 18 ms), at 8 kHz.
		static const int Lengths[8] = {121, 131, 149, 157, 173, 187, 199, 217};
		static const int Diffuse[2] = {59, 131};
		int Total = 0;
		for (int I = 0; I < 8; ++I)
		{
			Off[I] = Total;
			Len[I] = Lengths[I];
			Pos[I] = 0;
			Total += Lengths[I];
			G[I] = std::pow(10.0f, -3.0f * static_cast<float>(Lengths[I]) / (Rate * Rt60));
			Damp[I] = 0.0f;
		}
		Keep = std::exp(-TwoPiOver(Rate) * 2500.0f);
		Buf.assign(static_cast<size_t>(Total), 0.0f);
		ApLen[0] = Diffuse[0];
		ApLen[1] = Diffuse[1];
		Ap.assign(static_cast<size_t>(Diffuse[0] + Diffuse[1]), 0.0f);
		ApPos[0] = ApPos[1] = 0;
	}
	/** Adds the wet sound for In to L and R. */
	void Process(const float* SS_VOX_RESTRICT In, float* SS_VOX_RESTRICT L, float* SS_VOX_RESTRICT R, int N)
	{
		float* SS_VOX_RESTRICT B = Buf.data();
		float* SS_VOX_RESTRICT A0 = Ap.data();
		float* SS_VOX_RESTRICT A1 = A0 + ApLen[0];
		const float Hold = Keep;
		float D[8], Gs[8];
		int P[8], Ln[8];
		float* Lines[8];
		for (int K = 0; K < 8; ++K)
		{
			D[K] = Damp[K];
			Gs[K] = G[K];
			P[K] = Pos[K];
			Ln[K] = Len[K];
			Lines[K] = B + Off[K];
		}
		int P0 = ApPos[0], P1 = ApPos[1];
		const int N0 = ApLen[0], N1 = ApLen[1];
		const float Wet = WetGain;
		for (int I = 0; I < N; ++I)
		{
			// Two allpasses smear the input first.
			float X = In[I];
			{
				const float Dl = A0[P0];
				const float V = X + 0.6f * Dl;
				A0[P0] = V;
				X = Dl - 0.6f * V;
				P0 = P0 + 1 < N0 ? P0 + 1 : 0;
			}
			{
				const float Dl = A1[P1];
				const float V = X + 0.6f * Dl;
				A1[P1] = V;
				X = Dl - 0.6f * V;
				P1 = P1 + 1 < N1 ? P1 + 1 : 0;
			}
			float V[8];
			float Sum = 0.0f;
			for (int K = 0; K < 8; ++K)
			{
				const float Raw = Lines[K][P[K]];
				D[K] = Raw + (D[K] - Raw) * Hold;
				V[K] = D[K] * Gs[K];
				Sum += V[K];
			}
			const float Mixed = Sum * 0.25f;
			const float Feed = X * 0.35355f;
			for (int K = 0; K < 8; ++K)
			{
				Lines[K][P[K]] = V[K] - Mixed + ((K & 1) ? -Feed : Feed);
				P[K] = P[K] + 1 < Ln[K] ? P[K] + 1 : 0;
			}
			L[I] += (V[0] + V[1] + V[2] + V[3] - V[4] - V[5] - V[6] - V[7]) * Wet;
			R[I] += (V[0] - V[1] + V[2] - V[3] + V[4] - V[5] + V[6] - V[7]) * Wet;
		}
		for (int K = 0; K < 8; ++K)
		{
			Damp[K] = D[K] + 1.0e-18f;
			Pos[K] = P[K];
		}
		ApPos[0] = P0;
		ApPos[1] = P1;
	}
};

/** 16 kHz to 48 kHz: a 24-tap windowed sinc in three phases. */
struct Up3
{
	float H[3][8];
	float Hist[2][7 + Bus];
	Up3()
	{
		float Proto[24];
		float Sum = 0.0f;
		const float Wc = vox::TwoPi * 7200.0f / Rate48;
		for (int M = 0; M < 24; ++M)
		{
			const float X = static_cast<float>(M) - 11.5f;
			const float Sinc = std::sin(Wc * X) / (vox::Pi * X);
			const float W = 0.42f - 0.5f * std::cos(vox::TwoPi * (static_cast<float>(M) + 0.5f) / 24.0f) + 0.08f * std::cos(2.0f * vox::TwoPi * (static_cast<float>(M) + 0.5f) / 24.0f);
			Proto[M] = Sinc * W;
			Sum += Proto[M];
		}
		for (int P = 0; P < 3; ++P)
		{
			for (int K = 0; K < 8; ++K)
			{
				H[P][K] = Proto[P + 3 * K] / Sum * 3.0f;
			}
		}
		std::memset(Hist, 0, sizeof(Hist));
	}
	/** Adds the 48 kHz version of In (Bus samples at 16 kHz) to Out. */
	void Process(int Ch, const float* In, float* SS_VOX_RESTRICT Out)
	{
		// The block after the last 7 samples of the one before, so every output reads straight back along the array.
		float* X = Hist[Ch];
		std::memcpy(X + 7, In, sizeof(float) * Bus);
		float Hl[3][8];
		std::memcpy(Hl, H, sizeof(Hl));
		for (int I = 0; I < Bus; ++I)
		{
			const float* At = X + I + 7; // At[-K] is the input K samples back
			for (int P = 0; P < 3; ++P)
			{
				const float* Hp = Hl[P];
				Out[3 * I + P] += Hp[0] * At[0] + Hp[1] * At[-1] + Hp[2] * At[-2] + Hp[3] * At[-3] + Hp[4] * At[-4] + Hp[5] * At[-5] + Hp[6] * At[-6] + Hp[7] * At[-7];
			}
		}
		std::memmove(X, X + Bus, sizeof(float) * 7);
	}
};
/** 8 kHz to 48 kHz: a 48-tap windowed sinc in six phases (the bus holds nothing over about 3.5 kHz). */
struct Up6
{
	float H[6][8];
	float Hist[2][7 + Low];
	Up6()
	{
		float Proto[48];
		float Sum = 0.0f;
		const float Wc = vox::TwoPi * 3500.0f / Rate48;
		for (int M = 0; M < 48; ++M)
		{
			const float X = static_cast<float>(M) - 23.5f;
			const float Sinc = std::sin(Wc * X) / (vox::Pi * X);
			const float W = 0.42f - 0.5f * std::cos(vox::TwoPi * (static_cast<float>(M) + 0.5f) / 48.0f) + 0.08f * std::cos(2.0f * vox::TwoPi * (static_cast<float>(M) + 0.5f) / 48.0f);
			Proto[M] = Sinc * W;
			Sum += Proto[M];
		}
		for (int P = 0; P < 6; ++P)
		{
			for (int K = 0; K < 8; ++K)
			{
				H[P][K] = Proto[P + 6 * K] / Sum * 6.0f;
			}
		}
		std::memset(Hist, 0, sizeof(Hist));
	}
	/** Adds the 48 kHz version of In (Low samples at 8 kHz) to Out. */
	void Process(int Ch, const float* In, float* SS_VOX_RESTRICT Out)
	{
		float* X = Hist[Ch];
		std::memcpy(X + 7, In, sizeof(float) * Low);
		float Hl[6][8];
		std::memcpy(Hl, H, sizeof(Hl));
		for (int I = 0; I < Low; ++I)
		{
			const float* At = X + I + 7;
			for (int P = 0; P < 6; ++P)
			{
				const float* Hp = Hl[P];
				Out[6 * I + P] += Hp[0] * At[0] + Hp[1] * At[-1] + Hp[2] * At[-2] + Hp[3] * At[-3] + Hp[4] * At[-4] + Hp[5] * At[-5] + Hp[6] * At[-6] + Hp[7] * At[-7];
			}
		}
		std::memmove(X, X + Low, sizeof(float) * 7);
	}
};
} // namespace room_audio

// ------------------------------------------------------------------ the room

struct CardRoom::Impl
{
	using Place = room_audio::Place;
	uint32_t Seed = 1u;
	vox::Rng TalkRng, BedRng, RailRng, EventRng;
	uint32_t Events = 0; // the room's own talkers' phrases
	uint32_t Said = 0;   // lines (Say), counted apart: a line never changes the room's other voices
	uint32_t Shouts = 0; // the crowd's (Crowd)

	// Geometry.
	std::vector<Emitter> Emitters;
	room_audio::Listener Ear;
	bool Moved = true;
	room_audio::TableState Tables[room_audio::MaxTables];
	int TableCount = 0;
	room_audio::Foley Voices[room_audio::FoleyPool];
	room_audio::SpotState Spots[room_audio::MaxSpots];
	int SpotCount = 0;
	room_audio::SlotFloor Slots;
	room_audio::Wind Terrace;
	int VoicedCount = 0;
	float FarShare = 0.0f;

	// The mix.
	float Db[room_audio::LCount] = {0, 0, 0, 0, -60, 0};
	float Target[room_audio::LCount] = {0, 0, 0, 0, -60, 0};
	float Speed[room_audio::LCount] = {0, 0, 0, 0, 0, 0};
	float Gain[room_audio::LCount] = {1, 1, 1, 1, 0.001f, 1};
	float NextGain[room_audio::LCount] = {1, 1, 1, 1, 0.001f, 1};
	float RawGain[room_audio::LCount] = {1, 1, 1, 1, 0.001f, 1};     // the same, without the ducks (lines at your table)
	float RawNextGain[room_audio::LCount] = {1, 1, 1, 1, 0.001f, 1};
	float CrowdNow = 1.0f, CrowdTarget = 1.0f, CrowdSpeed = 0.0f;
	room_audio::DuckState Ducks[room_audio::MaxDucks];
	float MuffleHz = 20000.0f, MuffleTargetHz = 20000.0f, MuffleDb = 0.0f, MuffleTargetDb = 0.0f;
	float MuffleGain = 1.0f;
	bool MuffleIdle = true;
	vox::Biquad MuffleL, MuffleR;
	float AppliedHz = -1.0f;
	float Pace = 1.0f;
	int Budget = 0; // the fallback by default (next_segment_spec 11, the listening gate): talkers only when a host asks

	// Clocks: output samples (real time) and the room's event clock (dilated by Pace).
	long long Frame = 0;
	double Now = 0.0;
	double EventNow = 0.0;

	// Voices.
	std::vector<room_audio::Ambient> Talk;
	uint32_t People = 0;
	room_audio::LineSlot Lines[room_audio::MaxLines];
	room_audio::RailSlot Rail[room_audio::MaxRail];
	uint32_t Order = 0;
	vox::PaStage Pa;
	float PaDelay[128];
	int PaPos = 0;
	float PaTail = 0.0f;
	float PaGain = 1.0f, PaGainTarget = 1.0f; // SetPaLevel: the PA as heard (the host's Voices setting), ramped over a block
	vox::Clapper Hands[room_audio::MaxHands];
	room_audio::HandSpot HandAt[room_audio::MaxHands];
	int HandCount = 0;
	vox::ClapVoice Claps[room_audio::MaxClaps];
	room_audio::HandSpot ClapAt[room_audio::MaxClaps];
	int ClapStart[room_audio::MaxClaps];
	room_audio::FarChips Far;
	room_audio::WallaSide Walla[2];
	float Brown[2] = {0, 0}, Rumble[2] = {0, 0}, HissLo[2] = {0, 0}, HissHi[2] = {0, 0};
	vox::Rng AirNoise[2];
	float AirSwell = 1.0f;

	// Buffers for one block.
	float BusL[room_audio::Bus], BusR[room_audio::Bus], Send16[room_audio::Bus], Mono16[room_audio::Bus], PaBus[room_audio::Bus];
	float MixL[room_audio::Block], MixR[room_audio::Block], Send48[room_audio::Block], Mono48[room_audio::Block];
	float LowL[room_audio::Low], LowR[room_audio::Low], LowSend[room_audio::Low], Mono8[room_audio::Low];
	int Bus16Hot = 0; // blocks the 16 kHz bus still has to be lifted (a tail of zeros clears its filter)
	room_audio::Fdn Reverb;
	room_audio::Up3 Up;
	room_audio::Up6 UpLow;
	float Fifo[room_audio::Block * 2];
	int FifoAt = room_audio::Block;

	// The meter.
	float Power[room_audio::LCount] = {0, 0, 0, 0, 0, 0};
	float Smoothed[room_audio::LCount] = {0, 0, 0, 0, 0, 0};
	float OutPower = 0.0f;
	int Speaking = 0;

	explicit Impl(uint32_t InSeed);
	void Resize();
	void Rebuild();
	void AimAll();
	void Control();
	void TableEvents(room_audio::TableState& T, double From, double To);
	void BuildHand(room_audio::TableState& T, float Seconds, bool AllIn, bool Called, float PotBb);
	void FireStep(room_audio::TableState& T, const room_audio::Step& S, double At);
	void PlayFx(room_audio::TableState& T, room_audio::Fx Kind, float Level, float P1, double At);
	void FarEvent(float Level, int Clicks, float Gap);
	void Ambience(double To);
	void Recast(int I, float RailShare);
	void StartPhrase(int I);
	void SpotEvents(double From, double To);
	void ClapEvents(double From, double To);
	void RenderBlock();
	void RenderAir();
	void RenderWalla();
	void RenderSlots();
	void RenderVoices();
	void RenderFoley();
	void RenderClaps();
	void Finish();
	int OffsetOf(double At) const;
	const Emitter* Find(Spot Where, int Id) const;
	void PlaceFor(Spot Where, int Id, Place& P, float Spread);
	void AddDuck(float Attack, float Hold, float Release, float Bed, float TalkDb, float Chips);
	float DuckDb(int L, double At) const;
	int Bytes() const;
};

CardRoom::Impl::Impl(uint32_t InSeed) : Seed(InSeed == 0u ? 1u : InSeed)
{
	TalkRng.Seed(vox::Mix32(Seed, 1u));
	BedRng.Seed(vox::Mix32(Seed, 3u));
	RailRng.Seed(vox::Mix32(Seed, 4u));
	EventRng.Seed(vox::Mix32(Seed, 5u));
	Far.Rng.Seed(vox::Mix32(Seed, 6u));
	Slots.R.Seed(vox::Mix32(Seed, 7u));
	Terrace.R.Seed(vox::Mix32(Seed, 8u));
	AirNoise[0].Seed(vox::Mix32(Seed, 9u));
	AirNoise[1].Seed(vox::Mix32(Seed, 10u));
	for (int I = 0; I < room_audio::FoleyPool; ++I)
	{
		Voices[I].R.Seed(vox::Mix32(Seed, 0xf01e00u + static_cast<uint32_t>(I)));
	}
	for (int S = 0; S < 2; ++S)
	{
		room_audio::WallaSide& W = Walla[S];
		W.Shape.BandPass(560.0f, 0.55f, room_audio::Rate8);
		W.Noise.Seed(vox::Mix32(Seed, 20u + static_cast<uint32_t>(S)));
		for (room_audio::WallaVoice& V : W.A)
		{
			V.Next = static_cast<double>(TalkRng.Range(0.0f, 0.3f));
			V.PhraseUntil = static_cast<double>(TalkRng.Range(0.0f, 2.0f));
			V.On = TalkRng.Chance(0.6f);
		}
		for (room_audio::WallaVoice& V : W.B)
		{
			V.Next = static_cast<double>(TalkRng.Range(0.0f, 0.3f));
			V.PhraseUntil = static_cast<double>(TalkRng.Range(0.0f, 2.0f));
			V.On = TalkRng.Chance(0.6f);
		}
	}
	Resize();
	Pa.Init(room_audio::Rate16, vox::Mix32(Seed, 11u));
	std::memset(PaDelay, 0, sizeof(PaDelay));
	Reverb.Init(0.7f, room_audio::Rate8);
	Reverb.WetGain = 0.16f;
	std::memset(Fifo, 0, sizeof(Fifo));
	std::memset(ClapStart, 0, sizeof(ClapStart));
	MuffleL.LowPass(20000.0f, 0.707f, room_audio::Rate48);
	MuffleR = MuffleL;
	AppliedHz = 20000.0f;
	Far.Next = 0.5;
}

void CardRoom::Impl::Resize()
{
	// The room's talkers, as many as the budget (two in the fallback, for its laughs and sighs).
	const size_t Want = static_cast<size_t>(std::max(2, Budget));
	const size_t Had = Talk.size();
	Talk.resize(Want);
	if (Want < Had)
	{
		Talk.shrink_to_fit(); // back from 12 to the budget: the memory goes too
	}
	for (size_t I = 0; I < Talk.size(); ++I)
	{
		room_audio::Ambient& A = Talk[I];
		A.Voice.Rebind(A.Phrase); // the phrase moved with the slot
		if (I >= Had)
		{
			A.NextAt = EventNow + static_cast<double>(TalkRng.Range(0.1f, 2.5f));
		}
	}
}

const CardRoom::Emitter* CardRoom::Impl::Find(Spot Where, int Id) const
{
	// A table by its id (the asked kind first); a spot by its id, else the first of its kind; the feature table also
	// by kind alone. A numbered table nobody listed has no place: the caller falls back.
	const bool TableKind = Where == Spot::Table || Where == Spot::FeatureTable;
	const Emitter* Same = nullptr;
	const Emitter* Any = nullptr;
	for (const Emitter& E : Emitters)
	{
		if (TableKind)
		{
			if ((E.Kind == Spot::Table || E.Kind == Spot::FeatureTable) && E.Id == Id)
			{
				if (E.Kind == Where)
				{
					return &E;
				}
				Same = Same ? Same : &E;
			}
			if (Where == Spot::FeatureTable && E.Kind == Spot::FeatureTable && !Any)
			{
				Any = &E;
			}
		}
		else if (E.Kind == Where)
		{
			if (E.Id == Id)
			{
				return &E;
			}
			Any = Any ? Any : &E;
		}
	}
	return Same ? Same : Any;
}

void CardRoom::Impl::PlaceFor(Spot Where, int Id, Place& P, float Spread)
{
	const Emitter* E = Find(Where, Id);
	float X, Y;
	if (E)
	{
		X = E->X;
		Y = E->Y;
	}
	else if (!Emitters.empty())
	{
		// Somewhere we weren't told: the middle of what we know.
		X = Y = 0.0f;
		for (const Emitter& M : Emitters)
		{
			X += M.X;
			Y += M.Y;
		}
		X /= static_cast<float>(Emitters.size());
		Y /= static_cast<float>(Emitters.size());
	}
	else
	{
		X = Ear.X + 600.0f * Ear.Cos;
		Y = Ear.Y + 600.0f * Ear.Sin;
	}
	if (Spread > 0.0f)
	{
		X += Spread * EventRng.Signed();
		Y += Spread * EventRng.Signed();
	}
	P.At(X, Y);
	room_audio::Aim(P, Ear);
}

void CardRoom::Impl::Rebuild()
{
	// Tables keep their hands by id; the Foley pool lets go (a click cut short is nothing); spots start fresh.
	std::vector<room_audio::TableState> Old(Tables, Tables + TableCount);
	const int OldCount = TableCount;
	for (room_audio::Foley& F : Voices)
	{
		F.Silence();
		F.Owner = -1;
	}
	TableCount = 0;
	SpotCount = 0;
	Slots.Exists = false;
	Terrace.Exists = false;
	for (const Emitter& E : Emitters)
	{
		if (E.Kind == Spot::Table || E.Kind == Spot::FeatureTable)
		{
			if (TableCount >= room_audio::MaxTables)
			{
				continue;
			}
			room_audio::TableState& T = Tables[TableCount++];
			bool Kept = false;
			for (int I = 0; I < OldCount; ++I)
			{
				if (Old[static_cast<size_t>(I)].Exists && Old[static_cast<size_t>(I)].Id == E.Id && Old[static_cast<size_t>(I)].Feature == (E.Kind == Spot::FeatureTable))
				{
					T = Old[static_cast<size_t>(I)];
					Old[static_cast<size_t>(I)].Exists = false;
					Kept = true;
					break;
				}
			}
			if (!Kept)
			{
				T = room_audio::TableState();
				T.R.Seed(vox::Mix32(Seed, 0x7ab1e000u + static_cast<uint32_t>(E.Id) * 2u + (E.Kind == Spot::FeatureTable ? 1u : 0u)));
				T.IdleNext = EventNow + static_cast<double>(T.R.Range(0.5f, 6.0f));
			}
			T.Exists = true;
			T.Id = E.Id;
			T.Feature = E.Kind == Spot::FeatureTable;
			T.Open = E.Open;
			T.Players = std::max(0, E.Players);
			T.Voice = -1;
			if (!Kept || T.Where.X != E.X || T.Where.Y != E.Y)
			{
				T.Where.At(E.X, E.Y);
			}
		}
		else if (E.Kind == Spot::Doorway)
		{
			if (!Slots.Exists)
			{
				Slots.Exists = true;
				if (Slots.Where.X != E.X || Slots.Where.Y != E.Y)
				{
					Slots.Where.At(E.X, E.Y);
				}
			}
		}
		else if (E.Kind == Spot::Terrace)
		{
			Terrace.Exists = true;
			Terrace.Open = E.Open;
			if (Terrace.Where.X != E.X || Terrace.Where.Y != E.Y)
			{
				Terrace.Where.At(E.X, E.Y);
			}
		}
		else if ((E.Kind == Spot::Bar || E.Kind == Spot::Cage || E.Kind == Spot::Desk) && SpotCount < room_audio::MaxSpots)
		{
			room_audio::SpotState& S = Spots[SpotCount++];
			S = room_audio::SpotState();
			S.Kind = E.Kind;
			S.Id = E.Id;
			S.Where.At(E.X, E.Y);
			S.R.Seed(vox::Mix32(Seed, 0x5b07u + static_cast<uint32_t>(E.Kind) * 131u + static_cast<uint32_t>(E.Id)));
			S.Sound.R.Seed(vox::Mix32(Seed, 0x5b08u + static_cast<uint32_t>(E.Kind) * 131u + static_cast<uint32_t>(E.Id)));
			S.Next = EventNow + static_cast<double>(S.R.Range(1.0f, 8.0f));
		}
	}
	Moved = true;
}

void CardRoom::Impl::AimAll()
{
	// The voiced tables: the four nearest open ones with players, and the feature table.
	int Rank[room_audio::MaxTables];
	int N = 0;
	for (int I = 0; I < TableCount; ++I)
	{
		room_audio::Aim(Tables[I].Where, Ear);
		Tables[I].Voiced = false;
		if (Tables[I].Open && Tables[I].Players > 0 && !Tables[I].Feature && Tables[I].Where.Dist < 14.0f)
		{
			Rank[N++] = I;
		}
	}
	std::sort(Rank, Rank + N, [&](int A, int B) { return Tables[A].Where.Dist < Tables[B].Where.Dist || (Tables[A].Where.Dist == Tables[B].Where.Dist && A < B); });
	VoicedCount = 0;
	for (int K = 0; K < std::min(N, room_audio::VoicedTables); ++K)
	{
		Tables[Rank[K]].Voiced = true;
		++VoicedCount;
	}
	int OpenTables = 0;
	for (int I = 0; I < TableCount; ++I)
	{
		room_audio::TableState& T = Tables[I];
		if (T.Feature && T.Open && T.Players > 0 && VoicedCount < room_audio::FoleyPool)
		{
			T.Voiced = true;
			++VoicedCount;
		}
		if (T.Open && T.Players > 0)
		{
			++OpenTables;
		}
	}
	FarShare = TableCount > 0 ? static_cast<float>(std::max(0, OpenTables - VoicedCount)) / static_cast<float>(std::max(1, TableCount)) : 0.0f;
	// The Foley pool follows the voiced tables: a voice keeps its table while it's voiced, or rings out after.
	for (int V = 0; V < room_audio::FoleyPool; ++V)
	{
		room_audio::Foley& F = Voices[V];
		if (F.Owner >= 0 && (F.Owner >= TableCount || !Tables[F.Owner].Voiced))
		{
			if (F.Owner < TableCount)
			{
				Tables[F.Owner].Voice = -1;
			}
			F.Owner = F.Busy > 0 ? -2 - F.Owner : -1; // ringing out for its old table
		}
	}
	for (int I = 0; I < TableCount; ++I)
	{
		room_audio::TableState& T = Tables[I];
		if (!T.Voiced || T.Voice >= 0)
		{
			continue;
		}
		int Free = -1;
		for (int V = 0; V < room_audio::FoleyPool && Free < 0; ++V)
		{
			Free = Voices[V].Owner == -1 ? V : -1;
		}
		for (int V = 0; V < room_audio::FoleyPool && Free < 0; ++V)
		{
			Free = Voices[V].Owner <= -2 ? V : -1;
		}
		if (Free >= 0)
		{
			Voices[Free].Silence();
			Voices[Free].Owner = I;
			T.Voice = Free;
		}
	}
	for (int I = 0; I < SpotCount; ++I)
	{
		room_audio::Aim(Spots[I].Where, Ear);
	}
	if (Slots.Exists)
	{
		room_audio::Aim(Slots.Where, Ear);
	}
	if (Terrace.Exists)
	{
		room_audio::Aim(Terrace.Where, Ear);
	}
	for (room_audio::Ambient& A : Talk)
	{
		if (A.Known)
		{
			room_audio::Aim(A.Where, Ear);
		}
	}
	for (room_audio::LineSlot& L : Lines)
	{
		if (L.Active && !L.Pa)
		{
			room_audio::Aim(L.Where, Ear);
		}
	}
	for (room_audio::RailSlot& R : Rail)
	{
		if (R.Active)
		{
			room_audio::Aim(R.Where, Ear);
		}
	}
	Moved = false;
}

void CardRoom::Impl::AddDuck(float Attack, float Hold, float Release, float Bed, float TalkDb, float Chips)
{
	int Slot = 0;
	double Oldest = 1.0e30;
	for (int I = 0; I < room_audio::MaxDucks; ++I)
	{
		if (!Ducks[I].Live)
		{
			Slot = I;
			break;
		}
		if (Ducks[I].Start < Oldest)
		{
			Oldest = Ducks[I].Start;
			Slot = I;
		}
	}
	room_audio::DuckState& D = Ducks[Slot];
	D = room_audio::DuckState();
	D.Live = true;
	// From the next sample rendered, the same one a line said now starts on (the FIFO holds under 5 ms before it).
	D.Start = Now;
	D.Attack = std::max(0.005f, Attack);
	D.Hold = std::max(0.0f, Hold);
	D.Release = std::max(0.01f, Release);
	D.Depth[room_audio::LBed] = -std::fabs(Bed);
	D.Depth[room_audio::LTalk] = -std::fabs(TalkDb);
	D.Depth[room_audio::LChips] = -std::fabs(Chips);
}

float CardRoom::Impl::DuckDb(int L, double At) const
{
	float Deepest = 0.0f;
	for (const room_audio::DuckState& D : Ducks)
	{
		if (!D.Live || D.Depth[L] == 0.0f)
		{
			continue;
		}
		const float T = static_cast<float>(At - D.Start);
		if (T < 0.0f)
		{
			continue;
		}
		float Env;
		if (T < D.Attack)
		{
			Env = T / D.Attack;
		}
		else if (T < D.Attack + D.Hold)
		{
			Env = 1.0f;
		}
		else
		{
			Env = std::max(0.0f, 1.0f - (T - D.Attack - D.Hold) / D.Release);
		}
		Deepest = std::min(Deepest, D.Depth[L] * Env);
	}
	return Deepest;
}

int CardRoom::Impl::OffsetOf(double At) const
{
	// Where an event on the room's clock falls inside the block about to render.
	const double Into = (At - EventNow) / static_cast<double>(std::max(0.01f, Pace));
	const int Off = static_cast<int>(Into * static_cast<double>(room_audio::Rate48));
	return std::max(0, std::min(room_audio::Block - 1, Off));
}

void CardRoom::Impl::PlayFx(room_audio::TableState& T, room_audio::Fx Kind, float Level, float P1, double At)
{
	using room_audio::Fx;
	if (T.Players <= 0)
	{
		return; // the hero sat down mid-hand: the table actor has it now
	}
	if (T.Voiced && T.Voice >= 0)
	{
		Voices[T.Voice].Add(OffsetOf(At), Kind, Level, P1);
		return;
	}
	// A table nobody near can pick out: its chips join the far scatter.
	if (Kind == Fx::Click || Kind == Fx::Clicks || Kind == Fx::ChipRiffle || Kind == Fx::Slide)
	{
		Far.Requests = std::min(Far.Requests + 1, 6);
	}
}

void CardRoom::Impl::FireStep(room_audio::TableState& T, const room_audio::Step& S, double At)
{
	using room_audio::Fx;
	const float Level = static_cast<float>(S.Gain) / 255.0f;
	if (S.N > 1 || S.Kind == Fx::Clicks || S.Kind == Fx::ChipRiffle || S.Kind == Fx::Card)
	{
		// Into a run; the older one finishes in the other slot.
		room_audio::Seq& Run = T.Runs[0].Left > 0 && T.Runs[1].Left <= 0 ? T.Runs[1] : T.Runs[0];
		Run.Kind = S.Kind;
		Run.Left = std::max<int>(1, S.N);
		Run.Count = 0;
		Run.Next = At;
		Run.Gap = static_cast<float>(S.Gap) * 0.005f;
		Run.Gain = Level;
		Run.PitchA = T.R.Range(3000.0f, 3900.0f);
		Run.PitchB = Run.PitchA * T.R.Range(1.08f, 1.22f);
		return;
	}
	PlayFx(T, S.Kind, Level, S.Kind == Fx::Slide ? static_cast<float>(S.Gap) * 0.02f : 0.0f, At);
}

void CardRoom::Impl::TableEvents(room_audio::TableState& T, double From, double To)
{
	using room_audio::Fx;
	while (T.StepAt < T.StepCount && T.HandStart + static_cast<double>(T.Steps[T.StepAt].At) < To)
	{
		const double At = std::max(From, T.HandStart + static_cast<double>(T.Steps[T.StepAt].At));
		FireStep(T, T.Steps[T.StepAt], At);
		++T.StepAt;
	}
	for (room_audio::Seq& Run : T.Runs)
	{
		while (Run.Left > 0 && Run.Next < To)
		{
			const double At = std::max(From, Run.Next);
			float P1 = 0.0f;
			float G = Run.Gain * T.R.Range(0.7f, 1.0f);
			if (Run.Kind == Fx::ChipRiffle)
			{
				// Two stacks falling into each other: the pitches alternate.
				P1 = (Run.Count & 1) ? Run.PitchB : Run.PitchA;
				G *= 0.55f;
			}
			PlayFx(T, Run.Kind == Fx::Card ? Fx::Card : Fx::Click, G, P1, At);
			--Run.Left;
			++Run.Count;
			Run.Next += static_cast<double>(Run.Gap * T.R.Range(0.75f, 1.3f));
		}
	}
	// Players riffle their chips while they wait.
	if (T.Voiced && T.Players > 0 && T.IdleNext < To)
	{
		room_audio::Seq& Run = T.Runs[0].Left > 0 ? T.Runs[1] : T.Runs[0];
		if (Run.Left <= 0)
		{
			Run.Kind = Fx::ChipRiffle;
			Run.Left = 8 + T.R.Below(14);
			Run.Count = 0;
			Run.Next = std::max(From, T.IdleNext);
			Run.Gap = T.R.Range(0.028f, 0.042f);
			Run.Gain = T.R.Range(0.35f, 0.75f);
			Run.PitchA = T.R.Range(3000.0f, 3900.0f);
			Run.PitchB = Run.PitchA * T.R.Range(1.08f, 1.22f);
		}
		T.IdleNext = To + static_cast<double>(T.R.Range(2.0f, 8.0f) * 6.0f / static_cast<float>(std::max(2, T.Players)));
	}
}

void CardRoom::Impl::BuildHand(room_audio::TableState& T, float Seconds, bool AllIn, bool Called, float PotBb)
{
	using room_audio::Clamp;
	using room_audio::Fx;
	using room_audio::Step;
	vox::Rng R;
	R.Seed(vox::Mix32(vox::Mix32(Seed, static_cast<uint32_t>(T.Id) * 977u + (T.Feature ? 1u : 0u)), ++T.Hands));
	const float L = std::max(4.0f, Seconds);
	T.HandStart = EventNow;
	T.StepCount = 0;
	T.StepAt = 0;
	auto Add = [&](float At, Fx Kind, int N, float Level, float GapSeconds) {
		if (T.StepCount < 24 && At < L)
		{
			Step& S = T.Steps[T.StepCount++];
			S.At = At;
			S.Kind = Kind;
			S.N = static_cast<uint8_t>(std::max(1, std::min(N, 255)));
			S.Gain = static_cast<uint8_t>(Clamp(Level, 0.0f, 1.0f) * 255.0f);
			S.Gap = static_cast<uint8_t>(Clamp(GapSeconds / 0.005f, 1.0f, 255.0f));
		}
	};
	// Each table runs its own offset, so they never fall into step.
	float At = 0.2f + R.Range(0.0f, 0.12f) * L;
	// The shuffle: riffle, riffle, box, riffle, cut (spec §4.4), squeezed into at most 8 s or a fifth of the hand.
	const float Squeeze = std::min(1.0f, std::min(8.0f, 0.2f * L) / 4.6f);
	Add(At, Fx::Shuffle, 1, 0.8f, 0.0f);
	At += 1.05f * Squeeze;
	Add(At, Fx::Shuffle, 1, 0.8f, 0.0f);
	At += 1.05f * Squeeze;
	Add(At, Fx::Tap, 1, 0.7f, 0.0f);
	At += 0.45f * Squeeze;
	Add(At, Fx::Shuffle, 1, 0.75f, 0.0f);
	At += 1.05f * Squeeze;
	Add(At, Fx::Tap, 1, 0.6f, 0.0f);
	At += 0.5f * Squeeze;
	// The pitch: two cards to each player, in a quick ticking line.
	const int Seats = std::max(2, std::min(10, T.Players));
	const float Pitch = std::min(0.26f, 0.12f * L / static_cast<float>(2 * Seats));
	Add(At, Fx::Card, 2 * Seats, 0.7f, Pitch);
	At += Pitch * static_cast<float>(2 * Seats) + 0.4f;
	// The action, street by street, with what's left (less the push and the stacking).
	const float Left = std::max(1.0f, L - At - 3.0f);
	const int Streets = AllIn ? 4 : (R.Chance(0.6f) ? (R.Chance(0.55f) ? (R.Chance(0.7f) ? 4 : 3) : 2) : 1);
	const int AllInStreet = AllIn ? R.Below(Streets) : -1;
	const float Share = Left / static_cast<float>(Streets);
	for (int S = 0; S < Streets; ++S)
	{
		float T0 = At + Share * static_cast<float>(S);
		if (S > 0)
		{
			Add(T0, Fx::Card, S == 1 ? 3 : 1, 0.55f, 0.28f);
			T0 += S == 1 ? 1.0f : 0.5f;
		}
		if (S == AllInStreet)
		{
			const float Big = Clamp(std::log2(1.0f + PotBb) / 6.0f, 0.3f, 1.0f);
			Add(T0 + 0.6f, Fx::Slide, 1, 0.8f * Big + 0.2f, 0.7f);
			Add(T0 + 0.7f, Fx::Clicks, 10 + R.Below(9), 0.9f, 0.04f);
			if (Called)
			{
				Add(T0 + 2.4f, Fx::Slide, 1, 0.7f, 0.6f);
				Add(T0 + 2.5f, Fx::Clicks, 8 + R.Below(9), 0.85f, 0.045f);
			}
			continue;
		}
		if (AllInStreet >= 0 && S > AllInStreet)
		{
			continue; // the board runs out with no action
		}
		const int Actions = 1 + R.Below(3);
		for (int K = 0; K < Actions; ++K)
		{
			const float Ta = T0 + (static_cast<float>(K) + 0.5f) * (Share - 1.0f) / static_cast<float>(Actions);
			if (R.Chance(0.35f))
			{
				Add(Ta, Fx::Knock, 1, 0.6f, 0.0f);
			}
			else
			{
				Add(Ta, Fx::Clicks, 3 + R.Below(6), R.Range(0.6f, 0.9f), R.Range(0.035f, 0.07f));
			}
		}
	}
	// The push to the winner, then they stack it.
	const float End = L - 2.6f;
	const float Size = Clamp(std::log2(1.0f + std::max(0.0f, PotBb)), 1.0f, 8.0f);
	Add(End, Fx::Slide, 1, 0.5f + 0.06f * Size, 0.4f + 0.1f * Size);
	Add(End + 0.3f, Fx::Clicks, 4 + static_cast<int>(2.0f * Size), 0.8f, 0.05f);
	Add(End + 1.2f, Fx::Clicks, 6 + R.Below(10), 0.6f, R.Range(0.07f, 0.12f));
	if (R.Chance(0.5f))
	{
		Add(End + 2.3f, Fx::ChipRiffle, 10 + R.Below(10), 0.6f, 0.033f);
	}
	if (R.Chance(0.08f))
	{
		Add(R.Range(1.0f, L - 1.0f), Fx::Scrape, 1, 0.6f, 0.0f);
	}
	// The plan's steps go in time order.
	std::stable_sort(T.Steps, T.Steps + T.StepCount, [](const Step& A, const Step& B) { return A.At < B.At; });
}

void CardRoom::Impl::FarEvent(float Level, int Clicks, float Gap)
{
	room_audio::Seq& Run = Far.Run;
	if (Run.Left > 0)
	{
		return;
	}
	Run.Kind = room_audio::Fx::Click;
	Run.Left = Clicks;
	Run.Next = EventNow;
	Run.Gap = Gap;
	Run.Gain = Level;
	Run.PitchA = Far.Rng.Range(2600.0f, 3800.0f);
	Run.PitchB = Run.PitchA * Far.Rng.Range(1.08f, 1.2f);
	Run.Count = 0;
	Far.RunPan = Far.Rng.Signed();
}

void CardRoom::Impl::Recast(int I, float RailShare)
{
	// Somebody else takes the chair (the room has more people than voices), and where they are: a voiced table (a
	// partner's, most of the time), the rail when it's formed (more of them once the tables hush), the bar.
	room_audio::Ambient& A = Talk[static_cast<size_t>(I)];
	vox::Rng& R = TalkRng;
	const int Count = static_cast<int>(Talk.size());
	A.Person = vox::Mix32(Seed, 0x9e0000u + ++People);
	const bool Woman = (A.Person % 100u) < 38u;
	const int Age = 21 + static_cast<int>((A.Person >> 8) % 55u);
	A.Who = TimbreFor(Woman, Age, A.Person);
	int Pick = -1;
	if ((I ^ 1) < Count)
	{
		const room_audio::Ambient& Partner = Talk[static_cast<size_t>(I ^ 1)];
		if (Partner.Known && Partner.Table >= 0 && R.Chance(0.7f))
		{
			for (int K = 0; K < TableCount; ++K)
			{
				if (Tables[K].Id == Partner.Table && Tables[K].Voiced && !Tables[K].Feature)
				{
					Pick = K;
				}
			}
		}
	}
	if (Pick < 0)
	{
		float Total = 0.0f;
		for (int K = 0; K < TableCount; ++K)
		{
			if (Tables[K].Voiced)
			{
				Total += static_cast<float>(Tables[K].Players);
			}
		}
		float X = R.Uniform() * Total;
		for (int K = 0; K < TableCount && Total > 0.0f; ++K)
		{
			if (!Tables[K].Voiced)
			{
				continue;
			}
			X -= static_cast<float>(Tables[K].Players);
			if (X < 0.0f)
			{
				Pick = K;
				break;
			}
		}
	}
	A.Layer = room_audio::LTalk;
	if (RailShare > 0.0f && R.Chance(RailShare))
	{
		PlaceFor(Spot::Rail, 0, A.Where, 250.0f);
		A.Table = -1;
		A.Layer = room_audio::LRail;
	}
	else if (Pick >= 0)
	{
		const room_audio::TableState& T = Tables[Pick];
		const float Seat = static_cast<float>(A.Person % 9u) / 9.0f * vox::TwoPi + R.Range(-0.2f, 0.2f);
		const float Out = R.Range(95.0f, 125.0f);
		A.Where.At(T.Where.X + Out * std::cos(Seat), T.Where.Y + Out * std::sin(Seat));
		room_audio::Aim(A.Where, Ear);
		A.Table = T.Id;
	}
	else if (Find(Spot::Bar, 0) != nullptr && R.Chance(0.5f))
	{
		PlaceFor(Spot::Bar, 0, A.Where, 200.0f);
		A.Table = -1;
	}
	else
	{
		// No tables to sit at: around the listener, 4 to 9 m out.
		const float Angle = R.Uniform() * vox::TwoPi;
		const float Out = R.Range(400.0f, 900.0f);
		A.Where.At(Ear.X + Out * std::cos(Angle), Ear.Y + Out * std::sin(Angle));
		room_audio::Aim(A.Where, Ear);
		A.Table = -1;
	}
	A.Known = true;
}

void CardRoom::Impl::StartPhrase(int I)
{
	using room_audio::Clamp;
	room_audio::Ambient& A = Talk[static_cast<size_t>(I)];
	vox::Rng& R = TalkRng;
	const bool Fallback = Budget == 0;
	// The Lombard effect: in a louder room talkers press their voices, higher and brighter. Only the quality: the level
	// they gain is already in the Talk layer (room::MixFor's lift), so it isn't counted twice.
	const float Lombard = Clamp(0.02f * (Db[room_audio::LTalk] + 2.0f), 0.0f, 0.15f);
	const float Calm = Clamp(0.5f + 0.15f * R.Uniform(), 0.35f, 0.9f);
	const float Effort = Clamp(Calm + Lombard, 0.35f, 0.9f);
	const float Choice = R.Uniform();
	A.Gain = vox::DbToGain((Calm - 0.6f) * 14.0f);
	if (Fallback)
	{
		// The fallback: no talk, only the room's non-word sounds now and then.
		const Vocal V = Choice < 0.45f ? Vocal::Laugh : Choice < 0.75f ? Vocal::Scoff : Vocal::Sigh;
		A.Count = vox::VocalPlan(V, A.Who, R, 0.0f, A.Phrase, 16);
	}
	else if (Choice < 0.04f)
	{
		A.Count = vox::VocalPlan(Vocal::Laugh, A.Who, R, 0.0f, A.Phrase, 16);
	}
	else if (Choice < 0.055f)
	{
		A.Count = vox::VocalPlan(Vocal::Scoff, A.Who, R, 0.0f, A.Phrase, 16);
	}
	else
	{
		const float Length = R.Uniform();
		const int N = 3 + static_cast<int>(Length * Length * 12.0f);
		vox::BabblePhrase(N, R.Chance(0.12f), A.Who, Effort, R, 0.0f, A.Phrase);
		A.Count = N;
		A.Gain *= (0.55f + 0.75f * Calm) / (0.55f + 0.75f * Effort); // the prosody's effort gain, back to the calm voice's
	}
	A.Voice.Start(A.Phrase, A.Count, A.Who, true, Effort, vox::Mix32(A.Person, ++Events), room_audio::Rate8);
	A.Speaking = A.Count > 0;
}

void CardRoom::Impl::Ambience(double To)
{
	using room_audio::Clamp;
	vox::Rng& R = TalkRng;
	const float Activity = Clamp(1.0f + Db[room_audio::LTalk] / 12.0f, 0.1f, 1.4f);
	const int Count = static_cast<int>(Talk.size());
	const int Active = Budget == 0 ? 2 : std::max(1, static_cast<int>(std::floor(static_cast<float>(Budget) * Clamp(0.25f + 0.75f * CrowdNow, 0.15f, 1.0f) + 0.5f)));
	// The rail, once it's formed, takes a share of the room's voices and hushes with it (the bubble's hush is everyone's).
	// Once the tables have emptied (the last tables, the final table) the rail leads: most of the voices are there, and
	// they talk at the rail's own level while the felt goes quiet.
	const bool RailUp = Db[room_audio::LRail] > -30.0f && Find(Spot::Rail, 0) != nullptr;
	const bool RailLeads = RailUp && CrowdNow < 0.2f;
	const float RailActivity = RailLeads ? Clamp(1.0f + Db[room_audio::LRail] / 12.0f, 0.1f, 1.4f) : Activity;
	const float RailShare = RailUp ? (RailLeads ? 0.8f : 0.3f) : 0.0f;
	for (int I = 0; I < Count; ++I)
	{
		room_audio::Ambient& A = Talk[static_cast<size_t>(I)];
		if (A.Speaking && !A.Voice.Active())
		{
			A.Speaking = false;
			const int Was = A.Table;
			// Now and then somebody else takes the chair; always when the room has moved on (to the rail, or off it).
			const bool Away = A.Layer == room_audio::LRail ? !RailUp : RailLeads;
			if (R.Chance(0.3f) || Away)
			{
				Recast(I, RailShare);
			}
			const float Act = A.Layer == room_audio::LRail ? RailActivity : Activity;
			const float Gap = Budget == 0 ? R.Range(4.0f, 12.0f) : R.Range(0.4f, 3.5f) * (1.4f - 0.6f * std::min(1.0f, Act));
			A.NextAt = To + static_cast<double>(Gap / Act);
			// A conversation: the partner at the same table answers.
			if (Budget > 0 && (I ^ 1) < std::min(Active, Count))
			{
				room_audio::Ambient& Mate = Talk[static_cast<size_t>(I ^ 1)];
				if (!Mate.Speaking && Mate.Known && Mate.Table == Was && Was >= 0 && R.Chance(0.65f))
				{
					Mate.NextAt = std::min(Mate.NextAt, To + static_cast<double>(R.Range(0.15f, 0.7f)));
				}
			}
		}
		if (!A.Speaking && I < Active && A.NextAt <= To)
		{
			if (!A.Known || (A.Layer == room_audio::LRail && !RailUp))
			{
				Recast(I, RailShare); // nobody talks at a rail that's gone
			}
			StartPhrase(I);
		}
	}
	// The walla's virtual talkers: syllables in phrases, many of them, so the wash breathes.
	const int OnA = 2 + static_cast<int>(2.0f * CrowdNow + 0.5f);
	const int OnB = 4 + static_cast<int>(6.0f * CrowdNow + 0.5f);
	for (room_audio::WallaSide& W : Walla)
	{
		for (int Pass = 0; Pass < 2; ++Pass)
		{
			room_audio::WallaVoice* Voice = Pass == 0 ? W.A : W.B;
			const int N = Pass == 0 ? 4 : 10;
			const int On = Pass == 0 ? OnA : OnB;
			for (int K = 0; K < N; ++K)
			{
				room_audio::WallaVoice& V = Voice[K];
				if (V.PhraseUntil <= To)
				{
					V.On = K < On && R.Chance(V.On ? 0.35f : 0.75f * std::min(1.0f, Activity));
					V.PhraseUntil = To + static_cast<double>(V.On ? R.Range(1.0f, 4.5f) : R.Range(0.3f, 2.5f) / Activity);
				}
				if (V.Next <= To)
				{
					V.Target = V.On ? R.Range(0.25f, 1.0f) : 0.0f;
					V.Next = To + static_cast<double>(R.Range(0.11f, 0.26f));
				}
				V.Level += (V.Target - V.Level) * 0.18f;
			}
		}
	}
}

void CardRoom::Impl::SpotEvents(double From, double To)
{
	using room_audio::Clamp;
	using room_audio::Fx;
	for (int I = 0; I < SpotCount; ++I)
	{
		room_audio::SpotState& S = Spots[I];
		if (S.Next > To)
		{
			continue;
		}
		const int Off = OffsetOf(S.Next);
		switch (S.Kind)
		{
		case Spot::Bar:
			S.Sound.Add(Off, S.R.Chance(0.75f) ? Fx::Glass : Fx::Bottle, S.R.Range(0.4f, 1.0f), 0.0f);
			S.Next = To + static_cast<double>(S.R.Range(2.5f, 10.0f));
			break;
		case Spot::Cage:
			S.Sound.Add(Off, S.R.Chance(0.6f) ? Fx::Rack : Fx::Clicks, S.R.Range(0.5f, 1.0f), 0.0f);
			S.Next = To + static_cast<double>(S.R.Range(8.0f, 30.0f));
			break;
		default:
			S.Sound.Add(Off, Fx::Printer, S.R.Range(0.5f, 0.9f), 0.0f);
			S.Next = To + static_cast<double>(S.R.Range(30.0f, 80.0f));
			break;
		}
	}
	// The slot floor's winners.
	if (Slots.Exists && Slots.NextJingle <= To)
	{
		Slots.JingleAt = To;
		Slots.JingleRoot = Slots.R.Chance(0.5f) ? 1046.5f : 880.0f;
		Slots.JinglePhase = 0.0f;
		Slots.NextJingle = To + static_cast<double>(Slots.R.Range(9.0f, 34.0f));
	}
	if (Slots.Exists && Slots.NextCoin <= To)
	{
		Slots.Coin.Strike(Slots.R.Range(1900.0f, 3100.0f), Slots.R.Range(0.12f, 0.3f), Slots.R.Range(0.03f, 0.08f), room_audio::Rate8);
		Slots.NextCoin = To + static_cast<double>(Slots.R.Range(0.7f, 5.0f));
	}
	// The far tables' scatter: chips somebody, somewhere, is always handling.
	if ((FarShare > 0.0f || Far.Requests > 0) && (Far.Next <= To || Far.Requests > 0))
	{
		const bool Riffle = Far.Rng.Chance(0.4f);
		if (Far.Run.Left <= 0)
		{
			FarEvent(Far.Rng.Range(0.3f, 1.0f), Riffle ? 8 + Far.Rng.Below(8) : 2 + Far.Rng.Below(4), Riffle ? Far.Rng.Range(0.028f, 0.04f) : Far.Rng.Range(0.04f, 0.09f));
			Far.Requests = std::max(0, Far.Requests - (Riffle ? 3 : 1));
		}
		if (Far.Next <= To)
		{
			Far.Next = To + static_cast<double>(Far.Rng.Range(0.15f, 0.9f) / std::max(0.08f, FarShare * 1.6f));
		}
	}
	room_audio::Seq& Run = Far.Run;
	while (Run.Left > 0 && Run.Next < To && Far.Queued < 8)
	{
		Far.Queue[Far.Queued] = room_audio::Pending{OffsetOf(std::max(From, Run.Next)), Fx::Click, Run.Gain * Far.Rng.Range(0.6f, 1.0f), (Run.Count & 1) ? Run.PitchB : Run.PitchA};
		Far.QueuePan[Far.Queued] = Clamp(Far.RunPan + 0.1f * Far.Rng.Signed(), -1.0f, 1.0f);
		++Far.Queued;
		--Run.Left;
		++Run.Count;
		Run.Next += static_cast<double>(Run.Gap * Far.Rng.Range(0.8f, 1.25f));
	}
	if (Terrace.Exists && Terrace.NextGust <= To)
	{
		Terrace.GustTarget = Terrace.R.Range(0.15f, 1.0f);
		Terrace.NextGust = To + static_cast<double>(Terrace.R.Range(0.6f, 3.0f));
	}
}

void CardRoom::Impl::ClapEvents(double From, double To)
{
	// Each clapper keeps their pace; a clap takes a free hand-voice (or the quietest).
	for (int I = 0; I < HandCount; ++I)
	{
		vox::Clapper& H = Hands[I];
		while (static_cast<double>(H.Next) < To && H.Next <= H.Until)
		{
			const float G = vox::ClapGain(H, H.Next);
			if (G > 0.0f)
			{
				int Slot = 0;
				float Quiet = 1.0e9f;
				for (int K = 0; K < room_audio::MaxClaps; ++K)
				{
					const float Level = Claps[K].Live ? Claps[K].Env * Claps[K].Gain : -1.0f;
					if (Level < Quiet)
					{
						Quiet = Level;
						Slot = K;
					}
				}
				vox::ClapVoice& C = Claps[Slot];
				C.Strike(H.Hz * RailRng.Range(0.95f, 1.05f), H.Q, G, RailRng.Range(0.004f, 0.008f), room_audio::Rate48);
				C.Band.Clear();
				const room_audio::HandSpot& At = HandAt[I];
				C.L = At.GL * (1.0f - 0.45f * H.Pan);
				C.R = At.GR * (1.0f + 0.45f * H.Pan);
				ClapAt[Slot] = At;
				ClapStart[Slot] = OffsetOf(std::max(From, static_cast<double>(H.Next)));
			}
			H.Next += H.Period * RailRng.Range(0.85f, 1.15f);
		}
	}
	// Clappers who've stopped leave the list.
	int Kept = 0;
	for (int I = 0; I < HandCount; ++I)
	{
		if (static_cast<double>(Hands[I].Until) > EventNow)
		{
			Hands[Kept] = Hands[I];
			HandAt[Kept] = HandAt[I];
			++Kept;
		}
	}
	HandCount = Kept;
}

void CardRoom::Impl::Control()
{
	using namespace room_audio;
	const float Dt = BlockSeconds;
	// The mix eases (1.5 s, linear in dB), the crowd too.
	for (int L = 0; L < LCount; ++L)
	{
		if (Db[L] != Target[L])
		{
			const float Step = Speed[L] * Dt;
			Db[L] = Db[L] < Target[L] ? std::min(Target[L], Db[L] + Step) : std::max(Target[L], Db[L] - Step);
		}
	}
	if (CrowdNow != CrowdTarget)
	{
		const float Step = CrowdSpeed * Dt;
		CrowdNow = CrowdNow < CrowdTarget ? std::min(CrowdTarget, CrowdNow + Step) : std::max(CrowdTarget, CrowdNow - Step);
	}
	// Ducks, evaluated at the block's end; the gain ramps to it across the block.
	const double End = Now + static_cast<double>(Dt);
	bool AnyDuck = false;
	for (DuckState& D : Ducks)
	{
		if (D.Live && End > D.Start + static_cast<double>(D.Attack + D.Hold + D.Release) + 0.01)
		{
			D.Live = false;
		}
		AnyDuck = AnyDuck || D.Live;
	}
	for (int L = 0; L < LCount; ++L)
	{
		Gain[L] = NextGain[L];
		RawGain[L] = RawNextGain[L];
		const float Duck = AnyDuck ? DuckDb(L, End) : 0.0f;
		RawNextGain[L] = vox::DbToGain(Db[L]);
		NextGain[L] = Duck < 0.0f ? vox::DbToGain(Db[L] + Duck) : RawNextGain[L];
	}
	// Muffling: down in 0.2 s, back up in 0.4 s, in octaves.
	if (MuffleHz != MuffleTargetHz || MuffleDb != MuffleTargetDb)
	{
		const float Octaves = std::log2(MuffleTargetHz / MuffleHz);
		const float Rate = (Octaves < 0.0f ? 6.0f / 0.2f : 6.0f / 0.4f) * Dt;
		const float Move = Clamp(Octaves, -Rate, Rate);
		MuffleHz = std::fabs(Octaves) <= Rate ? MuffleTargetHz : MuffleHz * std::exp2(Move);
		const float DbRate = 20.0f * Dt / 0.3f;
		MuffleDb = std::fabs(MuffleTargetDb - MuffleDb) <= DbRate ? MuffleTargetDb : MuffleDb + (MuffleTargetDb > MuffleDb ? DbRate : -DbRate);
	}
	if (std::fabs(MuffleHz - AppliedHz) > 1.0f)
	{
		MuffleL.LowPass(std::min(MuffleHz, 20000.0f), 0.707f, Rate48);
		MuffleR.B0 = MuffleL.B0;
		MuffleR.B1 = MuffleL.B1;
		MuffleR.B2 = MuffleL.B2;
		MuffleR.A1 = MuffleL.A1;
		MuffleR.A2 = MuffleL.A2;
		AppliedHz = MuffleHz;
	}
	if (Moved)
	{
		AimAll();
	}
	// The room's events, on its clock.
	const double From = EventNow;
	const double To = EventNow + static_cast<double>(Dt * Pace);
	for (int I = 0; I < TableCount; ++I)
	{
		TableEvents(Tables[I], From, To);
	}
	Ambience(To);
	SpotEvents(From, To);
	ClapEvents(From, To);
}

void CardRoom::Impl::RenderAir()
{
	using namespace room_audio;
	// The air handling: a low rush, and the diffusers' hiss, a little different each side.
	const float G0 = Gain[LBed], G1 = NextGain[LBed];
	const float Kr = vox::OnePoleK(110.0f, Rate8);
	const float Kh = vox::OnePoleK(1500.0f, Rate8);
	const float Kl = vox::OnePoleK(220.0f, Rate8);
	AirSwell += (BedRng.Range(0.85f, 1.15f) - AirSwell) * 0.002f;
	const float Swell = 0.9f + 0.1f * AirSwell;
	float Pw = 0.0f;
	for (int S = 0; S < 2; ++S)
	{
		float* SS_VOX_RESTRICT Out = S == 0 ? LowL : LowR;
		float* SS_VOX_RESTRICT Snd = LowSend;
		float B = Brown[S], Rm = Rumble[S], Hl = HissLo[S], Hh = HissHi[S];
		uint32_t Ns = AirNoise[S].S;
		float G = G0 * Swell;
		const float Dg = (G1 - G0) * Swell / static_cast<float>(Low);
		for (int I = 0; I < Low; ++I)
		{
			Ns ^= Ns << 13;
			Ns ^= Ns >> 17;
			Ns ^= Ns << 5;
			const float X = static_cast<float>(Ns >> 8) * (2.0f / 16777216.0f) - 1.0f;
			B = (B + 0.04f * X) * (1.0f / 1.04f);
			Rm += Kr * (B * 3.5f - Rm);
			Hh += Kh * (X - Hh);
			Hl += Kl * (Hh - Hl);
			const float Y = (Rm * 0.085f + (Hh - Hl) * 0.057f) * G; // white noise at 8 kHz is twice as dense: 1/sqrt 2 keeps the level
			Out[I] += Y;
			Snd[I] += Y * 0.12f;
			Pw += Y * Y;
			G += Dg;
		}
		AirNoise[S].S = Ns;
		Brown[S] = B + 1.0e-18f;
		Rumble[S] = Rm;
		HissLo[S] = Hl;
		HissHi[S] = Hh;
	}
	Power[LBed] += Pw * 0.5f / static_cast<float>(Low);
}

void CardRoom::Impl::RenderWalla()
{
	using namespace room_audio;
	// The walla: speech-shaped noise, each side its own crowd; A breathes with a few talkers, B is the far wash.
	const float G0 = Gain[LTalk], G1 = NextGain[LTalk];
	const float Boost = Budget == 0 ? 2.5f : 1.0f; // the fallback's walla carries the talk the talkers would have
	const float Kb = vox::OnePoleK(900.0f, Rate8);
	const float Inv = 1.0f / static_cast<float>(Low);
	float Pw = 0.0f;
	for (int S = 0; S < 2; ++S)
	{
		WallaSide& W = Walla[S];
		float SumA = 0.0f, SumB = 0.0f;
		for (const WallaVoice& V : W.A)
		{
			SumA += V.Level * V.Level;
		}
		for (const WallaVoice& V : W.B)
		{
			SumB += V.Level * V.Level;
		}
		// (Noise at 8 kHz is twice as dense as at 16 kHz, where these were set: 1/sqrt 2.)
		const float EnvA = std::sqrt(SumA * 0.25f) * 0.034f * Boost;
		const float EnvB = std::sqrt(SumB * 0.1f) * 0.0495f * Boost;
		const float DA = (EnvA * G1 - W.EnvA * G0) * Inv;
		const float DB = (EnvB * G1 - W.EnvB * G0) * Inv;
		float A = W.EnvA * G0, Bv = W.EnvB * G0;
		float Lp = W.Lp;
		vox::Biquad Shape = W.Shape;
		uint32_t Ns = W.Noise.S;
		float* SS_VOX_RESTRICT Out = S == 0 ? LowL : LowR;
		float* SS_VOX_RESTRICT Snd = LowSend;
		for (int I = 0; I < Low; ++I)
		{
			Ns ^= Ns << 13;
			Ns ^= Ns >> 17;
			Ns ^= Ns << 5;
			const float Sh = Shape.Run(static_cast<float>(Ns >> 8) * (2.0f / 16777216.0f) - 1.0f);
			Lp += Kb * (Sh - Lp);
			const float Y = Sh * A + Lp * Bv;
			Out[I] += Y;
			Snd[I] += Y * 0.3f;
			Pw += Y * Y;
			A += DA;
			Bv += DB;
		}
		W.Noise.S = Ns;
		W.Shape.Z1 = Shape.Z1;
		W.Shape.Z2 = Shape.Z2;
		W.Lp = Lp + 1.0e-18f;
		W.EnvA = EnvA;
		W.EnvB = EnvB;
	}
	Power[LTalk] += Pw * 0.5f * Inv;
}

void CardRoom::Impl::RenderSlots()
{
	using namespace room_audio;
	if (!Slots.Exists)
	{
		return;
	}
	// The slot floor: a soft chord that breathes, a winner's arpeggio, coins.
	const SineTable& Sn = Sines();
	static const float Chord[4] = {523.25f, 659.25f, 783.99f, 261.63f};
	static const float Amp[4] = {0.0035f, 0.0035f, 0.003f, 0.002f};
	static const float Steps[8] = {1.0f, 1.25f, 1.5f, 2.0f, 1.5f, 2.0f, 2.5f, 3.0f};
	const float Breath = 0.55f + 0.45f * std::sin(vox::TwoPi * 0.07f * static_cast<float>(std::fmod(EventNow, 1000.0)));
	const float Since = static_cast<float>(EventNow - Slots.JingleAt);
	const int Note = std::min(7, static_cast<int>(std::max(0.0f, Since) / 0.12f));
	const float JingleStep = Slots.JingleRoot * Steps[Note] / Rate8;
	const float Local = Since - static_cast<float>(Note) * 0.12f;
	const float JingleGain = Since >= 0.0f && Since < 1.6f ? 0.012f * std::exp(-Local * 9.0f) * (1.0f - Since / 1.6f) : 0.0f;
	float Ph[4] = {Slots.Phase[0], Slots.Phase[1], Slots.Phase[2], Slots.Phase[3]};
	float Inc[4], Gn[4];
	for (int K = 0; K < 4; ++K)
	{
		Inc[K] = Chord[K] / Rate8;
		Gn[K] = Amp[K] * Breath * 2.5f;
	}
	float Jp = Slots.JinglePhase;
	const float Jg = JingleGain * 2.5f;
	float C1 = Slots.Coin.Y1, C2 = Slots.Coin.Y2, Cin = Slots.Coin.In;
	const float Cb = Slots.Coin.B, Cc = Slots.Coin.C;
	float* SS_VOX_RESTRICT Out = Mono8;
	for (int I = 0; I < Low; ++I)
	{
		float Y = 0.0f;
		for (int K = 0; K < 4; ++K)
		{
			Ph[K] += Inc[K];
			Ph[K] -= Ph[K] >= 1.0f ? 1.0f : 0.0f;
			const float X = Ph[K] * 1024.0f;
			const int J = static_cast<int>(X);
			Y += (Sn.T[J] + (Sn.T[J + 1] - Sn.T[J]) * (X - static_cast<float>(J))) * Gn[K];
		}
		if (Jg > 0.0f)
		{
			Jp += JingleStep;
			Jp -= Jp >= 1.0f ? 1.0f : 0.0f;
			const float X = Jp * 1024.0f;
			const int J = static_cast<int>(X);
			Y += (Sn.T[J] + (Sn.T[J + 1] - Sn.T[J]) * (X - static_cast<float>(J))) * Jg;
		}
		const float Cn = Cb * C1 + Cc * C2 + Cin;
		Cin = 0.0f;
		C2 = C1;
		C1 = Cn;
		Out[I] = Y + Cn * 2.5f;
	}
	for (int K = 0; K < 4; ++K)
	{
		Slots.Phase[K] = Ph[K];
	}
	Slots.JinglePhase = Jp;
	Slots.Coin.Y1 = C1 + 1.0e-18f;
	Slots.Coin.Y2 = C2;
	Slots.Coin.In = 0.0f;
	Power[LBed] += Spatial(Slots.Where, Mono8, Low, Slots.Where.K8, Gain[LBed], NextGain[LBed], LowL, LowR, LowSend) / static_cast<float>(Low);
}

void CardRoom::Impl::RenderVoices()
{
	using namespace room_audio;
	const float Inv = 1.0f / static_cast<float>(Bus);
	const float InvLow = 1.0f / static_cast<float>(Low);
	bool Clear = false;
	// The room's talkers.
	for (Ambient& A : Talk)
	{
		if (!A.Speaking)
		{
			continue;
		}
		A.Voice.Render(Mono8, Low);
		++Speaking;
		const int L = A.Layer;
		const float Level = A.Gain * 2.2f;
		Power[L] += Spatial(A.Where, Mono8, Low, A.Where.K8, Gain[L] * Level, NextGain[L] * Level, LowL, LowR, LowSend) * InvLow;
	}
	// Lines: positioned, or the PA's speakers.
	bool PaLive = false;
	std::memset(PaBus, 0, sizeof(PaBus));
	for (LineSlot& Line : Lines)
	{
		if (!Line.Active)
		{
			continue;
		}
		if (!Line.Voice.Active())
		{
			// Done: its plan goes (a line's syllables are speech data, transient; the room's state stays small).
			Line.Active = false;
			std::vector<Syllable>().swap(Line.Plan);
			continue;
		}
		++Speaking;
		// A line at your table is what the room ducks for: it keeps its layer's level.
		const float* G0s = Line.Near ? RawGain : Gain;
		const float* G1s = Line.Near ? RawNextGain : NextGain;
		if (Line.Low)
		{
			Line.Voice.Render(Mono8, Low);
			const int L = Line.Layer;
			Power[L] += Spatial(Line.Where, Mono8, Low, Line.Where.K8, G0s[L] * Line.Gain, G1s[L] * Line.Gain, LowL, LowR, LowSend) * InvLow;
			continue;
		}
		Line.Voice.Render(Mono16, Bus);
		Clear = true;
		if (Line.Pa)
		{
			for (int I = 0; I < Bus; ++I)
			{
				PaBus[I] += Mono16[I] * Line.Gain;
			}
			PaLive = true;
			continue;
		}
		const int L = Line.Layer;
		Power[L] += Spatial(Line.Where, Mono16, Bus, Line.Where.K16, G0s[L] * Line.Gain, G1s[L] * Line.Gain, BusL, BusR, Send16) * Inv;
	}
	PaTail = PaLive ? 0.4f : PaTail - BlockSeconds;
	if (PaTail > 0.0f)
	{
		Clear = true;
		// The PA chain runs while anything is on it and for its taps' tail after; the far side's speakers 6 ms late.
		const int Delay = 96;
		float Pw = 0.0f;
		Pa.Process(PaBus, Bus);
		// Its level (SetPaLevel), ramped across the block from the last one's: no click when the Voices slider moves. At
		// the default (0 dB) the gain is exactly 1, sample for sample.
		const float G0 = PaGain;
		const float Step = (PaGainTarget - PaGain) / static_cast<float>(Bus);
		PaGain = PaGainTarget;
		for (int I = 0; I < Bus; ++I)
		{
			const float Y = PaBus[I] * (G0 + Step * static_cast<float>(I + 1));
			PaDelay[PaPos] = Y;
			const float Other = PaDelay[(PaPos + 128 - Delay) & 127];
			PaPos = (PaPos + 1) & 127;
			BusL[I] += Y * 0.6f;
			BusR[I] += Other * 0.6f;
			Send16[I] += Y * 0.3f;
			Pw += Y * Y * 0.36f;
		}
		Power[LPa] += Pw * Inv;
	}
	else
	{
		PaGain = PaGainTarget; // nothing on the PA: the next announcement starts at the level set
	}
	// The rail's bank.
	for (RailSlot& S : Rail)
	{
		if (!S.Active)
		{
			continue;
		}
		if (S.Delay >= Low)
		{
			S.Delay -= Low;
			continue;
		}
		if (S.Delay > 0)
		{
			std::memset(Mono8, 0, sizeof(float) * static_cast<size_t>(S.Delay));
			S.Voice.Render(Mono8 + S.Delay, Low - S.Delay);
			S.Delay = 0;
		}
		else
		{
			S.Voice.Render(Mono8, Low);
		}
		++Speaking;
		const int L = S.Layer;
		Power[L] += Spatial(S.Where, Mono8, Low, S.Where.K8, Gain[L] * S.Gain, NextGain[L] * S.Gain, LowL, LowR, LowSend) * InvLow;
		if (!S.Voice.Active())
		{
			S.Active = false;
		}
	}
	if (Clear)
	{
		Bus16Hot = 2;
	}
}

void CardRoom::Impl::RenderFoley()
{
	using namespace room_audio;
	const float Inv = 1.0f / static_cast<float>(Block);
	// The voiced tables' chips and cards, each from its table (or ringing out from where it was).
	for (int V = 0; V < FoleyPool; ++V)
	{
		Foley& F = Voices[V];
		const int Owner = F.Owner >= 0 ? F.Owner : (F.Owner <= -2 ? -2 - F.Owner : -1);
		if (Owner < 0 || Owner >= TableCount)
		{
			F.Queued = 0;
			continue;
		}
		if (!F.Render(Mono48, Block, Rate48))
		{
			if (F.Owner <= -2)
			{
				F.Owner = -1;
			}
			continue;
		}
		Power[LChips] += Spatial(Tables[Owner].Where, Mono48, Block, Tables[Owner].Where.K48, Gain[LChips] * 2.4f, NextGain[LChips] * 2.4f, MixL, MixR, Send48) * Inv;
	}
	for (int I = 0; I < SpotCount; ++I)
	{
		SpotState& S = Spots[I];
		if (!S.Sound.Render(Mono48, Block, Rate48))
		{
			continue;
		}
		Power[LBed] += Spatial(S.Where, Mono48, Block, S.Where.K48, Gain[LBed] * 0.8f, NextGain[LBed] * 0.8f, MixL, MixR, Send48) * Inv;
	}
	// The far scatter: each click its own spot in the stereo field, dull with distance.
	if (Far.Queued > 0 || Far.Ringing > 0)
	{
		const float G = 0.5f * (Gain[LChips] + NextGain[LChips]) * 0.35f;
		const float K = vox::OnePoleK(4200.0f, Rate48);
		float Pw = 0.0f;
		int Q = 0;
		float Lp = Far.LpL, Rp = Far.LpR;
		float* SS_VOX_RESTRICT Ml = MixL;
		float* SS_VOX_RESTRICT Mr = MixR;
		float* SS_VOX_RESTRICT Snd = Send48;
		for (int I = 0; I < Block; ++I)
		{
			while (Q < Far.Queued && Far.Queue[Q].At <= I)
			{
				const Pending& E = Far.Queue[Q];
				const float Pan = Far.QueuePan[Q];
				const float Decay = Far.Rng.Range(0.003f, 0.005f);
				Far.L.Strike(E.P1, Decay, E.Gain * std::sqrt(0.5f * (1.0f - 0.9f * Pan)), Rate48);
				Far.R.Strike(E.P1, Decay, E.Gain * std::sqrt(0.5f * (1.0f + 0.9f * Pan)), Rate48);
				Far.Ringing = Block + static_cast<int>(0.03f * Rate48);
				++Q;
			}
			const float Yl0 = Far.L.B * Far.L.Y1 + Far.L.C * Far.L.Y2 + Far.L.In;
			const float Yr0 = Far.R.B * Far.R.Y1 + Far.R.C * Far.R.Y2 + Far.R.In;
			Far.L.In = Far.R.In = 0.0f;
			Far.L.Y2 = Far.L.Y1;
			Far.L.Y1 = Yl0;
			Far.R.Y2 = Far.R.Y1;
			Far.R.Y1 = Yr0;
			Lp += K * (Yl0 - Lp);
			Rp += K * (Yr0 - Rp);
			const float Yl = Lp * G, Yr = Rp * G;
			Ml[I] += Yl;
			Mr[I] += Yr;
			Snd[I] += (Yl + Yr) * 0.35f;
			Pw += 0.5f * (Yl * Yl + Yr * Yr);
		}
		Far.LpL = Lp + 1.0e-18f;
		Far.LpR = Rp + 1.0e-18f;
		Far.Ringing -= Block;
		if (Far.Ringing <= 0)
		{
			Far.L.Clear();
			Far.R.Clear();
		}
		Power[LChips] += Pw * Inv;
	}
	Far.Queued = 0;
}

void CardRoom::Impl::RenderClaps()
{
	using namespace room_audio;
	// Claps: each hand-voice from its clapper's spot.
	float PwRail = 0.0f, PwTalk = 0.0f;
	for (int K = 0; K < MaxClaps; ++K)
	{
		vox::ClapVoice& C = Claps[K];
		if (!C.Live)
		{
			continue;
		}
		const HandSpot& At = ClapAt[K];
		const float Lg = 0.5f * (Gain[At.Layer] + NextGain[At.Layer]) * 0.5f;
		vox::Biquad Band = C.Band;
		float Env = C.Env, Click = C.Click;
		const float Decay = C.Decay, Cg = C.Gain * Lg, Gl = C.L, Gr = C.R, Gs = At.GS;
		uint32_t Ns = RailRng.S;
		float Pw = 0.0f;
		float* SS_VOX_RESTRICT Ml = MixL;
		float* SS_VOX_RESTRICT Mr = MixR;
		float* SS_VOX_RESTRICT Snd = Send48;
		for (int I = ClapStart[K]; I < Block; ++I)
		{
			Ns ^= Ns << 13;
			Ns ^= Ns >> 17;
			Ns ^= Ns << 5;
			const float Noise = static_cast<float>(Ns >> 8) * (2.0f / 16777216.0f) - 1.0f;
			const float Y = (Band.Run(Noise) * Env * 2.2f + Noise * Click * 0.25f) * Cg;
			Env *= Decay;
			Click *= 0.6f;
			Ml[I] += Y * Gl;
			Mr[I] += Y * Gr;
			Snd[I] += Y * Gs;
			Pw += Y * Y;
		}
		RailRng.S = Ns;
		C.Band.Z1 = Band.Z1;
		C.Band.Z2 = Band.Z2;
		C.Env = Env;
		C.Click = Click;
		(At.Layer == LRail ? PwRail : PwTalk) += Pw * 0.5f * (Gl * Gl + Gr * Gr);
		ClapStart[K] = 0;
		if (C.Env < 1.0e-4f)
		{
			C.Live = false;
			C.Band.Clear();
		}
	}
	Power[LRail] += PwRail / static_cast<float>(Block);
	Power[LTalk] += PwTalk / static_cast<float>(Block);
}

void CardRoom::Impl::Finish()
{
	using namespace room_audio;
	const float Inv = 1.0f / static_cast<float>(Block);
	// The meter (before muffling: what the room is, not what Focus lets through).
	const float Smooth = std::exp(-BlockSeconds / 0.45f);
	for (int L = 0; L < LCount; ++L)
	{
		Smoothed[L] = Smoothed[L] * Smooth + Power[L] * (1.0f - Smooth);
	}
	// Muffling: skipped while the room is clear; when it engages, its filters start where a steady input would have
	// left them, so nothing clicks.
	const float MG0 = MuffleGain;
	const float MG1 = vox::DbToGain(MuffleDb);
	MuffleGain = MG1;
	if (MuffleHz >= 19500.0f && MG0 == 1.0f && MG1 == 1.0f)
	{
		MuffleIdle = true;
	}
	else
	{
		if (MuffleIdle)
		{
			for (int S = 0; S < 2; ++S)
			{
				vox::Biquad& F = S == 0 ? MuffleL : MuffleR;
				const float X = S == 0 ? MixL[0] : MixR[0];
				F.Z2 = (F.B2 - F.A2) * X;
				F.Z1 = (F.B1 - F.A1) * X + F.Z2;
			}
			MuffleIdle = false;
		}
		const float Dm = (MG1 - MG0) * Inv;
		vox::Biquad Fl = MuffleL, Fr = MuffleR;
		float Mg = MG0;
		float* SS_VOX_RESTRICT Ml = MixL;
		float* SS_VOX_RESTRICT Mr = MixR;
		for (int I = 0; I < Block; ++I)
		{
			Ml[I] = Fl.Run(Ml[I]) * Mg;
			Mr[I] = Fr.Run(Mr[I]) * Mg;
			Mg += Dm;
		}
		MuffleL.Z1 = Fl.Z1;
		MuffleL.Z2 = Fl.Z2;
		MuffleR.Z1 = Fr.Z1;
		MuffleR.Z2 = Fr.Z2;
	}
	if (Terrace.Exists)
	{
		// The wind comes in at the terrace doors, past the muffle (it's out there with you).
		Terrace.Gust += (Terrace.GustTarget - Terrace.Gust) * 0.02f;
		const float K = vox::OnePoleK(250.0f + 700.0f * Terrace.Gust, Rate48);
		const float Level = 0.2f * Terrace.Gust * (Terrace.Open ? 1.0f : 0.3f);
		float A = Terrace.Lp1, B = Terrace.Lp2;
		uint32_t Ns = Terrace.R.S;
		for (int I = 0; I < Block; ++I)
		{
			Ns ^= Ns << 13;
			Ns ^= Ns >> 17;
			Ns ^= Ns << 5;
			A += K * ((static_cast<float>(Ns >> 8) * (2.0f / 16777216.0f) - 1.0f) - A);
			B += K * (A - B);
			Mono48[I] = B * Level;
		}
		Terrace.R.S = Ns;
		Terrace.Lp1 = A + 1.0e-18f;
		Terrace.Lp2 = B + 1.0e-18f;
		float Unused[Block];
		std::memset(Unused, 0, sizeof(Unused));
		Power[LBed] += Spatial(Terrace.Where, Mono48, Block, Terrace.Where.K48, Gain[LBed], NextGain[LBed], MixL, MixR, Unused) * Inv;
	}
	// Out, through a soft knee above 0.9: the room never clips.
	float Out = 0.0f;
	float Peak = 0.0f;
	for (int I = 0; I < Block; ++I)
	{
		const float L = MixL[I];
		const float R = MixR[I];
		Out += L * L + R * R;
		Peak = std::max(Peak, std::max(std::fabs(L), std::fabs(R)));
		Fifo[2 * I] = L;
		Fifo[2 * I + 1] = R;
	}
	Out *= 0.5f;
	if (Peak > 0.9f)
	{
		for (int I = 0; I < 2 * Block; ++I)
		{
			const float V = Fifo[I];
			if (std::fabs(V) > 0.9f)
			{
				Fifo[I] = (V > 0.0f ? 1.0f : -1.0f) * (0.9f + 0.1f * std::tanh((std::fabs(V) - 0.9f) * 10.0f));
			}
		}
	}
	OutPower = OutPower * Smooth + Out * Inv * (1.0f - Smooth);
}

void CardRoom::Impl::RenderBlock()
{
	using namespace room_audio;
	Control();
	std::memset(BusL, 0, sizeof(BusL));
	std::memset(BusR, 0, sizeof(BusR));
	std::memset(Send16, 0, sizeof(Send16));
	std::memset(MixL, 0, sizeof(MixL));
	std::memset(MixR, 0, sizeof(MixR));
	std::memset(Send48, 0, sizeof(Send48));
	std::memset(LowL, 0, sizeof(LowL));
	std::memset(LowR, 0, sizeof(LowR));
	std::memset(LowSend, 0, sizeof(LowSend));
	for (float& Pw : Power)
	{
		Pw = 0.0f;
	}
	Speaking = 0;
	// 8 kHz: the air, the walla, the slot floor, the room's talkers and the rail. 16 kHz: clear lines and the PA.
	// 48 kHz: chips, cards, claps, the bar and the cage.
	RenderAir();
	RenderWalla();
	RenderSlots();
	RenderVoices();
	RenderFoley();
	RenderClaps();
	// The reverb's send down to 8 kHz (averaged: the reverb's damping takes what little folds over), the reverb, and
	// the slow buses up to 48 kHz (the 16 kHz one only while it carries something, and a block after).
	for (int I = 0; I < Low; ++I)
	{
		const float* S48 = Send48 + 6 * I;
		LowSend[I] += (Send16[2 * I] + Send16[2 * I + 1]) * 0.5f + (S48[0] + S48[1] + S48[2] + S48[3] + S48[4] + S48[5]) * (1.0f / 6.0f);
	}
	Reverb.Process(LowSend, LowL, LowR, Low);
	UpLow.Process(0, LowL, MixL);
	UpLow.Process(1, LowR, MixR);
	if (Bus16Hot > 0)
	{
		Up.Process(0, BusL, MixL);
		Up.Process(1, BusR, MixR);
		--Bus16Hot;
	}
	Finish();
	Frame += Block;
	Now = static_cast<double>(Frame) / static_cast<double>(Rate48);
	EventNow += static_cast<double>(BlockSeconds * Pace);
	FifoAt = 0;
}

int CardRoom::Impl::Bytes() const
{
	size_t B = sizeof(Impl);
	B += Emitters.capacity() * sizeof(Emitter);
	B += Talk.capacity() * sizeof(room_audio::Ambient);
	B += (Reverb.Buf.capacity() + Reverb.Ap.capacity()) * sizeof(float);
	B += static_cast<size_t>(Pa.LineBytes());
	for (const room_audio::LineSlot& L : Lines)
	{
		B += L.Plan.capacity() * sizeof(Syllable);
	}
	return static_cast<int>(B);
}

// ------------------------------------------------------------------ the public face

CardRoom::CardRoom(uint32_t InSeed) : P(new Impl(InSeed))
{
}

CardRoom::~CardRoom() = default;
CardRoom::CardRoom(CardRoom&& Other) noexcept = default;
CardRoom& CardRoom::operator=(CardRoom&& Other) noexcept = default;

void CardRoom::SetEmitters(const std::vector<Emitter>& InEmitters)
{
	// A place that isn't a number (a host's bug) would ring through the reverb for the rest of the night: it's left out.
	const std::vector<Emitter>* List = &InEmitters;
	std::vector<Emitter> Finite;
	for (const Emitter& E : InEmitters)
	{
		if (!std::isfinite(E.X) || !std::isfinite(E.Y))
		{
			for (const Emitter& F : InEmitters)
			{
				if (std::isfinite(F.X) && std::isfinite(F.Y))
				{
					Finite.push_back(F);
				}
			}
			List = &Finite;
			break;
		}
	}
	const std::vector<Emitter>& Emitters = *List;
	bool Same = Emitters.size() == P->Emitters.size();
	for (size_t I = 0; Same && I < Emitters.size(); ++I)
	{
		const Emitter& A = Emitters[I];
		const Emitter& B = P->Emitters[I];
		Same = A.Kind == B.Kind && A.Id == B.Id && A.X == B.X && A.Y == B.Y && A.Open == B.Open && A.Players == B.Players;
	}
	if (Same)
	{
		return;
	}
	P->Emitters = Emitters;
	P->Rebuild();
}

void CardRoom::SetListener(float X, float Y, float Yaw)
{
	if (!std::isfinite(X) || !std::isfinite(Y) || !std::isfinite(Yaw))
	{
		return;
	}
	const float Rad = std::fmod(Yaw, 360.0f) * (vox::Pi / 180.0f);
	const float C = std::cos(Rad), S = std::sin(Rad);
	if (X == P->Ear.X && Y == P->Ear.Y && C == P->Ear.Cos && S == P->Ear.Sin)
	{
		return;
	}
	P->Ear.X = X;
	P->Ear.Y = Y;
	P->Ear.Cos = C;
	P->Ear.Sin = S;
	P->Moved = true;
}

void CardRoom::SetMix(float Bed, float Talk, float Chips, float Calls, float Rail, float Crowd)
{
	const float In[5] = {Bed, Talk, Chips, Calls, Rail};
	for (int L = 0; L < 5; ++L)
	{
		if (!std::isfinite(In[L]))
		{
			continue; // keep the level it had
		}
		const float T = room_audio::Clamp(In[L], -120.0f, 24.0f);
		if (std::fabs(T - P->Target[L]) > 0.001f)
		{
			// A new level: ease there in 1.5 s from wherever the old ease had got to.
			P->Target[L] = T;
			P->Speed[L] = std::max(1.0f, std::fabs(T - P->Db[L]) / 1.5f);
		}
	}
	const float C = std::isfinite(Crowd) ? room_audio::Clamp(Crowd, 0.0f, 1.0f) : P->CrowdTarget;
	if (std::fabs(C - P->CrowdTarget) > 0.001f)
	{
		P->CrowdTarget = C;
		P->CrowdSpeed = std::max(0.01f, std::fabs(C - P->CrowdNow) / 1.5f);
	}
}

void CardRoom::SetMuffle(float CutoffHz, float Db)
{
	if (std::isfinite(CutoffHz))
	{
		P->MuffleTargetHz = room_audio::Clamp(CutoffHz, 200.0f, 20000.0f);
	}
	if (std::isfinite(Db))
	{
		P->MuffleTargetDb = room_audio::Clamp(Db, -60.0f, 6.0f);
	}
}

void CardRoom::SetPace(float EventClock)
{
	if (std::isfinite(EventClock))
	{
		P->Pace = room_audio::Clamp(EventClock, 0.45f, 1.5f);
	}
}

void CardRoom::Duck(float Db, float Seconds)
{
	if (!std::isfinite(Db) || !std::isfinite(Seconds))
	{
		return;
	}
	const float D = std::min(60.0f, std::fabs(Db));
	P->AddDuck(0.06f, std::max(0.0f, Seconds), 0.5f, D * 0.5f, D, D * 0.5f);
}

void CardRoom::TableHand(int Id, float Seconds, bool AllIn, bool Called, float PotBb)
{
	// The hand plays where the table is: on the feature table when it's there and open, else at its slot.
	room_audio::TableState* Slot = nullptr;
	for (int I = 0; I < P->TableCount; ++I)
	{
		room_audio::TableState& T = P->Tables[I];
		if (T.Id != Id)
		{
			continue;
		}
		if (T.Feature && T.Open)
		{
			Slot = &T;
			break;
		}
		if (!Slot)
		{
			Slot = &T;
		}
	}
	// Your own table (Players 0) is the table actor's to sound: nothing here, not even the far scatter.
	if (Slot && Slot->Players > 0)
	{
		P->BuildHand(*Slot, std::isfinite(Seconds) ? Seconds : 45.0f, AllIn, Called, std::isfinite(PotBb) ? std::max(0.0f, PotBb) : 0.0f);
	}
}

void CardRoom::Say(const SpeechPlan& Plan, const Timbre& T, Spot Where, int Id, float Effort, bool Murmur)
{
	if (Plan.Syllables.empty())
	{
		return;
	}
	for (const Syllable& S : Plan.Syllables)
	{
		// A plan that isn't numbers (PlanSpeech never makes one) would poison the stream for the night: it isn't said.
		if (!std::isfinite(S.Start) || !std::isfinite(S.Length) || !std::isfinite(S.F1) || !std::isfinite(S.F2) || !std::isfinite(S.F3) ||
			!std::isfinite(S.Pitch) || !std::isfinite(S.Gain) || !std::isfinite(S.Air) || !std::isfinite(S.Locus) || !std::isfinite(S.CodaLocus) ||
			!std::isfinite(S.G1) || !std::isfinite(S.G2) || S.Start < 0.0f || S.Start > 600.0f)
		{
			return;
		}
	}
	// A free line, or the oldest.
	room_audio::LineSlot* Slot = nullptr;
	for (room_audio::LineSlot& L : P->Lines)
	{
		if (!L.Active || !L.Voice.Active())
		{
			Slot = &L;
			break;
		}
		if (!Slot || L.Order < Slot->Order)
		{
			Slot = &L;
		}
	}
	room_audio::LineSlot& L = *Slot;
	L.Plan.assign(Plan.Syllables.begin(), Plan.Syllables.end());
	L.Pa = Where == Spot::Pa;
	L.Order = ++P->Order;
	L.Active = true;
	L.Near = false;
	const float E = std::isfinite(Effort) ? room_audio::Clamp(Effort, 0.0f, 1.2f) : 0.6f;
	L.Gain = vox::DbToGain((E - 0.6f) * 18.0f) * (L.Pa ? 1.5f : 1.3f);
	L.Layer = L.Pa ? room_audio::LPa : Where == Spot::Rail ? room_audio::LRail : (Murmur ? room_audio::LTalk : room_audio::LCalls);
	L.Low = Murmur && !L.Pa;
	if (L.Low && P->Budget == 0)
	{
		// The fallback (§4.10): no formant talk in the room, so a murmured line is only a soft breath under its caption.
		for (Syllable& S : L.Plan)
		{
			S.Air = 1.0f;
			S.Gain *= 0.7f;
		}
		L.Gain *= 0.5f;
	}
	L.Voice.Start(L.Plan.data(), static_cast<int>(L.Plan.size()), T, L.Low, E, vox::Mix32(P->Seed, 0x5a1d0000u + ++P->Said), L.Low ? room_audio::Rate8 : room_audio::Rate16);
	const float Seconds = std::isfinite(Plan.Seconds) ? Plan.Seconds : 0.0f;
	const float Hold = std::max(0.2f, Seconds - 0.1f);
	if (L.Pa)
	{
		// The PA ducks the room's talk: a milestone deeper and longer (spec §4.9). Not a silenced PA (SetPaLevel -120,
		// the host's Voices slider at 0): the room doesn't step back for a voice nobody hears.
		if (P->PaGainTarget <= 0.0f)
		{
			return;
		}
		if (E >= 0.8f)
		{
			P->AddDuck(0.1f, Hold, 1.2f, 0.0f, 6.0f, 3.0f);
		}
		else
		{
			P->AddDuck(0.1f, Hold, 0.8f, 0.0f, 2.0f, 0.0f);
		}
		return;
	}
	P->PlaceFor(Where, Id, L.Where, 0.0f);
	if ((Where == Spot::Table || Where == Spot::FeatureTable) && L.Where.Dist <= 3.0f)
	{
		// Speech at your table: the room steps back for it (spec §4.9), and the line itself stays where it was.
		L.Near = true;
		P->AddDuck(0.06f, Hold, 0.5f, 3.0f, 6.0f, 2.0f);
	}
}

void CardRoom::Crowd(Vocal V, int Voices, Spot Where, int Id, float Level)
{
	if (V == Vocal::None || Voices <= 0 || !std::isfinite(Level))
	{
		return;
	}
	const float Lv = room_audio::Clamp(Level, 0.0f, 2.0f);
	// From the PA's "spot": the whole room, all around the listener (applause all around, a cheer from everywhere).
	const bool Around = Where == Spot::Pa;
	const int Layer = (Around || Where == Spot::Rail || Where == Spot::Bar) ? room_audio::LRail : room_audio::LTalk;
	vox::Rng& R = P->RailRng;
	// Each of the crowd in their own slice of the circle, so the room is even around the listener.
	auto AllAround = [&](room_audio::Place& Pl, int Index, int Count) {
		const float Angle = (static_cast<float>(Index) + R.Uniform()) / static_cast<float>(std::max(1, Count)) * vox::TwoPi;
		const float Out = R.Range(300.0f, 1000.0f);
		Pl.At(P->Ear.X + Out * std::cos(Angle), P->Ear.Y + Out * std::sin(Angle));
		room_audio::Aim(Pl, P->Ear);
	};
	if (V == Vocal::Clap || V == Vocal::Applause)
	{
		room_audio::Place From;
		if (!Around)
		{
			P->PlaceFor(Where, Id, From, 0.0f);
		}
		const int Room = room_audio::MaxHands - P->HandCount;
		const int N = vox::SeedClappers(V, Voices, R, static_cast<float>(P->EventNow), P->Hands + P->HandCount, std::min(Room, std::min(Voices, 24)));
		// More people than hand-voices: louder, not denser.
		const float Many = std::sqrt(static_cast<float>(std::max(1, Voices)) / static_cast<float>(std::max(1, N)));
		for (int I = 0; I < N; ++I)
		{
			room_audio::HandSpot& H = P->HandAt[P->HandCount + I];
			if (Around)
			{
				AllAround(From, I, N);
			}
			H.GL = From.TL;
			H.GR = From.TR;
			H.GS = From.TS;
			H.Layer = Layer;
			P->Hands[P->HandCount + I].Gain *= Lv * Many * 0.35f;
		}
		P->HandCount += N;
		return;
	}
	// The rail's bank: up to 8 of the crowd get their own voice.
	const int N = std::min(Voices, room_audio::MaxRail);
	const float Many = std::sqrt(static_cast<float>(Voices) / static_cast<float>(N));
	const Timbre Base = TimbreFor(R.Chance(0.4f), 25 + R.Below(45), vox::Mix32(P->Seed, 0xc0d0u + ++P->Shouts));
	const float Spread = V == Vocal::Laugh ? 0.6f : V == Vocal::Cheer ? 0.4f : V == Vocal::Ooh ? 0.25f : 0.3f;
	const float Effort = V == Vocal::Cheer ? 1.0f : V == Vocal::Gasp ? 0.8f : V == Vocal::Laugh ? 0.65f : V == Vocal::Sigh ? 0.4f : 0.55f;
	for (int K = 0; K < N; ++K)
	{
		room_audio::RailSlot* Slot = nullptr;
		for (room_audio::RailSlot& S : P->Rail)
		{
			if (!S.Active)
			{
				Slot = &S;
				break;
			}
			if (!Slot || S.Order < Slot->Order)
			{
				Slot = &S;
			}
		}
		room_audio::RailSlot& S = *Slot;
		const Timbre Who = K == 0 ? Base : vox::CrowdMember(Base, R);
		S.Count = vox::VocalPlan(V, Who, R, 0.0f, S.Plan, 8);
		if (S.Count <= 0)
		{
			continue;
		}
		S.Order = ++P->Order;
		S.Active = true;
		S.Layer = Layer;
		S.Gain = Lv * Many * 1.4f;
		S.Delay = K == 0 ? 0 : static_cast<int>(R.Range(0.0f, Spread) * room_audio::Rate8);
		if (Around)
		{
			AllAround(S.Where, K, N);
		}
		else
		{
			P->PlaceFor(Where, Id, S.Where, Where == Spot::Rail ? 220.0f : 120.0f);
		}
		S.Voice.Start(S.Plan, S.Count, Who, true, Effort, vox::Mix32(P->Seed, 0xcafe0000u + ++P->Shouts), room_audio::Rate8);
	}
}

void CardRoom::SetPaLevel(float Db)
{
	if (!std::isfinite(Db))
	{
		return; // keep the level it had
	}
	const float D = room_audio::Clamp(Db, -120.0f, 12.0f);
	P->PaGainTarget = D <= -119.0f ? 0.0f : vox::DbToGain(D);
}

void CardRoom::SetVoiceBudget(int Talkers)
{
	P->Budget = std::max(0, std::min(Talkers, room_audio::MaxAmbient));
	P->Resize();
}

void CardRoom::RenderStereo(float* Out, int Frames)
{
	if (!Out || Frames <= 0)
	{
		return;
	}
	vox::DenormalGuard Guard;
	int Done = 0;
	while (Done < Frames)
	{
		if (P->FifoAt >= room_audio::Block)
		{
			P->RenderBlock();
		}
		const int Take = std::min(Frames - Done, room_audio::Block - P->FifoAt);
		std::memcpy(Out + 2 * Done, P->Fifo + 2 * P->FifoAt, sizeof(float) * 2 * static_cast<size_t>(Take));
		P->FifoAt += Take;
		Done += Take;
	}
}

CardRoom::Meter CardRoom::Measure() const
{
	Meter M;
	auto Db = [](float Pw) { return Pw <= 1.0e-12f ? -120.0f : 10.0f * std::log10(Pw); };
	M.BedDb = Db(P->Smoothed[room_audio::LBed]);
	M.TalkDb = Db(P->Smoothed[room_audio::LTalk]);
	M.ChipsDb = Db(P->Smoothed[room_audio::LChips]);
	M.CallsDb = Db(P->Smoothed[room_audio::LCalls]);
	M.PaDb = Db(P->Smoothed[room_audio::LPa]);
	M.RailDb = Db(P->Smoothed[room_audio::LRail]);
	M.OutDb = Db(P->OutPower);
	M.Talkers = P->Speaking;
	int Tables = 0;
	for (int I = 0; I < P->TableCount; ++I)
	{
		Tables += P->Tables[I].Voiced ? 1 : 0;
	}
	M.Tables = Tables;
	M.Voiced = M.Talkers + M.Tables;
	M.Bytes = P->Bytes();
	return M;
}
} // namespace audio
} // namespace ss
