#include "StreetTraffic.h"

#include "BlenderProps.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "StreetStage.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogStreetTraffic, Log, All);

namespace StreetTrafficDetail
{
/** The car kit (spec 4.3, ns_cars.py): its mesh, its wheels (axles' y, track's x, hub's z: Blender meters), its length, tinted. */
struct FKind
{
	const TCHAR* Name;
	double AxleFront;
	double AxleRear;
	double Track;
	double Hub;
	double Length;
	bool bTinted;
	bool bCab;
};
const FKind Kinds[] = {
	{TEXT("SM_NS_Hatchback"), -1.30, 1.25, 0.755, 0.312, 4.20, true, false}, {TEXT("SM_NS_SUV"), -1.42, 1.36, 0.82, 0.37, 4.80, true, false},
	{TEXT("SM_NS_Cab"), -1.42, 1.38, 0.785, 0.326, 4.85, false, true},      {TEXT("SM_NS_Van"), -1.86, 1.80, 0.865, 0.345, 5.90, false, false},
	{TEXT("SM_NS_BoxTruck"), -2.40, 2.30, 0.95, 0.42, 7.50, false, false},   {TEXT("SM_NS_Pickup"), -1.75, 1.75, 0.85, 0.39, 5.30, true, false},
};
// The street's paints (spec 3.6): white, black, grey and silver most of all.
const uint32 Paints[] = {0xe8e8e6, 0x16171a, 0x5a5d62, 0xa7abb0, 0xe8e8e6, 0x16171a, 0x1f3354, 0x8b1a1a, 0xa7abb0, 0x5a5d62, 0x1f4a3a, 0xb9a888};

/**
 * How far from the road's centerline each keep-right lane runs (cm): the middle of its 2.6 m travel lane, clear of the cars
 * parked at the curb (their middles 3.95 m out, StreetStageFurniture.cpp and MarketStage.cpp): a 2.3 m box truck passing a
 * parked one keeps 25 cm between them (2 m out it ran 35 cm into it), and two passing each other half a meter.
 */
const double LaneOffset = 140.0;
/** The pool: two cars a street. */
const int32 PoolSize = 6;
const float HeadCandela = 4000.0f;

UStaticMesh* KitMesh(const TCHAR* Name)
{
	return AStreetStage::NsMesh(Name);
}

AStreetTraffic* TrafficIn(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<AStreetTraffic> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

FAutoConsoleCommandWithWorldAndArgs CmdSpawn(TEXT("ss.Traffic.Spawn"), TEXT("A car onto a cross street's lane now: ss.Traffic.Spawn [lane 0..5, -1 whichever is due]."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (AStreetTraffic* T = TrafficIn(World))
		{
			UE_LOG(LogStreetTraffic, Display, TEXT("%s"), *T->SpawnNow(Args.Num() > 0 ? FCString::Atoi(*Args[0]) : -1));
		}
	}));
FAutoConsoleCommandWithWorldAndArgs CmdClear(TEXT("ss.Traffic.Clear"), TEXT("Every car off the cross streets."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (AStreetTraffic* T = TrafficIn(World))
		{
			UE_LOG(LogStreetTraffic, Display, TEXT("%s"), *T->ClearAll());
		}
	}));
} // namespace StreetTrafficDetail

using namespace StreetTrafficDetail;

AStreetTraffic::AStreetTraffic()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	CubeMesh = Cube.Object;
}

void AStreetTraffic::AppendPreload(TArray<FString>& Paths)
{
	for (const FKind& K : Kinds)
	{
		Paths.Add(FStreetPreload::MeshPath(K.Name));
		Paths.Add(FStreetPreload::MeshPath(*FString::Printf(TEXT("%s_Wheel"), K.Name)));
		Paths.Add(FStreetPreload::MeshPath(*FString::Printf(TEXT("%s_GlowLamps"), K.Name)));
	}
	Paths.Add(FStreetPreload::MeshPath(TEXT("SM_Sedan")));
}

AStreetTraffic* AStreetTraffic::FindOrSpawn(UWorld* World, AStreetStage* InStreet)
{
	if (!World || !InStreet)
	{
		return nullptr;
	}
	AStreetTraffic* T = TrafficIn(World);
	if (!T)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		T = World->SpawnActor<AStreetTraffic>(AStreetTraffic::StaticClass(), InStreet->GetActorTransform(), Params);
	}
	if (T)
	{
		T->Begin(InStreet);
	}
	return T;
}

