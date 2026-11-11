// world_u4 (update 4 SPEC v1.7 §3.24, §5.15, §8.1; the 26th suite): the living world at Update 4's scale. The Scene Book and its
// arrivals, Growth, the TPI, the Arc Book and the Director, the seating rule, the crowd, promotion and the scale runs, with the
// held turn's slice invariance (0.5, 2 and unlimited ms over a 30-day run that crosses a Monday the 1st) and its structural checks.
// Default run <= 45 s alone (the cap is its TIMEOUT); slow modes "long" (365-730 days x 2-3 seeds), "big3k" (the grown 3,000
// fixture) and "big" (1,100 days to the scene's 10,000, then to day 1,460) run outside ctest. The lead's file (A3's slice harness);
// each track's checks are in its own file: _scene (W-arrive), _growth (W-growth), _story (W-story), u4_session.cpp (W-table, then
// S-crowd), _crowd (S-crowd), _promote (S-promote), _scale (S-scale).
// Phase A (§6 PA-48, the lead): the suite is registered. The story-surfaces seat harness (§6 PA-49, u4_seat_harness.h) is included so
// it compiles with the suite; W-table's fidelity_exact runs it at G2. A3 (step A-2, §6 PA-33, PA-35, §6.6) adds the held turn's slice
// harness below, world-only and session-wide, and Phase A's turn checks (SegTurn): slice invariance at 0.5, 2 and unlimited ms (and
// 0: one unit a call) over a run from Sun Jan 3 to Tue Feb 2, 2027 (crossing Monday, Feb 1, 2027), a copy taken mid-hold,
// save-during-hold and texts-during-hold in their Phase A form, the HoldGuard, the structural turn checks, and PA-33's two caches.
//   world_u4_test                    the checks (ctest)
//   world_u4_test turn-session       the session-wide slice checks alone, whether or not this build wires them (see kTurnSessionWired)
#include "u4_seat_harness.h"

