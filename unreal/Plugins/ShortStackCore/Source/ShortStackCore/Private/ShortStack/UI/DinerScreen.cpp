// Ruby's (the realism update's spec, §5.4 and §7.4): the menu card with what each dish does to the needs (DrawVitals),
// bottomless coffee, the paper (the world's headlines for the last day), "sit a while" with its hour picker, and the
// green guest check with the tip.
#include "ShopFrame.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Body.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Town.h"
#include "ShortStack/Game/Weather.h"
#include "ShortStack/Game/World.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <string>
#include <vector>

namespace ss
{
namespace ui
{
namespace shopframe
{
namespace
{
using namespace frontend_detail;

constexpr int MenuTab = 0;
constexpr int PaperTab = 1;
constexpr int SitTab = 2;

bool OnMenu(const shop::Offer& O)
{
	return O.Type == shop::OfferKind::Meal || O.Type == shop::OfferKind::Drink;
}

bool Coffee(const shop::Offer& O)
{
	return O.Id.find("coffee") != std::string::npos;
}

/** What a dish does, the line cook's boost included (store::ReliefOf's rule: food's hunger and anything's energy). */
NeedsChange NeedsOf(const shop::Offer& O, double MealBoost)
{
	NeedsChange N;
	N.Hunger = O.Type == shop::OfferKind::Meal && O.Hunger > 0.0 ? O.Hunger * MealBoost : O.Hunger;
	N.Thirst = O.Thirst;
	N.Energy = O.Energy > 0.0 ? O.Energy * MealBoost : O.Energy;
	return N;
}

class DinerPage final : public Page
{
public:
	explicit DinerPage(Session& InSession) : Page(InSession, "diner")
	{
		TipChoices = {0, 15, 18, 20, 25};
		TipIndex = TipDefault = 2; // 18%, the default (§5.0)
		Seated = true;
	}

protected:
	std::vector<std::string> TabNames() const override { return {"MENU", "THE PAPER", "SIT A WHILE"}; }

	void Opened(double Now, const std::string& PageId) override
	{
		Order_.clear();
		Paid.clear();
		PaidByLou = false;
		PaidTip = 0;
		PaidPct = 0;
		Hours = 2;
		PaperFirst = 0;
		Seat = PageId;
		if (PageId == "PAPER" || PageId == "THE PAPER")
		{
			Tab = PaperTab;
		}
		else if (PageId == "SIT" || PageId == "SIT A WHILE")
		{
			Tab = SitTab;
		}
		if (PageId.compare(0, 11, "diner-booth") == 0)
		{
			Camera("diner-booth");
		}
		else if (PageId.compare(0, 11, "diner-stool") == 0)
		{
			Camera("diner-counter");
		}
		(void)Now;
	}

	bool PageKey(const std::string& Name, double Now) override
	{
		if (Tab == SitTab)
		{
			if (Name == "Left" || Name == "Down")
			{
				Hours = std::max(1, Hours - 1);
				return true;
			}
			if (Name == "Right" || Name == "Up")
			{
				Hours = std::min(8, Hours + 1);
				return true;
			}
			if (Name == "Enter" || Name == "Space")
			{
				DoMain(Now);
				return true;
			}
			return false;
		}
		if (Tab == PaperTab)
		{
			if (Name == "Down" || Name == "Right")
			{
				PaperFirst = std::min(PaperFirst + 1, std::max(0, static_cast<int>(Headlines().size()) - 1));
				return true;
			}
			if (Name == "Up" || Name == "Left")
			{
				PaperFirst = std::max(0, PaperFirst - 1);
				return true;
			}
			return false;
		}
		const std::vector<const shop::Offer*> On = Menu();
		if (GridKey(Name, static_cast<int>(On.size()), Cols, Now))
		{
			return true;
		}
		if ((Name == "Enter" || Name == "Space") && Sel < static_cast<int>(On.size()))
		{
			Pick(*On[static_cast<size_t>(Sel)], Now);
			return true;
		}
		if ((Name == "Backspace" || Name == "Delete" || Name == "PutBack") && Sel < static_cast<int>(On.size()))
		{
			RemoveLine(On[static_cast<size_t>(Sel)]->Id, Now);
			return true;
		}
		return false;
	}

	void RemoveLine(const std::string& Id, double Now) override
	{
		const auto It = std::find(Order_.begin(), Order_.end(), Id);
		if (It != Order_.end())
		{
			Order_.erase(It);
			Touch(Id, Now);
			Tipping = false;
		}
	}

