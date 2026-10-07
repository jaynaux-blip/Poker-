// Northside's shared plan for the street's stages (ue-world; world realism spec 0.3-0.5): the named lines of the stage
// frame, the kit's mesh frames, the road's crown, the hours shops keep, and small helpers the street (StreetStage*.cpp),
// Market (MarketStage.cpp) and the traffic (StreetTraffic.cpp) share. Everything is in AStreetStage's frame, centimeters:
// +X west, +Y north, Z 0 the sidewalk's top, Z -15 the road's edge.
#pragma once

#include "BlenderProps.h"
#include "Components/LightComponent.h" // FLit keeps TWeakObjectPtr<ULightComponent>: incomplete, it fails C2338 in any includer
#include "CoreMinimal.h"
#include "ShortStack/Game/Shop.h"

class FWidgetRenderer;
class ULightComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;
class UStaticMeshComponent;
class UTextureRenderTarget2D;

namespace Ns
{
// ------------------------------------------------------------------ the plan (spec 0.3)
constexpr double HomeLine = 0.0;
constexpr double RoadX0 = 420.0;
constexpr double RoadX1 = 1620.0;
constexpr double FarFront = 2040.0;
constexpr double FifthMidX = 1020.0;
constexpr double HollisN = -6200.0;
constexpr double HollisRoad0 = -6620.0;
constexpr double HollisRoad1 = -7620.0;
constexpr double HollisS = -8040.0;
constexpr double WexlerS = 9200.0;
constexpr double WexlerRoad0 = 9620.0;
constexpr double WexlerRoad1 = 10620.0;
constexpr double WexlerN = 11040.0;
constexpr double StoreY0 = 3700.0;
constexpr double StoreY1 = 4950.0;
constexpr double MarketS = 5400.0;
constexpr double MarketN = 6500.0;
constexpr double MarketLineN = 6950.0;
constexpr double MarketMidY = 5950.0;
constexpr double MarketEast = -9600.0;
constexpr double MarketSplitS = -1400.0;
constexpr double MarketSplitN = -1500.0;
constexpr double CardinalRoad0 = -10020.0;
constexpr double CardinalRoad1 = -11020.0;
constexpr double CardinalMidX = -10520.0;
constexpr double CardinalE = -11440.0;
// Market as The Block built it (its crowd still walks these lines until it reads the registry's walkways, spec 6.8).
constexpr double OldMarketS = 5250.0;
constexpr double OldMarketN = 5850.0;
constexpr double OldMarketLineN = 6150.0;
constexpr double OldMarketWest = -3000.0;
// The walk limits (spec 2.9): each cross street's near sidewalk ends at a closure.
constexpr double HollisWalk0 = -1800.0;
constexpr double HollisWalk1 = 3840.0;
constexpr double WexlerWalk0 = -1500.0;
constexpr double WexlerWalk1 = 3840.0;
constexpr double CardinalWalk0 = 3400.0;
constexpr double CardinalWalk1 = 8400.0;
// The cross streets' off-set ends (spec 2.7): 60 m either side of their T, behind the proxies' haze.
constexpr double PassReach = 6000.0;
// How far the proxies line the cross streets (spec 2.3): 120 m from the T.
constexpr double ProxyReach = 12000.0;
// Where the cross streets' ground ends in the haze (ns_ground.py's Far: Hollis and Wexler X -90..130.2 m, Cardinal Y -50..170 m),
// each end closed by a row of stand-in fronts across it, so no look down a street ends in nothing (spec 1's "no stage set").
constexpr double CrossEast = -9000.0;
constexpr double CrossWest = FifthMidX + ProxyReach;
constexpr double CardinalSouth = -5000.0;
constexpr double CardinalNorth = 17000.0;
// The backs of Cardinal's west-side fronts (MarketEast, 12 m deep): Hollis's and Wexler's rows stop there, and those fronts,
// running on across both cross streets, close the views east down them.
constexpr double CardinalBacks = MarketEast + 1200.0;
// How deep a stand-in front's massing runs behind its face (the proxies are 12 m deep, ns_fronts.py).
constexpr double ProxyDepth = 1200.0;

/** The road's top at D cm from its centerline on a road W cm curb to curb (spec 4.1's crown: +3 cm in the middle). */
inline double RoadZ(double D, double W)
{
	const double T = 2.0 * D / FMath::Max(1.0, W);
	return -15.0 + 3.0 * (1.0 - FMath::Min(1.0, T * T));
}

/**
 * The modelled road's top at X, Y (stage cm) as ns_ground.py's road_crown_z lays it: every street crowned 3 cm, the
 * through streets (Hollis, Wexler, Cardinal) keeping theirs across the intersections, Fifth's fading out over the 6 m
 * before each T and Market's over the 6 m before Fifth and before Cardinal. -15 off the roads (their edges; the
 * sidewalks are the caller's). bWideMarket: Market at its new width (else The Block's, crowned the same way).
 */
inline double RoadCrownZ(double X, double Y, bool bWideMarket)
{
	auto Crown = [](double D, double W) {
		const double T = 2.0 * D / FMath::Max(1.0, W);
		return FMath::Max(0.0, 1.0 - T * T);
	};
	auto Smooth = [](double E0, double E1, double V) { return static_cast<double>(FMath::SmoothStep(0.0f, 1.0f, static_cast<float>((V - E0) / (E1 - E0)))); };
	double C = 0.0;
	if (X >= RoadX0 - 2.0 && X <= RoadX1 + 2.0 && Y > HollisRoad0 - 100.0 && Y < WexlerRoad0 + 100.0)
	{
		C = FMath::Max(C, Crown(X - FifthMidX, RoadX1 - RoadX0) * Smooth(HollisRoad0, HollisRoad0 + 600.0, Y) * Smooth(WexlerRoad0, WexlerRoad0 - 600.0, Y));
	}
	for (const double Mid : {(HollisRoad0 + HollisRoad1) * 0.5, (WexlerRoad0 + WexlerRoad1) * 0.5})
	{
		if (FMath::Abs(Y - Mid) <= 502.0)
		{
			C = FMath::Max(C, Crown(Y - Mid, 1000.0));
		}
	}
	const double S = bWideMarket ? MarketS : 5250.0;
	const double N = bWideMarket ? MarketN : 5850.0;
	const double East = bWideMarket ? CardinalRoad0 : -3000.0;
	if (Y >= S - 2.0 && Y <= N + 2.0 && X < RoadX0 + 2.0 && X > East - 2.0)
	{
		C = FMath::Max(C, Crown(Y - (S + N) * 0.5, N - S) * Smooth(RoadX0, RoadX0 - 600.0, X) * (bWideMarket ? Smooth(CardinalRoad0, CardinalRoad0 + 600.0, X) : 1.0));
	}
	if (bWideMarket && FMath::Abs(X - CardinalMidX) <= 502.0)
	{
		C = FMath::Max(C, Crown(X - CardinalMidX, 1000.0));
	}
	return -15.0 + 3.0 * C;
}

// ------------------------------------------------------------------ the mesh frames (spec 0.4)

/** A PROP mesh of the NS kit (front toward Blender -X) turned to face stage yaw Yaw: Yaw + 180 with the importer's mapping. */
inline FRotator PropFacing(float Yaw)
{
	return FRotator(0.0f, BlenderImportYaw() + Yaw + 90.0f, 0.0f);
}

/** A SITE mesh (Blender's origin at the stage's): yaw 180 at the stage's origin. */
inline FRotator SiteRotation()
{
	return FRotator(0.0f, 180.0f, 0.0f);
}

/** Blender meters (SITE) to stage cm. */
inline FVector StageOf(const FVector& Blender)
{
	return FVector(-100.0 * Blender.X, 100.0 * Blender.Y, 100.0 * Blender.Z);
}

// ------------------------------------------------------------------ hours

/** When a shop is lit (spec 2.3's Lit column): minutes after midnight, Closes <= Opens wraps past midnight, Days bit 0 Monday. Opens < 0: never; Opens == Closes: all day. */
struct FHours
{
	int32 Opens = 0;
	int32 Closes = 1440;
	int32 Days = 0x7f;
};

/** Whether H is open at World (world minutes since Monday 00:00). */
inline bool IsOpenAt(const FHours& H, double World)
{
	if (H.Opens < 0)
	{
		return false;
	}
	if (H.Opens == H.Closes)
	{
		return true;
	}
	const double Day = FMath::FloorToDouble(World / 1440.0);
	const int32 Minute = static_cast<int32>(World - Day * 1440.0);
	const int32 Weekday = ((static_cast<int32>(Day) % 7) + 7) % 7;
	auto DayOk = [&H](int32 W) { return (H.Days >> (((W % 7) + 7) % 7) & 1) != 0; };
	if (H.Closes > H.Opens)
	{
		return DayOk(Weekday) && Minute >= H.Opens && Minute < H.Closes;
	}
	// Past midnight: the evening's part by today's day, the small hours' by yesterday's.
	return (DayOk(Weekday) && Minute >= H.Opens) || (DayOk(Weekday - 1) && Minute < H.Closes);
}

// ------------------------------------------------------------------ small things

/** A stable 0..1 from a seed (the windows, the cars, the bulbs). */
inline float Hash01(uint32 X)
{
	X ^= X >> 16;
	X *= 0x7feb352du;
	X ^= X >> 15;
	X *= 0x846ca68bu;
	X ^= X >> 16;
	return static_cast<float>(X & 0xffffff) / static_cast<float>(0xffffff);
}

inline FLinearColor Srgb(uint32 Hex)
{
	return FLinearColor::FromSRGBColor(FColor(static_cast<uint8>((Hex >> 16) & 0xff), static_cast<uint8>((Hex >> 8) & 0xff), static_cast<uint8>(Hex & 0xff)));
}

// ------------------------------------------------------------------ the stand-in fronts down the cross streets (spec 2.3)

/** SM_NS_Proxy_A..H's widths (cm, ns_fronts.py) and floors. */
inline constexpr double ProxyWidths[8] = {2400.0, 2800.0, 2000.0, 2600.0, 2200.0, 3000.0, 2500.0, 2100.0};
inline constexpr int32 ProxyFloors[8] = {5, 7, 4, 6, 4, 5, 6, 5};
inline constexpr uint32 ProxyBricks[8] = {0x7a3a2a, 0x9c8160, 0xb9b2a3, 0x2f2420, 0x7a3a2a, 0x8d877c, 0x6b3427, 0x9c8160};

/** One front of a run: its letter (0..7, A..H), its middle's distance along the run, its width there (cm), mirrored or not. */
struct FProxySlot
{
	int32 Letter = 0;
	double Along = 0.0;
	double Width = 0.0;
	bool bMirror = false;
};

/**
 * Fronts side by side filling a building line Length cm long exactly: the letters in a seeded order that never repeats a
 * neighbor, every other one mirrored (they carry no lettering, ns_fronts.py), each squeezed or stretched a little so the run
 * ends where its street does (never past it, into a road or a neighbor's corner). A short run is the one front nearest its
 * width.
 */
inline TArray<FProxySlot> ProxyPlan(double Length, int32 Seed)
{
	TArray<FProxySlot> Out;
	if (Length < 600.0)
	{
		return Out;
	}
	if (Length < 3200.0)
	{
		int32 Best = 0;
		for (int32 K = 1; K < 8; ++K)
		{
			Best = FMath::Abs(ProxyWidths[K] - Length) < FMath::Abs(ProxyWidths[Best] - Length) ? K : Best;
		}
		FProxySlot S;
		S.Letter = Best;
		S.Width = Length;
		S.Along = Length * 0.5;
		S.bMirror = (Seed & 1) != 0;
		Out.Add(S);
		return Out;
	}
	double Sum = 0.0;
	int32 Last = -1;
	for (int32 N = 0; Sum < Length - 1.0; ++N)
	{
		int32 K = static_cast<int32>(Hash01(static_cast<uint32>(Seed * 131 + N * 17)) * 7.999f);
		if (K == Last)
		{
			K = (K + 3) % 8;
		}
		Last = K;
		FProxySlot S;
		S.Letter = K;
		S.Width = ProxyWidths[K];
		S.bMirror = N % 2 == 1;
		Out.Add(S);
		Sum += S.Width;
	}
	// Keep the last one or not, whichever leaves the fronts nearer their own widths.
	const double Without = Sum - Out.Last().Width;
	if (Out.Num() > 1 && FMath::Abs(FMath::Loge(Length / Without)) < FMath::Abs(FMath::Loge(Length / Sum)))
	{
		Out.Pop();
		Sum = Without;
	}
	const double Scale = Length / Sum;
	double At = 0.0;
	for (FProxySlot& S : Out)
	{
		S.Width *= Scale;
		S.Along = At + S.Width * 0.5;
		At += S.Width;
	}
	return Out;
}

/** The glow families (spec 0.5): what a _Glow<Family> mesh is and how the hour sets it. */
enum class EGlow : uint8
{
	Warm,
	Cool,
	Neon,
	Signal,
	Lamps,
	Chase,
};

/**
 * The hour's glow levels by family (nits, set for the exposure the sun gives: ApplyHour), and how far into the night it is
 * (0 day .. 1 night: the lamps' photocells) and how much the exposure has closed down since night (a multiple).
 */
struct FGlowLevels
{
	float Warm = 25.0f;
	float Cool = 30.0f;
	float Neon = 45.0f;
	float Signal = 120.0f;
	float Lamps = 150.0f;
	float Chase = 40.0f;
	float Night = 1.0f;
	float Exposure = 1.0f;
	/** The [lit] displays behind shop glass (DimImportedGlow's level, as baked = 1). */
	float LitDisplays = 1.0f;
};

/** A shop of a plot (spec 2.3's Lit): what lights with it by its hours. */
struct FShop
{
	FHours Hours;
	/** Its venue in core's table (shop::Venues: the display shops' and the enterable shops' real hours), or null: Hours. */
	const ss::shop::Venue* Venue = nullptr;
	bool OpenAt(double World) const { return Venue ? ss::shop::IsOpen(*Venue, World) : IsOpenAt(Hours, World); }
	TArray<TWeakObjectPtr<ULightComponent>> Lights;
	TArray<float> Levels;
	/** Its trim's lit displays (the [lit] slot, 3: scaled with the hours), its grille (rolled up into its hood while open). */
	TWeakObjectPtr<UStaticMeshComponent> Trim;
	int32 TrimSlot = 3;
	TWeakObjectPtr<USceneComponent> Grille;
	float GrilleOpen = 1.0f;
	/** As last applied: 1 open, 0 shut, -1 not yet. */
	int8 Open = -1;
	/** As last applied: the night level the lights were set for. */
	float NightApplied = -1.0f;
};

/** A glow mesh (or a stand-in's glowing part) lit by its family's level, with its shop's hours or at night. */
struct FGlow
{
	TWeakObjectPtr<UMaterialInstanceDynamic> Mid;
	TWeakObjectPtr<UStaticMeshComponent> Mesh;
	EGlow Family = EGlow::Warm;
	FLinearColor Color = FLinearColor::White;
	float Scale = 1.0f;
	/** Its shop (NsLit's Shops), or -1: lit at night whatever the hour's shops do. */
	int32 Shop = -1;
	/** Lit by day too (an open shop's neon, the lobby's canopy): else only from dusk to dawn. */
	bool bDay = false;
	/** M_NS_Glow (its Mode and LitA/LitB set by whoever drives it), or an imported/M_Street glow scaled. */
	bool bNsGlow = false;
	/** An M_Street part (a stand-in's lit window, a sign's tube): its Emissive set to the level (its color is its base color). */
	bool bStreetEmissive = false;
	/** The level last set (set again only when it moves). */
	float Applied = -1.0f;
	TArray<TPair<FName, FLinearColor>> Colors;
	TArray<TPair<FName, float>> Strengths;
};

/** The street's lit things by the hour: the plots' shops and every glow. The street and Market each keep one. */
class FLit
{
public:
	TArray<FShop> Shops;
	TArray<FGlow> Glows;

