// ABackRoomPlayer's hands: a queue of moves per hand, and the gestures of the game built from them.
//
// Hand poses are in the body mesh's component space (+Y toward the table, +X the player's left, +Z up;
// side 0 is the left hand). Pos is the wrist; the fingertips are about 17 cm along Finger.

#include "BackRoomCard.h"
#include "BackRoomChips.h"
#include "BackRoomPlayer.h"
#include "BackRoomPlayerShared.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "ShortStack/Game/Session.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"

using namespace BackRoomPlayerDetail;

namespace BackRoomHandsDetail
{
/** Wrist to fingertip, by how curled the fingers are. */
double Reach(float Curl)
{
	return 17.5 * (1.0 - 0.4 * Curl);
}

/** The deck's thickness (cm). */
const double DeckThickness = 1.7;

/**
 * Where a hand goes first on its way from From to Goal (the body's space), so that it goes round the table and not
 * through it. The fingers reach a hand's length ahead of the wrist, so a hand beside the body can only go down
 * (to the lap) or come up once the wrist is well back from the rail's edge: a hand coming from the lap backs out
 * from under, rises clear of the rail's crown and comes in over it; a hand going to the lap rises clear of the
 * crown, backs out above the rail's edge and goes down beside the belly. Goal itself when the way is clear.
 */
FVector RouteHand(int32 Side, const FVector& From, const FVector& Goal)
{
	const double Edge = Rail - 3.0;  // the wrist is over the table's plan beyond this (the rail's outer edge less its own size)
	const double Clear = 84.0;       // and clear of the rail's crown above this
	const double Out = Rail - 18.0;  // backed out to here, the fingers are clear of the rail
	const double Back = Out + 2.0;
	const double Beside = (Side == 0 ? 1.0 : -1.0) * 22.0;
	if (Goal.Y > Edge)
	{
		// To the table: along it, or in over the crown from clear of it.
		if (From.Y > Edge || From.Z >= Clear - 0.5)
		{
			return Goal;
		}
		return From.Y > Back ? FVector(Beside, Out, From.Z) : FVector(From.X, From.Y, Clear + 2.0);
	}
	// To the back (the lap, the face): free once back there, or high above the rail the whole way.
	if (From.Y <= FMath::Max(Back, Goal.Y + 1.0) || (From.Z >= Clear - 0.5 && Goal.Z >= Clear))
	{
		return Goal;
	}
	return From.Z < Clear - 0.5 ? FVector(From.X, From.Y, Clear + 2.0) : FVector(Beside, Out, From.Z);
}

int32 Snd(ss::SoundId Id)
{
	return static_cast<int32>(Id);
}

/** The fastest a hand sweeps through a point on its way (cm/s). */
const double MaxPass = 150.0;

/**
 * A critically damped spring: X chases Goal with velocity V and settles in about 4 / Omega seconds, starting
 * and stopping without a jolt from wherever it is, steady at any frame rate.
 */
template <typename T>
void Spring(T& X, T& V, const T& Goal, float Omega, float Dt)
{
	const float W = Omega * Dt;
	const float Decay = 1.0f / (1.0f + W + 0.48f * W * W + 0.235f * W * W * W);
	const T Change = X - Goal;
	const T Carry = (V + Change * Omega) * Dt;
	V = (V - Carry * Omega) * Decay;
	X = Goal + (Change + Carry) * Decay;
}

/** A direction, or the last one if a blend between opposites shrinks it to nothing. */
FVector Dir(const FVector& V, const FVector& Fallback)
{
	return V.SizeSquared() > 0.01 ? V.GetUnsafeNormal() : Fallback;
}
} // namespace BackRoomHandsDetail

using namespace BackRoomHandsDetail;

static TAutoConsoleVariable<float> CVarRestY(TEXT("ss.RestY"), 7.0f, TEXT("Resting wrist: cm past the rail's outer edge."));
static TAutoConsoleVariable<float> CVarRestZ(TEXT("ss.RestZ"), 85.0f, TEXT("Resting wrist: height."));
static TAutoConsoleVariable<float> CVarRestFingerZ(TEXT("ss.RestFingerZ"), -0.30f, TEXT("Resting fingers: how far they point down (per unit forward)."));

// ------------------------------------------------------------------ poses and the queue

ABackRoomPlayer::FHandPose ABackRoomPlayer::RestPose(int32 Side) const
{
	const float Sx = Side == 0 ? 1.0f : -1.0f;
	FHandPose P;
	if (SeatRole == EBackRoomRole::Dealer)
	{
		if (Side == 0)
		{
			// The deck in the left hand, palm up, fingers wrapped around its far side.
			P.Pos = FVector(10.0, Rail + 10.0, 83.0);
			P.Palm = FVector(-0.25, 0.15, 1.0);
			P.Finger = FVector(-1.0, 0.55, 0.05);
			P.Curl = 0.5f;
			P.Thumb = 0.15f;
			return P;
		}
		P.Pos = FVector(-12.0, Rail + 12.0, 81.0);
		P.Palm = FVector(0.1, 0.0, -1.0);
		P.Finger = FVector(0.35, 1.0, -0.1);
		P.Curl = 0.4f;
		return P;
	}
	// Forearms across the rail, the wrists at its inner edge and the hands loose on the felt beyond.
	P.Pos = FVector(Sx * 9.0, Rail + CVarRestY.GetValueOnAnyThread(), CVarRestZ.GetValueOnAnyThread());
	P.Palm = FVector(Sx * 0.15f, 0.0, -1.0);
	P.Finger = FVector(-Sx * 0.45f, 1.0, CVarRestFingerZ.GetValueOnAnyThread());
	P.Curl = 0.35f + 0.1f * Persona.Nervousness;
	P.Thumb = 0.2f;
	return P;
}