void AStreetTraffic::Begin(AStreetStage* InStreet)
{
	if (bBegun || !InStreet)
	{
		return;
	}
	bBegun = true;
	Street = InStreet;
	SetActorTransform(InStreet->GetActorTransform());
	Dice = static_cast<uint32>(FPlatformTime::Cycles()) | 1u;
	// The lanes (spec 2.7, keeping right: facing east (-X) the right is south): Hollis's and Wexler's along X, 60 m either
	// side of Fifth's middle; Cardinal's along Y, 60 m either side of Market's (only with Market at its new width).
	const double Hollis = (Ns::HollisRoad0 + Ns::HollisRoad1) * 0.5;
	const double Wexler = (Ns::WexlerRoad0 + Ns::WexlerRoad1) * 0.5;
	const double Z = Ns::RoadZ(LaneOffset, 1000.0);
	const double X0 = Ns::FifthMidX + Ns::PassReach;
	const double X1 = Ns::FifthMidX - Ns::PassReach;
	// Past its end a car runs on while it would be seen going, as far as the street's ground (short of the fronts across it).
	const double East = FMath::Max(0.0, X1 - (Ns::CardinalBacks + 400.0));
	const double West = FMath::Max(0.0, (Ns::CrossWest - 400.0) - X0);
	Lanes.Add({FVector(X0, Hollis - LaneOffset, Z), FVector(X1, Hollis - LaneOffset, Z), 180.0f, 0, TEXT("Hollis eastbound"), East});
	Lanes.Add({FVector(X1, Hollis + LaneOffset, Z), FVector(X0, Hollis + LaneOffset, Z), 0.0f, 0, TEXT("Hollis westbound"), West});
	Lanes.Add({FVector(X0, Wexler - LaneOffset, Z), FVector(X1, Wexler - LaneOffset, Z), 180.0f, 1, TEXT("Wexler eastbound"), East});
	Lanes.Add({FVector(X1, Wexler + LaneOffset, Z), FVector(X0, Wexler + LaneOffset, Z), 0.0f, 1, TEXT("Wexler westbound"), West});
	if (InStreet->IsWideMarket())
	{
		const double Y0 = Ns::MarketMidY - Ns::PassReach;
		const double Y1 = Ns::MarketMidY + Ns::PassReach;
		const double South = FMath::Max(0.0, Y0 - (Ns::CardinalSouth + 400.0));
		const double North = FMath::Max(0.0, (Ns::CardinalNorth - 400.0) - Y1);
		Lanes.Add({FVector(Ns::CardinalMidX - LaneOffset, Y0, Z), FVector(Ns::CardinalMidX - LaneOffset, Y1, Z), 90.0f, 2, TEXT("Cardinal northbound"), North});
		Lanes.Add({FVector(Ns::CardinalMidX + LaneOffset, Y1, Z), FVector(Ns::CardinalMidX + LaneOffset, Y0, Z), -90.0f, 2, TEXT("Cardinal southbound"), South});
	}
	// The pool: the kit's kinds that are imported (never the same twice in a row), each its paint.
	TArray<int32> Have;
	for (int32 K = 0; K < static_cast<int32>(UE_ARRAY_COUNT(Kinds)); ++K)
	{
		if (KitMesh(Kinds[K].Name))
		{
			Have.Add(K);
		}
	}
	for (int32 I = 0; I < PoolSize; ++I)
	{
		const int32 Kind = Have.Num() > 0 ? Have[I % Have.Num()] : -1;
		MakeCar(Kind, Paints[(I * 5 + 3) % UE_ARRAY_COUNT(Paints)]);
	}
	// The first cars soon, each street on its own clock.
	for (int32 S = 0; S < 3; ++S)
	{
		NextAt[S] = 1.0 + 4.0 * S;
	}
	UE_LOG(LogStreetTraffic, Log, TEXT("The cross streets' traffic: %d lanes, %d cars (%d of the kit's kinds imported)"), Lanes.Num(), Cars.Num(), Have.Num());
}