#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/World.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace world_u4_test
{
int Failures = 0;
int Checks = 0;

void Expect(bool Condition, const char* What)
{
	++Checks;
	if (!Condition)
	{
		++Failures;
		std::printf("FAILED: %s\n", What);
	}
}

void Expect(bool Condition, const std::string& What)
{
	Expect(Condition, What.c_str());
}

void SegScene();   // world_u4_scene.cpp (W-arrive)
void SegGrowth();  // world_u4_growth.cpp (W-growth)
void SegStory();   // world_u4_story.cpp (W-story)
void SegSession(); // u4_session.cpp (W-table; S-crowd at G4)
void SegCrowd();   // world_u4_crowd.cpp (S-crowd)
void SegPromote(); // world_u4_promote.cpp (S-promote)
void SegScale();   // world_u4_scale.cpp (S-scale)

// ================================================================== the held turn's slice harness (A3; SPEC v1.7 §3.7, §6.6, PA-35)
//
// The helpers G2's turn checks build on (scene_slice_invariant, scene_turn_4ms, turn_structure, scene_save_during_hold,
// scene_texts_hold; declare the ones you use in your own file, as each track does with Expect):
//   ss::world::World TurnWorld(uint32_t Seed, int Day)             an established world made at 06:00 on Day
//   TurnRun TurnFrames(ss::world::World& W, const TurnPlan& P)    frames in a session's rhythm to P.To under P.BudgetMs (world-only)
//   TurnSessionRun TurnSession(const TurnSessionPlan& P)          a career's Session over P.Days days at P.BudgetMs (session-wide)
//   std::string TurnText(const ss::world::World& W), uint32_t TurnFnv(const std::string& S)
// A frame is AdvanceTo(the session's clock), as Session::WorldStep does every frame. The rhythm (TurnStep): a minute a frame for the
// half hour either side of midnight (so a midnight turn is held across frames), seven minutes otherwise, and the afternoon from 14:00
// in one three-hour frame (the calendar running over a gap, like the fixtures' At); TurnPlan::JumpAt adds one frame that jumps days
// ahead in a single AdvanceTo (a skip). Draining (the session's drain-on-touch) happens where the plan says: every day's noon frame
// (a checkpoint: the world's text hashed after the drain) and the end. Two runs compare only when they make the same AdvanceTo calls
// from the same world (Rev moves once a call, and a call's end bounds its hour slices): TurnFramesTo replays the rhythm to a time.

/** The budgets Phase A's invariance runs at (ms): unlimited (as before), 2 (G2's), 0.5 (§6.6's forced) and 0 (one unit a call). */
const double kTurnBudgets[] = {std::numeric_limits<double>::infinity(), 2.0, 0.5, 0.0};
/** The run (30 days): an established world from 06:00 on Sun Jan 3, 2027 (day 90) to 06:00 on Tue Feb 2, 2027 (day 120), in frames
 *  but for one 22-day skip in a single AdvanceTo (Tue Jan 5 to Wed Jan 27: forty-odd units in one target, so a hold spans many
 *  frames and many queued targets); it crosses Mon Feb 1, 2027 (a Monday the 1st, day 119: NewMonth, NewWeek and NewDay in one
 *  turn) in frames. kTurnMidDay's 06:00 is where the copy, save, guard and cache checks start, from the unlimited run's world. */
constexpr int kTurnFirstDay = 90;
constexpr int kTurnLastDay = 120;
constexpr int kTurnMonday1st = 119;
constexpr int kTurnMidDay = 116;
constexpr double kTurnSkipAt = 92.0 * 1440.0 + 15.0 * 60.0;
constexpr double kTurnSkipDays = 22.0;
constexpr uint32_t kTurnSeed = 0x7a3b1du;

uint32_t TurnFnv(const std::string& S)
{
	uint32_t H = 2166136261u;
	for (const char Ch : S)
	{
		H = (H ^ static_cast<unsigned char>(Ch)) * 16777619u;
	}
	return H;
}

std::string TurnText(const ss::world::World& W)
{
	std::string Out;
	W.Write(Out);
	return Out;
}

ss::world::World TurnWorld(uint32_t Seed, int Day)
{
	ss::world::World W;
	W.Create(Seed, static_cast<double>(Day) * 1440.0 + 6.0 * 60.0);
	return W;
}

/** A world read back from its text (the save's world lines). */
void TurnLoad(ss::world::World& W, const std::string& Text)
{
	size_t Start = 0;
	while (Start < Text.size())
	{
		size_t End = Text.find('\n', Start);
		End = End == std::string::npos ? Text.size() : End;
		std::vector<std::string> Fields;
		size_t From = Start;
		for (size_t K = Start; K <= End; ++K)
		{
			if (K == End || Text[K] == '\t')
			{
				Fields.push_back(Text.substr(From, K - From));
				From = K + 1;
			}
		}
		W.Read(Fields);
		Start = End + 1;
	}
	W.Finish();
}

/** The next frame's clock. */
double TurnStep(double T)
{
	const double M = std::fmod(T, 1440.0);
	if (M >= 1410.0 || M < 30.0)
	{
		return T + 1.0;
	}
	if (M >= 840.0 && M < 847.0)
	{
		return T + 180.0;
	}
	return T + 7.0;
}

struct TurnPlan
{
	double BudgetMs = std::numeric_limits<double>::infinity();
	double To = 0.0;               // the run's end (world minutes)
	double JumpAt = -1.0;          // the first frame at or past it jumps JumpDays ahead in one AdvanceTo (-1: none)
	double JumpDays = 0.0;
	bool DrainDaily = true;        // the noon drain and checkpoint
	bool News = false;             // Session::WorldStep's phone scan (§3.7 R6: none while the world holds)
	bool NewsWhileHolding = false; // (the planted bug: the scan mid-hold)
	double SaveEvery = 0.0;        // a background save's frozen copy every this many world minutes, kept a frame (0: none)
	std::function<void(ss::world::World&, double)> Frame; // after each frame's AdvanceTo (a save request, a planted call)
};

struct TurnRun
{
	std::string Text;               // the world's text at the end (drained)
	std::vector<uint32_t> Checks;   // each noon checkpoint's hash
	std::vector<std::string> News;  // the phone scan's events (At:npc:kind:what), in scan order
	ss::world::World::TurnStats Stats;
	int Frames = 0;
	int HeldFrames = 0;             // frames that ended with the world holding
	int Rev = 0;
	int HeroRev = 0;
};

TurnRun TurnFrames(ss::world::World& W, const TurnPlan& P)
{
	TurnRun Out;
	W.TurnBudgetMs = P.BudgetMs;
	W.ResetTurnStats();
	double T = W.Clock();
	double NewsAt = W.Clock();
	int Checked = -1;
	bool Jumped = false;
	std::unique_ptr<ss::world::World> Saving; // the save's copy, sharing the world's roster pages until the next frame (PA-34)
	double SavedAt = W.Clock();
	while (T < P.To)
	{
		double Next = TurnStep(T);
		if (!Jumped && P.JumpAt >= 0.0 && Next >= P.JumpAt)
		{
			Next = P.JumpAt + P.JumpDays * 1440.0;
			Jumped = true;
		}
		T = std::min(Next, P.To);
		W.AdvanceTo(T);
		Saving.reset();
		if (P.SaveEvery > 0.0 && !W.Holding() && T - SavedAt >= P.SaveEvery)
		{
			Saving.reset(new ss::world::World(W));
			SavedAt = T;
		}
		++Out.Frames;
		Out.HeldFrames += W.Holding() ? 1 : 0;
		if (P.Frame)
		{
			P.Frame(W, T);
		}
		if (P.News && (P.NewsWhileHolding || !W.Holding()) && W.Clock() > NewsAt)
		{
			const std::vector<ss::world::WorldEvent>& Log = W.Events();
			for (auto It = Log.rbegin(); It != Log.rend() && It->At > NewsAt; ++It)
			{
				Out.News.push_back(std::to_string(static_cast<long long>(std::llround(It->At))) + ":" + std::to_string(It->Npc) + ":" +
					std::to_string(static_cast<int>(It->Kind)) + ":" + It->What);
			}
			NewsAt = W.Clock();
		}
		const int Day = static_cast<int>(std::floor(T / 1440.0));
		const double Minute = std::fmod(T, 1440.0);
		if (P.DrainDaily && Day != Checked && Minute >= 720.0 && Minute < 727.0) // the rhythm's noon frame (a skip landing later waits a day)
		{
			W.DrainHold();
			Out.Checks.push_back(TurnFnv(TurnText(W)));
			Checked = Day;
		}
	}
	W.DrainHold();
	Out.Text = TurnText(W);
	Out.Stats = W.TurnCounters();
	Out.Rev = W.Revision();
	Out.HeroRev = W.HeroRevision();
	return Out;
}

std::string TurnBudgetName(double Ms)
{
	if (!(Ms < std::numeric_limits<double>::infinity()))
	{
		return "unlimited";
	}
	char Buf[32];
	std::snprintf(Buf, sizeof(Buf), "%g ms", Ms);
	return Buf;
}

// ------------------------------------------------------------------ the session-wide helpers

/** Session-wide slicing needs the session's half of the machine (§6.3 A1: DrainHold before every outcome-affecting LivingWorld call
 *  and the 32 hand-offs, Save deferred while holding, WorldStep returning while holding). Until A1's track is merged a held
 *  session world reads half-turned state, so the default run checks session-wide slicing only once this is true (the integrator
 *  sets it with the merge; `world_u4_test turn-session` runs them in any build). The transcript at SS_TURN_BUDGET=0.5 is the same
 *  check end to end (Session::StartWorld applies World::ForcedTurnBudgetMs, patch a2-A3_to_a2-A1_1). Step A-2's integration merged
 *  A1, A2 and A3 with Session::DrainWorld (patch a2-A3_to_a2-A1_2), so the default run checks it. */
constexpr bool kTurnSessionWired = true;

struct TurnHooks : ss::SessionHooks
{
	std::vector<std::string> Texts;
	ss::SaveData Last;
	int Saves = 0;
	void Text(const std::string& From, const std::string& Body) override { Texts.push_back(From + ": " + Body); }
	void Save(const ss::SaveData& D) override
	{
		++Saves;
		Last = D;
	}
	bool GoOut(const std::string&, ss::Chips) override { return true; }
};

struct TurnSessionPlan
{
	double BudgetMs = std::numeric_limits<double>::infinity();
	int Days = 30;
	std::string Seed = "u4-turn";
	/** Every frame, with the frame's index: a save request, say (§3.7 R5's check asks for one every frame of a hold). */
	std::function<void(ss::Session&, int)> Frame;
};

struct TurnSessionRun
{
	std::vector<std::string> Texts; // the phone, in order
	std::string Save;               // the career's save at the end (the world drained first)
	int HeldFrames = 0;
	ss::world::World::TurnStats Stats;
};

ss::hero::Character TurnSomeone()
{
	ss::hero::Character C;
	C.FirstName = "Jesse";
	C.LastName = "Cole";
	C.Story = ss::hero::Background::DealersKid;
	C.Created = true;
	return C;
}

/** A career's days in the fixtures' rhythm: each morning the clock jumps to 07:00 (the calendar runs over the gap) and a few
 *  frames pass; each evening it walks the hour either side of midnight a minute a frame (frames at 30 a second). */
TurnSessionRun TurnSession(const TurnSessionPlan& P)
{
	TurnSessionRun Out;
	TurnHooks H;
	std::unique_ptr<ss::Session> S(new ss::Session(H, P.Seed));
	S->NewCareer(TurnSomeone());
	S->CurrentScreen = ss::Screen::Lobby;
	S->Living().TurnBudgetMs = P.BudgetMs;
	S->Living().ResetTurnStats();
	double Now = 0.0;
	int Frame = 0;
	auto Tick = [&]() {
		Now += 1.0 / 30.0;
		S->Update(Now);
		Out.HeldFrames += S->Living().Holding() ? 1 : 0;
		if (P.Frame)
		{
			P.Frame(*S, Frame);
		}
		++Frame;
	};
	for (int K = 0; K < 6; ++K)
	{
		Tick();
	}
	const int First = static_cast<int>(std::floor(S->WorldMinutes() / 1440.0));
	for (int D = First + 1; D <= First + P.Days; ++D)
	{
		S->Life.Hunger = 10.0;
		S->LobbyMinutes += std::max(0.0, static_cast<double>(D) * 1440.0 + 7.0 * 60.0 - S->WorldMinutes());
		for (int K = 0; K < 4; ++K)
		{
			Tick();
		}
		S->LobbyMinutes += std::max(0.0, static_cast<double>(D) * 1440.0 + 23.0 * 60.0 - S->WorldMinutes());
		while (S->WorldMinutes() < static_cast<double>(D + 1) * 1440.0 + 60.0)
		{
			S->LobbyMinutes += 1.0;
			Tick();
		}
	}
	S->Living().DrainHold();
	S->Save();
	Out.Texts = H.Texts;
	Out.Save = H.Last.Serialize();
	Out.Stats = S->Living().TurnCounters();
	return Out;
}

// ------------------------------------------------------------------ the HoldGuard's test sink

int TurnGuardHits = 0;
std::string TurnGuardLast;

void TurnGuardCount(const char* Method)
{
	++TurnGuardHits;
	TurnGuardLast = Method;
}

/** Frames in the rhythm from the world's clock until the frame at or past To (the world's own budget): the same calls as any other
 *  run from the same world. Returns the last frame's clock. */
double TurnFramesTo(ss::world::World& W, double To)
{
	double T = W.Clock();
	while (T < To)
	{
		T = TurnStep(T);
		W.AdvanceTo(T);
	}
	return T;
}

/** From a world that isn't holding, frames at a budget of 0 until it holds a turn on Day's midnight (the frames that cross it);
 *  Lagging: until its clock also stands behind the time asked for (frames' targets queued). The frame's clock in T; false: never. */
bool TurnHoldAtMidnight(ss::world::World& W, int Day, double& T, bool Lagging = false)
{
	W.TurnBudgetMs = 0.0;
	T = W.Clock();
	while (T < static_cast<double>(Day) * 1440.0 + 30.0)
	{
		T = TurnStep(T);
		W.AdvanceTo(T);
		if (W.Holding() && T >= static_cast<double>(Day) * 1440.0 && (!Lagging || W.Clock() < T))
		{
			return true;
		}
	}
	return false;
}

// ------------------------------------------------------------------ PA-33's caches, against the rules worked out afresh

/** The hour-ago Night Shift values, from the world's own text (its "nightago" line: id~value records, written to round-trip). */
std::vector<std::pair<int, double>> TurnNightAgo(const std::string& Text)
{
	std::vector<std::pair<int, double>> Out;
	const std::string Key = "world\tnightago\t";
	const size_t At = Text.find(Key);
	if (At == std::string::npos)
	{
		return Out;
	}
	const size_t End = Text.find('\n', At);
	const std::string Line = Text.substr(At + Key.size(), (End == std::string::npos ? Text.size() : End) - At - Key.size());
	size_t From = 0;
	while (From < Line.size())
	{
		size_t Bar = Line.find('|', From);
		Bar = Bar == std::string::npos ? Line.size() : Bar;
		const std::string Rec = Line.substr(From, Bar - From);
		const size_t Tilde = Rec.find('~');
		if (Tilde != std::string::npos)
		{
			Out.push_back({std::atoi(Rec.c_str()), std::strtod(Rec.c_str() + Tilde + 1, nullptr)});
		}
		From = Bar + 1;
	}
	return Out;
}

/** Night Shift's rank an hour ago by today's rule (update 2 §3.6): 1 + how many had more; 0 for nothing an hour ago. */
int TurnNightRankByCount(const std::vector<std::pair<int, double>>& Ago, int Npc)
{
	double V = 0.0;
	for (const auto& It : Ago)
	{
		V = It.first == Npc ? It.second : V;
	}
	int Rank = 1;
	for (const auto& It : Ago)
	{
		Rank += It.second > V ? 1 : 0;
	}
	return V > 0.0 ? Rank : 0;
}

/** Every person on the hour-ago board, and the first 40 ids, ranked by the world and by the rule; the first mismatch's id, or -1.
 *  Ago: how many were on the board. */
int TurnNightMismatch(const ss::world::World& W, size_t* Ago = nullptr)
{
	const std::vector<std::pair<int, double>> Board = TurnNightAgo(TurnText(W));
	if (Ago)
	{
		*Ago = Board.size();
	}
	std::vector<int> Ids;
	for (const auto& It : Board)
	{
		Ids.push_back(It.first);
	}
	for (int Id = 0; Id < 40; ++Id)
	{
		Ids.push_back(Id);
	}
	for (const int Id : Ids)
	{
		if (W.RankBefore(ss::net::Board::NightShift, Id) != TurnNightRankByCount(Board, Id))
		{
			return Id;
		}
	}
	return -1;
}

/** The world's text with its tonight's-points line ("night") replaced by the hour-ago board's people, the K-th given the next one's
 *  value plus K points: the next hour mark copies the same count of people over the hour-ago board with other values (not just the
 *  same values handed round: a rank is a count over the values, which a permutation keeps). */
std::string TurnNightMoved(const std::string& Text)
{
	const std::string AgoKey = "world\tnightago\t";
	const size_t At = Text.find(AgoKey);
	if (At == std::string::npos)
	{
		return Text;
	}
	const size_t End = std::min(Text.find('\n', At), Text.size());
	const std::string Line = Text.substr(At + AgoKey.size(), End - At - AgoKey.size());
	std::vector<std::string> Ids;
	std::vector<std::string> Values;
	size_t From = 0;
	while (From < Line.size())
	{
		size_t Bar = Line.find('|', From);
		Bar = Bar == std::string::npos ? Line.size() : Bar;
		const std::string Rec = Line.substr(From, Bar - From);
		const size_t Tilde = Rec.find('~');
		if (Tilde != std::string::npos)
		{
			Ids.push_back(Rec.substr(0, Tilde));
			Values.push_back(Rec.substr(Tilde + 1));
		}
		From = Bar + 1;
	}
	std::string Night = "world\tnight\t";
	for (size_t K = 0; K < Ids.size(); ++K)
	{
		char Value[48];
		std::snprintf(Value, sizeof(Value), "%.12g", std::strtod(Values[(K + 1) % Values.size()].c_str(), nullptr) + static_cast<double>(K));
		Night += (K > 0 ? "|" : "") + Ids[K] + "~" + Value;
	}
	std::string Out = Text;
	const size_t Was = Out.find("world\tnight\t");
	if (Was != std::string::npos)
	{
		Out.replace(Was, std::min(Out.find('\n', Was), Out.size()) - Was, Night);
	}
	else
	{
		Out.insert(At, Night + "\n");
	}
	return Out;
}

/** World::EmbercrestRegulars' rule (G/WorldHero.cpp), worked out afresh from the public view: its first Count (the whole pool for
 *  Count < 0). */
std::vector<int> TurnRegularsByRule(const ss::world::World& W, int Count = ss::world::World::EmbercrestRegularCount)
{
	std::vector<std::pair<uint64_t, int>> Pool;
	for (const ss::world::Npc& N : W.People())
	{
		if (!N.Faded && N.St != ss::world::Status::Retired && N.From != ss::world::Origin::Cast && !N.Rival && N.Home == ss::world::Region::Americas &&
			N.Country == "US" && (N.Live >= ss::world::LiveLevel::Local || N.PeakRoll >= 100000))
		{
			const uint64_t Tier = N.From == ss::world::Origin::Local ? 0u : N.Live >= ss::world::LiveLevel::Local ? 1u : 2u;
			Pool.push_back({(Tier << 32) | ss::live::FaceHash(N.Name), N.Id});
		}
	}
	std::sort(Pool.begin(), Pool.end());
	std::vector<int> Out;
	for (size_t I = 0; I < Pool.size() && (Count < 0 || I < static_cast<size_t>(Count)); ++I)
	{
		Out.push_back(Pool[I].second);
	}
	return Out;
}

/** The world's text with every person in Abroad moved out of the US (their npc line's country "US" -> "CA"). */
std::string TurnAbroad(const std::string& Text, const std::vector<int>& Abroad)
{
	std::vector<bool> Move;
	for (const int Id : Abroad)
	{
		Move.resize(std::max(Move.size(), static_cast<size_t>(Id) + 1), false);
		Move[static_cast<size_t>(Id)] = true;
	}
	std::string Out;
	size_t Start = 0;
	while (Start < Text.size())
	{
		size_t End = Text.find('\n', Start);
		End = End == std::string::npos ? Text.size() : End + 1;
		std::string Line = Text.substr(Start, End - Start);
		const std::string Key = "world\tnpc\t";
		if (Line.compare(0, Key.size(), Key) == 0)
		{
			const int Id = std::atoi(Line.c_str() + Key.size());
			const size_t Name = Line.find('\t', Key.size());
			const size_t Country = Name == std::string::npos ? std::string::npos : Line.find('\t', Name + 1);
			if (Id >= 0 && static_cast<size_t>(Id) < Move.size() && Move[static_cast<size_t>(Id)] && Country != std::string::npos &&
				Line.compare(Country, 4, "\tUS\t") == 0)
			{
				Line.replace(Country, 4, "\tCA\t");
			}
		}
		Out += Line;
		Start = End;
	}
	return Out;
}

/** What the unlimited run checks of the caches as it goes. */
struct TurnCacheWatch
{
	int NightChecks = 0;
	int NightMiss = -1;
	size_t NightMost = 0;
	int NightSameSize = 0;   // hour marks that moved the hour-ago values but not their count (what the hour in the key is for)
	size_t NightLastSize = 0;
	double NightLastHour = -1.0;
	int RegularChecks = 0;
	int RegularMiss = 0;
	int RegularChanges = 0;
	std::vector<int> RegularLast;
};

// ------------------------------------------------------------------ Phase A's checks

/** turn_slice_world (§6.6; G2's scene_slice_invariant builds on it), with turn_texts_hold (§3.7 R6, Phase A's world-level form of
 *  scene_texts_hold) and turn_structure (§3.7 R4's structural checks, Phase A's form) read off the same runs: the 30-day run at
 *  2, 0.5 and 0 ms equals the unlimited one: every noon checkpoint, the world's text at the end and its revisions; the phone's scan
 *  of the news, skipped while the world holds, texts the same events; and at 2 and 0 ms one AdvanceTo runs at most 40 units and
 *  clones at most 8 roster pages, and (once the units are A2's splits) no unit walks more than 500 people. The unlimited run also
 *  watches PA-33's caches (Night Shift's ranks every half hour of kTurnMidDay's two nights, the regulars every 25th frame) and keeps
 *  its world at kTurnMidDay's 06:00 for the checks after it.
 *  Mutations: the hold merging the frames' targets into one, or Rev moved when a call stops (the revisions and texts differ); the
 *  scan mid-hold (texts lost at the held instant, once NewDay's units are A2's splits: with PlanDay and NewDay whole, nothing posts
 *  at the held instant after a stop); the budget never checked (the skip runs forty-odd units in one call); PlanDay as one unit. */
void TurnSliceWorld(const ss::world::World& Base, ss::world::World& Mid, TurnCacheWatch& Watch)
{
	TurnRun Unlimited;
	std::vector<std::string> NewsUnlimited;
	bool HaveMid = false;
	for (const double Budget : kTurnBudgets)
	{
		const bool Endless = !(Budget < std::numeric_limits<double>::infinity());
		// Each budget's run from a world of its own (a copy of Base would share its roster pages, PA-34, and clone them as it went).
		ss::world::World W = Endless ? Base : TurnWorld(kTurnSeed, kTurnFirstDay);
		TurnPlan P;
		P.BudgetMs = Budget;
		P.To = static_cast<double>(kTurnLastDay) * 1440.0 + 6.0 * 60.0;
		P.JumpAt = kTurnSkipAt;
		P.JumpDays = kTurnSkipDays;
		P.News = true;
		// At 0.5 ms, a background save's copy every 60 world minutes, as Session saves (§3.7 R4): its pages clone inside the slices.
		P.SaveEvery = Budget == 0.5 ? 60.0 : 0.0;
		int Frame = 0;
		if (Endless)
		{
			P.Frame = [&](ss::world::World& Now, double T) {
				++Frame;
				if (!HaveMid && T >= static_cast<double>(kTurnMidDay) * 1440.0 + 6.0 * 60.0)
				{
					Mid = Now;
					HaveMid = true;
				}
				// Night Shift: every half hour of kTurnMidDay's two nights, and the first frame after every hour mark of every night
				// (18:00 to 06:00) from the skip's landing on.
				const double M = std::fmod(T, 60.0);
				const double Clock = std::fmod(T, 1440.0);
				const bool HalfHours = T >= static_cast<double>(kTurnMidDay) * 1440.0 && T < static_cast<double>(kTurnMidDay + 2) * 1440.0 && (M < 1.0 || (M >= 30.0 && M < 37.0));
				const bool NightHour = T > kTurnSkipAt + kTurnSkipDays * 1440.0 && (Clock >= 18.0 * 60.0 || Clock < 6.0 * 60.0 + 7.0) &&
					std::floor(T / 60.0) != Watch.NightLastHour;
				if (Watch.NightMiss < 0 && (HalfHours || NightHour))
				{
					size_t Ago = 0;
					Watch.NightMiss = TurnNightMismatch(Now, &Ago);
					Watch.NightMost = std::max(Watch.NightMost, Ago);
					++Watch.NightChecks;
					if (NightHour)
					{
						Watch.NightSameSize += Watch.NightLastHour == std::floor(T / 60.0) - 1.0 && Ago == Watch.NightLastSize && Ago > 0 ? 1 : 0;
						Watch.NightLastSize = Ago;
						Watch.NightLastHour = std::floor(T / 60.0);
					}
				}
				if (Frame % 25 == 0)
				{
					const std::vector<int> Rule = TurnRegularsByRule(Now);
					Watch.RegularMiss += Now.EmbercrestRegulars() == Rule ? 0 : 1;
					Watch.RegularChanges += !Watch.RegularLast.empty() && Rule != Watch.RegularLast ? 1 : 0;
					Watch.RegularLast = Rule;
					++Watch.RegularChecks;
				}
			};
		}
		TurnRun R = TurnFrames(W, P);
		std::sort(R.News.begin(), R.News.end()); // the same texts, whatever frame the phone got them in (a hold sends a few frames' worth at once)
		const std::string At = " at " + TurnBudgetName(Budget);
		std::printf("  turn_slice_world%s: %d frames, %d held, %d units (max %d a call, %d a drain), %d holds, %d drains, longest call %.2f ms, "
					"%d clones (max %d a call), %zu checkpoints, %zu texts\n",
			At.c_str(), R.Frames, R.HeldFrames, R.Stats.Units, R.Stats.MaxUnitsPerCall, R.Stats.MaxDrainUnits, R.Stats.Holds, R.Stats.Drains, R.Stats.MaxCallMs,
			R.Stats.Clones, R.Stats.MaxClonesPerCall, R.Checks.size(), R.News.size());
		if (Endless)
		{
			Unlimited = R;
			NewsUnlimited = R.News;
			Expect(R.Stats.Holds == 0 && R.HeldFrames == 0, "turn_slice_world: an unlimited budget never holds");
			Expect(R.Checks.size() >= 7 && !R.News.empty() && HaveMid, "turn_slice_world: the run has its checkpoints, its news and its kTurnMidDay world");
			continue;
		}
		Expect(R.Stats.Holds > 0 && R.HeldFrames > 0, "turn_slice_world" + At + ": the world holds turns across frames");
		if (Budget <= 0.0)
		{
			Expect(R.Stats.MaxUnitsPerCall == 1, "turn_slice_world: a budget of 0 runs one unit a call");
		}
		size_t Same = 0;
		while (Same < R.Checks.size() && Same < Unlimited.Checks.size() && R.Checks[Same] == Unlimited.Checks[Same])
		{
			++Same;
		}
		Expect(R.Checks.size() == Unlimited.Checks.size() && Same == R.Checks.size(),
			"turn_slice_world" + At + ": every noon checkpoint equals the unlimited run's (first difference at checkpoint " + std::to_string(Same) + ")");
		Expect(R.Text == Unlimited.Text, "turn_slice_world" + At + ": the world's text at the end equals the unlimited run's");
		Expect(R.Rev == Unlimited.Rev && R.HeroRev == Unlimited.HeroRev, "turn_slice_world" + At + ": the same revisions (Rev moves once a target)");
		Expect(R.News == NewsUnlimited, "turn_texts_hold" + At + ": the phone's scan texts the same events as at unlimited");
		if (Budget == 2.0 || Budget <= 0.0)
		{
			Expect(R.Stats.MaxUnitsPerCall >= 1 && R.Stats.MaxUnitsPerCall <= 40, "turn_structure" + At + ": one AdvanceTo runs at most 40 units (most: " +
				std::to_string(R.Stats.MaxUnitsPerCall) + ")");
			Expect(R.Stats.MaxClonesPerCall <= 8, "turn_structure" + At + ": a slice clones at most 8 roster pages");
			if (ss::world::World::TurnUnitsRanged())
			{
				Expect(R.Stats.MaxWalked <= 500, "turn_structure" + At + ": no unit walks more than 500 people (most: " + std::to_string(R.Stats.MaxWalked) + ")");
			}
		}
	}
	Expect(Unlimited.Stats.MaxUnitsPerCall > 40, "turn_structure: the unlimited run's skip runs more than 40 units in one call (the bound has something to bound)");
	if (!ss::world::World::TurnUnitsRanged())
	{
		std::printf("  turn_structure: SKIP the 500-people bound (this build runs PlanDay and NewDay whole: A2's splits, SS_WORLDSIM_TURN_PIECES in G/WorldSim.h, aren't in it)\n");
	}
}

/** turn_hold_copy (§3.7 R5: the hold's state is index-based, so World stays copyable): a copy taken mid-hold (WorldPreview's, a
 *  snapshot's) drains to the original's world, which equals the unlimited run's after the same frames; Simulate on a holding copy
 *  (the preview) runs from the time asked for, to the end, and equals the unlimited world's. Mutation: Simulate from Clock() (the
 *  held instant) instead. */
void TurnHoldCopy(const ss::world::World& Mid)
{
	ss::world::World W = Mid;
	double T = 0.0;
	const bool Held = TurnHoldAtMidnight(W, kTurnMonday1st, T, true);
	Expect(Held, "turn_hold_copy: a budget of 0 holds the Monday-the-1st turn, its clock behind the frames'");
	if (!Held)
	{
		return;
	}
	ss::world::World Ref = Mid;
	TurnFramesTo(Ref, T);
	ss::world::World Copy = W;
	Expect(Copy.Holding() && Copy.Clock() < T, "turn_hold_copy: the copy holds too, its clock at the held instant");
	ss::world::World Peek = W;
	Copy.DrainHold();
	W.DrainHold();
	Expect(!W.Holding() && W.Clock() == T, "turn_hold_copy: a drain reaches the time asked for");
	Expect(TurnText(Copy) == TurnText(W), "turn_hold_copy: the copy drains to the original's world");
	Expect(TurnText(W) == TurnText(Ref), "turn_hold_copy: and to the unlimited run's after the same frames");
	Peek.Simulate(3);
	Ref.Simulate(3);
	Expect(!Peek.Holding() && Peek.Clock() == Ref.Clock() && TurnText(Peek) == TurnText(Ref), "turn_hold_copy: Simulate(3) on a holding copy equals the unlimited world's");
}

/** turn_save_hold (§3.7 R5, Phase A's world-level form of scene_save_during_hold): a save asked for at every frame of the
 *  Monday-the-1st hold waits for the hold's end (as Session::Save will, bSaveAfterHold); that save, read back and played on three
 *  days in the same frames as the unbroken world, equals it byte for byte. Mutation: the save written mid-hold. */
bool TurnSaveHold(const ss::world::World& Mid, double BudgetMs, bool WriteMidHold, std::string& Why)
{
	ss::world::World W = Mid;
	W.TurnBudgetMs = BudgetMs;
	double T = W.Clock();
	const double From = static_cast<double>(kTurnMonday1st) * 1440.0;
	std::string Saved;
	bool Asked = false;
	int Requests = 0;
	while (Saved.empty() && T < From + 1440.0)
	{
		T = TurnStep(T);
		W.AdvanceTo(T);
		if (T >= From && W.Holding())
		{
			++Requests;
			Asked = true; // the session asks for a save: it waits for the hold's end
			if (WriteMidHold)
			{
				Saved = TurnText(W);
			}
		}
		else if (Asked)
		{
			Saved = TurnText(W); // the frame the hold completes: the waiting save
		}
	}
	if (Requests == 0 || Saved.empty())
	{
		Why = "no save was asked for during a hold";
		return false;
	}
	ss::world::World Back;
	TurnLoad(Back, Saved);
	ss::world::World& Unbroken = W;
	Unbroken.TurnBudgetMs = std::numeric_limits<double>::infinity();
	const double End = From + 3.0 * 1440.0 + 9.0 * 60.0;
	TurnFramesTo(Unbroken, End);
	TurnFramesTo(Back, End);
	if (TurnText(Back) != TurnText(Unbroken))
	{
		Why = "the save read back and played on three days differs from the unbroken run (" + std::to_string(Requests) + " requests)";
		return false;
	}
	Why = std::to_string(Requests) + " requests waited";
	return true;
}

/** turn_guard (§3.7 R3, rev-code M2): a guarded World method called while the world holds a turn, outside a drain, reaches the
 *  HoldGuard (a test build aborts unless a sink is set); a display read and a call after a drain don't; EnsurePlanned drains.
 *  Mutation: the guard off. */
void TurnGuard(const ss::world::World& Mid)
{
	ss::world::World W = Mid;
	double T = 0.0;
	if (!TurnHoldAtMidnight(W, kTurnMidDay + 1, T))
	{
		Expect(false, "turn_guard: no hold to test against");
		return;
	}
	const ss::world::World::HoldGuardSink Was = ss::world::World::SetHoldGuardSink(&TurnGuardCount);
	TurnGuardHits = 0;
	TurnGuardLast.clear();
	(void)W.BoardValue(ss::net::Board::Season, 0, T); // display reads
	(void)W.RankBefore(ss::net::Board::NightShift, 0);
	(void)W.Leaders(ss::world::Rep::Overall, 5);
	Expect(TurnGuardHits == 0, "turn_guard: display reads pass during a hold");
	(void)W.ResultOf("turn-guard@0"); // planted calls: no drain first
	Expect(TurnGuardHits == 1 && TurnGuardLast == "ResultOf", "turn_guard: an undrained ResultOf during a hold reaches the guard");
	W.Remember(0, ss::world::MemoryKind::Met, "turn guard", 0, T);
	Expect(TurnGuardHits == 2 && TurnGuardLast == "Remember", "turn_guard: an undrained Remember during a hold reaches the guard");
	Expect(W.TurnCounters().GuardTrips == 2, "turn_guard: the counters count the trips");
	W.EnsurePlanned(static_cast<int>(std::floor(T / 1440.0)) + 1); // drains itself (§6 PA-35: the drains in EnsurePlanned)
	Expect(!W.Holding(), "turn_guard: EnsurePlanned drains a held turn");
	(void)W.ResultOf("turn-guard@0");
	Expect(TurnGuardHits == 2, "turn_guard: after the drain the call passes");
	ss::world::World::SetHoldGuardSink(Was);
}

/** turn_caches (§6 PA-33's hour and day items): Night Shift's rank a row looks up equals the count over the hour-ago values (every
 *  half hour of two nights, every night hour after the skip, a reload, an hour mark that moves the values but not their count, a new
 *  night's clear); EmbercrestRegulars' kept answer equals the rule worked out afresh (every 25th frame of the 30 days, after money
 *  moves 80 outsiders, a reload). Mutations: the ranks built once; the ranks not rebuilt when the hour moves; the regulars kept across
 *  revisions and planned days; the regulars kept for good. */
void TurnCaches(const ss::world::World& Mid, const TurnCacheWatch& Watch)
{
	std::printf("  turn_caches: %d hour checks (at most %zu on the hour-ago board, %d hours of the same size), %d regulars checks (the list changed %d times)\n", Watch.NightChecks,
		Watch.NightMost, Watch.NightSameSize, Watch.RegularChecks, Watch.RegularChanges);
	Expect(Watch.NightChecks > 40 && Watch.NightMost > 0 && Watch.NightMiss < 0,
		"turn_caches: Night Shift's rank a row looks up equals the count, every half hour of two nights and every night hour after the skip (first mismatch: npc " + std::to_string(Watch.NightMiss) + ")");
	Expect(Watch.RegularChecks > 50 && Watch.RegularMiss == 0, "turn_caches: EmbercrestRegulars' kept answer equals the rule worked out afresh, all along (" +
		std::to_string(Watch.RegularMiss) + " misses)");
	// The kept answer's revision (what Rev in the cache's key is for): money that changes the list but not the roster's size. Here
	// the room's pool is far over 64 (its locals and the town's live players fill the list, so an online player's money can't reach
	// it), so it's thinned by hand: a world read back with all but ten of its pool moved abroad keeps a list of ten, and an
	// outsider's money then adds them to it.
	{
		const ss::world::World Full = Mid;
		const std::vector<int> Pool = TurnRegularsByRule(Full, -1);
		const std::vector<int> Abroad(Pool.begin() + static_cast<std::ptrdiff_t>(std::min<size_t>(Pool.size(), 10)), Pool.end());
		ss::world::World Thin;
		TurnLoad(Thin, TurnAbroad(TurnText(Full), Abroad));
		Thin.TurnBudgetMs = std::numeric_limits<double>::infinity();
		const std::vector<int> Before = Thin.EmbercrestRegulars(); // (kept)
		int Outsider = -1;
		for (const ss::world::Npc& N : Thin.People())
		{
			if (Outsider < 0 && !N.Faded && N.St != ss::world::Status::Retired && N.From != ss::world::Origin::Cast && N.From != ss::world::Origin::Local && !N.Rival &&
				N.Home == ss::world::Region::Americas && N.Country == "US" && N.Live < ss::world::LiveLevel::Local && N.PeakRoll < 100000)
			{
				Outsider = N.Id;
			}
		}
		if (Outsider >= 0)
		{
			Thin.HeroMoney(Outsider, 100000, ss::world::MemoryKind::Staked, "turn caches", Thin.Clock());
		}
		const std::vector<int> After = Thin.EmbercrestRegulars();
		Expect(Pool.size() > 64 && Before.size() == 10 && Outsider >= 0 && After.size() == 11 && After == TurnRegularsByRule(Thin),
			"turn_caches: money that changes the list but not the roster's size, the kept regulars follow (a pool of " + std::to_string(Pool.size()) +
				" thinned to " + std::to_string(Before.size()) + ", then " + std::to_string(After.size()) + ")");
	}
	ss::world::World W = Mid;
	W.TurnBudgetMs = std::numeric_limits<double>::infinity();
	// Money moves outsiders toward the room's list (their best bankroll reaches a live game's): the kept answer follows at once.
	(void)W.EmbercrestRegulars();
	std::vector<int> Outsiders;
	for (const ss::world::Npc& N : W.People())
	{
		if (Outsiders.size() < 80 && !N.Faded && N.St != ss::world::Status::Retired && N.From != ss::world::Origin::Cast && N.From != ss::world::Origin::Local &&
			!N.Rival && N.Home == ss::world::Region::Americas && N.Country == "US" && N.Live < ss::world::LiveLevel::Local && N.PeakRoll < 100000)
		{
			Outsiders.push_back(N.Id);
		}
	}
	for (const int Id : Outsiders)
	{
		W.HeroMoney(Id, 100000, ss::world::MemoryKind::Staked, "turn caches", W.Clock());
	}
	Expect(!Outsiders.empty() && W.EmbercrestRegulars() == TurnRegularsByRule(W), "turn_caches: after money moves " + std::to_string(Outsiders.size()) +
		" outsiders, the kept regulars are the rule's");
	// Read back: a fresh world's answers are the same.
	ss::world::World Back;
	TurnLoad(Back, TurnText(W));
	Expect(TurnNightMismatch(Back) < 0 && Back.EmbercrestRegulars() == W.EmbercrestRegulars(), "turn_caches: a world read back ranks and lists the same");
	// An hour mark that moves the hour-ago values but not their count (what the hour in the cache's key is for). A night's board
	// grows every hour under the world's own play (the 30 days saw no such hour: NightSameSize), so it's made by hand: by day the
	// night's points stand still (nothing adds to them after 06:00 and nothing clears them before 18:00), so a world read back at
	// 10:30 with other values for the hour-ago board's people tonight copies them over that board at 11:00.
	{
		ss::world::World Day = Mid;
		Day.TurnBudgetMs = std::numeric_limits<double>::infinity();
		TurnFramesTo(Day, static_cast<double>(kTurnMidDay) * 1440.0 + 10.0 * 60.0 + 30.0);
		ss::world::World Moved;
		TurnLoad(Moved, TurnNightMoved(TurnText(Day)));
		const std::vector<std::pair<int, double>> AgoBefore = TurnNightAgo(TurnText(Moved));
		const int MissBefore = TurnNightMismatch(Moved); // (ranks the board once)
		TurnFramesTo(Moved, (std::floor(Moved.Clock() / 60.0) + 1.0) * 60.0 + 1.0);
		const std::vector<std::pair<int, double>> AgoAfter = TurnNightAgo(TurnText(Moved));
		const int MissAfter = TurnNightMismatch(Moved);
		Expect(AgoBefore.size() > 2 && AgoAfter.size() == AgoBefore.size() && AgoAfter != AgoBefore && MissBefore < 0 && MissAfter < 0,
			"turn_caches: an hour mark that moves the hour-ago values but not their count, the ranks follow (" + std::to_string(AgoBefore.size()) + " then " +
				std::to_string(AgoAfter.size()) + " on the board; first mismatch: npc " + std::to_string(MissAfter) + ")");
	}
	// A new night clears the hour-ago board (Resolve, when the night key moves): the ranks follow it.
	TurnFramesTo(W, W.Clock() + 1440.0 + 7.0);
	Expect(TurnNightMismatch(W) < 0, "turn_caches: through a new night's clear, the ranks follow");
}

void SegTurn()
{
	const auto T0 = std::chrono::steady_clock::now();
	const ss::world::World Base = TurnWorld(kTurnSeed, kTurnFirstDay);
	std::printf("  turn: a world of %zu people from day %d (%s units)\n", Base.People().size(), kTurnFirstDay,
		ss::world::World::TurnUnitsRanged() ? "A2's range splits" : "whole PlanDay and NewDay");
	ss::world::World Mid;
	TurnCacheWatch Watch;
	TurnSliceWorld(Base, Mid, Watch);
	if (!Mid.Ready())
	{
		return;
	}
	TurnHoldCopy(Mid);
	for (const double Budget : {0.5, 0.0})
	{
		std::string Why;
		const bool Ok = TurnSaveHold(Mid, Budget, false, Why);
		Expect(Ok, "turn_save_hold at " + TurnBudgetName(Budget) + ": " + Why);
	}
	TurnGuard(Mid);
	TurnCaches(Mid, Watch);
	std::printf("  turn: %.1fs\n", std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count());
}

/** The session-wide slice checks (§3.7, §6.6): a new career over 28 days (Mon Oct 12 and 19, 2026, the month's turn on Nov 1) at
 *  0.5 ms equals the unlimited one: its phone, in order, and its save. With the HoldGuard armed (a test build aborts on any
 *  undrained World call), it is also the session's drain audit. */
void SegTurnSession()
{
	const auto T0 = std::chrono::steady_clock::now();
	TurnSessionPlan P;
	P.Days = 28;
	const TurnSessionRun Unlimited = TurnSession(P);
	Expect(Unlimited.HeldFrames == 0 && !Unlimited.Texts.empty(), "turn_session: an unlimited budget never holds, and the phone has texts");
	for (const double Budget : {0.5})
	{
		TurnSessionPlan Q = P;
		Q.BudgetMs = Budget;
		const TurnSessionRun R = TurnSession(Q);
		const std::string At = " at " + TurnBudgetName(Budget);
		std::printf("  turn_session%s: %d held frames, %d units, %d holds, %d drains, %d guard trips\n", At.c_str(), R.HeldFrames, R.Stats.Units, R.Stats.Holds,
			R.Stats.Drains, R.Stats.GuardTrips);
		Expect(R.HeldFrames > 0, "turn_session" + At + ": the session's world holds");
		Expect(R.Texts == Unlimited.Texts, "turn_session" + At + ": the phone gets the same texts in the same order");
		Expect(R.Save == Unlimited.Save, "turn_session" + At + ": the same save at the end");
	}
	std::printf("  turn_session: %.1fs\n", std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count());
}
} // namespace world_u4_test

int main(int Argc, char** Argv)
{
	const auto T0 = std::chrono::steady_clock::now();
	if (Argc > 1 && std::strcmp(Argv[1], "turn-session") == 0)
	{
		world_u4_test::SegTurnSession();
	}
	else
	{
		world_u4_test::SegTurn();
		if (world_u4_test::kTurnSessionWired)
		{
			world_u4_test::SegTurnSession();
		}
		else
		{
			std::printf("  turn_session: SKIP the session-wide slice checks (A1's session drains aren't merged in this build; world_u4_test turn-session runs them)\n");
		}
		world_u4_test::SegScene();
		world_u4_test::SegGrowth();
		world_u4_test::SegStory();
		world_u4_test::SegSession();
		world_u4_test::SegCrowd();
		world_u4_test::SegPromote();
		world_u4_test::SegScale();
	}
	const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count();
	std::printf("world u4 tests: %d checks, %d failed in %.1fs\n", world_u4_test::Checks, world_u4_test::Failures, Seconds);
	return world_u4_test::Failures == 0 ? 0 : 1;
}
