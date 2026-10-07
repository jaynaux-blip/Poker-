#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShortStack/Game/Hero.h"
#include "ShortStack/Game/Wardrobe.h"

#include "ShortStackCharacter.generated.h"

class UBlueprintGeneratedClass;
class UCameraComponent;
class UGroomAsset;
class UGroomBindingAsset;
class UGroomComponent;
class USkeletalMesh;
class USkinnedMeshComponent;
class USpringArmComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UStreetBodyAnim;
struct FStreetOutfit;

/**
 * What someone other than the player wears (CastLooks.h dresses each person, spec 6.1 and 7.5.4): the MetaHuman's tee in
 * a color with a print or a chest graphic, trousers (its shorts dyed, or trousers fitted to their body), a jacket when one
 * is fitted to their body, a hat (only over hair that lies close) and glasses from the street's one set, and their shoes.
 */
struct FCastWear
{
	FLinearColor Shirt = FLinearColor(0.03f, 0.03f, 0.035f);
	/** A print over the shirt (CastLooks::PrintTexture: 1 stripes .. 7 camo; 0 plain) and a chest graphic (1 ..; 0 none). */
	int32 Print = 0;
	int32 Graphic = 0;
	/** The print's or the graphic's inks (InkC with alpha 0: halfway between A and B). */
	FLinearColor InkA = FLinearColor(0.85f, 0.85f, 0.82f);
	FLinearColor InkB = FLinearColor(0.05f, 0.05f, 0.06f);
	FLinearColor InkC = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
	/** The shorts' (or the trousers') color. */
	FLinearColor Pants = FLinearColor(0.02f, 0.025f, 0.035f);
	/** A jacket (wardrobe::Cut Hoodie..Track) and trousers (Jeans, Chinos, WorkPants), worn where one is fitted to their body. Count: none. */
	ss::wardrobe::Cut Outer = ss::wardrobe::Cut::Count;
	FLinearColor OuterColor = FLinearColor(0.05f, 0.05f, 0.06f);
	ss::wardrobe::Cut Bottom = ss::wardrobe::Cut::Count;
	/** On the head (Beanie..Trilby) and the face (Round..Aviators), the street's set; Count: none. */
	ss::wardrobe::Cut Hat = ss::wardrobe::Cut::Count;
	ss::wardrobe::Cut Eyes = ss::wardrobe::Cut::Count;
	FLinearColor HatColor = FLinearColor(0.03f, 0.04f, 0.06f);
	FLinearColor FrameColor = FLinearColor(0.01f, 0.01f, 0.012f);
	/** Black work shoes (baked black: never dyed), or canvas sneakers in ShoeColor. */
	bool bWorkShoes = true;
	FLinearColor ShoeColor = FLinearColor(0.7f, 0.68f, 0.62f);
	/**
	 * The body's breadth against its height (0.9 slight .. 1.15 heavy; 1 as built): two people built on the same preset body
	 * (Ruby on Lani's, as Inez is) read as different people by their build as well as their height. Clothes scale with it.
	 */
	float Breadth = 1.0f;
};

/**
 * The player on foot: the person from the character creator, walking the street in third person over
 * the shoulder or in first person from behind their own eyes (V switches, easing between the two).
 * The body is the hero MetaHuman nearest the look (hero_cast.py builds them: HeroA0..HeroB2 by body
 * type and skin tone, falling back to the Back Room's Hero, then the archetype body). It wears what the closet says
 * (spec 7.5.4's ApplyWear: the jacket, the trousers and the shoes from garments_setup.py's garment meshes, fitted to
 * this hero or its body type and tinted by the palette; the MetaHuman's tee in the Top's color, print or graphic, where
 * no jacket covers it; its shorts where no trousers do), or, with no closet, the creator's jacket in its color over jeans;
 * sneakers fitted to its feet; the grooms the look asks for from the face's groom library (HeroGrooms::Resolve: the
 * hairstyle, facial hair and brows, in the hair color); the hat and glasses from the street's one set; and it is scaled
 * to the height and build. UStreetBodyAnim walks it. Without a controller (Benny behind the counter) it
 * stands idle and watches the player when they come near. The street's director and crowd drive the rest of a person
 * through the people hooks below (where to look, talking, the face, the detail tier, sitting, working).
 *
 * Changes in place (PreviewLook, RefreshLook, ApplyWear) swap only what differs: groom assets on the worn grooms, garment
 * meshes on their components, the tee's and the garments' tints, the hat and glasses. A different body (type, skin band,
 * build, height) rebuilds.
 */
