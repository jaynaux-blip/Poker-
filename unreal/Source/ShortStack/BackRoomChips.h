#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShortStack/Game/ChipSet.h"

#include <vector>

#include "BackRoomChips.generated.h"

class UInstancedStaticMeshComponent;
class UMeshComponent;
class UStaticMesh;

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
 * A pile of chips, drawn as one instanced mesh per denomination: the Wash & Fold's clay at Dee's game ($1 white, $5 red,
 * $25 green, $100 black; SM_Chip_<n> from art/blender/assets/chips.py), or the Embercrest's tournament set (100 black,
 * 500 purple, 1,000 yellow, 5,000 orange, 25,000 ash with the ember on its edge; SM_Chip_T100 ... SM_Chip_T25K) laid out
 * the way the room stacks them (ss::chipset::Lay: columns of at most 20, one denomination each, the biggest chips in
 * front; next_segment_spec 6.2 B).
 *
 * A pile holds an amount and lays its chips out for it; it slides as a whole (a bet pushed out, the pot pushed to the
 * winner), and a hand that carries it places it every frame.
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

	/** What one chip of the smallest denomination stands for: 1 at Dee's game, 100 in a tournament (the old scale; the
	 *  Embercrest's piles lay out in its own set, UseChips). */
	static int64 ChipUnit;

	/**
	 * The chips every pile lays out in: Dee's four (bEmbercrest false: today's mix, exactly), or the Embercrest's set with
	 * the chips in play now (ss::chipset::InPlay at the level; a colour-up races the small ones off). Every pile lays itself
	 * out again when this changes.
	 */
	static void UseChips(bool bEmbercrest, const std::vector<ss::chipset::Denom>& InPlay);
	static bool IsTournamentSet() { return bTournamentSet; }
	/** The chips in play now (the Embercrest's), smallest first; empty at Dee's game. */
	static const std::vector<ss::chipset::Denom>& ChipsInPlay() { return InPlayNow; }
	/** The smallest chip in play: $1 at Dee's game, 100 at the Embercrest's start (500 once the black ones race off). */
	static int64 SmallestChip();
	/**
	 * The mesh for one of the room's chips: the art's (SM_Chip_T100 ...), or, until the tournament set imports, a block-out
	 * of the club's clay that TintBlockout colours (bOutBlockout). Loaded once.
	 */
	static UStaticMesh* MeshFor(const ss::chipset::Denom& D, bool& bOutBlockout);
	/** A block-out chip's colour on a mesh (the imported material's base colour factor), for every slot. */
	static void TintBlockout(UMeshComponent* Mesh, uint32 Body);

	static constexpr double Radius = 1.95;
	static constexpr double Thickness = 0.33;

private:
	void Layout();
	/** The Embercrest's layout (ss::chipset::Lay) into the five meshes. */
	void LayoutTournament();
	/** Each mesh holds its denomination's chip (Dee's four, or the room's five). */
	void AssignMeshes();

	UPROPERTY()
	TObjectPtr<USceneComponent> Root;
	UPROPERTY()
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Denoms;

	EBackRoomChipStyle Style = EBackRoomChipStyle::Stack;
	int32 Seed = 1;
	int64 Amount = 0;
	double Height = 0.0;
	/** The chips this pile was laid out in (UseChips' revision) and which set its meshes hold. */
	int32 LaidRevision = -1;
	int32 MeshesFor = -1;

	FVector From = FVector::ZeroVector;
	FVector To = FVector::ZeroVector;
	float MoveT = 1.0f;
	float MoveDuration = 0.4f;
	float MoveArc = 0.0f;

	static bool bTournamentSet;
	static std::vector<ss::chipset::Denom> InPlayNow;
	static int32 SetRevision;
};
