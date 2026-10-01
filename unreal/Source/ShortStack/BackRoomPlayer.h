#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "BackRoomAnim.h"

#include "BackRoomPlayer.generated.h"

class ABackRoomCard;
class ABackRoomChips;
class USkeletalMeshComponent;
class UStaticMeshComponent;

/** What a person at the table is doing there. */
UENUM()
enum class EBackRoomRole : uint8
{
	/** An opponent: plays on their own, and their body gives them away. */
	Player,
	/** Dee: deals, runs the pot, watches. */
	Dealer,
	/** You: the hands are yours to move (the table drives them from your input); the face is hidden. */
	Hero,
};

/** A physical tell: something the body does that it shouldn't. */
UENUM(meta = (ScriptName = "BackRoomTellKind"))
enum class EBackRoomTell : uint8
{
	/** A quick look down at their own chips when the board helps them. */
	ChipGlance,
	/** The eyebrows flick up at a good card, a fraction of a second. */
	BrowFlash,
	/** A dry swallow: the throat working under stress. */
	Swallow,
	/** The lips pressed until they disappear. */
	LipPress,
	/** Goes still: breath held, blinks stop, eyes fixed; a burst of blinks once it's over. */
	Freeze,
	/** Holds your eyes, daring you. */
	StareDown,
	/** Studied disinterest: looks off, talks to someone else. */
	LookAway,
	/** The hands shake pushing chips in: adrenaline. */
	Tremble,
	/** A hand to the neck: the body calming itself. */
	NeckTouch,
	/** A smile with the mouth only: the eyes stay flat. */
	FalseSmile,
	/** A real smile leaking out (the eyes crinkle), then swallowed. */
	RealSmile,
	/** Rapid blinking when the pressure lets go. */
	BlinkBurst,
	/** A theatrical sigh and shrug: acting weak. */
	Sigh,
	/** Looks at their hole cards again (did that card help?). */
	Recheck,
	/** The pupils open up. Only visible up close. */
	PupilFlare,
	/** Reaches for chips while you decide, as if to call: trying to stop your bet. */
	ChipReach,
};

/** What a tell gives away when it is honest. */
UENUM()
enum class EBackRoomTellMeaning : uint8
{
	/** A strong hand. */
	Strong,
	/** A weak hand, or a missed draw. */
	Weak,
	/** Betting without the goods. */
	Bluff,
};

/** One tell of one player, and how honest it is. */
USTRUCT(BlueprintType)
struct FBackRoomTell
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Tell")
	EBackRoomTell Tell = EBackRoomTell::ChipGlance;

	UPROPERTY(EditAnywhere, Category = "Tell")
	EBackRoomTellMeaning Means = EBackRoomTellMeaning::Strong;

	/** Chance it shows when it should (0..1). */
	UPROPERTY(EditAnywhere, Category = "Tell")
	float Reliability = 0.7f;

	/** Chance it shows when it shouldn't (noise, or a player who knows they have it). */
	UPROPERTY(EditAnywhere, Category = "Tell")
	float FalseRate = 0.1f;
};

/** How a player carries themselves: temperament, and how much of it the face lets out. */
USTRUCT(BlueprintType)
struct FBackRoomPersona
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Persona")
	FString Name = TEXT("Player");

	/** Resting nerves 0..1 (breathing, blinking, fidgeting). */
	UPROPERTY(EditAnywhere, Category = "Persona")
	float Nervousness = 0.3f;

	/** How much emotion reaches the face 0..1 (a pro's poker face is low). */
	UPROPERTY(EditAnywhere, Category = "Persona")
	float Expressiveness = 0.5f;

	/** How often the eyes wander 0..1. */
	UPROPERTY(EditAnywhere, Category = "Persona")
	float Restlessness = 0.5f;

	/** Playing with chips when idle 0..1. */
	UPROPERTY(EditAnywhere, Category = "Persona")
	float ChipFidget = 0.4f;

	/** Leans in (1) or sprawls back (0) at rest. */
	UPROPERTY(EditAnywhere, Category = "Persona")
	float Posture = 0.5f;

	/** How much they talk 0..1. */
	UPROPERTY(EditAnywhere, Category = "Persona")
	float Chatter = 0.3f;

	/**
	 * How they read YOUR hands shaking when you bet (-1..1): above 0 they read it right (and the
	 * higher, the more often they notice); below 0 they notice and get it wrong (nerves mean a bluff).
	 */
	UPROPERTY(EditAnywhere, Category = "Persona")
	float HeroRead = 0.0f;

	/** Seed for this player's own rhythms. */
	UPROPERTY(EditAnywhere, Category = "Persona")
	int32 Seed = 1;

	/** The color of their shirt (linear), dyed onto the MetaHuman outfit. */
	UPROPERTY(EditAnywhere, Category = "Persona")
	FLinearColor Shirt = FLinearColor(0.5f, 0.5f, 0.5f);

	/** What their body gives away (2 to 4 each; pros have reverse tells). */
	UPROPERTY(EditAnywhere, Category = "Persona")
	TArray<FBackRoomTell> Tells;
};

