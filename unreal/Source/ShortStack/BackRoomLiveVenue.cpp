// The Embercrest's card room as a venue (ABackRoomGameMode's live tournaments; the flow is BackRoomLive.cpp): the desk
// writing the player in (the world's field on a worker, then the entry), what's in reach on foot and what it says, the
// boards by the entrance and over the stage, and the next event tonight: offered, registered, called, opened.

#include "BackRoomLiveShared.h"

#include "BackRoomGameMode.h"
#include "BackRoomStage.h"
#include "BackRoomTable.h"
#include "CareerSave.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NightOneAudio.h"
#include "Tasks/Task.h"
#include "TimerManager.h"
#include "ShortStack/Audio/Synth.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Tournament.h"

#include <algorithm>

namespace LiveDetail
{
/** The next event's seats open this long before its cards (world minutes): the floor calls them then. */
constexpr double SeatingLead = 10.0;
/** The desk stops writing players in this close to the end of late registration (there's the walk to the seat). */
constexpr double DeskClosesBefore = 5.0;
} // namespace LiveDetail

using namespace LiveDetail;

// ------------------------------------------------------------------ the desk's signup

void FLiveDeskSignup::Run()
{
	ss::world::World W;
	std::vector<std::string> Fields;
	for (size_t From = 0; From < WorldText.size();)
	{
		const size_t End = FMath::Min(WorldText.find('\n', From), WorldText.size());
		Fields.clear();
		for (size_t Cell = From, K = From; K <= End; ++K)
		{
			if (K == End || WorldText[K] == '\t')
			{
				Fields.emplace_back(WorldText, Cell, K - Cell);
				Cell = K + 1;
			}
		}
		W.Read(Fields);
		From = End + 1;
	}
	WorldText.clear();
	WorldText.shrink_to_fit();
	W.Finish();
	if (W.Ready() && O.Valid())
	{
		bWorld = true;
		W.HeroName = HeroName;
		// The field as the session's own registration describes it (Session::GoToLive calls the same; core's RoomField
		// test proves the desk's old copy gave the same notes): the people the world registered and how they play (the
		// room's faces as the Back Room knows them play their own way), the faces among them and what they'll say, the
		// champions' board, and their voices, badges, patches and the month's room leaders.
		Notes = ss::live::DescribeField(W, O, HeroName, Hero, At);
		// The world waits for the player's result in it (it never decides an event the player is in).
		W.HeroEntered(O.Id, O.T->Name);
		W.Write(WorldAfter);
	}
	bDone = true;
}

namespace LiveDetail
{
std::string RegisterFromSignup(ss::SaveData& D, const FLiveDeskSignup& S, double World, bool bInRoom, const ss::life::Funding& Fund)
{
	const ss::live::Occurrence& O = S.O;
	const int Entrants = S.bWorld ? FMath::Max(S.Notes.Planned, static_cast<int>(S.Notes.Roster.size())) + 1 : O.Field + 1;
	const ss::Chips Waived = bInRoom ? 2 * ss::live::BusFareCents : 0;
	ss::Chips Bank = D.BankrollCents + Waived;
	const std::string Why = ss::live::Register(Bank, D.Life, O, World, Entrants, S.Notes.Roster, Fund);
	D.BankrollCents = Bank - Waived;
	ss::life::LiveEntry* E = Why.empty() ? ss::live::EntryFor(D.Life, O.Id) : nullptr;
	if (!E)
	{
		return Why.empty() ? std::string("There's no such event.") : Why;
	}
	// As Session::GoToLive: the field's notes on the entry, then the patches the player wears in tonight.
	ss::live::Apply(S.Notes, *E);
	ss::sponsor::NoteWorn(*E, D.HeroName, D.Life.Closet, D.Life.Sponsors, World);
	if (S.bWorld)
	{
		D.WorldText = S.WorldAfter;
	}
	return std::string();
}
} // namespace LiveDetail

// ------------------------------------------------------------------ on foot: what's in reach

ECardRoomSpot ABackRoomGameMode::CurrentSpot() const
{
	const ABackRoomPawn* Pawn = Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	return Pawn && Stage && Pawn->IsFreeWalking() ? Stage->CardRoomSpot(Pawn->GetEye()) : ECardRoomSpot::None;
}

