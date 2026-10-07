#pragma once
// The talk card (the realism update's spec, §6.4 and §7.4): what someone on the block says when the hero talks to them.

#include "ShortStack/Game/Talk.h"
#include "ShortStack/UI/ShopScreen.h"

#include <string>
#include <vector>

namespace ss
{
namespace ui
{
/**
 * A bottom-center card with the speaker's name and label ("Sal", "Knows you"), their line typed on at 45 characters a
 * second, and 2-4 choices. Up and Down choose, Enter (or Space, or the pad's A) answers, finishing the line first when
 * it's still typing; Escape (or the pad's B) walks away. A click on a choice answers it.
 *
 * The card only shows what it's given: the host (AStreetNpcs) builds each line with town::ContextFor, Open and Answer,
 * hands it to Show, and drains TakeRequests for Talk (a choice, Name its id: the host answers it with town::Answer and
 * Show, or opens the person's shop for a service choice) and Leave (walked away, or a goodbye chosen: an id of "bye",
 * "leave" or "see-you", or the label "See you."). It's a ShopScreen, so the game's one ActiveScreen holds it and routes
 * its keys; for a moment after opening only Escape gets through, as the shops' screens do.
 */
class TalkCard final : public ShopScreen
{
public:
	static constexpr double TypeRate = 45.0;

	/** Opens for Speaker (the person's id); Show gives it its first line. */
	SHORTSTACKCORE_API void Open(double Now, const std::string& Speaker) override;
	/** The greeting, then each answer: the line types on afresh and the choices come back. */
	SHORTSTACKCORE_API void Show(double Now, const town::Talk& Line);
	SHORTSTACKCORE_API void Close(double Now) override;
	bool IsOpen() const override { return Shown; }
	SHORTSTACKCORE_API bool TakeLeave() override;
	SHORTSTACKCORE_API void Key(const std::string& Name, double Now) override;
	SHORTSTACKCORE_API void Draw(Canvas& C, double Now) override;
	SHORTSTACKCORE_API std::vector<HostRequest> TakeRequests() override;

	/** The line is still typing on. */
	SHORTSTACKCORE_API bool Typing(double Now) const;
	/** A choice was made and the card waits for the host's answer (Show). */
	bool Waiting() const { return Asked; }
	int Selected() const { return Sel; }
	const town::Talk& Current() const { return Said; }
	const std::string& Speaker() const { return Who; }

private:
	void Choose(int Index, double Now);
	void Leave(double Now);

	bool Shown = false;
	bool Left = false;
	bool Asked = false;
	std::string Who;
	town::Talk Said;
	int Sel = 0;
	double OpenedAt = -10.0;
	double LineAt = -10.0;
	double SkipTo = -1.0; // Enter while typing: the whole line now
	float PressX = -1.0f;
	float PressY = -1.0f;
	std::vector<HostRequest> Requests;
};
} // namespace ui
} // namespace ss
