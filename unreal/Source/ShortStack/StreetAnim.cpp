#include "StreetAnim.h"

#include "Animation/AnimNodeBase.h"
#include "BonePose.h"
#include "HAL/IConsoleManager.h"

namespace StreetAnimDetail
{
// Tuning from the console while walking around on the desktop (the gait was written without seeing it).
TAutoConsoleVariable<float> CVarWalkSign(TEXT("ss.Walk.Sign"), 1.0f, TEXT("Flips the swing axis if the legs swing backward (1 or -1)."));
TAutoConsoleVariable<float> CVarWalkStride(TEXT("ss.Walk.Stride"), 1.0f, TEXT("Scales the leg swing."));
TAutoConsoleVariable<float> CVarWalkArms(TEXT("ss.Walk.Arms"), 1.0f, TEXT("Scales the arm swing (0 hangs them still)."));
TAutoConsoleVariable<float> CVarWalkBob(TEXT("ss.Walk.Bob"), 1.0f, TEXT("Scales the pelvis bob and twist."));
TAutoConsoleVariable<float> CVarWalkCurl(TEXT("ss.Walk.Curl"), 1.0f, TEXT("Scales how far the hands curl (0 leaves them open)."));
// Where a sip or a bite brings the right wrist, from the mouth, as the face is turned. The knuckles of a gripping hand
// stand about 12 cm out from the wrist, so a wrist any closer than that puts the fingers through the lips.
TAutoConsoleVariable<float> CVarSipSide(TEXT("ss.Walk.SipSide"), 3.0f, TEXT("Sip: the right wrist this far to the right of the mouth (cm)."));
TAutoConsoleVariable<float> CVarSipAhead(TEXT("ss.Walk.SipAhead"), 8.0f, TEXT("Sip: the right wrist this far ahead of the mouth (cm)."));
TAutoConsoleVariable<float> CVarSipUp(TEXT("ss.Walk.SipUp"), -11.0f, TEXT("Sip: the right wrist this far above the mouth (cm, negative is below)."));

const TCHAR* const FingerBase[4] = {TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky")};

/** A pose held in component space (the Back Room's solver, cut down to what a gait needs), on the proxy's scratch. */
struct FGait
{
	FCompactPose& Pose;
	TArray<FTransform>& CS;
	const TArray<int32>& Parent;
	TArray<uint8>& Mark;

	FGait(FCompactPose& InPose, TArray<FTransform>& InCS, const TArray<int32>& InParent, TArray<uint8>& InMark)
		: Pose(InPose)
		, CS(InCS)
		, Parent(InParent)
		, Mark(InMark)
	{
		for (FCompactPoseBoneIndex I : Pose.ForEachBoneIndex())
		{
			const int32 B = I.GetInt();
			const int32 P = Parent[B];
			CS[B] = P == INDEX_NONE ? Pose[I] : Pose[I] * CS[P];
		}
	}

	FVector Pos(int32 B) const { return B == INDEX_NONE ? FVector::ZeroVector : CS[B].GetTranslation(); }

	template <typename FnT>
	void ForSubtree(int32 B, FnT&& Fn)
	{
		// Parents come before their children in a compact pose, so one pass from B marks its whole subtree.
		FMemory::Memzero(Mark.GetData(), Mark.Num());
		Mark[B] = 1;
		Fn(B);
		for (int32 I = B + 1; I < CS.Num(); ++I)
		{
			const int32 P = Parent[I];
			if (P != INDEX_NONE && Mark[P])
			{
				Mark[I] = 1;
				Fn(I);
			}
		}
	}

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
		if (B != INDEX_NONE)
		{
			ForSubtree(B, [&](int32 I) { CS[I].AddToTranslation(Delta); });
		}
	}

	/** Turns B so the direction from B to C points along Dir (Weight 0..1). */
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

/** Rotations about the body's axes (component space: X to its left, Y forward, Z up), in degrees. */
FQuat GaitPitch(float Deg)
{
	return FQuat(FVector(1.0, 0.0, 0.0), FMath::DegreesToRadians(Deg));
}

/** + turns the face from +Y toward -X: the body's right. */
FQuat GaitYaw(float Deg)
{
	return FQuat(FVector(0.0, 0.0, 1.0), FMath::DegreesToRadians(Deg));
}

FQuat GaitRoll(float Deg)
{
	return FQuat(FVector(0.0, 1.0, 0.0), FMath::DegreesToRadians(Deg));
}

/** The way a hand's palm faces, from its knuckles (the Back Room's rule: the cross product points out of the right palm). */
FVector PalmNormal(const FGait& G, const FStreetBodyProxy::FBones& B, int32 Side)
{
	const int32 Hand = B.Hand[Side];
	const int32 Middle = B.Fingers[Side][1][0];
	const int32 Index = B.Fingers[Side][0][0];
	const int32 Pinky = B.Fingers[Side][3][0];
	if (Hand == INDEX_NONE || Middle == INDEX_NONE || Index == INDEX_NONE || Pinky == INDEX_NONE)
	{
		return FVector::ZeroVector;
	}
	const FVector F = (G.Pos(Middle) - G.Pos(Hand)).GetSafeNormal();
	const FVector Across = (G.Pos(Index) - G.Pos(Pinky)).GetSafeNormal();
	const FVector N = FVector::CrossProduct(F, Across).GetSafeNormal();
	return Side == 0 ? -N : N;
}

/** Bends a chain of finger segments toward the palm (degrees per segment). */
void CurlChain(FGait& G, const int32* Chain, int32 Count, const FVector& Palm, const float* Angles)
{
	if (Palm.IsNearlyZero())
	{
		return;
	}
	for (int32 K = 0; K < Count; ++K)
	{
		const int32 B = Chain[K];
		if (B == INDEX_NONE || FMath::IsNearlyZero(Angles[K]))
		{
			continue;
		}
		// The segment runs toward the next joint; the last one keeps the direction of the one before it.
		const int32 Next = K + 1 < Count ? Chain[K + 1] : INDEX_NONE;
		const FVector Dir = Next != INDEX_NONE ? (G.Pos(Next) - G.Pos(B)).GetSafeNormal()
		                                       : (K > 0 && Chain[K - 1] != INDEX_NONE ? (G.Pos(B) - G.Pos(Chain[K - 1])).GetSafeNormal() : FVector::ZeroVector);
		const FVector Axis = FVector::CrossProduct(Dir, Palm).GetSafeNormal();
		if (!Axis.IsNearlyZero())
		{
			G.Rotate(B, FQuat(Axis, FMath::DegreesToRadians(Angles[K])));
		}
	}
}

/**
 * Two-bone reach: turns the upper arm and forearm so the wrist lands on Target, the elbow bending toward Pole (law of
 * cosines; out of reach, the arm straightens toward it). Weight blends from the pose it had.
 */
void Reach(FGait& G, int32 Upper, int32 Lower, int32 Hand, const FVector& Target, const FVector& Pole, float Weight)
{
	if (Upper == INDEX_NONE || Lower == INDEX_NONE || Hand == INDEX_NONE || Weight <= 0.0f)
	{
		return;
	}
	const FVector Shoulder = G.Pos(Upper);
	const float A = static_cast<float>(FVector::Dist(Shoulder, G.Pos(Lower)));
	const float B = static_cast<float>(FVector::Dist(G.Pos(Lower), G.Pos(Hand)));
	const FVector ToTarget = Target - Shoulder;
	const float Want = static_cast<float>(ToTarget.Size());
	if (A < 1.0f || B < 1.0f || Want < 1.0f)
	{
		return;
	}
	const FVector Dir = ToTarget / Want;
	const float D = FMath::Clamp(Want, FMath::Abs(A - B) + 0.5f, A + B - 0.5f);
	const float X = (A * A - B * B + D * D) / (2.0f * D);
	const float H = FMath::Sqrt(FMath::Max(0.0f, A * A - X * X));
	FVector Bend = Pole - Dir * FVector::DotProduct(Pole, Dir);
	if (!Bend.Normalize())
	{
		Bend = FVector(0.0, 0.0, -1.0);
	}
	const FVector Elbow = Shoulder + Dir * X + Bend * H;
	G.Aim(Upper, Lower, Elbow - Shoulder, Weight);
	G.Aim(Lower, Hand, Shoulder + Dir * D - G.Pos(Lower), Weight);
}
} // namespace StreetAnimDetail

using namespace StreetAnimDetail;

void FStreetBodyProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
	if (const UStreetBodyAnim* Anim = Cast<UStreetBodyAnim>(InAnimInstance))
	{
		Pose = Anim->Pose;
	}
}