void AStreetTraffic::MakeCar(int32 Kind, uint32 Paint)
{
	auto Part = [this](auto* Comp, USceneComponent* Parent) {
		Comp->SetupAttachment(Parent);
		Comp->RegisterComponent();
		return Comp;
	};
	FCar Car;
	Car.Id = static_cast<uint32>(Cars.Num());
	USceneComponent* CarRoot = Part(NewObject<USceneComponent>(this, NAME_None, RF_Transient), Root.Get());
	Car.Root = CarRoot;
	double Length = 480.0;
	if (Kind >= 0)
	{
		const FKind& K = Kinds[Kind];
		Length = K.Length * 100.0;
		Car.bCab = K.bCab;
		UStaticMeshComponent* Body = Part(NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient), CarRoot);
		Body->SetStaticMesh(KitMesh(K.Name));
		Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (K.bTinted)
		{
			TintImported(Body, ImportedSlot(Body, 0), Ns::Srgb(Paint));
		}
		Screens.Paint(this, Body, TEXT("ns_plate"), TEXT("plate:") + Ns::PlateFor(3, static_cast<int32>(Car.Id)), 256, Ns::EScreen::Printed);
		if (UStaticMesh* Wheel = KitMesh(*FString::Printf(TEXT("%s_Wheel"), K.Name)))
		{
			Car.WheelRadius = static_cast<float>(K.Hub * 100.0);
			for (int32 W = 0; W < 4; ++W)
			{
				const double Side = W % 2 == 0 ? 1.0 : -1.0;
				UStaticMeshComponent* Wc = Part(NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient), CarRoot);
				Wc->SetStaticMesh(Wheel);
				Wc->SetRelativeLocation(FVector(100.0 * Side * K.Track, -100.0 * (W < 2 ? K.AxleFront : K.AxleRear), 100.0 * K.Hub));
				Wc->SetRelativeRotation(Side > 0.0 ? FRotator::ZeroRotator : FRotator(0.0f, 180.0f, 0.0f));
				Wc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Car.Wheels.Add(Wc);
				Car.WheelSides.Add(static_cast<float>(Side));
			}
		}
		// Its lamps: a film over the baked lenses and glass (ns_cars.py), so drawn additive (M_NS_GlowAdd: an off lamp adds
		// nothing) and off Nanite (which draws no translucency); without that material, none (an opaque glow would black
		// the lenses out by day).
		UMaterialInterface* LampBase = Cast<UMaterialInterface>(AStreetStage::NsAsset(Ns::GlowAddPath));
		UStaticMesh* Glow = LampBase ? KitMesh(*FString::Printf(TEXT("%s_GlowLamps"), K.Name)) : nullptr;
		if (Glow)
		{
			UStaticMeshComponent* G = Part(NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient), CarRoot);
			G->SetForceDisableNanite(true);
			G->SetStaticMesh(Glow);
			G->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			G->SetCastShadow(false);
			G->SetVisibleInRayTracing(false);
			if (UMaterialInterface* Base = LampBase)
			{
				UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Base, this);
				Mid->SetScalarParameterValue(TEXT("Mode"), 2.0f);
				for (int32 Slot = 0; Slot < G->GetNumMaterials(); ++Slot)
				{
					G->SetMaterial(Slot, Mid);
				}
				Car.Lamps = Mid;
			}
		}
	}
	else if (UStaticMesh* Sedan = KitMesh(TEXT("SM_Sedan")))
	{
		UStaticMeshComponent* Body = Part(NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient), CarRoot);
		Body->SetStaticMesh(Sedan);
		Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		TintImported(Body, ImportedSlot(Body, 0), Ns::Srgb(Paint));
		Screens.Paint(this, Body, TEXT("ns_plate"), TEXT("plate:") + Ns::PlateFor(3, static_cast<int32>(Car.Id)), 256, Ns::EScreen::Printed);
	}
	else
	{
		// A box of a car (in its imported frame: its front along +Y).
		UStaticMeshComponent* Body = Part(NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient), CarRoot);
		Body->SetStaticMesh(CubeMesh);
		Body->SetRelativeLocation(FVector(0.0, 0.0, 70.0));
		Body->SetRelativeScale3D(FVector(1.8, 4.6, 1.2));
		Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	// Two headlights at the front (its imported frame's +Y), aimed down the road a little: MegaLights, 30 m, 22 degrees.
	for (int32 H = 0; H < 2; ++H)
	{
		USpotLightComponent* L = Part(NewObject<USpotLightComponent>(this, NAME_None, RF_Transient), CarRoot);
		L->SetRelativeLocationAndRotation(FVector(H == 0 ? -65.0 : 65.0, Length * 0.5 - 10.0, 68.0), FRotator(-4.0f, 90.0f, 0.0f));
		L->SetIntensityUnits(ELightUnits::Candelas);
		L->SetIntensity(0.0f);
		L->SetAttenuationRadius(3000.0f);
		L->SetOuterConeAngle(22.0f);
		L->SetInnerConeAngle(10.0f);
		L->SetLightColor(Ns::Srgb(0xfff2dc));
		L->SetCastShadows(true);
		L->SetVolumetricScatteringIntensity(0.25f);
		L->SetVisibility(false);
		Car.Heads[H] = L;
	}
	CarRoot->SetVisibility(false, true);
	Cars.Add(Car);
}

