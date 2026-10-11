// The Embercrest's test hooks and console commands (ABackRoomGameMode's live tournaments; the flow is BackRoomLive.cpp):
// the fast-forwards every track and the lead's acceptance runs share (ss.Live.Bubble, Final, Break, Shove), a bust, the
// last call, the next event from the desk and its seating.

#include "BackRoomLiveShared.h"

#include "BackRoomGameMode.h"
#include "BackRoomTable.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Tournament.h"

namespace LiveDetail
{
/** A room console command's game mode: the Embercrest's, or nothing (Dee's game, practice, another map). */
ABackRoomGameMode* LiveMode(UWorld* World)
{
	ABackRoomGameMode* GM = World ? World->GetAuthGameMode<ABackRoomGameMode>() : nullptr;
	return GM && GM->IsLive() ? GM : nullptr;
}

FAutoConsoleCommandWithWorldAndArgs BustCmd(TEXT("ss.Live.Bust"),
	TEXT("At the Embercrest: the player busts at the next gap between hands (testing: a one-chip stack, blinded out by the engine)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World) {
		if (ABackRoomGameMode* GM = LiveMode(World))
		{
			GM->TestBustNow();
		}
	}));
FAutoConsoleCommandWithWorldAndArgs LastCallCmd(TEXT("ss.Live.LastCall"),
	TEXT("At the Embercrest: the room plays on (you on autopilot) to just before the floor's last call for registration."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World) {
		if (ABackRoomGameMode* GM = LiveMode(World))
		{
			GM->TestLastCall();
		}
	}));
FAutoConsoleCommandWithWorldAndArgs NextCmd(TEXT("ss.Live.Next"),
	TEXT("ss.Live.Next [occurrence id]: once the player's night is over, the desk writes them into the next event today (or that one), as E twice at the desk does."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (ABackRoomGameMode* GM = LiveMode(World))
		{
			GM->TestRegisterNext(Args.Num() > 0 ? Args[0] : FString());
		}
	}));
FAutoConsoleCommandWithWorldAndArgs SeatCmd(TEXT("ss.Live.Seat"),
	TEXT("At the Embercrest, registered for the next event: the wait passes (as the terrace does) and the floor calls its seats."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World) {
		if (ABackRoomGameMode* GM = LiveMode(World))
		{
			GM->TestSeatingNow();
		}
	}));

// The shared fast-forwards (next_segment_spec 8.2): every track's checks and the lead's acceptance run (9.1) start from them.
FAutoConsoleCommandWithWorldAndArgs BubbleCmd(TEXT("ss.Live.Bubble"),
	TEXT("At the Embercrest: the whole room plays on at once (you on autopilot) until it's hand for hand on the bubble."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World) {
		if (ABackRoomGameMode* GM = LiveMode(World))
		{
			GM->TestFastForwardTo(TEXT("bubble"));
		}
	}));
FAutoConsoleCommandWithWorldAndArgs FinalCmd(TEXT("ss.Live.Final"),
	TEXT("At the Embercrest: the whole room plays on at once (you on autopilot) until the final table."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World) {
		if (ABackRoomGameMode* GM = LiveMode(World))
		{
			GM->TestFastForwardTo(TEXT("final"));
		}
	}));
FAutoConsoleCommandWithWorldAndArgs BreakCmd(TEXT("ss.Live.Break"),
	TEXT("At the Embercrest: the whole room plays on at once (you on autopilot) until the next break."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World) {
		if (ABackRoomGameMode* GM = LiveMode(World))
		{
			GM->TestFastForwardTo(TEXT("break"));
		}
	}));
FAutoConsoleCommandWithWorldAndArgs ShoveCmd(TEXT("ss.Live.Shove"),
	TEXT("At the Embercrest: the next opponent to act at your table while you're still in the hand moves all in."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World) {
		if (ABackRoomGameMode* GM = LiveMode(World))
		{
			GM->TestShove();
		}
	}));
// The room's director (ue-room fills DescribeRoom and TestCue).
FAutoConsoleCommandWithWorldAndArgs RoomCmd(TEXT("ss.Live.Room"),
	TEXT("At the Embercrest: the room's mood, each layer's level, the voices in use, the synth's cost, engine voices and the last 10 cues."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World) {
		if (ABackRoomGameMode* GM = LiveMode(World))
		{
			TArray<FString> Lines;
			GM->DescribeRoom().ParseIntoArrayLines(Lines, false);
			for (const FString& Line : Lines)
			{
				UE_LOG(LogEmbercrest, Display, TEXT("%s"), *Line);
			}
		}
	}));
FAutoConsoleCommandWithWorldAndArgs CueCmd(TEXT("ss.Live.Cue"),
	TEXT("ss.Live.Cue <moment> [seat]: at the Embercrest, the room's director says that moment now (a talk::Moment's name), from that seat at your table."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (ABackRoomGameMode* GM = LiveMode(World))
		{
			GM->TestCue(Args.Num() > 0 ? Args[0] : FString(), Args.Num() > 1 ? FCString::Atoi(*Args[1]) : -1);
		}
	}));
} // namespace LiveDetail

using namespace LiveDetail;

// ------------------------------------------------------------------ test hooks

void ABackRoomGameMode::TestFastForward(int32 Hands)
{
	// -1: the floor moves you to another table (at the next gap between hands).
	if (Hands < 0)
	{
		bTestMove = true;
		return;
	}
	FastForwardLeft = Hands;
}

void ABackRoomGameMode::TestHeroChips(int32 Chips)
{
	TestChips += Chips;
}

