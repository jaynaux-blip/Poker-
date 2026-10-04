#pragma once

#include "CareerSave.h"
#include "CoreMinimal.h"
#include "FrameBudget.h"
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
	/** The career's own save (CareerSave.h, the same file the desk and the Back Room use), written on a worker. */
	FCareerSaver Saver;
	/** Whether there was a career to walk out with (the level opened on its own, in the editor, has none). */
	bool bCareer = false;
	ss::Session Session;
	ss::ui::StoreCounter Counter;
	ss::ui::FrontEnd Menu;
	FSlateTextMeasurer Measurer;

	// ss::SessionHooks
	virtual void Sound(ss::SoundId Id, double Volume) override;
	virtual void Text(const std::string& From, const std::string& Body) override;
	virtual void Save(const ss::SaveData& Data) override;
	/** The world's text is written on the saver's worker (a purchase doesn't hitch the frame). */
	virtual bool SavesInBackground() const override { return true; }

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
 * session runs here as it does at the desk (the clock, the needs, texts), saved to the same career file.
 * The player's settings apply here as at the desk (quality, render scale, the lens, field of view, the mix).
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
	/** No career to walk out with: the title has been asked for. */
	bool bBackToTitle = false;
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
	/** The last device used was a gamepad (the HUD shows its buttons). */
	void SetGamepad(bool bInGamepad) { bGamepad = bInGamepad; }

	/** Saves and goes back up to the apartment. */
	void GoHome();
	void Toast(const FString& From, const FString& Body);
	/** The settings from the pause menu (or the saved ones at the start): applied now, saved once the player stops changing them. */
	void ApplySettings(const ss::ui::GameSettings& Settings, bool bSave);
	void SaveSettingsNow();
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
	/** Benny behind the counter (the character looks after him from there: his look at the player, his blinks). */
	void SpawnClerk();
	/** A player who has somehow left the set (fallen through, walked off an end) is put back at the building's door. */
	void KeepOnTheSet();
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
	/** Dynamic resolution, as the player set it (FrameBudget.h). */
	FFrameBudget FrameBudget;
	FString StartFrom;
	double RealTime = 0.0;
	double HintsAt = 1.5;
	double LeftAt = 0.0;
	bool bGoingHome = false;
	double HomeAt = 0.0;
	bool bDressed = false;
	bool bGamepad = false;
	/** When the settings last changed (written once they've settled), or negative. */
	double SettingsDirtyAt = -1.0;
	/** The rain's level in the mix: lower inside the store, behind the glass. */
	float AmbienceDuck = 1.0f;
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
