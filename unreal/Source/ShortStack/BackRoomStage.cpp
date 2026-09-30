#include "BackRoomStage.h"

#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace BackRoomDetail
{
FLinearColor Srgb(uint32 Hex)
{
	return FLinearColor::FromSRGBColor(FColor(static_cast<uint8>((Hex >> 16) & 0xff), static_cast<uint8>((Hex >> 8) & 0xff), static_cast<uint8>(Hex & 0xff)));
}

UMaterialInterface* LoadOptional(const TCHAR* Path)
{
	return LoadObject<UMaterialInterface>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
}

/** A prop imported by the editor setup script: /Game/ShortStack/Meshes/<Name>/<Name>. */
UStaticMesh* LoadProp(const TCHAR* Name)
{
	return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/ShortStack/Meshes/%s/%s.%s"), Name, Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
}

// The room (cm): the table in the middle, dryers along the far wall behind the dealer.
const double X0 = -330.0, X1 = 390.0;
const double Y0 = -300.0, Y1 = 320.0;
const double Ceiling = 285.0;
const double Wall = 20.0;
// The door to the laundromat, in the right-hand wall, standing ajar.
const double DoorX0 = 110.0, DoorX1 = 202.0, DoorTop = 212.0;

// art/blender/assets/table.py SEATS (meters, Blender axes) as Unreal centimeters: Blender (x, y) is
// Unreal (y, x) once the table is turned to face the player down +X (import flips Y, then yaw 90).
const FVector2D Seats[8] = {
	{-61.0, 0.0}, {-61.0, 64.0}, {0.0, 122.0}, {61.0, 64.0}, {61.0, 0.0}, {61.0, -64.0}, {0.0, -122.0}, {-61.0, -64.0},
};
// The table's straight run: the ends are half circles around (0, +/-HalfL).
const double HalfL = 61.0;
} // namespace BackRoomDetail

using namespace BackRoomDetail;

ABackRoomStage::ABackRoomStage()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Basic(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	CubeMesh = Cube.Object;
	CylinderMesh = Cylinder.Object;
	SphereMesh = Sphere.Object;
	ConeMesh = Cone.Object;
	FallbackMaterial = Basic.Object;
}

// ------------------------------------------------------------------ layout

FVector ABackRoomStage::SeatEdge(int32 Index)
{
	const FVector2D& S = Seats[FMath::Clamp(Index, 0, 7)];
	return FVector(S.X, S.Y, FeltZ);
}

FTransform ABackRoomStage::SeatTransform(int32 Index)
{
	const FVector Edge = SeatEdge(Index);
	// Outward from the nearest point of the table's middle line (x = 0, y in [-HalfL, HalfL]).
	const FVector Core(0.0, FMath::Clamp(Edge.Y, -HalfL, HalfL), Edge.Z);
	FVector Out = (Edge - Core).GetSafeNormal2D();
	if (Out.IsNearlyZero())
	{
		Out = FVector(-1.0, 0.0, 0.0);
	}
	const FVector Chair = Edge + Out * 38.0;
	return FTransform((-Out).Rotation(), FVector(Chair.X, Chair.Y, 0.0));
}

FVector ABackRoomStage::EyeLocation() const
{
	// Seated, leaning in a little: 30 cm back from the rail, eyes 1.18 m up.
	return GetActorTransform().TransformPosition(FVector(Seats[0].X - 30.0, 0.0, 118.0));
}

// ------------------------------------------------------------------ building blocks

template <typename T>
T* ABackRoomStage::NewPart(USceneComponent* Parent)
{
	T* Part = NewObject<T>(this);
	Part->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	Part->SetupAttachment(Parent ? Parent : Root.Get());
	BlueprintCreatedComponents.Add(Part);
	Part->RegisterComponent();
	return Part;
}

UStaticMeshComponent* ABackRoomStage::AddMesh(UStaticMesh* Mesh, UMaterialInterface* Material, const FVector& Location, const FVector& Scale, const FRotator& Rotation, USceneComponent* Parent, bool bShadows)
{
	UStaticMeshComponent* C = NewPart<UStaticMeshComponent>(Parent);
	C->SetStaticMesh(Mesh);
	C->SetRelativeLocationAndRotation(Location, Rotation);
	C->SetRelativeScale3D(Scale);
	if (Material)
	{
		C->SetMaterial(0, Material);
	}
	C->SetCastShadow(bShadows);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	return C;
}

