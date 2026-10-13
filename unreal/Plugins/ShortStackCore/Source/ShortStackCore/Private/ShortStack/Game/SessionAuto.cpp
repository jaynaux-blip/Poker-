#include "ShortStack/Game/Session.h"

#include "ShortStack/AI/Style.h"
#include "ShortStack/Game/Autoplay.h"
#include "ShortStack/Game/Format.h"

#include <algorithm>
#include <chrono>

// Autoplay's side of the session (update 2 §2, §6.2): the panel's calls, the plan's acts, the skip path, the post-hand
// hook's autoplay calls and the event's played record. Phase A, day 1 (the lead): stubs that keep today's behaviour
// (autoplay never holds a seat; SkipStep is today's Sprint, number for number). core-autoplay owns this file from here.

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
} // namespace session_auto_detail

std::string Session::SetAutoplay(int Table, const autoplay::TableAuto& A)
{
	// Phase A's stub: the plan isn't offered yet.
	(void)Table;
	(void)A;
	return "Autoplay isn't ready yet.";
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
	// Phase A's stub: the plan never holds a seat, so there's nothing to take back.
	(void)Table;
	return "";
}

std::string Session::SkipAhead(int Table, autoplay::Target To, int Hands)
{
	// Phase A's stub.
	(void)Table;
	(void)To;
	(void)Hands;
	return "Autoplay isn't ready yet.";
}

std::string Session::StopSkip(int Table)
{
	(void)Table;
	return "";
}

double Session::Sharpness(int Table) const
{
	(void)Table;
	return AutoRun.Sharp;
}

autoplay::Panel Session::AutoPanel(int Table) const
{
	// Phase A's stub: the catalog and the table's plan as it stands.
	autoplay::Panel P;
	P.Styles = style::Catalog();
	P.Greyed.assign(P.Styles.size(), std::string());
	P.Sharp = Sharpness(Table);
	P.Form = Life.Auto.Pref.Form;
	P.Now = Autoplay(Table);
	return P;
}

void Session::AutoAct(const BotDecision& D)
{
	// The plan acts at the table in front: the action applied as HeroAct applies the player's, with no grade, no coach's
	// tally and no BestPlay or Blunder moment (§2.9). Phase A's stub; core-autoplay adds the run's bookkeeping.
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
	// Phase A's stub: today's Sprint, number for number (Autopilot's profile; the bubble and final-table stops), with
	// the post-hand hook after each hand the player was dealt. core-autoplay generalises it (the style, the targets, the
	// stop rules, BeforeTick).
	const Profile Autopilot = style::ProfileOf(style::Style::Autopilot);
	const auto T0 = std::chrono::steady_clock::now();
	auto ElapsedMs = [&]() { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count(); };
	do
	{
		const std::vector<TEvent> Events = T->SimulateTick(&Autopilot);
		++HandsPlayed;
		if (const Hand* Last = T->LastHeroHand())
		{
			HandFacts F;
			F.ByPlan = true;
			F.Skipped = true;
			for (const PotResult& P : Last->PotResults)
			{
				F.PotBefore += P.Amount;
			}
			F.Table = Active;
			HeroHandDone(*Last, F);
		}
		HandleTourneyEvents(Events);
		// Sprinting still sits the player with people (who'll remember them).
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
			BustBanner(Hero, "Busted while sprinting");
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
	} while (ElapsedMs() < SprintBudgetMs / static_cast<double>(std::max(1, TableCount())));
}

void Session::AutoAfterHand(const Hand& H, const HandFacts& F)
{
	// Phase A's stub: the plan's tilt and counters, and the observed stats (reads::Count), are core-autoplay's.
	(void)H;
	(void)F;
}

void Session::FieldPotsFill(const Hand& H, const HandFacts& F)
{
	// Phase A's stub: BiggestPot and FieldPots (bug 1) land in Phase B (they change what the world hears).
	(void)H;
	(void)F;
}

void Session::SkipMoments(const Hand& H, const HandFacts& F)
{
	// Phase A's stub: the stream moments a skip drops (bug 2) land in Phase B (they change how a sprinted event plays).
	(void)H;
	(void)F;
}

void Session::AutoRecord(const Results& Res)
{
	// Phase A's stub: the event's autoplay\tplayed line (§6.4) is core-autoplay's.
	(void)Res;
}

std::string Session::DebugAuto(const std::string& Args)
{
	// Phase A's stub (ss.Auto.*: game/Update2Commands.cpp forwards here at home).
	(void)Args;
	const autoplay::TableAuto& A = Autoplay(Active);
	return std::string("Autoplay: ") + style::InfoOf(A.S).Name + (A.M == autoplay::Mode::Off ? ", off." : A.M == autoplay::Mode::Watch ? ", watching." : ", skipping.");
}
} // namespace ss
