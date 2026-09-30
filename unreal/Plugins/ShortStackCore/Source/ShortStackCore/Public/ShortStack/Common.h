// ShortStackCore: the SHORT STACK poker engine.
//
// Engine-agnostic C++17. The same sources compile as an Unreal module and as a
// standalone library (see Standalone/CMakeLists.txt), and reproduce the
// TypeScript prototype bit for bit (see Tests/golden_vectors.txt).
//
// Rules for this code so it stays portable and deterministic:
//   - No exceptions and no RTTI (Unreal builds with both off).
//   - Doubles only, no float; never let the compiler fuse multiply-adds
//     (-ffp-contract=off, MSVC /fp:precise).
//   - No libm transcendental functions in simulation code: use PowInt/DetPow.
//   - At most one RNG call per expression, since C++ does not define the
//     evaluation order of operands.
//   - File-local helpers live in uniquely named namespaces (Unreal unity builds
//     merge .cpp files).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#ifndef SS_ASSERT
#include <cassert>
#define SS_ASSERT(Expr) assert(Expr)
#endif

namespace ss
{
/** Card index 0..51: rank = card >> 2 (0 = deuce .. 12 = ace), suit = card & 3 (clubs, diamonds, hearts, spades). */
using Card = int;

/** Chip amounts. Always whole numbers; exact in a double up to 2^53 as in the TypeScript build. */
using Chips = int64_t;

/** x^n for a non-negative integer n, by repeated multiplication. */
double PowInt(double X, int N);

/** x^y for x, y >= 0 using only multiplication and square roots (bit-identical across platforms). */
double DetPow(double X, double Y);

/** JavaScript Math.round: halves round toward +infinity. */
double JsRound(double X);

/** JavaScript number formatting for whole numbers ("1200", not "1200.0"); other values use 17 significant digits. */
std::string JsNumber(double X);

/** JavaScript Number.prototype.toFixed(1): exact decimal value, ties round up. */
std::string JsToFixed1(double X);

/** 32-bit FNV-1a over bytes, used for golden-test digests. */
uint32_t Fnv1a(const std::string& Text);

inline double Min(double A, double B) { return A < B ? A : B; }
inline double Max(double A, double B) { return A > B ? A : B; }
inline Chips MinChips(Chips A, Chips B) { return A < B ? A : B; }
inline Chips MaxChips(Chips A, Chips B) { return A > B ? A : B; }
} // namespace ss
