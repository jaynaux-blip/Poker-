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
		Mid->SetScalarParameterValue(TEXT("Lift"), Peek * PeekMax);
		Mid->SetScalarParameterValue(TEXT("Radius"), PeekRadius);
		Mid->SetScalarParameterValue(TEXT("Taper0"), Flap.Taper0);
		Mid->SetScalarParameterValue(TEXT("Taper1"), Flap.Taper1);
		Mid->SetScalarParameterValue(TEXT("Cup"), Flap.Cup);
		Mid->SetScalarParameterValue(TEXT("Hinge"), Flap.Hinge);
		Mid->SetScalarParameterValue(TEXT("BendAngle"), Flap.Angle);
		Mid->SetScalarParameterValue(TEXT("Anchor"), Flap.Anchor);
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

namespace BackRoomCardDetail
{
/**
 * The tongue a peek lifts: its hinge runs almost parallel to the near edge (LegAlong is long, so the face turns
 * straight to the player), LegDown cm in from it at the corner.
 */
const double LegAlong = 33.0;
/** The pinch sits this far in from the tip along the near edge: on the corner itself, just clear of the index. */
const double GripInset = -0.3;
} // namespace BackRoomCardDetail

// The index corners are the print's top-left and bottom-right; in the mesh's own frame that is a pair of corners
// whose signs are the same (+1: as the glTF importer turned the card's axes) or opposite (-1). Kept as a console
// variable, the one place the mesh's handedness is stated, in case the card art is ever re-exported another way.
// The tongue's shape, tunable live: how deep it is at the corner, how fast the hinge falls away along the edge (the
// parabola's curvature), and where the lift's taper begins and ends (cm from the anchor, plus the stack's spread).
static TAutoConsoleVariable<float> CVarPeekDepth(TEXT("ss.PeekDepth"), 5.6f, TEXT("The tongue's depth at the corner (cm)."));
static TAutoConsoleVariable<float> CVarPeekCup(TEXT("ss.PeekCup"), 0.03f, TEXT("The hinge's curvature: depth lost per cm squared along the edge."));
static TAutoConsoleVariable<float> CVarPeekTaper0(TEXT("ss.PeekTaper0"), 4.5f, TEXT("Full lift out to this far along the edge (cm)."));
static TAutoConsoleVariable<float> CVarPeekTaper1(TEXT("ss.PeekTaper1"), 9.5f, TEXT("No lift beyond this far along the edge (cm)."));
static TAutoConsoleVariable<int32> CVarIndexParity(TEXT("ss.CardIndexParity"), 1, TEXT("+1 or -1: sign of x*y at the mesh's index corners."), ECVF_Default);

float ABackRoomCard::NearIndexSide()
{
	// A face-down card turned about its long axis: local x = -lateral. Its near end is local +Y.
	return CVarIndexParity.GetValueOnAnyThread() > 0 ? -1.0f : 1.0f;
}

float ABackRoomCard::RadiusFor(float MaxLift)
{
	// A light peek curls tight and short; a deep one rolls the whole corner in a long, soft curve.
	return FMath::Lerp(1.6f, 1.9f, FMath::Clamp((MaxLift - 1.05f) / 1.0f, 0.0f, 1.0f));
}

ABackRoomCard::FFlap ABackRoomCard::FlapToward(const FVector& Toward) const
{
	// The near edge is a short edge (the long axis, local Y, points at the peeker one way or the other). The
	// indices are the print's top-left and bottom-right corners: (-,-) and (+,+) in the mesh's frame (the print's
	// top is -Y), so the near corner that carries one is the one whose signs agree.
	const FVector Local = GetActorTransform().InverseTransformVector(Toward - GetActorLocation());
	FFlap F;
	const double Sy = Local.Y >= 0.0 ? 1.0 : -1.0;
	F.Corner = FVector2D(Sy * (CVarIndexParity.GetValueOnAnyThread() > 0 ? 1.0 : -1.0), Sy);
	// Into the card from the corner, so the hinge line cuts it LegAlong along the edge and LegDown down the side.
	FVector2D D(-F.Corner.X / LegAlong, -F.Corner.Y / CVarPeekDepth.GetValueOnAnyThread());
	D.Normalize();
	F.Angle = static_cast<float>(FMath::Atan2(D.Y, D.X));
	const FVector2D CornerAt(F.Corner.X * Width * 0.5, F.Corner.Y * Length * 0.5);
	F.Hinge = static_cast<float>(FVector2D::DotProduct(CornerAt, D) + LegAlong * FMath::Abs(D.X));
	F.Cup = CVarPeekCup.GetValueOnAnyThread();
	F.Taper0 = CVarPeekTaper0.GetValueOnAnyThread();
	F.Taper1 = CVarPeekTaper1.GetValueOnAnyThread();
	F.Anchor = static_cast<float>(FVector2D::DotProduct(CornerAt, FVector2D(-D.Y, D.X)));
	return F;
}

