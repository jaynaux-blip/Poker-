// card_room: the plan's rules at the card room (update 2 SPEC §2.8, §2.12; the lead's, C1). BackRoomAutoRules.h is a game
// header (unreal/Source/ShortStack) written over the core's types and nothing of Unreal's; CMakeLists.txt gives this file
// alone the include path for it, builds it as room_autorules_test and runs it inside card_room after room_test. These were
// ue-auto's harness outside the repo (ssb/u2-ue-auto-x), where mutating the read stayed green; here they pin every line:
//   RulesHeartTerms     UpdateComposure's terms one at a time (6; +30 x pot/stack to 1.5; +22 x the price to half the stack;
//                       +28 an all-in call; +28 all in; +18 a bluff; +8 watched; +20 x tilt; the moment), the resting rate's
//                       clamp, the shake's ramp (95 to 150) and the moment (hand for hand 14, the final table 10, near the
//                       money 6).
//   RulesBluffLine      the 0.42 line (AutoIsBluff), and over hundreds of seeded spots AutoPlanHeartAt's bluff is that line on
//                       the felt's equity, which counts the players still in when the bet went in (every reader of one bet
//                       sees the same shake).
//   RulesLastAggressor  nothing to read unless the player made this street's last bet or raise: re-raised, an earlier
//                       street's bet, folded; the price the player faced raising and the pot at the bet are the heart's.
//   RulesReadDirection  AutoApplyRead over 400 rolls a spot: readers above 0 call a bluff and fold to a bet with the goods,
//                       readers below 0 take both for a bluff; the roll, the chance, the 12% and 60% guards; strangers, a
//                       small read, nobody facing a bet; the note.
//   RulesWatchEqualsSkip watch == skip with the read on, hand by hand, for six styles; every read the hook takes is one the
//                       rules allow and repeatable; the hook changes the night; strangers don't read.
//   RulesHostText, RulesSmall  the host text's lines round-trip; the learned book, Dee's clock, the skip card, the parity
//                       hash, a skipped hand's record and showdown, and the HANDS page's numbers (unique across a skip).
// room_seg_autorules::Run() returns the failures, so room_test's main() could call it as it does room_seg_auto's; until then
// SS_AUTORULES_MAIN gives this file its own main().
#include "BackRoomAutoRules.h"

