#include "StreetAnim.h"

#include "Animation/AnimNodeBase.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimationPoseData.h"
#include "BonePose.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Misc/PackageName.h"
#include "ShortStackCharacter.h"
#include "StreetWorld.h"

DEFINE_LOG_CATEGORY_STATIC(LogStreetAnim, Log, All);

// Picks recorded here (spec 6.6's ranges): an idle break every 8-20 s; a turn in place once the body is ~50 degrees off the
// actor standing still (or 20 degrees once the actor has stopped turning for 0.35 s); the walk loop's rate held to
// 0.75-1.3 of its own pace (outside that the feet slide a little rather than the stride looking wrong), blended toward the
// run loop from 1.15x the walk's pace to 0.85x the run's; crossfades of 0.2-0.35 s; the heavy set for bodies deeper than
// 0.225 of their height (ss.Anim.HeavyDepth: a normal-weight MetaHuman body in its A pose measures about 0.19-0.21, the
// overweight ones 0.24 and up; anim_setup.status() prints every built body's, to check the split against).

SHORTSTACK_STREET_PRELOAD(UStreetMocap)

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
// The mocap (spec 6.6, 7.7).
TAutoConsoleVariable<int32> CVarMocap(TEXT("ss.Anim.Mocap"), 1,
	TEXT("1 (default): the Game Animation Sample's baked clips drive the street's bodies where they're baked (anim_setup.py); 0: the old procedural gait, for side-by-side checks."));
TAutoConsoleVariable<int32> CVarBreaks(TEXT("ss.Anim.Breaks"), 1, TEXT("1: someone standing idle shifts their weight, looks about, stretches now and then (the idle's breaks); 0: never."));
TAutoConsoleVariable<int32> CVarTurns(TEXT("ss.Anim.Turns"), 1, TEXT("1: turning on the spot steps round with the turn clips; 0: the body turns with the actor (feet slide)."));
TAutoConsoleVariable<float> CVarHeavyDepth(TEXT("ss.Anim.HeavyDepth"), 0.225f,
	TEXT("A body this deep (front to back) for its height or deeper takes the heavy build's clips (_Ovw, retargeted with RTG_UEFN_to_Metahuman_ovw)."));
// Test overrides outlive the PIE session that set them: the street puts them back as it begins (spec 7.7).
FStreetOverrides::FRegister MocapOverride(TEXT("ss.Anim.Mocap"), TEXT("1"));
FStreetOverrides::FRegister BreaksOverride(TEXT("ss.Anim.Breaks"), TEXT("1"));
FStreetOverrides::FRegister TurnsOverride(TEXT("ss.Anim.Turns"), TEXT("1"));

const TCHAR* const FingerBase[4] = {TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky")};

/** The clips' names, in EStreetClip's order (anim_setup.py bakes the same list). */
const TCHAR* const ClipNames[] = {TEXT("StandIdle"), TEXT("IdleBreak1"), TEXT("IdleBreak2"), TEXT("IdleBreak3"), TEXT("IdleBreak4"), TEXT("IdleBreak5"), TEXT("IdleBreak6"),
	TEXT("Turn045L"), TEXT("Turn045R"), TEXT("Turn090L"), TEXT("Turn090R"), TEXT("Turn135L"), TEXT("Turn135R"), TEXT("Turn180L"), TEXT("Turn180R"), TEXT("WalkLoop"),
	TEXT("WalkStartL"), TEXT("WalkStartR"), TEXT("WalkStopL"), TEXT("WalkStopR"), TEXT("RunLoop"), TEXT("RunStartL"), TEXT("RunStartR"), TEXT("RunStopL"), TEXT("RunStopR")};
static_assert(UE_ARRAY_COUNT(ClipNames) == static_cast<int32>(EStreetClip::Count), "a name per clip");

constexpr int32 NumClips = static_cast<int32>(EStreetClip::Count);

/** A turn clip's nominal turn (degrees, + right), 0 for the rest. */
float NominalTurn(EStreetClip C)
{
	switch (C)
	{
	case EStreetClip::Turn045L: return -45.0f;
	case EStreetClip::Turn045R: return 45.0f;
	case EStreetClip::Turn090L: return -90.0f;
	case EStreetClip::Turn090R: return 90.0f;
	case EStreetClip::Turn135L: return -135.0f;
	case EStreetClip::Turn135R: return 135.0f;
	case EStreetClip::Turn180L: return -180.0f;
	case EStreetClip::Turn180R: return 180.0f;
	default: return 0.0f;
	}
}

/** The walk's and the run's own pace when a loop's root doesn't travel (cm/s): the sample's walk and jog. */
constexpr float WalkPace = 150.0f;
constexpr float RunPace = 380.0f;

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
		Refresh();
	}

	/** Component space again from the local pose (after it was changed from outside). */
	void Refresh()
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

