#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "ShortStack/UI/Canvas.h"
#include "StreetWorld.h"

#include "ShopStage.generated.h"

class SDrawListWidget;
class UBoxComponent;
class ULightComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPointLightComponent;
class UPostProcessComponent;
class URectLightComponent;
class USpotLightComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UWidgetComponent;

/** How a shop's door opens (design spec 5.0). */
UENUM()
enum class EShopDoorKind : uint8
{
	/** A swing door with a bell over it: the barber, the thrift, Ruby's. */
	Swing,
	/** The gym's glass door: a member's fob at any hour, buzzed in while it's staffed. */
	Fob,
	/** The pawn's glass door on a buzzer lock: E while open buzzes, the keeper buzzes you in (BuzzSeconds), the lock clacks. */
	Buzzer,
};

/** What a shop light is for, which decides when it's on (ApplyLights; spec 3.8's sleep rule). */
UENUM()
enum class EShopLight : uint8
{
	/** An interior fixture: on while open; asleep, only the two brightest stay on, unshadowed. */
	Fixture,
	/** A night light: on while the shop is shut. */
	Night,
	/** The window's light on the sidewalk: on while open (lit), never shadowed. */
	Spill,
	/** A sign or neon: on while open. */
	Sign,
	/** A sign lit all night, open or not (the pawn's blue tube). */
	SignAlways,
};

/** A shop's door as its block-out or its art hangs it (stage cm, spec 2.4). */
USTRUCT()
struct FShopDoorSpec
{
	GENERATED_BODY()

	/** The hinge jamb's foot (stage cm). */
	UPROPERTY()
	FVector Hinge = FVector::ZeroVector;
	/** Stage yaw (degrees) from the hinge along the shut leaf to its latch edge. */
	UPROPERTY()
	float ShutYaw = 0.0f;
	/** Stage yaw looking in through the doorway from the street. */
	UPROPERTY()
	float InYaw = 0.0f;
	/** How far it swings open (degrees, about the hinge; positive turns it counter-clockwise seen from above). */
	UPROPERTY()
	float OpenYaw = 95.0f;
	UPROPERTY()
	float Width = 95.0f;
	UPROPERTY()
	float Height = 215.0f;
	UPROPERTY()
	EShopDoorKind Kind = EShopDoorKind::Swing;
	/** Glass in it (the gym's, the pawn's), or a solid leaf with a glass light (the barber's). */
	UPROPERTY()
	bool bGlass = true;
};

/**
 * One enterable shop on the street (design spec 7.5.5): its room built when play begins inside Street.umap (no level load:
 * a walk-in), registered with the street's registry (UStreetWorld) so the game mode, the crowd and the people find its
 * uses, stations, shots, zone, start and door without knowing which shop it is. Each shop is a subclass (BarberStage.cpp,
 * ThriftStage.cpp, GymStage.cpp, DinerStage.cpp, PawnStage.cpp, each UCLASS declared in its own header: UHT reads headers
 * only) that sets Venue and VenueName in its constructor, builds its room in BuildInterior with the helpers below, art where
 * it's imported and engine shapes at the spec's coordinates where it isn't (block-outs first, spec 8.3), and registers its
 * static AppendPreload (SHORTSTACK_STREET_PRELOAD). The stage that owns its plot spawns it by venue (FindOrSpawnVenue).
 *
 * Frame: the shop stands at the street stage's transform, and every helper takes the stage's cm (spec 0.3): X 0 the home
 * side's building line, +X west, +Y north, Z 0 the sidewalk's top. SITE meshes go at the origin turned 180, PROP meshes at
 * their spot turned to face it plus 180 (spec 0.4).
 *
 * At play the game mode tells it the hour (SetHour: open and staffed from core's shop hours), keeps its door going for the
 * hero (UpdateDoor, each frame) and opens it on E (OpenDoor). It wakes and sleeps by itself (spec 3.8: awake with the hero
 * inside, within 25 m, or within 40 m with its glass in view); asleep it keeps its meshes and glows and only its two
 * brightest fixtures and its spill, unshadowed. In the editor construction builds a preview (nothing saved), as the street
 * does; nothing is registered there.
 */
