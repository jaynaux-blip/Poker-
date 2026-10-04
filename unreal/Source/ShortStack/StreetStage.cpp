#include "StreetStage.h"

#include "BlenderProps.h"

#include "Camera/PlayerCameraManager.h"
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
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ShortStack/Game/Store.h"
#include "ShortStack/UI/PropArt.h"
#include "SlateDrawList.h"
#include "UObject/ConstructorHelpers.h"

#include <string>

namespace StreetStageDetail
{
// The street's plan (cm).
const double SidewalkW = 420.0;
const double RoadX0 = 420.0;
const double RoadX1 = 1620.0;
const double FarFront = 2040.0;
const double EndY0 = -6200.0; // the set's ends: a building across Fifth at each
const double EndY1 = 9200.0;
const double StoreY0 = 3700.0;
const double StoreY1 = 4950.0;
const double MarketS = 5250.0; // Market's road, between its two sidewalks
const double MarketN = 5850.0;
const double MarketY1 = 6150.0; // the building line on Market's far side
const double MarketWest = -3000.0;
const double StoreDepth = 1400.0;
const double StoreCeiling = 380.0;
const double DoorY0 = 4650.0;
const double DoorY1 = 4850.0;
// The sliding doors run on a track just inside the fixed glass (X -12..-10), clear of its mullions (X -18..-6) and of
// the low wall under the glass (X -20..0) as they slide behind it: each leaf spans X -26.5..-20.5.
const double DoorTrackX = -23.5;
// Blender's building fronts (art/blender/assets/facades.py, tenement.py), each modeled facing Blender's -X with its origin on
// its face at street level: its own rooms reach 3.6 m back into the block, so the block's massing starts behind them.
const double FrontBack = 365.0;
const double BrownstoneW = 675.0;
// The laundromat's building, as the apartment's window sees it: NightOneStage stands its middle 6 m to the right of the
// window, which is 3.2 m right of door 1812 here (facades.py). 36 m wide.
const double TenementY = 920.0;
const double TenementHalf = 1800.0;

// Patterns understood by M_Street (Content/Python/street_setup.py); 0 is plain.
const float PatBrick = 1.0f;
const float PatAsphalt = 2.0f;
const float PatSlabs = 3.0f;
const float PatTiles = 4.0f;
const float PatConcrete = 5.0f;
const float PatShutter = 6.0f;

// Window looks (M_StreetWindow's Kind).
const int32 WinDark = 0;
const int32 WinWarm = 1;
const int32 WinCool = 2;
const int32 WinTv = 3;

/** One layer of rain: its distance from the eye, the spacing of its streaks, their width and length (cm), its strength. */
struct FRainLayer
{
	double Depth;
	float Spacing;
	float Width;
	float Length;
	float Strength;
};
const FRainLayer RainLayers[4] = {{220.0, 6.0f, 0.22f, 22.0f, 1.0f}, {520.0, 11.0f, 0.4f, 30.0f, 0.85f}, {1000.0, 18.0f, 0.7f, 40.0f, 0.7f}, {1800.0, 28.0f, 1.1f, 55.0f, 0.55f}};

// The look, tunable live while walking around (the exposure in EV100: higher is darker).
TAutoConsoleVariable<float> CVarNightEV(TEXT("ss.Street.NightEV"), 1.0f, TEXT("The street's exposure floor at night (EV100): higher keeps the night darker."));
TAutoConsoleVariable<float> CVarNightRange(TEXT("ss.Street.NightRange"), 1.6f, TEXT("How far the street's exposure may rise over its floor (looking into the store's light)."));
TAutoConsoleVariable<float> CVarDayEV(TEXT("ss.Street.DayEV"), 10.0f, TEXT("The street's exposure floor by day (EV100)."));
TAutoConsoleVariable<float> CVarStoreEV(TEXT("ss.Street.StoreEV"), 5.0f, TEXT("The Lucky Penny's exposure floor (EV100)."));
TAutoConsoleVariable<float> CVarStoreRange(TEXT("ss.Street.StoreRange"), 2.2f, TEXT("How far the store's exposure may rise over its floor."));
TAutoConsoleVariable<float> CVarBias(TEXT("ss.Street.Bias"), 0.0f, TEXT("Extra exposure compensation on the street (stops; + brighter)."));
TAutoConsoleVariable<float> CVarRain(TEXT("ss.Street.Rain"), 1.0f, TEXT("The rain's strength (0 hides it)."));
TAutoConsoleVariable<float> CVarTenementGlow(TEXT("ss.Street.TenementGlow"), 0.4f,
	TEXT("How brightly the tenement's rooms glow from the street, against their bake for the apartment's view."));
TAutoConsoleVariable<float> CVarLaundromatGlow(TEXT("ss.Street.LaundromatGlow"), 0.08f,
	TEXT("How brightly the laundromat's shop window (the tenement's brightest bake) glows from the street."));

FLinearColor SrgbHex(uint32 Hex)
{
	return FLinearColor::FromSRGBColor(FColor(static_cast<uint8>((Hex >> 16) & 0xff), static_cast<uint8>((Hex >> 8) & 0xff), static_cast<uint8>(Hex & 0xff)));
}

UMaterialInterface* OptionalMaterial(const TCHAR* Path)
{
	return LoadObject<UMaterialInterface>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
}

/** A mesh from the Blender pipeline (unreal/Art/Meshes, imported when the editor opens), nullptr when it isn't. */
UStaticMesh* ImportedMesh(const TCHAR* Name)
{
	return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/ShortStack/Meshes/%s/%s.%s"), Name, Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
}

/**
 * An imported front's own light (the rooms behind its lit windows, its lanterns, a lit transom: all baked for the night)
 * at Level times its bake. The dimmed copies are made only while it's day and never saved, like the cars' paint
 * (PaintCars): a saved copy would tie the level to the front's own materials, and the importer then won't refresh them.
 * The glTF importer's materials scale their emissive texture by a color factor (EmissiveFactor, _RGB in this engine) or
 * a strength; whichever a material lists is scaled, and a slot that doesn't glow is left alone.
 */
void DimImportedGlow(UStaticMeshComponent* Mesh, float Level)
{
	const UStaticMesh* Source = Mesh ? Mesh->GetStaticMesh().Get() : nullptr;
	if (!Source)
	{
		return;
	}
	auto Key = [](const FName& Name) { return Name.ToString().ToLower().Replace(TEXT("_"), TEXT("")).Replace(TEXT(" "), TEXT("")); };
	for (int32 Slot = 0; Slot < Source->GetStaticMaterials().Num(); ++Slot)
	{
		UMaterialInterface* Baked = Source->GetMaterial(Slot);
		UMaterialInstanceDynamic* Dim = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(Slot));
		if (!Baked || Level > 0.999f)
		{
			if (Dim)
			{
				Mesh->SetMaterial(Slot, nullptr); // the bake again
			}
			continue;
		}
		auto Dimmed = [&]() -> UMaterialInstanceDynamic* {
			if (!Dim || Dim->Parent != Baked)
			{
				Dim = UMaterialInstanceDynamic::Create(Baked, Mesh);
				Dim->SetFlags(RF_Transient);
				Mesh->SetMaterial(Slot, Dim);
			}
			return Dim;
		};
		// A slot that lists a color factor glows by it: black (the walls, the glass), it doesn't glow and keeps its bake,
		// whatever strength it lists too.
		bool bFactor = false;
		TArray<FMaterialParameterInfo> Infos;
		TArray<FGuid> Ids;
		Baked->GetAllVectorParameterInfo(Infos, Ids);
		for (const FMaterialParameterInfo& Info : Infos)
		{
			const FString K = Key(Info.Name);
			FLinearColor Glow;
			if ((K.StartsWith(TEXT("emissivefactor")) || K == TEXT("emissivecolor")) && Baked->GetVectorParameterValue(FHashedMaterialParameterInfo(Info), Glow))
			{
				bFactor = true;
				if (!Glow.IsAlmostBlack())
				{
					Dimmed()->SetVectorParameterValue(Info.Name, FLinearColor(Glow.R * Level, Glow.G * Level, Glow.B * Level, Glow.A));
				}
			}
		}
		if (bFactor)
		{
			continue;
		}
		Infos.Reset();
		Ids.Reset();
		Baked->GetAllScalarParameterInfo(Infos, Ids);
		for (const FMaterialParameterInfo& Info : Infos)
		{
			const FString K = Key(Info.Name);
			float Strength = 0.0f;
			if ((K == TEXT("emissivestrength") || K == TEXT("emissiveintensity")) && Baked->GetScalarParameterValue(FHashedMaterialParameterInfo(Info), Strength) && Strength > 0.0f)
			{
				Dimmed()->SetScalarParameterValue(Info.Name, Strength * Level);
			}
		}
	}
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

/** Lamps over the street go dark between these (Session::Daylight): a photocell each, a little apart. */
float NightLevel(float Daylight)
{
	return 1.0f - FMath::SmoothStep(0.35f, 0.65f, Daylight);
}

} // namespace StreetStageDetail

using namespace StreetStageDetail;

AStreetStage::AStreetStage()
{
	PrimaryActorTick.bCanEverTick = true;
	// After the camera has moved this frame (the rain sheets and the exposure follow where it is now).
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
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
	UStaticMesh* Mesh = ImportedMesh(Name);
	if (!Mesh)
	{
		return nullptr;
	}
	UStaticMeshComponent* C = NewPart<UStaticMeshComponent>();
	C->SetStaticMesh(Mesh);
	C->SetRelativeLocationAndRotation(At, BlenderFacing(Yaw));
	C->SetRelativeScale3D(Scale);
	C->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	return C;
}

UStaticMeshComponent* AStreetStage::Facade(const TCHAR* Name, double Y, bool bFar)
{
	UStaticMesh* Mesh = ImportedMesh(Name);
	if (!Mesh)
	{
		return nullptr;
	}
	UStaticMeshComponent* C = NewPart<UStaticMeshComponent>();
	C->SetStaticMesh(Mesh);
	// Turned as the apartment turns the tenement, not as the props are (BlenderFacing): the importer keeps Blender's -X
	// front facing -X (it mirrors Y), so the far side stands as modeled and the home side turns half round.
	C->SetRelativeLocationAndRotation(FVector(bFar ? FarFront : 0.0, Y, 0.0), FRotator(0.0f, bFar ? 0.0f : 180.0f, 0.0f));
	// The importer's collision is one hull round all of it (the stoops and fire escapes, the store under its upper floors):
	// FrontBox blocks what should.
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LitFronts.Add(C);
	return C;
}

void AStreetStage::FrontBox(double Y, bool bFar, double X0, double X1, double Y0, double Y1, double Z0, double Z1)
{
	// The front's meters: x out of its face toward the street negative; y up the street on the home side (turned half
	// round), down it on the far side.
	const double Ax = bFar ? FarFront + X0 * 100.0 : -X0 * 100.0;
	const double Bx = bFar ? FarFront + X1 * 100.0 : -X1 * 100.0;
	const double Ay = bFar ? Y - Y0 * 100.0 : Y + Y0 * 100.0;
	const double By = bFar ? Y - Y1 * 100.0 : Y + Y1 * 100.0;
	Box(nullptr, FVector(FMath::Min(Ax, Bx), FMath::Min(Ay, By), Z0 * 100.0), FVector(FMath::Max(Ax, Bx), FMath::Max(Ay, By), Z1 * 100.0))->SetVisibility(false);
}

bool AStreetStage::BrownstoneRow(double Y0, double Y1, bool bFar, UMaterialInterface* Massing)
{
	if (!ImportedMesh(TEXT("SM_Facade_Brownstone")))
	{
		return false;
	}
	// Row houses: the same front side by side, the stoop at the same end of each (mirrored, their signs would read
	// backwards).
	const int32 Count = FMath::Max(1, static_cast<int32>((Y1 - Y0) / BrownstoneW + 0.01));
	for (int32 K = 0; K < Count; ++K)
	{
		const double Y = Y0 + BrownstoneW * (K + 0.5);
		Facade(TEXT("SM_Facade_Brownstone"), Y, bFar);
		// The stoop and the areaway's fence take 2 m of the sidewalk; over them the bay stands 80 cm out from the face.
		FrontBox(Y, bFar, -2.05, 0.6, -3.375, 3.375, 0.0, 1.6);
		FrontBox(Y, bFar, -0.85, 0.6, -3.375, 3.375, 1.6, 12.1);
	}
	const double BackX0 = bFar ? FarFront + FrontBack : -1500.0;
	const double BackX1 = bFar ? FarFront + 1500.0 : -FrontBack;
	Box(Massing, FVector(BackX0, Y0, 0.0), FVector(BackX1, Y0 + Count * BrownstoneW, 1190.0));
	return true;
}

UInstancedStaticMeshComponent* AStreetStage::StockRows(UStaticMesh* Mesh)
{
	UInstancedStaticMeshComponent* Rows = NewPart<UInstancedStaticMeshComponent>();
	Rows->SetStaticMesh(Mesh);
	// The cooler and the gondolas block; what's on their shelves doesn't.
	Rows->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	return Rows;
}

bool AStreetStage::StockCooler(const FTransform& Cooler)
{
	// Each kind of row one instanced mesh: S soda, C cans, E energy, W water, T tea and juice, I ice, D deli and dairy.
	static const TCHAR* const Kinds = TEXT("SCEWTID");
	static const TCHAR* const Names[7] = {TEXT("SM_Stock_CoolerRow_Soda"), TEXT("SM_Stock_CoolerRow_Cans"), TEXT("SM_Stock_CoolerRow_Energy"), TEXT("SM_Stock_CoolerRow_Water"),
		TEXT("SM_Stock_CoolerRow_Tea"), TEXT("SM_Stock_CoolerRow_Ice"), TEXT("SM_Stock_CoolerRow_Deli")};
	UStaticMesh* Meshes[7];
	for (int32 K = 0; K < 7; ++K)
	{
		Meshes[K] = ImportedMesh(Names[K]);
		if (!Meshes[K])
		{
			return false;
		}
	}
	UInstancedStaticMeshComponent* Rows[7];
	for (int32 K = 0; K < 7; ++K)
	{
		Rows[K] = StockRows(Meshes[K]);
	}
	// Door by door from the Market end, shelf by shelf from the bottom, under the header's sections (cooler.py): COLD
	// DRINKS 0-1, ENERGY 2, WATER 3-4, JUICE 5-6, ICE 7, DAIRY 8-9. No two doors stocked alike.
	static const TCHAR* const Doors[10] = {TEXT("CSSS"), TEXT("SCSC"), TEXT("EEEE"), TEXT("WWWW"), TEXT("WWWT"), TEXT("TTTT"), TEXT("TTSE"), TEXT("IIIW"), TEXT("DDTD"), TEXT("DTDC")};
	static const double ShelfZ[4] = {28.0, 73.0, 118.0, 163.0};
	for (int32 Door = 0; Door < 10; ++Door)
	{
		for (int32 Level = 0; Level < 4; ++Level)
		{
			const TCHAR* At = FCString::Strchr(Kinds, Doors[Door][Level]);
			const int32 K = At ? static_cast<int32>(At - Kinds) : 0;
			// A row's origin is on its shelf's top at the door's middle, on the cooler's own center plane: placed in the
			// cooler's frame, it turns with the cooler.
			Rows[K]->AddInstance(FTransform(FVector(-342.0 + 76.0 * Door, 0.0, ShelfZ[Level])) * Cooler);
		}
	}
	return true;
}

bool AStreetStage::StockShelves(const TArray<FTransform>& Gondolas)
{
	// C chips, T trail mix and jerky, S sweets, G grocery, H household; B the bulk packs on every base deck.
	static const TCHAR* const Kinds = TEXT("CTSGHB");
	static const TCHAR* const Names[6] = {TEXT("SM_Stock_ShelfRow_Chips"), TEXT("SM_Stock_ShelfRow_Trail"), TEXT("SM_Stock_ShelfRow_Candy"), TEXT("SM_Stock_ShelfRow_Grocery"),
		TEXT("SM_Stock_ShelfRow_Household"), TEXT("SM_Stock_ShelfRow_Bulk")};
	UStaticMesh* Meshes[6];
	for (int32 K = 0; K < 6; ++K)
	{
		Meshes[K] = ImportedMesh(Names[K]);
		if (!Meshes[K])
		{
			return false;
		}
	}
	UInstancedStaticMeshComponent* Rows[6];
	for (int32 K = 0; K < 6; ++K)
	{
		Rows[K] = StockRows(Meshes[K]);
	}
	// Each gondola's two faces bay by bay along the run (its X), the top shelf, the middle, the low: the snacks face the
	// aisle between the two (the first one's +Y face, the second's -Y), the groceries and the household face out.
	static const TCHAR* const Snacks[2][3] = {{TEXT("CTCCT"), TEXT("GCTGC"), TEXT("SSCSS")}, {TEXT("TCCTC"), TEXT("CGCTG"), TEXT("SCSSC")}};
	static const TCHAR* const Staples[2][3] = {{TEXT("GHGGH"), TEXT("GGHGG"), TEXT("HHGHH")}, {TEXT("HGGHG"), TEXT("GHGGH"), TEXT("HGHHG")}};
	static const double LevelZ[3] = {130.0, 85.0, 40.0};
	for (int32 G = 0; G < Gondolas.Num() && G < 2; ++G)
	{
		for (int32 Face = 0; Face < 2; ++Face)
		{
			// A row's packs face its gondola's +Y here (Blender's -Y: the importer mirrors Y); turned round for the other face.
			const bool bPlusY = (G == 0) == (Face == 0);
			const FRotator Turn(0.0f, bPlusY ? 0.0f : 180.0f, 0.0f);
			const TCHAR* const* Plan = Face == 0 ? Snacks[G] : Staples[G];
			for (int32 Bay = 0; Bay < 5; ++Bay)
			{
				// The bays every 1.2 m from the run's middle, the end ones a centimeter in.
				const double X = FMath::Clamp(-240.0 + 120.0 * Bay, -239.0, 239.0);
				for (int32 Level = 0; Level < 3; ++Level)
				{
					const TCHAR* At = FCString::Strchr(Kinds, Plan[Level][Bay]);
					Rows[At ? static_cast<int32>(At - Kinds) : 0]->AddInstance(FTransform(Turn, FVector(X, 0.0, LevelZ[Level])) * Gondolas[G]);
				}
				Rows[5]->AddInstance(FTransform(Turn, FVector(X, 0.0, 12.0)) * Gondolas[G]);
			}
		}
	}
	return true;
}

