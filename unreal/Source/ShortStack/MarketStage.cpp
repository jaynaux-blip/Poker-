#include "MarketStage.h"

#include "BlenderProps.h"
#include "Components/DecalComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "PhysicsEngine/BodySetup.h"
#include "ShopStage.h"
#include "StreetStage.h"
#include "StreetWorld.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogMarketStage, Log, All);

namespace MarketStageDetail
{
const float PatBrick = 1.0f;
const float PatAsphalt = 2.0f;
const float PatSlabs = 3.0f;
const float PatConcrete = 5.0f;
const float PatShutter = 6.0f;
const uint32 LedHex = 0xffe9d0;
const uint32 HpsHex = 0xffb35c;

/** One of Market's plots (spec 2.3): its kit id, its footprint and height, its brick for the stand-in, its shop. */
struct FPlot
{
	const TCHAR* Id;
	double X0;
	double X1;
	double Y0;
	double Y1;
	double Height;
	uint32 Brick;
	/** Its Cardinal face (the corner plots: X of the face; 0 none). */
	double CardinalX;
	/** Its storefront opening is a shop stage's (the thrift, the pawn, the gym): venue, and the opening (X0, X1, height); else its
	 *  display shop's id in core's table (its real hours), or null. */
	const TCHAR* Venue;
	double OpenX0;
	double OpenX1;
	double OpenZ;
	Ns::FHours Hours;
	uint32 NeonHex;
	uint32 CoolHex;
	double SpillX;
	double SpillWidth;
	uint32 SpillHex;
	float SpillLumens;
};
const FPlot Plots[] = {
	// The south side, fronts on Y 4950 facing north, to Y 2400.
	{TEXT("MS1"), -2400.0, -1400.0, 2400.0, 4950.0, 1450.0, 0x7a3a2a, 0.0, TEXT("pharmacy"), 0, 0, 0, {480, 1320, 0x7f}, 0, 0x30ff80, -1900.0, 700.0, 0xf0fff4, 650.0f},
	{TEXT("MS2"), -3700.0, -2400.0, 2400.0, 4950.0, 1000.0, 0x9c8160, 0.0, TEXT("thrift"), -3660.0, -2440.0, 400.0, {600, 1260, 0x7f}, 0, 0, 0.0, 0.0, 0, 0.0f},
	{TEXT("MS3"), -4500.0, -3700.0, 2400.0, 4950.0, 1400.0, 0xb9b2a3, 0.0, TEXT("pawn"), -4440.0, -3760.0, 380.0, {540, 1200, 0x7f}, 0, 0, 0.0, 0.0, 0, 0.0f},
	{TEXT("MS4"), -6600.0, -4500.0, 2400.0, 4950.0, 1050.0, 0x6b3427, 0.0, TEXT("gym"), -6540.0, -4560.0, 450.0, {0, 0, 0x7f}, 0, 0, 0.0, 0.0, 0, 0.0f},
	{TEXT("MS5"), -7700.0, -6600.0, 2400.0, 4950.0, 1750.0, 0x7a3a2a, 0.0, TEXT("ginos"), 0, 0, 0, {660, 120, 0x7f}, 0xff2a1a, 0, -7150.0, 600.0, 0xffe2c8, 600.0f},
	{TEXT("MS6"), -9600.0, -7700.0, 2400.0, 4950.0, 1350.0, 0x6b3427, -9600.0, nullptr, 0, 0, 0, {-1, 0, 0}, 0, 0, 0.0, 0.0, 0, 0.0f},
	// The north side, fronts on Y 6950 facing south, to Y 8600.
	{TEXT("MN2"), -2800.0, -1500.0, 6950.0, 8600.0, 1750.0, 0x7a3a2a, 0.0, TEXT("halden"), 0, 0, 0, {660, 1140, 0x7f}, 0, 0, -2150.0, 700.0, 0xfff0d8, 600.0f},
	{TEXT("MN3"), -3600.0, -2800.0, 6950.0, 8600.0, 1400.0, 0xb9b2a3, 0.0, TEXT("nails"), 0, 0, 0, {600, 1200, 0x7f}, 0, 0, -3200.0, 500.0, 0xfff4f8, 500.0f},
	{TEXT("MN4"), -4300.0, -3600.0, 6950.0, 8600.0, 1400.0, 0x9c8160, 0.0, TEXT("liquor"), 0, 0, 0, {600, 0, 0x7f}, 0x2a8aff, 0, -3950.0, 500.0, 0xfff0e0, 550.0f},
	{TEXT("MN5"), -6300.0, -4300.0, 6950.0, 8600.0, 2000.0, 0x8a6f5a, 0.0, TEXT("marlowe"), 0, 0, 0, {0, 0, 0x7f}, 0, 0, -5300.0, 400.0, 0xffe0b8, 450.0f},
	{TEXT("MN6"), -7600.0, -6300.0, 6950.0, 8600.0, 900.0, 0x2f2420, 0.0, TEXT("anchor"), 0, 0, 0, {960, 240, 0x7f}, 0xff2a1a, 0, -6950.0, 500.0, 0xffc8a0, 450.0f},
	{TEXT("MN7"), -9600.0, -7600.0, 6950.0, 8600.0, 1450.0, 0x7a3a2a, -9600.0, TEXT("furniture"), 0, 0, 0, {600, 1140, 0x7f}, 0, 0, -8600.0, 900.0, 0xfff2e0, 700.0f},
};

const TCHAR* const ProxyLetters = TEXT("ABCDEFGH");

/** Market's lamps (spec 2.8): south at Y 5340, north at Y 6560, their arms over the road; Cardinal's two on its west sidewalk. */
struct FLamp
{
	double X;
	double Y;
	float Arm;
	bool bLed;
};
const FLamp MarketLamps[] = {
	{-2900.0, 5340.0, 90.0f, true}, {-5650.0, 5340.0, 90.0f, true}, {-8400.0, 5340.0, 90.0f, true}, {-9450.0, 5340.0, 90.0f, true},
	{-1450.0, 6560.0, -90.0f, true}, {-4250.0, 6560.0, -90.0f, true}, {-7050.0, 6560.0, -90.0f, true}, {-9350.0, 6560.0, -90.0f, true},
	{-9660.0, 4200.0, 180.0f, true}, {-9660.0, 7700.0, 180.0f, false},
};
const FVector2D TreePits[] = {{-3300.0, 5310.0}, {-4300.0, 5310.0}, {-5100.0, 5310.0}, {-6000.0, 5310.0}, {-7800.0, 5310.0},
	{-1900.0, 6590.0}, {-3800.0, 6590.0}, {-4800.0, 6590.0}, {-5700.0, 6590.0}, {-6600.0, 6590.0}};

/** The parked cars (spec 2.7): six on the south lane east of the bay, seven on the north lane clear of the return stop, three on Cardinal's far curb. */
struct FParked
{
	double X;
	double Y;
	float Yaw;
};
const FParked Parked[] = {
	{-4300.0, 5505.0, 180.0f}, {-5200.0, 5505.0, 180.0f}, {-6100.0, 5505.0, 180.0f}, {-7000.0, 5505.0, 180.0f}, {-7900.0, 5505.0, 180.0f}, {-8800.0, 5505.0, 180.0f},
	{-900.0, 6395.0, 0.0f}, {-1900.0, 6395.0, 0.0f}, {-4300.0, 6395.0, 0.0f}, {-5300.0, 6395.0, 0.0f}, {-6400.0, 6395.0, 0.0f}, {-7500.0, 6395.0, 0.0f}, {-8600.0, 6395.0, 0.0f},
	{-10915.0, 3000.0, 90.0f}, {-10915.0, 4300.0, 90.0f}, {-10915.0, 7600.0, 90.0f},
};
struct FCarKind
{
	const TCHAR* Name;
	double AxleFront;
	double AxleRear;
	double Track;
	double Hub;
	double Length;
	bool bTinted;
};
const FCarKind CarKinds[] = {
	{TEXT("SM_NS_Hatchback"), -1.30, 1.25, 0.755, 0.312, 4.20, true}, {TEXT("SM_NS_SUV"), -1.42, 1.36, 0.82, 0.37, 4.80, true},
	{TEXT("SM_NS_Cab"), -1.42, 1.38, 0.785, 0.326, 4.85, false},      {TEXT("SM_NS_Van"), -1.86, 1.80, 0.865, 0.345, 5.90, false},
	{TEXT("SM_NS_BoxTruck"), -2.40, 2.30, 0.95, 0.42, 7.50, false},    {TEXT("SM_NS_Pickup"), -1.75, 1.75, 0.85, 0.39, 5.30, true},
};
const uint32 Paints[] = {0xa7abb0, 0x16171a, 0xe8e8e6, 0x5a5d62, 0x6e1b24, 0xe8e8e6, 0x1f3354, 0x16171a, 0xb9a888, 0x5a5d62, 0x2f6f6f, 0xa7abb0, 0x8b1a1a, 0xe8e8e6,
	0x4a3628, 0x16171a};

const TCHAR* const Meshes[] = {TEXT("SM_NS_Ground_Market"), TEXT("SM_NS_Ground_Cardinal"), TEXT("SM_NS_Vista_Library"), TEXT("SM_NS_Vista_Library_Trim"),
	TEXT("SM_NS_Vista_Library_Glass"), TEXT("SM_NS_Vista_Library_GlowWarm"), TEXT("SM_NS_StreetlightLED"), TEXT("SM_NS_StreetlightLED_GlowCool"), TEXT("SM_Streetlight"),
	TEXT("SM_NS_BusShelter"), TEXT("SM_NS_BusShelter_GlowCool"), TEXT("SM_NS_SignPost_BusFlag"), TEXT("SM_NS_SignPost_NoStanding"), TEXT("SM_NS_SignPost_NoParking"),
	TEXT("SM_NS_SignPost_Stop"), TEXT("SM_NS_Bench"), TEXT("SM_NS_Tree_A"), TEXT("SM_NS_Tree_B"), TEXT("SM_NS_TreeGrate"), TEXT("SM_NS_BikeRack"), TEXT("SM_NS_LockedBike"),
	TEXT("SM_NS_LockedBike_B"), TEXT("SM_NS_MuniMeter"), TEXT("SM_Hydrant"), TEXT("SM_TrashCan"), TEXT("SM_NS_CellarDoors"), TEXT("SM_NS_VaultLights"), TEXT("SM_NS_Manhole"),
	TEXT("SM_NS_ManholeVent"), TEXT("SM_NS_CurbInlet"), TEXT("SM_NS_Awning_S"), TEXT("SM_NS_Awning_M"), TEXT("SM_NS_Awning_L"), TEXT("SM_NS_RoadPlate"), TEXT("SM_NS_Cone"),
	TEXT("SM_NS_Barricade"), TEXT("SM_NS_SteamStack"), TEXT("SM_NS_SidewalkShed"), TEXT("SM_NS_SidewalkShed_GlowCool"), TEXT("SM_NS_SidewalkClosure"),
	TEXT("SM_NS_CornerBlades_D"), TEXT("SM_NS_SandwichBoard"), TEXT("SM_NS_CallBox"), TEXT("SM_NS_CallBox_GlowWarm"), TEXT("SM_Sedan")};

FString KitName(const TCHAR* Id, const TCHAR* Part)
{
	return FString::Printf(TEXT("SM_NS_Bldg_%s%s"), Id, Part);
}

AStreetStage* StreetIn(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<AStreetStage> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}
} // namespace MarketStageDetail

