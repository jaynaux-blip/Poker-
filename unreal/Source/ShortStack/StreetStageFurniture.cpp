// Fifth's furniture (world realism spec 2.5, 2.7, 2.8, 3.1, 3.6, ue-world): the lamps (Fifth's twelve HPS heads, two on each
// cross street, mixed with the LED cobra heads), the corner of Fifth & Market's signals and walk heads, the street blades,
// the signs, hydrants, cans and newsboxes, the trees in their pits, awnings over the shops that have them, the parked cars
// (the car kit in many paints, wheels and all), decals where the street has been used, and where its sounds come from.
// Placed from these tables with fixed seeds, so the set is the same every load. Market's furniture is AMarketStage's.
#include "StreetStage.h"

#include "BlenderProps.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "StreetAudio.h"
#include "StreetTraffic.h"

namespace StreetFurnitureDetail
{
const uint32 HpsHex = 0xffb35c;
const uint32 LedHex = 0xffe9d0;
const float HpsCandela = 1400.0f;
const float LedCandela = 1800.0f;

/** A lamp: its pole's foot (stage cm), the way its arm reaches (stage yaw), LED or HPS. */
struct FLamp
{
	double X;
	double Y;
	float Arm;
	bool bLed;
};
// Fifth's twelve HPS heads, staggered by side about every 26 m, 0.6 m in from the curb, clear of the doors, the stoops and
// 3 m from the tree pits (spec 2.8); two on each cross street's near sidewalk, mixed.
const FLamp FifthLamps[] = {
	{360.0, -5300.0, 0.0f, false},  {360.0, -3700.0, 0.0f, false},   {360.0, -1450.0, 0.0f, false},   {360.0, 700.0, 0.0f, false},
	{360.0, 2650.0, 0.0f, false},   {360.0, 4150.0, 0.0f, false},    {360.0, 7700.0, 0.0f, false},    {1680.0, -4800.0, 180.0f, false},
	{1680.0, -2300.0, 180.0f, false}, {1680.0, 650.0, 180.0f, false}, {1680.0, 3400.0, 180.0f, false}, {1680.0, 7300.0, 180.0f, false},
};
const FLamp CrossLamps[] = {
	{-1300.0, Ns::HollisRoad0 + 60.0, -90.0f, true},
	{3300.0, Ns::HollisRoad0 + 60.0, -90.0f, false},
	{-1100.0, Ns::WexlerRoad0 - 60.0, 90.0f, false},
	{3300.0, Ns::WexlerRoad0 - 60.0, 90.0f, true},
};

/** The trees' pits on Fifth (spec 4.1: final; the lamps keep 3 m from them). */
const FVector2D TreePits[] = {{330.0, -4700.0}, {330.0, -2900.0}, {330.0, 1500.0}, {1730.0, -3200.0}, {1730.0, -1600.0}, {1730.0, 5500.0}};

/** A post sign: which, where, facing (stage yaw). */
struct FSign
{
	const TCHAR* Mesh;
	double X;
	double Y;
	float Yaw;
};
// NO PARKING every 25 m on alternating sides (facing the traffic along their curb); STOP on Fifth's approaches to its T's.
const FSign Signs[] = {
	{TEXT("SM_NS_SignPost_NoParking"), 370.0, -5850.0, -90.0f},  {TEXT("SM_NS_SignPost_NoParking"), 1670.0, -4300.0, 90.0f},
	{TEXT("SM_NS_SignPost_NoParking"), 370.0, -2450.0, -90.0f},  {TEXT("SM_NS_SignPost_NoParking"), 1670.0, -900.0, 90.0f},
	{TEXT("SM_NS_SignPost_NoParking"), 370.0, 1050.0, -90.0f},   {TEXT("SM_NS_SignPost_NoParking"), 1670.0, 2300.0, 90.0f},
	{TEXT("SM_NS_SignPost_NoParking"), 370.0, 3700.0, -90.0f},   {TEXT("SM_NS_SignPost_NoParking"), 1670.0, 4600.0, 90.0f},
	{TEXT("SM_NS_SignPost_NoParking"), 370.0, 7300.0, -90.0f},   {TEXT("SM_NS_SignPost_NoParking"), 1670.0, 8550.0, 90.0f},
	{TEXT("SM_NS_SignPost_Stop"), 1680.0, Ns::HollisN + 150.0, 90.0f}, {TEXT("SM_NS_SignPost_Stop"), 360.0, Ns::WexlerS - 150.0, -90.0f},
};

/** A parked car: where (its middle on the road), facing (stage yaw), its kind's pick and paint's (seeded). */
struct FParked
{
	double X;
	double Y;
	float Yaw;
};
// Fifth: five along each curb (never within 4.5 m of the hydrants, the crosswalks or the T mouths); the cross streets: three
// on their far curbs, in view from the T (spec 2.7).
const FParked Parked[] = {
	{540.0, -5100.0, 90.0f},  {540.0, -3300.0, 90.0f},  {540.0, -1450.0, 90.0f},  {540.0, 2100.0, 90.0f},   {540.0, 8000.0, 90.0f},
	{1500.0, -5000.0, -90.0f}, {1500.0, -2700.0, -90.0f}, {1500.0, 1430.0, -90.0f}, {1500.0, 3600.0, -90.0f}, {1500.0, 7800.0, -90.0f},
	{-1500.0, Ns::HollisRoad1 + 105.0, 180.0f}, {2600.0, Ns::HollisRoad1 + 105.0, 180.0f}, {4300.0, Ns::HollisRoad1 + 105.0, 180.0f},
	{-1200.0, Ns::WexlerRoad1 - 105.0, 0.0f},   {2450.0, Ns::WexlerRoad1 - 105.0, 0.0f},   {4000.0, Ns::WexlerRoad1 - 105.0, 0.0f},
};

/** One of the car kit (spec 4.3): its mesh name, its wheels (axles' y, track's x, hub's z, Blender meters), its length. */
struct FCarKind
{
	const TCHAR* Name;
	double AxleFront;
	double AxleRear;
	double Track;
	double Hub;
	double Length;
	bool bTinted;
};
const FCarKind CarKinds[] = {
	{TEXT("SM_NS_Hatchback"), -1.30, 1.25, 0.755, 0.312, 4.20, true}, {TEXT("SM_NS_SUV"), -1.42, 1.36, 0.82, 0.37, 4.80, true},
	{TEXT("SM_NS_Cab"), -1.42, 1.38, 0.785, 0.326, 4.85, false},      {TEXT("SM_NS_Van"), -1.86, 1.80, 0.865, 0.345, 5.90, false},
	{TEXT("SM_NS_BoxTruck"), -2.40, 2.30, 0.95, 0.42, 7.50, false},    {TEXT("SM_NS_Pickup"), -1.75, 1.75, 0.85, 0.39, 5.30, true},
};

/** The paint palette (spec 3.6): weighted toward white, black, grey and silver (70%). */
struct FPaint
{
	uint32 Hex;
	float Weight;
};
const FPaint Paints[] = {{0xe8e8e6, 22.0f}, {0x16171a, 20.0f}, {0x5a5d62, 14.0f}, {0xa7abb0, 14.0f}, {0x1f3354, 7.0f}, {0x8b1a1a, 6.0f}, {0x1f4a3a, 4.0f},
	{0x6e1b24, 3.0f}, {0xb9a888, 3.0f}, {0x4a3628, 2.0f}, {0x6d8fb3, 2.0f}, {0xc9a227, 1.0f}, {0xc25a1f, 1.0f}, {0x2f6f6f, 1.0f}};

uint32 PaintFor(float U)
{
	float Total = 0.0f;
	for (const FPaint& P : Paints)
	{
		Total += P.Weight;
	}
	float At = U * Total;
	for (const FPaint& P : Paints)
	{
		if (At < P.Weight)
		{
			return P.Hex;
		}
		At -= P.Weight;
	}
	return Paints[0].Hex;
}

const TCHAR* const OtherMeshes[] = {TEXT("SM_Streetlight"), TEXT("SM_NS_StreetlightLED"), TEXT("SM_NS_StreetlightLED_GlowCool"), TEXT("SM_NS_SignalMast"),
	TEXT("SM_NS_SignalMast_GlowSignal"), TEXT("SM_NS_SignalMast_B"), TEXT("SM_NS_SignalMast_B_GlowSignal"), TEXT("SM_NS_SignalPedestal"),
	TEXT("SM_NS_SignalPedestal_GlowSignal"), TEXT("SM_NS_PedSignal"), TEXT("SM_NS_PedSignal_GlowSignal"), TEXT("SM_NS_SignalCabinet"), TEXT("SM_NS_CornerBlades_A"),
	TEXT("SM_NS_CornerBlades_B"), TEXT("SM_NS_CornerBlades_C"), TEXT("SM_NS_SignPost_NoParking"), TEXT("SM_NS_SignPost_Stop"), TEXT("SM_NS_Tree_A"), TEXT("SM_NS_Tree_B"),
	TEXT("SM_NS_TreeGrate"), TEXT("SM_NS_TreeGuard"), TEXT("SM_Hydrant"), TEXT("SM_TrashCan"), TEXT("SM_Newsbox"), TEXT("SM_NS_Mailbox"), TEXT("SM_NS_CallBox"),
	TEXT("SM_NS_CallBox_GlowWarm"), TEXT("SM_NS_MuniMeter"), TEXT("SM_NS_IceChest"), TEXT("SM_NS_PropaneCage"), TEXT("SM_NS_Payphone"), TEXT("SM_NS_CellarDoors"), TEXT("SM_NS_Awning_S"),
	TEXT("SM_NS_Awning_M"), TEXT("SM_NS_Awning_L"), TEXT("SM_Sedan")};
} // namespace StreetFurnitureDetail

