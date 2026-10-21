// synthesized_audio: every synthesized sound, the card room's voices and the room itself (next_segment_spec §7.6).
//
// Usage: audio_test                      the checks
//        audio_test <out-dir>            the checks, and every one-shot as a 16-bit WAV in <out-dir>
//        audio_test --listening <dir>    the listening gate's renders (§4.10) into <dir>, no checks
//        audio_test --cues <dir>         Update 3's site cues on their own and in their moments (reels), no checks
#include "ShortStack/Audio/CardRoom.h"
#include "ShortStack/Audio/Synth.h"
#include "ShortStack/Audio/Voice.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <complex>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <intrin.h>
#else
#include <time.h>
#endif

namespace audio_test
{
using namespace ss::audio;

int Failures = 0;
int Checks = 0;
std::string OutDir;

void Expect(bool Ok, const std::string& Name, const std::string& Detail = std::string())
{
	++Checks;
	if (!Ok)
	{
		++Failures;
	}
	std::printf("  %-6s %s%s%s\n", Ok ? "ok" : "FAILED", Name.c_str(), Detail.empty() ? "" : ": ", Detail.c_str());
}

std::string Fmt(const char* Format, double A, double B = 0.0, double C = 0.0)
{
	char Buf[256];
	std::snprintf(Buf, sizeof(Buf), Format, A, B, C);
	return Buf;
}

void WriteWav(const std::string& Path, const std::vector<float>& S, int Channels)
{
	FILE* F = std::fopen(Path.c_str(), "wb");
	if (!F)
	{
		return;
	}
	const uint32_t Rate = static_cast<uint32_t>(SampleRate);
	const uint32_t DataBytes = static_cast<uint32_t>(S.size() * 2);
	auto U32 = [&](uint32_t V) { std::fwrite(&V, 4, 1, F); };
	auto U16 = [&](uint16_t V) { std::fwrite(&V, 2, 1, F); };
	std::fwrite("RIFF", 1, 4, F);
	U32(36 + DataBytes);
	std::fwrite("WAVEfmt ", 1, 8, F);
	U32(16);
	U16(1);
	U16(static_cast<uint16_t>(Channels));
	U32(Rate);
	U32(Rate * 2u * static_cast<uint32_t>(Channels));
	U16(static_cast<uint16_t>(2 * Channels));
	U16(16);
	std::fwrite("data", 1, 4, F);
	U32(DataBytes);
	for (const float V : S)
	{
		const float C = V < -1.0f ? -1.0f : V > 1.0f ? 1.0f : V;
		const int16_t I = static_cast<int16_t>(C * 32767.0f);
		std::fwrite(&I, 2, 1, F);
	}
	std::fclose(F);
}

void Check(const std::string& Name, const std::vector<float>& S, float MinPeak)
{
	float Peak = 0.0f;
	bool Finite = true;
	for (const float V : S)
	{
		Finite = Finite && std::isfinite(V);
		Peak = std::fabs(V) > Peak ? std::fabs(V) : Peak;
	}
	const bool Ok = Finite && Peak >= MinPeak && Peak <= 1.0f && !S.empty();
	Expect(Ok, Name, Fmt("%.2f s, peak %.3f", static_cast<double>(S.size()) / SampleRate, static_cast<double>(Peak)));
	if (!OutDir.empty())
	{
		WriteWav(OutDir + "/" + Name + ".wav", S, 1);
	}
}

// ------------------------------------------------------------------ measuring

double Db(double Power)
{
	return Power <= 1.0e-20 ? -200.0 : 10.0 * std::log10(Power);
}

/** Mean power of channel Ch (0 left, 1 right, 2 both) of interleaved stereo, frames [From, To). */
double StereoPower(const std::vector<float>& S, int Ch, size_t From, size_t To)
{
	double P = 0.0;
	size_t N = 0;
	To = std::min(To, S.size() / 2);
	for (size_t I = From; I < To; ++I)
	{
		const double L = S[2 * I], R = S[2 * I + 1];
		P += Ch == 0 ? L * L : Ch == 1 ? R * R : 0.5 * (L * L + R * R);
		++N;
	}
	return N ? P / static_cast<double>(N) : 0.0;
}

void Fft(std::vector<std::complex<double>>& A)
{
	const size_t N = A.size();
	for (size_t I = 1, J = 0; I < N; ++I)
	{
		size_t Bit = N >> 1;
		for (; J & Bit; Bit >>= 1)
		{
			J ^= Bit;
		}
		J ^= Bit;
		if (I < J)
		{
			std::swap(A[I], A[J]);
		}
	}
	for (size_t Len = 2; Len <= N; Len <<= 1)
	{
		const double Ang = -2.0 * 3.14159265358979323846 / static_cast<double>(Len);
		const std::complex<double> W(std::cos(Ang), std::sin(Ang));
		for (size_t I = 0; I < N; I += Len)
		{
			std::complex<double> Wn(1.0, 0.0);
			for (size_t K = 0; K < Len / 2; ++K)
			{
				const std::complex<double> U = A[I + K];
				const std::complex<double> V = A[I + K + Len / 2] * Wn;
				A[I + K] = U + V;
				A[I + K + Len / 2] = U - V;
				Wn *= W;
			}
		}
	}
}

/** Energy of a mono signal between Lo and Hi Hz (Hann-windowed 4096-point frames, half overlapped). */
double BandEnergy(const std::vector<float>& X, double Lo, double Hi)
{
	const size_t N = 4096;
	double E = 0.0;
	std::vector<std::complex<double>> A(N);
	for (size_t At = 0; At + N <= X.size() || (At == 0 && !X.empty()); At += N / 2)
	{
		for (size_t I = 0; I < N; ++I)
		{
			const double W = 0.5 - 0.5 * std::cos(2.0 * 3.14159265358979323846 * static_cast<double>(I) / static_cast<double>(N));
			A[I] = std::complex<double>(At + I < X.size() ? X[At + I] * W : 0.0, 0.0);
		}
		Fft(A);
		for (size_t K = 1; K < N / 2; ++K)
		{
			const double Hz = static_cast<double>(K) * SampleRate / static_cast<double>(N);
			if (Hz >= Lo && Hz < Hi)
			{
				E += std::norm(A[K]);
			}
		}
		if (At + N >= X.size())
		{
			break;
		}
	}
	return E;
}

std::vector<float> Mono(const std::vector<float>& Stereo)
{
	std::vector<float> M(Stereo.size() / 2);
	for (size_t I = 0; I < M.size(); ++I)
	{
		M[I] = 0.5f * (Stereo[2 * I] + Stereo[2 * I + 1]);
	}
	return M;
}

/** F0 by autocorrelation over a 40 ms window at Center seconds (0 when unvoiced). */
double PitchAt(const std::vector<float>& X, double Center)
{
	const size_t W = static_cast<size_t>(0.04 * SampleRate);
	const long long C = static_cast<long long>(Center * SampleRate);
	if (C - static_cast<long long>(W) / 2 < 0 || C + static_cast<long long>(W) >= static_cast<long long>(X.size()))
	{
		return 0.0;
	}
	const size_t From = static_cast<size_t>(C - static_cast<long long>(W) / 2);
	const size_t MinLag = static_cast<size_t>(SampleRate / 400), MaxLag = static_cast<size_t>(SampleRate / 70);
	double Zero = 0.0;
	for (size_t I = 0; I < W; ++I)
	{
		Zero += static_cast<double>(X[From + I]) * X[From + I];
	}
	std::vector<double> R(MaxLag + 1, 0.0);
	double Best = 0.0;
	for (size_t Lag = MinLag; Lag <= MaxLag; ++Lag)
	{
		for (size_t I = 0; I < W; ++I)
		{
			R[Lag] += static_cast<double>(X[From + I]) * X[From + I + Lag];
		}
		Best = std::max(Best, R[Lag]);
	}
	if (Best <= 0.3 * Zero)
	{
		return 0.0;
	}
	// The shortest lag at a peak near the best: the period, not a multiple of it.
	for (size_t Lag = MinLag + 1; Lag < MaxLag; ++Lag)
	{
		if (R[Lag] >= 0.85 * Best && R[Lag] >= R[Lag - 1] && R[Lag] >= R[Lag + 1])
		{
			return SampleRate / static_cast<double>(Lag);
		}
	}
	return 0.0;
}

// ------------------------------------------------------------------ the thread's own time

/**
 * Milliseconds of CPU this thread has run, not counting time the scheduler gave to other processes: on a busy machine
 * (other builds, other apps) wall time measures the machine; this measures the room. Windows counts the thread's cycles
 * at the invariant TSC's rate (calibrated once against the steady clock); POSIX has a thread clock.
 */
double ThreadMs()
{
#if defined(_WIN32)
	static const double TscPerMs = []() {
		const unsigned long long C0 = __rdtsc();
		const auto T0 = std::chrono::steady_clock::now();
		while (std::chrono::steady_clock::now() - T0 < std::chrono::milliseconds(40))
		{
		}
		const unsigned long long C1 = __rdtsc();
		const double Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count();
		return static_cast<double>(C1 - C0) / Ms;
	}();
	ULONG64 Cycles = 0;
	QueryThreadCycleTime(GetCurrentThread(), &Cycles);
	return static_cast<double>(Cycles) / TscPerMs;
#else
	timespec T;
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &T);
	return static_cast<double>(T.tv_sec) * 1000.0 + static_cast<double>(T.tv_nsec) / 1.0e6;
#endif
}

// ------------------------------------------------------------------ the room, as a test sets it up

struct Mood
{
	const char* Name;
	float Bed, Talk, Chips, Calls, Rail, Crowd;
};
// The arc's levels (spec §4.3) as room::MixFor gives them; this suite keeps its own copy so it tests the room, not the director.
const Mood Running{"Running", 0, -1, 0, 0, -60, 0.7f};
const Mood HandForHand{"HandForHand", -2, -10, 3, 0, 0, 0.6f};
const Mood Break{"Break", 1, 4, -20, -60, -60, 0.7f};
const Mood FinalTable{"FinalTable", -6, -12, -6, 0, 4, 0.1f};
const Mood BurstMood{"Burst", 0, 6, 0, 0, 0, 0.7f};

void SetMood(CardRoom& Room, const Mood& M)
{
	Room.SetMix(M.Bed, M.Talk, M.Chips, M.Calls, M.Rail, M.Crowd);
}

/** A Nightly: 20 tables in four rows of five (5.25 m apart, 7 m between rows), Open of them running, the hero's (Hero) at
 *  zero players; the doorway, the bar, the rail, the desk and the cage. */
std::vector<CardRoom::Emitter> Nightly(int Open, int Hero)
{
	std::vector<CardRoom::Emitter> E;
	int Id = 1;
	for (int Row = 0; Row < 4; ++Row)
	{
		for (int Col = 0; Col < 5; ++Col)
		{
			CardRoom::Emitter T;
			T.Kind = CardRoom::Spot::Table;
			T.Id = Id;
			T.X = static_cast<float>(Row) * 700.0f;
			T.Y = static_cast<float>(Col - 2) * 525.0f;
			T.Open = Id <= Open;
			T.Players = Id == Hero ? 0 : 7;
			E.push_back(T);
			++Id;
		}
	}
	auto Spot = [&](CardRoom::Spot K, float X, float Y) {
		CardRoom::Emitter S;
		S.Kind = K;
		S.X = X;
		S.Y = Y;
		E.push_back(S);
	};
	Spot(CardRoom::Spot::Doorway, -600.0f, -1300.0f);
	Spot(CardRoom::Spot::Bar, -500.0f, 1400.0f);
	Spot(CardRoom::Spot::Rail, 2700.0f, 0.0f);
	Spot(CardRoom::Spot::Desk, -500.0f, 0.0f);
	Spot(CardRoom::Spot::Cage, -500.0f, 600.0f);
	return E;
}

/** Seated at table 12 (row 2, column 1), facing down the room. */
void SeatAtTwelve(CardRoom& Room)
{
	Room.SetEmitters(Nightly(14, 12));
	Room.SetListener(1310.0f, -525.0f, 0.0f);
}

/** The engine's hands: each open table deals one about every 45 s, each on its own offset. */
void Hands(CardRoom& Room, int Block, int Open)
{
	for (int Id = 1; Id <= Open; ++Id)
	{
		if ((Block + Id * 97) % 1800 == 0)
		{
			Room.TableHand(Id, 45.0f, Id % 5 == 4, Id % 5 == 4, 12.0f + static_cast<float>(Id));
		}
	}
}

std::vector<float> Play(CardRoom& Room, double Seconds, int Chunk = 1200)
{
	std::vector<float> Out(static_cast<size_t>(Seconds * SampleRate) * 2);
	const size_t Frames = Out.size() / 2;
	for (size_t At = 0; At < Frames; At += static_cast<size_t>(Chunk))
	{
		Room.RenderStereo(Out.data() + 2 * At, static_cast<int>(std::min(static_cast<size_t>(Chunk), Frames - At)));
	}
	return Out;
}

struct Levels
{
	double Bed = 0, Talk = 0, Chips = 0, Calls = 0, Pa = 0, Rail = 0;
};

/** A mood's layers as the meter reads them, power-averaged over Seconds (after a 4 s settle), the engine dealing. */
Levels MoodLevels(const Mood& M, double Seconds, uint32_t Seed)
{
	CardRoom Room(Seed);
	SeatAtTwelve(Room);
	SetMood(Room, M);
	std::vector<float> Buf(2400);
	Levels P;
	int N = 0;
	const int Blocks = static_cast<int>((Seconds + 4.0) * 40.0);
	for (int B = 0; B < Blocks; ++B)
	{
		Hands(Room, B, 14);
		Room.RenderStereo(Buf.data(), 1200);
		if (B >= 160 && B % 4 == 0)
		{
			const CardRoom::Meter Mt = Room.Measure();
			P.Bed += std::pow(10.0, Mt.BedDb / 10.0);
			P.Talk += std::pow(10.0, Mt.TalkDb / 10.0);
			P.Chips += std::pow(10.0, Mt.ChipsDb / 10.0);
			P.Calls += std::pow(10.0, Mt.CallsDb / 10.0);
			P.Pa += std::pow(10.0, Mt.PaDb / 10.0);
			P.Rail += std::pow(10.0, Mt.RailDb / 10.0);
			++N;
		}
	}
	Levels L;
	L.Bed = Db(P.Bed / N);
	L.Talk = Db(P.Talk / N);
	L.Chips = Db(P.Chips / N);
	L.Calls = Db(P.Calls / N);
	L.Pa = Db(P.Pa / N);
	L.Rail = Db(P.Rail / N);
	return L;
}

/** Only one layer audible (0 dB), the rest silent. */
void Only(CardRoom& Room, int Layer)
{
	const float Off = -120.0f;
	Room.SetMix(Layer == 0 ? 0.0f : Off, Layer == 1 ? 0.0f : Off, Layer == 2 ? 0.0f : Off, Layer == 3 ? 0.0f : Off, Layer == 4 ? 0.0f : Off, 0.7f);
}

CardRoom::Emitter TableAt(int Id, float X, float Y, int Players)
{
	CardRoom::Emitter T;
	T.Kind = CardRoom::Spot::Table;
	T.Id = Id;
	T.X = X;
	T.Y = Y;
	T.Players = Players;
	return T;
}

const char* const Lines[] = {
	"You take the 14 up?",
	"Driver took the switchbacks like he owed somebody money.",
	"Ninth in the Noon Deepstack Thursday. Seventy-four runners and I walk with a hundred sixty.",
	"Third cash in four nights. Don't say anything.",
	"Online I'd have seen forty hands by now.",
	"I'm in makeup to my guy. Every buy-in's his till I'm out.",
	"You can have the next one, champ.",
	"Take your time. I'm only getting older.",
	"That's me. Good luck, all.",
	"Min-cash, baby. My rent thanks you, Lin.",
};

// ------------------------------------------------------------------ the checks