UWidgetComponent* AStreetStage::Sign(const FString& Art, const FVector& At, float Yaw, const FVector2D& SizeCm, const FIntPoint& Pixels, float Glow, bool bTwoSided, USceneComponent* Parent)
{
	UWidgetComponent* Wc = NewPart<UWidgetComponent>(Parent);
	// Printed signs are lit like the walls they hang on; signs that shine glow past white, so they bloom in the rain.
	const bool bPrinted = Glow == 0.0f && WidgetLitMaterial;
	const float Shine = Glow == 0.0f ? 1.0f : Glow;
	Wc->SetWidgetSpace(EWidgetSpace::World);
	Wc->SetDrawSize(FVector2D(Pixels.X, Pixels.Y));
	Wc->SetPivot(FVector2D(0.5, 0.5));
	Wc->SetBlendMode(bPrinted ? EWidgetBlendMode::Opaque : EWidgetBlendMode::Transparent);
	Wc->SetTwoSided(bTwoSided);
	Wc->SetBackgroundColor(bPrinted ? FLinearColor(0.02f, 0.02f, 0.02f, 1.0f) : FLinearColor::Transparent);
	Wc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Wc->SetCastShadow(false);
	Wc->SetRelativeLocationAndRotation(At, FRotator(0.0f, Yaw, 0.0f));
	// The widget quad lies in its local Y (width) / Z (height) plane, one unit per pixel.
	Wc->SetRelativeScale3D(FVector(1.0, SizeCm.X / Pixels.X, SizeCm.Y / Pixels.Y));
	if (bPrinted)
	{
		Wc->SetMaterial(0, WidgetLitMaterial);
	}
	const float Tint = FMath::Abs(Shine);
	Wc->SetTintColorAndOpacity(FLinearColor(Tint, Tint, Tint, 1.0f));
	SignWidgets.Add(Wc);
	SignArt.Add(Art);
	SignGlow.Add(bPrinted ? 0.0f : Shine);
	return Wc;
}

void AStreetStage::Window(const FVector& Center, float Yaw, const FVector2D& SizeCm, int32 Kind, float Seed, float Level)
{
	// One instanced mesh holds every window when M_StreetWindow exists (each instance says how it's lit); without it,
	// one per look, plain emissive.
	const int32 Look = FMath::Clamp(Kind, WinDark, WinTv);
	const int32 Key = WindowMaterial ? -1 : Look;
	UInstancedStaticMeshComponent* Set = nullptr;
	if (TObjectPtr<UInstancedStaticMeshComponent>* Found = WindowSets.Find(Key))
	{
		Set = Found->Get();
	}
	if (!Set)
	{
		Set = NewPart<UInstancedStaticMeshComponent>();
		Set->SetStaticMesh(CubeMesh);
		Set->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Set->SetCastShadow(false);
		if (WindowMaterial)
		{
			UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(WindowMaterial, this);
			M->SetScalarParameterValue(TEXT("Glow"), 1.0f);
			WindowMids.Add(M);
			Set->SetMaterial(0, M);
			Set->SetNumCustomDataFloats(3);
		}
		else
		{
			static const uint32 Colors[4] = {0x0d1014, 0xffc98a, 0x9fc3ff, 0x7f9fff};
			static const float Glows[4] = {0.0f, 1.1f, 0.8f, 0.6f};
			UMaterialInstanceDynamic* M = Mat(*FString::Printf(TEXT("Window%d"), Look), Colors[Look], Look == WinDark ? 0.15f : 0.4f, 0.0f, Glows[Look], Look == WinDark ? 0.3f : 0.0f);
			if (Look != WinDark)
			{
				NightGlowMids.Add(M);
				NightGlowLevels.Add(Glows[Look]);
			}
			Set->SetMaterial(0, M);
		}
		WindowSets.Add(Key, Set);
	}
	// The engine cube, 5 cm deep: its faces along the window's X are the glass.
	const int32 Index = Set->AddInstance(FTransform(FRotator(0.0f, Yaw, 0.0f), Center, FVector(0.05, SizeCm.X / 100.0, SizeCm.Y / 100.0)));
	if (WindowMaterial && Index != INDEX_NONE)
	{
		const float Data[3] = {static_cast<float>(Look), Seed, Level};
		Set->SetCustomData(Index, TArrayView<const float>(Data, 3), true);
	}
}

void AStreetStage::WindowRows(const FVector& Origin, const FVector& Along, double Length, float Yaw, double FirstZ, double Top, const FVector2D& Size, double Pitch,
	double FloorH, double LitShare, int64 Seed)
{
	FStreetDice Dice;
	Dice.S = FMath::Max<int64>(1, Seed % 2147483646);
	for (double Z = FirstZ; Z + Size.Y + 40.0 < Top; Z += FloorH)
	{
		for (double S = Pitch * 0.6; S + Size.X * 0.5 + 40.0 < Length; S += Pitch)
		{
			// Mostly dark at this hour; the lit ones warm lamps, a few cold screens and TVs.
			const double Roll = Dice.Next();
			const int32 Kind = Roll < LitShare * 0.68 ? WinWarm : Roll < LitShare * 0.86 ? WinCool : Roll < LitShare ? WinTv : WinDark;
			const float Level = static_cast<float>(0.55 + 0.75 * Dice.Next());
			Window(Origin + Along * S + FVector(0.0, 0.0, Z + Size.Y * 0.5), Yaw, Size, Kind, static_cast<float>(Dice.Next()), Level);
		}
	}
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
	WindowSets.Reset();
	SkyMids.Reset();
	WindowMids.Reset();
	NightGlowMids.Reset();
	NightGlowLevels.Reset();
	RainSheets.Reset();
	RainMids.Reset();
	Streetlights.Reset();
	StoreLights.Reset();
	NightLights.Reset();
	NightLightLevels.Reset();
	SignWidgets.Reset();
	SignArt.Reset();
	SignGlow.Reset();
	ParkedCars.Reset();
	ParkedPaints.Reset();
	LitFronts.Reset();
	bHomeFront = false;
	DoorLeft = nullptr;
	DoorRight = nullptr;
	DoorDecal = nullptr;
	SkyLight = nullptr;
	Moon = nullptr;
	Fog = nullptr;
	Lens = nullptr;
	StreetMaterial = OptionalMaterial(TEXT("/Game/ShortStack/Materials/M_Street.M_Street"));
	SurfaceMaterial = OptionalMaterial(TEXT("/Game/ShortStack/Materials/M_Surface.M_Surface"));
	GlowMaterial = OptionalMaterial(TEXT("/Game/ShortStack/Materials/M_Glow.M_Glow"));
	GlassMaterial = OptionalMaterial(TEXT("/Game/ShortStack/Materials/M_RainGlass.M_RainGlass"));
	StoreGlassMaterial = OptionalMaterial(TEXT("/Game/ShortStack/Materials/M_StreetGlass.M_StreetGlass"));
	WindowMaterial = OptionalMaterial(TEXT("/Game/ShortStack/Materials/M_StreetWindow.M_StreetWindow"));
	WidgetLitMaterial = OptionalMaterial(TEXT("/Game/ShortStack/Materials/M_WidgetLit.M_WidgetLit"));
	// The street's own sky, skyline and rain (street_setup.py) have a strength to set for the hour; the apartment's
	// do for the night.
	SkyMaterial = OptionalMaterial(TEXT("/Game/ShortStack/Materials/M_StreetSky.M_StreetSky"));
	if (!SkyMaterial)
	{
		SkyMaterial = OptionalMaterial(TEXT("/Game/ShortStack/Materials/M_Sky.M_Sky"));
	}
	CityMaterial = OptionalMaterial(TEXT("/Game/ShortStack/Materials/M_StreetCity.M_StreetCity"));
	if (!CityMaterial)
	{
		CityMaterial = OptionalMaterial(TEXT("/Game/ShortStack/Materials/M_City.M_City"));
	}
	RainMaterial = OptionalMaterial(TEXT("/Game/ShortStack/Materials/M_StreetRain.M_StreetRain"));
	bRainMasksStore = RainMaterial != nullptr;
	if (!RainMaterial)
	{
		RainMaterial = OptionalMaterial(TEXT("/Game/ShortStack/Materials/M_Rain.M_Rain"));
	}
	BuildGround();
	BuildEnds();
	BuildHomeBlock();
	BuildEntrance();
	BuildAcross();
	BuildStore();
	BuildStoreDressing();
	BuildStreetlights();
	BuildSkyAndWeather();
	BuildSpots();
	AppliedDaylight = -1.0f;
	SignScale = -1.0f;
	RainScale = -1.0f;
	RainPixelAngle = -1.0f;
}

void AStreetStage::BuildGround()
{
	UMaterialInterface* Walk = Mat(TEXT("Sidewalk"), 0x6f6c66, 0.75f, PatSlabs);
	UMaterialInterface* Road = Mat(TEXT("Asphalt"), 0x26272a, 0.6f, PatAsphalt);
	UMaterialInterface* Curb = Mat(TEXT("Curb"), 0x8a867e, 0.7f, PatConcrete);
	// Fifth's sidewalk on the home side (its top at 0) and its curb; at the corner Market's two sidewalks, its road
	// between them 15 cm down, and Fifth's sidewalk again past it. Nothing overlaps: a shared top would flicker.
	Box(Walk, FVector(0.0, EndY0, -30.0), FVector(SidewalkW - 20.0, StoreY1, 0.0));
	Box(Curb, FVector(SidewalkW - 20.0, EndY0, -30.0), FVector(SidewalkW, MarketS - 20.0, 0.0));
	Box(Walk, FVector(MarketWest, StoreY1, -30.0), FVector(SidewalkW - 20.0, MarketS - 20.0, 0.0));
	Box(Curb, FVector(MarketWest, MarketS - 20.0, -30.0), FVector(SidewalkW, MarketS, 0.0));
	Box(Road, FVector(MarketWest, MarketS, -45.0), FVector(RoadX0, MarketN, -15.0));
	Box(Curb, FVector(MarketWest, MarketN, -30.0), FVector(SidewalkW, MarketN + 20.0, 0.0));
	Box(Walk, FVector(MarketWest, MarketN + 20.0, -30.0), FVector(SidewalkW - 20.0, MarketY1, 0.0));
	Box(Walk, FVector(0.0, MarketY1, -30.0), FVector(SidewalkW - 20.0, EndY1, 0.0));
	Box(Curb, FVector(SidewalkW - 20.0, MarketN + 20.0, -30.0), FVector(SidewalkW, EndY1, 0.0));
	// The road and the far side.
	Box(Road, FVector(RoadX0, EndY0, -45.0), FVector(RoadX1, EndY1, -15.0));
	Box(Curb, FVector(RoadX1, EndY0, -30.0), FVector(RoadX1 + 20.0, EndY1, 0.0));
	Box(Walk, FVector(RoadX1 + 20.0, EndY0, -30.0), FVector(FarFront, EndY1, 0.0));

	// The paint, a few instanced bars: the double yellow down Fifth (not through the corner), the crosswalks in line
	// with Market's sidewalks and across its mouth, Market's own center line and stop line.
	auto PaintSet = [this](UMaterialInterface* M) {
		UInstancedStaticMeshComponent* Set = NewPart<UInstancedStaticMeshComponent>();
		Set->SetStaticMesh(CubeMesh);
		Set->SetMaterial(0, M);
		Set->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Set->SetCastShadow(false);
		return Set;
	};
	auto Bar = [](UInstancedStaticMeshComponent* Set, double X0, double Y0, double X1, double Y1) {
		// 4 mm of paint on the road's top (Z -15).
		Set->AddInstance(FTransform(FRotator::ZeroRotator, FVector((X0 + X1) * 0.5, (Y0 + Y1) * 0.5, -14.8), FVector((X1 - X0) / 100.0, (Y1 - Y0) / 100.0, 0.004)));
	};
	UInstancedStaticMeshComponent* White = PaintSet(Mat(TEXT("RoadPaint"), 0xd8d4c8, 0.35f));
	UInstancedStaticMeshComponent* Yellow = PaintSet(Mat(TEXT("RoadYellow"), 0xd9a520, 0.35f));
	for (const double Line : {1008.0, 1030.0})
	{
		Bar(Yellow, Line, EndY0, Line + 12.0, StoreY1 - 200.0);
		Bar(Yellow, Line, MarketY1 + 200.0, Line + 12.0, EndY1);
	}
	for (double X = RoadX0 + 40.0; X + 50.0 < RoadX1 - 30.0; X += 100.0)
	{
		Bar(White, X, StoreY1 + 40.0, X + 50.0, MarketS - 40.0);
		Bar(White, X, MarketN + 40.0, X + 50.0, MarketY1 - 20.0);
	}
	for (double Y = MarketS + 40.0; Y + 50.0 < MarketN - 30.0; Y += 100.0)
	{
		Bar(White, 30.0, Y, 380.0, Y + 50.0);
	}
	const double MarketMid = (MarketS + MarketN) * 0.5;
	Bar(Yellow, MarketWest, MarketMid - 6.0, -140.0, MarketMid + 6.0);
	Bar(White, -130.0, MarketS + 20.0, -100.0, MarketMid - 20.0);

	// Storm drains at the curb and a couple of manhole covers, wet steel in the lamplight.
	UMaterialInterface* Grate = Mat(TEXT("Grate"), 0x1a1b1d, 0.35f, 0.0f, 0.0f, 0.8f);
	Box(Grate, FVector(SidewalkW + 2.0, 2200.0, -15.0), FVector(SidewalkW + 60.0, 2300.0, -14.6), false)->SetCastShadow(false);
	Box(Grate, FVector(RoadX1 - 60.0, -1900.0, -15.0), FVector(RoadX1 - 2.0, -1800.0, -14.6), false)->SetCastShadow(false);
	for (const FVector2D& At : {FVector2D(780.0, -900.0), FVector2D(1260.0, 3300.0), FVector2D(-900.0, MarketMid + 140.0)})
	{
		Cyl(Grate, FVector(At.X, At.Y, -15.0), 32.0f, 0.6f, false)->SetCastShadow(false);
	}
}

void AStreetStage::BuildEnds()
{
	UMaterialInterface* BrickEnd = Mat(TEXT("BrickEnd"), 0x553328, 0.85f, PatBrick);
	UMaterialInterface* BrickDark = Mat(TEXT("BrickDark"), 0x4a2a24, 0.85f, PatBrick);
	UMaterialInterface* BrickMarket = Mat(TEXT("BrickMarket"), 0x5e3b2c, 0.85f, PatBrick);
	UMaterialInterface* Stucco = Mat(TEXT("StuccoEnd"), 0x7c766b, 0.9f, PatConcrete);
	UMaterialInterface* Shutter = Mat(TEXT("Shutter"), 0x5d6166, 0.5f, PatShutter, 0.0f, 0.6f);
	UMaterialInterface* Concrete = Mat(TEXT("Barrier"), 0xa8a49a, 0.8f, PatConcrete);
	UMaterialInterface* Orange = Mat(TEXT("BarrierStripe"), 0xe8641c, 0.45f);
	UMaterialInterface* Reflect = Mat(TEXT("BarrierWhite"), 0xe8e6e0, 0.4f);
	UMaterialInstanceDynamic* Flasher = Mat(TEXT("Flasher"), 0xffa21f, 0.3f, 0.0f, 6.0f);
	NightGlowMids.Add(Flasher);
	NightGlowLevels.Add(6.0f);
	const FVector2D Win(120.0, 170.0);

	// Fifth stops at a building across each end, the road at the barriers in front of it (the city goes on past them,
	// the street just doesn't). The buildings are the set's walls, so nobody walks off it.
	Box(BrickEnd, FVector(-1500.0, EndY0 - 800.0, 0.0), FVector(FarFront + 1500.0, EndY0, 1500.0));
	Box(BrickDark, FVector(-1500.0, EndY1, 0.0), FVector(FarFront + 1500.0, EndY1 + 800.0, 1700.0));
	// Their windows only where the end wall shows between the street's own fronts (X 0..FarFront): none half sunk in a corner.
	WindowRows(FVector(0.0, EndY0 + 0.5, 0.0), FVector(1.0, 0.0, 0.0), FarFront, 90.0f, 420.0, 1500.0, Win, 260.0, 330.0, 0.25, 515);
	WindowRows(FVector(0.0, EndY1 - 0.5, 0.0), FVector(1.0, 0.0, 0.0), FarFront, -90.0f, 420.0, 1700.0, Win, 260.0, 330.0, 0.25, 919);
	for (const double Y : {EndY0, EndY1})
	{
		const double Out = Y < 0.0 ? 1.0 : -1.0; // toward the street
		Box(Shutter, FVector(60.0, FMath::Min(Y, Y + Out * 4.0), 20.0), FVector(360.0, FMath::Max(Y, Y + Out * 4.0), 290.0), false);
		Box(Shutter, FVector(1680.0, FMath::Min(Y, Y + Out * 4.0), 20.0), FVector(1990.0, FMath::Max(Y, Y + Out * 4.0), 290.0), false);
		// Jersey barriers across the road, striped, an amber flasher on each.
		const double By = Y + Out * 140.0;
		for (double X = RoadX0 + 20.0; X + 280.0 <= RoadX1; X += 300.0)
		{
			Box(Concrete, FVector(X, By - 30.0, -15.0), FVector(X + 280.0, By + 30.0, 25.0));
			Box(Concrete, FVector(X, By - 14.0, 25.0), FVector(X + 280.0, By + 14.0, 80.0));
			for (int32 K = 0; K < 7; ++K)
			{
				const double Sx = X + 10.0 + K * 38.0;
				const double Face = By + Out * 14.0; // the top's face toward the street
				Box((K % 2) ? Reflect : Orange, FVector(Sx, FMath::Min(Face, Face + Out), 40.0), FVector(Sx + 36.0, FMath::Max(Face, Face + Out), 70.0), false)->SetCastShadow(false);
			}
			UStaticMeshComponent* Lamp = NewPart<UStaticMeshComponent>();
			Lamp->SetStaticMesh(SphereMesh);
			Lamp->SetMaterial(0, Flasher);
			Lamp->SetRelativeLocation(FVector(X + 140.0, By, 88.0));
			Lamp->SetRelativeScale3D(FVector(0.12));
			Lamp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Lamp->SetCastShadow(false);
		}
	}

	// Market Street: a block on each side and a building across its far end.
	Box(BrickMarket, FVector(MarketWest, StoreY0, 0.0), FVector(-1420.0, StoreY1, 1300.0));
	WindowRows(FVector(MarketWest, StoreY1 + 0.5, 0.0), FVector(1.0, 0.0, 0.0), -1420.0 - MarketWest, 90.0f, 420.0, 1300.0, Win, 250.0, 330.0, 0.3, 4111);
	Box(Shutter, FVector(MarketWest + 200.0, StoreY1, 20.0), FVector(-1700.0, StoreY1 + 4.0, 290.0), false);
	Box(BrickDark, FVector(-1500.0, MarketY1, 0.0), FVector(0.0, EndY1, 1600.0));
	Box(Stucco, FVector(MarketWest, MarketY1, 0.0), FVector(-1500.0, EndY1, 1200.0));
	WindowRows(FVector(0.5, MarketY1, 0.0), FVector(0.0, 1.0, 0.0), EndY1 - MarketY1, 0.0f, 420.0, 1600.0, Win, 260.0, 330.0, 0.28, 6007);
	WindowRows(FVector(-1500.0, MarketY1 - 0.5, 0.0), FVector(1.0, 0.0, 0.0), 1500.0, -90.0f, 420.0, 1600.0, Win, 250.0, 330.0, 0.28, 6113);
	WindowRows(FVector(MarketWest, MarketY1 - 0.5, 0.0), FVector(1.0, 0.0, 0.0), 1500.0, -90.0f, 380.0, 1200.0, Win, 250.0, 320.0, 0.3, 6229);
	Box(Shutter, FVector(0.0, MarketY1 + 200.0, 20.0), FVector(4.0, MarketY1 + 900.0, 290.0), false);
	Box(Shutter, FVector(-1300.0, MarketY1 - 4.0, 20.0), FVector(-500.0, MarketY1, 290.0), false);
	Box(Stucco, FVector(MarketWest - 800.0, StoreY0, 0.0), FVector(MarketWest, EndY1, 1100.0));
	WindowRows(FVector(MarketWest + 0.5, StoreY1, 0.0), FVector(0.0, 1.0, 0.0), MarketY1 - StoreY1, 0.0f, 380.0, 1100.0, Win, 240.0, 320.0, 0.3, 7001);
}

