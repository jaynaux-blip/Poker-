#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "StreetCrowd.generated.h"

class AShortStackCharacter;
class AStreetStage;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;
struct FStreetCrowdMap;

/** What a pedestrian is out for (chosen as they come onto the street; it shapes their plan). */
enum class ECrowdRole : uint8
{
	/** Along the block and off it again, stopping at a shop window or two. */
	Stroll,
	/** Into the Lucky Penny: a shelf or two, the counter, and out. */
	Errand,
	/** Home: in at 1812's door, or the tenement's street door across from it (Dee's, and her neighbors'). */
	Home,
	/** A load at the Wash & Fold: a washer, the chairs, a dryer or a table (or a bag left at its counter), and off again. */
	Laundry,
	/** Dee's game night: through the Wash & Fold to her steel door at the back, or out of it near dawn. */
	Player,
	PlayerOut,
	/** Out of a door for a cigarette beside it, then back in. */
	Smoke,
	/** Waiting under the bus shelter, when the street has one. */
	Shelter,
};

/** One step of a pedestrian's plan (FStreetWalker::Plan). */
enum class ECrowdLeg : uint8
{
	/** On to a node of the map, not stopping there. */
	Go,
	/** To a spot (a shop window, a shelf, the counter, the bus shelter) and a while there. */
	Visit,
	/** In at a door (1812's, Dee's, her steel door at the back of the Wash & Fold): gone once nobody sees them go. */
	Enter,
	/** Off the set at one of its ends (Fifth's two, Market's two): gone once nobody sees them go. */
	Leave,
	/** Beside a door with a cigarette for a while. */
	Smoke,
};

struct FCrowdLeg
{
	ECrowdLeg Kind = ECrowdLeg::Go;
	/** The map's node (Go, Leave) or spot (Visit, Enter, Smoke). */
	int32 Target = INDEX_NONE;
	/** How long to stay (Visit, Smoke: seconds). */
	float Duration = 0.0f;
};

/** A point of a pedestrian's route (stage cm) and how the stretch from it to the next is walked. */
struct FCrowdWaypoint
{
	/** On the map's line, and where this pedestrian walks it (their lane, kept to the right or, in the rain, under the fronts). */
	FVector2D At = FVector2D::ZeroVector;
	FVector2D Walk = FVector2D::ZeroVector;
	/** The map's node it is (INDEX_NONE: on a spot's approach, or where they stood). */
	int32 Node = INDEX_NONE;
	/** How far from the line to the next point they may stray (cm), and that stretch's kind (FStreetCrowdMap::EEdge bits). */
	float Half = 100.0f;
	uint8 Flags = 0;
};

/**
 * One of the street's people: the MetaHuman they walk as (built once, kept for the night: in a door or off the set they're
 * only hidden), what they carry, and where they are in their plan. Positions are the stage's own (cm, see AStreetStage).
 */
