#pragma once

#include "CoreMinimal.h"

class UPoseableMeshComponent;

/**
 * The player's arms at the desk (SK_Arms, modeled in art/blender/assets/arms.py). The left hand rests on
 * the laptop's palm rest and plays the hotkeys with the finger a touch typist would use. The right hand
 * holds the mouse, follows the cursor and clicks.
 *
 * Two-bone IK places each wrist, the hand turns to a target frame and the fingers curl about their own
 * joints. Every rotation is a change from the mesh's reference pose, so this works whatever axes the
 * importer gave the bones.
 */
class FFirstPersonArms
{
public:
	/** Where the hands go this frame (world space). Forward points from the player toward the desk. */
	struct FTargets
	{
		FVector LeftRest = FVector::ZeroVector; // the left wrist at rest, over the palm rest
		FVector Mouse = FVector::ZeroVector;    // the top of the mouse's hump
		FVector Forward = FVector(1.0, 0.0, 0.0);
		FVector Up = FVector(0.0, 0.0, 1.0);
	};

	/** Reads the reference pose. Call once the component has its mesh and is registered. */
	bool Init(UPoseableMeshComponent* InMesh);
	bool IsReady() const { return bReady; }

	/** A hotkey press: the hand that owns the key reaches over and taps it with the right finger. */
	void Key(const FString& Name, const FVector& KeyWorld);
	/** A mouse click: the right index finger presses. */
	void Click();
	void Update(float Dt, const FTargets& Targets);

private:
	enum EFinger
	{
		Thumb,
		Index,
		Middle,
		Ring,
		Pinky,
		FingerCount
	};

	struct FFinger
	{
		FName Bones[3];
		FTransform RefCS[3];
		FVector AxisCS = FVector::ZeroVector; // positive rotation about it curls toward the palm
		float TipLength = 2.0f;               // centimeters from the last joint to the fingertip
	};

	struct FSide
	{
		FName Upper;
		FName Lower;
		FName Hand;
		FTransform UpperCS;
		FTransform LowerCS;
		FTransform HandCS;
		FVector HandFwdCS = FVector::ZeroVector; // wrist to middle knuckle
		FVector HandUpCS = FVector::ZeroVector;  // back of the hand
		FFinger Fingers[FingerCount];
		float Rest[FingerCount][3] = {};         // resting curls (degrees)
		float Sign = 1.0f;                       // +1 right, -1 left

		// Motion.
		FVector Wrist = FVector::ZeroVector;
		bool bHasWrist = false;
		FVector TipLocal[FingerCount];           // fingertip offsets from the wrist, in hand space (last frame)
		FVector KeyWorld = FVector::ZeroVector;
		int32 TapFinger = -1;
		float TapTime = 10.0f;                   // seconds since the tap began
		float ClickTime = 10.0f;
	};

	bool InitSide(FSide& S, const TCHAR* Suffix, float Sign);
	void Solve(FSide& S, const FVector& WristTarget, const FVector& HandFwd, const FVector& HandUp, const FVector& Up, float Dt,
		const float Extra[FingerCount][3]);
	FTransform RefWorld(const FTransform& RefCS) const;

	UPoseableMeshComponent* Mesh = nullptr;
	FSide Left;
	FSide Right;
	float Clock = 0.0f;
	bool bReady = false;
};