void FStreetBodyProxy::FindBones(FCompactPose& InPose)
{
	const FBoneContainer& Container = InPose.GetBoneContainer();
	BoneSerial = Container.GetSerialNumber();
	const int32 N = InPose.GetNumBones();
	CS.SetNum(N);
	Parent.SetNum(N);
	Mark.SetNumZeroed(N);
	for (FCompactPoseBoneIndex I : InPose.ForEachBoneIndex())
	{
		const FCompactPoseBoneIndex P = Container.GetParentBoneIndex(I);
		Parent[I.GetInt()] = P.IsValid() ? P.GetInt() : INDEX_NONE;
	}
	auto IndexOf = [&Container](const FString& Name) -> int32 {
		const int32 MeshIndex = Container.GetPoseBoneIndexForBoneName(FName(*Name));
		if (MeshIndex == INDEX_NONE)
		{
			return INDEX_NONE;
		}
		const FCompactPoseBoneIndex C = Container.MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshIndex));
		return C.IsValid() ? C.GetInt() : INDEX_NONE;
	};
	Bones = FBones();
	Bones.Pelvis = IndexOf(TEXT("pelvis"));
	Bones.Spine1 = IndexOf(TEXT("spine_01"));
	Bones.Spine3 = IndexOf(TEXT("spine_03"));
	Bones.Spine5 = IndexOf(TEXT("spine_05"));
	Bones.Neck = IndexOf(TEXT("neck_01"));
	Bones.Head = IndexOf(TEXT("head"));
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const TCHAR* S = Side == 0 ? TEXT("l") : TEXT("r");
		Bones.Thigh[Side] = IndexOf(FString::Printf(TEXT("thigh_%s"), S));
		Bones.Calf[Side] = IndexOf(FString::Printf(TEXT("calf_%s"), S));
		Bones.Foot[Side] = IndexOf(FString::Printf(TEXT("foot_%s"), S));
		Bones.Clavicle[Side] = IndexOf(FString::Printf(TEXT("clavicle_%s"), S));
		Bones.UpperArm[Side] = IndexOf(FString::Printf(TEXT("upperarm_%s"), S));
		Bones.LowerArm[Side] = IndexOf(FString::Printf(TEXT("lowerarm_%s"), S));
		Bones.Hand[Side] = IndexOf(FString::Printf(TEXT("hand_%s"), S));
		for (int32 Fi = 0; Fi < 4; ++Fi)
		{
			for (int32 K = 0; K < 3; ++K)
			{
				Bones.Fingers[Side][Fi][K] = IndexOf(FString::Printf(TEXT("%s_%02d_%s"), FingerBase[Fi], K + 1, S));
			}
		}
		for (int32 K = 0; K < 3; ++K)
		{
			Bones.Thumb[Side][K] = IndexOf(FString::Printf(TEXT("thumb_%02d_%s"), K + 1, S));
		}
	}
}

