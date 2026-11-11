#pragma once

#include "ShortStack/Common.h"

#include <array>
#include <climits>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
class Rng;
namespace net
{
struct EventResult;
}
namespace world
{
class World;
struct Npc;
struct Pending;

/**
 * Update 4, the living world (SPEC v1.7 §3.2-§3.6, §3.18; Phase A's declarations, §6 PA-19): the Scene Book, where every arrival
 * has a cause, the Curve Keeper that paces the scene toward kSceneCap, the wake of the hero's public results and the pinned
 * Embercrest regulars. It includes nothing of World.h (World.h includes it, appends SceneBook Scene to World and Moment, LeftWhy,
 * Noticed, CrowdSerial to Npc). Bodies in G/WorldScene.cpp, G/WorldArrive.cpp and G/WorldSceneText.cpp (W-arrive's at G2); the
 * private reads and writes go through scene::Access, World's friend (§6.2). Frozen after Phase A (append-only, by patch).
 *
 * Phase A: the book never opens (AnchorDay stays -1, so nothing is posted, nobody arrives and nothing is written), every hook is a
 * no-op, and scene::Week runs today's Sim::Newcomers in its place (§6 PA-23).
 */
/** Saved as int (append-only). Scheduled kinds (scaled by the keeper): Everyday, Festival, Season. The rest are surprises. */
enum class MomentKind : int
{
	Everyday,
	Festival,
	Season,
	Stream,
	Ripple,
	Summer,
	Launch,
	Closure,
	You,
	YourWave,
	Room,
	Count,
};

/** One thing that brings people into the game (DESIGN §5.2). Named SceneMoment, not Moment: world::Profile already has a nested
 *  Moment (H/World.h), and a namespace-level world::Moment would read ambiguously beside it. */
struct SceneMoment
{
	int Id = 0;                         // append-only from 1 (0: none)
	MomentKind Kind = MomentKind::Everyday;
	int Day = 0;                        // posted
	int Start = 0, End = 0;             // the wake's window, world days inclusive (End = INT_MAX for Everyday)
	float Pull = 0.0f;                  // arrivals at scale 1 (scheduled) or in all (surprises)
	double Carry = 0.0;                 // owed, full precision
	int Emitted = 0;
	int Npc = -2;                       // the winner, the streamer; -1 the hero; -2 nobody
	std::string What;                   // "RCOP 2026", "the RCOP Main", "Tiltproof" (escaped like every world string)
	std::string Key;                    // the posting key (series id, event id, "ny2027", ...): posts once
	int Site = 0;                       // net::SiteId the arrivals open their account on (0 RiverLine)
	int Region = -1;                    // world::Region 70% of arrivals come from (-1: the site's own country mix)
	int Flags = 0;                      // MomentFlag bits
	float Half = 0.0f;                  // surprises: half-life in days (0: even over the window)
	int16_t Mix = 0;                    // the cause mix (MixId)
};
enum MomentFlag : int
{
	MomQualifier = 1,
	MomFirstYear = 2,
	MomHero = 4,
	MomFlagship = 8,
	MomHighRoller = 16,
	MomLive = 32,
};
enum class MixId : int16_t
{
	Everyday,
	NewYear,
	SummerSchool,
	Holidays,
	OnlineFestival,
	HighRollerFestival,
	LiveFestival,
	Ripple,
	Summer,
	Stream,
	Launch,
	ForYou,
	Room,
	Closure,
	Count,
};

struct SceneBook
{
	std::vector<SceneMoment> Moments;   // all, kept forever (~56 a year [Mod])
	int NextId = 1;
	// The keeper's state (§3.3), the anchor and the staging cap.
	int AnchorDay = -1;                 // -1: the book hasn't opened (nothing is written; an old save untouched)
	double A0 = 0.0;                    // the scene at the anchor
	int CapNow = 3000;                  // §3.3 R6 (scene::kCapNowDefault)
	double Scale = 1.0;                 // this week's scale on scheduled moments
	double Shift = 0.0;                 // days added to t by Summers (+75 each)
	int LastSummer = -100000;           // the day the last Summer began
	int WeekArrivals = 0;               // arrivals since this week's Monday (the 210 cap)
	int NextClosure = 0;                // index into the ten closed sites (G/WorldText.cpp's ArrivalText)
	uint32_t ReachDone = 0;             // bits: the hero's Kast thresholds already noted (§3.5)
	int TitleWaveUntil = -1;            // the 15% cap's window (§3.5 R3)
	std::array<int, 32> HeroTrail{};    // hero arrivals by day, a 32-day ring (the 5% cap's trailing 28 days)
	std::array<int, 5> WeekRing{};      // all arrivals by week, the last 4 weeks and this one (the cap's denominator)
	// Per-tier softness and the drift guard (§3.12): set at the anchor.
	std::array<double, 5> TierRef{};    // [1..4]: the mean Overall of playing people at each online tier at the anchor
	double AvgAtAnchor = 0.0;           // the world mean at the anchor (keeps RefreshField continuous)
	double SteadyScale = 1.0;           // the drift guard's scale on Steady closing
};
/** Why someone left (private, N3; Npc::LeftWhy, written only when non-zero). */
enum class LeftWhy : int
{
	None,
	Busto,
	LostInterest,
	Burnout,
	Life,
	Age,
	Count,
};
enum class LiftMode : int
{
	ReAnchor,
	EnterAtCount,
};

namespace scene
{
// The knobs (§1.4; each defined here and nowhere else).
constexpr int kSceneHorizonDays = 730;   // [ASK] 2 (answered: about two years)
constexpr int kSceneCap = 10000;
constexpr int kWeekCap = 210, kDayCap = 80;
constexpr int kCapNowDefault = 3000;     // G2's staging cap; 10,000 from G4
constexpr LiftMode kLiftMode = LiftMode::ReAnchor;
constexpr double kWakeShare = 0.05;      // [ASK] 6 (answered: capped at 5%)
constexpr double kWakeShareTitle = 3.0 * kWakeShare;
constexpr double kEverydayPull = 2.5, kLateSummer = 0.85, kStreamPull = 0.45;
constexpr Chips kRippleFirstCents = 25000000; // $250,000
constexpr int kSummerCooldown = 730, kSummerShift = 75;

/** World's friend for the scene modules (§6.2: defined in G/WorldScene.cpp). */
struct Access;

// ---- the world's hooks (Phase A's calls, §6 PA-23, PA-24; no-ops in Phase A unless said).
/** Sim::NewDay, after the calendar turns: posting (the anchor's moments, festivals 7 days out, seasons, closures). */
SHORTSTACKCORE_API void Day(World& W, int Today);
/** World::AdvanceTo's hourly block: the 09:00 step (the carries and the arrivals). */
SHORTSTACKCORE_API void Hour(World& W, double Now);
/** Sim::NewWeek, in Newcomers' place: the keeper's weekly step (the scale; R draws nothing from G2). Phase A: today's
 *  Sim::Newcomers(W, Day, R), so the week's draws are exactly today's. */
SHORTSTACKCORE_API void Week(World& W, int Today, Rng& R);
/** Sim::NewMonth, after its per-person loop: the quick fade (§3.6 R3). */
SHORTSTACKCORE_API void Month(World& W, int Today);
/** Sim::Resolve, after its winner block: ripples and the rare Summer (§3.4). */
SHORTSTACKCORE_API void OnResolved(World& W, const Pending& P, const net::EventResult& Res);
/** World::HeroFinished: the hero's public results that pull a wake (§3.5 R1). */
SHORTSTACKCORE_API void OnHeroResult(World& W, const std::string& EventId, int Place, Chips Prize);
/** World::EmbercrestDone: a room win's Room moment (§3.5 R1). */
SHORTSTACKCORE_API void OnRoom(World& W, const std::string& EventId, int HeroPlace, int Field);
/** Sim::Lifecycle's exits (going broke, retiring): why they left (§3.6 R2). */
SHORTSTACKCORE_API void Left(World& W, Npc& N, int Today, bool Broke);
/** Sim::Lifecycle's Active case: the honeymoon's extra weekly leave chance (§3.6 R1). */
SHORTSTACKCORE_API void Honeymoon(World& W, Npc& N, int Today);
/** Sim::Broadcast's follower milestones: a streamer's Stream moment (§3.2). */
SHORTSTACKCORE_API void OnStreamMilestone(World& W, Npc& N, int Today, int Followers);

// ---- the keeper (§3.3) and the card.
/** The scene (X1): people Active or on a Break and not Faded (from G4 the crowd's too). */
SHORTSTACKCORE_API int SceneCount(const World& W);
/** The keeper's curve, Planned(t) with t = Day - AnchorDay + Shift (0 while the book is shut). */
SHORTSTACKCORE_API double Planned(const SceneBook& B, int Today);
/** Audience = sqrt(Scene / 1657) x max(0.15, 1 - Scene / 10000) (§3.2). */
SHORTSTACKCORE_API double Audience(int Scene);
/** The card's JOURNEY first line for someone the book brought (Npc::Moment > 0; §3.2.2). */
SHORTSTACKCORE_API std::string ArrivalLine(const World& W, const Npc& N);
/** Every string of §3.2-§3.6 (key, text), for copy_lint_world (§3.20). */
SHORTSTACKCORE_API std::vector<std::pair<std::string, std::string>> Inventory();
/** The pinned Embercrest regulars (§3.18 R1): World::Regulars from today's rule, once the book opens (G2 calls it; Phase A lands the
 *  function disabled: nothing calls it, so EmbercrestRegulars keeps today's per-day sort). */
SHORTSTACKCORE_API void PinRegulars(World& W);
} // namespace scene
} // namespace world
} // namespace ss