using namespace MarketStageDetail;

SHORTSTACK_STREET_PRELOAD(AMarketStage)

AMarketStage::AMarketStage()
{
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Basic(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	CubeMesh = Cube.Object;
	CylinderMesh = Cylinder.Object;
	SphereMesh = Sphere.Object;
	FallbackMaterial = Basic.Object;
}

void AMarketStage::AppendPreload(TArray<FString>& Paths)
{
	for (const TCHAR* Name : Meshes)
	{
		Paths.Add(FStreetPreload::MeshPath(Name));
	}
	for (const FPlot& P : Plots)
	{
		for (const TCHAR* Part : {TEXT(""), TEXT("_Trim"), TEXT("_Glass"), TEXT("_GlowWarm"), TEXT("_GlowCool"), TEXT("_GlowNeon"), TEXT("_Grille")})
		{
			Paths.Add(FStreetPreload::MeshPath(*KitName(P.Id, Part)));
		}
	}
	for (int32 K = 0; K < 8; ++K)
	{
		Paths.Add(FStreetPreload::MeshPath(*FString::Printf(TEXT("SM_NS_Proxy_%c"), ProxyLetters[K])));
	}
	for (const FCarKind& K : CarKinds)
	{
		Paths.Add(FStreetPreload::MeshPath(K.Name));
		Paths.Add(FStreetPreload::MeshPath(*FString::Printf(TEXT("%s_Wheel"), K.Name)));
	}
	Paths.Add(TEXT("/Game/ShortStack/Materials/M_Street.M_Street"));
	Paths.Add(TEXT("/Game/ShortStack/Materials/M_StreetWindow.M_StreetWindow"));
}

// ------------------------------------------------------------------ building

AMarketStage* AMarketStage::FindOrSpawn(UWorld* World, const FTransform& StageFrame, AStreetStage* InStreet)
{
	if (!World)
	{
		return nullptr;
	}
	AMarketStage* M = nullptr;
	for (TActorIterator<AMarketStage> It(World); It && !M; ++It)
	{
		M = *It;
	}
	if (!M)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		M = World->SpawnActor<AMarketStage>(AMarketStage::StaticClass(), StageFrame, Params);
	}
	if (M)
	{
		M->Street = InStreet;
		M->SetActorTransform(StageFrame);
		M->EnsureBuilt();
	}
	return M;
}

void AMarketStage::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UWorld* World = GetWorld();
	if (World && World->IsGameWorld() && bBuilt)
	{
		// Construction run again at play (a property set from the editor's Python): built again, as the street does.
		bBuilt = false;
		EnsureBuilt();
		return;
	}
	if (!World || World->WorldType != EWorldType::Editor)
	{
		return;
	}
	// The editor's preview (at Market's new width only), nothing of it saved.
	Street = StreetIn(World);
	if (bEditorPreview && AStreetStage::WideMarketWanted())
	{
		BuildAll();
	}
}

void AMarketStage::BeginPlay()
{
	Super::BeginPlay();
	EnsureBuilt();
}

void AMarketStage::EndPlay(const EEndPlayReason::Type Reason)
{
	Unregister();
	Screens.Reset();
	Super::EndPlay(Reason);
}

void AMarketStage::Clear()
{
	TInlineComponentArray<UActorComponent*> Old;
	GetComponents(Old);
	Old.RemoveAll([](const UActorComponent* C) { return !C || !C->IsCreatedByConstructionScript(); });
	auto Depth = [](const UActorComponent& C) {
		int32 D = 0;
		for (const USceneComponent* S = Cast<USceneComponent>(&C); S && S->GetAttachParent(); S = S->GetAttachParent())
		{
			++D;
		}
		return D;
	};
	Old.Sort([&Depth](const UActorComponent& A, const UActorComponent& B) { return Depth(A) > Depth(B); });
	for (UActorComponent* C : Old)
	{
		C->DestroyComponent();
	}
	BlueprintCreatedComponents.Reset();
}

void AMarketStage::EnsureBuilt()
{
	if (bBuilt)
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}
	if (!Street.IsValid())
	{
		Street = StreetIn(World);
	}
	if (AStreetStage* S = Street.Get())
	{
		SetActorTransform(S->GetActorTransform());
	}
	const double Started = FPlatformTime::Seconds();
	Clear();
	bBuilt = true;
	// Today's Market (the crowd on its old lines) is the street's own: this stage stands empty then.
	if (!AStreetStage::WideMarketWanted())
	{
		UE_LOG(LogMarketStage, Log, TEXT("Market stays as The Block built it (ss.Street.WideMarket, spec 6.8): its stage builds nothing"));
		return;
	}
	BuildAll();
	SpawnShops();
	Register();
	UE_LOG(LogMarketStage, Log, TEXT("Market built in %.0f ms: %d parts, %d kit plots, %d stand-ins, %d shops, %d lamps, %d parked cars"), (FPlatformTime::Seconds() - Started) * 1000.0,
		BlueprintCreatedComponents.Num(), PlotsKit, PlotsStandIn, ShopsSpawned, Lamps.Num(), CarsParked);
}

void AMarketStage::BuildAll()
{
	Materials.Reset();
	Lamps.Reset();
	Lit.Reset();
	Screens.Reset();
	Emitters.Reset();
	for (TArray<FBox2D>& F : Footprints)
	{
		F.Reset();
	}
	PlotsKit = 0;
	PlotsStandIn = 0;
	CarsParked = 0;
	AppliedNight = -1.0f;
	StreetMaterial = Cast<UMaterialInterface>(AStreetStage::NsAsset(TEXT("/Game/ShortStack/Materials/M_Street.M_Street")));
	WindowMaterial = Cast<UMaterialInterface>(AStreetStage::NsAsset(TEXT("/Game/ShortStack/Materials/M_StreetWindow.M_StreetWindow")));
	BuildGround();
	BuildPlots();
	BuildCardinal();
	BuildLamps();
	BuildStops();
	BuildFurniture();
	BuildParked();
	Lit.Update(WorldMinutes, Levels, 0.0f, true);
}

// ------------------------------------------------------------------ helpers

UMaterialInstanceDynamic* AMarketStage::Mat(FName Key, uint32 SrgbHex, float Roughness, float Pattern, float Emissive, float Metallic)
{
	if (TObjectPtr<UMaterialInstanceDynamic>* Found = Materials.Find(Key))
	{
		return *Found;
	}
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(StreetMaterial ? StreetMaterial.Get() : FallbackMaterial.Get(), this);
	const FLinearColor Color = Ns::Srgb(SrgbHex);
	M->SetVectorParameterValue(TEXT("BaseColor"), Color);
	M->SetVectorParameterValue(TEXT("Color"), Color);
	M->SetScalarParameterValue(TEXT("Roughness"), Roughness);
	M->SetScalarParameterValue(TEXT("Metallic"), Metallic);
	M->SetScalarParameterValue(TEXT("Pattern"), Pattern);
	M->SetScalarParameterValue(TEXT("Emissive"), Emissive);
	M->SetScalarParameterValue(TEXT("Wet"), 0.6f);
	Materials.Add(Key, M);
	return M;
}

