// ABackRoomGameMode's live tournaments: the Embercrest Casino card room's events (opened from Dee's Burner
// thread with "?Live=<occurrence id>", after ss::Session::GoToLive registered the player).
//
// The field is the player's entry (ss::life::LiveEntry): the people the living world registered and the
// room's anonymous regulars, seated by the engine's draw (ss::live::MakeField) with the entry's own seed, so
// the night is the same however often it's opened. The player's table is played hand by hand and the rest
// of the room in the background: other tables run under their pendants with extras at them and close as
// the field shrinks. Whoever the draw and the balancing put at the player's table sits there, busted players
// say goodbye and go, the floor runs the night over the PA (levels, breaks, busts near the money, hand for
// hand, the bubble, the final table), and a table break walks the player to a new seat. Busting, or
// winning, settles the entry once (ss::live::Settle) and tells the world what the player's tables saw.
//
// After that the night goes on in the room. The player's old table keeps playing the engine's rounds while they watch
// from the rail; the desk can write them into the next event today (the world's field for it, read from the save on a
// worker), the floor calls its seats when they open, and the room opens again for it with no second bus. Every way out
// leaves through the resort's entrance (the Embercrest map) when it's been built, and goes home from there.
//
// The room's code is split by job (next_segment_spec 8.2): this file is the night's flow (registration, seating, the
// checkpoint, moves, the end of the night, on foot, the settle, the way home, the tick); the room and its floor are in
// BackRoomLiveRoom.cpp, the desk, the boards and the next event in BackRoomLiveVenue.cpp, who sits where in
// BackRoomLiveCast.cpp, the test hooks and console commands in BackRoomLiveCommands.cpp. What they share is in
// BackRoomLiveShared.h.

#include "BackRoomLiveShared.h"

#include "BackRoomChips.h"
#include "CareerSave.h"
#include "BackRoomGameMode.h"
#include "BackRoomPlayer.h"
#include "BackRoomStage.h"
#include "BackRoomTable.h"
#include "BackRoomTableHost.h"
#include "CardRoomSound.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "NightOneAudio.h"
#include "NightOneSaveGame.h"
#include "TimerManager.h"
#include "ShortStack/AI/Profiles.h"
#include "ShortStack/Audio/Synth.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Room.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/TableTalk.h"
#include "ShortStack/Tournament.h"

#include <algorithm>

DEFINE_LOG_CATEGORY(LogEmbercrest);

namespace LiveDetail
{
const TCHAR* WeekdayName(int32 Day)
{
	static const TCHAR* Names[7] = {TEXT("Monday"), TEXT("Tuesday"), TEXT("Wednesday"), TEXT("Thursday"), TEXT("Friday"), TEXT("Saturday"), TEXT("Sunday")};
	return Names[((Day % 7) + 7) % 7];
}
} // namespace LiveDetail

using namespace LiveDetail;

FString ABackRoomGameMode::Chips(int64 Amount) const
{
	return Num(Amount);
}

// ------------------------------------------------------------------ registration

bool ABackRoomGameMode::LoadLive()
{
	FString Event = UGameplayStatics::ParseOption(OptionsString, TEXT("Live"));
	if (Event.IsEmpty())
	{
		return false;
	}
	if (Event == TEXT("embercrest"))
	{
		// From before the daily schedule (and the editor's test hooks): that day's evening event.
		const FString DayOption = UGameplayStatics::ParseOption(OptionsString, TEXT("Day"));
		const int32 Day = DayOption.IsEmpty() ? 6 : FCString::Atoi(*DayOption);
		for (const ss::live::Occurrence& O : ss::live::Occurrences(Day))
		{
			if (Event == TEXT("embercrest") || O.T->StartMinute == 19 * 60)
			{
				Event = Str(O.Id);
			}
		}
	}
	std::string CareerText;
	TSharedPtr<ss::SaveData> D = MakeShared<ss::SaveData>();
	if (!CareerSave::LoadText(CareerText) || !ss::SaveData::Parse(CareerText, *D))
	{
		UE_LOG(LogEmbercrest, Warning, TEXT("Live=%s but no career save: practice table."), *Event);
		return false;
	}
	const ss::live::Occurrence O = ss::live::FindOccurrence(std::string(TCHAR_TO_UTF8(*Event)));
	if (!O.Valid())
	{
		UE_LOG(LogEmbercrest, Warning, TEXT("Live=%s isn't one of the Embercrest's events: practice table."), *Event);
		return false;
	}
	LeftHomeAt = ss::net::MinutesPerDay * ss::net::NightOneDay + D->ClockMinutes;
	ss::life::LiveEntry* Entry = ss::live::EntryFor(D->Life, O.Id);
	if (!Entry)
	{
		// Opened directly (the editor, a test): registered at the desk on arrival, with the field the world has for it.
		const double At = FMath::Max(LeftHomeAt + ss::live::TravelMinutes, O.DeskOpens);
		FLiveDeskSignup Field;
		Field.WorldText = D->WorldText;
		Field.HeroName = D->HeroName;
		Field.Hero = D->Life;
		Field.O = O;
		Field.At = At;
		Field.Run();
		const std::string Why = RegisterFromSignup(*D, Field, At - ss::live::TravelMinutes, false);
		Entry = Why.empty() ? ss::live::EntryFor(D->Life, O.Id) : nullptr;
		if (!Entry)
		{
			UE_LOG(LogEmbercrest, Warning, TEXT("Can't register for %s (%s): practice table."), *Event, *Str(Why));
			return false;
		}
		LeftHomeAt = FMath::Max(LeftHomeAt, At - ss::live::TravelMinutes);
		ss::live::PayFare(D->BankrollCents, D->Life, O.Id, false, LeftHomeAt);
	}
	if (Entry->State != ss::life::LiveEntry::Registered)
	{
		UE_LOG(LogEmbercrest, Warning, TEXT("%s is already settled: practice table."), *Event);
		return false;
	}
	// Back after the game closed during the night (the entry says the player got here): no bus this time, and
	// nothing was dealt while the game was closed.
	bBackInRoom = Entry->ArrivedAt > 0.0;
	if (bBackInRoom)
	{
		LeftHomeAt = Entry->ArrivedAt - ss::live::TravelMinutes;
	}
	// An event the desk wrote the player into during the night, already in the room (it rides home on the first event's
	// bus), and the night's own departure, which Night One catches its calendar up from.
	bDeskEntry = bBackInRoom && Entry->ArrivedAt <= Entry->RegisteredAt + 0.01;
	NightLeftHomeAt = LeftHomeAt;
	if (bDeskEntry && Entry->Checkpoint.empty())
	{
		// (Before its first checkpoint, the desk's entry keeps that departure itself.)
		TArray<FString> Lines;
		Str(Entry->CheckpointHost).ParseIntoArray(Lines, TEXT("\n"), true);
		for (const FString& Line : Lines)
		{
			FString Key, Value;
			if (Line.Split(TEXT("\t"), &Key, &Value) && Key == TEXT("from"))
			{
				NightLeftHomeAt = FMath::Min(NightLeftHomeAt, FCString::Atod(*Value));
			}
		}
	}
	// Straight on from the night's earlier event in this room (the desk's): no bus and no trip, the clock as the room left it.
	bHere = bBackInRoom && UGameplayStatics::HasOption(OptionsString, TEXT("Here"));
	double HereAt = 0.0;
	if (bHere)
	{
		const FString From = UGameplayStatics::ParseOption(OptionsString, TEXT("From"));
		NightLeftHomeAt = From.IsEmpty() ? NightLeftHomeAt : FMath::Min(NightLeftHomeAt, FCString::Atod(*From));
		HereAt = FCString::Atod(*UGameplayStatics::ParseOption(OptionsString, TEXT("At")));
	}
	Save = D;
	for (const ss::life::LiveFace& F : Entry->Faces)
	{
		LiveFaces.Add(Str(F.Name), TPair<FString, FString>(Str(F.Label), Str(F.Line)));
	}
	LiveEntryId = Str(O.Id);
	LiveName = Str(O.T->Name);
	LiveShort = Str(O.T->Short);
	LiveLateRegEnds = O.LateRegEnds;
	LiveBreakMinutes = O.T->BreakMinutes;
	// Up the ridge on the 14 bus (or, on from the night's earlier event, already here: its hours count from now).
	Minutes = LeftHomeAt + ss::live::TravelMinutes;
	if (bHere)
	{
		Minutes = FMath::Max(Minutes, HereAt);
		LeftHomeAt = Minutes - ss::live::TravelMinutes;
	}
	LiveDay = O.Day;
	LiveDayStart = ss::net::MinutesPerDay * LiveDay;
	Energy = static_cast<float>(D->Life.Energy);
	PastLiveEvents = D->Life.LiveEvents;
	PastLiveCashes = D->Life.LiveCashes;
	PastBestPlace = D->Life.LiveBestPlace;
	PastNights = D->Life.BackRoomNights;
	PastNetCents = D->Life.BackRoomNetCents;
	bFirstVisit = false;
	// Paid at registration: the bankroll already shows it, the chips are on the table.
	StartBankrollCents = D->BankrollCents;
	BoughtInCents = Entry->PaidCents;
	BaseCents = StartBankrollCents;
	// The field the entry was registered into, drawn with its own seed.
	std::vector<ss::ReservedPlayer> Known;
	for (const std::pair<std::string, int>& Who : Entry->Roster)
	{
		ss::ReservedPlayer P;
		P.Name = Who.first;
		P.Type = static_cast<ss::Archetype>(Who.second);
		Known.push_back(P);
	}
	bCancelled = ss::live::BelowMinimum(O, Entry->Entrants);
	const bool bRestore = bBackInRoom && !Entry->Checkpoint.empty();
	bool bFits = false;
	Tourney = MakeShareable(ss::live::ResumeField(O, *Entry, FMath::Max(Entry->Entrants, 2), D->HeroName, Known, bRestore, bFits).release());
	HeroBuyInChips = Tourney->Spec.StartingStack;
	ABackRoomChips::ChipUnit = 100;
	LiveSaver = MakeShared<FCareerSaver>();
	if (bRestore)
	{
		// The room as it stood after the last hand the player saw.
		bResumed = bFits;
		if (bResumed)
		{
			Minutes = FMath::Max(Minutes, Entry->CheckpointAt);
			ReadLiveHost(Str(Entry->CheckpointHost));
			LastCheckpoint = Entry->Checkpoint;
			LastCheckpointAt = Entry->CheckpointAt;
		}
		UE_CLOG(!bResumed, LogEmbercrest, Warning, TEXT("%s: the checkpoint doesn't fit the field; the night starts over"), *LiveName);
	}
	if (!bBackInRoom)
	{
		// Here: the entry remembers it (a game closed from now on comes back to this room).
		Entry->ArrivedAt = Minutes;
		ss::SaveData Arrived = *Save;
		Arrived.ClockMinutes = Minutes - ss::net::MinutesPerDay * ss::net::NightOneDay;
		CareerSave::SaveNow(Arrived.Serialize());
	}
	Phase = EBackRoomPhase::Arriving;
	ArrivalDayText = FString::Printf(TEXT("%s   %s"), WeekdayName(LiveDay), *FString(UTF8_TO_TCHAR(ss::net::DateLabel(LiveDay).c_str())));
	UE_LOG(LogEmbercrest, Log, TEXT("%s: %d entrants (%d the world follows), pool %lld cents, %d paid; bankroll %lld; arriving %s"), *LiveName, Tourney->Spec.Entrants,
		static_cast<int32>(Known.size()), static_cast<int64>(Tourney->PrizePoolCents), Tourney->PaidPlaces(), StartBankrollCents, *ClockLabel());
	{
		FString Faces;
		for (const TPair<FString, TPair<FString, FString>>& F : LiveFaces)
		{
			Faces += (Faces.IsEmpty() ? TEXT("") : TEXT(", ")) + F.Key + TEXT(" (") + F.Value.Key + TEXT(")");
		}
		UE_CLOG(!Faces.IsEmpty(), LogEmbercrest, Log, TEXT("Faces in the field: %s"), *Faces);
	}
	UE_CLOG(bHere, LogEmbercrest, Log, TEXT("On from the night's earlier event: no bus, %s, the night left home at %s"), *ClockLabel(), *Str(ss::net::TimeLabel(NightLeftHomeAt)));
	UE_CLOG(bBackInRoom && !bHere, LogEmbercrest, Log, TEXT("Back in the room%s: hand %d, level %d, %d left, stack %lld, %d hands played%s"), bResumed ? TEXT(" from the checkpoint") : TEXT(""),
		Tourney->Tick, Tourney->LevelIndex + 1, Tourney->Remaining, static_cast<int64>(Tourney->Hero().Stack), HandsPlayed, bResumeDealt ? TEXT(", the dealt hand is dead") : TEXT(""));
	return true;
}

