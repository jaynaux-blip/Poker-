#include "BackRoomAnim.h"

#include "Animation/AnimNodeBase.h"
#include "BackRoomStage.h"
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
	Dt = FMath::Clamp(DeltaSeconds, 0.001f, 0.1f);
	if (const UBackRoomBodyAnim* Anim = Cast<UBackRoomBodyAnim>(InAnimInstance))
	{
		Pose = Anim->Pose;
	}
}

namespace
{
/**
 * Two-bone IK for an arm, the elbow's swing about the shoulder-to-hand line chosen here: where it hangs when
 * nothing is in the way (Prefer), else the nearest swing that keeps the elbow and both ends of the forearm
 * (and the upper arm by the elbow) out of the table, tested against its real surface. A swing that clears is taken
 * at once; coming back down to the relaxed one is slow.
 */
void SolveArm(const FBackRoomBodyPose& P, int32 Side, const FVector& Root, double L1, double L2, const FVector& Target, float Prefer, float Dt, float& Phi, FVector& OutJoint, FVector& OutEnd)
{
	const FVector To = Target - Root;
	double Dist = To.Size();
	const FVector D = Dist > 1.0e-3 ? To / Dist : FVector::YAxisVector;
	Dist = FMath::Clamp(Dist, FMath::Abs(L1 - L2) + 0.5, L1 + L2 - 0.05);
	OutEnd = Root + D * Dist;
	const double A = (L1 * L1 - L2 * L2 + Dist * Dist) / (2.0 * Dist);
	const double H = FMath::Sqrt(FMath::Max(L1 * L1 - A * A, 0.0));
	const FVector C = Root + D * A;
	// The elbow's circle: Up across the line (toward the ceiling), Out across it and away from the body.
	FVector Up = FVector::UpVector - D * FVector::DotProduct(FVector::UpVector, D);
	if (Up.SizeSquared() < 0.01)
	{
		Up = FVector::YAxisVector - D * D.Y;
	}
	Up = Up.GetSafeNormal();
	FVector Out = FVector::CrossProduct(Up, D).GetSafeNormal();
	if (FVector::DotProduct(Out, FVector(Side == 0 ? 1.0 : -1.0, 0.0, 0.0)) < 0.0)
	{
		Out = -Out;
	}
	auto Elbow = [&](float Angle) { return C + (Out * FMath::Cos(Angle) + Up * FMath::Sin(Angle)) * H; };
	float Want = Prefer;
	if (P.bTableContact)
	{
		auto Clearance = [&](const FVector& E) {
			float Worst = 1.0e3f;
			auto Test = [&](const FVector& At, float Radius) {
				FVector Push;
				float Clear;
				ABackRoomStage::TableContact(P.ToTable.TransformPosition(At), Radius + 0.15f, true, Push, Clear);
				Worst = FMath::Min(Worst, Clear);
			};
			Test(E, 3.2f);
			Test(FMath::Lerp(E, OutEnd, 0.35), 2.9f);
			Test(FMath::Lerp(E, OutEnd, 0.7), 2.7f);
			Test(FMath::Lerp(Root, E, 0.7), 3.6f);
			return Worst;
		};
		if (Clearance(Elbow(Prefer)) < 0.0f)
		{
			// The nearest swing that clears, walked out from the relaxed one on both sides and then narrowed to the
			// edge of clear (so it moves smoothly as the arm does, never in steps); where two sides clear at once, the
			// one nearer the swing it is at now. The most clearance if none does.
			constexpr float Lo = -1.9f, Hi = 1.5f, Step = 0.08f;
			float BestClear = -1.0e9f, BestAngle = Prefer;
			bool bFound = false;
			for (int32 K = 1; K <= 45 && !bFound; ++K)
			{
				float Cand[2] = {Prefer + Step * K, Prefer - Step * K};
				if (FMath::Abs(Cand[0] - Phi) > FMath::Abs(Cand[1] - Phi))
				{
					Swap(Cand[0], Cand[1]);
				}
				for (int32 Side2 = 0; Side2 < 2 && !bFound; ++Side2)
				{
					const float Angle = Cand[Side2];
					if (Angle < Lo || Angle > Hi)
					{
						continue;
					}
					const float Clear = Clearance(Elbow(Angle));
					if (Clear > BestClear)
					{
						BestClear = Clear;
						BestAngle = Angle;
					}
					if (Clear >= 0.0f)
					{
						// Narrow in toward the relaxed swing.
						float Fail = Angle > Prefer ? Angle - Step : Angle + Step, Pass = Angle;
						for (int32 Refine = 0; Refine < 5; ++Refine)
						{
							const float Mid = 0.5f * (Fail + Pass);
							if (Clearance(Elbow(Mid)) >= 0.0f)
							{
								Pass = Mid;
							}
							else
							{
								Fail = Mid;
							}
						}
						Want = Pass;
						bFound = true;
					}
				}
			}
			if (!bFound)
			{
				Want = BestAngle;
			}
		}
	}
	// Up to clear the rail fast (but not in a single frame), settling back down slowly.
	Phi = Want > Phi ? FMath::Min(Want, Phi + 24.0f * Dt) : FMath::Lerp(Phi, Want, 1.0f - FMath::Exp(-9.0f * Dt));
	OutJoint = Elbow(Phi);
}
} // namespace

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

	// The body: sits, leans, shoulders. (A function of the lean, so a lean that would put the belly through the
	// table's rail can be eased off and posed again in the same frame.)
	auto PoseBody = [&](float Lean) {
		S.CS = Ref;
		// Sit: the pelvis drops onto the seat and rolls back as the back slumps, or tips forward as the
		// player leans in over the table (people lean from the hips first).
		S.Move(Pelvis, FVector(0.0, -3.0, P.SeatHeight + 9.0) - S.Pos(Pelvis));
		S.Rotate(Pelvis, FQuat(FVector::XAxisVector, Rad(7.0f + 6.0f * P.Slouch - 15.0f * Lean)));

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
		const float LeanDeg = 4.0f + 34.0f * Lean + 8.0f * P.Slouch;
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
	};
	// How deep a ball at At (component space) of Radius is in the table, and the push out of it (table frame).
	// (The space under the apron is open for the body; a hand that is over the table is held by it all the way down,
	// so a finger deep in the felt is pushed back up, never on through; and a hand that is under the table
	// (in the lap) is held down out of the table's underside.)
	bool bHandUnder = true;
	auto Sunk = [&](const FVector& At, float Radius, FVector& Push) {
		float Clear;
		const FVector T = P.ToTable.TransformPosition(At);
		if (!ABackRoomStage::TableContact(T, Radius, bHandUnder, Push, Clear))
		{
			return 0.0f;
		}
		if (bHandUnder && T.Z + Radius > ABackRoomStage::ApronZ && T.Z < ABackRoomStage::ApronZ + 3.0)
		{
			const double Dy = FMath::Max(FMath::Abs(T.Y) - 61.0, 0.0);
			if (FMath::Sqrt(T.X * T.X + Dy * Dy) < ABackRoomStage::RailOuterD - 1.0)
			{
				// Up inside the table's underside: straight down out of it.
				Push = FVector(0.0, 0.0, -(T.Z + Radius - ABackRoomStage::ApronZ));
				return static_cast<float>(-Push.Z);
			}
		}
		return -Clear;
	};
	float Lean = P.Lean;
	PoseBody(Lean);
	if (P.bTableContact)
	{
		// The belly may press the rail's skirt and no further: a lean that would go through it is eased off.
		for (int32 Iter = 0; Iter < 6; ++Iter)
		{
			FVector Push;
			const float Deep = FMath::Max3(Sunk(S.Pos(Spine[1]), 13.0f, Push), Sunk(S.Pos(Spine[2]), 14.0f, Push), Sunk(S.Pos(Spine[3]), 13.0f, Push));
			if (Deep <= 0.0f || Lean <= 0.0f)
			{
				break;
			}
			Lean = FMath::Max(0.0f, Lean - 0.045f * Deep - 0.01f);
			PoseBody(Lean);
		}
	}

	// Arms: two-bone IK to the hand targets, the elbow where it hangs relaxed unless the table is in the way.
	// Posed against the table in the same frame: the arm, the hand and the fingers are posed, every ball of them
	// (fingertip, joint, knuckle, palm, wrist, forearm) is tested against the table's real surface, and what has
	// sunk in lifts the hand and pitches the fingers up about the wrist before it is posed again, until it clears.
	for (int32 Side = 0; Side < 2; ++Side)
	{
		ResolvedShift[Side] = FVector::ZeroVector;
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
		const int32 Mid = FindSided(S, TEXT("middle"), Side, TEXT("01"));
		int32 Chains[4][3];
		for (int32 Fi = 0; Fi < 4; ++Fi)
		{
			for (int32 K = 0; K < 3; ++K)
			{
				Chains[Fi][K] = FindSided(S, Fingers[Fi], Side, K == 0 ? TEXT("01") : (K == 1 ? TEXT("02") : TEXT("03")));
			}
		}
		const int32 Thumb[3] = {FindSided(S, TEXT("thumb"), Side, TEXT("01")), FindSided(S, TEXT("thumb"), Side, TEXT("02")), FindSided(S, TEXT("thumb"), Side, TEXT("03"))};

		const TArray<FTransform> Before = S.CS;
		const float PhiStart = ElbowPhi[Side];
		// Warm start: what held this hand off the table last frame, relaxing as the hand comes away. A hand resting on
		// the table settles on it, and the correction is one contact carried from frame to frame, never solved again
		// from nothing (which, near a tie, picks another answer each time and shows as a pop). A frame can change it
		// by no more than a hand can move.
		const FVector ShiftPrev = SolvedShift[Side];
		const float PitchPrev = SolvedPitch[Side];
		const float Relax = FMath::Exp(-14.0f * Dt);
		const float StepMax = 400.0f * Dt, PitchStepMax = 14.0f * Dt;
		FVector Shift = ShiftPrev * Relax;
		float Pitch = PitchPrev * Relax;
		auto Hold = [&]() {
			Shift = (ShiftPrev + (Shift - ShiftPrev).GetClampedToMaxSize(StepMax)).GetClampedToMaxSize(12.0);
			Pitch = FMath::Clamp(PitchPrev + FMath::Clamp(Pitch - PitchPrev, -PitchStepMax, PitchStepMax), 0.0f, 0.75f);
		};
		float Phi = PhiStart;
		for (int32 Iter = 0; Iter < 4; ++Iter)
		{
			if (Iter > 0)
			{
				S.CS = Before;
			}
			Phi = PhiStart;
			// The elbow hangs relaxed, or comes up and out only as far as it must to keep the forearm off the rail.
			FVector Joint, End;
			SolveArm(P, Side, S.Pos(Upper), FVector::Dist(S.Pos(Upper), S.Pos(Lower)), FVector::Dist(S.Pos(Lower), S.Pos(Hand)), Target + Shift, P.ElbowPrefer[Side], Dt, Phi, Joint, End);
			S.Aim(Upper, Lower, Joint - S.Pos(Upper), W);
			S.Aim(Lower, Hand, End - S.Pos(Lower), W);

			// The hand: fingers along FingerDir, palm toward PalmDir (pitched up about the wrist when fingers are in the table).
			FVector FingerDir = P.FingerDir[Side].GetSafeNormal(), PalmDir = P.PalmDir[Side].GetSafeNormal();
			if (Pitch > 0.0f)
			{
				const FVector Across = FVector::CrossProduct(FingerDir, FVector::UpVector).GetSafeNormal();
				if (!Across.IsNearlyZero())
				{
					const FQuat Q(Across, Pitch);
					FingerDir = Q.RotateVector(FingerDir);
					PalmDir = Q.RotateVector(PalmDir);
				}
			}
			// Which side of the table the hand is on this pass: under it (the lap) or over it (with a little hysteresis,
			// so a hand at the apron height does not flip between the two).
			const double WristZ = P.ToTable.TransformPosition(S.Pos(Hand)).Z;
			bHandUnder = bSolvedUnder[Side] ? WristZ < ABackRoomStage::ApronZ + 1.5 : WristZ < ABackRoomStage::ApronZ - 0.8;
			const FVector F = (S.Pos(Mid) - S.Pos(Hand)).GetSafeNormal();
			const FVector Palm = PalmNormal(S, Side);
			const FQuat Cur = FRotationMatrix::MakeFromXZ(F, Palm).ToQuat();
			const FQuat Want = FRotationMatrix::MakeFromXZ(FingerDir, PalmDir).ToQuat();
			S.Rotate(Hand, FQuat::Slerp(FQuat::Identity, Want * Cur.Inverse(), W));

			// Fingers curl toward the palm; the index closes further when pinching.
			const FVector PalmNow = PalmNormal(S, Side);
			for (int32 Fi = 0; Fi < 4; ++Fi)
			{
				float C = P.Curl[Side];
				if (Fi == 0)
				{
					C = FMath::Max(C, 0.55f * P.Pinch[Side]);
				}
				// Outer fingers curl a little more than the index, as a resting hand does.
				C = FMath::Clamp(C * (1.0f + 0.12f * Fi), 0.0f, 1.0f);
				const float Angles[3] = {70.0f * C, 92.0f * C, 55.0f * C};
				CurlChain(S, Chains[Fi], 3, PalmNow, Angles);
			}
			const float TC = FMath::Clamp(P.ThumbCurl[Side] + 0.5f * P.Pinch[Side], 0.0f, 1.0f);
			const float ThumbAngles[3] = {18.0f * TC, 35.0f * TC, 45.0f * TC};
			CurlChain(S, Thumb, 3, PalmNow, ThumbAngles);

			if (!P.bTableContact)
			{
				break;
			}
			// Against the table: the wrist, the palm and the forearm lift the hand; each finger pitches the hand up
			// by the angle that raises it the depth it has sunk (its distance from the wrist is the lever).
			const FVector Wrist = S.Pos(Hand);
			// Lift: out of the felt or the rail's crown from above. Drop: out of the table's underside (a hand in the
			// lap that has come up into it) from below.
			float Lift = 0.0f, Drop = 0.0f, OutMax = 0.0f;
			// The sideways push is the direction the pushes agree on, as deep as the deepest (opposite pushes cancel,
			// and it never jumps from one ball's answer to another's).
			FVector OutSum = FVector::ZeroVector;
			struct FFingerSink
			{
				float Rise, Lever;
			};
			FFingerSink Sinks[16];
			int32 NumSinks = 0;
			FVector Push;
			auto AddOut = [&](const FVector& At) {
				const FVector Flat(At.X, At.Y, 0.0);
				OutSum += Flat;
				OutMax = FMath::Max(OutMax, static_cast<float>(Flat.Size()));
			};
			auto Body = [&](const FVector& At, float Radius, float Gain) {
				const float D = Sunk(At, Radius + 0.12f, Push);
				if (D > 0.0f)
				{
					Lift = FMath::Max(Lift, static_cast<float>(Push.Z) * Gain);
					Drop = bHandUnder ? FMath::Max(Drop, static_cast<float>(-Push.Z) * Gain) : 0.0f;
					AddOut(Push);
				}
			};
			Body(Wrist, 2.4f, 1.0f);
			Body(S.Pos(Lower), 3.0f, 1.5f);
			Body(FMath::Lerp(S.Pos(Lower), Wrist, 0.5), 2.8f, 1.5f);
			Body(FMath::Lerp(Wrist, S.Pos(Mid), 0.5), 2.0f, 1.0f);
			auto Finger = [&](const FVector& At, float Radius) {
				const float D = Sunk(At, Radius + 0.12f, Push);
				if (D <= 0.0f)
				{
					return;
				}
				// Out of the table's side (the rail's inner wall) is sideways whichever way the hand also has to rise.
				AddOut(Push);
				if (Push.Z > 0.0)
				{
					if (NumSinks < 16)
					{
						Sinks[NumSinks++] = {static_cast<float>(Push.Z), FMath::Max(3.0f, static_cast<float>(FVector::Dist(At, Wrist)))};
					}
				}
				else
				{
					Drop = bHandUnder ? FMath::Max(Drop, static_cast<float>(-Push.Z)) : 0.0f;
				}
			};
			for (int32 Fi = 0; Fi < 4; ++Fi)
			{
				Finger(S.Pos(Chains[Fi][0]), 1.3f);
				Finger(S.Pos(Chains[Fi][1]), 1.0f);
				Finger(S.Pos(Chains[Fi][2]) + (S.Pos(Chains[Fi][2]) - S.Pos(Chains[Fi][1])).GetSafeNormal() * 2.0, 0.65f);
			}
			Finger(S.Pos(Thumb[0]), 1.4f);
			Finger(S.Pos(Thumb[1]), 1.0f);
			Finger(S.Pos(Thumb[2]) + (S.Pos(Thumb[2]) - S.Pos(Thumb[1])).GetSafeNormal() * 2.0, 0.65f);
			// The fingers first ask the whole hand to rise (a wrist held off the felt with the fingertips resting on it
			// is how a hand lies), a few centimeters at most; what that does not cover pitches them up about the wrist.
			float FingerRise = 0.0f;
			for (int32 K = 0; K < NumSinks; ++K)
			{
				FingerRise = FMath::Max(FingerRise, Sinks[K].Rise);
			}
			const float FingerLift = FMath::Min(FingerRise, FMath::Max(0.0f, 3.0f - static_cast<float>(Shift.Z)));
			float NeedPitch = 0.0f;
			for (int32 K = 0; K < NumSinks; ++K)
			{
				const float Residual = Sinks[K].Rise - FingerLift;
				if (Residual > 0.0f)
				{
					NeedPitch = FMath::Max(NeedPitch, FMath::Asin(FMath::Clamp(Residual / Sinks[K].Lever, 0.0f, 0.9f)));
				}
			}
			Lift = FMath::Max(Lift, FingerLift);
			const FVector Out = OutSum.GetSafeNormal() * FMath::Min(OutMax, static_cast<float>(OutSum.Size()));
			ResolvedWorst[Side] = FMath::Max3(Lift, Drop, FMath::Max(static_cast<float>(Out.Size()), NeedPitch * 10.0f));
			if ((Lift <= 0.03f && Drop <= 0.03f && NeedPitch <= 0.004f && Out.SizeSquared() < 0.0009) || Iter == 3)
			{
				break;
			}
			// (What sits above the felt comes up out of it; a hand that is only in the table underside goes down.)
			Shift += FVector(0.0, 0.0, (Lift > 0.03f ? Lift : -Drop) * 0.95f) + P.ToTable.InverseTransformVectorNoScale(Out) * 0.95f;
			Pitch += NeedPitch * 0.95f;
			Hold();
		}
		bSolvedUnder[Side] = bHandUnder;
		SolvedShift[Side] = Shift;
		SolvedPitch[Side] = Pitch;
		bHandUnder = true;
		ElbowPhi[Side] = Phi;
		ResolvedShift[Side] = Shift;
	}
	ResolvedLean = Lean;

	// Head: turn toward the look target, shared down the neck, within what a neck can do.
	if (Head != INDEX_NONE)
	{
		const FQuat Rel = S.CS[Head].GetRotation() * Ref[Head].GetRotation().Inverse();
		const FVector Fwd = Rel.RotateVector(FVector::YAxisVector);
		FVector Want = (P.LookAt - S.Pos(Head)).GetSafeNormal();
		// Limit to +/-85 degrees of yaw and -45..+35 of pitch from straight ahead.
		const float Yaw = FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan2(Want.X, Want.Y)), -85.0f, 85.0f);
		const float Pitch = FMath::Clamp(FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(Want.Z, -1.0, 1.0))), -45.0f, 35.0f);
		Want = FVector(FMath::Sin(Rad(Yaw)) * FMath::Cos(Rad(Pitch)), FMath::Cos(Rad(Yaw)) * FMath::Cos(Rad(Pitch)), FMath::Sin(Rad(Pitch)));
		FQuat D = FQuat::FindBetweenNormals(Fwd, Want);
		// The eyes take a comfortable 15-odd degrees; anything further, the head turns for (nobody
		// looks at the person beside them out of the corners of their eyes for long).
		const float Off = FMath::RadiansToDegrees(D.GetAngle());
		const float Follow = FMath::Max(P.HeadFollow, Off > 1.0f ? 1.0f - 16.0f / Off : 0.0f);
		D = FQuat::Slerp(FQuat::Identity, D, FMath::Clamp(Follow, 0.0f, 1.0f));
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
	bExpressionless = Anim->bExpressionless;
	Curves = Anim->bExpressionless ? TMap<FName, float>() : Anim->Curves;
	LookAt = Anim->LookAt;
	bHasLook = !LookAt.IsZero() && !bExpressionless;
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
	if (bExpressionless)
	{
		return;
	}
	FAnimationUpdateContext Context(this, DeltaSeconds);
	RigLogic.Update_AnyThread(Context);
}

