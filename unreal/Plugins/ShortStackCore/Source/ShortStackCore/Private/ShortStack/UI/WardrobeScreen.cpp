// The closet (the realism update's spec, §7.4 and §7.6): what the hero owns, slot by slot (jacket, top, pants, shoes,
// hat, glasses), with the one in focus on the hero (PreviewLook) or on the portrait, Wear and Take off (Redress), and
// what the outfit's Style comes to. Opened in the apartment, on Dee's couch, in the thrift's booth and by its "Wear it
// out".
#include "ShopFrame.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Format.h"
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

struct SlotTab
{
	const char* Tab;
	wardrobe::Slot Slot;
	const char* Bare; // what "nothing" in the slot is
};
const SlotTab Slots[] = {
	{"JACKETS", wardrobe::Slot::Outer, "No jacket"},
	{"TOPS", wardrobe::Slot::Top, "Just a tee"},
	{"PANTS", wardrobe::Slot::Bottom, "Shorts"},
	{"SHOES", wardrobe::Slot::Shoes, "Barefoot"},
	{"HATS", wardrobe::Slot::Hat, "No hat"},
	{"GLASSES", wardrobe::Slot::Eyes, "No glasses"},
};
constexpr int SlotTabs = 6;

class WardrobePage final : public Page
{
public:
	explicit WardrobePage(Session& InSession) : Page(InSession, "wardrobe") {}

protected:
	std::vector<std::string> TabNames() const override
	{
		std::vector<std::string> Out;
		for (const SlotTab& T : Slots)
		{
			Out.push_back(T.Tab);
		}
		return Out;
	}

	void Opened(double Now, const std::string& PageId) override
	{
		Where = PageId;
		ShownKey.clear();
		// From the thrift's "Wear it out", open on the jackets; anywhere else, on the jacket too: it's what shows most.
		Tab = 0;
		Focus(Now);
		SayText(Where == "dee" ? "Your bag's under the couch. Dee says fold what you don't wear." : Where == "thrift-booth" || Where == "thrift" ? "The curtain sticks. Pull, don't yank." : "", Now);
		Refresh();
	}

	void TabChanged(double Now) override
	{
		Focus(Now);
		Refresh();
	}

	void Closing(double /*Now*/) override { ShownKey.clear(); }

	std::string Title() const override
	{
		if (Where == "dee")
		{
			return "Dee's Couch";
		}
		if (Where == "thrift-booth" || Where == "thrift")
		{
			return "The Fitting Booth";
		}
		return "Your Closet";
	}

	std::string Under() const override
	{
		if (Where == "dee")
		{
			return "DEE'S FRONT ROOM \xC2\xB7 YOUR BAG, YOUR THINGS";
		}
		if (Where == "thrift-booth" || Where == "thrift")
		{
			return "SECOND CHANCE THRIFT \xC2\xB7 220 MARKET ST.";
		}
		return S.Evicted() ? "WHAT YOU CARRY" : "1812 FIFTH ST. \xC2\xB7 THE CLOSET BY THE DOOR";
	}

	std::string KeeperAway() const override { return Note.empty() ? std::string() : std::string(Where == "dee" ? "DEE" : "BEA \xC2\xB7 VOLUNTEER"); }

	bool PageKey(const std::string& Name, double Now) override
	{
		const std::vector<std::string> On = Items();
		if (GridKey(Name, static_cast<int>(On.size()), Cols, Now))
		{
			Refresh();
			return true;
		}
		if ((Name == "Enter" || Name == "Space") && Sel < static_cast<int>(On.size()))
		{
			Choose(On[static_cast<size_t>(Sel)], Now);
			return true;
		}
		if (Name == "Alt" || Name == "F" || Name == "RT" || Name == "Backspace" || Name == "Delete" || Name == "PutBack")
		{
			Choose(std::string(), Now);
			return true;
		}
		if (Name == "Tab" || Name == "Reset")
		{
			WalkAway(Now);
			return true;
		}
		return false;
	}

	Backdrop Backing() const override { return Caps.Preview3D ? Backdrop::Split : Backdrop::Full; }
	float BodyWidth(float LeftW) const override { return Caps.Preview3D ? std::clamp(LeftW * 0.6f, std::min(LeftW, 600.0f), 820.0f) : LeftW; }

	std::vector<Hint> Hints() const override { return {{"UP", "DPAD", "CHOOSE"}, {"ENTER", "A", "WEAR"}, {"F", "RT", "TAKE OFF"}, {"TAB", "Y", "DONE"}}; }

