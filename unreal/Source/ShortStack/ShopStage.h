#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "ShortStack/UI/Canvas.h"
#include "StreetWorld.h"

#include "ShopStage.generated.h"

class SDrawListWidget;
class UBoxComponent;
class UInstancedStaticMeshComponent;
class ULightComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPointLightComponent;
class UPostProcessComponent;
class URectLightComponent;
class USceneCaptureComponent2D;
class USpotLightComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTextureRenderTarget2D;
class UWidgetComponent;

namespace ss
{
class Session;
namespace ui
{
struct HostCaps;
}
} // namespace ss

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
	/** How far it swings open (degrees, about the hinge, added to the leaf's stage yaw: +90 turns a leaf along +X to +Y). */
	UPROPERTY()
	float OpenYaw = 95.0f;
	/** One leaf's width (a pair's two leaves each this wide). */
	UPROPERTY()
	float Width = 95.0f;
	UPROPERTY()
	float Height = 215.0f;
	UPROPERTY()
	EShopDoorKind Kind = EShopDoorKind::Swing;
	/** Glass in it (the gym's, the pawn's), or a solid leaf with a glass light (the barber's). */
	UPROPERTY()
	bool bGlass = true;
	/** A pair of leaves meeting in the middle (the thrift's, Ruby's): the second hinged at Hinge2, its leaf along ShutYaw2. */
	UPROPERTY()
	bool bPair = false;
	UPROPERTY()
	FVector Hinge2 = FVector::ZeroVector;
	UPROPERTY()
	float ShutYaw2 = 0.0f;
	UPROPERTY()
	float OpenYaw2 = -95.0f;
};

/** How the shops' mirrors reflect (ss.Shop.Mirror; spec 3.4's day-2 spike and its fallback). */
namespace ShopMirror
{
/** Lumen with hit lighting for reflections in the shop's own volume only (the spike's first answer). */
constexpr int32 Lumen = 0;
/** A SceneCapture2D mirrored in the glass (960 wide, 30 Hz, the shop's actors and the people in it), only while live. */
constexpr int32 Capture = 1;
/** The project's default Lumen reflections (surface cache): the cheapest, for measuring against. */
constexpr int32 Plain = 2;
} // namespace ShopMirror

