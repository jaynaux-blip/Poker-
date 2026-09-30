#include "ShortStack/Equity.h"

#include "ShortStack/Cards.h"
#include "ShortStack/Evaluator.h"
#include "ShortStack/Rng.h"

#include <cstring>

namespace ss
{
namespace equity_detail
{
struct PercentileTable
{
	double Values[169];
	PercentileTable()
	{
		int Cum = 0;
		for (int I = 0; I < 169; ++I)
		{
			const int Cls = PreflopOrder[I];
			Cum += HandClassCombos(Cls);
			Values[Cls] = static_cast<double>(Cum) / 1326.0;
		}
	}
};

bool InBand(Card A, Card B, const RangeBand& Band)
{
	const double P = ClassPercentile()[HandClass(A, B)];
	return P > Band.Min && P <= Band.Max;
}
} // namespace equity_detail

const double* ClassPercentile()
{
	static const equity_detail::PercentileTable Table;
	return Table.Values;
}

double HandPercentile(Card A, Card B)
{
	return ClassPercentile()[HandClass(A, B)];
}

double EquityVsRanges(const std::vector<Card>& Hero, const std::vector<Card>& Board, const std::vector<RangeBand>& Ranges, int Iterations, Rng& R)
{
	if (Ranges.empty())
	{
		return 1.0;
	}
	unsigned char Dead[52] = {0};
	for (Card C : Hero)
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

	Card Cards[7];
	std::vector<Card> OppHands(Ranges.size() * 2);
	const int BoardNeed = 5 - static_cast<int>(Board.size());
	unsigned char Used[52];
	double Won = 0.0;

	for (int It = 0; It < Iterations; ++It)
	{
		std::memset(Used, 0, sizeof(Used));
		for (size_t O = 0; O < Ranges.size(); ++O)
		{
			const RangeBand& Band = Ranges[O];
			Card A = 0;
			Card B = 0;
			for (int Tries = 0; Tries < 60; ++Tries)
			{
				A = Draw();
				B = Draw();
				if (A == B || Used[A] || Used[B])
				{
					continue;
				}
				if (Tries > 45 || equity_detail::InBand(A, B, Band))
				{
					break;
				}
			}
			if (A == B || Used[A] || Used[B])
			{
				// Fallback: any two free cards.
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
			OppHands[O * 2] = A;
			OppHands[O * 2 + 1] = B;
		}
		int N = 2;
		for (Card C : Board)
		{
			Cards[N++] = C;
		}
		for (int K = 0; K < BoardNeed; ++K)
		{
			Card C;
			do
			{
				C = Draw();
			} while (Used[C]);
			Used[C] = 1;
			Cards[N++] = C;
		}
		Cards[0] = Hero[0];
		Cards[1] = Hero[1];
		const int Hs = Evaluate(Cards, 7);
		int Ties = 1;
		bool Lost = false;
		for (size_t O = 0; O < Ranges.size(); ++O)
		{
			Cards[0] = OppHands[O * 2];
			Cards[1] = OppHands[O * 2 + 1];
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

std::vector<double> EquityKnownHands(const std::vector<std::vector<Card>>& Hands, const std::vector<Card>& Board, Rng& R, int PreflopSamples)
{
	unsigned char Dead[52] = {0};
	for (const std::vector<Card>& H : Hands)
	{
		for (Card C : H)
		{
			Dead[C] = 1;
		}
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
	const size_t NH = Hands.size();
	std::vector<double> Shares(NH, 0.0);
	std::vector<int> Scores(NH, 0);
	Card Cards[7];
	double Total = 0.0;

	auto Settle = [&](const Card Full[5]) {
		int Best = -1;
		int Count = 0;
		for (size_t H = 0; H < NH; ++H)
		{
			Cards[0] = Hands[H][0];
			Cards[1] = Hands[H][1];
			for (int I = 0; I < 5; ++I)
			{
				Cards[2 + I] = Full[I];
			}
			const int S = Evaluate(Cards, 7);
			Scores[H] = S;
			if (S > Best)
			{
				Best = S;
				Count = 1;
			}
			else if (S == Best)
			{
				++Count;
			}
		}
		for (size_t H = 0; H < NH; ++H)
		{
			if (Scores[H] == Best)
			{
				Shares[H] += 1.0 / static_cast<double>(Count);
			}
		}
		Total += 1.0;
	};

	const int BoardN = static_cast<int>(Board.size());
	const int Need = 5 - BoardN;
	Card Full[5] = {0, 0, 0, 0, 0};
	for (int I = 0; I < BoardN; ++I)
	{
		Full[I] = Board[static_cast<size_t>(I)];
	}
	if (Need == 0)
	{
		Settle(Full);
	}
	else if (Need <= 2)
	{
		if (Need == 1)
		{
			for (Card C : Live)
			{
				Full[4] = C;
				Settle(Full);
			}
		}
		else
		{
			for (size_t I = 0; I < Live.size(); ++I)
			{
				for (size_t J = I + 1; J < Live.size(); ++J)
				{
					Full[3] = Live[I];
					Full[4] = Live[J];
					Settle(Full);
				}
			}
		}
	}
	else
	{
		std::vector<Card> Deck = Live;
		const int DeckN = static_cast<int>(Deck.size());
		for (int S = 0; S < PreflopSamples; ++S)
		{
			for (int I = 0; I < Need; ++I)
			{
				const int J = I + R.Int(DeckN - I);
				const Card T = Deck[static_cast<size_t>(I)];
				Deck[static_cast<size_t>(I)] = Deck[static_cast<size_t>(J)];
				Deck[static_cast<size_t>(J)] = T;
			}
			for (int I = 0; I < BoardN; ++I)
			{
				Full[I] = Board[static_cast<size_t>(I)];
			}
			for (int I = 0; I < Need; ++I)
			{
				Full[BoardN + I] = Deck[static_cast<size_t>(I)];
			}
			Settle(Full);
		}
	}
	for (double& V : Shares)
	{
		V /= Total;
	}
	return Shares;
}
} // namespace ss
