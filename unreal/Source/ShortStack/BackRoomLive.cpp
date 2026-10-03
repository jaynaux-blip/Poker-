// ABackRoomGameMode's live tournament: the Riverside Sunday $150 (opened from Dee's Burner thread with
// "?Live=riverside").
//
// The field is an ss::Tournament (ss::live::MakeRiverside). The player's table is played hand by hand
// at the feature table (ABackRoomTable in tournament mode) and the rest of the room plays in the
// background: other tables run under their pendants with extras at them, and close as the field
// shrinks. The cast sits wherever the tournament seats them at the player's table (Tournament::
// FeatureIds keeps it stocked with them), busted players say goodbye and go, the floor runs the night
// over the PA (levels, busts near the money, hand for hand, the bubble, the final table), and a table
// break walks the player to a new seat while the room turns around them. Busting, or winning, settles
// the night into the save: the buy-in was paid at registration, a cash is collected at the cage on the
// way out.

#include "BackRoomChips.h"
#include "CareerSave.h"
#include "BackRoomGameMode.h"
#include "BackRoomPlayer.h"
#include "BackRoomStage.h"
#include "BackRoomTable.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NightOneAudio.h"
#include "NightOneSaveGame.h"
#include "TimerManager.h"
#include "ShortStack/Audio/Synth.h"
#include "ShortStack/Game/Chat.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Tournament.h"

DEFINE_LOG_CATEGORY_STATIC(LogRiverside, Log, All);

