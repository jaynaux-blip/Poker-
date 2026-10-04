#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShortStack/Game/Hero.h"

#include "ShortStackCharacter.generated.h"

class UCameraComponent;
class USkeletalMesh;
class USpringArmComponent;
class UStaticMesh;
class UStaticMeshComponent;
struct FStreetOutfit;

/**
 * The player on foot: the person from the character creator, walking the street in third person over
 * the shoulder or in first person from behind their own eyes (V switches, easing between the two).
 * The body is the hero MetaHuman nearest the look (hero_cast.py builds them: HeroA0..HeroB2 by body
 * type and skin tone, falling back to the Back Room's Hero, then the archetype body). It wears the
 * jacket color on the shirt (Flannel brings its plaid), shorts in a tone that goes with it, sneakers
 * fitted to its feet, the hair color, facial hair and hat and glasses from the creator, and is scaled to
 * the height and build; UStreetBodyAnim walks it. Without a controller (Benny behind the counter) it
 * stands idle and watches the player when they come near.
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

private:
	void BuildFromMetaHuman(UClass* Blueprint, const FStreetOutfit& Outfit);
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
};
