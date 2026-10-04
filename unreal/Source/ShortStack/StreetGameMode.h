#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/UI/FrontEnd.h"
#include "ShortStack/UI/StoreCounter.h"
#include "SlateDrawList.h"

#include "StreetGameMode.generated.h"

class AShortStackCharacter;
class AStreetGameMode;
class AStreetStage;
class SFrontEndWidget;
class UNightOneAudio;

/** The career out on the street: the session from the save, the store's counter, the pause menu, and their hooks. */
class FStreetGame final : public ss::SessionHooks, public ss::ui::FrontEndHooks
{
public:
	FStreetGame(AStreetGameMode& InMode, const ss::SaveData* Loaded, const std::string& Seed);

	AStreetGameMode& Mode;
	ss::Session Session;
	ss::ui::StoreCounter Counter;
	ss::ui::FrontEnd Menu;
	FSlateTextMeasurer Measurer;

	// ss::SessionHooks
	virtual void Sound(ss::SoundId Id, double Volume) override;
	virtual void Text(const std::string& From, const std::string& Body) override;
	virtual void Save(const ss::SaveData& Data) override;

	// ss::ui::FrontEndHooks (the pause menu)
	virtual void UiSound(ss::SoundId Id, double Volume) override;
	virtual void Resume() override;
	virtual void QuitToMenu() override;
	virtual void QuitGame() override;
	virtual void SettingsChanged(const ss::ui::GameSettings& Settings) override;
};

/**
 * The Street level: out of the apartment's door onto Fifth, down to the Lucky Penny #212 and back. The
 * player walks as AShortStackCharacter (V switches first and third person); E uses what's in reach (the
 * counter, the cooler, the building's door home); F eats or drinks from the bag; Escape pauses. The
 * session runs here as it does at the desk (the clock, the needs, texts), saved to the same slot.
 *
 * Options: "From=<world minutes>" when the apartment sent the player out; "Start=store" to begin inside.
 */
UCLASS()
class SHORTSTACK_API AStreetGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AStreetGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void RestartPlayer(AController* NewPlayer) override;

	// From AStreetPlayerController, each frame or on a key.
	void OnMove(float Forward, float Right, bool bRun);
	void OnLook(float DeltaYaw, float DeltaPitch);
	void OnToggleView();
	void OnInteract();
	void OnEat();
	void OnPause();
	void OnZoom(float Delta);
	/** The counter or the pause menu has the keyboard (the controller leaves input to it). */
	bool IsUiOpen() const;

	/** Saves and goes back up to the apartment. */
	void GoHome();
	void Toast(const FString& From, const FString& Body);
	void SaveSettings(const ss::ui::GameSettings& Settings);
	void ReturnInputToGame();
	UNightOneAudio* GetAudio() const { return Audio; }

	/** Look sensitivity (degrees per mouse count) and inverted look, from the settings. */
	float Sensitivity = 0.11f;
	bool bInvertLook = false;

	// Automation (scripts and the desktop's checks).
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestDescribe() const;
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestTeleport(const FString& Where);

private:
	AShortStackCharacter* GetHero() const;
	void FindOrSpawnStage();
	void DressHero();
	void SpawnClerk();
	void CreateWidgets();
	void DrawHud();
	void DrawOverlay(SFrontEndWidget* Widget, TFunctionRef<void(ss::ui::Canvas&)> Paint);
	void OpenCounter(int32 Shelf);
	void FocusWidget(SFrontEndWidget* Widget);
	FVector2D ViewSize() const;

	UPROPERTY(VisibleAnywhere, Category = "Short Stack")
	TObjectPtr<UNightOneAudio> Audio;
	UPROPERTY(Transient)
	TObjectPtr<AStreetStage> Stage;
	UPROPERTY(Transient)
	TObjectPtr<AShortStackCharacter> Clerk;

	TUniquePtr<FStreetGame> Game;
	TSharedPtr<SDrawListWidget> HudWidget;
	TSharedPtr<SFrontEndWidget> CounterWidget;
	TSharedPtr<SFrontEndWidget> MenuWidget;
	TArray<ss::ui::StreetHudInfo::Toast> Toasts;
	FString StartFrom;
	double RealTime = 0.0;
	double HintsAt = 1.5;
	double LeftAt = 0.0;
	bool bGoingHome = false;
	double HomeAt = 0.0;
	bool bDressed = false;
	bool bGamepad = false;
};

/** Keyboard, mouse and gamepad polled each frame for walking (the overlays take their own input). */
UCLASS()
class SHORTSTACK_API AStreetPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AStreetPlayerController();
	virtual void PlayerTick(float DeltaTime) override;
};