void AStreetStage::BuildHomeBlock()
{
	FStreetDice Dice;
	UMaterialInterface* Brick = Mat(TEXT("Brick"), 0x6b3427, 0.85f, PatBrick);
	UMaterialInterface* BrickDark = Mat(TEXT("BrickDark"), 0x4a2a24, 0.85f, PatBrick);
	UMaterialInterface* Stone = Mat(TEXT("Stone"), 0x8c8579, 0.8f, PatConcrete);
	UMaterialInterface* Shutter = Mat(TEXT("Shutter"), 0x5d6166, 0.5f, PatShutter, 0.0f, 0.6f);
	UMaterialInterface* Iron = Mat(TEXT("Iron"), 0x1c1e21, 0.45f, 0.0f, 0.0f, 0.7f);
	const FVector2D Win(120.0, 170.0);

	// The apartment building, 1812 Fifth Street (facades.py): buff brick over a limestone base, the door in its recess where
	// the stand-in has it (BuildEntrance), the player's window on the third floor still lit by the laptop; its block behind
	// it. What blocks is its face, the entrance's pilasters and the recess back to the door. Without it, four floors of
	// brick around the door's recess, a cornice.
	bHomeFront = Facade(TEXT("SM_Facade_1812"), 0.0, false) != nullptr;
	if (bHomeFront)
	{
		Box(Mat(TEXT("BrickBuff"), 0x8a7155, 0.85f, PatBrick), FVector(-1500.0, -800.0, 0.0), FVector(-FrontBack, 800.0, 1500.0));
		FrontBox(0.0, false, -0.06, 0.6, -8.0, -1.1, 0.0, 15.2);
		FrontBox(0.0, false, -0.06, 0.6, 1.1, 8.0, 0.0, 15.2);
		FrontBox(0.0, false, -0.06, 0.6, -1.1, 1.1, 2.7, 15.2);
		FrontBox(0.0, false, 0.62, 0.9, -1.1, 1.1, 0.0, 2.7);
		FrontBox(0.0, false, -0.18, -0.06, -1.5, -1.1, 0.0, 2.86);
		FrontBox(0.0, false, -0.18, -0.06, 1.1, 1.5, 0.0, 2.86);
	}
	else
	{
		Box(Brick, FVector(-1500.0, -800.0, 0.0), FVector(0.0, -110.0, 1500.0));
		Box(Brick, FVector(-1500.0, 110.0, 0.0), FVector(0.0, 800.0, 1500.0));
		Box(Brick, FVector(-1500.0, -110.0, 270.0), FVector(0.0, 110.0, 1500.0));
		Box(Brick, FVector(-1500.0, -110.0, 0.0), FVector(-70.0, 110.0, 270.0));
		Box(Stone, FVector(-4.0, -820.0, 1490.0), FVector(18.0, 820.0, 1520.0), false);
		Box(Stone, FVector(-2.0, -800.0, 352.0), FVector(6.0, 800.0, 364.0), false);
		// Its windows, each on a stone sill; the player's, on the third floor, still has the laptop's glow.
		for (int32 Floor = 0; Floor < 3; ++Floor)
		{
			for (int32 W = 0; W < 4; ++W)
			{
				const double Y = -700.0 + W * 360.0;
				const bool bMine = Floor == 1 && W == 2;
				const double Z0 = 420.0 + Floor * 340.0;
				const double Roll = Dice.Next();
				const int32 Kind = bMine ? WinCool : (Roll < 0.25 ? WinWarm : Roll < 0.32 ? WinTv : WinDark);
				Window(FVector(0.5, Y + 65.0, Z0 + 90.0), 0.0f, FVector2D(130.0, 180.0), Kind, bMine ? 0.37f : static_cast<float>(Dice.Next()), bMine ? 0.55f : 0.8f);
				Box(Stone, FVector(-2.0, Y - 6.0, Z0 - 8.0), FVector(9.0, Y + 136.0, Z0), false);
			}
		}
	}

	// Neighbors along Fifth, from Blender where they're imported: the walk-up over the barber's (its pole lit) and the
	// cleaners', next to the Lucky Penny, and two brownstones next door to the south, their stoops out on the sidewalk.
	// The pawn shop's lit window stays between 1812 and the walk-up; past the brownstones, brick with its shops gated for
	// the night. Without them, the barber's shut in a box of its own and the brick runs up to 1812.
	const double WalkupY = (2300.0 + StoreY0 - 20.0) * 0.5;
	const bool bWalkup = Facade(TEXT("SM_Facade_Walkup"), WalkupY, false) != nullptr;
	if (bWalkup)
	{
		// Its 13.5 m in the 13.8 m between the pawn shop and the store: the gaps show its party walls.
		Box(Brick, FVector(-1500.0, 2300.0, 0.0), FVector(-FrontBack, StoreY0 - 20.0, 1745.0));
		FrontBox(WalkupY, false, -0.06, 0.6, -6.9, 6.9, 0.0, 17.6);
		// What stands out of it at the sidewalk blocks too: the shopfront's cast-iron piers (18 cm), the tenants' granite
		// step (36 cm out, low enough to step onto) and the standpipe's brass inlets at the knee.
		for (const FVector2D& Pier : {FVector2D(-6.77, -6.28), FVector2D(-1.22, -0.6), FVector2D(0.6, 1.22), FVector2D(6.28, 6.77)})
		{
			FrontBox(WalkupY, false, -0.18, -0.06, Pier.X, Pier.Y, 0.0, 3.35);
		}
		FrontBox(WalkupY, false, -0.36, -0.06, -0.62, 0.62, 0.0, 0.16);
		FrontBox(WalkupY, false, -0.4, -0.18, -1.05, -0.77, 0.0, 0.65);
	}
	const double RowY0 = -800.0 - 2.0 * BrownstoneW;
	const bool bBrownstones = BrownstoneRow(RowY0, -800.0, false, BrickDark);
	const double South = bBrownstones ? RowY0 : -800.0;
	const double Mid = bBrownstones ? -3600.0 : -2400.0;
	struct FFront
	{
		double Y0, Y1, Height;
		bool bDark;
		int32 Shop; // 0 a rolled-down gate, 1 the pawn shop, 2 the barber's
	};
	// Each stops short of its neighbor's wall: two faces in one plane would flicker (the store's south wall is at 3680..3700).
	const FFront Fronts[] = {{800.0, 2300.0, 1100.0, false, 1}, {2300.0, StoreY0 - 20.0, 900.0, true, 2}, {Mid, South, 1900.0, true, 0}, {EndY0, Mid, 1300.0, false, 0}};
	int64 Seed = 101;
	for (const FFront& Fr : Fronts)
	{
		const int64 FrontSeed = (Seed++) * 7919;
		if (Fr.Shop == 2 && bWalkup)
		{
			continue;
		}
		Box(Fr.bDark ? BrickDark : Brick, FVector(-1500.0, Fr.Y0, 0.0), FVector(0.0, Fr.Y1, Fr.Height));
		Box(Stone, FVector(-2.0, Fr.Y0, 300.0), FVector(5.0, Fr.Y1, 318.0), false);
		WindowRows(FVector(0.5, Fr.Y0, 0.0), FVector(0.0, 1.0, 0.0), Fr.Y1 - Fr.Y0, 0.0f, 420.0, Fr.Height, Win, 260.0, 330.0, 0.3, FrontSeed);
		const double S0 = Fr.Y0 + 150.0;
		const double S1 = Fr.Y1 - 150.0;
		if (Fr.Shop == 1)
		{
			// The pawn shop's window: lit low behind its bars, guitars and TVs in silhouette, a blue tube around it and
			// the three gold balls over the door.
			UMaterialInterface* Display = Mat(TEXT("PawnDisplay"), 0xffd9a0, 0.6f, 0.0f, 1.3f);
			Box(Display, FVector(-1.0, S0, 40.0), FVector(2.0, S1, 280.0), false);
			UMaterialInterface* Shade = Mat(TEXT("PawnGoods"), 0x15120f, 0.7f);
			for (int32 K = 0; K < 6; ++K)
			{
				const double At = S0 + 90.0 + K * (S1 - S0 - 180.0) / 5.0;
				const double Ht = K % 3 == 0 ? 120.0 : K % 3 == 1 ? 60.0 : 90.0;
				Box(Shade, FVector(2.0, At - 22.0, 40.0), FVector(5.0, At + 22.0, 40.0 + Ht), false)->SetCastShadow(false);
				if (K % 3 == 0)
				{
					Box(Shade, FVector(2.0, At - 4.0, 40.0 + Ht), FVector(5.0, At + 4.0, 40.0 + Ht + 70.0), false)->SetCastShadow(false); // a guitar's neck
				}
			}
			UInstancedStaticMeshComponent* Bars = NewPart<UInstancedStaticMeshComponent>();
			Bars->SetStaticMesh(CubeMesh);
			Bars->SetMaterial(0, Iron);
			Bars->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			for (double Y = S0 + 6.0; Y < S1; Y += 12.0)
			{
				Bars->AddInstance(FTransform(FRotator::ZeroRotator, FVector(8.0, Y, 160.0), FVector(0.012, 0.012, 2.4)));
			}
			UMaterialInstanceDynamic* Tube = Mat(TEXT("NeonBlue"), 0x3aa8ff, 0.3f, 0.0f, 5.0f);
			NightGlowMids.Add(Tube);
			NightGlowLevels.Add(5.0f);
			Box(Tube, FVector(10.0, S0 - 10.0, 286.0), FVector(13.0, S1 + 10.0, 289.0), false)->SetCastShadow(false);
			Box(Tube, FVector(10.0, S0 - 10.0, 31.0), FVector(13.0, S1 + 10.0, 34.0), false)->SetCastShadow(false);
			Box(Tube, FVector(10.0, S0 - 10.0, 31.0), FVector(13.0, S0 - 7.0, 289.0), false)->SetCastShadow(false);
			Box(Tube, FVector(10.0, S1 + 7.0, 31.0), FVector(13.0, S1 + 10.0, 289.0), false)->SetCastShadow(false);
			UMaterialInstanceDynamic* Gold = Mat(TEXT("PawnGold"), 0xd9a520, 0.25f, 0.0f, 2.2f, 0.9f);
			NightGlowMids.Add(Gold);
			NightGlowLevels.Add(2.2f);
			Box(Iron, FVector(0.0, Fr.Y1 - 112.0, 392.0), FVector(70.0, Fr.Y1 - 108.0, 396.0), false);
			for (int32 Ball = 0; Ball < 3; ++Ball)
			{
				const FVector At(58.0, Fr.Y1 - 110.0 + (Ball - 1) * 16.0, Ball == 1 ? 352.0 : 366.0);
				UStaticMeshComponent* B = NewPart<UStaticMeshComponent>();
				B->SetStaticMesh(SphereMesh);
				B->SetMaterial(0, Gold);
				B->SetRelativeLocation(At);
				B->SetRelativeScale3D(FVector(0.15));
				B->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Box(Iron, FVector(At.X - 0.5, At.Y - 0.5, At.Z + 7.0), FVector(At.X + 0.5, At.Y + 0.5, 392.0), false)->SetCastShadow(false);
			}
			if (UPointLightComponent* Glow = NewPart<UPointLightComponent>())
			{
				Glow->SetRelativeLocation(FVector(40.0, (S0 + S1) * 0.5, 200.0));
				Glow->SetIntensityUnits(ELightUnits::Candelas);
				Glow->SetIntensity(30.0f);
				Glow->SetLightColor(SrgbHex(0x7fbfff));
				Glow->SetAttenuationRadius(600.0f);
				Glow->SetCastShadows(false);
				Glow->SetVolumetricScatteringIntensity(0.5f);
				NightLights.Add(Glow);
				NightLightLevels.Add(30.0f);
			}
		}
		else
		{
			// A gate rolled down over the shop front, its housing over it.
			Box(Shutter, FVector(0.0, S0, 20.0), FVector(4.0, S1, 290.0), false);
			Box(Iron, FVector(0.0, S0 - 10.0, 290.0), FVector(16.0, S1 + 10.0, 312.0), false);
			if (Fr.Shop == 2)
			{
				// The barber's pole, banded red, white and blue under its glass cap.
				static const uint32 Bands[3] = {0xc8202c, 0xf0ece4, 0x2048a8};
				for (int32 K = 0; K < 9; ++K)
				{
					UMaterialInterface* Band = Mat(*FString::Printf(TEXT("Barber%d"), K % 3), Bands[K % 3], 0.3f, 0.0f, 0.4f);
					Cyl(Band, FVector(22.0, S0 - 70.0, 150.0 + K * 12.0), 7.0f, 12.0f, false)->SetCastShadow(false);
				}
				UMaterialInstanceDynamic* Cap = Mat(TEXT("BarberCap"), 0xfff4e0, 0.2f, 0.0f, 4.0f);
				NightGlowMids.Add(Cap);
				NightGlowLevels.Add(4.0f);
				UStaticMeshComponent* Top = NewPart<UStaticMeshComponent>();
				Top->SetStaticMesh(SphereMesh);
				Top->SetMaterial(0, Cap);
				Top->SetRelativeLocation(FVector(22.0, S0 - 70.0, 264.0));
				Top->SetRelativeScale3D(FVector(0.16));
				Top->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Box(Iron, FVector(0.0, S0 - 74.0, 200.0), FVector(16.0, S0 - 66.0, 206.0), false);
			}
		}
	}
	// Street furniture from Blender where it's been imported.
	Prop(TEXT("SM_Hydrant"), FVector(360.0, 1300.0, 0.0), 0.0f); // the pumper nozzle to the street
	Prop(TEXT("SM_TrashCan"), FVector(330.0, -500.0, 0.0), 12.0f);
	Prop(TEXT("SM_TrashCan"), FVector(330.0, 3550.0, 0.0), -8.0f);
	Prop(TEXT("SM_TrashCan"), FVector(1720.0, 2500.0, 0.0), 170.0f);
	Prop(TEXT("SM_Newsbox"), FVector(300.0, 3400.0, 0.0), 180.0f); // its window to the sidewalk
}

void AStreetStage::BuildEntrance()
{
	if (bHomeFront)
	{
		// 1812's own front has the entrance where this one stands: the granite step, the recess lined in marble, the door
		// with its lit transom (the number in gold leaf on it), the buzzer panel on the recess's right-hand wall facing the
		// door's approach, the brass number over the opening and a carriage lamp each side. The step blocks; the lanterns
		// each throw a little warm light on the brick and the sidewalk, too faint and too far round to wash the number out.
		Box(nullptr, FVector(-70.0, -112.0, 0.0), FVector(45.0, 112.0, 14.0))->SetVisibility(false);
		for (const double Side : {-1.0, 1.0})
		{
			UPointLightComponent* Lantern = NewPart<UPointLightComponent>();
			Lantern->SetRelativeLocation(FVector(33.0, Side * 185.0, 222.0));
			Lantern->SetIntensityUnits(ELightUnits::Candelas);
			Lantern->SetIntensity(14.0f);
			Lantern->SetUseTemperature(true);
			Lantern->SetTemperature(2600.0f);
			Lantern->SetAttenuationRadius(380.0f);
			Lantern->SetSourceRadius(6.0f);
			// Inside its frosted glass: shadowed, the glass would keep all of it in.
			Lantern->SetCastShadows(false);
			Lantern->SetVolumetricScatteringIntensity(0.4f);
			NightLights.Add(Lantern);
			NightLightLevels.Add(14.0f);
		}
	}
	else
	{
		BuildDoor();
	}
	// The step's light: a spot turned down onto the step and the sidewalk (under the cornice over 1812's entrance, just
	// clear of its entablature, or from the stand-in's jelly jar), none of it up onto the number.
	if (USpotLightComponent* Lamp = NewPart<USpotLightComponent>())
	{
		Lamp->SetRelativeLocationAndRotation(bHomeFront ? FVector(24.0, 0.0, 266.0) : FVector(18.0, 0.0, 270.0), FRotator(-72.0f, 0.0f, 0.0f));
		Lamp->SetIntensityUnits(ELightUnits::Candelas);
		Lamp->SetIntensity(140.0f);
		Lamp->SetUseTemperature(true);
		Lamp->SetTemperature(2700.0f);
		Lamp->SetAttenuationRadius(650.0f);
		Lamp->SetInnerConeAngle(25.0f);
		Lamp->SetOuterConeAngle(62.0f);
		Lamp->SetSourceRadius(6.0f);
		Lamp->SetCastShadows(true);
		Lamp->SetVolumetricScatteringIntensity(0.6f);
		NightLights.Add(Lamp);
		NightLightLevels.Add(140.0f);
	}
}

