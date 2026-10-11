#include "ShortStack/Game/Sizing.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/ChipSet.h"
#include "ShortStack/Game/Format.h"

#include <algorithm>
#include <string>
#include <vector>

// Bet sizing (next_segment_spec §6.2 D). core-tableui.

namespace ss
{
namespace sizing
{
namespace sizing_detail
{
enum class Kind : int
{
	Unopened,
	FacingRaise,
	Unbet,
	FacingBet,
};

Kind KindOf(const Spot& S)
{
	if (S.On == Street::Preflop)
	{
		return S.Raises == 0 ? Kind::Unopened : Kind::FacingRaise;
	}
	return S.CurrentBet <= 0 ? Kind::Unbet : Kind::FacingBet;
}

/** The smallest chip that makes every blind and ante. Never the 25,000: the orange 5,000s stay in play to the end, so a
 *  raise at 25,000/50,000 still goes up in fives (a 2.2x open is 110,000, not a min-raise). */
Chips UnitFor(Chips Sb, Chips Bb, Chips Ante)
{
	static const Chips Candidates[] = {5000, 1000, 500, 100, 25, 5, 1};
	for (const Chips U : Candidates)
	{
		if (U <= std::max<Chips>(1, Bb) && Sb % U == 0 && Bb % U == 0 && Ante % U == 0)
		{
			return U;
		}
	}
	return 1;
}

Chips Round(const Spot& S, double Raw)
{
	const double U = static_cast<double>(std::max<Chips>(1, S.Unit));
	return static_cast<Chips>(JsRound(Raw / U)) * std::max<Chips>(1, S.Unit);
}

Chips Clamp(const Spot& S, Chips To)
{
	return std::max(S.MinTo, std::min(S.MaxTo, To));
}

/** "2.2x", "3x": one decimal, a trailing .0 dropped. */
std::string Times(double X)
{
	std::string T = Fixed(X, 1);
	if (T.size() > 2 && T.compare(T.size() - 2, 2, ".0") == 0)
	{
		T.resize(T.size() - 2);
	}
	return T + "x";
}

Chips PotTo(const Spot& S)
{
	return S.CurrentBet + S.Pot + S.ToCall;
}

/** The base a multiple is of: the bet to match (the big blind when nobody has raised). */
Chips Base(const Spot& S)
{
	return std::max(S.CurrentBet, S.Bb);
}
} // namespace sizing_detail

Spot SpotOf(const Hand& H, int Seat)
{
	using namespace sizing_detail;
	Spot S;
	int Me = -1;
	for (size_t I = 0; I < H.Seats.size(); ++I)
	{
		Me = H.Seats[I].Seat == Seat ? static_cast<int>(I) : Me;
	}
	S.On = H.CurrentStreet;
	S.Pot = H.Pot();
	S.CurrentBet = H.CurrentBet;
	S.Bb = H.BigBlind;
	S.Ante = H.Ante;
	S.Unit = UnitFor(H.SmallBlind, H.BigBlind, H.Ante);
	for (const ActionRecord& A : H.History)
	{
		S.Raises += A.OnStreet == H.CurrentStreet && (A.Type == ActionType::Bet || A.Type == ActionType::Raise) ? 1 : 0;
	}
	if (Me < 0)
	{
		return S;
	}
	const HandSeat& Hero = H.Seats[static_cast<size_t>(Me)];
	S.ToCall = std::min(std::max<Chips>(0, H.CurrentBet - Hero.StreetBet), Hero.Stack);
	S.MaxTo = Hero.StreetBet + Hero.Stack;
	if (H.ToAct == Me)
	{
		const LegalActions L = H.GetLegalActions();
		S.IsBet = L.IsBet;
		if (L.CanRaise)
		{
			S.MinTo = L.MinRaiseTo;
			S.MaxTo = L.MaxRaiseTo;
		}
		else
		{
			// Can't raise (everyone else is all in, or a short all-in didn't reopen it): nothing to size.
			S.MaxTo = std::min(S.MaxTo, H.CurrentBet);
			S.MinTo = S.MaxTo;
		}
	}
	else
	{
		S.MinTo = std::min(H.CurrentBet + std::max(H.MinRaiseInc, H.BigBlind), S.MaxTo);
		S.IsBet = H.CurrentBet == 0;
	}
	Chips Biggest = 0;
	for (size_t I = 0; I < H.Seats.size(); ++I)
	{
		const HandSeat& O = H.Seats[I];
		if (static_cast<int>(I) != Me && !O.Folded)
		{
			Biggest = std::max(Biggest, O.Stack + O.StreetBet);
		}
	}
	S.Effective = std::min(Hero.Stack + Hero.StreetBet, Biggest > 0 ? Biggest : Hero.Stack + Hero.StreetBet);
	// In position: the last to act after the flop among those still able to act (the button first, then back from it).
	const int N = static_cast<int>(H.Seats.size());
	for (int K = 0; K < N; ++K)
	{
		const int Idx = (H.ButtonIndex() - K + N * 2) % N;
		const HandSeat& O = H.Seats[static_cast<size_t>(Idx)];
		if (!O.Folded && (!O.AllIn || Idx == Me))
		{
			S.InPosition = Idx == Me;
			break;
		}
	}
	return S;
}

std::vector<Preset> Presets(const Spot& S)
{
	using namespace sizing_detail;
	std::vector<Preset> Out;
	if (!S.CanRaise())
	{
		return Out;
	}
	auto Add = [&](const char* Label, Chips To, bool AllIn) {
		To = AllIn ? S.MaxTo : Clamp(S, To);
		if (!AllIn && To >= S.MaxTo)
		{
			return; // that's all in: the last preset says so
		}
		for (const Preset& P : Out)
		{
			if (P.To == To)
			{
				return;
			}
		}
		Out.push_back({Label, To, AllIn});
	};
	if (S.MinTo < S.MaxTo)
	{
		const double B = static_cast<double>(Base(S));
		switch (KindOf(S))
		{
		case Kind::Unopened:
			Add("MIN", S.MinTo, false);
			if (S.Ante > 0)
			{
				Add("2.2x", Round(S, B * 2.2), false);
				Add("2.5x", Round(S, B * 2.5), false);
				Add("3x", Round(S, B * 3.0), false);
			}
			else
			{
				Add("2.5x", Round(S, B * 2.5), false);
				Add("3x", Round(S, B * 3.0), false);
				Add("4x", Round(S, B * 4.0), false);
			}
			Add("POT", Round(S, static_cast<double>(PotTo(S))), false);
			break;
		case Kind::FacingRaise:
			Add("MIN", S.MinTo, false);
			Add("3x", Round(S, B * 3.0), false);
			Add("4x", Round(S, B * 4.0), false);
			Add("POT", Round(S, static_cast<double>(PotTo(S))), false);
			break;
		case Kind::Unbet:
		{
			const double P = static_cast<double>(S.Pot);
			Add("1/3", Round(S, P / 3.0), false);
			Add("1/2", Round(S, P * 0.5), false);
			Add("2/3", Round(S, P * 2.0 / 3.0), false);
			Add("3/4", Round(S, P * 0.75), false);
			Add("POT", Round(S, P), false);
			break;
		}
		case Kind::FacingBet:
			Add("MIN", S.MinTo, false);
			Add("2.5x", Round(S, B * 2.5), false);
			Add("3x", Round(S, B * 3.0), false);
			Add("POT", Round(S, static_cast<double>(PotTo(S))), false);
			break;
		}
	}
	// Smallest first, so the bumpers always step one way (a pot-sized raise can come under 4x).
	std::stable_sort(Out.begin(), Out.end(), [](const Preset& A, const Preset& B) { return A.To < B.To; });
	Add("ALL IN", S.MaxTo, true);
	return Out;
}

Chips Default(const Spot& S)
{
	using namespace sizing_detail;
	if (!S.CanRaise())
	{
		return S.MinTo;
	}
	const double B = static_cast<double>(Base(S));
	double Raw = 0.0;
	switch (KindOf(S))
	{
	case Kind::Unopened: Raw = S.Ante > 0 ? B * (S.Effective < 25 * S.Bb ? 2.0 : 2.2) : B * 3.0; break;
	case Kind::FacingRaise: Raw = B * (S.InPosition ? 3.0 : 4.0); break;
	case Kind::Unbet: Raw = static_cast<double>(S.Pot) * 2.0 / 3.0; break;
	case Kind::FacingBet: Raw = B * 3.0; break;
	}
	return Clamp(S, Round(S, Raw));
}

Chips Step(const Spot& S, Chips To, int Steps)
{
	const Chips Bb = std::max<Chips>(1, S.Bb);
	const Chips By = To < 20 * Bb ? Bb : 5 * Bb;
	return sizing_detail::Clamp(S, To + static_cast<Chips>(Steps) * By);
}

int Snap(const Spot& S, Chips To, int Dir)
{
	const std::vector<Preset> P = Presets(S);
	int Best = -1;
	for (size_t I = 0; I < P.size(); ++I)
	{
		const int Idx = static_cast<int>(I);
		if (Dir > 0 && P[I].To > To)
		{
			return Idx;
		}
		if (Dir < 0 && P[I].To < To)
		{
			Best = Idx;
		}
		if (Dir == 0)
		{
			const Chips D = P[I].To > To ? P[I].To - To : To - P[I].To;
			if (Best < 0 || D < (P[static_cast<size_t>(Best)].To > To ? P[static_cast<size_t>(Best)].To - To : To - P[static_cast<size_t>(Best)].To))
			{
				Best = Idx;
			}
		}
	}
	return Best;
}

const char* Verb(const Spot& S, Chips To)
{
	if (S.MaxTo > 0 && To >= S.MaxTo)
	{
		return "ALL IN";
	}
	return S.On != Street::Preflop && S.CurrentBet <= 0 ? "BET" : "RAISE TO";
}

std::string Detail(const Spot& S, Chips To)
{
	using namespace sizing_detail;
	const std::string Bbs = chipset::InBb(To, S.Bb);
	if (S.MaxTo > 0 && To >= S.MaxTo)
	{
		return Bbs;
	}
	if (KindOf(S) == Kind::Unbet)
	{
		if (S.Pot <= 0)
		{
			return Bbs;
		}
		const Chips Pct = (To * 200 + S.Pot) / (S.Pot * 2);
		return std::to_string(Pct) + "% POT";
	}
	const Chips B = Base(S);
	const std::string X = B > 0 ? Times(static_cast<double>(To) / static_cast<double>(B)) : std::string();
	return X.empty() ? Bbs : Bbs.empty() ? X : X + " \xC2\xB7 " + Bbs;
}

std::string Describe(const Spot& S, Chips To)
{
	const std::string D = Detail(S, To);
	return std::string(Verb(S, To)) + " " + Grouped(To) + (D.empty() ? std::string() : " \xC2\xB7 " + D);
}
} // namespace sizing
} // namespace ss
