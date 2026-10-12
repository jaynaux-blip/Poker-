// The sponsors' patches worn (next_segment_spec 3.10, ue-wear): where a patch sits on a body (the chest on spine_05, the
// sleeve on upperarm_l, the cap's on the hat's front panel), measured offline from the cast's own bodies, tees and garments;
// what it looks like (the art's SM_Wear_Patch* and M_Patch once they're imported, block-outs of the same size and frame
// until then); which brands show (the closet's, while the deal runs); and the street's Polaroid behind the Lucky Penny's
// register. The street hero (AShortStackCharacter), the table's people (ABackRoomPlayer) and the card room's field
// (BackRoomLiveCast.cpp) all wear them through here, so a patch sits the same everywhere.
#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "ShortStack/Game/Wardrobe.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include <string>

#include "SponsorWear.generated.h"

class AActor;
class FCoreCanvasTarget;
class SDrawListWidget;
class UAnimSequence;
class USkeleton;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPointLightComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTextureRenderTarget2D;
class UWidgetComponent;

namespace ss
{
namespace sponsor
{
struct Book;
}
}

namespace SponsorWear
{
/** The closet's patch spots (sponsor::Spot): the chest, the sleeve, the cap. */
constexpr int32 Chest = 0;
constexpr int32 Sleeve = 1;
constexpr int32 Cap = 2;
constexpr int32 Spots = 3;

/**
 * What a chest or sleeve patch lies on and how far it stands off it (spec 3.10's standoffs): the tee (no jacket, and every
 * body at the table, where the MetaHuman's tee stands in for the jacket), a jacket (the bomber's mesh carries the bomber,
 * the denim, the leather, the flannel and the track jacket until each has its own), the hoodie.
 */
enum class ELayer : uint8
{
	Tee,
	Track,
	Flannel,
	Jacket,
	Hoodie,
};
/** The surface a layer is measured on: 0 the tee, 1 the bomber, 2 the hoodie. */
int32 SurfaceOf(ELayer Layer);
/** The outermost garment's layer for a closet jacket (wardrobe::Cut Hoodie..Track; anything else: the tee). */
SHORTSTACK_API ELayer LayerOf(ss::wardrobe::Cut Outer);
SHORTSTACK_API const TCHAR* LayerName(ELayer Layer);

/** What carries a cap patch: the street's ball cap or beanie (wearables.py's SM_Hat_*), or the table's (wear.py's SM_Wear_*). */
enum class EHat : uint8
{
	None,
	StreetCap,
	StreetBeanie,
	TableCap,
	TableBeanie,
};

/** The brands shown on the three spots (sponsor ids; empty: nothing sewn there, or it doesn't show). */
struct SHORTSTACK_API FShown
{
	FString Brand[Spots];
	bool operator==(const FShown& O) const { return Brand[0] == O.Brand[0] && Brand[1] == O.Brand[1] && Brand[2] == O.Brand[2]; }
	bool operator!=(const FShown& O) const { return !(*this == O); }
	bool IsEmpty() const { return Brand[0].IsEmpty() && Brand[1].IsEmpty() && Brand[2].IsEmpty(); }
	/** "chest luckypenny, cap voltrush" ("none"). */
	FString Describe() const;
};

/**
 * The closet's patches that show: sewn on a spot that shows on what's worn (wardrobe::PatchFits: the cap's only on a ball
 * cap, a backwards cap or a beanie), and, given the book, only while that brand's deal runs at World (a deal that ends takes
 * its patch with it: core unpicks it from the closet, and this keeps a stale copy of the closet from wearing it meanwhile).
 */
SHORTSTACK_API FShown FromCloset(const ss::wardrobe::Closet& C, const ss::sponsor::Book* Book, double World);
/**
 * Someone in the world wearing their brand (the entry's livepatch line): on the first spot the brand's patch takes that shows
 * on them; a cap brand under a bare head (or a hat with no front panel) on the sleeve; nothing for a brand that's only an
 * overlay on a stream.
 */
SHORTSTACK_API FShown ForWorldPerson(const FString& Brand, bool bCapShows);
/** ss.Wear.PatchTest's override (testing: a brand on a spot whatever the closet says), laid over Shown. */
SHORTSTACK_API void ApplyTest(FShown& Shown);
/** Bumped by ss.Wear.PatchTest: the wearers check it and dress again. */
SHORTSTACK_API uint32 TestRevision();
/** ss.Wear.PatchLog 1: every patch put on is logged (who, which spot, the fit). */
SHORTSTACK_API bool ShouldLog();
/** The cap patch's carrier for a hat mesh by its name (SM_Hat_Cap, SM_Hat_Beanie, SM_Wear_Cap, SM_Wear_Beanie; else None). */
SHORTSTACK_API EHat HatOf(const UStaticMesh* Hat);

/** A patch's place: its transform in the body mesh's component space in the reference pose, and the bone that carries it. */
struct FFit
{
	FTransform InComponent = FTransform::Identity;
	FName Bone;
	/** Measured on this body (else its build's typical numbers stand in). */
	bool bMeasured = false;
	/** How far the patch's back stands off the body's own surface there (cm), for the log. */
	float Depth = 0.0f;
};
/** BodyName: the MetaHuman's build ("HeroA1", "ExtraC"; None for the archetype); bWoman picks the spot for a body not measured. */
SHORTSTACK_API bool FitChest(const USkeletalMesh* Body, FName BodyName, bool bWoman, ELayer Layer, FFit& Out);
SHORTSTACK_API bool FitSleeve(const USkeletalMesh* Body, FName BodyName, bool bWoman, ELayer Layer, FFit& Out);
/** The cap patch in the hat mesh's own space (the front panel's middle, square to it). */
SHORTSTACK_API bool FitCap(EHat Hat, FTransform& Out);
/** "chest luckypenny on the hoodie (HeroA1, measured, 15.5 cm out)": one patch put on, for the logs. */
SHORTSTACK_API FString Note(int32 Spot, const FString& Brand, ELayer Layer, FName Body, const FFit& Fit);
/** The build a MetaHuman Blueprint's path names (".../MHC_ExtraC/BP_MHC_ExtraC.BP_MHC_ExtraC_C" -> ExtraC), or None. */
SHORTSTACK_API FName BodyNameOf(const FString& BlueprintPath);
/** A woman's build (CastLooks's list): her chest patch sits above the bust. */
SHORTSTACK_API bool IsWomansBody(FName Body);

/**
 * Piece carried by Bone of Parent at InComponent (the reference pose's component space), whatever the component's scale: as
 * AShortStackCharacter::WearOnHead carries the hat, the offset is put where the scaled bone has it.
 */
SHORTSTACK_API void Carry(UStaticMeshComponent* Piece, USkeletalMeshComponent* Parent, FName Bone, const FTransform& InComponent);
/**
 * The patch component in Slot (made on Owner the first time, never colliding, no shadow: 2.5 mm of cloth), wearing Brand's
 * patch for Spot; hidden with an empty Brand. False when there's nothing to wear it with (no game instance: the editor).
 */
SHORTSTACK_API bool Dress(AActor* Owner, TObjectPtr<UStaticMeshComponent>& Slot, int32 Spot, const FString& Brand);

/**
 * The street's baked standing idle for Skeleton (UStreetMocap's StandIdle: the standard set's, else the heavy build's), on
 * its own: never the street's whole library, whose first use loads and samples fifty clips (a rail backer stands up in the
 * middle of a night). bLoad: load the one package now if it isn't in memory; else only what PrewarmStanding brought in.
 * Null when none is baked for that skeleton.
 */
SHORTSTACK_API UAnimSequence* StandingIdle(const USkeleton* Skeleton, bool bLoad = true);
/**
 * The standing idles' packages loaded in the background (the card room calls it as its field is first seated), each handed
 * to Keep as it lands so it stays in memory until a backer stands: no load at all in the middle of the night.
 */
SHORTSTACK_API void PrewarmStanding(TFunction<void(UObject*)> Keep);
} // namespace SponsorWear

