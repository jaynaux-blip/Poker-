// The laptop's other apps: ShiftLink (gig shifts), Burner (Marcus and Sam), the bank (rent and history),
// sleep, and the time-lapse while a shift or a night goes by. They share the screen with RiverLine through
// the taskbar along the bottom.
#include "ShortStack/UI/RiverLine.h"
#include "../StrictFloat.h"
#include "RiverLineShared.h"

#include "ShortStack/Game/Chat.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/Stake.h"
#include "ShortStack/UI/BadgeArt.h"

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
	case RiverLine::App::Mail: return {"Mail", 0x2f5d9e, 0x1f2a3a};
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
	case 9: return Hex(0xec4899);
	case life::LedgerBacking: return Hex(0xf2a93b);  // Lantern Row's lantern
	case life::LedgerSponsors: return Hex(0x2f5d9e); // Mail's navy: the brands write there
	case life::LedgerCoaching: return Hex(0x0891b2);
	default: return Hex(0xf2c14e);
	}
}

/** The ledger's kinds as the Bank names them (life::LedgerEntry::Kind; next_segment_spec §1.2): each one, so none reads "Prizes". */
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
	case 9: return "Northside"; // the shops on the block (the barber, the thrift, the gym, Ruby's, the pawn)
	case life::LedgerBacking: return "Backing";   // stakes, pieces, swaps and their shares
	case life::LedgerSponsors: return "Sponsors"; // covers, stipends, cuts and bonuses
	case life::LedgerCoaching: return "Coaching";
	default: return "Prizes";
	}
}

/** life::State::Record keeps this many ledger lines (the newest). */
const size_t LedgerKept = 60;
/** The Embercrest's ember, for its lines in the Bank. */
const uint32_t EmberRgb = 0xff6a24;
/** The Bank's money in / money out rows: the first row's top in the card, and the pitch (nine In rows fit its 360). */
const float BankRowTop = 118.0f;
const float BankRowPitch = 26.0f;

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

/** Letters in the shared inbox the player hasn't opened (the Mail app marks a thread's letters read when it opens it). */
int UnreadLetters(const life::State& L)
{
	int N = 0;
	for (const life::Letter& Lt : L.Inbox)
	{
		N += Lt.Read ? 0 : 1;
	}
	return N;
}

/** A brand waiting on an answer: an offer (or a countered one) still standing at World. */
bool SponsorWaiting(const sponsor::Book& B, double World)
{
	for (const sponsor::Contract& K : B.Contracts)
	{
		if ((K.St == sponsor::State::Offered || K.St == sponsor::State::Countered) && K.ExpiresAt > World)
		{
			return true;
		}
	}
	return false;
}

/** Sam's side of the book, as the Burner shows it: his standing offer, his package while it runs, the debt a quit left. */
struct SamBook
{
	const stake::Offer* Offer = nullptr;
	const stake::Deal* Deal = nullptr;
	Chips Debt = 0;
};

SamBook SamOf(const stake::Book& B, double World)
{
	SamBook Out;
	for (const stake::Offer& O : B.Offers)
	{
		if (O.By == stake::Outfit::Sam && O.State == stake::OfferStanding && O.Expires > World)
		{
			Out.Offer = &O;
		}
	}
	for (const stake::Deal& D : B.Deals)
	{
		if (D.By != stake::Outfit::Sam)
		{
			continue;
		}
		if (D.K == stake::Kind::Package && D.State == stake::DealOpen)
		{
			Out.Deal = &D;
		}
		Out.Debt += std::max<Chips>(0, D.Debt);
	}
	return Out;
}

/** The sponsor's store credit as the Bank's card shows it: its own pot, spent only at the counter and on Penny Drop. */
struct BankCredit
{
	bool Shown = false; // credit on the books, or a running deal that grants it
	Chips Cents = 0;
	std::string From; // "Lucky Penny #212"
	std::string Sub;  // "+$25 every week"
};

BankCredit CreditOf(const sponsor::Book& B, double World)
{
	BankCredit Out;
	Out.Cents = B.StoreCreditCents;
	for (const sponsor::Contract& K : B.Contracts)
	{
		if (!sponsor::Active(B, K.Brand, World))
		{
			continue;
		}
		for (const sponsor::Perk& P : K.T.Perks)
		{
			if (P.Kind == sponsor::Gift::StoreCredit && P.Cents > 0)
			{
				const sponsor::Brand* Br = sponsor::FindBrand(K.Brand);
				Out.From = Br ? Br->Name : K.Brand;
				Out.Sub = "+" + NetMoney(P.Cents) + (P.EveryDays == 7 ? std::string(" every week") : " every " + std::to_string(P.EveryDays) + " days") +
					(K.CompsHalved ? " \xC2\xB7 half held for the cure" : std::string());
			}
		}
	}
	Out.Shown = Out.Cents > 0 || !Out.From.empty();
	if (Out.From.empty())
	{
		// Credit left on the books after the deal: still good where it was given.
		const sponsor::Brand* Penny = sponsor::FindBrand("luckypenny");
		Out.From = Penny ? Penny->Name : std::string("The corner store");
		Out.Sub = "Left on the books";
	}
	return Out;
}

/**
 * One of the pots beside the balance (the staked roll, the store credit): white, with its colour down the side. Short
 * (two stacked, 84 tall): the kicker and whose, the amount, a line. Tall (alone): the foot line too.
 */
