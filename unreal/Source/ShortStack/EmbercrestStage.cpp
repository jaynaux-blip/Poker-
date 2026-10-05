#include "EmbercrestStage.h"

#include "EmbercrestShared.h"

#include "BlenderProps.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "PhysicsEngine/BodySetup.h"
#include "RenderingThread.h"
#include "Slate/WidgetRenderer.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogEmbercrestStage, Log, All);

namespace EcxStageDetail
{
TAutoConsoleVariable<int32> CVarLightShadows(TEXT("ss.ECX.LightShadows"), 1,
	TEXT("The Embercrest's local lights cast shadows (MegaLights traces them at a fixed cost); 0 turns them all off."));

/** The bounds polygon (spec 2.8), map meters. */
const FVector2D PlayBounds[16] = {FVector2D(0.0, -18.5), FVector2D(0.0, 18.5), FVector2D(3.0, 18.5), FVector2D(3.0, 37.0), FVector2D(52.5, 37.0), FVector2D(52.5, 48.0),
	FVector2D(64.0, 48.0), FVector2D(64.0, 20.0), FVector2D(70.0, 20.0), FVector2D(70.0, 6.0), FVector2D(64.0, 6.0), FVector2D(64.0, -48.0), FVector2D(52.5, -48.0),
	FVector2D(52.5, -37.0), FVector2D(3.0, -37.0), FVector2D(3.0, -18.5)};

bool InsideBounds(const FVector2D& P)
{
	bool bIn = false;
	for (int32 I = 0, J = 15; I < 16; J = I++)
	{
		const FVector2D& A = PlayBounds[I];
		const FVector2D& B = PlayBounds[J];
		if ((A.Y > P.Y) != (B.Y > P.Y) && P.X < (B.X - A.X) * (P.Y - A.Y) / (B.Y - A.Y) + A.X)
		{
			bIn = !bIn;
		}
	}
	return bIn;
}

double DistanceToBounds(const FVector2D& P)
{
	double Best = TNumericLimits<double>::Max();
	for (int32 I = 0, J = 15; I < 16; J = I++)
	{
		const FVector2D A = PlayBounds[J];
		const FVector2D B = PlayBounds[I];
		const FVector2D AB = B - A;
		const double T = FMath::Clamp(FVector2D::DotProduct(P - A, AB) / FMath::Max(AB.SizeSquared(), 1e-6), 0.0, 1.0);
		Best = FMath::Min(Best, FVector2D::Distance(P, A + AB * T));
	}
	return Best;
}

/** Distance from P to the box Min..Max in the plane (0 inside). */
double DistanceToRect(const FVector2D& P, const FVector2D& Min, const FVector2D& Max)
{
	const double Dx = FMath::Max3(Min.X - P.X, 0.0, P.X - Max.X);
	const double Dy = FMath::Max3(Min.Y - P.Y, 0.0, P.Y - Max.Y);
	return FMath::Sqrt(Dx * Dx + Dy * Dy);
}

/** 1 within Near of a source, falling to 0 at Far (squared, as a sound falls off). */
float SoundFalloff(double Distance, double Near, double Far)
{
	const double T = FMath::Clamp(1.0 - (Distance - Near) / FMath::Max(Far - Near, 1.0), 0.0, 1.0);
	return static_cast<float>(T * T);
}
} // namespace EcxStageDetail

using namespace EcxStageDetail;

AEmbercrestStage::AEmbercrestStage()
{
	PrimaryActorTick.bCanEverTick = true;
	// After the camera has moved this frame: the steam turns to it, the screens and the fires follow the clock.
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	CineCam = CreateDefaultSubobject<UCameraComponent>(TEXT("CineCam"));
	CineCam->SetupAttachment(Root);
	CineCam->bConstrainAspectRatio = false;
	CineCam->SetFieldOfView(65.0f);
	CineCam->SetRelativeLocation(FVector(30000.0, -10000.0, 3000.0));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeShape(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Basic(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	CubeMesh = Cube.Object;
	CylinderMesh = Cylinder.Object;
	SphereMesh = Sphere.Object;
	ConeMesh = ConeShape.Object;
	PlaneMesh = Plane.Object;
	FallbackMaterial = Basic.Object;
}

// ------------------------------------------------------------------ building it

void AEmbercrestStage::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UWorld* World = GetWorld();
	if (!World || World->WorldType != EWorldType::Editor)
	{
		// In play (spawned by the game mode, or loaded in a cooked game) the world is built at BeginPlay.
		return;
	}
	// The references outside the always-cooked content, resolved here and saved with the level.
	if (!CloudMaterial)
	{
		CloudMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst.m_SimpleVolumetricCloud_Inst"), nullptr,
			LOAD_NoWarn | LOAD_Quiet);
	}
	if (bEditorPreview)
	{
		BuildAll();
	}
}

void AEmbercrestStage::RebuildSet()
{
#if WITH_EDITOR
	RerunConstructionScripts();
#endif
}

void AEmbercrestStage::BeginPlay()
{
	Super::BeginPlay();
	EnsureBuilt();
}