/** How the player is placed at the table: their seat's spots on the felt (world space). */
struct FBackRoomSeatSpots
{
	FVector Cards = FVector::ZeroVector;
	FVector Stack = FVector::ZeroVector;
	FVector Bet = FVector::ZeroVector;
	/** Toward the middle of the table, along the felt. */
	FVector Inward = FVector::ForwardVector;
};

/**
 * A person at the Back Room's table: a MetaHuman body and face, seated and animated procedurally.
 *
 * The actor sits at a chair's front edge facing the table down its +X. Each frame it runs a small
 * behavioral model (breathing, blinks, gaze, posture, hands, emotion) and hands the result to its body
 * and face anim instances as an FBackRoomBodyPose and a set of RigLogic expression controls.
 *
 * On top of that model the table plays it: the hands carry out gestures (sliding chips out, knocking
 * a check, peeking at cards, dealing), and what the player knows about their own hand moves their
 * emotions and fires their tells (BackRoomPlayerTells.cpp). Gestures live in BackRoomPlayerHands.cpp.
 */
UCLASS()
class SHORTSTACK_API ABackRoomPlayer : public AActor
{
	GENERATED_BODY()

public:
	ABackRoomPlayer();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }

	UPROPERTY(EditAnywhere, Category = "Short Stack")
	FBackRoomPersona Persona;

	UPROPERTY(EditAnywhere, Category = "Short Stack")
	EBackRoomRole SeatRole = EBackRoomRole::Player;

	// ------------------------------------------------------------ what the table tells the player
	/** World points of interest: the player to watch, the pot, the dealer. */
	UPROPERTY(EditAnywhere, Category = "Short Stack|Table")
	FVector HeroEyes = FVector(-98.0, 0.0, 124.0);
	UPROPERTY(EditAnywhere, Category = "Short Stack|Table")
	FVector PotAt = FVector(0.0, 0.0, 76.0);
	UPROPERTY(EditAnywhere, Category = "Short Stack|Table")
	FVector DealerAt = FVector(100.0, 0.0, 125.0);
	UPROPERTY(EditAnywhere, Category = "Short Stack|Table")
	TArray<FVector> OthersAt;

	/** This seat's spots on the felt, and the things on them (set by ABackRoomTable). */
	FBackRoomSeatSpots Spots;
	UPROPERTY(Transient)
	TArray<TObjectPtr<ABackRoomCard>> Hole;
	UPROPERTY(Transient)
	TObjectPtr<ABackRoomChips> StackPile;
	UPROPERTY(Transient)
	TObjectPtr<ABackRoomChips> BetPile;

	/** Emotional state (-1..1 valence, 0..1 arousal, 0..1 dominance), eased toward by the game. */
	UPROPERTY(EditAnywhere, Category = "Short Stack|Mood")
	float Valence = 0.0f;
	UPROPERTY(EditAnywhere, Category = "Short Stack|Mood")
	float Arousal = 0.25f;
	UPROPERTY(EditAnywhere, Category = "Short Stack|Mood")
	float Dominance = 0.5f;

	/** Sets a RigLogic control (CTRL_expressions_*, without the prefix) on top of everything, for testing. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Short Stack")
	void SetTestExpression(FName Control, float Value);

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Short Stack")
	void ClearTestExpressions();

	/** Fires a tell now (for testing and for the Focus tutorial). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack")
	void PlayTell(EBackRoomTell Tell, float Intensity = 1.0f);

	/** A MetaHuman body and face to use instead of the archetypes (from the Creator, once assembled). */
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	TSoftObjectPtr<USkeletalMesh> BodyMesh;
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	TSoftObjectPtr<USkeletalMesh> FaceMesh;
	/** An assembled MetaHuman Blueprint (BP_<Name>): its meshes, hair and clothes are worn by this player. */
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	TSoftClassPtr<AActor> MetaHumanClass;

	// ------------------------------------------------------------ the game (ABackRoomTable)
	/** New hand: forget the last one. */
	void BeginHand();
	/** What this player believes their hand is worth now (0..1 equity), and whether they are still in. */
	void SetHandStrength(float Strength, bool bInHand);
	/** Look at the hole cards (lift the corners), then react. Returns how long it takes. */
	float PeekHole();
	/** New board cards are out: react to what they did to the hand. */
	void OnBoard(float NewStrength);
	/** Their turn: thinking about ToCall into Pot (chips), with how strong they are. */
	void BeginThink(int64 ToCall, int64 Pot, int64 Stack);
	/** They acted: a bet or raise (with or without the goods), or not. */
	void OnActed(bool bAggressive, bool bBluff);
	/** Someone else acted (Seat is the world point of that player) with Aggression 0 check/call .. 1 raise. */
	void OnOtherAction(const FVector& Who, float Aggression, bool bHeroActing);
	/** The hero is deciding (true) or done (false): opponents still in watch and leak. */
	void OnHeroThinking(bool bThinking, int64 HeroToCall);

	/** Gestures (BackRoomPlayerHands.cpp). Each returns its length in seconds. */
	float GestureCheck();
	/** Slides Amount from the stack to the bet (the table updates the piles' amounts as chips move). */
	float GestureChips(int64 Amount, bool bAllIn, bool bBluff);
	float GestureFold(const FVector& Muck);
	float GestureShow();
	/** Dealer: pitches a card from the deck to lie at Target. */
	float GestureDeal(ABackRoomCard* Card, const FTransform& Target, bool bFaceUp);
	/** Dealer: draws Pile across the felt to To (gathering bets, pushing the pot); OnDone when it gets there. */
	float GestureSweep(ABackRoomChips* Pile, const FVector& To, TFunction<void()> OnDone = nullptr);
	/** Dealer: gathers the cards in (they slide to Muck). */
	float GestureCollect(const TArray<ABackRoomCard*>& Cards, const FVector& Muck);
	/** Dealer: the top of the deck in hand, where the next card comes from (face down). */
	FTransform GetDeckTop() const;
	/** Hero: peeking (hold) at the hole cards with both hands. */
	void SetHeroPeek(bool bPeeking);
	/** Hero: where the camera looks (world), so the head and neck follow it. */
	void SetHeroLook(const FVector& At) { HeroLook = At; }

	/** The game's result for this player: chips won (+) or lost (-), and whether cards were shown. */
	void OnResult(int64 Delta, int64 PotSize, bool bShowdown);
	/** Says a line (subtitled by the HUD); the jaw moves for its length. */
	void Say(const FString& Line);
	/** Lines said since the last call, for the HUD. */
	TArray<FString> TakeLines();

	/** Where the gestures' sounds go (ss::SoundId as int, world location, volume): set by the table. */
	TFunction<void(int32, const FVector&, float)> SoundHook;

	/** How hard the hero is studying this player (0..1, from the camera), for "stare too long and they notice". */
	void SetStudied(float Amount) { Studied = Amount; }
	float GetStudied() const { return Studied; }
	/**
	 * A tell that meant something (fired by the hand, not for a test) showed while the hero was looking:
	 * (this player, EBackRoomTell, EBackRoomTellMeaning, honest this time, how hard the hero was looking).
	 * Set by the table, for the read book.
	 */
	TFunction<void(ABackRoomPlayer*, uint8, uint8, bool, float)> TellHook;
	/** Hero: the hands shake this much (0..1) from the heart rate. */
	void SetExternalTremble(float Amount) { ExternalTremble = Amount; }
	/** Holding the hero's eyes now (a stare, or catching them staring), which raises the heart rate. */
	bool IsWatchingHero() const;
	/** World point between the eyes. */
	FVector GetEyes() const;
	USkeletalMeshComponent* GetBody() const { return Body; }
	bool IsBusy() const;

