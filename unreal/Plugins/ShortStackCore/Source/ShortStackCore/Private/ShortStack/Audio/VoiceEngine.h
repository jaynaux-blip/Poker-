#pragma once

// The voice engine behind Voice.cpp (one-shot renders on a worker) and CardRoom.cpp (the room's stream): a glottal
// source through a cascade of formant resonators, a frication branch, and the syllable machine that walks a plan
// (next_segment_spec §4.10). Private to ShortStackCore's audio.
//
// The rule (§8.3): every coefficient is computed per syllable segment or per event, never per sample. A segment's
// resonator targets are worked out once when it starts; between them the coefficients step every 2 ms by a delta fixed
// at the segment's start. The per-sample loops only run filters, a table lookup and a noise generator.
//
// Nothing here is shared between threads: the tables are built once (a function-local static) and only read after.

#include "ShortStack/Audio/Voice.h"

#include <cstdint>
#include <vector>

#if defined(_M_X64) || defined(_M_IX86) || defined(__SSE__) || defined(__x86_64__)
#include <xmmintrin.h>
#define SS_VOX_FTZ 1
#else
#define SS_VOX_FTZ 0
#endif

/** No-alias buffers in the hot loops (MSVC, Clang and GCC all take __restrict). */
#define SS_VOX_RESTRICT __restrict

