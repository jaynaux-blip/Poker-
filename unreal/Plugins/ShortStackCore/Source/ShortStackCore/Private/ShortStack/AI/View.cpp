#include "ShortStack/AI/View.h"
#include "../StrictFloat.h"

namespace ss
{
PlayerView MakeView(const Hand& H, int SeatIdx)
{
	const LegalActions Legal = H.GetLegalActions();
	PlayerView V;
	V.Me = SeatIdx;
	V.Hole = H.Seats[static_cast<size_t>(SeatIdx)].Hole;
	for (const HandSeat& S : H.Seats)
	{
		PublicSeat P;
		P.Seat = S.Seat;
		P.Id = S.Id;
		P.Stack = S.Stack;
		P.StreetBet = S.StreetBet;
		P.Committed = S.Committed;
		P.Folded = S.Folded;
		P.AllIn = S.AllIn;
		V.Seats.push_back(P);
	}
	V.Board = H.Board;
	V.CurrentStreet = H.CurrentStreet;
	V.History = H.History;
	V.ButtonIdx = H.ButtonIndex();
	V.BbIdx = H.BigBlindIndex();
	V.SbIdx = H.SmallBlindIndex();
	V.SmallBlind = H.SmallBlind;
	V.BigBlind = H.BigBlind;
	V.Ante = H.Ante;
	V.Pot = H.Pot();
	V.CurrentBet = H.CurrentBet;
	V.ToCall = Legal.CallAmount;
	V.MinRaiseTo = Legal.MinRaiseTo;
	V.MaxRaiseTo = Legal.MaxRaiseTo;
	V.CanRaise = Legal.CanRaise;
	V.CanCheck = Legal.CanCheck;
	return V;
}

const char* PositionName(Position P)
{
	switch (P)
	{
	case Position::BTN: return "BTN";
	case Position::SB: return "SB";
	case Position::BB: return "BB";
	case Position::UTG: return "UTG";
	case Position::UTG1: return "UTG+1";
	case Position::MP: return "MP";
	case Position::LJ: return "LJ";
	case Position::HJ: return "HJ";
	default: return "CO";
	}
}

Position PositionOf(const PlayerView& View, int Idx)
{
	if (Idx == View.ButtonIdx)
	{
		return Position::BTN;
	}
	if (Idx == View.SbIdx)
	{
		return Position::SB;
	}
	if (Idx == View.BbIdx)
	{
		return Position::BB;
	}
	const int N = static_cast<int>(View.Seats.size());
	// Seats between the big blind and the button, counted back from the button.
	const int FromButton = (View.ButtonIdx - Idx + N) % N; // 1 = cutoff
	static const Position Names[6] = {Position::CO, Position::HJ, Position::LJ, Position::MP, Position::UTG1, Position::UTG};
	const int Early = (Idx - View.BbIdx + N) % N; // 1 = first to act
	if (Early == 1 && N >= 4)
	{
		return Position::UTG;
	}
	const int K = FromButton - 1 < 5 ? FromButton - 1 : 5;
	return Names[K];
}

int PlayersBehind(const PlayerView& View, int Idx)
{
	if (Idx == View.BbIdx)
	{
		return 0;
	}
	const int N = static_cast<int>(View.Seats.size());
	int Count = 0;
	for (int K = 1; K < N; ++K)
	{
		const int I = (Idx + K) % N;
		const PublicSeat& S = View.Seats[static_cast<size_t>(I)];
		if (!S.Folded && !S.AllIn)
		{
			++Count;
		}
		// The big blind is the last to act preflop.
		if (I == View.BbIdx)
		{
			break;
		}
	}
	return Count;
}

StreetSummary PreflopSummary(const PlayerView& View)
{
	StreetSummary Sum;
	Sum.LastRaiseTo = View.BigBlind;
	for (const ActionRecord& H : View.History)
	{
		if (H.OnStreet != Street::Preflop)
		{
			continue;
		}
		if (H.Type == ActionType::Raise || H.Type == ActionType::Bet)
		{
			++Sum.Raises;
			Sum.LastRaiserSeat = H.Seat;
			Sum.LastRaiseTo = H.To;
		}
		else if (H.Type == ActionType::Call && Sum.Raises == 0)
		{
			++Sum.Limpers;
		}
	}
	return Sum;
}

int SeatIndex(const PlayerView& View, int Seat)
{
	for (size_t I = 0; I < View.Seats.size(); ++I)
	{
		if (View.Seats[I].Seat == Seat)
		{
			return static_cast<int>(I);
		}
	}
	return -1;
}
} // namespace ss
