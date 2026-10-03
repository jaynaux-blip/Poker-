#include "StreetStage.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ShortStack/Game/Store.h"
#include "ShortStack/UI/PropArt.h"
#include "ShortStack/UI/Ui.h"
#include "SlateDrawList.h"
#include "UObject/ConstructorHelpers.h"

namespace StreetStageDetail
{
// The street's plan (cm).
const double SidewalkW = 420.0;
const double RoadX0 = 420.0;
const double RoadX1 = 1620.0;
const double FarFront = 2040.0;
const double BlockY0 = -6000.0;
const double StoreY0 = 3700.0;
const double StoreY1 = 4950.0;
const double MarketY1 = 6150.0;
const double StoreDepth = 1400.0;
const double StoreCeiling = 380.0;
const double DoorY0 = 4650.0;
const double DoorY1 = 4850.0;

// Patterns understood by M_Street (Content/Python/street_setup.py); 0 is plain.
const float PatBrick = 1.0f;
const float PatAsphalt = 2.0f;
const float PatSlabs = 3.0f;
const float PatTiles = 4.0f;
const float PatConcrete = 5.0f;

FLinearColor SrgbHex(uint32 Hex)
{
	return FLinearColor::FromSRGBColor(FColor(static_cast<uint8>((Hex >> 16) & 0xff), static_cast<uint8>((Hex >> 8) & 0xff), static_cast<uint8>(Hex & 0xff)));
}

UMaterialInterface* OptionalMaterial(const TCHAR* Path)
{
	return LoadObject<UMaterialInterface>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
}


/** A small deterministic random sequence for windows and shelves. */
struct FStreetDice
{
	int64 S = 20261003;
	double Next()
	{
		S = (S * 16807) % 2147483647;
		return static_cast<double>(S) / 2147483647.0;
	}
};

} // namespace StreetStageDetail

using namespace StreetStageDetail;

AStreetStage::AStreetStage()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Basic(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	CubeMesh = Cube.Object;
	CylinderMesh = Cylinder.Object;
	SphereMesh = Sphere.Object;
	PlaneMesh = Plane.Object;
	FallbackMaterial = Basic.Object;
}

// ------------------------------------------------------------------ building blocks

template <typename T>
T* AStreetStage::NewPart(USceneComponent* Parent)
{
	T* Part = NewObject<T>(this);
	Part->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	Part->SetupAttachment(Parent ? Parent : Root.Get());
	BlueprintCreatedComponents.Add(Part);
	Part->RegisterComponent();
	return Part;
}

UMaterialInstanceDynamic* AStreetStage::Mat(FName Key, uint32 SrgbHexColor, float Roughness, float Pattern, float Emissive, float Metallic)
{
	if (TObjectPtr<UMaterialInstanceDynamic>* Found = Materials.Find(Key))
	{
		return *Found;
	}
	UMaterialInterface* Parent = StreetMaterial ? StreetMaterial.Get() : (SurfaceMaterial ? SurfaceMaterial.Get() : FallbackMaterial.Get());
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Parent, this);
	const FLinearColor Color = SrgbHex(SrgbHexColor);
	M->SetVectorParameterValue(TEXT("BaseColor"), Color);
	M->SetVectorParameterValue(TEXT("Color"), Color);
	M->SetScalarParameterValue(TEXT("Roughness"), Roughness);
	M->SetScalarParameterValue(TEXT("Metallic"), Metallic);
	M->SetScalarParameterValue(TEXT("Pattern"), Pattern);
	M->SetScalarParameterValue(TEXT("Emissive"), Emissive);
	M->SetScalarParameterValue(TEXT("Wet"), Wetness);
	Materials.Add(Key, M);
	return M;
}

