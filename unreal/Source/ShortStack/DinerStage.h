#pragma once

#include "CoreMinimal.h"
#include "ShopStage.h"

#include "DinerStage.generated.h"

/**
 * Ruby's (design spec 5.4): the 24-hour diner on the northeast corner of Fifth & Market, a curved glass corner under red
 * neon. Its room (spec 2.4: the quarter-round corner (center (-200, 7150), R 200) plus the Fifth face (X 0 plane, Y
 * 7150..8000) and the Market face (Y 6950 plane, X -200..-1500); the room X -40..-1460, Y 6990..7960, the kitchen behind
 * the pass visible but not walkable) from ns_diner's meshes where they're imported and engine shapes where they aren't: the
 * double door in the corner's curve, the L counter with nine stools, five booths down the Market windows, the backbar with
 * the pie case (its carousel turning), the pass with its heat lamps, the menu board (core's menu), both neons (24 h).
 *
 * Uses: diner-door, diner-stool1..9 and diner-booth1..5 (Sit: the hero sits, the menu opens; the camera's diner-counter or
 * diner-booth). Stations: diner-counter (the waitress), diner-pass (the cook), diner-stool1..9, diner-booth1a..5b. The cook
 * at the pass is a baked figure (Cast/Built/Crowd/SM_Crowd_Cook, back to the room) where the pose booth has made one.
 */
UCLASS()
class SHORTSTACK_API ADinerStage : public AShopStage
{
	GENERATED_BODY()

public:
	ADinerStage();

	static void AppendPreload(TArray<FString>& Paths);
	virtual void SyncSession(const ss::Session& S) override;

protected:
	virtual void BuildInterior() override;

private:
	int32 MenuBoard = INDEX_NONE;
	FString MenuShown;
};
