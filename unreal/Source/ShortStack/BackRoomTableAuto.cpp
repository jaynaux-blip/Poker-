// ABackRoomTable's side of autoplay (update 2 SPEC §2.12): the members BackRoomTable.h declares for the plan in the hero's
// seat, split from BackRoomTable.cpp as BackRoomPlayer*.cpp are from BackRoomPlayer.cpp. The table calls them from its
// hero branch (TickHeroAuto), its turn cue and its ReadHero bypass (HeroAutoHolds); the host's FAutoHost (BackRoomAuto.h)
// sets HeroBrain and reads the rest. ue-auto owns this file.
//
// The hero branch under the plan mirrors the bot branch below it in BackRoomTable.cpp (§2.12): the decision is drawn once,
// at the turn (the tournament's HeroDecisionFor, from its R exactly as a bot's would be; at Dee's game style::DecideCash
// with the table's own Rng), the stop rules see it, the plan "thinks" for style::ThinkSeconds of the table's own time (the
// pawn's watch speed dilates the room, so the think goes by quicker with it), and the move is made through HeroAct, so the
// body acts it out as it does the player's. The move is never shown before it's made (U1).

#include "BackRoomTable.h"

#include "BackRoomAuto.h"
#include "BackRoomGameMode.h"
#include "Engine/World.h"
#include "ShortStack/AI/Style.h"
#include "ShortStack/AI/View.h"
#include "ShortStack/Game/Autoplay.h"
#include "ShortStack/Game/Room.h"

namespace BackRoomTableAutoDetail
{
/** The host of the table's night (null in a world without a Back Room game mode). */
FAutoHost* TableAutoHost(const UWorld* World)
{
	return World ? FAutoHost::Of(Cast<ABackRoomGameMode>(World->GetAuthGameMode())) : nullptr;
}
} // namespace BackRoomTableAutoDetail

bool ABackRoomTable::HeroAutoHolds() const
{
	return HeroBrain != nullptr && !bHeroTakenOver && !bHeroAway;
}

bool ABackRoomTable::TickHeroAuto(int32 Idx)
{
	if (!HeroAutoHolds() || !Hand || Hand->bComplete || Idx < 0 || Idx >= static_cast<int32>(Hand->Seats.size()))
	{
		return false;
	}
	FAutoHost* Host = BackRoomTableAutoDetail::TableAutoHost(GetWorld());
	if (!Host)
	{
		return false;
	}
	if (bHeroAutoPending && !Host->PlanDecisionFresh(*Hand))
	{
		// The seat acted since the plan drew this (the away rule checked or mucked while the player was up, then they sat
		// back down): that decision was for a spot that's gone, so the plan decides this one afresh.
		bHeroAutoPending = false;
	}
	if (!bHeroAutoPending)
	{
		// Decide once (a bot's draws, in a bot's order), then let the stop rules see what the plan chose.
		const ss::PlayerView View = ss::MakeView(*Hand, Idx);
		const ss::BotDecision D = Tourney ? Tourney->HeroDecisionFor(*Hand, *HeroBrain)
										  : ss::style::DecideCash(View, *HeroBrain, *Rng, Host->CashMixSeed(), HandNumber);
		if (Host->StopsHere(*Hand, D, Idx))
		{
			// A stop rule hands this decision (and the rest of the hand) to the player: the prompt is open already.
			bHeroTakenOver = true;
			bHeroAutoPending = false;
			return false;
		}
		ss::style::HeroContext C;
		C.View = View;
		C.T = Tourney.Get();
		C.HeroIdx = Idx;
		C.MixSeed = Tourney ? Tourney->SeedHash : Host->CashMixSeed();
		C.Tick = Tourney ? Tourney->Tick : HandNumber;
		// The think on the table's clock at 1x: the watch speed is the pawn's pace over the whole room.
		const double Think = ss::style::ThinkSeconds(*HeroBrain, C, 1);
		Host->NotePlanDrew(*Hand);
		bHeroAutoPending = true;
		HeroAutoAt = Time + static_cast<float>(Think);
		HeroAutoKind = static_cast<int32>(D.Action.Type);
		HeroAutoTo = D.Action.To;
		return true;
	}
	if (Time < HeroAutoAt)
	{
		return true;
	}
	bHeroAutoPending = false;
	const size_t Before = Hand->History.size();
	HeroAct(HeroAutoKind, HeroAutoTo);
	if (Hand->History.size() == Before && bHeroTurn)
	{
		// Never stall the table on a move the hand won't take (the bots' rule): checked when it's free, else folded.
		const ss::LegalActions L = Hand->GetLegalActions();
		HeroAct(static_cast<int32>(L.CanCheck ? ss::PlayerAction::Kind::Check : ss::PlayerAction::Kind::Fold), 0.0);
	}
	if (Hand->History.size() > Before)
	{
		HeroAutoLast = FString(UTF8_TO_TCHAR(ss::autoplay::ActionLine(Hand->History.back()).c_str()));
		Host->NotePlanActed();
	}
	return true;
}

void ABackRoomTable::HeroTakeOver()
{
	bHeroTakenOver = true;
	bHeroAutoPending = false;
}

bool ABackRoomTable::IsHeroAutoDeciding() const
{
	return bHeroAutoPending && HeroAutoHolds();
}

float ABackRoomTable::HeroAutoActsIn() const
{
	return IsHeroAutoDeciding() ? FMath::Max(0.0f, HeroAutoAt - Time) : 0.0f;
}

FString ABackRoomTable::HeroAutoLastAction() const
{
	return HeroAutoLast;
}

const ss::Hand* ABackRoomTable::HeroHandForAuto() const
{
	return Hand ? Hand.Get() : EndedHand.Get();
}

const ss::room::HandNote* ABackRoomTable::PeekHandNote() const
{
	return HandNoteReady.Get();
}
