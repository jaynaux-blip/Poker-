#pragma once

#include "ShortStack/Common.h"

#include <functional>
#include <set>
#include <string>
#include <vector>

namespace ss
{
class Rng;
namespace world
{
enum class Region : int; // H/World.h
}

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

// ---- Update 4 (SPEC v1.7 §3.2.2, §4.1.8, §4.8.4, §5.2 R5; §6 PA-29).
/** A country from one region, over PickCountry's weights restricted to it (Sim::Arrive's 70%, G2). Unused before G2. */
SHORTSTACKCORE_API std::string PickCountryIn(Rng& R, world::Region In);
/** Wider handles for the crowd's arrivals only (an appended part set from R.Fork("wide"), G4's body). Phase A: Make(R, Country). */
SHORTSTACKCORE_API std::string MakeWide(Rng& R, const std::string& Country);
/** Field with the taken names as a predicate (World::NameUsed's index, PA-33): Used gets a LOWERCASED name and must match
 *  case-insensitively, as Field's own set does, so the retries (and every name after them) are exactly Field's. */
SHORTSTACKCORE_API std::vector<Identity> Field(int Count, Rng& R, const std::function<bool(const std::string&)>& Used);
/** A modelled seat's identity (§4.1.8): country and handle from Rng(FieldSeed + ":seat:" + F), six retries then digits (Field's rule)
 *  against Taken (a lowercased name). Unused before G3 (B-session's). */
SHORTSTACKCORE_API Identity FieldAt(const std::string& FieldSeed, int F, const std::function<bool(const std::string&)>& Taken);
/** A country's name in a sentence ("Plays from New Zealand.", §4.8.4; B-session's table at G3). Phase A: the code as given. */
SHORTSTACKCORE_API std::string CountryName(const std::string& Code);
} // namespace handles
} // namespace ss