namespace ss
{
namespace audio
{
namespace vox
{
constexpr float Pi = 3.14159265358979f;
constexpr float TwoPi = 6.28318530717959f;
/** The engine's output rate: audio::SampleRate (Synth.h, which this header doesn't pull in; Synth.cpp asserts they match). */
constexpr float OutRate = 48000.0f;

/**
 * Flushes denormals to zero while a render runs (a resonator ringing down to 1e-40 costs a hundred times a normal
 * sample on x86), and restores the thread's mode after: the game thread's floating point is left as it was found.
 */
struct DenormalGuard
{
#if SS_VOX_FTZ
	unsigned int Saved;
	DenormalGuard() : Saved(_mm_getcsr()) { _mm_setcsr(Saved | 0x8040u); }
	~DenormalGuard() { _mm_setcsr(Saved); }
#else
	DenormalGuard() {}
#endif
	DenormalGuard(const DenormalGuard&) = delete;
	DenormalGuard& operator=(const DenormalGuard&) = delete;
};

/** Mixes two words into a seed (a murmur-style finalizer): every random stream here is keyed by ids, never a clock. */
uint32_t Mix32(uint32_t A, uint32_t B);

/** xorshift32 with a scrambled seed. */
struct Rng
{
	uint32_t S = 1u;
	void Seed(uint32_t Value)
	{
		S = Mix32(Value, 0x68e31da4u);
		if (S == 0u)
		{
			S = 0x9e3779b9u;
		}
	}
	uint32_t Next()
	{
		S ^= S << 13;
		S ^= S >> 17;
		S ^= S << 5;
		return S;
	}
	/** 0 .. 1 (exclusive). */
	float Uniform() { return static_cast<float>(Next() >> 8) * (1.0f / 16777216.0f); }
	/** -1 .. 1. */
	float Signed() { return Uniform() * 2.0f - 1.0f; }
	float Range(float A, float B) { return A + (B - A) * Uniform(); }
	int Below(int N) { return N <= 1 ? 0 : static_cast<int>(Uniform() * static_cast<float>(N)) % N; }
	bool Chance(float P) { return Uniform() < P; }
	/** About normal, mean 0, deviation 1 (the sum of four uniforms). */
	float Gauss() { return (Uniform() + Uniform() + Uniform() + Uniform() - 2.0f) * 1.7320508f; }
};

float DbToGain(float Db);
float GainToDb(float Gain);
/** The one-pole low-pass coefficient for a cutoff. */
float OnePoleK(float Hz, float Rate);

/** A Klatt resonator: unity gain at DC, so a cascade keeps the formants' natural levels. */
struct Reso
{
	float A = 1.0f;
	float B = 0.0f;
	float C = 0.0f;
	float Y1 = 0.0f;
	float Y2 = 0.0f;
	void Set(float Hz, float Bw, float Rate);
	float Run(float X)
	{
		const float Y = A * X + B * Y1 + C * Y2;
		Y2 = Y1;
		Y1 = Y;
		return Y;
	}
	void Clear() { Y1 = Y2 = 0.0f; }
};

/** RBJ biquad, transposed direct form II. BandPass has a peak gain of 1. */
struct Biquad
{
	float B0 = 1.0f;
	float B1 = 0.0f;
	float B2 = 0.0f;
	float A1 = 0.0f;
	float A2 = 0.0f;
	float Z1 = 0.0f;
	float Z2 = 0.0f;
	void LowPass(float Hz, float Q, float Rate);
	void HighPass(float Hz, float Q, float Rate);
	void BandPass(float Hz, float Q, float Rate);
	void HighShelf(float Hz, float Db, float Rate);
	void Peak(float Hz, float Q, float Db, float Rate);
	float Run(float X)
	{
		const float Y = B0 * X + Z1;
		Z1 = B1 * X - A1 * Y + Z2;
		Z2 = B2 * X - A2 * Y;
		return Y;
	}
	void Clear() { Z1 = Z2 = 0.0f; }
};

/** Consonant classes on Syllable::Onset and Syllable::Coda (the place is in Locus / CodaLocus, the F2 locus in Hz). */
enum Manner : int
{
	MNone = 0,
	MVoicelessStop = 1, // p t k
	MVoicedStop = 2,    // b d g
	MSibilant = 3,      // s
	MShibilant = 4,     // sh ch
	MWeakFricative = 5, // f th
	MAspirate = 6,      // h
	MNasal = 7,         // m n ng
	MLateral = 8,       // l
	MRhotic = 9,        // r
	MGlide = 10,        // w y (y when the locus is high)
	MVoicedFricative = 11, // z v j
};

/** Vowel classes (Peterson and Barney's men; a timbre's Tract scales them). Diphthongs glide to G1/G2. */
enum Vowel : int
{
	VIY,
	VIH,
	VEH,
	VAE,
	VAA,
	VAO,
	VUH,
	VUW,
	VAH,
	VER,
	VAX,
	VEY,
	VAY,
	VOW,
	VAW,
	VOY,
	VHum, // a closed-mouth hum ("mm")
	VCount,
};
struct VowelTarget
{
	float F1, F2, F3, G1, G2;
};
const VowelTarget& VowelOf(int V);
/** The F2 locus a consonant at Place (0 lips, 1 teeth/ridge, 2 palate, 3 velum, 4 glottis) gives before vowel F2. */
float LocusFor(int Place, float VowelF2);

/** One syllable as text or a generator sees it, before timing and pitch. */
struct SylSpec
{
	uint8_t V = VAX;
	uint8_t Onset = MNone;
	uint8_t Coda = MNone;
	uint8_t OnPlace = 1;
	uint8_t CodaPlace = 1;
	bool Stressed = false;
	bool Function = false;
	bool WordEnd = false;
};
enum class PhraseEnd : int
{
	Comma,
	Stop,
	Question,
	Exclaim,
	Ellipsis,
	Open, // babble: trails off, no pause after
};
/**
 * Times and pitches one intonation phrase (declination, accents with downstep, final lengthening, the fall, the
 * question's rise) and writes Count syllables to Out from StartTime. Returns the time the phrase's last syllable ends.
 */
float ProsodyPhrase(const SylSpec* Specs, int Count, PhraseEnd End, const Timbre& T, float Effort, Rng& R, float StartTime, Syllable* Out);
/** The pause after a phrase, in seconds at the timbre's rate. */
float PauseAfter(PhraseEnd End, const Timbre& T, Rng& R);
/** A babble phrase for the room's ambient talkers: Count syllables of no language, from StartTime. Returns its end. */
float BabblePhrase(int Count, bool Question, const Timbre& T, float Effort, Rng& R, float StartTime, Syllable* Out);
/** A non-word vocal for one person (Laugh .. Cheer). Returns the syllable count (at most Max). */
int VocalPlan(Vocal V, const Timbre& T, Rng& R, float StartTime, Syllable* Out, int Max);
/** A person in a crowd near Base: their own pitch and tract, a little apart. */
Timbre CrowdMember(const Timbre& Base, Rng& R);

/**
 * One voice. Start it with a plan's syllables (they must outlive the voice), render it in any block sizes; it walks
 * the syllables segment by segment and goes quiet after the last one.
 */
class Talker
{
public:
	void Start(const Syllable* Syllables, int Count, const Timbre& T, bool Murmur, float Effort, uint32_t Seed, float Rate);
	/** Writes N mono samples (zeros once it's done). */
	void Render(float* Out, int N);
	bool Active() const { return Live; }
	void Stop() { Live = false; }
	/** Seconds since Start. */
	float Elapsed() const { return Clock; }
	/** Points the voice at a new array holding the same syllables (the owner moved them). */
	void Rebind(const Syllable* Syllables) { Syl = Syllables; }

private:
	struct Seg
	{
		int Len = 0;
		float F1 = 500.0f, F2 = 1500.0f, F3 = 2500.0f;
		float Bw = 1.0f;
		float Voice = 0.0f, Asp = 0.0f, Fric = 0.0f;
		float JumpFric = -1.0f;
		float FricHz = 0.0f, FricQ = 1.0f;
		float Creak = 0.0f;
	};
	void BuildSyllable();
	void BeginSegment();
	void StepCoefficients();
	void Period(float At);
	template <bool WithFric, bool WithF4>
	void RenderRun(float* SS_VOX_RESTRICT Out, int N);