	std::vector<Hint> Hints() const override
	{
		if (Tab == SitTab)
		{
			return {{"LEFT", "DPAD", "HOURS"}, {"TAB", "Y", Order_.empty() ? "SIT" : "THE CHECK FIRST"}};
		}
		if (Tab == PaperTab)
		{
			return {{"DOWN", "DPAD", "READ ON"}, {"TAB", "Y", "ORDER"}};
		}
		return {{"UP", "DPAD", "CHOOSE"}, {"ENTER", "A", "ORDER"}, {"TAB", "Y", "THE CHECK"}};
	}

	void Body(Canvas& C, const Rect& R, double Now) override
	{
		if (Tab == PaperTab)
		{
			Newspaper(C, R, Now);
			return;
		}
		if (Tab == SitTab)
		{
			Sit(C, R, Now);
			return;
		}
		const std::vector<const shop::Offer*> On = Menu();
		Cols = R.W >= 1000.0f ? 3 : 2;
		if (On.empty())
		{
			C.Text("The menu's being wiped down. Give it a minute.", R.X, R.Y + 40.0f, Ts(18.0f, 500, MenuDim));
		}
		Grid(
			C, {R.X, R.Y, R.W, R.H - 140.0f}, static_cast<int>(On.size()), Cols, 128.0f,
			[&](int I, const Rect& Cell, bool Focus, bool Over) { Card(C, *On[static_cast<size_t>(I)], Cell, Focus, Over, Now); },
			[&](int I) { Pick(*On[static_cast<size_t>(I)], Now); }, Now);
		// What the order would do (or the dish in focus, before anything's ordered).
		NeedsChange Change;
		const std::vector<std::string>& What = Order_;
		for (const std::string& Id : What)
		{
			if (const shop::Offer* O = shop::FindOffer(Id))
			{
				const NeedsChange N = NeedsOf(*O, S.Life.Perks.MealBoost);
				Change.Hunger += N.Hunger;
				Change.Thirst += N.Thirst;
				Change.Energy += N.Energy;
			}
		}
		if (What.empty() && Sel < static_cast<int>(On.size()))
		{
			Change = NeedsOf(*On[static_cast<size_t>(Sel)], S.Life.Perks.MealBoost);
		}
		const float Vy = R.Y + R.H - 96.0f;
		TrackedText(C, What.empty() ? "WHAT IT WOULD DO" : "WHAT THE ORDER DOES", R.X, Vy - 18.0f, 11.0f, 800, MenuMuted, 2.6f);
		const float VitW = std::clamp(R.W * 0.46f, 360.0f, 520.0f);
		DrawVitals(C, S.Life, R.X, Vy, VitW, Now, Change);
		std::string Aside;
		if (LouPays())
		{
			Aside = "Big Lou's picking up your check tonight: you cashed, he eats, that's his rule. The tip's still yours.";
		}
		else if (S.Life.CoffeeUntil > S.WorldMinutes())
		{
			Aside = "Coffee's going: a refill every half hour, and the hours wear on you a quarter slower.";
		}
		else if (Change.Hunger >= 40.0)
		{
			Aside = "A real meal. Mom will want to hear about it.";
		}
		if (!Aside.empty())
		{
			Paragraph(C, Aside, R.X + VitW + 48.0f, Vy + 14.0f, R.X + R.W - (R.X + VitW + 48.0f), 14.0f, 500, MenuMuted, 19.0f, 3);
		}
	}

	Chips TipBase() override
	{
		Chips Sub = 0;
		for (const std::string& Id : Order_)
		{
			if (const shop::Offer* O = shop::FindOffer(Id))
			{
				Sub += O->PriceCents;
			}
		}
		return Sub;
	}

