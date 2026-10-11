// The walking HUD (DrawStreetHud, declared in StoreCounter.h: the Embercrest draws it too). Moved here from
// StoreCounter.cpp for the realism update (§7.4), which adds the weather by the clock, a locked door's line, what people
// say (subtitles), the name over whoever the hero is looking at, and the overlay while a service or a wait plays out.
#include "ShortStack/UI/StoreCounter.h"
#include "../StrictFloat.h"
#include "FrontEndShared.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Weather.h"
#include "ShortStack/UI/Captions.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace ss
{
namespace ui
{
namespace streethud_detail
{
using namespace frontend_detail;

Color Dim(Color C, float A)
{
	C.A *= A;
	return C;
}

float Fl(int V)
{
	return static_cast<float>(V);
}

/** The weather as a glyph (Size across, centered): the sun or the moon, a cloud, drizzle, rain, a storm's bolt. */
void WeatherGlyph(Canvas& C, int Sky, bool Night, float X, float Y, float Size, const Color& Col, double Now)
{
	const float S = Size;
	auto Cloud = [&](float Cx, float Cy, float K, const Color& Fill) {
		C.FillCircle(Cx - S * 0.2f * K, Cy + S * 0.04f * K, S * 0.17f * K, Paint(Fill));
		C.FillCircle(Cx + S * 0.02f * K, Cy - S * 0.08f * K, S * 0.23f * K, Paint(Fill));
		C.FillCircle(Cx + S * 0.24f * K, Cy + S * 0.04f * K, S * 0.16f * K, Paint(Fill));
		C.FillRoundRect({Cx - S * 0.36f * K, Cy + S * 0.02f * K, S * 0.76f * K, S * 0.18f * K}, S * 0.09f * K, Paint(Fill));
	};
	if (Sky <= 0)
	{
		if (Night)
		{
			C.FillCircle(X, Y, S * 0.34f, Paint(Col));
			C.FillCircle(X + S * 0.16f, Y - S * 0.1f, S * 0.3f, Paint(Hex(0x0b0e14)));
		}
		else
		{
			C.FillCircle(X, Y, S * 0.2f, Paint(Hex(0xf2c14e)));
			for (int K = 0; K < 8; ++K)
			{
				const float A = Fl(K) * 0.785398f;
				C.StrokePolyline({{X + std::cos(A) * S * 0.3f, Y + std::sin(A) * S * 0.3f}, {X + std::cos(A) * S * 0.42f, Y + std::sin(A) * S * 0.42f}}, false, Hex(0xf2c14e), S * 0.06f, true);
			}
		}
		return;
	}
	Cloud(X, Y - S * 0.06f, 1.0f, Col);
	if (Sky == 2 || Sky == 3)
	{
		// Drizzle: a few short drops; rain: more, longer, falling.
		const int Drops = Sky == 2 ? 3 : 5;
		for (int K = 0; K < Drops; ++K)
		{
			const float Dx = X - S * 0.26f + Fl(K) * S * 0.52f / Fl(std::max(1, Drops - 1));
			const float Fall = static_cast<float>(std::fmod(Now * 1.6 + K * 0.37, 1.0));
			const float Dy = Y + S * 0.22f + Fall * S * 0.14f;
			const float Len = Sky == 2 ? S * 0.07f : S * 0.14f;
			C.StrokePolyline({{Dx, Dy}, {Dx - Len * 0.3f, Dy + Len}}, false, Hex(0x6fb8ff), S * 0.05f, true);
		}
	}
	else if (Sky >= 4)
	{
		C.FillPolygon({{X + S * 0.04f, Y + S * 0.12f}, {X - S * 0.12f, Y + S * 0.34f}, {X, Y + S * 0.34f}, {X - S * 0.06f, Y + S * 0.52f}, {X + S * 0.14f, Y + S * 0.26f}, {X + S * 0.02f, Y + S * 0.26f}},
			Paint(Hex(0xf2c14e)));
	}
}

/** A padlock (the closed line). */
void Padlock(Canvas& C, float X, float Y, float S, const Color& Col)
{
	C.StrokeArc(X, Y - S * 0.12f, S * 0.24f, 3.14159f, 6.28318f, Col, S * 0.09f);
	C.FillRoundRect({X - S * 0.34f, Y - S * 0.12f, S * 0.68f, S * 0.52f}, S * 0.08f, Paint(Col));
	C.FillCircle(X, Y + S * 0.1f, S * 0.07f, Paint(Hex(0x0b0e14)));
}

/** The skip's clock: a dial with its hands at World (minutes past midnight), spinning as the minutes race. */
void SkipClock(Canvas& C, float X, float Y, float R, double World, float Alpha)
{
	C.FillCircle(X, Y, R + 6.0f, Paint(Rgba(0, 0, 0, 0.45f * Alpha)));
	C.FillCircle(X, Y, R, Paint::Radial({X - R * 0.3f, Y - R * 0.35f}, 0.0f, {X, Y}, R, Rgba(250, 247, 238, Alpha), 0.7f, Rgba(226, 220, 205, Alpha), Rgba(190, 182, 164, Alpha)));
	C.StrokeEllipse(X, Y, R, R, Rgba(30, 32, 38, Alpha), 3.0f);
	for (int K = 0; K < 12; ++K)
	{
		const float A = Fl(K) * 0.5235988f;
		const float In = K % 3 == 0 ? 0.74f : 0.84f;
		C.StrokePolyline({{X + std::sin(A) * R * In, Y - std::cos(A) * R * In}, {X + std::sin(A) * R * 0.92f, Y - std::cos(A) * R * 0.92f}}, false, Rgba(30, 32, 38, Alpha), K % 3 == 0 ? 3.0f : 1.5f);
	}
	const double InDay = World - std::floor(World / 1440.0) * 1440.0;
	const float Ha = static_cast<float>(std::fmod(InDay / 60.0, 12.0) / 12.0 * 6.2831853);
	const float Ma = static_cast<float>(std::fmod(InDay, 60.0) / 60.0 * 6.2831853);
	C.StrokePolyline({{X, Y}, {X + std::sin(Ha) * R * 0.5f, Y - std::cos(Ha) * R * 0.5f}}, false, Rgba(20, 22, 26, Alpha), 4.5f, true);
	C.StrokePolyline({{X, Y}, {X + std::sin(Ma) * R * 0.78f, Y - std::cos(Ma) * R * 0.78f}}, false, Rgba(20, 22, 26, Alpha), 3.0f, true);
	C.FillCircle(X, Y, 4.0f, Paint(Rgba(215, 38, 61, Alpha)));
}
} // namespace streethud_detail

void DrawStreetHud(Canvas& C, const StreetHudInfo& Info, double Now)
{
	using namespace frontend_detail;
	using namespace streethud_detail;
	const float W = C.Width();
	const float H = StoreCounter::Height;
	const bool Skipping = Info.Skip.Active;
	// A service or a wait playing out: the cinematic's bars first, so the rest reads over them.
	if (Skipping)
	{
		C.FillRect({0.0f, 0.0f, W, 92.0f}, Paint(Rgba(0, 0, 0, 0.88f)));
		C.FillRect({0.0f, H - 112.0f, W, 112.0f}, Paint(Rgba(0, 0, 0, 0.88f)));
	}
	// Where and when, top left, over a soft shade so it reads on a bright window.
	C.FillRect({0.0f, 0.0f, 760.0f, 260.0f}, Paint::Radial({0.0f, 0.0f}, 0.0f, {0.0f, 0.0f}, 700.0f, Rgba(0, 0, 0, 0.55f), 0.5f, Rgba(0, 0, 0, 0.25f), Rgba(0, 0, 0, 0.0f)));
	C.FillRect({58.0f, 52.0f, 34.0f, 3.0f}, Paint(MenuNeon));
	TrackedText(C, Info.Place, 104.0f, 60.0f, 15.0f, 800, MenuInk, 5.0f);
	const float Cw = TrackedText(C, Info.Clock, 58.0f, 104.0f, 36.0f, 900, MenuInk, 2.0f);
	const float Mw = C.Text(Money(Info.BankrollCents), 58.0f + Cw + 22.0f, 102.0f, Ts(20.0f, 800, MenuGold));
	// The weather by the clock.
	if (Info.Weather >= 0)
	{
		const double InDay = Info.World >= 0.0 ? Info.World - std::floor(Info.World / 1440.0) * 1440.0 : 0.0;
		const bool Night = Info.World >= 0.0 && (InDay < 6.0 * 60.0 || InDay >= 19.5 * 60.0);
		const float Gx = 58.0f + Cw + 22.0f + Mw + 30.0f;
		WeatherGlyph(C, Info.Weather, Night, Gx, 93.0f, 30.0f, Hex(0xc9d2de), Now);
		const char* Word = weather::Word(static_cast<weather::Sky>(std::clamp(Info.Weather, 0, 4)));
		C.Text(Word, Gx + 22.0f, 100.0f, Ts(15.0f, 700, Dim(MenuInk, 0.8f)));
	}
	if (Info.Life)
	{
		DrawVitals(C, *Info.Life, 58.0f, 128.0f, 380.0f, Now, nullptr, 0.95f);
		// An order on its way (Penny Drop): how long until the soonest one is at the door, so the player knows to head home.
		if (Info.World >= 0.0 && !Info.Life->Deliveries.empty())
		{
			double Soonest = Info.Life->Deliveries.front().ArriveAt;
			for (const life::State::Delivery& Dv : Info.Life->Deliveries)
			{
				Soonest = std::min(Soonest, Dv.ArriveAt);
			}
			const int Mins = static_cast<int>(std::ceil(std::max(0.0, Soonest - Info.World)));
			const std::string Label = "PENNY DROP \xC2\xB7 " + (Mins <= 1 ? std::string("ANY MINUTE") : std::to_string(Mins) + " MIN");
			const float Lw = TrackedWidth(C, Label, 11.0f, 800, 2.4f);
			const Rect Chip{58.0f, 238.0f, Lw + 40.0f, 26.0f};
			C.FillRoundRect(Chip, 13.0f, Paint(Rgba(215, 38, 61, 0.88f)));
			const float Beat = 0.55f + 0.45f * static_cast<float>(std::sin(Now * 3.0));
			C.FillCircle(Chip.X + 15.0f, Chip.Y + 13.0f, 4.0f, Paint(Rgba(255, 255, 255, Beat)));
			TrackedText(C, Label, Chip.X + 28.0f, Chip.Y + 17.5f, 11.0f, 800, Hex(0xffffff), 2.4f);
		}
	}
	// A sponsor's duty the hero is due at (an appearance window): the chip under the clock, counting down to it, then
	// what's left of it. Gone once the window has closed.
	if (!Skipping && !Info.Due.Label.empty() && Info.World >= 0.0 && Info.World < Info.Due.To)
	{
		const bool On = Info.World >= Info.Due.From;
		const int Mins = static_cast<int>(std::ceil(std::max(0.0, (On ? Info.Due.To : Info.Due.From) - Info.World)));
		std::string When;
		if (On)
		{
			When = "NOW \xC2\xB7 " + (Mins >= 60 ? std::to_string(Mins / 60) + " HR " + std::to_string(Mins % 60) + " MIN LEFT" : std::to_string(Mins) + " MIN LEFT");
		}
		else
		{
			When = Mins < 60 ? "IN " + std::to_string(std::max(1, Mins)) + " MIN" : "AT " + net::TimeLabel(Info.Due.From);
		}
		std::string Up = Info.Due.Label;
		std::transform(Up.begin(), Up.end(), Up.begin(), [](char Ch) { return static_cast<char>(Ch >= 'a' && Ch <= 'z' ? Ch - 32 : Ch); });
		const std::string Label = Up + " \xC2\xB7 " + When;
		const float MaxW = std::min(620.0f, W * 0.5f);
		const float Lw = std::min(MaxW, TrackedWidth(C, Label, 13.0f, 800, 1.8f));
		const bool Penny = Info.Life && !Info.Life->Deliveries.empty();
		const Rect Chip{58.0f, Penny ? 272.0f : 238.0f, Lw + 40.0f, 28.0f};
		C.FillRoundRect(Chip, 14.0f, Paint(On ? Hex(0xf6b04a, 0.92f) : Rgba(8, 10, 16, 0.82f)));
		if (!On)
		{
			C.StrokeRoundRect({Chip.X + 0.5f, Chip.Y + 0.5f, Chip.W - 1.0f, Chip.H - 1.0f}, 14.0f, Hex(0xf6b04a, 0.7f), 1.0f);
		}
		const Color Ink = On ? Hex(0x1a1206) : Hex(0xf6b04a);
		C.FillCircle(Chip.X + 15.0f, Chip.Y + 14.0f, 4.0f, Paint(Ink));
		if (TrackedWidth(C, Label, 13.0f, 800, 1.8f) <= MaxW)
		{
			TrackedText(C, Label, Chip.X + 28.0f, Chip.Y + 19.0f, 13.0f, 800, Ink, 1.8f);
		}
		else
		{
			C.Text(Label, Chip.X + 28.0f, Chip.Y + 19.0f, Ts(13.0f, 800, Ink, Align::Left, Baseline::Alphabetic, false, Lw));
		}
	}
	// The camera, top right.
	const std::string Mode = Info.FirstPerson ? "FIRST PERSON" : "THIRD PERSON";
	const float ModeW = TrackedWidth(C, Mode, 12.0f, 800, 3.0f);
	if (!Skipping)
	{
		Glyph(C, Info.Gamepad ? "Y" : "V", W - 58.0f - ModeW - 46.0f, 66.0f, 0.85f);
		TrackedText(C, Mode, W - 58.0f, 62.0f, 12.0f, 800, Dim(MenuInk, 0.8f), 3.0f, Align::Right);
	}
	// Texts, top right under the camera chip: newest first, each for eight seconds.
	float Ty = 104.0f;
	for (auto It = Info.Toasts.rbegin(); It != Info.Toasts.rend(); ++It)
	{
		const double Age = Now - It->At;
		if (Age < 0.0 || Age > 8.0)
		{
			continue;
		}
		const float A = std::min(Ease(Age / 0.35), Ease((8.0 - Age) / 0.6));
		const float Tw = 420.0f;
		const Rect R{W - 58.0f - Tw + (1.0f - A) * 40.0f, Ty, Tw, 0.0f};
		const std::vector<std::string> Lines = WrapText(C, It->Body, Tw - 40.0f, 16.0f, 400);
		const float Th = 52.0f + Fl(static_cast<int>(std::min<size_t>(Lines.size(), 3))) * 22.0f;
		C.FillRoundRect({R.X, R.Y, Tw, Th}, 12.0f, Paint(Rgba(10, 13, 22, 0.86f * A)));
		C.FillRoundRect({R.X, R.Y, 4.0f, Th}, 2.0f, Paint(Dim(MenuTeal, A)));
		TrackedText(C, It->From, R.X + 20.0f, R.Y + 28.0f, 12.0f, 800, Dim(MenuTeal, A), 2.4f);
		float Ly = R.Y + 30.0f;
		for (size_t I = 0; I < Lines.size() && I < 3; ++I)
		{
			Ly += 22.0f;
			C.Text(Lines[I], R.X + 20.0f, Ly, Ts(16.0f, 400, Dim(MenuInk, 0.92f * A)));
		}
		Ty += Th + 10.0f;
	}
	// The name over whoever the hero is looking at.
	if (!Skipping && !Info.NameTag.Text.empty() && Info.NameTag.Alpha > 0.01f)
	{
		const float A = std::clamp(Info.NameTag.Alpha, 0.0f, 1.0f);
		std::string Sub = Info.NameTag.Sub; // "Knows you", "Barber": in the HUD's small caps
		std::transform(Sub.begin(), Sub.end(), Sub.begin(), [](char Ch) { return static_cast<char>(Ch >= 'a' && Ch <= 'z' ? Ch - 32 : Ch); });
		const float Nw = C.Measure(Info.NameTag.Text, 17.0f, 800);
		const float Sw = Sub.empty() ? 0.0f : TrackedWidth(C, Sub, 11.0f, 800, 2.2f);
		const float Pw = Nw + (Sub.empty() ? 0.0f : Sw + 26.0f) + 32.0f;
		const float Px = std::clamp(Info.NameTag.X - Pw * 0.5f, 20.0f, W - 20.0f - Pw);
		const float Py = std::clamp(Info.NameTag.Y - 52.0f, 20.0f, H - 80.0f);
		C.FillRoundRect({Px, Py, Pw, 38.0f}, 19.0f, Paint(Rgba(8, 10, 16, 0.78f * A)));
		C.FillPolygon({{Info.NameTag.X - 7.0f, Py + 38.0f}, {Info.NameTag.X + 7.0f, Py + 38.0f}, {Info.NameTag.X, Py + 46.0f}}, Paint(Rgba(8, 10, 16, 0.78f * A)));
		C.Text(Info.NameTag.Text, Px + 16.0f, Py + 25.0f, Ts(17.0f, 800, Dim(MenuInk, A)));
		if (!Sub.empty())
		{
			C.FillCircle(Px + 16.0f + Nw + 13.0f, Py + 19.0f, 2.5f, Paint(Dim(MenuMuted, A)));
			TrackedText(C, Sub, Px + 16.0f + Nw + 26.0f, Py + 24.0f, 11.0f, 800, Dim(MenuTeal, A), 2.2f);
		}
	}
	// A locked door's line shows over the prompt for a few seconds after the handle was tried; the subtitles stack above it.
	const double ClosedAge = Now - Info.ClosedAt;
	const bool ClosedShown = !Skipping && !Info.Closed.empty() && ClosedAge >= 0.0 && ClosedAge < 4.5;
	const float ClosedY = Info.Prompt.empty() ? H - 210.0f : H - 278.0f;
	// What people say: the two newest lines, the newest at the bottom, over the prompt (and the door's line), drawn the
	// way the card room's captions are (ui::DrawCaptions, with the player's caption settings; the street keeps its
	// two-box cap). Captions off: none.
	if (Info.CaptionLook.Mode > 0)
	{
		std::vector<Caption> Lines;
		for (const StreetHudInfo::Subtitle& Sb : Info.Subtitles)
		{
			Caption Cp;
			Cp.Speaker = Sb.Speaker;
			Cp.Line = Sb.Line;
			Cp.At = Sb.At;
			Lines.push_back(Cp);
		}
		CaptionStyle Look = Info.CaptionLook;
		Look.MaxShown = 2;
		const float By = Skipping ? H - 140.0f : ClosedShown ? ClosedY - 14.0f : !Info.Prompt.empty() ? H - 236.0f : H - 140.0f;
		DrawCaptions(C, Lines, Look, By, std::min(980.0f, W - 160.0f), Now);
	}
	// The locked door's line.
	if (ClosedShown)
	{
		const float A = std::min(Ease(ClosedAge / 0.2), Ease((4.5 - ClosedAge) / 0.6));
		const float Size = 16.0f;
		float Lw = TrackedWidth(C, Info.Closed, Size, 800, 2.4f);
		const float MaxW = W - 200.0f;
		const float Fit = Lw > MaxW ? MaxW / Lw : 1.0f;
		Lw *= Fit;
		const float Pw = Lw + 84.0f;
		const float Px = W * 0.5f - Pw * 0.5f;
		const float Py = ClosedY;
		C.FillRoundRect({Px, Py, Pw, 52.0f}, 26.0f, Paint(Rgba(40, 10, 14, 0.85f * A)));
		C.StrokeRoundRect({Px + 0.5f, Py + 0.5f, Pw - 1.0f, 51.0f}, 26.0f, Dim(MenuWarn, 0.55f * A), 1.0f);
		Padlock(C, Px + 32.0f, Py + 26.0f, 26.0f, Dim(MenuWarn, A));
		TrackedText(C, Info.Closed, Px + 60.0f, Py + 32.0f, Size * Fit, 800, Dim(MenuInk, A), 2.4f * Fit);
	}
	// The prompt, bottom center.
	if (!Skipping && !Info.Prompt.empty())
	{
		const float Pw = TrackedWidth(C, Info.Prompt, 15.0f, 800, 2.6f) + 92.0f;
		const float Px = W * 0.5f - Pw * 0.5f;
		const float Py = H - 210.0f;
		C.FillRoundRect({Px, Py, Pw, 52.0f}, 26.0f, Paint(Rgba(8, 10, 16, 0.78f)));
		C.StrokeRoundRect({Px + 0.5f, Py + 0.5f, Pw - 1.0f, 51.0f}, 26.0f, Dim(MenuInk, 0.18f), 1.0f);
		Glyph(C, Info.Gamepad ? "A" : Info.PromptKey, Px + 14.0f, Py + 38.0f, 1.0f);
		TrackedText(C, Info.Prompt, Px + 62.0f, Py + 32.0f, 15.0f, 800, MenuInk, 2.6f);
	}
	// The controls, for the first while.
	const double Since = Now - Info.HintsAt;
	if (!Skipping && Since >= 0.0 && Since < 14.0)
	{
		const float A = std::min(Ease(Since / 0.6), Ease((14.0 - Since) / 1.0));
		float Hx = 58.0f;
		const float Hy = H - 62.0f;
		const std::pair<const char*, const char*> Hints[6] = {{Info.Gamepad ? "LS" : "WASD", "MOVE"}, {Info.Gamepad ? "LB" : "SHIFT", "RUN"}, {Info.Gamepad ? "Y" : "V", "1ST / 3RD PERSON"},
			{Info.Gamepad ? "A" : "E", "INTERACT"}, {Info.Gamepad ? "X" : "F", "EAT OR DRINK"}, {Info.Gamepad ? "B" : "ESC", "PAUSE"}};
		for (const std::pair<const char*, const char*>& Hp : Hints)
		{
			Hx += Glyph(C, Hp.first, Hx, Hy, A) + 10.0f;
			Hx += TrackedText(C, Hp.second, Hx, Hy - 2.0f, 13.0f, 700, Dim(MenuMuted, A), 2.6f) + 30.0f;
		}
	}
	// The skip's overlay: what's happening, how far along, and the clock racing in the corner.
	if (Skipping)
	{
		const float P = std::clamp(Info.Skip.Progress, 0.0f, 1.0f);
		const double At = Info.Skip.From + (Info.Skip.To - Info.Skip.From) * static_cast<double>(P);
		const std::string Label = Info.Skip.Label.empty() ? std::string("TIME PASSES") : Info.Skip.Label;
		std::string Up = Label;
		std::transform(Up.begin(), Up.end(), Up.begin(), [](char Ch) { return static_cast<char>(Ch >= 'a' && Ch <= 'z' ? Ch - 32 : Ch); });
		const float BarY = H - 56.0f;
		TrackedText(C, Up, 58.0f, BarY - 14.0f, 15.0f, 800, MenuInk, 4.0f);
		const float Bw = std::min(520.0f, W * 0.3f);
		C.FillRoundRect({58.0f, BarY, Bw, 4.0f}, 2.0f, Paint(Dim(MenuInk, 0.15f)));
		C.FillRoundRect({58.0f, BarY, std::max(4.0f, Bw * P), 4.0f}, 2.0f, Paint(MenuNeon));
		// "40 min to go", "2 hr 15 min to go": an eight-hour sit at Ruby's isn't "480 min".
		const int Left = static_cast<int>(std::ceil(std::max(0.0, Info.Skip.To - At)));
		const std::string Togo = Left >= 60 ? std::to_string(Left / 60) + " hr" + (Left % 60 > 0 ? " " + std::to_string(Left % 60) + " min" : std::string()) : std::to_string(Left) + " min";
		C.Text(Left > 0 ? Togo + " to go" : std::string("Done"), 58.0f + Bw + 18.0f, BarY + 6.0f, Ts(13.0f, 600, MenuMuted));
		SkipClock(C, W - 124.0f, H - 160.0f, 52.0f, At, 1.0f);
		const std::string Time = net::TimeLabel(At);
		C.Text(Time, W - 124.0f, H - 74.0f, Ts(18.0f, 800, MenuInk, Align::Center));
	}
	if (Info.Fade > 0.0f)
	{
		C.FillRect({0.0f, 0.0f, W, H}, Paint(Rgba(0, 0, 0, std::min(1.0f, Info.Fade))));
	}
}
} // namespace ui
} // namespace ss
