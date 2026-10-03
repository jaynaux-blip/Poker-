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

/** A pose held in component space (the Back Room's solver, cut down to what a gait needs). */
struct FGait
{
	FCompactPose& Pose;
	const FBoneContainer& Bones;
	TArray<FTransform> CS;
	TArray<int32> Parent;

	explicit FGait(FCompactPose& InPose)
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

FQuat GaitYaw(float Deg)
{
	return FQuat(FVector(0.0, 0.0, 1.0), FMath::DegreesToRadians(Deg));
}

FQuat GaitRoll(float Deg)
{
	return FQuat(FVector(0.0, 1.0, 0.0), FMath::DegreesToRadians(Deg));
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

bool FStreetBodyProxy::Evaluate(FPoseContext& Output)
{
	Output.ResetToRefPose();
	FGait G(Output.Pose);
	const FStreetBodyPose& P = Pose;
	const float Sign = CVarWalkSign.GetValueOnAnyThread() < 0.0f ? -1.0f : 1.0f;
	const float StrideK = CVarWalkStride.GetValueOnAnyThread();
	const float ArmsK = CVarWalkArms.GetValueOnAnyThread();
	const float BobK = CVarWalkBob.GetValueOnAnyThread();

	// How much of a gait there is: none standing, all of it from a slow walk up.
	const float Moving = FMath::Clamp(P.Speed / 70.0f, 0.0f, 1.0f);
	const float Run = FMath::Clamp(P.Run, 0.0f, 1.0f);
	const float ThighAmp = FMath::Lerp(23.0f, 36.0f, Run) * Moving * StrideK;
	const float KneeAmp = FMath::Lerp(40.0f, 85.0f, Run) * Moving * StrideK;
	const float ArmAmp = FMath::Lerp(16.0f, 32.0f, Run) * Moving * ArmsK;
	const float Elbow = FMath::Lerp(12.0f, 80.0f, Run) * (0.4f + 0.6f * Moving) + 6.0f;
	const float Breath = FMath::Sin(P.Time * 1.6f);

	const int32 Pelvis = G.Find(TEXT("pelvis"));
	const int32 Spine1 = G.Find(TEXT("spine_01"));
	const int32 Spine3 = G.Find(TEXT("spine_03"));
	const int32 Spine5 = G.Find(TEXT("spine_05"));
	const int32 Neck = G.Find(TEXT("neck_01"));
	const int32 Head = G.Find(TEXT("head"));

	// The pelvis: lowest as each heel strikes, highest over the standing leg; it twists with the stride.
	const float S = FMath::Sin(P.Phase);
	const float Bob = FMath::Lerp(1.8f, 4.0f, Run) * Moving * BobK;
	G.Move(Pelvis, FVector(0.0, 0.0, Bob * (0.5f - S * S)));
	if (Moving < 0.2f)
	{
		// Standing: the weight drifts from foot to foot.
		G.Move(Pelvis, FVector(1.1 * FMath::Sin(P.Time * 0.35f), 0.0, 0.0));
	}
	G.Rotate(Pelvis, GaitYaw(Sign * 5.0f * S * Moving * BobK) * GaitRoll(Sign * 2.0f * FMath::Sin(P.Phase * 2.0f) * Moving * BobK + P.Bank * 0.3f));

	// Legs: swing from the hip, bend through the swing, the foot kept near level.
	const TCHAR* Sides[2] = {TEXT("l"), TEXT("r")};
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const float Ph = P.Phase + (Side == 0 ? 0.0f : PI);
		const float Swing = ThighAmp * FMath::Sin(Ph);
		const float Lift = FMath::Max(0.0f, FMath::Cos(Ph));
		const float Knee = KneeAmp * Lift * FMath::Sqrt(Lift) + 3.0f + Run * 14.0f * FMath::Max(0.0f, -FMath::Cos(Ph)) * Moving;
		const float Foot = (Knee - Swing) * 0.85f + 9.0f * FMath::Sin(Ph) * Moving;
		const int32 Thigh = G.Find(*FString::Printf(TEXT("thigh_%s"), Sides[Side]));
		const int32 Calf = G.Find(*FString::Printf(TEXT("calf_%s"), Sides[Side]));
		const int32 FootB = G.Find(*FString::Printf(TEXT("foot_%s"), Sides[Side]));
		G.Rotate(Thigh, GaitPitch(Sign * Swing));
		G.Rotate(Calf, GaitPitch(-Sign * Knee));
		G.Rotate(FootB, GaitPitch(Sign * Foot));
	}

	// The trunk: leaning into a run, the chest turning against the hips, breathing.
	const float Lean = 2.0f + Run * 9.0f * Moving + P.Tired * 4.0f;
	G.Rotate(Spine1, GaitPitch(-Sign * Lean));
	G.Rotate(Spine3, GaitYaw(-Sign * 6.0f * S * Moving * BobK) * GaitPitch(-Sign * 0.9f * Breath));
	G.Rotate(Spine5, GaitPitch(Sign * P.Tired * 3.0f));

	// Arms: down at the sides, swinging against the legs, elbows bending more the faster it goes.
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const float Out = Side == 0 ? 1.0f : -1.0f; // the left arm hangs toward +X
		const float Ph = P.Phase + (Side == 0 ? 0.0f : PI);
		const float ArmSwing = -ArmAmp * FMath::Sin(Ph);
		const FVector Hang = FVector(0.17 * Out, 0.04, -1.0).GetSafeNormal();
		FVector Upper = GaitPitch(Sign * ArmSwing).RotateVector(Hang);
		FVector Fore = GaitPitch(Sign * (ArmSwing + Elbow)).RotateVector(Hang);
		if (Side == 1 && P.Sip > 0.0f)
		{
			// The right hand brings a can or a bite up to the mouth.
			const FVector SipUpper = FVector(-0.25, 0.45, -0.86).GetSafeNormal();
			const FVector SipFore = FVector(0.55, 0.35, 0.76).GetSafeNormal();
			Upper = FMath::Lerp(Upper, SipUpper, P.Sip).GetSafeNormal();
			Fore = FMath::Lerp(Fore, SipFore, P.Sip).GetSafeNormal();
		}
		const int32 Clav = G.Find(*FString::Printf(TEXT("clavicle_%s"), Sides[Side]));
		const int32 UpperB = G.Find(*FString::Printf(TEXT("upperarm_%s"), Sides[Side]));
		const int32 Lower = G.Find(*FString::Printf(TEXT("lowerarm_%s"), Sides[Side]));
		const int32 Hand = G.Find(*FString::Printf(TEXT("hand_%s"), Sides[Side]));
		const int32 Middle = G.Find(*FString::Printf(TEXT("middle_01_%s"), Sides[Side]));
		G.Rotate(Clav, GaitRoll(Out * Sign * (P.Tired * 4.0f - 1.0f)));
		G.Aim(UpperB, Lower, Upper);
		G.Aim(Lower, Hand, Fore);
		G.Aim(Hand, Middle, Fore + FVector(0.0, 0.08, 0.0), 0.8f);
	}

	// The head: steady against the trunk, turned toward where the player looks.
	G.Rotate(Neck, GaitPitch(Sign * Lean * 0.7f));
	G.Rotate(Head, GaitYaw(-Sign * FMath::Clamp(P.LookYaw, -70.0f, 70.0f) * 0.75f) * GaitPitch(Sign * FMath::Clamp(P.LookPitch, -40.0f, 35.0f) * 0.6f));
	G.WriteBack();
	return true;
}