void AEmbercrestStage::EnsureBuilt()
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
	const double Started = FPlatformTime::Seconds();
	// Whatever of the editor's preview came along into play (its parts aren't duplicated, but a level saved before they were
	// marked so, or another path in, could bring some): gone, deepest first, before the world is built for play.
	TInlineComponentArray<UActorComponent*> Preview;
	GetComponents(Preview);
	Preview.RemoveAll([](const UActorComponent* C) { return !C || !C->IsCreatedByConstructionScript(); });
	auto Depth = [](const UActorComponent& C) {
		int32 D = 0;
		for (const USceneComponent* S = Cast<USceneComponent>(&C); S && S->GetAttachParent(); S = S->GetAttachParent())
		{
			++D;
		}
		return D;
	};
	Preview.Sort([&Depth](const UActorComponent& A, const UActorComponent& B) { return Depth(A) > Depth(B); });
	for (UActorComponent* C : Preview)
	{
		C->DestroyComponent();
	}
	BlueprintCreatedComponents.Reset();
	BuildAll();
	// The far ranges and the city reflect in the glass and the wet stone: Lumen traces far enough to reach them (here
	// only: put back as the level ends). The thin cloud layer at low quality (spec 4.1).
	SetCvar(TEXT("r.LumenScene.FarField"), TEXT("1"));
	SetCvar(TEXT("r.LumenScene.FarField.MaxTraceDistance"), TEXT("2500000"));
	SetCvar(TEXT("r.VolumetricCloud.ViewRaySampleMaxCount"), TEXT("256"));
	UE_LOG(LogEmbercrestStage, Log, TEXT("The Embercrest built in %.0f ms: %d parts, %d lights, %d glows, %d lit meshes"), (FPlatformTime::Seconds() - Started) * 1000.0,
		BlueprintCreatedComponents.Num(), Lights.Num(), Glows.Num(), LitMeshes.Num());
}

void AEmbercrestStage::SetCvar(const TCHAR* Name, const TCHAR* Value)
{
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name))
	{
		SavedCvars.Emplace(FString(Name), Var->GetString());
		Var->Set(Value, ECVF_SetByCode);
	}
}

void AEmbercrestStage::RestoreCvars()
{
	for (int32 I = SavedCvars.Num() - 1; I >= 0; --I)
	{
		if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(*SavedCvars[I].Key))
		{
			Var->Set(*SavedCvars[I].Value, ECVF_SetByCode);
		}
	}
	SavedCvars.Reset();
}

void AEmbercrestStage::EndPlay(const EEndPlayReason::Type Reason)
{
	RestoreCvars();
	if (Renderer)
	{
		BeginCleanup(Renderer);
		Renderer = nullptr;
	}
	Super::EndPlay(Reason);
}

void AEmbercrestStage::BeginDestroy()
{
	// The editor's preview draws its signs too, and an editor world never ends play.
	if (Renderer)
	{
		BeginCleanup(Renderer);
		Renderer = nullptr;
	}
	Super::BeginDestroy();
}

void AEmbercrestStage::ResetParts()
{
	Materials.Reset();
	Lights.Reset();
	Glows.Reset();
	LitMeshes.Reset();
	Fires.Reset();
	Spots.Reset();
	HourMids.Reset();
	SnowMids.Reset();
	SnowParts.Reset();
	Steam.Reset();
	SteamBase.Reset();
	GroundMesh = nullptr;
	TerrainMesh = nullptr;
	RidgeMesh = nullptr;
	Sun = nullptr;
	Moon = nullptr;
	Atmosphere = nullptr;
	Clouds = nullptr;
	CloudMid = nullptr;
	SkyLight = nullptr;
	Fog = nullptr;
	Lens = nullptr;
	StarMid = nullptr;
	StarDome = nullptr;
	MarqueeSpill = nullptr;
	RevWings = nullptr;
	SwingLeaves.Reset();
	SwingOpen.Reset();
	SwingTarget.Reset();
	SwingSign.Reset();
	MarqueeMid = nullptr;
	TickerMid = nullptr;
	MarqueePages[0] = nullptr;
	MarqueePages[1] = nullptr;
	TickerTarget = nullptr;
	PanelTargets.Reset();
	BusRoot = nullptr;
	BusBody = nullptr;
	BusDoors.Reset();
	BusWheels.Reset();
	BusSignMids.Reset();
	BusRunLights.Reset();
	BusDoorLight = nullptr;
	BusBlocker = nullptr;
	BusFloor = nullptr;
	BusLampsMesh = nullptr;
	AppliedHour = -100.0;
	AppliedGlowScale = -1.0f;
}

void AEmbercrestStage::BuildAll()
{
	ResetParts();
	BuildSky();
	BuildGround();
	BuildSteps();
	BuildBuilding();
	BuildEntryDoors();
	BuildCanopy();
	BuildTower();
	BuildPlaza();
	BuildFireBowls();
	BuildFixtures();
	BuildLandscape();
	BuildTrees();
	BuildFar();
	BuildLights();
	BuildScreens();
	BuildBus();
	BuildBlockers();
	BuildSpots();
	bBuilt = true;
	ApplyHour();
}

// ------------------------------------------------------------------ parts

UStaticMeshComponent* AEmbercrestStage::Site(const TCHAR* Name, bool bCollide, const FVector& Scale)
{
	UStaticMesh* M = Ecx::LoadMesh(Name);
	if (!M)
	{
		return nullptr;
	}
	UStaticMeshComponent* C = NewPart<UStaticMeshComponent>();
	C->SetStaticMesh(M);
	// SITE: modeled in place in Blender with its front toward -X; the importer keeps -X and mirrors Y, so turned half round
	// it stands where the map has it (UE = -100x, +100y, +100z).
	C->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator(0.0f, 180.0f, 0.0f));
	C->SetRelativeScale3D(Scale);
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

UStaticMeshComponent* AEmbercrestStage::Prop(const TCHAR* Name, const FVector& At, float Yaw, const FVector& Scale, USceneComponent* Parent)
{
	UStaticMesh* M = Ecx::LoadMesh(Name);
	if (!M)
	{
		return nullptr;
	}
	UStaticMeshComponent* C = NewPart<UStaticMeshComponent>(Parent);
	C->SetStaticMesh(M);
	C->SetRelativeLocationAndRotation(At, FRotator(0.0f, Yaw, 0.0f));
	C->SetRelativeScale3D(Scale);
	// The props don't collide as imported (their hulls are crude): BuildBlockers boxes what should.
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	return C;
}

