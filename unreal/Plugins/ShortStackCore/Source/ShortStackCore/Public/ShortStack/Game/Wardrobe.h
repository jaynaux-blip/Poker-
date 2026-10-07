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
 * The catalog: every jacket in each of the creator's eight colors (so any created look is a closet), tees plain, print
 * and graphic, jeans, chinos, work pants, shorts, sneakers, work shoes, four hats and four frames, the thrift's three
 * rare finds and the pawn's two vintage pieces. A thrift piece also comes Worn or Rough: the same garment under the id
 * "<id>~worn" / "<id>~rough", with its Condition set (15% and 30% off at the register, and less Style).
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
	/**
	 * Outer and Top: hero OutfitColor palette index 0..7, and for tees 8 white and 9 heather grey. Elsewhere the slot's
	 * own (Tone gives the RGB): jeans 0 dark, 1 mid, 2 light wash, 3 black; chinos 0 khaki, 1 navy, 2 olive, 3 black;
	 * work pants 0 brown duck, 1 black; shorts 0 khaki, 1 grey, 2 navy; sneakers 0 white, 1 black, 2 grey; work shoes 0
	 * brown, 1 black; hats and frames 0, their own color.
	 */
	int Color = 0;
	int Print = -1;    // PrintName's index (tees, the flannel's tartan), -1 none
	int Graphic = -1;  // GraphicName's index (tees, the Embercrest fleece's crest), -1 none
	int Tier = 0;      // 0 thrift, 1 vintage (the pawn's curios), 2 better (later)
	Chips PriceCents = 0; // the thrift's list price (the pawn's for the vintage pieces), before condition and tax
	int Condition = 0; // 0 good, 1 worn, 2 rough
	double Style = 0.0; // 0..1: the cut, its tier and its condition
	int Warmth = 0;    // 0..3, recorded for later
	bool RainProof = false;
	bool Rare = false;
};
SHORTSTACKCORE_API const std::vector<Garment>& Catalog();
SHORTSTACKCORE_API const Garment* Find(const std::string& Id);
/** The slot a cut is worn in. */
SHORTSTACKCORE_API Slot SlotOf(Cut K);
/** The garment's main color, 0xRRGGBB (the thrift's swatches, the portrait, the 3D's tint). */
SHORTSTACKCORE_API uint32_t Tone(const Garment& G);
/** "T_Print_Stripes" ... for Garment::Print (nullptr for none): Stripes, Breton, Ringer, Tartan, Gingham, Dots, Camo. */
SHORTSTACKCORE_API const char* PrintName(int Print);
/** "T_Graphic_Embercrest" ... for Garment::Graphic (nullptr for none): Embercrest, AllIn, Chip, BadBeat, Sunset,
 *  Number, SundayMajor (the thrift's rare tee, looks' prints.py). */
SHORTSTACKCORE_API const char* GraphicName(int Graphic);
SHORTSTACKCORE_API const char* ConditionName(int Condition); // "Good", "Worn", "Rough"
/** The tag on a thrift piece: 0 white, 1 yellow (half off on Saturdays), 2 pink, 3 green, 4 blue. Fixed per piece. */
SHORTSTACKCORE_API int TagOf(const Garment& G);

struct Closet
{
	std::set<std::string> Owned;
	std::array<std::string, SlotCount> Worn; // "" in Top: the tee in the outfit color; "" in Bottom: shorts; "" in Shoes: barefoot; "" elsewhere: none
	bool Seeded = false;
};
/** The creator's jacket (none for Outfit 6), hat and glasses, plus a white tee, mid-wash jeans, white sneakers, shorts. */
SHORTSTACKCORE_API Closet SeedFrom(const hero::Look& L);
/** Collapses the closet into Look for the portrait only: Outfit from Outer (6 "None" when Outer is empty), OutfitColor
 *  from the jacket (or the tee with no jacket), Hat and Glasses. The table (HeroPersona) and the Embercrest read the
 *  closet directly, so independent jacket and tee colors survive there. The trilby draws as the bucket hat and the
 *  aviators as shades on the portrait, which has no others; a white or heather tee as Sand or Charcoal. */
SHORTSTACKCORE_API void ApplyTo(const Closet& C, hero::Look& L);
/** 0..1: what's worn (the jacket counts most, then the tee, the trousers and the shoes; barefoot adds nothing; a vintage
 *  hat or frames add). */
SHORTSTACKCORE_API double StyleOf(const Closet& C);
/** The week the racks were last restocked (Thursdays): the number of Thursdays since the first, day 3. */
SHORTSTACKCORE_API int ThriftWeek(double World);
/** This week's racks at the thrift (restocked Thursdays), only of kinds the host can draw (a bit per Cut). */
SHORTSTACKCORE_API std::vector<std::string> ThriftStock(int Week, uint64_t Seed, uint64_t KindsAvailable);
SHORTSTACKCORE_API Chips ThriftPrice(const Garment& G, double World); // condition and Saturday's yellow tags
} // namespace wardrobe
} // namespace ss
