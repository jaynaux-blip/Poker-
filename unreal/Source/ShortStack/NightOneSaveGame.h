#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"

#include "NightOneSaveGame.generated.h"

/** Bankroll, screen name, results and story flags (ss::SaveData, serialized as text). */
UCLASS()
class SHORTSTACK_API UNightOneSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString Data;

	static const TCHAR* SlotName() { return TEXT("NightOne"); }
	/** The player's settings (ss::ui::GameSettings, serialized as text) live in their own slot. */
	static const TCHAR* SettingsSlotName() { return TEXT("Settings"); }
};
