#pragma once

#include "CoreMinimal.h"
#include "FrameBudget.h"
#include "SlateDrawList.h"
#include "GameFramework/GameModeBase.h"
#include "NightOneGame.h"

#include "NightOneGameMode.generated.h"

class ANightOnePawn;
class ANightOneStage;
class SBackgroundBlur;
class SFrontEndWidget;
class SNightOneOverlay;
class SWidget;
class UNightOneAudio;

/**
 * SHORT STACK: Night One. Boots the apartment, the title screen and menus,
 * the RiverLine client on the laptop, audio and the game session, and routes
 * input between the menus, the room and the screen (port of web/src/main.ts).
 * Works in any level: if no NightOneStage is placed, one is spawned at the origin.
 */
UCLASS()
class SHORTSTACK_API ANightOneGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ANightOneGameMode();

	virtual void StartPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void RestartPlayer(AController* NewPlayer) override;

	// Input from ANightOnePlayerController (ignored while a menu is open).
	void OnMouse(bool bOverScreen, const FVector2D& Client, float Nx, float Ny);
	void OnPress(bool bOverScreen);
	void OnRelease();
	void OnWheel(float Delta);
	void OnKey(const FString& Key);
	void OpenPauseMenu();
	bool HideCursor(bool bOverScreen) const;
	bool IsLeanedBack() const;
	bool IsMenuOpen() const;

	// From the menus (FNightOneGame's front-end hooks).
	void ContinueCareer();
	void StartNewCareer(const FString& ScreenName);
	void ResumePlay();
	void QuitToMainMenu();
	void QuitToDesktop();
	void ApplySettings(const ss::ui::GameSettings& Settings, bool bSave);

	/** Leaves the apartment for Dee's game across the street (the Back Room level), buying in with BuyInCents. */
	bool GoOut(const FString& ActivityId, int64 BuyInCents);

	ANightOneStage* GetStage() const { return Stage; }
	ANightOnePawn* GetSeat() const;
	UNightOneAudio* GetAudio() const { return Audio; }
	/** The running game (null before the title screen starts one). */
	FNightOneGame* GetGame() const { return Game.Get(); }
	void ShowToast(const FString& From, const FString& Body);

	// Test hooks, driven from the editor's Python for screenshots and checks.
	/** Sits back (Lean 0) or leans in to the laptop (1), looking Yaw / Pitch (radians) while sitting back. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestLook(float Lean, float Yaw, float Pitch);
	/** Goes live on Kast, or ends the stream; returns why not when it can't. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestStream(bool bLive);
	/** Starts a new career with this screen name (the title screen's New career). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestNewCareer(const FString& ScreenName);
	/** A click on the laptop's screen at client coordinates (1600 x 1000): pressed now, released a moment later. From
	 *  then on the real mouse no longer moves the screen's pointer (a script is playing). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestClick(float X, float Y);
	/** The size of each screen's last draw list (the menu, the laptop, the phone, the two monitors). */
	FDrawListHint MenuHint, ClientHint, PhoneHint, MonitorHint[2];
	/** Dynamic resolution (FrameBudget.h). */
	FFrameBudget FrameBudget;
	/** A key, as the player's keyboard sends it ("f", "c", "r", "a", "ArrowUp", " " to lean...). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestKey(const FString& Key);
	/** The mouse wheel over the screen (lists scroll). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestWheel(float Delta);
	/** The game as text: the career, the open table (seats, cards, the board, the pot, what the player may do), results. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestDescribe() const;

	/** Game time multiplier (for testing long tournaments). */
	UPROPERTY(EditAnywhere, Category = "Short Stack")
	float TimeScale = 1.0f;

private:
	void Begin(const FString& Name);
	void CreateViewportWidgets();
	void FocusMenu();
	void ReturnInputToGame();
	void DrawMenu();
	void SaveSettingsNow();

	UPROPERTY(Transient)
	TObjectPtr<ANightOneStage> Stage;

	UPROPERTY(VisibleAnywhere, Category = "Short Stack")
	TObjectPtr<UNightOneAudio> Audio;

	TUniquePtr<FNightOneGame> Game;
	TSharedPtr<SNightOneOverlay> Overlay;
	TSharedPtr<SFrontEndWidget> MenuWidget;
	TSharedPtr<SBackgroundBlur> MenuBlur;
	TSharedPtr<SWidget> MenuRoot;
	bool bStarted = false;
	bool bHasSave = false;
	double RealTime = 0.0;
	double GameTime = 0.0;
	double BeganAt = -1.0;
	double UiAccum = 1.0;
	double PhoneAccum = 1.0;
	double MonitorAccum = 1.0;
	// Test input: a scripted click's release, due at this real time; the real mouse is ignored once a script plays.
	double TestReleaseAt = -1.0;
	bool bTestInput = false;
	double SettingsDirtyAt = -1.0;
	float Tilt = 0.0f;
	float Pulse = 0.0f;
	float LookSensitivity = 1.0f;
	bool bInvertLook = false;
	bool bShowHints = true;
	bool bOverScreenNow = false;
	FVector2D ArmsPointer = FVector2D(0.5, 0.5); // the pointer across the view (0..1), for the mouse hand
	// Heading out: the room fades, then the Back Room level opens.
	bool bLeaving = false;
	double LeaveAt = -1.0;
	int64 LeaveBuyInCents = 0;
	/** Where to: "dee-game" (the Back Room) or "riverside" (the casino's tournament). */
	FString LeaveFor;
	// Home from Dee's game: what she texts once the room fades back in.
	FString HomeText;
	double HomeTextAt = -1.0;
};