bool AStreetTraffic::Seen(const FVector& At) const
{
	const UWorld* World = GetWorld();
	const APlayerController* Pc = World ? World->GetFirstPlayerController() : nullptr;
	const APlayerCameraManager* Cam = Pc ? Pc->PlayerCameraManager.Get() : nullptr;
	if (!Cam)
	{
		return false;
	}
	const FVector To = GetActorTransform().TransformPosition(At) - Cam->GetCameraLocation();
	const double Dist = To.Size();
	if (Dist > 7000.0)
	{
		return false;
	}
	const double Facing = FVector::DotProduct(To / FMath::Max(Dist, 1.0), Cam->GetCameraRotation().Vector());
	return Facing > FMath::Cos(FMath::DegreesToRadians(Cam->GetFOVAngle() * 0.5f + 10.0f));
}

void AStreetTraffic::Launch(FCar& Car, int32 Lane)
{
	Car.Lane = Lane;
	Car.S = 0.0;
	// 30-40 km/h.
	Dice = Dice * 1664525u + 1013904223u;
	Car.Speed = 833.0f + 278.0f * static_cast<float>(Dice >> 8) / static_cast<float>(1u << 24);
	if (USceneComponent* R = Car.Root.Get())
	{
		R->SetVisibility(true, true);
	}
	Place(Car);
	ApplyLamps(Car);
	++Passes;
}

void AStreetTraffic::Park(FCar& Car)
{
	Car.Lane = -1;
	if (USceneComponent* R = Car.Root.Get())
	{
		R->SetVisibility(false, true);
	}
}

void AStreetTraffic::Place(FCar& Car)
{
	if (!Lanes.IsValidIndex(Car.Lane))
	{
		return;
	}
	const FLane& L = Lanes[Car.Lane];
	const FVector Dir = (L.To - L.From).GetSafeNormal();
	if (USceneComponent* R = Car.Root.Get())
	{
		R->SetRelativeLocationAndRotation(L.From + Dir * Car.S, BlenderFacing(L.Yaw));
	}
	// The wheels roll with the distance: the near side about its axle one way, the far side (turned half round) the other.
	for (int32 W = 0; W < Car.Wheels.Num(); ++W)
	{
		if (USceneComponent* Wheel = Car.Wheels[W].Get())
		{
			const float Side = Car.WheelSides.IsValidIndex(W) ? Car.WheelSides[W] : 1.0f;
			const FQuat Spin = Side > 0.0f ? FQuat(FVector::XAxisVector, -Car.Roll) : FQuat(FVector::ZAxisVector, UE_DOUBLE_PI) * FQuat(FVector::XAxisVector, Car.Roll);
			Wheel->SetRelativeRotation(Spin);
		}
	}
}

void AStreetTraffic::ApplyLamps(FCar& Car)
{
	// Headlights from dusk (low beams, the tails), a running light by day; the cab's roof light on duty.
	const float Night = Levels.Night;
	if (UMaterialInstanceDynamic* M = Car.Lamps.Get())
	{
		M->SetScalarParameterValue(TEXT("Level"), Levels.Lamps);
		M->SetVectorParameterValue(TEXT("LitA"), FLinearColor(FMath::Max(0.25f, Night), 0.0f, Night, 0.0f));
		M->SetVectorParameterValue(TEXT("LitB"), FLinearColor(0.0f, 0.0f, 0.0f, Car.bCab ? 1.0f : 0.0f));
	}
	for (int32 H = 0; H < 2; ++H)
	{
		if (USpotLightComponent* L = Car.Heads[H].Get())
		{
			const bool bOn = Car.Lane >= 0 && Night > 0.3f;
			L->SetIntensity(HeadCandela * Night);
			L->SetVisibility(bOn);
		}
	}
}

void AStreetTraffic::SetLook(const Ns::FGlowLevels& InLevels, float InRain, bool bInHold)
{
	Levels = InLevels;
	Rain = InRain;
	bHold = bInHold;
	if (FMath::Abs(Levels.Night - AppliedNight) > 0.02f)
	{
		AppliedNight = Levels.Night;
		for (FCar& Car : Cars)
		{
			ApplyLamps(Car);
		}
	}
}

void AStreetTraffic::EndPlay(const EEndPlayReason::Type Reason)
{
	Screens.Reset();
	Super::EndPlay(Reason);
}

