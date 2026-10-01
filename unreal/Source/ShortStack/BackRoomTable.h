#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShortStack/AI/Profiles.h"
#include "ShortStack/Hand.h"
#include "ShortStack/Rng.h"

#include "BackRoomTable.generated.h"

class ABackRoomCard;
class ABackRoomChips;
class ABackRoomPlayer;
class FBackRoomAmbience;
class UNightOneAudio;

/** One line of table talk, for the subtitles (or a read: what your eye caught, whispered). */
struct FBackRoomLine
{
	FString Speaker;
	FString Text;
	float At = 0.0f;
	bool bRead = false;
};

/** What the table tells its host (the game mode) between hands. */
enum class EBackRoomTableNote : uint8
{
	/** A hand is over (stacks settled): a good moment to save. */
	HandEnded,
	/** The hero is out of chips; the table holds until Reload or the hero leaves. */
	HeroBusted,
	/** The hero racked up (after the hand they asked to leave in). */
	HeroLeft,
};

/**
 * Your own tells. A heart-rate model fed by the pressure of the hand: the pot against your stack,
 * the price of a call, a bluff you are running, a stare across the table, the last big loss. Above
 * about 95 bpm you hear your heart and your hands start to shake when they move chips, which the
 * sharper regulars read.
 */
struct FBackRoomComposure
{
	float Bpm = 68.0f;
	float Target = 68.0f;
	/** 0..1 set while a bluff is out there (the hand's last aggression was yours, without the goods). */
	float Bluff = 0.0f;
	/** 0..1 after a painful loss, fading over minutes. */
	float Tilt = 0.0f;
	/** Holding Shift: slow breaths. */
	bool bSteadying = false;
	/** How much of the pressure reaches the heart (falls as the career goes on: you get used to it). */
	float Sensitivity = 1.0f;
	/** 0..1: how much the hands shake (what others can see). */
	float Shake() const { return FMath::Clamp((Bpm - 95.0f) / 55.0f, 0.0f, 1.0f); }
};

/** What the hero can do now (for the HUD). */
struct FBackRoomPrompt
{
	bool bYourTurn = false;
	bool bCanCheck = false;
	int64 ToCall = 0;
	bool bCanRaise = false;
	bool bIsBet = false;
	int64 RaiseTo = 0;
	int64 MinRaiseTo = 0;
	int64 MaxRaiseTo = 0;
	int64 Pot = 0;
	int64 Stack = 0;
	int64 BigBlind = 2;
	/** The hero's cards once peeked at (ss::Card), else empty. */
	TArray<int32> Known;
	int32 HandNumber = 0;
};

/**
 * The Tuesday game at the Spin Cycle Club: $1/$2 No-Limit Hold'em, Dee dealing.
 *
 * Plays real hands with the SHORT STACK engine (ss::Hand: blinds, betting rules, side pots,
 * showdowns) and the engine's bots (ss::Decide, each opponent with an archetype's profile), and acts
 * them out at the table: Dee pitches the cards and runs the pot, the players knock, slide chips out,
 * fold and show, and every opponent's body reacts to what they really hold (ABackRoomPlayer's tells).
 * The hero plays through ABackRoomPawn's input.
 */
UCLASS()
class SHORTSTACK_API ABackRoomTable : public AActor
{
	GENERATED_BODY()

public:
	ABackRoomTable();
	virtual ~ABackRoomTable() override;

	virtual void Tick(float DeltaSeconds) override;

	/** Seats a player: TableSeat 0..7 (0 the hero, 4 the dealer's), the engine archetype for a bot, and a buy-in. */
	void AddPlayer(ABackRoomPlayer* Player, int32 TableSeat, ss::Archetype Archetype, int64 BuyIn);
	void SetDealer(ABackRoomPlayer* InDealer);
	/** Starts dealing (after a pause for everyone to settle). */
	void Begin(float Delay = 3.0f);
	/** The room tone (dryers, the fluorescent hum), before the game starts. */
	void StartRoomTone();

