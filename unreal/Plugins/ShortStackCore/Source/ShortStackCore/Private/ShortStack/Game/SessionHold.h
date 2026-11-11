#pragma once

// Update 4 (SPEC v1.7 §3.7 R3, R5, R6; §6 PA-35; Phase A step A-2, track A1): the session's side of the held turn, shared by the
// G/Session*.cpp files that touch the living world. Private to the module (the contract is World.h's Holding and DrainHold, A3's
// machine). Every name here starts with Hold (the unity rules).
//
// Drain-on-touch: a session call that changes the world, or reads what steers an outcome, first finishes a held turn
// (HoldDrain), so nothing the session decides ever reads a half-turned world. Display reads (a plate's badge, a view, a const
// accessor) don't drain: they read the last completed revision (R3). WorldStep returns while the turn holds (R6) and Save waits
// for it (R5), so the phone's scan, the stake book's money and a save never see the middle of a turn either.
//
// Phase A: TurnBudgetMs is unlimited, so the world never holds and both helpers do nothing (no output can move).

#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/World.h"

namespace ss
{
namespace session_hold_detail
{
/** Finishes a held turn now (World::DrainHold), as Insights' "SS.World.Turn" (SessionHooks::Scope). Nothing when nothing holds. */
inline void HoldDrain(world::World& W, SessionHooks& Hooks)
{
	if (!W.Holding())
	{
		return;
	}
	Hooks.Scope("SS.World.Turn", true);
	W.DrainHold();
	Hooks.Scope("SS.World.Turn", false);
}

/**
 * The world whole at To, as AdvanceTo left it before the held turn: after an AdvanceTo(To) whose reader needs the clock or the
 * log at once (a load, a host's note, a skip), every held turn is drained and the advance carried on until nothing holds.
 */
inline void HoldWhole(world::World& W, double To, SessionHooks& Hooks)
{
	if (!W.Holding())
	{
		return;
	}
	Hooks.Scope("SS.World.Turn", true);
	while (W.Holding())
	{
		W.DrainHold();
		W.AdvanceTo(To);
	}
	Hooks.Scope("SS.World.Turn", false);
}
} // namespace session_hold_detail
} // namespace ss
