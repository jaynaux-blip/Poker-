#include "ShortStack/Game/Recall.h"
#include "../StrictFloat.h"

#include <cstdlib>
#include <string>
#include <vector>

// The hands the player played (next_segment_spec §6.3). core-tableui.
//
// Only what the player saw: their own cards, the board as far as it was dealt, every action, and the cards turned up at
// the showdown (the host names those seats; a mucked hand stays unknown). Written as one line of printable text for the
// checkpoint's host text (recall\t<n>\t<escaped>): "1;number;level;sb;bb;ante;pot;net;mine;board;lines;shown", where a
// line is "street,to,who,what" and a shown hand "who,card,card", entries joined by '/', and '\' escapes '\', ';', '/'
// and ',' inside a name.

namespace ss
{
namespace recall
{
namespace recall_detail
{
const char* Verb(const ActionRecord& A)
{
	if (A.AllIn && A.Type != ActionType::Fold && A.Type != ActionType::Check)
	{
		return "all in";
	}
	switch (A.Type)
	{
	case ActionType::Fold: return "folds";
	case ActionType::Check: return "checks";
	case ActionType::Call: return "calls";
	case ActionType::Bet: return "bets";
	case ActionType::Raise: return "raises to";
	}
	return "";
}

std::string Esc(const std::string& In)
{
	std::string Out;
	for (const char Ch : In)
	{
		if (Ch == '\\' || Ch == ';' || Ch == '/' || Ch == ',')
		{
			Out.push_back('\\');
		}
		Out.push_back(Ch);
	}
	return Out;
}

std::string Unesc(const std::string& In)
{
	std::string Out;
	for (size_t I = 0; I < In.size(); ++I)
	{
		if (In[I] == '\\' && I + 1 < In.size())
		{
			++I;
		}
		Out.push_back(In[I]);
	}
	return Out;
}

/** Splits on Sep, leaving escapes in place (the leaves unescape). */
std::vector<std::string> Split(const std::string& In, char Sep)
{
	std::vector<std::string> Out(1);
	for (size_t I = 0; I < In.size(); ++I)
	{
		if (In[I] == '\\' && I + 1 < In.size())
		{
			Out.back().push_back(In[I]);
			Out.back().push_back(In[++I]);
		}
		else if (In[I] == Sep)
		{
			Out.emplace_back();
		}
		else
		{
			Out.back().push_back(In[I]);
		}
	}
	return Out;
}

std::string Cards(const std::vector<Card>& C)
{
	std::string Out;
	for (size_t I = 0; I < C.size(); ++I)
	{
		Out += (I > 0 ? "," : "") + std::to_string(C[I]);
	}
	return Out;
}

bool Number(const std::string& S, long long& Out)
{
	if (S.empty())
	{
		return false;
	}
	char* End = nullptr;
	Out = std::strtoll(S.c_str(), &End, 10);
	return End != nullptr && *End == '\0';
}

bool ReadCards(const std::vector<std::string>& Parts, size_t From, std::vector<Card>& Out)
{
	Out.clear();
	for (size_t I = From; I < Parts.size(); ++I)
	{
		if (Parts[I].empty() && Parts.size() == From + 1)
		{
			return true; // no cards
		}
		long long V = 0;
		if (!Number(Parts[I], V) || V < 0 || V > 51)
		{
			return false;
		}
		Out.push_back(static_cast<Card>(V));
	}
	return true;
}
} // namespace recall_detail

Record Of(const Hand& H, int HeroSeat, const std::vector<std::string>& NameBySeat, const std::vector<int>& ShownSeats, int Number, int Level)
{
	using namespace recall_detail;
	Record R;
	R.Number = Number;
	R.Level = Level;
	R.Blinds.Sb = H.SmallBlind;
	R.Blinds.Bb = H.BigBlind;
	R.Blinds.Ante = H.Ante;
	R.Board = H.Board;
	auto Name = [&NameBySeat](int Seat) {
		return Seat >= 0 && Seat < static_cast<int>(NameBySeat.size()) && !NameBySeat[static_cast<size_t>(Seat)].empty() ? NameBySeat[static_cast<size_t>(Seat)]
																														 : "Seat " + std::to_string(Seat + 1);
	};
	for (const HandSeat& S : H.Seats)
	{
		if (S.Seat == HeroSeat)
		{
			R.Mine = S.Hole;
			R.HeroNet = S.Stack - S.StartStack;
		}
	}
	for (const ActionRecord& A : H.History)
	{
		Line L;
		L.On = A.OnStreet;
		L.Who = Name(A.Seat);
		L.What = Verb(A);
		L.To = A.Type == ActionType::Fold || A.Type == ActionType::Check ? 0 : A.To;
		R.Lines.push_back(L);
	}
	for (const int Seat : ShownSeats)
	{
		if (Seat == HeroSeat)
		{
			continue;
		}
		for (const HandSeat& S : H.Seats)
		{
			if (S.Seat == Seat && !S.Folded && !S.Hole.empty())
			{
				R.Shown.push_back({Name(Seat), S.Hole});
			}
		}
	}
	Chips Pot = 0;
	for (const PotResult& P : H.PotResults)
	{
		Pot += P.Amount;
	}
	R.Pot = Pot > 0 ? Pot : H.Pot();
	return R;
}

std::string Write(const Record& R)
{
	using namespace recall_detail;
	std::string Lines;
	for (size_t I = 0; I < R.Lines.size(); ++I)
	{
		const Line& L = R.Lines[I];
		Lines += (I > 0 ? "/" : "") + std::to_string(static_cast<int>(L.On)) + "," + std::to_string(L.To) + "," + Esc(L.Who) + "," + Esc(L.What);
	}
	std::string Shown;
	for (size_t I = 0; I < R.Shown.size(); ++I)
	{
		Shown += (I > 0 ? "/" : "") + Esc(R.Shown[I].first) + (R.Shown[I].second.empty() ? std::string() : "," + Cards(R.Shown[I].second));
	}
	return "1;" + std::to_string(R.Number) + ";" + std::to_string(R.Level) + ";" + std::to_string(R.Blinds.Sb) + ";" + std::to_string(R.Blinds.Bb) + ";" +
		std::to_string(R.Blinds.Ante) + ";" + std::to_string(R.Pot) + ";" + std::to_string(R.HeroNet) + ";" + Cards(R.Mine) + ";" + Cards(R.Board) + ";" + Lines + ";" +
		Shown;
}

bool Read(const std::string& Text, Record& Out)
{
	using namespace recall_detail;
	const std::vector<std::string> F = Split(Text, ';');
	if (F.size() < 12 || F[0] != "1")
	{
		return false;
	}
	Record R;
	long long V[7] = {};
	for (int I = 0; I < 7; ++I)
	{
		if (!Number(F[static_cast<size_t>(I + 1)], V[I]))
		{
			return false;
		}
	}
	R.Number = static_cast<int>(V[0]);
	R.Level = static_cast<int>(V[1]);
	R.Blinds.Sb = V[2];
	R.Blinds.Bb = V[3];
	R.Blinds.Ante = V[4];
	R.Pot = V[5];
	R.HeroNet = V[6];
	if (!ReadCards(Split(F[8], ','), 0, R.Mine) || !ReadCards(Split(F[9], ','), 0, R.Board))
	{
		return false;
	}
	if (!F[10].empty())
	{
		for (const std::string& Entry : Split(F[10], '/'))
		{
			const std::vector<std::string> P = Split(Entry, ',');
			long long Street_ = 0, To = 0;
			if (P.size() != 4 || !Number(P[0], Street_) || !Number(P[1], To) || Street_ < 0 || Street_ > static_cast<long long>(Street::Showdown))
			{
				return false;
			}
			Line L;
			L.On = static_cast<Street>(Street_);
			L.To = To;
			L.Who = Unesc(P[2]);
			L.What = Unesc(P[3]);
			R.Lines.push_back(L);
		}
	}
	if (!F[11].empty())
	{
		for (const std::string& Entry : Split(F[11], '/'))
		{
			const std::vector<std::string> P = Split(Entry, ',');
			std::vector<Card> Hole;
			if (P.size() > 1 && !ReadCards(P, 1, Hole))
			{
				return false;
			}
			R.Shown.push_back({Unesc(P[0]), Hole});
		}
	}
	Out = R;
	return true;
}
} // namespace recall
} // namespace ss