/**
 * The patches' art for the session (spec 3.10: the logo drawn once per brand per session): the meshes (the art's once
 * imported, else block-outs made here at the same size, frame and slots), and the brand on each. With M_Patch imported, a
 * patch is the brand's cell of the embroidered atlas by custom primitive data (no material per patch); without it, the
 * brand's sheet (sponsorart::Patch, the round patch its mark) drawn once into a render target on M_WidgetLit, the merrowed
 * edge in its thread.
 */
UCLASS()
class SHORTSTACK_API USponsorWearArt : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static USponsorWearArt* Get(const UObject* Context);
	virtual void Deinitialize() override;

	/** Shape 0 wide (the chest), 1 round (the sleeve), 2 the cap's; bArt: the art's own mesh (else the block-out). */
	UStaticMesh* Mesh(int32 Shape, bool& bArt);
	/** Brand's look on a patch of Shape. */
	void Put(UStaticMeshComponent* Patch, int32 Shape, const FString& Brand);
	/** What's in use, in a line (ss.Wear.Patches). */
	FString Describe() const;

private:
	struct FBrandArt
	{
		UMaterialInterface* Face[2] = {nullptr, nullptr}; // the sheet path: wide, round
		UMaterialInterface* Edge = nullptr;
	};
	void Look();
	UMaterialInterface* PatchInstance(int32 Shape, int32 Part);
	const FBrandArt& ArtFor(const FString& Brand);
	UStaticMesh* BuildBlockOut(int32 Shape);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> Keep;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> Meshes[3];
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PatchMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> Instances[4];
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> SheetMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> SurfaceMaterial;
	bool bLooked = false;
	bool bArtMesh[3] = {false, false, false};
	TMap<FString, FBrandArt> Art;
	TSharedPtr<FCoreCanvasTarget> Painter;
};