void ABackRoomGameMode::SeatLive()
{
	Table->bCardRoomTone = true;
	Table->bFirstVisit = false;
	Table->SetTournament(Tourney);
	TWeakObjectPtr<ABackRoomGameMode> Self = this;
	Table->OnSeatPlayer = [Self](const ss::TPlayer& P, int32 Seat) -> ABackRoomPlayer* {
		ABackRoomGameMode* GM = Self.Get();
		return GM ? GM->SeatCast(P, Seat) : nullptr;
	};
	Table->OnUnseatPlayer = [Self](ABackRoomPlayer* A, const FString& Id, bool bBusted) {
		if (ABackRoomGameMode* GM = Self.Get())
		{
			GM->UnseatCast(A, Id, bBusted);
		}
	};
	// Doors open at six; nobody's walking in much before ten to seven.
	const double CardsAt = LiveDayStart + Tourney->Spec.StartClock;
	Minutes = FMath::Max(Minutes, CardsAt - 8.0);
	// Late: the field has played the levels you missed. Inside late registration your seat waits with a full stack;
	// after it (back to an entry you'd left), the seat was dealt in without you, and the blinds took their share. Written
	// in at the desk tonight, the seat waits however long the floor's call and the walk took: the registration was in time.
	const double SitAt = Minutes + 2.0;
	if (SitAt > CardsAt && !bResumed)
	{
		const bool bReturning = SitAt > LiveLateRegEnds && !bDeskEntry;
		const int32 Missed = FMath::FloorToInt((SitAt - CardsAt) * 60.0 / Tourney->Spec.SecondsPerHand);
		Tourney->HeroAway = !bReturning;
		Tourney->HeroSitsOut = bReturning;
		for (int32 I = 0; I < Missed && !Tourney->bFinished && !Tourney->Hero().Busted; ++I)
		{
			Tourney->SimulateTick();
		}
		Tourney->HeroAway = false;
		Tourney->HeroSitsOut = false;
		UE_LOG(LogEmbercrest, Log, TEXT("%s: %d hands played without you, level %d, %d left"), bReturning ? TEXT("Back after leaving") : TEXT("Late registration"), Missed,
			Tourney->LevelIndex + 1, Tourney->Remaining);
	}
	// The room's one voice (next_segment_spec 4.11): the director, with the field as it stands now (a night come back to
	// resumes from its checkpoint's liveroom), and the stereo room the table will stream in place of the old bed.
	BuildDirector();
	// The hand that was dealt when the game closed is dead: the player's cards go in the muck (checked through when it's
	// free), the room plays it out, and the night goes on from the next hand. Nobody sees the same deck twice.
	std::vector<ss::TEvent> DeadHand;
	if (bResumed && bResumeDealt && !Tourney->Hero().Busted && !Tourney->bFinished)
	{
		Tourney->HeroSitsOut = true;
		DeadHand = Tourney->SimulateTick();
		Tourney->HeroSitsOut = false;
		Minutes = FMath::Max(Minutes, LiveDayStart + Tourney->ClockMinutes());
		if (RoomHost.IsValid() && RoomDirector.IsValid())
		{
			RoomFeed::Simulated(*RoomHost, *RoomDirector, RoomSound.Get(), *Tourney, DeadHand, Minutes);
		}
	}
	if (bResumed)
	{
		// Where the night stands: the stage, hand for hand, what the player already knew of the regulars.
		bFinalTable = Tourney->Tables.size() == 1;
		bHandForHand = Tourney->HandForHand();
		bAnnouncedStart = true;
		for (const TPair<FString, int32>& R : ResumeReads)
		{
			Table->Reads.Add(R.Key, R.Value);
		}
		RaiseBanner(FString::Printf(TEXT("LEVEL %d   %s / %s   %d LEFT"), Tourney->LevelIndex + 1, *Num(Tourney->CurrentLevel().Sb), *Num(Tourney->CurrentLevel().Bb), Tourney->Remaining));
	}
	ArrivalDayText = FString::Printf(TEXT("%s, %s   %s"), WeekdayName(LiveDay), *FString(UTF8_TO_TCHAR(ss::net::DateLabel(LiveDay).c_str())), *ClockLabel().RightChop(5));
	// The floor's last call and the close of registration are news only to a room that's here for them.
	const double SeatedAt = FMath::Max(Minutes, LiveDayStart + Tourney->ClockMinutes());
	bLastCalled = SeatedAt >= LastCallAt(LiveLateRegEnds);
	bRegClosed = SeatedAt >= LiveLateRegEnds;
	// The night as it stands before the first hand here is dealt (so even that hand is never dealt twice).
	LiveCheckpoint(false);
	// The first table, seated before you get there; the room placed round it, the rest of the room at theirs.
	Table->PrepareNext();
	Table->TakeEvents();
	for (const ss::TEvent& E : DeadHand)
	{
		LiveEvent(E);
	}
	if (bOnBreak)
	{
		Table->Hold();
	}
	if (bFinalTable && Stage)
	{
		Stage->SetOnAir(true);
	}
	if (Stage)
	{
		Stage->SetRoomAnchor(SlotForTable(Tourney->Hero().TableId));
		// The champions' board as it stood tonight, and the cash list on the desk's whiteboard.
		TArray<FString> Champions;
		if (const ss::life::LiveEntry* Mine = ss::live::EntryFor(Save->Life, std::string(TCHAR_TO_UTF8(*LiveEntryId))))
		{
			for (const std::string& B : Mine->Board)
			{
				TArray<FString> Parts;
				Str(B).ParseIntoArray(Parts, TEXT("\t"), false);
				if (Parts.Num() == 3)
				{
					Champions.Add(FString::Printf(TEXT("%s     %s     %s"), *Parts[0], *Parts[1].ToUpper(), *Dollars(FCString::Atoi64(*Parts[2]))));
				}
			}
		}
		if (Champions.Num() == 0)
		{
			Champions.Add(TEXT("YOUR NAME HERE"));
		}
		Stage->SetChampions(Champions);
		static const TCHAR* Regulars[] = {TEXT("BOOTS"), TEXT("T.J."), TEXT("MARIA G."), TEXT("OMAR"), TEXT("LIN"), TEXT("DUKE"), TEXT("BEV"), TEXT("SONNY"), TEXT("J.P."), TEXT("ROSIE")};
		TArray<FString> Cash = {TEXT("1-2 NLH     3 TABLES")};
		FString List = TEXT("LIST:");
		for (int32 I = 0; I < 5; ++I)
		{
			List += TEXT("  ") + FString(Regulars[(LiveDay * 3 + I * 7) % 10]);
		}
		Cash.Add(List);
		Cash.Add(TEXT("2-5 NLH     1 TABLE"));
		Cash.Add(TEXT("4-8 LIMIT   FRI & SAT"));
		Stage->SetCashList(Cash);
	}
	PlaceExtras();
	UpdateRoomBoards();
}

