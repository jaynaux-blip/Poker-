#pragma once

#include "ShortStack/Tournament.h"

#include <memory>
#include <string>
#include <vector>

namespace ss
{
namespace live
{
/**
 * The Riverside's Sunday $150: a live 6-max deepstack in the casino's poker room, 7 PM, late
 * registration until 7:45. Dee deals the feature table on weekends.
 *
 * The field is ss::Tournament like an online event; the host plays the player's table hand by hand
 * at a real table and the rest of the room in the background. The feature table is where the
 * people with faces are: the Back Room's regulars, gh0stfold, and a few Sunday faces. Tournament::
 * FeatureIds keeps the player's table stocked with them while any are left in.
 */
constexpr Chips RiversideBuyInCents = 15000;
constexpr Chips RiversideFeeCents = 1500;
constexpr int RiversideTableSize = 6;

/** Someone you can meet at the feature table: their name in the field and how they play. */
struct CastMember
{
	const char* Id;   // the tournament player id ("npc:" + Name)
	const char* Name; // as the field knows them
	Archetype Type;
};

/** The feature table's cast, in the order they're seated from the start. */
SHORTSTACKCORE_API const std::vector<CastMember>& RiversideCast();

/** Twenty-minute levels from 100/200, a big-blind ante from level 3; 20,000 starts (100 BB). */
SHORTSTACKCORE_API const std::vector<Level>& DeepstackLevels();

/** The event as it runs on Day (net:: day number of the Sunday); the field size varies week to week. */
SHORTSTACKCORE_API TournamentSpec RiversideSpec(int Day);

/** The cast as tournament reserved players. */
SHORTSTACKCORE_API std::vector<ReservedPlayer> RiversideReserved();

/** A full Riverside tournament for the player, with the feature table switched on. */
SHORTSTACKCORE_API std::unique_ptr<Tournament> MakeRiverside(int Day, const std::string& HeroName, const std::string& Seed);

/** "Table 6, seat 3" style seat labels start at 1. */
inline int SeatLabel(int Seat) { return Seat + 1; }
} // namespace live
} // namespace ss
