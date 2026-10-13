#pragma once

#include "CoreMinimal.h"
#include "FrameBudget.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
// The next segment's types, whole (next_segment_spec 8.2: this header carries every Unreal track's members and froze
// after Phase A): the field's badges, the room's director and its cues, the table HUD's captions, plates, toasts, the
// last hands and the desk screen.
#include "ShortStack/Game/Badges.h"
#include "ShortStack/Game/Room.h"
#include "ShortStack/UI/TableHud.h"
// Update 2 (update 2 SPEC §2.12): autoplay's table plan, for the six functions the header carries for ue-auto.
#include "ShortStack/Game/Autoplay.h"

#include <string>
#include <vector>

#include "BackRoomGameMode.generated.h"

class ABackRoomPlayer;
class ABackRoomStage;
class ABackRoomTable;
class SDrawListWidget;
class UCameraComponent;
class UPointLightComponent;
class UTextureRenderTarget2D;

namespace ss
{
struct SaveData;
class Tournament;
struct TEvent;
struct TPlayer;
}
struct FBackRoomPersona;
class FCareerSaver;
enum class ECardRoomSpot : uint8;
enum class EBackRoomRole : uint8;

// The next segment's host classes, each defined in its track's own files (next_segment_spec 8.3): the card room's stereo
// sound and its clip seam (ue-room, CardRoomSound.h), the badge atlas (ue-venue, BadgeAtlas.h), and core draw lists into
// render targets (CoreCanvasTarget.h). All classes.
class FCardRoomSound;
class FVoiceBank;
class FBadgeAtlas;
class FCoreCanvasTarget;
// Each Unreal track's own state on the game mode and the HUD, defined in that track's files: anything a track needs past
// the members declared here goes in its host, so this header never needs a fifth owner. All structs. Made with MakeShared
// where the struct is complete; a UObject one makes is kept alive in that track's *Objects array below.
struct FRoomHost;     // ue-room
struct FTableHost;    // ue-table (the game mode's side of the HUD and the table input)
struct FTableHudHost; // ue-table (the HUD's side: the draw-list hint, the measurer, the projection's state)
struct FVenueHost;    // ue-venue
struct FWearHost;     // ue-wear
// Update 2 (update 2 SPEC §2.12): the plan in the hero's seat (BackRoomAuto.h), a friend of the game mode and the pawn, so its
// bodies reach the night's own state without this header changing again.
struct FAutoHost;     // ue-auto

/**
 * You, at the Back Room's table: a camera in your own head.
 *
 * The body is an ABackRoomPlayer in the hero's seat (its face hidden from you): the camera rides its
 * head, the head turns where you look, and its hands do what you do. Hold Space (or the left mouse
 * button) to lift the corners of your cards; hold the right mouse button to study whoever you look at
 * (Focus: the view narrows, time slows, a face sharpens; it drains while you hold it). F folds, C
 * checks or calls, R bets or raises to the amount shown (the mouse wheel changes it), A goes all in.
 * Hold Shift to breathe slowly (the heart settles; it costs Focus). L racks up and goes home. V takes the
 * camera out over your shoulder (your own head in the shot, the table past it) and back in; B (or the middle
 * mouse button) moves it to the other shoulder. Both are kept from night to night. Peeking, Focus and walking
 * are first person's business: they take the camera back in.
 *
 * Your heart is in the camera: past about 90 bpm you hear it, the view pulses with it and the edges
 * of the room fall away; your hands shake when they push chips, and the sharper regulars notice.
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

	/** Third person (V): the camera over the hero's shoulder with their head in the shot; first person again on V. (For
	 *  scripts: this doesn't change the player's kept choice; V does.) */
	UFUNCTION(BlueprintCallable, Category = "Short Stack")
	void SetThirdPerson(bool bOn) { bThirdPerson = bOn; }
	bool IsThirdPerson() const { return bThirdPerson; }
	/** 0..1: how far out over the shoulder the camera is right now (a peek or Focus brings it in). */
	float GetThirdBlend() const { return ThirdBlend; }
	/** Real seconds since V last changed the view (for the HUD's note). */
	float ViewShownAge() const { return static_cast<float>(FPlatformTime::Seconds() - ViewShownAt); }

	/** A free camera for looking at the table from outside (for testing from scripts): bOn takes the view, off gives it back. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestExtCam(FVector Pos, FVector At, float Fov, bool bOn);

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
	/** Holding Shift: slow breaths. */
	bool bSteadying = false;

	/**
	 * Walks the camera through the room along Points (world, eye height) over Seconds: in from the
	 * laundromat to the seat, or (bOut) up from the seat and out the door. OnDone when it arrives. A walk is
	 * first person: over the shoulder, the camera eases back into the head first, then sets off. A walk out that
	 * ends where another starts later (the cashier's window, the aisle on a table move) holds there until it does.
	 */
	void PlayWalk(const TArray<FVector>& Points, float Seconds, bool bOut, TFunction<void()> OnDone);
	bool IsWalking() const { return bWalking || bWalkPending; }
	/** On foot in the card room (WASD or the arrows and the mouse, Shift to hurry, E at whatever's in reach). */
	void BeginFreeWalk();
	void EndFreeWalk() { bFreeWalk = false; }
	bool IsFreeWalking() const { return bFreeWalk; }
	/** The room moved under a walker (a table move re-anchors it): keep them where they stood in it. */
	void ShiftWalk(const FVector& Delta) { WalkBase += Delta; }
	/** Testing: hold a key down (or let it go), or press one once ("W", "E", "Q", "N", "T"...). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestHold(const FString& Key, bool bDown);
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestPress(const FString& Key);
	/** Testing: a walker at a point of the room (room space; eye height comes with it), facing Yaw and Pitch (degrees). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestWalker(float RoomX, float RoomY, float InYaw, float InPitch);
	/** N: the rest of a hand the player has folded, played out quickly (until they're dealt in again). */
	bool IsSkippingHand() const { return bSkipHand; }
	/** 0..1 along the current walk. */
	float WalkProgress() const { return bWalking ? FMath::Clamp(WalkT / WalkSeconds, 0.0f, 1.0f) : (bWalkPending ? 0.0f : 1.0f); }
	/**
	 * Where the player's eyes are: where a walk sets off from, and what the regulars look at as you go by. In first
	 * person that's the camera; over the shoulder, the head the camera looks past.
	 */
	FVector GetEye() const;

	// ------------------------------------------------------------ the next segment: one control map (ue-table, spec 6.4)
	/** The table card (hold Tab or d-pad up; modal: the decision waits): up, on which page (READS, HANDS, PAYOUTS, FIELD,
	 *  NIGHT), and the real time it last closed (the table's actions stay locked 0.25 s after). */
	bool IsTableCardUp() const { return bTableCard; }
	int32 GetTableCardPage() const { return TableCardPage; }
	double GetTableCardClosedAt() const { return TableCardClosedAt; }
	/** 0..1: the ALL IN rung's fill while all in is held (Y on a pad past 0.35 s, A on the keyboard), for the ladder. */
	float GetAllInHold() const { return AllInHold; }

