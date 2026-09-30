#pragma once

#include "ShortStack/Common.h"

namespace ss
{
struct GoldenResult
{
	int Passed = 0;
	int Failed = 0;
	int Skipped = 0;
	/** Human-readable report: per-record-type counts and the first mismatches. */
	std::string Report;
};

/**
 * Replays every record in golden_vectors.txt (produced by the TypeScript
 * build, web/scripts/gen-golden.ts) and checks this build reproduces it bit
 * for bit. Used by the standalone test and by the Unreal automation test.
 */
GoldenResult RunGoldenVectors(const std::string& FileText);
} // namespace ss