UCLASS(Abstract)
class SHORTSTACK_API AShopStage : public AActor
{
	GENERATED_BODY()

public:
	AShopStage();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;

	/** The shop for StageFrame's street (the street stage's transform): one already in the level of Class, or a new one; built. */
	static AShopStage* FindOrSpawn(UWorld* World, TSubclassOf<AShopStage> Class, const FTransform& StageFrame);
	/** The same by venue ("barber"): whichever shop class says it's that venue (ClassFor), so a stage spawning it needn't include its header. Null when none is. */
	static AShopStage* FindOrSpawnVenue(UWorld* World, FName InVenue, const FTransform& StageFrame);
	/** The (non-abstract) shop class whose default Venue is InVenue, or null. */
	static TSubclassOf<AShopStage> ClassFor(FName InVenue);

	/** "barber", "thrift", "gym", "diner", "pawn" (core's venue id). */
	UPROPERTY(VisibleAnywhere, Category = "Short Stack")
	FName Venue;
	/** Its name on the HUD and in its closed line: "Fifth St. Barbers". */
	UPROPERTY(VisibleAnywhere, Category = "Short Stack")
	FString VenueName;
	/** The editor's preview of the room (construction builds it; nothing of it is saved). */
	UPROPERTY(EditAnywhere, Category = "Short Stack")
	bool bEditorPreview = true;

	/** Builds the room once for play: BuildInterior, then the door, the post-process volume, the lights by the hour, the registry's entries. */
	void EnsureBuilt();
	bool IsBuilt() const { return bBuilt; }
	/**
	 * The hour (world minutes, Session::Daylight) and whether it's open and staffed now (core's shop hours): the grille, the
	 * sign and the lights follow, and the door locks while it's shut. ClosedLine is what its door says then ("Fifth St.
	 * Barbers - Closed - Opens Tue 10 AM"); empty keeps the last.
	 */
	void SetHour(double InWorldMinutes, float InDaylight, bool bInOpen, bool bInStaffed, const FString& InClosedLine = FString());
	bool IsOpen() const { return bOpen; }
	bool IsStaffed() const { return bStaffed; }
	/** Awake: everything lit and moving; asleep: spec 3.8's few lights (it decides for itself each frame unless held). */
	void SetAwake(bool bInAwake);
	bool IsAwake() const { return bAwake; }
	/** Holds it awake or asleep whatever the camera does (ss.Shop tests); -1 lets it decide again. */
	void HoldAwake(int32 Hold) { AwakeHold = Hold; }
	/**
	 * The door each frame, Someone (the hero) keeping it from shutting on them and walkers (the registry's) opening it while
	 * it's open: 1 the moment it starts to open (the bell, the buzzer's clack), 2 the moment it shuts, else 0. Locked while
	 * the shop is shut.
	 */
	int32 UpdateDoor(const FVector& Someone, float Dt);
	/**
	 * E at the door: "" when it opens (or is being buzzed), else what to say (the closed line). bMember: the hero has the
	 * gym's fob. From inside (the hero as UpdateDoor last saw them) it always opens: locked to the street, never to the room.
	 */
	FString OpenDoor(bool bMember);
	/** How far open the door is (0 shut .. 1 open). */
	float DoorOpenAmount() const { return DoorAmount; }
	/** Whether At (world) is in the shop's room. */
	bool IsInside(const FVector& At) const;
	/** The service's cinematic: (registry shot, seconds) in order, ending on its fade (spec 7.5.3). None by default. */
	virtual TArray<TPair<FName, float>> Cinematic(FName Service) const;
	/** One line for ss.Street.Describe and ss.Shop tests. */
	FString Describe() const;

