// Second Chance Thrift (the realism update's spec, §5.2 and §7.4): this week's racks (restocked Thursdays) by the tag
// signs over them, each piece with its condition and its tag (Saturday's yellow at half), the fitting booth's try-on
// (PreviewLook and the three-way mirror), the register with "Wear it out", and the donation bin with Bea's receipt
// "for your taxes".
#include "ShopFrame.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/UI/Portrait.h"

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

struct Rack
{
	const char* Tab;
	const char* Id;  // shopart::RackOf
	const char* Use; // the registry use that opens it
};
const Rack Racks[] = {
	{"TOPS", "tops", "thrift-rack-tops"},
	{"JACKETS", "outer", "thrift-rack-outer"},
	{"PANTS", "pants", "thrift-rack-pants"},
	{"SHOES", "shoes", "thrift-shoes"},
	{"HATS & GLASSES", "hats", "thrift-hats"},
};
constexpr int RackCount = 5;
constexpr int DonateTab = 5;

const char* ConditionWord(int Condition)
{
	return Condition <= 0 ? "Good" : Condition == 1 ? "Worn" : "Rough";
}

class ThriftPage final : public Page
{
public:
	explicit ThriftPage(Session& InSession) : Page(InSession, "thrift") {}

protected:
	std::vector<std::string> TabNames() const override { return {"TOPS", "JACKETS", "PANTS", "SHOES", "HATS & GLASSES", "DONATE"}; }

	void Opened(double Now, const std::string& PageId) override
	{
		// What's in hand rides along from the rack to the register (or another rack) in the same visit: the screen
		// closes when the hero walks off, the basket doesn't. Anything since bought or gone off the racks drops out.
		const double World = S.WorldMinutes();
		if (World < LeftAt || World - LeftAt > CarryMinutes)
		{
			Basket.clear();
		}
		else
		{
			const std::vector<std::string> Racked = shopart::ThriftRack(S, Caps.GarmentKinds);
			Basket.erase(std::remove_if(Basket.begin(), Basket.end(),
							 [&](const std::string& Id) { return S.Life.Closet.Owned.count(Id) > 0 || std::find(Racked.begin(), Racked.end(), Id) == Racked.end(); }),
				Basket.end());
		}
		Paid.clear();
		Bin.clear();
		Given.clear();
		Fitting.clear();
		AtRegister = false;
		WearItOut = true;
		for (int I = 0; I < RackCount; ++I)
		{
			Tab = PageId == Racks[I].Use || PageId == Racks[I].Tab ? I : Tab;
		}
		if (PageId == "thrift-donate" || PageId == "DONATE")
		{
			Tab = DonateTab;
		}
		if (Tab != DonateTab)
		{
			Camera("thrift-rack");
		}
		if (PageId == "thrift-booth")
		{
			// The booth: try on whatever's in hand (nothing yet: Bea points at the racks).
			SayText("Bring something from the racks first. The booth's on the left.", Now);
		}
		else if (PageId == "thrift-register")
		{
			// At the register: straight to paying for what's in hand, or Bea pointing back at the racks.
			AtRegister = !Basket.empty();
			SayText(Basket.empty() ? "Nothing in your arms, sweetheart. Racks are behind you." : "Let's ring you up, sweetheart.", Now);
		}
		else if (net::Weekday(net::DayOf(S.WorldMinutes())) == 5)
		{
			Say("saturday", Now);
		}
	}

	void TabChanged(double Now) override
	{
		if (Tab == DonateTab)
		{
			SayText("Bin's by the door. Anything you don't wear, somebody will.", Now);
		}
	}

	void Closing(double /*Now*/) override
	{
		Fitting.clear();
		AtRegister = false;
		LeftAt = S.WorldMinutes();
	}

	bool TabsHeld() const override { return !Fitting.empty() || AtRegister; }

	bool StepBack(double Now) override
	{
		if (!Fitting.empty())
		{
			LeaveBooth(Now, false);
			return true;
		}
		if (AtRegister)
		{
			AtRegister = false;
			return true;
		}
		return false;
	}

