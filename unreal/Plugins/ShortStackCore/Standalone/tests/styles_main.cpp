// ai_styles (update 2 SPEC §2.10): autoplay's styles. core-styles.
//
//   style_test              the CI run (<= 30 s): determinism and legality, the chart checksum and anchors, reads::Count,
//                           the slip and Mix, ICM's arithmetic, a smoke test of SNGs and satellites, the copy lint
//   style_test --write      writes Tests/style_vectors.txt (the determinism hashes); review the diff
//   style_test s8 [hands] [sngs]   spike S-8: the ring and SNG harnesses with today's archetypes and Autopilot as the hero:
//                           the measured standard deviations, the cost of a hand, and the sample sizes they set
//   style_test ring [scale] assertions 2, 3, 4, 5, 7, 10 and 14 at S-8's sizes (scale multiplies them; minutes)
//   style_test sng [scale]  assertions 8, 9 and 13 at S-8's sizes (minutes)
//   style_test strength [hypers] [turbos] [deep] [styles]   §2.5's table: Autopilot, coach-best and the 94% hero and
//                           every style at Sharpness 1.0 and 0.80 on the same paired seeds, with intervals (offline, hours)
//   --threads N             worker threads for the long modes (default 3, and at most 3 in the CI run)
//
// The statistics rule (§2.10): fixed seeds make every comparison deterministic, so a comparison passes only when
// (difference - threshold) >= 2 SE, and its sample is sized from S-8's measured SD so that 2 SE <= threshold / 2.
#include "ShortStack/AI/Bot.h"
#include "ShortStack/AI/Charts.h"
#include "ShortStack/AI/Grading.h"
#include "ShortStack/AI/IcmPlay.h"
#include "ShortStack/AI/Model.h"
#include "ShortStack/AI/Profiles.h"
#include "ShortStack/AI/Reads.h"
#include "ShortStack/AI/Style.h"
#include "ShortStack/AI/View.h"
#include "ShortStack/Cards.h"
#include "ShortStack/Equity.h"
#include "ShortStack/Game/Coach.h"
#include "ShortStack/Game/Lobby.h"
#include "ShortStack/Hand.h"
#include "ShortStack/Rng.h"
#include "ShortStack/Structure.h"
#include "ShortStack/Tournament.h"
#include "CopyLint.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <vector>

#ifndef SS_TESTS_DIR
#define SS_TESTS_DIR "."
#endif

// The archetype bases with no jitter (Profiles.cpp; ModelDetail.h is internal to the core, so the harness declares it).
namespace ss
{
namespace profiles_detail
{
Profile Base(Archetype A);
}
} // namespace ss

