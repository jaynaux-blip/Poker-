// IronHouse Gym (the realism update's spec, §5.3 and §7.4): the desk (the membership or a day pass, Rosa's sessions,
// the shake, and a fitness meter with what it does in words, from body::EffectsOf and TableOf), the floor (each
// machine's workout with its minutes, what it costs the body, whether it counts, and Start), and the cooler.
#include "ShopFrame.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Body.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Gear.h"
#include "ShortStack/Game/Network.h"

#include <algorithm>
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

bool IsWorkout(const shop::Offer& O)
{
	return O.Type == shop::OfferKind::Workout || O.Type == shop::OfferKind::Training;
}

bool IsDesk(const shop::Offer& O)
{
	return O.Type == shop::OfferKind::Membership || O.Type == shop::OfferKind::DayPass || O.Type == shop::OfferKind::Shake;
}

bool IsCooler(const shop::Offer& O)
{
	return O.Type == shop::OfferKind::Drink || O.Type == shop::OfferKind::Meal;
}

std::string Pct(double V)
{
	return std::to_string(static_cast<int>(std::lround(V * 100.0))) + "%";
}

/** A reason as a button's label: in capitals, without its full stop. */
std::string Label(const std::string& Why)
{
	std::string Out = Upper(Why);
	while (!Out.empty() && (Out.back() == '.' || Out.back() == ' '))
	{
		Out.pop_back();
	}
	return Out;
}

/** What NeedPills draws for these needs, in width (so a badge can sit beside them, or above when it can't). */
float PillsWidth(Canvas& C, double Hunger, double Thirst, double Energy)
{
	float W = 0.0f;
	for (const double V : {Hunger, Thirst, Energy})
	{
		if (std::fabs(V) >= 0.5)
		{
			W += C.Measure((V > 0.0 ? "+" : "\xE2\x88\x92") + std::to_string(static_cast<int>(std::lround(std::fabs(V)))), 13.0f, 800) + 40.0f;
		}
	}
	return W;
}

constexpr const char* DayPassId = "gym/daypass";

class GymPage final : public Page
{
public:
	explicit GymPage(Session& InSession) : Page(InSession, "gym") {}

protected:
	std::vector<std::string> TabNames() const override
	{
		std::vector<std::string> Out = {"DESK"};
		bool Work = false;
		bool Cool = false;
		for (const shop::Offer& O : shop::Offers("gym"))
		{
			Work = Work || IsWorkout(O);
			Cool = Cool || IsCooler(O);
		}
		if (Work)
		{
			Out.push_back("WORKOUTS");
		}
		if (Cool)
		{
			Out.push_back("COOLER");
		}
		return Out;
	}

	void Opened(double Now, const std::string& PageId) override
	{
		Basket.clear();
		Paid.clear();
		const std::vector<std::string> Names = TabNames();
		auto TabOf = [&](const char* Name) {
			for (size_t I = 0; I < Names.size(); ++I)
			{
				if (Names[I] == Name)
				{
					return static_cast<int>(I);
				}
			}
			return 0;
		};
		const bool Machine = PageId.compare(0, 13, "gym-treadmill") == 0 || PageId == "gym-bag" || PageId == "gym-weights" || PageId == "WORKOUTS";
		if (Machine)
		{
			Tab = TabOf("WORKOUTS");
			// The machine's own workout in focus: the treadmills run, the bag's the bag, the rack is the weights.
			const std::string Key = PageId.compare(0, 13, "gym-treadmill") == 0 ? "treadmill" : PageId == "gym-bag" ? "bag" : PageId == "gym-weights" ? "weight" : "";
			const std::vector<const shop::Offer*> On = OnTab();
			for (size_t I = 0; I < On.size() && !Key.empty(); ++I)
			{
				if (On[I]->Id.find(Key) != std::string::npos)
				{
					Sel = static_cast<int>(I);
					break;
				}
			}
			Camera("gym-floor");
		}
		else if (PageId == "gym-cooler" || PageId == "COOLER")
		{
			Tab = TabOf("COOLER");
		}
		(void)Now;
	}