USTRUCT()
struct FStreetWalker
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AShortStackCharacter> Body;
	/** The open umbrella over the right hand (SM_Umbrella, or a canopy made of the engine's shapes), turned upright itself. */
	UPROPERTY()
	TObjectPtr<USceneComponent> Umbrella;
	/** A cigarette between the right hand's fingers and its ember (whose glow the drags brighten). */
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Cigarette;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Ember;
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> EmberGlow;

	// Who they are.
	FName Cast;
	bool bWoman = false;
	FLinearColor Jacket = FLinearColor::Black;
	/** Their height against the cast's 175 cm (the actor's scale), their walking pace (cm/s) and how far right they keep. */
	float Scale = 1.0f;
	float Pace = 135.0f;
	float Lane = 50.0f;
	FRandomStream Rng;

	// Out on the street (or in one of its rooms), or not (free for the next one: since when, crowd seconds), and what for.
	bool bOut = false;
	float AwaySince = -1000.0f;
	ECrowdRole Role = ECrowdRole::Stroll;
	/** Sent off the set early (the hour wants fewer out). */
	bool bSent = false;

	// Where they are (stage cm): the floor under them (Z), their facing (degrees, stage) and speed along it.
	FVector2D Pos = FVector2D::ZeroVector;
	float Floor = 0.0f;
	float FloorWant = 0.0f;
	float Yaw = 0.0f;
	float Speed = 0.0f;
	/** The last steer's turn off the route (degrees), kept so the choice doesn't flicker between sides. */
	float SteerAngle = 0.0f;
	/** Where they were drawn last (world), for the velocity the gait reads. */
	FVector Shown = FVector::ZeroVector;

	// The route being walked: its points, the stretch they're on, the next stop they must make on it (a curb, a room's door:
	// INDEX_NONE when none), and whether it ends with them standing at a spot.
	TArray<FCrowdWaypoint> Route;
	int32 RouteAt = 0;
	int32 Gate = INDEX_NONE;
	float GateWait = 0.0f;
	/** How long they stand at the next curb, looking each way. */
	float GatePause = 0.8f;
	bool bStops = false;
	bool bArrived = false;
	/** The node they last passed and the one they're walking toward (routes start from whichever is cheaper). */
	int32 Node = INDEX_NONE;
	int32 NextNode = INDEX_NONE;
	/** The spot they're at (a window, a door): the next route leaves by its approach. */
	int32 Spot = INDEX_NONE;
	bool bWantsDoors = false;

	// The plan.
	TArray<FCrowdLeg> Plan;
	int32 LegAt = 0;
	bool bLegStarted = false;
	float Stay = 0.0f;
	float Waited = 0.0f;
	/** Waiting a step off a spot someone else is at (this frame). */
	bool bHold = false;
	/** Held up (by the hero, by someone at their spot) for this long. */
	float Blocked = 0.0f;
	/** An end of the set they couldn't leave by unseen, and until when they leave it be (crowd seconds). */
	int32 Refused = INDEX_NONE;
	float RefusedUntil = 0.0f;

	// Where the head turns (a point, stage cm; or someone: a walker's index, -2 the hero; neither: ahead) and until when, eased.
	FVector LookAt = FVector::ZeroVector;
	int32 LookWho = INDEX_NONE;
	float LookUntil = 0.0f;
	float NextLook = 0.0f;
	float LookYaw = 0.0f;
	float LookPitch = 0.0f;
	/** Looking down (keys at a door, a phone) or off to one side without a thing to look at (degrees). */
	float LookDown = 0.0f;
	float LookAside = 0.0f;
	float HeroGlanceIn = 0.0f;
	float BarkIn = 0.0f;

	// What they carry.
	/** Hunched against the rain without an umbrella (the gait's tired shoulders, 0..0.35), eased as the rain comes and goes. */
	float Hunch = 0.0f;
	bool bUmbrella = false;
	/** 0 folded (hidden) .. 1 open over them; the hand comes up with it. */
	float UmbrellaOpen = 0.0f;
	FQuat UmbrellaTurn = FQuat::Identity;
	float DryFor = 0.0f;
	bool bSmoking = false;
	float NextDrag = 0.0f;
	float DragAt = -100.0f;

	/** An obstacle the map doesn't know (a prop added since), met by the probe ahead of them: where, and for how long yet. */
	FVector2D Bump = FVector2D::ZeroVector;
	float BumpFor = 0.0f;
	float ProbeIn = 0.0f;

	/** How much of them is worth drawing and moving (0 near .. 2 far, see AStreetCrowd::UpdateCost), and their shadows. */
	int32 Tier = -1;
	bool bShadows = true;
	int32 Frame = 0;
};

/**
 * Fifth Street's people: a handful of the cast's MetaHuman extras on the procedural gait, out by the hour (a couple in the
 * dead hours, up to six in the evening), each with a plan for being out (along the block and off it, the Lucky Penny's shelves
 * and counter, home at 1812 or Dee's door across the street, a load at the Wash & Fold, Dee's game through its steel door at
 * the back, a cigarette in a doorway, the bus shelter). They walk the sidewalks on a small map of the block (Fifth's from the
 * stage's own lanes and doors), kept right, round the street furniture and each other, stepping aside for the hero (with a
 * glance, and now and then a word), crossing at the corner after a look each way; the rooms' doors open for them as for the
 * hero (AStreetStage::NoteWalker, OpenWashDoor). In the rain they hurry, and most carry an umbrella (or keep under the fronts
 * without one). Nobody appears or vanishes where the camera can see: they come and go at the ends of the set and through
 * doors while unseen, and turn back when they can't.
 *
 * The street's game mode spawns one and calls Begin, then SetHour each frame. The people are built once, as the street fades
 * in (each MetaHuman costs a frame's hitch: the fade waits for them), and kept: off the set they're hidden under it, out of
 * the scene and its shadows, and cost nothing. Far off they're drawn and
 * posed more cheaply (lower LODs, slower pose updates, no face rig, no shadows), no groom simulates, and nobody walks into the
 * Lucky Penny or the Wash & Fold while the hero is in it (but Dee's players on their way to her door).
 *
 * Its own tick runs after theirs (it moves them, then sets their look over the character's own) and before their bodies
 * pose, so the gait and the look it set are the frame's.
 */
UCLASS()
class SHORTSTACK_API AStreetCrowd : public AActor
{
	GENERATED_BODY()

public:
	AStreetCrowd();

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/** The street's people on Stage, around Hero (the player's pawn is found again if this one goes). Again with another stage starts over. */
	void Begin(AStreetStage* Stage, AShortStackCharacter* Hero);
	/** The session's hour and weather (Daylight 0..1, the world's minutes, the rain's strength as AStreetStage::RainStrength); paused, everyone freezes. */
	void SetHour(float Daylight, double WorldMinutes, float Rain, bool bPaused);

	/** A pedestrian's word to the hero (a "Sorry." after a near bump), once each: where they said it and what. */
	bool TakeBark(FVector& OutAt, FString& OutLine);
	/** How many are out on the street now, and how many the hour wants. */
	int32 NumOut() const;
	int32 NumWanted() const { return Wanted; }
	/** One line for the crowd and one per pedestrian (the console's ss.Crowd.Describe, the desktop's checks). */
	FString Describe() const;

private:
	// The people.
	void ChooseCast();
	bool BuildBody(FStreetWalker& W);
	void DressProps(FStreetWalker& W);
	void Show(FStreetWalker& W, bool bShow);
	void Freeze(bool bFreeze);
	int32 FreeWalker() const;