UStaticMeshComponent* AStreetStage::Box(UMaterialInterface* Material, const FVector& Min, const FVector& Max, bool bCollide, USceneComponent* Parent)
{
	UStaticMeshComponent* C = NewPart<UStaticMeshComponent>(Parent);
	C->SetStaticMesh(CubeMesh);
	// The engine cube is 100 cm on a side, centered on its pivot.
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

UStaticMeshComponent* AStreetStage::Cyl(UMaterialInterface* Material, const FVector& Base, float Radius, float Height, bool bCollide)
{
	UStaticMeshComponent* C = NewPart<UStaticMeshComponent>();
	C->SetStaticMesh(CylinderMesh);
	C->SetRelativeLocation(Base + FVector(0.0, 0.0, Height * 0.5));
	C->SetRelativeScale3D(FVector(Radius / 50.0, Radius / 50.0, Height / 100.0));
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

UStaticMeshComponent* AStreetStage::Prop(const TCHAR* Name, const FVector& At, float Yaw, const FVector& Scale)
{
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/ShortStack/Meshes/%s/%s.%s"), Name, Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Mesh)
	{
		return nullptr;
	}
	UStaticMeshComponent* C = NewPart<UStaticMeshComponent>();
	C->SetStaticMesh(Mesh);
	C->SetRelativeLocationAndRotation(At, FRotator(0.0f, Yaw, 0.0f));
	C->SetRelativeScale3D(Scale);
	C->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	return C;
}

UWidgetComponent* AStreetStage::Sign(const FVector& At, float Yaw, const FVector2D& SizeCm, const FIntPoint& Pixels, bool bLit)
{
	UWidgetComponent* Wc = NewPart<UWidgetComponent>();
	Wc->SetWidgetSpace(EWidgetSpace::World);
	Wc->SetDrawSize(FVector2D(Pixels.X, Pixels.Y));
	Wc->SetPivot(FVector2D(0.5, 0.5));
	Wc->SetBlendMode(EWidgetBlendMode::Transparent);
	Wc->SetTwoSided(false);
	Wc->SetBackgroundColor(FLinearColor::Transparent);
	Wc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Wc->SetCastShadow(false);
	Wc->SetRelativeLocationAndRotation(At, FRotator(0.0f, Yaw, 0.0f));
	// The widget quad lies in its local Y (width) / Z (height) plane, one unit per pixel.
	Wc->SetRelativeScale3D(FVector(1.0, SizeCm.X / Pixels.X, SizeCm.Y / Pixels.Y));
	// Signs that light up glow past white so they bloom in the rain.
	Wc->SetTintColorAndOpacity(bLit ? FLinearColor(5.0f, 5.0f, 5.0f, 1.0f) : FLinearColor(0.7f, 0.7f, 0.7f, 1.0f));
	SignWidgets.Add(Wc);
	return Wc;
}

// ------------------------------------------------------------------ construction

void AStreetStage::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildSet();
}

void AStreetStage::RebuildSet()
{
#if WITH_EDITOR
	RerunConstructionScripts();
#endif
}

void AStreetStage::BuildSet()
{
	Materials.Reset();
	TimedMaterials.Reset();
	RainSheets.Reset();
	SignWidgets.Reset();
	Spots.Reset();
	StreetMaterial = OptionalMaterial(TEXT("/Game/ShortStack/Materials/M_Street.M_Street"));
	SurfaceMaterial = OptionalMaterial(TEXT("/Game/ShortStack/Materials/M_Surface.M_Surface"));
	GlowMaterial = OptionalMaterial(TEXT("/Game/ShortStack/Materials/M_Glow.M_Glow"));
	GlassMaterial = OptionalMaterial(TEXT("/Game/ShortStack/Materials/M_RainGlass.M_RainGlass"));
	SkyMaterial = OptionalMaterial(TEXT("/Game/ShortStack/Materials/M_Sky.M_Sky"));
	CityMaterial = OptionalMaterial(TEXT("/Game/ShortStack/Materials/M_City.M_City"));
	RainMaterial = OptionalMaterial(TEXT("/Game/ShortStack/Materials/M_Rain.M_Rain"));
	BuildGround();
	BuildHomeBlock();
	BuildAcross();
	BuildStore();
	BuildStreetlights();
	BuildSkyAndWeather();
}

void AStreetStage::BuildGround()
{
	UMaterialInterface* Walk = Mat(TEXT("Sidewalk"), 0x6f6c66, 0.75f, PatSlabs);
	UMaterialInterface* Road = Mat(TEXT("Asphalt"), 0x26272a, 0.6f, PatAsphalt);
	UMaterialInterface* Curb = Mat(TEXT("Curb"), 0x8a867e, 0.7f, PatConcrete);
	UMaterialInterface* Paint = Mat(TEXT("RoadPaint"), 0xd8d4c8, 0.5f);
	UMaterialInterface* Yellow = Mat(TEXT("RoadYellow"), 0xd9a520, 0.5f);
	// Sidewalks (their tops at 0), the curbs, and the road 15 cm down.
	Box(Walk, FVector(0.0, BlockY0, -30.0), FVector(SidewalkW - 20.0, StoreY1, 0.0));
	Box(Walk, FVector(0.0, MarketY1, -30.0), FVector(SidewalkW - 20.0, 9000.0, 0.0));
	Box(Curb, FVector(SidewalkW - 20.0, BlockY0, -30.0), FVector(SidewalkW, StoreY1, 0.0));
	Box(Walk, FVector(RoadX1 + 20.0, BlockY0, -30.0), FVector(FarFront, 9000.0, 0.0));
	Box(Curb, FVector(RoadX1, BlockY0, -30.0), FVector(RoadX1 + 20.0, 9000.0, 0.0));
	Box(Road, FVector(SidewalkW, BlockY0, -45.0), FVector(RoadX1, 9000.0, -15.0));
	// Market Street crossing at the corner.
	Box(Road, FVector(-3000.0, StoreY1, -45.0), FVector(SidewalkW, MarketY1, -15.0));
	// Lane lines and the crosswalks at the corner.
	for (double Y = BlockY0; Y < 9000.0; Y += 900.0)
	{
		if (Y < StoreY1 - 300.0 || Y > MarketY1 + 300.0)
		{
			Box(Yellow, FVector(1010.0, Y, -15.0), FVector(1030.0, Y + 450.0, -14.4), false);
		}
	}
	for (int32 I = 0; I < 9; ++I)
	{
		const double X = SidewalkW + 40.0 + I * 130.0;
		Box(Paint, FVector(X, StoreY1 + 120.0, -15.0), FVector(X + 70.0, MarketY1 - 120.0, -14.4), false);
	}
	for (int32 I = 0; I < 8; ++I)
	{
		const double Y = StoreY1 + 140.0 + I * 130.0;
		Box(Paint, FVector(-1400.0, Y, -15.0), FVector(-200.0, Y + 70.0, -14.4), false);
	}
	// A storm drain and the corner's ramp.
	Box(Mat(TEXT("Grate"), 0x1a1b1d, 0.35f, 0.0f, 0.0f, 0.8f), FVector(SidewalkW + 2.0, 2200.0, -15.0), FVector(SidewalkW + 60.0, 2300.0, -14.6), false);
}

void AStreetStage::BuildHomeBlock()
{
	FStreetDice Dice;
	UMaterialInterface* Brick = Mat(TEXT("Brick"), 0x6b3427, 0.85f, PatBrick);
	UMaterialInterface* BrickDark = Mat(TEXT("BrickDark"), 0x4a2a24, 0.85f, PatBrick);
	UMaterialInterface* Stone = Mat(TEXT("Stone"), 0x8c8579, 0.8f, PatConcrete);
	UMaterialInterface* WindowLit = Mat(TEXT("WindowLit"), 0xffc98a, 0.4f, 0.0f, 3.5f);
	UMaterialInterface* WindowCool = Mat(TEXT("WindowCool"), 0x9fc3ff, 0.4f, 0.0f, 2.0f);
	UMaterialInterface* WindowDark = Mat(TEXT("WindowDark"), 0x0d1014, 0.15f, 0.0f, 0.0f, 0.3f);
	UMaterialInterface* Wood = Mat(TEXT("DoorWood"), 0x3a2a1e, 0.6f);

	// The apartment building: four floors of brick, the door in a recess with a step, a lamp over it.
	Box(Brick, FVector(-1500.0, -800.0, 0.0), FVector(0.0, -110.0, 1500.0));
	Box(Brick, FVector(-1500.0, 110.0, 0.0), FVector(0.0, 800.0, 1500.0));
	Box(Brick, FVector(-1500.0, -110.0, 270.0), FVector(0.0, 110.0, 1500.0));
	Box(Brick, FVector(-1500.0, -110.0, 0.0), FVector(-70.0, 110.0, 270.0));
	Box(Wood, FVector(-74.0, -60.0, 14.0), FVector(-70.0, 60.0, 250.0));
	Box(Stone, FVector(-70.0, -130.0, 0.0), FVector(45.0, 130.0, 14.0));
	Box(Stone, FVector(-4.0, -820.0, 1490.0), FVector(18.0, 820.0, 1520.0), false);
	if (UPointLightComponent* Lamp = NewPart<UPointLightComponent>())
	{
		Lamp->SetRelativeLocation(FVector(30.0, 0.0, 290.0));
		Lamp->SetIntensityUnits(ELightUnits::Candelas);
		Lamp->SetIntensity(60.0f);
		Lamp->SetUseTemperature(true);
		Lamp->SetTemperature(2700.0f);
		Lamp->SetAttenuationRadius(700.0f);
		Lamp->SetCastShadows(false);
	}
	Cyl(Mat(TEXT("LampGlass"), 0xffd9a0, 0.3f, 0.0f, 12.0f), FVector(12.0, 0.0, 280.0), 9.0f, 18.0f, false);
	Sign(FVector(4.0, 0.0, 312.0), 0.0f, FVector2D(80.0, 18.0), FIntPoint(400, 90), false);

	// Neighbors along Fifth: the pawn shop, a barber, apartments; windows lit at random.
	struct FFront
	{
		double Y0, Y1, Height;
		bool bDark;
	};
	const FFront Fronts[] = {{800.0, 2300.0, 1100.0, false}, {2300.0, 3700.0, 900.0, true}, {-2400.0, -800.0, 1900.0, true}, {BlockY0, -2400.0, 1300.0, false}};
	for (const FFront& Fr : Fronts)
	{
		Box(Fr.bDark ? BrickDark : Brick, FVector(-1500.0, Fr.Y0, 0.0), FVector(0.0, Fr.Y1, Fr.Height));
		// Shop window at street level (dark, gated), then the floors above.
		Box(WindowDark, FVector(-2.0, Fr.Y0 + 150.0, 40.0), FVector(3.0, Fr.Y1 - 150.0, 280.0), false);
		for (double Z = 420.0; Z + 200.0 < Fr.Height; Z += 330.0)
		{
			for (double Y = Fr.Y0 + 160.0; Y + 120.0 < Fr.Y1; Y += 260.0)
			{
				const double Roll = Dice.Next();
				UMaterialInterface* Glass = Roll < 0.22 ? WindowLit : Roll < 0.3 ? WindowCool : WindowDark;
				Box(Glass, FVector(-2.0, Y, Z), FVector(3.0, Y + 120.0, Z + 170.0), false);
			}
		}
	}
	// The apartment's own windows (the player's on the third floor still has the laptop glow).
	for (int32 Floor = 0; Floor < 3; ++Floor)
	{
		for (int32 W = 0; W < 4; ++W)
		{
			const double Y = -700.0 + W * 360.0;
			const bool bMine = Floor == 1 && W == 2;
			UMaterialInterface* Glass = bMine ? WindowCool : Dice.Next() < 0.25 ? WindowLit : WindowDark;
			Box(Glass, FVector(-2.0, Y, 420.0 + Floor * 340.0), FVector(3.0, Y + 130.0, 600.0 + Floor * 340.0), false);
		}
	}
	// Street furniture from Blender where it's been imported.
	Prop(TEXT("SM_Hydrant"), FVector(360.0, 1300.0, 0.0), 0.0f);
	Prop(TEXT("SM_TrashCan"), FVector(330.0, -500.0, 0.0), 12.0f);
	Prop(TEXT("SM_TrashCan"), FVector(330.0, 3550.0, 0.0), -8.0f);
	Prop(TEXT("SM_Newsbox"), FVector(300.0, 3400.0, 0.0), 90.0f);

	FStreetSpot Home;
	Home.Id = TEXT("home");
	Home.At = FVector(60.0, 0.0, 100.0);
	Home.Radius = 170.0f;
	Home.Prompt = TEXT("Go home");
	Spots.Add(Home);
}

void AStreetStage::BuildAcross()
{
	FStreetDice Dice;
	Dice.S = 777;
	UMaterialInterface* Brick = Mat(TEXT("BrickFar"), 0x5a3a30, 0.85f, PatBrick);
	UMaterialInterface* Stucco = Mat(TEXT("Stucco"), 0x8a8478, 0.9f, PatConcrete);
	UMaterialInterface* Fluor = Mat(TEXT("Fluorescent"), 0xe8f4ff, 0.3f, 0.0f, 6.0f);
	UMaterialInterface* Washer = Mat(TEXT("Washer"), 0xd9dcdf, 0.3f, 0.0f, 0.0f, 0.2f);
	UMaterialInterface* WasherDoor = Mat(TEXT("WasherDoor"), 0x2a3440, 0.1f, 0.0f, 0.3f, 0.5f);
	UMaterialInterface* WindowLit = Mat(TEXT("WindowLit"), 0xffc98a, 0.4f, 0.0f, 3.5f);
	UMaterialInterface* WindowDark = Mat(TEXT("WindowDark"), 0x0d1014, 0.15f, 0.0f, 0.0f, 0.3f);

	// The Wash & Fold: a low storefront, lit all night, the neon over the windows.
	Box(Stucco, FVector(FarFront, -700.0, 0.0), FVector(FarFront + 1300.0, 700.0, 450.0));
	Box(Fluor, FVector(FarFront - 3.0, -620.0, 60.0), FVector(FarFront + 2.0, 620.0, 300.0), false);
	if (GlassMaterial)
	{
		Box(GlassMaterial, FVector(FarFront - 8.0, -640.0, 50.0), FVector(FarFront - 6.0, 640.0, 310.0), false);
	}
	for (int32 I = 0; I < 7; ++I)
	{
		const double Y = -540.0 + I * 160.0;
		Box(Washer, FVector(FarFront - 70.0, Y, 0.0), FVector(FarFront - 10.0, Y + 120.0, 110.0), false);
		Cyl(WasherDoor, FVector(FarFront - 11.0, Y + 60.0, 40.0), 26.0f, 2.0f, false)->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	}
	Sign(FVector(FarFront - 12.0, 0.0, 375.0), 180.0f, FVector2D(420.0, 131.0), FIntPoint(1024, 320), true);
	if (GlowMaterial)
	{
		UMaterialInstanceDynamic* Halo = UMaterialInstanceDynamic::Create(GlowMaterial, this);
		Halo->SetVectorParameterValue(TEXT("Color"), SrgbHex(0xff2e88));
		Halo->SetScalarParameterValue(TEXT("Strength"), 0.5f);
		UStaticMeshComponent* Plane = NewPart<UStaticMeshComponent>();
		Plane->SetStaticMesh(PlaneMesh);
		Plane->SetMaterial(0, Halo);
		// The engine plane faces +Z: pitched up it faces the street (-X), its local X running up the wall.
		Plane->SetRelativeLocationAndRotation(FVector(FarFront - 14.0, 0.0, 375.0), FRotator(90.0f, 0.0f, 0.0f));
		Plane->SetRelativeScale3D(FVector(4.0, 9.0, 1.0));
		Plane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Plane->SetCastShadow(false);
	}
	if (URectLightComponent* Spill = NewPart<URectLightComponent>())
	{
		// The laundromat's light on the wet sidewalk.
		Spill->SetRelativeLocationAndRotation(FVector(FarFront - 20.0, 0.0, 180.0), FRotator(-15.0f, 180.0f, 0.0f));
		Spill->SetIntensityUnits(ELightUnits::Lumens);
		Spill->SetIntensity(5000.0f);
		Spill->SetSourceWidth(1200.0f);
		Spill->SetSourceHeight(240.0f);
		Spill->SetAttenuationRadius(1500.0f);
		Spill->SetLightColor(SrgbHex(0xdff0ff));
		Spill->SetCastShadows(false);
	}
	// Taller buildings either side of it, windows lit at random.
	const double Spans[][3] = {{-6000.0, -700.0, 1800.0}, {700.0, 3000.0, 1400.0}, {3000.0, 9000.0, 2200.0}};
	for (const auto& Sp : Spans)
	{
		Box(Brick, FVector(FarFront, Sp[0], 0.0), FVector(FarFront + 1500.0, Sp[1], Sp[2]));
		for (double Z = 380.0; Z + 200.0 < Sp[2]; Z += 330.0)
		{
			for (double Y = Sp[0] + 140.0; Y + 120.0 < Sp[1]; Y += 250.0)
			{
				const bool bLit = Dice.Next() < 0.2;
				Box(bLit ? WindowLit : WindowDark, FVector(FarFront - 3.0, Y, Z), FVector(FarFront + 2.0, Y + 120.0, Z + 170.0), false);
			}
		}
	}
	// Parked cars at the far curb (boxes until the Blender cars come).
	UMaterialInterface* CarPaint[3] = {Mat(TEXT("CarA"), 0x1d2a3a, 0.25f, 0.0f, 0.0f, 0.6f), Mat(TEXT("CarB"), 0x5a1d1d, 0.25f, 0.0f, 0.0f, 0.6f), Mat(TEXT("CarC"), 0x9a9a96, 0.3f, 0.0f, 0.0f, 0.6f)};
	UMaterialInterface* CarGlass = Mat(TEXT("CarGlass"), 0x0b0e12, 0.05f, 0.0f, 0.0f, 0.4f);
	const double Cars[] = {-3800.0, -1500.0, 1200.0, 2800.0};
	for (int32 I = 0; I < 4; ++I)
	{
		const double Y = Cars[I];
		if (!Prop(TEXT("SM_Sedan"), FVector(RoadX1 - 120.0, Y + 230.0, -15.0), 90.0f))
		{
			Box(CarPaint[I % 3], FVector(RoadX1 - 210.0, Y, -5.0), FVector(RoadX1 - 30.0, Y + 460.0, 90.0));
			Box(CarGlass, FVector(RoadX1 - 195.0, Y + 110.0, 90.0), FVector(RoadX1 - 45.0, Y + 330.0, 140.0));
		}
	}
}

void AStreetStage::BuildStore()
{
	FStreetDice Dice;
	Dice.S = 212;
	const double X0 = -StoreDepth;
	UMaterialInterface* Wall = Mat(TEXT("StoreWall"), 0xe9e4d8, 0.85f);
	UMaterialInterface* Outside = Mat(TEXT("StoreOutside"), 0x5b5148, 0.85f, PatBrick);
	UMaterialInterface* Floor = Mat(TEXT("StoreFloor"), 0xd8d2c4, 0.35f, PatTiles);
	UMaterialInterface* Ceiling = Mat(TEXT("StoreCeiling"), 0xf2f0ea, 0.9f);
	UMaterialInterface* Panel = Mat(TEXT("StoreLightPanel"), 0xf4fbff, 0.3f, 0.0f, 9.0f);
	UMaterialInterface* Red = Mat(TEXT("PennyRed"), 0xd7263d, 0.5f);
	UMaterialInterface* Metal = Mat(TEXT("Steel"), 0xa0a4aa, 0.35f, 0.0f, 0.0f, 0.9f);
	UMaterialInterface* Counter = Mat(TEXT("CounterTop"), 0x2b2f36, 0.3f, 0.0f, 0.0f, 0.2f);
	UMaterialInterface* Shelf = Mat(TEXT("Shelf"), 0xe2e2e0, 0.5f, 0.0f, 0.0f, 0.4f);
	UMaterialInterface* CoolerGlow = Mat(TEXT("CoolerGlow"), 0xe6f3ff, 0.2f, 0.0f, 4.0f);

	// Shell: back, sides, roof with a parapet, the front's low wall, mullions and the header over the windows.
	Box(Outside, FVector(X0 - 20.0, StoreY0, 0.0), FVector(X0, StoreY1, 480.0));
	Box(Outside, FVector(X0, StoreY0 - 20.0, 0.0), FVector(0.0, StoreY0, 480.0));
	Box(Outside, FVector(X0, StoreY1, 0.0), FVector(0.0, StoreY1 + 20.0, 480.0));
	Box(Outside, FVector(X0 - 20.0, StoreY0 - 20.0, 480.0), FVector(10.0, StoreY1 + 20.0, 520.0));
	Box(Ceiling, FVector(X0, StoreY0, StoreCeiling), FVector(0.0, StoreY1, StoreCeiling + 4.0), false);
	Box(Outside, FVector(-20.0, StoreY0, 0.0), FVector(0.0, DoorY0 - 30.0, 45.0));
	Box(Outside, FVector(-20.0, DoorY1 + 30.0, 0.0), FVector(0.0, StoreY1, 45.0));
	Box(Outside, FVector(-20.0, StoreY0, 330.0), FVector(0.0, StoreY1, 480.0));
	for (double Y : {StoreY0 + 470.0, StoreY0 + 900.0, DoorY0 - 30.0, DoorY1 + 30.0})
	{
		Box(Metal, FVector(-18.0, Y - 6.0, 45.0), FVector(-6.0, Y + 6.0, 330.0));
	}
	if (GlassMaterial)
	{
		Box(GlassMaterial, FVector(-12.0, StoreY0, 45.0), FVector(-10.0, DoorY0 - 30.0, 330.0), false);
		Box(GlassMaterial, FVector(-12.0, DoorY1 + 30.0, 45.0), FVector(-10.0, StoreY1, 330.0), false);
	}
	// The windows themselves block (glass you can't walk through) even before the glass material exists.
	Box(nullptr, FVector(-14.0, StoreY0, 45.0), FVector(-8.0, DoorY0 - 30.0, 330.0))->SetVisibility(false);
	Box(nullptr, FVector(-14.0, DoorY1 + 30.0, 45.0), FVector(-8.0, StoreY1, 330.0))->SetVisibility(false);
	Box(Red, FVector(-6.0, StoreY0, 318.0), FVector(2.0, StoreY1, 330.0), false);
	// Sliding doors.
	UMaterialInterface* DoorGlass = GlassMaterial ? GlassMaterial.Get() : Mat(TEXT("DoorGlassFallback"), 0x9fb7c8, 0.05f);
	DoorLeft = Box(DoorGlass, FVector(-16.0, DoorY0, 0.0), FVector(-10.0, (DoorY0 + DoorY1) * 0.5, 260.0));
	DoorRight = Box(DoorGlass, FVector(-16.0, (DoorY0 + DoorY1) * 0.5, 0.0), FVector(-10.0, DoorY1, 260.0));
	Box(Metal, FVector(-20.0, DoorY0 - 30.0, 260.0), FVector(0.0, DoorY1 + 30.0, 330.0), false);

	// Inside: floor, the light panels and the light itself.
	Box(Floor, FVector(X0, StoreY0, -2.0), FVector(-16.0, StoreY1, 1.0));
	for (double X = X0 + 200.0; X < -100.0; X += 300.0)
	{
		for (double Y = StoreY0 + 200.0; Y < StoreY1 - 100.0; Y += 420.0)
		{
			Box(Panel, FVector(X, Y, StoreCeiling - 3.0), FVector(X + 120.0, Y + 60.0, StoreCeiling), false);
		}
	}
	for (int32 I = 0; I < 3; ++I)
	{
		if (URectLightComponent* L = NewPart<URectLightComponent>())
		{
			L->SetRelativeLocationAndRotation(FVector(-250.0 - I * 420.0, (StoreY0 + StoreY1) * 0.5, StoreCeiling - 6.0), FRotator(-90.0f, 0.0f, 0.0f));
			L->SetIntensityUnits(ELightUnits::Lumens);
			L->SetIntensity(5200.0f);
			L->SetSourceWidth(380.0f);
			L->SetSourceHeight(900.0f);
			L->SetAttenuationRadius(1200.0f);
			L->SetUseTemperature(true);
			L->SetTemperature(4600.0f);
			L->SetCastShadows(I == 1);
		}
	}
	// The store's light spilling out onto the sidewalk.
	if (URectLightComponent* Spill = NewPart<URectLightComponent>())
	{
		Spill->SetRelativeLocationAndRotation(FVector(-30.0, (StoreY0 + StoreY1) * 0.5, 200.0), FRotator(-20.0f, 0.0f, 0.0f));
		Spill->SetIntensityUnits(ELightUnits::Lumens);
		Spill->SetIntensity(6000.0f);
		Spill->SetSourceWidth(1100.0f);
		Spill->SetSourceHeight(260.0f);
		Spill->SetAttenuationRadius(1300.0f);
		Spill->SetUseTemperature(true);
		Spill->SetTemperature(4800.0f);
		Spill->SetCastShadows(false);
	}

	// The cooler wall at the back: glass doors lit from inside, rows of drinks behind them.
	if (!Prop(TEXT("SM_Cooler"), FVector(X0 + 40.0, 4500.0, 0.0), 0.0f))
	{
		Box(Metal, FVector(X0, 4100.0, 0.0), FVector(X0 + 90.0, StoreY1 - 50.0, 230.0));
		Box(CoolerGlow, FVector(X0 + 90.0, 4130.0, 20.0), FVector(X0 + 92.0, StoreY1 - 80.0, 215.0), false);
	}
	UInstancedStaticMeshComponent* Drinks = NewPart<UInstancedStaticMeshComponent>();
	Drinks->SetStaticMesh(CylinderMesh);
	Drinks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Drinks->SetMaterial(0, Mat(TEXT("DrinkCans"), 0xc8262f, 0.3f, 0.0f, 0.0f, 0.5f));
	for (double Z = 30.0; Z < 210.0; Z += 45.0)
	{
		for (double Y = 4150.0; Y < StoreY1 - 90.0; Y += 14.0)
		{
			Drinks->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X0 + 70.0, Y, Z + 9.0), FVector(0.13, 0.13, 0.2)));
		}
	}

	// Two aisles of shelves, stocked with the store's own colors.
	const std::vector<ss::store::Item>& Catalog = ss::store::Catalog();
	UInstancedStaticMeshComponent* Goods[4];
	for (int32 K = 0; K < 4; ++K)
	{
		const ss::store::Item& Item = Catalog[static_cast<size_t>(K * 3 + 5) % Catalog.size()];
		Goods[K] = NewPart<UInstancedStaticMeshComponent>();
		Goods[K]->SetStaticMesh(CubeMesh);
		Goods[K]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Goods[K]->SetMaterial(0, Mat(*FString::Printf(TEXT("Goods%d"), K), Item.Color, 0.5f));
	}
	for (double Ay : {4330.0, 4560.0})
	{
		if (!Prop(TEXT("SM_Shelf"), FVector(-850.0, Ay, 0.0), 90.0f))
		{
			Box(Shelf, FVector(-1150.0, Ay - 30.0, 0.0), FVector(-550.0, Ay + 30.0, 12.0));
			Box(Shelf, FVector(-1150.0, Ay - 3.0, 12.0), FVector(-550.0, Ay + 3.0, 160.0));
			for (double Z : {40.0, 85.0, 130.0})
			{
				Box(Shelf, FVector(-1150.0, Ay - 30.0, Z - 3.0), FVector(-550.0, Ay + 30.0, Z), false);
			}
		}
		for (double Z : {40.0, 85.0, 130.0})
		{
			for (double X = -1140.0; X < -560.0; X += 22.0)
			{
				for (double Side : {-1.0, 1.0})
				{
					const int32 K = static_cast<int32>(Dice.Next() * 4.0) % 4;
					Goods[K]->AddInstance(FTransform(FRotator(0.0f, static_cast<float>(Dice.Next() * 8.0 - 4.0), 0.0f), FVector(X, Ay + Side * 17.0, Z + 12.0), FVector(0.16, 0.2, 0.24)));
				}
			}
		}
	}

	// The counter, the register and the roller grill; the lotto and smokes behind Benny; the coffee bar.
	Box(Counter, FVector(-650.0, 3980.0, 0.0), FVector(-150.0, 4060.0, 100.0));
	Box(Mat(TEXT("CounterFront"), 0x7a1f2c, 0.6f), FVector(-650.0, 4058.0, 0.0), FVector(-150.0, 4062.0, 92.0), false);
	if (!Prop(TEXT("SM_Register"), FVector(-300.0, 4020.0, 100.0), 180.0f))
	{
		Box(Mat(TEXT("Register"), 0x1b1d22, 0.35f), FVector(-340.0, 3995.0, 100.0), FVector(-260.0, 4045.0, 128.0), false);
		Box(Mat(TEXT("RegisterScreen"), 0x6ad1ff, 0.3f, 0.0f, 3.0f), FVector(-330.0, 4044.0, 128.0), FVector(-270.0, 4046.0, 150.0), false);
	}
	Box(Metal, FVector(-600.0, 3995.0, 100.0), FVector(-470.0, 4050.0, 112.0), false);
	UMaterialInterface* Dog = Mat(TEXT("RollerDog"), 0xb5452e, 0.4f, 0.0f, 0.2f);
	for (int32 I = 0; I < 6; ++I)
	{
		Box(Dog, FVector(-592.0 + I * 21.0, 4004.0, 112.0), FVector(-578.0 + I * 21.0, 4042.0, 117.0), false);
	}
	Box(Mat(TEXT("Lotto"), 0x2b3a7a, 0.4f, 0.0f, 1.5f), FVector(-620.0, StoreY0 + 2.0, 120.0), FVector(-180.0, StoreY0 + 20.0, 300.0), false);
	Box(Mat(TEXT("CoffeeBar"), 0x3b2a20, 0.4f), FVector(X0 + 100.0, StoreY0, 0.0), FVector(-900.0, StoreY0 + 70.0, 95.0));
	for (int32 I = 0; I < 3; ++I)
	{
		Box(Metal, FVector(-1250.0 + I * 110.0, StoreY0 + 10.0, 95.0), FVector(-1190.0 + I * 110.0, StoreY0 + 60.0, 175.0), false);
	}
	Box(Wall, FVector(X0, StoreY0, 0.0), FVector(X0 + 2.0, StoreY1, StoreCeiling), false);

	// The signs: the box sign over the windows, OPEN in the window, the hours on the door, two posters.
	Sign(FVector(4.0, 4300.0, 405.0), 0.0f, FVector2D(720.0, 135.0), FIntPoint(1600, 300), true);
	Sign(FVector(-6.0, 4000.0, 230.0), 0.0f, FVector2D(110.0, 48.0), FIntPoint(640, 280), true);
	Sign(FVector(-8.0, DoorY0 + 50.0, 150.0), 0.0f, FVector2D(32.0, 42.0), FIntPoint(400, 520), false);
	Sign(FVector(-6.0, 3820.0, 160.0), 0.0f, FVector2D(60.0, 90.0), FIntPoint(600, 900), false);
	Sign(FVector(-6.0, 4470.0, 160.0), 0.0f, FVector2D(60.0, 90.0), FIntPoint(600, 900), false);

	// What the player can do in here.
	auto Spot = [this](const TCHAR* Id, const FVector& At, float Radius, const TCHAR* Prompt, int32 ShelfIndex) {
		FStreetSpot S;
		S.Id = Id;
		S.At = At;
		S.Radius = Radius;
		S.Prompt = Prompt;
		S.Shelf = ShelfIndex;
		Spots.Add(S);
	};
	Spot(TEXT("counter"), FVector(-330.0, 4150.0, 100.0), 140.0f, TEXT("Talk to Benny"), -1);
	Spot(TEXT("grill"), FVector(-540.0, 4150.0, 100.0), 110.0f, TEXT("Check the roller grill"), static_cast<int32>(ss::store::Shelf::Hot));
	Spot(TEXT("cooler"), FVector(X0 + 160.0, 4500.0, 100.0), 220.0f, TEXT("Open the cooler"), static_cast<int32>(ss::store::Shelf::Drinks));
	Spot(TEXT("aisle"), FVector(-850.0, 4445.0, 100.0), 230.0f, TEXT("Browse the snacks"), static_cast<int32>(ss::store::Shelf::Snacks));
	Spot(TEXT("coffee"), FVector(-1100.0, StoreY0 + 140.0, 100.0), 170.0f, TEXT("Pour a coffee"), static_cast<int32>(ss::store::Shelf::Coffee));
}

