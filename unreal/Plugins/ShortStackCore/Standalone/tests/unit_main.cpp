// Engine invariants that hold independently of the TypeScript build, plus a benchmark.
#include "ShortStack/AI/Profiles.h"
#include "ShortStack/Cards.h"
#include "ShortStack/Hand.h"
#include "ShortStack/Rng.h"
#include "ShortStack/Tournament.h"

#include <chrono>
#include <cstdio>

namespace
{
int Failures = 0;

void Expect(bool Condition, const char* What)
{
	if (!Condition)
	{
		++Failures;
		std::printf("FAILED: %s\n", What);
	}
}

void ChipConservationFuzz()
{
	ss::Rng R("fuzz-cpp");
	for (int T = 0; T < 5000; ++T)
	{
		const int N = 2 + R.Int(8);
		ss::HandConfig Cfg;
		ss::Chips Before = 0;
		for (int I = 0; I < N; ++I)
		{
			const ss::Chips Stack = 1 + R.Int(20000);
			Cfg.Players.push_back({I, "p" + std::to_string(I), Stack});
			Before += Stack;
		}
		Cfg.ButtonSeat = R.Int(N);
		Cfg.SmallBlind = 50;
		Cfg.BigBlind = 100;
		Cfg.Ante = R.Chance(0.5) ? 100 : 0;
		ss::Hand H(Cfg, R);
		int Guard = 0;
		while (!H.bComplete && ++Guard < 500)
		{
			const ss::LegalActions L = H.GetLegalActions();
			const double Roll = R.Next();
			ss::PlayerAction A = ss::PlayerAction::Call();
			if (Roll < 0.15)
			{
				A = ss::PlayerAction::Fold();
			}
			else if (Roll >= 0.6 && L.CanRaise)
			{
				const int Span = static_cast<int>(L.MaxRaiseTo - L.MinRaiseTo + 1);
				A = ss::PlayerAction::RaiseTo(static_cast<double>(L.MinRaiseTo + R.Int(Span > 0 ? Span : 1)));
			}
			if (!H.Act(A))
			{
				Expect(false, "legal action rejected");
				return;
			}
		}
		ss::Chips After = 0;
		for (const ss::HandSeat& S : H.Seats)
		{
			After += S.Stack;
			Expect(S.Stack >= 0, "negative stack");
		}
		Expect(H.bComplete, "hand terminates");
		Expect(After == Before, "chips conserved");
	}
}

void IllegalActionsRejected()
{
	ss::Rng R("illegal");
	ss::HandConfig Cfg;
	Cfg.Players = {{0, "a", 10000}, {1, "b", 10000}, {2, "c", 10000}, {3, "d", 10000}};
	Cfg.ButtonSeat = 0;
	Cfg.SmallBlind = 50;
	Cfg.BigBlind = 100;
	Cfg.Ante = 100;
	ss::Hand H(Cfg, R);
	const size_t EventsBefore = H.Events.size();
	Expect(!H.Act(ss::PlayerAction::Check()), "cannot check facing the big blind");
	Expect(!H.Act(ss::PlayerAction::RaiseTo(150)), "raise below the minimum is rejected");
	Expect(H.Events.size() == EventsBefore, "rejected actions change nothing");
	Expect(H.Act(ss::PlayerAction::RaiseTo(300)), "min raise to 300 accepted");
	Expect(H.GetLegalActions().MinRaiseTo == 500, "next minimum raise is to 500");

	ss::HandConfig Bad;
	Bad.Players = {{0, "solo", 1000}};
	ss::Hand Solo(Bad, R);
	Expect(!Solo.IsValid(), "a one-player hand is invalid");
}

void TournamentRunsToCompletion()
{
	ss::TournamentSpec Spec;
	Spec.Id = "bench";
	Spec.BuyInCents = 110;
	Spec.FeeCents = 10;
	Spec.GuaranteeCents = 100000;
	Spec.Entrants = 1000;
	Spec.LevelMinutes = 5;
	Spec.SecondsPerHand = 42;
	ss::Tournament T(Spec, "Hero", "bench-1000", {{"gh0stfold", ss::Archetype::Crusher}});
	const ss::Profile Sprint = ss::SprintProfile();
	const ss::Chips Total = 1000 * Spec.StartingStack;
	const auto T0 = std::chrono::steady_clock::now();
	int Guard = 0;
	while (!T.bFinished && ++Guard < 5000)
	{
		T.SimulateTick(&Sprint);
		ss::Chips Chips = 0;
		for (const ss::TPlayer* P : T.AlivePlayers())
		{
			Chips += P->Stack;
		}
		if (Chips != Total)
		{
			Expect(false, "tournament chips conserved");
			return;
		}
	}
	const double Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count();
	Expect(T.bFinished, "tournament finishes");
	ss::Chips Prizes = 0;
	for (const ss::TPlayer& P : T.Players)
	{
		Prizes += P.PrizeCents;
	}
	Expect(Prizes == T.PrizePoolCents, "every cent of the prize pool is paid out");
	std::printf("benchmark: 1,000-player tournament, %d rounds, %.0f ms (%.2f ms per round)\n", T.Tick, Ms, Ms / T.Tick);
}
} // namespace

int main()
{
	ChipConservationFuzz();
	IllegalActionsRejected();
	TournamentRunsToCompletion();
	std::printf("unit tests: %s\n", Failures == 0 ? "all passed" : "FAILED");
	return Failures == 0 ? 0 : 1;
}