UStaticMeshComponent* ABackRoomStage::Box(UMaterialInterface* Material, const FVector& Min, const FVector& Max, bool bShadows)
{
	// The engine cube is 100 cm on a side, centered on its pivot.
	return AddMesh(CubeMesh, Material, (Min + Max) * 0.5, (Max - Min) / 100.0, FRotator::ZeroRotator, nullptr, bShadows);
}

UMaterialInstanceDynamic* ABackRoomStage::Room(FName Key, int32 Pattern, uint32 SrgbHex, uint32 SrgbHex2, float Split, float Metallic)
{
	if (TObjectPtr<UMaterialInstanceDynamic>* Found = Materials.Find(Key))
	{
		return *Found;
	}
	UMaterialInterface* Parent = RoomMaterial ? RoomMaterial.Get() : (SurfaceMaterial ? SurfaceMaterial.Get() : FallbackMaterial.Get());
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Parent, this);
	M->SetVectorParameterValue(TEXT("BaseColor"), Srgb(SrgbHex));
	M->SetVectorParameterValue(TEXT("BaseColor2"), Srgb(SrgbHex2 ? SrgbHex2 : SrgbHex));
	M->SetVectorParameterValue(TEXT("Color"), Srgb(SrgbHex)); // BasicShapeMaterial fallback
	M->SetScalarParameterValue(TEXT("Split"), Split);
	M->SetScalarParameterValue(TEXT("Pattern"), static_cast<float>(Pattern));
	M->SetScalarParameterValue(TEXT("Metallic"), Metallic);
	M->SetScalarParameterValue(TEXT("Roughness"), 0.8f); // M_Surface fallback
	Materials.Add(Key, M);
	return M;
}

UMaterialInstanceDynamic* ABackRoomStage::Glow(FName Key, const FLinearColor& Color, float Strength)
{
	if (TObjectPtr<UMaterialInstanceDynamic>* Found = Materials.Find(Key))
	{
		return *Found;
	}
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(SurfaceMaterial ? SurfaceMaterial.Get() : FallbackMaterial.Get(), this);
	M->SetVectorParameterValue(TEXT("BaseColor"), Color);
	M->SetVectorParameterValue(TEXT("Color"), Color);
	M->SetScalarParameterValue(TEXT("Roughness"), 0.4f);
	M->SetScalarParameterValue(TEXT("Emissive"), Strength);
	Materials.Add(Key, M);
	return M;
}

// ------------------------------------------------------------------ construction

void ABackRoomStage::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildSet();
}

void ABackRoomStage::RebuildSet()
{
#if WITH_EDITOR
	RerunConstructionScripts();
#endif
}

void ABackRoomStage::BuildSet()
{
	Materials.Reset();
	RoomMaterial = LoadOptional(TEXT("/Game/ShortStack/Materials/M_Room.M_Room"));
	SurfaceMaterial = LoadOptional(TEXT("/Game/ShortStack/Materials/M_Surface.M_Surface"));
	TableMesh = LoadProp(TEXT("SM_PokerTable"));
	ChipsMesh = LoadProp(TEXT("SM_ChipStacks"));
	BuildShell();
	BuildTable();
	BuildMachines();
	BuildLights();
	BuildAir();
}

