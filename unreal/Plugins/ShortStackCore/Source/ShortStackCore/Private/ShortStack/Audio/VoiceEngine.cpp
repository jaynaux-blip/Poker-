#include "VoiceEngine.h"
#include "../StrictFloat.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>

// The voice engine (next_segment_spec §4.10): a Klatt-style cascade synthesizer driven syllable by syllable, the
// prosody that times and pitches a phrase, babble and non-word vocals for the room, the PA's chain and the clappers.
// audio-synth.

namespace ss
{
namespace audio
{
namespace vox
{
uint32_t Mix32(uint32_t A, uint32_t B)
{
	uint32_t H = A * 0x9e3779b1u ^ (B + 0x7f4a7c15u + (A << 6) + (A >> 2));
	H ^= H >> 16;
	H *= 0x85ebca6bu;
	H ^= H >> 13;
	H *= 0xc2b2ae35u;
	H ^= H >> 16;
	return H;
}

float DbToGain(float Db)
{
	return Db <= -119.0f ? 0.0f : std::pow(10.0f, Db / 20.0f);
}

float GainToDb(float Gain)
{
	return Gain <= 1.0e-6f ? -120.0f : 20.0f * std::log10(Gain);
}

float OnePoleK(float Hz, float Rate)
{
	const float H = std::min(std::max(Hz, 1.0f), Rate * 0.49f);
	return 1.0f - std::exp(-TwoPi * H / Rate);
}

void Reso::Set(float Hz, float BwHz, float Rate)
{
	const float Fh = std::min(std::max(Hz, 40.0f), Rate * 0.45f);
	const float R = std::exp(-Pi * std::max(BwHz, 10.0f) / Rate);
	C = -R * R;
	B = 2.0f * R * std::cos(TwoPi * Fh / Rate);
	A = 1.0f - B - C;
}

namespace
{
struct Rbj
{
	float Cw, Alpha;
	Rbj(float Hz, float Q, float Rate)
	{
		const float W = TwoPi * std::min(std::max(Hz, 10.0f), Rate * 0.45f) / Rate;
		Cw = std::cos(W);
		Alpha = std::sin(W) / (2.0f * std::max(Q, 0.05f));
	}
};
} // namespace

void Biquad::LowPass(float Hz, float Q, float Rate)
{
	const Rbj K(Hz, Q, Rate);
	const float A0 = 1.0f + K.Alpha;
	B0 = (1.0f - K.Cw) * 0.5f / A0;
	B1 = (1.0f - K.Cw) / A0;
	B2 = B0;
	A1 = -2.0f * K.Cw / A0;
	A2 = (1.0f - K.Alpha) / A0;
}

void Biquad::HighPass(float Hz, float Q, float Rate)
{
	const Rbj K(Hz, Q, Rate);
	const float A0 = 1.0f + K.Alpha;
	B0 = (1.0f + K.Cw) * 0.5f / A0;
	B1 = -(1.0f + K.Cw) / A0;
	B2 = B0;
	A1 = -2.0f * K.Cw / A0;
	A2 = (1.0f - K.Alpha) / A0;
}

void Biquad::BandPass(float Hz, float Q, float Rate)
{
	const Rbj K(Hz, Q, Rate);
	const float A0 = 1.0f + K.Alpha;
	B0 = K.Alpha / A0;
	B1 = 0.0f;
	B2 = -K.Alpha / A0;
	A1 = -2.0f * K.Cw / A0;
	A2 = (1.0f - K.Alpha) / A0;
}

void Biquad::HighShelf(float Hz, float Db, float Rate)
{
	const float W = TwoPi * std::min(std::max(Hz, 10.0f), Rate * 0.45f) / Rate;
	const float Amp = std::pow(10.0f, Db / 40.0f);
	const float Cw = std::cos(W);
	const float Alpha = std::sin(W) * 0.70710678f;
	const float Root = 2.0f * std::sqrt(Amp) * Alpha;
	const float A0 = (Amp + 1.0f) - (Amp - 1.0f) * Cw + Root;
	B0 = Amp * ((Amp + 1.0f) + (Amp - 1.0f) * Cw + Root) / A0;
	B1 = -2.0f * Amp * ((Amp - 1.0f) + (Amp + 1.0f) * Cw) / A0;
	B2 = Amp * ((Amp + 1.0f) + (Amp - 1.0f) * Cw - Root) / A0;
	A1 = 2.0f * ((Amp - 1.0f) - (Amp + 1.0f) * Cw) / A0;
	A2 = ((Amp + 1.0f) - (Amp - 1.0f) * Cw - Root) / A0;
}

void Biquad::Peak(float Hz, float Q, float Db, float Rate)
{
	const Rbj K(Hz, Q, Rate);
	const float Amp = std::pow(10.0f, Db / 40.0f);
	const float A0 = 1.0f + K.Alpha / Amp;
	B0 = (1.0f + K.Alpha * Amp) / A0;
	B1 = -2.0f * K.Cw / A0;
	B2 = (1.0f - K.Alpha * Amp) / A0;
	A1 = B1;
	A2 = (1.0f - K.Alpha / Amp) / A0;
}

// ------------------------------------------------------------------ vowels and consonants

const VowelTarget& VowelOf(int V)
{
	// Peterson and Barney (1952), men; the diphthongs glide from their first vowel toward G1/G2.
	static const VowelTarget Table[VCount] = {
		{270, 2290, 3010, 0, 0},     // IY  beet
		{390, 1990, 2550, 0, 0},     // IH  bit
		{530, 1840, 2480, 0, 0},     // EH  bet
		{660, 1720, 2410, 0, 0},     // AE  bat
		{730, 1090, 2440, 0, 0},     // AA  father
		{570, 840, 2410, 0, 0},      // AO  bought
		{440, 1020, 2240, 0, 0},     // UH  book
		{300, 870, 2240, 0, 0},      // UW  boot
		{640, 1190, 2390, 0, 0},     // AH  but
		{490, 1350, 1690, 0, 0},     // ER  bird
		{500, 1500, 2500, 0, 0},     // AX  the schwa
		{480, 1950, 2600, 330, 2200},  // EY  bait
		{720, 1150, 2450, 400, 1950},  // AY  bite
		{520, 920, 2400, 340, 850},    // OW  boat
		{720, 1150, 2450, 450, 1000},  // AW  bout
		{570, 860, 2410, 360, 1950},   // OY  boy
		{260, 1150, 2400, 0, 0},     // a closed-mouth hum
	};
	return Table[V < 0 || V >= VCount ? VAX : V];
}

float LocusFor(int Place, float VowelF2)
{
	// Locus equations (Sussman): the consonant's F2 onset is a line in the vowel's F2.
	switch (Place)
	{
	case 0: return 0.85f * VowelF2 + 150.0f;
	case 1: return 0.42f * VowelF2 + 1200.0f;
	case 2: return 0.30f * VowelF2 + 1550.0f;
	case 3: return 0.70f * VowelF2 + 650.0f;
	default: return VowelF2;
	}
}

namespace
{
bool LongVowel(int V)
{
	return V == VIY || V == VUW || V == VAO || V == VER || V >= VEY;
}

bool ShortVowel(int V)
{
	return V == VIH || V == VEH || V == VUH || V == VAX;
}

float Clamp(float X, float Lo, float Hi)
{
	return X < Lo ? Lo : X > Hi ? Hi : X;
}

/** The syllable a SylSpec becomes, before timing and pitch: formants scaled to the tract, reduced when unstressed. */
void Shape(const SylSpec& S, const Timbre& T, float Effort, Rng& R, Syllable& Out)
{
	const VowelTarget& V = VowelOf(S.V);
	const VowelTarget& Schwa = VowelOf(VAX);
	const float Reduce = S.Stressed ? 0.0f : (S.Function ? 0.5f : 0.35f);
	auto Mixed = [&](float A, float B) { return A + (B - A) * Reduce; };
	const float Tr = T.Tract;
	const float Wobble = 1.0f + 0.03f * R.Signed();
	Out.F1 = (Mixed(V.F1, Schwa.F1) + 90.0f * (Effort - 0.6f)) * Tr * (1.0f + 0.03f * R.Signed());
	Out.F2 = Mixed(V.F2, Schwa.F2) * Tr * Wobble;
	Out.F3 = Mixed(V.F3, Schwa.F3) * Tr * (1.0f + 0.02f * R.Signed());
	Out.G1 = V.G1 > 0.0f ? Mixed(V.G1, Schwa.F1) * Tr : 0.0f;
	Out.G2 = V.G2 > 0.0f ? Mixed(V.G2, Schwa.F2) * Tr * Wobble : 0.0f;
	Out.Onset = S.Onset;
	Out.Coda = S.Coda;
	Out.Locus = S.Onset != MNone ? LocusFor(S.OnPlace, Out.F2 / Tr) * Tr : 0.0f;
	Out.CodaLocus = S.Coda != MNone ? LocusFor(S.CodaPlace, (Out.G2 > 0.0f ? Out.G2 : Out.F2) / Tr) * Tr : 0.0f;
	Out.Air = 0.0f;
	if (S.V == VHum)
	{
		Out.F1 = V.F1 * Tr;
		Out.F2 = V.F2 * Tr;
		Out.F3 = V.F3 * Tr;
	}
}
} // namespace

float ProsodyPhrase(const SylSpec* Specs, int Count, PhraseEnd End, const Timbre& T, float Effort, Rng& R, float StartTime, Syllable* Out)
{
	if (Count <= 0)
	{
		return StartTime;
	}
	const float Base = 1.0f / std::max(2.5f, T.Rate) * (1.0f + 0.3f * std::max(0.0f, Effort - 0.7f));
	float Total = 0.0f;
	for (int I = 0; I < Count; ++I)
	{
		const SylSpec& S = Specs[I];
		float D = S.Stressed ? 1.18f : (S.Function ? 0.72f : 0.84f);
		D *= LongVowel(S.V) ? 1.1f : ShortVowel(S.V) ? 0.92f : 1.0f;
		D *= 1.0f + (S.Onset != MNone ? 0.04f : 0.0f) + (S.Coda != MNone ? 0.06f : 0.0f);
		D *= Clamp(1.0f + 0.08f * R.Gauss(), 0.8f, 1.2f);
		if (I == Count - 1 && End != PhraseEnd::Open)
		{
			D *= End == PhraseEnd::Question ? 1.45f : 1.32f; // the phrase's last syllable is drawn out
		}
		Out[I].Length = D;
		Total += D;
	}
	// The timbre's rate holds over the phrase, its drawn-out ending included.
	const float Scale = Base * static_cast<float>(Count) / Total;
	for (int I = 0; I < Count; ++I)
	{
		Out[I].Length *= Scale;
	}

	const float Range = (1.0f + (Effort - 0.6f) * 0.8f) * (End == PhraseEnd::Exclaim ? 1.4f : 1.0f);
	const float Lift = 1.0f + 0.18f * (Effort - 0.6f) + (End == PhraseEnd::Exclaim ? 0.05f : 0.0f);
	int Accents = 0;
	float T0 = StartTime;
	for (int I = 0; I < Count; ++I)
	{
		const SylSpec& S = Specs[I];
		Syllable& Y = Out[I];
		const float Length = Y.Length;
		Shape(S, T, Effort, R, Y);
		Y.Start = T0;
		Y.Length = Length;
		T0 += Length;
		const float Along = Count > 1 ? static_cast<float>(I) / static_cast<float>(Count - 1) : 0.0f;
		float P = 1.06f - 0.12f * Along; // declination
		if (S.Stressed && !S.Function)
		{
			P += 0.13f * Range * std::pow(0.78f, static_cast<float>(Accents));
			++Accents;
		}
		else
		{
			P -= 0.015f;
		}
		float G = S.Stressed ? 1.0f : (S.Function ? 0.6f : 0.74f);
		if (I == Count - 1)
		{
			switch (End)
			{
			case PhraseEnd::Stop:
			case PhraseEnd::Ellipsis:
				P = std::min(P, 0.84f + 0.02f * R.Signed());
				G *= 0.85f;
				Y.Air = 0.12f;
				break;
			case PhraseEnd::Comma:
				P = 1.0f - 0.12f * Along + 0.05f;
				break;
			case PhraseEnd::Question:
				P = 1.0f + 0.27f * Range;
				break;
			case PhraseEnd::Exclaim:
				P = std::min(P, 0.92f);
				break;
			case PhraseEnd::Open:
				break;
			}
		}
		else if (I == Count - 2 && End == PhraseEnd::Question)
		{
			P = std::max(P, 1.02f);
		}
		Y.Pitch = P * Lift * (1.0f + 0.02f * R.Gauss());
		Y.Gain = G * (0.55f + 0.75f * Effort) * Clamp(1.0f + 0.06f * R.Gauss(), 0.85f, 1.15f);
		Y.Open = Clamp((Y.F1 / T.Tract - 230.0f) / 520.0f, 0.06f, 1.0f) * (0.7f + 0.3f * std::min(1.0f, G));
	}
	return T0;
}

float PauseAfter(PhraseEnd End, const Timbre& T, Rng& R)
{
	const float Slow = 4.6f / std::max(2.5f, T.Rate);
	switch (End)
	{
	case PhraseEnd::Comma: return R.Range(0.17f, 0.26f) * Slow;
	case PhraseEnd::Stop: return R.Range(0.30f, 0.44f) * Slow;
	case PhraseEnd::Question:
	case PhraseEnd::Exclaim: return R.Range(0.32f, 0.46f) * Slow;
	case PhraseEnd::Ellipsis: return R.Range(0.50f, 0.65f) * Slow;
	case PhraseEnd::Open: return 0.0f;
	}
	return 0.0f;
}

namespace
{
/** Picks from a table of weights. */
int Pick(Rng& R, const float* Weights, int N)
{
	float Sum = 0.0f;
	for (int I = 0; I < N; ++I)
	{
		Sum += Weights[I];
	}
	float X = R.Uniform() * Sum;
	for (int I = 0; I < N; ++I)
	{
		X -= Weights[I];
		if (X < 0.0f)
		{
			return I;
		}
	}
	return N - 1;
}

void BabbleSpecs(int Count, Rng& R, SylSpec* Out)
{
	// English-like proportions: stressed syllables take the full vowels, the rest lean to the schwa and IH.
	static const float StressedV[VCount] = {8, 9, 9, 9, 7, 4, 2, 5, 8, 5, 0, 7, 7, 5, 2, 1, 0};
	static const float WeakV[VCount] = {6, 14, 4, 2, 1, 1, 1, 2, 4, 4, 30, 2, 2, 2, 0, 0, 0};
	// Onsets: none, p/t/k, b/d/g, s, sh, f/th, h, m/n, l, r, w/y, z/v.
	static const float Onsets[12] = {14, 20, 12, 10, 3, 7, 5, 13, 6, 5, 5, 2};
	static const float Codas[12] = {44, 16, 4, 9, 1, 3, 0, 13, 5, 4, 0, 3};
	int I = 0;
	while (I < Count)
	{
		const float W = R.Uniform();
		int Word = W < 0.55f ? 1 : W < 0.86f ? 2 : 3;
		Word = std::min(Word, Count - I);
		const bool Function = Word == 1 && R.Chance(0.45f);
		const int Stress = Word == 1 ? 0 : (R.Chance(0.8f) ? 0 : 1);
		for (int K = 0; K < Word; ++K)
		{
			SylSpec& S = Out[I + K];
			S.Stressed = !Function && K == Stress;
			S.Function = Function;
			S.WordEnd = K == Word - 1;
			S.V = static_cast<uint8_t>(Pick(R, S.Stressed ? StressedV : WeakV, VCount));
			S.Onset = static_cast<uint8_t>(Pick(R, Onsets, 12));
			S.Coda = static_cast<uint8_t>(K == Word - 1 ? Pick(R, Codas, 12) : (R.Chance(0.25f) ? Pick(R, Codas, 12) : 0));
			S.OnPlace = static_cast<uint8_t>(R.Below(4));
			S.CodaPlace = static_cast<uint8_t>(R.Below(4));
			if (S.Onset == MAspirate)
			{
				S.OnPlace = 4;
			}
		}
		I += Word;
	}
}
} // namespace

float BabblePhrase(int Count, bool Question, const Timbre& T, float Effort, Rng& R, float StartTime, Syllable* Out)
{
	SylSpec Specs[32];
	Count = std::max(1, std::min(Count, 32));
	BabbleSpecs(Count, R, Specs);
	// A longer phrase sometimes breaks in two with a breath between.
	if (Count >= 8 && R.Chance(0.45f))
	{
		const int Split = Count / 2 - 1 + R.Below(3);
		float End = ProsodyPhrase(Specs, Split, PhraseEnd::Comma, T, Effort, R, StartTime, Out);
		End += PauseAfter(PhraseEnd::Comma, T, R) * 0.7f;
		return ProsodyPhrase(Specs + Split, Count - Split, Question ? PhraseEnd::Question : PhraseEnd::Stop, T, Effort, R, End, Out + Split);
	}
	const PhraseEnd E = Question ? PhraseEnd::Question : (R.Chance(0.3f) ? PhraseEnd::Comma : PhraseEnd::Stop);
	return ProsodyPhrase(Specs, Count, E, T, Effort, R, StartTime, Out);
}

namespace
{
Syllable VocalSyllable(int V, const Timbre& T, float Start, float Length, float Pitch, float Gain, float Air)
{
	const VowelTarget& X = VowelOf(V);
	Syllable S;
	S.Start = Start;
	S.Length = Length;
	S.F1 = X.F1 * T.Tract;
	S.F2 = X.F2 * T.Tract;
	S.F3 = X.F3 * T.Tract;
	S.G1 = X.G1 * T.Tract;
	S.G2 = X.G2 * T.Tract;
	S.Pitch = Pitch;
	S.Gain = Gain;
	S.Air = Air;
	S.Open = Clamp((X.F1 - 230.0f) / 520.0f, 0.06f, 1.0f);
	return S;
}
} // namespace

int VocalPlan(Vocal V, const Timbre& T, Rng& R, float StartTime, Syllable* Out, int Max)
{
	int N = 0;
	auto Add = [&](const Syllable& S) {
		if (N < Max)
		{
			Out[N++] = S;
		}
	};
	float At = StartTime;
	switch (V)
	{
	case Vocal::Laugh:
	{
		// Ha-ha-ha: breathy syllables through an open vowel, each lower and softer, sometimes a breath in after.
		const int Vowel = R.Chance(0.5f) ? VAA : (R.Chance(0.5f) ? VAH : VEH);
		const int Count = 4 + R.Below(4);
		float Pitch = R.Range(1.35f, 1.6f);
		float Gain = 1.0f;
		for (int I = 0; I < Count && N < Max - 1; ++I)
		{
			const float Len = R.Range(0.1f, 0.15f);
			Syllable S = VocalSyllable(Vowel, T, At, Len, Pitch, Gain, 0.32f);
			S.Onset = MAspirate;
			S.Locus = S.F2;
			Add(S);
			At += Len + R.Range(0.015f, 0.05f);
			Pitch *= R.Range(0.9f, 0.96f);
			Gain *= R.Range(0.86f, 0.95f);
		}
		if (R.Chance(0.5f))
		{
			Syllable In = VocalSyllable(VAA, T, At + 0.05f, 0.24f, 1.0f, 0.3f, 1.0f);
			Add(In);
		}
		break;
	}
	case Vocal::Sigh:
	{
		Syllable A = VocalSyllable(VAH, T, At, R.Range(0.45f, 0.6f), R.Range(1.05f, 1.15f), 0.75f, 0.72f);
		A.Onset = MAspirate;
		A.Locus = A.F2;
		Add(A);
		Add(VocalSyllable(VAX, T, At + A.Length, R.Range(0.5f, 0.7f), R.Range(0.74f, 0.82f), 0.45f, 0.85f));
		break;
	}
	case Vocal::Scoff:
	{
		Syllable Hum = VocalSyllable(VHum, T, At, R.Range(0.12f, 0.17f), R.Range(0.92f, 1.0f), 0.6f, 0.3f);
		Hum.Open = 0.05f;
		Add(Hum);
		Syllable Puff = VocalSyllable(VAX, T, At + Hum.Length + 0.01f, R.Range(0.1f, 0.14f), 0.9f, 0.5f, 1.0f);
		Puff.Onset = MWeakFricative;
		Puff.Locus = 900.0f * T.Tract;
		Puff.Open = 0.15f;
		Add(Puff);
		break;
	}
	case Vocal::Groan:
	{
		Syllable A = VocalSyllable(VAH, T, At, R.Range(0.4f, 0.55f), R.Range(0.98f, 1.06f), 0.8f, 0.2f);
		Add(A);
		Syllable B = VocalSyllable(VAO, T, At + A.Length, R.Range(0.5f, 0.75f), R.Range(0.74f, 0.82f), 0.55f, 0.28f);
		Add(B);
		break;
	}
	case Vocal::Ooh:
	{
		// Ooooh: swells over about 1.2 s, rising then falling.
		const float L = R.Range(0.38f, 0.5f);
		Add(VocalSyllable(VUH, T, At, L, 1.0f, 0.45f, 0.25f));
		Add(VocalSyllable(VUW, T, At + L, L, R.Range(1.18f, 1.3f), 1.0f, 0.2f));
		Add(VocalSyllable(VUW, T, At + 2.0f * L, R.Range(0.45f, 0.6f), R.Range(1.0f, 1.08f), 0.6f, 0.3f));
		break;
	}
	case Vocal::Gasp:
	{
		// A quick breath in, then a sharp "oh!".
		Add(VocalSyllable(VAA, T, At, 0.1f, 1.0f, 0.45f, 1.0f));
		Syllable Oh = VocalSyllable(VOW, T, At + 0.12f, R.Range(0.26f, 0.34f), R.Range(1.45f, 1.65f), 1.0f, 0.3f);
		Add(Oh);
		break;
	}
	case Vocal::Cheer:
	{
		// "Yeah!" and "woo!", one to three of them.
		const int Count = 1 + R.Below(3);
		for (int I = 0; I < Count && N < Max; ++I)
		{
			const bool Woo = R.Chance(0.4f);
			const float Len = Woo ? R.Range(0.45f, 0.65f) : R.Range(0.35f, 0.5f);
			Syllable S = VocalSyllable(Woo ? VUW : VEH, T, At, Len, Woo ? R.Range(1.6f, 1.85f) : R.Range(1.45f, 1.7f), 1.0f, 0.22f);
			S.Onset = MGlide;
			S.Locus = (Woo ? 760.0f : 2200.0f) * T.Tract;
			if (!Woo)
			{
				S.G1 = 700.0f * T.Tract; // "yeah" opens toward AE
				S.G2 = 1700.0f * T.Tract;
			}
			Add(S);
			At += Len + R.Range(0.2f, 0.5f);
		}
		break;
	}
	default:
		break;
	}
	return N;
}

Timbre CrowdMember(const Timbre& Base, Rng& R)
{
	Timbre M = Base;
	const bool BaseWoman = Base.Tract > 1.08f;
	if (R.Chance(0.35f))
	{
		// Somebody of the other voice.
		if (BaseWoman)
		{
			M.F0 = R.Range(95.0f, 145.0f);
			M.Tract = R.Range(0.96f, 1.04f);
		}
		else
		{
			M.F0 = R.Range(175.0f, 240.0f);
			M.Tract = R.Range(1.12f, 1.2f);
		}
	}
	else
	{
		M.F0 = Base.F0 * R.Range(0.85f, 1.2f);
		M.Tract = Base.Tract * R.Range(0.95f, 1.06f);
	}
	M.Rate = Base.Rate * R.Range(0.9f, 1.1f);
	M.Breath = Clamp(Base.Breath + R.Range(-0.05f, 0.1f), 0.05f, 0.45f);
	M.Rasp = Clamp(R.Range(0.0f, 0.45f), 0.0f, 1.0f);
	return M;
}

// ------------------------------------------------------------------ the glottal source

namespace
{
constexpr int TableSize = 512;
struct GlottalTables
{
	float T[4][TableSize + 1];
	float OpenQ[4];
	GlottalTables()
	{
		// Rosenberg's flow derivative: from pressed (short open phase, sharp closure) to breathy (long, soft closure).
		const float Tp[4] = {0.34f, 0.40f, 0.44f, 0.48f};
		const float Tn[4] = {0.10f, 0.16f, 0.22f, 0.30f};
		const int Smooth[4] = {0, 1, 3, 6};
		float Tmp[TableSize];
		for (int K = 0; K < 4; ++K)
		{
			OpenQ[K] = Tp[K] + Tn[K];
			for (int I = 0; I < TableSize; ++I)
			{
				const float X = static_cast<float>(I) / static_cast<float>(TableSize);
				float D = 0.0f;
				if (X < Tp[K])
				{
					D = Pi / (2.0f * Tp[K]) * std::sin(Pi * X / Tp[K]);
				}
				else if (X < Tp[K] + Tn[K])
				{
					D = -Pi / (2.0f * Tn[K]) * std::sin(Pi * (X - Tp[K]) / (2.0f * Tn[K]));
				}
				T[K][I] = D;
			}
			// The softer voices: a circular smoothing tilts the spectrum down.
			for (int Pass = 0; Pass < Smooth[K]; ++Pass)
			{
				for (int I = 0; I < TableSize; ++I)
				{
					Tmp[I] = 0.25f * T[K][(I + TableSize - 1) % TableSize] + 0.5f * T[K][I] + 0.25f * T[K][(I + 1) % TableSize];
				}
				for (int I = 0; I < TableSize; ++I)
				{
					T[K][I] = Tmp[I];
				}
			}
			// A share of the flow itself under its derivative: the radiated fundamental a recorded voice keeps (without
			// it the first harmonics sit 5 to 14 dB under a real voice's long-term spectrum).
			float Flow[TableSize];
			float Sum = 0.0f;
			for (int I = 0; I < TableSize; ++I)
			{
				Sum += T[K][I] * (TwoPi / static_cast<float>(TableSize));
				Flow[I] = Sum;
			}
			float FlowMean = 0.0f;
			for (int I = 0; I < TableSize; ++I)
			{
				FlowMean += Flow[I];
			}
			FlowMean /= static_cast<float>(TableSize);
			for (int I = 0; I < TableSize; ++I)
			{
				T[K][I] += 2.5f * (Flow[I] - FlowMean);
			}
			float Mean = 0.0f;
			for (int I = 0; I < TableSize; ++I)
			{
				Mean += T[K][I];
			}
			Mean /= static_cast<float>(TableSize);
			float Power = 0.0f;
			for (int I = 0; I < TableSize; ++I)
			{
				T[K][I] -= Mean;
				Power += T[K][I] * T[K][I];
			}
			const float Norm = 0.5f / std::sqrt(Power / static_cast<float>(TableSize));
			for (int I = 0; I < TableSize; ++I)
			{
				T[K][I] *= Norm;
			}
			T[K][TableSize] = T[K][0];
		}
	}
};

const GlottalTables& Glottis()
{
	static const GlottalTables Tables;
	return Tables;
}

float BurstHz(float Locus, float Tract)
{
	const float L = Locus / std::max(0.5f, Tract);
	return L < 1250.0f ? 1100.0f : L < 1900.0f ? 3900.0f : 2500.0f;
}

float NasalF2(float Locus, float Tract)
{
	return Clamp(Locus * 0.6f + 400.0f * Tract, 1000.0f * Tract, 1900.0f * Tract);
}
} // namespace

// ------------------------------------------------------------------ the talker

void Talker::Start(const Syllable* Syllables, int InCount, const Timbre& T, bool InMurmur, float InEffort, uint32_t Seed, float InRate)
{
	Syl = Syllables;
	Count = Syllables ? std::max(0, InCount) : 0;
	Index = 0;
	Live = Count > 0;
	Murmur = InMurmur;
	// A timbre or an effort that isn't a number (a host's bug) gets the default voice, not a stream of NaNs.
	const Timbre Plain;
	auto Sane = [](float X, float Default) { return std::isfinite(X) ? X : Default; };
	Voice = T;
	Voice.F0 = Clamp(Sane(T.F0, Plain.F0), 60.0f, 400.0f);
	Voice.Tract = Clamp(Sane(T.Tract, Plain.Tract), 0.8f, 1.35f);
	Voice.Breath = Clamp(Sane(T.Breath, Plain.Breath), 0.0f, 1.0f);
	Voice.Rasp = Clamp(Sane(T.Rasp, Plain.Rasp), 0.0f, 1.0f);
	Voice.Rate = Sane(T.Rate, Plain.Rate);
	Effort = Clamp(Sane(InEffort, 0.6f), 0.0f, 1.2f);
	Rate = InRate;
	InvRate = 1.0f / InRate;
	Clock = 0.0f;
	Sample = 0;
	R.Seed(Seed);
	SubLen = std::max(8, static_cast<int>(Rate * 0.002f));
	NoiseScale = std::sqrt(Rate / 48000.0f);
	SubLeft = 0;
	StepsLeft = 0;
	SegCount = 0;
	SegAt = 0;
	SegLeft = 0;
	FricOn = false;
	const float Shape = (Voice.Breath - 0.08f) / 0.12f + (0.6f - Effort) * 2.5f;
	const int K = static_cast<int>(Clamp(std::floor(Shape + 0.5f), 0.0f, 3.0f));
	Table = Glottis().T[K];
	OpenQ = Glottis().OpenQ[K];
	Phase = 0.0f;
	VoiceAmp = AspAmp = FricAmp = 0.0f;
	DVoice = DAsp = DFric = 0.0f;
	const Syllable First = Count > 0 ? Syl[0] : Syllable();
	const float Init[3] = {First.F1, First.F2, First.F3};
	for (int I = 0; I < 3; ++I)
	{
		F[I].Set(Init[I], 90.0f + 40.0f * static_cast<float>(I), Rate);
		F[I].Clear();
		TargetB[I] = F[I].B;
		TargetC[I] = F[I].C;
		StepB[I] = StepC[I] = 0.0f;
	}
	F[3].Set(3500.0f * Voice.Tract, 260.0f, Rate);
	F[3].Clear();
	FricBand.Clear();
	FricBand.BandPass(4000.0f, 1.0f, Rate);
	Muffle.Clear();
	if (Murmur)
	{
		Muffle.LowPass(2100.0f, 0.6f, Rate);
	}
	else
	{
		Muffle.HighShelf(1800.0f, 15.0f, Rate);
	}
	Inc = Voice.F0 * First.Pitch * InvRate;
	PeriodAmp = 1.0f;
	Flutter = FlutterTarget = 0.0f;
	FlutterNext = 0.0f;
	AnchorAt = 0;
	AnchorT0 = 0.0f;
	AnchorP0 = First.Pitch;
	AnchorT1 = First.Start + First.Length * 0.5f;
	AnchorP1 = First.Pitch;
	Creak = 0.0f;
	Gate = 0.35f;
	// A cascade's gain grows with the formants' sharpness; this keeps a talked /a/ near half scale.
	OutGain = Murmur ? 0.11f : 0.085f;
	NoiseState = 0.0f;
}

void Talker::BuildSyllable()
{
	SegCount = 0;
	SegAt = 0;
	if (Index >= Count)
	{
		Live = false;
		return;
	}
	const Syllable& S = Syl[Index];
	const Syllable* Next = Index + 1 < Count ? &Syl[Index + 1] : nullptr;
	++Index;
	const float Tr = Voice.Tract;
	const float FricScale = Murmur ? 0.3f : 0.45f;

	Seg Last;

	Last.F1 = S.F1;
	Last.F2 = S.F2;
	Last.F3 = S.F3;
	Last.Voice = VoiceAmp;
	Last.Asp = AspAmp / NoiseScale; // the running amplitude is in the rate's units, a segment's are not
	Last.Fric = 0.0f;
	double Cursor = static_cast<double>(Sample);
	auto Push = [&](double Seconds) -> Seg& {
		Seg& G = Segs[std::min(SegCount, 9)];
		G = Last;
		G.JumpFric = -1.0f;
		G.FricHz = 0.0f;
		G.Creak = 0.0f;
		const double End = Cursor + std::max(0.0, Seconds) * static_cast<double>(Rate);
		G.Len = std::max(1, static_cast<int>(std::floor(End + 0.5) - std::floor(Cursor + 0.5)));
		Cursor = End;
		if (SegCount < 10)
		{
			++SegCount;
		}
		return G;
	};
	auto Done = [&](Seg& G) { Last = G; };

	const double S0 = static_cast<double>(S.Start) * static_cast<double>(Rate);
	const double S1 = static_cast<double>(S.Start + S.Length) * static_cast<double>(Rate);
	if (S0 > Cursor + 1.0)
	{
		// Silence until the syllable (the previous one already let go).
		Seg& G = Push((S0 - Cursor) / static_cast<double>(Rate));
		G.Voice = G.Asp = G.Fric = 0.0f;
		Done(G);
	}
	const float L = static_cast<float>(std::max(1.0, S1 - std::max(S0, Cursor)) / static_cast<double>(Rate));
	const bool Liquid = S.Onset == MLateral || S.Onset == MRhotic || S.Onset == MGlide;
	float On = S.Onset != MNone ? Clamp(0.28f * L, 0.035f, 0.085f) : 0.0f;
	float Co = S.Coda != MNone ? Clamp(0.2f * L, 0.03f, 0.075f) : 0.0f;
	float TrIn = S.Onset != MNone ? std::min(Liquid ? 0.07f : 0.05f, 0.25f * L) : std::min(0.03f, 0.15f * L);
	float Vo = L - On - Co - TrIn;
	if (Vo < 0.25f * L)
	{
		const float Squeeze = 0.75f * L / std::max(0.001f, On + Co + TrIn);
		On *= Squeeze;
		Co *= Squeeze;
		TrIn *= Squeeze;
		Vo = L - On - Co - TrIn;
	}
	const float G = S.Gain;
	const float Air = Clamp(S.Air, 0.0f, 1.0f);
	const float VoiceFull = G * (1.0f - 0.88f * Air);
	const float AspFull = G * (Voice.Breath * 0.3f + Air * 0.85f);
	const float Lc = S.Locus > 0.0f ? S.Locus : S.F2;

	switch (S.Onset)
	{
	case MVoicelessStop:
	{
		Seg& A = Push(On * 0.62f);
		A.F1 = 230.0f * Tr;
		A.F2 = Lc;
		A.Voice = A.Asp = A.Fric = 0.0f;
		Done(A);
		Seg& B = Push(On * 0.13f);
		B.JumpFric = 0.9f * G * FricScale;
		B.Fric = 0.15f * G * FricScale;
		B.FricHz = BurstHz(Lc, Tr);
		B.FricQ = 1.2f;
		B.Asp = 0.5f * G;
		Done(B);
		Seg& C = Push(On * 0.25f);
		C.F1 = (230.0f * Tr + S.F1) * 0.5f;
		C.F2 = (Lc + S.F2) * 0.5f;
		C.Fric = 0.0f;
		C.Asp = 0.45f * G;
		C.Voice = 0.1f * G;
		Done(C);
		break;
	}
	case MVoicedStop:
	{
		Seg& A = Push(On * 0.65f);
		A.F1 = 200.0f * Tr;
		A.F2 = Lc;
		A.Voice = 0.12f * G;
		A.Asp = A.Fric = 0.0f;
		Done(A);
		Seg& B = Push(On * 0.35f);
		B.JumpFric = 0.4f * G * FricScale;
		B.Fric = 0.0f;
		B.FricHz = BurstHz(Lc, Tr);
		B.FricQ = 1.2f;
		B.Voice = 0.5f * G;
		B.F1 = 200.0f * Tr + (S.F1 - 200.0f * Tr) * 0.3f;
		B.F2 = Lc + (S.F2 - Lc) * 0.3f;
		Done(B);
		break;
	}
	case MSibilant:
	case MShibilant:
	case MWeakFricative:
	case MVoicedFricative:
	{
		const bool Weak = S.Onset == MWeakFricative;
		const bool Voiced = S.Onset == MVoicedFricative;
		const float Hz = S.Onset == MShibilant ? 3000.0f : Weak ? 4500.0f : (Lc / Tr < 1300.0f ? 4200.0f : 5600.0f);
		const float Level = (Weak ? 0.22f : Voiced ? 0.35f : 0.7f) * G * FricScale;
		Seg& A = Push(On * 0.3f);
		A.F1 = 300.0f * Tr;
		A.F2 = Lc;
		A.Voice = Voiced ? 0.45f * G : 0.0f;
		A.Asp = 0.0f;
		A.Fric = Level;
		A.FricHz = Hz;
		A.FricQ = Weak ? 0.6f : (S.Onset == MShibilant ? 1.4f : 1.6f);
		Done(A);
		Seg& B = Push(On * 0.7f);
		B.Fric = Level * 0.85f;
		B.FricHz = Hz;
		B.FricQ = A.FricQ;
		Done(B);
		break;
	}
	case MAspirate:
	{
		Seg& A = Push(On);
		A.F1 = S.F1;
		A.F2 = S.F2;
		A.F3 = S.F3;
		A.Voice = 0.05f * G;
		A.Asp = 0.75f * G + AspFull;
		A.Fric = 0.0f;
		Done(A);
		break;
	}
	case MNasal:
	{
		Seg& A = Push(On);
		A.F1 = 270.0f * Tr;
		A.F2 = NasalF2(Lc, Tr);
		A.F3 = 2500.0f * Tr;
		A.Bw = 2.0f;
		A.Voice = 0.42f * G;
		A.Asp = 0.0f;
		A.Fric = 0.0f;
		Done(A);
		break;
	}
	case MLateral:
	case MRhotic:
	case MGlide:
	{
		Seg& A = Push(On);
		if (S.Onset == MLateral)
		{
			A.F1 = 360.0f * Tr;
			A.F2 = 1250.0f * Tr;
			A.F3 = 2750.0f * Tr;
		}
		else if (S.Onset == MRhotic)
		{
			A.F1 = 420.0f * Tr;
			A.F2 = 1150.0f * Tr;
			A.F3 = 1650.0f * Tr;
		}
		else if (Lc / Tr > 1700.0f)
		{
			A.F1 = 280.0f * Tr;
			A.F2 = 2200.0f * Tr;
			A.F3 = 2900.0f * Tr;
		}
		else
		{
			A.F1 = 310.0f * Tr;
			A.F2 = 760.0f * Tr;
			A.F3 = 2250.0f * Tr;
		}
		A.Voice = 0.62f * G;
		A.Asp = 0.0f;
		A.Fric = 0.0f;
		Done(A);
		break;
	}
	default:
		break;
	}

	// Into the vowel, through it (gliding if it's a diphthong), and out to the coda.
	{
		Seg& A = Push(TrIn);
		A.F1 = S.F1;
		A.F2 = S.F2;
		A.F3 = S.F3;
		A.Bw = 1.0f;
		A.Voice = VoiceFull;
		A.Asp = AspFull;
		A.Fric = 0.0f;
		Done(A);
		Seg& B = Push(Vo);
		B.F1 = S.G1 > 0.0f ? S.G1 : S.F1 * (1.0f + 0.02f * R.Signed());
		B.F2 = S.G2 > 0.0f ? S.G2 : S.F2 * (1.0f + 0.02f * R.Signed());
		B.Voice = VoiceFull * 0.88f;
		B.Asp = AspFull;
		Done(B);
	}
	const float Cl = S.CodaLocus > 0.0f ? S.CodaLocus : S.F2;
	switch (S.Coda)
	{
	case MVoicelessStop:
	case MVoicedStop:
	{
		const bool Voiced = S.Coda == MVoicedStop;
		Seg& A = Push(Co * 0.45f);
		A.F1 = 300.0f * Tr;
		A.F2 = Cl;
		A.Voice = (Voiced ? 0.35f : 0.25f) * G;
		Done(A);
		Seg& B = Push(Co * 0.55f);
		B.Voice = Voiced ? 0.08f * G : 0.0f;
		B.Asp = 0.0f;
		Done(B);
		break;
	}
	case MSibilant:
	case MShibilant:
	case MWeakFricative:
	case MVoicedFricative:
	{
		const bool Weak = S.Coda == MWeakFricative;
		const bool Voiced = S.Coda == MVoicedFricative;
		const float Hz = S.Coda == MShibilant ? 3000.0f : Weak ? 4500.0f : 5600.0f;
		Seg& A = Push(Co * 0.3f);
		A.F1 = 320.0f * Tr;
		A.F2 = Cl;
		A.Voice = 0.3f * G;
		Done(A);
		Seg& B = Push(Co * 0.7f);
		B.Voice = Voiced ? 0.35f * G : 0.0f;
		B.Asp = 0.0f;
		B.JumpFric = (Weak ? 0.1f : 0.35f) * G * FricScale;
		B.Fric = (Weak ? 0.2f : 0.55f) * G * FricScale;
		B.FricHz = Hz;
		B.FricQ = Weak ? 0.6f : 1.5f;
		Done(B);
		break;
	}
	case MNasal:
	{
		Seg& A = Push(Co);
		A.F1 = 270.0f * Tr;
		A.F2 = NasalF2(Cl, Tr);
		A.F3 = 2500.0f * Tr;
		A.Bw = 2.0f;
		A.Voice = 0.4f * G;
		A.Asp = 0.0f;
		Done(A);
		break;
	}
	case MLateral:
	case MRhotic:
	case MGlide:
	{
		Seg& A = Push(Co);
		A.F1 = (S.Coda == MRhotic ? 430.0f : 380.0f) * Tr;
		A.F2 = (S.Coda == MRhotic ? 1200.0f : 1100.0f) * Tr;
		A.F3 = (S.Coda == MRhotic ? 1700.0f : 2700.0f) * Tr;
		A.Voice = 0.55f * G;
		Done(A);
		break;
	}
	case MAspirate:
	{
		Seg& A = Push(Co);
		A.Voice = 0.2f * G;
		A.Asp = 0.3f * G;
		Done(A);
		break;
	}
	default:
		break;
	}
	// A pause or the end: let the voice go (a raspy voice creaks as it trails off).
	const double NextStart = Next ? static_cast<double>(Next->Start) * static_cast<double>(Rate) : -1.0;
	if (!Next || NextStart > Cursor + 0.03 * static_cast<double>(Rate))
	{
		const double Gap = Next ? (NextStart - Cursor) / static_cast<double>(Rate) : 0.08;
		Seg& A = Push(std::min(0.07, std::max(0.01, Gap)));
		A.Voice = 0.0f;
		A.Asp = 0.0f;
		A.Fric = 0.0f;
		A.Creak = Voice.Rasp > 0.25f ? Voice.Rasp : 0.0f;
		Done(A);
		if (!Next)
		{
			Seg& B = Push(0.04);
			Done(B);
		}
	}
	// A whispered syllable (Air 1: a breath in, a puff, the fallback's lines): no voice at all, its voicing turned to breath.
	Gate = 0.35f;
	if (Air >= 0.999f)
	{
		for (int K = 0; K < SegCount; ++K)
		{
			Segs[K].Asp += 0.6f * Segs[K].Voice;
			Segs[K].Voice = 0.0f;
		}
		Gate = 1.0f;
	}
	(void)Last;
}

void Talker::BeginSegment()
{
	const Seg& G = Segs[SegAt];
	SegLeft = std::max(1, G.Len);
	// Breath and frication are noise: at a lower rate the same amplitude is denser, so they scale with the rate (the
	// voice was set at 48 kHz) and a voice is as breathy on the room's 8 kHz bus as in a one-shot.
	const float Dense = NoiseScale;
	if (G.JumpFric >= 0.0f)
	{
		FricAmp = G.JumpFric * Dense;
	}
	const float Inv = 1.0f / static_cast<float>(SegLeft);
	DVoice = (G.Voice - VoiceAmp) * Inv;
	DAsp = (G.Asp * Dense - AspAmp) * Inv;
	DFric = (G.Fric * Dense - FricAmp) * Inv;
	FricOn = FricAmp > 0.0f || G.Fric > 0.0f;
	if (G.FricHz > 0.0f)
	{
		FricBand.BandPass(std::min(G.FricHz, Rate * 0.42f), G.FricQ, Rate);
	}
	Creak = G.Creak;
	const float Breathy = 1.0f + Voice.Breath * 0.6f;
	const float Hz[3] = {G.F1, G.F2, G.F3};
	// Formant bandwidths after Fant and Hawks & Miller: about 80 Hz at F1, 90 at F2, 150 at F3, wider when breathy.
	const float BwHz[3] = {(65.0f + 0.05f * G.F1) * G.Bw * Breathy, (60.0f + 0.03f * G.F2) * G.Bw * Breathy, (100.0f + 0.025f * G.F3) * G.Bw * Breathy};
	StepsLeft = std::max(1, (SegLeft + SubLen - 1) / SubLen);
	const float InvSteps = 1.0f / static_cast<float>(StepsLeft);
	for (int K = 0; K < 3; ++K)
	{
		Reso Target;
		Target.Set(Hz[K], BwHz[K], Rate);
		TargetB[K] = Target.B;
		TargetC[K] = Target.C;
		StepB[K] = (TargetB[K] - F[K].B) * InvSteps;
		StepC[K] = (TargetC[K] - F[K].C) * InvSteps;
	}
	SubLeft = 0;
}

void Talker::StepCoefficients()
{
	if (StepsLeft <= 0)
	{
		return;
	}
	--StepsLeft;
	for (int K = 0; K < 3; ++K)
	{
		if (StepsLeft == 0)
		{
			F[K].B = TargetB[K];
			F[K].C = TargetC[K];
		}
		else
		{
			F[K].B += StepB[K];
			F[K].C += StepC[K];
		}
		F[K].A = 1.0f - F[K].B - F[K].C;
	}
}

void Talker::Period(float At)
{
	// The pitch contour runs through each syllable's middle; flutter, jitter and shimmer keep it a person's.
	while (At >= AnchorT1 && AnchorAt + 1 < Count)
	{
		++AnchorAt;
		AnchorT0 = AnchorT1;
		AnchorP0 = AnchorP1;
		AnchorT1 = Syl[AnchorAt].Start + Syl[AnchorAt].Length * 0.5f;
		AnchorP1 = Syl[AnchorAt].Pitch;
	}
	const float Span = AnchorT1 - AnchorT0;
	const float U = Span > 1.0e-4f ? Clamp((At - AnchorT0) / Span, 0.0f, 1.0f) : 1.0f;
	const float P = AnchorP0 + (AnchorP1 - AnchorP0) * U;
	if (At >= FlutterNext)
	{
		FlutterTarget = R.Gauss() * 0.008f;
		FlutterNext = At + 0.15f + 0.15f * R.Uniform();
	}
	Flutter += (FlutterTarget - Flutter) * 0.15f;
	const float Jitter = 1.0f + R.Gauss() * (0.003f + 0.01f * Voice.Rasp);
	float F0 = Voice.F0 * P * (1.0f + Flutter) * Jitter;
	float Amp = 1.0f + R.Gauss() * (0.025f + 0.07f * Voice.Rasp);
	if (Creak > 0.0f)
	{
		F0 *= 1.0f - 0.4f * Creak;
		if (R.Uniform() < 0.3f * Creak)
		{
			F0 *= 0.6f;
		}
		Amp *= 1.0f - 0.3f * Creak * R.Uniform();
	}
	Inc = Clamp(F0, 50.0f, 600.0f) * InvRate;
	PeriodAmp = Clamp(Amp, 0.6f, 1.4f);
}

template <bool WithFric, bool WithF4>
void Talker::RenderRun(float* SS_VOX_RESTRICT Out, int N)
{
	// Everything the loop touches lives in locals, so the filters' recursions stay in registers.
	float Ph = Phase;
	float Step = Inc;
	float Amp = PeriodAmp;
	float Va = VoiceAmp;
	float Aa = AspAmp;
	float Fa = FricAmp;
	const float Dv = DVoice;
	const float Da = DAsp;
	const float Df = DFric;
	const float Oq = OpenQ;
	const float Gt = Gate;
	const float Gain = OutGain;
	const float* SS_VOX_RESTRICT Tab = Table;
	Reso R0 = F[0];
	Reso R1 = F[1];
	Reso R2 = F[2];
	Reso R3 = F[3];
	Biquad Fb = FricBand;
	Biquad Mf = Muffle;
	uint32_t Ns = R.S;
	for (int I = 0; I < N; ++I)
	{
		Ph += Step;
		if (Ph >= 1.0f)
		{
			Ph -= 1.0f;
			R.S = Ns;
			Period(Clock + static_cast<float>(I) * InvRate);
			Ns = R.S;
			Step = Inc;
			Amp = PeriodAmp;
		}
		const float X = Ph * static_cast<float>(TableSize);
		const int I0 = static_cast<int>(X);
		const float Fr = X - static_cast<float>(I0);
		const float Gl = Tab[I0] + (Tab[I0 + 1] - Tab[I0]) * Fr;
		Ns ^= Ns << 13;
		Ns ^= Ns >> 17;
		Ns ^= Ns << 5;
		const float Noise = static_cast<float>(Ns >> 8) * (2.0f / 16777216.0f) - 1.0f;
		const float Src = Va * Amp * Gl + Aa * Noise * (Ph < Oq ? 1.0f : Gt) + 1.0e-15f;
		float Y = R2.Run(R1.Run(R0.Run(Src)));
		if (WithF4)
		{
			Y = R3.Run(Y);
		}
		if (WithFric)
		{
			Y += Fb.Run(Noise) * Fa;
		}
		Y = Mf.Run(Y);
		Out[I] = Y * Gain;
		Va += Dv;
		Aa += Da;
		Fa += Df;
	}
	R.S = Ns;
	Phase = Ph;
	VoiceAmp = Va;
	AspAmp = Aa;
	FricAmp = Fa;
	F[0].Y1 = R0.Y1;
	F[0].Y2 = R0.Y2;
	F[1].Y1 = R1.Y1;
	F[1].Y2 = R1.Y2;
	F[2].Y1 = R2.Y1;
	F[2].Y2 = R2.Y2;
	F[3].Y1 = R3.Y1;
	F[3].Y2 = R3.Y2;
	FricBand.Z1 = Fb.Z1;
	FricBand.Z2 = Fb.Z2;
	Muffle.Z1 = Mf.Z1;
	Muffle.Z2 = Mf.Z2;
}

void Talker::Render(float* Out, int N)
{
	int I = 0;
	while (I < N)
	{
		if (!Live)
		{
			for (; I < N; ++I)
			{
				Out[I] = 0.0f;
			}
			return;
		}
		if (SegLeft <= 0)
		{
			++SegAt;
			if (SegAt >= SegCount)
			{
				BuildSyllable();
				if (!Live || SegCount == 0)
				{
					Live = false;
					continue;
				}
			}
			BeginSegment();
		}
		if (SubLeft <= 0)
		{
			StepCoefficients();
			SubLeft = SubLen;
		}
		const int Run = std::min(N - I, std::min(SegLeft, SubLeft));
		float* Dst = Out + I;
		if (Murmur)
		{
			if (FricOn)
			{
				RenderRun<true, false>(Dst, Run);
			}
			else
			{
				RenderRun<false, false>(Dst, Run);
			}
		}
		else if (FricOn)
		{
			RenderRun<true, true>(Dst, Run);
		}
		else
		{
			RenderRun<false, true>(Dst, Run);
		}
		I += Run;
		SegLeft -= Run;
		SubLeft -= Run;
		Clock += static_cast<float>(Run) * InvRate;
		Sample += Run;
	}
}

// ------------------------------------------------------------------ the PA

void PaStage::Init(float Rate, uint32_t Seed)
{
	Rng R;
	R.Seed(Seed ^ 0x5a17u);
	// 250 Hz to 5 kHz, fourth-order Butterworth each side, and the horn's bump.
	Hp[0].HighPass(250.0f, 0.5412f, Rate);
	Hp[1].HighPass(250.0f, 1.3066f, Rate);
	Lp[0].LowPass(5000.0f, 0.5412f, Rate);
	Lp[1].LowPass(5000.0f, 1.3066f, Rate);
	Horn.Peak(2600.0f, 1.2f, 4.0f, Rate);
	for (Biquad* B : {&Hp[0], &Hp[1], &Lp[0], &Lp[1], &Horn})
	{
		B->Clear();
	}
	const float Ms[3] = {25.0f, 55.0f, 90.0f};
	const float Gains[3] = {0.55f, 0.36f, 0.24f};
	const float Dull[3] = {3800.0f, 3000.0f, 2400.0f};
	int Longest = 0;
	for (int K = 0; K < 3; ++K)
	{
		TapLen[K] = static_cast<int>((Ms[K] + R.Range(-1.5f, 1.5f)) * 0.001f * Rate);
		TapGain[K] = Gains[K] * R.Range(0.92f, 1.08f);
		TapK[K] = OnePoleK(Dull[K], Rate);
		TapLp[K] = 0.0f;
		Longest = std::max(Longest, TapLen[K]);
	}
	Line.assign(static_cast<size_t>(Longest + 2), static_cast<int16_t>(0));
	Pos = 0;
}

void PaStage::Process(float* X, int N)
{
	if (Line.empty())
	{
		return;
	}
	const int Size = static_cast<int>(Line.size());
	for (int I = 0; I < N; ++I)
	{
		float Y = Hp[1].Run(Hp[0].Run(X[I]));
		Y = Horn.Run(Lp[1].Run(Lp[0].Run(Y)));
		Y = Y / (1.0f + 0.35f * std::fabs(Y)); // the small speaker's give
		const float Held = std::min(32767.0f, std::max(-32767.0f, Y * 8192.0f));
		Line[static_cast<size_t>(Pos)] = static_cast<int16_t>(Held);
		float Out = Y;
		for (int K = 0; K < 3; ++K)
		{
			int At = Pos - TapLen[K];
			if (At < 0)
			{
				At += Size;
			}
			TapLp[K] += TapK[K] * (static_cast<float>(Line[static_cast<size_t>(At)]) * (1.0f / 8192.0f) - TapLp[K]);
			Out += TapGain[K] * TapLp[K];
		}
		X[I] = Out * 0.72f;
		Pos = Pos + 1 < Size ? Pos + 1 : 0;
	}
}

// ------------------------------------------------------------------ clappers

int SeedClappers(Vocal V, int Count, Rng& R, float At, Clapper* Out, int Max)
{
	const bool Light = V == Vocal::Clap;
	const int N = std::max(0, std::min(Count, Max));
	const float Length = Light ? 2.0f : 3.2f + std::min(2.8f, 0.03f * static_cast<float>(std::max(0, Count - 1)));
	for (int I = 0; I < N; ++I)
	{
		Clapper& C = Out[I];
		C.Start = At + R.Range(0.0f, Light ? 0.25f : 0.35f);
		C.Next = C.Start;
		C.Period = 1.0f / R.Range(Light ? 2.2f : 3.2f, Light ? 3.4f : 5.8f);
		C.Swell = Length;
		C.Until = At + Length * R.Range(0.65f, 1.0f);
		C.Hz = R.Range(900.0f, 2500.0f);
		C.Q = R.Range(1.2f, 2.4f);
		C.Gain = R.Range(0.6f, 1.0f) * (Light ? 0.55f : 1.0f);
		C.Pan = R.Signed();
	}
	return N;
}

float ClapGain(const Clapper& C, float T)
{
	if (T > C.Until || T < C.Start)
	{
		return 0.0f;
	}
	const float U = (T - C.Start) / std::max(0.1f, C.Swell);
	const float Rise = std::min(1.0f, U / 0.12f);
	return C.Gain * Rise * (1.0f - 0.35f * U);
}

void ClapVoice::Strike(float Hz, float Q, float InGain, float Seconds, float Rate)
{
	Band.BandPass(Hz, Q, Rate);
	Env = 1.0f;
	Decay = std::exp(-1.0f / (std::max(0.001f, Seconds) * Rate));
	Click = 1.0f;
	Gain = InGain;
	Live = true;
}
} // namespace vox
} // namespace audio
} // namespace ss