namespace LiveDetail
{
FString Ordinal(int32 N)
{
	const int32 Tens = N % 100;
	const TCHAR* Suffix = (Tens >= 11 && Tens <= 13) ? TEXT("th") : (N % 10 == 1 ? TEXT("st") : (N % 10 == 2 ? TEXT("nd") : (N % 10 == 3 ? TEXT("rd") : TEXT("th"))));
	return FString::Printf(TEXT("%d%s"), N, Suffix);
}

FString Num(int64 V)
{
	return FText::AsNumber(V).ToString();
}

FString Dollars(int64 Cents)
{
	return TEXT("$") + FText::AsNumber(Cents / 100).ToString();
}

FString Str(const std::string& S)
{
	return FString(UTF8_TO_TCHAR(S.c_str()));
}

/** The room's extras: low-detail MetaHumans for the other tables. */
const TCHAR* ExtraBodies[4] = {TEXT("ExtraA"), TEXT("ExtraB"), TEXT("ExtraC"), TEXT("ExtraD")};
/** At each other table: the dealer, and two players facing across. */
const int32 ExtraSeats[3] = {4, 2, 6};
const uint32 ExtraShirts[] = {0x2f3b52, 0x6e2b2b, 0x3a5a40, 0x8a7a5a, 0x5a4a6e, 0x9a9a9a, 0x7a4a2a, 0x24464f, 0x6a5a3a};

/** What a player says leaving the tournament. */
FString Goodbye(const FString& Name)
{
	if (Name == TEXT("Sal")) return TEXT("That's me. Good luck, kid.");
	if (Name == TEXT("Mrs. Park")) return TEXT("Thirty years. Still nothing. Goodnight, everybody.");
	if (Name == TEXT("Rick")) return TEXT("Unbelievable! I'm going to the bar.");
	if (Name == TEXT("Dre")) return TEXT("That's poker, baby. Good luck, y'all.");
	if (Name == TEXT("Big Lou")) return TEXT("Lou's out! Lou is OUT! Unbelievable.");
	if (Name == TEXT("Twitch")) return TEXT("Whatever, man. This deck hates me.");
	if (Name == TEXT("Mei")) return TEXT("Good game, everyone.");
	if (Name == TEXT("gh0stfold")) return TEXT("gg. see you online.");
	return TEXT("Good luck, everyone.");
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
	const FString Event = UGameplayStatics::ParseOption(OptionsString, TEXT("Live"));
	if (Event.IsEmpty())
	{
		return false;
	}
	std::string CareerText;
	TSharedPtr<ss::SaveData> D = MakeShared<ss::SaveData>();
	if (!CareerSave::LoadText(CareerText) || !ss::SaveData::Parse(CareerText, *D))
	{
		UE_LOG(LogRiverside, Warning, TEXT("Live=%s but no career save: practice table."), *Event);
		return false;
	}
	if (D->BankrollCents < ss::live::RiversideBuyInCents)
	{
		UE_LOG(LogRiverside, Warning, TEXT("Bankroll %lld cents can't cover the Riverside: practice table."), static_cast<int64>(D->BankrollCents));
		return false;
	}
	Save = D;
	LeftHomeAt = ss::net::MinutesPerDay * ss::net::NightOneDay + D->ClockMinutes;
	// Across town on the 14 bus: twenty minutes.
	Minutes = LeftHomeAt + 20.0;
	const FString DayOption = UGameplayStatics::ParseOption(OptionsString, TEXT("Day"));
	LiveDay = DayOption.IsEmpty() ? ss::net::DayOf(Minutes) : FCString::Atoi(*DayOption);
	LiveDayStart = ss::net::MinutesPerDay * LiveDay;
	Energy = static_cast<float>(D->Life.Energy);
	PastNights = D->Life.BackRoomNights;
	PastNetCents = D->Life.BackRoomNetCents;
	bFirstVisit = false;
	// Registered at the desk: the $150 is gone, the chips are on the table.
	StartBankrollCents = D->BankrollCents;
	BoughtInCents = ss::live::RiversideBuyInCents;
	BaseCents = StartBankrollCents - BoughtInCents;
	const FString Seed = FString::Printf(TEXT("riverside-%d-%lld"), LiveDay, FDateTime::Now().GetTicks());
	Tourney = MakeShareable(ss::live::MakeRiverside(LiveDay, D->HeroName, std::string(TCHAR_TO_UTF8(*Seed))).release());
	HeroBuyInChips = Tourney->Spec.StartingStack;
	ABackRoomChips::ChipUnit = 100;
	{
		ss::SaveData Registered = *Save;
		Registered.BankrollCents = BaseCents;
		Registered.ClockMinutes = Minutes - ss::net::MinutesPerDay * ss::net::NightOneDay;
		CareerSave::SaveNow(Registered.Serialize());
	}
	Phase = EBackRoomPhase::Arriving;
	ArrivalDayText = FString::Printf(TEXT("Sunday   %s"), *FString(UTF8_TO_TCHAR(ss::net::DateLabel(LiveDay).c_str())));
	UE_LOG(LogRiverside, Log, TEXT("Riverside: %d entrants, pool %lld cents, %d paid; bankroll %lld -> %lld; arriving %s"), Tourney->Spec.Entrants,
		static_cast<int64>(Tourney->PrizePoolCents), Tourney->PaidPlaces(), StartBankrollCents, BaseCents, *ClockLabel());
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
	// Late: the field has played the levels you missed, your seat waiting with a full stack.
	const double SitAt = Minutes + 2.0;
	if (SitAt > CardsAt)
	{
		const int32 Missed = FMath::FloorToInt((SitAt - CardsAt) * 60.0 / Tourney->Spec.SecondsPerHand);
		Tourney->HeroAway = true;
		for (int32 I = 0; I < Missed && !Tourney->bFinished; ++I)
		{
			Tourney->SimulateTick();
		}
		Tourney->HeroAway = false;
		UE_LOG(LogRiverside, Log, TEXT("Late registration: %d hands played without you, level %d, %d left"), Missed, Tourney->LevelIndex + 1, Tourney->Remaining);
	}
	ArrivalDayText = FString::Printf(TEXT("Sunday, %s   %s"), *FString(UTF8_TO_TCHAR(ss::net::DateLabel(LiveDay).c_str())), *ClockLabel().RightChop(5));
	// The first table, seated before you get there; the rest of the room around it.
	Table->PrepareNext();
	Table->TakeEvents();
	PlaceExtras();
}

// ------------------------------------------------------------------ people

ABackRoomPlayer* ABackRoomGameMode::SeatCast(const ss::TPlayer& P, int32 TableSeat)
{
	const FString Id = Str(P.Id);
	const FString Name = Str(P.Name);
	const FTransform At = ABackRoomStage::SeatTransform(TableSeat);
	ABackRoomPlayer* A = nullptr;
	if (TObjectPtr<ABackRoomPlayer>* Found = CastActors.Find(Id))
	{
		A = Found->Get();
	}
	if (!A)
	{
		FString Asset = CastAssetFor(Name);
		if (Asset.IsEmpty())
		{
			// A stranger at the feature table (the cast has thinned out): one of the room's faces.
			Asset = ExtraBodies[AnonymousLooks++ % 4];
		}
		A = SpawnPerson(Asset, At, EBackRoomRole::Player, PersonaFor(Name));
		CastActors.Add(Id, A);
	}
	A->SetActorTransform(At);
	// In plain sight, a new face takes the chair in a blink rather than appearing in it.
	BlinkSwaps.RemoveAll([A](const TPair<TWeakObjectPtr<ABackRoomPlayer>, bool>& S) { return S.Key.Get() == A; });
	if (Phase == EBackRoomPhase::Playing && MoveT < 0.0f && HeroCanSee(At.GetLocation() + FVector(0.0, 0.0, 110.0)))
	{
		A->SetActorHiddenInGame(true);
		BlinkSwap(A, true);
	}
	else
	{
		A->SetActorHiddenInGame(false);
	}
	// Where everyone is, for their eyes.
	A->HeroEyes = Stage ? Stage->EyeLocation() : FVector(-98.0, 0.0, 124.0);
	A->PotAt = FVector(10.0, 0.0, ABackRoomStage::FeltZ);
	A->DealerAt = ABackRoomStage::SeatTransform(4).TransformPosition(FVector(-14.0, 0.0, 112.0));
	A->OthersAt.Reset();
	for (int32 S : {1, 2, 3, 5, 6})
	{
		if (S != TableSeat)
		{
			A->OthersAt.Add(ABackRoomStage::SeatTransform(S).TransformPosition(FVector(-14.0, 0.0, 112.0)));
		}
	}
	if (Phase == EBackRoomPhase::Playing && bAnnouncedStart && Dealer && FMath::FRand() < 0.6f)
	{
		Table->DealerLine(FString::Printf(TEXT("Welcome. Seat %d."), ss::live::SeatLabel(P.Seat)));
	}
	// gh0stfold, across a real table for the first time.
	if (Name == UTF8_TO_TCHAR(ss::RivalName) && !bGhostMet)
	{
		bGhostMet = true;
		TWeakObjectPtr<ABackRoomPlayer> Ghost = A;
		const FString HeroName = Save.IsValid() ? Str(Save->HeroName) : FString(TEXT("grinder_3c"));
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [Ghost, HeroName]() {
			if (ABackRoomPlayer* G = Ghost.Get())
			{
				G->Say(FString::Printf(TEXT("%s? From RiverLine? Huh. I've got two thousand hands on you."), *HeroName));
			}
		}), Phase == EBackRoomPhase::Arriving ? 9.0f : 3.0f, false);
	}
	return A;
}