using namespace StreetFurnitureDetail;

void AStreetStage::AppendFurniturePreload(TArray<FString>& Paths)
{
	for (const TCHAR* Name : OtherMeshes)
	{
		Paths.Add(FStreetPreload::MeshPath(Name));
	}
	for (const FCarKind& K : CarKinds)
	{
		Paths.Add(FStreetPreload::MeshPath(K.Name));
		Paths.Add(FStreetPreload::MeshPath(*FString::Printf(TEXT("%s_Wheel"), K.Name)));
		Paths.Add(FStreetPreload::MeshPath(*FString::Printf(TEXT("%s_GlowLamps"), K.Name)));
	}
}

void AStreetStage::BuildFurniture()
{
	// What stands on Fifth's sidewalks, for its walkways (Sidewalk): footprints by side (0 home, 1 far).
	auto Footprint = [this](double X, double Y, double HalfX, double HalfY) {
		if (Y < Ns::HollisN || Y > Ns::WexlerS)
		{
			return;
		}
		const int32 Side = X < Ns::FifthMidX ? 0 : 1;
		NsFootprints[Side].Add(FBox2D(FVector2D(X - HalfX, Y - HalfY), FVector2D(X + HalfX, Y + HalfY)));
	};
	BuildStreetlights();
	for (const FLamp& L : FifthLamps)
	{
		Footprint(L.X, L.Y, 20.0, 20.0);
	}
	BuildCorner();
	BuildTrees();
	BuildParkedCars();
	UMaterialInterface* Pole = Mat(TEXT("Pole"), 0x2c3033, 0.5f, 0.0f, 0.0f, 0.7f);

	// Signs on their posts.
	UMaterialInterface* SignFace = Mat(TEXT("SignFace"), 0xe8e6e0, 0.45f);
	for (const FSign& S : Signs)
	{
		if (!NsProp(S.Mesh, FVector(S.X, S.Y, 0.0), S.Yaw))
		{
			Cyl(Pole, FVector(S.X, S.Y, 0.0), 2.5f, 300.0f, false);
			const FVector Face = FRotator(0.0f, S.Yaw, 0.0f).Vector();
			const FVector At = FVector(S.X, S.Y, 255.0) + Face * 3.0;
			const FVector Half = FVector(FMath::Abs(Face.X) * 0.5 + FMath::Abs(Face.Y) * 23.0, FMath::Abs(Face.Y) * 0.5 + FMath::Abs(Face.X) * 23.0, 30.0);
			Box(SignFace, At - Half, At + Half, false)->SetCastShadow(false);
		}
		Blocker(FVector(S.X - 5.0, S.Y - 5.0, 0.0), FVector(S.X + 5.0, S.Y + 5.0, 300.0));
		Footprint(S.X, S.Y, 10.0, 10.0);
	}

	// Hydrants (the one by QuikCash, one across the street, one on each cross street's near sidewalk), the cans at the
	// corners, the newsboxes in their clusters (the Lucky Penny's door, Ruby's corner), the mailbox and the call boxes.
	struct FThing
	{
		const TCHAR* Mesh;
		double X;
		double Y;
		float Yaw;
		double Half;
		bool bLegacy;
	};
	const FThing Things[] = {
		{TEXT("SM_Hydrant"), 360.0, 1300.0, 0.0f, 25.0, true},
		{TEXT("SM_Hydrant"), 1690.0, -1000.0, 180.0f, 25.0, true},
		{TEXT("SM_Hydrant"), -600.0, Ns::HollisRoad0 + 55.0, -90.0f, 25.0, true},
		{TEXT("SM_Hydrant"), 2800.0, Ns::WexlerRoad0 - 55.0, 90.0f, 25.0, true},
		{TEXT("SM_TrashCan"), 330.0, 3550.0, -8.0f, 30.0, true},
		{TEXT("SM_TrashCan"), 1720.0, 2500.0, 170.0f, 30.0, true},
		{TEXT("SM_TrashCan"), 330.0, 7080.0, 14.0f, 30.0, true},
		{TEXT("SM_TrashCan"), 1700.0, Ns::HollisN + 120.0, 160.0f, 30.0, true},
		{TEXT("SM_Newsbox"), 330.0, 4420.0, 180.0f, 25.0, true},
		{TEXT("SM_Newsbox"), 330.0, 4480.0, 180.0f, 25.0, true},
		{TEXT("SM_Newsbox"), 330.0, 4540.0, 180.0f, 25.0, true},
		{TEXT("SM_Newsbox"), 300.0, 7200.0, 180.0f, 25.0, true},
		{TEXT("SM_Newsbox"), 300.0, 7260.0, 180.0f, 25.0, true},
		{TEXT("SM_Newsbox"), 300.0, 7320.0, 180.0f, 25.0, true},
		{TEXT("SM_NS_Mailbox"), 300.0, 7420.0, 180.0f, 35.0, false},
		{TEXT("SM_NS_CallBox"), 370.0, Ns::HollisN + 140.0, 180.0f, 20.0, false},
		// Fifth's two pay-and-display kiosks, 0.5 m in from the curb, their faces to the sidewalk (spec 2.8).
		{TEXT("SM_NS_MuniMeter"), 370.0, -950.0, 180.0f, 20.0, false},
		{TEXT("SM_NS_MuniMeter"), 1670.0, 2950.0, 0.0f, 20.0, false},
		// The corner's other two cabinets (the signal controller's on the far corner, BuildCorner): one against the Lucky
		// Penny's Market wall, one on the far side's north corner.
		{TEXT("SM_NS_SignalCabinet"), -260.0, Ns::StoreY1 + 45.0, 90.0f, 40.0, false},
		{TEXT("SM_NS_SignalCabinet"), 1900.0, 6760.0, 180.0f, 40.0, false},
	};
	for (const FThing& T : Things)
	{
		UStaticMeshComponent* C = T.bLegacy ? Prop(T.Mesh, FVector(T.X, T.Y, 0.0), T.Yaw) : NsProp(T.Mesh, FVector(T.X, T.Y, 0.0), T.Yaw);
		if (!C)
		{
			continue;
		}
		if (!T.bLegacy)
		{
			Blocker(FVector(T.X - T.Half, T.Y - T.Half, 0.0), FVector(T.X + T.Half, T.Y + T.Half, 120.0));
		}
		Footprint(T.X, T.Y, T.Half, T.Half);
		if (FCString::Strcmp(T.Mesh, TEXT("SM_NS_CallBox")) == 0)
		{
			if (UStaticMeshComponent* G = NsProp(TEXT("SM_NS_CallBox_GlowWarm"), FVector(T.X, T.Y, 0.0), T.Yaw))
			{
				AddGlow(G, Ns::EGlow::Warm, 0xffd9a0, 0.6f, -1);
			}
		}
	}

	// The Lucky Penny's Market wall (spec 2.5): the ice chest, the propane cage, a dead payphone, its cellar doors; QuikCash's
	// cellar doors on Fifth.
	if (NsProp(TEXT("SM_NS_IceChest"), FVector(-420.0, Ns::StoreY1 + 60.0, 0.0), 90.0f))
	{
		Blocker(FVector(-500.0, Ns::StoreY1 + 20.0, 0.0), FVector(-340.0, Ns::StoreY1 + 100.0, 120.0));
	}
	if (NsProp(TEXT("SM_NS_PropaneCage"), FVector(-1150.0, Ns::StoreY1 + 60.0, 0.0), 90.0f))
	{
		Blocker(FVector(-1210.0, Ns::StoreY1 + 20.0, 0.0), FVector(-1090.0, Ns::StoreY1 + 100.0, 120.0));
	}
	NsProp(TEXT("SM_NS_Payphone"), FVector(-620.0, Ns::StoreY1 + 20.0, 0.0), 90.0f);
	NsProp(TEXT("SM_NS_CellarDoors"), FVector(-800.0, Ns::StoreY1 + 80.0, 0.4), 90.0f);
	NsProp(TEXT("SM_NS_CellarDoors"), FVector(80.0, 2050.0, 0.4), 0.0f);

	// Awnings (spec 2.8): the barber's red over its bay (its stripe's on the fabric), Kim's green over the produce, Golden
	// Dragon's red over its takeout window; their valances carry the shops' names (core's ShopArt, drawn on the [screen]).
	struct FAwning
	{
		const TCHAR* Mesh;
		double X;
		double Y;
		double Z;
		float Yaw;
		double Width;
		uint32 Fabric;
		const TCHAR* Venue;
	};
	const FAwning Awnings[] = {
		{TEXT("SM_NS_Awning_M"), 0.0, 3365.0, 345.0, 0.0f, 0.84, 0xb3262a, TEXT("barber")},
		{TEXT("SM_NS_Awning_L"), 0.0, -2875.0, 380.0, 0.0f, 1.0, 0x3b6b2a, TEXT("kims")},
		{TEXT("SM_NS_Awning_M"), Ns::FarFront, -5500.0, 380.0, 180.0f, 1.0, 0xa81c1c, TEXT("golden-dragon")},
	};
	for (const FAwning& A : Awnings)
	{
		if (UStaticMeshComponent* C = NsProp(A.Mesh, FVector(A.X, A.Y, A.Z), A.Yaw, FVector(1.0, A.Width, 1.0)))
		{
			TintImported(C, ImportedSlot(C, 0), Ns::Srgb(A.Fabric));
			C->SetCastShadow(true);
			// The shop's name on the valance, printed white on the awning's own color.
			Screens.Paint(this, C, TEXT("ns_screen_valance"), FString::Printf(TEXT("valance:%s"), A.Venue), 1024, Ns::EScreen::Printed, Ns::Srgb(A.Fabric));
			// Its valance's edge, 1.6 m out and about 0.8 m down from where it hangs (spec 4.2), drips in the rain.
			const double Span = (FCString::Strstr(A.Mesh, TEXT("_L")) ? 900.0 : FCString::Strstr(A.Mesh, TEXT("_S")) ? 300.0 : 600.0) * A.Width;
			const FVector Out = FRotator(0.0f, A.Yaw, 0.0f).Vector();
			const FVector Along = FRotator(0.0f, A.Yaw + 90.0f, 0.0f).Vector();
			const FVector Edge = FVector(A.X, A.Y, A.Z - 80.0) + Out * 160.0;
			DripEdges.Emplace(Edge - Along * Span * 0.5, Edge + Along * Span * 0.5);
		}
	}

	// Decals where the street has been used (spec 3.5, 2.8): gum at the doors, butts by the cans at the corners.
	GroundDecal(2, FVector(120.0, 40.0, 0.0), 20.0f, FVector2D(140.0, 140.0));
	GroundDecal(2, FVector(160.0, 4700.0, 0.0), 75.0f, FVector2D(160.0, 160.0));
	GroundDecal(2, FVector(Ns::FarFront - 120.0, 140.0, 0.0), 140.0f, FVector2D(140.0, 140.0));
	GroundDecal(5, FVector(300.0, 3600.0, 0.0), 10.0f, FVector2D(90.0, 90.0));
	GroundDecal(5, FVector(260.0, 7120.0, 0.0), 210.0f, FVector2D(90.0, 90.0));
	GroundDecal(7, FVector(240.0, 4450.0, 0.0), 35.0f, FVector2D(110.0, 110.0));
	GroundDecal(6, FVector(1840.0, 2420.0, 0.0), 300.0f, FVector2D(100.0, 100.0));
}