int32 ABackRoomGameMode::SlotForTable(int32 TableId)
{
	// Every table has its place: table N stands at slot N - 1, and the final table plays on the stage.
	if (Tourney && Tourney->Tables.size() == 1)
	{
		return ABackRoomStage::StreamSlot();
	}
	return FMath::Clamp(TableId - 1, 0, ABackRoomStage::StreamSlot() - 1);
}

int32 ABackRoomGameMode::AnchorTableId()
{
	if (!Stage || !Tourney)
	{
		return -1;
	}
	for (const auto& It : Tourney->Tables)
	{
		if (SlotForTable(It.first) == Stage->GetRoomAnchor())
		{
			return It.first;
		}
	}
	return -1;
}

double ABackRoomGameMode::LevelTimeLeft() const
{
	if (!Tourney)
	{
		return 0.0;
	}
	// The clock runs between hands too: a hand takes about forty-five seconds to play at this table.
	const double Hand = Tourney->Spec.SecondsPerHand;
	const double Since = Table ? FMath::Min(static_cast<double>(Table->HandAge()) * Hand / 45.0, Hand - 1.0) : 0.0;
	return FMath::Max(0.0, Tourney->LevelSecondsLeft() - Since);
}

void ABackRoomGameMode::LiveCheckpoint(bool bDealt)
{
	if (!Save.IsValid() || !LiveSaver || !Tourney || bLiveOver || bCancelled)
	{
		return;
	}
	if (!bDealt)
	{
		bCheckpointDue = false;
		const std::string Ck = Tourney->Checkpoint();
		if (Ck.empty())
		{
			return;
		}
		LastCheckpoint = Ck;
		LastCheckpointAt = FMath::Max(Minutes, LiveDayStart + Tourney->ClockMinutes());
	}
	if (LastCheckpoint.empty())
	{
		return;
	}
	ss::SaveData D = *Save;
	ss::life::LiveEntry* E = ss::live::EntryFor(D.Life, std::string(TCHAR_TO_UTF8(*LiveEntryId)));
	if (!E || E->State != ss::life::LiveEntry::Registered)
	{
		return;
	}
	// The room's own memory (liveroom: what the director has said) goes with the tournament's and the host's, in one
	// assignment, so a save never holds one without the others (next_segment_spec 4.11).
	E->Checkpoint = LastCheckpoint;
	E->CheckpointAt = LastCheckpointAt;
	E->CheckpointHost = std::string(TCHAR_TO_UTF8(*LiveHostText(bDealt)));
	if (RoomDirector.IsValid())
	{
		E->Room = RoomDirector->Write();
	}
	D.ClockMinutes = LastCheckpointAt - ss::net::MinutesPerDay * ss::net::NightOneDay;
	D.Life.Energy = FMath::Clamp(static_cast<double>(Energy), 0.0, 100.0);
	LiveSaver->Submit(D);
	++Checkpoints;
	UE_LOG(LogEmbercrest, Log, TEXT("Checkpoint %d%s: hand %d, level %d, %d left, stack %lld"), Checkpoints, bDealt ? TEXT(" (dealt)") : TEXT(""), Tourney->Tick, Tourney->LevelIndex + 1,
		Tourney->Remaining, static_cast<int64>(Tourney->Hero().Stack));
}

FString ABackRoomGameMode::LiveHostText(bool bDealt) const
{
	// "key\tvalue" lines: what the room knows that the tournament doesn't.
	FString Out = FString::Printf(TEXT("hands\t%d\n"), HandsPlayed);
	if (bDealt)
	{
		Out += TEXT("dealt\t1\n");
	}
	if (bOnBreak)
	{
		Out += FString::Printf(TEXT("break\t%d\t%.2f\n"), BreakLevel, BreakT);
	}
	for (const FString& Id : LiveMet)
	{
		Out += TEXT("met\t") + Id + TEXT("\n");
	}
	// The night's own departure, for an event entered from the room (Night One catches its calendar up from it).
	if (NightLeftHomeAt < LeftHomeAt - 0.5)
	{
		Out += FString::Printf(TEXT("from\t%.2f\n"), NightLeftHomeAt);
	}
	// The big pots shown down, and the faces people have worn tonight (so a night come back to looks the same).
	for (const TPair<FString, int64>& P : LiveBigPots)
	{
		Out += FString::Printf(TEXT("pot\t%s\t%lld\n"), *P.Key, P.Value);
	}
	for (const TPair<FString, FString>& L : LiveBodies)
	{
		Out += FString::Printf(TEXT("look\t%s\t%s\n"), *L.Key, *L.Value);
	}
	if (Table && Save.IsValid())
	{
		// The reads that changed tonight (the rest are in the save already).
		for (const TPair<FString, int32>& R : Table->Reads)
		{
			const auto Was = Save->Life.Reads.find(std::string(TCHAR_TO_UTF8(*R.Key)));
			if (Was == Save->Life.Reads.end() || Was->second != R.Value)
			{
				Out += FString::Printf(TEXT("read\t%s\t%d\n"), *R.Key, R.Value);
			}
		}
	}
	// The last hands the player played (ss::recall: the table card's HANDS page when the night comes back; ue-table's
	// helpers, BackRoomTableHost.h).
	Out += TableRecall::HostLines(RecentHands);
	return Out;
}

void ABackRoomGameMode::ReadLiveHost(const FString& Text)
{
	TArray<FString> Lines;
	Text.ParseIntoArray(Lines, TEXT("\n"), true);
	for (const FString& Line : Lines)
	{
		TArray<FString> F;
		Line.ParseIntoArray(F, TEXT("\t"), false);
		if (TableRecall::ReadHostLine(F, RecentHands))
		{
			continue;
		}
		if (F.Num() == 2 && F[0] == TEXT("hands"))
		{
			HandsPlayed = FCString::Atoi(*F[1]);
		}
		else if (F.Num() == 2 && F[0] == TEXT("dealt"))
		{
			bResumeDealt = F[1] == TEXT("1");
		}
		else if (F.Num() == 3 && F[0] == TEXT("break"))
		{
			bOnBreak = true;
			BreakLevel = FCString::Atoi(*F[1]);
			BreakT = FMath::Clamp(FCString::Atof(*F[2]), 0.0f, BreakRealSeconds - 3.0f);
		}
		else if (F.Num() == 2 && F[0] == TEXT("met"))
		{
			LiveMet.Add(F[1]);
		}
		else if (F.Num() == 3 && F[0] == TEXT("read"))
		{
			ResumeReads.Add(F[1], FCString::Atoi(*F[2]));
		}
		else if (F.Num() == 2 && F[0] == TEXT("from"))
		{
			NightLeftHomeAt = FMath::Min(NightLeftHomeAt, FCString::Atod(*F[1]));
		}
		else if (F.Num() == 3 && F[0] == TEXT("pot"))
		{
			LiveBigPots.Add(F[1], FCString::Atoi64(*F[2]));
		}
		else if (F.Num() == 3 && F[0] == TEXT("look"))
		{
			LiveBodies.Add(F[1], F[2]);
		}
	}
}

