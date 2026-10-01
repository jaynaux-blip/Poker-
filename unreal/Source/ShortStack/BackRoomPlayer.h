#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "BackRoomAnim.h"

#include "BackRoomPlayer.generated.h"

class USkeletalMeshComponent;
class UStaticMeshComponent;

/** How a player carries themselves: temperament, and how much of it the face lets out. */
USTRUCT(BlueprintType)
struct FBackRoomPersona
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Persona")
	FString Name = TEXT("Player");

	/** Resting nerves 0..1 (breathing, blinking, fidgeting). */
	UPROPERTY(EditAnywhere, Category = "Persona")
	float Nervousness = 0.3f;

	/** How much emotion reaches the face 0..1 (a pro's poker face is low). */
	UPROPERTY(EditAnywhere, Category = "Persona")
	float Expressiveness = 0.5f;

	/** How often the eyes wander 0..1. */
	UPROPERTY(EditAnywhere, Category = "Persona")
	float Restlessness = 0.5f;

	/** Playing with chips when idle 0..1. */
	UPROPERTY(EditAnywhere, Category = "Persona")
	float ChipFidget = 0.4f;

	/** Leans in (1) or sprawls back (0) at rest. */
	UPROPERTY(EditAnywhere, Category = "Persona")
	float Posture = 0.5f;

	/** Seed for this player's own rhythms. */
	UPROPERTY(EditAnywhere, Category = "Persona")
	int32 Seed = 1;
};

/**
 * A player at the Back Room's table: a MetaHuman body and face, seated and animated procedurally.
 *
 * The actor sits at a chair's front edge facing the table down its +X. Each frame it runs a small
 * behavioral model (breathing, blinks, gaze, posture, hands, emotion) and hands the result to its body
 * and face anim instances as an FBackRoomBodyPose and a set of RigLogic expression controls.
 */
UCLASS()
class SHORTSTACK_API ABackRoomPlayer : public AActor
{
	GENERATED_BODY()

public:
	ABackRoomPlayer();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }

	UPROPERTY(EditAnywhere, Category = "Short Stack")
	FBackRoomPersona Persona;

	// ------------------------------------------------------------ what the table tells the player
	/** World points of interest: the player to watch, the pot, the dealer, this player's cards and chips. */
	UPROPERTY(EditAnywhere, Category = "Short Stack|Table")
	FVector HeroEyes = FVector(-98.0, 0.0, 124.0);
	UPROPERTY(EditAnywhere, Category = "Short Stack|Table")
	FVector PotAt = FVector(0.0, 0.0, 76.0);
	UPROPERTY(EditAnywhere, Category = "Short Stack|Table")
	FVector DealerAt = FVector(100.0, 0.0, 125.0);
	UPROPERTY(EditAnywhere, Category = "Short Stack|Table")
	TArray<FVector> OthersAt;

	/** Emotional state (-1..1 valence, 0..1 arousal, 0..1 dominance), eased toward by the game. */
	UPROPERTY(EditAnywhere, Category = "Short Stack|Mood")
	float Valence = 0.0f;
	UPROPERTY(EditAnywhere, Category = "Short Stack|Mood")
	float Arousal = 0.25f;
	UPROPERTY(EditAnywhere, Category = "Short Stack|Mood")
	float Dominance = 0.5f;

	/** Sets a RigLogic control (CTRL_expressions_*, without the prefix) on top of everything, for testing. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Short Stack")
	void SetTestExpression(FName Control, float Value);

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Short Stack")
	void ClearTestExpressions();

	/** A MetaHuman body and face to use instead of the archetypes (from the Creator, once assembled). */
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	TSoftObjectPtr<USkeletalMesh> BodyMesh;
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	TSoftObjectPtr<USkeletalMesh> FaceMesh;

private:
	void Build();
	void UpdateBody(float Dt);
	void UpdateFace(float Dt);
	/** Component space of the body mesh from world. */
	FVector ToBody(const FVector& World) const;

	UPROPERTY()
	TObjectPtr<USceneComponent> Root;
	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> Body;
	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> Face;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> ChairSeat;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> ChairBack;

	UPROPERTY(Transient)
	TMap<FName, float> TestCurves;

	FRandomStream Rng;
	float Time = 0.0f;
	// Breathing.
	float BreathPhase = 0.0f;
	// Blinks.
	float NextBlink = 2.0f;
	float BlinkT = -1.0f;
	bool bDoubleBlink = false;
	// Gaze: the current target, the eyes' point, and the head lagging behind.
	FVector GazeTarget = FVector::ZeroVector;
	FVector EyeAt = FVector::ZeroVector;
	FVector HeadAt = FVector::ZeroVector;
	float GazeLeft = 0.0f;
	// Posture drift.
	float Lean = 0.3f;
	float LeanTarget = 0.3f;
	float NextShift = 8.0f;
	// Hands: resting spots in body space, eased toward.
	FVector HandAt[2];
	FVector HandGoal[2];
	float HandSwitch = 5.0f;
	int32 HandMode = 0;
	// A micro-expression flashing across the face (0..1 envelope) and which one.
	float MicroT = -1.0f;
	int32 MicroKind = 0;
	float NextMicro = 6.0f;
};