/**
 * One enterable shop on the street (design spec 7.5.5): its room built when play begins inside Street.umap (no level load:
 * a walk-in), registered with the street's registry (UStreetWorld) so the game mode, the crowd and the people find its
 * uses, stations, shots, zone, start and door without knowing which shop it is. Each shop is a subclass (BarberStage.cpp,
 * ThriftStage.cpp, GymStage.cpp, DinerStage.cpp, PawnStage.cpp, each UCLASS declared in its own header: UHT reads headers
 * only) that sets Venue and VenueName in its constructor, builds its room in BuildInterior with the helpers below, art where
 * it's imported and engine shapes at the spec's coordinates where it isn't (block-outs first, spec 8.3), and registers its
 * static AppendPreload (SHORTSTACK_STREET_PRELOAD). The stage that owns its plot spawns it by venue (FindOrSpawnVenue; the
 * street's game mode makes sure each of the five stands, so a shop works before Market's stage lands).
 *
 * Frame: the shop stands at the street stage's transform, and every helper takes the stage's cm (spec 0.3): X 0 the home
 * side's building line, +X west, +Y north, Z 0 the sidewalk's top. SITE meshes go at the origin turned 180, PROP meshes at
 * their spot turned to face it plus 180 (spec 0.4).
 *
 * At play the game mode tells it the hour (SetHour: open and staffed from core's shop hours), keeps its door going for the
 * hero (UpdateDoor, each frame), opens it on E (OpenDoor) and shows it the session (SyncSession: the pawn's shelf, the
 * thrift's racks, the boards). It wakes and sleeps by itself (spec 3.8: awake with the hero inside, within 25 m, or within
 * 40 m with its glass in view); asleep it keeps its meshes and glows and only its two brightest fixtures and its spill,
 * unshadowed. In the editor construction builds a preview (nothing saved), as the street does; nothing is registered there.
 *
 * What the old street built where the room stands (today's massing blocks round the walk-up, Market's end wall) is carved
 * away at play (Carve): any engine-cube block-out of another stage overlapping the room is hidden and replaced by the pieces
 * of it outside the room, so a shop works on today's street and on Market's once its stage leaves the openings clear.
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
	/** The five enterable shops' venues, in the street's order (spec 0.1). */
	static const TArray<FName>& Venues();

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
	 * Barbers · Closed · Opens Tue 10 AM"); empty keeps the last.
	 */
	void SetHour(double InWorldMinutes, float InDaylight, bool bInOpen, bool bInStaffed, const FString& InClosedLine = FString());
	bool IsOpen() const { return bOpen; }
	bool IsStaffed() const { return bStaffed; }
	/** Whether the hero has the gym's fob (the gym's door opens to it at any hour). */
	void SetMember(bool bInMember) { bMember = bInMember; }
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
	 * E at the door: "" when it opens (or is being buzzed), else what to say (the closed line). bInMember: the hero has the
	 * gym's fob. From inside (the hero as UpdateDoor last saw them) it always opens: locked to the street, never to the room.
	 */
	FString OpenDoor(bool bInMember);
	/** What E at the door says now: "Open the door", "Tap your fob", "Buzz in", "Buzz", or the closed line ("" while it stands open). */
	FString DoorPromptNow() const;
	/** The buzzer is buzzing (the keeper's hand on the button): the pawn's clack follows when it lets go. */
	bool IsBuzzing() const { return BuzzIn >= 0.0f; }
	/** How far open the door is (0 shut .. 1 open). */
	float DoorOpenAmount() const { return DoorAmount; }
	/** The middle of the doorway (world, a meter up). */
	FVector DoorMiddle() const;
	/** Where someone put out of the shop stands: on the sidewalk outside its door, facing away from it (world). */
	FTransform OutsideDoor() const;
	/** Whether At (world) is in the shop's room. */
	bool IsInside(const FVector& At) const;
	/** The room (world). */
	FBox RoomBounds() const;
	/**
	 * The service's cinematic (spec 7.5.3): (registry shot, seconds) in order, played under the service's skip and scaled to
	 * its length; a negative seconds plays after the skip, once the look has changed under black (the barber's reveal).
	 * Service is the skip's offer ids ("barber/crew,barber/beard-short"). None by default.
	 */
	virtual TArray<TPair<FName, float>> Cinematic(FName Service) const;
	/** A seat (a use or station id: "barber-chair1", "diner-stool3"): the sitter's feet (world, facing the way they face), the seat's height (cm), where they stand up to. */
	bool SeatFor(FName Id, FTransform& OutFeet, float& OutHeight, FTransform& OutStand, FName* OutShot = nullptr) const;
	/**
	 * Where the hero does a service (the gym's treadmill, the bag, the bench): their feet (world), the seat's height (0
	 * standing) and the work beat (people-ue's names, AStreetNpcs::ServiceBeatFor: "treadmill", "bag", "weights"). False when the
	 * service is done where they are.
	 */
	virtual bool ServiceMark(FName Service, FTransform& OutFeet, float& OutSeat, FName& OutBeat) const { return false; }
	/** A service starting or ending under its skip (the gym's belt pans, the bag swings). */
	virtual void OnService(FName Service, bool bRunning) {}
	/** Where the hero stands for a shot (the thrift's fitting mirror), if the shot has a mark (world). */
	bool ShotMark(FName Shot, FTransform& Out) const;
	/** A shot asked for or let go (the screens' Camera request): the thrift draws its curtain, the mirrors go live. */
	virtual void OnShot(FName Shot, bool bActive);
	/** The session as the shop shows it (the pawn's shelf, the thrift's racks, the boards): the game mode calls it now and then, and after a screen closes. */
	virtual void SyncSession(const ss::Session& S) {}
	/** What this shop's 3D can show the screens (the pawn's shelf: HostCaps.GearKinds). */
	virtual void FillCaps(ss::ui::HostCaps& Caps) const {}
	/** The mirrors live (the hero in the barber's chair, at the thrift's three-way): the SceneCapture fallback renders only then. */
	void SetMirrorLive(bool bLive);
	/** ss.Shop.Mirror: how every shop's mirrors reflect (ShopMirror::Lumen, Capture, Plain). */
	static int32 MirrorMode();
	/** One line for ss.Street.Describe and ss.Shop tests. */
	FString Describe() const;

	/** Every asset the shop loads (none here; each subclass has its own, registered with SHORTSTACK_STREET_PRELOAD). */
	static void AppendPreload(TArray<FString>& Paths) {}
	/** The shops' own materials (shops_setup.py) and the shared ones they use: every subclass's AppendPreload adds these. */
	static void AppendShopMaterials(TArray<FString>& Paths);
	/** An imported Blender mesh (nullptr when it isn't): what the block-outs stand in for. */
	static UStaticMesh* ShopMesh(const TCHAR* Name);
	/** "$12.00" (whole cents). */
	static FString Dollars(int64 Cents);
	/**
	 * The garment kinds (a bit per wardrobe::Cut) the hero can be shown in today, until looks says (its FillHostCaps): the
	 * MetaHuman's tee and shorts and the street's sneakers always; a jacket, pants, a hat or glasses only where their meshes
	 * are imported (spec 5.0: the 3D only sells what it can show). The closet at home and the thrift's racks use it too.
	 */
	static uint64 WearableKinds();

	// ------------------------------------------------------------ painters: the faces the shops print (core's data, drawn here)
	/** A paper card or a decal: a heading and lines, centered (the hours card, BACK AT 4, a hand-lettered sign). */
	static void PaintCard(ss::ui::Canvas& C, const FString& Heading, const TArray<FString>& Lines, uint32 Paper, uint32 Ink, bool bTransparent = false);
	/** A letter board or a price board: a heading over name and price rows (the barber's prices, Ruby's menu). */
	static void PaintBoard(ss::ui::Canvas& C, const FString& Heading, const TArray<TPair<FString, FString>>& Rows, uint32 Board, uint32 Ink);
	/** A manila price tag on a string: the price, a line under it (the pawn's stock, LOT 114). */
	static void PaintTag(ss::ui::Canvas& C, const FString& Price, const FString& Under);
	/** The OPEN / CLOSED card hung in the door. */
	static void PaintOpenClosed(ss::ui::Canvas& C, bool bOpen);

