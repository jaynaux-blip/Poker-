#include "BackRoomPlayer.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace BackRoomPlayerDetail
{
const TCHAR* ArchetypeBody = TEXT("/MetaHumanCharacter/Body/IdentityTemplate/SKM_Body.SKM_Body");
const TCHAR* ArchetypeFace = TEXT("/MetaHumanCharacter/Face/SKM_Face.SKM_Face");

// The chair: the actor sits on its front edge; the pelvis rests SeatDepth behind it.
const double SeatDepth = 20.0;
const float SeatHeight = 46.0f;
// The rail's outer edge, ahead of the chair's front edge (ABackRoomStage seats players this far out),
// and in the body's space (+Y toward the table): the rail is 13 cm wide, the felt beyond it.
const double RailGap = 22.0;
const double Rail = RailGap + SeatDepth;

/** Critically damped approach of X toward Target at Rate (per second). */
float Ease(float X, float Target, float Rate, float Dt)
{
	return FMath::Lerp(X, Target, 1.0f - FMath::Exp(-Rate * Dt));
}

FVector Ease(const FVector& X, const FVector& Target, float Rate, float Dt)
{
	return FMath::Lerp(X, Target, 1.0f - FMath::Exp(-Rate * Dt));
}
} // namespace BackRoomPlayerDetail

using namespace BackRoomPlayerDetail;

ABackRoomPlayer::ABackRoomPlayer()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	// MetaHuman bodies face +Y: turn the mesh so it faces the table (the actor's +X), pelvis over the seat.
	Body->SetRelativeLocationAndRotation(FVector(-SeatDepth, 0.0, 0.0), FRotator(0.0f, -90.0f, 0.0f));
	Body->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	Body->SetAnimInstanceClass(UBackRoomBodyAnim::StaticClass());
	Body->SetUpdateAnimationInEditor(true);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Face = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Face"));
	Face->SetupAttachment(Body);
	Face->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	Face->SetAnimInstanceClass(UBackRoomFaceAnim::StaticClass());
	Face->SetUpdateAnimationInEditor(true);
	Face->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	ChairSeat = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChairSeat"));
	ChairSeat->SetupAttachment(Root);
	ChairSeat->SetStaticMesh(Cube.Object);
	ChairSeat->SetRelativeLocation(FVector(-SeatDepth, 0.0, SeatHeight - 2.0));
	ChairSeat->SetRelativeScale3D(FVector(0.42, 0.44, 0.04));
	ChairSeat->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ChairBack = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChairBack"));
	ChairBack->SetupAttachment(Root);
	ChairBack->SetStaticMesh(Cube.Object);
	ChairBack->SetRelativeLocationAndRotation(FVector(-SeatDepth - 23.0, 0.0, SeatHeight + 26.0), FRotator(-8.0f, 0.0f, 0.0f));
	ChairBack->SetRelativeScale3D(FVector(0.03, 0.42, 0.4));
	ChairBack->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABackRoomPlayer::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
}

void ABackRoomPlayer::Build()
{
	USkeletalMesh* BodyAsset = BodyMesh.IsNull() ? LoadObject<USkeletalMesh>(nullptr, ArchetypeBody) : BodyMesh.LoadSynchronous();
	USkeletalMesh* FaceAsset = FaceMesh.IsNull() ? LoadObject<USkeletalMesh>(nullptr, ArchetypeFace) : FaceMesh.LoadSynchronous();
	if (BodyAsset && Body->GetSkeletalMeshAsset() != BodyAsset)
	{
		Body->SetSkeletalMeshAsset(BodyAsset);
	}
	if (FaceAsset && Face->GetSkeletalMeshAsset() != FaceAsset)
	{
		Face->SetSkeletalMeshAsset(FaceAsset);
	}
	Face->AddTickPrerequisiteComponent(Body);
	if (UBackRoomFaceAnim* FaceAnim = Cast<UBackRoomFaceAnim>(Face->GetAnimInstance()))
	{
		FaceAnim->Body = Body;
	}
	Rng.Initialize(Persona.Seed * 7919 + 17);
	for (int32 Side = 0; Side < 2; ++Side)
	{
		HandAt[Side] = HandGoal[Side] = FVector(Side == 0 ? 12.0 : -12.0, Rail + 17.0, 80.5);
	}
	EyeAt = HeadAt = GazeTarget = HeroEyes;
	Lean = LeanTarget = 0.3f + 0.45f * Persona.Posture;
}