FVector ABackRoomCard::BendLocal(const FVector& Local, const FFlap& F, float Lift, float Radius, float Height) const
{
	// The same as M_Card's bend (backroom_setup.py CARD_BEND), here for hands to follow the corner.
	const double Up = bFaceUp ? 1.0 : -1.0;
	const FVector2D D(FMath::Cos(F.Angle), FMath::Sin(F.Angle));
	const double T = Local.X * D.X + Local.Y * D.Y;
	// The hinge is a parabola about the corner, and the lift is strongest there and dies away along it (as the
	// material does).
	const double Dp = Local.X * -D.Y + Local.Y * D.X - F.Anchor;
	const double S = F.Hinge - T - F.Cup * Dp * Dp;
	const double Smooth = FMath::Clamp((FMath::Abs(Dp) - F.Taper0) / FMath::Max(F.Taper1 - F.Taper0, 0.01f), 0.0, 1.0);
	Lift *= static_cast<float>(1.0 - Smooth * Smooth * (3.0 - 2.0 * Smooth));
	if (S <= 0.0 || Lift <= 0.0001f)
	{
		return Local + FVector(0.0, 0.0, Up * Height);
	}
	const double R = FMath::Max(Radius, 0.2f);
	const double Arc = Lift * R;
	double U, Z, Phi;
	if (S <= Arc)
	{
		Phi = S / R;
		U = F.Hinge - R * FMath::Sin(Phi);
		Z = R * (1.0 - FMath::Cos(Phi));
	}
	else
	{
		Phi = Lift;
		U = F.Hinge - R * FMath::Sin(Lift) - (S - Arc) * FMath::Cos(Lift);
		Z = R * (1.0 - FMath::Cos(Lift)) + (S - Arc) * FMath::Sin(Lift);
	}
	U += Height * FMath::Sin(Phi);
	Z += Height * FMath::Cos(Phi);
	const FVector2D Perp = FVector2D(Local.X, Local.Y) - T * D;
	return FVector(Perp.X + U * D.X, Perp.Y + U * D.Y, Up * Z);
}

FVector ABackRoomCard::GetPeekTip(float Amount, const FVector& Toward, float MaxLift) const
{
	const FFlap F = FlapToward(Toward);
	const FVector Corner(F.Corner.X * Width * 0.5, F.Corner.Y * Length * 0.5, 0.0);
	return GetActorTransform().TransformPosition(BendLocal(Corner, F, FMath::Clamp(Amount, 0.0f, 1.0f) * MaxLift, RadiusFor(MaxLift), 0.0f));
}

FVector ABackRoomCard::GetPeekGrip(float Amount, const FVector& Toward, float MaxLift) const
{
	const FFlap F = FlapToward(Toward);
	const FVector Grip(F.Corner.X * (Width * 0.5 - GripInset), F.Corner.Y * Length * 0.5, 0.0);
	return GetActorTransform().TransformPosition(BendLocal(Grip, F, FMath::Clamp(Amount, 0.0f, 1.0f) * MaxLift, RadiusFor(MaxLift), 0.0f));
}

float ABackRoomCard::GetPeekSide(const FVector& Toward, const FVector& PeekerRight) const
{
	const FFlap F = FlapToward(Toward);
	const FVector Corner = GetActorTransform().TransformPosition(FVector(F.Corner.X * Width * 0.5, F.Corner.Y * Length * 0.5, 0.0));
	return FVector::DotProduct(Corner - GetActorLocation(), PeekerRight) >= 0.0 ? 1.0f : -1.0f;
}