FString ABackRoomGameMode::SpotPrompt() const
{
	switch (CurrentSpot())
	{
	case ECardRoomSpot::Seat: return bLiveOver ? FString() : FString(TEXT("[E] Sit back down"));
	case ECardRoomSpot::Desk:
	{
		// Out of tonight's event: the desk can write the player into the next one.
		FString WhyNot;
		const ss::live::Occurrence Open = bLiveOver && NextEntryId.IsEmpty() && !Signup.IsValid() ? ss::live::FindOccurrence(std::string(TCHAR_TO_UTF8(*NextOpenEvent(WhyNot))))
																									 : ss::live::Occurrence();
		if (Open.Valid())
		{
			return DeskOffer == Str(Open.Id) && LiveT - DeskAskedAt < 8.0f
					   ? FString::Printf(TEXT("[E] again: register for the %s (%s)"), *Str(Open.T->Short), *Dollars(Open.T->BuyInCents))
					   : FString::Printf(TEXT("[E] Tournament desk: the %s is open (%s)"), *Str(Open.T->Short), *Dollars(Open.T->BuyInCents));
		}
		return TEXT("[E] Tournament desk");
	}
	case ECardRoomSpot::Cage: return bLiveOver && HeroPrizeCents > 0 && !bCollected ? FString(TEXT("[E] Collect your winnings")) : FString(TEXT("[E] Cashier"));
	case ECardRoomSpot::Bar: return TEXT("[E] The bar");
	case ECardRoomSpot::Terrace:
		return !NextEntryId.IsEmpty() && !bNextCalled && !bWaitingOut ? FString::Printf(TEXT("[E] Step out on the terrace until the %s is called"), *NextName)
																	  : FString(TEXT("[E] Step out on the terrace"));
	case ECardRoomSpot::Exit:
		if (bLiveOver && !NextEntryId.IsEmpty())
		{
			return LiveT - ExitAskedAt < 5.0f ? FString::Printf(TEXT("[E] again: off the list for the %s, refunded, and home"), *NextName)
											  : FString::Printf(TEXT("[E] Go home (the desk refunds the %s)"), *NextName);
		}
		return bLiveOver ? FString(TEXT("[E] Go home")) : (LiveT - ExitAskedAt < 5.0f ? FString(TEXT("[E] again: leave the tournament (your stack is blinded off)")) : FString(TEXT("[E] Leave the tournament")));
	default: return FString();
	}
}

