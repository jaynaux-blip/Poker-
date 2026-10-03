#include "BackRoomPlayer.h"

#include "BackRoomCard.h"
#include "BackRoomChips.h"
#include "BackRoomPlayerShared.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GroomComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

using namespace BackRoomPlayerDetail;

namespace BackRoomPlayerCoreDetail
{
const TCHAR* ArchetypeBody = TEXT("/MetaHumanCharacter/Body/IdentityTemplate/SKM_Body.SKM_Body");
const TCHAR* ArchetypeFace = TEXT("/MetaHumanCharacter/Face/SKM_Face.SKM_Face");
const TCHAR* FoldingChair = TEXT("/Game/ShortStack/Meshes/SM_FoldingChair/SM_FoldingChair.SM_FoldingChair");

void CopyMaterials(const UMeshComponent* From, UMeshComponent* To)
{
	for (int32 I = 0; I < From->OverrideMaterials.Num(); ++I)
	{
		if (From->OverrideMaterials[I])
		{
			To->SetMaterial(I, From->OverrideMaterials[I]);
		}
	}
}
} // namespace BackRoomPlayerCoreDetail

using namespace BackRoomPlayerCoreDetail;

ABackRoomPlayer::ABackRoomPlayer()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	// MetaHuman bodies face +Y: turn the mesh so it faces the table (the actor's +X), pelvis over the seat.
	Body->SetRelativeLocationAndRotation(FVector(-SeatDepth, 0.0, 0.0), FRotator(0.0f, -90.0f, 0.0f));
	Body->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	Body->SetAnimInstanceClass(UBackRoomBodyAnim::StaticClass());
	Body->SetUpdateAnimationInEditor(true);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Seated players never leave the camera's view: keep their bones moving even when off screen.
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	Face = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Face"));
	Face->SetupAttachment(Body);
	Face->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	Face->SetAnimInstanceClass(UBackRoomFaceAnim::StaticClass());
	Face->SetUpdateAnimationInEditor(true);
	Face->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Face->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	ChairSeat = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChairSeat"));
	ChairSeat->SetupAttachment(Root);
	ChairSeat->SetStaticMesh(Cube.Object);
	ChairSeat->SetRelativeLocation(FVector(-SeatDepth, 0.0, SeatHeight - 2.0));
	ChairSeat->SetRelativeScale3D(FVector(0.42, 0.44, 0.04));
	ChairSeat->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ChairBack = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChairBack"));
	ChairBack->SetupAttachment(Root);
	ChairBack->SetStaticMesh(Cube.Object);
	ChairBack->SetRelativeLocationAndRotation(FVector(-SeatDepth - 23.0, 0.0, SeatHeight + 26.0), FRotator(-8.0f, 0.0f, 0.0f));
	ChairBack->SetRelativeScale3D(FVector(0.03, 0.42, 0.4));
	ChairBack->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABackRoomPlayer::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
}