	Receipt Bill(double /*Now*/) override
	{
		const double World = S.WorldMinutes();
		Receipt R;
		R.Paper = PaperKind::GuestCheck;
		R.Title = "RUBY'S";
		const Keeper K = Who();
		std::string Where = "COUNTER";
		for (const char* Pre : {"diner-stool", "diner-booth"})
		{
			const std::string P = Pre;
			if (Seat.compare(0, P.size(), P) == 0)
			{
				Where = (P == "diner-booth" ? "BOOTH " : "STOOL ") + Seat.substr(P.size());
			}
		}
		R.Head = {"GUEST CHECK \xC2\xB7 201 MARKET ST.", Where + " \xC2\xB7 1 GUEST \xC2\xB7 SERVER " + Upper(K.Name.empty() ? std::string("RUBY") : K.Name), Upper(Stamped(World))};
		R.Number = std::to_string(4100 + (net::DayOf(World) * 53 + static_cast<int>(World) % 97) % 900);
		const std::vector<std::string>& List = Order_.empty() ? Paid : Order_;
		// Big Lou's night: the check says he got it, and what's left to pay is the tip (what the Session charges).
		const bool Lou = Order_.empty() ? PaidByLou : LouPays();
		Chips Sub = 0;
		Chips TaxedPart = 0;
		for (const std::string& Id : List)
		{
			if (const shop::Offer* O = shop::FindOffer(Id))
			{
				Line L;
				L.Id = Id;
				L.Left = O->Name;
				L.Right = Money(O->PriceCents);
				L.Removable = !Order_.empty();
				R.Lines.push_back(L);
				Sub += O->PriceCents;
				TaxedPart += O->Taxed ? O->PriceCents : 0;
			}
		}
		R.Empty = "What'll it be, hon?";
		if (!List.empty())
		{
			const Chips Tax = store::Tax(TaxedPart);
			R.Sums.push_back({"subtotal", "SUBTOTAL", "", Money(Sub)});
			R.Sums.push_back({"tax", "TAX 7.25%", "", Money(Tax)});
			if (Lou)
			{
				Line Got{"lou", "BIG LOU GOT IT", "", "\xE2\x88\x92" + Money(Sub + Tax)};
				Got.Red = true;
				R.Sums.push_back(Got);
			}
			if (Tipping)
			{
				const int Pct = TipPercent();
				R.Sums.push_back({"tip", Pct == 0 ? "NO TIP" : "TIP " + std::to_string(Pct) + "%", "", Money(TipAmount(Pct))});
			}
			else if (Order_.empty() && PaidTip > 0)
			{
				// The check as it was paid: the tip on it, whoever had the food.
				R.Sums.push_back({"tip", "TIP " + std::to_string(PaidPct) + "%", "", Money(PaidTip)});
			}
			R.Total = {"total", "TOTAL", "", Money(Order_.empty() ? (Lou ? 0 : Sub + Tax) + PaidTip : Total())};
		}
		R.Foot = {"THANK YOU \xC2\xB7 OPEN 24 HOURS", "CARD BALANCE " + Money(S.BankrollCents)};
		return R;
	}

	Button Main(double /*Now*/) override
	{
		Button B;
		if (Tab == SitTab)
		{
			// An order still on the check is settled on the menu first: sitting down never drops it without a word.
			B.Enabled = Order_.empty();
			B.Label = Order_.empty() ? "SIT UNTIL " + Upper(net::TimeLabel(S.WorldMinutes() + 60.0 * Hours)) : std::string("SETTLE THE CHECK FIRST");
			return B;
		}
		if (Order_.empty())
		{
			B.Label = "ORDER SOMETHING";
			return B;
		}
		B.Enabled = true;
		const bool Lou = LouPays();
		if (!Tipping)
		{
			B.Label = Lou ? std::string("THE CHECK \xC2\xB7 BIG LOU GOT IT") : "THE CHECK \xC2\xB7 " + Money(Total());
			return B;
		}
		const Chips Due = Total();
		B.Warn = Due > S.BankrollCents;
		B.Label = B.Warn ? std::string("CARD WON'T COVER IT") : !Lou ? "PAY " + Money(Due) : Due > 0 ? "PAY THE TIP " + Money(Due) : std::string("THANK BIG LOU");
		return B;
	}

	void DoMain(double Now) override
	{
		if (Tab == SitTab)
		{
			if (!Order_.empty())
			{
				SayText("Let me bring what you ordered first, hon. Check's on the menu page.", Now, true);
				return;
			}
			const std::string Why = S.WaitAt("diner", S.WorldMinutes() + 60.0 * Hours);
			if (!Why.empty())
			{
				SayText(Why, Now, true);
				return;
			}
			Say("sit", Now);
			Started(Now);
			return;
		}
		if (Order_.empty())
		{
			Say("browse", Now);
			return;
		}
		if (!Tipping)
		{
			Tipping = true;
			return;
		}
		const Chips Due = Total();
		const std::vector<std::string> Ordered = Order_;
		// Anything that takes a seat (a plate, the coffee) is a service: the order sits down with it (Shop.h).
		const bool Meal = std::any_of(Order_.begin(), Order_.end(), [](const std::string& Id) {
			const shop::Offer* O = shop::FindOffer(Id);
			return O && O->Minutes > 0;
		});
		const Session::ShopOrder O = Order("diner", Order_, TipPercent());
		const bool Lou = LouPays(); // read before the charge marks his night
		const int Pct = TipPercent();
		const Chips Tip = TipAmount(Pct); // (on the check, before it's cleared)
		const std::string Why = Meal ? S.ShopService(O) : S.ShopPay(O);
		if (!Why.empty())
		{
			Tipping = false;
			if (Due > S.BankrollCents)
			{
				Stamp(StampKind::Declined, Now);
				Say("declined", Now, true);
			}
			else
			{
				SayText(Why, Now, true);
			}
			return;
		}
		Paid = Ordered;
		PaidByLou = Lou;
		PaidTip = Tip;
		PaidPct = Pct;
		Order_.clear();
		Stamp(StampKind::Paid, Now);
		Tipping = false;
		if (!Started(Now))
		{
			Say(Pct > 0 ? "tip" : "stiff", Now);
		}
	}

private:
	/** A bottomless cup is going (Life.CoffeeUntil ahead): the refills come free while the hero sits. */
	bool Pouring() const { return S.Life.CoffeeUntil > S.WorldMinutes(); }

