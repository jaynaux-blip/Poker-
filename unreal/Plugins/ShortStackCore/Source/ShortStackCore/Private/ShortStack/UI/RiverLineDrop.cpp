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
#include <iterator>
#include <map>
#include <string>
#include <vector>

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
const Color DropGold = Hex(0xf2c14e);
/** Basket::Add stops a line at nine. */
const int DropMaxEach = 9;
/** Cart lines in view before the list scrolls. */
const int DropCartRows = 5;
const float DropCartRowH = 40.0f;
/** The bag's cells per page (three across, two down). */
const int DropBagPer = 6;
/** How long something new in the bag wears its tag (seconds). */
const double DropNewFor = 60.0;
/** Where on the tracker the courier picks up the order. */
const float DropPickup = 0.3f;

/** "+45 drink", "+18 energy", "-8 drink": what an item does, for its card (store::ReliefOf: what eating it applies). */
std::vector<std::pair<std::string, Color>> Effects(const store::Item& I, double MealBoost)
{
	std::vector<std::pair<std::string, Color>> Out;
	const store::Relief Rl = store::ReliefOf(I, MealBoost);
	auto Add = [&Out](double V, const char* What, uint32_t Col) {
		if (std::fabs(V) >= 1.0)
		{
			const int N = static_cast<int>(std::lround(V));
			Out.emplace_back((N > 0 ? "+" : "\xE2\x88\x92") + std::to_string(std::abs(N)) + " " + What, N > 0 ? Hex(Col) : Hex(0xb4553c));
		}
	};
	Add(Rl.Hunger, "food", 0xd97706);
	Add(Rl.Thirst, "drink", 0x2b7fc4);
	Add(Rl.Energy, "energy", 0x9a7b0c);
	return Out;
}

/**
 * Whether eating or drinking it now would mostly go to waste: less than a third of what it does would land (fed and
 * watered already). Items that only cost thirst or energy never count as wasted.
 */
bool Wasted(const store::Item& I, const life::State& L)
{
	const store::Relief Rl = store::ReliefOf(I, L.Perks.MealBoost);
	const double Hunger = std::max(0.0, Rl.Hunger);
	const double Thirst = std::max(0.0, Rl.Thirst);
	const double Energy = std::max(0.0, Rl.Energy);
	const double Full = Hunger + Thirst + Energy;
	const double Lands = std::min(Hunger, L.Hunger) + std::min(Thirst, L.Thirst) + std::min(Energy, 100.0 - L.Energy);
	return Full > 0.0 && Lands < Full / 3.0;
}

/** The courier's extra time in the dead hours (2 to 6 AM): Session::PlaceOrder adds the same. */
int NightExtra(double World)
{
	const double Hour = std::fmod(std::fmod(World, net::MinutesPerDay) + net::MinutesPerDay, net::MinutesPerDay) / 60.0;
	return Hour >= 2.0 && Hour < 6.0 ? 8 : 0;
}

/** A small scooter for the tracker. */
void Scooter(Canvas& Cv, float X, float Y, float Sz, const Color& Col)
{
	Cv.FillCircle(X - Sz * 0.38f, Y + Sz * 0.22f, Sz * 0.16f, Col);
	Cv.FillCircle(X + Sz * 0.38f, Y + Sz * 0.22f, Sz * 0.16f, Col);
	Cv.FillPolygon({{X - Sz * 0.5f, Y + Sz * 0.12f}, {X + Sz * 0.1f, Y + Sz * 0.12f}, {X + Sz * 0.28f, Y - Sz * 0.3f}, {X + Sz * 0.4f, Y - Sz * 0.3f}, {X + Sz * 0.26f, Y + Sz * 0.16f}, {X - Sz * 0.5f, Y + Sz * 0.2f}}, Col);
	Cv.FillRoundRect({X - Sz * 0.5f, Y - Sz * 0.22f, Sz * 0.42f, Sz * 0.3f}, Sz * 0.06f, Col);
}

/** A cell's top-left in the bag grid, for a slot that may be fractional (a cell sliding to its place). */
Vec2 BagCellAt(const Rect& Grid, float Slot)
{
	auto At = [&Grid](int K) {
		const int Col = ((K % 3) + 3) % 3;
		const int Row = (K - Col) / 3;
		return Vec2{Grid.X + Nf(Col) * 150.0f, Grid.Y + Nf(Row) * 90.0f};
	};
	const float F = std::floor(Slot);
	const int K = static_cast<int>(F);
	const Vec2 A = At(K);
	const Vec2 B = At(K + 1);
	const float T = Slot - F;
	return {A.X + (B.X - A.X) * T, A.Y + (B.Y - A.Y) * T};
}

