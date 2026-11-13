#pragma once

#include "Components/SceneComponent.h"
#include "CoreMinimal.h"

#include <string>

#include "FestShelf.generated.h"

class SDrawListWidget;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;
class UWidgetComponent;
class FNightOneGame;

namespace FestShelfDetail
{
struct FFsPlan;
}

/**
 * Update 4, Festival Season (SPEC v1.7 §9.4, §2.6.4, §9.1 rule 7; G1, F-look FL-9): the festival shelf against the apartment's right
 * wall and the cork pin board on the left wall by the desk. The shelf is the Blender kit's (art/blender/assets/fest_shelf.py,
 * fest_trophies.py, fest_belt.py; FL-8a/FL-8b): ten plinths, one a festival, each with its brass plate; a won festival's trophy stands
 * on its plinth (Ring Rush's ring and the Championship Online's bracelet in their cases) and its plate carries the edition's name; an
 * empty plinth's plate reads U39 ("RCOP · not yet", FestArtInventory's entry, engraved here); up to three belts lie on the top board.
 * The board (fest_pinboard.py) holds the hero's festival pins, a year a row (the newest on top), in the year's calendar order, each
 * pin's enamel in its look's Accent and its rim by grade (gold, silver, or an outline in Accent2).
 *
 * Derived, never saved: twice a second (as ANightOneStage::UpdateSponsorProps) it reads the career's living world on the game thread
 * (the hero's Main Honors for trophies, crown Honors for belts, world::HeroFestPins for pins) and rebuilds only when what it shows
 * changed. Title objects (a championship's Mini and Micro) are 2D only and never stand here; ss.Fest.Moment previews write no Honor,
 * so they never place a trophy. While the room is packed into cartons (evicted) the shelf and the board are hidden.
 *
 * Each mesh loads from the CC0 build's soft path /Game/ThirdParty/FestShelf/<mesh> (git-ignored), else the committed procedural
 * build /Game/ShortStack/Meshes/<mesh>/<mesh>, else engine shapes; Nanite off on every piece (small props). ANightOneStage::BuildSet
 * attaches it (the lead's P6) and calls Build.
 */
UCLASS(ClassGroup = (ShortStack))
class SHORTSTACK_API UFestShelf : public USceneComponent
{
	GENERATED_BODY()

public:
	UFestShelf();

	/** Builds the shelf, its ten plinths and plates, the hidden trophies and belts, and the pin board, in the owning stage's space. */
	void Build();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	template <typename T>
	T* FsPart(USceneComponent* Parent);
	UMaterialInstanceDynamic* FsSurface(uint32 SrgbHex, float Roughness, float Metallic, float Pattern = 0.0f);
	UStaticMeshComponent* FsMesh(UStaticMesh* Mesh, UMaterialInterface* Material, const FVector& Location, const FRotator& Rotation, const FVector& Scale,
		USceneComponent* Parent, bool bShadows = true);
	UInstancedStaticMeshComponent* FsInstanced(UStaticMesh* Mesh, UMaterialInterface* Material, USceneComponent* Parent, bool bShadows = true);
	/** The engine-shape stand-ins when SM_FS_<Name> isn't imported (a fresh clone before the editor's import). */
	void BuildShelfShapes();
	void BuildBoardShapes();
	/** The plates' engraving: one transparent strip a tier over its five plates, drawn by ShortStackCore's canvas. */
	void AttachPlateSlate();
	void DrawPlates(const FestShelfDetail::FFsPlan& Plan, FNightOneGame& Game);
	void Apply(const FestShelfDetail::FFsPlan& Plan, bool bPacked);
	void ShowPins(const FestShelfDetail::FFsPlan& Plan);
	UInstancedStaticMeshComponent* PinGroup(const FString& Key, bool bEnamel, uint32 SrgbHex, bool bMetal);

	UPROPERTY()
	TObjectPtr<USceneComponent> ShelfRoot;
	UPROPERTY()
	TObjectPtr<USceneComponent> BoardRoot;
	UPROPERTY()
	TObjectPtr<UMaterialInterface> SurfaceMaterial;
	/** The stand-ins' and the pins' colours (M_Surface, else the engine's basic material), one a colour and finish. */
	UPROPERTY()
	TMap<FString, TObjectPtr<UMaterialInstanceDynamic>> Surfaces;
	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> CylinderMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> PinMesh;
	/** One a brand (fest::Brand's order, None's empty): its trophy (or case) on its plinth, hidden until won. */
	UPROPERTY()
	TArray<TObjectPtr<USceneComponent>> Trophies;
	/** The three belt places on the top board, hidden until a crown. */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Belts;
	/** The engraving strips, the bottom tier's then the middle tier's. */
	UPROPERTY()
	TArray<TObjectPtr<UWidgetComponent>> PlateStrips;
	/** The pins, instanced a material at a time (a rim colour, an enamel colour), keyed by it. */
	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UInstancedStaticMeshComponent>> PinGroups;
	TArray<TSharedPtr<SDrawListWidget>> PlateSlates;
	/** What the shelf shows now (FestShelfDetail::FsKey), "" before the first poll. */
	std::string Shown;
	bool bPlatesDrawn = false;
};