	bool PageKey(const std::string& Name, double Now) override
	{
		if (!Fitting.empty())
		{
			if (Name == "Enter" || Name == "Space" || Name == "Tab" || Name == "Reset")
			{
				LeaveBooth(Now, true);
			}
			else if (Name == "Backspace" || Name == "Delete" || Name == "PutBack" || Name == "Alt" || Name == "F" || Name == "RT")
			{
				LeaveBooth(Now, false);
			}
			return true;
		}
		if (AtRegister)
		{
			if (Name == "Left" || Name == "Right" || Name == "Up" || Name == "Down")
			{
				WearItOut = !WearItOut;
			}
			else if (Name == "Enter" || Name == "Space" || Name == "Tab" || Name == "Reset")
			{
				Pay(Now);
			}
			else if (Name == "Backspace" || Name == "Delete" || Name == "PutBack")
			{
				AtRegister = false;
			}
			return true;
		}
		const std::vector<const wardrobe::Garment*> On = OnTab();
		if (GridKey(Name, static_cast<int>(On.size()), Cols, Now))
		{
			return true;
		}
		const wardrobe::Garment* G = Sel < static_cast<int>(On.size()) ? On[static_cast<size_t>(Sel)] : nullptr;
		if ((Name == "Enter" || Name == "Space") && G)
		{
			Pick(*G, Now);
			return true;
		}
		if ((Name == "Backspace" || Name == "Delete" || Name == "PutBack") && G)
		{
			RemoveLine(G->Id, Now);
			return true;
		}
		if ((Name == "Alt" || Name == "F" || Name == "RT") && G && Tab != DonateTab)
		{
			TryOn(*G, Now);
			return true;
		}
		return false;
	}

	void RemoveLine(const std::string& Id, double Now) override
	{
		std::vector<std::string>& List = Tab == DonateTab ? Bin : Basket;
		const auto It = std::find(List.begin(), List.end(), Id);
		if (It != List.end())
		{
			List.erase(It);
			Touch(Id, Now);
			AtRegister = false;
		}
	}

	Backdrop Backing() const override { return Fitting.empty() ? Backdrop::Full : Backdrop::Low; }

	std::vector<Hint> Hints() const override
	{
		if (!Fitting.empty())
		{
			return {{"ENTER", "A", "KEEP IT"}, {"BACKSPACE", "X", "PUT IT BACK"}};
		}
		if (AtRegister)
		{
			return {{"LEFT", "DPAD", "WEAR IT OUT"}, {"TAB", "Y", "PAY"}};
		}
		if (Tab == DonateTab)
		{
			return {{"UP", "DPAD", "CHOOSE"}, {"ENTER", "A", "IN THE BIN"}, {"TAB", "Y", "DONATE"}};
		}
		return {{"UP", "DPAD", "CHOOSE"}, {"ENTER", "A", "ADD"}, {"F", "RT", "TRY ON"}, {"TAB", "Y", "PAY"}};
	}

	std::string KeeperAway() const override { return std::string(); }

	void Body(Canvas& C, const Rect& R, double Now) override
	{
		if (!Fitting.empty())
		{
			Booth(C, R, Now);
			return;
		}
		const std::vector<const wardrobe::Garment*> On = OnTab();
		const double World = S.WorldMinutes();
		Cols = R.W >= 1100.0f ? 4 : R.W >= 760.0f ? 3 : 2;
		std::string Top;
		if (Tab == DonateTab)
		{
			Top = "What you own. Anything in the bin goes to St. Agnes; anything you're wearing comes off first.";
		}
		else
		{
			const int Day = net::DayOf(World);
			const int Wd = net::Weekday(Day);
			const int ToThu = ((3 - Wd) % 7 + 7) % 7;
			Top = std::string(Wd == 5 ? "Yellow tags half off today. " : "Yellow tags half off Saturdays. ") + "New racks " + (ToThu == 0 ? "came in this morning." : ToThu == 1 ? "tomorrow." : "Thursday.");
		}
		C.Text(Top, R.X, R.Y + 10.0f, Ts(14.0f, 500, MenuMuted, Align::Left, Baseline::Alphabetic, false, R.W));
		if (On.empty())
		{
			C.Text(Tab == DonateTab ? "Nothing to give: the closet's empty." : "This rack's picked over. Thursday brings more.", R.X, R.Y + 70.0f, Ts(18.0f, 500, MenuDim));
			return;
		}
		Grid(
			C, {R.X, R.Y + 34.0f, R.W, R.H - 34.0f}, static_cast<int>(On.size()), Cols, 248.0f,
			[&](int I, const Rect& Cell, bool Focus, bool Over) { Card(C, *On[static_cast<size_t>(I)], Cell, Focus, Over, Now); }, [&](int I) {
				if (SkipPick)
				{
					SkipPick = false;
					return;
				}
				Pick(*On[static_cast<size_t>(I)], Now);
			},
			Now);
	}