void ABackRoomGameMode::LiveInteract()
{
	ABackRoomPawn* Pawn = Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!bLive || !Stage || !Pawn || !Pawn->IsFreeWalking() || !Tourney)
	{
		return;
	}
	const ss::Level& L = Tourney->CurrentLevel();
	switch (Stage->CardRoomSpot(Pawn->GetEye()))
	{
	case ECardRoomSpot::Seat:
		LiveSitDown();
		break;
	case ECardRoomSpot::Desk:
	{
		// The floor at the desk: where the night is, and a regular gets a regular's hello. Out of tonight's event, the next
		// one today: offered on the first press, written in on the second.
		FString Line;
		FString WhyNot;
		const FString Open = bLiveOver && NextEntryId.IsEmpty() && !Signup.IsValid() ? NextOpenEvent(WhyNot) : FString();
		const ss::live::Occurrence Offer = ss::live::FindOccurrence(std::string(TCHAR_TO_UTF8(*Open)));
		if (Signup.IsValid())
		{
			Line = TEXT("One second, I'm writing you in.");
		}
		else if (!NextEntryId.IsEmpty())
		{
			Line = bNextCalled ? FString::Printf(TEXT("They're seating the %s now. Go on, find your table."), *NextName)
							   : FString::Printf(TEXT("You're all set for the %s. Seats open at %s; the floor will call it. The bar's open, the terrace too."), *NextName,
									 *Str(ss::net::TimeLabel(NextSeatingAt)));
		}
		else if (Offer.Valid() && DeskOffer == Open && LiveT - DeskAskedAt < 8.0f)
		{
			DeskOffer.Empty();
			DeskSignup(Open);
			Line = TEXT("Let me write you in.");
		}
		else if (Offer.Valid())
		{
			DeskOffer = Open;
			DeskAskedAt = LiveT;
			const ss::live::EventTemplate& T = *Offer.T;
			const FString When = Minutes < Offer.Start ? TEXT("cards at ") + Str(ss::net::TimeLabel(Offer.Start)) : TEXT("late registration till ") + Str(ss::net::TimeLabel(Offer.LateRegEnds));
			Line = FString::Printf(TEXT("The %s is still open%s. %s: %s to the prize pool, %s to the house. %s in chips, %s. Want in?"), *Str(T.Short), *AddressTo(Save),
				*Dollars(T.BuyInCents), *Dollars(T.BuyInCents - T.FeeCents), *Dollars(T.FeeCents), *Num(T.StartingStack), *When);
		}
		else if (!WhyNot.IsEmpty())
		{
			Line = WhyNot;
		}
		else if (bLiveOver)
		{
			FString Next;
			for (int32 Day = LiveDay; Day <= LiveDay + 1 && Next.IsEmpty(); ++Day)
			{
				for (const ss::live::Occurrence& O : ss::live::Occurrences(Day))
				{
					if (O.Start > Minutes && Next.IsEmpty())
					{
						Next = FString::Printf(TEXT("the %s, %s %s"), *Str(O.T->Name), Day == LiveDay ? TEXT("tonight at") : TEXT("tomorrow at"), *Str(ss::net::TimeLabel(O.Start)));
					}
				}
			}
			Line = Next.IsEmpty() ? FString(TEXT("Thanks for coming in. Get home safe.")) : FString::Printf(TEXT("Thanks for playing. Next one's %s."), *Next);
		}
		else
		{
			const int32 Every = FMath::Max(1, Tourney->Spec.BreakEvery);
			const int32 NextBreak = (Tourney->LevelIndex / Every + 1) * Every + 1;
			const FString Status = FString::Printf(TEXT("%d left, %d paid. Level %d, %s and %s; the next break's before level %d."), Tourney->Remaining, Tourney->PaidPlaces(),
				Tourney->LevelIndex + 1, *Num(L.Sb), *Num(L.Bb), NextBreak);
			Line = PastLiveEvents == 0 ? TEXT("First time with us? Welcome to the Embercrest. ") + Status
				   : PastLiveEvents >= 4 ? FString::Printf(TEXT("There's my regular%s. "), *AddressTo(Save)) + Status
										 : Status;
		}
		Table->Announce(TEXT("Desk"), Line);
		break;
	}
	case ECardRoomSpot::Cage:
		if (bLiveOver && HeroPrizeCents > 0 && !bCollected)
		{
			bCollected = true;
			Table->Announce(TEXT("Cashier"), FString::Printf(TEXT("Congratulations%s. %s, cash. Count it with me... there you go."), *AddressTo(Save), *Dollars(HeroPrizeCents)));
			if (Table->GetAudio())
			{
				Table->GetAudio()->Play(ss::SoundId::Cash, 0.8f);
			}
		}
		else if (bCollected)
		{
			Table->Announce(TEXT("Cashier"), TEXT("You're all paid up, hon. Don't spend it all at once."));
		}
		else if (PastLiveCashes > 0)
		{
			Table->Announce(TEXT("Cashier"), FString::Printf(TEXT("Back again%s. Your best here's %s place. Bring me something bigger tonight."), *AddressTo(Save),
				*Ordinal(FMath::Max(1, PastBestPlace))));
		}
		else
		{
			Table->Announce(TEXT("Cashier"), PastLiveEvents == 0 ? FString(TEXT("Nothing to cash yet. First night? Good luck in there."))
																 : FString::Printf(TEXT("Nothing for you yet%s. Come back with good news."), *AddressTo(Save)));
		}
		break;
	case ECardRoomSpot::Bar:
		if (LiveT - LastWater > 300.0f)
		{
			LastWater = LiveT;
			Energy = FMath::Min(100.0f, Energy + 3.0f);
			Table->Announce(TEXT("Bartender"), TEXT("Club soda, lime. On the house for players."));
		}
		else
		{
			Table->Announce(TEXT("Bartender"), TEXT("Another one? Pace yourself, the night's long."));
		}
		break;
	case ECardRoomSpot::Terrace:
		if (!NextEntryId.IsEmpty() && !bNextCalled && !bWaitingOut)
		{
			// Waiting for the next event out in the cold: the wait passes (the room plays on without you) until the floor
			// calls it.
			bWaitingOut = true;
			APlayerController* PC = GetWorld()->GetFirstPlayerController();
			if (PC && PC->PlayerCameraManager)
			{
				PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, 0.9f, FLinearColor::Black, true, true);
			}
			FTimerHandle Handle;
			GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]() {
				const double Until = NextSeatingAt;
				RailCatchUp(Until);
				LastAir = LiveT;
				Energy = FMath::Min(100.0f, Energy + 6.0f);
				bWaitingOut = false;
				if (APlayerController* P = GetWorld()->GetFirstPlayerController(); P && P->PlayerCameraManager)
				{
					P->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 1.4f, FLinearColor::Black, true, false);
				}
				Table->Announce(FString(), FString::Printf(TEXT("Out on the terrace until %s: the ridge, the cold, the resort's lights on the slopes. Inside, the floor's on the mic."),
											   *Str(ss::net::TimeLabel(Until))));
			}), 1.0f, false);
		}
		else if (LiveT - LastAir > 420.0f)
		{
			LastAir = LiveT;
			Energy = FMath::Min(100.0f, Energy + 6.0f);
			Table->Announce(FString(), TEXT("Cold mountain air, the ridge white under the stars. You wake up a little."));
		}
		else
		{
			Table->Announce(FString(), TEXT("The ranges, dark and huge. The room's warm behind you."));
		}
		break;
	case ECardRoomSpot::Exit:
		if (bLiveOver && !NextEntryId.IsEmpty())
		{
			// Registered for the next event: leaving takes the player off its list (refunded: they never sat), then home.
			if (bOpeningNext)
			{
				break;
			}
			if (LiveT - ExitAskedAt < 5.0f)
			{
				LiveUnregister();
				bExitNow = true;
			}
			else
			{
				ExitAskedAt = LiveT;
				Table->Announce(TEXT("Floor"), FString::Printf(TEXT("You're on the list for the %s. Head out now and the desk takes you off it, money back. Press again to go."), *NextName));
			}
		}
		else if (bLiveOver)
		{
			bExitNow = true;
		}
		else if (LiveT - ExitAskedAt < 5.0f)
		{
			// Walking out on a live tournament: the seat's dealt in until the blinds take it (the published rule).
			bExitNow = true;
			QuitAskedAt = LiveT;
			LiveRequestLeave();
		}
		else
		{
			ExitAskedAt = LiveT;
			Table->Announce(TEXT("Floor"), TEXT("Heading out? Your seat stays in and gets blinded off. Press again to go."));
		}
		break;
	default:
		break;
	}
}