	void Body(Canvas& C, const Rect& R, double Now) override
	{
		const float Bw = BodyWidth(R.W);
		const std::vector<std::string> On = Items();
		Cols = Bw >= 1000.0f ? 4 : Bw >= 700.0f ? 3 : 2;
		const SlotTab& T = Slots[Tab];
		const std::string Now_ = Worn(T.Slot);
		const wardrobe::Garment* Cur = wardrobe::Find(Now_);
		C.Text("Wearing: " + (Cur ? Cur->Name : std::string(T.Bare)), R.X, R.Y + 10.0f, Ts(14.0f, 500, MenuMuted, Align::Left, Baseline::Alphabetic, false, Bw));
		Grid(
			C, {R.X, R.Y + 34.0f, Bw, R.H - 34.0f}, static_cast<int>(On.size()), Cols, 232.0f,
			[&](int I, const Rect& Cell, bool Focus, bool Over) { Card(C, On[static_cast<size_t>(I)], Cell, Focus, Over, Now); }, [&](int I) { Choose(On[static_cast<size_t>(I)], Now); }, Now);
		Refresh();
	}

	Receipt Bill(double /*Now*/) override { return Receipt(); }

	void Side(Canvas& C, const Rect& R, double Now) override
	{
		// The mirror: the hero in what's in focus (the 3D shows it when it can; the portrait always says it).
		const wardrobe::Closet Try = Trying();
		hero::Character Me = S.Person;
		Me.Appearance = Dressed(S.Person.Appearance, Try);
		const float Ph = Caps.Preview3D ? 0.0f : 430.0f;
		float Y = R.Y;
		if (Ph > 0.0f)
		{
			const Rect Frame{R.X, Y, ReceiptW, Ph};
			C.FillRoundRect(Frame, 10.0f, Paint::Radial({Frame.X + Frame.W * 0.5f, Frame.Y + 120.0f}, 0.0f, {Frame.X + Frame.W * 0.5f, Frame.Y + 200.0f}, 380.0f, Hex(0x2a3442), 0.55f, Hex(0x161d27), Hex(0x0b0f15)));
			C.PushClip(Frame);
			DrawPortrait(C, Me, Frame.X + Frame.W * 0.5f, Frame.Y + 170.0f, 0.95f, Now);
			C.PopClip();
			C.StrokeRoundRect(Frame, 10.0f, Rgba(255, 255, 255, 0.12f), 1.0f);
			Y += Ph + 24.0f;
		}
		// What's on, slot by slot, and the outfit's Style.
		TrackedText(C, "WEARING", R.X, Y + 14.0f, 11.0f, 800, MenuMuted, 2.6f);
		Y += 26.0f;
		for (int I = 0; I < SlotTabs; ++I)
		{
			const std::string& Id = Try.Worn[static_cast<size_t>(Slots[I].Slot)];
			const wardrobe::Garment* G = wardrobe::Find(Id);
			const bool Changing = Id != S.Life.Closet.Worn[static_cast<size_t>(Slots[I].Slot)];
			Y += 30.0f;
			C.Text(Slots[I].Tab, R.X, Y, Ts(12.0f, 800, MenuDim));
			C.Text(G ? G->Name : std::string(Slots[I].Bare), R.X + 110.0f, Y, Ts(16.0f, Changing ? 800 : 500, Changing ? Accent("wardrobe") : MenuInk, Align::Left, Baseline::Alphabetic, false, ReceiptW - 110.0f));
		}
		Y += 40.0f;
		const double Style = wardrobe::StyleOf(Try);
		TrackedText(C, "STYLE", R.X, Y, 11.0f, 800, MenuMuted, 2.6f);
		const float Bw = ReceiptW - 140.0f;
		C.FillRoundRect({R.X + 80.0f, Y - 9.0f, Bw, 8.0f}, 4.0f, Paint(Fade(MenuInk, 0.1f)));
		C.FillRoundRect({R.X + 80.0f, Y - 9.0f, std::max(8.0f, Bw * static_cast<float>(std::clamp(Style, 0.0, 1.0))), 8.0f}, 4.0f, Paint(Accent("wardrobe")));
		C.Text(std::to_string(static_cast<int>(std::lround(Style * 100.0))), R.X + ReceiptW, Y, Ts(14.0f, 800, MenuInk, Align::Right));
		Y += 22.0f;
		C.Text("Tips on stream run up to 2% higher in a sharper outfit.", R.X, Y, Ts(13.0f, 500, MenuMuted, Align::Left, Baseline::Alphabetic, false, ReceiptW));
		DrawButton(C, Main(Now), R.X, std::max(Y + 30.0f, FooterY - 40.0f - 66.0f - 10.0f), Now);
	}

	Button Main(double /*Now*/) override
	{
		Button B;
		B.Label = "DONE";
		B.Enabled = true;
		return B;
	}

