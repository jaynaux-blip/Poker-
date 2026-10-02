#pragma once

#include "ShortStack/Common.h"

#include <map>
#include <string>
#include <vector>

namespace ss
{
/**
 * GearDrop: the online store on the laptop. Everything sold changes the game: screens add tables, the rig and the
 * camera, mic and lights make the stream (Kast) look and sound like a real broadcast, the chair, the coffee and a
 * better bed keep the player sharp for longer, the gym takes the edge off tilt. Subscriptions renew every thirty days
 * from the bankroll, and lapse when it can't cover them.
 */
namespace gear
{
enum class Category : int
{
	Rig,
	Stream,
	Home,
	Subscription,
};

/** Items in one slot replace each other: only the best one owned counts. */
enum class Slot : int
{
	None,
	Camera,
	Mic,
	Light,
	Pc,
	Net,
	Screen1,
	Screen2,
};

/** The pictures the store draws (UI/GearArt). */
enum class Art : int
{
	Ram,
	Monitor,
	MonitorWide,
	Tower,
	TowerDual,
	MacroPad,
	Webcam,
	WebcamPro,
	Mirrorless,
	MicUsb,
	MicXlr,
	RingLight,
	KeyLights,
	GreenScreen,
	Overlay,
	Router,
	Chair,
	Espresso,
	Mattress,
	Curtains,
	Gym,
	MealKit,
	ModBot,
	Headphones,
	Plant,
	Count,
};

struct Item
{
	std::string Id;
	std::string Name;
	std::string Brand;
	std::string Blurb;
	std::string Effect; // the one-line promise on the card ("+1 table", "Stream 1080p60")
	Category Cat = Category::Rig;
	Slot Where = Slot::None;
	Art Pic = Art::Ram;
	uint32_t Color = 0xff6b2c; // card accent
	Chips PriceCents = 0;      // one-off, or each month for subscriptions
	bool Monthly = false;
	std::string Requires; // an item that has to be owned first ("" for none)
	double Rating = 4.5;  // stars, for the card
	int Reviews = 0;
	// What it does.
	int Tables = 0;       // more screens, more tables
	double Quality = 0.0; // stream production value (best item per slot)
	int Resolution = 0;   // the stream resolution the rig or the line allows (480, 720, 1080, 1440)
	int StreamTables = 0; // tables the rig can stream without dropping frames
	double Drain = 0.0;   // share off the hourly energy drain
	double Rest = 0.0;    // share more energy from sleep
	double Calm = 0.0;    // tilt fades this much faster
	double Follow = 0.0;  // more viewers follow (overlays and alerts)
	double ModBot = 0.0;  // chance an automated filter catches a troll or a spammer
};

SHORTSTACKCORE_API const std::vector<Item>& Catalog();
SHORTSTACKCORE_API const Item* Find(const std::string& Id);
SHORTSTACKCORE_API const char* CategoryName(Category C);

/** Owned items: id -> when the subscription renews (world minutes; 0 for things bought outright). */
using Owned = std::map<std::string, double>;

/** What the owned gear adds up to. */
struct Effects
{
	int Tables = 2;            // the laptop's screen fits two tables
	double Quality = 0.0;      // 0 (a laptop webcam in the dark) .. about 1 (a studio)
	int Resolution = 480;      // what the stream goes out at
	int StreamTables = 1;      // tables streamed without dropped frames
	double Drain = 0.0;
	double Rest = 0.0;
	double Calm = 0.0;
	double Follow = 0.0;
	double ModBot = 0.0;
	int CamTier = 0;           // 0 the laptop's own webcam, 1 a 720p webcam, 2 1080p60, 3 a mirrorless camera
	int MicTier = 0;           // 0 the laptop's mic, 1 a USB mic, 2 a broadcast mic on an arm
	int Lights = 0;            // 0 the monitor's glow, 1 a ring light, 2 key lights
	int PcTier = 0;            // 0 the laptop, 1 more RAM, 2 a desktop, 3 a two-PC setup
	/** The laptop alone can't run the poker client and an encoder at once: streaming takes the first PC upgrade. */
	bool CanStream() const { return PcTier >= 1; }
	bool GreenScreen = false;
	bool Overlay = false;
	bool MacroPad = false;
	bool Fiber = false;
	Chips MonthlyCents = 0;    // what the subscriptions cost a month
};
SHORTSTACKCORE_API Effects Sum(const Owned& Items);
/** "1080p60" for the resolution. */
SHORTSTACKCORE_API std::string ResolutionLabel(int Resolution);
/** The cheapest PC upgrade (the one that unlocks streaming). */
SHORTSTACKCORE_API const Item& FirstPcUpgrade();
} // namespace gear
} // namespace ss
