#include "ShortStack/Cards.h"
#include "StrictFloat.h"

#include <cstring>

namespace ss
{
const char* const RankChars = "23456789TJQKA";
const char* const SuitChars = "cdhs";
const char* const RankNames[13] = {"Deuce", "Three", "Four", "Five", "Six", "Seven", "Eight", "Nine", "Ten", "Jack", "Queen", "King", "Ace"};
const char* const RankPlurals[13] = {"Deuces", "Threes", "Fours", "Fives", "Sixes", "Sevens", "Eights", "Nines", "Tens", "Jacks", "Queens", "Kings", "Aces"};

std::string CardToString(Card C)
{
	std::string S;
	S += RankChars[RankOf(C)];
	S += SuitChars[SuitOf(C)];
	return S;
}

Card ParseCard(const std::string& Text)
{
	if (Text.size() < 2)
	{
		return -1;
	}
	char R = Text[0];
	if (R >= 'a' && R <= 'z')
	{
		R = static_cast<char>(R - 'a' + 'A');
	}
	char S = Text[1];
	if (S >= 'A' && S <= 'Z')
	{
		S = static_cast<char>(S - 'A' + 'a');
	}
	const char* RP = std::strchr(RankChars, R);
	const char* SP = std::strchr(SuitChars, S);
	if (!RP || !SP || R == '\0' || S == '\0')
	{
		return -1;
	}
	return MakeCard(static_cast<int>(RP - RankChars), static_cast<int>(SP - SuitChars));
}

std::vector<Card> ParseCards(const std::string& Text)
{
	std::string Clean;
	for (char Ch : Text)
	{
		if (Ch != ' ' && Ch != '\t')
		{
			Clean += Ch;
		}
	}
	std::vector<Card> Out;
	for (size_t I = 0; I + 1 < Clean.size(); I += 2)
	{
		Out.push_back(ParseCard(Clean.substr(I, 2)));
	}
	return Out;
}

std::vector<Card> FullDeck()
{
	std::vector<Card> D;
	D.reserve(52);
	for (int C = 0; C < 52; ++C)
	{
		D.push_back(C);
	}
	return D;
}

int HandClass(Card A, Card B)
{
	const int Ra = RankOf(A);
	const int Rb = RankOf(B);
	const int Hi = Ra > Rb ? Ra : Rb;
	const int Lo = Ra > Rb ? Rb : Ra;
	const bool Suited = SuitOf(A) == SuitOf(B);
	const int Row = 12 - Hi;
	const int Col = 12 - Lo;
	if (Hi == Lo)
	{
		return Row * 13 + Col;
	}
	return Suited ? Row * 13 + Col : Col * 13 + Row;
}

std::string HandClassLabel(int Cls)
{
	const int Row = Cls / 13;
	const int Col = Cls % 13;
	std::string S;
	if (Row == Col)
	{
		S += RankChars[12 - Row];
		S += RankChars[12 - Row];
	}
	else if (Col > Row)
	{
		S += RankChars[12 - Row];
		S += RankChars[12 - Col];
		S += 's';
	}
	else
	{
		S += RankChars[12 - Col];
		S += RankChars[12 - Row];
		S += 'o';
	}
	return S;
}

int HandClassCombos(int Cls)
{
	const int Row = Cls / 13;
	const int Col = Cls % 13;
	if (Row == Col)
	{
		return 6;
	}
	return Col > Row ? 4 : 12;
}
} // namespace ss
