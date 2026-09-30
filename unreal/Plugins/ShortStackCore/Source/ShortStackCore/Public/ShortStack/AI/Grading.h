#pragma once

#include "ShortStack/AI/Model.h"

namespace ss
{
class Rng;

/** Chess-engine style decision grades, best to worst. */
enum class Grade : int
{
	Best,
	Good,
	Inaccuracy,
	Mistake,
	Blunder,
};

const char* GradeName(Grade G);
/** Accuracy points per grade (Best 100 .. Blunder 0). */
double GradeScore(Grade G);

struct OptionEV
{
	std::string Label; // "Fold", "Call", "Raise to 900", "All-in"
	PlayerAction Action;
	double Ev = 0.0; // chips, relative to folding now
};

struct DecisionAnalysis
{
	PlayerView View;
	double Equity = 0.0;
	double PotOdds = 0.0;
	std::vector<OptionEV> Options;
	std::vector<OpponentModel> Models;
};

struct DecisionGrade
{
	Street OnStreet = Street::Preflop;
	Grade Result = Grade::Best;
	OptionEV Chosen;
	OptionEV Best;
	double Equity = 0.0;
	double PotOdds = 0.0;
	double LossBB = 0.0;
	std::string Note;
};

/**
 * Estimates the chip EV of every option at a decision point: equity vs the
 * ranges implied by opponents' actions, fold equity for bets and raises, and
 * equity realization for hands that continue. A lightweight model, not a
 * solver: exact for all-in and river spots, directional elsewhere.
 */
DecisionAnalysis AnalyzeDecision(const PlayerView& View, Rng& R, int Iterations = 1500);
OptionEV EvaluateChoice(const DecisionAnalysis& Analysis, const PlayerAction& Action);
DecisionGrade GradeDecision(const DecisionAnalysis& Analysis, const PlayerAction& Action);
double Accuracy(const std::vector<DecisionGrade>& Grades);
} // namespace ss
