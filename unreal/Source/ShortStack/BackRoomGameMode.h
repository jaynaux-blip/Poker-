#pragma once

#include "CoreMinimal.h"
#include "FrameBudget.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"

#include "BackRoomGameMode.generated.h"

class ABackRoomPlayer;
class ABackRoomStage;
class ABackRoomTable;
class UCameraComponent;
class UPointLightComponent;

namespace ss
{
struct SaveData;
class Tournament;
struct TEvent;
struct TPlayer;
}
struct FBackRoomPersona;
enum class EBackRoomRole : uint8;

/**
 * You, at the Back Room's table: a camera in your own head.
 *
 * The body is an ABackRoomPlayer in the hero's seat (its face hidden from you): the camera rides its
 * head, the head turns where you look, and its hands do what you do. Hold Space (or the left mouse
 * button) to lift the corners of your cards; hold the right mouse button to study whoever you look at
 * (Focus: the view narrows, time slows, a face sharpens; it drains while you hold it). F folds, C
 * checks or calls, R bets or raises to the amount shown (the mouse wheel changes it), A goes all in.
 * Hold Shift to breathe slowly (the heart settles; it costs Focus). L racks up and goes home.
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
	 * laundromat to the seat, or (bOut) up from the seat and out the door. OnDone when it arrives.
	 */
	void PlayWalk(const TArray<FVector>& Points, float Seconds, bool bOut, TFunction<void()> OnDone);
	bool IsWalking() const { return bWalking; }
	/** 0..1 along the current walk. */
	float WalkProgress() const { return bWalking ? FMath::Clamp(WalkT / WalkSeconds, 0.0f, 1.0f) : 1.0f; }
	/** Where the camera is (for the regulars to look at while you walk by). */
	FVector GetEye() const;

private:
	void HandleInput(float RealDt);
	void TickWalk(float RealDt);
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
	bool bBodyShown = true;
	TFunction<void()> WalkDone;
	FVector WalkAt(float Distance) const;

	// The heart.
	float BeatPhase = 0.0f;
	/** The table's pace: quicker once you're out of the hand (a tournament). */
	float Pace = 1.0f;
	int32 StepCount = 0;
	float Kick = 0.0f;
	float Steady = 0.0f;
	float BreathT = 0.0f;
};

/** Prompts, stacks, Focus, the heart, the clock, reads and the table talk, drawn over the view. */
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

	// ------------------------------------------------------------ the Riverside (BackRoomLive.cpp)
	/** A live tournament tonight (opened with "?Live=<occurrence id>") instead of Dee's game. */
	bool IsLive() const { return bLive; }
	/** Tonight's event as the room calls it ("Riverside Nightly $120"). */
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
	/** The persona for a cast member by name (the Back Room's regulars and the Riverside's Sunday faces). */
	static FBackRoomPersona PersonaFor(const FString& Name);
	/** The cast's MetaHuman asset name for a player name ("Big Lou" -> "BigLou"), or empty. */
	static FString CastAssetFor(const FString& Name);

private:
	ABackRoomStage* FindOrSpawnStage();
	void SeatEveryone();
	ABackRoomPlayer* SpawnPerson(const FString& CastName, const FTransform& At, EBackRoomRole AtTableAs, const FBackRoomPersona& Persona);

	// The Riverside (BackRoomLive.cpp).
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
	void UpdateBoard(float RealDt);
	void RaiseBanner(const FString& Text);
	void Floor(const FString& Line, bool bChime = true);
	void LiveRequestLeave();
	FString Chips(int64 Amount) const;
	int32 SlotForTable(int32 TableId);

	bool bLive = false;
	TSharedPtr<ss::Tournament> Tourney;
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
	/** The player confirmed leaving while a hand or the room's round was still being played: they go when it's done. */
	bool bLeaveWhenFree = false;
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
};