bool FBackRoomFaceProxy::Evaluate(FPoseContext& Output)
{
	if (bExpressionless)
	{
		EvaluateSource(Output);
		return true;
	}
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
		// Never all the way to the rig's limit: that much white looks wrong at once.
		const float Left = FMath::Clamp(Yaw / 32.0f, 0.0f, 0.6f), Right = FMath::Clamp(-Yaw / 32.0f, 0.0f, 0.6f);
		const float Up = FMath::Clamp(Pitch / 24.0f, 0.0f, 0.6f), Down = FMath::Clamp(-Pitch / 28.0f, 0.0f, 0.85f);
		for (const TCHAR* Eye : {TEXT("L"), TEXT("R")})
		{
			Output.Curve.Set(FName(*FString::Printf(TEXT("CTRL_expressions_eyeLookLeft%s"), Eye)), Left);
			Output.Curve.Set(FName(*FString::Printf(TEXT("CTRL_expressions_eyeLookRight%s"), Eye)), Right);
			Output.Curve.Set(FName(*FString::Printf(TEXT("CTRL_expressions_eyeLookUp%s"), Eye)), Up);
			Output.Curve.Set(FName(*FString::Printf(TEXT("CTRL_expressions_eyeLookDown%s"), Eye)), Down);
		}
	}
}