private:
	void HandleInput(float RealDt);
	void TickWalk(float RealDt);
	void TickFreeWalk(float RealDt);
	/** A walk out waiting on the camera to come back into the head: it sets off now. */
	void StartPendingWalk();
	/** Holding at a walk's end (see PlayWalk): the head still looks about, the heart still beats. */
	void TickHold(float RealDt);
	/**
	 * Over the shoulder: the camera for this frame, eased out from the first-person eye (where Tick has just put it) by
	 * ThirdBlend, pivoting on Head (the eyes, before a peek takes them down to the cards), kept clear of the room and the
	 * people in it, with the heart's kick (Shake) and its squeeze on the lens (Racing). Also shows the hero's head once
	 * the camera is clear of it, and builds or drops it with the choice.
	 */
	void PlaceShoulderCamera(const FVector& Head, const FRotator& Shake, float Racing, float RealDt);
	/** How far out along the boom From -> To (0..1) the camera can go before it meets the room or someone at the table. */
	float BoomClearance(const FVector& From, const FVector& To) const;
	/** V: the view the other way, kept for the next night. */
	void ToggleThirdPerson();
	/** A key held, or pressed this frame (the real one, or a test's). */
	bool Held(const APlayerController* PC, const FKey& Key) const;
	bool Pressed(const APlayerController* PC, const FKey& Key) const;
	/** The heart: the beat you hear and the pulse you see. */
	void TickHeart(float RealDt, float& PitchKick, float& Intensity);
	ABackRoomTable* GetTable() const;
	class ABackRoomGameMode* GetMode() const;

	FRotator Smoothed = FRotator(-14.0f, 0.0f, 0.0f);
	FVector Seat = FVector::ZeroVector;
	float Time = 0.0f;
	float PeekBlend = 0.0f;
	bool bPeekReported = false;
	bool bExtCam = false;
	bool bThirdPerson = false;
	float ThirdBlend = 0.0f;
	double ViewShownAt = -100.0;
	/** The eyes (GetEye), wherever the camera is. */
	FVector FirstEye = FVector::ZeroVector;
	// The shoulder camera: which shoulder (+1 right, -1 left; eased) and the one the player picked (B), switched while they
	// look well across the other way; the boom from the head (trailing it a little), and how much of it is clear.
	float ShoulderSide = 1.0f;
	float ShoulderSideWant = 1.0f;
	bool bShoulderFlipped = false;
	/** B pressed while looking across: the shoulder they picked stays put until the look comes back. */
	bool bShoulderHeld = false;
	FVector ShoulderAt = FVector::ZeroVector;
	bool bShoulderFresh = true;
	float BoomFrac = 1.0f;
	/** Seconds the shoulder camera has been put away (the hero's copied hair goes after a moment of it). */
	float HeadIdleT = 0.0f;
	/** Frames the camera keeps to the eyes while a head just built gets ready to be drawn. */
	int32 HeadWarmFrames = 0;
	FVector ExtPos = FVector::ZeroVector;
	FVector ExtAt = FVector::ZeroVector;
	float ExtFov = 50.0f;

	// The walk.
	TArray<FVector> WalkPoints;
	TArray<float> WalkLengths;
	float WalkTotal = 0.0f;
	float WalkT = 0.0f;
	float WalkSeconds = 1.0f;
	bool bWalking = false;
	bool bWalkOut = false;
	/** A walk out asked for over the shoulder: it sets off once the camera is back in the head. */
	bool bWalkPending = false;
	/** A walk out ended and the next one isn't under way yet: the view stays where it stopped. */
	bool bHoldWalkEnd = false;
	bool bBodyShown = true;
	TFunction<void()> WalkDone;
	FVector WalkAt(float Distance) const;

	// On foot.
	bool bFreeWalk = false;
	FVector WalkBase = FVector::ZeroVector; // the eye, without the stride's bob
	FVector WalkVel = FVector::ZeroVector;
	float WalkYaw = 0.0f;
	float WalkPitch = -6.0f;
	float WalkDist = 0.0f;
	TSet<FName> TestHeld;
	TSet<FName> TestPressed;

	// The heart.
	float BeatPhase = 0.0f;
	/** The table's pace: quicker once you're out of the hand (a tournament). */
	float Pace = 1.0f;
	bool bSkipHand = false;
	int32 StepCount = 0;
	float Kick = 0.0f;
	float Steady = 0.0f;
	float BreathT = 0.0f;

	// The next segment's table input (ue-table, spec 6.4): the table card, and Y's tap (raise, on release under 0.35 s) or
	// hold (the ALL IN rung's fill from 0.35 s, complete at 0.95 s; let go between: nothing).
	bool bTableCard = false;
	int32 TableCardPage = 0;
	double TableCardClosedAt = -100.0;
	/** Real seconds Y (or A on the keyboard) has been held this press, or -1 when it's up. */
	float RaiseHeld = -1.0f;
	float AllInHold = 0.0f;

	// Update 2 (update 2 SPEC §2.6.2, §2.12): the plan's keys at the table (K, T under the plan, the PLAY page, the take-over
	// press) are ue-auto's FAutoHost::PawnKeys, which reads them here (Held, Pressed, the table card).
	friend struct FAutoHost;
};

