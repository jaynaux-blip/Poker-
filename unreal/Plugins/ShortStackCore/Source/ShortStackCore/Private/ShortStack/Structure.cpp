#include "ShortStack/Structure.h"
#include "StrictFloat.h"

#include <cmath>

namespace ss
{
namespace structure_detail
{
struct IcmState
{
	const std::vector<double>* Stacks;
	const std::vector<double>* Prizes;
	std::vector<double>* Result;
	int N;
	int Places;
};

// Expand every finishing order: P(i finishes next) = stack_i / chips left.
void IcmRecurse(const IcmState& S, uint32_t Mask, int Depth, double Prob, double Remaining)
{
	if (Depth >= S.Places)
	{
		return;
	}
	for (int I = 0; I < S.N; ++I)
	{
		if (Mask & (1u << I))
		{
			continue;
		}
		const double Stack = (*S.Stacks)[static_cast<size_t>(I)];
		const double P = (Stack / Remaining) * Prob;
		if (P == 0.0)
		{
			continue;
		}
		(*S.Result)[static_cast<size_t>(I)] += P * (*S.Prizes)[static_cast<size_t>(Depth)];
		IcmRecurse(S, Mask | (1u << I), Depth + 1, P, Remaining - Stack);
	}
}
} // namespace structure_detail

const std::vector<Level>& StandardLevels()
{
	static const std::vector<Level> Levels = [] {
		const Chips Pairs[][2] = {
			{50, 100}, {60, 120}, {80, 160}, {100, 200}, {125, 250}, {150, 300}, {200, 400}, {250, 500},
			{300, 600}, {400, 800}, {500, 1000}, {600, 1200}, {800, 1600}, {1000, 2000}, {1250, 2500},
			{1500, 3000}, {2000, 4000}, {2500, 5000}, {3000, 6000}, {4000, 8000}, {5000, 10000},
			{6000, 12000}, {8000, 16000}, {10000, 20000}, {12500, 25000}, {15000, 30000}, {20000, 40000},
			{25000, 50000}, {30000, 60000}, {40000, 80000}, {50000, 100000}, {60000, 120000},
			{80000, 160000}, {100000, 200000}, {125000, 250000}, {150000, 300000}, {200000, 400000},
			{250000, 500000}, {300000, 600000}, {400000, 800000}, {500000, 1000000},
		};
		std::vector<Level> Out;
		for (const auto& P : Pairs)
		{
			Level L;
			L.Sb = P[0];
			L.Bb = P[1];
			L.Ante = P[1];
			Out.push_back(L);
		}
		return Out;
	}();
	return Levels;
}

std::vector<std::pair<int, int>> PayoutBands(int Paid)
{
	std::vector<std::pair<int, int>> Bands;
	int Place = 1;
	while (Place <= Paid)
	{
		int Size;
		if (Place <= 9) Size = 1;
		else if (Place <= 18) Size = 3;
		else if (Place <= 72) Size = 9;
		else if (Place <= 216) Size = 18;
		else Size = 36;
		const int End = Paid < Place + Size - 1 ? Paid : Place + Size - 1;
		Bands.emplace_back(Place, End);
		Place = End + 1;
	}
	return Bands;
}

std::vector<Chips> PayoutTable(Chips PoolCents, int Entrants, Chips MinCashCents, double PaidShare)
{
	const double Pool = static_cast<double>(PoolCents);
	const int Paid = static_cast<int>(Max(1.0, Min(static_cast<double>(Entrants - 1), JsRound(static_cast<double>(Entrants) * PaidShare))));
	if (Entrants <= 3)
	{
		return {PoolCents};
	}
	const double Exponent = Paid > 60 ? 1.0 : Paid > 12 ? 0.95 : 0.85;
	std::vector<double> Weights;
	for (int I = 1; I <= Paid; ++I)
	{
		Weights.push_back(1.0 / DetPow(static_cast<double>(I), Exponent));
	}
	double WSum = 0.0;
	for (double W : Weights)
	{
		WSum += W;
	}
	std::vector<double> Cents;
	for (double W : Weights)
	{
		Cents.push_back((W / WSum) * Pool);
	}
	// Enforce the min-cash floor, taking the difference proportionally from the top.
	const double Floor = Min(static_cast<double>(MinCashCents), Pool / Paid);
	double Deficit = 0.0;
	for (double& C : Cents)
	{
		if (C < Floor)
		{
			Deficit += Floor - C;
			C = Floor;
		}
	}
	if (Deficit > 0.0)
	{
		std::vector<double> Above;
		double AboveSum = 0.0;
		for (double C : Cents)
		{
			Above.push_back(Max(0.0, C - Floor));
		}
		for (double A : Above)
		{
			AboveSum += A;
		}
		for (size_t I = 0; I < Cents.size(); ++I)
		{
			Cents[I] = Cents[I] - (AboveSum > 0.0 ? (Above[I] / AboveSum) * Deficit : 0.0);
		}
	}
	// Payout bands: places 10+ share the band average like real sites.
	for (const auto& Band : PayoutBands(Paid))
	{
		const int A = Band.first;
		const int B = Band.second;
		if (B <= A)
		{
			continue;
		}
		double Sum = 0.0;
		for (int I = A; I <= B; ++I)
		{
			Sum += Cents[static_cast<size_t>(I - 1)];
		}
		for (int I = A; I <= B; ++I)
		{
			Cents[static_cast<size_t>(I - 1)] = Sum / (B - A + 1);
		}
	}
	std::vector<Chips> Rounded;
	Chips RoundedSum = 0;
	for (double C : Cents)
	{
		const Chips V = static_cast<Chips>(std::floor(C));
		Rounded.push_back(V);
		RoundedSum += V;
	}
	Chips Rem = PoolCents - RoundedSum;
	for (size_t I = 0; Rem > 0; I = (I + 1) % Rounded.size(), --Rem)
	{
		++Rounded[I];
	}
	return Rounded;
}

std::vector<double> Icm(const std::vector<double>& Stacks, const std::vector<double>& Prizes)
{
	const int N = static_cast<int>(Stacks.size());
	double Total = 0.0;
	for (double S : Stacks)
	{
		Total += S;
	}
	std::vector<double> Result(static_cast<size_t>(N), 0.0);
	if (N == 0 || Total == 0.0)
	{
		return Result;
	}
	const int Places = static_cast<int>(Prizes.size()) < N ? static_cast<int>(Prizes.size()) : N;
	double Orders = 1.0;
	for (int K = 0; K < Places; ++K)
	{
		Orders *= N - K;
	}
	if (Orders > 2000000.0)
	{
		// Beyond exact range: proportional approximation.
		double PrizeSum = 0.0;
		for (double P : Prizes)
		{
			PrizeSum += P;
		}
		for (int I = 0; I < N; ++I)
		{
			Result[static_cast<size_t>(I)] = (Stacks[static_cast<size_t>(I)] / Total) * PrizeSum;
		}
		return Result;
	}
	const structure_detail::IcmState State{&Stacks, &Prizes, &Result, N, Places};
	structure_detail::IcmRecurse(State, 0u, 0, 1.0, Total);
	return Result;
}
} // namespace ss