void ABackRoomGameMode::UnseatCast(ABackRoomPlayer* Player, const FString& Id, bool bBusted)
{
	if (!Player)
	{
		return;
	}
	// Up and gone (the body waits out of sight in case the floor brings them back); in front of the hero, in a blink.
	if (Phase == EBackRoomPhase::Playing && MoveT < 0.0f && HeroCanSee(Player->GetActorLocation() + FVector(0.0, 0.0, 110.0)))
	{
		BlinkSwap(Player, false);
	}
	else
	{
		Player->SetActorHiddenInGame(true);
		Player->SetActorLocation(FVector(0.0, 0.0, -5000.0));
	}
	if (!bBusted && Dealer && Phase == EBackRoomPhase::Playing && LiveT - LastFarewellAt > 20.0f)
	{
		LastFarewellAt = LiveT;
		Table->DealerLine(TEXT("Good luck at the new table."));
	}
}

bool ABackRoomGameMode::HeroCanSee(const FVector& At) const
{
	const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC || !PC->PlayerCameraManager)
	{
		return false;
	}
	const FVector To = (At - PC->PlayerCameraManager->GetCameraLocation()).GetSafeNormal();
	// Half the (horizontal, so wider) field of view, and a margin for a body's width.
	const float Half = FMath::DegreesToRadians(FMath::Min(PC->PlayerCameraManager->GetFOVAngle() * 0.5f + 14.0f, 89.0f));
	return FVector::DotProduct(PC->PlayerCameraManager->GetCameraRotation().Vector(), To) > FMath::Cos(Half);
}

void ABackRoomGameMode::BlinkSwap(ABackRoomPlayer* Player, bool bShow)
{
	BlinkSwaps.Add({Player, bShow});
	if (BlinkT >= 0.0f)
	{
		return; // this blink covers it
	}
	BlinkT = 0.0f;
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, 0.07f, FLinearColor::Black, false, true);
	}
}

void ABackRoomGameMode::ApplyBlinkSwaps()
{
	UE_LOG(LogRiverside, Log, TEXT("Seat change in a blink: %d bodies"), BlinkSwaps.Num());
	for (const TPair<TWeakObjectPtr<ABackRoomPlayer>, bool>& S : BlinkSwaps)
	{
		if (ABackRoomPlayer* A = S.Key.Get())
		{
			A->SetActorHiddenInGame(!S.Value);
			if (!S.Value)
			{
				A->SetActorLocation(FVector(0.0, 0.0, -5000.0));
			}
		}
	}
	BlinkSwaps.Reset();
}

int32 ABackRoomGameMode::SlotForTable(int32 TableId)
{
	if (const int32* Found = SlotByTable.Find(TableId))
	{
		return *Found;
	}
	TSet<int32> Used;
	for (const TPair<int32, int32>& It : SlotByTable)
	{
		Used.Add(It.Value);
	}
	for (int32 S = 0; S < ABackRoomStage::NumTableSlots(); ++S)
	{
		if (!Used.Contains(S))
		{
			SlotByTable.Add(TableId, S);
			return S;
		}
	}
	return -1;
}