	// ------------------------------------------------------------ the hero's input
	FBackRoomPrompt GetPrompt() const;
	UFUNCTION(BlueprintCallable, Category = "Short Stack")
	void HeroFold();
	UFUNCTION(BlueprintCallable, Category = "Short Stack")
	void HeroCheckCall();
	/** Bets or raises to the amount shown in the prompt. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack")
	void HeroRaise();
	UFUNCTION(BlueprintCallable, Category = "Short Stack")
	void HeroAllIn();
	/** Moves the raise amount by Steps (big blinds early, then bigger jumps). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack")
	void HeroAdjustRaise(int32 Steps);
	/** Hand number, street, who acts, pot and stacks (for logs and tests). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack")
	FString Describe() const;
	/** The hero looked at their cards. */
	void HeroPeeked();

	/** Table talk said since the last call. */
	TArray<FBackRoomLine> TakeLines();
	/** Everyone seated, for the camera's study of faces. */
	const TArray<ABackRoomPlayer*>& GetOpponents() const { return Opponents; }
	ABackRoomPlayer* GetHeroPlayer() const;

	// ------------------------------------------------------------ the night (ABackRoomGameMode)
	/** Called between hands and when the hero busts or leaves. */
	TFunction<void(EBackRoomTableNote)> OnNote;
	/** Rack up after the current hand (bLastHand: Dee is closing the game for everyone). */
	void RequestLeave(bool bLastHand = false);
	bool IsLeaving() const { return bLeaveRequested; }
	bool IsHolding() const { return bHolding; }
	/** Busted: buys back in for Chips (dollars) and deals on. */
	void HeroReload(int64 Chips);
	/** The hero's chips now (behind, plus what is in front of them this hand). */
	int64 GetHeroStack() const;
	int32 GetHandNumber() const { return HandNumber; }
	/** In a career the hero's busts are the host's call; in practice they reload on the house. */
	bool bHeroAutoReload = true;
	/** First night at Dee's: she explains everything. */
	bool bFirstVisit = true;

	/** The hero's heart (read by the pawn for sound and sight, by the opponents through the hands). */
	FBackRoomComposure Composure;
	void SetSteadying(bool bOn) { Composure.bSteadying = bOn; }

	/** The read book: "Player/Tell" -> sightings confirmed at showdown (2 or more: learned). */
	TMap<FString, int32> Reads;
	int32 LearnedThisNight = 0;
	/** A tell just played across the table (from ABackRoomPlayer); Studied is how hard you were looking. */
	void OnTellSeen(ABackRoomPlayer* Player, uint8 Tell, uint8 Means, bool bHonest, float Studied);
	/** "Player/Tell" for the read book. */
	static FString ReadKey(const ABackRoomPlayer* Player, uint8 Tell);
	/** "the glance at the chips", for whispers and the summary. */
	static FString TellPhrase(uint8 Tell);

	/** Dee says something (the night's host lines). */
	void DealerLine(const FString& Line) { DealerSays(Line); }
	UNightOneAudio* GetAudio() const { return Audio; }

private:
	struct FSeat
	{
		int32 TableSeat = 0;
		TObjectPtr<ABackRoomPlayer> Player;
		bool bHero = false;
		FString Id;
		int64 Stack = 0;
		int64 BuyIn = 0;
		int64 StartStack = 0;
		ss::Profile Profile;
		TObjectPtr<ABackRoomChips> StackPile;
		TObjectPtr<ABackRoomChips> BetPile;
		TArray<TObjectPtr<ABackRoomCard>> Hole;
		bool bDealt = false;
		bool bShown = false;
	};

	FSeat* SeatAt(int32 TableSeat);
	const FSeat* SeatAt(int32 TableSeat) const;
	FSeat* HeroSeat();
	const FSeat* HeroSeat() const;
	/** Where things go on the felt in front of a seat. */
	FTransform CardSpot(int32 TableSeat, int32 Index) const;
	FVector StackSpot(int32 TableSeat) const;
	FVector BetSpot(int32 TableSeat) const;
	FTransform BoardSpot(int32 Index) const;
	FVector PotSpot() const;
	FVector MuckSpot() const;

