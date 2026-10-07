#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StreetAudio.h"
#include "StreetNs.h"

#include "StreetTraffic.generated.h"

class AStreetStage;
class UMaterialInstanceDynamic;
class USpotLightComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * The cars passing on the cross streets (world realism spec 3.6's cross-street pass, P0; 2.7's lanes and off-set ends):
 * one now and then down each of Hollis's, Wexler's and Cardinal's lanes, keeping right, every 6-20 s by day and 20-60 s
 * at night, at 30-40 km/h, coming on only at the lanes' ends 60 m either side of the T where nobody's looking, and going
 * off there (or, if the camera would see it go, on into the haze until it wouldn't, short of the fronts across the
 * street's end). No signals, queues or yielding: nobody on foot can reach a crossing of these streets, so they just read
 * as streets the hero doesn't step into (spec 2.9). The car kit (SM_NS_<Car> with its four _Wheels turning, its
 * _GlowLamps over the baked lenses in M_NS_GlowAdd, its plates from core), each in a paint of the street's palette;
 * headlights at night (two spots each, MegaLights); their sound for the street's positioned voices. While a skip runs they
 * hold where they are (spec 2.5).
 *
 * Fifth's and Market's traffic (signals, queues, yielding) and the 14 are P1: this class grows into them.
 * Built at play (BeginPlay, or Begin from the street); one per street. Console: ss.Traffic.Spawn [lane], ss.Traffic.Clear.
 */
UCLASS()
class SHORTSTACK_API AStreetTraffic : public AActor
{
	GENERATED_BODY()

public:
	AStreetTraffic();

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/** The traffic for Street (one already in the world, or a new one at its frame), begun. */
	static AStreetTraffic* FindOrSpawn(UWorld* World, AStreetStage* Street);
	/** Its lanes (the cross streets' keep-right lanes, Cardinal's only with Market at its new width) and its cars, parked out of sight. */
	void Begin(AStreetStage* InStreet);
	/** The hour's lamps (night: headlights on), the rain (their spray), whether a skip holds the street still. */
	void SetLook(const Ns::FGlowLevels& InLevels, float InRain, bool bInHold);

	/** ss.Traffic.Spawn: a car onto Lane now (-1: whichever lane is due), whatever the camera sees. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString SpawnNow(int32 Lane);
	/** ss.Traffic.Clear: every car off the street. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString ClearAll();
	/** One line: cars moving, passes so far, the lanes. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	FString Describe() const;
	/** The moving cars near Camera as positioned voices (their engine, tyres and spray). */
	void AppendEmitters(TArray<FStreetEmitter>& Out, const FVector& Camera, float Rain) const;

	static void AppendPreload(TArray<FString>& Paths);

private:
	/** A lane: from its off-set end to the other (stage cm, on the road's crown), the way cars face on it, which street. */
	struct FLane
	{
		FVector From = FVector::ZeroVector;
		FVector To = FVector::ZeroVector;
		float Yaw = 0.0f;
		int32 Street = 0;
		const TCHAR* Name = TEXT("");
		/** How far past To a car may run on while the camera would see it go (its ground runs on into the haze, to the fronts across its end). */
		double Beyond = 0.0;
	};
	/** A car of the pool: hidden off the street until it's sent down a lane. */
	struct FCar
	{
		TWeakObjectPtr<USceneComponent> Root;
		TArray<TWeakObjectPtr<USceneComponent>> Wheels;
		TArray<float> WheelSides;
		TWeakObjectPtr<UMaterialInstanceDynamic> Lamps;
		TWeakObjectPtr<USpotLightComponent> Heads[2];
		float WheelRadius = 32.0f;
		bool bCab = false;
		int32 Lane = -1;
		double S = 0.0;
		float Speed = 0.0f;
		double Roll = 0.0;
		uint32 Id = 0;
	};

	/** One car of Kind (StreetTraffic.cpp's kit table; -1 the sedan or a box) made for the pool, hidden. */
	void MakeCar(int32 Kind, uint32 Paint);
	void Launch(FCar& Car, int32 Lane);
	void Park(FCar& Car);
	void Place(FCar& Car);
	void ApplyLamps(FCar& Car);
	/** Whether the camera would see a car appear at At (in front of it, within 70 m). */
	bool Seen(const FVector& At) const;

	UPROPERTY()
	TObjectPtr<USceneComponent> Root;
	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;
	TWeakObjectPtr<AStreetStage> Street;
	TArray<FLane> Lanes;
	TArray<FCar> Cars;
	/** The cars' plates (core's PlateText, drawn on their [screen] slot). */
	Ns::FScreens Screens;
	/** When each street sends its next car (its own clock, seconds). */
	double NextAt[3] = {0.0, 0.0, 0.0};
	Ns::FGlowLevels Levels;
	float Rain = 0.0f;
	bool bHold = false;
	double Clock = 0.0;
	uint32 Dice = 1u;
	bool bBegun = false;
	int32 Passes = 0;
	float AppliedNight = -1.0f;
};