void AStreetTraffic::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bBegun || bHold || Lanes.Num() == 0)
	{
		return;
	}
	const float Dt = FMath::Min(DeltaSeconds, 0.1f);
	Clock += Dt;
	// The cars on their way, gone at the far end (in the haze, 60 m off).
	for (FCar& Car : Cars)
	{
		if (Car.Lane < 0)
		{
			continue;
		}
		const FLane& L = Lanes[Car.Lane];
		Car.S += Car.Speed * Dt;
		Car.Roll = FMath::Fmod(Car.Roll + Car.Speed * Dt / FMath::Max(10.0f, Car.WheelRadius), 2.0 * UE_DOUBLE_PI);
		const double Length = (L.To - L.From).Size();
		if (Car.S >= Length)
		{
			// Gone at its off-set end, unless the camera would see it go: then on into the haze until it wouldn't.
			const FVector At = L.From + (L.To - L.From).GetSafeNormal() * Car.S;
			if (Car.S >= Length + L.Beyond || !Seen(At))
			{
				Park(Car);
				continue;
			}
		}
		Place(Car);
	}
	// Each street's next car when it's due (6-20 s by day, 20-60 s at night), onto one of its lanes, never where it'd be seen
	// appearing; a free car of the pool, or wait.
	for (int32 Cross = 0; Cross < 3; ++Cross)
	{
		if (Clock < NextAt[Cross])
		{
			continue;
		}
		TArray<int32> Choices;
		for (int32 I = 0; I < Lanes.Num(); ++I)
		{
			if (Lanes[I].Street == Cross)
			{
				Choices.Add(I);
			}
		}
		if (Choices.Num() == 0)
		{
			NextAt[Cross] = Clock + 1.0e6;
			continue;
		}
		Dice = Dice * 1664525u + 1013904223u;
		const float U = static_cast<float>(Dice >> 8) / static_cast<float>(1u << 24);
		const int32 Lane = Choices[FMath::Min(Choices.Num() - 1, static_cast<int32>(U * Choices.Num()))];
		FCar* Free = Cars.FindByPredicate([](const FCar& C) { return C.Lane < 0; });
		if (!Free || Seen(Lanes[Lane].From))
		{
			NextAt[Cross] = Clock + 1.0;
			continue;
		}
		Launch(*Free, Lane);
		const float Night = Levels.Night;
		NextAt[Cross] = Clock + FMath::Lerp(6.0f + 14.0f * U, 20.0f + 40.0f * U, Night);
	}
}

FString AStreetTraffic::SpawnNow(int32 Lane)
{
	if (Lanes.Num() == 0)
	{
		return TEXT("traffic: no lanes (not begun)");
	}
	const int32 Use = Lanes.IsValidIndex(Lane) ? Lane : Passes % Lanes.Num();
	FCar* Free = Cars.FindByPredicate([](const FCar& C) { return C.Lane < 0; });
	if (!Free)
	{
		return TEXT("traffic: every car is out");
	}
	Launch(*Free, Use);
	return FString::Printf(TEXT("traffic: a car onto %s"), Lanes[Use].Name);
}

FString AStreetTraffic::ClearAll()
{
	for (FCar& Car : Cars)
	{
		Park(Car);
	}
	return TEXT("traffic: every car off the street");
}

FString AStreetTraffic::Describe() const
{
	int32 Moving = 0;
	for (const FCar& C : Cars)
	{
		Moving += C.Lane >= 0 ? 1 : 0;
	}
	return FString::Printf(TEXT("traffic: %d of %d cars moving on %d lanes, %d passes%s"), Moving, Cars.Num(), Lanes.Num(), Passes, bHold ? TEXT(" (held: a skip)") : TEXT(""));
}

void AStreetTraffic::AppendEmitters(TArray<FStreetEmitter>& Out, const FVector& Camera, float InRain) const
{
	const FTransform& T = GetActorTransform();
	for (const FCar& C : Cars)
	{
		if (C.Lane < 0 || !Lanes.IsValidIndex(C.Lane))
		{
			continue;
		}
		const FLane& L = Lanes[C.Lane];
		const FVector At = T.TransformPosition(L.From + (L.To - L.From).GetSafeNormal() * C.S + FVector(0.0, 0.0, 50.0));
		if (FVector::DistSquared(At, Camera) > 4000.0 * 4000.0)
		{
			continue;
		}
		FStreetEmitter E;
		E.At = At;
		E.Kind = EStreetEmitter::Engine;
		E.Level = 0.9f;
		E.Hz = C.Speed / 100.0f;
		E.Id = 9000u + C.Id;
		Out.Add(E);
	}
}
