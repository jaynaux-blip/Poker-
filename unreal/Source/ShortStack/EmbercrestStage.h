#pragma once

#include "CoreMinimal.h"
#include "CoreCanvasTarget.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "ShortStack/UI/Canvas.h"

#include "EmbercrestStage.generated.h"

class SDrawListWidget;
class UCameraComponent;
class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class UInstancedStaticMeshComponent;
class ULightComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPointLightComponent;
class UPostProcessComponent;
class URectLightComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;
class USpotLightComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTexture;
class UTextureRenderTarget2D;
class UVolumetricCloudComponent;

/** Where the hero appears (spec 2.9). */
UENUM()
enum class EEcxSpawn : uint8
{
	ArriveInside,   // inside the bus's front door, the step-off's start
	ArrivePlatform, // on the platform, facing up the walk (also the skip and recovery point)
	Leave,          // just out of the revolving door, facing the valley
};

/** Where the 14 is. */
UENUM()
enum class EEcxBusPhase : uint8
{
	Gone,      // out of sight (hidden)
	Arriving,  // climbing the ridge road and easing into the bay
	Stopped,   // at the stop
	Departing, // pulling out and away north
};

/** When an interaction spot can be used. */
UENUM()
enum class EEcxWhen : uint8
{
	Always,
	Arriving,
	Leaving,
	/** Leaving, once the bus has opened its front door. */
	LeavingBusOpen,
};

/** Something the bus just did, for the sound (AEmbercrestStage::TakeBusEvents). */
namespace EcxBusEvent
{
constexpr uint32 Brakes = 1u << 0;     // the air brakes as it stops
constexpr uint32 Kneel = 1u << 1;      // the suspension letting out air (kneeling or rising)
constexpr uint32 DoorsOpen = 1u << 2;  // the doors' hiss and the two-note chime
constexpr uint32 DoorsClose = 1u << 3; // the doors' hiss shut
constexpr uint32 PullAway = 1u << 4;   // the motor winding up as it leaves
} // namespace EcxBusEvent

/** A place the player can use: walk up, the prompt shows, E does it (At is the floor plus 100 cm). */
USTRUCT()
struct FEcxSpot
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
	UPROPERTY()
	EEcxWhen When = EEcxWhen::Always;
};

/** One slide on the marquee (2048 x 768 at 128 px a meter): the base slide's art with the game's text, or the series lockup alone. */
USTRUCT()
struct FEcxSlide
{
	GENERATED_BODY()

	/** 0: the base slide (T_ECX_MarqueeBg) with the text over its middle and right; 1: the series lockup slide (T_ECX_MarqueeSeries). */
	UPROPERTY()
	int32 Image = 0;
	/** A small line over the title, in ember ("TONIGHT"). */
	UPROPERTY()
	FString Kicker;
	/** The big line, in ash ("EMBERCREST NIGHTLY"). */
	UPROPERTY()
	FString Title;
	/** Under it, in gold ("CARDS AT 7:00 PM  ·  $120  ·  20,000 CHIPS"). */
	UPROPERTY()
	FString Detail;
	/** A table of rows instead (the day's schedule: "12:00 PM\tNOON DEEPSTACK\t$80"). */
	UPROPERTY()
	TArray<FString> Rows;
};

/** A local light with its level and when it shines. */
USTRUCT()
struct FEcxLight
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<ULightComponent> Light;
	/** Its intensity at full (candelas). */
	UPROPERTY(Transient)
	float Level = 0.0f;
	/** EEcxRule (EmbercrestStageLook.cpp). */
	UPROPERTY(Transient)
	uint8 Rule = 0;
};

/** A glow material (the game-colored LED lines, lenses and letters) and the family of light it takes. */
USTRUCT()
struct FEcxGlow
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Mid;
	/** EEcxGlow (EmbercrestStageLook.cpp). */
	UPROPERTY(Transient)
	uint8 Family = 0;
	UPROPERTY(Transient)
	float Scale = 1.0f;
};

/** An imported mesh with light baked into some of its slots ([lit]: the casino through its windows, the rooms of the tower). */
USTRUCT()
struct FEcxLit
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Mesh;
	/** Per material slot: below 0 not lit (left alone); otherwise the least it dims to by day (0.35 for rooms open all night). */
	UPROPERTY(Transient)
	TArray<float> Floors;
	UPROPERTY(Transient)
	float Scale = 1.0f;
};