/** A round arrow button (paging, scrolling); Dir as NetChevron takes it: 0 right, 1 left, 2 up, 3 down. */
bool RoundArrow(Ui& UI, Canvas& Cv, const std::string& Id, float Cx, float Cy, int Dir, bool Enabled, bool Dark)
{
	const Rect R{Cx - 14.0f, Cy - 14.0f, 28.0f, 28.0f};
	const Ui::ClickState St = UI.Clickable(Id, R, Enabled);
	const Color Fill = Dark ? (St.Hover ? Hex(0x343a44) : Hex(0x262b33)) : (St.Hover ? Hex(0xf1e6d8) : Hex(0xf8f2ea));
	const Color Edge = Dark ? Hex(0x3d4450) : DropRule;
	const Color Ink = Dark ? Hex(0xf4efe6) : DropInk;
	Cv.FillCircle(Cx, Cy, 14.0f, Fill);
	Cv.StrokeEllipse(Cx, Cy, 14.0f, 14.0f, Edge, 1.0f);
	NetChevron(Cv, Cx + (Dir == 0 ? 1.0f : Dir == 1 ? -1.0f : 0.0f), Cy + (Dir == 3 ? 1.0f : Dir == 2 ? -1.0f : 0.0f), 11.0f, Dir, NetA(Ink, Enabled ? 1.0f : 0.3f));
	return St.Clicked;
}

/** Where the cart's list starts so Id's line is in view (past DropCartRows lines it shows one fewer, and scrolls). */
int CartScrollTo(const store::Basket& Cart, const std::string& Id, int Scroll)
{
	const int N = static_cast<int>(Cart.Lines.size());
	if (N <= DropCartRows)
	{
		return 0;
	}
	const int Shown = DropCartRows - 1;
	for (int At = 0; At < N; ++At)
	{
		if (Cart.Lines[static_cast<size_t>(At)].first == Id)
		{
			return At < Scroll ? At : At >= Scroll + Shown ? At - Shown + 1 : Scroll;
		}
	}
	return Scroll;
}

/** A bag cell's name: a size down when it wouldn't fit whole ("Sunny Peach"). */
float CellNameSize(const Ui& UI, const std::string& Name)
{
	return UI.Measure(Name, 13.0f, 800) > 74.0f ? 12.0f : 13.0f;
}
} // namespace rldrop_detail

using namespace rldrop_detail;

void RiverLine::DropSay(const std::string& Words, bool Bad, bool Home, double Now)
{
	DropNote = Words;
	DropNoteBad = Bad;
	DropNoteHome = Home;
	DropNoteAt = Now;
}