void OneShots()
{
	std::printf("one-shots\n");
	// (Update 3, §1.11: through the revamped sites' ten cues, the last id; SiteCueChecks below holds them to their own.)
	for (int I = 0; I <= static_cast<int>(ss::SoundId::RitRank); ++I)
	{
		const ss::SoundId Id = static_cast<ss::SoundId>(I);
		Check(ss::SoundName(Id), Render(Id, 1.0f, 7u + static_cast<uint32_t>(I)), 0.01f);
	}
	Check("buzz", Render(Effect::Buzz, 1u), 0.05f);
	Check("canopen", Render(Effect::CanOpen, 2u), 0.02f);
	Check("heartbeat", Render(Effect::Thump, 3u), 0.05f);
	Check("step", Render(Effect::Step, 4u), 0.02f);
	Check("scrape", Render(Effect::Scrape, 5u), 0.02f);
	Check("breath", Render(Effect::Breath, 6u), 0.01f);
	Check("chime", Render(Effect::Chime, 7u), 0.05f);
	Check("applause", Render(Effect::Applause, 8u), 0.02f);
	Check("cardprint", Render(Effect::CardPrint, 9u), 0.02f);
	Check("thunder", RenderThunder(1.0f, 4u), 0.02f);
	Ambience Amb(5u);
	std::vector<float> Room(static_cast<size_t>(SampleRate) * 6);
	for (size_t I = 0; I < Room.size(); I += 480)
	{
		Amb.Render(Room.data() + I, 480);
	}
	Check("ambience", Room, 0.02f);

	// The two this segment adds: deterministic, and their seeds matter.
	const std::vector<float> PrintA = Render(Effect::CardPrint, 11u), PrintB = Render(Effect::CardPrint, 11u), PrintC = Render(Effect::CardPrint, 12u);
	Expect(PrintA == PrintB && PrintA != PrintC, "CardPrint is deterministic and seeded");
	Expect(PrintA.size() > static_cast<size_t>(1.5 * SampleRate) && PrintA.size() < static_cast<size_t>(2.2 * SampleRate), "CardPrint runs about 1.8 s",
		Fmt("%.2f s", static_cast<double>(PrintA.size()) / SampleRate));
	const std::vector<float> MailA = Render(ss::SoundId::Mail, 1.0f, 3u), MailB = Render(ss::SoundId::Mail, 1.0f, 3u), MailC = Render(ss::SoundId::Mail, 1.0f, 4u);
	Expect(MailA == MailB && MailA != MailC, "Mail is deterministic and seeded");
	const std::vector<float> MailQuiet = Render(ss::SoundId::Mail, 0.3f, 3u);
	float PeakLoud = 0.0f, PeakQuiet = 0.0f;
	for (const float V : MailA)
	{
		PeakLoud = std::max(PeakLoud, std::fabs(V));
	}
	for (const float V : MailQuiet)
	{
		PeakQuiet = std::max(PeakQuiet, std::fabs(V));
	}
	Expect(PeakQuiet < PeakLoud * 0.6f, "Mail follows its volume");
}

// ------------------------------------------------------------------ Update 3: the revamped sites' ten cues (SPEC §1.11, §10)

/** A cue as the sites ask for it: the id at the volume a caller uses (which picks IgBoom's and RitRank's variants), a
 *  quieter volume inside the same variant, the longest it may run, whether it's a tick that repeats (a count, a wheel, a
 *  wave, a feed: short and quiet), and whether it carries noise (so each render is its own). */
struct SiteCue
{
	const char* Name;
	ss::SoundId Id;
	float Vol, Quieter;
	double MaxSeconds;
	bool Tick, Seeded;
};

const SiteCue SiteCues[] = {
	{"igspark", ss::SoundId::IgSpark, 0.35f, 0.25f, 0.1, true, true},
	{"igpop", ss::SoundId::IgPop, 0.45f, 0.3f, 0.5, false, true},
	{"igtick", ss::SoundId::IgTick, 0.35f, 0.25f, 0.06, true, true},
	{"igfuse", ss::SoundId::IgFuse, 0.4f, 0.3f, 2.0, false, true},
	{"igboom", ss::SoundId::IgBoom, 0.45f, 0.25f, 1.2, false, true},
	{"igboom-long", ss::SoundId::IgBoom, 0.6f, 0.5f, 2.6, false, true},
	{"ritblip", ss::SoundId::RitBlip, 0.3f, 0.25f, 0.06, true, true},
	{"ritflap", ss::SoundId::RitFlap, 0.35f, 0.25f, 0.25, true, true},
	{"ritbell", ss::SoundId::RitBell, 0.45f, 0.3f, 2.0, false, true},
	{"ritstamp", ss::SoundId::RitStamp, 0.5f, 0.3f, 0.4, false, true},
	{"ritrank", ss::SoundId::RitRank, 0.35f, 0.25f, 0.08, true, true},
	{"ritrank-fall", ss::SoundId::RitRank, 0.45f, 0.45f, 0.8, false, false},
	{"ritrank-rise", ss::SoundId::RitRank, 0.6f, 0.5f, 0.8, false, false},
};
/** Distinct: a cue's nearest other sound is at least this far (dB, Apart below) and this much farther than the cue is from
 *  itself with other seeds. */
const double SiteCueApart = 8.0;
const double SiteCueOverSelf = 2.0;

float PeakOf(const std::vector<float>& S)
{
	float P = 0.0f;
	for (const float V : S)
	{
		P = std::max(P, std::fabs(V));
	}
	return P;
}

/** RMS over [T0, T1) seconds (0 outside the render). */
double RmsOf(const std::vector<float>& S, double T0, double T1)
{
	const size_t From = static_cast<size_t>(std::max(0.0, T0) * SampleRate), To = std::min(S.size(), static_cast<size_t>(T1 * SampleRate));
	double P = 0.0;
	for (size_t I = From; I < To; ++I)
	{
		P += static_cast<double>(S[I]) * S[I];
	}
	return To > From ? std::sqrt(P / static_cast<double>(To - From)) : 0.0;
}

/** The strongest frequency in [T0, T1) seconds (Hann-windowed, zero-padded to 8192 points). */
double DominantHz(const std::vector<float>& S, double T0, double T1)
{
	const size_t N = 8192;
	const size_t From = static_cast<size_t>(T0 * SampleRate), To = std::min(S.size(), static_cast<size_t>(T1 * SampleRate));
	if (To <= From + 1)
	{
		return 0.0;
	}
	std::vector<std::complex<double>> A(N);
	for (size_t I = From; I < To && I - From < N; ++I)
	{
		const double W = 0.5 - 0.5 * std::cos(2.0 * 3.14159265358979323846 * static_cast<double>(I - From) / static_cast<double>(To - From - 1));
		A[I - From] = std::complex<double>(S[I] * W, 0.0);
	}
	Fft(A);
	size_t Best = 1;
	for (size_t K = 1; K < N / 2; ++K)
	{
		Best = std::norm(A[K]) > std::norm(A[Best]) ? K : Best;
	}
	return static_cast<double>(Best) * SampleRate / static_cast<double>(N);
}

/** The share of a render's energy under Hz. */
double ShareUnder(const std::vector<float>& S, double Hz)
{
	const double All = BandEnergy(S, 1.0, SampleRate * 0.5);
	return All > 0.0 ? BandEnergy(S, 1.0, Hz) / All : 0.0;
}

/**
 * A sound's look, as an ear tells two apart: a spectrogram in 24 third-octave bands from 80 Hz (1,024-point frames, 512
 * apart), in dB under its own reference, floored 45 dB under it and capped at it, running to the last frame with a cell
 * above the floor. The reference is the 99th percentile of the cells within 60 dB of the loudest, so one loud cell (a
 * thump's first frame) doesn't push the rest of the sound into the floor. Its loudness doesn't count, only its colour and
 * its shape in time.
 */
using CueLook = std::vector<std::vector<double>>;

CueLook LookOf(const std::vector<float>& S)
{
	const size_t N = 1024, Hop = 512;
	const int Bands = 24;
	CueLook L;
	std::vector<std::complex<double>> A(N);
	double Loudest = -300.0;
	for (long long At = -static_cast<long long>(N / 2); At < static_cast<long long>(S.size()); At += static_cast<long long>(Hop))
	{
		for (size_t I = 0; I < N; ++I)
		{
			const double W = 0.5 - 0.5 * std::cos(2.0 * 3.14159265358979323846 * static_cast<double>(I) / static_cast<double>(N));
			const long long J = At + static_cast<long long>(I);
			A[I] = std::complex<double>(J >= 0 && J < static_cast<long long>(S.size()) ? S[static_cast<size_t>(J)] * W : 0.0, 0.0);
		}
		Fft(A);
		std::vector<double> Row(Bands, 0.0);
		for (size_t K = 1; K < N / 2; ++K)
		{
			const double Hz = static_cast<double>(K) * SampleRate / static_cast<double>(N);
			const int Band = static_cast<int>(std::floor(std::log2(Hz / 80.0) * 3.0));
			if (Band >= 0 && Band < Bands)
			{
				Row[static_cast<size_t>(Band)] += std::norm(A[K]);
			}
		}
		for (double& E : Row)
		{
			E = 10.0 * std::log10(E + 1.0e-30);
			Loudest = std::max(Loudest, E);
		}
		L.push_back(Row);
	}
	std::vector<double> Cells;
	for (const std::vector<double>& Row : L)
	{
		for (const double E : Row)
		{
			if (E > Loudest - 60.0)
			{
				Cells.push_back(E);
			}
		}
	}
	std::sort(Cells.begin(), Cells.end());
	const double Reference = Cells.empty() ? Loudest : Cells[static_cast<size_t>(0.99 * static_cast<double>(Cells.size() - 1))];
	size_t Last = 0;
	for (size_t F = 0; F < L.size(); ++F)
	{
		for (double& E : L[F])
		{
			E = std::min(0.0, std::max(-45.0, E - Reference));
			Last = E > -45.0 ? F : Last;
		}
	}
	L.resize(Last + 1);
	return L;
}

/** How far apart two looks are: the mean dB difference over every cell where either is above the floor. */
double Apart(const CueLook& A, const CueLook& B)
{
	double Sum = 0.0;
	size_t Cells = 0;
	for (size_t F = 0; F < std::max(A.size(), B.size()); ++F)
	{
		for (size_t K = 0; K < 24; ++K)
		{
			const double Ea = F < A.size() ? A[F][K] : -45.0, Eb = F < B.size() ? B[F][K] : -45.0;
			if (Ea > -45.0 || Eb > -45.0)
			{
				Sum += std::fabs(Ea - Eb);
				++Cells;
			}
		}
	}
	return Cells ? Sum / static_cast<double>(Cells) : 0.0;
}

/** The ten site cues (§1.11): each its own, bounded, distinct from each other and from the game's sounds, at the
 *  volumes the sites use (§10: 0.25-0.6), cheap enough to render on the game thread. */