	const Syllable* Syl = nullptr;
	int Count = 0;
	int Index = 0;
	bool Live = false;
	bool Murmur = false;
	Timbre Voice;
	float Effort = 0.6f;
	float Rate = 48000.0f;
	float InvRate = 1.0f / 48000.0f;
	float Clock = 0.0f;
	long long Sample = 0;
	Rng R;

	Seg Segs[10];
	int SegCount = 0;
	bool FricOn = false;
	int AnchorAt = 0;
	int SegAt = 0;
	int SegLeft = 0;
	int SubLen = 96;
	int SubLeft = 0;
	int StepsLeft = 0;

	float VoiceAmp = 0.0f, AspAmp = 0.0f, FricAmp = 0.0f;
	float DVoice = 0.0f, DAsp = 0.0f, DFric = 0.0f;

	Reso F[4];
	float TargetB[3] = {0, 0, 0};
	float TargetC[3] = {0, 0, 0};
	float StepB[3] = {0, 0, 0};
	float StepC[3] = {0, 0, 0};
	float Bw[3] = {1.0f, 1.0f, 1.0f};
	Biquad FricBand;
	Biquad Muffle; // the tone: a murmur's low-pass, or a clear voice's presence shelf

	const float* Table = nullptr;
	float OpenQ = 0.56f;
	float Phase = 0.0f;
	float Inc = 0.0f;
	float PeriodAmp = 1.0f;
	float Flutter = 0.0f;
	float FlutterTarget = 0.0f;
	float FlutterNext = 0.0f;
	float AnchorT0 = 0.0f, AnchorP0 = 1.0f, AnchorT1 = 0.0f, AnchorP1 = 1.0f;
	float Creak = 0.0f;
	float Gate = 0.35f; // the breath's level while the folds are closed (1: a whisper, nothing pulses it)
	float OutGain = 1.0f;
	float NoiseState = 0.0f;
	float NoiseScale = 1.0f;
};

/** The floor's PA (§4.4): 250 Hz to 5 kHz through a small horn, the ceiling's speakers at 25, 55 and 90 ms. Mono, in place. */
class PaStage
{
public:
	void Init(float Rate, uint32_t Seed);
	void Process(float* X, int N);
	/** How long the taps ring after the input ends. */
	float TailSeconds() const { return 0.3f; }
	/** The speakers' delay line, for the room's memory meter. */
	int LineBytes() const { return static_cast<int>(Line.capacity() * sizeof(int16_t)); }

private:
	Biquad Hp[2], Lp[2], Horn;
	float Tap[3] = {0, 0, 0};
	float TapGain[3] = {0, 0, 0};
	int TapLen[3] = {0, 0, 0};
	float TapLp[3] = {0, 0, 0};
	float TapK[3] = {0, 0, 0};
	std::vector<int16_t> Line; // 16-bit: the speakers' band needs no more
	int Pos = 0;
};

/** People clapping (Clap, Applause): each clapper keeps their own pace, cupping and place in the crowd. */
struct Clapper
{
	float Next = 0.0f;   // seconds, on the owner's clock
	float Period = 0.25f;
	float Until = 0.0f;
	float Hz = 1400.0f;
	float Q = 1.6f;
	float Gain = 0.0f;
	float Pan = 0.0f;    // -1 left .. 1 right, around the spot
	float Start = 0.0f;
	float Swell = 3.0f;  // the applause's length, for its envelope
};
/** Seeds Count clappers for a Clap (light, about 2 s) or Applause (3.2 s, up to 6 s for a big crowd) from At. */
int SeedClappers(Vocal V, int Count, Rng& R, float At, Clapper* Out, int Max);
/** The gain of one clap from this clapper at time T (0 once they've stopped). */
float ClapGain(const Clapper& C, float T);
/** One clap: a noise burst through the hands' cupping. Renders N samples into Out (added). */
struct ClapVoice
{
	Biquad Band;
	float Env = 0.0f;
	float Decay = 0.99f;
	float Click = 0.0f;
	float Gain = 0.0f;
	float L = 0.0f, R = 0.0f;
	bool Live = false;
	void Strike(float Hz, float Q, float InGain, float Seconds, float Rate);
};
} // namespace vox
} // namespace audio
} // namespace ss