/** A fire bowl: its light, its flame and the embers in its bowl, flickering together. */
USTRUCT()
struct FEcxFire
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> Light;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Flame;
	/** The bowl's ember media ([lit]), its glow scaled by the flicker. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Media;
	UPROPERTY(Transient)
	FLinearColor MediaGlow = FLinearColor::Black;
	UPROPERTY(Transient)
	FName MediaParam;
	UPROPERTY(Transient)
	FVector At = FVector::ZeroVector;
	UPROPERTY(Transient)
	int32 Seed = 1;
};

/**
 * THE EMBERCREST from outside: the resort casino on the ridge above the city, where the 14 bus stops (design spec
 * ec_exterior_spec.md). The plaza stepping down from the porte-cochere between two cascades with fire bowls standing in
 * them, the entrance pavilion and its revolving door, the casino wing with its LED marquee, the north wing with the Ember
 * Grill and the poker room upstairs, the hotel tower and its crown of three ember peaks, the bus stop and the ridge road,
 * the parapet over the drop, and beyond it the valley with the city's lights far below; the Crest massif behind.
 *
 * Every Blender mesh of the contract (SM_ECX_*, art/blender/assets/ecx_*.py) is placed where the spec puts it (SITE
 * meshes at the origin turned half round, PROPs at their spots); one that isn't imported yet stands in as a block-out of
 * engine shapes at the same coordinates, so the arrival, the walk, the doors and the bus all work before the art lands.
 *
 * Cook-safe: the stage builds its world when play begins (BeginPlay, from its own properties and the always-cooked
 * /Game/ShortStack content), never from construction scripts a cooked game doesn't run; in the editor, construction
 * builds a preview that isn't saved with the level (every part is transient). The look (the sun or moon, the sky, the
 * fixtures, the exposure and the grade) follows the hour it's given (SetHour, from the session's clock); the fires
 * flicker, the revolving door turns and the screens change slides as it ticks.
 */
UCLASS()
class SHORTSTACK_API AEmbercrestStage : public AActor
{
	GENERATED_BODY()

public:
	AEmbercrestStage();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void BeginDestroy() override;
	virtual void Tick(float DeltaSeconds) override;

	/** The editor's preview again (after new meshes or materials). */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Short Stack")
	void RebuildSet();
	/** Builds the world once for play (BeginPlay does; the game mode calls it first when it needs the ground sooner). */
	void EnsureBuilt();

	// ------------------------------------------------------------ the flows
	FTransform SpawnAt(EEcxSpawn Where) const;
	/** The spot within reach of an eye looking along Dir that may be used now (the nearest it faces), or null. */
	const FEcxSpot* SpotFor(const FVector& Eye, const FVector& Dir, bool bLeaving, bool bBusOpen) const;
	/** Any door spot within reach, whether or not it may be used (leaving, the doorman says why not). */
	const FEcxSpot* DoorSpotNear(const FVector& At) const;
	/** "THE EMBERCREST", "ROUTE 14 · EMBERCREST STOP" by the shelter, "THE OVERLOOK". */
	FString PlaceName(const FVector& At) const;
	/** Under the set (Z below -15 m) or more than 10 m outside its bounds. */
	bool IsOffTheSet(const FVector& At) const;
	/** The door nearest At (0 the revolving door, 1..4 the swing leaves south to north) and where to step to go through it. */
	int32 NearestDoor(const FVector& At, FVector& OutApproach) const;
	/** The revolving door speeds up to 4 rpm (someone going in), or back to 2. */
	void SetRevolvingFast(bool bFast);
	/** A swing leaf (1..4) opens 80 degrees outward. */
	void OpenSwingDoor(int32 Door);
	/** The revolving door's speed now (degrees a second). */
	float RevolvingSpeed() const { return RevSpeed; }

	// ------------------------------------------------------------ the look
	/** The world hour (0..24) the light is set for: the sun or the moon, the sky, the fixtures, the exposure, the grade. */
	void SetHour(double Hour);
	double GetHour() const { return Hour; }
	double GetSunElevation() const { return SunEl; }
	/** The player's image settings: Brightness in stops, film grain and fringe as multiples, motion blur. */
	void SetLensOptions(float InBrightnessBias, float InGrainScale, float InFringeScale, bool bInMotionBlur);
	/** The free camera the arrival's shots and the acceptance shots look through (the stage is the view target then). */
	UCameraComponent* GetCineCamera() const { return CineCam; }
	void SetCineCamera(const FVector& Location, const FVector& LookAt, float HorizontalFov);
	/** The acceptance shots (spec 7): 3 the overlook, 4 the crane, 5 mid-walk, 6 low fire (1 and 2 are gameplay cameras). */
	static bool ShotFor(int32 Shot, FVector& OutLocation, FVector& OutLookAt, float& OutFov);

