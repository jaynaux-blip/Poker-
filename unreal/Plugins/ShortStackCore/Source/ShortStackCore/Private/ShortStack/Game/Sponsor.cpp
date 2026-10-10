#include "ShortStack/Game/Sponsor.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/Life.h"

// Sponsors (next_segment_spec §3). core-sponsor.
//
// Phase A, day 1: stubs that compile and change nothing: an empty catalog, no contract active, nothing covered, no
// store credit (SpendCredit leaves the whole bill to the bankroll), nothing the brand's property, no lines read or
// written, and every action refused. Kast keeps its own deals until core-sponsor's view lands (StreamView is empty and
// AbsorbStream keeps nothing).

namespace ss
{
namespace sponsor
{
const std::vector<Brand>& Brands()
{
	static const std::vector<Brand> Empty{};
	return Empty;
}

const Brand* FindBrand(const std::string&)
{
	return nullptr;
}

const Brand* FindBrandByName(const std::string&)
{
	return nullptr;
}

int Tier(const rep::Public&, const std::string&)
{
	return 0;
}

void Step(Book&, life::State&, Chips&, const kast::Channel&, const rep::Public&, double, double, const std::string&)
{
}

std::string CanSign(const Book&, const life::State&, const std::string&, double)
{
	return "There's no deal on the table.";
}

std::string Sign(Book&, life::State&, Chips&, const std::string&, double)
{
	return "There's no deal on the table.";
}

double Leverage(const Book&, const rep::Public&, const std::string&)
{
	return 0.0;
}

std::string Counter(Book&, const rep::Public&, const std::string&, Lever, double)
{
	return "There's no deal on the table.";
}

void Decline(Book&, const std::string&, double)
{
}

std::string GiveNotice(Book&, const std::string&, double)
{
	return "There's no deal to end.";
}

std::string AskForRaise(Book&, const rep::Public&, const std::string&, double)
{
	return "There's no deal to talk about.";
}

std::string Fits(const Book&, const wardrobe::Closet&, bool, double)
{
	return std::string();
}

void NoteWorn(life::LiveEntry&, const std::string&, const wardrobe::Closet&, const Book&, double)
{
}

std::vector<Window> Appearances(const Book&, double, double)
{
	return {};
}

std::string Appear(Book&, const std::string&, double)
{
	return "Nobody's expecting you.";
}

const Contract* Covering(const Book&, const live::Occurrence&, double)
{
	return nullptr;
}

void OnRegister(Book&, life::State&, Chips&, const life::LiveEntry&, double)
{
}

void Settled(Book&, life::State&, Chips&, const life::LiveEntry&)
{
}

void OnRefund(Book&, life::State&, Chips&, const life::LiveEntry&, double)
{
}

std::vector<kast::Deal> StreamView(const Book&, double)
{
	return {};
}

void AbsorbStream(Book&, const std::vector<kast::Deal>&)
{
}

void Conduct(Book&, life::State&, Chips&, Breach, double)
{
}

bool Active(const Book&, const std::string&, double)
{
	return false;
}

Chips SpendCredit(Book&, Chips Due)
{
	return Due;
}

bool IsProperty(const Book&, const std::string&, double)
{
	return false;
}

std::vector<std::pair<std::string, uint32_t>> Badges(const Book&, double)
{
	return {};
}

std::vector<life::Mark> Marks(const Book&, double, double)
{
	return {};
}

Chips NpcWeekly(const std::string&)
{
	return 0;
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
} // namespace sponsor
} // namespace ss
