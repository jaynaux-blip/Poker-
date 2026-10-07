// Fifth St. Barbers' Style Book (the realism update's spec, §5.1 and §7.4): CUT, BEARD, COLOR and BROWS, each choice a
// portrait of the hero with it, the mirror showing the one in focus (PreviewLook), Lonnie's length rule ("Give it
// three weeks"), the styles the 3D can't show marked PORTRAIT ONLY, the ticket with each service's minutes, the tip,
// and the chair's cinematic (BeginService) once it's paid.
#include "ShopFrame.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Body.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/UI/Portrait.h"

#include <algorithm>
#include <cmath>
#include <map>
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

const char* const BarberTabs[] = {"CUT", "BEARD", "COLOR", "BROWS"};

/** The longest length a rank allows, in words (§5.1's ranks). */
const char* RankWords(int Rank)
{
	switch (std::clamp(Rank, 0, 6))
	{
	case 0: return "a shave";
	case 1: return "a buzz";
	case 2: return "a crew cut";
	case 3: return "a crop or a side part";
	case 4: return "curls";
	case 5: return "an afro or a bob";
	default: return "shoulder length";
	}
}

const char* BeardWords(int Rank)
{
	switch (std::clamp(Rank, 0, 4))
	{
	case 0: return "a clean shave";
	case 1: return "stubble";
	case 2: return "a mustache or a goatee";
	case 3: return "a short beard";
	default: return "a full beard";
	}
}

bool Has(const std::vector<bool>& Caps, int Value)
{
	return Value >= 0 && Value < static_cast<int>(Caps.size()) && Caps[static_cast<size_t>(Value)];
}

std::string DaysAgo(double Since)
{
	const int Days = static_cast<int>(std::floor(Since / 1440.0));
	return Days <= 0 ? std::string("today") : Days == 1 ? std::string("yesterday") : std::to_string(Days) + " days ago";
}

class BarberPage final : public Page
{
public:
	explicit BarberPage(Session& InSession) : Page(InSession, "barber")
	{
		TipChoices = {0, 15, 20, 25};
		TipIndex = TipDefault = 2; // 20%, the default (§5.0)
		Seated = true;
	}

protected:
	std::vector<std::string> TabNames() const override { return {BarberTabs[0], BarberTabs[1], BarberTabs[2], BarberTabs[3]}; }

	void Opened(double Now, const std::string& PageId) override
	{
		Picks.clear();
		Reveal = false;
		ShownKey.clear();
		for (int I = 0; I < 4; ++I)
		{
			Tab = PageId == BarberTabs[I] ? I : Tab;
		}
		Camera("barber-mirror");
		Focus(Now);
		Refresh();
	}

	void TabChanged(double Now) override
	{
		Focus(Now);
		Refresh();
		if (Tab == 2)
		{
			Say("pick:color", Now);
		}
	}

	void Closing(double /*Now*/) override { ShownKey.clear(); }

	bool PageKey(const std::string& Name, double Now) override
	{
		const std::vector<const shop::Offer*> On = OnTab();
		if (GridKey(Name, static_cast<int>(On.size()), Cols, Now))
		{
			Refresh();
			return true;
		}
		if ((Name == "Enter" || Name == "Space") && Sel < static_cast<int>(On.size()))
		{
			Pick(*On[static_cast<size_t>(Sel)], Now);
			return true;
		}
		if (Name == "Backspace" || Name == "Delete" || Name == "PutBack")
		{
			const auto It = Picks.find(BarberTabs[Tab]);
			if (It != Picks.end())
			{
				RemoveLine(It->second, Now);
			}
			return true;
		}
		if (Name == "Alt" || Name == "F" || Name == "RT")
		{
			Reveal = !Reveal;
			Camera(Reveal ? "barber-reveal" : "barber-mirror");
			return true;
		}
		return false;
	}

	void RemoveLine(const std::string& Id, double Now) override
	{
		for (auto It = Picks.begin(); It != Picks.end(); ++It)
		{
			if (It->second == Id)
			{
				Picks.erase(It);
				Touch(Id, Now);
				Tipping = false;
				Refresh();
				return;
			}
		}
	}

	Backdrop Backing() const override { return Backdrop::Split; }
	float BodyWidth(float LeftW) const override { return std::clamp(LeftW * 0.56f, std::min(LeftW, 560.0f), 780.0f); }