/**
 * The Lucky Penny's Polaroid (spec 3.1 scene 1, 3.10): the player's picture on the corkboard behind the register by the
 * lotto, PENNY'S PICK in Sharpie (sponsorart::Polaroid), pinned up while the Lucky Penny's deal runs and taken down when it
 * ends. The flash goes off when the deal is signed or the month's photo is taken with the player in the store. The street
 * stage places it (AStreetStage's store dressing); it reads the street's session (CastLooks::BoundSession) four times a second.
 */
UCLASS()
class SHORTSTACK_API UPennyPolaroid : public USceneComponent
{
	GENERATED_BODY()

public:
	UPennyPolaroid();
	/** The printed-sign material (M_WidgetLit), a mesh and material for the pin; before it registers. */
	void Setup(UMaterialInterface* InPrinted, UStaticMesh* PinMesh, UMaterialInterface* PinMaterial);
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	/** Testing (ss.Wear.Polaroid): up, down, or as the deal says (-1); and the flash. */
	void Force(int32 Up);
	void Flash();
	FString Describe() const;

protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	/** The photo, the pin and the flash go with it (the street rebuilt in play clears its parts: none is left behind). */
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

private:
	/** The photo, the pin and the flash, made once (in a game world, once there's a material to print with). */
	void BuildParts();
	void Show(bool bShow);
	void Draw(const ss::hero::Look& Look);

	UPROPERTY(Transient)
	TObjectPtr<UWidgetComponent> Photo;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Pin;
	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> FlashLight;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> Printed;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> PinMeshAsset;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PinMat;
	TSharedPtr<SDrawListWidget> Slate;
	uint32 LookKey = 0;
	bool bDrawn = false;
	bool bUp = false;
	/** -1: as the deal says; 0 or 1: forced (testing). */
	int32 Forced = -1;
	/** The deal's state last seen: running, and the photos taken this period (-1: not seen yet). */
	int32 SeenRunning = -1;
	int32 SeenPhotos = -1;
	float Check = 0.0f;
	float FlashT = -1.0f;
};

/** ue-wear's state on the card room's game mode (ABackRoomGameMode::WearHost; BackRoomLiveCast.cpp). */
struct FWearHost
{
	/** The entry whose badges and patches LiveBadges and LivePatches hold. */
	FString FilledFor;
	/** The player's own patches at the table (the closet's, filtered at the night's clock), worn once an entry (a second
	 *  event played in the same visit is dressed again at its own clock). */
	bool bHeroDressed = false;
	SponsorWear::FShown HeroShown;
	/** ss.Wear.PatchTest's revision the field was dressed at. */
	uint32 TestRevision = 0;
};
