#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShortStack/Game/Hero.h"

#include "ShortStackCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;

/**
 * The player on foot: the person from the character creator, walking the street in third person over
 * the shoulder or in first person from behind their own eyes (V switches, easing between the two).
 * The body is the hero MetaHuman nearest the look (hero_cast.py builds them: HeroA0..HeroB2 by body
 * type and skin tone, falling back to the Back Room's Hero, then the archetype body), dressed in the
 * jacket color, scaled to the height and build, and walked by UStreetBodyAnim. Without a controller
 * (Benny behind the counter) it stands idle.
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

	/** Third person: the camera's distance and its offset over the right shoulder (cm). */
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
	/** Dresses a non-player character from a cast member's MetaHuman (Benny: "ExtraB"). */
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
	/** Where the eyes are and which way they look. */
	FVector EyeLocation() const;
	FVector LookDirection() const;
	/** True once for each footfall (and how hard: walking soft, running heavy). */
	bool TakeFootstep(float& OutVolume);

private:
	void BuildFromMetaHuman(UClass* Blueprint, const FLinearColor& Dye);
	void ClearWearables();
	void FitToHeight(float HeightCm);
	void UpdateCamera(float Dt);
	void UpdateAccessories();
	void SetHeadVisible(bool bVisible);
	FVector EyeLocal() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USceneComponent>> Wearables;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> HatMesh;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> GlassesMesh;

	ss::hero::Look LookNow;
	bool bHasLook = false;
	bool bFirstPerson = false;
	bool bHeadHidden = false;
	float Blend = 0.0f;
	float Phase = 0.0f;
	float Clock = 0.0f;
	float SipAt = -10.0f;
	float StepPhase = 0.0f;
	bool bStepPending = false;
	float StepVolume = 0.0f;
	bool bRunning = false;
	float LastYaw = 0.0f;
	float Bank = 0.0f;
	float MeshScale = 1.0f;
};
