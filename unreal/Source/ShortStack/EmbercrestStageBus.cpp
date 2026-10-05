// The 14 (spec 3.3, 5.2, 5.3): the city's bus up the ridge, its arrival along the road into the bay, kneeling and opening
// its front door at the stop, waiting there with its engine idling for the ride home, and pulling away north.
#include "EmbercrestStage.h"

#include "EmbercrestPaint.h"
#include "EmbercrestShared.h"

#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "SlateDrawList.h"

#include <string>

namespace EcxBusDetail
{
constexpr double WheelRadius = 51.0;
constexpr double Wheelbase = 710.0;
/** The arrival: up the inbound lane at 9 m/s to here, then the S into the bay (spec 5.2). */
const FVector2D ArriveStart(5775.0, -3940.0);
const FVector2D ArriveTurn(5775.0, -2320.0);
constexpr double ArriveCruise = 1.8;
constexpr double ArriveStop = 6.0;
constexpr double ArriveSpeed = 900.0;
/** Away: the mirrored S out into the lane, then north out of sight past the stand at Y +7000. */
const FVector2D DepartLane(5775.0, 1500.0);
constexpr double DepartGone = 7000.0;
constexpr double DepartAccel = 130.0;
constexpr double DepartTop = 1200.0;
/** After the doors close: the bus rises off its knees, then pulls out. */
constexpr float DepartRise = 0.9f;
constexpr float DepartGo = 1.6f;

FVector2D BusBezier(const FVector2D& A, const FVector2D& B, const FVector2D& C, const FVector2D& D, double T)
{
	const double U = 1.0 - T;
	return A * (U * U * U) + B * (3.0 * U * U * T) + C * (3.0 * U * T * T) + D * (T * T * T);
}

/** A path as points with the distance along it to each. */
void AddCurve(TArray<FVector2D>& Path, const FVector2D& From, const FVector2D& To, double Handle)
{
	const FVector2D Dir(0.0, 1.0);
	for (int32 I = Path.Num() == 0 ? 0 : 1; I <= 48; ++I)
	{
		Path.Add(BusBezier(From, From + Dir * Handle, To - Dir * Handle, To, I / 48.0));
	}
}

void MeasurePath(const TArray<FVector2D>& Path, TArray<float>& S)
{
	S.SetNum(Path.Num());
	double Run = 0.0;
	for (int32 I = 0; I < Path.Num(); ++I)
	{
		if (I > 0)
		{
			Run += FVector2D::Distance(Path[I - 1], Path[I]);
		}
		S[I] = static_cast<float>(Run);
	}
}

/** Where along Path a distance falls, and the heading there (degrees, UE yaw of travel). */
FVector2D PathPoint(const TArray<FVector2D>& Path, const TArray<float>& S, double Dist, double& OutHeading)
{
	if (Path.Num() < 2)
	{
		OutHeading = 90.0;
		return Path.Num() ? Path[0] : FVector2D::ZeroVector;
	}
	int32 I = 1;
	while (I < Path.Num() - 1 && S[I] < Dist)
	{
		++I;
	}
	const FVector2D A = Path[I - 1];
	const FVector2D B = Path[I];
	const double Seg = FMath::Max(1e-3, static_cast<double>(S[I] - S[I - 1]));
	const double T = FMath::Clamp((Dist - S[I - 1]) / Seg, 0.0, 1.0);
	const FVector2D D = B - A;
	OutHeading = FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X));
	return FMath::Lerp(A, B, T);
}
} // namespace EcxBusDetail

using namespace EcxBusDetail;
using namespace Ecx;