ABackRoomPlayer::FHandPose ABackRoomPlayer::IdleGoal(int32 Side) const
{
	FHandPose Goal = RestPose(Side);
	const float Sx = Side == 0 ? 1.0f : -1.0f;
	if (SeatRole == EBackRoomRole::Hero)
	{
		if (bHeroPeek && Hole.Num() == 2 && Hole[0] && Hole[1])
		{
			// Peeking: one hand lies over the cards, the other takes the corner between thumb and finger and lifts it.
			const int32 Lifter = PeekHand();
			if (Side != Lifter)
			{
				return PeekCoverPose(Side);
			}
			// The fingers close as they arrive at the corner, and only then does it come up.
			const float Near = static_cast<float>(FVector::Dist(TipBody(Side), ToBody(FingerPathAt(HeroLift))));
			return PeekGripPose(Side, HeroLift, 0.12f + 0.62f * (1.0f - FMath::SmoothStep(0.8f, 3.5f, Near)));
		}
		return Goal;
	}
	if (SeatRole != EBackRoomRole::Player)
	{
		return Goal;
	}
	switch (HandMode)
	{
	case 1:
		if (Side == 1)
		{
			if (HasPlayChips())
			{
				// Over the column to play with: thumb on the near side, middle finger on the far side, index on top.
				// (The Touch's fingertip estimate runs about 4 cm toward the body's middle of where a curled hand's
				// fingers land: aimed that far to the player's right of the column.)
				Goal = Touch(PlayChipsBase() + FVector(0.0, 0.0, TrickTop + 0.3) + Spots.Inward * 1.4 + GetActorRightVector() * 4.0, FVector(-0.15, 1.0, -0.6),
					FVector(0.3, 0.0, -1.0), 0.55f, 0.35f);
			}
			else if (StackPile && StackPile->GetAmount() > 0)
			{
				// Fingers on top of the stack, toying with it.
				Goal = Touch(StackPile->GetTop() + FVector(0.0, 0.0, 0.4), FVector(-0.2, 1.0, -0.35), FVector(0.3, 0.0, -1.0), 0.45f, 0.4f);
			}
		}
		break;
	case 2:
		if (Side == 0 && bInHand && Hole.Num() > 0 && Hole[0])
		{
			// A hand guarding the cards, resting beside them.
			Goal = Touch(Hole[0]->GetActorLocation() + GetActorRightVector() * -5.0 + FVector(0.0, 0.0, 0.5), FVector(-0.35, 1.0, -0.2), FVector(0.2, 0.0, -1.0), 0.5f);
		}
		break;
	case 3:
		if (Side == HabitSide)
		{
			// Chin on a fist, the elbow out on the table: the fist's knuckles under the chin, the palm to the face.
			const FVector Chin = ToBody(GetEyes()) + FVector(0.0, 0.5, -11.5);
			Goal.Finger = FVector(0.0, 0.2, 1.0).GetSafeNormal();
			Goal.Palm = FVector(Sx * 0.25, -1.0, 0.0).GetSafeNormal();
			Goal.Curl = 0.85f;
			Goal.Thumb = 0.55f;
			Goal.Pinch = 0.0f;
			Goal.Pos = Chin - Goal.Finger * Reach(Goal.Curl) - Goal.Palm * 1.6;
		}
		break;
	case 4:
		// Hands folded on the felt in front, the right over the left, fingers across the other hand.
		Goal.Pos = FVector(Sx * 5.0, Rail + 8.0, Side == 1 ? 86.2 : 85.0);
		Goal.Finger = FVector(-Sx * 1.0, 0.45, -0.15).GetSafeNormal();
		Goal.Palm = FVector(-Sx * 0.25, 0.0, -1.0).GetSafeNormal();
		Goal.Curl = 0.55f;
		Goal.Thumb = 0.3f;
		break;
	case 5:
		// Sat back, the hands in the lap.
		Goal.Pos = FVector(Sx * 8.5, 17.0, 65.0);
		Goal.Finger = FVector(-Sx * 0.55, 1.0, -0.2).GetSafeNormal();
		Goal.Palm = FVector(Sx * 0.1, 0.0, -1.0).GetSafeNormal();
		Goal.Curl = 0.45f;
		Goal.Thumb = 0.25f;
		break;
	case 6:
		if (Side == HabitSide)
		{
			// The forearm laid along the rail's crown, the weight on it.
			Goal.Pos = FVector(Sx * 3.0, Rail + 5.0, 85.0);
			Goal.Finger = FVector(-Sx * 1.0, 0.3, -0.15).GetSafeNormal();
			Goal.Palm = FVector(0.0, 0.1, -1.0).GetSafeNormal();
			Goal.Curl = 0.5f;
		}
		break;
	default:
		break;
	}
	return Goal;
}

void ABackRoomPlayer::PickHabit(bool bDeciding)
{
	if (SeatRole != EBackRoomRole::Player)
	{
		HandMode = 0;
		return;
	}
	// Each persona has their own ways of sitting: fidgets play with chips, the composed fold their hands or
	// rest a chin, sprawlers sit back; nobody holds one for long.
	const float Calm = 1.0f - Persona.Nervousness;
	const bool bChips = HasPlayChips() || (StackPile && StackPile->GetAmount() > 0);
	float W[7] = {};
	if (bDeciding)
	{
		W[0] = 0.3f;
		W[1] = bChips ? 0.5f + 0.4f * Persona.ChipFidget : 0.0f;
		W[3] = 0.35f * Calm;
	}
	else
	{
		W[0] = 0.35f;
		W[1] = bChips ? (HasPlayChips() ? 0.25f + 0.6f * Persona.ChipFidget : 0.15f * Persona.ChipFidget) : 0.0f;
		W[2] = bInHand ? 0.25f : 0.0f;
		W[3] = 0.15f * Calm + (bInHand ? 0.1f : 0.0f);
		W[4] = 0.25f * Persona.Posture * Calm;
		W[5] = 0.45f * (1.0f - Persona.Posture) + (bInHand ? 0.0f : 0.15f);
		W[6] = 0.2f;
	}
	// Something other than what they're doing now, as a rule.
	W[HandMode] *= 0.25f;
	float Sum = 0.0f;
	for (float X : W)
	{
		Sum += X;
	}
	float Pick = Rng.FRandRange(0.0f, Sum);
	int32 Mode = 0;
	for (int32 I = 0; I < 7; ++I)
	{
		Pick -= W[I];
		if (W[I] > 0.0f && Pick <= 0.0f)
		{
			Mode = I;
			break;
		}
	}
	HandMode = Mode;
	HabitSide = Rng.FRand() < 0.5f ? 0 : 1;
	HandSwitch = bDeciding ? Rng.FRandRange(3.0f, 6.0f) : Rng.FRandRange(6.0f, 16.0f);
}

void ABackRoomPlayer::TestShow(bool bPeek)
{
	if (bPeek)
	{
		PeekHole();
	}
	else
	{
		GestureShow();
	}
}

void ABackRoomPlayer::TestHabit(int32 Mode, int32 Side)
{
	HandMode = FMath::Clamp(Mode, 0, 6);
	HabitSide = Side == 0 ? 0 : 1;
	HandSwitch = 60.0f;
}

ABackRoomPlayer::FHandPose ABackRoomPlayer::Still()
{
	FHandPose Z;
	Z.Pos = Z.Palm = Z.Finger = FVector::ZeroVector;
	Z.Curl = Z.Thumb = Z.Pinch = 0.0f;
	return Z;
}

void ABackRoomPlayer::Hermite(const FHandPose& P0, const FHandPose& V0, const FHandPose& P1, const FHandPose& V1, float Duration, float T, FHandPose& Out, FHandPose& Rate)
{
	const float D = FMath::Max(Duration, 0.01f);
	const float T2 = T * T;
	const float T3 = T2 * T;
	const float H00 = 2.0f * T3 - 3.0f * T2 + 1.0f, H10 = (T3 - 2.0f * T2 + T) * D, H01 = 3.0f * T2 - 2.0f * T3, H11 = (T3 - T2) * D;
	// The same, differentiated per second.
	const float R00 = (6.0f * T2 - 6.0f * T) / D, R10 = 3.0f * T2 - 4.0f * T + 1.0f, R01 = (6.0f * T - 6.0f * T2) / D, R11 = 3.0f * T2 - 2.0f * T;
	auto Mix = [&](const auto& A0, const auto& B0, const auto& A1, const auto& B1, auto& Value, auto& Speed) {
		Value = A0 * H00 + B0 * H10 + A1 * H01 + B1 * H11;
		Speed = A0 * R00 + B0 * R10 + A1 * R01 + B1 * R11;
	};
	Mix(P0.Pos, V0.Pos, P1.Pos, V1.Pos, Out.Pos, Rate.Pos);
	Mix(P0.Palm, V0.Palm, P1.Palm, V1.Palm, Out.Palm, Rate.Palm);
	Mix(P0.Finger, V0.Finger, P1.Finger, V1.Finger, Out.Finger, Rate.Finger);
	Mix(P0.Curl, V0.Curl, P1.Curl, V1.Curl, Out.Curl, Rate.Curl);
	Mix(P0.Thumb, V0.Thumb, P1.Thumb, V1.Thumb, Out.Thumb, Rate.Thumb);
	Mix(P0.Pinch, V0.Pinch, P1.Pinch, V1.Pinch, Out.Pinch, Rate.Pinch);
	Out.Palm = Dir(Out.Palm, P0.Palm);
	Out.Finger = Dir(Out.Finger, P0.Finger);
}