/** Prompts, stacks, Focus, the heart, the clock, reads and the table talk, drawn over the view. */
UCLASS()
class SHORTSTACK_API ABackRoomHUD : public AHUD
{
	GENERATED_BODY()

public:
	/** BackRoomHud.cpp (ue-table's after Phase A). */
	virtual void DrawHUD() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	// ------------------------------------------------------------ the next segment: the core HUD (ue-table, spec 6.3)
	/**
	 * The core table HUD (ss::ui::DrawTableHud) hosted through an SDrawListWidget over the view, from the game mode's
	 * FillTableHud; the legacy canvas HUD below stays for one milestone (ss.Table.LegacyHud 1).
	 */
	void DrawCoreHud();
	/** P0a: the AI stack plates (the game mode's GetPlates, projected) drawn on the legacy canvas HUD. */
	void DrawLegacyPlates(float W, float H, float S);
	TSharedPtr<SDrawListWidget> HudWidget;
	TSharedPtr<FTableHudHost> HudHost;

	struct FShown
	{
		FString Speaker;
		FString Text;
		float Until = 0.0f;
		float From = 0.0f;
	};
	TArray<FShown> Subtitles;
	TArray<FShown> Whispers;
	float Clock = 0.0f;
	/** The ECG trace: recent samples, newest last. */
	TArray<float> Trace;
	float TracePhase = 0.0f;
	float TraceAccum = 0.0f;
	float HeartAlpha = 0.0f;
};

/** Where the night is. */
enum class EBackRoomPhase : uint8
{
	/** Practice: the table on its own, no save, no clock (the level opened directly). */
	Practice,
	/** Walking in from the laundromat. */
	Arriving,
	Playing,
	/** Felted: rebuy from the bankroll, or go home. */
	Busted,
	/** Racked up: the goodbye, then the walk out. */
	Leaving,
	/** Tournament: carrying your chips to another table. */
	Moving,
};

/**
 * The Back Room: seats the cast at the table, deals Dee in, and starts the Tuesday game.
 *
 * Opened from Night One (Dee's thread in the Burner app) with "?BuyIn=<cents>", it is a night of your
 * career: the buy-in comes out of the saved bankroll, the clock runs (a minute of play is six at the
 * table), you tire, Dee calls the last hand at a quarter to five, and every hand is saved. Racking up
 * settles the night into the save (the ledger, the reads you learned, the hours and the energy spent)
 * and walks you back across the street to the apartment ("?Home"). Opened directly, it is practice.
 */