FVector ABackRoomPlayer::ToBody(const FVector& World) const
{
	return Body->GetComponentTransform().InverseTransformPosition(World);
}

void ABackRoomPlayer::SetTestExpression(FName Control, float Value)
{
	TestCurves.Add(FName(*(TEXT("CTRL_expressions_") + Control.ToString())), Value);
}

void ABackRoomPlayer::ClearTestExpressions()
{
	TestCurves.Reset();
}

// ------------------------------------------------------------------ behavior

void ABackRoomPlayer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Dt = FMath::Min(DeltaSeconds, 0.1f);
	Time += Dt;
	UpdateBody(Dt);
	UpdateFace(Dt);
}

void ABackRoomPlayer::UpdateBody(float Dt)
{
	UBackRoomBodyAnim* Anim = Cast<UBackRoomBodyAnim>(Body->GetAnimInstance());
	if (!Anim)
	{
		return;
	}
	// Breathing: faster and deeper when aroused.
	const float Rate = 11.0f + 9.0f * Arousal + 4.0f * Persona.Nervousness; // breaths per minute
	BreathPhase += Dt * Rate / 60.0f * UE_TWO_PI;
	const float Breath = FMath::Sin(BreathPhase) * (0.6f + 0.6f * Arousal);

	// Posture: settles, then shifts every so often; leans in when engaged or confident.
	NextShift -= Dt;
	if (NextShift <= 0.0f)
	{
		LeanTarget = FMath::Clamp(0.3f + 0.45f * Persona.Posture + 0.25f * Dominance * Arousal + Rng.FRandRange(-0.15f, 0.15f), 0.0f, 1.0f);
		NextShift = Rng.FRandRange(7.0f, 22.0f);
	}
	Lean = Ease(Lean, LeanTarget, 0.8f, Dt);

	// Gaze: a new target every second or few; the eyes jump, the head follows.
	GazeLeft -= Dt;
	if (GazeLeft <= 0.0f)
	{
		const FTransform& T = GetActorTransform();
		const FVector Cards = T.TransformPosition(FVector(RailGap + 27.0, 4.0, 76.0));
		const FVector Chips = T.TransformPosition(FVector(RailGap + 22.0, -18.0, 78.0));
		const FVector Away = T.TransformPosition(FVector(60.0, Rng.FRandRange(-80.0f, 80.0f), 20.0));
		struct FChoice { FVector At; float W; };
		TArray<FChoice, TInlineAllocator<8>> Choices = {
			{HeroEyes, 0.3f + 0.3f * Dominance}, {PotAt, 0.25f}, {Cards, 0.12f}, {Chips, 0.1f}, {DealerAt, 0.1f}, {Away, 0.12f * Persona.Nervousness}};
		for (const FVector& Other : OthersAt)
		{
			Choices.Add({Other, 0.12f});
		}
		float Sum = 0.0f;
		for (const FChoice& C : Choices)
		{
			Sum += C.W;
		}
		float Pick = Rng.FRandRange(0.0f, Sum);
		for (const FChoice& C : Choices)
		{
			Pick -= C.W;
			if (Pick <= 0.0f)
			{
				GazeTarget = C.At;
				break;
			}
		}
		GazeTarget += FVector(Rng.FRandRange(-4.0f, 4.0f), Rng.FRandRange(-4.0f, 4.0f), Rng.FRandRange(-3.0f, 3.0f));
		GazeLeft = Rng.FRandRange(0.7f, 4.5f) * (1.0f - 0.55f * Persona.Restlessness);
	}
	// Microsaccades: the eyes never sit perfectly still.
	const FVector Jitter = 0.6f * FVector(FMath::Sin(Time * 3.1f + Persona.Seed), FMath::Sin(Time * 2.3f + 1.7f * Persona.Seed), FMath::Sin(Time * 2.9f));
	EyeAt = Ease(EyeAt, GazeTarget + Jitter, 22.0f, Dt);
	HeadAt = Ease(HeadAt, GazeTarget, 2.6f, Dt);

	// Hands: forearms on the rail, a hand guarding the cards, or fingers on the chips.
	HandSwitch -= Dt;
	if (HandSwitch <= 0.0f)
	{
		const float R = Rng.FRand();
		HandMode = R < 0.45f ? 0 : (R < 0.45f + 0.35f * Persona.ChipFidget ? 1 : 2);
		HandSwitch = Rng.FRandRange(5.0f, 14.0f);
	}
	HandGoal[0] = HandMode == 2 ? FVector(2.0, Rail + 26.0, 79.0) : FVector(12.0, Rail + 17.0, 80.5);
	HandGoal[1] = HandMode == 1 ? FVector(-19.0, Rail + 21.0, 81.5) : FVector(-12.0, Rail + 17.0, 80.5);
	for (int32 Side = 0; Side < 2; ++Side)
	{
		HandAt[Side] = Ease(HandAt[Side], HandGoal[Side], 3.0f, Dt);
	}

	FBackRoomBodyPose& P = Anim->Pose;
	P.SeatHeight = SeatHeight;
	P.Time = Time;
	P.Lean = Lean;
	P.Slouch = FMath::Clamp(0.25f - 0.3f * Dominance + 0.3f * (1.0f - Arousal) * 0.5f, 0.0f, 1.0f);
	P.Breath = Breath;
	P.ShoulderRaise = FMath::Clamp(0.5f * Arousal * (1.0f - Dominance) + 0.3f * Persona.Nervousness - 0.2f, 0.0f, 1.0f);
	P.LookAt = ToBody(HeadAt);
	P.HeadFollow = 0.75f;
	P.Twist = 0.0f;
	P.HeadTilt = 2.5f * FMath::Sin(Time * 0.21f + Persona.Seed);
	P.HeadNod = 1.2f * FMath::Sin(Time * 0.33f + 2.0f * Persona.Seed) + 1.5f * Breath * 0.3f;
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const float Sx = Side == 0 ? 1.0f : -1.0f;
		P.HandPos[Side] = HandAt[Side];
		P.PalmDir[Side] = FVector(Sx * 0.15f, 0.0f, -1.0f);
		P.FingerDir[Side] = FVector(-Sx * 0.45f, 1.0f, -0.1f);
		P.HandWeight[Side] = 1.0f;
		P.Curl[Side] = 0.35f + 0.1f * Persona.Nervousness;
		P.ThumbCurl[Side] = 0.2f;
		P.Pinch[Side] = 0.0f;
	}
	// Riffling: the fingers work the top chips.
	if (HandMode == 1)
	{
		P.Curl[1] = 0.45f + 0.2f * FMath::Sin(Time * 7.0f);
		P.Pinch[1] = 0.5f + 0.4f * FMath::Sin(Time * 7.0f + 1.0f);
	}
	P.Tremble = 0.0f;
	P.KneeSpread = 0.2f + 0.5f * Dominance;
}

