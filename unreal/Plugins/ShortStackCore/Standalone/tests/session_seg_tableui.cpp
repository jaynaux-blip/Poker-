// night_one_session: core-tableui's additions (next_segment_spec §7.6, §8.3), compiled into session_test and called
// from main() in session_main.cpp (the shared kit is SessionSeg.h):
// - EmbercrestClock: live::Clock and live::PayLadder against a real Nightly as the engine plays it: every fact the
//   tournament's, late registration and its last call by the occurrence's clock, the funding row, the colour-up, the pay
//   jumps once in the money, and a ladder that pays the whole pool;
// - TableSizing: the presets, defaults, steps and labels of §6.2 D on real hands (unopened with an ante, facing a raise
//   out of position, a flop bet, a raise of a bet, a cash game with no ante), every preset a legal raise;
// - HandRecallHonest: a hand's record holds only what the player saw (their cards, the board as dealt, the actions, the
//   hands turned up), and survives the checkpoint's text;
// - HandForHandWords: online, hand for hand is "Hand-for-hand play." under HAND FOR HAND at two and more from the money,
//   "on the bubble" (THE BUBBLE) only at one.
#include "SessionSeg.h"

#include "ShortStack/AI/Profiles.h"
#include "ShortStack/Game/ChipSet.h"
#include "ShortStack/Game/Recall.h"
#include "ShortStack/Game/Sizing.h"
#include "ShortStack/Rng.h"
#include "ShortStack/UI/TableHud.h"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

