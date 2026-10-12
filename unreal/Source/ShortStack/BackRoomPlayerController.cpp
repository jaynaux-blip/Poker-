#include "BackRoomPlayerController.h"

#include "BackRoomGameMode.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"
#include "Misc/App.h"

namespace BackRoomControllerDetail
{
// A held bumper keeps sizing the raise: after this long, a step every Every seconds.
constexpr float RepeatDelay = 0.4f;
constexpr float RepeatEvery = 0.1f;

/** How many repeat steps a bumper held for T seconds has made. */
int32 RepeatCount(float T)
{
	return T > RepeatDelay ? FMath::FloorToInt((T - RepeatDelay) / RepeatEvery) + 1 : 0;
}
} // namespace BackRoomControllerDetail

using namespace BackRoomControllerDetail;

ABackRoomPlayerController::ABackRoomPlayerController()
{
	bShowMouseCursor = false;
	// (B held went all in before the one control map; Y held does now, in the pawn: next_segment_spec 6.4.)
	Holds[0].Key = EKeys::Gamepad_DPad_Down;
	Repeats[0].Key = EKeys::Gamepad_RightShoulder;
	Repeats[1].Key = EKeys::Gamepad_LeftShoulder;
}

bool ABackRoomPlayerController::HoldCompleted(const FKey& Key) const
{
	for (const FHold& H : Holds)
	{
		if (H.Key == Key)
		{
			return H.bCompleted;
		}
	}
	return false;
}

float ABackRoomPlayerController::HoldProgress(const FKey& Key) const
{
	for (const FHold& H : Holds)
	{
		if (H.Key == Key)
		{
			return H.bFired ? 1.0f : FMath::Clamp(H.Held / HoldSeconds, 0.0f, 1.0f);
		}
	}
	return 0.0f;
}

int32 ABackRoomPlayerController::RepeatSteps(const FKey& Key) const
{
	for (const FRepeat& R : Repeats)
	{
		if (R.Key == Key)
		{
			return R.Steps;
		}
	}
	return 0;
}

void ABackRoomPlayerController::TestHoldPad(const FString& Key, bool bDown)
{
	for (FHold& H : Holds)
	{
		if (H.Key.GetFName() == FName(*Key))
		{
			H.bTest = bDown;
		}
	}
}

void ABackRoomPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	ABackRoomGameMode* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<ABackRoomGameMode>() : nullptr;
	// The pause menu takes its own input; what was held when it opened starts over when it closes.
	const bool bMenu = Mode && Mode->IsPauseMenuUp();
	// Real seconds: Focus and a folded hand's pace dilate the room, never the buttons.
	const float RealDt = FMath::Min(static_cast<float>(FApp::GetDeltaTime()), 0.1f);
	for (FHold& H : Holds)
	{
		H.bCompleted = false;
		if (bMenu || !(IsInputKeyDown(H.Key) || H.bTest))
		{
			H.Held = 0.0f;
			H.bFired = false;
			H.bArmed = false;
			continue;
		}
		H.bArmed = H.bArmed || H.bTest || WasInputKeyJustPressed(H.Key);
		if (!H.bArmed)
		{
			continue;
		}
		H.Held += RealDt;
		if (!H.bFired && H.Held >= HoldSeconds)
		{
			H.bFired = true;
			H.bCompleted = true;
		}
	}
	for (FRepeat& R : Repeats)
	{
		const float Was = R.Held;
		const bool bDown = !bMenu && IsInputKeyDown(R.Key);
		R.bArmed = bDown && (R.bArmed || WasInputKeyJustPressed(R.Key));
		R.Held = R.bArmed ? R.Held + RealDt : 0.0f;
		R.Steps = R.Held > 0.0f ? RepeatCount(R.Held) - RepeatCount(Was) : 0;
	}
	if (bMenu)
	{
		return;
	}

	// The hints and prompts follow the last device used: a gamepad's button, a stick or a trigger pushed; or a key, a mouse
	// button or the mouse moved.
	static const FKey PadButtons[] = {EKeys::Gamepad_FaceButton_Bottom, EKeys::Gamepad_FaceButton_Right, EKeys::Gamepad_FaceButton_Left, EKeys::Gamepad_FaceButton_Top,
		EKeys::Gamepad_LeftShoulder, EKeys::Gamepad_RightShoulder, EKeys::Gamepad_LeftTrigger, EKeys::Gamepad_RightTrigger, EKeys::Gamepad_DPad_Up, EKeys::Gamepad_DPad_Down,
		EKeys::Gamepad_DPad_Left, EKeys::Gamepad_DPad_Right, EKeys::Gamepad_Special_Left, EKeys::Gamepad_Special_Right, EKeys::Gamepad_LeftThumbstick, EKeys::Gamepad_RightThumbstick};
	static const FKey PadAxes[] = {EKeys::Gamepad_LeftX, EKeys::Gamepad_LeftY, EKeys::Gamepad_RightX, EKeys::Gamepad_RightY, EKeys::Gamepad_LeftTriggerAxis, EKeys::Gamepad_RightTriggerAxis};
	static const FKey DeskKeys[] = {EKeys::W, EKeys::A, EKeys::S, EKeys::D, EKeys::E, EKeys::F, EKeys::C, EKeys::R, EKeys::L, EKeys::N, EKeys::Q, EKeys::T, EKeys::V, EKeys::B,
		EKeys::P, EKeys::Tab, EKeys::SpaceBar, EKeys::Escape, EKeys::LeftShift, EKeys::RightShift, EKeys::Up, EKeys::Down, EKeys::Left, EKeys::Right, EKeys::LeftMouseButton,
		EKeys::RightMouseButton, EKeys::MiddleMouseButton, EKeys::MouseScrollUp, EKeys::MouseScrollDown, EKeys::U, EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four,
		EKeys::Five, EKeys::Six, EKeys::BackSpace};
	bool bPad = false;
	for (const FKey& Key : PadButtons)
	{
		bPad = bPad || WasInputKeyJustPressed(Key);
	}
	for (const FKey& Axis : PadAxes)
	{
		bPad = bPad || FMath::Abs(GetInputAnalogKeyState(Axis)) > 0.3f;
	}
	float Mx = 0.0f;
	float My = 0.0f;
	GetInputMouseDelta(Mx, My);
	bool bDesk = FMath::Abs(Mx) + FMath::Abs(My) > 0.5f;
	for (const FKey& Key : DeskKeys)
	{
		bDesk = bDesk || WasInputKeyJustPressed(Key);
	}
	if (bPad)
	{
		bGamepad = true;
	}
	else if (bDesk)
	{
		bGamepad = false;
	}

	// Pause. In Play-In-Editor, Escape stops the session before the game sees it, so P works too; on a gamepad, Start. With
	// the table card up (or just put down by this very press), Escape is the card's: back, not pause.
	const ABackRoomPawn* Seated = Cast<ABackRoomPawn>(GetPawn());
	const bool bCardsEscape = Seated && (Seated->IsTableCardUp() || FPlatformTime::Seconds() - Seated->GetTableCardClosedAt() < 0.1);
	if (Mode && ((WasInputKeyJustPressed(EKeys::Escape) && !bCardsEscape) || WasInputKeyJustPressed(EKeys::P) || WasInputKeyJustPressed(EKeys::Gamepad_Special_Right)))
	{
		Mode->OpenPauseMenu();
	}
}
