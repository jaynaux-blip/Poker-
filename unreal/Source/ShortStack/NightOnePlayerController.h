#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "NightOnePlayerController.generated.h"

/**
 * Mouse and keyboard, polled each frame and handed to the game mode: the
 * mouse ray onto the laptop screen (like the prototype's UV raycast), clicks,
 * the wheel for bet sizing and the hotkeys.
 */
UCLASS()
class SHORTSTACK_API ANightOnePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ANightOnePlayerController();
	virtual void PlayerTick(float DeltaTime) override;
};
