#include "ShortStack/Evaluator.h"

#include "ShortStack/Cards.h"

namespace ss
{
const char* const CategoryNames[9] = {"High Card", "Pair", "Two Pair", "Three of a Kind", "Straight", "Flush", "Full House", "Four of a Kind", "Straight Flush"};

namespace eval_detail
{
struct Tables
{
	signed char StraightHighTable[8192];
	unsigned char PopCount[8192];

	Tables()
	{
		for (int M = 0; M < 8192; ++M)
		{
			int P = 0;
			for (int B = 0; B < 13; ++B)
			{
				if (M & (1 << B))
				{
					++P;
				}
			}
			PopCount[M] = static_cast<unsigned char>(P);
			int High = -1;
			for (int Top = 12; Top >= 4; --Top)
			{
				const int Run = 0x1F << (Top - 4);
				if ((M & Run) == Run)
				{
					High = Top;
					break;
				}
			}
			// Wheel: A-2-3-4-5 plays as a five-high straight.
			if (High < 0 && (M & 0x100F) == 0x100F)
			{
				High = 3;
			}
			StraightHighTable[M] = static_cast<signed char>(High);
		}
	}
};

const Tables& GetTables()
{
	static const Tables Instance;
	return Instance;
}

int TopRanks(int Mask, int N)
{
	int Out = 0;
	int Taken = 0;
	for (int R = 12; R >= 0 && Taken < N; --R)
	{
		if (Mask & (1 << R))
		{
			Out = (Out << 4) | R;
			++Taken;
		}
	}
	return Out << (4 * (5 - Taken));
}

constexpr int Cat(Category C) { return static_cast<int>(C) << 20; }
} // namespace eval_detail

int StraightHigh(int RankMask)
{
	return eval_detail::GetTables().StraightHighTable[RankMask & 8191];
}

int Evaluate(const Card* Cards, int N)
{
	using namespace eval_detail;
	const Tables& T = GetTables();
	int Counts[13] = {0};
	int SuitMasks[4] = {0, 0, 0, 0};
	int RankMask = 0;
	for (int I = 0; I < N; ++I)
	{
		const int C = Cards[I];
		const int R = C >> 2;
		++Counts[R];
		SuitMasks[C & 3] |= 1 << R;
		RankMask |= 1 << R;
	}

	// With at most seven cards a flush rules out quads and full houses.
	for (int S = 0; S < 4; ++S)
	{
		const int Sm = SuitMasks[S];
		if (T.PopCount[Sm] >= 5)
		{
			const int Sf = T.StraightHighTable[Sm];
			if (Sf >= 0)
			{
				return Cat(Category::StraightFlush) | (Sf << 16);
			}
			return Cat(Category::Flush) | TopRanks(Sm, 5);
		}
	}

	int Quad = -1;
	int Trip1 = -1;
	int Trip2 = -1;
	int Pair1 = -1;
	int Pair2 = -1;
	for (int R = 12; R >= 0; --R)
	{
		const int K = Counts[R];
		if (K == 4)
		{
			Quad = R;
		}
		else if (K == 3)
		{
			if (Trip1 < 0)
			{
				Trip1 = R;
			}
			else if (Trip2 < 0)
			{
				Trip2 = R;
			}
		}
		else if (K == 2)
		{
			if (Pair1 < 0)
			{
				Pair1 = R;
			}
			else if (Pair2 < 0)
			{
				Pair2 = R;
			}
		}
	}

	if (Quad >= 0)
	{
		const int Kick = TopRanks(RankMask & ~(1 << Quad), 1) >> 16;
		return Cat(Category::Quads) | (Quad << 16) | (Kick << 12);
	}
	if (Trip1 >= 0 && (Trip2 >= 0 || Pair1 >= 0))
	{
		const int PairRank = Trip2 > Pair1 ? Trip2 : Pair1;
		return Cat(Category::FullHouse) | (Trip1 << 16) | (PairRank << 12);
	}
	const int St = T.StraightHighTable[RankMask];
	if (St >= 0)
	{
		return Cat(Category::Straight) | (St << 16);
	}
	if (Trip1 >= 0)
	{
		const int Kick = TopRanks(RankMask & ~(1 << Trip1), 2) >> 12;
		return Cat(Category::Trips) | (Trip1 << 16) | (Kick << 8);
	}
	if (Pair1 >= 0 && Pair2 >= 0)
	{
		const int Kick = TopRanks(RankMask & ~(1 << Pair1) & ~(1 << Pair2), 1) >> 16;
		return Cat(Category::TwoPair) | (Pair1 << 16) | (Pair2 << 12) | (Kick << 8);
	}
	if (Pair1 >= 0)
	{
		const int Kick = TopRanks(RankMask & ~(1 << Pair1), 3) >> 8;
		return Cat(Category::Pair) | (Pair1 << 16) | (Kick << 4);
	}
	return Cat(Category::HighCard) | TopRanks(RankMask, 5);
}

int EvaluateHand(const std::vector<Card>& Hole, const std::vector<Card>& Board)
{
	Card All[7];
	int N = 0;
	for (Card C : Hole)
	{
		All[N++] = C;
	}
	for (Card C : Board)
	{
		All[N++] = C;
	}
	return Evaluate(All, N);
}

std::string Describe(int Score)
{
	const int CatIdx = Score >> 20;
	const int R1 = (Score >> 16) & 15;
	const int R2 = (Score >> 12) & 15;
	switch (static_cast<Category>(CatIdx))
	{
	case Category::StraightFlush:
		return R1 == 12 ? std::string("Royal Flush") : std::string("Straight Flush, ") + RankNames[R1] + " high";
	case Category::Quads:
		return std::string("Four of a Kind, ") + RankPlurals[R1];
	case Category::FullHouse:
		return std::string("Full House, ") + RankPlurals[R1] + " full of " + RankPlurals[R2];
	case Category::Flush:
		return std::string("Flush, ") + RankNames[R1] + " high";
	case Category::Straight:
		return std::string("Straight, ") + RankNames[R1] + " high";
	case Category::Trips:
		return std::string("Three of a Kind, ") + RankPlurals[R1];
	case Category::TwoPair:
		return std::string("Two Pair, ") + RankPlurals[R1] + " and " + RankPlurals[R2];
	case Category::Pair:
		return std::string("Pair of ") + RankPlurals[R1];
	default:
		return std::string(RankNames[R1]) + " High";
	}
}
} // namespace ss
