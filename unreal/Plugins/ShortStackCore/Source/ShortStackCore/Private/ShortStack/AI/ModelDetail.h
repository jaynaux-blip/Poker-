#pragma once

// Internal to the core (update 2 §2.4.1, §6.1): the engine's own helpers the styles build their decisions from, declared
// here so AI/Style*.cpp can call them. The bodies stay where they are and don't change (Bot.cpp's bot_detail, Profiles.cpp's
// profiles_detail: golden-pinned). Model.h's ModelSeat, MadeLevel, HasDraw, BoardCategory and HeuristicEquity are public
// already. Frozen after Phase A.

#include "ShortStack/AI/Bot.h"
#include "ShortStack/AI/Profiles.h"
#include "ShortStack/AI/View.h"

namespace ss
{
namespace bot_detail
{
/** The share of hands a position opens before the profile's width (UTG 0.12 to the button's 0.42). */
double OpenRange(Position P);
/** The profile as tilt bends it (wider, stickier, more bluffs; Tilt <= 0 returns P as it is). */
Profile Tilted(const Profile& P, double Tilt);
/** The action made legal for the view: a raise clamped to the limits, else a call or a check. */
PlayerAction Legalize(const PlayerView& V, const PlayerAction& A);
/** How long a bot thinks, in ms (draws from R unless the profile's ThinkBase is 0: a hero profile's draws nothing). */
double ThinkTime(const Profile& P, Rng& R, double Strength, double Difficulty, bool Snap);
/** The big blind in chips, at least 1. */
double BbOf(const PlayerView& V);
/** All in: a raise to the most the view allows. */
PlayerAction AllIn(const PlayerView& V);
/** The last player still in to act postflop is the one in V.Me's seat. */
bool IsInPosition(const PlayerView& V);
BotDecision MakeChoice(const PlayerAction& Action, double ThinkMs, double Equity);
} // namespace bot_detail

namespace profiles_detail
{
/** An archetype's numbers before MakeProfile's jitter (no RNG). */
Profile Base(Archetype A);
} // namespace profiles_detail
} // namespace ss