void AStreetStage::BuildStreetlights()
{
	UMaterialInterface* Pole = Mat(TEXT("Pole"), 0x2c3033, 0.5f, 0.0f, 0.0f, 0.7f);
	UMaterialInterface* Sodium = Mat(TEXT("Sodium"), 0xffb066, 0.3f, 0.0f, 14.0f);
	const FVector2D Posts[] = {{380.0, -4300.0}, {380.0, -2000.0}, {380.0, 700.0}, {380.0, 2700.0}, {380.0, 4700.0}, {1660.0, -3200.0}, {1660.0, -600.0}, {1660.0, 1800.0}, {1660.0, 4200.0}};
	int32 Index = 0;
	for (const FVector2D& P : Posts)
	{
		const bool bNear = P.X < 1000.0;
		const double Out = bNear ? 1.0 : -1.0; // the arm reaches over the road
		if (!Prop(TEXT("SM_Streetlight"), FVector(P.X, P.Y, 0.0), bNear ? 0.0f : 180.0f))
		{
			Cyl(Pole, FVector(P.X, P.Y, 0.0), 9.0f, 820.0f);
			Box(Pole, FVector(FMath::Min(P.X, P.X + Out * 180.0), P.Y - 5.0, 805.0), FVector(FMath::Max(P.X, P.X + Out * 180.0), P.Y + 5.0, 815.0), false);
			Box(Sodium, FVector(P.X + Out * 150.0 - 30.0, P.Y - 14.0, 795.0), FVector(P.X + Out * 150.0 + 30.0, P.Y + 14.0, 805.0), false);
		}
		if (USpotLightComponent* L = NewPart<USpotLightComponent>())
		{
			L->SetRelativeLocationAndRotation(FVector(P.X + Out * 150.0, P.Y, 790.0), FRotator(-90.0f, 0.0f, 0.0f));
			L->SetIntensityUnits(ELightUnits::Candelas);
			L->SetIntensity(StreetlightCandela);
			L->SetAttenuationRadius(2400.0f);
			L->SetOuterConeAngle(68.0f);
			L->SetInnerConeAngle(30.0f);
			L->SetUseTemperature(true);
			L->SetTemperature(Index % 3 == 2 ? 4000.0f : 2300.0f); // sodium, and the odd newer LED
			L->SetVolumetricScatteringIntensity(2.0f);
			L->SetCastShadows(Index < 5);
		}
		++Index;
	}
	// The corner's street blades.
	Cyl(Pole, FVector(390.0, StoreY1 + 60.0, 0.0), 5.0f, 330.0f);
	Sign(FVector(390.0, StoreY1 + 60.0, 340.0), 90.0f, FVector2D(110.0, 24.0), FIntPoint(900, 200), false);
	Sign(FVector(390.0, StoreY1 + 60.0, 312.0), 0.0f, FVector2D(110.0, 24.0), FIntPoint(900, 200), false);
}