	/** Big Lou's picking up the check tonight (town::PaysYourCheck, once a night): the food and its tax are his. */
	bool LouPays() const { return town::PaysYourCheck(S, S.WorldMinutes()) == "big-lou"; }

	std::vector<const shop::Offer*> Menu() const
	{
		std::vector<const shop::Offer*> Out;
		for (const shop::Offer& O : shop::Offers("diner"))
		{
			if (OnMenu(O))
			{
				Out.push_back(&O);
			}
		}
		return Out;
	}

	Chips Total()
	{
		const Chips Core = S.ShopTotal(Order("diner", Order_, Tipping ? TipPercent() : 0));
		if (Core > 0 || (!Order_.empty() && LouPays()))
		{
			return Core; // (on Big Lou's night the Session's total is the tip alone, nothing at all without one)
		}
		Chips Sub = 0;
		Chips TaxedPart = 0;
		for (const std::string& Id : Order_)
		{
			if (const shop::Offer* O = shop::FindOffer(Id))
			{
				Sub += O->PriceCents;
				TaxedPart += O->Taxed ? O->PriceCents : 0;
			}
		}
		return Sub + store::Tax(TaxedPart) + (Tipping ? TipAmount(TipPercent()) : 0);
	}

	void Pick(const shop::Offer& O, double Now)
	{
		const auto It = std::find(Order_.begin(), Order_.end(), O.Id);
		Tipping = false;
		if (It != Order_.end())
		{
			Order_.erase(It);
			Touch(O.Id, Now);
			return;
		}
		const std::string Why = S.ShopBlocked(O.Id);
		if (!Why.empty())
		{
			// Bottomless means bottomless: a cup already going isn't rung up twice (the Session's rule, in her words). That
			// one isn't a refusal, so it isn't said in red.
			SayText(Why, Now, !(Coffee(O) && Pouring()));
			return;
		}
		if (Order_.empty())
		{
			Paid.clear();
			StampWas = StampKind::None;
		}
		Order_.push_back(O.Id);
		Touch(O.Id, Now);
		Say(Coffee(O) ? "coffee" : "pick", Now);
	}

	void Card(Canvas& C, const shop::Offer& O, const Rect& R, bool Focus, bool Over, double Now)
	{
		const bool On = std::find(Order_.begin(), Order_.end(), O.Id) != Order_.end();
		const std::string Why = S.ShopBlocked(O.Id);
		const bool Bottomless = Coffee(O);
		// A cup already going reads REFILLS FREE, not as a dish that can't be had.
		const bool Free = Bottomless && !On && Pouring();
		const bool Dim = !Why.empty() && !On && !Free;
		CardBack(C, R, Focus, Over, Dim, Now);
		const Color Acc = Accent("diner");
		const std::string Pr = Money(O.PriceCents);
		const float Pw = C.Measure(Pr, 18.0f, 900);
		C.Text(O.Name, R.X + 16.0f, R.Y + 32.0f, Ts(18.0f, 800, Dim ? MenuMuted : MenuInk, Align::Left, Baseline::Alphabetic, false, R.W - 44.0f - Pw));
		C.Text(Pr, R.X + R.W - 16.0f, R.Y + 32.0f, Ts(18.0f, 900, Dim ? MenuMuted : Acc, Align::Right));
		const std::string Blurb = !O.Blurb.empty() ? O.Blurb : Bottomless ? std::string("Bottomless while you're sitting.") : std::string();
		if (!Blurb.empty())
		{
			C.Text(Blurb, R.X + 16.0f, R.Y + 54.0f, Ts(13.0f, 400, Fade(MenuInk, 0.6f), Align::Left, Baseline::Alphabetic, false, R.W - 32.0f));
		}
		const NeedsChange N = NeedsOf(O, S.Life.Perks.MealBoost);
		NeedPills(C, N.Hunger, N.Thirst, N.Energy, R.X + 16.0f, R.Y + R.H - 38.0f, R.W - 32.0f - (On || Free ? 130.0f : 0.0f));
		if (On)
		{
			Badge(C, Bottomless ? "BOTTOMLESS" : "ORDERED", R.X + R.W - 16.0f - BadgeWidth(C, Bottomless ? "BOTTOMLESS" : "ORDERED"), R.Y + R.H - 36.0f, Mint, true);
		}
		else if (Free)
		{
			Badge(C, "REFILLS FREE", R.X + R.W - 16.0f - BadgeWidth(C, "REFILLS FREE"), R.Y + R.H - 36.0f, Mint);
		}
		else if (Dim)
		{
			const std::string W = BadgeText(C, Why, R.W * 0.55f, "NOT NOW");
			Badge(C, W, R.X + R.W - 16.0f - BadgeWidth(C, W), R.Y + R.H - 36.0f, MenuWarn);
		}
	}

