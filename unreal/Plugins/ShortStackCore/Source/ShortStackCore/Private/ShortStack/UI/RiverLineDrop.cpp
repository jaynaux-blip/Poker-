// Penny Drop: the Lucky Penny #212's delivery app on the laptop. The store's shelves at app prices (15% over the
// counter, a flat fee), a cart that shows what the same food costs if you walk down the block, the orders on their
// way, and "at home": hunger, thirst and energy, what's in the bag to eat or drink, and the kitchen tap.
#include "ShortStack/UI/RiverLine.h"
#include "../StrictFloat.h"
#include "RiverLineShared.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/UI/StoreCounter.h"

#include <algorithm>
#include <cmath>

namespace ss
{
namespace ui
{
using namespace rlnet_detail;

namespace rldrop_detail
{
const float DropH = 962.0f; // above the taskbar
const Color DropInk = Hex(0x1f1a17);
const Color DropGray = Hex(0x7a7068);
const Color DropRule = Hex(0xeadfd2);
const Color DropRed = Hex(0xd7263d);
const Color DropRed2 = Hex(0xa3122a);
const Color DropCream = Hex(0xfbf6ee);
const Color DropGreen = Hex(0x16a34a);

/** "+45 thirst", "+18 energy", "-8 thirst": what an item does, for its card. */
std::vector<std::pair<std::string, Color>> Effects(const store::Item& I, double MealBoost)
{
	std::vector<std::pair<std::string, Color>> Out;
	const double Boost = I.Food() ? MealBoost : 1.0;
	auto Add = [&Out](double V, const char* What, uint32_t Col) {
		if (std::fabs(V) >= 1.0)
		{
			const int N = static_cast<int>(std::lround(V));
			Out.emplace_back((N > 0 ? "+" : "\xE2\x88\x92") + std::to_string(std::abs(N)) + " " + What, N > 0 ? Hex(Col) : Hex(0xb4553c));
		}
	};
	Add(I.Hunger * Boost, "food", 0xd97706);
	Add(I.Thirst, "drink", 0x2b7fc4);
	Add(I.Energy * Boost, "energy", 0x9a7b0c);
	return Out;
}

/** A small scooter for the tracker. */
void Scooter(Canvas& Cv, float X, float Y, float Sz, const Color& Col)
{
	Cv.FillCircle(X - Sz * 0.38f, Y + Sz * 0.22f, Sz * 0.16f, Col);
	Cv.FillCircle(X + Sz * 0.38f, Y + Sz * 0.22f, Sz * 0.16f, Col);
	Cv.FillPolygon({{X - Sz * 0.5f, Y + Sz * 0.12f}, {X + Sz * 0.1f, Y + Sz * 0.12f}, {X + Sz * 0.28f, Y - Sz * 0.3f}, {X + Sz * 0.4f, Y - Sz * 0.3f}, {X + Sz * 0.26f, Y + Sz * 0.16f}, {X - Sz * 0.5f, Y + Sz * 0.2f}}, Col);
	Cv.FillRoundRect({X - Sz * 0.5f, Y - Sz * 0.22f, Sz * 0.42f, Sz * 0.3f}, Sz * 0.06f, Col);
}
} // namespace rldrop_detail

using namespace rldrop_detail;

void RiverLine::PennyDropApp(double Now)
{
	const life::State& L = S.Life;
	const float In = NetEase((Now - AppAt) / 0.5);
	C->FillRect({0.0f, 0.0f, NetW, DropH}, DropCream);

	// Header.
	C->FillRect({0.0f, 0.0f, NetW, 72.0f}, Paint::Linear({0.0f, 0.0f}, {NetW, 0.0f}, DropRed, DropRed2));
	AppIcon(App::PennyDrop, 28.0f, 16.0f, 40.0f);
	const float Lw = UI.Text("Penny Drop", 80.0f, 45.0f, Ts(26.0f, 900, Hex(0xffffff)));
	UI.Text("from the Lucky Penny #212 \xC2\xB7 open 24 hours", 92.0f + Lw, 45.0f, Ts(15.0f, 500, Hex(0xffd7dc)));
	UI.Text("Delivering to 1812 Fifth St, Apt 3B", NetW - 32.0f, 32.0f, Ts(13.0f, 600, Hex(0xffd7dc), Align::Right));
	UI.Text(Money(S.BankrollCents) + " on your card", NetW - 32.0f, 54.0f, Ts(17.0f, 800, Hex(0xffffff), Align::Right, Baseline::Alphabetic, true));

	// Shelf tabs.
	float Tx = 40.0f;
	for (int K = 0; K < store::ShelfCount; ++K)
	{
		const std::string Name = store::ShelfName(static_cast<store::Shelf>(K));
		const float W = UI.Measure(Name, 14.0f, 800) + 36.0f;
		const Rect Tab{Tx, 96.0f, W, 38.0f};
		const Ui::ClickState St = UI.Clickable("dropshelf" + std::to_string(K), Tab);
		if (St.Clicked)
		{
			DropShelf = K;
		}
		const bool On = DropShelf == K;
		UI.RRect(Tab, 19.0f, On ? Paint(DropInk) : Paint(St.Hover ? Hex(0xf1e6d8) : Hex(0xffffff)), On ? DropInk : DropRule, 1.0f);
		UI.Text(Name, Tab.X + W / 2.0f, Tab.Y + 25.0f, Ts(14.0f, 800, On ? Hex(0xffffff) : DropInk, Align::Center));
		Tx += W + 10.0f;
	}
	UI.Text("App prices run 15% over the counter, plus " + Money(store::DeliveryFeeCents) + " delivery.", 980.0f, 121.0f, Ts(13.0f, 500, DropGray, Align::Right));

	// Products on the shelf.
	std::vector<const store::Item*> Shelf;
	for (const store::Item& I : store::Catalog())
	{
		if (static_cast<int>(I.Where) == DropShelf)
		{
			Shelf.push_back(&I);
		}
	}
	for (size_t K = 0; K < Shelf.size(); ++K)
	{
		const store::Item& I = *Shelf[K];
		const float Ci = NetEase((Now - AppAt - 0.05 * static_cast<double>(K)) / 0.4);
		const Rect R{40.0f + Nf(K % 3) * 316.0f, 152.0f + Nf(K / 3) * 272.0f + (1.0f - Ci) * 18.0f, 300.0f, 256.0f};
		const float A0 = C->GetAlpha();
		C->SetAlpha(A0 * Ci);
		UI.RRect(R, 16.0f, Hex(0xffffff), DropRule, 1.0f);
		C->FillRoundRect({R.X + 14.0f, R.Y + 14.0f, 112.0f, 132.0f}, 12.0f, Hex(0xf6efe4));
		DrawProduct(*C, I, R.X + 70.0f, R.Y + 82.0f, 104.0f);
		UI.Text(I.Name, R.X + 140.0f, R.Y + 40.0f, Ts(19.0f, 900, DropInk, Align::Left, Baseline::Alphabetic, false, R.W - 152.0f));
		UI.Text(I.Kind, R.X + 140.0f, R.Y + 62.0f, Ts(12.5f, 600, DropGray, Align::Left, Baseline::Alphabetic, false, R.W - 152.0f));
		float Ey = R.Y + 78.0f;
		for (const auto& E : Effects(I, L.Perks.MealBoost))
		{
			const float Ew = UI.Measure(E.first, 12.0f, 800) + 16.0f;
			UI.RRect({R.X + 140.0f, Ey, Ew, 22.0f}, 11.0f, NetA(E.second, 0.12f));
			UI.Text(E.first, R.X + 148.0f, Ey + 15.5f, Ts(12.0f, 800, E.second));
			Ey += 26.0f;
		}
		NetParagraph(*C, I.Blurb, R.X + 16.0f, R.Y + 176.0f, R.W - 32.0f, 13.0f, 500, DropGray, 17.0f, 2);
		UI.Text(Money(store::AppPrice(I)), R.X + 16.0f, R.Y + 234.0f, Ts(20.0f, 900, DropInk, Align::Left, Baseline::Alphabetic, true));
		const float Pw = UI.Measure(Money(store::AppPrice(I)), 20.0f, 900, true);
		UI.Text(Money(I.PriceCents) + " in store", R.X + 24.0f + Pw, R.Y + 233.0f, Ts(12.0f, 600, DropGray));
		int InCart = 0;
		for (const auto& Ln : DropCart.Lines)
		{
			InCart += Ln.first == I.Id ? Ln.second : 0;
		}
		if (AppButton("dropadd" + I.Id, {R.X + R.W - 112.0f, R.Y + 206.0f, 96.0f, 38.0f}, InCart > 0 ? "Add (" + std::to_string(InCart) + ")" : "Add", DropRed, Hex(0xffffff), true))
		{
			DropCart.Add(I.Id);
		}
		C->SetAlpha(A0);
	}

	// The cart.
	const Rect Cart{1020.0f, 96.0f, 540.0f, 466.0f};
	UI.RRect(Cart, 18.0f, Hex(0xffffff), DropRule, 1.0f);
	UI.Text("Your order", Cart.X + 28.0f, Cart.Y + 44.0f, Ts(22.0f, 900, DropInk));
	if (DropCart.Empty())
	{
		UI.Text("Nothing yet. Pick something from the shelves.", Cart.X + 28.0f, Cart.Y + 80.0f, Ts(14.0f, 500, DropGray));
	}
	float Ly = Cart.Y + 72.0f;
	std::string Remove;
	std::string More;
	for (const auto& Ln : DropCart.Lines)
	{
		const store::Item* I = store::Find(Ln.first);
		if (!I || Ly > Cart.Y + 250.0f)
		{
			continue;
		}
		DrawProduct(*C, *I, Cart.X + 46.0f, Ly + 20.0f, 34.0f);
		UI.Text(I->Name, Cart.X + 76.0f, Ly + 26.0f, Ts(15.0f, 800, DropInk));
		const Rect Minus{Cart.X + 300.0f, Ly + 6.0f, 30.0f, 30.0f};
		const Rect Plus{Cart.X + 374.0f, Ly + 6.0f, 30.0f, 30.0f};
		for (int Side = 0; Side < 2; ++Side)
		{
			const bool IsPlus = Side == 1;
			const Rect B = IsPlus ? Plus : Minus;
			const Ui::ClickState St = UI.Clickable((IsPlus ? "dropmore" : "dropless") + I->Id, B);
			UI.RRect(B, 15.0f, St.Hover ? Hex(0xf1e6d8) : Hex(0xf8f2ea), DropRule, 1.0f);
			UI.Text(IsPlus ? "+" : "\xE2\x88\x92", B.X + 15.0f, B.Y + 21.0f, Ts(17.0f, 900, DropInk, Align::Center));
			if (St.Clicked)
			{
				(IsPlus ? More : Remove) = I->Id;
			}
		}
		UI.Text(std::to_string(Ln.second), Cart.X + 352.0f, Ly + 27.0f, Ts(16.0f, 900, DropInk, Align::Center, Baseline::Alphabetic, true));
		UI.Text(Money(store::AppPrice(*I) * Ln.second), Cart.X + Cart.W - 28.0f, Ly + 27.0f, Ts(15.0f, 800, DropInk, Align::Right, Baseline::Alphabetic, true));
		Ly += 42.0f;
	}
	if (!Remove.empty())
	{
		DropCart.Remove(Remove);
	}
	if (!More.empty())
	{
		DropCart.Add(More);
	}
	const Chips Goods = DropCart.AppSubtotal();
	const Chips Total = DropCart.Empty() ? 0 : store::DeliveryTotal(DropCart);
	float Sy = Cart.Y + 300.0f;
	C->FillRect({Cart.X + 28.0f, Sy - 18.0f, Cart.W - 56.0f, 1.0f}, DropRule);
	const std::pair<std::string, Chips> Sums[3] = {{"Items", Goods}, {"Delivery", DropCart.Empty() ? 0 : store::DeliveryFeeCents}, {"Tax", DropCart.Empty() ? 0 : Total - Goods - store::DeliveryFeeCents}};
	for (const auto& Sm : Sums)
	{
		UI.Text(Sm.first, Cart.X + 28.0f, Sy, Ts(14.0f, 600, DropGray));
		UI.Text(Money(Sm.second), Cart.X + Cart.W - 28.0f, Sy, Ts(14.0f, 700, DropInk, Align::Right, Baseline::Alphabetic, true));
		Sy += 24.0f;
	}
	UI.Text("Total", Cart.X + 28.0f, Sy + 10.0f, Ts(18.0f, 900, DropInk));
	UI.Text(Money(Total), Cart.X + Cart.W - 28.0f, Sy + 10.0f, Ts(22.0f, 900, DropInk, Align::Right, Baseline::Alphabetic, true));
	// Why not (the same checks PlaceOrder makes, shown before the click).
	std::string Why;
	if (DropCart.Empty())
	{
		Why = "Add something first";
	}
	else if (Goods < store::DeliveryMinimumCents)
	{
		Why = "Delivery starts at " + Money(store::DeliveryMinimumCents);
	}
	else if (L.Deliveries.size() >= 3)
	{
		Why = "Three orders already on the way";
	}
	else if (Total > S.BankrollCents)
	{
		Why = "Short " + Money(Total - S.BankrollCents);
	}
	if (AppButton("dropplace", {Cart.X + 28.0f, Cart.Y + Cart.H - 76.0f, Cart.W - 56.0f, 52.0f}, "Place order \xC2\xB7 25\xE2\x80\x93" "45 min", DropRed, Hex(0xffffff), Why.empty(), Why))
	{
		const std::string Fail = S.PlaceOrder(DropCart);
		DropNote = Fail.empty() ? "Ordered. Benny's bagging it now." : Fail;
		DropNoteBad = !Fail.empty();
		DropNoteAt = Now;
		if (Fail.empty())
		{
			DropCart = store::Basket();
		}
	}
	if (!DropCart.Empty())
	{
		// The walk is cheaper: say by how much.
		UI.Text("Walking down the block, the same comes to " + Money(DropCart.Total()) + " at the counter.", Cart.X + 28.0f, Cart.Y + 262.0f, Ts(13.0f, 600, DropGray));
	}

	// On the way.
	const Rect Way{1020.0f, 580.0f, 540.0f, 362.0f};
	UI.RRect(Way, 18.0f, Hex(0xffffff), DropRule, 1.0f);
	UI.Text("On the way", Way.X + 28.0f, Way.Y + 42.0f, Ts(20.0f, 900, DropInk));
	if (L.Deliveries.empty())
	{
		UI.Text(L.Orders > 0 ? "Nothing on the way right now." : "Your first order shows up here, door to door.", Way.X + 28.0f, Way.Y + 76.0f, Ts(14.0f, 500, DropGray));
	}
	float Wy = Way.Y + 70.0f;
	for (size_t K = 0; K < L.Deliveries.size(); ++K)
	{
		const life::State::Delivery& Dv = L.Deliveries[K];
		const double Left = std::max(0.0, Dv.ArriveAt - World);
		const double Span = std::max(1.0, Dv.ArriveAt - Dv.PlacedAt);
		const float Done = Nf(Clamp01(1.0 - Left / Span));
		std::string What;
		for (const auto& Ln : Dv.Lines)
		{
			if (const store::Item* I = store::Find(Ln.first))
			{
				What += (What.empty() ? "" : ", ") + (Ln.second > 1 ? std::to_string(Ln.second) + " " : std::string()) + I->Name;
			}
		}
		UI.Text(What, Way.X + 28.0f, Wy + 14.0f, Ts(14.0f, 700, DropInk, Align::Left, Baseline::Alphabetic, false, Way.W - 220.0f));
		UI.Text("Arrives " + net::TimeLabel(Dv.ArriveAt) + " \xC2\xB7 " + std::to_string(static_cast<int>(std::ceil(Left))) + " min", Way.X + Way.W - 28.0f, Wy + 14.0f, Ts(13.0f, 700, DropRed, Align::Right));
		const Rect Bar{Way.X + 28.0f, Wy + 46.0f, Way.W - 56.0f, 8.0f};
		UI.RRect(Bar, 4.0f, Hex(0xf1e6d8));
		UI.RRect({Bar.X, Bar.Y, std::max(8.0f, Bar.W * Done * In), Bar.H}, 4.0f, Paint::Linear({Bar.X, 0.0f}, {Bar.X + Bar.W, 0.0f}, DropRed, Hex(0xf2c14e)));
		Scooter(*C, Bar.X + Bar.W * Done * In, Bar.Y - 12.0f, 20.0f, DropInk);
		UI.Text("Bagged", Bar.X, Bar.Y + 26.0f, Ts(11.0f, 700, DropGray));
		UI.Text("Your door", Bar.X + Bar.W, Bar.Y + 26.0f, Ts(11.0f, 700, DropGray, Align::Right));
		Wy += 92.0f;
	}
	if (!DropNote.empty() && Now - DropNoteAt < 4.0)
	{
		const float Na = Nf(Clamp01(4.0 - (Now - DropNoteAt)));
		const Color Nc = DropNoteBad ? DropRed : DropGreen;
		UI.RRect({Way.X + 28.0f, Way.Y + Way.H - 58.0f, Way.W - 56.0f, 38.0f}, 10.0f, NetA(Nc, 0.12f * Na));
		UI.Text(DropNote, Way.X + Way.W / 2.0f, Way.Y + Way.H - 33.0f, Ts(14.0f, 800, NetA(Nc, Na), Align::Center));
	}

	// At home: the needs, the bag, the tap.
	const Rect Home{40.0f, 704.0f, 940.0f, 238.0f};
	C->FillRoundRect(Home, 18.0f, Paint::Linear({Home.X, Home.Y}, {Home.X + Home.W, Home.Y + Home.H}, Hex(0x1d2129), Hex(0x14171d)));
	NetSpaced(*C, "AT HOME", Home.X + 28.0f, Home.Y + 38.0f, 12.0f, 900, Hex(0xf2c14e), 2.4f);
	DrawVitals(*C, L, Home.X + 28.0f, Home.Y + 58.0f, 400.0f, Now);
	// The tap.
	const double Wait = S.TapWait();
	const bool Dry = L.Thirst >= Session::TapFloor + 1.0;
	const std::string TapWhy = !Dry ? "Not thirsty" : Wait > 0.0 ? "Again in " + std::to_string(static_cast<int>(std::ceil(Wait))) + " min" : std::string();
	if (AppButton("droptap", {Home.X + 28.0f, Home.Y + 170.0f, 400.0f, 46.0f}, "Glass of tap water \xC2\xB7 free", Hex(0x2b7fc4), Hex(0xffffff), TapWhy.empty(), TapWhy))
	{
		const std::string Fail = S.DrinkTapWater();
		DropNote = Fail.empty() ? "Tastes like pipes. Helps a little." : Fail;
		DropNoteBad = !Fail.empty();
		DropNoteAt = Now;
	}
	// The bag.
	UI.Text("In your bag", Home.X + 470.0f, Home.Y + 38.0f, Ts(14.0f, 800, Hex(0xe8e2d6)));
	if (L.Pantry.empty())
	{
		NetParagraph(*C, "Empty. Order something, or walk down to the Lucky Penny (G at the desk).", Home.X + 470.0f, Home.Y + 70.0f, 440.0f, 14.0f, 500, Hex(0x9a9387), 20.0f, 2);
	}
	int Slot = 0;
	std::string Eat;
	for (const auto& Held : L.Pantry)
	{
		const store::Item* I = store::Find(Held.first);
		if (!I || Slot >= 6)
		{
			continue;
		}
		const Rect Cell{Home.X + 470.0f + Nf(Slot % 3) * 150.0f, Home.Y + 54.0f + Nf(Slot / 3) * 90.0f, 140.0f, 80.0f};
		const Ui::ClickState St = UI.Clickable("dropeat" + I->Id, Cell);
		UI.RRect(Cell, 12.0f, St.Hover ? Hex(0x2a303a) : Hex(0x22272f), St.Hover ? Hex(0xf2c14e) : Hex(0x343a44), 1.0f);
		DrawProduct(*C, *I, Cell.X + 32.0f, Cell.Y + 40.0f, 52.0f);
		UI.Text(I->Name, Cell.X + 62.0f, Cell.Y + 30.0f, Ts(13.0f, 800, Hex(0xf4efe6), Align::Left, Baseline::Alphabetic, false, 74.0f));
		UI.Text("\xC3\x97" + std::to_string(Held.second), Cell.X + 62.0f, Cell.Y + 48.0f, Ts(12.0f, 700, Hex(0x9a9387)));
		UI.Text(I->Thirst > I->Hunger ? "Drink" : "Eat", Cell.X + 62.0f, Cell.Y + 68.0f, Ts(12.0f, 900, St.Hover ? Hex(0xf2c14e) : Hex(0xd8b25a)));
		if (St.Clicked)
		{
			Eat = I->Id;
		}
		++Slot;
	}
	if (!Eat.empty())
	{
		const store::Item* I = store::Find(Eat);
		const std::string Fail = S.Consume(Eat);
		DropNote = Fail.empty() && I ? (I->Thirst > I->Hunger ? "Drank the " : "Ate the ") + I->Name + "." : Fail;
		DropNoteBad = !Fail.empty();
		DropNoteAt = Now;
	}
}
} // namespace ui
} // namespace ss
