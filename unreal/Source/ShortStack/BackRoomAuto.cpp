// ue-auto's FAutoHost (BackRoomAuto.h) and ABackRoomGameMode's six autoplay functions (update 2 SPEC §2.12), which forward
// to it. The lead's Phase A stubs: the plan is off, so every hook keeps today's behaviour (the seat is the player's, the
// clocks, the talk, the HUD and the host text are as they were). ue-auto owns this file.

#include "BackRoomAuto.h"

#include "BackRoomGameMode.h"
#include "BackRoomTable.h"
#include "ShortStack/Game/Room.h"
#include "ShortStack/UI/TableHud.h"

FAutoHost::FAutoHost(ABackRoomGameMode& InMode)
	: Mode(InMode)
{
}

FAutoHost* FAutoHost::Of(const ABackRoomGameMode* InMode)
{
	return InMode ? InMode->AutoHost.Get() : nullptr;
}

// ------------------------------------------------------------------ the game mode's six

bool FAutoHost::IsPlaying() const
{
	return false;
}

float FAutoHost::WatchSpeed() const
{
	return 1.0f;
}

void FAutoHost::Set(const ss::autoplay::TableAuto& /*NewPlan*/)
{
}

void FAutoHost::TakeOver()
{
}

void FAutoHost::Skip(ss::autoplay::Target /*To*/, int32 /*Hands*/)
{
}

FString FAutoHost::Describe() const
{
	return TEXT("Autoplay: off. The seat is the player's.");
}

bool ABackRoomGameMode::IsAutoPlaying() const
{
	return AutoHost && AutoHost->IsPlaying();
}

float ABackRoomGameMode::AutoWatchSpeed() const
{
	return AutoHost ? AutoHost->WatchSpeed() : 1.0f;
}

void ABackRoomGameMode::SetAuto(const ss::autoplay::TableAuto& Plan)
{
	if (AutoHost)
	{
		AutoHost->Set(Plan);
	}
}

void ABackRoomGameMode::AutoTakeOver()
{
	if (AutoHost)
	{
		AutoHost->TakeOver();
	}
}

void ABackRoomGameMode::AutoSkip(ss::autoplay::Target To, int32 Hands)
{
	if (AutoHost)
	{
		AutoHost->Skip(To, Hands);
	}
}

FString ABackRoomGameMode::DescribeAuto() const
{
	return AutoHost ? AutoHost->Describe() : FString(TEXT("Autoplay: no table tonight."));
}

// ------------------------------------------------------------------ the hooks

void FAutoHost::Tick(float /*RealDt*/)
{
}

bool FAutoHost::PawnKeys(ABackRoomPawn& /*Pawn*/, APlayerController* /*PC*/, bool /*bCardLock*/)
{
	return false;
}

void FAutoHost::OnHandEnded(const ss::recall::Record& /*Rec*/, const ss::room::HandNote* /*Note*/)
{
}

void FAutoHost::OnHeroBusted()
{
}

double FAutoHost::DeeMinutes(double RealMinutes)
{
	return RealMinutes;
}

FString FAutoHost::HostLines() const
{
	return FString();
}

bool FAutoHost::ReadHostLine(const TArray<FString>& /*Fields*/)
{
	return false;
}

void FAutoHost::OnStand(bool /*bUp*/)
{
}

bool FAutoHost::TickSkip(float /*RealDt*/)
{
	return false;
}

void FAutoHost::OnSettle(ss::SaveData& /*D*/, const std::string& /*EntryId*/)
{
}

bool FAutoHost::TalkGate(const ss::room::Cue& /*Cue*/) const
{
	return true;
}

void FAutoHost::FillHud(ss::ui::TableHudInfo& /*Out*/, const FBackRoomPrompt& /*Prompt*/) const
{
}

void FAutoHost::FillCard(ss::ui::TableCardInfo& /*Out*/) const
{
}

FString FAutoHost::StatusLine() const
{
	return FString();
}

FString FAutoHost::Command(const FString& Line)
{
	// (ue-auto: style, on, off, speed, skip, takeover, sharp, form, describe, parity.)
	if (Line.StartsWith(TEXT("describe")))
	{
		return Describe();
	}
	return FString::Printf(TEXT("ss.Auto %s: not at the table yet. Autoplay here is off; the seat is the player's."), *Line);
}
