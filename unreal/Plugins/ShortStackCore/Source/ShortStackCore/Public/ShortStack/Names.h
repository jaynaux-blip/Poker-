#pragma once

#include "ShortStack/Common.h"

namespace ss
{
class Rng;

/** One procedural online-poker screen name. */
std::string ScreenName(Rng& R);

/** Count distinct names (case-insensitive), avoiding the reserved ones. */
std::vector<std::string> UniqueNames(int Count, Rng& R, const std::vector<std::string>& Reserved);
} // namespace ss
