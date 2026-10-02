#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "BackRoomCard.generated.h"

class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class UTexture;

/**
 * One playing card on the Back Room's table: SM_Card with M_Card (backroom_setup.py), which bends
 * the card for a peek in the vertex shader.
 *
 * The card moves on its own: it flies where it is pitched (an arc, a spin and a short slide on the
 * felt) and turns over in place; a hand that holds it places it every frame. Its rest pose is a
 * transform on the felt with the face up or down; "Lift" curls the near edge up for a peek.
 */
UCLASS()
class SHORTSTACK_API ABackRoomCard : public AActor
{
	GENERATED_BODY()

public:
	ABackRoomCard();

	virtual void Tick(float DeltaSeconds) override;

	/** Which card this is (ss::Card: rank << 2 | suit), or -1 for a card nobody has seen. */
	void SetCard(int32 Card);
	int32 GetCard() const { return Card; }

	/** Pitches the card to lie at Target (on the felt) after Duration, along a low arc with a spin. */
	void PitchTo(const FTransform& Target, float Duration, float Arc = 6.0f, float Spin = 0.0f, bool bFlipOnLand = false);
	/** Slides along the felt to Target (folds, mucks, collecting). */
	void SlideTo(const FTransform& Target, float Duration);
	/** Turns the card over where it lies, lifting it off the felt as it flips. */
	void Flip(bool bFaceUp, float Duration = 0.35f);
	/** Sets the face without animating. */
	void SetFaceUp(bool bFaceUp);
	bool IsFaceUp() const { return bFaceUp; }
	bool IsMoving() const { return MoveT < 1.0f || FlipT < 1.0f; }

	/**
	 * Curls the short edge facing Toward (a world point) up by Amount (0..1): a peek. At 1 the edge stands
	 * MaxLift radians up: about 60 degrees for a careful player across the table; the hero's own peek curls
	 * on past upright (deeper, softer) so the face's index turns up to the eyes above it.
	 */
	void SetPeek(float Amount, const FVector& Toward, float MaxLift = 1.05f);
	/** Stops any motion where the card is (a hand has picked it up and places it each frame). */
	void Stop() { MoveT = FlipT = 1.0f; }

	/** Where the card lies (or will, once it lands). */
	FTransform GetRest() const { return Rest; }
	UStaticMeshComponent* GetMesh() const { return Mesh; }

private:
	void ApplyMaterials();
	void ApplyBend();

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> Mids;
	UPROPERTY()
	TObjectPtr<UTexture> FaceTexture;
	UPROPERTY()
	TObjectPtr<UTexture> BackTexture;

	int32 Card = -1;
	bool bFaceUp = false;
	/** Where the card lies: Yaw is its turn on the felt, the face is applied on top. */
	FTransform Rest;

	// Moving from From to Rest: t 0..1 over Duration.
	FTransform From;
	float MoveT = 1.0f;
	float MoveDuration = 0.3f;
	float MoveArc = 0.0f;
	float MoveSpin = 0.0f;
	bool bSliding = false;
	bool bFlipOnLand = false;

	// Turning over: 0..1.
	float FlipT = 1.0f;
	float FlipDuration = 0.35f;

	// The peek's curl.
	float Peek = 0.0f;
	float PeekAngle = 0.0f;
	float PeekMax = 1.05f;
};
