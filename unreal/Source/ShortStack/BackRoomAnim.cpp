#include "BackRoomAnim.h"

#include "Animation/AnimNodeBase.h"
#include "BonePose.h"
#include "Components/SkeletalMeshComponent.h"
#include "TwoBoneIK.h"

namespace BackRoomAnimDetail
{
/**
 * A pose held in component space: whole subtrees are turned, moved or re-seated, then the result is
 * written back as local transforms. Compact pose indices put every parent before its children.
 */
struct FSolver
{
	FCompactPose& Pose;
	const FBoneContainer& Bones;
	TArray<FTransform> CS;
	TArray<int32> Parent;

	explicit FSolver(FCompactPose& InPose)
		: Pose(InPose)
		, Bones(InPose.GetBoneContainer())
	{
		const int32 N = Pose.GetNumBones();
		CS.SetNum(N);
		Parent.SetNum(N);
		for (FCompactPoseBoneIndex I : Pose.ForEachBoneIndex())
		{
			const FCompactPoseBoneIndex P = Bones.GetParentBoneIndex(I);
			Parent[I.GetInt()] = P.IsValid() ? P.GetInt() : INDEX_NONE;
			CS[I.GetInt()] = P.IsValid() ? Pose[I] * CS[P.GetInt()] : Pose[I];
		}
	}

	int32 Find(const TCHAR* Name) const
	{
		const int32 MeshIndex = Bones.GetPoseBoneIndexForBoneName(FName(Name));
		if (MeshIndex == INDEX_NONE)
		{
			return INDEX_NONE;
		}
		const FCompactPoseBoneIndex C = Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshIndex));
		return C.IsValid() ? C.GetInt() : INDEX_NONE;
	}

	FVector Pos(int32 B) const { return B == INDEX_NONE ? FVector::ZeroVector : CS[B].GetTranslation(); }

	/** Calls Fn(i) for B and every bone under it. */
	template <typename FnT>
	void ForSubtree(int32 B, FnT&& Fn)
	{
		TBitArray<> In(false, CS.Num());
		In[B] = true;
		Fn(B);
		for (int32 I = B + 1; I < CS.Num(); ++I)
		{
			const int32 P = Parent[I];
			if (P != INDEX_NONE && In[P])
			{
				In[I] = true;
				Fn(I);
			}
		}
	}

	/** Turns bone B and everything under it about B's position. */
	void Rotate(int32 B, const FQuat& Delta)
	{
		if (B == INDEX_NONE)
		{
			return;
		}
		const FVector Pivot = CS[B].GetTranslation();
		ForSubtree(B, [&](int32 I) {
			FTransform& T = CS[I];
			T.SetRotation((Delta * T.GetRotation()).GetNormalized());
			T.SetTranslation(Pivot + Delta.RotateVector(T.GetTranslation() - Pivot));
		});
	}

	void Move(int32 B, const FVector& Delta)
	{
		if (B == INDEX_NONE)
		{
			return;
		}
		ForSubtree(B, [&](int32 I) { CS[I].AddToTranslation(Delta); });
	}

	/** Puts bone B at NewCS, carrying everything under it rigidly. */
	void Reseat(int32 B, const FTransform& NewCS)
	{
		if (B == INDEX_NONE)
		{
			return;
		}
		const FTransform Old = CS[B];
		ForSubtree(B, [&](int32 I) { CS[I] = CS[I].GetRelativeTransform(Old) * NewCS; });
	}

	/** Turns B so that the direction to C points along Dir (Weight 0..1). */
	void Aim(int32 B, int32 C, const FVector& Dir, float Weight = 1.0f)
	{
		if (B == INDEX_NONE || C == INDEX_NONE || Weight <= 0.0f)
		{
			return;
		}
		const FVector Cur = (Pos(C) - Pos(B)).GetSafeNormal();
		const FVector Want = Dir.GetSafeNormal();
		if (Cur.IsNearlyZero() || Want.IsNearlyZero())
		{
			return;
		}
		FQuat Q = FQuat::FindBetweenNormals(Cur, Want);
		if (Weight < 1.0f)
		{
			Q = FQuat::Slerp(FQuat::Identity, Q, Weight);
		}
		Rotate(B, Q);
	}

	void WriteBack()
	{
		for (FCompactPoseBoneIndex I : Pose.ForEachBoneIndex())
		{
			const int32 P = Parent[I.GetInt()];
			FTransform L = P == INDEX_NONE ? CS[I.GetInt()] : CS[I.GetInt()].GetRelativeTransform(CS[P]);
			L.NormalizeRotation();
			Pose[I] = L;
		}
	}
};