void ABackRoomStage::BuildShell()
{
	// Two-tone institutional paint: hospital green below the line, nicotine cream above.
	UMaterialInterface* Walls = Room(TEXT("Walls"), 1, 0xc9bf9f, 0x5d7a63, 1.22f);
	UMaterialInterface* Floor = Room(TEXT("Floor"), 2, 0x77726a);
	UMaterialInterface* Ceil = Room(TEXT("Ceiling"), 3, 0xcfc9b8);
	const double T = Wall;
	Box(Floor, FVector(X0 - T, Y0 - T, -T), FVector(X1 + T, Y1 + T, 0.0));
	Box(Ceil, FVector(X0 - T, Y0 - T, Ceiling), FVector(X1 + T, Y1 + T, Ceiling + T));
	Box(Walls, FVector(X1, Y0 - T, 0.0), FVector(X1 + T, Y1 + T, Ceiling));      // far, behind the dryers
	Box(Walls, FVector(X0 - T, Y0 - T, 0.0), FVector(X0, Y1 + T, Ceiling));      // behind the player
	Box(Walls, FVector(X0, Y0 - T, 0.0), FVector(X1, Y0, Ceiling));              // left
	// Right wall, with the doorway to the laundromat.
	Box(Walls, FVector(X0, Y1, 0.0), FVector(DoorX0, Y1 + T, Ceiling));
	Box(Walls, FVector(DoorX1, Y1, 0.0), FVector(X1, Y1 + T, Ceiling));
	Box(Walls, FVector(DoorX0, Y1, DoorTop), FVector(DoorX1, Y1 + T, Ceiling));
	// Beyond the door: the laundromat's bright white floor and a wall of washers, just glimpsed.
	UMaterialInterface* Beyond = Room(TEXT("Laundromat"), 4, 0xe7e9e4);
	Box(Beyond, FVector(DoorX0 - 150.0, Y1 + T, -T), FVector(DoorX1 + 150.0, Y1 + T + 320.0, 0.0));
	Box(Beyond, FVector(DoorX0 - 150.0, Y1 + T + 320.0, 0.0), FVector(DoorX1 + 150.0, Y1 + T + 340.0, Ceiling));
	Box(Room(TEXT("LaundromatCeiling"), 3, 0xf0efe8), FVector(DoorX0 - 150.0, Y1 + T, Ceiling), FVector(DoorX1 + 150.0, Y1 + T + 340.0, Ceiling + T));
	// The steel door, swung open into the room.
	UMaterialInterface* DoorPaint = Room(TEXT("Door"), 4, 0x6f6a5c, 0, 0.0f, 0.2f);
	const FVector Hinge(DoorX0, Y1, 0.0);
	USceneComponent* Door = NewPart<USceneComponent>();
	Door->SetRelativeLocationAndRotation(Hinge, FRotator(0.0f, -68.0f, 0.0f));
	AddMesh(CubeMesh, DoorPaint, FVector((DoorX1 - DoorX0) * 0.5, -2.5, DoorTop * 0.5), FVector((DoorX1 - DoorX0) / 100.0, 0.045, DoorTop / 100.0), FRotator::ZeroRotator, Door);
	AddMesh(CubeMesh, Room(TEXT("Steel"), 4, 0x9a9b98, 0, 0.0f, 0.9f), FVector((DoorX1 - DoorX0) * 0.72, -6.0, 104.0), FVector(0.6, 0.03, 0.04), FRotator::ZeroRotator, Door);

	// Pipes and the dryers' exhaust duct along the ceiling.
	UMaterialInterface* PipePaint = Room(TEXT("Pipes"), 4, 0x8c8a80, 0, 0.0f, 0.3f);
	UMaterialInterface* Duct = Room(TEXT("Duct"), 4, 0xa7a9a6, 0, 0.0f, 0.85f);
	AddMesh(CylinderMesh, Duct, FVector(X1 - 45.0, 0.0, Ceiling - 32.0), FVector(0.3, 0.3, (Y1 - Y0) / 100.0), FRotator(0.0f, 0.0f, 90.0f));
	AddMesh(CylinderMesh, PipePaint, FVector(0.0, Y0 + 22.0, Ceiling - 18.0), FVector(0.07, 0.07, (X1 - X0) / 100.0), FRotator(90.0f, 0.0f, 0.0f));
	AddMesh(CylinderMesh, PipePaint, FVector(0.0, Y0 + 36.0, Ceiling - 14.0), FVector(0.045, 0.045, (X1 - X0) / 100.0), FRotator(90.0f, 0.0f, 0.0f));
	// A floor drain under the table's left end, where the floor slopes.
	AddMesh(CylinderMesh, Room(TEXT("Drain"), 4, 0x2a2724, 0, 0.0f, 0.8f), FVector(40.0, -150.0, 0.15), FVector(0.22, 0.22, 0.004));
}

void ABackRoomStage::BuildTable()
{
	if (TableMesh)
	{
		// art/blender/assets/table.py: origin on the floor under the middle, the player's side toward
		// Blender -Y (Unreal +Y after import); turned so the player sits at -X.
		AddMesh(TableMesh, nullptr, FVector::ZeroVector, FVector(1.0), FRotator(0.0f, 90.0f, 0.0f));
	}
	else
	{
		UMaterialInterface* Felt = Room(TEXT("FeltStandIn"), 0, 0x245a3b);
		UMaterialInterface* Rail = Room(TEXT("RailStandIn"), 0, 0x2b1210);
		Box(Felt, FVector(-50.0, -110.0, FeltZ - 4.0), FVector(50.0, 110.0, FeltZ));
		Box(Rail, FVector(-61.0, -122.0, FeltZ - 4.0), FVector(61.0, 122.0, FeltZ - 1.0));
	}
	if (ChipsMesh)
	{
		// Stacks at a few seats for scale until the game places real stacks.
		for (int32 Seat : {1, 3, 5, 6})
		{
			const FVector Edge = SeatEdge(Seat);
			const FVector In = (FVector(0.0, FMath::Clamp(Edge.Y, -HalfL, HalfL), FeltZ) - Edge).GetSafeNormal2D();
			AddMesh(ChipsMesh, nullptr, Edge + In * 26.0, FVector(1.0), FRotator(0.0f, In.Rotation().Yaw + 90.0f, 0.0f));
		}
	}
}

