#pragma once

#include "CoreMinimal.h"
#include "ShopStage.h"

#include "PawnStage.generated.h"

/**
 * Northside Pawn & Loan (design spec 5.8): 232 Market St (M-S3), between the thrift and the gym: three gold balls on a
 * bracket, a blue tube round barred windows, a buzzer door. Its room (spec 2.4 and ns_pawn's plan: the opening X
 * -3760..-4440 on the Y 4950 plane, the room X -3740..-4460 by Y 4920..3600, the pressed tin at 3.60) from ns_pawn's meshes
 * where they're imported and engine shapes where they aren't: the sales floor with the guitar wall and the steel shelves,
 * the glass-case counter, the loan cage with its window, the safe and the bagged pledges, the wall of clocks (two keep the
 * world's time), the ceiling fan turning.
 *
 * The stock is the session's (Session::PawnShelf): the used gear for sale stands at the shelves' G sockets on the gear meshes
 * the apartment draws (a wide piece takes a pair), the curios and the forfeited mementos at the curio case's C sockets, each
 * with its price tag (V, core's: the shelf never promises what the register won't ring up), re-placed when the shelf
 * changes. Its FillCaps says which gear its shelves can stand up (HostCaps.GearKinds).
 *
 * Uses: pawn-door (Buzz), pawn-counter (SELL), pawn-cage (TICKETS), pawn-cases (BUY, curios), pawn-electronics (BUY, gear),
 * pawn-guitars (Augie's story). Stations: pawn-counter, pawn-cage, pawn-bench, pawn-door. Shots: pawn-counter, pawn-case,
 * pawn-cage.
 */
UCLASS()
class SHORTSTACK_API APawnStage : public AShopStage
{
	GENERATED_BODY()

public:
	APawnStage();

	static void AppendPreload(TArray<FString>& Paths);
	virtual void SyncSession(const ss::Session& S) override;
	virtual void FillCaps(ss::ui::HostCaps& Caps) const override;
	/** The gear mesh a gear::Catalog item stands on the shelf as (and how many sockets it takes, 0: none fits), or null. */
	static const TCHAR* GearMesh(const FString& GearId, int32& OutSockets);
	/** What stands on the shelves and in the case now (ss.Pawn.List). */
	FString DescribeStock() const;

protected:
	virtual void BuildInterior() override;

private:
	/** The stock's parts (the meshes and their tags), dropped and stood again when the shelf changes. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UActorComponent>> Stock;
	FString StockKey;
	TArray<FString> StockLines;
	int32 HoursDecal = INDEX_NONE;
	FString HoursShown;
};