void AStreetStage::BuildDoor()
{
	// Door 1812: a painted door in the recess (proud of its back wall at X -70, so the two never share a plane), a cream
	// casing, brass, a glass lite with the hall light behind it, the buzzer panel on the recess wall, the number over
	// the opening and a downlight that lights the step without washing the brick or the number out.
	UMaterialInterface* Paint = Mat(TEXT("DoorPaint"), 0x1f3a2e, 0.35f);
	UMaterialInterface* Casing = Mat(TEXT("DoorCasing"), 0xd9d2c3, 0.5f);
	UMaterialInterface* Brass = Mat(TEXT("Brass"), 0xc9a24d, 0.3f, 0.0f, 0.0f, 0.9f);
	UMaterialInterface* Steel = Mat(TEXT("Steel"), 0xa0a4aa, 0.35f, 0.0f, 0.0f, 0.9f);
	UMaterialInterface* Black = Mat(TEXT("Fixture"), 0x141517, 0.4f, 0.0f, 0.0f, 0.6f);
	UMaterialInterface* Stone = Mat(TEXT("Stone"), 0x8c8579, 0.8f, PatConcrete);
	UMaterialInterface* Hall = Mat(TEXT("HallLite"), 0xffc98a, 0.1f, 0.0f, 0.45f);

	Box(Stone, FVector(-70.0, -112.0, 0.0), FVector(45.0, 112.0, 14.0));
	Box(Stone, FVector(30.0, -112.0, 12.0), FVector(47.0, 112.0, 15.0), false); // the step's worn nosing
	Box(Paint, FVector(-68.0, -55.0, 14.0), FVector(-63.0, 55.0, 250.0));
	Box(Paint, FVector(-63.0, -45.0, 30.0), FVector(-62.0, -5.0, 150.0), false);
	Box(Paint, FVector(-63.0, 5.0, 30.0), FVector(-62.0, 45.0, 150.0), false);
	Box(Hall, FVector(-63.5, -40.0, 172.0), FVector(-62.8, 40.0, 236.0), false);
	Box(Paint, FVector(-63.0, -1.5, 172.0), FVector(-62.0, 1.5, 236.0), false);
	Box(Paint, FVector(-63.0, -40.0, 202.5), FVector(-62.0, 40.0, 205.5), false);
	Box(Casing, FVector(-70.0, -68.0, 14.0), FVector(-61.0, -55.0, 262.0), false);
	Box(Casing, FVector(-70.0, 55.0, 14.0), FVector(-61.0, 68.0, 262.0), false);
	Box(Casing, FVector(-70.0, -68.0, 250.0), FVector(-61.0, 68.0, 262.0), false);
	Box(Brass, FVector(-62.8, -55.0, 14.0), FVector(-62.3, 55.0, 34.0), false); // kick plate
	Box(Brass, FVector(-62.0, 34.0, 98.0), FVector(-58.0, 47.0, 101.0), false);  // the lever
	Box(Brass, FVector(-62.5, 41.0, 92.0), FVector(-61.5, 47.0, 110.0), false);  // its rose
	Box(Brass, FVector(-62.4, 42.0, 116.0), FVector(-61.2, 46.0, 120.0), false); // the deadbolt
	// The buzzer panel on the recess's right-hand wall (its face at Y 110, facing the door's approach).
	Box(Steel, FVector(-46.0, 108.0, 118.0), FVector(-20.0, 110.0, 178.0), false);
	UMaterialInterface* Label = Mat(TEXT("BuzzerLabel"), 0xe8e2d0, 0.6f);
	for (int32 Row = 0; Row < 4; ++Row)
	{
		for (int32 Col = 0; Col < 2; ++Col)
		{
			const double X = -42.0 + Col * 12.0;
			const double Z = 163.0 - Row * 10.0;
			Box(Brass, FVector(X, 107.2, Z - 1.2), FVector(X + 2.4, 108.0, Z + 1.2), false)->SetCastShadow(false);
			Box(Label, FVector(X + 3.2, 107.6, Z - 1.6), FVector(X + 9.0, 108.0, Z + 1.6), false)->SetCastShadow(false);
		}
	}
	for (int32 Slot = 0; Slot < 5; ++Slot)
	{
		Box(Black, FVector(-41.0, 107.6, 122.0 + Slot * 2.4), FVector(-25.0, 108.0, 123.0 + Slot * 2.4), false)->SetCastShadow(false); // the speaker grille
	}

	// The light: a jelly-jar fixture on the brick over the opening (its spot, BuildEntrance's, turned down onto the step).
	Box(Black, FVector(0.0, -9.0, 282.0), FVector(6.0, 9.0, 300.0), false);
	Box(Black, FVector(0.0, -2.0, 289.0), FVector(16.0, 2.0, 293.0), false);
	UMaterialInstanceDynamic* Jar = Mat(TEXT("DoorLampGlass"), 0xffd9a0, 0.3f, 0.0f, 9.0f);
	NightGlowMids.Add(Jar);
	NightGlowLevels.Add(9.0f);
	Cyl(Jar, FVector(16.0, 0.0, 272.0), 6.0f, 16.0f, false)->SetCastShadow(false);
	Cyl(Black, FVector(16.0, 0.0, 288.0), 7.0f, 3.0f, false);
	// The number on the brick above the opening: enamel and brass that read from across the street, not lit by the lamp below.
	Sign(TEXT("number:1812"), FVector(1.5, 0.0, 322.0), 0.0f, FVector2D(90.0, 20.0), FIntPoint(400, 90), 1.4f);
}

void AStreetStage::BuildWashAndFold()
{
	UMaterialInterface* Stucco = Mat(TEXT("Stucco"), 0x8a8478, 0.9f, PatConcrete);
	UMaterialInterface* Fluor = Mat(TEXT("Fluorescent"), 0xe8f4ff, 0.3f, 0.0f, 4.0f);
	UMaterialInterface* Washer = Mat(TEXT("Washer"), 0xd9dcdf, 0.3f, 0.0f, 0.0f, 0.2f);
	UMaterialInterface* WasherDoor = Mat(TEXT("WasherDoor"), 0x2a3440, 0.1f, 0.0f, 0.3f, 0.5f);
	UMaterialInterface* Iron = Mat(TEXT("Iron"), 0x1c1e21, 0.45f, 0.0f, 0.0f, 0.7f);

	// The Wash & Fold: a low storefront lit all night, its glass on the building line and the room behind it (the
	// washers along the back wall, tubes on the ceiling), the neon over the windows.
	const double Room = 260.0; // how deep the laundromat's front room is
	Box(Stucco, FVector(FarFront + Room, -700.0, 0.0), FVector(FarFront + 1300.0, 700.0, 450.0));
	Box(Stucco, FVector(FarFront, -700.0, 310.0), FVector(FarFront + Room, 700.0, 450.0));
	Box(Stucco, FVector(FarFront, -700.0, 0.0), FVector(FarFront + Room, -680.0, 310.0));
	Box(Stucco, FVector(FarFront, 680.0, 0.0), FVector(FarFront + Room, 700.0, 310.0));
	Box(Stucco, FVector(FarFront, -680.0, 0.0), FVector(FarFront + 12.0, 680.0, 40.0));
	Box(Mat(TEXT("WashFloor"), 0xc9c4b6, 0.3f, PatTiles), FVector(FarFront, -680.0, -2.0), FVector(FarFront + Room, 680.0, 1.0));
	Box(Mat(TEXT("WashWall"), 0xd8e2e0, 0.6f), FVector(FarFront + Room - 2.0, -680.0, 0.0), FVector(FarFront + Room, 680.0, 310.0), false);
	Box(Mat(TEXT("WashCeiling"), 0xeceae4, 0.9f), FVector(FarFront, -680.0, 300.0), FVector(FarFront + Room, 680.0, 310.0), false);
	for (double Y = -560.0; Y < 600.0; Y += 280.0)
	{
		Box(Fluor, FVector(FarFront + 60.0, Y, 297.0), FVector(FarFront + 200.0, Y + 14.0, 300.0), false)->SetCastShadow(false);
	}
	for (int32 I = 0; I < 7; ++I)
	{
		const double Y = -540.0 + I * 160.0;
		Box(Washer, FVector(FarFront + Room - 64.0, Y, 1.0), FVector(FarFront + Room - 4.0, Y + 120.0, 110.0), false);
		Cyl(WasherDoor, FVector(FarFront + Room - 65.0, Y + 60.0, 52.0), 26.0f, 2.0f, false)->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	}
	Box(Mat(TEXT("FoldTable"), 0xd9d6cc, 0.5f), FVector(FarFront + 80.0, -220.0, 74.0), FVector(FarFront + 150.0, 220.0, 78.0), false);
	if (URectLightComponent* Tubes = NewPart<URectLightComponent>())
	{
		Tubes->SetRelativeLocationAndRotation(FVector(FarFront + 130.0, 0.0, 296.0), FRotator(-90.0f, 0.0f, 0.0f));
		Tubes->SetIntensityUnits(ELightUnits::Lumens);
		Tubes->SetIntensity(900.0f);
		Tubes->SetSourceWidth(1200.0f);
		Tubes->SetSourceHeight(150.0f);
		Tubes->SetAttenuationRadius(800.0f);
		Tubes->SetLightColor(SrgbHex(0xe8f4ff));
		Tubes->SetCastShadows(false);
		Tubes->SetVolumetricScatteringIntensity(0.0f);
	}
	// The glass on the building line (it blocks), a door's frame drawn in it.
	UMaterialInterface* WashGlass = StoreGlassMaterial ? StoreGlassMaterial.Get() : nullptr;
	Box(WashGlass, FVector(FarFront + 12.0, -680.0, 40.0), FVector(FarFront + 14.0, 680.0, 300.0))->SetVisibility(WashGlass != nullptr);
	for (const double Y : {380.0, 480.0})
	{
		Box(Iron, FVector(FarFront + 8.0, Y - 3.0, 0.0), FVector(FarFront + 14.0, Y + 3.0, 300.0), false);
	}
	Box(Iron, FVector(FarFront + 8.0, 380.0, 236.0), FVector(FarFront + 14.0, 480.0, 242.0), false);
	Sign(TEXT("neon"), FVector(FarFront - 2.0, 0.0, 375.0), 180.0f, FVector2D(420.0, 131.0), FIntPoint(1024, 320), 6.0f);
	if (GlowMaterial)
	{
		UMaterialInstanceDynamic* Halo = UMaterialInstanceDynamic::Create(GlowMaterial, this);
		Halo->SetVectorParameterValue(TEXT("Color"), SrgbHex(0xff2e88));
		Halo->SetScalarParameterValue(TEXT("Strength"), 0.4f);
		NightGlowMids.Add(Halo);
		NightGlowLevels.Add(0.4f);
		UStaticMeshComponent* Plane = NewPart<UStaticMeshComponent>();
		Plane->SetStaticMesh(PlaneMesh);
		Plane->SetMaterial(0, Halo);
		// The engine plane faces +Z: pitched up it faces the street (-X), its local X running up the wall.
		Plane->SetRelativeLocationAndRotation(FVector(FarFront - 4.0, 0.0, 375.0), FRotator(90.0f, 0.0f, 0.0f));
		Plane->SetRelativeScale3D(FVector(4.0, 9.0, 1.0));
		Plane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Plane->SetCastShadow(false);
	}
	// The neon's pink on the wet street: a rect light the sign's size, so the puddles reflect its shape.
	if (URectLightComponent* Pink = NewPart<URectLightComponent>())
	{
		Pink->SetRelativeLocationAndRotation(FVector(FarFront - 22.0, 0.0, 372.0), FRotator(-20.0f, 180.0f, 0.0f));
		Pink->SetIntensityUnits(ELightUnits::Lumens);
		Pink->SetIntensity(700.0f);
		Pink->SetSourceWidth(420.0f);
		Pink->SetSourceHeight(110.0f);
		Pink->SetAttenuationRadius(1700.0f);
		Pink->SetLightColor(SrgbHex(0xff3a8c));
		Pink->SetCastShadows(false);
		Pink->SetVolumetricScatteringIntensity(0.8f);
		NightLights.Add(Pink);
		NightLightLevels.Add(700.0f);
	}
	if (URectLightComponent* Spill = NewPart<URectLightComponent>())
	{
		// The laundromat's light through its glass onto the wet sidewalk.
		Spill->SetRelativeLocationAndRotation(FVector(FarFront + 20.0, 0.0, 180.0), FRotator(-15.0f, 180.0f, 0.0f));
		Spill->SetIntensityUnits(ELightUnits::Lumens);
		Spill->SetIntensity(1600.0f);
		Spill->SetSourceWidth(1200.0f);
		Spill->SetSourceHeight(240.0f);
		Spill->SetAttenuationRadius(1400.0f);
		Spill->SetLightColor(SrgbHex(0xdff0ff));
		Spill->SetCastShadows(false);
	}
	// No rain falls in its room seen through the glass; the rain by its windows catches the neon's pink.
	WashMin = FVector(FarFront + 11.0, -700.0, -60.0);
	WashMax = FVector(FarFront + 1300.0, 700.0, 450.0);
	NeonGlowAt = FVector(FarFront - 60.0, 0.0, 260.0);
}

void AStreetStage::BuildAcross()
{
	UMaterialInterface* Brick = Mat(TEXT("BrickFar"), 0x5a3a30, 0.85f, PatBrick);
	UMaterialInterface* Shutter = Mat(TEXT("Shutter"), 0x5d6166, 0.5f, PatShutter, 0.0f, 0.6f);
	UMaterialInterface* Iron = Mat(TEXT("Iron"), 0x1c1e21, 0.45f, 0.0f, 0.0f, 0.7f);
	const FVector2D Win(120.0, 170.0);

	// The laundromat's building: the brick walk-up the apartment's window looks out on (tenement.py), the Wash & Fold in its
	// long shop window, standing where that window sees it: its fire escape and the tenants' door across from 1812, the
	// laundromat to their right and the neon over it on the third floor. Without it, the laundromat on its own, low.
	TenementFront = Facade(TEXT("SM_Tenement"), TenementY, true);
	const bool bTenement = TenementFront != nullptr;
	if (bTenement)
	{
		const double Y0 = TenementY - TenementHalf;
		const double Y1 = TenementY + TenementHalf;
		// Its shop window (tenement.py SHOP_Y), its Blender y running down the street.
		const double Shop0 = TenementY - 860.0;
		const double Shop1 = TenementY + 860.0;
		// Its block behind it (the laundromat's room reaches 3.9 m back), brick returns closing its ends over its lower
		// neighbors, and its face blocking.
		Box(Brick, FVector(FarFront + 390.0, Y0, 0.0), FVector(FarFront + 1500.0, Y1, 2480.0));
		Box(Brick, FVector(FarFront + 45.0, Y0, 0.0), FVector(FarFront + 390.0, Y0 + 30.0, 2485.0));
		Box(Brick, FVector(FarFront + 45.0, Y1 - 30.0, 0.0), FVector(FarFront + 390.0, Y1, 2485.0));
		FrontBox(TenementY, true, -0.06, 0.45, -18.0, 18.0, 0.0, 24.95);
		// The rain beaded on the laundromat's window: a pane through the middle of its mullions (12-22 cm into the opening).
		if (StoreGlassMaterial)
		{
			Box(StoreGlassMaterial, FVector(FarFront + 16.5, Shop0, 45.0), FVector(FarFront + 17.5, Shop1, 335.0), false);
		}
		// The neon where the apartment's window sees it, on the bricked-up third floor over the laundromat (SIGN_BLANK) and
		// as big as it is there, its halo on the wet brick.
		const FVector NeonAt(FarFront - 10.0, TenementY - 280.0, 1140.0);
		Sign(TEXT("neon"), NeonAt, 180.0f, FVector2D(640.0, 200.0), FIntPoint(1024, 320), 6.0f);
		if (GlowMaterial)
		{
			UMaterialInstanceDynamic* Halo = UMaterialInstanceDynamic::Create(GlowMaterial, this);
			Halo->SetVectorParameterValue(TEXT("Color"), SrgbHex(0xff2e88));
			Halo->SetScalarParameterValue(TEXT("Strength"), 0.4f);
			NightGlowMids.Add(Halo);
			NightGlowLevels.Add(0.4f);
			UStaticMeshComponent* Plane = NewPart<UStaticMeshComponent>();
			Plane->SetStaticMesh(PlaneMesh);
			Plane->SetMaterial(0, Halo);
			// The engine plane faces +Z: pitched up it faces the street (-X), its local X running up the wall.
			Plane->SetRelativeLocationAndRotation(NeonAt - FVector(2.0, 0.0, 0.0), FRotator(90.0f, 0.0f, 0.0f));
			Plane->SetRelativeScale3D(FVector(6.1, 13.7, 1.0));
			Plane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Plane->SetCastShadow(false);
		}
		// Its pink on the wet street: a rect light the sign's size, aimed down across the road from up there (the road gets
		// about what it got from the low sign; the puddles still reflect its shape).
		if (URectLightComponent* Pink = NewPart<URectLightComponent>())
		{
			Pink->SetRelativeLocationAndRotation(NeonAt + FVector(-20.0, 0.0, -10.0), FRotator(-45.0f, 180.0f, 0.0f));
			Pink->SetIntensityUnits(ELightUnits::Lumens);
			Pink->SetIntensity(600.0f);
			Pink->SetSourceWidth(640.0f);
			Pink->SetSourceHeight(170.0f);
			Pink->SetAttenuationRadius(2600.0f);
			Pink->SetLightColor(SrgbHex(0xff3a8c));
			Pink->SetCastShadows(false);
			Pink->SetVolumetricScatteringIntensity(0.8f);
			NightLights.Add(Pink);
			NightLightLevels.Add(600.0f);
		}
		if (URectLightComponent* Spill = NewPart<URectLightComponent>())
		{
			// The laundromat's light through its window onto the wet sidewalk.
			Spill->SetRelativeLocationAndRotation(FVector(FarFront + 25.0, TenementY, 180.0), FRotator(-15.0f, 180.0f, 0.0f));
			Spill->SetIntensityUnits(ELightUnits::Lumens);
			Spill->SetIntensity(2300.0f);
			Spill->SetSourceWidth(static_cast<float>(Shop1 - Shop0));
			Spill->SetSourceHeight(240.0f);
			Spill->SetAttenuationRadius(1400.0f);
			Spill->SetLightColor(SrgbHex(0xdff0ff));
			Spill->SetCastShadows(false);
		}
		// No rain falls in the laundromat seen through its window; the rain catches the neon's pink in its beam.
		WashMin = FVector(FarFront + 16.0, Shop0 - 10.0, -60.0);
		WashMax = FVector(FarFront + 400.0, Shop1 + 10.0, 360.0);
		NeonGlowAt = FVector(FarFront - 450.0, NeonAt.Y, 700.0);
	}
	else
	{
		BuildWashAndFold();
	}

	// Taller buildings either side of it, windows lit at random, their shops gated for the night; up the street, across
	// from the walk-up and the store, three brownstones (facades.py) where they're imported.
	int64 Seed = 777;
	auto Block = [&](double Y0, double Y1, double Height) {
		Box(Brick, FVector(FarFront, Y0, 0.0), FVector(FarFront + 1500.0, Y1, Height));
		WindowRows(FVector(FarFront - 0.5, Y0, 0.0), FVector(0.0, 1.0, 0.0), Y1 - Y0, 180.0f, 380.0, Height, Win, 250.0, 330.0, 0.24, (Seed++) * 104729);
		for (double Y = Y0 + 200.0; Y + 700.0 < Y1; Y += 1100.0)
		{
			Box(Shutter, FVector(FarFront - 4.0, Y, 20.0), FVector(FarFront, Y + 600.0, 290.0), false);
			Box(Iron, FVector(FarFront - 16.0, Y - 10.0, 290.0), FVector(FarFront, Y + 610.0, 312.0), false);
		}
	};
	const double North = TenementY + TenementHalf;
	const double RowEnd = North + 3.0 * BrownstoneW;
	Block(EndY0, bTenement ? TenementY - TenementHalf : -700.0, 1800.0);
	if (!bTenement)
	{
		Block(700.0, ImportedMesh(TEXT("SM_Facade_Brownstone")) ? North : 3000.0, 1400.0);
	}
	if (BrownstoneRow(North, RowEnd, true, Brick))
	{
		Block(RowEnd, EndY1, 2200.0);
	}
	else
	{
		Block(bTenement ? North : 3000.0, EndY1, 2200.0);
	}

	// Parked cars, each with the traffic on its side (the curb on its right): the far curb's facing down the street,
	// two on this side facing up it. Each its own paint, and the one across from the building's door is oxblood, so
	// the first car the player sees isn't another grey one.
	struct FCar
	{
		double X, Y;
		float Yaw;
		uint32 Paint;
	};
	const FCar Cars[] = {{RoadX1 - 120.0, -3570.0, -90.0f, 0x1f3354}, {RoadX1 - 120.0, -1270.0, -90.0f, 0x2a2b2e}, {RoadX1 - 120.0, 1430.0, -90.0f, 0x6e1b24},
		{RoadX1 - 120.0, 3030.0, -90.0f, 0x7b7f86}, {RoadX0 + 120.0, -2650.0, 90.0f, 0x1f4a3a}, {RoadX0 + 120.0, -4400.0, 90.0f, 0x9a9a96}};
	UMaterialInterface* CarGlass = Mat(TEXT("CarGlass"), 0x0b0e12, 0.05f, 0.0f, 0.0f, 0.4f);
	for (const FCar& Car : Cars)
	{
		if (UStaticMeshComponent* Mesh = Prop(TEXT("SM_Sedan"), FVector(Car.X, Car.Y, -15.0), Car.Yaw))
		{
			ParkedCars.Add(Mesh);
			ParkedPaints.Add(SrgbHex(Car.Paint));
		}
		else
		{
			UMaterialInterface* Paint = Mat(*FString::Printf(TEXT("Car%06x"), Car.Paint), Car.Paint, 0.25f, 0.0f, 0.0f, 0.6f);
			Box(Paint, FVector(Car.X - 90.0, Car.Y - 230.0, -5.0), FVector(Car.X + 90.0, Car.Y + 230.0, 90.0));
			Box(CarGlass, FVector(Car.X - 75.0, Car.Y - 120.0, 90.0), FVector(Car.X + 75.0, Car.Y + 100.0, 140.0));
		}
	}
	PaintCars();
}

