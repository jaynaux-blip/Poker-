#pragma once

#include "ShortStack/Common.h"

#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace world
{
class World;
struct Npc;
struct Pending;
struct Profile;
struct WorldEvent;
enum class Identity : int; // H/World.h

/**
 * Update 4, the stories (SPEC v1.7 §3.13-§3.17; Phase A's declarations, §6 PA-19): the TPI Rankings (one public rank for every
 * regular, the hero included), the Arc Book (story memory that outlives the log), the Story Director and its budgets, the follow
 * list, Names to Watch, Dee's tip, PASSED YOU, Rookie of the Year and sponsors on results. It includes nothing of World.h (World.h
 * includes it, appends TpiRec Tpi to Npc and the books to World: Tpi, HeroTpi, HeroHist, Arcs, Follows, Watch, Feed, Tip). Bodies
 * in G/WorldTpi.cpp, G/WorldArcs.cpp, G/WorldDirector.cpp, G/WorldStoryText.cpp, G/WorldProfile4.cpp, G/SessionStory.cpp and
 * G/SessionTable4.cpp (W-story's, W-ui's and W-table's at G2); the private reads and writes go through story::Access, World's friend
 * (§6.2). Frozen after Phase A (append-only, by patch).
 *
 * Phase A: nobody is ranked, no arc opens, the Director tells nothing; every hook is a no-op or returns today's value (Labels returns
 * today's label, MysteryInMoney false, TpiCash today's Prize > 0 || Seat), and every reader answers empty (§6 PA-23, PA-30, PA-31).
 */
struct TpiRec
{
	std::array<uint16_t, 26> Week{};      // the best-3 cash score of each of the last 26 weeks, x100 fixed point; [0] is this week
	uint32_t PlayMask = 0;                // bit k: the week k weeks ago had 5+ events (turning pro, "quieter lately")
	int16_t Now = 0, Prev = 0;            // this week's and last week's rank (0: unranked)
	int16_t Best = 0;                     // the best rank ever
	int BestDay = 0;                      // and when
	int16_t First = 0;                    // the first rank ever
	int FirstDay = 0;                     // and when ("from #2,710 to #95")
	std::array<int16_t, 4> Monthly{};     // the rank in each of the last four monthly rankings (the 1st of the month), [0] the latest
	uint8_t Run100 = 0, Run1000 = 0;      // straight weeks in the top 100 / top 1,000 (sponsors, turning pro)
	bool Ranked() const { return Now > 0; }
};
/** World::Tpi. Order (ids by rank, the hero as -1; from G4 crowd entries as -(index + 2)) is rebuilt at each re-rank, never saved. */
struct TpiBook
{
	int MonthlyDay[4] = {};
	int RankedCount = 0;
	int LastWeek = -1;
	std::vector<int> Order;
};

/** Saved as int, append-only. */
enum class ArcKind : uint8_t
{
	Climb,
	Prodigy,
	MoneyFirst,
	SecondWind,
	Slide,
	YourRival,
	OneYouBeat,
	RoomsOwn,
	Streamer,
	CameForYou,
	Count,
};
enum class ArcState : uint8_t
{
	Open,
	Peaked,
	Closed,
};
/** Hidden*: never in copy (N3). ChampionshipPair (v1.6, appended last): a second championship Main win in a calendar year (§2.6.7). */
enum class BeatKind : uint8_t
{
	FirstCash1k,
	FirstFinalTable,
	FirstTitle,
	MovedUp,
	MovedDown,
	TpiTop1000,
	TpiTop250,
	TpiTop100,
	TpiTop25,
	TpiFirst,
	Breakout,
	Sponsored,
	TeamSigning,
	RookieLead,
	MetHero,
	PassedYou,
	Quiet,
	Back,
	TurnedPro,
	FestivalFinalTable,
	FestivalTitle,
	Debut,
	RankSample,
	HiddenCoach,
	HiddenBusy,
	HiddenTilt,
	HiddenGrind,
	ChampionshipPair,
	Count,
};
struct ArcBeat
{
	int Day = 0;
	BeatKind Kind = BeatKind::Debut;
	int Value = 0;          // a rank, a place, a tier
	int Ref = -1;
	uint8_t Surfaced = 0;   // a bit per StorySurface that has told this beat (a reload never re-sends)
};
enum class StorySurface : uint8_t // bits 0-7
{
	Feed,
	Dee,
	Own,
	Mail,
	News,
	Rerun,
	Kast,
	Table,
};
struct Arc
{
	int Id = 0;
	int Npc = -1;
	ArcKind Kind = ArcKind::Climb;
	int Began = 0;
	ArcState State = ArcState::Open;
	float Score = 0.0f;        // the Director's StoryScore at the last surfacing (not Heat: Kast's word, build merge 8)
	int Other = -1;            // the hero (-1) for hero arcs, or another person
	int ClosedDay = 0;
	int16_t BestRank = 0;
	int BestDay = 0;
	std::array<ArcBeat, 12> Beats{}; // the first beat always kept; then the newest 11
	uint8_t BeatCount = 0;
};
struct ArcBook
{
	std::vector<Arc> Arcs;
	int NextId = 1;
};
constexpr int kArcOpenCap = 300, kArcClosedCap = 200;

/** The follow list lives in the world beside HeroBonds, so the Director stays pure core (§3.15). */
struct FollowRec
{
	int Npc = -1;
	int Since = 0;
	std::array<int16_t, 24> Hist{}; // the rank in each month's rankings since the follow
};
struct WatchPick
{
	int Npc = -1;
	uint8_t Why = 0; // the reason template
	int Day = 0;
};
struct FeedItem
{
	int Day = 0;
	int ArcId = 0;
	uint8_t Beat = 0;
};
struct DeeTip
{
	int Npc = -1;
	int Day = 0;
	bool Closed = false;
};
/** What the Director hands the session (World::TakeStoryNotes, §3.15 R6): keys arc:<id>:<beat>:<surface>. */
struct StoryNote
{
	std::string Key;
	uint8_t Surface = 0; // StorySurface
	int Npc = -1;
	uint8_t Kind = 0;    // BeatKind
	std::string From, Subject, Text;
};

namespace tpi
{
// The knobs (§1.4; each defined here and nowhere else).
constexpr bool kTpiHero = true;                       // [ASK] 5 (answered: yes, the hero is on the TPI)
constexpr int kNoteworthyTop = 1000;                  // "noteworthy" = the top min(1,000, ceil(0.10 x ranked)) (§3.13 R9)
constexpr double kNoteworthyShare = 0.10;
constexpr double kPassedTopShare = 0.25;              // PASSED YOU (§3.16 R1)
constexpr int kPassedTopMax = 500;
constexpr double kPassedClimb = 3.0;
} // namespace tpi

namespace story
{
constexpr double kChampBeatBoost = 2.0; // festival beats x2.0 during a championship's Live days (v1.6, the lead's L3)

/** World's friend for the story modules (§6.2: defined in G/WorldDirector.cpp). */
struct Access;

// ---- the world's hooks (G/WorldSim.cpp, G/WorldHero.cpp; Phase A's calls, §6 PA-23, PA-24). No-ops in Phase A.
/** Sim::NewDay, after Ranks: the Director's urgent pass (titles, PASSED YOU). */
SHORTSTACKCORE_API void Day(World& W, int Today);
/** Sim::NewWeek, after Standings: the TPI's weekly re-rank, the arcs and the Director's week. */
SHORTSTACKCORE_API void Week(World& W, int Today);
/** Sim::NewYear's end: Rookie of the Year (§3.16 R2). */
SHORTSTACKCORE_API void Year(World& W, int Calendar, int Today);
/** Sim::Apply's end: the TPI's week fold and the beats a result makes. */
SHORTSTACKCORE_API void OnResult(World& W, Npc& N, const Pending& P, int Place, Chips Prize, bool Seat);
/** World::HeroFinished: the hero's TPI cash and the hero's meetings. */
SHORTSTACKCORE_API void OnHeroResult(World& W, const std::string& EventId, int Place, Chips Prize);
/** Sim::Broadcast, after the follower path: sponsors on results (§3.16 R3). */
SHORTSTACKCORE_API void SponsorOnResults(World& W, Npc& N, int Today);
/** The month's label for the YoungCrusher and RisingProspect rows (Sim's Label, §3.11): Hidden is today's hidden-skill answer for those
 *  two (Identity::Count when neither holds); G2 answers from public results. Phase A: Hidden. */
SHORTSTACKCORE_API Identity Labels(const World& W, const Npc& N, int Today, Identity Hidden);
/** Bounties(): from G2 a mystery event pays the world's envelopes only in the money (§3.13 R1). Phase A: false (every place, today). */
SHORTSTACKCORE_API bool MysteryInMoney(const Pending& P);
/** Sim::Apply's cash test: from G2 a paid place or a satellite seat (§3.13 R1). Phase A: today's Prize > 0 || Seat. */
SHORTSTACKCORE_API bool TpiCash(const Pending& P, int Place, Chips Prize, bool Seat);

// ---- the readers' fall-throughs (G/WorldText.cpp, G/Talk.cpp, G/Room.cpp, G/Live.cpp; §6 PA-30, PA-31). Empty in Phase A.
/** World::Headline for TpiMark, SceneEra and a book arrival's Debut: true with FlagStory only (§3.15 R3). Phase A: false. */
SHORTSTACKCORE_API bool Headline(const World& W, const WorldEvent& E, std::string& Title, std::string& Body, std::string& Tag);
/** World::ProfileOf's end: the card's story extras (no-op in Phase A). */
SHORTSTACKCORE_API void ProfileExtras(const World& W, const Npc& N, Profile& Pr);
/** Talk's Knows set: followed people and storied Embercrest locals and regulars (§3.16 R5). Empty in Phase A. */
SHORTSTACKCORE_API std::vector<int> TownKnown(const World& W);
/** The Embercrest PA's honorific fallback after badges and patches (§3.16 R6); Rank as HonorOf's. "" in Phase A. */
SHORTSTACKCORE_API std::string PaHonor(const std::string& Name, int& Rank);
/** live::DescribeField's voice: the riser's own brag (LiveVoice::TpiRank, Rising; §3.16 R6). 0 and false in Phase A. */
SHORTSTACKCORE_API void VoiceStory(const World& W, int Npc, int& TpiRank, bool& Rising);
/** World::Greeting / SpokenGreeting: PASSED YOU within 30 days speaks first (§3.16 R1). "" in Phase A (Bond::PassedDay is 0). */
SHORTSTACKCORE_API std::string PassedGreeting(const World& W, int Npc, uint32_t Salt, bool Spoken);
/** Every string of §3.13-§3.17 (key, text), for copy_lint_world (§3.20). */
SHORTSTACKCORE_API std::vector<std::pair<std::string, std::string>> Inventory();
} // namespace story
} // namespace world
} // namespace ss