	std::vector<Hint> Hints() const override
	{
		return {{"UP", "DPAD", "CHOOSE"}, {"ENTER", "A", "PICK"}, {"F", "RT", Reveal ? "THE MIRROR" : "THE BACK"}, {"TAB", "Y", "CONFIRM"}};
	}

	void Body(Canvas& C, const Rect& R, double Now) override
	{
		const float Bw = BodyWidth(R.W);
		const hero::Look Mine = S.Person.Appearance;
		const double World = S.WorldMinutes();
		// What Lonnie can do: the hair's grown back this far since the last cut (the length rule, §5.1).
		std::string Rule;
		if (Tab == 0 || Tab == 1)
		{
			const bool Hair = Tab == 0;
			// What Lonnie's books say. Every career starts with a cut and a shave a week back, from before the city: whether
			// the hero has sat in his chair is Groom.Cuts and the shop's visits (body::Sharp's rule), never CutAt. With
			// nothing on record (-1e9) there's no "Last shave 694,000 days ago" either.
			const auto Book = S.Life.Visits.find("barber");
			const bool Been = Book != S.Life.Visits.end() && Book->second.Count > 0;
			const body::Grooming& G = S.Life.Groom;
			std::string When;
			if (Hair)
			{
				When = G.Cuts > 0 && G.CutAt > -1.0e8 ? "Last cut " + DaysAgo(World - G.CutAt) : Been ? std::string("No cut on Lonnie's books yet") : std::string("First time in Lonnie's chair");
			}
			else
			{
				// No count of beard work is kept: a shave on record is a fact once he's had the hero in the chair.
				When = Been && G.ShaveAt > -1.0e8 ? "Last shave " + DaysAgo(World - G.ShaveAt) : std::string("No shave on Lonnie's books yet");
			}
			const int Rank = Hair ? body::HairAllowed(S.Life.Groom, Mine, World) : body::BeardAllowed(S.Life.Groom, Mine, World);
			Rule = When + " \xC2\xB7 grown out enough for " + (Hair ? RankWords(Rank) : BeardWords(Rank));
		}
		else if (Tab == 2)
		{
			Rule = "Color takes an hour. It washes out slow, if it washes out at all.";
		}
		else
		{
			Rule = "Brows only come thinner here. Thick, then natural, then thin.";
		}
		C.Text(Rule, R.X, R.Y + 10.0f, Ts(14.0f, 500, MenuMuted, Align::Left, Baseline::Alphabetic, false, Bw));

		// The book: a portrait of the hero with each choice (no hat in the chair).
		const std::vector<const shop::Offer*> On = OnTab();
		Cols = Bw >= 690.0f ? 3 : 2;
		if (On.empty())
		{
			C.Text("Lonnie hasn't put the prices up yet.", R.X, R.Y + 70.0f, Ts(18.0f, 500, MenuDim));
		}
		const Rect GridR{R.X, R.Y + 34.0f, Bw, R.H - 34.0f};
		Grid(
			C, GridR, static_cast<int>(On.size()), Cols, 214.0f,
			[&](int I, const Rect& Cell, bool Focus, bool Over) { Card(C, *On[static_cast<size_t>(I)], Cell, Focus, Over, Now); },
			[&](int I) { Pick(*On[static_cast<size_t>(I)], Now); }, Now);
		Refresh();

		// The mirror's side of the screen: the 3D hero when the host can dress them, else the portrait.
		const float Mx = R.X + Bw + 48.0f;
		const float Mw = C.Width() - Margin - ReceiptW - 48.0f - Mx;
		if (Mw < 200.0f)
		{
			return;
		}
		const hero::Look Shown_ = Previewed();
		if (!Caps.Preview3D)
		{
			const float Cx = Mx + Mw * 0.5f;
			const float Ry = std::min(R.H * 0.36f, Mw * 0.6f);
			const float Mid = R.Y + R.H * 0.42f;
			C.FillEllipse(Cx, Mid, Ry * 0.78f, Ry, Paint::Radial({Cx, Mid - Ry * 0.3f}, 0.0f, {Cx, Mid}, Ry, Hex(0x2c3a4a), 0.6f, Hex(0x18212d), Hex(0x0c1118)));
			C.PushClip({Cx - Ry * 0.78f, Mid - Ry, Ry * 1.56f, Ry * 2.0f});
			hero::Character Sitter = S.Person;
			Sitter.Appearance = Shown_;
			DrawPortrait(C, Sitter, Cx, Mid - Ry * 0.12f, Ry / 330.0f * 1.15f, Now);
			C.PopClip();
			C.StrokeEllipse(Cx, Mid, Ry * 0.78f, Ry, Hex(0xc9b27a), 6.0f);
			C.StrokeEllipse(Cx, Mid, Ry * 0.78f + 6.0f, Ry + 6.0f, Rgba(255, 236, 190, 0.18f), 2.0f);
		}
		const std::string What = PreviewName();
		if (!What.empty())
		{
			const float Tw = TrackedWidth(C, What, 13.0f, 800, 3.0f);
			const float Ty = R.Y + R.H - 10.0f;
			C.FillRoundRect({Mx + Mw * 0.5f - Tw * 0.5f - 18.0f, Ty - 24.0f, Tw + 36.0f, 36.0f}, 18.0f, Paint(Rgba(4, 6, 10, 0.7f)));
			TrackedText(C, What, Mx + Mw * 0.5f, Ty, 13.0f, 800, MenuInk, 3.0f, Align::Center);
		}
	}

