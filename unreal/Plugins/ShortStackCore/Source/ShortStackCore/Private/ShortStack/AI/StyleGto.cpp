#include "ShortStack/AI/Style.h"
#include "../StrictFloat.h"

#include "StyleDetail.h"

#include "ShortStack/AI/Model.h"
#include "ShortStack/Cards.h"
#include "ShortStack/Equity.h"
#include "ShortStack/Evaluator.h"
#include "ShortStack/Rng.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

// GTO (update 2 §2.4.2): preflop from charts (Tests/charts/gto_preflop.txt, compiled by tools/chartgen, over 25 big blinds;
// Push/Fold's Nash charts at 15 and under, its fallback profile between, since C1's calibration: StylePushFold.cpp's
// PushFoldChartDepth), and heads-up after the flop from equilibrium identities over the range it actually
// holds: value and bluff regions by the bluff-to-value ratio, defence by MDF. Multiway pots and stack-deciding spots go to
// the engine's decision with GTO's fallback profile. Every mix is drawn with style::Mix, so it draws nothing from the
// tournament's R; only the fallback does, exactly as a bot would. An approximation, not a solver: the tile says so.
// core-styles.

namespace ss
{
namespace charts
{
namespace charts_detail
{
const chart_data::GtoRow* GtoFind(int SpotKind, int Pos, int Vs, int Band)
{
	// The band asked for, else the nearest one written.
	static const int Order[3][3] = {{0, 1, 2}, {1, 2, 0}, {2, 1, 0}};
	for (int K = 0; K < 3; ++K)
	{
		const int B = Order[Band][K];
		for (int I = 0; I < chart_data::GtoRows; ++I)
		{
			const chart_data::GtoRow& R = chart_data::Gto[I];
			if (R.Spot == SpotKind && R.Pos == Pos && R.Vs == Vs && R.Band == B)
			{
				return &R;
			}
		}
	}
	return nullptr;
}

/** How far into a region of Share a class plays: 1 inside it, 0 outside, linear within 2 points of the edge (§2.4.2). */
double GtoIn(double Share, double Mid)
{
	if (Share <= 0.0)
	{
		return 0.0;
	}
	const double F = (Share + 0.02 - Mid) / 0.04;
	return F <= 0.0 ? 0.0 : F >= 1.0 ? 1.0 : F;
}

double GtoShareOf(const chart_data::Bits169& B)
{
	int Combos = 0;
	for (int H = 0; H < 169; ++H)
	{
		Combos += chart_data::HasClass(B, H) ? HandClassCombos(H) : 0;
	}
	return static_cast<double>(Combos) / 1326.0;
}

int GtoPostOrder(int Pos)
{
	return Pos == 7 ? 0 : Pos == 8 ? 1 : Pos + 2;
}

Mixed PreflopWide(Spot S, int Position, int VsPosition, double EffBb, int HandClass, double Wide)
{
	Mixed M;
	const int Band = EffBb <= 25.0 ? 0 : EffBb <= 40.0 ? 1 : 2;
	const double Mid = ClassMid(HandClass);
	switch (S)
	{
	case charts::Spot::Rfi:
	{
		const chart_data::GtoRow* R = GtoFind(0, Position, 0, Band);
		M.Raise = R ? GtoIn(R->A * Wide, Mid) : 0.0;
		return M;
	}
	case charts::Spot::BbVsOpen:
	{
		const chart_data::GtoRow* R = GtoFind(1, 8, VsPosition, Band);
		if (R)
		{
			M.Raise = GtoIn(R->B * Wide, Mid);
			M.Call = Max(0.0, GtoIn(R->A * Wide, Mid) - M.Raise);
		}
		return M;
	}
	case charts::Spot::IpVsOpen:
	{
		const chart_data::GtoRow* R = GtoFind(2, Position, VsPosition, Band);
		if (R)
		{
			const bool Bluff = chart_data::HasClass(R->Bluff, HandClass);
			const double Value = GtoIn(R->B * Wide, Mid);
			M.Raise = Bluff ? 1.0 : Value;
			M.Call = Bluff ? 0.0 : Max(0.0, GtoIn((R->A + R->B) * Wide, Mid) - Value);
		}
		return M;
	}
	case charts::Spot::Vs3Bet:
	{
		// Position: mine; VsPosition: the 3-bettor's. In position after the flop or not.
		const int Side = GtoPostOrder(Position) > GtoPostOrder(VsPosition) ? 0 : 1;
		const chart_data::GtoRow* R = GtoFind(3, Side, 0, Band);
		if (R)
		{
			const bool Four = chart_data::HasClass(R->Set, HandClass) || chart_data::HasClass(R->Bluff, HandClass);
			M.Raise = Four ? 1.0 : 0.0;
			M.Call = Four ? 0.0 : GtoIn(GtoShareOf(R->Set) + R->B * Wide, Mid);
		}
		return M;
	}
	case charts::Spot::Vs4Bet:
	{
		const chart_data::GtoRow* R = GtoFind(4, 0, 0, Band);
		if (R)
		{
			M.Raise = chart_data::HasClass(R->Set, HandClass) ? 1.0 : chart_data::HasClass(R->Mix, HandClass) ? R->MixPct : 0.0;
		}
		return M;
	}
	}
	return M;
}
} // namespace charts_detail

Mixed Preflop(Spot S, int Position, int VsPosition, double EffBb, int HandClass)
{
	return charts_detail::PreflopWide(S, Position, VsPosition, EffBb, HandClass, 1.0);
}
} // namespace charts

namespace style
{
namespace style_detail
{
PreflopPlan StyleGtoPlan(const PreflopNode& N, int HandClass, double Wide)
{
	if (N.E <= 25.0)
	{
		// Push/Fold's charts, to their depth (15 BB, C1); from there to 25 they're unknown and the fallback profile plays.
		return StylePushFoldPlan(N, HandClass, Wide);
	}
	PreflopPlan Plan;
	Plan.Known = true;
	const bool Deep = N.E > 40.0;
	switch (N.K)
	{
	case PreflopNode::Kind::Unopened:
		if (N.Me == 8)
		{
			// The big blind's option over limpers: raise the top 12%, check the rest.
			Plan.F.Raise = charts::charts_detail::GtoIn(0.12 * Wide, charts::charts_detail::ClassMid(HandClass));
			Plan.RaiseToBb = 3.0 + N.Limpers;
			return Plan;
		}
		Plan.F = charts::charts_detail::PreflopWide(charts::Spot::Rfi, N.Me, 0, N.E, HandClass, Wide);
		if (N.Limpers > 0)
		{
			// Isolate or fold: the open's top 60%, to 3 big blinds and one more a limper. Never limp behind.
			Plan.F = charts::charts_detail::PreflopWide(charts::Spot::Rfi, N.Me, 0, N.E, HandClass, Wide * 0.6);
			Plan.RaiseToBb = 3.0 + N.Limpers;
			return Plan;
		}
		Plan.RaiseToBb = N.Me == 7 ? 3.0 : (Deep && N.Me <= 4) ? 2.5 : 2.2;
		return Plan;
	case PreflopNode::Kind::VsOpen:
	{
		const bool Bb = N.Me == 8;
		Plan.F = Bb ? charts::charts_detail::PreflopWide(charts::Spot::BbVsOpen, 8, N.Vs, N.E, HandClass, Wide) : charts::charts_detail::PreflopWide(charts::Spot::IpVsOpen, N.Me, N.Vs, N.E, HandClass, Wide);
		if (N.Callers > 0)
		{
			// A squeeze spot: value 3-bets only and fewer flats.
			const charts::Mixed Value = Bb ? charts::charts_detail::PreflopWide(charts::Spot::BbVsOpen, 8, N.Vs, N.E, HandClass, Wide * 0.7) : charts::charts_detail::PreflopWide(charts::Spot::IpVsOpen, N.Me, N.Vs, N.E, HandClass, Wide * 0.7);
			Plan.F.Raise = Min(Plan.F.Raise, Value.Raise);
			Plan.F.Call *= 0.5;
		}
		// 3x in position, 3.8x out of position (the blinds), one more open for each caller.
		Plan.RaiseToBb = N.LastRaiseBb * ((N.Me >= 7 ? 3.8 : 3.0) + N.Callers);
		return Plan;
	}
	case PreflopNode::Kind::Vs3Bet:
		if (N.IOpened)
		{
			Plan.F = charts::charts_detail::PreflopWide(charts::Spot::Vs3Bet, N.Me, N.Vs, N.E, HandClass, Wide);
			Plan.RaiseToBb = N.LastRaiseBb * 2.3;
			return Plan;
		}
		// Cold against a 3-bet: the 4-bet's value only.
		Plan.F = charts::charts_detail::PreflopWide(charts::Spot::Vs4Bet, 0, 0, N.E, HandClass, 1.0);
		Plan.Jam = N.E < 60.0;
		Plan.RaiseToBb = N.LastRaiseBb * 2.3;
		return Plan;
	case PreflopNode::Kind::Vs4Bet:
		Plan.F = charts::charts_detail::PreflopWide(charts::Spot::Vs4Bet, 0, 0, N.E, HandClass, 1.0);
		Plan.Jam = true;
		return Plan;
	case PreflopNode::Kind::VsJam:
	{
		const double J = Min(N.JamBb, N.E);
		if (J <= 20.0 && !N.IOpened && N.Vs <= 7 && N.Me > N.Vs)
		{
			Plan.F.Call = charts::CallJam(N.Vs, N.Me, J, N.Ante, HandClass, N.Callers > 0) ? 1.0 : 0.0;
			return Plan;
		}
		// A deep jam: call with the 4-bet jam range (QQ+, AK; JJ mixes).
		const charts::Mixed J4 = charts::charts_detail::PreflopWide(charts::Spot::Vs4Bet, 0, 0, N.E, HandClass, 1.0);
		Plan.F.Call = J4.Raise;
		return Plan;
	}
	}
	return Plan;
}

// ---------------------------------------------------------------------------------------------------------- postflop

/** A combo of GTO's own range, ranked at a node. */
struct GtoCombo
{
	Card A = 0, B = 0;
	double Heur = 0; // HeuristicEquity heads-up (the flop and turn), or 0 on the river
	int Score = 0;   // Evaluate: the river's ranking, and the tie-break inside a heuristic bucket
	double Q = 0;    // the share of the range that beats it (ties half)
	int Group = 9;   // the bluff order: 0 draws, 1 overcards and backdoors (the flop and turn); 0 every hand (the river)
	double BluffPos = 2.0; // where it sits in the bluff order (share of the range before it, to its middle)
	bool Draw = false;
};

/** One of GTO's decisions after the flop, as it stood then. */
struct GtoNode
{
	Street St = Street::Flop;
	std::vector<Card> Board;
	Chips Facing = 0;
	Chips PotBefore = 0;
	bool Pfr = false;
	int ActionIndex = 0;
};

enum class GtoMove : int
{
	Check,
	Bet,
	Fold,
	Call,
	Raise,
};

bool GtoBlocked(Card A, Card B, const std::vector<Card>& Board)
{
	for (const Card C : Board)
	{
		if (C == A || C == B)
		{
			return true;
		}
	}
	return false;
}

/** Dry or paired flops get a third of the pot, wet or connected ones two thirds (§2.4.2). */
bool GtoWetFlop(const std::vector<Card>& Board)
{
	int Suits[4] = {0, 0, 0, 0};
	int Mask = 0;
	for (const Card C : Board)
	{
		++Suits[SuitOf(C)];
		Mask |= 1 << RankOf(C);
	}
	if (BoardCategory(Board) >= static_cast<int>(Category::Pair))
	{
		return false;
	}
	const int MaxSuit = std::max(std::max(Suits[0], Suits[1]), std::max(Suits[2], Suits[3]));
	// Ranks with the ace counted low as well (bit 0), deuce at bit 1 .. ace at bit 13.
	const int Ranks = (Mask << 1) | ((Mask >> 12) & 1);
	bool Connected = false; // three ranks inside a five-rank window
	bool Close = false;     // two ranks within two of each other
	for (int Low = 0; Low <= 9; ++Low)
	{
		int In = 0;
		for (int R = Low; R < Low + 5; ++R)
		{
			In += (Ranks >> R) & 1;
		}
		Connected = Connected || In >= 3;
	}
	for (int R = 0; R <= 11; ++R)
	{
		Close = Close || (((Ranks >> R) & 1) != 0 && ((Ranks >> (R + 1)) & 3) != 0);
	}
	// Wet: a monotone flop, a connected one, or a flush draw with two cards close together.
	return MaxSuit >= 3 || Connected || (MaxSuit == 2 && Close);
}

/** The bet size (a share of the pot) GTO uses with no bet to face, for a hand at q. */
double GtoBetSize(const GtoNode& N, double Q)
{
	if (N.St == Street::Flop)
	{
		return GtoWetFlop(N.Board) ? 0.66 : 0.33;
	}
	if (N.St == Street::Turn)
	{
		return 0.66;
	}
	return Q <= 0.05 ? 1.0 : 0.75;
}

double GtoValueRegion(const GtoNode& N)
{
	return N.St == Street::Flop ? (N.Pfr ? 0.30 : 0.15) : N.St == Street::Turn ? 0.25 : 0.20;
}

/** Overcards (both above the board) or a backdoor draw (three to a flush, or three ranks of a straight with a hole card). */
bool GtoOverOrBackdoor(Card A, Card B, const std::vector<Card>& Board)
{
	int Top = 0;
	int Suits[4] = {0, 0, 0, 0};
	int Mask = 0;
	for (const Card C : Board)
	{
		Top = std::max(Top, RankOf(C));
		++Suits[SuitOf(C)];
		Mask |= 1 << RankOf(C);
	}
	if (RankOf(A) > Top && RankOf(B) > Top)
	{
		return true;
	}
	if (Board.size() != 3)
	{
		return false;
	}
	// A backdoor flush: exactly three of a hole card's suit (four is a flush draw, counted as a draw).
	for (const Card H : {A, B})
	{
		const int S = SuitOf(H);
		if (Suits[S] + (SuitOf(A) == S ? 1 : 0) + (SuitOf(B) == S ? 1 : 0) == 3)
		{
			return true;
		}
	}
	const int Hole = (1 << RankOf(A)) | (1 << RankOf(B));
	const int All = Mask | Hole;
	for (int Low = 0; Low <= 8; ++Low)
	{
		const int Window = 0x1F << Low;
		int In = 0;
		for (int R = Low; R < Low + 5; ++R)
		{
			In += (All >> R) & 1;
		}
		if (In >= 3 && (Hole & Window) != 0)
		{
			return true;
		}
	}
	return false;
}

/** Rank a set of combos at a board: q for each, and the bluff order (§2.4.2). */
void GtoRank(std::vector<GtoCombo>& Range, const std::vector<Card>& Board, Street St)
{
	const bool River = St == Street::River;
	for (GtoCombo& C : Range)
	{
		const std::vector<Card> Hole = {C.A, C.B};
		C.Score = EvaluateHand(Hole, Board);
		C.Heur = River ? 0.0 : HeuristicEquity(Hole, Board, 1);
		C.Draw = !River && HasDraw(C.A, C.B, Board);
	}
	const double N = static_cast<double>(Range.size());
	std::vector<size_t> Order(Range.size());
	for (size_t I = 0; I < Order.size(); ++I)
	{
		Order[I] = I;
	}
	auto Stronger = [&](const GtoCombo& X, const GtoCombo& Y) { return X.Heur != Y.Heur ? X.Heur > Y.Heur : X.Score > Y.Score; };
	std::stable_sort(Order.begin(), Order.end(), [&](size_t X, size_t Y) { return Stronger(Range[X], Range[Y]); });
	// q: those strictly stronger, plus half the ties.
	size_t I = 0;
	while (I < Order.size())
	{
		size_t J = I;
		while (J < Order.size() && !Stronger(Range[Order[I]], Range[Order[J]]) && !Stronger(Range[Order[J]], Range[Order[I]]))
		{
			++J;
		}
		const double Q = (static_cast<double>(I) + 0.5 * static_cast<double>(J - I)) / N;
		for (size_t K = I; K < J; ++K)
		{
			Range[Order[K]].Q = Q;
		}
		I = J;
	}
	for (GtoCombo& C : Range)
	{
		C.Group = River ? 0 : C.Draw ? 0 : GtoOverOrBackdoor(C.A, C.B, Board) ? 1 : 9;
	}
}

/** The bluff order of the range's non-value hands: draws, then overcards and backdoors, each from the bottom up. */
void GtoBluffOrder(std::vector<GtoCombo>& Range, double ValueRegion)
{
	const double N = static_cast<double>(Range.size());
	std::vector<size_t> Order;
	for (size_t I = 0; I < Range.size(); ++I)
	{
		Range[I].BluffPos = 2.0;
		if (Range[I].Q > ValueRegion && Range[I].Group <= 1)
		{
			Order.push_back(I);
		}
	}
	std::stable_sort(Order.begin(), Order.end(), [&](size_t X, size_t Y) {
		return Range[X].Group != Range[Y].Group ? Range[X].Group < Range[Y].Group : Range[X].Q > Range[Y].Q;
	});
	for (size_t K = 0; K < Order.size(); ++K)
	{
		Range[Order[K]].BluffPos = (static_cast<double>(K) + 0.5) / N;
	}
}

/** Where a hand outside the range would sit in it: its q and its bluff position. */
GtoCombo GtoPlace(const std::vector<GtoCombo>& Range, Card A, Card B, const std::vector<Card>& Board, Street St, double ValueRegion)
{
	GtoCombo Me;
	Me.A = A;
	Me.B = B;
	const bool River = St == Street::River;
	const std::vector<Card> Hole = {A, B};
	Me.Score = EvaluateHand(Hole, Board);
	Me.Heur = River ? 0.0 : HeuristicEquity(Hole, Board, 1);
	Me.Draw = !River && HasDraw(A, B, Board);
	Me.Group = River ? 0 : Me.Draw ? 0 : GtoOverOrBackdoor(A, B, Board) ? 1 : 9;
	double Better = 0, Tie = 0, BluffBefore = 0;
	for (const GtoCombo& C : Range)
	{
		const bool Above = C.Heur != Me.Heur ? C.Heur > Me.Heur : C.Score > Me.Score;
		const bool Same = C.Heur == Me.Heur && C.Score == Me.Score;
		Better += Above ? 1.0 : 0.0;
		Tie += Same ? 1.0 : 0.0;
	}
	const double N = static_cast<double>(Range.size());
	Me.Q = (Better + 0.5 * Tie) / N;
	if (Me.Q > ValueRegion && Me.Group <= 1)
	{
		for (const GtoCombo& C : Range)
		{
			if (C.Q > ValueRegion && C.Group <= 1 && (C.Group != Me.Group ? C.Group < Me.Group : C.Q > Me.Q))
			{
				BluffBefore += 1.0;
			}
		}
		Me.BluffPos = (BluffBefore + 0.5) / N;
	}
	return Me;
}

double GtoEdge(double Boundary, double X)
{
	// Every boundary mixes over +-0.03 (§2.4.2): in fully 0.03 inside it, out fully 0.03 outside, 50/50 on it.
	const double F = (Boundary + 0.03 - X) / 0.06;
	return F <= 0.0 ? 0.0 : F >= 1.0 ? 1.0 : F;
}

/** A bluff region's edge: the same mix, narrowed for a small region (half its size), so the share it bets stays its size. */
double GtoBluffEdge(double Region, double Lo, double X)
{
	const double W = Min(0.03, 0.5 * Max(0.0, Region));
	if (W <= 0.0)
	{
		return X <= Lo + Region ? 1.0 : 0.0;
	}
	const double F = (Lo + Region + W - X) / (2.0 * W);
	return F <= 0.0 ? 0.0 : F >= 1.0 ? 1.0 : F;
}

int GtoComboId(Card A, Card B)
{
	return A < B ? A * 52 + B : B * 52 + A;
}

/**
 * What the range does at a node as a whole: the share it value-bets (each combo by its mixed frequency), split by size on
 * the river, and facing a bet the share of draws in the bluff-raise slice. The bluff regions are sized from these actual
 * shares, so ties and a lopsided range keep the bluff-to-value ratio s / (1 + s) exact (§2.4.2).
 */
struct GtoShares
{
	double Value = 0, Value100 = 0, Value75 = 0;
	double Mdf = 1.0, DrawSlice = 0;
};

GtoShares GtoSharesOf(const GtoNode& N, const std::vector<GtoCombo>& Range)
{
	GtoShares Sh;
	const double Count = static_cast<double>(std::max<size_t>(1, Range.size()));
	if (N.Facing <= 0)
	{
		const double V = GtoValueRegion(N);
		for (const GtoCombo& X : Range)
		{
			const double E = GtoEdge(V, X.Q);
			Sh.Value += E;
			Sh.Value100 += N.St == Street::River && X.Q <= 0.05 ? E : 0.0;
		}
		Sh.Value /= Count;
		Sh.Value100 /= Count;
		Sh.Value75 = Sh.Value - Sh.Value100;
		return Sh;
	}
	const double Before = static_cast<double>(std::max<Chips>(1, N.PotBefore - N.Facing));
	Sh.Mdf = 1.0 / (1.0 + static_cast<double>(N.Facing) / Before);
	for (const GtoCombo& X : Range)
	{
		Sh.DrawSlice += X.Draw && X.Q >= Sh.Mdf - 0.05 && X.Q <= Sh.Mdf ? 1.0 : 0.0;
	}
	Sh.DrawSlice /= Count;
	return Sh;
}

/** GTO's rule for one hand at one node (§2.4.2), its mixes drawn from Mix keyed by the combo. Bluffed: a bet or raise made
 *  from the bluff region rather than the value one. */
GtoMove GtoRule(const GtoNode& N, const GtoShares& Sh, const GtoCombo& Me, const HeroContext& C, double* SizeOut, bool* Bluffed)
{
	const int Key = N.ActionIndex * 2704 + GtoComboId(Me.A, Me.B);
	*Bluffed = false;
	if (N.Facing <= 0)
	{
		const double V = GtoValueRegion(N);
		const double S = GtoBetSize(N, Me.Q);
		if (Mix(C.MixSeed, C.Tick, C.HeroIdx, Key, SaltPost) < GtoEdge(V, Me.Q))
		{
			*SizeOut = S;
			return GtoMove::Bet;
		}
		if (Me.Q <= V || Me.Group > 1)
		{
			return GtoMove::Check;
		}
		*Bluffed = true;
		// The bluff region: B = value x r, r = s / (1 + s) (river), 1.5 r (turn), 2 r (flop), capped by what's left.
		if (N.St == Street::River)
		{
			// Two sizes on the river, each balanced: the pot-size bluffs first, then the three-quarter ones.
			const double B100 = Sh.Value100 * (1.0 / 2.0);
			const double B75 = Sh.Value75 * (0.75 / 1.75);
			const double U = Mix(C.MixSeed, C.Tick, C.HeroIdx, Key, SaltBluff);
			const double Pot = GtoBluffEdge(B100, 0.0, Me.BluffPos);
			if (U < Pot)
			{
				*SizeOut = 1.0;
				return GtoMove::Bet;
			}
			// The whole region [0, B100 + B75) less the pot-size part: its share is B75.
			if (U < Max(Pot, GtoBluffEdge(B100 + B75, 0.0, Me.BluffPos)))
			{
				*SizeOut = 0.75;
				return GtoMove::Bet;
			}
			*Bluffed = false;
			return GtoMove::Check;
		}
		const double R = S / (1.0 + S) * (N.St == Street::Turn ? 1.5 : 2.0);
		const double Bluffs = Min(1.0 - Sh.Value, Sh.Value * Min(1.0, R));
		if (Mix(C.MixSeed, C.Tick, C.HeroIdx, Key, SaltBluff) < GtoBluffEdge(Bluffs, 0.0, Me.BluffPos))
		{
			*SizeOut = S;
			return GtoMove::Bet;
		}
		*Bluffed = false;
		return GtoMove::Check;
	}
	// Facing a bet of b x pot: defend MDF = 1 / (1 + b) of the range.
	const double Mdf = Sh.Mdf;
	const double RaiseV = N.St == Street::River ? 0.04 : 0.07;
	const double U = Mix(C.MixSeed, C.Tick, C.HeroIdx, Key, SaltPost);
	if (U < GtoEdge(RaiseV, Me.Q))
	{
		return GtoMove::Raise;
	}
	if (N.St != Street::River && Me.Draw && Me.Q >= Mdf - 0.05 && Me.Q <= Mdf && Sh.DrawSlice > 0.0)
	{
		// The bluff raises: draws in the slice just inside MDF, at most one for every two value raises.
		if (Mix(C.MixSeed, C.Tick, C.HeroIdx, Key, SaltBluff) < Min(1.0, 0.5 * RaiseV / Sh.DrawSlice))
		{
			*Bluffed = true;
			return GtoMove::Raise;
		}
	}
	if (U < GtoEdge(Mdf, Me.Q))
	{
		return GtoMove::Call;
	}
	return GtoMove::Fold;
}

GtoMove GtoMoveOf(ActionType T)
{
	switch (T)
	{
	case ActionType::Check: return GtoMove::Check;
	case ActionType::Bet: return GtoMove::Bet;
	case ActionType::Call: return GtoMove::Call;
	case ActionType::Raise: return GtoMove::Raise;
	default: return GtoMove::Fold;
	}
}

/** GTO's range at the decision now: the preflop line's weights, 128 combos from Mix, filtered by every action since. */
struct GtoState
{
	bool Valid = false;
	std::vector<GtoCombo> Range; // at the current board, ranked
	GtoNode Now;
	GtoCombo Me;
};

GtoState GtoBuild(const HeroContext& C)
{
	GtoState S;
	const PlayerView& V = C.View;
	if (V.Board.size() < 3 || V.Hole.size() != 2)
	{
		return S;
	}
	const int MySeat = V.Seats[static_cast<size_t>(V.Me)].Seat;
	// The preflop line: each class weighted by its frequency for every action GTO took.
	std::array<double, 169> W{};
	W.fill(1.0);
	int LastPreRaiser = -1;
	for (size_t I = 0; I < V.History.size(); ++I)
	{
		const ActionRecord& A = V.History[I];
		if (A.OnStreet != Street::Preflop)
		{
			break;
		}
		if (A.Type == ActionType::Raise || A.Type == ActionType::Bet)
		{
			LastPreRaiser = A.Seat;
		}
		if (A.Seat != MySeat)
		{
			continue;
		}
		PreflopNode N = StylePreflopNode(V, I);
		N.Pressure = C.Pressure * ProfileOf(Style::Gto).IcmAware; // the same in every node of the hand
		for (int H = 0; H < 169; ++H)
		{
			const PreflopPlan Plan = StyleGtoPlan(N, H, 1.0);
			if (!Plan.Known)
			{
				return S;
			}
			const double F = A.Type == ActionType::Raise || A.Type == ActionType::Bet ? Plan.F.Raise
				: A.Type == ActionType::Call                                         ? Plan.F.Call
				: A.Type == ActionType::Check                                        ? 1.0 - Plan.F.Raise
																					 : 0.0;
			W[static_cast<size_t>(H)] *= F;
		}
	}
	// 128 combos from the flop's unblocked ones, by weight, drawn with Mix: the same sample on every street.
	const std::vector<Card> Flop(V.Board.begin(), V.Board.begin() + 3);
	std::vector<std::array<Card, 2>> Pool;
	std::vector<double> Cum;
	double Total = 0;
	for (Card A = 0; A < 52; ++A)
	{
		for (Card B = A + 1; B < 52; ++B)
		{
			if (GtoBlocked(A, B, Flop))
			{
				continue;
			}
			const double Wt = W[static_cast<size_t>(HandClass(A, B))];
			if (Wt <= 0.0)
			{
				continue;
			}
			Total += Wt;
			Pool.push_back({A, B});
			Cum.push_back(Total);
		}
	}
	if (Pool.size() < 8 || Total <= 0.0)
	{
		return S;
	}
	std::vector<GtoCombo> Range;
	for (int K = 0; K < 128; ++K)
	{
		const double U = Mix(C.MixSeed, C.Tick, C.HeroIdx, K, SaltRange) * Total;
		const size_t Pick = static_cast<size_t>(std::upper_bound(Cum.begin(), Cum.end(), U) - Cum.begin());
		const std::array<Card, 2>& P = Pool[std::min(Pick, Pool.size() - 1)];
		GtoCombo X;
		X.A = P[0];
		X.B = P[1];
		Range.push_back(X);
	}
	// Replay every decision GTO made after the flop: keep a combo only if its own rule takes the action actually taken.
	const bool Pfr = LastPreRaiser == MySeat;
	auto NodeAt = [&](Street St, size_t Index, Chips Facing, Chips PotBefore) {
		GtoNode N;
		N.St = St;
		const size_t Cards = St == Street::Flop ? 3 : St == Street::Turn ? 4 : 5;
		N.Board.assign(V.Board.begin(), V.Board.begin() + static_cast<long long>(std::min(Cards, V.Board.size())));
		N.Facing = Facing;
		N.PotBefore = PotBefore;
		N.Pfr = Pfr;
		N.ActionIndex = static_cast<int>(Index);
		return N;
	};
	auto Filter = [&](std::vector<GtoCombo>& R, const std::vector<Card>& Board) {
		R.erase(std::remove_if(R.begin(), R.end(), [&](const GtoCombo& X) { return GtoBlocked(X.A, X.B, Board); }), R.end());
	};
	for (size_t I = 0; I < V.History.size(); ++I)
	{
		const ActionRecord& A = V.History[I];
		if (A.OnStreet == Street::Preflop || A.OnStreet == Street::Showdown || A.Seat != MySeat)
		{
			continue;
		}
		const GtoNode N = NodeAt(A.OnStreet, I, A.Facing, A.PotBefore);
		Filter(Range, N.Board);
		if (Range.size() < 8)
		{
			return S;
		}
		GtoRank(Range, N.Board, N.St);
		GtoBluffOrder(Range, GtoValueRegion(N));
		std::vector<GtoCombo> Kept;
		const GtoShares Sh = GtoSharesOf(N, Range);
		const GtoMove Took = GtoMoveOf(A.Type);
		for (const GtoCombo& X : Range)
		{
			double Size = 0;
			bool Bluffed = false;
			const GtoMove M = GtoRule(N, Sh, X, C, &Size, &Bluffed);
			// A raise or a bet is the same act whatever the size; a check facing nothing and a call of 0 agree.
			if (M == Took || (Took == GtoMove::Check && M == GtoMove::Call && N.Facing <= 0))
			{
				Kept.push_back(X);
			}
		}
		Range = Kept;
	}
	S.Now = NodeAt(V.CurrentStreet, V.History.size(), V.ToCall, V.Pot);
	Filter(Range, S.Now.Board);
	if (Range.size() < 8)
	{
		return S;
	}
	GtoRank(Range, S.Now.Board, S.Now.St);
	const double Vr = GtoValueRegion(S.Now);
	GtoBluffOrder(Range, Vr);
	S.Me = GtoPlace(Range, V.Hole[0], V.Hole[1], S.Now.Board, S.Now.St, Vr);
	S.Range = Range;
	S.Valid = true;
	return S;
}

/** Heads-up, not for the stack: the spots GTO plays from its range. */
bool GtoRangeSpot(const PlayerView& V)
{
	int Opponents = 0;
	for (size_t I = 0; I < V.Seats.size(); ++I)
	{
		Opponents += static_cast<int>(I) != V.Me && !V.Seats[I].Folded ? 1 : 0;
	}
	if (Opponents != 1)
	{
		return false;
	}
	const PublicSeat& Me = V.Seats[static_cast<size_t>(V.Me)];
	for (size_t I = 0; I < V.Seats.size(); ++I)
	{
		if (static_cast<int>(I) != V.Me && !V.Seats[I].Folded && V.Seats[I].AllIn)
		{
			return false; // all in against me: a decision for the stack
		}
	}
	return !(V.ToCall > 0 && static_cast<double>(V.ToCall) >= 0.35 * static_cast<double>(Me.Stack));
}

BotDecision StyleGtoDecide(const HeroContext& C, const HeroBrain& B, const Profile& P, double Tilt, Rng& R)
{
	(void)B;
	const PlayerView& V = C.View;
	if (V.Hole.size() != 2)
	{
		return StyleDecideProfile(C, P, Tilt, R);
	}
	if (V.CurrentStreet == Street::Preflop)
	{
		PreflopNode N = StylePreflopNode(V, V.History.size());
		N.Pressure = C.Pressure * P.IcmAware;
		const int Cls = HandClass(V.Hole[0], V.Hole[1]);
		const PreflopPlan Plan = StyleGtoPlan(N, Cls, StyleTiltWide(Tilt));
		if (!Plan.Known)
		{
			return StyleDecideProfile(C, P, Tilt, R);
		}
		const double U = Mix(C.MixSeed, C.Tick, C.HeroIdx, static_cast<int>(V.History.size()), SaltPre);
		return StyleChoice(V, StylePlanAction(V, Plan, U), 1.0 - charts::charts_detail::ClassMid(Cls));
	}
	if (!GtoRangeSpot(V))
	{
		return StyleDecideProfile(C, P, Tilt, R);
	}
	const GtoState S = GtoBuild(C);
	if (!S.Valid)
	{
		return StyleDecideProfile(C, P, Tilt, R);
	}
	double Size = 0;
	bool Bluffed = false;
	const GtoMove M = GtoRule(S.Now, GtoSharesOf(S.Now, S.Range), S.Me, C, &Size, &Bluffed);
	const double Pot = static_cast<double>(V.Pot);
	const double Bb = bot_detail::BbOf(V);
	switch (M)
	{
	case GtoMove::Bet: return StyleChoice(V, PlayerAction::RaiseTo(static_cast<double>(V.CurrentBet) + Max(Bb, Pot * Size)), 1.0 - S.Me.Q);
	case GtoMove::Raise: return StyleChoice(V, PlayerAction::RaiseTo(static_cast<double>(V.CurrentBet) * 3.0), 1.0 - S.Me.Q);
	case GtoMove::Call: return StyleChoice(V, V.ToCall > 0 ? PlayerAction::Call() : PlayerAction::Check(), 1.0 - S.Me.Q);
	case GtoMove::Fold: return StyleChoice(V, V.CanCheck ? PlayerAction::Check() : PlayerAction::Fold(), 1.0 - S.Me.Q);
	default: return StyleChoice(V, PlayerAction::Check(), 1.0 - S.Me.Q);
	}
}
} // namespace style_detail

RangeAudit AuditGtoRiver(const HeroContext& C)
{
	using namespace style_detail;
	RangeAudit Out;
	const PlayerView& V = C.View;
	if (V.CurrentStreet != Street::River || V.Hole.size() != 2 || !GtoRangeSpot(V))
	{
		return Out;
	}
	const GtoState S = GtoBuild(C);
	if (!S.Valid)
	{
		return Out;
	}
	const double N = static_cast<double>(S.Range.size());
	const GtoShares Sh = GtoSharesOf(S.Now, S.Range);
	double Bet75 = 0, Bluff75 = 0, Bet100 = 0, Bluff100 = 0, Continue = 0;
	for (const GtoCombo& X : S.Range)
	{
		double Size = 0;
		bool Bluff = false;
		const GtoMove M = GtoRule(S.Now, Sh, X, C, &Size, &Bluff);
		if (S.Now.Facing <= 0 && M == GtoMove::Bet)
		{

			if (Size >= 0.99)
			{
				Bet100 += 1.0;
				Bluff100 += Bluff ? 1.0 : 0.0;
			}
			else
			{
				Bet75 += 1.0;
				Bluff75 += Bluff ? 1.0 : 0.0;
			}
		}
		Continue += S.Now.Facing > 0 && (M == GtoMove::Call || M == GtoMove::Raise) ? 1.0 : 0.0;
	}
	Out.Valid = true;
	Out.Bet75 = Bet75 / N;
	Out.Bluff75 = Bet75 > 0 ? Bluff75 / Bet75 : 0.0;
	Out.Bet100 = Bet100 / N;
	Out.Bluff100 = Bet100 > 0 ? Bluff100 / Bet100 : 0.0;
	if (S.Now.Facing > 0)
	{
		Out.Faced = static_cast<double>(S.Now.Facing) / static_cast<double>(std::max<Chips>(1, S.Now.PotBefore - S.Now.Facing));
		Out.Continue = Continue / N;
	}
	return Out;
}
} // namespace style
} // namespace ss