	void Reset()
	{
		Shops.Reset();
		Glows.Reset();
	}
	int32 AddShop(const FHours& H)
	{
		FShop S;
		S.Hours = H;
		return Shops.Add(S);
	}
	/** A shop that keeps its venue's hours (core's: "kims", "pharmacy"...), or H when core doesn't know it (null or ""). */
	int32 AddShop(const TCHAR* VenueId, const FHours& H)
	{
		FShop S;
		S.Hours = H;
		S.Venue = VenueId && *VenueId ? ss::shop::FindVenue(TCHAR_TO_UTF8(VenueId)) : nullptr;
		return Shops.Add(S);
	}
	/** A light of Shop's (its spill, a sign's light): on at Level while the shop is open after dusk. */
	void AddLight(int32 Shop, ULightComponent* L, float Level);
	/**
	 * Mesh made to glow (its every slot, or the one named Slot): M_NS_Glow in Color (GlowMaterial non-null), or its own
	 * emissive parameters scaled (an imported glow mesh without M_NS_Glow, an M_Street part). Outer owns the material made.
	 */
	int32 AddGlow(UObject* Outer, UStaticMeshComponent* Mesh, UMaterialInterface* GlowMaterial, EGlow Family, const FLinearColor& Color, float Scale, int32 Shop, bool bDay);
	bool IsOpen(int32 Shop, double World) const { return Shops.IsValidIndex(Shop) && Shops[Shop].OpenAt(World); }
	/** Each shop open or shut (its lights, its grille, its displays) and each glow at its family's level for the hour. */
	void Update(double World, const FGlowLevels& Levels, float Dt, bool bForce);
};

/** How a [screen] face shows (FScreens): printed (lit by the street), backlit (it shines by itself, by the hour), or a light box's bulbs. */
enum class EScreen : uint8
{
	Printed,
	Backlit,
	Bulbs,
};

/**
 * The street's [screen] faces (spec 0.5): core's ShopArt (ss::ui::shopart: a plate, the 14's timetable, the shelter's ad, the
 * Rialto's marquee, an awning's valance) drawn into a render target through Slate, as the Embercrest's screens are, and shown
 * on the mesh's screen slot by M_NS_Screen (its picture over the slot's own color where the face is clear: an awning's
 * fabric). The faces that change with the clock (the timetable's next buses, the marquee on Fridays) are drawn again once
 * a world minute; the backlit ones shine by the hour's glow levels. Each stage keeps one; its meshes hold the materials
 * (and so the targets) alive.
 */
class FScreens
{
public:
	FScreens() = default;
	FScreens(const FScreens&) {}
	FScreens& operator=(const FScreens&) { return *this; }
	~FScreens() { Reset(); }

