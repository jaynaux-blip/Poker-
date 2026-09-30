#include "ShortStack/UI/Phone.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/UI/Ui.h"

namespace ss
{
namespace ui
{
void PhoneScreen::Notify(const std::string& From, const std::string& Body, double Now)
{
	Message M;
	M.From = From;
	M.Body = Body;
	M.At = Now;
	Messages.insert(Messages.begin(), M);
	if (Messages.size() > 4)
	{
		Messages.resize(4);
	}
	LitUntil = Now + 9.0;
}

double PhoneScreen::Brightness(double Now) const
{
	if (Now >= LitUntil)
	{
		return 0.0;
	}
	const double Fade = (LitUntil - Now) / 1.5;
	const double Level = Fade < 1.0 ? Fade : 1.0;
	return Level > 0.15 ? Level : 0.15;
}

void PhoneScreen::Draw(Canvas& C, double ClockMinutes) const
{
	const float W = Width;
	const float H = Height;
	C.FillRect({0.0f, 0.0f, W, H}, Paint::Linear({0.0f, 0.0f}, {W, H}, Hex(0x1d2a4a), Hex(0x3a1638)));
	std::string Time = ClockString(ClockMinutes);
	Time = Time.substr(0, Time.find(' '));
	C.Text(Time, W / 2.0f, 170.0f, Ts(96.0f, 300, Hex(0xffffff), Align::Center));
	C.Text("Wednesday", W / 2.0f, 210.0f, Ts(22.0f, 500, Rgba(255, 255, 255, 0.75f), Align::Center));
	float Y = 260.0f;
	for (const Message& M : Messages)
	{
		C.FillRoundRect({16.0f, Y, W - 32.0f, 150.0f}, 22.0f, Rgba(255, 255, 255, 0.16f));
		C.Text(M.From, 36.0f, Y + 38.0f, Ts(22.0f, 700, Hex(0xffffff)));
		// Word wrap the body.
		std::string Line;
		float Ly = Y + 70.0f;
		size_t Start = 0;
		bool Overflow = false;
		while (Start <= M.Body.size() && !Overflow)
		{
			size_t End = M.Body.find(' ', Start);
			if (End == std::string::npos)
			{
				End = M.Body.size();
			}
			const std::string Word = M.Body.substr(Start, End - Start);
			const std::string Test = Line.empty() ? Word : Line + " " + Word;
			if (C.Measure(Test, 19.0f, 400) > W - 72.0f)
			{
				C.Text(Line, 36.0f, Ly, Ts(19.0f, 400, Rgba(255, 255, 255, 0.88f)));
				Line = Word;
				Ly += 25.0f;
				Overflow = Ly > Y + 140.0f;
			}
			else
			{
				Line = Test;
			}
			Start = End + 1;
		}
		if (!Overflow && Ly <= Y + 140.0f)
		{
			C.Text(Line, 36.0f, Ly, Ts(19.0f, 400, Rgba(255, 255, 255, 0.88f)));
		}
		Y += 164.0f;
		if (Y > H - 100.0f)
		{
			break;
		}
	}
}
} // namespace ui
} // namespace ss
