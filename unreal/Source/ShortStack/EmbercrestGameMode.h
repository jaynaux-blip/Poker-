#pragma once

#include "CoreMinimal.h"
#include "EmbercrestStage.h"
#include "FrameBudget.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/UI/FrontEnd.h"
#include "ShortStack/UI/StoreCounter.h"
#include "SlateDrawList.h"

#include "EmbercrestGameMode.generated.h"

class AEmbercrestGameMode;
class AEmbercrestStage;
class AShortStackCharacter;
class FEmbercrestAmbience;
class SFrontEndWidget;
class UNightOneAudio;

/** The pause menu's hooks out here (the career isn't written from the Embercrest: the card room owns the night's saves). */
class FEmbercrestGame final : public ss::ui::FrontEndHooks
{
public:
	explicit FEmbercrestGame(AEmbercrestGameMode& InMode) : Mode(InMode), Menu(*this) {}

	AEmbercrestGameMode& Mode;
	ss::ui::FrontEnd Menu;
	FSlateTextMeasurer Measurer;

	virtual void UiSound(ss::SoundId Id, double Volume) override;
	virtual void Resume() override;
	virtual void QuitToMenu() override;
	virtual void QuitGame() override;
	virtual void SettingsChanged(const ss::ui::GameSettings& Settings) override;
};

/** Where the exterior is in its flow (spec 5). */
UENUM()
enum class EEcxFlow : uint8
{
	/** The arrival's shots: the crane over the valley, then the stop as the bus pulls in. */
	Cinematic,
	/** Off the bus onto the platform; the camera settles behind the hero. */
	StepOff,
	Playing,
	/** Through the doors into the card room (the fade). */
	Entering,
	/** Onto the bus for home (the fade). */
	Boarding,
	/** At the overlook or the viewer: the view out over the valley for a moment. */
	Looking,
};

/**
 * THE EMBERCREST from outside (design spec ec_exterior_spec.md, section 5): the 14 bus arrives at the resort's stop, the
 * hero steps off and walks up the plaza under the porte-cochere to the doors (E: into the card room, "BackRoom" with the
 * options this level was given); after the night, out through the same doors to the bus waiting at the stop (E at its
 * door: home to Night One with the card room's options, less "Leaving"). The hero is the street's character dressed from
 * the save, with the street's controls (WASD, Shift, mouse or stick look, V, E, the wheel, Escape or Start to pause).
 *
 * Options: "Live=<occurrence id>" arriving for an event (from Night One); the card room's home options plus "Leaving" on
 * the way home; none opens it to walk around (the editor, a test: the doors open the practice table). The career save is
 * read, never written: the card room owns the night (spec 5.1); the settings slot is the player's as everywhere.
 */
UCLASS()
class SHORTSTACK_API AEmbercrestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AEmbercrestGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void RestartPlayer(AController* NewPlayer) override;

	// From AEmbercrestPlayerController, each frame or on a key.
	void OnMove(float Forward, float Right, bool bRun);
	void OnLook(float DeltaYaw, float DeltaPitch);
	void OnToggleView();
	void OnInteract();
	void OnPause();
	void OnZoom(float Delta);
	/** A key that skips the arrival's shots (E, Space, Enter, Escape, gamepad A or B). */
	void OnSkip();
	/** The pause menu has the keyboard (the controller leaves input to it). */
	bool IsUiOpen() const;
	/** The arrival's shots are playing (the controller only listens for a skip). */
	bool IsCinematic() const { return Flow == EEcxFlow::Cinematic || Flow == EEcxFlow::StepOff; }
	bool IsLooking() const { return Flow == EEcxFlow::Looking; }
	void SetGamepad(bool bInGamepad) { bGamepad = bInGamepad; }

	void Toast(const FString& From, const FString& Body);
	void ApplySettings(const ss::ui::GameSettings& Settings, bool bSave);
	void SaveSettingsNow();
	void ReturnInputToGame();
	UNightOneAudio* GetAudio() const { return Audio; }

	/** Look sensitivity (degrees per mouse count) and inverted look, from the settings. */
	float Sensitivity = 0.11f;
	bool bInvertLook = false;

	// Automation (scripts, the console's ss.ECX.* commands, the desktop's checks).
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestDescribe() const;
	/** "bus", "doors", "canopy", "overlook", "landingA", or a spawn: "arrive", "leave". */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestTeleport(const FString& Where);
	/** The acceptance shots (spec 7): "S1".."S6", or "off" for the hero's camera again. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestShot(const FString& Shot);
	/** Plays the flow again: "arrive" (the bus pulls in) or "leave" (out of the doors, the bus waiting). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestMode(const FString& Mode);
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestSkip() { OnSkip(); }

	/** Leaving the menu or the game: nothing of the career to save here, only the settings. */
	void LeaveForMenu();
	void LeaveGame();

