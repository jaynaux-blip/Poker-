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

/** One line of table talk, for the subtitles. */
struct FBackRoomLine
{
	FString Speaker;
	FString Text;
	float At = 0.0f;
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
};