namespace style_test
{
int Failures = 0;
int Threads = 3;

void Expect(bool Condition, const std::string& What)
{
	if (!Condition)
	{
		++Failures;
		std::printf("FAILED: %s\n", What.c_str());
	}
}

double NowMs()
{
	using Clock = std::chrono::steady_clock;
	return std::chrono::duration<double, std::milli>(Clock::now().time_since_epoch()).count();
}

/** Body(0) .. Body(Count - 1) on the worker threads; each index writes only its own results, so the order never matters. */
template <typename Fn>
void ParallelFor(int Count, Fn&& Body)
{
	if (Threads <= 1 || Count <= 1)
	{
		for (int I = 0; I < Count; ++I)
		{
			Body(I);
		}
		return;
	}
	std::atomic<int> Next{0};
	std::vector<std::thread> Pool;
	const int N = std::min(Threads, Count);
	for (int T = 0; T < N; ++T)
	{
		Pool.emplace_back([&]() {
			for (;;)
			{
				const int I = Next++;
				if (I >= Count)
				{
					break;
				}
				Body(I);
			}
		});
	}
	for (std::thread& T : Pool)
	{
		T.join();
	}
}

uint64_t Fnv64(const std::string& Text, uint64_t H = 14695981039346656037ull)
{
	for (const char Ch : Text)
	{
		H = (H ^ static_cast<unsigned char>(Ch)) * 1099511628211ull;
	}
	return H;
}

std::string Hex64(uint64_t X)
{
	char Buf[32];
	std::snprintf(Buf, sizeof(Buf), "0x%016llx", static_cast<unsigned long long>(X));
	return Buf;
}

// ---------------------------------------------------------------------------------------------------------- statistics

/** Running mean and SD (Welford). */
struct Moments
{
	double N = 0, Mean = 0, M2 = 0;
	void Add(double X)
	{
		N += 1;
		const double D = X - Mean;
		Mean += D / N;
		M2 += D * (X - Mean);
	}
	double Var() const { return N > 1 ? M2 / (N - 1) : 0.0; }
	double Sd() const { return std::sqrt(Var()); }
	double Se() const { return N > 0 ? Sd() / std::sqrt(N) : 0.0; }
};

Moments Of(const std::vector<double>& X)
{
	Moments M;
	for (double V : X)
	{
		M.Add(V);
	}
	return M;
}

/** The paired difference A - B, element by element (the same seeds). */
Moments Paired(const std::vector<double>& A, const std::vector<double>& B)
{
	Moments M;
	const size_t N = std::min(A.size(), B.size());
	for (size_t I = 0; I < N; ++I)
	{
		M.Add(A[I] - B[I]);
	}
	return M;
}

std::vector<double> Diff(const std::vector<double>& A, const std::vector<double>& B)
{
	std::vector<double> X;
	const size_t N = std::min(A.size(), B.size());
	for (size_t I = 0; I < N; ++I)
	{
		X.push_back(A[I] - B[I]);
	}
	return X;
}

/** Block means of a per-hand series (the ring's SE is over blocks of 1,000 hands, §2.10). */
std::vector<double> Blocks(const std::vector<double>& X, size_t Size)
{
	std::vector<double> Out;
	for (size_t Start = 0; Start + Size <= X.size(); Start += Size)
	{
		double Sum = 0;
		for (size_t I = Start; I < Start + Size; ++I)
		{
			Sum += X[I];
		}
		Out.push_back(Sum / static_cast<double>(Size));
	}
	return Out;
}

/** "A beats B by at least Threshold": passes when Diff - Threshold >= 2 SE. Prints the line either way. */
bool Clears(const std::string& What, double DiffValue, double Se, double Threshold)
{
	const bool Ok = DiffValue - Threshold >= 2.0 * Se;
	std::printf("  %-72s %+9.3f  SE %7.3f  need >= %+8.3f  %s\n", What.c_str(), DiffValue, Se, Threshold + 2.0 * Se, Ok ? "ok" : "MISS");
	return Ok;
}

// ---------------------------------------------------------------------------------------------------------- the heroes

/** Who plays the hero's seat in a harness. */
struct HeroSpec
{
	enum class Kind
	{
		Style,     // style::Decide through a HeroBrain (the game's own path)
		Archetype, // an archetype's base profile through ss::Decide (S-8's baselines)
		CoachBest, // the grader's best option at every decision (session_main's DeepRuns rule)
		Accurate,  // the grader's best option with probability BestProb, else the second best (the 94% hero)
	};
	std::string Label;
	Kind K = Kind::Style;
	ss::style::Style S = ss::style::Style::Autopilot;
	ss::Archetype A = ss::Archetype::Tag;
	double Sharp = 1.0;
	double BestProb = 1.0;
};

HeroSpec StyleHero(ss::style::Style S, double Sharp = 1.0)
{
	HeroSpec H;
	H.K = HeroSpec::Kind::Style;
	H.S = S;
	H.Sharp = Sharp;
	H.Label = ss::style::InfoOf(S).Name;
	if (Sharp < 1.0)
	{
		H.Label += " @" + std::to_string(static_cast<int>(Sharp * 100.0 + 0.5));
	}
	return H;
}

HeroSpec ArchetypeHero(ss::Archetype A)
{
	HeroSpec H;
	H.K = HeroSpec::Kind::Archetype;
	H.A = A;
	H.Label = std::string("base ") + ss::ArchetypeName(A);
	return H;
}

HeroSpec CoachHero()
{
	HeroSpec H;
	H.K = HeroSpec::Kind::CoachBest;
	H.Label = "coach-best";
	return H;
}

HeroSpec AccurateHero(double BestProb)
{
	HeroSpec H;
	H.K = HeroSpec::Kind::Accurate;
	H.BestProb = BestProb;
	char Buf[64];
	std::snprintf(Buf, sizeof(Buf), "94%% hero (p %.2f)", BestProb);
	H.Label = Buf;
	return H;
}

/** A hero profile with no draw for its think time (what ProfileOf does for every style). */
ss::Profile QuietBase(ss::Archetype A)
{
	ss::Profile P = ss::profiles_detail::Base(A);
	P.Timing = ss::TimingStyle::Balanced;
	P.ThinkBase = 0.0;
	P.ThinkVar = 0.0;
	return P;
}

struct GraderPick
{
	ss::PlayerAction Action;
	double Accuracy = 100.0; // the chosen option's grade points (100 best .. 0 blunder)
};

/**
 * Smart pace's auto-fold (Session::ShouldAutoFold, Session.cpp): the GDD measured its coach-best and 94% heroes in the
 * session at Smart pace, where junk hands fold without a prompt, so the grader heroes fold those too and are graded only
 * on what the session would have asked them.
 */
bool SmartFolds(const ss::PlayerView& V)
{
	if (V.CurrentStreet != ss::Street::Preflop || V.CanCheck || V.Hole.size() != 2)
	{
		return false;
	}
	const double Price = static_cast<double>(V.ToCall) / static_cast<double>(std::max<ss::Chips>(1, V.Pot + V.ToCall));
	if (Price < 0.25)
	{
		return false;
	}
	const double Pct = ss::HandPercentile(V.Hole[0], V.Hole[1]);
	const ss::StreetSummary Sum = ss::PreflopSummary(V);
	const ss::PublicSeat& Me = V.Seats[static_cast<size_t>(V.Me)];
	const double StackBb = static_cast<double>(Me.Stack + Me.StreetBet) / static_cast<double>(std::max<ss::Chips>(1, V.BigBlind));
	if (StackBb <= 15.0 && Sum.Raises == 0)
	{
		return Pct > ss::PushRange(StackBb, ss::PlayersBehind(V, V.Me)) * 1.4;
	}
	if (Sum.Raises == 0)
	{
		static const double Open[9] = {0.55, 0.5, 1.0, 0.16, 0.18, 0.2, 0.24, 0.28, 0.36}; // BTN SB BB UTG UTG1 MP LJ HJ CO
		return Pct > Open[static_cast<int>(ss::PositionOf(V, V.Me))];
	}
	if (V.Me == V.BbIdx)
	{
		return Pct > 0.5;
	}
	return Pct > 0.22;
}

/** The grader's verdict on the hero's options: the best, or with probability 1 - BestProb the second best. */
GraderPick GraderChoice(const ss::PlayerView& V, double BestProb, ss::Rng& GradeRng, ss::Rng& Choice)
{
	const ss::DecisionAnalysis A = ss::AnalyzeDecision(V, GradeRng, 1200);
	GraderPick Out;
	if (A.Options.empty())
	{
		Out.Action = V.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Fold();
		return Out;
	}
	std::vector<size_t> Order(A.Options.size());
	for (size_t I = 0; I < Order.size(); ++I)
	{
		Order[I] = I;
	}
	std::stable_sort(Order.begin(), Order.end(), [&](size_t X, size_t Y) { return A.Options[X].Ev > A.Options[Y].Ev; });
	size_t Pick = Order[0];
	if (BestProb < 1.0 && Order.size() > 1 && Choice.Next() >= BestProb)
	{
		Pick = Order[1];
	}
	Out.Action = A.Options[Pick].Action;
	Out.Accuracy = ss::GradeScore(ss::GradeDecision(A, Out.Action).Result);
	return Out;
}

// ---------------------------------------------------------------------------------------------------------- the ring

/** One ring run: who the opponents are, how deep, how many hands. Every hero meets the same decks and opponents. */
struct RingConfig
{
	std::string Seed = "ring";
	std::string Pop = "micro"; // GetPopulation's name, or "crusher" for the 8-Crusher table
	double Depth = 100.0;      // big blinds, reset every hand
	int Hands = 1000;
	int Reseat = 150;            // a fresh table of strangers every 150 hands (§2.10)
	bool WipeEveryOrbit = false; // the reads curve's control: the hero's stats wiped every 9 hands
	bool FullField = false;      // the opponents decide at full strength too (the game's hero table); else Fast (§2.10)
	bool Audit = false;          // collect GTO's river balance (assertion 14)
};

struct RingResult
{
	std::vector<double> Net;      // the hero's net per hand, in big blinds
	std::vector<double> Adjusted; // all-in adjusted (EquityKnownHands at the all-in)
	std::vector<int> Seen;        // hands into the current table (the reads curve's bucket)
	int Illegal = 0;
	int Decisions = 0;
	double Ms = 0;
	// GTO's river balance (assertion 14): bluff shares by size, and the continue share against MDF, by spot.
	Moments Bluff75, Bluff100, ContinueGap;
};

constexpr ss::Chips RingBb = 100;

std::vector<ss::Profile> RingTable(const RingConfig& C, int Block)
{
	ss::Rng R(C.Seed + ":table:" + C.Pop + ":" + std::to_string(Block));
	std::vector<ss::Profile> Out;
	for (int K = 0; K < 8; ++K)
	{
		const ss::Archetype A = C.Pop == "crusher" ? ss::Archetype::Crusher : ss::RollArchetype(ss::GetPopulation(C.Pop), R);
		Out.push_back(ss::MakeProfile(A, R));
	}
	return Out;
}

/** The hero's net, all-in adjusted: where every remaining chip went in before the river, its equity share of each pot. */
double RingAdjusted(const ss::Hand& H, int HeroIdx, const std::string& Key)
{
	const ss::HandSeat& Hero = H.Seats[static_cast<size_t>(HeroIdx)];
	const double Net = static_cast<double>(Hero.Stack - Hero.StartStack);
	if (H.CurrentStreet != ss::Street::Showdown || H.History.empty() || Hero.Folded)
	{
		return Net;
	}
	const ss::Street Last = H.History.back().OnStreet;
	const size_t Known = Last == ss::Street::Preflop ? 0 : Last == ss::Street::Flop ? 3 : Last == ss::Street::Turn ? 4 : 5;
	if (Known >= 5)
	{
		return Net;
	}
	const std::vector<ss::Card> Board(H.Board.begin(), H.Board.begin() + static_cast<long long>(Known));
	ss::Rng R("ring-ev:" + Key);
	double Ev = 0;
	for (const ss::PotResult& Pot : H.BuildPots())
	{
		if (std::find(Pot.Eligible.begin(), Pot.Eligible.end(), Hero.Seat) == Pot.Eligible.end())
		{
			continue;
		}
		std::vector<std::vector<ss::Card>> Hands;
		size_t Mine = 0;
		for (int SeatNo : Pot.Eligible)
		{
			const ss::HandSeat* S = H.SeatByNumber(SeatNo);
			Mine = SeatNo == Hero.Seat ? Hands.size() : Mine;
			Hands.push_back(S->Hole);
		}
		if (Hands.size() < 2)
		{
			Ev += static_cast<double>(Pot.Amount);
			continue;
		}
		const std::vector<double> Share = ss::EquityKnownHands(Hands, Board, R, 1500);
		Ev += Share[Mine] * static_cast<double>(Pot.Amount);
	}
	return Ev - static_cast<double>(Hero.Committed);
}

ss::style::HeroContext RingContext(const ss::Hand& H, int Idx, int HandNo, uint64_t MixSeed)
{
	ss::style::HeroContext C;
	C.View = ss::MakeView(H, Idx);
	C.T = nullptr;
	C.HeroIdx = Idx;
	C.MixSeed = MixSeed;
	C.Tick = HandNo;
	return C;
}

/** The hero's decision in a ring hand (Tick = the hand number: the styles' Mix key). */
ss::BotDecision RingHeroDecision(const HeroSpec& Hero, const ss::style::HeroContext& C, const ss::reads::Table& Seen, ss::Rng& R, ss::Rng& GradeRng,
	ss::Rng& Choice)
{
	switch (Hero.K)
	{
	case HeroSpec::Kind::Archetype:
	{
		const ss::Profile P = QuietBase(Hero.A);
		ss::BotContext Ctx;
		Ctx.Prof = &P;
		Ctx.R = &R;
		return ss::Decide(C.View, Ctx);
	}
	case HeroSpec::Kind::CoachBest:
	case HeroSpec::Kind::Accurate:
	{
		ss::BotDecision D;
		D.Action = SmartFolds(C.View) ? ss::PlayerAction::Fold() : GraderChoice(C.View, Hero.K == HeroSpec::Kind::CoachBest ? 1.0 : Hero.BestProb, GradeRng, Choice).Action;
		return D;
	}
	default:
	{
		ss::style::HeroBrain B;
		B.S = Hero.S;
		B.Sharp = Hero.Sharp;
		B.Seen = &Seen;
		B.Where = 0;
		return ss::style::Decide(C, B, R);
	}
	}
}

/** One table's worth of hands (Reseat of them): the blocks are independent, so the ring runs them in parallel. */
void RingBlock(const RingConfig& C, const HeroSpec& Hero, int Block, RingResult& Out)
{
	const uint64_t MixSeed = Fnv64(C.Seed);
	ss::reads::Table Seen;
	const std::vector<ss::Profile> Table = RingTable(C, Block);
	ss::Rng Unused("ring-unused");
	const int First = Block * C.Reseat;
	const int Last = std::min(C.Hands, First + C.Reseat);
	for (int HandNo = First; HandNo < Last; ++HandNo)
	{
		if (C.WipeEveryOrbit && HandNo % 9 == 0)
		{
			Seen.clear();
		}
		ss::HandConfig Cfg;
		for (int Seat = 0; Seat < 9; ++Seat)
		{
			ss::SeatInput In;
			In.Seat = Seat;
			In.Id = Seat == 0 ? std::string(ss::HeroId) : "b" + std::to_string(Block) + "s" + std::to_string(Seat);
			In.Stack = static_cast<ss::Chips>(C.Depth * static_cast<double>(RingBb));
			Cfg.Players.push_back(In);
		}
		Cfg.ButtonSeat = HandNo % 9;
		Cfg.SmallBlind = RingBb / 2;
		Cfg.BigBlind = RingBb;
		Cfg.Ante = RingBb;
		ss::Rng DeckRng(C.Seed + ":deck:" + std::to_string(HandNo));
		Cfg.Deck = ss::FullDeck();
		DeckRng.Shuffle(Cfg.Deck);
		ss::Hand H(Cfg, Unused);
		std::vector<ss::Rng> Forks;
		for (int Seat = 0; Seat < 9; ++Seat)
		{
			Forks.emplace_back(C.Seed + ":d:" + std::to_string(HandNo) + ":" + std::to_string(Seat));
		}
		ss::Rng GradeRng(C.Seed + ":grade:" + std::to_string(HandNo));
		ss::Rng Choice(C.Seed + ":choice:" + std::to_string(HandNo));
		int Guard = 0;
		while (!H.bComplete && ++Guard < 400)
		{
			const int Idx = H.ToAct;
			const int Seat = H.Seats[static_cast<size_t>(Idx)].Seat;
			ss::BotDecision D;
			if (Seat == 0)
			{
				++Out.Decisions;
				const ss::style::HeroContext Ctx = RingContext(H, Idx, HandNo, MixSeed);
				if (C.Audit && Ctx.View.CurrentStreet == ss::Street::River)
				{
					const ss::style::RangeAudit A = ss::style::AuditGtoRiver(Ctx);
					if (A.Valid && A.Faced <= 0.0)
					{
						const double Want75 = 0.75 / 2.5;
						const double Want100 = 1.0 / 3.0;
						if (A.Bet75 > 0)
						{
							Out.Bluff75.Add(A.Bluff75 - Want75);
						}
						if (A.Bet100 > 0)
						{
							Out.Bluff100.Add(A.Bluff100 - Want100);
						}
					}
					else if (A.Valid)
					{
						Out.ContinueGap.Add(A.Continue - 1.0 / (1.0 + A.Faced));
					}
				}
				D = RingHeroDecision(Hero, Ctx, Seen, Forks[0], GradeRng, Choice);
			}
			else
			{
				ss::BotContext Ctx;
				Ctx.Prof = &Table[static_cast<size_t>(Seat - 1)];
				Ctx.Fast = !C.FullField;
				Ctx.R = &Forks[static_cast<size_t>(Seat)];
				D = ss::Decide(ss::MakeView(H, Idx), Ctx);
			}
			if (!H.Act(D.Action))
			{
				++Out.Illegal;
				const ss::LegalActions L = H.GetLegalActions();
				H.Act(L.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Fold());
			}
		}
		const std::string Key = C.Seed + ":" + std::to_string(HandNo);
		Out.Net[static_cast<size_t>(HandNo)] = static_cast<double>(H.Seats[0].Stack - H.Seats[0].StartStack) / static_cast<double>(RingBb);
		Out.Adjusted[static_cast<size_t>(HandNo)] = RingAdjusted(H, 0, Key) / static_cast<double>(RingBb);
		Out.Seen[static_cast<size_t>(HandNo)] = HandNo % C.Reseat;
		ss::reads::Count(H, 0, Seen);
	}
}

RingResult RunRing(const RingConfig& C, const HeroSpec& Hero)
{
	RingResult Out;
	Out.Net.assign(static_cast<size_t>(C.Hands), 0.0);
	Out.Adjusted.assign(static_cast<size_t>(C.Hands), 0.0);
	Out.Seen.assign(static_cast<size_t>(C.Hands), 0);
	const int BlockCount = (C.Hands + C.Reseat - 1) / C.Reseat;
	std::vector<RingResult> Parts(static_cast<size_t>(BlockCount));
	const double Start = NowMs();
	ParallelFor(BlockCount, [&](int B) {
		RingResult& P = Parts[static_cast<size_t>(B)];
		P.Net.assign(static_cast<size_t>(C.Hands), 0.0);
		P.Adjusted.assign(static_cast<size_t>(C.Hands), 0.0);
		P.Seen.assign(static_cast<size_t>(C.Hands), 0);
		RingBlock(C, Hero, B, P);
	});
	for (int B = 0; B < BlockCount; ++B)
	{
		const RingResult& P = Parts[static_cast<size_t>(B)];
		const int First = B * C.Reseat;
		const int Last = std::min(C.Hands, First + C.Reseat);
		for (int I = First; I < Last; ++I)
		{
			Out.Net[static_cast<size_t>(I)] = P.Net[static_cast<size_t>(I)];
			Out.Adjusted[static_cast<size_t>(I)] = P.Adjusted[static_cast<size_t>(I)];
			Out.Seen[static_cast<size_t>(I)] = P.Seen[static_cast<size_t>(I)];
		}
		Out.Illegal += P.Illegal;
		Out.Decisions += P.Decisions;
		auto Merge = [](Moments& Into, const Moments& From) {
			if (From.N <= 0)
			{
				return;
			}
			const double N = Into.N + From.N;
			const double D = From.Mean - Into.Mean;
			Into.M2 += From.M2 + D * D * Into.N * From.N / N;
			Into.Mean += D * From.N / N;
			Into.N = N;
		};
		Merge(Out.Bluff75, P.Bluff75);
		Merge(Out.Bluff100, P.Bluff100);
		Merge(Out.ContinueGap, P.ContinueGap);
	}
	Out.Ms = NowMs() - Start;
	return Out;
}

/** Hands in a reads bucket (hands into the table): [Lo, Hi). */
std::vector<double> Bucket(const RingResult& R, const std::vector<double>& Series, int Lo, int Hi)
{
	std::vector<double> Out;
	for (size_t I = 0; I < Series.size(); ++I)
	{
		if (R.Seen[I] >= Lo && R.Seen[I] < Hi)
		{
			Out.push_back(Series[I]);
		}
	}
	return Out;
}

/** bb/100 with its SE over blocks of 1,000 hands (SE of a per-hand mean x 100). */
struct Rate
{
	double Bb100 = 0, Se = 0;
};

Rate RateOf(const std::vector<double>& PerHand)
{
	Rate R;
	const Moments M = Of(PerHand);
	R.Bb100 = M.Mean * 100.0;
	const std::vector<double> B = Blocks(PerHand, 1000);
	R.Se = B.size() >= 10 ? Of(B).Se() * 100.0 : M.Se() * 100.0;
	return R;
}

// ---------------------------------------------------------------------------------------------------------- tournaments

/** One tournament, played to the hero's finish with the hero's seat taken by Hero. */
struct EventResult
{
	int Place = 0;
	int Entrants = 0;
	double Pct = 0;    // the GDD's yardstick: 100 * (Entrants - Place) / (Entrants - 1); 50 is average
	double Roi = 0;    // (prize - buy-in) / buy-in
	bool Itm = false;
	bool Won = false;
	bool Seat = false; // satellites: a seat won
	int Hands = 0;
	int Decisions = 0;
	double AccuracySum = 0; // the grader heroes: grade points of their own choices
	int Graded = 0;          // and how many they were (Smart pace's auto-folds aren't graded)
	int BigStackSpots = 0, BigStackJams = 0; // satellites: first-in decisions as the covering stack near the end, and jams
	int Illegal = 0;
	uint64_t Hash = 0; // every tick's checkpoint, hashed (determinism)
	double Ms = 0;
};

/** How the harness plays an event: the hero's policy at its turns, the bots by BotDecisionFor, the tick order kept. */
EventResult PlayEvent(const ss::TournamentSpec& Spec, const std::string& Seed, const HeroSpec& Hero, bool FastField, const std::vector<ss::Chips>* Payouts,
	bool Satellite, bool HashTicks = false)
{
	EventResult Out;
	const double Start = NowMs();
	ss::Tournament T(Spec, "Hero", Seed);
	if (Payouts)
	{
		T.Payouts = *Payouts;
	}
	T.FastField = FastField;
	ss::reads::Table Seen;
	ss::style::HeroBrain Brain;
	Brain.S = Hero.S;
	Brain.Sharp = Hero.Sharp;
	Brain.Seen = &Seen;
	const ss::Profile Arch = QuietBase(Hero.A);
	ss::Rng GradeRng(Seed + ":grade");
	ss::Rng Choice(Seed + ":choice");
	int Seats = 0;
	for (const ss::Chips P : T.Payouts)
	{
		Seats += Satellite && P == Spec.SeatValueCents ? 1 : 0;
	}
	Out.Hash = Fnv64(Seed);
	int Guard = 0;
	while (!T.bFinished && !T.Hero().Busted && ++Guard < 20000)
	{
		if (Satellite && T.Remaining <= Seats)
		{
			Out.Seat = true;
			break;
		}
		std::vector<ss::TEvent> Events;
		std::unique_ptr<ss::Hand> H = T.StartTick(Events);
		if (H)
		{
			++Out.Hands;
			int Steps = 0;
			while (!H->bComplete && ++Steps < 1000)
			{
				const int Idx = H->ToAct;
				const bool IsHero = H->Seats[static_cast<size_t>(Idx)].Id == ss::HeroId;
				ss::BotDecision D;
				if (IsHero && !T.HeroSitsOut)
				{
					++Out.Decisions;
					const ss::PlayerView V = ss::MakeView(*H, Idx);
					// The big stack's first-in decisions in the satellite's last three times the seats (assertion 8).
					bool BigStackSpot = false;
					if (Satellite && V.CurrentStreet == ss::Street::Preflop && ss::PreflopSummary(V).Raises == 0 && T.Remaining <= 3 * Seats)
					{
						const ss::PublicSeat& MeSeat = V.Seats[static_cast<size_t>(V.Me)];
						BigStackSpot = true;
						for (const ss::PublicSeat& S : V.Seats)
						{
							BigStackSpot = BigStackSpot && (S.Seat == MeSeat.Seat || S.Stack + S.StreetBet < MeSeat.Stack + MeSeat.StreetBet);
						}
						Out.BigStackSpots += BigStackSpot ? 1 : 0;
					}
					switch (Hero.K)
					{
					case HeroSpec::Kind::Archetype: D = T.BotDecisionFor(*H, false, &Arch); break;
					case HeroSpec::Kind::CoachBest:
					case HeroSpec::Kind::Accurate:
					{
						if (SmartFolds(V))
						{
							D.Action = ss::PlayerAction::Fold();
							break;
						}
						const GraderPick G = GraderChoice(V, Hero.K == HeroSpec::Kind::CoachBest ? 1.0 : Hero.BestProb, GradeRng, Choice);
						D.Action = G.Action;
						Out.AccuracySum += G.Accuracy;
						++Out.Graded;
						break;
					}
					default: D = T.HeroDecisionFor(*H, Brain); break;
					}
					if (BigStackSpot)
					{
						const bool Jam = D.Action.Type == ss::PlayerAction::Kind::Raise && D.Action.To >= static_cast<double>(V.MaxRaiseTo) * 0.8;
						Out.BigStackJams += Jam ? 1 : 0;
					}
					if (!H->Act(D.Action))
					{
						++Out.Illegal;
						const ss::LegalActions L = H->GetLegalActions();
						H->Act(L.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Fold());
					}
					continue;
				}
				if (IsHero)
				{
					const ss::LegalActions L = H->GetLegalActions();
					H->Act(L.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Fold());
					continue;
				}
				D = T.BotDecisionFor(*H, FastField);
				if (!H->Act(D.Action))
				{
					const ss::LegalActions L = H->GetLegalActions();
					H->Act(L.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Fold());
				}
			}
			int HeroIdx = 0;
			for (size_t I = 0; I < H->Seats.size(); ++I)
			{
				HeroIdx = H->Seats[I].Id == ss::HeroId ? static_cast<int>(I) : HeroIdx;
			}
			ss::reads::Count(*H, HeroIdx, Seen);
		}
		T.FinishTick(H.get(), nullptr);
		if (HashTicks)
		{
			Out.Hash = Fnv64(T.Checkpoint(), Out.Hash);
		}
	}
	const ss::TPlayer& Me = T.Hero();
	Out.Entrants = Spec.Entrants;
	Out.Place = Out.Seat ? std::max(1, T.HeroRank()) : Me.Place > 0 ? Me.Place : std::max(1, T.HeroRank());
	Out.Pct = 100.0 * static_cast<double>(Out.Entrants - Out.Place) / static_cast<double>(std::max(1, Out.Entrants - 1));
	const ss::Chips Prize = Out.Seat ? Spec.SeatValueCents : Me.PrizeCents;
	Out.Roi = static_cast<double>(Prize - Spec.BuyInCents) / static_cast<double>(std::max<ss::Chips>(1, Spec.BuyInCents));
	Out.Itm = Prize > 0;
	Out.Won = Out.Place == 1 && !Satellite;
	Out.Hash = Fnv64(std::to_string(Out.Place) + ":" + std::to_string(Out.Hands), Out.Hash);
	Out.Ms = NowMs() - Start;
	return Out;
}

/** A 9-player sit-and-go: 50 big blinds, 5-minute levels, 50/30/20 (the satellite: three equal seats). */
ss::TournamentSpec SngSpec(const std::string& Pop, bool Satellite)
{
	ss::TournamentSpec S;
	S.Id = Satellite ? "sat9" : "sng9";
	S.Name = Satellite ? "Step satellite" : "Sit and go";
	S.BuyInCents = 1100;
	S.FeeCents = 100;
	S.Entrants = 9;
	S.StartingStack = 5000;
	S.LevelMinutes = 5.0;
	S.SecondsPerHand = 42.0;
	S.Population = Pop;
	S.Speed = "Turbo";
	S.TableSize = 9;
	if (Satellite)
	{
		S.SeatValueCents = 3000; // 9 x $10 = $90: three $30 seats
	}
	return S;
}

std::vector<ss::Chips> SngPayouts()
{
	return {4500, 2700, 1800};
}

/** The MTTs of §2.5: RiverLine's $0.25 Hyper Sprint (180), the Night Owl Turbo (1,000) and a 700-runner deep event. */
ss::TournamentSpec HyperSpec()
{
	return ss::Lobby()[1].Spec;
}

ss::TournamentSpec TurboSpec()
{
	return ss::Lobby()[0].Spec;
}

ss::TournamentSpec DeepSpec()
{
	// The $3.30 Daily Grind's structure (Network.cpp: 30,000 chips, 8-minute levels, 8-handed, blinds from 100/200).
	ss::TournamentSpec S;
	S.Id = "grind-330";
	S.Name = "$3.30 Daily Grind";
	S.BuyInCents = 330;
	S.FeeCents = 30;
	S.GuaranteeCents = 200000;
	S.Entrants = 700;
	S.StartingStack = 30000;
	S.LevelMinutes = 8.0;
	S.SecondsPerHand = 42.0;
	S.Population = "micro";
	S.Speed = "Regular";
	S.TableSize = 8;
	for (ss::Level L : ss::StandardLevels())
	{
		L.Sb *= 2;
		L.Bb *= 2;
		L.Ante *= 2;
		S.Levels.push_back(L);
	}
	return S;
}

struct EventSet
{
	std::vector<EventResult> R;
	std::vector<double> Pct() const
	{
		std::vector<double> X;
		for (const EventResult& E : R)
		{
			X.push_back(E.Pct);
		}
		return X;
	}
	std::vector<double> Roi() const
	{
		std::vector<double> X;
		for (const EventResult& E : R)
		{
			X.push_back(E.Roi * 100.0);
		}
		return X;
	}
	std::vector<double> Flag(bool EventResult::*F) const
	{
		std::vector<double> X;
		for (const EventResult& E : R)
		{
			X.push_back(E.*F ? 100.0 : 0.0);
		}
		return X;
	}
	double Ms() const
	{
		double S = 0;
		for (const EventResult& E : R)
		{
			S += E.Ms;
		}
		return S;
	}
	int Hands() const
	{
		int S = 0;
		for (const EventResult& E : R)
		{
			S += E.Hands;
		}
		return S;
	}
	int Illegal() const
	{
		int S = 0;
		for (const EventResult& E : R)
		{
			S += E.Illegal;
		}
		return S;
	}
	double Accuracy() const
	{
		double S = 0;
		int N = 0;
		for (const EventResult& E : R)
		{
			S += E.AccuracySum;
			N += E.Graded;
		}
		return N > 0 ? S / N : 0.0;
	}
};

EventSet RunEvents(const ss::TournamentSpec& Spec, const std::string& SeedBase, int Count, const HeroSpec& Hero, bool FastField, bool SngPay, bool Satellite)
{
	EventSet Out;
	Out.R.resize(static_cast<size_t>(Count));
	const std::vector<ss::Chips> Pay = SngPayouts();
	ParallelFor(Count, [&](int I) {
		Out.R[static_cast<size_t>(I)] = PlayEvent(Spec, SeedBase + ":" + std::to_string(I), Hero, FastField, SngPay ? &Pay : nullptr, Satellite);
	});
	return Out;
}

// ---------------------------------------------------------------------------------------------------------- random spots

/**
 * Random spots for legality and determinism: random tables (2 to 9 players, 1 to 250 big blinds, an ante or not), the other
 * seats acting at random (legal folds, calls and raises of any size, all in), the hero deciding by Hero at every turn.
 * Returns how many hero decisions were made and how many were illegal; Hash folds every hero action in.
 */
struct SpotRun
{
	int Decisions = 0;
	int Illegal = 0;
	uint64_t Hash = 14695981039346656037ull;
};

SpotRun RandomSpots(ss::style::Style S, int Want, const std::string& Seed)
{
	SpotRun Out;
	ss::reads::Table Seen;
	ss::style::HeroBrain B;
	B.S = S;
	B.Sharp = 0.9;
	B.Seen = &Seen;
	const uint64_t MixSeed = Fnv64(Seed);
	for (int Hand = 0; Out.Decisions < Want && Hand < Want * 4; ++Hand)
	{
		ss::Rng G(Seed + ":" + std::to_string(Hand));
		const int Players = 2 + G.Int(8);
		ss::HandConfig Cfg;
		for (int Seat = 0; Seat < Players; ++Seat)
		{
			ss::SeatInput In;
			In.Seat = Seat;
			In.Id = Seat == 0 ? std::string(ss::HeroId) : "r" + std::to_string(Seat);
			const double Bb = G.Chance(0.3) ? 1.0 + G.Next() * 14.0 : G.Chance(0.5) ? 15.0 + G.Next() * 30.0 : 45.0 + G.Next() * 205.0;
			In.Stack = static_cast<ss::Chips>(Bb * 100.0);
			Cfg.Players.push_back(In);
		}
		Cfg.ButtonSeat = G.Int(Players);
		Cfg.SmallBlind = 50;
		Cfg.BigBlind = 100;
		Cfg.Ante = G.Chance(0.6) ? 100 : 0;
		ss::Hand H(Cfg, G);
		ss::Rng R(Seed + ":r:" + std::to_string(Hand));
		int Guard = 0;
		while (!H.bComplete && ++Guard < 300)
		{
			const int Idx = H.ToAct;
			const ss::LegalActions L = H.GetLegalActions();
			if (H.Seats[static_cast<size_t>(Idx)].Id == ss::HeroId)
			{
				ss::style::HeroContext C;
				C.View = ss::MakeView(H, Idx);
				C.HeroIdx = Idx;
				C.MixSeed = MixSeed;
				C.Tick = Hand;
				const ss::BotDecision D = ss::style::Decide(C, B, R);
				++Out.Decisions;
				Out.Hash = Fnv64(std::to_string(static_cast<int>(D.Action.Type)) + ":" + std::to_string(static_cast<long long>(D.Action.To)) + ";", Out.Hash);
				if (!H.Act(D.Action))
				{
					++Out.Illegal;
					H.Act(L.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Fold());
				}
				continue;
			}
			const double Roll = G.Next();
			ss::PlayerAction A = L.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Fold();
			if (Roll < 0.45)
			{
				A = L.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Call();
			}
			else if (Roll < 0.75 && L.CanRaise)
			{
				const double Lo = static_cast<double>(L.MinRaiseTo);
				const double Hi = static_cast<double>(L.MaxRaiseTo);
				A = ss::PlayerAction::RaiseTo(G.Chance(0.2) ? Hi : Lo + G.Next() * (Hi - Lo) * 0.5);
			}
			if (!H.Act(A))
			{
				H.Act(L.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Fold());
			}
		}
		ss::reads::Count(H, 0, Seen);
	}
	return Out;
}

// ---------------------------------------------------------------------------------------------------------- CI checks

std::string ReadText(const std::string& Path, bool& Ok)
{
	std::string Out;
	FILE* F = std::fopen(Path.c_str(), "rb");
	Ok = F != nullptr;
	if (!F)
	{
		return Out;
	}
	char Buf[65536];
	size_t N = 0;
	while ((N = std::fread(Buf, 1, sizeof(Buf), F)) > 0)
	{
		Out.append(Buf, N);
	}
	std::fclose(F);
	Out.erase(std::remove(Out.begin(), Out.end(), '\r'), Out.end());
	return Out;
}

/** The chart tables match their texts, and the anchors hold (§2.4.2). */
void CiCharts()
{
	// The checksum chartgen compiled in, against the texts now.
	uint64_t H = 14695981039346656037ull;
	bool All = true;
	for (const char* Name : {"gto_preflop.txt", "nash_push.txt", "nash_call.txt", "nash_reshove.txt"})
	{
		bool Ok = false;
		const std::string Text = ReadText(std::string(SS_TESTS_DIR) + "/charts/" + Name, Ok);
		All = All && Ok;
		H = Fnv64(std::string(Name) + "\n" + Text, H);
	}
	Expect(All, "charts: the four chart texts are in Tests/charts");
	Expect(H == ss::charts::Checksum(), "charts: ChartTables.cpp was generated from Tests/charts as they are (" + Hex64(H) + " vs " + Hex64(ss::charts::Checksum()) + ")");
	const int AA = ss::HandClass(ss::MakeCard(12, 0), ss::MakeCard(12, 1));
	const int SevenTwo = ss::HandClass(ss::MakeCard(5, 0), ss::MakeCard(0, 1));
	bool AaJams = true;
	for (int Pos = 0; Pos <= 7; ++Pos)
	{
		for (double E = 1.0; E <= 20.0; E += 0.5)
		{
			AaJams = AaJams && ss::charts::Jam(Pos, E, 0.0, AA) && ss::charts::Jam(Pos, E, 1.0, AA);
		}
	}
	Expect(AaJams, "charts: AA jams from every seat at every depth");
	Expect(!ss::charts::Jam(0, 15.0, 0.0, SevenTwo) && !ss::charts::Jam(0, 15.0, 1.0, SevenTwo), "charts: 72o folds UTG at 15 BB");
	// The button at 10 BB: 40% or more with the big-blind ante every RiverLine structure has. With no ante the first-caller
	// Nash is about a third (32%; published charts, which carry antes, read 37% to 45%), so the 40% anchor is ante-on.
	int Combos = 0, NoAnte = 0;
	for (int Cls = 0; Cls < 169; ++Cls)
	{
		Combos += ss::charts::Jam(6, 10.0, 1.0, Cls) ? ss::HandClassCombos(Cls) : 0;
		NoAnte += ss::charts::Jam(6, 10.0, 0.0, Cls) ? ss::HandClassCombos(Cls) : 0;
	}
	std::printf("  charts: the button jams %d%% at 10 BB with a big-blind ante, %d%% with none\n", Combos * 100 / 1326, NoAnte * 100 / 1326);
	Expect(Combos >= 0.40 * 1326 && NoAnte >= 0.30 * 1326, "charts: the button jams 40% or more at 10 BB (big-blind ante), 30% or more with none");
	// The antes widen the jams; the call ranges tighten as the jam gets deeper.
	int Wider = 0, Tighter = 0;
	for (int Cls = 0; Cls < 169; ++Cls)
	{
		Wider += ss::charts::Jam(3, 12.0, 1.0, Cls) && !ss::charts::Jam(3, 12.0, 0.0, Cls) ? 1 : 0;
		Wider -= !ss::charts::Jam(3, 12.0, 1.0, Cls) && ss::charts::Jam(3, 12.0, 0.0, Cls) ? 1 : 0;
		Tighter += ss::charts::CallJam(6, 8, 6.0, 1.0, Cls, false) ? 1 : 0;
		Tighter -= ss::charts::CallJam(6, 8, 18.0, 1.0, Cls, false) ? 1 : 0;
	}
	Expect(Wider > 0 && Tighter > 0, "charts: an ante widens the jams, a deeper jam is called tighter");
	// GTO's charts: the button opens wider than UTG, the big blind defends the button widest, 4-bets AA, folds 72o.
	const ss::charts::Mixed Utg = ss::charts::Preflop(ss::charts::Spot::Rfi, 0, 0, 100.0, SevenTwo);
	const ss::charts::Mixed Aa4 = ss::charts::Preflop(ss::charts::Spot::Vs3Bet, 6, 7, 100.0, AA);
	double Btn = 0, UtgShare = 0, BbVsBtn = 0, BbVsUtg = 0;
	for (int Cls = 0; Cls < 169; ++Cls)
	{
		const double N = ss::HandClassCombos(Cls) / 1326.0;
		Btn += ss::charts::Preflop(ss::charts::Spot::Rfi, 6, 0, 100.0, Cls).Raise * N;
		UtgShare += ss::charts::Preflop(ss::charts::Spot::Rfi, 0, 0, 100.0, Cls).Raise * N;
		const ss::charts::Mixed A = ss::charts::Preflop(ss::charts::Spot::BbVsOpen, 8, 6, 100.0, Cls);
		const ss::charts::Mixed B = ss::charts::Preflop(ss::charts::Spot::BbVsOpen, 8, 0, 100.0, Cls);
		BbVsBtn += (A.Raise + A.Call) * N;
		BbVsUtg += (B.Raise + B.Call) * N;
	}
	std::printf("  charts: RFI UTG %.1f%%, BTN %.1f%%; BB defends %.1f%% vs BTN, %.1f%% vs UTG\n", UtgShare * 100, Btn * 100, BbVsBtn * 100, BbVsUtg * 100);
	Expect(Utg.Raise == 0.0 && Aa4.Raise == 1.0, "charts: GTO folds 72o UTG and 4-bets AA");
	Expect(std::fabs(UtgShare - 0.14) < 0.015 && std::fabs(Btn - 0.47) < 0.015, "charts: GTO's raise-first-in shares are the chart's (UTG 14%, BTN 47%)");
	Expect(std::fabs(BbVsBtn - 0.70) < 0.02 && std::fabs(BbVsUtg - 0.38) < 0.02, "charts: the big blind continues 70% against the button, 38% against UTG");
}

/** ProfileOf: the §2.4.1 table, drawn from nothing; Autopilot's numbers are today's Sprint's. */
void CiProfiles()
{
	using ss::style::Style;
	struct Row
	{
		Style S;
		double Open, Limp, Three, Call, Aggr, Bluff, Sticky, Cbet, Size, Tilt, Push, Icm;
	};
	const Row Table[] = {
		{Style::Autopilot, 1.00, .02, .040, 1.35, .60, .12, .35, .65, 1.00, .35, .65, .45},
		{Style::Tag, 1.00, .02, .040, 1.00, .75, .18, .20, .65, 1.00, .35, .85, .60},
		{Style::Lag, 1.45, .02, .075, 1.30, .85, .32, .30, .75, 1.10, .45, .80, .40},
		{Style::Nit, 0.60, .05, .015, 0.60, .60, .04, .05, .55, 1.00, .30, .60, .80},
		{Style::Maniac, 3.20, .05, .200, 2.20, .95, .50, .45, .90, 1.45, .70, .25, .05},
		{Style::Reg, 1.10, .01, .050, 1.05, .80, .24, .22, .62, 1.00, .25, .95, .85},
		{Style::Gto, 1.20, 0, .060, 1.10, .82, .28, .24, .58, 1.00, .10, 1.00, 1.00},
		{Style::Exploit, 1.00, .02, .040, 1.00, .75, .18, .20, .65, 1.00, .35, .85, .60},
		{Style::PushFold, 0.90, .02, .040, 0.90, .75, .12, .20, .55, 1.00, .35, 1.00, .60},
		{Style::IcmShark, 1.10, .01, .050, 1.05, .80, .24, .22, .62, 1.00, .25, .95, 1.00},
		{Style::BountyHunter, 1.45, .02, .075, 1.30, .85, .26, .30, .75, 1.10, .45, .80, .50},
	};
	ss::Rng Probe("profiles-draw-nothing");
	uint32_t Before[4], After[4];
	Probe.GetState(Before);
	bool Match = true;
	for (const Row& R : Table)
	{
		const ss::Profile P = ss::style::ProfileOf(R.S);
		const bool Ok = P.OpenWidth == R.Open && P.LimpRate == R.Limp && P.ThreeBet == R.Three && P.CallWidth == R.Call && P.Aggression == R.Aggr &&
			P.Bluff == R.Bluff && P.Stickiness == R.Sticky && P.Cbet == R.Cbet && P.Sizing == R.Size && P.TiltProne == R.Tilt && P.PushFold == R.Push &&
			P.IcmAware == R.Icm && P.ThinkBase == 0.0 && P.ThinkVar == 0.0 && P.Timing == ss::TimingStyle::Balanced;
		if (!Ok)
		{
			std::printf("  ProfileOf(%s) doesn't match §2.4.1\n", ss::style::InfoOf(R.S).Id);
		}
		Match = Match && Ok;
	}
	Probe.GetState(After);
	Expect(Match, "ProfileOf: every style's numbers are §2.4.1's, with no think draw");
	Expect(std::memcmp(Before, After, sizeof(Before)) == 0, "ProfileOf draws nothing");
	const ss::Profile Sprint = ss::SprintProfile();
	const ss::Profile Auto = ss::style::ProfileOf(Style::Autopilot);
	Expect(Auto.OpenWidth == Sprint.OpenWidth && Auto.LimpRate == Sprint.LimpRate && Auto.ThreeBet == Sprint.ThreeBet && Auto.Cbet == Sprint.Cbet &&
			   Auto.Sizing == Sprint.Sizing && Auto.TiltProne == Sprint.TiltProne && Auto.Timing == Sprint.Timing && Auto.ThinkBase == Sprint.ThinkBase,
		"Autopilot is Sprint's profile with the session's nerf");
	// The catalog: append-only ids, every tile's copy, the ship rule's flag on Bounty Hunter (P1) only.
	Expect(static_cast<int>(ss::style::Catalog().size()) == static_cast<int>(Style::Count), "the catalog has every style");
	ss::style::Style Back = Style::Autopilot;
	bool Ids = true;
	for (const ss::style::Info& I : ss::style::Catalog())
	{
		Ids = Ids && ss::style::FromId(I.Id, Back) && Back == I.S && &ss::style::InfoOf(I.S) == &I;
		Ids = Ids && (I.Hidden == (I.S == Style::BountyHunter));
	}
	Expect(Ids, "the catalog's ids round-trip; only Bounty Hunter (P1) is hidden");
	Expect(ss::style::Unavailable(Style::PushFold, 2, false) == "Plays as TAG here" && ss::style::Unavailable(Style::IcmShark, 2, false) == "Plays as Balanced Reg here" &&
			   ss::style::Unavailable(Style::BountyHunter, 0, false) == "Knockout events only" && ss::style::Unavailable(Style::BountyHunter, 2, true) == "Plays as LAG here" &&
			   ss::style::Unavailable(Style::Gto, 2, false).empty() && ss::style::Unavailable(Style::PushFold, 1, false).empty(),
		"greyed tiles say why (§2.3, §2.6.3)");
	// The think tempos of §2.4.7, in catalog order (presentation only, but the watch strip's pace is the spec's).
	const double Tempo[] = {1.5, 1.8, 1.4, 2.0, 0.9, 2.0, 2.4, 2.2, 1.2, 2.6, 1.6};
	bool Tempos = static_cast<int>(sizeof(Tempo) / sizeof(Tempo[0])) == static_cast<int>(Style::Count);
	for (int S = 0; Tempos && S < static_cast<int>(Style::Count); ++S)
	{
		Tempos = ss::style::InfoOf(static_cast<Style>(S)).Tempo == Tempo[S];
	}
	Expect(Tempos, "every style thinks at §2.4.7's tempo");
}

/** Autopilot under autoplay is today's Sprint exactly (U3: it's the bar): the same actions and the same draws. */
void CiAutopilot()
{
	bool Same = true;
	int Spots = 0;
	for (int Hand = 0; Hand < 400; ++Hand)
	{
		ss::Rng G("autopilot-parity:" + std::to_string(Hand));
		ss::HandConfig Cfg;
		const int Players = 2 + G.Int(8);
		for (int Seat = 0; Seat < Players; ++Seat)
		{
			ss::SeatInput In;
			In.Seat = Seat;
			In.Id = "s" + std::to_string(Seat);
			In.Stack = 300 + G.Int(20000);
			Cfg.Players.push_back(In);
		}
		Cfg.ButtonSeat = 0;
		Cfg.SmallBlind = 50;
		Cfg.BigBlind = 100;
		Cfg.Ante = 100;
		ss::Hand H(Cfg, G);
		const ss::Profile P = ss::style::ProfileOf(ss::style::Style::Autopilot);
		int Guard = 0;
		while (!H.bComplete && ++Guard < 200)
		{
			const double Pressure = G.Next();
			ss::Rng A("ap:" + std::to_string(Hand) + ":" + std::to_string(Guard));
			ss::Rng B = A;
			ss::style::HeroContext C;
			C.View = ss::MakeView(H, H.ToAct);
			C.Pressure = Pressure;
			C.HeroIdx = H.ToAct;
			ss::style::HeroBrain Brain;
			Brain.S = ss::style::Style::Autopilot;
			Brain.Sharp = 0.5; // Autopilot never slips, whatever the Sharpness
			Brain.Tilt = 0.9;  // and never tilts
			const ss::BotDecision X = ss::style::Decide(C, Brain, A);
			ss::BotContext Ctx;
			Ctx.Prof = &P;
			Ctx.Tilt = 0.0;
			Ctx.IcmPressure = Pressure;
			Ctx.R = &B;
			const ss::BotDecision Y = ss::Decide(C.View, Ctx);
			uint32_t Sa[4], Sb[4];
			A.GetState(Sa);
			B.GetState(Sb);
			Same = Same && X.Action.Type == Y.Action.Type && X.Action.To == Y.Action.To && X.ThinkMs == Y.ThinkMs && std::memcmp(Sa, Sb, sizeof(Sa)) == 0;
			++Spots;
			H.Act(X.Action);
		}
	}
	Expect(Same && Spots > 1000, "Autopilot is ss::Decide with Sprint's profile, tilt 0 and the bots' pressure, draw for draw (" + std::to_string(Spots) + " spots)");
	// The engine path too: SimulateTickWith(Autopilot) is SimulateTick(&Autopilot), checkpoint for checkpoint.
	ss::TournamentSpec Spec = HyperSpec();
	ss::Tournament A(Spec, "Hero", "autopilot-ticks");
	ss::Tournament B(Spec, "Hero", "autopilot-ticks");
	ss::style::HeroBrain Brain;
	const ss::Profile P = ss::style::ProfileOf(ss::style::Style::Autopilot);
	bool Ticks = true;
	for (int K = 0; K < 60 && !A.bFinished; ++K)
	{
		A.SimulateTickWith(Brain);
		B.SimulateTick(&P);
		Ticks = Ticks && A.Checkpoint() == B.Checkpoint();
	}
	Expect(Ticks, "SimulateTickWith(Autopilot) equals SimulateTick(&Autopilot) through 60 ticks of a hyper");
}

/** reads::Count on scripted hands; the estimates; the fold into notes; the text form. */
void CiReads()
{
	// A 4-handed hand: UTG opens, CO 3-bets, BTN folds, SB folds, BB calls, UTG folds to the 3-bet; on the flop the
	// 3-bettor (the preflop raiser) bets and the BB folds. The hero is the button.
	ss::HandConfig Cfg;
	const char* Ids[5] = {"utg", "co", "hero", "sb", "bb"};
	for (int Seat = 0; Seat < 5; ++Seat)
	{
		ss::SeatInput In;
		In.Seat = Seat;
		In.Id = Ids[Seat];
		In.Stack = 10000;
		Cfg.Players.push_back(In);
	}
	Cfg.ButtonSeat = 2;
	Cfg.SmallBlind = 50;
	Cfg.BigBlind = 100;
	ss::Rng R("reads-hand");
	ss::Hand H(Cfg, R);
	// Seats: utg 0 acts first (after the bb at 4).
	auto Act = [&](const ss::PlayerAction& A) { return H.Act(A); };
	bool Ok = Act(ss::PlayerAction::RaiseTo(250));  // utg opens
	Ok = Ok && Act(ss::PlayerAction::RaiseTo(800)); // co 3-bets
	Ok = Ok && Act(ss::PlayerAction::Fold());       // hero
	Ok = Ok && Act(ss::PlayerAction::Fold());       // sb
	Ok = Ok && Act(ss::PlayerAction::Call());       // bb calls the 3-bet
	Ok = Ok && Act(ss::PlayerAction::Fold());       // utg folds to the 3-bet
	Ok = Ok && Act(ss::PlayerAction::Check());      // bb checks the flop
	Ok = Ok && Act(ss::PlayerAction::RaiseTo(900)); // co c-bets
	Ok = Ok && Act(ss::PlayerAction::Fold());       // bb folds to the c-bet
	Expect(Ok && H.bComplete, "reads: the scripted hand plays");
	ss::reads::Table T;
	ss::reads::Count(H, 2, T);
	const ss::reads::Stats& U = T["utg"];
	const ss::reads::Stats& C = T["co"];
	const ss::reads::Stats& B = T["bb"];
	Expect(T.find("hero") == T.end(), "reads: the hero isn't counted");
	Expect(U.Hands == 1 && U.Vpip == 1 && U.Pfr == 1 && U.ThreeBetOpp == 0 && U.FoldTo3BetOpp == 1 && U.FoldTo3Bet == 1 && U.Flops == 0, "reads: the opener who folded to a 3-bet");
	Expect(C.Vpip == 1 && C.Pfr == 1 && C.ThreeBetOpp == 1 && C.ThreeBet == 1 && C.CbetOpp == 1 && C.Cbet == 1 && C.Flops == 1 && C.AggBets == 1 && C.Wtsd == 0,
		"reads: the 3-bettor's c-bet");
	Expect(B.Vpip == 1 && B.Pfr == 0 && B.ThreeBetOpp == 0 && B.ThreeBet == 0 && B.FoldToCbetOpp == 1 && B.FoldToCbet == 1 && B.Flops == 1 && B.AggCalls == 0,
		"reads: the big blind who folded to the c-bet");
	ss::reads::Count(H, 2, T);
	Expect(T["co"].Hands == 2 && T["co"].Cbet == 2, "reads: counts add up hand after hand");
	// The estimates: the prior at no confidence, a halved K with a learned tell, and the notes at their weight.
	const ss::reads::Read None = ss::reads::Of(nullptr, nullptr, 0);
	Expect(None.C == 0.0 && None.Vpip == 0.30 && None.Pfr == 0.15 && None.Af == 1.8, "reads: no reads is the population prior at no confidence");
	ss::reads::Stats Loose;
	Loose.Hands = 30;
	Loose.Vpip = 21;
	const ss::reads::Read L0 = ss::reads::Of(&Loose, nullptr, 0);
	const ss::reads::Read L1 = ss::reads::Of(&Loose, nullptr, 2);
	Expect(std::fabs(L0.Vpip - (21 + 0.3 * 30) / 60.0) < 1e-12 && std::fabs(L0.C - 0.5) < 1e-12 && L1.C > L0.C && L1.Vpip > L0.Vpip, "reads: shrunk to the prior; a learned tell halves K");
	ss::reads::Table Notes;
	ss::reads::Table Event;
	Event["p1"] = Loose;
	ss::reads::Fold(Notes, Event, 0.5);
	Expect(Notes["p1"].Hands == 15 && Notes["p1"].Vpip == 11, "reads: an event folds into the notes at half weight");
	const ss::reads::Read Both = ss::reads::Of(&Loose, &Notes["p1"], 0);
	Expect(Both.C > L0.C, "reads: the notes add to tonight's counts");
	ss::reads::Stats Back;
	Expect(ss::reads::Parse(ss::reads::Write(C), Back) && ss::reads::Write(Back) == ss::reads::Write(C), "reads: the 15 counts round-trip as text");
	Expect(!ss::reads::Parse("1\t2\t3", Back), "reads: a short line doesn't parse");
}

/** Mix and ThinkSeconds (§2.4.7); the slip itself is CiSlipAndReads'. */
void CiMixAndSlip()
{
	bool Range = true;
	double Sum = 0;
	for (int I = 0; I < 20000; ++I)
	{
		const double X = ss::style::Mix(12345, I, I % 9, I % 7, 99);
		Range = Range && X >= 0.0 && X < 1.0;
		Sum += X;
	}
	Expect(Range && std::fabs(Sum / 20000 - 0.5) < 0.01, "Mix is uniform in [0, 1)");
	Expect(ss::style::Mix(1, 2, 3, 4, 5) == ss::style::Mix(1, 2, 3, 4, 5) && ss::style::Mix(1, 2, 3, 4, 5) != ss::style::Mix(1, 2, 3, 4, 6), "Mix is a pure function of its keys");
	ss::style::HeroBrain B;
	B.S = ss::style::Style::Gto;
	ss::style::HeroContext C;
	C.View.Seats.resize(2);
	C.View.Seats[0].Stack = 10000;
	C.View.Me = 0;
	B.Where = 1;
	Expect(ss::style::ThinkSeconds(B, C, 1) >= 1.0 && ss::style::ThinkSeconds(B, C, 8) >= 0.15, "think time floors: 1 s at 1x live, 0.15 s otherwise");
	B.Where = 0;
	double Fast = 0, Slow = 0;
	for (int I = 0; I < 200; ++I)
	{
		C.Tick = I;
		Fast += ss::style::ThinkSeconds(B, C, 8);
		Slow += ss::style::ThinkSeconds(B, C, 1);
	}
	Expect(Slow > Fast * 4.0, "think time scales with the watch speed");
}

/** A decision worth a slip (§2.5), as style::Decide reads it: anything after the flop, a pot over 10 big blinds, facing a
 *  raise, or an unopened hand the plan would play (the top half) or a stack short enough to shove. */
bool SlipSpot(const ss::PlayerView& V)
{
	if (V.CurrentStreet != ss::Street::Preflop)
	{
		return true;
	}
	const double Bb = std::max(1.0, static_cast<double>(V.BigBlind));
	if (static_cast<double>(V.Pot) > 10.0 * Bb || ss::PreflopSummary(V).Raises > 0)
	{
		return true;
	}
	const ss::PublicSeat& Me = V.Seats[static_cast<size_t>(V.Me)];
	const double StackBb = static_cast<double>(Me.Stack + Me.StreetBet) / Bb;
	return V.Hole.size() == 2 && (ss::HandPercentile(V.Hole[0], V.Hole[1]) <= 0.5 || StackBb <= 15.0);
}

/** §2.5's slip profile: pushed toward the losing station direction. */
ss::Profile SlipProfile(const ss::Profile& P)
{
	ss::Profile S = P;
	S.Aggression = P.Aggression * 0.5;
	S.Stickiness = std::min(0.95, P.Stickiness + 0.30);
	S.CallWidth = P.CallWidth * 1.4;
	S.Bluff = P.Bluff * 0.5;
	S.PushFold = P.PushFold * 0.5;
	S.IcmAware = P.IcmAware * 0.3;
	return S;
}

/** What one random-spot run saw: decisions, the non-trivial ones, the slips among them, and any that played otherwise. */
struct SlipRun
{
	int Decisions = 0, NonTrivial = 0, Slips = 0, Mismatch = 0, Differs = 0;
};

/**
 * Random spots (RandomSpots' tables and opponents) where every hero decision of style S goes through style::Decide and is
 * checked, draw for draw, against ss::Decide with the profile the spec says it plays (Expect): the style's own, or the
 * slipped one when the decision is non-trivial and Mix's slip draw lands under k x (1 - Sharpness), at the tilt the style
 * feels (Tilt x TiltProne / 0.35, clamped) and the bots' pressure. Seen: the observed stats Exploitative reads (never
 * counted here, so they stay as given). Differs counts the decisions that don't match Expect (Exploitative's reads).
 */
SlipRun SlipSpots(ss::style::Style S, double Sharp, double Tilt, int Want, const std::string& Seed, const ss::reads::Table* Seen, ss::style::Style Expect)
{
	SlipRun Out;
	ss::style::HeroBrain B;
	B.S = S;
	B.Sharp = Sharp;
	B.Tilt = Tilt;
	B.Seen = Seen;
	const uint64_t MixSeed = Fnv64(Seed);
	const ss::Profile Base = ss::style::ProfileOf(Expect);
	const double Feels = std::min(1.0, std::max(0.0, Tilt * Base.TiltProne / 0.35));
	for (int Hand = 0; Out.Decisions < Want && Hand < Want * 4; ++Hand)
	{
		ss::Rng G(Seed + ":" + std::to_string(Hand));
		const int Players = 2 + G.Int(8);
		ss::HandConfig Cfg;
		for (int Seat = 0; Seat < Players; ++Seat)
		{
			ss::SeatInput In;
			In.Seat = Seat;
			In.Id = Seat == 0 ? std::string(ss::HeroId) : "r" + std::to_string(Seat);
			const double Bb = G.Chance(0.3) ? 1.0 + G.Next() * 14.0 : G.Chance(0.5) ? 15.0 + G.Next() * 30.0 : 45.0 + G.Next() * 205.0;
			In.Stack = static_cast<ss::Chips>(Bb * 100.0);
			Cfg.Players.push_back(In);
		}
		Cfg.ButtonSeat = G.Int(Players);
		Cfg.SmallBlind = 50;
		Cfg.BigBlind = 100;
		Cfg.Ante = G.Chance(0.6) ? 100 : 0;
		const double Pressure = G.Chance(0.5) ? G.Next() : 0.0;
		ss::Hand H(Cfg, G);
		int Guard = 0;
		while (!H.bComplete && ++Guard < 300)
		{
			const int Idx = H.ToAct;
			const ss::LegalActions L = H.GetLegalActions();
			if (H.Seats[static_cast<size_t>(Idx)].Id == ss::HeroId)
			{
				ss::style::HeroContext C;
				C.View = ss::MakeView(H, Idx);
				C.HeroIdx = Idx;
				C.MixSeed = MixSeed;
				C.Tick = Hand;
				C.Pressure = Pressure;
				ss::Rng Ra(Seed + ":r:" + std::to_string(Hand) + ":" + std::to_string(Guard));
				ss::Rng Rb = Ra;
				const ss::BotDecision X = ss::style::Decide(C, B, Ra);
				const bool NonTrivial = SlipSpot(C.View);
				const bool Slip = NonTrivial &&
					ss::style::Mix(MixSeed, Hand, Idx, static_cast<int>(C.View.History.size()), 0x736c6970u) < 1.2 * (1.0 - std::max(0.0, std::min(1.0, Sharp)));
				const ss::Profile P = Slip ? SlipProfile(Base) : Base;
				ss::BotContext Ctx;
				Ctx.Prof = &P;
				Ctx.Tilt = Feels;
				Ctx.IcmPressure = Pressure;
				Ctx.R = &Rb;
				const ss::BotDecision Y = ss::Decide(C.View, Ctx);
				uint32_t Sa[4], Sb[4];
				Ra.GetState(Sa);
				Rb.GetState(Sb);
				const bool Same = X.Action.Type == Y.Action.Type && X.Action.To == Y.Action.To && std::memcmp(Sa, Sb, sizeof(Sa)) == 0;
				++Out.Decisions;
				Out.NonTrivial += NonTrivial ? 1 : 0;
				Out.Slips += Slip ? 1 : 0;
				Out.Mismatch += Same ? 0 : 1;
				Out.Differs += X.Action.Type != Y.Action.Type || X.Action.To != Y.Action.To ? 1 : 0;
				if (!H.Act(X.Action))
				{
					H.Act(L.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Fold());
				}
				continue;
			}
			const double Roll = G.Next();
			ss::PlayerAction A = L.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Fold();
			if (Roll < 0.45)
			{
				A = L.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Call();
			}
			else if (Roll < 0.75 && L.CanRaise)
			{
				const double Lo = static_cast<double>(L.MinRaiseTo);
				const double Hi = static_cast<double>(L.MaxRaiseTo);
				A = ss::PlayerAction::RaiseTo(G.Chance(0.2) ? Hi : Lo + G.Next() * (Hi - Lo) * 0.5);
			}
			if (!H.Act(A))
			{
				H.Act(L.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Fold());
			}
		}
	}
	return Out;
}

/**
 * The slip and the tilt inside style::Decide (§2.5), decision by decision: the profile styles (TAG, LAG, Nit, Maniac,
 * Balanced Reg) play ss::Decide with their own profile, or with the slipped one at the non-trivial decisions Mix picks
 * (k x (1 - Sharpness): 60% at 0.5), at the tilt each feels; never a slip at Sharpness 1.0. And Exploitative is plain TAG
 * until it has reads (§2.4.3: the tile's promise), and plays otherwise once it has some.
 */
void CiSlipAndReads()
{
	using ss::style::Style;
	const std::vector<Style> Plain = {Style::Tag, Style::Lag, Style::Nit, Style::Maniac, Style::Reg};
	std::vector<SlipRun> Half(Plain.size()), Full(Plain.size());
	ss::reads::Table Empty, Stations;
	for (int Seat = 1; Seat < 9; ++Seat)
	{
		ss::reads::Stats& S = Stations["r" + std::to_string(Seat)];
		S.Hands = 300;
		S.Vpip = 180; // 60%, and calls far more than it bets: a station (and a loose fish to isolate)
		S.Pfr = 15;
		S.Flops = 200;
		S.Wtsd = 120;
		S.AggBets = 10;
		S.AggCalls = 150;
		S.FoldToCbetOpp = 60;
		S.FoldToCbet = 12;
	}
	SlipRun NoReads, WithReads;
	ParallelFor(static_cast<int>(Plain.size()) + 2, [&](int K) {
		if (K < static_cast<int>(Plain.size()))
		{
			const std::string Id = ss::style::InfoOf(Plain[static_cast<size_t>(K)]).Id;
			Half[static_cast<size_t>(K)] = SlipSpots(Plain[static_cast<size_t>(K)], 0.5, 0.5, 900, "slip:" + Id, nullptr, Plain[static_cast<size_t>(K)]);
			Full[static_cast<size_t>(K)] = SlipSpots(Plain[static_cast<size_t>(K)], 1.0, 0.0, 300, "slip-full:" + Id, nullptr, Plain[static_cast<size_t>(K)]);
		}
		else if (K == static_cast<int>(Plain.size()))
		{
			NoReads = SlipSpots(Style::Exploit, 1.0, 0.0, 900, "exploit-reads", &Empty, Style::Tag);
		}
		else
		{
			WithReads = SlipSpots(Style::Exploit, 1.0, 0.0, 900, "exploit-reads", &Stations, Style::Tag);
		}
	});
	for (size_t K = 0; K < Plain.size(); ++K)
	{
		const SlipRun& H = Half[K];
		const double Share = static_cast<double>(H.Slips) / std::max(1, H.NonTrivial);
		std::printf("  slip: %-13s at 0.5, %d of %d non-trivial decisions slipped (%.1f%%), %d trivial\n", ss::style::InfoOf(Plain[K]).Name, H.Slips, H.NonTrivial, 100.0 * Share,
			H.Decisions - H.NonTrivial);
		Expect(H.Decisions >= 900 && H.Mismatch == 0, std::string("slip: ") + ss::style::InfoOf(Plain[K]).Name +
														   " at Sharpness 0.5 plays its profile, or the slipped one where the slip lands, at the tilt it feels (" +
														   std::to_string(H.Mismatch) + " off)");
		Expect(std::fabs(Share - 0.6) < 0.05 && H.Decisions > H.NonTrivial, std::string("slip: ") + ss::style::InfoOf(Plain[K]).Name +
																			   " slips at k x (1 - Sharpness) of its non-trivial decisions, never a trivial one");
		Expect(Full[K].Decisions >= 300 && Full[K].Mismatch == 0 && Full[K].Slips == 0,
			std::string("slip: ") + ss::style::InfoOf(Plain[K]).Name + " never slips at Sharpness 1.0");
	}
	std::printf("  Exploitative against TAG: %d of %d decisions differ with no reads, %d of %d against eight read stations\n", NoReads.Differs, NoReads.Decisions,
		WithReads.Differs, WithReads.Decisions);
	// At a cash table (Dee's) each style plays as §2.3 says: Push/Fold as TAG, ICM Shark as Balanced Reg, the rest as
	// themselves, with no tournament, no bubble and the hand number as the mix key.
	bool Cash = true;
	int CashSpots = 0;
	for (int Hand = 0; Hand < 120; ++Hand)
	{
		ss::Rng G("cash:" + std::to_string(Hand));
		ss::HandConfig Cfg;
		const int Players = 2 + G.Int(6);
		for (int Seat = 0; Seat < Players; ++Seat)
		{
			ss::SeatInput In;
			In.Seat = Seat;
			In.Id = Seat == 0 ? std::string(ss::HeroId) : "c" + std::to_string(Seat);
			In.Stack = 4000 + G.Int(16000);
			Cfg.Players.push_back(In);
		}
		Cfg.ButtonSeat = G.Int(Players);
		Cfg.SmallBlind = 50;
		Cfg.BigBlind = 100;
		ss::Hand H(Cfg, G);
		int Guard = 0;
		while (!H.bComplete && ++Guard < 200)
		{
			const ss::LegalActions L = H.GetLegalActions();
			if (H.Seats[static_cast<size_t>(H.ToAct)].Id != ss::HeroId)
			{
				H.Act(G.Chance(0.5) ? (L.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Call()) : L.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Fold());
				continue;
			}
			const ss::PlayerView V = ss::MakeView(H, H.ToAct);
			ss::BotDecision Played;
			for (const auto& Pair : std::vector<std::pair<Style, Style>>{{Style::PushFold, Style::Tag}, {Style::IcmShark, Style::Reg}, {Style::Gto, Style::Gto}})
			{
				ss::style::HeroBrain Pick;
				Pick.S = Pair.first;
				Pick.Sharp = 0.9;
				Pick.Where = 2;
				ss::style::HeroBrain As = Pick;
				As.S = Pair.second;
				ss::style::HeroContext C;
				C.View = V;
				C.HeroIdx = V.Me;
				C.MixSeed = 99;
				C.Tick = Hand;
				ss::Rng Ra("cash-r:" + std::to_string(Hand) + ":" + std::to_string(Guard));
				ss::Rng Rb = Ra;
				const ss::BotDecision X = ss::style::DecideCash(V, Pick, Ra, 99, Hand);
				const ss::BotDecision Y = ss::style::Decide(C, As, Rb);
				uint32_t Sa[4], Sb[4];
				Ra.GetState(Sa);
				Rb.GetState(Sb);
				Cash = Cash && X.Action.Type == Y.Action.Type && X.Action.To == Y.Action.To && std::memcmp(Sa, Sb, sizeof(Sa)) == 0;
				Played = X;
			}
			++CashSpots;
			if (!H.Act(Played.Action))
			{
				H.Act(L.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Fold());
			}
		}
	}
	Expect(Cash && CashSpots >= 100, "at a cash table Push/Fold plays as TAG, ICM Shark as Balanced Reg, GTO as itself (" + std::to_string(CashSpots) + " spots)");
	Expect(NoReads.Decisions >= 900 && NoReads.Mismatch == 0, "Exploitative with no reads is plain TAG, draw for draw (§2.4.3)");
	Expect(WithReads.Differs >= 20, "Exploitative plays differently once it has reads (a table of stations)");
}

/**
 * Exploitative's pre-pass (§2.4.3), on heads-up flops built for it: against a read station (VPIP >= 0.40, aggression
 * <= 1.2, c >= 0.3) with no bet to face, it bets half the pot with HeuristicEquity in [0.60 - 0.06c, 0.60); facing a read
 * nit's bet (130 into 200: a price of 0.28), it folds under the pot odds + 0.08c. The villain is "v", read over 300 hands.
 */
void CiExploitPrePass()
{
	ss::reads::Table Station, Nit;
	ss::reads::Stats& S = Station["v"];
	S.Hands = 300;
	S.Vpip = 180;
	S.Pfr = 15;
	S.Flops = 200;
	S.AggBets = 10;
	S.AggCalls = 150;
	ss::reads::Stats& N = Nit["v"];
	N.Hands = 300;
	N.Vpip = 24;
	N.Pfr = 20;
	N.Flops = 20;
	N.AggBets = 12;
	N.AggCalls = 6;
	const double C = 300.0 / 330.0; // the confidence 300 hands give (K = 30)
	int ThinSpots = 0, ThinBets = 0, ThinTagDiffers = 0, FoldSpots = 0, Folds = 0, FoldTagDiffers = 0;
	for (int Deal = 0; Deal < 4000 && (ThinSpots < 12 || FoldSpots < 12); ++Deal)
	{
		const bool Thin = Deal % 2 == 0; // the hero in the big blind (thin value), else on the button facing a bet (the nit)
		ss::HandConfig Cfg;
		for (int Seat = 0; Seat < 2; ++Seat)
		{
			ss::SeatInput In;
			In.Seat = Seat;
			In.Id = Seat == 0 ? std::string(ss::HeroId) : "v";
			In.Stack = 20000;
			Cfg.Players.push_back(In);
		}
		Cfg.ButtonSeat = Thin ? 1 : 0;
		Cfg.SmallBlind = 50;
		Cfg.BigBlind = 100;
		ss::Rng G("exploit-prepass:" + std::to_string(Deal));
		ss::Hand H(Cfg, G);
		bool Ok = true;
		if (Thin)
		{
			Ok = H.Act(ss::PlayerAction::RaiseTo(300)) && H.Act(ss::PlayerAction::Call()); // the villain opens, the hero calls
		}
		else
		{
			Ok = H.Act(ss::PlayerAction::Call()) && H.Act(ss::PlayerAction::Check()) && H.Act(ss::PlayerAction::RaiseTo(130)); // a lead of 130 into 200
		}
		if (!Ok || H.bComplete || H.CurrentStreet != ss::Street::Flop || H.Seats[static_cast<size_t>(H.ToAct)].Id != ss::HeroId)
		{
			continue;
		}
		ss::style::HeroContext Ctx;
		Ctx.View = ss::MakeView(H, H.ToAct);
		Ctx.HeroIdx = H.ToAct;
		Ctx.MixSeed = 7;
		Ctx.Tick = Deal;
		const double He = ss::HeuristicEquity(Ctx.View.Hole, Ctx.View.Board, 1);
		const double Odds = static_cast<double>(Ctx.View.ToCall) / static_cast<double>(Ctx.View.Pot + Ctx.View.ToCall);
		ss::style::HeroBrain B;
		B.S = ss::style::Style::Exploit;
		B.Sharp = 1.0;
		ss::style::HeroBrain Tag = B;
		Tag.S = ss::style::Style::Tag;
		ss::Rng R1("exploit-prepass-r:" + std::to_string(Deal));
		ss::Rng R2 = R1;
		if (Thin && He >= 0.60 - 0.06 * C + 0.003 && He < 0.597)
		{
			B.Seen = &Station;
			const ss::BotDecision D = ss::style::Decide(Ctx, B, R1);
			const ss::BotDecision T = ss::style::Decide(Ctx, Tag, R2);
			++ThinSpots;
			ThinBets += D.Action.Type == ss::PlayerAction::Kind::Raise && D.Action.To == 0.5 * static_cast<double>(Ctx.View.Pot) ? 1 : 0;
			ThinTagDiffers += D.Action.Type != T.Action.Type || D.Action.To != T.Action.To ? 1 : 0;
		}
		else if (!Thin && He < Odds + 0.08 * C - 0.003 && He > Odds + 0.003)
		{
			B.Seen = &Nit;
			const ss::BotDecision D = ss::style::Decide(Ctx, B, R1);
			const ss::BotDecision T = ss::style::Decide(Ctx, Tag, R2);
			++FoldSpots;
			Folds += D.Action.Type == ss::PlayerAction::Kind::Fold ? 1 : 0;
			FoldTagDiffers += D.Action.Type != T.Action.Type ? 1 : 0;
		}
	}
	std::printf("  Exploitative's pre-pass: %d of %d thin half-pot bets into a station (TAG plays %d of them otherwise); %d of %d folds to a nit's lead (TAG %d)\n",
		ThinBets, ThinSpots, ThinTagDiffers, Folds, FoldSpots, FoldTagDiffers);
	Expect(ThinSpots >= 12 && ThinBets == ThinSpots && ThinTagDiffers > 0, "Exploitative bets thin value, half the pot, into a read station (§2.4.3)");
	Expect(FoldSpots >= 12 && Folds == FoldSpots && FoldTagDiffers > 0, "Exploitative folds under the pot odds + 0.08c to a read nit's bet (§2.4.3)");
}

/** ICM Shark's arithmetic: RequiredEquity, the bubble factor's shape, the zone (§2.4.5). */
void CiIcm()
{
	Expect(std::fabs(ss::icmplay::RequiredEquity(1.0, 1000, 3000) - 0.25) < 1e-12 && std::fabs(ss::icmplay::RequiredEquity(2.0, 1000, 3000) - 0.4) < 1e-12,
		"RequiredEquity = BF x Risk / (BF x Risk + Pot)");
	// A satellite with four left and three seats: the covered short stack's factor is large, the chip leader's call cheap.
	ss::TournamentSpec S = SngSpec("micro", true);
	ss::Tournament T(S, "Hero", "icm-check");
	int Left = 0;
	for (ss::TPlayer& P : T.Players)
	{
		const bool Keep = Left < 4;
		P.Busted = !Keep;
		P.Stack = Keep ? (Left == 0 ? 20000 : Left == 1 ? 15000 : Left == 2 ? 8000 : 2000) : 0;
		Left += Keep ? 1 : 0;
	}
	T.Remaining = 4;
	const double Short = ss::icmplay::BubbleFactor(T, 3, 0);
	const double Big = ss::icmplay::BubbleFactor(T, 0, 3);
	const double Mid = ss::icmplay::BubbleFactor(T, 2, 0);
	std::printf("  ICM: satellite bubble, 3 seats among 4: BF short-vs-leader %.2f, leader-vs-short %.2f, middle-vs-leader %.2f\n", Short, Big, Mid);
	Expect(ss::icmplay::InZone(T), "a satellite's last three times the seats is ICM Shark's zone");
	Expect(Mid > 1.5 && Mid > Big, "a middle stack risks its seat against the leader (factor > 1.5), the leader much less");
	// A regular payout ladder far from the money is chip EV: no zone, a factor near 1.
	ss::Tournament Far(HyperSpec(), "Hero", "icm-far");
	Expect(!ss::icmplay::InZone(Far), "180 left of 180 is far from the zone");
	const double Early = ss::icmplay::BubbleFactor(Far, 0, 1);
	Expect(Early > 0.95 && Early < 1.25, "a factor near 1 with the money far away (" + std::to_string(Early) + ")");
	// The cost (§2.4.5: <= 0.3 ms a stack-deciding decision). A first-in jam reads two factors for each player behind (at
	// most 16); a call reads one.
	const double Start = NowMs();
	double Sink = 0;
	for (int K = 0; K < 160; ++K)
	{
		Sink += ss::icmplay::BubbleFactor(T, K % 4, (K + 1) % 4) + ss::icmplay::BubbleFactor(Far, K % 9, (K + 3) % 9);
	}
	const double PerJam = (NowMs() - Start) / 160.0 * 8.0; // 16 factors, half of each kind
	std::printf("  ICM: a jam's factors cost %.3f ms (exact and six-player model mixed)%s\n", PerJam, Sink > 0 ? "" : " ");
	// The budget is 0.3 ms; the check allows twice that, since a loaded machine (ctest -j3) slows any timing.
	Expect(PerJam <= 0.6, "ICM Shark's factors for a jam stay near their 0.3 ms budget");
	// A blind posted all in is no bet to call off (§2.4.5's stack-deciding spots are between two players): first in on an
	// SNG's bubble, with the big blind all in on the blind, ICM Shark plays the jam rule with aces, it doesn't limp them.
	ss::Tournament B(SngSpec("micro", false), "Hero", "icm-blind-all-in");
	B.Payouts = SngPayouts();
	const ss::Chips Stacks[4] = {20000, 15000, 100, 1500}; // the button, the small blind, the big blind (all in), me
	int Alive = 0;
	for (ss::TPlayer& P : B.Players)
	{
		P.Busted = Alive >= 4;
		P.Stack = Alive < 4 ? Stacks[Alive] : 0;
		++Alive;
	}
	B.Remaining = 4;
	ss::HandConfig Cfg;
	for (int Seat = 0; Seat < 4; ++Seat)
	{
		ss::SeatInput In;
		In.Seat = Seat;
		In.Id = B.Players[static_cast<size_t>(Seat)].Id;
		In.Stack = Stacks[Seat];
		Cfg.Players.push_back(In);
	}
	Cfg.ButtonSeat = 0;
	Cfg.SmallBlind = 50;
	Cfg.BigBlind = 100;
	Cfg.Deck = ss::FullDeck();
	// Dealt one at a time from the small blind: my two cards are the third and the seventh.
	const ss::Card Aces[2] = {ss::MakeCard(12, 0), ss::MakeCard(12, 1)};
	for (int K = 0; K < 2; ++K)
	{
		const auto It = std::find(Cfg.Deck.begin(), Cfg.Deck.end(), Aces[K]);
		std::iter_swap(It, Cfg.Deck.begin() + (K == 0 ? 2 : 6));
	}
	ss::Rng Unused("icm-blind-all-in");
	ss::Hand H(Cfg, Unused);
	ss::style::HeroContext Ctx;
	Ctx.View = ss::MakeView(H, H.ToAct);
	Ctx.T = &B;
	Ctx.HeroIdx = H.ToAct;
	ss::style::HeroBrain Brain;
	Brain.S = ss::style::Style::IcmShark;
	Brain.Sharp = 1.0;
	ss::Rng R("icm-blind-all-in:r");
	const ss::BotDecision D = ss::style::Decide(Ctx, Brain, R);
	const bool Setup = H.ToAct == 3 && Ctx.View.Seats[2].AllIn && ss::HandClass(Ctx.View.Hole[0], Ctx.View.Hole[1]) == ss::HandClass(Aces[0], Aces[1]) &&
		ss::icmplay::InZone(B);
	Expect(Setup && D.Action.Type == ss::PlayerAction::Kind::Raise && D.Action.To == static_cast<double>(Ctx.View.MaxRaiseTo),
		"ICM Shark jams aces first in on the bubble when the big blind is all in on the blind (no limp)");
}

/** Every style's actions are legal in 10,000 random spots, and the same spots give the same actions (style_vectors.txt). */
void CiLegality(std::map<std::string, std::string>& Vectors)
{
	const std::vector<ss::style::Info>& All = ss::style::Catalog();
	std::vector<SpotRun> Runs(All.size());
	std::vector<double> Ms(All.size(), 0.0);
	ParallelFor(static_cast<int>(All.size()), [&](int K) {
		const double Start = NowMs();
		Runs[static_cast<size_t>(K)] = RandomSpots(All[static_cast<size_t>(K)].S, 10000, std::string("legal:") + All[static_cast<size_t>(K)].Id);
		Ms[static_cast<size_t>(K)] = NowMs() - Start;
	});
	for (size_t K = 0; K < All.size(); ++K)
	{
		const SpotRun& R = Runs[K];
		Expect(R.Illegal == 0 && R.Decisions >= 10000, std::string("legal: ") + All[K].Name + " in 10,000 random spots (" + std::to_string(R.Illegal) + " illegal)");
		Vectors[std::string("spots ") + All[K].Id] = Hex64(R.Hash);
		std::printf("  %-14s %d spots, %d illegal, %.0f ms\n", All[K].Name, R.Decisions, R.Illegal, Ms[K]);
	}
}

/** A smoke test: 20 sit-and-gos and 10 satellites a style finish with legal actions; their hashes (style_vectors.txt). */
void CiSmoke(std::map<std::string, std::string>& Vectors)
{
	const std::vector<ss::style::Info>& All = ss::style::Catalog();
	struct Smoke
	{
		uint64_t Sng = 14695981039346656037ull, Ring = 14695981039346656037ull;
		int Illegal = 0, RingIllegal = 0;
	};
	std::vector<Smoke> Out(All.size());
	ParallelFor(static_cast<int>(All.size()), [&](int K) {
		const ss::style::Info& I = All[static_cast<size_t>(K)];
		Smoke& S = Out[static_cast<size_t>(K)];
		const HeroSpec H = StyleHero(I.S, 0.9);
		const std::vector<ss::Chips> Pay = SngPayouts();
		for (int E = 0; E < 30; ++E)
		{
			const bool Sat = E >= 20;
			const EventResult R = PlayEvent(SngSpec("micro", Sat), std::string("smoke:") + I.Id + ":" + std::to_string(E), H, true, Sat ? nullptr : &Pay, Sat, E < 3);
			S.Sng = Fnv64(Hex64(R.Hash), S.Sng);
			S.Illegal += R.Illegal;
		}
		// A short ring run's nets, one table after another on this thread.
		RingConfig C;
		C.Seed = std::string("vec:") + I.Id;
		C.Hands = 270;
		RingResult R;
		R.Net.assign(270, 0.0);
		R.Adjusted.assign(270, 0.0);
		R.Seen.assign(270, 0);
		for (int B = 0; B * C.Reseat < C.Hands; ++B)
		{
			RingBlock(C, H, B, R);
		}
		for (const double X : R.Net)
		{
			S.Ring = Fnv64(std::to_string(static_cast<long long>(std::llround(X * 100.0))) + ",", S.Ring);
		}
		S.RingIllegal = R.Illegal;
	});
	for (size_t K = 0; K < All.size(); ++K)
	{
		Expect(Out[K].Illegal == 0, std::string("smoke: ") + All[K].Name + " plays 20 sit-and-gos and 10 satellites legally");
		Expect(Out[K].RingIllegal == 0, std::string("smoke: ") + All[K].Name + " plays 270 ring hands legally");
		Vectors[std::string("sng ") + All[K].Id] = Hex64(Out[K].Sng);
		Vectors[std::string("ring ") + All[K].Id] = Hex64(Out[K].Ring);
	}
}

/** The copy lint over the styles' inventory: edge lines as sentences, names and labels as labels. */
void CiCopy()
{
	std::vector<std::pair<std::string, std::string>> Sentences;
	std::set<std::string> Edges;
	for (const ss::style::Info& I : ss::style::Catalog())
	{
		Sentences.emplace_back(std::string("edge.") + I.Id, I.Edge);
		Edges.insert(I.Edge);
	}
	Failures += copylint::Run("styles", Sentences, copylint::Pa);
	int Bad = 0;
	for (const std::string& Line : ss::style::Inventory())
	{
		if (Edges.count(Line))
		{
			continue;
		}
		const std::string Low = copylint::Lower(Line);
		const bool Ok = !Line.empty() && Line.find('{') == std::string::npos && Line.find("  ") == std::string::npos && copylint::Characters(Line) <= 64 &&
			Low.find("todo") == std::string::npos && Low.find("tbd") == std::string::npos && Low.find("placeholder") == std::string::npos &&
			Low.find("lorem") == std::string::npos && Line.back() != ' ';
		if (!Ok)
		{
			++Bad;
			std::printf("LABEL (styles) \"%s\"\n", Line.c_str());
		}
	}
	Expect(Bad == 0, "copy: every style name, best-in line, swing word and greyed reason is a clean label");
	// No real brand, and nothing that claims to be a real solver.
	bool Clean = true;
	for (const std::string& Line : ss::style::Inventory())
	{
		const std::string Low = copylint::Lower(Line);
		for (const char* Word : {"youtube", "pokerstars", "ggpoker", "piosolver", "gto wizard"})
		{
			Clean = Clean && Low.find(Word) == std::string::npos;
		}
	}
	Expect(Clean && std::string(ss::style::InfoOf(ss::style::Style::Gto).Edge).find("not a solver") != std::string::npos,
		"copy: no real brand; GTO calls itself an approximation, not a solver");
	Expect(copylint::SelfCheck() == 0, "copy: the lint itself");
}

/** style_vectors.txt: compare (or with --write, write) the determinism hashes. */
void CiVectors(const std::map<std::string, std::string>& Vectors, bool Write)
{
	const std::string Path = std::string(SS_TESTS_DIR) + "/style_vectors.txt";
	if (Write)
	{
		std::string Text = "# ai_styles' determinism hashes (update 2 SPEC §2.10), written by style_test --write. Review the diff: a change\n"
						   "# here means a style now plays differently on the same seeds.\n";
		for (const auto& Kv : Vectors)
		{
			Text += Kv.first + " " + Kv.second + "\n";
		}
		FILE* F = std::fopen(Path.c_str(), "wb");
		Expect(F != nullptr, "style_vectors.txt is writable");
		if (F)
		{
			std::fwrite(Text.data(), 1, Text.size(), F);
			std::fclose(F);
			std::printf("  wrote %s (%d vectors)\n", Path.c_str(), static_cast<int>(Vectors.size()));
		}
		return;
	}
	bool Ok = false;
	const std::string Text = ReadText(Path, Ok);
	Expect(Ok, "style_vectors.txt is there (style_test --write)");
	std::map<std::string, std::string> Want;
	size_t Start = 0;
	while (Start < Text.size())
	{
		size_t End = Text.find('\n', Start);
		End = End == std::string::npos ? Text.size() : End;
		const std::string Line = Text.substr(Start, End - Start);
		Start = End + 1;
		if (Line.empty() || Line[0] == '#')
		{
			continue;
		}
		const size_t Sp = Line.rfind(' ');
		if (Sp != std::string::npos)
		{
			Want[Line.substr(0, Sp)] = Line.substr(Sp + 1);
		}
	}
	int Mismatch = 0;
	for (const auto& Kv : Vectors)
	{
		const auto It = Want.find(Kv.first);
		if (It == Want.end() || It->second != Kv.second)
		{
			++Mismatch;
			std::printf("  vector %s: %s, expected %s\n", Kv.first.c_str(), Kv.second.c_str(), It == Want.end() ? "(none)" : It->second.c_str());
		}
	}
	Expect(Mismatch == 0 && Want.size() == Vectors.size(), "determinism: every hash matches style_vectors.txt (" + std::to_string(Vectors.size()) + " vectors)");
}

void RunCi(bool Write)
{
	const double Start = NowMs();
	std::printf("ai_styles: the CI run\n");
	CiCharts();
	CiProfiles();
	CiAutopilot();
	CiReads();
	CiMixAndSlip();
	CiSlipAndReads();
	CiExploitPrePass();
	CiIcm();
	CiCopy();
	std::map<std::string, std::string> Vectors;
	Vectors["charts"] = Hex64(ss::charts::Checksum());
	CiLegality(Vectors);
	CiSmoke(Vectors);
	CiVectors(Vectors, Write);
	std::printf("  CI run: %.1f s\n", (NowMs() - Start) / 1000.0);
}

// ---------------------------------------------------------------------------------------------------------- S-8

/** S-8 (§2.10): the harnesses with today's archetypes and Autopilot; the SDs and costs that size every other mode. */
void SpikeS8(int RingHands, int Sngs, bool Full, bool Styles = false, double Sharp = 1.0)
{
	std::printf("%s%s\n", Styles ? "The styles, surveyed with S-8's harnesses" : "S-8: the strength harness spike", Full ? " (opponents at full strength)" : "");
	std::vector<HeroSpec> Heroes = {StyleHero(ss::style::Style::Autopilot)};
	for (int A = 0; !Styles && A <= static_cast<int>(ss::Archetype::Crusher); ++A)
	{
		Heroes.push_back(ArchetypeHero(static_cast<ss::Archetype>(A)));
	}
	for (const ss::style::Info& I : ss::style::Catalog())
	{
		if (Styles && !I.Hidden && I.S != ss::style::Style::Autopilot)
		{
			Heroes.push_back(StyleHero(I.S, Sharp));
		}
	}
	for (const char* Pop : {"micro", "high"})
	{
		for (double Depth : {100.0, 12.0})
		{
			RingConfig C;
			C.Seed = std::string("s8:") + Pop;
			C.Pop = Pop;
			C.Depth = Depth;
			C.Hands = RingHands;
			C.FullField = Full;
			std::printf(" ring %s, %.0f BB, %d hands a hero\n", Pop, Depth, RingHands);
			std::vector<RingResult> Res;
			for (const HeroSpec& H : Heroes)
			{
				Res.push_back(RunRing(C, H));
				const RingResult& R = Res.back();
				const Moments M = Of(R.Net);
				const Moments Adj = Of(R.Adjusted);
				std::printf("  %-16s bb/100 %+7.2f (adj %+7.2f)  SD/hand %6.2f bb  SD/100 %6.1f  %.3f ms/hand  illegal %d\n", H.Label.c_str(), M.Mean * 100, Adj.Mean * 100, M.Sd(),
					M.Sd() * 10.0, R.Ms / RingHands * Threads, R.Illegal);
			}
			for (size_t I = 1; I < Heroes.size(); ++I)
			{
				const Moments D = Paired(Res[I].Net, Res[0].Net);
				const Moments Da = Paired(Res[I].Adjusted, Res[0].Adjusted);
				const double Need = std::pow(2.0 * D.Sd() / 0.01, 2.0);
				const double NeedAdj = std::pow(2.0 * Da.Sd() / 0.01, 2.0);
				std::printf("  %-16s - Autopilot: %+7.2f bb/100, paired SD/hand %6.2f (adj %6.2f); hands for a 2 bb/100 margin: %.0f (adj %.0f)\n", Heroes[I].Label.c_str(),
					D.Mean * 100, D.Sd(), Da.Sd(), Need, NeedAdj);
			}
		}
	}
	for (const char* Pop : {"micro", "high"})
	{
		std::printf(" SNG %s, 9 players, 50/30/20, %d a hero\n", Pop, Sngs);
		std::vector<EventSet> Res;
		for (const HeroSpec& H : Heroes)
		{
			Res.push_back(RunEvents(SngSpec(Pop, false), std::string("s8sng:") + Pop, Sngs, H, !Full, true, false));
			const EventSet& E = Res.back();
			const Moments Roi = Of(E.Roi());
			const Moments Pct = Of(E.Pct());
			const Moments Itm = Of(E.Flag(&EventResult::Itm));
			const Moments Won = Of(E.Flag(&EventResult::Won));
			std::printf("  %-16s ROI %+7.1f%% (SD %6.1f)  finish %5.1f%% (SD %5.1f)  ITM %5.1f%%  1st %5.1f%%  %.1f hands, %.2f ms/event\n", H.Label.c_str(), Roi.Mean, Roi.Sd(),
				Pct.Mean, Pct.Sd(), Itm.Mean, Won.Mean, static_cast<double>(E.Hands()) / Sngs, E.Ms() / Sngs);
		}
		for (size_t I = 1; I < Heroes.size(); ++I)
		{
			const Moments Dr = Paired(Res[I].Roi(), Res[0].Roi());
			const Moments Dp = Paired(Res[I].Pct(), Res[0].Pct());
			const Moments Di = Paired(Res[I].Flag(&EventResult::Itm), Res[0].Flag(&EventResult::Itm));
			std::printf("  %-16s - Autopilot: ROI %+6.1f (paired SD %6.1f), finish %+5.1f (SD %5.1f), ITM %+5.1f (SD %5.1f); SNGs for a 10-point ROI margin %.0f, a 3-point ITM margin %.0f\n",
				Heroes[I].Label.c_str(), Dr.Mean, Dr.Sd(), Dp.Mean, Dp.Sd(), Di.Mean, Di.Sd(), std::pow(4.0 * Dr.Sd() / 10.0, 2.0), std::pow(4.0 * Di.Sd() / 3.0, 2.0));
		}
	}
}

// ---------------------------------------------------------------------------------------------------------- trace

/** A look inside a style (debugging, not a check): its actions by street and spot over ring hands, and a few hands in full. */
void TraceMode(const std::string& Id, int Hands, double Depth, const std::string& Pop, int Print)
{
	ss::style::Style S = ss::style::Style::Tag;
	if (!ss::style::FromId(Id, S))
	{
		std::printf("no style %s\n", Id.c_str());
		return;
	}
	RingConfig C;
	C.Seed = "trace:" + Pop;
	C.Pop = Pop;
	C.Depth = Depth;
	C.Hands = Hands;
	const HeroSpec Hero = StyleHero(S);
	const uint64_t MixSeed = Fnv64(C.Seed);
	ss::reads::Table Seen;
	std::map<std::string, int> Counts;
	std::map<std::string, double> NetBy;
	ss::Rng Unused("trace-unused");
	std::vector<ss::Profile> Table;
	for (int HandNo = 0; HandNo < Hands; ++HandNo)
	{
		if (HandNo % C.Reseat == 0)
		{
			Table = RingTable(C, HandNo / C.Reseat);
		}
		ss::HandConfig Cfg;
		for (int Seat = 0; Seat < 9; ++Seat)
		{
			ss::SeatInput In;
			In.Seat = Seat;
			In.Id = Seat == 0 ? std::string(ss::HeroId) : "t" + std::to_string(HandNo / C.Reseat) + "s" + std::to_string(Seat);
			In.Stack = static_cast<ss::Chips>(Depth * static_cast<double>(RingBb));
			Cfg.Players.push_back(In);
		}
		Cfg.ButtonSeat = HandNo % 9;
		Cfg.SmallBlind = RingBb / 2;
		Cfg.BigBlind = RingBb;
		Cfg.Ante = RingBb;
		ss::Rng DeckRng(C.Seed + ":deck:" + std::to_string(HandNo));
		Cfg.Deck = ss::FullDeck();
		DeckRng.Shuffle(Cfg.Deck);
		ss::Hand H(Cfg, Unused);
		std::vector<ss::Rng> Forks;
		for (int Seat = 0; Seat < 9; ++Seat)
		{
			Forks.emplace_back(C.Seed + ":d:" + std::to_string(HandNo) + ":" + std::to_string(Seat));
		}
		std::string Log;
		std::string LastSpot = "preflop folded";
		int Guard = 0;
		while (!H.bComplete && ++Guard < 400)
		{
			const int Idx = H.ToAct;
			const int Seat = H.Seats[static_cast<size_t>(Idx)].Seat;
			ss::BotDecision D;
			if (Seat == 0)
			{
				const ss::style::HeroContext Ctx = RingContext(H, Idx, HandNo, MixSeed);
				ss::Rng G("g"), Ch("c");
				D = RingHeroDecision(Hero, Ctx, Seen, Forks[0], G, Ch);
				const ss::PlayerView& V = Ctx.View;
				const char* Street = V.CurrentStreet == ss::Street::Preflop ? "pre" : V.CurrentStreet == ss::Street::Flop ? "flop" : V.CurrentStreet == ss::Street::Turn ? "turn" : "river";
				int Opps = 0;
				for (size_t K = 0; K < V.Seats.size(); ++K)
				{
					Opps += static_cast<int>(K) != V.Me && !V.Seats[K].Folded ? 1 : 0;
				}
				const char* Act = D.Action.Type == ss::PlayerAction::Kind::Fold ? "fold" : D.Action.Type == ss::PlayerAction::Kind::Check ? "check" : D.Action.Type == ss::PlayerAction::Kind::Call ? (V.ToCall > 0 ? "call" : "check") : V.ToCall > 0 ? "raise" : "bet";
				const std::string Key = std::string(Street) + (V.ToCall > 0 ? " facing" : " free") + (Opps >= 2 ? " multi" : " hu") + " " + Act;
				++Counts[Key];
				LastSpot = std::string(Street) + (Opps >= 2 ? " multi" : " hu");
				char Buf[256];
				std::snprintf(Buf, sizeof(Buf), "    %-5s %s%s board %-14s pot %5lld call %5lld: %s %.0f (eq %.2f)\n", Street, ss::CardToString(V.Hole[0]).c_str(), ss::CardToString(V.Hole[1]).c_str(),
					[&]() {
						std::string B;
						for (const ss::Card X : V.Board)
						{
							B += ss::CardToString(X);
						}
						return B;
					}()
						.c_str(),
					static_cast<long long>(V.Pot), static_cast<long long>(V.ToCall), Act, D.Action.To, D.Equity);
				Log += Buf;
			}
			else
			{
				ss::BotContext Ctx;
				Ctx.Prof = &Table[static_cast<size_t>(Seat - 1)];
				Ctx.Fast = true;
				Ctx.R = &Forks[static_cast<size_t>(Seat)];
				D = ss::Decide(ss::MakeView(H, Idx), Ctx);
			}
			if (!H.Act(D.Action))
			{
				const ss::LegalActions L = H.GetLegalActions();
				H.Act(L.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Fold());
			}
		}
		const double Net = static_cast<double>(H.Seats[0].Stack - H.Seats[0].StartStack) / static_cast<double>(RingBb);
		NetBy[LastSpot] += Net;
		NetBy["all"] += Net;
		if (HandNo < Print && !Log.empty())
		{
			std::printf("  hand %d: net %+.1f bb\n%s", HandNo, Net, Log.c_str());
		}
		ss::reads::Count(H, 0, Seen);
	}
	std::printf("  %s over %d hands at %.0f BB (%s): actions\n", Hero.Label.c_str(), Hands, Depth, Pop.c_str());
	for (const auto& Kv : Counts)
	{
		std::printf("    %-32s %6d\n", Kv.first.c_str(), Kv.second);
	}
	std::printf("  net by the last street the hero acted on (bb/100 of all hands):\n");
	for (const auto& Kv : NetBy)
	{
		std::printf("    %-20s %+8.2f\n", Kv.first.c_str(), Kv.second * 100.0 / Hands);
	}
}

// ---------------------------------------------------------------------------------------------------------- ring mode

/**
 * Sample sizes from S-8 (2026-10-12, ssb/u2-core-styles/runs/s8_100k.txt; the opponents Fast as §2.10 has them): at 100 BB
 * a style against another has a paired SD of 6 to 10 bb a hand (about 1.4 to 3.5 million hands for a 2 bb/100 margin at
 * 2 SE <= 1); at 12 BB 1.0 to 1.2 bb a hand, 0.4 all-in adjusted (about 6,000 hands). A ring hand costs 0.1 ms at 100 BB
 * and 0.6 to 0.9 ms at 12 BB.
 */
constexpr int RingDeepHands = 1500000;
constexpr int RingShortHands = 60000;

/** Calibration (§2.5's knob 2): Exploitative against TAG on the same hands, by hands observed, against one population. */
void CurveMode(int Hands, const std::string& Pop, double Depth)
{
	RingConfig C;
	C.Seed = "curve:" + Pop;
	C.Pop = Pop;
	C.Depth = Depth;
	C.Hands = Hands;
	const RingResult Tag = RunRing(C, StyleHero(ss::style::Style::Tag));
	const RingResult Ex = RunRing(C, StyleHero(ss::style::Style::Exploit));
	const std::vector<double> Edge = Diff(Ex.Net, Tag.Net);
	const Rate All = RateOf(Edge);
	const Rate A = RateOf(Bucket(Ex, Edge, 0, 20));
	const Rate B = RateOf(Bucket(Ex, Edge, 20, 100));
	const Rate L = RateOf(Bucket(Ex, Edge, 100, 150));
	std::printf("  Exploitative - TAG, %s %.0f BB, %d hands: all %+.2f (SE %.2f); 0-20 %+.2f (%.2f), 20-100 %+.2f (%.2f), 100+ %+.2f (%.2f) bb/100\n", Pop.c_str(), Depth,
		Hands, All.Bb100, All.Se, A.Bb100, A.Se, B.Bb100, B.Se, L.Bb100, L.Se);
}

void RingMode(double Scale)
{
	using ss::style::Style;
	std::printf("ai_styles ring: assertions 2, 3, 4, 5, 7, 10 and 14 (scale %.2f, %d threads)\n", Scale, Threads);
	const int Deep = static_cast<int>(RingDeepHands * Scale);
	const int Short = static_cast<int>(RingShortHands * Scale);
	auto Run = [&](const char* Pop, double Depth, int Hands, const HeroSpec& H, bool Wipe = false, bool Audit = false, bool Full = false) {
		RingConfig C;
		C.Seed = std::string("ring:") + Pop;
		C.Pop = Pop;
		C.Depth = Depth;
		C.Hands = Hands;
		C.WipeEveryOrbit = Wipe;
		C.Audit = Audit;
		C.FullField = Full;
		const RingResult R = RunRing(C, H);
		const Rate X = RateOf(R.Net);
		std::printf("  %-14s %-8s %5.0f BB %8d hands: %+7.2f bb/100 (SE %.2f), SD/100 %.1f, %.1f s\n", H.Label.c_str(), Pop, Depth, Hands, X.Bb100, X.Se, Of(R.Net).Sd() * 10.0,
			R.Ms / 1000.0);
		Expect(R.Illegal == 0, H.Label + " plays legally in the ring");
		return R;
	};
	// #2: LAG swings more than TAG (block SDs, paired).
	for (const char* Pop : {"micro", "high"})
	{
		const RingResult Tag = Run(Pop, 100, Deep / 4, StyleHero(Style::Tag));
		const RingResult Lag = Run(Pop, 100, Deep / 4, StyleHero(Style::Lag));
		Moments Ratio;
		for (size_t Start = 0; Start + 1000 <= Tag.Net.size(); Start += 1000)
		{
			const std::vector<double> A(Lag.Net.begin() + static_cast<long long>(Start), Lag.Net.begin() + static_cast<long long>(Start + 1000));
			const std::vector<double> B(Tag.Net.begin() + static_cast<long long>(Start), Tag.Net.begin() + static_cast<long long>(Start + 1000));
			Ratio.Add(Of(A).Sd() / std::max(1e-9, Of(B).Sd()));
		}
		Expect(Clears(std::string("#2 LAG's SD/100 over TAG's, ") + Pop, Ratio.Mean, Ratio.Se(), 1.15), std::string("#2 LAG swings at least 1.15x TAG, ") + Pop);
	}
	// #3 and #5: GTO is steady across populations; Exploitative with reads beats it against micro, not against high.
	std::map<std::string, Rate> GtoRates, TagRates, LagRates;
	std::map<std::string, RingResult> Gto, Exploit;
	for (const char* Pop : {"micro", "low", "high", "crusher"})
	{
		Gto[Pop] = Run(Pop, 100, Deep, StyleHero(Style::Gto), false, std::string(Pop) == "high");
		GtoRates[Pop] = RateOf(Gto[Pop].Net);
		TagRates[Pop] = RateOf(Run(Pop, 100, Deep / 2, StyleHero(Style::Tag)).Net);
		LagRates[Pop] = RateOf(Run(Pop, 100, Deep / 2, StyleHero(Style::Lag)).Net);
	}
	for (const char* Pop : {"micro", "high", "crusher"})
	{
		Expect(Clears(std::string("#3 GTO bb/100 against ") + Pop, GtoRates[Pop].Bb100, GtoRates[Pop].Se, 0.0), std::string("#3 GTO wins against ") + Pop);
	}
	auto Spread = [](std::map<std::string, Rate>& M) {
		double Lo = 1e9, Hi = -1e9;
		for (const auto& Kv : M)
		{
			Lo = std::min(Lo, Kv.second.Bb100);
			Hi = std::max(Hi, Kv.second.Bb100);
		}
		return Hi - Lo;
	};
	std::printf("  #3 spread across micro, low, high, crusher: GTO %.2f, TAG %.2f, LAG %.2f bb/100\n", Spread(GtoRates), Spread(TagRates), Spread(LagRates));
	Expect(Spread(GtoRates) <= Spread(TagRates) && Spread(GtoRates) <= Spread(LagRates), "#3 GTO's spread across the populations is the smallest of the winning styles");
	Expect(TagRates["crusher"].Bb100 < GtoRates["crusher"].Bb100 && LagRates["crusher"].Bb100 < GtoRates["crusher"].Bb100,
		"#3 TAG and LAG win less against the Crusher table than GTO");
	// #4: Exploitative's reads curve, against TAG on the same hands (its play before reads), and the wiped control.
	{
		const RingResult Tag = Run("micro", 100, Deep, StyleHero(Style::Tag));
		const RingResult Ex = Run("micro", 100, Deep, StyleHero(Style::Exploit));
		const RingResult Wiped = Run("micro", 100, Deep, StyleHero(Style::Exploit), true);
		Exploit["micro"] = Ex;
		const std::vector<double> Edge = Diff(Ex.Net, Tag.Net);
		const std::vector<double> WipedEdge = Diff(Wiped.Net, Tag.Net);
		RingResult Shape = Ex;
		const Rate Early = RateOf(Bucket(Shape, Edge, 0, 20));
		const Rate Mid = RateOf(Bucket(Shape, Edge, 20, 100));
		const Rate Late = RateOf(Bucket(Shape, Edge, 100, 150));
		const Rate WEarly = RateOf(Bucket(Shape, WipedEdge, 0, 20));
		const Rate WLate = RateOf(Bucket(Shape, WipedEdge, 100, 150));
		std::printf("  #4 Exploitative - TAG by hands observed: 0-20 %+.2f, 20-100 %+.2f, 100+ %+.2f bb/100; wiped every orbit 0-20 %+.2f, 100+ %+.2f\n", Early.Bb100, Mid.Bb100,
			Late.Bb100, WEarly.Bb100, WLate.Bb100);
		const double Rise = Late.Bb100 - Early.Bb100;
		const double RiseSe = std::sqrt(Late.Se * Late.Se + Early.Se * Early.Se);
		Expect(Clears("#4 the 100+ bucket over the 0-20 bucket, micro", Rise, RiseSe, 3.0), "#4 Exploitative climbs with what it has seen");
		const double WRise = WLate.Bb100 - WEarly.Bb100;
		const double WSe = std::sqrt(WLate.Se * WLate.Se + WEarly.Se * WEarly.Se);
		Expect(Clears("#4 the wiped control rises under 1 bb/100", 1.0 - WRise, WSe, 0.0), "#4 the wiped control doesn't climb");
		// #5: Exploitative with 100+ observed hands against GTO, micro.
		const Rate VsGto = RateOf(Bucket(Shape, Diff(Ex.Net, Gto["micro"].Net), 100, 150));
		Expect(Clears("#5 Exploitative (100+ hands) - GTO, micro", VsGto.Bb100, VsGto.Se, 0.0), "#5 Exploitative with reads beats GTO against micro");
	}
	{
		const RingResult Ex = Run("high", 100, Deep, StyleHero(Style::Exploit));
		const Rate VsGto = RateOf(Bucket(Ex, Diff(Gto["high"].Net, Ex.Net), 100, 150));
		Expect(Clears("#5 GTO - Exploitative (100+ hands), high, over -1", VsGto.Bb100, VsGto.Se, -1.0), "#5 GTO is within 1 bb/100 of Exploitative against high");
	}
	// #6: Maniac loses to TAG in micro and high, and loses money against high. Against Fast opponents (§2.10's ring) it
	// doesn't: Fast bots fold to aggression far too often (S-8: Maniac +55 bb/100 against micro, TAG +32), so #6 is read
	// against the full-strength field the hero's table really has (a tenth of the hands; a hand costs ~1 ms).
	for (const char* Pop : {"micro", "high"})
	{
		const RingResult Tag = Run(Pop, 100, Deep / 10, StyleHero(Style::Tag), false, false, true);
		const RingResult Man = Run(Pop, 100, Deep / 10, StyleHero(Style::Maniac), false, false, true);
		const Rate D = RateOf(Diff(Tag.Net, Man.Net));
		Expect(Clears(std::string("#6 TAG - Maniac, ") + Pop, D.Bb100, D.Se, 0.0), std::string("#6 Maniac loses to TAG, ") + Pop);
		if (std::string(Pop) == "high")
		{
			const Rate M = RateOf(Man.Net);
			Expect(Clears("#6 Maniac's loss against high", -M.Bb100, M.Se, 0.0), "#6 Maniac loses money against high");
		}
	}
	// #7: Push/Fold beats TAG at 12 BB by 2 bb/100 or more (all-in adjusted, paired) and loses to it at 100 BB.
	{
		const RingResult Tag12 = Run("micro", 12, Short, StyleHero(Style::Tag));
		const RingResult Pf12 = Run("micro", 12, Short, StyleHero(Style::PushFold));
		const Rate Short12 = RateOf(Diff(Pf12.Adjusted, Tag12.Adjusted));
		Expect(Clears("#7 Push/Fold - TAG at 12 BB (all-in adjusted)", Short12.Bb100, Short12.Se, 2.0), "#7 Push/Fold beats TAG at 12 BB");
		const RingResult Tag100 = Run("micro", 100, Deep / 2, StyleHero(Style::Tag));
		const RingResult Pf100 = Run("micro", 100, Deep / 2, StyleHero(Style::PushFold));
		const Rate Deep100 = RateOf(Diff(Tag100.Net, Pf100.Net));
		Expect(Clears("#7 TAG - Push/Fold at 100 BB", Deep100.Bb100, Deep100.Se, 0.0), "#7 Push/Fold loses to TAG deep");
	}
	// #10: Sharpness 0.5 loses to Sharpness 1.0 for every style but Autopilot (slips cost), at 12 BB all-in adjusted.
	for (const ss::style::Info& I : ss::style::Catalog())
	{
		if (I.S == Style::Autopilot || I.Hidden)
		{
			continue;
		}
		const RingResult Full = Run("micro", 12, Short / 2, StyleHero(I.S, 1.0));
		const RingResult Half = Run("micro", 12, Short / 2, StyleHero(I.S, 0.5));
		const Rate Cost = RateOf(Diff(Full.Adjusted, Half.Adjusted));
		Expect(Clears(std::string("#10 ") + I.Name + " at 1.0 - at 0.5 (12 BB, adjusted)", Cost.Bb100, Cost.Se, 0.0), std::string("#10 slips cost ") + I.Name);
	}
	// #14: GTO's river balance over its own range.
	{
		const RingResult& G = Gto["high"];
		std::printf("  #14 GTO's river: bluff share - s/(1+2s) at 75%%: %+.3f (n %.0f), at pot: %+.3f (n %.0f); continue - MDF: %+.3f (n %.0f)\n", G.Bluff75.Mean, G.Bluff75.N,
			G.Bluff100.Mean, G.Bluff100.N, G.ContinueGap.Mean, G.ContinueGap.N);
		Expect(G.Bluff75.N >= 50 && std::fabs(G.Bluff75.Mean) + 2.0 * G.Bluff75.Se() <= 0.05, "#14 GTO's 75% river bets are balanced within 5 points");
		Expect(G.Bluff100.N >= 50 && std::fabs(G.Bluff100.Mean) + 2.0 * G.Bluff100.Se() <= 0.05, "#14 GTO's pot-size river bets are balanced within 5 points");
		Expect(G.ContinueGap.N >= 50 && std::fabs(G.ContinueGap.Mean) + 2.0 * G.ContinueGap.Se() <= 0.05, "#14 GTO defends its river range at MDF within 5 points");
	}
}

// ---------------------------------------------------------------------------------------------------------- sng mode

/** Sample sizes from S-8: a style against another has a paired ROI SD of 170 to 220 points an SNG and an ITM SD of 57 to 67. */
constexpr int SngCount = 8000;

void SngMode(double Scale)
{
	using ss::style::Style;
	const int N = static_cast<int>(SngCount * Scale);
	std::printf("ai_styles sng: assertions 8, 9 and 13 (%d SNGs a style, %d threads)\n", N, Threads);
	std::map<int, EventSet> Res;
	for (const ss::style::Info& I : ss::style::Catalog())
	{
		if (I.Hidden)
		{
			continue;
		}
		Res[static_cast<int>(I.S)] = RunEvents(SngSpec("micro", false), "sng:micro", N, StyleHero(I.S), true, true, false);
		const EventSet& E = Res[static_cast<int>(I.S)];
		std::printf("  %-14s ROI %+6.1f%% (SD %5.1f)  finish %5.1f%%  ITM %5.1f%%  1st %5.1f%%  %.1f s\n", I.Name, Of(E.Roi()).Mean, Of(E.Roi()).Sd(), Of(E.Pct()).Mean,
			Of(E.Flag(&EventResult::Itm)).Mean, Of(E.Flag(&EventResult::Won)).Mean, E.Ms() / 1000.0 / Threads);
		Expect(E.Illegal() == 0, std::string(I.Name) + " plays its SNGs legally");
	}
	auto Get = [&](Style S) -> const EventSet& { return Res[static_cast<int>(S)]; };
	// #9: Nit reaches the money at least as often as TAG; its ROI swings least.
	{
		const Moments D = Paired(Get(Style::Nit).Flag(&EventResult::Itm), Get(Style::Tag).Flag(&EventResult::Itm));
		Expect(Clears("#9 Nit's ITM - TAG's", D.Mean, D.Se(), 0.0), "#9 Nit cashes at least as often as TAG");
		double NitSd = Of(Get(Style::Nit).Roi()).Sd();
		bool Lowest = true;
		for (const auto& Kv : Res)
		{
			Lowest = Lowest && (Kv.first == static_cast<int>(Style::Nit) || Of(Kv.second.Roi()).Sd() >= NitSd);
		}
		Expect(Lowest, "#9 Nit's ROI swings the least of the styles");
	}
	// #13: against micro on paired seeds.
	for (const Style S : {Style::Reg, Style::Tag, Style::Gto})
	{
		const Moments D = Paired(Get(S).Roi(), Get(Style::Autopilot).Roi());
		Expect(Clears(std::string("#13 ") + ss::style::InfoOf(S).Name + " ROI - Autopilot's, over -2", D.Mean, D.Se(), -2.0),
			std::string("#13 ") + ss::style::InfoOf(S).Name + " is no worse than Autopilot by 2 points of ROI");
	}
	{
		const Moments D = Paired(Get(Style::Autopilot).Roi(), Get(Style::Maniac).Roi());
		Expect(Clears("#13 Autopilot's ROI - Maniac's, over 10", D.Mean, D.Se(), 10.0), "#13 Autopilot beats Maniac by 10 points of ROI");
		const Moments G = Of(Get(Style::Gto).Roi());
		Expect(Clears("#13 GTO's ROI", G.Mean, G.Se(), 0.0), "#13 GTO wins money in micro SNGs");
		const Moments W = Paired(Get(Style::Lag).Flag(&EventResult::Won), Get(Style::Tag).Flag(&EventResult::Won));
		Expect(Clears("#13 LAG's first places - TAG's", W.Mean, W.Se(), 0.0), "#13 LAG wins more SNGs than TAG");
	}
	// #8: ICM Shark in satellites: more seats than TAG, and fewer big-stack jams near the end.
	{
		const EventSet Icm = RunEvents(SngSpec("micro", true), "sat:micro", N, StyleHero(Style::IcmShark), true, false, true);
		const EventSet Tag = RunEvents(SngSpec("micro", true), "sat:micro", N, StyleHero(Style::Tag), true, false, true);
		const Moments D = Paired(Icm.Flag(&EventResult::Seat), Tag.Flag(&EventResult::Seat));
		std::printf("  satellites: ICM Shark %.1f%% seats, TAG %.1f%%\n", Of(Icm.Flag(&EventResult::Seat)).Mean, Of(Tag.Flag(&EventResult::Seat)).Mean);
		Expect(Clears("#8 ICM Shark's seat rate - TAG's, over 3", D.Mean, D.Se(), 3.0), "#8 ICM Shark wins 3 points more seats than TAG");
		int Is = 0, Ij = 0, Ts = 0, Tj = 0;
		for (const EventResult& E : Icm.R)
		{
			Is += E.BigStackSpots;
			Ij += E.BigStackJams;
		}
		for (const EventResult& E : Tag.R)
		{
			Ts += E.BigStackSpots;
			Tj += E.BigStackJams;
		}
		const double Ri = Is > 0 ? static_cast<double>(Ij) / Is : 0.0;
		const double Rt = Ts > 0 ? static_cast<double>(Tj) / Ts : 0.0;
		const double Se = std::sqrt(Ri * (1 - Ri) / std::max(1, Is) + Rt * (1 - Rt) / std::max(1, Ts));
		Expect(Clears("#8 TAG's big-stack jam rate - ICM Shark's (last 3x the seats)", Rt - Ri, Se, 0.0), "#8 ICM Shark's covering stack jams less than TAG's");
	}
}

// ---------------------------------------------------------------------------------------------------------- strength mode

struct StrengthRow
{
	std::string Label;
	std::map<std::string, EventSet> ByFormat;
};

void StrengthMode(int Hypers, int Turbos, int Deeps, const std::vector<std::string>& Only)
{
	using ss::style::Style;
	std::printf("ai_styles strength: §2.5 (hypers %d, turbos %d, deep %d, %d threads). Average finish, 50%% = average.\n", Hypers, Turbos, Deeps, Threads);
	struct Format
	{
		const char* Name;
		ss::TournamentSpec Spec;
		int Count;
	};
	const std::vector<Format> Formats = {{"hyper", HyperSpec(), Hypers}, {"turbo", TurboSpec(), Turbos}, {"deep", DeepSpec(), Deeps}};
	std::vector<HeroSpec> Heroes = {StyleHero(Style::Autopilot)};
	// "sharp=0.9" (one or more) replaces the two Sharpnesses measured (1.0 and the default 0.80).
	std::vector<double> Sharps;
	std::vector<std::string> Ids;
	for (const std::string& O : Only)
	{
		(O.rfind("sharp=", 0) == 0 ? (void)Sharps.push_back(std::atof(O.c_str() + 6)) : (void)Ids.push_back(O));
	}
	if (Sharps.empty())
	{
		Sharps = {1.0, 0.8};
	}
	auto Wanted = [&](const std::string& Id) { return Ids.empty() || std::find(Ids.begin(), Ids.end(), Id) != Ids.end(); };
	if (Wanted("coach"))
	{
		Heroes.push_back(CoachHero());
	}
	if (Wanted("accurate"))
	{
		// The 94% hero: tune its best-option probability on a pilot until its own accuracy reads 94.
		double Lo = 0.6, Hi = 1.0;
		for (int Round = 0; Round < 5; ++Round)
		{
			const double P = 0.5 * (Lo + Hi);
			const EventSet Pilot = RunEvents(HyperSpec(), "pilot", 60, AccurateHero(P), false, false, false);
			const double Acc = Pilot.Accuracy();
			std::printf("  94%% hero pilot: p %.3f reads %.1f\n", P, Acc);
			(Acc < 94.0 ? Lo : Hi) = P;
		}
		Heroes.push_back(AccurateHero(0.5 * (Lo + Hi)));
	}
	for (const ss::style::Info& I : ss::style::Catalog())
	{
		if (I.Hidden || I.S == Style::Autopilot || !Wanted(I.Id))
		{
			continue;
		}
		for (const double Sh : Sharps)
		{
			Heroes.push_back(StyleHero(I.S, Sh));
		}
	}
	std::vector<StrengthRow> Rows;
	for (const HeroSpec& H : Heroes)
	{
		StrengthRow Row;
		Row.Label = H.Label;
		for (const Format& F : Formats)
		{
			if (F.Count <= 0)
			{
				continue;
			}
			const double Start = NowMs();
			Row.ByFormat[F.Name] = RunEvents(F.Spec, std::string("strength:") + F.Name, F.Count, H, false, false, false);
			// The shape §2.5 checks: the finish's SD, the cash rate, wins per 100 and early busts (the bottom quarter).
			const EventSet& Got = Row.ByFormat[F.Name];
			double Early = 0;
			for (const EventResult& E : Got.R)
			{
				Early += E.Pct < 25.0 ? 1.0 : 0.0;
			}
			std::printf("  %-22s %-6s %5.1f%% (SE %.2f, SD %4.1f)  ROI %+6.1f%%  cash %4.1f%%  wins/100 %.2f  early %4.1f%%  accuracy %.1f  %.0f s\n", H.Label.c_str(), F.Name,
				Of(Got.Pct()).Mean, Of(Got.Pct()).Se(), Of(Got.Pct()).Sd(), Of(Got.Roi()).Mean, Of(Got.Flag(&EventResult::Itm)).Mean, Of(Got.Flag(&EventResult::Won)).Mean,
				100.0 * Early / static_cast<double>(std::max<size_t>(1, Got.R.size())), Got.Accuracy(), (NowMs() - Start) / 1000.0);
			std::fflush(stdout);
		}
		Rows.push_back(Row);
	}
	// The table: each format's mean with its 95% interval and the paired difference to Autopilot.
	std::printf("\n  %-22s", "hero");
	for (const Format& F : Formats)
	{
		if (F.Count > 0)
		{
			std::printf(" | %-36s", F.Name);
		}
	}
	std::printf(" | mixed: finish, ROI vs Autopilot\n");
	const StrengthRow& A = Rows.front();
	std::vector<std::pair<bool, std::string>> Later; // the checks, printed after the table
	for (const StrengthRow& Row : Rows)
	{
		std::printf("  %-22s", Row.Label.c_str());
		std::vector<double> MixPct, MixRoi;
		for (const Format& F : Formats)
		{
			if (F.Count <= 0)
			{
				continue;
			}
			const EventSet& E = Row.ByFormat.at(F.Name);
			const EventSet& Ea = A.ByFormat.at(F.Name);
			const Moments M = Of(E.Pct());
			const Moments D = Paired(E.Pct(), Ea.Pct());
			std::printf(" | %5.1f +-%4.2f  vs A %+5.2f +-%4.2f      ", M.Mean, 1.96 * M.Se(), D.Mean, 1.96 * D.Se());
			const std::vector<double> Dp = Diff(E.Pct(), Ea.Pct());
			const std::vector<double> Dr = Diff(E.Roi(), Ea.Roi());
			MixPct.insert(MixPct.end(), Dp.begin(), Dp.end());
			MixRoi.insert(MixRoi.end(), Dr.begin(), Dr.end());
			// The ceilings (one-sided): no style above A + 1.5 or 55.5%; ROI at most Autopilot's + 8.
			if (&Row != &A)
			{
				const Moments R = Paired(E.Roi(), Ea.Roi());
				const bool Ceiling = D.Mean + 1.645 * D.Se() <= 1.5 && M.Mean + 1.645 * M.Se() <= 55.5;
				const bool RoiCeiling = R.Mean + 1.645 * R.Se() <= 8.0;
				if (Row.Label.find('@') == std::string::npos && Row.Label.find("coach") == std::string::npos && Row.Label.find("94%") == std::string::npos)
				{
					Later.emplace_back(Ceiling, "U3 ceiling: " + Row.Label + " in " + F.Name + " stays under Autopilot + 1.5 and 55.5%");
					Later.emplace_back(RoiCeiling, "U3 ceiling: " + Row.Label + " in " + F.Name + " stays under Autopilot's ROI + 8");
				}
			}
		}
		const Moments Mp = Of(MixPct);
		const Moments Mr = Of(MixRoi);
		std::printf(" | %+5.2f +-%4.2f, %+6.1f +-%5.1f\n", Mp.Mean, 1.96 * Mp.Se(), Mr.Mean, 1.96 * Mr.Se());
		// At the default Sharpness, on the mixed schedule, nothing beats Autopilot: one-sided the same way as the ceilings
		// (§2.5), the paired difference plus 1.645 SE at or under 0, for the finish and for the ROI.
		if (Row.Label.find("@80") != std::string::npos)
		{
			Later.emplace_back(Mp.Mean + 1.645 * Mp.Se() <= 0.0 && Mr.Mean + 1.645 * Mr.Se() <= 0.0, "U3 default Sharpness: " + Row.Label + " doesn't beat Autopilot on the mixed schedule");
		}
	}
	for (const auto& L : Later)
	{
		Expect(L.first, L.second);
	}
}
} // namespace style_test