/** A hand's fingers and thumb curled by Curl and ThumbCurl (0..1; outer fingers a little more, as a resting hand), the index and thumb closed further by Pinch. */
void CurlHand(FGait& G, const FStreetBodyProxy::FBones& B, int32 Side, float Curl, float ThumbCurl, float Pinch)
{
	const FVector Palm = PalmNormal(G, B, Side);
	for (int32 Fi = 0; Fi < 4; ++Fi)
	{
		const float C = FMath::Clamp(Curl * (1.0f + 0.12f * Fi) + (Fi == 0 ? 0.35f * Pinch : 0.0f), 0.0f, 1.0f);
		const float Angles[3] = {70.0f * C, 92.0f * C, 55.0f * C};
		CurlChain(G, B.Fingers[Side][Fi], 3, Palm, Angles);
	}
	const float TC = FMath::Clamp(ThumbCurl + 0.4f * Pinch, 0.0f, 1.0f);
	const float ThumbAngles[3] = {18.0f * TC, 35.0f * TC, 45.0f * TC};
	CurlChain(G, B.Thumb[Side], 3, Palm, ThumbAngles);
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

/** Turns the hand so its fingers point along Fingers and its palm faces Palm (component space), by Weight. */
void OrientHand(FGait& G, const FStreetBodyProxy::FBones& B, int32 Side, const FVector& Fingers, const FVector& Palm, float Weight)
{
	const int32 HandB = B.Hand[Side];
	const int32 Middle = B.Fingers[Side][1][0];
	if (HandB == INDEX_NONE || Middle == INDEX_NONE || Weight <= 0.0f)
	{
		return;
	}
	const FVector WantF = Fingers.GetSafeNormal();
	const FVector WantP = (Palm - WantF * FVector::DotProduct(Palm, WantF)).GetSafeNormal();
	const FVector F = (G.Pos(Middle) - G.Pos(HandB)).GetSafeNormal();
	const FVector Facing = PalmNormal(G, B, Side);
	if (WantF.IsNearlyZero() || WantP.IsNearlyZero() || F.IsNearlyZero() || Facing.IsNearlyZero())
	{
		return;
	}
	const FQuat Cur = FRotationMatrix::MakeFromXZ(F, Facing).ToQuat();
	const FQuat Want = FRotationMatrix::MakeFromXZ(WantF, WantP).ToQuat();
	G.Rotate(HandB, FQuat::Slerp(FQuat::Identity, Want * Cur.Inverse(), Weight));
}

/** A smooth 0..1..0 pulse over [A, B] of a cycle position T (seconds), easing in and out over Ease seconds. */
float Pulse(float T, float A, float B, float Ease)
{
	return FMath::SmoothStep(A, A + Ease, T) * (1.0f - FMath::SmoothStep(B - Ease, B, T));
}

/**
 * One frame of a work beat (spec 6.6): what each hand reaches for in the station's frame (cm: F ahead of their feet, R to
 * their right, U up from the floor), the palm and fingers there, the grip, and how far the trunk hinges at the hips. A hand
 * left out (Weight 0) keeps whatever the body below gave it.
 */
struct FBeatPose
{
	FVector Hand[2] = {FVector::ZeroVector, FVector::ZeroVector};
	FVector Pole[2] = {FVector(-0.2, -0.6, -1.0), FVector(-0.2, 0.6, -1.0)};
	FVector Fingers[2] = {FVector(1.0, 0.0, 0.0), FVector(1.0, 0.0, 0.0)};
	FVector Palm[2] = {FVector(0.0, 0.0, -1.0), FVector(0.0, 0.0, -1.0)};
	float Weight[2] = {0.0f, 0.0f};
	float Curl[2] = {0.3f, 0.3f};
	float Pinch[2] = {0.0f, 0.0f};
	float Hinge = 0.0f;
	/** A bite or a sip with the right hand (0..1), through the sip's own reach. */
	float Sip = 0.0f;
	/** The head held still (the barber's chair). */
	float HoldHead = 0.0f;
	/** The hands rest on the thighs (seated, nothing else in them). */
	bool bLap = false;

	void Set(int32 Side, const FVector& At, const FVector& InFingers, const FVector& InPalm, float InCurl = 0.3f, float InPinch = 0.0f, float InWeight = 1.0f)
	{
		Hand[Side] = At;
		Fingers[Side] = InFingers;
		Palm[Side] = InPalm;
		Curl[Side] = InCurl;
		Pinch[Side] = InPinch;
		Weight[Side] = InWeight;
	}
};

/** The two sets of a beat's hands blended (A to B by Alpha: a beat's own phases crossfading). */
FBeatPose Mix(const FBeatPose& A, const FBeatPose& B, float Alpha)
{
	FBeatPose Out = A;
	for (int32 S = 0; S < 2; ++S)
	{
		Out.Hand[S] = FMath::Lerp(A.Hand[S], B.Hand[S], Alpha);
		Out.Fingers[S] = FMath::Lerp(A.Fingers[S], B.Fingers[S], Alpha).GetSafeNormal();
		Out.Palm[S] = FMath::Lerp(A.Palm[S], B.Palm[S], Alpha).GetSafeNormal();
		Out.Weight[S] = FMath::Lerp(A.Weight[S], B.Weight[S], Alpha);
		Out.Curl[S] = FMath::Lerp(A.Curl[S], B.Curl[S], Alpha);
		Out.Pinch[S] = FMath::Lerp(A.Pinch[S], B.Pinch[S], Alpha);
	}
	Out.Hinge = FMath::Lerp(A.Hinge, B.Hinge, Alpha);
	return Out;
}

// Heights (cm, from the floor the station stands on): the Lucky Penny's counter, Ruby's from behind, the barber's head mark
// over chair 1 (a seated customer's head: the chair's 55 cm seat and a trunk), a table's top over a seat.
constexpr float StoreCounter = 95.0f;
constexpr float DinerCounter = 98.0f;
const FVector HeadMark(50.0, 0.0, 122.0);
const FVector Down(0.0, 0.0, -1.0);
const FVector Ahead(1.0, 0.0, 0.0);

/** The barber at the head mark: comb lifts in the left hand while the right pinches the scissors (about 4 Hz). */
FBeatPose BarberComb(float T)
{
	FBeatPose P;
	const float Lift = 9.0f * Pulse(FMath::Fmod(T, 1.6f), 0.1f, 1.3f, 0.35f);
	P.Set(0, HeadMark + FVector(-9.0, -9.0, 3.0 + Lift), FVector(0.4, 0.6, 0.3), FVector(0.6, 0.3, -0.5), 0.75f, 0.2f);
	const float Snip = 0.5f + 0.5f * FMath::Sin(T * 2.0f * PI * 4.0f);
	P.Set(1, HeadMark + FVector(-12.0, 8.0 + 2.0 * FMath::Sin(T * 2.1f), 1.0 + 3.0 * FMath::Sin(T * 0.9f)), FVector(0.6, -0.5, 0.4), FVector(0.3, -0.7, -0.4), 0.35f, Snip);
	P.Hinge = 9.0f;
	return P;
}

/** Clippers up the back and side of the head in passes, the left hand steadying it from above. */
FBeatPose BarberClippers(float T)
{
	FBeatPose P;
	const float Pass = FMath::Fmod(T, 1.3f) / 1.1f;
	const float Up = FMath::SmoothStep(0.0f, 1.0f, FMath::Min(Pass, 1.0f));
	P.Set(1, HeadMark + FVector(-14.0, 9.0, FMath::Lerp(-10.0f, 9.0f, Up)), FVector(0.1, -0.3, 1.0), FVector(0.2, -1.0, 0.0), 0.75f);
	P.Set(0, HeadMark + FVector(-4.0, -4.0, 13.0), FVector(0.5, 0.6, 0.0), Down, 0.25f);
	P.Hinge = 11.0f;
	return P;
}

/** The neck duster across the back of the neck. */
FBeatPose BarberBrush(float T)
{
	FBeatPose P;
	P.Set(1, HeadMark + FVector(-10.0, 2.0 + 6.0 * FMath::Sin(T * 2.0f * PI * 2.0f), -14.0), FVector(0.6, -0.4, 0.5), FVector(0.4, 0.0, -0.9), 0.7f);
	P.Hinge = 8.0f;
	return P;
}

/** The beat's frame of hands now, in the station's frame (see FBeatPose). False: a beat this body doesn't know (the idle keeps the hands). */
bool MakeBeat(FName Beat, float T, float SeatHeight, FBeatPose& P)
{
	static const FName Ring(TEXT("ring")), Attend(TEXT("attend")), PhoneLean(TEXT("phone")), PhoneStand(TEXT("phonestand")), PhoneSit(TEXT("phonesit")), Wipe(TEXT("wipe")),
		Cut(TEXT("cut")), Comb(TEXT("comb")), Clippers(TEXT("clippers")), Brush(TEXT("brush")), Pour(TEXT("pour")), Plate(TEXT("plate")), Pad(TEXT("pad")),
		Lean(TEXT("lean")), Chair(TEXT("chair")), Eat(TEXT("eat")), Coffee(TEXT("coffee")), Paper(TEXT("paper")), Bag(TEXT("bag")), Weights(TEXT("weights")),
		Lift(TEXT("lift")), Train(TEXT("train")), Treadmill(TEXT("treadmill")), Run(TEXT("run"));
	P = FBeatPose();
	// A table before a seat (a counter, a booth), or a counter before someone standing.
	const float Table = (SeatHeight > 1.0f ? SeatHeight : 70.0f) + 30.0f;
	// The gym stage's names for its machines (GymStage's ServiceMark: run, bag, lift, train) are these beats.
	if (Beat == Run)
	{
		return MakeBeat(Treadmill, T, SeatHeight, P);
	}
	if (Beat == Lift || Beat == Train)
	{
		return MakeBeat(Weights, T, SeatHeight, P);
	}
	if (Beat == PhoneSit)
	{
		// Seated with the phone (a booth, a stool): both forearms on the table, the phone in the right hand, the thumb scrolling.
		const float Scroll = 0.5f + 0.5f * FMath::Sin(T * 5.0f);
		P.Set(1, FVector(26.0, 4.0, Table + 6.0), FVector(0.7, -0.6, 0.3), FVector(-0.5, 0.0, 0.86), 0.45f, 0.15f * Scroll);
		P.Set(0, FVector(30.0, -16.0, Table + 1.0), FVector(0.7, 0.7, 0.0), Down, 0.3f);
		P.Hinge = 6.0f;
		return true;
	}
	if (Beat == Paper)
	{
		// The paper folded in quarters, held up in the right hand by its fold, the left steadying its edge; a page turned
		// (the hands dropping and coming back up) now and then.
		const float Turn = Pulse(FMath::Fmod(T, 21.0f), 17.5f, 20.0f, 0.6f);
		const float Up = 1.0f - 0.6f * Turn;
		P.Set(1, FVector(30.0, 6.0, Table + 4.0 + 16.0 * Up), FVector(0.2, -0.2, 1.0), FVector(-1.0, -0.15, 0.1), 0.55f, 0.35f);
		P.Set(0, FVector(30.0, -12.0, Table + 4.0 + 18.0 * Up), FVector(0.2, 0.3, 1.0), FVector(-1.0, 0.2, 0.1), 0.35f, 0.0f, 0.8f);
		P.Hinge = 4.0f;
		return true;
	}
	if (Beat == Ring)
	{
		// Ringing up: the right hand on the register's keys, a press now and then; the left flat on the counter.
		const float Press = FMath::Max(0.0f, FMath::Sin(T * 12.0f)) * (FMath::Fmod(T, 2.4f) < 1.5f ? 1.0f : 0.0f);
		P.Set(1, FVector(42.0, 18.0, StoreCounter + 7.0 - 1.5 * Press), FVector(1.0, -0.2, -0.3), Down, 0.25f, 0.4f);
		P.Set(0, FVector(34.0, -22.0, StoreCounter + 1.5), FVector(1.0, 0.3, 0.0), Down, 0.15f);
		P.Hinge = 6.0f;
		return true;
	}
	if (Beat == Attend)
	{
		P.Set(0, FVector(34.0, -24.0, StoreCounter + 1.5), FVector(1.0, 0.25, 0.0), Down, 0.2f);
		P.Set(1, FVector(34.0, 24.0, StoreCounter + 1.5), FVector(1.0, -0.25, 0.0), Down, 0.2f);
		P.Hinge = 4.0f;
		return true;
	}
	if (Beat == PhoneLean || Beat == PhoneStand)
	{
		// The phone held low in the right hand, screen up, the thumb scrolling; leaning on the counter, the left forearm on it.
		const bool bLean = Beat == PhoneLean;
		const float Scroll = 0.5f + 0.5f * FMath::Sin(T * 5.0f);
		P.Set(1, bLean ? FVector(26.0, 6.0, StoreCounter + 18.0) : FVector(24.0, 4.0, 112.0), FVector(0.7, -0.6, 0.3), FVector(-0.5, 0.0, 0.86), 0.4f, 0.0f);
		P.Curl[1] = 0.45f;
		P.Pinch[1] = 0.15f * Scroll;
		if (bLean)
		{
			P.Set(0, FVector(38.0, -10.0, StoreCounter + 3.0), FVector(0.6, 0.8, 0.0), Down, 0.25f);
			P.Hinge = 14.0f;
		}
		return true;
	}
	if (Beat == Wipe)
	{
		// A rag in circles on the counter, the other hand resting on it.
		const float A = T * 2.0f * PI * 1.3f;
		P.Set(1, FVector(46.0 + 13.0 * FMath::Cos(A), 8.0 + 13.0 * FMath::Sin(A), StoreCounter + 2.0), FVector(1.0, 0.0, -0.2), Down, 0.45f);
		P.Set(0, FVector(30.0, -26.0, StoreCounter + 1.0), FVector(1.0, 0.3, 0.0), Down, 0.2f);
		P.Hinge = 10.0f;
		return true;
	}
	if (Beat == Comb)
	{
		P = BarberComb(T);
		return true;
	}
	if (Beat == Clippers)
	{
		P = BarberClippers(T);
		return true;
	}
	if (Beat == Brush)
	{
		P = BarberBrush(T);
		return true;
	}
	if (Beat == Cut)
	{
		// The cut's cycle: comb and scissors, the clippers, the scissors again, the duster; each into the next over 0.4 s.
		const float C = FMath::Fmod(T, 16.0f);
		const float Bounds[5] = {0.0f, 6.0f, 10.0f, 14.0f, 16.0f};
		FBeatPose Phases[4] = {BarberComb(T), BarberClippers(T), BarberComb(T + 3.0f), BarberBrush(T)};
		int32 K = 0;
		while (K < 3 && C >= Bounds[K + 1])
		{
			++K;
		}
		P = Phases[K];
		const float Into = C - Bounds[K];
		if (Into < 0.4f)
		{
			P = Mix(Phases[(K + 3) % 4], Phases[K], FMath::SmoothStep(0.0f, 0.4f, Into));
		}
		return true;
	}
	if (Beat == Pour)
	{
		// The carafe from the hip to the cup mark, a pour held there, and back; every five seconds.
		const float C = FMath::Fmod(T, 5.0f);
		const float Out = Pulse(C, 0.2f, 3.4f, 0.8f);
		const FVector Rest(30.0, 22.0, 108.0);
		const FVector Cup(52.0, 6.0, DinerCounter + 17.0);
		const float Tilt = Pulse(C, 1.1f, 2.6f, 0.3f);
		P.Set(1, FMath::Lerp(Rest, Cup, Out), FVector(1.0, -0.3, -0.6 * Tilt).GetSafeNormal(), FVector(-0.2, -1.0, 0.0), 0.7f);
		P.Set(0, FVector(34.0, -24.0, DinerCounter + 1.5), FVector(1.0, 0.25, 0.0), Down, 0.2f);
		P.Hinge = 5.0f + 5.0f * Out;
		return true;
	}
	if (Beat == Plate)
	{
		// A plate set down in front of someone with both hands, then the hands back to the counter's edge.
		const float C = FMath::Fmod(T, 4.0f);
		const float Out = Pulse(C, 0.0f, 2.4f, 0.9f);
		const float Lower = FMath::SmoothStep(0.6f, 1.4f, C) * Out;
		for (int32 S = 0; S < 2; ++S)
		{
			const float Side = S == 0 ? -1.0f : 1.0f;
			P.Set(S, FMath::Lerp(FVector(32.0, 24.0 * Side, DinerCounter + 2.0), FVector(52.0, 11.0 * Side, DinerCounter + 9.0 - 6.0 * Lower), Out), FVector(1.0, -0.3 * Side, 0.0),
				FVector(0.0, -0.4 * Side, -0.9), 0.5f, 0.3f);
		}
		P.Hinge = 6.0f + 8.0f * Out;
		return true;
	}
	if (Beat == Pad)
	{
		// The order pad in the left palm, the pen scribbling over it.
		P.Set(0, FVector(26.0, -6.0, 114.0), FVector(0.3, 1.0, 0.2), FVector(-0.4, 0.0, 0.9), 0.35f);
		P.Set(1, FVector(27.0 + 1.0 * FMath::Sin(T * 6.0f * 2.0f * PI), 4.0 + 1.5 * FMath::Sin(T * 11.0f), 117.0), FVector(0.3, -0.6, -0.7), FVector(-0.3, -0.6, -0.7), 0.5f,
			0.6f);
		P.Hinge = 6.0f;
		return true;
	}
	if (Beat == Lean)
	{
		// Leaning on the pass between orders, forearms on it.
		P.Set(0, FVector(36.0, -18.0, DinerCounter + 4.0), FVector(0.7, 0.7, 0.0), Down, 0.3f);
		P.Set(1, FVector(36.0, 18.0, DinerCounter + 4.0), FVector(0.7, -0.7, 0.0), Down, 0.3f);
		P.Hinge = 16.0f;
		return true;
	}
	if (Beat == Chair)
	{
		// In the barber's chair: hands on the armrests, the head held for him.
		P.Set(0, FVector(18.0, -25.0, SeatHeight + 20.0), Ahead, Down, 0.35f);
		P.Set(1, FVector(18.0, 25.0, SeatHeight + 20.0), Ahead, Down, 0.35f);
		P.HoldHead = 1.0f;
		return true;
	}
	if (Beat == Eat || Beat == Coffee)
	{
		// At the counter or a booth: the left hand on the table, the right with the fork (a bite every 4.5 s) or the mug (a sip every 7).
		const bool bEat = Beat == Eat;
		const float Every = bEat ? 4.5f : 7.0f;
		const float C = FMath::Fmod(T, Every);
		P.Sip = FMath::SmoothStep(0.0f, 0.55f, C) * (1.0f - FMath::SmoothStep(1.2f, 1.8f, C));
		P.Set(0, FVector(32.0, -18.0, Table + 1.0), FVector(1.0, 0.3, 0.0), Down, 0.25f);
		P.Set(1, FVector(bEat ? 34.0 : 28.0, 14.0, Table + (bEat ? 3.0 : 9.0)), bEat ? FVector(1.0, -0.6, -0.4) : FVector(0.6, -0.8, 0.0), bEat ? Down : FVector(0.0, -1.0, 0.0),
			bEat ? 0.55f : 0.7f, bEat ? 0.3f : 0.0f);
		return true;
	}
	if (Beat == Bag)
	{
		// The heavy bag: the guard up, a jab and a cross every 1.4 s.
		const float C = FMath::Fmod(T, 1.4f);
		const float Jab = Pulse(C, 0.0f, 0.3f, 0.12f);
		const float Cross = Pulse(C, 0.7f, 1.0f, 0.12f);
		P.Set(0, FMath::Lerp(FVector(22.0, -12.0, 142.0), FVector(58.0, -6.0, 146.0), Jab), FVector(1.0, 0.1, 0.0), FVector(0.0, 0.6, -0.8), 0.95f);
		P.Set(1, FMath::Lerp(FVector(18.0, 12.0, 140.0), FVector(58.0, -2.0, 146.0), Cross), FVector(1.0, -0.1, 0.0), FVector(0.0, -0.6, -0.8), 0.95f);
		P.Curl[0] = P.Curl[1] = 0.95f;
		P.Hinge = 6.0f + 4.0f * Cross;
		return true;
	}
	if (Beat == Weights)
	{
		// Dumbbell curls, both arms, slowly (on the bench, from the thighs: seated, the hands start from the seat's height).
		const float S = 0.5f - 0.5f * FMath::Cos(T * 2.0f * PI * 0.45f);
		const float Base = SeatHeight > 1.0f && SeatHeight < 70.0f ? SeatHeight + 14.0f : 86.0f;
		const float Ahead2 = SeatHeight > 1.0f && SeatHeight < 70.0f ? 22.0f : 6.0f;
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const float R = Side == 0 ? -21.0f : 21.0f;
			P.Set(Side, FVector(Ahead2 + 10.0 * S, R, Base + 38.0 * S), FVector(0.0, 0.0, -1.0).RotateAngleAxis(-80.0f * S, FVector(0.0, 1.0, 0.0)),
				FVector(0.3 * S, 0.0, 1.0).GetSafeNormal(), 0.9f);
			P.Pole[Side] = FVector(-1.0, R > 0.0f ? 0.3 : -0.3, -0.5);
		}
		return true;
	}
	if (Beat == Treadmill)
	{
		return true; // the legs run on the spot; the arms swing with them
	}
	return false;
}
} // namespace StreetAnimDetail

