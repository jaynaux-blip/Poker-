#pragma once

#include "ShortStack/UI/Canvas.h"

namespace ss
{
namespace ui
{
/** The phone's lock screen with text notifications. Port of web/src/game/phone.ts. */
class PhoneScreen
{
public:
	static constexpr float Width = 360.0f;
	static constexpr float Height = 760.0f;

	/** A new text: lights the screen for nine seconds. */
	SHORTSTACKCORE_API void Notify(const std::string& From, const std::string& Body, double Now);
	/** Screen brightness 0..1 at Now (0 when dark). */
	SHORTSTACKCORE_API double Brightness(double Now) const;
	SHORTSTACKCORE_API void Draw(Canvas& C, double ClockMinutes) const;

private:
	struct Message
	{
		std::string From;
		std::string Body;
		double At = 0.0;
	};
	std::vector<Message> Messages;
	double LitUntil = 0.0;
};
} // namespace ui
} // namespace ss
