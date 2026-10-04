#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShortStack/UI/Canvas.h"

#include "StreetStage.generated.h"

class SDrawListWidget;
class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class UInstancedStaticMeshComponent;
class ULightComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPostProcessComponent;
class URectLightComponent;
class USkyLightComponent;
class USpotLightComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UWidgetComponent;

/** A place on the street the player can use: walk up, the prompt shows, E does it. */
USTRUCT()
struct FStreetSpot
{
	GENERATED_BODY()

	UPROPERTY()
	FName Id;
	UPROPERTY()
	FVector At = FVector::ZeroVector;
	UPROPERTY()
	float Radius = 120.0f;
	UPROPERTY()
	FString Prompt;
	/** The store shelf it opens the counter on (-1: not a shelf). */
	UPROPERTY()
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
 *
 * Everything the stage uses while playing is a plain UPROPERTY (not transient): a cooked build, and Play
 * started on the Street map, use the components the editor saved instead of running construction again.
 * The look follows the camera each frame after it has moved (TG_PostUpdateWork): the rain sheets, the
 * exposure (the street kept dark, the store's own brighter range inside it) and which streetlights shadow.
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
	/** Where Benny stands behind the counter, facing the customers (his feet on the store's floor). */
	FTransform ClerkSpot() const;
	/** The top of the store's floor (world Z). */
	double StoreFloorZ() const;
	/** The spot within reach of an eye looking along Dir (the nearest it's facing), or null. */
	const FStreetSpot* SpotFor(const FVector& Eye, const FVector& Dir) const;
	/** "LUCKY PENNY #212" inside the store, "FIFTH STREET" outside it. */
	FString PlaceName(const FVector& At) const;
	bool IsInsideStore(const FVector& At) const;
	/** Somewhere nobody should be: under the street (fell through a gap) or far past its ends. */
	bool IsOffTheSet(const FVector& At) const;
	/** The store's doors slide open for someone within reach of them. True the moment they start to open. */
	bool UpdateDoors(const FVector& Someone, float DeltaSeconds);
	/** 0 night .. 1 full day (Session::Daylight): the sky, the lamps and windows, the exposure. */
	void SetDaylight(float InDaylight);
	/** The player's image settings: Brightness in stops, film grain and fringe as multiples, motion blur. */
	void SetLensOptions(float InBrightnessBias, float InGrainScale, float InFringeScale, bool bInMotionBlur);
	/** How far inside the store the camera is (0 out on the street .. 1 inside), eased. */
	float CameraInside() const { return Inside; }

	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	float ExposureBias = 0.0f;
	/** How wet the street is (0 dry .. 1 puddles everywhere). */
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	float Wetness = 0.85f;
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	float StreetlightCandela = 1400.0f;
	/** How many streetlights cast shadows at once (the nearest to the camera); the rest light without them. */
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	int32 ShadowedStreetlights = 2;

private:
	void BuildSet();
	void BuildGround();
	void BuildEnds();
	void BuildHomeBlock();
	void BuildEntrance();
	void BuildAcross();
	void BuildStore();
	void BuildStoreDressing();
	void BuildStreetlights();
	void BuildSkyAndWeather();
	void BuildSpots();
	void AttachSlate();
	/** The parked sedans in their paint (again when play begins: the tint isn't saved with the level, see PaintCars). */
	void PaintCars();

	/** The rain sheets in front of the camera (sized to the view), and which of them may show. */
	void FollowCamera(const FVector& Location, const FRotator& Rotation, float Fov);
	/** Exposure, the lens, which lamps shadow and light the rain: from where the camera is. */
	void UpdateLook(const FVector& Camera, float Dt);
	void ApplyLens();
	void PickStreetlights(const FVector& Camera);
	void ApplyDaylight();

	template <typename T>
	T* NewPart(USceneComponent* Parent = nullptr);
	UMaterialInstanceDynamic* Mat(FName Key, uint32 SrgbHex, float Roughness, float Pattern = 0.0f, float Emissive = 0.0f, float Metallic = 0.0f);
	UStaticMeshComponent* Box(UMaterialInterface* Material, const FVector& Min, const FVector& Max, bool bCollide = true, USceneComponent* Parent = nullptr);
	UStaticMeshComponent* Cyl(UMaterialInterface* Material, const FVector& Base, float Radius, float Height, bool bCollide = true);
	/** An imported Blender prop (nullptr when it isn't), its origin at At and its front facing Yaw (degrees, 0 is +X). */
	UStaticMeshComponent* Prop(const TCHAR* Name, const FVector& At, float Yaw, const FVector& Scale = FVector(1.0));
	/**
	 * A flat sign drawn by AttachSlate. Art names what it shows ("number:1812", "street:FIFTH ST:1800", "promo:roller-dog",
	 * "menu", see AttachSlate).
	 * Glow > 0: it shines by itself out on the street (the tint, kept against the street's exposure); < 0: it shines
	 * inside the store (a fixed tint for the store's exposure); 0: printed, lit by the lights around it.
	 */
	UWidgetComponent* Sign(const FString& Art, const FVector& At, float Yaw, const FVector2D& SizeCm, const FIntPoint& Pixels, float Glow, bool bTwoSided = false, USceneComponent* Parent = nullptr);
	/** One window on a facade: dark, lit warm, cool or by a TV (Kind 0..3), its glass facing along +X (Yaw 0) or as turned. */
	void Window(const FVector& Center, float Yaw, const FVector2D& SizeCm, int32 Kind, float Seed, float Level);
	/**
	 * A facade's windows, floor over floor from FirstZ to Top, every Pitch cm from Origin along Along (Length cm), most
	 * dark and LitShare of them lit (warm, cool, a TV), the same each time for the same Seed.
	 */
	void WindowRows(const FVector& Origin, const FVector& Along, double Length, float Yaw, double FirstZ, double Top, const FVector2D& Size, double Pitch, double FloorH, double LitShare, int64 Seed);

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

	// Only while building.
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> StreetMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> SurfaceMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> GlowMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> GlassMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> StoreGlassMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> SkyMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> CityMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> RainMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> WindowMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> WidgetLitMaterial;
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UMaterialInstanceDynamic>> Materials;
	/** The windows being laid out: one instanced mesh (M_StreetWindow) or one per look (the fallback). */
	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<UInstancedStaticMeshComponent>> WindowSets;

	// Used while playing (kept in the saved level, see the class comment).
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> SkyMids;
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> WindowMids;
	/** The street lamps' glass and the neon: dark by day. */
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> NightGlowMids;
	UPROPERTY()
	TArray<float> NightGlowLevels;
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> RainSheets;
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> RainMids;
	/** The rain sheets' material knows where the store is (no rain falls inside); the old one doesn't. */
	UPROPERTY()
	bool bRainMasksStore = false;
	/** The sliding doors: each a scene component holding its glass, frame and push bar (UpdateDoors moves it). */
	UPROPERTY()
	TObjectPtr<USceneComponent> DoorLeft;
	UPROPERTY()
	TObjectPtr<USceneComponent> DoorRight;
	/** The hours sticker: attached to the left door, so it slides with it. */
	UPROPERTY()
	TObjectPtr<UWidgetComponent> DoorDecal;
	UPROPERTY()
	TObjectPtr<USkyLightComponent> SkyLight;
	UPROPERTY()
	TObjectPtr<UDirectionalLightComponent> Moon;
	UPROPERTY()
	TObjectPtr<UExponentialHeightFogComponent> Fog;
	UPROPERTY()
	TObjectPtr<UPostProcessComponent> Lens;
	UPROPERTY()
	TArray<TObjectPtr<USpotLightComponent>> Streetlights;
	/** The store's ceiling lights: shadowed while the camera is near the store, so none of their light gets through its walls. */
	UPROPERTY()
	TArray<TObjectPtr<URectLightComponent>> StoreLights;
	/** Lights that only shine at night (the door lamp, the neon's glow on the street). */
	UPROPERTY()
	TArray<TObjectPtr<ULightComponent>> NightLights;
	UPROPERTY()
	TArray<float> NightLightLevels;
	UPROPERTY()
	TArray<TObjectPtr<UWidgetComponent>> SignWidgets;
	UPROPERTY()
	TArray<FString> SignArt;
	UPROPERTY()
	TArray<float> SignGlow;
	UPROPERTY()
	TArray<FStreetSpot> Spots;
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> ParkedCars;
	UPROPERTY()
	TArray<FLinearColor> ParkedPaints;

	TArray<TSharedPtr<SDrawListWidget>> SignSlates;
	TArray<TSharedPtr<const ss::ui::DrawList>> SignLists;
	float DoorOpen = 0.0f;
	bool bDoorsNear = false;
	float Daylight = 0.0f;
	float AppliedDaylight = -1.0f;
	float CapturedDaylight = -1.0f;
	float Inside = 0.0f;
	float PickIn = 0.0f;
	float SignScale = -1.0f;
	float RainScale = -1.0f;
	float RainPixelAngle = -1.0f;
	float BrightnessBias = 0.0f;
	float GrainScale = 1.0f;
	float FringeScale = 1.0f;
	bool bMotionBlur = false;
};
