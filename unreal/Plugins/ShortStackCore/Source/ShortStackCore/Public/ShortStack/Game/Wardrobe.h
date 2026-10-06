#pragma once

#include "ShortStack/Common.h"
#include "ShortStack/Game/Hero.h"

#include <array>
#include <cstdint>
#include <set>
#include <string>
#include <vector>

namespace ss
{
/**
 * The hero's clothes (the realism update's spec, §5.2 and §7.1): every garment the game can draw, the closet of what
 * the player owns and wears, and the thrift's weekly racks. The closet is saved (§7.3) and travels with the player
 * (the apartment, Dee's couch, the thrift's booth).
 *
 * Phase A: the contract only. The bodies return defaults (an empty catalog, an unseeded closet) until core-shops'
 * Phase B fills them.
 */
namespace wardrobe
{
enum class Slot : int
{
	Top,
	Outer,
	Bottom,
	Shoes,
	Hat,
	Eyes,
	Count,
};
constexpr int SlotCount = static_cast<int>(Slot::Count);

/** The mesh family the host draws. Outer's six are hero::Look's Outfit 0..5 in order; Outfit 6, "None", is no jacket. */
enum class Cut : int
{
	Tee,
	Hoodie,
	Bomber,
	Flannel,
	Denim,
	Leather,
	Track,
	Shorts,
	Jeans,
	Chinos,
	WorkPants,
	Sneakers,
	WorkShoes,
	Beanie,
	BallCap,
	CapBack,
	Bucket,
	Trilby,
	Round,
	Square,
	Wire,
	Shades,
	Aviators,
	Count,
};

struct Garment
{
	std::string Id;    // "outer.denim.navy", "top.graphic.sundaymajor"
	Slot Where = Slot::Top;
	Cut Kind = Cut::Tee;
	std::string Name;  // "Navy denim jacket"
	int Color = 0;     // hero OutfitColor palette index 0..7 (Outer, Top), or the slot's own palette
	int Print = -1;    // T_Print_* index (shirts), -1 none
	int Graphic = -1;  // T_Graphic_* index (tees), -1 none
	int Tier = 0;      // 0 thrift, 1 vintage (the pawn's curios), 2 better (later)
	Chips PriceCents = 0;
	int Condition = 0; // 0 good, 1 worn, 2 rough
	double Style = 0.0;
	int Warmth = 0;    // 0..3, recorded for later
	bool RainProof = false;
	bool Rare = false;
};
SHORTSTACKCORE_API const std::vector<Garment>& Catalog();
SHORTSTACKCORE_API const Garment* Find(const std::string& Id);

struct Closet
{
	std::set<std::string> Owned;
	std::array<std::string, SlotCount> Worn; // "" in Top: the tee in the outfit color; "" in Bottom: shorts; "" elsewhere: none
	bool Seeded = false;
};
/** The creator's jacket (none for Outfit 6), hat and glasses, plus a white tee, mid-wash jeans, white sneakers, shorts. */
SHORTSTACKCORE_API Closet SeedFrom(const hero::Look& L);
/** Collapses the closet into Look for the portrait only: Outfit from Outer (6 "None" when Outer is empty), OutfitColor
 *  from the jacket (or the tee with no jacket), Hat and Glasses. The table (HeroPersona) and the Embercrest read the
 *  closet directly, so independent jacket and tee colors survive there. */
SHORTSTACKCORE_API void ApplyTo(const Closet& C, hero::Look& L);
SHORTSTACKCORE_API double StyleOf(const Closet& C);
/** This week's racks at the thrift (restocked Thursdays), only of kinds the host can draw (a bit per Cut). */
SHORTSTACKCORE_API std::vector<std::string> ThriftStock(int Week, uint64_t Seed, uint64_t KindsAvailable);
SHORTSTACKCORE_API Chips ThriftPrice(const Garment& G, double World); // condition and Saturday's yellow tags
} // namespace wardrobe
} // namespace ss