void ABackRoomPlayer::UpdateFace(float Dt)
{
	UBackRoomFaceAnim* Anim = Cast<UBackRoomFaceAnim>(Face->GetAnimInstance());
	if (!Anim)
	{
		return;
	}
	Anim->Body = Body;
	TMap<FName, float> C;
	auto Add = [&C](const TCHAR* Control, float Value) {
		if (Value > 0.0005f)
		{
			float& V = C.FindOrAdd(FName(*FString::Printf(TEXT("CTRL_expressions_%s"), Control)));
			V = FMath::Clamp(V + Value, 0.0f, 1.0f);
		}
	};
	auto Both = [&Add](const TCHAR* Control, float Value) {
		Add(*FString::Printf(TEXT("%sL"), Control), Value);
		Add(*FString::Printf(TEXT("%sR"), Control), Value);
	};

	// Blinks: 12-25 a minute, more when anxious, now and then a double.
	NextBlink -= Dt;
	if (NextBlink <= 0.0f && BlinkT < 0.0f)
	{
		BlinkT = 0.0f;
		const float PerMinute = 12.0f + 14.0f * Arousal * (1.0f - Dominance) + 6.0f * Persona.Nervousness;
		NextBlink = Rng.FRandRange(0.3f, 2.0f) * 60.0f / PerMinute;
		bDoubleBlink = Rng.FRand() < 0.12f;
	}
	if (BlinkT >= 0.0f)
	{
		BlinkT += Dt / 0.17f;
		const float B = BlinkT < 1.0f ? FMath::Pow(FMath::Sin(BlinkT * UE_PI), 0.6f) : 0.0f;
		Both(TEXT("eyeBlink"), B);
		if (BlinkT >= 1.0f)
		{
			BlinkT = bDoubleBlink ? 0.0f : -1.0f;
			bDoubleBlink = false;
		}
	}

	// The resting face: never quite still, a touch of asymmetry, lips just parted.
	Both(TEXT("eyeRelax"), 0.12f);
	Add(TEXT("jawOpen"), 0.02f + 0.02f * FMath::Max(0.0f, FMath::Sin(BreathPhase)));
	Add(TEXT("mouthCornerPullL"), 0.03f + 0.02f * FMath::Sin(Time * 0.4f + Persona.Seed));

	// Emotion, damped by the poker face.
	const float E = Persona.Expressiveness;
	const float Happy = FMath::Max(Valence, 0.0f) * E;
	const float Low = FMath::Max(-Valence, 0.0f) * E;
	Both(TEXT("mouthCornerPull"), 0.5f * Happy);
	Both(TEXT("eyeCheekRaise"), 0.4f * Happy);
	Both(TEXT("browRaiseIn"), 0.45f * Low * (1.0f - Dominance) + 0.2f * Arousal * (1.0f - Dominance) * E);
	Both(TEXT("mouthCornerDepress"), 0.3f * Low * (1.0f - Dominance));
	Both(TEXT("browDown"), 0.5f * Low * Dominance * Arousal);
	Both(TEXT("jawClench"), 0.4f * Low * Dominance * Arousal);
	Both(TEXT("noseNostrilDilate"), 0.3f * Arousal * (0.5f + 0.5f * Low));
	Both(TEXT("mouthStretch"), 0.12f * Arousal * (1.0f - Dominance) * E);
	Both(TEXT("eyePupilWide"), 0.35f * Arousal);
	// Breathing through the throat.
	Add(TEXT("neckThroatInhale"), 0.12f * FMath::Max(0.0f, FMath::Sin(BreathPhase)) * (0.4f + Arousal));

	// Micro-expressions: a flash too quick to hide.
	NextMicro -= Dt;
	if (NextMicro <= 0.0f && MicroT < 0.0f)
	{
		MicroT = 0.0f;
		MicroKind = Rng.RandRange(0, 3);
		NextMicro = Rng.FRandRange(5.0f, 14.0f);
	}
	if (MicroT >= 0.0f)
	{
		MicroT += Dt / 0.45f;
		const float M = MicroT < 1.0f ? FMath::Sin(MicroT * UE_PI) : 0.0f;
		switch (MicroKind)
		{
		case 0: Both(TEXT("mouthLipsPress"), 0.5f * M); break;
		case 1: Both(TEXT("browRaiseOuter"), 0.35f * M); break;
		case 2: Add(TEXT("mouthCornerPullR"), 0.35f * M); Add(TEXT("mouthDimpleR"), 0.3f * M); break;
		default: Both(TEXT("noseWrinkle"), 0.25f * M); break;
		}
		if (MicroT >= 1.0f)
		{
			MicroT = -1.0f;
		}
	}

	for (const TPair<FName, float>& T : TestCurves)
	{
		C.Add(T.Key, T.Value);
	}
	Anim->Curves = MoveTemp(C);
	Anim->LookAt = ToBody(EyeAt);
}