namespace session_test
{
namespace tableui_seg
{
ss::live::Occurrence Nightly()
{
	for (int Day = 1; Day < 9; ++Day)
	{
		for (const ss::live::Occurrence& O : ss::live::Occurrences(Day))
		{
			if (std::string(O.T->Key) == "nightly")
			{
				return O;
			}
		}
	}
	return ss::live::Occurrence();
}

void EmbercrestClock()
{
	const ss::live::Occurrence O = Nightly();
	Expect(O.Valid(), "EmbercrestClock: a Nightly on the schedule");
	if (!O.Valid())
	{
		return;
	}
	const std::vector<ss::ReservedPlayer> People = {{"Rick", ss::Archetype::Lag}, {"Mrs. Park", ss::Archetype::Nit}, {"Mei", ss::Archetype::Tag}, {"Sal", ss::Archetype::Station}};
	const std::vector<std::pair<std::string, std::string>> Known = {{"Rick", "Knows you"}, {"Mrs. Park", "Regular"}, {"Mei", ""}, {"Sal", "Regular"}};
	std::unique_ptr<ss::Tournament> T = ss::live::MakeField(O, 94, "Jesse", "tableui-clock", People);
	std::vector<ss::ui::FieldRow> AtBreak;
	bool FieldHonest = true;
	bool SawCounts = false;
	ss::live::ClockFacts F = ss::live::Clock(*T, O, nullptr, O.Start + 5.0, T->LevelSecondsLeft(), false, 0.0);
	Expect(F.Level == 1 && F.Now.Bb == T->CurrentLevel().Bb && F.Next.Bb == T->NextLevel().Bb, "EmbercrestClock: the level and the next from the engine");
	Expect(F.Entrants == 94 && F.Remaining == T->Remaining && F.Paid == T->PaidPlaces() && F.ToTheMoney == T->Remaining - T->PaidPlaces(), "EmbercrestClock: the field and the paid places");
	Expect(F.PoolCents == T->PrizePoolCents && F.MinCashCents == T->Payouts.back(), "EmbercrestClock: the pool and the min-cash");
	Expect(F.Average == static_cast<ss::Chips>(ss::JsRound(T->AverageStack())), "EmbercrestClock: the average is the engine's (the dealer rounds the same number)");
	Expect(F.BreakBeforeLevel == 7 && F.SecondsToBreak == F.LevelSecondsLeft + 5.0 * 20.0 * 60.0, "EmbercrestClock: the first break comes before level 7");
	Expect(F.Projected && F.LateReg == "LATE REG TO 8:00 PM", "EmbercrestClock: late registration until 8:00 PM, the pool projected");
	F = ss::live::Clock(*T, O, nullptr, ss::live::LastCallAt(O.LateRegEnds) + 1.0, T->LevelSecondsLeft(), false, 0.0);
	Expect(F.LateReg == "LAST CALL \xC2\xB7 REG TO 8:00 PM", "EmbercrestClock: the floor's last call");
	F = ss::live::Clock(*T, O, nullptr, O.LateRegEnds + 1.0, T->LevelSecondsLeft(), false, 0.0);
	Expect(F.LateReg.empty() && !F.Projected, "EmbercrestClock: registration closed, the pool posted");
	Expect(F.TableId == T->Hero().TableId && F.Seat == T->Hero().Seat + 1, "EmbercrestClock: the player's table and seat");
	// Who paid, in the clock card's words.
	ss::life::LiveEntry E;
	E.Fund.From = ss::life::Paid::Backer;
	E.Fund.Who = "Big Lou";
	E.Fund.Permille = 1000;
	Expect(ss::live::Clock(*T, O, &E, O.Start, 600.0, false, 0.0).Funding == "BACKED \xC2\xB7 BIG LOU 100%", "EmbercrestClock: BACKED \xC2\xB7 BIG LOU 100%");
	E.Fund = ss::life::Funding();
	E.Fund.From = ss::life::Paid::Sponsor;
	E.Fund.Who = "Team Embercrest";
	Expect(ss::live::Clock(*T, O, &E, O.Start, 600.0, false, 0.0).Funding == "COVERED \xC2\xB7 TEAM EMBERCREST", "EmbercrestClock: COVERED \xC2\xB7 TEAM EMBERCREST");
	E.Fund = ss::life::Funding();
	E.Fund.Permille = 350;
	E.Fund.Cents = 5530;
	Expect(ss::live::Clock(*T, O, &E, O.Start, 600.0, false, 0.0).Funding == "SOLD 35% OF YOUR ACTION", "EmbercrestClock: pieces sold");
	E.Fund = ss::life::Funding();
	Expect(ss::live::Clock(*T, O, &E, O.Start, 600.0, false, 0.0).Funding.empty(), "EmbercrestClock: the player's own money says nothing");
	// Play the night with the engine; check the facts at the colour-up level and in the money.
	const ss::Profile Sprint = ss::SprintProfile();
	bool SawColorUp = false;
	bool SawBreak = false;
	bool SawMoney = false;
	int Guard = 0;
	while (!T->bFinished && ++Guard < 4000)
	{
		const int Was = T->LevelIndex;
		const std::vector<ss::TEvent> Ev = T->SimulateTick(&Sprint);
		for (const ss::TEvent& Te : Ev)
		{
			if (Te.Type == ss::TEventType::Break)
			{
				// The floor posts counts at the break: the known players still in, their stacks now.
				AtBreak = ss::ui::FieldCounts(*T, Known);
				for (const ss::ui::FieldRow& R : AtBreak)
				{
					const ss::TPlayer* P = nullptr;
					for (const ss::TPlayer& Q : T->Players)
					{
						P = Q.Name == R.Name ? &Q : P;
					}
					FieldHonest = FieldHonest && P && !P->Busted && R.Count == P->Stack && R.Table == P->TableId;
				}
				SawCounts = SawCounts || !AtBreak.empty();
			}
			if (Te.Type == ss::TEventType::Break && Te.LevelNumber == 13)
			{
				const ss::live::ClockFacts B = ss::live::Clock(*T, O, nullptr, O.LateRegEnds + 300.0, 1200.0, true, 900.0);
				SawBreak = B.OnBreak && B.Level == 13 && B.ColorUp == "COLORING UP THE BLACK 100s" && B.BreakBeforeLevel == 19 && B.SecondsToBreak == 900.0 + 1200.0 + 5.0 * 1200.0;
			}
		}
		// Between breaks: who's still in and where, with the break's counts (the room can't see other tables' stacks).
		if (!AtBreak.empty())
		{
			for (const ss::ui::FieldRow& R : ss::ui::FieldNow(*T, Known, AtBreak))
			{
				ss::Chips Posted = -1;
				for (const ss::ui::FieldRow& B : AtBreak)
				{
					Posted = B.Name == R.Name ? B.Count : Posted;
				}
				const ss::TPlayer* P = nullptr;
				for (const ss::TPlayer& Q : T->Players)
				{
					P = Q.Name == R.Name ? &Q : P;
				}
				FieldHonest = FieldHonest && R.Count == Posted && P && !P->Busted && R.Table == P->TableId && R.Here == (!T->Hero().Busted && P->TableId == T->Hero().TableId);
			}
		}
		if (T->LevelIndex == 11 && Was != 11)
		{
			const ss::live::ClockFacts L12 = ss::live::Clock(*T, O, nullptr, O.LateRegEnds + 200.0, T->LevelSecondsLeft(), false, 0.0);
			SawColorUp = L12.ColorUp == "COLOR-UP AT THE BREAK \xC2\xB7 THE BLACK 100s" && L12.BreakBeforeLevel == 13;
		}
		if (T->InTheMoney() && T->Remaining >= 3 && !SawMoney)
		{
			const ss::live::ClockFacts M = ss::live::Clock(*T, O, nullptr, O.LateRegEnds + 400.0, T->LevelSecondsLeft(), false, 0.0);
			int Top = 0;
			for (const std::pair<int, int>& Band : ss::PayoutBands(T->PaidPlaces()))
			{
				Top = T->Remaining >= Band.first && T->Remaining <= Band.second ? Band.first - 1 : Top;
			}
			SawMoney = M.InTheMoney && M.ToTheMoney == 0 && M.NextJumpAt == Top && M.NextJumpCents == T->PrizeFor(Top) - T->PrizeFor(T->Remaining) && M.NextJumpCents > 0;
			// The ladder pays the whole pool, band by band, and marks the next one out's band.
			const std::vector<ss::live::PayRow> Ladder = ss::live::PayLadder(*T);
			ss::Chips Paid = 0;
			int Next = 1;
			int Here = 0;
			for (const ss::live::PayRow& R : Ladder)
			{
				Expect(R.From == Next && R.To >= R.From, "EmbercrestClock: the ladder's bands run 1st down, no gaps");
				Next = R.To + 1;
				for (int P = R.From; P <= R.To; ++P)
				{
					Paid += T->PrizeFor(P);
				}
				Here += R.Here ? 1 : 0;
				if (R.Here)
				{
					Expect(T->Hero().Busted ? (T->Hero().Place >= R.From && T->Hero().Place <= R.To) : (T->Remaining >= R.From && T->Remaining <= R.To),
						"EmbercrestClock: the ladder marks the player's place or the next one out");
				}
			}
			Expect(Next == T->PaidPlaces() + 1 && Paid == T->PrizePoolCents && Here <= 1, "EmbercrestClock: the ladder pays the whole pool");
		}
	}
	Expect(SawColorUp, "EmbercrestClock: level 12 announces the colour-up of the black 100s at the next break");
	Expect(SawBreak, "EmbercrestClock: the break before level 13 colours up the black 100s");
	Expect(SawMoney, "EmbercrestClock: in the money, the next jump is the next band's");
	Expect(SawCounts && FieldHonest, "EmbercrestClock: the FIELD page posts counts at the breaks and keeps them till the next");
}

// ------------------------------------------------------------------ sizing

ss::HandConfig Table(const std::vector<ss::Chips>& Stacks, int Button, ss::Chips Sb, ss::Chips Bb, ss::Chips Ante)
{
	ss::HandConfig Cfg;
	for (size_t K = 0; K < Stacks.size(); ++K)
	{
		Cfg.Players.push_back({static_cast<int>(K), "p" + std::to_string(K), Stacks[K]});
	}
	Cfg.ButtonSeat = Button;
	Cfg.SmallBlind = Sb;
	Cfg.BigBlind = Bb;
	Cfg.Ante = Ante;
	return Cfg;
}

int ToActSeat(const ss::Hand& H)
{
	return H.ToAct >= 0 ? H.Seats[static_cast<size_t>(H.ToAct)].Seat : -1;
}

std::string Labels(const std::vector<ss::sizing::Preset>& P)
{
	std::string Out;
	for (const ss::sizing::Preset& Pr : P)
	{
		Out += (Out.empty() ? "" : " ") + Pr.Label;
	}
	return Out;
}

bool AllLegal(const ss::Hand& H, const std::vector<ss::sizing::Preset>& P)
{
	for (const ss::sizing::Preset& Pr : P)
	{
		ss::Hand Copy = H;
		if (!Copy.Act(ss::PlayerAction::RaiseTo(static_cast<double>(Pr.To))))
		{
			return false;
		}
	}
	return !P.empty();
}

void TableSizing()
{
	using namespace ss::sizing;
	// Level 9 at the Embercrest: 800/1,600, a big-blind ante of 1,600. The player on the button (seat 5), folded to.
	ss::Rng R("tableui-sizing");
	ss::Hand H(Table({64200, 31800, 18400, 21000, 27500, 49600}, 5, 800, 1600, 1600), R);
	for (int K = 0; K < 3; ++K)
	{
		H.Act(ss::PlayerAction::Fold());
	}
	Expect(ToActSeat(H) == 5, "TableSizing: folded to the button");
	Spot S = SpotOf(H, 5);
	Expect(S.On == ss::Street::Preflop && S.Raises == 0 && S.CurrentBet == 1600 && S.Pot == 4000 && S.ToCall == 1600 && S.MinTo == 3200 && S.MaxTo == 49600 && S.Unit == 100,
		"TableSizing: the unopened spot");
	Expect(S.Effective == 49600 && S.InPosition, "TableSizing: 31 BB effective, in position");
	std::vector<Preset> P = Presets(S);
	Expect(Labels(P) == "MIN 2.2x 2.5x 3x POT ALL IN", ("TableSizing: unopened presets (" + Labels(P) + ")").c_str());
	Expect(P[1].To == 3500 && P[2].To == 4000 && P[3].To == 4800 && P[4].To == 7200 && P[5].AllIn && P[5].To == 49600, "TableSizing: the unopened sizes, in whole hundreds");
	Expect(AllLegal(H, P), "TableSizing: every unopened preset is a legal raise");
	Expect(Default(S) == 3500, "TableSizing: the default open is 2.2x");
	Expect(Describe(S, 3500) == "RAISE TO 3,500 \xC2\xB7 2.2x \xC2\xB7 2.2 BB" && Describe(S, 49600) == "ALL IN 49,600 \xC2\xB7 31 BB", ("TableSizing: the labels (" + Describe(S, 3500) + ")").c_str());
	Expect(Step(S, 3500, 1) == 5100 && Step(S, 3500, -1) == 3200 && Step(S, 40000, 1) == 48000, "TableSizing: fine steps: a big blind under 20 BB, five above");
	Expect(Snap(S, 3500, 1) == 2 && Snap(S, 3500, -1) == 0 && Snap(S, 49600, 1) == -1 && Snap(S, 3550, 0) == 1, "TableSizing: the bumpers step through the presets");
	// Shallower: under 25 BB effective the open is 2x.
	{
		ss::Rng R2("tableui-sizing-short");
		ss::Hand Hs(Table({30000, 31800, 18400, 21000, 27500, 30000}, 5, 800, 1600, 1600), R2);
		for (int K = 0; K < 3; ++K)
		{
			Hs.Act(ss::PlayerAction::Fold());
		}
		const Spot Ss = SpotOf(Hs, 5);
		Expect(Ss.Effective == 30000 && Default(Ss) == 3200, "TableSizing: under 25 BB effective the open is 2x");
	}
	// The button opens to 3,500; the small blind, out of position, faces it.
	H.Act(ss::PlayerAction::RaiseTo(3500.0));
	Expect(ToActSeat(H) == 0, "TableSizing: the small blind to act");
	S = SpotOf(H, 0);
	P = Presets(S);
	Expect(S.Raises == 1 && !S.InPosition && S.MinTo == 5400, "TableSizing: facing a raise, out of position");
	Expect(Labels(P) == "MIN 3x POT 4x ALL IN" && P[1].To == 10500 && P[2].To == 13700 && P[3].To == 14000, ("TableSizing: facing-a-raise presets (" + Labels(P) + ")").c_str());
	Expect(AllLegal(H, P), "TableSizing: every facing-a-raise preset is legal");
	Expect(Default(S) == 14000, "TableSizing: 4x out of position");
	Expect(Describe(S, 10500) == "RAISE TO 10,500 \xC2\xB7 3x \xC2\xB7 6.6 BB", ("TableSizing: a 3-bet's label (" + Describe(S, 10500) + ")").c_str());
	// In position (the button behind the cutoff), the 3-bet is 3x.
	{
		ss::Rng R3("tableui-sizing-ip");
		ss::Hand Hi(Table({64200, 31800, 18400, 21000, 27500, 49600}, 5, 800, 1600, 1600), R3);
		Hi.Act(ss::PlayerAction::Fold());
		Hi.Act(ss::PlayerAction::Fold());
		Hi.Act(ss::PlayerAction::RaiseTo(3500.0)); // the cutoff opens
		const Spot Si = SpotOf(Hi, 5);              // the button, behind them
		Expect(Si.InPosition && Default(Si) == 10500, "TableSizing: 3x in position");
	}
	// To the flop: the small blind calls, the big blind calls. The small blind leads.
	H.Act(ss::PlayerAction::Call());
	H.Act(ss::PlayerAction::Call());
	Expect(H.CurrentStreet == ss::Street::Flop && ToActSeat(H) == 0, "TableSizing: on the flop, the small blind first");
	S = SpotOf(H, 0);
	P = Presets(S);
	Expect(S.IsBet && S.Pot == 3 * 3500 + 1600, "TableSizing: an unbet flop");
	Expect(Labels(P) == "1/3 1/2 2/3 3/4 POT ALL IN" && P[2].To == 8100 && P[4].To == 12100, ("TableSizing: flop presets (" + Labels(P) + ")").c_str());
	Expect(AllLegal(H, P), "TableSizing: every flop preset is a legal bet");
	Expect(Default(S) == 8100 && Describe(S, 8100) == "BET 8,100 \xC2\xB7 67% POT", ("TableSizing: two-thirds pot, said as a share of it (" + Describe(S, 8100) + ")").c_str());
	// A bet of 4,000, and the big blind facing it.
	H.Act(ss::PlayerAction::RaiseTo(4000.0));
	S = SpotOf(H, 1);
	P = Presets(S);
	Expect(Labels(P) == "MIN 2.5x 3x POT ALL IN" && P[0].To == 8000 && P[1].To == 10000 && P[2].To == 12000, ("TableSizing: facing a bet (" + Labels(P) + ")").c_str());
	Expect(AllLegal(H, P), "TableSizing: every raise of the bet is legal");
	Expect(Default(S) == 12000, "TableSizing: 3x the bet");
	// Dee's game: $1/$2, no ante: the opens are 2.5x, 3x and 4x, in dollars.
	{
		ss::Rng R4("tableui-sizing-dee");
		ss::Hand Hd(Table({212, 340, 96, 455, 180, 248}, 5, 1, 2, 0), R4);
		for (int K = 0; K < 3; ++K)
		{
			Hd.Act(ss::PlayerAction::Fold());
		}
		const Spot Sd = SpotOf(Hd, 5);
		const std::vector<Preset> Pd = Presets(Sd);
		Expect(Sd.Unit == 1 && Labels(Pd) == "MIN 2.5x 3x POT 4x ALL IN" && Default(Sd) == 6, ("TableSizing: a cash game opens 3x (" + Labels(Pd) + ")").c_str());
		Expect(AllLegal(Hd, Pd), "TableSizing: every cash-game preset is legal");
	}
	// Short: a raise that can only be all in has the one preset.
	{
		ss::Rng R5("tableui-sizing-shove");
		ss::Hand Hs(Table({64200, 31800, 18400, 21000, 27500, 3000}, 5, 800, 1600, 1600), R5);
		for (int K = 0; K < 3; ++K)
		{
			Hs.Act(ss::PlayerAction::Fold());
		}
		const Spot Ss = SpotOf(Hs, 5);
		Expect(Labels(Presets(Ss)) == "ALL IN" && Presets(Ss)[0].To == 3000, "TableSizing: a short stack's one preset is all in");
	}
	// Deep in the night, 25,000/50,000 with a 50,000 ante: the orange 5,000s are still in play, so a 2.2x open is
	// 110,000 (not rounded down to the min-raise in 25,000 chips).
	{
		ss::Rng R6("tableui-sizing-deep");
		ss::Hand Hd(Table({4100000, 2600000, 1800000, 2950000, 2200000, 3300000}, 5, 25000, 50000, 50000), R6);
		for (int K = 0; K < 3; ++K)
		{
			Hd.Act(ss::PlayerAction::Fold());
		}
		const Spot Sd = SpotOf(Hd, 5);
		const std::vector<Preset> Pd = Presets(Sd);
		Expect(Sd.Unit == 5000 && Labels(Pd) == "MIN 2.2x 2.5x 3x POT ALL IN" && Pd[1].To == 110000 && Default(Sd) == 110000,
			("TableSizing: late levels still size in the chips in play (" + Labels(Pd) + ")").c_str());
		Expect(AllLegal(Hd, Pd), "TableSizing: every late-level preset is legal");
	}
}

// ------------------------------------------------------------------ recall

void HandRecallHonest()
{
	ss::Rng R("tableui-recall");
	ss::Hand H(Table({20000, 20000, 20000, 20000}, 3, 100, 200, 200), R);
	const std::vector<std::string> Names = {"Rick", "Mrs. Park", "Mei; the \\/ one, ok", "You"};
	// Seat 2 (UTG in a four-handed game... the first to act) folds; everyone else checks it down.
	int Folded = -1;
	int Guard = 0;
	while (!H.bComplete && ++Guard < 60)
	{
		const int Seat = ToActSeat(H);
		const ss::LegalActions L = H.GetLegalActions();
		if (Folded < 0 && Seat != 3)
		{
			Folded = Seat;
			H.Act(ss::PlayerAction::Fold());
		}
		else
		{
			H.Act(L.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Call());
		}
	}
	Expect(H.bComplete && H.Board.size() == 5 && Folded >= 0, "HandRecallHonest: a hand to the river with one fold");
	// Only the winner turns their cards up; the other hand at the showdown is mucked.
	int Shown = -1;
	for (const ss::PotResult& P : H.PotResults)
	{
		for (const int W : P.Winners)
		{
			Shown = W != 3 ? W : Shown;
		}
	}
	const std::vector<int> ShownSeats = Shown >= 0 ? std::vector<int>{Shown} : std::vector<int>{};
	const ss::recall::Record Rec = ss::recall::Of(H, 3, Names, ShownSeats, 212, 9);
	const ss::HandSeat* Me = H.SeatByNumber(3);
	Expect(Rec.Number == 212 && Rec.Level == 9 && Rec.Blinds.Bb == 200 && Rec.Blinds.Ante == 200, "HandRecallHonest: the hand's number and level");
	Expect(Me && Rec.Mine == Me->Hole && Rec.HeroNet == Me->Stack - Me->StartStack, "HandRecallHonest: the player's own cards and what the hand did to them");
	Expect(Rec.Board == H.Board, "HandRecallHonest: the board as dealt");
	Expect(Rec.Lines.size() == H.History.size() && Rec.Lines.front().Who == Names[static_cast<size_t>(H.History.front().Seat)], "HandRecallHonest: every action, by name");
	bool Honest = true;
	for (const std::pair<std::string, std::vector<ss::Card>>& S : Rec.Shown)
	{
		Honest = Honest && S.first != Names[static_cast<size_t>(Folded)];
		for (const ss::HandSeat& Hs : H.Seats)
		{
			const bool Visible = Hs.Seat == Shown;
			for (const ss::Card C : Hs.Hole)
			{
				if (!Visible && Hs.Seat != 3)
				{
					Honest = Honest && std::find(S.second.begin(), S.second.end(), C) == S.second.end();
				}
			}
		}
	}
	Expect(Honest && Rec.Shown.size() == ShownSeats.size(), "HandRecallHonest: no folded or mucked hand, only what was turned up");
	// The checkpoint's text: written, read, the same.
	ss::recall::Record Back;
	Expect(ss::recall::Read(ss::recall::Write(Rec), Back) && ss::recall::Write(Back) == ss::recall::Write(Rec) && Back.Lines.size() == Rec.Lines.size() &&
			   Back.Mine == Rec.Mine && Back.Board == Rec.Board && Back.HeroNet == Rec.HeroNet && Back.Pot == Rec.Pot,
		"HandRecallHonest: the record survives the checkpoint's text");
	bool Names_ = false;
	for (const ss::recall::Line& L : Back.Lines)
	{
		Names_ = Names_ || L.Who == Names[2];
	}
	Expect(Names_, "HandRecallHonest: a name with the text's separators in it comes back whole");
	Expect(!ss::recall::Read("not a record", Back) && !ss::recall::Read("1;2;3", Back), "HandRecallHonest: junk isn't read");
	// A hand over before the flop has no board.
	{
		ss::Rng R2("tableui-recall-pre");
		ss::Hand Hp(Table({20000, 20000, 20000}, 0, 100, 200, 200), R2);
		while (!Hp.bComplete)
		{
			Hp.Act(ss::PlayerAction::Fold());
		}
		const ss::recall::Record Pre = ss::recall::Of(Hp, 0, {"You", "Rick", "Mei"}, {}, 3, 1);
		Expect(Pre.Board.empty() && Pre.Shown.empty(), "HandRecallHonest: no board when it ended preflop");
	}
}

/**
 * Online, hand for hand (round 4): "Hand-for-hand play." under HAND FOR HAND at two and more from the money, "on the
 * bubble" (THE BUBBLE) only at one, as the card room's floor says it. The $0.25 Hyper Sprint cut to twenty runners (two
 * tables, three paid) is played through to hand for hand, the player taking the grader's best option, night after night
 * until the engine has called it both at two from the money and at one (the field jumping there in one hand); each call's
 * line and banner are read against how far the money is when it's made.
 */
void HandForHandWords()
{
	ss::LobbyEvent Listing = ss::Lobby()[1];
	Listing.Spec.Entrants = 20;
	int OneAway = 0;
	int Further = 0;
	int Nights = 0;
	std::string Problem;
	for (char Seed = 'a'; Seed <= 'x' && (OneAway == 0 || Further == 0); ++Seed)
	{
		++Nights;
		Hooks H;
		ss::Session S(H, std::string("h4h-words-") + Seed);
		S.CurrentScreen = ss::Screen::Lobby;
		S.BankrollCents = 10000;
		S.RegisterEvent(Listing);
		if (!S.T)
		{
			Problem = ": no table";
			break;
		}
		double Now = 0.0;
		std::string Line;
		int Away = 0;
		for (int Frames = 0; Frames < 400000 && Line.empty() && S.CurrentScreen == ss::Screen::Table; ++Frames)
		{
			Now += 1.0 / 30.0;
			S.Update(Now);
			for (const ss::ChatLine& L : S.Chat) // the chat keeps its last 80 lines: read whole, every frame
			{
				if (Line.empty() && L.Kind == ss::ChatKind::System && L.Text.find("Hand-for-hand") != std::string::npos)
				{
					Line = L.Text;
					Away = S.T ? S.T->Remaining - S.T->PaidPlaces() : 0;
				}
			}
			if (S.HasPrompt && Now - S.Prompt.OpenedAt > 0.3)
			{
				const std::vector<ss::OptionEV>& Opts = S.Prompt.Analysis.Options;
				size_t Best = 0;
				for (size_t I = 1; I < Opts.size(); ++I)
				{
					Best = Opts[I].Ev > Opts[Best].Ev ? I : Best;
				}
				S.HeroAct(Opts.empty() ? ss::PlayerAction::Fold() : Opts[Best].Action);
			}
		}
		if (Line.empty())
		{
			continue; // out before hand for hand
		}
		// The banner, unless the player's own bust took its place in the same hand.
		const std::string& Title = S.CurrentBanner.Title;
		const bool Banner = Title == "HAND FOR HAND" || Title == "THE BUBBLE";
		const bool Right = Away == 1 ? Line == "We are on the bubble. Hand-for-hand play." && (!Banner || Title == "THE BUBBLE")
									 : Away >= 2 && Line == "Hand-for-hand play." && (!Banner || Title == "HAND FOR HAND");
		std::printf("  hand for hand online (night %c): %d from the money, \"%s\" under %s (%s)\n", Seed, Away, Line.c_str(), Title.c_str(), S.CurrentBanner.Sub.c_str());
		OneAway += Away == 1 && Banner ? 1 : 0;
		Further += Away >= 2 && Banner ? 1 : 0;
		if (!Right && Problem.empty())
		{
			Problem = ": \"" + Line + "\" under " + Title + ", " + std::to_string(Away) + " from the money";
		}
	}
	Expect(Problem.empty() && OneAway >= 1 && Further >= 1, ("HandForHandWords: hand for hand online is \"Hand-for-hand play.\" (HAND FOR HAND) at two and more from the money, "
															 "\"on the bubble\" (THE BUBBLE) at one (" +
															 std::to_string(Further) + " at two and more, " + std::to_string(OneAway) + " at one, " +
															 std::to_string(Nights) + " nights" + Problem + ")")
															.c_str());
}
} // namespace tableui_seg

void SegTableUi()
{
	tableui_seg::EmbercrestClock();
	tableui_seg::TableSizing();
	tableui_seg::HandRecallHonest();
	tableui_seg::HandForHandWords();
}
} // namespace session_test
