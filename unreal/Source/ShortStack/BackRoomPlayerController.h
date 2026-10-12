#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "BackRoomPlayerController.generated.h"

/**
 * The card table's controller (Dee's game and the Embercrest). Escape pauses (P in Play-In-Editor, where Escape stops the
 * session; Start on a gamepad; with the table card up, Escape puts the card down instead), and it keeps which device the
 * player last touched, so the table's hints and prompts show that one's keys or buttons. The rest of the table's input is
 * read by the pawn (ABackRoomPawn) against the one control map (ss::ui::Controls; next_segment_spec 6.4), which asks this
 * for the gamepad's held d-pad down (leaving the table completes only once held, so a stray press can't) and a bumper held
 * down (it keeps stepping the raise's size).
 *
 * The gamepad at the table: A checks or calls (folded at a live table: the next hand), X folds, Y raises when let go
 * quickly and goes all in when held (past 0.35 s the ALL IN rung fills, all in at 0.95 s; let go between and nothing
 * happens), B backs out (the table card; never anything to the hand), a bumper tapped steps the presets and held steps the
 * raise finely. The left trigger lifts the corners of your cards, the right one studies a face, the left stick held in
 * breathes slowly, the right stick looks. View is over the shoulder (the right stick in: the other shoulder). On the
 * d-pad: left stands up, right sets the table's pace, up held shows the table card, down held leaves. On foot, as on the
 * street: the left stick walks (held in, or the left bumper, to hurry), A uses what's in reach, Y is the view, B pauses.
 */
UCLASS()
class SHORTSTACK_API ABackRoomPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ABackRoomPlayerController();
	virtual void PlayerTick(float DeltaTime) override;

	/** The last input came from a gamepad (a button pressed, or a stick or trigger pushed): the HUD shows its buttons. */
	bool IsUsingGamepad() const { return bGamepad; }
	void SetUsingGamepad(bool bOn) { bGamepad = bOn; }

	/** Real seconds a held button takes to complete. */
	static constexpr float HoldSeconds = 0.6f;
	/**
	 * The held buttons (down on the d-pad: leaving the table): true the one frame the hold completes, and not again until
	 * the button is let go. HoldProgress (0..1) is for the HUD's fill.
	 */
	bool HoldCompleted(const FKey& Key) const;
	float HoldProgress(const FKey& Key) const;
	/** A bumper held down: the extra raise steps its repeat adds this frame (the press itself is the pawn's to count). */
	int32 RepeatSteps(const FKey& Key) const;

	/** Testing: as if the last input came from a gamepad (or from the keyboard and mouse), for the HUD's glyphs. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestGamepad(bool bOn) { bGamepad = bOn; }
	/** Testing: holds one of the held buttons down, or lets it go ("Gamepad_DPad_Down"). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Test")
	void TestHoldPad(const FString& Key, bool bDown);

private:
	struct FHold
	{
		FKey Key;
		/** Pressed while the game had the input: a button already down (held as the pause menu closed, and coming back
		 *  through the pad's repeats) never counts as held. */
		bool bArmed = false;
		float Held = 0.0f;
		/** Completed this hold (it waits for the button to come up before it can again). */
		bool bFired = false;
		/** Completed this frame. */
		bool bCompleted = false;
		bool bTest = false;
	};
	struct FRepeat
	{
		FKey Key;
		/** As FHold's: only a press the game saw repeats. */
		bool bArmed = false;
		float Held = 0.0f;
		int32 Steps = 0;
	};
	FHold Holds[1];
	FRepeat Repeats[2];
	bool bGamepad = false;
};
