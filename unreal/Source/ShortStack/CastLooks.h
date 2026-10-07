#pragma once

#include "CoreMinimal.h"
#include "ShortStack/Game/Hero.h"
#include "ShortStack/UI/ShopScreen.h"
#include "ShortStackCharacter.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/WeakObjectPtrTemplates.h"

#include "CastLooks.generated.h"

class AShortStackCharacter;
struct FBackRoomPersona;

namespace ss
{
class Session;
}

/**
 * The cast's looks off the table and on it (spec 6.1, 6.7, 7.5.4): whose face is whose on the Street map, the face
 * registry people-ue's director and crowd consult, and how everyone is dressed. PersonaFor (from BackRoomGameMode.cpp) and
 * DefaultBody (from BackRoomLive.cpp) live here, so the Back Room, the Embercrest and the street dress a person the same:
 * a stranger's tee, print, trousers, hat and glasses at Dee's table are what they wear walking past 1812.
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
 * doors (EmbercrestGameMode swaps one for a free face of the same body); they still sit in the Back Room's crowd, which
 * is another map. People ids are town::People()'s (spec 6.1), plus "inez" and "hero".
 *
 * The staff's wardrobes, heights and builds (spec 6.1; WearFor, HeightFor): Lonnie a white barber's smock over charcoal
 * trousers (175 cm), Rosa a teal track jacket over a ringer tee and black trousers (168), Marlene the night waitress's pink
 * uniform, top and trousers (163), Ruby a burgundy cardigan over dark slacks (170, fuller: her build is on Lani's preset
 * body, as Inez's face is), Bea a heather cardigan, grey slacks and round reading glasses (158, slight), Ray the Lucky
 * Penny's red polo over khakis (178), Augie a short-sleeved white shirt, grey trousers and gold wire readers (172), Benny the
 * store's red over jeans (183), Inez her smock over slacks (157, slight). Anyone else is as tall as a seed of their id makes
 * them, for their sex as town::People lists it (PedestrianHeight's ranges).
 *
 * Garments on people other than the hero: a jacket or trousers in a wardrobe is worn from a garment fitted to that person's
 * own body (garments.py fits any exported MetaHuman: SK_Garment_<Kind>_<Face>; the lead's steps fit the staff's chinos and
 * Rosa's bomber as each build lands, and the extras' jeans, chinos, bomber and hoodie), never from
 * another body's (a preset body's proportions differ: the hero's type garments would clip), unless ss.Look.NpcGarments 1
 * allows the body type's. Until their fit is imported the MetaHuman's own tee and shorts carry the wardrobe, dyed. Rosa's
 * track jacket is the bomber's cut in teal until the P1 track jacket lands; the slacks and trousers are the chinos' cut in
 * their color. Strangers (WearOf) wear jeans or chinos, and about two in five a hoodie or a bomber over the shirt; the known
 * faces keep their table look on the street.
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
/** Whose body a face is: ExtraA, C, E, G, I and K, the HeroB faces and the women staff are women's (BackRoomLive.cpp's lists). */
SHORTSTACK_API bool IsWomansFace(FName Face);
/** The twelve extras. */
SHORTSTACK_API const TArray<FName>& Extras();
/** The Embercrest exterior's five (EmbercrestGameMode.cpp's SpawnPeople): avoided for the named staff where possible. */
SHORTSTACK_API const TArray<FName>& EmbercrestFaces();
/** The hero MetaHuman a look is worn as (AShortStackCharacter::ApplyLook's rule): Hero<A|B><skin band>. */
SHORTSTACK_API FName HeroFaceFor(const ss::hero::Look& Look);
/** Anything stable for a career (the hero's full name), for BlockFaces' spare heroes: every map passes the same. */
SHORTSTACK_API FString CareerKey(const ss::hero::Character& Person);
/**
 * Everyone on the block with a kept face, and that face, for a player wearing PlayerHero in the career CareerKey (anything
 * stable for the career, the save's id or the hero's full name: it picks the spare heroes).
 */
SHORTSTACK_API TArray<FBlockFace> BlockFaces(FName PlayerHero, const FString& CareerKey);
/** Every face nobody else may wear on the Street map now: the block's kept and borrowed faces and the player's own. */
SHORTSTACK_API TArray<FName> ReservedFaces(FName PlayerHero, const FString& CareerKey);
/** Face itself when it isn't in Reserved, else a built extra of the same body that isn't (and isn't in Avoid); None if there's none. */
SHORTSTACK_API FName FreeFaceLike(FName Face, const TArray<FName>& Reserved, const TArray<FName>& Avoid = TArray<FName>());

