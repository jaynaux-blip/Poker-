#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "StreetWorld.generated.h"

class USceneComponent;

/**
 * The street's registry (design spec 7.5.2): every stage that builds part of the street at play (AStreetStage, the
 * shops' AShopStage, Market's stage) registers what it built here, so nothing that walks, talks or prompts on the street
 * needs to know which stage built what: the uses E can reach (and what each does, by verb), the zones (place names, room
 * tones, dry boxes for the rain, the camera inside), the starts, the cinematic shots, the stations people work, sit and
 * wait at, the doors people go in and out of, and the walkways they walk.
 *
 * Everything here is in world space (a stage turns its own cm into the world's as it registers). Nothing is registered
 * in the editor's world: only a game or Play-In-Editor world has a registry.
 *
 * A stage that can go away before its world does removes what it added (AShopStage::EndPlay): the uses' and zones'
 * functions may hold it.
 */

/** A straight line along a sidewalk, clear to walk from Start to End (world space, on the sidewalk's top). */
struct FStreetLane
{
	FVector Start = FVector::ZeroVector;
	FVector End = FVector::ZeroVector;
};

/** A sidewalk as people walk it (AStreetStage::Sidewalk, UStreetWorld::Walkways): its clear lanes, what stands on it, its top. */
struct FStreetWalkway
{
	/** The street it runs along: "Fifth", "Market", "Hollis", "Wexler", "Cardinal". */
	FName Street = FName(TEXT("Fifth"));
	/** Fifth: 0 the home side (1812, the Lucky Penny, Market's corner), 1 the far side (the tenement, the Wash & Fold). Others: 0 south or east, 1 north or west. */
	int32 Side = 0;
	/** Lines along it, each clear of the stoops, steps and street furniture end to end (the home side's cross Market). */
	TArray<FStreetLane> Lanes;
	/** What stands on it, as footprints in world XY (stoops, steps, the hydrant, cans, poles), for anyone off a lane. */
	TArray<FBox2D> Obstacles;
	/** The sidewalk's top (world Z). */
	double Z = 0.0;
};

/** A door people go in and out of (AStreetStage::Doors, UStreetWorld::Doors; world space). */
struct FStreetDoor
{
	/** "store" (the Lucky Penny), "washfold" (the Wash & Fold), "home" (door 1812), "dee" (the tenement's street door); the shops' by venue ("barber", "pawn"). */
	FName Id;
	/** On the sidewalk's top in front of it, facing it. */
	FTransform Outside;
	/** The middle of the doorway at its floor. */
	FVector Threshold = FVector::ZeroVector;
	/** A room a pedestrian walks on into (the store, the laundromat, the shops); the others are homes: people go in and are gone. */
	bool bRoom = false;
};

/** What E does at a use (AStreetGameMode::OnInteract dispatches on it, spec 7.5.3). */
UENUM()
enum class EStreetVerb : uint8
{
	/** A door the hero lives behind: 1812's, or Dee's while evicted (Id "home", "dee"). */
	Home,
	/** The Lucky Penny's counter and shelves (Shelf: the counter's page). */
	Counter,
	/** A door that opens on E: the Wash & Fold's, Dee's steel door, a shop's (its closed line when shut). */
	Door,
	/** A shop's screen, opened on its page (Venue, Param). */
	Shop,
	/** Someone to talk to (Param: the person's id; "attendant" is the Wash & Fold's). */
	Talk,
	/** A seat (a stool, a booth, the barber's bench), then the venue's screen. */
	Sit,
	/** Pass time here (the shelter, the bench). */
	Wait,
	/** Board the 14. */
	Board,
	/** Get dressed (the closet, the fitting booth). */
	Wardrobe,
};

/** Something E can do somewhere on the street. */
USTRUCT()
struct FStreetUse
{
	GENERATED_BODY()

