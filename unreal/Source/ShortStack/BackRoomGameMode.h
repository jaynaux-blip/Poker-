#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"

#include "BackRoomGameMode.generated.h"

class ABackRoomPlayer;
class ABackRoomStage;
class UCameraComponent;

/** The player's seat at the Back Room's table: a first-person camera that looks around like a head. */
UCLASS()
class SHORTSTACK_API ABackRoomPawn : public APawn
{
	GENERATED_BODY()

public:
	ABackRoomPawn();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "Short Stack")
	TObjectPtr<UCameraComponent> Camera;

	/** Degrees of head turn available from facing the dealer. */
	UPROPERTY(EditAnywhere, Category = "Short Stack")
	float MaxYaw = 75.0f;
	UPROPERTY(EditAnywhere, Category = "Short Stack")
	float MinPitch = -55.0f;
	UPROPERTY(EditAnywhere, Category = "Short Stack")
	float MaxPitch = 30.0f;

	/** Where the head points (degrees from facing the dealer); the camera eases toward it. */
	float Yaw = 0.0f;
	float Pitch = -12.0f;

private:
	FRotator Smoothed = FRotator(-12.0f, 0.0f, 0.0f);
	FVector Seat = FVector::ZeroVector;
	float Time = 0.0f;
};

/** The Back Room's rules for now: seat the player at the table and fill the other seats. */
UCLASS()
class SHORTSTACK_API ABackRoomGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABackRoomGameMode();

	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual void StartPlay() override;

	/** Seats to fill with opponents when the map has none (0 is the player's). */
	UPROPERTY(EditAnywhere, Category = "Short Stack")
	TArray<int32> OpponentSeats = {2, 3, 4, 5, 6};

	UPROPERTY(Transient)
	TObjectPtr<ABackRoomStage> Stage;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ABackRoomPlayer>> Opponents;

private:
	ABackRoomStage* FindOrSpawnStage();
};