void RiverLine::PennyDropApp(double Now)
{
	const life::State& L = S.Life;
	C->FillRect({0.0f, 0.0f, NetW, DropH}, DropCream);

	// What arrived since the last frame: an order at the door, or anything new in the bag.
	if (DropBagKnown)
	{
		for (const auto& Held : L.Pantry)
		{
			const auto Was = DropBagSeen.find(Held.first);
			if (Held.second > (Was == DropBagSeen.end() ? 0 : Was->second))
			{
				DropBagNewAt[Held.first] = Now;
			}
		}
		if (L.Deliveries.size() < DropOrdersSeen && Now - AppAt > 0.3)
		{
			DropSay("Knock knock. Your order's in the bag.", false, true, Now);
			DropArrivedAt = Now;
		}
	}
	DropBagSeen = L.Pantry;
	DropOrdersSeen = L.Deliveries.size();
	DropBagKnown = true;
	for (auto It = DropBagNewAt.begin(); It != DropBagNewAt.end();)
	{
		It = Now - It->second > DropNewFor || Now < It->second ? DropBagNewAt.erase(It) : std::next(It);
	}

	// Header.
	C->FillRect({0.0f, 0.0f, NetW, 72.0f}, Paint::Linear({0.0f, 0.0f}, {NetW, 0.0f}, DropRed, DropRed2));
	AppIcon(App::PennyDrop, 28.0f, 16.0f, 40.0f);
	const float Lw = UI.Text("Penny Drop", 80.0f, 45.0f, Ts(26.0f, 900, Hex(0xffffff)));
	UI.Text("from the Lucky Penny #212 \xC2\xB7 open 24 hours", 92.0f + Lw, 45.0f, Ts(15.0f, 500, Hex(0xffd7dc)));
	// Where the courier goes: the apartment, or Dee's while the locks are changed.
	UI.Text("Delivering to " + S.DeliveryAddress(), NetW - 32.0f, 32.0f, Ts(13.0f, 600, Hex(0xffd7dc), Align::Right));
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
		// The cart takes nine of a kind: past that the button says so instead of clicking for nothing.
		const bool Full = InCart >= DropMaxEach;
		if (AppButton("dropadd" + I.Id, {R.X + R.W - 112.0f, R.Y + 206.0f, 96.0f, 38.0f}, Full ? std::to_string(DropMaxEach) + " in cart" : InCart > 0 ? "Add (" + std::to_string(InCart) + ")" : "Add", DropRed,
				Hex(0xffffff), !Full))
		{
			DropCart.Add(I.Id);
			DropCartFlash = I.Id;
			DropCartFlashAt = Now;
			// Bring the line into view.
			DropCartScroll = CartScrollTo(DropCart, I.Id, DropCartScroll);
		}
		C->SetAlpha(A0);
	}

	// The cart.
	const Rect Cart{1020.0f, 96.0f, 540.0f, 466.0f};
	DropCartList(Cart, Now);

	// On the way.
	const Rect Way{1020.0f, 580.0f, 540.0f, 362.0f};
	DropTracker(Way, Now);

	// At home (or at Dee's): the needs, the bag, the tap.
	const Rect Home{40.0f, 704.0f, 940.0f, 238.0f};
	C->FillRoundRect(Home, 18.0f, Paint::Linear({Home.X, Home.Y}, {Home.X + Home.W, Home.Y + Home.H}, Hex(0x1d2129), Hex(0x14171d)));
	// A note about the bag or the tap takes the card label's place for a few seconds (the label fades out under it).
	auto HomeNote = [this, Now]() {
		const double Age = Now - DropNoteAt;
		return DropNoteHome && !DropNote.empty() && Age >= 0.0 && Age < 4.0 ? std::min(NetEase(Age / 0.25), Nf(Clamp01((4.0 - Age) / 0.6))) : 0.0f;
	};
	NetSpaced(*C, S.Evicted() ? "AT DEE'S" : "AT HOME", Home.X + 28.0f, Home.Y + 38.0f, 12.0f, 900, NetA(DropGold, 1.0f - HomeNote()), 2.4f);
	const std::string Hovered = DropBagGrid(Home, Now);
	// The ghost on the bars: what the item under the pointer would do, or else what the order would.
	store::Basket One;
	if (!Hovered.empty())
	{
		One.Add(Hovered);
	}
	DrawVitals(*C, L, Home.X + 28.0f, Home.Y + 58.0f, 400.0f, Now, !One.Empty() ? &One : !DropCart.Empty() ? &DropCart : nullptr);
	// The tap.
	const double Wait = S.TapWait();
	const bool Dry = L.Thirst >= Session::TapFloor + 1.0;
	const std::string TapWhy = !Dry ? "Not thirsty" : Wait > 0.0 ? "Again in " + std::to_string(static_cast<int>(std::ceil(Wait))) + " min" : std::string();
	if (AppButton("droptap", {Home.X + 28.0f, Home.Y + 170.0f, 400.0f, 46.0f}, "Glass of tap water \xC2\xB7 free", Hex(0x2b7fc4), Hex(0xffffff), TapWhy.empty(), TapWhy))
	{
		const std::string Fail = S.DrinkTapWater();
		DropSay(Fail.empty() ? "Tastes like pipes. Helps a little." : Fail, !Fail.empty(), true, Now);
	}
	// The note itself: a pill in the card's header over the vitals column, clear of the bars and the bag (as wide as the
	// vitals, so "Ate the Ridgeline trail mix. Hunger -22, thirst +4, energy +4." fits whole).
	const float Na = HomeNote();
	if (Na > 0.0f)
	{
		const Color Nc = DropNoteBad ? Hex(0xff8a8d) : Hex(0x6ee7a0);
		const float Nw = std::min(436.0f, UI.Measure(DropNote, 12.0f, 700) + 28.0f);
		const Rect Pill{Home.X + 20.0f + (1.0f - Na) * 10.0f, Home.Y + 20.0f, Nw, 26.0f};
		UI.RRect(Pill, 13.0f, NetA(Nc, 0.14f * Na));
		// A little slack past the text's own width: (W + 28) - 28 isn't always W in floats, and a hair short ellipsizes it.
		UI.Text(DropNote, Pill.X + 14.0f, Pill.Y + 17.5f, Ts(12.0f, 700, NetA(Nc, Na), Align::Left, Baseline::Alphabetic, false, Nw - 24.0f));
	}
}

