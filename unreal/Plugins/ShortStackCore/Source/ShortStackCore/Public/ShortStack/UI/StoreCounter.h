#pragma once

#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Store.h"
#include "ShortStack/UI/Ui.h"

#include <map>
#include <string>
#include <vector>

namespace ss
{
namespace ui
{
/** A product from the Lucky Penny's shelves, centered on (X, Y), Size tall. */
SHORTSTACKCORE_API void DrawProduct(Canvas& C, const store::Item& I, float X, float Y, float Size);

/**
 * Hunger, thirst and energy as a compact HUD block (the open world shows it while walking), Width wide
 * with its top-left at (X, Y). Preview, when not null, ghosts in what eating and drinking it would do.
 */
SHORTSTACKCORE_API void DrawVitals(Canvas& C, const life::State& L, float X, float Y, float Width, double Now, const store::Basket* Preview = nullptr, float Alpha = 1.0f);

/** What the HUD shows while walking around (the Street level fills it each frame). */
struct StreetHudInfo
{
	std::string Place = "FIFTH STREET"; // where the player is
	std::string Clock;                  // "2:41 AM"
	/** World minutes now (net::DayOf). With it, the HUD counts down to the next Penny Drop order; negative shows none. */
	double World = -1.0;
	Chips BankrollCents = 0;
	const life::State* Life = nullptr;
	/** The interaction in reach ("Talk to Benny"), and its key ("E"); empty when nothing is. */
	std::string Prompt;
	std::string PromptKey = "E";
	/** Texts that arrived recently: sender, body and when (seconds, the HUD's clock). */
	struct Toast
	{
		std::string From;
		std::string Body;
		double At = 0.0;
	};
	std::vector<Toast> Toasts;
	bool FirstPerson = false;
	/** When the controls hint first showed (it fades after a while); negative hides it. */
	double HintsAt = -100.0;
	bool Gamepad = false;
	/** 0..1: the screen fading to black (leaving, arriving). */
	float Fade = 0.0f;
};

/** The walking HUD: place and time, vitals, the prompt, texts, the camera mode and the controls. 1080 tall. */
SHORTSTACKCORE_API void DrawStreetHud(Canvas& C, const StreetHudInfo& Info, double Now);

/**
 * The counter at the Lucky Penny #212: the shelves on the left, Benny behind the register, the receipt
 * printing as the basket fills, and the bag to eat and drink from. Drawn full screen over the 3D store
 * into a canvas 1080 logical units tall (width follows the viewport). Keys: arrows choose, Enter puts
 * one in the basket, Backspace (or Delete, or "PutBack": the gamepad's X) takes one out, Q/E (or the
 * shoulders) change shelves, Tab (or Y) pays, Escape walks away from the counter. For a moment after
 * walking up only Escape gets through (the button that opened it may still be held down).
 */
class StoreCounter
{
public:
	static constexpr float Height = 1080.0f;

	explicit StoreCounter(Session& InSession) : S(InSession) {}
	StoreCounter(const StoreCounter&) = delete;
	StoreCounter& operator=(const StoreCounter&) = delete;

	Pointer Ptr;
	bool Gamepad = false;
	store::Basket Basket;

	/** Walks up to the counter (Shelf: the shelf to start on, from what the player was looking at). */
	SHORTSTACKCORE_API void Open(double Now, int Shelf = 0);
	SHORTSTACKCORE_API void Close(double Now);
	bool IsOpen() const { return Shown; }
	/** The player walked away from the counter since the last call (the host hands control back). */
	bool TakeLeave()
	{
		const bool Was = Left;
		Left = false;
		return Was;
	}
	int Shelf() const { return ShelfAt; }
	int Selected() const { return Sel; }
	/** The bag's chips as last drawn (item ids; one eaten to the last keeps its chip while the pointer is over the bag). */
	const std::vector<std::string>& BagShown() const { return BagCells; }
	/** The first bag chip showing (the bag pages when it holds more kinds than fit). */
	int BagPage() const { return BagFirst; }

	SHORTSTACKCORE_API void Key(const std::string& Name, double Now);
	SHORTSTACKCORE_API void Draw(Canvas& C, double Now);

	/** How long after walking up the counter ignores everything but Escape (seconds). */
	static constexpr double OpenGrace = 0.3;

private:
	std::vector<const store::Item*> OnShelf() const;
	void Pay(double Now);
	void Say(const std::string& Words, double Now, bool Bad);
	void Pick(const std::string& Id, double Now);
	void PutBack(const std::string& Id, double Now);
	void DrawBag(Canvas& C, float X, float Y, float W, double Now);
	void DrawReceipt(Canvas& C, float Rx, double Now);

	Session& S;
	bool Shown = false;
	bool Left = false;
	double OpenedAt = -10.0;
	int ShelfAt = 0;
	int Sel = 0;
	double SelAt = 0.0;
	std::string Note;
	double NoteAt = -10.0;
	bool NoteBad = false;
	double PaidAt = -10.0;
	double DeclinedAt = -10.0;
	store::Basket LastPaid;
	double PaidAuth = 0.0; // what the card reader printed on the last PAID receipt (a number from the moment it paid)
	float PressX = -1.0f;
	float PressY = -1.0f;
	/** When each receipt line last changed (it prints out of the slot again). */
	std::map<std::string, double> LineAt;
	/** The bag's chips, in a stable order while the pointer is over the bag (so the next click lands where it was aimed). */
	std::vector<std::string> BagCells;
	int BagFirst = 0;
};
} // namespace ui
} // namespace ss