int main(int Argc, char** Argv)
{
	using namespace style_test;
	std::vector<std::string> Args;
	for (int I = 1; I < Argc; ++I)
	{
		const std::string A = Argv[I];
		if (A == "--threads" && I + 1 < Argc)
		{
			Threads = std::max(1, std::atoi(Argv[++I]));
			continue;
		}
		Args.push_back(A);
	}
	const std::string Mode = Args.empty() ? "" : Args[0];
	auto Num = [&](size_t I, double Default) { return Args.size() > I ? std::atof(Args[I].c_str()) : Default; };
	if (Mode == "s8")
	{
		SpikeS8(static_cast<int>(Num(1, 20000)), static_cast<int>(Num(2, 400)), Args.size() > 3 && Args[3] == "full");
	}
	else if (Mode == "trace" && Args.size() > 1)
	{
		TraceMode(Args[1], static_cast<int>(Num(2, 3000)), Num(3, 100.0), Args.size() > 4 ? Args[4] : "micro", static_cast<int>(Num(5, 10)));
	}
	else if (Mode == "curve")
	{
		CurveMode(static_cast<int>(Num(1, 300000)), Args.size() > 2 ? Args[2] : "micro", Num(3, 100.0));
	}
	else if (Mode == "balance")
	{
		RingConfig C;
		C.Seed = "ring:high";
		C.Pop = "high";
		C.Hands = static_cast<int>(Num(1, 100000));
		C.Audit = true;
		const RingResult G = RunRing(C, StyleHero(ss::style::Style::Gto));
		std::printf("  GTO river: bluff share - s/(1+2s) at 75%%: %+.3f (SE %.3f, n %.0f), at pot: %+.3f (SE %.3f, n %.0f); continue - MDF: %+.3f (SE %.3f, n %.0f)\n",
			G.Bluff75.Mean, G.Bluff75.Se(), G.Bluff75.N, G.Bluff100.Mean, G.Bluff100.Se(), G.Bluff100.N, G.ContinueGap.Mean, G.ContinueGap.Se(), G.ContinueGap.N);
	}
	else if (Mode == "survey")
	{
		SpikeS8(static_cast<int>(Num(1, 20000)), static_cast<int>(Num(2, 400)), Args.size() > 3 && Args[3] == "full", true, Num(4, 1.0));
	}
	else if (Mode == "ring")
	{
		RingMode(Num(1, 1.0));
	}
	else if (Mode == "sng")
	{
		SngMode(Num(1, 1.0));
	}
	else if (Mode == "strength")
	{
		std::vector<std::string> Only(Args.size() > 4 ? Args.begin() + 4 : Args.end(), Args.end());
		StrengthMode(static_cast<int>(Num(1, 3000)), static_cast<int>(Num(2, 1000)), static_cast<int>(Num(3, 1000)), Only);
	}
	else
	{
		Threads = std::min(Threads, 3);
		RunCi(Mode == "--write");
	}
	std::printf(Failures == 0 ? "style tests: all passed\n" : "style tests: %d FAILED\n", Failures);
	return Failures == 0 ? 0 : 1;
}