	void DoMain(double Now) override { WalkAway(Now); }

private:
	std::string Worn(wardrobe::Slot Sl) const { return S.Life.Closet.Worn[static_cast<size_t>(Sl)]; }

	/** What's owned for this slot: "" (nothing in it) first, then each garment. */
	std::vector<std::string> Items() const
	{
		std::vector<std::string> Out = {std::string()};
		for (const std::string& Id : S.Life.Closet.Owned)
		{
			const wardrobe::Garment* G = wardrobe::Find(Id);
			if (G && G->Where == Slots[Tab].Slot)
			{
				Out.push_back(Id);
			}
		}
		return Out;
	}

	void Focus(double Now)
	{
		const std::vector<std::string> On = Items();
		const std::string Cur = Worn(Slots[Tab].Slot);
		Sel = 0;
		for (size_t I = 0; I < On.size(); ++I)
		{
			Sel = On[I] == Cur ? static_cast<int>(I) : Sel;
		}
		SelAt = Now;
	}

	/** The closet with the one in focus on. */
	wardrobe::Closet Trying() const
	{
		wardrobe::Closet Try = S.Life.Closet;
		const std::vector<std::string> On = Items();
		if (Sel >= 0 && Sel < static_cast<int>(On.size()))
		{
			Try.Worn[static_cast<size_t>(Slots[Tab].Slot)] = On[static_cast<size_t>(Sel)];
		}
		return Try;
	}

	void Refresh()
	{
		if (!IsOpen())
		{
			return;
		}
		const wardrobe::Closet Try = Trying();
		std::string Key;
		for (const std::string& W : Try.Worn)
		{
			Key += W + "|";
		}
		if (Key != ShownKey)
		{
			ShownKey = Key;
			Preview(Dressed(S.Person.Appearance, Try), Try);
		}
	}

	void Choose(const std::string& Id, double Now)
	{
		const wardrobe::Slot Sl = Slots[Tab].Slot;
		const std::string Why = Id.empty() ? S.TakeOff(static_cast<int>(Sl)) : S.Wear(Id);
		if (!Why.empty())
		{
			SayText(Why, Now, true);
			return;
		}
		if (Id.empty())
		{
			Sel = 0;
		}
		Redress();
		ShownKey.clear();
		Refresh();
	}

	void Card(Canvas& C, const std::string& Id, const Rect& R, bool Focus, bool Over, double Now)
	{
		const SlotTab& T = Slots[Tab];
		const bool On = Worn(T.Slot) == Id;
		CardBack(C, R, Focus, Over, false, Now);
		const wardrobe::Garment* G = wardrobe::Find(Id);
		if (G)
		{
			shopart::Garment(C, *G, {R.X + 14.0f, R.Y + 14.0f, R.W - 28.0f, R.H - 96.0f});
		}
		else
		{
			// Nothing: an empty hanger.
			const float Cx = R.X + R.W * 0.5f;
			const float Cy = R.Y + (R.H - 96.0f) * 0.5f + 14.0f;
			C.StrokePolyline({{Cx, Cy - 26.0f}, {Cx - 50.0f, Cy + 14.0f}, {Cx + 50.0f, Cy + 14.0f}, {Cx, Cy - 26.0f}}, false, Fade(MenuInk, 0.35f), 3.0f, true);
			C.StrokeArc(Cx + 6.0f, Cy - 34.0f, 7.0f, 3.14159f, 6.28318f, Fade(MenuInk, 0.35f), 3.0f, true);
		}
		const float Ty = R.Y + R.H - 54.0f;
		C.Text(G ? G->Name : std::string(T.Bare), R.X + 14.0f, Ty, Ts(17.0f, 800, MenuInk, Align::Left, Baseline::Alphabetic, false, R.W - 28.0f));
		const std::string Sub = G ? (G->Condition <= 0 ? std::string("Good") : G->Condition == 1 ? std::string("Worn") : std::string("Rough")) + (G->Tier >= 1 ? " \xC2\xB7 vintage" : "") : std::string("Take it off");
		C.Text(Sub, R.X + 14.0f, Ty + 20.0f, Ts(12.0f, 600, MenuMuted, Align::Left, Baseline::Alphabetic, false, R.W - 28.0f));
		if (On)
		{
			Badge(C, "WEARING", R.X + 12.0f, R.Y + 12.0f, Mint, true);
		}
	}

	std::string Where;
	std::string ShownKey;
	int Cols = 3;
};
} // namespace

std::unique_ptr<ShopScreen> MakeWardrobe(Session& S)
{
	return std::make_unique<WardrobePage>(S);
}
} // namespace shopframe
} // namespace ui
} // namespace ss