void ABackRoomGameMode::PlaceExtras()
{
	if (!Stage || !Tourney)
	{
		return;
	}
	// The tables still running besides yours, each at a slot it keeps.
	const int32 HeroTable = Tourney->Hero().TableId;
	TSet<int32> Running;
	for (const auto& It : Tourney->Tables)
	{
		if (It.first != HeroTable)
		{
			Running.Add(It.first);
		}
	}
	for (auto It = SlotByTable.CreateIterator(); It; ++It)
	{
		if (!Running.Contains(It.Key()))
		{
			It.RemoveCurrent();
		}
	}
	TArray<int32> Ids = Running.Array();
	Ids.Sort();
	for (int32 Id : Ids)
	{
		SlotForTable(Id);
	}
	USceneComponent* RoomRoot = Stage->GetRoomRoot();
	const int32 Slots = ABackRoomStage::NumTableSlots();
	Extras.SetNum(Slots * 3);
	for (int32 Slot = 0; Slot < Slots; ++Slot)
	{
		const int32* TableId = SlotByTable.FindKey(Slot);
		const bool bOpen = TableId != nullptr;
		Stage->SetTableOpen(Slot, bOpen);
		int32 Seated = 0;
		if (bOpen)
		{
			for (int32 S : Tourney->Tables[*TableId].Seats)
			{
				Seated += S >= 0 ? 1 : 0;
			}
		}
		const FTransform TableFrame = ABackRoomStage::TableSlotLocal(Slot);
		for (int32 K = 0; K < 3; ++K)
		{
			const int32 Index = Slot * 3 + K;
			// The dealer, and as many as two of the players there.
			const bool bWanted = bOpen && (K == 0 || K <= Seated - 1);
			TObjectPtr<ABackRoomPlayer>& E = Extras[Index];
			if (!bWanted)
			{
				if (E)
				{
					E->SetActorHiddenInGame(true);
				}
				continue;
			}
			const FTransform Local = ABackRoomStage::SeatTransform(ExtraSeats[K]) * TableFrame;
			if (!E)
			{
				FBackRoomPersona Persona = PersonaFor(FString::Printf(TEXT("Extra%d"), Index));
				const uint32 Shirt = K == 0 ? 0x141418 : ExtraShirts[(Index * 5 + 3) % 9];
				Persona.Shirt = FLinearColor(FColor((Shirt >> 16) & 0xff, (Shirt >> 8) & 0xff, Shirt & 0xff));
				E = SpawnPerson(ExtraBodies[(Index * 3 + Slot) % 4], Local * (RoomRoot ? RoomRoot->GetComponentTransform() : FTransform::Identity), EBackRoomRole::Extra, Persona);
				if (RoomRoot)
				{
					E->AttachToComponent(RoomRoot, FAttachmentTransformRules::KeepWorldTransform);
				}
			}
			E->SetActorRelativeTransform(Local);
			E->SetActorHiddenInGame(false);
			// They look at their own table.
			const FTransform World = TableFrame * (RoomRoot ? RoomRoot->GetComponentTransform() : FTransform::Identity);
			E->TableToWorld = World;
			E->PotAt = World.TransformPosition(FVector(10.0, 0.0, ABackRoomStage::FeltZ));
			E->DealerAt = World.TransformPosition(ABackRoomStage::SeatTransform(4).TransformPosition(FVector(-14.0, 0.0, 112.0)));
			E->HeroEyes = World.TransformPosition(ABackRoomStage::SeatTransform(ExtraSeats[(K + 1) % 3]).TransformPosition(FVector(-14.0, 0.0, 112.0)));
			E->OthersAt.Reset();
			for (int32 J = 0; J < 3; ++J)
			{
				if (J != K)
				{
					E->OthersAt.Add(World.TransformPosition(ABackRoomStage::SeatTransform(ExtraSeats[J]).TransformPosition(FVector(-14.0, 0.0, 112.0))));
				}
			}
		}
	}
	Table->SetCrowd(static_cast<float>(Running.Num() + 1) / static_cast<float>(FMath::Max(1, (Tourney->Spec.Entrants + Tourney->TableSize - 1) / Tourney->TableSize)));
}

// ------------------------------------------------------------------ the night

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

void ABackRoomGameMode::RaiseBanner(const FString& Text)
{
	Banner = Text;
	BannerAge = 0.0f;
}

void ABackRoomGameMode::Floor(const FString& Line, bool bChime)
{
	if (bChime && Table && Table->GetAudio())
	{
		Table->GetAudio()->PlayEffect(ss::audio::Effect::Chime, 0.55f);
	}
	if (Table)
	{
		// After the chime.
		TWeakObjectPtr<ABackRoomTable> T = Table.Get();
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [T, Line]() {
			if (ABackRoomTable* Tb = T.Get())
			{
				Tb->Announce(TEXT("Floor"), Line);
			}
		}), bChime ? 1.1f : 0.01f, false);
	}
}

