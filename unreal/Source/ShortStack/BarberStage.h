#pragma once

#include "CoreMinimal.h"
#include "ShopStage.h"

#include "BarberStage.generated.h"

/**
 * Fifth St. Barbers (design spec 5.1): the walk-up's north bay at 1818 Fifth St., Lonnie Pruitt's shop. Its room behind the
 * walk-up's opening (spec 2.4: the opening X 0 plane Y 3112..3618, the room X -35..-900, Y 3125..3610, the pressed tin at
 * 3.30), from ns_barber's meshes where they're imported and engine shapes where they aren't: the storefront with its door
 * (a swing door with the bell on its spring), the grille, the pole turning while it's open, the neon OPEN, the two chairs
 * facing the backbar's mirrors, the waiting chairs, the wall with its TV, price board (core's prices) and clock.
 *
 * Uses: barber-door; barber-chair1 ("Get a cut": the hero sits, the Style Book opens on barber-mirror); barber-bench (wait
 * on the bench). Stations: barber-chair1 and -chair2 (the barber's ring at each chair's head), barber-bench1..4,
 * barber-door (a smoke at closing). Shots: barber-mirror, barber-cut1..3, barber-reveal. The mirror goes live (spec
 * 3.4) while one of those shots is up.
 *
 * The Style Book's preload (spec 5.1, 7.5.4): while the shop is awake the hero face's whole groom set is held in memory
 * (AShortStackCharacter::PreloadGroomSet, loaded asynchronously as it wakes), so no key press in the book loads a groom;
 * asleep, or gone, it lets go.
 */
UCLASS()
class SHORTSTACK_API ABarberStage : public AShopStage
{
	GENERATED_BODY()

public:
	ABarberStage();

	static void AppendPreload(TArray<FString>& Paths);
	virtual TArray<TPair<FName, float>> Cinematic(FName Service) const override;
	virtual void OnShot(FName Shot, bool bActive) override;
	virtual void SyncSession(const ss::Session& S) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

protected:
	virtual void BuildInterior() override;
	virtual void OnHour() override;
	virtual void OnAwake() override;
	virtual void Animate(float Dt) override;

private:
	/** The hero's groom set held (awake) or let go (asleep); retried until there's a hero to ask. */
	void HoldGrooms(bool bHold);
	bool bGroomsHeld = false;
	/** The pole's stripes (their phase, turning while the shop's open) and its lit globe. */
	float PolePhase = 0.0f;
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> PoleSpin;
	/** The bell on its spring over the door: its swing since the door last opened. */
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> Bell;
	float BellAge = 10.0f;
	int32 PriceBoard = INDEX_NONE;
	int32 HoursDecal = INDEX_NONE;
	FString PricesShown;
	FString HoursShown;
};