	// ------------------------------------------------------------ the screens
	/** The marquee's slides, 7 s each with a 0.6 s crossfade, from the first. */
	void SetMarquee(const TArray<FEcxSlide>& Slides);
	/** The monument's LED strip: one line scrolling at 1.2 m/s on both faces. */
	void SetTicker(const FString& Line);

	// ------------------------------------------------------------ the 14
	/** Starts the arrival: from down the ridge road at 9 m/s into the bay, stopping at the stop 6 s from now. */
	void BusArrive();
	/** At the stop now (the arrival skipped, or leaving: waiting), knelt and its doors open or not. */
	void BusAtStop(bool bOpen);
	/** Doors shut, up off its knees, out into the lane and away north (hidden past the north stand). */
	void BusDepart();
	/** At the stop: kneel and open the front door, or close it and rise. */
	void BusSetDoors(bool bOpen);
	EEcxBusPhase GetBusPhase() const { return BusPhase; }
	/** Seconds since the phase began. */
	float GetBusClock() const { return BusClock; }
	/** How far open the front door is (0 shut .. 1 open). */
	float GetBusDoors() const { return BusDoor; }
	/** The front door's middle at the curb (world). */
	FVector BusDoorPoint() const;
	/** The destination signs: up the ridge ("14 EMBERCREST") or home ("14 NORTHSIDE · FIFTH ST"). */
	void SetBusSign(bool bHome);
	/** The stopped bus blocks the hero (off while someone steps off it). */
	void SetBusBlocking(bool bBlock);
	/** A floor in the bus's door well for the step-off (the bus itself doesn't collide while the hero is in it). */
	void SetStepOffFloor(bool bOn);
	/** What the bus did since the last call (EcxBusEvent bits). */
	uint32 TakeBusEvents();

	// ------------------------------------------------------------ for the sound (0 out of earshot .. 1 close)
	float FireNear(const FVector& Ear) const;
	float WaterNear(const FVector& Ear) const;
	float DoorsNear(const FVector& Ear) const;
	float BusNear(const FVector& Ear) const;
	float ValleyNear(const FVector& Ear) const;
	/** Snow (or the terrain) under a footstep's trace: a crunch rather than a splash on wet stone. */
	bool IsSnow(const FHitResult& Hit) const;
	/** The fire bowls' flicker now (0.6..1.3, the nearest bowl to Ear). */
	float FireFlicker(const FVector& Ear) const;

	/** One line for TestDescribe: the hour, the sun, the exposure clamps, the bus. */
	FString Describe() const;

	// ------------------------------------------------------------ tuning
	/** Stops added to the exposure (+ brighter). */
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	float ExposureBias = 0.0f;
	/** The editor's preview of the world (construction builds it; nothing of it is saved). */
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	bool bEditorPreview = true;
	/**
	 * The volumetric clouds' material, outside the always-cooked content: resolved in the editor and saved with the level,
	 * so a cooked build carries it.
	 */
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	TObjectPtr<UMaterialInterface> CloudMaterial;

private:
	void ResetParts();
	void BuildAll();
	void BuildGround();
	void BuildSteps();
	void BuildBuilding();
	void BuildEntryDoors();
	void BuildCanopy();
	void BuildTower();
	void BuildPlaza();
	void BuildFireBowls();
	void BuildFixtures();
	void BuildLandscape();
	void BuildTrees();
	void BuildFar();
	void BuildSky();
	void BuildLights();
	void BuildScreens();
	void BuildBus();
	void BuildBlockers();
	void BuildSpots();

	void ApplyHour();
	void ApplyLens();
	void UpdateFires(float Dt);
	void UpdateDoors(float Dt);
	void UpdateScreens(float Dt);
	void UpdateSteam(const FVector& Camera, float Dt);
	void UpdateBus(float Dt);
	void PlaceBus(const FVector& At, float HeadingYaw, float Speed, float Dt);
	void DrawSlide(int32 Page, const FEcxSlide& Slide);
	void DrawTicker();
	UTextureRenderTarget2D* MakeTarget(int32 Width, int32 Height);
	void RenderToTarget(UTextureRenderTarget2D* Target, const TSharedPtr<const ss::ui::DrawList>& List);

