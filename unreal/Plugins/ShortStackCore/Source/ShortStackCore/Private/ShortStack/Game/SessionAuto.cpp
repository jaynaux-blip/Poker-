#include "ShortStack/Game/Session.h"

#include "ShortStack/AI/Reads.h"
#include "ShortStack/AI/Style.h"
#include "ShortStack/Game/Autoplay.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Sites.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>

// Autoplay's side of the session (update 2 §2, §6.2): the panel's calls (SetAutoplay, TakeOver, SkipAhead, StopSkip,
// Sharpness, AutoPanel), the plan's acts (AutoAct: no grading), the skip path (SkipStep: today's Sprint when autoplay is
// off, the plan's skip with its targets and stop rules when it's on; BeforeTick on every tick; a hand a stop rule paused
// mid-hand goes to the watch path through AdoptHand), the post-hand hook's calls (AutoAfterHand, FieldPotsFill,
// SkipMoments), the event's played record and notes (AutoRecord) and ss.Auto.* (DebugAuto). The watch path's scheduling
// stays in Session.cpp's TableStep, which steers by BrainFor, ShouldStop, AutoAct, TakeOver and the run's fields.
// core-autoplay's file.

namespace ss
{
namespace session_auto_detail
{
/** The plan of a table nobody asked about (an index out of range): autoplay off. */
const autoplay::TableAuto& AutoNone()
{
	static const autoplay::TableAuto None;
	return None;
}

constexpr uint32_t AutoDecisionStops = autoplay::StopMyAllIn | autoplay::StopBigPot | autoplay::StopPremium;

int AutoSpeedOf(int Speed)
{
	return Speed >= 8 ? 8 : Speed >= 4 ? 4 : Speed >= 2 ? 2 : 1;
}

double AutoMilli(double V)
{
	return std::round(V * 1000.0) / 1000.0;
}

int AutoHeroIndex(const Hand& H)
{
	for (size_t I = 0; I < H.Seats.size(); ++I)
	{
		if (H.Seats[I].Id == HeroId)
		{
			return static_cast<int>(I);
		}
	}
	return -1;
}

/** Autoplay takes a table's seat (§2.5): Sharpness read once from the player's Form and condition now, and held for the
 *  run; the plan's tilt starts from the player's; no take-over. Form is read with its decay applied and kept (FormAt), so
 *  a Form nothing has dated yet (a new career, or one an older save's results gave) starts its weekly clock here: a
 *  career that only ever autoplays rusts toward 70 like any other, and never keeps an edge it isn't playing for. */
void AutoTurnOn(autoplay::Run& R, autoplay::Book& B, double World, double HeroTilt, bool Exhausted, bool Needy)
{
	const double Form = autoplay::FormAt(B, World);
	R.Sharp = autoplay::SharpnessOf(Form, HeroTilt, Exhausted, Needy);
	R.SharpSetAt = World;
	R.Tilt = AutoMilli(std::min(1.0, std::max(0.0, HeroTilt)));
	R.TakenOver = false;
	R.TakenMidThink = false;
	R.ResumeWindowUntil = 0.0;
}

/** The plan a table shows on the panel, and takes when autoplay is turned on or a skip starts there, while it's off: the
 *  table's own once it has had one (a style picked there, a run turned off), else the career's default (autoplay\tpref). */
autoplay::TableAuto AutoShownPlan(const autoplay::TableAuto& Table, const autoplay::Book& B)
{
	const autoplay::TableAuto Fresh;
	const bool Untouched = Table.S == Fresh.S && Table.Speed == Fresh.Speed && Table.To == Fresh.To && Table.Hands == Fresh.Hands && Table.Stops == Fresh.Stops;
	autoplay::TableAuto A = Untouched ? autoplay::DefaultPlan(B) : Table;
	A.M = autoplay::Mode::Off;
	return A;
}

bool AutoHas(const std::vector<TEvent>& Events, TEventType Type)
{
	return std::any_of(Events.begin(), Events.end(), [Type](const TEvent& E) { return E.Type == Type; });
}

uint64_t AutoHash(const std::string& Text)
{
	uint64_t H = 14695981039346656037ull;
	for (const char Ch : Text)
	{
		H = (H ^ static_cast<unsigned char>(Ch)) * 1099511628211ull;
	}
	return H;
}

std::string AutoHex(uint64_t V)
{
	char Buf[24];
	std::snprintf(Buf, sizeof(Buf), "%016llx", static_cast<unsigned long long>(V));
	return Buf;
}

std::vector<std::string> AutoWords(const std::string& Line)
{
	std::vector<std::string> Out;
	std::string Cur;
	for (const char Ch : Line)
	{
		if (Ch == ' ' || Ch == '\t')
		{
			if (!Cur.empty())
			{
				Out.push_back(Cur);
				Cur.clear();
			}
		}
		else
		{
			Cur.push_back(Ch >= 'A' && Ch <= 'Z' ? static_cast<char>(Ch - 'A' + 'a') : Ch);
		}
	}
	if (!Cur.empty())
	{
		Out.push_back(Cur);
	}
	return Out;
}

/** The run as text (the host text's auto line and an autoseen line a player): what ss.Auto's parity check hashes. */
std::string AutoRunState(const autoplay::TableAuto& A, const autoplay::Run& R)
{
	std::string Out = autoplay::RunText(A, R);
	for (const auto& It : R.Seen)
	{
		Out += "\nautoseen\t" + It.first + "\t" + reads::Write(It.second);
	}
	return Out;
}
} // namespace session_auto_detail

std::string Session::SetAutoplay(int Table, const autoplay::TableAuto& A)
{
	if (Table < 0 || Table >= TableCount())
	{
		return autoplay::Text(autoplay::Words::NoTable);
	}
	std::string Why;
	WithTable(Table, [&]() {
		using autoplay::Mode;
		if (!T || HasBustInfo || Closing || T->bFinished || T->Hero().Busted)
		{
			Why = autoplay::Text(autoplay::Words::TableDone);
			return;
		}
		autoplay::TableAuto N = A;
		N.Speed = session_auto_detail::AutoSpeedOf(N.Speed);
		N.Stops &= autoplay::StopFinalTable | autoplay::StopBubble | session_auto_detail::AutoDecisionStops;
		N.Hands = std::max(0, N.Hands);
		if (N.M != Mode::Off)
		{
			const std::string Greyed = autoplay::CantPick(N.S, 0, T->Spec.BountyCents > 0);
			if (!Greyed.empty())
			{
				Why = Greyed + ".";
				return;
			}
		}
		if (N.M == Mode::Skip)
		{
			if (N.To == autoplay::Target::Closing)
			{
				Why = autoplay::Text(autoplay::Words::ClosingIsCash);
				return;
			}
			if (N.To == autoplay::Target::Bubble && T->InTheMoney())
			{
				Why = autoplay::Text(autoplay::Words::AlreadyInMoney);
				return;
			}
			if (N.To == autoplay::Target::FinalTable && T->Tables.size() <= 1)
			{
				Why = autoplay::Text(autoplay::Words::AlreadyFinal);
				return;
			}
			if (N.To == autoplay::Target::Hands && N.Hands <= 0)
			{
				N.Hands = 50;
			}
		}
		// Any use of autoplay keeps its line in the save (§6.2).
		Life.Update2LinesRead |= life::U2AutoPref;
		if (N.M != Mode::Off && Auto.M == Mode::Off)
		{
			session_auto_detail::AutoTurnOn(AutoRun, Life.Auto, WorldMinutes(), HeroTilt, Life.Energy < BodyFx.ExhaustedAt, NeedsTilt() > 1.0);
		}
		const bool WasSkipping = Sprinting;
		Auto = N;
		SprintStopReason.clear();
		if (N.M == Mode::Skip)
		{
			// Mid-skip: a new target from here. Otherwise the skip starts at the next hand (Sprint's precedent, §2.6.1), and
			// the plan holds the seat until then.
			if (WasSkipping)
			{
				return;
			}
			Auto.M = Mode::Watch;
			CurrentPace = Pace::Sprint;
			return;
		}
		if (CurrentPace == Pace::Sprint)
		{
			CurrentPace = Pace::Smart;
		}
		if (WasSkipping)
		{
			// Between hands (a skip only ever stops there): the table deals on, watched or played.
			Sprinting = false;
			StartNextHand();
		}
	});
	return Why;
}

const autoplay::TableAuto& Session::Autoplay(int Table) const
{
	if (Table == Active && Active >= 0)
	{
		return Auto;
	}
	if (Table >= 0 && Table < TableCount())
	{
		return Runs[static_cast<size_t>(Table)]->Auto;
	}
	return session_auto_detail::AutoNone();
}

std::string Session::TakeOver(int Table)
{
	// K (§2.6.5): mid-skip, the skip stops and the plan watches at 1x; in the resume window, autoplay off; in a hand
	// already the player's, autoplay off; otherwise this hand is the player's (the plan's pending move, if it was
	// thinking, is dropped and the prompt opens now).
	if (Table < 0 || Table >= TableCount())
	{
		return autoplay::Text(autoplay::Words::NoTable);
	}
	std::string Why;
	WithTable(Table, [&]() {
		using autoplay::Mode;
		if (!T || HasBustInfo)
		{
			Why = autoplay::Text(autoplay::Words::TableDone);
			return;
		}
		if (Sprinting)
		{
			Why = StopSkip(Active);
			return;
		}
		if (Auto.M == Mode::Off)
		{
			Why = autoplay::Text(autoplay::Words::NotOn);
			return;
		}
		Life.Update2LinesRead |= life::U2AutoPref;
		if (CurrentPace == Pace::Sprint)
		{
			// A skip waiting for the next hand: called off. The plan keeps the seat; a second K takes the wheel.
			CurrentPace = Pace::Smart;
			Auto.M = Mode::Watch;
			return;
		}
		if (Now < AutoRun.ResumeWindowUntil || AutoRun.TakenOver)
		{
			Auto.M = Mode::Off;
			AutoRun.ResumeWindowUntil = 0.0;
			AutoRun.TakenOver = false;
			AutoRun.TakenMidThink = false;
			return;
		}
		AutoRun.TakenOver = true;
		const int Idx = HeroSeatIdx();
		AutoRun.TakenMidThink = BotPending && Idx >= 0 && BotSeatIdx == Idx;
		if (AutoRun.TakenMidThink)
		{
			NextAt = Now;
		}
	});
	return Why;
}

std::string Session::SkipAhead(int Table, autoplay::Target To, int Hands)
{
	if (Table < 0 || Table >= TableCount())
	{
		return autoplay::Text(autoplay::Words::NoTable);
	}
	autoplay::TableAuto A = Autoplay(Table);
	if (A.M == autoplay::Mode::Off)
	{
		// The plan the panel shows (the table's own or the career's default), not a fresh table's Autopilot.
		A = session_auto_detail::AutoShownPlan(A, Life.Auto);
	}
	A.M = autoplay::Mode::Skip;
	A.To = To;
	A.Hands = Hands;
	return SetAutoplay(Table, A);
}

std::string Session::StopSkip(int Table)
{
	// The skip ends between hands (it only ever runs whole ticks) and the plan watches at 1x, holding the seat (§2.6.5).
	if (Table < 0 || Table >= TableCount())
	{
		return autoplay::Text(autoplay::Words::NoTable);
	}
	WithTable(Table, [&]() {
		using autoplay::Mode;
		if (Sprinting)
		{
			Sprinting = false;
			CurrentPace = Pace::Smart;
			if (Auto.M == Mode::Off)
			{
				// Today's Sprint was the Autopilot plan: it keeps the seat.
				Auto = autoplay::TableAuto();
				session_auto_detail::AutoTurnOn(AutoRun, Life.Auto, WorldMinutes(), HeroTilt, Life.Energy < BodyFx.ExhaustedAt, NeedsTilt() > 1.0);
			}
			Auto.M = Mode::Watch;
			Auto.Speed = 1;
			Life.Update2LinesRead |= life::U2AutoPref;
			SprintStopReason = autoplay::Text(autoplay::Words::StoppedByYou);
			if (T && !T->Hero().Busted && !T->bFinished && !HasBustInfo)
			{
				StartNextHand();
			}
		}
		else if (CurrentPace == Pace::Sprint)
		{
			CurrentPace = Pace::Smart;
			if (Auto.M == Mode::Skip)
			{
				Auto.M = Mode::Watch;
			}
		}
	});
	return "";
}

double Session::Sharpness(int Table) const
{
	if (Table == Active && Active >= 0)
	{
		return AutoRun.Sharp;
	}
	if (Table >= 0 && Table < TableCount())
	{
		return Runs[static_cast<size_t>(Table)]->AutoRun.Sharp;
	}
	return autoplay::Run().Sharp;
}

autoplay::Panel Session::AutoPanel(int Table) const
{
	autoplay::Panel P;
	P.Styles = style::Catalog();
	const double World = WorldMinutes();
	bool Rusty = false;
	const double Form = autoplay::FormNow(Life.Auto, World, &Rusty);
	const bool Exhausted = Life.Energy < BodyFx.ExhaustedAt;
	const bool Needy = NeedsTilt() > 1.0;
	P.Form = Form;
	P.Rusty = Rusty;
	P.SharpWhy = autoplay::SharpWhy(Form, Rusty, HeroTilt, Exhausted, Needy);
	P.Now = autoplay::DefaultPlan(Life.Auto);
	P.Sharp = autoplay::SharpnessOf(Form, HeroTilt, Exhausted, Needy);
	P.SharpSetAt = World;
	bool Knockout = false;
	auto Fill = [&](const auto& Src) {
		using autoplay::Mode;
		const autoplay::TableAuto& A = Src.Auto;
		const autoplay::Run& R = Src.AutoRun;
		Knockout = Src.T && Src.T->Spec.BountyCents > 0;
		P.On = A.M != Mode::Off;
		if (P.On)
		{
			P.Now = A;
			P.Sharp = R.Sharp;
			P.SharpSetAt = R.SharpSetAt;
		}
		else
		{
			P.Now = session_auto_detail::AutoShownPlan(A, Life.Auto);
		}
		P.TakenOver = R.TakenOver;
		P.ResumeWindow = A.M == Mode::Watch && Now < R.ResumeWindowUntil;
		P.ResumeLeft = P.ResumeWindow ? R.ResumeWindowUntil - Now : 0.0;
		P.Skipping = Src.Sprinting || (Src.CurrentPace == Pace::Sprint && A.M != Mode::Off);
		const int Idx = Src.CurHand ? session_auto_detail::AutoHeroIndex(*Src.CurHand) : -1;
		P.Thinking = A.M == Mode::Watch && !R.TakenOver && !Src.Sprinting && Src.BotPending && Idx >= 0 && Src.BotSeatIdx == Idx;
		P.ActsIn = P.Thinking ? std::max(0.0, Src.BotAt - Now) : 0.0;
		if (P.On && !R.TakenOver && !Src.Sprinting && Src.CurHand && Idx >= 0)
		{
			// The plan's last action in this hand, once played (it never shows before: U1). Only actions already shown.
			const Hand& H = *Src.CurHand;
			const int Seat = H.Seats[static_cast<size_t>(Idx)].Seat;
			for (size_t E = 0; E < Src.Cursor && E < H.Events.size(); ++E)
			{
				if (H.Events[E].Type == EventType::Action && H.Events[E].Action.Seat == Seat)
				{
					P.LastAction = autoplay::ActionLine(H.Events[E].Action);
				}
			}
		}
		autoplay::TableAuto Shown = A;
		if (P.Skipping)
		{
			Shown.M = Mode::Skip;
		}
		P.Status = autoplay::TileStatus(Shown, Src.T.get());
		P.SkipLine = P.Skipping && Src.T ? autoplay::SkipLine(Shown, *Src.T) : std::string();
		P.StopReason = Src.SprintStopReason;
		P.AiDecisions = R.AiDecisions;
		P.OwnDecisions = R.OwnDecisions;
		P.AiHands = R.AiHands;
		P.OwnHands = R.OwnHands;
	};
	if (Table == Active && Active >= 0)
	{
		Fill(*this);
	}
	else if (Table >= 0 && Table < TableCount())
	{
		Fill(*Runs[static_cast<size_t>(Table)]);
	}
	for (const style::Info& I : P.Styles)
	{
		P.Greyed.push_back(autoplay::CantPick(I.S, 0, Knockout));
	}
	return P;
}

void Session::AutoAct(const BotDecision& D)
{
	// The plan acts at the table in front: the action applied as HeroAct applies the player's, with no grade, no coach's
	// tally, no BestPlay or Blunder moment and no prompt (§2.9). It counts as the plan's decision.
	if (!CurHand || CurHand->bComplete)
	{
		return;
	}
	Hand& H = *CurHand;
	const LegalActions Legal = H.GetLegalActions();
	PlayerAction A = D.Action;
	if (A.Type == PlayerAction::Kind::Raise)
	{
		A = !Legal.CanRaise ? (Legal.CallAmount > 0 ? PlayerAction::Call() : PlayerAction::Check())
							: PlayerAction::RaiseTo(Max(static_cast<double>(Legal.MinRaiseTo), Min(static_cast<double>(Legal.MaxRaiseTo), JsRound(A.To))));
	}
	if (A.Type == PlayerAction::Kind::Check && !Legal.CanCheck)
	{
		A = PlayerAction::Call();
	}
	if (A.Type == PlayerAction::Kind::Fold)
	{
		HeroFolded = true;
	}
	H.Act(A);
	++AutoRun.AiDecisions;
	Life.Update2LinesRead |= life::U2AutoPref;
	NextAt = Now;
}

void Session::AdoptHand(std::unique_ptr<Hand> H, const std::vector<TEvent>& StartEvents)
{
	// A skip's paused hand to the watch path: StartNextHand's bookkeeping without StartTick; the hand replays from its
	// first event (Cursor 0) up to the decision that stopped it.
	BeginHand(std::move(H), StartEvents);
}

void Session::SkipStep()
{
	// The skip path (§2.8), a batch of whole ticks within today's budget (10 ms an update, split across the tables). With
	// autoplay off it's today's Sprint: the Autopilot plan, stopping at the bubble's approach and the final table, the seat
	// back to the player. With autoplay on it's the plan's skip: its style, its target, its stop rules, and the seat stays
	// the plan's (watch 1x) when it stops; a decision-level stop rule pauses the hand mid-way for the player (AdoptHand).
	// Every tick: BeforeTick (the rival's move), then the post-hand hook for a hand the player was dealt (HeroHandDone).
	using autoplay::Mode;
	using autoplay::Target;
	const bool Plan = Auto.M != Mode::Off;
	if (Plan)
	{
		Auto.M = Mode::Skip;
	}
	Life.Update2LinesRead |= life::U2AutoPref;
	autoplay::TableAuto A = Auto;
	if (!Plan)
	{
		A = autoplay::TableAuto();
		A.M = Mode::Skip;
		A.To = Target::Result;
	}
	const auto T0 = std::chrono::steady_clock::now();
	auto ElapsedMs = [&]() { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count(); };
	auto Stop = [&](const std::string& Reason) {
		// The plan's skip ends between hands and the plan watches at 1x, holding the seat (§2.6.4: "You'll want to see this").
		Sprinting = false;
		CurrentPace = Pace::Smart;
		Auto.M = Mode::Watch;
		Auto.Speed = 1;
		SprintStopReason = Reason;
		StartNextHand();
	};
	do
	{
		if (!T || T->bFinished || T->Hero().Busted || HasBustInfo)
		{
			Sprinting = false;
			return;
		}
		BeforeTick();
		const style::HeroBrain Brain = autoplay::BrainFor(A, AutoRun, Life.Perks.TiltGain, 0, &Life.Auto.Notes, nullptr);
		int Made = 0;
		const Tournament::HeroStop HeroStop = [&](const Hand& H, const BotDecision& D) {
			if (Plan && (A.Stops & session_auto_detail::AutoDecisionStops) && autoplay::ShouldStop(A, H, D, H.ToAct, T.get()))
			{
				return true;
			}
			++Made;
			return false;
		};
		const int Zone = T->PaidPlaces() + 15;
		const bool OutsideZone = T->Remaining > Zone;
		Tournament::TickResult Res = T->SimulateTickWith(Brain, HeroStop);
		AutoRun.AiDecisions += Made;
		if (Res.Paused)
		{
			// A stop rule handed the player this decision (§2.6.4): the hand goes to the watch path, replayed from its start
			// up to the decision, and from the next hand the plan watches at 1x.
			Sprinting = false;
			CurrentPace = Pace::Smart;
			Auto.M = Mode::Watch;
			Auto.Speed = 1;
			AutoRun.TakenOver = true;
			AutoRun.TakenMidThink = false;
			SprintStopReason = autoplay::Text(autoplay::Words::HandedOver);
			AdoptHand(std::move(Res.Paused), Res.Events);
			return;
		}
		++HandsPlayed;
		const bool Dealt = T->LastHeroHand() != nullptr;
		if (const Hand* Last = T->LastHeroHand())
		{
			HandFacts F;
			F.ByPlan = true;
			F.Skipped = true;
			F.EquityAtAllIn = autoplay::AllInEquity(*Last, session_auto_detail::AutoHeroIndex(*Last));
			for (const PotResult& P : Last->PotResults)
			{
				F.PotBefore += P.Amount;
			}
			F.Table = Active;
			HeroHandDone(*Last, F);
		}
		HandleTourneyEvents(Res.Events);
		// A skip still sits the player with people (who'll remember them).
		if (!FieldNpc.empty() && !T->Hero().Busted)
		{
			const auto Table = T->Tables.find(T->Hero().TableId);
			if (Table != T->Tables.end())
			{
				for (const int Idx : Table->second.Seats)
				{
					const auto Npc = Idx >= 0 ? FieldNpc.find(T->Players[static_cast<size_t>(Idx)].Id) : FieldNpc.end();
					if (Npc != FieldNpc.end())
					{
						FieldMet.insert(Npc->second);
					}
				}
			}
		}
		if (CheckSatellite())
		{
			return;
		}
		const TPlayer& Hero = T->Hero();
		if (Hero.Busted)
		{
			Sprinting = false;
			HasBustInfo = true;
			BustPlace = Hero.Place;
			BustPrize = Hero.PrizeCents;
			BustAt = Now + 2.5;
			Sound(SoundId::Bust, 1.0);
			BustBanner(Hero, Plan ? autoplay::Text(autoplay::Words::BustedSkipping) : "Busted while sprinting");
			return;
		}
		if (T->bFinished)
		{
			Sprinting = false;
			HasBustInfo = true;
			BustPlace = 1;
			BustPrize = Hero.PrizeCents;
			BustAt = Now + 3.0;
			Hooks.Celebrate();
			return;
		}
		if (!Plan)
		{
			// Today's Sprint, as it was.
			if (!SprintBubble && T->Remaining <= T->PaidPlaces() + 15)
			{
				StopSprint("approaching the bubble");
				return;
			}
			if (!SprintFinal && T->Tables.size() == 1)
			{
				StopSprint("final table");
				return;
			}
			continue;
		}
		// The plan's skip: its target first, then the stop rules (§2.6.4), each only on crossing it.
		const bool IntoZone = OutsideZone && T->Remaining <= Zone;
		const bool HandForHand = session_auto_detail::AutoHas(Res.Events, TEventType::HandForHand);
		const bool FinalNow = session_auto_detail::AutoHas(Res.Events, TEventType::FinalTable);
		if (Auto.To == Target::Hands && Dealt)
		{
			--Auto.Hands;
		}
		const char* Reason = nullptr;
		switch (Auto.To)
		{
		case Target::NextBreak:
			if (session_auto_detail::AutoHas(Res.Events, T->Spec.BreakEvery > 0 ? TEventType::Break : TEventType::Level))
			{
				Reason = autoplay::Text(T->Spec.BreakEvery > 0 ? autoplay::Words::AtBreak : autoplay::Words::AtLevel);
			}
			break;
		case Target::Bubble:
			if (IntoZone || HandForHand || session_auto_detail::AutoHas(Res.Events, TEventType::Bubble))
			{
				Reason = autoplay::Text(HandForHand && !IntoZone ? autoplay::Words::AtHandForHand : autoplay::Words::AtBubble);
			}
			break;
		case Target::FinalTable:
			if (FinalNow)
			{
				Reason = autoplay::Text(autoplay::Words::AtFinalTable);
			}
			break;
		case Target::Hands:
			if (Auto.Hands <= 0)
			{
				Reason = autoplay::Text(autoplay::Words::AtHands);
			}
			break;
		default: break;
		}
		if (!Reason && (Auto.Stops & autoplay::StopFinalTable) && FinalNow)
		{
			Reason = autoplay::Text(autoplay::Words::AtFinalTable);
		}
		if (!Reason && (Auto.Stops & autoplay::StopBubble) && (IntoZone || HandForHand))
		{
			Reason = autoplay::Text(IntoZone ? autoplay::Words::AtBubble : autoplay::Words::AtHandForHand);
		}
		if (Reason)
		{
			Stop(Reason);
			return;
		}
	} while (ElapsedMs() < SprintBudgetMs / static_cast<double>(std::max(1, TableCount())));
}

void Session::AutoAfterHand(const Hand& H, const HandFacts& F)
{
	// The plan's tilt and counts and what the player saw of the table (reads::Count, read from the next hand on), on both
	// paths (§1.3). A take-over lasts the hand: from the next one the plan has the seat back, and for its first moments
	// K turns autoplay off instead ("AUTO RESUMES", §2.6.5).
	const int Idx = session_auto_detail::AutoHeroIndex(H);
	if (Idx < 0)
	{
		return;
	}
	autoplay::NoteHand(AutoRun, H, Idx, F.ByPlan, Life.Perks.TiltGain);
	if (AutoRun.TakenOver && !F.Skipped)
	{
		AutoRun.TakenOver = false;
		AutoRun.TakenMidThink = false;
		if (Auto.M == autoplay::Mode::Watch)
		{
			// The award's last beats and the pause before the next deal (TableStep's), then the window into that hand.
			const double S = Speed();
			AutoRun.ResumeWindowUntil = Now + 1.0 * S + 1.4 * std::max(0.5, S) + autoplay::ResumeWindowSeconds;
		}
	}
}

void Session::FieldPotsFill(const Hand& H, const HandFacts& F)
{
	// Bug 1: the big pots the player played against the world's people (30 big blinds or more, the player in to the end)
	// reach the world at the event's end (ReportToWorld), the biggest with each of them; and the biggest pot the player
	// won (the skip used to leave it at 0). Draws nothing.
	(void)F;
	const int Idx = session_auto_detail::AutoHeroIndex(H);
	if (Idx < 0)
	{
		return;
	}
	const HandSeat& Me = H.Seats[static_cast<size_t>(Idx)];
	Chips Pot = 0;
	for (const PotResult& P : H.PotResults)
	{
		Pot += P.Amount;
		if (std::find(P.Winners.begin(), P.Winners.end(), Me.Seat) != P.Winners.end())
		{
			BiggestPot = MaxChips(BiggestPot, P.Amount);
		}
	}
	const Chips Net = Me.Stack - Me.StartStack;
	if (Me.Folded || Net == 0 || Pot < H.BigBlind * 30)
	{
		return;
	}
	for (const HandSeat& S : H.Seats)
	{
		const auto Npc = S.Folded || S.Id == HeroId ? FieldNpc.end() : FieldNpc.find(S.Id);
		if (Npc == FieldNpc.end())
		{
			continue;
		}
		auto Had = std::find_if(FieldPots.begin(), FieldPots.end(), [&](const std::pair<int, Chips>& P) { return P.first == Npc->second; });
		if (Had == FieldPots.end())
		{
			FieldPots.push_back({Npc->second, Net});
		}
		else if (std::llabs(Net) > std::llabs(Had->second))
		{
			Had->second = Net;
		}
	}
}

void Session::SkipMoments(const Hand& H, const HandFacts& F)
{
	// Bug 2 (skip only): the stream moments the skip used to drop: an all-in won or a bad beat, a big pot won (60 big
	// blinds or more), the bust, the cash or the win, and the stream's prediction resolved. Knockouts come through
	// HandleTourneyEvents as they always did. Draws nothing from the tournament or the session.
	if (!Stream.Live || !T)
	{
		return;
	}
	const int Idx = session_auto_detail::AutoHeroIndex(H);
	if (Idx < 0)
	{
		return;
	}
	const HandSeat& Me = H.Seats[static_cast<size_t>(Idx)];
	bool Won = false;
	for (const PotResult& P : H.PotResults)
	{
		Won = Won || std::find(P.Winners.begin(), P.Winners.end(), Me.Seat) != P.Winners.end();
	}
	const double PotBb = static_cast<double>(F.PotBefore) / static_cast<double>(std::max<Chips>(1, H.BigBlind));
	if (!Me.Folded && F.EquityAtAllIn >= 0.0)
	{
		if (Won)
		{
			StreamMoment(kast::Moment::WonAllIn, ChipsText(static_cast<double>(F.PotBefore)), PotBb / 30.0);
		}
		else if (F.EquityAtAllIn >= 0.6)
		{
			StreamMoment(kast::Moment::BadBeat, ChipsText(static_cast<double>(F.PotBefore)), PotBb / 30.0);
		}
	}
	else if (!Me.Folded && Won && PotBb >= 60.0)
	{
		StreamMoment(kast::Moment::BigPot, ChipsText(static_cast<double>(F.PotBefore)), PotBb / 40.0);
	}
	const TPlayer& Hero = T->Hero();
	if (Hero.Busted)
	{
		StreamMoment(Hero.PrizeCents > 0 ? kast::Moment::Cashed : kast::Moment::Bust, Ordinal(Hero.Place));
		if (Stream.Live)
		{
			Stream.Resolve(Channel, StreamInputs(), Hero.PrizeCents > 0);
		}
	}
	else if (T->bFinished)
	{
		StreamMoment(kast::Moment::Win, T->Spec.Name, 1.0 + static_cast<double>(Hero.PrizeCents) / 50000.0);
		if (Stream.Live)
		{
			Stream.Resolve(Channel, StreamInputs(), true);
		}
	}
}

void Session::AutoRecord(const Results& Res)
{
	// The event's end (§2.9): what the plan played (the played line, keyed by the history entry's event id, for an event
	// the plan played at all), and what the player's tables saw of the world's people, into the notes.
	(void)Res;
	if (!T)
	{
		return;
	}
	const std::string Id = T->Spec.Id.find('@') != std::string::npos ? T->Spec.Id : std::string();
	if (!Id.empty() && (AutoRun.AiHands > 0 || AutoRun.AiDecisions > 0))
	{
		autoplay::Record(Life.Auto, Id, 0, static_cast<int>(net::SiteOf(T->Spec.Id)), Auto, AutoRun);
		Life.Update2LinesRead |= life::U2AutoPlayed | life::U2AutoPref;
	}
	reads::Table Named;
	for (const auto& It : AutoRun.Seen)
	{
		const std::string Key = FieldNpc.count(It.first) ? autoplay::NoteKey(T.get(), It.first) : std::string();
		if (!Key.empty())
		{
			Named[Key] = It.second;
		}
	}
	autoplay::FoldNotes(Life.Auto, Named, WorldMinutes());
	if (!Life.Auto.Notes.empty())
	{
		Life.Update2LinesRead |= life::U2Notes;
	}
}

std::string Session::DebugAuto(const std::string& Args)
{
	// ss.Auto.* at home (game/Update2Commands.cpp): the table in front's plan.
	using autoplay::Mode;
	const std::vector<std::string> W = session_auto_detail::AutoWords(Args);
	const std::string Verb = W.empty() ? std::string("describe") : W[0];
	const std::string Arg = W.size() > 1 ? W[1] : std::string();
	if (Verb == "form")
	{
		const double Form = std::min(100.0, std::max(0.0, std::atof(Arg.c_str())));
		autoplay::FormAt(Life.Auto, WorldMinutes());
		Life.Auto.Pref.Form = std::round(Form * 1000.0) / 1000.0;
		Life.Update2LinesRead |= life::U2AutoPref;
		return "Form set to " + std::to_string(static_cast<int>(std::lround(Form))) + ". Toggle autoplay off and on to read Sharpness again.";
	}
	if (Active < 0 || !T)
	{
		return "Autoplay: no table open.";
	}
	// Off, the plan the panel shows (the table's own or the career's default) is the one these change and turn on.
	autoplay::TableAuto A = Auto.M == Mode::Off ? session_auto_detail::AutoShownPlan(Auto, Life.Auto) : Auto;
	if (Verb == "style")
	{
		style::Style S = style::Style::Autopilot;
		if (!style::FromId(Arg, S))
		{
			return "No style called \"" + Arg + "\".";
		}
		A.S = S;
		const std::string Why = SetAutoplay(Active, A);
		return Why.empty() ? std::string("Style: ") + style::InfoOf(S).Name + "." : Why;
	}
	if (Verb == "on" || Verb == "off" || Verb == "speed")
	{
		if (Verb == "speed")
		{
			A.Speed = std::atoi(Arg.c_str());
			if (A.M == Mode::Off)
			{
				A.Speed = session_auto_detail::AutoSpeedOf(A.Speed);
				Auto = A;
				return "Watch speed " + autoplay::SpeedName(Auto.Speed) + " (autoplay is off).";
			}
		}
		else
		{
			A.M = Verb == "on" ? Mode::Watch : Mode::Off;
		}
		const std::string Why = SetAutoplay(Active, A);
		return Why.empty() ? DebugAuto("describe") : Why;
	}
	if (Verb == "skip")
	{
		autoplay::Target To = autoplay::Target::Result;
		int Hands = 0;
		if (Arg == "break")
		{
			To = autoplay::Target::NextBreak;
		}
		else if (Arg == "bubble")
		{
			To = autoplay::Target::Bubble;
		}
		else if (Arg == "final")
		{
			To = autoplay::Target::FinalTable;
		}
		else if (Arg == "hands")
		{
			To = autoplay::Target::Hands;
			Hands = W.size() > 2 ? std::atoi(W[2].c_str()) : 50;
		}
		else if (Arg == "closing")
		{
			To = autoplay::Target::Closing;
		}
		const std::string Why = SkipAhead(Active, To, Hands);
		if (!Why.empty())
		{
			return Why;
		}
		return To == autoplay::Target::Hands ? "Skipping " + std::to_string(Auto.Hands) + " hands." : "Skipping to: " + std::string(autoplay::TargetName(To)) + ".";
	}
	if (Verb == "takeover")
	{
		const std::string Why = TakeOver(Active);
		return Why.empty() ? DebugAuto("describe") : Why;
	}
	if (Verb == "stop")
	{
		StopSkip(Active);
		return DebugAuto("describe");
	}
	if (Verb == "sharp")
	{
		AutoRun.Sharp = std::round(std::min(1.0, std::max(0.5, std::atof(Arg.c_str()))) * 100.0) / 100.0;
		AutoRun.SharpSetAt = WorldMinutes();
		return "Sharpness pinned at " + std::to_string(static_cast<int>(std::lround(AutoRun.Sharp * 100.0))) + "% for this run (dev).";
	}
	if (Verb == "run")
	{
		return session_auto_detail::AutoRunState(Auto, AutoRun);
	}
	if (Verb == "parity")
	{
		// §2.12: the next N hands watched through the real path at 8x (TableStep), then the same hands again headless from
		// the checkpoint saved before them (the skip path's calls), both hashed (the engine checkpoint and the run); the
		// table then carries on from the watched hands, its clock as it was.
		const int Hands = std::min(500, std::max(1, std::atoi(Arg.c_str())));
		if (Sprinting || HasBustInfo || CurrentPace == Pace::Sprint)
		{
			return "ss.Auto.Parity needs the table in front playing, not skipping.";
		}
		const double RealNow = Now;
		const autoplay::TableAuto Was = Auto;
		autoplay::TableAuto Watch = Auto.M == Mode::Off ? session_auto_detail::AutoShownPlan(Auto, Life.Auto) : Auto;
		Watch.M = Mode::Watch;
		Watch.Speed = 8;
		Watch.Stops &= ~session_auto_detail::AutoDecisionStops; // no hand-offs: nobody's there to answer them
		const std::string Why = SetAutoplay(Active, Watch);
		if (!Why.empty())
		{
			return Why;
		}
		const double Dt = 1.0 / 30.0;
		int Guard = 0;
		auto Rebase = [&]() {
			// The steps ran the table's own clock ahead of the host's: everything timed on it comes back by as much.
			const double Shift = Now - RealNow;
			Now = RealNow;
			NextAt -= Shift;
			BotAt -= Shift;
			MovingAt -= Shift;
			BustAt -= Shift;
			LastIdleChat -= Shift;
			LastLevelUpAt -= Shift;
			AutoFoldedAt -= Shift;
			LastBountyAt -= Shift;
			FocusHoldUntil -= Shift;
			CurrentBanner.At -= Shift;
			AutoRun.ResumeWindowUntil = std::max(0.0, AutoRun.ResumeWindowUntil - Shift);
			for (Flight& Fl : Flights)
			{
				Fl.Start -= Shift;
			}
			for (GradeBadge& B : Badges)
			{
				B.At -= Shift;
			}
			for (SeatVis& S : Seats)
			{
				S.ActStart -= Shift;
				S.ActEnd -= Shift;
				S.LastActionAt -= Shift;
				S.DealtAt -= Shift;
			}
			for (double& At : BoardShownAt)
			{
				At -= Shift;
			}
		};
		// To a fresh deal: hand 1 dealt, nothing acted.
		const int Before = HandsPlayed;
		while (HandsPlayed == Before && !HasBustInfo && T && ++Guard < 400000)
		{
			Now += Dt;
			TableStep();
		}
		if (HandsPlayed == Before || HasBustInfo || !T || !CurHand)
		{
			Rebase();
			return "ss.Auto.Parity: the table didn't reach a fresh hand.";
		}
		const std::string Ck0 = T->Checkpoint();
		const Hand Hand0 = *CurHand;
		const autoplay::Run Run0 = AutoRun;
		const int Played0 = HandsPlayed;
		const bool Rival0 = RivalArrived;
		// Watched: N hands through TableStep, to the deal of hand N + 1 (or the end of the night).
		while (HandsPlayed < Played0 + Hands && !HasBustInfo && T && ++Guard < 4000000)
		{
			Now += Dt;
			TableStep();
		}
		if (!T)
		{
			Now = RealNow;
			return "ss.Auto.Parity: the table closed.";
		}
		const bool Ended = HasBustInfo;
		const int Watched = HandsPlayed - Played0 + (Ended ? 1 : 0);
		const std::string Ck1 = T->Checkpoint();
		const std::string Run1Text = session_auto_detail::AutoRunState(Auto, AutoRun);
		const uint64_t Watch1 = session_auto_detail::AutoHash(Ck1 + "\n" + Run1Text);
		const autoplay::Run Run1 = AutoRun;
		const int Played1 = HandsPlayed;
		const bool Rival1 = RivalArrived;
		// Skipped: the same hands headless from the fresh deal, the skip path's calls.
		T->Restore(Ck0);
		AutoRun = Run0;
		HandsPlayed = Played0;
		RivalArrived = Rival0;
		const autoplay::TableAuto Plan = Auto;
		int Done = 0;
		auto Brain = [&]() { return autoplay::BrainFor(Plan, AutoRun, Life.Perks.TiltGain, 0, &Life.Auto.Notes, nullptr); };
		auto Count = [&](const Hand& H, const BotDecision&) {
			++AutoRun.AiDecisions;
			(void)H;
			return false;
		};
		{
			// The dealt hand, played out by the plan and the tick finished: what SimulateTickWith does after its deal. Nothing
			// had acted in it, so every action of the player's seat in its record is one of the plan's decisions.
			Hand First = Hand0;
			T->FinishTickWith(&First, Brain());
			const int Me = session_auto_detail::AutoHeroIndex(First);
			const int Seat = Me >= 0 ? First.Seats[static_cast<size_t>(Me)].Seat : -1;
			for (const ActionRecord& Rec : First.History)
			{
				AutoRun.AiDecisions += Rec.Seat == Seat ? 1 : 0;
			}
			HandFacts F;
			F.ByPlan = true;
			AutoAfterHand(First, F);
			++Done;
		}
		while (Done < Watched && !T->Hero().Busted && !T->bFinished)
		{
			BeforeTick();
			T->SimulateTickWith(Brain(), Count);
			++HandsPlayed;
			if (const Hand* Last = T->LastHeroHand())
			{
				HandFacts F;
				F.ByPlan = true;
				F.Skipped = true;
				AutoAfterHand(*Last, F);
			}
			++Done;
		}
		if (!Ended && !T->Hero().Busted && !T->bFinished)
		{
			BeforeTick();
			std::vector<TEvent> Start;
			T->StartTick(Start);
			++HandsPlayed;
		}
		const uint64_t Skip1 = session_auto_detail::AutoHash(T->Checkpoint() + "\n" + session_auto_detail::AutoRunState(Auto, AutoRun));
		// Back to the watched hands' end, the plan as it was.
		T->Restore(Ck1);
		AutoRun = Run1;
		HandsPlayed = Played1;
		RivalArrived = Rival1;
		Auto = Was;
		Rebase();
		return "Parity over " + std::to_string(Watched) + " hands: watched " + session_auto_detail::AutoHex(Watch1) + ", skipped " + session_auto_detail::AutoHex(Skip1) +
			(Watch1 == Skip1 ? ": equal." : ": DIFFERENT.");
	}
	// describe
	const TableGlance G = Glance(Active);
	const autoplay::Panel P = AutoPanel(Active);
	std::string Out = "Autoplay at " + G.Name + ": " + style::InfoOf(P.Now.S).Name;
	if (!P.On && !P.Skipping)
	{
		Out += ", off (the default plan shown).";
	}
	else if (P.Skipping)
	{
		Out += ", skipping to " + std::string(autoplay::TargetName(Auto.To)) + " (" + P.Status + ").";
	}
	else
	{
		Out += ", watching at " + autoplay::SpeedName(Auto.Speed) + (P.TakenOver ? ", this hand taken over." : ".");
	}
	Out += "\n" + autoplay::SharpLine(P.Sharp, P.SharpWhy) + (P.On ? " (held for this run)." : " (if it took the seat now).");
	char Numbers[96];
	std::snprintf(Numbers, sizeof(Numbers), "Form %.1f, %d graded decisions of yours this week; the plan's tilt %.3f.", P.Form, Life.Auto.Pref.WeekDecisions, AutoRun.Tilt);
	Out += "\n" + std::string(Numbers);
	Out += "\nDecisions: the plan " + std::to_string(AutoRun.AiDecisions) + ", you " + std::to_string(AutoRun.OwnDecisions) + ". Hands: the plan " +
		std::to_string(AutoRun.AiHands) + ", you " + std::to_string(AutoRun.OwnHands) + ". Graded this event: " + std::to_string(Grades.size()) + ".";
	int Hands = 0;
	for (const auto& It : AutoRun.Seen)
	{
		Hands += It.second.Hands;
	}
	int Noted = 0;
	if (CurHand)
	{
		for (const HandSeat& S : CurHand->Seats)
		{
			Noted += autoplay::NoteFor(&Life.Auto.Notes, T.get(), S.Id) ? 1 : 0;
		}
	}
	Out += "\nObserved: " + std::to_string(AutoRun.Seen.size()) + " players over " + std::to_string(Hands) + " player-hands; notes on " +
		std::to_string(Life.Auto.Notes.size()) + " regulars, " + std::to_string(Noted) + " at this table.";
	std::string Stops;
	for (const uint32_t Bit : {autoplay::StopFinalTable, autoplay::StopBubble, autoplay::StopMyAllIn, autoplay::StopBigPot, autoplay::StopPremium})
	{
		if (Auto.Stops & Bit)
		{
			Stops += (Stops.empty() ? "" : ", ") + std::string(autoplay::StopName(Bit));
		}
	}
	Out += "\nStops: " + (Stops.empty() ? std::string("none") : Stops) + "." + (P.StopReason.empty() ? std::string() : " Last skip ended: " + P.StopReason);
	return Out;
}
} // namespace ss
