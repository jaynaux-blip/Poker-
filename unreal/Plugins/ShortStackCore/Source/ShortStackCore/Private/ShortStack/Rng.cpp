#include "ShortStack/Rng.h"

#include <cmath>

namespace ss
{
namespace rng_detail
{
// cyrb128 string hash: expands a seed into 128 bits of state.
void SeedState(const std::string& Str, uint32_t Out[4])
{
	uint32_t H1 = 1779033703u;
	uint32_t H2 = 3144134277u;
	uint32_t H3 = 1013904242u;
	uint32_t H4 = 2773480762u;
	for (const char Ch : Str)
	{
		const uint32_t K = static_cast<unsigned char>(Ch);
		H1 = H2 ^ ((H1 ^ K) * 597399067u);
		H2 = H3 ^ ((H2 ^ K) * 2869860233u);
		H3 = H4 ^ ((H3 ^ K) * 951274213u);
		H4 = H1 ^ ((H4 ^ K) * 2716044179u);
	}
	H1 = (H3 ^ (H1 >> 18)) * 597399067u;
	H2 = (H4 ^ (H2 >> 22)) * 2869860233u;
	H3 = (H1 ^ (H3 >> 17)) * 951274213u;
	H4 = (H2 ^ (H4 >> 19)) * 2716044179u;
	H1 ^= H2 ^ H3 ^ H4;
	H2 ^= H1;
	H3 ^= H1;
	H4 ^= H1;
	Out[0] = H1;
	Out[1] = H2;
	Out[2] = H3;
	Out[3] = H4;
}
} // namespace rng_detail

Rng::Rng(const std::string& Seed)
{
	uint32_t S[4];
	rng_detail::SeedState(Seed, S);
	A = S[0];
	B = S[1];
	C = S[2];
	D = S[3];
	for (int I = 0; I < 15; ++I)
	{
		NextUint32();
	}
}

uint32_t Rng::NextUint32()
{
	uint32_t T = A + B;
	A = B ^ (B >> 9);
	B = C + (C << 3);
	C = (C << 21) | (C >> 11);
	D = D + 1u;
	T = T + D;
	C = C + T;
	return T;
}

double Rng::Next()
{
	return static_cast<double>(NextUint32()) / 4294967296.0;
}

int Rng::Int(int N)
{
	SS_ASSERT(N > 0);
	const uint64_t Bound = static_cast<uint64_t>(N);
	const uint64_t Limit = (4294967296ull / Bound) * Bound;
	uint64_t X = NextUint32();
	while (X >= Limit)
	{
		X = NextUint32();
	}
	return static_cast<int>(X % Bound);
}

double Rng::Range(double Lo, double Hi)
{
	const double R = Next();
	return Lo + (Hi - Lo) * R;
}

bool Rng::Chance(double P)
{
	return Next() < P;
}

double Rng::Gauss(double Mean, double Sd)
{
	double S = Next();
	S += Next();
	S += Next();
	S += Next();
	S -= 2.0;
	return Mean + Sd * S * std::sqrt(3.0);
}

Rng Rng::Fork(const std::string& Label)
{
	const uint32_t V = NextUint32();
	return Rng(std::to_string(V) + ":" + Label);
}
} // namespace ss