private:
	AShortStackCharacter* GetHero() const;
	void FindOrSpawnStage();
	void DressHero();
	void SpawnPeople();
	void CullPeople(const FVector& Camera);
	void CreateWidgets();
	void FocusWidget(SFrontEndWidget* Widget);
	FVector2D ViewSize() const;
	void DrawHud();
	void KeepOnTheSet();

	/** The night as the save has it: the event, the hour, the screens' slides. */
	void ReadTheNight();
	void BuildSlides();
	void StartArrival(bool bCinematic);
	void StartLeaving();
	void TickCinematic();
	/** The arrival's shots skipped or done: the hero on the platform with control, the bus leaving in 3 s. */
	void BeginControl(bool bCut);
	void PlaceHero(EEcxSpawn Where, float Pitch);
	void GoInside();
	void BoardBus();
	void LookOut(bool bViewer);
	void EndLook();
	/** The doorman's word when the hero comes up under the canopy (arriving) or tries the doors (leaving). */
	void Greet(const FVector& At);
	/**
	 * The camera's arm held short while the 14 stands at the stop behind the hero (off the bus, the platform is only 1.1 m
	 * deep between them and its side: at the arm's 3 m the camera would be inside the bus), and let out again once it's gone.
	 */
	void HoldArmShort(bool bShort);
	void UpdateSound(float Dt);
	/** The hour shown now: the night's, plus the real seconds since the level opened (display only). */
	double ShownMinutes() const;
	double LightHour() const;

	UPROPERTY(VisibleAnywhere, Category = "Short Stack")
	TObjectPtr<UNightOneAudio> Audio;
	UPROPERTY(Transient)
	TObjectPtr<AEmbercrestStage> Stage;
	UPROPERTY(Transient)
	TArray<TObjectPtr<AShortStackCharacter>> People;

	TUniquePtr<FEmbercrestGame> Game;
	TSharedPtr<SDrawListWidget> HudWidget;
	TSharedPtr<SFrontEndWidget> MenuWidget;
	TArray<ss::ui::StreetHudInfo::Toast> Toasts;
	TSharedPtr<FEmbercrestAmbience> Tone;
	FFrameBudget FrameBudget;

	// The night, from the options and the save.
	FString OptionsString;
	bool bLeaving = false;
	bool bLiveOption = false;
	bool bCareer = false;
	ss::SaveData Save;
	FString LiveId;
	ss::live::Occurrence Event;
	bool bEntry = false;
	bool bCancelled = false;
	bool bStarted = false;
	bool bWon = false;
	int64 PrizeCents = 0;
	FString EventShort;
	TArray<FString> Board;
	/** The world's minutes when the level opened (the arrival, or the walk to the bus), and the hour the light is set for. */
	double BaseMinutes = 0.0;
	double AppliedOverride = -2.0;

	// The flow.
	EEcxFlow Flow = EEcxFlow::Playing;
	double FlowAt = 0.0;
	double RealTime = 0.0;
	double LeaveAt = -1.0;
	double DepartAt = -1.0;
	double LookUntil = -1.0;
	double HintsAt = 1.5;
	double GreetedAt = -100.0;
	double SettingsDirtyAt = -1.0;
	double CullIn = 0.0;
	bool bBlendedIn = false;
	bool bGreetedCanopy = false;
	bool bDressed = false;
	bool bGamepad = false;
	bool bShotHold = false;
	bool bBusOpenNear = false;
	/** The arm's length to let out to once the bus has gone (below 0: not held short). */
	float SavedArm = -1.0f;
	/** Where the hero is stepping as they go through a door or onto the bus. */
	FVector Approach = FVector::ZeroVector;
	EEcxFlow LookReturn = EEcxFlow::Playing;
};

/** Keyboard, mouse and gamepad polled each frame for walking (the pause menu takes its own input). */
UCLASS()
class SHORTSTACK_API AEmbercrestPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AEmbercrestPlayerController();
	virtual void PlayerTick(float DeltaTime) override;
};
