#pragma once

#include "ShortStack/Common.h"

#include <array>
#include <string>

namespace ss
{
struct PlayerView;
struct DecisionGrade;

/**
 * Coaching (next_segment_spec §2.11): coaches who back the player. Fred's monthly leak review comes with the Lantern
 * Row package; Dee reads the tells she confirmed. Coaching gives information (reviews) and composure (the tilt factor,
 * read weight), never decisions and never play strength. Graded decisions are tallied by spot (online, where grading
 * exists: Session.cpp:2243). Saved as coach\ttally and coach\tnotes.
 *
 * Bodies: core-stake (Coach.cpp). Phase A, day 1: stubs that compile and change nothing; TiltGainNow and
 * ReadWeightNow hand back the background's own value, which is today's behavior at the perk sites.
 */
namespace coach
{
enum class Spot : int
{
	Open,
	DefendBlind,
	Vs3Bet,
	PushFold,
	CallShove,
	Flop,
	FacingFlop,
	Turn,
	RiverValue,
	RiverCall,
	Bubble,
	Bounty,
	Count,
};
struct Tally
{
	int Decisions = 0;
	int Blunders = 0;
	long long LossMilliBb = 0;
};
struct Notes
{
	std::array<Tally, 12> All{};
	std::array<Tally, 12> SinceReview{};
	double LastReview = 0.0;
	double TiltFactor = 1.0;
	double TiltSince = 0.0;
	double ReadsUntil = 0.0;
	int Sessions = 0;
};
struct Review
{
	Spot Worst = Spot::Open;
	Spot Next = Spot::Open;
	double BbPer[2] = {0.0, 0.0};
	std::string Hand;
	std::string Homework;
};
SHORTSTACKCORE_API Spot SpotOf(const PlayerView& V, const DecisionGrade& G, bool Bubble, bool Bounty);
SHORTSTACKCORE_API void Add(Notes& N, Spot S, const DecisionGrade& G);
SHORTSTACKCORE_API Review ReviewOf(const Notes& N, const std::string& Coach);
SHORTSTACKCORE_API double TiltGainNow(const Notes& N, double Background, double World);
/** + 1 for 7 days after Dee's morning-after (P2). */
SHORTSTACKCORE_API int ReadWeightNow(const Notes& N, int Background, double World);
} // namespace coach
} // namespace ss