	// Parts.
	template <typename T>
	T* NewPart(USceneComponent* Parent = nullptr);
	/** An imported SITE mesh (at the origin, turned half round), nullptr when it isn't imported. */
	UStaticMeshComponent* Site(const TCHAR* Name, bool bCollide = false, const FVector& Scale = FVector(1.0));
	/** An imported PROP mesh at At turned to Yaw (its own front faces -X at yaw 0), nullptr when it isn't imported. */
	UStaticMeshComponent* Prop(const TCHAR* Name, const FVector& At, float Yaw, const FVector& Scale = FVector(1.0), USceneComponent* Parent = nullptr);
	UStaticMeshComponent* Box(UMaterialInterface* Material, const FVector& Min, const FVector& Max, bool bCollide = true, USceneComponent* Parent = nullptr);
	/** A box turned about Z (Yaw) around its center. */
	UStaticMeshComponent* TurnedBox(UMaterialInterface* Material, const FVector& Center, const FVector& Size, float Yaw, bool bCollide = true, USceneComponent* Parent = nullptr);
	/** A slab whose top runs from (X0, Z0) to (X1, Z1) across Y0..Y1, Thick deep (a ramp, a sloped road). */
	UStaticMeshComponent* Slope(UMaterialInterface* Material, double X0, double X1, double Y0, double Y1, double Z0, double Z1, double Thick, bool bCollide = true);
	/** The same with its slope along Y. */
	UStaticMeshComponent* SlopeY(UMaterialInterface* Material, double X0, double X1, double Y0, double Y1, double Z0, double Z1, double Thick, bool bCollide = true);
	UStaticMeshComponent* Cyl(UMaterialInterface* Material, const FVector& Base, float Radius, float Height, bool bCollide = false, USceneComponent* Parent = nullptr);
	/** An engine cone with its base on Base, Radius across and Height tall (fitted by its bounds). */
	UStaticMeshComponent* Cone(UMaterialInterface* Material, const FVector& Base, float Radius, float Height, bool bCollide = false);
	/** An invisible box that blocks pawns only (the set's open edges). */
	void PawnWall(const FVector& Min, const FVector& Max);
	/**
	 * An invisible box that blocks everything (what the art doesn't: the buildings, the props). The camera's arm passes
	 * through it unless bStopCamera (the building's faces: the camera stays outside the glass and the stone).
	 */
	void Block(const FVector& Min, const FVector& Max, float Yaw = 0.0f, USceneComponent* Parent = nullptr, bool bStopCamera = false);
	/** A plain surface (M_Surface): color, roughness, metal, glow. */
	UMaterialInstanceDynamic* Mat(FName Key, uint32 SrgbHex, float Roughness, float Metallic = 0.0f, float Emissive = 0.0f);
	/** A ground surface (M_ECX_Ground, else M_Street, else plain): Kind 0 paver, 1 sett, 2 asphalt, 3 curb, 4 concrete, 5 wallstone, 7 marking. */
	UMaterialInstanceDynamic* GroundMat(int32 Kind, bool bWorldUV);
	UMaterialInstanceDynamic* SnowMat(bool bWorldUV = true);
	/** One of the game's glows (EEcxGlow): its color and strength follow the hour. */
	UMaterialInstanceDynamic* GlowMat(uint8 Family, float Scale = 1.0f);
	UMaterialInterface* GlassMat(bool bTint);
	/** M_ECX_Screen over a render target (or text alone, no image), its UVs from the mesh or, on a stand-in box, its local position. */
	UMaterialInstanceDynamic* ScreenMat(FName Key, bool bLocalUV);
	/** Every slot of an imported glow mesh takes the family's glow. */
	void GlowAll(UStaticMeshComponent* C, uint8 Family, float Scale = 1.0f);
	/** A slot of an imported mesh by its Blender index (baked) or its material's name (unbaked); INDEX_NONE when absent. */
	static int32 SlotOf(const UStaticMeshComponent* C, int32 Index, const TCHAR* Name = nullptr);
	void SetSlot(UStaticMeshComponent* C, int32 Slot, UMaterialInterface* M);
	/** Registers an imported mesh's [lit] slots (Floors per Blender index; -1 not lit) for the hour's dimming. */
	void Lit(UStaticMeshComponent* C, std::initializer_list<float> Floors, float Scale = 1.0f);
	/**
	 * Local lights under the hour's rule (EEcxRule), in candelas: Kelvin above 0 sets a color temperature, else ColorHex.
	 * Volumetric is the light's share in the night's volumetric fog.
	 */
	USpotLightComponent* SpotAt(uint8 Rule, float Candela, const FVector& At, const FRotator& Facing, float Kelvin, uint32 ColorHex, float Attenuation, float Inner, float Outer,
		bool bShadows, float Volumetric, USceneComponent* Parent = nullptr);
	URectLightComponent* RectAt(uint8 Rule, float Candela, const FVector& At, const FRotator& Facing, float Kelvin, uint32 ColorHex, float Attenuation, float Width, float Height,
		bool bShadows, float Volumetric, USceneComponent* Parent = nullptr);
	UPointLightComponent* PointAt(uint8 Rule, float Candela, const FVector& At, float Kelvin, uint32 ColorHex, float Attenuation, float SourceRadius, bool bShadows, float Volumetric,
		USceneComponent* Parent = nullptr);
	void AddLight(ULightComponent* L, uint8 Rule, float Level);
	/**
	 * A sign drawn once into a render target on a stand-in box facing Yaw (0 faces +X): EMBERCREST on the canopy, the
	 * monument's lettering. A first line "@mark" draws the brand's crest instead of text. It glows in Family's light.
	 */
	void TextPanel(FName Key, const TArray<FString>& Lines, const FVector& Center, const FVector2D& SizeCm, float Yaw, uint8 Family, float Thick = 2.0f, USceneComponent* Parent = nullptr);
	/** A bar of square section Thick from A to B (the crown's edges, struts). */
	UStaticMeshComponent* Bar(UMaterialInterface* Material, const FVector& A, const FVector& B, float Thick, bool bCollide = false, USceneComponent* Parent = nullptr);
	/** Instanced copies of Mesh (the stand-ins' repeats, the forest), no collision. */
	UInstancedStaticMeshComponent* Instances(UStaticMesh* InMesh, UMaterialInterface* Material, bool bShadows = true);
	/** The ground's height under a point: traced on the imported ground or terrain where it is, else the stand-in's. */
	double GroundZ(double X, double Y) const;
	/** The stand-in terrain's height (the slabs BuildLandscape lays when SM_ECX_SiteTerrain isn't imported). */
	static double FallbackTerrainZ(double X, double Y);

