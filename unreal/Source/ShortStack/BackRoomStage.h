#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "BackRoomStage.generated.h"

class UExponentialHeightFogComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPointLightComponent;
class UPostProcessComponent;
class URectLightComponent;
class USpotLightComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTextRenderComponent;

/** What the player can walk up to in the card room. */
enum class ECardRoomSpot : uint8
{
	None,
	Seat,  // the player's own chair
	Desk,  // the tournament desk
	Cage,  // the cashier
	Bar,
	Terrace, // the glass doors onto the terrace
	Exit,  // the entrance, out to the casino floor
	Rail,  // in front of the stage, watching the feature table
};

/**
 * The Back Room: the Spin Cycle Club's game behind the 24-hour laundromat, Tuesday, 1 a.m.
 *
 * A cinderblock room with a drop ceiling and a concrete floor. The oval table sits under one
 * green-enamel poker lamp, the only warm light; a fluorescent strip buzzes and flickers over the
 * dryers, the door to the laundromat stands ajar and spills cold light, one dryer is running. Smoke
 * hangs in the lamp's cone. Built in C++ like ANightOneStage so it runs in any level; each piece uses
 * the Blender prop when it has been imported and an engine-shape stand-in otherwise.
 *
 * Coordinates: Unreal centimeters, origin on the floor under the table's middle. The player sits at
 * -X looking down +X at the dealer; +Y is the player's right.
 */
UCLASS()
class SHORTSTACK_API ABackRoomStage : public AActor
{
	GENERATED_BODY()

public:
	ABackRoomStage();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Builds the set again so it picks up materials and props imported since the level loaded. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Short Stack")
	void RebuildSet();

	// ------------------------------------------------------------ layout
	/** Felt height (cm): cards and chips lie here. */
	static constexpr double FeltZ = 76.0;
	/**
	 * A seated player's chair front edge is this far out from the rail's outer edge (cm). Close, as people sit at a
	 * card table: the belly an arm's length less a forearm from the rail, the elbows resting on it, the hands out
	 * over the felt (more, and the arms reach out straight and become the picture).
	 */
	static constexpr double RailGap = 10.0;
	/** Height of the table's underside (the apron's lower edge, cm): under it is open (knees, a lap, a belly). */
	static constexpr double ApronZ = 71.6;
	/** How far the table's outer edge is from its middle line (cm). */
	static constexpr double RailOuterD = 61.0;
	/** Where seat Index (0 = player, 4 = dealer, counterclockwise) meets the rail's outer edge. */
	static FVector SeatEdge(int32 Index);
	/** Where a player in seat Index sits: the chair's front edge, facing the table. */
	static FTransform SeatTransform(int32 Index);
	/** The player's eyes, seated. */
	FVector EyeLocation() const;
	/**
	 * The poker table's solid, in the table's own frame (its middle line along y: x = 0, y in [-61, 61]; the same
	 * section the table mesh is built from: the felt, the padded rail with its crown, the outer skirt). Whether a
	 * ball of Radius at P is sunk into it, the push that takes it out, and its clearance (negative when sunk).
	 * bUnderTableFree: the space under the apron is open (knees, a belly), else the solid runs on down.
	 */
	static bool TableContact(const FVector& P, float Radius, bool bUnderTableFree, FVector& OutPush, float& OutClearance);