void AStreetStage::BuildStreetlights()
{
	// Fifth keeps its HPS heads (2050 K, 1400 cd) on SM_Streetlight's poles; the cross streets mix in the LED cobra heads (4000
	// K, 1800 cd, a tighter cone): the two eras side by side, as real streets are (spec 3.1). Every lamp casts shadows
	// (MegaLights), a quarter of its light in the fog; every head's lens glows at night.
	UMaterialInterface* Pole = Mat(TEXT("Pole"), 0x2c3033, 0.5f, 0.0f, 0.0f, 0.7f);
	UMaterialInstanceDynamic* Sodium = Mat(TEXT("Sodium"), 0xffb066, 0.3f, 0.0f, 14.0f);
	NightGlowMids.Add(Sodium);
	NightGlowLevels.Add(14.0f);
	auto Lamp = [&](const FLamp& P) {
		const FVector Out = FRotator(0.0f, P.Arm, 0.0f).Vector();
		FVector Head;
		float Candela = HpsCandela;
		if (P.bLed)
		{
			// The cobra head's lens 2.15 m out along its arm, 8.92 m up (ns_kit.py).
			Head = FVector(P.X, P.Y, 0.0) + Out * 215.0 + FVector(0.0, 0.0, 885.0);
			Candela = LedCandela;
			if (NsProp(TEXT("SM_NS_StreetlightLED"), FVector(P.X, P.Y, 0.0), P.Arm))
			{
				if (UStaticMeshComponent* G = NsProp(TEXT("SM_NS_StreetlightLED_GlowCool"), FVector(P.X, P.Y, 0.0), P.Arm))
				{
					AddGlow(G, Ns::EGlow::Cool, LedHex, 3.0f, -1);
				}
			}
			else
			{
				Cyl(Pole, FVector(P.X, P.Y, 0.0), 9.0f, 900.0f);
				const FVector Tip = FVector(P.X, P.Y, 0.0) + Out * 230.0;
				Box(Pole, FVector(FMath::Min(P.X, Tip.X) - 5.0, FMath::Min(P.Y, Tip.Y) - 5.0, 890.0), FVector(FMath::Max(P.X, Tip.X) + 5.0, FMath::Max(P.Y, Tip.Y) + 5.0, 900.0), false);
				UMaterialInstanceDynamic* Lens = Mat(TEXT("LedLens"), 0xfff4e8, 0.3f, 0.0f, 18.0f);
				if (!NightGlowMids.Contains(Lens))
				{
					NightGlowMids.Add(Lens);
					NightGlowLevels.Add(18.0f);
				}
				Box(Lens, Head + FVector(-25.0, -25.0, -2.0), Head + FVector(25.0, 25.0, 3.0), false)->SetCastShadow(false);
			}
		}
		else
		{
			Head = FVector(P.X, P.Y, 0.0) + Out * 150.0 + FVector(0.0, 0.0, 790.0);
			if (UStaticMeshComponent* C = Prop(TEXT("SM_Streetlight"), FVector(P.X, P.Y, 0.0), P.Arm))
			{
				// Its lens's own bake glows at night only.
				AddGlow(C, Ns::EGlow::Warm, HpsHex, 1.0f, -1);
				C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
			else
			{
				Cyl(Pole, FVector(P.X, P.Y, 0.0), 9.0f, 820.0f);
				const FVector Tip = FVector(P.X, P.Y, 0.0) + Out * 180.0;
				Box(Pole, FVector(FMath::Min(P.X, Tip.X) - 5.0, FMath::Min(P.Y, Tip.Y) - 5.0, 805.0), FVector(FMath::Max(P.X, Tip.X) + 5.0, FMath::Max(P.Y, Tip.Y) + 5.0, 815.0), false);
				Box(Sodium, FVector(Head.X - 22.0, Head.Y - 22.0, 795.0), FVector(Head.X + 22.0, Head.Y + 22.0, 805.0), false)->SetCastShadow(false);
			}
		}
		Blocker(FVector(P.X - 12.0, P.Y - 12.0, 0.0), FVector(P.X + 12.0, P.Y + 12.0, 400.0));
		if (USpotLightComponent* L = NewPart<USpotLightComponent>())
		{
			L->SetRelativeLocationAndRotation(Head, FRotator(-90.0f, 0.0f, 0.0f));
			L->SetIntensityUnits(ELightUnits::Candelas);
			L->SetIntensity(Candela);
			L->SetAttenuationRadius(2400.0f);
			// A pool with an edge to it under each (the LED's tighter and sharper), the rain bright in the cone.
			L->SetOuterConeAngle(P.bLed ? 50.0f : 62.0f);
			L->SetInnerConeAngle(P.bLed ? 30.0f : 26.0f);
			L->SetSourceRadius(P.bLed ? 8.0f : 12.0f);
			L->SetLightColor(Ns::Srgb(P.bLed ? LedHex : HpsHex));
			L->SetUseTemperature(false);
			L->SetVolumetricScatteringIntensity(0.25f);
			L->SetCastShadows(true);
			L->SetCastVolumetricShadow(false);
			Streetlights.Add(L);
		}
	};
	for (const FLamp& P : FifthLamps)
	{
		Lamp(P);
	}
	for (const FLamp& P : CrossLamps)
	{
		Lamp(P);
	}
}

void AStreetStage::BuildCorner()
{
	// The corner of Fifth & Market's signals (spec 2.5): the mast arm from the far sidewalk over Fifth (heads north and south),
	// the one from Ruby's corner over Market's westbound lane (a head east), the pedestal on the Lucky Penny's corner (south),
	// and six walk heads at the crosswalks' ends, each facing across its crosswalk. Their glows (M_NS_Glow's Signal grid) are
	// lit by town::Signal (UpdateSignals). Today's Market has the pedestal and the far mast alone.
	UMaterialInterface* GlowBase = Cast<UMaterialInterface>(NsAsset(Ns::GlowPath));
	UMaterialInterface* Pole = Mat(TEXT("Pole"), 0x2c3033, 0.5f, 0.0f, 0.0f, 0.7f);
	UMaterialInterface* Housing = Mat(TEXT("SignalHousing"), 0x15171a, 0.4f, 0.0f, 0.0f, 0.3f);
	auto SignalGlow = [&](const TCHAR* Glow, const FVector& At, float Yaw, uint8 SignalRole) {
		UStaticMeshComponent* G = NsProp(Glow, At, Yaw);
		if (!G || !GlowBase)
		{
			return false;
		}
		UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(GlowBase, this);
		Mid->SetScalarParameterValue(TEXT("Mode"), 1.0f);
		for (int32 Slot = 0; Slot < G->GetNumMaterials(); ++Slot)
		{
			G->SetMaterial(Slot, Mid);
		}
		G->SetCastShadow(false);
		FNsSignal S;
		S.Mid = Mid;
		S.Role = SignalRole;
		NsSignals.Add(S);
		return true;
	};
	// A stand-in head: a housing and three lamps (or walk, hand) of M_Street glow, each its own signal entry by cell.
	auto StandInHead = [&](const FVector& At, float Yaw, uint8 SignalRole, bool bPed) {
		const FVector F = FRotator(0.0f, Yaw, 0.0f).Vector();
		const FVector R = FRotator(0.0f, Yaw + 90.0f, 0.0f).Vector();
		const FVector Half = F * 12.0 + R * 18.0;
		Box(Housing, At - FVector(FMath::Abs(Half.X), FMath::Abs(Half.Y), bPed ? 25.0 : 50.0), At + FVector(FMath::Abs(Half.X), FMath::Abs(Half.Y), bPed ? 25.0 : 50.0), false);
		const int32 Cells = bPed ? 2 : 3;
		static const uint32 Hex[5] = {0xff2010, 0xff8a10, 0x20ffa0, 0xf2f6ff, 0xff7a20};
		for (int32 K = 0; K < Cells; ++K)
		{
			const int32 Cell = bPed ? 3 + K : K;
			const FVector C = At + F * 13.0 + FVector(0.0, 0.0, bPed ? (K == 0 ? -10.0 : 10.0) : 30.0 - 30.0 * K);
			UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(StreetMaterial ? StreetMaterial.Get() : FallbackMaterial.Get(), this);
			const FLinearColor Col = Ns::Srgb(Hex[Cell]);
			M->SetVectorParameterValue(TEXT("BaseColor"), Col);
			M->SetVectorParameterValue(TEXT("Color"), Col);
			M->SetScalarParameterValue(TEXT("Roughness"), 0.3f);
			M->SetScalarParameterValue(TEXT("Emissive"), 0.0f);
			Box(M, C - FVector(1.0, 1.0, 1.0) * 9.0, C + FVector(1.0, 1.0, 1.0) * 9.0, false)->SetCastShadow(false);
			FNsSignal S;
			S.Mid = M;
			S.Role = static_cast<uint8>(SignalRole | ((Cell + 1) << 4));
			NsSignals.Add(S);
		}
	};
	auto Mast = [&](const TCHAR* Mesh, const TCHAR* Glow, const FVector& At, float Yaw, uint8 SignalRole, double Reach) {
		const bool bMesh = NsProp(Mesh, At, Yaw) != nullptr;
		if (!(bMesh && SignalGlow(Glow, At, Yaw, SignalRole)))
		{
			if (!bMesh)
			{
				Cyl(Pole, At, 12.0f, 650.0f);
				const FVector Tip = At + FRotator(0.0f, Yaw, 0.0f).Vector() * Reach;
				Box(Pole, FVector(FMath::Min(At.X, Tip.X) - 6.0, FMath::Min(At.Y, Tip.Y) - 6.0, 595.0), FVector(FMath::Max(At.X, Tip.X) + 6.0, FMath::Max(At.Y, Tip.Y) + 6.0, 607.0), false);
			}
			StandInHead(At + FRotator(0.0f, Yaw, 0.0f).Vector() * Reach * 0.85 + FVector(0.0, 0.0, 545.0), Yaw + 90.0f, SignalRole, false);
		}
		Blocker(At - FVector(18.0, 18.0, 0.0), At + FVector(18.0, 18.0, 400.0));
	};
	Mast(TEXT("SM_NS_SignalMast"), TEXT("SM_NS_SignalMast_GlowSignal"), FVector(1760.0, Ns::MarketMidY, 0.0), 180.0f, 0, 650.0);
	if (bWideMarket)
	{
		Mast(TEXT("SM_NS_SignalMast_B"), TEXT("SM_NS_SignalMast_B_GlowSignal"), FVector(330.0, 6620.0, 0.0), -90.0f, 1, 650.0);
	}
	const FVector Pedestal(330.0, bWideMarket ? 5280.0 : 5160.0, 0.0);
	if (!(NsProp(TEXT("SM_NS_SignalPedestal"), Pedestal, -90.0f) && SignalGlow(TEXT("SM_NS_SignalPedestal_GlowSignal"), Pedestal, -90.0f, 0)))
	{
		Cyl(Pole, Pedestal, 8.0f, 300.0f);
		StandInHead(Pedestal + FVector(0.0, 0.0, 330.0), -90.0f, 0, false);
	}
	Blocker(Pedestal - FVector(15.0, 15.0, 0.0), Pedestal + FVector(15.0, 15.0, 350.0));
	// The walk heads at the crosswalks' ends (the Fifth crossings: Role 2; Market's mouth: Role 3).
	struct FPed
	{
		double X;
		double Y;
		float Yaw;
		uint8 Role;
	};
	const double SouthLeg = bWideMarket ? 4960.0 : 4980.0;
	const double NorthLeg = bWideMarket ? 6940.0 : 6160.0;
	TArray<FPed> Peds = {{370.0, SouthLeg, 0.0f, 2}, {1670.0, SouthLeg, 180.0f, 2}, {370.0, NorthLeg, 0.0f, 2}, {1670.0, NorthLeg, 180.0f, 2}};
	if (bWideMarket)
	{
		Peds.Add({20.0, Ns::MarketS - 40.0, 90.0f, 3});
		Peds.Add({20.0, Ns::MarketN + 40.0, -90.0f, 3});
	}
	for (const FPed& P : Peds)
	{
		const FVector At(P.X, P.Y, 0.0);
		if (!(NsProp(TEXT("SM_NS_PedSignal"), At, P.Yaw) && SignalGlow(TEXT("SM_NS_PedSignal_GlowSignal"), At, P.Yaw, P.Role)))
		{
			Cyl(Pole, At, 6.0f, 300.0f);
			StandInHead(At + FVector(0.0, 0.0, 260.0), P.Yaw, P.Role, true);
		}
		Blocker(At - FVector(10.0, 10.0, 0.0), At + FVector(10.0, 10.0, 300.0));
		if (P.Y >= Ns::HollisN && P.Y <= Ns::WexlerS)
		{
			NsFootprints[P.X < Ns::FifthMidX ? 0 : 1].Add(FBox2D(FVector2D(P.X - 15.0, P.Y - 15.0), FVector2D(P.X + 15.0, P.Y + 15.0)));
		}
	}
	NsProp(TEXT("SM_NS_SignalCabinet"), FVector(1880.0, Ns::MarketMidY - 250.0, 0.0), 180.0f);

	// The street blades: Fifth & Market on the Lucky Penny's corner (FIFTH ST along Fifth, MARKET ST along Market), and at
	// each T's home corner. Without the kit: the old crossed plates, printed.
	const FVector Corner(390.0, bWideMarket ? 5040.0 : Ns::StoreY1 + 90.0, 0.0);
	if (!NsProp(TEXT("SM_NS_CornerBlades_A"), Corner, 90.0f))
	{
		UMaterialInterface* Blade = Mat(TEXT("StreetBlade"), 0x0f6b3a, 0.4f, 0.0f, 0.0f, 0.3f);
		Cyl(Pole, Corner, 5.0f, 340.0f);
		Box(Blade, Corner + FVector(-0.4, -54.0, 340.0), Corner + FVector(0.4, 54.0, 364.0), false)->SetCastShadow(false);
		Box(Blade, Corner + FVector(-54.0, -0.4, 364.0), Corner + FVector(54.0, 0.4, 388.0), false)->SetCastShadow(false);
		Sign(TEXT("street:FIFTH ST:1800"), Corner + FVector(0.8, 0.0, 352.0), 0.0f, FVector2D(110.0, 24.0), FIntPoint(900, 200), 0.0f);
		Sign(TEXT("street:FIFTH ST:1800"), Corner + FVector(-0.8, 0.0, 352.0), 180.0f, FVector2D(110.0, 24.0), FIntPoint(900, 200), 0.0f);
		Sign(TEXT("street:MARKET ST:200"), Corner + FVector(0.0, 0.8, 376.0), 90.0f, FVector2D(110.0, 24.0), FIntPoint(900, 200), 0.0f);
		Sign(TEXT("street:MARKET ST:200"), Corner + FVector(0.0, -0.8, 376.0), -90.0f, FVector2D(110.0, 24.0), FIntPoint(900, 200), 0.0f);
	}
	Blocker(Corner - FVector(8.0, 8.0, 0.0), Corner + FVector(8.0, 8.0, 340.0));
	NsFootprints[0].Add(FBox2D(FVector2D(Corner.X - 10.0, Corner.Y - 10.0), FVector2D(Corner.X + 10.0, Corner.Y + 10.0)));
	NsProp(TEXT("SM_NS_CornerBlades_B"), FVector(390.0, Ns::HollisN - 330.0, 0.0), 90.0f);
	NsProp(TEXT("SM_NS_CornerBlades_C"), FVector(390.0, Ns::WexlerS + 330.0, 0.0), 90.0f);
}

void AStreetStage::BuildTrees()
{
	// London planes and honey locusts in their pits (spec 4.2), October: masked Nanite foliage swaying in MPC_NS's wind,
	// their shadows cached as rigid (the sway doesn't invalidate the sun's pages), the sway stopped past ~40 m, their leaves
	// out of the ray-traced scene (ns_setup.py, spec 3.8). The grates over the pits; leaves fallen round them.
	UMaterialInterface* Bark = Mat(TEXT("TreeBark"), 0x3b3128, 0.9f);
	UMaterialInterface* Crown = Mat(TEXT("TreeCrown"), 0x4a5a2a, 0.8f);
	int32 K = 0;
	for (const FVector2D& Pit : TreePits)
	{
		const FVector At(Pit.X, Pit.Y, -3.0);
		const float Yaw = 37.0f * static_cast<float>(K) + 11.0f;
		UStaticMeshComponent* Tree = NsProp(K % 2 == 0 ? TEXT("SM_NS_Tree_A") : TEXT("SM_NS_Tree_B"), At, Yaw);
		if (Tree)
		{
			Tree->ShadowCacheInvalidationBehavior = EShadowCacheInvalidationBehavior::Rigid;
			Tree->SetWorldPositionOffsetDisableDistance(4000);
			Tree->bAffectDistanceFieldLighting = false;
			Tree->MarkRenderStateDirty();
			Trees.Add(Tree);
		}
		else
		{
			// A stand-in: a trunk and a rough crown.
			Cyl(Bark, FVector(Pit.X, Pit.Y, 0.0), 14.0f, 380.0f, false);
			UStaticMeshComponent* C = NewPart<UStaticMeshComponent>();
			C->SetStaticMesh(SphereMesh);
			C->SetMaterial(0, Crown);
			C->SetRelativeLocation(FVector(Pit.X, Pit.Y, 560.0));
			C->SetRelativeScale3D(FVector(5.2, 5.2, 4.0));
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		NsProp(TEXT("SM_NS_TreeGrate"), FVector(Pit.X, Pit.Y, -0.5), 0.0f);
		Blocker(FVector(Pit.X - 18.0, Pit.Y - 18.0, 0.0), FVector(Pit.X + 18.0, Pit.Y + 18.0, 300.0));
		NsFootprints[Pit.X < Ns::FifthMidX ? 0 : 1].Add(FBox2D(FVector2D(Pit.X - 60.0, Pit.Y - 60.0), FVector2D(Pit.X + 60.0, Pit.Y + 60.0)));
		for (int32 D = 0; D < 2; ++D)
		{
			const FVector Off = FRotator(0.0f, Yaw + 120.0f * D, 0.0f).Vector() * 110.0;
			GroundDecal(3 + D, FVector(Pit.X, Pit.Y, 0.0) + Off, Yaw * 3.0f + D * 70.0f, FVector2D(120.0, 120.0));
		}
		++K;
	}
}

void AStreetStage::BuildParkedCars()
{
	// The car kit in many paints (spec 3.6, 2.7): each car its kind and paint from the table and the career's seed, so no
	// two alike in a view; its four wheels at its axles; oil under it on the road; a box keeps the hero out of it. The cab
	// keeps its yellow. Without the kit: the sedan, or a box of a car.
	TArray<int32> Kinds;
	for (int32 I = 0; I < static_cast<int32>(UE_ARRAY_COUNT(CarKinds)); ++I)
	{
		if (NsMesh(CarKinds[I].Name))
		{
			Kinds.Add(I);
		}
	}
	UMaterialInterface* CarGlass = Mat(TEXT("CarGlass"), 0x0b0e12, 0.05f, 0.0f, 0.0f, 0.4f);
	// A fixed seed: the same cars in the same paints every load (spec 2.8).
	const uint32 Seed = 0x5eed5eedu;
	int32 Last = -1;
	for (int32 I = 0; I < static_cast<int32>(UE_ARRAY_COUNT(Parked)); ++I)
	{
		const FParked& P = Parked[I];
		const float U = Ns::Hash01(Seed + 977u * static_cast<uint32>(I));
		const uint32 Paint = PaintFor(Ns::Hash01(Seed * 31u + 131u * static_cast<uint32>(I)));
		// A car's road under its middle: Fifth's crown, or the cross streets'.
		const bool bFifth = P.X > Ns::RoadX0 && P.X < Ns::RoadX1;
		const double Z = bFifth ? Ns::RoadZ(FMath::Abs(P.X - Ns::FifthMidX), Ns::RoadX1 - Ns::RoadX0)
								 : Ns::RoadZ(FMath::Abs(P.Y - (P.Y < 0.0 ? (Ns::HollisRoad0 + Ns::HollisRoad1) * 0.5 : (Ns::WexlerRoad0 + Ns::WexlerRoad1) * 0.5)), 1000.0);
		const FVector At(P.X, P.Y, Z);
		double Length = 480.0;
		bool bPlaced = false;
		if (Kinds.Num() > 0 && U < 0.85f)
		{
			// Never the same kind twice in a row.
			int32 Pick = Kinds[FMath::Min(Kinds.Num() - 1, static_cast<int32>(U / 0.85f * Kinds.Num()))];
			if (Pick == Last && Kinds.Num() > 1)
			{
				Pick = Kinds[(Kinds.IndexOfByKey(Pick) + 1) % Kinds.Num()];
			}
			Last = Pick;
			const FCarKind& K = CarKinds[Pick];
			USceneComponent* Car = NewPart<USceneComponent>();
			Car->SetRelativeLocationAndRotation(At, BlenderFacing(P.Yaw));
			if (UStaticMeshComponent* Body = NewPart<UStaticMeshComponent>(Car))
			{
				Body->SetStaticMesh(NsMesh(K.Name));
				Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				if (K.bTinted)
				{
					ParkedCars.Add(Body);
					ParkedPaints.Add(Ns::Srgb(Paint));
				}
				// Its plates, front and rear (one [screen] slot), its own number (core's PlateText: never two alike, never 7FTH 212).
				Screens.Paint(this, Body, TEXT("ns_plate"), TEXT("plate:") + Ns::PlateFor(1, I), 256, Ns::EScreen::Printed);
			}
			if (UStaticMesh* Wheel = NsMesh(*FString::Printf(TEXT("%s_Wheel"), K.Name)))
			{
				// Four wheels at (+-track, axle, hub) in the car's imported frame (Blender's Y mirrored), the -X side turned half round.
				for (int32 W = 0; W < 4; ++W)
				{
					const double Side = W % 2 == 0 ? 1.0 : -1.0;
					const double Axle = W < 2 ? K.AxleFront : K.AxleRear;
					UStaticMeshComponent* Wc = NewPart<UStaticMeshComponent>(Car);
					Wc->SetStaticMesh(Wheel);
					Wc->SetRelativeLocationAndRotation(FVector(100.0 * Side * K.Track, -100.0 * Axle, 100.0 * K.Hub), FRotator(0.0f, Side > 0.0 ? 0.0f : 180.0f, 0.0f));
					Wc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				}
			}
			Length = K.Length * 100.0;
			bPlaced = true;
		}
		else if (UStaticMeshComponent* Sedan = Prop(TEXT("SM_Sedan"), At, P.Yaw))
		{
			Sedan->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ParkedCars.Add(Sedan);
			ParkedPaints.Add(Ns::Srgb(Paint));
			// sedan.py's split moves its plate to a [screen] slot (spec 4.3); until then its baked plate stays.
			Screens.Paint(this, Sedan, TEXT("ns_plate"), TEXT("plate:") + Ns::PlateFor(1, I), 256, Ns::EScreen::Printed);
			bPlaced = true;
		}
		const FVector Fwd = FRotator(0.0f, P.Yaw, 0.0f).Vector();
		const FVector Rt = FRotator(0.0f, P.Yaw + 90.0f, 0.0f).Vector();
		const FVector Ext = (Fwd * Length * 0.5).GetAbs() + (Rt * 90.0).GetAbs();
		if (!bPlaced)
		{
			UMaterialInterface* Body = Mat(*FString::Printf(TEXT("Car%06x"), Paint), Paint, 0.25f, 0.0f, 0.0f, 0.6f);
			Box(Body, At - FVector(Ext.X, Ext.Y, 0.0) + FVector(0.0, 0.0, 10.0), At + FVector(Ext.X, Ext.Y, 105.0));
			const FVector Cab = (Fwd * Length * 0.22).GetAbs() + (Rt * 75.0).GetAbs();
			Box(CarGlass, At - FVector(Cab.X, Cab.Y, 0.0) + FVector(0.0, 0.0, 105.0), At + FVector(Cab.X, Cab.Y, 150.0));
		}
		else
		{
			Blocker(At - FVector(Ext.X, Ext.Y, -5.0), At + FVector(Ext.X, Ext.Y, 140.0));
		}
		// Oil soaked into the road under it (spec 3.5).
		GroundDecal(I % 3 == 0 ? 1 : 0, FVector(P.X, P.Y, Z), P.Yaw + 90.0f, FVector2D(260.0, 150.0));
	}
	PaintCars();
}

void AStreetStage::BuildEmitters()
{
	// Where the street's positioned sounds come from (spec 3.7, stage cm): the signs' and tubes' hum, the AC units' fans in
	// upper windows, the downspouts running in the rain, the steady hiss of the corner's signal cabinet. Shops' open doors
	// leak their room tone (from the registry's doors at play); Market's are its own.
	auto Add = [this](const FVector& At, uint8 Kind, float Level, float Hz, int32 Shop) {
		FNsEmitterSpot E;
		E.At = At;
		E.Kind = Kind;
		E.Level = Level;
		E.Hz = Hz;
		E.Shop = Shop;
		NsEmitters.Add(E);
	};
	Add(FVector(20.0, 1550.0, 200.0), static_cast<uint8>(EStreetEmitter::Hum), 0.8f, 120.0f, -1);
	Add(FVector(Ns::FarFront - 20.0, -5600.0, 300.0), static_cast<uint8>(EStreetEmitter::Hum), 0.7f, 120.0f, -1);
	Add(FVector(Ns::FarFront - 20.0, 5520.0, 250.0), static_cast<uint8>(EStreetEmitter::Hum), 0.6f, 118.0f, -1);
	Add(FVector(20.0, 8600.0, 420.0), static_cast<uint8>(EStreetEmitter::Hum), 0.5f, 121.0f, -1);
	Add(FVector(-10.0, -3150.0, 1060.0), static_cast<uint8>(EStreetEmitter::Ac), 0.7f, 59.0f, -1);
	Add(FVector(Ns::FarFront + 10.0, -1300.0, 900.0), static_cast<uint8>(EStreetEmitter::Ac), 0.6f, 61.0f, -1);
	Add(FVector(-10.0, 8700.0, 1210.0), static_cast<uint8>(EStreetEmitter::Ac), 0.6f, 58.0f, -1);
	Add(FVector(-5.0, -3600.0, 30.0), static_cast<uint8>(EStreetEmitter::Gutter), 0.8f, 0.0f, -1);
	Add(FVector(Ns::FarFront + 5.0, -880.0, 30.0), static_cast<uint8>(EStreetEmitter::Gutter), 0.8f, 0.0f, -1);
	Add(FVector(-5.0, 800.0, 30.0), static_cast<uint8>(EStreetEmitter::Gutter), 0.7f, 0.0f, -1);
	Add(FVector(Ns::FarFront + 5.0, 6300.0, 30.0), static_cast<uint8>(EStreetEmitter::Gutter), 0.7f, 0.0f, -1);
	Add(FVector(0.0, 3365.0, 330.0), static_cast<uint8>(EStreetEmitter::RainRoof), 0.7f, 0.0f, -1);
	Add(FVector(0.0, -2875.0, 370.0), static_cast<uint8>(EStreetEmitter::RainRoof), 0.7f, 0.0f, -1);
}