using namespace StreetAnimDetail;

// ------------------------------------------------------------------ the mocap library

UStreetMocap* UStreetMocap::Get(const UObject* Context)
{
	const UWorld* World = Context ? Context->GetWorld() : nullptr;
	UGameInstance* Instance = World ? World->GetGameInstance() : nullptr;
	return Instance ? Instance->GetSubsystem<UStreetMocap>() : nullptr;
}

FString UStreetMocap::ClipName(int32 InSet, EStreetClip C)
{
	const int32 Index = FMath::Clamp(static_cast<int32>(C), 0, NumClips - 1);
	return InSet == Heavy ? FString::Printf(TEXT("AS_NS_%s_Ovw"), ClipNames[Index]) : FString::Printf(TEXT("AS_NS_%s"), ClipNames[Index]);
}

FString UStreetMocap::ClipPath(int32 InSet, EStreetClip C)
{
	const FString Name = ClipName(InSet, C);
	return FString::Printf(TEXT("/Game/ShortStack/Anim/Mocap/%s.%s"), *Name, *Name);
}

void UStreetMocap::AppendPreload(TArray<FString>& Paths)
{
	for (int32 S = 0; S < NumSets; ++S)
	{
		for (int32 C = 0; C < NumClips; ++C)
		{
			Paths.Add(ClipPath(S, static_cast<EStreetClip>(C)));
		}
	}
}

void UStreetMocap::EnsureLoaded() const
{
	if (!bLoaded)
	{
		const_cast<UStreetMocap*>(this)->Load();
	}
}

