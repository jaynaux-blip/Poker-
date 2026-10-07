#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "BonePose.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "StreetAnim.generated.h"

class AShortStackCharacter;
class UAnimSequence;
class USkeleton;

/**
 * One frame of a standing or walking body, set on the game thread by AShortStackCharacter (and over it by the crowd: the
 * rain's stumble and hand) and turned into a pose by UStreetBodyAnim. Component space as the Back Room's bodies: MetaHuman
 * bodies face +Y with their left side at +X, the origin on the floor under the pelvis.
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
	/** Knocked off balance (spec 6.5's bump at a run): 1 the moment it happens, the crowd eases it out over a second. */
	float Stumble = 0.0f;
	/** The left hand up over the brow against the rain (0..1), for someone caught out without an umbrella. */
	float HandOverHead = 0.0f;
};

/** The baked Game Animation Sample clips the street's bodies use (spec 6.6), on the MetaHuman body skeleton (anim_setup.py). */
enum class EStreetClip : uint8
{
	StandIdle,
	IdleBreak1,
	IdleBreak2,
	IdleBreak3,
	IdleBreak4,
	IdleBreak5,
	IdleBreak6,
	Turn045L,
	Turn045R,
	Turn090L,
	Turn090R,
	Turn135L,
	Turn135R,
	Turn180L,
	Turn180R,
	WalkLoop,
	WalkStartL,
	WalkStartR,
	WalkStopL,
	WalkStopR,
	RunLoop,
	RunStartL,
	RunStartR,
	RunStopL,
	RunStopR,
	Count,
};

/**
 * The mocap library (spec 6.6): the Game Animation Sample's stand idle and its breaks, the turns in place, and the walk and
 * run loops with their starts and stops, retargeted onto the MetaHuman body skeleton by anim_setup.py (RTG_UEFN_to_Metahuman
 * _nrw for the standard bodies, _ovw for the heavy build) and baked to /Game/ShortStack/Anim/Mocap/AS_NS_<Clip>[_Ovw]. The
 * game loads only these (cook-safe: nothing references the sample's own folders), once per game instance, and the street's
 * saved preload list streams them with the level (AppendPreload). What isn't baked yet is simply missing: a body without a
 * stand idle and a walk loop for its skeleton walks on the procedural gait, as it always has.
 */
UCLASS()
class SHORTSTACK_API UStreetMocap : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** The set a clip comes from: the standard bodies' (nrw) or the heavy build's (ovw). */
	static constexpr int32 Standard = 0;
	static constexpr int32 Heavy = 1;
	static constexpr int32 NumSets = 2;

	/** What the game needs to know about a clip, measured once as it's loaded. */
	struct FClipInfo
	{
		float Length = 0.0f;
		/** Over the whole clip: how far the root turns (degrees, + right) and how far it travels (cm). */
		float RootYaw = 0.0f;
		float RootDistance = 0.0f;
		/** Its root's ground speed (cm/s; 0 when the root doesn't travel). */
		float Speed = 0.0f;
		/** Where its first left-foot and right-foot sync markers fall (seconds; negative: none). */
		float LeftFootAt = -1.0f;
		float RightFootAt = -1.0f;
	};

	/** The library of Context's game instance (null without one: the editor's preview poses procedurally). */
	static UStreetMocap* Get(const UObject* Context);

	/** A clip of a set (the heavy set falls back to the standard one clip by clip), null when it isn't baked. */
	UAnimSequence* Clip(int32 Set, EStreetClip C) const;
	const FClipInfo& Info(int32 Set, EStreetClip C) const;
	/** Whether a body on Skeleton can walk on the set: its stand idle and walk loop are baked, on that skeleton. */
	bool CanDrive(int32 Set, const USkeleton* Skeleton) const;
	/** Whether the turns in place carry their turn in the root (they can be played: the body turns with the clip). */
	bool TurnsUsable() const { return bTurnsUsable; }

	/** The baked clip's asset name ("AS_NS_WalkLoop", "AS_NS_WalkLoop_Ovw") and object path. */
	static FString ClipName(int32 Set, EStreetClip C);
	static FString ClipPath(int32 Set, EStreetClip C);
	/** Every clip's path (the street's preload list, FStreetPreload). */
	static void AppendPreload(TArray<FString>& Paths);
	/** One line: how many clips of each set are baked, the loops' speeds (the console's ss.Anim.Describe). */
	FString Describe() const;