	Receipt Bill(double /*Now*/) override
	{
		const double World = S.WorldMinutes();
		Receipt R;
		if (Tab == DonateTab)
		{
			R.Paper = PaperKind::Donation;
			R.Title = "ST. AGNES OUTREACH";
			R.Head = {"SECOND CHANCE THRIFT \xC2\xB7 220 MARKET ST.", "DONATION RECEIPT \xC2\xB7 " + Upper(Stamped(World))};
			const std::vector<std::string>& List = Bin.empty() ? Given : Bin;
			for (const std::string& Id : List)
			{
				if (const wardrobe::Garment* G = wardrobe::Find(Id))
				{
					Line L;
					L.Id = Id;
					L.Left = G->Name;
					L.Right = ConditionWord(G->Condition);
					L.Removable = !Bin.empty();
					R.Lines.push_back(L);
				}
			}
			R.Empty = "The bin's empty.";
			R.Foot = {"Received with thanks.", "No goods or services were provided", "in exchange. Keep for your tax records."};
			return R;
		}
		R.Paper = PaperKind::Thermal;
		R.Title = "SECOND CHANCE THRIFT";
		R.Head = {"220 MARKET ST. \xC2\xB7 ST. AGNES OUTREACH", Upper(Stamped(World))};
		const std::vector<std::string>& List = Basket.empty() ? Paid : Basket;
		Chips Sub = 0;
		for (const std::string& Id : List)
		{
			if (const wardrobe::Garment* G = wardrobe::Find(Id))
			{
				const Chips P = wardrobe::ThriftPrice(*G, World);
				Line L;
				L.Id = Id;
				L.Left = Upper(G->Name);
				L.Mid = shopart::YellowTag(*G, World) && net::Weekday(net::DayOf(World)) == 5 ? "\xC2\xBD YELLOW" : "";
				L.Right = Money(P);
				L.Removable = !Basket.empty();
				R.Lines.push_back(L);
				Sub += P;
			}
		}
		R.Empty = "Bring it to the register.";
		if (!List.empty())
		{
			R.Sums.push_back({"subtotal", "SUBTOTAL", "", Money(Sub)});
			R.Sums.push_back({"tax", "TAX 7.25%", "", Money(store::Tax(Sub))});
			R.Total = {"total", "TOTAL", "", Money(Basket.empty() ? Sub + store::Tax(Sub) : Total())};
		}
		R.Foot = {"ALL SALES FINAL \xC2\xB7 GOD BLESS", "CARD BALANCE " + Money(S.BankrollCents)};
		return R;
	}

	void Side(Canvas& C, const Rect& R, double Now) override
	{
		float Y = DrawBill(C, Bill(Now), R.X, R.Y, Now);
		if (Tab != DonateTab && !Basket.empty())
		{
			// "Wear it out": on by default; at the register it's the one thing to decide.
			const Rect Box{R.X, Y + 24.0f, ReceiptW, 44.0f};
			const bool Over = Hover(Box);
			C.FillRoundRect(Box, 8.0f, Paint(Fade(MenuInk, AtRegister ? 0.1f : Over ? 0.08f : 0.04f)));
			if (AtRegister)
			{
				C.StrokeRoundRect(Box, 8.0f, Fade(Accent("thrift"), 0.9f), 1.5f);
			}
			C.StrokeRoundRect({Box.X + 14.0f, Box.Y + 12.0f, 20.0f, 20.0f}, 4.0f, MenuInk, 1.5f);
			if (WearItOut)
			{
				C.StrokePolyline({{Box.X + 18.0f, Box.Y + 22.0f}, {Box.X + 23.0f, Box.Y + 27.0f}, {Box.X + 31.0f, Box.Y + 16.0f}}, false, Mint, 2.6f, true);
			}
			TrackedText(C, "WEAR IT OUT", Box.X + 46.0f, Box.Y + 27.0f, 13.0f, 800, MenuInk, 2.6f);
			C.Text(WearItOut ? "Bea bags your old things" : "Bea bags it all", Box.X + Box.W - 14.0f, Box.Y + 27.0f, Ts(12.0f, 500, MenuMuted, Align::Right));
			if (Released(Box))
			{
				WearItOut = !WearItOut;
			}
			Y = Box.Y + Box.H;
		}
		DrawButton(C, Main(Now), R.X, Y + 26.0f, Now);
	}