void AStreetStage::BuildSkyAndWeather()
{
	// Sky dome and the skyline beyond the blocks (the apartment's materials).
	if (SkyMaterial)
	{
		UMaterialInstanceDynamic* Sky = UMaterialInstanceDynamic::Create(SkyMaterial, this);
		TimedMaterials.Add(Sky);
		UStaticMeshComponent* Dome = NewPart<UStaticMeshComponent>();
		Dome->SetStaticMesh(SphereMesh);
		Dome->SetMaterial(0, Sky);
		Dome->SetRelativeScale3D(FVector(2400.0));
		Dome->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Dome->SetCastShadow(false);
		Dome->bAffectDistanceFieldLighting = false;
	}
	UInstancedStaticMeshComponent* Skyline = NewPart<UInstancedStaticMeshComponent>();
	Skyline->SetStaticMesh(CubeMesh);
	UMaterialInstanceDynamic* City = CityMaterial ? UMaterialInstanceDynamic::Create(CityMaterial, this) : nullptr;
	if (City)
	{
		TimedMaterials.Add(City);
	}
	Skyline->SetMaterial(0, City ? static_cast<UMaterialInterface*>(City) : Mat(TEXT("CityFallback"), 0x0a0b0e, 0.9f));
	Skyline->SetCastShadow(false);
	Skyline->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Skyline->bAffectDistanceFieldLighting = false;
	FStreetDice Dice;
	Dice.S = 4242;
	for (int32 I = 0; I < 220; ++I)
	{
		const double A = Dice.Next() * 2.0 * PI;
		const double R = 9000.0 + Dice.Next() * 30000.0;
		const double Hgt = 1500.0 + Dice.Next() * Dice.Next() * 12000.0;
		const double Wd = 1200.0 + Dice.Next() * 2400.0;
		Skyline->AddInstance(FTransform(FRotator(0.0f, static_cast<float>(Dice.Next() * 90.0), 0.0f), FVector(R * FMath::Cos(A), 2000.0 + R * FMath::Sin(A), Hgt * 0.5), FVector(Wd / 100.0, Wd / 100.0, Hgt / 100.0)));
	}

	// Rain sheets in front of the camera (FollowCamera moves them).
	if (RainMaterial)
	{
		UMaterialInstanceDynamic* Rain = UMaterialInstanceDynamic::Create(RainMaterial, this);
		TimedMaterials.Add(Rain);
		for (int32 I = 0; I < 3; ++I)
		{
			UStaticMeshComponent* Sheet = NewPart<UStaticMeshComponent>();
			Sheet->SetStaticMesh(PlaneMesh);
			Sheet->SetMaterial(0, Rain);
			Sheet->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Sheet->SetCastShadow(false);
			Sheet->bAffectDistanceFieldLighting = false;
			Sheet->SetRelativeScale3D(FVector(14.0 + I * 6.0, 10.0 + I * 4.0, 1.0));
			RainSheets.Add(Sheet);
		}
	}

	// Night: a faint moon through the clouds, a sky light for the shadows, wet haze, the lens.
	if (UDirectionalLightComponent* Moon = NewPart<UDirectionalLightComponent>())
	{
		Moon->SetRelativeRotation(FRotator(-38.0f, 35.0f, 0.0f));
		Moon->SetIntensity(0.25f);
		Moon->SetLightColor(SrgbHex(0x8fa6d6));
		Moon->SetCastShadows(false);
	}
	SkyLight = NewPart<USkyLightComponent>();
	SkyLight->SourceType = ESkyLightSourceType::SLS_CapturedScene;
	SkyLight->SetIntensity(0.35f);
	SkyLight->SetLightColor(SrgbHex(0x6f7f9f));
	if (UExponentialHeightFogComponent* Fog = NewPart<UExponentialHeightFogComponent>())
	{
		Fog->SetFogDensity(0.035f);
		Fog->SetFogHeightFalloff(0.25f);
		Fog->SetFogInscatteringColor(SrgbHex(0x1a2232));
		Fog->SetVolumetricFog(true);
		Fog->SetVolumetricFogScatteringDistribution(0.6f);
		Fog->SetVolumetricFogExtinctionScale(0.6f);
	}
	if (UPostProcessComponent* Lens = NewPart<UPostProcessComponent>())
	{
		Lens->bUnbound = true;
		FPostProcessSettings& S = Lens->Settings;
		S.bOverride_AutoExposureBias = true;
		S.AutoExposureBias = ExposureBias;
		S.bOverride_AutoExposureMinBrightness = true;
		S.AutoExposureMinBrightness = -2.0f;
		S.bOverride_AutoExposureMaxBrightness = true;
		S.AutoExposureMaxBrightness = 3.0f;
		S.bOverride_BloomIntensity = true;
		S.BloomIntensity = 0.9f;
		S.bOverride_VignetteIntensity = true;
		S.VignetteIntensity = 0.45f;
		S.bOverride_FilmGrainIntensity = true;
		S.FilmGrainIntensity = 0.12f;
	}
}