	/**
	 * Mesh's slot named Slot (its imported slot name, or its material's) shows Key ("plate:KRT 4471", "timetable:market-east",
	 * "shelterad", "marquee", "valance:kims"), Pixels wide (its height by the face's proportions), Back behind its clear parts.
	 * Outer owns the material and the target. False (the slot keeps the art's default picture) when there's no such slot,
	 * no M_NS_Screen, nothing to draw, or no Slate to draw with (a server, a commandlet).
	 */
	bool Paint(UObject* Outer, UStaticMeshComponent* Mesh, const TCHAR* Slot, const FString& Key, int32 Pixels, EScreen How, const FLinearColor& Back = FLinearColor(0.5f, 0.5f, 0.5f));
	/** The clock's faces drawn again when the world's minute moves (World: the look's minutes), the backlit ones by Levels. */
	void Update(double World, const FGlowLevels& Levels);
	/** Every face forgotten and the renderer let go (the stage built again, or gone). */
	void Reset();
	int32 Num() const { return Faces.Num(); }

private:
	struct FFace
	{
		TWeakObjectPtr<UMaterialInstanceDynamic> Mid;
		TWeakObjectPtr<UTextureRenderTarget2D> Target;
		FString Key;
		EScreen How = EScreen::Printed;
		bool bClock = false;
		float Applied = -1.0f;
	};
	bool Draw(FFace& Face, double World);
	TArray<FFace> Faces;
	FWidgetRenderer* Renderer = nullptr;
	int64 DrawnMinute = TNumericLimits<int64>::Lowest();
	/** When the clock's faces were last drawn (real seconds). */
	double LastDraw = 0.0;
	double LastWorld = 22.0 * 60.0;
};

/** A plate's text for car Index of a stage's set (Set: 1 Fifth's parked, 2 Market's, 3 the traffic's): the same every load, no two alike. */
FString PlateFor(int32 Set, int32 Index);

/** The runtime materials' paths (ns_setup.py). */
inline const TCHAR* const MpcPath = TEXT("/Game/ShortStack/Materials/MPC_NS.MPC_NS");
inline const TCHAR* const GlowPath = TEXT("/Game/ShortStack/Materials/M_NS_Glow.M_NS_Glow");
/** M_NS_Glow's code in an additive, unlit material: the cars' lamp films (their off cells add nothing over the baked lenses). */
inline const TCHAR* const GlowAddPath = TEXT("/Game/ShortStack/Materials/M_NS_GlowAdd.M_NS_GlowAdd");
inline const TCHAR* const ScreenPath = TEXT("/Game/ShortStack/Materials/M_NS_Screen.M_NS_Screen");
inline const TCHAR* const GlassPath = TEXT("/Game/ShortStack/Materials/M_NS_Glass.M_NS_Glass");
inline const TCHAR* const WindowPath = TEXT("/Game/ShortStack/Materials/M_NS_WindowRoom.M_NS_WindowRoom");
inline const TCHAR* const SkyGlowPath = TEXT("/Game/ShortStack/Materials/M_NS_SkyGlow.M_NS_SkyGlow");
inline const TCHAR* const RainFxPath = TEXT("/Game/ShortStack/Materials/M_NS_RainFx.M_NS_RainFx");
inline const TCHAR* const CloudPath = TEXT("/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst.m_SimpleVolumetricCloud_Inst");

/** A decal cell's instance (MI_NS_Decal_00..15). */
inline FString DecalPath(int32 Cell)
{
	return FString::Printf(TEXT("/Game/ShortStack/Materials/NS/MI_NS_Decal_%02d.MI_NS_Decal_%02d"), Cell, Cell);
}
} // namespace Ns