#include "ShortStack/Cards.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/LiveAuto.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace room_seg_autorules
{
namespace rules = BackRoomAutoRules;
namespace autoplay = ss::autoplay;
namespace style = ss::style;

int RulesFailures = 0;
int RulesChecks = 0;

void RulesExpect(bool Ok, const std::string& What)
{
	++RulesChecks;
	if (!Ok)
	{
		++RulesFailures;
		std::printf("FAILED: %s\n", What.c_str());
	}
}

bool RulesNear(double A, double B)
{
	return std::fabs(A - B) < 1e-9;
}

const char* const RulesCast[] = {"Mei", "Rick", "Dre", "Sal", "Twitch", "Big Lou", "Mrs. Park", "gh0stfold", "Omar", "Lin", "Duke", "Bev"};
const double RulesCastRead[] = {0.9, -0.3, -0.2, 0.6, -0.5, -0.6, 0.7, 1.0, 0.0, 0.0, 0.0, 0.0};

/** A seeded 60-runner Nightly with the cast in it (the hero's stack times HeroStacks, so a night lasts long enough). */
std::unique_ptr<ss::Tournament> RulesField(const std::string& Salt, int HeroStacks)
{
	const ss::live::Occurrence O = ss::live::FindOccurrence("embercrest-nightly@15");
	std::vector<ss::ReservedPlayer> Known;
	static const ss::Archetype Kinds[] = {ss::Archetype::Reg, ss::Archetype::Lag, ss::Archetype::Station, ss::Archetype::Nit, ss::Archetype::Maniac, ss::Archetype::Tag};
	int K = 0;
	for (const char* Name : RulesCast)
	{
		Known.push_back({Name, Kinds[K++ % 6]});
	}
	std::unique_ptr<ss::Tournament> T = ss::live::MakeField(O, 60, "Jesse", "autorules:" + Salt, Known);
	if (T)
	{
		T->Players[static_cast<size_t>(T->PlayerIndex(ss::HeroId))].Stack *= HeroStacks;
	}
	return T;
}

// ------------------------------------------------------------------ spots: a three-handed hand on a fixed deck

/**
 * A hand the player is in, three-handed, the player on the button (first to act preflop), the two others players of a real
 * field so the read can name them. Cards: the player's two, then the board's five; the others get what's left in order.
 */
struct RulesSpot
{
	std::unique_ptr<ss::Tournament> T;
	std::unique_ptr<ss::Hand> H;
	std::string A, B; // seats 1 (the small blind) and 2 (the big blind): their tournament ids
	rules::AutoReadModel M;
};

std::unique_ptr<RulesSpot> RulesDeal(const std::string& Mine, const std::string& Board, double ReadA, double ReadB)
{
	std::unique_ptr<RulesSpot> S(new RulesSpot());
	S->T = RulesField("spot", 1);
	if (!S->T)
	{
		return S;
	}
	for (const ss::TPlayer& P : S->T->Players)
	{
		if (P.IsHero)
		{
			continue;
		}
		if (S->A.empty())
		{
			S->A = P.Id;
		}
		else if (S->B.empty())
		{
			S->B = P.Id;
		}
	}
	const std::vector<ss::Card> Hole = ss::ParseCards(Mine);
	const std::vector<ss::Card> Run = ss::ParseCards(Board);
	std::vector<ss::Card> Rest;
	for (const ss::Card C : ss::FullDeck())
	{
		bool Used = false;
		for (const ss::Card U : Hole)
		{
			Used = Used || U == C;
		}
		for (const ss::Card U : Run)
		{
			Used = Used || U == C;
		}
		if (!Used)
		{
			Rest.push_back(C);
		}
	}
	// Dealt one at a time from the left of the button: A, B, the player, A, B, the player; a burn before each street.
	size_t Next = 0;
	auto Take = [&Rest, &Next]() { return Rest[Next++]; };
	ss::HandConfig C;
	C.Players = {{0, ss::HeroId, 2000}, {1, S->A, 2000}, {2, S->B, 2000}};
	C.ButtonSeat = 0;
	C.SmallBlind = 50;
	C.BigBlind = 100;
	C.Deck = {Take(), Take(), Hole[0], Take(), Take(), Hole[1], Take(), Run[0], Run[1], Run[2], Take(), Run[3], Take(), Run[4]};
	while (Next < Rest.size())
	{
		C.Deck.push_back(Take());
	}
	ss::Rng R("autorules:deck");
	S->H.reset(new ss::Hand(C, R));
	S->M.RestBpm = 74.0;
	S->M.Sensitivity = 1.0;
	if (ReadA != 0.0)
	{
		S->M.HeroRead[S->A] = ReadA;
	}
	if (ReadB != 0.0)
	{
		S->M.HeroRead[S->B] = ReadB;
	}
	return S;
}

/** Plays the actions given ("r600" raise to, "c" call or check, "k" check, "f" fold), in turn order. */
bool RulesPlay(ss::Hand& H, const std::vector<std::string>& Moves)
{
	for (const std::string& M : Moves)
	{
		ss::PlayerAction A = ss::PlayerAction::Fold();
		if (M[0] == 'r')
		{
			A = ss::PlayerAction::RaiseTo(std::atof(M.c_str() + 1));
		}
		else if (M[0] == 'c')
		{
			A = ss::PlayerAction::Call();
		}
		else if (M[0] == 'k')
		{
			A = ss::PlayerAction::Check();
		}
		if (!H.Act(A))
		{
			return false;
		}
	}
	return true;
}

int RulesSeatOf(const ss::Hand& H, const std::string& Id)
{
	for (size_t I = 0; I < H.Seats.size(); ++I)
	{
		if (H.Seats[I].Id == Id)
		{
			return static_cast<int>(I);
		}
	}
	return -1;
}

// ------------------------------------------------------------------ the heart's terms

void RulesHeartTerms()
{
	auto P = [](const rules::AutoHeartTerms& In) { return rules::AutoHeartPressure(In); };
	rules::AutoHeartTerms In;
	RulesExpect(RulesNear(P(In), 6.0), "heart: in the hand, 6 before anything else");
	In.PotOverStack = 0.75;
	RulesExpect(RulesNear(P(In), 21.0), "heart: the pot at three quarters of the stack adds 15 (30 x pot/stack / 1.5)");
	In.PotOverStack = 3.0;
	RulesExpect(RulesNear(P(In), 36.0), "heart: the pot's share stops at 1.5 stacks (+30)");
	In = {};
	In.PriceOverStack = 0.25;
	RulesExpect(RulesNear(P(In), 17.0), "heart: a quarter of the stack to call adds 11 (22 x price x 2)");
	In.PriceOverStack = 0.8;
	RulesExpect(RulesNear(P(In), 28.0), "heart: the price's share stops at half the stack (+22)");
	In = {};
	In.CallAllIn = true;
	RulesExpect(RulesNear(P(In), 34.0), "heart: an all-in call adds 28");
	In = {};
	In.AllIn = true;
	RulesExpect(RulesNear(P(In), 34.0), "heart: all in adds 28");
	In = {};
	In.Bluff = true;
	RulesExpect(RulesNear(P(In), 24.0), "heart: a bluff out there adds 18");
	In = {};
	In.Watched = true;
	RulesExpect(RulesNear(P(In), 14.0), "heart: eyes on you add 8");
	In = {};
	In.Tilt = 0.5;
	RulesExpect(RulesNear(P(In), 16.0), "heart: half tilted adds 10 (20 x tilt)");
	In.Tilt = 3.0;
	RulesExpect(RulesNear(P(In), 26.0), "heart: tilt counts to 1 (+20)");
	In = {};
	In.Extra = 14.0;
	RulesExpect(RulesNear(P(In), 20.0), "heart: the moment adds what it says");
	In = {};
	In.PotOverStack = 1.5;
	In.PriceOverStack = 0.5;
	In.CallAllIn = true;
	In.AllIn = true;
	In.Bluff = true;
	In.Watched = true;
	In.Tilt = 1.0;
	In.Extra = 10.0;
	RulesExpect(RulesNear(P(In), 6.0 + 30.0 + 22.0 + 28.0 + 28.0 + 18.0 + 8.0 + 20.0 + 10.0), "heart: every term together is their sum");

	rules::AutoReadModel M;
	M.RestBpm = 66.0;
	M.Sensitivity = 1.0;
	RulesExpect(RulesNear(rules::AutoHeartBpm(M, 30.0), 96.0), "heart: rest 66 and 30 of pressure settle at 96");
	M.Sensitivity = 0.5;
	RulesExpect(RulesNear(rules::AutoHeartBpm(M, 40.0), 86.0), "heart: half the pressure reaches a hardened heart");
	M.Sensitivity = 1.0;
	M.RestBpm = 120.0;
	RulesExpect(RulesNear(rules::AutoHeartBpm(M, 10.0), 90.0), "heart: the resting rate counts to 80");
	M.RestBpm = 20.0;
	RulesExpect(RulesNear(rules::AutoHeartBpm(M, 10.0), 58.0), "heart: the resting rate counts from 48");
	M.RestBpm = 66.0;
	RulesExpect(RulesNear(rules::AutoHeartBpm(M, -50.0), 56.0) && RulesNear(rules::AutoHeartBpm(M, 500.0), 168.0), "heart: from 10 under rest to 168");
	RulesExpect(RulesNear(rules::AutoShakeAt(95.0), 0.0) && RulesNear(rules::AutoShakeAt(60.0), 0.0) && RulesNear(rules::AutoShakeAt(122.5), 0.5) &&
			RulesNear(rules::AutoShakeAt(150.0), 1.0) && RulesNear(rules::AutoShakeAt(190.0), 1.0),
		"heart: hands shake from 95 and all the way at 150");

	// The moment (LiveTick's ExtraPressure): nothing early, 6 near the money, 14 hand for hand, 10 at the final table.
	std::unique_ptr<ss::Tournament> T = RulesField("moment", 1);
	RulesExpect(T != nullptr, "moment: the field");
	if (T)
	{
		RulesExpect(RulesNear(rules::AutoMomentPressure(nullptr), 0.0) && RulesNear(rules::AutoMomentPressure(T.get()), 0.0), "moment: nothing at the start of a night");
		const int Paid = T->PaidPlaces();
		T->Remaining = Paid + 6;
		const double Near = rules::AutoMomentPressure(T.get());
		T->Remaining = Paid + 7;
		const double Far = rules::AutoMomentPressure(T.get());
		T->Remaining = Paid + 1;
		const double Bubble = rules::AutoMomentPressure(T.get());
		RulesExpect(T->HandForHand() && RulesNear(Bubble, 14.0), "moment: hand for hand is 14");
		RulesExpect(RulesNear(Far, 0.0) && (RulesNear(Near, 6.0) || (T->HandForHand() && RulesNear(Near, 14.0))), "moment: six from the money is 6, seven isn't");
		T->Remaining = Paid;
		while (T->Tables.size() > 1)
		{
			T->Tables.erase(std::prev(T->Tables.end()));
		}
		RulesExpect(RulesNear(rules::AutoMomentPressure(T.get()), 10.0), "moment: the final table is 10");
	}
}

// ------------------------------------------------------------------ the bluff line

void RulesBluffLine()
{
	RulesExpect(rules::AutoBluffEquity == 0.42, "bluff: the table's line is 0.42 (FeltEquity's)");
	RulesExpect(rules::AutoIsBluff(0.4199) && !rules::AutoIsBluff(0.42) && !rules::AutoIsBluff(0.4201) && rules::AutoIsBluff(0.0) && !rules::AutoIsBluff(1.0),
		"bluff: under 0.42 is a bluff, 0.42 and up isn't");

	// Seeded spots: every deck and player count, the player's bet preflop or on the flop; the heart's bluff is the line on the
	// felt's equity, literally 0.42 (so moving the line in the rules, or the comparison, fails here).
	ss::Rng Dealer("autorules:bluffline");
	std::unique_ptr<ss::Tournament> T = RulesField("bluffline", 1);
	if (!T)
	{
		RulesExpect(false, "bluff: the field");
		return;
	}
	std::vector<std::string> Ids;
	for (const ss::TPlayer& P : T->Players)
	{
		if (!P.IsHero && Ids.size() < 5)
		{
			Ids.push_back(P.Id);
		}
	}
	rules::AutoReadModel M;
	M.HeroRead[Ids[0]] = 0.7;
	int Spots = 0, Agree = 0, Band = 0, Bluffs = 0, Values = 0, Constant = 0, Checked = 0;
	for (int K = 0; K < 700; ++K)
	{
		const int N = 2 + Dealer.Int(5); // 2..6 players
		ss::HandConfig C;
		C.Players.push_back({0, ss::HeroId, 3000});
		for (int I = 1; I < N; ++I)
		{
			C.Players.push_back({I, Ids[static_cast<size_t>(I - 1)], 3000});
		}
		C.ButtonSeat = Dealer.Int(N);
		C.SmallBlind = 50;
		C.BigBlind = 100;
		ss::Rng DeckRng("autorules:deck:" + std::to_string(K));
		ss::Hand H(C, DeckRng);
		const bool Flop = (K % 2) == 1;
		// Everyone ahead of the player limps (or folds now and then); the player raises to 3 big blinds; on the flop spots the
		// others call it, everyone checks to the player, and the player bets the pot.
		int Guard = 0;
		bool Ok = true;
		while (Ok && !H.bComplete && ++Guard < 60)
		{
			const bool Mine = H.Seats[static_cast<size_t>(H.ToAct)].Id == ss::HeroId;
			if (H.CurrentStreet == ss::Street::Preflop)
			{
				if (Mine && rules::AutoLastAggression(H) < 0)
				{
					Ok = H.Act(ss::PlayerAction::RaiseTo(300.0));
					if (!Flop)
					{
						break;
					}
					continue;
				}
				const bool Fold = !Mine && Dealer.Int(5) == 0 && H.ActiveCount() > 2;
				Ok = H.Act(Fold ? ss::PlayerAction::Fold() : ss::PlayerAction::Call());
				continue;
			}
			if (H.CurrentStreet == ss::Street::Flop)
			{
				if (Mine)
				{
					Ok = H.Act(ss::PlayerAction::RaiseTo(static_cast<double>(H.Pot())));
					break;
				}
				Ok = H.Act(ss::PlayerAction::Check());
				continue;
			}
			Ok = false;
		}
		const int Hero = rules::AutoHeroIndex(H);
		const int Last = rules::AutoLastAggression(H);
		if (!Ok || H.bComplete || Hero < 0 || Last < 0 || H.History[static_cast<size_t>(Last)].Seat != H.Seats[static_cast<size_t>(Hero)].Seat)
		{
			continue;
		}
		++Spots;
		const rules::AutoPlanHeart P = rules::AutoPlanHeartAt(M, 0.0, T.get(), H, Hero);
		const double Eq = rules::AutoPlanEquity(T.get(), H, Hero, Last);
		Agree += P.Shake >= 0.0 && RulesNear(P.Equity, Eq) && P.Bluff == (Eq < 0.42) && P.Terms.Bluff == P.Bluff ? 1 : 0;
		Band += Eq >= 0.38 && Eq < 0.46 ? 1 : 0;
		Bluffs += P.Bluff ? 1 : 0;
		Values += P.Bluff ? 0 : 1;
		// The players still in when the bet went in, whoever has folded to it since, and the pot as the bet went in, whoever
		// has called it since: the next player folds, or calls, and the shake holds for the one after.
		if (!H.bComplete && H.ActiveCount() > 2 && H.Seats[static_cast<size_t>(H.ToAct)].Id != ss::HeroId)
		{
			for (const ss::PlayerAction Next : {ss::PlayerAction::Fold(), ss::PlayerAction::Call()})
			{
				ss::Hand After = H;
				if (After.Act(Next) && !After.bComplete && After.CurrentStreet == H.CurrentStreet)
				{
					++Checked;
					const rules::AutoPlanHeart Q = rules::AutoPlanHeartAt(M, 0.0, T.get(), After, Hero);
					Constant += RulesNear(Q.Shake, P.Shake) && Q.Bluff == P.Bluff && RulesNear(Q.Equity, P.Equity) && RulesNear(Q.Pressure, P.Pressure) ? 1 : 0;
				}
			}
		}
	}
	std::printf("bluff line: %d spots (%d bluffs, %d with the goods), %d within 0.04 of the line, %d checked after a fold or a call\n", Spots, Bluffs, Values, Band, Checked);
	RulesExpect(Spots >= 500 && Agree == Spots, "bluff: the heart's bluff is the felt's equity under 0.42, every spot");
	RulesExpect(Band >= 20 && Bluffs >= 50 && Values >= 50, "bluff: the spots straddle the line (enough of them close to it to catch a line moved)");
	RulesExpect(Checked >= 100 && Constant == Checked, "bluff: a player folding to the bet, or calling it, changes neither its equity nor its shake");
}

// ------------------------------------------------------------------ the last aggressor

void RulesLastAggressor()
{
	// The player raises to 600 preflop: the street's last aggression is theirs; the small blind faces it.
	{
		std::unique_ptr<RulesSpot> S = RulesDeal("7c2d", "AhKsQd3c9h", 0.9, 0.9);
		RulesExpect(S->H && RulesPlay(*S->H, {"r600"}), "last aggressor: the spot plays");
		const rules::AutoPlanHeart P = rules::AutoPlanHeartAt(S->M, 0.0, S->T.get(), *S->H, 0);
		RulesExpect(P.Shake >= 0.0, "last aggressor: the player's raise is there to read");
		// Price: facing the big blind, 100 of 2,000; the pot at the raise 50 + 100 + 600 against 2,000.
		RulesExpect(RulesNear(P.Terms.PriceOverStack, 100.0 / 2000.0) && RulesNear(P.Terms.PotOverStack, 750.0 / 2000.0) && !P.Terms.CallAllIn && !P.Terms.AllIn,
			"last aggressor: the price the player faced raising and the pot as the raise went in");
		RulesExpect(P.Terms.Watched && RulesNear(P.Pressure, rules::AutoHeartPressure(P.Terms)) && RulesNear(P.Bpm, rules::AutoHeartBpm(S->M, P.Pressure)) &&
				RulesNear(P.Shake, rules::AutoShakeAt(P.Bpm)),
			"last aggressor: a reader at the table watches; pressure, heart and shake follow from the terms");
		// The small blind re-raises: now the big blind faces their raise, not the player's. Nothing to read.
		RulesExpect(RulesPlay(*S->H, {"r1500"}), "last aggressor: the re-raise plays");
		RulesExpect(rules::AutoPlanHeartAt(S->M, 1.0, S->T.get(), *S->H, 0).Shake < 0.0, "last aggressor: re-raised, the player's raise isn't the one to read");
		int Reads = 0;
		for (int Tick = 0; Tick < 200; ++Tick)
		{
			S->T->Tick = Tick;
			ss::BotDecision D;
			D.Action = ss::PlayerAction::Fold();
			D.Equity = 0.5;
			Reads += rules::AutoApplyRead(S->M, 1.0, *S->T, *S->H, RulesSeatOf(*S->H, S->B), D, nullptr) ? 1 : 0;
		}
		RulesExpect(Reads == 0, "last aggressor: facing someone else's raise, no reader reads the player");
	}
	// The player bets the flop and is called; on the turn the small blind bets into the big blind: the flop's bet is gone.
	{
		std::unique_ptr<RulesSpot> S = RulesDeal("7c2d", "AhKsQd3c9h", 0.9, 0.9);
		RulesExpect(RulesPlay(*S->H, {"r300", "c", "c", "k", "k", "r900", "c", "c", "r500"}), "last aggressor: the flop and turn play");
		RulesExpect(S->H->CurrentStreet == ss::Street::Turn && rules::AutoPlanHeartAt(S->M, 1.0, S->T.get(), *S->H, 0).Shake < 0.0,
			"last aggressor: an earlier street's bet isn't read on this one");
		int Reads = 0;
		for (int Tick = 0; Tick < 200; ++Tick)
		{
			S->T->Tick = Tick;
			ss::BotDecision D;
			D.Action = ss::PlayerAction::Fold();
			D.Equity = 0.5;
			Reads += rules::AutoApplyRead(S->M, 1.0, *S->T, *S->H, RulesSeatOf(*S->H, S->B), D, nullptr) ? 1 : 0;
		}
		RulesExpect(Reads == 0, "last aggressor: nobody reads the player off someone else's bet");
	}
	// Checked around on the flop after the player's preflop raise: nobody faces a bet, nothing's read.
	{
		std::unique_ptr<RulesSpot> S = RulesDeal("7c2d", "AhKsQd3c9h", 0.9, 0.9);
		RulesExpect(RulesPlay(*S->H, {"r300", "c", "c", "k"}), "last aggressor: a checked flop plays");
		RulesExpect(rules::AutoPlanHeartAt(S->M, 1.0, S->T.get(), *S->H, 0).Shake < 0.0, "last aggressor: no bet this street, nothing to read");
	}
	// The player raises facing a bet on the flop: the price they faced (400 of the 1,700 they had) is in the heart.
	{
		std::unique_ptr<RulesSpot> S = RulesDeal("7c2d", "AhKsQd3c9h", 0.9, 0.9);
		RulesExpect(RulesPlay(*S->H, {"r300", "c", "c", "r400", "f", "r1200"}), "last aggressor: the check-raise spot plays");
		const rules::AutoPlanHeart P = rules::AutoPlanHeartAt(S->M, 0.0, S->T.get(), *S->H, 0);
		RulesExpect(P.Shake >= 0.0 && RulesNear(P.Terms.PriceOverStack, 400.0 / 1700.0) && RulesNear(P.Terms.PotOverStack, (900.0 + 400.0 + 1200.0) / 2000.0),
			"last aggressor: raising a bet, the price faced and the pot at the raise");
		// Folded: nothing of the player's to read.
		std::unique_ptr<RulesSpot> F = RulesDeal("7c2d", "AhKsQd3c9h", 0.9, 0.9);
		RulesExpect(RulesPlay(*F->H, {"f"}) && rules::AutoPlanHeartAt(F->M, 1.0, F->T.get(), *F->H, 0).Shake < 0.0, "last aggressor: a folded player shows nothing");
	}
	// All in: the player shoves preflop; the all-in term is there.
	{
		std::unique_ptr<RulesSpot> S = RulesDeal("7c2d", "AhKsQd3c9h", 0.9, 0.9);
		RulesExpect(RulesPlay(*S->H, {"r2000"}), "last aggressor: the shove plays");
		const rules::AutoPlanHeart P = rules::AutoPlanHeartAt(S->M, 0.0, S->T.get(), *S->H, 0);
		RulesExpect(P.Shake >= 0.0 && P.Terms.AllIn && !P.Terms.CallAllIn, "last aggressor: all in counts, it wasn't an all-in call");
	}
}

// ------------------------------------------------------------------ the read's direction

struct RulesTally
{
	int Rolls = 0;
	int Seen = 0;    // the roll came under the chance
	int Changed = 0; // the decision changed
	int Wrong = 0;   // changed when it shouldn't have, or not when it should, or the wrong way
};

/**
 * AutoApplyRead at the spot over 400 ticks (each its own roll): Expect is what a reader who sees it does to D (Fold to Call,
 * Call to Fold, or nothing). The roll and the chance are checked against ReadHero's own formulas, written out here.
 */
RulesTally RulesReadAt(RulesSpot& S, const std::string& Reader, double Tilt, ss::PlayerAction::Kind Kind, double Equity, ss::PlayerAction::Kind Expect)
{
	RulesTally Out;
	const int Seat = RulesSeatOf(*S.H, Reader);
	const auto It = S.M.HeroRead.find(Reader);
	const double Read = It == S.M.HeroRead.end() ? 0.0 : It->second;
	for (int Tick = 0; Tick < 400; ++Tick)
	{
		S.T->Tick = Tick;
		const rules::AutoPlanHeart Heart = rules::AutoPlanHeartAt(S.M, Tilt, S.T.get(), *S.H, 0);
		const double Roll = ss::style::Mix(S.T->SeedHash, Tick, Seat, static_cast<int>(S.H->History.size()), 0x52454144u);
		const double Chance = std::min(0.9, Heart.Shake * std::fabs(Read) * 1.4);
		const bool Sees = std::fabs(Read) >= 0.05 && Heart.Shake >= 0.08 && Roll <= Chance;
		ss::BotDecision D;
		D.Action.Type = Kind;
		D.Equity = Equity;
		rules::AutoReadNote N;
		const bool Changed = rules::AutoApplyRead(S.M, Tilt, *S.T, *S.H, Seat, D, &N);
		++Out.Rolls;
		Out.Seen += Sees ? 1 : 0;
		Out.Changed += Changed ? 1 : 0;
		const bool Want = Sees && Expect != Kind;
		const bool NoteOk = !Changed || (N.ReaderId == Reader && N.Tick == Tick && RulesNear(N.Shake, Heart.Shake) && N.Bluffing == Heart.Bluff &&
											N.Called == (D.Action.Type == ss::PlayerAction::Kind::Call) && !N.Reader.empty() && N.Reader.find("npc:") == std::string::npos);
		Out.Wrong += Changed != Want || (Changed && D.Action.Type != Expect) || !NoteOk || !RulesNear(rules::AutoReadRoll(*S.T, *S.H, Seat), Roll) ? 1 : 0;
	}
	return Out;
}

void RulesReadDirection()
{
	using Kind = ss::PlayerAction::Kind;
	// A bluff: 7-2 on an ace-king-queen flop, bet into two; a bet with the goods: aces with a set on an ace-high flop.
	const std::vector<std::string> Flop = {"r300", "c", "c", "k", "k", "r900"};
	std::unique_ptr<RulesSpot> Bluff = RulesDeal("7c2d", "AhKsQd3c9h", 0.9, -0.6);
	std::unique_ptr<RulesSpot> Value = RulesDeal("AcAd", "As7h2c3d9s", 0.9, -0.6);
	RulesExpect(RulesPlay(*Bluff->H, Flop) && RulesPlay(*Value->H, Flop), "read: the flop bets play");
	const rules::AutoPlanHeart Hb = rules::AutoPlanHeartAt(Bluff->M, 1.0, Bluff->T.get(), *Bluff->H, 0);
	const rules::AutoPlanHeart Hv = rules::AutoPlanHeartAt(Value->M, 1.0, Value->T.get(), *Value->H, 0);
	RulesExpect(Hb.Bluff && Hb.Equity < 0.3 && !Hv.Bluff && Hv.Equity > 0.8, "read: 7-2 on A-K-Q is a bluff, a set of aces isn't");
	// The heart at the bet: pot 1,800 of 2,000, nothing to call, a reader watching, tilt 1; the bluff adds its 18.
	RulesExpect(RulesNear(Hb.Pressure, 6.0 + 18.0 + 18.0 + 8.0 + 20.0) && RulesNear(Hv.Pressure, 6.0 + 18.0 + 8.0 + 20.0), "read: the bet's heart, term by term");
	std::printf("read: bluff shake %.3f (bpm %.1f), value shake %.3f (bpm %.1f)\n", Hb.Shake, Hb.Bpm, Hv.Shake, Hv.Bpm);
	const std::string A = Bluff->A; // reads at 0.9: sharp
	const std::string B = Bluff->B; // reads at -0.6: nerves look like a bluff
	struct Case
	{
		const char* What;
		RulesSpot* S;
		bool bSharp;
		Kind From;
		double Equity;
		Kind To;
		bool bFires; // the case must change some decisions (else it must change none)
	};
	const Case Cases[] = {
		{"a sharp reader calls the plan's bluff (fold to call)", Bluff.get(), true, Kind::Fold, 0.3, Kind::Call, true},
		{"a sharp reader never folds to a bluff (a call stays)", Bluff.get(), true, Kind::Call, 0.3, Kind::Call, false},
		{"a sharp reader folds to the goods (call to fold)", Value.get(), true, Kind::Call, 0.3, Kind::Fold, true},
		{"a sharp reader never calls the goods off a read (a fold stays)", Value.get(), true, Kind::Fold, 0.3, Kind::Fold, false},
		{"a nervous read takes the goods for a bluff (fold to call)", Value.get(), false, Kind::Fold, 0.3, Kind::Call, true},
		{"a nervous read takes a bluff for a bluff (fold to call)", Bluff.get(), false, Kind::Fold, 0.3, Kind::Call, true},
		{"a nervous read never folds a call", Value.get(), false, Kind::Call, 0.3, Kind::Call, false},
		{"read as weak, a fold with 12% or less stays folded", Bluff.get(), true, Kind::Fold, 0.12, Kind::Fold, false},
		{"read as weak, a fold with 13% calls", Bluff.get(), true, Kind::Fold, 0.13, Kind::Call, true},
		{"read as strong, a call with 60% or more stays", Value.get(), true, Kind::Call, 0.6, Kind::Call, false},
		{"read as strong, a call with 59% folds", Value.get(), true, Kind::Call, 0.59, Kind::Fold, true},
		{"a raise is never read into anything", Bluff.get(), true, Kind::Raise, 0.3, Kind::Raise, false},
	};
	for (const Case& C : Cases)
	{
		const RulesTally T = RulesReadAt(*C.S, C.bSharp ? A : B, 1.0, C.From, C.Equity, C.To);
		const bool Fires = C.bFires ? (T.Changed > 0 && T.Changed < T.Rolls) : T.Changed == 0;
		RulesExpect(T.Wrong == 0 && Fires && T.Seen > 0, std::string("read: ") + C.What + " (" + std::to_string(T.Changed) + " of " + std::to_string(T.Rolls) + ")");
	}
	// The chance itself: the shakier and the sharper, the more often; never past 0.9.
	RulesExpect(RulesNear(rules::AutoReadChance(0.5, 0.6), 0.42) && RulesNear(rules::AutoReadChance(1.0, -1.0), 0.9) && RulesNear(rules::AutoReadChance(0.0, 1.0), 0.0),
		"read: chance = shake x |read| x 1.4, at most 0.9");
	RulesExpect(rules::AutoReadsWeak(0.9, true) && !rules::AutoReadsWeak(0.9, false) && rules::AutoReadsWeak(-0.4, true) && rules::AutoReadsWeak(-0.4, false),
		"read: sharp eyes read it right; the rest take nerves for a bluff");
	RulesExpect(rules::AutoReadMinShake == 0.08 && rules::AutoReadMinEye == 0.05, "read: the least shake read and the least eye that reads (ReadHero's)");
	// Who never reads: a stranger, a read under 0.05, a player not facing a bet, and nobody when the hands barely move.
	{
		std::unique_ptr<RulesSpot> S = RulesDeal("7c2d", "AhKsQd3c9h", 0.04, 0.0);
		RulesExpect(RulesPlay(*S->H, Flop), "read: the quiet spot plays");
		RulesExpect(RulesReadAt(*S, S->A, 1.0, Kind::Fold, 0.3, Kind::Fold).Changed == 0, "read: a read under 0.05 sees nothing");
		RulesExpect(RulesReadAt(*S, S->B, 1.0, Kind::Fold, 0.3, Kind::Fold).Changed == 0, "read: a stranger (not in the book of readers) sees nothing");
		RulesExpect(!rules::AutoPlanHeartAt(S->M, 1.0, S->T.get(), *S->H, 0).Terms.Watched, "read: with no reader at the table, nobody's watching");
	}
	{
		std::unique_ptr<RulesSpot> S = RulesDeal("AcAd", "As7h2c3d9s", 0.9, 0.9);
		S->M.RestBpm = 48.0;
		S->M.Sensitivity = 0.65;
		RulesExpect(RulesPlay(*S->H, {"r220"}), "read: the small raise plays");
		const rules::AutoPlanHeart Calm = rules::AutoPlanHeartAt(S->M, 0.0, S->T.get(), *S->H, 0);
		RulesExpect(Calm.Shake < 0.08 && RulesReadAt(*S, S->A, 0.0, Kind::Call, 0.3, Kind::Call).Changed == 0, "read: steady hands give nothing away");
	}
	{
		std::unique_ptr<RulesSpot> S = RulesDeal("7c2d", "AhKsQd3c9h", 0.9, 0.9);
		RulesExpect(RulesPlay(*S->H, {"r300", "c", "c"}), "read: the called raise plays");
		// The flop: the small blind to act, nothing bet yet.
		RulesExpect(RulesReadAt(*S, S->A, 1.0, Kind::Fold, 0.3, Kind::Fold).Changed == 0, "read: nobody facing a bet, nobody reads");
		// The plan's flop bet, with the reader's chips already level with it (as if matched): there's nothing to face, so no
		// read, however shaky the bet (ReadHero's "to call" guard; the last-aggressor rule alone wouldn't stop it).
		std::unique_ptr<RulesSpot> L = RulesDeal("7c2d", "AhKsQd3c9h", 0.9, 0.9);
		RulesExpect(RulesPlay(*L->H, Flop), "read: the level spot plays");
		ss::HandSeat& Reader = L->H->Seats[static_cast<size_t>(RulesSeatOf(*L->H, L->A))];
		Reader.StreetBet = L->H->CurrentBet;
		RulesExpect(rules::AutoPlanHeartAt(L->M, 1.0, L->T.get(), *L->H, 0).Shake > 0.5 && RulesReadAt(*L, L->A, 1.0, Kind::Fold, 0.3, Kind::Fold).Changed == 0,
			"read: a reader with nothing to call reads nothing");
	}
}

// ------------------------------------------------------------------ watch == skip with the read on (ue-auto's harness)

rules::AutoReadModel RulesModelFor(const ss::Tournament& T, bool bEveryone)
{
	rules::AutoReadModel M;
	M.RestBpm = 74.0;
	M.Sensitivity = 1.0;
	for (const ss::TPlayer& P : T.Players)
	{
		if (P.IsHero)
		{
			continue;
		}
		double R = 0.0;
		for (size_t I = 0; I < sizeof(RulesCast) / sizeof(RulesCast[0]); ++I)
		{
			R = P.Name == RulesCast[I] ? RulesCastRead[I] : R;
		}
		if (bEveryone && R == 0.0)
		{
			R = (P.Id.size() % 2) ? 0.85 : -0.7;
		}
		if (std::fabs(R) >= 0.05)
		{
			M.HeroRead[P.Id] = R;
		}
	}
	return M;
}

struct RulesNight
{
	std::unique_ptr<ss::Tournament> T;
	rules::AutoReadModel Model;
	autoplay::TableAuto Plan;
	autoplay::Run Run;
	style::HeroBrain Brain;
	style::Learned Book;
	int Reads = 0;
	int BadReads = 0;
	void Refresh() { Brain = autoplay::BrainFor(Plan, Run, 1.0, 1, nullptr, &Book); }
	void Hook(bool bOn)
	{
		if (!bOn)
		{
			T->HeroReadHook = nullptr;
			return;
		}
		T->HeroReadHook = [this](const ss::Hand& H, int Seat, ss::BotDecision& D) {
			const ss::BotDecision Before = D;
			ss::BotDecision Again = D;
			rules::AutoReadNote N;
			const bool Changed = rules::AutoApplyRead(Model, Run.Tilt, *T, H, Seat, D, &N);
			const bool Twice = rules::AutoApplyRead(Model, Run.Tilt, *T, H, Seat, Again, nullptr);
			if (Changed != Twice || Again.Action.Type != D.Action.Type)
			{
				++BadReads; // the same inputs read the same way
			}
			if (!Changed)
			{
				return;
			}
			++Reads;
			// What a read may do: only to a player who reads hands, facing the plan's bet of this street.
			const int Last = rules::AutoLastAggression(H);
			const int Hero = rules::AutoHeroIndex(H);
			const bool HeroLast = Last >= 0 && Hero >= 0 && H.History[static_cast<size_t>(Last)].Seat == H.Seats[static_cast<size_t>(Hero)].Seat;
			const bool Facing = H.CurrentBet - H.Seats[static_cast<size_t>(Seat)].StreetBet > 0;
			const auto R = Model.HeroRead.find(H.Seats[static_cast<size_t>(Seat)].Id);
			const bool Eyes = R != Model.HeroRead.end() && std::fabs(R->second) >= 0.05;
			const bool Flip = (Before.Action.Type == ss::PlayerAction::Kind::Fold && D.Action.Type == ss::PlayerAction::Kind::Call) ||
				(Before.Action.Type == ss::PlayerAction::Kind::Call && D.Action.Type == ss::PlayerAction::Kind::Fold);
			if (!HeroLast || !Facing || !Eyes || !Flip || N.Reader.empty() || N.ReaderId != H.Seats[static_cast<size_t>(Seat)].Id)
			{
				++BadReads;
			}
		};
	}
};

std::unique_ptr<RulesNight> RulesMakeNight(style::Style S, const std::string& Salt, bool bReads, bool bEveryone)
{
	std::unique_ptr<RulesNight> N(new RulesNight());
	N->T = RulesField(Salt, 8);
	N->Model = RulesModelFor(*N->T, bEveryone);
	N->Plan.S = S;
	N->Plan.M = autoplay::Mode::Watch;
	N->Plan.Stops = 0;
	N->Run.Sharp = 0.80;
	N->Book.Tells["Mei"] = 2;
	N->Refresh();
	N->Hook(bReads);
	return N;
}

std::string RulesState(const RulesNight& N)
{
	return rules::AutoParityState(*N.T, N.Plan, N.Run);
}

/** The actor's watch path, headless: one tick with the plan in the hero's seat, as ABackRoomTable plays it. */
bool RulesWatchTick(RulesNight& N)
{
	std::vector<ss::TEvent> Start;
	std::unique_ptr<ss::Hand> H = N.T->StartTick(Start);
	if (H)
	{
		int Guard = 0;
		while (!H->bComplete && ++Guard < 1000)
		{
			const int Idx = H->ToAct;
			const bool Hero = H->Seats[static_cast<size_t>(Idx)].Id == ss::HeroId;
			const ss::BotDecision D = Hero ? N.T->HeroDecisionFor(*H, N.Brain) : N.T->BotDecisionFor(*H, false);
			if (!H->Act(D.Action))
			{
				const ss::LegalActions L = H->GetLegalActions();
				H->Act(L.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Fold());
			}
			N.Run.AiDecisions += Hero ? 1 : 0;
		}
	}
	N.T->FinishTickWith(H.get(), N.Brain);
	if (H)
	{
		autoplay::NoteHand(N.Run, *H, rules::AutoHeroIndex(*H), true, 1.0);
		N.Refresh();
		return true;
	}
	return false;
}

struct RulesSink : ss::live::SkipSink
{
	RulesNight* N = nullptr;
	int Hands = 0;
	int LastNumber = 0;
	std::vector<ss::recall::Record> Kept;
	void HandDone(const ss::Hand& H, int HeroSeat) override
	{
		autoplay::NoteSkippedHand(N->Run, H, HeroSeat, 1.0);
		N->Refresh();
		++Hands;
		// The skipped hand's leftovers: the record (numbered on from the page), the showdown memory, the tablemates.
		ss::recall::Record Rec = rules::AutoSkippedRecord(*N->T, H, HeroSeat, rules::AutoNextHandNumber(Kept, LastNumber + 1));
		RulesExpect(Rec.Mine == H.Seats[static_cast<size_t>(HeroSeat)].Hole, "skipped record: the player's own cards");
		bool Named = true;
		for (const ss::recall::Line& L : Rec.Lines)
		{
			Named = Named && L.Who.find("npc:") == std::string::npos && L.Who != ss::HeroId;
		}
		RulesExpect(Named, "skipped record: no engine id read out");
		LastNumber = Rec.Number;
		Kept.push_back(Rec);
		std::string Id;
		ss::Chips Net = 0, Pot = 0;
		if (rules::AutoShowdownAcross(H, HeroSeat, Id, Net, Pot))
		{
			int Them = -1;
			for (size_t I = 0; I < H.Seats.size(); ++I)
			{
				Them = H.Seats[I].Id == Id ? static_cast<int>(I) : Them;
			}
			RulesExpect(Them >= 0 && Them != HeroSeat && (H.Seats[static_cast<size_t>(Them)].Stack - H.Seats[static_cast<size_t>(Them)].StartStack) * Net < 0 && Pot > 0,
				"showdown across: an opponent on the other side of the player's result");
		}
		RulesExpect(rules::AutoTablemates(H, HeroSeat).size() + 1 == H.Seats.size(), "tablemates: everyone at the table but the player");
	}
	void Events(const std::vector<ss::TEvent>& E) override { (void)E; }
	void Checkpoint(int Tick) override { (void)Tick; }
};

/** The skip path over the same hands: live::SkipSlice to Hands, one hand a slice when States wants the state after each. */
void RulesSkipHands(RulesNight& N, int Hands, std::vector<std::string>* States)
{
	RulesSink S;
	S.N = &N;
	int Left = Hands;
	for (int Guard = 0; Guard < 10000 && Left > 0 && !N.T->Hero().Busted && !N.T->bFinished; ++Guard)
	{
		autoplay::TableAuto A = N.Plan;
		A.M = autoplay::Mode::Skip;
		A.To = autoplay::Target::Hands;
		A.Hands = States ? 1 : Left;
		const ss::live::SkipOut O = ss::live::SkipSlice(*N.T, N.Brain, A, 1.0e9, S);
		Left -= O.Hands;
		if (States && O.Hands > 0)
		{
			States->push_back(RulesState(N));
		}
		if (O.Ticks == 0)
		{
			break;
		}
	}
}

void RulesWatchEqualsSkip()
{
	const auto T0 = std::chrono::steady_clock::now();
	const style::Style Styles[] = {style::Style::Gto, style::Style::Lag, style::Style::Exploit, style::Style::Maniac, style::Style::Autopilot, style::Style::Nit};
	int TotalReads = 0;
	for (const style::Style S : Styles)
	{
		const std::string Salt = std::string("parity:") + style::InfoOf(S).Id;
		std::unique_ptr<RulesNight> Watch = RulesMakeNight(S, Salt, true, true);
		std::unique_ptr<RulesNight> Skip = RulesMakeNight(S, Salt, true, true);
		RulesExpect(RulesState(*Watch) == RulesState(*Skip), "parity: the two nights start alike");
		std::vector<std::string> Watched;
		int Hands = 0;
		for (int K = 0; K < 300 && Hands < 80 && !Watch->T->Hero().Busted && !Watch->T->bFinished; ++K)
		{
			if (RulesWatchTick(*Watch))
			{
				++Hands;
				Watched.push_back(RulesState(*Watch));
			}
		}
		std::vector<std::string> Skipped;
		RulesSkipHands(*Skip, Hands, &Skipped);
		size_t D = 0;
		while (D < Watched.size() && D < Skipped.size() && Watched[D] == Skipped[D])
		{
			++D;
		}
		RulesExpect(Hands > 20 && Watched == Skipped, std::string("parity: watch == skip with the read on, hand by hand (") + style::InfoOf(S).Id + ")");
		if (Watched != Skipped)
		{
			std::printf("  first difference at hand %d of %d/%d\n", static_cast<int>(D + 1), static_cast<int>(Watched.size()), static_cast<int>(Skipped.size()));
		}
		RulesExpect(Watch->Reads == Skip->Reads, std::string("parity: the same reads on both paths (") + style::InfoOf(S).Id + ")");
		RulesExpect(Watch->BadReads == 0 && Skip->BadReads == 0, std::string("parity: every read is one the rules allow, and repeatable (") + style::InfoOf(S).Id + ")");
		TotalReads += Watch->Reads;
	}
	// The read changes the night: the same seed with the hook off plays differently once a read lands.
	{
		std::unique_ptr<RulesNight> On = RulesMakeNight(style::Style::Lag, "effect", true, true);
		std::unique_ptr<RulesNight> Off = RulesMakeNight(style::Style::Lag, "effect", false, true);
		RulesSkipHands(*On, 120, nullptr);
		RulesSkipHands(*Off, 120, nullptr);
		RulesExpect(On->Reads > 0 && RulesState(*On) != RulesState(*Off), "parity: a read the room takes changes what happens next");
		// The hook is live: reads land over the six parity nights or this one (plans at the default Sharpness slip into
		// passive play and bluff less since C1's calibration, so 80 hands a style can pass without one).
		RulesExpect(TotalReads + On->Reads > 0, "parity: the room reads the plan at all (the hook is live)");
	}
	// Strangers don't read: with only the cast's own reads, no more reads than with everyone reading.
	{
		std::unique_ptr<RulesNight> CastOnly = RulesMakeNight(style::Style::Lag, "cast", true, false);
		std::unique_ptr<RulesNight> Everyone = RulesMakeNight(style::Style::Lag, "cast", true, true);
		RulesSkipHands(*CastOnly, 120, nullptr);
		RulesSkipHands(*Everyone, 120, nullptr);
		RulesExpect(CastOnly->Reads <= Everyone->Reads && CastOnly->BadReads == 0, "parity: only players who read hands read the plan");
	}
	std::printf("watch == skip with the read on: %d reads over six styles, %.1f s\n", TotalReads, std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count());
}

// ------------------------------------------------------------------ the host text, and the small rules

void RulesHostText()
{
	std::unique_ptr<RulesNight> N = RulesMakeNight(style::Style::Exploit, "host", false, false);
	RulesSkipHands(*N, 25, nullptr);
	N->Plan.M = autoplay::Mode::Skip;
	N->Plan.Speed = 4;
	N->Plan.To = autoplay::Target::FinalTable;
	N->Plan.Stops = autoplay::StopBubble | autoplay::StopFinalTable;
	N->Book.Tells["Sal"] = 1;
	N->Book.Tells["Big Lou"] = 3;
	const std::string Text = rules::AutoHostText(N->Plan, N->Run, N->Book, true);
	autoplay::TableAuto Plan;
	autoplay::Run Run;
	style::Learned Book;
	int Lines = 0, Runs = 0, Seen = 0, Read = 0;
	size_t Start = 0;
	while (Start < Text.size())
	{
		size_t End = Text.find('\n', Start);
		End = End == std::string::npos ? Text.size() : End;
		std::vector<std::string> F;
		size_t A = Start;
		while (A <= End)
		{
			size_t B = Text.find('\t', A);
			B = B == std::string::npos || B > End ? End : B;
			F.push_back(Text.substr(A, B - A));
			A = B + 1;
		}
		const rules::AutoHostLine Kind = rules::AutoReadHostLine(F, Plan, Run, Book);
		++Lines;
		Runs += Kind == rules::AutoHostLine::Run ? 1 : 0;
		Seen += Kind == rules::AutoHostLine::Seen ? 1 : 0;
		Read += Kind == rules::AutoHostLine::Read ? 1 : 0;
		Start = End + 1;
	}
	RulesExpect(Runs == 1 && Seen == static_cast<int>(N->Run.Seen.size()) && Read == static_cast<int>(N->Book.Tells.size()) && Lines == Runs + Seen + Read,
		"host text: one auto line, an autoseen line a player, an autoread line a name");
	RulesExpect(autoplay::RunText(Plan, Run) == autoplay::RunText(N->Plan, N->Run), "host text: the plan and its run come back exactly");
	bool SeenSame = Run.Seen.size() == N->Run.Seen.size();
	for (const auto& Kv : N->Run.Seen)
	{
		const auto It = Run.Seen.find(Kv.first);
		SeenSame = SeenSame && It != Run.Seen.end() && ss::reads::Write(It->second) == ss::reads::Write(Kv.second);
	}
	RulesExpect(SeenSame && !Run.Seen.empty(), "host text: what was observed tonight comes back exactly");
	RulesExpect(Book.Tells == N->Book.Tells, "host text: the read book the night seated with comes back");
	RulesExpect(rules::AutoHostText(Plan, Run, Book, true) == Text, "host text: written again, the same text");
	const std::string NoRun = rules::AutoHostText(N->Plan, N->Run, N->Book, false);
	RulesExpect(NoRun.find("auto\t") == std::string::npos && NoRun.find("autoseen\t") != std::string::npos, "host text: no auto line until the plan plays");
	autoplay::TableAuto P2;
	autoplay::Run R2;
	style::Learned B2;
	RulesExpect(rules::AutoReadHostLine({"hands", "12"}, P2, R2, B2) == rules::AutoHostLine::None && rules::AutoReadHostLine({"auto", "gto"}, P2, R2, B2) == rules::AutoHostLine::None,
		"host text: other keys and short lines are left alone");
}

void RulesSmall()
{
	// The learned book: two sightings or more is learned; counted by name.
	const std::map<std::string, int> Reads = {{"Sal/ChipGlance", 2}, {"Sal/NeckTouch", 3}, {"Mei/Sigh", 1}, {"Big Lou/RealSmile", 2}, {"/Odd", 4}};
	const style::Learned L = rules::AutoLearnedOf(Reads);
	RulesExpect(L.Tells.size() == 2 && L.Tells.at("Sal") == 2 && L.Tells.at("Big Lou") == 1, "learned book: names with 2+ sightings, tells counted");
	// Dee's clock: by the hand under the plan, real time otherwise; 24 hands move the night 24 x the minutes a hand.
	double Pending = 4.5;
	RulesExpect(rules::AutoDeeClock(true, 0.3, Pending) == 4.5 && Pending == 0.0, "Dee's clock: the plan's hand moves it, real time doesn't");
	RulesExpect(rules::AutoDeeClock(true, 0.3, Pending) == 0.0, "Dee's clock: nothing pending, nothing moves under the plan");
	Pending = 0.0;
	RulesExpect(std::fabs(rules::AutoDeeClock(false, 0.3, Pending) - 0.3) < 1e-12, "Dee's clock: the player's hands run on real time");
	double Clock = 0.0;
	for (int H = 0; H < 24; ++H)
	{
		Pending += autoplay::DeeMinutesPerHand;
		for (int F = 0; F < 37; ++F)
		{
			Clock += rules::AutoDeeClock(true, 0.05, Pending);
		}
	}
	RulesExpect(std::fabs(Clock - 24.0 * autoplay::DeeMinutesPerHand) < 1e-9, "Dee's clock: 24 hands move the night 24 x the minutes a hand");
	// The skip card's line.
	std::unique_ptr<ss::Tournament> T = RulesField("card", 1);
	autoplay::TableAuto A;
	A.S = style::Style::PushFold;
	A.To = autoplay::Target::Bubble;
	const std::string Line = rules::AutoSkipCardLine(*T, A);
	RulesExpect(Line.find("Level 1") == 0 && Line.find("left of 60") != std::string::npos && Line.find(" BB") != std::string::npos && Line.find("to the bubble") != std::string::npos,
		"skip card: level, the field, your stack in big blinds, the target");
	A.To = autoplay::Target::Hands;
	A.Hands = 37;
	RulesExpect(rules::AutoSkipCardLine(*T, A).find("37 hands to go") != std::string::npos, "skip card: hands to go");
	RulesExpect(rules::AutoHash("a") != rules::AutoHash("b") && rules::AutoHex(rules::AutoHash("")).size() == 16 && rules::AutoHex(rules::AutoHash("")) == "cbf29ce484222325",
		"parity's hash: FNV-1a, 16 hex digits");
	// The HANDS page's numbers: the table's own count until a skip; the skip's hands carry it on; the table's next hand, whose
	// own count is behind, takes the next free number; a night come back to (its count from 1 again) carries on too.
	std::vector<ss::recall::Record> Kept;
	auto Keep = [&Kept](int Wanted) {
		ss::recall::Record R;
		R.Number = rules::AutoNextHandNumber(Kept, Wanted);
		Kept.push_back(R);
		if (Kept.size() > 10)
		{
			Kept.erase(Kept.begin());
		}
		return R.Number;
	};
	bool Plain = true;
	for (int Hand = 1; Hand <= 7; ++Hand)
	{
		Plain = Plain && Keep(Hand) == Hand;
	}
	RulesExpect(Plain, "hands page: with no skip, the table's own numbers");
	const int SkipFirst = Keep(8); // the skip starts after hand 7
	int SkipLast = SkipFirst;
	for (int K = 1; K < 5; ++K)
	{
		SkipLast = Keep(SkipLast + 1);
	}
	const int TableNext = Keep(8); // the table deals its 8th hand after the skip
	const int Resumed = Keep(1);   // a night come back to: the table counts from 1 again
	bool Unique = true;
	for (size_t I = 0; I < Kept.size(); ++I)
	{
		for (size_t J = I + 1; J < Kept.size(); ++J)
		{
			Unique = Unique && Kept[I].Number != Kept[J].Number;
		}
	}
	RulesExpect(SkipFirst == 8 && SkipLast == 12 && TableNext == 13 && Resumed == 14 && Unique, "hands page: a skip's hands carry the count on and no number repeats");
}

int Run()
{
	RulesFailures = 0;
	RulesChecks = 0;
	const auto T0 = std::chrono::steady_clock::now();
	RulesHeartTerms();
	RulesBluffLine();
	RulesLastAggressor();
	RulesReadDirection();
	RulesWatchEqualsSkip();
	RulesHostText();
	RulesSmall();
	std::printf("autoplay rules (BackRoomAutoRules.h): %d checks, %d failed, %.1f s\n", RulesChecks, RulesFailures,
		std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count());
	return RulesFailures;
}
} // namespace room_seg_autorules

#ifdef SS_AUTORULES_MAIN
int main()
{
	return room_seg_autorules::Run() == 0 ? 0 : 1;
}
#endif
