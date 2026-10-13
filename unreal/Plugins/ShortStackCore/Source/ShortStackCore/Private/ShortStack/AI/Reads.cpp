#include "ShortStack/AI/Reads.h"
#include "../StrictFloat.h"

#include "ShortStack/Hand.h"

#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

// The observed stats (update 2 §2.4.3): what the hero has seen of each player in hands the hero was dealt, counted after
// the hand from its public record (the action history, the board, who reached a showdown), never inside the engine and
// never from anything hidden. Count draws nothing, so the watch path and the skip path count the same. core-styles.

namespace ss
{
namespace reads
{
namespace reads_detail
{
/** The 15 counts in Write's order. */
std::vector<int*> ReadsFields(Stats& S)
{
	return {&S.Hands, &S.Vpip, &S.Pfr, &S.ThreeBetOpp, &S.ThreeBet, &S.FoldTo3BetOpp, &S.FoldTo3Bet, &S.CbetOpp, &S.Cbet, &S.FoldToCbetOpp, &S.FoldToCbet, &S.Flops, &S.Wtsd,
		&S.AggBets, &S.AggCalls};
}

/** One player's flags over one hand, before they become counts (each stat counts at most once a hand). */
struct ReadsHandFlags
{
	bool Vpip = false, Pfr = false, ThreeBetOpp = false, ThreeBet = false, FoldTo3BetOpp = false, FoldTo3Bet = false;
	bool CbetOpp = false, Cbet = false, FoldToCbetOpp = false, FoldToCbet = false, FoldedPreflop = false;
	int AggBets = 0, AggCalls = 0;
	bool ActedOnFlop = false;
};

int ReadsIndexOfSeat(const Hand& H, int SeatNumber)
{
	for (size_t I = 0; I < H.Seats.size(); ++I)
	{
		if (H.Seats[I].Seat == SeatNumber)
		{
			return static_cast<int>(I);
		}
	}
	return -1;
}

/** Shrunk toward the prior: (count + prior × K) / (chances + K). */
double ReadsShrink(double Count, double Chances, double Prior, double K)
{
	return (Count + Prior * K) / (Chances + K);
}

/** Half up, and monotone (a <= b stays a <= b), so a weighted fold keeps every count within its chances. */
int ReadsWeighted(int Count, double Weight)
{
	return static_cast<int>(std::floor(static_cast<double>(Count) * Weight + 0.5));
}
} // namespace reads_detail

void Count(const Hand& H, int HeroSeat, Table& Out)
{
	using reads_detail::ReadsHandFlags;
	const int N = static_cast<int>(H.Seats.size());
	if (N < 2)
	{
		return;
	}
	std::vector<ReadsHandFlags> F(static_cast<size_t>(N));

	// Preflop: the open, the 3-bet chance and the answer to a 3-bet.
	int Raises = 0;
	int Opener = -1;
	int LastRaiser = -1;
	for (const ActionRecord& A : H.History)
	{
		if (A.OnStreet != Street::Preflop)
		{
			break;
		}
		const int P = reads_detail::ReadsIndexOfSeat(H, A.Seat);
		if (P < 0)
		{
			continue;
		}
		ReadsHandFlags& Me = F[static_cast<size_t>(P)];
		const bool Raise = A.Type == ActionType::Raise || A.Type == ActionType::Bet;
		if (Raises == 1 && P != Opener && !Me.ThreeBetOpp)
		{
			Me.ThreeBetOpp = true;
			Me.ThreeBet = Raise;
		}
		if (P == Opener && Raises >= 2 && !Me.FoldTo3BetOpp)
		{
			Me.FoldTo3BetOpp = true;
			Me.FoldTo3Bet = A.Type == ActionType::Fold;
		}
		if (A.Type == ActionType::Call || Raise)
		{
			Me.Vpip = true;
		}
		if (Raise)
		{
			Me.Pfr = true;
			++Raises;
			Opener = Raises == 1 ? P : Opener;
			LastRaiser = P;
		}
		if (A.Type == ActionType::Fold)
		{
			Me.FoldedPreflop = true;
		}
	}

	// Postflop: the c-bet and the answer to it, and aggression (bets and raises against calls).
	const bool SawFlop = H.Board.size() >= 3;
	bool FlopBetBeforePfa = false;
	bool CbetLive = false;
	for (const ActionRecord& A : H.History)
	{
		if (A.OnStreet == Street::Preflop || A.OnStreet == Street::Showdown)
		{
			continue;
		}
		const int P = reads_detail::ReadsIndexOfSeat(H, A.Seat);
		if (P < 0)
		{
			continue;
		}
		ReadsHandFlags& Me = F[static_cast<size_t>(P)];
		const bool Aggressive = A.Type == ActionType::Bet || A.Type == ActionType::Raise;
		if (Aggressive)
		{
			++Me.AggBets;
		}
		else if (A.Type == ActionType::Call)
		{
			++Me.AggCalls;
		}
		if (A.OnStreet != Street::Flop)
		{
			continue;
		}
		if (P == LastRaiser && !Me.ActedOnFlop && !FlopBetBeforePfa)
		{
			Me.CbetOpp = true;
			Me.Cbet = A.Type == ActionType::Bet;
			CbetLive = Me.Cbet;
		}
		else if (P != LastRaiser && CbetLive && !Me.FoldToCbetOpp)
		{
			Me.FoldToCbetOpp = true;
			Me.FoldToCbet = A.Type == ActionType::Fold;
		}
		if (Aggressive && P != LastRaiser && LastRaiser >= 0 && !F[static_cast<size_t>(LastRaiser)].ActedOnFlop)
		{
			FlopBetBeforePfa = true; // a donk bet: the preflop raiser's first flop decision faces a bet, no c-bet chance
		}
		if (A.Type == ActionType::Raise)
		{
			CbetLive = false; // what comes next faces a raise, not the c-bet
		}
		Me.ActedOnFlop = true;
	}

	const bool Showdown = H.CurrentStreet == Street::Showdown;
	for (int I = 0; I < N; ++I)
	{
		if (I == HeroSeat)
		{
			continue;
		}
		const HandSeat& Seat = H.Seats[static_cast<size_t>(I)];
		const ReadsHandFlags& Fl = F[static_cast<size_t>(I)];
		Stats& S = Out[Seat.Id];
		++S.Hands;
		S.Vpip += Fl.Vpip ? 1 : 0;
		S.Pfr += Fl.Pfr ? 1 : 0;
		S.ThreeBetOpp += Fl.ThreeBetOpp ? 1 : 0;
		S.ThreeBet += Fl.ThreeBet ? 1 : 0;
		S.FoldTo3BetOpp += Fl.FoldTo3BetOpp ? 1 : 0;
		S.FoldTo3Bet += Fl.FoldTo3Bet ? 1 : 0;
		S.CbetOpp += Fl.CbetOpp ? 1 : 0;
		S.Cbet += Fl.Cbet ? 1 : 0;
		S.FoldToCbetOpp += Fl.FoldToCbetOpp ? 1 : 0;
		S.FoldToCbet += Fl.FoldToCbet ? 1 : 0;
		const bool Flop = SawFlop && !Fl.FoldedPreflop;
		S.Flops += Flop ? 1 : 0;
		S.Wtsd += Flop && Showdown && !Seat.Folded ? 1 : 0;
		S.AggBets += Fl.AggBets;
		S.AggCalls += Fl.AggCalls;
	}
}

void Fold(Table& Notes, const Table& Event, double Weight)
{
	for (const auto& Kv : Event)
	{
		Stats E = Kv.second;
		Stats& Note = Notes[Kv.first];
		const std::vector<int*> To = reads_detail::ReadsFields(Note);
		const std::vector<int*> From = reads_detail::ReadsFields(E);
		for (size_t I = 0; I < To.size(); ++I)
		{
			*To[I] += reads_detail::ReadsWeighted(*From[I], Weight);
		}
	}
}

Read Of(const Stats* Seen, const Stats* Note, int LearnedTells)
{
	// What was seen tonight and the notes on the player together; a learned tell halves every K ("you've studied them").
	Stats S;
	if (Seen)
	{
		S = *Seen;
	}
	if (Note)
	{
		Stats Copy = *Note;
		const std::vector<int*> To = reads_detail::ReadsFields(S);
		const std::vector<int*> From = reads_detail::ReadsFields(Copy);
		for (size_t I = 0; I < To.size(); ++I)
		{
			*To[I] += *From[I];
		}
	}
	const double Kx = LearnedTells > 0 ? 0.5 : 1.0;
	using reads_detail::ReadsShrink;
	Read R;
	R.Vpip = ReadsShrink(S.Vpip, S.Hands, 0.30, 30.0 * Kx);
	R.Pfr = ReadsShrink(S.Pfr, S.Hands, 0.15, 30.0 * Kx);
	R.ThreeBet = ReadsShrink(S.ThreeBet, S.ThreeBetOpp, 0.05, 20.0 * Kx);
	R.FoldTo3Bet = ReadsShrink(S.FoldTo3Bet, S.FoldTo3BetOpp, 0.55, 20.0 * Kx);
	R.Cbet = ReadsShrink(S.Cbet, S.CbetOpp, 0.60, 15.0 * Kx);
	R.FoldToCbet = ReadsShrink(S.FoldToCbet, S.FoldToCbetOpp, 0.50, 15.0 * Kx);
	R.Wtsd = ReadsShrink(S.Wtsd, S.Flops, 0.30, 20.0 * Kx);
	// The aggression factor (bets and raises over calls) against half a pseudo-call a flop: K = 20 flops.
	R.Af = ReadsShrink(S.AggBets, S.AggCalls, 1.8, 10.0 * Kx);
	R.C = static_cast<double>(S.Hands) / (static_cast<double>(S.Hands) + 30.0 * Kx);
	return R;
}

std::string Write(const Stats& S)
{
	Stats Copy = S;
	std::string Out;
	for (const int* F : reads_detail::ReadsFields(Copy))
	{
		Out += (Out.empty() ? "" : "\t") + std::to_string(*F);
	}
	return Out;
}

bool Parse(const std::string& Text, Stats& Out)
{
	Stats S;
	const std::vector<int*> Fields = reads_detail::ReadsFields(S);
	size_t Start = 0;
	size_t K = 0;
	while (K < Fields.size() && Start <= Text.size())
	{
		size_t End = Text.find('\t', Start);
		End = End == std::string::npos ? Text.size() : End;
		const std::string Part = Text.substr(Start, End - Start);
		if (Part.empty())
		{
			return false;
		}
		*Fields[K++] = std::atoi(Part.c_str());
		Start = End + 1;
	}
	if (K < Fields.size())
	{
		return false;
	}
	Out = S;
	return true;
}
} // namespace reads
} // namespace ss