bool FStreetBodyProxy::Evaluate(FPoseContext& Output)
{
	Output.ResetToRefPose();
	FCompactPose& Compact = Output.Pose;
	if (Compact.GetNumBones() == 0)
	{
		return true;
	}
	if (BoneSerial != static_cast<int32>(Compact.GetBoneContainer().GetSerialNumber()) || CS.Num() != Compact.GetNumBones())
	{
		FindBones(Compact);
	}
	FGait G(Compact, CS, Parent, Mark);
	const FBones& B = Bones;
	const FStreetBodyPose& P = Pose;
	const float Sign = CVarWalkSign.GetValueOnAnyThread() < 0.0f ? -1.0f : 1.0f;
	const float StrideK = CVarWalkStride.GetValueOnAnyThread();
	const float ArmsK = CVarWalkArms.GetValueOnAnyThread();
	const float BobK = CVarWalkBob.GetValueOnAnyThread();
	const float CurlK = FMath::Clamp(CVarWalkCurl.GetValueOnAnyThread(), 0.0f, 2.0f);
	// The head's reference orientation, before anything turns it: the mouth is measured from it.
	const FQuat HeadRef = B.Head != INDEX_NONE ? G.CS[B.Head].GetRotation() : FQuat::Identity;

	// How much of a gait there is: none standing, all of it from a slow walk up.
	const float Moving = FMath::Clamp(P.Speed / 70.0f, 0.0f, 1.0f);
	const float Run = FMath::Clamp(P.Run, 0.0f, 1.0f);
	const float ThighAmp = FMath::Lerp(23.0f, 36.0f, Run) * Moving * StrideK;
	const float KneeAmp = FMath::Lerp(40.0f, 85.0f, Run) * Moving * StrideK;
	const float ArmAmp = FMath::Lerp(16.0f, 32.0f, Run) * Moving * ArmsK;
	const float Elbow = FMath::Lerp(12.0f, 80.0f, Run) * (0.4f + 0.6f * Moving) + 6.0f;
	const float Breath = FMath::Sin(P.Time * 1.6f);
	const float Sip = FMath::Clamp(P.Sip, 0.0f, 1.0f);

	// The pelvis: lowest as each heel strikes, highest over the standing leg; it twists with the stride.
	const float S = FMath::Sin(P.Phase);
	const float Bob = FMath::Lerp(1.8f, 4.0f, Run) * Moving * BobK;
	G.Move(B.Pelvis, FVector(0.0, 0.0, Bob * (0.5f - S * S)));
	// Standing: the weight drifts from foot to foot (faded in as the walk stops, so it never jumps).
	const float Still = 1.0f - FMath::SmoothStep(0.05f, 0.25f, Moving);
	if (Still > 0.0f)
	{
		G.Move(B.Pelvis, FVector(1.1 * FMath::Sin(P.Time * 0.35f) * Still, 0.0, 0.0));
	}
	G.Rotate(B.Pelvis, GaitYaw(Sign * 5.0f * S * Moving * BobK) * GaitRoll(Sign * 2.0f * FMath::Sin(P.Phase * 2.0f) * Moving * BobK + P.Bank * 0.3f));

	// Legs: swing from the hip, bend through the swing, the foot kept near level.
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const float Ph = P.Phase + (Side == 0 ? 0.0f : PI);
		const float Swing = ThighAmp * FMath::Sin(Ph);
		const float Lift = FMath::Max(0.0f, FMath::Cos(Ph));
		const float Knee = KneeAmp * Lift * FMath::Sqrt(Lift) + 3.0f + Run * 14.0f * FMath::Max(0.0f, -FMath::Cos(Ph)) * Moving;
		const float Foot = (Knee - Swing) * 0.85f + 9.0f * FMath::Sin(Ph) * Moving;
		G.Rotate(B.Thigh[Side], GaitPitch(Sign * Swing));
		G.Rotate(B.Calf[Side], GaitPitch(-Sign * Knee));
		G.Rotate(B.Foot[Side], GaitPitch(Sign * Foot));
	}

	// The trunk: leaning into a run and into a start, rocking back on a stop, the chest turning against the hips, breathing.
	const float Lean = FMath::Max(-2.0f, 2.0f + Run * 9.0f * Moving + P.Tired * 4.0f + FMath::Clamp(P.Surge, -1.0f, 1.0f) * 5.0f * (0.3f + 0.7f * Moving));
	G.Rotate(B.Spine1, GaitPitch(-Sign * Lean));
	G.Rotate(B.Spine3, GaitYaw(-Sign * 6.0f * S * Moving * BobK) * GaitPitch(-Sign * 0.9f * Breath));
	G.Rotate(B.Spine5, GaitPitch(Sign * P.Tired * 3.0f));

	// The head: steady against the trunk, turned toward where the player looks. The neck takes a third of the turn and
	// the head the rest (the eyes go further still); a sip tips it back a little.
	const float LookYaw = FMath::Clamp(P.LookYaw, -70.0f, 70.0f) * 0.75f;
	const float LookPitch = FMath::Clamp(P.LookPitch, -40.0f, 35.0f) * 0.6f + 5.0f * Sip;
	G.Rotate(B.Neck, GaitYaw(LookYaw * 0.35f) * GaitPitch(Sign * (Lean * 0.7f + LookPitch * 0.3f)));
	G.Rotate(B.Head, GaitYaw(LookYaw * 0.65f) * GaitPitch(Sign * LookPitch * 0.7f));
	// How far the face has turned from its reference pose (the trunk, the neck and the head together): the mouth, and where
	// a sip holds the hand before it, turn with it.
	const FQuat FaceTurn = B.Head != INDEX_NONE ? G.CS[B.Head].GetRotation() * HeadRef.Inverse() : FQuat::Identity;
	const FVector Mouth = B.Head != INDEX_NONE ? G.Pos(B.Head) + FaceTurn.RotateVector(P.Mouth) : FVector::ZeroVector;

	// Arms: down at the sides, swinging against the legs, elbows bending more the faster it goes; hands loosely curled.
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const float Out = Side == 0 ? 1.0f : -1.0f; // the left arm hangs toward +X
		const float Ph = P.Phase + (Side == 0 ? 0.0f : PI);
		const float ArmSwing = -ArmAmp * FMath::Sin(Ph);
		const FVector Hang = FVector(0.17 * Out, 0.04, -1.0).GetSafeNormal();
		const FVector Upper = GaitPitch(Sign * ArmSwing).RotateVector(Hang);
		const FVector Fore = GaitPitch(Sign * (ArmSwing + Elbow)).RotateVector(Hang);
		const int32 HandB = B.Hand[Side];
		const int32 Middle = B.Fingers[Side][1][0];
		G.Rotate(B.Clavicle[Side], GaitRoll(Out * Sign * (P.Tired * 4.0f - 1.0f)));
		G.Aim(B.UpperArm[Side], B.LowerArm[Side], Upper);
		G.Aim(B.LowerArm[Side], HandB, Fore);
		G.Aim(HandB, Middle, Fore + FVector(0.0, 0.08, 0.0), 0.8f);

		const bool bSipping = Side == 1 && Sip > 0.001f && B.Head != INDEX_NONE;
		if (bSipping)
		{
			// The right hand brings a can or a bite to the mouth: the wrist just below and before it, the elbow down and
			// out, the fingers turned toward the lips with the palm in.
			const FVector Offset(-CVarSipSide.GetValueOnAnyThread(), CVarSipAhead.GetValueOnAnyThread(), CVarSipUp.GetValueOnAnyThread());
			const FVector Wrist = Mouth + FaceTurn.RotateVector(Offset);
			Reach(G, B.UpperArm[Side], B.LowerArm[Side], HandB, Wrist, FVector(-0.45, -0.25, -1.0), Sip);
			if (HandB != INDEX_NONE && Middle != INDEX_NONE)
			{
				const FVector FingerDir = (Mouth - G.Pos(HandB)).GetSafeNormal();
				const FVector Inward = FaceTurn.RotateVector(FVector(1.0, 0.0, 0.0));
				const FVector PalmDir = (Inward - FingerDir * FVector::DotProduct(Inward, FingerDir)).GetSafeNormal();
				const FVector F = (G.Pos(Middle) - G.Pos(HandB)).GetSafeNormal();
				const FVector Facing = PalmNormal(G, B, Side);
				if (!FingerDir.IsNearlyZero() && !PalmDir.IsNearlyZero() && !F.IsNearlyZero() && !Facing.IsNearlyZero())
				{
					const FQuat Cur = FRotationMatrix::MakeFromXZ(F, Facing).ToQuat();
					const FQuat Want = FRotationMatrix::MakeFromXZ(FingerDir, PalmDir).ToQuat();
					G.Rotate(HandB, FQuat::Slerp(FQuat::Identity, Want * Cur.Inverse(), Sip));
				}
			}
		}

		// Fingers: a relaxed curl, a little tighter at a run; closed round the can or the bite while sipping.
		float Curl = (0.28f + 0.2f * Run * Moving) * CurlK;
		float ThumbCurl = 0.18f * CurlK;
		if (bSipping)
		{
			Curl = FMath::Lerp(Curl, 0.62f, Sip);
			ThumbCurl = FMath::Lerp(ThumbCurl, 0.5f, Sip);
		}
		const FVector Palm = PalmNormal(G, B, Side);
		for (int32 Fi = 0; Fi < 4; ++Fi)
		{
			// Outer fingers curl a little more than the index, as a resting hand does.
			const float C = FMath::Clamp(Curl * (1.0f + 0.12f * Fi), 0.0f, 1.0f);
			const float Angles[3] = {70.0f * C, 92.0f * C, 55.0f * C};
			CurlChain(G, B.Fingers[Side][Fi], 3, Palm, Angles);
		}
		const float TC = FMath::Clamp(ThumbCurl, 0.0f, 1.0f);
		const float ThumbAngles[3] = {18.0f * TC, 35.0f * TC, 45.0f * TC};
		CurlChain(G, B.Thumb[Side], 3, Palm, ThumbAngles);
	}

	G.WriteBack();
	return true;
}
