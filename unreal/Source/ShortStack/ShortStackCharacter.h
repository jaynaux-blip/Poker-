#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShortStack/Game/Hero.h"

#include "ShortStackCharacter.generated.h"

class UCameraComponent;
class UGroomAsset;
class UGroomBindingAsset;
class UGroomComponent;
class USkeletalMesh;
class USpringArmComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UStreetBodyAnim;
struct FStreetOutfit;

/**
 * The player on foot: the person from the character creator, walking the street in third person over
 * the shoulder or in first person from behind their own eyes (V switches, easing between the two).
 * The body is the hero MetaHuman nearest the look (hero_cast.py builds them: HeroA0..HeroB2 by body
 * type and skin tone, falling back to the Back Room's Hero, then the archetype body). It wears jeans and a
 * bomber in the jacket color (PutOnGarments; until they're imported, the MetaHuman's tee in that color, Flannel
 * bringing its plaid, and shorts in a tone that goes with it), sneakers fitted to its feet, the hair color,
 * facial hair and hat and glasses from the creator, and is scaled to
 * the height and build; UStreetBodyAnim walks it. Without a controller (Benny behind the counter) it
 * stands idle and watches the player when they come near. The street's director and crowd drive the rest of a person
 * through the people hooks below (where to look, talking, the face, the detail tier, sitting, working).
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

	/** Dresses the character as the player made them (call once the actor exists; again to change). */
	void ApplyLook(const ss::hero::Character& Who);
	/** Dresses a non-player character from a cast member's MetaHuman (Benny: "ExtraB"), set on the floor where it stands. */
	void ApplyCast(const TCHAR* CastName, const FLinearColor& Shirt);

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
	/** The hooks' state in a line. */
	FString DescribeHooks() const;

private:
	void BuildFromMetaHuman(UClass* Blueprint, const FStreetOutfit& Outfit);
	/**
	 * Real clothes in place of the MetaHuman's tee and shorts, when garments_setup.py has imported them for the body type:
	 * straight-leg jeans and a bomber (SK_Garment_Jeans_<A|B>, SK_Garment_Jacket_<A|B>, or a pair fitted to Hero itself),
	 * skinned to the hero bodies' own skeleton and following the body as their leader pose, the jacket in the creator's
	 * jacket color. Hero is the hero_cast MetaHuman being worn (HeroA1); for any other (the Back Room's Hero standing in), or
	 * without both pieces, nothing changes.
	 */
	void PutOnGarments(const FStreetOutfit& Outfit, const FString& Hero);
	void UseArchetype();
	void ClearWearables();
	void LinkFace();
	void FitToHeight(float HeightCm);
	void FitFace();
	void BuildShoes(const FStreetOutfit& Outfit);
	/** Hides one foot's toes in a shoe (0 left, 1 right), or shows them again. */
	void HideToes(int32 Side, bool bHide);
	void PutOnHeadwear();
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