// ------------------------------------------------------------------ the boards

void ABackRoomGameMode::UpdateRoomBoards()
{
	if (!Stage || !Tourney || !Save.IsValid())
	{
		return;
	}
	// Today's events by the entrance, each with where it stands now.
	TArray<FString> Lines;
	for (const ss::live::Occurrence& O : ss::live::Occurrences(LiveDay))
	{
		FString State;
		if (Str(O.Id) == LiveEntryId && bLiveOver)
		{
			State = TEXT("YOUR NIGHT IS OVER");
		}
		else if (Str(O.Id) == LiveEntryId)
		{
			State = bLastCalled && !bRegClosed ? TEXT("LAST CALL  \u00b7  REG TO ") + Str(ss::net::TimeLabel(LiveLateRegEnds))
											   : FString::Printf(TEXT("PLAYING  \u00b7  %d OF %d LEFT"), Tourney->Remaining, Tourney->Spec.Entrants);
		}
		else if (Str(O.Id) == NextEntryId)
		{
			// The event the desk wrote the player into tonight.
			State = bNextCalled ? FString(TEXT("YOU'RE IN  \u00b7  SEATING NOW")) : TEXT("YOU'RE IN  \u00b7  SEATS ") + Str(ss::net::TimeLabel(NextSeatingAt));
		}
		else if (Minutes < O.DeskOpens)
		{
			State = TEXT("REGISTRATION ") + Str(ss::net::TimeLabel(O.DeskOpens));
		}
		else if (Minutes < O.Start)
		{
			State = TEXT("REGISTERING NOW");
		}
		else if (Minutes < O.LateRegEnds)
		{
			State = (ss::live::LastCall(O, Minutes) ? TEXT("LAST CALL  \u00b7  REG TO ") : TEXT("LATE REG TO ")) + Str(ss::net::TimeLabel(O.LateRegEnds));
		}
		else if (Minutes < O.Start + O.T->Hours * 60.0)
		{
			State = TEXT("RUNNING");
		}
		else
		{
			State = TEXT("FINISHED");
		}
		Lines.Add(FString::Printf(TEXT("%-9s %-24s %s"), *Str(ss::net::TimeLabel(O.Start)), *Str(O.T->Name).ToUpper(), *State));
	}
	Stage->SetSchedule(Lines);
}