UStaticMeshComponent* AEmbercrestStage::Box(UMaterialInterface* Material, const FVector& Min, const FVector& Max, bool bCollide, USceneComponent* Parent)
{
	UStaticMeshComponent* C = NewPart<UStaticMeshComponent>(Parent);
	C->SetStaticMesh(CubeMesh);
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

UStaticMeshComponent* AEmbercrestStage::TurnedBox(UMaterialInterface* Material, const FVector& Center, const FVector& Size, float Yaw, bool bCollide, USceneComponent* Parent)
{
	UStaticMeshComponent* C = Box(Material, Center - Size * 0.5, Center + Size * 0.5, bCollide, Parent);
	C->SetRelativeRotation(FRotator(0.0f, Yaw, 0.0f));
	return C;
}

UStaticMeshComponent* AEmbercrestStage::Slope(UMaterialInterface* Material, double X0, double X1, double Y0, double Y1, double Z0, double Z1, double Thick, bool bCollide)
{
	// The engine cube turned about Y: its top face runs from (X0, Z0) to (X1, Z1).
	const double Dx = X1 - X0;
	const double Dz = Z1 - Z0;
	const double Len = FMath::Sqrt(Dx * Dx + Dz * Dz);
	const float Pitch = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Dz, Dx)));
	const FVector TopMid((X0 + X1) * 0.5, (Y0 + Y1) * 0.5, (Z0 + Z1) * 0.5);
	const FVector Down = FRotator(Pitch, 0.0f, 0.0f).RotateVector(FVector(0.0, 0.0, -1.0));
	UStaticMeshComponent* C = NewPart<UStaticMeshComponent>();
	C->SetStaticMesh(CubeMesh);
	C->SetRelativeLocationAndRotation(TopMid + Down * (Thick * 0.5), FRotator(Pitch, 0.0f, 0.0f));
	C->SetRelativeScale3D(FVector(Len / 100.0, FMath::Abs(Y1 - Y0) / 100.0, Thick / 100.0));
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

UStaticMeshComponent* AEmbercrestStage::SlopeY(UMaterialInterface* Material, double X0, double X1, double Y0, double Y1, double Z0, double Z1, double Thick, bool bCollide)
{
	// Turned about X: the top runs from (Y0, Z0) to (Y1, Z1). A positive roll lifts +Y.
	const double Dy = Y1 - Y0;
	const double Dz = Z1 - Z0;
	const double Len = FMath::Sqrt(Dy * Dy + Dz * Dz);
	const float Roll = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Dz, Dy)));
	const FRotator Turn(0.0f, 0.0f, -Roll);
	const FVector TopMid((X0 + X1) * 0.5, (Y0 + Y1) * 0.5, (Z0 + Z1) * 0.5);
	// Pick the roll's sign from where it puts the far end (engines differ in what a positive roll does).
	FRotator Use = Turn;
	if (FMath::Sign(Use.RotateVector(FVector(0.0, 1.0, 0.0)).Z) != FMath::Sign(Dz) && !FMath::IsNearlyZero(Dz))
	{
		Use = FRotator(0.0f, 0.0f, Roll);
	}
	const FVector Down = Use.RotateVector(FVector(0.0, 0.0, -1.0));
	UStaticMeshComponent* C = NewPart<UStaticMeshComponent>();
	C->SetStaticMesh(CubeMesh);
	C->SetRelativeLocationAndRotation(TopMid + Down * (Thick * 0.5), Use);
	C->SetRelativeScale3D(FVector(FMath::Abs(X1 - X0) / 100.0, Len / 100.0, Thick / 100.0));
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

