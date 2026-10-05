// The Embercrest's exterior: the plan every Embercrest*.cpp shares (the map frame of the design spec, ec_exterior_spec.md
// section 0-2), and the small helpers its stage files use. Map cm unless a name says meters. +X is west (out of the doors
// toward the bus stop and the valley), +Y north, +Z up; the origin is the doors' threshold, at the top of the apron.
#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInterface.h"
#include "Math/UnrealMathUtility.h"

namespace Ecx
{
// ------------------------------------------------------------------ elevations (spec 2.2)
constexpr double ZApron = 0.0;
constexpr double ZLane = -12.0;
constexpr double ZLandB = -75.0;
constexpr double ZLandA = -150.0;
constexpr double ZPlatform = -225.0;
constexpr double ZRoad = -240.0;
constexpr double ZOverlook = -225.0;

// ------------------------------------------------------------------ the plan's lines along X (spec 2.3)
constexpr double XApron1 = 800.0;  // the apron, X 0..800
constexpr double XLane1 = 1650.0;  // the drop-off and through lanes, X 800..1650
constexpr double XTop1 = 2320.0;   // the top landing, X 1650..2320
constexpr double XFlight3 = 2480.0; // each flight's bottom riser; it climbs 160 cm toward -X
constexpr double XFlight2 = 3640.0;
constexpr double XFlight1 = 4800.0;
constexpr double XPlatform1 = 5250.0; // the bus platform, X 4800..5250
constexpr double XBay1 = 5600.0;      // the bay, X 5250..5600
constexpr double XRoad1 = 6300.0;     // the ridge road, X 5600..6300
constexpr double XParapet = 6400.0;   // the parapet's inner face
constexpr double XOverlook1 = 7000.0;
constexpr double WalkHalf = 350.0;    // the axial walk, Y -350..350
constexpr double FlightHalf = 400.0;  // a flight with its cheeks, 8 m wide
constexpr double CanopyX1 = 2430.0;   // the canopy, X 0..2430, Y +-1820
constexpr double CanopyHalf = 1820.0;
constexpr double ArmY = 3000.0;       // the drive arms' centerlines (south in, north out), 7 m wide
constexpr double ArmHalf = 350.0;

// ------------------------------------------------------------------ the bus (spec 3.3, 5.2)
/** Where the 14 stops: its middle on the road, heading north. */
constexpr double BusStopX = 5395.0;
constexpr double BusStopY = -428.0;
/** Bus frame, meters as Blender has it (front toward -x, the curb side +y), to the bus's own UE cm (Y mirrored). */
inline FVector BusLocal(double Bx, double By, double Bz)
{
	return FVector(Bx * 100.0, -By * 100.0, Bz * 100.0);
}

// ------------------------------------------------------------------ the plaza's things (spec 2.5), map cm

/** The fire bowls' plinth bases (each stands in its cascade's water). */
inline const FVector FireBowls[6] = {FVector(4930.0, 530.0, -225.0), FVector(4930.0, -530.0, -225.0), FVector(4100.0, 975.0, -150.0), FVector(4100.0, -975.0, -150.0),
	FVector(3060.0, 975.0, -75.0), FVector(3060.0, -975.0, -75.0)};
/** The lamp posts' bases (Z: the ground there). */
inline const FVector LampPosts[16] = {FVector(2400.0, -3430.0, 0.0), FVector(3200.0, -3430.0, 0.0), FVector(4000.0, -3430.0, 0.0), FVector(4800.0, -3430.0, 0.0),
	FVector(2400.0, 3430.0, 0.0), FVector(3200.0, 3430.0, 0.0), FVector(4000.0, 3430.0, 0.0), FVector(4800.0, 3430.0, 0.0), FVector(2100.0, -2250.0, 0.0),
	FVector(2100.0, 2250.0, 0.0), FVector(5160.0, -1750.0, -225.0), FVector(5160.0, 1050.0, -225.0), FVector(6960.0, 660.0, -225.0), FVector(6960.0, 1940.0, -225.0),
	FVector(6370.0, -3000.0, -240.0), FVector(6370.0, 4000.0, -240.0)};
/** The canopy's downlights (lens at Z 698). */
inline const double DownlightX[4] = {450.0, 1000.0, 1550.0, 2100.0};
inline const double DownlightY[6] = {-1500.0, -900.0, -300.0, 300.0, 900.0, 1500.0};
/** The canopy's tree columns. */
inline const FVector2D CanopyColumns[4] = {FVector2D(1850.0, -1500.0), FVector2D(1850.0, -600.0), FVector2D(1850.0, 600.0), FVector2D(1850.0, 1500.0)};
/** The swing door hinges (spec 2.4: south pair Y -460..-250, north pair +250..+460), south to north; +1 built as modeled, -1 mirrored. */
inline const double SwingHingeY[4] = {-460.0, -250.0, 250.0, 460.0};
inline const double SwingMirror[4] = {1.0, -1.0, 1.0, -1.0};

/** The ground's height (Z) where the drive arms run (a straight grade from -12 at X 2125 to the road at X 5600). */
inline double ArmZ(double X)
{
	return FMath::Lerp(ZLane, ZRoad, FMath::Clamp((X - 2125.0) / (XBay1 - 2125.0), 0.0, 1.0));
}

/** The road's height along it (spec 2.2: down 4% south of Y -4200, up 2% north of Y +6000). */
inline double RoadZ(double Y)
{
	if (Y < -4200.0)
	{
		return ZRoad - 0.04 * (-4200.0 - Y);
	}
	if (Y > 6000.0)
	{
		return ZRoad + 0.02 * (Y - 6000.0);
	}
	return ZRoad;
}

/** The plaza's walkable height under a point (the hero's floor, roughly; the ground mesh or the block-out decides). */
inline double PlazaZ(double X, double Y)
{
	const double AY = FMath::Abs(Y);
	if (X >= XParapet - 100.0)
	{
		return X > XParapet + 45.0 ? ZOverlook : RoadZ(Y);
	}
	if (AY > 2650.0 && X > 2125.0)
	{
		return ArmZ(X);
	}
	if (X < XApron1)
	{
		return ZApron;
	}
	if (X < XLane1)
	{
		return AY <= WalkHalf ? ZApron : ZLane;
	}
	if (X < XTop1)
	{
		return ZApron;
	}
	if (X < XFlight3)
	{
		return FMath::Lerp(ZApron, ZLandB, (X - XTop1) / 160.0);
	}
	if (X < XFlight2 - 160.0)
	{
		return ZLandB;
	}
	if (X < XFlight2)
	{
		return FMath::Lerp(ZLandB, ZLandA, (X - (XFlight2 - 160.0)) / 160.0);
	}
	if (X < XFlight1 - 160.0)
	{
		return ZLandA;
	}
	if (X < XFlight1)
	{
		return FMath::Lerp(ZLandA, ZPlatform, (X - (XFlight1 - 160.0)) / 160.0);
	}
	if (X < XPlatform1)
	{
		return ZPlatform;
	}
	return RoadZ(Y);
}

// ------------------------------------------------------------------ the hour's rules for lights and glows

/** When a local light shines (FEcxLight::Rule). */
enum EEcxRule : uint8
{
	RuleAlways = 0, // at every hour (the lobby's spill, the grill)
	RuleDimByDay,   // always, at 40% by day (the canopy's downlights)
	RuleNight,      // from dusk (below +4 degrees of sun) through the night
	RuleFire,       // the fire bowls: always, flickering
	RuleCrown,      // inside the crown: dusk and night, the beacon
	RuleManual,     // set by its owner (the bus, the marquee's spill)
};

/** Which light a glow material takes (FEcxGlow::Family). */
enum EEcxGlow : uint8
{
	GlowEmber = 0, // the brand's LED lines (0xff6a24)
	GlowWarm,      // downlight lenses and coves, 2700-3000 K (always lit, low by day)
	GlowCool,      // transit lighting, 4300 K
	GlowAsh,       // the lit letter faces
	GlowCrown,     // the crown's ember, brighter than the podium's
	GlowLantern,   // the lamp posts' panes and the step lights: on from dusk
	GlowSlope,     // the far slope's lamps and chalets: off by day
	GlowInterior,  // the lobby's chandelier and coves (seen through the glass at every hour)
	GlowCount,
};

// ------------------------------------------------------------------ colors (spec 0.5)
constexpr uint32 EmberHex = 0xff6a24;
constexpr uint32 WarmHex = 0xffc98a;
constexpr uint32 WarmPaleHex = 0xffead2;
constexpr uint32 CoolHex = 0xe2ecff;
constexpr uint32 AshHex = 0xebe7df;
constexpr uint32 BasaltHex = 0x26282d;
constexpr uint32 BronzeHex = 0x5a3e2a;
constexpr uint32 CopperHex = 0xb8743f;
constexpr uint32 TimberHex = 0x98653a;
constexpr uint32 GraniteHex = 0x8c8a86;
constexpr uint32 GraniteWetHex = 0x5e5c59;
constexpr uint32 FrameDarkHex = 0x15161a;
constexpr uint32 SnowHex = 0xe8edf5;
constexpr uint32 MidnightHex = 0x0a1020;
constexpr uint32 NavyHex = 0x121c33;
constexpr uint32 GoldHex = 0xf6b04a;

inline FLinearColor Srgb(uint32 Hex)
{
	return FLinearColor::FromSRGBColor(FColor(static_cast<uint8>((Hex >> 16) & 0xff), static_cast<uint8>((Hex >> 8) & 0xff), static_cast<uint8>(Hex & 0xff)));
}

/** Rec. 709 luminance of a linear color (a glow's strength is set so that its luminance is the nits asked for). */
inline float Luma(const FLinearColor& C)
{
	return FMath::Max(0.02f, 0.2126f * C.R + 0.7152f * C.G + 0.0722f * C.B);
}

// ------------------------------------------------------------------ assets

/** A mesh from the Blender pipeline (unreal/Art/Meshes, imported when the editor opens), nullptr when it isn't. */
inline UStaticMesh* LoadMesh(const TCHAR* Name)
{
	return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/ShortStack/Meshes/%s/%s.%s"), Name, Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
}

inline UMaterialInterface* LoadMat(const TCHAR* Name)
{
	return LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/ShortStack/Materials/%s.%s"), Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
}

/** One of the exterior's textures (unreal/Art/Textures/ECX), or the brand's (Brand), nullptr when it isn't imported. */
inline UTexture* LoadTex(const TCHAR* Group, const TCHAR* Name)
{
	return LoadObject<UTexture>(nullptr, *FString::Printf(TEXT("/Game/ShortStack/Textures/%s/%s.%s"), Group, Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
}

// ------------------------------------------------------------------ the sun (spec 4.1)

/** The sun's elevation (degrees) at a world hour (0..24), the season fixed in early October. */
inline double SunElevation(double Hour)
{
	const double H = FMath::Fmod(FMath::Fmod(Hour, 24.0) + 24.0, 24.0);
	if (H < 3.45 || H > 21.25)
	{
		return -18.0;
	}
	return 40.0 * FMath::Sin(UE_DOUBLE_PI * (H - 5.45) / 13.8);
}

/** The sun's compass azimuth (degrees, 0 north, 90 east) at a world hour. */
inline double SunAzimuth(double Hour)
{
	const double H = FMath::Fmod(FMath::Fmod(Hour, 24.0) + 24.0, 24.0);
	return 97.0 + 166.0 * FMath::Clamp((H - 5.45) / 13.8, 0.0, 1.0);
}

/** 0 above Lo .. 1 below Hi (degrees of sun elevation, Lo > Hi): how far into the evening the sky has gone. */
inline float BelowEl(double El, double Lo, double Hi)
{
	return static_cast<float>(FMath::Clamp((Lo - El) / FMath::Max(0.001, Lo - Hi), 0.0, 1.0));
}

/** A smooth value noise in one dimension (the fires' flicker): -1..1. */
inline float Noise1(double X, uint32 Seed)
{
	auto Hash = [Seed](int64 I) {
		uint32 H = static_cast<uint32>(I) * 0x9E3779B1u ^ Seed * 0x85EBCA77u;
		H ^= H >> 15;
		H *= 0x2C1B3C6Du;
		H ^= H >> 12;
		return static_cast<float>(H & 0xffffff) / static_cast<float>(0xffffff) * 2.0f - 1.0f;
	};
	const int64 I = static_cast<int64>(FMath::FloorToDouble(X));
	const float F = static_cast<float>(X - static_cast<double>(I));
	const float U = F * F * (3.0f - 2.0f * F);
	return FMath::Lerp(Hash(I), Hash(I + 1), U);
}
} // namespace Ecx
