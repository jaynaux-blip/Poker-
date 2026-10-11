// Engine invariants that hold independently of the TypeScript build, plus a benchmark.
#include "ShortStack/AI/Profiles.h"
#include "ShortStack/Cards.h"
#include "ShortStack/Hand.h"
#include "ShortStack/Rng.h"
#include "ShortStack/Tournament.h"
#include "ShortStack/Game/ChipSet.h"
#include "ShortStack/Game/Live.h"

#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

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

// ------------------------------------------------------------------ the chips on the felt (core-tableui, next_segment_spec §6.2 B)

/** Columns hold one denomination each, at most 20, at most one short column per denomination, biggest at the front. */
bool WellStacked(const std::vector<ss::chipset::Column>& Cols)
{
	ss::Chips Prev = -1;
	ss::Chips ShortOf = -1;
	for (const ss::chipset::Column& C : Cols)
	{
		if (C.Count <= 0 || C.Count > 20 || C.Value <= 0)
		{
			return false;
		}
		if (Prev >= 0 && C.Value > Prev)
		{
			return false; // a bigger chip behind a smaller one
		}
		if (C.Value != Prev)
		{
			ShortOf = -1;
		}
		if (ShortOf == C.Value)
		{
			return false; // a column after this denomination's short one
		}
		if (C.Count < 20)
		{
			ShortOf = C.Value;
		}
		Prev = C.Value;
	}
	return true;
}

