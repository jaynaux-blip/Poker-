#include "ShortStack/Game/Weather.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <unordered_map>
#include <vector>

// The weather over Northside (the realism update's spec, §3.3; Weather.h has the model). people-core.
//
// Picks recorded here (the spec's ranges): the regimes' step odds below were tuned (a random search over the tables,
// measured on 1,200 seeds) so 90 days from Night One land within 5 points of October's shares for 99% of seeds (mean
// error 1.05 points, the worst 5.9), with spells of about 4.8 h clear, 4.1 h overcast, 2.6 h drizzle, 3.7 h rain and
// 1.5 h storm; and so the look-back's two runs meet fast: within 48 slots for 99.5% of cold reads, within 96 for all but
// about 3 in a million (23 of 10.3 million; those reach back 192, the spec's fallback, never further). The first
// tables, which mean-reverted less, missed 96 about 60 times in a million. The days of each ten are ranked by value
// noise with a 2-day lattice: 3 dry, 4 mixed, 3 wet.

namespace ss
{
namespace weather
{
namespace weather_detail
{
constexpr int States = 5;
constexpr int64_t SlotsPerDay = 48;
/** The window At reads behind the slot it's asked for: 24 h of wetness (48 slots), the slot before it for the easing
 *  into it, and 4 more for the storm cap's count. */
constexpr int64_t Reach = 48 + 1 + 4;
constexpr int RegimeBlock = 10;
constexpr int DryDays = 3, MixedDays = 4; // of each ten; the rest (3) wet
/**
 * Night One: from the career's first minute (2:07 AM on Tuesday, October 6) for 12 world hours, it rains. It has been
 * raining since 10 PM, before anyone was watching, so the opening's street is soaked and its puddles full (the window's
 * wetness integrates the rain behind it).
 */
constexpr double CareerStarts = net::MinutesPerDay * net::NightOneDay + 2.0 * 60.0 + 7.0;
constexpr double NightOneFrom = net::MinutesPerDay * net::NightOneDay - 2.0 * 60.0;
constexpr double NightOneTo = CareerStarts + 12.0 * 60.0;

enum Regime : int
{
	Dry,
	Mixed,
	Wet,
};

/**
 * p_up and p_down by regime and state. Each state stays put with p >= 0.8 (p_up + p_down <= 0.2), so the step is
 * monotone. Dry days break up to clear (cloud now and then, never rain), mixed days cloud over with spells of drizzle
 * and let a wet day's rain ease off, wet days build to rain and storms. Within a regime p_down (near enough) rises and p_up
 * falls with the state (every state reverts toward the regime's own sky), which is what lets the look-back's two runs meet:
 * the gap between them closes on every u that moves the higher run down but not the lower one, or the lower run up but
 * not the higher one.
 */
constexpr double Up[3][States] = {
	{0.08, 0.0, 0.0, 0.0, 0.0},
	{0.195, 0.055, 0.0, 0.0, 0.0},
	{0.2, 0.2, 0.175, 0.115, 0.0},
};
constexpr double Down[3][States] = {
	{0.0, 0.2, 0.2, 0.2, 0.2},
	{0.0, 0.035, 0.2, 0.195, 0.2},
	{0.0, 0.0, 0.005, 0.055, 0.19},
};

// The outputs by the state shown.
constexpr double RainOf[States] = {0.0, 0.0, 0.3, 0.75, 1.0};
constexpr double CloudOf[States] = {0.2, 0.85, 0.9, 0.95, 1.0};
constexpr double WindOf[States] = {0.14, 0.22, 0.3, 0.42, 0.55};
constexpr double FogOf[States] = {0.12, 0.2, 0.32, 0.4, 0.5};

uint64_t Mix(uint64_t Z)
{
	Z += 0x9e3779b97f4a7c15ull;
	Z = (Z ^ (Z >> 30)) * 0xbf58476d1ce4e5b9ull;
	Z = (Z ^ (Z >> 27)) * 0x94d049bb133111ebull;
	return Z ^ (Z >> 31);
}

/** A uniform in [0, 1) from the seed, an index and a salt. */
double Unit(uint64_t Seed, int64_t K, uint64_t Salt)
{
	const uint64_t H = Mix(Seed ^ Mix(static_cast<uint64_t>(K) ^ (Salt * 0x632be59bd9b4e019ull)));
	return static_cast<double>(H >> 11) * (1.0 / 9007199254740992.0);
}

/** Value noise: seeded values on a lattice L apart, eased between. */
double Noise(uint64_t Seed, double X, double L, uint64_t Salt)
{
	const double Q = X / L;
	const double I = std::floor(Q);
	const double F = Q - I;
	const int64_t K = static_cast<int64_t>(I);
	const double A = Unit(Seed, K, Salt);
	const double B = Unit(Seed, K + 1, Salt);
	return A + (B - A) * (F * F * (3.0 - 2.0 * F));
}

double Smooth(double X)
{
	const double T = std::min(1.0, std::max(0.0, X));
	return T * T * (3.0 - 2.0 * T);
}

int64_t FloorDiv(int64_t A, int64_t B)
{
	const int64_t Q = A / B;
	return (A % B != 0 && ((A < 0) != (B < 0))) ? Q - 1 : Q;
}

/** One seed's settled slots and regimes. */
struct SeedCache
{
	uint64_t Seed = 0;
	uint64_t Used = 0;
	std::unordered_map<int64_t, uint8_t> Chain;                        // slot -> the chain's state (the truth: any history gives it)
	std::unordered_map<int64_t, std::array<uint8_t, RegimeBlock>> Days; // block of ten days -> each day's regime
};

struct Cache
{
	std::vector<SeedCache> Seeds;
	uint64_t Clock = 0;
	Cost Last;
	// The last read whole (a frame's callers ask with the same clock: the stage's MPC_NS, the window, the crowd).
	bool Held = false;
	uint64_t HeldSeed = 0;
	double HeldWorld = 0.0;
	double HeldReal = 0.0;
	Now HeldNow;
};

Cache& Mine()
{
	thread_local Cache C;
	return C;
}

SeedCache& For(uint64_t Seed)
{
	Cache& C = Mine();
	++C.Clock;
	for (SeedCache& S : C.Seeds)
	{
		if (S.Seed == Seed)
		{
			S.Used = C.Clock;
			return S;
		}
	}
	// A handful of careers at once at most (a test's seeds): the least recently used goes.
	if (C.Seeds.size() < 4)
	{
		C.Seeds.emplace_back();
	}
	else
	{
		auto Oldest = std::min_element(C.Seeds.begin(), C.Seeds.end(), [](const SeedCache& A, const SeedCache& B) { return A.Used < B.Used; });
		*Oldest = SeedCache();
		std::swap(*Oldest, C.Seeds.back());
	}
	SeedCache& S = C.Seeds.back();
	S.Seed = Seed;
	S.Used = C.Clock;
	return S;
}

Regime RegimeOn(SeedCache& C, int64_t Day)
{
	const int64_t Block = FloorDiv(Day, RegimeBlock);
	auto It = C.Days.find(Block);
	if (It == C.Days.end())
	{
		// The block's ten days ranked by value noise: the lowest dry, the highest wet. Neighboring days share the noise's
		// lattice, so the wet days of a block tend to come together (a front through), and every ten days get their share.
		std::array<std::pair<double, int>, RegimeBlock> Rank;
		for (int I = 0; I < RegimeBlock; ++I)
		{
			const int64_t D = Block * RegimeBlock + I;
			Rank[static_cast<size_t>(I)] = {Noise(C.Seed, static_cast<double>(D), 2.0, 7), I};
		}
		std::sort(Rank.begin(), Rank.end());
		std::array<uint8_t, RegimeBlock> Out{};
		for (int R = 0; R < RegimeBlock; ++R)
		{
			Out[static_cast<size_t>(Rank[static_cast<size_t>(R)].second)] = static_cast<uint8_t>(R < DryDays ? Dry : R < DryDays + MixedDays ? Mixed : Wet);
		}
		It = C.Days.emplace(Block, Out).first;
	}
	return static_cast<Regime>(It->second[static_cast<size_t>(Day - Block * RegimeBlock)]);
}

bool NightOne(int64_t Slot)
{
	const double From = static_cast<double>(Slot) * SlotMinutes;
	return From < NightOneTo && From + SlotMinutes > NightOneFrom;
}

/** Slot K's step from State (the state of slot K - 1). */
int Step(SeedCache& C, int64_t K, int State)
{
	if (NightOne(K))
	{
		return static_cast<int>(Sky::Rain);
	}
	const Regime R = RegimeOn(C, FloorDiv(K, SlotsPerDay));
	const double U = Unit(C.Seed, K, 1);
	if (U < Down[R][State])
	{
		return State - 1;
	}
	if (U > 1.0 - Up[R][State])
	{
		return State + 1;
	}
	return State;
}

/**
 * Slot M's state by the bounded look-back: the chain run from every state 48 slots back (Clear and Storm bound the rest:
 * the step is monotone). Met by M, any history gives that state. Not met, the window reaches back to 96, 192 and 384
 * slots, composing the runs (each extension costs only its new slots); past that, the Clear run's value.
 */
int LookBack(SeedCache& C, int64_t M, int& Steps)
{
	std::array<int, States> Map{0, 1, 2, 3, 4}; // a state at From -> its state at M
	int64_t From = M;
	int64_t Span = 48;
	for (;;)
	{
		std::array<int, States> Run{0, 1, 2, 3, 4};
		for (int64_t K = From - Span + 1; K <= From; ++K)
		{
			for (int& S : Run)
			{
				S = Step(C, K, S);
			}
		}
		Steps += static_cast<int>(Span);
		for (int& S : Run)
		{
			S = Map[static_cast<size_t>(S)];
		}
		Map = Run;
		From -= Span;
		const int64_t Back = M - From;
		if (Map[0] == Map[States - 1] || Back >= 384)
		{
			return Map[0];
		}
		Span = Back; // 48, then 96, 192, 384 in all
	}
}

/** Makes sure slots N - Reach .. N are settled: from the nearest settled slot by single steps, or by a look-back. */
void Settle(SeedCache& C, int64_t N, Cost& K)
{
	if (C.Chain.size() > 60000)
	{
		C.Chain.clear(); // years of slots: start over (the next read looks back once)
	}
	int64_t First = N - Reach;
	while (First <= N && C.Chain.count(First) > 0)
	{
		++First;
	}
	if (First > N)
	{
		return; // all settled
	}
	int State = 0;
	const auto Before = C.Chain.find(First - 1);
	if (Before != C.Chain.end())
	{
		State = Before->second;
	}
	else
	{
		K.Miss = true;
		State = LookBack(C, First, K.LookBack);
		C.Chain[First] = static_cast<uint8_t>(State);
		++First;
	}
	for (int64_t S = First; S <= N; ++S)
	{
		const auto Had = C.Chain.find(S);
		if (Had != C.Chain.end())
		{
			State = Had->second;
			continue;
		}
		State = Step(C, S, State);
		C.Chain[S] = static_cast<uint8_t>(State);
		++K.Roll;
	}
}

/** The state slot S shows: the chain's, a storm past its fourth slot reading Rain. Slots S - 4 .. S must be settled. */
int Shown(const SeedCache& C, int64_t S)
{
	const int State = C.Chain.at(S);
	if (State != static_cast<int>(Sky::Storm))
	{
		return State;
	}
	for (int64_t Back = 1; Back <= 4; ++Back)
	{
		if (C.Chain.at(S - Back) != static_cast<int>(Sky::Storm))
		{
			return State;
		}
	}
	return static_cast<int>(Sky::Rain);
}
} // namespace weather_detail

using namespace weather_detail;

Sky SlotSky(int64_t Slot, uint64_t Seed)
{
	SeedCache& C = For(Seed);
	Cost K;
	Settle(C, Slot, K);
	Mine().Last = K;
	return static_cast<Sky>(Shown(C, Slot));
}

namespace weather_detail
{
/** At's read, uncached: the window's slots settled, then the outputs. */
Now Read(double World, uint64_t Seed, double RealSeconds)
{
	SeedCache& C = For(Seed);
	const int64_t N = static_cast<int64_t>(std::floor(World / SlotMinutes));
	Cost K;
	Settle(C, N, K);
	Mine().Last = K;

	// What the window's slots show (N - 49 .. N: the 24 h behind and the slot before it, for the easing into it).
	constexpr int64_t Span = 50;
	std::array<int, Span> Show{};
	for (int64_t I = 0; I < Span; ++I)
	{
		Show[static_cast<size_t>(I)] = Shown(C, N - (Span - 1) + I);
	}
	auto Eased = [&](double T, const double* Table) {
		const int64_t S = std::min(N, std::max(N - (Span - 2), static_cast<int64_t>(std::floor(T / SlotMinutes))));
		const double Into = T - static_cast<double>(S) * SlotMinutes;
		const double Was = Table[Show[static_cast<size_t>(S - 1 - (N - (Span - 1)))]];
		const double Is = Table[Show[static_cast<size_t>(S - (N - (Span - 1)))]];
		return Was + (Is - Was) * Smooth(Into / 10.0);
	};

	Now Out;
	Out.State = static_cast<Sky>(Show[Span - 1]);
	Out.Rain = Eased(World, RainOf);
	Out.Cloud = Eased(World, CloudOf);

	// Wetness over the fixed 24 h ending now, from 0 at its start, in 5-minute steps; puddles track it and hold 2 h.
	constexpr int Steps = 288;
	constexpr double Dt = 1440.0 / Steps;
	const double Start = World - 1440.0;
	double Wet = 0.0;
	double Pool = 0.0;
	for (int I = 0; I < Steps; ++I)
	{
		const double Mid = Start + (static_cast<double>(I) + 0.5) * Dt;
		const double R = Eased(Mid, RainOf);
		const double Rate = R >= 0.1 ? 1.2 * R : -(0.08 + 0.12 * life::Daylight(Mid));
		Wet = std::min(1.0, std::max(0.0, Wet + Rate * Dt / 60.0));
		const double End = Start + static_cast<double>(I + 1) * Dt;
		if (End >= World - 120.0)
		{
			const double X = (Wet - 0.55) / 0.45;
			Pool = std::max(Pool, Smooth(X));
		}
	}
	Out.Wetness = Wet;
	Out.Puddle = Pool;

	// Wind and fog by the state, drifting; the wind mostly from the west (blowing toward -X, east), veering in a storm.
	const bool Storming = Out.State == Sky::Storm;
	Out.Wind = std::min(0.6, std::max(0.1, Eased(World, WindOf) + 0.12 * (Noise(Seed, World, 120.0, 31) - 0.5)));
	Out.WindDir = 3.14159265358979323846 + 1.2 * (Noise(Seed, World, 360.0, 37) - 0.5) + (Storming ? 0.35 : 0.0);
	const double Hour = std::fmod(std::fmod(World, 1440.0) + 1440.0, 1440.0) / 60.0;
	const double Dawn = std::exp(-((Hour - 6.5) / 1.6) * ((Hour - 6.5) / 1.6)); // the river's mist before the sun burns it off
	Out.Fog = std::min(0.6, std::max(0.1, Eased(World, FogOf) + 0.14 * Dawn * (1.0 - 0.5 * Out.Rain) + 0.06 * (Noise(Seed, World, 180.0, 41) - 0.5)));

	// Lightning, in a storm only: a flash every 20-90 real seconds (one in each 55 s, jittered +-17.5 s), the strike and a
	// second flicker, the thunder 1-6 s behind.
	if (Storming)
	{
		constexpr double Period = 55.0;
		const int64_t K0 = static_cast<int64_t>(std::floor(RealSeconds / Period));
		double Last = -1.0e18;
		int64_t Which = K0;
		for (int64_t Kf = K0 - 1; Kf <= K0 + 1; ++Kf)
		{
			const double At = Period * static_cast<double>(Kf) + 35.0 * (Unit(Seed, Kf, 23) - 0.5);
			if (At <= RealSeconds && At > Last)
			{
				Last = At;
				Which = Kf;
			}
		}
		const double Tau = RealSeconds - Last;
		const double Flicker = (Tau - 0.19) / 0.045;
		Out.Flash = Tau < 1.0 ? std::min(1.0, std::exp(-Tau / 0.07) + 0.65 * std::exp(-Flicker * Flicker)) : 0.0;
		const double Delay = 1.0 + 5.0 * Unit(Seed, Which, 29);
		Out.ThunderIn = Tau < Delay ? Delay - Tau : -1.0;
	}
	return Out;
}
} // namespace weather_detail

Now At(double World, uint64_t Seed, double RealSeconds)
{
	// The same read again (a frame's callers ask with the same clock) is the last answer; anything else reads.
	Cache& C = Mine();
	if (C.Held && C.HeldSeed == Seed && C.HeldWorld == World && C.HeldReal == RealSeconds)
	{
		C.Last = Cost();
		return C.HeldNow;
	}
	const Now Out = Read(World, Seed, RealSeconds);
	C.Held = true;
	C.HeldSeed = Seed;
	C.HeldWorld = World;
	C.HeldReal = RealSeconds;
	C.HeldNow = Out;
	return Out;
}

const char* Word(Sky S)
{
	switch (S)
	{
	case Sky::Clear: return "Clear";
	case Sky::Overcast: return "Overcast";
	case Sky::Drizzle: return "Drizzle";
	case Sky::Rain: return "Rain";
	case Sky::Storm: return "Storm";
	}
	return "";
}

Cost LastCost()
{
	return Mine().Last;
}

void Forget()
{
	Cache& C = Mine();
	C.Seeds.clear();
	C.Last = Cost();
	C.Held = false;
}
} // namespace weather
} // namespace ss
