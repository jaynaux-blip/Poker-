// Blueprint-facing types for the SHORT STACK engine. The engine itself (ShortStack/*.h)
// is plain C++; these structs are the copies Blueprints and UMG read.
#pragma once

#include "CoreMinimal.h"

#include "ShortStackTypes.generated.h"

/** Player pool a tournament draws its opponents from (softer to tougher). */
UENUM(BlueprintType)
enum class EShortStackField : uint8
{
	Freeroll,
	Micro,
	Low,
	High,
};

UENUM(BlueprintType)
enum class EShortStackEventType : uint8
{
	Bust,
	Level,
	Moved,
	TableBroken,
	HandForHand,
	Bubble,
	FinalTable,
	Finished,
};

USTRUCT(BlueprintType)
struct SHORTSTACKCORE_API FShortStackTournamentSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Short Stack")
	FString Name = TEXT("$1.10 Nightly Turbo");

	/** Total paid per entry in cents, fee included. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Short Stack")
	int64 BuyInCents = 110;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Short Stack")
	int64 FeeCents = 10;

	/** Prize pool floor in cents (the whole pool for a freeroll). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Short Stack")
	int64 GuaranteeCents = 10000;

	/** Field size, hero included. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Short Stack", meta = (ClampMin = "2"))
	int32 Entrants = 180;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Short Stack", meta = (ClampMin = "1"))
	int64 StartingStack = 10000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Short Stack", meta = (ClampMin = "0.5"))
	double LevelMinutes = 5.0;

	/** Game-clock seconds one round (a hand at every table) takes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Short Stack", meta = (ClampMin = "1"))
	double SecondsPerHand = 42.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Short Stack")
	EShortStackField Field = EShortStackField::Micro;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Short Stack", meta = (ClampMin = "2", ClampMax = "10"))
	int32 TableSize = 9;

	/** Game clock at the start, in minutes after midnight (1170 = 7:30 pm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Short Stack")
	double StartClockMinutes = 1170.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Short Stack")
	FString HeroName = TEXT("Hero");

	/** Optional named rival entered in the field (a Crusher), e.g. gh0stfold. Empty for none. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Short Stack")
	FString RivalName;

	/** Same seed, same tournament, card for card. Empty picks a random seed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Short Stack")
	FString Seed;
};

USTRUCT(BlueprintType)
struct SHORTSTACKCORE_API FShortStackStanding
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Short Stack")
	int32 Rank = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Short Stack")
	FString PlayerId;

	UPROPERTY(BlueprintReadOnly, Category = "Short Stack")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Short Stack")
	int64 Stack = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Short Stack")
	int32 TableId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Short Stack")
	bool bIsHero = false;
};

USTRUCT(BlueprintType)
struct SHORTSTACKCORE_API FShortStackEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Short Stack")
	EShortStackEventType Type = EShortStackEventType::Bust;

	/** Bust and Moved: the player. Bubble: the bubble boy. Finished: the winner. */
	UPROPERTY(BlueprintReadOnly, Category = "Short Stack")
	FString PlayerName;

	UPROPERTY(BlueprintReadOnly, Category = "Short Stack")
	bool bIsHero = false;

	/** Bust: finishing place and prize. */
	UPROPERTY(BlueprintReadOnly, Category = "Short Stack")
	int32 Place = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Short Stack")
	int64 PrizeCents = 0;

	/** Bust and TableBroken: the table. Moved: the table moved to. */
	UPROPERTY(BlueprintReadOnly, Category = "Short Stack")
	int32 TableId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Short Stack")
	int32 FromTableId = 0;

	/** Level: the new level (1-based) and its blinds. */
	UPROPERTY(BlueprintReadOnly, Category = "Short Stack")
	int32 LevelNumber = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Short Stack")
	int64 SmallBlind = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Short Stack")
	int64 BigBlind = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Short Stack")
	int64 Ante = 0;
};
