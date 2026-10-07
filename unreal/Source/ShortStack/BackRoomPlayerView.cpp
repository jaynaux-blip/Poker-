// The hero's head for the third-person view at the table (kept apart from BackRoomPlayer.cpp).
#include "BackRoomPlayer.h"

#include "CastLooks.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "GroomAsset.h"
#include "GroomBindingAsset.h"
#include "GroomComponent.h"
#include "HeroGrooms.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace BackRoomPlayerViewDetail
{
using HeroGrooms::EGroomKind;

/**
 * Whether the hero wears one of the MetaHuman's own grooms, given the creator's choices, where the face has no groom library
 * (HeroGrooms::WearsBuildGroom, the street's rule): shaved is bald, a buzz cut is bald unless this hair is cropped that short,
 * and under a hat only hair that lies close stays; no facial hair takes the beard and mustache off, stubble keeps only
 * stubble, a mustache only the mustache. A career from before the creator wears them all.
 */
bool HeroWearsGroom(const FBackRoomPersona& P, EGroomKind Kind, const FString& Asset)
{
	if ((Kind == EGroomKind::Scalp && P.HeroHairStyle < 0) || ((Kind == EGroomKind::Beard || Kind == EGroomKind::Mustache) && P.HeroFacialHair < 0))
	{
		return true;
	}
	ss::hero::Look L;
	L.Hair = P.HeroHairStyle;
	L.FacialHair = P.HeroFacialHair;
	L.Hat = P.HeroHat;
	return HeroGrooms::WearsBuildGroom(L, Kind, Asset);
}

/** The grooms the library swaps; the lashes and the fuzz stay as built. */
bool IsManaged(EGroomKind Kind)
{
	return Kind == EGroomKind::Scalp || Kind == EGroomKind::Brows || Kind == EGroomKind::Beard || Kind == EGroomKind::Mustache;
}

/** The hero face a Blueprint was assembled as (".../MHC_HeroA0/BP_MHC_HeroA0" is HeroA0); None for any other. */
FName HeroFaceOf(const UClass* Blueprint)
{
	const FString Path = Blueprint ? Blueprint->GetPathName() : FString();
	const int32 At = Path.Find(TEXT("/MHC_Hero"));
	if (At == INDEX_NONE)
	{
		return NAME_None;
	}
	const FString Rest = Path.Mid(At + 5);
	int32 End = INDEX_NONE;
	return FName(*(Rest.FindChar(TEXT('/'), End) ? Rest.Left(End) : Rest));
}

/** What the library puts on the hero's head at the table (the look the game mode bound as it seated them); unset without either. */
HeroGrooms::FGroomSet TableSet(const ABackRoomPlayer* Player, const UClass* Blueprint)
{
	ss::hero::Look Look;
	const FName Face = HeroFaceOf(Blueprint);
	if (Face.IsNone() || !CastLooks::HeroLookOn(Player, Look))
	{
		return HeroGrooms::FGroomSet();
	}
	return HeroGrooms::Resolve(Look, Face);
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
	const HeroGrooms::FGroomSet Set = TableSet(this, Bp);
	if (Set.bLibrary)
	{
		return Set.Scalp.IsSet();
	}
	for (const USCS_Node* Node : Bp->SimpleConstructionScript->GetAllNodes())
	{
		const UGroomComponent* Template = Node ? Cast<UGroomComponent>(Node->ComponentTemplate) : nullptr;
		if (!Template || !Template->GroomAsset)
		{
			continue;
		}
		const FString Asset = Template->GroomAsset->GetName();
		if (HeroGrooms::KindOf(Node->GetVariableName().ToString(), Asset) == EGroomKind::Scalp && HeroWearsGroom(Persona, EGroomKind::Scalp, Asset))
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
	// The grooms Build skipped for the hero: with the face's groom library, its scalp, brows, beard and mustache as the look
	// asks (HeroGrooms::Resolve, the street's one resolver; the game mode bound the look and started them loading as it seated
	// the hero) and the build's own lashes and fuzz; without one, the build's own as far as the creator's hairstyle, facial hair
	// and hat leave them on. In the creator's hair color. They're made hidden, with the face: the head shows once the camera is
	// clear of it. Every cast build holds its grooms on their nearest cards, as the street holds them (without the Blueprint's
	// LOD sync a groom falls to its helmet).
	const HeroGrooms::FGroomSet Set = TableSet(this, Bp);
	auto Shade = [this](UGroomComponent* Copy, EGroomKind Kind) {
		if (Persona.HairMelanin < 0.0f || Kind == EGroomKind::Lashes)
		{
			return;
		}
		// As the street colors it: the hair materials take hairMelanin, hairRedness and WhiteAmount, the peach fuzz Melanin and
		// Redness; the brows a shade darker, the lashes left dark.
		const float Melanin = FMath::Min(1.0f, Persona.HairMelanin + (Kind == EGroomKind::Brows ? 0.06f : 0.0f));
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
	};
	TMap<uint8, const UGroomComponent*> Templates;
	for (USCS_Node* Node : Bp->SimpleConstructionScript->GetAllNodes())
	{
		const UGroomComponent* Template = Node ? Cast<UGroomComponent>(Node->ComponentTemplate) : nullptr;
		if (!Template || !Template->GroomAsset)
		{
			continue;
		}
		const FString Asset = Template->GroomAsset->GetName();
		const EGroomKind Kind = HeroGrooms::KindOf(Node->GetVariableName().ToString(), Asset);
		if (!Templates.Contains(static_cast<uint8>(Kind)))
		{
			Templates.Add(static_cast<uint8>(Kind), Template);
		}
		if (Set.bLibrary ? IsManaged(Kind) : !HeroWearsGroom(Persona, Kind, Asset))
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
		Shade(Copy, Kind);
		Copy->RegisterComponent();
		HeroGrooms::HoldOnCards(Copy);
		HeroHair.Add(Copy);
	}
	if (!Set.bLibrary || !Face)
	{
		return;
	}
	for (const EGroomKind Kind : {EGroomKind::Scalp, EGroomKind::Brows, EGroomKind::Beard, EGroomKind::Mustache})
	{
		const HeroGrooms::FWornGroom& Want = Set.Of(Kind);
		if (!Want.IsSet())
		{
			continue;
		}
		// In memory already (the seat started the load); if not, it's loaded here, as going over the shoulder builds the head.
		UGroomAsset* Groom = Want.Groom.LoadSynchronous();
		UGroomBindingAsset* Binding = Want.Binding.LoadSynchronous();
		if (!Groom)
		{
			continue;
		}
		// The build's own groom of this kind is the template (its settings as the Blueprint made them), when it has one.
		const UGroomComponent* const* Template = Templates.Find(static_cast<uint8>(Kind));
		UGroomComponent* Copy = NewObject<UGroomComponent>(this, UGroomComponent::StaticClass(), MakeUniqueObjectName(this, UGroomComponent::StaticClass(), Want.Style),
			RF_Transient, Template ? const_cast<UGroomComponent*>(*Template) : nullptr);
		Copy->SetupAttachment(Face.Get());
		Copy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Copy->SetHiddenInGame(true);
		// The template's overrides are its own groom's; set before registering, so it's built once, as this groom.
		Copy->EmptyOverrideMaterials();
		Copy->SetGroomAsset(Groom, Binding);
		Shade(Copy, Kind);
		Copy->RegisterComponent();
		HeroGrooms::HoldOnCards(Copy);
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
