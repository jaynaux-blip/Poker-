// ABackRoomPlayer's contact with the table.
//
// The contact itself is solved where the body is posed (UBackRoomBodyAnim's evaluation, BackRoomAnim.cpp), in the same
// frame as the pose and with the arms' real geometry: the belly may press the rail's skirt and no further, the elbow
// swings clear of the rail's crown, and each ball of the hands (fingertip, joint, knuckle, palm, wrist, forearm) that
// has sunk into the felt or the rail's surface (ABackRoomStage::TableContact) lifts the hand (and, past a few
// centimeters, pitches the fingers up about the wrist) before the pose is drawn. A touch (a fingertip resting on the
// felt, a forearm lying on the rail) is the same thing at zero depth, with a hair of air between skin and surface.
// What holds a hand off the table is carried from frame to frame (so a hand resting on it settles there and nothing
// pops), and a hand going between the lap and the table goes round the rail's edge (RouteHand, BackRoomPlayerHands.cpp)
// instead of through it.
//
// What is here is the audit: the posed skeleton read back every frame (when ss.ContactDebug is on) against the
// table's solid: the deepest sink-in and how many frames sank more than a centimeter, and the hands that moved faster
// than a hand can between two frames, so it can be seen that nothing goes through the table or snaps.

#include "BackRoomAnim.h"
#include "BackRoomPlayer.h"
#include "BackRoomPlayerShared.h"
#include "BackRoomStage.h"
#include "Components/SkeletalMeshComponent.h"
#include "HAL/IConsoleManager.h"

using namespace BackRoomPlayerDetail;

namespace BackRoomContactDetail
{
TAutoConsoleVariable<int32> CVarContact(TEXT("ss.Contact"), 1, TEXT("1: hands, forearms and torsos are posed so they stay out of the table's solid."), ECVF_Default);
TAutoConsoleVariable<int32> CVarContactDebug(TEXT("ss.ContactDebug"), 0, TEXT("1: log the deepest sink-in of each player's hands once a second."), ECVF_Default);
} // namespace BackRoomContactDetail

using namespace BackRoomContactDetail;

bool ABackRoomPlayer::ContactEnabled()
{
	return CVarContact.GetValueOnAnyThread() != 0;
}

FVector ABackRoomPlayer::ContactShift(int32 Side) const
{
	if (UBackRoomBodyAnim* Anim = Body ? Cast<UBackRoomBodyAnim>(Body->GetAnimInstance()) : nullptr)
	{
		return Anim->GetResolvedShift(Side);
	}
	return FVector::ZeroVector;
}

float ABackRoomPlayer::ContactPitch(int32 Side) const
{
	if (UBackRoomBodyAnim* Anim = Body ? Cast<UBackRoomBodyAnim>(Body->GetAnimInstance()) : nullptr)
	{
		return Anim->GetResolvedPitch(Side);
	}
	return 0.0f;
}

