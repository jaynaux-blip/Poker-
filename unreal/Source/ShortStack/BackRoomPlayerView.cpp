// The hero's head for the third-person view at the table (kept apart from BackRoomPlayer.cpp).
#include "BackRoomPlayer.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "GroomComponent.h"

void ABackRoomPlayer::ShowHeroHead(bool bShow)
{
	if (SeatRole != EBackRoomRole::Hero || bShow == bHeroHeadShown)
	{
		return;
	}
	bHeroHeadShown = bShow;
	// The face (and what hangs off it: brows, lashes) shows; in first person it's back to casting a shadow only.
	Face->SetHiddenInGame(!bShow);
	Face->SetForcedLOD(bShow ? 0 : 4);
	for (USceneComponent* W : Wearables)
	{
		if (W && W->GetAttachParent() == Face)
		{
			W->SetHiddenInGame(!bShow);
		}
	}
	if (!bShow)
	{
		// The hair goes before the face stops drawing (a groom bound to a hidden face trips the hair system's checks).
		for (USceneComponent* H : HeroHair)
		{
			if (H)
			{
				H->DestroyComponent();
			}
		}
		HeroHair.Reset();
		return;
	}
	UBlueprintGeneratedClass* Bp = Cast<UBlueprintGeneratedClass>(WornClass.Get());
	if (!Bp || !Bp->SimpleConstructionScript || HeroHair.Num() > 0)
	{
		return;
	}
	// The grooms Build skipped for the hero, copied as it copies everything else.
	for (USCS_Node* Node : Bp->SimpleConstructionScript->GetAllNodes())
	{
		const UGroomComponent* Template = Node ? Cast<UGroomComponent>(Node->ComponentTemplate) : nullptr;
		if (!Template || !Template->GroomAsset)
		{
			continue;
		}
		const FName Name = MakeUniqueObjectName(this, Template->GetClass(), Node->GetVariableName());
		UGroomComponent* Copy = NewObject<UGroomComponent>(this, Template->GetClass(), Name, RF_Transient, const_cast<UGroomComponent*>(Template));
		const USCS_Node* ParentNode = Bp->SimpleConstructionScript->FindParentNode(Node);
		USceneComponent* Parent = ParentNode && ParentNode->GetVariableName() == TEXT("Face") ? static_cast<USceneComponent*>(Face.Get()) : static_cast<USceneComponent*>(Body.Get());
		Copy->SetupAttachment(Parent, Node->AttachToName);
		Copy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Copy->RegisterComponent();
		HeroHair.Add(Copy);
	}
}