UStaticMeshComponent* AEmbercrestStage::Cyl(UMaterialInterface* Material, const FVector& Base, float Radius, float Height, bool bCollide, USceneComponent* Parent)
{
	UStaticMeshComponent* C = NewPart<UStaticMeshComponent>(Parent);
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

UStaticMeshComponent* AEmbercrestStage::Cone(UMaterialInterface* Material, const FVector& Base, float Radius, float Height, bool bCollide)
{
	if (!ConeMesh)
	{
		return nullptr;
	}
	// Fitted by its bounds (wherever the engine shape keeps its pivot): its base on Base, Radius and Height as asked.
	const FBox B = ConeMesh->GetBoundingBox();
	const FVector Ext = B.GetExtent();
	const FVector Mid = B.GetCenter();
	const FVector Scale(Radius / FMath::Max(Ext.X, 1.0), Radius / FMath::Max(Ext.Y, 1.0), Height / FMath::Max(2.0 * Ext.Z, 1.0));
	UStaticMeshComponent* C = NewPart<UStaticMeshComponent>();
	C->SetStaticMesh(ConeMesh);
	C->SetRelativeScale3D(Scale);
	C->SetRelativeLocation(Base - FVector(Mid.X * Scale.X, Mid.Y * Scale.Y, (Mid.Z - Ext.Z) * Scale.Z));
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

void AEmbercrestStage::PawnWall(const FVector& Min, const FVector& Max)
{
	UStaticMeshComponent* C = Box(nullptr, Min, Max, false);
	C->SetVisibility(false);
	C->SetHiddenInGame(true);
	C->SetCastShadow(false);
	// Pawns only: the camera, the bus and traces pass through.
	C->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	C->SetCollisionObjectType(ECC_WorldStatic);
	C->SetCollisionResponseToAllChannels(ECR_Ignore);
	C->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	C->SetAffectDistanceFieldLighting(false);
	C->SetVisibleInRayTracing(false);
	C->bAffectDynamicIndirectLighting = false;
}

void AEmbercrestStage::Block(const FVector& Min, const FVector& Max, float Yaw, USceneComponent* Parent, bool bStopCamera)
{
	UStaticMeshComponent* C = Yaw == 0.0f ? Box(nullptr, Min, Max, true, Parent) : TurnedBox(nullptr, (Min + Max) * 0.5, (Max - Min).GetAbs(), Yaw, true, Parent);
	C->SetVisibility(false);
	C->SetHiddenInGame(true);
	C->SetCastShadow(false);
	C->SetAffectDistanceFieldLighting(false);
	C->SetVisibleInRayTracing(false);
	C->bAffectDynamicIndirectLighting = false;
	// The camera's arm doesn't bump on the props' boxes; it does stop at the building's faces (the art has no collision of
	// its own, so without them the camera would end up behind the entrance's glass or inside the wings).
	if (!bStopCamera)
	{
		C->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}
}

UMaterialInstanceDynamic* AEmbercrestStage::Mat(FName Key, uint32 SrgbHexColor, float Roughness, float Metallic, float Emissive)
{
	if (TObjectPtr<UMaterialInstanceDynamic>* Found = Materials.Find(Key))
	{
		return *Found;
	}
	UMaterialInterface* Parent = Ecx::LoadMat(TEXT("M_Surface"));
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Parent ? Parent : FallbackMaterial.Get(), this);
	const FLinearColor Color = Ecx::Srgb(SrgbHexColor);
	M->SetVectorParameterValue(TEXT("BaseColor"), Color);
	M->SetVectorParameterValue(TEXT("Color"), Color);
	M->SetScalarParameterValue(TEXT("Roughness"), Roughness);
	M->SetScalarParameterValue(TEXT("Metallic"), Metallic);
	M->SetScalarParameterValue(TEXT("Emissive"), Emissive);
	Materials.Add(Key, M);
	return M;
}

UMaterialInstanceDynamic* AEmbercrestStage::GroundMat(int32 Kind, bool bWorldUV)
{
	if (Kind == 6)
	{
		return SnowMat(true);
	}
	const FName Key(*FString::Printf(TEXT("Ground%d_%d"), Kind, bWorldUV ? 1 : 0));
	if (TObjectPtr<UMaterialInstanceDynamic>* Found = Materials.Find(Key))
	{
		return *Found;
	}
	// The texture sets the plaza's author writes (spec 3.2), each with its own tile; a set that isn't imported falls back
	// to the material's own pattern for the kind.
	static const TCHAR* const Sets[8] = {TEXT("Paver"), TEXT("Sett"), TEXT("Asphalt"), TEXT("Curb"), TEXT("Concrete"), TEXT("WallStone"), nullptr, nullptr};
	static const float Tiles[8] = {3.6f, 2.0f, 4.0f, 2.0f, 2.0f, 3.0f, 4.0f, 1.0f};
	static const uint32 Colors[8] = {0x8c8a86, 0x2e3036, 0x26272a, 0x9a978f, 0x8f8b84, 0x2b2d32, Ecx::SnowHex, 0xe9e6dc};
	UMaterialInstanceDynamic* M = nullptr;
	if (UMaterialInterface* Parent = Ecx::LoadMat(TEXT("M_ECX_Ground")))
	{
		M = UMaterialInstanceDynamic::Create(Parent, this);
		M->SetScalarParameterValue(TEXT("Kind"), static_cast<float>(Kind));
		M->SetScalarParameterValue(TEXT("WorldUV"), bWorldUV ? 1.0f : 0.0f);
		M->SetScalarParameterValue(TEXT("UseVertexColor"), bWorldUV ? 0.0f : 1.0f);
		M->SetScalarParameterValue(TEXT("Tile"), Tiles[FMath::Clamp(Kind, 0, 7)]);
		M->SetVectorParameterValue(TEXT("Tint"), Ecx::Srgb(Colors[FMath::Clamp(Kind, 0, 7)]));
		bool bTex = false;
		if (const TCHAR* Set = Sets[FMath::Clamp(Kind, 0, 7)])
		{
			UTexture* Base = Ecx::LoadTex(TEXT("ECX"), *FString::Printf(TEXT("T_ECX_%s_BaseColor"), Set));
			UTexture* Normal = Ecx::LoadTex(TEXT("ECX"), *FString::Printf(TEXT("T_ECX_%s_Normal"), Set));
			UTexture* Orm = Ecx::LoadTex(TEXT("ECX"), *FString::Printf(TEXT("T_ECX_%s_ORM"), Set));
			if (Base && Normal && Orm)
			{
				M->SetTextureParameterValue(TEXT("BaseTex"), Base);
				M->SetTextureParameterValue(TEXT("NormalTex"), Normal);
				M->SetTextureParameterValue(TEXT("OrmTex"), Orm);
				bTex = true;
			}
		}
		M->SetScalarParameterValue(TEXT("UseTex"), bTex ? 1.0f : 0.0f);
	}
	else if (UMaterialInterface* Street = Ecx::LoadMat(TEXT("M_Street")))
	{
		// The street's wet patterns: slabs for the pavers, asphalt, concrete.
		static const float Patterns[8] = {3.0f, 3.0f, 2.0f, 5.0f, 5.0f, 5.0f, 0.0f, 0.0f};
		M = UMaterialInstanceDynamic::Create(Street, this);
		const FLinearColor C = Ecx::Srgb(Colors[FMath::Clamp(Kind, 0, 7)]);
		M->SetVectorParameterValue(TEXT("BaseColor"), C);
		M->SetScalarParameterValue(TEXT("Roughness"), 0.7f);
		M->SetScalarParameterValue(TEXT("Pattern"), Patterns[FMath::Clamp(Kind, 0, 7)]);
		M->SetScalarParameterValue(TEXT("Wet"), Kind == 3 || Kind == 5 ? 0.4f : 0.75f);
	}
	else
	{
		return Mat(Key, Colors[FMath::Clamp(Kind, 0, 7)], 0.45f);
	}
	Materials.Add(Key, M);
	return M;
}

UMaterialInstanceDynamic* AEmbercrestStage::SnowMat(bool bWorldUV)
{
	const FName Key(TEXT("Snow"));
	if (TObjectPtr<UMaterialInstanceDynamic>* Found = Materials.Find(Key))
	{
		return *Found;
	}
	UMaterialInstanceDynamic* M = nullptr;
	if (UMaterialInterface* Parent = Ecx::LoadMat(TEXT("M_ECX_Snow")))
	{
		M = UMaterialInstanceDynamic::Create(Parent, this);
		HourMids.Add(M);
		Materials.Add(Key, M);
	}
	else
	{
		M = Mat(Key, Ecx::SnowHex, 0.62f);
	}
	SnowMids.AddUnique(M);
	return M;
}

UMaterialInstanceDynamic* AEmbercrestStage::GlowMat(uint8 Family, float Scale)
{
	const FName Key(*FString::Printf(TEXT("Glow%d_%.3f"), Family, Scale));
	if (TObjectPtr<UMaterialInstanceDynamic>* Found = Materials.Find(Key))
	{
		return *Found;
	}
	UMaterialInterface* Parent = Ecx::LoadMat(TEXT("M_Surface"));
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Parent ? Parent : FallbackMaterial.Get(), this);
	M->SetScalarParameterValue(TEXT("Roughness"), 0.35f);
	M->SetScalarParameterValue(TEXT("Metallic"), 0.0f);
	Materials.Add(Key, M);
	FEcxGlow G;
	G.Mid = M;
	G.Family = Family;
	G.Scale = Scale;
	Glows.Add(G);
	AppliedHour = -100.0; // the new glow takes the hour's color when it's next applied
	return M;
}