	struct Story
	{
		std::string Title;
		std::string Body;
		std::string Tag;
		double At = 0.0;
	};

	/** The world's news from the last day, newest first (World::Headline). */
	std::vector<Story> Headlines() const
	{
		std::vector<Story> Out;
		const world::World& Wd = S.Living();
		const double World = S.WorldMinutes();
		const std::vector<world::WorldEvent>& Log = Wd.Events();
		for (auto It = Log.rbegin(); It != Log.rend() && Out.size() < 12; ++It)
		{
			if (It->At > World)
			{
				continue;
			}
			if (It->At < World - 1440.0)
			{
				break;
			}
			Story St;
			if (Wd.Headline(*It, St.Title, St.Body, St.Tag))
			{
				St.At = It->At;
				Out.push_back(St);
			}
		}
		return Out;
	}

	void Newspaper(Canvas& C, const Rect& R, double /*Now*/)
	{
		const double World = S.WorldMinutes();
		const int Day = net::DayOf(World);
		int Y = 0;
		int M = 0;
		int D = 0;
		world::CivilDate(Day, Y, M, D);
		const Rect P{R.X, R.Y, std::min(R.W, 1100.0f), R.H};
		const Color Newsprint = Hex(0xe9e4d6);
		const Color Ink = Hex(0x1d1b18);
		C.GlowRoundRect(P, 2.0f, Rgba(0, 0, 0, 0.5f), 22.0f);
		C.FillRect(P, Paint::Linear({P.X, P.Y}, {P.X + P.W, P.Y + P.H}, Newsprint, Hex(0xd9d2c0)));
		// The masthead.
		C.Text("THE NORTHSIDE COURIER", P.X + P.W * 0.5f, P.Y + 62.0f, Ts(44.0f, 900, Ink, Align::Center, Baseline::Alphabetic, false, P.W - 40.0f));
		C.FillRect({P.X + 24.0f, P.Y + 78.0f, P.W - 48.0f, 2.0f}, Paint(Ink));
		C.FillRect({P.X + 24.0f, P.Y + 108.0f, P.W - 48.0f, 1.0f}, Paint(Ink));
		C.Text(std::string(net::WeekdayName(Day, true)) + ", " + net::DateLabel(Day) + ", " + std::to_string(Y), P.X + 30.0f, P.Y + 99.0f, Ts(14.0f, 700, Ink));
		C.Text("LATE EDITION \xC2\xB7 CARDS & GAMES", P.X + P.W - 30.0f, P.Y + 99.0f, Ts(14.0f, 700, Ink, Align::Right));
		const std::vector<Story> All = Headlines();
		const bool Box = P.H >= 560.0f;
		if (Box)
		{
			Listings(C, P, Ink);
		}
		const float Bottom = P.Y + P.H - (Box ? 230.0f : 90.0f);
		// The weather box goes wherever the news leaves room above the listings.
		const float Floor = Box ? P.Y + P.H - 196.0f : P.Y + P.H - 20.0f;
		if (All.empty())
		{
			C.Text("A quiet day at the tables.", P.X + 40.0f, P.Y + 170.0f, Ts(30.0f, 900, Ink));
			const float Py = Paragraph(C, "No results worth the ink from the card rooms or the sites since yesterday. The crossword's half done; somebody wrote CHECK in seven across.", P.X + 40.0f,
				P.Y + 210.0f, P.W - 80.0f, 17.0f, 500, Ink, 24.0f, 4);
			const float Fy = std::max(Py + 30.0f, P.Y + 330.0f);
			if (Fy + ForecastH <= Floor - 14.0f)
			{
				Forecast(C, P.X + 40.0f, Fy, P.W - 80.0f, Ink);
			}
			return;
		}
		// The lead story across the top, then two columns.
		const int First = std::clamp(PaperFirst, 0, static_cast<int>(All.size()) - 1);
		float Ly = P.Y + 150.0f;
		const Story& Lead = All[static_cast<size_t>(First)];
		if (!Lead.Tag.empty())
		{
			TrackedText(C, Upper(Lead.Tag), P.X + 40.0f, Ly - 8.0f, 11.0f, 800, Hex(0x8c1d22), 2.2f);
		}
		Ly = Paragraph(C, Lead.Title, P.X + 40.0f, Ly + 26.0f, P.W - 80.0f, 30.0f, 900, Ink, 34.0f, 2) + 30.0f;
		Ly = Paragraph(C, Lead.Body, P.X + 40.0f, Ly, P.W - 80.0f, 17.0f, 500, Ink, 24.0f, 3) + 20.0f;
		C.FillRect({P.X + 40.0f, Ly, P.W - 80.0f, 1.0f}, Paint(Fade(Ink, 0.6f)));
		const float ColW = (P.W - 80.0f - 30.0f) * 0.5f;
		float Cy[2] = {Ly + 30.0f, Ly + 30.0f};
		int Col = 0;
		for (size_t K = static_cast<size_t>(First) + 1; K < All.size(); ++K)
		{
			const float X = P.X + 40.0f + Fl(Col) * (ColW + 30.0f);
			if (Cy[Col] > Bottom)
			{
				Col = 1 - Col;
				if (Cy[Col] > Bottom)
				{
					break;
				}
				continue;
			}
			float Yy = Paragraph(C, All[K].Title, X, Cy[Col], ColW, 19.0f, 900, Ink, 23.0f, 2) + 24.0f;
			Yy = Paragraph(C, All[K].Body, X, Yy, ColW, 14.0f, 500, Fade(Ink, 0.85f), 19.0f, 3) + 30.0f;
			Cy[Col] = Yy;
			Col = 1 - Col;
		}
		const float Used = std::max(Cy[0], Cy[1]);
		if (Used + ForecastH <= Floor - 14.0f)
		{
			Forecast(C, P.X + 40.0f, Used, P.W - 80.0f, Ink);
		}
		if (First > 0 || All.size() > 1)
		{
			C.Text(std::to_string(First + 1) + " of " + std::to_string(All.size()), P.X + P.W - 30.0f, P.Y + P.H - 20.0f, Ts(13.0f, 700, Fade(Ink, 0.6f), Align::Right));
		}
		(void)M;
		(void)D;
	}