	Chips TipBase() override { return Subtotal(); }

	Receipt Bill(double /*Now*/) override
	{
		const double World = S.WorldMinutes();
		Receipt R;
		R.Paper = PaperKind::Slip;
		R.Title = "FIFTH ST. BARBERS";
		R.Head = {"1818 FIFTH ST. \xC2\xB7 EST. 1995", Upper(Stamped(World))};
		const auto Vt = S.Life.Visits.find("barber");
		const int Count = Vt == S.Life.Visits.end() ? 0 : Vt->second.Count;
		R.Number = "No. " + std::to_string(100 + (net::DayOf(World) * 37 + Count * 11) % 900);
		R.Empty = "Pick a cut, a beard, a color.";
		int Minutes = 0;
		for (const shop::Offer* O : Ordered())
		{
			Line L;
			L.Id = O->Id;
			L.Left = Upper(O->Name);
			L.Mid = MinutesText(O->Minutes);
			L.Right = Money(O->PriceCents);
			L.Removable = true;
			R.Lines.push_back(L);
			Minutes += O->Minutes;
		}
		if (Combo())
		{
			Line L;
			L.Left = "CUT & BEARD";
			L.Right = "\xE2\x88\x92" + Money(ComboOff);
			L.Red = true;
			R.Lines.push_back(L);
		}
		if (!R.Lines.empty())
		{
			R.Sums.push_back({"time", "IN THE CHAIR", "", MinutesText(Minutes)});
			R.Sums.push_back({"subtotal", "SUBTOTAL", "", Money(Subtotal())});
			if (Tipping)
			{
				const int Pct = TipPercent();
				R.Sums.push_back({"tip", Pct == 0 ? "NO TIP" : "TIP " + std::to_string(Pct) + "%", "", Money(TipAmount(Pct))});
			}
			R.Total = {"total", "TOTAL", "", Money(Total())};
		}
		R.Foot = {"SERVICES ARE NOT TAXED", "CARD BALANCE " + Money(S.BankrollCents)};
		return R;
	}

	Button Main(double /*Now*/) override
	{
		Button B;
		if (Picks.empty())
		{
			B.Label = "PICK A STYLE";
			return B;
		}
		B.Enabled = true;
		if (!Tipping)
		{
			int Minutes = 0;
			for (const shop::Offer* O : Ordered())
			{
				Minutes += O->Minutes;
			}
			B.Label = "CONFIRM \xC2\xB7 " + MinutesText(Minutes);
			return B;
		}
		const Chips Due = Total();
		B.Warn = Due > S.BankrollCents;
		B.Label = B.Warn ? "CARD WON'T COVER IT" : "PAY " + Money(Due);
		return B;
	}

