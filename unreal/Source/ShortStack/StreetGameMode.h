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
struct FStreetSpot;

/**
 * The cast members at work on Fifth, behind their counters all night (backroom_cast.py's extras): Benny at the Lucky
 * Penny, Inez at the Wash & Fold. The street's pedestrians are the other extras (a face can't be two people at once).
 */
namespace StreetCast
{
inline const TCHAR* const Clerk = TEXT("ExtraB");
inline const TCHAR* const Attendant = TEXT("ExtraK");
} // namespace StreetCast

/** Dee's game as the street sees it (AStreetGameMode::DeeGameAt): her game runs Tuesdays, Thursdays and Saturdays, nine to five. */
enum class EDeeGame : uint8
{
	Off,       // not tonight, or not yet today
	Tonight,   // a game night, before the doors open at nine
	Open,      // the doors are open: anyone with the buy-in sits (life::InWindow, the session's own rule)
	LastHands, // after three: no new seats, the last hands dealt until five
};

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
	/** Through the Wash & Fold's back door into Dee's game (Session::GoToGame, as the Burner app goes). */
	virtual bool GoOut(const std::string& ActivityId, ss::Chips BuyInCents) override;

	// ss::ui::FrontEndHooks (the pause menu)
	virtual void UiSound(ss::SoundId Id, double Volume) override;
	virtual void Resume() override;
	virtual void QuitToMenu() override;
	virtual void QuitGame() override;
	virtual void SettingsChanged(const ss::ui::GameSettings& Settings) override;
};

/**
 * The Street level: out of the apartment's door onto Fifth, down to the Lucky Penny #212 and back, across to
 * the Wash & Fold. The player walks as AShortStackCharacter (V switches first and third person); E uses what's
 * in reach (the counter, the cooler, the building's door home, the laundromat's door, its attendant, Dee's
 * steel door at the back on her game nights, the tenement's street door); F eats or drinks from the bag;
 * Escape pauses. The session runs here as it does at the desk (the clock, the needs, texts), saved to the same
 * career file. The player's settings apply here as at the desk (quality, render scale, the lens, field of view,
 * the mix). Evicted, 1812's key no longer turns: the player comes and goes by Dee's door across the street.
 *
 * Options: "From=<world minutes>" when the apartment sent the player out ("Door=Dee" from her front room: out
 * of her door); "Start=store" (or "washfold", "wash", "backdoor", "dee") to begin there. Out of Dee's game walked
 * into from here, the Back Room opens this level with "Start=washfold?Night?From=<when they left>?Net=<cents>
 * ?Learned=<reads>" (and "?Busted", "?Closed"): the hours at the table are caught up on the calendar, and Dee
 * has a word about the night.
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

	/** Saves and goes back up to the apartment (evicted: up to Dee's front room, the same hub). */
	void GoHome();
	/** Out to a game the session sent the player to (FStreetGame::GoOut): Dee's, through the steel door. False when it can't go now. */
	bool GoOut(const FString& ActivityId, int64 BuyInCents);
	void Toast(const FString& From, const FString& Body);
	/** The settings from the pause menu (or the saved ones at the start): applied now, saved once the player stops changing them. */
	void ApplySettings(const ss::ui::GameSettings& Settings, bool bSave);
	void SaveSettingsNow();
	void ReturnInputToGame();
	UNightOneAudio* GetAudio() const { return Audio; }

	/** Look sensitivity (degrees per mouse count) and inverted look, from the settings. */
	float Sensitivity = 0.11f;
	bool bInvertLook = false;

	// Automation (scripts and the desktop's checks; the console's ss.Street.*).
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestDescribe() const;
	/** "counter", "attendant", or a StartAt place: "home", "store", "washfold", "wash", "backdoor", "dee". */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestTeleport(const FString& Where);
	/** The clock on to Dee's next game night, just after the doors open (now, if they're open); what the back door says then. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestGameNight();

	/** Dee's game as the street sees it at World (world minutes). */
	static EDeeGame DeeGameAt(double World);

private:
	AShortStackCharacter* GetHero() const;
	void FindOrSpawnStage();
	void DressHero();
	/** A cast member standing at Spot (their feet on the floor at FloorZ), dressed before they begin play: Benny, the attendant. */
	AShortStackCharacter* SpawnNpc(const FTransform& Spot, double FloorZ, const TCHAR* CastName, const FLinearColor& Shirt);
	/** Benny behind the counter (the character looks after him from there: his look at the player, his blinks). */
	void SpawnClerk();
	/** Inez behind the Wash & Fold's counter, folding. */
	void SpawnAttendant();
	/** The street's pedestrians (StreetCrowd.h, once it's in the module), started once the stage and the hero both stand. */
	void SpawnCrowd();
	/** A player who has somehow left the set (fallen through, walked off an end) is put back at the door they live by. */
	void KeepOnTheSet();
	/** The door the player comes and goes by: 1812's ("home"), or Dee's across the street ("dee") while evicted. */
	FString HomeDoor() const;
	/** What E does at a spot, worded for now (the night, the eviction, the laundromat's door); empty: nothing to do there. */
	FString PromptFor(const FStreetSpot& Spot) const;
	/** E at 1812's door: home, or (evicted) the new lock refusing the key. */
	void TryHomeDoor();
	/** E at the tenement's street door: up to Dee's while evicted; otherwise her buzzer, and nobody comes. */
	void TryDeeDoor();
	/** E at the steel door: into Dee's game on its nights (the session's own rules), else when it runs or why not. */
	void TryBackDoor();
	/** E at the attendant's counter: a line for the hour and the night. */
	void TalkToAttendant();
	/**
	 * Leaves the street for Level (with Options) once the screen has faded, the street's sound falling away behind; with
	 * bWithClock, "?From=<world minutes>" as it opens (home: the desk picks the calendar up from there).
	 */
	void Leave(const FName& Level, const FString& Options, bool bWithClock);
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
	UPROPERTY(Transient)
	TObjectPtr<AShortStackCharacter> Attendant;
	/** The pedestrians (an AStreetCrowd, when the module has one). */
	UPROPERTY(Transient)
	TObjectPtr<AActor> Crowd;

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
	/** On the way out (home, or into Dee's game): the level and its options, opened at LeaveAt once the screen is black. */
	bool bLeaving = false;
	double LeaveAt = 0.0;
	FName LeaveLevel;
	FString LeaveOptions;
	bool bLeaveWithClock = false;
	bool bDressed = false;
	bool bGamepad = false;
	bool bCrowdStarted = false;
	/** How many times the attendant has been talked to (her lines go round). */
	int32 AttendantTalks = 0;
	/**
	 * Out of Dee's game: what the Back Room passed back, and when she has her word about it (RealTime; negative: nothing
	 * to say). It's written as it's sent, the hours away caught up by then (the locks may have changed at 3 AM).
	 */
	FString NightOptions;
	double NightWordAt = -1.0;
	/** When the settings last changed (written once they've settled), or negative. */
	double SettingsDirtyAt = -1.0;
	/** The street's own sound (StreetAudio.h), streamed by Audio: the rain and the traffic outside, the coolers inside. */
	TSharedPtr<class FStreetAmbience> StreetTone;
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
