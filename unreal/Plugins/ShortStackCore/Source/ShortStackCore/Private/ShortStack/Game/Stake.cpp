#include "ShortStack/Game/Stake.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Life.h"

// Backing (next_segment_spec §2). core-stake.
//
// Phase A, day 1: stubs that compile and change nothing. The money paths hand back today's behavior (FundFor: the
// player's own money; OnRegister, OnSettle, OnRefund, OnlineSettle, Step: no lines, no money), the save hooks read
// and write nothing, and every action the player could take is refused.

namespace ss
{
namespace stake
{
double ShrunkRoi(int, double, bool, double Prior)
{
	return Prior;
}

int Ceiling(const View&, const Book&, const std::string&, bool, Chips)
{
	return 0;
}

std::vector<Offer> OffersFor(const View&, const Book&, double)
{
	return {};
}

int Answer(const Offer&, const Terms&, int)
{
	return 0;
}

double Margin(const Offer&, const Terms&)
{
	return 0.0;
}

std::string Accept(Book&, Chips&, life::State&, const Offer&, double)
{
	return "Nobody's backing anyone yet.";
}

life::Funding FundFor(const Book&, const std::string&, Chips, bool, double)
{
	return life::Funding();
}

void OnRegister(Book&, Chips&, life::State&, const life::LiveEntry&, double)
{
}

void OnSettle(Book&, Chips&, life::State&, const life::LiveEntry&, Chips, double)
{
}

void OnRefund(Book&, Chips&, life::State&, const life::LiveEntry&, double)
{
}

void OnlineSettle(Book&, Chips&, life::State&, const std::string&, Chips, double)
{
}

std::string List(Book&, const View&, const std::string&, int, int, double, double)
{
	return "Nobody's buying pieces yet.";
}

std::string ProposeSwap(Book&, const View&, const std::string&, const std::string&, int)
{
	return "Nobody's swapping yet.";
}

void SettleSwap(Book&, Chips&, life::State&, const std::string&, const std::string&, Chips, double)
{
}

void Step(Book&, Chips&, life::State&, double, double, std::vector<std::pair<std::string, std::string>>&)
{
}

std::string Chop(Book&, Chips&, life::State&, const std::string&, double)
{
	return "There's no deal to chop.";
}

std::string WalkAway(Book&, Chips&, life::State&, const std::string&, double)
{
	return "There's no deal to walk away from.";
}

std::string Pay(Book&, Chips&, life::State&, const std::string&, double)
{
	return "You don't owe anyone.";
}

std::vector<std::pair<std::string, int>> RailFor(const Book&, const std::string&, const std::set<std::string>&)
{
	return {};
}

std::string RailLine(const Book&, const std::string&, const std::string&, int, uint32_t)
{
	return std::string();
}

std::vector<life::Mark> Marks(const Book&, double, double)
{
	return {};
}

void WriteLines(const life::State&, std::string&)
{
}

bool ReadLine(const std::vector<std::string>&, life::State&)
{
	return false;
}

void AfterParse(SaveData&)
{
}
} // namespace stake
} // namespace ss
