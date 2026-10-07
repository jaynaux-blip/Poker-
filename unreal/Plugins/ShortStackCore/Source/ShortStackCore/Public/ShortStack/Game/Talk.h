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
 * and the barks in passing, built from what the living world knows. Pure functions on a context; the Session applies
 * the effects (Session::Heard, Lend, Tipped; §7.2). Owned by people-core.
 *
 * **The talk card.** Open gives the greeting, the speaker's name and label (their bond's label, "Knows you", or their
 * role, "Barber"), and the choices: "news" (What's new?), "games" (Any games around?), "service" (the person's service:
 * Get a cut, Menu, Day pass, Sell or pawn...: the host opens that shop's screen), a person's own ("lend": Twitch's
 * twenty, then "lend-yes" / "lend-no"), and "bye" (See you.). Answer gives the line for a choice and the choices after it
 * (none after "bye" or "service"). Each Talk goes to Session::Heard as it's shown: it records the one-time key the line
 * spent (SaidKey: a rumour, a remark about the player's new cut), the familiarity it earned, and the world's memory of
 * the meeting. Build a new context after Heard (it reads what has been said). On "lend-yes" the host calls
 * Session::Lend(Speaker, 2000) and, if that says why not, shows it instead.
 *
 * **The order.** A greeting is the first of, as ClerkLine orders them (Store.cpp): how the player looks (hungry, tired,
 * thirsty, soaked, a fresh cut, a new jacket, famous); what this person knows (a debt either way, a stiffed check, the
 * eviction, a long absence, the player's background, the world's memories of them: "You again."); news (people they know,
 * the player's last result); rumours, each told once (last night's Embercrest winner, tonight's Nightly, the Night Shift
 * board's leader, Dee's next game and who's coming, the Garage and the Basement); then the hour and the weather.
 *
 * **Keepers** speak through KeeperLine at their counters: greet, browse, pick, try, pay, declined, tip, stiff, leave,
 * closing; the barber's wait, grow and pick:color; the thrift's donate; the gym's train and door; the diner's coffee and
 * sit; Augie's appraise (appraise:gear, :cup, :award, :keepsake, :garment, :lot114), offer (offer:<sell cents>:<loan
 * cents> says the numbers as he would), refuse (refuse:dee-chip, refuse:furniture), loan, redeem, forfeit and guitars.
 * Barks (bump, recognize, rain, closing, hello, excuse) are a line in passing; the host keeps them to one a person every
 * two minutes. Every call returns "" when that person has nothing to say to it.
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
	/** What the line is about: "look:hungry", "bond:memory", "news", "rumour", "games", "hour", "service", "bye"... (the
	 *  subtitles' tone; town_test checks the order). */
	std::string Topic;
};

SHORTSTACKCORE_API std::shared_ptr<const TalkContext> ContextFor(const Session& S, const std::string& Speaker, const std::string& Venue);
SHORTSTACKCORE_API Talk Open(const TalkContext& C);
SHORTSTACKCORE_API Talk Answer(const TalkContext& C, const std::string& ChoiceId);
/** greet, browse, pick, try, pay, declined, tip, stiff, leave, closing; appraise, offer, refuse, loan, redeem, forfeit */
SHORTSTACKCORE_API std::string KeeperLine(const TalkContext& C, const std::string& Moment);
/** bump, recognize, rain, closing */
SHORTSTACKCORE_API std::string Bark(const TalkContext& C, const std::string& Trigger);
/** Dollars as a pawnbroker says them: 247 "two-forty-seven", 1370 "thirteen-seventy", 4500 "forty-five hundred". */
SHORTSTACKCORE_API std::string SpokenDollars(long long Dollars);
} // namespace town
} // namespace ss
