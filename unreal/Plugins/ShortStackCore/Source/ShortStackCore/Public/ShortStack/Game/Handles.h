#pragma once

#include "ShortStack/Common.h"

#include <set>
#include <string>
#include <vector>

namespace ss
{
class Rng;

/**
 * RiverLine screen names: the handles people actually pick. Slang compounds ("VelvetRiver"), real names in
 * the styles of their countries ("kenji.k", "pablo_ortega"), poker words in their own languages
 * ("Kartenhai", "ReiDoRio"), late-night jokes ("OneMoreTable") and the occasional gamer tag. Display only:
 * the engine's own name generator (Names.h) stays as it is for parity with the TypeScript build.
 */
namespace handles
{
struct Identity
{
	std::string Name;
	std::string Country; // ISO 3166 alpha-2
};

/** A country, weighted like the network's player base. */
SHORTSTACKCORE_API std::string PickCountry(Rng& R);
/** One handle for a player from Country; Pro favors the understated names of high-stakes regulars. */
SHORTSTACKCORE_API std::string Make(Rng& R, const std::string& Country, bool Pro = false);
/** Count identities with names unique among themselves and Taken. */
SHORTSTACKCORE_API std::vector<Identity> Field(int Count, Rng& R, const std::set<std::string>& Taken);
} // namespace handles
} // namespace ss