void RiverLine::DropCartList(const Rect& Cart, double Now)
{
	const life::State& L = S.Life;
	UI.RRect(Cart, 18.0f, Hex(0xffffff), DropRule, 1.0f);
	UI.Text("Your order", Cart.X + 28.0f, Cart.Y + 44.0f, Ts(22.0f, 900, DropInk));
	const int Count = DropCart.Count();
	if (Count > 0)
	{
		UI.Text(std::to_string(Count) + (Count == 1 ? " item" : " items"), Cart.X + Cart.W - 28.0f, Cart.Y + 44.0f, Ts(13.0f, 700, DropGray, Align::Right));
	}
	if (DropCart.Empty())
	{
		UI.Text("Nothing yet. Pick something from the shelves.", Cart.X + 28.0f, Cart.Y + 80.0f, Ts(14.0f, 500, DropGray));
	}
	// The last order, one click from the cart again (the same snack run most nights).
	if (DropCart.Empty() && !DropLast.Empty())
	{
		std::string Was;
		for (const auto& Ln : DropLast.Lines)
		{
			if (const store::Item* I = store::Find(Ln.first))
			{
				Was += (Was.empty() ? "" : ", ") + (Ln.second > 1 ? std::to_string(Ln.second) + " " : std::string()) + I->Name;
			}
		}
		const Rect Again{Cart.X + 28.0f, Cart.Y + 100.0f, Cart.W - 56.0f, 56.0f};
		if (AppButton("dropagain", Again, "Same as last time \xC2\xB7 " + Money(store::DeliveryTotal(DropLast)), Hex(0xf8f2ea), DropInk, true, Was))
		{
			DropCart = DropLast;
			DropCartScroll = 0;
			DropCartFlash = DropCart.Lines.front().first;
			DropCartFlashAt = Now;
		}
		C->StrokeRoundRect(Again, 12.0f, DropRule, 1.0f);
	}
	// The lines: five in view; past that, four and a footer that scrolls the rest (the wheel works over the list too).
	const int N = static_cast<int>(DropCart.Lines.size());
	const bool Scrolls = N > DropCartRows;
	const int Shown = Scrolls ? DropCartRows - 1 : N;
	const Rect List{Cart.X + 16.0f, Cart.Y + 62.0f, Cart.W - 32.0f, DropCartRowH * Nf(DropCartRows)};
	if (Scrolls && UI.Ptr.Wheel != 0.0f && UI.Hover(List))
	{
		DropCartScroll += UI.Ptr.Wheel > 0.0f ? 1 : -1;
	}
	DropCartScroll = Scrolls ? std::clamp(DropCartScroll, 0, N - Shown) : 0;
	std::string Remove;
	std::string More;
	for (int K = 0; K < Shown; ++K)
	{
		const std::pair<std::string, int>& Ln = DropCart.Lines[static_cast<size_t>(DropCartScroll + K)];
		const store::Item* I = store::Find(Ln.first);
		if (!I)
		{
			continue;
		}
		const float Ly = List.Y + Nf(K) * DropCartRowH;
		// The line just added (or taken from) lights up for a moment.
		const double Since = Now - DropCartFlashAt;
		if (Ln.first == DropCartFlash && Since >= 0.0 && Since < 0.8)
		{
			UI.RRect({List.X, Ly + 1.0f, List.W, DropCartRowH - 2.0f}, 10.0f, NetA(DropRed, 0.1f * Nf(1.0 - Since / 0.8)));
		}
		DrawProduct(*C, *I, Cart.X + 46.0f, Ly + 20.0f, 32.0f);
		UI.Text(I->Name, Cart.X + 76.0f, Ly + 25.0f, Ts(15.0f, 800, DropInk, Align::Left, Baseline::Alphabetic, false, 214.0f));
		for (int Side = 0; Side < 2; ++Side)
		{
			const bool IsPlus = Side == 1;
			const bool Can = !IsPlus || Ln.second < DropMaxEach;
			const Rect B{Cart.X + (IsPlus ? 374.0f : 300.0f), Ly + 5.0f, 30.0f, 30.0f};
			const Ui::ClickState St = UI.Clickable((IsPlus ? "dropmore" : "dropless") + I->Id, B, Can);
			UI.RRect(B, 15.0f, St.Hover ? Hex(0xf1e6d8) : Hex(0xf8f2ea), DropRule, 1.0f);
			UI.Text(IsPlus ? "+" : "\xE2\x88\x92", B.X + 15.0f, B.Y + 21.0f, Ts(17.0f, 900, NetA(DropInk, Can ? 1.0f : 0.25f), Align::Center));
			if (St.Clicked)
			{
				(IsPlus ? More : Remove) = I->Id;
			}
		}
		UI.Text(std::to_string(Ln.second), Cart.X + 352.0f, Ly + 26.0f, Ts(16.0f, 900, DropInk, Align::Center, Baseline::Alphabetic, true));
		UI.Text(Money(store::AppPrice(*I) * Ln.second), Cart.X + Cart.W - 28.0f, Ly + 26.0f, Ts(15.0f, 800, DropInk, Align::Right, Baseline::Alphabetic, true));
		if (K + 1 < Shown)
		{
			C->FillRect({Cart.X + 76.0f, Ly + DropCartRowH - 0.5f, Cart.W - 104.0f, 1.0f}, NetA(DropRule, 0.7f));
		}
	}
	if (Scrolls)
	{
		// The footer: what's out of view, and the arrows.
		const float Fy = List.Y + Nf(Shown) * DropCartRowH;
		const int Above = DropCartScroll;
		const int Below = N - Shown - DropCartScroll;
		std::string Rest;
		if (Above > 0 && Below > 0)
		{
			Rest = std::to_string(Above) + " above, " + std::to_string(Below) + " below";
		}
		else
		{
			Rest = "+" + std::to_string(Above + Below) + (Above + Below == 1 ? " more line " : " more lines ") + (Above > 0 ? "above" : "below");
		}
		C->FillRect({List.X + 12.0f, Fy + 2.0f, List.W - 24.0f, 1.0f}, DropRule);
		UI.Text(Rest, Cart.X + 28.0f, Fy + 26.0f, Ts(13.0f, 700, DropGray));
		if (RoundArrow(UI, *C, "dropcartup", Cart.X + Cart.W - 76.0f, Fy + 21.0f, 2, Above > 0, false))
		{
			--DropCartScroll;
		}
		if (RoundArrow(UI, *C, "dropcartdown", Cart.X + Cart.W - 40.0f, Fy + 21.0f, 3, Below > 0, false))
		{
			++DropCartScroll;
		}
	}
	if (!Remove.empty())
	{
		DropCart.Remove(Remove);
		DropCartFlash = Remove;
		DropCartFlashAt = Now;
	}
	if (!More.empty())
	{
		DropCart.Add(More);
		DropCartFlash = More;
		DropCartFlashAt = Now;
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
	// The window PlaceOrder promises: 25 to 45 minutes, eight more in the dead hours (one courier on nights).
	const int Extra = NightExtra(World);
	const std::string Label = "Place order \xC2\xB7 " + std::to_string(25 + Extra) + "\xE2\x80\x93" + std::to_string(45 + Extra) + " min";
	// Ready to go, the button's second line says what walking down the block would cost instead.
	const std::string Sub = !Why.empty() ? Why : "Walk down the block and it's " + Money(DropCart.Total()) + " at the counter";
	if (AppButton("dropplace", {Cart.X + 28.0f, Cart.Y + Cart.H - 76.0f, Cart.W - 56.0f, 52.0f}, Label, DropRed, Hex(0xffffff), Why.empty(), Sub))
	{
		const std::string Fail = S.PlaceOrder(DropCart);
		DropSay(Fail.empty() ? "Ordered. Benny's bagging it now." : Fail, !Fail.empty(), false, Now);
		if (Fail.empty())
		{
			DropLast = DropCart;
			DropCart = store::Basket();
			DropCartScroll = 0;
		}
	}
}

void RiverLine::DropTracker(const Rect& Way, double Now)
{
	const life::State& L = S.Life;
	const float In = NetEase((Now - AppAt) / 0.5);
	UI.RRect(Way, 18.0f, Hex(0xffffff), DropRule, 1.0f);
	UI.Text("On the way", Way.X + 28.0f, Way.Y + 42.0f, Ts(20.0f, 900, DropInk));
	// A note about an order: a pill in the card's header, clear of the trackers.
	if (!DropNoteHome && !DropNote.empty() && Now - DropNoteAt < 4.0)
	{
		const float Na = std::min(NetEase((Now - DropNoteAt) / 0.25), Nf(Clamp01((4.0 - (Now - DropNoteAt)) / 0.6)));
		const Color Nc = DropNoteBad ? DropRed : DropGreen;
		const float Nw = std::min(340.0f, UI.Measure(DropNote, 13.0f, 800) + 28.0f);
		const Rect Pill{Way.X + Way.W - 24.0f - Nw + (1.0f - Na) * 12.0f, Way.Y + 22.0f, Nw, 28.0f};
		UI.RRect(Pill, 14.0f, NetA(Nc, 0.12f * Na));
		UI.Text(DropNote, Pill.X + Nw / 2.0f, Pill.Y + 19.0f, Ts(13.0f, 800, NetA(Nc, Na), Align::Center, Baseline::Alphabetic, false, Nw - 16.0f));
	}
	if (L.Deliveries.empty())
	{
		UI.Text(L.Orders > 0 ? "Nothing on the way right now." : "Your first order shows up here, door to door.", Way.X + 28.0f, Way.Y + 76.0f, Ts(14.0f, 500, DropGray));
		return;
	}
	float Wy = Way.Y + 62.0f;
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
		const int Mins = static_cast<int>(std::ceil(Left));
		UI.Text(What, Way.X + 28.0f, Wy + 16.0f, Ts(14.0f, 700, DropInk, Align::Left, Baseline::Alphabetic, false, Way.W - 250.0f));
		UI.Text("Arrives " + net::TimeLabel(Dv.ArriveAt) + " \xC2\xB7 " + (Mins <= 1 ? std::string("any minute") : std::to_string(Mins) + " min"), Way.X + Way.W - 28.0f, Wy + 16.0f,
			Ts(13.0f, 700, DropRed, Align::Right));
		// Where it is, in words: bagged at the counter, on the scooter, around the corner.
		const char* Stage = Done < DropPickup ? "Benny's bagging it" : Done < 0.85f ? "On the scooter" : Done < 1.0f ? "Around the corner" : "At your door";
		UI.Text(Stage, Way.X + 28.0f, Wy + 36.0f, Ts(12.0f, 600, DropGray));
		// The bar, its stops, and the courier riding it.
		const Rect Bar{Way.X + 28.0f, Wy + 52.0f, Way.W - 56.0f, 6.0f};
		const float Px = Bar.X + Bar.W * Done * In;
		UI.RRect(Bar, 3.0f, Hex(0xf1e6d8));
		UI.RRect({Bar.X, Bar.Y, std::max(6.0f, Px - Bar.X), Bar.H}, 3.0f, Paint::Linear({Bar.X, 0.0f}, {Bar.X + Bar.W, 0.0f}, DropRed, DropGold));
		const float Stops[3] = {0.0f, DropPickup, 1.0f};
		const char* StopNames[3] = {"Bagged", "Picked up", "Your door"};
		for (int Si = 0; Si < 3; ++Si)
		{
			const float Sx = Bar.X + Bar.W * Stops[Si];
			const bool Reached = Done >= Stops[Si];
			C->FillCircle(Sx, Bar.Y + 3.0f, 5.0f, Reached ? DropRed : Hex(0xe6d9c8));
			C->FillCircle(Sx, Bar.Y + 3.0f, 2.0f, Hex(0xffffff));
			const Align Al = Si == 0 ? Align::Left : Si == 2 ? Align::Right : Align::Center;
			UI.Text(StopNames[Si], Si == 0 ? Bar.X - 4.0f : Si == 2 ? Bar.X + Bar.W + 4.0f : Sx, Wy + 84.0f, Ts(11.0f, 700, Reached ? DropInk : DropGray, Al));
		}
		// The courier: a scooter on a white puck that rides the bar (between the stage line and the stops, touching neither).
		C->GlowRoundRect({Px - 12.0f, Bar.Y - 9.0f, 24.0f, 24.0f}, 12.0f, Rgba(31, 26, 23, 0.18f), 6.0f);
		C->FillCircle(Px, Bar.Y + 3.0f, 12.0f, Hex(0xffffff));
		C->StrokeEllipse(Px, Bar.Y + 3.0f, 12.0f, 12.0f, NetA(DropRed, 0.6f), 1.5f);
		Scooter(*C, Px + 0.5f, Bar.Y + 2.0f, 14.0f, DropInk);
		Wy += 96.0f;
	}
}

