#include "ShortStack/AI/Reads.h"

#include "ShortStack/Hand.h"

#include <cstdlib>
#include <string>
#include <vector>

// The observed stats (update 2 §2.4.3). Phase A, day 1 (the lead): stubs that keep today's behaviour (nothing is
// counted, every estimate is its prior at no confidence) and the text form. core-styles owns this file from here.

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
} // namespace reads_detail

void Count(const Hand& H, int HeroSeat, Table& Out)
{
	// Stub: counts nothing yet (core-styles).
	(void)H;
	(void)HeroSeat;
	(void)Out;
}

void Fold(Table& Notes, const Table& Event, double Weight)
{
	// Stub: folds nothing yet (core-styles).
	(void)Notes;
	(void)Event;
	(void)Weight;
}

Read Of(const Stats* Seen, const Stats* Note, int LearnedTells)
{
	// Stub: the population priors (§2.4.3's table) at no confidence: Exploitative plays as TAG.
	(void)Seen;
	(void)Note;
	(void)LearnedTells;
	Read R;
	R.Vpip = 0.30;
	R.Pfr = 0.15;
	R.ThreeBet = 0.05;
	R.FoldTo3Bet = 0.55;
	R.Cbet = 0.60;
	R.FoldToCbet = 0.50;
	R.Wtsd = 0.30;
	R.Af = 1.8;
	R.C = 0.0;
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