void ABackRoomStage::BuildMachines()
{
	// Stand-ins until the Blender machines exist: a bank of stacked dryers behind the dealer.
	UMaterialInterface* Enamel = Room(TEXT("DryerEnamel"), 4, 0xd8d2bf, 0, 0.0f, 0.0f);
	UMaterialInterface* Chrome = Room(TEXT("DryerChrome"), 4, 0xb9bab7, 0, 0.0f, 0.9f);
	UMaterialInterface* Glass = Room(TEXT("DryerGlass"), 4, 0x1b1d1f, 0, 0.0f, 0.1f);
	for (int32 I = 0; I < 6; ++I)
	{
		const double Y = -235.0 + I * 86.0;
		Box(Enamel, FVector(X1 - 84.0, Y - 41.0, 0.0), FVector(X1 - 4.0, Y + 41.0, 196.0));
		for (double Z : {50.0, 146.0})
		{
			AddMesh(CylinderMesh, Chrome, FVector(X1 - 85.0, Y, Z), FVector(0.62, 0.62, 0.03), FRotator(90.0f, 0.0f, 0.0f));
			AddMesh(CylinderMesh, Glass, FVector(X1 - 86.0, Y, Z), FVector(0.52, 0.52, 0.02), FRotator(90.0f, 0.0f, 0.0f));
		}
	}
	// A washer and a folding table along the left wall; shelves of detergent.
	UMaterialInterface* Laminate = Room(TEXT("Laminate"), 4, 0xb8ac8c, 0, 0.0f, 0.0f);
	Box(Laminate, FVector(-120.0, Y0 + 5.0, 72.0), FVector(90.0, Y0 + 80.0, 76.0));
	for (double X : {-110.0, 80.0})
	{
		Box(Chrome, FVector(X - 2.0, Y0 + 10.0, 0.0), FVector(X + 2.0, Y0 + 12.0, 72.0));
		Box(Chrome, FVector(X - 2.0, Y0 + 73.0, 0.0), FVector(X + 2.0, Y0 + 75.0, 72.0));
	}
	// Behind the player: two front-load washers.
	for (int32 I = 0; I < 2; ++I)
	{
		const double Y = -80.0 + I * 75.0;
		Box(Enamel, FVector(X0 + 4.0, Y - 35.0, 0.0), FVector(X0 + 74.0, Y + 35.0, 110.0));
	}
}