// ------------------------------------------------------------------ play

void AStreetStage::BeginPlay()
{
	Super::BeginPlay();
	AttachSlate();
	if (SkyLight)
	{
		SkyLight->RecaptureSky();
	}
}

void AStreetStage::AttachSlate()
{
	if (!FSlateTextMeasurer::IsAvailable())
	{
		return;
	}
	namespace P = ss::ui::props;
	FSlateTextMeasurer Measurer;
	auto Draw = [&Measurer](float Wd, float Ht, TFunctionRef<void(ss::ui::Canvas&)> Paint) {
		TSharedPtr<ss::ui::DrawList> L = MakeShared<ss::ui::DrawList>();
		ss::ui::Canvas C(*L, Measurer, Wd, Ht, 1.0f);
		Paint(C);
		return TSharedPtr<const ss::ui::DrawList>(L);
	};
	// In the order Sign() made them: the building number first, then the Wash & Fold, the store's, the corner's.
	TArray<TSharedPtr<const ss::ui::DrawList>> Lists;
	Lists.Add(Draw(400.0f, 90.0f, [](ss::ui::Canvas& C) {
		C.FillRoundRect({0.0f, 0.0f, 400.0f, 90.0f}, 8.0f, ss::ui::Hex(0x1b1d22));
		C.Text("1812", 200.0f, 66.0f, ss::ui::Ts(64.0f, 900, ss::ui::Hex(0xd9c79a), ss::ui::Align::Center));
	}));
	Lists.Add(Draw(P::NeonW, P::NeonH, [](ss::ui::Canvas& C) { P::NeonSign(C); }));
	Lists.Add(Draw(P::StoreSignW, P::StoreSignH, [](ss::ui::Canvas& C) { P::StoreSign(C); }));
	Lists.Add(Draw(P::OpenSignW, P::OpenSignH, [](ss::ui::Canvas& C) { P::OpenSign(C); }));
	Lists.Add(Draw(P::DoorDecalW, P::DoorDecalH, [](ss::ui::Canvas& C) { P::DoorDecal(C); }));
	Lists.Add(Draw(P::PromoW, P::PromoH, [](ss::ui::Canvas& C) { P::Promo(C, "volt-rush", "2 FOR $5"); }));
	Lists.Add(Draw(P::PromoW, P::PromoH, [](ss::ui::Canvas& C) { P::Promo(C, "roller-dog", "$1.99"); }));
	Lists.Add(Draw(P::StreetSignW, P::StreetSignH, [](ss::ui::Canvas& C) { P::StreetSign(C, "FIFTH ST", "1800"); }));
	Lists.Add(Draw(P::StreetSignW, P::StreetSignH, [](ss::ui::Canvas& C) { P::StreetSign(C, "MARKET ST", "200"); }));
	SignSlates.Reset();
	SignLists = Lists;
	for (int32 I = 0; I < SignWidgets.Num() && I < Lists.Num(); ++I)
	{
		if (!SignWidgets[I] || !Lists[I])
		{
			continue;
		}
		TSharedPtr<SDrawListWidget> Sw = SNew(SDrawListWidget).DesiredSize(FVector2D(Lists[I]->Width, Lists[I]->Height));
		Sw->SetDrawList(Lists[I]);
		SignSlates.Add(Sw);
		SignWidgets[I]->SetSlateWidget(Sw);
		SignWidgets[I]->SetManuallyRedraw(true);
		SignWidgets[I]->RequestRedraw();
	}
}