void AStreetStage::PaintCars()
{
	for (int32 I = 0; I < ParkedCars.Num() && I < ParkedPaints.Num(); ++I)
	{
		UStaticMeshComponent* Mesh = ParkedCars[I];
		if (!Mesh)
		{
			continue;
		}
		// The Blender sedan is baked light grey; its body is the first material (the paint), the rest stay as baked. Each
		// tint is made fresh and never saved with the level: a saved one would tie the level to the sedan's own materials,
		// and the importer won't refresh the textures of materials another package uses (a re-baked sedan would keep its
		// old ones).
		Mesh->EmptyOverrideMaterials();
		const int32 PaintSlot = ImportedSlot(Mesh, 0);
		const bool bTinted = TintImported(Mesh, PaintSlot, ParkedPaints[I]);
		UMaterialInterface* Paint = Mesh->GetMaterial(PaintSlot);
		if (UMaterialInstanceDynamic* Tint = Cast<UMaterialInstanceDynamic>(Paint))
		{
			Tint->SetFlags(RF_Transient);
		}
		if (!bTinted && I == 0)
		{
			// Set blind; if the cars still park grey, this says which material didn't list a base color factor.
			const UMaterialInstance* Instance = Cast<UMaterialInstance>(Paint);
			UE_LOG(LogTemp, Warning, TEXT("ShortStack: the sedan's paint (slot %d, %s) lists no base color factor"), PaintSlot,
				*GetNameSafe(Instance ? Instance->Parent.Get() : Paint));
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
	// The store's own lights read against its exposure (a few stops over the street's), so they're set brighter.
	UMaterialInterface* Panel = Mat(TEXT("StoreLightPanel"), 0xf4fbff, 0.3f, 0.0f, 80.0f);
	UMaterialInterface* Red = Mat(TEXT("PennyRed"), 0xd7263d, 0.5f);
	UMaterialInterface* Metal = Mat(TEXT("Steel"), 0xa0a4aa, 0.35f, 0.0f, 0.0f, 0.9f);
	UMaterialInterface* Shelf = Mat(TEXT("Shelf"), 0xe2e2e0, 0.5f, 0.0f, 0.0f, 0.4f);
	UMaterialInterface* CoolerGlow = Mat(TEXT("CoolerGlow"), 0xe6f3ff, 0.2f, 0.0f, 60.0f);

	// The three floors over the store (facades.py) where they're imported: painted brick from the roof's edge up, round the
	// corner onto Market, a cast-stone band along both hiding the roof slab's edge. On the south side, where no band
	// runs, the block's plain wall comes down to Z 476 in the store's own south wall's plane: that wall stops there, and
	// the slab 2 cm short of it, so neither shares its face.
	const bool bUpper = Facade(TEXT("SM_Facade_StoreUpper"), (StoreY0 + StoreY1) * 0.5, false) != nullptr;

	// Shell: back, sides, roof with a parapet, the front's low wall, mullions and the header over the windows.
	Box(Outside, FVector(X0 - 20.0, StoreY0, 0.0), FVector(X0, StoreY1, 480.0));
	Box(Outside, FVector(X0, StoreY0 - 20.0, 0.0), FVector(0.0, StoreY0, bUpper ? 476.0 : 480.0));
	Box(Outside, FVector(X0, StoreY1, 0.0), FVector(0.0, StoreY1 + 20.0, 480.0));
	Box(Outside, FVector(X0 - 20.0, StoreY0 - (bUpper ? 18.0 : 20.0), 480.0), FVector(10.0, StoreY1 + 20.0, 520.0));
	// The ceiling stops the camera (a third-person view pitched down inside would otherwise rise through it).
	Box(Ceiling, FVector(X0, StoreY0, StoreCeiling), FVector(0.0, StoreY1, StoreCeiling + 4.0), true);
	// The doorway's mullions stand on its edges (DoorY0, DoorY1): the closed doors tuck in behind them, so no slot of
	// open air shows either side, and open they stop behind them.
	Box(Outside, FVector(-20.0, StoreY0, 0.0), FVector(0.0, DoorY0 + 6.0, 45.0));
	Box(Outside, FVector(-20.0, DoorY1 - 6.0, 0.0), FVector(0.0, StoreY1, 45.0));
	Box(Outside, FVector(-20.0, StoreY0, 330.0), FVector(0.0, StoreY1, 480.0));
	for (double Y : {StoreY0 + 470.0, DoorY0, DoorY1})
	{
		Box(Metal, FVector(-18.0, Y - 6.0, 45.0), FVector(-6.0, Y + 6.0, 330.0));
	}
	// The fixed panes. M_StreetGlass is a clean pane with rain beaded on it; the apartment's window glass, sized to
	// each pane so its drops stay drop-sized, if that's all there is.
	auto PaneGlass = [this](double Wd, double Ht) -> UMaterialInterface* {
		if (StoreGlassMaterial)
		{
			return StoreGlassMaterial;
		}
		if (!GlassMaterial)
		{
			return nullptr;
		}
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(GlassMaterial, this);
		M->SetScalarParameterValue(TEXT("WidthCm"), static_cast<float>(Wd));
		M->SetScalarParameterValue(TEXT("HeightCm"), static_cast<float>(Ht));
		M->SetScalarParameterValue(TEXT("Aspect"), static_cast<float>(Wd / FMath::Max(1.0, Ht)));
		M->SetScalarParameterValue(TEXT("Refraction"), 0.3f);
		return M;
	};
	// The fixed panes draw after what's behind them (the posters and OPEN taped inside, an open door with its sticker),
	// whichever is nearer by bounds, so the rain's beads always lie over them.
	if (UMaterialInterface* G = PaneGlass(DoorY0 - StoreY0, 285.0))
	{
		Box(G, FVector(-12.0, StoreY0, 45.0), FVector(-10.0, DoorY0, 330.0), false)->SetTranslucentSortPriority(2);
	}
	if (UMaterialInterface* G = PaneGlass(StoreY1 - DoorY1, 285.0))
	{
		Box(G, FVector(-12.0, DoorY1, 45.0), FVector(-10.0, StoreY1, 330.0), false)->SetTranslucentSortPriority(2);
	}
	// The windows themselves block (glass you can't walk through, and the camera stays on its side of it).
	Box(nullptr, FVector(-14.0, StoreY0, 45.0), FVector(-8.0, DoorY0, 330.0))->SetVisibility(false);
	Box(nullptr, FVector(-14.0, DoorY1, 45.0), FVector(-8.0, StoreY1, 330.0))->SetVisibility(false);
	Box(Red, FVector(-6.0, StoreY0, 318.0), FVector(2.0, StoreY1, 330.0), false);
	// The sliding doors: each a steel frame round its glass on the track inside the fixed panes (UpdateDoors slides
	// them; the hours sticker rides on the left one), the transom over the doorway and the track the length of their
	// travel, so an open door still hangs from something behind the glass.
	const double Half = (DoorY1 - DoorY0) * 0.5;
	const double Leaf = Half * 0.5;
	UMaterialInterface* DoorGlass = PaneGlass(Half, 246.0);
	if (!DoorGlass)
	{
		DoorGlass = Mat(TEXT("DoorGlassFallback"), 0x9fb7c8, 0.05f);
	}
	for (int32 Side = 0; Side < 2; ++Side)
	{
		USceneComponent* Door = NewPart<USceneComponent>();
		Door->SetRelativeLocation(FVector(DoorTrackX, Side == 0 ? DoorY0 + Leaf : DoorY1 - Leaf, 0.0));
		Box(DoorGlass, FVector(-2.0, -Leaf + 4.0, 8.0), FVector(2.0, Leaf - 4.0, 254.0), true, Door);
		Box(Metal, FVector(-3.0, -Leaf, 0.0), FVector(3.0, -Leaf + 4.0, 260.0), false, Door);
		Box(Metal, FVector(-3.0, Leaf - 4.0, 0.0), FVector(3.0, Leaf, 260.0), false, Door);
		Box(Metal, FVector(-3.0, -Leaf + 4.0, 0.0), FVector(3.0, Leaf - 4.0, 8.0), false, Door);
		Box(Metal, FVector(-3.0, -Leaf + 4.0, 254.0), FVector(3.0, Leaf - 4.0, 260.0), false, Door);
		Box(Metal, FVector(-5.0, -Leaf + 8.0, 98.0), FVector(3.5, Leaf - 8.0, 104.0), false, Door);
		if (Side == 0)
		{
			DoorLeft = Door;
		}
		else
		{
			DoorRight = Door;
		}
	}
	Box(Metal, FVector(-28.0, DoorY0 - 6.0, 260.0), FVector(0.0, DoorY1 + 6.0, 330.0), false);
	Box(Metal, FVector(-29.0, DoorY0 - Half, 260.5), FVector(-19.0, DoorY1 + Half - 2.0, 272.0), false)->SetCastShadow(false);

	// Inside: the floor (out to the doorway, so the threshold has one), the walls' paint and the red stripe, the light
	// panels and the light itself.
	Box(Floor, FVector(X0, StoreY0, -2.0), FVector(0.0, StoreY1, 1.0));
	Box(Wall, FVector(X0, StoreY0, 0.0), FVector(X0 + 2.0, StoreY1, StoreCeiling), false);
	Box(Wall, FVector(X0 + 2.0, StoreY0, 0.0), FVector(-20.0, StoreY0 + 2.0, StoreCeiling), false);
	Box(Wall, FVector(X0 + 2.0, StoreY1 - 2.0, 0.0), FVector(-20.0, StoreY1, StoreCeiling), false);
	Box(Red, FVector(X0 + 2.0, StoreY0 + 2.0, 104.0), FVector(-20.0, StoreY0 + 3.0, 116.0), false)->SetCastShadow(false);
	Box(Red, FVector(X0 + 2.0, StoreY1 - 3.0, 104.0), FVector(-20.0, StoreY1 - 2.0, 116.0), false)->SetCastShadow(false);
	Box(Red, FVector(X0 + 2.0, StoreY0 + 2.0, 104.0), FVector(X0 + 3.0, StoreY1 - 2.0, 116.0), false)->SetCastShadow(false);
	for (double X = X0 + 200.0; X < -100.0; X += 300.0)
	{
		for (double Y = StoreY0 + 200.0; Y < StoreY1 - 100.0; Y += 420.0)
		{
			Box(Panel, FVector(X, Y, StoreCeiling - 3.0), FVector(X + 120.0, Y + 60.0, StoreCeiling), false)->SetCastShadow(false);
		}
	}
	for (int32 I = 0; I < 3; ++I)
	{
		if (URectLightComponent* L = NewPart<URectLightComponent>())
		{
			// Three bands across the room, front to back. Turned to face down, a rect light's height runs along X (the
			// store's depth) and its width along Y: 380 deep and 900 across, each stays inside the walls (laid the other
			// way, the front one reached 2 m out over the sidewalk and lit it from the open air).
			L->SetRelativeLocationAndRotation(FVector(-250.0 - I * 420.0, (StoreY0 + StoreY1) * 0.5, StoreCeiling - 6.0), FRotator(-90.0f, 0.0f, 0.0f));
			L->SetIntensityUnits(ELightUnits::Lumens);
			L->SetIntensity(2200.0f);
			L->SetSourceWidth(900.0f);
			L->SetSourceHeight(380.0f);
			L->SetAttenuationRadius(1200.0f);
			L->SetUseTemperature(true);
			L->SetTemperature(4600.0f);
			// Shadowed (PickStreetlights keeps them so while the camera is near): unshadowed, their light would come
			// through the store's walls onto Market's sidewalk.
			L->SetCastShadows(true);
			L->SetVolumetricScatteringIntensity(0.0f);
			StoreLights.Add(L);
		}
	}
	// The store's light spilling out onto the sidewalk (sized for the street's exposure, not the store's).
	if (URectLightComponent* Spill = NewPart<URectLightComponent>())
	{
		Spill->SetRelativeLocationAndRotation(FVector(-30.0, (StoreY0 + StoreY1) * 0.5, 200.0), FRotator(-20.0f, 0.0f, 0.0f));
		Spill->SetIntensityUnits(ELightUnits::Lumens);
		Spill->SetIntensity(2200.0f);
		Spill->SetSourceWidth(1100.0f);
		Spill->SetSourceHeight(260.0f);
		Spill->SetAttenuationRadius(1300.0f);
		Spill->SetUseTemperature(true);
		Spill->SetTemperature(4800.0f);
		Spill->SetCastShadows(false);
	}

	// The cooler wall at the back: glass doors lit from inside, every shelf stocked with a row of real packs (stock.py)
	// under the header's sections. Without the rows (or the cooler, whose stand-in box would hide them), the drinks in
	// columns behind the glass, each column one product.
	UStaticMeshComponent* Cooler = Prop(TEXT("SM_Cooler"), FVector(X0 + 45.0, 4500.0, 0.0), 0.0f);
	if (!Cooler)
	{
		Box(Metal, FVector(X0, 4100.0, 0.0), FVector(X0 + 90.0, StoreY1 - 50.0, 230.0));
		Box(CoolerGlow, FVector(X0 + 90.0, 4130.0, 20.0), FVector(X0 + 92.0, StoreY1 - 80.0, 215.0), false);
	}
	if (!Cooler || !StockCooler(Cooler->GetRelativeTransform()))
	{
		static const char* const DrinkIds[4] = {"cascade", "fizz-cola", "volt-rush", "sunny-peach"};
		UInstancedStaticMeshComponent* Drinks[4];
		for (int32 K = 0; K < 4; ++K)
		{
			const ss::store::Item* Item = ss::store::Find(DrinkIds[K]);
			Drinks[K] = NewPart<UInstancedStaticMeshComponent>();
			Drinks[K]->SetStaticMesh(CylinderMesh);
			Drinks[K]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Drinks[K]->SetCastShadow(false);
			Drinks[K]->SetMaterial(0, Mat(*FString::Printf(TEXT("Drink%d"), K), Item ? Item->Color : 0xc8262f, 0.3f, 0.0f, 0.0f, 0.5f));
		}
		for (double Z = 30.0; Z < 210.0; Z += 45.0)
		{
			for (double Y = 4150.0; Y < StoreY1 - 90.0; Y += 14.0)
			{
				const int32 K = static_cast<int32>((Y - 4150.0) / 98.0) % 4;
				Drinks[K]->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X0 + 70.0, Y, Z + 9.0), FVector(0.13, 0.13, 0.2)));
			}
		}
	}

	// Two aisles of shelves (the gondolas' frame as SM_Shelf's, so the stock sits on it or on the stand-in's boxes), every
	// bay and level on both faces stocked with rows of real packs: snacks facing the aisle between them, groceries and
	// household outward, the bulk packs on the base decks. Without the rows, boxes in the store's own colors.
	const TArray<FTransform> Gondolas = {FTransform(BlenderFacing(90.0f), FVector(-850.0, 4330.0, 0.0)), FTransform(BlenderFacing(90.0f), FVector(-850.0, 4560.0, 0.0))};
	for (const FTransform& Gondola : Gondolas)
	{
		const double Ay = Gondola.GetLocation().Y;
		if (!Prop(TEXT("SM_Shelf"), Gondola.GetLocation(), 90.0f))
		{
			Box(Shelf, FVector(-1150.0, Ay - 30.0, 0.0), FVector(-550.0, Ay + 30.0, 12.0));
			Box(Shelf, FVector(-1150.0, Ay - 3.0, 12.0), FVector(-550.0, Ay + 3.0, 160.0));
			for (double Z : {40.0, 85.0, 130.0})
			{
				Box(Shelf, FVector(-1150.0, Ay - 30.0, Z - 3.0), FVector(-550.0, Ay + 30.0, Z), false);
			}
		}
	}
	if (!StockShelves(Gondolas))
	{
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
		for (const FTransform& Gondola : Gondolas)
		{
			const double Ay = Gondola.GetLocation().Y;
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
	}

	// The signs on the front: the box sign over the windows; OPEN hung inside the window and two posters taped to the
	// inside of the glass (the rain's beads over them), each at its shelf price, so the window never promises what the
	// register won't ring up; the hours sticker on the door.
	Sign(TEXT("storesign"), FVector(4.0, 4300.0, 405.0), 0.0f, FVector2D(720.0, 135.0), FIntPoint(1600, 300), 3.5f);
	Sign(TEXT("open"), FVector(-17.0, 4000.0, 230.0), 0.0f, FVector2D(110.0, 48.0), FIntPoint(640, 280), 5.0f);
	Sign(TEXT("promo:volt-rush"), FVector(-12.6, 3820.0, 160.0), 0.0f, FVector2D(60.0, 90.0), FIntPoint(600, 900), 1.2f);
	Sign(TEXT("promo:roller-dog"), FVector(-12.6, 4470.0, 160.0), 0.0f, FVector2D(60.0, 90.0), FIntPoint(600, 900), 1.2f);
	// On the door's outer face, inside the mullions' plane: it slides with the door, drawn over the door's own glass
	// (and under the fixed pane it slides behind).
	DoorDecal = Sign(TEXT("door"), FVector(3.3, 0.0, 150.0), 0.0f, FVector2D(32.0, 42.0), FIntPoint(400, 520), 1.2f, false, DoorLeft);
	DoorDecal->SetTranslucentSortPriority(1);
	// The box sign's light on the sidewalk and in the puddles; the OPEN sign's red in the window.
	if (URectLightComponent* SignLight = NewPart<URectLightComponent>())
	{
		SignLight->SetRelativeLocationAndRotation(FVector(20.0, 4300.0, 405.0), FRotator(-30.0f, 0.0f, 0.0f));
		SignLight->SetIntensityUnits(ELightUnits::Lumens);
		SignLight->SetIntensity(600.0f);
		SignLight->SetSourceWidth(700.0f);
		SignLight->SetSourceHeight(120.0f);
		SignLight->SetAttenuationRadius(1500.0f);
		SignLight->SetUseTemperature(true);
		SignLight->SetTemperature(5200.0f);
		SignLight->SetCastShadows(false);
		NightLights.Add(SignLight);
		NightLightLevels.Add(600.0f);
	}
	if (UPointLightComponent* OpenGlow = NewPart<UPointLightComponent>())
	{
		OpenGlow->SetRelativeLocation(FVector(10.0, 4000.0, 230.0));
		OpenGlow->SetIntensityUnits(ELightUnits::Candelas);
		OpenGlow->SetIntensity(6.0f);
		OpenGlow->SetLightColor(SrgbHex(0xff3b3b));
		OpenGlow->SetAttenuationRadius(320.0f);
		OpenGlow->SetCastShadows(false);
		NightLights.Add(OpenGlow);
		NightLightLevels.Add(6.0f);
	}
}

