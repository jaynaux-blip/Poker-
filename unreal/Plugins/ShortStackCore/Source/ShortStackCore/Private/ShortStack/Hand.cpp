#include "ShortStack/Hand.h"
#include "StrictFloat.h"

#include "ShortStack/Cards.h"
#include "ShortStack/Evaluator.h"
#include "ShortStack/Rng.h"

#include <algorithm>
#include <cmath>

namespace ss
{
Hand::Hand(const HandConfig& Config, Rng& R)
{
	if (Config.Players.size() < 2)
	{
		bValid = false;
		bComplete = true;
		return;
	}
	std::vector<SeatInput> Sorted = Config.Players;
	std::stable_sort(Sorted.begin(), Sorted.end(), [](const SeatInput& X, const SeatInput& Y) { return X.Seat < Y.Seat; });
	for (const SeatInput& P : Sorted)
	{
		HandSeat S;
		S.Seat = P.Seat;
		S.Id = P.Id;
		S.StartStack = P.Stack;
		S.Stack = P.Stack;
		Seats.push_back(S);
	}
	SmallBlind = Config.SmallBlind;
	BigBlind = Config.BigBlind;
	Ante = Config.Ante;
	ButtonSeat = Config.ButtonSeat;
	ButtonIdx = -1;
	for (size_t I = 0; I < Seats.size(); ++I)
	{
		if (Seats[I].Seat == Config.ButtonSeat)
		{
			ButtonIdx = static_cast<int>(I);
			break;
		}
	}
	if (ButtonIdx < 0)
	{
		bValid = false;
		bComplete = true;
		return;
	}
	if (!Config.Deck.empty())
	{
		Deck = Config.Deck;
	}
	else
	{
		Deck = FullDeck();
		R.Shuffle(Deck);
	}
	Start();
}

// ---------------------------------------------------------------- queries

Chips Hand::Pot() const
{
	Chips P = 0;
	for (const HandSeat& S : Seats)
	{
		P += S.Committed;
	}
	return P;
}

Chips Hand::PotBeforeStreet() const
{
	Chips P = 0;
	for (const HandSeat& S : Seats)
	{
		P += S.Committed - S.StreetBet;
	}
	return P;
}

const HandSeat* Hand::SeatByNumber(int SeatNumber) const
{
	for (const HandSeat& S : Seats)
	{
		if (S.Seat == SeatNumber)
		{
			return &S;
		}
	}
	return nullptr;
}

int Hand::ActiveCount() const
{
	int N = 0;
	for (const HandSeat& S : Seats)
	{
		if (!S.Folded)
		{
			++N;
		}
	}
	return N;
}

int Hand::AbleCount() const
{
	int N = 0;
	for (const HandSeat& S : Seats)
	{
		if (!S.Folded && !S.AllIn)
		{
			++N;
		}
	}
	return N;
}

LegalActions Hand::GetLegalActions() const
{
	LegalActions L;
	if (ToAct < 0)
	{
		return L;
	}
	const HandSeat& S = Seats[static_cast<size_t>(ToAct)];
	const Chips ToCall = MaxChips(0, CurrentBet - S.StreetBet);
	L.CallAmount = MinChips(ToCall, S.Stack);
	L.MaxRaiseTo = S.StreetBet + S.Stack;
	const Chips FullMin = CurrentBet + MinRaiseInc;
	bool OthersAble = false;
	for (size_t I = 0; I < Seats.size(); ++I)
	{
		if (static_cast<int>(I) != ToAct && !Seats[I].Folded && !Seats[I].AllIn)
		{
			OthersAble = true;
			break;
		}
	}
	L.CanFold = true;
	L.CanCheck = ToCall == 0;
	L.CanRaise = !S.RaiseLocked && L.MaxRaiseTo > CurrentBet && OthersAble;
	L.MinRaiseTo = MinChips(FullMin, L.MaxRaiseTo);
	L.IsBet = CurrentBet == 0;
	return L;
}

// ---------------------------------------------------------------- actions

bool Hand::Act(const PlayerAction& Action)
{
	if (bComplete || ToAct < 0)
	{
		return false;
	}
	const int Idx = ToAct;
	HandSeat& S = Seats[static_cast<size_t>(Idx)];
	const LegalActions Legal = GetLegalActions();
	const Chips PotBefore = Pot();
	const Chips Facing = Legal.CallAmount;
	ActionType Type = ActionType::Fold;
	Chips Added = 0;

	switch (Action.Type)
	{
	case PlayerAction::Kind::Fold:
		S.Folded = true;
		Type = ActionType::Fold;
		break;
	case PlayerAction::Kind::Check:
		if (!Legal.CanCheck)
		{
			return false;
		}
		Type = ActionType::Check;
		break;
	case PlayerAction::Kind::Call:
		if (Legal.CallAmount == 0)
		{
			Type = ActionType::Check;
			break;
		}
		Added = Commit(S, Legal.CallAmount);
		Type = ActionType::Call;
		break;
	case PlayerAction::Kind::Raise:
	{
		if (!Legal.CanRaise)
		{
			return false;
		}
		const Chips Floored = static_cast<Chips>(std::floor(Action.To));
		const Chips To = MinChips(Floored, Legal.MaxRaiseTo);
		if (To <= CurrentBet)
		{
			return false;
		}
		const bool IsAllIn = To == Legal.MaxRaiseTo;
		if (To < Legal.MinRaiseTo && !IsAllIn)
		{
			return false;
		}
		Type = CurrentBet == 0 ? ActionType::Bet : ActionType::Raise;
		const Chips RaiseSize = To - CurrentBet;
		Added = Commit(S, To - S.StreetBet);
		if (RaiseSize >= MinRaiseInc)
		{
			// Full raise: reopens the betting for everyone.
			MinRaiseInc = RaiseSize;
			for (size_t I = 0; I < Seats.size(); ++I)
			{
				if (static_cast<int>(I) == Idx)
				{
					continue;
				}
				HandSeat& O = Seats[I];
				if (!O.Folded && !O.AllIn)
				{
					O.HasActed = false;
					O.RaiseLocked = false;
				}
			}
		}
		else
		{
			// Incomplete all-in raise: players who already acted may only call or fold.
			for (size_t I = 0; I < Seats.size(); ++I)
			{
				if (static_cast<int>(I) == Idx)
				{
					continue;
				}
				HandSeat& O = Seats[I];
				if (!O.Folded && !O.AllIn && O.HasActed)
				{
					O.HasActed = false;
					O.RaiseLocked = true;
				}
			}
		}
		CurrentBet = To;
		LastAggressor = Idx;
		break;
	}
	}

	S.HasActed = true;
	S.RaiseLocked = false;
	ActionRecord Rec;
	Rec.OnStreet = CurrentStreet;
	Rec.Seat = S.Seat;
	Rec.Type = Type;
	Rec.Added = Added;
	Rec.To = S.StreetBet;
	Rec.AllIn = S.AllIn;
	Rec.PotBefore = PotBefore;
	Rec.Facing = Facing;
	History.push_back(Rec);
	HandEvent E;
	E.Type = EventType::Action;
	E.Action = Rec;
	Events.push_back(E);
	Advance();
	return true;
}

// ---------------------------------------------------------------- internals

Chips Hand::Commit(HandSeat& S, Chips Amount)
{
	const Chips A = MinChips(Amount, S.Stack);
	S.Stack -= A;
	S.StreetBet += A;
	S.Committed += A;
	if (S.Stack == 0)
	{
		S.AllIn = true;
	}
	return A;
}

int Hand::NextIdx(int From) const
{
	return (From + 1) % static_cast<int>(Seats.size());
}

void Hand::Start()
{
	const int N = static_cast<int>(Seats.size());
	if (N == 2)
	{
		// Heads-up: the button posts the small blind and acts first preflop.
		SbIdx = ButtonIdx;
		BbIdx = NextIdx(ButtonIdx);
	}
	else
	{
		SbIdx = NextIdx(ButtonIdx);
		BbIdx = NextIdx(SbIdx);
	}
	HandSeat& Sb = Seats[static_cast<size_t>(SbIdx)];
	HandSeat& Bb = Seats[static_cast<size_t>(BbIdx)];
	{
		const Chips Amt = Commit(Sb, SmallBlind);
		HandEvent E;
		E.Type = EventType::Blind;
		E.Seat = Sb.Seat;
		E.Amount = Amt;
		E.IsSmallBlind = true;
		Events.push_back(E);
	}
	{
		const Chips Amt = Commit(Bb, BigBlind);
		HandEvent E;
		E.Type = EventType::Blind;
		E.Seat = Bb.Seat;
		E.Amount = Amt;
		E.IsSmallBlind = false;
		Events.push_back(E);
	}
	if (Ante > 0 && Bb.Stack > 0)
	{
		// Big-blind ante is dead money: it never counts toward the BB's bet.
		const Chips A = MinChips(Ante, Bb.Stack);
		Bb.Stack -= A;
		Bb.Committed += A;
		Bb.AnteCommitted += A;
		if (Bb.Stack == 0)
		{
			Bb.AllIn = true;
		}
		HandEvent E;
		E.Type = EventType::Ante;
		E.Seat = Bb.Seat;
		E.Amount = A;
		Events.push_back(E);
	}
	CurrentBet = BigBlind;
	MinRaiseInc = BigBlind;

	// Deal two cards each, one at a time, starting left of the button.
	std::vector<int> Order;
	for (int I = 1; I <= N; ++I)
	{
		Order.push_back((ButtonIdx + I) % N);
	}
	for (int Round = 0; Round < 2; ++Round)
	{
		for (int I : Order)
		{
			const Card C = Draw();
			Seats[static_cast<size_t>(I)].Hole.push_back(C);
		}
	}
	HandEvent DealEvent;
	DealEvent.Type = EventType::Deal;
	for (int I : Order)
	{
		DealEvent.Seats.push_back(Seats[static_cast<size_t>(I)].Seat);
	}
	Events.push_back(DealEvent);

	// First to act preflop: left of the big blind.
	ToAct = FindNextToAct(BbIdx);
	if (ToAct < 0)
	{
		FinishStreet();
	}
}

Card Hand::Draw()
{
	return Deck[static_cast<size_t>(DeckPos++)];
}

int Hand::FindNextToAct(int From) const
{
	const int N = static_cast<int>(Seats.size());
	for (int K = 1; K <= N; ++K)
	{
		const int I = (From + K) % N;
		const HandSeat& S = Seats[static_cast<size_t>(I)];
		if (S.Folded || S.AllIn)
		{
			continue;
		}
		if (!S.HasActed || S.StreetBet < CurrentBet)
		{
			return I;
		}
	}
	return -1;
}

void Hand::Advance()
{
	if (ActiveCount() == 1)
	{
		ReturnUncalled();
		AwardUncontested();
		return;
	}
	const int Next = FindNextToAct(ToAct);
	if (Next >= 0)
	{
		ToAct = Next;
		return;
	}
	FinishStreet();
}

void Hand::FinishStreet()
{
	ToAct = -1;
	ReturnUncalled();
	for (HandSeat& S : Seats)
	{
		S.StreetBet = 0;
		S.HasActed = false;
		S.RaiseLocked = false;
	}
	CurrentBet = 0;
	MinRaiseInc = BigBlind;

	if (CurrentStreet == Street::River)
	{
		DoShowdown();
		return;
	}

	// If at most one player can still bet, the rest of the board just runs out.
	const bool Runout = AbleCount() <= 1;
	if (Runout && !bRevealed)
	{
		bRevealed = true;
		HandEvent E;
		E.Type = EventType::Reveal;
		for (const HandSeat& S : Seats)
		{
			if (!S.Folded)
			{
				E.Seats.push_back(S.Seat);
			}
		}
		Events.push_back(E);
	}

	DealNextStreet();
	if (Runout)
	{
		FinishStreet();
		return;
	}
	// Postflop, the first active player left of the button acts first.
	ToAct = FindNextToAct(ButtonIdx);
	if (ToAct < 0)
	{
		FinishStreet();
	}
}

void Hand::DealNextStreet()
{
	Draw(); // burn
	std::vector<Card> Dealt;
	if (CurrentStreet == Street::Preflop)
	{
		for (int I = 0; I < 3; ++I)
		{
			Dealt.push_back(Draw());
		}
		CurrentStreet = Street::Flop;
	}
	else if (CurrentStreet == Street::Flop)
	{
		Dealt.push_back(Draw());
		CurrentStreet = Street::Turn;
	}
	else
	{
		Dealt.push_back(Draw());
		CurrentStreet = Street::River;
	}
	Board.insert(Board.end(), Dealt.begin(), Dealt.end());
	HandEvent E;
	E.Type = EventType::Street;
	E.NewStreet = CurrentStreet;
	E.Cards = Dealt;
	E.Board = Board;
	Events.push_back(E);
}

void Hand::ReturnUncalled()
{
	// The highest street bet beyond what anyone else matched goes back.
	int Top = -1;
	Chips TopBet = -1;
	Chips Second = 0;
	for (size_t I = 0; I < Seats.size(); ++I)
	{
		const Chips B = Seats[I].StreetBet;
		if (B > TopBet)
		{
			Second = MaxChips(Second, TopBet);
			TopBet = B;
			Top = static_cast<int>(I);
		}
		else if (B > Second)
		{
			Second = B;
		}
	}
	if (Top >= 0 && TopBet > Second)
	{
		HandSeat& S = Seats[static_cast<size_t>(Top)];
		const Chips Back = TopBet - Second;
		S.Stack += Back;
		S.StreetBet -= Back;
		S.Committed -= Back;
		if (S.Stack > 0)
		{
			S.AllIn = false;
		}
		HandEvent E;
		E.Type = EventType::Return;
		E.Seat = S.Seat;
		E.Amount = Back;
		Events.push_back(E);
	}
}

void Hand::AwardUncontested()
{
	ToAct = -1;
	HandSeat* Winner = nullptr;
	for (HandSeat& S : Seats)
	{
		if (!S.Folded)
		{
			Winner = &S;
			break;
		}
	}
	SS_ASSERT(Winner != nullptr);
	const Chips Amount = Pot();
	PotResult P;
	P.Amount = Amount;
	P.Eligible.push_back(Winner->Seat);
	P.Winners.push_back(Winner->Seat);
	P.Shares.push_back(Amount);
	Winner->Stack += Amount;
	PotResults.clear();
	PotResults.push_back(P);
	HandEvent E;
	E.Type = EventType::Award;
	E.Pot = P;
	E.PotIndex = 0;
	E.PotCount = 1;
	Events.push_back(E);
	End();
}

std::vector<PotResult> Hand::BuildPots() const
{
	std::vector<Chips> Remaining;
	Chips Antes = 0;
	for (const HandSeat& S : Seats)
	{
		Remaining.push_back(S.Committed - S.AnteCommitted);
		Antes += S.AnteCommitted;
	}
	std::vector<PotResult> Pots;
	for (;;)
	{
		bool Found = false;
		Chips Level = 0;
		for (size_t I = 0; I < Seats.size(); ++I)
		{
			if (!Seats[I].Folded && Remaining[I] > 0 && (!Found || Remaining[I] < Level))
			{
				Level = Remaining[I];
				Found = true;
			}
		}
		if (!Found)
		{
			break;
		}
		PotResult P;
		for (size_t I = 0; I < Seats.size(); ++I)
		{
			const Chips Take = MinChips(Remaining[I], Level);
			if (Take > 0)
			{
				P.Amount += Take;
				Remaining[I] -= Take;
				if (!Seats[I].Folded)
				{
					P.Eligible.push_back(Seats[I].Seat);
				}
			}
		}
		Pots.push_back(P);
	}
	// Leftover chips from folded players (rare) join the last pot.
	Chips Leftover = 0;
	for (Chips V : Remaining)
	{
		Leftover += V;
	}
	if (Pots.empty())
	{
		PotResult P;
		P.Amount = Antes + Leftover;
		for (const HandSeat& S : Seats)
		{
			if (!S.Folded)
			{
				P.Eligible.push_back(S.Seat);
			}
		}
		Pots.push_back(P);
	}
	else
	{
		Pots.front().Amount += Antes;
		Pots.back().Amount += Leftover;
	}
	return Pots;
}

void Hand::DoShowdown()
{
	CurrentStreet = Street::Showdown;
	ToAct = -1;
	HandEvent Sd;
	Sd.Type = EventType::Showdown;
	std::vector<std::pair<int, int>> Scores; // seat, score
	for (const HandSeat& S : Seats)
	{
		if (S.Folded)
		{
			continue;
		}
		const int Score = EvaluateHand(S.Hole, Board);
		Scores.emplace_back(S.Seat, Score);
		ShowdownHand H;
		H.Seat = S.Seat;
		H.Hole = S.Hole;
		H.Score = Score;
		Sd.Hands.push_back(H);
	}
	Events.push_back(Sd);
	auto ScoreOf = [&](int SeatNumber) {
		for (const auto& P : Scores)
		{
			if (P.first == SeatNumber)
			{
				return P.second;
			}
		}
		return -1;
	};

	const std::vector<PotResult> Pots = BuildPots();
	const int N = static_cast<int>(Seats.size());
	auto LeftOfButton = [&](int SeatNumber) {
		int I = 0;
		for (int K = 0; K < N; ++K)
		{
			if (Seats[static_cast<size_t>(K)].Seat == SeatNumber)
			{
				I = K;
				break;
			}
		}
		return (I - ButtonIdx - 1 + N) % N;
	};
	PotResults.clear();
	for (size_t Index = 0; Index < Pots.size(); ++Index)
	{
		const PotResult& P = Pots[Index];
		int Best = -1;
		for (int SeatNumber : P.Eligible)
		{
			const int Score = ScoreOf(SeatNumber);
			Best = Score > Best ? Score : Best;
		}
		std::vector<int> Winners;
		for (int SeatNumber : P.Eligible)
		{
			if (ScoreOf(SeatNumber) == Best)
			{
				Winners.push_back(SeatNumber);
			}
		}
		std::stable_sort(Winners.begin(), Winners.end(), [&](int X, int Y) { return LeftOfButton(X) < LeftOfButton(Y); });
		const Chips Count = static_cast<Chips>(Winners.size());
		const Chips Base = P.Amount / Count; // amounts are positive: same as Math.floor
		Chips Odd = P.Amount - Base * Count;
		PotResult Result;
		Result.Amount = P.Amount;
		Result.Eligible = P.Eligible;
		Result.Winners = Winners;
		for (size_t K = 0; K < Winners.size(); ++K)
		{
			Result.Shares.push_back(Base + (Odd > 0 ? 1 : 0));
			--Odd;
		}
		for (size_t K = 0; K < Winners.size(); ++K)
		{
			for (HandSeat& S : Seats)
			{
				if (S.Seat == Winners[K])
				{
					S.Stack += Result.Shares[K];
				}
			}
		}
		PotResults.push_back(Result);
		HandEvent E;
		E.Type = EventType::Award;
		E.Pot = Result;
		E.PotIndex = static_cast<int>(Index);
		E.PotCount = static_cast<int>(Pots.size());
		Events.push_back(E);
	}
	End();
}

void Hand::End()
{
	bComplete = true;
	ToAct = -1;
	HandEvent E;
	E.Type = EventType::End;
	Events.push_back(E);
}
} // namespace ss