ABackRoomPlayer::FHandPose ABackRoomPlayer::PassRate(int32 Side, const FHandPose& From, const FHandPose& To) const
{
	const TArray<FHandStep>& Q = Steps[Side];
	if (Q.Num() < 2)
	{
		return Still();
	}
	const FHandPose After = Q[1].bRest ? IdleGoal(Side) : Q[1].Pose;
	const FVector In = To.Pos - From.Pos;
	const FVector Out = After.Pos - To.Pos;
	// It stops where it presses, holds or turns back; a point it only passes, it sweeps through.
	if (In.Size() < 1.5 || Out.Size() < 1.5 || FVector::DotProduct(In.GetSafeNormal(), Out.GetSafeNormal()) < 0.1)
	{
		return Still();
	}
	const float Span = FMath::Max(Q[0].Duration + Q[1].Duration, 0.05f);
	FHandPose V;
	V.Pos = ((After.Pos - From.Pos) / Span).GetClampedToMaxSize(MaxPass);
	V.Palm = (After.Palm - From.Palm) / Span;
	V.Finger = (After.Finger - From.Finger) / Span;
	V.Curl = (After.Curl - From.Curl) / Span;
	V.Thumb = (After.Thumb - From.Thumb) / Span;
	V.Pinch = (After.Pinch - From.Pinch) / Span;
	return V;
}

ABackRoomPlayer::FHandPose ABackRoomPlayer::Touch(const FVector& Tip, const FVector& Finger, const FVector& Palm, float Curl, float Pinch) const
{
	FHandPose P;
	P.Finger = Finger.GetSafeNormal();
	P.Palm = Palm.GetSafeNormal();
	P.Curl = Curl;
	P.Pinch = Pinch;
	P.Thumb = 0.25f + 0.4f * Pinch;
	P.Pos = ToBody(Tip) - P.Finger * Reach(Curl) - P.Palm * 1.6;
	return P;
}

// ------------------------------------------------------------------ the peek

int32 ABackRoomPlayer::PeekHand() const
{
	if (Hole.Num() < 1 || !Hole[0])
	{
		return 1;
	}
	// The corner with the index is the near-left one on a card held the usual way: that hand lifts it, the other covers.
	return Hole[0]->GetPeekSide(GetEyes(), GetActorRightVector()) > 0.0f ? 1 : 0;
}

FVector ABackRoomPlayer::PeekGripAt(float Amount, float Height) const
{
	if (Hole.Num() == 2 && Hole[0] && Hole[1])
	{
		// The two cards are a stack: one hinge, the upper card's curl inside the lower's.
		const FVector Eyes = GetEyes();
		const ABackRoomCard::FPeekFlapWorld W = ABackRoomCard::MakeSharedFlap(*Hole[0], *Hole[1], Eyes);
		return (Hole[0]->GetPeekGripShared(Amount, W, PeekMaxLift(), 0.0f, Height, Eyes) + Hole[1]->GetPeekGripShared(Amount, W, PeekMaxLift(), ABackRoomCard::NestGap, Height, Eyes)) * 0.5;
	}
	FVector Sum = FVector::ZeroVector;
	int32 N = 0;
	for (const ABackRoomCard* Card : Hole)
	{
		if (Card)
		{
			Sum += Card->GetPeekGrip(Amount, GetEyes(), PeekMaxLift());
			++N;
		}
	}
	return N > 0 ? Sum / N : Spots.Cards;
}

FVector ABackRoomPlayer::FingerPathAt(float Amount) const
{
	// A fingertip's flesh is about 0.65 cm deep: its center is that far under the card's skin, and a finger can't
	// sink into the felt to get there.
	FVector At = PeekGripAt(Amount, -0.65f);
	At.Z = FMath::Max(At.Z, ABackRoomStage::FeltZ + 0.8);
	return At;
}

FVector ABackRoomPlayer::TipBody(int32 Side) const
{
	const FHandPose& H = HandNow[Side];
	return H.Pos + H.Finger.GetSafeNormal() * Reach(H.Curl) + H.Palm.GetSafeNormal() * 1.6;
}

float ABackRoomPlayer::PeekAmountFromHand(int32 Side) const
{
	const FVector G0 = ToBody(FingerPathAt(0.0f));
	const FVector Chord = ToBody(FingerPathAt(1.0f)) - G0;
	if (Chord.SizeSquared() < 0.01)
	{
		return 0.0f;
	}
	const FVector Rel = TipBody(Side) - G0;
	const double Along = FVector::DotProduct(Rel, Chord) / Chord.SizeSquared();
	// Only fingers actually on the corner's path lift it.
	const double Off = (Rel - Chord * Along).Size();
	return FMath::Clamp(static_cast<float>(Along), 0.0f, 1.0f) * (1.0f - FMath::SmoothStep(1.5f, 4.0f, static_cast<float>(Off)));
}

ABackRoomPlayer::FHandPose ABackRoomPlayer::PeekGripPose(int32 Side, float Amount, float Pinch) const
{
	const float Sx = Side == 0 ? 1.0f : -1.0f;
	// Thumb and finger on the corner, the hand coming in from the corner's own side (as in the photos: the thumb
	// enters from the left) so the lifted face stays clear of it; the hand tilts up as the corner comes up.
	// The fingers stay long and loose (a finger tucked under the corner, the thumb along beside it): a procedural
	// pinch can't make a thumb oppose, and a long finger lets the lifted face stay in view.
	return Touch(FingerPathAt(Amount), FVector(-Sx * 0.9f, 0.35f, -0.12f + 0.45f * Amount), FVector(-Sx * 0.25f, 0.0f, -1.0f), 0.14f, 0.3f * Pinch);
}

