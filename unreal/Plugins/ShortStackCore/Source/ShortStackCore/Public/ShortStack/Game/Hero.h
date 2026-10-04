#pragma once

#include "ShortStack/Common.h"

#include <string>
#include <vector>

namespace ss
{
class Rng;

/**
 * The person behind the screen name: who the player made in the character creator. A name, an age,
 * where they're from, the life that brought them to a rented room and a $2.37 bankroll (a background,
 * with a perk the rest of the game honors), and what they look like, from the face to the jacket.
 * The screen name stays the session's HeroName; this is the person the street and the live rooms see.
 */
namespace hero
{
enum class Background : int
{
	Kitchen,    // line cook: shifts pay better, food goes further
	DealersKid, // grew up around a card room: reads come faster
	Dropout,    // left a stats degree for the online games: bounty events from the start
	Bouncer,    // years on a casino door: bad beats sting less
	Hustler,    // knows Marcus from the old block: runs are safer, heat cools faster
	Newcomer,   // new in town with a suitcase and some savings
	Count,
};
constexpr int BackgroundCount = static_cast<int>(Background::Count);

struct BackgroundInfo
{
	Background Id = Background::Newcomer;
	const char* Key = "";     // saved: "kitchen"
	const char* Name = "";    // "Line Cook"
	const char* Tagline = ""; // one line under the name
	const char* Story = "";   // the paragraph on the creator's background page
	const char* PerkName = "";
	const char* PerkText = "";
	uint32_t Color = 0xffffff;
};
SHORTSTACKCORE_API const BackgroundInfo& InfoOf(Background B);
/** By saved key ("kitchen"); nullptr when unknown. */
SHORTSTACKCORE_API const BackgroundInfo* FindBackground(const std::string& Key);

/** What a background changes, applied where the game decides those things. Neutral for a career with no character. */
struct Perks
{
	double JobPay = 1.0;     // ShiftLink wages and tips
	double MealBoost = 1.0;  // energy from food and drink
	double HustleRisk = 0.0; // added to a Burner run's chance of going wrong
	double HeatCool = 1.0;   // how fast police heat fades
	double TiltGain = 1.0;   // how hard bad beats and coolers hit
	int ReadWeight = 1;      // confirmations a tell counts for at showdown (2 or more: learned)
	Chips StartCents = 0;    // added to the starting bankroll
	std::vector<std::string> StartUnlocks; // RiverLine features open from the first night
};
SHORTSTACKCORE_API Perks PerksOf(Background B);

/** Where the player can be from: the countries the network knows (with a flag), and a home city for each. */
struct CountryInfo
{
	const char* Code = ""; // "US"
	const char* Name = ""; // "United States"
	const char* City = ""; // "Cleveland"
	const char* Demonym = ""; // "American"
};
SHORTSTACKCORE_API const std::vector<CountryInfo>& Countries();
SHORTSTACKCORE_API const CountryInfo* FindCountry(const std::string& Code);

/** Every choice the creator's LOOK page offers, in page order. Height is in centimeters; the rest are option indices. */
enum class Slot : int
{
	Body,        // body type
	Face,        // face shape
	Skin,        // skin tone
	Eyes,        // eye color
	Brows,
	Hair,        // style (0: shaved)
	HairColor,
	FacialHair,
	Build,
	Height,
	Outfit,
	OutfitColor,
	Glasses,
	Hat,
	Count,
};
constexpr int SlotCount = static_cast<int>(Slot::Count);
constexpr int MinHeight = 152;
constexpr int MaxHeight = 203;

struct Look
{
	int Body = 0;
	int Face = 0;
	int Skin = 3;
	int Eyes = 1;
	int Brows = 1;
	int Hair = 3;
	int HairColor = 1;
	int FacialHair = 1;
	int Build = 1;
	int Height = 178;
	int Outfit = 0;
	int OutfitColor = 0;
	int Glasses = 0;
	int Hat = 0;

	SHORTSTACKCORE_API int Get(Slot S) const;
	/** Clamped (wrapped for options when Wrap). */
	SHORTSTACKCORE_API void Set(Slot S, int Value, bool Wrap = false);
	bool operator==(const Look& O) const;
};
SHORTSTACKCORE_API const char* SlotName(Slot S); // "HAIR COLOR"
SHORTSTACKCORE_API int OptionCount(Slot S);      // options (Height: the centimeters in range)
SHORTSTACKCORE_API std::string OptionName(Slot S, int Value); // "Undercut", "6'1\" (185 cm)"
/** The page the slot sits under: 0 face and skin, 1 hair, 2 body, 3 clothes. */
SHORTSTACKCORE_API int SlotGroup(Slot S);

// Palettes (0xRRGGBB): the portrait and the 3D character share them.
SHORTSTACKCORE_API uint32_t SkinTone(int Index);
SHORTSTACKCORE_API uint32_t HairTone(int Index);
SHORTSTACKCORE_API uint32_t EyeTone(int Index);
SHORTSTACKCORE_API uint32_t OutfitTone(int Index);
/** The hair as it shows at this age: the color drains and silver comes in from the mid-forties (salt and pepper, not beige). */
SHORTSTACKCORE_API uint32_t HairToneAt(const Look& L, int Age);
/** The headwear's color: one that goes with the jacket without matching it, and stands apart from the hair under it (at this age). */
SHORTSTACKCORE_API uint32_t HatTone(const Look& L, int Age = 0);

constexpr int MinAge = 18;
constexpr int MaxAge = 65;

struct Character
{
	std::string FirstName = "Jesse";
	std::string LastName = "Cole";
	int Age = 24;
	std::string Country = "US";
	Background Story = Background::Newcomer;
	Look Appearance;
	/** False for a career from before the creator (or a fresh default): the background's perk doesn't apply. */
	bool Created = false;

	SHORTSTACKCORE_API std::string FullName() const;
	/** Why the creator can't finish yet ("" when it can). */
	SHORTSTACKCORE_API std::string Problem() const;
	/** "hero\t..." lines for the save. */
	SHORTSTACKCORE_API std::string Serialize() const;
	/** Reads one save line split on tabs (a "hero" line); false when it isn't one. */
	SHORTSTACKCORE_API bool Read(const std::vector<std::string>& Fields);
};

/** The perks this character's background grants (neutral when the character was never created). */
SHORTSTACKCORE_API Perks PerksOf(const Character& C);
/** A random, plausible person (the creator's RANDOMIZE). */
SHORTSTACKCORE_API Character Random(Rng& R);
/** A random look for someone of this age and body type (the creator's RANDOM LOOK): grey comes with the years. */
SHORTSTACKCORE_API Look RandomLook(Rng& R, int Age, int Body);
/** Names people from this country often have (first names, or last names), for the creator to suggest. */
SHORTSTACKCORE_API std::vector<std::string> NameSuggestions(const std::string& Country, bool Last);
/** The ID card's paragraph: "Jesse Cole, 24. Grew up in Cleveland, ..." */
SHORTSTACKCORE_API std::string Bio(const Character& C);
/** Letters in a name part (an accented letter is one, however many bytes it takes). */
SHORTSTACKCORE_API int NameLength(const std::string& Part);
constexpr int MaxNameLength = 16;
/** A name part the creator accepts: 1 to 16 letters, spaces, apostrophes or hyphens, starting with a letter. */
SHORTSTACKCORE_API bool NameValid(const std::string& Part);
} // namespace hero
} // namespace ss