void ABackRoomCard::SetPeek(float Amount, const FVector& Toward, float MaxLift)
{
	Peek = FMath::Clamp(Amount, 0.0f, 1.0f);
	PeekMax = MaxLift;
	PeekRadius = RadiusFor(MaxLift);
	if (Peek > 0.0f)
	{
		Flap = FlapToward(Toward);
	}
	ApplyBend();
}

ABackRoomCard::FPeekFlapWorld ABackRoomCard::MakeSharedFlap(const ABackRoomCard& Under, const ABackRoomCard& Over, const FVector& Toward)
{
	const FFlap F0 = Under.FlapToward(Toward);
	const FFlap F1 = Over.FlapToward(Toward);
	const FTransform& T0 = Under.GetActorTransform();
	const FVector2D Corner0(F0.Corner.X * Width * 0.5, F0.Corner.Y * Length * 0.5);
	const FVector C0 = T0.TransformPosition(FVector(Corner0.X, Corner0.Y, 0.0));
	const FVector C1 = Over.GetActorTransform().TransformPosition(FVector(F1.Corner.X * Width * 0.5, F1.Corner.Y * Length * 0.5, 0.0));
	const FVector2D DLocal(FMath::Cos(F0.Angle), FMath::Sin(F0.Angle));
	FVector Dir = T0.TransformVectorNoScale(FVector(DLocal.X, DLocal.Y, 0.0));
	Dir.Z = 0.0;
	Dir = Dir.GetSafeNormal();
	const FVector Pd(-Dir.Y, Dir.X, 0.0);
	// The hinge's depth in from the lower card's corner, then slid along the hinge to between the two corners.
	const double Depth = F0.Hinge - FVector2D::DotProduct(Corner0, DLocal);
	FPeekFlapWorld W;
	W.Dir = Dir;
	W.Vertex = C0 + Dir * Depth + Pd * FVector::DotProduct((C0 + C1) * 0.5 - C0, Pd);
	W.Spread = 0.5f * static_cast<float>(FMath::Abs(FVector::DotProduct(C1 - C0, Pd)));
	return W;
}

ABackRoomCard::FFlap ABackRoomCard::FlapShared(const FPeekFlapWorld& W, const FVector& Toward) const
{
	// The card's own corner (for the grips), with the bend itself taken from the world flap: its direction and its
	// vertex, turned into this card's frame.
	FFlap F = FlapToward(Toward);
	const FTransform& T = GetActorTransform();
	FVector DLoc = T.InverseTransformVectorNoScale(W.Dir);
	DLoc.Z = 0.0;
	DLoc = DLoc.GetSafeNormal();
	if (DLoc.IsNearlyZero())
	{
		return F;
	}
	const FVector VLoc = T.InverseTransformPosition(W.Vertex);
	const FVector2D D(DLoc.X, DLoc.Y), Pd(-DLoc.Y, DLoc.X);
	F.Angle = static_cast<float>(FMath::Atan2(D.Y, D.X));
	F.Hinge = static_cast<float>(FVector2D::DotProduct(FVector2D(VLoc.X, VLoc.Y), D));
	F.Anchor = static_cast<float>(FVector2D::DotProduct(FVector2D(VLoc.X, VLoc.Y), Pd));
	F.Taper0 += W.Spread;
	F.Taper1 += W.Spread;
	return F;
}

void ABackRoomCard::SetPeekShared(float Amount, const FPeekFlapWorld& W, float MaxLift, float RadiusReduce, const FVector& Toward)
{
	Peek = FMath::Clamp(Amount, 0.0f, 1.0f);
	PeekMax = MaxLift;
	PeekRadius = RadiusFor(MaxLift) - RadiusReduce;
	if (Peek > 0.0f)
	{
		Flap = FlapShared(W, Toward);
	}
	ApplyBend();
}

FVector ABackRoomCard::GetPeekGripShared(float Amount, const FPeekFlapWorld& W, float MaxLift, float RadiusReduce, float Height, const FVector& Toward) const
{
	const FFlap F = FlapShared(W, Toward);
	const FVector Grip(F.Corner.X * (Width * 0.5 - GripInset), F.Corner.Y * Length * 0.5, 0.0);
	return GetActorTransform().TransformPosition(BendLocal(Grip, F, FMath::Clamp(Amount, 0.0f, 1.0f) * MaxLift, RadiusFor(MaxLift) - RadiusReduce, Height));
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