UCLASS()
class SHORTSTACK_API ABackRoomGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABackRoomGameMode();
	virtual ~ABackRoomGameMode() override;

	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual void StartPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;
	/** Dynamic resolution, as the player set it in the apartment's menus (FrameBudget.h). */
	FFrameBudget FrameBudget;

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

	// ------------------------------------------------------------ the night (for the pawn and the HUD)
	EBackRoomPhase GetPhase() const { return Phase; }
	bool IsCareer() const { return Phase != EBackRoomPhase::Practice; }
	/** L: rack up after this hand (or now, between hands or busted). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack")
	void RequestLeave();
	/** Busted: R buys back in for the amount shown; the wheel changes it. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack")
	void Reload();
	/** Phase, clock, energy and money (for logs and tests). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack")
	FString DescribeNight() const;
	/** Moves the night's clock on (for testing closing time and tiredness). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestAdvanceClock(float ClockMinutes, float EnergySpent);
	/** Tournament: plays the next Hands of the whole room at once between hands (you play on autopilot). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestFastForward(int32 Hands);
	/** Tournament: adds chips to your stack (between hands). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestHeroChips(int32 Chips);
	void AdjustReload(int32 Steps);
	int64 GetReloadChips() const { return ReloadChips; }
	bool CanReload() const;
	/** Money not on the table, in cents. */
	int64 GetBankrollOffTable() const { return BaseCents; }
	/** "TUE 11:42 PM" */
	FString ClockLabel() const;
	/** 0..100 */
	float GetEnergy() const { return Energy; }
	/** 0..1: the eyelids closing (tired), drawn by the HUD. */
	float GetEyelids() const { return Eyelids; }
	/** Seconds into the walk in (for the title card), or -1. */
	float GetArrivalTime() const { return Phase == EBackRoomPhase::Arriving || ArrivalT < 9.0f ? ArrivalT : -1.0f; }
	/** The title card's lines while walking in. */
	FString ArrivalDay() const { return ArrivalDayText; }
	/** Seconds since racking up (for the summary), or -1. */
	float GetLeaveTime() const { return Phase == EBackRoomPhase::Leaving ? LeaveT : -1.0f; }
	const TArray<FString>& GetSummary() const { return Summary; }
	int64 GetNetCents() const { return NetCents; }
	bool IsFirstVisit() const { return bFirstVisit; }
	bool IsLeaveRequested() const { return bLeaveAsked; }

	// ------------------------------------------------------------ the Embercrest (BackRoomLive.cpp)
	/** A live tournament tonight (opened with "?Live=<occurrence id>") instead of Dee's game. */
	bool IsLive() const { return bLive; }
	/** Tonight's event as the room calls it ("Embercrest Nightly $120"). */
	const FString& GetLiveName() const { return LiveName; }
	/** The tournament for the HUD (null at Dee's game). */
	const ss::Tournament* GetTourney() const { return Tourney.Get(); }
	/** The level's time left on the clock screens, counting down between hands (game seconds). */
	double LevelTimeLeft() const;
	/** A big moment across the screen (HAND FOR HAND, THE BUBBLE HAS BURST, FINAL TABLE...), and its age. */
	const FString& GetBanner(float& OutAge) const
	{
		OutAge = BannerAge;
		return Banner;
	}
	/** Leaving a tournament early: L asks, L again within a few seconds confirms (your stack is blinded off). */
	bool IsQuitPending() const { return QuitAskedAt >= 0.0f; }
	/** The live table's pace (the Table pace setting): 0 Live, 1 Brisk, 2 Fast. T cycles it (and saves it). */
	int32 GetTablePace() const { return TablePace; }
	void CycleTablePace();
	/** Real seconds since the pace last changed (for the HUD's note). */
	float PaceShownAge() const { return static_cast<float>(FPlatformTime::Seconds() - PaceShownAt); }
	/** How quickly a hand the player is out of plays out, at this pace (time dilation). */
	float FoldedPace() const { return TablePace == 2 ? 4.0f : (TablePace == 1 ? 2.8f : 1.9f); }
	/** The persona for a cast member by name (the Back Room's regulars and the Embercrest's Sunday faces). */
	static FBackRoomPersona PersonaFor(const FString& Name);
	/** Who someone in tonight's field is to the player ("Embercrest regular", "Knows you", ...; empty for a stranger). */
	FString FaceNote(const FString& Name) const;
	/** Their badges, from the entry's livebadge lines (LiveBadges; none for anyone without): ue-wear fills them in BackRoomLiveCast.cpp, ue-table draws them. */
	ss::badge::Set FaceBadges(const FString& Name) const;
	/** The brand someone in tonight's field wears on their chest (the entry's livepatch lines: LivePatches), or empty (ue-wear). */
	FString FacePatch(const FString& Name) const;

	// ------------------------------------------------------------ the Embercrest on foot (BackRoomLive.cpp)
	/** Q at the table: up and walking (still dealt in: the dealer checks the hand when it's free and mucks it when not). */
	void LiveStandUp();
	/** E on foot: the chair (sit back down), the desk, the cashier, the bar, the terrace, the door. */
	void LiveInteract();
	/** Out of the tournament: Q stays in the room to watch the rest of it, on foot; L goes home now. */
	void LiveStay();
	void LiveLeaveNow();
	bool IsHeroUp() const { return bHeroUp; }
	bool IsRailing() const { return bRailing; }
	/** What the walker can reach right now, and its prompt ("" for nothing). */
	ECardRoomSpot CurrentSpot() const;
	FString SpotPrompt() const;
	/** Seconds into the leaving summary while staying is still offered (-1 when it isn't). */
	float StayOffer() const;
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestStandUp() { LiveStandUp(); }
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestInteract() { LiveInteract(); }
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestStay() { LiveStay(); }
	/** The cast's MetaHuman asset name for a player name ("Big Lou" -> "BigLou"), or empty. */
	static FString CastAssetFor(const FString& Name);

	// ============================================================ rail & room (BackRoomLive.cpp) -- this block's owner: rail & room
	// The Embercrest after the player's own event: the old table playing on, faces that stay with their people, the next
	// event from the desk, the floor's last call, and the way out through the resort's entrance.
	/** The event the desk registered the player for tonight and when it seats ("Registered: Midnight Turbo, seats at 10:20
	 *  PM"; empty for none), for the on-foot line. */
	FString GetNextEventNote() const;
	/** Testing: the player busts at the next gap between hands (a one-chip stack blinded out: the engine's own bust). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestBustNow();
	/** Testing: the whole room plays on (you on autopilot) to just before the floor's last call for registration. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestLastCall();
	/** Testing: the desk registers the player for the next event today (or OccurrenceId) as two presses of E there do, from
	 *  anywhere in the room once their night is over (off the summary and onto the rail first). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestRegisterNext(const FString& OccurrenceId);
	/** Testing: the wait for the next event passes, as stepping out on the terrace does, up to the floor calling its seats. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestSeatingNow();

	// ============================================================ the next segment (next_segment_spec 8.2, 8.3)
	// Declared by the lead in Phase A for the four Unreal tracks; this header froze after it. Each body sits in the file of
	// the track that fills it (empty, or forwarding, until it does).

	// ---- shared fast-forwards (the lead; BackRoomLiveCommands.cpp: ss.Live.Bubble, ss.Live.Final, ss.Live.Break, ss.Live.Shove)
	/** Testing: the whole room plays on at once (you on autopilot) until Moment: "bubble" (hand for hand), "final" (the final
	 *  table) or "break" (the next break). A table move on the way is walked, then it goes on. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestFastForwardTo(const FString& Moment);
	/** Testing: the next opponent to act at your table while you're still in the hand moves all in (facing an all-in). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestShove();

	// ---- ue-room: the director's host, the card room's sound, captions (BackRoomLiveRoom.cpp, BackRoomLiveCommands.cpp)
	/** The director's cue played: the chime (PlayEffect), the speech (through PaChain for the PA), the caption to the HUD
	 *  queue. Floor() becomes this (spec 4.11). */
	void PlayCue(const ss::room::Cue& Cue);
	/** A line for the HUD's caption zone and the night log (the table card's NIGHT page). */
	void PushCaption(const ss::ui::Caption& Caption);
	const std::vector<ss::ui::Caption>& GetCaptions() const { return Captions; }
	const std::vector<ss::ui::Caption>& GetNightLog() const { return NightLog; }
	/** The card room's stereo sound (null at Dee's game and until ue-room builds it). */
	FCardRoomSound* GetRoomSound() const { return RoomSound.Get(); }
	/** ss.Live.Room: the mood, each layer's dB, the voices in use, synth ms (average and peak), engine voices, the last 10 cues. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString DescribeRoom() const;
	/** ss.Live.Cue <moment> [seat]: the director says that moment now (a talk::Moment's name), from Seat at your table or anyone. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestCue(const FString& Moment, int32 Seat);

	// ---- ue-table: the HUD info fill, plates, presets, the table card, toasts (BackRoomHud.cpp)
	/** Everything the core table HUD draws this frame (ss::ui::DrawTableHud), in its 1,080-tall canvas CanvasWidth wide. */
	void FillTableHud(ss::ui::TableHudInfo& Out, float CanvasWidth) const;
	/** The table card's page (ss::ui::DrawTableCard). */
	void FillTableCard(ss::ui::TableCardInfo& Out) const;
	/** The AI stack plates as of this frame (GetSeatViews projected into the HUD's canvas; spec 6.3), the hero's included. */
	const std::vector<ss::ui::SeatPlate>& GetPlates() const { return Plates; }
	/** Each frame: the plates from the table's seats (projection, fades, 2 s after a seat acts), then the audit. */
	void UpdatePlates(float RealDt);
	/** ss.Table.PlateAudit 1: every plate against the engine's stack for its seat; a mismatch that outlasts the chips' landing is logged. */
	void PlateAudit(float RealDt);
	/** The settings the table runs on (the pause menu's: captions, stack readout and units, turn cue, the mix), or null before Begin. */
	const ss::ui::GameSettings* GetSettings() const;
	/** U (or Y with the table card up): big blinds and chips swap places on the plates and the hero block. */
	void FlipStackUnits();
	/** The toast lane (a text, a sponsor bonus, the sit-down patch line), held while it's the player's decision (spec 1.6). */
	void PushToast(const FString& From, const FString& Line, uint32 Accent = 0);
	const std::vector<ss::ui::Toast>& GetToasts() const { return Toasts; }
	/** The last hands the player played (recall::Of at EndHand), newest last, at most 10: HANDS and the last-hand card. */
	const std::vector<ss::recall::Record>& GetRecentHands() const { return RecentHands; }
	/** The raise presets on the ladder for the hero's decision now (ss::sizing::Presets) and the one selected (-1: none):
	 *  keys 1-6 and the bumpers step them (the table's HeroPreset and HeroSnap carry them out; spec 6.2 D, 6.4). */
	const std::vector<ss::sizing::Preset>& GetPresets() const { return Presets; }
	int32 GetPresetIndex() const { return PresetIndex; }
	/** The rail's stage view: the stream table from a fixed camera with the ON AIR lower third (spec 6.5). */
	bool IsWatchingFeature() const { return bFeatureView; }
	void WatchFeature(bool bOn);

	// ---- ue-venue: the desk screen, the room's clocks, the seat marker, the badge atlas (BackRoomLiveVenue.cpp)
	/** The desk or the cage (ss::ui::DeskScreen, modal), Window "desk" or "cage"; its requests go through DeskSignup,
	 *  FinishSignup and LiveUnregister. */
	void OpenDesk(const FString& Window);
	void CloseDesk();
	bool IsDeskOpen() const { return bDeskUp; }
	/** Where the seat marker points on foot (the hero's chair, world space; spec 6.5). */
	FVector SeatMarkerTarget() const;
	/** The badge glyphs, one render at map start (null until ue-venue builds it). */
	const FBadgeAtlas* GetBadgeAtlas() const { return BadgeAtlas.Get(); }
	/** Testing: the desk or the cage opened (as E there does), and a key pressed on it ("Up", "Enter", "Escape"...). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestDeskOpen(const FString& Window);
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestDeskKey(const FString& Key);

private:
	// ---- the lead: the shared fast-forwards' stop (0 none, 1 hand for hand, 2 the final table, 3 a break)
	uint8 FastForwardStop = 0;
	bool FastForwardReached() const;

	// ---- ue-room
	/** Built once the field is loaded, Resume'd from the entry's liveroom and the checkpoint (spec 4.11). */
	TSharedPtr<ss::room::Director> RoomDirector;
	TSharedPtr<FCardRoomSound> RoomSound;
	/** The VoiceBank seam (spec 4.10): a clip by cue Key and speaker over the murmur; empty this segment. */
	TSharedPtr<FVoiceBank> VoiceBank;
	TSharedPtr<FRoomHost> RoomHost;
	/** Lines up now (the HUD's caption zone) and the night's (the NIGHT page). */
	std::vector<ss::ui::Caption> Captions;
	std::vector<ss::ui::Caption> NightLog;
	/** The last cues played, newest last (ss.Live.Room shows 10). */
	std::vector<ss::room::Cue> RecentCues;
	/** The director built (after LoadLive, from the entry), fed each frame (events, rounds, the hand, the clock), its cues played. */
	void BuildDirector();
	void TickRoom(float RealDt);
	/** The rail's backers (stake::RailFor): at most two, at the bar line or the stage edge (spec 2.13). */
	void PlaceBackers();
	UPROPERTY(Transient)
	TArray<TObjectPtr<ABackRoomPlayer>> RailBackers;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> RoomObjects;

	// ---- ue-wear: the field's badges and patches, by name (filled beside FaceNote's LiveFaces, BackRoomLiveCast.cpp)
	TMap<FString, ss::badge::Set> LiveBadges;
	TMap<FString, FString> LivePatches;
	TSharedPtr<FWearHost> WearHost;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> WearObjects;

	// ---- ue-table
	std::vector<ss::ui::SeatPlate> Plates;
	std::vector<ss::sizing::Preset> Presets;
	int32 PresetIndex = -1;
	std::vector<ss::ui::Toast> Toasts;
	std::vector<ss::recall::Record> RecentHands;
	/** ss.Table.PlateAudit: how long each seat's plate has disagreed with the engine (seconds, by table seat). */
	TMap<int32, float> PlateMismatchFor;
	bool bFeatureView = false;
	TSharedPtr<FTableHost> TableHost;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> TableObjects;

	// ---- ue-venue
	ss::ui::DeskScreen Desk;
	FString DeskWindow;
	bool bDeskUp = false;
	TSharedPtr<SDrawListWidget> DeskWidget;
	TSharedPtr<FBadgeAtlas> BadgeAtlas;
	/** The room's clock screens (ss::ui::DrawRoomClock at about 2 Hz; pages CLOCK, PAYOUTS, TODAY, LEADERS) and their painter. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextureRenderTarget2D>> RoomClockTargets;
	TSharedPtr<FCoreCanvasTarget> RoomClockCanvas;
	int32 RoomClockPage = 0;
	float RoomClockT = 0.0f;
	void UpdateRoomClocks(float RealDt);
	/** What the desk screen shows now (ss::ui::DeskInfo; the card room has a SaveData, not a Session). */
	void FillDeskInfo(ss::ui::DeskInfo& Out) const;
	/** Each frame while it's up: the requests it made carried out. */
	void TickDesk(float RealDt);
	TSharedPtr<FVenueHost> VenueHost;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> VenueObjects;
	// ============================================================ end of the next segment

	// ============================================================ update 2: autoplay (update 2 SPEC §2.12)
	// Declared by the lead in Phase A for ue-auto, once; the header froze again after it. The bodies are ue-auto's
	// (BackRoomAuto.cpp, forwarding to FAutoHost in BackRoomAuto.h), and the hot files call FAutoHost's hooks from the sites
	// that header lists. Until ue-auto fills them nothing turns autoplay on: the seat is the player's.