void UStreetMocap::Load()
{
	bLoaded = true;
	Clips.SetNum(NumSets * NumClips);
	Infos.SetNum(NumSets * NumClips);
	int32 Found[NumSets] = {0, 0};
	for (int32 S = 0; S < NumSets; ++S)
	{
		for (int32 C = 0; C < NumClips; ++C)
		{
			const int32 I = S * NumClips + C;
			const FString Path = ClipPath(S, static_cast<EStreetClip>(C));
			// Only what's baked: a missing clip is the normal state until anim_setup.py runs (no warning for each).
			UAnimSequence* Seq = FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(Path))
									 ? LoadObject<UAnimSequence>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet)
									 : nullptr;
			Clips[I] = Seq;
			if (!Seq)
			{
				continue;
			}
			++Found[S];
			FClipInfo& Info = Infos[I];
			Info.Length = Seq->GetPlayLength();
			// The root over the clip, a few samples at a time (a turn can pass 180 degrees, and a loop's root goes straight on).
			constexpr int32 Steps = 16;
			FTransform Last = Seq->ExtractRootTrackTransform(FAnimExtractContext(0.0), nullptr);
			const FVector From = Last.GetLocation();
			double Yaw = 0.0;
			for (int32 K = 1; K <= Steps; ++K)
			{
				const FTransform Now = Seq->ExtractRootTrackTransform(FAnimExtractContext(static_cast<double>(Info.Length) * K / Steps), nullptr);
				Yaw += FMath::FindDeltaAngleDegrees(Last.Rotator().Yaw, Now.Rotator().Yaw);
				Last = Now;
			}
			Info.RootYaw = static_cast<float>(Yaw);
			Info.RootDistance = static_cast<float>(FVector::Dist2D(From, Last.GetLocation()));
			Info.Speed = Info.Length > 0.05f ? Info.RootDistance / Info.Length : 0.0f;
			for (const FAnimSyncMarker& M : Seq->AuthoredSyncMarkers)
			{
				const FString Name = M.MarkerName.ToString().ToLower();
				const bool bLeft = Name.StartsWith(TEXT("l")) || Name.Contains(TEXT("left"));
				const bool bRight = Name.StartsWith(TEXT("r")) || Name.Contains(TEXT("right"));
				if (bLeft && Info.LeftFootAt < 0.0f)
				{
					Info.LeftFootAt = M.Time;
				}
				else if (bRight && Info.RightFootAt < 0.0f)
				{
					Info.RightFootAt = M.Time;
				}
			}
		}
	}
	// The turns are played only when they carry their turn in the root (the retarget kept the root motion): otherwise the
	// body would turn twice, once with the clip and once with the actor.
	const FClipInfo& Ninety = Infos[static_cast<int32>(EStreetClip::Turn090R)];
	bTurnsUsable = Clips[static_cast<int32>(EStreetClip::Turn090R)] && FMath::Abs(Ninety.RootYaw) > 45.0f;
	UE_LOG(LogStreetAnim, Log, TEXT("The street's mocap: %d of %d standard clips baked, %d heavy; %s"), Found[0], NumClips, Found[1], *Describe());
}

UAnimSequence* UStreetMocap::Clip(int32 InSet, EStreetClip C) const
{
	EnsureLoaded();
	const int32 Index = static_cast<int32>(C);
	if (Index < 0 || Index >= NumClips)
	{
		return nullptr;
	}
	if (InSet == Heavy)
	{
		if (UAnimSequence* Seq = Clips[NumClips + Index])
		{
			return Seq;
		}
	}
	return Clips[Index];
}

const UStreetMocap::FClipInfo& UStreetMocap::Info(int32 InSet, EStreetClip C) const
{
	EnsureLoaded();
	const int32 Index = FMath::Clamp(static_cast<int32>(C), 0, NumClips - 1);
	if (InSet == Heavy && Clips[NumClips + Index])
	{
		return Infos[NumClips + Index];
	}
	return Infos[Index];
}

bool UStreetMocap::CanDrive(int32 InSet, const USkeleton* Skeleton) const
{
	const UAnimSequence* Idle = Clip(InSet, EStreetClip::StandIdle);
	const UAnimSequence* Walk = Clip(InSet, EStreetClip::WalkLoop);
	return Skeleton && Idle && Walk && Idle->GetSkeleton() == Skeleton && Walk->GetSkeleton() == Skeleton;
}

FString UStreetMocap::Describe() const
{
	EnsureLoaded();
	int32 Found[NumSets] = {0, 0};
	for (int32 S = 0; S < NumSets; ++S)
	{
		for (int32 C = 0; C < NumClips; ++C)
		{
			Found[S] += Clips[S * NumClips + C] ? 1 : 0;
		}
	}
	const FClipInfo& Walk = Info(Standard, EStreetClip::WalkLoop);
	const FClipInfo& Run = Info(Standard, EStreetClip::RunLoop);
	const FClipInfo& Turn = Info(Standard, EStreetClip::Turn090R);
	return FString::Printf(TEXT("mocap: %d/%d standard, %d/%d heavy; walk loop %.2f s at %.0f cm/s, run loop %.2f s at %.0f cm/s; the 90 right turns %.0f deg in its root (turns %s); %s"),
		Found[0], NumClips, Found[1], NumClips, Walk.Length, Walk.Speed, Run.Length, Run.Speed, Turn.RootYaw, bTurnsUsable ? TEXT("on") : TEXT("off"),
		CVarMocap.GetValueOnGameThread() > 0 ? TEXT("ss.Anim.Mocap 1") : TEXT("ss.Anim.Mocap 0 (procedural)"));
}

// ------------------------------------------------------------------ the body's graph (game thread)

void UStreetBodyAnim::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	Dice.Initialize(static_cast<int32>(GetUniqueID() * 2654435761u >> 8));
	NextBreak = Dice.FRandRange(8.0f, 20.0f);
	Library = UStreetMocap::Get(this);
	bYawKnown = false;
	State = EMove::Idle;
	Cur = FNode();
	Prev = FNode();
	Fade = 1.0f;
}

void UStreetBodyAnim::ChooseSet(const AShortStackCharacter* Owner)
{
	const USkeletalMeshComponent* Mesh = GetSkelMeshComponent();
	const USkeletalMesh* Asset = Mesh ? Mesh->GetSkeletalMeshAsset() : nullptr;
	if (Asset == SetFor.Get() && SetFor.IsValid())
	{
		return;
	}
	SetFor = Asset;
	Set = UStreetMocap::Standard;
	bCanDrive = false;
	const UStreetMocap* Lib = Library.Get();
	if (!Asset || !Lib)
	{
		return;
	}
	// The heavy build's clips for a body deep for its height (the overweight MetaHuman bodies' bellies): measured from its bounds.
	const FBoxSphereBounds Bounds = Asset->GetBounds();
	const double Height = FMath::Max(1.0, Bounds.BoxExtent.Z * 2.0);
	const double Depth = Bounds.BoxExtent.Y * 2.0;
	const bool bHeavy = Depth / Height >= CVarHeavyDepth.GetValueOnGameThread();
	Set = bHeavy && Lib->CanDrive(UStreetMocap::Heavy, Asset->GetSkeleton()) ? UStreetMocap::Heavy : UStreetMocap::Standard;
	bCanDrive = Lib->CanDrive(Set, Asset->GetSkeleton());
	UE_LOG(LogStreetAnim, Verbose, TEXT("%s: %s, depth %.2f of its height: %s"), *GetNameSafe(Owner), *GetNameSafe(Asset), Depth / Height,
		bCanDrive ? (Set == UStreetMocap::Heavy ? TEXT("the heavy clips") : TEXT("the standard clips")) : TEXT("no clips for its skeleton: the procedural gait"));
}

bool UStreetBodyAnim::HasClip(EStreetClip C) const
{
	const UStreetMocap* Lib = Library.Get();
	const UAnimSequence* Seq = Lib ? Lib->Clip(Set, C) : nullptr;
	const USkeletalMeshComponent* Mesh = GetSkelMeshComponent();
	const USkeletalMesh* Asset = Mesh ? Mesh->GetSkeletalMeshAsset() : nullptr;
	return Seq && Asset && Seq->GetSkeleton() == Asset->GetSkeleton();
}

float UStreetBodyAnim::ClipLength(EStreetClip C) const
{
	const UStreetMocap* Lib = Library.Get();
	return Lib ? FMath::Max(0.05f, Lib->Info(Set, C).Length) : 1.0f;
}

float UStreetBodyAnim::TurnYawAt(EStreetClip C, float Time) const
{
	const UStreetMocap* Lib = Library.Get();
	const UAnimSequence* Seq = Lib ? Lib->Clip(Set, C) : nullptr;
	if (!Seq)
	{
		return 0.0f;
	}
	const FTransform Start = Seq->ExtractRootTrackTransform(FAnimExtractContext(0.0), nullptr);
	const FTransform Now = Seq->ExtractRootTrackTransform(FAnimExtractContext(static_cast<double>(Time)), nullptr);
	// A turn past 180 reads back across the seam: unwrap toward the clip's own turn.
	float Yaw = FMath::FindDeltaAngleDegrees(Start.Rotator().Yaw, Now.Rotator().Yaw);
	const float Total = Lib->Info(Set, C).RootYaw;
	if (Total > 170.0f && Yaw < -90.0f)
	{
		Yaw += 360.0f;
	}
	else if (Total < -170.0f && Yaw > 90.0f)
	{
		Yaw -= 360.0f;
	}
	return Yaw;
}

void UStreetBodyAnim::Go(EMove To, EStreetClip Clip, float InFade, bool bLoop)
{
	Prev = Cur;
	Cur = FNode();
	Cur.Clip = Clip;
	Cur.bLoop = bLoop;
	Cur.bLocoLoop = To == EMove::Loop || To == EMove::InPlace;
	Fade = InFade > 0.0f ? 0.0f : 1.0f;
	FadeLen = FMath::Max(0.01f, InFade);
	State = To;
	StateTime = 0.0f;
}

