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

int32 Snd(ss::SoundId Id)
{
	return static_cast<int32>(Id);
}
} // namespace BackRoomHandsDetail

using namespace BackRoomHandsDetail;

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
	P.Pos = FVector(Sx * 12.0, Rail + 17.0, 80.5);
	P.Palm = FVector(Sx * 0.15f, 0.0, -1.0);
	P.Finger = FVector(-Sx * 0.45f, 1.0, -0.1);
	P.Curl = 0.35f + 0.1f * Persona.Nervousness;
	P.Thumb = 0.2f;
	return P;
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
	const FVector F = B.TransformVectorNoScale(HandNow[Side].Finger.GetSafeNormal());
	const FVector Back = -B.TransformVectorNoScale(HandNow[Side].Palm.GetSafeNormal());
	return FTransform(FRotationMatrix::MakeFromXZ(F, Back).ToQuat(), B.TransformPosition(HandNow[Side].Pos));
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
	for (int32 Side = 0; Side < 2; ++Side)
	{
		TArray<FHandStep>& Q = Steps[Side];
		if (Q.Num() > 0)
		{
			FHandStep& S = Q[0];
			const FHandPose Target = S.bRest ? RestPose(Side) : S.Pose;
			StepT[Side] += Dt / FMath::Max(S.Duration, 0.01f);
			const float T = FMath::Min(StepT[Side], 1.0f);
			const float E = Smoother(T);
			const FHandPose& From = StepFrom[Side];
			FHandPose& H = HandNow[Side];
			H.Pos = FMath::Lerp(From.Pos, Target.Pos, static_cast<double>(E)) + FVector(0.0, 0.0, S.Arc * FMath::Sin(T * UE_PI));
			H.Palm = FMath::Lerp(From.Palm, Target.Palm, static_cast<double>(E)).GetSafeNormal();
			H.Finger = FMath::Lerp(From.Finger, Target.Finger, static_cast<double>(E)).GetSafeNormal();
			H.Curl = FMath::Lerp(From.Curl, Target.Curl, E);
			H.Thumb = FMath::Lerp(From.Thumb, Target.Thumb, E);
			H.Pinch = FMath::Lerp(From.Pinch, Target.Pinch, E);
			if (StepT[Side] >= 1.0f)
			{
				H = Target;
				StepFrom[Side] = Target;
				StepT[Side] = 0.0f;
				TFunction<void()> Fn = MoveTemp(S.OnArrive);
				Q.RemoveAt(0);
				if (Fn)
				{
					Fn();
				}
			}
			continue;
		}

		// Idle: where the hand drifts on its own.
		FHandPose Goal = RestPose(Side);
		if (SeatRole == EBackRoomRole::Player && Side == 1 && HandMode == 1 && StackPile && StackPile->GetAmount() > 0)
		{
			// Riffling the top chips of the stack.
			Goal = Touch(StackPile->GetTop() + FVector(0.0, 0.0, 0.4), FVector(-0.2, 1.0, -0.35), FVector(0.3, 0.0, -1.0), 0.45f, 0.5f);
			Goal.Curl = 0.45f + 0.2f * FMath::Sin(Time * 7.0f);
			Goal.Pinch = 0.5f + 0.4f * FMath::Sin(Time * 7.0f + 1.0f);
		}
		else if (SeatRole == EBackRoomRole::Player && Side == 0 && HandMode == 2 && bInHand && Hole.Num() > 0 && Hole[0])
		{
			// A hand guarding the cards, resting beside them.
			Goal = Touch(Hole[0]->GetActorLocation() + GetActorRightVector() * -5.0 + FVector(0.0, 0.0, 0.5), FVector(-0.35, 1.0, -0.2), FVector(0.2, 0.0, -1.0), 0.5f);
		}
		else if (SeatRole == EBackRoomRole::Hero && bHeroPeek && Hole.Num() == 2 && Hole[0] && Hole[1])
		{
			// Peeking: the left hand covers the cards, the right thumb lifts the near corner.
			const FVector C = (Hole[0]->GetActorLocation() + Hole[1]->GetActorLocation()) * 0.5;
			const FVector In = Spots.Inward;
			const FVector Left = -GetActorRightVector();
			Goal = Side == 0
				? Touch(C + In * 4.0 + Left * 6.5 + FVector(0.0, 0.0, 1.0), FVector(-0.65, 1.0, -0.2), FVector(0.1, 0.0, -1.0), 0.2f)
				: Touch(C - In * 4.3 - Left * 3.6 + FVector(0.0, 0.0, 0.8), FVector(0.35, 1.0, -0.3), FVector(0.65, 0.0, -0.75), 0.25f, 0.45f);
		}
		const float Rate = SeatRole == EBackRoomRole::Hero ? 7.0f : 3.0f;
		FHandPose& H = HandNow[Side];
		H.Pos = Ease(H.Pos, Goal.Pos, Rate, Dt);
		H.Palm = Ease(H.Palm, Goal.Palm, Rate, Dt).GetSafeNormal();
		H.Finger = Ease(H.Finger, Goal.Finger, Rate, Dt).GetSafeNormal();
		H.Curl = Ease(H.Curl, Goal.Curl, Rate * 2.0f, Dt);
		H.Thumb = Ease(H.Thumb, Goal.Thumb, Rate * 2.0f, Dt);
		H.Pinch = Ease(H.Pinch, Goal.Pinch, Rate * 2.0f, Dt);
		StepFrom[Side] = H;
	}

	// Idle hand habits: change every several seconds.
	HandSwitch -= Dt;
	if (HandSwitch <= 0.0f)
	{
		const float R = Rng.FRand();
		HandMode = R < 0.45f ? 0 : (R < 0.45f + 0.35f * Persona.ChipFidget ? 1 : 2);
		HandSwitch = Rng.FRandRange(5.0f, 14.0f);
	}

	// The hero's peek lifts the cards once the hands are on them.
	if (SeatRole == EBackRoomRole::Hero)
	{
		HeroPeekT = Ease(HeroPeekT, bHeroPeek && Steps[1].Num() == 0 ? 1.0f : 0.0f, 9.0f, Dt);
		PeekGoal = HeroPeekT > 0.8f ? 1.0f : 0.0f;
	}
	PeekNow = Ease(PeekNow, PeekGoal, 9.0f, Dt);
	if (PeekNow > 0.002f || PeekApplied > 0.002f)
	{
		for (ABackRoomCard* Card : Hole)
		{
			if (Card && !Card->IsFaceUp())
			{
				Card->SetPeek(PeekNow, GetEyes());
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
	const FVector In = Spots.Inward;
	const FVector Left = -GetActorRightVector();
	const float Hold = Rng.FRandRange(0.7f, 1.3f) * (1.0f - 0.3f * Persona.Nervousness);
	// Lean in and look down at the cards for the whole peek.
	LeanTarget = 0.95f;
	NextShift = Hold + 1.5f;
	GazeHold = C;
	GazeHoldLeft = 0.45f + Hold + 0.3f;
	HeadFollow = 0.85f;
	Queue(0, Touch(C + In * 4.0 + Left * 6.5 + FVector(0.0, 0.0, 1.0), FVector(-0.65, 1.0, -0.2), FVector(0.1, 0.0, -1.0), 0.2f), 0.4f, 3.0f);
	Queue(0, Touch(C + In * 4.0 + Left * 6.5 + FVector(0.0, 0.0, 1.0), FVector(-0.65, 1.0, -0.2), FVector(0.1, 0.0, -1.0), 0.2f), Hold + 0.3f);
	QueueRest(0, 0.45f);
	Queue(1, Touch(C - In * 4.3 - Left * 3.6 + FVector(0.0, 0.0, 0.8), FVector(0.35, 1.0, -0.3), FVector(0.65, 0.0, -0.75), 0.25f, 0.45f), 0.45f, 3.0f,
		[this]() {
			PeekGoal = 1.0f;
			ReactToHole();
		});
	Queue(1, Touch(C - In * 4.3 - Left * 3.6 + FVector(0.0, 0.0, 1.4), FVector(0.35, 1.0, -0.3), FVector(0.65, 0.0, -0.75), 0.25f, 0.55f), Hold,
		0.0f, [this]() { PeekGoal = 0.0f; });
	QueueRest(1, 0.45f);
	return 0.45f + Hold + 0.45f;
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
	TArray<ABackRoomCard*> Cards = {Hole[0], Hole[1]};
	Queue(1, Touch(C + Spots.Inward * 2.0 + FVector(0.0, 0.0, 0.6), FVector(0.0, 1.0, -0.2), FVector(0.0, 0.0, -1.0), 0.1f), 0.32f, 2.5f, [this, Cards]() {
		for (ABackRoomCard* Card : Cards)
		{
			Card->Stop();
			Grab(Card, 1, Card->GetActorTransform().GetRelativeTransform(HandFrame(1)));
		}
	});
	Queue(1, Touch(C + Spots.Inward * (bToss ? 14.0 : 22.0) + FVector(0.0, 0.0, bToss ? 6.0 : 0.6), FVector(0.0, 1.0, bToss ? 0.1 : -0.2), FVector(0.0, 0.0, -1.0), 0.1f), bToss ? 0.16f : 0.3f, 0.0f,
		[this, Cards, Muck, bToss]() {
			Sound(Snd(ss::SoundId::Fold), Cards[0]->GetActorLocation(), 0.8f);
			for (int32 I = 0; I < Cards.Num(); ++I)
			{
				ABackRoomCard* Card = Cards[I];
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
	for (int32 I = 0; I < 2; ++I)
	{
		ABackRoomCard* Card = Hole[I];
		const FVector At = Card->GetActorLocation();
		Queue(1, Touch(At - Spots.Inward * 3.0 + FVector(0.0, 0.0, 0.8), FVector(0.0, 1.0, -0.3), FVector(0.3, 0.0, -1.0), 0.3f, 0.5f), 0.3f, 2.0f,
			[this, Card]() {
				Card->Flip(true, 0.3f);
				Sound(Snd(ss::SoundId::Flip), Card->GetActorLocation(), 0.7f);
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
	Queue(1, Take, 0.13f, 1.0f, [this, Card]() {
		Card->Stop();
		Card->SetActorTransform(GetDeckTop());
		Card->SetActorHiddenInGame(false);
		Grab(Card, 1, Card->GetActorTransform().GetRelativeTransform(HandFrame(1)));
	});
	Queue(1, Flick, 0.13f, 0.0f, [this, Card, Target, Flight, bFaceUp]() {
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
	Queue(1, Touch(Reachable(From - Dir * 5.0 + FVector(0.0, 0.0, 1.5)), Finger, Palm, 0.2f), 0.3f, 3.0f, [Pile, To]() { Pile->SlideTo(To, 0.45f); });
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
	Queue(1, Touch(Reach + FVector(0.0, 0.0, 1.0), FVector(0.6, 0.6, -0.3), FVector(-0.4, -0.3, -0.8), 0.2f), 0.35f, 3.0f, [this, Cards, Muck]() {
		for (int32 I = 0; I < Cards.Num(); ++I)
		{
			if (ABackRoomCard* Card = Cards[I])
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
