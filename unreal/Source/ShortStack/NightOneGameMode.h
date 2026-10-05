#pragma once

#include "CoreMinimal.h"
#include "DeeRoomLife.h"
#include "FrameBudget.h"
#include "SlateDrawList.h"
#include "GameFramework/GameModeBase.h"
#include "NightOneGame.h"

#include "NightOneGameMode.generated.h"

class ADeeRoomStage;
class ANightOnePawn;
class ANightOneStage;
class FDeeRoomTone;
class SBackgroundBlur;
class SDeeRoomCaption;
class SFrontEndWidget;
class SNightOneOverlay;
class SWidget;
class UNightOneAudio;

/**
 * SHORT STACK: Night One. Boots the hub (the apartment at 1812, or Dee's front room across the street while the career
 * is evicted), the title screen and menus, the RiverLine client on the laptop, audio and the game session, and routes
 * input between the menus, the room and the screen (port of web/src/main.ts).
 * Works in any level: the apartment's stage placed in it is used, or one is spawned at the origin; Dee's is built when
 * it's wanted, and only one room is in the level at a time.
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
	/** A new career; Who is the person the character creator made (none: the defaults). */
	void StartNewCareer(const FString& ScreenName, const ss::hero::Character* Who = nullptr);
	void ResumePlay();
	void QuitToMainMenu();
	void QuitToDesktop();
	void ApplySettings(const ss::ui::GameSettings& Settings, bool bSave);

	/**
	 * Leaves the hub for the street outside (the Street level: the walk to the Lucky Penny), out of 1812's door or, from
	 * Dee's, out of hers. False when it can't.
	 */
	bool GoOutside();
	/** Leaves the apartment for Dee's game across the street (the Back Room level), buying in with BuyInCents. */
	bool GoOut(const FString& ActivityId, int64 BuyInCents);

	ANightOneStage* GetStage() const { return Stage; }
	ANightOnePawn* GetSeat() const;
	UNightOneAudio* GetAudio() const { return Audio; }
	/** The running game (null before the title screen starts one). */
	FNightOneGame* GetGame() const { return Game.Get(); }
	void ShowToast(const FString& From, const FString& Body);
	/** Whether the hub is Dee's front room (the career evicted, or ss.Home.Evicted says so). */
	bool IsAtDees() const { return bAtDees; }

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
	/**
	 * The hub as if the career were evicted (1: Dee's front room) or not (0: the apartment), the room changing now; -1
	 * follows the career again. Not saved (the career's rent is untouched); a fresh start of the level forgets it.
	 */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestEvicted(int32 Evicted);
	/**
	 * The hub as at this hour of today (0..24): the light, the window, Dee's whereabouts; the career's clock doesn't move.
	 * Below 0, the clock's own hour again. Not saved.
	 */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestHour(float Hour);
	/** Moves the career's clock forward to the next time it's this hour (the hours pass as they would). Why not, or what it did. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestSkipTo(float Hour);
	/** Where the hub is, the hour it shows, and where Dee is. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString DescribeHub() const;

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
	/** The stage's own image options (Brightness, motion blur, film grain, fringe) from the settings. */
	void ApplyStageSettings(const ss::ui::GameSettings& Settings);

	/** Whether the hub is Dee's front room: the career is evicted, or the test hook says so. */
	bool HubEvicted() const;
	/** The world minute the hub shows: the career's clock, or the test hook's hour today. */
	double HubWorldMinutes() const;
	/** Makes the hub Dee's front room or the apartment (the other room leaves the level), and hooks it up. */
	void UseRoom(bool bDees);
	/** The stage's thunder, the image settings and the seat on it, and the room's sound. */
	void BindStage();
	ADeeRoomStage* DeeStage() const;
	/** Dee's front room each frame: where she is and what she says, the room's sound, the note she leaves. */
	void TickDeeRoom();
	/** Dee says it in the room (a caption, her jaw moving), texts it, or the room's own line is captioned. */
	void SayDee(const DeeRoom::FLine& Line);
	/** Her door (or the apartment's) shutting: she came in or went out. */
	void DeeDoor();
	/** Asleep (a night or a nap) in a time skip now. */
	bool IsSleeping() const;
	/**
	 * Locked out while in 1812 (the landlord at the door, or on waking): once nothing holds the player at the laptop, the
	 * room fades and they come in at Dee's, as the next load would have them.
	 */
	void TickLockedOut();

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
	/** Where to: "dee-game" (the Back Room) or "embercrest" (the casino's tournament). */
	FString LeaveFor;
	// Home from Dee's game (or the Embercrest): what the night passed back, for her text once the room fades back in.
	FString HomeOptions;
	double HomeTextAt = -1.0;
	/** Whether the locks were changed last frame (-1 before the first): the room's dressing follows it. */
	int32 EvictedShown = -1;

	// Dee's front room (the hub while evicted).
	bool bAtDees = false;
	TSharedPtr<SDeeRoomCaption> Caption;
	/** The room's sound (the radiator, her TV), streamed as the hub's ambience while it's her room. */
	TSharedPtr<FDeeRoomTone> RoomTone;
	DeeRoom::EWhere DeeWhere = DeeRoom::EWhere::Out;
	bool bDeeKnown = false;
	/** What she says as the player comes in, once the camera's settled (and with what the level was opened with). */
	double DeeHelloAt = -1.0;
	/** Home from another level: until this real time, the hours caught up on arrival can still make the room hers. */
	double ArrivalSwapUntil = -1.0;
	/** Locked out in 1812 (TickLockedOut): from when the move to Dee's may start, and when the faded-out room changes. */
	double MoveToDeesAt = -1.0;
	double MoveSwapAt = -1.0;
	FString ArrivedWith;
	/** What she says once she's sat down in her chair (she came in while the player was there). */
	DeeRoom::FLine PendingArrival;
	/** Asleep on her couch: the camera lies down (and gets up again till this real time), the eyes close a little. */
	bool bWasSleeping = false;
	double CouchShotUntil = -1.0;
	double WokeAt = -1.0;
	float SleepFade = 0.0f;
	uint32 DoorSeed = 1;
};