	// ------------------------------------------------------------ the Embercrest (card room venue)
	/**
	 * The Embercrest Casino's card room instead of the Back Room (BackRoomStageCardRoom.cpp): twenty numbered tables
	 * and the stream table on the final table's stage, built from the Embercrest kit (art/blender/assets/embercrest.py;
	 * the old showroom kit, art/blender/assets/cardroom.py, where it isn't imported).
	 * The player's table is always the one at the origin; the room (RoomRoot) is placed so that it's the table the
	 * tournament seated them at, so every table number is a real place in the room.
	 */
	UPROPERTY(EditAnywhere, Category = "Short Stack|Card Room")
	bool bCardRoom = false;
	/** How many tables the room holds (the stream table last), and where each stands in the room (its frame: player side -X). */
	static int32 NumTableSlots();
	/** The stream table's slot, on the stage (the final table plays there). */
	static int32 StreamSlot();
	/** Slot's frame in room space (RoomRoot), and in the world as the room is turned now. */
	static FTransform TableSlotLocal(int32 Slot);
	FTransform TableSlot(int32 Slot) const;
	/** A table running, or closed for the night (lights down, chairs pushed in, a cover over the felt). */
	void SetTableOpen(int32 Slot, bool bOpen);
	/** Places the room so that Slot's table is the player's, at the origin (its own furniture and light hide). */
	void SetRoomAnchor(int32 Slot);
	int32 GetRoomAnchor() const { return AnchorSlot; }
	/** The crowd at the far tables: cheap figures (kinds 0-5 seated players, 6 a dealer, 7-8 standing), room space. */
	/** The crowd's figures: seated (0-5, and 9-14 where the baked crowd has them), the dealer (6), standing (7, 8). */
	static constexpr int32 CrowdKinds = 15;
	void ClearCrowd();
	void AddCrowd(int32 Kind, const FTransform& RoomLocal);
	/** The boards: today's events by the entrance, the champions between the desk and the cage, the cash list. */
	void SetSchedule(const TArray<FString>& Lines);
	void SetChampions(const TArray<FString>& Lines);
	void SetCashList(const TArray<FString>& Lines);
	/** A move to another table: out of the player's seat toward ToSlot (before the room re-anchors), in from FromSlot's side (after). */
	TArray<FVector> CardRoomMoveOut(const FVector& Eye, int32 ToSlot) const;
	TArray<FVector> CardRoomMoveIn(const FVector& Eye, int32 FromSlot) const;
	/** From the seat to the cashier's window, and from the window out through the doors to the casino floor. */
	TArray<FVector> CardRoomToCage(const FVector& Eye) const;
	TArray<FVector> CardRoomCageToDoor(const FVector& At) const;
	USceneComponent* GetRoomRoot() const { return RoomRoot; }
	/** The tournament clock screens: a title, the level, the time left, blinds, what's next, and the field. */
	void SetBoard(const FString& Title, const FString& Level, const FString& Clock, const FString& Blinds, const FString& Next, const FString& Field);
	/** The stream's tally light over the feature table: on air (the final table). */
	void SetOnAir(bool bOn);
	/** The walk in from the casino floor to the feature table's seat (world points, eye height). */
	TArray<FVector> CardRoomWalkIn(const FVector& Eye) const;
	TArray<FVector> CardRoomWalkOut(const FVector& Eye) const;
	/**
	 * Walking the card room on foot (world points at eye height): From toward To as far as the room allows, stopped by
	 * the tables and their chairs, the columns, the counters and the desk's rope, the bar's stools, the stage and the
	 * walls (the entrance is open), sliding along whatever it meets.
	 */
	FVector CardRoomStep(const FVector& From, const FVector& To) const;
	/** The floor's height under a world point (the stage is raised). */
	double CardRoomFloorZ(const FVector& At) const;
	/** What a player standing at Eye (world) can reach. */
	ECardRoomSpot CardRoomSpot(const FVector& Eye) const;
	/** Room space to world and back (the room moves so the player's table sits at the origin). */
	FVector RoomToWorld(const FVector& RoomAt) const;
	FVector WorldToRoom(const FVector& WorldAt) const;

	// ------------------------------------------------------------ tuning
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	float LampCandela = 330.0f;

	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	float FluorescentCandela = 45.0f;

	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	float DoorCandela = 140.0f;

	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	float FogDensity = 0.06f;

	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	float ExposureCompensation = 0.0f;

private:
	void BuildSet();
	void BuildShell();
	void BuildTable();
	void BuildMachines();
	void BuildLights();
	void BuildAir();
	void BuildCardRoom();
	void BuildCardRoomTables();
	void BuildCardRoomLights();
	void BuildCardRoomAir();
	UTextRenderComponent* AddText(const FString& Text, const FVector& At, const FRotator& Facing, float Size, const FLinearColor& Color, USceneComponent* Parent);

	template <typename T>
	T* NewPart(USceneComponent* Parent = nullptr);
	UStaticMeshComponent* AddMesh(UStaticMesh* Mesh, UMaterialInterface* Material, const FVector& Location, const FVector& Scale, const FRotator& Rotation = FRotator::ZeroRotator, USceneComponent* Parent = nullptr, bool bShadows = true);
	/** Box from min/max corners (cm). */
	UStaticMeshComponent* Box(UMaterialInterface* Material, const FVector& Min, const FVector& Max, bool bShadows = true);
	/** A room surface: M_Room with a pattern (1 cinderblock, 2 concrete, 3 ceiling tile, 4 painted steel). */
	UMaterialInstanceDynamic* Room(FName Key, int32 Pattern, uint32 SrgbHex, uint32 SrgbHex2 = 0, float Split = 0.0f, float Metallic = 0.0f);
	/** An unlit glow for stand-in light sources (tubes, signs). */
	UMaterialInstanceDynamic* Glow(FName Key, const FLinearColor& Color, float Strength);
	/** An LED screen showing one of the Embercrest's images (Textures/Brand/<Image>, art/blender/brand.py); a dark
	 *  glow if it isn't imported. */
	UMaterialInstanceDynamic* ScreenImage(FName Key, const TCHAR* Image, float Strength, const FLinearColor& Tint = FLinearColor::White);