	/** Every asset the shop loads (none here; each subclass has its own, registered with SHORTSTACK_STREET_PRELOAD). */
	static void AppendPreload(TArray<FString>& Paths) {}
	/** An imported Blender mesh (nullptr when it isn't): what the block-outs stand in for. */
	static UStaticMesh* ShopMesh(const TCHAR* Name);

protected:
	/** The room: every mesh, light, glow, screen, use, station, shot, zone and start (the helpers below), and the door (Door). */
	virtual void BuildInterior() PURE_VIRTUAL(AShopStage::BuildInterior, );
	/** After SetHour changed something (open, staffed): the subclass's own signs and dressing. */
	virtual void OnHour() {}
	/** After SetAwake changed it. */
	virtual void OnAwake() {}
	/** Each frame while awake (what moves in it: a fan, a belt, the pie case). */
	virtual void Animate(float Dt) {}
	/** What the door's use says while it can be opened: "Open the door", "Tap your fob", "Buzz". */
	virtual FString DoorPrompt() const;
	/** What it says while the shop is shut (the closed line, or the venue's name and "Closed"). */
	virtual FString ClosedPrompt() const;

	// ------------------------------------------------------------ helpers (stage cm, stage yaw)
	template <typename T>
	T* NewPart(USceneComponent* Parent = nullptr);
	/** M_Street (the street's procedural material) in a color, keyed so each look is made once. */
	UMaterialInstanceDynamic* Mat(FName Key, uint32 SrgbHex, float Roughness, float Pattern = 0.0f, float Emissive = 0.0f, float Metallic = 0.0f);
	/** A block-out box from Min to Max (collision: blocks everything). */
	UStaticMeshComponent* Block(UMaterialInterface* Material, const FVector& Min, const FVector& Max, bool bCollide = true, USceneComponent* Parent = nullptr);
	/** A SITE mesh (spec 0.4) at the stage's origin, turned 180 (nullptr when it isn't imported). No collision: Block what should. */
	UStaticMeshComponent* Site(const TCHAR* Name);
	/** A PROP mesh at At facing stage yaw Yaw (turned Yaw + 180; nullptr when it isn't imported). */
	UStaticMeshComponent* Mesh(const TCHAR* Name, const FVector& At, float Yaw, bool bCollide = false);
	/** A light for Role (interior fixtures shadowed and out of the volumetric fog; spills and signs unshadowed). Candela at its brightest. */
	UPointLightComponent* PointLight(const FVector& At, float Candela, uint32 SrgbHex, float Radius, EShopLight LightRole);
	USpotLightComponent* SpotLight(const FVector& At, const FRotator& Aim, float Candela, uint32 SrgbHex, float Radius, float InnerDeg, float OuterDeg, EShopLight LightRole);
	URectLightComponent* RectLight(const FVector& At, const FRotator& Aim, float Width, float Height, float Candela, uint32 SrgbHex, float Radius, EShopLight LightRole);
	/** Something that glows for Role (a sign's tube, a lit case): its M_Street Emissive, or an imported mesh's emissive factors, scaled by it. */
	void Glow(UMaterialInstanceDynamic* Mid, EShopLight LightRole);
	void Glow(UStaticMeshComponent* Imported, EShopLight LightRole);
	/** A flat screen or sign drawn from core (ShopArt, PropArt): Paint draws it on a canvas of Pixels; Shine > 0 glows (a tint), 0 printed. */
	UWidgetComponent* Screen(const FVector& At, float Yaw, const FVector2D& SizeCm, const FIntPoint& Pixels, float Shine, TFunction<void(ss::ui::Canvas&)> Paint);
	/** Redraws a screen made by Screen (its index in creation order) with a new painter. */
	void Redraw(int32 Index, TFunction<void(ss::ui::Canvas&)> Paint);
	/** A use E reaches at At (stage cm); WhyNot unset: usable while open, the closed line while shut. Its registry handle. */
	int32 Use(FName Id, EStreetVerb Verb, const FVector& At, float Radius, const FString& Prompt, const FString& Param = FString(), TFunction<FString()> WhyNot = nullptr);
	/** Where someone works, sits or waits (their feet at At, facing Yaw), indoors. */
	void Station(FName Id, const FVector& At, float Yaw, FName Pose, float SeatHeight = 0.0f);
	/** A camera at Camera looking at LookAt (stage cm), its horizontal field of view. */
	void Shot(FName Id, const FVector& Camera, const FVector& LookAt, float Fov);
	/** Where the hero appears (just inside the door, facing in: spec 2.10's "barber", "pawn"). */
	void Start(FName Id, const FVector& At, float Yaw);
	/**
	 * The room (stage cm): its zone (the place name, the room tone, dry, the camera inside: spec 3.8's shorter boom and wider
	 * view, -1 for the street's), its post-process volume (the inside exposure, spec 3.2), what wakes it.
	 */
	void Zone(const FBox& Room, const FString& Place, int32 RoomTone, float BoomLength = 140.0f, float Fov = 80.0f, bool bFirstPerson = false);
	/** Where its glass faces the street (stage cm: the storefront's middle and the way out): the 40 m wake rule looks at it. */
	void Storefront(const FVector& Middle, const FVector& Out);
	/** The door: hung on its hinge (a block-out leaf, or Leaf when the art has one), its stop, its use ("<venue>-door") and the crowd's door. */
	void Door(const FShopDoorSpec& Spec, UStaticMeshComponent* Leaf = nullptr);
	/** The grille over the storefront (stage cm), rolled up while open and down while shut. */
	void Grille(const FVector& Min, const FVector& Max);