void SiteCueChecks()
{
	std::printf("the sites' cues\n");
	const int Cases = static_cast<int>(sizeof(SiteCues) / sizeof(SiteCues[0]));
	std::vector<std::vector<float>> Heard;
	for (const SiteCue& C : SiteCues)
	{
		const std::vector<float> S = Render(C.Id, C.Vol, 11u);
		const float Peak = PeakOf(S);
		bool Finite = !S.empty();
		for (const float V : S)
		{
			Finite = Finite && std::isfinite(V);
		}
		const double Seconds = static_cast<double>(S.size()) / SampleRate;
		const bool Ok = Finite && Seconds <= C.MaxSeconds && Peak >= 0.01f && Peak <= (C.Tick ? 0.1f : 0.6f);
		Expect(Ok, std::string(C.Name) + " is bounded",
			Fmt("%.3f s (%.2f at most), ", Seconds, C.MaxSeconds) + Fmt("peak %.3f (%.2f at most) at volume %.2f", static_cast<double>(Peak), C.Tick ? 0.1 : 0.6, static_cast<double>(C.Vol)));
		if (!OutDir.empty())
		{
			WriteWav(OutDir + "/" + C.Name + ".wav", S, 1);
		}
		const float Quiet = PeakOf(Render(C.Id, C.Quieter, 11u));
		if (C.Quieter < C.Vol)
		{
			const double Want = std::sqrt(static_cast<double>(C.Vol) / static_cast<double>(C.Quieter));
			Expect(static_cast<double>(Peak) >= static_cast<double>(Quiet) * Want, std::string(C.Name) + " follows its volume",
				Fmt("peak %.3f at %.2f against %.3f", static_cast<double>(Quiet), static_cast<double>(C.Quieter), static_cast<double>(Peak)));
		}
		const bool Same = Render(C.Id, C.Vol, 5u) == Render(C.Id, C.Vol, 5u);
		const bool Own = !C.Seeded || Render(C.Id, C.Vol, 5u) != Render(C.Id, C.Vol, 6u);
		Expect(Same && Own, std::string(C.Name) + (C.Seeded ? " is deterministic and seeded" : " is deterministic"));
		Heard.push_back(S);
	}

	// §10's range: every id, 0.25 to 0.6, heard and bounded, whatever seed the host renders it with (eight of them).
	bool Range = true;
	float Loudest = 0.0f;
	std::string RangeWhy;
	for (int I = static_cast<int>(ss::SoundId::IgSpark); I <= static_cast<int>(ss::SoundId::RitRank); ++I)
	{
		const ss::SoundId Id = static_cast<ss::SoundId>(I);
		for (uint32_t Seed = 1u; Seed <= 8u; ++Seed)
		{
			const float Low = PeakOf(Render(Id, 0.25f, Seed)), High = PeakOf(Render(Id, 0.6f, Seed));
			Loudest = std::max(Loudest, High);
			if (Low < 0.01f || High > 0.6f)
			{
				Range = false;
				RangeWhy += std::string(ss::SoundName(Id)) + Fmt(" seed %.0f: %.3f to %.3f; ", Seed, static_cast<double>(Low), static_cast<double>(High));
			}
		}
	}
	Expect(Range, "every cue is heard at 0.25 and peaks at 0.6 or under at 0.6 (eight seeds)",
		RangeWhy.empty() ? Fmt("the loudest %.3f", static_cast<double>(Loudest)) : RangeWhy);
	{
		// The booms' noise is the loudest thing a seed moves: the host seeds each render afresh (Seed++), and without the
		// ceiling 4 of seeds 1-96 took the long tail past 0.6 at 0.6 (21, 38, 65, 70; about 1 render in 20 overall).
		float BoomTop = 0.0f;
		for (uint32_t Seed = 1u; Seed <= 96u; ++Seed)
		{
			BoomTop = std::max({BoomTop, PeakOf(Render(ss::SoundId::IgBoom, 0.45f, Seed)), PeakOf(Render(ss::SoundId::IgBoom, 0.6f, Seed))});
		}
		Expect(BoomTop <= 0.6f, "IgBoom peaks at 0.6 or under whatever the seed (96 seeds, both variants)", Fmt("the loudest %.3f", static_cast<double>(BoomTop)));
	}

	// The variants by volume, at the volumes callers pass (UiSound's default 0.4 is the everyday one).
	auto Secs = [](const std::vector<float>& S) { return static_cast<double>(S.size()) / SampleRate; };
	const double Boom4 = Secs(Render(ss::SoundId::IgBoom, 0.4f, 1u)), Boom45 = Secs(Render(ss::SoundId::IgBoom, 0.45f, 1u));
	const double Boom5 = Secs(Render(ss::SoundId::IgBoom, 0.5f, 1u)), Boom6 = Secs(Render(ss::SoundId::IgBoom, 0.6f, 1u));
	Expect(Boom4 <= 1.2 && Boom45 <= 1.2 && Boom5 >= 1.8 && Boom6 >= 1.8, "IgBoom: 0.25-0.45 lands short, 0.5-0.6 carries the long tail (x25 and up)",
		Fmt("%.2f s at 0.4, %.2f at 0.45, ", Boom4, Boom45) + Fmt("%.2f at 0.5, %.2f at 0.6", Boom5, Boom6));
	const std::vector<float> Tick = Render(ss::SoundId::RitRank, 0.4f, 1u), Fall = Render(ss::SoundId::RitRank, 0.45f, 1u);
	const std::vector<float> Rise = Render(ss::SoundId::RitRank, 0.5f, 1u), Rise6 = Render(ss::SoundId::RitRank, 0.6f, 1u);
	const double FallA = DominantHz(Fall, 0.02, 0.12), FallB = DominantHz(Fall, 0.2, 0.35), RiseA = DominantHz(Rise, 0.02, 0.12), RiseB = DominantHz(Rise, 0.2, 0.35);
	Expect(Secs(Tick) <= 0.08 && Secs(Render(ss::SoundId::RitRank, 0.25f, 1u)) <= 0.08, "RitRank: 0.25-0.4 is one tick", Fmt("%.3f s at 0.4", Secs(Tick)));
	Expect(Secs(Fall) > 0.3 && FallB < FallA && FallB / FallA > 0.7 && FallB / FallA < 0.8, "RitRank at 0.45: two notes falling a fourth (a relegation)",
		Fmt("%.0f Hz then %.0f Hz", FallA, FallB));
	Expect(Secs(Rise) > 0.3 && RiseB / RiseA > 1.4 && RiseB / RiseA < 1.6 && Rise6 != Rise && Secs(Rise6) > 0.3,
		"RitRank at 0.5-0.6: two notes rising a fifth (a promotion)", Fmt("%.0f Hz then %.0f Hz", RiseA, RiseB));

	// Each cue's character.
	const std::vector<float>& Spark = Heard[0];
	const double SparkA = DominantHz(Spark, 0.003, 0.015), SparkB = DominantHz(Spark, 0.03, 0.045);
	Expect(SparkB >= SparkA * 1.25, "IgSpark rises", Fmt("%.0f Hz then %.0f Hz", SparkA, SparkB));
	const std::vector<float>& Fuse = Heard[3];
	const double FuseRms = RmsOf(Fuse, 0.0, 1.0);
	Expect(static_cast<double>(PeakOf(Fuse)) >= 6.0 * FuseRms, "IgFuse crackles (peak over RMS 6 or more)", Fmt("%.1f", static_cast<double>(PeakOf(Fuse)) / FuseRms));
	Expect(std::fabs(Fuse.front()) <= 0.01f * PeakOf(Fuse) && std::fabs(Fuse.back()) <= 0.01f * PeakOf(Fuse), "IgFuse fades in and out (a loop segment)");
	{
		// Four segments fired 0.5 s apart, as the sites hold a long fuse: the 20 ms overlaps hold the level (no gap, no dip).
		std::vector<float> Chain(static_cast<size_t>(2.1 * SampleRate), 0.0f);
		for (uint32_t K = 0; K < 4; ++K)
		{
			const std::vector<float> Seg = Render(ss::SoundId::IgFuse, 0.4f, 21u + K);
			const size_t At = static_cast<size_t>(0.5 * K * SampleRate);
			for (size_t I = 0; I < Seg.size() && At + I < Chain.size(); ++I)
			{
				Chain[At + I] += Seg[I];
			}
		}
		std::vector<double> Windows;
		for (double T = 0.03; T + 0.02 < 2.0; T += 0.01)
		{
			Windows.push_back(RmsOf(Chain, T, T + 0.02));
		}
		std::sort(Windows.begin(), Windows.end());
		const double Median = Windows[Windows.size() / 2];
		double Worst = 1.0e9;
		for (int K = 1; K <= 3; ++K)
		{
			Worst = std::min(Worst, RmsOf(Chain, 0.5 * K, 0.5 * K + 0.02));
		}
		Expect(Db(Worst * Worst) >= Db(Median * Median) - 3.0 && Secs(Fuse) <= 2.0, "IgFuse loops: fired every 0.5 s, the seams hold the level",
			Fmt("the weakest seam %.1f dB against the median", Db(Worst * Worst) - Db(Median * Median)));
	}
	const std::vector<float>& BoomShort = Heard[4];
	const std::vector<float>& BoomLong = Heard[5];
	Expect(ShareUnder(BoomShort, 200.0) >= 0.3 && ShareUnder(BoomLong, 200.0) >= 0.3, "IgBoom booms (a third of its energy or more under 200 Hz)",
		Fmt("%.2f and %.2f", ShareUnder(BoomShort, 200.0), ShareUnder(BoomLong, 200.0)));
	Expect(Db(RmsOf(BoomLong, 1.2, 2.0) * RmsOf(BoomLong, 1.2, 2.0)) >= Db(RmsOf(BoomLong, 0.0, 0.3) * RmsOf(BoomLong, 0.0, 0.3)) - 40.0,
		"IgBoom's long tail is still heard 1.2-2.0 s in (within 40 dB of the boom)");
	{
		// Flaps: onsets in the 1 ms envelope above 30% of its peak, 10 ms or more apart.
		const std::vector<float>& Flap = Heard[7];
		std::vector<float> Env;
		for (size_t I = 0; I < Flap.size(); I += 48)
		{
			float P = 0.0f;
			for (size_t J = I; J < std::min(Flap.size(), I + 48); ++J)
			{
				P = std::max(P, std::fabs(Flap[J]));
			}
			Env.push_back(P);
		}
		const float Top = PeakOf(Env);
		int Onsets = 0;
		for (size_t I = 0; I < Env.size(); ++I)
		{
			bool Crest = Env[I] >= 0.3f * Top;
			for (size_t J = I >= 5 ? I - 5 : 0; J < std::min(Env.size(), I + 6) && Crest; ++J)
			{
				Crest = Env[J] <= Env[I];
			}
			if (Crest)
			{
				++Onsets;
				I += 9;
			}
		}
		Expect(Onsets == 6 && Secs(Flap) <= 0.2, "RitFlap is six flaps settling", Fmt("%.0f flaps in %.3f s", Onsets, Secs(Flap)));
	}
	const std::vector<float>& Bell = Heard[8];
	const double BellHz = DominantHz(Bell, 0.05, 0.5);
	Expect(BellHz > 2250.0 && BellHz < 2450.0 && Db(RmsOf(Bell, 0.9, 1.0) * RmsOf(Bell, 0.9, 1.0)) >= Db(RmsOf(Bell, 0.0, 0.1) * RmsOf(Bell, 0.0, 0.1)) - 35.0,
		"RitBell is a bell at about 2.35 kHz, ringing past 1 s", Fmt("%.0f Hz", BellHz));
	Expect(ShareUnder(Heard[9], 300.0) >= 0.4, "RitStamp thunks (two fifths of its energy or more under 300 Hz)", Fmt("%.2f", ShareUnder(Heard[9], 300.0)));

	// Distinct: every cue (and each variant) is farther from everything else than from itself with another seed, and far
	// from all 17 of the game's other sounds and its effects (how far the game's own sounds sit apart: printed).
	std::vector<std::pair<std::string, CueLook>> Others;
	for (int I = 0; I <= static_cast<int>(ss::SoundId::Mail); ++I)
	{
		Others.push_back({ss::SoundName(static_cast<ss::SoundId>(I)), LookOf(Render(static_cast<ss::SoundId>(I), 0.6f, 7u + static_cast<uint32_t>(I)))});
	}
	const char* const EffectNames[] = {"buzz", "canopen", "heartbeat", "step", "scrape", "breath", "chime", "applause", "cardprint"};
	for (int I = 0; I <= static_cast<int>(Effect::CardPrint); ++I)
	{
		Others.push_back({EffectNames[I], LookOf(Render(static_cast<Effect>(I), 1u + static_cast<uint32_t>(I)))});
	}
	double GameNearest = 1.0e9;
	std::string GamePair;
	for (int I = 0; I <= static_cast<int>(ss::SoundId::Mail); ++I)
	{
		for (int J = I + 1; J <= static_cast<int>(ss::SoundId::Mail); ++J)
		{
			const double D = Apart(Others[static_cast<size_t>(I)].second, Others[static_cast<size_t>(J)].second);
			if (D < GameNearest)
			{
				GameNearest = D;
				GamePair = Others[static_cast<size_t>(I)].first + " and " + Others[static_cast<size_t>(J)].first;
			}
		}
	}
	std::printf("         the game's own nearest pair: %s, %.1f dB apart\n", GamePair.c_str(), GameNearest);
	std::vector<CueLook> Looks;
	for (const std::vector<float>& S : Heard)
	{
		Looks.push_back(LookOf(S));
	}
	for (int I = 0; I < Cases; ++I)
	{
		const SiteCue& C = SiteCues[I];
		double Self = 0.0;
		if (C.Seeded)
		{
			const CueLook A = LookOf(Render(C.Id, C.Vol, 12u)), B = LookOf(Render(C.Id, C.Vol, 13u));
			Self = std::max({Apart(Looks[static_cast<size_t>(I)], A), Apart(Looks[static_cast<size_t>(I)], B), Apart(A, B)});
		}
		double Nearest = 1.0e9;
		std::string Who;
		for (int J = 0; J < Cases; ++J)
		{
			const double D = J == I ? 1.0e9 : Apart(Looks[static_cast<size_t>(I)], Looks[static_cast<size_t>(J)]);
			if (D < Nearest)
			{
				Nearest = D;
				Who = SiteCues[J].Name;
			}
		}
		for (const auto& O : Others)
		{
			const double D = Apart(Looks[static_cast<size_t>(I)], O.second);
			if (D < Nearest)
			{
				Nearest = D;
				Who = O.first;
			}
		}
		Expect(Nearest >= SiteCueApart && Nearest >= Self + SiteCueOverSelf, std::string(C.Name) + " is distinct",
			Fmt("nearest %.1f dB (", Nearest) + Who + Fmt("), %.1f dB from itself with other seeds", Self));
	}

	// Cheap enough for the game thread (the host renders a cue when it's asked for, up to 20 a second in a count): the
	// thread's own time, the best of five, per second of sound no more than the game's own one-shots take (their median,
	// measured in the same run, so a busy machine slows both), and under a ceiling (ticks 0.5 ms, the rest 6 ms).
	auto Cost = [](ss::SoundId Id, float Vol, double& Seconds) {
		double Best = 1.0e9;
		for (uint32_t K = 0; K < 5; ++K)
		{
			const double T0 = ThreadMs();
			const std::vector<float> S = Render(Id, Vol, 40u + K);
			Best = std::min(Best, ThreadMs() - T0);
			Seconds = static_cast<double>(S.size()) / SampleRate;
		}
		return Best;
	};
	std::vector<double> GameRates;
	for (int I = 0; I <= static_cast<int>(ss::SoundId::Mail); ++I)
	{
		double Seconds = 1.0;
		const double Ms = Cost(static_cast<ss::SoundId>(I), 0.6f, Seconds);
		GameRates.push_back(Ms / std::max(Seconds, 0.01));
	}
	std::sort(GameRates.begin(), GameRates.end());
	const double GameRate = GameRates[GameRates.size() / 2];
	std::printf("         the game's one-shots render in %.2f ms a second of sound (the median)\n", GameRate);
	for (const SiteCue& C : SiteCues)
	{
		double Seconds = 1.0;
		const double Ms = Cost(C.Id, C.Vol, Seconds);
		const double Ceiling = C.Tick ? 0.5 : 6.0;
		Expect(Ms <= Ceiling && Ms / std::max(Seconds, 0.01) <= GameRate, std::string(C.Name) + " renders fast",
			Fmt("%.3f ms (%.1f at most), %.2f ms a second", Ms, Ceiling, Ms / std::max(Seconds, 0.01)));
	}

	// A host's bad volume: a non-number or an infinity is silence, a negative one too, a huge one the top of the range;
	// never past the limiter.
	bool BadOk = true;
	const float Bad[] = {std::nanf(""), INFINITY, -INFINITY, -1.0f, 1.0e9f};
	for (int I = static_cast<int>(ss::SoundId::IgSpark); I <= static_cast<int>(ss::SoundId::RitRank); ++I)
	{
		for (const float V : Bad)
		{
			const std::vector<float> S = Render(static_cast<ss::SoundId>(I), V, 2u);
			bool Finite = !S.empty();
			for (const float X : S)
			{
				Finite = Finite && std::isfinite(X);
			}
			BadOk = BadOk && Finite && PeakOf(S) <= 0.84f && (V == 1.0e9f ? PeakOf(S) > 0.0f : PeakOf(S) == 0.0f);
		}
	}
	Expect(BadOk, "a bad volume renders finite and bounded: NaN, infinities and negatives are silence, a huge one the top");
}

void Timbres()
{
	std::printf("timbres\n");
	bool Men = true, Women = true, Rates = true, Pas = true;
	for (uint32_t S = 0; S < 200; ++S)
	{
		const int Age = 18 + static_cast<int>(S % 70);
		const Timbre M = TimbreFor(false, Age, S);
		const Timbre W = TimbreFor(true, Age, S);
		Men = Men && M.F0 >= 85.0f && M.F0 <= 155.0f && M.Tract > 0.9f && M.Tract < 1.1f;
		Women = Women && W.F0 >= 165.0f && W.F0 <= 255.0f && W.Tract > 1.08f && W.Tract < 1.26f;
		Rates = Rates && M.Rate >= 4.0f && M.Rate <= 5.0f && W.Rate >= 4.0f && W.Rate <= 5.0f;
		const Timbre P = PaTimbre(S);
		Pas = Pas && P.Rate >= 3.3f && P.Rate <= 3.8f;
	}
	Expect(Men, "men speak at 85 to 155 Hz, formant scale 1.0");
	Expect(Women, "women speak at 165 to 255 Hz, formant scale about 1.17");
	Expect(Rates, "talkers run 4 to 5 syllables a second");
	Expect(Pas, "the PA's announcer runs 3.3 to 3.8");
	Expect(TimbreFor(false, 70, 9u).Rasp > TimbreFor(false, 25, 9u).Rasp, "an older voice is raspier");
}