UCLASS()
class SHORTSTACK_API AShortStackCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AShortStackCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "Short Stack")
	TObjectPtr<USpringArmComponent> Boom;

	UPROPERTY(VisibleAnywhere, Category = "Short Stack")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, Category = "Short Stack")
	TObjectPtr<USkeletalMeshComponent> Face;

	/** Walking and running speeds (cm/s); Shift runs. */
	UPROPERTY(EditAnywhere, Category = "Short Stack|Movement")
	float WalkSpeed = 160.0f;
	UPROPERTY(EditAnywhere, Category = "Short Stack|Movement")
	float RunSpeed = 430.0f;

	/** Third person: the camera's distance (eased toward, so the wheel zooms smoothly) and its offset over the right shoulder (cm). */
	UPROPERTY(EditAnywhere, Category = "Short Stack|Camera")
	float ArmLength = 300.0f;
	UPROPERTY(EditAnywhere, Category = "Short Stack|Camera")
	FVector ShoulderOffset = FVector(0.0, 42.0, 18.0);
	UPROPERTY(EditAnywhere, Category = "Short Stack|Camera")
	float ThirdPersonFov = 72.0f;
	UPROPERTY(EditAnywhere, Category = "Short Stack|Camera")
	float FirstPersonFov = 84.0f;
	/** Seconds to ease between the views. */
	UPROPERTY(EditAnywhere, Category = "Short Stack|Camera")
	float SwitchSeconds = 0.35f;

	/**
	 * Dresses the character as the player made them, in what Closet says they wear (null: the creator's jacket, hat and
	 * glasses, as before the closet). A full build: call it once the actor exists, under black (the load fade); later
	 * changes go through RefreshLook, which rebuilds only for a different body.
	 */
	void ApplyLook(const ss::hero::Character& Who, const ss::wardrobe::Closet* Closet = nullptr);
	/** Dresses a non-player character from a cast member's MetaHuman (Benny: "ExtraB"), set on the floor where it stands. */
	void ApplyCast(const TCHAR* CastName, const FLinearColor& Shirt);
	/** As above, in a wardrobe (CastLooks::WearFor, WearOf) at a height (cm, 155-195). */
	void ApplyCast(const TCHAR* CastName, const FCastWear& Wear, float HeightCm);

	// ------------------------------------------------------------------ the look in place (spec 7.5.4)

	/**
	 * A moment's look (the barber's draft, the thrift's try-on): changes only what differs from what's on now (groom assets,
	 * the tee and the garments, the hat and glasses), the hero's own look kept for EndPreview. Grooms not yet in memory load
	 * asynchronously and go on when they arrive (the barber's stage preloads the face's set as it wakes: PreloadGroomSet), so
	 * nothing loads synchronously on a key press.
	 */
	void PreviewLook(const ss::hero::Look& Look, const ss::wardrobe::Closet& Closet);
	/** Back to the hero's own look, in place. */
	void EndPreview();
	bool IsPreviewing() const { return bPreviewing; }
	/** The look for keeps (a cut, a closet change, the end of a skip), in place; a different body rebuilds (ApplyLook). */
	void RefreshLook(const ss::hero::Look& Look, const ss::wardrobe::Closet& Closet);
	/** What's worn from Closet (Outer, Bottom and Shoes from garment meshes, Top as the tee, Hat and Eyes), in place. */
	void ApplyWear(const ss::wardrobe::Closet& Closet);
	/** The closet worn now (empty until one is given) and whether one was. */
	const ss::wardrobe::Closet& GetCloset() const { return ClosetNow; }
	bool HasCloset() const { return bHasCloset; }
	/**
	 * The Style Book's preload (spec 5.1): this hero face's whole groom set loaded asynchronously and held (bHold), or let go.
	 * The barber's stage calls it as it wakes and sleeps.
	 */
	void PreloadGroomSet(bool bHold);
	/** A bit per wardrobe::Cut this body can wear from a mesh of its own (garments, shoes, the hats and glasses imported). */
	uint64 GarmentKindsDrawable() const;
	/**
	 * What the hero wears in a look and closet (null: the creator's choices), as a wardrobe for where garments aren't drawn
	 * (the table: the MetaHuman's tee stands in for the jacket, in its color, a flannel's plaid on it; with no jacket the
	 * Top's own color, print or graphic). The street's own resolution of the closet, so the table and the resort read it
	 * the same (spec 7.5.4: HeroPersona).
	 */
	static FCastWear HeroWear(const ss::hero::Look& Look, int32 Age, const ss::wardrobe::Closet* Closet);
	/** What's on, in a line (ss.Look.Describe). */
	FString DescribeLook() const;

	void SetFirstPerson(bool bFirst);
	void ToggleView() { SetFirstPerson(!bFirstPerson); }
	bool IsFirstPerson() const { return bFirstPerson; }
	/** 0 third person .. 1 first person (eased). */
	float ViewBlend() const { return Blend; }

	/** From the controller each frame: movement relative to the camera (-1..1 each), running, and look deltas (degrees). */
	void Drive(float Forward, float Right, bool bRun);
	void Look(float DeltaYaw, float DeltaPitch);
	/** Raise a can or a bite to the mouth for a moment (eating and drinking from the bag). */
	void Sip() { SipAt = Clock; }
	/** How worn out the player is (0 fresh .. 1 spent, from their energy): the shoulders drop and the trunk sags. */
	void SetTired(float InTired) { Tired = FMath::Clamp(InTired, 0.0f, 1.0f); }
	/** Where the eyes are and which way they look. */
	FVector EyeLocation() const;
	FVector LookDirection() const;
	/** True once for each footfall (and how hard: walking soft, running heavy). */
	bool TakeFootstep(float& OutVolume);
	/** Puts the hat and glasses on again, after a change to the ss.Wear.* tunables (the console's ss.Wear.Refit). */
	void RefitWear();

	/**
	 * Where one of the street's hats (bHat) or pairs of glasses sits on a MetaHuman face, in the face mesh's component
	 * space in its reference pose: the hat's crown on top of the skull, the glasses' bridge before the eyes, both square
	 * to the face and sized to it (Turn 180 wears a cap backwards). The ss.Wear.* tunables override the fit once set.
	 * The Back Room can use it too, so the hero wears them the same at the table.
	 */
	static FTransform FitHeadWear(const USkeletalMesh* FaceAsset, bool bHat, float Turn = 0.0f);

	// ------------------------------------------------------------------ people (spec 7.5.4, Phase A)
	// What people-ue's director (AStreetNpcs) and crowd drive. Each works here in a minimal way; the layers behind them (the
	// attention stack, the work beats, the seated solve) are people-ue's, in UStreetBodyAnim, reading the state below.

	/** NPC gaze: where to look (world) and how much it overrides the default (0 = today's behaviour). */
	void SetLookTarget(const FVector& WorldAt, float Weight);
	/** The talk flap (jawOpen/funnel/stretch syllables and nods, TalkFace.h) for Seconds, at Loudness 0..1. */
	void SetTalking(float Seconds, float Loudness);
	bool IsTalking() const { return TalkLeft > 0.0f; }
	/**
	 * An expression control held on the face over the blinks and the flap (RigLogic's CTRL_expressions_<Control>:
	 * "browRaiseInL", "mouthCornerPullR"; 0..1, and 0 lets it go): a double take's brows, a smile. Drawn at Near only.
	 */
	void SetFaceCurve(FName Control, float Value);
	/**
	 * Detail tier 0 Near .. 3 Gone (spec 6.7): face rig, tick intervals, URO, groom LOD hold (Mid: a lower cards LOD, never
	 * the helmet), shadows, ray-tracing visibility (Near only), visibility. The Block's crowd tiers (StreetCrowd.cpp's
	 * UpdateCost), moved here as the one implementation the crowd and AStreetNpcs both call. Nothing changes until the first
	 * call. Near is each part as built (grooms on their first cards LOD); Gone hides the actor, takes it out of collision and
	 * stops its ticks (the actor's, its meshes', grooms' and movement's), and leaving Gone starts again only what Gone stopped:
	 * a caller that hides or stops someone itself (the crowd's Show) keeps that. The tier owns its parts' shadows, ray tracing,
	 * tick intervals, URO, forced LODs and grooms' visibility from its first call; take hair off with DropGroom, not by hiding it.
	 */
	void SetDetailTier(int32 Tier);
	int32 GetDetailTier() const { return DetailTierNow; }
	/** The pose layers people-ue drives (locomotion, work beats, seated). */
	UStreetBodyAnim* StreetBody() const;
	/**
	 * Sitting (the seated mode, spec 6.6): movement stops and holds until standing, the hero's input moves nobody, and the
	 * seat's height (cm; the barber chair 55, stools 75, booths 46) and a 0..1 blend eased over 0.6 s are kept for the pose.
	 * The character stands where the seat's floor mark is; the pose puts the pelvis on the seat.
	 */
	void SetSeated(bool bSit, float SeatHeight = 46.0f);
	bool IsSeated() const { return bSeated; }
	float GetSeatHeight() const { return SeatHeightCm; }
	float GetSeatBlend() const { return SeatBlend; }
	/**
	 * At work (spec 6.6's work beats): the beat ("ring", "pour", "comb"; None stops) and the station's frame (world, +X the way
	 * they face at it). The gaze rests on the work until someone or something takes it; the beat itself is people-ue's.
	 */
	void SetWork(FName Beat, const FTransform& Station);
	FName GetWorkBeat() const { return WorkBeat; }
	const FTransform& GetWorkStation() const { return WorkStation; }
	/** Seconds at the beat (for its loop). */
	float GetWorkTime() const { return WorkBeat.IsNone() ? 0.0f : Clock - WorkSince; }
	/** The MetaHuman this one is built from ("HeroA1", "ExtraB"; None for the archetype body): the face registry's key. */
	FName GetCastFace() const { return CastFaceName; }
	/** The look worn (the creator's for the hero; the default for a cast member) and the age it's worn at. */
	const ss::hero::Look& GetLook() const { return LookNow; }
	int32 GetLookAge() const { return LookAge; }

	// ------------------------------------------------------------------ grooms (spec 7.5.4)

	/** The scalp hair's groom (none when the look is bald, or before dressing), and every groom worn. */
	UGroomComponent* ScalpGroom() const;
	TArray<UGroomComponent*> GetGrooms() const;
	/**
	 * Changes a worn groom in place: the asset and its binding (baked against this face), held at this tier's LOD (cards),
	 * shaded as the look says. No rebuild; nothing loads here (the caller has the assets resident). HeroGrooms.h's spike
	 * measures what it costs.
	 */
	bool SwapGroom(UGroomComponent* Groom, UGroomAsset* Asset, UGroomBindingAsset* Binding);
	/** A scalp groom on a face built clean or a look worn bald (none in it yet: SwapGroom puts one in). */
	UGroomComponent* AddScalpGroom();
	void DropGroom(UGroomComponent* Groom);

	// ------------------------------------------------------------------ tests (the console's twins, for the remote-control scripts)

	/** ss.Look.GroomSpike: the day-one groom check (HeroGrooms.h). Candidate "all", "self", a cast name or a groom path. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestGroomSpike(const FString& Candidate, const FString& Options);
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestGroomSpikeReport() const;
	/**
	 * ss.Look.Hook: "tier 2", "talk 3 0.8", "look X Y Z 1", "lookhero 1", "face browRaiseInL 0.6", "sit 46", "stand", "work ring",
	 * "work none", "describe", or "selftest": every hook driven and put back on this character, its parts checked at each tier
	 * and against how they were before ("selftest PASS ..." or "selftest FAIL: ..."); it leaves the character as it found it.
	 */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestHook(const FString& Command);
	/** ss.Look.Faces: the face registry (CastLooks.h), configured from this hero when nobody has (or CareerKey given), described. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestFaces(const FString& CareerKey);
	/**
	 * ss.Look.Preview: "<slot> <value>" previews the look with one choice changed (hair, facialhair, brows, haircolor, hat,
	 * glasses, outfit, outfitcolor; and body, skin, build, height, which rebuild: the creator's option index), or a closet
	 * slot (top, outer, bottom, shoes, headwear, eyes) worn as a garment id or a cut name ("outer hoodie", "bottom chinos",
	 * "headwear trilby", "eyes none": the catalog's of that cut, in the outfit color where it has colors); "end" ends the
	 * preview, "keep" keeps it (RefreshLook), "redress" dresses again in place, "preload" holds the face's groom set,
	 * "grooms" describes the library, "caps" the HostCaps, "describe" what's on.
	 */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestLookPreview(const FString& Command);
	/** The hooks' state in a line. */
	FString DescribeHooks() const;

