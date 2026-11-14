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
 * build /Game/ShortStack/Meshes/<mesh>/<mesh>, else engine shapes; Nanite off on every piece (small props). The assets must have
 * Nanite off too (§9.4): on a Nanite asset SetForceDisableNanite draws its coarse fallback, so each budget line names any such mesh.
 * ANightOneStage::BuildSet attaches it (the lead's P6) and calls Build.
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

	/**
	 * Dev (ss.Fest.Shelf all|off; SPEC v1.7 §9.4's read of the shelf with all ten trophies against 40k triangles and 12 draw calls, §2.16
	 * step 11's look for a missing or pink mesh): while on, the shelf shows every piece (ten trophies, three belts, a full board of 40
	 * pins) in place of the career's Honors and pins. Presentation only, never saved; game thread; off again at each shelf's BeginPlay.
	 */
	static void SetDevShowAll(bool bOn);

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
	/** The pins' instanced group by its key (FestShelfDetail::FsPinBodyKey and FsPinFieldKey), made on first use: a body (Body 0 gold,
	 *  1 silver, 2 outline) or an enamel field (bField), in SrgbHex unless the group takes each instance's colour (bShared). */
	UInstancedStaticMeshComponent* PinGroup(const FString& Key, bool bField, int32 Body, uint32 SrgbHex, bool bShared);
	/** The tint material over Baked's base colour texture (none: white) at Roughness, or null without TintMaterial. */
	UMaterialInstanceDynamic* PinTint(UMaterialInterface* Baked, float Roughness);

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
	/** G1 polish (P-shelf, §9.4's board budget): the enamel field built at Build (FestShelfDetail::FsDiscMesh: a flat hexagon inside the
	 *  kit's rim, a 12-sided disc on the stand-in body) and the stand-in body (a 12-sided prism; null when SM_FS_Pin is imported). */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> FieldMesh;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> BodyShapeMesh;
	/** shops_setup.py's M_NS_ClothTint (each instance's colour its custom data 0-2), so the outline rims and the enamel fields draw in
	 *  one group each; null (not built yet): one group a colour. */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> TintMaterial;
	/** One a brand (fest::Brand's order, None's empty): its trophy (or case) on its plinth, hidden until won. */
	UPROPERTY()
	TArray<TObjectPtr<USceneComponent>> Trophies;
	/** The three belt places on the top board, hidden until a crown. */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Belts;
	/** The engraving strips, the bottom tier's then the middle tier's. */
	UPROPERTY()
	TArray<TObjectPtr<UWidgetComponent>> PlateStrips;
	/** The pins, instanced a group at a time (a grade's bodies, the fields; or a colour each without the tint material), keyed by it. */
	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UInstancedStaticMeshComponent>> PinGroups;
	TArray<TSharedPtr<SDrawListWidget>> PlateSlates;
	/** What the shelf shows now (FestShelfDetail::FsKey), "" before the first poll. */
	std::string Shown;
	bool bPlatesDrawn = false;
};