void ABackRoomGameMode::UpdateBoard(float RealDt)
{
	BoardTick -= RealDt;
	if (BoardTick > 0.0f || !Stage || !Tourney)
	{
		return;
	}
	BoardTick = 0.25f;
	RoomBoardTick -= 0.25f;
	if (RoomBoardTick <= 0.0f)
	{
		RoomBoardTick = 3.0f;
		UpdateRoomBoards();
	}
	const ss::Level& L = Tourney->CurrentLevel();
	const ss::Level& N = Tourney->NextLevel();
	const int32 Secs = FMath::FloorToInt(LevelTimeLeft());
	const bool bBeforeCards = Phase == EBackRoomPhase::Arriving && !bAnnouncedStart && Tourney->Tick == 0;
	// In the last minutes of late registration the title takes turns with the last call.
	const bool bLastCallUp = bLastCalled && !bRegClosed && !bCancelled && FMath::FloorToInt(LiveT / 3.0f) % 2 == 1;
	const FString Title = bFinalTable ? FString(TEXT("FINAL TABLE"))
					  : bOnBreak	  ? FString(TEXT("BREAK"))
					  : bLastCallUp	  ? TEXT("LAST CALL  ·  REG TO ") + Str(ss::net::TimeLabel(LiveLateRegEnds))
									  : LiveName.ToUpper();
	const int32 BreakSecs = FMath::Max(0, FMath::FloorToInt(LiveBreakMinutes * 60.0 * (1.0 - static_cast<double>(BreakT) / BreakRealSeconds)));
	const FString Clock = bBeforeCards ? TEXT("SOON") : bOnBreak ? FString::Printf(TEXT("%02d:%02d"), BreakSecs / 60, BreakSecs % 60) : FString::Printf(TEXT("%02d:%02d"), Secs / 60, Secs % 60);
	const FString Blinds = L.Ante > 0 ? FString::Printf(TEXT("%s / %s   ANTE %s"), *Num(L.Sb), *Num(L.Bb), *Num(L.Ante)) : FString::Printf(TEXT("%s / %s"), *Num(L.Sb), *Num(L.Bb));
	const FString Next = FString::Printf(TEXT("NEXT   %s / %s"), *Num(N.Sb), *Num(N.Bb));
	// The champion as the room knows them: the person's name (the creator's), or the screen name for a career from before it.
	const FString Champion = !Save.IsValid() ? FString() : Save->Person.Created ? Str(Save->Person.FullName()).ToUpper() : Str(Save->HeroName);
	const FString Field = bHeroWon ? FString::Printf(TEXT("CHAMPION   %s"), *Champion)
								   : FString::Printf(TEXT("PLAYERS %d / %d     AVG %s     PAID %d"), Tourney->Remaining, Tourney->Spec.Entrants, *Num(static_cast<int64>(Tourney->AverageStack())), Tourney->PaidPlaces());
	Stage->SetBoard(Title, FString::Printf(TEXT("LEVEL %d"), Tourney->LevelIndex + 1), Clock, Blinds, Next, Field);
}

// ------------------------------------------------------------------ the next event