UMaterialInterface* AEmbercrestStage::GlassMat(bool bTint)
{
	const FName Key(bTint ? TEXT("GlassTint") : TEXT("Glass"));
	if (TObjectPtr<UMaterialInstanceDynamic>* Found = Materials.Find(Key))
	{
		return *Found;
	}
	UMaterialInterface* Parent = Ecx::LoadMat(bTint ? TEXT("M_ECX_GlassTint") : TEXT("M_ECX_Glass"));
	if (!Parent)
	{
		Parent = Ecx::LoadMat(TEXT("M_ECX_Glass"));
	}
	if (Parent)
	{
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Parent, this);
		Materials.Add(Key, M);
		return M;
	}
	if (UMaterialInterface* Street = Ecx::LoadMat(TEXT("M_StreetGlass")))
	{
		// The Lucky Penny's glass, dry: clean panes.
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Street, this);
		M->SetScalarParameterValue(TEXT("Wet"), 0.0f);
		M->SetVectorParameterValue(TEXT("Tint"), bTint ? FLinearColor(0.008f, 0.009f, 0.01f) : FLinearColor(0.02f, 0.025f, 0.03f));
		Materials.Add(Key, M);
		return M;
	}
	return Mat(Key, 0x0b0e12, 0.04f, 0.2f);
}

UMaterialInstanceDynamic* AEmbercrestStage::ScreenMat(FName Key, bool bLocalUV)
{
	if (TObjectPtr<UMaterialInstanceDynamic>* Found = Materials.Find(Key))
	{
		return *Found;
	}
	UMaterialInterface* Parent = Ecx::LoadMat(TEXT("M_ECX_Screen"));
	const bool bOwn = Parent != nullptr;
	if (!Parent)
	{
		Parent = Ecx::LoadMat(TEXT("M_ScreenImage"));
	}
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Parent ? Parent : FallbackMaterial.Get(), this);
	if (bOwn)
	{
		M->SetScalarParameterValue(TEXT("LocalUV"), bLocalUV ? 1.0f : 0.0f);
		M->SetScalarParameterValue(TEXT("Mix"), 0.0f);
	}
	M->SetScalarParameterValue(TEXT("Strength"), 300.0f);
	Materials.Add(Key, M);
	return M;
}

void AEmbercrestStage::GlowAll(UStaticMeshComponent* C, uint8 Family, float Scale)
{
	if (!C)
	{
		return;
	}
	UMaterialInstanceDynamic* M = GlowMat(Family, Scale);
	for (int32 Slot = 0; Slot < C->GetNumMaterials(); ++Slot)
	{
		C->SetMaterial(Slot, M);
	}
	// Light sources: they cast no shadow of their own and don't block the light they give.
	C->SetCastShadow(false);
	C->bAffectDistanceFieldLighting = false;
}

int32 AEmbercrestStage::SlotOf(const UStaticMeshComponent* C, int32 Index, const TCHAR* Name)
{
	if (!C)
	{
		return INDEX_NONE;
	}
	const TArray<FName> Names = C->GetMaterialSlotNames();
	if (Name)
	{
		// Unbaked slots carry their Blender material's name ("ecx_paver").
		const FString Want(Name);
		for (int32 Slot = 0; Slot < Names.Num(); ++Slot)
		{
			if (Names[Slot].ToString().Contains(Want, ESearchCase::IgnoreCase))
			{
				return Slot;
			}
		}
		for (int32 Slot = 0; Slot < C->GetNumMaterials(); ++Slot)
		{
			const UMaterialInterface* M = C->GetMaterial(Slot);
			if (M && M->GetName().Contains(Want, ESearchCase::IgnoreCase))
			{
				return Slot;
			}
		}
	}
	if (Index < 0)
	{
		return INDEX_NONE;
	}
	// Baked slots are M_<Mesh>_<index> (the only slot has no suffix).
	const int32 Slot = ImportedSlot(C, Index);
	return Slot >= 0 && Slot < C->GetNumMaterials() ? Slot : INDEX_NONE;
}

void AEmbercrestStage::SetSlot(UStaticMeshComponent* C, int32 Slot, UMaterialInterface* M)
{
	if (C && M && Slot >= 0 && Slot < C->GetNumMaterials())
	{
		C->SetMaterial(Slot, M);
		if (SnowMids.Contains(M))
		{
			SnowParts.AddUnique(C);
		}
	}
}

namespace
{
/** A lamp's share of the volumetric fog: a full share floods the plaza amber at night (112 warm lights). */
constexpr float VolumetricShare = 0.25f;
}

void AEmbercrestStage::Lit(UStaticMeshComponent* C, std::initializer_list<float> Floors, float Scale)
{
	if (!C)
	{
		return;
	}
	FEcxLit L;
	L.Mesh = C;
	L.Scale = Scale;
	L.Floors.Init(-1.0f, C->GetNumMaterials());
	int32 Index = 0;
	for (const float Floor : Floors)
	{
		if (Floor >= 0.0f)
		{
			const int32 Slot = SlotOf(C, Index);
			if (L.Floors.IsValidIndex(Slot))
			{
				L.Floors[Slot] = Floor;
			}
		}
		++Index;
	}
	LitMeshes.Add(L);
}

