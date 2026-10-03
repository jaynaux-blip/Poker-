#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShortStack/UI/Canvas.h"

#include "StreetStage.generated.h"

class SDrawListWidget;
class UExponentialHeightFogComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPostProcessComponent;
class USkyLightComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UWidgetComponent;

/** A place on the street the player can use: walk up, the prompt shows, E does it. */
struct FStreetSpot
{
	FName Id;
	FVector At = FVector::ZeroVector;
	float Radius = 120.0f;
	FString Prompt;
	/** The store shelf it opens the counter on (-1: not a shelf). */
	int32 Shelf = -1;
};

/**
 * Fifth Street at night, from the apartment building's door to the Lucky Penny #212 on the corner of
 * Market: sidewalks and the wet road, the building fronts on both sides, the Wash & Fold's neon across
 * the street (Dee's game is in its back room), streetlights, rain, and the store itself, lit inside, with
 * sliding doors, coolers, aisles, the counter, the roller grill and the coffee bar. Built procedurally
 * from engine shapes with collision, like the apartment; Blender props (street_setup.py imports them)
 * replace the boxes where they exist.
 *
 * Coordinates (cm): the building fronts on the player's side of the street stand on X = 0, the road
 * runs along Y at X 420..1620, up is Z with the sidewalk's top at 0. The store spans Y 3700..4950.
 */
UCLASS()
class SHORTSTACK_API AStreetStage : public AActor
{
	GENERATED_BODY()

public:
	AStreetStage();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Short Stack")
	void RebuildSet();

	/** Where the player appears: "home" just out of the building's door, "store" inside the Lucky Penny. */
	FTransform StartAt(const FString& From) const;
	/** Where Benny stands behind the counter, facing the customers. */
	FTransform ClerkSpot() const;
	/** The spot within reach of an eye looking along Dir (the nearest it's facing), or null. */
	const FStreetSpot* SpotFor(const FVector& Eye, const FVector& Dir) const;
	/** "LUCKY PENNY #212" inside the store, "FIFTH STREET" outside it. */
	FString PlaceName(const FVector& At) const;
	bool IsInsideStore(const FVector& At) const;
	/** The rain sheets follow the camera. */
	void FollowCamera(const FVector& Location, const FRotator& Rotation);
	/** The store's doors slide open for someone within Reach of them. */
	void UpdateDoors(const FVector& Someone, float DeltaSeconds);

	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	float ExposureBias = 0.4f;
	/** How wet the street is (0 dry .. 1 puddles everywhere). */
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	float Wetness = 0.85f;
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	float StreetlightCandela = 1400.0f;

private:
	void BuildSet();
	void BuildGround();
	void BuildHomeBlock();
	void BuildAcross();
	void BuildStore();
	void BuildStreetlights();
	void BuildSkyAndWeather();
	void AttachSlate();

	template <typename T>
	T* NewPart(USceneComponent* Parent = nullptr);
	UMaterialInstanceDynamic* Mat(FName Key, uint32 SrgbHex, float Roughness, float Pattern = 0.0f, float Emissive = 0.0f, float Metallic = 0.0f);
	UStaticMeshComponent* Box(UMaterialInterface* Material, const FVector& Min, const FVector& Max, bool bCollide = true, USceneComponent* Parent = nullptr);
	UStaticMeshComponent* Cyl(UMaterialInterface* Material, const FVector& Base, float Radius, float Height, bool bCollide = true);
	UStaticMeshComponent* Prop(const TCHAR* Name, const FVector& At, float Yaw, const FVector& Scale = FVector(1.0));
	UWidgetComponent* Sign(const FVector& At, float Yaw, const FVector2D& SizeCm, const FIntPoint& Pixels, bool bLit);

	UPROPERTY()
	TObjectPtr<USceneComponent> Root;
	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> CylinderMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> SphereMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> PlaneMesh;
	UPROPERTY()
	TObjectPtr<UMaterialInterface> FallbackMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> StreetMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> SurfaceMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> GlowMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> GlassMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> SkyMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> CityMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> RainMaterial;
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UMaterialInstanceDynamic>> Materials;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> TimedMaterials;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> RainSheets;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> DoorLeft;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> DoorRight;
	UPROPERTY(Transient)
	TObjectPtr<USkyLightComponent> SkyLight;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidgetComponent>> SignWidgets;

	TArray<FStreetSpot> Spots;
	TArray<TSharedPtr<SDrawListWidget>> SignSlates;
	TArray<TSharedPtr<const ss::ui::DrawList>> SignLists;
	float DoorOpen = 0.0f;
	float Time = 0.0f;
};
