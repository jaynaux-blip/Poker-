#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NightOneGame.h"

#include "NightOneGameMode.generated.h"

class ANightOnePawn;
class ANightOneStage;
class SNightOneOverlay;
class UNightOneAudio;

/**
 * SHORT STACK: Night One. Boots the apartment, the RiverLine client on the
 * laptop, audio and the game session, and routes input between the room and
 * the screen (port of web/src/main.ts). Works in any level: if no
 * NightOneStage is placed, one is spawned at the origin.
 */
UCLASS()
class SHORTSTACK_API ANightOneGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ANightOneGameMode();
	virtual ~ANightOneGameMode() override;

	virtual void StartPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void RestartPlayer(AController* NewPlayer) override;

	// Input from ANightOnePlayerController.
	void OnMouse(bool bOverScreen, const FVector2D& Client, float Nx, float Ny);
	void OnPress(bool bOverScreen);
	void OnRelease();
	void OnWheel(float Delta);
	void OnKey(const FString& Key);
	bool HideCursor(bool bOverScreen) const;
	bool IsLeanedBack() const;

	ANightOneStage* GetStage() const { return Stage; }
	ANightOnePawn* GetSeat() const;
	UNightOneAudio* GetAudio() const { return Audio; }
	void ShowToast(const FString& From, const FString& Body);

	/** Game time multiplier (for testing long tournaments). */
	UPROPERTY(EditAnywhere, Category = "Short Stack")
	float TimeScale = 1.0f;

private:
	void Begin(const FString& Name);

	UPROPERTY(Transient)
	TObjectPtr<ANightOneStage> Stage;

	UPROPERTY(VisibleAnywhere, Category = "Short Stack")
	TObjectPtr<UNightOneAudio> Audio;

	TUniquePtr<FNightOneGame> Game;
	TSharedPtr<SNightOneOverlay> Overlay;
	bool bStarted = false;
	double RealTime = 0.0;
	double GameTime = 0.0;
	double BeganAt = -1.0;
	double UiAccum = 1.0;
	double PhoneAccum = 1.0;
	float Tilt = 0.0f;
	float Pulse = 0.0f;
	bool bOverScreenNow = false;
};
