#pragma once

#include "ShortStack/Common.h"

#include <string>
#include <utility>
#include <vector>

namespace ss
{
/**
 * The Lucky Penny #212: the all-night mini-mart at Fifth and Market, a block from the apartment (and
 * the store behind ShiftLink's night cashier job). What's on the shelves, what it costs, and what it
 * does for someone who hasn't eaten since the last tournament. Brands are the store's own.
 */
namespace store
{
enum class Shelf : int
{
	Drinks,
	Snacks,
	Hot, // the roller grill, the microwave, the deli case
	Coffee,
	Count,
};
constexpr int ShelfCount = static_cast<int>(Shelf::Count);
SHORTSTACKCORE_API const char* ShelfName(Shelf S); // "DRINKS"

/** How a product is drawn (the counter's vector art; the 3D shelves use the same list). */
enum class Art : int
{
	Can,
	Bottle,
	Cup,      // a coffee or fountain cup with a lid
	Bag,      // chips
	Bar,      // candy
	Pouch,    // trail mix
	HotDog,
	Burrito,
	Noodles,  // a noodle cup
	Sandwich, // a wedge in a clamshell
};

struct Item
{
	std::string Id;    // "volt-rush"
	std::string Name;  // "Volt Rush"
	std::string Kind;  // "Energy drink, 16 oz"
	std::string Blurb; // the shelf tag's line
	Shelf Where = Shelf::Drinks;
	Art Look = Art::Can;
	Chips PriceCents = 0;
	double Hunger = 0.0; // hunger relieved (0..100 scale; negative: makes it worse)
	double Thirst = 0.0; // thirst relieved
	double Energy = 0.0; // energy gained
	uint32_t Color = 0xffffff;  // packaging
	uint32_t Accent = 0x000000; // label
	bool Food() const { return Hunger > 0.0; }
};

/** Everything on the shelves, shelf by shelf. */
SHORTSTACKCORE_API const std::vector<Item>& Catalog();
SHORTSTACKCORE_API const Item* Find(const std::string& Id);

/** Sales tax on the receipt (7.25%). */
SHORTSTACKCORE_API Chips Tax(Chips Subtotal);

/** A basket at the counter: item ids and counts, in the order they were picked up. */
struct Basket
{
	std::vector<std::pair<std::string, int>> Lines;

	SHORTSTACKCORE_API void Add(const std::string& Id, int Count = 1);
	SHORTSTACKCORE_API void Remove(const std::string& Id);
	SHORTSTACKCORE_API int Count() const;
	SHORTSTACKCORE_API Chips Subtotal() const;
	Chips Total() const { return Subtotal() + Tax(Subtotal()); }
	bool Empty() const { return Lines.empty(); }
};

/** What the clerk (Benny, nights) says as the player walks up, by the hour, the basket and their history. */
SHORTSTACKCORE_API std::string ClerkLine(double World, const Basket& B, int ShiftsWorked, double Hunger, double Energy);
} // namespace store
} // namespace ss
