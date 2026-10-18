// night_one_session: core-autoplay's additions (update 2 SPEC §6.7): assertions 11 and 12 of §2.10 (WatchEqualsSkip,
// WatchEqualsSkipWithStops, SharpnessIgnoresOtherTables, ResumeEqualsUninterrupted, TakeoverPrefix), AdoptHand, the stop
// rules, no grading, Form's decay and round trip, the played lines, the Career "—", Kast's factors by speed. Compiled
// into session_test and called from main() in session_main.cpp (the kit is SessionSeg.h). core-autoplay's file; its
// helpers live in session_test::auto_seg.
//
//   AutoUnits          SkillTerm, Sharpness, Form's decay and its 1/400 step, the plan's tilt, BrainFor, ShouldStop on
//                      crafted hands, the played record, notes, the auto host line, the save lines and their migration,
//                      the deterministic all-in equity, the copy lint over autoplay::Inventory().
//   AutoFieldPots      bug 1: a big pot with one of the world's people reaches FieldPots on the post-hand hook.
//   AutoNoGrading      a whole event played by the plan (watched, then skipped to the result): nothing graded, the coach's
//                      tallies and Form untouched, a played line, the Career "—" (no decisions of the player's own).
//   AutoWatchEqualsSkip  every style: the same hyper watched at 8x and skipped plays the same hand at every tick, and the
//                      engine checkpoint and the run match where both deal the next hand.
//   AutoWithStops      every decision-level stop on: the hand-offs at the same tick, action and T->R state (AdoptHand
//                      replays the paused hand to the decision).
//   AutoSprintParity   today's Sprint (autoplay off) and the plan's Autopilot skip play the same ticks to the bubble's stop.
//   AutoTakeover       a take-over at hand k leaves hands 0..k-1 as they were; mid-think it doesn't move Form; the resume
//                      window; K in it turns autoplay off.
//   AutoFormRoundTrip  a career with graded decisions and no panel use keeps its Form through save and load (1e-9).
//   AutoParityCommand  ss.Auto.Parity at home: watched and headless hashes equal, the table carries on.
//   AutoKastSpeed      the stream's inputs by speed (4x, 8x and a skip).
//   AutoOtherTables    SharpnessIgnoresOtherTables: a skipping table and a watched one play as each does alone.
//   AutoResume         ResumeEqualsUninterrupted: every style, a live-style skip resumed from the engine checkpoint and the
//                      host text (auto and autoseen lines) plays on as the uninterrupted one.
#include "CopyLint.h"
#include "SessionSeg.h"