void Speech()
{
	std::printf("speech\n");
	// The rate: 3.5 to 5.5 syllables a second while talking, every voice; a line said in one breath keeps it whole.
	double Lo = 99.0, Hi = 0.0, WholeLo = 99.0, WholeHi = 0.0;
	for (const char* Line : Lines)
	{
		const std::string Text = Line;
		const bool OneBreath = Text.find_first_of(",.!?;:") == Text.size() - 1;
		for (int V = 0; V < 4; ++V)
		{
			const Timbre T = TimbreFor(V % 2 == 1, V < 2 ? 26 : 68, 40u + static_cast<uint32_t>(V));
			const SpeechPlan P = PlanSpeech(Line, T, 0.6f, 5u);
			double Spoken = 0.0;
			for (const Syllable& S : P.Syllables)
			{
				Spoken += S.Length;
			}
			const double Articulation = static_cast<double>(P.Syllables.size()) / Spoken;
			Lo = std::min(Lo, Articulation);
			Hi = std::max(Hi, Articulation);
			if (OneBreath)
			{
				const double Whole = static_cast<double>(P.Syllables.size()) / P.Seconds;
				WholeLo = std::min(WholeLo, Whole);
				WholeHi = std::max(WholeHi, Whole);
			}
		}
	}
	Expect(Lo >= 3.5 && Hi <= 5.5, "3.5 to 5.5 syllables a second", Fmt("%.2f to %.2f", Lo, Hi));
	Expect(WholeLo >= 3.5 && WholeHi <= 5.5, "a line in one breath keeps the rate start to end", Fmt("%.2f to %.2f", WholeLo, WholeHi));
	{
		const Timbre Pa = PaTimbre(3u);
		const SpeechPlan P = PlanSpeech("Players, we are three away from the money. Fifteen places are paid.", Pa, 0.7f, 2u);
		double Spoken = 0.0;
		for (const Syllable& S : P.Syllables)
		{
			Spoken += S.Length;
		}
		const double Rate = static_cast<double>(P.Syllables.size()) / Spoken;
		Expect(Rate >= 3.0 && Rate <= 4.1, "the PA speaks slower", Fmt("%.2f syllables a second", Rate));
	}
	// The words count: numbers spoken out, a comma's pause, a question's rise and a statement's fall.
	const Timbre Dealer = TimbreFor(false, 44, 8u);
	const SpeechPlan Avg = PlanSpeech("Average is about 41,000.", Dealer, 0.6f, 1u);
	Expect(Avg.Syllables.size() >= 9 && Avg.Syllables.size() <= 13, "numbers are spoken out (41,000: forty-one thousand)", Fmt("%.0f syllables", static_cast<double>(Avg.Syllables.size())));
	Expect(PlanSpeech("Thirty BB.", Dealer, 0.6f, 1u).Syllables.size() == 4 && PlanSpeech("Thirty bb.", Dealer, 0.6f, 1u).Syllables.size() == 2,
		"an acronym is spelled out, letter by letter");
	Expect(PlanSpeech("Cards at 7:00 PM.", Dealer, 0.6f, 1u).Syllables.size() == PlanSpeech("Cards at 7 PM.", Dealer, 0.6f, 1u).Syllables.size() &&
			PlanSpeech("Cards at 7:06.", Dealer, 0.6f, 1u).Syllables.size() == PlanSpeech("Cards at 7.", Dealer, 0.6f, 1u).Syllables.size() + 2,
		"a clock reads as said: 7:00 is seven, 7:06 is seven-oh-six");
	const SpeechPlan Bubble = PlanSpeech("Players, we are on the bubble.", Dealer, 0.7f, 1u);
	double Gap = 0.0;
	for (size_t I = 1; I < Bubble.Syllables.size(); ++I)
	{
		Gap = std::max(Gap, static_cast<double>(Bubble.Syllables[I].Start - (Bubble.Syllables[I - 1].Start + Bubble.Syllables[I - 1].Length)));
	}
	Expect(Gap >= 0.12 && Gap <= 0.4, "a comma is a short pause", Fmt("%.2f s", Gap));
	const Timbre Woman = TimbreFor(true, 33, 2u);
	const SpeechPlan Ask = PlanSpeech("What's average?", Woman, 0.6f, 4u);
	const SpeechPlan Tell = PlanSpeech("That's average.", Woman, 0.6f, 4u);
	const float AskEnd = Ask.Syllables.back().Pitch;
	const float TellEnd = Tell.Syllables.back().Pitch;
	float AskBefore = 0.0f;
	for (size_t I = 0; I + 1 < Ask.Syllables.size(); ++I)
	{
		AskBefore = std::max(AskBefore, Ask.Syllables[I].Pitch);
	}
	Expect(AskEnd >= 1.15f && AskEnd > AskBefore + 0.05f, "a question rises at its end", Fmt("%.2f after at most %.2f", AskEnd, AskBefore));
	Expect(TellEnd <= 0.92f, "a statement falls at its end", Fmt("%.2f", TellEnd));
	{
		// The rise is in the sound, not only the plan.
		const std::vector<float> S = RenderSpeech(Ask, Woman, false, 3u);
		std::vector<double> F0s;
		for (const Syllable& Y : Ask.Syllables)
		{
			F0s.push_back(PitchAt(S, Y.Start + Y.Length * 0.55));
		}
		double Early = 0.0;
		int N = 0;
		for (size_t I = 0; I + 1 < F0s.size(); ++I)
		{
			if (F0s[I] > 0.0)
			{
				Early += F0s[I];
				++N;
			}
		}
		Early = N ? Early / N : 0.0;
		Expect(F0s.back() > 1.1 * Early && Early > 0.0, "the question's rise is heard", Fmt("%.0f Hz at the end, %.0f before", F0s.back(), Early));
	}
	// The jaw only moves inside syllables, and opens in every one.
	bool Inside = true, Opens = true;
	for (const char* Line : Lines)
	{
		const SpeechPlan P = PlanSpeech(Line, Dealer, 0.6f, 9u);
		for (double T = -0.5; T < P.Seconds + 0.5; T += 0.001)
		{
			const float M = MouthAt(P, static_cast<float>(T));
			bool In = false;
			for (const Syllable& S : P.Syllables)
			{
				In = In || (T >= S.Start && T < S.Start + S.Length);
			}
			Inside = Inside && (In || M == 0.0f) && M >= 0.0f && M <= 1.0f;
		}
		for (const Syllable& S : P.Syllables)
		{
			Opens = Opens && MouthAt(P, S.Start + 0.5f * S.Length) > 0.0f;
		}
	}
	Expect(Inside, "MouthAt is zero outside every syllable");
	Expect(Opens, "MouthAt opens in every syllable");
	// It sounds like a person: the pitch moves, the syllables aren't a metronome.
	{
		const SpeechPlan P = PlanSpeech(Lines[2], Dealer, 0.6f, 4u);
		double Mp = 0, Ml = 0;
		for (const Syllable& S : P.Syllables)
		{
			Mp += S.Pitch;
			Ml += S.Length;
		}
		Mp /= static_cast<double>(P.Syllables.size());
		Ml /= static_cast<double>(P.Syllables.size());
		double Vp = 0, Vl = 0;
		for (const Syllable& S : P.Syllables)
		{
			Vp += (S.Pitch - Mp) * (S.Pitch - Mp);
			Vl += (S.Length - Ml) * (S.Length - Ml);
		}
		const double Sp = std::sqrt(Vp / static_cast<double>(P.Syllables.size()));
		const double Cv = std::sqrt(Vl / static_cast<double>(P.Syllables.size())) / Ml;
		Expect(Sp >= 0.05 && Cv >= 0.12, "intonation and rhythm vary", Fmt("pitch spread %.2f, length variation %.2f", Sp, Cv));
	}
	// Effort: a call across the room is higher and louder than talk.
	{
		const SpeechPlan Talk = PlanSpeech("All in and a call, table four!", Dealer, 0.6f, 7u);
		const SpeechPlan Call = PlanSpeech("All in and a call, table four!", Dealer, 0.9f, 7u);
		double Pt = 0, Pc = 0, Gt = 0, Gc = 0;
		for (const Syllable& S : Talk.Syllables)
		{
			Pt += S.Pitch;
			Gt += S.Gain;
		}
		for (const Syllable& S : Call.Syllables)
		{
			Pc += S.Pitch;
			Gc += S.Gain;
		}
		Expect(Pc > Pt && Gc > Gt * 1.15, "effort raises the voice");
	}
	// Renders: finite, peak at most 1, deterministic, seeded.
	bool Fine = true, Same = true, Seeded = true;
	for (const char* Line : Lines)
	{
		for (int M = 0; M < 2; ++M)
		{
			const SpeechPlan P = PlanSpeech(Line, Woman, 0.6f, 2u);
			const std::vector<float> A = RenderSpeech(P, Woman, M == 1, 5u);
			const std::vector<float> B = RenderSpeech(P, Woman, M == 1, 5u);
			const std::vector<float> C = RenderSpeech(P, Woman, M == 1, 6u);
			float Peak = 0.0f;
			for (const float V : A)
			{
				Fine = Fine && std::isfinite(V);
				Peak = std::max(Peak, std::fabs(V));
			}
			Fine = Fine && Peak <= 1.0f && Peak > 0.05f && A.size() >= static_cast<size_t>(P.Seconds * SampleRate);
			Same = Same && A == B;
			Seeded = Seeded && A != C;
		}
	}
	Expect(Fine, "speech renders finite, audible, peak at most 1");
	Expect(Same, "speech renders are deterministic");
	Expect(Seeded, "the seed changes the voice's grain");
	Expect(PlanSpeech("", Dealer, 0.6f, 1u).Syllables.empty() && RenderSpeech(SpeechPlan(), Dealer, true, 1u).empty(), "an empty line is silence");
	Expect(PlanSpeech("Players, we are on the bubble.", Dealer, 0.6f, 3u).Seconds == PlanSpeech("Players, we are on the bubble.", Dealer, 0.6f, 3u).Seconds, "plans are deterministic");
	// The murmur is rounded off: little energy above 2.5 kHz next to the clear voice.
	{
		const SpeechPlan P = PlanSpeech(Lines[1], Dealer, 0.6f, 3u);
		const std::vector<float> Clear = RenderSpeech(P, Dealer, false, 1u);
		const std::vector<float> Murmur = RenderSpeech(P, Dealer, true, 1u);
		const double ClearHigh = BandEnergy(Clear, 2500.0, 12000.0) / BandEnergy(Clear, 50.0, 12000.0);
		const double MurmurHigh = BandEnergy(Murmur, 2500.0, 12000.0) / BandEnergy(Murmur, 50.0, 12000.0);
		Expect(MurmurHigh < 0.3 * ClearHigh, "the murmur is rounded off", Fmt("%.4f of its energy above 2.5 kHz against %.4f", MurmurHigh, ClearHigh));
		(void)ClearHigh;
	}
	// A clear voice sounds like a voice: its long-term spectrum tracks a man's LTASS (after Byrne et al. 1994) within 7 dB
	// a third-octave band from 250 Hz to 5 kHz (a cascade alone runs 10 dB dull up top; the source, the bandwidths and
	// the presence shelf put it back).
	{
		std::vector<float> All;
		uint32_t Seed = 1u;
		for (const char* Line : Lines)
		{
			const Timbre Man = TimbreFor(false, 30 + static_cast<int>(Seed * 5), Seed * 13u);
			const std::vector<float> S = RenderSpeech(PlanSpeech(Line, Man, 0.6f, Seed), Man, false, Seed);
			All.insert(All.end(), S.begin(), S.end());
			++Seed;
		}
		static const int Bands[] = {250, 315, 400, 500, 630, 800, 1000, 1250, 1600, 2000, 2500, 3150, 4000, 5000};
		static const double Ltass[] = {0, 0, -1, -2, -4, -6, -8, -9, -11, -14, -15, -17, -20, -22};
		double Level[14];
		for (int K = 0; K < 14; ++K)
		{
			Level[K] = Db(BandEnergy(All, Bands[K] / std::pow(2.0, 1.0 / 6.0), Bands[K] * std::pow(2.0, 1.0 / 6.0)));
		}
		const double Top = std::max(Level[0], std::max(Level[1], std::max(Level[2], Level[3])));
		double Worst = 0.0;
		int WorstBand = 0;
		for (int K = 0; K < 14; ++K)
		{
			const double D = Level[K] - Top - Ltass[K];
			if (std::fabs(D) > std::fabs(Worst))
			{
				Worst = D;
				WorstBand = Bands[K];
			}
		}
		Expect(std::fabs(Worst) <= 7.0, "a clear voice's spectrum tracks a man's LTASS within 7 dB, 250 Hz to 5 kHz", Fmt("worst %+.1f dB at %.0f Hz", Worst, WorstBand));
	}
	// Speech renders off the game thread in a few milliseconds (§10: at most 4 ms of worker time for a 3 s line).
	{
		const SpeechPlan P = PlanSpeech("Players, we are on the bubble. We're now hand for hand.", Dealer, 0.7f, 3u);
		double Best = 1.0e9;
		for (int R = 0; R < 5; ++R)
		{
			const auto A = std::chrono::steady_clock::now();
			const std::vector<float> S = RenderSpeech(P, Dealer, true, static_cast<uint32_t>(R));
			const auto B = std::chrono::steady_clock::now();
			Best = std::min(Best, std::chrono::duration<double, std::milli>(B - A).count());
		}
		const double Per3 = Best * 3.0 / std::max(0.5, static_cast<double>(P.Seconds));
		Expect(Per3 <= 4.0, "a 3 s line renders in at most 4 ms", Fmt("%.2f ms for 3 s", Per3));
	}
}

void Vocals()
{
	std::printf("vocals\n");
	static const char* Names[] = {"none", "laugh", "sigh", "scoff", "groan", "ooh", "gasp", "cheer", "clap", "applause"};
	const Timbre T = TimbreFor(false, 40, 3u);
	for (int V = 1; V <= 9; ++V)
	{
		for (const int N : {1, 6, 40})
		{
			const std::vector<float> A = RenderVocal(static_cast<Vocal>(V), T, N, 11u);
			const std::vector<float> B = RenderVocal(static_cast<Vocal>(V), T, N, 11u);
			Check(std::string("vocal_") + Names[V] + "_" + std::to_string(N), A, 0.02f);
			if (A != B)
			{
				Expect(false, std::string(Names[V]) + " is deterministic");
			}
		}
	}
	Expect(RenderVocal(Vocal::None, T, 3, 1u).empty(), "Vocal::None is silence");
	const std::vector<float> Clap = RenderVocal(Vocal::Clap, T, 6, 2u);
	const std::vector<float> Small = RenderVocal(Vocal::Applause, T, 10, 2u);
	const std::vector<float> Big = RenderVocal(Vocal::Applause, T, 120, 2u);
	const double Cs = static_cast<double>(Clap.size()) / SampleRate, As = static_cast<double>(Small.size()) / SampleRate, Bs = static_cast<double>(Big.size()) / SampleRate;
	Expect(Cs > 1.5 && Cs < 2.8, "light claps run about 2 s", Fmt("%.2f s", Cs));
	Expect(As > 2.8 && As < 4.2 && Bs > As && Bs < 6.6, "applause runs 3.2 s, longer for a big crowd", Fmt("%.2f s, %.2f s for 120", As, Bs));
	// The rail's ooh swells: its middle is louder than its start.
	const std::vector<float> Ooh = RenderVocal(Vocal::Ooh, T, 12, 3u);
	double Start = 0.0, Middle = 0.0;
	const size_t S0 = static_cast<size_t>(0.15 * SampleRate);
	for (size_t I = 0; I < S0 && I < Ooh.size(); ++I)
	{
		Start += static_cast<double>(Ooh[I]) * Ooh[I];
	}
	for (size_t I = static_cast<size_t>(0.55 * SampleRate); I < static_cast<size_t>(0.7 * SampleRate) && I < Ooh.size(); ++I)
	{
		Middle += static_cast<double>(Ooh[I]) * Ooh[I];
	}
	Expect(Middle > 2.0 * Start, "the ooh swells");
}

void PaChecks()
{
	std::printf("the PA\n");
	const Timbre Pa = PaTimbre(5u);
	const SpeechPlan P = PlanSpeech("Players, we are on the bubble. We're now hand for hand.", Pa, 0.7f, 5u);
	std::vector<float> S = RenderSpeech(P, Pa, false, 5u);
	const size_t Before = S.size();
	PaChain(S, 5u);
	const double In = BandEnergy(S, 250.0, 5000.0), All = BandEnergy(S, 1.0, 24000.0);
	Expect(In / All >= 0.85, "the PA keeps at least 85% of its energy in 250 Hz to 5 kHz", Fmt("%.1f%%", 100.0 * In / All));
	Expect(S.size() > Before + static_cast<size_t>(0.2 * SampleRate), "the ceiling's speakers ring on after the line");
	float Peak = 0.0f;
	bool Finite = true;
	for (const float V : S)
	{
		Peak = std::max(Peak, std::fabs(V));
		Finite = Finite && std::isfinite(V);
	}
	Expect(Finite && Peak <= 1.0f && Peak > 0.05f, "the PA is finite and within range", Fmt("peak %.3f", static_cast<double>(Peak)));
	std::vector<float> Again = RenderSpeech(P, Pa, false, 5u);
	PaChain(Again, 5u);
	Expect(Again == S, "the PA chain is deterministic");
	if (!OutDir.empty())
	{
		WriteWav(OutDir + "/pa_bubble.wav", S, 1);
	}
}