void ABackRoomGameMode::LiveMove()
{
	ABackRoomPawn* Pawn = Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	const ss::TPlayer& H = Tourney->Hero();
	// The table the player was moved from, by its engine id (Bug 2, next_segment_spec 6.8): the room's host keeps the last
	// one it saw, and the engine's Moved event names it; the room's anchor is a slot (the final table's is the stage's),
	// and by now it may already be the new table's.
	int32 FromTable = -1;
	if (RoomHost.IsValid())
	{
		// The Moved event's own From when it came this engine tick (LiveEvent, or the test move), else the table the room
		// last saw the player at (the table's note can come before the event's frame).
		const int32 Moved = RoomHost->MovedTick == Tourney->Tick ? RoomHost->MovedFrom : -1;
		FromTable = Moved > 0 && Moved != H.TableId ? Moved : (RoomHost->HeroTable > 0 && RoomHost->HeroTable != H.TableId ? RoomHost->HeroTable : -1);
		RoomHost->MovedFrom = -1;
	}
	if (bHeroUp && Pawn && Pawn->IsFreeWalking() && Stage)
	{
		// Away from the table when it broke: the floor carries the chips over, the player finds the new seat. The floor says
		// so (the director's "moved" call, from the engine's own Moved event; a host without one says it here).
		const FVector Before = Stage->RoomToWorld(FVector::ZeroVector);
		Stage->SetRoomAnchor(SlotForTable(H.TableId));
		Pawn->ShiftWalk(Stage->RoomToWorld(FVector::ZeroVector) - Before);
		Table->PrepareNext();
		PlaceExtras();
		Table->Resume(1.0f);
		if (!RoomDirector.IsValid())
		{
			Floor(FromTable > 0 ? FString::Printf(TEXT("Player from table %d, you've been moved: table %d, seat %d. Your chips are there."), FromTable, H.TableId, ss::live::SeatLabel(H.Seat))
								: FString::Printf(TEXT("Player, you've been moved: table %d, seat %d. Your chips are there."), H.TableId, ss::live::SeatLabel(H.Seat)),
				false);
		}
		RaiseBanner(FString::Printf(TEXT("TABLE %d   SEAT %d"), H.TableId, ss::live::SeatLabel(H.Seat)));
		UE_LOG(LogEmbercrest, Log, TEXT("Moved while up: from table %d to table %d, seat %d"), FromTable, H.TableId, ss::live::SeatLabel(H.Seat));
		return;
	}
	Phase = EBackRoomPhase::Moving;
	MoveT = 0.0f;
	// (Seated, the player's dealer tells them: the director's "move" call at the old table, Dee's "kid" on a Sunday; at the
	// final table the floor's call carries everyone. A host without a director says it as it always did.)
	if (!RoomDirector.IsValid())
	{
		const bool bDee = ((LiveDay % 7) + 7) % 7 == 6;
		Table->DealerLine(FString::Printf(TEXT("You're moving%s. Table %d, seat %d. Grab your chips."), bDee ? TEXT(", kid") : TEXT(""), H.TableId, ss::live::SeatLabel(H.Seat)));
	}
	RaiseBanner(FString::Printf(TEXT("TABLE %d   SEAT %d"), H.TableId, ss::live::SeatLabel(H.Seat)));
	UE_LOG(LogEmbercrest, Log, TEXT("Moved: from table %d to table %d, seat %d"), FromTable, H.TableId, ss::live::SeatLabel(H.Seat));
	if (!Pawn)
	{
		Table->PrepareNext();
		Phase = EBackRoomPhase::Playing;
		Table->Resume(1.0f);
		return;
	}
	// Up from the seat and along the aisle toward the new table...
	const FVector Eye = Pawn->GetEye();
	const int32 FromSlot = Stage ? Stage->GetRoomAnchor() : 0;
	const int32 ToSlot = SlotForTable(H.TableId);
	const TArray<FVector> Out = Stage ? Stage->CardRoomMoveOut(Eye, ToSlot) : TArray<FVector>{Eye, Eye + FVector(-34.0, -16.0, 44.0), FVector(-262.0, -60.0, 166.0)};
	TWeakObjectPtr<ABackRoomGameMode> Self = this;
	Pawn->PlayWalk(Out, 4.0f, true, [Self, FromSlot, ToSlot]() {
		ABackRoomGameMode* GM = Self.Get();
		if (!GM)
		{
			return;
		}
		// ...a blink, and the room has turned: a different corner of it, the new table's faces.
		APlayerController* PC = GM->GetWorld()->GetFirstPlayerController();
		if (PC && PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, 0.35f, FLinearColor::Black, true, true);
		}
		FTimerHandle Handle;
		GM->GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(GM, [GM, FromSlot, ToSlot]() {
			GM->Stage->SetRoomAnchor(ToSlot);
			GM->Table->PrepareNext();
			GM->PlaceExtras();
			APlayerController* PC2 = GM->GetWorld()->GetFirstPlayerController();
			if (PC2 && PC2->PlayerCameraManager)
			{
				PC2->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 0.6f, FLinearColor::Black, true, false);
			}
			ABackRoomPawn* P = Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(GM, 0));
			const FVector SeatEye = GM->Stage ? GM->Stage->EyeLocation() : FVector(-91.0, 0.0, 118.0);
			const TArray<FVector> In = GM->Stage->CardRoomMoveIn(SeatEye, FromSlot);
			TWeakObjectPtr<ABackRoomGameMode> Self2 = GM;
			if (P)
			{
				P->PlayWalk(In, 3.6f, false, [Self2]() {
					if (ABackRoomGameMode* G = Self2.Get())
					{
						G->Phase = EBackRoomPhase::Playing;
						G->Table->Resume(1.2f);
						G->Table->DealerLine(TEXT("Welcome to the table."));
					}
				});
			}
		}), 0.45f, false);
	});
}

void ABackRoomGameMode::LiveRequestLeave()
{
	if (Phase != EBackRoomPhase::Playing || bLiveOver)
	{
		return;
	}
	if (QuitAskedAt < 0.0f)
	{
		QuitAskedAt = LiveT;
		Table->DealerLine(((LiveDay % 7) + 7) % 7 == 6 ? TEXT("You sure, kid? Walk now and your stack gets blinded off.") : TEXT("You sure? Walk now and your stack gets blinded off."));
		return;
	}
	// The room's last round is still being finished (a big field's tables play over several frames): leave once it is.
	if (Tourney->FinishPending() || !Table->IsBetweenHands())
	{
		bLeaveWhenFree = true;
		return;
	}
	// Gone: the stack sits there and the blinds take it, hand by hand, until it's out (or it limps into
	// the money on its own).
	bLeaveWhenFree = false;
	QuitAskedAt = -1.0f;
	Tourney->HeroSitsOut = true;
	// (The director keeps up with the blind-off; only its last round is news.)
	const bool bHeard = RoomHost.IsValid() && RoomDirector.IsValid();
	if (bHeard)
	{
		RoomFeed::BeginCatchUp(*RoomHost);
	}
	for (int32 Guard = 0; Guard < 4000 && !Tourney->Hero().Busted && !Tourney->bFinished; ++Guard)
	{
		const std::vector<ss::TEvent> Round = Tourney->SimulateTick();
		if (bHeard)
		{
			RoomFeed::Simulated(*RoomHost, *RoomDirector, RoomSound.Get(), *Tourney, Round, LiveDayStart + Tourney->ClockMinutes());
		}
		for (const ss::TEvent& E : Round)
		{
			if (E.Type == ss::TEventType::Bust && E.IsHero)
			{
				HeroPlace = E.Place;
				HeroPrizeCents = E.PrizeCents;
			}
		}
	}
	if (bHeard)
	{
		RoomFeed::EndCatchUp(*RoomHost);
	}
	if (!Tourney->Hero().Busted)
	{
		HeroPlace = Tourney->Hero().Place > 0 ? Tourney->Hero().Place : 1;
		HeroPrizeCents = Tourney->Hero().PrizeCents;
	}
	Minutes = LiveDayStart + Tourney->ClockMinutes();
	LiveOver();
}

