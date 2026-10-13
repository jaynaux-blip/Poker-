// card_room: core-autoplay's additions (update 2 SPEC §6.7, §2.12): LiveSkipSliceEqualsTicks (live::SkipSlice in slices
// of any budget gives the same checkpoints as stepping the night tick by tick, to every target), and RoomPace with the
// plan in the seat at 1x, 4x and 8x (the cue log by hand is the same: no frame clock), and Dee's per-hand clock against
// the manual rate. Compiled into room_test and called from main() in room_main.cpp, as tests/room_field.cpp is.
// core-autoplay's file; its helpers live in room_seg_auto's own namespace.
#include "ShortStack/AI/Style.h"
#include "ShortStack/Cards.h"
#include "ShortStack/Game/Autoplay.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/LiveAuto.h"
#include "ShortStack/Game/Room.h"
#include "ShortStack/Hand.h"
#include "ShortStack/Tournament.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

namespace room_seg_auto
{
namespace autoplay = ss::autoplay;
namespace style = ss::style;

int RoomAutoFailures = 0;

void RoomAutoExpect(bool Condition, const std::string& What)
{
	if (!Condition)
	{
		++RoomAutoFailures;
		std::printf("FAILED: %s\n", What.c_str());
	}
}

/** One night as the card room would have it: the occurrence, the entry (a roster of voices), the seed, the known faces. */
struct RoomAutoNight
{
	ss::live::Occurrence O;
	ss::life::LiveEntry E;
	std::string Seed;
	std::string Hero = "Jesse";
	std::vector<ss::ReservedPlayer> Known;
	int Entrants = 60;
	int HeroStacks = 1;
};

RoomAutoNight RoomAutoMake(const std::string& OccurrenceId, int Entrants, const std::string& Salt)
{
	RoomAutoNight N;
	N.O = ss::live::FindOccurrence(OccurrenceId);
	N.Entrants = Entrants;
	N.E.Id = N.O.Id;
	N.E.Name = N.O.Valid() ? N.O.T->Name : std::string("Embercrest Nightly");
	N.E.Entrants = Entrants;
	N.E.RegisteredAt = N.O.DeskOpens + 25.0;
	static const char* const Names[] = {"Mei", "Rick", "Dre", "Sal", "Twitch", "Boots", "T.J.", "Maria G.", "Omar", "Lin", "Duke", "Bev"};
	static const ss::Archetype Styles[] = {ss::Archetype::Reg, ss::Archetype::Lag, ss::Archetype::Station, ss::Archetype::Nit, ss::Archetype::Maniac, ss::Archetype::Tag};
	int K = 0;
	for (const char* Name : Names)
	{
		ss::life::LiveVoice V;
		V.Name = Name;
		V.Style = static_cast<int>(Styles[K % 6]);
		V.Social = 0.2f + 0.06f * static_cast<float>(K % 9);
		V.Aggro = 0.3f + 0.05f * static_cast<float>(K % 7);
		V.Age = 30 + K * 3;
		N.E.Voices.push_back(V);
		N.E.Roster.push_back({V.Name, V.Style});
		N.Known.push_back({V.Name, Styles[K % 6]});
		++K;
	}
	N.Seed = ss::live::SeedFor(N.E, N.Hero) + Salt;
	return N;
}

std::unique_ptr<ss::Tournament> RoomAutoField(const RoomAutoNight& N)
{
	std::unique_ptr<ss::Tournament> T = ss::live::MakeField(N.O, N.Entrants, N.Hero, N.Seed, N.Known);
	// A deep-stacked player (HeroStacks starting stacks) lives to see the bubble and the final table the skips stop at.
	T->Players[static_cast<size_t>(T->PlayerIndex(ss::HeroId))].Stack *= N.HeroStacks;
	return T;
}

/** A hand as text: who sat where with what, every action, the board, the pots. */
std::string RoomAutoHandText(const ss::Hand& H)
{
	std::string Out;
	for (const ss::HandSeat& S : H.Seats)
	{
		Out += S.Id + "@" + std::to_string(S.Seat) + ":" + std::to_string(S.StartStack) + ">" + std::to_string(S.Stack) + " ";
	}
	for (const ss::ActionRecord& A : H.History)
	{
		Out += std::to_string(A.Seat) + "/" + std::to_string(static_cast<int>(A.Type)) + "/" + std::to_string(A.To) + " ";
	}
	for (const ss::Card C : H.Board)
	{
		Out += ss::CardToString(C);
	}
	return Out;
}

int RoomAutoHeroSeat(const ss::Hand& H)
{
	for (size_t I = 0; I < H.Seats.size(); ++I)
	{
		if (H.Seats[I].Id == ss::HeroId)
		{
			return static_cast<int>(I);
		}
	}
	return -1;
}

/** The host's sink, as FAutoHost's would be: the plan's tilt, counts and observed stats after each hand (NoteSkippedHand), then the
 *  brain rebuilt from them; the tick's events and the checkpoints logged. */
struct RoomAutoSink : ss::live::SkipSink
{
	ss::Tournament* T = nullptr;
	autoplay::Run* Run = nullptr;
	style::HeroBrain* Brain = nullptr;
	const autoplay::TableAuto* Plan = nullptr;
	std::vector<std::string> Log;
	void HandDone(const ss::Hand& H, int HeroSeat) override
	{
		autoplay::NoteSkippedHand(*Run, H, HeroSeat, 1.0);
		*Brain = autoplay::BrainFor(*Plan, *Run, 1.0, 1, nullptr, nullptr);
		Log.push_back("H " + RoomAutoHandText(H));
	}
	void Events(const std::vector<ss::TEvent>& E) override
	{
		std::string Line = "E";
		for (const ss::TEvent& Ev : E)
		{
			Line += " " + std::to_string(static_cast<int>(Ev.Type));
		}
		Log.push_back(Line);
	}
	void Checkpoint(int Tick) override { Log.push_back("C " + std::to_string(Tick) + " " + T->Checkpoint() + " " + autoplay::RunText(*Plan, *Run)); }
};

struct RoomAutoSkip
{
	std::vector<std::string> Log;
	ss::live::SkipWhy Why = ss::live::SkipWhy::Budget;
	int Ticks = 0;
	int Slices = 0;
};

/** The night skipped from its first hand with the plan, in slices of Budget ms (0: tick by tick), to its target or stop. */
RoomAutoSkip RoomAutoSkipNight(const RoomAutoNight& N, const autoplay::TableAuto& Plan, double Budget)
{
	RoomAutoSkip Out;
	std::unique_ptr<ss::Tournament> T = RoomAutoField(N);
	autoplay::Run Run;
	style::HeroBrain Brain = autoplay::BrainFor(Plan, Run, 1.0, 1, nullptr, nullptr);
	RoomAutoSink Sink;
	Sink.T = T.get();
	Sink.Run = &Run;
	Sink.Brain = &Brain;
	Sink.Plan = &Plan;
	autoplay::TableAuto A = Plan;
	for (int Guard = 0; Guard < 100000; ++Guard)
	{
		const ss::live::SkipOut O = ss::live::SkipSlice(*T, Brain, A, Budget, Sink);
		++Out.Slices;
		Out.Ticks += O.Ticks;
		A.Hands -= O.Hands;
		if (O.Why != ss::live::SkipWhy::Budget)
		{
			Out.Why = O.Why;
			break;
		}
	}
	Out.Log = Sink.Log;
	return Out;
}

/** The same night stepped by hand, a tick at a time, with the same calls: the reference SkipSlice must match. */
RoomAutoSkip RoomAutoStepNight(const RoomAutoNight& N, const autoplay::TableAuto& Plan, int Ticks)
{
	RoomAutoSkip Out;
	std::unique_ptr<ss::Tournament> T = RoomAutoField(N);
	autoplay::Run Run;
	style::HeroBrain Brain = autoplay::BrainFor(Plan, Run, 1.0, 1, nullptr, nullptr);
	RoomAutoSink Sink;
	Sink.T = T.get();
	Sink.Run = &Run;
	Sink.Brain = &Brain;
	Sink.Plan = &Plan;
	for (int K = 0; K < Ticks && !T->bFinished && !T->Hero().Busted; ++K)
	{
		const ss::Tournament::TickResult R = T->SimulateTickWith(Brain);
		if (const ss::Hand* Last = T->LastHeroHand())
		{
			Sink.HandDone(*Last, RoomAutoHeroSeat(*Last));
		}
		Sink.Events(R.Events);
		if (K + 1 == Ticks || T->Tick % 5 == 0)
		{
			Sink.Checkpoint(T->Tick);
		}
		++Out.Ticks;
	}
	Out.Log = Sink.Log;
	return Out;
}

const char* RoomAutoWhy(ss::live::SkipWhy W)
{
	switch (W)
	{
	case ss::live::SkipWhy::Budget: return "budget";
	case ss::live::SkipWhy::Target: return "target";
	case ss::live::SkipWhy::Bubble: return "bubble";
	case ss::live::SkipWhy::HandForHand: return "hand for hand";
	case ss::live::SkipWhy::FinalTable: return "final table";
	case ss::live::SkipWhy::Break: return "break";
	case ss::live::SkipWhy::Busted: return "busted";
	case ss::live::SkipWhy::Stopped: return "stopped";
	}
	return "?";
}

void LiveSkipSliceEqualsTicks()
{
	const auto T0 = std::chrono::steady_clock::now();
	RoomAutoNight N = RoomAutoMake("embercrest-nightly@15", 60, ":auto-skip");
	N.HeroStacks = 8;
	RoomAutoExpect(N.O.Valid(), "live skip: the Nightly occurrence exists");
	if (!N.O.Valid())
	{
		return;
	}
	struct Case
	{
		style::Style S;
		autoplay::Target To;
		int Hands;
		uint32_t Stops;
	};
	const Case Cases[] = {
		{style::Style::Gto, autoplay::Target::NextBreak, 0, 0},
		{style::Style::Lag, autoplay::Target::Bubble, 0, autoplay::StopBubble},
		{style::Style::Reg, autoplay::Target::Bubble, 0, 0},
		{style::Style::Tag, autoplay::Target::FinalTable, 0, 0},
		{style::Style::Nit, autoplay::Target::Result, 0, 0},
		{style::Style::Exploit, autoplay::Target::Hands, 23, 0},
	};
	for (const Case& C : Cases)
	{
		autoplay::TableAuto Plan;
		Plan.S = C.S;
		Plan.M = autoplay::Mode::Skip;
		Plan.To = C.To;
		Plan.Hands = C.Hands;
		Plan.Stops = C.Stops;
		const RoomAutoSkip Ticked = RoomAutoSkipNight(N, Plan, 0.0);
		const RoomAutoSkip Sliced = RoomAutoSkipNight(N, Plan, 0.4);
		const RoomAutoSkip Whole = RoomAutoSkipNight(N, Plan, 1.0e9);
		const RoomAutoSkip Stepped = RoomAutoStepNight(N, Plan, Ticked.Ticks);
		const std::string What = std::string(style::InfoOf(C.S).Id) + " to " + autoplay::TargetName(C.To);
		RoomAutoExpect(Ticked.Log == Sliced.Log && Ticked.Log == Whole.Log && Ticked.Why == Sliced.Why && Ticked.Why == Whole.Why,
			"live skip: slices of any budget play the same night (" + What + ")");
		RoomAutoExpect(Stepped.Log == Ticked.Log, "live skip: the same checkpoints as stepping the night tick by tick (" + What + ")");
		RoomAutoExpect(Ticked.Slices == Ticked.Ticks && Whole.Slices == 1, "live skip: a budget of 0 steps a tick a slice; a long one, one slice (" + What + ")");
		std::printf("live skip (%s): %d ticks, stopped at %s, %d slices at 0.4 ms\n", What.c_str(), Ticked.Ticks, RoomAutoWhy(Ticked.Why), Sliced.Slices);
		if (C.To == autoplay::Target::Hands)
		{
			int Dealt = 0;
			for (const std::string& L : Ticked.Log)
			{
				Dealt += L.rfind("H ", 0) == 0 ? 1 : 0;
			}
			RoomAutoExpect(Ticked.Why == ss::live::SkipWhy::Busted || (Ticked.Why == ss::live::SkipWhy::Target && Dealt == C.Hands), "live skip: Hands stops after that many hands");
		}
		if (C.To == autoplay::Target::NextBreak)
		{
			RoomAutoExpect(Ticked.Why == ss::live::SkipWhy::Break || Ticked.Why == ss::live::SkipWhy::Busted, "live skip: Next break stops at the break");
		}
		if (C.To == autoplay::Target::Bubble)
		{
			RoomAutoExpect(Ticked.Why == ss::live::SkipWhy::Bubble, "live skip: Bubble stops as the field comes within Paid + 15 (" + What + ")");
		}
		if (C.To == autoplay::Target::FinalTable)
		{
			RoomAutoExpect(Ticked.Why == ss::live::SkipWhy::FinalTable, "live skip: Final table stops after the tick that seats it (" + What + ")");
		}
		if (C.To == autoplay::Target::Result)
		{
			RoomAutoExpect(Ticked.Why == ss::live::SkipWhy::Busted || Ticked.Why == ss::live::SkipWhy::FinalTable || Ticked.Why == ss::live::SkipWhy::Target,
				"live skip: Result runs to the end, stopping at the final table (always, live)");
		}
		// Checkpoints every 5 ticks and at the end.
		std::vector<int> Checkpoints;
		for (const std::string& L : Ticked.Log)
		{
			if (L.rfind("C ", 0) == 0)
			{
				Checkpoints.push_back(std::atoi(L.c_str() + 2));
			}
		}
		bool Fives = !Checkpoints.empty();
		for (size_t K = 0; K + 1 < Checkpoints.size(); ++K)
		{
			Fives = Fives && Checkpoints[K] % 5 == 0;
		}
		RoomAutoExpect(Fives && static_cast<int>(Checkpoints.size()) >= Ticked.Ticks / 5 && Ticked.Log.back().rfind("C ", 0) == 0,
			"live skip: a checkpoint every 5 ticks and one at the end (" + What + ")");
	}
	// A target already behind the night ends the call at once; a plan that isn't skipping plays nothing.
	{
		std::unique_ptr<ss::Tournament> T = RoomAutoField(N);
		autoplay::Run Run;
		autoplay::TableAuto Plan;
		Plan.M = autoplay::Mode::Watch;
		style::HeroBrain Brain = autoplay::BrainFor(Plan, Run, 1.0, 1, nullptr, nullptr);
		RoomAutoSink Sink;
		Sink.T = T.get();
		Sink.Run = &Run;
		Sink.Brain = &Brain;
		Sink.Plan = &Plan;
		const ss::live::SkipOut Watch = ss::live::SkipSlice(*T, Brain, Plan, 10.0, Sink);
		Plan.M = autoplay::Mode::Skip;
		Plan.To = autoplay::Target::Hands;
		Plan.Hands = 0;
		const ss::live::SkipOut None = ss::live::SkipSlice(*T, Brain, Plan, 10.0, Sink);
		RoomAutoExpect(Watch.Why == ss::live::SkipWhy::Stopped && Watch.Ticks == 0 && None.Why == ss::live::SkipWhy::Target && None.Ticks == 0 && T->Tick == 0,
			"live skip: nothing played when the plan isn't skipping or the target's behind it");
	}
	std::printf("live skip: %.1f s\n", std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count());
}

// ------------------------------------------------------------------ RoomPace with the plan in the seat

/** The host's note of a hand (BackRoomTable.cpp's NoteHand, as room_main's harness writes it). */
ss::room::HandNote RoomAutoNote(const ss::Tournament& T, const ss::Hand& H, int Tick)
{
	ss::room::HandNote N;
	N.Hand = Tick;
	N.Board = H.Board;
	auto NameOf = [&T](const ss::HandSeat& S) -> std::string {
		if (S.Id == ss::HeroId)
		{
			return ss::HeroId;
		}
		const int I = T.PlayerIndex(S.Id);
		return I >= 0 ? T.Players[static_cast<size_t>(I)].Name : S.Id;
	};
	const ss::HandSeat* Mine = nullptr;
	for (const ss::HandSeat& S : H.Seats)
	{
		(S.Folded ? N.Folded : N.InHand).push_back(NameOf(S));
		if (S.StartStack > 0 && S.Stack <= 0)
		{
			N.Busted.push_back(NameOf(S));
		}
		Mine = S.Id == ss::HeroId ? &S : Mine;
	}
	N.HeroStack = Mine ? Mine->Stack : 0;
	if (H.LastAggressor >= 0 && H.LastAggressor < static_cast<int>(H.Seats.size()))
	{
		N.Aggressor = NameOf(H.Seats[static_cast<size_t>(H.LastAggressor)]);
	}
	for (const ss::ActionRecord& A : H.History)
	{
		if (A.AllIn && !N.AllIn)
		{
			N.AllIn = true;
			const size_t Seen = A.OnStreet == ss::Street::Preflop ? 0 : A.OnStreet == ss::Street::Flop ? 3 : A.OnStreet == ss::Street::Turn ? 4 : 5;
			N.BoardAtAllIn.assign(H.Board.begin(), H.Board.begin() + static_cast<std::ptrdiff_t>(std::min(Seen, H.Board.size())));
		}
	}
	bool Showdown = false;
	for (const ss::HandEvent& Ev : H.Events)
	{
		if (Ev.Type == ss::EventType::Showdown)
		{
			Showdown = true;
			for (const ss::ShowdownHand& SH : Ev.Hands)
			{
				const ss::HandSeat* HS = H.SeatByNumber(SH.Seat);
				N.Shown.push_back({HS ? NameOf(*HS) : std::to_string(SH.Seat), SH.Hole});
			}
		}
	}
	N.Called = N.AllIn && Showdown && N.Shown.size() >= 2;
	for (const ss::PotResult& R : H.PotResults)
	{
		N.Pot += R.Amount;
		for (const int W : R.Winners)
		{
			const ss::HandSeat* HS = H.SeatByNumber(W);
			const std::string Who = HS ? NameOf(*HS) : std::to_string(W);
			if (std::find(N.Winners.begin(), N.Winners.end(), Who) == N.Winners.end())
			{
				N.Winners.push_back(Who);
			}
		}
	}
	N.HeroThink = 4.0;
	return N;
}

/**
 * The night watched with the plan in the seat (the table's watch path: the hand dealt, the plan finishing it with
 * FinishTickWith, the director told the hand and the tick), the host's Clock calls a hand being ClockCalls (its pace: 9
 * at 1x, 3 at 4x, 1 at 8x). The cue log: every cue in Take's order.
 */
std::vector<std::string> RoomAutoWatched(const RoomAutoNight& N, style::Style S, int ClockCalls, int MaxTicks)
{
	std::vector<std::string> Log;
	std::unique_ptr<ss::Tournament> T = RoomAutoField(N);
	ss::room::Director D(N.E, N.O, N.Seed + ":room", N.Hero);
	autoplay::TableAuto Plan;
	Plan.S = S;
	Plan.M = autoplay::Mode::Watch;
	autoplay::Run Run;
	auto WorldAt = [&]() { return N.O.Start + (T->ClockMinutes() - T->Spec.StartClock); };
	auto Drain = [&](int Tick) {
		for (const ss::room::Cue& C : D.Take())
		{
			Log.push_back(std::to_string(Tick) + "\t" + std::to_string(static_cast<int>(C.What)) + "\t" + C.Speaker + "\t" + C.Key + "\t" + C.Text);
		}
	};
	for (const double At : {N.O.DeskOpens + 5.0, N.O.Start - 60.0, N.O.Start - 9.0, N.O.Start})
	{
		D.Clock(*T, At, 0.0);
	}
	Drain(0);
	for (int K = 0; K < MaxTicks && !T->bFinished && !T->Hero().Busted; ++K)
	{
		std::vector<ss::TEvent> Start;
		std::unique_ptr<ss::Hand> H = T->StartTick(Start);
		D.Events(*T, Start, WorldAt());
		Drain(T->Tick + 1);
		const std::vector<ss::TEvent> Fin = T->FinishTickWith(H.get(), autoplay::BrainFor(Plan, Run, 1.0, 1, nullptr, nullptr));
		if (H)
		{
			autoplay::NoteHand(Run, *H, RoomAutoHeroSeat(*H), true, 1.0);
			D.Hand(*T, RoomAutoNote(*T, *H, T->Tick));
		}
		D.Rounds(*T, T->Rounds());
		D.Events(*T, Fin, WorldAt());
		for (int C = 0; C < ClockCalls; ++C)
		{
			D.Clock(*T, WorldAt() + 2.5 * C / std::max(1, ClockCalls), 0.0);
		}
		if (std::any_of(Fin.begin(), Fin.end(), [](const ss::TEvent& E) { return E.Type == ss::TEventType::Break; }))
		{
			for (int Left = 24; Left >= 0; Left -= 2)
			{
				D.Clock(*T, WorldAt(), static_cast<double>(Left));
			}
		}
		Drain(T->Tick);
	}
	return Log;
}

void RoomPaceUnderThePlan()
{
	const auto T0 = std::chrono::steady_clock::now();
	RoomAutoNight N = RoomAutoMake("embercrest-nightly@15", 60, ":auto-pace");
	N.HeroStacks = 6; // a long night in the seat, whatever the style
	for (const style::Style S : {style::Style::Gto, style::Style::Maniac})
	{
		const std::vector<std::string> One = RoomAutoWatched(N, S, 9, 140);
		const std::vector<std::string> Four = RoomAutoWatched(N, S, 3, 140);
		const std::vector<std::string> Eight = RoomAutoWatched(N, S, 1, 140);
		for (const std::vector<std::string>* Other : {&Four, &Eight})
		{
			size_t D = 0;
			while (D < One.size() && D < Other->size() && One[D] == (*Other)[D])
			{
				++D;
			}
			if (One != *Other)
			{
				std::printf("room pace (plan, %s): line %d of %d/%d differs\n  1x:  %s\n  now: %s\n", style::InfoOf(S).Id, static_cast<int>(D + 1), static_cast<int>(One.size()),
					static_cast<int>(Other->size()), D < One.size() ? One[D].c_str() : "(end)", D < Other->size() ? (*Other)[D].c_str() : "(end)");
			}
		}
		RoomAutoExpect(One.size() > 40 && One == Four && One == Eight,
			std::string("room pace: with the plan in the seat (") + style::InfoOf(S).Id + "), 1x, 4x and 8x say the same lines in the same hands");
	}
	// Dee's clock under the plan (§1.1): a fixed world minutes a hand, holding the manual rate (real seconds x 6 a hand at
	// LIVE pace) within ±10%, until S-9 measures it.
	const double PlanPerHour = 60.0 / autoplay::DeeMinutesPerHand;
	const double ManualPerHour = 3600.0 / (autoplay::DeeManualSecondsPerHand * 6.0);
	RoomAutoExpect(std::fabs(PlanPerHour / ManualPerHour - 1.0) <= 0.10, "room pace: autoplay's hands per world hour at Dee's within 10% of the manual rate");
	std::printf("room pace (plan): %.1f s; Dee's %.1f hands a world hour under the plan, %.1f by hand\n", std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count(),
		PlanPerHour, ManualPerHour);
}

/** core-autoplay's card_room checks. The failures they found. */
int Run()
{
	RoomAutoFailures = 0;
	LiveSkipSliceEqualsTicks();
	RoomPaceUnderThePlan();
	return RoomAutoFailures;
}
} // namespace room_seg_auto
