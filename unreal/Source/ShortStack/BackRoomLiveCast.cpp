// Who sits at the player's table at the Embercrest (ABackRoomGameMode's live tournaments; the flow is BackRoomLive.cpp):
// the faces in the field and what they say sitting down, the bodies taking the chairs and leaving them (in a blink
// where the player can see), and what the hero can see.

#include "BackRoomLiveShared.h"

#include "BackRoomGameMode.h"
#include "BackRoomPlayer.h"
#include "BackRoomStage.h"
#include "BackRoomTable.h"
#include "Camera/PlayerCameraManager.h"
#include "SponsorWear.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "ShortStack/Game/Chat.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Sponsor.h"
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

namespace LiveDetail
{
/** The night's entry in the save (the field's snapshot), before SeatCast has read it into LiveBadges and LivePatches. */
const ss::life::LiveEntry* NightEntry(const TSharedPtr<ss::SaveData>& Save, const FString& EntryId)
{
	return Save.IsValid() && !EntryId.IsEmpty() ? ss::live::EntryFor(static_cast<const ss::life::State&>(Save->Life), std::string(TCHAR_TO_UTF8(*EntryId))) : nullptr;
}
} // namespace LiveDetail

ss::badge::Set ABackRoomGameMode::FaceBadges(const FString& Name) const
{
	// The entry's livebadge lines (next_segment_spec 5.7), read with the field as the first chair is taken (SeatCast); asked
	// before that, straight from the entry.
	if (const ss::badge::Set* Held = LiveBadges.Find(Name))
	{
		return *Held;
	}
	if (!WearHost.IsValid() || WearHost->FilledFor != LiveEntryId)
	{
		if (const ss::life::LiveEntry* E = NightEntry(Save, LiveEntryId))
		{
			const std::string Key(TCHAR_TO_UTF8(*Name));
			for (const std::pair<std::string, std::string>& B : E->Badges)
			{
				if (B.first == Key)
				{
					return ss::badge::Unpack(B.second);
				}
			}
		}
	}
	return ss::badge::Set();
}

FString ABackRoomGameMode::FacePatch(const FString& Name) const
{
	// The entry's livepatch lines (next_segment_spec 3.10): the world's sponsored people, and the player (the chest's brand).
	if (const FString* Brand = LivePatches.Find(Name))
	{
		return *Brand;
	}
	if (!WearHost.IsValid() || WearHost->FilledFor != LiveEntryId)
	{
		if (const ss::life::LiveEntry* E = NightEntry(Save, LiveEntryId))
		{
			const std::string Key(TCHAR_TO_UTF8(*Name));
			for (const std::pair<std::string, std::string>& P : E->Patches)
			{
				if (P.first == Key)
				{
					return Str(P.second);
				}
			}
		}
	}
	return FString();
}