// ------------------------------------------------------------------ the Back Room's people (moved here, spec 7.5.4)

/**
 * A person's table persona by name (ABackRoomGameMode::PersonaFor forwards here): the regulars' and the Embercrest's
 * Sunday faces as written, anyone else a stranger with a couple of the common tells and the same clothes every night (a
 * print or a graphic on about half the shirts, trousers, now and then a hat over short hair, glasses or shades).
 */
SHORTSTACK_API FBackRoomPersona PersonaFor(const FString& Name);
/** The cast build a name sits in at the table ("Big Lou" BigLou, "You" Hero); empty for anyone without their own. */
SHORTSTACK_API FString CastAssetFor(const FString& Name);
/**
 * The body an entrant's tournament id picks (LIVE_TOURNAMENTS.md section 10, moved from BackRoomLive.cpp): their own
 * MetaHuman for the cast, the block's face for the block's people (Lonnie plays the Monday Nightly), one of the room's
 * that suits a regular of the room's, any of the twelve for anyone else.
 */
SHORTSTACK_API FString DefaultBody(const FString& Id, const FString& Name);

// ------------------------------------------------------------------ wardrobes (spec 6.1, 7.5.4)

/** What a person of the block wears (WearFor) and how tall they are (cm, 155-195). Unknown people: a stranger's by their id. */
SHORTSTACK_API FCastWear WearFor(FName Person);
SHORTSTACK_API float HeightFor(FName Person);
/** A table persona's clothes as the street draws them (Dee's regulars outside the Wash & Fold, a stranger by name). */
SHORTSTACK_API FCastWear WearOf(const FBackRoomPersona& P);
/** A pedestrian's height from a seed: women 155-175 cm, men 165-195, most near the middle. */
SHORTSTACK_API float PedestrianHeight(uint32 Seed, bool bWoman);
/** The shirts' prints (T_Print_*) and chest graphics (T_Graphic_*) by the persona's numbering (0 none); null past the list. */
SHORTSTACK_API const TCHAR* PrintTexture(int32 Index);
SHORTSTACK_API float PrintTiling(int32 Index);
SHORTSTACK_API const TCHAR* GraphicTexture(int32 Index);
SHORTSTACK_API int32 PrintCount();
SHORTSTACK_API int32 GraphicCount();

// ------------------------------------------------------------------ what the 3D can show (spec 5.0, 7.4)

/**
 * The shop screens' HostCaps for the hero: the hairstyles, facial hair and brows their face has grooms for
 * (HeroGrooms::FillCaps), the garment kinds with a mesh on their body (less ss.Look.DenyGarments), the gear the pawn's shelf
 * can stand up (a mesh for its picture), the curios that render (none until the P1 pieces land), and Preview3D.
 */
SHORTSTACK_API ss::ui::HostCaps HostCapsFor(const AShortStackCharacter* Hero);
/** A bit per gear::Catalog index whose item has a mesh the pawn's shelf can stand up (the apartment's gear meshes). */
SHORTSTACK_API uint64 GearKindsDrawable();

// ------------------------------------------------------------------ the console's session (ss.Closet.*, ss.Fit.Set)

/**
 * The session a map plays (the street's, the apartment's: each game mode binds its own as its game starts and unbinds it as
 * it ends): the console's ss.Closet.* and ss.Fit.Set act on it, and the people director reads it. Null unbinds.
 */
SHORTSTACK_API void BindSession(const UObject* WorldContext, ss::Session* Session);
SHORTSTACK_API ss::Session* BoundSession(const UObject* WorldContext);
/**
 * The hero's look on a map where only a save is at hand (the table: BackRoomGameMode binds it as it seats the hero, and the
 * hero's head at the table resolves its grooms from it, brows included). HeroLookOn: false when none is bound.
 */
SHORTSTACK_API void BindHeroLook(const UObject* WorldContext, const ss::hero::Look& Look);
SHORTSTACK_API bool HeroLookOn(const UObject* WorldContext, ss::hero::Look& OutLook);
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

	/**
	 * ss.Closet's twin: "list", "give <garment id|all>", "wear <garment id>" (Session::Wear), "off <slot>" (Session::TakeOff), on
	 * the session bound to WorldContext's map (CastLooks::BindSession); the hero is dressed in the result (RefreshLook).
	 */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test", meta = (WorldContext = "WorldContext"))
	static FString TestCloset(const UObject* WorldContext, const FString& Command);
	/** ss.Fit.Set's twin: the career's fitness (0-100; below 0 leaves it), and what it does at the table (body::TableOf). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test", meta = (WorldContext = "WorldContext"))
	static FString TestFit(const UObject* WorldContext, float Level);

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
