#include "ShortStack/Game/Live.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/ChipSet.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

// The clock facts and the payout ladder (next_segment_spec §6.2 C; declared in Live.h). core-tableui.
//
// One set of facts for the clock card, the room's screens, the pause menu, the dealer's answers and the PA. Every
// number is the engine's: the level and the field from the tournament, the money from its payout table, the late
// registration from the occurrence. The seconds are the host's (it knows how far into a hand the table is).

namespace ss
{
namespace live
{
namespace liveclock_detail
{
std::string Upper(std::string S)
{
	for (char& Ch : S)
	{
		if (Ch >= 'a' && Ch <= 'z')
		{
			Ch = static_cast<char>(Ch - 'a' + 'A');
		}
	}
	return S;
}

/** "35", "12.5": a permille as a percentage. */
std::string Percent(int Permille)
{
	return Permille % 10 == 0 ? std::to_string(Permille / 10) : std::to_string(Permille / 10) + "." + std::to_string(Permille % 10);
}

/** The colour-up's chips by name ("THE BLACK 100s"). */
std::string ChipWords(Chips Value)
{
	switch (Value)
	{
	case 100: return "THE BLACK 100s";
	case 500: return "THE PURPLE 500s";
	case 1000: return "THE YELLOW 1,000s";
	case 5000: return "THE ORANGE 5,000s";
	default: return "THE " + Grouped(Value) + "s";
	}
}

/** The chips raced off at the break before level index B (empty: none). */
std::vector<Chips> RacedAt(const Tournament& T, int B)
{
	std::vector<Chips> Out;
	if (B <= 0)
	{
		return Out;
	}
	const std::vector<chipset::Denom> Before = chipset::InPlay(chipset::Room::Embercrest, T.Levels, B - 1, T.Spec.BreakEvery);
	const std::vector<chipset::Denom> After = chipset::InPlay(chipset::Room::Embercrest, T.Levels, B, T.Spec.BreakEvery);
	for (const chipset::Denom& D : Before)
	{
		if (std::none_of(After.begin(), After.end(), [&D](const chipset::Denom& A) { return A.Value == D.Value; }))
		{
			Out.push_back(D.Value);
		}
	}
	return Out;
}

std::string Joined(const std::vector<Chips>& Values)
{
	std::string Out;
	for (size_t I = 0; I < Values.size(); ++I)
	{
		Out += (I == 0 ? "" : I + 1 == Values.size() ? " AND " : ", ") + ChipWords(Values[I]);
	}
	return Out;
}
} // namespace liveclock_detail

ClockFacts Clock(const Tournament& T, const Occurrence& O, const life::LiveEntry* E, double World, double LevelSecondsLeft, bool OnBreak, double BreakSecondsLeft)
{
	using namespace liveclock_detail;
	ClockFacts F;
	F.Level = T.LevelIndex + 1;
	F.Now = T.CurrentLevel();
	F.Next = T.NextLevel();
	F.LevelSecondsLeft = std::max(0.0, LevelSecondsLeft);
	F.OnBreak = OnBreak;
	F.BreakSecondsLeft = OnBreak ? std::max(0.0, BreakSecondsLeft) : 0.0;
	const int Every = T.Spec.BreakEvery;
	const double LevelLen = T.Spec.LevelMinutes * 60.0;
	int NextBreak = -1; // the level index the next break comes before
	if (Every > 0)
	{
		NextBreak = (T.LevelIndex / Every + 1) * Every;
		if (NextBreak < static_cast<int>(T.Levels.size()))
		{
			F.BreakBeforeLevel = NextBreak + 1;
			F.SecondsToBreak = F.BreakSecondsLeft + F.LevelSecondsLeft + static_cast<double>(NextBreak - T.LevelIndex - 1) * LevelLen;
		}
		else
		{
			NextBreak = -1;
		}
	}
	F.Entrants = T.Spec.Entrants;
	F.Remaining = T.Remaining;
	F.Paid = T.PaidPlaces();
	F.ToTheMoney = std::max(0, T.Remaining - F.Paid);
	F.PoolCents = T.PrizePoolCents;
	F.MinCashCents = T.Payouts.empty() ? 0 : T.Payouts.back();
	F.Average = static_cast<Chips>(JsRound(T.AverageStack()));
	F.AverageBb = F.Now.Bb > 0 ? static_cast<double>(F.Average) / static_cast<double>(F.Now.Bb) : 0.0;
	F.HandForHand = T.HandForHand();
	F.InTheMoney = T.InTheMoney();
	F.Projected = O.Valid() && World < O.LateRegEnds;
	if (F.InTheMoney)
	{
		// The next pay jump: the band the next player out is paid from, and the place one under its top.
		for (const std::pair<int, int>& Band : PayoutBands(F.Paid))
		{
			if (T.Remaining >= Band.first && T.Remaining <= Band.second && Band.first > 1)
			{
				F.NextJumpAt = Band.first - 1;
				F.NextJumpCents = T.PrizeFor(F.NextJumpAt) - T.PrizeFor(T.Remaining);
			}
		}
	}
	else
	{
		F.NextJumpAt = F.Paid;
		F.NextJumpCents = F.MinCashCents;
	}
	if (O.Valid() && World < O.LateRegEnds)
	{
		const std::string Until = Upper(net::TimeLabel(O.LateRegEnds));
		F.LateReg = World >= LastCallAt(O.LateRegEnds) ? "LAST CALL \xC2\xB7 REG TO " + Until : "LATE REG TO " + Until;
	}
	if (IsEmbercrest(O.Id) || (!O.Valid() && Every > 0))
	{
		if (OnBreak)
		{
			const std::vector<Chips> Now = RacedAt(T, T.LevelIndex);
			F.ColorUp = Now.empty() ? std::string() : "COLORING UP " + Joined(Now);
		}
		else if (NextBreak > 0)
		{
			const std::vector<Chips> Next = RacedAt(T, NextBreak);
			F.ColorUp = Next.empty() ? std::string() : "COLOR-UP AT THE BREAK \xC2\xB7 " + Joined(Next);
		}
	}
	if (E && !life::IsOwnMoney(E->Fund))
	{
		const life::Funding& Fd = E->Fund;
		const std::string Who = Upper(Fd.Who);
		switch (Fd.From)
		{
		case life::Paid::Own:
			F.Funding = Fd.Permille > 0 ? "SOLD " + Percent(Fd.Permille) + "%" + (Who.empty() ? std::string(" OF YOUR ACTION") : " \xC2\xB7 " + Who) : std::string();
			break;
		case life::Paid::Backer: F.Funding = "BACKED \xC2\xB7 " + (Who.empty() ? std::string("A BACKER") : Who) + " " + Percent(Fd.Permille > 0 ? Fd.Permille : 1000) + "%"; break;
		case life::Paid::Package: F.Funding = "PACKAGE \xC2\xB7 " + (Who.empty() ? std::string("THE ROLL") : Who); break;
		case life::Paid::Sponsor: F.Funding = "COVERED \xC2\xB7 " + (Who.empty() ? std::string("YOUR SPONSOR") : Who); break;
		case life::Paid::Ticket: F.Funding = "TICKET"; break;
		}
	}
	const TPlayer& Hero = T.Hero();
	if (!Hero.Busted && Hero.TableId >= 0)
	{
		F.TableId = Hero.TableId;
		F.Seat = SeatLabel(Hero.Seat);
	}
	return F;
}

std::vector<PayRow> PayLadder(const Tournament& T)
{
	std::vector<PayRow> Out;
	const int Paid = T.PaidPlaces();
	const TPlayer& Hero = T.Hero();
	// Here: the place the player finished, or (still in it, in the money) the band the next one out is paid from.
	const int Mark = Hero.Busted ? Hero.Place : T.InTheMoney() ? T.Remaining : 0;
	for (const std::pair<int, int>& Band : PayoutBands(Paid))
	{
		PayRow R;
		R.From = Band.first;
		R.To = Band.second;
		R.Cents = T.PrizeFor(Band.first);
		R.Here = Mark >= Band.first && Mark <= Band.second;
		Out.push_back(R);
	}
	return Out;
}
} // namespace live
} // namespace ss