protected:
	/** The room: every mesh, light, glow, screen, use, station, shot, zone and start (the helpers below), and the door (Door). */
	virtual void BuildInterior() PURE_VIRTUAL(AShopStage::BuildInterior, );
	/** After SetHour changed something (open, staffed): the subclass's own signs and dressing. */
	virtual void OnHour() {}
	/** After SetAwake changed it. */
	virtual void OnAwake() {}
	/** Each frame while awake (what moves in it: a fan, a belt, the pie case). The base turns the spinners and the clocks. */
	virtual void Animate(float Dt);
	/** What the door's use says while it can be opened: "Open the door", "Tap your fob", "Buzz". */
	virtual FString DoorPrompt() const;
	/** What it says while the shop is shut (the closed line, or the venue's name and "Closed"). */
	virtual FString ClosedPrompt() const;

	// ------------------------------------------------------------ helpers (stage cm, stage yaw)
	template <typename T>
	T* NewPart(USceneComponent* Parent = nullptr);
	/** Takes a part away (one made by NewPart: the pawn's stock as it changes). */
	void DropPart(UActorComponent* Part);
	/** M_Street (the street's procedural material) in a color, keyed so each look is made once. */
	UMaterialInstanceDynamic* Mat(FName Key, uint32 SrgbHex, float Roughness, float Pattern = 0.0f, float Emissive = 0.0f, float Metallic = 0.0f);
	/** Shop glass for a block-out pane: M_NS_Glass (ue-world's), else the street's store glass, else a dark sheet. */
	UMaterialInterface* GlassMat();
	/** A block-out box from Min to Max (collision: blocks everything). */
	UStaticMeshComponent* Block(UMaterialInterface* Material, const FVector& Min, const FVector& Max, bool bCollide = true, USceneComponent* Parent = nullptr);
	/** A wall, counter or floor that blocks whatever the art: drawn only while bShow (the block-out), always solid. */
	UStaticMeshComponent* Solid(UMaterialInterface* Material, const FVector& Min, const FVector& Max, bool bShow);
	/** A SITE mesh (spec 0.4) at the stage's origin, turned 180 (nullptr when it isn't imported). No collision: Block what should. */
	UStaticMeshComponent* Site(const TCHAR* Name);
	/** A PROP mesh at At facing stage yaw Yaw (turned Yaw + 180; nullptr when it isn't imported). */
	UStaticMeshComponent* Mesh(const TCHAR* Name, const FVector& At, float Yaw, bool bCollide = false);
	/** A PROP's own point (Blender meters in its frame: x toward its back, y to its left seen from the front, z up) in stage cm, for a PROP placed at At facing Yaw. */
	static FVector PropPoint(const FVector& At, float Yaw, const FVector& BlenderMeters);
	/** A light for Role (interior fixtures shadowed and out of the volumetric fog; spills and signs unshadowed). Candela at its brightest. */
	UPointLightComponent* PointLight(const FVector& At, float Candela, uint32 SrgbHex, float Radius, EShopLight LightRole);
	USpotLightComponent* SpotLight(const FVector& At, const FRotator& Aim, float Candela, uint32 SrgbHex, float Radius, float InnerDeg, float OuterDeg, EShopLight LightRole);
	URectLightComponent* RectLight(const FVector& At, const FRotator& Aim, float Width, float Height, float Candela, uint32 SrgbHex, float Radius, EShopLight LightRole);
	/** Something that glows for Role (a sign's tube, a lit case): its M_Street Emissive, or an imported mesh's emissive factors, scaled by it. */
	void Glow(UMaterialInstanceDynamic* Mid, EShopLight LightRole);
	/** An imported glow mesh (a _Glow<Family>, one white emissive slot): colored and scaled as the game sets it (spec 0.5). */
	void Glow(UStaticMeshComponent* Imported, EShopLight LightRole, uint32 TintSrgb = 0xffffff, float Level = 1.0f);
	/** A flat screen or sign drawn from core (ShopArt, PropArt): Paint draws it on a canvas of Pixels; Shine > 0 glows (a tint), 0 printed. */
	UWidgetComponent* Screen(const FVector& At, float Yaw, const FVector2D& SizeCm, const FIntPoint& Pixels, float Shine, TFunction<void(ss::ui::Canvas&)> Paint);
	/** Redraws a screen made by Screen (its index in creation order) with a new painter. */
	void Redraw(int32 Index, TFunction<void(ss::ui::Canvas&)> Paint);
	int32 ScreenIndex(const UWidgetComponent* Wc) const { return Screens.IndexOfByKey(Wc); }
	UWidgetComponent* ScreenAt(int32 Index) const;
	/** A use E reaches at At (stage cm); WhyNot unset: usable while open, the closed line while shut. Its registry handle. */
	int32 Use(FName Id, EStreetVerb Verb, const FVector& At, float Radius, const FString& Prompt, const FString& Param = FString(), TFunction<FString()> WhyNot = nullptr);
	/** Where someone works, sits or waits (their feet at At, facing Yaw), indoors. */
	void Station(FName Id, const FVector& At, float Yaw, FName Pose, float SeatHeight = 0.0f);
	/** A seat the hero can take (and, bStation, the station people sit at): the feet at Feet facing Yaw, the seat's top Height cm up, standing up to Stand. */
	void Seat(FName Id, const FVector& Feet, float Yaw, float Height, const FVector& Stand, FName Pose = FName(TEXT("Sit")), bool bStation = true, FName SeatShot = NAME_None);
	/** The sidewalk outside a door with no street under it yet (Market's shops before its stage): a slab a hair under where the street's will lie, and a zone so it's on the set. */
	void Porch(const FBox& Sidewalk, const FString& Place);
	/**
	 * A recessed entry between the door and the storefront's line (stage cm): a zone so whoever stands in it is on the set
	 * (the room's zone stops at the door, the street's at its own line; between them KeepOnTheSet would send them home). No
	 * slab: the shop's floor runs out to the glass.
	 */
	void Entry(const FBox& Box, const FString& Place);
	/** A camera at Camera looking at LookAt (stage cm), its horizontal field of view. */
	void Shot(FName Id, const FVector& Camera, const FVector& LookAt, float Fov);
	/** Where the hero stands for a shot (the thrift's mirror), facing Yaw. */
	void ShotMarkAt(FName Shot, const FVector& At, float Yaw);
	/** Where the hero appears (just inside the door, facing in: spec 2.10's "barber", "pawn"). */
	void Start(FName Id, const FVector& At, float Yaw);
	/**
	 * The room (stage cm): its zone (the place name, the room tone, dry, the camera inside: spec 3.8's shorter boom and wider
	 * view, -1 for the street's), its post-process volume (the inside exposure, spec 3.2), what wakes it.
	 */
	void Zone(const FBox& Room, const FString& Place, int32 RoomTone, float BoomLength = 140.0f, float Fov = 80.0f, bool bFirstPerson = false);
	/** Where its glass faces the street (stage cm: the storefront's middle and the way out): the 40 m wake rule looks at it. */
	void Storefront(const FVector& Middle, const FVector& Out);
	/** The door: hung on its hinge (a block-out leaf, or Leaf when the art has one; Leaf2 a pair's second), its stop, its use ("<venue>-door") and the crowd's door. */
	void Door(const FShopDoorSpec& Spec, UStaticMeshComponent* Leaf = nullptr, UStaticMeshComponent* Leaf2 = nullptr);
	/** The grille over the storefront (stage cm), rolled up while open and down while shut: the art's curtain (scaled in Z) or a block-out. */
	void Grille(const FVector& Min, const FVector& Max, UStaticMeshComponent* Art = nullptr);
	/**
	 * What the old street built inside Hole (stage cm) goes: every engine-cube block-out of another stage overlapping it is
	 * hidden and stood again as its pieces outside the hole (slivers under 2 cm dropped). Done at play once the street has
	 * built, and again whenever it builds again. Keep the hole off the ground (Z from 1 up): the sidewalks stay.
	 */
	void Carve(const FBox& Hole);
	/** Something that turns while the shop is awake (a fan's blades, the pie carousel, the barber's pole): PerSecond about its own axes; bOpenOnly: only while open. */
	void Spinner(USceneComponent* Part, const FRotator& PerSecond, bool bOpenOnly);
	/** A wall clock's hands (hour and minute) on a dial at Center facing stage yaw FaceYaw, Radius cm: they keep the world's time. */
	void ClockHands(const FVector& Center, float FaceYaw, float Radius);
	/**
	 * A mirror (stage cm): the panels' plane through PlanePoint facing Normal (into the room). With no art, Size (width,
	 * height) makes a block-out panel there in M_NS_Mirror. The capture fallback renders it while live (SetMirrorLive).
	 */
	void Mirror(const FVector& PlanePoint, const FVector& Normal, const FVector2D& Size, bool bBlockOut);
	/** The materials its art's unbaked slots were given at build (by slot name): the barber pole's stripes, the treadmills' belts, the mirrors, tinted cloth. */
	const TArray<TObjectPtr<UMaterialInstanceDynamic>>& PoleMaterials() const { return PoleMids; }
	const TArray<TObjectPtr<UMaterialInstanceDynamic>>& BeltMaterials() const { return BeltMids; }
	/** M_NS_ClothTint for an instanced garment (the thrift's racks; its tint from each instance's custom data 0..2), or null without shops_setup's material. */
	UMaterialInterface* ClothMaterial(UMaterialInterface* Baked);
	/** The hour as the shop shows it (hours, minutes past midnight). */
	double MinuteOfDay() const;

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

	/** How long the buzzer takes (the keeper looks up and buzzes). */
	float BuzzSeconds = 0.6f;
	/** The hour as last set (world minutes) and the daylight (0 night .. 1 day). */
	double WorldMinutes = 0.0;
	float Daylight = 0.0f;
	/** Seconds since play began (what the spinners and the bell turn by). */
	float Clock = 0.0f;
	/** The door swung open this frame (the bell's swing starts). */
	bool bDoorJustOpened = false;
	bool bMember = false;

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
	struct FSpin
	{
		TWeakObjectPtr<USceneComponent> Part;
		FRotator Base = FRotator::ZeroRotator;
		FRotator PerSecond = FRotator::ZeroRotator;
		bool bOpenOnly = false;
		float Turned = 0.0f;
	};
	struct FClock
	{
		TWeakObjectPtr<USceneComponent> Hour;
		TWeakObjectPtr<USceneComponent> Minute;
		float FaceYaw = 0.0f;
	};
	struct FSeat
	{
		FTransform Feet;
		float Height = 46.0f;
		FVector Stand = FVector::ZeroVector;
		/** The camera while sitting there (the diner's counter or booth), or none. */
		FName Shot;
	};
	struct FCarved
	{
		TWeakObjectPtr<UStaticMeshComponent> Original;
		TArray<TWeakObjectPtr<UStaticMeshComponent>> Pieces;
		/** How the original was, to put it back if the shop goes first. */
		bool bWasVisible = true;
		TEnumAsByte<ECollisionEnabled::Type> WasCollision = ECollisionEnabled::NoCollision;
	};
	struct FMirrorPlane
	{
		FVector Point = FVector::ZeroVector;
		FVector Normal = FVector::ForwardVector;
	};

	void ResetParts();
	void Unregister();
	/** A zone of the shop's outside the room (its porch, its entry): Id "<venue>-<Suffix>", taken away with the shop. */
	void OutsideZone(const TCHAR* Suffix, const FBox& Box, const FString& Place);
	/** The grille as GrilleOpen has it (the art's curtain scaled into its hood, the block-out raised and hidden). */
	void ApplyGrille();
	void ApplyLights();
	void ApplyExposure();
	void ApplyMirrorMode();
	void UpdateWake(float Dt);
	void UpdateMirrorCapture(float Dt);
	/** Carves what's in the holes now (again, after the street built again: an original gone). */
	void ApplyCarves();
	UStreetWorld* Registry() const;
	ULightComponent* AddLight(ULightComponent* L, float Candela, uint32 SrgbHex, EShopLight LightRole);
	/** The unbaked slots of an imported mesh given their runtime materials (spec 0.5: ns_mirror, ns_barberpole, ns_belt, ns_glass). */
	void DressSlots(UStaticMeshComponent* C);
	/** The shops' runtime material for a slot (by its name), or null to keep the imported one. */
	UMaterialInterface* RuntimeMaterialFor(const FString& Slot);
	/** Whether Pos (world) is at the doorway (within Reach across its line, either side). */
	bool AtDoorway(const FVector& Pos, double Reach) const;
	/** The doorway's middle and its span along the shut line (stage cm). */
	FVector DoorwayMiddle() const;
	double DoorwaySpan() const;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UMaterialInstanceDynamic>> Materials;
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> DoorLeaf;
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> DoorLeaf2;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> DoorStop;
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> GrilleRoot;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> GrilleArt;
	UPROPERTY(Transient)
	TObjectPtr<UBoxComponent> RoomVolume;
	UPROPERTY(Transient)
	TObjectPtr<UPostProcessComponent> RoomLens;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidgetComponent>> Screens;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> PoleMids;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> BeltMids;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> MirrorMids;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> MirrorMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PoleMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> BeltMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> ClothTintMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> ShopGlassMaterial;
	UPROPERTY(Transient)
	TObjectPtr<USceneCaptureComponent2D> MirrorCapture;
	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> MirrorTarget;
	TArray<FLight> Lights;
	TArray<FGlow> Glows;
	TArray<FSpin> Spins;
	TArray<FClock> Clocks;
	TArray<TSharedPtr<SDrawListWidget>> ScreenSlates;
	TMap<FName, FSeat> Seats;
	TMap<FName, FTransform> ShotMarks;
	TArray<FBox> CarveHoles;
	TArray<FCarved> Carved;
	TArray<FMirrorPlane> MirrorPlanes;
	/** The porches' zones (taken away with the shop). */
	TArray<FName> PorchZones;

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
	/** Told its hour since it was built (the first SetHour snaps the grille and the lights to it, no rolling as the street fades in). */
	bool bHourSet = false;
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
	/** The registry's zone revision the carves were last made against (the street builds again: carve again). */
	int32 CarvedRevision = -1;
	float CarveCheckIn = 0.0f;
	bool bMirrorLive = false;
	int32 AppliedMirrorMode = -1;
	float MirrorCaptureIn = 0.0f;
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