void ChipSetLayout()
{
	using namespace ss::chipset;
	const std::vector<ss::Level>& Levels = ss::live::DeepstackLevels();
	const std::vector<Denom> Start = InPlay(Room::Embercrest, Levels, 0, 6);
	Expect(Start.size() == 5 && Start.front().Value == 100 && Start.back().Value == 25000, "chipset: the Embercrest's five chips are all in play at level 1");
	// Every multiple of 100 to 3,000,000 lays out exactly in the chips in play, in good columns, as a stack, a bet and a pot.
	bool Exact = true;
	bool Columns = true;
	bool InPlayOnly = true;
	for (ss::Chips A = 100; A <= 3000000; A += 100)
	{
		for (const Style S : {Style::Stack, Style::Bet, Style::Pot})
		{
			const std::vector<Column> Cols = Lay(A, Start, S);
			Exact = Exact && Sum(Cols) == A;
			Columns = Columns && WellStacked(Cols);
			for (const Column& C : Cols)
			{
				InPlayOnly = InPlayOnly && C.Denom >= 0 && C.Denom < static_cast<int>(Start.size()) && Start[static_cast<size_t>(C.Denom)].Value == C.Value;
			}
		}
	}
	Expect(Exact, "chipset: every multiple of 100 to 3,000,000 lays out exactly");
	Expect(Columns, "chipset: columns of at most 20, one denomination, one short column each, biggest at the front");
	Expect(InPlayOnly, "chipset: only chips in play");
	// A bet slides the fewest chips.
	{
		const std::vector<Column> Bet = Lay(18400, Start, Style::Bet);
		int N = 0;
		for (const Column& C : Bet)
		{
			N += C.Count;
		}
		Expect(N == 3 + 3 + 4, "chipset: a bet of 18,400 is three orange, three yellow and four black");
	}
	// The bags sum to the templates' stacks, the Nightly's exactly as dealt.
	for (const ss::live::EventTemplate& T : ss::live::Schedule())
	{
		const std::vector<Column> Bag = StartingBag(T);
		Expect(Sum(Bag) == T.StartingStack && WellStacked(Bag), (std::string("chipset: the bag sums for ") + T.Name).c_str());
	}
	{
		int Blacks = 0, Purples = 0, Yellows = 0;
		for (const ss::live::EventTemplate& T : ss::live::Schedule())
		{
			if (std::string(T.Key) == "nightly")
			{
				for (const Column& C : StartingBag(T))
				{
					Blacks += C.Value == 100 ? C.Count : 0;
					Purples += C.Value == 500 ? C.Count : 0;
					Yellows += C.Value == 1000 ? C.Count : 0;
				}
			}
		}
		Expect(Blacks == 10 && Purples == 8 && Yellows == 15, "chipset: the Nightly's bag is 10 black, 8 purple and 15 yellow");
	}
	// The colour-ups: the black 100s go at the break before level 13, the purple 500s before level 19, the yellow 1,000s
	// before level 25.
	auto Has = [&](int LevelIndex, ss::Chips V) {
		for (const Denom& D : InPlay(Room::Embercrest, Levels, LevelIndex, 6))
		{
			if (D.Value == V)
			{
				return true;
			}
		}
		return false;
	};
	Expect(Has(11, 100) && !Has(12, 100), "chipset: the black 100s race off at the break before level 13");
	Expect(Has(17, 500) && !Has(18, 500), "chipset: the purple 500s race off at the break before level 19");
	Expect(Has(23, 1000) && !Has(24, 1000), "chipset: the yellow 1,000s race off at the break before level 25");
	Expect(Has(29, 5000) && Has(29, 25000), "chipset: the orange and the top chip stay");
	Expect(InPlay(Room::BackRoom, Levels, 20, 6).size() == 4, "chipset: Dee's game keeps its four");
	// With the race off, odd hundreds stay on the felt as one short column of the old black chip; nothing changes count.
	{
		const std::vector<Denom> Later = InPlay(Room::Embercrest, Levels, 12, 6);
		const std::vector<Column> Cols = Lay(41300, Later, Style::Stack);
		int Old = 0;
		bool OneColumn = true;
		for (const Column& C : Cols)
		{
			if (C.Denom < 0)
			{
				OneColumn = OneColumn && C.Value == 100 && Old == 0;
				Old += C.Count;
			}
		}
		Expect(Sum(Cols) == 41300 && Old == 3 && OneColumn, "chipset: 41,300 after the colour-up keeps three black chips in one column");
		Expect(Find(Room::Embercrest, 100) && Find(Room::Embercrest, 100)->Mesh == std::string("SM_Chip_T100"), "chipset: the old chip is found by value");
	}
	// Dee's game lays out exactly as ABackRoomChips::Break did ($248: two blacks, a green... as before).
	{
		const std::vector<Denom> Dee = Set(Room::BackRoom);
		const std::vector<Column> Cols = Lay(248, Dee, Style::Stack);
		Expect(Sum(Cols) == 248, "chipset: Dee's $248 lays out exactly");
		const std::vector<Column> Bet = Lay(250, Dee, Style::Bet);
		Expect(Sum(Bet) == 250 && Bet.front().Value == 25, "chipset: at Dee's nobody bets a black chip under $300");
	}
	// A glance: full columns exact, a short one to the nearest five chips, the truth inside the bounds; counted is exact.
	bool Bounds = true;
	for (ss::Chips A = 100; A <= 400000; A += 700)
	{
		const Read R = ReadStack(A, Start, false);
		Bounds = Bounds && R.Low <= A && A <= R.High && R.Low <= R.Shown && R.Shown <= R.High;
		const Read Cnt = ReadStack(A, Start, true);
		Bounds = Bounds && Cnt.Exact && Cnt.Low == A && Cnt.High == A && Cnt.Shown == A;
	}
	Expect(Bounds, "chipset: ReadStack's bounds hold the true stack; a count is exact");
	Expect(ReadStack(20000, Start, false).Exact == false, "chipset: a starting stack's short columns are a glance, not a count");
	// The dealer's words.
	Expect(Spoken(18400) == "Eighteen thousand four hundred.", "chipset: spoken 18,400");
	Expect(Spoken(1600) == "Sixteen hundred.", "chipset: spoken 1,600");
	Expect(Spoken(125000) == "One hundred twenty-five thousand.", "chipset: spoken 125,000");
	Expect(Spoken(1000000) == "One million.", "chipset: spoken 1,000,000");
	Expect(Spoken(2000) == "Two thousand." && Spoken(41500) == "Forty-one thousand five hundred.", "chipset: spoken thousands");
	// Big blinds: one decimal under 100 (no trailing .0), never rounding up to ten from under it.
	Expect(InBb(18400, 1600) == "11.5 BB" && InBb(49600, 1600) == "31 BB" && InBb(58800, 6000) == "9.8 BB", "chipset: InBb");
	Expect(InBb(15960, 1600) == "9.9 BB" && InBb(200000, 1000) == "200 BB" && InBb(5, 0).empty(), "chipset: InBb edges");
	// Never rounded up across a whole big blind: the number agrees with the depth colours at 8 and 15 and the 25-BB open.
	Expect(InBb(23950, 1600) == "14.9 BB" && InBb(12790, 1600) == "7.9 BB" && InBb(39990, 1600) == "24.9 BB" && InBb(45854, 1600) == "28.7 BB",
		"chipset: InBb never rounds up to the next whole big blind");
	bool Under = true;
	for (ss::Chips A = 0; A < 160000; A += 7)
	{
		const std::string N = BbNumber(A, 1600);
		Under = Under && std::stoll(N.substr(0, N.find('.'))) <= A / 1600;
	}
	Expect(Under, "chipset: a stack never reads a whole big blind it hasn't got (under 100 BB)");
	// Huge amounts say themselves without running off the words.
	Expect(Spoken(1500000000000LL) == "Fifteen hundred billion." && Spoken(2000000000000LL) == "Two thousand billion.", "chipset: spoken past a billion");
}
} // namespace

int main()
{
	ChipConservationFuzz();
	IllegalActionsRejected();
	TournamentRunsToCompletion();
	ChipSetLayout();
	std::printf("unit tests: %s\n", Failures == 0 ? "all passed" : "FAILED");
	return Failures == 0 ? 0 : 1;
}