void UStreetBodyAnim::Advance(FNode& N, float Dt) const
{
	if (N.bLocoLoop)
	{
		return; // the loops run on the shared cycle (LoopPhase)
	}
	const float Len = ClipLength(N.Clip);
	N.Time += Dt * N.Rate;
	N.Time = N.bLoop ? FMath::Fmod(N.Time, Len) : FMath::Min(N.Time, Len);
}

void UStreetBodyAnim::Emit(const FNode& N, float Weight)
{
	const UStreetMocap* Lib = Library.Get();
	if (!Lib || Weight <= 0.001f)
	{
		return;
	}
	auto Add = [this](const UAnimSequence* Seq, float Time, float W) {
		if (Seq && W > 0.001f && Motion.NumLayers < static_cast<int32>(UE_ARRAY_COUNT(Motion.Layers)))
		{
			FStreetMotion::FLayer& L = Motion.Layers[Motion.NumLayers++];
			L.Seq = Seq;
			L.Time = Time;
			L.Weight = W;
		}
	};
	if (N.bLocoLoop)
	{
		const UAnimSequence* Run = HasClip(EStreetClip::RunLoop) ? Lib->Clip(Set, EStreetClip::RunLoop) : nullptr;
		const float Share = Run ? N.RunShare : 0.0f;
		Add(Lib->Clip(Set, EStreetClip::WalkLoop), LoopPhase * ClipLength(EStreetClip::WalkLoop), Weight * (1.0f - Share));
		Add(Run, LoopPhase * ClipLength(EStreetClip::RunLoop), Weight * Share);
		return;
	}
	Add(Lib->Clip(Set, N.Clip), N.Time, Weight);
}

void UStreetBodyAnim::UpdateLocomotion(float Dt, const AShortStackCharacter* Owner)
{
	static const FName Treadmill(TEXT("treadmill")), RunBeat(TEXT("run"));
	Motion.NumLayers = 0;
	Motion.bMocap = false;
	const UStreetMocap* Lib = Library.Get();
	const USkeletalMeshComponent* Mesh = GetSkelMeshComponent();
	// The ground speed in the body's own units (the clips were baked at the cast's size; a taller body's mesh is scaled up).
	const float MeshScale = Mesh ? FMath::Max(0.5f, static_cast<float>(Mesh->GetRelativeScale3D().Z)) : 1.0f;
	const float Speed = Pose.Speed / MeshScale;
	const float Accel = Dt > 0.0f ? (Speed - LastSpeed) / Dt : 0.0f;
	LastSpeed = Speed;
	const bool bInPlace = Motion.Beat == Treadmill || Motion.Beat == RunBeat;
	Motion.InPlace = FMath::FInterpConstantTo(Motion.InPlace, bInPlace ? 1.0f : 0.0f, Dt, 2.0f);
	Motion.InPlacePhase = FMath::Fmod(Motion.InPlacePhase + Dt * 2.0f * PI * 1.4f * Motion.InPlace, 2.0f * PI);

	// Turning on the spot: the body keeps facing where it did while the actor turns, until a turn clip steps it round (spec
	// 6.6). A snap (placed somewhere new) has nothing to catch up with; walking or sitting, the body goes with the actor.
	const float ActorYaw = Owner ? static_cast<float>(Owner->GetActorRotation().Yaw) : 0.0f;
	float DYaw = bYawKnown ? FMath::FindDeltaAngleDegrees(LastActorYaw, ActorYaw) : 0.0f;
	LastActorYaw = ActorYaw;
	bYawKnown = true;
	if (FMath::Abs(DYaw) > 45.0f)
	{
		Motion.RootYaw = 0.0f;
		DYaw = 0.0f;
	}
	const bool bWant = CVarMocap.GetValueOnGameThread() > 0 && Lib && bCanDrive;
	if (!bWant)
	{
		Motion.RootYaw = 0.0f;
		State = EMove::Idle;
		Cur = FNode();
		Fade = 1.0f;
		return;
	}
	Motion.bMocap = true;
	Motion.PelvisRef = Lib->Clip(Set, EStreetClip::StandIdle);
	const bool bSeated = Motion.SeatBlend > 0.01f;
	const bool bStanding = (State == EMove::Idle || State == EMove::Break || State == EMove::Turn) && !bSeated;
	// Not in first person: there the body turns with the view at once (the camera rides the eyes, and a body left facing where
	// it was would swing the view sideways round the feet as the mouse turns it).
	const bool bTurns = CVarTurns.GetValueOnGameThread() > 0 && Lib->TurnsUsable() && !(Owner && Owner->IsFirstPerson());
	if (bStanding && bTurns)
	{
		Motion.RootYaw -= DYaw;
		StillTurning = FMath::Abs(DYaw) > 0.05f ? 0.0f : StillTurning + Dt;
	}
	else
	{
		Motion.RootYaw *= FMath::Exp(-10.0f * Dt);
	}
	Motion.RootYaw = FMath::Clamp(Motion.RootYaw, -150.0f, 150.0f);
	StateTime += Dt;

	const bool bTalking = Owner && Owner->IsTalking();
	const bool bWorking = !Motion.Beat.IsNone();
	const float StartSpeed = 22.0f;
	const bool bMoving = Speed > StartSpeed && !bSeated && !bInPlace;
	// The loops: the walk at its own pace, toward the run as the speed rises past it (a hurry is a quick walk; the hero's run).
	const UStreetMocap::FClipInfo& WalkInfo = Lib->Info(Set, EStreetClip::WalkLoop);
	const bool bRunClip = HasClip(EStreetClip::RunLoop);
	const float WalkV = WalkInfo.Speed > 20.0f ? WalkInfo.Speed : WalkPace;
	const float RunV = bRunClip && Lib->Info(Set, EStreetClip::RunLoop).Speed > 50.0f ? Lib->Info(Set, EStreetClip::RunLoop).Speed : RunPace;
	auto RunShareAt = [&](float V) { return bRunClip ? FMath::Clamp((V - WalkV * 1.15f) / FMath::Max(1.0f, RunV * 0.85f - WalkV * 1.15f), 0.0f, 1.0f) : 0.0f; };
	auto StartLoop = [&](float InFade) {
		Go(EMove::Loop, EStreetClip::WalkLoop, InFade, true);
		Cur.RunShare = RunShareAt(Speed);
	};
	auto StartWalking = [&]() {
		const bool bRunning = RunShareAt(Speed) > 0.5f;
		const EStreetClip Start =
			bRunning ? (bLeftNext ? EStreetClip::RunStartL : EStreetClip::RunStartR) : (bLeftNext ? EStreetClip::WalkStartL : EStreetClip::WalkStartR);
		bLeftNext = !bLeftNext;
		if (HasClip(Start) && Accel > 0.0f && Speed < WalkV * (bRunning ? 2.5f : 1.0f))
		{
			Go(EMove::Start, Start, 0.18f, false);
		}
		else
		{
			StartLoop(0.25f);
		}
		LoopPhase = 0.0f;
	};
	auto WantsTurn = [&]() {
		const float Off = FMath::Abs(Motion.RootYaw);
		return bTurns && bStanding && (Off > 50.0f || (Off > 20.0f && StillTurning > 0.35f));
	};
	auto StartTurn = [&]() {
		const float Off = FMath::Abs(Motion.RootYaw);
		// The body faces left of the actor (negative): it turns right to catch up, and the reverse.
		const bool bRight = Motion.RootYaw < 0.0f;
		EStreetClip Clip = Off < 67.5f ? (bRight ? EStreetClip::Turn045R : EStreetClip::Turn045L)
						   : Off < 112.5f ? (bRight ? EStreetClip::Turn090R : EStreetClip::Turn090L)
						   : Off < 157.5f ? (bRight ? EStreetClip::Turn135R : EStreetClip::Turn135L)
										  : (bRight ? EStreetClip::Turn180R : EStreetClip::Turn180L);
		if (!HasClip(Clip))
		{
			return false;
		}
		Go(EMove::Turn, Clip, 0.2f, false);
		TurnStartOffset = Motion.RootYaw;
		TurnLastYaw = 0.0f;
		// A wider turn than the clip's own plays it a little quicker.
		Cur.Rate = FMath::Clamp(Off / FMath::Max(30.0f, FMath::Abs(NominalTurn(Clip))), 1.0f, 1.5f);
		return true;
	};

	if (bInPlace && State != EMove::InPlace)
	{
		Go(EMove::InPlace, EStreetClip::WalkLoop, 0.3f, true);
		Cur.RunShare = bRunClip ? 1.0f : 0.0f;
	}
	switch (State)
	{
	case EMove::Idle:
	case EMove::Break:
		if (bMoving)
		{
			StartWalking();
			break;
		}
		if (WantsTurn() && StartTurn())
		{
			break;
		}
		if (State == EMove::Break)
		{
			if (Cur.Time >= ClipLength(Cur.Clip) - 0.35f || bTalking || bWorking || bSeated)
			{
				Go(EMove::Idle, EStreetClip::StandIdle, 0.35f, true);
			}
		}
		else if (Cur.Clip != EStreetClip::StandIdle || Cur.bLocoLoop)
		{
			Go(EMove::Idle, EStreetClip::StandIdle, 0.3f, true);
		}
		else if (Clock >= NextBreak)
		{
			NextBreak = Clock + Dice.FRandRange(8.0f, 20.0f);
			if (CVarBreaks.GetValueOnGameThread() > 0 && !bTalking && !bWorking && !bSeated)
			{
				// One of the six breaks, never the same twice running.
				TArray<int32, TInlineAllocator<6>> Pool;
				for (int32 K = 0; K < 6; ++K)
				{
					if (K != LastBreak && HasClip(static_cast<EStreetClip>(static_cast<int32>(EStreetClip::IdleBreak1) + K)))
					{
						Pool.Add(K);
					}
				}
				if (Pool.Num() > 0)
				{
					LastBreak = Pool[Dice.RandRange(0, Pool.Num() - 1)];
					const EStreetClip Clip = static_cast<EStreetClip>(static_cast<int32>(EStreetClip::IdleBreak1) + LastBreak);
					Go(EMove::Break, Clip, 0.35f, false);
					NextBreak += ClipLength(Clip);
				}
			}
		}
		break;
	case EMove::Turn:
	{
		if (bMoving)
		{
			StartWalking();
			break;
		}
		// The clip's own turn closes the offset it started with (scaled to it), as its root turns.
		const float Total = Lib->Info(Set, Cur.Clip).RootYaw;
		if (FMath::Abs(Total) > 1.0f)
		{
			const float P = FMath::Clamp(TurnYawAt(Cur.Clip, Cur.Time) / Total, 0.0f, 1.2f);
			Motion.RootYaw += -TurnStartOffset * (P - TurnLastYaw);
			TurnLastYaw = P;
		}
		if (Cur.Time >= ClipLength(Cur.Clip) - 0.2f)
		{
			Go(EMove::Idle, EStreetClip::StandIdle, 0.25f, true);
		}
		break;
	}
	case EMove::Start:
		if (Speed < 10.0f || bSeated)
		{
			Go(EMove::Idle, EStreetClip::StandIdle, 0.3f, true);
		}
		else if (Cur.Time >= ClipLength(Cur.Clip) * 0.65f)
		{
			StartLoop(0.3f);
		}
		break;
	case EMove::Loop:
		Cur.RunShare = FMath::FInterpTo(Cur.RunShare, RunShareAt(Speed), Dt, 6.0f);
		if (Speed < 12.0f || (Speed < 32.0f && Accel <= 0.0f) || bSeated)
		{
			// The stop on the foot the cycle is on (the loop's own markers where it has them).
			const float L = WalkInfo.LeftFootAt >= 0.0f ? WalkInfo.LeftFootAt / FMath::Max(0.05f, WalkInfo.Length) : 0.0f;
			const float Rel = FMath::Fmod(LoopPhase - L + 1.0f, 1.0f);
			const bool bLeft = Rel < 0.5f;
			const bool bRunning = Cur.RunShare > 0.5f;
			const EStreetClip Stop = bRunning ? (bLeft ? EStreetClip::RunStopL : EStreetClip::RunStopR) : (bLeft ? EStreetClip::WalkStopL : EStreetClip::WalkStopR);
			if (HasClip(Stop) && !bSeated)
			{
				Go(EMove::Stop, Stop, 0.2f, false);
			}
			else
			{
				Go(EMove::Idle, EStreetClip::StandIdle, 0.3f, true);
			}
		}
		break;
	case EMove::Stop:
		if (bMoving && Speed > 40.0f)
		{
			StartLoop(0.2f);
		}
		else if (Cur.Time >= ClipLength(Cur.Clip) - 0.3f || bSeated)
		{
			Go(EMove::Idle, EStreetClip::StandIdle, 0.3f, true);
		}
		break;
	case EMove::InPlace:
		if (!bInPlace)
		{
			Go(EMove::Idle, EStreetClip::StandIdle, 0.4f, true);
		}
		break;
	}

	// The loops' shared cycle: as far along as the ground covered (on the spot, at the run's own pace).
	if (State == EMove::Loop || State == EMove::InPlace || Prev.bLocoLoop)
	{
		const float Share = State == EMove::InPlace ? Cur.RunShare : (Cur.bLocoLoop ? Cur.RunShare : Prev.RunShare);
		const float Pace = FMath::Lerp(WalkV, RunV, Share);
		const float Cycle = FMath::Lerp(ClipLength(EStreetClip::WalkLoop), bRunClip ? ClipLength(EStreetClip::RunLoop) : ClipLength(EStreetClip::WalkLoop), Share);
		const float V = State == EMove::InPlace ? Pace : Speed;
		const float Rate = FMath::Clamp(V / FMath::Max(1.0f, Pace), State == EMove::Loop ? 0.75f : 0.0f, 1.3f);
		LoopPhase = FMath::Fmod(LoopPhase + Dt * Rate / FMath::Max(0.1f, Cycle), 1.0f);
	}
	Advance(Cur, Dt);
	if (Fade < 1.0f)
	{
		Advance(Prev, Dt);
		Fade = FMath::Min(1.0f, Fade + Dt / FadeLen);
	}
	if (Fade < 1.0f)
	{
		Emit(Prev, 1.0f - Fade);
	}
	Emit(Cur, Fade);
	if (Motion.NumLayers == 0)
	{
		Motion.bMocap = false;
	}
}