void ABackRoomGameMode::LiveOver()
{
	if (bLiveOver)
	{
		return;
	}
	bLiveOver = true;
	BustMinutes = Minutes;
	if (bHeroUp)
	{
		// Out while on foot (blinded off, or heading for the door): already up, so already on the rail, and the old table
		// catches up with the engine at once (a blind-off plays many hands in one go).
		bRailing = true;
		bRailSeatsDue = true;
		RailT = RailSettleSeconds;
	}
	UE_LOG(LogEmbercrest, Log, TEXT("Over: phase %d, break %d, hero busted %d, finished %d, moving %d"), static_cast<int32>(Phase), bOnBreak ? 1 : 0, Tourney->Hero().Busted ? 1 : 0, Tourney->bFinished ? 1 : 0, MoveT >= 0.0f ? 1 : 0);
	const int32 Field = Tourney->Spec.Entrants;
	if (bCancelled)
	{
		// Nothing was played: the entry comes back, and the bus home.
		HeroPlace = 0;
		HeroPrizeCents = 0;
		NetCents = 0;
		bBustedOut = true;
		LiveSettle();
		Phase = EBackRoomPhase::Leaving;
		LeaveT = 0.0f;
		Summary.Reset();
		Summary.Add(LiveName.ToUpper());
		Summary.Add(FString::Printf(TEXT("Cancelled   \u00b7   %d players"), Field));
		Summary.Add(FString::Printf(TEXT("Refunded   %s"), *Dollars(BoughtInCents)));
		Table->DealerLine(TEXT("Not enough of us tonight. The desk has your money."));
		return;
	}
	if (HeroPlace <= 0)
	{
		HeroPlace = Tourney->Hero().Place > 0 ? Tourney->Hero().Place : Tourney->HeroRank();
		HeroPrizeCents = Tourney->PrizeFor(HeroPlace);
	}
	NetCents = HeroPrizeCents - BoughtInCents;
	bBustedOut = HeroPrizeCents <= 0;
	// Which reads became yours tonight.
	TArray<FString> Learned;
	for (const TPair<FString, int32>& R : Table->Reads)
	{
		const auto Was = Save->Life.Reads.find(std::string(TCHAR_TO_UTF8(*R.Key)));
		const int32 Before = Was == Save->Life.Reads.end() ? 0 : Was->second;
		if (R.Value >= 2 && Before < 2)
		{
			FString Who, TellName;
			R.Key.Split(TEXT("/"), &Who, &TellName);
			const int64 Value = StaticEnum<EBackRoomTell>()->GetValueByNameString(TellName);
			Learned.Add(FString::Printf(TEXT("Read learned   %s: %s"), *Who, *ABackRoomTable::TellPhrase(static_cast<uint8>(FMath::Max<int64>(Value, 0)))));
		}
	}
	LiveSettle();
	Phase = EBackRoomPhase::Leaving;
	LeaveT = 0.0f;
	Summary.Reset();
	Summary.Add(bHeroWon ? LiveName.ToUpper() + TEXT("   CHAMPION") : LiveName.ToUpper());
	const int32 Sat = FMath::Max(0, FMath::FloorToInt(Minutes - LeftHomeAt - 20.0));
	Summary.Add(FString::Printf(TEXT("%s of %d   ·   %d hands   ·   %dh %02dm"), *Ordinal(HeroPlace), Field, HandsPlayed, Sat / 60, Sat % 60));
	Summary.Add(HeroPrizeCents > 0 ? FString::Printf(TEXT("Cashed   %s"), *Dollars(HeroPrizeCents)) : FString::Printf(TEXT("No cash   ·   %d paid"), Tourney->PaidPlaces()));
	Summary.Add(FString::Printf(TEXT("Net   %s%s"), NetCents >= 0 ? TEXT("+") : TEXT("-"), *Dollars(FMath::Abs(NetCents))));
	Summary.Append(Learned);
	// The rest of today at the desk, if there's an event the player could still get into.
	FString WhyNot;
	const ss::live::Occurrence Next = ss::live::FindOccurrence(std::string(TCHAR_TO_UTF8(*NextOpenEvent(WhyNot))));
	if (Next.Valid())
	{
		const FString When = Minutes < Next.Start ? TEXT("cards at ") + Str(ss::net::TimeLabel(Next.Start)) : TEXT("late reg to ") + Str(ss::net::TimeLabel(Next.LateRegEnds));
		Summary.Add(FString::Printf(TEXT("Still open at the desk   %s, %s"), *Str(Next.T->Name), *When));
	}
	// The dealer, from the box (Dee on a Sunday; the day's dealer otherwise).
	const bool bDee = ((LiveDay % 7) + 7) % 7 == 6;
	FString Line;
	if (bHeroWon)
	{
		Line = bDee ? TEXT("Kid. You won the whole thing. Go get your money before I cry.") : TEXT("You won the whole thing. Congratulations. The cage is waiting on you.");
	}
	else if (HeroPrizeCents > 0)
	{
		Line = FString::Printf(TEXT("%s. That's a cash%s. The cage is behind you."), *Ordinal(HeroPlace), bDee ? TEXT(", kid") : TEXT(""));
	}
	else if (HeroPlace <= Tourney->PaidPlaces() + 3)
	{
		Line = TEXT("Right before the money. That one stings. Go home, sleep it off.");
	}
	else
	{
		Line = bDee ? FString::Printf(TEXT("%s of %d. Happens. See you Tuesday."), *Ordinal(HeroPlace), Field)
					: FString::Printf(TEXT("%s of %d. Happens. Come see us again."), *Ordinal(HeroPlace), Field);
	}
	Table->DealerLine(Line);
	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
	UE_LOG(LogEmbercrest, Log, TEXT("Embercrest over: %s of %d, prize %lld cents, net %lld, %d hands"), *Ordinal(HeroPlace), Field, HeroPrizeCents, NetCents, HandsPlayed);
}

// ------------------------------------------------------------------ on foot

void ABackRoomGameMode::LiveStandUp()
{
	ABackRoomPawn* Pawn = Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!bLive || !Stage || !Pawn || bHeroUp || bLiveOver || Phase != EBackRoomPhase::Playing || Pawn->IsWalking() || Pawn->IsFreeWalking())
	{
		return;
	}
	bHeroUp = true;
	Table->bHeroAway = true;
	if (!bAwayExplained && !bOnBreak)
	{
		bAwayExplained = true;
		Table->DealerLine(TEXT("You're still dealt in while you're up. I check it when it's free and muck it when it isn't."));
	}
	TWeakObjectPtr<ABackRoomPawn> Walker = Pawn;
	Pawn->PlayWalk(Stage->CardRoomMoveOut(Pawn->GetEye(), Stage->GetRoomAnchor()), 1.8f, true, [Walker]() {
		if (ABackRoomPawn* W = Walker.Get())
		{
			W->BeginFreeWalk();
		}
	});
	UE_LOG(LogEmbercrest, Log, TEXT("Up from the table (hand %d, %s)"), Tourney->Tick, bOnBreak ? TEXT("on a break") : TEXT("dealt in while away"));
}

void ABackRoomGameMode::LiveSitDown()
{
	ABackRoomPawn* Pawn = Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Pawn || !Stage || !bHeroUp || bLiveOver)
	{
		return;
	}
	Pawn->EndFreeWalk();
	TArray<FVector> In = Stage->CardRoomMoveIn(Stage->EyeLocation(), Stage->GetRoomAnchor());
	In.Insert(Pawn->GetEye(), 0);
	TWeakObjectPtr<ABackRoomGameMode> Self = this;
	Pawn->PlayWalk(In, 2.8f, false, [Self]() {
		if (ABackRoomGameMode* GM = Self.Get())
		{
			GM->bHeroUp = false;
			GM->Table->bHeroAway = false;
			// The dealer says it most times (a hash of the hand, not a dice roll: next_segment_spec N5), in the room's copy.
			const uint32 Gate = FCrc::StrCrc32(*FString::Printf(TEXT("back:%s:%d"), *GM->LiveEntryId, GM->Table->GetHandNumber()));
			if (!GM->bOnBreak && Gate % 10u < 6u)
			{
				ss::talk::Context C;
				C.Salt = Gate;
				if (((GM->LiveDay % 7) + 7) % 7 == 6)
				{
					C.Vars["Dealer"] = "Dee";
				}
				const ss::talk::Said Back = ss::talk::Dealer("back", C);
				if (!Back.Text.empty())
				{
					GM->Table->DealerLine(Str(Back.Text));
				}
			}
		}
	});
	UE_LOG(LogEmbercrest, Log, TEXT("Back in the chair (hand %d)"), Tourney->Tick);
}