void ABackRoomPlayer::Build()
{
	if (!MetaHumanClass.IsNull())
	{
		BuildFromMetaHuman();
	}
	else
	{
		USkeletalMesh* BodyAsset = BodyMesh.IsNull() ? LoadObject<USkeletalMesh>(nullptr, ArchetypeBody) : BodyMesh.LoadSynchronous();
		USkeletalMesh* FaceAsset = FaceMesh.IsNull() ? LoadObject<USkeletalMesh>(nullptr, ArchetypeFace) : FaceMesh.LoadSynchronous();
		if (BodyAsset && Body->GetSkeletalMeshAsset() != BodyAsset)
		{
			Body->SetSkeletalMeshAsset(BodyAsset);
		}
		if (FaceAsset && Face->GetSkeletalMeshAsset() != FaceAsset)
		{
			Face->SetSkeletalMeshAsset(FaceAsset);
		}
	}
	Face->AddTickPrerequisiteComponent(Body);
	if (UBackRoomFaceAnim* FaceAnim = Cast<UBackRoomFaceAnim>(Face->GetAnimInstance()))
	{
		FaceAnim->Body = Body;
	}

	// You don't see your own face: it still casts its shadow, and so does the hair.
	const bool bHero = SeatRole == EBackRoomRole::Hero;
	Face->SetHiddenInGame(bHero);
	Face->bCastHiddenShadow = true;
	// Only its shadow shows: a coarse level of detail does for that (0 lets the screen size choose).
	Face->SetForcedLOD(bHero ? 4 : 0);
	for (USceneComponent* W : Wearables)
	{
		if (W && W->GetAttachParent() == Face)
		{
			W->SetHiddenInGame(bHero);
			if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(W))
			{
				Prim->bCastHiddenShadow = true;
			}
		}
	}
	// A folding chair (art/blender/assets/folding_chair.py) once it's imported; boxes until then, which
	// the hero would only see as boxes, so they're hidden for the hero.
	if (UStaticMesh* Chair = LoadObject<UStaticMesh>(nullptr, FoldingChair, nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		ChairSeat->SetStaticMesh(Chair);
		ChairSeat->SetRelativeLocationAndRotation(FVector(-SeatDepth + 0.5, 0.0, 0.0), FRotator::ZeroRotator);
		ChairSeat->SetRelativeScale3D(FVector(1.0));
		// Extras sit on the room's own chairs.
		ChairSeat->SetVisibility(SeatRole != EBackRoomRole::Extra);
		ChairBack->SetVisibility(false);
	}
	else
	{
		ChairSeat->SetVisibility(SeatRole != EBackRoomRole::Hero);
		ChairBack->SetVisibility(SeatRole != EBackRoomRole::Hero);
	}

	Rng.Initialize(Persona.Seed * 7919 + 17);
	for (int32 Side = 0; Side < 2; ++Side)
	{
		HandNow[Side] = StepFrom[Side] = RestPose(Side);
		HandRate[Side] = Wiggle[Side] = Still();
		HandGoal[Side] = HandNow[Side].Pos;
		Steps[Side].Reset();
	}
	EyeAt = HeadAt = GazeTarget = HeroEyes;
	HeroLook = HeroEyes;
	Lean = LeanTarget = 0.45f + 0.35f * Persona.Posture;
	NextChat = Rng.FRandRange(15.0f, 40.0f);
}

void ABackRoomPlayer::BuildFromMetaHuman()
{
	UBlueprintGeneratedClass* Bp = Cast<UBlueprintGeneratedClass>(MetaHumanClass.LoadSynchronous());
	if (!Bp || !Bp->SimpleConstructionScript)
	{
		return;
	}
	// Built already (construction reruns whenever the actor moves in the editor).
	if (Wearables.Num() > 0 && Body->GetSkeletalMeshAsset() && Wearables[0] && Wearables[0]->GetOuter() == this && WornClass == Bp)
	{
		return;
	}
	for (USceneComponent* W : Wearables)
	{
		if (W)
		{
			W->DestroyComponent();
		}
	}
	Wearables.Reset();
	WornClass = Bp;

	// The Blueprint's Body and Face become ours (with our anim instances); everything else visible it
	// holds (hair, brows, lashes, beard, peach fuzz, clothes) is copied onto ours, attached the same way.
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
		USkeletalMeshComponent* Mine = Name == TEXT("Body") ? Body.Get() : (Name == TEXT("Face") ? Face.Get() : nullptr);
		if (Mine)
		{
			Mine->SetSkeletalMeshAsset(Template->GetSkeletalMeshAsset());
			CopyMaterials(Template, Mine);
			Ours.Add(Name, Mine);
		}
	}
	for (USCS_Node* Node : Nodes)
	{
		const UPrimitiveComponent* Template = Node ? Cast<UPrimitiveComponent>(Node->ComponentTemplate) : nullptr;
		if (!Template || Ours.Contains(Node->GetVariableName()))
		{
			continue;
		}
		// Empty slots (a beard on a clean-shaven face) cost nothing to skip. Nor does the hero's hair:
		// you never see it, and the hair system can't bind to a face that isn't drawn (its skinned
		// sections aren't cached, which trips HairStrandsMeshProjection's checks).
		const UGroomComponent* Groom = Cast<UGroomComponent>(Template);
		if (Groom && (!Groom->GroomAsset || SeatRole == EBackRoomRole::Hero))
		{
			continue;
		}
		const FName Name = MakeUniqueObjectName(this, Template->GetClass(), Node->GetVariableName());
		UPrimitiveComponent* Copy = NewObject<UPrimitiveComponent>(this, Template->GetClass(), Name, RF_Transient, const_cast<UPrimitiveComponent*>(Template));
		// The parent is the node above this one in the Blueprint's tree (ParentComponentOrVariableName
		// only names parents inherited from a parent class): grooms hang off the face, clothes the body.
		const USCS_Node* ParentNode = Bp->SimpleConstructionScript->FindParentNode(Node);
		USceneComponent* const* Parent = ParentNode ? Ours.Find(ParentNode->GetVariableName()) : nullptr;
		Copy->SetupAttachment(Parent ? *Parent : Body.Get(), Node->AttachToName);
		Copy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (USkinnedMeshComponent* Skinned = Cast<USkinnedMeshComponent>(Copy))
		{
			// Clothes ride the body's pose, and are dyed the persona's colors.
			Skinned->SetLeaderPoseComponent(Body);
			for (int32 Slot = 0; Slot < Skinned->GetNumMaterials(); ++Slot)
			{
				UMaterialInterface* Material = Skinned->GetMaterial(Slot);
				if (!Material)
				{
					continue;
				}
				const FString MaterialName = Material->GetName();
				const bool bShirt = MaterialName.Contains(TEXT("Shirt"));
				const FLinearColor Dye = bShirt ? Persona.Shirt : FLinearColor(0.035f, 0.037f, 0.045f);
				UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Material, Skinned);
				Mid->SetVectorParameterValue(TEXT("diffuse_color_1"), Dye);
				Mid->SetVectorParameterValue(TEXT("diffuse_color_2"), Dye);
				Skinned->SetMaterial(Slot, Mid);
			}
		}
		Copy->RegisterComponent();
		Ours.Add(Node->GetVariableName(), Copy);
		Wearables.Add(Copy);
	}
}

