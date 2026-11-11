#pragma once

#include "ShortStack/Common.h"

#include <array>
#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace ss
{
namespace net
{
struct Placing;
}
namespace world
{
class World;
struct Npc;
struct Pending;

/**
 * Update 4, the crowd (SPEC v1.7 §5.1-§5.9; Phase A's declarations, §6 PA-19): the scene past the full records' 3,000, as compact
 * records with real names, skills, results and TPI ranks, promoted with their history when they make a name. It includes nothing of
 * World.h (World.h includes it, appends CrowdBook Crowd to World, CrowdIn to Pending and Crowd to net::Placing). Bodies in
 * G/WorldCrowd.cpp, G/WorldCrowdField.cpp, G/WorldPromote.cpp and G/WorldCrowdTurn.cpp (S-crowd's, S-promote's and S-scale's at G4);
 * the private reads and writes go through crowd::Access, World's friend (§6.2). Frozen after Phase A (append-only, by patch).
 *
 * Phase A: the crowd is empty (Count 0), so every hook is a no-op or answers today's way: FtSeat and DiscoverFromCrowd false (the
 * anonymous seat and Discover run as today), OnPlaced takes no place (§6 PA-23).
 */
/** 24 B (v1.0's order padded to 40 B, rev-code minor 10); a real event: (site, template, day) names it; Kind: 0 cash, 1 final table,
 *  2 win. */
struct CrowdFinish
{
	Chips Prize = 0;
	int32_t Place = 0, Of = 0, Template = -1;
	uint16_t Day = 0;
	uint8_t Site = 0, Kind = 0;
};
static_assert(sizeof(CrowdFinish) == 24, "CrowdFinish is 24 bytes (SPEC §5.2)");
/** v1.1: replayed as Arc beats at promotion (rev-fun-real B1). */
struct CrowdMilestone
{
	uint16_t Day = 0;
	int16_t Value = 0;
};
/** One crowd regular (world days fit uint16_t until year 2205). */
struct CrowdRec
{
	// Identity (the name is identity: unique across roster and crowd)
	char Name[18] = {};
	uint8_t Country = 0;                                // handles' country index
	uint8_t Came = 0;
	uint16_t Arrived = 0;
	int32_t CameWith = -1;
	int32_t Moment = 0;
	int16_t Born = 0;
	uint32_t Serial = 0;                                // never reused; keys every draw of this record
	// Ability (hidden): 16-bit fixed point (value / 65535), traits 8-bit
	uint16_t Overall = 0, Peak = 0, Potential = 0;
	std::array<uint8_t, 7> Traits{};
	// Growth, compact (§3.8's record in 28 bytes): phase and cause, the windows, band and dwell, birth values, flags
	uint8_t PhWhy = 0;
	uint16_t Until = 0, BandSince = 0, CoachUntil = 0, BusyUntil = 0, GrindCool = 0, SeriousAt = 0;
	int8_t Band = -1;
	uint8_t GrindRun = 0;
	uint16_t BirthPeak = 0, BirthPot = 0, GFlags = 0;
	// Habits
	uint8_t Plan = 0, OnlineShare = 0, Tier = 1, Live = 0, Sites = 1, Flags = 0; // Flags: streams
	// State (private, N3)
	uint8_t St = 0, Mood = 0, Fatigue = 0, LeftWhy = 0;
	uint16_t TierSince = 0, Left = 0, BreakUntil = 0;
	int32_t Bankroll = 0, PeakRoll = 0, Income = 0;    // whole dollars
	int32_t Followers = 0;
	// Record (public): every printed crowd number comes from here
	int32_t Events = 0, Cashes = 0, FinalTables = 0, Wins = 0;
	Chips Won = 0, Spent = 0;
	float Season = 0.0f;
	CrowdFinish Best;
	std::array<CrowdFinish, 3> Last{};
	// TPI (§3.13's record, compact)
	std::array<uint16_t, 26> Week{};
	uint32_t PlayMask = 0;
	int16_t Now = 0, Prev = 0, BestRank = 0;
	int16_t First = 0;                                  // v1.1: where the rise started ("from #2,710 to #95"), rev-fun-real B1
	uint16_t FirstDay = 0, BestDay = 0;
	std::array<int16_t, 4> Monthly{};                   // four monthly ranks, as full records (v1.0 kept two)
	uint8_t Run100 = 0, Run1000 = 0;
	std::array<CrowdMilestone, 3> Mile{};               // first cash of $1,000+ (Value = place), first final table (place), first
	                                                    // noteworthy week (rank)
	// Notice (not "Heat": Kast's word, build merge 8)
	float Notice = 0.0f;
	uint8_t Met = 0;
	uint16_t LastMet = 0;
	int32_t WorldId = -1;                               // the full record's id once promoted (a tombstone until compaction)
};
/** The crowd, in copy-on-write pages of 256 like Phase A's roster, so a save snapshot copies page pointers. */
struct CrowdBook
{
	static constexpr int PageSize = 256;
	std::vector<std::shared_ptr<std::array<CrowdRec, PageSize>>> Pages; // copy-on-write, as Phase A's roster pages
	int Count = 0;
	uint32_t NextSerial = 1;
	std::unordered_map<std::string, int> ByName;        // name -> crowd index (rebuilt at load and compaction; not saved)
	/** The record at a crowd index (0 <= I < Count). */
	SHORTSTACKCORE_API const CrowdRec& Get(int I) const;
	/** The same, to change: clones a shared page first (frozen-page asserts in tests). */
	SHORTSTACKCORE_API CrowdRec& Edit(int I);
};
/** The crowd's day (§5.3): per (site, tier), at Site x 5 + Tier, today's anonymous entries and the cumulative weights over the members
 *  who play there today (E: expected entries; WFt: x FtFactor; WCash: x CashFactor), with their crowd indexes in the same order. */
struct CrowdDayWeights
{
	int Day = -1;
	std::vector<double> Anon;
	std::vector<std::vector<double>> E, WFt, WCash;
	std::vector<std::vector<int>> Who;
};
/** A crowd member in one of the hero's events (Pending::CrowdIn, §5.4 R4; the world\tqcrowd line). */
struct CrowdEntry
{
	uint32_t Serial = 0;
	int Known = 0;  // their real place at the hero's tables (0: unknown)
	int Better = 0; // still in when the hero left, so better than this place (0: no bound)
	int Bullets = 1;
};
/** The light card (§5.9): every number from the record (X13, W5); never the bankroll (N3). */
struct CrowdProfile
{
	std::string Name, Country;
	int Since = 0;                // the day they arrived
	std::string CameLine;         // the arrival line (§3.2.2)
	std::string Stakes, Style;
	int Events = 0, Cashes = 0, FinalTables = 0, Wins = 0;
	CrowdFinish Best;
	std::array<CrowdFinish, 3> Last{};
	std::string TpiLine;          // "" when unranked
	int Met = 0;                  // times at the hero's tables
};
/** Why a crowd member was promoted (§5.7 R1). */
enum class PromoteWhy : uint8_t
{
	MidStakes,
	Title,
	Top100,
	RaceTop25,
	Verified,
	Regular,
	Followed,
	Notice,
	MetTwice,
	Followers,
	Count,
};

namespace crowd
{
// The knobs (§1.4, §5.2; each defined here and nowhere else).
constexpr int kFullRecordCap = 3000;    // the promotion budget's reference (a design constant since v1.1)
constexpr int kFullSoftCeiling = 3500;  // above it only the three always-cases promote (§5.7 R1; the lead's decision, v1.2)
constexpr int kHeroTableCrowd = 2;      // crowd seats at the hero's own table after the seating rule
constexpr float kNoticePromote = 6.0f;

/** World's friend for the crowd modules (§6.2: defined in G/WorldCrowd.cpp). */
struct Access;

// ---- the world's hooks (G/WorldSim.cpp; Phase A's calls, §6 PA-23). No-ops in Phase A.
/** Sim::NewWeek, after Standings: the crowd's week (sliced from G4, §5.6). */
SHORTSTACKCORE_API void Week(World& W, int Day);
/** Sim::Review on the 1st: the crowd's Verified rule (a promotion trigger, §5.7 R1). */
SHORTSTACKCORE_API void Review(World& W, int Day);
/** Sim::Resolve, after the tracked draws: the crowd's cash places below the final table, from the live taken set (§5.4 R2). */
SHORTSTACKCORE_API void OnPlaced(World& W, const Pending& P, int Field, const std::vector<Chips>& Pay, std::set<int>& Taken);
/** Sim::Resolve's anonymous final-table seat (§5.4 R1): true when a crowd member holds it (Pc's Name, Country and Crowd set).
 *  Phase A: false, so Anonymous names the seat as today. */
SHORTSTACKCORE_API bool FtSeat(World& W, const Pending& P, int Place, net::Placing& Pc);
/** Sim::Resolve's Discover branch (§5.4 R6): true when the winner is a crowd seat promoted by Sim::Promote (Id set). Phase A: false,
 *  so Sim::Discover runs as today. */
SHORTSTACKCORE_API bool DiscoverFromCrowd(World& W, const Pending& P, const net::Placing& Winner, int Day, int& Id);
/** A full record's compact crowd form (§5.2 R1: Sim::Arrive's people from G4). */
SHORTSTACKCORE_API CrowdRec FromNpc(const Npc& N);
} // namespace crowd
} // namespace world
} // namespace ss
