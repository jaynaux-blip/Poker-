#include "ShortStack/Structure.h"
#include "StrictFloat.h"

#include <cmath>
#include <cstdint>

namespace ss
{
namespace structure_detail
{
struct IcmState
{
	const std::vector<double>* Stacks;
	const std::vector<double>* Prizes;
	std::vector<double>* Result;
	// By player: already in the finishing order being expanded. One flag each, so any number of players: a 32-bit mask
	// (1u << I) took player 33 for player 1 (shift counts wrap at 32 on x64), e.g. 40 left paying 3 (g1-audit N7).
	std::vector<uint8_t>* Placed;
	int N;
	int Places;
};

// Expand every finishing order: P(i finishes next) = stack_i / chips left.
void IcmRecurse(const IcmState& S, int Depth, double Prob, double Remaining)
{
	if (Depth >= S.Places)
	{
		return;
	}
	for (int I = 0; I < S.N; ++I)
	{
		if ((*S.Placed)[static_cast<size_t>(I)] != 0)
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
		(*S.Placed)[static_cast<size_t>(I)] = 1;
		IcmRecurse(S, Depth + 1, P, Remaining - Stack);
		(*S.Placed)[static_cast<size_t>(I)] = 0;
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

// Update 4 (SPEC v1.7 §2.4.3, §6 PA-11). Standard is PayoutTable exactly. Main is the festival Mains' written curve (G1, F-net's
// FN-5), ported from model/payout_port/harness/main_curve.cpp (the C++ port proven on 7.3 million tables against engine_payout.py)
// in the engine's arithmetic: JsRound, DetPow, PowInt, PayoutBands, left-to-right sums and no fused multiply-add (StrictFloat.h).
namespace structure_detail
{
/** Step 3's lift for N entrants: 0.30 up to kMainLiftFull, tapering in ln(N) to 0 at kMainLiftZero. std::log is the lead's M-2
 *  exception to Common.h's rule (net::Points already calls std::log10); MSVC's and Python's agree bit for bit at every N between. */
double StructureMainLift(int Entrants)
{
	if (Entrants <= payout::kMainLiftFull)
	{
		return payout::kMainLift;
	}
	if (Entrants >= payout::kMainLiftZero)
	{
		return 0.0;
	}
	return payout::kMainLift * std::log(static_cast<double>(payout::kMainLiftZero) / Entrants) /
		std::log(static_cast<double>(payout::kMainLiftZero) / static_cast<double>(payout::kMainLiftFull));
}

int StructureMainPaid(int Entrants)
{
	if (Entrants <= 3)
	{
		return 1;
	}
	return static_cast<int>(Max(1.0, Min(static_cast<double>(Entrants - 1), JsRound(static_cast<double>(Entrants) * payout::kMainPaidShare))));
}

/** §2.4.3's steps 1-7, with the lead's M-1 (step 5's floors made non-increasing, then scaled to the pool) and N-1 (scaled floors
 *  are the table). Non-increasing, never below 0 and summing to the pool by construction. */
std::vector<Chips> StructureMainTable(Chips PoolCents, int Entrants, Chips MinCashCents)
{
	// 1. Entrants <= 3: {Pool}. Else Paid = max(1, min(N - 1, JsRound(N x 0.12))).
	if (Entrants <= 3)
	{
		return {PoolCents};
	}
	const int Paid = StructureMainPaid(Entrants);
	const double Pool = static_cast<double>(PoolCents);

	// 2. w_i = 1 / DetPow(i, e), e = Paid > 60 ? 1.0 : Paid > 12 ? 0.95 : 0.85 (PayoutTable's).
	const double E = Paid > 60 ? 1.0 : (Paid > 12 ? 0.95 : 0.85);
	std::vector<double> W(static_cast<size_t>(Paid));
	for (int I = 1; I <= Paid; ++I)
	{
		W[static_cast<size_t>(I - 1)] = 1.0 / DetPow(static_cast<double>(I), E);
	}

	// 3. w_i *= 1 + L x (9 - i) / 8 for i = 1 .. min(9, Paid): 1st gets 1 + L, 9th gets 1 (the written lift is the contract).
	const double L = StructureMainLift(Entrants);
	const int Top = Paid < 9 ? Paid : 9;
	for (int I = 1; I <= Top; ++I)
	{
		W[static_cast<size_t>(I - 1)] *= 1.0 + L * (9 - I) / 8.0;
	}

	// 4. c_i = w_i / sum(w) x Pool. F0 = min(MinCash, Pool / Paid).
	double SumW = 0.0;
	for (int I = 0; I < Paid; ++I)
	{
		SumW += W[static_cast<size_t>(I)];
	}
	std::vector<double> C(static_cast<size_t>(Paid));
	for (int I = 0; I < Paid; ++I)
	{
		C[static_cast<size_t>(I)] = W[static_cast<size_t>(I)] / SumW * Pool;
	}
	const double F0 = Min(static_cast<double>(MinCashCents), Pool / Paid);

	// 5. f_i = F0 for i <= Paid / 2; below, b = (Paid - i) / Paid (0 at the last paid place), k = b < 0.20 ? 0 :
	//    1 + floor((b - 0.20) / 0.10), f_i = F0 x 1.15^k.
	std::vector<double> F(static_cast<size_t>(Paid));
	for (int I = 1; I <= Paid; ++I)
	{
		double Fi = F0;
		if (I > Paid / 2)
		{
			const double B = static_cast<double>(Paid - I) / static_cast<double>(Paid);
			const int K = B < payout::kMainFloorFlat ? 0 : 1 + static_cast<int>(std::floor((B - payout::kMainFloorFlat) / payout::kMainFloorBlock));
			Fi = F0 * PowInt(payout::kMainFloorStep, K);
		}
		F[static_cast<size_t>(I - 1)] = Fi;
	}
	//    (M-1) (a) each place's floor is the larger of its own and every lower place's, so the floors never rise down the table;
	//    (b) if they sum past the pool, every floor is scaled by Pool / sum(f), and (N-1) the scaled floors are the table.
	for (int I = Paid - 2; I >= 0; --I)
	{
		F[static_cast<size_t>(I)] = Max(F[static_cast<size_t>(I)], F[static_cast<size_t>(I + 1)]);
	}
	double SumF = 0.0;
	for (int I = 0; I < Paid; ++I)
	{
		SumF += F[static_cast<size_t>(I)];
	}
	if (SumF > Pool)
	{
		const double Scale = Pool / SumF;
		for (int I = 0; I < Paid; ++I)
		{
			F[static_cast<size_t>(I)] *= Scale;
		}
		C = F;
	}

	// 6. Raise every c_i < f_i to f_i; take the deficit from max(0, c_i - f_i) in proportion, across all places.
	double Deficit = 0.0;
	for (int I = 0; I < Paid; ++I)
	{
		const size_t U = static_cast<size_t>(I);
		if (C[U] < F[U])
		{
			Deficit += F[U] - C[U];
			C[U] = F[U];
		}
	}
	if (Deficit > 0.0)
	{
		std::vector<double> Over(static_cast<size_t>(Paid));
		double OverSum = 0.0;
		for (int I = 0; I < Paid; ++I)
		{
			Over[static_cast<size_t>(I)] = Max(0.0, C[static_cast<size_t>(I)] - F[static_cast<size_t>(I)]);
		}
		for (int I = 0; I < Paid; ++I)
		{
			OverSum += Over[static_cast<size_t>(I)];
		}
		for (int I = 0; I < Paid; ++I)
		{
			const size_t U = static_cast<size_t>(I);
			C[U] = C[U] - (OverSum > 0.0 ? (Over[U] / OverSum) * Deficit : 0.0);
		}
	}

	// 7. Average inside PayoutBands(Paid); floor to cents; the remainder one cent at a time from 1st.
	for (const std::pair<int, int>& Band : PayoutBands(Paid))
	{
		if (Band.second <= Band.first)
		{
			continue;
		}
		double BandSum = 0.0;
		for (int P = Band.first; P <= Band.second; ++P)
		{
			BandSum += C[static_cast<size_t>(P - 1)];
		}
		const double Avg = BandSum / (Band.second - Band.first + 1);
		for (int P = Band.first; P <= Band.second; ++P)
		{
			C[static_cast<size_t>(P - 1)] = Avg;
		}
	}
	std::vector<Chips> Out(static_cast<size_t>(Paid));
	Chips Total = 0;
	for (int I = 0; I < Paid; ++I)
	{
		Out[static_cast<size_t>(I)] = static_cast<Chips>(std::floor(C[static_cast<size_t>(I)]));
		Total += Out[static_cast<size_t>(I)];
	}
	size_t At = 0;
	for (Chips Left = PoolCents - Total; Left > 0; --Left)
	{
		++Out[At];
		At = (At + 1) % Out.size();
	}
	return Out;
}
} // namespace structure_detail

std::vector<Chips> PayoutFor(Chips PoolCents, int Entrants, Chips MinCashCents, PayoutCurve Curve)
{
	switch (Curve)
	{
	case PayoutCurve::Main:
		return structure_detail::StructureMainTable(PoolCents, Entrants, MinCashCents);
	case PayoutCurve::Standard:
	default:
		return PayoutTable(PoolCents, Entrants, MinCashCents);
	}
}

int PaidPlaces(int Entrants, PayoutCurve Curve)
{
	switch (Curve)
	{
	case PayoutCurve::Main:
		// StructureMainTable's count: one place up to 3 entrants (the whole pool), else max(1, min(N - 1, JsRound(N x 0.12))).
		return structure_detail::StructureMainPaid(Entrants);
	case PayoutCurve::Standard:
	default:
		// PayoutTable's own count, in its arithmetic: one place up to 3 entrants (the whole pool), else
		// max(1, min(N - 1, JsRound(N x 0.15))) places.
		if (Entrants <= 3)
		{
			return 1;
		}
		return static_cast<int>(Max(1.0, Min(static_cast<double>(Entrants - 1), JsRound(static_cast<double>(Entrants) * 0.15))));
	}
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
	std::vector<uint8_t> Placed(static_cast<size_t>(N), 0);
	const structure_detail::IcmState State{&Stacks, &Prizes, &Result, &Placed, N, Places};
	structure_detail::IcmRecurse(State, 0, 1.0, Total);
	return Result;
}
} // namespace ss