void ABackRoomGameMode::LiveStay()
{
	ABackRoomPawn* Pawn = Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!bLive || !Stage || !Pawn || Phase != EBackRoomPhase::Leaving || bRailing || bWalkingOut || bExitNow || Pawn->IsWalking() || bCancelled)
	{
		return;
	}
	bRailing = true;
	RailT = 0.0f;
	if (bHeroUp)
	{
		// Already on foot (out while walking about): just stay.
		if (!Pawn->IsFreeWalking())
		{
			Pawn->BeginFreeWalk();
		}
	}
	else
	{
		bHeroUp = true;
		TWeakObjectPtr<ABackRoomPawn> Walker = Pawn;
		Pawn->PlayWalk(Stage->CardRoomMoveOut(Pawn->GetEye(), Stage->GetRoomAnchor()), 1.8f, true, [Walker]() {
			if (ABackRoomPawn* W = Walker.Get())
			{
				W->BeginFreeWalk();
			}
		});
	}
	if (HeroPrizeCents > 0)
	{
		Table->DealerLine(TEXT("Stick around. The cage is open all night."));
	}
	// The old table as the engine has it now (whoever went out with the player's last hand gets up).
	const int32 Here = AnchorTableId();
	Table->FollowTable(Here, Here == Tourney->Hero().TableId ? Tourney->Hero().Seat : 0, true);
	PlaceExtras();
	UE_LOG(LogEmbercrest, Log, TEXT("Staying to watch: %d left"), Tourney->Remaining);
}

void ABackRoomGameMode::LiveLeaveNow()
{
	if (Phase != EBackRoomPhase::Leaving || bWalkingOut)
	{
		return;
	}
	if (bRailing)
	{
		bExitNow = true;
		return;
	}
	LeaveT = FMath::Max(LeaveT, 31.0f);
}

float ABackRoomGameMode::StayOffer() const
{
	return bLive && Phase == EBackRoomPhase::Leaving && !bRailing && !bWalkingOut && !bExitNow && !bCancelled && LeaveT < 30.0f ? LeaveT : -1.0f;
}

void ABackRoomGameMode::LiveSettle()
{
	if (!Save.IsValid())
	{
		return;
	}
	ss::SaveData D = *Save;
	const std::string EntryId(TCHAR_TO_UTF8(*LiveEntryId));
	if (bCancelled)
	{
		ss::live::Refund(D.BankrollCents, D.Life, EntryId, Minutes, "not enough players");
	}
	else
	{
		// Once: a second call (or a reload after this save) finds the entry settled and changes nothing.
		ss::live::Settle(D.BankrollCents, D.Life, EntryId, HeroPlace, Tourney->Spec.Entrants, HeroPrizeCents, Minutes);
	}
	// The bus home. An event the desk wrote the player into during the night rides home on the night's first ticket,
	// paid when that one ended: one trip, one fare each way.
	if (ss::life::LiveEntry* Mine = bDeskEntry ? ss::live::EntryFor(D.Life, EntryId) : nullptr)
	{
		Mine->FareHome = true;
	}
	else
	{
		ss::live::PayFare(D.BankrollCents, D.Life, EntryId, true, Minutes);
	}
	D.ClockMinutes = Minutes - ss::net::MinutesPerDay * ss::net::NightOneDay + 25.0;
	D.Life.Energy = FMath::Clamp(static_cast<double>(Energy), 0.0, 100.0);
	for (const TPair<FString, int32>& R : Table->Reads)
	{
		D.Life.Reads[std::string(TCHAR_TO_UTF8(*R.Key))] = R.Value;
	}
	if (!bCancelled)
	{
		// What the player's tables saw, for the living world: who busted before them and where, who was still in,
		// who sat with them, who sent them home and whom they sent home. Only what happened in the open.
		std::vector<ss::LiveSeen> Seen;
		const ss::TPlayer& Me = Tourney->Hero();
		for (const ss::TPlayer& P : Tourney->Players)
		{
			if (P.IsHero || P.Id.rfind("npc:", 0) != 0)
			{
				continue; // the room's anonymous regulars: nobody the world follows
			}
			ss::LiveSeen S;
			S.Name = P.Name;
			if (P.Busted && P.Place > HeroPlace)
			{
				S.Tag = 'P';
				S.Value = P.Place;
				Seen.push_back(S);
			}
			else if (!P.Busted || P.Place < HeroPlace)
			{
				S.Tag = 'S';
				Seen.push_back(S);
			}
			if (LiveMet.Contains(Str(P.Id)))
			{
				S.Tag = 'M';
				S.Value = 0;
				Seen.push_back(S);
			}
			if (P.KnockedOutBy == ss::HeroId)
			{
				S.Tag = 'H';
				Seen.push_back(S);
			}
			if (Me.Busted && Me.KnockedOutBy == P.Id)
			{
				S.Tag = 'K';
				Seen.push_back(S);
			}
			// The biggest pot shown down with them tonight (the player's net, in chips): a memory, as at Dee's game.
			if (const int64* Net = LiveBigPots.Find(Str(P.Id)); Net && *Net != 0)
			{
				S.Tag = 'B';
				S.Value = *Net;
				Seen.push_back(S);
			}
		}
		D.NoteLive(Minutes, EntryId, HeroPlace, HeroPrizeCents, Tourney->Spec.Entrants, Seen);
		UE_LOG(LogEmbercrest, Log, TEXT("Told the world: %d things seen, %d big pots"), static_cast<int32>(Seen.size()),
			static_cast<int32>(std::count_if(Seen.begin(), Seen.end(), [](const ss::LiveSeen& S) { return S.Tag == 'B'; })));
	}
	*Save = D;
	// A checkpoint still being written lands first, so the settled night is the save that stays.
	if (LiveSaver)
	{
		LiveSaver->Flush();
	}
	CareerSave::SaveNow(D.Serialize());
}

void ABackRoomGameMode::LiveGoHome()
{
	if (bGoingHome || bOpeningNext)
	{
		return; // (on the way already: home, or into the next event's room)
	}
	// Never home with a seat waiting tonight: the desk takes the player off the next event's list first.
	if (!NextEntryId.IsEmpty() && !bOpeningNext)
	{
		LiveUnregister();
	}
	bGoingHome = true;
	ABackRoomChips::ChipUnit = 1;
	FString Options = FString::Printf(TEXT("Home?Live=1?Place=%d?Of=%d?Prize=%lld?Net=%lld?Learned=%d?From=%.0f"), HeroPlace, Tourney->Spec.Entrants, HeroPrizeCents, NetCents,
		Table ? Table->LearnedThisNight : 0, NightLeftHomeAt);
	if (bHeroWon)
	{
		Options += TEXT("?Won");
	}
	if (bCancelled)
	{
		Options += TEXT("?Cancelled");
	}
	Options += TEXT("?Event=") + LiveShort.Replace(TEXT(" "), TEXT("_"));
	// Out through the resort's entrance to the bus stop where it's been built (it opens Night One with the rest of these
	// once the player boards); else straight home.
	const bool bOutside = FPackageName::DoesPackageExist(TEXT("/Game/Maps/Embercrest"));
	UE_LOG(LogEmbercrest, Log, TEXT("%s: %s"), bOutside ? TEXT("Out through the entrance (Embercrest)") : TEXT("Home (NightOne)"), *Options);
	UGameplayStatics::OpenLevel(this, FName(bOutside ? TEXT("Embercrest") : TEXT("NightOne")), true, bOutside ? Options + TEXT("?Leaving") : Options);
}