float Rad(float Deg)
{
	return FMath::DegreesToRadians(Deg);
}

/** Smooth, never-repeating wobble in -1..1 (sum of incommensurate sines). */
float Wobble(float T, float Seed)
{
	return 0.55f * FMath::Sin(T * 1.0f + Seed) + 0.3f * FMath::Sin(T * 2.37f + Seed * 1.7f) + 0.15f * FMath::Sin(T * 5.13f + Seed * 3.1f);
}

const TCHAR* Fingers[4] = {TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky")};

int32 FindSided(const FSolver& S, const TCHAR* Base, int32 Side, const TCHAR* Segment = nullptr)
{
	const FString Name = Segment ? FString::Printf(TEXT("%s_%s_%s"), Base, Segment, Side == 0 ? TEXT("l") : TEXT("r"))
	                             : FString::Printf(TEXT("%s_%s"), Base, Side == 0 ? TEXT("l") : TEXT("r"));
	return S.Find(*Name);
}

/** The palm's normal for the hand as posed (fingers F, across the knuckles from pinky to index). */
FVector PalmNormal(const FSolver& S, int32 Side)
{
	const int32 Hand = FindSided(S, TEXT("hand"), Side);
	const FVector F = (S.Pos(FindSided(S, TEXT("middle"), Side, TEXT("01"))) - S.Pos(Hand)).GetSafeNormal();
	const FVector Across = (S.Pos(FindSided(S, TEXT("index"), Side, TEXT("01"))) - S.Pos(FindSided(S, TEXT("pinky"), Side, TEXT("01")))).GetSafeNormal();
	const FVector N = FVector::CrossProduct(F, Across).GetSafeNormal();
	// The left hand's cross product points out of its back; the right hand's out of its palm.
	return Side == 0 ? -N : N;
}

/** Bends a chain of finger segments toward the palm; Angles in degrees per segment. */
void CurlChain(FSolver& S, const int32* Chain, int32 Count, const FVector& Palm, const float* Angles)
{
	for (int32 K = 0; K < Count; ++K)
	{
		const int32 B = Chain[K];
		if (B == INDEX_NONE || FMath::IsNearlyZero(Angles[K]))
		{
			continue;
		}
		// Segment direction: toward the next joint, or the last segment's own direction at the tip.
		const int32 Next = K + 1 < Count ? Chain[K + 1] : INDEX_NONE;
		const FVector Dir = Next != INDEX_NONE ? (S.Pos(Next) - S.Pos(B)).GetSafeNormal()
		                                       : (K > 0 ? (S.Pos(B) - S.Pos(Chain[K - 1])).GetSafeNormal() : FVector::ZeroVector);
		const FVector Axis = FVector::CrossProduct(Dir, Palm).GetSafeNormal();
		if (!Axis.IsNearlyZero())
		{
			S.Rotate(B, FQuat(Axis, Rad(Angles[K])));
		}
	}
}
} // namespace BackRoomAnimDetail

using namespace BackRoomAnimDetail;

// ------------------------------------------------------------------ body

void FBackRoomBodyProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
	if (const UBackRoomBodyAnim* Anim = Cast<UBackRoomBodyAnim>(InAnimInstance))
	{
		Pose = Anim->Pose;
	}
}