	/** One six-hour spell of the paper's forecast. */
	struct Spell
	{
		std::string When;   // "Tonight", "Tomorrow morning"
		std::string Sky;    // "Rain", "Cloudy"
		std::string Detail; // "Rain chance 40%", "Staying dry"
		bool Thunder = false;
	};
	static constexpr float ForecastH = 150.0f;

	/**
	 * The next day in four six-hour spells, read from the same model the street's sky and puddles are (weather::SlotSky
	 * on the career's seed): what most of each spell is, how much of it is wet, and any thunder. Read once per half-hour
	 * slot; the weather is a pure function of the clock, so the paper can't promise what the street won't do.
	 */
	const std::array<Spell, 4>& Spells() const
	{
		const double World = S.WorldMinutes();
		const int64_t Slot = static_cast<int64_t>(std::floor(World / weather::SlotMinutes));
		if (Slot == SpellSlot)
		{
			return SpellCache;
		}
		SpellSlot = Slot;
		const uint64_t Seed = S.WorldSeed();
		constexpr int PerSpell = static_cast<int>(360.0 / weather::SlotMinutes);
		for (int K = 0; K < 4; ++K)
		{
			std::array<int, 5> Count{};
			int Worst = 0;
			for (int I = 0; I < PerSpell; ++I)
			{
				const int Sk = std::clamp(static_cast<int>(weather::SlotSky(Slot + K * PerSpell + I, Seed)), 0, 4);
				++Count[static_cast<size_t>(Sk)];
				Worst = std::max(Worst, Sk);
			}
			int Most = 0;
			for (int I = 1; I < 5; ++I)
			{
				Most = Count[static_cast<size_t>(I)] > Count[static_cast<size_t>(Most)] ? I : Most;
			}
			const int Wet = Count[2] + Count[3] + Count[4];
			// Named by the spell's middle: four six-hour middles always land in four different parts of the day.
			const double Mid = static_cast<double>(Slot + K * PerSpell) * weather::SlotMinutes + 180.0;
			const double InDay = Mid - std::floor(Mid / 1440.0) * 1440.0;
			const bool Today = net::DayOf(Mid) == net::DayOf(World);
			Spell& Sp = SpellCache[static_cast<size_t>(K)];
			if (InDay < 360.0)
			{
				Sp.When = "Overnight";
			}
			else if (InDay < 720.0)
			{
				Sp.When = Today ? "This morning" : "Tomorrow morning";
			}
			else if (InDay < 1080.0)
			{
				Sp.When = Today ? "This afternoon" : "Tomorrow afternoon";
			}
			else
			{
				Sp.When = Today ? "Tonight" : "Tomorrow night";
			}
			const bool Day = InDay >= 360.0 && InDay < 1080.0;
			static const char* const Words[5] = {"Clear", "Cloudy", "Drizzle", "Rain", "Storms"};
			Sp.Sky = Most == 0 && Day ? std::string("Sunny") : std::string(Words[Most]);
			const int Pct = static_cast<int>(std::lround(10.0 * Wet / PerSpell)) * 10;
			Sp.Detail = Wet == 0 ? std::string("Staying dry") : Wet == PerSpell ? std::string("Wet throughout") : "Rain chance " + std::to_string(std::clamp(Pct, 10, 90)) + "%";
			Sp.Thunder = Worst == 4;
		}
		return SpellCache;
	}