FVector ABackRoomPlayer::ToBody(const FVector& World) const
{
	return Body->GetComponentTransform().InverseTransformPosition(World);
}

FVector ABackRoomPlayer::ToBodyDir(const FVector& World) const
{
	return Body->GetComponentTransform().InverseTransformVectorNoScale(World);
}

FVector ABackRoomPlayer::ToWorld(const FVector& BodyPos) const
{
	return Body->GetComponentTransform().TransformPosition(BodyPos);
}

FVector ABackRoomPlayer::GetEyes() const
{
	// Between the eyes: in front of the head joint.
	if (Body->GetSkeletalMeshAsset() && Body->GetBoneIndex(TEXT("head")) != INDEX_NONE)
	{
		const FTransform Head = Body->GetSocketTransform(TEXT("head"));
		return Head.GetLocation() + Body->GetComponentTransform().TransformVectorNoScale(FVector(0.0, 9.0, 7.0));
	}
	return ToWorld(FVector(0.0, 10.0, 118.0));
}

void ABackRoomPlayer::SetTestExpression(FName Control, float Value)
{
	TestCurves.Add(FName(*(TEXT("CTRL_expressions_") + Control.ToString())), Value);
}

void ABackRoomPlayer::ClearTestExpressions()
{
	TestCurves.Reset();
}

bool ABackRoomPlayer::IsBusy() const
{
	return Steps[0].Num() > 0 || Steps[1].Num() > 0;
}

// ------------------------------------------------------------------ behavior

void ABackRoomPlayer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Dt = FMath::Min(DeltaSeconds, 0.1f);
	Time += Dt;
	UpdateMood(Dt);
	UpdateTells(Dt);
	UpdateHands(Dt);
	UpdateBody(Dt);
	UpdateFace(Dt);
}

