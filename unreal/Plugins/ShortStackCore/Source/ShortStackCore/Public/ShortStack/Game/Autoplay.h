#pragma once

#include "ShortStack/AI/Reads.h"
#include "ShortStack/AI/Style.h"
#include "ShortStack/Common.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace ss
{
class Hand;
class Tournament;
struct SaveData;
namespace life
{
struct State;
}

/**
 * Autoplay (update 2 §2): the game plays the hero's seat in a style the player picks, watched at 1x to 8x or skipped to a
 * target, and hands it back with one press. This is the plan's bookkeeping, shared by the session (online), the card room
 * (FAutoHost) and Dee's: a table's setting (TableAuto), a run's state (Run, restorable outside the engine checkpoint),
 * the player's Form and preferences and what each event's plan played (Book, saved as autoplay\t... lines once
 * Update2LinesRead's bits say so). Only the player's own decisions are ever graded (U4); nothing in the world ever says
 * autoplay played (U1). Declared by the lead in Phase A; bodies by core-autoplay (Game/Autoplay.cpp, SessionAuto.cpp).
 */
namespace autoplay
{
/** Saved: append-only. */
enum class Mode : int
{
	Off,
	Watch,
	Skip,
};
/** Where a skip runs to. Saved: append-only. */
enum class Target : int
{
	NextBreak,
	Bubble,
	FinalTable,
	Result,
	Hands,
	Closing,
};
/** Stop rules (§2.6.4). Saved: append-only bits. */
enum Stop : uint32_t
{
	StopFinalTable = 1,
	StopBubble = 2,
	StopMyAllIn = 4,
	StopBigPot = 8,
	StopPremium = 16,
};
/** One table's plan: the style, watching or skipping, the watch speed (1, 2, 4, 8), the skip's target and the stops. */
struct TableAuto
{
	style::Style S = style::Style::Autopilot;
	Mode M = Mode::Off;
	int Speed = 1;
	Target To = Target::Bubble;
	int Hands = 0;
	uint32_t Stops = StopFinalTable | StopBubble;
};
/**
 * A run (from autoplay turning on for a table until the event ends): the plan's own tilt (quantized to 1e-3), the
 * Sharpness read once when it turned on, the take-over state, the counts by plan and by the player, and what it has
 * observed tonight. Online it lives on the session's TableRun; live, in the checkpoint host text (RunText).
 */
struct Run
{
	double Tilt = 0, Sharp = 0.80;
	double SharpSetAt = 0;
	bool TakenOver = false, TakenMidThink = false;
	double ResumeWindowUntil = 0;
	int AiDecisions = 0, OwnDecisions = 0, AiHands = 0, OwnHands = 0;
	Chips StartStack = 0;
	reads::Table Seen;
};
/** What one event's plan played (autoplay\tplayed): keyed by HistoryEntry::EventId online, the live entry's id live. */
struct Played
{
	std::string Id;
	int Venue = 0, Site = 0; // Venue: 0 online, 1 the Embercrest, 2 Dee's
	style::Style S = style::Style::Autopilot;
	int AiDecisions = 0, OwnDecisions = 0, AiHands = 0, OwnHands = 0;
};
/** The career's defaults (autoplay\tpref) and the player's Form: graded accuracy over their own online decisions. */
struct Prefs
{
	style::Style S = style::Style::Autopilot;
	int Speed = 1;
	Target To = Target::Bubble;
	uint32_t Stops = StopFinalTable | StopBubble;
	double Form = 70.0;
	int FormWeek = -1, WeekDecisions = 0;
};
/** Everything autoplay keeps on the career (life::State::Auto). */
struct Book
{
	Prefs Pref;
	std::vector<Played> Results; // the last 200
	reads::Table Notes;          // named NPCs and the cast, by "npc:<id>" or "cast:<name>"
	std::map<std::string, double> NoteSeen; // when each note was last added to (at most 300)
};
/** A hand's end as the plan's tilt and counters see it. */
struct HandOutcome
{
	bool HeroIn = false, AllIn = false, Won = false, BadBeat = false, ByPlan = false;
	Chips Pot = 0, BigBlind = 0, HeroNet = 0;
	int Decisions = 0;
};
/** Everything the panel (and the PLAY page) draws for a table. */
struct Panel
{
	std::vector<style::Info> Styles;
	std::vector<std::string> Greyed; // by style: why it can't be picked here ("" when it can)
	double Sharp = 0, Form = 0, SharpSetAt = 0;
	std::string SharpWhy; // "Form 89, rested"
	TableAuto Now;
};
/** 0.80 up to a Form of 80, 1.00 at 95 (§2.5). */
SHORTSTACKCORE_API double SkillTerm(double Form);
/** Sharpness, 0.50 to 1.00: SkillTerm with the player's condition (tilt, exhausted, starving or parched). */
SHORTSTACKCORE_API double SharpnessOf(double Form, double Tilt, bool Exhausted, bool Needy);
/** Form as of World (the weekly decay toward 70 applied lazily, by week key). */
SHORTSTACKCORE_API double FormAt(Book& B, double World);
SHORTSTACKCORE_API style::HeroBrain BrainFor(const TableAuto& A, const Run& R, double TiltGain, int Where, const reads::Table* Notes, const style::Learned* Book);
/** The plan's tilt and counters after a hand (Tilt quantized to 1e-3, so the live host text restores it exactly). */
SHORTSTACKCORE_API void AfterHand(Run& R, const HandOutcome& O, double TiltGain);
/** One of the player's own graded decisions: moves Form (not in a hand taken over mid-think) and marks U2AutoPref. */
SHORTSTACKCORE_API void NoteOwnDecision(Book& B, life::State& L, int AccuracyPoints, double World, bool TakenMidThink);
/** A stop rule against the decision the plan drew (both paths, right after HeroDecisionFor): true hands it to the player. */
SHORTSTACKCORE_API bool ShouldStop(const TableAuto& A, const Hand& H, const BotDecision& Planned, int HeroSeat, const Tournament* T);
/** The event's played line (the style used most, the counts). */
SHORTSTACKCORE_API void Record(Book& B, const std::string& Id, int Venue, int Site, const TableAuto& A, const Run& R);
/** An event's observed stats into the notes (named NPCs and the cast only), half weight. */
SHORTSTACKCORE_API void FoldNotes(Book& B, const reads::Table& Event, double World);
SHORTSTACKCORE_API const Played* PlayedFor(const Book& B, const std::string& Id);
/**
 * The table's plan and its run as the live host text's auto line (§2.12: style, mode, speed, tiltMilli, sharpCenti,
 * target, stops, aiDec, ownDec, aiHands, ownHands after the "auto" tag), and back: the line carries both, so a resume
 * restores the plan and the run exactly. The observed stats ride on their own autoseen lines (reads::Write).
 */
SHORTSTACKCORE_API std::string RunText(const TableAuto& A, const Run& R);
SHORTSTACKCORE_API bool ReadRun(const std::string& Text, TableAuto& A, Run& R);
/** Every player-facing string autoplay has (the panel, the strip, the skip card), for the copy lint. */
SHORTSTACKCORE_API std::vector<std::string> Inventory();
/** autoplay\tpref, played and note lines (pure const: they run on the save worker). */
SHORTSTACKCORE_API void WriteLines(const life::State& L, std::string& Out);
/** One autoplay line split on tabs; false when it isn't one. */
SHORTSTACKCORE_API bool ReadLine(const std::vector<std::string>& P, life::State& L);
/** After a save is read (§6.5 step 1): what an older save lacked, by Update2LinesRead's bits. */
SHORTSTACKCORE_API void AfterParse(SaveData& D);
} // namespace autoplay
} // namespace ss
