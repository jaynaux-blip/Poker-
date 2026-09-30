#include "ShortStack/AI/Model.h"
#include "../StrictFloat.h"

#include "ShortStack/Cards.h"
#include "ShortStack/Evaluator.h"
#include "ShortStack/Rng.h"

#include <cstring>

namespace ss
{
namespace model_detail
{
double OpenBand(Position P)
{
	switch (P)
	{
	case Position::UTG: return 0.14;
	case Position::UTG1: return 0.16;
	case Position::MP: return 0.18;
	case Position::LJ: return 0.21;
	case Position::HJ: return 0.25;
	case Position::CO: return 0.32;
	case Position::BTN: return 0.45;
	case Position::SB: return 0.4;
	default: return 0.3; // BB
	}
}

RangeBand Band(double Min, double Max)
{
	RangeBand B;
	B.Min = Min;
	B.Max = Max;
	return B;
}

bool Accepts(const OpponentModel& M, Card A, Card B, const std::vector<Card>& Board, int BoardCat, double R)
{
	const double P = ClassPercentile()[HandClass(A, B)];
	if (P <= M.Band.Min || P > M.Band.Max)
	{
		return false;
	}
	if (Board.empty() || (M.Aggr == 0 && M.Calls == 0))
	{
		return true;
	}
	const int Made = MadeLevel(A, B, Board, BoardCat);
	const bool Draw = Made == 0 && HasDraw(A, B, Board);
	if (M.Aggr >= 2)
	{
		return Made == 2 || (Made == 1 && R < 0.55) || (Draw && R < 0.35) || R < 0.08;
	}
	if (M.Aggr == 1)
	{
		return Made >= 1 || (Draw && R < 0.8) || R < 0.22;
	}
	return Made >= 1 || Draw || R < 0.25;
}
} // namespace model_detail

std::vector<OpponentModel> ModelOpponents(const PlayerView& View)
{
	std::vector<OpponentModel> Models;
	for (size_t I = 0; I < View.Seats.size(); ++I)
	{
		if (static_cast<int>(I) == View.Me)
		{
			continue;
		}
		const PublicSeat& S = View.Seats[I];
		if (S.Folded)
		{
			continue;
		}
		Models.push_back(ModelSeat(View, S.Seat));
	}
	return Models;
}

OpponentModel ModelSeat(const PlayerView& View, int Seat)
{
	using model_detail::Band;
	const int Idx = SeatIndex(View, Seat);
	OpponentModel M;
	M.Seat = Seat;
	M.Band = Band(0.0, 1.0);
	int RaisesSeen = 0;
	const double BbStack = Max(1.0, static_cast<double>(View.BigBlind));

	for (const ActionRecord& H : View.History)
	{
		if (H.OnStreet == Street::Preflop)
		{
			const bool IsRaise = H.Type == ActionType::Raise || H.Type == ActionType::Bet;
			if (H.Seat == Seat)
			{
				if (IsRaise)
				{
					if (RaisesSeen == 0)
					{
						// Open-raise. Short-stack shoves are wider than opens.
						const double Open = model_detail::OpenBand(PositionOf(View, Idx));
						const double ShoveWide = H.AllIn && static_cast<double>(H.To) / BbStack <= 15.0 ? 0.1 : 0.0;
						M.Band = Band(0.0, Open + ShoveWide);
					}
					else if (RaisesSeen == 1)
					{
						M.Band = Band(0.0, H.AllIn ? 0.12 : 0.07);
					}
					else
					{
						M.Band = Band(0.0, H.AllIn ? 0.06 : 0.03);
					}
				}
				else if (H.Type == ActionType::Call)
				{
					if (RaisesSeen == 0)
					{
						M.Band = Idx == View.SbIdx ? Band(0.03, 0.8) : Band(0.04, 0.65);
					}
					else if (RaisesSeen == 1)
					{
						M.Band = Idx == View.BbIdx ? Band(0.03, 0.55) : Band(0.015, 0.35);
					}
					else
					{
						M.Band = Band(0.005, 0.12);
					}
				}
			}
			if (IsRaise)
			{
				++RaisesSeen;
			}
		}
		else if (H.Seat == Seat)
		{
			if (H.Type == ActionType::Bet || H.Type == ActionType::Raise)
			{
				++M.Aggr;
			}
			else if (H.Type == ActionType::Call)
			{
				++M.Calls;
			}
		}
	}
	return M;
}

int MadeLevel(Card A, Card B, const std::vector<Card>& Board, int BoardCat)
{
	Card Scratch[7];
	Scratch[0] = A;
	Scratch[1] = B;
	for (size_t I = 0; I < Board.size(); ++I)
	{
		Scratch[2 + I] = Board[I];
	}
	const int Cat = Evaluate(Scratch, 2 + static_cast<int>(Board.size())) >> 20;
	if (Cat <= BoardCat)
	{
		return 0;
	}
	if (Cat >= static_cast<int>(Category::Trips))
	{
		return 2;
	}
	if (Cat == static_cast<int>(Category::TwoPair) && BoardCat == static_cast<int>(Category::HighCard))
	{
		return 2;
	}
	return 1;
}

bool HasDraw(Card A, Card B, const std::vector<Card>& Board)
{
	if (Board.size() >= 5)
	{
		return false;
	}
	int SuitCount[4] = {0, 0, 0, 0};
	int Mask = 0;
	++SuitCount[A & 3];
	++SuitCount[B & 3];
	Mask |= (1 << (A >> 2)) | (1 << (B >> 2));
	for (Card C : Board)
	{
		++SuitCount[C & 3];
		Mask |= 1 << (C >> 2);
	}
	for (int S = 0; S < 4; ++S)
	{
		if (SuitCount[S] == 4 && ((A & 3) == S || (B & 3) == S))
		{
			return true;
		}
	}
	if (StraightHigh(Mask) >= 0)
	{
		return false;
	}
	int Outs = 0;
	for (int R = 0; R < 13; ++R)
	{
		if (!(Mask & (1 << R)) && StraightHigh(Mask | (1 << R)) >= 0)
		{
			++Outs;
		}
	}
	return Outs >= 2;
}

int BoardCategory(const std::vector<Card>& Board)
{
	if (Board.empty())
	{
		return 0;
	}
	Card Scratch[7];
	for (size_t I = 0; I < Board.size(); ++I)
	{
		Scratch[I] = Board[I];
	}
	return Evaluate(Scratch, static_cast<int>(Board.size())) >> 20;
}

double EquityVsModels(const std::vector<Card>& Hole, const std::vector<Card>& Board, const std::vector<OpponentModel>& Models, int Iterations, Rng& R)
{
	if (Models.empty())
	{
		return 1.0;
	}
	unsigned char Used[52];
	unsigned char Dead[52] = {0};
	for (Card C : Hole)
	{
		Dead[C] = 1;
	}
	for (Card C : Board)
	{
		Dead[C] = 1;
	}
	std::vector<Card> Live;
	for (int C = 0; C < 52; ++C)
	{
		if (!Dead[C])
		{
			Live.push_back(C);
		}
	}
	const int LiveN = static_cast<int>(Live.size());
	auto Draw = [&]() { return Live[static_cast<size_t>(R.Int(LiveN))]; };
	const int BoardCat = BoardCategory(Board);
	Card Cards[7];
	std::vector<Card> Opp(Models.size() * 2);
	double Won = 0.0;

	for (int It = 0; It < Iterations; ++It)
	{
		std::memset(Used, 0, sizeof(Used));
		for (size_t O = 0; O < Models.size(); ++O)
		{
			Card A = -1;
			Card B = -1;
			for (int Tries = 0; Tries < 80; ++Tries)
			{
				const Card X = Draw();
				const Card Y = Draw();
				if (X == Y || Used[X] || Used[Y])
				{
					continue;
				}
				A = X;
				B = Y;
				const double Roll = R.Next();
				if (model_detail::Accepts(Models[O], X, Y, Board, BoardCat, Roll))
				{
					break;
				}
			}
			if (A < 0)
			{
				do
				{
					A = Draw();
				} while (Used[A]);
				do
				{
					B = Draw();
				} while (Used[B] || B == A);
			}
			Used[A] = 1;
			Used[B] = 1;
			Opp[O * 2] = A;
			Opp[O * 2 + 1] = B;
		}
		int N = 2;
		for (Card C : Board)
		{
			Cards[N++] = C;
		}
		while (N < 7)
		{
			const Card C = Draw();
			if (Used[C])
			{
				continue;
			}
			Used[C] = 1;
			Cards[N++] = C;
		}
		Cards[0] = Hole[0];
		Cards[1] = Hole[1];
		const int Hs = Evaluate(Cards, 7);
		int Ties = 1;
		bool Lost = false;
		for (size_t O = 0; O < Models.size(); ++O)
		{
			Cards[0] = Opp[O * 2];
			Cards[1] = Opp[O * 2 + 1];
			const int Os = Evaluate(Cards, 7);
			if (Os > Hs)
			{
				Lost = true;
				break;
			}
			if (Os == Hs)
			{
				++Ties;
			}
		}
		if (!Lost)
		{
			Won += 1.0 / static_cast<double>(Ties);
		}
	}
	return Won / static_cast<double>(Iterations);
}

double HeuristicEquity(const std::vector<Card>& Hole, const std::vector<Card>& Board, int Opponents)
{
	const int BoardCat = BoardCategory(Board);
	Card Scratch[7];
	Scratch[0] = Hole[0];
	Scratch[1] = Hole[1];
	for (size_t I = 0; I < Board.size(); ++I)
	{
		Scratch[2 + I] = Board[I];
	}
	const int Score = Evaluate(Scratch, 2 + static_cast<int>(Board.size()));
	const int Cat = Score >> 20;
	double Eq;
	if (Cat <= BoardCat && Cat <= static_cast<int>(Category::Pair))
	{
		const int HighRank = (Hole[0] >> 2) > (Hole[1] >> 2) ? (Hole[0] >> 2) : (Hole[1] >> 2);
		Eq = 0.12 + 0.02 * static_cast<double>(HighRank) / 12.0;
	}
	else if (Cat == static_cast<int>(Category::Pair))
	{
		const int PairRank = (Score >> 16) & 15;
		int HigherBoard = 0;
		for (Card C : Board)
		{
			if ((C >> 2) > PairRank)
			{
				++HigherBoard;
			}
		}
		Eq = HigherBoard == 0 ? 0.62 + 0.012 * static_cast<double>(PairRank) : HigherBoard == 1 ? 0.45 : 0.32;
	}
	else if (Cat == static_cast<int>(Category::TwoPair))
	{
		Eq = BoardCat == static_cast<int>(Category::Pair) ? 0.55 : 0.76;
	}
	else if (Cat == static_cast<int>(Category::Trips))
	{
		Eq = 0.82;
	}
	else if (Cat == static_cast<int>(Category::Straight))
	{
		Eq = 0.86;
	}
	else if (Cat == static_cast<int>(Category::Flush))
	{
		Eq = 0.9;
	}
	else
	{
		Eq = 0.97;
	}
	if (Board.size() < 5 && HasDraw(Hole[0], Hole[1], Board))
	{
		Eq += Board.size() == 3 ? 0.18 : 0.1;
	}
	Eq = Min(0.99, Eq);
	return DetPow(Eq, 1.0 + 0.6 * static_cast<double>(Opponents - 1));
}
} // namespace ss