void ABackRoomPlayer::UpdateMood(float Dt)
{
	const UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld() || SeatRole == EBackRoomRole::Hero)
	{
		// In the editor the mood is what the details panel says; the hero's face is hidden.
		return;
	}
	// What the moment asks for: worry when the hand is weak and there's a price to pay or a bluff to
	// sell, a thrill under a strong hand, gloom after a loss, all fading back to temperament.
	const float Base = 0.08f + 0.22f * Persona.Nervousness;
	float WantStress = Base;
	if (bInHand)
	{
		WantStress += bBluffing ? 0.55f : 0.0f;
		WantStress += bThinking ? 0.3f * (1.0f - Strength) : 0.0f;
		WantStress += bHeroThinking ? 0.15f + (bBluffing ? 0.2f : 0.0f) : 0.0f;
	}
	StressGoal = Ease(StressGoal, FMath::Max(WantStress, StressGoal * 0.98f), 0.6f, Dt);
	StressGoal = Ease(StressGoal, WantStress, 0.25f, Dt);
	ThrillGoal = Ease(ThrillGoal, bInHand ? FMath::Max(0.0f, Strength - 0.62f) * 2.2f : 0.0f, 0.2f, Dt);
	GloomGoal = Ease(GloomGoal, 0.0f, 0.05f, Dt);

	Stress = Ease(Stress, StressGoal, 1.4f, Dt);
	Thrill = Ease(Thrill, ThrillGoal, 1.0f, Dt);
	Gloom = Ease(Gloom, GloomGoal, 0.8f, Dt);

	// A player in a hand works at a blank face; out of it, they let go.
	const float Mask = bInHand ? 0.45f : 1.0f;
	Valence = FMath::Clamp(Mask * (0.9f * Thrill - 0.35f * Stress - 0.9f * Gloom), -1.0f, 1.0f);
	Arousal = FMath::Clamp(0.15f + 0.6f * Stress + 0.5f * Thrill + 0.2f * Persona.Nervousness, 0.0f, 1.0f);
	// Bluffers put on confidence; worry otherwise shrinks a body.
	Dominance = FMath::Clamp(0.35f + 0.25f * Persona.Posture + 0.35f * Thrill - (bBluffing ? -0.1f : 0.4f) * Stress - 0.3f * Gloom, 0.0f, 1.0f);
}