	std::string KeeperAway() const override
	{
		// Between Rosa's shifts: the tent card on the desk says when she's back (§5.3).
		const shop::Venue* V = shop::FindVenue("gym");
		if (!V)
		{
			return "THE DESK";
		}
		const double World = S.WorldMinutes();
		double Back = World;
		for (int K = 1; K <= 48 * 4; ++K)
		{
			const double At = World + 15.0 * K;
			if (shop::IsStaffed(*V, At))
			{
				Back = At;
				break;
			}
		}
		// "4 PM", as the tent card says it (BackSign).
		const int Hour = static_cast<int>(std::floor((Back - std::floor(Back / 1440.0) * 1440.0) / 60.0));
		return "THE DESK \xC2\xB7 BACK AT " + std::to_string(Hour % 12 == 0 ? 12 : Hour % 12) + (Hour < 12 ? " AM" : " PM");
	}

	std::string Greeting() const override { return S.Owns("gym") ? "greet" : "browse"; }

	bool PageKey(const std::string& Name, double Now) override
	{
		const std::vector<const shop::Offer*> On = OnTab();
		const bool Floor = OnFloor();
		if (GridKey(Name, static_cast<int>(On.size()), Cols, Now))
		{
			return true;
		}
		if ((Name == "Enter" || Name == "Space") && Sel < static_cast<int>(On.size()))
		{
			if (Floor)
			{
				DoMain(Now);
			}
			else
			{
				Pick(*On[static_cast<size_t>(Sel)], Now);
			}
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
		const auto It = std::find(Basket.begin(), Basket.end(), Id);
		if (It != Basket.end())
		{
			Basket.erase(It);
			Touch(Id, Now);
		}
	}

	std::vector<Hint> Hints() const override
	{
		if (OnFloor())
		{
			return {{"UP", "DPAD", "CHOOSE"}, {"TAB", "Y", "START"}};
		}
		return {{"UP", "DPAD", "CHOOSE"}, {"ENTER", "A", "ADD"}, {"TAB", "Y", "PAY"}};
	}

	void Body(Canvas& C, const Rect& R, double Now) override
	{
		const std::vector<const shop::Offer*> On = OnTab();
		const bool Floor = OnFloor();
		if (TabNames()[static_cast<size_t>(std::min<int>(Tab, static_cast<int>(TabNames().size()) - 1))] == "DESK")
		{
			// The desk: what it sells on the left, the fitness card on the right.
			const float Lw = std::max(360.0f, R.W * 0.5f);
			Cols = Lw >= 600.0f ? 2 : 1;
			if (On.empty())
			{
				C.Text("Rosa's prices aren't up on the board yet.", R.X, R.Y + 40.0f, Ts(18.0f, 500, MenuDim));
			}
			Grid(
				C, {R.X, R.Y, Lw, R.H}, static_cast<int>(On.size()), Cols, 150.0f,
				[&](int I, const Rect& Cell, bool Focus, bool Over) { Card(C, *On[static_cast<size_t>(I)], Cell, Focus, Over, Now); },
				[&](int I) { Pick(*On[static_cast<size_t>(I)], Now); }, Now);
			FitnessCard(C, {R.X + Lw + 32.0f, R.Y, R.W - Lw - 32.0f, R.H}, Now);
			return;
		}
		Cols = R.W >= 900.0f ? 3 : 2;
		const float GridH = Floor ? R.H - 190.0f : R.H - 150.0f;
		Grid(
			C, {R.X, R.Y, R.W, GridH}, static_cast<int>(On.size()), Cols, 190.0f,
			[&](int I, const Rect& Cell, bool Focus, bool Over) { Card(C, *On[static_cast<size_t>(I)], Cell, Focus, Over, Now); },
			[&](int I) {
				if (!Floor)
				{
					Pick(*On[static_cast<size_t>(I)], Now);
				}
			},
			Now);
		// What it would do: the focused workout, or the cooler's basket, on the vitals.
		NeedsChange Change;
		std::string Head = "WHAT IT TAKES";
		if (Floor && Sel < static_cast<int>(On.size()))
		{
			const shop::Offer& O = *On[static_cast<size_t>(Sel)];
			Change = {O.Hunger, O.Thirst, O.Energy};
		}
		else
		{
			Head = "WHAT IT GIVES";
			for (const std::string& Id : Basket)
			{
				if (const shop::Offer* O = shop::FindOffer(Id))
				{
					Change.Hunger += O->Hunger;
					Change.Thirst += O->Thirst;
					Change.Energy += O->Energy > 0.0 ? O->Energy * S.Life.Perks.MealBoost : O->Energy;
				}
			}
		}
		const float Vy = R.Y + R.H - (Floor ? 130.0f : 96.0f);
		TrackedText(C, Head, R.X, Vy - 18.0f, 11.0f, 800, MenuMuted, 2.6f);
		const float VitW = std::clamp(R.W * 0.48f, 360.0f, 520.0f);
		DrawVitals(C, S.Life, R.X, Vy, VitW, Now, Change);
		if (Floor && Sel < static_cast<int>(On.size()))
		{
			const shop::Offer& O = *On[static_cast<size_t>(Sel)];
			const float Tx = R.X + VitW + 48.0f;
			const float Tw = R.X + R.W - Tx;
			const std::string Count = CountLine(O);
			Paragraph(C, Count, Tx, Vy + 10.0f, Tw, 15.0f, 500, MenuInk, 21.0f, 3);
			if (!O.Blurb.empty())
			{
				Paragraph(C, O.Blurb, Tx, Vy + 80.0f, Tw, 13.0f, 400, MenuMuted, 18.0f, 2);
			}
		}
	}

	Receipt Bill(double /*Now*/) override
	{
		const double World = S.WorldMinutes();
		Receipt R;
		R.Paper = PaperKind::Thermal;
		R.Title = "IRONHOUSE GYM";
		R.Head = {"246 MARKET ST. \xC2\xB7 24/7 FOR MEMBERS", Upper(Stamped(World))};
		if (OnFloor())
		{
			const std::vector<const shop::Offer*> On = OnTab();
			R.Empty = "Pick a machine.";
			if (Sel < static_cast<int>(On.size()))
			{
				const shop::Offer& O = *On[static_cast<size_t>(Sel)];
				const std::vector<std::string> Ids = FloorOrder(O);
				if (Ids.size() > 1)
				{
					R.Lines.push_back({DayPassId, "DAY PASS \xC2\xB7 TODAY", "", Money(PassPrice())});
				}
				Line L;
				L.Id = O.Id;
				L.Left = Upper(O.Name);
				L.Mid = MinutesText(O.Minutes);
				L.Right = O.MembersFree && (Access() || Ids.size() > 1) ? std::string("INCLUDED") : Money(O.PriceCents);
				R.Lines.push_back(L);
				const std::string Why = FloorBlocked(O);
				if (Why.empty())
				{
					R.Total = {"total", "TOTAL", "", Money(Due1(O))};
				}
				else
				{
					Line No{"", Label(Why), "", ""};
					No.Red = true;
					R.Sums.push_back(No);
				}
			}
			R.Foot = {S.Owns("gym") ? "MEMBER \xC2\xB7 THE FOB WORKS ANY HOUR" : Access() ? "DAY PASS \xC2\xB7 TODAY, WHILE THE DESK IS STAFFED" : "NOT A MEMBER \xC2\xB7 THE DESK SELLS DAY PASSES",
				"CARD BALANCE " + Money(S.BankrollCents)};
			return R;
		}
		const std::vector<std::string>& List = Basket.empty() ? Paid : Basket;
		Chips Sub = 0;
		Chips TaxedPart = 0;
		for (const std::string& Id : List)
		{
			if (const shop::Offer* O = shop::FindOffer(Id))
			{
				Line L;
				L.Id = Id;
				L.Left = Upper(O->Name);
				L.Right = Money(O->PriceCents);
				L.Removable = !Basket.empty();
				R.Lines.push_back(L);
				Sub += O->PriceCents;
				TaxedPart += O->Taxed ? O->PriceCents : 0;
			}
		}
		R.Empty = "Towel's free. The rest is on the board.";
		if (!List.empty())
		{
			R.Sums.push_back({"subtotal", "SUBTOTAL", "", Money(Sub)});
			R.Sums.push_back({"tax", "TAX 7.25%", "", Money(store::Tax(TaxedPart))});
			R.Total = {"total", "TOTAL", "", Money(Basket.empty() ? Sub + store::Tax(TaxedPart) : Total())};
		}
		R.Foot = {"MEMBERSHIPS RENEW EVERY 30 DAYS", "CARD BALANCE " + Money(S.BankrollCents)};
		return R;
	}

	Button Main(double /*Now*/) override
	{
		Button B;
		if (OnFloor())
		{
			const std::vector<const shop::Offer*> On = OnTab();
			if (Sel >= static_cast<int>(On.size()))
			{
				B.Label = "START";
				return B;
			}
			const shop::Offer& O = *On[static_cast<size_t>(Sel)];
			const std::string Why = FloorBlocked(O);
			if (!Why.empty())
			{
				B.Label = Label(Why);
				return B;
			}
			B.Enabled = true;
			const Chips Due = Due1(O);
			B.Warn = Due > S.BankrollCents;
			const bool WithPass = FloorOrder(O).size() > 1;
			B.Label = B.Warn ? "CARD WON'T COVER IT" : (WithPass ? "DAY PASS + START" : "START \xC2\xB7 " + MinutesText(O.Minutes)) + (Due > 0 ? " \xC2\xB7 " + Money(Due) : "");
			return B;
		}
		if (Basket.empty())
		{
			B.Label = "PAY";
			return B;
		}
		B.Enabled = true;
		const Chips Due = Total();
		B.Warn = Due > S.BankrollCents;
		B.Label = B.Warn ? "CARD WON'T COVER IT" : "PAY " + Money(Due);
		return B;
	}

	void DoMain(double Now) override
	{
		if (OnFloor())
		{
			const std::vector<const shop::Offer*> On = OnTab();
			if (Sel >= static_cast<int>(On.size()))
			{
				return;
			}
			const shop::Offer& O = *On[static_cast<size_t>(Sel)];
			const Chips Due = Due1(O);
			const std::string Why = S.ShopService(Order("gym", FloorOrder(O)));
			if (!Why.empty())
			{
				if (Due > S.BankrollCents && Due > 0)
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
			Say(O.Type == shop::OfferKind::Training ? "train" : "pay", Now);
			Started(Now);
			return;
		}
		if (Basket.empty())
		{
			Say("browse", Now);
			return;
		}
		const Chips Due = Total();
		const std::vector<std::string> Bought = Basket;
		const std::string Why = S.ShopPay(Order("gym", Basket));
		if (!Why.empty())
		{
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
		Paid = Bought;
		Basket.clear();
		Stamp(StampKind::Paid, Now);
		Say("pay", Now);
		Started(Now);
	}

private:
	bool OnFloor() const
	{
		const std::vector<std::string> Names = TabNames();
		return Tab < static_cast<int>(Names.size()) && Names[static_cast<size_t>(Tab)] == "WORKOUTS";
	}

	std::vector<const shop::Offer*> OnTab() const
	{
		const std::vector<std::string> Names = TabNames();
		const std::string& T = Names[static_cast<size_t>(std::clamp(Tab, 0, static_cast<int>(Names.size()) - 1))];
		std::vector<const shop::Offer*> Out;
		for (const shop::Offer& O : shop::Offers("gym"))
		{
			if ((T == "DESK" && IsDesk(O)) || (T == "WORKOUTS" && IsWorkout(O)) || (T == "COOLER" && IsCooler(O)))
			{
				Out.push_back(&O);
			}
		}
		return Out;
	}

	/** One workout's charge (with the day pass when it goes on the order): the Session's sum, or the prices'. */
	Chips Due1(const shop::Offer& O)
	{
		const std::vector<std::string> Ids = FloorOrder(O);
		const Chips Core = S.ShopTotal(Order("gym", Ids));
		if (Core > 0)
		{
			return Core;
		}
		const Chips Own = O.MembersFree && (Access() || Ids.size() > 1) ? 0 : O.PriceCents + (O.Taxed ? store::Tax(O.PriceCents) : 0);
		return Own + (Ids.size() > 1 ? PassPrice() : 0);
	}

	Chips Total()
	{
		const Chips Core = S.ShopTotal(Order("gym", Basket));
		if (Core > 0)
		{
			return Core;
		}
		Chips Sub = 0;
		Chips TaxedPart = 0;
		for (const std::string& Id : Basket)
		{
			if (const shop::Offer* O = shop::FindOffer(Id))
			{
				Sub += O->PriceCents;
				TaxedPart += O->Taxed ? O->PriceCents : 0;
			}
		}
		return Sub + store::Tax(TaxedPart);
	}

	/** Whether a workout counts toward Fitness now (one counted workout per 16 hours, §5.3), in words. */
	std::string CountLine(const shop::Offer& O) const
	{
		const double World = S.WorldMinutes();
		const double Gain = body::Gain(S.Life.Fit, O.FitnessBase, S.Life.Energy, World);
		if (Gain > 0.0)
		{
			std::string Out = "+" + Fixed(Gain, 1) + " Fitness when it's done.";
			if (S.Life.Energy < 25.0)
			{
				Out += " Half, running on empty.";
			}
			return Out;
		}
		if (O.FitnessBase <= 0.0)
		{
			return "For the head, not the legs.";
		}
		const double Next = S.Life.Fit.LastCounted + 16.0 * 60.0;
		if (Next > World)
		{
			return "Won't count toward Fitness till " + net::TimeLabel(Next) + ": one counted workout every 16 hours. It still costs the energy.";
		}
		return "Fitness is as high as it goes.";
	}

	void Pick(const shop::Offer& O, double Now)
	{
		if (IsWorkout(O))
		{
			return;
		}
		const std::string Why = S.ShopBlocked(O.Id);
		const auto It = std::find(Basket.begin(), Basket.end(), O.Id);
		if (It != Basket.end())
		{
			Basket.erase(It);
			Touch(O.Id, Now);
			return;
		}
		if (!Why.empty())
		{
			SayText(Why, Now, true);
			return;
		}
		// The membership covers today: never both on one receipt (each passes on its own while the hero is neither).
		const auto Pass = std::find(Basket.begin(), Basket.end(), std::string(DayPassId));
		if (O.Type == shop::OfferKind::DayPass && std::find(Basket.begin(), Basket.end(), std::string("gym/membership")) != Basket.end())
		{
			SayText("Membership covers today. Skip the pass.", Now, true);
			return;
		}
		if (O.Type == shop::OfferKind::Membership && Pass != Basket.end())
		{
			Basket.erase(Pass);
			Touch(DayPassId, Now);
		}
		if (Basket.empty())
		{
			Paid.clear();
			StampWas = StampKind::None;
		}
		Basket.push_back(O.Id);
		Touch(O.Id, Now);
		Say("pick", Now);
	}

	void Card(Canvas& C, const shop::Offer& O, const Rect& R, bool Focus, bool Over, double Now)
	{
		const bool Work = IsWorkout(O);
		const std::string Why = Work ? FloorBlocked(O) : S.ShopBlocked(O.Id);
		const bool Picked = std::find(Basket.begin(), Basket.end(), O.Id) != Basket.end();
		const bool Member = O.Type == shop::OfferKind::Membership && S.Owns("gym");
		const bool Dim = !Why.empty() && !Picked;
		CardBack(C, R, Focus, Over, Dim, Now);
		const Color Acc = Accent("gym");
		const float X = R.X + 18.0f;
		// A price in the price's type; a word (INCLUDED, WITH A PASS) in the badges' small capitals.
		const std::string Pr = PriceLabel(O);
		const bool Word = Pr.empty() || Pr[0] != '$';
		const float Pw = Word ? TrackedWidth(C, Pr, 12.0f, 800, 2.2f) : C.Measure(Pr, 20.0f, 900);
		C.Text(O.Name, X, R.Y + 36.0f, Ts(20.0f, 900, Dim ? MenuMuted : MenuInk, Align::Left, Baseline::Alphabetic, false, R.W - 54.0f - Pw));
		if (Word)
		{
			TrackedText(C, Pr, R.X + R.W - 18.0f - Pw, R.Y + 33.0f, 12.0f, 800, Dim ? MenuMuted : Acc, 2.2f);
		}
		else
		{
			C.Text(Pr, R.X + R.W - 18.0f, R.Y + 36.0f, Ts(20.0f, 900, Dim ? MenuMuted : Acc, Align::Right));
		}
		// The badge: what it is to the hero now (a member, on the receipt, why not, what it would count for).
		std::string Tag;
		Color TagCol = Acc;
		bool TagSolid = false;
		if (Member)
		{
			const auto Renews = S.Gear.find("gym");
			Tag = Renews != S.Gear.end() && Renews->second > 0.0 ? "MEMBER \xC2\xB7 RENEWS " + Upper(net::DateLabel(net::DayOf(Renews->second))) : "MEMBER";
			TagCol = Mint;
			TagSolid = true;
		}
		else if (Picked)
		{
			Tag = "ON THE RECEIPT";
			TagCol = Mint;
			TagSolid = true;
		}
		else if (Dim)
		{
			Tag = BadgeText(C, Why, R.W - 36.0f, "NOT NOW");
			TagCol = MenuWarn;
		}
		else if (Work)
		{
			const double Gain = body::Gain(S.Life.Fit, O.FitnessBase, S.Life.Energy, S.WorldMinutes());
			if (FloorOrder(O).size() > 1)
			{
				Tag = "+ DAY PASS " + Price(PassPrice());
				TagCol = MenuGold;
			}
			else if (Gain > 0.0)
			{
				Tag = "+" + Fixed(Gain, 1) + " FITNESS";
			}
			else if (O.FitnessBase > 0.0)
			{
				Tag = "WON'T COUNT";
				TagCol = MenuMuted;
			}
		}
		// The needs it moves along the bottom, the badge beside them; a badge that won't fit beside them sits on the row
		// above (a short card's blurb gives up its second line for it).
		const double Energy = O.Type == shop::OfferKind::Meal || O.Type == shop::OfferKind::Drink || O.Type == shop::OfferKind::Shake ? (O.Energy > 0.0 ? O.Energy * S.Life.Perks.MealBoost : O.Energy) : O.Energy;
		const float Inner = R.W - 36.0f;
		const float Tw = Tag.empty() ? 0.0f : BadgeWidth(C, Tag);
		const float PillW = PillsWidth(C, O.Hunger, O.Thirst, Energy);
		const bool Above = !Tag.empty() && PillW > 0.0f && PillW + 12.0f + Tw > Inner;
		const bool Short = R.H < 180.0f;
		float Y = R.Y + 58.0f;
		if (O.Minutes > 0)
		{
			C.Text(MinutesText(O.Minutes), X, Y, Ts(13.0f, 700, MenuMuted));
			Y += 6.0f;
		}
		if (!O.Blurb.empty())
		{
			Paragraph(C, O.Blurb, X, Y + 16.0f, Inner, 13.0f, 400, Fade(MenuInk, 0.6f), 18.0f, Above && Short ? 1 : 2);
		}
		NeedPills(C, O.Hunger, O.Thirst, Energy, X, R.Y + R.H - 40.0f, Above || Tag.empty() ? Inner : Inner - Tw - 12.0f);
		if (!Tag.empty())
		{
			Badge(C, Tag, R.X + R.W - 18.0f - Tw, Above ? R.Y + R.H - (Short ? 64.0f : 68.0f) : R.Y + R.H - 38.0f, TagCol, TagSolid);
		}
	}

	/** Whether the hero has the floor today: a member, or today's day pass. */
	bool Access() const
	{
		if (S.Owns("gym"))
		{
			return true;
		}
		const auto It = S.Life.Visits.find(DayPassId);
		return It != S.Life.Visits.end() && It->second.Count > 0 && net::DayOf(It->second.LastAt) == net::DayOf(S.WorldMinutes());
	}

	Chips PassPrice() const
	{
		const shop::Offer* P = shop::FindOffer(DayPassId);
		return P ? P->PriceCents : 0;
	}

	/** A workout's price as the board puts it: included with the fob or today's pass, its own price, or with a pass. */
	std::string PriceLabel(const shop::Offer& O) const
	{
		if (O.MembersFree && Access())
		{
			return "INCLUDED";
		}
		if (O.PriceCents > 0)
		{
			return Price(O.PriceCents);
		}
		return O.MembersFree ? "WITH A PASS" : "FREE";
	}

	/** The floor's order for a workout: today's day pass goes on it when the hero has none and the desk can sell one. */
	std::vector<std::string> FloorOrder(const shop::Offer& O) const
	{
		if (O.MembersFree && !Access() && S.ShopBlocked(DayPassId).empty())
		{
			return {DayPassId, O.Id};
		}
		return {O.Id};
	}

	/**
	 * Why a workout can't start now ("" when it can). With a pass going on the order the Session's pass rule no longer
	 * applies, so only the body's (you don't have it in you) is checked here; anything else, the Session says on START.
	 */
	std::string FloorBlocked(const shop::Offer& O) const
	{
		if (FloorOrder(O).size() > 1)
		{
			return S.Life.Energy < -O.Energy ? std::string("You don't have it in you. Eat or sleep first.") : std::string();
		}
		return S.ShopBlocked(O.Id);
	}

	void FitnessCard(Canvas& C, const Rect& R, double Now)
	{
		if (R.W < 260.0f)
		{
			return;
		}
		const double World = S.WorldMinutes();
		const body::Fitness& F = S.Life.Fit;
		const bool Member = S.Owns("gym");
		const body::Effects Fx = body::EffectsOf(F, Member);
		const body::TableEffects Tx = body::TableOf(F, World);
		C.FillRoundRect(R, 8.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, Rgba(24, 28, 38, 0.9f), Rgba(12, 14, 20, 0.9f)));
		const Color Acc = Accent("gym");
		// The meter: a ring, the level in the middle.
		const float Cx = R.X + 96.0f;
		const float Cy = R.Y + 100.0f;
		const float Lv = static_cast<float>(std::clamp(F.Level, 0.0, 100.0) / 100.0);
		C.StrokeArc(Cx, Cy, 58.0f, -1.5708f, 4.7124f, Fade(MenuInk, 0.1f), 12.0f);
		const float Grow = Ease((Now - OpenedAt - 0.2) / 0.8);
		if (Lv > 0.001f)
		{
			C.StrokeArc(Cx, Cy, 58.0f, -1.5708f, -1.5708f + 6.2832f * Lv * Grow, Acc, 12.0f, true);
		}
		C.Text(std::to_string(static_cast<int>(std::lround(F.Level))), Cx, Cy + 12.0f, Ts(40.0f, 900, MenuInk, Align::Center));
		TrackedText(C, "FITNESS", Cx, Cy + 34.0f, 10.0f, 800, MenuMuted, 2.2f, Align::Center);
		// Where it stands: membership, the last workout that counted, the fade.
		const float Tx0 = R.X + 184.0f;
		const float Tw = R.X + R.W - 20.0f - Tx0;
		TrackedText(C, Member ? "MEMBER" : "NO MEMBERSHIP", Tx0, R.Y + 50.0f, 11.0f, 800, Member ? Mint : MenuMuted, 2.4f);
		// A member migrated from an older save has a LastCounted (the fading counts from it) but no workout behind it: the
		// count says whether one has ever counted, as the barber's Cuts does.
		std::string Last = F.Workouts <= 0 || F.LastCounted < -1.0e8 ? std::string("No workout counted yet.")
																	   : "Last counted " + (net::DayOf(World) == net::DayOf(F.LastCounted) ? std::string("today") : std::string(net::WeekdayName(net::DayOf(F.LastCounted), true))) + ".";
		if (F.LastCounted > -1.0e8 && World - F.LastCounted > 3.0 * 1440.0)
		{
			Last += " Fading a point a day.";
		}
		else if (F.Level > 0.0)
		{
			Last += " It fades after three days without one.";
		}
		Paragraph(C, Last, Tx0, R.Y + 76.0f, Tw, 14.0f, 500, MenuInk, 19.0f, 3);
		// What it does, in words.
		std::vector<std::string> Lines;
		if (Fx.DrainScale < 0.995)
		{
			Lines.push_back("You tire " + Pct(1.0 - Fx.DrainScale) + " slower awake.");
		}
		if (Fx.RestBonus > 0.005)
		{
			Lines.push_back("Sleep gives back " + Pct(Fx.RestBonus) + " more.");
		}
		if (Fx.ExhaustedAt < 19.5)
		{
			Lines.push_back("Exhausted only below " + std::to_string(static_cast<int>(std::lround(Fx.ExhaustedAt))) + " energy, not 20.");
		}
		if (Fx.Calm > 0.005)
		{
			Lines.push_back("Tilt fades " + Pct(Fx.Calm) + " faster" + (Member ? ", the sauna included." : "."));
		}
		if (Tx.RestBpm < 65.5)
		{
			Lines.push_back("Resting heart at the table: " + std::to_string(static_cast<int>(std::lround(Tx.RestBpm))) + " bpm.");
		}
		if (Tx.FocusRefill > 1.0 / 14.0 + 1.0e-4)
		{
			Lines.push_back("Focus refills " + Pct(Tx.FocusRefill * 14.0 - 1.0) + " faster.");
		}
		if (Tx.SteadyBpm > 16.5)
		{
			Lines.push_back("Box Breathing: steadying takes " + std::to_string(static_cast<int>(std::lround(Tx.SteadyBpm))) + " bpm off, for less Focus.");
		}
		else if (F.Sessions > 0)
		{
			Lines.push_back("Box Breathing: " + std::to_string(std::min(F.Sessions, 3)) + " of 3 sessions with Rosa.");
		}
		if (F.ClearHeadUntil > World)
		{
			Lines.push_back("Clear head until " + net::TimeLabel(F.ClearHeadUntil) + ": bad beats sting " + Pct(1.0 - Tx.TiltGainScale) + " less.");
		}
		if (Lines.empty())
		{
			Lines.push_back("Nothing yet. Fitness builds one workout at a time: a steadier heart at the table, better sleep, more left in the tank.");
		}
		float Y = R.Y + 196.0f;
		TrackedText(C, "WHAT IT DOES", R.X + 22.0f, Y, 11.0f, 800, MenuMuted, 2.6f);
		Y += 8.0f;
		for (const std::string& L : Lines)
		{
			if (Y > R.Y + R.H - 30.0f)
			{
				break;
			}
			C.FillCircle(R.X + 28.0f, Y + 16.0f, 3.0f, Paint(Acc));
			Y = Paragraph(C, L, R.X + 42.0f, Y + 22.0f, R.W - 64.0f, 15.0f, 500, MenuInk, 20.0f, 2) + 8.0f;
		}
	}

	std::vector<std::string> Basket;
	std::vector<std::string> Paid;
	int Cols = 2;
};
} // namespace

std::unique_ptr<ShopScreen> MakeGym(Session& S)
{
	return std::make_unique<GymPage>(S);
}
} // namespace shopframe
} // namespace ui
} // namespace ss