bool FBackRoomBodyProxy::Evaluate(FPoseContext& Output)
{
	Output.ResetToRefPose();
	FSolver S(Output.Pose);
	const TArray<FTransform> Ref = S.CS;
	const FBackRoomBodyPose& P = Pose;

	const int32 Pelvis = S.Find(TEXT("pelvis"));
	if (Pelvis == INDEX_NONE)
	{
		return true;
	}
	const int32 Spine[5] = {S.Find(TEXT("spine_01")), S.Find(TEXT("spine_02")), S.Find(TEXT("spine_03")), S.Find(TEXT("spine_04")), S.Find(TEXT("spine_05"))};
	const int32 Neck1 = S.Find(TEXT("neck_01")), Neck2 = S.Find(TEXT("neck_02")), Head = S.Find(TEXT("head"));

	// Sit: the pelvis drops onto the seat and rolls back a little as the back slumps.
	S.Move(Pelvis, FVector(0.0, -3.0, P.SeatHeight + 9.0) - S.Pos(Pelvis));
	S.Rotate(Pelvis, FQuat(FVector::XAxisVector, Rad(7.0f + 6.0f * P.Slouch)));

	// Legs: thighs along the seat (sloping down a touch so the feet reach the floor), shins down,
	// feet flat, knees apart as far as the player sprawls.
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const float Sx = Side == 0 ? 1.0f : -1.0f;
		const int32 Thigh = FindSided(S, TEXT("thigh"), Side), Calf = FindSided(S, TEXT("calf"), Side);
		const int32 Foot = FindSided(S, TEXT("foot"), Side), Ball = FindSided(S, TEXT("ball"), Side);
		S.Aim(Thigh, Calf, FVector(Sx * (0.08f + 0.28f * P.KneeSpread), 1.0f, -0.24f));
		S.Aim(Calf, Foot, FVector(Sx * 0.04f, 0.12f, -1.0f));
		S.Aim(Foot, Ball, FVector(Sx * 0.14f, 0.87f, -0.48f));
	}

	// Spine: lean in toward the table (+Y), twist, and breathe.
	const float LeanDeg = 4.0f + 30.0f * P.Lean + 8.0f * P.Slouch;
	const float LeanShare[5] = {0.14f, 0.18f, 0.22f, 0.24f, 0.22f};
	for (int32 K = 0; K < 5; ++K)
	{
		float Deg = LeanDeg * LeanShare[K];
		// The inhale lifts and opens the chest: the upper spine straightens a little.
		if (K >= 3)
		{
			Deg -= 1.4f * P.Breath;
		}
		S.Rotate(Spine[K], FQuat(FVector::ZAxisVector, Rad(P.Twist * LeanShare[K])) * FQuat(FVector::XAxisVector, Rad(-Deg)));
	}

	// Shoulders: raised by tension and each inhale, rounded forward by a slouch.
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const int32 Clav = FindSided(S, TEXT("clavicle"), Side), Upper = FindSided(S, TEXT("upperarm"), Side);
		const FVector Dir = (S.Pos(Upper) - S.Pos(Clav)).GetSafeNormal();
		S.Aim(Clav, Upper, Dir + FVector(0.0, 0.08f * P.Slouch, 0.22f * P.ShoulderRaise + 0.03f * P.Breath));
	}

	// Arms: two-bone IK to the hand targets, elbows out, back and down.
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const float Sx = Side == 0 ? 1.0f : -1.0f;
		const float W = FMath::Clamp(P.HandWeight[Side], 0.0f, 1.0f);
		const int32 Upper = FindSided(S, TEXT("upperarm"), Side), Lower = FindSided(S, TEXT("lowerarm"), Side), Hand = FindSided(S, TEXT("hand"), Side);
		if (Upper == INDEX_NONE || Lower == INDEX_NONE || Hand == INDEX_NONE || W <= 0.0f)
		{
			continue;
		}
		FVector Target = P.HandPos[Side];
		if (P.Tremble > 0.0f)
		{
			const float T = P.Time * 9.0f;
			Target += P.Tremble * FVector(Wobble(T, 1.3f + Side), Wobble(T * 1.1f, 4.1f + Side), Wobble(T * 0.9f, 7.7f + Side));
		}
		// Elbows out to the sides and a little forward: forearms come to rest on the rail's padding.
		const FVector Pole = S.Pos(Upper) + FVector(Sx * 45.0, 12.0, -22.0);
		FVector Joint, End;
		AnimationCore::SolveTwoBoneIK(S.Pos(Upper), S.Pos(Lower), S.Pos(Hand), Pole, Target, Joint, End, false, 1.0, 1.0);
		S.Aim(Upper, Lower, Joint - S.Pos(Upper), W);
		S.Aim(Lower, Hand, End - S.Pos(Lower), W);

		// The hand: fingers along FingerDir, palm toward PalmDir.
		const int32 Mid = FindSided(S, TEXT("middle"), Side, TEXT("01"));
		const FVector F = (S.Pos(Mid) - S.Pos(Hand)).GetSafeNormal();
		const FVector Palm = PalmNormal(S, Side);
		const FQuat Cur = FRotationMatrix::MakeFromXZ(F, Palm).ToQuat();
		const FQuat Want = FRotationMatrix::MakeFromXZ(P.FingerDir[Side].GetSafeNormal(), P.PalmDir[Side].GetSafeNormal()).ToQuat();
		S.Rotate(Hand, FQuat::Slerp(FQuat::Identity, Want * Cur.Inverse(), W));

		// Fingers curl toward the palm; the index closes further when pinching.
		const FVector PalmNow = PalmNormal(S, Side);
		for (int32 Fi = 0; Fi < 4; ++Fi)
		{
			const int32 Chain[3] = {FindSided(S, Fingers[Fi], Side, TEXT("01")), FindSided(S, Fingers[Fi], Side, TEXT("02")), FindSided(S, Fingers[Fi], Side, TEXT("03"))};
			float C = P.Curl[Side];
			if (Fi == 0)
			{
				C = FMath::Max(C, 0.55f * P.Pinch[Side]);
			}
			// Outer fingers curl a little more than the index, as a resting hand does.
			C = FMath::Clamp(C * (1.0f + 0.12f * Fi), 0.0f, 1.0f);
			const float Angles[3] = {70.0f * C, 92.0f * C, 55.0f * C};
			CurlChain(S, Chain, 3, PalmNow, Angles);
		}
		const int32 Thumb[3] = {FindSided(S, TEXT("thumb"), Side, TEXT("01")), FindSided(S, TEXT("thumb"), Side, TEXT("02")), FindSided(S, TEXT("thumb"), Side, TEXT("03"))};
		const float TC = FMath::Clamp(P.ThumbCurl[Side] + 0.5f * P.Pinch[Side], 0.0f, 1.0f);
		const float ThumbAngles[3] = {18.0f * TC, 35.0f * TC, 45.0f * TC};
		CurlChain(S, Thumb, 3, PalmNow, ThumbAngles);
	}

	// Head: turn toward the look target, shared down the neck, within what a neck can do.
	if (Head != INDEX_NONE)
	{
		const FQuat Rel = S.CS[Head].GetRotation() * Ref[Head].GetRotation().Inverse();
		const FVector Fwd = Rel.RotateVector(FVector::YAxisVector);
		FVector Want = (P.LookAt - S.Pos(Head)).GetSafeNormal();
		// Limit to +/-75 degrees of yaw and -45..+35 of pitch from straight ahead.
		const float Yaw = FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan2(Want.X, Want.Y)), -75.0f, 75.0f);
		const float Pitch = FMath::Clamp(FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(Want.Z, -1.0, 1.0))), -45.0f, 35.0f);
		Want = FVector(FMath::Sin(Rad(Yaw)) * FMath::Cos(Rad(Pitch)), FMath::Cos(Rad(Yaw)) * FMath::Cos(Rad(Pitch)), FMath::Sin(Rad(Pitch)));
		FQuat D = FQuat::FindBetweenNormals(Fwd, Want);
		D = FQuat::Slerp(FQuat::Identity, D, FMath::Clamp(P.HeadFollow, 0.0f, 1.0f));
		S.Rotate(Neck1, FQuat::Slerp(FQuat::Identity, D, 0.3f));
		S.Rotate(Neck2, FQuat::Slerp(FQuat::Identity, D, 0.3f));
		S.Rotate(Head, FQuat::Slerp(FQuat::Identity, D, 0.4f));
		// Nod and tilt about the head's own axes.
		const FQuat HeadRel = S.CS[Head].GetRotation() * Ref[Head].GetRotation().Inverse();
		const FVector HeadFwd = HeadRel.RotateVector(FVector::YAxisVector);
		const FVector HeadRight = HeadRel.RotateVector(-FVector::XAxisVector);
		S.Rotate(Head, FQuat(HeadFwd, Rad(P.HeadTilt)) * FQuat(HeadRight, Rad(P.HeadNod)));
	}

	S.WriteBack();
	return true;
}

