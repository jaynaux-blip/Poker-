// Engine invariants that hold independently of the TypeScript build, plus a benchmark.
#include "ShortStack/AI/Profiles.h"
#include "ShortStack/AI/Style.h"
#include "ShortStack/Cards.h"
#include "ShortStack/Hand.h"
#include "ShortStack/Rng.h"
#include "ShortStack/Structure.h"
#include "ShortStack/Tournament.h"
#include "ShortStack/Game/ChipSet.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Session.h"

#include <algorithm>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
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

/**
 * Update 3 (SPEC §3.6 R8, §8.2; spike S-D's harness): once a hand ends, the deck it was dealt from reads back through Hand's
 * two const accessors, so the Second Board can show the stub past DeckPosForShow(). What the hand drew is the deck's head in
 * the order it drew it (the hole cards round by round from the button's left, then a burn before each street's cards), the
 * stub is the rest, and looking draws nothing: the hand and its Rng carry on exactly as if nobody had.
 */
void DeckStubForShow()
{
	ss::Rng R("deck-stub");
	int Hands = 0, AllIns = 0, Wrong = 0;
	for (int T = 0; T < 2000; ++T)
	{
		const int N = 2 + R.Int(8);
		ss::HandConfig Cfg;
		for (int I = 0; I < N; ++I)
		{
			Cfg.Players.push_back({I, "p" + std::to_string(I), 1 + R.Int(20000)});
		}
		Cfg.ButtonSeat = R.Int(N);
		Cfg.SmallBlind = 50;
		Cfg.BigBlind = 100;
		ss::Hand H(Cfg, R);
		bool AllIn = false;
		for (int Guard = 0; !H.bComplete && Guard < 500; ++Guard)
		{
			const ss::LegalActions L = H.GetLegalActions();
			const double Roll = R.Next();
			ss::PlayerAction A = Roll < 0.2 ? ss::PlayerAction::Fold() : ss::PlayerAction::Call();
			if (Roll >= 0.85 && L.CanRaise)
			{
				A = ss::PlayerAction::RaiseTo(static_cast<double>(L.MaxRaiseTo));
			}
			if (!H.Act(A))
			{
				break;
			}
		}
		for (const ss::HandSeat& S : H.Seats)
		{
			AllIn = AllIn || (S.AllIn && !S.Folded);
		}
		uint32_t Before[4];
		R.GetState(Before);
		const size_t Events = H.Events.size();
		const std::vector<ss::Card> Board = H.Board;
		const std::vector<ss::Card>& Deck = H.DeckForShow();
		const int Pos = H.DeckPosForShow();
		// The head: two rounds of hole cards, then each street's burn and cards.
		std::vector<ss::Card> Head;
		for (int Round = 0; Round < 2; ++Round)
		{
			for (int I = 1; I <= N; ++I)
			{
				const ss::HandSeat& S = H.Seats[static_cast<size_t>((H.ButtonIndex() + I) % N)];
				Head.push_back(S.Hole.size() > static_cast<size_t>(Round) ? S.Hole[static_cast<size_t>(Round)] : -1);
			}
		}
		const size_t Streets = Board.size() >= 5 ? 3 : Board.size() >= 4 ? 2 : Board.size() >= 3 ? 1 : 0;
		bool Ok = H.bComplete && Deck.size() == 52 && Pos == static_cast<int>(Head.size() + Streets + Board.size()) && Pos <= static_cast<int>(Deck.size());
		for (size_t K = 0; Ok && K < Head.size(); ++K)
		{
			Ok = Deck[K] == Head[K];
		}
		const size_t At = Head.size();
		for (size_t K = 0; Ok && K < Board.size(); ++K)
		{
			// The flop sits after one burn, the turn after two, the river after three.
			const size_t Burns = K < 3 ? 1 : K == 3 ? 2 : 3;
			Ok = Deck[At + Burns + K] == Board[K];
		}
		std::vector<ss::Card> Sorted(Deck.begin(), Deck.end());
		std::sort(Sorted.begin(), Sorted.end());
		Ok = Ok && std::adjacent_find(Sorted.begin(), Sorted.end()) == Sorted.end() && (Sorted.empty() || (Sorted.front() == 0 && Sorted.back() == 51));
		// Looking drew nothing and changed nothing.
		uint32_t After[4];
		R.GetState(After);
		Ok = Ok && std::equal(Before, Before + 4, After) && H.Events.size() == Events && H.DeckPosForShow() == Pos && H.Board == Board;
		Wrong += Ok ? 0 : 1;
		++Hands;
		AllIns += AllIn ? 1 : 0;
	}
	std::printf("deck stub (S-D): %d hands read back through DeckForShow (%d with an all-in), %d wrong\n", Hands, AllIns, Wrong);
	Expect(Wrong == 0, "update 3: a finished hand's deck reads back through its const accessors, the stub past DeckPosForShow, drawing nothing");
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

// ------------------------------------------------------------------ update 2's engine seams (update 2 SPEC §1.3, §2.8, §6.7)

ss::TournamentSpec U2Spec()
{
	// The parity tests' field (§6.7): a 180-runner hyper, its first 150 ticks.
	ss::TournamentSpec Spec;
	Spec.Id = "u2-hyper";
	Spec.Name = "Hyper Sprint";
	Spec.BuyInCents = 330;
	Spec.FeeCents = 30;
	Spec.Entrants = 180;
	Spec.LevelMinutes = 3;
	Spec.SecondsPerHand = 42;
	Spec.Speed = "Hyper";
	return Spec;
}

std::unique_ptr<ss::Tournament> U2Field(const char* Seed)
{
	return std::unique_ptr<ss::Tournament>(new ss::Tournament(U2Spec(), "Hero", Seed, {{"gh0stfold", ss::Archetype::Crusher}}));
}

bool U2SameAction(const ss::ActionRecord& A, const ss::ActionRecord& B)
{
	return A.OnStreet == B.OnStreet && A.Seat == B.Seat && A.Type == B.Type && A.Added == B.Added && A.To == B.To && A.AllIn == B.AllIn;
}

std::string U2ReadFile(const std::string& Path)
{
	std::string Out;
	if (FILE* F = std::fopen(Path.c_str(), "rb"))
	{
		char Buf[65536];
		size_t N = 0;
		while ((N = std::fread(Buf, 1, sizeof(Buf), F)) > 0)
		{
			Out.append(Buf, N);
		}
		std::fclose(F);
	}
	Out.erase(std::remove(Out.begin(), Out.end(), '\r'), Out.end());
	return Out;
}

void Update2Engine()
{
	const ss::style::HeroBrain Brain; // Autopilot: the stub style::Decide forwards to ss::Decide with ProfileOf(Autopilot)
	const ss::Profile Auto = ss::style::ProfileOf(ss::style::Style::Autopilot);
	// Autopilot's profile is today's Sprint autopilot, number for number.
	{
		const ss::Profile Sprint = ss::SprintProfile();
		Expect(Auto.Label == "Autopilot" && Auto.CallWidth == 1.35 && Auto.Stickiness == 0.35 && Auto.Aggression == 0.6 && Auto.Bluff == 0.12 && Auto.PushFold == 0.65 &&
				   Auto.IcmAware == 0.45 && Auto.OpenWidth == Sprint.OpenWidth && Auto.LimpRate == Sprint.LimpRate && Auto.ThreeBet == Sprint.ThreeBet && Auto.Cbet == Sprint.Cbet &&
				   Auto.Sizing == Sprint.Sizing && Auto.TiltProne == Sprint.TiltProne && Auto.ThinkBase == 0.0 && Auto.ThinkVar == 0.0 && Auto.Timing == ss::TimingStyle::Balanced,
			"update 2: style::ProfileOf(Autopilot) is the Sprint autopilot's numbers");
	}
	// 1. The plan's skip path with Autopilot is SimulateTick with Autopilot's profile, checkpoint for checkpoint.
	{
		std::unique_ptr<ss::Tournament> A = U2Field("u2-parity");
		std::unique_ptr<ss::Tournament> B = U2Field("u2-parity");
		bool Same = true;
		int Ticks = 0;
		for (; Ticks < 150 && !A->bFinished; ++Ticks)
		{
			A->SimulateTick(&Auto);
			const ss::Tournament::TickResult R = B->SimulateTickWith(Brain);
			Same = Same && !R.Paused && A->Checkpoint() == B->Checkpoint();
		}
		Expect(Same && Ticks > 100 && (Ticks == 150 || A->bFinished), "update 2: SimulateTickWith(Autopilot) plays SimulateTick(Autopilot)'s ticks, checkpoint for checkpoint");
	}
	// 2. LastHeroHand is the tick's hand at the hero's table, owned by the tournament: its seats and its history go on from a
	//    copy a stop rule took at the hero's last decision. A stop that never stops, and reading it, draw nothing.
	{
		std::unique_ptr<ss::Tournament> C = U2Field("u2-last");
		std::unique_ptr<ss::Tournament> D = U2Field("u2-last");
		std::unique_ptr<ss::Hand> Probe;
		const ss::Tournament::HeroStop Copy = [&Probe](const ss::Hand& H, const ss::BotDecision&) {
			Probe.reset(new ss::Hand(H));
			return false;
		};
		bool Owns = true;
		bool Same = true;
		int Hands = 0;
		for (int Tick = 0; Tick < 150 && !C->bFinished; ++Tick)
		{
			Probe.reset();
			C->SimulateTickWith(Brain, Copy);
			D->SimulateTickWith(Brain);
			const ss::Hand* Last = C->LastHeroHand();
			if (Probe)
			{
				++Hands;
				bool Seats = Last && Last->Seats.size() == Probe->Seats.size() && Last->History.size() >= Probe->History.size() && Last->bComplete;
				for (size_t K = 0; Seats && K < Probe->Seats.size(); ++K)
				{
					Seats = Last->Seats[K].Id == Probe->Seats[K].Id && Last->Seats[K].StartStack == Probe->Seats[K].StartStack;
				}
				Owns = Owns && Seats && std::equal(Probe->History.begin(), Probe->History.end(), Last->History.begin(), U2SameAction);
			}
			Same = Same && C->Checkpoint() == D->Checkpoint() && (C->LastHeroHand() != nullptr) == (D->LastHeroHand() != nullptr);
		}
		Expect(Hands > 20 && Owns, "update 2: LastHeroHand is the tick's hand (its seats, its history from the hero's last decision on)");
		Expect(Same, "update 2: a stop rule that never stops, and reading LastHeroHand, draw nothing");
	}
	// 3. A hand paused by a stop rule and finished the watch path's way (the decision the plan drew, then the plan, then
	//    BeginFinish, FinishSome, EndFinish) is the tick the skip path plays straight through.
	{
		std::unique_ptr<ss::Tournament> E = U2Field("u2-pause");
		std::unique_ptr<ss::Tournament> F = U2Field("u2-pause");
		ss::BotDecision Planned;
		const ss::Tournament::HeroStop Every = [&Planned](const ss::Hand&, const ss::BotDecision& D) {
			Planned = D;
			return true;
		};
		bool Same = true;
		int Paused = 0;
		for (int Tick = 0; Tick < 150 && !E->bFinished; ++Tick)
		{
			ss::Tournament::TickResult R = E->SimulateTickWith(Brain, Tick % 3 == 0 ? Every : ss::Tournament::HeroStop());
			if (R.Paused)
			{
				++Paused;
				Same = Same && E->Checkpoint() != F->Checkpoint(); // mid-tick: not the same state as a finished one
				R.Paused->Act(Planned.Action);
				E->PlayHeroHand(*R.Paused, Brain, ss::Tournament::HeroStop());
				E->BeginFinish(R.Paused.get());
				E->FinishSome(INT_MAX);
				E->EndFinish();
			}
			F->SimulateTickWith(Brain);
			Same = Same && E->Checkpoint() == F->Checkpoint();
		}
		Expect(Paused > 10 && Same, "update 2: a paused hand finished the watch path's way is the skip path's tick");
	}
	// 4. The caller's hand finished by the plan (StartTick, then FinishTickWith) is SimulateTickWith's tick.
	{
		std::unique_ptr<ss::Tournament> G = U2Field("u2-finish");
		std::unique_ptr<ss::Tournament> K = U2Field("u2-finish");
		bool Same = true;
		for (int Tick = 0; Tick < 150 && !G->bFinished; ++Tick)
		{
			std::vector<ss::TEvent> Events;
			std::unique_ptr<ss::Hand> H = G->StartTick(Events);
			G->FinishTickWith(H.get(), Brain);
			Same = Same && (!H || H->bComplete);
			K->SimulateTickWith(Brain);
			Same = Same && G->Checkpoint() == K->Checkpoint();
		}
		Expect(Same, "update 2: FinishTickWith finishes the caller's hand as SimulateTickWith would");
	}
	// 5. HeroReadHook: applied to the bots at the hero's table only, on both paths; null (every check above) changes nothing.
	{
		std::unique_ptr<ss::Tournament> T = U2Field("u2-read");
		int Calls = 0;
		bool HeroTable = true;
		T->HeroReadHook = [&Calls, &HeroTable](const ss::Hand& H, int Seat, ss::BotDecision& D) {
			(void)D;
			++Calls;
			bool Hero = false;
			for (const ss::HandSeat& S : H.Seats)
			{
				Hero = Hero || S.Id == ss::HeroId;
			}
			HeroTable = HeroTable && Hero && H.Seats[static_cast<size_t>(Seat)].Id != ss::HeroId;
		};
		for (int Tick = 0; Tick < 40 && !T->bFinished; ++Tick)
		{
			T->SimulateTickWith(Brain);
		}
		Expect(Calls > 0 && HeroTable, "update 2: HeroReadHook reads the bots at the hero's table, and only them");
	}
	// 6. A ck1 checkpoint written by bda07f3 (the day-0 staked fixture's Nightly, forty hands in) restores and writes back.
	{
		ss::SaveData D;
		const bool Parsed = ss::SaveData::Parse(U2ReadFile(std::string(SS_TESTS_DIR) + "/fixtures/pre_update2_staked_mid_nightly.ssave"), D);
		const ss::life::LiveEntry* E = Parsed ? ss::live::EntryFor(D.Life, "embercrest-nightly@9") : nullptr;
		bool Restored = false;
		std::unique_ptr<ss::Tournament> T;
		if (E)
		{
			std::vector<ss::ReservedPlayer> Known;
			for (const std::pair<std::string, int>& R : E->Roster)
			{
				ss::ReservedPlayer P;
				P.Name = R.first;
				P.Type = static_cast<ss::Archetype>(R.second);
				Known.push_back(P);
			}
			T = ss::live::ResumeField(ss::live::FindOccurrence(E->Id), *E, E->Entrants, D.HeroName, Known, true, Restored);
		}
		Expect(E && !E->Checkpoint.empty() && E->Checkpoint.rfind("ck1 ", 0) == 0 && Restored && T && T->Checkpoint() == E->Checkpoint,
			"update 2: bda07f3's ck1 checkpoint restores and writes back as it was");
	}
}

namespace
{
/** Structure's Icm() as it was before g1-audit N7 (placed players in a 32-bit mask): the reference the fix must match below 33. */
void IcmMask32Recurse(const std::vector<double>& Stacks, const std::vector<double>& Prizes, std::vector<double>& Out, int N, int Places, uint32_t Mask, int Depth, double Prob,
	double Remaining)
{
	if (Depth >= Places)
	{
		return;
	}
	for (int I = 0; I < N; ++I)
	{
		if (Mask & (1u << I))
		{
			continue;
		}
		const double Stack = Stacks[static_cast<size_t>(I)];
		const double P = (Stack / Remaining) * Prob;
		if (P == 0.0)
		{
			continue;
		}
		Out[static_cast<size_t>(I)] += P * Prizes[static_cast<size_t>(Depth)];
		IcmMask32Recurse(Stacks, Prizes, Out, N, Places, Mask | (1u << I), Depth + 1, P, Remaining - Stack);
	}
}

std::vector<double> IcmMask32(const std::vector<double>& Stacks, const std::vector<double>& Prizes)
{
	const int N = static_cast<int>(Stacks.size());
	double Total = 0.0;
	for (double S : Stacks)
	{
		Total += S;
	}
	std::vector<double> Out(static_cast<size_t>(N), 0.0);
	const int Places = static_cast<int>(Prizes.size()) < N ? static_cast<int>(Prizes.size()) : N;
	if (N == 0 || Total == 0.0 || N > 32)
	{
		return Out;
	}
	IcmMask32Recurse(Stacks, Prizes, Out, N, Places, 0u, 0, 1.0, Total);
	return Out;
}

/**
 * g1-audit N7: Icm()'s exact path past the 32nd player. Up to 32 players the per-player flags change no bit (every places count
 * to 9 left and the hardest exact cases beyond, against the old 32-bit mask); at 40 left paying 3 and 70 paying 2 (inside the
 * two-million-order range) the shares add up to the prizes and equal stacks get equal shares wherever they sit.
 */
void IcmPastThirtyTwo()
{
	uint64_t Seed = 0x2545f4914f6cdd1dull;
	const auto Next = [&Seed]() {
		Seed ^= Seed << 13;
		Seed ^= Seed >> 7;
		Seed ^= Seed << 17;
		return Seed;
	};
	int Compared = 0;
	int Moved = 0;
	const int Shapes[][2] = {{2, 1}, {3, 2}, {4, 4}, {5, 3}, {6, 6}, {7, 5}, {8, 8}, {9, 9}, {9, 4}, {10, 8}, {12, 6}, {20, 4}, {32, 3}, {32, 2}};
	for (const auto& Shape : Shapes)
	{
		for (int Set = 0; Set < 3; ++Set)
		{
			std::vector<double> Stacks;
			for (int I = 0; I < Shape[0]; ++I)
			{
				Stacks.push_back(Set == 1 ? 1000.0 * static_cast<double>(1 + Next() % 3) : static_cast<double>(1 + Next() % 100000));
			}
			std::vector<double> Prizes;
			for (int I = 0; I < Shape[1]; ++I)
			{
				Prizes.push_back(std::floor(500000.0 / (1.0 + 0.7 * static_cast<double>(I))));
			}
			const std::vector<double> New = ss::Icm(Stacks, Prizes);
			const std::vector<double> Old = IcmMask32(Stacks, Prizes);
			Compared += static_cast<int>(New.size());
			Moved += New.size() == Old.size() && std::memcmp(New.data(), Old.data(), New.size() * sizeof(double)) == 0 ? 0 : 1;
		}
	}
	Expect(Compared > 300 && Moved == 0, "icm: per-player flags give the old 32-bit mask's equities to the bit, up to 32 players");
	for (const int Left : {40, 70})
	{
		std::vector<double> Stacks;
		for (int I = 0; I < Left; ++I)
		{
			Stacks.push_back(1000.0 + 500.0 * static_cast<double>(I % 8)); // seat I and seat I + 32 hold the same stack
		}
		const std::vector<double> Prizes = Left == 40 ? std::vector<double>{300000.0, 180000.0, 120000.0} : std::vector<double>{300000.0, 180000.0};
		const std::vector<double> Eq = ss::Icm(Stacks, Prizes);
		double Sum = 0.0;
		bool Equal = Eq.size() == Stacks.size();
		for (size_t I = 0; I < Eq.size(); ++I)
		{
			Sum += Eq[I];
			Equal = Equal && (I < 32 || std::fabs(Eq[I] - Eq[I - 32]) <= 1e-6 * Eq[I]);
		}
		const double Want = Left == 40 ? 600000.0 : 480000.0;
		std::printf("  icm: %d left paying %zu shares out %.4f of %.4f cents\n", Left, Prizes.size(), Sum, Want);
		Expect(std::fabs(Sum - Want) < 1e-4 && Equal, "icm: past the 32nd player every player is kept apart (the shares add up; equal stacks, equal shares)");
	}
}
} // namespace

int main()
{
	ChipConservationFuzz();
	DeckStubForShow();
	IllegalActionsRejected();
	TournamentRunsToCompletion();
	ChipSetLayout();
	Update2Engine();
	IcmPastThirtyTwo();
	std::printf("unit tests: %s\n", Failures == 0 ? "all passed" : "FAILED");
	return Failures == 0 ? 0 : 1;
}