FString ABackRoomGameMode::NextOpenEvent(FString& OutWhyNot) const
{
	OutWhyNot.Empty();
	if (!Save.IsValid() || !bLiveOver || bCancelled || ss::live::ActiveEntry(Save->Life))
	{
		return FString();
	}
	// Today's events the player hasn't played, while the walk to the seat still makes late registration. What the desk
	// sells is the entry (the night's bus home is paid), and a long night needs the legs for it, as at home.
	const ss::life::Activity* Trip = ss::life::Find("embercrest");
	for (const ss::live::Occurrence& O : ss::live::Occurrences(LiveDay))
	{
		if (Str(O.Id) == LiveEntryId || ss::live::EntryFor(Save->Life, O.Id) || Minutes >= O.LateRegEnds)
		{
			continue;
		}
		const FString Short = Str(O.T->Short);
		if (Minutes + DeskClosesBefore > O.LateRegEnds)
		{
			OutWhyNot = OutWhyNot.IsEmpty() ? FString::Printf(TEXT("Registration for the %s is closing. There isn't time to get you to a seat, sorry."), *Short) : OutWhyNot;
			continue;
		}
		if (Minutes < O.DeskOpens)
		{
			OutWhyNot = OutWhyNot.IsEmpty() ? FString::Printf(TEXT("Registration for the %s opens at %s. Come see me then."), *Short, *Str(ss::net::TimeLabel(O.DeskOpens))) : OutWhyNot;
			continue;
		}
		if (Save->BankrollCents < O.T->BuyInCents)
		{
			OutWhyNot = OutWhyNot.IsEmpty() ? FString::Printf(TEXT("The %s is %s. Come back when you've got it."), *Short, *Dollars(O.T->BuyInCents)) : OutWhyNot;
			continue;
		}
		if (Trip && Energy < Trip->Energy)
		{
			OutWhyNot = OutWhyNot.IsEmpty() ? FString(TEXT("A tournament's a long night, hon, and you look done in. Go home and get some sleep.")) : OutWhyNot;
			continue;
		}
		return Str(O.Id);
	}
	return FString();
}

void ABackRoomGameMode::DeskSignup(const FString& OccurrenceId)
{
	const ss::live::Occurrence O = ss::live::FindOccurrence(std::string(TCHAR_TO_UTF8(*OccurrenceId)));
	if (!O.Valid() || !Save.IsValid() || Signup.IsValid() || !NextEntryId.IsEmpty())
	{
		return;
	}
	// The world's field for it, read on a worker (the save's world is megabytes of text); the entry when it's done.
	TSharedPtr<FLiveDeskSignup> S = MakeShared<FLiveDeskSignup>();
	S->WorldText = Save->WorldText;
	S->HeroName = Save->HeroName;
	S->Hero = Save->Life;
	S->O = O;
	S->At = Minutes;
	Signup = S;
	UE::Tasks::Launch(UE_SOURCE_LOCATION, [S]() { S->Run(); });
	UE_LOG(LogEmbercrest, Log, TEXT("The desk writes the player in for %s (%s)"), *OccurrenceId, *ClockLabel());
}

void ABackRoomGameMode::FinishSignup()
{
	const TSharedPtr<FLiveDeskSignup> S = Signup;
	Signup.Reset();
	if (!S.IsValid() || !Save.IsValid() || bGoingHome || bExitNow || bOpeningNext)
	{
		return; // on the way out: the desk never wrote it down
	}
	const ss::live::Occurrence& O = S->O;
	ss::SaveData D = *Save;
	const std::string WorldWas = D.WorldText;
	// At the desk, here already (the core's check counts the trip there: it's made).
	const double Booked = Minutes - ss::live::TravelMinutes;
	const std::string Why = RegisterFromSignup(D, *S, Booked, true);
	ss::life::LiveEntry* E = Why.empty() ? ss::live::EntryFor(D.Life, O.Id) : nullptr;
	if (!E)
	{
		Table->Announce(TEXT("Desk"), Str(Why));
		UE_LOG(LogEmbercrest, Warning, TEXT("The desk can't register the player for %s: %s"), *Str(O.Id), *Str(Why));
		return;
	}
	// Registered and arrived both now: no bus there, and none home of its own (the night's ticket covers it). The ledger's
	// lines carry the desk's own time.
	E->RegisteredAt = Minutes;
	E->ArrivedAt = Minutes;
	E->FareThere = true;
	// The night's own departure, kept on the entry until its first checkpoint carries it (a game closed while waiting goes
	// back to this room, and Night One still catches its calendar up from the night's first bus).
	E->CheckpointHost = std::string(TCHAR_TO_UTF8(*FString::Printf(TEXT("from\t%.2f\n"), NightLeftHomeAt)));
	for (size_t K = 0; K < D.Life.Ledger.size() && K < 2; ++K)
	{
		if (D.Life.Ledger[K].At == Booked)
		{
			D.Life.Ledger[K].At = Minutes;
		}
	}
	D.ClockMinutes = Minutes - ss::net::MinutesPerDay * ss::net::NightOneDay;
	D.Life.Energy = FMath::Clamp(static_cast<double>(Energy), 0.0, 100.0);
	if (S->bWorld)
	{
		WorldBeforeSignup = WorldWas;
	}
	*Save = D;
	if (LiveSaver)
	{
		LiveSaver->Flush();
	}
	CareerSave::SaveNow(D.Serialize());
	NextEntryId = Str(O.Id);
	NextName = Str(O.T->Short);
	NextSeatingAt = FMath::Max(Minutes, O.Start - SeatingLead);
	bNextCalled = false;
	NextCallT = 0.0f;
	Table->Announce(TEXT("Desk"), Minutes >= NextSeatingAt
									  ? FString::Printf(TEXT("You're in%s. %s. They're seating it now; the floor will call you over."), *AddressTo(Save), *Str(O.T->Name))
									  : FString::Printf(TEXT("You're in%s. %s. Seats open at %s; the floor will call it. The bar's open, the terrace too."), *AddressTo(Save),
											*Str(O.T->Name), *Str(ss::net::TimeLabel(NextSeatingAt))));
	if (Table->GetAudio())
	{
		Table->GetAudio()->Play(ss::SoundId::Cash, 0.5f);
	}
	UpdateRoomBoards();
	UE_LOG(LogEmbercrest, Log, TEXT("Registered at the desk for %s: %d entrants (%d the world follows, %d faces), bankroll %lld; seats at %s"), *NextEntryId, E->Entrants,
		static_cast<int32>(E->Roster.size()), static_cast<int32>(E->Faces.size()), static_cast<int64>(D.BankrollCents), *Str(ss::net::TimeLabel(NextSeatingAt)));
}

