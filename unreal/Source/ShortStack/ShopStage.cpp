#include "ShopStage.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "ShortStack/Game/Wardrobe.h"
#include "SlateDrawList.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectIterator.h"

DEFINE_LOG_CATEGORY_STATIC(LogShopStage, Log, All);

namespace ShopStageDetail
{
// The wake rule (spec 3.8): awake with the hero inside, within 25 m of the room, or within 40 m with its glass in view.
constexpr double WakeNear = 2500.0;
constexpr double WakeSeen = 4000.0;
constexpr float WakeEvery = 0.25f;
// Inside a shop the exposure has its own clamps (EV100, spec 3.2): night and day; dusk and an overcast day between.
constexpr float NightEvMin = 4.5f;
constexpr float NightEvMax = 6.5f;
constexpr float DayEvMin = 5.5f;
constexpr float DayEvMax = 7.5f;
// M_Street's pattern for a rolled-down shop gate (street_setup.py).
constexpr float PatShutter = 6.0f;
// A carved block's pieces thinner than this are dropped (a slab of a centimeter under a floor would only flicker).
constexpr double Sliver = 2.0;
// How often the carves are checked against the street (it may have built again) and the hidden originals kept hidden.
constexpr float CarveEvery = 1.0f;
// The capture mirror (spec 3.4's fallback): its width in pixels and its rate.
constexpr int32 MirrorPixels = 960;
constexpr float MirrorRate = 1.0f / 30.0f;
// The art grille's curtain rolled up into its hood (its Z scale; 1 is down).
constexpr float GrilleRolled = 0.02f;

// The shops' runtime materials (shops_setup.py) and the shared ones.
const TCHAR* const MirrorPath = TEXT("/Game/ShortStack/Materials/M_NS_Mirror.M_NS_Mirror");
const TCHAR* const PolePath = TEXT("/Game/ShortStack/Materials/M_NS_BarberPole.M_NS_BarberPole");
const TCHAR* const BeltPath = TEXT("/Game/ShortStack/Materials/M_NS_Belt.M_NS_Belt");
const TCHAR* const ClothPath = TEXT("/Game/ShortStack/Materials/M_NS_ClothTint.M_NS_ClothTint");
const TCHAR* const NsGlassPath = TEXT("/Game/ShortStack/Materials/M_NS_Glass.M_NS_Glass");
const TCHAR* const StoreGlassPath = TEXT("/Game/ShortStack/Materials/M_StreetGlass.M_StreetGlass");
const TCHAR* const StreetPath = TEXT("/Game/ShortStack/Materials/M_Street.M_Street");
const TCHAR* const WidgetLitPath = TEXT("/Game/ShortStack/Materials/M_WidgetLit.M_WidgetLit");

TAutoConsoleVariable<int32> CVarShopMirror(TEXT("ss.Shop.Mirror"), ShopMirror::Lumen,
	TEXT("How the shops' mirrors reflect: 0 Lumen with hit lighting in the shop's volume (default), 1 a SceneCapture while the hero is at the mirror, 2 Lumen's default (surface cache)."));
static FStreetOverrides::FRegister MirrorOverride(TEXT("ss.Shop.Mirror"), TEXT("0"));

FLinearColor SrgbHex(uint32 Hex)
{
	return FLinearColor::FromSRGBColor(FColor(static_cast<uint8>((Hex >> 16) & 0xff), static_cast<uint8>((Hex >> 8) & 0xff), static_cast<uint8>(Hex & 0xff)));
}

FVector Along(float Yaw)
{
	const double R = FMath::DegreesToRadians(static_cast<double>(Yaw));
	return FVector(FMath::Cos(R), FMath::Sin(R), 0.0);
}

UMaterialInterface* Optional(const TCHAR* Path)
{
	return LoadObject<UMaterialInterface>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
}

std::string Utf8(const FString& S)
{
	return std::string(TCHAR_TO_UTF8(*S));
}

/** The parameters an imported material glows by: its emissive color factors that aren't black, or else its emissive strengths (as the street's DimImportedGlow reads the glTF importer's). */
bool GlowParams(const UMaterialInterface* Baked, TArray<TPair<FName, FLinearColor>>& Colors, TArray<TPair<FName, float>>& Strengths)
{
	Colors.Reset();
	Strengths.Reset();
	if (!Baked)
	{
		return false;
	}
	auto Key = [](const FName& Name) { return Name.ToString().ToLower().Replace(TEXT("_"), TEXT("")).Replace(TEXT(" "), TEXT("")); };
	bool bFactor = false;
	TArray<FMaterialParameterInfo> Infos;
	TArray<FGuid> Ids;
	Baked->GetAllVectorParameterInfo(Infos, Ids);
	for (const FMaterialParameterInfo& Info : Infos)
	{
		const FString K = Key(Info.Name);
		FLinearColor Value;
		if ((K.StartsWith(TEXT("emissivefactor")) || K == TEXT("emissivecolor")) && Baked->GetVectorParameterValue(FHashedMaterialParameterInfo(Info), Value))
		{
			bFactor = true;
			if (!Value.IsAlmostBlack())
			{
				Colors.Emplace(Info.Name, Value);
			}
		}
	}
	if (!bFactor)
	{
		Infos.Reset();
		Ids.Reset();
		Baked->GetAllScalarParameterInfo(Infos, Ids);
		for (const FMaterialParameterInfo& Info : Infos)
		{
			const FString K = Key(Info.Name);
			float Value = 0.0f;
			if ((K == TEXT("emissivestrength") || K == TEXT("emissiveintensity")) && Baked->GetScalarParameterValue(FHashedMaterialParameterInfo(Info), Value) && Value > 0.0f)
			{
				Strengths.Emplace(Info.Name, Value);
			}
		}
	}
	return Colors.Num() > 0 || Strengths.Num() > 0;
}

/** The imported material's base color texture (the glTF importer's parameter by any of its names), or null. */
UTexture* BaseTextureOf(const UMaterialInterface* Baked)
{
	if (!Baked)
	{
		return nullptr;
	}
	TArray<FMaterialParameterInfo> Infos;
	TArray<FGuid> Ids;
	Baked->GetAllTextureParameterInfo(Infos, Ids);
	for (const FMaterialParameterInfo& Info : Infos)
	{
		const FString K = Info.Name.ToString().ToLower().Replace(TEXT("_"), TEXT("")).Replace(TEXT(" "), TEXT(""));
		UTexture* Tex = nullptr;
		if ((K.Contains(TEXT("basecolor")) || K.Contains(TEXT("diffuse")) || K.Contains(TEXT("albedo"))) && Baked->GetTextureParameterValue(FHashedMaterialParameterInfo(Info), Tex) && Tex)
		{
			return Tex;
		}
	}
	return nullptr;
}

/** Splits Box by Hole into the pieces of it outside the hole (axis-aligned, the same frame); slivers dropped. */
void Subtract(const FBox& Box, const FBox& Hole, TArray<FBox>& Out)
{
	if (!Box.Intersect(Hole) || Box.Max.X <= Hole.Min.X || Box.Min.X >= Hole.Max.X || Box.Max.Y <= Hole.Min.Y || Box.Min.Y >= Hole.Max.Y || Box.Max.Z <= Hole.Min.Z ||
		Box.Min.Z >= Hole.Max.Z)
	{
		Out.Add(Box);
		return;
	}
	auto Add = [&Out](const FVector& Mn, const FVector& Mx) {
		const FVector Size = Mx - Mn;
		if (Size.X >= Sliver && Size.Y >= Sliver && Size.Z >= Sliver)
		{
			Out.Add(FBox(Mn, Mx));
		}
	};
	if (Box.Min.X < Hole.Min.X)
	{
		Add(Box.Min, FVector(Hole.Min.X, Box.Max.Y, Box.Max.Z));
	}
	if (Box.Max.X > Hole.Max.X)
	{
		Add(FVector(Hole.Max.X, Box.Min.Y, Box.Min.Z), Box.Max);
	}
	const double X0 = FMath::Max(Box.Min.X, Hole.Min.X);
	const double X1 = FMath::Min(Box.Max.X, Hole.Max.X);
	if (Box.Min.Y < Hole.Min.Y)
	{
		Add(FVector(X0, Box.Min.Y, Box.Min.Z), FVector(X1, Hole.Min.Y, Box.Max.Z));
	}
	if (Box.Max.Y > Hole.Max.Y)
	{
		Add(FVector(X0, Hole.Max.Y, Box.Min.Z), FVector(X1, Box.Max.Y, Box.Max.Z));
	}
	const double Y0 = FMath::Max(Box.Min.Y, Hole.Min.Y);
	const double Y1 = FMath::Min(Box.Max.Y, Hole.Max.Y);
	if (Box.Min.Z < Hole.Min.Z)
	{
		Add(FVector(X0, Y0, Box.Min.Z), FVector(X1, Y1, Hole.Min.Z));
	}
	if (Box.Max.Z > Hole.Max.Z)
	{
		Add(FVector(X0, Y0, Hole.Max.Z), FVector(X1, Y1, Box.Max.Z));
	}
}

/** Whether a rotation is a quarter turn about Z (or none): a cube under it is still an axis-aligned box. */
bool IsSquare(const FQuat& Q)
{
	const FRotator R = Q.Rotator();
	auto Quarter = [](double Deg) {
		const double M = FMath::Fmod(FMath::Abs(Deg) + 0.05, 90.0);
		return M < 0.1;
	};
	return Quarter(R.Yaw) && FMath::Abs(R.Pitch) < 0.05 && FMath::Abs(R.Roll) < 0.05;
}
} // namespace ShopStageDetail

using namespace ShopStageDetail;

AShopStage::AShopStage()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork; // after the camera has moved (the wake rule reads it)
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

// ------------------------------------------------------------------ building it

const TArray<FName>& AShopStage::Venues()
{
	static const TArray<FName> All = {FName(TEXT("barber")), FName(TEXT("thrift")), FName(TEXT("gym")), FName(TEXT("diner")), FName(TEXT("pawn"))};
	return All;
}

AShopStage* AShopStage::FindOrSpawn(UWorld* InWorld, TSubclassOf<AShopStage> Class, const FTransform& StageFrame)
{
	if (!InWorld || !Class || Class->HasAnyClassFlags(CLASS_Abstract))
	{
		return nullptr;
	}
	AShopStage* Shop = nullptr;
	for (TActorIterator<AShopStage> It(InWorld, Class); It && !Shop; ++It)
	{
		Shop = *It;
	}
	if (!Shop)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags |= RF_Transient;
		Shop = InWorld->SpawnActor<AShopStage>(Class, StageFrame, Params);
	}
	if (Shop)
	{
		Shop->EnsureBuilt();
	}
	return Shop;
}

AShopStage* AShopStage::FindOrSpawnVenue(UWorld* InWorld, FName InVenue, const FTransform& StageFrame)
{
	const TSubclassOf<AShopStage> Class = ClassFor(InVenue);
	if (!Class)
	{
		UE_LOG(LogShopStage, Warning, TEXT("No shop stage for the venue %s"), *InVenue.ToString());
		return nullptr;
	}
	return FindOrSpawn(InWorld, Class, StageFrame);
}

