// The hero's head for the third-person view at the table (kept apart from BackRoomPlayer.cpp).
#include "BackRoomPlayer.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "GroomAsset.h"
#include "GroomComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace BackRoomPlayerViewDetail
{
/** What one of a MetaHuman's grooms is for, by its variable (Hair, Eyebrows, Beard...) or its asset (Beard_S_Stubble). */
enum class EHeroGroom : uint8
{
	Scalp,
	Brows,
	Lashes,
	Beard,
	Mustache,
	Fuzz,
};

// The hero wears the creator's hair at the table as on the street (ShortStackCharacter.cpp sorts and picks the grooms
// the same way): kept in step with it by hand.
EHeroGroom HeroGroomKind(const FString& Variable, const FString& Asset)
{
	auto Is = [&](const TCHAR* Word) { return Variable.Contains(Word) || Asset.StartsWith(Word); };
	if (Is(TEXT("Eyelash")))
	{
		return EHeroGroom::Lashes;
	}
	if (Is(TEXT("Eyebrow")))
	{
		return EHeroGroom::Brows;
	}
	if (Is(TEXT("Mustache")))
	{
		return EHeroGroom::Mustache;
	}
	if (Is(TEXT("Beard")) || Is(TEXT("Goatee")))
	{
		return EHeroGroom::Beard;
	}
	if (Is(TEXT("Fuzz")) || Is(TEXT("Peachfuzz")))
	{
		return EHeroGroom::Fuzz;
	}
	return EHeroGroom::Scalp;
}

/** Hair that lies close to the scalp, so a hat sits over it without it coming up through the crown. */
bool HairUnderHat(const FString& Asset)
{
	static const TCHAR* const Tight[] = {TEXT("BuzzCut"), TEXT("360Waves"), TEXT("BaldingStubble"), TEXT("HairLoss"), TEXT("Cornrows"), TEXT("SlickBack"),
		TEXT("PulledBack"), TEXT("LowPonytail"), TEXT("RecedeMessy"), TEXT("BrushCut"), TEXT("CurlyFade")};
	for (const TCHAR* Style : Tight)
	{
		if (Asset.Contains(Style))
		{
			return true;
		}
	}
	return false;
}

/** Hair cropped about as short as a buzz cut (a ponytail or cornrows lie close, but aren't one). */
bool HairCropped(const FString& Asset)
{
	static const TCHAR* const Cropped[] = {TEXT("BuzzCut"), TEXT("360Waves"), TEXT("BaldingStubble"), TEXT("HairLoss"), TEXT("BrushCut"), TEXT("CurlyFade")};
	for (const TCHAR* Style : Cropped)
	{
		if (Asset.Contains(Style))
		{
			return true;
		}
	}
	return false;
}

/**
 * Whether the hero wears one of the MetaHuman's grooms, given the creator's choices: shaved is bald, a buzz cut is bald
 * unless this hair is cropped that short, and under a hat only hair that lies close stays; no facial hair takes the beard
 * and mustache off, stubble keeps only stubble, a mustache only the mustache. A career from before the creator wears them all.
 */
bool HeroWearsGroom(const FBackRoomPersona& P, EHeroGroom Kind, const FString& Asset)
{
	switch (Kind)
	{
	case EHeroGroom::Scalp:
		if (P.HeroHairStyle < 0)
		{
			return true;
		}
		if (P.HeroHairStyle == 0 || (P.HeroHairStyle == 1 && !HairCropped(Asset)))
		{
			return false;
		}
		return P.HeroHat == 0 || HairUnderHat(Asset);
	case EHeroGroom::Beard:
		return P.HeroFacialHair < 0 || P.HeroFacialHair >= 3 || (P.HeroFacialHair == 1 && Asset.Contains(TEXT("Stubble")));
	case EHeroGroom::Mustache:
		return P.HeroFacialHair < 0 || P.HeroFacialHair >= 2 || (P.HeroFacialHair == 1 && Asset.Contains(TEXT("Stubble")));
	default:
		return true;
	}
}
} // namespace BackRoomPlayerViewDetail

using namespace BackRoomPlayerViewDetail;

bool ABackRoomPlayer::HeroWearsScalpHair() const
{
	const UBlueprintGeneratedClass* Bp = Cast<UBlueprintGeneratedClass>(WornClass.Get());
	if (SeatRole != EBackRoomRole::Hero || !Bp || !Bp->SimpleConstructionScript)
	{
		return false;
	}
	for (const USCS_Node* Node : Bp->SimpleConstructionScript->GetAllNodes())
	{
		const UGroomComponent* Template = Node ? Cast<UGroomComponent>(Node->ComponentTemplate) : nullptr;
		if (!Template || !Template->GroomAsset)
		{
			continue;
		}
		const FString Asset = Template->GroomAsset->GetName();
		if (HeroGroomKind(Node->GetVariableName().ToString(), Asset) == EHeroGroom::Scalp && HeroWearsGroom(Persona, EHeroGroom::Scalp, Asset))
		{
			return true;
		}
	}
	return false;
}