	Button Main(double /*Now*/) override
	{
		Button B;
		if (!Fitting.empty())
		{
			B.Label = "KEEP IT";
			B.Enabled = true;
			return B;
		}
		if (Tab == DonateTab)
		{
			B.Label = Bin.empty() ? "PICK SOMETHING TO GIVE" : "DONATE " + std::to_string(Bin.size()) + (Bin.size() == 1 ? " PIECE" : " PIECES");
			B.Enabled = !Bin.empty();
			return B;
		}
		if (Basket.empty())
		{
			B.Label = "PAY";
			return B;
		}
		B.Enabled = true;
		const Chips Due = Total();
		if (!AtRegister)
		{
			B.Label = "TO THE REGISTER \xC2\xB7 " + Money(Due);
			return B;
		}
		B.Warn = Due > S.BankrollCents;
		B.Label = B.Warn ? "CARD WON'T COVER IT" : "PAY " + Money(Due);
		return B;
	}

	void DoMain(double Now) override
	{
		if (!Fitting.empty())
		{
			LeaveBooth(Now, true);
			return;
		}
		if (Tab == DonateTab)
		{
			Donate(Now);
			return;
		}
		if (Basket.empty())
		{
			Say("browse", Now);
			return;
		}
		if (!AtRegister)
		{
			AtRegister = true;
			return;
		}
		Pay(Now);
	}

private:
	std::vector<const wardrobe::Garment*> OnTab() const
	{
		std::vector<const wardrobe::Garment*> Out;
		if (Tab == DonateTab)
		{
			for (const std::string& Id : S.Life.Closet.Owned)
			{
				const wardrobe::Garment* G = wardrobe::Find(Id);
				if (G && std::find(Given.begin(), Given.end(), Id) == Given.end())
				{
					Out.push_back(G);
				}
			}
			return Out;
		}
		for (const std::string& Id : shopart::ThriftRack(S, Caps.GarmentKinds))
		{
			const wardrobe::Garment* G = wardrobe::Find(Id);
			if (G && std::string(shopart::RackOf(*G)) == Racks[Tab].Id)
			{
				Out.push_back(G);
			}
		}
		return Out;
	}

	bool Worn(const std::string& Id) const
	{
		return std::find(S.Life.Closet.Worn.begin(), S.Life.Closet.Worn.end(), Id) != S.Life.Closet.Worn.end();
	}

	Chips Total()
	{
		const Session::ShopOrder O = GarmentOrder();
		const Chips Core = S.ShopTotal(O);
		if (Core > 0)
		{
			return Core;
		}
		Chips Sub = 0;
		for (const std::string& Id : Basket)
		{
			if (const wardrobe::Garment* G = wardrobe::Find(Id))
			{
				Sub += wardrobe::ThriftPrice(*G, S.WorldMinutes());
			}
		}
		return Sub + store::Tax(Sub);
	}

	Session::ShopOrder GarmentOrder() const
	{
		Session::ShopOrder O;
		O.Venue = "thrift";
		O.Garments = Basket;
		return O;
	}