TSubclassOf<AShopStage> AShopStage::ClassFor(FName InVenue)
{
	if (InVenue.IsNone())
	{
		return nullptr;
	}
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* C = *It;
		if (!C->IsChildOf(AShopStage::StaticClass()) || C->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
		{
			continue;
		}
		const AShopStage* Defaults = C->GetDefaultObject<AShopStage>();
		if (Defaults && Defaults->Venue == InVenue)
		{
			return C;
		}
	}
	return nullptr;
}

void AShopStage::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UWorld* InWorld = GetWorld();
	if (InWorld && InWorld->IsGameWorld() && bBuilt)
	{
		// Construction run again at play has just cleared the room built for play: built again (as the street does).
		bBuilt = false;
		EnsureBuilt();
		return;
	}
	if (!InWorld || InWorld->WorldType != EWorldType::Editor || !bEditorPreview)
	{
		// In play the room is built at BeginPlay (or when it's spawned: FindOrSpawn).
		return;
	}
	ResetParts();
	BuildInterior();
}

void AShopStage::BeginPlay()
{
	Super::BeginPlay();
	EnsureBuilt();
}

void AShopStage::EndPlay(const EEndPlayReason::Type Reason)
{
	Unregister();
	// What it carved out of the street stands again when the shop goes first (the street outlives it). Not as the whole world
	// ends (a level change, the end of Play): the street goes with it, and its physics shouldn't be rebuilt on the way out.
	if (Reason == EEndPlayReason::Destroyed || Reason == EEndPlayReason::RemovedFromWorld)
	{
		for (const FCarved& C : Carved)
		{
			if (UStaticMeshComponent* O = C.Original.Get())
			{
				O->SetVisibility(C.bWasVisible);
				O->SetCollisionEnabled(C.WasCollision);
			}
		}
	}
	Carved.Reset();
	Super::EndPlay(Reason);
}

void AShopStage::ResetParts()
{
	Materials.Reset();
	Lights.Reset();
	Glows.Reset();
	Spins.Reset();
	Clocks.Reset();
	Screens.Reset();
	ScreenSlates.Reset();
	Seats.Reset();
	ShotMarks.Reset();
	CarveHoles.Reset();
	MirrorPlanes.Reset();
	PoleMids.Reset();
	BeltMids.Reset();
	MirrorMids.Reset();
	DoorLeaf = nullptr;
	DoorLeaf2 = nullptr;
	DoorStop = nullptr;
	GrilleRoot = nullptr;
	GrilleArt = nullptr;
	RoomVolume = nullptr;
	RoomLens = nullptr;
	MirrorCapture = nullptr;
	bHasDoor = false;
	bHasGlass = false;
	RoomBox = FBox(ForceInit);
	GrilleHeight = 0.0;
	DoorAmount = 0.0f;
	bDoorHeld = false;
	DoorClear = 0.0f;
	BuzzIn = -1.0f;
	AppliedDaylight = -1.0f;
	AppliedMirrorMode = -1;
	StreetMaterial = Optional(StreetPath);
	MirrorMaterial = Optional(MirrorPath);
	PoleMaterial = Optional(PolePath);
	BeltMaterial = Optional(BeltPath);
	ClothTintMaterial = Optional(ClothPath);
	ShopGlassMaterial = Optional(NsGlassPath);
}

void AShopStage::EnsureBuilt()
{
	if (bBuilt)
	{
		return;
	}
	UWorld* InWorld = GetWorld();
	if (!InWorld || !InWorld->IsGameWorld())
	{
		return;
	}
	const double Started = FPlatformTime::Seconds();
	// Whatever of the editor's preview came along: gone, deepest first (as the street and the Embercrest clear theirs).
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
	for (const FCarved& C : Carved)
	{
		if (UStaticMeshComponent* O = C.Original.Get())
		{
			O->SetVisibility(C.bWasVisible);
			O->SetCollisionEnabled(C.WasCollision);
		}
	}
	Carved.Reset();
	Unregister();
	ResetParts();
	BuildInterior();
	bBuilt = true;
	bHourSet = false;
	GrilleOpen = bOpen ? 1.0f : 0.0f;
	ApplyGrille();
	ApplyLights();
	ApplyExposure();
	ApplyMirrorMode();
	ApplyCarves();
	UE_LOG(LogShopStage, Log, TEXT("%s built in %.0f ms: %d parts, %d lights, %d glows, %d uses, %d seats, %d carved"), *VenueName, (FPlatformTime::Seconds() - Started) * 1000.0,
		BlueprintCreatedComponents.Num(), Lights.Num(), Glows.Num(), UseHandles.Num(), Seats.Num(), Carved.Num());
}

UStreetWorld* AShopStage::Registry() const
{
	const UWorld* InWorld = GetWorld();
	return InWorld && InWorld->IsGameWorld() ? UStreetWorld::Get(this) : nullptr;
}

void AShopStage::Unregister()
{
	UStreetWorld* R = Registry();
	if (R)
	{
		for (int32 Handle : UseHandles)
		{
			R->RemoveUse(Handle);
		}
		for (const FName& Id : StartIds)
		{
			R->RemoveStart(Id);
		}
		for (const FName& Id : ShotIds)
		{
			R->RemoveShot(Id);
		}
		R->RemoveVenue(Venue);
		for (const FName& Id : PorchZones)
		{
			R->RemoveZone(Id);
		}
	}
	UseHandles.Reset();
	StartIds.Reset();
	ShotIds.Reset();
	PorchZones.Reset();
}