	UPROPERTY()
	TObjectPtr<USceneComponent> Root;
	UPROPERTY()
	TObjectPtr<UCameraComponent> CineCam;
	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> CylinderMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> SphereMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> ConeMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> PlaneMesh;
	UPROPERTY()
	TObjectPtr<UMaterialInterface> FallbackMaterial;

	// The built world (transient: built when play begins, or for the editor's preview).
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UMaterialInstanceDynamic>> Materials;
	UPROPERTY(Transient)
	TArray<FEcxLight> Lights;
	UPROPERTY(Transient)
	TArray<FEcxGlow> Glows;
	UPROPERTY(Transient)
	TArray<FEcxLit> LitMeshes;
	UPROPERTY(Transient)
	TArray<FEcxFire> Fires;
	UPROPERTY(Transient)
	TArray<FEcxSpot> Spots;
	/** Materials that read the hour (snow sparkle, mist, the flames by day, the falls): their Day/Night parameters. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> HourMids;
	/** Snow surfaces (a footstep crunches on them). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInterface>> SnowMids;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> SnowParts;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Steam;
	UPROPERTY(Transient)
	TArray<FVector> SteamBase;
	/** The imported ground and terrain (their complex collision carries the hero), when they are. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> GroundMesh;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> TerrainMesh;
	/** The near ridges round the site (the forest is set on them), when they're imported. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> RidgeMesh;

	// The sky.
	UPROPERTY(Transient)
	TObjectPtr<UDirectionalLightComponent> Sun;
	UPROPERTY(Transient)
	TObjectPtr<UDirectionalLightComponent> Moon;
	UPROPERTY(Transient)
	TObjectPtr<USkyAtmosphereComponent> Atmosphere;
	UPROPERTY(Transient)
	TObjectPtr<UVolumetricCloudComponent> Clouds;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CloudMid;
	UPROPERTY(Transient)
	TObjectPtr<USkyLightComponent> SkyLight;
	UPROPERTY(Transient)
	TObjectPtr<UExponentialHeightFogComponent> Fog;
	UPROPERTY(Transient)
	TObjectPtr<UPostProcessComponent> Lens;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> StarMid;
	/** The stars' sphere (hidden by day: a full-screen additive pass with nothing to add). */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> StarDome;
	UPROPERTY(Transient)
	TObjectPtr<URectLightComponent> MarqueeSpill;

