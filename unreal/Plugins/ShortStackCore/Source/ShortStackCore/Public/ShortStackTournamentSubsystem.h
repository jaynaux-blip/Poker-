// Runs a SHORT STACK tournament inside the game instance, for Blueprints and UMG.
//
// This first version sims whole rounds with the hero on autopilot, which is enough
// to drive the lobby, the tournament clock, standings and bust-out UI while the
// playable table is built. Interactive hands come next: C++ can already call
// GetTournament() and use ss::Tournament::StartTick / FinishTick directly.
#pragma once

#include "CoreMinimal.h"
#include "ShortStack/Tournament.h"
#include "ShortStackTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Templates/UniquePtr.h"

#include "ShortStackTournamentSubsystem.generated.h"

UCLASS()
class SHORTSTACKCORE_API UShortStackTournamentSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	/** Seats the field and deals the first round. Replaces any tournament in progress. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Tournament")
	void StartTournament(const FShortStackTournamentSettings& Settings);

	/** Plays one hand at every table (the hero on autopilot) and returns what happened. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Tournament")
	TArray<FShortStackEvent> SimulateRound();

	/** Plays up to Count rounds, stopping early when the tournament ends. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Tournament")
	TArray<FShortStackEvent> SimulateRounds(int32 Count);

	UFUNCTION(BlueprintPure, Category = "Short Stack|Tournament")
	bool IsRunning() const { return Tournament.IsValid(); }

	UFUNCTION(BlueprintPure, Category = "Short Stack|Tournament")
	bool IsFinished() const;

	UFUNCTION(BlueprintPure, Category = "Short Stack|Tournament")
	FString GetTournamentName() const;

	UFUNCTION(BlueprintPure, Category = "Short Stack|Tournament")
	int32 GetRound() const;

	UFUNCTION(BlueprintPure, Category = "Short Stack|Tournament")
	int32 GetEntrants() const;

	UFUNCTION(BlueprintPure, Category = "Short Stack|Tournament")
	int32 GetPlayersRemaining() const;

	UFUNCTION(BlueprintPure, Category = "Short Stack|Tournament")
	int32 GetPaidPlaces() const;

	UFUNCTION(BlueprintPure, Category = "Short Stack|Tournament")
	int64 GetPrizePoolCents() const;

	/** Prize in cents for a finishing place (1 = winner), 0 outside the money. */
	UFUNCTION(BlueprintPure, Category = "Short Stack|Tournament")
	int64 GetPrizeForPlace(int32 Place) const;

	UFUNCTION(BlueprintPure, Category = "Short Stack|Tournament")
	bool IsInTheMoney() const;

	UFUNCTION(BlueprintPure, Category = "Short Stack|Tournament")
	bool IsHandForHand() const;

	/** Current blind level, 1-based. */
	UFUNCTION(BlueprintPure, Category = "Short Stack|Tournament")
	int32 GetLevelNumber() const;

	UFUNCTION(BlueprintPure, Category = "Short Stack|Tournament")
	void GetBlinds(int64& SmallBlind, int64& BigBlind, int64& Ante) const;

	UFUNCTION(BlueprintPure, Category = "Short Stack|Tournament")
	double GetLevelSecondsLeft() const;

	/** Game clock in minutes after midnight. */
	UFUNCTION(BlueprintPure, Category = "Short Stack|Tournament")
	double GetClockMinutes() const;

	UFUNCTION(BlueprintPure, Category = "Short Stack|Tournament")
	int64 GetAverageStack() const;

	/** Hero's current rank, or finishing place once busted. */
	UFUNCTION(BlueprintPure, Category = "Short Stack|Tournament")
	int32 GetHeroRank() const;

	UFUNCTION(BlueprintPure, Category = "Short Stack|Tournament")
	int64 GetHeroStack() const;

	UFUNCTION(BlueprintPure, Category = "Short Stack|Tournament")
	bool IsHeroBusted() const;

	/** Chip leaders first. MaxCount <= 0 returns every player still in. */
	UFUNCTION(BlueprintPure, Category = "Short Stack|Tournament")
	TArray<FShortStackStanding> GetStandings(int32 MaxCount = 10) const;

	/** The engine object for C++ callers (nullptr before StartTournament). */
	ss::Tournament* GetTournament() const { return Tournament.Get(); }

private:
	TUniquePtr<ss::Tournament> Tournament;
	ss::Profile HeroAutopilot;
};
