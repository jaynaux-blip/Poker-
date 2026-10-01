#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"

#include "BackRoomGameMode.generated.h"

class ABackRoomPlayer;
class ABackRoomStage;
class ABackRoomTable;
class UCameraComponent;
class UPointLightComponent;

/**
 * You, at the Back Room's table: a camera in your own head.
 *
 * The body is an ABackRoomPlayer in the hero's seat (its face hidden from you): the camera rides its
 * head, the head turns where you look, and its hands do what you do. Hold Space (or the left mouse
 * button) to lift the corners of your cards; hold the right mouse button to study whoever you look at
 * (Focus: the view narrows, time slows, a face sharpens; it drains while you hold it). F folds, C
 * checks or calls, R bets or raises to the amount shown (the mouse wheel changes it), A goes all in.
 */
UCLASS()
class SHORTSTACK_API ABackRoomPawn : public APawn
{
	GENERATED_BODY()

public:
	ABackRoomPawn();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UPROPERTY(VisibleAnywhere, Category = "Short Stack")
	TObjectPtr<UCameraComponent> Camera;

	/** A faint fill on the lifted corner while you peek: the lamp's light, caught. */
	UPROPERTY(VisibleAnywhere, Category = "Short Stack")
	TObjectPtr<UPointLightComponent> PeekLight;

	/** Degrees of head turn available from facing the dealer. */
	UPROPERTY(EditAnywhere, Category = "Short Stack")
	float MaxYaw = 80.0f;
	UPROPERTY(EditAnywhere, Category = "Short Stack")
	float MinPitch = -60.0f;
	UPROPERTY(EditAnywhere, Category = "Short Stack")
	float MaxPitch = 30.0f;

	/** Where the head points (degrees from facing the dealer); the camera eases toward it. */
	UPROPERTY(EditAnywhere, Category = "Short Stack")
	float Yaw = 0.0f;
	UPROPERTY(EditAnywhere, Category = "Short Stack")
	float Pitch = -14.0f;

	/** Hold the peek or Focus without input (for testing from scripts). */
	UPROPERTY(EditAnywhere, Category = "Short Stack|Test")
	bool bTestPeek = false;
	UPROPERTY(EditAnywhere, Category = "Short Stack|Test")
	bool bTestFocus = false;

	/** 0..1: how much Focus is left, and how far in it you are. */
	float FocusLeft = 1.0f;
	float Focus = 0.0f;
	/** Who Focus is on (null when nobody). */
	TWeakObjectPtr<ABackRoomPlayer> Studying;

private:
	void HandleInput(float RealDt);
	ABackRoomTable* GetTable() const;

	FRotator Smoothed = FRotator(-14.0f, 0.0f, 0.0f);
	FVector Seat = FVector::ZeroVector;
	float Time = 0.0f;
	float PeekBlend = 0.0f;
	bool bPeekReported = false;
};

/** Prompts, stacks, Focus and the table talk, drawn over the view. */
UCLASS()
class SHORTSTACK_API ABackRoomHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	struct FShown
	{
		FString Speaker;
		FString Text;
		float Until = 0.0f;
	};
	TArray<FShown> Subtitles;
	float Clock = 0.0f;
};

/** The Back Room: seats the cast at the table, deals Dee in, and starts the Tuesday game. */
UCLASS()
class SHORTSTACK_API ABackRoomGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABackRoomGameMode();

	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual void StartPlay() override;

	UPROPERTY(Transient)
	TObjectPtr<ABackRoomStage> Stage;

	UPROPERTY(Transient)
	TObjectPtr<ABackRoomTable> Table;

	UPROPERTY(Transient)
	TObjectPtr<ABackRoomPlayer> Hero;

	UPROPERTY(Transient)
	TObjectPtr<ABackRoomPlayer> Dealer;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ABackRoomPlayer>> Opponents;

private:
	ABackRoomStage* FindOrSpawnStage();
	void SeatEveryone();
};
