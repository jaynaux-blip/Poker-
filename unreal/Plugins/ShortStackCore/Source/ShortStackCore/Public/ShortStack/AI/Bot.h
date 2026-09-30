#pragma once

#include "ShortStack/AI/Profiles.h"
#include "ShortStack/AI/View.h"

namespace ss
{
class Rng;

struct BotContext
{
	const Profile* Prof = nullptr;
	/** 0..1, raised by bad beats and big losses, decays over time. */
	double Tilt = 0.0;
	/** 0..1, how much survival matters right now (bubble, pay jumps). */
	double IcmPressure = 0.0;
	/** Lightweight mode for the off-screen tables of a big field. */
	bool Fast = false;
	Rng* R = nullptr;
};

struct BotDecision
{
	PlayerAction Action;
	/** How long the player "thinks" first: the online timing tell. */
	double ThinkMs = 0.0;
	/** The bot's own equity estimate. */
	double Equity = 0.0;
};

/** Push/fold shoving range for an unopened pot (share of hands). */
double PushRange(double StackBB, int Behind);

/** Chooses an action from public information only. Port of decide() in web/src/core/ai/bot.ts. */
BotDecision Decide(const PlayerView& View, const BotContext& Ctx);
} // namespace ss