ABackRoomPlayer* ABackRoomGameMode::SeatCast(const ss::TPlayer& P, int32 TableSeat)
{
	const FString Id = Str(P.Id);
	const FString Name = Str(P.Name);
	const FTransform At = ABackRoomStage::SeatTransform(TableSeat);
	// The night's field as the entry has it, read as the first chair is taken (and kept: the snapshot goes at settle, and
	// the room plays on after): everyone's badges and sponsors' patches by name (live::DescribeField's livebadge and
	// livepatch lines). The player's own patch rows are the closet's at registration; the player wears the closet's now
	// (the same, unless a deal has ended since), filtered at the night's clock.
	if (!WearHost.IsValid())
	{
		WearHost = MakeShared<FWearHost>();
	}
	if (WearHost->FilledFor != LiveEntryId && Save.IsValid())
	{
		WearHost->FilledFor = LiveEntryId;
		// A second event in the same visit: the player is dressed again at its clock (a deal may have ended in between).
		WearHost->bHeroDressed = false;
		LiveBadges.Reset();
		LivePatches.Reset();
		// The rail's backers stand on the street's idle (ABackRoomPlayer::SetStanding): its one clip loaded now, in the
		// background while the room fills, and kept, so nobody's standing up costs a load in the middle of the night.
		SponsorWear::PrewarmStanding([Weak = TWeakObjectPtr<ABackRoomGameMode>(this)](UObject* Loaded) {
			if (ABackRoomGameMode* Mode = Weak.Get())
			{
				Mode->WearObjects.AddUnique(Loaded);
			}
		});
		if (const ss::life::LiveEntry* E = ss::live::EntryFor(static_cast<const ss::life::State&>(Save->Life), std::string(TCHAR_TO_UTF8(*LiveEntryId))))
		{
			for (const std::pair<std::string, std::string>& B : E->Badges)
			{
				LiveBadges.Add(Str(B.first), ss::badge::Unpack(B.second));
			}
			for (const std::pair<std::string, std::string>& Row : E->Patches)
			{
				// One brand a person (the world's sponsored wear one; the player's rows come in the closet's order of spots).
				if (!LivePatches.Contains(Str(Row.first)))
				{
					LivePatches.Add(Str(Row.first), Str(Row.second));
				}
			}
		}
		UE_LOG(LogEmbercrest, Log, TEXT("The field's badges: %d people; sponsors' patches: %d people"), LiveBadges.Num(), LivePatches.Num());
	}
	if (!WearHost->bHeroDressed && Hero && Save.IsValid())
	{
		WearHost->bHeroDressed = true;
		WearHost->HeroShown = SponsorWear::FromCloset(Save->Life.Closet, &Save->Life.Sponsors, Minutes);
		Hero->WearPatches(WearHost->HeroShown.Brand[SponsorWear::Chest], WearHost->HeroShown.Brand[SponsorWear::Sleeve], WearHost->HeroShown.Brand[SponsorWear::Cap]);
		if (!WearHost->HeroShown.Brand[SponsorWear::Chest].IsEmpty())
		{
			LivePatches.Add(Str(Save->HeroName), WearHost->HeroShown.Brand[SponsorWear::Chest]);
		}
	}
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
	// Their sponsor's patch, on the spot the brand's patch takes that shows on them (Team RiverLine's pros the gold one on
	// the chest; an energy drink's on a cap, else the sleeve); nothing for a brand that's only an overlay on a stream.
	{
		const FString* Brand = LivePatches.Find(Name);
		const SponsorWear::FShown Theirs = Brand ? SponsorWear::ForWorldPerson(*Brand, A->CanWearCapPatch()) : SponsorWear::FShown();
		A->WearPatches(Theirs.Brand[SponsorWear::Chest], Theirs.Brand[SponsorWear::Sleeve], Theirs.Brand[SponsorWear::Cap]);
	}
	// Someone who knows the player (or a regular the dealer knows), sitting down with them for the first time tonight:
	// a word once they're settled. (Not once the player's night is over: they sit down at the old table without them.)
	const TPair<FString, FString>* Face = LiveMet.Contains(Id) || bLiveOver ? nullptr : LiveFaces.Find(Name);
	// (At the Embercrest the room's director says a face's hello itself, at the sit-down and for a move in: one voice, spec 1.5.)
	const bool bRoomGreets = RoomDirector.IsValid();
	if (Face)
	{
		const FString Line = Face->Value;
		const bool bRegular = Face->Key == TEXT("Embercrest regular");
		const float Delay = (Phase == EBackRoomPhase::Arriving ? 11.0f : 2.4f) + 3.4f * static_cast<float>(GreetCount++ % 3);
		TWeakObjectPtr<ABackRoomPlayer> Who = A;
		TWeakObjectPtr<ABackRoomTable> Tb = Table.Get();
		const int32 Pick = static_cast<int32>(FCrc::StrCrc32(*(Name + LiveEntryId)) % 3);
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [Who, Tb, Line, bRegular, Name, Pick, bRoomGreets]() {
			ABackRoomPlayer* P = Who.Get();
			if (!P || P->IsHidden())
			{
				return;
			}
			if (!Line.IsEmpty())
			{
				if (!bRoomGreets)
				{
					P->Say(Line);
				}
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
	// (The dealer's "Welcome. Seat N." for a mover is the director's now, hash-gated; a host without one keeps the old gate.)
	if (!bRoomGreets && !Face && Phase == EBackRoomPhase::Playing && bAnnouncedStart && Dealer && FMath::FRand() < 0.6f)
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