	void Pick(const wardrobe::Garment& G, double Now)
	{
		if (Tab == DonateTab)
		{
			const auto It = std::find(Bin.begin(), Bin.end(), G.Id);
			if (It != Bin.end())
			{
				Bin.erase(It);
			}
			else
			{
				Bin.push_back(G.Id);
				Given.clear();
				StampWas = StampKind::None;
			}
			Touch(G.Id, Now);
			return;
		}
		if (S.Life.Closet.Owned.count(G.Id) > 0)
		{
			SayText(std::find(Paid.begin(), Paid.end(), G.Id) != Paid.end() ? "That's yours now, sweetheart. Paid and bagged." : "You've got that one already. Leave it for somebody.", Now);
			return;
		}
		AtRegister = false;
		const auto It = std::find(Basket.begin(), Basket.end(), G.Id);
		if (It != Basket.end())
		{
			Basket.erase(It);
		}
		else
		{
			if (Basket.empty())
			{
				Paid.clear();
				StampWas = StampKind::None;
			}
			Basket.push_back(G.Id);
			// Bea's story about the box from Florida is a jacket's; any other rare find gets her usual nod.
			if (G.Rare && G.Where == wardrobe::Slot::Outer)
			{
				SayText("That jacket came in a box from a man who moved to Florida. You wear it better.", Now);
			}
			else
			{
				Say("pick", Now);
			}
		}
		Touch(G.Id, Now);
	}

	void TryOn(const wardrobe::Garment& G, double Now)
	{
		Fitting = G.Id;
		FitAt = Now;
		wardrobe::Closet Try = S.Life.Closet;
		Try.Owned.insert(G.Id);
		Try.Worn[static_cast<size_t>(G.Where)] = G.Id;
		Try.Seeded = true;
		Preview(Dressed(S.Person.Appearance, Try), Try);
		Camera("thrift-fitting");
		Say("try", Now);
	}

	void LeaveBooth(double Now, bool Keep)
	{
		const std::string Id = Fitting;
		Fitting.clear();
		EndPreview();
		Camera("thrift-rack");
		if (Keep)
		{
			if (const wardrobe::Garment* G = wardrobe::Find(Id))
			{
				if (std::find(Basket.begin(), Basket.end(), Id) == Basket.end() && S.Life.Closet.Owned.count(Id) == 0)
				{
					Pick(*G, Now);
				}
			}
		}
	}

