#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "BackRoomChips.generated.h"

class UInstancedStaticMeshComponent;

/** How a pile of chips is laid out. */
UENUM()
enum class EBackRoomChipStyle : uint8
{
	/** A player's stack: tidy columns by denomination, big chips at the back. */
	Stack,
	/** Chips slid out as a bet: one or two short columns, a little askew. */
	Bet,
	/** The pot: chips splashed together in short, leaning piles. */
	Pot,
};

/**
 * A pile of the Spin Cycle Club's clay chips ($1 white, $5 red, $25 green, $100 black; SM_Chip_<n> from
 * art/blender/assets/chips.py), drawn as one instanced mesh per denomination.
 *
 * A pile holds an amount and lays its chips out for it; it slides as a whole (a bet pushed out, the
 * pot pushed to the winner), and a hand that carries it places it every frame.
 */
UCLASS()
class SHORTSTACK_API ABackRoomChips : public AActor
{
	GENERATED_BODY()

public:
	ABackRoomChips();

	virtual void Tick(float DeltaSeconds) override;

	void SetStyle(EBackRoomChipStyle InStyle, int32 InSeed);
	void SetAmount(int64 InAmount);
	int64 GetAmount() const { return Amount; }

	/** Slides the pile over the felt to Target (a point on the felt) in Duration. */
	void SlideTo(const FVector& Target, float Duration, float Arc = 0.0f);
	/** Stops sliding where it is (a hand has picked the pile up and places it each frame). */
	void Stop() { MoveT = 1.0f; }
	bool IsMoving() const { return MoveT < 1.0f; }
	/** The height of the tallest column (cm). */
	double GetHeight() const { return Height; }

	/** The top of the pile (for a hand reaching to take chips from it). */
	FVector GetTop() const;

	/** Chips worth Amount as the club's denominations (counts of 100, 25, 5 and 1), the way players stack them. */
	static void Break(int64 Amount, EBackRoomChipStyle Style, int32 Counts[4]);

	static constexpr double Radius = 1.95;
	static constexpr double Thickness = 0.33;

private:
	void Layout();

	UPROPERTY()
	TObjectPtr<USceneComponent> Root;
	UPROPERTY()
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Denoms;

	EBackRoomChipStyle Style = EBackRoomChipStyle::Stack;
	int32 Seed = 1;
	int64 Amount = 0;
	double Height = 0.0;

	FVector From = FVector::ZeroVector;
	FVector To = FVector::ZeroVector;
	float MoveT = 1.0f;
	float MoveDuration = 0.4f;
	float MoveArc = 0.0f;
};
