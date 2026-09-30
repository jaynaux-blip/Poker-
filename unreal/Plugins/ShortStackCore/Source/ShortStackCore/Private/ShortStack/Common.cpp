#include "ShortStack/Common.h"
#include "StrictFloat.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace ss
{
double PowInt(double X, int N)
{
	double R = 1.0;
	for (int I = 0; I < N; ++I)
	{
		R *= X;
	}
	return R;
}

double DetPow(double X, double Y)
{
	if (Y == 0.0)
	{
		return 1.0;
	}
	if (X <= 0.0)
	{
		return 0.0;
	}
	const double Whole = std::floor(Y);
	double R = PowInt(X, static_cast<int>(Whole));
	double Frac = Y - Whole;
	double Root = X;
	for (int I = 0; I < 16 && Frac > 0.0; ++I)
	{
		Root = std::sqrt(Root);
		Frac *= 2.0;
		if (Frac >= 1.0)
		{
			R *= Root;
			Frac -= 1.0;
		}
	}
	return R;
}

double JsRound(double X)
{
	double R = std::floor(X);
	if (X - R >= 0.5)
	{
		R += 1.0;
	}
	return R;
}

std::string JsNumber(double X)
{
	char Buf[64];
	if (X == std::floor(X) && std::fabs(X) < 1e15)
	{
		std::snprintf(Buf, sizeof(Buf), "%lld", static_cast<long long>(X));
	}
	else
	{
		std::snprintf(Buf, sizeof(Buf), "%.17g", X);
	}
	return Buf;
}

std::string JsToFixed1(double X)
{
	const bool Negative = X < 0.0;
	const double A = Negative ? -X : X;
	// Every double has a finite decimal expansion; 60 fraction digits holds it exactly for this range.
	char Buf[400];
	std::snprintf(Buf, sizeof(Buf), "%.60f", A);
	std::string Digits(Buf);
	const size_t Dot = Digits.find('.');
	std::string IntPart = Digits.substr(0, Dot);
	int Tenth = Digits[Dot + 1] - '0';
	const bool RoundUp = Digits[Dot + 2] >= '5';
	if (RoundUp)
	{
		++Tenth;
		if (Tenth == 10)
		{
			Tenth = 0;
			// Carry into the integer part.
			int I = static_cast<int>(IntPart.size()) - 1;
			while (I >= 0)
			{
				if (IntPart[static_cast<size_t>(I)] == '9')
				{
					IntPart[static_cast<size_t>(I)] = '0';
					--I;
				}
				else
				{
					IntPart[static_cast<size_t>(I)] = static_cast<char>(IntPart[static_cast<size_t>(I)] + 1);
					break;
				}
			}
			if (I < 0)
			{
				IntPart = "1" + IntPart;
			}
		}
	}
	std::string Out = IntPart + "." + static_cast<char>('0' + Tenth);
	if (Negative && Out != "0.0")
	{
		Out = "-" + Out;
	}
	return Out;
}

uint32_t Fnv1a(const std::string& Text)
{
	uint32_t H = 2166136261u;
	for (const char Ch : Text)
	{
		H ^= static_cast<unsigned char>(Ch);
		H *= 16777619u;
	}
	return H;
}
} // namespace ss
