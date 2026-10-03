#include "ShortStackCharacter.h"

#include "BackRoomAnim.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "GroomComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/PackageName.h"
#include "StreetAnim.h"

namespace ShortStackCharacterDetail
{
const TCHAR* HeroArchetypeBody = TEXT("/MetaHumanCharacter/Body/IdentityTemplate/SKM_Body.SKM_Body");
const TCHAR* HeroArchetypeFace = TEXT("/MetaHumanCharacter/Face/SKM_Face.SKM_Face");

/** A cast member's assembled MetaHuman (backroom_cast.py / hero_cast.py), if it's been built. */
UClass* CastClass(const FString& Name)
{
	const FString Package = FString::Printf(TEXT("/Game/ShortStack/Cast/Built/MHC_%s/BP_MHC_%s"), *Name, *Name);
	if (!FPackageName::DoesPackageExist(Package))
	{
		return nullptr;
	}
	return LoadClass<AActor>(nullptr, *FString::Printf(TEXT("%s.BP_MHC_%s_C"), *Package, *Name));
}

FLinearColor SrgbOf(uint32 Hex)
{
	return FLinearColor::FromSRGBColor(FColor(static_cast<uint8>((Hex >> 16) & 0xff), static_cast<uint8>((Hex >> 8) & 0xff), static_cast<uint8>(Hex & 0xff)));
}

void CopyOverrides(const UMeshComponent* From, UMeshComponent* To)
{
	for (int32 I = 0; I < From->OverrideMaterials.Num(); ++I)
	{
		if (From->OverrideMaterials[I])
		{
			To->SetMaterial(I, From->OverrideMaterials[I]);
		}
	}
}

/** Hats and glasses from the Blender pipeline (art/blender/assets/wearables.py), when imported. */
UStaticMesh* Wearable(const TCHAR* Name)
{
	return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/ShortStack/Meshes/%s/%s.%s"), Name, Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
}

/** The groom's melanin and redness for the creator's hair colors (MetaHuman hair materials). */
void HairShade(int32 Index, float& Melanin, float& Redness)
{
	static const float Table[8][2] = {{0.95f, 0.1f}, {0.75f, 0.2f}, {0.55f, 0.28f}, {0.45f, 0.7f}, {0.3f, 0.95f}, {0.12f, 0.3f}, {0.03f, 0.05f}, {0.2f, 0.0f}};
	const int32 I = FMath::Clamp(Index, 0, 7);
	Melanin = Table[I][0];
	Redness = Table[I][1];
}
} // namespace ShortStackCharacterDetail

using namespace ShortStackCharacterDetail;

AShortStackCharacter::AShortStackCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(32.0f, 89.0f);
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.0f, 520.0f, 0.0f);
	Move->MaxWalkSpeed = WalkSpeed;
	Move->MaxAcceleration = 1100.0f;
	Move->BrakingDecelerationWalking = 1500.0f;
	Move->GroundFriction = 8.0f;
	Move->bCanWalkOffLedges = true;

	// MetaHuman bodies face +Y: turn the mesh to the capsule's +X, feet on the capsule's floor.
	USkeletalMeshComponent* Body = GetMesh();
	Body->SetRelativeLocationAndRotation(FVector(0.0, 0.0, -89.0), FRotator(0.0f, -90.0f, 0.0f));
	Body->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	Body->SetAnimInstanceClass(UStreetBodyAnim::StaticClass());
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	Face = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Face"));
	Face->SetupAttachment(Body);
	Face->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	Face->SetAnimInstanceClass(UBackRoomFaceAnim::StaticClass());
	Face->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Face->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	Boom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Boom"));
	Boom->SetupAttachment(RootComponent);
	Boom->SetRelativeLocation(FVector(0.0, 0.0, 58.0));
	Boom->TargetArmLength = ArmLength;
	Boom->SocketOffset = ShoulderOffset;
	Boom->bUsePawnControlRotation = true;
	Boom->bDoCollisionTest = true;
	Boom->ProbeSize = 14.0f;
	Boom->bEnableCameraLag = true;
	Boom->CameraLagSpeed = 16.0f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Boom, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;
	Camera->SetFieldOfView(ThirdPersonFov);
}

void AShortStackCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (!bHasLook)
	{
		// Nobody dressed us: the archetype body, so there's someone to walk.
		ApplyLook(ss::hero::Character());
	}
	SetFirstPerson(bFirstPerson);
	Blend = bFirstPerson ? 1.0f : 0.0f;
}

// ------------------------------------------------------------------ dressing

void AShortStackCharacter::ClearWearables()
{
	for (USceneComponent* W : Wearables)
	{
		if (W)
		{
			W->DestroyComponent();
		}
	}
	Wearables.Reset();
}

void AShortStackCharacter::BuildFromMetaHuman(UClass* Blueprint, const FLinearColor& Dye)
{
	UBlueprintGeneratedClass* Bp = Cast<UBlueprintGeneratedClass>(Blueprint);
	if (!Bp || !Bp->SimpleConstructionScript)
	{
		return;
	}
	ClearWearables();
	// As the Back Room's players: the Blueprint's Body and Face become ours, everything else visible is copied
	// onto ours and attached the same way. Here the hair comes too: the player sees their own head in third person.
	TMap<FName, USceneComponent*> Ours;
	const TArray<USCS_Node*>& Nodes = Bp->SimpleConstructionScript->GetAllNodes();
	for (USCS_Node* Node : Nodes)
	{
		const USkeletalMeshComponent* Template = Node ? Cast<USkeletalMeshComponent>(Node->ComponentTemplate) : nullptr;
		if (!Template)
		{
			continue;
		}
		const FName Name = Node->GetVariableName();
		USkeletalMeshComponent* Mine = Name == TEXT("Body") ? GetMesh() : (Name == TEXT("Face") ? Face.Get() : nullptr);
		if (Mine)
		{
			Mine->SetSkeletalMeshAsset(Template->GetSkeletalMeshAsset());
			CopyOverrides(Template, Mine);
			Ours.Add(Name, Mine);
		}
	}
	float Melanin = 0.5f;
	float Redness = 0.2f;
	HairShade(LookNow.HairColor, Melanin, Redness);
	for (USCS_Node* Node : Nodes)
	{
		const UPrimitiveComponent* Template = Node ? Cast<UPrimitiveComponent>(Node->ComponentTemplate) : nullptr;
		if (!Template || Ours.Contains(Node->GetVariableName()))
		{
			continue;
		}
		const UGroomComponent* Groom = Cast<UGroomComponent>(Template);
		if (Groom && !Groom->GroomAsset)
		{
			continue;
		}
		const FName Name = MakeUniqueObjectName(this, Template->GetClass(), Node->GetVariableName());
		UPrimitiveComponent* Copy = NewObject<UPrimitiveComponent>(this, Template->GetClass(), Name, RF_Transient, const_cast<UPrimitiveComponent*>(Template));
		const USCS_Node* ParentNode = Bp->SimpleConstructionScript->FindParentNode(Node);
		USceneComponent* const* Parent = ParentNode ? Ours.Find(ParentNode->GetVariableName()) : nullptr;
		Copy->SetupAttachment(Parent ? *Parent : GetMesh(), Node->AttachToName);
		Copy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (USkinnedMeshComponent* Skinned = Cast<USkinnedMeshComponent>(Copy))
		{
			// Clothes ride the body's pose, dyed the jacket color the player chose.
			Skinned->SetLeaderPoseComponent(GetMesh());
			for (int32 Slot = 0; Slot < Skinned->GetNumMaterials(); ++Slot)
			{
				if (UMaterialInterface* Material = Skinned->GetMaterial(Slot))
				{
					UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Material, Skinned);
					Mid->SetVectorParameterValue(TEXT("diffuse_color_1"), Dye);
					Mid->SetVectorParameterValue(TEXT("diffuse_color_2"), Dye * 0.8f);
					Skinned->SetMaterial(Slot, Mid);
				}
			}
		}
		else if (UGroomComponent* Hair = Cast<UGroomComponent>(Copy))
		{
			// The hair color from the creator, as far as the groom's material takes it.
			for (int32 Slot = 0; Slot < Hair->GetNumMaterials(); ++Slot)
			{
				if (UMaterialInterface* Material = Hair->GetMaterial(Slot))
				{
					UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Material, Hair);
					Mid->SetScalarParameterValue(TEXT("Melanin"), Melanin);
					Mid->SetScalarParameterValue(TEXT("Redness"), Redness);
					Hair->SetMaterial(Slot, Mid);
				}
			}
		}
		Copy->RegisterComponent();
		Ours.Add(Node->GetVariableName(), Copy);
		Wearables.Add(Copy);
	}
}