void AEmbercrestStage::BuildBus()
{
	BusRoot = NewPart<USceneComponent>();
	BusBody = NewPart<USceneComponent>(BusRoot);
	UMaterialInterface* Dark = Mat(TEXT("FrameDark"), FrameDarkHex, 0.45f, 0.85f);
	UMaterialInterface* Glass = GlassMat(false);
	UMaterialInterface* Tint = GlassMat(true);
	// The body: the city's white, green and gold livery, the interior, the cab, the lamps (on while it runs).
	if (UStaticMeshComponent* Body = Prop(TEXT("SM_ECX_Bus"), FVector::ZeroVector, 0.0f, FVector(1.0), BusBody))
	{
		Body->SetMobility(EComponentMobility::Movable);
		Lit(Body, {-1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f});
	}
	else
	{
		// Panels, so the step-off isn't out of a solid box: the floor, the roof, the sides (the curb side's front door open),
		// the ends; the livery's bands; the dark windows.
		UMaterialInterface* White = Mat(TEXT("BusWhite"), 0xeceeed, 0.35f, 0.0f);
		UMaterialInterface* Green = Mat(TEXT("TransitGreen"), 0x0f6b3a, 0.4f);
		UMaterialInterface* Gold = Mat(TEXT("TransitGold"), 0xf2b705, 0.35f, 0.5f);
		UMaterialInterface* Skirt = Mat(TEXT("BusSkirt"), 0x2a2d31, 0.6f);
		UMaterialInterface* Floor = Mat(TEXT("BusFloor"), 0x3a3d40, 0.8f);
		Box(Floor, BusLocal(-6.05, -1.25, 0.30), BusLocal(6.05, 1.25, 0.36), false, BusBody);
		Box(White, BusLocal(-6.10, -1.295, 3.10), BusLocal(6.09, 1.295, 3.20), false, BusBody);
		// The curb side (Blender +y), the front door's opening left clear.
		for (const FVector2D& Span : {FVector2D(-6.10, -5.85), FVector2D(-4.70, -0.075), FVector2D(1.075, 6.09)})
		{
			Box(White, BusLocal(Span.X, 1.285, 0.30), BusLocal(Span.Y, 1.295, 3.10), false, BusBody);
		}
		Box(White, BusLocal(-6.10, -1.295, 0.30), BusLocal(6.09, -1.285, 3.10), false, BusBody);
		Box(White, BusLocal(-6.10, -1.295, 0.30), BusLocal(-6.09, 1.295, 3.10), false, BusBody);
		Box(White, BusLocal(6.08, -1.295, 0.30), BusLocal(6.09, 1.295, 3.10), false, BusBody);
		for (const double Y : {-1.300, 1.290})
		{
			Box(Green, BusLocal(-6.11, Y, 0.95), BusLocal(6.10, Y + 0.01, 1.45), false, BusBody);
			Box(Gold, BusLocal(-6.11, Y, 1.45), BusLocal(6.10, Y + 0.01, 1.49), false, BusBody);
			Box(Tint, BusLocal(-4.65, Y - 0.001, 1.55), BusLocal(5.9, Y + 0.011, 2.65), false, BusBody)->SetCastShadow(false);
			Box(Skirt, BusLocal(-6.11, Y, 0.30), BusLocal(6.10, Y + 0.01, 0.55), false, BusBody);
		}
		Box(Glass, BusLocal(-6.115, -1.2, 1.2), BusLocal(-6.105, 1.2, 2.75), false, BusBody)->SetCastShadow(false);
		// The interior's seats in rows (dark shapes through the glass).
		UMaterialInterface* Seat = Mat(TEXT("Moquette"), 0x1f4a35, 0.9f);
		for (double X = -3.6; X < 5.6; X += 0.8)
		{
			for (const double Y : {-0.75, 0.75})
			{
				Box(Seat, BusLocal(X, Y - 0.45, 0.36), BusLocal(X + 0.45, Y + 0.45, 0.85), false, BusBody)->SetCastShadow(false);
			}
		}
	}
	if (UStaticMeshComponent* G = Prop(TEXT("SM_ECX_Bus_Glass"), FVector::ZeroVector, 0.0f, FVector(1.0), BusBody))
	{
		SetSlot(G, SlotOf(G, 0, TEXT("ecx_glass")), Glass);
		SetSlot(G, SlotOf(G, 1, TEXT("ecx_glass_tint")), Tint);
		G->SetCastShadow(false);
	}
	if (UStaticMeshComponent* Cool = Prop(TEXT("SM_ECX_Bus_GlowCool"), FVector::ZeroVector, 0.0f, FVector(1.0), BusBody))
	{
		GlowAll(Cool, GlowCool);
	}
	else
	{
		for (const double Y : {-0.6, 0.6})
		{
			Box(GlowMat(GlowCool), BusLocal(-5.2, Y - 0.04, 3.05), BusLocal(5.8, Y + 0.04, 3.08), false, BusBody)->SetCastShadow(false);
		}
	}
	// The doors: four leaves on their hinges (y +1.275, z 0.36), the front pair at x -5.85 and -4.70, the rear at -0.075 and
	// +1.075; the outer leaf of each pair mirrored. Open is 90 degrees, the free edge swinging out.
	const double Hinges[4] = {-5.85, -4.70, -0.075, 1.075};
	for (int32 K = 0; K < 4; ++K)
	{
		USceneComponent* Pivot = NewPart<USceneComponent>(BusBody);
		Pivot->SetRelativeLocation(BusLocal(Hinges[K], 1.275, 0.36));
		const bool bMirror = K % 2 == 1;
		if (!Prop(TEXT("SM_ECX_Bus_DoorLeaf"), FVector::ZeroVector, 0.0f, FVector(bMirror ? -1.0 : 1.0, 1.0, 1.0), Pivot))
		{
			const double Dir = bMirror ? -1.0 : 1.0;
			Box(Dark, FVector(FMath::Min(0.0, Dir * 57.5), -2.5, 0.0), FVector(FMath::Max(0.0, Dir * 57.5), 2.5, 235.0), false, Pivot);
			Box(Glass, FVector(FMath::Min(Dir * 5.0, Dir * 52.5), -2.6, 20.0), FVector(FMath::Max(Dir * 5.0, Dir * 52.5), 2.6, 225.0), false, Pivot)->SetCastShadow(false);
		}
		else if (UStaticMeshComponent* Leaf = Cast<UStaticMeshComponent>(Pivot->GetChildComponent(0)))
		{
			SetSlot(Leaf, SlotOf(Leaf, 1), Glass);
		}
		BusDoors.Add(Pivot);
	}
	// The wheels: the steering fronts, the rear duals; the left side turned round on its pivot (not mirrored, so the
	// sidewall lettering reads on both sides).
	struct FWheel
	{
		double X, Y;
		bool bDual;
	};
	const FWheel Wheels[4] = {{-3.80, 1.06, false}, {-3.80, -1.06, false}, {3.30, 0.93, true}, {3.30, -0.93, true}};
	UMaterialInterface* Tire = Mat(TEXT("Tire"), 0x151515, 0.85f);
	for (const FWheel& W : Wheels)
	{
		USceneComponent* Pivot = NewPart<USceneComponent>(BusRoot);
		Pivot->SetRelativeLocation(BusLocal(W.X, W.Y, 0.51));
		const double Side = W.Y > 0.0 ? 1.0 : -1.0;
		if (!Prop(W.bDual ? TEXT("SM_ECX_Bus_WheelDual") : TEXT("SM_ECX_Bus_Wheel"), FVector::ZeroVector, Side > 0.0 ? 0.0f : 180.0f, FVector::OneVector, Pivot))
		{
			UStaticMeshComponent* T = Cyl(Tire, FVector(0.0, 0.0, -(W.bDual ? 33.0 : 15.5)), 51.0f, W.bDual ? 66.0f : 31.0f, false, Pivot);
			// Its axis along the axle (the cylinder stands along Z).
			T->SetRelativeRotation(FRotator(0.0f, 0.0f, 90.0f));
			T->SetRelativeLocation(FVector::ZeroVector);
		}
		BusWheels.Add(Pivot);
	}
	// The destination signs (front, side and the route at the back).
	BusSignTextures[0] = Ecx::LoadTex(TEXT("ECX"), TEXT("T_ECX_BusSign_Up"));
	BusSignTextures[1] = Ecx::LoadTex(TEXT("ECX"), TEXT("T_ECX_BusSign_Down"));
	BusSignTextures[2] = Ecx::LoadTex(TEXT("ECX"), TEXT("T_ECX_BusSign_Route"));
	UMaterialInstanceDynamic* Dest = ScreenMat(TEXT("BusDest"), Ecx::LoadMesh(TEXT("SM_ECX_Bus_Signs")) == nullptr);
	UMaterialInstanceDynamic* Route = ScreenMat(TEXT("BusRoute"), Ecx::LoadMesh(TEXT("SM_ECX_Bus_Signs")) == nullptr);
	for (UMaterialInstanceDynamic* M : {Dest, Route})
	{
		M->SetScalarParameterValue(TEXT("Cells"), 0.0f);
		M->SetScalarParameterValue(TEXT("Grid"), 0.0f);
		BusSignMids.Add(M);
	}
	if (UStaticMeshComponent* Signs = Prop(TEXT("SM_ECX_Bus_Signs"), FVector::ZeroVector, 0.0f, FVector(1.0), BusBody))
	{
		SetSlot(Signs, SlotOf(Signs, 0, TEXT("ecx_sign_dest")), Dest);
		SetSlot(Signs, SlotOf(Signs, 1, TEXT("ecx_sign_route")), Route);
		Signs->SetCastShadow(false);
	}
	else
	{
		TurnedBox(Dest, BusLocal(-6.11, 0.0, 2.95), FVector(1.0, 185.0, 26.0), 180.0f, false, BusBody)->SetCastShadow(false);
		TurnedBox(Dest, BusLocal(-4.0, 1.305, 2.55), FVector(1.0, 110.0, 22.0), -90.0f, false, BusBody)->SetCastShadow(false);
		TurnedBox(Route, BusLocal(6.10, 0.0, 2.85), FVector(1.0, 34.0, 26.0), 0.0f, false, BusBody)->SetCastShadow(false);
	}
	if (BusSignTextures[2])
	{
		Route->SetTextureParameterValue(TEXT("ImageA"), BusSignTextures[2]);
		Route->SetTextureParameterValue(TEXT("Image"), BusSignTextures[2]);
		Route->SetScalarParameterValue(TEXT("HasImageA"), 1.0f);
	}
	else if (UTextureRenderTarget2D* T = MakeTarget(256, 196))
	{
		PanelTargets.Add(T);
		if (FSlateTextMeasurer::IsAvailable())
		{
			TSharedPtr<ss::ui::DrawList> List = MakeShared<ss::ui::DrawList>();
			FEcxPaint P(*List, 256.0f, 196.0f);
			P.Rect(0.0f, 0.0f, 256.0f, 196.0f, FEcxPaint::Ink(0x050505));
			P.Text(TEXT("14"), 128.0f, 100.0f, 150.0f, 900, FEcxPaint::Ink(0xffb000), 1);
			RenderToTarget(T, List);
		}
		Route->SetTextureParameterValue(TEXT("TextA"), T);
		Route->SetTextureParameterValue(TEXT("Image"), T);
	}
	SetBusSign(false);
	// Who's aboard: the driver and two passengers (the room's baked crowd where it's been made).
	UStaticMesh* Figure = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/ShortStack/Cast/Built/Crowd/SM_Crowd_MH_0.SM_Crowd_MH_0"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	UStaticMesh* Figure2 = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/ShortStack/Cast/Built/Crowd/SM_Crowd_MH_3.SM_Crowd_MH_3"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Figure)
	{
		Figure = Ecx::LoadMesh(TEXT("SM_Crowd_Seated_A"));
		Figure2 = Ecx::LoadMesh(TEXT("SM_Crowd_Seated_B"));
	}
	if (Figure)
	{
		const FVector Seats[3] = {BusLocal(-4.95, -0.70, 0.36), BusLocal(2.4, -0.75, 0.36), BusLocal(4.1, 0.75, 0.36)};
		for (int32 K = 0; K < 3; ++K)
		{
			UStaticMeshComponent* C = NewPart<UStaticMeshComponent>(BusBody);
			C->SetStaticMesh(K == 2 && Figure2 ? Figure2 : Figure);
			// The crowd's figures face +X: turned to face the bus's front (-X here).
			C->SetRelativeLocationAndRotation(Seats[K], FRotator(0.0f, 180.0f, 0.0f));
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	// Its lights: the cabin's strips, the headlights and the tails, the light over the front door.
	BusRunLights.Add(RectAt(RuleManual, 300.0f, BusLocal(-2.0, 0.0, 3.0), FRotator(-90.0f, 0.0f, 0.0f), 4500.0f, 0, 700.0f, 500.0f, 40.0f, false, 0.0f, BusBody));
	BusRunLights.Add(RectAt(RuleManual, 300.0f, BusLocal(3.0, 0.0, 3.0), FRotator(-90.0f, 0.0f, 0.0f), 4500.0f, 0, 700.0f, 500.0f, 40.0f, false, 0.0f, BusBody));
	for (const double Y : {-0.95, 0.95})
	{
		BusRunLights.Add(SpotAt(RuleManual, 12000.0f, BusLocal(-6.12, Y, 0.85), FRotator(-3.0f, 180.0f, 0.0f), 4300.0f, 0, 6000.0f, 15.0f, 30.0f, true, 0.0f, BusBody));
	}
	for (const double Y : {-1.0, 1.0})
	{
		BusRunLights.Add(PointAt(RuleManual, 40.0f, BusLocal(6.12, Y, 0.95), 0.0f, 0xff1a10, 400.0f, 5.0f, false, 0.0f, BusBody));
	}
	BusDoorLight = SpotAt(RuleManual, 60.0f, BusLocal(-5.27, 1.42, 2.40), FRotator(-80.0f, 0.0f, 0.0f), 3000.0f, 0, 600.0f, 30.0f, 60.0f, false, 0.0f, BusBody);
	// What blocks while it stands at the stop, and the step-off's floor in its door well.
	BusBlocker = Box(nullptr, BusLocal(-6.10, -1.295, 0.0), BusLocal(6.09, 1.295, 3.20), true, BusRoot);
	BusBlocker->SetVisibility(false);
	BusBlocker->SetHiddenInGame(true);
	BusBlocker->SetCastShadow(false);
	// It stops the camera's arm as well (the bus's meshes have no collision): on the platform with the bus behind them the
	// camera stays on the near side of its body rather than inside it.
	BusFloor = Box(nullptr, FVector(5250.0, 20.0, -260.0), FVector(5420.0, 180.0, -206.0), true);
	BusFloor->SetVisibility(false);
	BusFloor->SetHiddenInGame(true);
	BusFloor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Out of sight until it's sent for.
	BusPhase = EEcxBusPhase::Gone;
	BusRoot->SetVisibility(false, true);
	BusBlocker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlaceBus(FVector(BusStopX, BusStopY, ZRoad), 90.0f, 0.0f, 0.0f);
}

void AEmbercrestStage::SetBusSign(bool bHome)
{
	if (BusSignMids.Num() < 1 || !BusSignMids[0])
	{
		return;
	}
	UMaterialInstanceDynamic* Dest = BusSignMids[0];
	if (UTexture* T = BusSignTextures[bHome ? 1 : 0])
	{
		Dest->SetTextureParameterValue(TEXT("ImageA"), T);
		Dest->SetTextureParameterValue(TEXT("Image"), T);
		Dest->SetScalarParameterValue(TEXT("HasImageA"), 1.0f);
		return;
	}
	// The amber destination drawn when the art isn't imported.
	UTextureRenderTarget2D* Target = MakeTarget(1024, 144);
	if (!Target || !FSlateTextMeasurer::IsAvailable())
	{
		return;
	}
	PanelTargets.Add(Target);
	TSharedPtr<ss::ui::DrawList> List = MakeShared<ss::ui::DrawList>();
	FEcxPaint P(*List, 1024.0f, 144.0f);
	P.Rect(0.0f, 0.0f, 1024.0f, 144.0f, FEcxPaint::Ink(0x050505));
	P.Text(TEXT("14"), 30.0f, 74.0f, 96.0f, 700, FEcxPaint::Ink(0xffb000));
	P.Text(bHome ? FString(TEXT("NORTHSIDE \xB7 FIFTH ST")) : FString(TEXT("EMBERCREST")), 150.0f, 74.0f, bHome ? 62.0f : 80.0f, 700, FEcxPaint::Ink(0xffb000), 0, 860.0f);
	RenderToTarget(Target, List);
	Dest->SetScalarParameterValue(TEXT("HasImageA"), 0.0f);
	Dest->SetTextureParameterValue(TEXT("TextA"), Target);
	Dest->SetTextureParameterValue(TEXT("Image"), Target);
}

void AEmbercrestStage::BusArrive()
{
	if (!BusRoot)
	{
		return;
	}
	BusPath.Reset();
	BusPath.Add(ArriveStart);
	BusPath.Add(ArriveTurn);
	AddCurve(BusPath, ArriveTurn, FVector2D(BusStopX, BusStopY), 700.0);
	MeasurePath(BusPath, BusPathS);
	BusPhase = EEcxBusPhase::Arriving;
	BusClock = 0.0f;
	BusDoor = 0.0f;
	BusDoorTarget = 0.0f;
	BusKneel = 0.0f;
	BusKneelTarget = 0.0f;
	bBusRunning = true;
	BusRoot->SetVisibility(true, true);
	if (BusBlocker)
	{
		BusBlocker->SetVisibility(false);
		BusBlocker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	PlaceBus(FVector(ArriveStart.X, ArriveStart.Y, RoadZ(ArriveStart.Y)), 90.0f, static_cast<float>(ArriveSpeed), 0.0f);
}

void AEmbercrestStage::BusAtStop(bool bOpen)
{
	if (!BusRoot)
	{
		return;
	}
	BusPhase = EEcxBusPhase::Stopped;
	BusClock = 0.0f;
	bBusRunning = true;
	BusDoor = bOpen ? 1.0f : 0.0f;
	BusDoorTarget = BusDoor;
	BusKneel = BusDoor;
	BusKneelTarget = BusDoor;
	BusRoot->SetVisibility(true, true);
	if (BusBlocker)
	{
		BusBlocker->SetVisibility(false);
	}
	PlaceBus(FVector(BusStopX, BusStopY, ZRoad), 90.0f, 0.0f, 0.0f);
}

void AEmbercrestStage::BusSetDoors(bool bOpen)
{
	if (BusPhase != EEcxBusPhase::Stopped)
	{
		return;
	}
	const float Want = bOpen ? 1.0f : 0.0f;
	if (BusDoorTarget != Want)
	{
		BusDoorTarget = Want;
		BusKneelTarget = Want;
		BusEvents |= (bOpen ? EcxBusEvent::DoorsOpen : EcxBusEvent::DoorsClose) | EcxBusEvent::Kneel;
	}
}

void AEmbercrestStage::BusDepart()
{
	if (BusPhase != EEcxBusPhase::Stopped)
	{
		return;
	}
	BusSetDoors(false);
	BusPath.Reset();
	AddCurve(BusPath, FVector2D(BusStopX, BusStopY), DepartLane, 700.0);
	BusPath.Add(FVector2D(DepartLane.X, DepartGone + 3000.0));
	MeasurePath(BusPath, BusPathS);
	BusPhase = EEcxBusPhase::Departing;
	BusClock = 0.0f;
}

FVector AEmbercrestStage::BusDoorPoint() const
{
	if (!BusRoot)
	{
		return GetActorTransform().TransformPosition(FVector(5265.0, 100.0, ZPlatform + 100.0));
	}
	return BusRoot->GetComponentTransform().TransformPosition(BusLocal(-5.275, 1.45, 1.0));
}

void AEmbercrestStage::SetBusBlocking(bool bBlock)
{
	bBusBlocking = bBlock;
}

void AEmbercrestStage::SetStepOffFloor(bool bOn)
{
	if (BusFloor)
	{
		BusFloor->SetCollisionEnabled(bOn ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	}
}

uint32 AEmbercrestStage::TakeBusEvents()
{
	const uint32 Out = BusEvents;
	BusEvents = 0;
	return Out;
}

void AEmbercrestStage::PlaceBus(const FVector& Where, float HeadingYaw, float Speed, float Dt)
{
	if (!BusRoot)
	{
		return;
	}
	// The bus's front is its -X: turned to the heading plus half a turn.
	const float Was = BusHeading;
	BusHeading = HeadingYaw;
	BusAt = Where;
	BusSpeed = Speed;
	BusRoot->SetRelativeLocationAndRotation(Where, FRotator(0.0f, HeadingYaw + 180.0f, 0.0f));
	// The wheels roll with it (a turn of the axle is the wheel's pitch); the fronts steer into the curve.
	BusSpin = FMath::Fmod(BusSpin + static_cast<float>(FMath::RadiansToDegrees(Speed * Dt / WheelRadius)), 360.0f);
	const float Moved = Speed * Dt;
	const float Rate = Moved > 0.5f ? FMath::FindDeltaAngleDegrees(Was, HeadingYaw) / Moved : 0.0f;
	const float Steer = FMath::Clamp(static_cast<float>(FMath::RadiansToDegrees(FMath::Atan(Wheelbase * FMath::DegreesToRadians(Rate)))), -12.0f, 12.0f);
	for (int32 K = 0; K < BusWheels.Num(); ++K)
	{
		if (BusWheels[K])
		{
			BusWheels[K]->SetRelativeRotation(FRotator(BusSpin, K < 2 ? Steer : 0.0f, 0.0f));
		}
	}
}

void AEmbercrestStage::UpdateBus(float Dt)
{
	if (!BusRoot || BusPhase == EEcxBusPhase::Gone)
	{
		return;
	}
	BusClock += Dt;
	if (BusPhase == EEcxBusPhase::Arriving)
	{
		// 9 m/s up the lane to the turn, then into the bay on a smooth S, braking to a stop at 6.0 s.
		const double T = BusClock;
		const double Curve = BusPathS.Num() ? BusPathS.Last() - BusPathS[1] : 1.0;
		double Dist = 0.0;
		double Speed = 0.0;
		if (T < ArriveCruise)
		{
			Dist = ArriveSpeed * T;
			Speed = ArriveSpeed;
		}
		else
		{
			// s(u) = L (k u + (3 - 2k) u^2 + (k - 2) u^3): leaving the turn at 9 m/s, stopped exactly at the stop.
			const double Span = ArriveStop - ArriveCruise;
			const double U = FMath::Clamp((T - ArriveCruise) / Span, 0.0, 1.0);
			const double K = ArriveSpeed * Span / FMath::Max(Curve, 1.0);
			Dist = BusPathS[1] + Curve * (K * U + (3.0 - 2.0 * K) * U * U + (K - 2.0) * U * U * U);
			Speed = Curve * (K + 2.0 * (3.0 - 2.0 * K) * U + 3.0 * (K - 2.0) * U * U) / Span;
		}
		double Heading = 90.0;
		const FVector2D P = PathPoint(BusPath, BusPathS, Dist, Heading);
		PlaceBus(FVector(P.X, P.Y, RoadZ(P.Y)), static_cast<float>(Heading), static_cast<float>(FMath::Max(0.0, Speed)), Dt);
		if (T >= ArriveStop)
		{
			BusPhase = EEcxBusPhase::Stopped;
			BusClock = 0.0f;
			BusEvents |= EcxBusEvent::Brakes;
			PlaceBus(FVector(BusStopX, BusStopY, ZRoad), 90.0f, 0.0f, Dt);
			// Kneel and open at once (spec 5.2: the hiss, the chime, the door's light).
			BusSetDoors(true);
		}
	}
	else if (BusPhase == EEcxBusPhase::Departing)
	{
		// The doors shut, it rises off its knees, then out along the mirrored S and away north.
		const double T = BusClock - DepartGo;
		if (T > 0.0)
		{
			if (T - Dt <= 0.0)
			{
				BusEvents |= EcxBusEvent::PullAway;
			}
			const double Accel = DepartAccel;
			const double Tmax = DepartTop / Accel;
			const double Dist = T < Tmax ? 0.5 * Accel * T * T : 0.5 * Accel * Tmax * Tmax + DepartTop * (T - Tmax);
			const double Speed = T < Tmax ? Accel * T : DepartTop;
			double Heading = 90.0;
			const FVector2D P = PathPoint(BusPath, BusPathS, Dist, Heading);
			PlaceBus(FVector(P.X, P.Y, RoadZ(P.Y)), static_cast<float>(Heading), static_cast<float>(Speed), Dt);
			if (P.Y > DepartGone)
			{
				// Behind the north stand: gone, and silent.
				BusPhase = EEcxBusPhase::Gone;
				bBusRunning = false;
				BusRoot->SetVisibility(false, true);
			}
		}
		if (BusClock >= DepartRise && BusClock - Dt < DepartRise)
		{
			BusKneelTarget = 0.0f;
		}
	}
	// The doors over 0.9 s, the kneel over 0.6 s.
	BusDoor = FMath::FInterpConstantTo(BusDoor, BusDoorTarget, Dt, 1.0f / 0.9f);
	BusKneel = FMath::FInterpConstantTo(BusKneel, BusPhase == EEcxBusPhase::Departing && BusClock < DepartRise ? BusKneel : BusKneelTarget, Dt, 1.0f / 0.6f);
	const float Open = FMath::SmoothStep(0.0f, 1.0f, BusDoor);
	for (int32 K = 0; K < BusDoors.Num(); ++K)
	{
		if (BusDoors[K])
		{
			// The front pair opens (the rear stays shut at this stop); the outer leaf of each pair the other way.
			const float Leaf = K < 2 ? Open : 0.0f;
			BusDoors[K]->SetRelativeRotation(FRotator(0.0f, (K % 2 == 0 ? -90.0f : 90.0f) * Leaf, 0.0f));
		}
	}
	if (BusBody)
	{
		// Kneeling: 8 cm down at the front curb corner.
		const float Kn = FMath::SmoothStep(0.0f, 1.0f, BusKneel);
		BusBody->SetRelativeLocationAndRotation(FVector(0.0, 0.0, -6.0 * Kn), FRotator(0.6f * Kn, 0.0f, -0.6f * Kn));
	}
	// Its lights: the cabin and the tails while it's here, the headlights while it runs at dusk and night, the door's light.
	const float NightOn = BelowEl(SunEl, 4.0, 2.0);
	for (int32 K = 0; K < BusRunLights.Num(); ++K)
	{
		if (ULightComponent* L = BusRunLights[K])
		{
			const bool bHead = K == 2 || K == 3;
			const bool bOn = bBusRunning && (!bHead || NightOn > 0.01f);
			if (L->IsVisible() != bOn)
			{
				L->SetVisibility(bOn);
			}
		}
	}
	if (BusDoorLight)
	{
		const bool bOn = BusDoor > 0.4f;
		if (BusDoorLight->IsVisible() != bOn)
		{
			BusDoorLight->SetVisibility(bOn);
		}
	}
	if (BusBlocker)
	{
		const bool bBlock = BusPhase == EEcxBusPhase::Stopped && bBusBlocking;
		if ((BusBlocker->GetCollisionEnabled() != ECollisionEnabled::NoCollision) != bBlock)
		{
			BusBlocker->SetCollisionEnabled(bBlock ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		}
	}
}
