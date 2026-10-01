#include "BackRoomGameMode.h"

#include "BackRoomPlayer.h"
#include "BackRoomStage.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

// ------------------------------------------------------------------ pawn

ABackRoomPawn::ABackRoomPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	RootComponent = Camera;
	Camera->SetFieldOfView(72.0f);
	Camera->bUsePawnControlRotation = false;
	AutoPossessPlayer = EAutoReceiveInput::Disabled;
}

void ABackRoomPawn::BeginPlay()
{
	Super::BeginPlay();
	Seat = GetActorLocation();
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}
}

void ABackRoomPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		float Dx = 0.0f, Dy = 0.0f;
		PC->GetInputMouseDelta(Dx, Dy);
		Yaw = FMath::Clamp(Yaw + Dx * 0.9f, -MaxYaw, MaxYaw);
		Pitch = FMath::Clamp(Pitch + Dy * 0.9f, MinPitch, MaxPitch);
	}
	// A head, not a gimbal: it eases after the mouse, and the body breathes under it.
	Smoothed = FMath::RInterpTo(Smoothed, FRotator(Pitch, Yaw, 0.0f), DeltaSeconds, 14.0f);
	const float Breath = FMath::Sin(Time * 1.35f);
	SetActorLocationAndRotation(Seat + FVector(0.25 * Breath, 0.0, 0.35 * Breath), Smoothed + FRotator(0.2f * Breath, 0.0f, 0.15f * FMath::Sin(Time * 0.37f)));
}

// ------------------------------------------------------------------ game mode

ABackRoomGameMode::ABackRoomGameMode()
{
	DefaultPawnClass = ABackRoomPawn::StaticClass();
}

ABackRoomStage* ABackRoomGameMode::FindOrSpawnStage()
{
	if (Stage)
	{
		return Stage;
	}
	for (TActorIterator<ABackRoomStage> It(GetWorld()); It; ++It)
	{
		Stage = *It;
		return Stage;
	}
	Stage = GetWorld()->SpawnActor<ABackRoomStage>(ABackRoomStage::StaticClass(), FTransform::Identity);
	return Stage;
}

void ABackRoomGameMode::RestartPlayer(AController* NewPlayer)
{
	if (!NewPlayer || NewPlayer->IsPendingKillPending())
	{
		return;
	}
	ABackRoomStage* S = FindOrSpawnStage();
	const FVector Eye = S ? S->EyeLocation() : FVector(-98.0, 0.0, 124.0);
	APawn* Pawn = SpawnDefaultPawnAtTransform(NewPlayer, FTransform(FRotator(-12.0f, 0.0f, 0.0f), Eye));
	if (Pawn)
	{
		NewPlayer->SetPawn(Pawn);
		NewPlayer->Possess(Pawn);
		NewPlayer->ClientSetRotation(Pawn->GetActorRotation(), true);
		FinishRestartPlayer(NewPlayer, Pawn->GetActorRotation());
	}
}

void ABackRoomGameMode::StartPlay()
{
	Super::StartPlay();
	ABackRoomStage* S = FindOrSpawnStage();
	for (TActorIterator<ABackRoomPlayer> It(GetWorld()); It; ++It)
	{
		Opponents.Add(*It);
	}
	if (Opponents.Num() == 0)
	{
		// Each opponent gets their own temperament until the cast is written.
		auto Make = [](const TCHAR* Name, float Nerves, float Expressive, float Restless, float Fidget, float Posture, int32 Seed) {
			FBackRoomPersona P;
			P.Name = Name;
			P.Nervousness = Nerves;
			P.Expressiveness = Expressive;
			P.Restlessness = Restless;
			P.ChipFidget = Fidget;
			P.Posture = Posture;
			P.Seed = Seed;
			return P;
		};
		const FBackRoomPersona Personas[] = {
			Make(TEXT("Sal"), 0.2f, 0.35f, 0.4f, 0.6f, 0.3f, 11),
			Make(TEXT("Marisol"), 0.45f, 0.6f, 0.6f, 0.3f, 0.6f, 23),
			Make(TEXT("Dee"), 0.1f, 0.2f, 0.3f, 0.2f, 0.55f, 37),
			Make(TEXT("Twitch"), 0.8f, 0.75f, 0.9f, 0.9f, 0.7f, 41),
			Make(TEXT("Big Lou"), 0.25f, 0.5f, 0.35f, 0.5f, 0.2f, 53),
		};
		int32 K = 0;
		for (int32 Seat : OpponentSeats)
		{
			ABackRoomPlayer* P = GetWorld()->SpawnActorDeferred<ABackRoomPlayer>(ABackRoomPlayer::StaticClass(), ABackRoomStage::SeatTransform(Seat));
			P->Persona = Personas[K % UE_ARRAY_COUNT(Personas)];
			P->FinishSpawning(ABackRoomStage::SeatTransform(Seat));
			Opponents.Add(P);
			++K;
		}
	}
	// Everyone knows where the others sit: gaze targets at head height.
	const FVector Eye = S ? S->EyeLocation() : FVector(-98.0, 0.0, 124.0);
	for (ABackRoomPlayer* P : Opponents)
	{
		P->HeroEyes = Eye;
		P->PotAt = FVector(0.0, 0.0, ABackRoomStage::FeltZ);
		P->OthersAt.Reset();
		for (ABackRoomPlayer* Other : Opponents)
		{
			if (Other != P)
			{
				P->OthersAt.Add(Other->GetActorTransform().TransformPosition(FVector(-14.0, 0.0, 112.0)));
			}
		}
		P->DealerAt = ABackRoomStage::SeatTransform(4).TransformPosition(FVector(-14.0, 0.0, 112.0));
	}
}