	void DoMain(double Now) override
	{
		if (Picks.empty())
		{
			Say("browse", Now);
			return;
		}
		if (!Tipping)
		{
			Tipping = true;
			return;
		}
		std::vector<std::string> Ids;
		for (const shop::Offer* O : Ordered())
		{
			Ids.push_back(O->Id);
		}
		const Chips Due = Total();
		const std::string Why = S.ShopService(Order("barber", Ids, TipPercent()));
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
		Stamp(StampKind::Paid, Now);
		if (!Started(Now))
		{
			// Charged with no chair to sit in (a skip couldn't start): the ticket stays paid, the book clears.
			Picks.clear();
			Tipping = false;
			Say(TipPercent() > 0 ? "tip" : "stiff", Now);
			Refresh();
		}
	}

private:
	std::vector<const shop::Offer*> OnTab() const
	{
		std::vector<const shop::Offer*> Out;
		for (const shop::Offer& O : shop::Offers("barber"))
		{
			if (O.Tab == BarberTabs[Tab])
			{
				Out.push_back(&O);
			}
		}
		return Out;
	}

	/** The picks in the book's order (cut, beard, color, brows). */
	std::vector<const shop::Offer*> Ordered() const
	{
		std::vector<const shop::Offer*> Out;
		for (const char* T : BarberTabs)
		{
			const auto It = Picks.find(T);
			if (It != Picks.end())
			{
				if (const shop::Offer* O = shop::FindOffer(It->second))
				{
					Out.push_back(O);
				}
			}
		}
		return Out;
	}

	bool Combo() const { return Picks.count("CUT") > 0 && Picks.count("BEARD") > 0; }

	Chips Subtotal() const
	{
		Chips Sum = 0;
		Chips TaxedPart = 0;
		for (const shop::Offer* O : Ordered())
		{
			Sum += O->PriceCents;
			TaxedPart += O->Taxed ? O->PriceCents : 0;
		}
		return Sum - (Combo() ? ComboOff : 0) + store::Tax(TaxedPart);
	}

	/** What the card is charged: the Session's sum when it has one (it's what Pay charges), else the ticket's. */
	Chips Total()
	{
		std::vector<std::string> Ids;
		for (const shop::Offer* O : Ordered())
		{
			Ids.push_back(O->Id);
		}
		const Chips Core = S.ShopTotal(Order("barber", Ids, Tipping ? TipPercent() : 0));
		return Core > 0 ? Core : Subtotal() + (Tipping ? TipAmount(TipPercent()) : 0);
	}

	/** The draft: the hero's look with every pick, and no hat (it comes off in the chair). */
	hero::Look Draft() const
	{
		hero::Look L = Dressed(S.Person.Appearance, S.Life.Closet);
		L.Hat = 0;
		for (const shop::Offer* O : Ordered())
		{
			Apply(*O, L);
		}
		return L;
	}

	static void Apply(const shop::Offer& O, hero::Look& L)
	{
		if (O.LookSlot >= 0 && O.LookSlot < hero::SlotCount && O.LookValue >= 0)
		{
			L.Set(static_cast<hero::Slot>(O.LookSlot), O.LookValue);
		}
	}

	struct Standing
	{
		bool Current = false;
		bool Picked = false;
		bool PortraitOnly = false;
		bool TooLong = false;
		std::string Why; // "" when it can be had
	};

	Standing StateOf(const shop::Offer& O) const
	{
		Standing St;
		const hero::Look& L = S.Person.Appearance;
		const double World = S.WorldMinutes();
		const auto It = Picks.find(O.Tab);
		St.Picked = It != Picks.end() && It->second == O.Id;
		if (O.LookSlot >= 0 && O.LookSlot < hero::SlotCount && O.LookValue >= 0)
		{
			const hero::Slot Slot = static_cast<hero::Slot>(O.LookSlot);
			St.Current = L.Get(Slot) == O.LookValue;
			if (Slot == hero::Slot::Hair)
			{
				St.PortraitOnly = !Has(Caps.HairStyles, O.LookValue);
				St.TooLong = body::HairRank(O.LookValue) > body::HairAllowed(S.Life.Groom, L, World);
			}
			else if (Slot == hero::Slot::FacialHair)
			{
				St.PortraitOnly = !Has(Caps.FacialHair, O.LookValue);
				St.TooLong = body::BeardRank(O.LookValue) > body::BeardAllowed(S.Life.Groom, L, World);
			}
			else if (Slot == hero::Slot::Brows)
			{
				St.PortraitOnly = !Has(Caps.Brows, O.LookValue);
			}
		}
		if (St.PortraitOnly)
		{
			St.Why = "PORTRAIT ONLY";
		}
		else if (St.TooLong)
		{
			St.Why = O.LookSlot == static_cast<int>(hero::Slot::FacialHair) ? "Give it a few days." : "Give it three weeks.";
		}
		else
		{
			St.Why = S.ShopBlocked(O.Id);
		}
		return St;
	}