private:
	/** Loads every clip once (the const accessors call it: loading is the library's own business). */
	void EnsureLoaded() const;
	void Load();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UAnimSequence>> Clips;
	TArray<FClipInfo> Infos;
	bool bLoaded = false;
	bool bTurnsUsable = false;
};

/** What the proxy samples this frame (the locomotion's clips and their weights), set on the game thread. */
struct FStreetMotion
{
	struct FLayer
	{
		const UAnimSequence* Seq = nullptr;
		float Time = 0.0f;
		float Weight = 0.0f;
	};
	FLayer Layers[4];
	int32 NumLayers = 0;
	/** The base pose comes from the clips (else the procedural gait from the reference pose). */
	bool bMocap = false;
	/** The stand idle, which the proxy measures the clips' standing pelvis from (each body's own pelvis height is scaled to it). */
	const UAnimSequence* PelvisRef = nullptr;
	/** How far the body is turned from the actor while it catches up with a turn on the spot (degrees, + right). */
	float RootYaw = 0.0f;
	/** The work beat (spec 6.6): its name, seconds into it, the station's frame in component space (+X the way they face). */
	FName Beat;
	float BeatTime = 0.0f;
	FTransform Station = FTransform::Identity;
	/** Seated (spec 6.6): the blend through the 0.6 s crouch (0 standing .. 1 down) and the seat's height (component cm). */
	float SeatBlend = 0.0f;
	float SeatHeight = 46.0f;
	/** Running on the spot (the treadmill): the gait's phase and how much of a run (the procedural gait's, when no clip). */
	float InPlace = 0.0f;
	float InPlacePhase = 0.0f;
	/** Held still for the barber (the head's idle drift and the look damped). */
	float HoldHead = 0.0f;
};

struct FStreetBodyProxy : public FAnimInstanceProxy
{
	FStreetBodyProxy() = default;
	explicit FStreetBodyProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}

	FStreetBodyPose Pose;
	FStreetMotion Motion;

	/** The bones the gait moves, as compact pose indices (INDEX_NONE when this body or LOD lacks one); per side, 0 left. */
	struct FBones
	{
		int32 Root = INDEX_NONE;
		int32 Pelvis = INDEX_NONE;
		int32 Spine1 = INDEX_NONE;
		int32 Spine3 = INDEX_NONE;
		int32 Spine5 = INDEX_NONE;
		int32 Neck = INDEX_NONE;
		int32 Head = INDEX_NONE;
		int32 Thigh[2] = {INDEX_NONE, INDEX_NONE};
		int32 Calf[2] = {INDEX_NONE, INDEX_NONE};
		int32 Foot[2] = {INDEX_NONE, INDEX_NONE};
		int32 Ball[2] = {INDEX_NONE, INDEX_NONE};
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
	/** The clips' standing pelvis (measured once per bone container from the stand idle), for scaling the clips' pelvis to this body. */
	float ClipPelvisHeight = 0.0f;
	const UAnimSequence* ClipPelvisFrom = nullptr;
	/** Scratch kept from frame to frame, so posing doesn't allocate. */
	TArray<FTransform> CS;
	TArray<int32> Parent;
	TArray<uint8> Mark;

	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate(FPoseContext& Output) override;

private:
	void FindBones(FCompactPose& InPose);
	/** The clips, blended, into Output (local space), with this body's own bone lengths. False when nothing could be sampled. */
	bool SampleClips(FPoseContext& Output);
};