void AEmbercrestStage::AddLight(ULightComponent* L, uint8 Rule, float Level)
{
	if (!L)
	{
		return;
	}
	FEcxLight E;
	E.Light = L;
	E.Level = Level;
	E.Rule = Rule;
	Lights.Add(E);
}

USpotLightComponent* AEmbercrestStage::SpotAt(uint8 Rule, float Candela, const FVector& At, const FRotator& Facing, float Kelvin, uint32 ColorHex, float Attenuation, float Inner,
	float Outer, bool bShadows, float Volumetric, USceneComponent* Parent)
{
	USpotLightComponent* L = NewPart<USpotLightComponent>(Parent);
	L->SetRelativeLocationAndRotation(At, Facing);
	L->SetIntensityUnits(ELightUnits::Candelas);
	L->SetIntensity(Candela);
	L->SetAttenuationRadius(Attenuation);
	L->SetInnerConeAngle(Inner);
	L->SetOuterConeAngle(Outer);
	if (Kelvin > 0.0f)
	{
		L->SetUseTemperature(true);
		L->SetTemperature(Kelvin);
	}
	else
	{
		L->SetLightColor(Ecx::Srgb(ColorHex));
	}
	L->SetCastShadows(bShadows && CVarLightShadows.GetValueOnGameThread() != 0);
	L->SetVolumetricScatteringIntensity(Volumetric * VolumetricShare); // the fog only hints at a lamp
	L->SetCastVolumetricShadow(false);
	AddLight(L, Rule, Candela);
	return L;
}

URectLightComponent* AEmbercrestStage::RectAt(uint8 Rule, float Candela, const FVector& At, const FRotator& Facing, float Kelvin, uint32 ColorHex, float Attenuation, float Width,
	float Height, bool bShadows, float Volumetric, USceneComponent* Parent)
{
	URectLightComponent* L = NewPart<URectLightComponent>(Parent);
	L->SetRelativeLocationAndRotation(At, Facing);
	L->SetIntensityUnits(ELightUnits::Candelas);
	L->SetIntensity(Candela);
	L->SetAttenuationRadius(Attenuation);
	L->SetSourceWidth(Width);
	L->SetSourceHeight(Height);
	if (Kelvin > 0.0f)
	{
		L->SetUseTemperature(true);
		L->SetTemperature(Kelvin);
	}
	else
	{
		L->SetLightColor(Ecx::Srgb(ColorHex));
	}
	L->SetCastShadows(bShadows && CVarLightShadows.GetValueOnGameThread() != 0);
	L->SetVolumetricScatteringIntensity(Volumetric * VolumetricShare); // the fog only hints at a lamp
	AddLight(L, Rule, Candela);
	return L;
}

UPointLightComponent* AEmbercrestStage::PointAt(uint8 Rule, float Candela, const FVector& At, float Kelvin, uint32 ColorHex, float Attenuation, float SourceRadius, bool bShadows,
	float Volumetric, USceneComponent* Parent)
{
	UPointLightComponent* L = NewPart<UPointLightComponent>(Parent);
	L->SetRelativeLocation(At);
	L->SetIntensityUnits(ELightUnits::Candelas);
	L->SetIntensity(Candela);
	L->SetAttenuationRadius(Attenuation);
	L->SetSourceRadius(SourceRadius);
	if (Kelvin > 0.0f)
	{
		L->SetUseTemperature(true);
		L->SetTemperature(Kelvin);
	}
	else
	{
		L->SetLightColor(Ecx::Srgb(ColorHex));
	}
	L->SetCastShadows(bShadows && CVarLightShadows.GetValueOnGameThread() != 0);
	L->SetVolumetricScatteringIntensity(Volumetric * VolumetricShare); // the fog only hints at a lamp
	L->SetCastVolumetricShadow(false);
	AddLight(L, Rule, Candela);
	return L;
}

// ------------------------------------------------------------------ the flows' places

void AEmbercrestStage::BuildSpots()
{
	Spots.Reset();
	auto Add = [this](const TCHAR* Id, const FVector& At, float Radius, const TCHAR* Prompt, EEcxWhen When) {
		FEcxSpot S;
		S.Id = Id;
		S.At = At;
		S.Radius = Radius;
		S.Prompt = Prompt;
		S.When = When;
		Spots.Add(S);
	};
	Add(TEXT("doors"), FVector(150.0, 0.0, 100.0), 200.0f, TEXT("Go inside"), EEcxWhen::Arriving);
	Add(TEXT("doors_s"), FVector(120.0, -355.0, 100.0), 140.0f, TEXT("Go inside"), EEcxWhen::Arriving);
	Add(TEXT("doors_n"), FVector(120.0, 355.0, 100.0), 140.0f, TEXT("Go inside"), EEcxWhen::Arriving);
	Add(TEXT("bus"), FVector(5200.0, 100.0, -125.0), 180.0f, TEXT("Take the 14 home"), EEcxWhen::LeavingBusOpen);
	Add(TEXT("overlook"), FVector(6800.0, 1300.0, -125.0), 250.0f, TEXT("Look out over the valley"), EEcxWhen::Always);
	Add(TEXT("viewer"), FVector(6880.0, 900.0, -125.0), 120.0f, TEXT("Use the viewer"), EEcxWhen::Always);
}