void ABackRoomGameMode::LiveUnregister()
{
	if (NextEntryId.IsEmpty() || !Save.IsValid())
	{
		return;
	}
	// Off the list before sitting down: the buy-in and fee back (their own ledger lines), and the world as it was before the
	// desk told it. The entry goes with them: a refunded entry reads as an event the room called off (the desk turns the
	// player away from it as cancelled, the phone's card says so), and this one wasn't. The player can buy in again from
	// home while its registration's open.
	ss::SaveData D = *Save;
	const std::string Id(TCHAR_TO_UTF8(*NextEntryId));
	if (ss::live::Refund(D.BankrollCents, D.Life, Id, Minutes, "off the list before sitting down"))
	{
		D.Life.LiveEntries.erase(std::remove_if(D.Life.LiveEntries.begin(), D.Life.LiveEntries.end(), [&Id](const ss::life::LiveEntry& E) { return E.Id == Id; }),
			D.Life.LiveEntries.end());
		if (!WorldBeforeSignup.empty())
		{
			D.WorldText = WorldBeforeSignup;
		}
	}
	*Save = D;
	if (LiveSaver)
	{
		LiveSaver->Flush();
	}
	CareerSave::SaveNow(D.Serialize());
	UE_LOG(LogEmbercrest, Log, TEXT("Off the list for %s, refunded"), *NextEntryId);
	WorldBeforeSignup.clear();
	NextEntryId.Empty();
	NextName.Empty();
	bNextCalled = false;
	UpdateRoomBoards();
}

void ABackRoomGameMode::LiveCallNext()
{
	bNextCalled = true;
	NextCallT = 0.0f;
	const ss::live::Occurrence O = ss::live::FindOccurrence(std::string(TCHAR_TO_UTF8(*NextEntryId)));
	if (!O.Valid())
	{
		return;
	}
	Floor(Minutes >= O.Start
			  ? FString::Printf(TEXT("Players registered for the %s, your seats are ready. Late registration closes at %s."), *NextName, *Str(ss::net::TimeLabel(O.LateRegEnds)))
			  : FString::Printf(TEXT("Players registered for the %s, your seats are open. Please find your table and seat; cards go in the air at %s."), *NextName,
					*Str(ss::net::TimeLabel(O.Start))));
	RaiseBanner(FString::Printf(TEXT("%s   SEATS OPEN"), *NextName.ToUpper()));
	UpdateRoomBoards();
	UE_LOG(LogEmbercrest, Log, TEXT("The floor calls %s (%s)"), *NextEntryId, *ClockLabel());
}

