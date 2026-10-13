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
struct ActionRecord;
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
	// Appended by core-autoplay (G1): what the watch strip, the skip card and the table tile draw too. Styles and Greyed
	// are indexed by style::Style. With autoplay off, Now is the plan the table would take (Mode Off): its own once it has
	// had one (a style picked there, a run turned off), else the career's default; Sharp is what the plan would get if it
	// took the seat now. With it on, Sharp is the run's, held since SharpSetAt (world minutes).
	bool On = false;           // the plan holds or skips this table (Now.M != Off)
	bool Rusty = false;        // Form has slipped for lack of play since the player last played (SharpWhy says so)
	bool Thinking = false;     // the plan's think is on screen: its move is never shown before it's played (U1)
	double ActsIn = 0;         // seconds of that think left
	bool TakenOver = false;    // this hand is the player's (a take-over or a stop rule)
	bool ResumeWindow = false; // "AUTO RESUMES · [K] keep playing": K now turns autoplay off (§2.6.5)
	double ResumeLeft = 0;     // seconds of that window left
	bool Skipping = false;     // the skip runs, or starts at the next hand
	std::string LastAction;    // the plan's last action this hand once played ("CALLED 3,200"); "" when none
	std::string Status;        // the table tile's line: "AUTO · GTO 2x", "SKIP · 31 to the bubble"; "" when off
	std::string SkipLine;      // the skip card: "SKIPPING · PUSH/FOLD · hand 214 · level 11 · 31 to the bubble"
	std::string StopReason;    // why the last skip stopped ("The bubble."), "" when none
	int AiDecisions = 0, OwnDecisions = 0, AiHands = 0, OwnHands = 0;
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

// ---- Appended by core-autoplay (G1). Shared by the session (online), FAutoHost (the card room) and the tests.

/** "AUTO RESUMES · [K] keep playing" lasts this long into the hand after a take-over (§2.6.5), in real seconds. */
constexpr double ResumeWindowSeconds = 3.0;
/** Form's bounds and start (§2.5): a new career's, and where the weekly decay pulls it. */
constexpr double FormStart = 70.0;
/**
 * Dee's clock under the plan (§1.1, §2.6.3): world minutes a hand while the plan holds the seat. 4.5 until spike S-9
 * measures human-paced hands at LIVE pace and replaces it; today's manual clock is real seconds × 6, so 4.5 is the
 * DeeManualSecondsPerHand below. card_room's RoomPace holds autoplay's hands per world hour within ±10% of that rate.
 */
constexpr double DeeMinutesPerHand = 4.5;
constexpr double DeeManualSecondsPerHand = 45.0;
/** The world week a moment falls in (Form's decay key): world day / 7, weeks starting Mondays (day 0 is Mon Oct 5). */
SHORTSTACKCORE_API int WeekOf(double World);
/** Form as of World without changing the book (FormAt on a copy); Rusty: the decay took something since the last play. */
SHORTSTACKCORE_API double FormNow(const Book& B, double World, bool* Rusty = nullptr);
/**
 * The pot share the player held when the cards went on their backs in an all-in (§2.5's bad beat): EquityKnownHands
 * over the hands still in, from the board as it was then (exact from the flop on; preflop 2,000 samples on a seed taken
 * from the hand itself, so watch and skip get the same number). -1 when the player wasn't all in and called.
 */
SHORTSTACKCORE_API double AllInEquity(const Hand& H, int HeroSeat);
/** A finished hand as the plan's tilt and counts see it (HeroSeat: the player's index in H.Seats). Draws nothing. */
SHORTSTACKCORE_API HandOutcome OutcomeOf(const Hand& H, int HeroSeat, bool ByPlan);
/**
 * A hand the player was dealt, finished, on any path and at any venue: AfterHand on its OutcomeOf, then reads::Count of
 * it into R.Seen (read only from the next hand on). The session's AutoAfterHand, FAutoHost's OnHandEnded and the live
 * skip's sink all call it. Returns the outcome.
 */
SHORTSTACKCORE_API HandOutcome NoteHand(Run& R, const Hand& H, int HeroSeat, bool ByPlan, double TiltGain);
/**
 * A hand the live skip played (live::SkipSlice's sink calls this instead of NoteHand): NoteHand by the plan, and every
 * action of the player's seat in it counted as one of the plan's decisions (a skipped hand is never taken over; the
 * online skip and both watch paths count each decision as it's made). Without it a skipped night's played line and the
 * auto host line would carry the plan's hands but none of its decisions. Returns the outcome.
 */
SHORTSTACKCORE_API HandOutcome NoteSkippedHand(Run& R, const Hand& H, int HeroSeat, double TiltGain);
/**
 * The key the notes use for a player (§2.4.3; autoplay\tnote): "npc:<name>" for one of the world's people (a seat whose id
 * is already "npc:<name>", as the card room's faces and the rival are, or a RiverLine regular by the name the table
 * shows, T's player name) and "cast:<name>" for Dee's cast (no tournament: the seat's id is their name). World names
 * are unique (the field never borrows one), so the name finds the same person event after event; a seat filled by a
 * stranger finds no note. T may be null (Dee's).
 */
