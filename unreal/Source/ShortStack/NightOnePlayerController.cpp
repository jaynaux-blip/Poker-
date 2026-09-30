#include "NightOnePlayerController.h"

#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "NightOneGameMode.h"
#include "NightOneStage.h"

ANightOnePlayerController::ANightOnePlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
}

void ANightOnePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	ANightOneGameMode* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<ANightOneGameMode>() : nullptr;
	if (!Mode)
	{
		return;
	}
	float Mx = 0.0f;
	float My = 0.0f;
	int32 Vw = 0;
	int32 Vh = 0;
	GetViewportSize(Vw, Vh);
	bool bOverScreen = false;
	FVector2D Client(-1.0, -1.0);
	if (GetMousePosition(Mx, My) && Vw > 0 && Vh > 0)
	{
		FVector Origin;
		FVector Direction;
		if (Mode->GetStage() && DeprojectScreenPositionToWorld(Mx, My, Origin, Direction))
		{
			bOverScreen = Mode->GetStage()->ScreenHit(Origin, Direction, Client);
		}
		Mode->OnMouse(bOverScreen, Client, Mx / static_cast<float>(Vw) - 0.5f, My / static_cast<float>(Vh) - 0.5f);
	}
	if (WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		Mode->OnPress(bOverScreen);
	}
	if (WasInputKeyJustReleased(EKeys::LeftMouseButton))
	{
		Mode->OnRelease();
	}
	if (WasInputKeyJustPressed(EKeys::MouseScrollUp))
	{
		Mode->OnWheel(-100.0f);
	}
	if (WasInputKeyJustPressed(EKeys::MouseScrollDown))
	{
		Mode->OnWheel(100.0f);
	}
	struct FHotkey
	{
		FKey Key;
		const TCHAR* Name;
	};
	const FHotkey Hotkeys[] = {
		{EKeys::SpaceBar, TEXT(" ")}, {EKeys::F, TEXT("f")}, {EKeys::C, TEXT("c")}, {EKeys::X, TEXT("x")}, {EKeys::R, TEXT("r")}, {EKeys::B, TEXT("b")},
		{EKeys::A, TEXT("a")}, {EKeys::M, TEXT("m")}, {EKeys::Up, TEXT("ArrowUp")}, {EKeys::Down, TEXT("ArrowDown")},
	};
	for (const FHotkey& H : Hotkeys)
	{
		if (WasInputKeyJustPressed(H.Key))
		{
			Mode->OnKey(H.Name);
		}
	}
	// Over the laptop the client draws its own cursor.
	CurrentMouseCursor = Mode->HideCursor(bOverScreen) ? EMouseCursor::None : Mode->IsLeanedBack() ? EMouseCursor::Crosshairs : EMouseCursor::Default;
}