void ABackRoomStage::BuildLights()
{
	// The poker lamp: a green enamel shade hanging low over the felt, a warm bulb inside.
	const FVector LampAt(0.0, 0.0, FeltZ + 108.0);
	UMaterialInterface* Enamel = Room(TEXT("ShadeEnamel"), 4, 0x1f4a32, 0, 0.0f, 0.0f);
	AddMesh(ConeMesh, Enamel, LampAt + FVector(0.0, 0.0, 12.0), FVector(0.46, 0.46, 0.24), FRotator::ZeroRotator, nullptr, true);
	AddMesh(CylinderMesh, Room(TEXT("Cord"), 4, 0x111111), LampAt + FVector(0.0, 0.0, (Ceiling - LampAt.Z) * 0.5 + 12.0), FVector(0.008, 0.008, (Ceiling - LampAt.Z - 24.0) / 100.0));
	AddMesh(SphereMesh, Glow(TEXT("Bulb"), Srgb(0xffd9a0), 40.0f), LampAt + FVector(0.0, 0.0, -3.0), FVector(0.07), FRotator::ZeroRotator, nullptr, false);
	LampLight = NewPart<USpotLightComponent>();
	LampLight->SetRelativeLocationAndRotation(LampAt + FVector(0.0, 0.0, -2.0), FRotator(-90.0f, 0.0f, 0.0f));
	LampLight->SetIntensityUnits(ELightUnits::Candelas);
	LampLight->SetIntensity(LampCandela);
	LampLight->SetLightColor(Srgb(0xffc98f));
	LampLight->SetOuterConeAngle(62.0f);
	LampLight->SetInnerConeAngle(34.0f);
	LampLight->SetSourceRadius(4.0f);
	LampLight->SetSoftSourceRadius(6.0f);
	LampLight->SetAttenuationRadius(900.0f);
	LampLight->SetVolumetricScatteringIntensity(1.6f);
	LampLight->SetCastShadows(true);
	// The light leaking out of the shade's top vents onto the ceiling.
	ShadeGlow = NewPart<UPointLightComponent>();
	ShadeGlow->SetRelativeLocation(LampAt + FVector(0.0, 0.0, 30.0));
	ShadeGlow->SetIntensityUnits(ELightUnits::Candelas);
	ShadeGlow->SetIntensity(4.0f);
	ShadeGlow->SetLightColor(Srgb(0xffc98f));
	ShadeGlow->SetAttenuationRadius(260.0f);
	ShadeGlow->SetCastShadows(false);

	// A fluorescent strip over the dryers: one tube dead, the other failing.
	const FVector TubeAt(X1 - 120.0, -60.0, Ceiling - 6.0);
	Box(Room(TEXT("Fixture"), 4, 0xcfcfc8, 0, 0.0f, 0.2f), TubeAt + FVector(-9.0, -62.0, 0.0), TubeAt + FVector(9.0, 62.0, 6.0));
	TubeGlow = Glow(TEXT("Tube"), Srgb(0xe6f0ff), 30.0f);
	AddMesh(CylinderMesh, TubeGlow, TubeAt + FVector(4.0, 0.0, -1.5), FVector(0.028, 0.028, 1.2), FRotator(0.0f, 0.0f, 90.0f), nullptr, false);
	AddMesh(CylinderMesh, Room(TEXT("DeadTube"), 4, 0xbfc2c4), TubeAt + FVector(-4.0, 0.0, -1.5), FVector(0.028, 0.028, 1.2), FRotator(0.0f, 0.0f, 90.0f), nullptr, false);
	Fluorescent = NewPart<URectLightComponent>();
	Fluorescent->SetRelativeLocationAndRotation(TubeAt + FVector(0.0, 0.0, -4.0), FRotator(-90.0f, 0.0f, 0.0f));
	Fluorescent->SetIntensityUnits(ELightUnits::Candelas);
	Fluorescent->SetIntensity(FluorescentCandela);
	Fluorescent->SetLightColor(Srgb(0xdce9ff));
	Fluorescent->SetSourceWidth(120.0f);
	Fluorescent->SetSourceHeight(6.0f);
	Fluorescent->SetBarnDoorAngle(80.0f);
	Fluorescent->SetAttenuationRadius(900.0f);
	Fluorescent->SetVolumetricScatteringIntensity(0.6f);
	Fluorescent->SetCastShadows(true);

	// The laundromat's lights, spilling through the open door.
	DoorSpill = NewPart<URectLightComponent>();
	DoorSpill->SetRelativeLocationAndRotation(FVector((DoorX0 + DoorX1) * 0.5, Y1 + 60.0, DoorTop * 0.55), FRotator(-8.0f, -90.0f, 0.0f));
	DoorSpill->SetIntensityUnits(ELightUnits::Candelas);
	DoorSpill->SetIntensity(DoorCandela);
	DoorSpill->SetLightColor(Srgb(0xe8f1ff));
	DoorSpill->SetSourceWidth(DoorX1 - DoorX0);
	DoorSpill->SetSourceHeight(DoorTop);
	DoorSpill->SetBarnDoorAngle(60.0f);
	DoorSpill->SetAttenuationRadius(1100.0f);
	DoorSpill->SetVolumetricScatteringIntensity(1.0f);
	DoorSpill->SetCastShadows(true);

	// One dryer running (second from the right, top): warm light through its door glass.
	DryerGlow = NewPart<UPointLightComponent>();
	DryerGlow->SetRelativeLocation(FVector(X1 - 70.0, -235.0 + 4 * 86.0, 146.0));
	DryerGlow->SetIntensityUnits(ELightUnits::Candelas);
	DryerGlow->SetIntensity(6.0f);
	DryerGlow->SetLightColor(Srgb(0xffb46b));
	DryerGlow->SetAttenuationRadius(240.0f);
	DryerGlow->SetCastShadows(true);

	// The exit sign over the door.
	AddMesh(CubeMesh, Glow(TEXT("Exit"), Srgb(0xff2a1c), 18.0f), FVector((DoorX0 + DoorX1) * 0.5, Y1 - 3.0, DoorTop + 20.0), FVector(0.34, 0.04, 0.14), FRotator::ZeroRotator, nullptr, false);
}

