#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShortStack/UI/Canvas.h"
#include "StreetWorld.h"

#include "StreetStage.generated.h"

class SDrawListWidget;
class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class UInstancedStaticMeshComponent;
class ULightComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPointLightComponent;
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

// FStreetLane, FStreetWalkway and FStreetDoor live in StreetWorld.h now (the registry hands them out too).

/**
 * Fifth Street at night, from the apartment building's door to the Lucky Penny #212 on the corner of
 * Market: sidewalks and the wet road, the building fronts on both sides, the tenement across the street
 * with the Wash & Fold at its foot, streetlights, rain, and the store itself, lit inside, with sliding
 * doors, coolers, aisles, the counter, the roller grill and the coffee bar, its walls and ceiling finished
 * as a corner store's are (a clock on the wall keeps the world's time). The Wash & Fold is a room too, three
 * steps up through its door (its bell rings as it swings in): rows of washers, dryers stacked along the back
 * and side walls (the running ones' drums lit and tumbling), folding tables, chairs at the window, the
 * change and soap machines, carts, the attendant's counter, a TV up on the back wall, and Dee's steel door
 * at the back, where her game is. Beside it the tenement's own street door, Dee's, up to her front room.
 * Built procedurally from engine shapes with collision, like the apartment; Blender props (street_setup.py
 * imports them) replace the boxes where they exist: the building fronts (1812 itself, the walk-up, the
 * brownstones, the floors over the store, and across the street the tenement as the apartment's window
 * sees it), the store's stock, row by row on its shelves, and the laundromat's machines and furniture.
 *
 * Coordinates (cm): the building fronts on the player's side of the street stand on X = 0, the road
 * runs along Y at X 420..1620, up is Z with the sidewalk's top at 0. The store spans Y 3700..4950; the
 * Wash & Fold, across the street, Y 15..1825 and X 2085..3140, its floor at Z 45.
 *
 * Built when play begins, as the Embercrest is (design spec 0.8 and 7.5.1): in the editor's world construction builds
 * only a preview (nothing of it is saved: every part is transient), and at play EnsureBuilt (BeginPlay, or the game mode
 * first when it needs the street sooner) clears whatever of the preview came along and builds the street for play, then
 * registers what it built with the street's registry (UStreetWorld: its uses, zones, starts, doors, walkways and
 * stations). Street.umap keeps only the actor, its settings and its preload list: every asset the street's stages load,
 * resolved in the editor (Preload), so the level's load streams them in and the build at play doesn't stop on them.
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

	/** The editor's preview again (after new meshes or materials), its preload list with it. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Short Stack")
	void RebuildSet();
	/** Builds the street once for play and registers it (BeginPlay does; the game mode calls it first, before the player is placed). */
	void EnsureBuilt();
	bool IsBuilt() const { return bBuilt; }

	/** Every asset path the street itself loads (its materials and Blender meshes), for the preload list (FStreetPreload). */
	static void AppendPreload(TArray<FString>& Paths);
	/** Every registered stage's paths (the street's, Market's, every shop's): what the editor resolves onto Preload. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack")
	static TArray<FString> PreloadPaths();
	/** How many assets the saved preload list holds (street_setup.py logs it). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack")
	int32 PreloadCount() const { return Preload.Num(); }

	/**
	 * Where the player appears: "home" just out of 1812's door, "store" inside the Lucky Penny, "washfold" on the sidewalk
	 * at the foot of the Wash & Fold's steps (out of Dee's game), "wash" just inside its door, "backdoor" at Dee's steel
	 * door inside it, "dee" out of the tenement's street door (Dee's, while the player sleeps on her couch).
	 */
	FTransform StartAt(const FString& From) const;
	/** Where Benny stands behind the counter, facing the customers (his feet on the store's floor). */
	FTransform ClerkSpot() const;
	/** Where the Wash & Fold's attendant folds behind her counter, facing the customers (her feet on its floor). */
	FTransform AttendantSpot() const;
	/** The top of the store's floor (world Z). */
	double StoreFloorZ() const;
	/** The top of the Wash & Fold's floor (world Z): three steps up from the sidewalk, level with its window's sill. */
	double WashFloorZ() const;
	/** The sidewalks' top (world Z). */
	double SidewalkZ() const;
	/** The spot within reach of an eye looking along Dir (the nearest it's facing), or null. The game mode asks the registry (UStreetWorld::Best: these spots, as uses, and every other stage's), which scores the same way. */
	const FStreetSpot* SpotFor(const FVector& Eye, const FVector& Dir) const;
	/** The registry's zone's place at At: "LUCKY PENNY #212" inside the store, "WASH & FOLD" inside the laundromat, "FIFTH STREET" (or the corner) outside. */
	FString PlaceName(const FVector& At) const;
	bool IsInsideStore(const FVector& At) const;
	bool IsInsideWash(const FVector& At) const;
	/** Somewhere nobody should be: under the street (fell through a gap) or outside every zone of the registry. */
	bool IsOffTheSet(const FVector& At) const;
	/** The store's doors slide open for someone within reach of them (or a walker, NoteWalker). True the moment they start to open. */
	bool UpdateDoors(const FVector& Someone, float DeltaSeconds);
	/** A hand on the Wash & Fold's door (the player's E, anyone walking in): it swings in, and its closer shuts it once nobody's in its way. */
	void OpenWashDoor();
	/** The Wash & Fold's door each frame (Someone keeps it from shutting on them): 1 the moment it starts to open (its bell), 2 the moment it shuts, else 0. */
	int32 UpdateWashDoor(const FVector& Someone, float DeltaSeconds);
	/** How far open the Wash & Fold's door is (0 shut .. 1 open). */
	float WashDoorOpen() const { return WashDoorAmount; }
	/** How far inside the Wash & Fold the camera is (0 out on the street .. 1 inside), eased. */
	float CameraInWash() const { return WashInside; }
	/** How near Listener stands to the Wash & Fold's front, out on the sidewalk (0 out of earshot .. 1 at its window). */
	float WashNear(const FVector& Listener) const;
	/** Someone besides the player walking about (the street's pedestrians, each frame): the doors open for them too. */
	void NoteWalker(const FVector& At);
	/** A sidewalk as people walk it (0 the home side, 1 the far side), for the street's pedestrians. */
	FStreetWalkway Sidewalk(int32 Side) const;
	/** The doors on the street people go in and out of: the Lucky Penny's, the Wash & Fold's, 1812's, Dee's. */
	TArray<FStreetDoor> Doors() const;
	/** 0 night .. 1 full day (Session::Daylight): the sky, the lamps and windows, the exposure. */
	void SetDaylight(float InDaylight);
	/** The player's image settings: Brightness in stops, film grain and fringe as multiples, motion blur. */
	void SetLensOptions(float InBrightnessBias, float InGrainScale, float InFringeScale, bool bInMotionBlur);
	/** How far inside the store the camera is (0 out on the street .. 1 inside), eased. */
	float CameraInside() const { return Inside; }
	/** How far open the store's sliding doors are (0 shut .. 1 open). */
	float DoorsOpen() const { return DoorOpen; }
	/** The rain's strength as it shows (ss.Street.Rain: 0 none, 1 the night's rain). */
	float RainStrength() const;
	/**
	 * How near Listener is to a buzzing sign (0 out of earshot .. 1 under it): the Lucky Penny's box sign at any hour, the
	 * pawn shop's blue tube and the laundromat's neon while they're lit.
	 */
	float SignHum(const FVector& Listener) const;
	/** The store's wall clock shows the world's time (minutes, as Session::WorldMinutes). */
	void SetClock(double WorldMinutes);

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
	/** The editor's preview of the street (construction builds it; nothing of it is saved). */
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	bool bEditorPreview = true;

	/**
	 * Every asset the street's stages load at play (PreloadPaths, plus whatever the editor's preview loaded), resolved in
	 * the editor and saved with the level: the level's load brings them in, so the build at play finds them loaded. Only
	 * the editor's construction sets it (street_setup.py saves the level when the list changes).
	 */
	UPROPERTY(VisibleAnywhere, Category = "Short Stack")
	TArray<TObjectPtr<UObject>> Preload;