	UPROPERTY()
	TObjectPtr<USceneComponent> Root;
	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY()
	TObjectPtr<UMaterialInterface> FallbackMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> StreetMaterial;

	/** How long the buzzer takes (the keeper looks up and buzzes). */
	float BuzzSeconds = 0.6f;
	/** The hour as last set (world minutes) and the daylight (0 night .. 1 day). */
	double WorldMinutes = 0.0;
	float Daylight = 0.0f;

private:
	struct FLight
	{
		TWeakObjectPtr<ULightComponent> Light;
		float Base = 0.0f;
		EShopLight Role = EShopLight::Fixture;
	};
	struct FGlow
	{
		TWeakObjectPtr<UMaterialInstanceDynamic> Mid;
		TWeakObjectPtr<UStaticMeshComponent> Mesh;
		EShopLight Role = EShopLight::Fixture;
		/** The parameters it glows by and their full values (an M_Street Emissive, or the imported factors). */
		TArray<TPair<FName, FLinearColor>> Colors;
		TArray<TPair<FName, float>> Strengths;
	};

	void ResetParts();
	void Unregister();
	void ApplyLights();
	void ApplyExposure();
	void UpdateWake(float Dt);
	UStreetWorld* Registry() const;
	ULightComponent* AddLight(ULightComponent* L, float Candela, uint32 SrgbHex, EShopLight LightRole);
	/** Whether Pos (world) is at the doorway (within Reach across its line, either side) and, InSwing, in the leaf's way. */
	bool AtDoorway(const FVector& Pos, double Reach) const;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UMaterialInstanceDynamic>> Materials;
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> DoorLeaf;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> DoorStop;
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> GrilleRoot;
	UPROPERTY(Transient)
	TObjectPtr<UBoxComponent> RoomVolume;
	UPROPERTY(Transient)
	TObjectPtr<UPostProcessComponent> RoomLens;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidgetComponent>> Screens;
	TArray<FLight> Lights;
	TArray<FGlow> Glows;
	TArray<TSharedPtr<SDrawListWidget>> ScreenSlates;

	FShopDoorSpec DoorSpec;
	bool bHasDoor = false;
	FBox RoomBox = FBox(ForceInit);
	FVector GlassAt = FVector::ZeroVector;
	FVector GlassOut = FVector::ZeroVector;
	bool bHasGlass = false;
	double GrilleHeight = 0.0;
	FString PlaceName;
	FString ClosedLine;

	// What it registered (taken away at EndPlay).
	TArray<int32> UseHandles;
	TArray<FName> StartIds;
	TArray<FName> ShotIds;

	bool bBuilt = false;
	bool bOpen = true;
	bool bStaffed = true;
	bool bAwake = true;
	int32 AwakeHold = -1;
	float WakeCheckIn = 0.0f;
	float DoorAmount = 0.0f;
	/** The hero (UpdateDoor's Someone) is in the room: the door lets them out whatever the hour. */
	bool bSomeoneInside = false;
	bool bDoorHeld = false;
	float DoorClear = 0.0f;
	/** The buzzer: seconds until the lock lets go (negative: not buzzing). */
	float BuzzIn = -1.0f;
	float GrilleOpen = 1.0f;
	float AppliedDaylight = -1.0f;
};

template <typename T>
T* AShopStage::NewPart(USceneComponent* Parent)
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