	UPROPERTY()
	TMap<FName, TObjectPtr<UMaterialInstanceDynamic>> Materials;

	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> CylinderMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> SphereMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> ConeMesh;
	UPROPERTY()
	TObjectPtr<UMaterialInterface> FallbackMaterial;

	// Generated by Content/Python/backroom_setup.py and shortstack_setup.py; optional.
	UPROPERTY()
	TObjectPtr<UMaterialInterface> RoomMaterial;
	/** M_Room plus the card room's surfaces (patterns 5 and up). */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> CardRoomMaterial;
	UPROPERTY()
	TObjectPtr<UMaterialInterface> SurfaceMaterial;
	UPROPERTY()
	TObjectPtr<UMaterialInterface> EmissiveMaterial;
	/** M_ScreenImage: an image on an LED screen (backroom_setup.py). */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> ScreenImageMaterial;

	// Blender props (art/blender), imported from unreal/Art/Meshes; each is optional.
	UPROPERTY()
	TObjectPtr<UStaticMesh> TableMesh;

	UPROPERTY()
	TObjectPtr<USpotLightComponent> LampLight;
	UPROPERTY()
	TObjectPtr<UPointLightComponent> ShadeGlow;
	UPROPERTY()
	TObjectPtr<URectLightComponent> Fluorescent;
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> TubeGlow;
	UPROPERTY()
	TObjectPtr<URectLightComponent> DoorSpill;
	UPROPERTY()
	TObjectPtr<UPointLightComponent> DryerGlow;
	UPROPERTY()
	TObjectPtr<UExponentialHeightFogComponent> Fog;
	UPROPERTY()
	TObjectPtr<UPostProcessComponent> Lens;

	float Time = 0.0f;
	float NextFlicker = 3.0f;
	float FlickerLeft = 0.0f;

	// The card room.
	UPROPERTY()
	TObjectPtr<USceneComponent> RoomRoot;
	UPROPERTY()
	TObjectPtr<UMaterialInterface> TextMaterial;
	UPROPERTY()
	TArray<TObjectPtr<UTextRenderComponent>> BoardLines;
	UPROPERTY()
	TArray<TObjectPtr<URectLightComponent>> TableLights;
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> TableCovers;
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> TableGlows;
	UPROPERTY()
	TObjectPtr<UPointLightComponent> Tally;
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> TallyGlow;
	/** Each table slot's furniture, light and sign (hidden for the player's own, which is the real table at the origin). */
	UPROPERTY()
	TArray<TObjectPtr<USceneComponent>> SlotRoots;
	UPROPERTY()
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> CrowdMeshes;
	UPROPERTY()
	TArray<TObjectPtr<UTextRenderComponent>> ScheduleLines;
	UPROPERTY()
	TArray<TObjectPtr<UTextRenderComponent>> ChampionLines;
	UPROPERTY()
	TArray<TObjectPtr<UTextRenderComponent>> CashLines;
	/** The player's table's own pendant and its light (off at the stream table, which has the truss). */
	UPROPERTY()
	TObjectPtr<USceneComponent> HeroPendant;
	UPROPERTY()
	TObjectPtr<URectLightComponent> HeroLight;
	UStaticMesh* Kit(const TCHAR* Name) const;
	UInstancedStaticMeshComponent* Instances(UStaticMesh* Mesh, UMaterialInterface* Override, USceneComponent* Parent, bool bShadows);
	int32 AnchorSlot = 0;
	bool bOnAir = false;
};

template <typename T>
T* ABackRoomStage::NewPart(USceneComponent* Parent)
{
	T* Part = NewObject<T>(this);
	Part->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	Part->SetupAttachment(Parent ? Parent : Root.Get());
	BlueprintCreatedComponents.Add(Part);
	Part->RegisterComponent();
	return Part;
}