FTransform AEmbercrestStage::SpawnAt(EEcxSpawn Where) const
{
	// The floor plus 100 (the hero settles the last few centimeters).
	FTransform Local;
	switch (Where)
	{
	case EEcxSpawn::ArriveInside:
		Local = FTransform(FRotator(0.0f, 180.0f, 0.0f), FVector(5340.0, 100.0, -204.0 + 100.0));
		break;
	case EEcxSpawn::ArrivePlatform:
		Local = FTransform(FRotator(0.0f, 180.0f, 0.0f), FVector(5140.0, 100.0, Ecx::ZPlatform + 100.0));
		break;
	default:
		// Out past the drum's canopy edge, so the camera starts behind the hero, not inside the glass.
		Local = FTransform(FRotator(0.0f, 0.0f, 0.0f), FVector(460.0, 0.0, Ecx::ZApron + 100.0));
		break;
	}
	return Local * GetActorTransform();
}

const FEcxSpot* AEmbercrestStage::SpotFor(const FVector& Eye, const FVector& Dir, bool bLeaving, bool bBusOpen) const
{
	const FTransform& T = GetActorTransform();
	const FEcxSpot* Best = nullptr;
	double BestScore = -1.0;
	for (const FEcxSpot& S : Spots)
	{
		const bool bNow = S.When == EEcxWhen::Always || (S.When == EEcxWhen::Arriving && !bLeaving) || (S.When == EEcxWhen::Leaving && bLeaving) ||
			(S.When == EEcxWhen::LeavingBusOpen && bLeaving && bBusOpen);
		if (!bNow)
		{
			continue;
		}
		const FVector At = T.TransformPosition(S.At);
		const FVector To = At - Eye;
		const double Flat = FVector(To.X, To.Y, 0.0).Size();
		if (Flat > S.Radius + 80.0 || FMath::Abs(To.Z) > 260.0)
		{
			continue;
		}
		const double Facing = FVector::DotProduct(FVector(To.X, To.Y, 0.0).GetSafeNormal(), FVector(Dir.X, Dir.Y, 0.0).GetSafeNormal());
		if (Facing < 0.1 && Flat > 70.0)
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

const FEcxSpot* AEmbercrestStage::DoorSpotNear(const FVector& At) const
{
	const FVector L = GetActorTransform().InverseTransformPosition(At);
	for (const FEcxSpot& S : Spots)
	{
		if (S.Id.ToString().StartsWith(TEXT("doors")) && FVector(L.X - S.At.X, L.Y - S.At.Y, 0.0).Size() < S.Radius)
		{
			return &S;
		}
	}
	return nullptr;
}

FString AEmbercrestStage::PlaceName(const FVector& At) const
{
	const FVector L = GetActorTransform().InverseTransformPosition(At);
	if (L.X > Ecx::XParapet && L.Y > 555.0 && L.Y < 2045.0)
	{
		return TEXT("THE OVERLOOK");
	}
	if (L.X > 4700.0 && L.X < Ecx::XBay1 && L.Y > -2400.0 && L.Y < 1400.0)
	{
		return TEXT("ROUTE 14 \xB7 EMBERCREST STOP");
	}
	return TEXT("THE EMBERCREST");
}

bool AEmbercrestStage::IsOffTheSet(const FVector& At) const
{
	const FVector L = GetActorTransform().InverseTransformPosition(At);
	if (L.Z < -1500.0)
	{
		return true;
	}
	const FVector2D P(L.X / 100.0, L.Y / 100.0);
	return !InsideBounds(P) && DistanceToBounds(P) > 10.0;
}

int32 AEmbercrestStage::NearestDoor(const FVector& At, FVector& OutApproach) const
{
	const FVector L = GetActorTransform().InverseTransformPosition(At);
	// The revolving door's middle, or a swing leaf's (each 105 cm from its hinge).
	int32 Best = 0;
	double BestD = FMath::Abs(L.Y);
	FVector Target(-40.0, 0.0, L.Z);
	for (int32 K = 0; K < 4; ++K)
	{
		// A leaf built as modeled runs +Y from its hinge, a mirrored one -Y.
		const double LeafMid = Ecx::SwingHingeY[K] + (Ecx::SwingMirror[K] > 0.0 ? 52.5 : -52.5);
		const double D = FMath::Abs(L.Y - LeafMid);
		if (D < BestD - 1.0 && FMath::Abs(L.Y) > 170.0)
		{
			BestD = D;
			Best = K + 1;
			Target = FVector(-40.0, LeafMid, L.Z);
		}
	}
	OutApproach = GetActorTransform().TransformPosition(Target);
	return Best;
}

void AEmbercrestStage::SetRevolvingFast(bool bFast)
{
	// 2 rpm all night, 4 while someone goes through.
	RevTarget = bFast ? 24.0f : 12.0f;
}

void AEmbercrestStage::OpenSwingDoor(int32 Door)
{
	const int32 K = Door - 1;
	if (SwingTarget.IsValidIndex(K))
	{
		SwingTarget[K] = 1.0f;
	}
}

void AEmbercrestStage::UpdateDoors(float Dt)
{
	RevSpeed = FMath::FInterpTo(RevSpeed, RevTarget, Dt, 1.2f);
	RevAngle = FMath::Fmod(RevAngle + RevSpeed * Dt, 360.0f);
	if (RevWings)
	{
		RevWings->SetRelativeRotation(FRotator(0.0f, 180.0f + RevAngle, 0.0f));
	}
	for (int32 K = 0; K < SwingLeaves.Num() && K < SwingOpen.Num(); ++K)
	{
		const float Was = SwingOpen[K];
		SwingOpen[K] = FMath::FInterpTo(SwingOpen[K], SwingTarget[K], Dt, 3.0f);
		if (SwingLeaves[K] && !FMath::IsNearlyEqual(Was, SwingOpen[K], 1e-4f))
		{
			// Outward (+X): a leaf built along +Y turns clockwise from above, a mirrored one the other way.
			SwingLeaves[K]->SetRelativeRotation(FRotator(0.0f, SwingSign[K] * 80.0f * FMath::SmoothStep(0.0f, 1.0f, SwingOpen[K]), 0.0f));
		}
	}
}

// ------------------------------------------------------------------ cameras

void AEmbercrestStage::SetCineCamera(const FVector& Location, const FVector& LookAt, float HorizontalFov)
{
	if (!CineCam)
	{
		return;
	}
	const FTransform& T = GetActorTransform();
	const FVector W = T.TransformPosition(Location);
	const FVector Target = T.TransformPosition(LookAt);
	CineCam->SetWorldLocationAndRotation(W, (Target - W).Rotation());
	CineCam->SetFieldOfView(HorizontalFov);
}

bool AEmbercrestStage::ShotFor(int32 Shot, FVector& OutLocation, FVector& OutLookAt, float& OutFov)
{
	switch (Shot)
	{
	case 3: // the overlook: the city band, the mist, the switchback lamps, the moon
		OutLocation = FVector(6900.0, 1300.0, -60.0);
		OutLookAt = FVector(1000000.0, -100000.0, -90000.0);
		OutFov = 60.0f;
		return true;
	case 4: // the crane: the whole resort, the crown, the massif
		OutLocation = FVector(30000.0, -10000.0, 3000.0);
		OutLookAt = FVector(0.0, 0.0, 6300.0);
		OutFov = 65.0f;
		return true;
	case 5: // mid-walk: the fire bowls in the water, the steam, the soffit and the letters, the tower
		OutLocation = FVector(3000.0, -250.0, 90.0);
		OutLookAt = FVector(0.0, 0.0, 900.0);
		OutFov = 72.0f;
		return true;
	case 6: // low fire: the flame, the bronze, the wet reflections, the snow on the copings
		OutLocation = FVector(4300.0, 650.0, -110.0);
		OutLookAt = FVector(4100.0, 975.0, -20.0);
		OutFov = 50.0f;
		return true;
	default:
		return false;
	}
}

// ------------------------------------------------------------------ for the sound

float AEmbercrestStage::FireNear(const FVector& Ear) const
{
	const FVector L = GetActorTransform().InverseTransformPosition(Ear);
	float Best = 0.0f;
	for (const FVector& B : Ecx::FireBowls)
	{
		Best = FMath::Max(Best, SoundFalloff(FVector::Dist(L, B + FVector(0.0, 0.0, 160.0)), 150.0, 1200.0));
	}
	return Best;
}

float AEmbercrestStage::FireFlicker(const FVector& Ear) const
{
	const FVector L = GetActorTransform().InverseTransformPosition(Ear);
	float Flicker = 1.0f;
	double Near = TNumericLimits<double>::Max();
	for (int32 I = 0; I < Fires.Num(); ++I)
	{
		const double D = FVector::DistSquared(L, Fires[I].At);
		if (D < Near)
		{
			Near = D;
			const uint32 S = static_cast<uint32>(Fires[I].Seed);
			Flicker = 0.78f + 0.22f * Ecx::Noise1(Clock * 2.0, S) + 0.08f * Ecx::Noise1(Clock * 9.0, S + 17u);
		}
	}
	return Flicker;
}

float AEmbercrestStage::WaterNear(const FVector& Ear) const
{
	const FVector L = GetActorTransform().InverseTransformPosition(Ear);
	const FVector2D P(L.X, FMath::Abs(L.Y));
	return SoundFalloff(DistanceToRect(P, FVector2D(2520.0, 460.0), FVector2D(4640.0, 1490.0)), 100.0, 1500.0);
}

float AEmbercrestStage::DoorsNear(const FVector& Ear) const
{
	const FVector L = GetActorTransform().InverseTransformPosition(Ear);
	return SoundFalloff(FVector::Dist(L, FVector(0.0, 0.0, 150.0)), 150.0, 1100.0);
}

float AEmbercrestStage::BusNear(const FVector& Ear) const
{
	if (BusPhase == EEcxBusPhase::Gone || !BusRoot)
	{
		return 0.0f;
	}
	return SoundFalloff(FVector::Dist(Ear, BusRoot->GetComponentLocation() + FVector(0.0, 0.0, 120.0)), 400.0, 3200.0);
}

float AEmbercrestStage::ValleyNear(const FVector& Ear) const
{
	const FVector L = GetActorTransform().InverseTransformPosition(Ear);
	return static_cast<float>(FMath::Clamp((L.X - 4500.0) / 2200.0, 0.0, 1.0));
}

bool AEmbercrestStage::IsSnow(const FHitResult& Hit) const
{
	UPrimitiveComponent* C = Hit.GetComponent();
	if (!C)
	{
		return false;
	}
	if (C == TerrainMesh.Get() || C->ComponentHasTag(TEXT("Snow")))
	{
		return true;
	}
	if (C == GroundMesh.Get() && Hit.FaceIndex >= 0)
	{
		int32 Section = 0;
		const UMaterialInterface* M = C->GetMaterialFromCollisionFaceIndex(Hit.FaceIndex, Section);
		return M && SnowMids.Contains(M);
	}
	return false;
}

FString AEmbercrestStage::Describe() const
{
	static const TCHAR* const Phases[4] = {TEXT("gone"), TEXT("arriving"), TEXT("stopped"), TEXT("departing")};
	return FString::Printf(TEXT("hour %.2f | sun %.1f deg | EV %.1f..%.1f | bus %s %.1fs doors %.2f | revolving %.0f deg/s | %d lights"), Hour, SunEl, EvMin, EvMax,
		Phases[FMath::Clamp(static_cast<int32>(BusPhase), 0, 3)], BusClock, BusDoor, RevSpeed, Lights.Num());
}

// ------------------------------------------------------------------ play

void AEmbercrestStage::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bBuilt)
	{
		return;
	}
	const float Dt = FMath::Min(DeltaSeconds, 0.1f);
	Clock += Dt;
	UpdateBus(Dt);
	UpdateDoors(Dt);
	UpdateFires(Dt);
	UpdateScreens(Dt);
	const UWorld* World = GetWorld();
	const APlayerController* Pc = World ? World->GetFirstPlayerController() : nullptr;
	if (const APlayerCameraManager* Cam = Pc ? Pc->PlayerCameraManager.Get() : nullptr)
	{
		UpdateSteam(Cam->GetCameraLocation(), Dt);
	}
}
