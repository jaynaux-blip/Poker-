#pragma once

#include "CoreMinimal.h"
#include "DeeRoomLife.h"
#include "NightOneStage.h"

#include <string>
#include <vector>

#include "DeeRoomStage.generated.h"

class ABackRoomPlayer;
class SDrawListWidget;
class UInstancedStaticMeshComponent;
class UPointLightComponent;
class URectLightComponent;
class USpotLightComponent;
class UStaticMeshComponent;
class UWidgetComponent;
struct FStreamableHandle;

/**
 * Dee's front room, upstairs over the Wash & Fold across Fifth Street from 1812: the hub while the player is evicted.
 * The player sits forward on her worn couch with the laptop on the coffee table (leaning in to it as at the desk: the
 * same laptop, phone, arms and lens as the apartment's stage), her recliner and her old TV in the corner, the floor
 * lamp, the radiator under the window, her shelves of photos and a dealer's mementos, the kitchenette, a couple of
 * the player's boxes by the couch; rain on her two windows, and through them, across the street, 1812 with the player's
 * own window dark. Dee is in her chair with the TV on of an evening, asleep in her room late, or downstairs running
 * her game (SetDee: DeeRoom::WhereAt by the game mode); she's a MetaHuman seated as the Back Room seats its players.
 *
 * Built at runtime (the game mode spawns it when the career is evicted; nothing about it is saved in a level), from
 * dee_room.py's SM_DR_* props where they're imported and boxes in the stage's materials where they aren't; the street
 * outside from the street's kit (SM_Facade_1812 and its neighbours, SM_Streetlight, SM_Sedan) the same way.
 *
 * Coordinates (cm, the stage's own): the floor at Z 0, the couch facing +X to the street wall (its inside face at
 * X 170, 45 cm of brick); the main window straight ahead over the coffee table, the second to its left (-Y). Fifth
 * Street is AStreetStage's turned half round about her front wall (DeeRoomStage.cpp FromStreet): 1812's face 20.4 m
 * out, its sidewalk 4.4 m below her floor.
 */
UCLASS()
class SHORTSTACK_API ADeeRoomStage : public ANightOneStage
{
	GENERATED_BODY()

public:
	ADeeRoomStage();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	virtual void MenuShot(double T, FVector& OutLocation, FVector& OutLookAt) const override;
	/** Lying on the couch, the head on the player's pillow at its left end, the window and the hour's light across the room. */
	virtual bool SleepShot(double T, FVector& OutLocation, FVector& OutLookAt) const override;
	/** On the coffee table by the laptop, then on the floor by the couch. */
	virtual void AddCan() override;

	/**
	 * Where Dee is (DeeRoom::WhereAt): Home, she's in her chair with the TV on; anywhere else the chair's empty and the TV
	 * off. A change waits until the player isn't looking at the chair (she's never seen to pop in or out), unless
	 * bAtOnce (the room just loaded, or the player's asleep).
	 */
	void SetDee(DeeRoom::EWhere Where, bool bAtOnce);
	DeeRoom::EWhere DeeWhere() const { return Where; }
	/** She's in her chair now. */
	bool IsDeeHere() const;
	/** True once each time she has come in and sat down (or gone) since the last call. */
	bool ConsumeDeeArrived();
	bool ConsumeDeeLeft();
	/** She says a line (her jaw moves for it; the caption is the game mode's). */
	void DeeSay(const FString& Line);
	/** The TV's on (she's watching it): the room's sound has it too. */
	bool IsTvOn() const { return TvLevel > 0.5f; }
	/** The player's boxes by the couch (while they live here). */
	void SetBoxes(bool bShown);
	/** The note Dee leaves by the laptop (DeeRoom::Note; none: no note). */
	void SetNote(const std::vector<std::string>& Lines);
	/** Where the chair is, for the room's sound of her (and the door's, for the door). */
	FVector ChairLocation() const;
	FVector DoorLocation() const;

protected:
	virtual void BuildSet() override;
	virtual FVector EyeLocal() const override;
	virtual void ApplyHour() override;
	/** The room's own tunables too (ss.Dee.Lamp, ss.Dee.FrontGlow), so they can be tuned live at any hour. */
	virtual uint32 HourKey() const override;
	virtual void AttachSlate() override;

private:
	void BuildRoom();
	void BuildWindows();
	void BuildFurniture();
	void BuildDressing();
	void BuildRoomLights();
	void BuildStreet();
	void BuildFronts();
	void BuildStreetlights();