	void Pick(const shop::Offer& O, double Now)
	{
		const Standing St = StateOf(O);
		if (!St.Why.empty() && !St.Picked)
		{
			if (St.TooLong)
			{
				Say("grow", Now, true);
			}
			else if (!St.PortraitOnly)
			{
				SayText(St.Why, Now, true);
			}
			return;
		}
		Tipping = false;
		if (St.Picked)
		{
			Picks.erase(O.Tab);
		}
		else
		{
			Picks[O.Tab] = O.Id;
			Say(O.Tab == "COLOR" ? "pick:color" : "pick", Now);
		}
		Touch(O.Id, Now);
		Refresh();
	}

	/** The card in focus on the current tab when the tab opens: the hero's own style. */
	void Focus(double Now)
	{
		const std::vector<const shop::Offer*> On = OnTab();
		Sel = 0;
		for (size_t I = 0; I < On.size(); ++I)
		{
			const auto It = Picks.find(BarberTabs[Tab]);
			if ((It != Picks.end() && It->second == On[I]->Id) || (It == Picks.end() && StateOf(*On[I]).Current))
			{
				Sel = static_cast<int>(I);
				break;
			}
		}
		SelAt = Now;
	}

	/** What the mirror shows: the draft with the choice in focus, when the 3D can show it. */
	hero::Look Previewed() const
	{
		hero::Look L = Draft();
		const std::vector<const shop::Offer*> On = OnTab();
		if (Sel >= 0 && Sel < static_cast<int>(On.size()))
		{
			const Standing St = StateOf(*On[static_cast<size_t>(Sel)]);
			if (!St.TooLong && (!St.PortraitOnly || !Caps.Preview3D))
			{
				Apply(*On[static_cast<size_t>(Sel)], L);
			}
		}
		return L;
	}

	std::string PreviewName() const
	{
		const std::vector<const shop::Offer*> On = OnTab();
		if (Sel >= 0 && Sel < static_cast<int>(On.size()))
		{
			const Standing St = StateOf(*On[static_cast<size_t>(Sel)]);
			if (St.TooLong)
			{
				return "IN THREE WEEKS: " + Upper(On[static_cast<size_t>(Sel)]->Name);
			}
			if (St.PortraitOnly && Caps.Preview3D)
			{
				return "ON PAPER ONLY: " + Upper(On[static_cast<size_t>(Sel)]->Name);
			}
			return (St.Picked ? "ON THE TICKET: " : "TRYING: ") + Upper(On[static_cast<size_t>(Sel)]->Name);
		}
		return std::string();
	}

	/** Shows the mirror the look in focus (once per change). */
	void Refresh()
	{
		if (!IsOpen())
		{
			return;
		}
		const hero::Look L = Previewed();
		wardrobe::Closet Bare = S.Life.Closet;
		Bare.Worn[static_cast<size_t>(wardrobe::Slot::Hat)].clear();
		std::string Key;
		for (int I = 0; I < hero::SlotCount; ++I)
		{
			Key += std::to_string(L.Get(static_cast<hero::Slot>(I))) + ",";
		}
		if (Key != ShownKey)
		{
			ShownKey = Key;
			Preview(L, Bare);
		}
	}