void Moods()
{
	std::printf("the room's moods\n");
	const Levels R = MoodLevels(Running, 24.0, 21u);
	const Levels H = MoodLevels(HandForHand, 24.0, 21u);
	const Levels B = MoodLevels(Break, 24.0, 21u);
	std::printf("         Running: bed %.1f talk %.1f chips %.1f | HandForHand: talk %.1f chips %.1f | Break: talk %.1f\n", R.Bed, R.Talk, R.Chips, H.Talk, H.Chips, B.Talk);
	Expect(R.Bed >= -34.0 && R.Bed <= -26.0, "Running's bed is -34 to -26 dBFS", Fmt("%.1f dBFS", R.Bed));
	Expect(H.Talk <= R.Talk - 8.0, "hand for hand, the talk is 8 dB or more under Running", Fmt("%.1f dB under", R.Talk - H.Talk));
	const double ChipsMove = H.Chips - R.Chips;
	Expect(std::fabs(ChipsMove - 3.0) <= 1.0, "hand for hand, the chips follow their level within 1 dB", Fmt("%+.1f dB for the mix's +3", ChipsMove));
	Expect(B.Talk >= R.Talk + 3.0, "on a break the talk is 3 dB or more over Running", Fmt("%+.1f dB", B.Talk - R.Talk));
	// The rail at the final table: it murmurs while it watches (it talks at its own level, not the hushed tables'), and an
	// ooh lifts it 6 dB or more over that.
	{
		CardRoom Room(31u);
		SeatAtTwelve(Room);
		SetMood(Room, FinalTable);
		std::vector<float> Buf(2400);
		double Murmur = 0.0;
		int N = 0;
		for (int I = 0; I < 1200; ++I)
		{
			Room.RenderStereo(Buf.data(), 1200);
			if (I >= 200 && I % 4 == 0)
			{
				Murmur += std::pow(10.0, Room.Measure().RailDb / 10.0);
				++N;
			}
		}
		const double Before = Db(Murmur / N);
		Room.Crowd(Vocal::Ooh, 12, CardRoom::Spot::Rail, 0, 1.0f);
		float After = -120.0f;
		for (int I = 0; I < 80; ++I)
		{
			Room.RenderStereo(Buf.data(), 1200);
			After = std::max(After, Room.Measure().RailDb);
		}
		Expect(Before > -60.0, "the rail murmurs while it watches", Fmt("%.1f dB", Before));
		Expect(After >= Before + 6.0, "the rail is 6 dB or more up after an ooh", Fmt("%.1f to %.1f dB", Before, static_cast<double>(After)));
	}
}

void Space()
{
	std::printf("the room as a space\n");
	const Timbre Dealer = TimbreFor(false, 44, 8u);
	const SpeechPlan Call = PlanSpeech("All in and a call, table four!", Dealer, 0.9f, 3u);
	// A call at 90 degrees to the right.
	{
		CardRoom Room(41u);
		Room.SetEmitters({TableAt(1, 0.0f, 120.0f, 0)});
		Room.SetListener(0.0f, 0.0f, 0.0f);
		Only(Room, 3);
		Play(Room, 2.0);
		Room.Say(Call, Dealer, CardRoom::Spot::Table, 1, 0.9f, false);
		const std::vector<float> S = Play(Room, 3.0);
		const double Lr = Db(StereoPower(S, 0, 0, S.size())) - Db(StereoPower(S, 1, 0, S.size()));
		Expect(Lr <= -9.0, "a call 90 degrees right is 9 dB or more down on the left", Fmt("L-R %.1f dB", Lr));
		// Turning around puts it on the left.
		CardRoom Back(41u);
		Back.SetEmitters({TableAt(1, 0.0f, 120.0f, 0)});
		Back.SetListener(0.0f, 0.0f, 180.0f);
		Only(Back, 3);
		Play(Back, 2.0);
		Back.Say(Call, Dealer, CardRoom::Spot::Table, 1, 0.9f, false);
		const std::vector<float> T = Play(Back, 3.0);
		const double Lr2 = Db(StereoPower(T, 0, 0, T.size())) - Db(StereoPower(T, 1, 0, T.size()));
		Expect(Lr2 >= 9.0, "turning around moves it to the left", Fmt("L-R %.1f dB", Lr2));
	}
	// Behind the head: 3 dB down and darker (a table's chips, the same hand in front and behind).
	{
		std::vector<float> Out[2];
		for (int K = 0; K < 2; ++K)
		{
			CardRoom Room(43u);
			Room.SetEmitters({TableAt(1, 150.0f, 0.0f, 7)});
			Room.SetListener(0.0f, 0.0f, K == 0 ? 0.0f : 180.0f);
			Only(Room, 2);
			Play(Room, 0.5);
			Room.TableHand(1, 20.0f, true, true, 40.0f);
			Out[K] = Mono(Play(Room, 20.0));
		}
		const double HighFront = BandEnergy(Out[0], 6000.0, 20000.0), HighBack = BandEnergy(Out[1], 6000.0, 20000.0);
		const double AllFront = BandEnergy(Out[0], 20.0, 20000.0), AllBack = BandEnergy(Out[1], 20.0, 20000.0);
		Expect(Db(HighBack) <= Db(HighFront) - 3.0, "behind the head, 3 dB or more down over 6 kHz", Fmt("%.1f dB", Db(HighBack) - Db(HighFront)));
		Expect(Db(AllBack) <= Db(AllFront) - 1.5 && Db(AllBack) >= Db(AllFront) - 8.0, "behind the head, about 3 dB down overall", Fmt("%.1f dB", Db(AllBack) - Db(AllFront)));
	}
	// Your own table (Players 0) is the table actor's: its hands play nothing in the room, not even in the far scatter.
	{
		CardRoom Room(44u);
		Room.SetEmitters({TableAt(1, 150.0f, 0.0f, 0)});
		Room.SetListener(0.0f, 0.0f, 0.0f);
		Only(Room, 2);
		Play(Room, 2.0);
		float Loudest = -120.0f;
		std::vector<float> Buf(2400);
		for (int B = 0; B < 800; ++B)
		{
			if (B % 400 == 0)
			{
				Room.TableHand(1, 10.0f, true, true, 40.0f);
			}
			Room.RenderStereo(Buf.data(), 1200);
			Loudest = std::max(Loudest, Room.Measure().ChipsDb);
		}
		Expect(Loudest <= -119.0f, "your own table's hands are left to the table actor", Fmt("chips at most %.1f dB", static_cast<double>(Loudest)));
	}
	// Distance: 20 m is 14 dB or more under 2 m.
	{
		double Power[2];
		for (int K = 0; K < 2; ++K)
		{
			CardRoom Room(45u);
			Room.SetEmitters({TableAt(1, K == 0 ? 200.0f : 2000.0f, 0.0f, 0)});
			Room.SetListener(0.0f, 0.0f, 0.0f);
			Only(Room, 3);
			Play(Room, 1.0);
			Room.Say(Call, Dealer, CardRoom::Spot::Table, 1, 0.9f, false);
			const std::vector<float> S = Play(Room, 4.0);
			Power[K] = StereoPower(S, 2, 0, S.size());
		}
		Expect(Db(Power[1]) <= Db(Power[0]) - 14.0, "20 m is 14 dB or more under 2 m", Fmt("%.1f dB", Db(Power[0]) - Db(Power[1])));
	}
	// The reverb: RT60 0.5 to 0.9 s, from a short call's decay (Schroeder's backward integral, T20).
	{
		CardRoom Room(47u);
		Room.SetEmitters({TableAt(1, 300.0f, 0.0f, 0)});
		Room.SetListener(0.0f, 0.0f, 0.0f);
		Only(Room, 3);
		Play(Room, 1.0);
		const SpeechPlan Hey = PlanSpeech("Hey!", Dealer, 1.0f, 4u);
		Room.Say(Hey, Dealer, CardRoom::Spot::Table, 1, 1.0f, false);
		const std::vector<float> M = Mono(Play(Room, 3.0));
		const size_t From = static_cast<size_t>((Hey.Seconds + 0.1) * SampleRate);
		std::vector<double> Edc(M.size() - From);
		double Sum = 0.0;
		for (size_t I = M.size(); I-- > From;)
		{
			Sum += static_cast<double>(M[I]) * M[I];
			Edc[I - From] = Sum;
		}
		const double Top = Edc[0];
		double T5 = -1.0, T25 = -1.0;
		for (size_t I = 0; I < Edc.size(); ++I)
		{
			const double D = Db(Edc[I] / Top);
			if (T5 < 0.0 && D <= -5.0)
			{
				T5 = static_cast<double>(I) / SampleRate;
			}
			if (T25 < 0.0 && D <= -25.0)
			{
				T25 = static_cast<double>(I) / SampleRate;
			}
		}
		const double Rt60 = T5 >= 0.0 && T25 > T5 ? 3.0 * (T25 - T5) : -1.0;
		Expect(Rt60 >= 0.5 && Rt60 <= 0.9, "the room's RT60 is 0.5 to 0.9 s", Fmt("%.2f s", Rt60));
	}
	// Applause "from the PA": the whole room, around the listener, not from one side.
	{
		CardRoom Room(48u);
		Room.SetEmitters(Nightly(14, 12));
		Room.SetListener(1310.0f, -525.0f, 0.0f);
		Room.SetMix(-120.0f, -120.0f, -120.0f, -120.0f, 0.0f, 0.7f);
		Play(Room, 1.0);
		Room.Crowd(Vocal::Applause, 40, CardRoom::Spot::Pa, 0, 1.0f);
		const std::vector<float> S = Play(Room, 4.0);
		const double Lr = Db(StereoPower(S, 0, 0, S.size())) - Db(StereoPower(S, 1, 0, S.size()));
		Expect(std::fabs(Lr) <= 3.0 && StereoPower(S, 2, 0, S.size()) > 1.0e-8, "applause all around is heard on both sides", Fmt("L-R %.1f dB", Lr));
	}
	// The PA in the room: no direction, and in its band.
	{
		CardRoom Room(49u);
		Room.SetEmitters(Nightly(14, 12));
		Room.SetListener(1310.0f, -525.0f, 0.0f);
		Room.SetMix(-120.0f, -120.0f, -120.0f, -120.0f, -120.0f, 0.7f);
		Play(Room, 1.0);
		const Timbre Pa = PaTimbre(2u);
		Room.Say(PlanSpeech("We're down to our last two tables. Sixteen players left.", Pa, 0.8f, 6u), Pa, CardRoom::Spot::Pa, 0, 0.8f, false);
		const std::vector<float> S = Play(Room, 4.5);
		const double Lr = Db(StereoPower(S, 0, 0, S.size())) - Db(StereoPower(S, 1, 0, S.size()));
		const std::vector<float> M = Mono(S);
		const double Band = BandEnergy(M, 250.0, 5000.0) / BandEnergy(M, 1.0, 24000.0);
		Expect(std::fabs(Lr) <= 1.5, "the PA has no direction", Fmt("L-R %.1f dB", Lr));
		Expect(Band >= 0.85, "the PA in the room stays in its band", Fmt("%.1f%%", 100.0 * Band));
		Expect(Room.Measure().PaDb > -60.0f, "the PA meters");
	}
	// The PA's level (SetPaLevel, the Voices slider): -12 dB is about 12 dB quieter as heard; 0 dB is the room as mixed,
	// sample for sample; -120 is silent.
	{
		const Timbre Pa = PaTimbre(2u);
		const SpeechPlan Line = PlanSpeech("We're down to our last two tables. Sixteen players left.", Pa, 0.8f, 6u);
		std::vector<float> Heard[4];
		const float PaLevels[4] = {0.0f, 0.0f, -12.0f, -120.0f};
		for (int K = 0; K < 4; ++K)
		{
			CardRoom Room(49u);
			Room.SetEmitters(Nightly(14, 12));
			Room.SetListener(1310.0f, -525.0f, 0.0f);
			Room.SetMix(-120.0f, -120.0f, -120.0f, -120.0f, -120.0f, 0.7f);
			Play(Room, 1.0);
			if (K > 0)
			{
				Room.SetPaLevel(PaLevels[K]);
			}
			Room.Say(Line, Pa, CardRoom::Spot::Pa, 0, 0.8f, false);
			Heard[K] = Play(Room, 4.5);
		}
		const double Base = Db(StereoPower(Heard[0], 2, 0, Heard[0].size()));
		const double Down = Db(StereoPower(Heard[2], 2, 0, Heard[2].size())) - Base;
		const double Off = Db(StereoPower(Heard[3], 2, 0, Heard[3].size())) - Base;
		Expect(Heard[1] == Heard[0], "the PA at 0 dB is the room as mixed, sample for sample");
		Expect(Down <= -10.5 && Down >= -13.5, "the PA's level moves the PA as heard", Fmt("%.1f dB at -12", Down));
		Expect(Off <= -40.0, "the PA's level at -120 silences it", Fmt("%.1f dB", Off));
	}
}

void Ducks()
{
	std::printf("ducking\n");
	const Timbre Dealer = TimbreFor(false, 44, 8u);
	const SpeechPlan Line = PlanSpeech("Seat four, it's on you.", Dealer, 0.6f, 3u);
	// A line at your table: the bed is 3 dB down within 80 ms of the line's first sample (the same stream, so no lag).
	{
		CardRoom A(51u), B(51u);
		for (CardRoom* R : {&A, &B})
		{
			R->SetEmitters({TableAt(1, 100.0f, 50.0f, 0)});
			R->SetListener(0.0f, 0.0f, 0.0f);
			R->SetMix(0.0f, -120.0f, -120.0f, -120.0f, -120.0f, 0.7f);
			Play(*R, 1.0);
		}
		A.Say(Line, Dealer, CardRoom::Spot::Table, 1, 0.6f, false);
		const std::vector<float> Sa = Play(A, 0.5), Sb = Play(B, 0.5);
		auto Ratio = [&](double From, double To) {
			const size_t F = static_cast<size_t>(From * SampleRate), T = static_cast<size_t>(To * SampleRate);
			return Db(StereoPower(Sa, 2, F, T)) - Db(StereoPower(Sb, 2, F, T));
		};
		const double At80 = Ratio(0.07, 0.09);
		Expect(At80 <= -2.5 && At80 >= -3.6, "a line at your table ducks the bed 3 dB within 80 ms", Fmt("%.2f dB at 70-90 ms", At80));
		Expect(std::fabs(Ratio(0.0, 0.004)) < 0.6, "the duck starts with the line, not before", Fmt("%.2f dB in the first 4 ms", Ratio(0.0, 0.004)));
	}
	// The host's Duck: the room's talk by the depth asked.
	{
		CardRoom A(53u), B(53u);
		for (CardRoom* R : {&A, &B})
		{
			R->SetEmitters(Nightly(14, 12));
			R->SetListener(1310.0f, -525.0f, 0.0f);
			R->SetMix(-120.0f, 0.0f, -120.0f, -120.0f, -120.0f, 0.7f);
			Play(*R, 2.0);
		}
		A.Duck(-6.0f, 1.0f);
		const std::vector<float> Sa = Play(A, 2.5), Sb = Play(B, 2.5);
		auto Ratio = [&](double From, double To) {
			const size_t F = static_cast<size_t>(From * SampleRate), T = static_cast<size_t>(To * SampleRate);
			return Db(StereoPower(Sa, 2, F, T)) - Db(StereoPower(Sb, 2, F, T));
		};
		Expect(Ratio(0.07, 0.09) <= -4.5, "Duck(-6) takes the talk down within 80 ms", Fmt("%.2f dB at 70-90 ms", Ratio(0.07, 0.09)));
		Expect(Ratio(0.2, 1.0) <= -5.5, "and holds it", Fmt("%.2f dB", Ratio(0.2, 1.0)));
		Expect(std::fabs(Ratio(2.0, 2.5)) < 0.5, "and lets it go", Fmt("%.2f dB", Ratio(2.0, 2.5)));
	}
	// The PA ducks the room's talk while it's heard at all (a quiet PA, the Voices slider down, still does); a silenced PA
	// (SetPaLevel -120, Voices at 0) leaves the room as it was.
	{
		const Timbre Pa = PaTimbre(2u);
		const SpeechPlan Call = PlanSpeech("Players, we are on the bubble. We're now hand for hand.", Pa, 0.85f, 7u);
		double Under[2] = {0.0, 0.0};
		const float PaLevels[2] = {-60.0f, -120.0f};
		for (int K = 0; K < 2; ++K)
		{
			CardRoom A(57u), B(57u);
			for (CardRoom* R : {&A, &B})
			{
				R->SetEmitters(Nightly(14, 12));
				R->SetListener(1310.0f, -525.0f, 0.0f);
				R->SetMix(-120.0f, 0.0f, -120.0f, -120.0f, -120.0f, 0.7f);
				R->SetPaLevel(PaLevels[K]);
				Play(*R, 2.0);
			}
			A.Say(Call, Pa, CardRoom::Spot::Pa, 0, 0.85f, false);
			const std::vector<float> Sa = Play(A, 2.0), Sb = Play(B, 2.0);
			const size_t F = static_cast<size_t>(0.3 * SampleRate), T = static_cast<size_t>(1.5 * SampleRate);
			Under[K] = Db(StereoPower(Sa, 2, F, T)) - Db(StereoPower(Sb, 2, F, T));
		}
		Expect(Under[0] <= -3.0, "a quiet PA still ducks the room's talk", Fmt("%.2f dB at -60", Under[0]));
		Expect(std::fabs(Under[1]) < 0.5, "a silenced PA leaves the room's talk alone", Fmt("%.2f dB at -120", Under[1]));
	}
	// A murmured line at your table ducks the room's talk, never itself: from the table (which ducks) it's as loud as the
	// same line from the bar's spot in the same place (which doesn't). Each line is heard alone: the room with it, less
	// the same room without it but ducked the same way (a line never changes the room's other voices).
	{
		const Timbre Rick = TimbreFor(false, 51, 6u);
		const SpeechPlan Needle = PlanSpeech("You can have the next one, champ.", Rick, 0.9f, 2u);
		double Heard[2] = {0.0, 0.0};
		for (int K = 0; K < 2; ++K)
		{
			CardRoom A(55u), B(55u);
			for (CardRoom* R : {&A, &B})
			{
				CardRoom::Emitter Bar;
				Bar.Kind = CardRoom::Spot::Bar;
				Bar.X = 100.0f;
				Bar.Y = 50.0f;
				R->SetEmitters({TableAt(1, 100.0f, 50.0f, 0), Bar});
				R->SetListener(0.0f, 0.0f, 0.0f);
				R->SetMix(-120.0f, 0.0f, -120.0f, -120.0f, -120.0f, 0.7f);
				Play(*R, 2.0);
			}
			A.Say(Needle, Rick, K == 0 ? CardRoom::Spot::Table : CardRoom::Spot::Bar, K == 0 ? 1 : 0, 0.9f, true);
			if (K == 0)
			{
				B.Duck(-6.0f, Needle.Seconds - 0.1f); // the talk's duck the table's line brings, without the line
			}
			const std::vector<float> Sa = Play(A, Needle.Seconds), Sb = Play(B, Needle.Seconds);
			const size_t From = static_cast<size_t>(0.1 * SampleRate);
			std::vector<float> Alone(Sa.size());
			for (size_t I = 0; I < Alone.size(); ++I)
			{
				Alone[I] = Sa[I] - Sb[I];
			}
			Heard[K] = StereoPower(Alone, 2, From, Alone.size() / 2);
		}
		Expect(Heard[0] > 0.0 && std::fabs(Db(Heard[0]) - Db(Heard[1])) <= 0.5, "a line at your table isn't ducked by its own duck",
			Fmt("%+.1f dB against the same line unducked", Db(Heard[0]) - Db(Heard[1])));
	}
}

