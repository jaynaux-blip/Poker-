#include "ShortStack/Game/Format.h"
#include "../StrictFloat.h"

#include <cmath>
#include <cstdio>

namespace ss
{
std::string Grouped(int64_t N)
{
	const bool Neg = N < 0;
	uint64_t V = Neg ? static_cast<uint64_t>(-(N + 1)) + 1 : static_cast<uint64_t>(N);
	std::string Digits = std::to_string(V);
	std::string Out;
	const size_t Len = Digits.size();
	for (size_t I = 0; I < Len; ++I)
	{
		Out.push_back(Digits[I]);
		const size_t Left = Len - 1 - I;
		if (Left > 0 && Left % 3 == 0)
		{
			Out.push_back(',');
		}
	}
	return Neg ? "-" + Out : Out;
}

std::string Money(Chips Cents)
{
	const bool Neg = Cents < 0;
	const int64_t Abs = Neg ? -Cents : Cents;
	const int64_t Dollars = Abs / 100;
	const int64_t Rem = Abs % 100;
	std::string Out = Neg ? "-$" : "$";
	Out += Dollars >= 1000 ? Grouped(Dollars) : std::to_string(Dollars);
	Out += '.';
	Out += static_cast<char>('0' + Rem / 10);
	Out += static_cast<char>('0' + Rem % 10);
	return Out;
}

std::string Fixed(double V, int Digits)
{
	char Buf[64];
	std::snprintf(Buf, sizeof(Buf), "%.*f", Digits, V);
	return Buf;
}

std::string ChipsText(double N)
{
	if (N >= 10000000.0)
	{
		return Fixed(N / 1000000.0, 1) + "M";
	}
	if (N >= 1000000.0)
	{
		return Fixed(N / 1000000.0, 2) + "M";
	}
	return Grouped(static_cast<int64_t>(JsRound(N)));
}

std::string Ordinal(int N)
{
	static const char* const Suffix[4] = {"th", "st", "nd", "rd"};
	const int V = N % 100;
	const int A = (V - 20) % 10; // may be negative, like the JavaScript original
	const char* S = nullptr;
	if (A >= 0 && A < 4 && V >= 20)
	{
		S = Suffix[A];
	}
	if (!S && V >= 0 && V < 4)
	{
		S = Suffix[V];
	}
	if (!S)
	{
		S = Suffix[0];
	}
	return Grouped(N) + S;
}

std::string ClockString(double Minutes)
{
	const int64_t Floor = static_cast<int64_t>(std::floor(Minutes));
	const int64_t M = ((Floor % 1440) + 1440) % 1440;
	const int64_t H24 = M / 60;
	const int64_t Mm = M % 60;
	const int64_t H12 = H24 % 12 == 0 ? 12 : H24 % 12;
	char Buf[32];
	std::snprintf(Buf, sizeof(Buf), "%d:%02d %s", static_cast<int>(H12), static_cast<int>(Mm), H24 < 12 ? "AM" : "PM");
	return Buf;
}

const char* StreetTitle(Street S)
{
	switch (S)
	{
	case Street::Preflop: return "Preflop";
	case Street::Flop: return "Flop";
	case Street::Turn: return "Turn";
	case Street::River: return "River";
	default: return "Showdown";
	}
}

double Clamp01(double V)
{
	return V < 0.0 ? 0.0 : V > 1.0 ? 1.0 : V;
}

double EaseOutCubic(double T)
{
	const double X = 1.0 - Clamp01(T);
	return 1.0 - X * X * X;
}

double EaseInOut(double T)
{
	const double X = Clamp01(T);
	if (X < 0.5)
	{
		return 4.0 * X * X * X;
	}
	const double Y = -2.0 * X + 2.0;
	return 1.0 - Y * Y * Y / 2.0;
}

double EaseOutBack(double T)
{
	const double X = Clamp01(T) - 1.0;
	const double C1 = 1.70158;
	const double C3 = C1 + 1.0;
	return 1.0 + C3 * X * X * X + C1 * X * X;
}
} // namespace ss