void ABackRoomStage::BuildAir()
{
	// Cigarette smoke and dryer lint hanging in the air: volumetric fog lit by the lamp and the door.
	Fog = NewPart<UExponentialHeightFogComponent>();
	Fog->SetRelativeLocation(FVector(0.0, 0.0, 0.0));
	Fog->SetFogDensity(FogDensity);
	Fog->SetFogHeightFalloff(0.08f);
	Fog->SetFogInscatteringColor(FLinearColor(0.012f, 0.011f, 0.01f));
	Fog->SetVolumetricFog(true);
	Fog->SetVolumetricFogScatteringDistribution(0.55f);
	Fog->SetVolumetricFogAlbedo(FColor(236, 232, 224));
	Fog->SetVolumetricFogExtinctionScale(1.0f);
	Fog->SetVolumetricFogDistance(1400.0f);
	Fog->SetStartDistance(0.0f);

	Lens = NewPart<UPostProcessComponent>();
	Lens->bUnbound = true;
	FPostProcessSettings& P = Lens->Settings;
	// The eye adapts a little between the bright felt and the dark corners, within a narrow range.
	P.bOverride_AutoExposureMethod = true;
	P.AutoExposureMethod = EAutoExposureMethod::AEM_Histogram;
	P.bOverride_AutoExposureMinBrightness = true;
	P.AutoExposureMinBrightness = 6.2f;
	P.bOverride_AutoExposureMaxBrightness = true;
	P.AutoExposureMaxBrightness = 7.6f;
	P.bOverride_AutoExposureBias = true;
	P.AutoExposureBias = ExposureCompensation;
	P.bOverride_AutoExposureSpeedUp = true;
	P.AutoExposureSpeedUp = 1.5f;
	P.bOverride_AutoExposureSpeedDown = true;
	P.AutoExposureSpeedDown = 0.8f;
	P.bOverride_BloomIntensity = true;
	P.BloomIntensity = 0.55f;
	P.bOverride_BloomThreshold = true;
	P.BloomThreshold = 1.0f;
	P.bOverride_VignetteIntensity = true;
	P.VignetteIntensity = 0.45f;
	P.bOverride_FilmGrainIntensity = true;
	P.FilmGrainIntensity = 0.22f;
	P.bOverride_SceneFringeIntensity = true;
	P.SceneFringeIntensity = 0.25f;
	// A slightly warm, low-saturation film grade: sodium and nicotine, green in the shadows.
	P.bOverride_WhiteTemp = true;
	P.WhiteTemp = 6900.0f;
	P.bOverride_ColorSaturation = true;
	P.ColorSaturation = FVector4(0.92, 0.92, 0.92, 1.0);
	P.bOverride_ColorGainShadows = true;
	P.ColorGainShadows = FVector4(0.95, 1.02, 0.98, 1.0);
	P.bOverride_ColorContrast = true;
	P.ColorContrast = FVector4(1.06, 1.06, 1.06, 1.0);
}

// ------------------------------------------------------------------ play

void ABackRoomStage::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	// The failing tube: steady for a while, then a burst of stutters as the starter tries again.
	float Tube = 1.0f;
	NextFlicker -= DeltaSeconds;
	if (NextFlicker <= 0.0f)
	{
		FlickerLeft = FMath::FRandRange(0.25f, 1.1f);
		NextFlicker = FMath::FRandRange(2.5f, 9.0f);
	}
	if (FlickerLeft > 0.0f)
	{
		FlickerLeft -= DeltaSeconds;
		Tube = FMath::Frac(Time * 17.0f) < 0.45f ? FMath::FRandRange(0.0f, 0.35f) : FMath::FRandRange(0.7f, 1.0f);
	}
	if (Fluorescent)
	{
		Fluorescent->SetIntensity(FluorescentCandela * Tube);
	}
	if (TubeGlow)
	{
		TubeGlow->SetScalarParameterValue(TEXT("Emissive"), 30.0f * Tube);
	}
	// The running dryer: its drum's light rises and falls as the clothes tumble past the glass.
	if (DryerGlow)
	{
		const float Tumble = 0.75f + 0.25f * FMath::Sin(Time * 2.3f) * FMath::Sin(Time * 0.7f + 1.3f);
		DryerGlow->SetIntensity(6.0f * Tumble);
	}
}