UStaticMeshComponent* AMarketStage::Box(UMaterialInterface* Material, const FVector& Min, const FVector& Max, bool bCollide, USceneComponent* Parent)
{
	UStaticMeshComponent* C = NewPart<UStaticMeshComponent>(Parent);
	C->SetStaticMesh(CubeMesh);
	C->SetRelativeLocation((Min + Max) * 0.5);
	C->SetRelativeScale3D((Max - Min) / 100.0);
	C->SetMaterial(0, Material ? Material : FallbackMaterial.Get());
	if (bCollide)
	{
		C->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	}
	else
	{
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	return C;
}

UStaticMeshComponent* AMarketStage::Site(const TCHAR* Name)
{
	UStaticMesh* Mesh = AStreetStage::NsMesh(Name);
	if (!Mesh)
	{
		return nullptr;
	}
	UStaticMeshComponent* C = NewPart<UStaticMeshComponent>();
	C->SetStaticMesh(Mesh);
	C->SetRelativeRotation(Ns::SiteRotation());
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	return C;
}

UStaticMeshComponent* AMarketStage::Prop(const TCHAR* Name, const FVector& At, float Yaw, const FVector& Scale)
{
	UStaticMesh* Mesh = AStreetStage::NsMesh(Name);
	if (!Mesh)
	{
		return nullptr;
	}
	UStaticMeshComponent* C = NewPart<UStaticMeshComponent>();
	C->SetStaticMesh(Mesh);
	// The legacy props (SM_Streetlight, SM_Hydrant, SM_TrashCan) face Blender's -Y; the NS kit's -X.
	const bool bLegacy = !FString(Name).StartsWith(TEXT("SM_NS_"));
	C->SetRelativeLocationAndRotation(At, bLegacy ? BlenderFacing(Yaw) : Ns::PropFacing(Yaw));
	C->SetRelativeScale3D(Scale);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	return C;
}

void AMarketStage::Blocker(const FVector& Min, const FVector& Max)
{
	Box(nullptr, Min, Max)->SetVisibility(false);
}

void AMarketStage::Footprint(int32 Walk, double X, double Y, double HalfX, double HalfY)
{
	if (Walk >= 0 && Walk < 3)
	{
		Footprints[Walk].Add(FBox2D(FVector2D(X - HalfX, Y - HalfY), FVector2D(X + HalfX, Y + HalfY)));
	}
}

void AMarketStage::StandIn(const FVector& Min, const FVector& Max, uint32 Brick, bool bFacesX, bool bFacesY)
{
	Box(Mat(*FString::Printf(TEXT("StandIn%06x"), Brick), Brick, 0.85f, PatBrick), Min, Max);
	if (!WindowMaterial)
	{
		return;
	}
	// A few rows of lit and dark windows on its street faces (M_StreetWindow's instance data: kind, seed, level).
	UInstancedStaticMeshComponent* Set = NewPart<UInstancedStaticMeshComponent>();
	Set->SetStaticMesh(CubeMesh);
	Set->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Set->SetCastShadow(false);
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(WindowMaterial, this);
	M->SetScalarParameterValue(TEXT("Glow"), 1.0f);
	Set->SetMaterial(0, M);
	Set->SetNumCustomDataFloats(3);
	uint32 Seed = static_cast<uint32>(FMath::Abs(Min.X * 7.0 + Min.Y * 13.0)) | 1u;
	auto Row = [&](const FVector& Origin, const FVector& Along, double Length, float Yaw) {
		for (double Z = 420.0; Z + 210.0 < Max.Z; Z += 330.0)
		{
			for (double S = 160.0; S + 100.0 < Length; S += 260.0)
			{
				const float U = Ns::Hash01(Seed++);
				const float Kind = U < 0.2f ? 1.0f : U < 0.26f ? 2.0f : U < 0.3f ? 3.0f : 0.0f;
				const int32 I = Set->AddInstance(FTransform(FRotator(0.0f, Yaw, 0.0f), Origin + Along * S + FVector(0.0, 0.0, Z + 85.0), FVector(0.05, 1.2, 1.7)));
				const float Data[3] = {Kind, Ns::Hash01(Seed++), 0.6f + 0.6f * Ns::Hash01(Seed++)};
				Set->SetCustomData(I, TArrayView<const float>(Data, 3), true);
			}
		}
	};
	if (bFacesX)
	{
		Row(FVector(Max.X + 0.5, Min.Y, 0.0), FVector(0.0, 1.0, 0.0), Max.Y - Min.Y, 0.0f);
		Row(FVector(Min.X - 0.5, Min.Y, 0.0), FVector(0.0, 1.0, 0.0), Max.Y - Min.Y, 180.0f);
	}
	if (bFacesY)
	{
		Row(FVector(Min.X, Max.Y + 0.5, 0.0), FVector(1.0, 0.0, 0.0), Max.X - Min.X, 90.0f);
		Row(FVector(Min.X, Min.Y - 0.5, 0.0), FVector(1.0, 0.0, 0.0), Max.X - Min.X, -90.0f);
	}
}

void AMarketStage::Proxy(const FVector& At, float Yaw, double Width, int32 Seed)
{
	const int32 K = ((Seed % 8) + 8) % 8;
	const bool bMirror = (Seed / 8) % 2 == 1;
	// Fitted to its run (Ns::ProxyPlan), mirrored every other one.
	const double Fit = Width / Ns::ProxyWidths[K];
	if (Prop(*FString::Printf(TEXT("SM_NS_Proxy_%c"), ProxyLetters[K]), At, Yaw, FVector(1.0, (bMirror ? -1.0 : 1.0) * Fit, 1.0)))
	{
		return;
	}
	const FVector Out = FRotator(0.0f, Yaw, 0.0f).Vector();
	const FVector Along = FRotator(0.0f, Yaw + 90.0f, 0.0f).Vector();
	const FVector A = At - Along * Width * 0.5;
	const FVector B = At + Along * Width * 0.5 - Out * Ns::ProxyDepth;
	const double H = 420.0 + 320.0 * (Ns::ProxyFloors[K] - 1) + 80.0;
	const FVector Min(FMath::Min(A.X, B.X), FMath::Min(A.Y, B.Y), 0.0);
	const FVector Max(FMath::Max(A.X, B.X), FMath::Max(A.Y, B.Y), H);
	StandIn(Min, Max, Ns::ProxyBricks[K], FMath::Abs(Out.X) > 0.5, FMath::Abs(Out.Y) > 0.5);
}

void AMarketStage::ProxyRun(const FVector& A, const FVector& B, float Yaw, int32 Seed)
{
	// Fronts side by side filling the building line from A to B exactly (Ns::ProxyPlan).
	const FVector Dir = (B - A).GetSafeNormal();
	for (const Ns::FProxySlot& S : Ns::ProxyPlan((B - A).Size(), Seed))
	{
		Proxy(A + Dir * S.Along, Yaw, S.Width, S.Letter + (S.bMirror ? 8 : 0));
	}
}

URectLightComponent* AMarketStage::Spill(const FVector& At, float Yaw, float Width, uint32 Hex, float Lumens, int32 Shop)
{
	URectLightComponent* L = NewPart<URectLightComponent>();
	L->SetRelativeLocationAndRotation(At, FRotator(-35.0f, Yaw, 0.0f));
	L->SetIntensityUnits(ELightUnits::Lumens);
	L->SetIntensity(Lumens);
	L->SetSourceWidth(Width);
	L->SetSourceHeight(180.0f);
	L->SetAttenuationRadius(900.0f);
	L->SetLightColor(Ns::Srgb(Hex));
	L->SetSpecularScale(0.25f);
	L->SetCastShadows(false);
	L->SetVolumetricScatteringIntensity(0.0f);
	L->MaxDrawDistance = 4000.0f;
	L->MaxDistanceFadeRange = 600.0f;
	L->MarkRenderStateDirty();
	Lit.AddLight(Shop, L, Lumens);
	return L;
}

int32 AMarketStage::AddGlow(UStaticMeshComponent* Mesh, Ns::EGlow Family, uint32 Hex, float Scale, int32 Shop)
{
	if (!Mesh)
	{
		return INDEX_NONE;
	}
	const UStaticMesh* Source = Mesh->GetStaticMesh();
	const bool bKitGlow = Source && Source->GetName().Contains(TEXT("_Glow"));
	UMaterialInterface* Base = bKitGlow ? Cast<UMaterialInterface>(AStreetStage::NsAsset(Ns::GlowPath)) : nullptr;
	if (bKitGlow)
	{
		Mesh->SetCastShadow(false);
	}
	return Lit.AddGlow(this, Mesh, Base, Family, Ns::Srgb(Hex ? Hex : 0xffd9a0), Scale, Shop, Shop >= 0 && Family != Ns::EGlow::Warm);
}

void AMarketStage::Decal(int32 Cell, const FVector& At, float Yaw, const FVector2D& Size)
{
	UMaterialInterface* M = AStreetStage::DecalMaterial(Cell);
	if (!M)
	{
		return;
	}
	UDecalComponent* D = NewPart<UDecalComponent>();
	D->SetDecalMaterial(M);
	D->DecalSize = FVector(12.0, Size.X * 0.5, Size.Y * 0.5);
	D->SetRelativeLocationAndRotation(At + FVector(0.0, 0.0, 2.0), FRotator(-90.0f, Yaw, 0.0f));
	D->SetFadeScreenSize(0.004f);
}

// ------------------------------------------------------------------ the ground

void AMarketStage::BuildGround()
{
	// Market's ground (X 0..-9600 between its building lines: its west end lies before the street's plots) and Cardinal's.
	auto Place = [this](const TCHAR* Name) {
		UStaticMeshComponent* C = Site(Name);
		if (!C)
		{
			return 0;
		}
		C->bAffectDistanceFieldLighting = false;
		const UStaticMesh* Mesh = C->GetStaticMesh();
		const UBodySetup* Body = Mesh ? Mesh->GetBodySetup() : nullptr;
		if (Body && Body->CollisionTraceFlag == CTF_UseComplexAsSimple)
		{
			C->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
			return 2;
		}
		return 1;
	};
	const int32 MarketGround = Place(TEXT("SM_NS_Ground_Market"));
	const int32 CardinalGround = Place(TEXT("SM_NS_Ground_Cardinal"));
	// The stand-ins (visible without the meshes; collision only under a mesh that isn't its own collision yet).
	auto Piece = [&](bool bVisible, float Pattern, uint32 Hex, double X0, double Y0, double X1, double Y1, double Z0, double Z1) {
		UMaterialInterface* M = bVisible ? Mat(*FString::Printf(TEXT("Ground%d"), static_cast<int32>(Pattern)), Hex, Pattern == PatAsphalt ? 0.6f : 0.75f, Pattern) : nullptr;
		UStaticMeshComponent* C = Box(M, FVector(FMath::Min(X0, X1), FMath::Min(Y0, Y1), Z0), FVector(FMath::Max(X0, X1), FMath::Max(Y0, Y1), Z1));
		C->SetVisibility(bVisible);
	};
	if (MarketGround < 2)
	{
		const bool bVisible = MarketGround == 0;
		Piece(bVisible, PatSlabs, 0x6f6c66, Ns::MarketEast, Ns::StoreY1, 0.0, Ns::MarketS - 20.0, -30.0, 0.0);
		Piece(bVisible, PatConcrete, 0x8a867e, Ns::MarketEast, Ns::MarketS - 20.0, 0.0, Ns::MarketS, -30.0, 0.0);
		Piece(bVisible, PatAsphalt, 0x26272a, Ns::CardinalRoad0, Ns::MarketS, 0.0, Ns::MarketN, -45.0, -15.0);
		Piece(bVisible, PatConcrete, 0x8a867e, Ns::MarketEast, Ns::MarketN, 0.0, Ns::MarketN + 20.0, -30.0, 0.0);
		Piece(bVisible, PatSlabs, 0x6f6c66, Ns::MarketEast, Ns::MarketN + 20.0, 0.0, Ns::MarketLineN, -30.0, 0.0);
		if (bVisible)
		{
			UMaterialInterface* Yellow = Mat(TEXT("MarketYellow"), 0xd9a520, 0.55f);
			Box(Yellow, FVector(Ns::MarketEast, Ns::MarketMidY - 18.0, -15.0), FVector(-120.0, Ns::MarketMidY - 8.0, -14.6), false)->SetCastShadow(false);
			Box(Yellow, FVector(Ns::MarketEast, Ns::MarketMidY + 8.0, -15.0), FVector(-120.0, Ns::MarketMidY + 18.0, -14.6), false)->SetCastShadow(false);
			Box(Mat(TEXT("BusBay"), 0x6a2a24, 0.7f, PatAsphalt), FVector(-3500.0, Ns::MarketS, -15.0), FVector(-1500.0, 5640.0, -14.6), false)->SetCastShadow(false);
		}
	}
	if (CardinalGround < 2)
	{
		const bool bVisible = CardinalGround == 0;
		// As far as the modelled ground (ns_ground.py's Far), into the rows of fronts across each end.
		const double Y0 = Ns::CardinalSouth - 300.0;
		const double Y1 = Ns::CardinalNorth + 300.0;
		Piece(bVisible, PatSlabs, 0x6f6c66, Ns::MarketEast, Y0, Ns::CardinalRoad0 + 20.0, Ns::StoreY1, -30.0, 0.0);
		Piece(bVisible, PatSlabs, 0x6f6c66, Ns::MarketEast, Ns::MarketLineN, Ns::CardinalRoad0 + 20.0, Y1, -30.0, 0.0);
		Piece(bVisible, PatSlabs, 0x6f6c66, Ns::MarketEast, Ns::StoreY1, Ns::CardinalRoad0 + 20.0, Ns::MarketS - 20.0, -30.0, 0.0);
		Piece(bVisible, PatSlabs, 0x6f6c66, Ns::MarketEast, Ns::MarketN + 20.0, Ns::CardinalRoad0 + 20.0, Ns::MarketLineN, -30.0, 0.0);
		Piece(bVisible, PatConcrete, 0x8a867e, Ns::CardinalRoad0 + 20.0, Y0, Ns::CardinalRoad0, Ns::MarketS, -30.0, 0.0);
		Piece(bVisible, PatConcrete, 0x8a867e, Ns::CardinalRoad0 + 20.0, Ns::MarketN, Ns::CardinalRoad0, Y1, -30.0, 0.0);
		Piece(bVisible, PatAsphalt, 0x26272a, Ns::CardinalRoad0, Y0, Ns::CardinalRoad1, Y1, -45.0, -15.0);
		Piece(bVisible, PatConcrete, 0x8a867e, Ns::CardinalRoad1, Y0, Ns::CardinalRoad1 - 20.0, Y1, -30.0, 0.0);
		Piece(bVisible, PatSlabs, 0x6f6c66, Ns::CardinalRoad1 - 20.0, Y0, Ns::CardinalE, Y1, -30.0, 0.0);
	}
	// The castings in their recesses (spec 4.1): the manholes on Market's crown (the steam stack's vented), the inlets, the vault lights.
	struct FCast
	{
		const TCHAR* Mesh;
		double X;
		double Y;
		float Yaw;
		bool bRoad;
	};
	const FCast Casts[] = {
		{TEXT("SM_NS_Manhole"), -1100.0, 5800.0, 0.0f, true},    {TEXT("SM_NS_Manhole"), -4300.0, 6100.0, 40.0f, true},
		{TEXT("SM_NS_ManholeVent"), -5200.0, 6000.0, 0.0f, true}, {TEXT("SM_NS_Manhole"), -8200.0, 5800.0, 110.0f, true},
		{TEXT("SM_NS_CurbInlet"), -1300.0, Ns::MarketS, 90.0f, false}, {TEXT("SM_NS_CurbInlet"), -6400.0, Ns::MarketS, 90.0f, false},
		{TEXT("SM_NS_CurbInlet"), -700.0, Ns::MarketN, -90.0f, false}, {TEXT("SM_NS_CurbInlet"), -9300.0, Ns::MarketN, -90.0f, false},
		{TEXT("SM_NS_VaultLights"), -1700.0, 5050.0, 90.0f, false},    {TEXT("SM_NS_VaultLights"), -3900.0, 6850.0, -90.0f, false},
	};
	for (const FCast& C : Casts)
	{
		const double Z = C.bRoad ? Ns::RoadZ(FMath::Abs(C.Y - Ns::MarketMidY), Ns::MarketN - Ns::MarketS) - 0.2 : 0.0;
		Prop(C.Mesh, FVector(C.X, C.Y, Z), C.Yaw);
	}
}

// ------------------------------------------------------------------ the plots

void AMarketStage::BuildPlots()
{
	for (int32 I = 0; I < static_cast<int32>(UE_ARRAY_COUNT(Plots)); ++I)
	{
		BuildPlot(I);
	}
}

void AMarketStage::BuildPlot(int32 Index)
{
	const FPlot& P = Plots[Index];
	const bool bSouth = P.Y1 <= Ns::StoreY1 + 1.0;
	const double FaceY = bSouth ? Ns::StoreY1 : Ns::MarketLineN;
	const float OutYaw = bSouth ? 90.0f : -90.0f;
	// Its lights by its shop's real hours (core's), the gym's window all night (members on the fob, spec 2.3).
	const bool bShopStage = P.OpenZ > 0.0;
	const int32 Shop = Lit.AddShop(P.Venue && FCString::Strcmp(P.Venue, TEXT("gym")) != 0 ? P.Venue : nullptr, P.Hours);
	const AStreetStage* S = Street.Get();
	if (Site(*KitName(P.Id, TEXT(""))))
	{
		++PlotsKit;
		if (UStaticMeshComponent* Trim = Site(*KitName(P.Id, TEXT("_Trim"))))
		{
			Lit.Shops[Shop].Trim = Trim;
			Lit.Shops[Shop].TrimSlot = ImportedSlot(Trim, 3);
		}
		Site(*KitName(P.Id, TEXT("_Glass")));
		AddGlow(Site(*KitName(P.Id, TEXT("_GlowWarm"))), Ns::EGlow::Warm, 0xffc98a, 1.0f, Shop);
		AddGlow(Site(*KitName(P.Id, TEXT("_GlowCool"))), Ns::EGlow::Cool, P.CoolHex ? P.CoolHex : 0xe8f0ff, 1.0f, Shop);
		AddGlow(Site(*KitName(P.Id, TEXT("_GlowNeon"))), Ns::EGlow::Neon, P.NeonHex ? P.NeonHex : 0xff4060, 1.0f, Shop);
		const FString GrilleName = KitName(P.Id, TEXT("_Grille"));
		const FVector* Pivot = S ? S->Places.Find(FName(*GrilleName)) : nullptr;
		UStaticMesh* GrilleMesh = Pivot ? AStreetStage::NsMesh(*GrilleName) : nullptr;
		if (GrilleMesh)
		{
			UStaticMeshComponent* G = NewPart<UStaticMeshComponent>();
			G->SetStaticMesh(GrilleMesh);
			G->SetRelativeLocationAndRotation(*Pivot, Ns::SiteRotation());
			G->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Lit.Shops[Shop].Grille = G;
		}
	}
	else
	{
		++PlotsStandIn;
		StandIn(FVector(P.X0, P.Y0, 0.0), FVector(P.X1, P.Y1, P.Height), P.Brick, P.CardinalX != 0.0, true);
		if (!bShopStage && P.Hours.Opens >= 0)
		{
			// Its shop window lit while it's open, the grille's hood over it.
			const double Y = bSouth ? FaceY + 2.0 : FaceY - 6.0;
			UMaterialInstanceDynamic* Display = Mat(*FString::Printf(TEXT("MarketDisplay%d"), Index), P.SpillHex ? P.SpillHex : 0xffd9a0, 0.6f);
			UStaticMeshComponent* Window = Box(Display, FVector(P.X0 + 150.0, Y, 40.0), FVector(P.X1 - 150.0, Y + 4.0, 300.0), false);
			Window->SetCastShadow(false);
			Lit.AddGlow(this, Window, nullptr, Ns::EGlow::Cool, Ns::Srgb(P.SpillHex ? P.SpillHex : 0xffd9a0), 1.0f, Shop, true);
		}
	}
	// Its face blocks (the kit's meshes don't collide), round a shop stage's opening (its stage's door and glass block there);
	// its Cardinal face too.
	const double B0 = bSouth ? FaceY - 40.0 : FaceY;
	const double B1 = bSouth ? FaceY : FaceY + 40.0;
	if (bShopStage)
	{
		Blocker(FVector(P.X0, B0, 0.0), FVector(P.OpenX0, B1, P.Height));
		Blocker(FVector(P.OpenX1, B0, 0.0), FVector(P.X1, B1, P.Height));
		Blocker(FVector(P.OpenX0, B0, P.OpenZ), FVector(P.OpenX1, B1, P.Height));
		if (!AShopStage::ClassFor(P.Venue))
		{
			// Its stage isn't there yet: the opening's grille down.
			Box(Mat(TEXT("Shutter"), 0x5d6166, 0.5f, PatShutter, 0.0f, 0.6f), FVector(P.OpenX0, bSouth ? FaceY : FaceY - 4.0, 0.0), FVector(P.OpenX1, bSouth ? FaceY + 4.0 : FaceY, P.OpenZ));
		}
	}
	else
	{
		Blocker(FVector(P.X0, B0, 0.0), FVector(P.X1, B1, P.Height));
	}
	if (P.CardinalX != 0.0)
	{
		Blocker(FVector(P.CardinalX - 40.0, P.Y0, 0.0), FVector(P.CardinalX, P.Y1, P.Height));
	}
	if (P.SpillWidth > 0.0)
	{
		Spill(FVector(P.SpillX, FaceY + (bSouth ? 70.0 : -70.0), 260.0), OutYaw, static_cast<float>(P.SpillWidth), P.SpillHex, P.SpillLumens, Shop);
	}
	// Its neon's or lightbox cross's own light (spec 3.1), in its color, while it's lit: Gino's SLICE, the liquor store's
	// LIQUORS, the Anchor's red script, the pharmacy's green cross.
	if ((P.NeonHex || P.CoolHex) && P.SpillWidth > 0.0)
	{
		URectLightComponent* L = NewPart<URectLightComponent>();
		L->SetRelativeLocationAndRotation(FVector(P.SpillX, FaceY + (bSouth ? 45.0 : -45.0), 320.0), FRotator(-12.0f, OutYaw, 0.0f));
		L->SetIntensityUnits(ELightUnits::Lumens);
		L->SetIntensity(300.0f);
		L->SetSourceWidth(P.NeonHex ? 220.0f : 90.0f);
		L->SetSourceHeight(P.NeonHex ? 50.0f : 90.0f);
		L->SetAttenuationRadius(700.0f);
		L->SetLightColor(Ns::Srgb(P.NeonHex ? P.NeonHex : P.CoolHex));
		L->SetSpecularScale(0.5f);
		L->SetCastShadows(false);
		L->SetVolumetricScatteringIntensity(0.4f);
		L->MaxDrawDistance = 4000.0f;
		L->MaxDistanceFadeRange = 600.0f;
		Lit.AddLight(Shop, L, 300.0f);
	}
}

// ------------------------------------------------------------------ Cardinal: the library, the proxies, the walk limits

void AMarketStage::BuildCardinal()
{
	// The Northside Branch Library across Cardinal, closing the view down Market (spec 2.3): its lamps at night, its reading
	// room's windows (M_NS_WindowRoom's shop back rooms: lit 10:00-20:00).
	const int32 Library = Lit.AddShop({600, 1200, 0x3f});
	if (Site(TEXT("SM_NS_Vista_Library")))
	{
		++PlotsKit;
		if (UStaticMeshComponent* Trim = Site(TEXT("SM_NS_Vista_Library_Trim")))
		{
			Lit.Shops[Library].Trim = Trim;
			Lit.Shops[Library].TrimSlot = ImportedSlot(Trim, 3);
		}
		Site(TEXT("SM_NS_Vista_Library_Glass"));
		AddGlow(Site(TEXT("SM_NS_Vista_Library_GlowWarm")), Ns::EGlow::Warm, 0xffd9a8, 1.2f, -1);
	}
	else
	{
		++PlotsStandIn;
		UMaterialInterface* Limestone = Mat(TEXT("Limestone"), 0xbdb4a2, 0.8f, PatConcrete);
		Box(Limestone, FVector(Ns::CardinalE - 2200.0, 4750.0, 0.0), FVector(Ns::CardinalE, 7150.0, 1500.0));
		UMaterialInstanceDynamic* Room = Mat(TEXT("LibraryRoom"), 0xffd8a0, 0.6f);
		for (double Y = 5000.0; Y < 7000.0; Y += 330.0)
		{
			UStaticMeshComponent* W = Box(Room, FVector(Ns::CardinalE, Y, 300.0), FVector(Ns::CardinalE + 4.0, Y + 150.0, 900.0), false);
			W->SetCastShadow(false);
			Lit.AddGlow(this, W, nullptr, Ns::EGlow::Warm, Ns::Srgb(0xffd8a0), 1.0f, Library, true);
		}
	}
	// C-S and C-N as proxies (spec 0.10), then Cardinal's fronts out to where its ground ends in the haze (ns_ground.py's Far:
	// Y -50..170 m), a row across it closing each end. Its west side runs on across Hollis's and Wexler's lines: those fronts'
	// backs close the views east down both (their own rows stop there, AStreetStage::BuildCrossStreets).
	Proxy(FVector(Ns::CardinalE, 3575.0, 0.0), 0.0f, 2350.0, 2);
	Proxy(FVector(Ns::CardinalE, 8375.0, 0.0), 0.0f, 2450.0, 13);
	ProxyRun(FVector(Ns::CardinalE, 2400.0, 0.0), FVector(Ns::CardinalE, Ns::CardinalSouth, 0.0), 0.0f, 907);
	ProxyRun(FVector(Ns::CardinalE, 9600.0, 0.0), FVector(Ns::CardinalE, Ns::CardinalNorth, 0.0), 0.0f, 1009);
	ProxyRun(FVector(Ns::MarketEast, 2400.0, 0.0), FVector(Ns::MarketEast, Ns::HollisS, 0.0), 180.0f, 1103);
	ProxyRun(FVector(Ns::MarketEast, 8600.0, 0.0), FVector(Ns::MarketEast, Ns::CardinalNorth, 0.0), 180.0f, 1201);
	ProxyRun(FVector(Ns::MarketEast, Ns::CardinalSouth, 0.0), FVector(Ns::CardinalE, Ns::CardinalSouth, 0.0), 90.0f, 1301);
	ProxyRun(FVector(Ns::MarketEast, Ns::CardinalNorth, 0.0), FVector(Ns::CardinalE, Ns::CardinalNorth, 0.0), -90.0f, 1409);
	// The walk limits on Cardinal's near sidewalk (spec 2.9) and its near curb blocked between them.
	UMaterialInterface* Fence = Mat(TEXT("Fence"), 0x9aa0a6, 0.4f, 0.0f, 0.0f, 0.8f);
	for (const double Y : {Ns::CardinalWalk0, Ns::CardinalWalk1})
	{
		const float Facing = Y < Ns::MarketMidY ? 90.0f : -90.0f;
		const double Mid = (Ns::MarketEast + Ns::CardinalRoad0) * 0.5;
		bool bKit = false;
		for (const double Off : {-100.0, 100.0})
		{
			bKit |= Prop(TEXT("SM_NS_SidewalkClosure"), FVector(Mid + Off, Y, 0.0), Facing) != nullptr;
		}
		if (!bKit)
		{
			Box(Fence, FVector(Ns::CardinalRoad0, Y - 3.0, 0.0), FVector(Ns::MarketEast, Y + 3.0, 200.0), false);
		}
		const FVector Toward = FRotator(0.0f, Facing, 0.0f).Vector();
		Prop(TEXT("SM_NS_Barricade"), FVector(Mid, Y, 0.0) + Toward * 120.0, Facing);
		Prop(TEXT("SM_NS_Cone"), FVector(Mid - 120.0, Y, 0.0) + Toward * 70.0, Facing + 25.0f);
		Blocker(FVector(Ns::CardinalRoad0, Y - 20.0, 0.0), FVector(Ns::MarketEast, Y + 20.0, 300.0));
	}
	Blocker(FVector(Ns::CardinalRoad0 - 40.0, Ns::CardinalWalk0, -20.0), FVector(Ns::CardinalRoad0, Ns::CardinalWalk1, 300.0));
	Prop(TEXT("SM_NS_CornerBlades_D"), FVector(Ns::MarketEast - 40.0, Ns::StoreY1 + 90.0, 0.0), 0.0f);
}

// ------------------------------------------------------------------ the lamps, the stops

void AMarketStage::BuildLamps()
{
	// Market's eight LED cobra heads (4000 K, 1800 cd, a tighter cone), Cardinal's two (one of each era); every one shadowed
	// (MegaLights), a quarter of it in the fog, its lens glowing at night.
	UMaterialInterface* Pole = Mat(TEXT("Pole"), 0x2c3033, 0.5f, 0.0f, 0.0f, 0.7f);
	for (const FLamp& L : MarketLamps)
	{
		const FVector Out = FRotator(0.0f, L.Arm, 0.0f).Vector();
		FVector Head;
		if (L.bLed)
		{
			Head = FVector(L.X, L.Y, 0.0) + Out * 215.0 + FVector(0.0, 0.0, 885.0);
			if (Prop(TEXT("SM_NS_StreetlightLED"), FVector(L.X, L.Y, 0.0), L.Arm))
			{
				AddGlow(Prop(TEXT("SM_NS_StreetlightLED_GlowCool"), FVector(L.X, L.Y, 0.0), L.Arm), Ns::EGlow::Cool, LedHex, 3.0f, -1);
			}
			else
			{
				Box(Pole, FVector(L.X - 9.0, L.Y - 9.0, 0.0), FVector(L.X + 9.0, L.Y + 9.0, 900.0));
				UMaterialInstanceDynamic* Lens = Mat(TEXT("LedLens"), 0xfff4e8, 0.3f);
				UStaticMeshComponent* G = Box(Lens, Head + FVector(-25.0, -25.0, -2.0), Head + FVector(25.0, 25.0, 3.0), false);
				Lit.AddGlow(this, G, nullptr, Ns::EGlow::Cool, Ns::Srgb(LedHex), 3.0f, -1, false);
			}
		}
		else
		{
			Head = FVector(L.X, L.Y, 0.0) + Out * 150.0 + FVector(0.0, 0.0, 790.0);
			if (UStaticMeshComponent* C = Prop(TEXT("SM_Streetlight"), FVector(L.X, L.Y, 0.0), L.Arm))
			{
				AddGlow(C, Ns::EGlow::Warm, HpsHex, 1.0f, -1);
			}
			else
			{
				Box(Pole, FVector(L.X - 9.0, L.Y - 9.0, 0.0), FVector(L.X + 9.0, L.Y + 9.0, 820.0));
			}
		}
		Blocker(FVector(L.X - 12.0, L.Y - 12.0, 0.0), FVector(L.X + 12.0, L.Y + 12.0, 400.0));
		Footprint(L.Y < Ns::MarketMidY && L.X > Ns::MarketEast ? 0 : (L.X > Ns::MarketEast ? 1 : 2), L.X, L.Y, 20.0, 20.0);
		USpotLightComponent* S = NewPart<USpotLightComponent>();
		S->SetRelativeLocationAndRotation(Head, FRotator(-90.0f, 0.0f, 0.0f));
		S->SetIntensityUnits(ELightUnits::Candelas);
		S->SetIntensity(L.bLed ? 1800.0f : 1400.0f);
		S->SetAttenuationRadius(2400.0f);
		S->SetOuterConeAngle(L.bLed ? 50.0f : 62.0f);
		S->SetInnerConeAngle(L.bLed ? 30.0f : 26.0f);
		S->SetSourceRadius(L.bLed ? 8.0f : 12.0f);
		S->SetLightColor(Ns::Srgb(L.bLed ? LedHex : HpsHex));
		S->SetVolumetricScatteringIntensity(0.25f);
		S->SetCastShadows(true);
		S->SetCastVolumetricShadow(false);
		Lamps.Add(S);
	}
}

void AMarketStage::BuildStops()
{
	// The 14's stops (spec 2.6): the eastbound shelter facing the bay (its name keeps "Shelter": the crowd waits at it), the
	// flag where the bus's front door stops, NO STANDING at the bay; the westbound return's flag and bench on the north side.
	if (UStaticMeshComponent* Shelter = Prop(TEXT("SM_NS_BusShelter"), FVector(-2250.0, 5180.0, 0.0), 90.0f))
	{
		Shelter->SetCastShadow(true);
		// Its screens from core: the timetable (town::BusTimes' next departures, drawn again by the minute) and the lightbox ad.
		Screens.Paint(this, Shelter, TEXT("ns_screen_timetable"), TEXT("timetable:market-east"), 512, Ns::EScreen::Backlit);
		Screens.Paint(this, Shelter, TEXT("ns_screen_ad"), TEXT("shelterad"), 512, Ns::EScreen::Backlit);
		AddGlow(Prop(TEXT("SM_NS_BusShelter_GlowCool"), FVector(-2250.0, 5180.0, 0.0), 90.0f), Ns::EGlow::Cool, 0xeaf2ff, 1.4f, -1);
	}
	else
	{
		// A stand-in: posts, a roof, a glass back.
		UMaterialInterface* Steel = Mat(TEXT("ShelterSteel"), 0x9aa0a6, 0.4f, 0.0f, 0.0f, 0.8f);
		for (const double X : {-2470.0, -2030.0})
		{
			Box(Steel, FVector(X - 5.0, 5100.0, 0.0), FVector(X + 5.0, 5110.0, 260.0));
			Box(Steel, FVector(X - 5.0, 5250.0, 0.0), FVector(X + 5.0, 5260.0, 260.0));
		}
		Box(Steel, FVector(-2480.0, 5100.0, 255.0), FVector(-2020.0, 5265.0, 265.0), false);
		Box(Mat(TEXT("ShelterGlass"), 0x9fb7c8, 0.05f, 0.0f, 0.0f, 0.2f), FVector(-2470.0, 5104.0, 20.0), FVector(-2030.0, 5106.0, 240.0));
	}
	Blocker(FVector(-2480.0, 5098.0, 0.0), FVector(-2020.0, 5112.0, 260.0));
	Blocker(FVector(-2482.0, 5100.0, 0.0), FVector(-2470.0, 5260.0, 260.0));
	Footprint(0, -2250.0, 5180.0, 235.0, 85.0);
	Emitters.Add({FVector(-2250.0, 5180.0, 260.0), EStreetEmitter::RainRoof, 0.9f, 0.0f});
	Prop(TEXT("SM_NS_SignPost_BusFlag"), FVector(-1700.0, 5340.0, 0.0), 0.0f);
	Prop(TEXT("SM_NS_SignPost_NoStanding"), FVector(-2900.0, 5350.0, 0.0), 0.0f);
	Prop(TEXT("SM_NS_SignPost_BusFlag"), FVector(-3200.0, 6560.0, 0.0), 180.0f);
	Prop(TEXT("SM_NS_Bench"), FVector(-3200.0, 6680.0, 0.0), -90.0f);
	for (const FVector2D& At : {FVector2D(-1700.0, 5340.0), FVector2D(-2900.0, 5350.0), FVector2D(-3200.0, 6560.0)})
	{
		Blocker(FVector(At.X - 5.0, At.Y - 5.0, 0.0), FVector(At.X + 5.0, At.Y + 5.0, 300.0));
		Footprint(At.Y < Ns::MarketMidY ? 0 : 1, At.X, At.Y, 10.0, 10.0);
	}
	Footprint(1, -3200.0, 6680.0, 90.0, 30.0);
}

// ------------------------------------------------------------------ the furniture, the cars

void AMarketStage::BuildFurniture()
{
	// The trees in their pits (as Fifth's: rigid shadows, sway stopped past 40 m, their leaves out of ray tracing).
	int32 K = 0;
	for (const FVector2D& Pit : TreePits)
	{
		const float Yaw = 53.0f * static_cast<float>(K) + 7.0f;
		if (UStaticMeshComponent* Tree = Prop(K % 2 == 0 ? TEXT("SM_NS_Tree_B") : TEXT("SM_NS_Tree_A"), FVector(Pit.X, Pit.Y, -3.0), Yaw))
		{
			Tree->ShadowCacheInvalidationBehavior = EShadowCacheInvalidationBehavior::Rigid;
			Tree->SetWorldPositionOffsetDisableDistance(4000);
			Tree->bAffectDistanceFieldLighting = false;
			Tree->MarkRenderStateDirty();
		}
		Prop(TEXT("SM_NS_TreeGrate"), FVector(Pit.X, Pit.Y, -0.5), 0.0f);
		Blocker(FVector(Pit.X - 18.0, Pit.Y - 18.0, 0.0), FVector(Pit.X + 18.0, Pit.Y + 18.0, 300.0));
		Footprint(Pit.Y < Ns::MarketMidY ? 0 : 1, Pit.X, Pit.Y, 60.0, 60.0);
		Decal(3 + K % 2, FVector(Pit.X + 90.0, Pit.Y, 0.0), Yaw * 2.0f, FVector2D(120.0, 120.0));
		++K;
	}
	// Bike racks along the curb (in front of the pharmacy, the thrift, the gym, Ruby's), two with a locked bike.
	struct FThing
	{
		const TCHAR* Mesh;
		double X;
		double Y;
		float Yaw;
		double Half;
	};
	const FThing Things[] = {
		{TEXT("SM_NS_BikeRack"), -2200.0, 5300.0, -90.0f, 45.0},  {TEXT("SM_NS_BikeRack"), -3300.0, 5300.0, -90.0f, 45.0},
		{TEXT("SM_NS_BikeRack"), -5000.0, 5300.0, -90.0f, 45.0},  {TEXT("SM_NS_BikeRack"), -700.0, 6600.0, -90.0f, 45.0},
		{TEXT("SM_Hydrant"), -3550.0, 5340.0, 90.0f, 25.0},       {TEXT("SM_Hydrant"), -9450.0, 6560.0, -90.0f, 25.0},
		{TEXT("SM_TrashCan"), -1550.0, 5330.0, 20.0f, 30.0},      {TEXT("SM_TrashCan"), -2600.0, 5300.0, -10.0f, 30.0},
		{TEXT("SM_TrashCan"), -9300.0, 5330.0, 40.0f, 30.0},      {TEXT("SM_TrashCan"), -1550.0, 6570.0, 190.0f, 30.0},
		{TEXT("SM_TrashCan"), -5600.0, 6570.0, 170.0f, 30.0},     {TEXT("SM_TrashCan"), -9250.0, 6570.0, 200.0f, 30.0},
		{TEXT("SM_NS_MuniMeter"), -4600.0, 5350.0, -90.0f, 20.0}, {TEXT("SM_NS_MuniMeter"), -7300.0, 5350.0, -90.0f, 20.0},
		{TEXT("SM_NS_MuniMeter"), -2400.0, 6550.0, 90.0f, 20.0},  {TEXT("SM_NS_MuniMeter"), -6000.0, 6550.0, 90.0f, 20.0},
		// The fire-alarm call box at Market and Cardinal (spec 2.8), at the building corner, clear of the walk.
		{TEXT("SM_NS_CallBox"), -9480.0, 5060.0, 90.0f, 20.0},
	};
	for (const FThing& T : Things)
	{
		if (Prop(T.Mesh, FVector(T.X, T.Y, 0.0), T.Yaw))
		{
			Blocker(FVector(T.X - T.Half * 0.6, T.Y - T.Half * 0.6, 0.0), FVector(T.X + T.Half * 0.6, T.Y + T.Half * 0.6, 110.0));
			Footprint(T.Y < Ns::MarketMidY ? 0 : 1, T.X, T.Y, T.Half, T.Half);
			if (FCString::Strcmp(T.Mesh, TEXT("SM_NS_CallBox")) == 0)
			{
				AddGlow(Prop(TEXT("SM_NS_CallBox_GlowWarm"), FVector(T.X, T.Y, 0.0), T.Yaw), Ns::EGlow::Warm, 0xffd9a0, 0.6f, -1);
			}
		}
	}
	Prop(TEXT("SM_NS_LockedBike"), FVector(-3300.0, 5300.0, 0.0) + FRotator(0.0f, -90.0f, 0.0f).Vector() * 6.0, -90.0f);
	Prop(TEXT("SM_NS_LockedBike_B"), FVector(-5000.0, 5300.0, 0.0) + FRotator(0.0f, -90.0f, 0.0f).Vector() * 6.0, -90.0f);
	// Cellar doors in front of the pharmacy and the liquor store.
	Prop(TEXT("SM_NS_CellarDoors"), FVector(-2150.0, Ns::StoreY1 + 80.0, 0.4), 90.0f);
	Prop(TEXT("SM_NS_CellarDoors"), FVector(-4150.0, Ns::MarketLineN - 80.0, 0.4), -90.0f);
	// The awnings (spec 2.8): the pharmacy's, the thrift's, Gino's, the liquor store's, Halden & Sons' green, Ruby's red on
	// her Market face; their valances carry the shops' names ([screen]).
	struct FAwning
	{
		const TCHAR* Mesh;
		double X;
		double Y;
		double Z;
		float Yaw;
		uint32 Fabric;
		const TCHAR* Venue;
	};
	const FAwning Awnings[] = {
		{TEXT("SM_NS_Awning_M"), -1900.0, Ns::StoreY1, 380.0, 90.0f, 0x2f7a4a, TEXT("pharmacy")},
		{TEXT("SM_NS_Awning_L"), -3050.0, Ns::StoreY1, 430.0, 90.0f, 0x2a4f8a, TEXT("thrift")},
		{TEXT("SM_NS_Awning_S"), -7150.0, Ns::StoreY1, 380.0, 90.0f, 0xb3262a, TEXT("ginos")},
		{TEXT("SM_NS_Awning_S"), -3950.0, Ns::MarketLineN, 380.0, -90.0f, 0x5a1e2a, TEXT("liquor")},
		{TEXT("SM_NS_Awning_M"), -2150.0, Ns::MarketLineN, 380.0, -90.0f, 0x1f4a33, TEXT("halden")},
		{TEXT("SM_NS_Awning_L"), -820.0, Ns::MarketLineN, 330.0, -90.0f, 0xb01c22, TEXT("diner")},
	};
	// The drips off the awnings' valances and the shed's deck in the rain (M_NS_RainFx's drip lines, spec 3.3).
	UInstancedStaticMeshComponent* DripSet = nullptr;
	if (UMaterialInterface* Fx = Cast<UMaterialInterface>(AStreetStage::NsAsset(Ns::RainFxPath)))
	{
		UMaterialInstanceDynamic* Drip = UMaterialInstanceDynamic::Create(Fx, this);
		Drip->SetScalarParameterValue(TEXT("Mode"), 1.0f);
		DripSet = NewPart<UInstancedStaticMeshComponent>();
		DripSet->SetStaticMesh(CylinderMesh);
		DripSet->SetMaterial(0, Drip);
		DripSet->SetNumCustomDataFloats(2);
		DripSet->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		DripSet->SetCastShadow(false);
		DripSet->SetVisibleInRayTracing(false);
		DripSet->bAffectDistanceFieldLighting = false;
		DripSet->SetCullDistances(0, 3000);
		Drips = DripSet;
	}
	for (const FAwning& A : Awnings)
	{
		if (UStaticMeshComponent* C = Prop(A.Mesh, FVector(A.X, A.Y, A.Z), A.Yaw))
		{
			TintImported(C, ImportedSlot(C, 0), Ns::Srgb(A.Fabric));
			Screens.Paint(this, C, TEXT("ns_screen_valance"), FString::Printf(TEXT("valance:%s"), A.Venue), 1024, Ns::EScreen::Printed, Ns::Srgb(A.Fabric));
			Emitters.Add({FVector(A.X, A.Y + (A.Yaw > 0.0f ? 80.0 : -80.0), A.Z), EStreetEmitter::RainRoof, 0.6f, 0.0f});
			const double Span = FCString::Strstr(A.Mesh, TEXT("_L")) ? 900.0 : FCString::Strstr(A.Mesh, TEXT("_S")) ? 300.0 : 600.0;
			const FVector Out = FRotator(0.0f, A.Yaw, 0.0f).Vector();
			const FVector Along = FRotator(0.0f, A.Yaw + 90.0f, 0.0f).Vector();
			const FVector Edge = FVector(A.X, A.Y, A.Z - 80.0) + Out * 160.0;
			AStreetStage::AddDrips(DripSet, GetActorTransform(), Edge - Along * Span * 0.5, Edge + Along * Span * 0.5, 0.0, 32.0);
		}
	}
	// The utility cut in the westbound lane (spec 2.8): its plates, cones round it, a barricade at its head; the steam stack
	// over its vent; the sidewalk shed along Cardinal Furniture (its bulkheads lit at night).
	const double PlateZ = Ns::RoadZ(155.0, Ns::MarketN - Ns::MarketS) + 2.5;
	for (const double X : {-4575.0, -4725.0, -4875.0, -5025.0})
	{
		Prop(TEXT("SM_NS_RoadPlate"), FVector(X, 6105.0, PlateZ), 0.0f);
	}
	for (const FVector2D& C : {FVector2D(-4440.0, 5990.0), FVector2D(-4440.0, 6220.0), FVector2D(-5160.0, 5990.0), FVector2D(-5160.0, 6220.0), FVector2D(-4800.0, 5965.0)})
	{
		Prop(TEXT("SM_NS_Cone"), FVector(C.X, C.Y, Ns::RoadZ(FMath::Abs(C.Y - Ns::MarketMidY), Ns::MarketN - Ns::MarketS)), 0.0f);
	}
	Prop(TEXT("SM_NS_Barricade"), FVector(-4360.0, 6105.0, Ns::RoadZ(155.0, Ns::MarketN - Ns::MarketS)), 0.0f);
	if (Prop(TEXT("SM_NS_SteamStack"), FVector(-5200.0, 6000.0, Ns::RoadZ(50.0, Ns::MarketN - Ns::MarketS)), 0.0f))
	{
		Emitters.Add({FVector(-5200.0, 6000.0, 300.0), EStreetEmitter::Steam, 0.8f, 0.0f});
	}
	for (const double X : {-7900.0, -8500.0, -9100.0})
	{
		if (Prop(TEXT("SM_NS_SidewalkShed"), FVector(X, Ns::MarketN + 190.0, 0.0), -90.0f))
		{
			AddGlow(Prop(TEXT("SM_NS_SidewalkShed_GlowCool"), FVector(X, Ns::MarketN + 190.0, 0.0), -90.0f), Ns::EGlow::Cool, 0xeaf2ff, 1.0f, -1);
			Emitters.Add({FVector(X, Ns::MarketN + 320.0, 310.0), EStreetEmitter::RainRoof, 0.7f, 0.0f});
			// Its deck's curb-side edge drips (the module 6 m along Market).
			AStreetStage::AddDrips(DripSet, GetActorTransform(), FVector(X - 300.0, Ns::MarketN + 190.0, 310.0), FVector(X + 300.0, Ns::MarketN + 190.0, 310.0), 0.0, 40.0);
		}
	}
	// Signs: NO PARKING along both curbs, STOP before Cardinal.
	for (const FVector& S : {FVector(-4500.0, 5350.0, 0.0), FVector(-7600.0, 5350.0, 0.0), FVector(-3000.0, 6550.0, 180.0), FVector(-6200.0, 6550.0, 180.0),
			 FVector(-8900.0, 6550.0, 180.0)})
	{
		Prop(TEXT("SM_NS_SignPost_NoParking"), FVector(S.X, S.Y, 0.0), static_cast<float>(S.Z));
		Blocker(FVector(S.X - 5.0, S.Y - 5.0, 0.0), FVector(S.X + 5.0, S.Y + 5.0, 300.0));
	}
	Prop(TEXT("SM_NS_SignPost_Stop"), FVector(-9500.0, 5350.0, 0.0), 0.0f);
	// Positioned sounds: the signs' hum (the pharmacy's cross, Gino's, the liquor store's and the Anchor's neon), AC units,
	// downspouts in the rain.
	Emitters.Add({FVector(-1900.0, Ns::StoreY1 + 20.0, 420.0), EStreetEmitter::Hum, 0.6f, 120.0f});
	Emitters.Add({FVector(-7150.0, Ns::StoreY1 + 20.0, 330.0), EStreetEmitter::Hum, 0.6f, 119.0f});
	Emitters.Add({FVector(-3950.0, Ns::MarketLineN - 20.0, 330.0), EStreetEmitter::Hum, 0.6f, 121.0f});
	Emitters.Add({FVector(-6950.0, Ns::MarketLineN - 20.0, 350.0), EStreetEmitter::Hum, 0.7f, 118.0f});
	Emitters.Add({FVector(-2600.0, Ns::MarketLineN - 10.0, 900.0), EStreetEmitter::Ac, 0.6f, 60.0f});
	Emitters.Add({FVector(-7300.0, Ns::StoreY1 + 10.0, 1050.0), EStreetEmitter::Ac, 0.6f, 58.0f});
	Emitters.Add({FVector(-3700.0, Ns::StoreY1 + 5.0, 30.0), EStreetEmitter::Gutter, 0.7f, 0.0f});
	Emitters.Add({FVector(-6300.0, Ns::MarketLineN - 5.0, 30.0), EStreetEmitter::Gutter, 0.7f, 0.0f});
	// Gum at the shelter and the doors, butts by the cans.
	Decal(2, FVector(-2250.0, 5320.0, 0.0), 30.0f, FVector2D(180.0, 180.0));
	Decal(2, FVector(-3050.0, 5050.0, 0.0), 110.0f, FVector2D(140.0, 140.0));
	Decal(5, FVector(-1520.0, 5280.0, 0.0), 0.0f, FVector2D(90.0, 90.0));
	Decal(6, FVector(-5650.0, 6620.0, 0.0), 200.0f, FVector2D(100.0, 100.0));
}

void AMarketStage::BuildParked()
{
	// The car kit in the street's paints, its wheels at its axles, oil under each, a box keeping the hero out (spec 2.7, 3.6).
	TArray<int32> Kinds;
	for (int32 I = 0; I < static_cast<int32>(UE_ARRAY_COUNT(CarKinds)); ++I)
	{
		if (AStreetStage::NsMesh(CarKinds[I].Name))
		{
			Kinds.Add(I);
		}
	}
	UMaterialInterface* CarGlass = Mat(TEXT("CarGlass"), 0x0b0e12, 0.05f, 0.0f, 0.0f, 0.4f);
	for (int32 I = 0; I < static_cast<int32>(UE_ARRAY_COUNT(Parked)); ++I)
	{
		const FParked& P = Parked[I];
		const uint32 Paint = Paints[I % UE_ARRAY_COUNT(Paints)];
		const bool bCardinal = P.X < Ns::CardinalRoad0;
		const double Z = bCardinal ? Ns::RoadZ(FMath::Abs(P.X - Ns::CardinalMidX), 1000.0) : Ns::RoadZ(FMath::Abs(P.Y - Ns::MarketMidY), Ns::MarketN - Ns::MarketS);
		const FVector At(P.X, P.Y, Z);
		double Length = 480.0;
		bool bPlaced = false;
		if (Kinds.Num() > 0)
		{
			const FCarKind& K = CarKinds[Kinds[(I * 7 + 3) % Kinds.Num()]];
			USceneComponent* Car = NewPart<USceneComponent>();
			Car->SetRelativeLocationAndRotation(At, BlenderFacing(P.Yaw));
			UStaticMeshComponent* Body = NewPart<UStaticMeshComponent>(Car);
			Body->SetStaticMesh(AStreetStage::NsMesh(K.Name));
			Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			if (K.bTinted)
			{
				TintImported(Body, ImportedSlot(Body, 0), Ns::Srgb(Paint));
			}
			Screens.Paint(this, Body, TEXT("ns_plate"), TEXT("plate:") + Ns::PlateFor(2, I), 256, Ns::EScreen::Printed);
			if (UStaticMesh* Wheel = AStreetStage::NsMesh(*FString::Printf(TEXT("%s_Wheel"), K.Name)))
			{
				for (int32 W = 0; W < 4; ++W)
				{
					const double Side = W % 2 == 0 ? 1.0 : -1.0;
					UStaticMeshComponent* Wc = NewPart<UStaticMeshComponent>(Car);
					Wc->SetStaticMesh(Wheel);
					Wc->SetRelativeLocationAndRotation(FVector(100.0 * Side * K.Track, -100.0 * (W < 2 ? K.AxleFront : K.AxleRear), 100.0 * K.Hub),
						FRotator(0.0f, Side > 0.0 ? 0.0f : 180.0f, 0.0f));
					Wc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				}
			}
			Length = K.Length * 100.0;
			bPlaced = true;
		}
		else if (UStaticMeshComponent* Sedan = Prop(TEXT("SM_Sedan"), At, P.Yaw))
		{
			TintImported(Sedan, ImportedSlot(Sedan, 0), Ns::Srgb(Paint));
			Screens.Paint(this, Sedan, TEXT("ns_plate"), TEXT("plate:") + Ns::PlateFor(2, I), 256, Ns::EScreen::Printed);
			bPlaced = true;
		}
		const FVector Fwd = FRotator(0.0f, P.Yaw, 0.0f).Vector();
		const FVector Rt = FRotator(0.0f, P.Yaw + 90.0f, 0.0f).Vector();
		const FVector Ext = (Fwd * Length * 0.5).GetAbs() + (Rt * 90.0).GetAbs();
		if (!bPlaced)
		{
			Box(Mat(*FString::Printf(TEXT("Car%06x"), Paint), Paint, 0.25f, 0.0f, 0.0f, 0.6f), At - FVector(Ext.X, Ext.Y, -10.0), At + FVector(Ext.X, Ext.Y, 105.0));
			const FVector Cab = (Fwd * Length * 0.22).GetAbs() + (Rt * 75.0).GetAbs();
			Box(CarGlass, At - FVector(Cab.X, Cab.Y, -105.0), At + FVector(Cab.X, Cab.Y, 150.0));
		}
		else
		{
			Blocker(At - FVector(Ext.X, Ext.Y, -5.0), At + FVector(Ext.X, Ext.Y, 140.0));
		}
		Decal(I % 3 == 0 ? 1 : 0, At, P.Yaw + 90.0f, FVector2D(260.0, 150.0));
		++CarsParked;
	}
}

// ------------------------------------------------------------------ the shops, the registry

void AMarketStage::SpawnShops()
{
	// Market's three enterable shops at their anchors (spec 7.5.6): the thrift, the pawn, the gym (ue-frame's stages).
	UWorld* World = GetWorld();
	ShopsSpawned = 0;
	for (const TCHAR* Venue : {TEXT("thrift"), TEXT("pawn"), TEXT("gym")})
	{
		if (AShopStage::ClassFor(Venue) && AShopStage::FindOrSpawnVenue(World, Venue, GetActorTransform()))
		{
			++ShopsSpawned;
		}
	}
}

void AMarketStage::Register()
{
	UStreetWorld* Registry = UStreetWorld::Get(this);
	if (!Registry)
	{
		return;
	}
	const FTransform& T = GetActorTransform();
	Unregister();
	auto Zone = [&](const TCHAR* Id, const FBox& Local, const TCHAR* Place, int32 Priority) {
		FStreetZone Z;
		Z.Id = Id;
		Z.Box = Local.TransformBy(T);
		Z.Place = Place;
		Z.Priority = Priority;
		Z.RoomTone = StreetRoomTone::Street;
		Registry->AddZone(Z);
	};
	Zone(TEXT("market"), FBox(FVector(Ns::CardinalRoad0, Ns::StoreY1 - 50.0, -200.0), FVector(-100.0, Ns::MarketLineN + 50.0, 100000.0)), TEXT("MARKET STREET"), 5);
	Zone(TEXT("cardinal"), FBox(FVector(Ns::CardinalRoad0 - 30.0, Ns::CardinalWalk0, -200.0), FVector(Ns::MarketEast + 300.0, Ns::CardinalWalk1, 100000.0)), TEXT("CARDINAL AVE"), 6);
	// Market's two sidewalks and Cardinal's near one, each to its ends (spec 6.8: the crowd's map from the registry).
	auto Walk = [&](FName Street, int32 Side, const FVector& A, const FVector& B, int32 Prints) {
		FStreetWalkway W;
		W.Street = Street;
		W.Side = Side;
		W.Z = T.TransformPosition(FVector::ZeroVector).Z;
		FStreetLane L;
		L.Start = T.TransformPosition(A);
		L.End = T.TransformPosition(B);
		W.Lanes.Add(L);
		for (const FBox2D& F : Footprints[Prints])
		{
			const FVector P0 = T.TransformPosition(FVector(F.Min, 0.0));
			const FVector P1 = T.TransformPosition(FVector(F.Max, 0.0));
			W.Obstacles.Add(FBox2D(FVector2D(FMath::Min(P0.X, P1.X), FMath::Min(P0.Y, P1.Y)), FVector2D(FMath::Max(P0.X, P1.X), FMath::Max(P0.Y, P1.Y))));
		}
		Registry->AddWalkway(W);
	};
	Walk(TEXT("Market"), 0, FVector(-150.0, 5160.0, 0.0), FVector(Ns::MarketEast + 150.0, 5160.0, 0.0), 0);
	Walk(TEXT("Market"), 1, FVector(-150.0, 6740.0, 0.0), FVector(Ns::MarketEast + 150.0, 6740.0, 0.0), 1);
	Walk(TEXT("Cardinal"), 1, FVector(-9810.0, Ns::CardinalWalk0 + 100.0, 0.0), FVector(-9810.0, Ns::CardinalWalk1 - 100.0, 0.0), 2);
	// The starts (spec 2.10): at the shelter facing the curb; Cardinal's walk limits' middle, facing the library.
	Registry->AddStart(TEXT("market-stop"), FTransform(FRotator(0.0f, 90.0f, 0.0f), FVector(-2250.0, 5320.0, 100.0)) * T);
	Registry->AddStart(TEXT("cardinal"), FTransform(FRotator(0.0f, 180.0f, 0.0f), FVector(-9810.0, Ns::MarketMidY, 100.0)) * T);
	// Where people wait for the 14 under the shelter (covered), and the return stop's bench.
	for (int32 K = 0; K < 4; ++K)
	{
		Registry->AddStation(*FString::Printf(TEXT("market-shelter%d"), K + 1), TEXT("market-stop"), FTransform(FRotator(0.0f, 90.0f, 0.0f), FVector(-2400.0 + 100.0 * K, 5170.0, 0.0)) * T,
			TEXT("Wait"), 0.0f, true);
	}
	Registry->AddStation(TEXT("market-bench"), TEXT("market-stop"), FTransform(FRotator(0.0f, -90.0f, 0.0f), FVector(-3200.0, 6680.0, 0.0)) * T, TEXT("Sit"), 45.0f, false);
	// The shelter's use: wait for the 14 (a skip to its next arrival, ue-frame's Wait verb).
	FStreetUse U;
	U.Id = TEXT("market-shelter");
	U.Verb = EStreetVerb::Wait;
	U.Venue = TEXT("market-stop");
	U.Param = TEXT("market-east");
	U.Prompt = TEXT("Wait for the 14");
	U.Radius = 220.0f;
	U.Anchor = Root.Get();
	U.Offset = FVector(-2250.0, 5200.0, 100.0);
	UseHandles.Add(Registry->AddUse(U));
}

void AMarketStage::Unregister()
{
	if (UStreetWorld* Registry = UStreetWorld::Get(this))
	{
		for (const int32 H : UseHandles)
		{
			Registry->RemoveUse(H);
		}
	}
	UseHandles.Reset();
}

// ------------------------------------------------------------------ at play

void AMarketStage::SetLook(double InWorldMinutes, const Ns::FGlowLevels& InLevels, float Dt)
{
	WorldMinutes = InWorldMinutes;
	Levels = InLevels;
	if (!bBuilt)
	{
		return;
	}
	// The drips only while it rains or the street is still wet.
	if (UInstancedStaticMeshComponent* D = Drips.Get())
	{
		const AStreetStage* S = Street.Get();
		const bool bDrip = S && (S->WeatherNow().Rain > 0.01 || S->WeatherNow().Wetness > 0.4);
		if (D->IsVisible() != bDrip)
		{
			D->SetVisibility(bDrip);
		}
	}
	Lit.Update(WorldMinutes, Levels, Dt, false);
	Screens.Update(WorldMinutes, Levels);
	// The lamps' photocells.
	if (FMath::Abs(Levels.Night - AppliedNight) > 0.02f)
	{
		AppliedNight = Levels.Night;
		int32 I = 0;
		for (USpotLightComponent* L : Lamps)
		{
			if (L)
			{
				const bool bOn = Levels.Night > 0.35f + 0.04f * static_cast<float>(I % 4);
				L->SetVisibility(bOn);
			}
			++I;
		}
	}
}

void AMarketStage::AppendEmitters(TArray<FStreetEmitter>& Out, const FVector& Camera, float Rain, float Night) const
{
	const FTransform& T = GetActorTransform();
	for (int32 I = 0; I < Emitters.Num(); ++I)
	{
		const FSpot& S = Emitters[I];
		const FVector At = T.TransformPosition(S.At);
		if (FVector::DistSquared(At, Camera) > 4000.0 * 4000.0)
		{
			continue;
		}
		float Level = S.Level;
		if (S.Kind == EStreetEmitter::Hum)
		{
			Level *= Night;
		}
		else if (S.Kind == EStreetEmitter::Gutter || S.Kind == EStreetEmitter::RainRoof)
		{
			Level *= FMath::SmoothStep(0.05f, 0.6f, Rain);
		}
		if (Level <= 0.01f)
		{
			continue;
		}
		FStreetEmitter E;
		E.At = At;
		E.Kind = S.Kind;
		E.Level = Level;
		E.Hz = S.Hz;
		E.Id = 3000u + static_cast<uint32>(I);
		Out.Add(E);
	}
}

FString AMarketStage::Describe() const
{
	return FString::Printf(TEXT("Market: %s, %d kit plots, %d stand-ins, %d shops spawned, %d lamps, %d parked cars, %d shops lit by the hour"), bBuilt ? TEXT("built") : TEXT("not built"),
		PlotsKit, PlotsStandIn, ShopsSpawned, Lamps.Num(), CarsParked, Lit.Shops.Num());
}
