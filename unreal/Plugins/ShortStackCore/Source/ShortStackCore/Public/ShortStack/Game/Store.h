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
	/** Drunk, not eaten: everything from the cooler and the coffee bar (a cola's sugar doesn't make it a meal). */
	bool Drink() const { return Where == Shelf::Drinks || Where == Shelf::Coffee; }
	bool Food() const { return !Drink(); }
};

/** Everything on the shelves, shelf by shelf. */
SHORTSTACKCORE_API const std::vector<Item>& Catalog();
SHORTSTACKCORE_API const Item* Find(const std::string& Id);

/**
 * What one of an item does, as relief: hunger and thirst go down by it, energy goes up (negative: worse). MealBoost
 * is the line cook's (hero::Perks): a meal goes further, and food and drink both give more energy. The counter, the
 * app and the bag all use this, so what's promised is what's applied.
 */
struct Relief
{
	double Hunger = 0.0;
	double Thirst = 0.0;
	double Energy = 0.0;
};
SHORTSTACKCORE_API Relief ReliefOf(const Item& I, double MealBoost);
/** "the Cascade", "the Hilltop chips": the item in a sentence. */
SHORTSTACKCORE_API std::string Called(const Item& I);
/**
 * The line after eating or drinking one, from what the meters actually did (the changes, so relief is negative):
 * "Drank the Cascade. Thirst -45, energy +2." (with a real minus sign).
 */
SHORTSTACKCORE_API std::string UsedLine(const Item& I, double HungerChange, double ThirstChange, double EnergyChange);

/** Sales tax on the receipt (7.25%, half a cent up). */
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
	/** The same basket at Penny Drop's prices. */
	SHORTSTACKCORE_API Chips AppSubtotal() const;
};

/**
 * Penny Drop: the store's own delivery app on the laptop. Everything costs 15% more than on the shelf
 * (rounded up to the next 9 cents), plus a flat fee, and taxed; it's at the door in 25 to 45 minutes.
 * Walking down there stays cheaper.
 */
constexpr Chips DeliveryFeeCents = 399;
constexpr Chips DeliveryMinimumCents = 500; // of goods, at app prices
SHORTSTACKCORE_API Chips AppPrice(const Item& I);
/** What an order costs in all: the app's prices, the fee, and tax on both. */
SHORTSTACKCORE_API Chips DeliveryTotal(const Basket& B);

/** What Benny knows about the player walking up to the counter. */
struct ClerkContext
{
	double World = 0.0; // world minutes
	double Hunger = 0.0;
	double Energy = 100.0;
	bool OnSchedule = false;  // worked a Lucky Penny shift in the last week
	bool Evicted = false;     // first time in since the locks changed (the block talks)
	bool Declined = false;    // the card just said no at this counter
	double SinceVisit = -1.0; // minutes since the player last paid at this counter (-1: never)
	std::string Usual;        // what the player buys most here ("Oodle Cup"; "" until it's a habit)
};
/** What the clerk (Benny, nights) says: how the player looks first, then what he knows about them, then the hour. */
SHORTSTACKCORE_API std::string ClerkLine(const ClerkContext& Ctx, const Basket& B);
} // namespace store
} // namespace ss
