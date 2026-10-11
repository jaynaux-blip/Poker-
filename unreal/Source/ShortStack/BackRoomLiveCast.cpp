// Who sits at the player's table at the Embercrest (ABackRoomGameMode's live tournaments; the flow is BackRoomLive.cpp):
// the faces in the field and what they say sitting down, the bodies taking the chairs and leaving them (in a blink
// where the player can see), and what the hero can see.

#include "BackRoomLiveShared.h"

#include "BackRoomGameMode.h"
#include "BackRoomPlayer.h"
#include "BackRoomStage.h"
#include "BackRoomTable.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "ShortStack/Game/Chat.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Tournament.h"

namespace LiveDetail
{
/** The room's body an actor wears ("ExtraC", "Sal"; empty when it has none): the folder its MetaHuman was built in. */
FString WornBody(const ABackRoomPlayer* P)
{
	if (!P)
	{
		return FString();
	}
	const FString Path = P->MetaHumanClass.ToSoftObjectPath().ToString();
	const int32 At = Path.Find(TEXT("/MHC_"));
	if (At == INDEX_NONE)
	{
		return FString();
	}
	const FString Rest = Path.Mid(At + 5);
	int32 End = INDEX_NONE;
	return Rest.FindChar(TEXT('/'), End) ? Rest.Left(End) : Rest;
}
} // namespace LiveDetail

using namespace LiveDetail;

// ------------------------------------------------------------------ people

FString ABackRoomGameMode::FaceNote(const FString& Name) const
{
	const TPair<FString, FString>* Face = LiveFaces.Find(Name);
	return Face ? Face->Key : FString();
}

ss::badge::Set ABackRoomGameMode::FaceBadges(const FString& Name) const
{
	// (ue-wear fills LiveBadges from the entry's livebadge lines, next_segment_spec 5.7.)
	const ss::badge::Set* Held = LiveBadges.Find(Name);
	return Held ? *Held : ss::badge::Set();
}

FString ABackRoomGameMode::FacePatch(const FString& Name) const
{
	// (ue-wear fills LivePatches from the entry's livepatch lines, next_segment_spec 3.10.)
	const FString* Brand = LivePatches.Find(Name);
	return Brand ? *Brand : FString();
}

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
		// Their own face (the cast), or the room's body they've worn at another table tonight, or the one their id picks
		// (the same every night they play), passing over any already sitting at this table and the dealer's.
		TSet<FString> Worn;
		for (const TPair<FString, TObjectPtr<ABackRoomPlayer>>& It : CastActors)
		{
			const ABackRoomPlayer* O = It.Value.Get();
			const bool bSeated = O && (!O->IsHidden() || BlinkSwaps.ContainsByPredicate([O](const TPair<TWeakObjectPtr<ABackRoomPlayer>, bool>& S) {
				return S.Key.Get() == O && S.Value;
			}));
			if (bSeated)
			{
				Worn.Add(WornBody(O));
			}
		}
		Worn.Add(WornBody(Dealer.Get()));
		A = SpawnPerson(LiveBodyFor(Id, Name, Worn), At, EBackRoomRole::Player, PersonaFor(Name));
		CastActors.Add(Id, A);
	}
	A->SetActorTransform(At);
	// Someone who knows the player (or a regular the dealer knows), sitting down with them for the first time tonight:
	// a word once they're settled. (Not once the player's night is over: they sit down at the old table without them.)
	const TPair<FString, FString>* Face = LiveMet.Contains(Id) || bLiveOver ? nullptr : LiveFaces.Find(Name);
	if (Face)
	{
		const FString Line = Face->Value;
		const bool bRegular = Face->Key == TEXT("Embercrest regular");
		const float Delay = (Phase == EBackRoomPhase::Arriving ? 11.0f : 2.4f) + 3.4f * static_cast<float>(GreetCount++ % 3);
		TWeakObjectPtr<ABackRoomPlayer> Who = A;
		TWeakObjectPtr<ABackRoomTable> Tb = Table.Get();
		const int32 Pick = static_cast<int32>(FCrc::StrCrc32(*(Name + LiveEntryId)) % 3);
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [Who, Tb, Line, bRegular, Name, Pick]() {
			ABackRoomPlayer* P = Who.Get();
			if (!P || P->IsHidden())
			{
				return;
			}
			if (!Line.IsEmpty())
			{
				P->Say(Line);
			}
			else if (bRegular && Tb.IsValid())
			{
				Tb->DealerLine(Pick == 0 ? FString::Printf(TEXT("Hey, %s."), *Name)
							   : Pick == 1 ? FString::Printf(TEXT("Good to see you, %s."), *Name)
										   : FString::Printf(TEXT("%s. Good luck tonight."), *Name));
			}
		}), Delay, false);
	}
	if (!bLiveOver)
	{
		LiveMet.Add(Id);
	}
	// In plain sight (from the chair, or from the rail), a new face takes the chair in a blink rather than appearing in it.
	BlinkSwaps.RemoveAll([A](const TPair<TWeakObjectPtr<ABackRoomPlayer>, bool>& S) { return S.Key.Get() == A; });
	if ((Phase == EBackRoomPhase::Playing || bRailing) && MoveT < 0.0f && HeroCanSee(At.GetLocation() + FVector(0.0, 0.0, 110.0)))
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
	if (!Face && Phase == EBackRoomPhase::Playing && bAnnouncedStart && Dealer && FMath::FRand() < 0.6f)
	{
		Table->DealerLine(FString::Printf(TEXT("Welcome. Seat %d."), ss::live::SeatLabel(P.Seat)));
	}
	// gh0stfold, across a real table for the first time.
	if (Name == UTF8_TO_TCHAR(ss::RivalName) && !bGhostMet && !bLiveOver)
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
	if ((Phase == EBackRoomPhase::Playing || bRailing) && MoveT < 0.0f && HeroCanSee(Player->GetActorLocation() + FVector(0.0, 0.0, 110.0)))
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
	UE_LOG(LogEmbercrest, Log, TEXT("Seat change in a blink: %d bodies"), BlinkSwaps.Num());
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
