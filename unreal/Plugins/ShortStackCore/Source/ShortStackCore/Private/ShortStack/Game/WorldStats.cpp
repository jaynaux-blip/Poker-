// The stats pages: a results tracker's view of everyone (ROI, ITM, the profit graph, the breakdowns and records).
// Every tournament the world plays is added as it ends; a regular's years before the story began are drawn from
// their lifetime numbers, with a random walk of their own (seeded by their name, so the world's dice never move).
#include "ShortStack/Game/World.h"

#include "ShortStack/Rng.h"
#include "WorldSim.h"

#include <algorithm>
#include <cmath>

namespace ss
{
namespace world
{
void Tracker::Add(Chips BuyIn, Chips Prize, int Place, int Field, int FinalSize, TrackStake Stake, TrackFormat Format)
{
	const bool Cash = Prize > 0;
	++Events;
	BuyIns += BuyIn;
	Prizes += Prize;
	Cashes += Cash ? 1 : 0;
	FinalTables += Place >= 1 && Place <= FinalSize ? 1 : 0;
	Podiums += Place == 2 || Place == 3 ? 1 : 0;
	Wins += Place == 1 ? 1 : 0;
	for (TrackLine* L : {&ByStake[static_cast<size_t>(Stake)], &ByFormat[static_cast<size_t>(Format)]})
	{
		++L->Events;
		L->BuyIns += BuyIn;
		L->Prizes += Prize;
		L->Cashes += Cash ? 1 : 0;
	}
	Net += Prize - BuyIn;
	if (Net > Peak)
	{
		Peak = Net;
		PeakAt = Events;
	}
	if (Peak - Net > Downswing)
	{
		Downswing = Peak - Net;
		DownFrom = PeakAt;
		DownTo = Events;
	}
	if (Prize > Best)
	{
		Best = Prize;
		BestAt = Events;
	}
	Dry = Cash ? 0 : Dry + 1;
	LongestDry = std::max(LongestDry, Dry);
	if (Place >= 1 && Field >= 1)
	{
		FinishSum += static_cast<double>(std::min(Place, Field)) / static_cast<double>(Field);
		++Finished;
	}
	// The graph: a point every Stride tournaments; full, it keeps every other point and the stride doubles.
	if (Events % Stride == 0)
	{
		Curve.push_back(Net);
		// (Never below the last point: a point loaded from a save is rounded to the dollar, and a ticket adds no buy-in.)
		Spend.push_back(std::max(BuyIns, Spend.empty() ? Chips(0) : Spend.back()));
		if (static_cast<int>(Curve.size()) >= CurveMax)
		{
			std::vector<Chips> Half;
			std::vector<Chips> HalfSpend;
			Half.reserve(Curve.size() / 2);
			for (size_t K = 1; K < Curve.size(); K += 2)
			{
				Half.push_back(Curve[K]);
				HalfSpend.push_back(K < Spend.size() ? Spend[K] : BuyIns);
			}
			Curve.swap(Half);
			Spend.swap(HalfSpend);
			Stride *= 2;
		}
	}
}

Chips Tracker::StretchAbi(size_t K) const
{
	if (K > Curve.size() || K > Spend.size())
	{
		return AverageBuyIn();
	}
	const Chips Before = K == 0 ? 0 : Spend[K - 1];
	const Chips After = K < Spend.size() ? Spend[K] : BuyIns;
	const int Count = K < Curve.size() ? Stride : Events - static_cast<int>(Curve.size()) * Stride;
	return Count > 0 ? (After - Before) / Count : 0;
}

TrackStake Sim::StakeOf(const Pending& P)
{
	if (!P.Online)
	{
		return TrackStake::Live;
	}
	const int T = sim::TierIndex(P.Tier);
	return T <= 1 ? TrackStake::Micro : T == 2 ? TrackStake::Low : T == 3 ? TrackStake::Mid : TrackStake::High;
}

TrackFormat Sim::FormatOf(const Pending& P)
{
	const auto Is = [&](int Bit) { return (P.Kinds & Bit) != 0; };
	return Is(KindSatellite) ? TrackFormat::Satellite
		: Is(KindBounty) || Is(KindMystery) ? TrackFormat::Bounty
		: Is(KindHyper) ? TrackFormat::Hyper
		: Is(KindTurbo) ? TrackFormat::Turbo
		: Is(KindDeep) ? TrackFormat::Deep
					   : TrackFormat::Regular;
}

namespace
{
/** Splits Total into parts in proportion to Weights (whole units; the remainder goes to the largest shares). */
template <typename T>
std::vector<T> Split(T Total, const std::vector<double>& Weights)
{
	std::vector<T> Out(Weights.size(), 0);
	double Sum = 0.0;
	for (double V : Weights)
	{
		Sum += std::max(0.0, V);
	}
	if (Sum <= 0.0 || Total <= 0 || Weights.empty())
	{
		if (!Out.empty())
		{
			Out[0] = Total;
		}
		return Out;
	}
	T Given = 0;
	std::vector<std::pair<double, size_t>> Rest;
	for (size_t K = 0; K < Weights.size(); ++K)
	{
		const double Exact = static_cast<double>(Total) * std::max(0.0, Weights[K]) / Sum;
		Out[K] = static_cast<T>(std::floor(Exact));
		Given += Out[K];
		Rest.push_back({Exact - std::floor(Exact), K});
	}
	std::sort(Rest.begin(), Rest.end(), [](const std::pair<double, size_t>& A, const std::pair<double, size_t>& B) { return A.first > B.first || (A.first == B.first && A.second < B.second); });
	for (size_t K = 0; Given < Total && K < Rest.size(); ++K, ++Given)
	{
		++Out[Rest[K].second];
	}
	return Out;
}

/** Fills one breakdown from totals: events by Weights, a return of their own for each part around the overall one. */
template <size_t Count>
void Spread(std::array<TrackLine, Count>& Lines, int Events, Chips BuyIns, Chips Prizes, int Cashes, const std::vector<double>& Weights, double Roi, Rng& R)
{
	const std::vector<int> Ev = Split<int>(Events, Weights);
	std::vector<double> Spend;
	std::vector<double> Return;
	std::vector<double> Hits;
	for (size_t K = 0; K < Count; ++K)
	{
		Spend.push_back(static_cast<double>(Ev[K]));
		Return.push_back(static_cast<double>(Ev[K]) * std::max(0.05, 1.0 + Roi + R.Range(-0.25, 0.25)));
		Hits.push_back(static_cast<double>(Ev[K]) * R.Range(0.8, 1.2));
	}
	const std::vector<Chips> Bi = Split<Chips>(BuyIns, Spend);
	const std::vector<Chips> Pz = Split<Chips>(Prizes, Return);
	const std::vector<int> Ca = Split<int>(Cashes, Hits);
	for (size_t K = 0; K < Count; ++K)
	{
		Lines[K].Events += Ev[K];
		Lines[K].BuyIns += Bi[K];
		Lines[K].Prizes += Pz[K];
		Lines[K].Cashes += std::min(Ca[K], Ev[K]);
	}
}
} // namespace

void Sim::SeedStats(Npc& N)
{
	const Ledger& On = N.Totals[static_cast<size_t>(Venue::Online)];
	const Ledger& Lv = N.Totals[static_cast<size_t>(Venue::Live)];
	const int Events = On.Events + Lv.Events;
	if (N.Faded || N.Stats.Events > 0 || Events <= 0)
	{
		return;
	}
	Rng R("stats:" + N.Name);
	Tracker& T = N.Stats;
	T = Tracker();
	T.Events = Events;
	T.BuyIns = On.Spent + Lv.Spent;
	T.Prizes = On.Won + Lv.Won;
	T.Cashes = std::min(Events, On.Cashes + Lv.Cashes);
	T.FinalTables = std::min(T.Cashes, On.FinalTables + Lv.FinalTables);
	T.Wins = std::min(T.FinalTables, On.Wins + Lv.Wins);
	T.Podiums = std::min(T.FinalTables - T.Wins, static_cast<int>(std::lround(static_cast<double>(T.FinalTables - T.Wins) * R.Range(0.2, 0.3))));
	const double Roi = T.Roi();
	// By buy-in: mostly their own stakes, some below, a few shots above; live on its own line.
	{
		const int Own = std::max(0, std::min(3, N.Tier - 1));
		std::vector<double> W(4, 0.0);
		W[static_cast<size_t>(Own)] += 0.7;
		W[static_cast<size_t>(std::max(0, Own - 1))] += 0.22;
		W[static_cast<size_t>(std::min(3, Own + 1))] += 0.08;
		std::array<TrackLine, 4> Online{};
		Spread(Online, On.Events, On.Spent, On.Won, std::min(On.Events, On.Cashes), W, On.Spent > 0 ? static_cast<double>(On.Won - On.Spent) / static_cast<double>(On.Spent) : 0.0, R);
		for (size_t K = 0; K < 4; ++K)
		{
			T.ByStake[K] = Online[K];
		}
		TrackLine& L = T.ByStake[static_cast<size_t>(TrackStake::Live)];
		L.Events = Lv.Events;
		L.BuyIns = Lv.Spent;
		L.Prizes = Lv.Won;
		L.Cashes = std::min(Lv.Events, Lv.Cashes);
	}
	// By format: what they like gets played more.
	{
		const auto Likes = [&](int Bit) { return (N.Formats & Bit) != 0 ? 1.0 : 0.0; };
		const std::vector<double> W = {0.3, 0.1 + 0.15 * Likes(LikeDeep), 0.16 + 0.15 * Likes(LikeTurbo), 0.06 + 0.05 * Likes(LikeTurbo),
			0.16 + 0.2 * std::max(Likes(LikeBounty), Likes(LikeMystery)), 0.05 + 0.1 * Likes(LikeSatellite)};
		Spread(T.ByFormat, Events, T.BuyIns, T.Prizes, T.Cashes, W, Roi, R);
	}
	// The graph: buy-ins bleed steadily, prizes come in bursts, the biggest score where it landed.
	T.Stride = 1;
	while (Events / T.Stride > 48)
	{
		T.Stride *= 2;
	}
	const int Points = Events / T.Stride;
	const int Segments = Points + (Events % T.Stride > 0 ? 1 : 0);
	std::vector<double> Size;
	std::vector<double> Weight;
	std::vector<double> Stakes; // buy-ins per segment: most careers start smaller and move up, with spells back down
	const double Start = R.Range(0.3, 0.8);
	double Drift = 0.0;
	for (int K = 0; K < Segments; ++K)
	{
		const double S = static_cast<double>(K < Points ? T.Stride : Events % T.Stride);
		Size.push_back(S);
		const double U = R.Next();
		Weight.push_back(S * (0.35 + 4.0 * U * U * U * U * U));
		const double Along = Segments > 1 ? static_cast<double>(K) / static_cast<double>(Segments - 1) : 1.0;
		Drift = std::max(-0.35, std::min(0.35, Drift * 0.8 + R.Range(-0.12, 0.12)));
		Stakes.push_back(S * std::max(0.1, Start + (1.0 - Start) * std::sqrt(Along) + Drift));
	}
	const Chips Best = std::min(T.Prizes, std::max(On.Best, Lv.Best));
	const int BestSeg = R.Int(Segments);
	const std::vector<Chips> Spent = Split<Chips>(T.BuyIns, Stakes);
	// Prizes follow the stakes too (a score at $100 is bigger than one at $5).
	for (size_t K = 0; K < Weight.size(); ++K)
	{
		Weight[K] *= Stakes[K] / std::max(1e-9, Size[K]);
	}
	std::vector<Chips> Won = Split<Chips>(T.Prizes - Best, Weight);
	Won[static_cast<size_t>(BestSeg)] += Best;
	Chips Run = 0;
	Chips Paid = 0;
	int At = 0;
	for (int K = 0; K < Segments; ++K)
	{
		Paid += Spent[static_cast<size_t>(K)];
		const int Before = At;
		At += static_cast<int>(Size[static_cast<size_t>(K)]);
		// Within a segment: the bleed first, then its prizes (the bottom of a swing is just before a score).
		const Chips Low = Run - Spent[static_cast<size_t>(K)];
		if (T.Peak - Low > T.Downswing)
		{
			T.Downswing = T.Peak - Low;
			T.DownFrom = T.PeakAt;
			T.DownTo = std::max(Before + 1, At - 1);
		}
		Run = Low + Won[static_cast<size_t>(K)];
		if (Run > T.Peak)
		{
			T.Peak = Run;
			T.PeakAt = At;
		}
		if (K == BestSeg)
		{
			T.Best = Best;
			T.BestAt = std::max(1, Before + static_cast<int>(Size[static_cast<size_t>(K)] * R.Range(0.3, 0.9)));
		}
		if (K < Points)
		{
			T.Curve.push_back(Run);
			T.Spend.push_back(Paid);
		}
	}
	T.Net = T.Prizes - T.BuyIns;
	// Droughts and finishes, as someone with this record would have had them.
	const double Itm = std::max(0.02, T.Itm());
	T.LongestDry = std::max(1, static_cast<int>(std::lround(std::log(static_cast<double>(Events) + 1.0) / -std::log(1.0 - std::min(0.6, Itm)) * R.Range(0.85, 1.25))));
	T.Dry = std::min(T.LongestDry, R.Int(static_cast<int>(1.0 / Itm) + 1));
	T.Finished = Events;
	T.FinishSum = static_cast<double>(Events) * std::max(0.28, std::min(0.62, 0.5 - 0.1 * Roi + R.Range(-0.03, 0.03)));
}

void World::GrantHeroResults(int Count)
{
	Rng R("hero-results:" + std::to_string(HeroBook.Events));
	static const double BuyIns[7] = {0.25, 1.10, 2.20, 3.30, 5.50, 11.0, 22.0};
	for (int K = 0; K < Count; ++K)
	{
		const double Bi = BuyIns[R.Int(7)];
		const int Entrants = 60 + R.Int(1800);
		const bool Fast = R.Chance(0.35);
		// A little better than the field: finishes lean toward the front.
		const double U = std::pow(R.Next(), 1.2);
		const int Place = std::max(1, std::min(Entrants, static_cast<int>(U * static_cast<double>(Entrants)) + 1));
		const int Paid = std::max(3, Entrants * 15 / 100);
		Chips Prize = 0;
		if (Place <= Paid)
		{
			const double Pool = Bi * 0.9 * static_cast<double>(Entrants);
			const double Share = std::max(1.6 * Bi / Pool, 0.22 * std::pow(static_cast<double>(Place), -1.15));
			Prize = sim::Cents(Pool * std::min(0.22, Share));
		}
		const TrackStake Stake = Bi <= 5.5 ? TrackStake::Micro : TrackStake::Low;
		const TrackFormat Format = Fast ? (R.Chance(0.3) ? TrackFormat::Hyper : TrackFormat::Turbo) : R.Chance(0.3) ? TrackFormat::Bounty : TrackFormat::Regular;
		HeroBook.Add(sim::Cents(Bi), Prize, Place, Entrants, 9, Stake, Format);
	}
	++HeroRev;
}

double World::RoiRank(double Roi, int Min) const
{
	int Below = 0;
	int Count = 0;
	for (const Npc& N : Roster)
	{
		if (N.Faded || N.Stats.Events < Min || N.Stats.BuyIns <= 0)
		{
			continue;
		}
		++Count;
		Below += N.Stats.Roi() < Roi ? 1 : 0;
	}
	return Count > 0 ? static_cast<double>(Below) / static_cast<double>(Count) : 0.0;
}
} // namespace world
} // namespace ss