ABackRoomPlayer::FHandPose ABackRoomPlayer::PeekCoverPose(int32 Side) const
{
	const FVector C = Hole.Num() == 2 && Hole[0] && Hole[1] ? (Hole[0]->GetActorLocation() + Hole[1]->GetActorLocation()) * 0.5 : Spots.Cards;
	const float Sx = Side == 0 ? 1.0f : -1.0f;
	// Laid over the far half of the cards, the fingers curled over them and reaching across toward the corner's side.
	const float Toward = PeekHand() == 1 ? 1.0f : -1.0f;
	const FVector Tip = C + Spots.Inward * 3.2 + GetActorRightVector() * (-Toward * 9.2) + FVector(0.0, 0.0, 0.6);
	// Resting against the cards' far edge on the side away from the lifted corner, fingers curled and pointing across
	// (it keeps them still, and both cards stay in clear view between the hands).
	return Touch(Tip, FVector(-Sx * 0.4f, 0.9f, -0.35f), FVector(-Sx * 0.1f, 0.0f, -1.0f), 0.6f);
}

void ABackRoomPlayer::Queue(int32 Side, const FHandPose& Pose, float Duration, float Arc, TFunction<void()> OnArrive)
{
	if (Steps[Side].Num() == 0)
	{
		StepFrom[Side] = HandNow[Side];
		StepT[Side] = 0.0f;
	}
	FHandStep& S = Steps[Side].AddDefaulted_GetRef();
	S.Pose = Pose;
	S.Duration = Duration;
	S.Arc = Arc;
	S.OnArrive = MoveTemp(OnArrive);
}

void ABackRoomPlayer::QueueRest(int32 Side, float Duration)
{
	Queue(Side, RestPose(Side), Duration, 1.5f);
	Steps[Side].Last().bRest = true;
}

float ABackRoomPlayer::QueuedTime(int32 Side) const
{
	float T = 0.0f;
	for (int32 I = 0; I < Steps[Side].Num(); ++I)
	{
		T += Steps[Side][I].Duration * (I == 0 ? 1.0f - StepT[Side] : 1.0f);
	}
	return T;
}

FTransform ABackRoomPlayer::HandFrame(int32 Side) const
{
	const FTransform& B = Body->GetComponentTransform();
	FVector F = B.TransformVectorNoScale(HandNow[Side].Finger.GetSafeNormal());
	FVector Back = -B.TransformVectorNoScale(HandNow[Side].Palm.GetSafeNormal());
	// The pose pitches the fingers up about the wrist when they would be in the table: what the hand holds goes with it.
	const float Pitch = ContactPitch(Side);
	const FVector Across = FVector::CrossProduct(F, FVector::UpVector).GetSafeNormal();
	if (Pitch > 0.002f && !Across.IsNearlyZero())
	{
		const FQuat Q(Across, Pitch);
		F = Q.RotateVector(F);
		Back = Q.RotateVector(Back);
	}
	return FTransform(FRotationMatrix::MakeFromXZ(F, Back).ToQuat(), B.TransformPosition(HandNow[Side].Pos + PeekBias[Side] + ContactShift(Side)));
}

void ABackRoomPlayer::Grab(AActor* Thing, int32 Side, const FTransform& Offset)
{
	LetGo(Thing);
	FHeld& H = HeldThings.AddDefaulted_GetRef();
	H.Thing = Thing;
	H.Side = Side;
	H.Offset = Offset;
}

void ABackRoomPlayer::LetGo(AActor* Thing)
{
	HeldThings.RemoveAll([Thing](const FHeld& H) { return H.Thing.Get() == Thing; });
}

// ------------------------------------------------------------------ every frame