public:
	/** The plan holds the hero's seat now: autoplay on, this hand not taken over, the player in the chair. */
	bool IsAutoPlaying() const;
	/** The watch speed (1, 2, 4 or 8) while the plan holds the seat, the pawn's pace target for the whole hand; 1 otherwise. */
	float AutoWatchSpeed() const;
	/** The table's plan from the next decision on (K, the PLAY page, ss.Auto.*): the style, watching or skipping, the speed,
	 *  the skip's target and its stops. Off hands the seat back at the next decision. */
	void SetAuto(const ss::autoplay::TableAuto& Plan);
	/** Takes the wheel: this hand is the player's (pressed again in the next hand's first 3 s: autoplay off). */
	void AutoTakeOver();
	/** Skips ahead to To (Hands of them for Target::Hands), between hands only (§2.6.4). */
	void AutoSkip(ss::autoplay::Target To, int32 Hands = 0);
	/** ss.Auto.Describe: the style, mode, speed, Sharpness and its causes, Form, tilt, the counts, tonight's reads. */
	FString DescribeAuto() const;

private:
	friend struct FAutoHost;
	TSharedPtr<FAutoHost> AutoHost;
	// ============================================================ end of update 2

private:
	/** The engine table playing where the player's own table stands (the room's anchor), or -1 (it broke, or went to the stage). */
	int32 AnchorTableId();
	/**
	 * The body an entrant wears in the room: their own MetaHuman (the cast), the one they've worn up close tonight, or the
	 * one their tournament id picks among the bodies that suit them (a regular of the room's: theirs), passing over any in
	 * Worn at the table they're sitting down at. Kept for the night, so a face follows its person from table to table.
	 */
	FString LiveBodyFor(const FString& Id, const FString& Name, const TSet<FString>& Worn);
	/** The far crowd's seated figure for an entrant: the one baked from the body they wear up close (their id's, unseen). */
	int32 CrowdKindFor(const FString& Id, const FString& Name) const;
	/** The floor's last call for registration a few minutes before late registration closes, and the close. */
	void LiveLastCall();
	/** The next event today the desk could register the player for (its occurrence id; empty for none), or why not. */
	FString NextOpenEvent(FString& OutWhyNot) const;
	/** The desk writes the player in for OccurrenceId: the world's field read on a worker, then the entry (FinishSignup). */
	void DeskSignup(const FString& OccurrenceId);
	void FinishSignup();
	/** Leaving before the next event's cards: off the list, refunded, and the world told nothing happened. */
	void LiveUnregister();
	/** The floor calls the next event's seats; a moment later the room opens again for it (LiveOpenNext). */
	void LiveCallNext();
	void LiveOpenNext();
	/** The rest of the room played on (railing) to world minute Until, quietly: the terrace, or a test, passing the wait. */
	void RailCatchUp(double Until);
	/** The world's people and places the player saw from the rail (NoteRailedPlaces), into D. */
	void NoteRailed(ss::SaveData& D) const;
	/** Who wears which body tonight (tournament id -> body), once seen up close. */
	TMap<FString, FString> LiveBodies;
	/** The near tables' people (Extras, by index): the body each one is, and whom it's sitting for now. */
	TArray<FString> ExtraBody;
	TArray<FString> ExtraWho;
	/** The old table's chairs settle a moment after each railed round (a bust says goodbye before getting up). */
	bool bRailSeatsDue = false;
	/** The floor's last call for registration, and the close, once each. */
	bool bLastCalled = false;
	bool bRegClosed = false;
	/** The biggest pot shown down with each of the world's people tonight (tournament id -> the player's net, chips). */
	TMap<FString, int64> LiveBigPots;
	/** Testing: bust at the next gap between hands. */
	bool bTestBust = false;
	/** The night's departure from home (the first event's bus; ?From for Night One), and an event entered from the room. */
	double NightLeftHomeAt = 0.0;
	bool bHere = false;
	/** This entry was registered at the desk with the player already in the room (its bus home rode on the first's). */
	bool bDeskEntry = false;
	/** The desk: the event offered on the first press of E, and when. */
	FString DeskOffer;
	float DeskAskedAt = -100.0f;
	/** The next event tonight: its occurrence id, its name, when its seats open (world minutes), the call, the reopen. */
	FString NextEntryId;
	FString NextName;
	double NextSeatingAt = 0.0;
	bool bNextCalled = false;
	float NextCallT = 0.0f;
	bool bOpeningNext = false;
	/** Out on the terrace while the wait for the next event passes. */
	bool bWaitingOut = false;
	/** The desk's signup on its worker (the world's field for the event), and the world as it was before it. */
	TSharedPtr<class FLiveDeskSignup> Signup;
	std::string WorldBeforeSignup;
	// ============================================================ end of rail & room

