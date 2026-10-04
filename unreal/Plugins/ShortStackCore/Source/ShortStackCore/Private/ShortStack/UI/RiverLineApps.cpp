// The laptop's other apps: ShiftLink (gig shifts), Burner (Marcus and Sam), the bank (rent and history),
// sleep, and the time-lapse while a shift or a night goes by. They share the screen with RiverLine through
// the taskbar along the bottom.
#include "ShortStack/UI/RiverLine.h"
#include "../StrictFloat.h"
#include "RiverLineShared.h"

#include "ShortStack/Game/Chat.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Live.h"

#include <algorithm>
#include <cmath>

namespace ss
{
namespace ui
{
using namespace rlnet_detail;

namespace rlapps_detail
{
const float AppH = 962.0f; // above the taskbar

struct AppBrand
{
	const char* Name;
	uint32_t Col;
	uint32_t Col2;
};

AppBrand BrandOf(RiverLine::App A)
{
	switch (A)
	{
	case RiverLine::App::ShiftLink: return {"ShiftLink", 0xff8a1f, 0xff5a1f};
	case RiverLine::App::Burner: return {"Burner", 0x2fd27a, 0x0f8a4a};
	case RiverLine::App::Bank: return {"Bank", 0x3b82f6, 0x1d4ed8};
	case RiverLine::App::GearDrop: return {"GearDrop", 0xff6b2c, 0xf43f5e};
	case RiverLine::App::Kast: return {"Kast", 0xb07cff, 0x6d28d9};
	case RiverLine::App::PennyDrop: return {"Penny Drop", 0xe23a4f, 0xa3122a};
	default: return {"RiverLine", 0x27d3c3, 0x1a8fd8};
	}
}

/** Time of day only ("2:07 AM") from world minutes. */
std::string AppClock(double World)
{
	return net::TimeLabel(World);
}

std::string AppDay(double World)
{
	return std::string(net::WeekdayName(net::DayOf(World))) + ", " + net::DateLabel(net::DayOf(World));
}

Color KindColor(int Kind)
{
	switch (Kind)
	{
	case 0: return Hex(0x27d3c3);
	case 1: return Hex(0xff8a1f);
	case 2: return Hex(0x2fd27a);
	case 3: return Hex(0xef4d5a);
	case 5: return Hex(0xff6b2c);
	case 6: return Hex(0x9b5cff);
	case 7: return Hex(0x60a5fa);
	case 8: return Hex(0xe23b4e);
	default: return Hex(0xf2c14e);
	}
}

const char* KindName(int Kind)
{
	switch (Kind)
	{
	case 0: return "RiverLine";
	case 1: return "ShiftLink";
	case 2: return "Cash";
	case 3: return "Bills";
	case 5: return "GearDrop";
	case 6: return "Kast";
	case 7: return "Transfer";
	case 8: return "Corner store";
	default: return "Prizes";
	}
}

/** life::State::Record keeps this many ledger lines (the newest). */
const size_t LedgerKept = 60;
/** The Embercrest's ember, for its lines in the Bank. */
const uint32_t EmberRgb = 0xff6a24;

/** A ledger line from the Embercrest (a live buy-in, fee, prize or refund): its label starts with the event's name. */
bool LiveLine(const life::State& L, const std::string& Label)
{
	for (const life::LiveEntry& E : L.LiveEntries)
	{
		if (!E.Name.empty() && Label.size() > E.Name.size() + 2 && Label.compare(0, E.Name.size(), E.Name) == 0 && Label.compare(E.Name.size(), 2, ": ") == 0)
		{
			return true;
		}
	}
	return false;
}

/** "LP" for Lucky Penny #212: the first letters of the first two words. */
std::string AppInitials(const std::string& Name)
{
	std::string Out;
	bool Start = true;
	for (const char Ch : Name)
	{
		const bool Letter = (Ch >= 'A' && Ch <= 'Z') || (Ch >= 'a' && Ch <= 'z');
		if (Letter && Start && Out.size() < 2)
		{
			Out.push_back(Ch >= 'a' && Ch <= 'z' ? static_cast<char>(Ch - 'a' + 'A') : Ch);
		}
		Start = Ch == ' ' || Ch == '&';
	}
	return Out;
}

/** Moon glyph: a disc with a bite out of it in the background color. */
void AppMoon(Canvas& Cv, float Cx, float Cy, float R, const Color& Col, const Color& Bg)
{
	Cv.FillCircle(Cx, Cy, R, Col);
	Cv.FillCircle(Cx + R * 0.45f, Cy - R * 0.3f, R * 0.85f, Bg);
}

void AppSun(Canvas& Cv, float Cx, float Cy, float R, const Color& Col)
{
	Cv.FillCircle(Cx, Cy, R * 0.55f, Col);
	for (int K = 0; K < 8; ++K)
	{
		const float A = Nf(K) * Pi / 4.0f;
		Cv.StrokePolyline({{Cx + std::cos(A) * R * 0.75f, Cy + std::sin(A) * R * 0.75f}, {Cx + std::cos(A) * R, Cy + std::sin(A) * R}}, false, Col, R * 0.12f, true);
	}
}

void AppFlame(Canvas& Cv, float Cx, float Base, float H, const Color& Col)
{
	std::vector<Vec2> P;
	for (int I = 0; I <= 16; ++I)
	{
		const float T = Nf(I) / 16.0f;
		const float A = T * Pi * 2.0f;
		// A teardrop: round at the bottom, pointed at the top.
		const float R = H * 0.32f * (1.0f - 0.55f * (0.5f - 0.5f * std::cos(A)));
		P.push_back({Cx + std::sin(A) * R, Base - H * 0.32f - std::cos(A) * H * 0.5f * (std::cos(A) > 0.0f ? 1.0f : 0.62f)});
	}
	Cv.FillPolygon(P, Col);
}
} // namespace rlapps_detail

using namespace rlapps_detail;

void RiverLine::OpenApp(App A, double Now)
{
	if (A != AppShown)
	{
		AppAt = Now;
	}
	AppShown = A;
	SleepOpen = false;
}

void RiverLine::TryActivity(const std::string& Id, double Now)
{
	const std::string Why = S.StartActivity(Id);
	if (!Why.empty())
	{
		Toast = Why;
		ToastAt = Now;
	}
	SleepOpen = false;
}

// ------------------------------------------------------------------ chrome

void RiverLine::AppIcon(App A, float X, float Y, float Sz)
{
	const AppBrand B = BrandOf(A);
	const Rect R{X, Y, Sz, Sz};
	C->FillRoundRect(R, Sz * 0.26f, Paint::Linear({X, Y}, {X + Sz, Y + Sz}, Hex(B.Col), Hex(B.Col2)));
	const Color W = Hex(0xffffff);
	const float Cx = X + Sz / 2.0f;
	const float Cy = Y + Sz / 2.0f;
	switch (A)
	{
	case App::RiverLine:
		for (int K = 0; K < 2; ++K)
		{
			std::vector<Vec2> Wave;
			for (int I = 0; I <= 12; ++I)
			{
				const float T = Nf(I) / 12.0f;
				Wave.push_back({X + Sz * (0.18f + 0.64f * T), Y + Sz * (0.4f + 0.2f * Nf(K)) + std::sin(T * Pi * 2.0f) * Sz * 0.08f});
			}
			C->StrokePolyline(Wave, false, NetA(W, K == 0 ? 1.0f : 0.6f), Sz * 0.09f, true);
		}
		break;
	case App::ShiftLink:
		C->StrokeRoundRect({Cx - Sz * 0.12f, Cy - Sz * 0.27f, Sz * 0.24f, Sz * 0.16f}, Sz * 0.05f, W, Sz * 0.07f);
		C->FillRoundRect({Cx - Sz * 0.3f, Cy - Sz * 0.14f, Sz * 0.6f, Sz * 0.4f}, Sz * 0.07f, W);
		C->FillRect({Cx - Sz * 0.3f, Cy + Sz * 0.02f, Sz * 0.6f, Sz * 0.04f}, Hex(B.Col2));
		break;
	case App::Burner:
		C->FillRoundRect({Cx - Sz * 0.3f, Cy - Sz * 0.24f, Sz * 0.6f, Sz * 0.4f}, Sz * 0.12f, W);
		C->FillPolygon({{Cx - Sz * 0.16f, Cy + Sz * 0.14f}, {Cx - Sz * 0.22f, Cy + Sz * 0.3f}, {Cx - Sz * 0.02f, Cy + Sz * 0.14f}}, W);
		for (int K = 0; K < 3; ++K)
		{
			C->FillCircle(Cx - Sz * 0.13f + Nf(K) * Sz * 0.13f, Cy - Sz * 0.04f, Sz * 0.045f, Hex(B.Col2));
		}
		break;
	case App::Bank:
		C->FillPolygon({{Cx - Sz * 0.32f, Cy - Sz * 0.1f}, {Cx, Cy - Sz * 0.3f}, {Cx + Sz * 0.32f, Cy - Sz * 0.1f}}, W);
		for (int K = 0; K < 3; ++K)
		{
			C->FillRect({Cx - Sz * 0.22f + Nf(K) * Sz * 0.18f, Cy - Sz * 0.06f, Sz * 0.08f, Sz * 0.24f}, W);
		}
		C->FillRect({Cx - Sz * 0.32f, Cy + Sz * 0.2f, Sz * 0.64f, Sz * 0.07f}, W);
		break;
	case App::GearDrop:
		C->FillPolygon({{Cx - Sz * 0.28f, Cy - Sz * 0.12f}, {Cx, Cy - Sz * 0.26f}, {Cx + Sz * 0.28f, Cy - Sz * 0.12f}, {Cx + Sz * 0.28f, Cy + Sz * 0.18f}, {Cx, Cy + Sz * 0.3f}, {Cx - Sz * 0.28f, Cy + Sz * 0.18f}}, W);
		C->StrokePolyline({{Cx, Cy - Sz * 0.18f}, {Cx, Cy + Sz * 0.12f}}, false, Hex(B.Col2), Sz * 0.07f, true);
		C->StrokePolyline({{Cx - Sz * 0.1f, Cy + Sz * 0.02f}, {Cx, Cy + Sz * 0.13f}, {Cx + Sz * 0.1f, Cy + Sz * 0.02f}}, false, Hex(B.Col2), Sz * 0.07f, true);
		break;
	case App::Kast:
		C->FillPolygon({{Cx - Sz * 0.14f, Cy - Sz * 0.22f}, {Cx + Sz * 0.24f, Cy}, {Cx - Sz * 0.14f, Cy + Sz * 0.22f}}, Hex(0xc6f432));
		C->StrokeArc(Cx - Sz * 0.14f, Cy, Sz * 0.44f, -0.6f, 0.6f, NetA(W, 0.6f), Sz * 0.06f, true);
		break;
	case App::PennyDrop:
		// A penny with speed lines.
		for (int K = 0; K < 3; ++K)
		{
			const float Ly = Cy - Sz * 0.12f + Nf(K) * Sz * 0.12f;
			C->FillRoundRect({X + Sz * 0.12f, Ly - Sz * 0.025f, Sz * (0.18f - 0.04f * Nf(K % 2)), Sz * 0.05f}, Sz * 0.025f, NetA(W, 0.75f));
		}
		C->FillCircle(Cx + Sz * 0.1f, Cy, Sz * 0.27f, Hex(0xe0a15a));
		C->StrokeEllipse(Cx + Sz * 0.1f, Cy, Sz * 0.21f, Sz * 0.21f, Hex(0x9a5b22), Sz * 0.05f);
		C->FillCircle(Cx + Sz * 0.1f, Cy, Sz * 0.08f, Hex(0x9a5b22));
		break;
	}
}

bool RiverLine::AppButton(const std::string& Id, const Rect& R, const std::string& Label, const Color& Fill, const Color& Ink, bool Enabled, const std::string& Sub)
{
	const Ui::ClickState St = UI.Clickable(Id, R, Enabled);
	const float Shift = St.Down ? 2.0f : 0.0f;
	const Rect B{R.X, R.Y + Shift, R.W, R.H};
	if (Enabled)
	{
		if (St.Hover)
		{
			C->GlowRoundRect(B, 12.0f, NetA(Fill, 0.35f), 14.0f);
		}
		C->FillRoundRect(B, 12.0f, Paint::Linear({0.0f, B.Y}, {0.0f, B.Y + B.H}, St.Hover ? Mix(Fill, Hex(0xffffff), 0.12f) : Fill, Mix(Fill, Hex(0x000000), 0.12f)));
	}
	else
	{
		C->FillRoundRect(B, 12.0f, Rgba(128, 128, 128, 0.16f));
		C->StrokeRoundRect(B, 12.0f, Rgba(128, 128, 128, 0.3f), 1.0f);
	}
	const Color Text = Enabled ? Ink : Rgba(128, 128, 128, 0.95f);
	const float Cy = B.Y + B.H / 2.0f;
	if (Sub.empty())
	{
		UI.Text(Label, B.X + B.W / 2.0f, Cy + 1.0f, Ts(17.0f, 800, Text, Align::Center, Baseline::Middle, false, B.W - 24.0f));
	}
	else
	{
		UI.Text(Label, B.X + B.W / 2.0f, Cy - 3.0f, Ts(17.0f, 800, Text, Align::Center, Baseline::Alphabetic, false, B.W - 24.0f));
		UI.Text(Sub, B.X + B.W / 2.0f, Cy + 15.0f, Ts(12.5f, 600, NetA(Text, 0.85f), Align::Center, Baseline::Alphabetic, false, B.W - 24.0f));
	}
	return St.Clicked;
}

void RiverLine::Taskbar(const Rect& R, double Now)
{
	C->FillRect(R, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, Hex(0x0c1422), Hex(0x060a12)));
	C->FillRect({R.X, R.Y, R.W, 1.0f}, Rgba(255, 255, 255, 0.08f));
	const life::Context Ctx = S.LifeContext();
	const App Apps[7] = {App::RiverLine, App::ShiftLink, App::Burner, App::Bank, App::GearDrop, App::Kast, App::PennyDrop};
	float X = R.X + 8.0f;
	for (const App A : Apps)
	{
		const AppBrand B = BrandOf(A);
		const float W = 40.0f + UI.Measure(B.Name, 13.0f, 700) + 12.0f;
		const Rect Btn{X, R.Y + 4.0f, W, R.H - 8.0f};
		const Ui::ClickState St = UI.Clickable(std::string("app") + B.Name, Btn);
		if (St.Clicked)
		{
			OpenApp(A, Now);
		}
		const bool On = AppShown == A;
		if (On || St.Hover)
		{
			UI.RRect(Btn, 8.0f, Rgba(255, 255, 255, On ? 0.09f : 0.05f));
		}
		AppIcon(A, Btn.X + 6.0f, Btn.Y + 3.0f, 24.0f);
		UI.Text(B.Name, Btn.X + 38.0f, Btn.Y + Btn.H / 2.0f + 1.0f, Ts(13.0f, 700, On ? pal::Ink : Hex(0x9aa7bd), Align::Left, Baseline::Middle));
		if (On)
		{
			UI.RRect({Btn.X + 8.0f, R.Y + R.H - 3.0f, W - 16.0f, 2.0f}, 1.0f, Hex(B.Col));
		}
		// Badges.
		Color Dot{0.0f, 0.0f, 0.0f, 0.0f};
		if (A == App::RiverLine && S.HasPrompt && AppShown != App::RiverLine)
		{
			Dot = NetA(pal::Red, 0.6f + 0.4f * Nf(std::sin(Now * 6.0)));
		}
		if (A == App::Burner)
		{
			const life::Activity* Drop = life::Find("marcus-drop");
			if (S.Life.DebtCents > 0)
			{
				Dot = pal::Red;
			}
			else if (Drop && life::Blocked(*Drop, S.Life, Ctx).empty())
			{
				Dot = pal::Green;
			}
		}
		if (A == App::PennyDrop)
		{
			// Hungry or thirsty: orange, then red; an order on the way: gold.
			const double Need = std::max(S.Life.Hunger, S.Life.Thirst);
			if (Need >= 85.0)
			{
				Dot = NetA(pal::Red, 0.6f + 0.4f * Nf(std::sin(Now * 4.0)));
			}
			else if (Need >= 65.0)
			{
				Dot = pal::Orange;
			}
			else if (!S.Life.Deliveries.empty())
			{
				Dot = pal::Gold;
			}
		}
		if (A == App::Bank)
		{
			// Rent due within the day, or a final notice: red. Evicted: orange, red when the storage unit is about to
			// go, green once there's enough to move back in.
			const life::State& Lf = S.Life;
			if (S.Evicted())
			{
				Dot = S.BankrollCents >= Lf.RentDueCents ? pal::Green
					: Lf.StorageDue > 0.0 && Lf.StorageDue - World < 2.0 * net::MinutesPerDay && S.BankrollCents < life::StorageCents ? pal::Red
																														   : pal::Orange;
			}
			else if (Lf.RentStage == life::Rent::FinalNotice || (Lf.RentStage == life::Rent::Due && Lf.RentDeadline - World < 24.0 * 60.0))
			{
				Dot = pal::Red;
			}
		}
		if (A == App::Kast)
		{
			// Live: a pulsing red dot. A sponsor waiting: gold. Locked until the PC can stream: no dot, a padlock.
			if (S.Streaming())
			{
				Dot = NetA(pal::Red, 0.6f + 0.4f * Nf(std::sin(Now * 4.0)));
			}
			else if (!S.Channel.Offers.empty())
			{
				Dot = pal::Gold;
			}
			else if (!S.GearFx().CanStream())
			{
				NetLockIcon(*C, Btn.X + 24.0f, Btn.Y - 1.0f, 10.0f, Hex(0xd8ccff));
			}
		}
		if (Dot.A > 0.0f)
		{
			C->FillCircle(Btn.X + 29.0f, Btn.Y + 4.0f, 4.5f, Dot);
			C->StrokeEllipse(Btn.X + 29.0f, Btn.Y + 4.0f, 4.5f, 4.5f, Hex(0x060a12), 1.5f);
		}
		X += W + 4.0f;
	}
	// Sleep.
	{
		const float W = 40.0f + UI.Measure("Sleep", 13.0f, 700) + 12.0f;
		const Rect Btn{X, R.Y + 4.0f, W, R.H - 8.0f};
		const Ui::ClickState St = UI.Clickable("appSleep", Btn);
		if (St.Clicked)
		{
			SleepOpen = !SleepOpen;
		}
		if (SleepOpen || St.Hover)
		{
			UI.RRect(Btn, 8.0f, Rgba(255, 255, 255, SleepOpen ? 0.09f : 0.05f));
		}
		C->FillRoundRect({Btn.X + 6.0f, Btn.Y + 3.0f, 24.0f, 24.0f}, 6.0f, Paint::Linear({Btn.X, Btn.Y}, {Btn.X + 24.0f, Btn.Y + 24.0f}, Hex(0x8b5cf6), Hex(0x4c1d95)));
		AppMoon(*C, Btn.X + 17.0f, Btn.Y + 15.0f, 7.0f, Hex(0xffffff), Hex(0x6a3cc9));
		UI.Text("Sleep", Btn.X + 38.0f, Btn.Y + Btn.H / 2.0f + 1.0f, Ts(13.0f, 700, SleepOpen ? pal::Ink : Hex(0x9aa7bd), Align::Left, Baseline::Middle));
		X += W + 12.0f;
	}
	// Status on the right: energy, heat, the clock.
	const float Right = R.X + R.W - 12.0f;
	const float Tw = UI.Text(AppClock(World), Right, R.Y + R.H / 2.0f + 1.0f, Ts(14.0f, 700, pal::Ink, Align::Right, Baseline::Middle, true));
	const std::string Day = AppDay(World);
	const float Dw = UI.Text(Day, Right - Tw - 10.0f, R.Y + R.H / 2.0f + 1.0f, Ts(12.0f, 600, pal::Muted, Align::Right, Baseline::Middle));
	float Sx = Right - Tw - Dw - 30.0f;
	if (S.Life.Heat >= 1.0)
	{
		const std::string Ht = std::to_string(static_cast<int>(S.Life.Heat));
		const float Hw = UI.Text(Ht, Sx, R.Y + R.H / 2.0f + 1.0f, Ts(13.0f, 800, S.Life.Heat > 50.0 ? pal::Red : pal::Orange, Align::Right, Baseline::Middle, true));
		AppFlame(*C, Sx - Hw - 10.0f, R.Y + R.H / 2.0f + 8.0f, 17.0f, S.Life.Heat > 50.0 ? pal::Red : pal::Orange);
		Sx -= Hw + 30.0f;
	}
	const float E = Nf(S.Life.Energy / 100.0);
	const Color Ec = E > 0.5f ? pal::Green : E > 0.2f ? pal::Gold : pal::Red;
	const std::string Et = std::to_string(static_cast<int>(std::round(S.Life.Energy))) + "%";
	const float Ew = UI.Text(Et, Sx, R.Y + R.H / 2.0f + 1.0f, Ts(13.0f, 800, Ec, Align::Right, Baseline::Middle, true));
	const Rect Bat{Sx - Ew - 40.0f, R.Y + 12.0f, 28.0f, 14.0f};
	C->StrokeRoundRect(Bat, 3.0f, Rgba(255, 255, 255, 0.55f), 1.5f);
	C->FillRect({Bat.X + Bat.W + 1.0f, Bat.Y + 4.0f, 2.5f, 6.0f}, Rgba(255, 255, 255, 0.55f));
	C->FillRoundRect({Bat.X + 2.5f, Bat.Y + 2.5f, std::max(2.0f, (Bat.W - 5.0f) * E), Bat.H - 5.0f}, 1.5f, Ec);
	const float LaneEnd = Bat.X - 18.0f;
	// The middle: the news wire, or what's running in the background.
	const Rect Lane{X, R.Y, std::max(0.0f, LaneEnd - X), R.H};
	if (AppShown == App::RiverLine && S.CurrentScreen == Screen::Lobby && Lane.W > 120.0f)
	{
		Ticker(Lane, Now);
	}
	else if (S.T && AppShown != App::RiverLine && Lane.W > 120.0f)
	{
		C->FillCircle(Lane.X + 10.0f, R.Y + R.H / 2.0f, 4.0f, NetA(pal::Red, 0.6f + 0.4f * Nf(std::sin(Now * 4.0))));
		const int Waiting = S.TablesWaiting();
		const int Tables = S.TableCount();
		std::string Lane2 = Tables > 1 ? "Playing " + std::to_string(Tables) + " tables" : "Playing " + S.T->Spec.Name;
		if (Waiting > 1)
		{
			Lane2 = "Your turn at " + std::to_string(Waiting) + " tables  \xC2\xB7  the clocks are running";
		}
		else if (Waiting == 1)
		{
			std::string Where = S.T->Spec.Name;
			for (int I = 0; I < Tables; ++I)
			{
				const TableGlance G = S.Glance(I);
				Where = G.YourTurn ? G.Name : Where;
			}
			Lane2 = "Your turn at " + Where + "  \xC2\xB7  the clock is running";
		}
		UI.Text(Lane2, Lane.X + 22.0f, R.Y + R.H / 2.0f + 1.0f, Ts(13.0f, 700, Waiting > 0 ? pal::Red : Hex(0xb9c4d6), Align::Left, Baseline::Middle, false, Lane.W - 30.0f));
	}
	// Toast for something that can't be done.
	if (Now - ToastAt < 3.0 && !Toast.empty())
	{
		const float A0 = C->GetAlpha();
		C->SetAlpha(A0 * NetEase((ToastAt + 3.0 - Now) / 0.4));
		const float W = UI.Measure(Toast, 15.0f, 700) + 40.0f;
		const Rect Tr{NetW / 2.0f - W / 2.0f, R.Y - 58.0f, W, 40.0f};
		C->GlowRoundRect(Tr, 20.0f, Rgba(0, 0, 0, 0.5f), 16.0f);
		UI.RRect(Tr, 20.0f, Hex(0x2a1218), NetA(pal::Red, 0.7f));
		UI.Text(Toast, Tr.X + W / 2.0f, Tr.Y + 21.0f, Ts(15.0f, 700, Hex(0xffd7dc), Align::Center, Baseline::Middle));
		C->SetAlpha(A0);
	}
}

// ------------------------------------------------------------------ ShiftLink

void RiverLine::ShiftLinkApp(double Now)
{
	const life::Context Ctx = S.LifeContext();
	const life::State& L = S.Life;
	const Color Dark = Hex(0x1f1c18);
	const Color Gray = Hex(0x857e73);
	const Color ShiftOrange = Hex(0xff7a1f);
	C->FillRect({0.0f, 0.0f, NetW, AppH}, Hex(0xf3f0ea));
	// Header.
	C->FillRect({0.0f, 0.0f, NetW, 72.0f}, Hex(0xffffff));
	C->FillRect({0.0f, 72.0f, NetW, 1.0f}, Hex(0xe3ddd2));
	AppIcon(App::ShiftLink, 28.0f, 16.0f, 40.0f);
	const float Lw = UI.Text("ShiftLink", 80.0f, 45.0f, Ts(26.0f, 900, Dark));
	UI.Text("Shifts near you", 92.0f + Lw, 45.0f, Ts(15.0f, 500, Gray));
	UI.Text("Earned on ShiftLink", NetW - 200.0f, 30.0f, Ts(12.0f, 600, Gray, Align::Right));
	UI.Text(Money(L.EarnedJobs), NetW - 200.0f, 54.0f, Ts(20.0f, 900, Hex(0x16a34a), Align::Right, Baseline::Alphabetic, true));
	NetAvatar(*C, NetW - 56.0f, 36.0f, 20.0f, S.HeroName, Color{0.0f, 0.0f, 0.0f, 0.0f}, true);
	UI.Text(S.HeroName, NetW - 86.0f, 42.0f, Ts(15.0f, 700, Dark, Align::Right));

	// Jobs.
	std::vector<const life::Activity*> Jobs;
	for (const life::Activity& A : life::Catalog())
	{
		if (A.Type == life::Kind::Job)
		{
			Jobs.push_back(&A);
		}
	}
	int OpenNow = 0;
	for (const life::Activity* A : Jobs)
	{
		OpenNow += life::InWindow(*A, World) ? 1 : 0;
	}
	UI.Text("Open shifts", 40.0f, 128.0f, Ts(30.0f, 900, Dark));
	UI.Text(AppDay(World) + " \xC2\xB7 " + AppClock(World) + " \xC2\xB7 " + std::to_string(OpenNow) + " hiring right now", 40.0f, 156.0f, Ts(15.0f, 500, Gray));
	for (size_t I = 0; I < Jobs.size(); ++I)
	{
		const life::Activity& A = *Jobs[I];
		const float Ci = NetEase((Now - AppAt - 0.06 * static_cast<double>(I)) / 0.45);
		const Rect R{40.0f + Nf(I % 2) * 520.0f, 182.0f + Nf(I / 2) * 384.0f + (1.0f - Ci) * 24.0f, 500.0f, 364.0f};
		const float A0 = C->GetAlpha();
		C->SetAlpha(A0 * Ci);
		const Color Brand = Hex(A.Color);
		C->GlowRoundRect({R.X, R.Y + 4.0f, R.W, R.H}, 16.0f, Rgba(60, 40, 10, 0.1f), 16.0f);
		UI.RRect(R, 16.0f, Hex(0xffffff), Hex(0xe8e2d8));
		C->PushClip({R.X, R.Y, R.W, 8.0f});
		C->FillRoundRect({R.X, R.Y, R.W, 30.0f}, 16.0f, Brand);
		C->PopClip();
		C->FillCircle(R.X + 54.0f, R.Y + 66.0f, 28.0f, Paint::Linear({R.X + 26.0f, R.Y + 38.0f}, {R.X + 82.0f, R.Y + 94.0f}, Mix(Brand, Hex(0xffffff), 0.15f), Mix(Brand, Hex(0x000000), 0.2f)));
		UI.Text(AppInitials(A.Place), R.X + 54.0f, R.Y + 67.0f, Ts(18.0f, 900, A.Id == "dashdrop" ? Dark : Hex(0xffffff), Align::Center, Baseline::Middle));
		UI.Text(A.Title, R.X + 96.0f, R.Y + 60.0f, Ts(22.0f, 800, Dark, Align::Left, Baseline::Alphabetic, false, 250.0f));
		UI.Text(A.Place, R.X + 96.0f, R.Y + 84.0f, Ts(15.0f, 500, Gray));
		const bool Open = life::InWindow(A, World);
		const std::string Chip = Open ? "OPEN NOW" : "OPENS " + NetUpper(net::TimeLabel(life::NextOpen(A, World)));
		const float Cw = UI.Measure(Chip, 11.0f, 800) + 22.0f;
		UI.RRect({R.X + R.W - 24.0f - Cw, R.Y + 34.0f, Cw, 24.0f}, 12.0f, Open ? Hex(0xdcfce7) : Hex(0xefebe4));
		UI.Text(Chip, R.X + R.W - 24.0f - Cw / 2.0f, R.Y + 47.0f, Ts(11.0f, 800, Open ? Hex(0x15803d) : Gray, Align::Center, Baseline::Middle));
		// Pay.
		// The character's background can raise the rate (a line cook's), and the card says so.
		const double Rate = L.Perks.JobPay;
		auto Scaled = [Rate](Chips Cents) { return static_cast<Chips>(std::llround(static_cast<double>(Cents) * Rate)); };
		const float Pw = UI.Text(Money(Scaled(A.WageCents)), R.X + 28.0f, R.Y + 148.0f, Ts(36.0f, 900, Dark, Align::Left, Baseline::Alphabetic, true));
		const float Hw = UI.Text("/hr", R.X + 32.0f + Pw, R.Y + 148.0f, Ts(16.0f, 600, Gray));
		if (Rate > 1.0)
		{
			UI.Text("+" + std::to_string(static_cast<int>(std::lround((Rate - 1.0) * 100.0))) + "% cook's rate", R.X + 42.0f + Pw + Hw, R.Y + 148.0f, Ts(13.0f, 800, ShiftOrange));
		}
		UI.Text(std::to_string(static_cast<int>(A.Hours)) + "-hour shift", R.X + R.W - 28.0f, R.Y + 148.0f, Ts(16.0f, 700, Dark, Align::Right));
		const Chips Base = static_cast<Chips>(std::llround(static_cast<double>(A.WageCents) * A.Hours * Rate));
		const std::string Extra = A.PayMax <= 0 ? std::string() : A.PayMin > 0 ? " + " + NetMoney(Scaled(A.PayMin)) + "\xE2\x80\x93" + NetMoney(Scaled(A.PayMax)) + " in tips" : " + up to " + NetMoney(Scaled(A.PayMax)) + " extra";
		UI.Text("Est. " + Money(Base) + Extra, R.X + 28.0f, R.Y + 178.0f,
			Ts(16.0f, 700, Hex(0x16a34a)));
		UI.Text("Shifts start " + net::TimeLabel(static_cast<double>(A.Opens)) + " \xE2\x80\x93 " + net::TimeLabel(static_cast<double>(A.Closes)), R.X + 28.0f, R.Y + 204.0f, Ts(14.0f, 500, Gray));
		// Energy it takes.
		UI.Text("Energy", R.X + 28.0f, R.Y + 236.0f, Ts(13.0f, 600, Gray));
		UI.RRect({R.X + 90.0f, R.Y + 227.0f, 200.0f, 10.0f}, 5.0f, Hex(0xefebe4));
		UI.RRect({R.X + 90.0f, R.Y + 227.0f, 200.0f * Nf(A.Energy / 50.0), 10.0f}, 5.0f, Hex(0xf59e0b));
		UI.Text("\xE2\x88\x92" + std::to_string(static_cast<int>(A.Energy)), R.X + 302.0f, R.Y + 237.0f, Ts(13.0f, 800, Hex(0xd97706)));
		NetParagraph(*C, A.Blurb, R.X + 28.0f, R.Y + 268.0f, R.W - 56.0f, 14.0f, 500, Gray, 19.0f, 2);
		const std::string Why = life::Blocked(A, L, Ctx);
		const double Wake = World + A.Hours * 60.0;
		if (AppButton("job:" + A.Id, {R.X + 24.0f, R.Y + R.H - 66.0f, R.W - 48.0f, 48.0f}, Why.empty() ? "Take shift" : Why, ShiftOrange, Hex(0xffffff), Why.empty(),
				Why.empty() ? "Done at " + net::TimeLabel(Wake) : std::string()))
		{
			TryActivity(A.Id, Now);
		}
		C->SetAlpha(A0);
	}

	// This week.
	const Rect W{1090.0f, 104.0f, 470.0f, 420.0f};
	C->GlowRoundRect({W.X, W.Y + 4.0f, W.W, W.H}, 16.0f, Rgba(60, 40, 10, 0.08f), 16.0f);
	UI.RRect(W, 16.0f, Hex(0xffffff), Hex(0xe8e2d8));
	UI.Text("Your week", W.X + 28.0f, W.Y + 46.0f, Ts(22.0f, 900, Dark));
	const std::pair<std::string, std::string> Rows[3] = {{"Shifts worked", std::to_string(L.Shifts)}, {"Earned", Money(L.EarnedJobs)}, {"Rating", L.Shifts > 0 ? "4.9 \xE2\x98\x85" : "New"}};
	for (int I = 0; I < 3; ++I)
	{
		UI.Text(Rows[I].first, W.X + 28.0f, W.Y + 92.0f + Nf(I) * 34.0f, Ts(15.0f, 500, Gray));
		UI.Text(Rows[I].second, W.X + W.W - 28.0f, W.Y + 92.0f + Nf(I) * 34.0f, Ts(16.0f, 800, Dark, Align::Right));
	}
	C->FillRect({W.X + 28.0f, W.Y + 190.0f, W.W - 56.0f, 1.0f}, Hex(0xefebe4));
	UI.Text("Energy", W.X + 28.0f, W.Y + 232.0f, Ts(15.0f, 600, Gray));
	const float E = Nf(L.Energy / 100.0);
	const Color Ec = E > 0.5f ? Hex(0x16a34a) : E > 0.2f ? Hex(0xd97706) : Hex(0xdc2626);
	UI.Text(std::to_string(static_cast<int>(std::round(L.Energy))) + "%", W.X + W.W - 28.0f, W.Y + 236.0f, Ts(30.0f, 900, Ec, Align::Right, Baseline::Alphabetic, true));
	UI.RRect({W.X + 28.0f, W.Y + 254.0f, W.W - 56.0f, 12.0f}, 6.0f, Hex(0xefebe4));
	UI.RRect({W.X + 28.0f, W.Y + 254.0f, std::max(12.0f, (W.W - 56.0f) * E), 12.0f}, 6.0f, Ec);
	NetParagraph(*C,
		L.Energy < 25.0 ? "You're running on fumes. Sleep before the next shift, and before the next tournament: tired players think slower."
						: "Shifts cost energy, and so does staying up. Sleep from the taskbar to get it back.",
		W.X + 28.0f, W.Y + 300.0f, W.W - 56.0f, 14.0f, 500, Gray, 20.0f, 4);
	// The point of it all.
	const Rect Rr{1090.0f, 544.0f, 470.0f, 210.0f};
	UI.RRect(Rr, 16.0f, Hex(0x2a1a0e));
	const bool Out = S.Evicted();
	UI.Text(Out ? "To get your key back" : L.RentStage == life::Rent::Paid ? "Next rent" : "Rent", Rr.X + 28.0f, Rr.Y + 42.0f, Ts(14.0f, 700, Hex(0xffb36b)));
	UI.Text(Money(L.RentDueCents), Rr.X + 28.0f, Rr.Y + 88.0f, Ts(38.0f, 900, Hex(0xffffff), Align::Left, Baseline::Alphabetic, true));
	UI.Text(Out ? std::string("back rent + a month up front") : "due in " + net::Countdown(std::max(0.0, L.RentDeadline - World)), Rr.X + 28.0f, Rr.Y + 116.0f, Ts(15.0f, 600, Hex(0xf5d0a9)));
	const Chips Short = std::max<Chips>(0, L.RentDueCents - S.BankrollCents);
	const double Shifts = static_cast<double>(Short) / 4350.0;
	NetParagraph(*C,
		Short > 0 ? "You're " + Money(Short) + " short. That's about " + std::to_string(static_cast<int>(std::ceil(Shifts))) + " night shifts at the Lucky Penny."
		: Out     ? "You have it. Move back in from the Bank app."
				  : "You have the rent. Pay it from the Bank app.",
		Rr.X + 28.0f, Rr.Y + 152.0f, Rr.W - 56.0f, 14.0f, 500, Hex(0xf5d0a9), 20.0f, 2);
}

// ------------------------------------------------------------------ Burner

void RiverLine::BurnerApp(double Now)
{
	const life::Context Ctx = S.LifeContext();
	const life::State& L = S.Life;
	const Color BurnerGreen = Hex(0x2fd27a);
	const Color Text = Hex(0xd9f5e3);
	const Color Dim = Hex(0x6b8a76);
	C->FillRect({0.0f, 0.0f, NetW, AppH}, Hex(0x070b08));
	for (float Y = 0.0f; Y < AppH; Y += 4.0f)
	{
		C->FillRect({0.0f, Y, NetW, 1.0f}, Rgba(47, 210, 122, 0.012f));
	}
	// Header.
	C->FillRect({0.0f, 0.0f, NetW, 64.0f}, Hex(0x0b120d));
	C->FillRect({0.0f, 64.0f, NetW, 1.0f}, Hex(0x1a2a1f));
	const float Bw = NetSpaced(*C, "BURNER", 28.0f, 41.0f, 20.0f, 900, BurnerGreen, 5.0f);
	NetLockIcon(*C, 40.0f + Bw, 22.0f, 18.0f, Dim);
	UI.Text("end-to-end encrypted \xC2\xB7 messages vanish after 24h", 66.0f + Bw, 38.0f, Ts(13.0f, 500, Dim));
	// Heat.
	const int Lit = static_cast<int>(std::ceil(L.Heat / 100.0 * 12.0));
	float Hx = NetW - 28.0f - 12.0f * 17.0f;
	NetSpaced(*C, "HEAT", Hx - 16.0f, 38.0f, 12.0f, 900, L.Heat > 50.0 ? pal::Red : Dim, 2.0f, Align::Right);
	for (int K = 0; K < 12; ++K)
	{
		const Color Seg = Mix(Hex(0x2fd27a), Hex(0xef4d5a), Nf(K) / 11.0f);
		UI.RRect({Hx, 25.0f, 14.0f, 14.0f}, 3.0f, K < Lit ? Seg : Rgba(255, 255, 255, 0.06f));
		Hx += 17.0f;
	}

	// Contacts.
	C->FillRect({0.0f, 65.0f, 340.0f, AppH - 65.0f}, Hex(0x0a100c));
	C->FillRect({340.0f, 65.0f, 1.0f, AppH - 65.0f}, Hex(0x1a2a1f));
	const bool SamKnown = Ctx.Cashes > 0;
	struct Contact
	{
		const char* Name;
		const char* Preview;
		uint32_t Col;
		bool Known;
	};
	const Contact Contacts[3] = {
		{"Marcus", L.DebtCents > 0 ? "pay up before you get more work." : L.Runs >= 2 ? "got a bigger one when you're ready." : "I got work if you're not scared.", 0x2fd27a, true},
		{"Sam", SamKnown ? (L.Bans > 0 ? "lay low for a day" : "i pay ppl to play my account") : "No messages yet", 0xf2c14e, SamKnown},
		{"Dee", L.BackRoomNights == 0 ? "game's tonight. bring forty." : L.BackRoomNetCents > 0 ? "my regulars are talking about you." : "come back with a plan.", 0xf2a541, true},
	};
	for (int I = 0; I < 3; ++I)
	{
		const Rect R{0.0f, 66.0f + Nf(I) * 84.0f, 340.0f, 84.0f};
		const Ui::ClickState St = UI.Clickable("contact" + std::to_string(I), R);
		if (St.Clicked)
		{
			BurnerContact = I;
		}
		if (BurnerContact == I || St.Hover)
		{
			C->FillRect(R, Rgba(47, 210, 122, BurnerContact == I ? 0.08f : 0.04f));
		}
		if (BurnerContact == I)
		{
			C->FillRect({R.X, R.Y, 3.0f, R.H}, BurnerGreen);
		}
		const Color Cc = Hex(Contacts[I].Col);
		C->FillCircle(R.X + 44.0f, R.Y + 42.0f, 22.0f, Contacts[I].Known ? NetA(Cc, 0.2f) : Rgba(255, 255, 255, 0.05f));
		C->StrokeEllipse(R.X + 44.0f, R.Y + 42.0f, 22.0f, 22.0f, Contacts[I].Known ? Cc : Dim, 1.5f);
		UI.Text(std::string(1, Contacts[I].Name[0]), R.X + 44.0f, R.Y + 43.0f, Ts(18.0f, 900, Contacts[I].Known ? Cc : Dim, Align::Center, Baseline::Middle));
		UI.Text(Contacts[I].Name, R.X + 80.0f, R.Y + 36.0f, Ts(17.0f, 800, Contacts[I].Known ? Text : Dim));
		UI.Text(Contacts[I].Preview, R.X + 80.0f, R.Y + 58.0f, Ts(13.0f, 500, Dim, Align::Left, Baseline::Alphabetic, false, 240.0f));
		if (!Contacts[I].Known)
		{
			NetLockIcon(*C, R.X + R.W - 34.0f, R.Y + 32.0f, 16.0f, Dim);
		}
	}
	UI.Text("Unknown numbers stay unknown.", 24.0f, AppH - 24.0f, Ts(12.0f, 500, NetA(Dim, 0.7f)));

	// Thread.
	const Contact& Cur = Contacts[BurnerContact];
	const Color Cc = Hex(Cur.Col);
	C->FillRect({341.0f, 65.0f, NetW - 341.0f, 64.0f}, Hex(0x0b120d));
	C->FillRect({341.0f, 129.0f, NetW - 341.0f, 1.0f}, Hex(0x1a2a1f));
	C->FillCircle(380.0f, 97.0f, 18.0f, NetA(Cc, 0.2f));
	UI.Text(std::string(1, Cur.Name[0]), 380.0f, 98.0f, Ts(15.0f, 900, Cc, Align::Center, Baseline::Middle));
	UI.Text(Cur.Name, 410.0f, 94.0f, Ts(17.0f, 800, Text));
	UI.Text(Cur.Known ? "online" : "not in your contacts", 410.0f, 114.0f, Ts(12.0f, 600, Cur.Known ? BurnerGreen : Dim));
	if (!Cur.Known)
	{
		UI.Text("Sam hasn't messaged you.", 970.0f, 420.0f, Ts(20.0f, 800, Dim, Align::Center));
		UI.Text("He only hires players who cash. Get your name on the board.", 970.0f, 450.0f, Ts(15.0f, 500, NetA(Dim, 0.8f), Align::Center));
		return;
	}
	struct Msg
	{
		bool Mine;
		std::string Body;
	};
	std::vector<Msg> Thread;
	if (BurnerContact == 0)
	{
		Thread = {{false, "heard you're behind on rent."}, {false, "I got work if you're not scared."}, {false, "drop-offs. 120 to 200 a run. you drive, you don't ask questions."}};
		if (L.Runs >= 1)
		{
			Thread.push_back({true, "done."});
			Thread.push_back({false, "good. keep your phone on."});
		}
		if (L.Runs >= 2)
		{
			Thread.push_back({false, "you're reliable. got a bigger one when you're ready. long drive, real money."});
		}
		if (L.Busts >= 1)
		{
			Thread.push_back({false, "you lost my bag."});
			Thread.push_back({false, L.DebtCents > 0 ? "that's " + Money(L.DebtCents) + ". pay up before you get more work." : "we're square. don't lose another one."});
		}
		if (L.Heat > 50.0)
		{
			Thread.push_back({false, "cops are hot on your plates right now. your call."});
		}
	}
	else if (BurnerContact == 1)
	{
		Thread = {{false, "saw ur name on the riverline board"}, {false, "i pay ppl to play my account when im busy. 150 + a tip if im up"}, {false, "dont get caught lol. they call it ghosting"}};
		if (L.Ghosts > L.Bans)
		{
			Thread.push_back({true, "session done. you're up."});
			Thread.push_back({false, "LEGEND. sending now"});
		}
		if (L.Bans > 0)
		{
			Thread.push_back({false, "dude they flagged BOTH accounts"});
			Thread.push_back({false, "lay low for a day"});
		}
	}
	else
	{
		// Dee: the game across the street, and what she thinks of how you play in it.
		Thread = {{false, "it's Dee. heard about the notice on your door."},
			{false, "the game's still on. back room at the Spin Cycle, across the street. one-two no limit, tuesdays, thursdays, saturdays."},
			{false, "forty to sit, two hundred max. doors at nine, last hand at five."},
			{false, "and kid. watch their hands, not their faces. faces lie."}};
		if (L.BackRoomNights >= 1)
		{
			Thread.push_back({true, "thanks for having me."});
			Thread.push_back({false, L.BackRoomNetCents > 0 ? "you're up $" + std::to_string(L.BackRoomNetCents / 100) + " at my table. my regulars are talking about you. don't get cute."
															: L.BackRoomNetCents < 0 ? "you've given my regulars $" + std::to_string(-L.BackRoomNetCents / 100) + ". come back with a plan."
																					 : "broke even. that's a start."});
		}
		int Learned = 0;
		for (const auto& Rd : L.Reads)
		{
			Learned += Rd.second >= 2 ? 1 : 0;
		}
		if (L.BackRoomNights >= 1 || L.LiveEvents >= 1)
		{
			Thread.push_back({false, L.LiveEvents == 0 ? "the embercrest runs every day now. noon and seven, a turbo late on weekends. sixty, a hundred people a night. i deal sundays."
														   : L.LiveCashes > 0 ? "you cashed at the embercrest. the floor knows your name now." : "the embercrest's there every day. strangers are easier to read than my regulars. mostly."});
			if (L.LiveEvents == 0)
			{
				Thread.push_back({false, "sal plays the nightly. mrs. park's been playing the noon game since before you were born. start with the nightly."});
			}
		}
		if (Learned >= 1)
		{
			Thread.push_back({false, Learned == 1 ? "you caught one of their tells. most people never see a single one." : "you've got " + std::to_string(Learned) + " of their tells now. they'll start to feel it."});
		}
	}
	// Bubbles, newest at the bottom, above the offers.
	// Dee's offers are the two games and the Embercrest's schedule: a taller strip.
	const float OffersTop = AppH - (BurnerContact == 2 ? 318.0f : 260.0f);
	float By = OffersTop - 24.0f;
	const float Mx = 380.0f;
	const float MaxW = 560.0f;
	for (size_t K = Thread.size(); K-- > 0;)
	{
		const Msg& M = Thread[K];
		const std::vector<std::string> Lines = NetWrap(*C, M.Body, MaxW - 36.0f, 15.0f, 500);
		float W = 0.0f;
		for (const std::string& Ln : Lines)
		{
			W = std::max(W, UI.Measure(Ln, 15.0f, 500));
		}
		const float H = 22.0f * Nf(Lines.size()) + 22.0f;
		By -= H;
		if (By < 140.0f)
		{
			break;
		}
		const float Bx = M.Mine ? NetW - 40.0f - W - 36.0f : Mx;
		const float Bi = NetEase((Now - AppAt - 0.05 * static_cast<double>(Thread.size() - K)) / 0.35);
		const float A0 = C->GetAlpha();
		C->SetAlpha(A0 * Bi);
		UI.RRect({Bx, By, W + 36.0f, H}, 16.0f, M.Mine ? BurnerGreen : Hex(0x15211a));
		float Ly = By + 27.0f;
		for (const std::string& Ln : Lines)
		{
			UI.Text(Ln, Bx + 18.0f, Ly, Ts(15.0f, 500, M.Mine ? Hex(0x04210f) : Text));
			Ly += 22.0f;
		}
		C->SetAlpha(A0);
		By -= 10.0f;
	}

	// Offers, or the debt.
	C->FillRect({341.0f, OffersTop, NetW - 341.0f, 1.0f}, Hex(0x1a2a1f));
	if (BurnerContact == 0 && L.DebtCents > 0)
	{
		const Rect D{380.0f, OffersTop + 30.0f, 880.0f, 190.0f};
		UI.RRect(D, 16.0f, Hex(0x1f0c10), NetA(pal::Red, 0.6f));
		UI.Text("You owe Marcus", D.X + 32.0f, D.Y + 50.0f, Ts(16.0f, 700, Hex(0xff9aa5)));
		UI.Text(Money(L.DebtCents), D.X + 32.0f, D.Y + 104.0f, Ts(46.0f, 900, Hex(0xffffff), Align::Left, Baseline::Alphabetic, true));
		UI.Text("No more work until it's paid. He knows where you live.", D.X + 32.0f, D.Y + 140.0f, Ts(14.0f, 500, Hex(0xff9aa5)));
		const bool Can = S.BankrollCents >= L.DebtCents;
		if (AppButton("paydebt", {D.X + D.W - 292.0f, D.Y + 64.0f, 260.0f, 60.0f}, "Pay " + Money(L.DebtCents), pal::Red, Hex(0xffffff), Can, Can ? std::string() : "Balance " + Money(S.BankrollCents)))
		{
			S.PayDebt();
		}
		return;
	}
	if (BurnerContact == 2)
	{
		// Dee's two games: the back room's, with a buy-in from the bankroll, and the Embercrest's Sunday tournament.
		const life::Activity* G = life::Find("dee-game");
		const life::Activity* Rv = life::Find("embercrest");
		if (!G || !Rv)
		{
			return;
		}
		static const char* NightNames[7] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
		// The night a game is on: tonight's while its doors are open, else the next one.
		auto NightOf = [&](const life::Activity& A) {
			const double At = life::InWindow(A, Ctx.World) ? Ctx.World : life::NextOpen(A, Ctx.World);
			int Day = static_cast<int>(std::floor(At / 1440.0));
			if (std::fmod(At, 1440.0) < 12.0 * 60.0 && A.Opens > A.Closes)
			{
				--Day;
			}
			return NightNames[((Day % 7) + 7) % 7];
		};
		const float Gap = 16.0f;
		const float TotalW = NetW - 420.0f;
		const Rect R{380.0f, OffersTop + 24.0f, (TotalW - Gap) * 0.4f, 270.0f};
		const Rect Rr{R.X + R.W + Gap, R.Y, TotalW - Gap - R.W, R.H};
		{
			UI.RRect(R, 16.0f, Hex(0x15110a), NetA(Cc, 0.45f));
			UI.Text(std::string("The ") + NightOf(*G) + " game", R.X + 24.0f, R.Y + 38.0f, Ts(20.0f, 800, Text));
			UI.Text("$1/$2 no-limit  \xC2\xB7  Tue, Thu, Sat  \xC2\xB7  doors 9 PM", R.X + 24.0f, R.Y + 61.0f, Ts(13.0f, 600, Dim));
			const std::string Why = life::Blocked(*G, L, Ctx);
			const bool Open = Why.empty();
			const bool WindowOnly = !Open && !life::InWindow(*G, Ctx.World) && Ctx.Bankroll >= life::GameMinBuyInCents && L.Energy >= G->Energy;
			UI.Text(Open ? "The game's running. Dee saved you a seat." : WindowOnly ? "Next game " + Why : Why, R.X + 24.0f, R.Y + 88.0f, Ts(14.0f, 800, Open ? Cc : Dim));
			UI.Text(G->Blurb, R.X + 24.0f, R.Y + 112.0f, Ts(12.5f, 500, Dim, Align::Left, Baseline::Alphabetic, false, R.W - 48.0f));
			const Chips Amounts[3] = {life::GameMinBuyInCents, 10000, life::GameMaxBuyInCents};
			const float ButtonW = (R.W - 48.0f - 2.0f * 12.0f) / 3.0f;
			for (int K = 0; K < 3; ++K)
			{
				const Chips Amt = Amounts[K];
				const bool Ok = Open && S.BankrollCents >= Amt;
				const std::string Sub = Open && !Ok ? "Balance " + Money(S.BankrollCents) : std::string();
				if (AppButton("buyin:" + std::to_string(Amt), {R.X + 24.0f + Nf(K) * (ButtonW + 12.0f), R.Y + R.H - 60.0f, ButtonW, 44.0f}, "Sit " + NetMoney(Amt), Cc, Hex(0x1d1204), Ok, Sub))
				{
					const std::string E = S.GoToGame(G->Id, Amt);
					if (!E.empty())
					{
						Toast = E;
						ToastAt = Now;
					}
				}
			}
		}
		{
			// The Embercrest: a casino's blue, the next events the bus can still make, and what you've done there.
			const Color Blue = Hex(Rv->Color);
			UI.RRect(Rr, 16.0f, Hex(0x0b1119), NetA(Blue, 0.45f));
			UI.Text(Rv->Title, Rr.X + 24.0f, Rr.Y + 36.0f, Ts(20.0f, 800, Text));
			UI.Text("Embercrest Casino card room  \xC2\xB7  6-max freezeouts  \xC2\xB7  the 14 bus, " + Money(live::BusFareCents), Rr.X + Rr.W - 24.0f, Rr.Y + 36.0f,
				Ts(12.5f, 600, Dim, Align::Right));
			const std::string Record = L.LiveEvents == 0 ? std::string("Two or three a day, sixty to a hundred and twenty runners. Registration at the desk until late reg closes.")
				: "Played " + std::to_string(L.LiveEvents) + "  \xC2\xB7  cashed " + std::to_string(L.LiveCashes) +
					(L.LiveBestPlace > 0 ? "  \xC2\xB7  best " + Ordinal(L.LiveBestPlace) : std::string()) + "  \xC2\xB7  won " + Money(L.LiveWonCents);
			UI.Text(Record, Rr.X + 24.0f, Rr.Y + 60.0f, Ts(12.5f, 500, Dim, Align::Left, Baseline::Alphabetic, false, Rr.W - 48.0f));
			// The entry the player holds comes first (they can go back to it), then what's next.
			std::vector<live::Occurrence> Events;
			if (const life::LiveEntry* Mine = live::ActiveEntry(L))
			{
				const live::Occurrence O = live::FindOccurrence(Mine->Id);
				if (O.Valid())
				{
					Events.push_back(O);
				}
			}
			for (const live::Occurrence& O : live::Reachable(Ctx.World, 30.0))
			{
				if (Events.size() < 3 && (Events.empty() || Events.front().Id != O.Id))
				{
					Events.push_back(O);
				}
			}
			const int Today = net::DayOf(Ctx.World);
			static const char* DayNames[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
			const std::string Gate = Ctx.InTournament ? "Finish your tournament first." : L.Energy < Rv->Energy ? "Too tired. Sleep first." : std::string();
			float Ry = Rr.Y + 76.0f;
			for (const live::Occurrence& O : Events)
			{
				C->FillRect({Rr.X + 20.0f, Ry, Rr.W - 40.0f, 1.0f}, NetA(Blue, 0.18f));
				const life::LiveEntry* Mine = live::EntryFor(L, O.Id);
				const bool In = Mine && Mine->State == life::LiveEntry::Registered;
				const std::string When = O.Day == Today ? "Today" : O.Day == Today + 1 ? "Tomorrow" : DayNames[((O.Day % 7) + 7) % 7];
				UI.Text(net::TimeLabel(O.Start), Rr.X + 24.0f, Ry + 27.0f, Ts(16.0f, 800, In ? Blue : Text));
				UI.Text(When, Rr.X + 24.0f, Ry + 46.0f, Ts(12.0f, 600, Dim));
				UI.Text(std::string(O.T->Short) + "  " + NetMoney(O.T->BuyInCents), Rr.X + 118.0f, Ry + 27.0f, Ts(16.0f, 800, Text));
				const std::string Detail = ChipsText(static_cast<double>(O.T->StartingStack)) + " chips  \xC2\xB7  " + std::to_string(static_cast<int>(O.T->LevelMinutes)) + "-min levels  \xC2\xB7  ~" + std::to_string(O.Field) + " runners  \xC2\xB7  reg to " +
					net::TimeLabel(O.LateRegEnds);
				UI.Text(Detail, Rr.X + 118.0f, Ry + 46.0f, Ts(12.0f, 500, Dim, Align::Left, Baseline::Alphabetic, false, Rr.W - 118.0f - 220.0f));
				const std::string Why = In ? std::string() : !Gate.empty() ? Gate : live::CanRegister(S.BankrollCents, L, O, Ctx.World);
				const bool Ok = Why.empty();
				const std::string Label = In ? "Go back" : "Register " + NetMoney(O.T->BuyInCents);
				if (AppButton("live:" + O.Id, {Rr.X + Rr.W - 24.0f - 188.0f, Ry + 9.0f, 188.0f, 44.0f}, Label, Blue, Hex(0x06101a), Ok, Ok ? std::string() : Why))
				{
					const std::string E = S.GoToLive(O.Id);
					if (!E.empty())
					{
						Toast = E;
						ToastAt = Now;
					}
				}
				Ry += 62.0f;
			}
		}
		return;
	}
	std::vector<const life::Activity*> Offers;
	for (const life::Activity& A : life::Catalog())
	{
		if ((BurnerContact == 0 && A.Type == life::Kind::Hustle) || (BurnerContact == 1 && A.Type == life::Kind::Ghost))
		{
			Offers.push_back(&A);
		}
	}
	for (size_t I = 0; I < Offers.size(); ++I)
	{
		const life::Activity& A = *Offers[I];
		const Rect R{380.0f + Nf(I) * 600.0f, OffersTop + 24.0f, 580.0f, 214.0f};
		UI.RRect(R, 16.0f, Hex(0x0d1610), NetA(Cc, 0.35f));
		UI.Text(A.Title, R.X + 26.0f, R.Y + 40.0f, Ts(20.0f, 800, Text));
		UI.Text(std::to_string(static_cast<int>(A.Hours)) + " hours \xC2\xB7 starts " + net::TimeLabel(static_cast<double>(A.Opens)) + "\xE2\x80\x93" + net::TimeLabel(static_cast<double>(A.Closes)), R.X + 26.0f, R.Y + 64.0f,
			Ts(13.0f, 600, Dim));
		UI.Text(NetMoney(A.PayMin) + "\xE2\x80\x93" + NetMoney(A.PayMax), R.X + R.W - 26.0f, R.Y + 46.0f, Ts(24.0f, 900, BurnerGreen, Align::Right, Baseline::Alphabetic, true));
		// Risk.
		const double Risk = life::RiskOf(A, L);
		const Color Rc = Risk < 0.1 ? pal::Gold : Risk < 0.25 ? pal::Orange : pal::Red;
		UI.Text(A.Type == life::Kind::Ghost ? "Account risk" : "Risk of getting picked up", R.X + 26.0f, R.Y + 98.0f, Ts(13.0f, 600, Dim));
		UI.Text(Fixed(Risk * 100.0, 0) + "%", R.X + R.W - 26.0f, R.Y + 98.0f, Ts(15.0f, 900, Rc, Align::Right, Baseline::Alphabetic, true));
		UI.RRect({R.X + 26.0f, R.Y + 108.0f, R.W - 52.0f, 8.0f}, 4.0f, Rgba(255, 255, 255, 0.06f));
		UI.RRect({R.X + 26.0f, R.Y + 108.0f, std::max(8.0f, (R.W - 52.0f) * Nf(Risk / 0.6)), 8.0f}, 4.0f, Rc);
		const std::string Fine = A.Type == life::Kind::Ghost ? "If flagged: RiverLine locks your account for 24 hours. No pay." : "If stopped: a fine up to $250, a night in holding, and Marcus wants the bag paid for.";
		UI.Text(A.Heat > 0.0 ? "+" + std::to_string(static_cast<int>(A.Heat)) + " heat \xC2\xB7 " + Fine : Fine, R.X + 26.0f, R.Y + 140.0f, Ts(12.5f, 500, Dim, Align::Left, Baseline::Alphabetic, false, R.W - 52.0f));
		const std::string Why = life::Blocked(A, L, Ctx);
		if (AppButton("offer:" + A.Id, {R.X + 26.0f, R.Y + R.H - 62.0f, R.W - 52.0f, 46.0f}, Why.empty() ? "Accept" : Why, BurnerGreen, Hex(0x04210f), Why.empty()))
		{
			TryActivity(A.Id, Now);
		}
	}
}

// ------------------------------------------------------------------ bank

void RiverLine::BankApp(double Now)
{
	const life::State& L = S.Life;
	const Color Navy = Hex(0x0d2547);
	const Color Dark = Hex(0x0f1d33);
	const Color Gray = Hex(0x6b7a90);
	C->FillRect({0.0f, 0.0f, NetW, AppH}, Hex(0xf2f5fa));
	C->FillRect({0.0f, 0.0f, NetW, 72.0f}, Paint::Linear({0.0f, 0.0f}, {NetW, 0.0f}, Navy, Hex(0x173a6b)));
	AppIcon(App::Bank, 28.0f, 16.0f, 40.0f);
	const float Lw = UI.Text("Northside Credit Union", 80.0f, 45.0f, Ts(24.0f, 900, Hex(0xffffff)));
	UI.Text("Online banking", 94.0f + Lw, 45.0f, Ts(15.0f, 500, Hex(0x9fb6d6)));
	UI.Text("Signed in as " + S.HeroName, NetW - 28.0f, 44.0f, Ts(14.0f, 600, Hex(0xc9d7ea), Align::Right));
	const float In = NetEase((Now - AppAt) / 0.6);

	// Balance.
	const Rect Bal{40.0f, 100.0f, 860.0f, 180.0f};
	C->GlowRoundRect({Bal.X, Bal.Y + 8.0f, Bal.W, Bal.H}, 18.0f, Rgba(13, 37, 71, 0.25f), 20.0f);
	C->FillRoundRect(Bal, 18.0f, Paint::Linear({Bal.X, Bal.Y}, {Bal.X + Bal.W, Bal.Y + Bal.H}, Hex(0x1d4ed8), Navy));
	C->PushClip(Bal);
	C->FillEllipse(Bal.X + Bal.W - 120.0f, Bal.Y + 30.0f, 260.0f, 200.0f, Rgba(255, 255, 255, 0.06f));
	C->PopClip();
	NetSpaced(*C, "CHECKING \xC2\xB7\xC2\xB7\xC2\xB7 4471", Bal.X + 32.0f, Bal.Y + 42.0f, 12.0f, 800, Hex(0xbcd0f0), 2.0f);
	UI.Text(Money(static_cast<Chips>(static_cast<double>(S.BankrollCents) * static_cast<double>(In))), Bal.X + 32.0f, Bal.Y + 112.0f, Ts(58.0f, 900, Hex(0xffffff), Align::Left, Baseline::Alphabetic, true));
	UI.Text("Available balance \xC2\xB7 linked to RiverLine", Bal.X + 32.0f, Bal.Y + 146.0f, Ts(14.0f, 600, Hex(0xbcd0f0)));

	// Rent.
	const Rect Rr{40.0f, 300.0f, 860.0f, 250.0f};
	const bool Late = L.RentStage == life::Rent::FinalNotice;
	const bool Out = L.RentStage == life::Rent::Evicted;
	const bool Paid = L.RentStage == life::Rent::Paid;
	const Color Tone = Out ? Hex(0x6b7280) : Late ? Hex(0xdc2626) : Paid ? Hex(0x16a34a) : Hex(0xea580c);
	C->GlowRoundRect({Rr.X, Rr.Y + 6.0f, Rr.W, Rr.H}, 18.0f, Rgba(13, 37, 71, 0.1f), 16.0f);
	UI.RRect(Rr, 18.0f, Hex(0xffffff), NetA(Tone, 0.5f), 1.5f);
	C->FillRoundRect({Rr.X, Rr.Y, 8.0f, Rr.H}, 4.0f, Tone);
	const std::string DueDay = NetUpper(net::WeekdayName(net::DayOf(L.RentDeadline - 1.0), true));
	const std::string Head = Out ? "EVICTED \xC2\xB7 ON DEE'S COUCH"
		: Late ? "FINAL NOTICE \xC2\xB7 RENT + LATE FEE \xC2\xB7 LOCKS CHANGE " + DueDay
		: Paid ? "RENT PAID \xC2\xB7 NEXT MONTH"
		: L.RentsPaid == 0 && L.Evictions == 0 ? "RENT + LATE FEE \xC2\xB7 DUE " + DueDay : "RENT \xC2\xB7 DUE " + DueDay;
	NetSpaced(*C, Head, Rr.X + 36.0f, Rr.Y + 44.0f, 12.0f, 900, Tone, 1.8f);
	if (Out)
	{
		// What it takes to get a key back, and what being out costs meanwhile.
		const Chips Back = std::max<Chips>(0, L.RentDueCents - life::MonthlyRentCents);
		UI.Text(Money(L.RentDueCents), Rr.X + 36.0f, Rr.Y + 104.0f, Ts(46.0f, 900, Dark, Align::Left, Baseline::Alphabetic, true));
		UI.Text("Back rent " + Money(Back) + " + a month up front " + Money(life::MonthlyRentCents), Rr.X + 36.0f, Rr.Y + 134.0f, Ts(15.0f, 600, Gray));
		const int Nights = std::max(0, net::DayOf(World) - net::DayOf(L.EvictedAt));
		std::string Unit = "Nothing in storage";
		Color UnitInk = Gray;
		if (L.StorageDue > 0.0)
		{
			const double ToUnit = L.StorageDue - World;
			Unit = "Storage renews in " + net::Countdown(std::max(0.0, ToUnit)) + " \xC2\xB7 " + Money(life::StorageCents);
			UnitInk = ToUnit < 2.0 * net::MinutesPerDay && S.BankrollCents < life::StorageCents ? Hex(0xdc2626) : Dark;
		}
		else if (L.Auctions > 0)
		{
			Unit = "The storage unit went to auction";
			UnitInk = Hex(0xdc2626);
		}
		UI.Text(Unit, Rr.X + Rr.W - 36.0f, Rr.Y + 98.0f, Ts(15.0f, 800, UnitInk, Align::Right));
		if (L.StorageDue > 0.0)
		{
			// The unit's next month, paid ahead from here (Session::PayStorage: no more than a month ahead), so the card
			// on file isn't the gear's last chance.
			const bool Ahead = L.StorageDue - World > life::StorageDays * net::MinutesPerDay;
			const bool Can = !Ahead && S.BankrollCents >= life::StorageCents;
			const std::string Label = Ahead ? "Unit paid through " + net::DateLabel(net::DayOf(L.StorageDue - 1.0)) : "Pay the unit ahead \xC2\xB7 " + NetMoney(life::StorageCents);
			const float Lead = Ahead ? 34.0f : 16.0f; // room for the check mark once it's paid
			const float Bw = UI.Measure(Label, 13.0f, 800) + Lead + 16.0f;
			const Rect Sb{Rr.X + Rr.W - 36.0f - Bw, Rr.Y + 22.0f, Bw, 30.0f};
			const Ui::ClickState Cs = UI.Clickable("paystorage", Sb, Can);
			const Color Ink = Ahead ? Hex(0x16a34a) : Can ? Dark : Gray;
			UI.RRect(Sb, 15.0f, Cs.Hover ? Hex(0xeef2f7) : Hex(0xffffff), Ahead ? NetA(Hex(0x16a34a), 0.5f) : Hex(0xd5dde8), 1.0f);
			if (Ahead)
			{
				NetCheck(*C, Sb.X + 18.0f, Sb.Y + 15.0f, 10.0f, Ink);
			}
			UI.Text(Label, Sb.X + Lead, Sb.Y + 20.0f, Ts(13.0f, 800, Ink));
			if (Cs.Clicked)
			{
				const std::string Fail = S.PayStorage();
				if (!Fail.empty())
				{
					Toast = Fail;
					ToastAt = Now;
				}
			}
		}
		UI.Text(Nights == 0 ? "First night on the couch" : "Night " + std::to_string(Nights + 1) + " on the couch \xC2\xB7 " + Money(L.CouchCents) + " to Dee", Rr.X + Rr.W - 36.0f, Rr.Y + 122.0f,
			Ts(13.0f, 600, Gray, Align::Right));
		const float Have = Nf(Clamp01(static_cast<double>(S.BankrollCents) / static_cast<double>(std::max<Chips>(1, L.RentDueCents))));
		UI.RRect({Rr.X + 36.0f, Rr.Y + 158.0f, Rr.W - 72.0f, 12.0f}, 6.0f, Hex(0xe6ebf3));
		UI.RRect({Rr.X + 36.0f, Rr.Y + 158.0f, std::max(12.0f, (Rr.W - 72.0f) * Have * In), 12.0f}, 6.0f, Paint::Linear({Rr.X, 0.0f}, {Rr.X + Rr.W, 0.0f}, Tone, Mix(Tone, Hex(0x16a34a), Have)));
		// The same checks MoveBackIn makes; the button's second line says which one is in the way.
		const Chips Short = std::max<Chips>(0, L.RentDueCents - S.BankrollCents);
		const std::string Why = Short > 0 ? "Short " + Money(Short) : S.T ? std::string("Finish your tables first") : S.TimeSkip.Active ? std::string("Busy right now") : std::string();
		if (AppButton("movein", {Rr.X + 36.0f, Rr.Y + 186.0f, 300.0f, 46.0f}, "Move back in", Hex(0x16a34a), Hex(0xffffff), Why.empty(), Why))
		{
			const std::string Fail = S.MoveBackIn();
			if (!Fail.empty())
			{
				Toast = Fail;
				ToastAt = Now;
			}
		}
		// What being out costs meanwhile, as it stands: the gear in the unit, gone at auction, or never stored.
		std::string Rig = "it's just the laptop (two tables, no stream)";
		if (L.StorageDue > 0.0)
		{
			Rig = "the gear's in storage (two tables, no stream)";
		}
		else if (L.Auctions > 0)
		{
			Rig = "the unit was auctioned, so it's the laptop (two tables, no stream)";
		}
		const int Less = static_cast<int>(std::lround((1.0 - life::CouchRest) * 100.0));
		NetParagraph(*C, "Meanwhile: " + Rig + ", the couch gives back " + std::to_string(Less) + "% less sleep, and " + NetMoney(life::CouchChipInCents) + " a day goes to Dee's groceries.",
			Rr.X + 356.0f, Rr.Y + 200.0f, Rr.W - 392.0f, 13.0f, 500, Gray, 18.0f, 2);
	}
	else
	{
		UI.Text(Money(L.RentDueCents), Rr.X + 36.0f, Rr.Y + 104.0f, Ts(46.0f, 900, Dark, Align::Left, Baseline::Alphabetic, true));
		const double Left = L.RentDeadline - World;
		UI.Text(std::string(Late ? "Last day: " : "Due ") + net::WeekdayName(net::DayOf(L.RentDeadline - 1.0), true) + ", " + net::DateLabel(net::DayOf(L.RentDeadline - 1.0)) + " at midnight", Rr.X + 36.0f,
			Rr.Y + 134.0f, Ts(15.0f, 600, Late ? Hex(0xdc2626) : Gray));
		UI.Text(NetHms(std::max(0.0, Left)), Rr.X + Rr.W - 36.0f, Rr.Y + 98.0f, Ts(32.0f, 700, Left < 24.0 * 60.0 ? Hex(0xdc2626) : Dark, Align::Right, Baseline::Alphabetic, true));
		UI.Text("left", Rr.X + Rr.W - 36.0f, Rr.Y + 122.0f, Ts(13.0f, 600, Gray, Align::Right));
		const float Have = Nf(Clamp01(static_cast<double>(S.BankrollCents) / static_cast<double>(std::max<Chips>(1, L.RentDueCents))));
		UI.RRect({Rr.X + 36.0f, Rr.Y + 158.0f, Rr.W - 72.0f, 12.0f}, 6.0f, Hex(0xe6ebf3));
		UI.RRect({Rr.X + 36.0f, Rr.Y + 158.0f, std::max(12.0f, (Rr.W - 72.0f) * Have * In), 12.0f}, 6.0f, Paint::Linear({Rr.X, 0.0f}, {Rr.X + Rr.W, 0.0f}, Tone, Mix(Tone, Hex(0x16a34a), Have)));
		// The same checks PayRent makes; the button's second line says which one is in the way.
		const Chips Short = std::max<Chips>(0, L.RentDueCents - S.BankrollCents);
		const std::string Why = Short > 0 ? "Short " + Money(Short) : S.T ? std::string("Finish your tables first") : std::string();
		if (AppButton("payrent", {Rr.X + 36.0f, Rr.Y + 186.0f, 300.0f, 46.0f}, Paid ? "Pay next month early" : "Pay rent", Hex(0x1d4ed8), Hex(0xffffff), Why.empty(), Why))
		{
			S.PayRent();
		}
		if (Late)
		{
			// Only gear bought outright goes into a unit (Session::Evict); with none, it's just the laptop and the couch.
			bool Stores = false;
			for (const auto& G : S.Gear)
			{
				const gear::Item* I = gear::Find(G.first);
				Stores = Stores || (I && !I->Monthly);
			}
			NetParagraph(*C,
				Stores ? "Miss this and the locks change: the gear goes to storage, you go to Dee's couch, and a key costs the back rent plus a month."
					   : "Miss this and the locks change: you and the laptop go to Dee's couch, and a key costs the back rent plus a month.",
				Rr.X + 356.0f, Rr.Y + 200.0f, Rr.W - 392.0f, 13.0f, 600, Hex(0xdc2626), 18.0f, 2);
		}
		else
		{
			UI.Text(Paid ? "Paid " + std::to_string(L.RentsPaid) + (L.RentsPaid == 1 ? " month" : " months") + " so far." : "The landlord collects at the deadline if the money is here.", Rr.X + 356.0f,
				Rr.Y + 214.0f, Ts(14.0f, 500, Gray));
		}
	}

	// Money in and money out, over the same stretch of the ledger for every row (it keeps the last sixty lines, so
	// lifetime counters and ledger sums would disagree): where it came from on the left, where it went on the right.
	const Rect Flow{40.0f, 570.0f, 860.0f, 360.0f};
	UI.RRect(Flow, 18.0f, Hex(0xffffff), Hex(0xe2e8f0));
	UI.Text("Money in, money out", Flow.X + 32.0f, Flow.Y + 44.0f, Ts(20.0f, 900, Dark));
	std::string Window = "Nothing yet: it all shows up here.";
	if (!L.Ledger.empty())
	{
		const int Since = net::DayOf(L.Ledger.back().At);
		const std::string When = std::string(net::WeekdayName(Since)) + ", " + net::DateLabel(Since);
		Window = L.Ledger.size() >= LedgerKept ? "Your last " + std::to_string(LedgerKept) + " transactions, since " + When : "Everything since " + When;
	}
	UI.Text(Window, Flow.X + 32.0f, Flow.Y + 66.0f, Ts(12.5f, 600, Gray));
	// In: 0 tournament cashes, 1 the Embercrest, 2 leaderboard prizes, 3 shifts, 4 cash work, 5 streaming.
	// Out: 0 buy-ins (less refunds), 1 rent, bills and fares, 2 food and deliveries, 3 gear, 4 fines and Marcus.
	Chips Ins[6] = {0, 0, 0, 0, 0, 0};
	Chips Outs[5] = {0, 0, 0, 0, 0};
	for (const life::LedgerEntry& E : L.Ledger)
	{
		if (E.Kind == 7 || E.Amount == 0)
		{
			continue; // transfers (savings from home) are neither earned nor spent
		}
		if (E.Amount > 0)
		{
			if (E.Kind == 0 && E.Label.find(": refunded") != std::string::npos)
			{
				Outs[0] -= E.Amount; // a refund takes its buy-in back off the spending
			}
			else if (E.Kind == 0)
			{
				Ins[0] += E.Amount;
			}
			else if (E.Kind == 4)
			{
				Ins[LiveLine(L, E.Label) ? 1 : 2] += E.Amount;
			}
			else if (E.Kind == 1)
			{
				Ins[3] += E.Amount;
			}
			else if (E.Kind == 2)
			{
				Ins[4] += E.Amount;
			}
			else if (E.Kind == 6)
			{
				Ins[5] += E.Amount;
			}
			continue;
		}
		const Chips Spent = -E.Amount;
		if (E.Kind == 8 || E.Label == "Groceries at Dee's")
		{
			Outs[2] += Spent; // the corner store, Penny Drop, and the couch's groceries (a bill by kind, food all the same)
			continue;
		}
		switch (E.Kind)
		{
		case 0: Outs[0] += Spent; break;
		case 5: Outs[3] += Spent; break;
		case 2: Outs[4] += Spent; break;
		default: Outs[1] += Spent; break; // rent, the storage unit, the bus to the Embercrest
		}
	}
	Outs[0] = std::max<Chips>(0, Outs[0]);
	struct FlowRow
	{
		const char* Label;
		Chips Amount;
		uint32_t Col;
	};
	const FlowRow InRows[6] = {{"Tournament cashes", Ins[0], 0x27d3c3}, {"The Embercrest", Ins[1], EmberRgb}, {"Leaderboard prizes", Ins[2], 0xf2c14e}, {"Shifts", Ins[3], 0xff8a1f},
		{"Cash work", Ins[4], 0x16a34a}, {"Streaming", Ins[5], 0x9b5cff}};
	const FlowRow OutRows[5] = {{"Buy-ins & entry fees", Outs[0], 0x0d9488}, {"Rent, bills & fares", Outs[1], 0xef4d5a}, {"Food & deliveries", Outs[2], 0xe23b4e}, {"Gear", Outs[3], 0xff6b2c},
		{"Fines & Marcus", Outs[4], 0x7c3aed}};
	Chips Top = 1;
	Chips InSum = 0;
	Chips OutSum = 0;
	for (const FlowRow& R : InRows)
	{
		Top = std::max(Top, R.Amount);
		InSum += R.Amount;
	}
	for (const FlowRow& R : OutRows)
	{
		Top = std::max(Top, R.Amount);
		OutSum += R.Amount;
	}
	const Chips Net = InSum - OutSum;
	UI.Text((Net > 0 ? "Net +" : Net < 0 ? "Net \xE2\x88\x92" : "Net ") + Money(Net < 0 ? -Net : Net), Flow.X + Flow.W - 32.0f, Flow.Y + 44.0f,
		Ts(17.0f, 900, Net > 0 ? Hex(0x16a34a) : Net < 0 ? Hex(0xdc2626) : Gray, Align::Right, Baseline::Alphabetic, true));
	auto Column = [&](const FlowRow* Rows, int Count, float X, float W, const char* Title, Chips Sum, const Color& SumInk) {
		NetSpaced(*C, Title, X, Flow.Y + 100.0f, 11.0f, 900, Gray, 1.8f);
		UI.Text(Money(Sum), X + W, Flow.Y + 100.0f, Ts(14.0f, 900, SumInk, Align::Right, Baseline::Alphabetic, true));
		C->FillRect({X, Flow.Y + 110.0f, W, 1.0f}, Hex(0xe2e8f0));
		for (int I = 0; I < Count; ++I)
		{
			const FlowRow& R = Rows[I];
			const float Y = Flow.Y + 118.0f + Nf(I) * 38.0f;
			const bool Some = R.Amount > 0;
			UI.Text(R.Label, X, Y + 14.0f, Ts(13.5f, 600, Some ? Dark : NetA(Gray, 0.7f)));
			UI.Text(Some ? Money(R.Amount) : std::string("\xE2\x80\x94"), X + W, Y + 14.0f, Ts(13.5f, 800, Some ? Dark : NetA(Gray, 0.6f), Align::Right, Baseline::Alphabetic, true));
			UI.RRect({X, Y + 21.0f, W, 8.0f}, 4.0f, Hex(0xeef2f7));
			const float Bw = W * Nf(static_cast<double>(R.Amount) / static_cast<double>(Top)) * In;
			if (Some && Bw > 0.5f)
			{
				UI.RRect({X, Y + 21.0f, std::max(8.0f, Bw), 8.0f}, 4.0f, Hex(R.Col));
			}
		}
	};
	Column(InRows, 6, Flow.X + 32.0f, 380.0f, "MONEY IN", InSum, Hex(0x16a34a));
	Column(OutRows, 5, Flow.X + 448.0f, 380.0f, "MONEY OUT", OutSum, Hex(0xdc2626));

	// History.
	const Rect H{930.0f, 100.0f, 630.0f, 830.0f};
	UI.RRect(H, 18.0f, Hex(0xffffff), Hex(0xe2e8f0));
	UI.Text("Recent activity", H.X + 28.0f, H.Y + 46.0f, Ts(20.0f, 900, Dark));
	if (L.Ledger.empty())
	{
		UI.Text("Nothing yet. Money in, money out: it all shows up here.", H.X + 28.0f, H.Y + 90.0f, Ts(15.0f, 500, Gray));
	}
	for (size_t I = 0; I < L.Ledger.size() && I < 14; ++I)
	{
		const life::LedgerEntry& E = L.Ledger[I];
		const float Y = H.Y + 72.0f + Nf(I) * 53.0f;
		const float Ri = NetEase((Now - AppAt - 0.03 * static_cast<double>(I)) / 0.4);
		const float A0 = C->GetAlpha();
		C->SetAlpha(A0 * Ri);
		// The Embercrest's lines (buy-in, fee, prize, refund) wear its name and its ember, whatever their kind.
		const bool Live = LiveLine(L, E.Label);
		const Color Kc = Live ? Hex(EmberRgb) : KindColor(E.Kind);
		const std::string Kn = Live ? "Embercrest" : KindName(E.Kind);
		C->FillCircle(H.X + 46.0f, Y + 24.0f, 17.0f, NetA(Kc, 0.15f));
		UI.Text(Kn.substr(0, 1), H.X + 46.0f, Y + 25.0f, Ts(14.0f, 900, Mix(Kc, Hex(0x000000), 0.2f), Align::Center, Baseline::Middle));
		UI.Text(E.Label, H.X + 76.0f, Y + 21.0f, Ts(15.0f, 700, Dark, Align::Left, Baseline::Alphabetic, false, 360.0f));
		UI.Text(std::string(net::WeekdayName(net::DayOf(E.At))) + " " + net::TimeLabel(E.At) + " \xC2\xB7 " + Kn, H.X + 76.0f, Y + 40.0f, Ts(12.5f, 500, Gray));
		UI.Text(E.Amount == 0 ? std::string("\xE2\x80\x94") : (E.Amount > 0 ? "+" : "\xE2\x88\x92") + Money(E.Amount > 0 ? E.Amount : -E.Amount), H.X + H.W - 28.0f, Y + 30.0f,
			Ts(16.0f, 800, E.Amount > 0 ? Hex(0x16a34a) : E.Amount < 0 ? Hex(0xdc2626) : Gray, Align::Right, Baseline::Alphabetic, true));
		if (I + 1 < L.Ledger.size() && I < 13)
		{
			C->FillRect({H.X + 76.0f, Y + 52.0f, H.W - 104.0f, 1.0f}, Hex(0xeef2f7));
		}
		C->SetAlpha(A0);
	}
}

// ------------------------------------------------------------------ sleep and time

void RiverLine::SleepMenu(double Now)
{
	const Rect R{360.0f, RiverLine::Height - 38.0f - 228.0f, 380.0f, 220.0f};
	if (UI.Ptr.Released && !UI.Hover(R) && UI.Ptr.Y < RiverLine::Height - 38.0f)
	{
		SleepOpen = false;
		return;
	}
	C->GlowRoundRect({R.X, R.Y + 8.0f, R.W, R.H}, 16.0f, Rgba(0, 0, 0, 0.55f), 22.0f);
	UI.RRect(R, 16.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, Hex(0x1b1433), Hex(0x110c22)), Hex(0x3b2a6b));
	AppMoon(*C, R.X + 34.0f, R.Y + 36.0f, 12.0f, Hex(0xd8c8ff), Hex(0x1a1331));
	UI.Text(S.Evicted() ? "Dee's couch" : "Sleep", R.X + 58.0f, R.Y + 43.0f, Ts(20.0f, 900, Hex(0xeee6ff)));
	UI.Text("Energy " + std::to_string(static_cast<int>(std::round(S.Life.Energy))) + "%", R.X + R.W - 22.0f, R.Y + 42.0f, Ts(14.0f, 700, Hex(0xb9a6e8), Align::Right));
	const char* Ids[2] = {"nap", "sleep"};
	const life::Context Ctx = S.LifeContext();
	for (int I = 0; I < 2; ++I)
	{
		const life::Activity* A = life::Find(Ids[I]);
		if (!A)
		{
			continue;
		}
		const std::string Why = life::Blocked(*A, S.Life, Ctx);
		const std::string Label = A->Title + " \xC2\xB7 " + std::to_string(static_cast<int>(A->Hours)) + " hours";
		// What it actually gives back: more with the mattress and the curtains, less on a couch.
		const int Gain = static_cast<int>(std::round(std::min(100.0, -A->Energy * S.RestFactor())));
		const std::string Sub = Why.empty() ? "Wake at " + net::TimeLabel(World + A->Hours * 60.0) + " \xC2\xB7 +" + std::to_string(Gain) + " energy" : Why;
		if (AppButton(std::string("sleep:") + Ids[I], {R.X + 18.0f, R.Y + 66.0f + Nf(I) * 72.0f, R.W - 36.0f, 60.0f}, Label, Hex(0x7c3aed), Hex(0xffffff), Why.empty(), Sub))
		{
			TryActivity(Ids[I], Now);
		}
	}
}

void RiverLine::SkipOverlay(double Now)
{
	const Session::Skip& K = S.TimeSkip;
	const float P = Nf(Clamp01((Now - K.RealStart) / std::max(0.1, K.RealSeconds)));
	const float Day = Nf(S.Daylight());
	C->FillRect({0.0f, 0.0f, NetW, RiverLine::Height}, Rgba(2, 5, 10, 0.92f));
	const Color Sky = Mix(Hex(0x1e2a6b), Hex(0xffb347), Day);
	C->FillEllipse(800.0f, 420.0f, 620.0f, 420.0f, Paint::Radial({800.0f, 420.0f}, 0.0f, {800.0f, 420.0f}, 620.0f, NetA(Sky, 0.28f), 0.5f, NetA(Sky, 0.07f), NetA(Sky, 0.0f)));
	// Clock face.
	const float Cx = 800.0f;
	const float Cy = 400.0f;
	const float R = 140.0f;
	C->FillCircle(Cx, Cy, R, Rgba(255, 255, 255, 0.03f));
	C->StrokeEllipse(Cx, Cy, R, R, Rgba(255, 255, 255, 0.18f), 2.0f);
	for (int I = 0; I < 12; ++I)
	{
		const float A = Nf(I) * Pi / 6.0f;
		const float In = I % 3 == 0 ? R - 22.0f : R - 12.0f;
		C->StrokePolyline({{Cx + std::cos(A) * In, Cy + std::sin(A) * In}, {Cx + std::cos(A) * (R - 4.0f), Cy + std::sin(A) * (R - 4.0f)}}, false, Rgba(255, 255, 255, I % 3 == 0 ? 0.7f : 0.3f), I % 3 == 0 ? 3.0f : 2.0f);
	}
	C->StrokeArc(Cx, Cy, R + 18.0f, -Pi / 2.0f, -Pi / 2.0f + 2.0f * Pi * std::max(0.01f, P), Mix(Hex(0x8b5cf6), Hex(0xf2c14e), Day), 6.0f, true);
	const double Tod = std::fmod(World, net::MinutesPerDay);
	const float Ha = Nf(Tod / 720.0) * 2.0f * Pi - Pi / 2.0f;
	const float Ma = Nf(std::fmod(Tod, 60.0) / 60.0) * 2.0f * Pi - Pi / 2.0f;
	C->StrokePolyline({{Cx, Cy}, {Cx + std::cos(Ha) * R * 0.5f, Cy + std::sin(Ha) * R * 0.5f}}, false, Hex(0xffffff), 7.0f, true);
	C->StrokePolyline({{Cx, Cy}, {Cx + std::cos(Ma) * R * 0.82f, Cy + std::sin(Ma) * R * 0.82f}}, false, NetA(Hex(0xffffff), 0.8f), 4.0f, true);
	C->FillCircle(Cx, Cy, 8.0f, Hex(0xffffff));
	if (Day > 0.5f)
	{
		AppSun(*C, Cx, Cy - R - 70.0f, 26.0f, Hex(0xffd27a));
	}
	else
	{
		AppMoon(*C, Cx, Cy - R - 70.0f, 20.0f, Hex(0xd8c8ff), Hex(0x05080f));
	}
	UI.Text(net::TimeLabel(World), Cx, Cy + R + 86.0f, Ts(54.0f, 700, Hex(0xffffff), Align::Center, Baseline::Alphabetic, true));
	UI.Text(std::string(net::WeekdayName(net::DayOf(World), true)) + ", " + net::DateLabel(net::DayOf(World)), Cx, Cy + R + 116.0f, Ts(16.0f, 600, pal::Muted, Align::Center));
	UI.Text(K.Label + std::string(static_cast<size_t>(1 + static_cast<int>(Now * 2.0) % 3), '.'), Cx, Cy + R + 170.0f, Ts(24.0f, 800, pal::Ink, Align::Center));
	UI.RRect({Cx - 220.0f, Cy + R + 196.0f, 440.0f, 6.0f}, 3.0f, Rgba(255, 255, 255, 0.08f));
	UI.RRect({Cx - 220.0f, Cy + R + 196.0f, std::max(6.0f, 440.0f * P), 6.0f}, 3.0f, Mix(Hex(0x8b5cf6), Hex(0xf2c14e), Day));
}

void RiverLine::OutcomeCard(double Now)
{
	const life::Outcome& O = S.LastOutcome;
	const life::Activity* A = life::Find(O.ActivityId);
	const life::Kind K = A ? A->Type : life::Kind::Sleep;
	const Color Tone = O.Bad ? pal::Red : K == life::Kind::Job ? Hex(0xff8a1f) : K == life::Kind::Hustle ? Hex(0x2fd27a) : K == life::Kind::Ghost ? pal::Gold : Hex(0x8b5cf6);
	C->FillRect({0.0f, 0.0f, NetW, RiverLine::Height}, Rgba(2, 5, 10, 0.72f));
	const Rect R{470.0f, 270.0f, 660.0f, 380.0f};
	C->GlowRoundRect({R.X, R.Y + 10.0f, R.W, R.H}, 20.0f, NetA(Tone, 0.25f), 30.0f);
	UI.RRect(R, 20.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, Mix(Hex(0x111c2e), Tone, 0.14f), Hex(0x0d1729)), NetA(Tone, 0.6f), 1.5f);
	NetSpaced(*C, A ? NetUpper(A->Place == "Bed" ? std::string(S.Evicted() ? "Dee's couch" : "Rest") : A->Place) : std::string(), R.X + 36.0f, R.Y + 46.0f, 12.0f, 900, Tone, 2.0f);
	UI.Text(O.Title, R.X + 36.0f, R.Y + 92.0f, Ts(38.0f, 900, pal::Ink));
	UI.Text(net::TimeLabel(O.Start) + "  \xE2\x86\x92  " + net::TimeLabel(O.End) + "  \xC2\xB7  " + net::Countdown(O.End - O.Start), R.X + 36.0f, R.Y + 120.0f, Ts(14.0f, 600, pal::Muted));
	NetParagraph(*C, O.Body, R.X + 36.0f, R.Y + 160.0f, R.W - 72.0f, 16.0f, 500, Hex(0xc9d3e2), 23.0f, 4);
	// What changed.
	const float Sy = R.Y + 250.0f;
	C->FillRect({R.X + 36.0f, Sy - 16.0f, R.W - 72.0f, 1.0f}, pal::Line);
	if (O.Money != 0)
	{
		UI.Text((O.Money > 0 ? "+" : "\xE2\x88\x92") + Money(O.Money > 0 ? O.Money : -O.Money), R.X + 36.0f, Sy + 30.0f, Ts(32.0f, 900, O.Money > 0 ? pal::Green : pal::Red, Align::Left, Baseline::Alphabetic, true));
	}
	else
	{
		UI.Text("$0.00", R.X + 36.0f, Sy + 30.0f, Ts(32.0f, 900, pal::Dim, Align::Left, Baseline::Alphabetic, true));
	}
	UI.Text("Balance " + Money(S.BankrollCents), R.X + 36.0f, Sy + 52.0f, Ts(13.0f, 600, pal::Muted));
	UI.Text("Energy " + std::to_string(static_cast<int>(std::round(S.Life.Energy))) + "%", R.X + 330.0f, Sy + 22.0f, Ts(16.0f, 800, O.Energy >= 0.0 ? pal::Green : pal::Gold));
	UI.Text((O.Energy >= 0.0 ? "+" : "\xE2\x88\x92") + std::to_string(static_cast<int>(std::round(std::fabs(O.Energy)))), R.X + 330.0f, Sy + 44.0f, Ts(13.0f, 600, pal::Muted));
	if (O.Heat != 0.0 || S.Life.Heat > 0.0)
	{
		UI.Text("Heat " + std::to_string(static_cast<int>(S.Life.Heat)), R.X + 480.0f, Sy + 22.0f, Ts(16.0f, 800, S.Life.Heat > 50.0 ? pal::Red : pal::Orange));
		UI.Text((O.Heat >= 0.0 ? "+" : "\xE2\x88\x92") + std::to_string(static_cast<int>(std::round(std::fabs(O.Heat)))), R.X + 480.0f, Sy + 44.0f, Ts(13.0f, 600, pal::Muted));
	}
	if (O.Debt > 0)
	{
		UI.Text("Owed to Marcus: " + Money(S.Life.DebtCents), R.X + 330.0f, Sy + 66.0f, Ts(13.0f, 700, pal::Red));
	}
	(void)Now;
	ButtonOpts B;
	B.Kind = O.Bad ? ButtonKind::Danger : ButtonKind::Primary;
	B.Size = 18.0f;
	if (UI.Button("outcome-ok", {R.X + R.W - 236.0f, R.Y + R.H - 76.0f, 200.0f, 52.0f}, "Continue", B))
	{
		S.HasOutcome = false;
	}
}
} // namespace ui
} // namespace ss