	void Pay(double Now)
	{
		const Chips Due = Total();
		const std::vector<std::string> Bought = Basket;
		const std::string Why = S.ShopPay(GarmentOrder());
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
			AtRegister = false;
			return;
		}
		Paid = Bought;
		Basket.clear();
		AtRegister = false;
		Stamp(StampKind::Paid, Now);
		Say("pay", Now);
		if (WearItOut)
		{
			bool Changed = false;
			for (const std::string& Id : Bought)
			{
				Changed = S.Wear(Id).empty() || Changed;
			}
			if (Changed)
			{
				Redress();
			}
		}
	}

	void Donate(double Now)
	{
		if (Bin.empty())
		{
			return;
		}
		bool OffTheBack = false;
		std::vector<std::string> Done;
		std::string Why;
		for (const std::string& Id : Bin)
		{
			const bool WasWorn = Worn(Id);
			const std::string Res = S.Donate(Id);
			if (Res.empty())
			{
				Done.push_back(Id);
				OffTheBack = OffTheBack || WasWorn;
			}
			else
			{
				Why = Res;
			}
		}
		Bin.clear();
		if (Done.empty())
		{
			SayText(Why, Now, true);
			return;
		}
		Given = Done;
		Stamp(StampKind::Thanks, Now);
		Say("donate", Now);
		if (OffTheBack)
		{
			Redress();
		}
		Sel = 0;
	}

	void Booth(Canvas& C, const Rect& R, double Now)
	{
		const wardrobe::Garment* G = wardrobe::Find(Fitting);
		if (!G)
		{
			return;
		}
		const double World = S.WorldMinutes();
		const float H = 220.0f;
		const Rect Panel{R.X, 1080.0f - 96.0f - H - 30.0f, std::min(R.W, 900.0f), H};
		const float In = Ease((Now - FitAt) / 0.35);
		C.FillRoundRect({Panel.X, Panel.Y + (1.0f - In) * 30.0f, Panel.W, Panel.H}, 14.0f, Paint(Rgba(8, 10, 16, 0.86f * In)));
		float X = Panel.X + 28.0f;
		if (!Caps.Preview3D)
		{
			// No 3D to dress: the booth's mirror is the portrait.
			wardrobe::Closet Try = S.Life.Closet;
			Try.Owned.insert(G->Id);
			Try.Worn[static_cast<size_t>(G->Where)] = G->Id;
			Try.Seeded = true;
			hero::Character Me = S.Person;
			Me.Appearance = Dressed(S.Person.Appearance, Try);
			C.PushClip({X, Panel.Y + 12.0f, 180.0f, H - 24.0f});
			C.FillRect({X, Panel.Y + 12.0f, 180.0f, H - 24.0f}, Paint(Hex(0x1a2230)));
			DrawPortrait(C, Me, X + 90.0f, Panel.Y + 92.0f, 0.42f, Now);
			C.PopClip();
			X += 204.0f;
		}
		shopart::Garment(C, *G, {X, Panel.Y + 26.0f, 150.0f, 150.0f});
		X += 172.0f;
		TrackedText(C, "IN THE BOOTH", X, Panel.Y + 46.0f, 11.0f, 800, Accent("thrift"), 2.6f);
		C.Text(G->Name, X, Panel.Y + 84.0f, Ts(26.0f, 900, MenuInk, Align::Left, Baseline::Alphabetic, false, Panel.X + Panel.W - X - 24.0f));
		C.Text(std::string(ConditionWord(G->Condition)) + " \xC2\xB7 " + Money(wardrobe::ThriftPrice(*G, World)) + (Caps.Preview3D ? " \xC2\xB7 turn with the mouse or the right stick" : ""), X, Panel.Y + 112.0f,
			Ts(15.0f, 500, MenuMuted, Align::Left, Baseline::Alphabetic, false, Panel.X + Panel.W - X - 24.0f));
		const Rect Keep{X, Panel.Y + 140.0f, 180.0f, 50.0f};
		const Rect Back{X + 196.0f, Panel.Y + 140.0f, 200.0f, 50.0f};
		C.FillRoundRect(Keep, 8.0f, Paint(Mix(Mint, Hex(0xffffff), Hover(Keep) ? 0.15f : 0.0f)));
		TrackedText(C, "KEEP IT", Keep.X + Keep.W * 0.5f, Keep.Y + 32.0f, 15.0f, 900, Hex(0x05140c), 2.6f, Align::Center);
		C.FillRoundRect(Back, 8.0f, Paint(Fade(MenuInk, Hover(Back) ? 0.16f : 0.08f)));
		TrackedText(C, "PUT IT BACK", Back.X + Back.W * 0.5f, Back.Y + 32.0f, 15.0f, 900, MenuInk, 2.6f, Align::Center);
		if (Released(Keep))
		{
			LeaveBooth(Now, true);
		}
		else if (Released(Back))
		{
			LeaveBooth(Now, false);
		}
	}

	void Card(Canvas& C, const wardrobe::Garment& G, const Rect& R, bool Focus, bool Over, double Now)
	{
		const double World = S.WorldMinutes();
		const bool Have = Tab != DonateTab && S.Life.Closet.Owned.count(G.Id) > 0;
		const bool InBasket = std::find(Basket.begin(), Basket.end(), G.Id) != Basket.end();
		const bool InBin = std::find(Bin.begin(), Bin.end(), G.Id) != Bin.end();
		CardBack(C, R, Focus, Over, Have, Now);
		const float A0 = C.GetAlpha();
		if (Have)
		{
			C.SetAlpha(A0 * 0.5f);
		}
		shopart::Garment(C, G, {R.X + 14.0f, R.Y + 14.0f, R.W - 28.0f, R.H - 112.0f});
		C.SetAlpha(A0);
		const float Ty = R.Y + R.H - 70.0f;
		C.Text(G.Name, R.X + 14.0f, Ty, Ts(17.0f, 800, Have ? MenuMuted : MenuInk, Align::Left, Baseline::Alphabetic, false, R.W - 28.0f));
		C.Text(std::string(ConditionWord(G.Condition)) + (G.Rare ? " \xC2\xB7 a rare find" : ""), R.X + 14.0f, Ty + 20.0f, Ts(12.0f, 600, G.Rare ? MenuGold : MenuMuted, Align::Left, Baseline::Alphabetic, false, R.W - 28.0f));
		if (Tab != DonateTab)
		{
			// The tag: its color, the price Bea's gun printed, and Saturday's half off on the yellow ones.
			const Chips P = wardrobe::ThriftPrice(G, World);
			const bool Half = shopart::YellowTag(G, World) && net::Weekday(net::DayOf(World)) == 5;
			const Rect Tag{R.X + R.W - 96.0f, R.Y + R.H - 46.0f, 82.0f, 32.0f};
			C.FillPolygon({{Tag.X + 10.0f, Tag.Y}, {Tag.X + Tag.W, Tag.Y}, {Tag.X + Tag.W, Tag.Y + Tag.H}, {Tag.X + 10.0f, Tag.Y + Tag.H}, {Tag.X, Tag.Y + Tag.H * 0.5f}},
				Paint(Hex(shopart::TagColor(G, World))));
			C.FillCircle(Tag.X + 9.0f, Tag.Y + Tag.H * 0.5f, 3.0f, Paint(Hex(0x1a1408)));
			C.Text(Price(P), Tag.X + Tag.W * 0.5f + 5.0f, Tag.Y + 22.0f, Ts(16.0f, 900, Hex(0x1a1408), Align::Center));
			if (Half)
			{
				const Chips Was = wardrobe::ThriftPrice(G, World - 1440.0);
				if (Was > P)
				{
					const std::string WasText = Price(Was);
					const float Ww = C.Measure(WasText, 12.0f, 700);
					C.Text(WasText, Tag.X - 8.0f, Tag.Y + 21.0f, Ts(12.0f, 700, MenuMuted, Align::Right));
					C.FillRect({Tag.X - 8.0f - Ww, Tag.Y + 16.0f, Ww, 1.5f}, Paint(MenuWarn));
				}
			}
		}
		float Bx = R.X + 12.0f;
		if (InBasket)
		{
			Bx += Badge(C, "ON THE RECEIPT", Bx, R.Y + 12.0f, Mint, true) + 6.0f;
		}
		if (InBin)
		{
			Bx += Badge(C, "IN THE BIN", Bx, R.Y + 12.0f, Accent("thrift"), true) + 6.0f;
		}
		if (Have)
		{
			Bx += Badge(C, std::find(Paid.begin(), Paid.end(), G.Id) != Paid.end() ? "JUST BOUGHT" : "YOU OWN IT", Bx, R.Y + 12.0f, MenuInk) + 6.0f;
		}
		if (Tab == DonateTab && Worn(G.Id))
		{
			Badge(C, "WEARING IT", Bx, R.Y + 12.0f, MenuGold);
		}
		if (Focus && Tab != DonateTab && !Have)
		{
			// TRY ON, for the mouse.
			const Rect Try{R.X + R.W - 96.0f, R.Y + 12.0f, 84.0f, 26.0f};
			C.FillRoundRect(Try, 13.0f, Paint(Fade(MenuInk, Hover(Try) ? 0.24f : 0.12f)));
			TrackedText(C, "TRY ON", Try.X + Try.W * 0.5f, Try.Y + 18.0f, 11.0f, 800, MenuInk, 2.2f, Align::Center);
			if (Released(Try))
			{
				TryOn(G, Now);
				SkipPick = true;
			}
		}
	}

	std::vector<std::string> Basket; // garment ids on the receipt
	std::vector<std::string> Paid;   // the last receipt paid (shown until something new goes on it)
	std::vector<std::string> Bin;    // the donation bin
	std::vector<std::string> Given;  // the last donation (its receipt)
	std::string Fitting;             // the garment in the booth
	double FitAt = -10.0;
	bool AtRegister = false;
	bool WearItOut = true;
	bool SkipPick = false; // a click on a card's TRY ON isn't also a click on the card
	double LeftAt = -1.0e9; // when the screen last closed (world minutes): the basket's still in hand a while after
	static constexpr double CarryMinutes = 30.0;
	int Cols = 3;
};
} // namespace

std::unique_ptr<ShopScreen> MakeThrift(Session& S)
{
	return std::make_unique<ThriftPage>(S);
}
} // namespace shopframe
} // namespace ui
} // namespace ss
