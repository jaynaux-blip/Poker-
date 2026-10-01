#include "BackRoomCard.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace BackRoomCardDetail
{
const TCHAR* CardMesh = TEXT("/Game/ShortStack/Meshes/SM_Card/SM_Card.SM_Card");
const TCHAR* CardMaterial = TEXT("/Game/ShortStack/Materials/M_Card.M_Card");
const TCHAR* RankChars = TEXT("23456789TJQKA");
const TCHAR* SuitChars = TEXT("cdhs");
// Lying on the felt: a hair above it so the felt's fibers never poke through.
const double FaceUpLift = 0.03;
const double FaceDownLift = 0.06;

UTexture* LoadCardTexture(const FString& Code)
{
	const FString Path = FString::Printf(TEXT("/Game/ShortStack/Cards/T_Card_%s.T_Card_%s"), *Code, *Code);
	return LoadObject<UTexture>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
}

/** The card's turn for its face: face down is a half turn about its long axis (local Y). */
FQuat FaceQuat(float FaceDownAngle)
{
	return FQuat(FVector::YAxisVector, FaceDownAngle);
}

float SmoothStep01(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	return T * T * (3.0f - 2.0f * T);
}
} // namespace BackRoomCardDetail

using namespace BackRoomCardDetail;

ABackRoomCard::ABackRoomCard()
{
	PrimaryActorTick.bCanEverTick = true;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetMobility(EComponentMobility::Movable);
	// Thin: a card's shadow on the felt is a soft line, which contact shadows draw better than maps.
	Mesh->bCastContactShadow = true;
}

void ABackRoomCard::SetCard(int32 InCard)
{
	Card = InCard;
	if (!BackTexture)
	{
		BackTexture = LoadCardTexture(TEXT("back"));
	}
	FaceTexture = BackTexture;
	if (Card >= 0 && Card < 52)
	{
		const FString Code = FString::Printf(TEXT("%c%c"), RankChars[Card >> 2], SuitChars[Card & 3]);
		if (UTexture* Tex = LoadCardTexture(Code))
		{
			FaceTexture = Tex;
		}
	}
	ApplyMaterials();
}

void ABackRoomCard::ApplyMaterials()
{
	if (!Mesh->GetStaticMesh())
	{
		Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, CardMesh));
	}
	UMaterialInterface* Parent = LoadObject<UMaterialInterface>(nullptr, CardMaterial);
	if (!Parent || !Mesh->GetStaticMesh())
	{
		return;
	}
	if (Mids.Num() != 3)
	{
		Mids.Reset();
		for (int32 Slot = 0; Slot < 3; ++Slot)
		{
			UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Parent, this);
			Mid->SetScalarParameterValue(TEXT("Side"), static_cast<float>(Slot));
			Mesh->SetMaterial(Slot, Mid);
			Mids.Add(Mid);
		}
	}
	Mids[0]->SetTextureParameterValue(TEXT("Face"), FaceTexture);
	Mids[1]->SetTextureParameterValue(TEXT("Face"), BackTexture);
	Mids[2]->SetTextureParameterValue(TEXT("Face"), BackTexture);
	ApplyBend();
}

void ABackRoomCard::ApplyBend()
{
	for (UMaterialInstanceDynamic* Mid : Mids)
	{
		// The lift goes toward the ceiling whichever way up the card lies.
		Mid->SetScalarParameterValue(TEXT("Up"), bFaceUp ? 1.0f : -1.0f);
		// Up to about 80 degrees: the lifted corner faces the eyes.
		Mid->SetScalarParameterValue(TEXT("Lift"), Peek * 1.45f);
		// A tight bend: the lifted end stands up as a flap (plastic cards spring back flat).
		Mid->SetScalarParameterValue(TEXT("Radius"), 0.6f);
		// The hinge sits about 3.5 cm in from the near edge: enough to bare the index, no more.
		Mid->SetScalarParameterValue(TEXT("Hinge"), -1.0f);
		Mid->SetScalarParameterValue(TEXT("BendAngle"), PeekAngle);
	}
}