void BankPot(Canvas& Cv, Ui& UI, const Rect& R, const std::string& Kicker, const std::string& From, const std::string& Big, const std::string& Sub, const Color& SubInk,
	const Color& Accent, const std::string& Foot)
{
	const Color Dark = Hex(0x0f1d33);
	const Color Gray = Hex(0x6b7a90);
	const bool Tall = R.H >= 140.0f;
	Cv.GlowRoundRect({R.X, R.Y + 6.0f, R.W, R.H}, 16.0f, Rgba(13, 37, 71, 0.1f), 14.0f);
	UI.RRect(R, 16.0f, Hex(0xffffff), Hex(0xe2e8f0));
	Cv.FillRoundRect({R.X, R.Y + 10.0f, 5.0f, R.H - 20.0f}, 2.5f, Accent);
	const float X = R.X + 22.0f;
	const float W = R.W - 40.0f;
	const float Kw = NetSpaced(Cv, Kicker, X, R.Y + (Tall ? 34.0f : 24.0f), 10.5f, 900, Mix(Accent, Dark, 0.35f), 1.6f);
	if (Tall)
	{
		UI.Text(From, X, R.Y + 58.0f, Ts(13.0f, 700, Dark, Align::Left, Baseline::Alphabetic, false, W));
	}
	else
	{
		UI.Text(From, X + W, R.Y + 24.0f, Ts(12.0f, 700, Gray, Align::Right, Baseline::Alphabetic, false, W - Kw - 14.0f));
	}
	UI.Text(Big, X, R.Y + (Tall ? 104.0f : 54.0f), Ts(Tall ? 34.0f : 24.0f, 900, Dark, Align::Left, Baseline::Alphabetic, true, W));
	UI.Text(Sub, X, R.Y + (Tall ? 130.0f : 74.0f), Ts(12.5f, 700, SubInk, Align::Left, Baseline::Alphabetic, false, W));
	if (Tall)
	{
		UI.Text(Foot, X, R.Y + R.H - 22.0f, Ts(12.0f, 500, Gray, Align::Left, Baseline::Alphabetic, false, W));
	}
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
	case App::Mail:
		// An envelope: the sheet and its flap.
		C->FillRoundRect({Cx - Sz * 0.26f, Cy - Sz * 0.17f, Sz * 0.52f, Sz * 0.34f}, Sz * 0.04f, W);
		C->StrokePolyline({{Cx - Sz * 0.24f, Cy - Sz * 0.14f}, {Cx, Cy + Sz * 0.04f}, {Cx + Sz * 0.24f, Cy - Sz * 0.14f}}, false, Hex(B.Col), Sz * 0.05f, true);
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
	const App Apps[8] = {App::RiverLine, App::ShiftLink, App::Burner, App::Bank, App::GearDrop, App::Kast, App::PennyDrop, App::Mail};
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
			// Live: a pulsing red dot. A sponsor waiting on an answer (the offer's in Mail): gold. Locked until the PC can
			// stream: no dot, a padlock.
			if (S.Streaming())
			{
				Dot = NetA(pal::Red, 0.6f + 0.4f * Nf(std::sin(Now * 4.0)));
			}
			else if (SponsorWaiting(S.Life.Sponsors, World))
			{
				Dot = pal::Gold;
			}
			else if (!S.GearFx().CanStream())
			{
				NetLockIcon(*C, Btn.X + 24.0f, Btn.Y - 1.0f, 10.0f, Hex(0xd8ccff));
			}
		}
		if (A == App::Mail && UnreadLetters(S.Life) > 0)
		{
			// Unread mail: the envelope's own blue, brightened to read on the dark bar.
			Dot = Hex(0x6fa8ff);
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
	// Sam's package lives on the Burner (he never texts the phone, next_segment_spec §1.4): his lines, his standing offer,
	// the deal while it runs, and the debt if the player quit in makeup.
	const SamBook Sam = SamOf(L.Stakes, Ctx.World);
	const bool SamKnown = Ctx.Cashes > 0 || !L.Stakes.Burner.empty();
	struct Contact
	{
		const char* Name;
		std::string Preview;
		uint32_t Col;
		bool Known;
	};
	const std::string SamPreview = !SamKnown ? std::string("No messages yet")
		: Sam.Debt > 0                      ? "debt's " + NetMoney(Sam.Debt) + ". 25% til its paid"
		: !L.Stakes.Burner.empty()          ? L.Stakes.Burner.back().second
		: L.Bans > 0                        ? std::string("lay low for a day")
											: std::string("i pay ppl to play my account");
	const Contact Contacts[3] = {
		{"Marcus", L.DebtCents > 0 ? "pay up before you get more work." : L.Runs >= 2 ? "got a bigger one when you're ready." : "I got work if you're not scared.", 0x2fd27a, true},
		{"Sam", SamPreview, 0xf2c14e, SamKnown},
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
		UI.Text(Contacts[I].Preview, R.X + 80.0f, R.Y + 58.0f, Ts(13.0f, 500, I == 1 && Sam.Debt > 0 ? Hex(0xff9aa5) : Dim, Align::Left, Baseline::Alphabetic, false, 240.0f));
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
		// His package: what he's said about it since, in order (the book keeps his last twenty lines).
		for (const std::pair<double, std::string>& Line : L.Stakes.Burner)
		{
			Thread.push_back({false, Line.second});
		}
	}
	else
	{
		// Dee: the game across the street, and what she thinks of how you play in it.
		Thread = {{false, "it's Dee. heard about the notice on your door."},
			{false, "the game's still on. back room at the Wash & Fold, across the street. one-two no limit, tuesdays, thursdays, saturdays."},
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
	// Dee's offers are the two games and the Embercrest's next events: a taller strip.
	const float OffersTop = AppH - (BurnerContact == 2 ? 330.0f : 260.0f);
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
		// Dee's two games: the back room's, with a buy-in from the bankroll, and the Embercrest's daily freezeouts.
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
		const Rect R{380.0f, OffersTop + 24.0f, (TotalW - Gap) * 0.4f, 282.0f};
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
			// The Embercrest: a casino's blue, the next two events the bus can still make (each with the room's own facts,
			// who's down to play and what it costs against the rent), and the whole week a click away on RiverLine.
			const Color Blue = Hex(Rv->Color);
			UI.RRect(Rr, 16.0f, Hex(0x0b1119), NetA(Blue, 0.45f));
			UI.Text(Rv->Title, Rr.X + 24.0f, Rr.Y + 36.0f, Ts(20.0f, 800, Text));
			{
				const std::string Week = "The week on RiverLine  \xE2\x80\xBA";
				const float Ww = UI.Measure(Week, 13.0f, 700) + 28.0f;
				const Rect Wr{Rr.X + Rr.W - 20.0f - Ww, Rr.Y + 16.0f, Ww, 28.0f};
				const Ui::ClickState Ws = UI.Clickable("liveweek", Wr);
				UI.RRect(Wr, 14.0f, NetA(Blue, Ws.Hover ? 0.24f : 0.1f), NetA(Blue, 0.5f));
				UI.Text(Week, Wr.X + Ww / 2.0f, Wr.Y + 19.0f, Ts(13.0f, 700, Ws.Hover ? Text : Blue, Align::Center));
				if (Ws.Clicked)
				{
					OpenApp(App::RiverLine, Now);
					S.ShowLobby();
					OpenPage(Page::Embercrest, Now);
				}
			}
			const std::string Record = L.LiveEvents == 0 ? std::string("6-max freezeouts every day, sixty to a hundred and twenty runners. Registration at the desk.")
				: "Played " + std::to_string(L.LiveEvents) + "  \xC2\xB7  cashed " + std::to_string(L.LiveCashes) +
					(L.LiveBestPlace > 0 ? "  \xC2\xB7  best " + Ordinal(L.LiveBestPlace) : std::string()) + "  \xC2\xB7  won " + Money(L.LiveWonCents);
			UI.Text(Record, Rr.X + 24.0f, Rr.Y + 58.0f, Ts(12.5f, 500, Dim, Align::Left, Baseline::Alphabetic, false, Rr.W - 48.0f));
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
			for (const live::Occurrence& O : LiveAhead(S, Ctx.World, 30.0))
			{
				if (Events.size() < 2 && (Events.empty() || Events.front().Id != O.Id))
				{
					Events.push_back(O);
				}
			}
			float Ry = Rr.Y + 70.0f;
			for (const live::Occurrence& O : Events)
			{
				LiveRow(O, {Rr.X + 20.0f, Ry, Rr.W - 40.0f, 104.0f}, Blue, Text, Dim, Now);
				Ry += 104.0f;
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
	// Sam's package (next_segment_spec §2.7) beside his account work: the offer with its strings spelled out before you
	// sign, the deal while it runs, or the debt a quit in makeup left. The Backing page has the same offer with a counter.
	if (BurnerContact == 1 && (Sam.Offer || Sam.Deal || Sam.Debt > 0))
	{
		const Rect R{380.0f + Nf(Offers.size()) * 600.0f, OffersTop + 24.0f, 580.0f, 214.0f};
		const Color Gold = Hex(0xf2c14e);
		const bool Owing = !Sam.Offer && !Sam.Deal;
		UI.RRect(R, 16.0f, Owing ? Hex(0x1f0c10) : Hex(0x15110a), Owing ? NetA(pal::Red, 0.6f) : NetA(Gold, 0.45f));
		if (const stake::Offer* O = Sam.Offer)
		{
			UI.Text("His package", R.X + 26.0f, R.Y + 40.0f, Ts(20.0f, 800, Text));
			UI.Text("stands " + net::Countdown(std::max(0.0, O->Expires - Ctx.World)), R.X + R.W - 26.0f, R.Y + 38.0f, Ts(12.5f, 600, Dim, Align::Right));
			const float Ty = NetParagraph(*C, stake::TermsLine(*O), R.X + 26.0f, R.Y + 66.0f, R.W - 52.0f, 12.5f, 600, Text, 17.0f, 3);
			// The string that matters, on its own line before he gets a yes (§2.7: spelled out before you sign).
			NetParagraph(*C, "Quit in makeup and it's a debt: 25% of every cash and shift until it's paid.", R.X + 26.0f, Ty + 4.0f, R.W - 52.0f, 12.5f, 800, pal::Orange, 17.0f, 1);
			const float Btn = R.Y + R.H - 62.0f;
			// The id by value: answering changes the book's offers, and O points into them.
			const std::string OfferId = O->Id;
			if (AppButton("sam-accept", {R.X + 26.0f, Btn, 250.0f, 46.0f}, "Take it", Gold, Hex(0x1d1204), true))
			{
				const std::string Not = S.AcceptStake(OfferId);
				Toast = Not;
				ToastAt = Not.empty() ? -100.0 : Now;
			}
			else if (AppButton("sam-pass", {R.X + 290.0f, Btn, 264.0f, 46.0f}, "Leave him on read", Hex(0x2a2416), Text, true))
			{
				const std::string Not = stake::Decline(S.Life.Stakes, OfferId, Ctx.World);
				S.Save();
				Toast = Not;
				ToastAt = Not.empty() ? -100.0 : Now;
			}
		}
		else if (const stake::Deal* D = Sam.Deal)
		{
			const Chips Makeup = std::max<Chips>(0, D->Funded - D->RollNow);
			UI.Text("His package", R.X + 26.0f, R.Y + 40.0f, Ts(20.0f, 800, Text));
			UI.Text("Roll " + NetMoney(D->RollNow), R.X + R.W - 26.0f, R.Y + 42.0f, Ts(20.0f, 900, Gold, Align::Right, Baseline::Alphabetic, true));
			UI.Text(stake::DealLine(*D), R.X + 26.0f, R.Y + 66.0f, Ts(13.0f, 600, Dim, Align::Left, Baseline::Alphabetic, false, R.W - 52.0f));
			// Makeup carries its sign (never colour alone).
			UI.Text(Makeup > 0 ? "\xE2\x88\x92" + NetMoney(Makeup) + " makeup" : std::string("No makeup"), R.X + 26.0f, R.Y + 104.0f,
				Ts(18.0f, 800, Makeup > 0 ? pal::Red : BurnerGreen, Align::Left, Baseline::Alphabetic, true));
			UI.Text(Makeup > 0 ? std::string("Quit now and it's a debt: 25% of every cash and every shift until it's paid.") : std::string("Quit now and you owe him nothing."), R.X + 26.0f,
				R.Y + 130.0f, Ts(12.5f, 700, Makeup > 0 ? pal::Orange : Dim, Align::Left, Baseline::Alphabetic, false, R.W - 52.0f));
			UI.Text("Chop " + net::DateLabel(net::DayOf(D->NextChop)) + " \xC2\xB7 no chops on request \xC2\xB7 the deal's on RiverLine's Backing page", R.X + 26.0f, R.Y + 186.0f,
				Ts(12.0f, 600, Dim, Align::Left, Baseline::Alphabetic, false, R.W - 52.0f));
		}
		else
		{
			// Quit in makeup: the debt, garnished from every cash and shift.
			UI.Text("You owe Sam", R.X + 32.0f, R.Y + 50.0f, Ts(16.0f, 700, Hex(0xff9aa5)));
			UI.Text(Money(Sam.Debt), R.X + 32.0f, R.Y + 104.0f, Ts(46.0f, 900, Hex(0xffffff), Align::Left, Baseline::Alphabetic, true));
			NetParagraph(*C, "He takes 25% of every cash and every shift until it's paid. You walked out in makeup, and that was the deal.", R.X + 32.0f, R.Y + 140.0f, R.W - 64.0f,
				13.5f, 500, Hex(0xff9aa5), 19.0f, 2);
		}
	}
}

// ------------------------------------------------------------------ the Embercrest's event cards

namespace backing_detail
{
// RiverLineBacking.cpp: whether the sell panel takes a live event (and keeps it among its picks when preselected).
bool LiveSellable(const Session& S, const std::string& OccurrenceId, double World);
} // namespace backing_detail

namespace rlnet_detail
{
std::vector<live::Occurrence> LiveAhead(const Session& S, double World, double HoursAhead)
{
	std::vector<live::Occurrence> Out = live::Reachable(World, HoursAhead);
	Out.erase(std::remove_if(Out.begin(), Out.end(), [&S](const live::Occurrence& O) { return S.Living().CancellationOf(O.Id) != nullptr; }), Out.end());
	return Out;
}

bool LiveForSale(const Session& S, const std::string& OccurrenceId, double World)
{
	// The Backing page's own rule (RiverLineBacking.cpp): reachable this week, not entered, nobody else paying. The page keeps
	// a preselected event among its picks, so the link always lands on it, however far down the week it is.
	return backing_detail::LiveSellable(S, OccurrenceId, World);
}

LiveCard LiveCardOf(const Session& S, const live::Occurrence& O, double World, int PlannedField)
{
	LiveCard Card;
	Card.O = O;
	if (!O.Valid())
	{
		return Card;
	}
	const live::EventTemplate& T = *O.T;
	const life::State& L = S.Life;
	const int Today = net::DayOf(World);
	Card.Entry = live::EntryFor(L, O.Id);
	Card.Registered = Card.Entry && Card.Entry->State == life::LiveEntry::Registered;
	Card.When = O.Day == Today ? "Today" : O.Day == Today + 1 ? "Tomorrow" : O.Day == Today - 1 ? "Yesterday" : net::WeekdayName(O.Day, true);
	Card.Split = NetMoney(T.BuyInCents - T.FeeCents) + " + " + NetMoney(T.FeeCents) + " fee";
	const std::vector<Level>& Levels = live::DeepstackLevels();
	Card.StackBb = Levels.empty() ? 0 : static_cast<int>(T.StartingStack / std::max<Chips>(1, Levels.front().Bb));
	// The field, and a deep run with it: the entry's (fixed when the player registered, them included), else the world's
	// plan with the player in it, else the room's usual crowd for it.
	int Entrants = (T.FieldLo + T.FieldHi) / 2 + 1;
	if (Card.Entry && Card.Entry->Entrants > 0)
	{
		Card.Field = Entrants = Card.Entry->Entrants;
		Card.Planned = true;
	}
	else if (PlannedField > 0)
	{
		Card.Field = PlannedField;
		Card.Planned = true;
		Entrants = PlannedField + 1;
	}
	LiveRun(Card, Entrants);

	// Where the night stands, for the player: the desk is theirs once the bus would get them there with it open, and
	// late registration only while the bus still makes it (live::CanRegister's windows).
	if (World + live::TravelMinutes < O.DeskOpens)
	{
		Card.Phase = LiveCard::Soon;
		Card.Status = "Desk opens " + net::TimeLabel(O.DeskOpens);
	}
	else if (World < O.Start)
	{
		Card.Phase = LiveCard::Desk;
		Card.Status = "Registering \xC2\xB7 cards at " + net::TimeLabel(O.Start);
	}
	else if (World + live::TravelMinutes <= O.LateRegEnds)
	{
		Card.Phase = LiveCard::LateReg;
		Card.Status = "Late reg till " + net::TimeLabel(O.LateRegEnds);
	}
	else
	{
		Card.Phase = World < Card.Ends ? LiveCard::Closed : LiveCard::Over;
		Card.Status = Card.Phase == LiveCard::Closed ? "Registration closed" : "Played out";
		// The world's result, once it has decided the night.
		if (const net::EventResult* Res = S.Living().ResultOf(O.Id))
		{
			for (const net::Placing& P : Res->FinalTable)
			{
				if (P.Place == 1)
				{
					const world::Npc* N = P.Player >= 0 ? S.Living().Get(P.Player) : nullptr;
					Card.Phase = LiveCard::Over;
					Card.Status = "Won by " + (P.Player == -1 ? S.HeroName : N ? N->Name : P.Name) + " \xC2\xB7 " + NetMoney(P.Prize);
				}
			}
		}
	}
	// Called off short of its minimum when its cards should have gone in the air (World::Cancel): nobody plays it, and
	// the desk takes no entries for it.
	if (const world::Cancellation* Off = S.Living().CancellationOf(O.Id))
	{
		Card.Cancelled = true;
		Card.Phase = LiveCard::Over;
		Card.Field = Off->Field;
		Card.Planned = true;
		Card.Status = "Cancelled: " + std::to_string(Off->Field) + " came, " + std::to_string(T.MinEntrants) + " needed";
	}
	if (Card.Entry)
	{
		const life::LiveEntry& E = *Card.Entry;
		Card.Status = E.State == life::LiveEntry::Refunded ? std::string("Cancelled \xC2\xB7 refunded")
			: E.State == life::LiveEntry::Finished ? Ordinal(E.Place) + " of " + Grouped(E.Entrants) + (E.PrizeCents > 0 ? " \xC2\xB7 " + NetMoney(E.PrizeCents) : std::string())
			: E.ArrivedAt > 0.0 ? std::string("You're in: the night's under way")
								: std::string("You're registered");
	}

	// Who pays (next_segment_spec §1.2): the entry's funding once it's registered; before then, the way GoToLive would
	// register it right now (the first of Session::FundingOptions that can), and what that leaves the bankroll to find.
	if (Card.Entry)
	{
		Card.Fund = Card.Entry->Fund;
	}
	else if (!Card.Cancelled && Card.Phase <= LiveCard::LateReg)
	{
		for (const std::pair<life::Funding, std::string>& Option : S.FundingOptions(O.Id))
		{
			if (Option.second.empty())
			{
				Card.Fund = Option.first;
				break;
			}
		}
		// The pieces on it, and whether more can go up for sale (the Backing page's own picks: the player's money, the
		// next events the bus can make).
		Card.Asked = stake::AskedOn(L.Stakes, O.Id);
		Card.Filled = stake::FilledOn(L.Stakes, O.Id, World);
		Card.Sellable = Card.Fund.From == life::Paid::Own && LiveForSale(S, O.Id, World);
		// A sponsor's cover that takes it (it pays only if GoToLive picks it: a backer's stake comes first).
		for (const std::pair<life::Funding, std::string>& Option : sponsor::CoverOptions(L.Sponsors, O, World))
		{
			const sponsor::Contract* K = Option.second.empty() ? sponsor::Find(L.Sponsors, Option.first.Ref) : nullptr;
			if (K && Card.Cover.empty())
			{
				const sponsor::Brand* Br = sponsor::FindBrand(K->Brand);
				const int Allowed = K->CompsHalved ? K->T.CoveredEntries / 2 : K->T.CoveredEntries;
				Card.Cover = "Covered by " + (Br ? (Br->Short.empty() ? Br->Name : Br->Short) : K->Brand) + " \xC2\xB7 " + std::to_string(std::max(0, Allowed - K->Now.CoveredUsed)) + " of " +
					std::to_string(Allowed) + " left";
			}
		}
	}
	Card.FundLine = live::FundingLine(Card.Fund, T.Short);
	Card.Cost = LiveCostWith(O, Card.Fund);

	// Why Session::GoToLive would turn it down right now (the same checks).
	if (S.TimeSkip.Active)
	{
		Card.Why = "You're busy.";
	}
	else if (S.LifeContext().InTournament)
	{
		Card.Why = "Finish your tables first.";
	}
	else if (!Card.Entry && Card.Cancelled)
	{
		Card.Why = "That one was cancelled.";
	}
	else if (!Card.Registered && Card.Phase >= LiveCard::Closed)
	{
		Card.Why = "Registration's closed.";
	}
	else if (!Card.Entry && live::ActiveEntry(L))
	{
		Card.Why = "One entry at a time.";
	}
	else if (!Card.Registered)
	{
		Card.Why = live::CanRegister(S.BankrollCents, L, O, World, Card.Fund);
		const life::Activity* A = life::Find("embercrest");
		if (Card.Why.empty() && A && L.Energy < A->Energy)
		{
			Card.Why = "Too tired. Sleep first.";
		}
		else if (Card.Phase == LiveCard::Soon && net::DayOf(O.DeskOpens) != Today)
		{
			// CanRegister's desk hour, with its day: a card can be days out.
			const int DeskDay = net::DayOf(O.DeskOpens);
			Card.Why = "Desk opens " + (DeskDay == Today + 1 ? std::string("tomorrow") : std::string(net::WeekdayName(DeskDay, true))) + ", " + LiveTime(O.DeskOpens) + ".";
		}
	}

	auto Warn = [&Card](const std::string& Text, int Severity) { Card.Warnings.push_back({Text, Severity}); };
	// A field short of the minimum is called off at cards-up and the buy-in refunded (the bus fare isn't): the entry's
	// field as it was registered, or the world's plan with the player in it.
	if (Card.Planned && !Card.Cancelled && Card.Phase <= LiveCard::LateReg && (Card.Registered || !Card.Entry) && live::BelowMinimum(O, Card.Entry ? Card.Field : Card.Field + 1))
	{
		Warn(Card.Entry ? "Only " + std::to_string(Card.Field) + " in the field: under " + std::to_string(T.MinEntrants) + " the room calls it off and refunds the buy-in."
						: "Only ~" + std::to_string(Card.Field) + " are down for it: under " + std::to_string(T.MinEntrants) + " it's called off and the buy-in refunded.",
			1);
	}

	// What it costs against the money: only for a night the player could still enter.
	if (Card.Entry || Card.Phase >= LiveCard::Closed)
	{
		return Card;
	}
	const Chips Have = S.BankrollCents;
	const Chips Cost = Card.Cost;
	const Chips After = Have - Cost;
	if (Have < Cost)
	{
		const bool Own = Card.Fund.From == life::Paid::Own;
		const std::string What = !Own ? "the bus is " : Card.Fund.Cents > 0 ? "your end of the buy-in and the bus are " : "the buy-in and the bus are ";
		Warn("You're " + NetMoney(Cost - Have) + " short: " + What + NetMoney(Cost) + ".", 2);
	}
	// The landlord collects at the deadline if the money is there: a night before then spends the rent's money, and a
	// night after it is paid from what the rent leaves.
	const Chips Rent = L.RentDueCents;
	if (L.RentStage == life::Rent::Due || L.RentStage == life::Rent::FinalNotice)
	{
		const int DueDay = net::DayOf(L.RentDeadline - 1.0);
		const int Days = DueDay - Today;
		const std::string By = Days <= 0 ? "tonight" : Days == 1 ? "tomorrow" : "in " + std::to_string(Days) + " days";
		const std::string Head = L.RentStage == life::Rent::FinalNotice
			? "Final notice: " + NetMoney(Rent) + " by midnight " + (Days <= 1 ? By : std::string(net::WeekdayName(DueDay, true)))
			: "Rent is due " + By + " (" + NetMoney(Rent) + ")";
		if (O.Start >= L.RentDeadline)
		{
			const Chips Left = Have - Rent;
			if (Have < Rent)
			{
				Warn(Head + ", before this. You're " + NetMoney(Rent - Have) + " short of it.", 1);
			}
			else if (Left < Cost)
			{
				Warn(Head + ", before this. That leaves " + NetMoney(Left) + ", " + NetMoney(Cost - Left) + " short of this.", 1);
			}
			else
			{
				Warn(Head + ", before this. That leaves " + NetMoney(Left) + ", enough for this.", 0);
			}
		}
		else if (Have < Cost)
		{
			Warn(Head + ".", 1);
		}
		else if (After >= Rent)
		{
			Warn(Head + ". You'd still have it after this.", 0);
		}
		else if (Have >= Rent)
		{
			Warn(Head + ". After this you'd be " + NetMoney(Rent - After) + " short.", 1);
		}
		else
		{
			Warn(Head + ". You're " + NetMoney(Rent - Have) + " short already; this makes it " + NetMoney(Rent - After) + ".", 1);
		}
	}
	else if (L.RentStage == life::Rent::Evicted && Have >= Cost)
	{
		Warn("A key back is " + NetMoney(Rent) + (After >= Rent ? std::string(". You'd still have it after this.") : ". After this you'd be " + NetMoney(Rent - After) + " short of it."),
			After >= Rent ? 0 : 1);
	}
	std::stable_sort(Card.Warnings.begin(), Card.Warnings.end(), [](const LiveWarning& A, const LiveWarning& B) { return A.Severity > B.Severity; });
	return Card;
}
} // namespace rlnet_detail

const RiverLine::LivePlan& RiverLine::PlanOf(const live::Occurrence& O)
{
	// Looked up again each minute, and whenever the world changes (it plans, registers and plays as the clock runs).
	world::World& W = S.Living();
	const long long Key = static_cast<long long>(std::floor(World)) * 1000000LL + static_cast<long long>(W.Revision() % 1000000);
	if (Key != LivePlansKey)
	{
		LivePlansKey = Key;
		LivePlans.clear();
	}
	const auto Found = LivePlans.find(O.Id);
	if (Found != LivePlans.end())
	{
		return Found->second;
	}
	LivePlan& Plan = LivePlans[O.Id];
	// The world plans a day ahead (World::EnsurePlanned): further out nobody has said they're going yet.
	if (!W.Ready() || !O.Valid() || O.Day > net::DayOf(World) + 1)
	{
		return Plan;
	}
	const std::vector<int> Who = W.Registered(O.Id);
	if (Who.empty())
	{
		return Plan;
	}
	Plan.Field = W.PlannedEntries(O.Id);
	// The people in it the player knows, the strongest feelings first.
	static const char* Order[6] = {"Rival", "Holds a grudge", "Respects you", "Trusts you", "Knows you well", "Knows you"};
	struct Face
	{
		int Rank;
		float Familiarity;
		std::string Name;
		std::string Label;
	};
	std::vector<Face> Faces;
	for (const int Npc : Who)
	{
		const world::Npc* N = W.Get(Npc);
		const world::Bond* Bd = W.BondWith(Npc);
		const std::string Label = Bd ? Bd->Label() : std::string();
		if (!N || Label.empty() || N->Name == S.HeroName)
		{
			continue;
		}
		int Rank = 6;
		for (int K = 0; K < 6; ++K)
		{
			Rank = Label == Order[K] ? K : Rank;
		}
		Faces.push_back({Rank, Bd->Familiarity, N->Name, Label});
	}
	std::stable_sort(Faces.begin(), Faces.end(), [](const Face& A, const Face& B) { return A.Rank != B.Rank ? A.Rank < B.Rank : A.Familiarity > B.Familiarity; });
	for (const Face& F : Faces)
	{
		Plan.Known.push_back({F.Name, F.Label});
	}
	return Plan;
}

LiveCard RiverLine::CardFor(const live::Occurrence& O)
{
	const LivePlan& Plan = PlanOf(O);
	LiveCard Card = LiveCardOf(S, O, World, Plan.Field);
	Card.Known = Plan.Known;
	return Card;
}

namespace rlapps_detail
{
/** The field as a card states it: the entry's, the world's plan, or the room's usual crowd for the event. */
std::string LiveFieldText(const LiveCard& Card)
{
	if (Card.Entry && Card.Planned)
	{
		return Grouped(Card.Field) + " in the field";
	}
	if (Card.Planned)
	{
		return "~" + Grouped(Card.Field) + " expected";
	}
	return "usually " + std::to_string(Card.O.T->FieldLo) + " to " + std::to_string(Card.O.T->FieldHi);
}

Color LiveWarningColor(int Severity, const Color& Note)
{
	return Severity >= 2 ? pal::Red : Severity == 1 ? pal::Orange : Note;
}
} // namespace rlapps_detail

void RiverLine::LiveRow(const live::Occurrence& O, const Rect& R, const Color& Tint, const Color& Ink, const Color& Faint, double Now)
{
	if (!O.Valid())
	{
		return;
	}
	const LiveCard Card = CardFor(O);
	const live::EventTemplate& T = *O.T;
	C->FillRect({R.X, R.Y, R.W, 1.0f}, NetA(Tint, 0.18f));
	// When, and where it stands.
	UI.Text(net::TimeLabel(O.Start), R.X + 4.0f, R.Y + 26.0f, Ts(16.0f, 800, Card.Registered ? Tint : Ink));
	UI.Text(Card.When, R.X + 4.0f, R.Y + 44.0f, Ts(12.0f, 600, Faint));
	const std::string Stage = Card.Registered ? std::string("YOU'RE IN") : Card.Phase == LiveCard::Soon ? "DESK " + NetUpper(LiveTime(O.DeskOpens))
		: Card.Phase == LiveCard::Desk ? std::string("REGISTERING") : Card.Phase == LiveCard::LateReg ? std::string("LATE REG") : std::string("CLOSED");
	NetSpaced(*C, Stage, R.X + 4.0f, R.Y + 62.0f, 9.5f, 800, Card.Registered || Card.Phase == LiveCard::LateReg ? Tint : Faint, 1.0f);
	// The room's facts, in the order a player weighs them.
	const float Mx = R.X + 98.0f;
	const float Bw = 188.0f;
	const float Bx = R.X + R.W - Bw - 4.0f;
	const float Short = Bx - Mx - 12.0f;
	const float Full = R.X + R.W - 4.0f - Mx;
	UI.Text(std::string(T.Short) + "  " + NetMoney(T.BuyInCents), Mx, R.Y + 26.0f, Ts(16.0f, 800, Ink, Align::Left, Baseline::Alphabetic, false, Short));
	UI.Text(Card.Split + "  \xC2\xB7  " + Grouped(T.StartingStack) + " chips (" + std::to_string(Card.StackBb) + " BB)  \xC2\xB7  " + std::to_string(static_cast<int>(T.LevelMinutes)) + "-min levels",
		Mx, R.Y + 44.0f, Ts(12.0f, 500, Faint, Align::Left, Baseline::Alphabetic, false, Short));
	UI.Text("Breaks " + LiveBreaks(T) + "  \xC2\xB7  late reg to " + net::TimeLabel(O.LateRegEnds) + "  \xC2\xB7  runs with " + std::to_string(T.MinEntrants) + "+  \xC2\xB7  deep run ~" +
			LiveHours(Card.RunMinutes / 60.0),
		Mx, R.Y + 61.0f, Ts(12.0f, 500, Faint, Align::Left, Baseline::Alphabetic, false, Full));
	{
		// The bus, the field, and the regulars down for it ("you know Sal and Rick +3"), each name with the one glyph the
		// room would show by it (next_segment_spec §5.6: Room scope, 12 units).
		const Color LineInk = Mix(Faint, Ink, 0.45f);
		const TextStyle Style = Ts(12.0f, 600, LineInk);
		const float End = Mx + Full;
		float Lx = Mx + UI.Text("The 14 bus: " + NetMoney(live::BusFareCents) + ", 20 min each way  \xC2\xB7  " + LiveFieldText(Card), Mx, R.Y + 78.0f,
			Ts(12.0f, 600, LineInk, Align::Left, Baseline::Alphabetic, false, Full));
		if (!Card.Known.empty() && Lx + 120.0f < End)
		{
			Lx += UI.Text("  \xC2\xB7  you know ", Lx, R.Y + 78.0f, Style);
			const world::World& W = S.Living();
			const size_t Names = std::min<size_t>(2, Card.Known.size());
			size_t Said = 0;
			for (; Said < Names; ++Said)
			{
				const std::string Name = (Said == 0 ? std::string() : Said + 1 == Card.Known.size() ? std::string(" and ") : std::string(", ")) + Card.Known[Said].first;
				const int Who = W.Find(Card.Known[Said].first);
				const badge::Badge* B = Who >= 0 ? badge::Top(NetBadgesOf(S, Who), badge::Room, badge::FrameNone) : nullptr;
				if (Said > 0 && Lx + UI.Measure(Name, 12.0f, 600) + (B ? 16.0f : 0.0f) > End - 28.0f)
				{
					break;
				}
				Lx += UI.Text(Name, Lx, R.Y + 78.0f, Ts(12.0f, 600, LineInk, Align::Left, Baseline::Alphabetic, false, std::max(10.0f, End - Lx)));
				if (B && Lx + 16.0f <= End)
				{
					badgeart::Draw(*C, *B, Lx + 8.0f, R.Y + 73.5f, 12.0f, Now);
					Lx += 16.0f;
				}
			}
			if (Said < Card.Known.size() && Lx + 20.0f <= End)
			{
				UI.Text(" +" + std::to_string(Card.Known.size() - Said), Lx, R.Y + 78.0f, Style);
			}
		}
	}
	// The most serious warning, or why it can't be entered right now, or who's paying for it.
	const std::string Paying = Card.Registered || (!Card.Entry && Card.Phase <= LiveCard::LateReg) ? LivePaying(Card) : std::string();
	if (!Card.Warnings.empty() && (Card.Warnings.front().Severity >= 1 || Paying.empty()))
	{
		const LiveWarning& Wn = Card.Warnings.front();
		UI.Text(Wn.Text, Mx, R.Y + 95.0f, Ts(12.0f, 700, LiveWarningColor(Wn.Severity, Mix(Faint, Ink, 0.45f)), Align::Left, Baseline::Alphabetic, false, Full));
	}
	else if (!Card.Why.empty() && !Card.Entry)
	{
		UI.Text(Card.Why, Mx, R.Y + 95.0f, Ts(12.0f, 600, Faint, Align::Left, Baseline::Alphabetic, false, Full));
	}
	else if (!Paying.empty())
	{
		UI.Text(Paying, Mx, R.Y + 95.0f, Ts(12.0f, 700, Tint, Align::Left, Baseline::Alphabetic, false, Full));
	}
	// Registering is the session's (the same as RiverLine's Embercrest page); an entry already played shows how it went.
	if (Card.Entry && !Card.Registered)
	{
		UI.Text(Card.Status, R.X + R.W - 4.0f, R.Y + 30.0f, Ts(14.0f, 800, Card.Entry->PrizeCents > 0 ? pal::Gold : Faint, Align::Right));
		return;
	}
	const bool Ok = Card.Why.empty();
	const std::string Label = Card.Registered ? std::string("Go back") : Card.Fund.From == life::Paid::Own ? "Register " + NetMoney(T.BuyInCents) : LiveRegisterLabelOf(Card, live::BusFareCents).Text;
	if (AppButton("live:" + O.Id, {Bx, R.Y + 8.0f, Bw, 38.0f}, Label, Tint, Hex(0x06101a), Ok, Ok ? std::string() : Card.Why))
	{
		GoToLive(O.Id, Now);
	}
}

void RiverLine::GoToLive(const std::string& OccurrenceId, double Now)
{
	LiveConfirm.clear();
	const std::string Why = S.GoToLive(OccurrenceId);
	if (!Why.empty())
	{
		Toast = Why;
		ToastAt = Now;
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

	// The pots that aren't the balance (next_segment_spec §1.2: never in the bankroll, never in Net, never the rent): a
	// package's staked roll and a sponsor's store credit, each on its own small card beside the balance.
	const stake::Deal* Package = stake::ActivePackage(L.Stakes);
	const BankCredit Credit = CreditOf(L.Sponsors, World);
	const int Pots = (Package ? 1 : 0) + (Credit.Shown ? 1 : 0);
	const float PotW = 304.0f;
	const float PotGap = 16.0f;

	// Balance.
	const Rect Bal{40.0f, 100.0f, Pots > 0 ? 860.0f - PotW - PotGap : 860.0f, 180.0f};
	C->GlowRoundRect({Bal.X, Bal.Y + 8.0f, Bal.W, Bal.H}, 18.0f, Rgba(13, 37, 71, 0.25f), 20.0f);
	C->FillRoundRect(Bal, 18.0f, Paint::Linear({Bal.X, Bal.Y}, {Bal.X + Bal.W, Bal.Y + Bal.H}, Hex(0x1d4ed8), Navy));
	C->PushClip(Bal);
	C->FillEllipse(Bal.X + Bal.W - 120.0f, Bal.Y + 30.0f, 260.0f, 200.0f, Rgba(255, 255, 255, 0.06f));
	C->PopClip();
	NetSpaced(*C, "CHECKING \xC2\xB7\xC2\xB7\xC2\xB7 4471", Bal.X + 32.0f, Bal.Y + 42.0f, 12.0f, 800, Hex(0xbcd0f0), 2.0f);
	UI.Text(Money(static_cast<Chips>(static_cast<double>(S.BankrollCents) * static_cast<double>(In))), Bal.X + 32.0f, Bal.Y + 112.0f, Ts(58.0f, 900, Hex(0xffffff), Align::Left, Baseline::Alphabetic, true));
	UI.Text("Available balance \xC2\xB7 linked to RiverLine", Bal.X + 32.0f, Bal.Y + 146.0f, Ts(14.0f, 600, Hex(0xbcd0f0)));
	if (Pots > 0)
	{
		const float PotH = Pots == 2 ? (Bal.H - 12.0f) / 2.0f : Bal.H;
		float Py = Bal.Y;
		if (Package)
		{
			const stake::Deal& D = *Package;
			const Chips Makeup = stake::MakeupOf(L.Stakes);
			// Makeup carries its sign (never colour alone): "−$680 makeup".
			const std::string Sub = Makeup > 0 ? "\xE2\x88\x92" + NetMoney(Makeup) + " makeup \xC2\xB7 not a debt" : "No makeup \xC2\xB7 chop " + net::DateLabel(net::DayOf(D.NextChop));
			BankPot(*C, UI, {Bal.X + Bal.W + PotGap, Py, PotW, PotH}, "STAKED ROLL", stake::BackerName(D), Money(D.RollNow), Sub, Makeup > 0 ? Hex(0xdc2626) : Gray, Hex(0xf2a93b),
				"Theirs: it never pays the rent.");
			Py += PotH + 12.0f;
		}
		if (Credit.Shown)
		{
			BankPot(*C, UI, {Bal.X + Bal.W + PotGap, Py, PotW, PotH}, "STORE CREDIT", Credit.From, Money(Credit.Cents), Credit.Sub, Gray, Hex(0x2f5d9e),
				"At the counter and on Penny Drop. Never the rent.");
		}
	}

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
	// In: 0 tournament cashes, 1 the Embercrest, 2 leaderboard prizes, 3 shifts, 4 cash work, 5 streaming, 6 sponsors
	// (covers, stipends and bonuses, less a cut or a clawback), 7 backers and buyers (a stake, the pieces sold), 8
	// Northside's sales and refunds (the pawn's cash, a loan).
	// Out: 0 buy-ins (less refunds), 1 rent, bills and fares, 2 food and deliveries, 3 gear, 4 fines and Marcus, 5 the
	// shops on the block (cuts, clothes, the gym, Ruby's, the pawn), 6 backers' shares and swaps, 7 coaching.
	Chips Ins[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
	Chips Outs[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	for (const life::LedgerEntry& E : L.Ledger)
	{
		if (E.Kind == 7 || E.Amount == 0)
		{
			continue; // transfers (savings from home) are neither earned nor spent
		}
		if (E.Kind == life::LedgerSponsors)
		{
			Ins[6] += E.Amount; // netted: a cut or a clawback comes off what they paid (floored below)
			continue;
		}
		if (E.Kind == life::LedgerCoaching)
		{
			Outs[7] -= E.Amount; // netted the same way: a refund comes off what it cost
			continue;
		}
		if (E.Amount > 0)
		{
			if (E.Kind == life::LedgerBacking)
			{
				Ins[7] += E.Amount;
			}
			else if (E.Kind == 0 && E.Label.find(": refunded") != std::string::npos)
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
			else if (E.Kind == 9)
			{
				Ins[8] += E.Amount;
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
		case 9: Outs[5] += Spent; break; // the shops on the block
		case life::LedgerBacking: Outs[6] += Spent; break; // a backer's share, a buyer's piece, a swap, a stake handed back
		default: Outs[1] += Spent; break; // rent, the storage unit, the bus to the Embercrest
		}
	}
	Outs[0] = std::max<Chips>(0, Outs[0]);
	Ins[6] = std::max<Chips>(0, Ins[6]);
	Outs[7] = std::max<Chips>(0, Outs[7]);
	struct FlowRow
	{
		const char* Label;
		Chips Amount;
		uint32_t Col;
	};
	const FlowRow InRows[9] = {{"Tournament cashes", Ins[0], 0x27d3c3}, {"The Embercrest", Ins[1], EmberRgb}, {"Leaderboard prizes", Ins[2], 0xf2c14e}, {"Shifts", Ins[3], 0xff8a1f},
		{"Cash work", Ins[4], 0x16a34a}, {"Streaming", Ins[5], 0x9b5cff}, {"Sponsors", Ins[6], 0x2f5d9e}, {"Backers & buyers", Ins[7], 0xf2a93b}, {"Sales & refunds", Ins[8], 0xec4899}};
	const FlowRow OutRows[8] = {{"Buy-ins & entry fees", Outs[0], 0x0d9488}, {"Rent, bills & fares", Outs[1], 0xef4d5a}, {"Food & deliveries", Outs[2], 0xe23b4e}, {"Gear", Outs[3], 0xff6b2c},
		{"Fines & Marcus", Outs[4], 0x7c3aed}, {"Looks, fitness & food out", Outs[5], 0xdb2777}, {"Backers' shares & swaps", Outs[6], 0xd97706}, {"Coaching", Outs[7], 0x0891b2}};
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
			// Nine rows in the card at a 26-unit pitch (next_segment_spec §1.9 #23: 118 + 8 x 26 + 25 stays inside its 360).
			const FlowRow& R = Rows[I];
			const float Y = Flow.Y + BankRowTop + Nf(I) * BankRowPitch;
			const bool Some = R.Amount > 0;
			UI.Text(R.Label, X, Y + 13.0f, Ts(13.0f, 600, Some ? Dark : NetA(Gray, 0.7f)));
			UI.Text(Some ? Money(R.Amount) : std::string("\xE2\x80\x94"), X + W, Y + 13.0f, Ts(13.0f, 800, Some ? Dark : NetA(Gray, 0.6f), Align::Right, Baseline::Alphabetic, true));
			UI.RRect({X, Y + 18.0f, W, 6.0f}, 3.0f, Hex(0xeef2f7));
			const float Bw = W * Nf(static_cast<double>(R.Amount) / static_cast<double>(Top)) * In;
			if (Some && Bw > 0.5f)
			{
				UI.RRect({X, Y + 18.0f, std::max(6.0f, Bw), 6.0f}, 3.0f, Hex(R.Col));
			}
		}
	};
	Column(InRows, 9, Flow.X + 32.0f, 380.0f, "MONEY IN", InSum, Hex(0x16a34a));
	Column(OutRows, 8, Flow.X + 448.0f, 380.0f, "MONEY OUT", OutSum, Hex(0xdc2626));

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