void AShortStackCharacter::ApplyLook(const ss::hero::Character& Who)
{
	LookNow = Who.Appearance;
	bHasLook = true;
	const ss::hero::Look& L = LookNow;
	// The hero MetaHuman nearest the look: body type, then skin tone in three bands.
	const int32 Band = L.Skin <= 2 ? 0 : (L.Skin <= 5 ? 1 : 2);
	UClass* Blueprint = CastClass(FString::Printf(TEXT("Hero%c%d"), L.Body == 1 ? TEXT('B') : TEXT('A'), Band));
	if (!Blueprint)
	{
		Blueprint = CastClass(TEXT("Hero"));
	}
	const FLinearColor Dye = SrgbOf(ss::hero::OutfitTone(L.OutfitColor));
	if (Blueprint)
	{
		BuildFromMetaHuman(Blueprint, Dye);
	}
	else
	{
		ClearWearables();
		if (USkeletalMesh* BodyAsset = LoadObject<USkeletalMesh>(nullptr, HeroArchetypeBody, nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			GetMesh()->SetSkeletalMeshAsset(BodyAsset);
		}
		if (USkeletalMesh* FaceAsset = LoadObject<USkeletalMesh>(nullptr, HeroArchetypeFace, nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			Face->SetSkeletalMeshAsset(FaceAsset);
		}
	}
	Face->AddTickPrerequisiteComponent(GetMesh());
	if (UBackRoomFaceAnim* FaceAnim = Cast<UBackRoomFaceAnim>(Face->GetAnimInstance()))
	{
		FaceAnim->Body = GetMesh();
	}

	// A hat and glasses, from the Blender props when they're imported.
	static const TCHAR* const Hats[5] = {nullptr, TEXT("SM_Hat_Beanie"), TEXT("SM_Hat_Cap"), TEXT("SM_Hat_Cap"), TEXT("SM_Hat_Bucket")};
	static const TCHAR* const Specs[5] = {nullptr, TEXT("SM_Glasses_Round"), TEXT("SM_Glasses_Square"), TEXT("SM_Glasses_Wire"), TEXT("SM_Glasses_Shades")};
	auto Accessory = [this](TObjectPtr<UStaticMeshComponent>& Slot, const TCHAR* MeshName, const FLinearColor& Tint) {
		UStaticMesh* Mesh = MeshName ? Wearable(MeshName) : nullptr;
		if (!Mesh)
		{
			if (Slot)
			{
				Slot->SetVisibility(false);
			}
			return;
		}
		if (!Slot)
		{
			Slot = NewObject<UStaticMeshComponent>(this);
			Slot->SetupAttachment(RootComponent);
			Slot->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Slot->RegisterComponent();
		}
		Slot->SetStaticMesh(Mesh);
		Slot->SetVisibility(true);
		if (UMaterialInterface* Material = Slot->GetMaterial(0))
		{
			UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Material, Slot);
			Mid->SetVectorParameterValue(TEXT("BaseColor"), Tint);
			Slot->SetMaterial(0, Mid);
		}
	};
	static const int32 HatPartner[8] = {7, 4, 5, 0, 1, 3, 0, 5}; // as the portrait: a hat that goes with the jacket
	Accessory(HatMesh, Hats[FMath::Clamp(L.Hat, 0, 4)], SrgbOf(ss::hero::OutfitTone(HatPartner[FMath::Clamp(L.OutfitColor, 0, 7)])));
	Accessory(GlassesMesh, Specs[FMath::Clamp(L.Glasses, 0, 4)], SrgbOf(L.Glasses == 3 ? 0xc9a24d : 0x141416));

	FitToHeight(static_cast<float>(L.Height));
	SetHeadVisible(!bFirstPerson);
}

void AShortStackCharacter::ApplyCast(const TCHAR* CastName, const FLinearColor& Shirt)
{
	bHasLook = true;
	if (UClass* Blueprint = CastClass(CastName))
	{
		BuildFromMetaHuman(Blueprint, Shirt);
	}
	else if (USkeletalMesh* BodyAsset = LoadObject<USkeletalMesh>(nullptr, HeroArchetypeBody, nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		GetMesh()->SetSkeletalMeshAsset(BodyAsset);
	}
	Face->AddTickPrerequisiteComponent(GetMesh());
	if (UBackRoomFaceAnim* FaceAnim = Cast<UBackRoomFaceAnim>(Face->GetAnimInstance()))
	{
		FaceAnim->Body = GetMesh();
	}
	FitToHeight(175.0f);
}

void AShortStackCharacter::FitToHeight(float HeightCm)
{
	USkeletalMesh* Asset = GetMesh()->GetSkeletalMeshAsset();
	if (!Asset)
	{
		return;
	}
	// The crown: the top of the face mesh (it rides the body at identity), or a skull's height over the head bone.
	float Crown = 0.0f;
	if (USkeletalMesh* FaceAsset = Face->GetSkeletalMeshAsset())
	{
		const FBoxSphereBounds B = FaceAsset->GetBounds();
		Crown = static_cast<float>(B.Origin.Z + B.BoxExtent.Z);
	}
	if (Crown < 100.0f)
	{
		const FReferenceSkeleton& Ref = Asset->GetRefSkeleton();
		int32 Bone = Ref.FindBoneIndex(TEXT("head"));
		FTransform At = FTransform::Identity;
		while (Bone != INDEX_NONE)
		{
			At = At * Ref.GetRefBonePose()[Bone];
			Bone = Ref.GetParentIndex(Bone);
		}
		Crown = static_cast<float>(At.GetTranslation().Z) + 18.0f;
	}
	if (Crown < 100.0f)
	{
		Crown = 175.0f;
	}
	const float K = FMath::Clamp(HeightCm / Crown, 0.85f, 1.18f);
	const float Width = LookNow.Build == 0 ? 0.95f : LookNow.Build == 2 ? 1.04f : LookNow.Build == 3 ? 1.12f : 1.0f;
	MeshScale = K;
	const float Half = HeightCm * 0.5f;
	GetCapsuleComponent()->SetCapsuleSize(30.0f * Width, Half);
	GetMesh()->SetRelativeLocation(FVector(0.0, 0.0, -Half));
	GetMesh()->SetRelativeScale3D(FVector(K * Width, K * Width, K));
	Boom->SetRelativeLocation(FVector(0.0, 0.0, Half * 0.62f));
}

// ------------------------------------------------------------------ view

void AShortStackCharacter::SetHeadVisible(bool bVisible)
{
	bHeadHidden = !bVisible;
	// Not hidden: kept from the player's own view only, so its shadow and the hair binding stay intact.
	Face->SetOwnerNoSee(!bVisible);
	for (USceneComponent* W : Wearables)
	{
		if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(W))
		{
			if (W->GetAttachParent() == Face)
			{
				Prim->SetOwnerNoSee(!bVisible);
			}
		}
	}
	if (HatMesh)
	{
		HatMesh->SetOwnerNoSee(!bVisible);
	}
	if (GlassesMesh)
	{
		GlassesMesh->SetOwnerNoSee(!bVisible);
	}
}

void AShortStackCharacter::SetFirstPerson(bool bFirst)
{
	bFirstPerson = bFirst;
	// First person turns the body with the view; third person turns it toward where it walks.
	bUseControllerRotationYaw = bFirst;
	GetCharacterMovement()->bOrientRotationToMovement = !bFirst;
}

FVector AShortStackCharacter::EyeLocal() const
{
	const USkeletalMeshComponent* Body = GetMesh();
	if (Body && Body->GetSkeletalMeshAsset() && Body->DoesSocketExist(TEXT("head")))
	{
		// The head bone sits at the base of the skull: the eyes are a little above it and forward.
		const FVector Head = GetActorTransform().InverseTransformPosition(Body->GetSocketLocation(TEXT("head")));
		return Head + FVector(9.0, 0.0, 8.0) * MeshScale;
	}
	return FVector(8.0, 0.0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.86f);
}

FVector AShortStackCharacter::EyeLocation() const
{
	return GetActorTransform().TransformPosition(EyeLocal());
}

FVector AShortStackCharacter::LookDirection() const
{
	return GetControlRotation().Vector();
}

void AShortStackCharacter::UpdateCamera(float Dt)
{
	const float Target = bFirstPerson ? 1.0f : 0.0f;
	const float Step = Dt / FMath::Max(0.05f, SwitchSeconds);
	Blend = Blend < Target ? FMath::Min(Target, Blend + Step) : FMath::Max(Target, Blend - Step);
	const float B = Blend * Blend * (3.0f - 2.0f * Blend);
	const FVector Pivot(0.0, 0.0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.62f);
	Boom->SetRelativeLocation(FMath::Lerp(Pivot, EyeLocal(), static_cast<double>(B)));
	Boom->TargetArmLength = FMath::Lerp(ArmLength, 0.0f, B);
	Boom->SocketOffset = FMath::Lerp(ShoulderOffset, FVector::ZeroVector, static_cast<double>(B));
	Boom->bDoCollisionTest = B < 0.5f;
	Boom->bEnableCameraLag = B < 0.5f;
	Camera->SetFieldOfView(FMath::Lerp(ThirdPersonFov, FirstPersonFov, B));
	const bool bShowHead = B < 0.6f;
	if (bShowHead == bHeadHidden)
	{
		SetHeadVisible(bShowHead);
	}
}

void AShortStackCharacter::UpdateAccessories()
{
	USkeletalMeshComponent* Body = GetMesh();
	if (!Body->GetSkeletalMeshAsset() || !Body->DoesSocketExist(TEXT("head")))
	{
		return;
	}
	// Placed from the head bone each frame, square to the body (the props are modeled facing +X):
	// the hat's origin is the crown, the glasses' the bridge of the nose.
	const FVector Head = Body->GetSocketLocation(TEXT("head"));
	const FRotator Facing(0.0f, GetActorRotation().Yaw, 0.0f);
	const FVector Fwd = Facing.Vector();
	const FVector Up = FVector::UpVector;
	if (HatMesh && HatMesh->IsVisible())
	{
		const bool bBackwards = LookNow.Hat == 3;
		HatMesh->SetWorldLocationAndRotation(Head + Up * (19.0 * MeshScale) + Fwd * (1.5 * MeshScale), Facing + FRotator(0.0f, bBackwards ? 180.0f : 0.0f, 0.0f));
		HatMesh->SetWorldScale3D(FVector(MeshScale));
	}
	if (GlassesMesh && GlassesMesh->IsVisible())
	{
		GlassesMesh->SetWorldLocationAndRotation(Head + Up * (8.5 * MeshScale) + Fwd * (10.5 * MeshScale), Facing);
		GlassesMesh->SetWorldScale3D(FVector(MeshScale));
	}
}

// ------------------------------------------------------------------ moving

void AShortStackCharacter::Drive(float Forward, float Right, bool bRun)
{
	bRunning = bRun;
	GetCharacterMovement()->MaxWalkSpeed = bRun ? RunSpeed : WalkSpeed;
	const FRotator Yaw(0.0f, GetControlRotation().Yaw, 0.0f);
	const FRotationMatrix M(Yaw);
	if (!FMath::IsNearlyZero(Forward))
	{
		AddMovementInput(M.GetUnitAxis(EAxis::X), Forward);
	}
	if (!FMath::IsNearlyZero(Right))
	{
		AddMovementInput(M.GetUnitAxis(EAxis::Y), Right);
	}
}

void AShortStackCharacter::Look(float DeltaYaw, float DeltaPitch)
{
	if (AController* C = GetController())
	{
		FRotator R = C->GetControlRotation();
		R.Yaw += DeltaYaw;
		R.Pitch = FMath::ClampAngle(R.Pitch + static_cast<double>(DeltaPitch), -70.0, 65.0);
		C->SetControlRotation(R);
	}
}

bool AShortStackCharacter::TakeFootstep(float& OutVolume)
{
	if (!bStepPending)
	{
		return false;
	}
	bStepPending = false;
	OutVolume = StepVolume;
	return true;
}

void AShortStackCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Dt = FMath::Min(DeltaSeconds, 0.05f);
	Clock += Dt;
	const float Speed = static_cast<float>(GetVelocity().Size2D());
	const float Run = FMath::Clamp((Speed - WalkSpeed) / FMath::Max(1.0f, RunSpeed - WalkSpeed), 0.0f, 1.0f);
	// One gait cycle (two steps) covers about 1.4 m walking and 2.6 m running, longer for taller people.
	const float Cycle = FMath::Lerp(140.0f, 260.0f, Run) * MeshScale;
	const float Before = Phase;
	Phase = FMath::Fmod(Phase + 2.0f * PI * Speed / Cycle * Dt, 2.0f * PI);
	if (Speed > 30.0f && FMath::FloorToInt((Phase - PI * 0.5f) / PI) != FMath::FloorToInt((Before - PI * 0.5f) / PI))
	{
		// A heel strikes a quarter cycle after each leg passes under the body.
		bStepPending = true;
		StepVolume = FMath::Lerp(0.25f, 0.6f, Run);
	}
	if (Speed < 5.0f)
	{
		// Standing: settle back to the neutral stance.
		Phase = FMath::FInterpTo(Phase, Phase < PI ? 0.0f : 2.0f * PI, Dt, 4.0f);
	}
	const float Yaw = static_cast<float>(GetActorRotation().Yaw);
	const float TurnRate = FMath::FindDeltaAngleDegrees(LastYaw, Yaw) / FMath::Max(Dt, 0.001f);
	LastYaw = Yaw;
	Bank = FMath::FInterpTo(Bank, FMath::Clamp(-TurnRate * Speed / 40000.0f, -8.0f, 8.0f), Dt, 6.0f);

	if (UStreetBodyAnim* Anim = Cast<UStreetBodyAnim>(GetMesh()->GetAnimInstance()))
	{
		FStreetBodyPose& P = Anim->Pose;
		P.Speed = Speed;
		P.Phase = Phase;
		P.Run = Run;
		P.Bank = Bank;
		P.Time = Clock;
		const float SipT = Clock - SipAt;
		P.Sip = SipT < 1.8f ? FMath::Sin(FMath::Clamp(SipT / 1.8f, 0.0f, 1.0f) * PI) : 0.0f;
		if (GetController())
		{
			const FRotator View = GetControlRotation();
			P.LookYaw = FMath::FindDeltaAngleDegrees(Yaw, static_cast<float>(View.Yaw));
			P.LookPitch = static_cast<float>(FRotator::NormalizeAxis(View.Pitch));
		}
	}
	if (IsPlayerControlled())
	{
		UpdateCamera(Dt);
	}
	UpdateAccessories();
}