// ------------------------------------------------------------------ face

namespace BackRoomAnimDetail
{
// The bones the face shares with the body, root to head (parents first).
const TCHAR* SharedBones[] = {TEXT("root"), TEXT("pelvis"), TEXT("spine_01"), TEXT("spine_02"), TEXT("spine_03"), TEXT("spine_04"), TEXT("spine_05"),
                              TEXT("neck_01"), TEXT("neck_02"), TEXT("head")};
} // namespace BackRoomAnimDetail

void FBackRoomFaceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
	const UBackRoomFaceAnim* Anim = Cast<UBackRoomFaceAnim>(InAnimInstance);
	if (!Anim)
	{
		return;
	}
	Curves = Anim->Curves;
	LookAt = Anim->LookAt;
	bHasLook = !LookAt.IsZero();
	BodyBones.Reset();
	// The body ticks first (a tick prerequisite), so its pose for this frame is ready.
	if (const USkeletalMeshComponent* Body = Anim->Body.Get())
	{
		const TArray<FTransform>& Transforms = Body->GetComponentSpaceTransforms();
		for (const TCHAR* Name : SharedBones)
		{
			const int32 Index = Body->GetBoneIndex(FName(Name));
			if (Transforms.IsValidIndex(Index))
			{
				BodyBones.Emplace(FName(Name), Transforms[Index]);
			}
		}
	}
}