void AStreetStage::BuildStoreDressing()
{
	FStreetDice Dice;
	Dice.S = 1212;
	const double X0 = -StoreDepth;
	UMaterialInterface* Metal = Mat(TEXT("Steel"), 0xa0a4aa, 0.35f, 0.0f, 0.0f, 0.9f);
	UMaterialInterface* Front = Mat(TEXT("CounterFront"), 0x8e1c2a, 0.55f);
	UMaterialInterface* Groove = Mat(TEXT("CounterGroove"), 0x4a0f17, 0.6f);
	UMaterialInterface* Kick = Mat(TEXT("CounterKick"), 0x18191c, 0.7f);
	UMaterialInterface* Cabinet = Mat(TEXT("RackCabinet"), 0x1b1d22, 0.5f);
	// The counter's acrylic (the ticket case, the sneeze guard, the rack's front) is the store's glass, dry: indoors.
	UMaterialInterface* Glass = nullptr;
	if (StoreGlassMaterial)
	{
		UMaterialInstanceDynamic* Dry = UMaterialInstanceDynamic::Create(StoreGlassMaterial, this);
		Dry->SetScalarParameterValue(TEXT("Wet"), 0.0f);
		Glass = Dry;
	}

	// The counter: red-fronted with steel on top, a kick plate, an impulse rack of candy on the customer's side.
	Box(Front, FVector(-650.0, 3980.0, 12.0), FVector(-150.0, 4060.0, 95.0));
	// The plinth under it, set back 2 cm on the customer's side and running through to Benny's (no slot under the counter).
	Box(Kick, FVector(-648.0, 3982.0, 0.0), FVector(-152.0, 4058.0, 12.0));
	// A half door at each end of it, back to the wall: behind the counter is Benny's (nobody walks round into him or
	// the rack).
	for (const double Gx : {-652.0, -152.0})
	{
		Box(Front, FVector(Gx, StoreY0 + 2.0, 0.0), FVector(Gx + 4.0, 3980.0, 92.0));
		Box(Metal, FVector(Gx - 1.0, StoreY0 + 2.0, 92.0), FVector(Gx + 5.0, 3980.0, 96.0), false);
	}
	Box(Metal, FVector(-662.0, 3972.0, 95.0), FVector(-138.0, 4074.0, 100.0));
	for (double X = -625.0; X < -150.0; X += 50.0)
	{
		Box(Groove, FVector(X, 4060.0, 14.0), FVector(X + 1.5, 4061.0, 93.0), false)->SetCastShadow(false);
	}
	Box(Metal, FVector(-575.0, 4062.0, 38.0), FVector(-455.0, 4064.0, 92.0), false);
	// The candy rack on the counter's front: stock.py's candy rows, three high, faced to the customer (as the gondolas'
	// rows: their products stand 3.5 to 29.5 cm before the origin; here a little shallower, the rack being a rack).
	bool bCandyRows = false;
	for (const double Z : {40.0, 58.0, 76.0})
	{
		if (UStaticMeshComponent* Row = Prop(TEXT("SM_Stock_ShelfRow_Candy"), FVector(-515.0, 4061.0, Z), 90.0f))
		{
			Row->SetRelativeScale3D(FVector(1.0, 0.6, 0.96));
			Row->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			bCandyRows = true;
		}
	}
	if (!bCandyRows)
	{
		UInstancedStaticMeshComponent* Candy[3];
		static const uint32 CandyColors[3] = {0xf2c14e, 0x5b2a17, 0xd8262f};
		for (int32 K = 0; K < 3; ++K)
		{
			Candy[K] = NewPart<UInstancedStaticMeshComponent>();
			Candy[K]->SetStaticMesh(CubeMesh);
			Candy[K]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Candy[K]->SetCastShadow(false);
			Candy[K]->SetMaterial(0, Mat(*FString::Printf(TEXT("Candy%d"), K), CandyColors[K], 0.45f));
		}
		for (double Z : {44.0, 62.0, 80.0})
		{
			for (double X = -570.0; X < -462.0; X += 9.0)
			{
				Candy[static_cast<int32>(Dice.Next() * 3.0) % 3]->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X + 4.0, 4068.0, Z + 6.0), FVector(0.08, 0.06, 0.13)));
			}
		}
	}
	if (!Prop(TEXT("SM_Register"), FVector(-300.0, 4020.0, 100.0), -90.0f)) // its screen to Benny, the card reader to us
	{
		Box(Mat(TEXT("Register"), 0x1b1d22, 0.35f), FVector(-340.0, 3995.0, 100.0), FVector(-260.0, 4045.0, 128.0), false);
		Box(Mat(TEXT("RegisterScreen"), 0x6ad1ff, 0.3f, 0.0f, 25.0f), FVector(-330.0, 3996.0, 128.0), FVector(-270.0, 3998.0, 150.0), false);
	}
	// The impulse buys on the counter between the grill and the register, faced to the customer (stock.py): the Choco
	// Stack caddy, the lighters, the gum rack and the mints.
	if (UStaticMeshComponent* Impulse = Prop(TEXT("SM_Stock_Counter"), FVector(-405.0, 4030.0, 100.0), 90.0f))
	{
		Impulse->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	// The scratch-off case on the counter: rolls of tickets in their colors behind clear acrylic.
	Box(Cabinet, FVector(-250.0, 3990.0, 100.0), FVector(-166.0, 4050.0, 103.0), false);
	static const uint32 TicketColors[6] = {0xe8c22a, 0x2e9e5b, 0xd7263d, 0x3a6fd8, 0x9a3fd0, 0xf28c28};
	for (int32 K = 0; K < 6; ++K)
	{
		const double X = -246.0 + (K % 3) * 26.0;
		const double Z = 104.0 + (K / 3) * 15.0;
		Box(Mat(*FString::Printf(TEXT("Ticket%d"), K), TicketColors[K], 0.4f, 0.0f, 0.15f), FVector(X + 1.0, 4045.5, Z), FVector(X + 23.0, 4046.5, Z + 12.0), false)->SetCastShadow(false);
	}
	if (Glass)
	{
		Box(Glass, FVector(-252.0, 4049.0, 100.0), FVector(-164.0, 4051.0, 136.0), false);
		Box(Glass, FVector(-252.0, 3990.0, 135.0), FVector(-164.0, 4051.0, 137.0), false);
	}

	// The roller grill: a steel tray, eight rollers, the dogs turning on them under a sneeze guard and a warm lamp.
	Box(Metal, FVector(-600.0, 3995.0, 100.0), FVector(-470.0, 4052.0, 106.0), false);
	Box(Metal, FVector(-604.0, 3995.0, 100.0), FVector(-600.0, 4052.0, 124.0), false);
	Box(Metal, FVector(-470.0, 3995.0, 100.0), FVector(-466.0, 4052.0, 124.0), false);
	UInstancedStaticMeshComponent* Rollers = NewPart<UInstancedStaticMeshComponent>();
	Rollers->SetStaticMesh(CylinderMesh);
	Rollers->SetMaterial(0, Mat(TEXT("Roller"), 0xc8ccd2, 0.2f, 0.0f, 0.0f, 1.0f));
	Rollers->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Rollers->SetCastShadow(false);
	for (int32 K = 0; K < 8; ++K)
	{
		// The engine cylinder stands along Z: pitched over it lies along X.
		Rollers->AddInstance(FTransform(FRotator(90.0f, 0.0f, 0.0f), FVector(-535.0, 4002.0 + K * 6.5, 110.0), FVector(0.026, 0.026, 1.28)));
	}
	UInstancedStaticMeshComponent* Dogs = NewPart<UInstancedStaticMeshComponent>();
	Dogs->SetStaticMesh(CylinderMesh);
	Dogs->SetMaterial(0, Mat(TEXT("RollerDog"), 0xa8432a, 0.35f, 0.0f, 0.15f));
	Dogs->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	UInstancedStaticMeshComponent* Ends = NewPart<UInstancedStaticMeshComponent>();
	Ends->SetStaticMesh(SphereMesh);
	Ends->SetMaterial(0, Mat(TEXT("RollerDog"), 0xa8432a, 0.35f, 0.0f, 0.15f));
	Ends->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Ends->SetCastShadow(false);
	for (int32 K = 0; K < 7; ++K)
	{
		const double Y = 4005.25 + K * 6.5;
		const double X = -575.0 + Dice.Next() * 70.0;
		Dogs->AddInstance(FTransform(FRotator(90.0f, 0.0f, 0.0f), FVector(X, Y, 113.0), FVector(0.03, 0.03, 0.14)));
		Ends->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X - 7.0, Y, 113.0), FVector(0.03)));
		Ends->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X + 7.0, Y, 113.0), FVector(0.03)));
	}
	if (Glass)
	{
		Box(Glass, FVector(-602.0, 4050.0, 106.0), FVector(-468.0, 4052.0, 136.0), false);
		Box(Glass, FVector(-602.0, 3995.0, 136.0), FVector(-468.0, 4052.0, 138.0), false);
	}
	if (UPointLightComponent* Warm = NewPart<UPointLightComponent>())
	{
		Warm->SetRelativeLocation(FVector(-535.0, 4022.0, 134.0));
		Warm->SetIntensityUnits(ELightUnits::Candelas);
		Warm->SetIntensity(8.0f);
		Warm->SetUseTemperature(true);
		Warm->SetTemperature(2200.0f);
		Warm->SetAttenuationRadius(140.0f);
		Warm->SetCastShadows(false);
	}
	Sign(TEXT("promo:roller-dog"), FVector(-535.0, 4053.0, 150.0), 90.0f, FVector2D(16.0, 24.0), FIntPoint(600, 900), 0.0f);

	// Behind Benny, against the wall between the half doors: the back bar (stock.py), its cabinet with cartons on top, the
	// phone and gift cards on their slatwall, the tobacco merchandiser with a pack in every pusher and the scratch-off
	// dispenser, the acrylic 3.5 cm in front of the cigarettes. Without it, a rack of pack faces behind the acrylic, a lit
	// price strip over them. The menu board above either, backlit.
	const FVector BackBarAt(-410.0, StoreY0 + 2.0, 0.0);
	if (ImportedMesh(TEXT("SM_Stock_BackBar")) && ImportedMesh(TEXT("SM_Stock_BackBar_Tobacco")))
	{
		Prop(TEXT("SM_Stock_BackBar"), BackBarAt, 90.0f);
		if (UStaticMeshComponent* Tobacco = Prop(TEXT("SM_Stock_BackBar_Tobacco"), BackBarAt, 90.0f))
		{
			Tobacco->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	else
	{
		Box(Cabinet, FVector(-640.0, StoreY0 + 2.0, 128.0), FVector(-180.0, StoreY0 + 36.0, 248.0));
		static const uint32 Brands[5] = {0xc8202c, 0x1f4fa8, 0x2e8b57, 0xc9a24d, 0x2a2b2e};
		UInstancedStaticMeshComponent* Packs[5];
		for (int32 K = 0; K < 5; ++K)
		{
			Packs[K] = NewPart<UInstancedStaticMeshComponent>();
			Packs[K]->SetStaticMesh(CubeMesh);
			Packs[K]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Packs[K]->SetCastShadow(false);
			Packs[K]->SetMaterial(0, Mat(*FString::Printf(TEXT("Pack%d"), K), Brands[K], 0.45f));
		}
		UInstancedStaticMeshComponent* PackWhite = NewPart<UInstancedStaticMeshComponent>();
		PackWhite->SetStaticMesh(CubeMesh);
		PackWhite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PackWhite->SetCastShadow(false);
		PackWhite->SetMaterial(0, Mat(TEXT("PackWhite"), 0xeeece6, 0.45f));
		for (int32 Row = 0; Row < 8; ++Row)
		{
			const double Z = 134.0 + Row * 13.0;
			for (double X = -632.0; X < -190.0; X += 7.0)
			{
				// A brand runs a few columns wide, as the rack's stocked; each pack two-tone, its color over white.
				// The faces stand on the cabinet's front (Y +36), the acrylic just in front of them.
				const int32 Brand = static_cast<int32>((X + 632.0) / 49.0) % 5;
				Packs[Brand]->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X + 2.8, StoreY0 + 37.2, Z + 6.5), FVector(0.055, 0.022, 0.05)));
				PackWhite->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X + 2.8, StoreY0 + 37.2, Z + 2.0), FVector(0.055, 0.022, 0.04)));
			}
		}
		Box(Mat(TEXT("PriceStrip"), 0xfff4e0, 0.3f, 0.0f, 50.0f), FVector(-638.0, StoreY0 + 36.0, 238.0), FVector(-182.0, StoreY0 + 38.5, 246.0), false)->SetCastShadow(false);
	}
	if (Glass)
	{
		Box(Glass, FVector(-640.0, StoreY0 + 39.0, 128.0), FVector(-180.0, StoreY0 + 40.0, 236.0), false);
	}
	// The menu board over the rack, backlit (lit like the store, so it doesn't follow the street's exposure): the grill
	// and the coffee bar at the catalog's prices, and the lotto (PropArt's MenuBoard, its 1760 x 720 kept in shape).
	const double BoardW = 300.0;
	const double BoardH = BoardW * static_cast<double>(ss::ui::props::MenuBoardH) / static_cast<double>(ss::ui::props::MenuBoardW);
	const double BoardZ = StoreCeiling - 6.0 - BoardH * 0.5;
	Box(Cabinet, FVector(-410.0 - BoardW * 0.5 - 4.0, StoreY0 + 2.0, BoardZ - BoardH * 0.5 - 4.0), FVector(-410.0 + BoardW * 0.5 + 4.0, StoreY0 + 4.0, StoreCeiling), false);
	Sign(TEXT("menu"), FVector(-410.0, StoreY0 + 4.6, BoardZ), 90.0f, FVector2D(BoardW, BoardH), FIntPoint(1760, 720), -60.0f);

	// The coffee bar along the same wall: brewers with their pots on the warmers, cups stacked, a sign over it.
	Box(Mat(TEXT("CoffeeBar"), 0x3b2a20, 0.4f), FVector(X0 + 100.0, StoreY0, 0.0), FVector(-900.0, StoreY0 + 70.0, 95.0));
	UMaterialInterface* Pot = Mat(TEXT("CoffeePot"), 0x1a120c, 0.08f, 0.0f, 0.0f, 0.2f);
	UMaterialInterface* Cup = Mat(TEXT("CoffeeCup"), 0xf5f1e8, 0.6f);
	UMaterialInterface* Handle = Mat(TEXT("PotHandle"), 0xd2601c, 0.5f);
	for (int32 I = 0; I < 3; ++I)
	{
		const double X = -1250.0 + I * 110.0;
		Box(Metal, FVector(X, StoreY0 + 4.0, 95.0), FVector(X + 60.0, StoreY0 + 40.0, 175.0), false);
		Box(Metal, FVector(X + 8.0, StoreY0 + 40.0, 95.0), FVector(X + 52.0, StoreY0 + 64.0, 97.0), false);
		Cyl(Pot, FVector(X + 30.0, StoreY0 + 52.0, 97.0), 8.0f, 18.0f, false);
		Box(Handle, FVector(X + 22.0, StoreY0 + 43.0, 113.0), FVector(X + 38.0, StoreY0 + 61.0, 117.0), false)->SetCastShadow(false);
	}
	for (int32 I = 0; I < 3; ++I)
	{
		Cyl(Cup, FVector(-960.0 + I * 14.0, StoreY0 + 40.0, 95.0), 4.5f, 30.0f + I * 4.0f, false);
	}
	Sign(TEXT("promo:drip-coffee"), FVector(-1140.0, StoreY0 + 3.0, 245.0), 90.0f, FVector2D(46.0, 69.0), FIntPoint(600, 900), -50.0f);

	// The mat inside the door, and a wet-floor sign, because it's raining.
	Box(Mat(TEXT("DoorMat"), 0x1c1d20, 0.95f), FVector(-170.0, DoorY0 - 30.0, 1.0), FVector(-28.0, DoorY1 + 30.0, 1.8), false);
	UMaterialInterface* Caution = Mat(TEXT("Caution"), 0xf2c014, 0.4f);
	for (int32 Side = 0; Side < 2; ++Side)
	{
		UStaticMeshComponent* Leg = Box(Caution, FVector(-181.0 + Side * 14.0, DoorY0 - 140.0, 0.0), FVector(-179.0 + Side * 14.0, DoorY0 - 110.0, 62.0), false);
		Leg->SetRelativeRotation(FRotator(Side == 0 ? -12.0f : 12.0f, 0.0f, 0.0f));
	}
}