void ABackRoomGameMode::LiveEvent(const ss::TEvent& E)
{
	const int32 Paid = Tourney->PaidPlaces();
	const int32 HeroTable = Tourney->Hero().Busted ? -1 : Tourney->Hero().TableId;
	switch (E.Type)
	{
	case ss::TEventType::Bust:
	{
		const FString Name = Str(E.Name);
		if (E.IsHero)
		{
			HeroPlace = E.Place;
			HeroPrizeCents = E.PrizeCents;
			break;
		}
		if (ABackRoomPlayer* A = Table->PlayerById(Str(E.Id)))
		{
			A->Say(Goodbye(Name));
			if (E.PrizeCents > 0 && FMath::FRand() < 0.7f)
			{
				Table->DealerLine(FString::Printf(TEXT("%s place. See the cage for %s. Good game."), *Ordinal(E.Place), *Dollars(E.PrizeCents)));
			}
			break;
		}
		// Busts elsewhere: called out near the money and in it; now and then before.
		const int32 Idx = Tourney->PlayerIndex(E.Id);
		const int32 Seat = Idx >= 0 ? Tourney->Players[static_cast<size_t>(Idx)].Seat : 0;
		if (E.Place <= Paid + 4 || FMath::FRand() < 0.18f)
		{
			Floor(E.PrizeCents > 0 ? FString::Printf(TEXT("Table %d, seat %d is out in %s place, collecting %s."), E.TableId, ss::live::SeatLabel(Seat), *Ordinal(E.Place), *Dollars(E.PrizeCents))
								   : FString::Printf(TEXT("Table %d, seat %d is out in %s place."), E.TableId, ss::live::SeatLabel(Seat), *Ordinal(E.Place)),
				E.Place <= Paid + 4);
		}
		break;
	}
	case ss::TEventType::Level:
	{
		const ss::Level& L = E.Blinds;
		Floor(L.Ante > 0 ? FString::Printf(TEXT("Players, the blinds are now %s and %s, with a big blind ante of %s."), *Num(L.Sb), *Num(L.Bb), *Num(L.Ante))
						 : FString::Printf(TEXT("Players, the blinds are now %s and %s."), *Num(L.Sb), *Num(L.Bb)));
		RaiseBanner(FString::Printf(TEXT("LEVEL %d   %s / %s"), E.LevelNumber, *Num(L.Sb), *Num(L.Bb)));
		if (FMath::FRand() < 0.5f)
		{
			Table->DealerLine(TEXT("Blinds are up."));
		}
		break;
	}
	case ss::TEventType::TableBroken:
		if (E.TableId != HeroTable && (Tourney->Tables.size() <= 4 || FMath::FRand() < 0.5f))
		{
			Floor(FString::Printf(TEXT("Table %d is breaking. Players, take your chips and your seat cards from the floor."), E.TableId));
		}
		PlaceExtras();
		break;
	case ss::TEventType::HandForHand:
		bHandForHand = true;
		Floor(TEXT("Players, we are on the bubble. We're now hand for hand. Dealers, please hold your decks until every table has finished the hand."));
		RaiseBanner(TEXT("HAND FOR HAND"));
		Table->DealerLine(TEXT("Hand for hand, folks. Take your time."));
		break;
	case ss::TEventType::Bubble:
		bHandForHand = false;
		Floor(FString::Printf(TEXT("And that's the bubble! Congratulations, everyone remaining is in the money.")));
		RaiseBanner(TEXT("IN THE MONEY"));
		if (Table->GetAudio())
		{
			Table->GetAudio()->PlayEffect(ss::audio::Effect::Applause, 0.8f);
		}
		break;
	case ss::TEventType::FinalTable:
		bFinalTable = true;
		Floor(TEXT("Ladies and gentlemen, we are down to our final table! Players, please bring your chips to the feature table."));
		RaiseBanner(TEXT("FINAL TABLE"));
		Stage->SetOnAir(true);
		if (Table->GetAudio())
		{
			Table->GetAudio()->PlayEffect(ss::audio::Effect::Applause, 0.9f);
		}
		break;
	case ss::TEventType::Finished:
		if (E.Id == ss::HeroId)
		{
			bHeroWon = true;
			HeroPlace = 1;
			HeroPrizeCents = Tourney->PrizeFor(1);
			Floor(FString::Printf(TEXT("Ladies and gentlemen, your Riverside Sunday champion... %s!"), *Str(E.Name)));
			RaiseBanner(TEXT("CHAMPION"));
			if (Table->GetAudio())
			{
				Table->GetAudio()->PlayEffect(ss::audio::Effect::Applause, 1.0f);
			}
		}
		break;
	default:
		break;
	}
	if (E.Type == ss::TEventType::Moved && !E.IsHero)
	{
		PlaceExtras();
	}
}

void ABackRoomGameMode::LiveNote(uint8 Note)
{
	switch (static_cast<EBackRoomTableNote>(Note))
	{
	case EBackRoomTableNote::HandEnded:
		++HandsPlayed;
		if (bHandForHand && !Tourney->InTheMoney())
		{
			// The other tables are still playing their hand: everyone waits for the floor.
			Table->Hold();
			MoveT = -3.5f;
		}
		break;
	case EBackRoomTableNote::HeroMoved:
		if (Phase == EBackRoomPhase::Arriving)
		{
			Table->PrepareNext();
		}
		else
		{
			LiveMove();
		}
		break;
	case EBackRoomTableNote::TournamentOver:
		LiveOver();
		break;
	default:
		break;
	}
}

