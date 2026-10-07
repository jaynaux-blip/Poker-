// The talk card (the realism update's spec, §6.4 and §7.4): the speaker, the line typed on, the choices.
#include "ShortStack/UI/TalkCard.h"
#include "../StrictFloat.h"
#include "FrontEndShared.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace ss
{
namespace ui
{
namespace talkcard_detail
{
using namespace frontend_detail;

constexpr double OpenGrace = 0.3;

Color Faded(Color C, float A)
{
	C.A *= A;
	return C;
}

bool Goodbye(const town::Choice& C)
{
	return C.Id == "bye" || C.Id == "leave" || C.Id == "see-you" || C.Label == "See you." || C.Label == "See you";
}

/** The first Count glyphs of Text (UTF-8 aware, so a typed "é" never shows half a letter). */
std::string FirstGlyphs(const std::string& Text, size_t Count)
{
	size_t I = 0;
	size_t N = 0;
	while (I < Text.size() && N < Count)
	{
		const unsigned char Lead = static_cast<unsigned char>(Text[I]);
		I += Lead >= 0xF0 ? 4 : Lead >= 0xE0 ? 3 : Lead >= 0xC0 ? 2 : 1;
		++N;
	}
	return Text.substr(0, std::min(I, Text.size()));
}

size_t GlyphCount(const std::string& Text)
{
	size_t N = 0;
	for (const char Ch : Text)
	{
		N += (static_cast<unsigned char>(Ch) & 0xC0) != 0x80 ? 1 : 0;
	}
	return N;
}
} // namespace talkcard_detail

using namespace talkcard_detail;

void TalkCard::Open(double Now, const std::string& Speaker)
{
	Shown = true;
	Left = false;
	Asked = false;
	Who = Speaker;
	Said = town::Talk();
	Sel = 0;
	OpenedAt = Now;
	LineAt = Now;
	SkipTo = -1.0;
	PressX = PressY = -1.0f;
}

void TalkCard::Show(double Now, const town::Talk& Line)
{
	if (!Shown)
	{
		Open(Now, Who);
	}
	Said = Line;
	Asked = false;
	Sel = 0;
	LineAt = Now;
	SkipTo = -1.0;
}

void TalkCard::Close(double Now)
{
	Shown = false;
	Asked = false;
	OpenedAt = Now;
}

bool TalkCard::TakeLeave()
{
	const bool Was = Left;
	Left = false;
	return Was;
}

std::vector<HostRequest> TalkCard::TakeRequests()
{
	std::vector<HostRequest> Out;
	Out.swap(Requests);
	return Out;
}

bool TalkCard::Typing(double Now) const
{
	if (SkipTo >= 0.0)
	{
		return false;
	}
	return static_cast<double>(GlyphCount(Said.Line)) > (Now - LineAt) * TypeRate;
}

void TalkCard::Leave(double Now)
{
	HostRequest R;
	R.K = HostRequest::Leave;
	Requests.push_back(R);
	Close(Now);
	Left = true;
}

void TalkCard::Choose(int Index, double Now)
{
	if (Index < 0 || Index >= static_cast<int>(Said.Choices.size()) || Asked)
	{
		return;
	}
	const town::Choice& C = Said.Choices[static_cast<size_t>(Index)];
	HostRequest R;
	R.K = HostRequest::Talk;
	R.Name = C.Id;
	Requests.push_back(R);
	if (Goodbye(C))
	{
		Leave(Now);
		return;
	}
	Asked = true;
}

void TalkCard::Key(const std::string& Name, double Now)
{
	if (!Shown)
	{
		return;
	}
	if (Now - OpenedAt < OpenGrace && Name != "Escape" && Name != "P")
	{
		return;
	}
	const int N = static_cast<int>(Said.Choices.size());
	if (Name == "Escape" || Name == "P")
	{
		Leave(Now);
	}
	else if ((Name == "Up" || Name == "Left") && N > 0)
	{
		Sel = (Sel + N - 1) % N;
	}
	else if ((Name == "Down" || Name == "Right") && N > 0)
	{
		Sel = (Sel + 1) % N;
	}
	else if (Name == "Enter" || Name == "Space")
	{
		if (Typing(Now))
		{
			SkipTo = Now;
		}
		else
		{
			Choose(Sel, Now);
		}
	}
}

void TalkCard::Draw(Canvas& C, double Now)
{
	if (!Shown)
	{
		Ptr.EndFrame();
		return;
	}
	if (Ptr.Pressed)
	{
		PressX = Ptr.X;
		PressY = Ptr.Y;
	}
	auto Released = [&](const Rect& R) { return Ptr.Released && Inside(R, Ptr.X, Ptr.Y) && Inside(R, PressX, PressY); };
	const float W = C.Width();
	const float H = Height;
	const float In = Ease((Now - OpenedAt) / 0.3);
	const float Al = C.GetAlpha();
	C.SetAlpha(Al * In);
	// A shade along the bottom so the card reads over the street.
	C.FillRect({0.0f, H - 420.0f, W, 420.0f}, Paint::Linear({0.0f, H - 420.0f}, {0.0f, H}, Rgba(4, 6, 10, 0.0f), Rgba(4, 6, 10, 0.7f)));
	const float Cw = std::min(980.0f, W - 2.0f * 96.0f);
	const float Lx = (W - Cw) * 0.5f;
	// The line, typed on (all of it once Enter skipped ahead).
	const bool Done = !Typing(Now);
	const size_t Shown_ = Done ? GlyphCount(Said.Line) : static_cast<size_t>(std::max(0.0, (Now - LineAt) * TypeRate));
	const std::vector<std::string> Full = WrapText(C, Said.Line, Cw - 64.0f, 21.0f, 500);
	std::vector<std::string> Lines;
	size_t Left_ = Shown_;
	for (const std::string& Ln : Full)
	{
		const size_t G = GlyphCount(Ln) + 1;
		if (Left_ == 0)
		{
			break;
		}
		Lines.push_back(Left_ >= G ? Ln : FirstGlyphs(Ln, Left_));
		Left_ = Left_ >= G ? Left_ - G : 0;
	}
	const int N = static_cast<int>(Said.Choices.size());
	const float LineH = 29.0f;
	const float ChoiceH = 44.0f;
	const float Body = 70.0f + std::max(1.0f, static_cast<float>(Full.size())) * LineH + 18.0f + static_cast<float>(N) * ChoiceH + 56.0f;
	const float Ty = H - 96.0f - Body + (1.0f - In) * 24.0f;
	const Rect Card{Lx, Ty, Cw, Body};
	C.GlowRoundRect(Card, 16.0f, Rgba(0, 0, 0, 0.55f), 30.0f);
	C.FillRoundRect(Card, 16.0f, Paint::Linear({0.0f, Card.Y}, {0.0f, Card.Y + Card.H}, Rgba(16, 19, 28, 0.96f), Rgba(9, 11, 17, 0.96f)));
	C.StrokeRoundRect(Card, 16.0f, Rgba(255, 255, 255, 0.08f), 1.0f);
	C.FillRoundRect({Card.X, Card.Y + 18.0f, 4.0f, 40.0f}, 2.0f, Paint(MenuTeal));
	// Who: the name and the label (their bond, or their role).
	std::string Name = Said.Name.empty() ? Who : Said.Name;
	if (!Name.empty() && Name[0] >= 'a' && Name[0] <= 'z')
	{
		Name[0] = static_cast<char>(Name[0] - 32);
	}
	const float Nw = C.Text(Name, Card.X + 32.0f, Card.Y + 48.0f, Ts(24.0f, 900, MenuInk, Align::Left, Baseline::Alphabetic, false, Cw * 0.6f));
	if (!Said.Label.empty())
	{
		std::string Label = Said.Label;
		std::transform(Label.begin(), Label.end(), Label.begin(), [](char Ch) { return static_cast<char>(Ch >= 'a' && Ch <= 'z' ? Ch - 32 : Ch); });
		const float Lw = TrackedWidth(C, Label, 11.0f, 800, 2.2f) + 20.0f;
		C.FillRoundRect({Card.X + 32.0f + Nw + 14.0f, Card.Y + 28.0f, Lw, 24.0f}, 12.0f, Paint(Faded(MenuTeal, 0.16f)));
		TrackedText(C, Label, Card.X + 32.0f + Nw + 24.0f, Card.Y + 44.5f, 11.0f, 800, MenuTeal, 2.2f);
	}
	float Ly = Card.Y + 70.0f;
	for (const std::string& Ln : Lines)
	{
		Ly += LineH;
		C.Text(Ln, Card.X + 32.0f, Ly, Ts(21.0f, 500, Faded(MenuInk, 0.95f)));
	}
	if (!Done)
	{
		// The caret where the typing is.
		const float Blink = std::fmod(Now, 0.6) < 0.3 ? 1.0f : 0.25f;
		const float Cx = Card.X + 32.0f + (Lines.empty() ? 0.0f : C.Measure(Lines.back(), 21.0f, 500)) + 4.0f;
		C.FillRect({Cx, (Lines.empty() ? Card.Y + 70.0f + LineH : Ly) - 19.0f, 2.0f, 22.0f}, Paint(Faded(MenuTeal, Blink)));
	}
	// The choices: they come up once the line has.
	float Cy = Card.Y + 70.0f + std::max(1.0f, static_cast<float>(Full.size())) * LineH + 22.0f;
	const float Ca = Done ? Ease((Now - std::max(LineAt + static_cast<double>(GlyphCount(Said.Line)) / TypeRate, SkipTo)) / 0.25) : 0.0f;
	int Clicked = -1;
	for (int I = 0; I < N; ++I)
	{
		const Rect Row{Card.X + 20.0f, Cy, Cw - 40.0f, ChoiceH - 6.0f};
		const bool On = I == Sel;
		const bool Over = Inside(Row, Ptr.X, Ptr.Y);
		const float A = (Asked ? 0.35f : 1.0f) * std::max(Done ? Ca : 0.0f, 0.25f);
		if (On || Over)
		{
			C.FillRoundRect(Row, 8.0f, Paint(Faded(On ? MenuTeal : MenuInk, (On ? 0.14f : 0.06f) * A)));
		}
		if (On)
		{
			Arrow(C, Row.X + 18.0f, Row.Y + Row.H * 0.5f, 6.0f, 3, Faded(MenuTeal, A));
		}
		C.Text(Said.Choices[static_cast<size_t>(I)].Label, Row.X + 38.0f, Row.Y + 26.0f, Ts(18.0f, On ? 800 : 600, Faded(On ? MenuInk : MenuMuted, A), Align::Left, Baseline::Alphabetic, false, Row.W - 60.0f));
		if (Released(Row) && Done && !Asked)
		{
			Clicked = I;
		}
		else if (Released(Row) && !Done)
		{
			SkipTo = Now;
		}
		Cy += ChoiceH;
	}
	if (Asked)
	{
		// Waiting for the answer: three dots.
		for (int K = 0; K < 3; ++K)
		{
			const float B = 0.4f + 0.6f * static_cast<float>(std::max(0.0, std::sin(Now * 6.0 - K * 0.7)));
			C.FillCircle(Card.X + Cw - 70.0f + static_cast<float>(K) * 16.0f, Card.Y + 44.0f, 4.0f, Paint(Faded(MenuInk, B)));
		}
	}
	// The keys.
	float Hx = Card.X + 32.0f;
	const float Hy = Card.Y + Card.H - 20.0f;
	const std::pair<const char*, const char*> Hints[3] = {{Gamepad ? "DPAD" : "UP", "CHOOSE"}, {Gamepad ? "A" : "ENTER", Done ? "SAY IT" : "GO ON"}, {Gamepad ? "B" : "ESC", "WALK AWAY"}};
	for (int K = 0; K < 3; ++K)
	{
		const float X0 = Hx;
		Hx += Glyph(C, Hints[K].first, Hx, Hy, 0.9f) + 10.0f;
		Hx += TrackedText(C, Hints[K].second, Hx, Hy - 2.0f, 12.0f, 700, MenuMuted, 2.4f) + 28.0f;
		if (K == 2 && Released({X0 - 8.0f, Hy - 29.0f, Hx - X0 - 12.0f, 40.0f}))
		{
			C.SetAlpha(Al);
			Ptr.EndFrame();
			Leave(Now);
			return;
		}
	}
	C.SetAlpha(Al);
	Ptr.EndFrame();
	if (Clicked >= 0)
	{
		Sel = Clicked;
		Choose(Clicked, Now);
	}
}
} // namespace ui
} // namespace ss