void AStreetStage::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	for (UMaterialInstanceDynamic* M : TimedMaterials)
	{
		if (M)
		{
			M->SetScalarParameterValue(TEXT("Time"), Time);
		}
	}
}

void AStreetStage::FollowCamera(const FVector& Location, const FRotator& Rotation)
{
	const FVector Fwd = FRotator(0.0f, Rotation.Yaw, 0.0f).Vector();
	const double Depths[3] = {250.0, 650.0, 1200.0};
	for (int32 I = 0; I < RainSheets.Num(); ++I)
	{
		if (UStaticMeshComponent* Sheet = RainSheets[I])
		{
			// The plane faces back at the camera, standing upright.
			Sheet->SetWorldLocationAndRotation(Location + Fwd * Depths[I % 3], FRotationMatrix::MakeFromZX(-Fwd, FVector::UpVector).Rotator());
		}
	}
}

void AStreetStage::UpdateDoors(const FVector& Someone, float DeltaSeconds)
{
	const FVector Local = GetActorTransform().InverseTransformPosition(Someone);
	const bool bNear = FMath::Abs(Local.X) < 260.0 && Local.Y > DoorY0 - 220.0 && Local.Y < DoorY1 + 220.0;
	DoorOpen = FMath::FInterpTo(DoorOpen, bNear ? 1.0f : 0.0f, DeltaSeconds, bNear ? 5.0f : 2.5f);
	const double Half = (DoorY1 - DoorY0) * 0.5;
	if (DoorLeft)
	{
		DoorLeft->SetRelativeLocation(FVector(-13.0, DoorY0 + Half * 0.5 - DoorOpen * Half * 0.95, 130.0));
	}
	if (DoorRight)
	{
		DoorRight->SetRelativeLocation(FVector(-13.0, DoorY1 - Half * 0.5 + DoorOpen * Half * 0.95, 130.0));
	}
}

