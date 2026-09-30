#include "ShortStack/AI/Grading.h"
#include "../StrictFloat.h"

#include "ShortStack/Rng.h"

#include <algorithm>
#include <cmath>

namespace ss
{
namespace grading_detail
{
/** How often one opponent folds to a bet of Ratio x the pot. */
double FoldProb(const OpponentModel& M, double Ratio, bool Preflop)
{
	const double R = Max(0.0, Ratio);
	const double Size = R / (R + 0.5); // 0 for tiny bets, 0.5 at half pot, 0.67 at pot
	if (Preflop)
	{
		// Players yet to act preflop fold most hands to any real raise.
		if (M.Band.Max >= 0.9)
		{
			return 0.55 + 0.35 * Size;
		}
		return 0.1 + 0.45 * Size;
	}
	const double MaxFold = M.Aggr >= 1 ? 0.3 : M.Calls >= 1 ? 0.5 : 0.62;
	return MaxFold * Size;
}

bool IsInPosition(const PlayerView& V)
{
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

double Realization(const PlayerView& V)
{
	const bool Pre = V.CurrentStreet == Street::Preflop;
	const bool Ip = IsInPosition(V);
	return V.CurrentStreet == Street::River ? 1.0 : Pre ? (Ip ? 0.9 : 0.8) : Ip ? 0.95 : 0.85;
}

double RaiseEV(const PlayerView& V, const std::vector<OpponentModel>& Models, double Equity, double Realize, double To)
{
	const PublicSeat& Me = V.Seats[static_cast<size_t>(V.Me)];
	const double P = static_cast<double>(V.Pot);
	const double B = To - static_cast<double>(Me.StreetBet); // chips we add
	const double Ratio = (To - static_cast<double>(V.CurrentBet)) / Max(1.0, P);
	const bool Pre = V.CurrentStreet == Street::Preflop;
	double FoldAll = 1.0;
	for (const OpponentModel& M : Models)
	{
		FoldAll *= FoldProb(M, Ratio, Pre);
	}
	// Largest amount any single caller would need to add.
	double D = 0.0;
	for (size_t I = 0; I < V.Seats.size(); ++I)
	{
		const PublicSeat& S = V.Seats[I];
		if (S.Folded || static_cast<int>(I) == V.Me)
		{
			continue;
		}
		D = Max(D, Min(To - static_cast<double>(S.StreetBet), static_cast<double>(S.Stack)));
	}
	const double EqCalled = Equity * (1.0 - 0.35 * (1.0 - Equity) * Min(1.0, Ratio));
	const double R = To >= static_cast<double>(V.MaxRaiseTo) ? 1.0 : Realize;
	return FoldAll * P + (1.0 - FoldAll) * (EqCalled * R * (P + B + D) - B);
}

std::string RaiseLabel(const PlayerView& V, double To)
{
	if (To == static_cast<double>(V.MaxRaiseTo))
	{
		return "All-in";
	}
	return std::string(V.CurrentBet == 0 ? "Bet" : "Raise to") + " " + JsNumber(To);
}
} // namespace grading_detail

const char* GradeName(Grade G)
{
	switch (G)
	{
	case Grade::Best: return "Best";
	case Grade::Good: return "Good";
	case Grade::Inaccuracy: return "Inaccuracy";
	case Grade::Mistake: return "Mistake";
	default: return "Blunder";
	}
}

double GradeScore(Grade G)
{
	switch (G)
	{
	case Grade::Best: return 100.0;
	case Grade::Good: return 88.0;
	case Grade::Inaccuracy: return 62.0;
	case Grade::Mistake: return 30.0;
	default: return 0.0;
	}
}

DecisionAnalysis AnalyzeDecision(const PlayerView& View, Rng& R, int Iterations)
{
	using namespace grading_detail;
	DecisionAnalysis A;
	A.View = View;
	A.Models = ModelOpponents(View);
	A.Equity = EquityVsModels(View.Hole, View.Board, A.Models, Iterations, R);
	const PublicSeat& Me = View.Seats[static_cast<size_t>(View.Me)];
	const double P = static_cast<double>(View.Pot);
	const double C = static_cast<double>(View.ToCall);
	const double Realize = Realization(View);

	if (View.ToCall > 0)
	{
		A.Options.push_back({"Fold", PlayerAction::Fold(), 0.0});
	}
	if (View.CanCheck)
	{
		A.Options.push_back({"Check", PlayerAction::Check(), A.Equity * Realize * P});
	}
	else
	{
		const bool CallAllIn = View.ToCall >= Me.Stack;
		const double Rz = CallAllIn ? 1.0 : Realize;
		A.Options.push_back({"Call", PlayerAction::Call(), A.Equity * Rz * (P + C) - C});
	}

	if (View.CanRaise)
	{
		std::vector<double> Sizes;
		const double MinTo = static_cast<double>(View.MinRaiseTo);
		const double MaxTo = static_cast<double>(View.MaxRaiseTo);
		auto Add = [&](double To) {
			const double V = Max(MinTo, Min(MaxTo, JsRound(To)));
			if (std::find(Sizes.begin(), Sizes.end(), V) == Sizes.end())
			{
				Sizes.push_back(V);
			}
		};
		Add(MinTo);
		const double Cb = static_cast<double>(View.CurrentBet);
		if (View.CurrentBet == 0)
		{
			Add(P * 0.5);
			Add(P * 0.75);
			Add(P);
		}
		else
		{
			Add(Cb * 2.5 + (P - Cb) * 0.2);
			Add(Cb * 3.5 + (P - Cb) * 0.3);
		}
		Add(MaxTo);
		std::stable_sort(Sizes.begin(), Sizes.end());
		for (double To : Sizes)
		{
			A.Options.push_back({RaiseLabel(View, To), PlayerAction::RaiseTo(To), RaiseEV(View, A.Models, A.Equity, Realize, To)});
		}
	}
	A.PotOdds = View.ToCall > 0 ? C / (P + C) : 0.0;
	return A;
}

OptionEV EvaluateChoice(const DecisionAnalysis& Analysis, const PlayerAction& Action)
{
	using namespace grading_detail;
	const PlayerView& V = Analysis.View;
	if (Action.Type == PlayerAction::Kind::Fold)
	{
		return {"Fold", Action, V.CanCheck ? -1e-9 : 0.0};
	}
	if (Action.Type == PlayerAction::Kind::Check || Action.Type == PlayerAction::Kind::Call)
	{
		for (const OptionEV& O : Analysis.Options)
		{
			if (O.Label == "Check" || (Action.Type == PlayerAction::Kind::Call && O.Label == "Call"))
			{
				return O;
			}
		}
		return {"Check", Action, 0.0};
	}
	const double To = Max(static_cast<double>(V.MinRaiseTo), Min(static_cast<double>(V.MaxRaiseTo), Action.To));
	return {RaiseLabel(V, To), PlayerAction::RaiseTo(To), RaiseEV(V, Analysis.Models, Analysis.Equity, Realization(V), To)};
}

DecisionGrade GradeDecision(const DecisionAnalysis& Analysis, const PlayerAction& Action)
{
	const PlayerView& V = Analysis.View;
	DecisionGrade G;
	G.Chosen = EvaluateChoice(Analysis, Action);
	OptionEV Best = Analysis.Options.front();
	for (const OptionEV& O : Analysis.Options)
	{
		if (O.Ev > Best.Ev)
		{
			Best = O;
		}
	}
	if (G.Chosen.Ev > Best.Ev)
	{
		Best = G.Chosen;
	}
	G.Best = Best;
	const double Loss = Max(0.0, Best.Ev - G.Chosen.Ev);
	const double Scale = Max(static_cast<double>(V.Pot), static_cast<double>(V.BigBlind) * 4.0);
	const double Ratio = Loss / Scale;
	G.Result = Ratio < 0.04 ? Grade::Best : Ratio < 0.12 ? Grade::Good : Ratio < 0.28 ? Grade::Inaccuracy : Ratio < 0.55 ? Grade::Mistake : Grade::Blunder;
	G.OnStreet = V.CurrentStreet;
	G.Equity = Analysis.Equity;
	G.PotOdds = Analysis.PotOdds;
	G.LossBB = Loss / Max(1.0, static_cast<double>(V.BigBlind));
	const std::string EqPct = JsNumber(JsRound(Analysis.Equity * 100.0));
	const std::string NeedPct = JsNumber(JsRound(Analysis.PotOdds * 100.0));
	if (G.Result == Grade::Best || G.Result == Grade::Good)
	{
		G.Note = V.ToCall > 0 ? "Equity about " + EqPct + "% vs " + NeedPct + "% needed to call." : "Equity about " + EqPct + "% against their likely range.";
	}
	else
	{
		G.Note = Best.Label + " was better by " + JsToFixed1(G.LossBB) + " BB. " +
			(V.ToCall > 0 ? "You had about " + EqPct + "% equity and needed " + NeedPct + "%." : "You had about " + EqPct + "% equity.");
	}
	return G;
}

double Accuracy(const std::vector<DecisionGrade>& Grades)
{
	if (Grades.empty())
	{
		return 0.0;
	}
	double Sum = 0.0;
	for (const DecisionGrade& G : Grades)
	{
		Sum += GradeScore(G.Result);
	}
	return Sum / static_cast<double>(Grades.size());
}
} // namespace ss