	/** "home", "counter", "washdoor", "barber-chair1" (spec 2.10's ids). */
	UPROPERTY()
	FName Id;
	UPROPERTY()
	EStreetVerb Verb = EStreetVerb::Shop;
	/** The venue it belongs to: "store", "washfold", "barber", "thrift", "gym", "diner", "pawn" (none for a home door). */
	UPROPERTY()
	FName Venue;
	/** What it opens: the offer page, the rack tab, the person's id. */
	UPROPERTY()
	FString Param;
	/** What the prompt says when it can be used ("Go home", "Get a cut"). */
	UPROPERTY()
	FString Prompt;
	/** How near (cm, across the ground) an eye must be to reach it; facing it counts more than being on it. */
	UPROPERTY()
	float Radius = 150.0f;
	/** The Lucky Penny's counter page it opens on (-1: not a shelf). */
	UPROPERTY()
	int32 Shelf = -1;
	/** What it moves with (a door's leaf, a person, the stage's root); with none, Offset is its place in the world. */
	UPROPERTY()
	TWeakObjectPtr<USceneComponent> Anchor;
	/** Its place in Anchor's space (or the world's, with no Anchor). */
	UPROPERTY()
	FVector Offset = FVector::ZeroVector;
	/** "" when it can be used now; otherwise what the prompt says instead ("Fifth St. Barbers - Closed - Opens Tue 10 AM"). Unset: always usable. */
	TFunction<FString()> WhyNot;
};

/** A part of the street with a name and its own rules: the place on the HUD, the room's tone, no rain in it, the camera inside. */
USTRUCT()
struct FStreetZone
{
	GENERATED_BODY()

	/** "store", "washfold", "fifth", "corner", "market", "barber". Adding a zone with an Id that's there replaces it. */
	UPROPERTY()
	FName Id;
	/** World space. */
	UPROPERTY()
	FBox Box = FBox(ForceInit);
	/** The HUD's place name: "FIFTH ST. BARBERS", "MARKET STREET", "FIFTH & MARKET", "HOLLIS ST". */
	UPROPERTY()
	FString Place;
	/** Which room tone the street's sound plays in it (StreetRoomTone). */
	UPROPERTY()
	int32 RoomTone = 0;
	/** No rain falls in it (DryBoxes). */
	UPROPERTY()
	bool bDry = false;
	/** A room (the pause menu names it; steps are softer in it). */
	UPROPERTY()
	bool bInterior = false;
	/** Where zones overlap, the highest wins (a shop over its street). */
	UPROPERTY()
	int32 Priority = 0;
	/** How far open its door is (0..1), for the room tone's crossfade. Unset: always shut. */
	TFunction<float()> DoorOpen;
	/** The camera inside (spec 3.8): the third person boom's length (cm) and the field of view (horizontal degrees at the player's default setting); -1 keeps the street's. */
	UPROPERTY()
	float BoomLength = -1.0f;
	UPROPERTY()
	float Fov = -1.0f;
	/** First person while inside (put back as it was on the way out). */
	UPROPERTY()
	bool bFirstPerson = false;

	/** Whether it asks anything of the camera. */
	bool HasCameraProfile() const { return BoomLength >= 0.0f || Fov > 0.0f || bFirstPerson; }
};

/** The room tones a zone can ask the street's sound for (FStreetZone::RoomTone; StreetAudio plays them). */
namespace StreetRoomTone
{
constexpr int32 Street = 0;
constexpr int32 Store = 1;
constexpr int32 Laundry = 2;
constexpr int32 Barber = 3;
constexpr int32 Thrift = 4;
constexpr int32 Gym = 5;
constexpr int32 Diner = 6;
constexpr int32 Pawn = 7;
} // namespace StreetRoomTone

