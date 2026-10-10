#pragma once

#include "CareerSave.h"
#include "CoreMinimal.h"
#include "FrameBudget.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Talk.h"
#include "ShortStack/UI/FrontEnd.h"
#include "ShortStack/UI/ShopScreen.h"
#include "ShortStack/UI/StoreCounter.h"
#include "SlateDrawList.h"

#include "StreetGameMode.generated.h"

class ACameraActor;
class AShopStage;
class AShortStackCharacter;
class AStreetGameMode;
class AStreetNpcs;
class AStreetStage;
class SFrontEndWidget;
class UNightOneAudio;
class UStreetWorld;
struct FStreetUse;
struct FStreetZone;

namespace ss
{
namespace ui
{
class TalkCard;
}
} // namespace ss

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
	/** The ledger's top line as it stood when the counter last looked (WatchCounterSale); bCounterTop false while it's shut. */
	ss::life::LedgerEntry CounterTop;
	bool bCounterTop = false;
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
 * What E does comes from the street's registry (UStreetWorld): the use in reach, dispatched by its verb (OnInteract), so
 * every stage that registers uses (the street, the shops) is served without this mode knowing which built them. Inside a
 * zone with a camera profile (the shops: a shorter boom, a wider view) the camera takes it, blended over 0.3 s.
 *
 * Options: "From=<world minutes>" when the apartment sent the player out ("Door=Dee" from her front room: out
 * of her door); "Start=store" (or "washfold", "wash", "backdoor", "dee", or any registry start) to begin there. Out of Dee's game walked
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

	// Automation (scripts and the desktop's checks; the console's ss.Street.*, StreetCommands.cpp).
	/** One paragraph: the hour, where the player is, the zone, the use in reach, the needs, the open screen, the budget (ss.Street.Describe). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestDescribe() const;
	/** "counter", "attendant", or a StartAt place: "home", "store", "washfold", "wash", "backdoor", "dee". */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestTeleport(const FString& Where);
	/** The clock on to Dee's next game night, just after the doors open (now, if they're open); what the back door says then. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestGameNight();
	/** The career's clock on to the next Hour (0..24) o'clock, as ss.Street.GameNight moves it (the calendar runs through the
	 *  hours skipped): the shops' real hours, the schedule and the look follow, unlike ss.Street.Hour's look alone. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestClock(float Hour);
	/** E where the player stands (ss.Street.Use); what was in reach and what it did. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestUse();
	/**
	 * To a registry start, a door (a shop's by its venue), a plot's front (spec 2.3's ids: "H-CASH", "MS3") or The Block's
	 * places (ss.Street.Go). Refused where today's set doesn't reach: outside every zone, or nowhere to stand (a plot past
	 * today's end buildings, or behind Market's).
	 */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestGo(const FString& Where);
	/** An acceptance camera (spec 9.1: W1..W8 by their stage cm, I1..I6 from the registry's shots, or a shot's id), or "off" (ss.Street.Shot). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestShot(const FString& Which);
	/** Draws the registry (zones, uses, stations, starts, doors) for Seconds and lists it (ss.Zone.Show). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestZones(float Seconds);
	/** A shop's screen opened where the player stands ("barber", "thrift", "gym", "diner", "pawn", "wardrobe"), on Page (ss.Shop.Open). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestShopOpen(const FString& Venue, const FString& Page);
	/** The open screen closed (ss.Shop.Close). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestShopClose();
	/** A key to the open screen (the counter, a shop's page or the talk card) by StoreCounter's names: "Down", "Enter", "Escape" (ss.Shop.Key). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestShopKey(const FString& Key);
	/** Every venue's hours now (core's), and each shop stage as it stands (ss.Shop.Hours). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestShopHours() const;
	/** A shop held awake (1), asleep (0) or left to decide (-1) (ss.Shop.Awake). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestShopAwake(const FString& Venue, int32 Hold);
	/** A service's cinematic played without its skip or its charge, its shots for a camera check (ss.Shop.Cinematic barber barber/crew). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestCinematic(const FString& Venue, const FString& Service);
	/** The pawn book: the tickets, the shelf, what's gone, what the hero could bring, and what stands in the shop (ss.Pawn.List). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestPawnList() const;
	/** Every ticket and shelf lot made Days older (the calendar settles forfeits and expiry on its next step; ss.Pawn.Age). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestPawnAge(float Days);
	/** Unit 114 comes due now (evicted with a unit): the card pays it if it can, else the lien auction runs (ss.Pawn.Auction). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestPawnAuction();

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
	/**
	 * The block's people (people-ue's AStreetNpcs, spec 7.5.7): found or spawned once the stage and the hero stand, begun with
	 * them and the crowd, given the career; told the hour each frame (Tick), as the crowd is.
	 */
	void SpawnPeople();
	/** A player who has somehow left the set (fallen through, walked off an end) is put back at the door they live by. */
	void KeepOnTheSet();
	/** The door the player comes and goes by: 1812's ("home"), or Dee's across the street ("dee") while evicted. */
	FString HomeDoor() const;
	/** The street's registry (null before the world has one). */
	UStreetWorld* StreetWorld() const;
	/** What the hero's eye reaches now (the registry's Best), or null. */
	const FStreetUse* UseInReach() const;
	/** The zone the hero (or At) is in, or null. */
	const FStreetZone* ZoneAt(const FVector& At) const;
	/** Where the player appears for a start id: the registry's, else the stage's own (StartAt). */
	FTransform StartTransform(const FString& Id) const;
	/** What E does at a use, worded for now (the night, the eviction, a door open or shut); empty: nothing to do there. */
	FString PromptFor(const FStreetUse& Use) const;
	/** E at a Door use: the Wash & Fold's (it swings in), Dee's steel door (her game), or a shop's (open, buzz, or its closed line). */
	void UseDoor(const FStreetUse& Use);
	/** E at a Talk use: the Wash & Fold's attendant (her lines), or the talk card with whoever it is (OpenTalk). */
	void TalkTo(const FStreetUse& Use);
	/** A door that wouldn't open: its line on the HUD where the locked door's line goes (spec 7.4), and the handle's sound by the caller. */
	void ShowClosed(const FString& Line);
	/** E at a use whose screen hasn't landed yet (the 14's door): said so, once a session per use. */
	void NotYet(const FStreetUse& Use);

	// ---- the shops' screens, seats and services (spec 7.5.3)
	/** Opens a venue's screen (core's MakeShopScreen: "barber", "thrift", "gym", "diner", "pawn", "wardrobe") on Page. False when there's none. */
	bool OpenShop(FName Venue, const FString& Page);
	/** Closes the open screen: the preview put back, the camera back on the hero, standing up if bStand and seated. */
	void CloseShop(bool bStand);
	/** The open screen each frame: drawn, its requests drained (spec 7.5.3), whether the player walked away. */
	void UpdateShopScreen(float Dt);
	void DrainRequests();
	/** What the 3D can show the screens (spec 5.0): looks' answer where it has one, else what's dressed and imported now; the pawn's shelf. */
	ss::ui::HostCaps BuildCaps() const;
	/** The camera on a registry shot, blended (and the shop told, its mark taken: the thrift's mirror), or back to the hero. */
	void ShowShot(FName Shot, float Blend);
	void EndShot(float Blend);
	/** The hero sat at a shop's seat (its feet, its height, its camera), or stood up from it. */
	bool SitAt(AShopStage* Shop, FName Seat);
	void StandUp();
	/** E at a Sit, Wait, Shop or Wardrobe use. */
	void UseSeat(const FStreetUse& Use);
	void UseWait(const FStreetUse& Use);
	/**
	 * A service's cinematic (spec 7.5.3): the shop's shots under the skip the page started (movement blocked, the corner
	 * held, the HUD's skip overlay), a fade through black as it ends (the new look on there), the after-shot, then the hero
	 * back. Service is the skip's offer ids; with bDry nothing was charged (ss.Shop.Cinematic) and it runs Seconds.
	 */
	void BeginCinematic(FName Venue, const FString& Service, bool bDry = false, float Seconds = 0.0f);
	void UpdateCinematic(float Dt);
	/** Something done under black: the screen fades out, Action runs, it fades back in (the builds, the walk-outs). */
	void UnderBlack(TFunction<void()> Action, float Out = 0.3f);
	void UpdateBlack(float Dt);
	/** The hero dressed again for keeps (looks' RefreshLook in place, or the whole look rebuilt under black). */
	void RedressHero(const ss::hero::Look& Look, const ss::wardrobe::Closet& Closet);
	/** The shops' session state (the boards, the pawn's shelf, the thrift's racks), every couple of seconds. */
	void SyncShops();
	/** Every enterable shop's stage, standing (a stage that owns its plot may spawn it first: they find each other). */
	void SpawnShops();
	/** Closing time (spec 5.0): the keeper's word ten minutes out; at close any open screen ends and the hero is walked out. */
	void UpdateClosing();
	/**
	 * The talk card with Person (spec 6.4, 7.5.3): the director engages them (they stop, turn and hold the hero's eyes) and
	 * says their opening line (town's), shown on core-ui's TalkCard as the one ActiveScreen. Someone the director doesn't have
	 * out (Augie's guitar wall before his body is built) still talks, from town's context alone. False when nobody answers.
	 */
	bool OpenTalk(FName Person, FName Venue);
	/** A choice on the card (its id): their answer, or their service ("service": the card closes and their shop opens). */
	void AnswerTalk(const FString& Choice);
	/** The talk over (walked away, a goodbye, the card closed): the person let go. */
	void EndTalk();
	/** Where Person's service is had (their venue's screen on its page, or their chair). */
	void OpenServiceOf(FName Person, FName Venue);
	/** The keeper on shift at a venue (town's), and their name. */
	FString KeeperAt(FName Venue, FString* OutName = nullptr) const;
	/** The enterable shop for a venue (the registry's shops: AShopStage), or null. */
	AShopStage* ShopFor(FName Venue) const;
	/** The shops each frame: the hour, their doors for the hero (the bell, the buzzer's clack). */
	void UpdateShops(const FVector& Ear, float Dt);
	/** The camera inside a zone with a profile (the shops' shorter boom and wider view, or first person), blended over 0.3 s. */
	void UpdateCameraProfile(float Dt);
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
	/** The clerk rings a sale up (spec 6.6): a new corner-store line at the top of the ledger while the counter is open. */
	void WatchCounterSale();
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
	/** The block's people (the staff at their stations, the world's people, talk and subtitles). */
	UPROPERTY(Transient)
	TObjectPtr<AStreetNpcs> People;

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
	/** The enterable shops in the level (found again every second: the stages spawn them). */
	TArray<TWeakObjectPtr<AShopStage>> Shops;
	double ShopsFoundAt = -100.0;
	/** The uses whose screens haven't landed yet that have said so this session (NotYet). */
	TSet<FName> NotYetSaid;
	/** The camera profile: the zone it's from (None on the street), the player's own boom out there, how far the blend has got and from where. */
	FName ProfileZone;
	float StreetArm = -1.0f;
	float ProfileBlend = 1.0f;
	float BlendFromArm = 300.0f;
	float BlendFromFov = 72.0f;
	/** First person was put on by a zone's profile (and comes off again on the way out). */
	bool bProfileFirstPerson = false;
	/** An acceptance camera (ss.Street.Shot) while one is up, and its name. */
	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> ShotCamera;
	FString ShotName;

	// ---- the shops' screens, seats and services (spec 7.5.3)
	/** The open shop screen (core-ui's page for the venue), its venue and the page it opened on. */
	TUniquePtr<ss::ui::ShopScreen> Screen;
	/** Counts every screen put up or taken down: a request that changes it ends the drain (an address can come back reused). */
	int32 ScreenSerial = 0;
	FName ScreenVenue;
	FString ScreenPage;
	TSharedPtr<SFrontEndWidget> ShopWidget;
	/** The screen's camera (a registry shot), its shot, and the thrift mirror's orbit (degrees about the hero's mark). */
	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> ShopCamera;
	FName CameraShot;
	FTransform CameraBase;
	FVector OrbitPivot = FVector::ZeroVector;
	float OrbitYaw = 0.0f;
	FVector2D OrbitMouse = FVector2D(-1.0, -1.0);
	/** Where the hero stood before a shot's mark took them (put back as the shot goes). */
	bool bMarkMoved = false;
	FTransform BeforeMark;
	/** A look being previewed on the hero (put back as the screen closes). */
	bool bPreviewing = false;
	/** The seat the hero is in (a shop's seat id and its venue), and where they stand up to. */
	FName SeatedAt;
	FName SeatedVenue;
	FTransform StandTo;
	/**
	 * The talk under way (the talk card in Screen, Card pointing at it while it's up): who and where, whether the director
	 * has them (else town's context alone answers), and that context.
	 */
	ss::ui::TalkCard* Card = nullptr;
	FName TalkingTo;
	FName TalkingAt;
	bool bTalkByDirector = false;
	std::shared_ptr<const ss::town::TalkContext> TalkContext;
	/** What people said near the hero (the director's subtitles), for the HUD (it shows the newest two as they age). */
	TArray<ss::ui::StreetHudInfo::Subtitle> Subtitles;
	/** A locked door's line and when it was tried (RealTime). */
	FString ClosedNow;
	double ClosedNowAt = -100.0;

	/** A service's cinematic as it plays (BeginCinematic). */
	struct FCinema
	{
		bool bActive = false;
		bool bDry = false;
		FName Venue;
		FString Service;
		/** The screen's page it began from (a seat's: the diner's menu opens there again after the meal). */
		FString Page;
		/** Under the skip, and after it (the reveal), each (shot, real seconds). */
		TArray<TPair<FName, float>> During;
		TArray<TPair<FName, float>> After;
		double StartedAt = 0.0;
		double Length = 4.0;
		int32 Shot = -1;
		/** 0 the skip, 1 fading out at its end, 2 the after-shots, 3 fading back to the hero. */
		int32 Phase = 0;
		double PhaseAt = 0.0;
		/** Where the hero was before the service took them to its mark (a treadmill), and whether it did. */
		bool bMoved = false;
		FTransform Before;
		bool bSeatedForIt = false;
		/** A wait for the barber's chair: once it's over, into the chair and the Style Book. */
		bool bThenChair = false;
	};
	FCinema Cinema;
	/** Under black (UnderBlack): the action, when it runs, when the fade back starts. */
	TFunction<void()> BlackAction;
	double BlackAt = -1.0;
	double BlackInAt = -1.0;
	/** The shops' session state last synced (RealTime), and each shop's closing as said (its close time in world minutes). */
	double ShopsSyncedAt = -100.0;
	TMap<FName, double> ClosingSaid;
	TMap<FName, bool> WasOpen;
	/** The hours' lines as last built (once a second), by venue. */
	double HoursAt = -100.0;
	TMap<FName, FString> ClosedLines;
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