#include "ShortStack/AI/Reads.h"
#include "ShortStack/AI/Style.h"
#include "ShortStack/Cards.h"
#include "ShortStack/Game/Autoplay.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/LiveAuto.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Hand.h"
#include "ShortStack/Rng.h"
#include "ShortStack/Tournament.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace session_test
{
namespace auto_seg
{
namespace autoplay = ss::autoplay;
namespace style = ss::style;

double AutoSecondsSince(const std::chrono::steady_clock::time_point& T0)
{
	return std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count();
}

autoplay::TableAuto AutoPlanOf(style::Style S, autoplay::Mode M, int Speed = 8, autoplay::Target To = autoplay::Target::Result, int Hands = 0, uint32_t Stops = 0)
{
	autoplay::TableAuto A;
	A.S = S;
	A.M = M;
	A.Speed = Speed;
	A.To = To;
	A.Hands = Hands;
	A.Stops = Stops;
	return A;
}

/** A hand as text: who sat where with what, every action, the board, the pots. Two equal texts are the same hand. */
std::string AutoHandText(const ss::Hand& H)
{
	std::string Out;
	for (const ss::HandSeat& S : H.Seats)
	{
		Out += S.Id + "@" + std::to_string(S.Seat) + ":" + std::to_string(S.StartStack) + ">" + std::to_string(S.Stack) + " ";
	}
	Out += "|";
	for (const ss::ActionRecord& A : H.History)
	{
		Out += std::to_string(static_cast<int>(A.OnStreet)) + "/" + std::to_string(A.Seat) + "/" + std::to_string(static_cast<int>(A.Type)) + "/" + std::to_string(A.To) +
			(A.AllIn ? "!" : "") + " ";
	}
	Out += "|";
	for (const ss::Card C : H.Board)
	{
		Out += ss::CardToString(C);
	}
	for (const ss::PotResult& P : H.PotResults)
	{
		Out += " " + std::to_string(P.Amount) + ">";
		for (const int W : P.Winners)
		{
			Out += std::to_string(W) + ",";
		}
	}
	return Out;
}

/** The hero's hand of a tick, once played (the watch path's between its finish and the next deal; a skip's right after). */
void AutoNoteTick(const ss::Session& S, std::map<int, std::string>& Hands)
{
	if (!S.T)
	{
		return;
	}
	if (const ss::Hand* Last = S.T->LastHeroHand())
	{
		const int Tick = S.T->FinishPending() ? S.T->Tick : S.T->Tick - 1;
		Hands[Tick] = AutoHandText(*Last);
	}
}

/** The run with the table's plan left out (a stop changes the mode and the speed): tilt, Sharpness, the counts, the stats. */
std::string AutoRunCore(ss::Session& S)
{
	const std::string Text = S.DebugAuto("run");
	const size_t Eol = Text.find('\n');
	autoplay::TableAuto A;
	autoplay::Run R;
	autoplay::ReadRun(Text.substr(0, Eol), A, R);
	return autoplay::RunText(autoplay::TableAuto(), R) + (Eol == std::string::npos ? std::string() : Text.substr(Eol));
}

/** Registers the suite's 180-runner hyper ($0.25 Hyper Sprint) with a bankroll to play it. */
void AutoSeat(ss::Session& S)
{
	S.CurrentScreen = ss::Screen::Lobby;
	S.BankrollCents = 10000;
	S.Register(1);
}

int AutoDiffTicks(const std::map<int, std::string>& A, const std::map<int, std::string>& B, int Upto, int& Compared)
{
	int First = -1;
	Compared = 0;
	for (const auto& It : A)
	{
		if (It.first >= Upto)
		{
			break;
		}
		const auto Other = B.find(It.first);
		if (Other == B.end())
		{
			continue;
		}
		++Compared;
		if (Other->second != It.second && First < 0)
		{
			First = It.first;
		}
	}
	return First;
}

// ------------------------------------------------------------------ units

ss::Hand AutoCraftedHand(ss::Rng& R)
{
	ss::HandConfig Cfg;
	for (int K = 0; K < 4; ++K)
	{
		ss::SeatInput In;
		In.Seat = K;
		In.Id = K == 0 ? std::string(ss::HeroId) : "p" + std::to_string(K);
		In.Stack = 2000;
		Cfg.Players.push_back(In);
	}
	Cfg.ButtonSeat = 3;
	Cfg.SmallBlind = 25;
	Cfg.BigBlind = 50;
	return ss::Hand(Cfg, R);
}

void AutoUnits()
{
	// SkillTerm and Sharpness (§2.5).
	Expect(autoplay::SkillTerm(70) == 0.80 && autoplay::SkillTerm(80) == 0.80 && std::fabs(autoplay::SkillTerm(87.5) - 0.90) < 1e-12 && autoplay::SkillTerm(95) == 1.0 &&
			   autoplay::SkillTerm(100) == 1.0,
		"autoplay: SkillTerm is 0.80 to a Form of 80 and 1.00 from 95");
	Expect(autoplay::SharpnessOf(70, 0, false, false) == 0.80, "autoplay: the default Sharpness is 0.80 (Form 70, rested, calm)");
	Expect(autoplay::SharpnessOf(70, 0, true, true) == 0.63, "autoplay: tired and running on empty at Form 70 is Scene 3's 63%");
	Expect(autoplay::SharpnessOf(95, 1.0, false, false) == 0.76 && autoplay::SharpnessOf(70, 1.0, true, true) == 0.50, "autoplay: tilt costs, and Sharpness floors at 0.50");
	Expect(autoplay::SharpWarning(0.63, 70, 0, true, true) == "Sharpness 63%: tired and running on empty, and your Form's still 70. Your plan gets sloppy." &&
			   autoplay::SharpWarning(0.80, 70, 0, false, false).empty(),
		"autoplay: the PLAY page's warning when the plan will be sloppy");
	Expect(autoplay::SharpLine(0.92, autoplay::SharpWhy(89, false, 0, false, false)) == "Sharpness 92%: Form 89, rested", "autoplay: Sharpness and its causes");

	// Form: the 1/400 step, the weekly decay toward 70 (lazily, by week), the mid-think rule, thousandths.
	{
		const double Week = 7.0 * 24.0 * 60.0;
		ss::life::State L;
		autoplay::Book B;
		autoplay::NoteOwnDecision(B, L, 100, 3.0 * 1440.0, false);
		Expect(std::fabs(B.Pref.Form - 70.075) < 1e-12 && B.Pref.WeekDecisions == 1 && (L.Update2LinesRead & ss::life::U2AutoPref), "autoplay: a Best moves Form 1/400 of the way (70.075) and marks the pref line");
		autoplay::NoteOwnDecision(B, L, 0, 3.5 * 1440.0, true);
		Expect(std::fabs(B.Pref.Form - 70.075) < 1e-12 && B.Pref.WeekDecisions == 2, "autoplay: a decision in a hand taken over mid-think counts for the week, not for Form");
		B.Pref.Form = 90.0;
		B.Pref.FormWeek = 0;
		B.Pref.WeekDecisions = 150;
		bool Rusty = true;
		Expect(autoplay::FormNow(B, 1.5 * Week, &Rusty) == 90.0 && !Rusty, "autoplay: a week with 100 graded decisions keeps Form");
		Expect(autoplay::FormNow(B, 2.5 * Week, &Rusty) == 89.0 && Rusty, "autoplay: then an idle week costs a point (lazily, by week), and it says rusty");
		B.Pref.WeekDecisions = 20;
		Expect(autoplay::FormAt(B, 3.5 * Week) == 87.0 && B.Pref.FormWeek == 3 && B.Pref.WeekDecisions == 0, "autoplay: a short week and two idle ones: three points");
		B.Pref.Form = 69.5;
		Expect(autoplay::FormAt(B, 6.0 * Week) == 70.0, "autoplay: below 70 the decay pulls up to 70 and no further");
		B.Pref.Form = 81.2345678;
		autoplay::NoteOwnDecision(B, L, 88, 6.1 * Week, false);
		Expect(B.Pref.Form == std::round(B.Pref.Form * 1000.0) / 1000.0, "autoplay: Form is kept in thousandths (formMilli round-trips it exactly)");
	}

	// The plan's tilt (§2.5) and its counts; BrainFor.
	{
		autoplay::Run R;
		autoplay::HandOutcome O;
		O.HeroIn = true;
		O.BigBlind = 100;
		O.BadBeat = true;
		O.ByPlan = true;
		autoplay::AfterHand(R, O, 1.0);
		Expect(R.Tilt == 0.419 && R.AiHands == 1, "autoplay: a bad beat: (0 + 0.45) x 0.93, to the thousandth");
		O.BadBeat = false;
		O.Won = true;
		autoplay::AfterHand(R, O, 1.0);
		Expect(R.Tilt == 0.25, "autoplay: a won pot takes 0.15 off, then x0.93");
		O.Won = false;
		O.Pot = 2500;
		O.ByPlan = false;
		autoplay::AfterHand(R, O, 1.5);
		Expect(R.Tilt == std::round((0.25 + 0.30) * 0.93 * 1000.0) / 1000.0 && R.OwnHands == 1, "autoplay: a lost pot over 20 BB, by the background's TiltGain");
		O.HeroIn = false;
		const double Before = R.Tilt;
		autoplay::AfterHand(R, O, 1.0);
		Expect(R.Tilt == std::round(Before * 0.93 * 1000.0) / 1000.0, "autoplay: a folded hand only cools");
		const style::HeroBrain Auto = autoplay::BrainFor(AutoPlanOf(style::Style::Autopilot, autoplay::Mode::Watch), R, 1.2, 0, nullptr, nullptr);
		const style::HeroBrain Lag = autoplay::BrainFor(AutoPlanOf(style::Style::Lag, autoplay::Mode::Watch), R, 1.2, 1, nullptr, nullptr);
		Expect(Auto.Tilt == 0.0 && Lag.Tilt == R.Tilt && Lag.Seen == &R.Seen && Lag.Where == 1 && Lag.TiltGain == 1.2, "autoplay: BrainFor (Autopilot never tilts: today's Sprint)");
	}

	// The stop rules on crafted hands (§2.6.4).
	{
		ss::Rng R("auto-stops");
		ss::Hand H = AutoCraftedHand(R);
		const int Hero = 0;
		H.Seats[0].Hole = {ss::MakeCard(10, 0), ss::MakeCard(10, 1)}; // QQ
		ss::BotDecision Fold;
		Fold.Action = ss::PlayerAction::Fold();
		const autoplay::TableAuto None = AutoPlanOf(style::Style::Tag, autoplay::Mode::Skip);
		autoplay::TableAuto Premium = None;
		Premium.Stops = autoplay::StopPremium;
		Expect(!autoplay::ShouldStop(None, H, Fold, Hero, nullptr), "autoplay: no stop rules, no stop");
		Expect(autoplay::ShouldStop(Premium, H, Fold, Hero, nullptr), "autoplay: QQ at the first decision goes to the player (premium hands)");
		H.Seats[0].Hole = {ss::MakeCard(12, 0), ss::MakeCard(11, 1)}; // AKo
		Expect(autoplay::ShouldStop(Premium, H, Fold, Hero, nullptr), "autoplay: so does AK");
		H.Seats[0].Hole = {ss::MakeCard(9, 0), ss::MakeCard(9, 1)}; // JJ
		Expect(!autoplay::ShouldStop(Premium, H, Fold, Hero, nullptr), "autoplay: JJ isn't premium");
		H.Seats[0].Hole = {ss::MakeCard(12, 0), ss::MakeCard(12, 1)};
		ss::ActionRecord Mine;
		Mine.Seat = H.Seats[0].Seat;
		H.History.push_back(Mine);
		Expect(!autoplay::ShouldStop(Premium, H, Fold, Hero, nullptr), "autoplay: only at the first decision of the hand");
		autoplay::TableAuto AllIn = None;
		AllIn.Stops = autoplay::StopMyAllIn;
		ss::BotDecision Jam;
		Jam.Action = ss::PlayerAction::RaiseTo(static_cast<double>(H.Seats[0].Stack + H.Seats[0].StreetBet));
		ss::BotDecision Raise;
		Raise.Action = ss::PlayerAction::RaiseTo(static_cast<double>(H.CurrentBet * 3));
		Expect(autoplay::ShouldStop(AllIn, H, Jam, Hero, nullptr) && !autoplay::ShouldStop(AllIn, H, Raise, Hero, nullptr), "autoplay: the plan's all in goes to the player, a raise doesn't");
		ss::BotDecision Call;
		Call.Action = ss::PlayerAction::Call();
		H.CurrentBet = H.Seats[0].StreetBet + (H.Seats[0].Stack + 1) / 2;
		const bool Half = autoplay::ShouldStop(AllIn, H, Call, Hero, nullptr);
		H.CurrentBet = H.Seats[0].StreetBet + H.Seats[0].Stack / 4;
		Expect(Half && !autoplay::ShouldStop(AllIn, H, Call, Hero, nullptr), "autoplay: a call of half the stack goes to the player, a quarter doesn't");
		autoplay::TableAuto Pot = None;
		Pot.Stops = autoplay::StopBigPot;
		const bool Small = autoplay::ShouldStop(Pot, H, Fold, Hero, nullptr);
		H.Seats[1].Committed = 50 * 45;
		Expect(!Small && autoplay::ShouldStop(Pot, H, Fold, Hero, nullptr), "autoplay: a pot over 40 BB goes to the player");
	}

	// The all-in equity is a function of the hand alone (watch and skip get the same bad beat).
	{
		ss::Rng R("auto-allin");
		int Revealed = 0;
		bool Same = true;
		bool Bounded = true;
		for (int K = 0; K < 60 && Revealed < 6; ++K)
		{
			ss::Hand H = AutoCraftedHand(R);
			while (!H.bComplete)
			{
				const ss::LegalActions L = H.GetLegalActions();
				H.Act(L.CanRaise ? ss::PlayerAction::RaiseTo(static_cast<double>(L.MaxRaiseTo)) : L.CallAmount > 0 ? ss::PlayerAction::Call() : ss::PlayerAction::Check());
			}
			const double E1 = autoplay::AllInEquity(H, 0);
			const double E2 = autoplay::AllInEquity(H, 0);
			Same = Same && E1 == E2;
			if (E1 >= 0.0)
			{
				++Revealed;
				Bounded = Bounded && E1 <= 1.0;
				const autoplay::HandOutcome O = autoplay::OutcomeOf(H, 0, true);
				Bounded = Bounded && O.AllIn && O.BadBeat == (!O.Won && E1 >= 0.6);
			}
		}
		Expect(Revealed >= 3 && Same && Bounded, "autoplay: the all-in equity is deterministic and the outcome reads it (bad beat at 60%)");
	}

	// The played record, the notes' keys.
	{
		autoplay::Book B;
		autoplay::Run R;
		R.AiDecisions = 214;
		R.OwnDecisions = 46;
		const autoplay::TableAuto A = AutoPlanOf(style::Style::Lag, autoplay::Mode::Watch);
		for (int K = 0; K < 205; ++K)
		{
			autoplay::Record(B, "ev-" + std::to_string(K), 0, 0, A, R);
		}
		autoplay::Record(B, "ev-204", 1, 0, A, R);
		Expect(B.Results.size() == 200 && B.Results[0].Id == "ev-204" && B.Results[0].Venue == 1 && B.Results.back().Id == "ev-5" && autoplay::PlayedFor(B, "ev-100") &&
				   !autoplay::PlayedFor(B, "ev-0"),
			"autoplay: played lines, newest first, the last 200, one per event");
		Expect(autoplay::PlayedLine(B.Results[0]) == "Played by your plan: 214 of 260 decisions (LAG)" && autoplay::CareerTag(B.Results[0]) == "AUTO \xC2\xB7 LAG 82%",
			"autoplay: the results line and Career's AUTO tag");
		ss::TournamentSpec Spec;
		Spec.Id = "notes";
		Spec.Entrants = 18;
		ss::Tournament T(Spec, "Hero", "auto-notes", {{"Mei", ss::Archetype::Reg}});
		Expect(autoplay::NoteKey(&T, "npc:Mei") == "npc:Mei" && autoplay::NoteKey(&T, "p3") == "npc:" + T.Players[static_cast<size_t>(T.PlayerIndex("p3"))].Name &&
				   autoplay::NoteKey(nullptr, "Big Lou") == "cast:Big Lou" && autoplay::NoteKey(&T, ss::HeroId).empty(),
			"autoplay: the notes' keys (npc:<name> at a tournament, cast:<name> at Dee's, never the player)");

		{
			ss::reads::Table Event;
			ss::reads::Stats S;
			S.Hands = 40;
			S.Vpip = 12;
			Event["npc:Mei"] = S;
			Event["p7"] = S;
			autoplay::FoldNotes(B, Event, 100.0);
			if (B.Notes.empty())
			{
				std::printf("  autoplay: notes not checked (reads::Fold folds nothing yet: core-styles)\n");
			}
			else
			{
				Expect(B.Notes.count("npc:Mei") == 1 && B.Notes.count("p7") == 0 && B.NoteSeen.at("npc:Mei") == 100.0, "autoplay: only the world's people make notes");
				for (int K = 0; K < 310; ++K)
				{
					ss::reads::Table One;
					One["npc:n" + std::to_string(K)] = S;
					autoplay::FoldNotes(B, One, 200.0 + K);
				}
				Expect(B.Notes.size() == 300 && B.Notes.count("npc:Mei") == 0 && B.Notes.count("npc:n309") == 1 && B.NoteSeen.size() == 300, "autoplay: at most 300 notes, the longest unseen dropped first");
				Expect(autoplay::NoteFor(&B.Notes, nullptr, "npc:n309") != nullptr && autoplay::NoteFor(&B.Notes, nullptr, "npc:nobody") == nullptr, "autoplay: NoteFor");
			}
		}
	}

	// The auto host line: the plan and the run, exactly.
	{
		autoplay::TableAuto A = AutoPlanOf(style::Style::IcmShark, autoplay::Mode::Skip, 4, autoplay::Target::FinalTable, 0, autoplay::StopBubble | autoplay::StopPremium);
		autoplay::Run R;
		R.Tilt = 0.417;
		R.Sharp = 0.93;
		R.AiDecisions = 311;
		R.OwnDecisions = 7;
		R.AiHands = 140;
		R.OwnHands = 3;
		const std::string Text = autoplay::RunText(A, R);
		autoplay::TableAuto A2;
		autoplay::Run R2;
		Expect(Text == "auto\ticm\t2\t4\t417\t93\t2\t18\t311\t7\t140\t3" && autoplay::ReadRun(Text, A2, R2) && autoplay::RunText(A2, R2) == Text && R2.Tilt == R.Tilt &&
				   R2.Sharp == R.Sharp,
			"autoplay: the auto host line (12 fields) reads back exactly");
		Expect(!autoplay::ReadRun("auto\tgto\t1", A2, R2), "autoplay: a short auto line is refused");
	}

	// The save lines and their migration (§6.4, §6.5).
	{
		ss::SaveData Old;
		Expect(ss::SaveData::Parse("shortstack.nightone.v2\nbankroll\t500\nresult\tNight Owl Turbo\t12\t1000\t300\t80.000\nresult\tSprinted\t40\t180\t0\t0.000\nresult\tDaily Grind\t3\t700\t9000\t90.000\n", Old),
			"autoplay: an old save parses");
		Expect(std::fabs(Old.Life.Auto.Pref.Form - 85.0) < 1e-9 && Old.Life.Auto.Pref.S == style::Style::Autopilot && Old.Life.Auto.Pref.Stops == (autoplay::StopFinalTable | autoplay::StopBubble),
			"autoplay: no pref line: Autopilot, and Form the mean accuracy of the results that have one (80 and 90: 85)");
		const std::string Back = Old.Serialize();
		ss::SaveData Again;
		Expect(Back.find("autoplay\t") == std::string::npos && ss::SaveData::Parse(Back, Again) && Again.Life.Auto.Pref.Form == Old.Life.Auto.Pref.Form,
			"autoplay: an untouched old save writes no autoplay line, and reloads to the same Form");
		ss::life::State& L = Again.Life;
		L.Update2LinesRead |= ss::life::U2AutoPref;
		L.Auto.Pref.S = style::Style::Gto;
		L.Auto.Pref.Speed = 4;
		L.Auto.Pref.To = autoplay::Target::FinalTable;
		L.Auto.Pref.Stops = autoplay::StopBubble | autoplay::StopMyAllIn;
		L.Auto.Pref.Form = 88.123;
		L.Auto.Pref.FormWeek = 4;
		L.Auto.Pref.WeekDecisions = 61;
		autoplay::Run R;
		R.AiDecisions = 9;
		autoplay::Record(L.Auto, "night-owl@1560", 0, 0, AutoPlanOf(style::Style::Gto, autoplay::Mode::Watch), R);
		autoplay::Record(L.Auto, "weird\tid", 1, 0, AutoPlanOf(style::Style::Nit, autoplay::Mode::Watch), R);
		ss::reads::Stats S;
		S.Hands = 31;
		S.Pfr = 4;
		L.Auto.Notes["npc:Mei"] = S;
		L.Auto.NoteSeen["npc:Mei"] = 12345.0;
		L.Update2LinesRead |= ss::life::U2Notes | ss::life::U2AutoPlayed;
		const std::string Text = Again.Serialize();
		ss::SaveData Read;
		Expect(ss::SaveData::Parse(Text, Read) && Read.Serialize() == Text, "autoplay: Serialize(Parse(S)) == S with the pref, played and note lines");
		const autoplay::Prefs& P = Read.Life.Auto.Pref;
		Expect(P.S == style::Style::Gto && P.Speed == 4 && P.To == autoplay::Target::FinalTable && P.Stops == (autoplay::StopBubble | autoplay::StopMyAllIn) && P.Form == 88.123 &&
				   P.FormWeek == 4 && P.WeekDecisions == 61,
			"autoplay: the pref line round-trips");
		Expect(Read.Life.Auto.Results.size() == 2 && Read.Life.Auto.Results[0].Id == "weird\tid" && Read.Life.Auto.Results[1].S == style::Style::Gto &&
				   Read.Life.Auto.Results[1].AiDecisions == 9,
			"autoplay: the played lines round-trip (ids escaped)");
		Expect(Read.Life.Auto.Notes.count("npc:Mei") && Read.Life.Auto.Notes.at("npc:Mei").Pfr == 4 && Read.Life.Auto.NoteSeen.at("npc:Mei") == 12345.0,
			"autoplay: the note lines round-trip");
	}

	// The copy (system UI; linted with labels read as sentences).
	{
		std::vector<std::pair<std::string, std::string>> Lines;
		int Ai = 0;
		for (const std::string& Line : autoplay::Inventory())
		{
			const char Last = Line.empty() ? ' ' : Line.back();
			const bool Sentence = Last == '.' || Last == '!' || Last == '?' || Last == '"' || Last == '\'' || copylint::EndsWith(Line, "\xE2\x80\x9D");
			Lines.push_back({"autoplay", Sentence ? Line : Line + "."});
			Ai += (" " + Line + " ").find(" AI ") != std::string::npos ? 1 : 0; // the word, not "RAISED"
		}
		Failures += copylint::Run("autoplay", Lines, copylint::Pa);
		if (Ai != 1)
		{
			for (const auto& L : Lines)
			{
				if ((" " + L.second + " ").find(" AI ") != std::string::npos)
				{
					std::printf("  autoplay: says AI: %s\n", L.second.c_str());
				}
			}
		}
		std::printf("  autoplay: %d inventory lines\n", static_cast<int>(Lines.size()));
		Expect(Lines.size() > 80 && Ai == 1, "autoplay: the inventory is linted, and only the panel's title says AI (system UI, U1)");
		Expect(autoplay::ThinkingLine(style::Style::Gto) == "GTO is thinking\xE2\x80\xA6" && autoplay::StatusLine(AutoPlanOf(style::Style::Gto, autoplay::Mode::Watch), 0.93) ==
															   "AUTOPLAY \xC2\xB7 GTO \xC2\xB7 Sharpness 93% \xC2\xB7 [K] take over",
			"autoplay: the strip's think line names no move; the P0a status line");
	}
}

// ------------------------------------------------------------------ a whole event played by the plan

void AutoNoGrading()
{
	const auto T0 = std::chrono::steady_clock::now();
	Hooks H;
	ss::Session S(H, "auto-nograde");
	S.CurrentScreen = ss::Screen::Lobby;
	S.BankrollCents = 10000;
	ss::net::EventInstance Hyper;
	Expect(ss::net::Shared().FindInstance("hyper-sprint@1590", Hyper), "autoplay: the 2:30 hyper is scheduled");
	S.RegisterEvent(ss::net::Shared().Listing(Hyper));
	const double FormBefore = S.Life.Auto.Pref.Form;
	const ss::coach::Notes StudyBefore = S.Life.Study;
	Expect(S.SetAutoplay(0, AutoPlanOf(style::Style::Lag, autoplay::Mode::Watch, 8)).empty(), "autoplay: LAG takes the seat");
	const autoplay::Panel On = S.AutoPanel(0);
	Expect(On.On && On.Now.S == style::Style::Lag && On.Sharp == 0.80 && On.Status == "AUTO \xC2\xB7 LAG 8x" && On.Styles.size() == static_cast<size_t>(style::Style::Count) &&
			   On.Greyed.size() == On.Styles.size(),
		"autoplay: the panel (held Sharpness, the tile's line, a tile per style)");
	double Now = 0.0;
	int Prompts = 0;
	for (int F = 0; F < 200000 && S.HandsPlayed < 25 && !S.Finishing(); ++F)
	{
		Now += 0.1;
		S.Update(Now);
		Prompts += S.HasPrompt ? 1 : 0;
	}
	Expect(S.SkipAhead(0, autoplay::Target::Result).empty(), "autoplay: then skip to the result");
	S.SprintBudgetMs = 1.0e9;
	for (int F = 0; F < 20000 && S.CurrentScreen == ss::Screen::Table; ++F)
	{
		Now += 0.1;
		S.Update(Now);
		Prompts += S.HasPrompt ? 1 : 0;
	}
	Expect(S.CurrentScreen == ss::Screen::Results, "autoplay: the plan plays the event to its result");
	Expect(Prompts == 0 && S.LastResults.Grades.empty() && S.LastResults.AccuracyPct == 0.0 && !S.History.empty() && S.History[0].AccuracyPct == 0.0,
		"autoplay: no AI decision is graded (assertion 11): no prompt, no grade, accuracy 0");
	bool Untouched = S.Life.Study.Sessions == StudyBefore.Sessions;
	for (size_t K = 0; K < S.Life.Study.All.size(); ++K)
	{
		Untouched = Untouched && S.Life.Study.All[K].Decisions == StudyBefore.All[K].Decisions && S.Life.Study.All[K].LossMilliBb == StudyBefore.All[K].LossMilliBb;
	}
	Expect(Untouched && S.Life.Auto.Pref.Form == FormBefore, "autoplay: the coach's tallies and Form untouched by the plan");
	const autoplay::Played* P = autoplay::PlayedFor(S.Life.Auto, Hyper.Id);
	Expect(P && P->S == style::Style::Lag && P->AiDecisions > 0 && P->OwnDecisions == 0 && P->AiHands > 0 && P->OwnHands == 0,
		"autoplay: the played line (keyed by the event id: the plan's decisions and hands, none of yours)");
	Expect(P && autoplay::CareerTag(*P) == "AUTO \xC2\xB7 LAG 100%", "autoplay: Career's row reads AUTO, and its accuracy has no decision of yours (the \xE2\x80\x94)");
	std::printf("  autoplay: %s, %d hands (%d by the plan, %d decisions); biggest pot %s; %.1f s\n", ss::Ordinal(S.LastResults.Place).c_str(), S.LastResults.Hands,
		P ? P->AiHands : 0, P ? P->AiDecisions : 0, ss::ChipsText(static_cast<double>(S.LastResults.BiggestPot)).c_str(), AutoSecondsSince(T0));
	Expect(S.LastResults.BiggestPot > 0 || S.LastResults.Hands < 20, "autoplay: the skip keeps the biggest pot won (bug 1's BiggestPot)");
	// What the player's tables saw of the world's people becomes notes for the next event (npc:<name>), and only theirs.
	int Met = 0;
	for (const auto& It : S.FieldNpc)
	{
		Met += S.Life.Auto.Notes.count(ss::autoplay::NoteKey(S.T.get(), It.first)) ? 1 : 0;
	}
	bool Named = true;
	for (const auto& It : S.Life.Auto.Notes)
	{
		Named = Named && It.first.rfind("npc:", 0) == 0 && It.second.Hands > 0;
	}
	std::printf("  autoplay: notes on %d people after the event (%d of the field's %d world people)\n", static_cast<int>(S.Life.Auto.Notes.size()), Met, static_cast<int>(S.FieldNpc.size()));
	Expect(Named && !S.Life.Auto.Notes.empty() && Met > 0 && (S.Life.Update2LinesRead & ss::life::U2Notes), "autoplay: the event's observed world people fold into the notes");
	ss::SaveData Back;
	Expect(ss::SaveData::Parse(H.Last.Serialize(), Back) && autoplay::PlayedFor(Back.Life.Auto, Hyper.Id) && H.Last.Serialize().find("autoplay\tpref\ttag") == std::string::npos &&
			   H.Last.Serialize().find("autoplay\tpref\t") != std::string::npos,
		"autoplay: the played line and the pref line are saved");
}

void AutoFieldPots()
{
	// Bug 1: a big pot against one of the world's people reaches the world (FieldPots, the biggest with each person), on
	// the post-hand hook both paths run. A real hand: the player and a regular all in for 20 big blinds each.
	Hooks H;
	ss::Session S(H, "auto-pots");
	AutoSeat(S);
	S.FieldNpc["regular-7"] = 4242;
	ss::HandConfig Cfg;
	for (int K = 0; K < 3; ++K)
	{
		ss::SeatInput In;
		In.Seat = K;
		In.Id = K == 0 ? std::string(ss::HeroId) : K == 1 ? std::string("regular-7") : std::string("stranger");
		In.Stack = 2000;
		Cfg.Players.push_back(In);
	}
	Cfg.ButtonSeat = 2;
	Cfg.SmallBlind = 50;
	Cfg.BigBlind = 100;
	int Tested = 0;
	ss::Rng R("auto-pots");
	for (int Try = 0; Try < 40 && Tested < 4; ++Try)
	{
		ss::Hand Hd(Cfg, R);
		while (!Hd.bComplete)
		{
			const std::string& Id = Hd.Seats[static_cast<size_t>(Hd.ToAct)].Id;
			const ss::LegalActions L = Hd.GetLegalActions();
			Hd.Act(Id == "stranger" ? ss::PlayerAction::Fold() : L.CanRaise ? ss::PlayerAction::RaiseTo(static_cast<double>(L.MaxRaiseTo)) : ss::PlayerAction::Call());
		}
		const ss::HandSeat& Me = Hd.Seats[0];
		if (Me.Stack == Me.StartStack)
		{
			continue; // a split: nothing to remember
		}
		S.FieldPots.clear();
		ss::Session::HandFacts F;
		F.ByPlan = true;
		S.HeroHandDone(Hd, F);
		const bool Remembered = S.FieldPots.size() == 1 && S.FieldPots[0].first == 4242 && S.FieldPots[0].second == Me.Stack - Me.StartStack;
		Expect(Remembered, "autoplay: a big pot with one of the world's people reaches the world, the player's net (bug 1)");
		Expect(Me.Stack < Me.StartStack || S.BiggestPot >= 4000, "autoplay: the biggest pot the player won is kept");
		++Tested;
	}
	Expect(Tested >= 2, "autoplay: the big-pot hands were played");
}

// ------------------------------------------------------------------ watch == skip

struct AutoParity
{
	std::map<int, std::string> Hands;
	std::string Checkpoint, Run;
	int Ticks = 0;
	bool Ended = false;
	std::vector<std::string> HandOffs; // tick, action index, the engine checkpoint at each stop rule's hand-off
};

/** The hyper watched at 8x until hand Ticks + 1 is dealt (or the night ends); Stops: the decision-level rules. */
AutoParity AutoWatched(const std::string& Seed, style::Style S, int Ticks, uint32_t Stops)
{
	AutoParity Out;
	Hooks H;
	ss::Session A(H, Seed);
	AutoSeat(A);
	A.SetAutoplay(0, AutoPlanOf(S, autoplay::Mode::Watch, 8, autoplay::Target::Result, 0, Stops));
	double Now = 0.0;
	for (int F = 0; F < 400000 && A.HandsPlayed < Ticks + 1 && !A.Finishing(); ++F)
	{
		Now += 0.1;
		A.Update(Now);
		AutoNoteTick(A, Out.Hands);
		if (A.HasPrompt && A.CurHand)
		{
			Out.HandOffs.push_back(std::to_string(A.T->Tick) + "/" + std::to_string(A.CurHand->History.size()) + "/" + A.T->Checkpoint());
			A.HeroAct(ss::PlayerAction::Fold());
		}
	}
	Out.Ended = A.Finishing();
	Out.Ticks = A.T->Tick;
	Out.Checkpoint = A.T->Checkpoint();
	Out.Run = AutoRunCore(A);
	return Out;
}

/** The same hyper: hand 1 watched, then skipped tick by tick (a budget of 0) through Ticks hands in all. */
AutoParity AutoSkipped(const std::string& Seed, style::Style S, int Ticks, uint32_t Stops)
{
	AutoParity Out;
	Hooks H;
	ss::Session B(H, Seed);
	AutoSeat(B);
	B.SprintBudgetMs = 0.0;
	B.SetAutoplay(0, AutoPlanOf(S, autoplay::Mode::Skip, 8, autoplay::Target::Hands, Ticks - 1, Stops));
	double Now = 0.0;
	for (int F = 0; F < 400000 && !B.Finishing(); ++F)
	{
		Now += 0.1;
		B.Update(Now);
		AutoNoteTick(B, Out.Hands);
		if (B.HasPrompt && B.CurHand)
		{
			// A stop rule paused the skip: the hand replayed to the decision (AdoptHand). Answered as the watcher did, then
			// the skip goes on from the next hand for what's left.
			Out.HandOffs.push_back(std::to_string(B.T->Tick) + "/" + std::to_string(B.CurHand->History.size()) + "/" + B.T->Checkpoint());
			B.HeroAct(ss::PlayerAction::Fold());
			const int Left = Ticks - B.T->Tick - 1;
			if (Left > 0)
			{
				B.SkipAhead(0, autoplay::Target::Hands, Left);
			}
		}
		const autoplay::Panel P = B.AutoPanel(0);
		if (!P.Skipping && P.On && B.HandsPlayed >= Ticks + 1 && B.T->Tick >= Ticks)
		{
			break; // the skip's stop dealt hand Ticks + 1
		}
	}
	Out.Ended = B.Finishing();
	Out.Ticks = B.T->Tick;
	Out.Checkpoint = B.T->Checkpoint();
	Out.Run = AutoRunCore(B);
	return Out;
}

void AutoWatchEqualsSkip()
{
	const auto T0 = std::chrono::steady_clock::now();
	for (const style::Info& I : style::Catalog())
	{
		if (I.Hidden)
		{
			continue;
		}
		const std::string Seed = std::string("auto-parity-") + I.Id;
		const int Ticks = I.S == style::Style::Autopilot || I.S == style::Style::Gto ? 150 : 60;
		const AutoParity W = AutoWatched(Seed, I.S, Ticks, 0);
		const AutoParity K = AutoSkipped(Seed, I.S, Ticks, 0);
		int Compared = 0;
		const int First = AutoDiffTicks(W.Hands, K.Hands, Ticks, Compared);
		if (First >= 0)
		{
			std::printf("  autoplay %s: tick %d differs\n    watched: %s\n    skipped: %s\n", I.Id, First, W.Hands.at(First).c_str(), K.Hands.at(First).c_str());
		}
		const bool Ends = W.Ended == K.Ended && W.Ticks == K.Ticks;
		Expect(First < 0 && Compared >= std::min(Ticks, W.Ticks) - 1 && Ends, (std::string("autoplay: watched and skipped play the same hand at every tick (") + I.Id + ")").c_str());
		Expect(Ends && W.Checkpoint == K.Checkpoint && W.Run == K.Run, (std::string("autoplay: and the engine checkpoint and the run match after them (") + I.Id + ")").c_str());
		if (W.Run != K.Run)
		{
			std::printf("    watched run: %s\n    skipped run: %s\n", W.Run.substr(0, 200).c_str(), K.Run.substr(0, 200).c_str());
		}
	}
	std::printf("  autoplay: watch == skip over every style, %.1f s\n", AutoSecondsSince(T0));
}

void AutoWithStops()
{
	const auto T0 = std::chrono::steady_clock::now();
	const uint32_t Stops = autoplay::StopMyAllIn | autoplay::StopBigPot | autoplay::StopPremium;
	int HandOffs = 0;
	for (const style::Style S : {style::Style::Lag, style::Style::Tag})
	{
		const std::string Seed = std::string("auto-stops-") + style::InfoOf(S).Id;
		const AutoParity W = AutoWatched(Seed, S, 100, Stops);
		const AutoParity K = AutoSkipped(Seed, S, 100, Stops);
		int Compared = 0;
		const int First = AutoDiffTicks(W.Hands, K.Hands, 100, Compared);
		Expect(First < 0 && Compared > 40, (std::string("autoplay: with every stop rule on, the same hands (") + style::InfoOf(S).Id + ")").c_str());
		Expect(W.HandOffs == K.HandOffs, (std::string("autoplay: the hand-offs at the same tick, action and T->R state (") + style::InfoOf(S).Id + ")").c_str());
		HandOffs += static_cast<int>(W.HandOffs.size());
		if (W.HandOffs != K.HandOffs)
		{
			std::printf("  autoplay: %d hand-offs watched, %d skipped\n", static_cast<int>(W.HandOffs.size()), static_cast<int>(K.HandOffs.size()));
		}
	}
	Expect(HandOffs >= 3, "autoplay: the stop rules handed decisions over (AdoptHand replayed the skip's paused hands)");
	std::printf("  autoplay: %d hand-offs with the stops on, %.1f s\n", HandOffs, AutoSecondsSince(T0));
}

void AutoSprintParity()
{
	// Today's Sprint (autoplay off, the pace at Sprint) against the plan's Autopilot skip from the same hand: the same ticks
	// to the bubble's approach, where both stop and deal the next hand.
	const auto T0 = std::chrono::steady_clock::now();
	AutoParity Legacy;
	AutoParity Plan;
	for (int Which = 0; Which < 2; ++Which)
	{
		AutoParity& Out = Which == 0 ? Legacy : Plan;
		Hooks H;
		ss::Session S(H, "auto-sprint-parity");
		AutoSeat(S);
		S.SprintBudgetMs = 0.0;
		S.SetAutoplay(0, AutoPlanOf(style::Style::Autopilot, autoplay::Mode::Watch, 8, autoplay::Target::Result, 0, autoplay::StopFinalTable | autoplay::StopBubble));
		double Now = 0.0;
		bool Switched = false;
		for (int F = 0; F < 400000 && !S.Finishing(); ++F)
		{
			Now += 0.1;
			S.Update(Now);
			AutoNoteTick(S, Out.Hands);
			if (!Switched && S.T->LastHeroHand())
			{
				Switched = true;
				if (Which == 0)
				{
					S.SetAutoplay(0, AutoPlanOf(style::Style::Autopilot, autoplay::Mode::Off));
					S.CurrentPace = ss::Pace::Sprint;
				}
				else
				{
					S.SkipAhead(0, autoplay::Target::Result);
				}
			}
			if (Switched && !S.Sprinting && S.CurrentPace != ss::Pace::Sprint && S.T->Tick > 2)
			{
				break; // the skip stopped (and dealt the next hand)
			}
		}
		Out.Ended = S.Finishing();
		Out.Ticks = S.T->Tick;
		Out.Checkpoint = S.T->Checkpoint();
		Out.Run = S.SprintStopReason;
	}
	int Compared = 0;
	const int First = AutoDiffTicks(Legacy.Hands, Plan.Hands, 1 << 30, Compared);
	Expect(First < 0 && Compared > 10 && Legacy.Ticks == Plan.Ticks && Legacy.Checkpoint == Plan.Checkpoint,
		"autoplay: Autopilot's skip plays today's Sprint, tick for tick, to the same stop");
	std::printf("  autoplay: Sprint and Autopilot's skip stopped at tick %d (%s / %s), %.1f s\n", Plan.Ticks, Legacy.Run.c_str(), Plan.Run.c_str(), AutoSecondsSince(T0));
}

// ------------------------------------------------------------------ take-over

void AutoTakeover()
{
	const auto T0 = std::chrono::steady_clock::now();
	const int K = 6;
	std::map<int, std::string> Plain;
	{
		Hooks H;
		ss::Session A(H, "auto-takeover");
		AutoSeat(A);
		A.CurrentPace = ss::Pace::Full; // a hand taken over isn't Smart-folded for the player in Full
		A.SetAutoplay(0, AutoPlanOf(style::Style::Tag, autoplay::Mode::Watch, 2));
		double Now = 0.0;
		for (int F = 0; F < 200000 && A.T->Tick < K + 2 && !A.Finishing(); ++F)
		{
			Now += 0.1;
			A.Update(Now);
			AutoNoteTick(A, Plain);
		}
	}
	Hooks H;
	ss::Session B(H, "auto-takeover");
	AutoSeat(B);
	B.CurrentPace = ss::Pace::Full;
	B.SetAutoplay(0, AutoPlanOf(style::Style::Tag, autoplay::Mode::Watch, 2));
	std::map<int, std::string> Taken;
	double Now = 0.0;
	bool MidThink = false;
	bool Prompted = false;
	double FormAtTake = 0.0;
	int WeekAtTake = 0;
	bool Window = false;
	bool Off = false;
	for (int F = 0; F < 200000 && !B.Finishing() && !Off; ++F)
	{
		Now += 0.05;
		B.Update(Now);
		AutoNoteTick(B, Taken);
		const autoplay::Panel P = B.AutoPanel(0);
		if (!MidThink && B.T->Tick >= K && P.Thinking)
		{
			// The plan is thinking at the player's turn in hand K: K takes the wheel; the prompt opens at once.
			MidThink = true;
			FormAtTake = B.Life.Auto.Pref.Form;
			WeekAtTake = B.Life.Auto.Pref.WeekDecisions;
			Expect(B.TakeOver(0).empty() && B.AutoPanel(0).TakenOver && !B.AutoPanel(0).Thinking, "autoplay: K during the plan's think takes the wheel");
			continue;
		}
		if (MidThink && B.HasPrompt && !Prompted)
		{
			Prompted = true;
			B.HeroAct(ss::PlayerAction::Fold());
			Expect(B.Life.Auto.Pref.Form == FormAtTake && B.Life.Auto.Pref.WeekDecisions == WeekAtTake + 1,
				"autoplay: a decision in a hand taken over mid-think is graded (it counts for the week) but doesn't move Form");
			continue;
		}
		if (Prompted && !Window && P.ResumeWindow)
		{
			Window = true;
			Expect(!P.TakenOver && P.On && std::string(autoplay::Text(autoplay::Words::Resumes)).find("AUTO RESUMES") == 0, "autoplay: the next hand is the plan's, with the resume window");
			Expect(B.TakeOver(0).empty() && B.Autoplay(0).M == autoplay::Mode::Off, "autoplay: K inside the window turns autoplay off");
			Off = true;
		}
	}
	Expect(MidThink && Prompted && Window, "autoplay: the take-over ran (think, prompt, window)");
	int Compared = 0;
	const int First = AutoDiffTicks(Plain, Taken, K, Compared);
	Expect(First < 0 && Compared >= K - 1, "autoplay: a take-over at hand k leaves hands 0 to k-1 as they were (TakeoverPrefix)");
	std::printf("  autoplay: take-over at tick %d, %.1f s\n", K, AutoSecondsSince(T0));
}

void AutoFormRoundTrip()
{
	// A career with graded decisions and no panel use (§6.5's test): the first graded decision keeps the pref line, and
	// Form comes back from the save to 1e-9.
	Hooks H;
	ss::Session S(H, "auto-form");
	AutoSeat(S);
	Expect((S.Life.Update2LinesRead & ss::life::U2AutoPref) == 0, "autoplay: a session from no save has no pref line yet");
	double Now = 0.0;
	ss::Rng Choice("auto-form-choices");
	int Decisions = 0;
	for (int F = 0; F < 100000 && Decisions < 12 && !S.Finishing(); ++F)
	{
		Now += 0.1;
		S.Update(Now);
		if (S.HasPrompt && Now - S.Prompt.OpenedAt > 0.3)
		{
			++Decisions;
			S.HeroAct(Choice.Chance(0.5) ? ss::PlayerAction::Fold() : (S.Prompt.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Call()));
		}
	}
	Expect(Decisions >= 5 && (S.Life.Update2LinesRead & ss::life::U2AutoPref) && S.Life.Auto.Pref.Form != 70.0, "autoplay: the player's own decisions move Form");
	S.Save();
	ss::SaveData Read;
	Expect(ss::SaveData::Parse(H.Last.Serialize(), Read) && std::fabs(Read.Life.Auto.Pref.Form - S.Life.Auto.Pref.Form) < 1e-9 && Read.Serialize() == H.Last.Serialize(),
		"autoplay: Form survives save and load to 1e-9 (no panel use)");
}

void AutoParityCommand()
{
	// ss.Auto.Parity (§2.12, at home): N hands watched through the session's real path at 8x, then the same hands headless
	// from the checkpoint before them; both hashes printed. The table carries on from the watched hands, on the host's clock.
	Hooks H;
	ss::Session S(H, "auto-parity-command");
	AutoSeat(S);
	S.SetAutoplay(0, AutoPlanOf(style::Style::Gto, autoplay::Mode::Watch, 2));
	double Now = 0.0;
	for (int F = 0; F < 50; ++F)
	{
		Now += 0.1;
		S.Update(Now);
	}
	const int Before = S.HandsPlayed;
	const std::string Answer = S.DebugAuto("parity 20");
	std::printf("  autoplay: %s\n", Answer.c_str());
	Expect(Answer.find(": equal.") != std::string::npos && S.HandsPlayed >= Before + 20 && S.Now == Now, "autoplay: ss.Auto.Parity's two hashes are equal, and the clock is the host's");
	for (int F = 0; F < 300 && !S.Finishing(); ++F)
	{
		Now += 0.1;
		S.Update(Now);
	}
	Expect(S.HandsPlayed > Before + 21 || S.Finishing(), "autoplay: and the table plays on after it");
}

void AutoKastSpeed()
{
	Hooks H;
	ss::Session S(H, "auto-kast");
	AutoSeat(S);
	const ss::kast::Inputs Off = S.StreamInputs();
	S.SetAutoplay(0, AutoPlanOf(style::Style::Gto, autoplay::Mode::Watch, 4));
	const ss::kast::Inputs Four = S.StreamInputs();
	S.SetAutoplay(0, AutoPlanOf(style::Style::Gto, autoplay::Mode::Watch, 8));
	const ss::kast::Inputs Eight = S.StreamInputs();
	S.SetAutoplay(0, AutoPlanOf(style::Style::Gto, autoplay::Mode::Watch, 2));
	const ss::kast::Inputs Two = S.StreamInputs();
	Expect(Off.WatchSpeed == 1 && !Off.Sprinting && Two.WatchSpeed == 2 && !Two.Sprinting && Four.WatchSpeed == 4 && !Four.Sprinting && Eight.WatchSpeed == 8 && Eight.Sprinting,
		"autoplay: the stream sees the watch speed (4x its own factor; 8x as a skip)");
	S.SprintBudgetMs = 0.0;
	S.SkipAhead(0, autoplay::Target::Result);
	double Now = 0.0;
	bool Skipping = false;
	for (int F = 0; F < 20000 && !Skipping && !S.Finishing(); ++F)
	{
		Now += 0.1;
		S.Update(Now);
		Skipping = S.Sprinting && S.StreamInputs().Sprinting;
	}
	Expect(Skipping, "autoplay: a skip is Sprinting to the stream");
}

void AutoSharpness()
{
	// Sharpness is read from the player's Form and condition when the plan takes the seat, then held (§2.5); off, the panel
	// shows the career's default plan and that's the plan that takes the seat; and a Form an older save's results gave
	// (undated) starts its weekly clock when the plan first plays, so a career that only autoplays still rusts.
	Hooks H;
	ss::Session S(H, "auto-sharp");
	AutoSeat(S);
	S.Life.Energy = 80.0;
	S.Life.Hunger = 10.0;
	S.Life.Thirst = 10.0;
	S.HeroTilt = 0.0;
	S.Life.Auto.Pref.Form = 88.0;
	S.Life.Auto.Pref.FormWeek = -1;
	S.Life.Auto.Pref.S = style::Style::Gto;
	S.Life.Auto.Pref.Speed = 4;
	const double Rested = autoplay::SharpnessOf(88.0, 0.0, false, false);
	const autoplay::Panel Off = S.AutoPanel(0);
	Expect(!Off.On && Off.Now.S == style::Style::Gto && Off.Now.Speed == 4 && Off.Sharp == Rested, "autoplay: off, the panel shows the career's default plan and the Sharpness it would get");
	S.DebugAuto("on");
	Expect(S.Autoplay(0).M == autoplay::Mode::Watch && S.Autoplay(0).S == style::Style::Gto && S.Autoplay(0).Speed == 4, "autoplay: turned on, the table takes the plan the panel showed");
	const double Held = S.Sharpness(0);
	Expect(Held == Rested && Held > 0.80, "autoplay: Sharpness is read from Form as the plan takes the seat (Form 88: above the default 0.80)");
	S.Life.Energy = 5.0;
	S.Life.Hunger = 99.0;
	Expect(S.Sharpness(0) == Held && S.AutoPanel(0).Sharp == Held, "autoplay: and held for the run while the player tires");
	autoplay::TableAuto A = S.Autoplay(0);
	A.M = autoplay::Mode::Off;
	S.SetAutoplay(0, A);
	A.M = autoplay::Mode::Watch;
	S.SetAutoplay(0, A);
	Expect(S.Sharpness(0) == autoplay::SharpnessOf(88.0, 0.0, true, true) && S.Sharpness(0) < Held, "autoplay: off and on again reads it afresh (tired and running on empty)");
	Expect(S.Life.Auto.Pref.FormWeek == autoplay::WeekOf(S.WorldMinutes()) && (S.Life.Update2LinesRead & ss::life::U2AutoPref), "autoplay: an undated Form is dated (and kept) when the plan first plays");
	bool Rusty = false;
	const double Later = autoplay::FormNow(S.Life.Auto, S.WorldMinutes() + 3.0 * 7.0 * 1440.0, &Rusty);
	Expect(Later == 85.0 && Rusty, "autoplay: three idle weeks on it's three points rustier: autoplaying alone never keeps an edge");
}

void AutoSkipStream()
{
	// Bug 2: on stream, a skip to the result fires the moments it used to drop and settles the stream's prediction, as the
	// watch path does at the bust or the win.
	Hooks H;
	ss::Session S(H, "auto-stream");
	S.CurrentScreen = ss::Screen::Lobby;
	S.BankrollCents = 300000;
	Expect(S.Buy(ss::gear::FirstPcUpgrade().Id).empty() && S.GoLive().empty() && S.Streaming(), "autoplay: live on Kast");
	ss::net::EventInstance Hyper;
	Expect(ss::net::Shared().FindInstance("hyper-sprint@1590", Hyper), "autoplay: the 2:30 hyper is scheduled");
	S.RegisterEvent(ss::net::Shared().Listing(Hyper));
	Expect(S.Stream.Pred.Active && !S.Stream.Pred.Resolved, "autoplay: registering on stream opens a prediction");
	S.SprintBudgetMs = 1.0e9;
	Expect(S.SetAutoplay(0, AutoPlanOf(style::Style::Tag, autoplay::Mode::Skip, 1, autoplay::Target::Result, 0, 0)).empty(), "autoplay: TAG skips to the result on stream");
	double Now = 0.0;
	for (int F = 0; F < 200000 && S.CurrentScreen == ss::Screen::Table; ++F)
	{
		Now += 0.1;
		S.Update(Now);
	}
	const ss::kast::Moment Last = S.Stream.LastMoment;
	const bool Ending = Last == ss::kast::Moment::Bust || Last == ss::kast::Moment::Cashed || Last == ss::kast::Moment::Win || Last == ss::kast::Moment::Knockout;
	Expect(S.CurrentScreen == ss::Screen::Results && S.Stream.Pred.Resolved && Ending, "autoplay: the skip's bust, cash or win reaches the stream and settles its prediction (bug 2)");
}

// ------------------------------------------------------------------ two tables

struct AutoTableLog
{
	std::map<int, std::string> Hands;
	std::string At41; // the engine checkpoint when hand 42 is dealt (41 ticks played)
	bool Gone = false;
};

void AutoOtherTables()
{
	// SharpnessIgnoresOtherTables (§2.10): the first table skipping and the second watched play as each does without the
	// other's mode. The skip runs the clock (and the energy) far ahead of the watched table; Sharpness was held from the
	// moment each run began, and a skip's batch never changes a result. Tables are told apart by their event.
	const auto T0 = std::chrono::steady_clock::now();
	const ss::LobbyEvent& Second = ss::Lobby()[0];
	const std::string FirstId = ss::Lobby()[1].Spec.Id;
	auto Play = [&](bool SecondTable, bool FirstSkips, double Budget, AutoTableLog& First, AutoTableLog& Other) {
		Hooks H;
		ss::Session S(H, "auto-tables");
		AutoSeat(S);
		S.SprintBudgetMs = Budget;
		if (SecondTable)
		{
			S.RegisterEvent(Second);
		}
		S.SetAutoplay(0, AutoPlanOf(style::Style::Reg, FirstSkips ? autoplay::Mode::Skip : autoplay::Mode::Watch, 8, autoplay::Target::Hands, 40));
		if (SecondTable)
		{
			S.SetAutoplay(1, AutoPlanOf(style::Style::Nit, autoplay::Mode::Watch, 8));
		}
		Other.Gone = !SecondTable;
		double Now = 0.0;
		for (int F = 0; F < 60000; ++F)
		{
			Now += 0.1;
			S.Update(Now);
			bool SawFirst = false;
			bool SawOther = false;
			for (int K = 0; K < S.TableCount(); ++K)
			{
				S.WithTable(K, [&]() {
					if (!S.T)
					{
						return;
					}
					const bool IsFirst = S.T->Spec.Id == FirstId;
					AutoTableLog& Log = IsFirst ? First : Other;
					(IsFirst ? SawFirst : SawOther) = true;
					AutoNoteTick(S, Log.Hands);
					if (Log.At41.empty() && S.T->Tick >= 41 && !S.Sprinting && S.CurrentPace != ss::Pace::Sprint)
					{
						Log.At41 = S.T->Tick == 41 ? S.T->Checkpoint() : std::string("past 41");
					}
				});
			}
			First.Gone = First.Gone || !SawFirst;
			Other.Gone = Other.Gone || !SawOther;
			const bool FirstDone = First.Gone || !First.At41.empty();
			const bool OtherDone = Other.Gone || (!Other.Hands.empty() && Other.Hands.rbegin()->first >= 40);
			if (FirstDone && OtherDone)
			{
				break;
			}
		}
	};
	AutoTableLog Both[2], Alone[2], Watched[2], Batch[2];
	Play(true, true, 0.0, Both[0], Both[1]);
	Play(false, true, 0.0, Alone[0], Alone[1]);
	Play(true, false, 0.0, Watched[0], Watched[1]);
	Play(true, true, 1.0e9, Batch[0], Batch[1]);
	int C1 = 0, C2 = 0, C3 = 0;
	auto Same = [](const std::map<int, std::string>& A, const std::map<int, std::string>& B, int& Compared, const char* What) {
		const int First = AutoDiffTicks(A, B, 40, Compared);
		if (First >= 0)
		{
			std::printf("  autoplay (%s): tick %d differs\n    %s\n    %s\n", What, First, A.at(First).c_str(), B.at(First).c_str());
		}
		return First < 0 && Compared > 15; // (the hero can bust a 1,000-runner field early: every common tick counts)
	};
	// The first table may bust before hand 42 (a plan at the default Sharpness slips, C1); then every path busts it at the
	// same hand, which the hand-by-hand comparison holds.
	const bool Reached = (!Both[0].At41.empty() && Both[0].At41 != "past 41") || (Both[0].At41.empty() && Both[0].Gone);
	Expect(Same(Both[0].Hands, Alone[0].Hands, C1, "alone") && Reached && Both[0].At41 == Alone[0].At41,
		"autoplay: a skipping table plays as it does alone (the other table watched)");
	Expect(Same(Both[1].Hands, Watched[1].Hands, C2, "watched"), "autoplay: a watched table plays the same whether the other skips or is watched");
	Expect(Same(Both[1].Hands, Batch[1].Hands, C3, "batch") && Both[0].At41 == Batch[0].At41, "autoplay: a skip's batch size changes nothing at either table");
	Expect(Reached && Watched[0].At41 == Both[0].At41, "autoplay: and the first table watched instead reaches the same hand 42");
	std::printf("  autoplay: two tables, %d/%d/%d ticks compared, %.1f s\n", C1, C2, C3, AutoSecondsSince(T0));
}

// ------------------------------------------------------------------ the live resume

struct AutoResumeSink : ss::live::SkipSink
{
	autoplay::Run* R = nullptr;
	style::HeroBrain* Brain = nullptr;
	const autoplay::TableAuto* Plan = nullptr;
	void HandDone(const ss::Hand& H, int HeroSeat) override
	{
		autoplay::NoteSkippedHand(*R, H, HeroSeat, 1.0);
		*Brain = autoplay::BrainFor(*Plan, *R, 1.0, 1, nullptr, nullptr);
	}
	void Events(const std::vector<ss::TEvent>&) override {}
	void Checkpoint(int) override {}
};

std::string AutoSeenText(const autoplay::Run& R)
{
	std::string Out;
	for (const auto& It : R.Seen)
	{
		Out += "autoseen\t" + It.first + "\t" + ss::reads::Write(It.second) + "\n";
	}
	return Out;
}

void AutoResume()
{
	// ResumeEqualsUninterrupted (§2.8): the live skip resumed from the engine checkpoint and the host text (the auto line,
	// an autoseen line a player) plays on exactly as the night that never stopped, for every style.
	const auto T0 = std::chrono::steady_clock::now();
	ss::TournamentSpec Spec;
	Spec.Id = "auto-resume";
	Spec.Name = "Hyper";
	Spec.BuyInCents = 330;
	Spec.FeeCents = 30;
	Spec.Entrants = 180;
	Spec.LevelMinutes = 3;
	const int Half = 40;
	for (const style::Info& I : style::Catalog())
	{
		if (I.Hidden)
		{
			continue;
		}
		const autoplay::TableAuto Plan = AutoPlanOf(I.S, autoplay::Mode::Skip, 1, autoplay::Target::Hands, 0, 0);
		auto Field = [&]() { return std::unique_ptr<ss::Tournament>(new ss::Tournament(Spec, "Hero", std::string("auto-resume-") + I.Id, {{"Mei", ss::Archetype::Reg}})); };
		auto SkipFor = [&](ss::Tournament& T, autoplay::Run& R, int Hands) {
			style::HeroBrain Brain = autoplay::BrainFor(Plan, R, 1.0, 1, nullptr, nullptr);
			AutoResumeSink Sink;
			Sink.R = &R;
			Sink.Brain = &Brain;
			Sink.Plan = &Plan;
			autoplay::TableAuto A = Plan;
			A.Hands = Hands;
			while (A.Hands > 0)
			{
				const ss::live::SkipOut O = ss::live::SkipSlice(T, Brain, A, 0.0, Sink);
				A.Hands -= O.Hands;
				if (O.Why != ss::live::SkipWhy::Budget && O.Why != ss::live::SkipWhy::Target)
				{
					break;
				}
				if (O.Ticks == 0)
				{
					break;
				}
			}
		};
		std::unique_ptr<ss::Tournament> Whole = Field();
		autoplay::Run RW;
		RW.Sharp = 0.71;
		RW.Tilt = 0.2;
		SkipFor(*Whole, RW, 2 * Half);
		std::unique_ptr<ss::Tournament> First = Field();
		autoplay::Run RF;
		RF.Sharp = 0.71;
		RF.Tilt = 0.2;
		SkipFor(*First, RF, Half);
		// The checkpoint and the host text, then a fresh night that reads them back.
		const std::string Ck = First->Checkpoint();
		const std::string Host = autoplay::RunText(Plan, RF) + "\n" + AutoSeenText(RF);
		std::unique_ptr<ss::Tournament> Second = Field();
		autoplay::TableAuto A2;
		autoplay::Run R2;
		bool Read = Second->Restore(Ck);
		size_t At = 0;
		while (At < Host.size())
		{
			const size_t End = Host.find('\n', At);
			const std::string Line = Host.substr(At, End == std::string::npos ? std::string::npos : End - At);
			At = End == std::string::npos ? Host.size() : End + 1;
			if (Line.rfind("auto\t", 0) == 0)
			{
				Read = autoplay::ReadRun(Line, A2, R2) && Read;
			}
			else if (Line.rfind("autoseen\t", 0) == 0)
			{
				const size_t Tab = Line.find('\t', 9);
				ss::reads::Stats S;
				Read = Tab != std::string::npos && ss::reads::Parse(Line.substr(Tab + 1), S) && Read;
				R2.Seen[Line.substr(9, Tab - 9)] = S;
			}
		}
		SkipFor(*Second, R2, Half);
		const bool Same = Read && Second->Checkpoint() == Whole->Checkpoint() && autoplay::RunText(Plan, R2) == autoplay::RunText(Plan, RW) && AutoSeenText(R2) == AutoSeenText(RW);
		Expect(Same, (std::string("autoplay: a resumed night plays on as the uninterrupted one (") + I.Id + ")").c_str());
		Expect(RW.AiHands > 0 && RW.AiDecisions >= RW.AiHands / 2 && RW.OwnDecisions == 0, (std::string("autoplay: a skipped night counts the plan's hands and decisions (") + I.Id + ")").c_str());
	}
	std::printf("  autoplay: resume == uninterrupted over every style, %.1f s\n", AutoSecondsSince(T0));
}
} // namespace auto_seg

void SegAuto()
{
	const auto T0 = std::chrono::steady_clock::now();
	auto_seg::AutoUnits();
	auto_seg::AutoNoGrading();
	auto_seg::AutoFieldPots();
	auto_seg::AutoWatchEqualsSkip();
	auto_seg::AutoWithStops();
	auto_seg::AutoSprintParity();
	auto_seg::AutoTakeover();
	auto_seg::AutoFormRoundTrip();
	auto_seg::AutoParityCommand();
	auto_seg::AutoKastSpeed();
	auto_seg::AutoSharpness();
	auto_seg::AutoSkipStream();
	auto_seg::AutoOtherTables();
	auto_seg::AutoResume();
	std::printf("  autoplay (core-autoplay): %.1f s\n", auto_seg::AutoSecondsSince(T0));
}
} // namespace session_test