void ABackRoomPlayer::UpdateBody(float Dt)
{
	UBackRoomBodyAnim* Anim = Cast<UBackRoomBodyAnim>(Body->GetAnimInstance());
	if (!Anim)
	{
		return;
	}
	// Breathing: faster and deeper when aroused, all but stopped in a freeze, a big one in a sigh.
	const float Rate = 11.0f + 9.0f * Arousal + 4.0f * Persona.Nervousness; // breaths per minute
	BreathPhase += Dt * Rate / 60.0f * UE_TWO_PI * (1.0f - 0.75f * Stillness);
	float Breath = FMath::Sin(BreathPhase) * (0.6f + 0.6f * Arousal) * (1.0f - 0.7f * Stillness);
	float SighLean = 0.0f;
	if (SighT >= 0.0f)
	{
		// In for a second, out with a slump.
		SighT += Dt / 2.0f;
		const float In = FMath::Clamp(SighT / 0.45f, 0.0f, 1.0f);
		const float Out = FMath::Clamp((SighT - 0.45f) / 0.55f, 0.0f, 1.0f);
		Breath = FMath::Lerp(Breath, 1.6f * In * (1.0f - Out) - 0.8f * Out * (1.0f - Out) * 4.0f, FMath::Sin(FMath::Min(SighT, 1.0f) * UE_PI));
		SighLean = -0.25f * Out * (1.0f - Out) * 4.0f;
		if (SighT >= 1.0f)
		{
			SighT = -1.0f;
		}
	}

	// Posture: settles, then shifts every so often; leans in when engaged or confident. Frozen: doesn't.
	NextShift -= Dt * (1.0f - Stillness);
	if (NextShift <= 0.0f)
	{
		LeanTarget = FMath::Clamp(0.45f + 0.35f * Persona.Posture + 0.2f * Dominance * Arousal + Rng.FRandRange(-0.12f, 0.12f), 0.0f, 1.0f);
		NextShift = Rng.FRandRange(7.0f, 22.0f);
	}
	if (SeatRole == EBackRoomRole::Hero)
	{
		// You lean in over your cards to look at them.
		LeanTarget = bHeroPeek ? 1.0f : 0.5f;
	}
	// The idle habit carries the body with it: forward onto a fist, back into the chair, onto an elbow.
	float WantLean = LeanTarget;
	float WantTwist = 0.0f;
	const bool bHabit = SeatRole == EBackRoomRole::Player && !IsBusy();
	if (bHabit)
	{
		switch (HandMode)
		{
		case 3: WantLean = FMath::Max(LeanTarget, 0.78f); break;
		case 4: WantLean = FMath::Max(LeanTarget, 0.5f); break;
		case 5: WantLean = 0.06f; break;
		case 6:
			WantLean = FMath::Max(LeanTarget, 0.6f);
			WantTwist = HabitSide == 0 ? -7.0f : 7.0f;
			break;
		default: break;
		}
	}
	Lean = Ease(Lean, WantLean, (SeatRole == EBackRoomRole::Hero ? 4.0f : 0.8f) * (1.0f - 0.9f * Stillness), Dt);
	Twist = Ease(Twist, WantTwist, 1.2f * (1.0f - 0.9f * Stillness), Dt);

	// Gaze: a new target every second or few; the eyes jump, the head follows.
	GazeLeft -= Dt * (1.0f - 0.85f * Stillness);
	AttentionLeft -= Dt;
	GazeHoldLeft -= Dt;
	if (GazeLeft <= 0.0f)
	{
		const FTransform& T = GetActorTransform();
		const FVector Cards = Hole.Num() > 0 && Hole[0] ? Hole[0]->GetActorLocation() : T.TransformPosition(FVector(RailGap + 27.0, 4.0, 76.0));
		const FVector Chips = StackPile ? StackPile->GetTop() : T.TransformPosition(FVector(RailGap + 22.0, 18.0, 78.0));
		const FVector Away = T.TransformPosition(FVector(60.0, Rng.FRandRange(-80.0f, 80.0f), 20.0));
		struct FChoice { FVector At; float W; };
		TArray<FChoice, TInlineAllocator<12>> Choices;
		if (SeatRole == EBackRoomRole::Dealer)
		{
			// The dealer watches the action and the pot, and keeps an eye on everyone.
			Choices = {{HeroEyes, 0.35f}, {PotAt, 0.35f}};
		}
		else
		{
			Choices = {{HeroEyes, 0.3f + 0.3f * Dominance}, {PotAt, 0.25f}, {Cards, bInHand ? 0.12f : 0.03f}, {Chips, 0.1f}, {DealerAt, 0.1f},
				{Away, 0.12f * Persona.Nervousness + (bInHand ? 0.0f : 0.15f)}};
		}
		for (const FVector& Other : OthersAt)
		{
			Choices.Add({Other, 0.12f});
		}
		if (AttentionLeft > 0.0f)
		{
			Choices.Add({Attention, 2.5f});
		}
		float Sum = 0.0f;
		for (const FChoice& C : Choices)
		{
			Sum += C.W;
		}
		float Pick = Rng.FRandRange(0.0f, Sum);
		for (const FChoice& C : Choices)
		{
			Pick -= C.W;
			if (Pick <= 0.0f)
			{
				GazeTarget = C.At;
				break;
			}
		}
		GazeTarget += FVector(Rng.FRandRange(-4.0f, 4.0f), Rng.FRandRange(-4.0f, 4.0f), Rng.FRandRange(-3.0f, 3.0f));
		GazeLeft = Rng.FRandRange(0.7f, 4.5f) * (1.0f - 0.55f * Persona.Restlessness);
	}
	FVector Want = GazeHoldLeft > 0.0f ? GazeHold : GazeTarget;
	float HeadRate = 2.6f;
	if (SeatRole == EBackRoomRole::Hero)
	{
		// Your head goes where the camera looks.
		Want = HeroLook;
		HeadRate = 12.0f;
	}
	// Microsaccades: the eyes never sit perfectly still (less so in a hard stare).
	const float JitterAmount = 0.6f * (1.0f - 0.7f * Stillness);
	const FVector Jitter = JitterAmount * FVector(FMath::Sin(Time * 3.1f + Persona.Seed), FMath::Sin(Time * 2.3f + 1.7f * Persona.Seed), FMath::Sin(Time * 2.9f));
	EyeAt = Ease(EyeAt, Want + Jitter, 22.0f, Dt);
	HeadAt = Ease(HeadAt, Want, HeadRate, Dt);

	FBackRoomBodyPose& P = Anim->Pose;
	P.SeatHeight = SeatHeight;
	P.Time = Time;
	P.Lean = FMath::Clamp(Lean + SighLean, 0.0f, 1.0f);
	P.Slouch = FMath::Clamp(0.25f - 0.3f * Dominance + 0.15f * (1.0f - Arousal) + 0.3f * Gloom, 0.0f, 1.0f);
	P.Breath = Breath;
	P.ShoulderRaise = FMath::Clamp(0.5f * Arousal * (1.0f - Dominance) + 0.3f * Persona.Nervousness - 0.2f + 0.3f * FMath::Max(0.0f, Breath - 1.0f), 0.0f, 1.0f);
	P.LookAt = ToBody(HeadAt);
	P.HeadFollow = SeatRole == EBackRoomRole::Hero ? 1.0f : (GazeHoldLeft > 0.0f ? HeadFollow : (bHabit && HandMode == 3 ? 0.3f : 0.75f));
	P.Twist = Twist;
	const float Drift = 1.0f - 0.9f * Stillness;
	P.HeadTilt = Drift * 2.5f * FMath::Sin(Time * 0.21f + Persona.Seed);
	P.HeadNod = Drift * (1.2f * FMath::Sin(Time * 0.33f + 2.0f * Persona.Seed) + 0.45f * Breath);
	if (TalkLeft > 0.0f)
	{
		P.HeadNod += 2.0f * FMath::Sin(Time * 5.3f) * FMath::Sin(Time * 1.7f);
		P.HeadTilt += 1.5f * FMath::Sin(Time * 2.1f);
	}
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const FHandPose& H = HandNow[Side];
		const FHandPose& W = Wiggle[Side];
		P.HandPos[Side] = H.Pos + W.Pos + PeekBias[Side];
		P.PalmDir[Side] = H.Palm;
		P.FingerDir[Side] = H.Finger;
		P.HandWeight[Side] = 1.0f;
		P.Curl[Side] = FMath::Clamp(H.Curl + W.Curl, 0.0f, 1.0f);
		P.ThumbCurl[Side] = FMath::Clamp(H.Thumb + W.Thumb, 0.0f, 1.0f);
		P.Pinch[Side] = FMath::Clamp(H.Pinch + W.Pinch, 0.0f, 1.0f);
		// The elbow hangs relaxed (down and a little out; straight down with a hand in the lap) and comes up off the
		// rail's crown only as far as the forearm needs: the arm solve finds that against the table itself.
		const float Low = FMath::SmoothStep(78.0f, 68.0f, static_cast<float>(H.Pos.Z));
		P.ElbowPrefer[Side] = FMath::Lerp(-1.0f, -1.45f, Low);
	}
	P.bTableContact = ContactEnabled();
	P.ToTable = Body->GetComponentTransform() * TableToWorld.Inverse();
	P.Tremble = Tremble;
	P.KneeSpread = 0.2f + 0.5f * Dominance;
}