	void StartHand();
	/** Acts out one engine event; returns how long to wait before the next. */
	float Consume(const ss::HandEvent& Event);
	float DealHoles(const TArray<int32>& Order);
	float DealStreet(const TArray<int32>& Cards);
	float GatherBets();
	void EndHand();
	void OpenHeroTurn();
	void HeroAct(int32 Kind, double To);
	/** Every player's equity now against random hands for the opponents still in (what they "feel"). */
	void UpdateStrengths(bool bBoardChanged);
	ABackRoomCard* NewCard(int32 Card);
	ABackRoomChips* NewPile(const FVector& At, const FRotator& Facing, int32 Seed, uint8 Style);
	void DealerSays(const FString& Line);

	/** The gestures' sounds, and the room tone. */
	UPROPERTY(VisibleAnywhere, Category = "Short Stack")
	TObjectPtr<UNightOneAudio> Audio;
	TSharedPtr<FBackRoomAmbience> RoomTone;
	void HookSounds(ABackRoomPlayer* Player);

	UPROPERTY(Transient)
	TObjectPtr<ABackRoomPlayer> Dealer;
	UPROPERTY(Transient)
	TArray<TObjectPtr<ABackRoomPlayer>> Opponents;
	UPROPERTY(Transient)
	TObjectPtr<ABackRoomChips> Pot;
	UPROPERTY(Transient)
	TArray<TObjectPtr<ABackRoomCard>> Board;
	/** Every card on the table this hand (collected at the end). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ABackRoomCard>> Cards;

	TArray<FSeat> Seats;
	TUniquePtr<ss::Hand> Hand;
	TUniquePtr<ss::Rng> Rng;
	FRandomStream Fx;

	float Time = 0.0f;
	float Wait = 0.0f;
	bool bRunning = false;
	size_t Cursor = 0;
	int32 HandNumber = 0;
	int32 Button = -1;
	bool bHandDone = false;

	// A bot deciding.
	bool bBotPending = false;
	float BotAt = 0.0f;
	int32 BotSeat = -1;
	int32 BotKind = 0;
	double BotTo = 0.0;
	bool bBotBluff = false;

	// The hero deciding.
	bool bHeroTurn = false;
	int64 HeroRaiseTo = 0;
	bool bHeroPeeked = false;

	TArray<FBackRoomLine> Lines;
	int32 TipsGiven = 0;

	// The night.
	bool bLeaveRequested = false;
	bool bLastHand = false;
	bool bHolding = false;
	/** The hero's chips when the hand began (for the pressure of a pot, and the loss that stings). */
	int64 HeroStartOfHand = 0;
	/** How shaken the hero looked when they last bet or raised this hand (what opponents read). */
	float HeroShakeAtBet = 0.0f;
	bool bHeroAggressedThisStreet = false;
	void UpdateComposure(float Dt);
	/** An opponent facing the hero's bet: do the hands give it away (and do they read it right)? */
	void ReadHero(const FSeat& Reader, int32& Kind, double& To, double Equity);
	/** Tells seen this hand while studying their player, waiting for the cards to say if they meant it. */
	struct FSighting
	{
		TWeakObjectPtr<ABackRoomPlayer> Player;
		FString Key;
		uint8 Tell = 0;
		uint8 Means = 0;
		bool bHonest = false;
	};
	/** Keys already whispered this hand (one whisper per tell per hand). */
	TSet<FString> WhisperedThisHand;
	bool bLeftNoted = false;
	bool bBustNoted = false;
	/** Equity of a seat's hole cards against random hands for everyone still in, as a player feels it. */
	float FeltEquity(int32 TableSeat);
	TArray<FSighting> Sightings;
	void ConfirmSightings(const ABackRoomPlayer* Shown);
	void Whisper(const FString& Text);
};