	/** The weather box: a rule, THE WEATHER, and four spells across. */
	void Forecast(Canvas& C, float X, float Y, float W, const Color& Ink)
	{
		const std::array<Spell, 4>& All = Spells();
		C.FillRect({X, Y, W, 2.0f}, Paint(Ink));
		TrackedText(C, "THE WEATHER", X, Y + 28.0f, 12.0f, 800, Hex(0x8c1d22), 2.4f);
		const float Gap = 24.0f;
		const float ColW = (W - Gap * 3.0f) / 4.0f;
		for (size_t K = 0; K < All.size(); ++K)
		{
			const Spell& Sp = All[K];
			const float Cx = X + static_cast<float>(K) * (ColW + Gap);
			if (K > 0)
			{
				C.FillRect({Cx - Gap * 0.5f, Y + 48.0f, 1.0f, ForecastH - 62.0f}, Paint(Fade(Ink, 0.35f)));
			}
			C.Text(Upper(Sp.When), Cx, Y + 62.0f, Ts(11.5f, 800, Fade(Ink, 0.75f), Align::Left, Baseline::Alphabetic, false, ColW));
			C.Text(Sp.Sky, Cx, Y + 94.0f, Ts(26.0f, 900, Ink, Align::Left, Baseline::Alphabetic, false, ColW));
			C.Text(Sp.Detail, Cx, Y + 118.0f, Ts(13.0f, 500, Fade(Ink, 0.85f), Align::Left, Baseline::Alphabetic, false, ColW));
			if (Sp.Thunder)
			{
				C.Text("Thunder possible", Cx, Y + 138.0f, Ts(13.0f, 700, Hex(0x8c1d22), Align::Left, Baseline::Alphabetic, false, ColW));
			}
		}
	}

	/** The paper's listings along the foot of the page, from the block's own books: who's open and the next 14 out. */
	void Listings(Canvas& C, const Rect& P, const Color& Ink)
	{
		const double World = S.WorldMinutes();
		const float Top = P.Y + P.H - 196.0f;
		C.FillRect({P.X + 40.0f, Top, P.W - 80.0f, 2.0f}, Paint(Ink));
		TrackedText(C, "AROUND THE BLOCK", P.X + 40.0f, Top + 28.0f, 12.0f, 800, Hex(0x8c1d22), 2.4f);
		std::vector<std::pair<std::string, std::string>> Rows;
		for (const char* Id : {"barber", "thrift", "pawn", "gym", "diner"})
		{
			if (const shop::Venue* V = shop::FindVenue(Id))
			{
				const std::string Open = shop::HoursLine(*V, World);
				if (!V->Name.empty() && !Open.empty())
				{
					Rows.emplace_back(V->Name, Open);
				}
			}
		}
		const std::vector<double> Bus = town::BusTimes("market-east", World, 1);
		if (!Bus.empty())
		{
			Rows.emplace_back("The 14 to the Embercrest", "Next from Market & Fifth at " + net::TimeLabel(Bus.front()));
		}
		const int PerCol = 2;
		const int Across = std::max(1, (static_cast<int>(Rows.size()) + PerCol - 1) / PerCol);
		const float ColW = (P.W - 80.0f - 24.0f * Fl(Across - 1)) / Fl(Across);
		for (size_t I = 0; I < Rows.size(); ++I)
		{
			const int Col = static_cast<int>(I) / PerCol;
			const int Row = static_cast<int>(I) % PerCol;
			const float X = P.X + 40.0f + Fl(Col) * (ColW + 24.0f);
			const float Y = Top + 62.0f + Fl(Row) * 58.0f;
			C.Text(Rows[I].first, X, Y, Ts(15.0f, 800, Ink, Align::Left, Baseline::Alphabetic, false, ColW));
			C.Text(Rows[I].second, X, Y + 21.0f, Ts(13.0f, 500, Fade(Ink, 0.8f), Align::Left, Baseline::Alphabetic, false, ColW));
		}
	}

	static float Fl(int V) { return static_cast<float>(V); }