void ABackRoomGameMode::LiveMove()
{
	ABackRoomPawn* Pawn = Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	const ss::TPlayer& H = Tourney->Hero();
	Phase = EBackRoomPhase::Moving;
	MoveT = 0.0f;
	Table->DealerLine(FString::Printf(TEXT("You're moving, kid. Table %d, seat %d. Grab your chips."), H.TableId, ss::live::SeatLabel(H.Seat)));
	RaiseBanner(FString::Printf(TEXT("TABLE %d   SEAT %d"), H.TableId, ss::live::SeatLabel(H.Seat)));
	if (!Pawn)
	{
		Table->PrepareNext();
		Phase = EBackRoomPhase::Playing;
		Table->Resume(1.0f);
		return;
	}
	// Up from the seat, out through the rope and into the aisle...
	const FVector Eye = Pawn->GetEye();
	const TArray<FVector> Out = {Eye, Eye + FVector(-34.0, -16.0, 44.0), FVector(-200.0, -150.0, 165.0), FVector(-140.0, -380.0, 166.0), FVector(-200.0, -700.0, 166.0)};
	TWeakObjectPtr<ABackRoomGameMode> Self = this;
	Pawn->PlayWalk(Out, 4.0f, true, [Self]() {
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
		GM->GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(GM, [GM]() {
			GM->Stage->SetRoomTurn(GM->Stage->GetRoomTurn() + 1);
			GM->Table->PrepareNext();
			GM->PlaceExtras();
			APlayerController* PC2 = GM->GetWorld()->GetFirstPlayerController();
			if (PC2 && PC2->PlayerCameraManager)
			{
				PC2->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 0.6f, FLinearColor::Black, true, false);
			}
			ABackRoomPawn* P = Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(GM, 0));
			const FVector SeatEye = GM->Stage ? GM->Stage->EyeLocation() : FVector(-91.0, 0.0, 118.0);
			const TArray<FVector> In = {FVector(-200.0, -700.0, 166.0), FVector(-140.0, -380.0, 166.0), FVector(-205.0, -130.0, 162.0), FVector(-160.0, 6.0, 148.0), SeatEye};
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
		Table->DealerLine(TEXT("You sure, kid? Walk now and your stack gets blinded off."));
		return;
	}
	// Gone: the stack sits there and the blinds take it, hand by hand, until it's out (or it limps into
	// the money on its own).
	QuitAskedAt = -1.0f;
	Tourney->HeroSitsOut = true;
	for (int32 Guard = 0; Guard < 4000 && !Tourney->Hero().Busted && !Tourney->bFinished; ++Guard)
	{
		for (const ss::TEvent& E : Tourney->SimulateTick())
		{
			if (E.Type == ss::TEventType::Bust && E.IsHero)
			{
				HeroPlace = E.Place;
				HeroPrizeCents = E.PrizeCents;
			}
		}
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
	const int32 Field = Tourney->Spec.Entrants;
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
	Summary.Add(bHeroWon ? FString(TEXT("RIVERSIDE SUNDAY $150   CHAMPION")) : FString(TEXT("RIVERSIDE SUNDAY $150")));
	const int32 Sat = FMath::Max(0, FMath::FloorToInt(Minutes - LeftHomeAt - 20.0));
	Summary.Add(FString::Printf(TEXT("%s of %d   ·   %d hands   ·   %dh %02dm"), *Ordinal(HeroPlace), Field, HandsPlayed, Sat / 60, Sat % 60));
	Summary.Add(HeroPrizeCents > 0 ? FString::Printf(TEXT("Cashed   %s"), *Dollars(HeroPrizeCents)) : FString::Printf(TEXT("No cash   ·   %d paid"), Tourney->PaidPlaces()));
	Summary.Add(FString::Printf(TEXT("Net   %s%s"), NetCents >= 0 ? TEXT("+") : TEXT("-"), *Dollars(FMath::Abs(NetCents))));
	Summary.Append(Learned);
	// Dee, from the box.
	FString Line;
	if (bHeroWon)
	{
		Line = TEXT("Kid. You won the whole thing. Go get your money before I cry.");
	}
	else if (HeroPrizeCents > 0)
	{
		Line = FString::Printf(TEXT("%s. That's a cash, kid. Cage is behind you."), *Ordinal(HeroPlace));
	}
	else if (HeroPlace <= Tourney->PaidPlaces() + 3)
	{
		Line = TEXT("Right before the money. That one stings. Go home, sleep it off.");
	}
	else
	{
		Line = FString::Printf(TEXT("%s of %d. Happens. See you Tuesday."), *Ordinal(HeroPlace), Field);
	}
	Table->DealerLine(Line);
	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
	UE_LOG(LogRiverside, Log, TEXT("Riverside over: %s of %d, prize %lld cents, net %lld, %d hands"), *Ordinal(HeroPlace), Field, HeroPrizeCents, NetCents, HandsPlayed);
}

void ABackRoomGameMode::LiveSettle()
{
	if (!Save.IsValid())
	{
		return;
	}
	ss::SaveData D = *Save;
	D.BankrollCents = BaseCents + HeroPrizeCents;
	// The bus home.
	D.ClockMinutes = Minutes - ss::net::MinutesPerDay * ss::net::NightOneDay + 25.0;
	D.Life.Energy = FMath::Clamp(static_cast<double>(Energy), 0.0, 100.0);
	for (const TPair<FString, int32>& R : Table->Reads)
	{
		D.Life.Reads[std::string(TCHAR_TO_UTF8(*R.Key))] = R.Value;
	}
	D.Life.LiveEvents += 1;
	D.Life.LiveCashes += HeroPrizeCents > 0 ? 1 : 0;
	{
		// The living world hears where the feature table's people finished (0: still in when the player left).
		std::vector<std::pair<std::string, int>> Places;
		for (const ss::live::CastMember& C : ss::live::RiversideCast())
		{
			for (const ss::TPlayer& P : Tourney->Players)
			{
				if (P.Name == C.Name)
				{
					Places.push_back({P.Name, P.Busted ? P.Place : 0});
				}
			}
		}
		D.NoteRiverside(Minutes, HeroPlace, Tourney->Spec.Entrants, Places);
	}
	D.Life.LiveBestPlace = D.Life.LiveBestPlace <= 0 ? HeroPlace : FMath::Min(D.Life.LiveBestPlace, HeroPlace);
	D.Life.LiveWonCents += HeroPrizeCents;
	D.Life.Record(Minutes, std::string(TCHAR_TO_UTF8(*FString::Printf(TEXT("Riverside $150: %s of %d"), *Ordinal(HeroPlace), Tourney->Spec.Entrants))),
		HeroPrizeCents - BoughtInCents, 0);
	*Save = D;
	CareerSave::SaveNow(D.Serialize());
}

void ABackRoomGameMode::LiveGoHome()
{
	if (bGoingHome)
	{
		return;
	}
	bGoingHome = true;
	ABackRoomChips::ChipUnit = 1;
	FString Options = FString::Printf(TEXT("Home?Live=1?Place=%d?Of=%d?Prize=%lld?Net=%lld?Learned=%d?From=%.0f"), HeroPlace, Tourney->Spec.Entrants, HeroPrizeCents, NetCents,
		Table ? Table->LearnedThisNight : 0, LeftHomeAt);
	if (bHeroWon)
	{
		Options += TEXT("?Won");
	}
	UGameplayStatics::OpenLevel(this, FName(TEXT("NightOne")), true, Options);
}

void ABackRoomGameMode::UpdateBoard(float RealDt)
{
	BoardTick -= RealDt;
	if (BoardTick > 0.0f || !Stage || !Tourney)
	{
		return;
	}
	BoardTick = 0.25f;
	const ss::Level& L = Tourney->CurrentLevel();
	const ss::Level& N = Tourney->NextLevel();
	const int32 Secs = FMath::FloorToInt(LevelTimeLeft());
	const bool bBeforeCards = Phase == EBackRoomPhase::Arriving && !bAnnouncedStart && Tourney->Tick == 0;
	const FString Title = bFinalTable ? TEXT("FINAL TABLE") : TEXT("RIVERSIDE SUNDAY $150");
	const FString Clock = bBeforeCards ? TEXT("SOON") : FString::Printf(TEXT("%02d:%02d"), Secs / 60, Secs % 60);
	const FString Blinds = L.Ante > 0 ? FString::Printf(TEXT("%s / %s   ANTE %s"), *Num(L.Sb), *Num(L.Bb), *Num(L.Ante)) : FString::Printf(TEXT("%s / %s"), *Num(L.Sb), *Num(L.Bb));
	const FString Next = FString::Printf(TEXT("NEXT   %s / %s"), *Num(N.Sb), *Num(N.Bb));
	const FString Field = bHeroWon ? FString::Printf(TEXT("CHAMPION   %s"), Save.IsValid() ? *Str(Save->HeroName) : TEXT(""))
								   : FString::Printf(TEXT("PLAYERS %d / %d     AVG %s     PAID %d"), Tourney->Remaining, Tourney->Spec.Entrants, *Num(static_cast<int64>(Tourney->AverageStack())), Tourney->PaidPlaces());
	Stage->SetBoard(Title, FString::Printf(TEXT("LEVEL %d"), Tourney->LevelIndex + 1), Clock, Blinds, Next, Field);
}

void ABackRoomGameMode::LiveTick(float RealDt)
{
	LiveT += RealDt;
	BannerAge += RealDt;
	ABackRoomPawn* Pawn = Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	TWeakObjectPtr<ABackRoomGameMode> Self = this;

	for (const ss::TEvent& E : Table->TakeEvents())
	{
		LiveEvent(E);
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
					Tourney->MoveToTable(ss::HeroId, It.first);
					Table->Hold();
					LiveMove();
					break;
				}
			}
		}
	}
	// (Testing: the whole room plays on at once, you on autopilot, between hands.)
	if (FastForwardLeft > 0 && Phase == EBackRoomPhase::Playing && Table->IsBetweenHands())
	{
		ss::Profile Auto = ss::MakeProfile(ss::Archetype::Tag, Tourney->R);
		bool bMoved = false;
		for (; FastForwardLeft > 0 && !Tourney->Hero().Busted && !Tourney->bFinished && !bMoved; --FastForwardLeft)
		{
			for (const ss::TEvent& E : Tourney->SimulateTick(&Auto))
			{
				bMoved = bMoved || (E.Type == ss::TEventType::Moved && E.IsHero);
				LiveEvent(E);
			}
		}
		FastForwardLeft = 0;
		Minutes = FMath::Max(Minutes, LiveDayStart + Tourney->ClockMinutes());
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
	UpdateBoard(RealDt);

	// Walking in from the casino floor.
	ArrivalT += RealDt;
	if (Phase == EBackRoomPhase::Arriving)
	{
		StartWalkIn(Pawn);
		if (ArrivalBeat == 1 && ArrivalT > 3.2f)
		{
			ArrivalBeat = 2;
			Table->DealerLine(TEXT("Look who made it across town. You're with me tonight, kid."));
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
	// Seated: the floor starts the night (or welcomes a late one).
	if (Phase == EBackRoomPhase::Playing && !bAnnouncedStart)
	{
		bAnnouncedStart = true;
		if (Tourney->Tick == 0)
		{
			Floor(FString::Printf(TEXT("Ladies and gentlemen, welcome to the Riverside Sunday one-fifty. %d players, a prize pool of %s. Dealers, shuffle up and deal!"),
				Tourney->Spec.Entrants, *Dollars(Tourney->PrizePoolCents)));
			RaiseBanner(TEXT("SHUFFLE UP AND DEAL"));
		}
		else
		{
			Floor(FString::Printf(TEXT("Late registration closes at seven forty-five. %d players, %s in the pool."), Tourney->Spec.Entrants, *Dollars(Tourney->PrizePoolCents)));
			RaiseBanner(FString::Printf(TEXT("LEVEL %d   %s / %s"), Tourney->LevelIndex + 1, *Num(Tourney->CurrentLevel().Sb), *Num(Tourney->CurrentLevel().Bb)));
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
	// Hand for hand: the table waits for the rest of the room, then the floor calls the next one.
	if (MoveT < 0.0f && Phase == EBackRoomPhase::Playing && Table->IsHolding())
	{
		MoveT += RealDt;
		if (MoveT >= 0.0f)
		{
			MoveT = -1000.0f;
			if (FMath::FRand() < 0.4f)
			{
				Floor(TEXT("All tables complete. Dealers, next hand."), false);
			}
			Table->Resume(0.5f);
		}
	}
	if (QuitAskedAt >= 0.0f && LiveT - QuitAskedAt > 4.0f)
	{
		QuitAskedAt = -1.0f;
	}

	// Out (or champion): Dee's goodbye, the summary, the cage if you cashed, and the doors.
	if (Phase == EBackRoomPhase::Leaving)
	{
		LeaveT += RealDt;
		Eyelids = 0.0f;
		if (!bWalkingOut && LeaveT > (bHeroWon ? 6.0f : 3.0f))
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
				const TArray<FVector> ToCage = {Eye, Eye + FVector(-34.0, -16.0, 44.0), FVector(-200.0, -150.0, 165.0), FVector(-140.0, -390.0, 166.0),
					Room.TransformPosition(FVector(-1000.0, -380.0, 166.0)), Room.TransformPosition(FVector(-1300.0, -60.0, 166.0))};
				Pawn->PlayWalk(ToCage, 7.0f, true, [Self, Room]() {
					ABackRoomGameMode* GM = Self.Get();
					if (!GM)
					{
						return;
					}
					GM->Table->Announce(TEXT("Cashier"), FString::Printf(TEXT("Congratulations. %s, cash. Count it with me... there you go."), *Dollars(GM->HeroPrizeCents)));
					if (GM->Table->GetAudio())
					{
						GM->Table->GetAudio()->Play(ss::SoundId::Cash, 0.8f);
					}
					FTimerHandle Handle;
					GM->GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(GM, [GM, Room]() {
						ABackRoomPawn* P = Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(GM, 0));
						if (!P)
						{
							GM->LiveGoHome();
							return;
						}
						const TArray<FVector> Out = {P->GetEye(), Room.TransformPosition(FVector(-1000.0, -600.0, 166.0)), Room.TransformPosition(FVector(-880.0, -1005.0, 166.0)),
							Room.TransformPosition(FVector(380.0, -1000.0, 166.0)), Room.TransformPosition(FVector(400.0, -1080.0, 166.0))};
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