bool ABackRoomPlayer::BuildContactSamples()
{
	if (!Body || !Body->GetSkeletalMeshAsset())
	{
		return false;
	}
	auto Bone = [this](const FString& Name) { return Body->GetBoneIndex(FName(*Name)); };
	auto Add = [](TArray<FContactSample>& To, int32 A, float Radius, int32 B = INDEX_NONE, float Mix = 0.0f, float Extend = 0.0f) {
		if (A == INDEX_NONE || (B == INDEX_NONE && (Mix != 0.0f || Extend != 0.0f)))
		{
			return;
		}
		FContactSample S;
		S.Bone = A;
		S.Bone2 = B;
		S.Mix = Mix;
		S.Extend = Extend;
		S.Radius = Radius;
		To.Add(S);
	};
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const FString Sd = Side == 0 ? TEXT("l") : TEXT("r");
		TArray<FContactSample>& Out = HandSamples[Side];
		Out.Reset();
		const int32 Hand = Bone(TEXT("hand_") + Sd);
		const int32 Lower = Bone(TEXT("lowerarm_") + Sd);
		Add(Out, Hand, 2.4f);
		Add(Out, Lower, 3.0f);
		Add(Out, Lower, 2.8f, Hand, 0.5f);
		Add(Out, Hand, 2.0f, Bone(TEXT("middle_01_") + Sd), 0.5f);
		for (const TCHAR* Finger : {TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky")})
		{
			const int32 B1 = Bone(FString::Printf(TEXT("%s_01_%s"), Finger, *Sd));
			const int32 B2 = Bone(FString::Printf(TEXT("%s_02_%s"), Finger, *Sd));
			const int32 B3 = Bone(FString::Printf(TEXT("%s_03_%s"), Finger, *Sd));
			Add(Out, B1, 1.3f);
			Add(Out, B2, 1.0f);
			Add(Out, B3, 0.65f, B2, 0.0f, 2.0f);
		}
		const int32 T1 = Bone(TEXT("thumb_01_") + Sd), T2 = Bone(TEXT("thumb_02_") + Sd), T3 = Bone(TEXT("thumb_03_") + Sd);
		Add(Out, T1, 1.4f);
		Add(Out, T2, 1.0f);
		Add(Out, T3, 0.65f, T2, 0.0f, 2.0f);
	}
	return HandSamples[0].Num() > 4 && HandSamples[1].Num() > 4;
}

FVector ABackRoomPlayer::ContactAt(const FContactSample& S) const
{
	const FVector A = Body->GetBoneTransform(S.Bone).GetLocation();
	if (S.Bone2 == INDEX_NONE)
	{
		return A;
	}
	const FVector B = Body->GetBoneTransform(S.Bone2).GetLocation();
	// A tip: the last joint carried on along the finger (the bone ends where the pad begins).
	return S.Extend != 0.0f ? A + (A - B).GetSafeNormal() * S.Extend : FMath::Lerp(A, B, static_cast<double>(S.Mix));
}

void ABackRoomPlayer::AuditContacts(float Dt)
{
	if (CVarContactDebug.GetValueOnGameThread() == 0 || !Body || !Body->GetSkeletalMeshAsset() || IsHidden())
	{
		return;
	}
	if (!bContactReady)
	{
		bContactReady = BuildContactSamples();
		if (!bContactReady)
		{
			return;
		}
	}
	// Pops: a wrist or fingertip that travels faster than a hand can (3 m/s) between two frames.
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const FString Sd = Side == 0 ? TEXT("l") : TEXT("r");
		const int32 Probe[2] = {Body->GetBoneIndex(FName(*(TEXT("hand_") + Sd))), Body->GetBoneIndex(FName(*(TEXT("middle_03_") + Sd)))};
		for (int32 K = 0; K < 2; ++K)
		{
			if (Probe[K] == INDEX_NONE)
			{
				continue;
			}
			const FVector Now = Body->GetBoneTransform(Probe[K]).GetLocation();
			const double Speed = bLastProbe && Dt > 1.0e-4f ? FVector::Dist(Now, LastProbe[Side][K]) / Dt : 0.0;
			if (Speed > 300.0)
			{
				UE_LOG(LogTemp, Display, TEXT("POP %s %s(%s) %.0f cm/s (%.1f cm in %.3f s) mode %d steps %d/%d t=%.2f"), *Persona.Name, K == 0 ? TEXT("hand") : TEXT("fingertip"), Side == 0 ? TEXT("L") : TEXT("R"), Speed, Speed * Dt, Dt, HandMode, Steps[0].Num(), Steps[1].Num(), GetWorld()->GetTimeSeconds());
			}
			LastProbe[Side][K] = Now;
		}
	}
	bLastProbe = true;
	// The deepest any ball of the posed hands has sunk since the last report (a frame's reading can be a touch on a
	// fast move; what shows is how many frames sink, and how deep the worst of a second goes).
	float FrameWorst = 0.0f;
	for (int32 Side = 0; Side < 2; ++Side)
	{
		for (const FContactSample& S : HandSamples[Side])
		{
			FVector Push;
			float Clear;
			if (ABackRoomStage::TableContact(TableToWorld.InverseTransformPosition(ContactAt(S)), S.Radius, true, Push, Clear) && -Clear > FrameWorst)
			{
				FrameWorst = -Clear;
				if (FrameWorst > ContactWorst)
				{
					ContactWorst = FrameWorst;
					ContactWorstName = FString::Printf(TEXT("%s(%s)"), *Body->GetBoneName(S.Bone).ToString(), Side == 0 ? TEXT("L") : TEXT("R"));
				}
			}
		}
	}
	++ContactFrames;
	ContactBad1 += FrameWorst > 1.0f ? 1 : 0;
	ContactBad2 += FrameWorst > 2.0f ? 1 : 0;
	ContactLogT += Dt;
	if (ContactLogT > 1.0f)
	{
		ContactLogT = 0.0f;
		UE_LOG(LogTemp, Display, TEXT("CONTACT %s [mode %d steps %d/%d]: deepest sink %.2f cm at %s, frames sunk over 1 cm %d and over 2 cm %d of %d | hands moved off the table L %.1f R %.1f cm | the pose's last pass still found L %.2f R %.2f"), *Persona.Name, HandMode, Steps[0].Num(), Steps[1].Num(), ContactWorst, *ContactWorstName, ContactBad1, ContactBad2, ContactFrames,
			ContactShift(0).Size(), ContactShift(1).Size(), Cast<UBackRoomBodyAnim>(Body->GetAnimInstance())->GetResolvedWorst(0), Cast<UBackRoomBodyAnim>(Body->GetAnimInstance())->GetResolvedWorst(1));
		ContactWorst = 0.0f;
		ContactWorstName.Reset();
		ContactFrames = ContactBad1 = ContactBad2 = 0;
	}
}
