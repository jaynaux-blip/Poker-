#include "SFrontEndWidget.h"

#include "InputCoreTypes.h"
#include "ShortStack/UI/FrontEnd.h"

void SFrontEndWidget::Construct(const FArguments& InArgs)
{
	SDrawListWidget::Construct(SDrawListWidget::FArguments().DesiredSize(FVector2D(1920.0, 1080.0)));
}

FVector2D SFrontEndWidget::ToLogical(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const FVector2D Size = MyGeometry.GetLocalSize();
	const double Scale = Size.Y > 0.0 ? static_cast<double>(ss::ui::FrontEnd::Height) / Size.Y : 1.0;
	return Local * Scale;
}

FReply SFrontEndWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	// A held button's repeats: arrows and the stick keep them (hold to move along); confirming, paging and backing out
	// don't (holding A or E to talk to Benny would otherwise drop a Cascade in the basket or flip the shelf).
	if (InKeyEvent.IsRepeat() && (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Tab || Key == EKeys::Escape || Key == EKeys::P || Key == EKeys::Q
		|| Key == EKeys::E || Key == EKeys::R || Key == EKeys::Gamepad_FaceButton_Bottom || Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::Gamepad_FaceButton_Top
		|| Key == EKeys::Gamepad_FaceButton_Left || Key == EKeys::Gamepad_LeftShoulder || Key == EKeys::Gamepad_RightShoulder || Key == EKeys::Gamepad_Special_Right))
	{
		return FReply::Handled();
	}
	FString Name = TEXT("Any");
	if (Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_LeftStick_Up)
	{
		Name = TEXT("Up");
	}
	else if (Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Gamepad_LeftStick_Down)
	{
		Name = TEXT("Down");
	}
	else if (Key == EKeys::Left || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_LeftStick_Left)
	{
		Name = TEXT("Left");
	}
	else if (Key == EKeys::Right || Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_LeftStick_Right)
	{
		Name = TEXT("Right");
	}
	else if (Key == EKeys::Enter || Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		Name = TEXT("Enter");
	}
	else if (Key == EKeys::SpaceBar)
	{
		Name = TEXT("Space");
	}
	else if (Key == EKeys::Escape || Key == EKeys::P || Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::Gamepad_Special_Right)
	{
		// P doubles as Escape: in Play-In-Editor, Escape stops the session instead of reaching the game.
		Name = Key == EKeys::P ? TEXT("P") : TEXT("Escape");
	}
	else if (Key == EKeys::BackSpace)
	{
		Name = TEXT("Backspace");
	}
	else if (Key == EKeys::Tab)
	{
		Name = TEXT("Tab");
	}
	else if (Key == EKeys::Z && InKeyEvent.IsControlDown() && !InKeyEvent.IsAltDown())
	{
		// Not with Alt: AltGr arrives as Ctrl+Alt, and AltGr+Z types a letter on some layouts (Polish z with a dot).
		Name = TEXT("Undo");
	}
	else if (Key == EKeys::Q || Key == EKeys::E || Key == EKeys::R)
	{
		Name = Key.GetFName().ToString();
	}
	else if (Key == EKeys::Gamepad_LeftShoulder)
	{
		Name = TEXT("TabPrev");
	}
	else if (Key == EKeys::Gamepad_RightShoulder)
	{
		Name = TEXT("TabNext");
	}
	else if (Key == EKeys::Gamepad_FaceButton_Top)
	{
		Name = TEXT("Reset");
	}
	else if (Key == EKeys::Gamepad_FaceButton_Left)
	{
		// Not "Backspace": the creator's text fields eat that. The counter puts an item back, the creator undoes.
		Name = TEXT("PutBack");
	}
	if (OnMenuKey)
	{
		OnMenuKey(Name, Key.IsGamepadKey());
	}
	return FReply::Handled();
}

FReply SFrontEndWidget::OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent)
{
	if (OnMenuChar)
	{
		OnMenuChar(static_cast<uint32>(InCharacterEvent.GetCharacter()));
	}
	return FReply::Handled();
}

FReply SFrontEndWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (OnMenuPointer)
	{
		OnMenuPointer(ToLogical(MyGeometry, MouseEvent), 0, 0.0f);
	}
	return FReply::Handled();
}

FReply SFrontEndWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (OnMenuPointer && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnMenuPointer(ToLogical(MyGeometry, MouseEvent), 1, 0.0f);
	}
	return FReply::Handled();
}

FReply SFrontEndWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (OnMenuPointer && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnMenuPointer(ToLogical(MyGeometry, MouseEvent), 2, 0.0f);
	}
	return FReply::Handled();
}

FReply SFrontEndWidget::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (OnMenuPointer)
	{
		OnMenuPointer(ToLogical(MyGeometry, MouseEvent), 0, -MouseEvent.GetWheelDelta() * 80.0f);
	}
	return FReply::Handled();
}
