#pragma once

#include "ShortStack/Common.h"

#include <array>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
struct PlayerView;
struct DecisionGrade;

/**
 * Coaching (next_segment_spec §2.11): coaches who back the player. Fred's monthly leak review comes with the Lantern
 * Row package; Dee reads the tells she confirmed. Coaching gives information (reviews) and composure (the tilt factor,
 * read weight), never decisions and never play strength. Graded decisions are tallied by spot (online, where grading
 * exists: Session.cpp's grading hook). Saved as coach\ttally, coach\tnotes and coach\tworst.
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
constexpr int SpotCount = static_cast<int>(Spot::Count);
struct Tally
{
	int Decisions = 0;
	int Blunders = 0;
	long long LossMilliBb = 0;
};
/** A real hand of the player's: the worst decision in a spot since the last review (what the review opens on). */
struct Worst
{
	long long LossMilliBb = 0;
	std::string Hand; // "A-T off, 22 big blinds, facing a 9-blind shove on the bubble: called; folding was 0.6 bb better."
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
	std::array<Worst, 12> Worsts{};
	/** The last review's homework (a spot, -1 none) and its bb per decision then: checked at the next review. */
	int Homework = -1;
	long long HomeworkMilliBb = 0;
	int Reviews = 0;
};
struct Review
{
	Spot Worst = Spot::Open;
	Spot Next = Spot::Open;
	double BbPer[2] = {0.0, 0.0};
	std::string Hand;
	std::string Homework;
	/** The rest of what the coach says, for the letter: the two spots, the hand, last time's homework. */
	int Decisions = 0;
	int Counts[2] = {0, 0};
	bool Enough = false; // 200 graded decisions since the last review
	std::string Body;
	std::string Subject;
};
/** Graded decisions a review needs since the last one ("Play me two hundred hands and we'll talk."). */
constexpr int ReviewDecisions = 200;

SHORTSTACKCORE_API Spot SpotOf(const PlayerView& V, const DecisionGrade& G, bool Bubble, bool Bounty);
SHORTSTACKCORE_API void Add(Notes& N, Spot S, const DecisionGrade& G);
/** "Bubble calls", "Opening the pot"... */
SHORTSTACKCORE_API const char* SpotName(Spot S);
SHORTSTACKCORE_API Review ReviewOf(const Notes& N, const std::string& Coach);
/** The review delivered: the tallies since start over, the homework kept for next time. */
SHORTSTACKCORE_API void Reviewed(Notes& N, const Review& R, double World);
SHORTSTACKCORE_API double TiltGainNow(const Notes& N, double Background, double World);
/** + 1 for 7 days after Dee's morning-after (P2). */
SHORTSTACKCORE_API int ReadWeightNow(const Notes& N, int Background, double World);
/** Every line of coaching copy (the review's sentences, the empty states), for the copy lint. */
SHORTSTACKCORE_API std::vector<std::pair<std::string, std::string>> Inventory();
/** The coach lines (§7.4): coach\ttally, coach\tnotes, coach\tworst; only once there's something to keep. */
SHORTSTACKCORE_API void WriteLines(const Notes& N, std::string& Out);
SHORTSTACKCORE_API bool ReadLine(const std::vector<std::string>& Fields, Notes& N);
} // namespace coach
} // namespace ss
