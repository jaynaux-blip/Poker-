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
	/**
	 * With Rewrite: the file as this build plays it, every tournament's tick and tresult lines replaced by this build's
	 * (a block can change length), every other line as it was. For a deliberate change to the bots, made in bot.ts too.
	 */
	std::string Rewritten;
};

/**
 * Replays every record in golden_vectors.txt (produced by the TypeScript
 * build, web/scripts/gen-golden.ts) and checks this build reproduces it bit
 * for bit. Used by the standalone test and by the Unreal automation test.
 */
GoldenResult RunGoldenVectors(const std::string& FileText, bool Rewrite = false);
} // namespace ss