void UStreetBodyAnim::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	const float Dt = FMath::Clamp(DeltaSeconds, 0.0f, 0.25f);
	Clock += Dt;
	if (!Library.IsValid())
	{
		Library = UStreetMocap::Get(this);
	}
	const AShortStackCharacter* Owner = Cast<AShortStackCharacter>(GetOwningActor());
	const USkeletalMeshComponent* Mesh = GetSkelMeshComponent();
	// The work beat and the seat, from the character (SetWork, SetSeated), in the body's component space.
	const float Scale = Mesh ? FMath::Max(0.1f, static_cast<float>(Mesh->GetComponentScale().Z)) : 1.0f;
	Motion.Beat = Owner ? Owner->GetWorkBeat() : NAME_None;
	Motion.BeatTime = Owner ? Owner->GetWorkTime() : 0.0f;
	// The body's own frame (+X ahead: component +Y; +Y to its right: component -X), in world cm.
	Motion.Station = FTransform(FRotator(0.0f, 90.0f, 0.0f), FVector::ZeroVector, FVector(1.0f / Scale));
	if (Owner && Mesh && !Motion.Beat.IsNone())
	{
		const FTransform& Station = Owner->GetWorkStation();
		// The station's frame, unless the body has been put somewhere else (it turned to talk, it's walking to a stool).
		const FVector Off = Station.GetLocation() - Owner->GetActorLocation();
		if (!Station.Equals(FTransform::Identity) && Off.Size2D() < 60.0)
		{
			FTransform Feet = Station;
			Feet.SetLocation(FVector(Station.GetLocation().X, Station.GetLocation().Y, Mesh->GetComponentLocation().Z));
			Feet.SetScale3D(FVector::OneVector);
			Motion.Station = Feet.GetRelativeTransform(Mesh->GetComponentTransform());
		}
	}
	Motion.SeatBlend = Owner ? Owner->GetSeatBlend() : 0.0f;
	Motion.SeatHeight = Owner ? Owner->GetSeatHeight() / Scale : 46.0f;
	ChooseSet(Owner);
	UpdateLocomotion(Dt, Owner);
	// The proxy takes this frame's pose and motion now (it was given last frame's in PreUpdate, before this ran).
	FStreetBodyProxy& Proxy = GetProxyOnGameThread<FStreetBodyProxy>();
	Proxy.Pose = Pose;
	Proxy.Motion = Motion;
}

FString UStreetBodyAnim::DescribeMotion() const
{
	static const TCHAR* const States[] = {TEXT("idle"), TEXT("break"), TEXT("turn"), TEXT("start"), TEXT("loop"), TEXT("stop"), TEXT("in place")};
	FString Out;
	if (Motion.bMocap)
	{
		Out = FString::Printf(TEXT("mocap (%s set): %s %s %.2f s"), Set == UStreetMocap::Heavy ? TEXT("heavy") : TEXT("standard"), States[static_cast<int32>(State)],
			ClipNames[static_cast<int32>(Cur.Clip)], Cur.bLocoLoop ? LoopPhase : Cur.Time);
		if (Cur.bLocoLoop)
		{
			Out += FString::Printf(TEXT(" (run %.2f)"), Cur.RunShare);
		}
		if (FMath::Abs(Motion.RootYaw) > 1.0f)
		{
			Out += FString::Printf(TEXT(", turned %.0f off the actor"), Motion.RootYaw);
		}
	}
	else
	{
		Out = CVarMocap.GetValueOnGameThread() > 0 ? TEXT("procedural gait (no clips for this body)") : TEXT("procedural gait (ss.Anim.Mocap 0)");
	}
	Out += FString::Printf(TEXT(", %.0f cm/s"), Pose.Speed);
	if (!Motion.Beat.IsNone())
	{
		Out += FString::Printf(TEXT(", beat %s %.1f s"), *Motion.Beat.ToString(), Motion.BeatTime);
	}
	if (Motion.SeatBlend > 0.0f)
	{
		Out += FString::Printf(TEXT(", seated %.0f%% at %.0f cm"), Motion.SeatBlend * 100.0f, Motion.SeatHeight);
	}
	return Out;
}

// ------------------------------------------------------------------ posing (any thread)

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
	ClipPelvisFrom = nullptr;
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
	Bones.Root = N > 0 ? 0 : INDEX_NONE;
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
		Bones.Ball[Side] = IndexOf(FString::Printf(TEXT("ball_%s"), S));
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