void ABackRoomGameMode::LiveOpenNext()
{
	if (bOpeningNext || bGoingHome || NextEntryId.IsEmpty() || !Save.IsValid())
	{
		return;
	}
	bOpeningNext = true;
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController(); PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, 1.2f, FLinearColor::Black, true, true);
	}
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]() {
		if (bGoingHome)
		{
			return; // quit from the pause menu meanwhile: that travel stands (Continue comes back to the entry)
		}
		// What the rail saw, the champion for the next event's board if the room crowned one, and the clock as it stands:
		// the room opens again for the next event with no bus and no trip, and the night's departure for Night One.
		ss::SaveData D = *Save;
		NoteRailed(D);
		ss::life::LiveEntry* Next = ss::live::EntryFor(D.Life, std::string(TCHAR_TO_UTF8(*NextEntryId)));
		const ss::TPlayer* Champion = nullptr;
		for (const ss::TPlayer& P : Tourney->Players)
		{
			Champion = Tourney->bFinished && P.Place == 1 ? &P : Champion;
		}
		if (Next && Champion)
		{
			static const TCHAR* Days[7] = {TEXT("MON"), TEXT("TUE"), TEXT("WED"), TEXT("THU"), TEXT("FRI"), TEXT("SAT"), TEXT("SUN")};
			const FString Who = Champion->IsHero ? Str(D.HeroName) : Str(Champion->Name);
			const FString Line = FString::Printf(TEXT("%s %s\t%s\t%lld"), Days[((LiveDay % 7) + 7) % 7], *LiveShort.ToUpper(), *Who, static_cast<int64>(Tourney->PrizeFor(1)));
			Next->Board.insert(Next->Board.begin(), std::string(TCHAR_TO_UTF8(*Line)));
			Next->Board.resize(FMath::Min<size_t>(Next->Board.size(), 8));
		}
		D.ClockMinutes = Minutes - ss::net::MinutesPerDay * ss::net::NightOneDay;
		D.Life.Energy = FMath::Clamp(static_cast<double>(Energy), 0.0, 100.0);
		*Save = D;
		if (LiveSaver)
		{
			LiveSaver->Flush();
		}
		CareerSave::SaveNow(D.Serialize());
		UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
		const FString Options = FString::Printf(TEXT("Live=%s?Here=1?At=%.2f?From=%.2f"), *NextEntryId, Minutes, NightLeftHomeAt);
		UE_LOG(LogEmbercrest, Log, TEXT("On to the next event: %s"), *Options);
		UGameplayStatics::OpenLevel(this, FName(TEXT("BackRoom")), true, Options);
	}), 1.3f, false);
}

FString ABackRoomGameMode::GetNextEventNote() const
{
	if (NextEntryId.IsEmpty())
	{
		return FString();
	}
	return bNextCalled ? FString::Printf(TEXT("Registered: %s, seating now"), *NextName)
					   : FString::Printf(TEXT("Registered: %s, seats at %s"), *NextName, *Str(ss::net::TimeLabel(NextSeatingAt)));
}

// ------------------------------------------------------------------ the desk screen and the room's clocks (ue-venue, next_segment_spec 6.5, 6.6)

void ABackRoomGameMode::OpenDesk(const FString& Window)
{
	DeskWindow = Window;
	bDeskUp = true;
	Desk.Open(LiveT, std::string(TCHAR_TO_UTF8(*Window)));
}

void ABackRoomGameMode::CloseDesk()
{
	bDeskUp = false;
}

FVector ABackRoomGameMode::SeatMarkerTarget() const
{
	return FVector::ZeroVector;
}

FString ABackRoomGameMode::TestDeskOpen(const FString& Window)
{
	OpenDesk(Window);
	return FString::Printf(TEXT("%s up"), *Window);
}

FString ABackRoomGameMode::TestDeskKey(const FString& Key)
{
	if (!bDeskUp)
	{
		return TEXT("The desk screen isn't up");
	}
	Desk.Key(std::string(TCHAR_TO_UTF8(*Key)), LiveT);
	return FString();
}

void ABackRoomGameMode::UpdateRoomClocks(float /*RealDt*/)
{
}

void ABackRoomGameMode::FillDeskInfo(ss::ui::DeskInfo& /*Out*/) const
{
}

void ABackRoomGameMode::TickDesk(float /*RealDt*/)
{
}
