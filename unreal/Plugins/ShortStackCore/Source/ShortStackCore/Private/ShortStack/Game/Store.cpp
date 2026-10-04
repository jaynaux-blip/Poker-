#include "ShortStack/Game/Store.h"
#include "../StrictFloat.h"

#include <algorithm>
#include <cmath>

namespace ss
{
namespace store
{
const char* ShelfName(Shelf S)
{
	switch (S)
	{
	case Shelf::Drinks: return "DRINKS";
	case Shelf::Snacks: return "SNACKS";
	case Shelf::Hot: return "HOT FOOD";
	case Shelf::Coffee: return "COFFEE BAR";
	default: return "";
	}
}

const std::vector<Item>& Catalog()
{
	static const std::vector<Item> L = [] {
		std::vector<Item> Out;
		auto Add = [&Out](const char* Id, const char* Name, const char* Kind, const char* Blurb, Shelf Where, Art Look, Chips Price, double Hunger, double Thirst, double Energy, uint32_t Color,
					   uint32_t Accent) {
			Item I;
			I.Id = Id;
			I.Name = Name;
			I.Kind = Kind;
			I.Blurb = Blurb;
			I.Where = Where;
			I.Look = Look;
			I.PriceCents = Price;
			I.Hunger = Hunger;
			I.Thirst = Thirst;
			I.Energy = Energy;
			I.Color = Color;
			I.Accent = Accent;
			Out.push_back(I);
		};
		// The cooler.
		Add("cascade", "Cascade", "Spring water, 20 oz", "Cold, clear, cheapest thing in the cooler.", Shelf::Drinks, Art::Bottle, 149, 0.0, 45.0, 2.0, 0x8fd3ff, 0x1d6fb8);
		Add("fizz-cola", "Fizz Cola", "Cola, 12 oz can", "Sugar, bubbles, a little caffeine.", Shelf::Drinks, Art::Can, 199, 4.0, 30.0, 6.0, 0xd8262f, 0xffffff);
		Add("volt-rush", "Volt Rush", "Energy drink, 16 oz", "Tastes like a battery. Works like one too.", Shelf::Drinks, Art::Can, 299, 0.0, 20.0, 18.0, 0x16181d, 0xb6ff2e);
		Add("night-owl-brew", "Night Owl", "Cold brew, 11 oz", "RiverLine's tournament, in a bottle. Strong.", Shelf::Drinks, Art::Bottle, 349, 0.0, 15.0, 14.0, 0x3a2418, 0xf2c14e);
		Add("sunny-peach", "Sunny Peach", "Iced tea, 18 oz", "Sweet tea with a cartoon peach on it.", Shelf::Drinks, Art::Bottle, 229, 2.0, 35.0, 4.0, 0xffb36b, 0xd2452b);
		// The aisle.
		Add("hilltop-chips", "Hilltop", "Kettle chips, salt & vinegar", "Loud bag, louder flavor. Makes you thirsty.", Shelf::Snacks, Art::Bag, 179, 15.0, -8.0, 2.0, 0x2e9e5b, 0xf5f0e1);
		Add("choco-stack", "Choco Stack", "Chocolate bar", "Peanuts, caramel, a sugar rush.", Shelf::Snacks, Art::Bar, 149, 12.0, -2.0, 5.0, 0x5b2a17, 0xf2c14e);
		Add("trail-mix", "Ridgeline", "Trail mix, 6 oz", "Nuts and raisins. The healthy choice, allegedly.", Shelf::Snacks, Art::Pouch, 399, 22.0, -4.0, 4.0, 0x8a6a3a, 0x2c4a2e);
		// The roller grill and the deli case.
		Add("roller-dog", "Roller Dog", "Hot dog, all-beef", "Been on the roller since about eleven.", Shelf::Hot, Art::HotDog, 199, 30.0, -2.0, 3.0, 0xc8553d, 0xf2c14e);
		Add("bean-burrito", "Big Bean", "Burrito, microwaved", "Ninety seconds, then it's lava.", Shelf::Hot, Art::Burrito, 279, 38.0, -3.0, 4.0, 0xe9dcc0, 0xb5462e);
		Add("oodle-cup", "Oodle Cup", "Noodle cup, chicken", "Hot water's free at the coffee bar.", Shelf::Hot, Art::Noodles, 129, 28.0, -5.0, 3.0, 0xffffff, 0xe0322e);
		Add("egg-salad", "Deli Wedge", "Egg salad sandwich", "Dated today. Probably.", Shelf::Hot, Art::Sandwich, 449, 42.0, 0.0, 5.0, 0xf3e3a2, 0x7a9a3a);
		// Self-serve.
		Add("drip-coffee", "Drip Coffee", "Self-serve, 16 oz", "Burnt, hot, refilled at midnight.", Shelf::Coffee, Art::Cup, 159, 0.0, 10.0, 10.0, 0xf5f1e8, 0x6b3a1e);
		Add("hot-cocoa", "Hot Cocoa", "Machine cocoa, 12 oz", "Powder and hot water. Tastes like being nine.", Shelf::Coffee, Art::Cup, 139, 6.0, 12.0, 4.0, 0xf5f1e8, 0x9b2c2c);
		return Out;
	}();
	return L;
}

const Item* Find(const std::string& Id)
{
	for (const Item& I : Catalog())
	{
		if (I.Id == Id)
		{
			return &I;
		}
	}
	return nullptr;
}

Chips Tax(Chips Subtotal)
{
	return static_cast<Chips>(std::llround(static_cast<double>(Subtotal) * 0.0725));
}

void Basket::Add(const std::string& Id, int Count)
{
	if (!Find(Id) || Count <= 0)
	{
		return;
	}
	for (std::pair<std::string, int>& L : Lines)
	{
		if (L.first == Id)
		{
			L.second = std::min(9, L.second + Count);
			return;
		}
	}
	Lines.push_back({Id, std::min(9, Count)});
}

void Basket::Remove(const std::string& Id)
{
	for (size_t I = 0; I < Lines.size(); ++I)
	{
		if (Lines[I].first == Id)
		{
			if (--Lines[I].second <= 0)
			{
				Lines.erase(Lines.begin() + static_cast<std::ptrdiff_t>(I));
			}
			return;
		}
	}
}

int Basket::Count() const
{
	int N = 0;
	for (const std::pair<std::string, int>& L : Lines)
	{
		N += L.second;
	}
	return N;
}

Chips AppPrice(const Item& I)
{
	// 15% over the shelf, up to the next price ending in 9.
	const Chips Raw = (I.PriceCents * 115 + 99) / 100;
	return Raw / 10 * 10 + 9;
}

Chips DeliveryTotal(const Basket& B)
{
	const Chips Goods = B.AppSubtotal() + DeliveryFeeCents;
	return Goods + Tax(Goods);
}

Chips Basket::AppSubtotal() const
{
	Chips Sum = 0;
	for (const std::pair<std::string, int>& L : Lines)
	{
		if (const Item* I = Find(L.first))
		{
			Sum += AppPrice(*I) * L.second;
		}
	}
	return Sum;
}

Chips Basket::Subtotal() const
{
	Chips Sum = 0;
	for (const std::pair<std::string, int>& L : Lines)
	{
		if (const Item* I = Find(L.first))
		{
			Sum += I->PriceCents * L.second;
		}
	}
	return Sum;
}

std::string ClerkLine(double World, const Basket& B, int ShiftsWorked, double Hunger, double Energy)
{
	const double Day = 1440.0;
	const double T = std::fmod(std::fmod(World, Day) + Day, Day) / 60.0;
	int Energies = 0;
	for (const std::pair<std::string, int>& L : B.Lines)
	{
		Energies += L.first == "volt-rush" || L.first == "night-owl-brew" ? L.second : 0;
	}
	if (Energies >= 2)
	{
		return "Two of those? You're either studying or playing cards.";
	}
	if (ShiftsWorked > 0 && B.Empty())
	{
		return "Hey, you're on the schedule this week, right? Don't tell Ray I gave you the employee discount. I didn't.";
	}
	if (B.Empty() && Hunger > 70.0)
	{
		return "You look like you need a roller dog. They're fresh. Ish.";
	}
	if (B.Empty() && Energy < 25.0)
	{
		return "Coffee's fresh at midnight. It's not midnight. Still hot, though.";
	}
	if (B.Empty())
	{
		return T >= 0.0 && T < 5.0 ? "Rough night? Everybody in here at this hour is having one." : T < 12.0 ? "Morning. Coffee's on the left." : T < 18.0 ? "Afternoon. Need anything, holler."
																														 : "Evening. Lotto's up to forty million if you're feeling lucky.";
	}
	return B.Count() >= 4 ? "Big night in, huh?" : "That it? Bag?";
}
} // namespace store
} // namespace ss