void Controls()
{
	std::printf("muffle, pace, listener\n");
	// Tunnel hearing: the room through a 2.5 kHz low-pass and -4 dB; back when it opens.
	{
		CardRoom A(61u), B(61u);
		for (CardRoom* R : {&A, &B})
		{
			SeatAtTwelve(*R);
			SetMood(*R, Running);
			Play(*R, 2.0);
		}
		A.SetMuffle(2500.0f, -4.0f);
		Play(A, 1.0);
		Play(B, 1.0);
		const std::vector<float> Ma = Mono(Play(A, 6.0)), Mb = Mono(Play(B, 6.0));
		const double High = Db(BandEnergy(Ma, 4000.0, 20000.0)) - Db(BandEnergy(Mb, 4000.0, 20000.0));
		const double All = Db(BandEnergy(Ma, 20.0, 20000.0)) - Db(BandEnergy(Mb, 20.0, 20000.0));
		Expect(High <= -10.0 && All <= -4.0, "muffling darkens and lowers the room", Fmt("%.1f dB over 4 kHz, %.1f dB overall", High, All));
		A.SetMuffle(20000.0f, 0.0f);
		Play(A, 1.5);
		Play(B, 1.5);
		const std::vector<float> Oa = Play(A, 3.0), Ob = Play(B, 3.0);
		Expect(Oa == Ob, "opening the muffle gives the same room back, sample for sample");
	}
	// Pace: the room's events on the dilated clock.
	{
		double Chips[2];
		for (int K = 0; K < 2; ++K)
		{
			CardRoom Room(63u);
			SeatAtTwelve(Room);
			Only(Room, 2);
			Room.SetPace(K == 0 ? 0.45f : 1.5f);
			std::vector<float> S(static_cast<size_t>(20 * SampleRate) * 2);
			for (int B = 0; B < 800; ++B)
			{
				Hands(Room, B, 14);
				Room.RenderStereo(S.data() + static_cast<size_t>(B) * 2400, 1200);
			}
			Chips[K] = StereoPower(S, 2, 0, S.size() / 2);
		}
		Expect(Db(Chips[1]) >= Db(Chips[0]) + 2.0, "the room's events follow its pace", Fmt("%.1f dB more at 1.5x than 0.45x", Db(Chips[1]) - Db(Chips[0])));
	}
	// SetEmitters every frame with the same list changes nothing; the room is a pure function of its calls and seed.
	{
		CardRoom A(65u), B(65u), C(66u);
		for (CardRoom* R : {&A, &B, &C})
		{
			SeatAtTwelve(*R);
			SetMood(*R, Running);
		}
		std::vector<float> Sa, Sb, Sc;
		std::vector<float> Buf(2400);
		for (int I = 0; I < 400; ++I)
		{
			for (CardRoom* R : {&A, &B, &C})
			{
				Hands(*R, I, 14);
			}
			B.SetEmitters(Nightly(14, 12));
			B.SetListener(1310.0f, -525.0f, 0.0f);
			A.RenderStereo(Buf.data(), 1200);
			Sa.insert(Sa.end(), Buf.begin(), Buf.end());
			B.RenderStereo(Buf.data(), 1200);
			Sb.insert(Sb.end(), Buf.begin(), Buf.end());
			C.RenderStereo(Buf.data(), 1200);
			Sc.insert(Sc.end(), Buf.begin(), Buf.end());
		}
		Expect(Sa == Sb, "the same calls give the same stream (re-sent emitters included)");
		Expect(Sa != Sc, "the seed changes the room");
		// Odd render sizes give the same samples as the host's 25 ms blocks.
		CardRoom D(65u);
		SeatAtTwelve(D);
		SetMood(D, Running);
		std::vector<float> Sd;
		const int Sizes[] = {1, 7, 240, 333, 1200, 919};
		size_t Frames = 0, Want = Sa.size() / 2;
		int K = 0;
		int Block = 0;
		size_t NextHands = 0;
		while (Frames < Want)
		{
			while (NextHands <= Frames && Block < 400)
			{
				Hands(D, Block, 14);
				++Block;
				NextHands = static_cast<size_t>(Block) * 1200;
			}
			const size_t Take = std::min(static_cast<size_t>(Sizes[K++ % 6]), std::min(Want - Frames, NextHands - Frames));
			std::vector<float> Part(Take * 2);
			D.RenderStereo(Part.data(), static_cast<int>(Take));
			Sd.insert(Sd.end(), Part.begin(), Part.end());
			Frames += Take;
		}
		Expect(Sd == Sa, "any render size gives the same samples");
	}
	// Memory and voices.
	{
		CardRoom Room(67u);
		Room.SetVoiceBudget(6); // the dev flag's talkers: more state than the fallback that ships (budget 0) ever holds
		SeatAtTwelve(Room);
		SetMood(Room, Break);
		std::vector<float> Buf(2400);
		int MaxVoiced = 0, MaxTables = 0;
		for (int B = 0; B < 600; ++B)
		{
			Hands(Room, B, 14);
			Room.RenderStereo(Buf.data(), 1200);
			const CardRoom::Meter M = Room.Measure();
			MaxVoiced = std::max(MaxVoiced, M.Voiced);
			MaxTables = std::max(MaxTables, M.Tables);
		}
		const CardRoom::Meter M = Room.Measure();
		Expect(MaxTables <= 5, "at most 5 tables voiced (the 4 nearest and the feature table)", Fmt("%.0f", static_cast<double>(MaxTables)));
		Expect(MaxVoiced <= 6 + 5 + 4 + 8, "voices stay inside the budget", Fmt("%.0f at most", static_cast<double>(MaxVoiced)));
		Expect(M.Bytes > 0 && M.Bytes <= 64 * 1024, "the room's state is under 64 KB", Fmt("%.1f KB", M.Bytes / 1024.0));
		std::printf("         state %.1f KB (6 talkers)\n", M.Bytes / 1024.0);
		// The floor's longest lines (four at once) and a spell at 12 talkers leave it there once they're done.
		const Timbre Floor = PaTimbre(3u);
		const SpeechPlan Long = PlanSpeech("Ladies and gentlemen, your final table. In seat one, with 412,000, Rick. In seat two, two-time Embercrest "
										   "champion, Mei, with 1,036,000. In seat three, with 288,000, Sal.", Floor, 0.85f, 4u);
		Room.SetVoiceBudget(12);
		for (int K = 0; K < 4; ++K)
		{
			Room.Say(Long, Floor, K == 0 ? CardRoom::Spot::Pa : CardRoom::Spot::Table, 3 + K, 0.85f, K == 3);
		}
		int Peak = 0;
		for (int B = 0; B < static_cast<int>(Long.Seconds * 40.0f) + 20; ++B)
		{
			Room.RenderStereo(Buf.data(), 1200);
			Peak = std::max(Peak, Room.Measure().Bytes);
		}
		Room.SetVoiceBudget(6);
		Play(Room, 1.0);
		const int After = Room.Measure().Bytes;
		Expect(After <= 64 * 1024 && Peak > After, "long lines and 12 talkers come and go; the state is back under 64 KB",
			Fmt("%.1f KB at most, %.1f KB after", Peak / 1024.0, After / 1024.0));
	}
}

/** A host's bug (a NaN from a camera or a level) is left out: the stream stays finite and the room carries on. */
void Robust()
{
	std::printf("robustness\n");
	const float Nan = std::nanf("");
	const float Inf = INFINITY;
	CardRoom Room(59u);
	SeatAtTwelve(Room);
	SetMood(Room, Running);
	Play(Room, 1.0);
	Room.SetMix(Nan, Inf, Nan, -Inf, Nan, Nan);
	Room.SetListener(Nan, -525.0f, 0.0f);
	Room.SetListener(1310.0f, -525.0f, Inf);
	Room.SetMuffle(Nan, Nan);
	Room.SetPace(Nan);
	Room.Duck(Nan, 1.0f);
	Room.Duck(-6.0f, Inf);
	Room.Crowd(Vocal::Ooh, 6, CardRoom::Spot::Rail, 0, Nan);
	std::vector<CardRoom::Emitter> Bad = Nightly(14, 12);
	Bad[3].X = Nan;
	Bad[5].Y = Inf;
	Room.SetEmitters(Bad);
	const Timbre Dealer = TimbreFor(false, 44, 8u);
	Timbre Broken = Dealer;
	Broken.F0 = Nan;
	Broken.Tract = Inf;
	Room.Say(PlanSpeech("Seat open, nine!", Dealer, 0.9f, 1u), Broken, CardRoom::Spot::Table, 9, Nan, false);
	SpeechPlan Junk = PlanSpeech("Seat open, nine!", Dealer, 0.9f, 1u);
	Junk.Syllables[1].F2 = Nan;
	Room.Say(Junk, Dealer, CardRoom::Spot::Table, 9, 0.9f, false);
	Room.TableHand(2, Nan, true, true, Nan);
	Room.TableHand(3, Inf, true, true, Inf);
	const std::vector<float> S = Play(Room, 6.0);
	bool Finite = true;
	for (const float V : S)
	{
		Finite = Finite && std::isfinite(V);
	}
	const CardRoom::Meter M = Room.Measure();
	Expect(Finite && StereoPower(S, 2, 0, S.size()) > 1.0e-7 && std::isfinite(M.OutDb), "non-numbers from the host are left out: the room plays on, finite");
}

/** The fallback (§4.10, SetVoiceBudget(0)): a murmured line is only a soft breath, well under the talkers' murmur. */
void Fallback()
{
	std::printf("the fallback\n");
	const Timbre Boots = TimbreFor(false, 33, 4u);
	const SpeechPlan Line = PlanSpeech("Driver took the switchbacks like he owed somebody money.", Boots, 0.6f, 3u);
	double Heard[2] = {0.0, 0.0};
	int Voiced[2] = {0, 0};
	for (int K = 0; K < 2; ++K)
	{
		CardRoom A(91u), B(91u);
		for (CardRoom* R : {&A, &B})
		{
			R->SetVoiceBudget(K == 0 ? 6 : 0);
			SeatAtTwelve(*R);
			SetMood(*R, Running);
			Play(*R, 2.0);
		}
		A.Say(Line, Boots, CardRoom::Spot::Table, 13, 0.6f, true);
		const std::vector<float> Sa = Play(A, Line.Seconds), Sb = Play(B, Line.Seconds);
		std::vector<float> Alone(Sa.size() / 2);
		for (size_t I = 0; I < Alone.size(); ++I)
		{
			Alone[I] = 0.5f * ((Sa[2 * I] - Sb[2 * I]) + (Sa[2 * I + 1] - Sb[2 * I + 1]));
		}
		double P = 0.0;
		for (const float V : Alone)
		{
			P += static_cast<double>(V) * V;
		}
		Heard[K] = P / static_cast<double>(std::max<size_t>(1, Alone.size()));
		for (const Syllable& Y : Line.Syllables)
		{
			// Voiced: the speaker's own pitch is there (a whisper's only "pitch" is its first formant ringing).
			const double F0 = PitchAt(Alone, Y.Start + Y.Length * 0.5), Want = static_cast<double>(Boots.F0 * Y.Pitch);
			Voiced[K] += F0 > 0.75 * Want && F0 < 1.25 * Want ? 1 : 0;
		}
	}
	const int N = static_cast<int>(Line.Syllables.size());
	Expect(Heard[1] > 0.0 && Db(Heard[1]) <= Db(Heard[0]) - 6.0, "the fallback's line is a soft breath, 6 dB or more under the murmur",
		Fmt("%.1f dB under", Db(Heard[0]) - Db(Heard[1])));
	Expect(Voiced[0] >= N / 2 && Voiced[1] <= Voiced[0] / 3, "and it carries no voice", Fmt("%.0f of the syllables voiced against %.0f", Voiced[1], Voiced[0]));
}

/**
 * The busiest night the room renders: the burst's crowd at a full room, the engine dealing, calls, the PA and the rail, and
 * the listener on the move (walking the aisle and looking around, so every place is re-aimed every block).
 */
void Busiest(CardRoom& Room, int Block, const SpeechPlan& Call, const Timbre& Dealer, const SpeechPlan& Pa, const Timbre& Floor)
{
	const float Walk = static_cast<float>(Block) * 0.025f;
	Room.SetListener(1310.0f + 300.0f * std::sin(0.11f * Walk), -525.0f + 400.0f * std::sin(0.07f * Walk), 40.0f * std::sin(0.5f * Walk));
	Hands(Room, Block, 14);
	if (Block % 400 == 100)
	{
		Room.Say(Call, Dealer, CardRoom::Spot::Table, 3 + (Block / 400) % 10, 0.9f, false);
	}
	if (Block % 600 == 300)
	{
		Room.Say(Pa, Floor, CardRoom::Spot::Pa, 0, 0.8f, false);
	}
	if (Block % 800 == 500)
	{
		Room.Crowd(Vocal::Applause, 30, CardRoom::Spot::Rail, 0, 0.8f);
		Room.Crowd(Vocal::Ooh, 12, CardRoom::Spot::Rail, 0, 1.0f);
	}
}

