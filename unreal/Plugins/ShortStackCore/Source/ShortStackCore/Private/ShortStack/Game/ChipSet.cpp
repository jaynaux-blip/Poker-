#include "ShortStack/Game/ChipSet.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Live.h"

#include <algorithm>
#include <string>
#include <vector>

// The chips on the felt (next_segment_spec §6.2 B). core-tableui.

namespace ss
{
namespace chipset
{
namespace chipset_detail
{
constexpr int ColumnMax = 20;

const std::vector<Denom>& Embercrest()
{
	// The common North American tournament code; the top chip carries the room's ember on its edge.
	static const std::vector<Denom> Chips_ = {
		{100, 0x161617u, 0xebe7dfu, 0x161617u, "SM_Chip_T100"},
		{500, 0x5b2a86u, 0xebe7dfu, 0x5b2a86u, "SM_Chip_T500"},
		{1000, 0xe3b23cu, 0x161617u, 0x161617u, "SM_Chip_T1K"},
		{5000, 0xe0702au, 0xebe7dfu, 0xa8461au, "SM_Chip_T5K"},
		{25000, 0xebe7dfu, 0xd9531eu, 0x9e2f0cu, "SM_Chip_T25K"},
	};
	return Chips_;
}

const std::vector<Denom>& Dees()
{
	// art/blender/assets/chips.py: the Spin Cycle Club's clay, $1, $5, $25 and $100.
	static const std::vector<Denom> Chips_ = {
		{1, 0xe8e4dau, 0x2a5bb8u, 0x2a5bb8u, "SM_Chip_1"},
		{5, 0xb01f2au, 0xf1ece0u, 0xb01f2au, "SM_Chip_5"},
		{25, 0x1c7443u, 0xf1ece0u, 0x1c7443u, "SM_Chip_25"},
		{100, 0x161617u, 0xe9c46au, 0x161617u, "SM_Chip_100"},
	};
	return Chips_;
}

/** Which room's chips these are (by the set's own mesh names), or -1. */
int RoomOf(const std::vector<Denom>& InPlay)
{
	for (const Denom& D : InPlay)
	{
		for (const Denom& E : Embercrest())
		{
			if (E.Mesh == D.Mesh)
			{
				return static_cast<int>(Room::Embercrest);
			}
		}
		for (const Denom& E : Dees())
		{
			if (E.Mesh == D.Mesh)
			{
				return static_cast<int>(Room::BackRoom);
			}
		}
	}
	return -1;
}

/** How many of a chip a player keeps working before the next one up takes over (the Nightly's bag: 10 black, 8 purple,
 *  15 yellow); a pot's columns run deeper. */
int Working(Chips Value, Style S)
{
	const bool Pot = S == Style::Pot;
	switch (Value)
	{
	case 100: return Pot ? 20 : 10;
	case 500: return Pot ? 16 : 8;
	case 1000: return Pot ? 20 : 15;
	case 5000: return Pot ? 10 : 4; // the big chips colour up into the top one
	default: return Pot ? 20 : 10;
	}
}

/** Dee's game, exactly as ABackRoomChips::Break lays it (BackRoomChips.cpp): Counts by $100, $25, $5, $1. */
void DeesBreak(Chips Amount, Style S, Chips Counts[4])
{
	for (int D = 0; D < 4; ++D)
	{
		Counts[D] = 0;
	}
	static const Chips Values[4] = {100, 25, 5, 1};
	Chips R = std::max<Chips>(Amount, 0);
	if (S == Style::Bet)
	{
		// What a player slides out: the fewest chips, but nobody bets a black chip at a $1/$2 game unless the bet is big.
		for (int D = R >= 300 ? 0 : 1; D < 4; ++D)
		{
			Counts[D] = R / Values[D];
			R -= Counts[D] * Values[D];
		}
		return;
	}
	// A stack (or the pot): singles for change, reds in fives, greens in fours, the rest in blacks, each denomination
	// capped at a few columns' worth.
	const Chips Cap = S == Style::Pot ? 30 : 20;
	Chips Ones = R % 5;
	if (R - Ones >= 10)
	{
		Ones += 5;
	}
	R -= Ones;
	Chips Fives = (R % 25) / 5;
	R -= Fives * 5;
	const Chips ExtraFives = std::min<Chips>(R / 25, std::max<Chips>(0, (Cap - Fives) / 5)) * 5;
	Fives += ExtraFives;
	R -= ExtraFives * 5;
	Chips Greens = (R % 100) / 25;
	R -= Greens * 25;
	const Chips ExtraGreens = std::min<Chips>(R / 100, std::max<Chips>(0, (Cap - Greens) / 4)) * 4;
	Greens += ExtraGreens;
	R -= ExtraGreens * 25;
	Counts[0] = R / 100;
	Counts[1] = Greens;
	Counts[2] = Fives;
	Counts[3] = Ones;
}

/** Count chips of one denomination as columns of at most 20, the short one last. */
void Columns(std::vector<Column>& Out, int Index, Chips Value, Chips Count)
{
	while (Count > 0)
	{
		const Chips N = std::min<Chips>(Count, ColumnMax);
		Column C;
		C.Denom = Index;
		C.Count = static_cast<int>(N);
		C.Value = Value;
		Out.push_back(C);
		Count -= N;
	}
}

// ------------------------------------------------------------------ words

const char* const Small[20] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve", "thirteen", "fourteen",
	"fifteen", "sixteen", "seventeen", "eighteen", "nineteen"};
const char* const Tens[10] = {"", "", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"};

std::string Under100(int N)
{
	if (N < 20)
	{
		return Small[N];
	}
	return std::string(Tens[N / 10]) + (N % 10 != 0 ? std::string("-") + Small[N % 10] : std::string());
}

std::string Under1000(int N)
{
	std::string Out;
	if (N >= 100)
	{
		Out = std::string(Small[N / 100]) + " hundred";
		N %= 100;
		if (N > 0)
		{
			Out += " ";
		}
	}
	if (N > 0 || Out.empty())
	{
		Out += Under100(N);
	}
	return Out;
}

std::string Words(Chips N)
{
	if (N <= 0)
	{
		return "nothing";
	}
	// The way a dealer says it: sixteen hundred, twenty-four hundred (but two thousand, eighteen thousand four hundred).
	if (N >= 1100 && N < 10000 && N % 100 == 0 && N % 1000 != 0)
	{
		return Under100(static_cast<int>(N / 100)) + " hundred";
	}
	static const std::pair<Chips, const char*> Scales[] = {{1000000000, "billion"}, {1000000, "million"}, {1000, "thousand"}};
	std::string Out;
	for (const std::pair<Chips, const char*>& Sc : Scales)
	{
		if (N >= Sc.first)
		{
			// (A thousand billion and up says its billions in full rather than run off the words.)
			const Chips Many = N / Sc.first;
			Out += (Out.empty() ? "" : " ") + (Many < 1000 ? Under1000(static_cast<int>(Many)) : Words(Many)) + " " + Sc.second;
			N %= Sc.first;
		}
	}
	if (N > 0)
	{
		Out += (Out.empty() ? "" : " ") + Under1000(static_cast<int>(N));
	}
	return Out;
}
} // namespace chipset_detail

const std::vector<Denom>& Set(Room R)
{
	return R == Room::Embercrest ? chipset_detail::Embercrest() : chipset_detail::Dees();
}

const Denom* Find(Room R, Chips Value)
{
	for (const Denom& D : Set(R))
	{
		if (D.Value == Value)
		{
			return &D;
		}
	}
	return nullptr;
}

std::vector<Denom> InPlay(Room R, const std::vector<Level>& Levels, int LevelIndex, int BreakEvery)
{
	const std::vector<Denom>& All = Set(R);
	if (R != Room::Embercrest || BreakEvery <= 0 || Levels.empty())
	{
		return All;
	}
	std::vector<Denom> Out;
	for (size_t I = 0; I < All.size(); ++I)
	{
		bool Raced = false;
		if (I + 1 < All.size())
		{
			const Chips Up = All[I + 1].Value;
			// The first break after which every blind and ante is a multiple of the next chip up: that break races it off.
			for (int B = BreakEvery; B <= LevelIndex && B < static_cast<int>(Levels.size()) && !Raced; B += BreakEvery)
			{
				bool Even = true;
				for (size_t L = static_cast<size_t>(B); L < Levels.size() && Even; ++L)
				{
					Even = Levels[L].Sb % Up == 0 && Levels[L].Bb % Up == 0 && Levels[L].Ante % Up == 0;
				}
				Raced = Even;
			}
		}
		if (!Raced)
		{
			Out.push_back(All[I]);
		}
	}
	return Out;
}

std::vector<Column> Lay(Chips Amount, const std::vector<Denom>& InPlay, Style S)
{
	using namespace chipset_detail;
	std::vector<Column> Out;
	if (Amount <= 0 || InPlay.empty())
	{
		return Out;
	}
	const int Which = RoomOf(InPlay);
	if (Which == static_cast<int>(Room::BackRoom) && InPlay.size() == Dees().size())
	{
		Chips Counts[4];
		DeesBreak(Amount, S, Counts);
		static const Chips Values[4] = {100, 25, 5, 1};
		for (int D = 0; D < 4; ++D)
		{
			int Index = -1;
			for (size_t K = 0; K < InPlay.size(); ++K)
			{
				Index = InPlay[K].Value == Values[D] ? static_cast<int>(K) : Index;
			}
			Columns(Out, Index, Values[D], Counts[D]);
		}
		return Out;
	}
	// The chips in play, smallest first (by index into InPlay).
	std::vector<int> Order(InPlay.size());
	for (size_t K = 0; K < InPlay.size(); ++K)
	{
		Order[K] = static_cast<int>(K);
	}
	std::stable_sort(Order.begin(), Order.end(), [&InPlay](int A, int B) { return InPlay[static_cast<size_t>(A)].Value < InPlay[static_cast<size_t>(B)].Value; });
	const Chips Smallest = InPlay[static_cast<size_t>(Order.front())].Value;
	// What the chips in play can't make stays on the felt in the old chips (the race is off: no count ever changes).
	const Chips Odd = Amount % Smallest;
	Chips R = Amount - Odd;
	std::vector<Chips> Count(InPlay.size(), 0);
	if (S == Style::Bet)
	{
		for (auto It = Order.rbegin(); It != Order.rend(); ++It)
		{
			const Chips V = InPlay[static_cast<size_t>(*It)].Value;
			Count[static_cast<size_t>(*It)] = R / V;
			R -= Count[static_cast<size_t>(*It)] * V;
		}
	}
	else
	{
		for (size_t K = 0; K + 1 < Order.size(); ++K)
		{
			const size_t I = static_cast<size_t>(Order[K]);
			const Chips Vi = InPlay[I].Value;
			const Chips Vj = InPlay[static_cast<size_t>(Order[K + 1])].Value;
			const Chips Ratio = std::max<Chips>(1, Vj / Vi);
			// What can't go up a chip stays in this one; then working chips up to the room's habit, a next-chip's worth at a time.
			const Chips Forced = (R % Vj) / Vi;
			const Chips Want = Working(Vi, S);
			Chips Groups = Want > Forced ? (Want - Forced) / Ratio : 0;
			Groups = std::min<Chips>(Groups, (R - Forced * Vi) / Vj);
			Count[I] = Forced + Groups * Ratio;
			R -= Count[I] * Vi;
		}
		const size_t Top = static_cast<size_t>(Order.back());
		Count[Top] = R / InPlay[Top].Value;
		R -= Count[Top] * InPlay[Top].Value;
	}
	for (auto It = Order.rbegin(); It != Order.rend(); ++It)
	{
		Columns(Out, *It, InPlay[static_cast<size_t>(*It)].Value, Count[static_cast<size_t>(*It)]);
	}
	if (Odd > 0 && Which >= 0)
	{
		// The old chips, biggest first, at the back.
		Chips Left = Odd;
		const std::vector<Denom>& All = Set(static_cast<Room>(Which));
		for (auto It = All.rbegin(); It != All.rend(); ++It)
		{
			if (It->Value < Smallest && Left >= It->Value)
			{
				Columns(Out, -1, It->Value, Left / It->Value);
				Left %= It->Value;
			}
		}
	}
	return Out;
}

Chips Sum(const std::vector<Column>& Columns)
{
	Chips Total = 0;
	for (const Column& C : Columns)
	{
		Total += static_cast<Chips>(C.Count) * C.Value;
	}
	return Total;
}

std::vector<Column> StartingBag(const live::EventTemplate& T)
{
	using namespace chipset_detail;
	const std::vector<Denom> At1 = InPlay(Room::Embercrest, live::DeepstackLevels(), 0, T.BreakEvery);
	auto Index = [&At1](Chips Value) {
		for (size_t K = 0; K < At1.size(); ++K)
		{
			if (At1[K].Value == Value)
			{
				return static_cast<int>(K);
			}
		}
		return -1;
	};
	// The bags (§6.2 B): they match the templates' stacks (Live.cpp's schedule).
	struct Bag
	{
		Chips Stack;
		int Hundreds, Fives, Thousands, FiveK;
	};
	static const Bag Bags[] = {{10000, 10, 6, 6, 0}, {15000, 10, 8, 10, 0}, {20000, 10, 8, 15, 0}, {30000, 10, 8, 15, 2}};
	for (const Bag& B : Bags)
	{
		if (B.Stack == T.StartingStack)
		{
			std::vector<Column> Out;
			Columns(Out, Index(5000), 5000, B.FiveK);
			Columns(Out, Index(1000), 1000, B.Thousands);
			Columns(Out, Index(500), 500, B.Fives);
			Columns(Out, Index(100), 100, B.Hundreds);
			return Out;
		}
	}
	return Lay(T.StartingStack, At1, Style::Stack);
}

Read ReadStack(Chips Exact, const std::vector<Denom>& InPlay, bool Counted)
{
	Read Out;
	Exact = std::max<Chips>(Exact, 0);
	if (Counted || Exact == 0)
	{
		Out.Low = Exact;
		Out.High = Exact;
		Out.Shown = Exact;
		Out.Exact = true;
		return Out;
	}
	const std::vector<Column> Cols = Lay(Exact, InPlay, Style::Stack);
	// What no chip on the felt shows (below the room's smallest chip) is in every bound.
	const Chips Residue = Exact - Sum(Cols);
	Out.Low = Residue;
	Out.High = Residue;
	Out.Shown = Residue;
	Out.Exact = true;
	for (const Column& C : Cols)
	{
		const Chips V = C.Value;
		if (C.Count >= chipset_detail::ColumnMax || C.Count <= 4)
		{
			// A full column counts by its height against the others; a few chips are counted at a glance.
			Out.Low += C.Count * V;
			Out.High += C.Count * V;
			Out.Shown += C.Count * V;
			continue;
		}
		// A short column, to the nearest five chips.
		const int Est = ((C.Count + 2) / 5) * 5;
		const int Lo = std::max(5, Est - 2);
		const int Hi = std::min(chipset_detail::ColumnMax - 1, Est + 2);
		Out.Low += Lo * V;
		Out.High += Hi * V;
		Out.Shown += std::clamp(Est, Lo, Hi) * V;
		Out.Exact = false;
	}
	return Out;
}

std::string Spoken(Chips Amount)
{
	std::string S = chipset_detail::Words(Amount);
	if (!S.empty() && S[0] >= 'a' && S[0] <= 'z')
	{
		S[0] = static_cast<char>(S[0] - 'a' + 'A');
	}
	return S + ".";
}

std::string BbNumber(Chips Amount, Chips Bb)
{
	if (Bb <= 0)
	{
		return std::string();
	}
	Amount = std::max<Chips>(Amount, 0);
	if (Amount >= 100 * Bb)
	{
		return Grouped((Amount * 2 + Bb) / (2 * Bb));
	}
	// Rounded to the tenth, but never up to the next whole big blind: 9.96 reads 9.9, never 10, and 14.97 reads 14.9 in
	// the ember gold of a stack under 15 (the depth lines at 8 and 15 and the 25-BB open agree with the number shown).
	const Chips Tenths = std::min((Amount * 20 + Bb) / (2 * Bb), (Amount / Bb) * 10 + 9);
	return Tenths % 10 == 0 ? std::to_string(Tenths / 10) : std::to_string(Tenths / 10) + "." + std::to_string(Tenths % 10);
}

std::string InBb(Chips Amount, Chips Bb)
{
	const std::string N = BbNumber(Amount, Bb);
	return N.empty() ? N : N + " BB";
}
} // namespace chipset
} // namespace ss