bool FStreetBodyProxy::SampleClips(FPoseContext& Output)
{
	FCompactPose& Out = Output.Pose;
	float Sum = 0.0f;
	bool bAny = false;
	for (int32 K = 0; K < Motion.NumLayers; ++K)
	{
		const FStreetMotion::FLayer& L = Motion.Layers[K];
		if (!L.Seq || L.Weight <= 0.001f)
		{
			continue;
		}
		FPoseContext Sample(Output);
		FAnimationPoseData Data(Sample);
		L.Seq->GetAnimationPose(Data, FAnimExtractContext(static_cast<double>(L.Time)));
		Sum += L.Weight;
		if (!bAny)
		{
			Out.CopyBonesFrom(Sample.Pose);
			bAny = true;
			continue;
		}
		const float Alpha = L.Weight / Sum;
		for (FCompactPoseBoneIndex I : Out.ForEachBoneIndex())
		{
			Out[I].BlendWith(Sample.Pose[I], Alpha);
		}
	}
	if (!bAny)
	{
		return false;
	}
	// The clips were baked on one body: each bone keeps this body's own length ("skeleton" translation retargeting), the
	// pelvis's travel scaled to this body's hip height, and the root stays put (the actor moves the body, not the clip).
	if (Motion.PelvisRef && ClipPelvisFrom != Motion.PelvisRef && Bones.Pelvis != INDEX_NONE)
	{
		FPoseContext Standing(Output);
		FAnimationPoseData Data(Standing);
		Motion.PelvisRef->GetAnimationPose(Data, FAnimExtractContext(0.0));
		ClipPelvisHeight = static_cast<float>(Standing.Pose[FCompactPoseBoneIndex(Bones.Pelvis)].GetTranslation().Size());
		ClipPelvisFrom = Motion.PelvisRef;
	}
	const FCompactPoseBoneIndex PelvisI(Bones.Pelvis);
	const float RefPelvis = Bones.Pelvis != INDEX_NONE ? static_cast<float>(Out.GetRefPose(PelvisI).GetTranslation().Size()) : 0.0f;
	const float PelvisScale = ClipPelvisHeight > 1.0f && RefPelvis > 1.0f ? RefPelvis / ClipPelvisHeight : 1.0f;
	for (FCompactPoseBoneIndex I : Out.ForEachBoneIndex())
	{
		const int32 B = I.GetInt();
		if (B == Bones.Root)
		{
			Out[I] = Out.GetRefPose(I);
		}
		else if (B == Bones.Pelvis)
		{
			Out[I].SetTranslation(Out[I].GetTranslation() * PelvisScale);
		}
		else
		{
			Out[I].SetTranslation(Out.GetRefPose(I).GetTranslation());
		}
	}
	return true;
}