UCLASS()
class SHORTSTACK_API UStreetWorld : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** The registry of Context's world (null in the editor's world, or with no world). */
	static UStreetWorld* Get(const UObject* Context);

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	// ------------------------------------------------------------ uses
	/** Registers a use; the handle removes it. */
	int32 AddUse(const FStreetUse& U);
	void RemoveUse(int32 Handle);
	/**
	 * What an eye at Eye looking along Dir reaches (null: nothing): a person first (a Talk use anchored on a pawn), then
	 * every other use, each by AStreetStage::SpotFor's scoring (facing it, then nearness). Valid until the next AddUse or
	 * RemoveUse.
	 */
	const FStreetUse* Best(const FVector& Eye, const FVector& Dir) const;
	/** The use with Id (null: none). */
	const FStreetUse* FindUse(FName Id) const;
	/** Where a use is now (world). */
	static FVector UseLocation(const FStreetUse& U);
	/** What the prompt says at U now: its WhyNot when it can't be used, else its Prompt. */
	static FString PromptOf(const FStreetUse& U);
	int32 NumUses() const { return Uses.Num(); }

	// ------------------------------------------------------------ zones
	void AddZone(const FStreetZone& Z);
	void RemoveZone(FName Id);
	/** The highest-priority zone containing At (the smaller one where two tie), or null. */
	const FStreetZone* ZoneAt(const FVector& At) const;
	const FStreetZone* FindZone(FName Id) const;
	/** Outside every zone, or below the set's floor (SetFloorZ). False while no zone is registered (nothing is known yet). */
	bool IsOffTheSet(const FVector& At) const;
	/** The lowest anyone may stand (world Z; the stage's Z -200). */
	void SetFloorZ(double WorldZ) { FloorZ = WorldZ; }
	/** The dry zones' boxes, highest priority first, at most 8 (M_StreetRain's DryMin0..7 / DryMax0..7). */
	TArray<FBox> DryBoxes() const;
	/** Goes up whenever a zone is added or removed (whoever feeds the rain its dry boxes sets them again). */
	int32 ZoneRevision() const { return Revision; }
	const TArray<FStreetZone>& AllZones() const { return Zones; }

	// ------------------------------------------------------------ starts, shots, stations
	/** Where the hero appears ("home", "store", "barber"...; spec 2.10). Adding one that's there moves it. */
	void AddStart(FName Id, const FTransform& T);
	void RemoveStart(FName Id);
	bool Start(FName Id, FTransform& Out) const;
	TArray<FName> StartIds() const { return StartOrder; }
	/** A camera (its transform and horizontal field of view) for a service, a screen or an acceptance shot ("barber-mirror"). */
	void AddShot(FName Id, const FTransform& T, float Fov);
	void RemoveShot(FName Id);
	bool Shot(FName Id, FTransform& Out, float& Fov) const;
	TArray<FName> ShotIds() const { return ShotOrder; }
	/** Where someone works, sits or waits: their feet's transform, facing the way they face; the pose; the seat's height (0 standing); under cover. */
	void AddStation(FName Id, FName Venue, const FTransform& T, FName Pose, float SeatHeight, bool bCovered);
	/** The venue's stations, in the order they were added (every station for None). */
	TArray<FName> Stations(FName Venue) const;
	bool Station(FName Id, FTransform& Out, FName& Pose, float& SeatHeight) const;
	/** Whether the station is out of the rain (a shelter, an awning, indoors). */
	bool IsCovered(FName Id) const;
	/** Takes away what a venue registered under its name: its uses and stations (by their Venue), its zone and its door (by Id). */
	void RemoveVenue(FName Venue);

	// ------------------------------------------------------------ the people's map
	/** A door people go in and out of; one with an Id that's there replaces it. */
	void AddDoor(const FStreetDoor& D);
	TArray<FStreetDoor> Doors() const { return DoorList; }
	/** A sidewalk people walk; one for the same street and side replaces it. */
	void AddWalkway(const FStreetWalkway& W);
	TArray<FStreetWalkway> Walkways() const { return WalkwayList; }
	/** Someone besides the player walking about (each frame, from the crowd): doors open for them too (WalkersNear). */
	void NoteWalker(const FVector& At);
	/** Whether anyone noted this frame or the last stands within Reach (cm, across the ground) of At, Z within ZReach. */
	bool WalkersNear(const FVector& At, double Reach, double ZReach = 250.0) const;

	/** Every use (handle and use), in the order they were added. */
	TArray<TPair<int32, const FStreetUse*>> AllUses() const;
	/** One line: what's registered (ss.Street.Describe). */
	FString Describe() const;
	/** Draws the zones, the uses, the stations, the starts and the doors for Seconds (ss.Zone.Show; not in a shipping build). */
	void DrawDebug(float Seconds) const;