void ABackRoomGameMode::TestBustNow()
{
	if (!bLive || !Tourney || bLiveOver || bCancelled || Tourney->Hero().Busted || Tourney->bFinished)
	{
		UE_LOG(LogEmbercrest, Warning, TEXT("Test bust: the player isn't in a tournament here"));
		return;
	}
	bTestBust = true;
	UE_LOG(LogEmbercrest, Log, TEXT("Test bust: at the next gap between hands"));
}

void ABackRoomGameMode::TestLastCall()
{
	if (!bLive || !Tourney || bCancelled)
	{
		return;
	}
	const double At = LastCallAt(LiveLateRegEnds);
	if (Minutes >= LiveLateRegEnds)
	{
		UE_LOG(LogEmbercrest, Warning, TEXT("Test last call: registration closed at %s"), *Str(ss::net::TimeLabel(LiveLateRegEnds)));
		return;
	}
	if (Minutes >= At)
	{
		bLastCalled = false; // inside the window: the floor calls it again, now
		return;
	}
	if (bLiveOver)
	{
		RailCatchUp(At - 0.5);
		return;
	}
	const int32 Hands = FMath::Max(0, FMath::FloorToInt((At - 0.5 - (LiveDayStart + Tourney->ClockMinutes())) * 60.0 / Tourney->Spec.SecondsPerHand));
	UE_LOG(LogEmbercrest, Log, TEXT("Test last call: %d hands on to %s"), Hands, *Str(ss::net::TimeLabel(At)));
	TestFastForward(Hands);
}

void ABackRoomGameMode::TestRegisterNext(const FString& OccurrenceId)
{
	if (!bLive || !bLiveOver || bCancelled || !NextEntryId.IsEmpty() || Signup.IsValid())
	{
		UE_LOG(LogEmbercrest, Warning, TEXT("Test register: only once the player's night is over, with nothing registered yet"));
		return;
	}
	if (!bRailing)
	{
		LiveStay();
	}
	FString WhyNot;
	const FString Id = OccurrenceId.IsEmpty() ? NextOpenEvent(WhyNot) : OccurrenceId;
	if (Id.IsEmpty())
	{
		UE_LOG(LogEmbercrest, Warning, TEXT("Test register: nothing the desk can sell now (%s)"), WhyNot.IsEmpty() ? TEXT("no event left today") : *WhyNot);
		return;
	}
	UE_CLOG(!bRailing, LogEmbercrest, Warning, TEXT("Test register: not on the rail yet (Q stays): leaving takes the player off the list"));
	DeskSignup(Id);
}

void ABackRoomGameMode::TestSeatingNow()
{
	if (NextEntryId.IsEmpty() || bNextCalled || bOpeningNext)
	{
		UE_LOG(LogEmbercrest, Warning, TEXT("Test seating: no next event waiting"));
		return;
	}
	RailCatchUp(NextSeatingAt);
}

// ------------------------------------------------------------------ the shared fast-forwards

void ABackRoomGameMode::TestFastForwardTo(const FString& Moment)
{
	const uint8 Stop = Moment == TEXT("bubble") ? 1 : Moment == TEXT("final") ? 2 : Moment == TEXT("break") ? 3 : 0;
	if (Stop == 0 || !bLive || !Tourney || bLiveOver || bCancelled || Tourney->Hero().Busted || Tourney->bFinished)
	{
		UE_LOG(LogEmbercrest, Warning, TEXT("Fast-forward to \"%s\": only at the Embercrest with the player still in it, to bubble, final or break"), *Moment);
		return;
	}
	FastForwardStop = Stop;
	if (FastForwardReached())
	{
		FastForwardStop = 0;
		UE_LOG(LogEmbercrest, Warning, TEXT("Fast-forward to %s: the room's there already (hand %d, %d left, %d tables)"), *Moment, Tourney->Tick, Tourney->Remaining,
			static_cast<int32>(Tourney->Tables.size()));
		return;
	}
	// As TestFastForward (LiveTick plays them at the next gap between hands), with a stop instead of a count: the guard is
	// a whole night's hands.
	FastForwardLeft = 4000;
	UE_LOG(LogEmbercrest, Log, TEXT("Fast-forward to %s: the room plays on from hand %d (%d left)"), *Moment, Tourney->Tick, Tourney->Remaining);
}

bool ABackRoomGameMode::FastForwardReached() const
{
	if (!Tourney)
	{
		return true;
	}
	switch (FastForwardStop)
	{
	case 1: return Tourney->HandForHand() || Tourney->InTheMoney();
	case 2: return Tourney->Tables.size() <= 1;
	case 3: return bOnBreak;
	default: return false;
	}
}

void ABackRoomGameMode::TestShove()
{
	if (!bLive || !Table || !Tourney || bLiveOver || bCancelled || Tourney->Hero().Busted || Tourney->bFinished)
	{
		UE_LOG(LogEmbercrest, Warning, TEXT("Test shove: the player isn't in a tournament here"));
		return;
	}
	Table->TestShoveNext();
	UE_LOG(LogEmbercrest, Log, TEXT("Test shove: the next opponent to act while you're in the hand moves all in"));
}

// ------------------------------------------------------------------ the room's director (ue-room)

FString ABackRoomGameMode::DescribeRoom() const
{
	return RoomDirector.IsValid() ? FString(TEXT("The room's director is built; ue-room's report comes with its host.")) : FString(TEXT("No director in this room yet (ue-room, next_segment_spec 4.11)."));
}

void ABackRoomGameMode::TestCue(const FString& Moment, int32 Seat)
{
	UE_LOG(LogEmbercrest, Warning, TEXT("ss.Live.Cue %s %d: the room's director comes with ue-room (next_segment_spec 4.11)"), *Moment, Seat);
}
