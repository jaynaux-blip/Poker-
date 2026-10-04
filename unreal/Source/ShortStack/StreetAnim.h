#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "BonePose.h"

#include "StreetAnim.generated.h"

/**
 * One frame of a standing or walking body, set on the game thread by AShortStackCharacter and turned
 * into a pose by UStreetBodyAnim. Component space as the Back Room's bodies: MetaHuman bodies face +Y
 * with their left side at +X, the origin on the floor under the pelvis.
 */
USTRUCT()
struct FStreetBodyPose
{
	GENERATED_BODY()

	/** Ground speed (cm/s) and the gait cycle (radians: the left foot strikes at 0, the right at pi). */
	float Speed = 0.0f;
	float Phase = 0.0f;
	/** 0 walking .. 1 running: longer strides, bent elbows, a forward lean. */
	float Run = 0.0f;
	/** Degrees the body leans into a turn (+ to its left). */
	float Bank = 0.0f;
	/** Speeding up (+) or slowing down (-), -1..1: the trunk leans into a start and rocks back on a stop. */
	float Surge = 0.0f;
	/** Where the head looks, relative to the body's facing (degrees: yaw + to the right, pitch + up). */
	float LookYaw = 0.0f;
	float LookPitch = 0.0f;
	/** Holding something to eat or drink up to the mouth (0..1), with the right hand. */
	float Sip = 0.0f;
	/** The mouth, from the head bone in the reference pose (component space): where Sip brings the hand. */
	FVector Mouth = FVector(0.0, 10.5, 1.0);
	/** Shoulders down and a slower step when tired (0..1). */
	float Tired = 0.0f;
	float Time = 0.0f;
};

struct FStreetBodyProxy : public FAnimInstanceProxy
{
	FStreetBodyProxy() = default;
	explicit FStreetBodyProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}

	FStreetBodyPose Pose;

	/** The bones the gait moves, as compact pose indices (INDEX_NONE when this body or LOD lacks one); per side, 0 left. */
	struct FBones
	{
		int32 Pelvis = INDEX_NONE;
		int32 Spine1 = INDEX_NONE;
		int32 Spine3 = INDEX_NONE;
		int32 Spine5 = INDEX_NONE;
		int32 Neck = INDEX_NONE;
		int32 Head = INDEX_NONE;
		int32 Thigh[2] = {INDEX_NONE, INDEX_NONE};
		int32 Calf[2] = {INDEX_NONE, INDEX_NONE};
		int32 Foot[2] = {INDEX_NONE, INDEX_NONE};
		int32 Clavicle[2] = {INDEX_NONE, INDEX_NONE};
		int32 UpperArm[2] = {INDEX_NONE, INDEX_NONE};
		int32 LowerArm[2] = {INDEX_NONE, INDEX_NONE};
		int32 Hand[2] = {INDEX_NONE, INDEX_NONE};
		/** index, middle, ring, pinky; three segments each. */
		int32 Fingers[2][4][3];
		int32 Thumb[2][3];
	};
	FBones Bones;
	/** The bone container the indices above were found in (they're found again when it changes: another LOD, another mesh). */
	int32 BoneSerial = -1;
	/** Scratch kept from frame to frame, so posing doesn't allocate. */
	TArray<FTransform> CS;
	TArray<int32> Parent;
	TArray<uint8> Mark;

	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate(FPoseContext& Output) override;

private:
	void FindBones(FCompactPose& InPose);
};

/**
 * Walking, running and standing, posed procedurally from FStreetBodyPose: no animation assets, so any
 * MetaHuman (or the archetype body) walks the street the moment it's built. The gait swings the legs
 * from the hips with the knees bending through the swing, bobs and twists the pelvis, counter-swings
 * the arms with the hands loosely curled, turns the head toward the look and reaches the right hand
 * to the mouth for a sip or a bite. The console variables ss.Walk.* tune it live on the desktop.
 */
UCLASS(Transient, NotBlueprintable)
class SHORTSTACK_API UStreetBodyAnim : public UAnimInstance
{
	GENERATED_BODY()

public:
	FStreetBodyPose Pose;

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override { return new FStreetBodyProxy(this); }
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override { delete InProxy; }
};