UStaticMesh* AShopStage::ShopMesh(const TCHAR* Name)
{
	return LoadObject<UStaticMesh>(nullptr, *FStreetPreload::MeshPath(Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
}

void AShopStage::AppendShopMaterials(TArray<FString>& Paths)
{
	for (const TCHAR* Path : {MirrorPath, PolePath, BeltPath, ClothPath, NsGlassPath, StoreGlassPath, StreetPath, WidgetLitPath})
	{
		Paths.Add(Path);
	}
}

FString AShopStage::Dollars(int64 Cents)
{
	const int64 A = FMath::Abs(Cents);
	return FString::Printf(TEXT("%s$%lld.%02lld"), Cents < 0 ? TEXT("-") : TEXT(""), A / 100, A % 100);
}

uint64 AShopStage::WearableKinds()
{
	using ss::wardrobe::Cut;
	// Whether a package is imported, asked once a session per path (the garments and the wear don't come and go in play).
	static TMap<FString, bool> Known;
	auto Imported = [](const FString& PackagePath) {
		if (const bool* Found = Known.Find(PackagePath))
		{
			return *Found;
		}
		const bool bThere = FPackageName::DoesPackageExist(PackagePath);
		Known.Add(PackagePath, bThere);
		return bThere;
	};
	auto Bit = [](Cut C) { return 1ull << static_cast<int>(C); };
	auto Garment = [&Imported](const TCHAR* Kind) {
		return Imported(FString::Printf(TEXT("/Game/ShortStack/Garments/SK_Garment_%s_A"), Kind)) || Imported(FString::Printf(TEXT("/Game/ShortStack/Garments/SK_Garment_%s_B"), Kind));
	};
	auto Wear = [&Imported](const TCHAR* Name) { return Imported(FString::Printf(TEXT("/Game/ShortStack/Meshes/%s/%s"), Name, Name)); };
	uint64 Kinds = Bit(Cut::Tee) | Bit(Cut::Shorts) | Bit(Cut::Sneakers);
	const TPair<Cut, const TCHAR*> Garments[] = {{Cut::Hoodie, TEXT("Hoodie")}, {Cut::Bomber, TEXT("Jacket")}, {Cut::Flannel, TEXT("Flannel")}, {Cut::Denim, TEXT("Denim")},
		{Cut::Leather, TEXT("Leather")}, {Cut::Track, TEXT("Track")}, {Cut::Jeans, TEXT("Jeans")}, {Cut::Chinos, TEXT("Chinos")}, {Cut::WorkPants, TEXT("WorkPants")}};
	for (const TPair<Cut, const TCHAR*>& G : Garments)
	{
		Kinds |= Garment(G.Value) ? Bit(G.Key) : 0ull;
	}
	Kinds |= Wear(TEXT("SM_WorkShoe_L")) ? Bit(Cut::WorkShoes) : 0ull;
	Kinds |= Wear(TEXT("SM_Hat_Beanie")) || Wear(TEXT("SM_Wear_Beanie")) ? Bit(Cut::Beanie) : 0ull;
	Kinds |= Wear(TEXT("SM_Hat_Cap")) || Wear(TEXT("SM_Wear_Cap")) ? Bit(Cut::BallCap) | Bit(Cut::CapBack) : 0ull;
	Kinds |= Wear(TEXT("SM_Hat_Bucket")) ? Bit(Cut::Bucket) : 0ull;
	Kinds |= Wear(TEXT("SM_Wear_Trilby")) ? Bit(Cut::Trilby) : 0ull;
	Kinds |= Wear(TEXT("SM_Glasses_Round")) ? Bit(Cut::Round) : 0ull;
	Kinds |= Wear(TEXT("SM_Glasses_Square")) ? Bit(Cut::Square) : 0ull;
	Kinds |= Wear(TEXT("SM_Glasses_Wire")) ? Bit(Cut::Wire) : 0ull;
	Kinds |= Wear(TEXT("SM_Glasses_Shades")) || Wear(TEXT("SM_Wear_Shades")) ? Bit(Cut::Shades) : 0ull;
	Kinds |= Wear(TEXT("SM_Wear_Aviators")) ? Bit(Cut::Aviators) : 0ull;
	return Kinds;
}

TArray<TPair<FName, float>> AShopStage::Cinematic(FName Service) const
{
	return {};
}

void AShopStage::OnShot(FName Shot, bool bActive)
{
}

double AShopStage::MinuteOfDay() const
{
	return WorldMinutes - FMath::FloorToDouble(WorldMinutes / 1440.0) * 1440.0;
}

// ------------------------------------------------------------------ helpers

void AShopStage::DropPart(UActorComponent* Part)
{
	if (!Part)
	{
		return;
	}
	BlueprintCreatedComponents.Remove(Part);
	if (const UWidgetComponent* Wc = Cast<UWidgetComponent>(Part))
	{
		// A screen (the pawn's tags as the shelf changes): its slot stays, emptied, since the subclasses hold screens by index;
		// its drawing goes with it. Empties at the end are trimmed (no live index points past a live screen).
		const int32 Index = Screens.IndexOfByKey(Wc);
		if (Index != INDEX_NONE)
		{
			Screens[Index] = nullptr;
			ScreenSlates[Index].Reset();
		}
		while (Screens.Num() > 0 && !Screens.Last())
		{
			Screens.Pop(EAllowShrinking::No);
			ScreenSlates.Pop(EAllowShrinking::No);
		}
	}
	Part->DestroyComponent();
}

UMaterialInstanceDynamic* AShopStage::Mat(FName Key, uint32 SrgbHexColor, float Roughness, float Pattern, float Emissive, float Metallic)
{
	if (TObjectPtr<UMaterialInstanceDynamic>* Found = Materials.Find(Key))
	{
		return *Found;
	}
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(StreetMaterial ? StreetMaterial.Get() : FallbackMaterial.Get(), this);
	const FLinearColor Color = SrgbHex(SrgbHexColor);
	M->SetVectorParameterValue(TEXT("BaseColor"), Color);
	M->SetVectorParameterValue(TEXT("Color"), Color);
	M->SetScalarParameterValue(TEXT("Roughness"), Roughness);
	M->SetScalarParameterValue(TEXT("Metallic"), Metallic);
	M->SetScalarParameterValue(TEXT("Pattern"), Pattern);
	M->SetScalarParameterValue(TEXT("Emissive"), Emissive);
	M->SetScalarParameterValue(TEXT("Wet"), 0.0f); // indoors
	Materials.Add(Key, M);
	return M;
}

UMaterialInterface* AShopStage::GlassMat()
{
	if (ShopGlassMaterial)
	{
		return ShopGlassMaterial;
	}
	if (UMaterialInterface* Store = Optional(StoreGlassPath))
	{
		return Store;
	}
	return Mat(TEXT("GlassDark"), 0x0b0e12, 0.04f);
}

UStaticMeshComponent* AShopStage::Block(UMaterialInterface* Material, const FVector& Min, const FVector& Max, bool bCollide, USceneComponent* Parent)
{
	UStaticMeshComponent* C = NewPart<UStaticMeshComponent>(Parent);
	C->SetStaticMesh(CubeMesh);
	// The engine cube is 100 cm on a side, centered on its pivot.
	C->SetRelativeLocation((Min + Max) * 0.5);
	C->SetRelativeScale3D((Max - Min).GetAbs() / 100.0);
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

UStaticMeshComponent* AShopStage::Solid(UMaterialInterface* Material, const FVector& Min, const FVector& Max, bool bShow)
{
	UStaticMeshComponent* C = Block(Material, Min, Max, true);
	if (!bShow)
	{
		// The art draws it; this only stops whoever walks into it.
		C->SetVisibility(false);
		C->SetCastShadow(false);
	}
	return C;
}

UStaticMeshComponent* AShopStage::Site(const TCHAR* Name)
{
	UStaticMesh* Asset = ShopMesh(Name);
	if (!Asset)
	{
		return nullptr;
	}
	UStaticMeshComponent* C = NewPart<UStaticMeshComponent>();
	C->SetStaticMesh(Asset);
	C->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DressSlots(C);
	return C;
}

UStaticMeshComponent* AShopStage::Mesh(const TCHAR* Name, const FVector& At, float Yaw, bool bCollide)
{
	UStaticMesh* Asset = ShopMesh(Name);
	if (!Asset)
	{
		return nullptr;
	}
	UStaticMeshComponent* C = NewPart<UStaticMeshComponent>();
	C->SetStaticMesh(Asset);
	C->SetRelativeLocationAndRotation(At, FRotator(0.0f, Yaw + 180.0f, 0.0f));
	if (bCollide)
	{
		C->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	}
	else
	{
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	DressSlots(C);
	return C;
}

FVector AShopStage::PropPoint(const FVector& At, float Yaw, const FVector& BlenderMeters)
{
	// The importer keeps Blender's x and z and mirrors its y (spec 0.4's PROP frame); the prop stands turned Yaw + 180.
	const FVector Local(BlenderMeters.X * 100.0, -BlenderMeters.Y * 100.0, BlenderMeters.Z * 100.0);
	return At + FRotator(0.0f, Yaw + 180.0f, 0.0f).RotateVector(Local);
}

UMaterialInterface* AShopStage::RuntimeMaterialFor(const FString& Slot)
{
	if (Slot.Contains(TEXT("ns_mirror")))
	{
		return MirrorMaterial;
	}
	if (Slot.Contains(TEXT("ns_barberpole")))
	{
		return PoleMaterial;
	}
	if (Slot.Contains(TEXT("ns_belt")))
	{
		return BeltMaterial;
	}
	if (Slot.Contains(TEXT("ns_glass")))
	{
		// Only ue-world's shop glass (M_NS_Glass): without it the imported glass stays.
		return ShopGlassMaterial;
	}
	return nullptr;
}

void AShopStage::DressSlots(UStaticMeshComponent* C)
{
	const UStaticMesh* Source = C ? C->GetStaticMesh().Get() : nullptr;
	if (!Source)
	{
		return;
	}
	const TArray<FStaticMaterial>& Slots = Source->GetStaticMaterials();
	for (int32 I = 0; I < Slots.Num(); ++I)
	{
		UMaterialInterface* Baked = Source->GetMaterial(I);
		// The slot's name as Blender wrote it (spec 0.5: the unbaked slots keep their names), and its material's.
		FString Name = Slots[I].MaterialSlotName.ToString().ToLower();
		if (Baked)
		{
			Name += TEXT("|") + Baked->GetName().ToLower();
		}
		UMaterialInterface* Runtime = RuntimeMaterialFor(Name);
		if (!Runtime || Runtime == Baked)
		{
			continue;
		}
		if (Runtime == ShopGlassMaterial)
		{
			C->SetMaterial(I, Runtime);
			continue;
		}
		// Its own copy (made at build, never saved), the art's texture carried over: the mirror's desilvering mask, the belt's tread.
		UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Runtime, C);
		Mid->SetFlags(RF_Transient);
		if (UTexture* Tex = BaseTextureOf(Baked))
		{
			Mid->SetTextureParameterValue(Runtime == MirrorMaterial ? TEXT("EdgeMask") : TEXT("BaseColorTexture"), Tex);
		}
		C->SetMaterial(I, Mid);
		if (Runtime == MirrorMaterial)
		{
			MirrorMids.Add(Mid);
		}
		else if (Runtime == PoleMaterial)
		{
			PoleMids.Add(Mid);
		}
		else if (Runtime == BeltMaterial)
		{
			BeltMids.Add(Mid);
		}
	}
}

UMaterialInterface* AShopStage::ClothMaterial(UMaterialInterface* Baked)
{
	if (!ClothTintMaterial)
	{
		return nullptr;
	}
	const FName Key(*FString::Printf(TEXT("Cloth_%s"), Baked ? *Baked->GetName() : TEXT("Plain")));
	if (TObjectPtr<UMaterialInstanceDynamic>* Found = Materials.Find(Key))
	{
		return *Found;
	}
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(ClothTintMaterial, this);
	if (UTexture* Tex = BaseTextureOf(Baked))
	{
		M->SetTextureParameterValue(TEXT("BaseColorTexture"), Tex);
	}
	Materials.Add(Key, M);
	return M;
}

ULightComponent* AShopStage::AddLight(ULightComponent* L, float Candela, uint32 SrgbHexColor, EShopLight LightRole)
{
	if (ULocalLightComponent* Local = Cast<ULocalLightComponent>(L))
	{
		Local->SetIntensityUnits(ELightUnits::Candelas);
	}
	L->SetIntensity(Candela);
	L->SetLightColor(SrgbHex(SrgbHexColor));
	// Interior fixtures shadow (MegaLights, the room's volume) and stay out of the fog; the window's spill and the signs
	// light the street's wet air a little and never shadow (spec 3.1).
	const bool bInside = LightRole == EShopLight::Fixture || LightRole == EShopLight::Night;
	L->SetCastShadows(LightRole == EShopLight::Fixture);
	L->SetVolumetricScatteringIntensity(bInside ? 0.0f : LightRole == EShopLight::Spill ? 0.25f : 0.4f);
	if (!bInside)
	{
		// Out on the street they drop out by distance (spec 3.1's budget).
		L->MaxDrawDistance = 4000.0f;
		L->MaxDistanceFadeRange = 600.0f;
		L->MarkRenderStateDirty();
	}
	FLight& Entry = Lights.AddDefaulted_GetRef();
	Entry.Light = L;
	Entry.Base = Candela;
	Entry.Role = LightRole;
	return L;
}

UPointLightComponent* AShopStage::PointLight(const FVector& At, float Candela, uint32 SrgbHexColor, float Radius, EShopLight LightRole)
{
	UPointLightComponent* L = NewPart<UPointLightComponent>();
	L->SetRelativeLocation(At);
	L->SetAttenuationRadius(Radius);
	AddLight(L, Candela, SrgbHexColor, LightRole);
	return L;
}

USpotLightComponent* AShopStage::SpotLight(const FVector& At, const FRotator& Aim, float Candela, uint32 SrgbHexColor, float Radius, float InnerDeg, float OuterDeg, EShopLight LightRole)
{
	USpotLightComponent* L = NewPart<USpotLightComponent>();
	L->SetRelativeLocationAndRotation(At, Aim);
	L->SetAttenuationRadius(Radius);
	L->SetInnerConeAngle(InnerDeg);
	L->SetOuterConeAngle(OuterDeg);
	AddLight(L, Candela, SrgbHexColor, LightRole);
	return L;
}

URectLightComponent* AShopStage::RectLight(const FVector& At, const FRotator& Aim, float Width, float Height, float Candela, uint32 SrgbHexColor, float Radius, EShopLight LightRole)
{
	URectLightComponent* L = NewPart<URectLightComponent>();
	L->SetRelativeLocationAndRotation(At, Aim);
	L->SetSourceWidth(Width);
	L->SetSourceHeight(Height);
	L->SetAttenuationRadius(Radius);
	AddLight(L, Candela, SrgbHexColor, LightRole);
	return L;
}

void AShopStage::Glow(UMaterialInstanceDynamic* Mid, EShopLight LightRole)
{
	if (!Mid)
	{
		return;
	}
	float Emissive = 0.0f;
	Mid->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Emissive")), Emissive);
	FGlow& G = Glows.AddDefaulted_GetRef();
	G.Mid = Mid;
	G.Role = LightRole;
	G.Strengths.Emplace(TEXT("Emissive"), Emissive);
}

void AShopStage::Glow(UStaticMeshComponent* Imported, EShopLight LightRole, uint32 TintSrgb, float Level)
{
	const UStaticMesh* Source = Imported ? Imported->GetStaticMesh().Get() : nullptr;
	if (!Source)
	{
		return;
	}
	Imported->SetCastShadow(false);
	const FLinearColor Tint = SrgbHex(TintSrgb) * Level;
	TArray<TPair<FName, FLinearColor>> Colors;
	TArray<TPair<FName, float>> Strengths;
	for (int32 Slot = 0; Slot < Source->GetStaticMaterials().Num(); ++Slot)
	{
		UMaterialInterface* Baked = Source->GetMaterial(Slot);
		if (!GlowParams(Baked, Colors, Strengths))
		{
			continue;
		}
		// Its own copy to scale, made at build and never saved (as the street's dimmed fronts); the family's color and level
		// on the importer's white (spec 0.5: the game sets them).
		UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Baked, Imported);
		Mid->SetFlags(RF_Transient);
		Imported->SetMaterial(Slot, Mid);
		FGlow& G = Glows.AddDefaulted_GetRef();
		G.Mid = Mid;
		G.Mesh = Imported;
		G.Role = LightRole;
		for (const TPair<FName, FLinearColor>& C : Colors)
		{
			G.Colors.Emplace(C.Key, FLinearColor(C.Value.R * Tint.R, C.Value.G * Tint.G, C.Value.B * Tint.B, C.Value.A));
		}
		for (const TPair<FName, float>& S : Strengths)
		{
			G.Strengths.Emplace(S.Key, S.Value * Level);
		}
	}
}

UWidgetComponent* AShopStage::Screen(const FVector& At, float Yaw, const FVector2D& SizeCm, const FIntPoint& Pixels, float Shine, TFunction<void(ss::ui::Canvas&)> Paint)
{
	UWidgetComponent* Wc = NewPart<UWidgetComponent>();
	UMaterialInterface* Lit = Optional(WidgetLitPath);
	// Printed (a menu, a price card) is lit like the wall it hangs on; a screen or a lightbox glows past white.
	const bool bPrinted = Shine <= 0.0f && Lit;
	Wc->SetWidgetSpace(EWidgetSpace::World);
	Wc->SetDrawSize(FVector2D(Pixels.X, Pixels.Y));
	Wc->SetPivot(FVector2D(0.5, 0.5));
	Wc->SetBlendMode(bPrinted ? EWidgetBlendMode::Opaque : EWidgetBlendMode::Transparent);
	Wc->SetBackgroundColor(bPrinted ? FLinearColor(0.02f, 0.02f, 0.02f, 1.0f) : FLinearColor::Transparent);
	Wc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Wc->SetCastShadow(false);
	Wc->SetRelativeLocationAndRotation(At, FRotator(0.0f, Yaw, 0.0f));
	// The widget's quad lies in its local Y (width) and Z (height), one unit per pixel.
	Wc->SetRelativeScale3D(FVector(1.0, SizeCm.X / FMath::Max(1, Pixels.X), SizeCm.Y / FMath::Max(1, Pixels.Y)));
	if (bPrinted)
	{
		Wc->SetMaterial(0, Lit);
	}
	const float Tint = bPrinted ? 1.0f : FMath::Max(Shine, 0.01f);
	Wc->SetTintColorAndOpacity(FLinearColor(Tint, Tint, Tint, 1.0f));
	Screens.Add(Wc);
	ScreenSlates.Add(nullptr);
	Redraw(Screens.Num() - 1, MoveTemp(Paint));
	return Wc;
}

UWidgetComponent* AShopStage::ScreenAt(int32 Index) const
{
	return Screens.IsValidIndex(Index) ? Screens[Index].Get() : nullptr;
}

void AShopStage::Redraw(int32 Index, TFunction<void(ss::ui::Canvas&)> Paint)
{
	UWidgetComponent* Wc = Screens.IsValidIndex(Index) ? Screens[Index].Get() : nullptr;
	if (!Wc || !Paint || !FSlateTextMeasurer::IsAvailable())
	{
		return;
	}
	const FVector2D Draw = Wc->GetDrawSize();
	const FIntPoint Pixels(FMath::RoundToInt32(Draw.X), FMath::RoundToInt32(Draw.Y));
	FSlateTextMeasurer Measurer;
	TSharedPtr<ss::ui::DrawList> List = MakeShared<ss::ui::DrawList>();
	ss::ui::Canvas C(*List, Measurer, static_cast<float>(Pixels.X), static_cast<float>(Pixels.Y), 1.0f);
	Paint(C);
	TSharedPtr<SDrawListWidget> Slate = ScreenSlates[Index];
	if (!Slate)
	{
		Slate = SNew(SDrawListWidget).DesiredSize(FVector2D(Pixels.X, Pixels.Y));
		ScreenSlates[Index] = Slate;
		Wc->SetSlateWidget(Slate);
		Wc->SetManuallyRedraw(true);
	}
	Slate->SetDrawList(List);
	Wc->RequestRedraw();
}

int32 AShopStage::Use(FName Id, EStreetVerb Verb, const FVector& At, float Radius, const FString& Prompt, const FString& Param, TFunction<FString()> WhyNot)
{
	UStreetWorld* R = Registry();
	if (!R)
	{
		return INDEX_NONE;
	}
	FStreetUse U;
	U.Id = Id;
	U.Verb = Verb;
	U.Venue = Venue;
	U.Param = Param;
	U.Prompt = Prompt;
	U.Radius = Radius;
	U.Anchor = Root.Get();
	U.Offset = At;
	if (WhyNot)
	{
		U.WhyNot = MoveTemp(WhyNot);
	}
	else
	{
		TWeakObjectPtr<AShopStage> Self(this);
		U.WhyNot = [Self]() { return Self.IsValid() && !Self->bOpen ? Self->ClosedPrompt() : FString(); };
	}
	const int32 Handle = R->AddUse(U);
	UseHandles.Add(Handle);
	return Handle;
}

void AShopStage::Station(FName Id, const FVector& At, float Yaw, FName Pose, float SeatHeight)
{
	if (UStreetWorld* R = Registry())
	{
		R->AddStation(Id, Venue, FTransform(FRotator(0.0f, Yaw, 0.0f), At) * GetActorTransform(), Pose, SeatHeight, true);
	}
}

void AShopStage::Seat(FName Id, const FVector& Feet, float Yaw, float Height, const FVector& Stand, FName Pose, bool bStation, FName SeatShot)
{
	FSeat& S = Seats.Add(Id);
	S.Feet = FTransform(FRotator(0.0f, Yaw, 0.0f), Feet);
	S.Height = Height;
	S.Stand = Stand;
	S.Shot = SeatShot;
	if (bStation)
	{
		Station(Id, Feet, Yaw, Pose, Height);
	}
}

void AShopStage::Porch(const FBox& Sidewalk, const FString& Place)
{
	// The sidewalk outside the door (the street's own lies over it, a hair higher, once its stage builds it) and a zone over
	// it, so whoever steps out of the door is still on the set.
	Solid(Mat(TEXT("Sidewalk"), 0x6f6c66, 0.75f, 3.0f), FVector(Sidewalk.Min.X, Sidewalk.Min.Y, -30.0), FVector(Sidewalk.Max.X, Sidewalk.Max.Y, -0.5), true);
	OutsideZone(TEXT("porch"), Sidewalk, Place);
}

void AShopStage::Entry(const FBox& Box, const FString& Place)
{
	OutsideZone(TEXT("entry"), Box, Place);
}

void AShopStage::OutsideZone(const TCHAR* Suffix, const FBox& Box, const FString& Place)
{
	UStreetWorld* R = Registry();
	if (!R)
	{
		return;
	}
	// Below the street's ones in priority (where they overlap theirs counts), a column over the ground.
	FStreetZone Z;
	Z.Id = FName(*FString::Printf(TEXT("%s-%s"), *Venue.ToString(), Suffix));
	Z.Box = FBox(FVector(Box.Min.X, Box.Min.Y, -200.0), FVector(Box.Max.X, Box.Max.Y, 1000.0)).TransformBy(GetActorTransform());
	Z.Place = Place;
	Z.Priority = 2;
	R->AddZone(Z);
	PorchZones.AddUnique(Z.Id);
}

bool AShopStage::SeatFor(FName Id, FTransform& OutFeet, float& OutHeight, FTransform& OutStand, FName* OutShot) const
{
	const FSeat* S = Seats.Find(Id);
	if (!S)
	{
		return false;
	}
	const FTransform& T = GetActorTransform();
	OutFeet = S->Feet * T;
	OutHeight = S->Height;
	// Up off the seat facing the way they sat, a meter over the floor (the capsule settles).
	OutStand = FTransform(S->Feet.GetRotation(), S->Stand + FVector(0.0, 0.0, 100.0)) * T;
	if (OutShot)
	{
		*OutShot = S->Shot;
	}
	return true;
}

void AShopStage::Shot(FName Id, const FVector& Camera, const FVector& LookAt, float Fov)
{
	if (UStreetWorld* R = Registry())
	{
		R->AddShot(Id, FTransform((LookAt - Camera).Rotation(), Camera) * GetActorTransform(), Fov);
		ShotIds.AddUnique(Id);
	}
}

void AShopStage::ShotMarkAt(FName InShot, const FVector& At, float Yaw)
{
	ShotMarks.Add(InShot, FTransform(FRotator(0.0f, Yaw, 0.0f), At + FVector(0.0, 0.0, 100.0)));
}

bool AShopStage::ShotMark(FName InShot, FTransform& Out) const
{
	const FTransform* T = ShotMarks.Find(InShot);
	if (!T)
	{
		return false;
	}
	Out = *T * GetActorTransform();
	return true;
}

void AShopStage::Start(FName Id, const FVector& At, float Yaw)
{
	if (UStreetWorld* R = Registry())
	{
		// As the street's starts: a meter over the floor (the pawn's capsule settles onto it).
		R->AddStart(Id, FTransform(FRotator(0.0f, Yaw, 0.0f), At + FVector(0.0, 0.0, 100.0)) * GetActorTransform());
		StartIds.AddUnique(Id);
	}
}

void AShopStage::Zone(const FBox& Room, const FString& Place, int32 RoomTone, float BoomLength, float Fov, bool bFirstPerson)
{
	RoomBox = Room;
	PlaceName = Place;
	// The room's own exposure (spec 3.2), MegaLights for its fixtures, inside its box only (a short blend at the door).
	RoomVolume = NewPart<UBoxComponent>();
	RoomVolume->SetRelativeLocation(Room.GetCenter());
	RoomVolume->SetBoxExtent(Room.GetExtent());
	RoomVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RoomVolume->SetGenerateOverlapEvents(false);
	RoomVolume->SetHiddenInGame(true);
	RoomLens = NewPart<UPostProcessComponent>(RoomVolume);
	RoomLens->bUnbound = false;
	RoomLens->BlendRadius = 60.0f;
	RoomLens->Priority = 5.0f;
	FPostProcessSettings& S = RoomLens->Settings;
	S.bOverride_bMegaLights = true;
	S.bMegaLights = 1;
	S.bOverride_AutoExposureMinBrightness = true;
	S.bOverride_AutoExposureMaxBrightness = true;
	ApplyExposure();
	UStreetWorld* R = Registry();
	if (!R)
	{
		return;
	}
	FStreetZone Z;
	Z.Id = Venue;
	Z.Box = Room.TransformBy(GetActorTransform());
	Z.Place = Place;
	Z.RoomTone = RoomTone;
	Z.bDry = true;
	Z.bInterior = true;
	Z.Priority = 10;
	Z.BoomLength = BoomLength;
	Z.Fov = Fov;
	Z.bFirstPerson = bFirstPerson;
	TWeakObjectPtr<AShopStage> Self(this);
	Z.DoorOpen = [Self]() { return Self.IsValid() ? Self->DoorAmount : 0.0f; };
	R->AddZone(Z);
}

void AShopStage::Storefront(const FVector& Middle, const FVector& Out)
{
	GlassAt = Middle;
	GlassOut = Out.GetSafeNormal();
	bHasGlass = true;
}

FVector AShopStage::DoorwayMiddle() const
{
	if (DoorSpec.bPair)
	{
		return (DoorSpec.Hinge + DoorSpec.Hinge2) * 0.5;
	}
	return DoorSpec.Hinge + Along(DoorSpec.ShutYaw) * (DoorSpec.Width * 0.5);
}

double AShopStage::DoorwaySpan() const
{
	return DoorSpec.bPair ? FVector::Dist2D(DoorSpec.Hinge, DoorSpec.Hinge2) : static_cast<double>(DoorSpec.Width);
}

void AShopStage::Door(const FShopDoorSpec& Spec, UStaticMeshComponent* Leaf, UStaticMeshComponent* Leaf2)
{
	DoorSpec = Spec;
	bHasDoor = true;
	const FVector In = Along(Spec.InYaw);
	const FVector Middle = DoorwayMiddle();
	// A leaf turns about its hinge: X along it from the hinge to the latch, Y its thickness, Z up.
	auto Hang = [this, &Spec](const FVector& Hinge, float ShutYaw, UStaticMeshComponent* Art) {
		USceneComponent* Pivot = NewPart<USceneComponent>();
		Pivot->SetRelativeLocationAndRotation(Hinge, FRotator(0.0f, ShutYaw, 0.0f));
		if (Art)
		{
			Art->AttachToComponent(Pivot, FAttachmentTransformRules::KeepWorldTransform);
			return Pivot;
		}
		// The block-out: a frame round glass (the gym's, the pawn's) or a painted leaf with a glass light (the barber's).
		UMaterialInterface* Frame = Mat(TEXT("DoorFrame"), Spec.bGlass ? 0x1c1e21 : 0x3b4a3f, 0.45f, 0.0f, 0.0f, Spec.bGlass ? 0.6f : 0.0f);
		UMaterialInterface* Pane = GlassMat();
		const double W = Spec.Width;
		const double H = Spec.Height;
		const double Rail = Spec.bGlass ? 9.0 : 14.0;
		Block(Frame, FVector(0.0, -2.5, 0.0), FVector(Rail, 2.5, H), false, Pivot);
		Block(Frame, FVector(W - Rail, -2.5, 0.0), FVector(W, 2.5, H), false, Pivot);
		Block(Frame, FVector(Rail, -2.5, H - Rail), FVector(W - Rail, 2.5, H), false, Pivot);
		Block(Frame, FVector(Rail, -2.5, 0.0), FVector(W - Rail, 2.5, Spec.bGlass ? 22.0 : H * 0.45), false, Pivot);
		Block(Pane, FVector(Rail, -0.6, Spec.bGlass ? 22.0 : H * 0.45), FVector(W - Rail, 0.6, H - Rail), false, Pivot)->SetCastShadow(false);
		// A push bar across it at the waist.
		Block(Mat(TEXT("DoorBar"), 0x9aa0a6, 0.3f, 0.0f, 0.0f, 1.0f), FVector(Rail, -4.5, 98.0), FVector(W - Rail, -2.5, 104.0), false, Pivot);
		return Pivot;
	};
	DoorLeaf = Hang(Spec.Hinge, Spec.ShutYaw, Leaf);
	DoorLeaf2 = Spec.bPair ? Hang(Spec.Hinge2, Spec.ShutYaw2, Leaf2) : nullptr;
	// The stop: blocks the doorway while the door's near shut (the leaves have no collision of their own, so they never shove
	// anyone), across the whole doorway.
	USceneComponent* StopFrame = NewPart<USceneComponent>();
	StopFrame->SetRelativeLocationAndRotation(Spec.Hinge, FRotator(0.0f, Spec.ShutYaw, 0.0f));
	DoorStop = Block(nullptr, FVector(-5.0, -5.0, 0.0), FVector(DoorwaySpan() + 5.0, 5.0, Spec.Height), true, StopFrame);
	DoorStop->SetHiddenInGame(true);
	DoorStop->SetVisibility(false);
	UStreetWorld* R = Registry();
	if (!R)
	{
		return;
	}
	// E at it (from either side), the crowd's door (outside on the sidewalk facing in), and the start just inside.
	TWeakObjectPtr<AShopStage> Self(this);
	Use(FName(*FString::Printf(TEXT("%s-door"), *Venue.ToString())), EStreetVerb::Door, Middle + FVector(0.0, 0.0, 100.0), 120.0f, DoorPrompt(), FString(), [Self]() {
		if (!Self.IsValid() || Self->bSomeoneInside)
		{
			return FString();
		}
		const bool bLocked = Self->DoorSpec.Kind == EShopDoorKind::Fob ? !Self->bMember && !(Self->bOpen && Self->bStaffed) : !Self->bOpen;
		return bLocked ? Self->ClosedPrompt() : FString();
	});
	FStreetDoor D;
	D.Id = Venue;
	D.Outside = FTransform(FRotator(0.0f, Spec.InYaw, 0.0f), Middle - In * 130.0) * GetActorTransform();
	D.Threshold = GetActorTransform().TransformPosition(Middle);
	D.bRoom = true;
	R->AddDoor(D);
	Start(Venue, Middle + In * 150.0, Spec.InYaw);
}

void AShopStage::Grille(const FVector& Min, const FVector& Max, UStaticMeshComponent* Art)
{
	GrilleHeight = FMath::Abs(Max.Z - Min.Z);
	if (Art)
	{
		// The art's curtain hangs from its hood and the game scales it in Z (1 down, rolled into the hood).
		GrilleArt = Art;
		return;
	}
	GrilleRoot = NewPart<USceneComponent>();
	Block(Mat(TEXT("Grille"), 0x6b6e70, 0.5f, PatShutter, 0.0f, 0.4f), Min, Max, false, GrilleRoot);
}

void AShopStage::Spinner(USceneComponent* Part, const FRotator& PerSecond, bool bOpenOnly)
{
	if (!Part)
	{
		return;
	}
	FSpin& S = Spins.AddDefaulted_GetRef();
	S.Part = Part;
	S.Base = Part->GetRelativeRotation();
	S.PerSecond = PerSecond;
	S.bOpenOnly = bOpenOnly;
}

void AShopStage::ClockHands(const FVector& Center, float FaceYaw, float Radius)
{
	// A pivot on the dial's face, +X out of it; each hand an arm turned in the dial's plane, its bar along the arm's +Z.
	USceneComponent* Dial = NewPart<USceneComponent>();
	Dial->SetRelativeLocationAndRotation(Center, FRotator(0.0f, FaceYaw, 0.0f));
	UMaterialInterface* Ink = Mat(TEXT("ClockHand"), 0x111111, 0.4f);
	auto Hand = [&](float Length, float Width, float Out) {
		USceneComponent* Arm = NewPart<USceneComponent>(Dial);
		Arm->SetRelativeLocation(FVector(Out, 0.0, 0.0));
		Block(Ink, FVector(-0.15, -Width * 0.5, -Length * 0.15), FVector(0.15, Width * 0.5, Length), false, Arm)->SetCastShadow(false);
		return Arm;
	};
	FClock& C = Clocks.AddDefaulted_GetRef();
	C.Hour = Hand(Radius * 0.55f, FMath::Max(0.6f, Radius * 0.07f), 0.4f);
	C.Minute = Hand(Radius * 0.85f, FMath::Max(0.4f, Radius * 0.045f), 0.8f);
	C.FaceYaw = FaceYaw;
	// A cap over the arbor.
	Block(Ink, FVector(0.6, -Radius * 0.05, -Radius * 0.05), FVector(1.0, Radius * 0.05, Radius * 0.05), false, Dial)->SetCastShadow(false);
}

void AShopStage::Mirror(const FVector& PlanePoint, const FVector& Normal, const FVector2D& Size, bool bBlockOut)
{
	FMirrorPlane& P = MirrorPlanes.AddDefaulted_GetRef();
	P.Point = PlanePoint;
	P.Normal = Normal.GetSafeNormal();
	if (!bBlockOut)
	{
		return;
	}
	// A plain panel facing the room in the shops' mirror material (or a polished steel stand-in without it).
	const FRotator Facing = P.Normal.Rotation();
	USceneComponent* Frame = NewPart<USceneComponent>();
	Frame->SetRelativeLocationAndRotation(PlanePoint, Facing);
	UMaterialInterface* Silver = MirrorMaterial ? static_cast<UMaterialInterface*>(UMaterialInstanceDynamic::Create(MirrorMaterial, this))
												: static_cast<UMaterialInterface*>(Mat(TEXT("MirrorSteel"), 0xd8dadc, 0.03f, 0.0f, 0.0f, 1.0f));
	if (UMaterialInstanceDynamic* Mid = Cast<UMaterialInstanceDynamic>(Silver); Mid && MirrorMaterial)
	{
		MirrorMids.Add(Mid);
	}
	Block(Silver, FVector(-0.6, -Size.X * 0.5, -Size.Y * 0.5), FVector(0.0, Size.X * 0.5, Size.Y * 0.5), false, Frame)->SetCastShadow(false);
}

void AShopStage::Carve(const FBox& Hole)
{
	CarveHoles.Add(Hole);
}

void AShopStage::ApplyCarves()
{
	UWorld* InWorld = GetWorld();
	if (!InWorld || !InWorld->IsGameWorld() || CarveHoles.Num() == 0 || !CubeMesh)
	{
		return;
	}
	if (const UStreetWorld* R = Registry())
	{
		CarvedRevision = R->ZoneRevision();
	}
	// Anything carved whose original is gone (the street built again) goes with it; the street's own build replaced it.
	for (int32 I = Carved.Num() - 1; I >= 0; --I)
	{
		if (!Carved[I].Original.IsValid())
		{
			for (const TWeakObjectPtr<UStaticMeshComponent>& Piece : Carved[I].Pieces)
			{
				DropPart(Piece.Get());
			}
			Carved.RemoveAt(I);
		}
		else if (UStaticMeshComponent* O = Carved[I].Original.Get(); O->IsVisible() || O->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
		{
			// Something showed it again (its stage's hour): hidden again, its pieces stand for it.
			O->SetVisibility(false);
			O->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	TSet<const UStaticMeshComponent*> Done;
	for (const FCarved& C : Carved)
	{
		Done.Add(C.Original.Get());
	}
	const FTransform& Mine = GetActorTransform();
	int32 Count = 0;
	for (TActorIterator<AActor> It(InWorld); It; ++It)
	{
		AActor* Other = *It;
		if (!Other || Other == this || Other->IsA<AShopStage>() || Other->IsA<APawn>())
		{
			continue;
		}
		TInlineComponentArray<UStaticMeshComponent*> Parts(Other);
		for (UStaticMeshComponent* Part : Parts)
		{
			if (!Part || Part->GetStaticMesh() != CubeMesh || Done.Contains(Part) || !Part->IsRegistered())
			{
				continue;
			}
			if (!Part->IsVisible() && Part->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
			{
				continue;
			}
			// In this shop's frame (the street stage's): a cube turned by a quarter turn is still a box there.
			const FTransform Rel = Part->GetComponentTransform().GetRelativeTransform(Mine);
			if (!IsSquare(Rel.GetRotation()))
			{
				continue;
			}
			const FBox Local = FBox(FVector(-50.0), FVector(50.0)).TransformBy(Rel);
			TArray<FBox> Pieces = {Local};
			bool bHit = false;
			for (const FBox& Hole : CarveHoles)
			{
				TArray<FBox> Next;
				for (const FBox& P : Pieces)
				{
					const int32 Before = Next.Num();
					Subtract(P, Hole, Next);
					bHit = bHit || Next.Num() != Before + 1 || !Next.Last().Equals(P);
				}
				Pieces = MoveTemp(Next);
			}
			if (!bHit)
			{
				continue;
			}
			FCarved& Entry = Carved.AddDefaulted_GetRef();
			Entry.Original = Part;
			Entry.bWasVisible = Part->IsVisible();
			Entry.WasCollision = Part->GetCollisionEnabled();
			for (const FBox& P : Pieces)
			{
				UStaticMeshComponent* Piece = NewPart<UStaticMeshComponent>();
				Piece->SetStaticMesh(CubeMesh);
				Piece->SetRelativeLocation(P.GetCenter());
				Piece->SetRelativeScale3D(P.GetSize() / 100.0);
				for (int32 M = 0; M < Part->GetNumMaterials(); ++M)
				{
					Piece->SetMaterial(M, Part->GetMaterial(M));
				}
				Piece->SetCollisionProfileName(Part->GetCollisionProfileName());
				Piece->SetCollisionEnabled(Entry.WasCollision);
				Piece->SetCastShadow(Part->CastShadow);
				Piece->SetVisibility(Entry.bWasVisible);
				Piece->SetHiddenInGame(Part->bHiddenInGame);
				Entry.Pieces.Add(Piece);
			}
			Part->SetVisibility(false);
			Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			++Count;
		}
	}
	if (Count > 0)
	{
		UE_LOG(LogShopStage, Log, TEXT("%s: carved %d of the street's blocks out of its room"), *VenueName, Count);
	}
}

FString AShopStage::DoorPrompt() const
{
	switch (DoorSpec.Kind)
	{
	case EShopDoorKind::Fob:
		return TEXT("Tap your fob");
	case EShopDoorKind::Buzzer:
		return TEXT("Buzz");
	default:
		return TEXT("Open the door");
	}
}

FString AShopStage::DoorPromptNow() const
{
	if (!bHasDoor || DoorAmount > 0.5f)
	{
		return FString();
	}
	if (bSomeoneInside)
	{
		return TEXT("Open the door");
	}
	switch (DoorSpec.Kind)
	{
	case EShopDoorKind::Fob:
		// Members tap the fob at any hour; anyone else is buzzed in while someone's at the desk (spec 5.0).
		return bMember ? FString(TEXT("Tap your fob")) : bOpen && bStaffed ? FString(TEXT("Buzz in")) : ClosedPrompt();
	case EShopDoorKind::Buzzer:
		return BuzzIn >= 0.0f ? FString(TEXT("Buzzing...")) : bOpen ? FString(TEXT("Buzz")) : ClosedPrompt();
	default:
		return bOpen ? FString(TEXT("Open the door")) : ClosedPrompt();
	}
}

FString AShopStage::ClosedPrompt() const
{
	return ClosedLine.IsEmpty() ? FString::Printf(TEXT("%s · Closed"), *VenueName) : ClosedLine;
}

// ------------------------------------------------------------------ painters

void AShopStage::PaintCard(ss::ui::Canvas& C, const FString& Heading, const TArray<FString>& Lines, uint32 Paper, uint32 Ink, bool bTransparent)
{
	const float W = C.Width();
	const float H = C.Height();
	if (!bTransparent)
	{
		C.FillRoundRect({0.0f, 0.0f, W, H}, H * 0.04f, ss::ui::Paint(ss::ui::Hex(Paper)));
		C.StrokeRoundRect({W * 0.03f, H * 0.04f, W * 0.94f, H * 0.92f}, H * 0.03f, ss::ui::Hex(Ink, 0.55f), FMath::Max(1.0f, H * 0.008f));
	}
	const int32 Rows = Lines.Num() + (Heading.IsEmpty() ? 0 : 1);
	const float Step = H / static_cast<float>(FMath::Max(Rows, 1) + 1);
	float Y = Step;
	if (!Heading.IsEmpty())
	{
		ss::ui::TextStyle T;
		T.Size = FMath::Min(Step * 0.72f, W * 0.11f);
		T.Weight = 900;
		T.Col = ss::ui::Hex(Ink);
		T.HAlign = ss::ui::Align::Center;
		T.VAlign = ss::ui::Baseline::Middle;
		T.MaxWidth = W * 0.9f;
		C.Text(Utf8(Heading), W * 0.5f, Y, T);
		Y += Step;
	}
	for (const FString& Line : Lines)
	{
		ss::ui::TextStyle T;
		T.Size = FMath::Min(Step * 0.5f, W * 0.075f);
		T.Weight = 600;
		T.Col = ss::ui::Hex(Ink, 0.92f);
		T.HAlign = ss::ui::Align::Center;
		T.VAlign = ss::ui::Baseline::Middle;
		T.MaxWidth = W * 0.9f;
		C.Text(Utf8(Line), W * 0.5f, Y, T);
		Y += Step;
	}
}

void AShopStage::PaintBoard(ss::ui::Canvas& C, const FString& Heading, const TArray<TPair<FString, FString>>& Rows, uint32 Board, uint32 Ink)
{
	const float W = C.Width();
	const float H = C.Height();
	C.FillRect({0.0f, 0.0f, W, H}, ss::ui::Paint(ss::ui::Hex(Board)));
	// A letter board's grooves.
	const float Groove = H / 48.0f;
	for (float Y = Groove; Y < H; Y += Groove)
	{
		C.FillRect({0.0f, Y, W, FMath::Max(1.0f, Groove * 0.12f)}, ss::ui::Paint(ss::ui::Hex(0x000000, 0.18f)));
	}
	const int32 Count = FMath::Max(1, Rows.Num());
	const float Head = Heading.IsEmpty() ? 0.0f : H * 0.16f;
	const float Step = FMath::Min((H - Head - H * 0.06f) / static_cast<float>(Count), H * 0.12f);
	if (!Heading.IsEmpty())
	{
		ss::ui::TextStyle T;
		T.Size = Head * 0.6f;
		T.Weight = 900;
		T.Col = ss::ui::Hex(Ink);
		T.HAlign = ss::ui::Align::Center;
		T.VAlign = ss::ui::Baseline::Middle;
		T.MaxWidth = W * 0.92f;
		C.Text(Utf8(Heading), W * 0.5f, Head * 0.55f, T);
	}
	float Y = Head + Step * 0.6f;
	for (const TPair<FString, FString>& Row : Rows)
	{
		ss::ui::TextStyle L;
		L.Size = Step * 0.56f;
		L.Weight = 700;
		L.Mono = true;
		L.Col = ss::ui::Hex(Ink);
		L.VAlign = ss::ui::Baseline::Middle;
		L.MaxWidth = W * 0.66f;
		C.Text(Utf8(Row.Key.ToUpper()), W * 0.05f, Y, L);
		ss::ui::TextStyle R = L;
		R.HAlign = ss::ui::Align::Right;
		R.MaxWidth = W * 0.26f;
		C.Text(Utf8(Row.Value), W * 0.95f, Y, R);
		Y += Step;
	}
}

void AShopStage::PaintTag(ss::ui::Canvas& C, const FString& Price, const FString& Under)
{
	const float W = C.Width();
	const float H = C.Height();
	// A manila tag with its corners clipped and a reinforced hole.
	const float Cut = W * 0.18f;
	C.FillPolygon({{Cut, 0.0f}, {W, 0.0f}, {W, H}, {Cut, H}, {0.0f, H * 0.62f}, {0.0f, H * 0.38f}}, ss::ui::Paint(ss::ui::Hex(0xe9d9a8)));
	C.FillCircle(Cut * 0.75f, H * 0.5f, H * 0.11f, ss::ui::Paint(ss::ui::Hex(0xc9b27a)));
	C.FillCircle(Cut * 0.75f, H * 0.5f, H * 0.06f, ss::ui::Paint(ss::ui::Hex(0x2a2520)));
	ss::ui::TextStyle T;
	T.Size = H * (Under.IsEmpty() ? 0.48f : 0.38f);
	T.Weight = 800;
	T.Col = ss::ui::Hex(0x1d1a16);
	T.HAlign = ss::ui::Align::Center;
	T.VAlign = ss::ui::Baseline::Middle;
	T.MaxWidth = (W - Cut) * 0.92f;
	C.Text(Utf8(Price), (W + Cut) * 0.5f, H * (Under.IsEmpty() ? 0.52f : 0.38f), T);
	if (!Under.IsEmpty())
	{
		ss::ui::TextStyle U = T;
		U.Size = H * 0.2f;
		U.Weight = 600;
		C.Text(Utf8(Under), (W + Cut) * 0.5f, H * 0.74f, U);
	}
}

void AShopStage::PaintOpenClosed(ss::ui::Canvas& C, bool bShopOpen)
{
	const float W = C.Width();
	const float H = C.Height();
	C.FillRoundRect({0.0f, 0.0f, W, H}, H * 0.08f, ss::ui::Paint(ss::ui::Hex(bShopOpen ? 0xf3efe4 : 0x1b1b1d)));
	C.StrokeRoundRect({W * 0.04f, H * 0.08f, W * 0.92f, H * 0.84f}, H * 0.06f, ss::ui::Hex(bShopOpen ? 0xb3261e : 0xe8e4da), FMath::Max(1.0f, H * 0.03f));
	ss::ui::TextStyle T;
	T.Size = H * 0.46f;
	T.Weight = 900;
	T.Col = ss::ui::Hex(bShopOpen ? 0xb3261e : 0xe8e4da);
	T.HAlign = ss::ui::Align::Center;
	T.VAlign = ss::ui::Baseline::Middle;
	T.MaxWidth = W * 0.86f;
	C.Text(bShopOpen ? "OPEN" : "CLOSED", W * 0.5f, H * 0.52f, T);
}

// ------------------------------------------------------------------ at play

void AShopStage::SetHour(double InWorldMinutes, float InDaylight, bool bInOpen, bool bInStaffed, const FString& InClosedLine)
{
	WorldMinutes = InWorldMinutes;
	Daylight = FMath::Clamp(InDaylight, 0.0f, 1.0f);
	if (!InClosedLine.IsEmpty())
	{
		ClosedLine = InClosedLine;
	}
	const bool bFirst = bBuilt && !bHourSet;
	const bool bChanged = bFirst || bInOpen != bOpen || bInStaffed != bStaffed;
	bOpen = bInOpen;
	bStaffed = bInStaffed;
	if (bFirst)
	{
		// The first hour it's told (play begins, it was just built): the grille as the hour has it, not rolling there.
		bHourSet = true;
		GrilleOpen = bOpen ? 1.0f : 0.0f;
		ApplyGrille();
	}
	if (bChanged && bBuilt)
	{
		ApplyLights();
		OnHour();
	}
	if (FMath::Abs(Daylight - AppliedDaylight) > 0.01f)
	{
		ApplyExposure();
	}
}

void AShopStage::SetAwake(bool bInAwake)
{
	if (bInAwake == bAwake)
	{
		return;
	}
	bAwake = bInAwake;
	if (bBuilt)
	{
		ApplyLights();
		OnAwake();
	}
}

void AShopStage::ApplyExposure()
{
	AppliedDaylight = Daylight;
	if (!RoomLens)
	{
		return;
	}
	FPostProcessSettings& S = RoomLens->Settings;
	S.AutoExposureMinBrightness = FMath::Lerp(NightEvMin, DayEvMin, Daylight);
	S.AutoExposureMaxBrightness = FMath::Lerp(NightEvMax, DayEvMax, Daylight);
}

int32 AShopStage::MirrorMode()
{
	return FMath::Clamp(CVarShopMirror.GetValueOnGameThread(), 0, 2);
}

void AShopStage::ApplyMirrorMode()
{
	const int32 Mode = MirrorMode();
	AppliedMirrorMode = Mode;
	if (RoomLens && MirrorPlanes.Num() > 0)
	{
		// The spike's first answer (spec 3.4): hit lighting for reflections in this shop's volume only, so the mirror shows
		// the people in it lit as they are; the capture fallback and the plain setting leave Lumen as the project has it.
		FPostProcessSettings& S = RoomLens->Settings;
		S.bOverride_LumenRayLightingMode = Mode == ShopMirror::Lumen;
		S.LumenRayLightingMode = Mode == ShopMirror::Lumen ? ELumenRayLightingModeOverride::HitLightingForReflections : ELumenRayLightingModeOverride::Default;
	}
	const bool bCapture = Mode == ShopMirror::Capture && bMirrorLive;
	for (UMaterialInstanceDynamic* Mid : MirrorMids)
	{
		if (Mid)
		{
			Mid->SetScalarParameterValue(TEXT("UseCapture"), bCapture && MirrorTarget ? 1.0f : 0.0f);
		}
	}
	if (MirrorCapture && !bCapture)
	{
		MirrorCapture->SetVisibility(false);
	}
}

void AShopStage::SetMirrorLive(bool bLive)
{
	if (bLive == bMirrorLive)
	{
		return;
	}
	bMirrorLive = bLive;
	MirrorCaptureIn = 0.0f;
	ApplyMirrorMode();
}

void AShopStage::UpdateMirrorCapture(float Dt)
{
	if (MirrorMode() != AppliedMirrorMode)
	{
		ApplyMirrorMode();
	}
	if (!bMirrorLive || MirrorMode() != ShopMirror::Capture || MirrorPlanes.Num() == 0 || MirrorMids.Num() == 0)
	{
		return;
	}
	MirrorCaptureIn -= Dt;
	if (MirrorCaptureIn > 0.0f)
	{
		return;
	}
	MirrorCaptureIn = MirrorRate;
	UWorld* InWorld = GetWorld();
	APlayerController* Pc = InWorld ? InWorld->GetFirstPlayerController() : nullptr;
	const APlayerCameraManager* Cam = Pc ? Pc->PlayerCameraManager.Get() : nullptr;
	if (!Cam)
	{
		return;
	}
	if (!MirrorTarget)
	{
		// The viewport's shape at 960 wide: the mirror samples it at the screen's own position (flipped), so the shapes must agree.
		FVector2D View(16.0, 9.0);
		if (UGameViewportClient* Vp = InWorld->GetGameViewport())
		{
			Vp->GetViewportSize(View);
		}
		const int32 H = FMath::Clamp(FMath::RoundToInt32(MirrorPixels * View.Y / FMath::Max(1.0, View.X)), 270, 720);
		MirrorTarget = NewObject<UTextureRenderTarget2D>(this, NAME_None, RF_Transient);
		MirrorTarget->RenderTargetFormat = RTF_RGBA8_SRGB;
		MirrorTarget->ClearColor = FLinearColor::Black;
		MirrorTarget->InitAutoFormat(MirrorPixels, H);
		MirrorTarget->UpdateResourceImmediate(true);
	}
	if (!MirrorCapture)
	{
		MirrorCapture = NewPart<USceneCaptureComponent2D>();
		MirrorCapture->bCaptureEveryFrame = false;
		MirrorCapture->bCaptureOnMovement = false;
		MirrorCapture->bAlwaysPersistRenderingState = true;
		MirrorCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		MirrorCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
		MirrorCapture->TextureTarget = MirrorTarget;
		MirrorCapture->bEnableClipPlane = true;
		if (RoomLens)
		{
			MirrorCapture->PostProcessSettings = RoomLens->Settings;
			MirrorCapture->PostProcessBlendWeight = 1.0f;
		}
		MirrorCapture->ShowFlags.SetMotionBlur(false);
		for (UMaterialInstanceDynamic* Mid : MirrorMids)
		{
			if (Mid)
			{
				Mid->SetTextureParameterValue(TEXT("Capture"), MirrorTarget);
				Mid->SetScalarParameterValue(TEXT("UseCapture"), 1.0f);
			}
		}
	}
	// The shop and whoever is in it: nothing of the street (it only shows through the window behind, and costs a second street).
	MirrorCapture->ShowOnlyActors.Reset();
	MirrorCapture->ShowOnlyActors.Add(this);
	const FBox Room = RoomBounds().ExpandBy(100.0);
	for (TActorIterator<APawn> It(InWorld); It; ++It)
	{
		if (Room.IsInsideOrOn(It->GetActorLocation()))
		{
			MirrorCapture->ShowOnlyActors.Add(*It);
		}
	}
	// The eye reflected in the mirror's plane: its forward and up reflected, the image flipped back by the material.
	const FTransform& T = GetActorTransform();
	const FVector P = T.TransformPosition(MirrorPlanes[0].Point);
	const FVector N = T.TransformVectorNoScale(MirrorPlanes[0].Normal).GetSafeNormal();
	auto Reflect = [&N](const FVector& V) { return V - 2.0 * FVector::DotProduct(V, N) * N; };
	const FVector Eye = Cam->GetCameraLocation();
	const FRotationMatrix View(Cam->GetCameraRotation());
	const FVector MirrorEye = Eye - 2.0 * FVector::DotProduct(Eye - P, N) * N;
	const FRotator MirrorRot = FRotationMatrix::MakeFromXZ(Reflect(View.GetUnitAxis(EAxis::X)), Reflect(View.GetUnitAxis(EAxis::Z))).Rotator();
	MirrorCapture->SetVisibility(true);
	MirrorCapture->SetWorldLocationAndRotation(MirrorEye, MirrorRot);
	MirrorCapture->FOVAngle = Cam->GetFOVAngle();
	MirrorCapture->ClipPlaneBase = P;
	MirrorCapture->ClipPlaneNormal = N;
	MirrorCapture->CaptureSceneDeferred();
}

void AShopStage::ApplyLights()
{
	// Asleep, only the two brightest fixtures stay on (unshadowed) with the spill; shut, only the night lights and the
	// all-night signs.
	TArray<int32> Brightest;
	for (int32 I = 0; I < Lights.Num(); ++I)
	{
		if (Lights[I].Role == EShopLight::Fixture)
		{
			Brightest.Add(I);
		}
	}
	Brightest.Sort([this](int32 A, int32 B) { return Lights[A].Base > Lights[B].Base; });
	Brightest.SetNum(FMath::Min(2, Brightest.Num()));
	auto IsOn = [this](EShopLight LightRole, bool bTopFixture) {
		switch (LightRole)
		{
		case EShopLight::Fixture:
			return bOpen && (bAwake || bTopFixture);
		case EShopLight::Night:
			return !bOpen;
		case EShopLight::Spill:
		case EShopLight::Sign:
			return bOpen;
		default:
			return true;
		}
	};
	for (int32 I = 0; I < Lights.Num(); ++I)
	{
		ULightComponent* L = Lights[I].Light.Get();
		if (!L)
		{
			continue;
		}
		const bool bOn = IsOn(Lights[I].Role, Brightest.Contains(I));
		if (L->IsVisible() != bOn)
		{
			L->SetVisibility(bOn);
		}
		const bool bShadow = bOn && bAwake && Lights[I].Role == EShopLight::Fixture;
		if (static_cast<bool>(L->CastShadows) != bShadow)
		{
			L->SetCastShadows(bShadow);
		}
	}
	for (const FGlow& G : Glows)
	{
		UMaterialInstanceDynamic* Mid = G.Mid.Get();
		if (!Mid)
		{
			continue;
		}
		// A glowing fixture stays lit asleep (only its light goes); everything follows the hours.
		const float K = IsOn(G.Role, true) ? 1.0f : 0.0f;
		for (const TPair<FName, FLinearColor>& C : G.Colors)
		{
			Mid->SetVectorParameterValue(C.Key, FLinearColor(C.Value.R * K, C.Value.G * K, C.Value.B * K, C.Value.A));
		}
		for (const TPair<FName, float>& S : G.Strengths)
		{
			Mid->SetScalarParameterValue(S.Key, S.Value * K);
		}
	}
}

bool AShopStage::IsInside(const FVector& At) const
{
	return RoomBox.IsValid && RoomBox.IsInsideOrOn(GetActorTransform().InverseTransformPosition(At));
}

FBox AShopStage::RoomBounds() const
{
	return RoomBox.IsValid ? RoomBox.TransformBy(GetActorTransform()) : FBox(ForceInit);
}

FVector AShopStage::DoorMiddle() const
{
	return GetActorTransform().TransformPosition(DoorwayMiddle() + FVector(0.0, 0.0, 100.0));
}

FTransform AShopStage::OutsideDoor() const
{
	// On the sidewalk a step and a half out from the doorway, facing the street (away from the door).
	const FVector In = Along(DoorSpec.InYaw);
	return FTransform(FRotator(0.0f, DoorSpec.InYaw + 180.0f, 0.0f), DoorwayMiddle() - In * 170.0 + FVector(0.0, 0.0, 100.0)) * GetActorTransform();
}

bool AShopStage::AtDoorway(const FVector& Pos, double Reach) const
{
	if (!bHasDoor)
	{
		return false;
	}
	const FVector L = GetActorTransform().InverseTransformPosition(Pos);
	const FVector Shut = Along(DoorSpec.ShutYaw);
	const FVector In = Along(DoorSpec.InYaw);
	const FVector D = L - DoorwayMiddle();
	return FMath::Abs(FVector::DotProduct(D, Shut)) < DoorwaySpan() * 0.5 + 60.0 && FMath::Abs(FVector::DotProduct(D, In)) < Reach && D.Z > -100.0 && D.Z < 400.0;
}

FString AShopStage::OpenDoor(bool bInMember)
{
	bMember = bInMember;
	if (!bHasDoor)
	{
		return FString();
	}
	if (bSomeoneInside)
	{
		// On the way out: the push bar, whatever the hour and the lock (nobody is shut in a closed shop).
		BuzzIn = -1.0f;
		bDoorHeld = true;
		DoorClear = 0.0f;
		return FString();
	}
	switch (DoorSpec.Kind)
	{
	case EShopDoorKind::Fob:
		// The fob opens it at any hour; without one, only while someone's at the desk to buzz you in.
		if (!bMember && !(bOpen && bStaffed))
		{
			return ClosedPrompt();
		}
		break;
	case EShopDoorKind::Buzzer:
		if (!bOpen)
		{
			return ClosedPrompt();
		}
		if (BuzzIn < 0.0f && DoorAmount <= 0.0f)
		{
			BuzzIn = BuzzSeconds; // the keeper looks up and buzzes
		}
		return FString();
	default:
		if (!bOpen)
		{
			return ClosedPrompt();
		}
		break;
	}
	bDoorHeld = true;
	DoorClear = 0.0f;
	return FString();
}

int32 AShopStage::UpdateDoor(const FVector& Someone, float Dt)
{
	bDoorJustOpened = false;
	if (!bHasDoor || !bBuilt)
	{
		return 0;
	}
	bSomeoneInside = IsInside(Someone);
	if (BuzzIn >= 0.0f)
	{
		BuzzIn -= Dt;
		if (BuzzIn < 0.0f)
		{
			BuzzIn = -1.0f;
			bDoorHeld = true;
			DoorClear = 0.0f;
		}
	}
	// Anyone at the doorway keeps it from shutting on them; anyone in its swing pushes it open again. Walkers open it for
	// themselves while the shop's open (the keeper buzzes them in at the pawn); shut, it's locked to them.
	const bool bNear = AtDoorway(Someone, 190.0);
	const bool bInSwing = AtDoorway(Someone, 80.0);
	bool bWalker = false;
	if (bOpen)
	{
		if (const UStreetWorld* R = Registry())
		{
			bWalker = R->WalkersNear(GetActorTransform().TransformPosition(DoorwayMiddle() + FVector(0.0, 0.0, 90.0)), DoorwaySpan() * 0.5 + 110.0);
		}
	}
	if (bWalker && !bDoorHeld)
	{
		bDoorHeld = true;
		DoorClear = 0.0f;
	}
	if (bDoorHeld)
	{
		// Its closer takes it once nobody's been at it for a moment.
		DoorClear = bNear || bWalker ? 0.0f : DoorClear + Dt;
		if (DoorClear > 1.0f)
		{
			bDoorHeld = false;
		}
	}
	else if (bInSwing && DoorAmount > 0.05f)
	{
		bDoorHeld = true;
		DoorClear = 0.0f;
	}
	const float Was = DoorAmount;
	if (bDoorHeld && DoorAmount < 1.0f)
	{
		DoorAmount = FMath::FInterpTo(DoorAmount, 1.0f, Dt, 5.0f);
		if (DoorAmount > 0.998f)
		{
			DoorAmount = 1.0f;
		}
		DoorAmount = FMath::Max(DoorAmount, 0.001f);
	}
	else if (!bDoorHeld && DoorAmount > 0.0f)
	{
		DoorAmount = FMath::FInterpConstantTo(DoorAmount, 0.0f, Dt, 0.85f);
	}
	if (Was == DoorAmount)
	{
		return 0;
	}
	const float Ease = FMath::InterpEaseInOut(0.0f, 1.0f, DoorAmount, 1.6f);
	if (DoorLeaf)
	{
		DoorLeaf->SetRelativeRotation(FRotator(0.0f, DoorSpec.ShutYaw + DoorSpec.OpenYaw * Ease, 0.0f));
	}
	if (DoorLeaf2)
	{
		DoorLeaf2->SetRelativeRotation(FRotator(0.0f, DoorSpec.ShutYaw2 + DoorSpec.OpenYaw2 * Ease, 0.0f));
	}
	if (DoorStop)
	{
		const bool bBlock = DoorAmount < 0.3f;
		if ((DoorStop->GetCollisionEnabled() != ECollisionEnabled::NoCollision) != bBlock)
		{
			DoorStop->SetCollisionEnabled(bBlock ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		}
	}
	if (Was <= 0.0f)
	{
		bDoorJustOpened = true;
		return 1;
	}
	return DoorAmount <= 0.0f ? 2 : 0;
}

void AShopStage::UpdateWake(float Dt)
{
	WakeCheckIn -= Dt;
	if (WakeCheckIn > 0.0f)
	{
		return;
	}
	WakeCheckIn = WakeEvery;
	if (AwakeHold >= 0)
	{
		SetAwake(AwakeHold != 0);
		return;
	}
	const UWorld* InWorld = GetWorld();
	const APlayerController* Pc = InWorld ? InWorld->GetFirstPlayerController() : nullptr;
	const APlayerCameraManager* Cam = Pc ? Pc->PlayerCameraManager.Get() : nullptr;
	if (!Cam || !RoomBox.IsValid)
	{
		return;
	}
	const FVector Eye = Cam->GetCameraLocation();
	const FVector Hero = Pc->GetPawn() ? Pc->GetPawn()->GetActorLocation() : Eye;
	const FBox World = RoomBox.TransformBy(GetActorTransform());
	const double Far = FMath::Sqrt(World.ComputeSquaredDistanceToPoint(Eye));
	bool bWake = IsInside(Hero) || Far <= WakeNear;
	if (!bWake && bHasGlass && Far <= WakeSeen)
	{
		// Its glass in view: the camera on the street side of it, the storefront's middle inside the view's cone.
		const FVector Glass = GetActorTransform().TransformPosition(GlassAt);
		const FVector Out = GetActorTransform().TransformVectorNoScale(GlassOut);
		const FVector To = (Glass - Eye).GetSafeNormal();
		const double HalfFov = FMath::DegreesToRadians(FMath::Min(Cam->GetFOVAngle() * 0.5 + 10.0, 89.0));
		bWake = FVector::DotProduct(Eye - Glass, Out) > 0.0 && FVector::DotProduct(Cam->GetCameraRotation().Vector(), To) > FMath::Cos(HalfFov);
	}
	SetAwake(bWake);
}

void AShopStage::ApplyGrille()
{
	if (GrilleArt)
	{
		const FVector Scale = GrilleArt->GetRelativeScale3D();
		GrilleArt->SetRelativeScale3D(FVector(Scale.X, Scale.Y, FMath::Lerp(1.0f, GrilleRolled, GrilleOpen)));
	}
	if (GrilleRoot)
	{
		if (GrilleHeight > 0.0)
		{
			GrilleRoot->SetRelativeLocation(FVector(0.0, 0.0, GrilleOpen * GrilleHeight));
		}
		const bool bShow = GrilleOpen < 0.999f;
		if (GrilleRoot->IsVisible() != bShow)
		{
			GrilleRoot->SetVisibility(bShow, true);
		}
	}
}

void AShopStage::Animate(float Dt)
{
	for (FSpin& S : Spins)
	{
		USceneComponent* Part = S.Part.Get();
		if (!Part || (S.bOpenOnly && !bOpen))
		{
			continue;
		}
		S.Turned += Dt;
		Part->SetRelativeRotation(S.Base + S.PerSecond * S.Turned);
	}
	if (Clocks.Num() > 0)
	{
		// The hands as the world's clock has them (the clock races through a skip: the hands spin with it).
		const double Mins = MinuteOfDay();
		const double MinuteDeg = FMath::Fmod(Mins, 60.0) / 60.0 * 360.0;
		const double HourDeg = FMath::Fmod(Mins / 60.0, 12.0) / 12.0 * 360.0;
		auto Point = [](USceneComponent* Arm, double Deg) {
			if (!Arm)
			{
				return;
			}
			// In the dial's plane: twelve up (+Z), three to the viewer's right (-Y, facing the dial along -X).
			const double R = FMath::DegreesToRadians(Deg);
			const FVector Dir(0.0, -FMath::Sin(R), FMath::Cos(R));
			Arm->SetRelativeRotation(FRotationMatrix::MakeFromZX(Dir, FVector::ForwardVector).Rotator());
		};
		for (const FClock& C : Clocks)
		{
			Point(C.Hour.Get(), HourDeg);
			Point(C.Minute.Get(), MinuteDeg);
		}
	}
}

void AShopStage::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bBuilt)
	{
		return;
	}
	const float Dt = FMath::Min(DeltaSeconds, 0.1f);
	Clock += Dt;
	UpdateWake(Dt);
	// The street may have built again since the carves were made (its zones registered again): carved again.
	CarveCheckIn -= Dt;
	if (CarveCheckIn <= 0.0f && CarveHoles.Num() > 0)
	{
		CarveCheckIn = CarveEvery;
		const UStreetWorld* R = Registry();
		const bool bStale = Carved.ContainsByPredicate([](const FCarved& C) { return !C.Original.IsValid() || C.Original->IsVisible(); });
		if (bStale || (R && R->ZoneRevision() != CarvedRevision))
		{
			ApplyCarves();
		}
	}
	// The grille rolls up when it opens and down when it shuts (the art's curtain into its hood; the block-out up and away).
	const float Want = bOpen ? 1.0f : 0.0f;
	if (GrilleOpen != Want && (GrilleRoot || GrilleArt))
	{
		GrilleOpen = FMath::FInterpConstantTo(GrilleOpen, Want, Dt, 0.6f);
		ApplyGrille();
	}
	UpdateMirrorCapture(Dt);
	if (bAwake)
	{
		Animate(Dt);
	}
}

FString AShopStage::Describe() const
{
	static const TCHAR* const MirrorNames[3] = {TEXT("lumen"), TEXT("capture"), TEXT("plain")};
	return FString::Printf(TEXT("%s: %s%s, %s, door %.2f%s, %d lights, %d seats, %d carved%s"), *VenueName, bOpen ? TEXT("open") : TEXT("shut"),
		bOpen && !bStaffed ? TEXT(" (unstaffed)") : TEXT(""), bAwake ? TEXT("awake") : TEXT("asleep"), DoorAmount, BuzzIn >= 0.0f ? TEXT(" (buzzing)") : TEXT(""), Lights.Num(),
		Seats.Num(), Carved.Num(),
		MirrorPlanes.Num() > 0 ? *FString::Printf(TEXT(", mirror %s%s"), MirrorNames[MirrorMode()], bMirrorLive ? TEXT(" live") : TEXT("")) : TEXT(""));
}
