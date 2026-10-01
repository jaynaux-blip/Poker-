#include "FirstPersonArms.h"

#include "Components/PoseableMeshComponent.h"
#include "Components/SkinnedMeshComponent.h"

namespace FirstPersonArmsDetail
{
const TCHAR* FingerNames[] = {TEXT("thumb"), TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky")};

/** Rotation whose X axis is Fwd and Z axis is Up. */
FQuat Basis(const FVector& Fwd, const FVector& Up)
{
	return FRotationMatrix::MakeFromXZ(Fwd, Up).ToQuat();
}

FVector Ortho(const FVector& V, const FVector& To)
{
	return (V - To * FVector::DotProduct(V, To)).GetSafeNormal();
}

/** 0 -> 1 -> 0 over [0, Duration], shaped like a key press: quick down, softer release. */
float Press(float T, float Duration)
{
	if (T < 0.0f || T > Duration)
	{
		return 0.0f;
	}
	const float X = T / Duration;
	return X < 0.35f ? FMath::Sin(X / 0.35f * PI * 0.5f) : FMath::Cos((X - 0.35f) / 0.65f * PI * 0.5f);
}
} // namespace FirstPersonArmsDetail

using namespace FirstPersonArmsDetail;

FTransform FFirstPersonArms::RefWorld(const FTransform& RefCS) const
{
	return RefCS * Mesh->GetComponentTransform();
}

bool FFirstPersonArms::InitSide(FSide& S, const TCHAR* Suffix, float Sign)
{
	S.Sign = Sign;
	S.Upper = FName(*FString::Printf(TEXT("upperarm_%s"), Suffix));
	S.Lower = FName(*FString::Printf(TEXT("lowerarm_%s"), Suffix));
	S.Hand = FName(*FString::Printf(TEXT("hand_%s"), Suffix));
	for (const FName& B : {S.Upper, S.Lower, S.Hand})
	{
		if (Mesh->GetBoneIndex(B) == INDEX_NONE)
		{
			return false;
		}
	}
	S.UpperCS = Mesh->GetBoneTransformByName(S.Upper, EBoneSpaces::ComponentSpace);
	S.LowerCS = Mesh->GetBoneTransformByName(S.Lower, EBoneSpaces::ComponentSpace);
	S.HandCS = Mesh->GetBoneTransformByName(S.Hand, EBoneSpaces::ComponentSpace);
	// In the reference pose the hands lie palm down, so the back of the hand faces the component's up.
	const FVector UpCS(0.0, 0.0, 1.0);
	for (int32 F = 0; F < FingerCount; ++F)
	{
		FFinger& Fi = S.Fingers[F];
		for (int32 K = 0; K < 3; ++K)
		{
			Fi.Bones[K] = FName(*FString::Printf(TEXT("%s_0%d_%s"), FingerNames[F], K + 1, Suffix));
			if (Mesh->GetBoneIndex(Fi.Bones[K]) == INDEX_NONE)
			{
				return false;
			}
			Fi.RefCS[K] = Mesh->GetBoneTransformByName(Fi.Bones[K], EBoneSpaces::ComponentSpace);
		}
		const FVector Dir = (Fi.RefCS[2].GetLocation() - Fi.RefCS[0].GetLocation()).GetSafeNormal();
		FVector Axis = FVector::CrossProduct(UpCS, Dir).GetSafeNormal();
		// Choose the sign that curls toward the palm (down), whatever handedness the import left.
		const FVector Turned = FQuat(Axis, 0.1f).RotateVector(Dir);
		if (FVector::DotProduct(Turned - Dir, -UpCS) < 0.0)
		{
			Axis = -Axis;
		}
		Fi.AxisCS = Axis;
		Fi.TipLength = 0.8f * static_cast<float>(FVector::Dist(Fi.RefCS[2].GetLocation(), Fi.RefCS[1].GetLocation()));
		S.TipLocal[F] = FVector::ZeroVector;
	}
	S.HandFwdCS = (S.Fingers[Middle].RefCS[0].GetLocation() - S.HandCS.GetLocation()).GetSafeNormal();
	S.HandUpCS = Ortho(UpCS, S.HandFwdCS);
	return true;
}

bool FFirstPersonArms::Init(UPoseableMeshComponent* InMesh)
{
	Mesh = InMesh;
	bReady = false;
	if (!Mesh || Mesh->GetNumBones() == 0)
	{
		return false;
	}
	if (!InitSide(Left, TEXT("l"), -1.0f) || !InitSide(Right, TEXT("r"), 1.0f))
	{
		return false;
	}
	// Resting curls on top of the modeled pose: the left hand poised over the keys, the right one easy on the mouse.
	const float LeftRest[FingerCount][3] = {{6, 8, 8}, {20, 30, 12}, {18, 34, 14}, {22, 36, 14}, {26, 36, 12}};
	const float RightRest[FingerCount][3] = {{4, 6, 6}, {6, 12, 6}, {6, 14, 8}, {14, 26, 12}, {18, 30, 12}};
	FMemory::Memcpy(Left.Rest, LeftRest, sizeof(LeftRest));
	FMemory::Memcpy(Right.Rest, RightRest, sizeof(RightRest));
	bReady = true;
	return true;
}

void FFirstPersonArms::Key(const FString& Name, const FVector& KeyWorld)
{
	// Touch-typing fingers for the game's hotkeys; the arrows and M are played by the mouse hand.
	int32 Finger = Index;
	FSide* S = &Left;
	if (Name == TEXT("a"))
	{
		Finger = Pinky;
	}
	else if (Name == TEXT("x"))
	{
		Finger = Ring;
	}
	else if (Name == TEXT("c"))
	{
		Finger = Middle;
	}
	else if (Name == TEXT(" "))
	{
		Finger = Thumb;
	}
	else if (Name == TEXT("ArrowUp") || Name == TEXT("ArrowDown") || Name == TEXT("m"))
	{
		S = &Right;
		Finger = Name == TEXT("m") ? Index : Middle;
	}
	S->TapFinger = Finger;
	S->TapTime = 0.0f;
	S->KeyWorld = KeyWorld;
}

void FFirstPersonArms::Click()
{
	Right.ClickTime = 0.0f;
}

void FFirstPersonArms::Solve(FSide& S, const FVector& WristTarget, const FVector& HandFwd, const FVector& HandUp, const FVector& Up, float Dt,
	const float Extra[FingerCount][3])
{
	// Wrist: follow the target with a quick, damped ease so reaches read as motion, not teleports.
	if (!S.bHasWrist)
	{
		S.Wrist = WristTarget;
		S.bHasWrist = true;
	}
	S.Wrist += (WristTarget - S.Wrist) * (1.0f - FMath::Exp(-Dt * 22.0f));

	const FTransform UpperW = RefWorld(S.UpperCS);
	const FTransform LowerW = RefWorld(S.LowerCS);
	const FTransform HandW = RefWorld(S.HandCS);
	const FVector Shoulder = UpperW.GetLocation();
	const FVector Elbow0 = LowerW.GetLocation();
	const FVector Wrist0 = HandW.GetLocation();
	const float L1 = static_cast<float>(FVector::Dist(Shoulder, Elbow0));
	const float L2 = static_cast<float>(FVector::Dist(Elbow0, Wrist0));

	// Two-bone IK: the elbow swings out and down, as it does with hands on a desk.
	const FVector Side = FVector::CrossProduct(Up, HandFwd).GetSafeNormal();
	const FVector ToWrist = S.Wrist - Shoulder;
	const float Reach = FMath::Clamp(static_cast<float>(ToWrist.Size()), FMath::Abs(L1 - L2) + 1.0f, (L1 + L2) * 0.995f);
	const FVector Dir = ToWrist.GetSafeNormal();
	const float A = (L1 * L1 - L2 * L2 + Reach * Reach) / (2.0f * Reach);
	const float H = FMath::Sqrt(FMath::Max(L1 * L1 - A * A, 0.0f));
	const FVector Pole = Ortho(Side * S.Sign * 0.75 - Up * 0.65, Dir);
	const FVector Elbow = Shoulder + Dir * A + Pole * H;
	const FVector Wrist = Shoulder + Dir * Reach;

	FTransform T = UpperW;
	T.SetRotation(FQuat::FindBetweenVectors(Elbow0 - Shoulder, Elbow - Shoulder) * UpperW.GetRotation());
	Mesh->SetBoneTransformByName(S.Upper, T, EBoneSpaces::WorldSpace);
	T = Mesh->GetBoneTransformByName(S.Lower, EBoneSpaces::WorldSpace);
	T.SetRotation(FQuat::FindBetweenVectors(Wrist0 - Elbow0, Wrist - Elbow) * LowerW.GetRotation());
	Mesh->SetBoneTransformByName(S.Lower, T, EBoneSpaces::WorldSpace);

	// Hand: turn the reference frame (wrist to knuckles, back of the hand) onto the target frame.
	const FQuat CompRot = Mesh->GetComponentTransform().GetRotation();
	const FVector FwdRef = CompRot.RotateVector(S.HandFwdCS);
	const FVector UpRef = CompRot.RotateVector(S.HandUpCS);
	const FQuat Turn = Basis(HandFwd, Ortho(HandUp, HandFwd)) * Basis(FwdRef, UpRef).Inverse();
	T = Mesh->GetBoneTransformByName(S.Hand, EBoneSpaces::WorldSpace);
	T.SetRotation(Turn * HandW.GetRotation());
	Mesh->SetBoneTransformByName(S.Hand, T, EBoneSpaces::WorldSpace);

	// Fingers: each phalanx turns with the hand, then curls about the finger's own axis.
	const FQuat Frame = Basis(HandFwd, Ortho(HandUp, HandFwd)); // the hand's target frame, as Update uses it
	for (int32 F = 0; F < FingerCount; ++F)
	{
		FFinger& Fi = S.Fingers[F];
		const FVector Axis = Turn.RotateVector(CompRot.RotateVector(Fi.AxisCS));
		float Curl = 0.0f;
		FTransform Last;
		for (int32 K = 0; K < 3; ++K)
		{
			Curl += FMath::DegreesToRadians(S.Rest[F][K] + Extra[F][K]);
			FTransform B = Mesh->GetBoneTransformByName(Fi.Bones[K], EBoneSpaces::WorldSpace);
			B.SetRotation(FQuat(Axis, Curl) * Turn * RefWorld(Fi.RefCS[K]).GetRotation());
			Mesh->SetBoneTransformByName(Fi.Bones[K], B, EBoneSpaces::WorldSpace);
			Last = Mesh->GetBoneTransformByName(Fi.Bones[K], EBoneSpaces::WorldSpace);
		}
		// Where this fingertip ended up, relative to the wrist in hand space, for the next reach.
		const FVector Prev = Mesh->GetBoneTransformByName(Fi.Bones[1], EBoneSpaces::WorldSpace).GetLocation();
		const FVector Tip = Last.GetLocation() + (Last.GetLocation() - Prev).GetSafeNormal() * Fi.TipLength;
		S.TipLocal[F] = Frame.UnrotateVector(Tip - T.GetLocation());
	}
}

void FFirstPersonArms::Update(float Dt, const FTargets& Tg)
{
	if (!bReady || !Mesh)
	{
		return;
	}
	Clock += Dt;
	const FVector Fwd = Tg.Forward.GetSafeNormal();
	const FVector Up = Tg.Up.GetSafeNormal();
	const FVector Rt = FVector::CrossProduct(Up, Fwd).GetSafeNormal();
	// A hand at rest is never quite still.
	auto Drift = [this](float Seed) {
		return FVector(FMath::Sin(Clock * 0.9f + Seed) * 0.12f, FMath::Sin(Clock * 0.7f + Seed * 2.0f) * 0.12f, FMath::Sin(Clock * 1.1f + Seed * 3.0f) * 0.08f);
	};

	// Left hand: the palm rest, or reaching to put the chosen fingertip on the key.
	{
		FSide& S = Left;
		S.TapTime += Dt;
		const FVector HandFwd = (Fwd + Rt * 0.2f - Up * 0.18f).GetSafeNormal();
		const FVector HandUp = (Up - Rt * 0.25f).GetSafeNormal();
		const FQuat HandRot = Basis(HandFwd, Ortho(HandUp, HandFwd));
		FVector Target = Tg.LeftRest + Drift(0.0f);
		float Extra[FingerCount][3] = {};
		const float Hold = 0.34f;
		if (S.TapFinger >= 0 && S.TapTime < Hold + 0.3f)
		{
			// Reach in 0.1 s, tap, then settle back over 0.3 s.
			const float Reach = S.TapTime < Hold ? FMath::SmoothStep(0.0f, 0.1f, S.TapTime) : 1.0f - FMath::SmoothStep(Hold, Hold + 0.3f, S.TapTime);
			const FVector OnKey = S.KeyWorld - HandRot.RotateVector(S.TipLocal[S.TapFinger]) + Up * 0.4f;
			Target = FMath::Lerp(Target, OnKey, Reach);
			const float Down = Press(S.TapTime - 0.08f, 0.16f);
			const float Lift = Press(S.TapTime, 0.1f) * (1.0f - Down);
			Extra[S.TapFinger][0] = Down * 14.0f - Lift * 16.0f;
			Extra[S.TapFinger][1] = Down * 6.0f - Lift * 8.0f;
			Target -= Up * (Down * 0.35f);
		}
		Solve(S, Target, HandFwd, HandUp, Up, Dt, Extra);
	}

	// Right hand: on the mouse, reaching for the arrows or M when they are pressed.
	{
		FSide& S = Right;
		S.TapTime += Dt;
		S.ClickTime += Dt;
		// The palm rides the mouse's back: the wrist sits behind and a little below its top, the hand
		// tipped up over it, the fingers curling down onto the buttons.
		const FVector HandFwd = (Fwd - Rt * 0.12f + Up * 0.12f).GetSafeNormal();
		const FVector HandUp = (Up + Rt * 0.18f).GetSafeNormal();
		const FQuat HandRot = Basis(HandFwd, Ortho(HandUp, HandFwd));
		FVector Target = Tg.Mouse - Fwd * 6.5f - Up * 0.3f + Drift(2.0f) * 0.5f;
		float Extra[FingerCount][3] = {};
		const float Click = Press(S.ClickTime, 0.14f);
		Extra[Index][0] = Click * 7.0f;
		Extra[Index][1] = Click * 3.0f;
		const float Hold = 0.4f;
		if (S.TapFinger >= 0 && S.TapTime < Hold + 0.35f)
		{
			const float Reach = S.TapTime < Hold ? FMath::SmoothStep(0.0f, 0.16f, S.TapTime) : 1.0f - FMath::SmoothStep(Hold, Hold + 0.35f, S.TapTime);
			const FVector OnKey = S.KeyWorld - HandRot.RotateVector(S.TipLocal[S.TapFinger]) + Up * 0.4f;
			Target = FMath::Lerp(Target, OnKey, Reach);
			const float Down = Press(S.TapTime - 0.12f, 0.16f);
			Extra[S.TapFinger][0] += Down * 14.0f;
			Extra[S.TapFinger][1] += Down * 6.0f;
			Target -= Up * (Down * 0.35f);
		}
		Solve(S, Target, HandFwd, HandUp, Up, Dt, Extra);
	}
}