FTransform AStreetStage::StartAt(const FString& From) const
{
	const FTransform Local = From == TEXT("store") ? FTransform(FRotator(0.0f, 180.0f, 0.0f), FVector(-200.0, 4700.0, 100.0)) : FTransform(FRotator(0.0f, 65.0f, 0.0f), FVector(150.0, 80.0, 100.0));
	return Local * GetActorTransform();
}

FTransform AStreetStage::ClerkSpot() const
{
	return FTransform(FRotator(0.0f, 90.0f, 0.0f), FVector(-360.0, 3880.0, 92.0)) * GetActorTransform();
}

bool AStreetStage::IsInsideStore(const FVector& At) const
{
	const FVector L = GetActorTransform().InverseTransformPosition(At);
	return L.X < -10.0 && L.X > -StoreDepth && L.Y > StoreY0 && L.Y < StoreY1;
}

FString AStreetStage::PlaceName(const FVector& At) const
{
	if (IsInsideStore(At))
	{
		return TEXT("LUCKY PENNY #212");
	}
	const FVector L = GetActorTransform().InverseTransformPosition(At);
	return L.Y > StoreY1 - 100.0 ? TEXT("FIFTH & MARKET") : TEXT("FIFTH STREET");
}

const FStreetSpot* AStreetStage::SpotFor(const FVector& Eye, const FVector& Dir) const
{
	const FTransform& T = GetActorTransform();
	const FStreetSpot* Best = nullptr;
	double BestScore = -1.0;
	for (const FStreetSpot& S : Spots)
	{
		const FVector At = T.TransformPosition(S.At);
		const FVector To = At - Eye;
		const double Flat = FVector(To.X, To.Y, 0.0).Size();
		if (Flat > S.Radius + 80.0)
		{
			continue;
		}
		// Facing it matters more than being exactly on it: the shelves are long.
		const double Facing = FVector::DotProduct(FVector(To.X, To.Y, 0.0).GetSafeNormal(), FVector(Dir.X, Dir.Y, 0.0).GetSafeNormal());
		if (Facing < 0.2 && Flat > 60.0)
		{
			continue;
		}
		const double Score = Facing - Flat / (S.Radius + 80.0);
		if (Score > BestScore)
		{
			BestScore = Score;
			Best = &S;
		}
	}
	return Best;
}