void Speed()
{
	std::printf("speed\n");
	const Timbre Dealer = TimbreFor(false, 44, 8u);
	const Timbre Floor = PaTimbre(2u);
	const SpeechPlan Call = PlanSpeech("All in and a call, table four!", Dealer, 0.9f, 3u);
	const SpeechPlan Pa = PlanSpeech("We have an all-in and a call at table four.", Floor, 0.8f, 5u);
	// 60 s in the host's 25 ms blocks, five times. The whole run's cost is the thread's own CPU time (the best run);
	// each block's is its fastest wall time over the runs (the rest is the machine: other processes, a cold cache).
	const int Blocks = 2400;
	std::vector<double> Best(static_cast<size_t>(Blocks), 1.0e9);
	double BestTotal = 1.0e9, BestWall = 1.0e9;
	std::vector<float> Buf(2400);
	for (int Run = 0; Run < 5; ++Run)
	{
		CardRoom Room(71u);
		SeatAtTwelve(Room);
		SetMood(Room, BurstMood);
		double Wall = 0.0;
		const double Cpu0 = ThreadMs();
		for (int B = 0; B < Blocks; ++B)
		{
			const auto A = std::chrono::steady_clock::now();
			Busiest(Room, B, Call, Dealer, Pa, Floor);
			Room.RenderStereo(Buf.data(), 1200);
			const auto Z = std::chrono::steady_clock::now();
			const double Ms = std::chrono::duration<double, std::milli>(Z - A).count();
			Best[static_cast<size_t>(B)] = std::min(Best[static_cast<size_t>(B)], Ms);
			Wall += Ms;
		}
		BestTotal = std::min(BestTotal, ThreadMs() - Cpu0);
		BestWall = std::min(BestWall, Wall);
	}
	double Sum = 0.0, Peak = 0.0;
	for (int B = 0; B < Blocks; ++B)
	{
		Sum += Best[static_cast<size_t>(B)];
		if (B > 0)
		{
			Peak = std::max(Peak, Best[static_cast<size_t>(B)] + Best[static_cast<size_t>(B - 1)]);
		}
	}
	// At 60 fps a frame renders 16.7 ms of the stream on average; at worst two 25 ms blocks land in one frame.
	const double PerFrame = Sum / Blocks * (1000.0 / 60.0) / 25.0;
	std::printf("         60 s in %.0f ms of the thread's time (%.0f ms on the wall); per frame %.3f ms average, %.3f ms at worst (two blocks)\n", BestTotal, BestWall, PerFrame, Peak);
	Expect(BestTotal < 250.0, "60 s of the busiest mood renders in under 250 ms", Fmt("%.0f ms", BestTotal));
	Expect(PerFrame <= 0.35, "the game thread's average is 0.35 ms a frame or less", Fmt("%.3f ms", PerFrame));
	Expect(Peak <= 0.8, "the worst frame is 0.8 ms or less", Fmt("%.3f ms", Peak));
}

void Threads()
{
	std::printf("background-safe\n");
	// Speech renders on workers: the same as on this thread, while another worker renders too.
	const Timbre T = TimbreFor(true, 30, 4u);
	const SpeechPlan P = PlanSpeech(Lines[2], T, 0.6f, 4u);
	const std::vector<float> Here = RenderSpeech(P, T, true, 9u);
	std::vector<float> There[2];
	std::thread A([&]() { There[0] = RenderSpeech(P, T, true, 9u); });
	std::thread B([&]() { There[1] = RenderSpeech(P, T, true, 9u); });
	A.join();
	B.join();
	Expect(There[0] == Here && There[1] == Here, "speech renders the same on any thread");
	// Two rooms on two threads, each its own stream: no state shared between them.
	auto Room = []() {
		CardRoom R(81u);
		SeatAtTwelve(R);
		SetMood(R, Running);
		std::vector<float> Out;
		std::vector<float> Buf(2400);
		for (int I = 0; I < 200; ++I)
		{
			Hands(R, I, 14);
			R.RenderStereo(Buf.data(), 1200);
			Out.insert(Out.end(), Buf.begin(), Buf.end());
		}
		return Out;
	};
	const std::vector<float> Solo = Room();
	std::vector<float> Pair[2];
	std::thread C([&]() { Pair[0] = Room(); });
	std::thread D([&]() { Pair[1] = Room(); });
	C.join();
	D.join();
	Expect(Pair[0] == Solo && Pair[1] == Solo, "two rooms on two threads render as one does alone");
}

// ------------------------------------------------------------------ the listening gate (§4.10)

/** Today's mono bed (game/CardRoomAmbience.h), ported as it is, for an A/B beside the new room. */
class TodayBed
{
public:
	explicit TodayBed(uint32_t Seed) : State(Seed | 1u)
	{
		for (Voice& V : Voices)
		{
			V.Center = 380.0 + 900.0 * Uniform();
			V.NextPhrase = 4.0 * Uniform();
			V.Pan = 0.4 + 0.6 * Uniform();
		}
	}
	void SetCrowd(float C) { Crowd = std::min(1.0, std::max(0.15, static_cast<double>(C))); }
	void Render(float* Out, int N)
	{
		const double Pi = 3.14159265358979323846;
		const double Rate = 48000.0, Dt = 1.0 / Rate;
		const double AirK = 1.0 - std::exp(-2.0 * Pi * 90.0 * Dt);
		const double ClickK = 1.0 - std::exp(-2.0 * Pi * 5200.0 * Dt);
		for (int I = 0; I < N; ++I)
		{
			Time += Dt;
			double S = 0.0;
			Brown = Brown * 0.996 + Noise() * 0.04;
			Air += (Brown - Air) * AirK;
			S += Air * 0.55;
			const int Talking = std::min(NumVoices, std::max(3, static_cast<int>(3.0 + 7.0 * Crowd)));
			for (int K = 0; K < NumVoices; ++K)
			{
				Voice& V = Voices[K];
				if (Time >= V.NextPhrase)
				{
					V.On = !V.On && K < Talking;
					V.NextPhrase = Time + (V.On ? 1.5 + 4.5 * Uniform() : 1.0 + 7.0 * Uniform());
				}
				if (Time >= V.NextSyllable)
				{
					V.Target = V.On ? 0.3 + 0.7 * Uniform() : 0.0;
					V.NextSyllable = Time + 0.07 + 0.18 * Uniform();
					V.Center = std::min(1500.0, std::max(300.0, V.Center * (0.93 + 0.14 * Uniform())));
				}
				V.Gain += (V.Target - V.Gain) * 0.0012;
				const double Lk = 1.0 - std::exp(-2.0 * Pi * V.Center * 1.6 * Dt);
				const double Hk = 1.0 - std::exp(-2.0 * Pi * V.Center * 0.55 * Dt);
				const double X = Noise();
				V.Low += (X - V.Low) * Lk;
				V.High += (V.Low - V.High) * Hk;
				S += (V.Low - V.High) * V.Gain * 0.11 * V.Pan;
			}
			if (Time >= NextLaugh)
			{
				LaughAt = Time;
				NextLaugh = Time + (8.0 + 22.0 * Uniform()) / Crowd;
			}
			const double SinceLaugh = Time - LaughAt;
			if (SinceLaugh < 1.1)
			{
				const double Ha = std::max(0.0, std::sin(2.0 * Pi * 5.5 * SinceLaugh));
				LaughBand += (Noise() - LaughBand) * 0.08;
				S += LaughBand * Ha * Ha * 0.05 * (1.0 - SinceLaugh / 1.1);
			}
			if (Time >= NextRiffle)
			{
				RiffleLeft = Uniform() < 0.6 ? 10 + static_cast<int>(14.0 * Uniform()) : 2 + static_cast<int>(4.0 * Uniform());
				RiffleGap = RiffleLeft > 6 ? 0.028 + 0.02 * Uniform() : 0.06 + 0.05 * Uniform();
				RiffleGain = 0.05 + 0.12 * Uniform();
				NextClick = Time;
				NextRiffle = Time + (0.4 + 2.2 * Uniform()) / Crowd;
			}
			if (RiffleLeft > 0 && Time >= NextClick)
			{
				ClickAt = Time;
				--RiffleLeft;
				NextClick = Time + RiffleGap * (0.8 + 0.4 * Uniform());
			}
			const double SinceClick = Time - ClickAt;
			if (SinceClick < 0.012)
			{
				Click += (Noise() - Click) * ClickK;
				S += (Noise() - Click) * std::exp(-SinceClick * 600.0) * RiffleGain;
			}
			if (Time >= NextShuffle)
			{
				ShuffleAt = Time;
				NextShuffle = Time + (14.0 + 30.0 * Uniform()) / Crowd;
			}
			const double SinceShuffle = Time - ShuffleAt;
			if (SinceShuffle < 1.3)
			{
				const double Burst = (SinceShuffle < 0.55 ? 1.0 : 0.0) + (SinceShuffle > 0.75 ? 1.0 : 0.0);
				const double Flutter = 0.5 + 0.5 * std::sin(2.0 * Pi * 38.0 * SinceShuffle);
				Shuffle += (Noise() - Shuffle) * 0.35;
				S += (Noise() - Shuffle) * Burst * Flutter * 0.02;
			}
			S += 0.0035 * (std::sin(2.0 * Pi * 523.25 * Time) + std::sin(2.0 * Pi * 659.25 * Time + 0.3) + std::sin(2.0 * Pi * 783.99 * Time + 1.2)) * (0.55 + 0.45 * std::sin(2.0 * Pi * 0.07 * Time));
			if (Time >= NextJingle)
			{
				JingleAt = Time;
				JingleRoot = Uniform() < 0.5 ? 1046.5 : 880.0;
				NextJingle = Time + 9.0 + 25.0 * Uniform();
			}
			const double SinceJingle = Time - JingleAt;
			if (SinceJingle < 1.6)
			{
				static const double Steps[8] = {1.0, 1.25, 1.5, 2.0, 1.5, 2.0, 2.5, 3.0};
				const int Note = std::min(7, static_cast<int>(SinceJingle / 0.12));
				const double Local = SinceJingle - Note * 0.12;
				S += 0.012 * std::sin(2.0 * Pi * JingleRoot * Steps[Note] * SinceJingle) * std::exp(-Local * 9.0) * (1.0 - SinceJingle / 1.6);
			}
			Out[I] = static_cast<float>(std::min(1.0, std::max(-1.0, S)));
		}
	}

private:
	double Noise()
	{
		State ^= State << 13;
		State ^= State >> 17;
		State ^= State << 5;
		return static_cast<double>(State) / 2147483648.0 - 1.0;
	}
	double Uniform() { return 0.5 + 0.5 * Noise(); }
	struct Voice
	{
		bool On = false;
		double NextPhrase = 0.0, NextSyllable = 0.0, Target = 0.0, Gain = 0.0, Center = 700.0, Pan = 1.0, Low = 0.0, High = 0.0;
	};
	static constexpr int NumVoices = 10;
	Voice Voices[NumVoices];
	uint32_t State = 1;
	double Time = 0.0, Crowd = 1.0, Brown = 0.0, Air = 0.0;
	double NextLaugh = 6.0, LaughAt = -10.0, LaughBand = 0.0;
	double NextRiffle = 0.3;
	int RiffleLeft = 0;
	double RiffleGap = 0.03, RiffleGain = 0.1, NextClick = 0.0, ClickAt = -1.0, Click = 0.0;
	double NextShuffle = 5.0, ShuffleAt = -10.0, Shuffle = 0.0;
	double NextJingle = 4.0, JingleAt = -10.0, JingleRoot = 880.0;
};

/** One mood of a Nightly, 60 s, seated at table 12: what the engine and the director would feed the room. */
std::vector<float> NightlyMood(const Mood& M, int Budget, uint32_t Seed, double Seconds)
{
	CardRoom Room(Seed);
	Room.SetVoiceBudget(Budget);
	SeatAtTwelve(Room);
	SetMood(Room, M);
	const Timbre Dealer = TimbreFor(false, 47, 12u);
	const Timbre Dealer2 = TimbreFor(true, 38, 13u);
	const Timbre Floor = PaTimbre(14u);
	const std::string Name = M.Name;
	std::vector<float> Out;
	std::vector<float> Buf(2400);
	const int Blocks = static_cast<int>(Seconds * 40.0);
	for (int B = 0; B < Blocks; ++B)
	{
		Hands(Room, B + 900, 14);
		if (Name == "Running")
		{
			// Two people at your table, then one at the next table over: the murmur under the captions (in the fallback,
			// a soft breath).
			const Timbre Boots = TimbreFor(false, 58, 21u), Tj = TimbreFor(false, 27, 22u), Maria = TimbreFor(true, 44, 23u);
			if (B == 160)
			{
				Room.Say(PlanSpeech("You take the 14 up?", Boots, 0.6f, 21u), Boots, CardRoom::Spot::Table, 12, 0.6f, true);
			}
			if (B == 230)
			{
				Room.Say(PlanSpeech("Driver took the switchbacks like he owed somebody money.", Tj, 0.6f, 22u), Tj, CardRoom::Spot::Table, 12, 0.6f, true);
			}
			if (B == 360)
			{
				Room.Say(PlanSpeech("Seat open, nine!", Dealer2, 0.9f, 1u), Dealer2, CardRoom::Spot::Table, 9, 0.9f, false);
			}
			if (B == 700)
			{
				Room.Say(PlanSpeech("Third cash in four nights. Don't say anything.", Maria, 0.6f, 23u), Maria, CardRoom::Spot::Table, 13, 0.6f, true);
			}
			if (B == 1300)
			{
				Room.Say(PlanSpeech("Players, the blinds are now five hundred and one thousand, with a one thousand big blind ante.", Floor, 0.7f, 2u), Floor, CardRoom::Spot::Pa, 0, 0.7f, false);
			}
			if (B == 1800)
			{
				Room.Crowd(Vocal::Laugh, 3, CardRoom::Spot::Table, 7, 0.8f);
			}
		}
		else if (Name == "HandForHand")
		{
			if (B == 200)
			{
				Room.Say(PlanSpeech("Players, we are on the bubble. We're now hand for hand.", Floor, 0.85f, 3u), Floor, CardRoom::Spot::Pa, 0, 0.85f, false);
			}
			if (B == 900)
			{
				Room.Say(PlanSpeech("All in and a call, table four!", Dealer, 0.95f, 4u), Dealer, CardRoom::Spot::Table, 4, 0.95f, false);
			}
			if (B == 1000)
			{
				Room.Say(PlanSpeech("We have an all-in and a call at table four.", Floor, 0.7f, 5u), Floor, CardRoom::Spot::Pa, 0, 0.7f, false);
				Room.Crowd(Vocal::Ooh, 8, CardRoom::Spot::Rail, 0, 0.7f);
			}
			if (B == 1700)
			{
				Room.Say(PlanSpeech("And that's the bubble! Lin, thank you for playing. Congratulations, everyone remaining is in the money.", Floor, 0.9f, 6u), Floor, CardRoom::Spot::Pa, 0, 0.9f, false);
			}
			if (B == 2000)
			{
				Room.Crowd(Vocal::Applause, 80, CardRoom::Spot::Rail, 0, 1.0f);
				Room.Crowd(Vocal::Cheer, 10, CardRoom::Spot::Rail, 0, 0.7f);
				SetMood(Room, BurstMood);
			}
		}
		else if (Name == "FinalTable")
		{
			if (B == 200)
			{
				Room.Say(PlanSpeech("Ladies and gentlemen, your final table. In seat one, with 412,000, Rick.", Floor, 0.85f, 7u), Floor, CardRoom::Spot::Pa, 0, 0.85f, false);
			}
			if (B == 900)
			{
				Room.Crowd(Vocal::Ooh, 12, CardRoom::Spot::Rail, 0, 1.0f);
			}
			if (B == 1000)
			{
				Room.Crowd(Vocal::Gasp, 8, CardRoom::Spot::Rail, 0, 0.9f);
			}
			if (B == 1030)
			{
				Room.Crowd(Vocal::Groan, 12, CardRoom::Spot::Rail, 0, 0.9f);
			}
			if (B == 1700)
			{
				Room.Crowd(Vocal::Applause, 12, CardRoom::Spot::Rail, 0, 0.7f);
			}
		}
		Room.RenderStereo(Buf.data(), 1200);
		Out.insert(Out.end(), Buf.begin(), Buf.end());
	}
	return Out;
}