	// Coming and going.
	void UpdateCount(float Dt);
	bool PlaceAtStart(FStreetWalker& W);
	bool StepOutAtEntry(FStreetWalker& W);
	void Enter(FStreetWalker& W, ECrowdRole Purpose, const FVector2D& At, float Yaw, int32 Node, int32 AtSpot);
	void PutAway(FStreetWalker& W);
	void SendOff(FStreetWalker& W);
	ECrowdRole PickRole(FStreetWalker& W, bool bPlaced) const;
	void PlanFor(FStreetWalker& W, ECrowdRole Purpose, int32 From);
	int32 PickEnd(FStreetWalker& W, int32 Not) const;
	int32 PickWindow(FStreetWalker& W) const;

	// The plan and the route.
	void RunLeg(FStreetWalker& W, float Dt);
	void StartLeg(FStreetWalker& W);
	void NextLeg(FStreetWalker& W);
	void TurnBack(FStreetWalker& W, int32 Refused);
	/** Drops what's left of the plan in one of the rooms (the hero's in it, its door won't open). */
	void SkipRoom(FStreetWalker& W, uint8 Room);
	void RouteTo(FStreetWalker& W, int32 TargetNode, int32 TargetSpot, bool bStop);
	void LayLanes(FStreetWalker& W);

	// Walking.
	void Walk(FStreetWalker& W, float Dt);
	FVector2D Steer(FStreetWalker& W, const FVector2D& Want, float WantSpeed, float& OutSpeed, bool bAvoidStatics) const;
	void Probe(FStreetWalker& W);
	void Settle(FStreetWalker& W, float Dt, bool bTrace);

	// How they look and what it costs.
	void Present(FStreetWalker& W, float Dt);
	void UpdateLook(FStreetWalker& W, float Dt);
	void UpdateProps(FStreetWalker& W, float Dt);
	void UpdateCost(FStreetWalker& W, double CameraDistance);
	void Bark(FStreetWalker& W, bool bRain);

	// What the camera sees.
	void UpdateView();
	bool IsSeen(const FVector2D& At, float Floor, float Height) const;
	bool CanVanish(const FStreetWalker& W) const;

	// The stage's frame.
	FVector ToWorld(const FVector2D& At, double Z) const;
	FVector2D ToStage(const FVector& World) const;
	FVector HeroAt() const;
	FVector2D HeroVelocity() const;
	/** Whether the hero stands on the same floor as someone at Floor (stage Z): the sidewalk and the Wash & Fold's are apart. */
	bool HeroLevelWith(float Floor) const;
	bool HeroInStore() const;
	bool HeroInWash() const;
	float FloorAt(const FVector2D& At, float Fallback) const;
	AShortStackCharacter* GetHero() const;
	void DrawDebug() const;

	UPROPERTY(Transient)
	TObjectPtr<AStreetStage> Stage;
	UPROPERTY(Transient)
	TArray<FStreetWalker> Walkers;
	TWeakObjectPtr<AShortStackCharacter> Hero;
	/** The block as people walk it (nodes, edges, spots and doors, the street furniture), built from the stage at Begin. */
	TSharedPtr<FStreetCrowdMap> Map;

	// The engine's shapes for the umbrella and the cigarette before the art lands (also referenced, so they're cooked).
	UPROPERTY()
	TObjectPtr<UStaticMesh> CylinderMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> ConeMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> SphereMesh;
	UPROPERTY()
	TObjectPtr<UMaterialInterface> FallbackMaterial;
	/** M_Street when it's built (its wet look, and an emissive for the ember); the engine's basic material otherwise. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PropMaterial;

	// The hour.
	bool bHourKnown = false;
	float Daylight = 0.0f;
	double WorldMinutes = 0.0;
	float Rain = 0.0f;
	bool bPaused = false;
	bool bFrozen = false;
	int32 Wanted = 0;

	// What the camera saw this frame (refreshed before anyone moves).
	FVector ViewAt = FVector::ZeroVector;
	FVector ViewAxes[3] = {FVector::ForwardVector, FVector::RightVector, FVector::UpVector};
	double ViewTanX = 1.0;
	double ViewTanY = 0.6;
	bool bHasView = false;
	/** The street fading in from black (a hitch nobody notices), and the screen dark enough that nothing can be seen (either way). */
	bool bFading = false;
	bool bDark = false;
	/** Someone was dressed this frame as the street faded in: next frame gives the fade back the time it took (see Tick). */
	bool bFadeHeld = false;

	/** Seconds the crowd has run (unpaused), when the next may come out, and the cast's build pace while the screen is lit. */
	float Clock = 0.0f;
	float SpawnIn = 0.0f;
	float BuildIn = 0.0f;
	bool bCastChosen = false;
	int32 NumCast = 0;
	FRandomStream Dice;
	TArray<TPair<FVector, FString>> Barks;
	float BarkQuiet = 0.0f;
	bool bLoggedNoCast = false;
};
