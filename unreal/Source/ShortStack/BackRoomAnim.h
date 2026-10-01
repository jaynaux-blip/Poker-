#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNodeBase.h"
#include "AnimNode_RigLogic.h"

#include "BackRoomAnim.generated.h"

/**
 * One frame of a seated player's body, set on the game thread by ABackRoomPlayer and turned into a
 * pose by UBackRoomBodyAnim. Everything is in the body mesh's component space (cm): MetaHuman bodies
 * face +Y with their left side at +X, and the component origin is on the floor under the pelvis.
 * Index 0 is the left hand, 1 the right.
 */
USTRUCT()
struct FBackRoomBodyPose
{
	GENERATED_BODY()

	/** Height of the chair seat (cm): the pelvis rests about 9 cm above it. */
	float SeatHeight = 47.0f;
	/** 0 upright .. 1 hunched over the table. */
	float Lean = 0.3f;
	/** Rounded shoulders and a sunk chest, from tiredness or defeat. */
	float Slouch = 0.2f;
	/** Torso yaw (degrees, + turns the chest toward the player's left). */
	float Twist = 0.0f;
	/** -1..1: the breathing cycle (chest and shoulders rise on the inhale). */
	float Breath = 0.0f;
	/** 0..1: shoulders pulled up by tension. */
	float ShoulderRaise = 0.0f;
	/** Where the head and eyes look (component space). */
	FVector LookAt = FVector(0.0, 150.0, 110.0);
	/** Share of the look the head turns for (the eyes do the rest). */
	float HeadFollow = 0.6f;
	/** Head tilt (degrees, roll) and a nod offset (pitch). */
	float HeadTilt = 0.0f;
	float HeadNod = 0.0f;

	FVector HandPos[2] = {FVector(16.0, 38.0, 80.0), FVector(-16.0, 38.0, 80.0)};
	/** Direction the palm faces and the fingers point (unit vectors). */
	FVector PalmDir[2] = {FVector(0.0, 0.0, -1.0), FVector(0.0, 0.0, -1.0)};
	FVector FingerDir[2] = {FVector(-0.3, 1.0, 0.0), FVector(0.3, 1.0, 0.0)};
	/** How strongly each arm follows its hand target (0 hangs in the reference pose). */
	float HandWeight[2] = {1.0f, 1.0f};
	/** 0 flat .. 1 fist; Pinch closes the index and thumb (holding a chip or a card corner). */
	float Curl[2] = {0.35f, 0.35f};
	float ThumbCurl[2] = {0.2f, 0.2f};
	float Pinch[2] = {0.0f, 0.0f};
	/** Adrenaline tremor in the hands (cm). */
	float Tremble = 0.0f;
	/** Knees apart (0 together .. 1 wide). */
	float KneeSpread = 0.35f;
	float Time = 0.0f;
};

struct FBackRoomBodyProxy : public FAnimInstanceProxy
{
	FBackRoomBodyProxy() = default;
	explicit FBackRoomBodyProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}

	FBackRoomBodyPose Pose;

	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate(FPoseContext& Output) override;
};

/** A seated MetaHuman body posed procedurally: no animation assets, every joint from FBackRoomBodyPose. */
UCLASS(Transient, NotBlueprintable)
class SHORTSTACK_API UBackRoomBodyAnim : public UAnimInstance
{
	GENERATED_BODY()

public:
	FBackRoomBodyPose Pose;

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override { return new FBackRoomBodyProxy(this); }
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override { delete InProxy; }
};

/** Feeds RigLogic with the face's pose and expression controls (a native stand-in for an AnimBP's input). */
struct FBackRoomFaceSource : public FAnimNode_Base
{
	TFunction<void(FPoseContext&)> Fn;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override
	{
		if (Fn)
		{
			Fn(Output);
		}
		else
		{
			Output.ResetToRefPose();
		}
	}
};

struct FBackRoomFaceProxy : public FAnimInstanceProxy
{
	FBackRoomFaceProxy() = default;
	explicit FBackRoomFaceProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}

	/** RigLogic, hosted here: it turns the CTRL_expressions_* controls into the face's joints. */
	FAnimNode_RigLogic RigLogic;
	FBackRoomFaceSource Source;
	int32 CachedBoneSerial = -1;
	int32 CachedLOD = -1;

	/** RigLogic controls (CTRL_expressions_*) for this frame. */
	TMap<FName, float> Curves;
	/** The body's spine, neck and head (component space) this frame, by bone name. */
	TArray<TPair<FName, FTransform>> BodyBones;
	/** Where the eyes look (component space). */
	FVector LookAt = FVector::ZeroVector;
	bool bHasLook = false;

	virtual void Initialize(UAnimInstance* InAnimInstance) override;
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual void Update(float DeltaSeconds) override;
	virtual bool Evaluate(FPoseContext& Output) override;
	/** The face before RigLogic: following the body, with the controls as curves. */
	void EvaluateSource(FPoseContext& Output);
};

/**
 * A MetaHuman face driven by expression curves: it follows the body's neck and head, aims the eyes
 * and runs RigLogic on the CTRL_expressions_* controls (the DNA on the face mesh defines the rig).
 */
UCLASS(Transient, NotBlueprintable)
class SHORTSTACK_API UBackRoomFaceAnim : public UAnimInstance
{
	GENERATED_BODY()

public:
	TMap<FName, float> Curves;
	/** The body to follow (its component space must match the face's: attach the face at identity). */
	TWeakObjectPtr<USkeletalMeshComponent> Body;
	FVector LookAt = FVector::ZeroVector;

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override { return new FBackRoomFaceProxy(this); }
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override { delete InProxy; }
};