void ABackRoomPlayer::UpdateHands(float Dt)
{
	bool bHandOnChips = false;
	for (int32 Side = 0; Side < 2; ++Side)
	{
		Wiggle[Side] = Still();
		TArray<FHandStep>& Q = Steps[Side];
		if (Q.Num() > 0)
		{
			// A move: a smooth curve from where the hand was, leaving at the speed it had, through the step's
			// pose, on into the next one when it only passes there (a stop where it presses, holds or turns back).
			FHandStep* Cur = &Q[0];
			if (!Cur->bBegun)
			{
				// Round the table's edge rather than through it: a stop on the way first when the straight way is blocked.
				const FHandPose Intended = Cur->bRest ? IdleGoal(Side) : Cur->Pose;
				const FVector Via = RouteHand(Side, StepFrom[Side].Pos, Intended.Pos);
				if (!Via.Equals(Intended.Pos, 0.05))
				{
					FHandStep Step;
					Step.Pose = Intended;
					Step.Pose.Pos = Via;
					Step.Duration = 0.2f;
					Q.Insert(MoveTemp(Step), 0);
					Cur = &Q[0];
					StepT[Side] = 0.0f;
				}
			}
			FHandStep& S = *Cur;
			const FHandPose Target = S.bRest ? IdleGoal(Side) : S.Pose;
			const FHandPose& From = StepFrom[Side];
			if (!S.bBegun)
			{
				S.bBegun = true;
				S.StartRate = HandRate[Side];
				S.StartRate.Pos = S.StartRate.Pos.GetClampedToMaxSize(MaxPass);
				S.EndRate = PassRate(Side, From, Target);
			}
			StepT[Side] += Dt / FMath::Max(S.Duration, 0.01f);
			const float T = FMath::Min(StepT[Side], 1.0f);
			FHandPose& H = HandNow[Side];
			Hermite(From, S.StartRate, Target, S.EndRate, S.Duration, T, H, HandRate[Side]);
			// Up over the felt and the chips, lifting off and settling with the move.
			const float Lift = FMath::Sin(T * UE_PI);
			H.Pos.Z += S.Arc * Lift * Lift;
			// A quick arrival can swing past its point: never down through the felt.
			H.Pos.Z = FMath::Max(H.Pos.Z, FMath::Min(From.Pos.Z, Target.Pos.Z) - 0.3);
			if (StepT[Side] >= 1.0f)
			{
				const float Over = (StepT[Side] - 1.0f) * S.Duration;
				HandRate[Side] = S.EndRate;
				StepFrom[Side] = Target;
				TFunction<void()> Fn = MoveTemp(S.OnArrive);
				Q.RemoveAt(0);
				// The frame's leftover time goes to the next move, so a hand sweeping through doesn't hitch.
				StepT[Side] = Q.Num() > 0 ? FMath::Min(Over / FMath::Max(Q[0].Duration, 0.01f), 0.5f) : 0.0f;
				if (Fn)
				{
					Fn();
				}
			}
			continue;
		}

		// Idle: the hand goes to its habit of the moment on a spring, never quite still once there.
		FHandPose Goal = IdleGoal(Side);
		const bool bTrickHand = Side == 1 && SeatRole == EBackRoomRole::Player && HandMode == 1;
		const float Live = (1.0f - Stillness) * (SeatRole == EBackRoomRole::Hero ? 0.5f : 1.0f) * (bTrickHand ? 0.3f : 1.0f);
		const float Ph = Persona.Seed * 1.37f + Side * 2.1f;
		Goal.Pos += Live * FVector(0.5f * FMath::Sin(Time * 0.61f + Ph), 0.4f * FMath::Sin(Time * 0.47f + 2.0f * Ph), 0.25f * FMath::Sin(Time * 0.83f + Ph));
		Goal.Pos = RouteHand(Side, HandNow[Side].Pos, Goal.Pos);
		Goal.Curl += Live * 0.05f * FMath::Sin(Time * 0.37f + Ph);
		const float Omega = SeatRole == EBackRoomRole::Hero ? 11.0f : (bTrickHand ? 9.0f : 4.5f);
		FHandPose& H = HandNow[Side];
		FHandPose& V = HandRate[Side];
		Spring(H.Pos, V.Pos, Goal.Pos, Omega, Dt);
		FVector Palm = H.Palm;
		FVector Finger = H.Finger;
		Spring(Palm, V.Palm, Goal.Palm, Omega, Dt);
		Spring(Finger, V.Finger, Goal.Finger, Omega, Dt);
		H.Palm = Dir(Palm, H.Palm);
		H.Finger = Dir(Finger, H.Finger);
		Spring(H.Curl, V.Curl, Goal.Curl, Omega * 1.5f, Dt);
		Spring(H.Thumb, V.Thumb, Goal.Thumb, Omega * 1.5f, Dt);
		Spring(H.Pinch, V.Pinch, Goal.Pinch, Omega * 1.5f, Dt);
		StepFrom[Side] = H;
		const double Off = FVector::Dist(H.Pos, Goal.Pos);
		if (bTrickHand)
		{
			if (HasPlayChips())
			{
				bHandOnChips = Off < 2.5;
			}
			else
			{
				// Fingering the top of the stack.
				Wiggle[Side].Curl = 0.08f * FMath::Sin(Time * 2.3f + Ph);
				Wiggle[Side].Pinch = 0.2f * FMath::Max(0.0f, FMath::Sin(Time * 1.7f));
			}
		}
		if (SeatRole == EBackRoomRole::Hero && bHeroPeek && Hole.Num() == 2 && Side == PeekHand())
		{
			// Fingers settled on the corner: it comes up under them (and goes back down the same way). The card
			// follows the hand, never the other way round.
			const bool bHolding = Off < 1.6;
			HeroLift = Ease(HeroLift, bHolding || HeroLift > 0.05f ? 1.0f : 0.0f, bHolding ? 4.2f : 1.0f, Dt);
			GripSide = Off < 3.0 ? Side : -1;
		}
	}
	if (SeatRole == EBackRoomRole::Hero && !bHeroPeek)
	{
		HeroLift = Ease(HeroLift, 0.0f, 14.0f, Dt);
		GripSide = -1;
	}
	AuditContacts(Dt);
	// The skeleton's fingers are not quite where the model puts them (a curled finger is shorter): for the hand
	// that works the corner, the real fingertip is measured against the intended one and the wrist moved the
	// difference, until they meet.
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const bool bTrack = TrackSide == Side || (SeatRole == EBackRoomRole::Hero && bHeroPeek && Hole.Num() == 2 && Side == PeekHand());
		if (bTrack && Body->GetSkeletalMeshAsset())
		{
			const TCHAR* Sd = Side == 0 ? TEXT("l") : TEXT("r");
			const FVector P3 = ToBody(Body->GetBoneLocation(*FString::Printf(TEXT("index_03_%s"), Sd)));
			const FVector P2 = ToBody(Body->GetBoneLocation(*FString::Printf(TEXT("index_02_%s"), Sd)));
			const FVector Real = P3 + (P3 - P2).GetSafeNormal() * 2.0;
			PeekBias[Side] = (PeekBias[Side] + (TipBody(Side) + ContactShift(Side) - Real) * (1.0f - FMath::Exp(-8.0f * Dt))).GetClampedToMaxSize(10.0);
		}
		else
		{
			PeekBias[Side] = Ease(PeekBias[Side], FVector::ZeroVector, 5.0f, Dt);
		}
	}
	UpdatePlayChips(Dt, bHandOnChips);

	// Idle hand habits: change every several seconds.
	HandSwitch -= Dt;
	if (HandSwitch <= 0.0f)
	{
		PickHabit(bThinking);
	}

	// The corner goes where the fingers pinching it go; once they let go it falls back to the felt.
	const bool bHole = Hole.Num() == 2 && Hole[0] && Hole[1] && !Hole[0]->IsFaceUp();
	if (GripSide >= 0 && SeatRole != EBackRoomRole::Hero && Steps[GripSide].Num() == 0)
	{
		GripSide = -1; // a queued peek was cut short
	}
	if (GripSide >= 0 && bHole)
	{
		PeekNow = PeekAmountFromHand(GripSide);
	}
	else
	{
		PeekNow = Ease(PeekNow, 0.0f, 16.0f, Dt);
		if (PeekNow < 0.02f)
		{
			PeekNow = 0.0f;
		}
	}
	if (bHole && PeekNow > 0.12f && !bPeekHeard)
	{
		// The scrape of a card's corner coming up.
		bPeekHeard = true;
		Sound(Snd(ss::SoundId::Flip), Hole[0]->GetActorLocation(), SeatRole == EBackRoomRole::Hero ? 0.32f : 0.22f);
	}
	else if (bPeekHeard && PeekNow < 0.03f)
	{
		// ...and the tap of it settling back.
		bPeekHeard = false;
		if (bHole)
		{
			Sound(Snd(ss::SoundId::Flip), Hole[0]->GetActorLocation(), SeatRole == EBackRoomRole::Hero ? 0.24f : 0.16f);
		}
	}
	if (PeekNow > 0.002f || PeekApplied > 0.002f)
	{
		if (bHole)
		{
			// One stack, one hinge: the upper card's curl nests inside the lower's.
			const FVector Eyes = GetEyes();
			const ABackRoomCard::FPeekFlapWorld W = ABackRoomCard::MakeSharedFlap(*Hole[0], *Hole[1], Eyes);
			Hole[0]->SetPeekShared(PeekNow, W, PeekMaxLift(), 0.0f, Eyes);
			Hole[1]->SetPeekShared(PeekNow, W, PeekMaxLift(), ABackRoomCard::NestGap, Eyes);
		}
		else
		{
			for (ABackRoomCard* Card : Hole)
			{
				if (Card && !Card->IsFaceUp())
				{
					Card->SetPeek(PeekNow, GetEyes(), PeekMaxLift());
				}
			}
		}
		PeekApplied = PeekNow;
	}

	// Things in hand go where the hand goes.
	for (int32 I = HeldThings.Num() - 1; I >= 0; --I)
	{
		FHeld& H = HeldThings[I];
		AActor* Thing = H.Thing.Get();
		if (!Thing)
		{
			HeldThings.RemoveAt(I);
			continue;
		}
		const FTransform Frame = HandFrame(H.Side);
		if (H.bUpright)
		{
			// Chips stay level, hanging from the fingertips.
			Thing->SetActorLocation(Frame.TransformPosition(H.Offset.GetLocation()) - FVector(0.0, 0.0, H.Drop));
		}
		else
		{
			// Location and turn only: a held thing keeps its own scale (the dealer's deck is a stretched card).
			const FTransform At = H.Offset * Frame;
			Thing->SetActorLocationAndRotation(At.GetLocation(), At.GetRotation());
		}
	}
}

// ------------------------------------------------------------------ gestures