private:
	ABackRoomStage* FindOrSpawnStage();
	void SeatEveryone();
	ABackRoomPlayer* SpawnPerson(const FString& CastName, const FTransform& At, EBackRoomRole AtTableAs, const FBackRoomPersona& Persona);

	// The Embercrest (BackRoomLive.cpp).
	bool LoadLive();
	void SeatLive();
	void LiveTick(float RealDt);
	void LiveEvent(const ss::TEvent& E);
	void LiveNote(uint8 Note);
	ABackRoomPlayer* SeatCast(const ss::TPlayer& P, int32 TableSeat);
	void UnseatCast(ABackRoomPlayer* Player, const FString& Id, bool bBusted);
	/** Whether a point is in (or near) the hero's view. */
	bool HeroCanSee(const FVector& At) const;
	/**
	 * A chair changing hands where the hero can see it (a bust, a player moved in) waits for a blink: the eyes close for a
	 * tenth of a second, the bodies swap, the eyes open; nobody pops in or out in plain sight. Out of view it's at once.
	 */
	void BlinkSwap(ABackRoomPlayer* Player, bool bShow);
	void ApplyBlinkSwaps();
	void LiveMove();
	void LiveOver();
	void LiveSettle();
	void LiveGoHome();
	void PlaceExtras();
	/** Today's schedule by the entrance, with each event's state now. */
	void UpdateRoomBoards();
	void UpdateBoard(float RealDt);
	void RaiseBanner(const FString& Text);
	void Floor(const FString& Line, bool bChime = true);
	void LiveRequestLeave();
	FString Chips(int64 Amount) const;
	int32 SlotForTable(int32 TableId);
	/**
	 * After each hand the player sees, the night is saved where it stands (ss::Tournament::Checkpoint, on a worker
	 * thread), and marked again when the next hand is dealt: a game closed during the night comes back to the seat
	 * after the last hand, and a hand that was already dealt is dead (folded), never dealt again.
	 */
	void LiveCheckpoint(bool bDealt);
	/** The room's own part of a checkpoint (hands played, who sat with the player, reads, a break), and reading it back. */
	FString LiveHostText(bool bDealt) const;
	void ReadLiveHost(const FString& Text);

	bool bLive = false;
	TSharedPtr<ss::Tournament> Tourney;
	int32 TablePace = 0;
	double PaceShownAt = -100.0;
	void ApplyTablePace();
	int32 LiveDay = 0;
	/** The event day's midnight in world minutes (the tournament's clock is minutes after it). */
	double LiveDayStart = 0.0;
	/** The player's entry (the occurrence id) and the event as the room names it. */
	FString LiveEntryId;
	FString LiveName;
	FString LiveShort;
	double LiveLateRegEnds = 0.0;
	double LiveBreakMinutes = 0.0;
	/** Fewer entrants than the event runs with: cancelled, the entry refunded. */
	bool bCancelled = false;
	/** On a break: the table holds, the screens count down, then the floor calls everyone back. */
	bool bOnBreak = false;
	float BreakT = 0.0f;
	int32 BreakLevel = 0;
	/** Everyone who sat at the player's table tonight (tournament ids), for the world to remember. */
	TSet<FString> LiveMet;
	float RoomBoardTick = 0.0f;
	/** The player confirmed leaving while a hand or the room's round was still being played: they go when it's done. */
	bool bLeaveWhenFree = false;
	/** The faces in tonight's field (the entry's): name -> (who they are to the player, what they say sitting down). */
	TMap<FString, TPair<FString, FString>> LiveFaces;
	int32 GreetCount = 0;
	/** Back after the game closed during the night: no bus, a few steps back to the chair. */
	bool bBackInRoom = false;
	/** ...and the room as it stood after the last hand (the checkpoint read back); the hand dealt then is dead. */
	bool bResumed = false;
	bool bResumeDealt = false;
	TMap<FString, int32> ResumeReads;
	bool bCheckpointDue = false;
	int32 Checkpoints = 0;
	// On foot.
	bool bHeroUp = false;
	bool bAwayExplained = false;
	bool bRailing = false;
	bool bCollected = false;
	bool bExitNow = false;
	float ExitT = 0.0f;
	float ExitAskedAt = -100.0f;
	float RailT = 0.0f;
	float LastWater = -1000.0f;
	float LastAir = -1000.0f;
	double BustMinutes = 0.0;
	/** The player's live record before tonight (the desk and the cage know a regular). */
	int32 PastLiveEvents = 0;
	int32 PastLiveCashes = 0;
	int32 PastBestPlace = 0;
	void LiveSitDown();
	/** Railing: the room plays on (a round every few seconds), the floor calls it, the boards keep up. */
	void LiveRail(float RealDt);
	/** What the player watched after their finish, for the world (and the clock they got home by). */
	void NoteRailedPlaces();
	std::string LastCheckpoint;
	double LastCheckpointAt = 0.0;
	TSharedPtr<FCareerSaver> LiveSaver;
	/** Who's in which body: tournament player id -> actor (kept, hidden, while they're elsewhere). */
	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<ABackRoomPlayer>> CastActors;
	/** The room's other tables: extras by slot (dealer and two players each). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ABackRoomPlayer>> Extras;
	TMap<int32, int32> SlotByTable;
	int32 HeroPlace = 0;
	int64 HeroPrizeCents = 0;
	bool bHeroWon = false;
	bool bLiveOver = false;
	bool bAnnouncedStart = false;
	bool bGhostMet = false;
	bool bHandForHand = false;
	bool bFinalTable = false;
	FString Banner;
	float BannerAge = 99.0f;
	float BoardTick = 0.0f;
	float QuitAskedAt = -1.0f;
	float LiveT = 0.0f;
	float MoveT = -1.0f;
	int32 AnonymousLooks = 0;
	int32 FastForwardLeft = 0;
	bool bTestMove = false;
	float LastFarewellAt = -100.0f;
	TArray<TPair<TWeakObjectPtr<ABackRoomPlayer>, bool>> BlinkSwaps; // body, shown (else gone) after the blink
	float BlinkT = -1.0f;
	int32 TestChips = 0;
	int32 RecentMovedIn = 0;

	// The career.
	bool LoadCareer();
	/** Writes the save: the bankroll with the chips on the table counted; bFinal settles the night. */
	void SaveCareer(bool bFinal);
	void OnTableNote(uint8 Note);
	void BeginArrival();
	/** The walk in from the laundromat, once the pawn is there to take it. */
	void StartWalkIn(ABackRoomPawn* Pawn);
	void BeginLeaving();
	void GoHome();
	FString ArrivalLine() const;
	FString GreetingLine(FString& Who) const;
	FString GoodbyeLine() const;
	/** The weekday the night belongs to (the day its doors opened), 0 Monday. */
	int32 NightWeekday() const;
	double TimeOfDay() const;

	EBackRoomPhase Phase = EBackRoomPhase::Practice;
	// Shared (not unique) so the header needs no complete ss::SaveData.
	TSharedPtr<ss::SaveData> Save;
	int64 HeroBuyInChips = 100;
	/** Bankroll at the door, the part of it not on the table, and the total bought in (cents). */
	int64 StartBankrollCents = 0;
	int64 BaseCents = 0;
	int64 BoughtInCents = 0;
	int64 NetCents = 0;
	/** World minutes (net::MinutesPerDay * day + minutes), now and on leaving home. */
	double Minutes = 0.0;
	double LeftHomeAt = 0.0;
	float Energy = 50.0f;
	bool bFirstVisit = true;
	int32 PastNights = 0;
	int64 PastNetCents = 0;
	int64 ReloadChips = 100;
	bool bLeaveAsked = false;
	bool bLastHandCalled = false;
	bool bSentHome = false;
	bool bClosed = false;
	bool bBustedOut = false;
	int32 HandsPlayed = 0;

	// Arrival and leaving.
	float ArrivalT = 99.0f;
	FString ArrivalDayText;
	int32 ArrivalBeat = 0;
	float LeaveT = 0.0f;
	bool bWalkingOut = false;
	bool bFadingOut = false;
	bool bGoingHome = false;
	float AttentionTick = 0.0f;
	TArray<FString> Summary;

	// Tired eyes.
	float Eyelids = 0.0f;
	float NextDroop = 8.0f;
	float DroopT = -1.0f;
	float DroopLength = 1.0f;
	float DroopDepth = 0.8f;

	// ============================================================ table input (BackRoomGameMode.cpp) -- this block's owner: table input
	// The pause menu (Escape, P in Play-In-Editor, Start on a gamepad), quitting from it (R5: a hand in play is folded
	// first), and the player's settings at the table: look sensitivity and inverted look, the controls reminder, the field
	// of view and the lens, the mix, the table's pace.