void ABackRoomCard::SetFaceUp(bool bInFaceUp)
{
	bFaceUp = bInFaceUp;
	FlipT = 1.0f;
	ApplyBend();
	SetActorTransform(FTransform(Rest.GetRotation() * FaceQuat(bFaceUp ? 0.0f : UE_PI), Rest.GetLocation() + FVector(0.0, 0.0, bFaceUp ? FaceUpLift : FaceDownLift)));
}

void ABackRoomCard::PitchTo(const FTransform& Target, float Duration, float Arc, float Spin, bool bInFlipOnLand)
{
	bFlipOnLand = bInFlipOnLand;
	From = GetActorTransform();
	Rest = Target;
	MoveT = 0.0f;
	MoveDuration = FMath::Max(Duration, 0.05f);
	MoveArc = Arc;
	MoveSpin = Spin;
	bSliding = false;
}

void ABackRoomCard::SlideTo(const FTransform& Target, float Duration)
{
	PitchTo(Target, Duration, 0.0f, 0.0f);
	bSliding = true;
}

void ABackRoomCard::Flip(bool bInFaceUp, float Duration)
{
	if (bInFaceUp == bFaceUp)
	{
		return;
	}
	bFaceUp = bInFaceUp;
	FlipT = 0.0f;
	FlipDuration = FMath::Max(Duration, 0.05f);
	Peek = 0.0f;
	ApplyBend();
}

void ABackRoomCard::SetPeek(float Amount, const FVector& Toward)
{
	Peek = FMath::Clamp(Amount, 0.0f, 1.0f);
	if (Peek > 0.0f)
	{
		// The bend runs from the hinge away from the edge being lifted: point it away from the peeker.
		const FVector Local = GetActorTransform().InverseTransformVector(Toward - GetActorLocation());
		const FVector2D Away = -FVector2D(Local.X, Local.Y).GetSafeNormal();
		PeekAngle = FMath::Atan2(Away.Y, Away.X);
	}
	ApplyBend();
}

void ABackRoomCard::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const bool bMoving = MoveT < 1.0f;
	const bool bFlipping = FlipT < 1.0f;
	if (!bMoving && !bFlipping)
	{
		return;
	}
	FlipT = FMath::Min(1.0f, FlipT + DeltaSeconds / FlipDuration);
	const float FlipEase = SmoothStep01(FlipT);
	const float DownAngle = bFaceUp ? UE_PI * (1.0f - FlipEase) : UE_PI * FlipEase;
	const double Lift = FMath::Lerp(bFaceUp ? FaceDownLift : FaceUpLift, bFaceUp ? FaceUpLift : FaceDownLift, FlipEase)
		+ 3.4 * FMath::Sin(FlipT * UE_PI);

	FVector Location = Rest.GetLocation();
	FQuat Base = Rest.GetRotation();
	if (bMoving)
	{
		MoveT = FMath::Min(1.0f, MoveT + DeltaSeconds / MoveDuration);
		// A pitched card leaves the hand fast, lands at three quarters of the way and slides to a stop.
		const float E = bSliding ? SmoothStep01(MoveT) : 1.0f - FMath::Pow(1.0f - MoveT, 2.6f);
		const FVector Start = From.GetLocation();
		Location = FMath::Lerp(Start, Rest.GetLocation(), static_cast<double>(E));
		const float Air = FMath::Clamp(MoveT / 0.72f, 0.0f, 1.0f);
		Location.Z += MoveArc * FMath::Sin(Air * UE_PI) * (1.0f - Air * 0.3f);
		// Start from the card's own facing (whichever way the hand held it), yaw spinning down to rest.
		const FQuat StartBase = From.GetRotation() * FaceQuat(bFaceUp ? 0.0f : UE_PI).Inverse();
		Base = FQuat::Slerp(StartBase, Rest.GetRotation(), FMath::Min(1.0f, E * 1.25f));
		Base = FQuat(FVector::UpVector, FMath::DegreesToRadians(MoveSpin * (1.0f - E))) * Base;
	}
	SetActorTransform(FTransform(Base * FaceQuat(DownAngle), Location + FVector(0.0, 0.0, Lift)));
	if (bFlipOnLand && MoveT >= 1.0f)
	{
		bFlipOnLand = false;
		Flip(true);
	}
}
