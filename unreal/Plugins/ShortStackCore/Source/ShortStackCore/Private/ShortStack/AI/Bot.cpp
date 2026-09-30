#include "ShortStack/AI/Bot.h"

#include "ShortStack/AI/Model.h"
#include "ShortStack/Equity.h"
#include "ShortStack/Rng.h"

#include <cmath>

// Note for maintainers: this mirrors web/src/core/ai/bot.ts expression by
// expression. Keep the operand order of arithmetic and the order of RNG draws
// identical, or the golden vectors will (rightly) fail.

namespace ss
{
namespace bot_detail
{
double OpenRange(Position P)
{
	switch (P)
	{
	case Position::UTG: return 0.12;
	case Position::UTG1: return 0.135;
	case Position::MP: return 0.155;
	case Position::LJ: return 0.18;
	case Position::HJ: return 0.22;
	case Position::CO: return 0.28;
	case Position::BTN: return 0.42;
	case Position::SB: return 0.36;
	default: return 0.3; // BB
	}
}

Profile Tilted(const Profile& P, double Tilt)
{
	if (Tilt <= 0.0)
	{
		return P;
	}
	Profile T = P;
	T.OpenWidth = P.OpenWidth * (1.0 + Tilt * 0.8);
	T.Aggression = Min(1.0, P.Aggression + Tilt * 0.25);
	T.Bluff = Min(1.0, P.Bluff + Tilt * 0.3);
	T.Stickiness = Min(0.95, P.Stickiness + Tilt * 0.25);
	T.PushFold = Max(0.0, P.PushFold - Tilt * 0.4);
	T.CallWidth = P.CallWidth * (1.0 + Tilt * 0.5);
	return T;
}

PlayerAction Legalize(const PlayerView& V, const PlayerAction& A)
{
	if (A.Type == PlayerAction::Kind::Raise)
	{
		if (!V.CanRaise)
		{
			return V.ToCall > 0 ? PlayerAction::Call() : PlayerAction::Check();
		}
		double To = JsRound(A.To);
		const double MinTo = static_cast<double>(V.MinRaiseTo);
		const double MaxTo = static_cast<double>(V.MaxRaiseTo);
		if (To < MinTo)
		{
			To = MinTo;
		}
		if (To > MaxTo)
		{
			To = MaxTo;
		}
		// Don't leave a sliver behind: commit fully when most of the stack goes in.
		if (To >= MaxTo * 0.8)
		{
			To = MaxTo;
		}
		if (To <= static_cast<double>(V.CurrentBet))
		{
			return V.ToCall > 0 ? PlayerAction::Call() : PlayerAction::Check();
		}
		return PlayerAction::RaiseTo(To);
	}
	if (A.Type == PlayerAction::Kind::Check && !V.CanCheck)
	{
		return PlayerAction::Fold();
	}
	if (A.Type == PlayerAction::Kind::Fold && V.CanCheck)
	{
		return PlayerAction::Check();
	}
	return A;
}

double ThinkTime(const Profile& P, Rng& R, double Strength, double Difficulty, bool Snap)
{
	if (P.ThinkBase == 0.0)
	{
		return 0.0;
	}
	const double Noise = R.Range(0.75, 1.25);
	if (Snap)
	{
		return R.Range(250.0, 800.0);
	}
	double T;
	switch (P.Timing)
	{
	case TimingStyle::Honest:
	{
		// Hard decisions take longer; monsters are often snapped.
		const double Base = P.ThinkBase * 0.5 + Difficulty * P.ThinkVar;
		const double Snapped = (Strength > 0.85 && R.Chance(0.5)) ? -P.ThinkBase * 0.3 : 0.0;
		T = Base + Snapped;
		break;
	}
	case TimingStyle::Reverse:
		// Acts out the opposite: tanks with strength, snaps with air.
		T = P.ThinkBase * 0.4 + (Strength > 0.7 ? P.ThinkVar * 1.1 : Strength < 0.3 ? 0.0 : P.ThinkVar * 0.4);
		break;
	default:
	{
		const double Roll = R.Next();
		T = P.ThinkBase * 0.6 + Roll * P.ThinkVar;
		break;
	}
	}
	return Max(300.0, T * Noise);
}

double BbOf(const PlayerView& V)
{
	return Max(1.0, static_cast<double>(V.BigBlind));
}

PlayerAction AllIn(const PlayerView& V)
{
	return PlayerAction::RaiseTo(static_cast<double>(V.MaxRaiseTo));
}

bool IsInPosition(const PlayerView& V)
{
	// Last active player to act postflop = closest to the button going backwards.
	const int N = static_cast<int>(V.Seats.size());
	for (int K = 0; K < N; ++K)
	{
		const int I = (V.ButtonIdx - K + N) % N;
		if (!V.Seats[static_cast<size_t>(I)].Folded)
		{
			return I == V.Me;
		}
	}
	return false;
}

BotDecision MakeChoice(const PlayerAction& Action, double ThinkMs, double Equity)
{
	BotDecision D;
	D.Action = Action;
	D.ThinkMs = ThinkMs;
	D.Equity = Equity;
	return D;
}

BotDecision Preflop(const PlayerView& V, const Profile& P, double Pressure, const BotContext& Ctx)
{
	Rng& R = *Ctx.R;
	const double Bb = BbOf(V);
	const PublicSeat& Me = V.Seats[static_cast<size_t>(V.Me)];
	const double StackBB = static_cast<double>(Me.Stack + Me.StreetBet) / Bb;
	const double Pct = HandPercentile(V.Hole[0], V.Hole[1]);
	const Position Pos = PositionOf(V, V.Me);
	const StreetSummary Sum = PreflopSummary(V);
	const double Strength = 1.0 - Pct;
	auto Fold = [&](bool Snap) {
		const PlayerAction A = V.CanCheck ? PlayerAction::Check() : PlayerAction::Fold();
		const double T = ThinkTime(P, R, Strength, 0.1, Snap);
		return MakeChoice(A, T, Strength);
	};
	auto Act = [&](const PlayerAction& A, double Difficulty) {
		const double T = ThinkTime(P, R, Strength, Difficulty, false);
		return MakeChoice(A, T, Strength);
	};

	if (Sum.Raises == 0)
	{
		if (V.Me == V.BbIdx)
		{
			// Limped pot, big blind option.
			if (Pct <= 0.12 * P.OpenWidth && R.Chance(P.Aggression))
			{
				return Act(PlayerAction::RaiseTo(Bb * (3.0 + Sum.Limpers)), 0.3);
			}
			return Act(PlayerAction::Check(), 0.1);
		}
		const int Behind = PlayersBehind(V, V.Me);
		if (StackBB <= 14.0 && Sum.Limpers == 0)
		{
			const double Chart = PushRange(StackBB, Behind) * (1.0 - 0.35 * Pressure);
			const double Sloppy = 0.16 * P.OpenWidth;
			const double Push = P.PushFold * Chart + (1.0 - P.PushFold) * Sloppy;
			if (Pct <= Push)
			{
				return Act(AllIn(V), std::fabs(Pct - Push) < 0.05 ? 0.7 : 0.3);
			}
			if (StackBB > 9.0 && P.PushFold < 0.5 && Pct <= 0.2 * P.OpenWidth)
			{
				return Act(PlayerAction::RaiseTo(Bb * 2.0), 0.3);
			}
			if (V.CanCheck)
			{
				return Act(PlayerAction::Check(), 0.1);
			}
			return Fold(Pct > Push + 0.15);
		}
		const double Open = OpenRange(Pos) * P.OpenWidth * (1.0 - 0.3 * Pressure);
		if (Sum.Limpers > 0)
		{
			if (Pct <= Open * 0.55 && R.Chance(Min(1.0, P.Aggression + 0.1)))
			{
				return Act(PlayerAction::RaiseTo(Bb * (3.5 + Sum.Limpers) * P.Sizing), 0.35);
			}
			if (Pct <= Open * 1.3 && R.Chance(0.5 + P.LimpRate * 0.5))
			{
				return Act(PlayerAction::Call(), 0.3);
			}
			return Fold(Pct > Open * 1.6);
		}
		if (Pct <= Open)
		{
			if (Pct > 0.06 && R.Chance(P.LimpRate))
			{
				return Act(PlayerAction::Call(), 0.2);
			}
			const double Size = StackBB > 40.0 ? ((Pos == Position::BTN || Pos == Position::CO) ? 2.2 : 2.4) : 2.05;
			return Act(PlayerAction::RaiseTo(Bb * Size * P.Sizing), Pct > Open * 0.8 ? 0.5 : 0.2);
		}
		if (Pct <= Open * 1.6 && R.Chance(P.LimpRate * 0.6))
		{
			return Act(PlayerAction::Call(), 0.3);
		}
		return Fold(Pct > Open * 1.3);
	}

	// Facing one or more raises.
	const double ToCall = static_cast<double>(V.ToCall);
	const int RaiserIdx = Sum.LastRaiserSeat >= 0 ? SeatIndex(V, Sum.LastRaiserSeat) : -1;
	const PublicSeat* Raiser = RaiserIdx >= 0 ? &V.Seats[static_cast<size_t>(RaiserIdx)] : nullptr;
	bool SomeoneAllIn = false;
	for (size_t I = 0; I < V.Seats.size(); ++I)
	{
		if (static_cast<int>(I) != V.Me && !V.Seats[I].Folded && V.Seats[I].AllIn)
		{
			SomeoneAllIn = true;
			break;
		}
	}
	const bool BigCall = ToCall >= static_cast<double>(Me.Stack) * 0.35;

	if (SomeoneAllIn || BigCall || (Raiser && static_cast<double>(Raiser->Stack) < Bb * 2.0))
	{
		// A stack-deciding spot: compare equity against the modelled ranges.
		const std::vector<OpponentModel> Models = ModelOpponents(V);
		const double Eq = EquityVsModels(V.Hole, std::vector<Card>(), Models, Ctx.Fast ? 120 : 500, R);
		const double PotOdds = ToCall / static_cast<double>(V.Pot + V.ToCall);
		const double Req = PotOdds + Pressure * 0.12 - P.Stickiness * 0.08;
		const double Difficulty = Max(0.0, 1.0 - std::fabs(Eq - Req) * 5.0);
		if (Eq >= Req)
		{
			if (V.CanRaise && Eq > 0.62 && !SomeoneAllIn && R.Chance(P.Aggression))
			{
				const double T = ThinkTime(P, R, Eq, Difficulty, false);
				return MakeChoice(AllIn(V), T, Eq);
			}
			const double T = ThinkTime(P, R, Eq, Difficulty, false);
			return MakeChoice(PlayerAction::Call(), T, Eq);
		}
		const double T = ThinkTime(P, R, Eq, Difficulty, false);
		return MakeChoice(PlayerAction::Fold(), T, Eq);
	}

	const bool Ip = IsInPosition(V);
	const Position RaiserPos = RaiserIdx >= 0 ? PositionOf(V, RaiserIdx) : Position::UTG;
	const bool LateRaiser = RaiserPos == Position::BTN || RaiserPos == Position::CO || RaiserPos == Position::SB;
	const double LastRaiseTo = static_cast<double>(Sum.LastRaiseTo);
	const double RaiseBB = LastRaiseTo / Bb;
	const double MyTotal = static_cast<double>(Me.Stack + Me.StreetBet);

	if (Sum.Raises == 1)
	{
		// Short enough to re-shove?
		if (StackBB <= 22.0)
		{
			const double Reshove = (0.06 + (LateRaiser ? 0.07 : 0.0)) * (1.0 - 0.3 * Pressure) * (0.6 + 0.4 * P.OpenWidth);
			if (Pct <= Reshove)
			{
				return Act(AllIn(V), 0.5);
			}
			if (V.Me == V.BbIdx && RaiseBB <= 2.2 && Pct <= 0.3 * P.CallWidth)
			{
				return Act(PlayerAction::Call(), 0.4);
			}
			return Fold(Pct > 0.4);
		}
		const double ValueThree = P.ThreeBet * (Ip ? 1.0 : 0.85) * (LateRaiser ? 1.4 : 1.0);
		if (Pct <= ValueThree)
		{
			const double To = LastRaiseTo * (Ip ? 3.0 : 3.6) * P.Sizing;
			return Act(To > MyTotal * 0.4 ? AllIn(V) : PlayerAction::RaiseTo(To), 0.3);
		}
		if (Pct > 0.12 && Pct < 0.3 && LateRaiser && R.Chance(P.Bluff * 0.25))
		{
			return Act(PlayerAction::RaiseTo(LastRaiseTo * (Ip ? 3.0 : 3.6)), 0.6);
		}
		double Call = V.Me == V.BbIdx ? 0.38 : V.Me == V.SbIdx ? 0.07 : Ip ? 0.12 : 0.09;
		Call *= P.CallWidth * Min(1.2, 2.5 / Max(2.0, RaiseBB));
		if (LateRaiser)
		{
			Call *= 1.25;
		}
		if (StackBB < 30.0)
		{
			Call *= 0.6;
		}
		if (Pct <= Call)
		{
			return Act(PlayerAction::Call(), Pct > Call * 0.8 ? 0.6 : 0.3);
		}
		return Fold(Pct > Call * 1.8);
	}

	// Facing a 3-bet or more.
	const double FourBet = 0.022 * (1.0 + P.Bluff);
	if (Pct <= FourBet)
	{
		const double To = LastRaiseTo * 2.3;
		return Act(StackBB < 60.0 || To > MyTotal * 0.4 ? AllIn(V) : PlayerAction::RaiseTo(To), 0.4);
	}
	const bool Cheap = ToCall < static_cast<double>(Me.Stack) * 0.12;
	if (Pct <= 0.055 * P.CallWidth * (Cheap ? 1.5 : 1.0))
	{
		return Act(PlayerAction::Call(), 0.6);
	}
	return Fold(false);
}

BotDecision Postflop(const PlayerView& V, const Profile& P, double Pressure, const BotContext& Ctx)
{
	Rng& R = *Ctx.R;
	const PublicSeat& Me = V.Seats[static_cast<size_t>(V.Me)];
	const std::vector<OpponentModel> Models = ModelOpponents(V);
	const int Opps = static_cast<int>(Models.size());
	const double Eq = Ctx.Fast ? HeuristicEquity(V.Hole, V.Board, Opps) : EquityVsModels(V.Hole, V.Board, Models, 260, R);
	const double Pot = static_cast<double>(V.Pot);
	const double ToCall = static_cast<double>(V.ToCall);
	const double CurrentBet = static_cast<double>(V.CurrentBet);
	const bool Ip = IsInPosition(V);
	const StreetSummary Sum = PreflopSummary(V);
	const bool Pfa = Sum.LastRaiserSeat == Me.Seat;
	const double Bb = BbOf(V);

	auto BetTo = [&](double Fraction) {
		const double Size = Max(Bb, Pot * Fraction * P.Sizing);
		return PlayerAction::RaiseTo(CurrentBet + Size);
	};
	auto Out = [&](const PlayerAction& A, double Threshold) {
		const double Difficulty = Max(0.0, 1.0 - std::fabs(Eq - Threshold) * 4.0);
		const double T = ThinkTime(P, R, Eq, Difficulty, false);
		return MakeChoice(A, T, Eq);
	};

	if (V.ToCall == 0)
	{
		const double ValueTh = 0.6 + 0.07 * (Opps - 1);
		if (Eq >= ValueTh)
		{
			const bool Slowplay = Eq > 0.9 && R.Chance(0.25 * (1.0 - P.Aggression));
			if (!Slowplay && R.Chance(Min(1.0, P.Aggression + 0.1)))
			{
				double Big;
				if (V.CurrentStreet == Street::River && Eq > 0.85)
				{
					const double Roll = R.Next();
					Big = 0.75 + 0.25 * Roll;
				}
				else
				{
					const double Roll = R.Next();
					Big = 0.5 + 0.3 * Roll;
				}
				return Out(BetTo(Big), ValueTh);
			}
			return Out(PlayerAction::Check(), ValueTh);
		}
		if (Pfa && V.CurrentStreet == Street::Flop && R.Chance(P.Cbet * PowInt(0.55, Opps - 1)))
		{
			const double Roll = R.Next();
			return Out(BetTo(0.33 + 0.2 * Roll), ValueTh);
		}
		if (Eq >= 0.3 && V.CurrentStreet != Street::River && R.Chance(P.Aggression * 0.3))
		{
			const double Roll = R.Next();
			return Out(BetTo(0.45 + 0.25 * Roll), ValueTh);
		}
		if (Eq < 0.3 && Opps == 1 && R.Chance(P.Bluff * (Ip ? 1.0 : 0.6) * (V.CurrentStreet == Street::River ? 0.8 : 1.0)))
		{
			const double Roll = R.Next();
			return Out(BetTo(0.35 + 0.3 * Roll), ValueTh);
		}
		return Out(PlayerAction::Check(), ValueTh);
	}

	const double PotOdds = ToCall / (Pot + ToCall);
	const double Commitment = ToCall / Max(1.0, static_cast<double>(Me.Stack));
	const double Req = PotOdds * (1.0 - P.Stickiness * 0.45) + Pressure * 0.1 * (Commitment > 0.3 ? 1.0 : 0.3);
	const double RaiseTh = 0.78 + 0.05 * (Opps - 1);
	if (Eq >= RaiseTh && V.CanRaise && R.Chance(P.Aggression))
	{
		const double To = CurrentBet * 2.6 + (Pot - CurrentBet) * 0.25;
		return Out(To > static_cast<double>(V.MaxRaiseTo) * 0.55 ? AllIn(V) : PlayerAction::RaiseTo(To), RaiseTh);
	}
	if (Eq >= Req)
	{
		return Out(PlayerAction::Call(), Req);
	}
	if (V.CanRaise && Opps == 1 && Eq > 0.22 && V.CurrentStreet != Street::River && R.Chance(P.Bluff * 0.18))
	{
		return Out(PlayerAction::RaiseTo(CurrentBet * 2.7), Req);
	}
	return Out(PlayerAction::Fold(), Req);
}
} // namespace bot_detail

double PushRange(double StackBB, int Behind)
{
	const double R = (1.15 * 1.9) / (Max(1.0, StackBB) * (0.25 + 0.12 * Behind));
	return Max(0.03, Min(1.0, R));
}

BotDecision Decide(const PlayerView& View, const BotContext& Ctx)
{
	using namespace bot_detail;
	const Profile P = Tilted(*Ctx.Prof, Ctx.Tilt);
	const double Pressure = Min(1.0, Ctx.IcmPressure * P.IcmAware);
	BotDecision D = View.CurrentStreet == Street::Preflop ? Preflop(View, P, Pressure, Ctx) : Postflop(View, P, Pressure, Ctx);
	D.Action = Legalize(View, D.Action);
	return D;
}
} // namespace ss
