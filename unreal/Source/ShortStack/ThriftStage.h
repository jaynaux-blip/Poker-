#pragma once

#include "CoreMinimal.h"
#include "ShopStage.h"

#include "ThriftStage.generated.h"

class UInstancedStaticMeshComponent;

/**
 * Second Chance Thrift (design spec 5.2): 220 Market St (M-S2), a former five-and-dime run by St. Agnes Outreach. Its
 * room (spec 2.4: the opening X -2440..-3660 on the Y 4950 plane, the room X -2460..-3640, Y 3650..4920, the drop ceiling
 * at 3.30) from ns_thrift's meshes where they're imported and engine shapes where they aren't, laid out as ns_thrift's
 * review plan has it: the counter and the case by the door, the hat wall and the shoe wall, two round racks by the
 * windows, the tees table, the outerwear and pants racks, the fitting booth with its three-way mirror in the back corner.
 *
 * The racks carry this week's stock (wardrobe::ThriftStock, restocked Thursdays: the garments instanced on the racks'
 * hanger points, each tinted its color), only of the kinds the hero can wear (HostCaps.GarmentKinds, from the game mode).
 * Uses: thrift-door, thrift-rack-tops, -outer, -pants, thrift-shoes, thrift-hats, thrift-booth (Try on), thrift-register
 * (Pay), thrift-donate. Stations: thrift-register, thrift-fold, thrift-rack. Shots: thrift-fitting (with its mark at the
 * mirror; the curtain draws as the hero changes), thrift-rack.
 */
UCLASS()
class SHORTSTACK_API AThriftStage : public AShopStage
{
	GENERATED_BODY()

public:
	AThriftStage();

	static void AppendPreload(TArray<FString>& Paths);
	virtual void OnShot(FName Shot, bool bActive) override;
	virtual void SyncSession(const ss::Session& S) override;
	/** The garment kinds the hero can be shown in (HostCaps.GarmentKinds: a bit per wardrobe::Cut); the racks stock only those. */
	void SetGarmentKinds(uint64 Kinds);

protected:
	virtual void BuildInterior() override;
	virtual void Animate(float Dt) override;

private:
	/** The racks' garments from this week's stock (again when the week or the kinds change). */
	void Restock(int32 Week, uint64 Seed);

	/** One instanced mesh per hung or folded garment shape. */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UInstancedStaticMeshComponent>> Hung;
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> Curtain;
	float CurtainNow = 0.25f;
	float CurtainWant = 0.25f;
	float CurtainHold = 0.0f;
	uint64 GarmentKinds = ~0ull;
	int32 StockedWeek = -1;
	uint64 StockedKinds = 0;
	int32 HoursDecal = INDEX_NONE;
	FString HoursShown;
};