void AStreetStage::BuildStreetlights()
{
	UMaterialInterface* Pole = Mat(TEXT("Pole"), 0x2c3033, 0.5f, 0.0f, 0.0f, 0.7f);
	UMaterialInstanceDynamic* Sodium = Mat(TEXT("Sodium"), 0xffb066, 0.3f, 0.0f, 14.0f);
	NightGlowMids.Add(Sodium);
	NightGlowLevels.Add(14.0f);
	struct FPost
	{
		double X, Y;
		float Arm; // the arm's yaw: over the road
	};
	const FPost Posts[] = {{380.0, -4300.0, 0.0f}, {380.0, -2000.0, 0.0f}, {380.0, 700.0, 0.0f}, {380.0, 2700.0, 0.0f}, {380.0, 4700.0, 0.0f}, {380.0, 7600.0, 0.0f},
		{1660.0, -3200.0, 180.0f}, {1660.0, -600.0, 180.0f}, {1660.0, 1800.0, 180.0f}, {1660.0, 4200.0, 180.0f}, {1660.0, 7000.0, 180.0f}, {-1700.0, MarketN + 60.0, -90.0f}};
	int32 Index = 0;
	for (const FPost& P : Posts)
	{
		const FVector Out = FRotator(0.0f, P.Arm, 0.0f).Vector();
		const FVector Head = FVector(P.X, P.Y, 0.0) + Out * 150.0;
		if (!Prop(TEXT("SM_Streetlight"), FVector(P.X, P.Y, 0.0), P.Arm))
		{
			Cyl(Pole, FVector(P.X, P.Y, 0.0), 9.0f, 820.0f);
			const FVector Tip = FVector(P.X, P.Y, 0.0) + Out * 180.0;
			Box(Pole, FVector(FMath::Min(P.X, Tip.X) - 5.0, FMath::Min(P.Y, Tip.Y) - 5.0, 805.0), FVector(FMath::Max(P.X, Tip.X) + 5.0, FMath::Max(P.Y, Tip.Y) + 5.0, 815.0), false);
			Box(Sodium, FVector(Head.X - 22.0, Head.Y - 22.0, 795.0), FVector(Head.X + 22.0, Head.Y + 22.0, 805.0), false)->SetCastShadow(false);
		}
		if (USpotLightComponent* L = NewPart<USpotLightComponent>())
		{
			L->SetRelativeLocationAndRotation(FVector(Head.X, Head.Y, 790.0), FRotator(-90.0f, 0.0f, 0.0f));
			L->SetIntensityUnits(ELightUnits::Candelas);
			L->SetIntensity(StreetlightCandela);
			L->SetAttenuationRadius(2400.0f);
			// A pool with an edge to it under each, the rain bright in the cone.
			L->SetOuterConeAngle(62.0f);
			L->SetInnerConeAngle(26.0f);
			L->SetSourceRadius(12.0f);
			L->SetUseTemperature(true);
			L->SetTemperature(Index % 3 == 2 ? 4000.0f : 2300.0f); // sodium, and the odd newer LED
			L->SetVolumetricScatteringIntensity(1.2f);
			// PickStreetlights hands out the shadows (the nearest few to the camera).
			L->SetCastShadows(false);
			L->SetCastVolumetricShadow(false);
			Streetlights.Add(L);
		}
		++Index;
	}
	// The corner's street blades, crossed on top of their pole (on Market's sidewalk): FIFTH ST runs along Fifth,
	// MARKET ST along Market sitting on it, each a thin green plate printed on both faces.
	const FVector Corner(390.0, StoreY1 + 90.0, 0.0);
	UMaterialInterface* Blade = Mat(TEXT("StreetBlade"), 0x0f6b3a, 0.4f, 0.0f, 0.0f, 0.3f);
	Cyl(Pole, Corner, 5.0f, 340.0f);
	Box(Blade, Corner + FVector(-0.4, -54.0, 340.0), Corner + FVector(0.4, 54.0, 364.0), false)->SetCastShadow(false);
	Box(Blade, Corner + FVector(-54.0, -0.4, 364.0), Corner + FVector(54.0, 0.4, 388.0), false)->SetCastShadow(false);
	Sign(TEXT("street:FIFTH ST:1800"), Corner + FVector(0.8, 0.0, 352.0), 0.0f, FVector2D(110.0, 24.0), FIntPoint(900, 200), 0.0f);
	Sign(TEXT("street:FIFTH ST:1800"), Corner + FVector(-0.8, 0.0, 352.0), 180.0f, FVector2D(110.0, 24.0), FIntPoint(900, 200), 0.0f);
	Sign(TEXT("street:MARKET ST:200"), Corner + FVector(0.0, 0.8, 376.0), 90.0f, FVector2D(110.0, 24.0), FIntPoint(900, 200), 0.0f);
	Sign(TEXT("street:MARKET ST:200"), Corner + FVector(0.0, -0.8, 376.0), -90.0f, FVector2D(110.0, 24.0), FIntPoint(900, 200), 0.0f);
}

void AStreetStage::BuildSkyAndWeather()
{
	// Sky dome and the skyline beyond the blocks.
	if (SkyMaterial)
	{
		UMaterialInstanceDynamic* Sky = UMaterialInstanceDynamic::Create(SkyMaterial, this);
		Sky->SetScalarParameterValue(TEXT("Strength"), 0.6f);
		SkyMids.Add(Sky);
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
		City->SetScalarParameterValue(TEXT("Strength"), 0.55f);
		SkyMids.Add(City);
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

	// The rain: layers at their distances in front of the eye (FollowCamera keeps them there, sized to the view).
	if (RainMaterial)
	{
		for (const FRainLayer& Layer : RainLayers)
		{
			UMaterialInstanceDynamic* Rain = UMaterialInstanceDynamic::Create(RainMaterial, this);
			Rain->SetScalarParameterValue(TEXT("Depth"), static_cast<float>(Layer.Depth));
			Rain->SetScalarParameterValue(TEXT("Spacing"), Layer.Spacing);
			Rain->SetScalarParameterValue(TEXT("WidthCm"), Layer.Width);
			Rain->SetScalarParameterValue(TEXT("LengthCm"), Layer.Length);
			Rain->SetScalarParameterValue(TEXT("Strength"), Layer.Strength);
			UStaticMeshComponent* Sheet = NewPart<UStaticMeshComponent>();
			Sheet->SetStaticMesh(PlaneMesh);
			Sheet->SetMaterial(0, Rain);
			Sheet->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Sheet->SetCastShadow(false);
			Sheet->bAffectDistanceFieldLighting = false;
			Sheet->SetVisibleInRayTracing(false);
			Sheet->SetRelativeScale3D(FVector(Layer.Depth * 3.2 / 100.0, Layer.Depth * 3.0 / 100.0, 1.0));
			// Placed under the street until the camera moves it (never lying flat across the sidewalk).
			Sheet->SetRelativeLocation(FVector(0.0, 0.0, -2000.0));
			RainSheets.Add(Sheet);
			RainMids.Add(Rain);
		}
	}

	// Night: the moon through the clouds (the overcast sun by day), the sky's light, wet haze, the lens.
	Moon = NewPart<UDirectionalLightComponent>();
	Moon->SetRelativeRotation(FRotator(-38.0f, 35.0f, 0.0f));
	Moon->SetIntensity(0.5f);
	Moon->SetLightColor(SrgbHex(0x8fa6d6));
	Moon->SetCastShadows(false);
	Moon->SetVolumetricScatteringIntensity(0.5f);
	SkyLight = NewPart<USkyLightComponent>();
	SkyLight->SourceType = ESkyLightSourceType::SLS_CapturedScene;
	// The dome (1.2 km off) is the sky; the skyline's boxes and the street are not.
	SkyLight->SkyDistanceThreshold = 100000.0f;
	SkyLight->SetIntensity(1.0f);
	SkyLight->SetLightColor(SrgbHex(0x9fb0d0));
	Fog = NewPart<UExponentialHeightFogComponent>();
	Fog->SetFogDensity(0.012f);
	Fog->SetFogHeightFalloff(0.2f);
	Fog->SetFogMaxOpacity(0.9f);
	// The dome (1.2 km off) stays out of it: its glow of city light over the rooftops shows, the skyline fades into the haze.
	Fog->SetFogCutoffDistance(100000.0f);
	Fog->SetFogInscatteringColor(FLinearColor(0.012f, 0.014f, 0.022f));
	// Volumetric fog thin enough to show only where a light passes through it: cones under the lamps, the neon's halo.
	Fog->SetVolumetricFog(true);
	Fog->SetVolumetricFogScatteringDistribution(0.75f);
	Fog->SetVolumetricFogExtinctionScale(1.0f);
	Fog->SetVolumetricFogAlbedo(FColor(190, 196, 205));
	Fog->SetVolumetricFogDistance(4000.0f);
	Lens = NewPart<UPostProcessComponent>();
	Lens->bUnbound = true;
	FPostProcessSettings& S = Lens->Settings;
	// Auto exposure inside clamps that keep a night a night (ApplyLens moves them between the street's and the store's).
	S.bOverride_AutoExposureMethod = true;
	S.AutoExposureMethod = EAutoExposureMethod::AEM_Histogram;
	S.bOverride_AutoExposureLowPercent = true;
	S.AutoExposureLowPercent = 10.0f;
	S.bOverride_AutoExposureHighPercent = true;
	S.AutoExposureHighPercent = 90.0f;
	S.bOverride_HistogramLogMin = true;
	S.HistogramLogMin = -4.0f;
	S.bOverride_HistogramLogMax = true;
	S.HistogramLogMax = 14.0f;
	S.bOverride_AutoExposureMinBrightness = true;
	S.bOverride_AutoExposureMaxBrightness = true;
	S.bOverride_AutoExposureBias = true;
	S.bOverride_AutoExposureSpeedUp = true;
	S.AutoExposureSpeedUp = 2.5f;
	S.bOverride_AutoExposureSpeedDown = true;
	S.AutoExposureSpeedDown = 1.4f;
	S.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	S.AutoExposureApplyPhysicalCameraExposure = 0;
	// Local exposure keeps the lit store and the signs from clipping against the dark street.
	S.bOverride_LocalExposureHighlightContrastScale = true;
	S.LocalExposureHighlightContrastScale = 0.75f;
	S.bOverride_LocalExposureShadowContrastScale = true;
	S.LocalExposureShadowContrastScale = 0.95f;
	S.bOverride_BloomIntensity = true;
	S.BloomIntensity = 0.7f;
	S.bOverride_VignetteIntensity = true;
	S.VignetteIntensity = 0.4f;
	S.bOverride_FilmGrainIntensity = true;
	S.bOverride_SceneFringeIntensity = true;
	S.bOverride_MotionBlurAmount = true;
	// The night grade: the darks a little cooler and greyer, the lamps and neon keeping their color.
	S.bOverride_ColorSaturationShadows = true;
	S.ColorSaturationShadows = FVector4(1.0, 1.0, 1.0, 0.85);
	S.bOverride_ColorOffsetShadows = true;
	S.ColorOffsetShadows = FVector4(0.0, 0.002, 0.006, 0.0);
	S.bOverride_ColorContrast = true;
	S.ColorContrast = FVector4(1.0, 1.0, 1.0, 1.06);
	ApplyLens();
}

void AStreetStage::BuildSpots()
{
	// What the player can do here (plain data: rebuilt at BeginPlay too, whatever the saved level holds).
	Spots.Reset();
	auto Spot = [this](const TCHAR* Id, const FVector& At, float Radius, const TCHAR* Prompt, int32 ShelfIndex) {
		FStreetSpot S;
		S.Id = Id;
		S.At = At;
		S.Radius = Radius;
		S.Prompt = Prompt;
		S.Shelf = ShelfIndex;
		Spots.Add(S);
	};
	const double X0 = -StoreDepth;
	Spot(TEXT("home"), FVector(60.0, 0.0, 100.0), 170.0f, TEXT("Go home"), -1);
	Spot(TEXT("counter"), FVector(-330.0, 4150.0, 100.0), 140.0f, TEXT("Talk to Benny"), -1);
	Spot(TEXT("grill"), FVector(-540.0, 4150.0, 100.0), 110.0f, TEXT("Check the roller grill"), static_cast<int32>(ss::store::Shelf::Hot));
	Spot(TEXT("cooler"), FVector(X0 + 160.0, 4500.0, 100.0), 220.0f, TEXT("Open the cooler"), static_cast<int32>(ss::store::Shelf::Drinks));
	Spot(TEXT("aisle"), FVector(-850.0, 4445.0, 100.0), 230.0f, TEXT("Browse the snacks"), static_cast<int32>(ss::store::Shelf::Snacks));
	Spot(TEXT("coffee"), FVector(-1100.0, StoreY0 + 140.0, 100.0), 170.0f, TEXT("Pour a coffee"), static_cast<int32>(ss::store::Shelf::Coffee));
}

// ------------------------------------------------------------------ play

void AStreetStage::BeginPlay()
{
	Super::BeginPlay();
	BuildSpots();
	AttachSlate();
	PaintCars();
	// Where the store and the laundromat's room are, for the rain (nothing falls inside them, the sheets crossing them
	// as they follow the eye), and the two lit fronts the rain catches (the laundromat's as BuildAcross built it).
	const FTransform& T = GetActorTransform();
	const FBox Store = FBox(FVector(-StoreDepth - 30.0, StoreY0, -60.0), FVector(-6.0, StoreY1, StoreCeiling + 120.0)).TransformBy(T);
	const FBox Wash = FBox(WashMin, WashMax).TransformBy(T);
	const FVector StoreGlow = T.TransformPosition(FVector(60.0, (StoreY0 + StoreY1) * 0.5, 200.0));
	const FVector WashGlow = T.TransformPosition(NeonGlowAt);
	for (UMaterialInstanceDynamic* M : RainMids)
	{
		if (M)
		{
			M->SetVectorParameterValue(TEXT("StoreMin"), FLinearColor(Store.Min));
			M->SetVectorParameterValue(TEXT("StoreMax"), FLinearColor(Store.Max));
			M->SetVectorParameterValue(TEXT("WashMin"), FLinearColor(Wash.Min));
			M->SetVectorParameterValue(TEXT("WashMax"), FLinearColor(Wash.Max));
			M->SetVectorParameterValue(TEXT("GlowPos0"), FLinearColor(StoreGlow));
			M->SetVectorParameterValue(TEXT("GlowCol0"), FLinearColor(0.75f, 0.85f, 1.0f));
			M->SetVectorParameterValue(TEXT("GlowPos1"), FLinearColor(WashGlow));
			// GlowCol1, the neon's pink, follows the hour (ApplyDaylight, just below).
		}
	}
	AppliedDaylight = -1.0f;
	ApplyDaylight();
	PickStreetlights(StartAt(TEXT("home")).GetLocation());
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
	SignSlates.Reset();
	SignLists.Reset();
	// Each sign says what it shows ("kind:arguments", see Sign()).
	for (int32 I = 0; I < SignWidgets.Num() && I < SignArt.Num(); ++I)
	{
		UWidgetComponent* Wc = SignWidgets[I];
		if (!Wc)
		{
			continue;
		}
		TArray<FString> Parts;
		SignArt[I].ParseIntoArray(Parts, TEXT(":"), false);
		const FString Kind = Parts.Num() > 0 ? Parts[0] : FString();
		const std::string A1 = Parts.Num() > 1 ? std::string(TCHAR_TO_UTF8(*Parts[1])) : std::string();
		const std::string A2 = Parts.Num() > 2 ? std::string(TCHAR_TO_UTF8(*Parts[2])) : std::string();
		TSharedPtr<const ss::ui::DrawList> List;
		if (Kind == TEXT("number"))
		{
			List = Draw(P::BuildingNumberW, P::BuildingNumberH, [&A1](ss::ui::Canvas& C) { P::BuildingNumber(C, A1); });
		}
		else if (Kind == TEXT("neon"))
		{
			List = Draw(P::NeonW, P::NeonH, [](ss::ui::Canvas& C) { P::NeonSign(C); });
		}
		else if (Kind == TEXT("storesign"))
		{
			List = Draw(P::StoreSignW, P::StoreSignH, [](ss::ui::Canvas& C) { P::StoreSign(C); });
		}
		else if (Kind == TEXT("open"))
		{
			List = Draw(P::OpenSignW, P::OpenSignH, [](ss::ui::Canvas& C) { P::OpenSign(C); });
		}
		else if (Kind == TEXT("door"))
		{
			List = Draw(P::DoorDecalW, P::DoorDecalH, [](ss::ui::Canvas& C) { P::DoorDecal(C); });
		}
		else if (Kind == TEXT("promo"))
		{
			// No deal given: the poster prints the shelf price.
			List = Draw(P::PromoW, P::PromoH, [&A1, &A2](ss::ui::Canvas& C) { P::Promo(C, A1, A2); });
		}
		else if (Kind == TEXT("menu"))
		{
			List = Draw(P::MenuBoardW, P::MenuBoardH, [](ss::ui::Canvas& C) { P::MenuBoard(C); });
		}
		else if (Kind == TEXT("street"))
		{
			List = Draw(P::StreetSignW, P::StreetSignH, [&A1, &A2](ss::ui::Canvas& C) { P::StreetSign(C, A1, A2); });
		}
		if (!List)
		{
			continue;
		}
		TSharedPtr<SDrawListWidget> Sw = SNew(SDrawListWidget).DesiredSize(FVector2D(List->Width, List->Height));
		Sw->SetDrawList(List);
		SignSlates.Add(Sw);
		SignLists.Add(List);
		Wc->SetSlateWidget(Sw);
		Wc->SetManuallyRedraw(true);
		Wc->RequestRedraw();
	}
}

void AStreetStage::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const UWorld* World = GetWorld();
	const APlayerController* Pc = World ? World->GetFirstPlayerController() : nullptr;
	const APlayerCameraManager* Cam = Pc ? Pc->PlayerCameraManager.Get() : nullptr;
	if (!Cam)
	{
		return;
	}
	// TG_PostUpdateWork: the camera has already moved this frame.
	const FVector At = Cam->GetCameraLocation();
	const float Dt = FMath::Min(DeltaSeconds, 0.1f);
	UpdateLook(At, Dt);
	FollowCamera(At, Cam->GetCameraRotation(), Cam->GetFOVAngle());
}

void AStreetStage::UpdateLook(const FVector& Camera, float Dt)
{
	// How far into the store the camera is: across the threshold the exposure's clamps swing from the street's to the
	// store's (the eye then adapts at the exposure's own speed).
	const FVector L = GetActorTransform().InverseTransformPosition(Camera);
	const bool bInStoreSpan = L.Y > StoreY0 + 5.0 && L.Y < StoreY1 - 5.0 && L.X > -StoreDepth && L.Z < StoreCeiling + 60.0;
	const float Want = bInStoreSpan ? FMath::Clamp(static_cast<float>((40.0 - L.X) / 160.0), 0.0f, 1.0f) : 0.0f;
	Inside = FMath::FInterpTo(Inside, Want, Dt, 8.0f);
	if (FMath::Abs(Inside - Want) < 0.001f)
	{
		Inside = Want;
	}
	ApplyLens();
	PickIn -= Dt;
	if (PickIn <= 0.0f)
	{
		PickIn = 0.4f;
		PickStreetlights(Camera);
	}
}

