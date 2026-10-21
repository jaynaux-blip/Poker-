// Update 3 (SPEC §2.4, §2.6, §7.2): Ignite's REWARDS and FUSE pages, ui-ignite's file: the Prize Counter, the Flame, Dares,
// the Spark Wheel, Crackers, the Wristband, the level-up banner, the Locker and the Sticker Book, the FUSE booth, its
// reveal and the 3-max table's marquee. This file has no RiverLine member function of its own: it reaches the client's
// private state and its gatekeepers through rlignite_detail::Access (RiverLine.h's friend; its forwarders are in IgniteArt.h).
// G1 (§2.9): REWARDS' Welcome bonus sub-page with the Fuel Gauge (§2.6 F11); the loops and their sub-pages are G2.
#include "ShortStack/UI/RiverLine.h"
#include "../StrictFloat.h"
#include "../Game/SitesDetail.h"
#include "IgniteArt.h"
#include "RiverLineShared.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Sites.h"

#include <algorithm>
#include <cmath>

namespace ss
{
namespace ui
{
namespace rlignite_detail
{
void IgniteRewards(RiverLine& RL, IgniteUi& U, const igniteart::IgniteTones& K, double Now, bool Preview)
{
	using namespace igniteart;
	Session& S = Access::S(RL);
	Canvas& C = Access::C(RL);
	Ui& UI = RL.UI;
	const double World = Access::World(RL);
	const float In = Preview ? 1.0f : static_cast<float>(EaseOutCubic(Clamp01((Now - Access::PageAt(RL)) / 0.6)));
	// The sub-pills: at G1 the Welcome bonus alone (Today, the Wristband and the Prize Counter open with the loops, G2).
	U.Rewards = 3;
	{
		const char* Label = "Welcome bonus";
		const float W = C.Measure(Label, 15.0f, 800) + 40.0f;
		const Rect R{24.0f, 88.0f, W, 40.0f};
		const Ui::ClickState St = UI.Clickable("rw3", R, !Preview);
		(void)St;
		IgnitePill(C, R, Hex(K.Coral), Color(), 0.0f);
		C.Text(Label, R.X + W / 2.0f, R.Y + 26.0f, IgniteTx(15.0f, 800, Hex(0x2a0d14), Align::Center));
		C.Text("REWARDS", 1576.0f, 122.0f, IgniteBi(32.0f, Hex(K.Cream), Align::Right));
	}
	const site::Account* A = rlsites_detail::SitesAccount(S, net::SiteId::Ignite);
	const IgniteWelcome W = IgniteWelcomeOf(S, World);
	const auto Card = [&](const Rect& R) {
		C.FillRoundRect({R.X, R.Y + 5.0f, R.W, R.H}, 26.0f, Rgba(0, 0, 0, 0.3f));
		C.FillRoundRect(R, 26.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, Hex(K.PanelTop), Hex(K.PanelBot)));
		C.StrokeRoundRect(R, 26.0f, Hex(K.Rim), 1.0f);
	};
	// The welcome bonus: what's matched, the release rule, the bar, released and pending, the window; the Fuel Gauge.
	{
		const Rect P{24.0f, 148.0f, 1552.0f, 420.0f};
		Card(P);
		C.FillRoundRect(P, 26.0f, Paint::Linear({P.X, 0.0f}, {P.X + P.W, 0.0f}, Hex(K.Coral, 0.12f), Hex(IgViolet, 0.04f)));
		const float Lx = P.X + 40.0f;
		C.Text("WELCOME BONUS", Lx, P.Y + 50.0f, IgniteBi(17.0f, Hex(K.Coral)));
		C.Text("100% of your first deposit, up to $100", Lx, P.Y + 96.0f, IgniteBi(32.0f, Hex(K.Cream), Align::Left, Baseline::Alphabetic, 920.0f));
		rlnet_detail::NetParagraph(C, IgniteBonusRule, Lx, P.Y + 130.0f, 900.0f, 16.0f, 500, IgniteA(Hex(K.Cream), 0.82f), 22.0f, 2);
		const Rect Bar{Lx, P.Y + 196.0f, 920.0f, 18.0f};
		C.FillRoundRect(Bar, 9.0f, Hex(K.Well));
		if (W.Started && !W.Closed && W.Released > 0)
		{
			const float Frac = static_cast<float>(Clamp01(static_cast<double>(W.Released) / static_cast<double>(W.Released + W.Pending)));
			C.FillRoundRect({Bar.X, Bar.Y, std::max(18.0f, Bar.W * Frac * In), Bar.H}, 9.0f, Paint::Linear({Bar.X, 0.0f}, {Bar.X + Bar.W, 0.0f}, Hex(IgTang), Hex(IgLime)));
		}
		if (W.Started)
		{
			C.Text(Money(W.Released) + " released", Lx, P.Y + 262.0f, IgniteBi(24.0f, Hex(IgLime)));
			const std::string State = W.Pending > 0 ? Money(W.Pending) + " still pending"
				: W.Closed ? rlsites_detail::SitesFill(IgniteWindowShut, {{"Date", net::DateLabel(net::DayOf(W.Ends))}})
						   : std::string("All of it's yours now.");
			C.Text(State, Lx + 300.0f, P.Y + 260.0f, IgniteTx(16.0f, 700, Hex(K.Cream), Align::Left, Baseline::Alphabetic, 640.0f));
			if (W.Pending > 0 && A)
			{
				const double Left = A->BonusExpires - World;
				C.Text(Left > 0.0 ? "Expires " + std::string(net::WeekdayName(net::DayOf(A->BonusExpires), true)) + ", " + net::DateLabel(net::DayOf(A->BonusExpires)) + " \xC2\xB7 " + net::Countdown(Left) + " left"
								  : std::string("Expired"),
					Lx, P.Y + 300.0f, IgniteTx(14.0f, 700, Left > 0.0 && Left < 7.0 * net::MinutesPerDay ? Hex(IgTang) : pal::Muted));
				C.Text("$5 for every $20 of rake \xC2\xB7 " + Money(A->RakeLife) + " of rake paid here so far", Lx, P.Y + 330.0f, IgniteTx(14.0f, 600, pal::Muted));
			}
		}
		else
		{
			C.Text(IgniteStartsIt, Lx, P.Y + 262.0f, IgniteTx(18.0f, 700, Hex(K.Cream)));
		}
		// The Fuel Gauge: the needle at what's released of what was matched, and how far the next $5 is.
		const float Gx = P.X + P.W - 260.0f;
		const float Gy = P.Y + 250.0f;
		C.GlowRoundRect({Gx - 150.0f, Gy - 150.0f, 300.0f, 200.0f}, 100.0f, Hex(IgLime, 0.06f), 40.0f);
		IgniteFuelGauge(C, Gx, Gy, 130.0f, W.Started ? W.Needle * static_cast<double>(In) : 0.0, K);
		C.Text("FUEL GAUGE", Gx, Gy + 36.0f, IgniteTx(11.0f, 900, pal::Muted, Align::Center));
		if (W.Started && !W.Closed && !W.Done && W.NextIn > 0)
		{
			C.Text("next $5 in " + Money(W.NextIn) + " of rake", Gx, Gy + 76.0f, IgniteBi(19.0f, Hex(IgLime), Align::Center));
		}
		else if (W.Started)
		{
			C.Text(Money(W.Released) + " of " + Money(W.Released + W.Pending), Gx, Gy + 76.0f, IgniteBi(19.0f, Hex(IgLime), Align::Center));
		}
		else
		{
			C.Text("up to $100", Gx, Gy + 76.0f, IgniteBi(19.0f, Hex(IgLime), Align::Center));
		}
	}
	// Your game here, and the way to the Wallet.
	{
		const Rect P{24.0f, 584.0f, 1552.0f, 248.0f};
		Card(P);
		C.Text("YOUR GAME ON IGNITE", P.X + 40.0f, P.Y + 44.0f, IgniteBi(17.0f, Hex(K.Cream)));
		const std::pair<std::string, std::string> Facts[4] = {
			{A ? std::to_string(A->LifetimeEvents) : std::string("0"), "TOURNAMENTS HERE"},
			{A ? Money(A->MonthKey == site::MonthKeyAt(World) ? A->RakeMonth : 0) : Money(0), "RAKE THIS MONTH"},
			{A ? Money(A->RakeLife) : Money(0), "RAKE ALL TIME"},
			{A ? Money(A->LifetimeWon) : Money(0), "WON HERE"},
		};
		for (int I = 0; I < 4; ++I)
		{
			const float Fx = P.X + 40.0f + static_cast<float>(I) * 300.0f;
			C.Text(Facts[I].first, Fx, P.Y + 108.0f, IgniteBi(34.0f, I == 3 ? Hex(IgSun) : Hex(K.Cream)));
			C.Text(Facts[I].second, Fx, P.Y + 132.0f, IgniteTx(10.5f, 900, pal::Muted));
		}
		C.Text(IgniteStaysHere, P.X + 40.0f, P.Y + 190.0f, IgniteTx(15.0f, 600, pal::Muted));
		ButtonOpts O;
		O.Kind = ButtonKind::Primary;
		O.Size = 17.0f;
		O.Enabled = !Preview;
		if (rlsites_detail::SitesButton(UI, C, "rwwallet", {P.X + P.W - 300.0f, P.Y + 150.0f, 260.0f, 60.0f}, "Open the Wallet", O, rlsites_detail::SitesLook(net::SiteId::Ignite)))
		{
			Access::OpenPage(RL, RiverLine::Page::Cashier, Now);
		}
	}
}
} // namespace rlignite_detail
} // namespace ui
} // namespace ss