private:
	void Build();
	void BuildFromMetaHuman();
	void UpdateMood(float Dt);
	void UpdateBody(float Dt);
	void UpdateHands(float Dt);
	void UpdateFace(float Dt);
	/** Component space of the body mesh from world, and back. */
	FVector ToBody(const FVector& World) const;
	FVector ToBodyDir(const FVector& World) const;
	FVector ToWorld(const FVector& BodyPos) const;

	// ------------------------------------------------------------ tells (BackRoomPlayerTells.cpp)
	/** Something happened that this persona's tells may answer: they fire with their reliabilities. */
	void Provoke(EBackRoomTellMeaning Meaning, float Intensity);
	/** The moment the hole cards are seen. */
	void ReactToHole();
	void UpdateTells(float Dt);
	/** Face controls from the active tells and talking. */
	void TellFace(TFunctionRef<void(const TCHAR*, float)> Add, TFunctionRef<void(const TCHAR*, float)> Both);

	UPROPERTY()
	TObjectPtr<USceneComponent> Root;
	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> Body;
	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> Face;
	/** Hair, brows, lashes, beard, clothes copied from the MetaHuman Blueprint. */
	UPROPERTY()
	TArray<TObjectPtr<USceneComponent>> Wearables;
	/** The Blueprint they were copied from. */
	UPROPERTY()
	TObjectPtr<UClass> WornClass;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> ChairSeat;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> ChairBack;

	UPROPERTY(Transient)
	TMap<FName, float> TestCurves;

	FRandomStream Rng;
	float Time = 0.0f;
	// Breathing.
	float BreathPhase = 0.0f;
	float BreathHold = 0.0f;
	// Blinks.
	float NextBlink = 2.0f;
	float BlinkT = -1.0f;
	bool bDoubleBlink = false;
	int32 BurstBlinks = 0;
	// Gaze: the current target, the eyes' point, and the head lagging behind.
	FVector GazeTarget = FVector::ZeroVector;
	FVector EyeAt = FVector::ZeroVector;
	FVector HeadAt = FVector::ZeroVector;
	float GazeLeft = 0.0f;
	/** Something demanding attention (a bet, a card coming out), for a while. */
	FVector Attention = FVector::ZeroVector;
	float AttentionLeft = 0.0f;
	/** A gaze the tells hold (a stare, a glance at chips): overrides everything while it lasts. */
	FVector GazeHold = FVector::ZeroVector;
	float GazeHoldLeft = 0.0f;
	float HeadFollow = 0.75f;
	// Posture drift.
	float Lean = 0.3f;
	float LeanTarget = 0.3f;
	float NextShift = 8.0f;
	/** 0..1: how still the body is held (a bluffer's freeze). */
	float Stillness = 0.0f;
	float Tremble = 0.0f;
	float SighT = -1.0f;

	// ------------------------------------------------------------ hands
	struct FHandPose
	{
		FVector Pos = FVector::ZeroVector;
		FVector Palm = FVector(0.0, 0.0, -1.0);
		FVector Finger = FVector(0.0, 1.0, 0.0);
		float Curl = 0.35f;
		float Thumb = 0.2f;
		float Pinch = 0.0f;
	};
	struct FHandStep
	{
		FHandPose Pose;
		float Duration = 0.3f;
		/** Lift mid-move (cm), so the hand clears the felt and the chips. */
		float Arc = 0.0f;
		/** Go back to the resting hand instead of Pose. */
		bool bRest = false;
		TFunction<void()> OnArrive;
	};
	/** Something a hand holds: placed every frame at the hand (Offset in the hand's frame). */
	struct FHeld
	{
		TWeakObjectPtr<AActor> Thing;
		int32 Side = 1;
		FTransform Offset;
		/** Kept level (chips), hanging Drop cm below the offset point. */
		bool bUpright = false;
		double Drop = 0.0;
	};
	FHandPose RestPose(int32 Side) const;
	/** The hand placed so its fingertips touch Tip (world), fingers along Finger, palm along Palm (body space). */
	FHandPose Touch(const FVector& Tip, const FVector& Finger, const FVector& Palm, float Curl = 0.25f, float Pinch = 0.0f) const;
	void Queue(int32 Side, const FHandPose& Pose, float Duration, float Arc = 0.0f, TFunction<void()> OnArrive = nullptr);
	void QueueRest(int32 Side, float Duration);
	float QueuedTime(int32 Side) const;
	void Grab(AActor* Thing, int32 Side, const FTransform& Offset);
	void LetGo(AActor* Thing);
	void Sound(int32 Id, const FVector& At, float Volume = 1.0f) const
	{
		if (SoundHook)
		{
			SoundHook(Id, At, Volume);
		}
	}
	FTransform HandFrame(int32 Side) const;

	FHandPose HandNow[2];
	FHandPose StepFrom[2];
	TArray<FHandStep> Steps[2];
	float StepT[2] = {0.0f, 0.0f};
	TArray<FHeld> HeldThings;
	/** Idle hands: resting spots in body space, eased toward. */
	FVector HandGoal[2];
	float HandSwitch = 5.0f;
	int32 HandMode = 0;
	bool bHeroPeek = false;
	float HeroPeekT = 0.0f;
	/** The hole cards' lift (0..1), eased toward the goal the hands set. */
	float PeekNow = 0.0f;
	float PeekGoal = 0.0f;
	float PeekApplied = 0.0f;
	/** Chips on their way from the stack to the bet, in hand. */
	TWeakObjectPtr<ABackRoomChips> Carrying;
	FVector HeroLook = FVector::ZeroVector;
	/** The dealer's deck, in the left hand. */
	UPROPERTY(Transient)
	TObjectPtr<ABackRoomCard> Deck;

	// ------------------------------------------------------------ the hand being played
	float Strength = 0.5f;
	float PrevStrength = 0.5f;
	bool bInHand = false;
	bool bBluffing = false;
	bool bValue = false;
	bool bThinking = false;
	bool bHeroThinking = false;
	/** 0..1, eased: worry, thrill, and disappointment, which the mood is made of. */
	float Stress = 0.0f;
	float Thrill = 0.0f;
	float Gloom = 0.0f;
	float StressGoal = 0.0f;
	float ThrillGoal = 0.0f;
	float GloomGoal = 0.0f;

	// ------------------------------------------------------------ tells
	struct FActiveTell
	{
		EBackRoomTell Tell = EBackRoomTell::ChipGlance;
		float T = 0.0f;
		float Duration = 1.0f;
		float Intensity = 1.0f;
		/** Fired by the hand (Provoke), so it means something: reported to TellHook once seen. */
		bool bCounts = false;
		bool bHonest = false;
		EBackRoomTellMeaning Means = EBackRoomTellMeaning::Strong;
		float SeenMax = 0.0f;
		bool bReported = false;
	};
	TArray<FActiveTell> Active;
	bool IsActive(EBackRoomTell Tell) const;
	/** Ends sustained tells (a freeze, a stare) when the moment has passed. */
	void EndTell(EBackRoomTell Tell);

	// A micro-expression flashing across the face (0..1 envelope) and which one.
	float MicroT = -1.0f;
	int32 MicroKind = 0;
	float NextMicro = 6.0f;

	// Talking: lines to show, and the jaw moving while one is said.
	TArray<FString> Lines;
	float TalkLeft = 0.0f;
	float NextChat = 20.0f;
	// Being studied by the hero.
	float Studied = 0.0f;
	float ExternalTremble = 0.0f;
	float StudiedFor = 0.0f;
	float NoticeCooldown = 0.0f;
	/** 0..1 while reacting to being stared at, -1 otherwise. */
	float NoticeT = -1.0f;
};
