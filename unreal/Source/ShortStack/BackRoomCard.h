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
	 * A peek: the corner nearest Toward (a world point: the peeker's eyes) that carries an index is lifted and
	 * curled back toward them: the stack's near edge comes up about 5.6 cm deep, strongest at the corner and easing away along the edge
	 * (the shape is tunable: ss.PeekDepth, ss.PeekCup, ss.PeekTaper0/1). At Amount 1 its
	 * tip stands MaxLift radians up: about 60 degrees for a careful player across the table; the hero's own
	 * peek curls on past upright, so the face's index turns up to the eyes above it.
	 */
	void SetPeek(float Amount, const FVector& Toward, float MaxLift = 1.05f);
	/** The lifted corner, at Amount of a peek from Toward (world): where the very tip is. */
	FVector GetPeekTip(float Amount, const FVector& Toward, float MaxLift) const;
	/** Where fingers pinch the lifted corner: the tip, drawn a little in along the near edge so the index stays clear. */
	FVector GetPeekGrip(float Amount, const FVector& Toward, float MaxLift) const;
	/**
	 * Cards lying one on another bend together or not at all: a flap in world space (the way into the card, and a
	 * point on the hinge line), the same hinge for each card of the stack, so their curls nest (each inside the
	 * one beneath it by NestGap) instead of cutting through one another.
	 */
	struct FPeekFlapWorld
	{
		FVector Dir = FVector::ZeroVector;
		FVector Vertex = FVector::ZeroVector;
		/** Half the distance between the stacked cards' corners along the hinge: the tongue is this much wider. */
		float Spread = 0.0f;
	};
	/** The flap two cards of a peek share: the lower card's, widened to take both corners. */
	static FPeekFlapWorld MakeSharedFlap(const ABackRoomCard& Under, const ABackRoomCard& Over, const FVector& Toward);
	/** How much tighter the upper card of a stack curls than the one beneath it (cm): a card's thickness and the air between. */
	static constexpr float NestGap = 0.2f;
	/** A peek of this card as part of a stack (RadiusReduce: its curl's radius is that much smaller than the lowest card's). */
	void SetPeekShared(float Amount, const FPeekFlapWorld& Flap, float MaxLift, float RadiusReduce, const FVector& Toward);
	/** Where the pinch is on the shared flap; Height above (+) or below (-) the card's skin, along its normal. */
	FVector GetPeekGripShared(float Amount, const FPeekFlapWorld& Flap, float MaxLift, float RadiusReduce, float Height, const FVector& Toward) const;
	/** Which side of the peeker (the right vector of where they sit) the lifted corner is on: +1 right, -1 left. */
	float GetPeekSide(const FVector& Toward, const FVector& PeekerRight) const;
	/**
	 * Which side of a player (+1 their right, -1 their left) the index corner of a face-down card lies on: the
	 * near corner a peek lifts, on a card held the usual way (its long side toward them, the print's top away).
	 * The table fans the hole cards by it (the front card's index corner stays clear of the back card's).
	 */
	static float NearIndexSide();
	/** The card's size (cm). */
	static constexpr double Width = 6.35;
	static constexpr double Length = 8.89;
	/** Stops any motion where the card is (a hand has picked it up and places it each frame). */
	void Stop() { MoveT = FlipT = 1.0f; }

	/** Where the card lies (or will, once it lands). */
	FTransform GetRest() const { return Rest; }
	UStaticMeshComponent* GetMesh() const { return Mesh; }

private:
	void ApplyMaterials();
	void ApplyBend();

	/** The dog-ear a peek lifts: its corner (local, signs), the direction into the card it hinges along, and where. */
	struct FFlap
	{
		FVector2D Corner = FVector2D(1.0, 1.0);
		float Angle = 0.0f;
		float Hinge = 0.0f;
		/** Where the corner sits along the hinge line: the lift is strongest here and tapers away from it. */
		float Anchor = 0.0f;
		/** The taper along the hinge: full lift to Taper0 from the anchor, none by Taper1 (cm). */
		float Taper0 = 4.5f;
		float Taper1 = 9.5f;
		/** How fast the hinge falls away from the anchor (cm of depth per cm squared along it). */
		float Cup = 0.03f;
	};
	FFlap FlapToward(const FVector& Toward) const;
	FFlap FlapShared(const FPeekFlapWorld& Flap, const FVector& Toward) const;
	/** A point of the card (local, cm; Height above the mid-plane toward the ceiling) bent by Lift, as the material does. */
	FVector BendLocal(const FVector& Local, const FFlap& Flap, float Lift, float Radius, float Height) const;
	static float RadiusFor(float MaxLift);

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
	float PeekMax = 1.05f;
	float PeekRadius = 1.6f;
	FFlap Flap;
};
