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
	explicit Rng(const std::string& Seed);

	uint32_t NextUint32();
	/** Uniform double in [0, 1). */
	double Next();
	/** Uniform integer in [0, N), without modulo bias. */
	int Int(int N);
	double Range(double Lo, double Hi);
	bool Chance(double P);
	/** Approximately normal sample (Irwin-Hall with four terms). */
	double Gauss(double Mean, double Sd);
	/** Independent child generator. */
	Rng Fork(const std::string& Label);

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