/**
 * Walking, running, standing, sitting and working: the street's bodies (the hero, the staff, the pedestrians).
 *
 * Three sources feed it (spec 6.6). The Game Animation Sample's mocap (UStreetMocap) drives the locomotion when it's baked
 * for the body's skeleton: the stand idle with a random break every 8-20 s, turns in place (past ~50 degrees of the actor
 * turning on the spot, the nearest of the 45/90/135/180 clips, the body held where it faced until the clip turns it), and
 * the walk and run loops with their starts and stops, the loop's rate matched to the ground speed and blended toward the
 * run as it rises. The procedural gait (ss.Anim.Mocap 0, or no clips) is the fallback, as it always was. Over either: the
 * rain's hunch, the stumble and the hand over the head, the head and eyes from the attention stack (the character's look),
 * a sip, the work beats (spec 6.6's clerk, barber and waitress, and the hero's service beats; small loops of keyed hand
 * targets in the station's frame, on Reach and CurlChain) and the seated mode (the Back Room's seated solve, through a
 * 0.6 s crouch). The character's own state drives the last two (AShortStackCharacter::SetWork, SetSeated): nothing else
 * needs to know about them.
 *
 * Beats: "ring", "phone", "wipe", "attend" (the clerk); "cut" (a cycle of the next three), "comb", "clippers", "brush" (the
 * barber at the chair's head mark); "pour", "plate", "pad", "lean" and "wipe" (the waitress); "chair", "eat", "coffee",
 * "treadmill", "bag", "weights" (the hero's services; the gym stage's "run", "lift" and "train" are the treadmill and the
 * weights, seated on the bench for "lift"); "paper" and "phonesit" (Ruby's regulars: Sal behind the paper, Mei on her
 * phone); "phonestand" (someone waiting with a phone). Any other name leaves the hands to the idle (spec 6.6's P1 beats
 * until they're written).
 */
UCLASS(Transient, NotBlueprintable)
class SHORTSTACK_API UStreetBodyAnim : public UAnimInstance
{
	GENERATED_BODY()

public:
	FStreetBodyPose Pose;

	/** Whether the clips drive this body now (else the procedural gait). */
	bool IsOnMocap() const { return Motion.bMocap; }
	/** One line: the source, the state and its clip, the turn's offset, the beat, the seat (ss.Anim.Describe, ss.Npc.List). */
	FString DescribeMotion() const;
	/** The clip set this body takes (the heavy build's when its body is wide for its height). */
	int32 ClipSet() const { return Set; }

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override { return new FStreetBodyProxy(this); }
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override { delete InProxy; }
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	/** The locomotion's states: a node is one clip (or the walk and run loops, synced). */
	enum class EMove : uint8
	{
		Idle,
		Break,
		Turn,
		Start,
		Loop,
		Stop,
		InPlace,
	};
	struct FNode
	{
		EStreetClip Clip = EStreetClip::StandIdle;
		float Time = 0.0f;
		/** Looping (the idle, the loops); a loop node's run share (0 the walk loop alone .. 1 the run loop). */
		bool bLoop = true;
		bool bLocoLoop = false;
		float RunShare = 0.0f;
		float Rate = 1.0f;
	};

	void UpdateLocomotion(float Dt, const AShortStackCharacter* Owner);
	void Go(EMove To, EStreetClip Clip, float InFade, bool bLoop);
	void Advance(FNode& N, float Dt) const;
	void Emit(const FNode& N, float Weight);
	float ClipLength(EStreetClip C) const;
	bool HasClip(EStreetClip C) const;
	/** The yaw a turn clip's root has turned by Time (degrees), or the share of its nominal turn with no root rotation. */
	float TurnYawAt(EStreetClip C, float Time) const;
	void ChooseSet(const AShortStackCharacter* Owner);

	FStreetMotion Motion;
	TWeakObjectPtr<UStreetMocap> Library;
	TWeakObjectPtr<const USkeletalMesh> SetFor;
	int32 Set = UStreetMocap::Standard;
	bool bCanDrive = false;

	EMove State = EMove::Idle;
	FNode Cur;
	FNode Prev;
	float Fade = 1.0f;
	float FadeLen = 0.25f;
	float StateTime = 0.0f;
	float NextBreak = 10.0f;
	int32 LastBreak = -1;
	bool bLeftNext = true;
	/** The loops' shared cycle (0..1), so the walk and run loops step together and a stop picks the right foot. */
	float LoopPhase = 0.0f;
	float LastActorYaw = 0.0f;
	bool bYawKnown = false;
	float TurnStartOffset = 0.0f;
	float TurnLastYaw = 0.0f;
	float StillTurning = 0.0f;
	float LastSpeed = 0.0f;
	float Clock = 0.0f;
	FRandomStream Dice;
};
