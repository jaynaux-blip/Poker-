#pragma once

#include "ShortStack/Common.h"

#include <array>
#include <cstdint>

namespace ss
{
namespace world
{
class World;
struct Npc;
struct Pending;

/**
 * Update 4, Growth: dynamic ability with causes (SPEC v1.7 §3.8-§3.12; Phase A's declarations, §6 PA-19). Plain data: World.h
 * includes it and appends Growth Grow to Npc, so it lives in the paged roster with the person. The functions are in
 * ss::world::grow (ss::kast::growth already exists, H/KastGrowth.h). Bodies in G/WorldGrowth.cpp (W-growth's at G2); the private
 * reads and writes go through grow::Access, World's friend (§6.2). Frozen after Phase A (append-only, by patch).
 *
 * Phase A: every hook is a no-op or returns today's value: Rate returns today's rate, VolumeScale and BuyInScale 1.0, so no draw and
 * no number moves (§6 PA-23).
 */
enum class GrowthPhase : uint8_t // not `Phase`: fest::Phase exists (rev-plan minor 5)
{
	Steady,
	Surge,
	Slump,
};
/** Saved as int, append-only. */
enum class Cause : uint8_t
{
	None,
	FirstFinalTable,
	Breakout,
	MovedUp,
	Grind,
	Coach,
	AllIn,
	Busy,
	Tilt,
	Burnout,
	Serious,
	TurnedPro,
	Count,
};
enum class ArcPace : uint8_t
{
	SlowRealistic,
	FastButBelievable,
	Fast,
};
enum GrowFlag : uint16_t
{
	GFFirstFT = 1,
	GFFirstWin = 2,
	GFWealthy = 4,
	GFSerious = 8,
	GFHoneyLeft = 16,
	GFHot7 = 32,
	GFDebut = 64,
	GFBurnHit = 128,
	GFSeriousTop = 256,
	GFLongShot = 512,
	GFLongShotWoke = 1024,
};
struct Growth
{
	GrowthPhase Ph = GrowthPhase::Steady;
	Cause Why = Cause::None;
	int Until = 0;                       // the day the surge or slump ends
	int Coach = -1;                      // a training partner's id, -3 a hired coach (the stable, a wealthy amateur's), -1 none
	int CoachUntil = 0;                  // the study bonus's last day
	int BusyUntil = 0;
	int GrindCool = 0, TiltCool = 0;     // cooldowns: the first day the cause may fire again
	uint8_t GrindRun = 0, TiltRun = 0;   // straight weeks of 12+ events; straight weeks with Tilt >= 0.35
	int8_t Band = -1;                    // the table band (0 < 0.45 <= 1 < 0.58 <= 2 < 0.72 <= 3)
	int BandSince = 0;                   // and the day it was entered
	float BirthPeak = 0.0f, BirthPot = 0.0f; // set the first time the record is written; never re-taken (build C6)
	uint16_t Flags = 0;
	int SeriousAt = 0;                   // the wealthy amateur's "gets serious" day
	float SeriousTop = 0.0f;             // and the one-in-four later ceiling
	/** True: no world\tgrow line (BirthPeak == 0 and every other field at its default). */
	SHORTSTACKCORE_API bool Default() const;
};
struct PaceRow
{
	float PaceX;
	std::array<uint8_t, 3> Dwell; // weeks in a band before stepping past 0.45 / 0.58 / 0.72
};

namespace grow
{
// The knobs (§1.4, §3.8; each defined here and nowhere else).
constexpr PaceRow kPace[3] = {{1.0f, {{4, 4, 5}}}, {3.5f, {{6, 6, 7}}}, {7.0f, {{2, 2, 3}}}}; // FastButBelievable's dwell 6/6/7: S-pace (Q-19)
constexpr ArcPace kArcPace = ArcPace::FastButBelievable;           // [ASK] 1 (answered: fast but believable)
constexpr float kSurgeA = 0.10f, kSurgeB = 0.035f;                  // x PaceX (kSurgeA 0.025 -> 0.10: S-pace, Q-19)
constexpr float kCap1 = 0.015f, kCap2 = 0.010f, kCap3 = 0.004f;     // x PaceX: below 0.58, 0.58-0.72, above
constexpr float kVeteranSurge = 6.0f, kFirstYearBoost = 2.0f;
constexpr int kLongShotsPerQuarter = 1;                             // R13
constexpr float kSeriousTopShare = 0.25f, kSeriousTopLo = 0.72f, kSeriousTopHi = 0.86f; // §3.9 R3

/** World's friend for the Growth module (§6.2: defined in G/WorldGrowth.cpp). */
struct Access;

// ---- the world's hooks (G/WorldSim.cpp; Phase A's calls, §6 PA-23).
/** Sim::NewWeek's per-person loop, before Learn: phases, causes, dwell and the weekly triggers. */
SHORTSTACKCORE_API void Week(World& W, Npc& N, int Day);
/** Sim::Learn's rate (today's TodayRate = 0.004 x Potential x (0.3 + Practice) x (0.4 + Study) x (playing ? 1 : 0.2)): the phase's
 *  closing, which may rescale the study term. Phase A: TodayRate, exactly. */
SHORTSTACKCORE_API double Rate(const World& W, const Npc& N, int Day, double TodayRate, double Study);
/** Sim::Learn, after the skills close toward the goal: the weekly caps. */
SHORTSTACKCORE_API void Clamp(World& W, Npc& N, int Day);
/** OnlineCount's volume (a surge's hot first week, a busy spell): 1.0 in Phase A. */
SHORTSTACKCORE_API double VolumeScale(const Npc& N, int Day);
/** Sim::PlanDay's buy-in window (the wealthy amateur's stakes): 1.0 in Phase A. */
SHORTSTACKCORE_API double BuyInScale(const Npc& N, int Day);
/** Sim::RefreshField's end: per-tier softness and the drift guard (§3.12). */
SHORTSTACKCORE_API void FieldTiers(World& W);
/** Sim::Apply's end: a result's causes (a first final table, a first win, the hot week). */
SHORTSTACKCORE_API void OnResult(World& W, Npc& N, const Pending& P, int Place, Chips Prize);
/** Sim::Apply's Breakout block. */
SHORTSTACKCORE_API void OnBreakout(World& W, Npc& N, int Day);
/** Sim::Stakes, after a move up. */
SHORTSTACKCORE_API void OnMovedUp(World& W, Npc& N, int Day);
/** Sim::Lifecycle, after a stake is taken. */
SHORTSTACKCORE_API void OnStaked(World& W, Npc& N, int Day);
/** Sim::NewMonth's per-person loop, right after today's TurnPro: turning pro on results (§3.10). */
SHORTSTACKCORE_API void TurnProOnResults(World& W, Npc& N, int Day);
} // namespace grow
} // namespace world
} // namespace ss
