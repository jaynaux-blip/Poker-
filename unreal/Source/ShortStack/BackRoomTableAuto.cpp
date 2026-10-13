// ABackRoomTable's side of autoplay (update 2 SPEC §2.12): the members BackRoomTable.h declares for the plan in the hero's
// seat, split from BackRoomTable.cpp as BackRoomPlayer*.cpp are from BackRoomPlayer.cpp. The table calls them from its
// hero branch (TickHeroAuto), its turn cue and its ReadHero bypass (HeroAutoHolds); the host's FAutoHost (BackRoomAuto.h)
// sets HeroBrain and reads the rest. The lead's Phase A stubs: nothing sets HeroBrain yet, so the plan never holds the seat
// and the table plays as it always did. ue-auto owns this file.

#include "BackRoomTable.h"

#include "ShortStack/AI/Style.h"
#include "ShortStack/Game/Room.h"

bool ABackRoomTable::HeroAutoHolds() const
{
	return HeroBrain != nullptr && !bHeroTakenOver && !bHeroAway;
}

bool ABackRoomTable::TickHeroAuto(int32 /*Idx*/)
{
	// ue-auto: while HeroAutoHolds, decide once (Tourney->HeroDecisionFor(*Hand, *HeroBrain), or style::DecideCash with Rng
	// at Dee's), check the stop rules against it, hold it for the think (HeroAutoAt), then HeroAct; true meanwhile.
	return false;
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