SHORTSTACKCORE_API std::string NoteKey(const Tournament* T, const std::string& PlayerId);
/** The notes row for the player at a seat (NoteKey's), or null. What Exploitative reads beside B.Seen. */
SHORTSTACKCORE_API const reads::Stats* NoteFor(const reads::Table* Notes, const Tournament* T, const std::string& PlayerId);
/** The career's default plan (autoplay\tpref) as a table's setting, Mode Off. */
SHORTSTACKCORE_API TableAuto DefaultPlan(const Book& B);
/** "Make this my default": the plan's style, speed, target and stops become the career's (autoplay\tpref). */
SHORTSTACKCORE_API void SetDefault(Book& B, life::State& L, const TableAuto& A);
/**
 * Why a style can't be picked at a table here ("" when it can): style::Unavailable's reason (core-styles'), and for a
 * hidden style (the ship rule, §2.5: its tile isn't shown) "Not on the menu". Where: 0 online, 1 the Embercrest, 2 Dee's.
 */
SHORTSTACKCORE_API std::string CantPick(style::Style S, int Where, bool Knockout);
/** The style that actually plays at a venue: at Dee's, a tournament-only style plays as its AtCash style. */
SHORTSTACKCORE_API style::Style PlaysAs(style::Style S, int Where);

// The copy (system UI only: U1 keeps every word of it out of the world). Each string is in Inventory() and linted.
SHORTSTACKCORE_API const char* TargetName(Target To);  // "Next break", "Bubble", "Final table", "Result", "50 hands", "Closing"
SHORTSTACKCORE_API const char* StopName(uint32_t Bit); // "Final table", "Bubble", "My all-in decisions", ...
SHORTSTACKCORE_API std::string SpeedName(int Speed);   // "1x" .. "8x"
/** "GTO is thinking…": the think bar's line. It never names a move. */
SHORTSTACKCORE_API std::string ThinkingLine(style::Style S);
/** The plan's action once played, for the strip: "FOLDED", "CHECKED", "CALLED 3,200", "BET 1,500", "RAISED TO 9,000", "ALL IN 12,400". */
SHORTSTACKCORE_API std::string ActionLine(const ActionRecord& A);
/** The P0a status line (§2.11): "AUTOPLAY · GTO · Sharpness 93% · [K] take over". */
SHORTSTACKCORE_API std::string StatusLine(const TableAuto& A, double Sharp);
/** The table tile's one line (§2.11): "AUTO · GTO 2x", "SKIP · 31 to the bubble" (T may be null); "" when off. */
SHORTSTACKCORE_API std::string TileStatus(const TableAuto& A, const Tournament* T);
/** The skip card's line: "SKIPPING · PUSH/FOLD · hand 214 · level 11 · 31 to the bubble". */
SHORTSTACKCORE_API std::string SkipLine(const TableAuto& A, const Tournament& T);
/** The results line: "Played by your plan: 214 of 260 decisions (LAG)." */
SHORTSTACKCORE_API std::string PlayedLine(const Played& P);
/** Career's tag for a row the plan played: "AUTO · LAG 82%" (the plan's share of the decisions). */
SHORTSTACKCORE_API std::string CareerTag(const Played& P);
/** Sharpness's causes: "Form 89, rested", "Form 84, rusty: play some hands", "Form 70, tired and hungry". */
SHORTSTACKCORE_API std::string SharpWhy(double Form, bool Rusty, double Tilt, bool Exhausted, bool Needy);
/** "Sharpness 92%: Form 89, rested". */
SHORTSTACKCORE_API std::string SharpLine(double Sharp, const std::string& Why);
/** The PLAY page's warning when the plan will be sloppy (Sharpness under 0.70), else "": "Sharpness 63%: tired and hungry, and your Form's still 70. Your plan gets sloppy." */
SHORTSTACKCORE_API std::string SharpWarning(double Sharp, double Form, double Tilt, bool Exhausted, bool Needy);
/** The fixed lines (not saved: entries may be added before Count). */
enum class Words : int
{
	PanelTitle,    // "AI AUTOPLAY" (the panel's and the PLAY page's header: system UI, U1)
	TakeOverKey,   // "Take over [K]"
	Resumes,       // "AUTO RESUMES · [K] keep playing"
	SkipKeys,      // "[K] Take the wheel · [Backspace] Stop"
	MakeDefault,   // "Make this my default"
	WantToSee,     // "You'll want to see this."
	DeeFirstNight, // Dee's line on night 1's PLAY page
	Paused,        // "Your plan's paused while you're up."
	StandUpAway,   // "Standing up pauses your plan."
	// Why a skip stopped (Panel::StopReason; the session's SprintStopReason while the plan skips).
	AtBreak,       // "The break."
	AtLevel,       // "A new level." (an online event takes no breaks: Next break stops at the next level)
	AtBubble,      // "The bubble."
	AtHandForHand, // "Hand for hand."
	AtFinalTable,  // "The final table. You'll want to see this."
	AtHands,       // "That's the hands you asked for."
	HandedOver,    // "A stop rule: this decision's yours."
	StoppedByYou,  // "Stopped. Your plan has the seat at 1x."
	// The session's answers (Session::SetAutoplay, TakeOver, SkipAhead: "" or one of these).
	NoTable,        // "There's no table there."
	TableDone,      // "That table's finished."
	ClosingIsCash,  // "Closing time is for a cash game."
	AlreadyInMoney, // "You're already in the money."
	AlreadyFinal,   // "You're at the final table already."
	NotOn,          // "Autoplay isn't on at this table."
	BustedSkipping, // "Busted while skipping" (the bust banner's line under a skip)
	Count,
};
SHORTSTACKCORE_API const char* Text(Words W);
/** "Your plan's back on: GTO." (sitting back down), "Your plan was playing: GTO. [K] carry on." (a resumed night), "WATCH 4x". */
SHORTSTACKCORE_API std::string BackOnLine(style::Style S);
SHORTSTACKCORE_API std::string WasPlayingLine(style::Style S);
SHORTSTACKCORE_API std::string WatchToast(int Speed);
} // namespace autoplay
} // namespace ss