float ABackRoomPlayer::PeekHole()
{
	if (Hole.Num() < 2 || !Hole[0] || !Hole[1])
	{
		return 0.0f;
	}
	const FVector C = (Hole[0]->GetActorLocation() + Hole[1]->GetActorLocation()) * 0.5;
	const float Hold = Rng.FRandRange(0.7f, 1.3f) * (1.0f - 0.3f * Persona.Nervousness);
	// Lean in and look down at the cards for the whole peek.
	LeanTarget = 0.95f;
	NextShift = Hold + 1.5f;
	GazeHold = C;
	GazeHoldLeft = 0.45f + Hold + 0.3f;
	HeadFollow = 0.85f;
	const int32 Lifter = PeekHand();
	const int32 Cover = 1 - Lifter;
	TrackSide = Lifter;
	// One hand goes down over the cards and stays; the other comes to the near corner, pinches it, lifts it
	// (the card follows the fingers), holds, and lets it down again.
	Queue(Cover, PeekCoverPose(Cover), 0.45f, 3.0f);
	Queue(Cover, PeekCoverPose(Cover), Hold + 1.0f);
	QueueRest(Cover, 0.45f);
	Queue(Lifter, PeekGripPose(Lifter, 0.0f, 0.1f), 0.45f, 3.5f);
	Queue(Lifter, PeekGripPose(Lifter, 0.0f, 0.7f), 0.15f, 0.0f, [this, Lifter]() { GripSide = Lifter; });
	Queue(Lifter, PeekGripPose(Lifter, 1.0f, 0.7f), 0.38f, 0.0f, [this]() { ReactToHole(); });
	Queue(Lifter, PeekGripPose(Lifter, 1.0f, 0.7f), Hold);
	Queue(Lifter, PeekGripPose(Lifter, 0.1f, 0.7f), 0.26f, 0.0f, [this]() { GripSide = -1; TrackSide = -1; });
	QueueRest(Lifter, 0.45f);
	return 0.45f + 0.15f + 0.38f + Hold + 0.26f + 0.45f;
}

float ABackRoomPlayer::GestureCheck()
{
	// A knock with the knuckles, or two taps of the fingers.
	const bool bKnuckles = Persona.Seed % 2 == 0;
	const float Curl = bKnuckles ? 0.85f : 0.1f;
	const FVector Spot = (Hole.Num() > 0 && Hole[0] ? Hole[0]->GetActorLocation() : Spots.Cards) + Spots.Inward * 9.0 + GetActorRightVector() * 6.0;
	const FVector Finger(0.1, 1.0, -0.45);
	const FVector Palm(0.0, 0.25, -1.0);
	const float Quick = 1.0f - 0.35f * Persona.Nervousness;
	Queue(1, Touch(Spot + FVector(0.0, 0.0, 6.0), Finger, Palm, Curl), 0.3f * Quick, 2.0f);
	for (int32 Tap = 0; Tap < 2; ++Tap)
	{
		Queue(1, Touch(Spot + FVector(0.0, 0.0, bKnuckles ? 2.0 : 0.3), Finger, Palm, Curl), 0.08f * Quick, 0.0f,
			[this, Spot, Tap]() { Sound(Snd(ss::SoundId::Check), Spot, Tap == 0 ? 0.8f : 0.55f); });
		Queue(1, Touch(Spot + FVector(0.0, 0.0, 5.0), Finger, Palm, Curl), 0.1f * Quick);
	}
	QueueRest(1, 0.4f);
	return QueuedTime(1);
}

float ABackRoomPlayer::GestureChips(int64 Amount, bool bAllIn, bool bBluff)
{
	if (!StackPile || !BetPile || Amount <= 0)
	{
		return 0.0f;
	}
	UWorld* World = GetWorld();
	// Bluffers push quick and hard; a big hand goes in gently.
	const float Pace = bBluff ? 0.75f : (Strength > 0.75f ? 1.2f : 1.0f);
	const FVector Bet = BetPile->GetActorLocation();
	if (bAllIn)
	{
		// Both hands behind the whole stack, and in it goes.
		ABackRoomChips* Pile = StackPile;
		const FVector Start = Pile->GetActorLocation();
		const FVector Right = GetActorRightVector();
		const double H = FMath::Max(Pile->GetHeight(), 2.0);
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const double S = Side == 0 ? -1.0 : 1.0;
			const FVector Palm = ToBodyDir(-Right * S) + FVector(0.0, 0.0, -0.3);
			Queue(Side, Touch(Start + Right * (S * 5.5) - Spots.Inward * 2.0 + FVector(0.0, 0.0, H * 0.5), FVector(0.0, 1.0, -0.2), Palm, 0.3f), 0.5f * Pace, 3.0f);
			TFunction<void()> Arrive;
			if (Side == 1)
			{
				Arrive = [this, Pile, Start, Amount]() {
					Sound(Snd(ss::SoundId::ChipStack), BetPile->GetActorLocation(), 1.0f);
					BetPile->SetAmount(BetPile->GetAmount() + Amount);
					Pile->SetAmount(0);
					Pile->SetActorLocation(Start);
				};
			}
			Queue(Side, Touch(Bet + Right * (S * 5.5) - Spots.Inward * 2.0 + FVector(0.0, 0.0, H * 0.5), FVector(0.0, 1.0, -0.2), Palm, 0.3f), 0.7f * Pace, 0.0f, MoveTemp(Arrive));
			QueueRest(Side, 0.6f);
		}
		// The stack starts sliding when the hands arrive behind it.
		FTimerHandle Handle;
		World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(Pile, [Pile, Bet, Pace]() { Pile->SlideTo(Bet, 0.7f * Pace); }), 0.5f * Pace, false);
		return QueuedTime(1);
	}

	// One hand: take the chips off the top of the stack and slide them out.
	const FVector Top = StackPile->GetTop();
	const FVector Finger(-0.25, 1.0, -0.35);
	const FVector Palm(0.25, 0.0, -1.0);
	Queue(1, Touch(Top + FVector(0.0, 0.0, 0.5), Finger, Palm, 0.45f, 0.25f), 0.33f * Pace, 3.0f, [this, Amount, World]() {
		ABackRoomChips* Moving = World->SpawnActor<ABackRoomChips>(ABackRoomChips::StaticClass(), FTransform(GetActorRotation(), StackPile->GetTop()));
		Moving->SetStyle(EBackRoomChipStyle::Bet, Persona.Seed + static_cast<int32>(Time * 10.0f));
		Moving->SetAmount(Amount);
		StackPile->SetAmount(FMath::Max<int64>(0, StackPile->GetAmount() - Amount));
		FHeld& H = HeldThings.AddDefaulted_GetRef();
		H.Thing = Moving;
		H.Side = 1;
		H.bUpright = true;
		H.Drop = Moving->GetHeight() + 0.2;
		H.Offset = FTransform(FVector(Reach(0.45f), 0.0, -1.6));
		Carrying = Moving;
	});
	Queue(1, Touch(Top + FVector(0.0, 0.0, 0.5), Finger, Palm, 0.5f, 0.85f), 0.1f);
	Queue(1, Touch(Bet + FVector(0.0, 0.0, 2.5), FVector(-0.1, 1.0, -0.3), Palm, 0.5f, 0.85f), 0.42f * Pace, bBluff ? 1.0f : 2.5f, [this, Amount, bBluff]() {
		if (ABackRoomChips* Moving = Carrying.Get())
		{
			LetGo(Moving);
			Moving->Destroy();
		}
		Carrying = nullptr;
		BetPile->SetAmount(BetPile->GetAmount() + Amount);
		Sound(Snd(Amount >= 25 ? ss::SoundId::ChipStack : ss::SoundId::Chip), BetPile->GetActorLocation(), bBluff ? 1.0f : 0.75f);
	});
	// A big hand lingers on the chips a beat; then the hand comes home.
	Queue(1, Touch(Bet + FVector(0.0, 0.0, 5.0), FVector(-0.1, 1.0, -0.3), Palm, 0.3f, 0.0f), Strength > 0.75f ? 0.35f : 0.12f);
	QueueRest(1, 0.45f);
	return QueuedTime(1);
}

