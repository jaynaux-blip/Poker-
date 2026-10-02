#include "ShortStack/Game/Gear.h"

#include <algorithm>

namespace ss
{
namespace gear
{
namespace gear_detail
{
Chips Dollars(double D)
{
	return static_cast<Chips>(D * 100.0 + 0.5);
}

std::vector<Item> Build()
{
	std::vector<Item> L;
	auto Add = [&](const char* Id, Category Cat, Slot Where, Art Pic, uint32_t Color, const char* Name, const char* Brand, double Price, const char* Effect, const char* Blurb) -> Item& {
		Item I;
		I.Id = Id;
		I.Cat = Cat;
		I.Where = Where;
		I.Pic = Pic;
		I.Color = Color;
		I.Name = Name;
		I.Brand = Brand;
		I.PriceCents = Dollars(Price);
		I.Effect = Effect;
		I.Blurb = Blurb;
		L.push_back(I);
		return L.back();
	};
	// The rig: screens for tables, a machine that can stream.
	{
		Item& I = Add("ram-32", Category::Rig, Slot::Pc, Art::Ram, 0x60a5fa, "32 GB RAM kit", "Vortex", 89.0, "Unlocks streaming \xC2\xB7 720p",
			"The laptop stops choking. Enough headroom to run RiverLine and a stream encoder at once: this is what gets you live on Kast.");
		I.Resolution = 720;
		I.StreamTables = 2;
		I.Quality = 0.05;
		I.Rating = 4.6;
		I.Reviews = 2214;
	}
	{
		Item& I = Add("monitor-24", Category::Rig, Slot::Screen1, Art::Monitor, 0x38bdf8, "24\" 144 Hz monitor", "Vantage", 149.0, "+1 table (3)",
			"A second screen next to the laptop. Room for a third table without squinting.");
		I.Tables = 1;
		I.Rating = 4.5;
		I.Reviews = 8840;
	}
	{
		Item& I = Add("monitor-27", Category::Rig, Slot::Screen2, Art::MonitorWide, 0x22d3ee, "27\" 1440p monitor", "Vantage", 289.0, "+1 table (4)",
			"The grinder's wall: two big screens and the laptop. Four tables, all readable.");
		I.Tables = 1;
		I.Requires = "monitor-24";
		I.Rating = 4.7;
		I.Reviews = 5121;
	}
	{
		Item& I = Add("tower-mid", Category::Rig, Slot::Pc, Art::Tower, 0x818cf8, "Desktop PC, 8-core", "Forge", 899.0, "Streams 1080p60 \xC2\xB7 4 tables smooth",
			"A real desktop with a hardware encoder. Four tables and a 1080p stream without a dropped frame.");
		I.Resolution = 1080;
		I.StreamTables = 4;
		I.Quality = 0.14;
		I.Rating = 4.8;
		I.Reviews = 1290;
	}
	{
		Item& I = Add("tower-dual", Category::Rig, Slot::Pc, Art::TowerDual, 0xa78bfa, "Two-PC streaming setup", "Forge", 2499.0, "Stream 1440p60 \xC2\xB7 studio grade",
			"One machine plays, one machine streams. What the big poker channels run.");
		I.Resolution = 1440;
		I.StreamTables = 4;
		I.Quality = 0.2;
		I.Requires = "tower-mid";
		I.Rating = 4.9;
		I.Reviews = 302;
	}
	{
		Item& I = Add("macro-pad", Category::Rig, Slot::None, Art::MacroPad, 0xf472b6, "15-key macro pad", "Keystrip", 129.0, "Scenes and alerts on a button",
			"Ads, scenes and instant replays on keys. Chat notices when the show runs smooth.");
		I.Quality = 0.03;
		I.Rating = 4.6;
		I.Reviews = 3310;
	}
	{
		Item& I = Add("headphones", Category::Rig, Slot::None, Art::Headphones, 0x94a3b8, "Noise-cancelling headphones", "Hush", 99.0, "Tilt fades 15% faster",
			"The neighbors, the rain and the bad beats get a little quieter.");
		I.Calm = 0.15;
		I.Rating = 4.4;
		I.Reviews = 15602;
	}
	// Streaming gear.
	{
		Item& I = Add("webcam-720", Category::Stream, Slot::Camera, Art::Webcam, 0xa3e635, "720p webcam", "Halo", 39.0, "Facecam 720p",
			"Better than the laptop's pinhole. Grainy in the dark, but it's a face.");
		I.Quality = 0.1;
		I.Rating = 3.9;
		I.Reviews = 22410;
	}
	{
		Item& I = Add("webcam-1080", Category::Stream, Slot::Camera, Art::WebcamPro, 0x84cc16, "1080p60 webcam", "Halo", 119.0, "Facecam 1080p60",
			"Sharp, smooth, autofocus that keeps up when you jump out of the chair.");
		I.Quality = 0.18;
		I.Rating = 4.5;
		I.Reviews = 9930;
	}
	{
		Item& I = Add("mirrorless", Category::Stream, Slot::Camera, Art::Mirrorless, 0x65a30d, "Mirrorless camera + capture card", "Halo Pro", 649.0, "Cinematic facecam",
			"Background melts into soft light. The camera the top channels use.");
		I.Quality = 0.27;
		I.Rating = 4.8;
		I.Reviews = 1870;
	}
	{
		Item& I = Add("mic-usb", Category::Stream, Slot::Mic, Art::MicUsb, 0xfbbf24, "USB microphone", "Corvid", 59.0, "Clear voice",
			"Plug it in and chat stops asking why you sound like you're in a tunnel.");
		I.Quality = 0.1;
		I.Rating = 4.5;
		I.Reviews = 31220;
	}
	{
		Item& I = Add("mic-xlr", Category::Stream, Slot::Mic, Art::MicXlr, 0xf59e0b, "Broadcast mic, arm + interface", "Corvid", 249.0, "Radio voice",
			"A dynamic mic on a boom arm. Warm, close, no keyboard clatter.");
		I.Quality = 0.18;
		I.Rating = 4.8;
		I.Reviews = 4410;
	}
	{
		Item& I = Add("ring-light", Category::Stream, Slot::Light, Art::RingLight, 0xfde68a, "Ring light", "Lumen", 35.0, "Lit face",
			"Even light on your face instead of the blue glow of a losing session.");
		I.Quality = 0.06;
		I.Rating = 4.2;
		I.Reviews = 40112;
	}
	{
		Item& I = Add("key-lights", Category::Stream, Slot::Light, Art::KeyLights, 0xfcd34d, "Pair of LED key lights", "Lumen", 219.0, "Studio lighting",
			"Two soft panels on desk arms. You look awake at 4 AM.");
		I.Quality = 0.12;
		I.Rating = 4.7;
		I.Reviews = 6012;
	}
	{
		Item& I = Add("green-screen", Category::Stream, Slot::None, Art::GreenScreen, 0x22c55e, "Collapsible green screen", "Lumen", 149.0, "You, keyed over the game",
			"The unmade bed disappears. Your face sits right on top of the tables.");
		I.Quality = 0.04;
		I.Rating = 4.3;
		I.Reviews = 7203;
	}
	{
		Item& I = Add("overlay-pack", Category::Stream, Slot::None, Art::Overlay, 0xe879f9, "Overlay and alert pack", "Glint", 49.0, "+25% follows",
			"Animated alerts, a rent goal bar, a webcam frame. New viewers stick around.");
		I.Quality = 0.04;
		I.Follow = 0.25;
		I.Rating = 4.6;
		I.Reviews = 2650;
	}
	// The apartment.
	{
		Item& I = Add("chair", Category::Home, Slot::None, Art::Chair, 0xfb923c, "Ergonomic chair", "Sitwell", 329.0, "-15% fatigue",
			"Lumbar support for twelve-hour sessions. The old kitchen chair goes back to the kitchen.");
		I.Drain = 0.15;
		I.Rating = 4.6;
		I.Reviews = 11930;
	}
	{
		Item& I = Add("espresso", Category::Home, Slot::None, Art::Espresso, 0xb45309, "Espresso machine", "Crema", 189.0, "-10% fatigue",
			"Real coffee at 3 AM. Fewer energy drinks on the desk.");
		I.Drain = 0.1;
		I.Rating = 4.5;
		I.Reviews = 8020;
	}
	{
		Item& I = Add("mattress", Category::Home, Slot::None, Art::Mattress, 0x818cf8, "Memory foam mattress", "Drift", 449.0, "+20% from sleep",
			"Eight hours that feel like eight hours.");
		I.Rest = 0.2;
		I.Rating = 4.7;
		I.Reviews = 19044;
	}
	{
		Item& I = Add("curtains", Category::Home, Slot::None, Art::Curtains, 0x6366f1, "Blackout curtains", "Drift", 59.0, "+10% from sleep",
			"Sleep through the morning like it's midnight.");
		I.Rest = 0.1;
		I.Rating = 4.4;
		I.Reviews = 9112;
	}
	{
		Item& I = Add("plant", Category::Home, Slot::None, Art::Plant, 0x22c55e, "Desk plant", "Greenroom", 24.0, "Tilt fades 5% faster",
			"Something alive on the desk that isn't a stack of cans.");
		I.Calm = 0.05;
		I.Rating = 4.8;
		I.Reviews = 1720;
	}
	// Subscriptions.
	{
		Item& I = Add("fiber", Category::Subscription, Slot::Net, Art::Router, 0x2dd4bf, "Fiber 500", "Northline", 69.99, "Upload for 1080p and up",
			"100 Mbps up. The old line tops out at a 720p stream; this one doesn't.");
		I.Monthly = true;
		I.Resolution = 1440;
		I.Quality = 0.04;
		I.Rating = 4.1;
		I.Reviews = 3380;
	}
	{
		Item& I = Add("gym", Category::Subscription, Slot::None, Art::Gym, 0xef4444, "Gym membership", "IronHouse", 34.99, "Tilt fades 25% faster \xC2\xB7 -5% fatigue",
			"Leave the bad beats on the bench press.");
		I.Monthly = true;
		I.Calm = 0.25;
		I.Drain = 0.05;
		I.Rating = 4.3;
		I.Reviews = 2140;
	}
	{
		Item& I = Add("meal-kit", Category::Subscription, Slot::None, Art::MealKit, 0x4ade80, "Meal kit delivery", "PrepBox", 79.99, "-10% fatigue",
			"Actual vegetables, on the doorstep twice a week.");
		I.Monthly = true;
		I.Drain = 0.1;
		I.Rating = 4.2;
		I.Reviews = 6630;
	}
	{
		Item& I = Add("modbot", Category::Subscription, Slot::None, Art::ModBot, 0x7c3aed, "Chat filter bot", "Warden", 7.99, "Catches 75% of trolls and spam",
			"An automated moderator for Kast chat. It never sleeps and it never feeds the trolls.");
		I.Monthly = true;
		I.ModBot = 0.75;
		I.Rating = 4.5;
		I.Reviews = 12030;
	}
	return L;
}
} // namespace gear_detail

const std::vector<Item>& Catalog()
{
	static const std::vector<Item> L = gear_detail::Build();
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

const char* CategoryName(Category C)
{
	switch (C)
	{
	case Category::Rig: return "Rig";
	case Category::Stream: return "Stream";
	case Category::Home: return "Home";
	default: return "Subscriptions";
	}
}

Effects Sum(const Owned& Items)
{
	Effects E;
	// The best item in each slot counts; everything else adds up.
	std::map<Slot, const Item*> Best;
	int PcRes = 480;
	int NetRes = 720;
	for (const auto& Entry : Items)
	{
		const Item* I = Find(Entry.first);
		if (!I)
		{
			continue;
		}
		if (I->Where != Slot::None)
		{
			const Item*& B = Best[I->Where];
			if (!B || I->Quality + static_cast<double>(I->Resolution) * 0.0001 + I->Tables > B->Quality + static_cast<double>(B->Resolution) * 0.0001 + B->Tables)
			{
				B = I;
			}
			continue;
		}
		E.Quality += I->Quality;
		E.Drain += I->Drain;
		E.Rest += I->Rest;
		E.Calm += I->Calm;
		E.Follow += I->Follow;
		E.ModBot = std::max(E.ModBot, I->ModBot);
		E.GreenScreen = E.GreenScreen || I->Id == "green-screen";
		E.Overlay = E.Overlay || I->Id == "overlay-pack";
		E.MacroPad = E.MacroPad || I->Id == "macro-pad";
		E.MonthlyCents += I->Monthly ? I->PriceCents : 0;
	}
	for (const auto& Pair : Best)
	{
		const Item& I = *Pair.second;
		E.Quality += I.Quality;
		E.Tables += I.Tables;
		E.MonthlyCents += I.Monthly ? I.PriceCents : 0;
		switch (Pair.first)
		{
		case Slot::Camera: E.CamTier = I.Id == "webcam-720" ? 1 : I.Id == "webcam-1080" ? 2 : 3; break;
		case Slot::Mic: E.MicTier = I.Id == "mic-usb" ? 1 : 2; break;
		case Slot::Light: E.Lights = I.Id == "ring-light" ? 1 : 2; break;
		case Slot::Pc:
			PcRes = I.Resolution;
			E.StreamTables = I.StreamTables;
			E.PcTier = I.Id == "ram-32" ? 1 : I.Id == "tower-mid" ? 2 : 3;
			break;
		case Slot::Net:
			NetRes = I.Resolution;
			E.Fiber = true;
			break;
		default: break;
		}
	}
	// The laptop's own camera and mic are something, barely.
	E.Quality += 0.03;
	E.Resolution = std::min(PcRes, NetRes);
	const double ResFactor = E.Resolution >= 1440 ? 1.08 : E.Resolution >= 1080 ? 1.0 : E.Resolution >= 720 ? 0.85 : 0.7;
	E.Quality = std::min(1.0, E.Quality * ResFactor);
	E.Drain = std::min(0.45, E.Drain);
	E.Rest = std::min(0.4, E.Rest);
	E.Calm = std::min(0.6, E.Calm);
	return E;
}

const Item& FirstPcUpgrade()
{
	const Item* Best = nullptr;
	for (const Item& I : Catalog())
	{
		if (I.Where == Slot::Pc && I.Requires.empty() && (!Best || I.PriceCents < Best->PriceCents))
		{
			Best = &I;
		}
	}
	return *Best;
}

std::string ResolutionLabel(int Resolution)
{
	return std::to_string(Resolution) + (Resolution >= 1080 ? "p60" : "p30");
}
} // namespace gear
} // namespace ss