void AStreetStage::ApplyLens()
{
	if (!Lens)
	{
		return;
	}
	const float D = Daylight;
	const float NightEV = CVarNightEV.GetValueOnGameThread();
	const float OutMin = FMath::Lerp(NightEV, CVarDayEV.GetValueOnGameThread(), D);
	const float OutMax = OutMin + FMath::Lerp(CVarNightRange.GetValueOnGameThread(), 1.5f, D);
	const float InMin = CVarStoreEV.GetValueOnGameThread() + 1.5f * D; // the day comes in through the windows too
	const float InMax = InMin + CVarStoreRange.GetValueOnGameThread();
	FPostProcessSettings& S = Lens->Settings;
	S.AutoExposureMinBrightness = FMath::Lerp(OutMin, InMin, Inside);
	S.AutoExposureMaxBrightness = FMath::Max(S.AutoExposureMinBrightness, FMath::Lerp(OutMax, InMax, Inside));
	S.AutoExposureBias = ExposureBias + BrightnessBias + CVarBias.GetValueOnGameThread();
	S.FilmGrainIntensity = 0.1f * GrainScale;
	S.SceneFringeIntensity = 0.3f * FringeScale;
	S.MotionBlurAmount = bMotionBlur ? 0.3f : 0.0f;

	// What shines by itself out on the street keeps its brightness against the exposure (by day it must shine harder
	// to show at all); the store's own backlit signs follow the store.
	const float Scale = FMath::Pow(2.0f, OutMin - NightEV);
	if (FMath::Abs(Scale - SignScale) > SignScale * 0.01f)
	{
		SignScale = Scale;
		const float SignK = FMath::Pow(Scale, 0.8f);
		for (int32 I = 0; I < SignWidgets.Num() && I < SignGlow.Num(); ++I)
		{
			if (SignWidgets[I] && SignGlow[I] > 0.0f)
			{
				const float Tint = SignGlow[I] * SignK;
				SignWidgets[I]->SetTintColorAndOpacity(FLinearColor(Tint, Tint, Tint, 1.0f));
			}
		}
	}
	const float RainK = Scale * FMath::Max(0.0f, CVarRain.GetValueOnGameThread());
	if (FMath::Abs(RainK - RainScale) > FMath::Max(RainScale, 0.01f) * 0.01f)
	{
		RainScale = RainK;
		for (int32 I = 0; I < RainMids.Num(); ++I)
		{
			if (RainMids[I])
			{
				RainMids[I]->SetScalarParameterValue(TEXT("Strength"), RainLayers[I % 4].Strength * RainK);
			}
		}
	}
}

void AStreetStage::SetLensOptions(float InBrightnessBias, float InGrainScale, float InFringeScale, bool bInMotionBlur)
{
	BrightnessBias = InBrightnessBias;
	GrainScale = InGrainScale;
	FringeScale = InFringeScale;
	bMotionBlur = bInMotionBlur;
	ApplyLens();
}

void AStreetStage::PickStreetlights(const FVector& Camera)
{
	// Shadows (and the fog's) from the few lamps nearest the camera: a far lamp's shadows are too small to see and
	// cost the most. Inside the store none: their light only comes in through the glass.
	TArray<TPair<double, int32>> Near;
	for (int32 I = 0; I < Streetlights.Num(); ++I)
	{
		if (Streetlights[I] && Streetlights[I]->IsVisible())
		{
			Near.Emplace(FVector::DistSquared(Streetlights[I]->GetComponentLocation(), Camera), I);
		}
	}
	Near.Sort([](const TPair<double, int32>& A, const TPair<double, int32>& B) { return A.Key < B.Key; });
	const int32 Budget = Inside > 0.5f ? 0 : FMath::Max(0, ShadowedStreetlights);
	TArray<bool> Shadowed;
	Shadowed.Init(false, Streetlights.Num());
	for (int32 Rank = 0; Rank < Near.Num() && Rank < Budget; ++Rank)
	{
		Shadowed[Near[Rank].Value] = Near[Rank].Key < 3000.0 * 3000.0;
	}
	for (int32 I = 0; I < Streetlights.Num(); ++I)
	{
		USpotLightComponent* L = Streetlights[I];
		if (L && static_cast<bool>(L->CastShadows) != Shadowed[I])
		{
			L->SetCastShadows(Shadowed[I]);
			L->SetCastVolumetricShadow(Shadowed[I]);
		}
	}
	// The store's own lights keep their shadows (and so their light inside its walls) while the camera is near enough to
	// see the store, inside or out; from farther off the difference doesn't show, and their shadows aren't drawn.
	const FVector StoreMiddle = GetActorTransform().TransformPosition(FVector(-StoreDepth * 0.5, (StoreY0 + StoreY1) * 0.5, StoreCeiling * 0.5));
	const bool bStoreShadows = Inside > 0.0f || FVector::DistSquared(StoreMiddle, Camera) < 3000.0 * 3000.0;
	for (URectLightComponent* L : StoreLights)
	{
		if (L && static_cast<bool>(L->CastShadows) != bStoreShadows)
		{
			L->SetCastShadows(bStoreShadows);
		}
	}
	// The rain catches the light of the nearest lamps (and glows in their cones).
	if (!bRainMasksStore)
	{
		return;
	}
	for (int32 K = 0; K < 4; ++K)
	{
		FLinearColor Pos(0.0f, 0.0f, -100000.0f);
		FLinearColor Col(0.0f, 0.0f, 0.0f);
		if (K < Near.Num())
		{
			const USpotLightComponent* L = Streetlights[Near[K].Value];
			Pos = FLinearColor(L->GetComponentLocation());
			const float Strength = L->Intensity / FMath::Max(1.0f, StreetlightCandela);
			Col = (L->Temperature < 3000.0f ? FLinearColor(1.0f, 0.55f, 0.24f) : FLinearColor(0.8f, 0.88f, 1.0f)) * Strength;
		}
		for (UMaterialInstanceDynamic* M : RainMids)
		{
			if (M)
			{
				M->SetVectorParameterValue(*FString::Printf(TEXT("LampPos%d"), K), Pos);
				M->SetVectorParameterValue(*FString::Printf(TEXT("LampCol%d"), K), Col);
			}
		}
	}
}

void AStreetStage::SetDaylight(float InDaylight)
{
	Daylight = FMath::Clamp(InDaylight, 0.0f, 1.0f);
	// Small steps: the sky and the lamps are scaled against an exposure nine stops apart, so a step of 0.02 would be
	// a visible jump in the sky at dusk.
	if (FMath::Abs(Daylight - AppliedDaylight) > 0.004f)
	{
		ApplyDaylight();
	}
}

void AStreetStage::ApplyDaylight()
{
	AppliedDaylight = Daylight;
	const float D = Daylight;
	const float Night = NightLevel(D);
	// Everything that shines by itself is set for the night's exposure; by day the exposure is this many times less.
	const float Scale = FMath::Pow(2.0f, (CVarDayEV.GetValueOnGameThread() - CVarNightEV.GetValueOnGameThread()) * D);
	for (UMaterialInstanceDynamic* M : SkyMids)
	{
		if (M)
		{
			// The sky, and the skyline (dimmer: it's mostly dark windows).
			const UMaterial* Base = M->GetBaseMaterial();
			const bool bSky = Base && Base->GetName().Contains(TEXT("Sky"));
			M->SetScalarParameterValue(TEXT("Dawn"), D);
			M->SetScalarParameterValue(TEXT("Strength"), Scale * (bSky ? FMath::Lerp(0.6f, 2.5f, D) : FMath::Lerp(0.55f, 1.5f, D)));
		}
	}
	for (UMaterialInstanceDynamic* M : WindowMids)
	{
		if (M)
		{
			M->SetScalarParameterValue(TEXT("Glow"), 1.0f - 0.8f * D);
		}
	}
	// By day the rain is grey streaks against the street, lit by the sky rather than the lamps; the neon's pink in it goes
	// with the neon's glow on the street (the store's light is on at every hour).
	for (UMaterialInstanceDynamic* M : RainMids)
	{
		if (M)
		{
			M->SetVectorParameterValue(TEXT("Ambient"), FLinearColor(0.05f, 0.055f, 0.07f) * FMath::Lerp(1.0f, 6.0f, D));
			M->SetVectorParameterValue(TEXT("GlowCol1"), FLinearColor(0.9f, 0.3f, 0.55f) * Night);
		}
	}
	for (int32 I = 0; I < NightGlowMids.Num() && I < NightGlowLevels.Num(); ++I)
	{
		if (UMaterialInstanceDynamic* M = NightGlowMids[I])
		{
			M->SetScalarParameterValue(TEXT("Emissive"), NightGlowLevels[I] * Night);
			M->SetScalarParameterValue(TEXT("Strength"), NightGlowLevels[I] * Night);
		}
	}
	for (int32 I = 0; I < NightLights.Num() && I < NightLightLevels.Num(); ++I)
	{
		if (ULightComponent* L = NightLights[I])
		{
			L->SetIntensity(NightLightLevels[I] * Night);
			L->SetVisibility(Night > 0.01f);
		}
	}
	// Blender's fronts glow by themselves too (their lit rooms, lanterns and transoms, baked for the night): out by day
	// with the lamps.
	for (UStaticMeshComponent* Lit : LitFronts)
	{
		DimImportedGlow(Lit, Night * (Lit == TenementFront ? CVarTenementGlow.GetValueOnGameThread() : 1.0f));
	}
	// The laundromat's shop window is the brightest of the tenement's bakes (tenement_store, the bake's M_SM_Tenement_9):
	// made to read through the apartment's rainy window, it whites out against the street's night exposure.
	if (const UStaticMesh* Tenement = TenementFront ? TenementFront->GetStaticMesh().Get() : nullptr)
	{
		for (int32 Slot = 0; Slot < Tenement->GetStaticMaterials().Num(); ++Slot)
		{
			const UMaterialInterface* Baked = Tenement->GetMaterial(Slot);
			UMaterialInstanceDynamic* Dim = Cast<UMaterialInstanceDynamic>(TenementFront->GetMaterial(Slot));
			if (Baked && Dim && Baked->GetName().EndsWith(TEXT("_9")))
			{
				const float Level = Night * CVarLaundromatGlow.GetValueOnGameThread();
				Dim->SetVectorParameterValue(TEXT("EmissiveFactor"), FLinearColor(Level, Level, Level, 1.0f));
			}
		}
	}
	// The streetlights' photocells, each a little apart.
	for (int32 I = 0; I < Streetlights.Num(); ++I)
	{
		if (USpotLightComponent* L = Streetlights[I])
		{
			const bool bOn = D < 0.45f + 0.04f * static_cast<float>(I % 4);
			if (L->IsVisible() != bOn)
			{
				L->SetVisibility(bOn);
			}
		}
	}
	if (Moon)
	{
		// Moonlight through the clouds at night; the overcast sun by day (a soft, high light, with shadows).
		Moon->SetIntensity(FMath::Exp(FMath::Lerp(FMath::Loge(0.5f), FMath::Loge(3000.0f), D)));
		Moon->SetLightColor(FMath::Lerp(SrgbHex(0x8fa6d6), SrgbHex(0xd8dde6), D));
		const bool bShadows = D > 0.25f;
		if (static_cast<bool>(Moon->CastShadows) != bShadows)
		{
			Moon->SetCastShadows(bShadows);
		}
	}
	if (Fog)
	{
		// The haze takes the sky's color at the horizon: near black-blue at night, the grey of rain cloud by day.
		Fog->SetFogInscatteringColor(FMath::Lerp(FLinearColor(0.012f, 0.014f, 0.022f), FLinearColor(0.3f, 0.32f, 0.36f), D) * (Scale * FMath::Lerp(1.0f, 3.0f, D)));
	}
	if (SkyLight && FMath::Abs(D - CapturedDaylight) > 0.08f)
	{
		CapturedDaylight = D;
		if (HasActorBegunPlay())
		{
			SkyLight->RecaptureSky();
		}
	}
	SignScale = -1.0f;
	RainScale = -1.0f;
	ApplyLens();
}

void AStreetStage::FollowCamera(const FVector& Location, const FRotator& Rotation, float Fov)
{
	if (RainSheets.Num() == 0)
	{
		return;
	}
	// Each layer stands upright Depth ahead, facing the eye, wide enough for the view and tall enough to look up or
	// down through. The streaks are laid out by angle around the eye (M_StreetRain), so turning doesn't drag them.
	const FVector Fwd = FRotator(0.0f, Rotation.Yaw, 0.0f).Vector();
	const FQuat Facing = FRotationMatrix::MakeFromZX(-Fwd, FVector::UpVector).ToQuat();
	// Looking up (the camera goes to 65 degrees) the view's top corners swing wide and high: the sheets reach 78 degrees
	// up and down and widen with the pitch, and their edges fade (M_StreetRain), so the rain never stops on a line.
	const double Pitch = FMath::Abs(FRotator::NormalizeAxis(Rotation.Pitch));
	const double HalfW = FMath::Tan(FMath::DegreesToRadians(FMath::Min(80.0, static_cast<double>(Fov) * 0.5 + 15.0 + Pitch * 0.5)));
	const double HalfH = FMath::Tan(FMath::DegreesToRadians(78.0));
	const FVector LocalFwd = GetActorTransform().InverseTransformVectorNoScale(Fwd);
	// Inside, looking away from the windows: no rain to see (the material hides what's in the store anyway).
	const bool bAny = CVarRain.GetValueOnGameThread() > 0.0f && !(Inside > 0.5f && LocalFwd.X < 0.15);
	// How wide a pixel is (radians), so each streak stays a pixel or two wide at any distance and resolution.
	FVector2D View(2560.0, 1440.0);
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->GetViewportSize(View);
	}
	const float PixelAngle = FMath::DegreesToRadians(FMath::Max(Fov, 10.0f)) / static_cast<float>(FMath::Max(320.0, View.X));
	if (FMath::Abs(PixelAngle - RainPixelAngle) > RainPixelAngle * 0.02f)
	{
		RainPixelAngle = PixelAngle;
		for (UMaterialInstanceDynamic* M : RainMids)
		{
			if (M)
			{
				M->SetScalarParameterValue(TEXT("PixelAngle"), PixelAngle);
			}
		}
	}
	// Out on the street the rain that shows is nearer than any shop glass (what falls behind it is masked), so it draws
	// over the glass; from inside the store it's beyond the windows, and the glass and its beads draw over it (the
	// windows' panes sort at 2).
	const int32 Priority = Inside > 0.5f ? 0 : 3;
	for (int32 I = 0; I < RainSheets.Num(); ++I)
	{
		UStaticMeshComponent* Sheet = RainSheets[I];
		if (!Sheet)
		{
			continue;
		}
		if (Sheet->TranslucencySortPriority != Priority)
		{
			Sheet->SetTranslucentSortPriority(Priority);
		}
		const double Depth = RainLayers[I % 4].Depth;
		const FVector At = Location + Fwd * Depth;
		bool bShow = bAny;
		if (bShow && !bRainMasksStore)
		{
			// The apartment's rain can't hide itself inside the store: keep its sheets out of it.
			bShow = Inside < 0.05f && !IsInsideStore(At);
		}
		if (Sheet->IsVisible() != bShow)
		{
			Sheet->SetVisibility(bShow);
		}
		if (bShow)
		{
			// The plane's local X is up, its Y across (MakeFromZX), each 100 cm.
			Sheet->SetWorldTransform(FTransform(Facing, At, FVector(2.0 * Depth * HalfH / 100.0 + 4.0, 2.0 * Depth * HalfW / 100.0, 1.0)));
		}
	}
}

bool AStreetStage::UpdateDoors(const FVector& Someone, float DeltaSeconds)
{
	const FVector Local = GetActorTransform().InverseTransformPosition(Someone);
	const bool bNear = FMath::Abs(Local.X) < 260.0 && Local.Y > DoorY0 - 220.0 && Local.Y < DoorY1 + 220.0;
	const bool bOpening = bNear && !bDoorsNear && DoorOpen < 0.2f;
	bDoorsNear = bNear;
	const float Was = DoorOpen;
	DoorOpen = FMath::FInterpTo(DoorOpen, bNear ? 1.0f : 0.0f, DeltaSeconds, bNear ? 5.0f : 2.5f);
	if (FMath::IsNearlyEqual(Was, DoorOpen, 1e-4f) && !bOpening)
	{
		return false;
	}
	// Each leaf slides out behind the fixed glass beside it (the sticker goes with the left one).
	const double Leaf = (DoorY1 - DoorY0) * 0.25;
	const double Travel = DoorOpen * Leaf * 2.0 * 0.95;
	if (DoorLeft)
	{
		DoorLeft->SetRelativeLocation(FVector(DoorTrackX, DoorY0 + Leaf - Travel, 0.0));
	}
	if (DoorRight)
	{
		DoorRight->SetRelativeLocation(FVector(DoorTrackX, DoorY1 - Leaf + Travel, 0.0));
	}
	return bOpening;
}

FTransform AStreetStage::StartAt(const FString& From) const
{
	const FTransform Local = From == TEXT("store") ? FTransform(FRotator(0.0f, 180.0f, 0.0f), FVector(-200.0, 4700.0, 100.0)) : FTransform(FRotator(0.0f, 65.0f, 0.0f), FVector(150.0, 80.0, 100.0));
	return Local * GetActorTransform();
}

FTransform AStreetStage::ClerkSpot() const
{
	// Beside the register (not hidden behind its screen), between it and the grill; 1 cm is the floor's top, 87.5 the
	// half height of a 175 cm capsule (SpawnClerk sets his feet on the floor exactly once he's dressed).
	return FTransform(FRotator(0.0f, 90.0f, 0.0f), FVector(-390.0, 3885.0, 1.0 + 87.5)) * GetActorTransform();
}

double AStreetStage::StoreFloorZ() const
{
	return GetActorTransform().TransformPosition(FVector(-700.0, 4300.0, 1.0)).Z;
}

bool AStreetStage::IsInsideStore(const FVector& At) const
{
	const FVector L = GetActorTransform().InverseTransformPosition(At);
	return L.X < -10.0 && L.X > -StoreDepth && L.Y > StoreY0 && L.Y < StoreY1;
}

bool AStreetStage::IsOffTheSet(const FVector& At) const
{
	const FVector L = GetActorTransform().InverseTransformPosition(At);
	return L.Z < -400.0 || L.Y < EndY0 - 600.0 || L.Y > EndY1 + 600.0 || L.X < MarketWest - 900.0 || L.X > FarFront + 1600.0;
}

FString AStreetStage::PlaceName(const FVector& At) const
{
	if (IsInsideStore(At))
	{
		return TEXT("LUCKY PENNY #212");
	}
	const FVector L = GetActorTransform().InverseTransformPosition(At);
	if (L.X < -100.0 && L.Y > StoreY1 - 50.0 && L.Y < MarketY1 + 50.0)
	{
		return TEXT("MARKET STREET");
	}
	return L.Y > StoreY1 - 100.0 && L.Y < MarketY1 + 300.0 ? TEXT("FIFTH & MARKET") : TEXT("FIFTH STREET");
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