	void Sit(Canvas& C, const Rect& R, double Now)
	{
		const double World = S.WorldMinutes();
		const double Until = World + 60.0 * Hours;
		C.Text("Sit a while.", R.X, R.Y + 40.0f, Ts(34.0f, 900, MenuInk));
		Paragraph(C, "Nobody rushes you at Ruby's. The coffee keeps coming, the window keeps the street, and the hours go by without a bed in them.", R.X, R.Y + 76.0f,
			std::min(R.W, 760.0f), 16.0f, 500, MenuMuted, 22.0f, 3);
		// The hour picker: eight cups, the ones you'll sit through filled.
		const float Py = R.Y + 170.0f;
		const float Cw = std::min(92.0f, (R.W - 7.0f * 14.0f) / 8.0f);
		for (int H = 1; H <= 8; ++H)
		{
			const Rect Cup{R.X + Fl(H - 1) * (Cw + 14.0f), Py, Cw, 96.0f};
			const bool On = H <= Hours;
			const bool Over = Hover(Cup);
			C.FillRoundRect(Cup, 10.0f, Paint(On ? Fade(Accent("diner"), H == Hours ? 0.9f : 0.45f) : Fade(MenuInk, Over ? 0.12f : 0.06f)));
			C.Text(std::to_string(H), Cup.X + Cup.W * 0.5f, Cup.Y + 50.0f, Ts(30.0f, 900, On ? Hex(0x1a0508) : MenuInk, Align::Center));
			C.Text(H == 1 ? "HOUR" : "HOURS", Cup.X + Cup.W * 0.5f, Cup.Y + 76.0f, Ts(11.0f, 800, On ? Hex(0x1a0508) : MenuMuted, Align::Center));
			if (Released(Cup))
			{
				Hours = H;
			}
		}
		C.Text("Until " + net::TimeLabel(Until) + (net::DayOf(Until) != net::DayOf(World) ? " tomorrow" : ""), R.X, Py + 140.0f, Ts(22.0f, 800, MenuInk));
		// What the hours do: hunger and thirst climb as they would awake; the coffee slows the energy's fall.
		const bool Coffee_ = S.Life.CoffeeUntil > World;
		const body::Effects Fx = body::EffectsOf(S.Life.Fit, S.Owns("gym"));
		const double Plain = 4.0 * Fx.DrainScale; // the awake drain, fitness included
		const double Drain = Plain * (Coffee_ ? 0.75 : 1.0);
		NeedsChange Change;
		Change.Hunger = -life::HungerPerHour * Hours;
		Change.Thirst = -life::ThirstPerHour * Hours;
		Change.Energy = -Drain * Hours;
		const float Vy = Py + 196.0f;
		TrackedText(C, "AFTER " + std::to_string(Hours) + (Hours == 1 ? " HOUR" : " HOURS"), R.X, Vy - 18.0f, 11.0f, 800, MenuMuted, 2.6f);
		const float VitW = std::clamp(R.W * 0.46f, 360.0f, 520.0f);
		DrawVitals(C, S.Life, R.X, Vy, VitW, Now, Change);
		// The rates as the drain runs them (the coffee's quarter off, fitness's share), not the round numbers.
		auto Rate = [](double V) {
			const double R1 = std::round(V * 10.0) / 10.0;
			return std::fabs(R1 - std::round(R1)) < 0.05 ? std::to_string(static_cast<int>(std::lround(R1))) : Fixed(R1, 1);
		};
		const std::string Note_ = !Order_.empty() ? "Your order's still on the check. Pay it on the menu page, then sit as long as you like."
			: Coffee_							? "With the coffee going, the energy falls about " + Rate(Drain) + " an hour, not " + Rate(Plain) + "."
												: "Order a coffee first and the energy falls about " + Rate(Plain * 0.75) + " an hour instead of " + Rate(Plain) + ".";
		Paragraph(C, Note_, R.X + VitW + 48.0f, Vy + 14.0f, R.X + R.W - (R.X + VitW + 48.0f), 14.0f, 500, Order_.empty() ? MenuMuted : MenuWarn, 19.0f, 3);
	}

	std::vector<std::string> Order_;
	std::vector<std::string> Paid;
	bool PaidByLou = false; // the check just paid was Big Lou's (it stays honest once his night is marked)
	Chips PaidTip = 0;      // and the tip that was the hero's on it
	int PaidPct = 0;        // at that percent
	std::string Seat;
	int Hours = 2;
	int PaperFirst = 0;
	int Cols = 2;
	mutable int64_t SpellSlot = INT64_MIN;
	mutable std::array<Spell, 4> SpellCache;
};
} // namespace

std::unique_ptr<ShopScreen> MakeDiner(Session& S)
{
	return std::make_unique<DinerPage>(S);
}
} // namespace shopframe
} // namespace ui
} // namespace ss
