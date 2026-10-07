#pragma once

#include "CoreMinimal.h"
#include "ShopStage.h"

#include "GymStage.generated.h"

/**
 * IronHouse Gym (design spec 5.3): 246 Market St (M-S4), a two-storey brick warehouse halfway down the block. Its room (spec
 * 2.4: the opening X -4560..-6540 on the Y 4950 plane, the room X -4560..-6540, Y 3450..4920, the roof deck at 5.20 over
 * exposed trusses) from ns_gym's meshes where they're imported and engine shapes where they aren't, laid out as ns_gym's
 * review plan has it: the desk and cooler inside the door, five treadmills and two bikes along the windows (the 2 AM runner
 * shows from the street), the dumbbell rack and benches at the mirror wall, two racks with their plate trees, three heavy
 * bags on the east wall.
 *
 * The door is glass on a fob (spec 5.0): a member taps it at any hour, anyone else is buzzed in while Rosa's at the desk.
 * Uses: gym-door, gym-desk (staffed only), gym-treadmill1..5, gym-bag, gym-weights, gym-cooler. Stations: gym-desk,
 * gym-floor, gym-treadmill1..5, gym-bag, gym-bench. Shots: gym-floor (the bag, I4), gym-run, gym-lift. A workout puts the
 * hero on the machine (ServiceMark) and the belt pans, the bag swings, for its skip.
 */
UCLASS()
class SHORTSTACK_API AGymStage : public AShopStage
{
	GENERATED_BODY()

public:
	AGymStage();

	static void AppendPreload(TArray<FString>& Paths);
	virtual TArray<TPair<FName, float>> Cinematic(FName Service) const override;
	virtual bool ServiceMark(FName Service, FTransform& OutFeet, float& OutSeat, FName& OutBeat) const override;
	virtual void OnService(FName Service, bool bRunning) override;
	virtual void SyncSession(const ss::Session& S) override;
	/** Someone running on a treadmill (0..4: the window row, west to east): its belt pans (people-ue's regulars, the hero's workout). */
	void SetTreadmillRunning(int32 Index, bool bRunning);
	/** A blow on a heavy bag (0..2, north to south), Strength 0..1: it swings and settles. */
	void HitBag(int32 Index, float Strength);

protected:
	virtual void BuildInterior() override;
	virtual void OnHour() override;
	virtual void Animate(float Dt) override;
	virtual FString ClosedPrompt() const override;

private:
	/** Each treadmill's belt: whether it's running and how far it has run (the belt material's Pan). */
	bool Running[5] = {false, false, false, false, false};
	float BeltPan[5] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
	/** Each bag's pivot and its swing. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<USceneComponent>> Bags;
	float BagSwing[3] = {0.0f, 0.0f, 0.0f};
	float BagVel[3] = {0.0f, 0.0f, 0.0f};
	/** The hero working the bag (a blow every so often while the skip runs). */
	bool bBagWorkout = false;
	float NextBlow = 0.0f;
	/** The BACK AT 4 card on the desk's tablet while nobody's at it (core's staffed hours). */
	int32 DeskSign = INDEX_NONE;
	FString DeskShown;
	FString StaffedAgain;
};
