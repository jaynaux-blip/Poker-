// The menus (ss::ui::FrontEnd): a full-screen draw list that also takes keyboard, gamepad and mouse input.
#pragma once

#include "CoreMinimal.h"
#include "Input/Events.h"
#include "Input/Reply.h"
#include "SlateDrawList.h"

class SHORTSTACK_API SFrontEndWidget : public SDrawListWidget
{
public:
	SLATE_BEGIN_ARGS(SFrontEndWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** A key name for ss::ui::FrontEnd::Key, and whether a gamepad sent it. */
	TFunction<void(const FString&, bool)> OnMenuKey;
	/** A typed character. */
	TFunction<void(uint32)> OnMenuChar;
	/** The pointer in the menu's logical units (1080 high): 0 moved, 1 pressed, 2 released; wheel in logical units. */
	TFunction<void(const FVector2D&, int32, float)> OnMenuPointer;

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	static FVector2D ToLogical(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
};