std::string RiverLine::DropBagGrid(const Rect& Home, double Now)
{
	const life::State& L = S.Life;
	const Rect Grid{Home.X + 470.0f, Home.Y + 54.0f, 440.0f, 170.0f};
	const bool Over = UI.Hover({Grid.X - 8.0f, Grid.Y - 8.0f, Grid.W + 16.0f, Grid.H + 16.0f});
	auto Held = [&L](const std::string& Id) {
		const auto It = L.Pantry.find(Id);
		return It == L.Pantry.end() ? 0 : It->second;
	};
	// The cells. Away from the bag: what's in it, shelf by shelf (the catalog's order). Over it: the same cells stay put,
	// one used up keeps its place (empty), and anything that arrives goes on the end.
	if (!Over)
	{
		DropBag.clear();
		for (const store::Item& I : store::Catalog())
		{
			if (Held(I.Id) > 0)
			{
				DropBag.push_back(I.Id);
			}
		}
	}
	else
	{
		for (const store::Item& I : store::Catalog())
		{
			if (Held(I.Id) > 0 && std::find(DropBag.begin(), DropBag.end(), I.Id) == DropBag.end())
			{
				DropBag.push_back(I.Id);
			}
		}
	}
	const int N = static_cast<int>(DropBag.size());
	const int Pages = std::max(1, (N + DropBagPer - 1) / DropBagPer);
	if (Over && UI.Ptr.Wheel != 0.0f && Pages > 1)
	{
		const int Next = std::clamp(DropBagPage + (UI.Ptr.Wheel > 0.0f ? 1 : -1), 0, Pages - 1);
		DropBagPageAt = Next != DropBagPage ? Now : DropBagPageAt;
		DropBagPage = Next;
	}
	DropBagPage = std::clamp(DropBagPage, 0, Pages - 1);

	// The header: the count, a pulse when an order lands, the pages.
	int Things = 0;
	for (const auto& H : L.Pantry)
	{
		Things += std::max(0, H.second);
	}
	const double Landed = Now - DropArrivedAt;
	const float Pulse = Landed >= 0.0 && Landed < 1.2 ? Nf(std::sin(Landed / 1.2 * Pi)) : 0.0f;
	const float Hw = UI.Text("In your bag", Home.X + 470.0f, Home.Y + 38.0f, Ts(14.0f, 800, Mix(Hex(0xe8e2d6), DropGold, Pulse)));
	if (Things > 0)
	{
		UI.Text("\xC2\xB7 " + std::to_string(Things) + (Things == 1 ? " thing" : " things"), Home.X + 478.0f + Hw, Home.Y + 38.0f, Ts(13.0f, 600, Hex(0x9a9387)));
	}
	if (Pages > 1)
	{
		const float Rx = Home.X + Home.W - 28.0f;
		if (RoundArrow(UI, *C, "dropbagnext", Rx - 12.0f, Home.Y + 33.0f, 0, DropBagPage < Pages - 1, true))
		{
			++DropBagPage;
			DropBagPageAt = Now;
		}
		UI.Text(std::to_string(DropBagPage + 1) + " / " + std::to_string(Pages), Rx - 40.0f, Home.Y + 38.0f, Ts(12.0f, 700, Hex(0x9a9387), Align::Right, Baseline::Alphabetic, true));
		const float Pw = UI.Measure(std::to_string(DropBagPage + 1) + " / " + std::to_string(Pages), 12.0f, 700, true);
		if (RoundArrow(UI, *C, "dropbagprev", Rx - 62.0f - Pw, Home.Y + 33.0f, 1, DropBagPage > 0, true))
		{
			--DropBagPage;
			DropBagPageAt = Now;
		}
	}
	if (DropBag.empty())
	{
		NetParagraph(*C, "Empty. Order something, or walk down to the Lucky Penny (G at the desk).", Home.X + 470.0f, Home.Y + 70.0f, 440.0f, 14.0f, 500, Hex(0x9a9387), 20.0f, 2);
		DropBagSlide.clear();
		return std::string();
	}

	// The cells on this page, sliding to their slots when the bag closes up (a page turn slides the page in).
	const float Turn = NetEase((Now - DropBagPageAt) / 0.3);
	const float Fly = Dt > 0.0 ? NetFollow(Dt, 16.0) : 1.0f;
	std::string Hovered;
	std::string Eat;
	std::string Reorder;
	C->PushClip({Grid.X - 6.0f, Grid.Y - 6.0f, Grid.W + 12.0f, Grid.H + 12.0f});
	const float A0 = C->GetAlpha();
	C->SetAlpha(A0 * (0.35f + 0.65f * Turn));
	for (int K = DropBagPage * DropBagPer; K < std::min(N, (DropBagPage + 1) * DropBagPer); ++K)
	{
		const std::string& Id = DropBag[static_cast<size_t>(K)];
		const store::Item* I = store::Find(Id);
		if (!I)
		{
			continue;
		}
		const float Target = Nf(K - DropBagPage * DropBagPer);
		auto Found = DropBagSlide.find(Id);
		if (Found == DropBagSlide.end() || Turn < 1.0f || std::fabs(Found->second - Target) > 3.5f)
		{
			Found = DropBagSlide.insert_or_assign(Id, Target).first;
		}
		Found->second += (Target - Found->second) * Fly;
		const bool Settled = std::fabs(Found->second - Target) < 0.05f;
		const Vec2 At = BagCellAt(Grid, Found->second);
		const Rect Cell{At.X + (1.0f - Turn) * 24.0f, At.Y, 140.0f, 80.0f};
		const int Have = Held(Id);
		if (Have > 0)
		{
			// Click only once it has settled, so a cell still sliding into place can't be eaten by accident.
			const Ui::ClickState St = UI.Clickable("dropeat" + Id, Cell, Settled);
			const bool Waste = Wasted(*I, L);
			UI.RRect(Cell, 12.0f, St.Hover ? Hex(0x2a303a) : Hex(0x22272f), St.Hover ? DropGold : Hex(0x343a44), 1.0f);
			DrawProduct(*C, *I, Cell.X + 32.0f, Cell.Y + 40.0f, 52.0f);
			UI.Text(I->Name, Cell.X + 62.0f, Cell.Y + 30.0f, Ts(CellNameSize(UI, I->Name), 800, Hex(0xf4efe6), Align::Left, Baseline::Alphabetic, false, 74.0f));
			UI.Text("\xC3\x97" + std::to_string(Have), Cell.X + 62.0f, Cell.Y + 48.0f, Ts(12.0f, 700, Hex(0x9a9387)));
			const char* Verb = I->Drink() ? "Drink" : "Eat";
			UI.Text(Waste && !St.Hover ? (I->Drink() ? "Not thirsty" : "Not hungry") : Verb, Cell.X + 62.0f, Cell.Y + 68.0f,
				Ts(12.0f, 900, Waste ? Hex(0x8a8377) : St.Hover ? DropGold : Hex(0xd8b25a)));
			// New in the bag: a tag for a minute, on the count's line (up by the name it would cover the letters' tops).
			const auto New = DropBagNewAt.find(Id);
			if (New != DropBagNewAt.end())
			{
				const float Na = Nf(Clamp01((DropNewFor - (Now - New->second)) / 5.0));
				UI.RRect({Cell.X + Cell.W - 42.0f, Cell.Y + 36.0f, 34.0f, 16.0f}, 8.0f, NetA(DropGold, Na));
				UI.Text("NEW", Cell.X + Cell.W - 25.0f, Cell.Y + 48.0f, Ts(9.5f, 900, NetA(Hex(0x1d2129), Na), Align::Center));
			}
			if (St.Hover)
			{
				Hovered = Id;
			}
			if (St.Clicked)
			{
				Eat = Id;
			}
		}
		else
		{
			// Used up: the cell stays (the pointer is still over the bag) and offers to put it in the order.
			const Ui::ClickState St = UI.Clickable("dropbagadd" + Id, Cell, Settled);
			UI.RRect(Cell, 12.0f, St.Hover ? Hex(0x252a32) : Hex(0x1b1f26), St.Hover ? NetA(DropRed, 0.8f) : Hex(0x2c323b), 1.0f);
			const float Ca = C->GetAlpha();
			C->SetAlpha(Ca * 0.3f);
			DrawProduct(*C, *I, Cell.X + 32.0f, Cell.Y + 40.0f, 52.0f);
			C->SetAlpha(Ca);
			UI.Text(I->Name, Cell.X + 62.0f, Cell.Y + 30.0f, Ts(CellNameSize(UI, I->Name), 800, Hex(0x6f6a62), Align::Left, Baseline::Alphabetic, false, 74.0f));
			UI.Text("All gone", Cell.X + 62.0f, Cell.Y + 48.0f, Ts(12.0f, 700, Hex(0x6f6a62)));
			UI.Text(St.Hover ? "+ Order" : "Order more", Cell.X + 62.0f, Cell.Y + 68.0f, Ts(12.0f, 900, St.Hover ? Hex(0xff8a8d) : Hex(0x8a6a6a)));
			if (St.Clicked)
			{
				Reorder = Id;
			}
		}
	}
	C->SetAlpha(A0);
	C->PopClip();
	// Forget the slides of cells that are gone for good.
	for (auto It = DropBagSlide.begin(); It != DropBagSlide.end();)
	{
		It = std::find(DropBag.begin(), DropBag.end(), It->first) == DropBag.end() ? DropBagSlide.erase(It) : std::next(It);
	}
	if (!Eat.empty())
	{
		// "Drank the Cascade. Thirst -45, energy +2.": what it actually did (Session::Consume writes the line).
		const std::string Fail = S.Consume(Eat);
		DropSay(Fail.empty() ? S.LastEaten.Line : Fail, !Fail.empty(), true, Now);
	}
	if (!Reorder.empty())
	{
		const store::Item* I = store::Find(Reorder);
		int InCart = 0;
		for (const auto& Ln : DropCart.Lines)
		{
			InCart += Ln.first == Reorder ? Ln.second : 0;
		}
		if (I && InCart < DropMaxEach)
		{
			DropCart.Add(Reorder);
			DropCartFlash = Reorder;
			DropCartFlashAt = Now;
			DropCartScroll = CartScrollTo(DropCart, Reorder, DropCartScroll);
			DropShelf = static_cast<int>(I->Where);
			DropSay(I->Name + " is in your order.", false, true, Now);
		}
		else if (I)
		{
			// The cart takes nine of a kind: say so rather than click for nothing.
			DropSay(I->Name + ": nine in your order already.", false, true, Now);
		}
	}
	return Hovered;
}
} // namespace ui
} // namespace ss