void Listening(const std::string& Dir)
{
	std::printf("listening gate renders into %s\n", Dir.c_str());
	auto Save = [&](const std::string& Name, const std::vector<float>& S, int Channels) {
		WriteWav(Dir + "/" + Name, S, Channels);
		std::printf("  %s (%.1f s)\n", Name.c_str(), static_cast<double>(S.size()) / Channels / SampleRate);
	};
	// The three moods the gate asks for (§4.10), with 6 talkers (the budget).
	Save("room_running.wav", NightlyMood(Running, 6, 101u, 60.0), 2);
	Save("room_handforhand.wav", NightlyMood(HandForHand, 6, 102u, 60.0), 2);
	Save("room_finaltable.wav", NightlyMood(FinalTable, 6, 103u, 60.0), 2);
	// The talkers on their own terms: 6 and 12 of them, and the fallback (walla noise and vocals, no talkers).
	Save("talkers_6.wav", NightlyMood(Running, 6, 104u, 30.0), 2);
	Save("talkers_12.wav", NightlyMood(Running, 12, 104u, 30.0), 2);
	Save("fallback_running.wav", NightlyMood(Running, 0, 101u, 60.0), 2);
	// Today's bed, for the A/B (CardRoomAmbience.h at its 0.4 send, both ears).
	{
		TodayBed Bed(0xcafeu);
		Bed.SetCrowd(0.7f);
		std::vector<float> M(static_cast<size_t>(60 * SampleRate));
		for (size_t I = 0; I < M.size(); I += 1200)
		{
			Bed.Render(M.data() + I, 1200);
		}
		std::vector<float> S(M.size() * 2);
		for (size_t I = 0; I < M.size(); ++I)
		{
			S[2 * I] = S[2 * I + 1] = M[I] * 0.4f;
		}
		Save("today_bed.wav", S, 2);
	}
	// The PA on its own: the floor's lines through the chain, gaps between.
	{
		const Timbre Floor = PaTimbre(14u);
		const char* Pa[] = {
			"The Embercrest Nightly is now open for registration at the tournament desk.",
			"Players, we are three away from the money. Fifteen places are paid, and a min-cash is 210.",
			"Players, we are on the bubble. We're now hand for hand.",
			"We're heads-up for the Embercrest Sunday title. First place pays 3,400.",
		};
		std::vector<float> All;
		uint32_t Seed = 1u;
		for (const char* Line : Pa)
		{
			std::vector<float> S = RenderSpeech(PlanSpeech(Line, Floor, 0.75f, Seed), Floor, false, Seed);
			PaChain(S, Seed);
			All.insert(All.end(), S.begin(), S.end());
			All.insert(All.end(), static_cast<size_t>(0.8 * SampleRate), 0.0f);
			++Seed;
		}
		Save("pa_lines.wav", All, 1);
	}
	// Near lines as the table hears them: murmured, and clear, several voices.
	{
		std::vector<float> Murmur, Clear;
		uint32_t Seed = 1u;
		for (const char* Line : Lines)
		{
			const Timbre T = TimbreFor(Seed % 3 == 0, 24 + static_cast<int>(Seed * 7 % 50), Seed * 31u);
			const SpeechPlan P = PlanSpeech(Line, T, 0.6f, Seed);
			std::vector<float> M = RenderSpeech(P, T, true, Seed), C = RenderSpeech(P, T, false, Seed);
			Murmur.insert(Murmur.end(), M.begin(), M.end());
			Murmur.insert(Murmur.end(), static_cast<size_t>(0.6 * SampleRate), 0.0f);
			Clear.insert(Clear.end(), C.begin(), C.end());
			Clear.insert(Clear.end(), static_cast<size_t>(0.6 * SampleRate), 0.0f);
			++Seed;
		}
		Save("lines_murmur.wav", Murmur, 1);
		Save("lines_clear.wav", Clear, 1);
	}
	// The vocals: one person, then a crowd.
	{
		std::vector<float> All;
		const Timbre T = TimbreFor(false, 41, 5u);
		const struct
		{
			Vocal V;
			int N;
		} List[] = {{Vocal::Laugh, 1}, {Vocal::Laugh, 4}, {Vocal::Sigh, 1}, {Vocal::Scoff, 1}, {Vocal::Groan, 1}, {Vocal::Groan, 12},
			{Vocal::Ooh, 1}, {Vocal::Ooh, 12}, {Vocal::Gasp, 8}, {Vocal::Cheer, 1}, {Vocal::Cheer, 12}, {Vocal::Clap, 6}, {Vocal::Applause, 40}};
		uint32_t Seed = 1u;
		for (const auto& E : List)
		{
			const std::vector<float> S = RenderVocal(E.V, T, E.N, Seed++);
			All.insert(All.end(), S.begin(), S.end());
			All.insert(All.end(), static_cast<size_t>(0.7 * SampleRate), 0.0f);
		}
		Save("vocals.wav", All, 1);
	}
	FILE* F = std::fopen((Dir + "/README.txt").c_str(), "wb");
	if (F)
	{
		std::fprintf(F,
			"The listening gate (next_segment_spec 4.10, 9.1 step 4): the synthesized card room, standalone renders.\n"
			"48 kHz 16-bit. Listen on headphones and on speakers.\n\n"
			"room_running.wav       60 s, a Nightly running: seated at table 12, 14 of 20 tables dealing, 6 talkers. Two people\n"
			"                       at your table murmur at 4 s and 5.75 s ('You take the 14 up?' and the answer; the room\n"
			"                       ducks for them), a dealer's 'Seat open, nine!' at 9 s, a line from table 13 at 17.5 s,\n"
			"                       the PA's level change at 32 s, a laugh at 45 s.\n"
			"room_handforhand.wav   60 s, the bubble: the PA's hand-for-hand call, the hush (talk -10 dB, chips riffling),\n"
			"                       'All in and a call, table four!' shouted at 22 s, the floor relaying it, the rail's ooh,\n"
			"                       the bubble's announcement at 42 s, then applause and the burst.\n"
			"room_finaltable.wav    60 s, the final table: the room dark and thin, the introduction on the PA, the rail's ooh,\n"
			"                       a sharp 'oh!' and a groan at 25 s, polite applause at 42 s.\n"
			"talkers_6.wav          30 s, Running with 6 talkers (the budget).\n"
			"talkers_12.wav         30 s, the same with 12.\n"
			"fallback_running.wav   60 s, the fallback if the talkers read as a machine: speech-shaped walla, the non-word\n"
			"                       vocals, chips and calls; no formant talkers, and the same table lines as room_running\n"
			"                       only as a soft breath under their captions.\n"
			"today_bed.wav          60 s of today's mono bed (CardRoomAmbience.h at its 0.4 send), for the A/B.\n"
			"pa_lines.wav           four floor announcements through the PA chain on their own.\n"
			"lines_murmur.wav       ten table lines as the room's murmur (the captions carry the words).\n"
			"lines_clear.wav        the same lines unrounded (dealer calls are rendered this way).\n"
			"vocals.wav             laugh, laugh x4, sigh, scoff, groan, groan x12, ooh, ooh x12, gasp x8, cheer, cheer x12,\n"
			"                       claps x6, applause x40.\n");
		std::fclose(F);
	}
}

/**
 * The sites' cues in their moments (SPEC §2.3, §2.4 FUSE's reveal, §3.3-§3.4), for the lead's ears before PIE: each cue on
 * its own, and a reel per site with the cues timed as the pages fire them (UiSound's 50 ms per id included).
 */
struct CueReelTrack
{
	std::vector<float> S;
	double LastAt[64] = {};
	/** Mixes Id in at At seconds, unless the same id played under 50 ms ago (Session::UiSound's limit). */
	void Put(ss::SoundId Id, float Vol, double At, uint32_t Seed)
	{
		const size_t K = static_cast<size_t>(Id);
		if (LastAt[K] > 0.0 && At - LastAt[K] < 0.05)
		{
			return;
		}
		LastAt[K] = At;
		const std::vector<float> X = Render(Id, Vol, Seed);
		const size_t From = static_cast<size_t>(At * SampleRate);
		if (S.size() < From + X.size())
		{
			S.resize(From + X.size(), 0.0f);
		}
		for (size_t I = 0; I < X.size(); ++I)
		{
			S[From + I] += X[I];
		}
	}
};

void CueReel(const std::string& Dir)
{
	std::printf("the sites' cues into %s\n", Dir.c_str());
	uint32_t Seed = 100u;
	auto Save = [&](const std::string& Name, const std::vector<float>& S) {
		WriteWav(Dir + "/" + Name, S, 1);
		std::printf("  %s (%.2f s)\n", Name.c_str(), static_cast<double>(S.size()) / SampleRate);
	};
	for (const SiteCue& C : SiteCues)
	{
		Save(std::string("cue_") + C.Name + ".wav", Render(C.Id, C.Vol, Seed++));
	}
	{
		CueReelTrack Ig;
		double T = 0.2;
		// The rewards ticker: Sparks counting up over 0.9 s (an odometer step every 55 ms), then three Dare checks 0.25 s apart.
		for (int K = 0; K < 17; ++K)
		{
			Ig.Put(ss::SoundId::IgSpark, 0.3f, T + 0.055 * K, Seed++);
		}
		T += 1.6;
		for (int K = 0; K < 3; ++K)
		{
			Ig.Put(ss::SoundId::IgSpark, 0.35f, T + 0.25 * K, Seed++);
		}
		T += 1.5;
		// The wheel: 3.2 s of deceleration, theta(t) = theta1 - (theta1 - theta0)(1 - t)^3 over 64 wedges, a tick a wedge.
		int Wedge = 0;
		for (double U = 0.0; U <= 3.2; U += 1.0 / 240.0)
		{
			const int Now = static_cast<int>((1.0 - std::pow(1.0 - U / 3.2, 3.0)) * 64.0);
			if (Now > Wedge)
			{
				Wedge = Now;
				Ig.Put(ss::SoundId::IgTick, 0.3f, T + U, Seed++);
			}
		}
		T += 4.0;
		// A cracker.
		Ig.Put(ss::SoundId::IgPop, 0.45f, T, Seed++);
		T += 1.5;
		// FUSE at x2: the fuse 0.15-0.6 s (one segment), the reel 0.6-1.2 s (an IgTick a symbol), the boom at 1.2 s.
		Ig.Put(ss::SoundId::IgFuse, 0.4f, T + 0.15, Seed++);
		for (int K = 0; K < 6; ++K)
		{
			Ig.Put(ss::SoundId::IgTick, 0.3f, T + 0.6 + 0.1 * K, Seed++);
		}
		Ig.Put(ss::SoundId::IgBoom, 0.45f, T + 1.2, Seed++);
		T += 3.0;
		// FUSE at x25: the fuse 0.3-1.9 s (a segment every 0.5 s), the reel 1.9-4.3 s slowing onto the result, the long boom.
		for (double F = 0.3; F < 1.9; F += 0.5)
		{
			Ig.Put(ss::SoundId::IgFuse, 0.4f, T + F, Seed++);
		}
		for (double U = 0.0, Gap = 0.07; U < 2.4; U += Gap, Gap *= 1.12)
		{
			Ig.Put(ss::SoundId::IgTick, 0.3f, T + 1.9 + U, Seed++);
		}
		Ig.Put(ss::SoundId::IgBoom, 0.6f, T + 4.3, Seed++);
		Save("reel_ignite.wav", Ig.S);
	}
	{
		CueReelTrack Rit;
		double T = 0.2;
		// Selecting rows on the Board, a new feed line.
		for (int K = 0; K < 3; ++K)
		{
			Rit.Put(ss::SoundId::RitBlip, 0.25f, T + 0.35 * K, Seed++);
		}
		T += 1.4;
		// Opening the Board: the settle wave, one flap a row over about 0.4 s (UiSound keeps them 50 ms apart).
		for (int K = 0; K < 10; ++K)
		{
			Rit.Put(ss::SoundId::RitFlap, 0.35f, T + 0.045 * K, Seed++);
		}
		T += 1.4;
		// The desk bell.
		Rit.Put(ss::SoundId::RitBell, 0.45f, T, Seed++);
		T += 2.0;
		// The Settlement prints for 0.6 s, then the stamp.
		Rit.Put(ss::SoundId::RitStamp, 0.5f, T + 0.6, Seed++);
		T += 1.6;
		// League Night: the rows re-sort, a tick per place gained (four), then the promotion; later a relegation.
		for (int K = 0; K < 4; ++K)
		{
			Rit.Put(ss::SoundId::RitRank, 0.3f, T + 0.12 * K, Seed++);
		}
		Rit.Put(ss::SoundId::RitRank, 0.55f, T + 0.8, Seed++);
		T += 2.0;
		Rit.Put(ss::SoundId::RitRank, 0.45f, T, Seed++);
		T += 1.0;
		// The Second Board's ghost runout slides in (a flap), and a TAKEN stamp on a Team pro's card.
		Rit.Put(ss::SoundId::RitFlap, 0.35f, T, Seed++);
		Rit.Put(ss::SoundId::RitStamp, 0.45f, T + 1.0, Seed++);
		Save("reel_runittwice.wav", Rit.S);
	}
	FILE* F = std::fopen((Dir + "/README.txt").c_str(), "wb");
	if (F)
	{
		std::fprintf(F,
			"The revamped sites' ten cues (Update 3 SPEC 1.11, 2.3, 3.3), standalone renders. 48 kHz 16-bit mono.\n\n"
			"cue_<name>.wav         each cue on its own at the volume a page asks for it: igspark 0.35, igpop 0.45, igtick 0.35,\n"
			"                       igfuse 0.4 (one 0.52 s loop segment), igboom 0.45, igboom-long 0.6 (x25 and up),\n"
			"                       ritblip 0.3, ritflap 0.35, ritbell 0.45, ritstamp 0.5, ritrank 0.35 (a tick),\n"
			"                       ritrank-fall 0.45, ritrank-rise 0.6.\n"
			"reel_ignite.wav        Sparks counting up, three Dare checks, the wheel's 3.2 s spin (a tick a wedge), a cracker,\n"
			"                       FUSE at x2 (the fuse, the reel, the boom: 1.5 s) and at x25 (the fuse held 1.6 s by\n"
			"                       segments every 0.5 s, the reel slowing, the long boom).\n"
			"reel_runittwice.wav    three row selections, the Board's settle wave, the desk bell, the Settlement's stamp,\n"
			"                       League Night (four place ticks, the promotion), a relegation, the Second Board's flap and\n"
			"                       a TAKEN stamp.\n");
		std::fclose(F);
	}
}
} // namespace audio_test

int main(int Argc, char** Argv)
{
	if (Argc > 2 && std::string(Argv[1]) == "--listening")
	{
		audio_test::Listening(Argv[2]);
		return 0;
	}
	if (Argc > 2 && std::string(Argv[1]) == "--cues")
	{
		audio_test::CueReel(Argv[2]);
		return 0;
	}
	if (Argc > 1)
	{
		audio_test::OutDir = Argv[1];
	}
	audio_test::OneShots();
	audio_test::SiteCueChecks();
	audio_test::Timbres();
	audio_test::Speech();
	audio_test::Vocals();
	audio_test::PaChecks();
	audio_test::Moods();
	audio_test::Space();
	audio_test::Ducks();
	audio_test::Controls();
	audio_test::Robust();
	audio_test::Fallback();
	audio_test::Threads();
	audio_test::Speed();
	std::printf("audio tests: %d checks, %d failed\n", audio_test::Checks, audio_test::Failures);
	if (audio_test::Failures == 0)
	{
		std::printf("audio tests: all passed\n");
	}
	return audio_test::Failures == 0 ? 0 : 1;
}