void FBackRoomFaceProxy::Initialize(UAnimInstance* InAnimInstance)
{
	FAnimInstanceProxy::Initialize(InAnimInstance);
	Source.Fn = [this](FPoseContext& Output) { EvaluateSource(Output); };
	RigLogic.AnimSequence.SetLinkNode(&Source);
	FAnimationInitializeContext Context(this);
	RigLogic.Initialize_AnyThread(Context);
	CachedBoneSerial = -1;
}

void FBackRoomFaceProxy::Update(float DeltaSeconds)
{
	FAnimationUpdateContext Context(this, DeltaSeconds);
	RigLogic.Update_AnyThread(Context);
}

bool FBackRoomFaceProxy::Evaluate(FPoseContext& Output)
{
	// RigLogic maps DNA joints to this LOD's compact pose: remap whenever the required bones change.
	const FBoneContainer& Required = GetRequiredBones();
	if (Required.IsValid() && (Required.GetSerialNumber() != CachedBoneSerial || GetLODLevel() != CachedLOD))
	{
		FAnimationCacheBonesContext Context(this);
		RigLogic.CacheBones_AnyThread(Context);
		CachedBoneSerial = Required.GetSerialNumber();
		CachedLOD = GetLODLevel();
	}
	RigLogic.Evaluate_AnyThread(Output);
	return true;
}

void FBackRoomFaceProxy::EvaluateSource(FPoseContext& Output)
{
	Output.ResetToRefPose();
	FSolver S(Output.Pose);
	const TArray<FTransform> Ref = S.CS;
	for (const TPair<FName, FTransform>& Bone : BodyBones)
	{
		S.Reseat(S.Find(*Bone.Key.ToString()), Bone.Value);
	}
	S.WriteBack();

	for (const TPair<FName, float>& Curve : Curves)
	{
		Output.Curve.Set(Curve.Key, Curve.Value);
	}

	// Eyes: the look direction in the head's frame (as posed from its reference orientation), turned
	// into RigLogic's look controls. A little over 30 degrees saturates a control.
	const int32 Head = S.Find(TEXT("head"));
	const int32 EyeL = S.Find(TEXT("FACIAL_L_Eye")), EyeR = S.Find(TEXT("FACIAL_R_Eye"));
	if (bHasLook && Head != INDEX_NONE && EyeL != INDEX_NONE && EyeR != INDEX_NONE)
	{
		const FVector Center = (S.Pos(EyeL) + S.Pos(EyeR)) * 0.5;
		const FQuat Rel = S.CS[Head].GetRotation() * Ref[Head].GetRotation().Inverse();
		const FVector Local = Rel.UnrotateVector((LookAt - Center).GetSafeNormal());
		const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Local.X, Local.Y));      // + toward the character's left
		const float Pitch = FMath::RadiansToDegrees(FMath::Atan2(Local.Z, FVector2D(Local.X, Local.Y).Size()));
		const float Left = FMath::Clamp(Yaw / 32.0f, 0.0f, 1.0f), Right = FMath::Clamp(-Yaw / 32.0f, 0.0f, 1.0f);
		const float Up = FMath::Clamp(Pitch / 24.0f, 0.0f, 1.0f), Down = FMath::Clamp(-Pitch / 28.0f, 0.0f, 1.0f);
		for (const TCHAR* Eye : {TEXT("L"), TEXT("R")})
		{
			Output.Curve.Set(FName(*FString::Printf(TEXT("CTRL_expressions_eyeLookLeft%s"), Eye)), Left);
			Output.Curve.Set(FName(*FString::Printf(TEXT("CTRL_expressions_eyeLookRight%s"), Eye)), Right);
			Output.Curve.Set(FName(*FString::Printf(TEXT("CTRL_expressions_eyeLookUp%s"), Eye)), Up);
			Output.Curve.Set(FName(*FString::Printf(TEXT("CTRL_expressions_eyeLookDown%s"), Eye)), Down);
		}
	}
}
