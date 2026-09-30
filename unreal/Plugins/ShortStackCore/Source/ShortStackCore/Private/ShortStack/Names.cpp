#include "ShortStack/Names.h"
#include "StrictFloat.h"

#include "ShortStack/Rng.h"

#include <unordered_set>

namespace ss
{
namespace names_detail
{
const std::vector<std::string>& WordsA()
{
	static const std::vector<std::string> V = {
		"River", "Nut", "Donk", "Ace", "Bluff", "Stack", "Fold", "Shove", "Grind", "Tilt", "Cooler", "Flop", "Lucky",
		"Kicker", "Suited", "Pocket", "Chip", "Felt", "Muck", "Turbo", "Hyper", "Deep", "Short", "Snap", "Tank", "Punt",
		"Value", "Rail", "Ship", "Spew", "Hero", "Villain", "Nit", "Whale", "Night", "Neon", "Coffee", "Rent", "Brick",
	};
	return V;
}
const std::vector<std::string>& WordsB()
{
	static const std::vector<std::string> V = {
		"King", "Lord", "Ninja", "Wizard", "Queen", "Master", "Machine", "Monster", "Shark", "Fish", "Crusher", "Rider",
		"Hunter", "Boss", "Owl", "Ghost", "Goblin", "Pilot", "Bandit", "Doctor", "Barber", "Prophet", "Dealer", "Cowboy",
	};
	return V;
}
const std::vector<std::string>& First()
{
	static const std::vector<std::string> V = {
		"mike", "jenny", "kostas", "dave", "luis", "priya", "tomas", "anna", "jay", "marco", "sven", "olga", "kev", "nina",
		"raj", "sam", "leo", "mia", "dmitri", "yuki", "hector", "ben", "chloe", "omar", "lars", "ivy", "gus", "fran",
	};
	return V;
}
const std::vector<std::string>& Whole()
{
	static const std::vector<std::string> V = {
		"VeggieLasagna", "nolimitnancy", "sunrun", "d0nkeyk0ng", "LaFleur77", "grandmas_chips", "BigSlickRick",
		"pocket_rockets", "callmemaybe", "the_nit_whisperer", "IShoveAnyTwo", "MinCashMike", "RunItTwice", "tiltedtower",
		"SetMineSally", "BadBeatBobby", "jamorfold", "overlayhunter", "deucesnever", "ICMhero", "floatdaddy", "suitedconnector",
		"sleepybluffs", "rakeback_ron", "coldcalledit", "QuadsOrBust", "reg_n_roll", "blindstealer", "CheckRaiseCheryl",
	};
	return V;
}

std::string Lower(std::string S)
{
	for (char& C : S)
	{
		if (C >= 'A' && C <= 'Z')
		{
			C = static_cast<char>(C - 'A' + 'a');
		}
	}
	return S;
}

std::string Upper(std::string S)
{
	for (char& C : S)
	{
		if (C >= 'a' && C <= 'z')
		{
			C = static_cast<char>(C - 'a' + 'A');
		}
	}
	return S;
}

std::string Leet(const std::string& S, Rng& R)
{
	std::string Out;
	for (char C : S)
	{
		if (C == 'a' || C == 'e' || C == 'i' || C == 'o')
		{
			if (R.Chance(0.35))
			{
				Out += C == 'a' ? '4' : C == 'e' ? '3' : C == 'i' ? '1' : '0';
				continue;
			}
		}
		Out += C;
	}
	return Out;
}
} // namespace names_detail

std::string ScreenName(Rng& R)
{
	using namespace names_detail;
	const double Roll = R.Next();
	if (Roll < 0.18)
	{
		std::string S = R.Pick(Whole());
		if (R.Chance(0.4))
		{
			S += std::to_string(R.Int(99));
		}
		return S;
	}
	if (Roll < 0.45)
	{
		std::string S = R.Pick(WordsA());
		S += R.Pick(WordsB());
		if (R.Chance(0.5))
		{
			S += std::to_string(R.Int(1000));
		}
		return S;
	}
	if (Roll < 0.7)
	{
		static const std::vector<std::string> Seps = {"", "_", ".", ""};
		const std::string Sep = R.Pick(Seps);
		std::string S = R.Pick(First());
		S += Sep;
		if (R.Chance(0.5))
		{
			S += std::to_string(1965 + R.Int(40));
		}
		else
		{
			S += Lower(R.Pick(WordsA()));
		}
		return S;
	}
	if (Roll < 0.85)
	{
		std::string S = Lower(R.Pick(WordsA()));
		S += Lower(R.Pick(WordsB()));
		return Leet(S, R);
	}
	static const std::vector<std::string> Joins = {"_", "x", ""};
	static const std::vector<std::string> Tails = {"", "!", "xx", "z"};
	std::string S = Upper(R.Pick(First()));
	S += R.Pick(Joins);
	S += R.Pick(WordsB());
	S += R.Pick(Tails);
	return S;
}

std::vector<std::string> UniqueNames(int Count, Rng& R, const std::vector<std::string>& Reserved)
{
	using names_detail::Lower;
	std::unordered_set<std::string> Seen;
	for (const std::string& N : Reserved)
	{
		Seen.insert(Lower(N));
	}
	std::vector<std::string> Out;
	while (static_cast<int>(Out.size()) < Count)
	{
		std::string N = ScreenName(R);
		if (N.size() > 16)
		{
			N = N.substr(0, 16);
		}
		if (Seen.count(Lower(N)))
		{
			N = N.substr(0, 13) + std::to_string(R.Int(1000));
		}
		if (Seen.count(Lower(N)))
		{
			continue;
		}
		Seen.insert(Lower(N));
		Out.push_back(N);
	}
	return Out;
}
} // namespace ss