void ABackRoomPlayer::UpdateFace(float Dt)
{
	UBackRoomFaceAnim* Anim = Cast<UBackRoomFaceAnim>(Face->GetAnimInstance());
	if (!Anim)
	{
		return;
	}
	Anim->Body = Body;
	// Your own face is only seen in third person; in first person it's there for its shadow, with no expressions
	// to work out or for RigLogic to run.
	Anim->bExpressionless = SeatRole == EBackRoomRole::Hero && !bHeroHeadShown;
	if (Anim->bExpressionless)
	{
		return;
	}
	TMap<FName, float> C;
	auto Add = [&C](const TCHAR* Control, float Value) {
		if (Value > 0.0005f)
		{
			float& V = C.FindOrAdd(FName(*FString::Printf(TEXT("CTRL_expressions_%s"), Control)));
			V = FMath::Clamp(V + Value, 0.0f, 1.0f);
		}
	};
	auto Both = [&Add](const TCHAR* Control, float Value) {
		Add(*FString::Printf(TEXT("%sL"), Control), Value);
		Add(*FString::Printf(TEXT("%sR"), Control), Value);
	};

	// Blinks: 12-25 a minute, more when anxious, now and then a double; none in a freeze, then a burst.
	NextBlink -= Dt * (1.0f - Stillness);
	if (BlinkT < 0.0f && (BurstBlinks > 0 || NextBlink <= 0.0f))
	{
		BlinkT = 0.0f;
		const float PerMinute = 12.0f + 14.0f * Arousal * (1.0f - Dominance) + 6.0f * Persona.Nervousness;
		NextBlink = Rng.FRandRange(0.3f, 2.0f) * 60.0f / PerMinute;
		bDoubleBlink = BurstBlinks == 0 && Rng.FRand() < 0.12f;
		BurstBlinks = FMath::Max(0, BurstBlinks - 1);
	}
	if (BlinkT >= 0.0f)
	{
		BlinkT += Dt / (BurstBlinks > 0 ? 0.13f : 0.17f);
		const float B = BlinkT < 1.0f ? FMath::Pow(FMath::Sin(BlinkT * UE_PI), 0.6f) : 0.0f;
		Both(TEXT("eyeBlink"), B);
		if (BlinkT >= 1.0f)
		{
			BlinkT = bDoubleBlink ? 0.0f : -1.0f;
			bDoubleBlink = false;
		}
	}

	// The resting face: never quite still, a touch of asymmetry, lips just parted.
	Both(TEXT("eyeRelax"), 0.12f * (1.0f - Stillness));
	Add(TEXT("jawOpen"), (0.02f + 0.02f * FMath::Max(0.0f, FMath::Sin(BreathPhase))) * (1.0f - Stillness));
	Add(TEXT("mouthCornerPullL"), 0.03f + 0.02f * FMath::Sin(Time * 0.4f + Persona.Seed));

	// Emotion, damped by the poker face.
	const float E = Persona.Expressiveness;
	const float Happy = FMath::Max(Valence, 0.0f) * E;
	const float Low = FMath::Max(-Valence, 0.0f) * E;
	Both(TEXT("mouthCornerPull"), 0.5f * Happy);
	Both(TEXT("eyeCheekRaise"), 0.4f * Happy);
	Both(TEXT("browRaiseIn"), 0.45f * Low * (1.0f - Dominance) + 0.2f * Arousal * (1.0f - Dominance) * E);
	Both(TEXT("mouthCornerDepress"), 0.3f * Low * (1.0f - Dominance));
	Both(TEXT("browDown"), 0.5f * Low * Dominance * Arousal);
	Both(TEXT("jawClench"), 0.4f * Low * Dominance * Arousal);
	Both(TEXT("noseNostrilDilate"), 0.3f * Arousal * (0.5f + 0.5f * Low));
	Both(TEXT("mouthStretch"), 0.12f * Arousal * (1.0f - Dominance) * E);
	Both(TEXT("eyePupilWide"), 0.35f * Arousal);
	// Breathing through the throat.
	Add(TEXT("neckThroatInhale"), 0.12f * FMath::Max(0.0f, FMath::Sin(BreathPhase)) * (0.4f + Arousal));

	// Micro-expressions: a flash too quick to hide (not in a freeze).
	NextMicro -= Dt * (1.0f - Stillness);
	if (NextMicro <= 0.0f && MicroT < 0.0f)
	{
		MicroT = 0.0f;
		MicroKind = Rng.RandRange(0, 3);
		NextMicro = Rng.FRandRange(5.0f, 14.0f);
	}
	if (MicroT >= 0.0f)
	{
		MicroT += Dt / 0.45f;
		const float M = MicroT < 1.0f ? FMath::Sin(MicroT * UE_PI) : 0.0f;
		switch (MicroKind)
		{
		case 0: Both(TEXT("mouthLipsPress"), 0.5f * M); break;
		case 1: Both(TEXT("browRaiseOuter"), 0.35f * M); break;
		case 2: Add(TEXT("mouthCornerPullR"), 0.35f * M); Add(TEXT("mouthDimpleR"), 0.3f * M); break;
		default: Both(TEXT("noseWrinkle"), 0.25f * M); break;
		}
		if (MicroT >= 1.0f)
		{
			MicroT = -1.0f;
		}
	}

	// The tells, and talking.
	TellFace(Add, Both);

	for (const TPair<FName, float>& T : TestCurves)
	{
		C.Add(T.Key, T.Value);
	}
	Anim->Curves = MoveTemp(C);
	Anim->LookAt = ToBody(EyeAt);
}