	/** A frame in the room at At (cm), its +X turned to Yaw: imported props and their stand-ins are built in it. */
	USceneComponent* Frame(const FVector& At, float Yaw);
	/** An imported prop (null when it isn't) at Local in Parent (or the stage), its front turned to Yaw in it. */
	UStaticMeshComponent* Prop(const TCHAR* Name, USceneComponent* Parent, const FVector& Local = FVector::ZeroVector, float Yaw = 0.0f);
	/** A box from Min to Max (cm) in Parent (or the stage). */
	UStaticMeshComponent* Block(UMaterialInterface* Material, const FVector& Min, const FVector& Max, USceneComponent* Parent = nullptr, bool bShadows = true);
	/** An upright cylinder: its base's middle, radius and height (cm). */
	UStaticMeshComponent* Post(UMaterialInterface* Material, const FVector& Base, double Radius, double Height, USceneComponent* Parent = nullptr, bool bShadows = true);
	/** A surface in the street's own material (M_Street: wet, puddles, its brick and slabs), or the stage's without it. */
	UMaterialInterface* StreetSurface(FName Key, uint32 SrgbHex, float Roughness, float Pattern);
	/**
	 * A front's windows (where the front isn't imported): floor over floor, every Pitch cm along Y from Y0 to Y1 on the
	 * face at X, most dark and LitShare of them lit (warm, cool, a TV's blue), the same each time for Seed.
	 */
	void WindowGrid(double X, double Y0, double Y1, double Z0, double Z1, double Pitch, double FloorH, double LitShare, int64 Seed);
	/** One window on a front facing her (-X): dark glass, or lit (Kind 1 warm, 2 cool, 3 a TV, 4 a stairwell). */
	void FrontWindow(const FVector& Center, const FVector2D& SizeCm, int32 Kind);

	void UpdateDee(float DeltaSeconds);
	void UpdateTv(float DeltaSeconds);
	void SpawnDee();
	void RemoveDee();
	/** Whether the player's camera has the chair in view (a change waits until it doesn't). */
	bool ChairInView() const;
	void DrawNote();

	// ------------------------------------------------------------ the room's parts the play drives
	UPROPERTY()
	TObjectPtr<USceneComponent> ArmchairFrame;
	UPROPERTY()
	TObjectPtr<USceneComponent> BoxesGroup;
	UPROPERTY()
	TObjectPtr<UWidgetComponent> NoteWidget;
	/** The TV's face (SM_DR_TVStand_Glow, or the stand-in's), its picture, and the light it throws. */
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> TvPicture;
	UPROPERTY()
	TObjectPtr<URectLightComponent> TvLight;
	UPROPERTY()
	FVector TvScreenAt = FVector::ZeroVector;
	/** The floor lamp: its bulb and lining's glow, its baked shade, the light. */
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> LampGlow;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> LampShade;
	UPROPERTY()
	TObjectPtr<UPointLightComponent> LampLight;
	/** The second window's daylight and night fill (the first's are the base's DayWash and CityFill). */
	UPROPERTY()
	TObjectPtr<URectLightComponent> LeftWash;
	UPROPERTY()
	TObjectPtr<URectLightComponent> LeftFill;
	// ------------------------------------------------------------ outside
	UPROPERTY()
	TArray<TObjectPtr<USpotLightComponent>> Streetlights;
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> LampHeads;
	/** The Wash & Fold's neon over her windows (its pink across the street), and its shop window's light on the sidewalk. */
	UPROPERTY()
	TObjectPtr<URectLightComponent> NeonPink;
	UPROPERTY()
	TObjectPtr<URectLightComponent> WashSpill;
	/** Blender's fronts across the street: their lit rooms baked for the night, dimmed by day (and for this room's exposure). */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Fronts;
	/** The stand-ins' lit windows (warm, cool, TV, stairwell): dark by day. */
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> LitWindows;
	UPROPERTY()
	TArray<float> LitLevels;
	/** While building: the stand-ins' windows, one instanced set per look (dark, then the lit ones). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> WindowSets;
	/** M_Street (street_setup.py), for the street's ground and the brick across it; the stage's own surfaces without it. */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> StreetMaterial;

	// ------------------------------------------------------------ Dee
	UPROPERTY(Transient)
	TObjectPtr<ABackRoomPlayer> Dee;
	TSharedPtr<FStreamableHandle> DeeLoad;
	DeeRoom::EWhere Where = DeeRoom::EWhere::Out;
	bool bWhereKnown = false;
	bool bAtOnceNext = false;
	bool bArrived = false;
	bool bLeft = false;
	/** Seconds until her hands are settled in her lap again (the seated body's idle habits are a card table's). */
	float HabitLeft = 0.0f;
	/** The TV: on (1) while she's in her chair, fading as a set does; its picture cuts from shot to shot. */
	float TvLevel = 0.0f;
	float TvCut = 0.0f;
	FLinearColor TvColor = FLinearColor::Black;
	FLinearColor TvGoal = FLinearColor::Black;
	float TvShimmer = 0.0f;
	int32 TvScene = 0;

	TSharedPtr<SDrawListWidget> NoteSlate;
	std::vector<std::string> NoteLines;
};
