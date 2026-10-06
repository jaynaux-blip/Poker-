#pragma once

#include "CoreMinimal.h"
#include "ShortStack/Game/Hero.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/WeakObjectPtrTemplates.h"

#include "CastLooks.generated.h"

class AShortStackCharacter;

/**
 * The cast's looks off the table (spec 6.1, 6.7, 7.5.4). Phase A: whose face is whose on the Street map, and the face
 * registry people-ue's director and crowd consult. Phase B moves PersonaFor (BackRoomGameMode.cpp) and DefaultBody
 * (BackRoomLive.cpp) here, so the Back Room, the Embercrest and the street dress a person the same, and adds the staff's
 * wardrobes, heights and headwear.
 *
 * Kept faces: Benny wears ExtraB and Inez (the Wash & Fold's attendant, The Block's) ExtraK, as today; Augie wears ExtraH,
 * restyled (or MHC_Augie, if the optional seventh build is ever made, spec 11.8). The six staff wear their own builds,
 * MHC_Ray, MHC_Lonnie, MHC_Bea, MHC_Rosa, MHC_Ruby and MHC_Marlene, once each lands; until then each borrows a face:
 *   - the women from the extras the Embercrest's exterior doesn't use (ExtraJ, D, L, E and F stand at its doors): Rosa
 *     ExtraI (the low ponytail, for the trainer), Ruby ExtraC, Marlene ExtraG, Bea ExtraA (picks recorded here; each
 *     falls through to the rule below if that extra isn't built);
 *   - the men: every men's extra outside the Embercrest's five is kept already (ExtraB, ExtraH), so the extras run short
 *     and Ray and Lonnie take spare heroes (HeroA0..A2 but the player's), picked per career by live::FaceHash and stable
 *     for it; failing that, one of the Embercrest's five.
 * The player's own hero face is nobody else's. Reserved faces stay out of the crowd (ChooseCast) and the Embercrest's
 * random pools (looks adds the list to EmbercrestGameMode.cpp in Phase B); they still sit in the Back Room's crowd, which
 * is another map. People ids are town::People()'s (spec 6.1), plus "inez" and "hero".
 */

/** One person of the block and the face they wear on the Street map. */
struct FBlockFace
{
	/** The person ("benny", "lonnie") and the face: a cast build's folder name ("ExtraB", "Lonnie", "HeroA1"); None when none can be had. */
	FName Person;
	FName Face;
	bool bWoman = false;
	/** Their own MetaHuman (MHC_<Name>) is built, and this is it. */
	bool bOwnBuild = false;
	/** Someone else's face, worn until their own build lands. */
	bool bBorrowed = false;
};

namespace CastLooks
{
/** Whether a cast build exists (/Game/ShortStack/Cast/Built/MHC_<Name>/BP_MHC_<Name>), the one AShortStackCharacter::ApplyCast loads. */
SHORTSTACK_API bool IsBuilt(const FString& Name);
/** Whose body a face is: ExtraA, C, E, G, I and K and the HeroB faces are women's (BackRoomLive.cpp's lists). */
SHORTSTACK_API bool IsWomansFace(FName Face);
/** The twelve extras. */
SHORTSTACK_API const TArray<FName>& Extras();
/** The Embercrest exterior's five (EmbercrestGameMode.cpp's SpawnPeople): avoided for the named staff where possible. */
SHORTSTACK_API const TArray<FName>& EmbercrestFaces();
/** The hero MetaHuman a look is worn as (AShortStackCharacter::ApplyLook's rule): Hero<A|B><skin band>. */
SHORTSTACK_API FName HeroFaceFor(const ss::hero::Look& Look);
/**
 * Everyone on the block with a kept face, and that face, for a player wearing PlayerHero in the career CareerKey (anything
 * stable for the career, the save's id or the hero's full name: it picks the spare heroes).
 */
SHORTSTACK_API TArray<FBlockFace> BlockFaces(FName PlayerHero, const FString& CareerKey);
/** Every face nobody else may wear on the Street map now: the block's kept and borrowed faces and the player's own. */
SHORTSTACK_API TArray<FName> ReservedFaces(FName PlayerHero, const FString& CareerKey);
} // namespace CastLooks

/**
 * The Street map's faces (spec 6.1, 6.7): who may wear which (a reserved face only its person, the player's face nobody),
 * and who wears each now. A face that isn't reserved may be worn by several bodies at once (pedestrians: the extras are too
 * few for one each); the rule is that it never shows twice in view, which InViewElsewhere answers for the director before
 * it shows someone. people-ue's AStreetNpcs configures it at the load and claims for everyone it builds; the crowd asks
 * IsAllowed in ChooseCast and claims too. ss.Look.Faces (AShortStackCharacter::TestFaces) lists it.
 */
UCLASS()
class SHORTSTACK_API UCastFaceRegistry : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** The registry of Context's world (null in the editor's world, or with no world). */
	static UCastFaceRegistry* Get(const UObject* Context);

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	/** The player's hero face (AShortStackCharacter::GetCastFace on the hero) and the career's key (see CastLooks::BlockFaces). */
	void Configure(FName PlayerHero, const FString& CareerKey);
	bool IsConfigured() const { return bConfigured; }
	/** The face a person of the block wears now (None: not one of the block's, or no face to be had). */
	FName FaceFor(FName Person) const;
	/** The person a face is kept for ("hero" for the player's own; None when it's free). */
	FName ReservedFor(FName Face) const;
	/** Whether Who may wear Face here (a pedestrian is any Who that isn't the block's: NAME_None works). */
	bool IsAllowed(FName Face, FName Who) const;
	/** Body wears Face now, as Who. False when Who may not wear it (it's kept for someone else). Claiming again is harmless. */
	bool Claim(FName Face, FName Who, AShortStackCharacter* Body);
	/** Body no longer wears Face (a destroyed body's claim lapses by itself). */
	void Release(FName Face, const AShortStackCharacter* Body);
	/** The live bodies wearing Face now. */
	TArray<AShortStackCharacter*> Wearers(FName Face) const;
	/**
	 * Whether someone else wearing Face (a live body other than Body, not hidden) stands in the view from Eye along Dir: inside
	 * the cone whose half-angle has the cosine CosHalfFov, and within MaxDistance cm. The director shows Body only when it
	 * isn't, so a face never shows twice in view (spec 6.7). No traces: behind a wall counts as in view.
	 */
	bool InViewElsewhere(FName Face, const AShortStackCharacter* Body, const FVector& Eye, const FVector& Dir, float CosHalfFov = 0.5f, double MaxDistance = 6000.0) const;
	const TArray<FBlockFace>& Block() const { return BlockNow; }
	TArray<FName> Reserved() const;
	/** One line per person and per face worn (ss.Look.Faces). */
	FString Describe() const;
	/** ss.Look.Faces and its twin: configured for PlayerHero first when nobody has, or when a CareerKey is given; then Describe. */
	FString Show(FName PlayerHero, const FString& CareerKey);

private:
	struct FClaim
	{
		FName Who;
		TWeakObjectPtr<AShortStackCharacter> Body;
	};
	bool bConfigured = false;
	FName PlayerHero;
	FString CareerKey;
	TArray<FBlockFace> BlockNow;
	/** Who wears each face now, every body that does. */
	TMap<FName, TArray<FClaim>> Claims;
};