float ABackRoomPlayer::GestureFold(const FVector& Muck)
{
	if (Hole.Num() < 2 || !Hole[0] || !Hole[1])
	{
		return 0.0f;
	}
	const FVector C = (Hole[0]->GetActorLocation() + Hole[1]->GetActorLocation()) * 0.5;
	const bool bToss = Persona.Nervousness > 0.6f;
	// Held weakly: the gesture finishes later, and the table may have cleared the cards by then.
	const TArray<TWeakObjectPtr<ABackRoomCard>> Cards = {Hole[0], Hole[1]};
	Queue(1, Touch(C + Spots.Inward * 2.0 + FVector(0.0, 0.0, 0.6), FVector(0.0, 1.0, -0.2), FVector(0.0, 0.0, -1.0), 0.1f), 0.32f, 2.5f, [this, Cards]() {
		for (const TWeakObjectPtr<ABackRoomCard>& Weak : Cards)
		{
			if (ABackRoomCard* Card = Weak.Get())
			{
				Card->Stop();
				Grab(Card, 1, Card->GetActorTransform().GetRelativeTransform(HandFrame(1)));
			}
		}
	});
	Queue(1, Touch(C + Spots.Inward * (bToss ? 14.0 : 22.0) + FVector(0.0, 0.0, bToss ? 6.0 : 0.6), FVector(0.0, 1.0, bToss ? 0.1 : -0.2), FVector(0.0, 0.0, -1.0), 0.1f), bToss ? 0.16f : 0.3f, 0.0f,
		[this, Cards, Muck, bToss, C]() {
			Sound(Snd(ss::SoundId::Fold), C, 0.8f);
			for (int32 I = 0; I < Cards.Num(); ++I)
			{
				ABackRoomCard* Card = Cards[I].Get();
				if (!Card)
				{
					continue;
				}
				LetGo(Card);
				const FTransform Target(FRotator(0.0f, Rng.FRandRange(0.0f, 360.0f), 0.0f), Muck + FVector(Rng.FRandRange(-3.0f, 3.0f), Rng.FRandRange(-3.0f, 3.0f), 0.1 * I));
				if (bToss)
				{
					Card->PitchTo(Target, 0.45f, 6.0f, Rng.FRandRange(-90.0f, 90.0f));
				}
				else
				{
					Card->SlideTo(Target, 0.5f);
				}
			}
		});
	QueueRest(1, 0.45f);
	return QueuedTime(1);
}

float ABackRoomPlayer::GestureShow()
{
	if (Hole.Num() < 2 || !Hole[0] || !Hole[1])
	{
		return 0.0f;
	}
	// The fanned pair is drawn apart first, so each card turns over clear of the other.
	const bool bSpread = !Spots.Spread[0].GetLocation().IsNearlyZero();
	if (bSpread)
	{
		const FVector Mid = (Spots.Spread[0].GetLocation() + Spots.Spread[1].GetLocation()) * 0.5;
		Queue(1, Touch(Mid - Spots.Inward * 3.0 + FVector(0.0, 0.0, 0.8), FVector(0.0, 1.0, -0.3), FVector(0.3, 0.0, -1.0), 0.3f, 0.4f), 0.3f, 2.0f, [this]() {
			for (int32 K = 0; K < 2; ++K)
			{
				if (Hole[K])
				{
					Hole[K]->SlideTo(Spots.Spread[K], 0.26f);
				}
			}
			Sound(Snd(ss::SoundId::Deal), Spots.Spread[0].GetLocation(), 0.35f);
		});
		Queue(1, Touch(Mid - Spots.Inward * 3.0 + FVector(0.0, 0.0, 0.8), FVector(0.0, 1.0, -0.3), FVector(0.3, 0.0, -1.0), 0.3f, 0.4f), 0.28f);
	}
	for (int32 I = 0; I < 2; ++I)
	{
		ABackRoomCard* Card = Hole[I];
		const FVector At = bSpread ? Spots.Spread[I].GetLocation() : Card->GetActorLocation();
		const TWeakObjectPtr<ABackRoomCard> Weak = Card;
		Queue(1, Touch(At - Spots.Inward * 3.0 + FVector(0.0, 0.0, 0.8), FVector(0.0, 1.0, -0.3), FVector(0.3, 0.0, -1.0), 0.3f, 0.5f), 0.3f, 2.0f,
			[this, Weak]() {
				if (ABackRoomCard* Shown = Weak.Get())
				{
					Shown->Flip(true, 0.3f);
					Sound(Snd(ss::SoundId::Flip), Shown->GetActorLocation(), 0.7f);
				}
			});
		Queue(1, Touch(At - Spots.Inward * 3.0 + FVector(0.0, 0.0, 3.0), FVector(0.0, 1.0, -0.3), FVector(-0.4, 0.0, -0.9), 0.3f, 0.2f), 0.25f);
	}
	QueueRest(1, 0.4f);
	return QueuedTime(1);
}

FTransform ABackRoomPlayer::GetDeckTop() const
{
	if (!Deck)
	{
		return FTransform(GetActorRotation(), ToWorld(RestPose(0).Pos) + FVector(0.0, 0.0, 4.0));
	}
	// Face down in the hand: the card's +Z points down, so its back is the top.
	const FTransform T = Deck->GetActorTransform();
	return FTransform(T.GetRotation(), T.GetLocation() - Deck->GetActorUpVector() * (DeckThickness * 0.5 + 0.05));
}

