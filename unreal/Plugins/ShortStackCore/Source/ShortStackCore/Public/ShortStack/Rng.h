#pragma once

#include "ShortStack/Common.h"

namespace ss
{
/**
 * Seeded pseudo-random number generator (sfc32 seeded by cyrb128).
 * Produces exactly the same stream as web/src/core/rng.ts for the same ASCII seed,
 * so a whole tournament replays identically in both builds.
 */
class Rng
{
public:
	SHORTSTACKCORE_API explicit Rng(const std::string& Seed);

	SHORTSTACKCORE_API uint32_t NextUint32();
	/** Uniform double in [0, 1). */
	SHORTSTACKCORE_API double Next();
	/** Uniform integer in [0, N), without modulo bias. */
	SHORTSTACKCORE_API int Int(int N);
	SHORTSTACKCORE_API double Range(double Lo, double Hi);
	SHORTSTACKCORE_API bool Chance(double P);
	/** Approximately normal sample (Irwin-Hall with four terms). */
	SHORTSTACKCORE_API double Gauss(double Mean, double Sd);
	/** Independent child generator. */
	SHORTSTACKCORE_API Rng Fork(const std::string& Label);
	/** The generator's whole state (checkpoints): setting it back continues the same stream. */
	void GetState(uint32_t Out[4]) const
	{
		Out[0] = A;
		Out[1] = B;
		Out[2] = C;
		Out[3] = D;
	}
	void SetState(const uint32_t In[4])
	{
		A = In[0];
		B = In[1];
		C = In[2];
		D = In[3];
	}

	template <typename T>
	const T& Pick(const std::vector<T>& Items)
	{
		const int Index = Int(static_cast<int>(Items.size()));
		return Items[static_cast<size_t>(Index)];
	}

	/** In-place Fisher-Yates shuffle, matching the TypeScript order of draws. */
	template <typename T>
	void Shuffle(std::vector<T>& Items)
	{
		for (int I = static_cast<int>(Items.size()) - 1; I > 0; --I)
		{
			const int J = Int(I + 1);
			T Tmp = Items[static_cast<size_t>(I)];
			Items[static_cast<size_t>(I)] = Items[static_cast<size_t>(J)];
			Items[static_cast<size_t>(J)] = Tmp;
		}
	}

private:
	uint32_t A = 0;
	uint32_t B = 0;
	uint32_t C = 0;
	uint32_t D = 0;
};
} // namespace ss