private:
	void BuildFromMetaHuman(UClass* Blueprint, const FStreetOutfit& Outfit);
	/**
	 * What's worn below the head (spec 7.5.4's ApplyWear), in place: the jacket and the trousers from garment meshes
	 * (SK_Garment_<Kind>_<this face>, or the body type's _A/_B for a hero), skinned to the hero bodies' own skeleton and
	 * following the body as their leader pose, tinted; the MetaHuman's tee and shorts dyed, each shown only where no garment
	 * covers it (a jacket with no mesh of its own is worn as the bomber, in its color); the shoes; the hat and glasses.
	 */
	void DressOutfit(const FStreetOutfit& Outfit);
	void DressGarments(const FStreetOutfit& Outfit);
	/** What the current look, closet or cast wardrobe puts on (FStreetOutfit). */
	FStreetOutfit CurrentOutfit() const;
	/** The look the grooms are chosen by: the closet's headwear counts as the hat. */
	ss::hero::Look GroomLook() const;
	/**
	 * The hero's grooms from the face's library (HeroGrooms::Resolve): the worn ones swapped in place, missing ones added,
	 * ones the look doesn't wear dropped, all in the hair color. bLoadNow loads what isn't in memory first (a build, under
	 * black); otherwise it loads asynchronously and goes on when it arrives (the latest request wins).
	 */
	void ApplyGrooms(bool bLoadNow);
	void ChangeLookInPlace(const ss::hero::Look& Look, const ss::wardrobe::Closet* Closet);
	UGroomComponent* GroomOfKind(uint8 Kind) const;
	/** A worn groom of a kind made for Asset (from the build's own groom of that kind, as its template, when it has one). */
	UGroomComponent* AddGroomOfKind(uint8 Kind, UGroomAsset* Asset, UGroomBindingAsset* Binding);
	void UseArchetype();
	void ClearWearables();
	void LinkFace();
	void FitToHeight(float HeightCm);
	void FitFace();
	void BuildShoes(const FStreetOutfit& Outfit);
	/** Hides one foot's toes in a shoe (0 left, 1 right), or shows them again. */
	void HideToes(int32 Side, bool bHide);
	void PutOnHeadwear(const FStreetOutfit& Outfit);
	/** What carries the hat and glasses: the face, or the body when the face has no head bone (none without either). */
	USkeletalMeshComponent* HeadCarrier() const;
	void WearOnHead(UStaticMeshComponent* Piece, const FTransform& InFace);
	void BecomeNpc();
	void UpdateLook(float Dt);
	void UpdateFace(float Dt);
	void UpdateCamera(float Dt);
	void RefreshVisibility();
	bool IsHeadPiece(const USceneComponent* Piece) const;
	FVector EyeLocal() const;
	void ApplyDetailTier();
	/** Starts again what Gone stopped (and only that): the actor shown, its collision, its ticks. */
	void UndoGone();
	/** TestHook("selftest"). */
	FString SelfTestHooks();
	/** Puts back what the tiers changed (before a rebuild, which records the new parts' own settings). */
	void RestoreTierDefaults();

	/** What the MetaHuman wore, copied onto us (clothes, hair, brows, lashes, beard). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<USceneComponent>> Wearables;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> HatMesh;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> GlassesMesh;
	/** The garments worn over the MetaHuman's own (garments_setup.py's skeletal meshes), when there are any. */
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> OuterGarment;
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> BottomGarment;
	/** The MetaHuman's own tee and shorts (one mesh: its shirt and shorts are its material sections). */
	TWeakObjectPtr<USkinnedMeshComponent> DefaultOutfit;
	/** The Blueprint the body was built from (its grooms are the templates for grooms added later). */
	TWeakObjectPtr<UClass> WornBlueprint;

	/** The closet worn now, and whether one was given (without: the creator's jacket, hat and glasses). */
	ss::wardrobe::Closet ClosetNow;
	bool bHasCloset = false;
	/** The hero's own look and closet while a preview is on. */
	ss::hero::Look OwnLook;
	ss::wardrobe::Closet OwnCloset;
	bool bOwnHasCloset = false;
	bool bPreviewing = false;
	/** A cast member's wardrobe (ApplyCast). */
	FCastWear CastWearNow;
	/** The latest groom set asked for: an async load that arrives after a newer request is dropped. */
	uint32 GroomRequest = 0;
	/**
	 * GarmentKindsDrawable's answer for this body before ss.Look.DenyGarments (the shops ask every couple of seconds; each
	 * answer is some thirty package lookups): found once per build, forgotten when the body changes or it's dressed again.
	 */
	mutable uint64 DrawableKinds = 0;
	mutable bool bDrawableKnown = false;
	/**
	 * The Style Book's preload is wanted (PreloadGroomSet): held for the face worn, and for the new face after a rebuild. Asked
	 * for before the hero is dressed (the barber wakes as the street is built), it's held once the face is known.
	 */
	bool bGroomSetWanted = false;
	/** The shoes on the foot bones (0 left, 1 right): the modeled pair fitted to this body's feet, or a pair made here to fit. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Shoes[2];
	/** The shoes made here, when the modeled pair isn't imported. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> ShoeMeshes[2];
	/** The body the shoe meshes were made for (they're made again for another). */
	TWeakObjectPtr<USkeletalMesh> ShoesFitFor;
	/** The toe bones hidden in the shoes, by index on the body (none: INDEX_NONE). */
	int32 HiddenToes[2] = {INDEX_NONE, INDEX_NONE};

	ss::hero::Look LookNow;
	/** The hero's age: the hair greys from the mid-forties and the hat's color is picked against it, as the creator's portrait has them. */
	int32 LookAge = 0;
	bool bHasLook = false;
	bool bNpc = false;
	/** Some of the MetaHuman's hair is on the head (a hat sits a little higher over it). */
	bool bWearsScalpHair = false;
	bool bFirstPerson = false;
	bool bHeadHidden = false;
	bool bBodyHidden = false;
	float Blend = 0.0f;
	float Phase = 0.0f;
	float Clock = 0.0f;
	float SipAt = -10.0f;
	bool bStepPending = false;
	float StepVolume = 0.0f;
	bool bRunning = false;
	float LastYaw = 0.0f;
	float LastSpeed = 0.0f;
	float Bank = 0.0f;
	float Surge = 0.0f;
	float Tired = 0.0f;
	/** Out of breath after a run (0..1, slow to fade): the mouth stays open. */
	float Winded = 0.0f;
	float MeshScale = 1.0f;
	/** The look the head eases toward (degrees from the body's facing), and where the eyes are aimed (world; zero for ahead). */
	float LookYawNow = 0.0f;
	float LookPitchNow = 0.0f;
	FVector EyeTarget = FVector::ZeroVector;
	/** The face as measured from its reference pose: the eyes and the mouth from the head bone (component space). */
	bool bFaceHasEyes = false;
	FVector MouthFromHead = FVector(0.0, 10.5, 1.0);
	/** Third person: the arm length the camera is easing to the player's choice from; first person: the eyes, steadied. */
	float ArmNow = 300.0f;
	FVector EyeSteady = FVector::ZeroVector;
	bool bEyeSteadyValid = false;
	float FovKick = 0.0f;
	/** Blinking: time to the next, and how far through one (below 0: eyes open). */
	FRandomStream Rng;
	float NextBlink = 2.0f;
	float BlinkT = -1.0f;
	bool bDoubleBlink = false;

	// The people hooks' state (spec 7.5.4).
	FVector LookTargetAt = FVector::ZeroVector;
	float LookTargetWeight = 0.0f;
	float TalkLeft = 0.0f;
	float TalkLoudness = 0.6f;
	/** Held expression controls, by their full CTRL_expressions_* name. */
	TMap<FName, float> FaceCurves;
	int32 DetailTierNow = 0;
	/** A tier has been set (until then nothing of it is applied: today's behaviour). */
	bool bTierApplied = false;
	/** What Gone stopped, and only that is started again: the actor hidden, its collision, its tick, its parts' ticks. */
	bool bTierHidActor = false;
	bool bTierStoppedCollision = false;
	bool bTierStoppedTick = false;
	TArray<TWeakObjectPtr<UActorComponent>> TierStoppedTicks;
	/** The actor's own tick interval before a tier set one (below 0: not recorded yet). */
	float TierOwnTickInterval = -1.0f;
	/** Each part's own settings before a tier changed them. */
	struct FTierPart
	{
		bool bShadow = false;
		bool bTraced = false;
		bool bVisible = true;
		bool bUro = false;
		float TickInterval = 0.0f;
		/** A skinned mesh's forced LOD (counted from 1; 0 lets the engine choose), or a groom's (its index; INDEX_NONE: none). */
		int32 ForcedLod = 0;
	};
	TMap<TWeakObjectPtr<UPrimitiveComponent>, FTierPart> TierDefaults;
	bool bSeated = false;
	float SeatHeightCm = 46.0f;
	float SeatBlend = 0.0f;
	uint8 MoveModeBeforeSeat = 0;
	FName WorkBeat;
	FTransform WorkStation = FTransform::Identity;
	float WorkSince = 0.0f;
	FName CastFaceName;
	/** The grooms worn and what each is (HeroGrooms::EGroomKind). */
	TArray<TPair<TWeakObjectPtr<UGroomComponent>, uint8>> GroomKinds;
};
