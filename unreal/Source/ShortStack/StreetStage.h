#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "ShortStack/Game/Town.h"
#include "ShortStack/Game/Weather.h"
#include "ShortStack/UI/Canvas.h"
#include "StreetNs.h"
#include "StreetWorld.h"

#include "StreetStage.generated.h"

class AMarketStage;
class AStreetTraffic;
class FStreetAmbience;
class SDrawListWidget;
class UDecalComponent;
class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class UInstancedStaticMeshComponent;
class ULightComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMaterialParameterCollection;
class UMeshComponent;
class UPointLightComponent;
class UPostProcessComponent;
class URectLightComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;
class USpotLightComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UVolumetricCloudComponent;
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
 * Fifth Street, from Hollis Savings Bank's clock at its south end to the Rialto's marquee at its north, the corner of
 * Market with the Lucky Penny #212 on it, and the tenement across from door 1812 with the Wash & Fold at its foot
 * (world realism spec 2-3). Sidewalks, curbs and the crowned road are Northside's modelled ground (ns_ground), puddled
 * by the weather; the home and far sides' plots are the building kit (ns_fronts: SM_NS_Bldg_<Plot> with its trim, shop
 * glass, glows and grille), between the fronts kept from before (1812, the walk-up over the barber's, the brownstones,
 * the store's floors, the tenement); the street's two ends are T intersections, Hollis and Wexler running across them
 * with cars passing (AStreetTraffic), each closed by a landmark (the bank, the Rialto) and lined by stand-in fronts out
 * into the haze. Market St east of the corner is AMarketStage's (at its new width, which the crowd walks from the
 * registry's walkways, spec 6.8; ss.Street.WideMarket 0 keeps The Block's Market for an A/B); this stage spawns the barber's and Ruby's shop
 * stages on its plots, Market's stage, and the traffic.
 *
 * The store itself, lit inside, with sliding doors, coolers, aisles, the counter, the roller grill and the coffee bar, its
 * walls and ceiling finished as a corner store's are (a clock on the wall keeps the world's time). The Wash & Fold is a
 * room too, three steps up through its door (its bell rings as it swings in): rows of washers, dryers stacked along the
 * back and side walls (the running ones' drums lit and tumbling), folding tables, chairs at the window, the change and
 * soap machines, carts, the attendant's counter, a TV up on the back wall, and Dee's steel door at the back, where her
 * game is. Beside it the tenement's own street door, Dee's, up to her front room.
 *
 * The look (StreetStageLook.cpp, spec 3.1-3.3): a SkyAtmosphere with the sun on its October path and the moon, volumetric
 * cloud as thick as the weather, two height-fog layers, exposure clamps and a grade by the sun's elevation, MegaLights
 * on every lamp; the weather (core's weather::At from the career's seed, ss.Street.Weather / ss.Street.Rain to force it)
 * drives the rain (its sheets, its hits on the ground round the camera, the drips off the awnings), the wet, the puddles,
 * the lightning and MPC_NS, which every M_NS_ material reads (the kit's windows light by the hour from it). Core draws the
 * [screen] faces (Ns::FScreens: the parked cars' plates, the Rialto's marquee, the awnings' valances), and the plots' shops
 * keep their real hours (core's shop::Venues). Every imported mesh has an engine-shape stand-in, so the street works before
 * its art lands. The cross streets run out to where their ground ends in the haze, each end closed by a row of fronts.
 *
 * Coordinates (cm, spec 0.3 and StreetNs.h): the building fronts on the player's side of the street stand on X = 0, the
 * road runs along Y at X 420..1620, up is Z with the sidewalk's top at 0; +X is west, +Y north. The store spans Y
 * 3700..4950; the Wash & Fold, across the street, Y 15..1825 and X 2085..3140, its floor at Z 45.
 *
 * Built when play begins, as the Embercrest is (design spec 0.8 and 7.5.1): in the editor's world construction builds
 * only a preview (nothing of it is saved: every part is transient), and at play EnsureBuilt (BeginPlay, or the game mode
 * first when it needs the street sooner) clears whatever of the preview came along and builds the street for play, then
 * registers what it built with the street's registry (UStreetWorld: its uses, zones, starts, doors, walkways and
 * stations). Street.umap keeps only the actor, its settings, its preload list (every asset the street's stages load,
 * resolved in the editor, so the level's load streams them in and the build at play doesn't stop on them), the cloud
 * material and the kit's pivots (Places), resolved in the editor too.
 * The look follows the camera each frame after it has moved (TG_PostUpdateWork): the rain sheets, the exposure (the
 * street's by the sun, the store's own brighter range inside it), which lamps scatter shadows into the fog.
 */
UCLASS()
class SHORTSTACK_API AStreetStage : public AActor
{
	GENERATED_BODY()

public:
	AStreetStage();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
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

	// ------------------------------------------------------------ the realism pass (spec 3)
	/** The career's world seed (Session::WorldSeed): the weather, the windows' hash and the parked cars follow it. Until it's set, 0. */
	void SetWorldSeed(uint64 InSeed);
	/** The hour the look shows (0..24): the clock's, or ss.Street.Hour's while it's set. */
	double LookHour() const;
	/** The weather as it shows now (the career's model, or ss.Street.Weather and ss.Street.Rain while they force it), eased. */
	const ss::weather::Now& WeatherNow() const { return Weather; }
	/** Market St at its new width (spec 0.3) and AMarketStage building it, or today's Market (ss.Street.WideMarket 0, for an A/B). */
	bool IsWideMarket() const { return bWideMarket; }
	/** Whether Market goes at its new width the next time the street is built: ss.Street.WideMarket (-1, the default: yes; the crowd walks the registry's walkways). */
	static bool WideMarketWanted();
	/** The world minutes the look shows (the clock's, or ss.Street.Hour's). */
	double LookWorld() const { return ShownWorld; }
	/** The hour's glow levels (Market's stage and the traffic light their glows by them). */
	const Ns::FGlowLevels& Glows() const { return GlowLevels; }
	/**
	 * A skip is running (a service under the TimeSkip): the corner's signals and the cross streets' traffic hold where they
	 * are (spec 2.5). It's seen from the clock's rate; the game mode may say so too (here, or UStreetWorld::SetHold).
	 */
	void SetSkipHold(bool bHold) { bSkipHoldSet = bHold; }
	bool IsHolding() const;
	/** The corner of Fifth & Market's signals now (town::Signal, held through a skip). */
	const ss::town::SignalState& SignalNow() const { return SignalShown; }

	/** ss.Street.Hour: the look's hour (the sky, the lamps, the windows, the shops' lights), -1 back to the clock's. The session's clock is untouched. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestHour(float Hour);
	/** ss.Street.Weather: clear, overcast, drizzle, rain, storm, or auto (the career's weather). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestWeather(const FString& State);
	/** ss.Street.Budget: the frame's cost and what's drawing it (lights in range, shadowed and in the fog; MetaHumans by tier; translucency). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestBudget() const;
	/** One line: the hour, the sun, the weather, the exposure's clamps, Market's width, the kit's plots placed. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestLook() const;

	/** An imported Blender mesh (nullptr when it isn't), noted for the editor's preload list while the preview builds. */
	static UStaticMesh* NsMesh(const TCHAR* Name);
	/** A material (or a parameter collection) by object path, noted the same way; nullptr when it isn't there. */
	static UObject* NsAsset(const TCHAR* Path);
	/** A decal cell's material (MI_NS_Decal_<Cell>), or nullptr while the decal atlas (T_NS_Decals, art-street's) isn't imported: no grey squares. */
	static UMaterialInterface* DecalMaterial(int32 Cell);
	/**
	 * Drip lines under an edge (an awning's valance, the shed's deck; spec 3.3): one thin upright line every Spacing cm from A
	 * to B (stage cm, at the edge's height), each falling to the ground at GroundZ, its drops gathering and letting go by the
	 * rain and the wet (M_NS_RainFx, Mode 1). Set is an instanced cylinder with two custom floats; Frame the stage's.
	 */
	static void AddDrips(UInstancedStaticMeshComponent* Set, const FTransform& Frame, const FVector& A, const FVector& B, double GroundZ, double Spacing);
	/** The ground's height (stage cm) at X, Y: a sidewalk's 0, a road's crown (spec 4.1's RoadZ) on Fifth, Market and the cross streets. */
	static double GroundZAt(double X, double Y, bool bWideMarket);
	/** The slot of C's mesh whose imported name (or material's) is Name ("ns_glass"), or INDEX_NONE. */
	static int32 NsSlot(const UMeshComponent* C, const TCHAR* Name);
	/**
	 * For ns_setup.py (the editor only): the sections of Mesh that draw with the slot named Slot in or out of the ray-traced
	 * scene, casting shadows or not, the mesh rebuilt. The trees' leaves leave ray tracing this way (spec 3.8). False when
	 * nothing matched (or outside the editor).
	 */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Setup")
	static bool NsSectionFlags(UStaticMesh* Mesh, const FString& Slot, bool bRayTracing, bool bCastShadow);

	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	float ExposureBias = 0.0f;
	/** How wet the street is (0 dry .. 1 puddles everywhere): the weather's, set each frame (the street's sound reads it). */
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	float Wetness = 0.85f;
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	float StreetlightCandela = 1400.0f;
	/** Every lamp casts shadows (MegaLights, spec 3.1); this many of the nearest to the camera cast them into the fog too. */
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	int32 ShadowedStreetlights = 4;
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
	/** The engine's simple volumetric cloud material, resolved in the editor and saved (nothing loads engine or plugin content by path at play, spec 0.8). */
	UPROPERTY(VisibleAnywhere, Category = "Short Stack")
	TObjectPtr<UMaterialInterface> CloudMaterial;
	/**
	 * Where the kit's pivoted meshes go (its grilles, the bank's clock hands: ns_fronts.py writes art/build/ns_fronts_places.json
	 * as it builds them), stage cm by mesh name, resolved in the editor and saved with the level.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Short Stack")
	TMap<FName, FVector> Places;

private:
	/** The paths the editor's preview loaded, onto Preload (with every registered stage's): the editor's world only. */
	void ResolvePreload(const TSet<FString>& Loaded);
	/** What it built, into the registry (UStreetWorld): the spots as uses, the zones, the starts, the doors, the walkways, the stations. */
	void RegisterWithWorld();
	/** No rain inside the registry's dry zones (M_StreetRain's DryMin0..7, and the older material's store and laundromat boxes). */
	void ApplyDryBoxes();
	void BuildSet();
	/** Fifth's ground (StreetStageGround.cpp): the modelled ground meshes, or engine-shape stand-ins in the same plan, then the castings and decals. */
	void BuildGround();
	/** The ground's stand-ins: Fifth's south half and the Hollis T (bSouth), its north half with the corner and the Wexler T (bNorth). */
	void BuildGroundStandIn(bool bSouth, bool bNorth, bool bVisible = true);
	/** Market as The Block built it (its ground, its blocks, the building across its end), while the crowd walks its hard-coded lines. */
	void BuildEnds();
	/** The street's ends as T intersections (StreetStageFronts.cpp): the cross streets' far sides (the bank, the Rialto, the proxies), the walk limits' closures, the curbs' blocking. */
	void BuildCrossStreets();
	/** Fifth's plots (StreetStageFronts.cpp): the building kit where it's imported, else a stand-in of each plot's massing, windows and shop. */
	void BuildPlots();
	/** One plot of StreetStageFronts.cpp's table: its kit meshes (or its stand-in), its shop's lit things, its collision. */
	void BuildPlot(int32 Index);
	/** The street's furniture (StreetStageFurniture.cpp, spec 2.8): lamps, the corner's signals and blades, signs, hydrants, cans, trees, castings, awnings, parked cars. */
	void BuildFurniture();
	void BuildCorner();
	void BuildTrees();
	void BuildParkedCars();
	/** The street's positioned sounds (StreetStageFurniture.cpp, spec 3.7): the signs' hum, the AC units, the downspouts, the shops' open doors. */
	void BuildEmitters();
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
	/** The sky, the sun and the moon, the clouds, the fog, the lens and the rain sheets (StreetStageLook.cpp, spec 3.2). */
	void BuildSkyAndWeather();
	/** The rain sheets in front of the camera (their layers: StreetStage.cpp). */
	void BuildRain();
	/** At play: the shops on this stage's plots (the barber's, Ruby's), Market's stage, the cross streets' traffic. */
	void SpawnNeighbors();
	/** The kit's pivots (art/build/ns_fronts_places.json) onto Places: the editor's world only. */
	void ResolvePlaces();
	/** Every asset path the realism pass's builders load (the ground, the plots, the furniture, the look's): AppendPreload's. */
	static void NsAppendPreload(TArray<FString>& Paths);
	static void AppendGroundPreload(TArray<FString>& Paths);
	static void AppendFrontsPreload(TArray<FString>& Paths);
	static void AppendFurniturePreload(TArray<FString>& Paths);
	static void AppendLookPreload(TArray<FString>& Paths);
	/** A decal (M_NS_Decal's cell, MI_NS_Decal_<Cell>) laid on the ground at At (stage cm), Size cm across, turned Yaw; nullptr without the material. */
	UDecalComponent* GroundDecal(int32 Cell, const FVector& At, float Yaw, const FVector2D& Size);
	/** The crowd's people kept off Market's roadway while Market is at its new width and the crowd still walks today's lines (hidden there). */
	void KeepCrowdOffMarket(float Dt);
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
	/** The hour shown (the clock, or ss.Street.Hour) and whether a skip is running (the clock racing ahead). */
	void UpdateClock(float Dt);
	/** The weather to show (the career's, or forced), eased toward; the lightning's flash and its thunder. */
	void UpdateWeather(float Dt);
	/** The sun and the moon, the sky's light, the clouds, the fog, the exposure's clamps and the grade, the glows and the lamps (spec 3.2, 3.1). */
	void ApplyHour();
	/** MPC_NS (spec 0.7), each frame. */
	void UpdateMpc();
	/** Each plot's shop by its hours: its glows, spill and sign lights, lit displays and grille. */
	void UpdateShops(float Dt);
	/** The corner's signals and the pedestrians' heads (town::Signal; held through a skip). */
	void UpdateSignals();
	/** The stand-in fronts' windows by the hour (the kit's light themselves from MPC_NS). */
	void UpdateWindowsByHour();
	/** The bank's clock shows the look's time. */
	void UpdateBankClock();
	/** The street's sound: its bed in stereo and the positioned sounds near the camera (StreetAudio.h), the thunder. */
	void UpdateSound(const FVector& Camera, const FRotator& Rotation, float Dt);
	/** The rain's hits on the ground (a pool of splashes kept round the camera) and the drips off Fifth's awnings (spec 3.3). */
	void BuildRainFx();
	/** The splashes' pool laid round the camera again once it has moved on (on open ground only, never in a room). */
	void UpdateRainFx(const FVector& Camera);
	/** The budget's A/B switches (spec 3.8's day-one baseline): MegaLights, the trees (Market's too), as the console sets them. */
	void ApplyBudgetToggles();

	// ------------------------------------------------------------ the realism pass's helpers (stage cm, stage yaw)
	/** A SITE mesh (spec 0.4) at the stage's origin turned 180; nullptr when it isn't imported. Collides only if asked (complex as simple). */
	UStaticMeshComponent* Site(const TCHAR* Name, bool bCollide = false);
	/** A PROP mesh of the NS kit (front toward Blender -X) at At facing stage yaw Yaw; nullptr when it isn't imported. No collision. */
	UStaticMeshComponent* NsProp(const TCHAR* Name, const FVector& At, float Yaw, const FVector& Scale = FVector(1.0));
	/** An invisible blocking box (stage cm). */
	void Blocker(const FVector& Min, const FVector& Max);
	/** A glow mesh made to glow by the hour (M_NS_Glow, or its own emissive scaled); Shop -1: lit at night whatever the hours. */
	int32 AddGlow(UStaticMeshComponent* Mesh, Ns::EGlow Family, uint32 Hex, float Scale, int32 Shop);
	/** A storefront's light on the sidewalk (spec 3.1): unshadowed, the glass's size, aimed down 35 degrees out of the face; dropped beyond 40 m. */
	URectLightComponent* Spill(const FVector& At, float Yaw, float Width, float Height, uint32 Hex, float Lumens, int32 Shop);
	/** A stand-in front down a cross street (SM_NS_Proxy_A..H, or a box of windows): its middle on the building line at At, facing Yaw, Width cm wide. */
	void Proxy(const FVector& At, float Yaw, double Width, int32 Seed);
	/** A run of proxies along a building line from A to B (stage cm), facing Yaw, neighbors never alike. */
	void ProxyRun(const FVector& A, const FVector& B, float Yaw, int32 Seed);
	/** A massing box of a stand-in plot with its windows on its street faces (+-X faces on Fifth, +-Y on the cross streets). */
	void StandInBlock(const FVector& Min, const FVector& Max, uint32 Brick, int32 Seed, bool bFacesX, bool bFacesY);

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

	// ------------------------------------------------------------ the realism pass's state
	/** A signal head's glow: 0 Fifth's vehicle heads, 1 Market's, 2 the walk heads across Fifth, 3 across Market's mouth. */
	struct FNsSignal
	{
		TWeakObjectPtr<UMaterialInstanceDynamic> Mid;
		uint8 Role = 0;
	};
	/** A stand-in window (M_StreetWindow's instance): lit by the hour from its seed. */
	struct FNsWindow
	{
		TWeakObjectPtr<UInstancedStaticMeshComponent> Set;
		int32 Index = 0;
		float Seed = 0.0f;
		float Level = 1.0f;
		uint8 Look = 1;
		uint8 Room = 0;
	};
	/** A positioned sound's place (stage cm) and kind (FStreetEmitter). */
	struct FNsEmitterSpot
	{
		FVector At = FVector::ZeroVector;
		uint8 Kind = 0;
		float Level = 1.0f;
		float Hz = 120.0f;
		int32 Shop = -1;
	};
	/** The plots' shops and every glow mesh, lit by the hour (StreetNs.h). */
	Ns::FLit Lit;
	/** The [screen] faces core draws (the parked cars' plates, the Rialto's marquee, the awnings' valances). */
	Ns::FScreens Screens;
	/** The hour's glow levels by family (ApplyHour), handed to Market's stage too. */
	Ns::FGlowLevels GlowLevels;
	TArray<FNsSignal> NsSignals;
	TArray<FNsWindow> NsWindows;
	TArray<FNsEmitterSpot> NsEmitters;
	/** What stands on each of Fifth's sidewalks (0 home, 1 far; stage cm footprints), for its walkways (Sidewalk): the furniture as BuildFurniture placed it. */
	TArray<FBox2D> NsFootprints[2];
	/** The crowd's people this stage hid on Market's roadway (KeepCrowdOffMarket), shown again once they're off it. */
	TArray<TWeakObjectPtr<AActor>> CrowdHidden;
	float CrowdCheckIn = 0.0f;
	/** ss.Street.CrowdOffMarket: the crowd's people kept off Market's roadway (a guard for a crowd on today's lines; off by default, the crowd reads the registry). */
	bool bCrowdOnOldLines = false;
	UPROPERTY(Transient)
	TObjectPtr<UDirectionalLightComponent> Sun;
	UPROPERTY(Transient)
	TObjectPtr<USkyAtmosphereComponent> Atmosphere;
	UPROPERTY(Transient)
	TObjectPtr<UVolumetricCloudComponent> Clouds;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CloudMid;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SkyGlowMid;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialParameterCollection> Mpc;
	/** The bank's clock hands (each turning about its own pivot, the dial facing north). */
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> BankHour;
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> BankMinute;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Trees;
	/** The rain's splashes round the camera (M_NS_RainFx, Mode 0) and the drips off the awnings (Mode 1). */
	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> Splashes;
	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> Drips;
	/** Where the splashes' pool was last laid (stage cm, snapped), and when it may be laid again. */
	FVector2D SplashCenter = FVector2D(1.0e9, 1.0e9);
	float SplashIn = 0.0f;
	/** ss.Street.Trees as last applied (-1: not yet). */
	int8 TreesApplied = -1;
	/** The awnings' front edges (stage cm, BuildFurniture), where the drips fall from. */
	TArray<TPair<FVector, FVector>> DripEdges;
	TWeakObjectPtr<AMarketStage> Market;
	TWeakObjectPtr<AStreetTraffic> Traffic;
	/** The street's sound, as the game mode made it (found when it's there, FStreetAmbience::Live). */
	TSharedPtr<FStreetAmbience> Tone;
	bool bToneStereo = false;
	/** Its listener's emitters: the stage's and Market's, sent each frame. */
	float SoundIn = 0.0f;

	uint64 WorldSeed = 0;
	/** The clock (SetClock's world minutes; negative: not told yet) and the look's (the override applied). */
	double ClockWorld = -1.0;
	double ShownWorld = 22.0 * 60.0 + 1440.0;
	double RealClock = 0.0;
	double RateWorld = -1.0;
	double RateReal = 0.0;
	bool bSkipSeen = false;
	bool bSkipHoldSet = false;
	float SkipQuiet = 0.0f;
	ss::weather::Now Weather;
	ss::town::SignalState SignalShown;
	float SunEl = -30.0f;
	float SunAz = 270.0f;
	float EvMin = 0.5f;
	float EvMax = 3.0f;
	/** What ApplyHour last applied (the hour, the cloud, the rain), so it runs only when they move. */
	float AppliedHourKey = -1000.0f;
	float FlashEnv = 0.0f;
	double NextFlashAt = 0.0;
	float LastCoreFlash = 0.0f;
	float WindowHourApplied = -1000.0f;
	bool bWideMarket = false;
	int32 PlotsKit = 0;
	int32 PlotsStandIn = 0;
	/** The stand-in windows' instances being laid out (Window() records them for the hour). */
	bool bRecordWindows = true;
	/** Console variables the street sets while it runs (Lumen's far field), put back as the level ends. */
	TArray<TPair<FString, FString>> SavedCvars;
	void SetCvar(const TCHAR* Name, const TCHAR* Value);
};

template <typename T>
T* AStreetStage::NewPart(USceneComponent* Parent)
{
	// Transient: built when play begins (or for the editor's preview), never saved with the level. The preview's parts are
	// not duplicated either: Play-In-Editor copies the level by duplication, which copies transient objects that are still
	// referenced (BlueprintCreatedComponents is), and the play copy builds its own street at BeginPlay (as the Embercrest).
	const UWorld* InWorld = GetWorld();
	const EObjectFlags Flags = InWorld && InWorld->IsGameWorld() ? RF_Transient : EObjectFlags(RF_Transient | RF_DuplicateTransient);
	T* Part = NewObject<T>(this, NAME_None, Flags);
	Part->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	Part->SetupAttachment(Parent ? Parent : Root.Get());
	BlueprintCreatedComponents.Add(Part);
	Part->RegisterComponent();
	return Part;
}
