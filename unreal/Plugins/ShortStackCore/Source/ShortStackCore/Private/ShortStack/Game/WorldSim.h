// The living world's simulation, shared by its .cpp files (World, WorldSim, WorldHero, WorldSave, WorldText).
#pragma once

#include "ShortStack/Game/World.h"
#include "ShortStack/Rng.h"

#include <cmath>
#include <string>
#include <vector>

namespace ss
{
namespace world
{
/** What an event is like (bits): what it rewards, and what playing it practises. */
enum KindBit : int
{
	KindBounty = 1,
	KindSatellite = 2,
	KindLive = 4,
	KindTurbo = 8,
	KindDeep = 16,
	KindOmaha = 32,
	KindMystery = 64,
	KindHyper = 128,
	KindReEntry = 256,
	KindUnderground = 512,
	KindSixMax = 1024,
};

namespace sim
{
/** A typical buy-in at each online tier (net::Tier), in cents. */
constexpr Chips TierAbi[5] = {0, 300, 2200, 15000, 105000};
/** The buy-ins a player at each tier plays, in cents. */
constexpr Chips TierLow[5] = {0, 25, 330, 2200, 21500};
constexpr Chips TierHigh[5] = {0, 550, 5500, 53000, 525000};
/** The comfortable buy-in (cents) where each tier starts (Sim::TierFor). */
constexpr Chips TierFloor[5] = {0, 0, 600, 4500, 32000};
/** What it takes to travel for each live level (dollars a hundred buy-ins of discipline would want). */
constexpr double LiveNeed[6] = {0.0, 1000.0, 8000.0, 40000.0, 150000.0, 1500000.0};
/** Weekly living costs of a professional at each tier (dollars), before their own lifestyle. */
constexpr double Living[5] = {120.0, 160.0, 280.0, 550.0, 1200.0};
/** The active population on day one (the network's regulars, the cast, Kast's streamers). */
constexpr int FoundingPopulation = 1618;
/** Results the world keeps in memory: every event for this long, the big ones for a year. */
constexpr double KeepResults = 8.0 * 1440.0;
constexpr double KeepMajors = 400.0 * 1440.0;
constexpr size_t MaxLog = 6000;

inline double Clamp(double V, double Lo, double Hi)
{
	return V < Lo ? Lo : V > Hi ? Hi : V;
}
inline float Clampf(double V, double Lo, double Hi)
{
	return static_cast<float>(Clamp(V, Lo, Hi));
}
inline Chips Cents(double Dollars)
{
	return static_cast<Chips>(std::llround(Dollars * 100.0));
}
inline double Dollars(Chips C)
{
	return static_cast<double>(C) / 100.0;
}
inline int TierIndex(int T)
{
	return T < 0 ? 0 : T > 4 ? 4 : T;
}
/** Calendar helpers (WorldCalendar.cpp): year * 12 + month (0-based), the day of the year, the day of the month. */
int MonthKey(int Day);
int DayOfYear(int Day);
int MonthDay(int Day);

/** The world day of a time. */
inline int DayAt(double Minutes)
{
	return static_cast<int>(std::floor(Minutes / 1440.0));
}
} // namespace sim

/** The simulation: free functions with access to the World's state (friend). */
struct Sim
{
	// ---- the people (World.cpp)
	static void Found(World& W, double Start);
	static int Add(World& W, Npc&& N);
	static std::string UniqueName(World& W, Rng& R, const std::string& Country, bool Pro);
	static void MakeSkills(Npc& N, Rng& R, double Base);
	static Npc Rookie(World& W, Rng& R, int Day);
	static Npc Discover(World& W, const std::string& Name, const std::string& Country, double Strength, int Day, uint32_t Salt);
	/** A buy-in they're comfortable with today (cents). */
	static Chips Comfort(const Npc& N);
	static double Need(const Npc& N); // buy-ins they want behind them
	static int TierFor(Chips Abi);
	/** Online events on a normal day for them (what their fatigue is measured against). */
	static double Habit(const Npc& N);

	// ---- the days (WorldSim.cpp)
	static void PlanDay(World& W, int Day);
	static Pending OnlineEvent(World& W, const net::EventInstance& E);
	static Pending LiveEventOf(World& W, const LiveEvent& E);
	static void Resolve(World& W, Pending& P);
	static void NewDay(World& W, int Day);
	static void NewWeek(World& W, int Day);
	static void NewMonth(World& W, int Day);
	static void NewYear(World& W, int Calendar, int Day);
	static double Score(const Npc& N, int Kinds);
	static void Post(World& W, double At, EventKind K, int Who, const std::string& What, Chips Amount = 0, int Value = 0, int Other = -1);
	/** A step on someone's journey (their card keeps the first few and the latest). */
	static void Mark(Npc& N, int Day, StepKind K, const std::string& What, int Place = 0, int Of = 0, Chips Amount = 0);
	/** The bracelet or ring an event's winner takes home. */
	static Award AwardOf(const Pending& P, int Day, Chips Prize);
	static void Reputation(Npc& N);
	static void SetMood(Npc& N, Momentum M, int Day);
	static Year& YearRow(Npc& N, int Day);
	static void RefreshField(World& W);
	static void Ranks(World& W);
	static bool Major(const Pending& P);

	static void Apply(World& W, Npc& N, const Pending& P, const Entry& E, int Place, Chips Prize, bool Seat);
	static void PlanTrips(World& W, int Day, Rng& R);
	static void Finances(World& W, Npc& N, int Day, Rng& R);
	static void Stakes(World& W, Npc& N, int Day, Rng& R);
	static void Form(Npc& N, int Day);
	static void Learn(World& W, Npc& N, int Day);
	static void Broadcast(World& W, Npc& N, int Day, Rng& R);
	static void Lifecycle(World& W, Npc& N, int Day, Rng& R);
	static void Connect(World& W, int Day, Rng& R);
	static void Newcomers(World& W, int Day, Rng& R);
	static void TeamRiverLine(World& W, int Day);
	static void Bind(World& W, int A, int B, TieKind K, float Strength, int Day);
	static void TurnPro(World& W, Npc& N, int Day, Rng& R);

	// ---- the player (WorldHero.cpp)
	static void Feel(Bond& B, MemoryKind K, float Emotion);
	static void Fade(World& W);
	static void Fade(World& W, Npc& N);
};
} // namespace world
} // namespace ss