private:
	/** The paths the editor's preview loaded, onto Preload (with every registered stage's): the editor's world only. */
	void ResolvePreload(const TSet<FString>& Loaded);
	/** What it built, into the registry (UStreetWorld): the spots as uses, the zones, the starts, the doors, the walkways, the stations. */
	void RegisterWithWorld();
	/** No rain inside the registry's dry zones (M_StreetRain's DryMin0..7, and the older material's store and laundromat boxes). */
	void ApplyDryBoxes();
	void BuildSet();
	void BuildGround();
	void BuildEnds();
	void BuildHomeBlock();
	void BuildEntrance();
	/** The stand-in's door, buzzer, lamp and number when 1812's own front isn't imported. */
	void BuildDoor();
	void BuildAcross();
	/**
	 * The tenement's front when SM_Tenement isn't imported: its brick round the same openings (the laundromat's window, the
	 * tenants' door), the shopfront's mullions and fascia, its floors of windows, so the Wash & Fold stands where it would.
	 */
	void BuildTenementStandIn();
	/** The Wash & Fold's front, both ways: the glass round its door's bay, the door on its hinge with its bell, the steps up. */
	void BuildWashFront();
	/** Its room: the floor, walls and pressed-tin ceiling, the fluorescents and their light. */
	void BuildWashRoom();
	/** What's in it: the washers, the dryers (some running), the tables, chairs, machines, carts, counter, TV and the back door. */
	void BuildWashMachines();
	/** The tenement's own baked laundromat (a shallow lit box behind its window) is hidden: the room built here stands there. */
	void HideTenementShop();
	/** At play: the imported running dryers' and the TV's glow slots, made fresh to animate (never saved, see PaintCars). */
	void PrepareWashGlow();
	/** The Wash & Fold as it runs: the drums' light tumbling, the TV flickering, the door's bell swinging on its spring. */
	void AnimateWash(float Dt, const FVector& Camera);
	void BuildStore();
	void BuildStoreDressing();
	/**
	 * The store's walls and ceiling finished: paint over a tiled wainscot, the slatwall by the door, its own sign and posters,
	 * the wall clock, the security mirrors, the ceiling's tile grid and the troffers' frames round the light panels.
	 */
	void BuildStoreInterior();
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
	 * One of Blender's building fronts (facades.py, tenement.py; nullptr when it isn't imported), its origin on its face at Y
	 * along the street: on the home side's building line turned to face the street, or on the far side's as modeled. It
	 * doesn't collide (FrontBox blocks what should) and its own glow dims by day.
	 */
	UStaticMeshComponent* Facade(const TCHAR* Name, double Y, bool bFar);
	/** An invisible blocking box in the meters of the front at Y: x out of its face toward the street negative, y along it, z up. */
	void FrontBox(double Y, bool bFar, double X0, double X1, double Y0, double Y1, double Z0, double Z1);
	/** Brownstones side by side from Y0 toward Y1, their block behind them; false (nothing built) when they aren't imported. */
	bool BrownstoneRow(double Y0, double Y1, bool bFar, UMaterialInterface* Massing);
	/** One kind of the store's stock rows (stock.py), every copy one instance. */
	UInstancedStaticMeshComponent* StockRows(UStaticMesh* Mesh);
	/** Every shelf of every door in the cooler (given its frame) stocked; false, nothing placed, unless every cooler row is imported. */
	bool StockCooler(const FTransform& Cooler);
	/** Every bay and level of both gondolas (their SM_Shelf frames, the south one first) stocked; false as StockCooler. */
	bool StockShelves(const TArray<FTransform>& Gondolas);
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

	// Used while playing: built at play (EnsureBuilt), or for the editor's preview, and never saved.
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> SkyMids;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> WindowMids;
	/** The street lamps' glass and the neon: dark by day. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> NightGlowMids;
	UPROPERTY(Transient)
	TArray<float> NightGlowLevels;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> RainSheets;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> RainMids;
	/** The rain sheets' material knows where the store is (no rain falls inside); the old one doesn't. */
	UPROPERTY(Transient)
	bool bRainMasksStore = false;
	/** The sliding doors: each a scene component holding its glass, frame and push bar (UpdateDoors moves it). */
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> DoorLeft;
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> DoorRight;
	/** The hours sticker: attached to the left door, so it slides with it. */
	UPROPERTY(Transient)
	TObjectPtr<UWidgetComponent> DoorDecal;
	UPROPERTY(Transient)
	TObjectPtr<USkyLightComponent> SkyLight;
	UPROPERTY(Transient)
	TObjectPtr<UDirectionalLightComponent> Moon;
	UPROPERTY(Transient)
	TObjectPtr<UExponentialHeightFogComponent> Fog;
	UPROPERTY(Transient)
	TObjectPtr<UPostProcessComponent> Lens;
	UPROPERTY(Transient)
	TArray<TObjectPtr<USpotLightComponent>> Streetlights;
	/** The store's ceiling lights: shadowed while the camera is near the store, so none of their light gets through its walls. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<URectLightComponent>> StoreLights;
	/** Lights that only shine at night (the door lamp, the neon's glow on the street). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULightComponent>> NightLights;
	UPROPERTY(Transient)
	TArray<float> NightLightLevels;
	/** One of the night lights: the laundromat's light through its window onto the sidewalk (ss.Street.LaundromatGlow). */
	UPROPERTY(Transient)
	TObjectPtr<URectLightComponent> WashSpill;
	/** The Wash & Fold's fluorescents: shadowed while the camera is near it, as the store's are (PickStreetlights). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<URectLightComponent>> WashLights;
	/** Its door: the leaf on its hinge (UpdateWashDoor swings it), and the invisible stop that blocks the doorway while it's shut. */
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> WashDoor;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> WashDoorStop;
	/** The bell on its spring over the door, inside (it swings once the door has struck it). */
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> WashBell;
	/** The running dryers: each one's light out through its drums' glass, tumbling (AnimateWash). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPointLightComponent>> DrumLights;
	/** The imported running dryers (their glow slots animated at play, PrepareWashGlow), as many as DrumLights; or none. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> DrumStacks;
	/** The stand-in running dryers' lit drum glass, one material each (none when the dryers are imported). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DrumMids;
	/** The imported idle dryers, every copy one instance: their drums are dark (PrepareWashGlow). */
	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> IdleDryers;
	/** The TV on the back wall: the imported one (its screen's glow slot animated), or the stand-in's screen; and its light on the room. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> WashTv;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> WashTvMid;
	UPROPERTY(Transient)
	TObjectPtr<URectLightComponent> WashTvLight;
	/** The glow slots PrepareWashGlow made to animate (the imported dryers' drums, the TV's screen), and what each scales. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> GlowMids;
	struct FGlowParam
	{
		int32 Mid = 0;
		FName Name;
		bool bVector = true;
		FLinearColor Base = FLinearColor::Black;
		/** The running dryer it lights (DrumLights' index), or -1 for the TV. */
		int32 Owner = -1;
	};
	TArray<FGlowParam> GlowParams;
	/** The store clock's hands, each turning about its own pivot at the dial's middle (SetClock). */
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> ClockHour;
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> ClockMinute;
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> ClockSecond;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidgetComponent>> SignWidgets;
	UPROPERTY(Transient)
	TArray<FString> SignArt;
	UPROPERTY(Transient)
	TArray<float> SignGlow;
	UPROPERTY(Transient)
	TArray<FStreetSpot> Spots;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> ParkedCars;
	UPROPERTY(Transient)
	TArray<FLinearColor> ParkedPaints;
	/** Blender's building fronts: their lit rooms, lanterns and transoms are baked for the night and dim by day (ApplyDaylight). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> LitFronts;
	/**
	 * The tenement, when it stands across the street: its rooms were baked for the apartment's view (seen through rain from
	 * a lit room, a brighter exposure than the street's night), so here they glow less; its own shallow laundromat is
	 * hidden (HideTenementShop), the Wash & Fold built here stands in it.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> TenementFront;
	/** The Wash & Fold's room, where no rain falls, and where the rain catches the neon's pink (the stage's own cm), as BuildAcross built them. */
	UPROPERTY(Transient)
	FVector WashMin = FVector(2056.0, -5.0, -60.0);
	UPROPERTY(Transient)
	FVector WashMax = FVector(3160.0, 1845.0, 370.0);
	UPROPERTY(Transient)
	FVector NeonGlowAt = FVector(1590.0, 640.0, 700.0);
	/** While building: 1812's own front stands, so BuildEntrance adds only what it lacks. */
	bool bHomeFront = false;
	/** Built for play (EnsureBuilt): until then it neither ticks nor answers for the registry. */
	bool bBuilt = false;
	/** The registry's zones as the rain's dry boxes were last set from them (UStreetWorld::ZoneRevision; -1 never). */
	int32 AppliedZoneRevision = -1;
	/** The uses it registered (taken back when it's built again). */
	TArray<int32> UseHandles;

	TArray<TSharedPtr<SDrawListWidget>> SignSlates;
	TArray<TSharedPtr<const ss::ui::DrawList>> SignLists;
	float DoorOpen = 0.0f;
	bool bDoorsNear = false;
	/** The Wash & Fold's door: how far open, whether a hand holds it open, and how long nobody's been in its way. */
	float WashDoorAmount = 0.0f;
	bool bWashDoorHeld = false;
	float WashDoorClear = 0.0f;
	float WashInside = 0.0f;
	/** How hard the bell over the door is still swinging (0 at rest), and the laundromat's own clock for what runs in it. */
	float BellSwing = 0.0f;
	float WashClock = 0.0f;
	/** The TV: its picture's level and color as it cuts from shot to shot. */
	float TvCutIn = 0.0f;
	float TvLevel = 0.8f;
	float TvTarget = 0.8f;
	FLinearColor TvColor = FLinearColor(0.7f, 0.8f, 1.0f);
	FLinearColor TvColorTarget = FLinearColor(0.7f, 0.8f, 1.0f);
	FRandomStream TvDice = FRandomStream(1812);
	/** Pedestrians near the doors (NoteWalker): this frame's, and last frame's (whichever ticked first, none is missed). */
	TArray<FVector> Walkers;
	TArray<FVector> WalkersSeen;
	/** The second of the day the clock's hands were last set to (-1: not yet). */
	int32 ClockShown = -1;
	float Daylight = 0.0f;
	float AppliedDaylight = -1.0f;
	/** ss.Street.TenementGlow and ss.Street.LaundromatGlow as ApplyDaylight last set the fronts' glow and the spill by them. */
	float AppliedTenementGlow = -1.0f;
	float AppliedLaundromatGlow = -1.0f;
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