public:
	/**
	 * Opens the pause menu (FrontEnd's Pause page, named for the place). The night stands still behind it: nothing is dealt,
	 * played or walked, and neither the tournament's clock nor the world's minutes move (LIVE_TOURNAMENTS.md, Time).
	 */
	void OpenPauseMenu();
	/** The pause menu has the screen and the input (the pawn and the HUD hold). */
	bool IsPauseMenuUp() const;
	/** A confirmed quit is under way: the hand in play folded and played out behind the fade, then out. */
	bool IsQuittingTable() const;
	/** The player's look sensitivity (a multiple of the table's own) and inverted look, from Settings. */
	float GetLookScale() const;
	bool IsLookInverted() const;
	/** Settings > Controls > Control hints: the table's controls reminder. */
	bool ShowsHints() const;
	/** One of the table's views at the player's Field of View setting (DefaultFov: its horizontal degrees at the default). */
	float SettingFov(float DefaultFov) const;
	/** The player's image settings on the pawn's lens, each frame: brightness over the room's exposure, film grain, color
	 *  fringing, motion blur. */
	void ApplyPlayerLens(UCameraComponent* Camera) const;
	/** Testing: opens the pause menu, or resumes, as Escape and the menu's Resume do. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestPause(bool bOpen);
	/** Testing: the pause menu's confirmed quit, to the main menu or the desktop (the menu needn't be up). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestQuit(bool bToDesktop);
	/** Testing: paused or not, the menu's page, a quit under way, and what the quit cards would say now. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString TestDescribeMenu() const;

private:
	/** The pause menu and its hooks, the player's settings, the night frozen behind it, a quit under way (BackRoomGameMode.cpp). */
	struct FTableMenu;
	TSharedPtr<FTableMenu> TableMenu;
	// ============================================================ end of table input
};