private:
	struct FShotEntry
	{
		FTransform T;
		float Fov = 60.0f;
	};
	struct FStationEntry
	{
		FName Venue;
		FTransform T;
		FName Pose;
		float SeatHeight = 0.0f;
		bool bCovered = false;
		int32 Order = 0;
	};
	struct FWalker
	{
		FVector At = FVector::ZeroVector;
		uint64 Frame = 0;
	};

	struct FUseEntry
	{
		int32 Handle = 0;
		FStreetUse Use;
	};

	/** In the order they were added (Best's ties go to the earlier, as SpotFor's did); a handle is never reused. */
	TArray<FUseEntry> Uses;
	int32 NextHandle = 1;
	TArray<FStreetZone> Zones;
	TMap<FName, FTransform> Starts;
	TArray<FName> StartOrder;
	TMap<FName, FShotEntry> Shots;
	TArray<FName> ShotOrder;
	TMap<FName, FStationEntry> StationMap;
	int32 NextStationOrder = 0;
	TArray<FStreetDoor> DoorList;
	TArray<FStreetWalkway> WalkwayList;
	mutable TArray<FWalker> WalkerList;
	double FloorZ = -200.0;
	int32 Revision = 0;
};

/**
 * Every asset path a runtime-built street stage loads (spec 0.8), so the editor can resolve them all onto the street's saved
 * preload list (AStreetStage::Preload) and the level load streams them in before the stages build. Each stage class
 * (AStreetStage, AMarketStage, each AShopStage) has a static AppendPreload and registers it once, at file scope in its
 * .cpp:
 *
 *     SHORTSTACK_STREET_PRELOAD(ABarberStage)
 *
 * Paths are object paths ("/Game/ShortStack/Meshes/SM_X/SM_X.SM_X"); one for an asset that isn't imported yet is fine
 * (it's skipped until it is).
 */
struct SHORTSTACK_API FStreetPreload
{
	using FAppend = void (*)(TArray<FString>& Paths);
	struct SHORTSTACK_API FRegister
	{
		FRegister(const TCHAR* Name, FAppend Append);
	};
	/** Every registered class's paths, sorted, each once. */
	static TArray<FString> Collect();
	/** An imported Blender mesh's object path (unreal/Art/Meshes, as ImportedMesh loads it). */
	static FString MeshPath(const TCHAR* Name);
};

#define SHORTSTACK_STREET_PRELOAD(Class) static FStreetPreload::FRegister Class##_Preload(TEXT(#Class), &Class::AppendPreload);

/**
 * The street's test overrides (console variables a test or a script sets, ss.Street.Rain and the like): they'd outlive
 * the Play-In-Editor session that set them, so the game mode puts each back at its reset value when the street begins
 * (FStreetOverrides::ResetAll), unless ss.Street.KeepOverrides is 1. Register each once, at file scope:
 *
 *     static FStreetOverrides::FRegister RainOverride(TEXT("ss.Street.Rain"), TEXT("-1"));
 */
struct SHORTSTACK_API FStreetOverrides
{
	struct SHORTSTACK_API FRegister
	{
		FRegister(const TCHAR* Name, const TCHAR* ResetTo);
	};
	/** Puts every registered override back (unless ss.Street.KeepOverrides 1); how many weren't at their reset value. */
	static int32 ResetAll();
};