void ABackRoomGameMode::LiveTick(float RealDt)
{
	LiveT += RealDt;
	BannerAge += RealDt;
	ABackRoomPawn* Pawn = Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	TWeakObjectPtr<ABackRoomGameMode> Self = this;

	const TArray<ss::TEvent> TableNews = Table->TakeEvents();
	for (const ss::TEvent& E : TableNews)
	{
		LiveEvent(E);
	}
	// The room hears the same news (with the hand that finished and every table's round), says what it says, and sounds
	// like itself from where the player is (BackRoomLiveRoom.cpp).
	if (RoomHost.IsValid())
	{
		RoomHost->TableEvents.insert(RoomHost->TableEvents.end(), TableNews.GetData(), TableNews.GetData() + TableNews.Num());
	}
	TickRoom(RealDt);
	// The hand's news heard: save the night where it stands (before the next hand is dealt).
	if (LiveSaver)
	{
		LiveSaver->Tick();
	}
	if (bCheckpointDue && Table->IsBetweenHands() && !Tourney->FinishPending())
	{
		LiveCheckpoint(false);
	}
	// A blink in progress: the bodies swap with the eyes shut, then they open.
	if (BlinkT >= 0.0f)
	{
		BlinkT += RealDt;
		if (BlinkT >= 0.1f || MoveT >= 0.0f)
		{
			ApplyBlinkSwaps();
			BlinkT = -1.0f;
			if (PC && PC->PlayerCameraManager && MoveT < 0.0f)
			{
				PC->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 0.16f, FLinearColor::Black, false, false);
			}
		}
	}
	// (Testing: chips, a forced move, between hands.)
	if (Phase == EBackRoomPhase::Playing && Table->IsBetweenHands() && (TestChips != 0 || bTestMove))
	{
		Tourney->Players[0].Stack = FMath::Max<int64>(1, Tourney->Players[0].Stack + TestChips);
		TestChips = 0;
		if (bTestMove)
		{
			bTestMove = false;
			for (const auto& It : Tourney->Tables)
			{
				if (It.first != Tourney->Hero().TableId)
				{
					// (The move is news like any balance: the host's part now, the room's next frame. Bug 2's check: on foot,
					// the floor names the table the player left.)
					const std::vector<ss::TEvent> Moves = Tourney->MoveToTable(ss::HeroId, It.first);
					for (const ss::TEvent& E : Moves)
					{
						if (E.Type == ss::TEventType::Moved && E.IsHero && RoomHost.IsValid())
						{
							RoomHost->HeroTable = E.From;
							RoomHost->MovedFrom = E.From;
							RoomHost->MovedTick = Tourney->Tick;
						}
					}
					if (RoomHost.IsValid())
					{
						RoomHost->TableEvents.insert(RoomHost->TableEvents.end(), Moves.begin(), Moves.end());
					}
					Table->Hold();
					LiveMove();
					break;
				}
			}
		}
	}
	// (Testing: the whole room plays on at once, you on autopilot, between hands and once the last round's tables
	// have all finished: nothing may touch the tournament while a round is being finished over several frames.)
	if (FastForwardLeft > 0 && Phase == EBackRoomPhase::Playing && Table->IsBetweenHands() && !Tourney->FinishPending())
	{
		ss::Profile Auto = ss::MakeProfile(ss::Archetype::Tag, Tourney->R);
		bool bMoved = false;
		bool bThere = false;
		// The director keeps up with every hand played (nothing it's said is said again), and the room hears only the last
		// one's news, as at the rail's catch-up.
		const bool bHeard = RoomHost.IsValid() && RoomDirector.IsValid();
		if (bHeard)
		{
			RoomDirector->HeroUp = bHeroUp;
			RoomFeed::BeginCatchUp(*RoomHost);
		}
		for (; FastForwardLeft > 0 && !Tourney->Hero().Busted && !Tourney->bFinished && !bMoved && !bThere; --FastForwardLeft)
		{
			const std::vector<ss::TEvent> Round = Tourney->SimulateTick(&Auto);
			if (bHeard)
			{
				RoomFeed::Simulated(*RoomHost, *RoomDirector, RoomSound.Get(), *Tourney, Round, LiveDayStart + Tourney->ClockMinutes());
			}
			for (const ss::TEvent& E : Round)
			{
				bMoved = bMoved || (E.Type == ss::TEventType::Moved && E.IsHero);
				LiveEvent(E);
			}
			bThere = FastForwardStop != 0 && FastForwardReached();
		}
		if (bHeard)
		{
			RoomFeed::EndCatchUp(*RoomHost);
		}
		// A count ends here, moved or not; a fast-forward to a moment (ss.Live.Bubble, Final, Break) goes on after the walk.
		FastForwardLeft = FastForwardStop != 0 && bMoved && !bThere ? FastForwardLeft : 0;
		FastForwardStop = FastForwardLeft > 0 ? FastForwardStop : 0;
		UE_CLOG(bThere, LogEmbercrest, Log, TEXT("Fast-forward: there at hand %d, level %d, %d left, %d tables"), Tourney->Tick, Tourney->LevelIndex + 1, Tourney->Remaining,
			static_cast<int32>(Tourney->Tables.size()));
		Minutes = FMath::Max(Minutes, LiveDayStart + Tourney->ClockMinutes());
		bCheckpointDue = true;
		if (Tourney->Hero().Busted || Tourney->bFinished)
		{
			LiveOver();
			return;
		}
		if (bMoved)
		{
			Table->Hold();
			LiveMove();
		}
		else
		{
			PlaceExtras();
		}
	}
	// (Testing: out at the next gap between hands: one chip left and sitting out, the engine's blinds take it.)
	if (bTestBust && Phase == EBackRoomPhase::Playing && Table->IsBetweenHands() && !Tourney->FinishPending())
	{
		bTestBust = false;
		Tourney->Players[0].Stack = 1;
		Tourney->HeroSitsOut = true;
		const bool bHeard = RoomHost.IsValid() && RoomDirector.IsValid();
		if (bHeard)
		{
			RoomFeed::BeginCatchUp(*RoomHost);
		}
		for (int32 Guard = 0; Guard < 400 && !Tourney->Hero().Busted && !Tourney->bFinished; ++Guard)
		{
			const std::vector<ss::TEvent> Round = Tourney->SimulateTick();
			if (bHeard)
			{
				RoomFeed::Simulated(*RoomHost, *RoomDirector, RoomSound.Get(), *Tourney, Round, LiveDayStart + Tourney->ClockMinutes());
			}
			for (const ss::TEvent& E : Round)
			{
				LiveEvent(E);
			}
		}
		if (bHeard)
		{
			RoomFeed::EndCatchUp(*RoomHost);
		}
		Tourney->HeroSitsOut = false;
		Minutes = FMath::Max(Minutes, LiveDayStart + Tourney->ClockMinutes());
		UE_LOG(LogEmbercrest, Log, TEXT("Test bust: %s of %d"), *Ordinal(Tourney->Hero().Place), Tourney->Spec.Entrants);
		LiveOver();
		return;
	}
	// The desk's signup, once its worker has read the world's field for the event.
	if (Signup.IsValid() && Signup->bDone)
	{
		FinishSignup();
	}
	if (bAnnouncedStart && Phase != EBackRoomPhase::Arriving)
	{
		LiveLastCall();
	}
	UpdateBoard(RealDt);

	// Walking in from the casino floor.
	ArrivalT += RealDt;
	if (Phase == EBackRoomPhase::Arriving)
	{
		StartWalkIn(Pawn);
		if (ArrivalBeat == 1 && ArrivalT > 3.2f)
		{
			ArrivalBeat = 2;
			// Dee deals the Sunday; any other day it's whoever's on the stick at the player's table.
			const bool bSunday = ((LiveDay % 7) + 7) % 7 == 6;
			if (bHere)
			{
				// On from the night's earlier event.
				Table->DealerLine(bSunday ? FString(TEXT("Back for another one, kid? You're with me."))
										  : FString::Printf(TEXT("Back for another one? Table %d, seat %d. Good luck."), Tourney->Hero().TableId, ss::live::SeatLabel(Tourney->Hero().Seat)));
			}
			else if (bBackInRoom && Tourney->Tick > 0)
			{
				Table->DealerLine(bResumeDealt ? FString(TEXT("Welcome back. Your hand was dead while you were up, you're in the next one."))
											   : bSunday ? FString(TEXT("There you are. Sit down, kid, your chips didn't go anywhere."))
														 : FString(TEXT("Welcome back. Your chips are right where you left them.")));
			}
			else
			{
				Table->DealerLine(bSunday ? FString(TEXT("Look who made it up the mountain. You're with me tonight, kid."))
										  : FString::Printf(TEXT("Evening. Table %d, seat %d, you're all set. Good luck."), Tourney->Hero().TableId, ss::live::SeatLabel(Tourney->Hero().Seat)));
			}
		}
		AttentionTick -= RealDt;
		if (Pawn && AttentionTick <= 0.0f)
		{
			AttentionTick = 0.5f;
			for (ABackRoomPlayer* O : Table->GetOpponents())
			{
				O->OnOtherAction(Pawn->GetEye(), 0.2f, false);
			}
		}
	}
	// Too few players: the floor cancels the event and the desk refunds everyone.
	if (bCancelled && Phase == EBackRoomPhase::Playing && !bLiveOver)
	{
		// The director never learns of a cancellation: the floor's copy (talk::Floor), said by the host in its parts.
		std::map<std::string, std::string> Vars;
		Vars["Players"] = std::to_string(Tourney->Spec.Entrants);
		Vars["Short"] = std::string(TCHAR_TO_UTF8(*LiveShort));
		const ss::talk::Said Cancelled = ss::talk::Floor("cancelled", Vars, 0u);
		Floor(Cancelled.Text.empty() ? FString::Printf(TEXT("Ladies and gentlemen, with %d players tonight's %s is cancelled. Please see the desk for your refund."),
										   Tourney->Spec.Entrants, *LiveShort)
									 : Str(Cancelled.Text));
		HeroPlace = 0;
		HeroPrizeCents = 0;
		LiveOver();
		return;
	}
	// Seated: the night starts (or a late seat is welcomed). The floor's words are the director's (its Clock says the start,
	// the late welcome or the closed one, from where registration stands); the banners are the host's.
	if (Phase == EBackRoomPhase::Playing && !bAnnouncedStart)
	{
		bAnnouncedStart = true;
		if (Tourney->Tick == 0)
		{
			RaiseBanner(TEXT("SHUFFLE UP AND DEAL"));
		}
		else
		{
			RaiseBanner(FString::Printf(TEXT("LEVEL %d   %s / %s"), Tourney->LevelIndex + 1, *Num(Tourney->CurrentLevel().Sb), *Num(Tourney->CurrentLevel().Bb)));
		}
		if (!RoomDirector.IsValid())
		{
			// (A host without a director: the floor as it always started the night.)
			Floor(Tourney->Tick == 0 ? FString::Printf(TEXT("Ladies and gentlemen, welcome to the Embercrest %s. %d players, a prize pool of %s. Dealers, shuffle up and deal!"),
										   *LiveShort, Tourney->Spec.Entrants, *Dollars(Tourney->PrizePoolCents))
									 : FString::Printf(TEXT("Late registration closes at %s. %d players, %s in the pool."), *Str(ss::net::TimeLabel(LiveLateRegEnds)),
										   Tourney->Spec.Entrants, *Dollars(Tourney->PrizePoolCents)));
		}
	}

	// The night's clock is the tournament's; it wears you down.
	if (Phase == EBackRoomPhase::Playing || Phase == EBackRoomPhase::Moving)
	{
		// The tournament's clock, running on through each hand (about forty-five seconds a hand here).
		const double Was = Minutes;
		const double PerHand = Tourney->Spec.SecondsPerHand;
		const double Progress = FMath::Min(static_cast<double>(Table->HandAge()) * PerHand / 45.0, PerHand - 1.0);
		Minutes = FMath::Max(Minutes, LiveDayStart + Tourney->ClockMinutes() + Progress / 60.0);
		Energy = FMath::Max(0.0f, Energy - static_cast<float>((Minutes - Was) / 60.0) * 4.0f);
		// The money, close: everything matters more.
		const float Pressure = bHandForHand ? 14.0f : (bFinalTable ? 10.0f : (Tourney->Remaining <= Tourney->PaidPlaces() + 6 ? 6.0f : 0.0f));
		Table->ExtraPressure = Pressure;
		if (Energy < 22.0f)
		{
			const float Tired = 1.0f - Energy / 22.0f;
			NextDroop -= RealDt;
			if (DroopT < 0.0f && NextDroop <= 0.0f)
			{
				DroopT = 0.0f;
				DroopLength = FMath::FRandRange(0.5f, 0.8f) + 0.9f * Tired;
				DroopDepth = FMath::Min(1.0f, FMath::FRandRange(0.4f, 0.7f) + 0.3f * Tired);
				NextDroop = FMath::FRandRange(9.0f, 20.0f) * (1.2f - 0.7f * Tired);
			}
		}
	}
	if (DroopT >= 0.0f)
	{
		DroopT += RealDt;
		const float U = DroopT / DroopLength;
		Eyelids = DroopDepth * (U < 0.7f ? FMath::SmoothStep(0.0f, 0.7f, U) : 1.0f - FMath::SmoothStep(0.7f, 1.0f, U));
		if (U >= 1.0f)
		{
			DroopT = -1.0f;
			Eyelids = 0.0f;
		}
	}
	// A break: the room stretches its legs while the screens count down, then the floor calls everyone back (the rail
	// waits it out too: no hands are dealt in the room).
	if (bOnBreak && (Phase == EBackRoomPhase::Playing || bRailing))
	{
		BreakT += RealDt;
		if (BreakT >= BreakRealSeconds)
		{
			// (The floor's "break is over" is the director's, when its clock sees the break end.)
			bOnBreak = false;
			RaiseBanner(FString::Printf(TEXT("LEVEL %d   %s / %s"), BreakLevel, *Num(Tourney->CurrentLevel().Sb), *Num(Tourney->CurrentLevel().Bb)));
			if (Phase == EBackRoomPhase::Playing)
			{
				Table->Resume(1.5f);
			}
		}
	}
	// Hand for hand: the table waits for the rest of the room, then the floor calls the next one.
	if (!bOnBreak && MoveT < 0.0f && Phase == EBackRoomPhase::Playing && Table->IsHolding())
	{
		MoveT += RealDt;
		if (MoveT >= 0.0f)
		{
			MoveT = -1000.0f;
			// (The floor sends the room on, "All tables complete", after any hand with an all-in and every third hand
			// otherwise: the director's deterministic gate, not a dice roll.)
			Table->Resume(0.5f);
		}
	}
	if (bLeaveWhenFree && Phase == EBackRoomPhase::Playing && Table->IsBetweenHands() && !Tourney->FinishPending())
	{
		LiveRequestLeave();
	}
	else if (QuitAskedAt >= 0.0f && LiveT - QuitAskedAt > 4.0f && !bLeaveWhenFree)
	{
		QuitAskedAt = -1.0f;
	}

	// Out (or champion): Dee's goodbye, the summary, the cage if you cashed, and the doors; or a while longer on the rail.
	if (Phase == EBackRoomPhase::Leaving)
	{
		LeaveT += RealDt;
		Eyelids = 0.0f;
		if (bRailing)
		{
			LiveRail(RealDt);
		}
		// The next event tonight: the floor calls its seats when they open, and a moment later the room opens again for it.
		if (!NextEntryId.IsEmpty() && !bExitNow && !bOpeningNext && !bWaitingOut)
		{
			if (!bNextCalled && Minutes >= NextSeatingAt)
			{
				LiveCallNext();
			}
			else if (bNextCalled && (NextCallT += RealDt) > 7.0f)
			{
				LiveOpenNext();
			}
		}
		if (bOpeningNext)
		{
			return;
		}
		if (bExitNow)
		{
			// Out through the entrance on foot: the night ends at the door.
			if (!bFadingOut && PC && PC->PlayerCameraManager)
			{
				bFadingOut = true;
				PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, 1.2f, FLinearColor::Black, true, true);
			}
			ExitT += RealDt;
			if (ExitT > 1.4f && !bGoingHome)
			{
				NoteRailedPlaces();
				LiveGoHome();
			}
			return;
		}
		// The summary waits for the player (Q stays, L goes); a while, then home anyway.
		if (!bRailing && !bWalkingOut && LeaveT > 30.0f)
		{
			bWalkingOut = true;
			if (!Pawn || !Stage)
			{
				LiveGoHome();
				return;
			}
			const FVector Eye = Pawn->GetEye();
			const FTransform Room = Stage->GetRoomRoot() ? Stage->GetRoomRoot()->GetComponentTransform() : FTransform::Identity;
			if (HeroPrizeCents > 0)
			{
				// To the cage first.
				const TArray<FVector> ToCage = Stage->CardRoomToCage(Eye);
				Pawn->PlayWalk(ToCage, 7.0f, true, [Self, Room]() {
					ABackRoomGameMode* GM = Self.Get();
					if (!GM)
					{
						return;
					}
					// The cashier, at the cage's window, through the room (captioned in the core HUD; the old announcement
					// when there's no room host).
					ss::room::Cue Cash;
					Cash.What = ss::room::Kind::Line;
					Cash.Where = ss::room::From::Desk;
					Cash.Speaker = "Cashier";
					Cash.Key = "host.cage";
					Cash.Text = TCHAR_TO_UTF8(*FString::Printf(TEXT("Congratulations%s. %s, cash. Count it with me... there you go."), *AddressTo(GM->Save), *Dollars(GM->HeroPrizeCents)));
					Cash.ToHero = true;
					Cash.Priority = 2;
					Cash.Tick = GM->Tourney ? GM->Tourney->Tick : 0;
					GM->PlayCue(Cash);
					if (GM->Table && GM->Table->GetAudio())
					{
						GM->Table->GetAudio()->Play(ss::SoundId::Cash, 0.8f);
					}
					FTimerHandle Handle;
					GM->GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(GM, [GM, Room]() {
						ABackRoomPawn* P = Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(GM, 0));
						if (!P || !GM->Stage)
						{
							GM->LiveGoHome();
							return;
						}
						const TArray<FVector> Out = GM->Stage->CardRoomCageToDoor(P->GetEye());
						TWeakObjectPtr<ABackRoomGameMode> Self3 = GM;
						P->PlayWalk(Out, 9.0f, true, [Self3]() {
							if (ABackRoomGameMode* G = Self3.Get())
							{
								G->LiveGoHome();
							}
						});
						GM->LeaveT = 100.0f; // fade from here
						GM->bFadingOut = false;
						GM->MoveT = 6.2f;
					}), 3.0f, false);
				});
			}
			else
			{
				Pawn->PlayWalk(Stage->CardRoomWalkOut(Eye), 9.0f, true, [Self]() {
					if (ABackRoomGameMode* GM = Self.Get())
					{
						GM->LiveGoHome();
					}
				});
				MoveT = 6.2f;
				LeaveT = 100.0f;
			}
		}
		// Fade out at the doors.
		if (LeaveT >= 100.0f && !bFadingOut && PC && PC->PlayerCameraManager && LeaveT - 100.0f > MoveT)
		{
			bFadingOut = true;
			PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, 1.5f, FLinearColor::Black, true, true);
		}
	}
}
