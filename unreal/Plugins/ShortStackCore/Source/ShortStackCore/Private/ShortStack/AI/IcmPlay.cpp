#include "ShortStack/AI/IcmPlay.h"
#include "../StrictFloat.h"

#include "StyleDetail.h"

#include "ShortStack/AI/Model.h"
#include "ShortStack/Cards.h"
#include "ShortStack/Rng.h"
#include "ShortStack/Structure.h"
#include "ShortStack/Tournament.h"

#include <algorithm>
#include <vector>

// ICM Shark (update 2 §2.4.5): Balanced Reg until chips stop being worth their face, then the payout ladder in every big
// decision: its stack-deciding calls against the equity a bubble factor demands (computed with the exact Icm() on the
// real stacks with seven or fewer left, else on a six-player model), and its jams from the push chart scaled by the
// factors on both sides (the covering big stack's bubble aggression; a short stack's own factor tightens it). Its only
// draws are EquityVsModels' from the tournament's R, as a bot's call would draw. core-styles.

namespace ss
{
namespace icmplay
{
namespace icmplay_detail
{
int IcmSeats(const Tournament& T)
{
	int Seats = 0;
	for (const Chips P : T.Payouts)
	{
		Seats += T.Spec.SeatValueCents > 0 && P == T.Spec.SeatValueCents ? 1 : 0;
	}
	return Seats;
}

/**
 * ICM equity of the player at index 0 for these stacks and prizes (Prizes[k] for place k + 1). A stack of nothing is a bust:
 * the busted share the last places' prizes, and the rest play for the places above them (Icm() itself would divide 0 by 0).
 */
double IcmFirst(const std::vector<double>& Stacks, const std::vector<double>& Prizes)
{
	const size_t N = Stacks.size();
	std::vector<double> Live;
	bool FirstLive = false;
	size_t Busted = 0;
	for (size_t I = 0; I < N; ++I)
	{
		if (Stacks[I] > 0.0)
		{
			FirstLive = FirstLive || I == 0;
			Live.push_back(Stacks[I]);
		}
		else
		{
			++Busted;
		}
	}
	auto PrizeAt = [&](size_t Place) { return Place < Prizes.size() ? Prizes[Place] : 0.0; };
	if (!FirstLive)
	{
		double Sum = 0;
		for (size_t K = N - Busted; K < N; ++K)
		{
			Sum += PrizeAt(K);
		}
		return Busted > 0 ? Sum / static_cast<double>(Busted) : 0.0;
	}
	std::vector<double> Top;
	for (size_t K = 0; K < Live.size(); ++K)
	{
		Top.push_back(PrizeAt(K));
	}
	// The first player is the first live stack, so its equity is the first entry.
	const std::vector<double> Eq = Icm(Live, Top);
	return Eq.empty() ? 0.0 : Eq[0];
}
} // namespace icmplay_detail

bool InZone(const Tournament& T)
{
	if (T.bFinished || T.Remaining <= 1)
	{
		return false;
	}
	const int Seats = icmplay_detail::IcmSeats(T);
	if (T.Spec.SeatValueCents > 0 && Seats > 0)
	{
		return T.Remaining <= 3 * Seats;
	}
	return T.Remaining <= T.TableSize || static_cast<double>(T.Remaining) <= 1.3 * static_cast<double>(T.PaidPlaces());
}

double BubbleFactor(const Tournament& T, int Player, int Against)
{
	const int N = static_cast<int>(T.Players.size());
	if (Player < 0 || Against < 0 || Player >= N || Against >= N || Player == Against)
	{
		return 1.0;
	}
	const double Sp = static_cast<double>(T.Players[static_cast<size_t>(Player)].Stack);
	const double Sa = static_cast<double>(T.Players[static_cast<size_t>(Against)].Stack);
	const double Risk = Min(Sp, Sa);
	if (Risk <= 0.0)
	{
		return 1.0;
	}
	std::vector<double> Stacks = {Sp, Sa};
	std::vector<double> Prizes;
	const std::vector<const TPlayer*> Alive = T.AlivePlayers();
	const int Left = static_cast<int>(Alive.size());
	if (Left <= 7)
	{
		// Exact, on the real stacks and payouts.
		for (const TPlayer* P : Alive)
		{
			const int Idx = T.PlayerIndex(P->Id);
			if (Idx != Player && Idx != Against)
			{
				Stacks.push_back(static_cast<double>(P->Stack));
			}
		}
		for (int Place = 1; Place <= Left; ++Place)
		{
			Prizes.push_back(static_cast<double>(T.PrizeFor(Place)));
		}
	}
	else
	{
		// The six-player model: the two of them, four stacks of the rest's average, the ladder block-compressed (model place j
		// gets the mean prize of the j-th sixth of the places left), which keeps the pool's shape.
		double Total = 0;
		for (const TPlayer* P : Alive)
		{
			Total += static_cast<double>(P->Stack);
		}
		const double Field = Max(1.0, (Total - Sp - Sa) / static_cast<double>(Left - 2));
		for (int K = 0; K < 4; ++K)
		{
			Stacks.push_back(Field);
		}
		for (int J = 0; J < 6; ++J)
		{
			const int From = J * Left / 6 + 1;
			const int To = (J + 1) * Left / 6;
			double Sum = 0;
			for (int Place = From; Place <= To; ++Place)
			{
				Sum += static_cast<double>(T.PrizeFor(Place));
			}
			Prizes.push_back(To >= From ? Sum / static_cast<double>(To - From + 1) : 0.0);
		}
	}
	const double Base = icmplay_detail::IcmFirst(Stacks, Prizes);
	std::vector<double> Win = Stacks;
	Win[0] += Risk;
	Win[1] -= Risk;
	std::vector<double> Lose = Stacks;
	Lose[0] -= Risk;
	Lose[1] += Risk;
	const double Gained = icmplay_detail::IcmFirst(Win, Prizes) - Base;
	const double Lost = Base - icmplay_detail::IcmFirst(Lose, Prizes);
	if (Gained <= 1e-9)
	{
		return Lost <= 1e-9 ? 1.0 : 10.0; // nothing to gain (a seat already locked up): the factor is as large as it gets
	}
	return Min(10.0, Max(0.1, Lost / Gained));
}

double RequiredEquity(double Bf, Chips Risk, Chips Pot)
{
	const double Rk = static_cast<double>(Risk);
	const double Den = Bf * Rk + static_cast<double>(Pot);
	return Den > 0.0 ? Bf * Rk / Den : 0.0;
}
} // namespace icmplay

namespace style
{
namespace style_detail
{
/** The calibration knobs of §2.4.5 (§2.5's list, item 2). */
constexpr double IcmCoverCap = 1.6;
constexpr double IcmCoverExp = 0.6;
constexpr double IcmHeroExp = 0.8;

BotDecision StyleIcmDecide(const HeroContext& C, const HeroBrain& B, const Profile& P, double Tilt, Rng& R)
{
	(void)B;
	const PlayerView& V = C.View;
	if (!C.T || V.Hole.size() != 2 || !icmplay::InZone(*C.T))
	{
		return StyleDecideProfile(C, P, Tilt, R);
	}
	const Tournament& T = *C.T;
	const int Me = T.PlayerIndex(V.Seats[static_cast<size_t>(V.Me)].Id);
	if (Me < 0)
	{
		return StyleDecideProfile(C, P, Tilt, R);
	}
	const PublicSeat& Mine = V.Seats[static_cast<size_t>(V.Me)];
	// A stack-deciding call (§2.4.5: between two players): the bet or raise I face is all in, or calling it costs 35% of my
	// stack or more. Its maker is the villain. A blind posted all in is no bet to call off: with nobody's bet or raise to
	// face, the pot is unopened and the jam rule below plays it (it used to limp or fold there, even with aces).
	int VillainSeat = -1;
	for (size_t K = V.History.size(); K-- > 0 && VillainSeat < 0;)
	{
		const ActionRecord& A = V.History[K];
		if (A.Type == ActionType::Bet || A.Type == ActionType::Raise)
		{
			const int Idx = SeatIndex(V, A.Seat);
			VillainSeat = Idx >= 0 && Idx != V.Me ? Idx : -1;
		}
	}
	const bool FacingAllIn = VillainSeat >= 0 && V.Seats[static_cast<size_t>(VillainSeat)].AllIn;
	if (V.ToCall > 0 && VillainSeat >= 0 && (FacingAllIn || static_cast<double>(V.ToCall) >= 0.35 * static_cast<double>(Mine.Stack)))
	{
		const int Villain = T.PlayerIndex(V.Seats[static_cast<size_t>(VillainSeat)].Id);
		const double Bf = Villain >= 0 ? icmplay::BubbleFactor(T, Me, Villain) : 1.0;
		const double Eq = EquityVsModels(V.Hole, V.Board, ModelOpponents(V), 500, R);
		const double Need = icmplay::RequiredEquity(Bf, V.ToCall, V.Pot);
		return StyleChoice(V, Eq >= Need ? PlayerAction::Call() : PlayerAction::Fold(), Eq);
	}

	// First in before the flop, short enough for the push chart: jam on the chart's threshold, scaled by the factors.
	if (V.CurrentStreet == Street::Preflop)
	{
		const PreflopNode N = StylePreflopNode(V, V.History.size());
		if (N.K == PreflopNode::Kind::Unopened && N.Limpers == 0 && N.Me <= 7 && N.E <= 20.0)
		{
			std::vector<int> Behind;
			const int Count = static_cast<int>(V.Seats.size());
			for (int K = 1; K < Count; ++K)
			{
				const int I = (V.Me + K) % Count;
				if (!V.Seats[static_cast<size_t>(I)].Folded)
				{
					const int Idx = T.PlayerIndex(V.Seats[static_cast<size_t>(I)].Id);
					if (Idx >= 0)
					{
						Behind.push_back(Idx);
					}
				}
				if (I == V.BbIdx)
				{
					break;
				}
			}
			double CallersBf = 0, MineBf = 0;
			for (const int Idx : Behind)
			{
				CallersBf += icmplay::BubbleFactor(T, Idx, Me); // what it costs each of them to call it off
				MineBf += icmplay::BubbleFactor(T, Me, Idx);
			}
			const double Nb = static_cast<double>(std::max<size_t>(1, Behind.size()));
			CallersBf /= Nb;
			MineBf = Max(0.1, MineBf / Nb);
			const bool Satellite = T.Spec.SeatValueCents > 0;
			const double Cover = Satellite ? 1.0 : Min(IcmCoverCap, DetPow(Max(0.0, CallersBf), IcmCoverExp));
			const double Threshold = charts::charts_detail::JamShare(N.Me, N.E, N.Ante) * Cover / DetPow(MineBf, IcmHeroExp) * StyleTiltWide(Tilt);
			const int Cls = HandClass(V.Hole[0], V.Hole[1]);
			const bool Jam = charts::charts_detail::JamOrderShare(N.Me, N.Ante, Cls) <= Threshold;
			return StyleChoice(V, Jam ? bot_detail::AllIn(V) : (V.CanCheck ? PlayerAction::Check() : PlayerAction::Fold()),
				1.0 - charts::charts_detail::ClassMid(Cls));
		}
	}
	// Everything else: Balanced Reg with the payout ladder fully on (IcmAware 1.0, the bots' pressure).
	return StyleDecideProfile(C, P, Tilt, R);
}
} // namespace style_detail
} // namespace style
} // namespace ss