bool FStreetBodyProxy::Evaluate(FPoseContext& Output)
{
	FCompactPose& Compact = Output.Pose;
	if (Compact.GetNumBones() == 0)
	{
		Output.ResetToRefPose();
		return true;
	}
	if (BoneSerial != static_cast<int32>(Compact.GetBoneContainer().GetSerialNumber()) || CS.Num() != Compact.GetNumBones())
	{
		FindBones(Compact);
	}
	const bool bClips = Motion.bMocap && Motion.NumLayers > 0 && SampleClips(Output);
	if (!bClips)
	{
		Output.ResetToRefPose();
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
	// Turned on the spot: the whole body about its feet (the actor has turned further; a turn clip catches it up).
	if (!FMath::IsNearlyZero(Motion.RootYaw) && B.Root != INDEX_NONE)
	{
		G.Rotate(B.Root, GaitYaw(Motion.RootYaw));
	}

	FBeatPose Beat;
	const bool bBeat = !Motion.Beat.IsNone() && MakeBeat(Motion.Beat, Motion.BeatTime, Motion.SeatBlend > 0.01f ? Motion.SeatHeight : 0.0f, Beat);
	// Running on the spot (the treadmill) on the procedural gait: its own cycle at a jog.
	const float InPlace = bClips ? 0.0f : Motion.InPlace;
	const float GaitSpeed = FMath::Max(P.Speed, 330.0f * InPlace);
	const float Phase = InPlace > 0.5f ? Motion.InPlacePhase : P.Phase;
	const float Moving = FMath::Clamp(GaitSpeed / 70.0f, 0.0f, 1.0f) * (1.0f - Motion.SeatBlend);
	const float Run = FMath::Max(FMath::Clamp(P.Run, 0.0f, 1.0f), 0.6f * InPlace);
	const float Breath = FMath::Sin(P.Time * 1.6f);
	const float Sip = FMath::Clamp(FMath::Max(P.Sip, Beat.Sip), 0.0f, 1.0f);
	const float Stumble = FMath::Clamp(P.Stumble, 0.0f, 1.0f);
	float Lean = 0.0f;

	if (!bClips)
	{
		// The procedural gait (the fallback): from the reference pose, legs and pelvis and arms swinging.
		const float ThighAmp = FMath::Lerp(23.0f, 36.0f, Run) * Moving * StrideK;
		const float KneeAmp = FMath::Lerp(40.0f, 85.0f, Run) * Moving * StrideK;
		const float ArmAmp = FMath::Lerp(16.0f, 32.0f, Run) * Moving * ArmsK;
		const float Elbow = FMath::Lerp(12.0f, 80.0f, Run) * (0.4f + 0.6f * Moving) + 6.0f;
		// The pelvis: lowest as each heel strikes, highest over the standing leg; it twists with the stride.
		const float S = FMath::Sin(Phase);
		const float Bob = FMath::Lerp(1.8f, 4.0f, Run) * Moving * BobK;
		G.Move(B.Pelvis, FVector(0.0, 0.0, Bob * (0.5f - S * S)));
		// Standing: the weight drifts from foot to foot (faded in as the walk stops, so it never jumps).
		const float Still = 1.0f - FMath::SmoothStep(0.05f, 0.25f, Moving);
		if (Still > 0.0f)
		{
			G.Move(B.Pelvis, FVector(1.1 * FMath::Sin(P.Time * 0.35f) * Still, 0.0, 0.0));
		}
		G.Rotate(B.Pelvis, GaitYaw(Sign * 5.0f * S * Moving * BobK) * GaitRoll(Sign * 2.0f * FMath::Sin(Phase * 2.0f) * Moving * BobK + P.Bank * 0.3f));
		// Legs: swing from the hip, bend through the swing, the foot kept near level.
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const float Ph = Phase + (Side == 0 ? 0.0f : PI);
			const float Swing = ThighAmp * FMath::Sin(Ph);
			const float Lift = FMath::Max(0.0f, FMath::Cos(Ph));
			const float Knee = KneeAmp * Lift * FMath::Sqrt(Lift) + 3.0f + Run * 14.0f * FMath::Max(0.0f, -FMath::Cos(Ph)) * Moving;
			const float Foot = (Knee - Swing) * 0.85f + 9.0f * FMath::Sin(Ph) * Moving;
			G.Rotate(B.Thigh[Side], GaitPitch(Sign * Swing));
			G.Rotate(B.Calf[Side], GaitPitch(-Sign * Knee));
			G.Rotate(B.Foot[Side], GaitPitch(Sign * Foot));
		}
		// The trunk: leaning into a run and into a start, rocking back on a stop, the chest turning against the hips, breathing.
		Lean = FMath::Max(-2.0f, 2.0f + Run * 9.0f * Moving + P.Tired * 4.0f + FMath::Clamp(P.Surge, -1.0f, 1.0f) * 5.0f * (0.3f + 0.7f * Moving));
		G.Rotate(B.Spine1, GaitPitch(-Sign * Lean));
		G.Rotate(B.Spine3, GaitYaw(-Sign * 6.0f * S * Moving * BobK) * GaitPitch(-Sign * 0.9f * Breath));
		G.Rotate(B.Spine5, GaitPitch(Sign * P.Tired * 3.0f));
		// Arms: down at the sides, swinging against the legs, elbows bending more the faster it goes.
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const float Out = Side == 0 ? 1.0f : -1.0f; // the left arm hangs toward +X
			const float Ph = Phase + (Side == 0 ? 0.0f : PI);
			const float ArmSwing = -ArmAmp * FMath::Sin(Ph);
			const FVector Hang = FVector(0.17 * Out, 0.04, -1.0).GetSafeNormal();
			const FVector Upper = GaitPitch(Sign * ArmSwing).RotateVector(Hang);
			const FVector Fore = GaitPitch(Sign * (ArmSwing + Elbow)).RotateVector(Hang);
			G.Rotate(B.Clavicle[Side], GaitRoll(Out * Sign * (P.Tired * 4.0f - 1.0f)));
			G.Aim(B.UpperArm[Side], B.LowerArm[Side], Upper);
			G.Aim(B.LowerArm[Side], B.Hand[Side], Fore);
			G.Aim(B.Hand[Side], B.Fingers[Side][1][0], Fore + FVector(0.0, 0.08, 0.0), 0.8f);
		}
	}
	else
	{
		// Over the clips: the rain's hunch and the tired shoulders, and a lean into a turn.
		Lean = P.Tired * 4.0f;
		G.Rotate(B.Pelvis, GaitRoll(P.Bank * 0.3f));
		G.Rotate(B.Spine1, GaitPitch(-Sign * Lean));
		G.Rotate(B.Spine5, GaitPitch(Sign * P.Tired * 3.0f));
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const float Out = Side == 0 ? 1.0f : -1.0f;
			G.Rotate(B.Clavicle[Side], GaitRoll(Out * Sign * P.Tired * 4.0f));
		}
	}

	// Knocked off balance: the trunk thrown back, the hips back under it, the arms out (spec 6.5's bump at a run).
	if (Stumble > 0.001f)
	{
		G.Move(B.Pelvis, FVector(0.0, -5.0 * Stumble, -2.0 * Stumble));
		G.Rotate(B.Spine1, GaitPitch(Sign * 11.0f * Stumble));
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const float Out = Side == 0 ? 1.0f : -1.0f;
			G.Rotate(B.Clavicle[Side], GaitRoll(-Out * 5.0f * Stumble));
			G.Rotate(B.UpperArm[Side], GaitRoll(-Out * 22.0f * Stumble));
		}
	}

	// Seated (spec 6.6): the Back Room's solve (the pelvis onto the seat, the thighs along it, the shins down, the feet flat),
	// blended from the standing pose through a crouch over the character's 0.6 s.
	const float SeatB = FMath::SmoothStep(0.0f, 1.0f, FMath::Clamp(Motion.SeatBlend, 0.0f, 1.0f));
	if (SeatB > 0.001f && B.Pelvis != INDEX_NONE)
	{
		G.WriteBack();
		FPoseContext Standing(Output);
		Standing.Pose.CopyBonesFrom(Compact);
		G.Move(B.Pelvis, FVector(0.0, -3.0, Motion.SeatHeight + 9.0) - G.Pos(B.Pelvis));
		G.Rotate(B.Pelvis, GaitPitch(7.0f));
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const float Sx = Side == 0 ? 1.0f : -1.0f;
			G.Aim(B.Thigh[Side], B.Calf[Side], FVector(Sx * 0.16f, 1.0f, -0.2f));
			G.Aim(B.Calf[Side], B.Foot[Side], FVector(Sx * 0.04f, 0.12f, -1.0f));
			G.Aim(B.Foot[Side], B.Ball[Side], FVector(Sx * 0.14f, 0.87f, -0.48f));
		}
		G.Rotate(B.Spine1, GaitPitch(-4.0f));
		G.Rotate(B.Spine3, GaitPitch(-3.0f));
		if (!bBeat)
		{
			// Nothing in the hands: they rest on the thighs.
			for (int32 Side = 0; Side < 2; ++Side)
			{
				const float Sx = Side == 0 ? 1.0f : -1.0f;
				Reach(G, B.UpperArm[Side], B.LowerArm[Side], B.Hand[Side], FVector(Sx * 13.0, 26.0, Motion.SeatHeight + 17.0), FVector(Sx * 0.5, -0.4, -1.0), 1.0f);
				OrientHand(G, B, Side, FVector(-Sx * 0.15, 1.0, -0.25), FVector(0.0, 0.0, -1.0), 0.8f);
			}
		}
		G.WriteBack();
		if (SeatB < 0.999f)
		{
			for (FCompactPoseBoneIndex I : Compact.ForEachBoneIndex())
			{
				FTransform Blended;
				Blended.Blend(Standing.Pose[I], Compact[I], SeatB);
				Compact[I] = Blended;
			}
		}
		G.Refresh();
		// Through a crouch: the trunk forward over the knees halfway down (or up).
		const float Crouch = FMath::Sin(PI * SeatB) * (SeatB < 0.999f ? 1.0f : 0.0f);
		G.Rotate(B.Spine1, GaitPitch(-Sign * 24.0f * Crouch));
	}

	// The beat's trunk: a hinge at the hips toward the work.
	if (bBeat && !FMath::IsNearlyZero(Beat.Hinge))
	{
		G.Rotate(B.Spine1, GaitPitch(-Sign * Beat.Hinge * 0.6f));
		G.Rotate(B.Spine3, GaitPitch(-Sign * Beat.Hinge * 0.4f));
		Lean += Beat.Hinge;
	}

	// The head: steady against the trunk, turned toward where the character looks (relative to the actor: less the turn the
	// body hasn't caught up with). The neck takes a third of the turn and the head the rest (the eyes go further still); a
	// sip tips it back a little. Held for the barber, it looks straight ahead into the mirror.
	const float Hold = FMath::Max(Motion.HoldHead, Beat.HoldHead);
	const float WantYaw = FMath::Lerp(P.LookYaw - Motion.RootYaw, 0.0f, Hold);
	const float WantPitch = FMath::Lerp(P.LookPitch, -3.0f, Hold);
	const float LookYaw = FMath::Clamp(WantYaw, -70.0f, 70.0f) * 0.75f;
	const float LookPitch = FMath::Clamp(WantPitch, -40.0f, 35.0f) * 0.6f + 5.0f * Sip;
	G.Rotate(B.Neck, GaitYaw(LookYaw * 0.35f) * GaitPitch(Sign * (Lean * 0.7f + LookPitch * 0.3f)));
	G.Rotate(B.Head, GaitYaw(LookYaw * 0.65f) * GaitPitch(Sign * LookPitch * 0.7f));
	// How far the face has turned from its reference pose (the trunk, the neck and the head together): the mouth, and where
	// a sip holds the hand before it, turn with it.
	const FQuat FaceTurn = B.Head != INDEX_NONE ? G.CS[B.Head].GetRotation() * HeadRef.Inverse() : FQuat::Identity;
	const FVector Mouth = B.Head != INDEX_NONE ? G.Pos(B.Head) + FaceTurn.RotateVector(P.Mouth) : FVector::ZeroVector;

	// The beat's hands: each wrist to its mark in the station's frame (the palm's middle less a hand's length), the hand
	// turned to it, the fingers gripping.
	if (bBeat)
	{
		const FTransform& Frame = Motion.Station;
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const float W = Beat.Weight[Side];
			if (W <= 0.001f)
			{
				continue;
			}
			const FVector Fingers = Frame.TransformVectorNoScale(Beat.Fingers[Side]).GetSafeNormal();
			const FVector Palm = Frame.TransformVectorNoScale(Beat.Palm[Side]).GetSafeNormal();
			const FVector Target = Frame.TransformPosition(Beat.Hand[Side]) - Fingers * 7.0;
			Reach(G, B.UpperArm[Side], B.LowerArm[Side], B.Hand[Side], Target, Frame.TransformVectorNoScale(Beat.Pole[Side]), W);
			OrientHand(G, B, Side, Fingers, Palm, W);
		}
	}

	// A sip or a bite (or the umbrella's hand up under its canopy): the right hand to the mouth, the elbow down and out, the
	// fingers turned toward the lips with the palm in.
	if (Sip > 0.001f && B.Head != INDEX_NONE)
	{
		const int32 HandB = B.Hand[1];
		const int32 Middle = B.Fingers[1][1][0];
		const FVector Offset(-CVarSipSide.GetValueOnAnyThread(), CVarSipAhead.GetValueOnAnyThread(), CVarSipUp.GetValueOnAnyThread());
		const FVector Wrist = Mouth + FaceTurn.RotateVector(Offset);
		Reach(G, B.UpperArm[1], B.LowerArm[1], HandB, Wrist, FVector(-0.45, -0.25, -1.0), Sip);
		if (HandB != INDEX_NONE && Middle != INDEX_NONE)
		{
			const FVector FingerDir = (Mouth - G.Pos(HandB)).GetSafeNormal();
			const FVector Inward = FaceTurn.RotateVector(FVector(1.0, 0.0, 0.0));
			OrientHand(G, B, 1, FingerDir, Inward - FingerDir * FVector::DotProduct(Inward, FingerDir), Sip);
		}
	}

	// Caught out in the rain: the left hand up over the brow, palm down.
	const float OverHead = FMath::Clamp(P.HandOverHead, 0.0f, 1.0f);
	if (OverHead > 0.001f && B.Head != INDEX_NONE)
	{
		const FVector Brow = G.Pos(B.Head) + FaceTurn.RotateVector(FVector(3.0, 8.0, 14.0));
		Reach(G, B.UpperArm[0], B.LowerArm[0], B.Hand[0], Brow - FaceTurn.RotateVector(FVector(-6.0, 0.0, 0.0)), FVector(1.0, -0.2, 0.2), OverHead);
		OrientHand(G, B, 0, FaceTurn.RotateVector(FVector(-1.0, 0.25, 0.0)), FVector(0.0, 0.0, -1.0), OverHead);
	}

	// Fingers. The procedural gait curls them (a relaxed curl, a little tighter at a run); the clips have their own, and only
	// what the hands hold now closes them further.
	for (int32 Side = 0; Side < 2; ++Side)
	{
		float Curl = bClips ? 0.0f : (0.28f + 0.2f * Run * Moving) * CurlK;
		float ThumbCurl = bClips ? 0.0f : 0.18f * CurlK;
		float Pinch = 0.0f;
		if (bBeat && Beat.Weight[Side] > 0.001f)
		{
			const float Base = bClips ? 0.3f : 0.0f; // the clips' hands are already loosely curled
			Curl = FMath::Lerp(Curl, FMath::Max(0.0f, Beat.Curl[Side] - Base), Beat.Weight[Side]);
			ThumbCurl = FMath::Lerp(ThumbCurl, FMath::Max(0.0f, Beat.Curl[Side] * 0.6f - Base * 0.6f), Beat.Weight[Side]);
			Pinch = Beat.Pinch[Side] * Beat.Weight[Side];
		}
		if (Side == 1 && Sip > 0.001f)
		{
			Curl = FMath::Lerp(Curl, bClips ? 0.32f : 0.62f, Sip);
			ThumbCurl = FMath::Lerp(ThumbCurl, bClips ? 0.3f : 0.5f, Sip);
		}
		if (Side == 0 && OverHead > 0.001f)
		{
			Curl = FMath::Lerp(Curl, 0.1f, OverHead);
		}
		if (Curl > 0.001f || ThumbCurl > 0.001f || Pinch > 0.001f)
		{
			CurlHand(G, B, Side, Curl, ThumbCurl, Pinch);
		}
	}

	G.WriteBack();
	return true;
}