	void Card(Canvas& C, const shop::Offer& O, const Rect& R, bool Focus, bool Over, double Now)
	{
		const Standing St = StateOf(O);
		const bool Blocked = !St.Why.empty() && !St.Picked;
		CardBack(C, R, Focus, Over, Blocked, Now);
		// The portrait: the hero with this choice (and the other picks), still.
		hero::Look L = Draft();
		Apply(O, L);
		hero::Character Sitter = S.Person;
		Sitter.Appearance = L;
		const Rect Win{R.X + 8.0f, R.Y + 8.0f, R.W - 16.0f, R.H - 74.0f};
		C.FillRoundRect(Win, 4.0f, Paint::Radial({Win.X + Win.W * 0.5f, Win.Y + Win.H * 0.35f}, 0.0f, {Win.X + Win.W * 0.5f, Win.Y + Win.H * 0.5f}, Win.W * 0.7f, Hex(0x2a3442), 0.6f,
									   Hex(0x161d27), Hex(0x0b0f15)));
		const float A0 = C.GetAlpha();
		if (Blocked)
		{
			C.SetAlpha(A0 * 0.45f);
		}
		C.PushClip(Win);
		// The brows' page looks closer; the others show the head and the shoulders.
		const float Scale = Tab == 3 ? 0.62f : 0.42f;
		const float EyeY = Tab == 3 ? Win.Y + Win.H * 0.55f : Tab == 1 ? Win.Y + Win.H * 0.4f : Win.Y + Win.H * 0.5f;
		uint64_t Key = Hash("barber-thumb") ^ static_cast<uint64_t>(Sitter.Age) * 0x9e3779b97f4a7c15ull;
		for (int I = 0; I < hero::SlotCount; ++I)
		{
			Key = Key * 1099511628211ull + static_cast<uint64_t>(L.Get(static_cast<hero::Slot>(I)) + 7);
		}
		Key = Key * 31ull + static_cast<uint64_t>(Sitter.Story);
		C.Cached(Key ^ static_cast<uint64_t>(Tab), Win.X + Win.W * 0.5f, EyeY, [&](Canvas& Cv) { DrawPortrait(Cv, Sitter, 0.0f, 0.0f, Scale, 0.0); });
		C.PopClip();
		C.SetAlpha(A0);
		if (O.LookSlot == static_cast<int>(hero::Slot::HairColor) && O.LookValue >= 0)
		{
			// The color's swatch, a lock of it.
			const Color Lock = Hex(hero::HairTone(O.LookValue));
			C.FillCircle(Win.X + Win.W - 20.0f, Win.Y + 20.0f, 11.0f, Paint::Linear({Win.X + Win.W - 31.0f, Win.Y + 9.0f}, {Win.X + Win.W - 9.0f, Win.Y + 31.0f}, Shade(Lock, 0.2f), Shade(Lock, -0.3f)));
			C.StrokeEllipse(Win.X + Win.W - 20.0f, Win.Y + 20.0f, 11.0f, 11.0f, Rgba(255, 255, 255, 0.4f), 1.2f);
		}
		// The name, the price and the minutes.
		const float Ty = R.Y + R.H - 40.0f;
		const std::string Pr = Price(O.PriceCents);
		const float Pw = C.Measure(Pr, 18.0f, 900);
		C.Text(O.Name, R.X + 14.0f, Ty, Ts(17.0f, 800, Blocked ? MenuMuted : MenuInk, Align::Left, Baseline::Alphabetic, false, R.W - 40.0f - Pw));
		C.Text(Pr, R.X + R.W - 14.0f, Ty, Ts(18.0f, 900, Blocked ? MenuMuted : Accent("barber"), Align::Right));
		std::string Sub = MinutesText(O.Minutes);
		if (!O.Blurb.empty() && Focus)
		{
			Sub += " \xC2\xB7 " + O.Blurb;
		}
		C.Text(Sub, R.X + 14.0f, Ty + 20.0f, Ts(12.0f, 600, MenuMuted, Align::Left, Baseline::Alphabetic, false, R.W - 28.0f));
		// The badges, top left of the portrait.
		float Bx = Win.X + 8.0f;
		if (St.Picked)
		{
			Bx += Badge(C, "ON THE TICKET", Bx, Win.Y + 8.0f, Mint, true) + 6.0f;
		}
		else if (St.Current)
		{
			Bx += Badge(C, "YOURS NOW", Bx, Win.Y + 8.0f, MenuInk) + 6.0f;
		}
		if (Blocked)
		{
			const std::string Why = BadgeText(C, St.Why, Win.W - 16.0f, "NOT TODAY");
			Badge(C, Why, Win.X + 8.0f, Win.Y + Win.H - 28.0f, St.TooLong ? MenuGold : MenuWarn);
		}
	}

	static constexpr Chips ComboOff = 500; // "Cut & beard" together: $5 off (§5.1)
	std::map<std::string, std::string> Picks; // tab -> offer id
	bool Reveal = false;
	int Cols = 3;
	std::string ShownKey;
};
} // namespace

std::unique_ptr<ShopScreen> MakeBarber(Session& S)
{
	return std::make_unique<BarberPage>(S);
}
} // namespace shopframe
} // namespace ui
} // namespace ss
