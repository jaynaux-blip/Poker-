#pragma once

#include "ShortStack/Common.h"

#include <memory>
#include <string>
#include <vector>

namespace ss
{
class Session;

/**
 * What people on the block say (the realism update's spec, §6.4): talk on E, the keepers' lines at their counters,
 * and the barks in passing, built from what the living world knows. Pure functions; the Session applies the effects
 * (Session::Heard, Lend, Tipped; §7.2).
 *
 * Owned by people-core. Phase A (core-shops): the contract only; the bodies return defaults (nothing said) until
 * people-core fills them.
 */
namespace town
{
struct TalkContext; // opaque to callers: built by ContextFor from the session, the world, the hour, the weather, the bond

struct Choice
{
	std::string Id;
	std::string Label;
};

struct Talk
{
	std::string Name;
	std::string Label;
	std::string Line;
	std::vector<Choice> Choices;
	std::string SaidKey;
	int Familiarity = 0;
};

SHORTSTACKCORE_API std::shared_ptr<const TalkContext> ContextFor(const Session& S, const std::string& Speaker, const std::string& Venue);
SHORTSTACKCORE_API Talk Open(const TalkContext& C);
SHORTSTACKCORE_API Talk Answer(const TalkContext& C, const std::string& ChoiceId);
/** greet, browse, pick, try, pay, declined, tip, stiff, leave, closing; appraise, offer, refuse, loan, redeem, forfeit */
SHORTSTACKCORE_API std::string KeeperLine(const TalkContext& C, const std::string& Moment);
/** bump, recognize, rain, closing */
SHORTSTACKCORE_API std::string Bark(const TalkContext& C, const std::string& Trigger);
} // namespace town
} // namespace ss
