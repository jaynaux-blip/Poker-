#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "BackRoomStage.generated.h"

class UExponentialHeightFogComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPointLightComponent;
class UPostProcessComponent;
class URectLightComponent;
class USpotLightComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTextRenderComponent;

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
	/** Where seat Index (0 = player, 4 = dealer, counterclockwise) meets the rail's outer edge. */
	static FVector SeatEdge(int32 Index);
	/** Where a player in seat Index sits: the chair's front edge, facing the table. */
	static FTransform SeatTransform(int32 Index);
	/** The player's eyes, seated. */
	FVector EyeLocation() const;

	// ------------------------------------------------------------ the Riverside (card room venue)
	/**
	 * The Riverside Casino's poker room instead of the Back Room: the player's table stays at the origin
	 * (the feature table, roped off, under the stream's lights) and the rest of the room is built around
	 * it on RoomRoot, which turns when the player is moved to another table.
	 */
	UPROPERTY(EditAnywhere, Category = "Short Stack|Card Room")
	bool bCardRoom = false;
	/** How many other tables the room holds, and where each stands in the room (its frame: player side -X). */
	static int32 NumTableSlots();
	/** Slot's frame in room space (RoomRoot), and in the world as the room is turned now. */
	static FTransform TableSlotLocal(int32 Slot);
	FTransform TableSlot(int32 Slot) const;
	/** A table running, or closed for the night (lights down, chairs pushed in, a cover over the felt). */
	void SetTableOpen(int32 Slot, bool bOpen);
	/** Turns the room around the feature table (quarter turns): the player was moved, the view changes. */
	void SetRoomTurn(int32 QuarterTurns);
	int32 GetRoomTurn() const { return RoomTurn; }
	USceneComponent* GetRoomRoot() const { return RoomRoot; }
	/** The tournament clock screens: a title, the level, the time left, blinds, what's next, and the field. */
	void SetBoard(const FString& Title, const FString& Level, const FString& Clock, const FString& Blinds, const FString& Next, const FString& Field);
	/** The stream's tally light over the feature table: on air (the final table). */
	void SetOnAir(bool bOn);
	/** The walk in from the casino floor to the feature table's seat (world points, eye height). */
	TArray<FVector> CardRoomWalkIn(const FVector& Eye) const;
	TArray<FVector> CardRoomWalkOut(const FVector& Eye) const;

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
	int32 RoomTurn = 0;
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