	// The doors.
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> RevWings;
	UPROPERTY(Transient)
	TArray<TObjectPtr<USceneComponent>> SwingLeaves;
	TArray<float> SwingOpen;
	TArray<float> SwingTarget;
	TArray<float> SwingSign;
	float RevAngle = 0.0f;
	float RevSpeed = 12.0f;
	float RevTarget = 12.0f;

	// The screens.
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MarqueeMid;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> TickerMid;
	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> MarqueePages[2];
	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> TickerTarget;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextureRenderTarget2D>> PanelTargets;
	UPROPERTY(Transient)
	TObjectPtr<UTexture> MarqueeImages[2];
	UPROPERTY(Transient)
	TArray<FEcxSlide> Slides;
	FString TickerLine;
	int32 SlideIndex = -1;
	int32 FrontPage = 0;
	float SlideClock = 0.0f;
	float PageMix = 1.0f;
	float TickerScroll = 0.0f;
	float TickerSpan = 1.0f;
	bool bTickerDirty = false;
	/** What paints the screens' draw lists into their targets (CoreCanvasTarget.h). */
	FCoreCanvasTarget Painter;

	// The 14.
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> BusRoot;
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> BusBody;
	UPROPERTY(Transient)
	TArray<TObjectPtr<USceneComponent>> BusDoors;
	UPROPERTY(Transient)
	TArray<TObjectPtr<USceneComponent>> BusWheels;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> BusSignMids;
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULightComponent>> BusRunLights;
	UPROPERTY(Transient)
	TObjectPtr<ULightComponent> BusDoorLight;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> BusBlocker;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> BusFloor;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> BusLampsMesh;
	UPROPERTY(Transient)
	TObjectPtr<UTexture> BusSignTextures[3];
	TArray<FVector2D> BusPath;
	TArray<float> BusPathS;
	EEcxBusPhase BusPhase = EEcxBusPhase::Gone;
	float BusClock = 0.0f;
	float BusDoor = 0.0f;
	float BusDoorTarget = 0.0f;
	float BusKneel = 0.0f;
	float BusKneelTarget = 0.0f;
	float BusSpeed = 0.0f;
	float BusSpin = 0.0f;
	float BusHeading = 90.0f;
	FVector BusAt = FVector(5395.0, -428.0, -240.0);
	uint32 BusEvents = 0;
	bool bBusBlocking = true;
	bool bBusRunning = false;

	// The look.
	double Hour = 19.0;
	double SunEl = 2.0;
	double AppliedHour = -100.0;
	float CapturedHour = -100.0f;
	float BrightnessBias = 0.0f;
	float GrainScale = 1.0f;
	float FringeScale = 1.0f;
	bool bMotionBlur = false;
	float EvMin = 3.5f;
	float EvMax = 6.5f;
	float AppliedGlowScale = -1.0f;
	/** The look's tunables (ss.ECX.*) as the hour was last applied with them. */
	float AppliedTuning = -1.0f;
	float Clock = 0.0f;
	bool bBuilt = false;
	/** Render settings the exterior sets for itself (Lumen's far field, the clouds' quality) and what they were before. */
	TArray<TPair<FString, FString>> SavedCvars;
	void SetCvar(const TCHAR* Name, const TCHAR* Value);
	void RestoreCvars();
};

template <typename T>
T* AEmbercrestStage::NewPart(USceneComponent* Parent)
{
	// Transient: built when play begins (or for the editor's preview), never saved with the level. The preview's parts are
	// not duplicated either: Play-In-Editor copies the level by duplication, which copies transient objects that are still
	// referenced (BlueprintCreatedComponents is), and the play copy builds its own world at BeginPlay.
	const UWorld* InWorld = GetWorld();
	const EObjectFlags Flags = InWorld && InWorld->IsGameWorld() ? RF_Transient : EObjectFlags(RF_Transient | RF_DuplicateTransient);
	T* Part = NewObject<T>(this, NAME_None, Flags);
	Part->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	Part->SetupAttachment(Parent ? Parent : Root.Get());
	BlueprintCreatedComponents.Add(Part);
	Part->RegisterComponent();
	return Part;
}