float ABackRoomPlayer::GestureDeal(ABackRoomCard* Card, const FTransform& Target, bool bFaceUp)
{
	if (!Card)
	{
		return 0.0f;
	}
	const FVector DeckAt = ToBody(GetDeckTop().GetLocation());
	const FVector Toward = (ToBody(Target.GetLocation()) - DeckAt).GetSafeNormal2D();
	// The right thumb slides the top card off; a flick of the wrist sends it.
	FHandPose Take;
	Take.Pos = DeckAt + FVector(-9.0, -3.0, 3.0);
	Take.Finger = FVector(0.9, 0.45, -0.25).GetSafeNormal();
	Take.Palm = FVector(0.15, 0.0, -1.0);
	Take.Curl = 0.3f;
	Take.Pinch = 0.6f;
	Take.Thumb = 0.5f;
	FHandPose Flick = Take;
	Flick.Pos = DeckAt + Toward * 16.0 + FVector(0.0, 0.0, 4.0);
	Flick.Finger = (Toward + FVector(0.0, 0.0, -0.15)).GetSafeNormal();
	Flick.Pinch = 0.2f;
	const float Distance = FVector::Dist2D(GetDeckTop().GetLocation(), Target.GetLocation());
	const float Flight = 0.18f + Distance / 420.0f;
	const TWeakObjectPtr<ABackRoomCard> Weak = Card;
	Queue(1, Take, 0.13f, 1.0f, [this, Weak]() {
		ABackRoomCard* Card = Weak.Get();
		if (!Card)
		{
			return;
		}
		Card->Stop();
		Card->SetActorTransform(GetDeckTop());
		Card->SetActorHiddenInGame(false);
		Grab(Card, 1, Card->GetActorTransform().GetRelativeTransform(HandFrame(1)));
	});
	Queue(1, Flick, 0.13f, 0.0f, [this, Weak, Target, Flight, bFaceUp]() {
		ABackRoomCard* Card = Weak.Get();
		if (!Card)
		{
			return;
		}
		LetGo(Card);
		Card->PitchTo(Target, Flight, 3.5f, Rng.FRandRange(-25.0f, 25.0f), bFaceUp);
		Sound(Snd(ss::SoundId::Deal), Card->GetActorLocation(), 0.6f);
		if (bFaceUp)
		{
			// The snap of the card turning over where it lands.
			FTimerHandle Handle;
			GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, Target]() {
				Sound(Snd(ss::SoundId::Flip), Target.GetLocation(), 0.6f);
			}), Flight + 0.05f, false);
		}
	});
	QueueRest(1, 0.2f);
	return 0.46f;
}

float ABackRoomPlayer::GestureSweep(ABackRoomChips* Pile, const FVector& To, TFunction<void()> OnDone)
{
	if (!Pile)
	{
		return 0.0f;
	}
	// An arm's reach from the right shoulder, leaning in: beyond it the hand points the chips along.
	const FVector Shoulder = ToWorld(FVector(-18.0, 4.0, 118.0));
	auto Reachable = [&Shoulder](const FVector& P) {
		const FVector D = P - Shoulder;
		return D.Size() > 78.0 ? Shoulder + D.GetSafeNormal() * 78.0 : P;
	};
	const FVector From = Pile->GetActorLocation();
	const FVector Dir = (To - From).GetSafeNormal2D();
	const FVector Palm = ToBodyDir(Dir) + FVector(0.0, 0.0, -0.6);
	const FVector Finger = ToBodyDir(FVector::CrossProduct(FVector::UpVector, Dir)) + FVector(0.0, 0.0, -0.2);
	LeanTarget = 0.9f;
	const TWeakObjectPtr<ABackRoomChips> WeakPile = Pile;
	Queue(1, Touch(Reachable(From - Dir * 5.0 + FVector(0.0, 0.0, 1.5)), Finger, Palm, 0.2f), 0.3f, 3.0f, [WeakPile, To]() {
		if (ABackRoomChips* P = WeakPile.Get())
		{
			P->SlideTo(To, 0.45f);
		}
	});
	Queue(1, Touch(Reachable(To - Dir * 5.0 + FVector(0.0, 0.0, 1.5)), Finger, Palm, 0.2f), 0.45f, 0.0f, [this, To, Done = MoveTemp(OnDone)]() {
		Sound(Snd(ss::SoundId::ChipStack), To, 0.7f);
		if (Done)
		{
			Done();
		}
	});
	QueueRest(1, 0.35f);
	return QueuedTime(1);
}

float ABackRoomPlayer::GestureCollect(const TArray<ABackRoomCard*>& Cards, const FVector& Muck)
{
	if (Cards.Num() == 0)
	{
		return 0.0f;
	}
	FVector Mid = FVector::ZeroVector;
	for (const ABackRoomCard* Card : Cards)
	{
		Mid += Card ? Card->GetActorLocation() : PotAt;
	}
	Mid /= Cards.Num();
	const FVector Shoulder = ToWorld(FVector(-18.0, 4.0, 118.0));
	const FVector D = Mid - Shoulder;
	const FVector Reach = D.Size() > 78.0 ? Shoulder + D.GetSafeNormal() * 78.0 : Mid;
	LeanTarget = 0.85f;
	// Held weakly: the table may clear the cards before the hand gets there.
	TArray<TWeakObjectPtr<ABackRoomCard>> Weak;
	for (ABackRoomCard* Card : Cards)
	{
		Weak.Add(Card);
	}
	Queue(1, Touch(Reach + FVector(0.0, 0.0, 1.0), FVector(0.6, 0.6, -0.3), FVector(-0.4, -0.3, -0.8), 0.2f), 0.35f, 3.0f, [this, Weak, Muck]() {
		for (int32 I = 0; I < Weak.Num(); ++I)
		{
			if (ABackRoomCard* Card = Weak[I].Get())
			{
				Card->SetPeek(0.0f, Muck);
				Card->SlideTo(FTransform(GetActorRotation(), Muck + FVector(0.0, 0.0, 0.05 * I)), 0.45f + 0.03f * I);
			}
		}
	});
	Queue(1, Touch(Muck + FVector(0.0, 0.0, 2.0), FVector(0.6, 0.6, -0.3), FVector(-0.4, -0.3, -0.8), 0.2f), 0.45f);
	QueueRest(1, 0.3f);
	return QueuedTime(1);
}

void ABackRoomPlayer::SetHeroPeek(bool bPeeking)
{
	bHeroPeek = bPeeking;
}

void ABackRoomPlayer::BeginPlay()
{
	Super::BeginPlay();
	if (SeatRole == EBackRoomRole::Extra)
	{
		// Across the room and out of focus: the body moves a little less often, the face rarely.
		SetActorTickInterval(0.05f);
		for (UActorComponent* C : GetComponents())
		{
			if (USkeletalMeshComponent* Sk = Cast<USkeletalMeshComponent>(C))
			{
				Sk->SetComponentTickInterval(Sk == Face ? 0.1f : 0.05f);
				Sk->bEnableUpdateRateOptimizations = true;
			}
		}
	}
	if (SeatRole == EBackRoomRole::Dealer && !Deck)
	{
		// The deck: a card stretched to a deck's thickness, face down in the left hand.
		Deck = GetWorld()->SpawnActor<ABackRoomCard>(ABackRoomCard::StaticClass(), GetActorTransform());
		Deck->SetCard(-1);
		const UStaticMesh* Mesh = Deck->GetMesh() ? Deck->GetMesh()->GetStaticMesh() : nullptr;
		const double Ez = Mesh ? FMath::Max(Mesh->GetBounds().BoxExtent.Z, 0.001) : 0.05;
		Deck->SetActorScale3D(FVector(1.0, 1.0, DeckThickness * 0.5 / Ez));
		FHeld& H = HeldThings.AddDefaulted_GetRef();
		H.Thing = Deck;
		H.Side = 0;
		// Lying on the palm (the palm side of the hand's frame is -Z), its long side along the fingers.
		H.Offset = FTransform(FRotator(0.0f, 90.0f, 0.0f), FVector(8.0, 0.0, -2.4));
	}
}
