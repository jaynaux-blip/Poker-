#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "StreetAudio.h"
#include "StreetNs.h"

#include "MarketStage.generated.h"

class AStreetStage;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class URectLightComponent;
class USpotLightComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * Market St east of the corner of Fifth, and Cardinal Ave across its end (world realism spec 2.3, 2.6, 2.8, 7.5.6,
 * ue-world): the whole Market ground (SM_NS_Ground_Market) and Cardinal's, Market's plots on the building kit (M-S1 the
 * pharmacy to M-S6 the vacant corner on the south side, M-N2 Halden & Sons to M-N7 Cardinal Furniture on the north;
 * the thrift's, the pawn's and the gym's openings left for their shop stages, which this stage spawns), the Northside
 * Branch Library closing the view down Market, the proxies standing in for Cardinal's far side and lining it out to 120 m,
 * Market's eight LED lamps and Cardinal's two, the 14's shelter, flags and bench, the trees, the furniture and the parked
 * cars; its zones, walkways, starts, stations and the shelter's use, into the street's registry. Every kit mesh has an
 * engine-shape stand-in in the same plan.
 *
 * It owns Market's buildings east of MarketSplitS (X -1400) on the south side and MarketSplitN (X -1500) on the north
 * (Ruby's corner and the store are AStreetStage's), and is built only at Market's new width: the crowd must walk the
 * registry's walkways first (spec 6.8; AStreetStage::WideMarketWanted). In AStreetStage's frame (stage cm). Built at play
 * (BeginPlay, or FindOrSpawn from the street); in the editor its construction builds a preview, nothing of it saved.
 */
UCLASS()
class SHORTSTACK_API AMarketStage : public AActor
{
	GENERATED_BODY()

public:
	AMarketStage();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/** Market's stage for the street at StageFrame (one already in the level, or a new one), built. */
	static AMarketStage* FindOrSpawn(UWorld* World, const FTransform& StageFrame, AStreetStage* InStreet);
	/** Builds Market once for play (at its new width only) and registers it. */
	void EnsureBuilt();
	bool IsBuilt() const { return bBuilt; }
	/** Every asset path it loads (its part of the street's preload list). */
	static void AppendPreload(TArray<FString>& Paths);

	/** Each frame from the street: the look's world minutes and its glow levels (its shops by their hours, its lamps' photocells). */
	void SetLook(double InWorldMinutes, const Ns::FGlowLevels& InLevels, float Dt);
	/** Its lamps (the street's rain lights itself from the nearest, and hands out their fog shadows). */
	const TArray<TObjectPtr<USpotLightComponent>>& StreetLamps() const { return Lamps; }
	/** Its positioned sounds near Camera (world): the signs' hum at night, AC units, downspouts and the shelter's roof in rain. */
	void AppendEmitters(TArray<FStreetEmitter>& Out, const FVector& Camera, float Rain, float Night) const;
	/** One line: plots placed, shops spawned, lamps, cars. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString Describe() const;

	UPROPERTY(EditAnywhere, Category = "Short Stack")
	bool bEditorPreview = true;

private:
	void BuildAll();
	void BuildGround();
	void BuildPlots();
	void BuildPlot(int32 Index);
	void BuildCardinal();
	void BuildLamps();
	void BuildStops();
	void BuildFurniture();
	void BuildParked();
	void SpawnShops();
	void Register();
	void Unregister();
	void Clear();

	template <typename T>
	T* NewPart(USceneComponent* Parent = nullptr);
	UMaterialInstanceDynamic* Mat(FName Key, uint32 SrgbHex, float Roughness, float Pattern = 0.0f, float Emissive = 0.0f, float Metallic = 0.0f);
	UStaticMeshComponent* Box(UMaterialInterface* Material, const FVector& Min, const FVector& Max, bool bCollide = true, USceneComponent* Parent = nullptr);
	/** A SITE mesh at the stage's origin turned 180 (nullptr when it isn't imported). */
	UStaticMeshComponent* Site(const TCHAR* Name);
	/** A PROP mesh of the NS kit at At facing stage yaw Yaw (nullptr when it isn't imported). */
	UStaticMeshComponent* Prop(const TCHAR* Name, const FVector& At, float Yaw, const FVector& Scale = FVector(1.0));
	void Blocker(const FVector& Min, const FVector& Max);
	/** A stand-in block with a few rows of windows on its street faces. */
	void StandIn(const FVector& Min, const FVector& Max, uint32 Brick, bool bFacesX, bool bFacesY);
	void Proxy(const FVector& At, float Yaw, double Width, int32 Seed);
	void ProxyRun(const FVector& A, const FVector& B, float Yaw, int32 Seed);
	URectLightComponent* Spill(const FVector& At, float Yaw, float Width, uint32 Hex, float Lumens, int32 Shop);
	int32 AddGlow(UStaticMeshComponent* Mesh, Ns::EGlow Family, uint32 Hex, float Scale, int32 Shop);
	void Decal(int32 Cell, const FVector& At, float Yaw, const FVector2D& Size);
	void Footprint(int32 Walk, double X, double Y, double HalfX, double HalfY);

	UPROPERTY()
	TObjectPtr<USceneComponent> Root;
	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> CylinderMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> SphereMesh;
	UPROPERTY()
	TObjectPtr<UMaterialInterface> FallbackMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> StreetMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> WindowMaterial;
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UMaterialInstanceDynamic>> Materials;
	UPROPERTY(Transient)
	TArray<TObjectPtr<USpotLightComponent>> Lamps;

	struct FSpot
	{
		FVector At = FVector::ZeroVector;
		EStreetEmitter Kind = EStreetEmitter::Hum;
		float Level = 1.0f;
		float Hz = 120.0f;
	};
	TWeakObjectPtr<AStreetStage> Street;
	Ns::FLit Lit;
	/** The [screen] faces core draws: the 14's timetable and the shelter's ad, the awnings' valances, the parked cars' plates. */
	Ns::FScreens Screens;
	Ns::FGlowLevels Levels;
	TArray<FSpot> Emitters;
	/** What stands on Market's south sidewalk (0), its north (1), Cardinal's west (2): the walkways' obstacles (stage cm). */
	TArray<FBox2D> Footprints[3];
	TArray<int32> UseHandles;
	/** The drips off its awnings and the shed (M_NS_RainFx), shown while it rains or the street is wet. */
	TWeakObjectPtr<UInstancedStaticMeshComponent> Drips;
	double WorldMinutes = 0.0;
	float AppliedNight = -1.0f;
	bool bBuilt = false;
	int32 PlotsKit = 0;
	int32 PlotsStandIn = 0;
	int32 ShopsSpawned = 0;
	int32 CarsParked = 0;
};

template <typename T>
T* AMarketStage::NewPart(USceneComponent* Parent)
{
	// Transient, as the street's parts: built when play begins (or for the editor's preview), never saved or duplicated.
	const UWorld* InWorld = GetWorld();
	const EObjectFlags Flags = InWorld && InWorld->IsGameWorld() ? RF_Transient : EObjectFlags(RF_Transient | RF_DuplicateTransient);
	T* Part = NewObject<T>(this, NAME_None, Flags);
	Part->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	Part->SetupAttachment(Parent ? Parent : Root.Get());
	BlueprintCreatedComponents.Add(Part);
	Part->RegisterComponent();
	return Part;
}