void ABackRoomPlayer::SetHeroHeadBuilt(bool bBuild)
{
	if (SeatRole != EBackRoomRole::Hero || bBuild == bHeroHeadBuilt)
	{
		return;
	}
	if (!bBuild)
	{
		// Back to first person for good: the hair goes before the face stops drawing (a groom bound to a face that isn't
		// drawn trips the hair system's checks), and the face goes back to a coarse shadow.
		for (USceneComponent* H : HeroHair)
		{
			if (H)
			{
				H->DestroyComponent();
			}
		}
		HeroHair.Reset();
		bHeroHeadBuilt = false;
		bHeroHeadShown = false;
		ApplyHeroHeadState();
		return;
	}
	bHeroHeadBuilt = true;
	bHeroHeadShown = false;
	ApplyHeroHeadState();
	UBlueprintGeneratedClass* Bp = Cast<UBlueprintGeneratedClass>(WornClass.Get());
	if (!Bp || !Bp->SimpleConstructionScript || HeroHair.Num() > 0)
	{
		return;
	}
	// The grooms Build skipped for the hero, copied as it copies everything else: the ones the creator's hairstyle, facial
	// hair and hat leave on, in the creator's hair color. They're made hidden, with the face: the head shows once the
	// camera is clear of it. The hero builds below the cinematic tier (HeroA*, HeroB*) carry cards, not strands: they're
	// held on their nearest cards, as the street holds them (without the Blueprint's LOD sync a groom falls to its helmet).
	const bool bCards = Bp->GetPathName().Contains(TEXT("/MHC_HeroA")) || Bp->GetPathName().Contains(TEXT("/MHC_HeroB"));
	for (USCS_Node* Node : Bp->SimpleConstructionScript->GetAllNodes())
	{
		const UGroomComponent* Template = Node ? Cast<UGroomComponent>(Node->ComponentTemplate) : nullptr;
		if (!Template || !Template->GroomAsset)
		{
			continue;
		}
		const FString Asset = Template->GroomAsset->GetName();
		const EHeroGroom Kind = HeroGroomKind(Node->GetVariableName().ToString(), Asset);
		if (!HeroWearsGroom(Persona, Kind, Asset))
		{
			continue;
		}
		const FName Name = MakeUniqueObjectName(this, Template->GetClass(), Node->GetVariableName());
		UGroomComponent* Copy = NewObject<UGroomComponent>(this, Template->GetClass(), Name, RF_Transient, const_cast<UGroomComponent*>(Template));
		const USCS_Node* ParentNode = Bp->SimpleConstructionScript->FindParentNode(Node);
		USceneComponent* Parent = ParentNode && ParentNode->GetVariableName() == TEXT("Face") ? static_cast<USceneComponent*>(Face.Get()) : static_cast<USceneComponent*>(Body.Get());
		Copy->SetupAttachment(Parent, Node->AttachToName);
		Copy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Copy->SetHiddenInGame(true);
		if (Persona.HairMelanin >= 0.0f && Kind != EHeroGroom::Lashes)
		{
			// As the street colors it: the hair materials take hairMelanin, hairRedness and WhiteAmount, the peach fuzz
			// Melanin and Redness; the brows a shade darker, the lashes left dark.
			const float Melanin = FMath::Min(1.0f, Persona.HairMelanin + (Kind == EHeroGroom::Brows ? 0.06f : 0.0f));
			for (int32 Slot = 0; Slot < Copy->GetNumMaterials(); ++Slot)
			{
				if (UMaterialInterface* Material = Copy->GetMaterial(Slot))
				{
					UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Material, Copy);
					Mid->SetScalarParameterValue(TEXT("hairMelanin"), Melanin);
					Mid->SetScalarParameterValue(TEXT("hairRedness"), Persona.HairRedness);
					Mid->SetScalarParameterValue(TEXT("WhiteAmount"), Persona.HairWhite);
					Mid->SetScalarParameterValue(TEXT("Melanin"), Melanin);
					Mid->SetScalarParameterValue(TEXT("Redness"), Persona.HairRedness);
					Copy->SetMaterial(Slot, Mid);
				}
			}
		}
		Copy->RegisterComponent();
		if (bCards)
		{
			int32 CardsLOD = INDEX_NONE;
			for (const FHairGroupsCardsSourceDescription& Cards : Copy->GroomAsset->GetHairGroupsCards())
			{
				if (Cards.ImportedMesh && Cards.LODIndex >= 0)
				{
					CardsLOD = CardsLOD == INDEX_NONE ? Cards.LODIndex : FMath::Min(CardsLOD, Cards.LODIndex);
				}
			}
			if (CardsLOD != INDEX_NONE)
			{
				Copy->SetForcedLOD(CardsLOD);
			}
		}
		HeroHair.Add(Copy);
	}
}

void ABackRoomPlayer::ShowHeroHead(bool bShow)
{
	const bool bNow = bShow && bHeroHeadBuilt;
	if (SeatRole != EBackRoomRole::Hero || bNow == bHeroHeadShown)
	{
		return;
	}
	bHeroHeadShown = bNow;
	ApplyHeroHeadState();
}

void ABackRoomPlayer::ApplyHeroHeadState()
{
	if (SeatRole != EBackRoomRole::Hero || !Face)
	{
		return;
	}
	// In first person the face (and what's on it) is there for its shadow only: a coarse level of detail does for that.
	// Built for the shoulder camera it keeps its full detail, shown or not, so the hair stays bound to the same face.
	const bool bDraw = bHeroHeadBuilt && bHeroHeadShown;
	if (!bDraw)
	{
		// The hair before the face, going; the face before the hair, coming back.
		for (USceneComponent* H : HeroHair)
		{
			if (H)
			{
				H->SetHiddenInGame(true);
			}
		}
	}
	Face->SetHiddenInGame(!bDraw);
	Face->SetForcedLOD(bHeroHeadBuilt ? 0 : 4);
	for (USceneComponent* W : Wearables)
	{
		if (W && W->GetAttachParent() == Face)
		{
			W->SetHiddenInGame(!bDraw);
		}
	}
	for (UStaticMeshComponent* W : Worn)
	{
		if (W)
		{
			W->SetHiddenInGame(!bDraw);
		}
	}
	if (bDraw)
	{
		for (USceneComponent* H : HeroHair)
		{
			if (H)
			{
				H->SetHiddenInGame(false);
			}
		}
	}
}
