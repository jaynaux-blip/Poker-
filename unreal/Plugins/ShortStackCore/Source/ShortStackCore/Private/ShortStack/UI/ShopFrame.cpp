// Northside's shop screens: the shared frame and MakeShopScreen (the realism update's spec, §7.4).
// Day 1 of the realism update: every venue opens a placeholder card (the venue's name, its hours from core, and a way
// out) so the host's dispatch, key routing and request queue can be built against ShopScreen.h before the pages land.
// The real frame (header, keeper's bubble, tabs, offer list, receipt, stamps, tip row, footers) replaces it in Phase B.
#include "ShortStack/UI/ShopScreen.h"
#include "FrontEndShared.h"

#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Shop.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace ui
{
namespace shopframe_detail
{
/** How long after opening a screen ignores everything but Escape (seconds), as StoreCounter::OpenGrace. */
constexpr double OpenGrace = 0.3;

std::string UpperAscii(std::string S)
{
	for (char& Ch : S)
	{
		if (Ch >= 'a' && Ch <= 'z')
		{
			Ch = static_cast<char>(Ch - 'a' + 'A');
		}
	}
	return S;
}

/** The venues MakeShopScreen knows, with the name shown until core's venue table has one. */
const char* FallbackName(const std::string& Venue)
{
	static const std::pair<const char*, const char*> Names[] = {
		{"barber", "Fifth St. Barbers"},
		{"thrift", "Second Chance Thrift"},
		{"gym", "IronHouse Gym"},
		{"diner", "Ruby's"},
		{"pawn", "Northside Pawn & Loan"},
		{"wardrobe", "Your Closet"},
	};
	for (const std::pair<const char*, const char*>& N : Names)
	{
		if (Venue == N.first)
		{
			return N.second;
		}
	}
	return nullptr;
}

/** The registry shot a screen asks for as it opens on Page (§7.4), or "" for none. */
std::string OpeningShot(const std::string& Venue, const std::string& Page)
{
	if (Venue == "barber")
	{
		return "barber-mirror";
	}
	if (Venue == "pawn")
	{
		if (Page == "pawn-cage" || Page == "TICKETS")
		{
			return "pawn-cage";
		}
		if (Page == "pawn-cases" || Page == "CURIOS")
		{
			return "pawn-case";
		}
		if (Page == "pawn-electronics" || Page == "BUY" || Page == "GEAR")
		{
			return "";
		}
		return "pawn-counter";
	}
	return "";
}

/** The placeholder: a card with the venue's name and hours, and a way out. Escape (or the card's button) walks away. */
class PlaceholderScreen final : public ShopScreen
{
public:
	PlaceholderScreen(std::string InVenue, const char* InName, Session& InSession) : S(InSession), Venue(std::move(InVenue)), Name(InName) {}

	void Open(double Now, const std::string& InPage) override
	{
		Shown = true;
		Left = false;
		OpenedAt = Now;
		Page = InPage;
		PressX = PressY = -1.0f;
		const std::string Shot = OpeningShot(Venue, Page);
		if (!Shot.empty())
		{
			HostRequest R;
			R.K = HostRequest::Camera;
			R.Name = Shot;
			Requests.push_back(R);
		}
	}

	void Close(double Now) override
	{
		Shown = false;
		OpenedAt = Now;
	}

	bool IsOpen() const override { return Shown; }

	bool TakeLeave() override
	{
		const bool Was = Left;
		Left = false;
		return Was;
	}

	void Key(const std::string& KeyName, double Now) override
	{
		if (!Shown)
		{
			return;
		}
		if (Now - OpenedAt < OpenGrace && KeyName != "Escape" && KeyName != "P")
		{
			return;
		}
		if (KeyName == "Escape" || KeyName == "P")
		{
			WalkAway(Now);
		}
	}

	std::vector<HostRequest> TakeRequests() override
	{
		std::vector<HostRequest> Out;
		Out.swap(Requests);
		return Out;
	}

	void Draw(Canvas& C, double Now) override
	{
		using namespace frontend_detail;
		if (!Shown)
		{
			Ptr.EndFrame();
			return;
		}
		const float W = C.Width();
		const float H = C.Height();
		const float Al = C.GetAlpha();
		const float In = static_cast<float>(std::clamp((Now - OpenedAt) / 0.2, 0.0, 1.0));
		C.SetAlpha(Al * In);

		// The shop stays in view behind a dimmed card.
		C.FillRect({0.0f, 0.0f, W, H}, Paint(Rgba(3, 4, 8, 0.55f)));
		const float Cw = std::min(720.0f, W - 2.0f * 48.0f);
		const float Ch = 300.0f;
		const Rect Card{(W - Cw) * 0.5f, (H - Ch) * 0.5f, Cw, Ch};
		C.GlowRoundRect(Card, 18.0f, Rgba(0, 0, 0, 0.6f), 40.0f);
		C.FillRoundRect(Card, 18.0f, Paint(MenuPanel));
		C.StrokeRoundRect(Card, 18.0f, Rgba(255, 255, 255, 0.08f), 1.5f);

		// The header: the venue's color, its name, OPEN or CLOSED and the hours line, all from core's venue table.
		const shop::Venue* V = shop::FindVenue(Venue);
		const std::string Title = V && !V->Name.empty() ? V->Name : Name;
		const Color Accent = V && V->Color != 0 ? Hex(V->Color) : MenuTeal;
		C.FillRoundRect({Card.X + 40.0f, Card.Y + 40.0f, 56.0f, 6.0f}, 3.0f, Paint(Accent));
		C.Text(UpperAscii(Title), Card.X + 40.0f, Card.Y + 98.0f, Ts(34.0f, 900, MenuInk, Align::Left, Baseline::Alphabetic, false, Card.W - 80.0f));
		float Ly = Card.Y + 136.0f;
		if (V)
		{
			const double World = S.WorldMinutes();
			const bool OpenNow = shop::IsOpen(*V, World);
			const std::string Badge = OpenNow ? "OPEN" : "CLOSED";
			const float Bw = C.Measure(Badge, 13.0f, 800) + 24.0f;
			C.FillRoundRect({Card.X + 40.0f, Ly - 19.0f, Bw, 26.0f}, 13.0f, Paint(OpenNow ? Hex(0x3ecf6e, 0.18f) : Hex(0xff5a5f, 0.18f)));
			C.Text(Badge, Card.X + 40.0f + Bw * 0.5f, Ly - 6.0f, Ts(13.0f, 800, OpenNow ? Hex(0x3ecf6e) : MenuWarn, Align::Center, Baseline::Middle));
			C.Text(shop::HoursLine(*V, World), Card.X + 40.0f + Bw + 14.0f, Ly, Ts(18.0f, 500, MenuMuted, Align::Left, Baseline::Alphabetic, false, Card.W - Bw - 94.0f));
			Ly += 44.0f;
		}
		else
		{
			Ly += 8.0f;
		}
		C.Text("Nobody's set up the counter yet.", Card.X + 40.0f, Ly, Ts(20.0f, 500, MenuMuted, Align::Left, Baseline::Alphabetic, false, Card.W - 80.0f));
		if (!Page.empty())
		{
			C.Text(Page, Card.X + 40.0f, Ly + 30.0f, Ts(14.0f, 500, MenuDim, Align::Left, Baseline::Alphabetic, true, Card.W - 80.0f));
		}

		// The way out: a key glyph and a label, and clicking them walks away too.
		if (Ptr.Pressed)
		{
			PressX = Ptr.X;
			PressY = Ptr.Y;
		}
		const float Fx = Card.X + 40.0f;
		const float Fy = Card.Y + Ch - 40.0f;
		const float Gw = Glyph(C, Gamepad ? "B" : "ESC", Fx, Fy, 1.0f);
		const float Tw = TrackedText(C, "WALK AWAY", Fx + Gw + 10.0f, Fy - 2.0f, 13.0f, 700, MenuMuted, 2.6f);
		const Rect Out{Fx - 8.0f, Fy - 29.0f, Gw + 10.0f + Tw + 16.0f, 40.0f};
		if (Ptr.Released && Inside(Out, Ptr.X, Ptr.Y) && Inside(Out, PressX, PressY))
		{
			WalkAway(Now);
		}

		C.SetAlpha(Al);
		Ptr.EndFrame();
	}

private:
	void WalkAway(double Now)
	{
		Left = true;
		Close(Now);
	}

	Session& S;
	std::string Venue;
	std::string Name;
	std::string Page;
	bool Shown = false;
	bool Left = false;
	double OpenedAt = -10.0;
	float PressX = -1.0f;
	float PressY = -1.0f;
	std::vector<HostRequest> Requests;
};
} // namespace shopframe_detail

std::unique_ptr<ShopScreen> MakeShopScreen(const std::string& Venue, Session& S)
{
	const char* Name = shopframe_detail::FallbackName(Venue);
	if (!Name)
	{
		return nullptr;
	}
	return std::make_unique<shopframe_detail::PlaceholderScreen>(Venue, Name, S);
}
} // namespace ui
} // namespace ss
