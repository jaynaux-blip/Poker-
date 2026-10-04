#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"

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
	/** Where the head looks, relative to the body's facing (degrees: yaw + left, pitch + up). */
	float LookYaw = 0.0f;
	float LookPitch = 0.0f;
	/** Holding something to eat or drink up to the mouth (0..1), with the right hand. */
	float Sip = 0.0f;
	/** Shoulders down and a slower step when tired (0..1). */
	float Tired = 0.0f;
	float Time = 0.0f;
};

struct FStreetBodyProxy : public FAnimInstanceProxy
{
	FStreetBodyProxy() = default;
	explicit FStreetBodyProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}

	FStreetBodyPose Pose;

	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate(FPoseContext& Output) override;
};

/**
 * Walking, running and standing, posed procedurally from FStreetBodyPose: no animation assets, so any
 * MetaHuman (or the archetype body) walks the street the moment it's built. The gait swings the legs
 * from the hips with the knees bending through the swing, bobs and twists the pelvis, counter-swings
 * the arms and settles the head. The console variables ss.Walk.* tune it live on the desktop.
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
