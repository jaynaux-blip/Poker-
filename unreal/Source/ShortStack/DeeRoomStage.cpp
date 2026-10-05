#include "DeeRoomStage.h"

#include "BackRoomGameMode.h"
#include "BackRoomPlayer.h"
#include "BackRoomPlayerShared.h"
#include "BlenderProps.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StaticMesh.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "ShortStack/UI/PropArt.h"
#include "SlateDrawList.h"

namespace DeeRoomStageDetail
{
// The room (cm). The street wall is the tenement's face: 45 cm of brick (tenement.py DEPTH).
const double FrontX = 170.0;
const double WallT = 45.0;
const double BackX = -230.0;
const double LeftY = -400.0;
const double RightY = 260.0;
const double Ceiling = 290.0;
// Her two windows, the tenement's (tenement.py: 1.15 x 1.75 m, the sill 0.85 m over the floor): the first floor over
// the Wash & Fold, bays 8 (ahead, over the coffee table) and 7 (to its left).
const double WinW = 115.0;
const double WinH = 175.0;
const double WinSill = 85.0;
const double WinY[2] = {0.0, -275.0};

// Fifth Street (AStreetStage's cm: 1812's face on X 0, the far side's at FarFront 2040, the sidewalk's top at Z 0) turned
// half round about her front wall: her windows over bay 8 of the tenement (street Y 370), her floor 4.4 m up (SHOP_H).
const double StreetX = 2040.0 + FrontX + WallT;
const double StreetY = 370.0;
const double StreetZ = 440.0;

/** A point on Fifth Street (AStreetStage's cm) as it lies from her front room. */
FVector FromStreet(double X, double Y, double Z)
{
	return FVector(StreetX - X, StreetY - Y, Z - StreetZ);
}

// The seat: perched on the couch's front edge, leaned in over the coffee table (the shoulders as far from the laptop's
// palm rest as the desk chair's are).
const FVector Eye(-126.0, 0.0, 108.0);
// dee_room.py: SM_DR_Couch 2.10 x 0.91 x 0.86 m, its back to the back wall; SM_DR_CoffeeTable ~1.10 x 0.60, the top at
// 0.45 m, LAPTOP_POINT 3 cm behind its middle (from the couch); Dee's recliner, its seat about 0.45 m up, turned to the
// TV in the corner.
const FVector CouchAt(-182.5, 0.0, 0.0);
const double CouchSeat = 47.0;
const FVector TableAt(-73.25, 0.0, 0.0);
const double TableTop = 45.0;
const FVector LaptopAt(TableAt.X + 3.0, 0.0, TableTop);
const FVector ArmchairAt(-25.0, 160.0, 0.0);
const float ArmchairYaw = 15.0f;
const double ArmchairSeat = 45.0;
const FVector TvAt(125.0, 210.0, 0.0);
const float TvYaw = -150.0f;
const FVector LampAt(-205.0, -135.0, 0.0);
const FVector HallDoorAt(-150.0, RightY, 0.0);
const double BedroomDoorY = -230.0;

// Surface patterns understood by M_Surface (NightOneStage.cpp) and M_Street (street_setup.py).
const float PatternPlaster = 1.0f;
const float PatternWood = 2.0f;
const float PatternFloor = 3.0f;
const float PatternFabric = 5.0f;
const float StreetBrick = 1.0f;
const float StreetAsphalt = 2.0f;
const float StreetSlabs = 3.0f;
const float StreetConcrete = 5.0f;

// Window looks on the stand-in fronts (FrontWindow's Kind): dark glass, then lit warm, cool, a TV's blue, a stairwell.
const uint32 LitHex[5] = {0x0b0d10, 0xffaf62, 0xe2ecff, 0x5f86ff, 0xffc98a};
const float LitGlow[5] = {0.0f, 2.4f, 2.0f, 1.8f, 1.4f};

TAutoConsoleVariable<float> CVarLamp(TEXT("ss.Dee.Lamp"), 24.0f, TEXT("Dee's floor lamp (candelas): on from dusk, left on low for the couch all night."));
TAutoConsoleVariable<float> CVarTv(TEXT("ss.Dee.Tv"), 1.2f, TEXT("The light Dee's TV throws on the room while it's on (candelas)."));
TAutoConsoleVariable<float> CVarFrontGlow(TEXT("ss.Dee.FrontGlow"), 0.4f,
	TEXT("How brightly 1812's and its neighbours' lit rooms glow from Dee's window at night, against their bake for the street."));

FLinearColor Srgb(uint32 Hex)
{
	return FLinearColor::FromSRGBColor(FColor(static_cast<uint8>((Hex >> 16) & 0xff), static_cast<uint8>((Hex >> 8) & 0xff), static_cast<uint8>(Hex & 0xff)));
}

/** Rotation whose forward (+X) is Normal and up (+Z) is Up. */
FRotator Facing(const FVector& Normal, const FVector& Up)
{
	return FRotationMatrix::MakeFromXZ(Normal, Up).Rotator();
}

UMaterialInterface* LoadOptional(const TCHAR* Path)
{
	return LoadObject<UMaterialInterface>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
}

/** A small deterministic random sequence (the stand-ins' windows). */
struct FDice
{
	int64 S = 1;
	double Next()
	{
		S = (S * 16807) % 2147483647;
		return static_cast<double>(S) / 2147483647.0;
	}
};

// What's on the TV: a studio's blue, a warm living room, a green field, a night street, a white kitchen, a red stage.
const FLinearColor TvShots[] = {FLinearColor(0.18f, 0.32f, 0.85f), FLinearColor(0.85f, 0.55f, 0.32f), FLinearColor(0.3f, 0.7f, 0.32f),
	FLinearColor(0.12f, 0.14f, 0.3f), FLinearColor(0.8f, 0.82f, 0.86f), FLinearColor(0.85f, 0.22f, 0.25f), FLinearColor(0.55f, 0.6f, 0.75f)};
} // namespace DeeRoomStageDetail

using namespace DeeRoomStageDetail;

ADeeRoomStage::ADeeRoomStage()
{
	// Her lamp is on of an evening: the room is lit more than the apartment's, and the lens is set a little darker for it.
	ExposureBias = 0.45f;
}

// ------------------------------------------------------------------ building blocks

USceneComponent* ADeeRoomStage::Frame(const FVector& At, float Yaw)
{
	USceneComponent* F = NewPart<USceneComponent>();
	F->SetRelativeLocationAndRotation(At, FRotator(0.0f, Yaw, 0.0f));
	return F;
}

UStaticMeshComponent* ADeeRoomStage::Prop(const TCHAR* Name, USceneComponent* Parent, const FVector& Local, float Yaw)
{
	UStaticMesh* Mesh = LoadProp(Name);
	return Mesh ? AddMesh(Mesh, nullptr, Local, FVector(1.0), PropFacing(Yaw), Parent) : nullptr;
}

UStaticMeshComponent* ADeeRoomStage::Block(UMaterialInterface* Material, const FVector& Min, const FVector& Max, USceneComponent* Parent, bool bShadows)
{
	// The engine cube is 100 cm on a side, centered on its pivot.
	const FVector Lo = Min.ComponentMin(Max);
	const FVector Hi = Min.ComponentMax(Max);
	return AddMesh(CubeMesh, Material, (Lo + Hi) * 0.5, (Hi - Lo) / 100.0, FRotator::ZeroRotator, Parent, bShadows);
}

UStaticMeshComponent* ADeeRoomStage::Post(UMaterialInterface* Material, const FVector& Base, double Radius, double Height, USceneComponent* Parent, bool bShadows)
{
	// The engine cylinder is 100 cm across and 100 cm tall, centered on its pivot.
	return AddMesh(CylinderMesh, Material, Base + FVector(0.0, 0.0, Height * 0.5), FVector(Radius / 50.0, Radius / 50.0, Height / 100.0), FRotator::ZeroRotator, Parent,
	               bShadows);
}

UMaterialInterface* ADeeRoomStage::StreetSurface(FName Key, uint32 SrgbHex, float Roughness, float Pattern)
{
	if (!StreetMaterial)
	{
		// The street's own material isn't built: plain wet surfaces (low roughness) in the stage's.
		return Surface(Key, SrgbHex, Roughness * 0.45f);
	}
	if (TObjectPtr<UMaterialInstanceDynamic>* Found = Materials.Find(Key))
	{
		return *Found;
	}
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(StreetMaterial, this);
	const FLinearColor Color = Srgb(SrgbHex);
	M->SetVectorParameterValue(TEXT("BaseColor"), Color);
	M->SetScalarParameterValue(TEXT("Roughness"), Roughness);
	M->SetScalarParameterValue(TEXT("Pattern"), Pattern);
	M->SetScalarParameterValue(TEXT("Wet"), 0.85f);
	Materials.Add(Key, M);
	return M;
}

// ------------------------------------------------------------------ construction

void ADeeRoomStage::BuildSet()
{
	LoadShared();
	StreetMaterial = LoadOptional(TEXT("/Game/ShortStack/Materials/M_Street.M_Street"));
	Fronts.Reset();
	LitWindows.Reset();
	LitLevels.Reset();
	Streetlights.Reset();
	WindowSets.Reset();
	ArmchairFrame = nullptr;
	BoxesGroup = nullptr;
	NoteWidget = nullptr;
	TvPicture = nullptr;
	TvLight = nullptr;
	LampGlow = nullptr;
	LampShade = nullptr;
	LampLight = nullptr;
	LeftWash = nullptr;
	LeftFill = nullptr;
	LampHeads = nullptr;
	NeonPink = nullptr;
	WashSpill = nullptr;
	BuildRoom();
	BuildWindows();
	BuildFurniture();
	// The player's own things on her coffee table: the laptop where the table's made room for it, the mouse, the phone.
	BuildWorkspace(LaptopAt);
	BuildArms();
	BuildDressing();
	AddCan();
	BuildRoomLights();
	BuildScreenLight();
	BuildLens();
	BuildSkyline();
	BuildRainSheets(FVector(FrontX + WallT + 250.0, -140.0, 150.0), 2000.0f);
	BuildStreet();
	BuildFronts();
	BuildStreetlights();
	WindowSets.Reset();
	BuiltSetVersion = SetVersion;
	AppliedDawn = -1.0f;
	ApplyHour();
}

void ADeeRoomStage::BuildRoom()
{
	// An old tenement's front room: sage paint over plaster, oak boards, a ceiling a little higher than they build now,
	// the trim painted cream so many times its edges are soft.
	UMaterialInterface* Wall = Surface(TEXT("DeeWall"), 0x8a9479, 0.9f, 0.0f, PatternPlaster);
	UMaterialInterface* Floor = Surface(TEXT("DeeFloor"), 0x6b4a30, 0.65f, 0.0f, PatternFloor);
	UMaterialInterface* Ceil = Surface(TEXT("DeeCeiling"), 0xc6c0b0, 0.95f, 0.0f, PatternPlaster);
	UMaterialInterface* Trim = Surface(TEXT("DeeTrim"), 0xd9cfb6, 0.5f);
	const double T = 20.0; // the inside walls are thick so Lumen does not leak light
	const double OutX = FrontX + WallT;
	BoxMinMax(Floor, FVector(BackX - T, LeftY - T, -T), FVector(OutX, RightY + T, 0.0));
	BoxMinMax(Ceil, FVector(BackX - T, LeftY - T, Ceiling), FVector(OutX, RightY + T, Ceiling + T));
	BoxMinMax(Wall, FVector(BackX - T, LeftY - T, 0.0), FVector(BackX, RightY + T, Ceiling));
	BoxMinMax(Wall, FVector(BackX, LeftY - T, 0.0), FVector(FrontX, LeftY, Ceiling));
	BoxMinMax(Wall, FVector(BackX, RightY, 0.0), FVector(FrontX, RightY + T, Ceiling));
	// The street wall around its two windows.
	const double Head = WinSill + WinH;
	double Y = LeftY - T;
	for (int32 I = 1; I >= 0; --I)
	{
		const double Y0 = WinY[I] - WinW * 0.5;
		const double Y1 = WinY[I] + WinW * 0.5;
		BoxMinMax(Wall, FVector(FrontX, Y, 0.0), FVector(OutX, Y0, Ceiling));
		BoxMinMax(Wall, FVector(FrontX, Y0, 0.0), FVector(OutX, Y1, WinSill));
		BoxMinMax(Wall, FVector(FrontX, Y0, Head), FVector(OutX, Y1, Ceiling));
		Y = Y1;
	}
	BoxMinMax(Wall, FVector(FrontX, Y, 0.0), FVector(OutX, RightY + T, Ceiling));
	// Baseboards, the picture rail (not across the windows), the crown.
	for (const double Z : {0.0, 222.0, Ceiling - 7.0})
	{
		const bool bRail = Z > 0.0 && Z < WinSill + WinH;
		const double H = Z == 0.0 ? 14.0 : (bRail ? 3.0 : 7.0);
		const double D = Z == 0.0 ? 1.6 : (bRail ? 2.0 : 5.0);
		BoxMinMax(Trim, FVector(BackX, LeftY, Z), FVector(BackX + D, RightY, Z + H));
		BoxMinMax(Trim, FVector(BackX, LeftY, Z), FVector(FrontX, LeftY + D, Z + H));
		BoxMinMax(Trim, FVector(BackX, RightY - D, Z), FVector(FrontX, RightY, Z + H));
		if (!bRail)
		{
			BoxMinMax(Trim, FVector(FrontX - D, LeftY, Z), FVector(FrontX, RightY, Z + H));
		}
	}
	// The ceiling's light, a frosted dish she never turns on.
	Post(Surface(TEXT("DeeDish"), 0xe6e1d6, 0.3f), FVector(-30.0, -60.0, Ceiling - 9.0), 19.0, 6.0);
	Post(Trim, FVector(-30.0, -60.0, Ceiling - 3.0), 6.0, 3.0);
}

void ADeeRoomStage::BuildWindows()
{
	UMaterialInterface* Paint = Surface(TEXT("DeeTrim"), 0xd9cfb6, 0.5f);
	UMaterialInterface* Shade = Surface(TEXT("DeeShade"), 0xd8cdb0, 0.9f, 0.0f, PatternFabric);
	UMaterialInstanceDynamic* Glass = Special(GlassMaterial, TEXT("DeeGlass"));
	const double PaneW = WinW - 10.0;
	const double PaneH = WinH - 10.0;
	if (Glass)
	{
		// M_RainGlass lays its drops out over the pane from its pivot (WidthCm x HeightCm), as the apartment's does.
		Glass->SetScalarParameterValue(TEXT("WidthCm"), static_cast<float>(PaneW));
		Glass->SetScalarParameterValue(TEXT("HeightCm"), static_cast<float>(PaneH));
		Glass->SetScalarParameterValue(TEXT("Aspect"), static_cast<float>(PaneW / PaneH));
		TimedMaterials.Add(Glass);
	}
	const double Head = WinSill + WinH;
	for (int32 I = 0; I < 2; ++I)
	{
		const double Y = WinY[I];
		const double Y0 = Y - WinW * 0.5;
		const double Y1 = Y + WinW * 0.5;
		// SM_DR_Window: the painted double-hung window, its stool and roller shade (its origin on the floor under the
		// window, its room side to the room), set into the wall's reveal. Its own glass is left out: the rain on it is
		// the stage's (the pane below), which lays its drops out from the pane's middle.
		if (!Prop(TEXT("SM_DR_Window"), nullptr, FVector(FrontX + 10.0, Y, 0.0), 180.0f))
		{
			// The stool and apron, the casing round the opening, the frame and the two sashes in the reveal, the shade
			// pulled part way.
			Block(Paint, FVector(FrontX - 6.0, Y0 - 8.0, WinSill - 4.0), FVector(FrontX + 14.0, Y1 + 8.0, WinSill));
			Block(Paint, FVector(FrontX - 2.0, Y0 - 4.0, WinSill - 14.0), FVector(FrontX, Y1 + 4.0, WinSill - 4.0));
			Block(Paint, FVector(FrontX - 2.0, Y0 - 9.0, WinSill), FVector(FrontX, Y0, Head + 9.0));
			Block(Paint, FVector(FrontX - 2.0, Y1, WinSill), FVector(FrontX, Y1 + 9.0, Head + 9.0));
			Block(Paint, FVector(FrontX - 2.5, Y0 - 11.0, Head), FVector(FrontX, Y1 + 11.0, Head + 11.0));
			Block(Paint, FVector(FrontX + 12.0, Y0, WinSill), FVector(FrontX + 26.0, Y0 + 4.0, Head));
			Block(Paint, FVector(FrontX + 12.0, Y1 - 4.0, WinSill), FVector(FrontX + 26.0, Y1, Head));
			Block(Paint, FVector(FrontX + 12.0, Y0, Head - 5.0), FVector(FrontX + 26.0, Y1, Head));
			const double Mid = WinSill + WinH * 0.5;
			// The lower sash (inside), then the upper (outside), each a frame of stiles and rails.
			for (int32 S = 0; S < 2; ++S)
			{
				const double X0 = FrontX + (S == 0 ? 14.0 : 19.0);
				const double X1 = X0 + 4.0;
				const double Z0 = S == 0 ? WinSill + 1.0 : Mid - 2.0;
				const double Z1 = S == 0 ? Mid + 2.0 : Head - 5.0;
				Block(Paint, FVector(X0, Y0 + 4.0, Z0), FVector(X1, Y0 + 9.0, Z1));
				Block(Paint, FVector(X0, Y1 - 9.0, Z0), FVector(X1, Y1 - 4.0, Z1));
				Block(Paint, FVector(X0, Y0 + 4.0, Z0), FVector(X1, Y1 - 4.0, Z0 + (S == 0 ? 7.0 : 3.0)));
				Block(Paint, FVector(X0, Y0 + 4.0, Z1 - (S == 0 ? 3.0 : 5.0)), FVector(X1, Y1 - 4.0, Z1));
			}
			const double Pulled = I == 0 ? 30.0 : 46.0;
			Block(Shade, FVector(FrontX + 8.0, Y0 + 3.0, Head - Pulled), FVector(FrontX + 8.6, Y1 - 3.0, Head - 4.0), nullptr, false);
			Post(Paint, FVector(FrontX + 8.3, Y, Head - Pulled - 2.0), 0.9, 2.5, nullptr, false);
		}
		if (Glass)
		{
			// The engine plane faces +Z: pitched 90 it faces the room.
			AddMesh(PlaneMesh, Glass, FVector(FrontX + 21.0, Y, WinSill + WinH * 0.5), FVector(PaneH / 100.0, PaneW / 100.0, 1.0), FRotator(90.0f, 0.0f, 0.0f), nullptr, false);
		}
	}
}

void ADeeRoomStage::BuildFurniture()
{
	// --- The couch: rust velour, rolled arms, the middle seat broken down; the throw over its back, her pillows at
	// the right end and the player's bed pillow on a folded army blanket at the left (dee_room.py build_couch).
	USceneComponent* Couch = Frame(CouchAt, 0.0f);
	if (!Prop(TEXT("SM_DR_Couch"), Couch))
	{
		UMaterialInterface* Velour = Surface(TEXT("DeeVelour"), 0x8a3f26, 0.85f, 0.0f, PatternFabric);
		UMaterialInterface* Feet = Surface(TEXT("DeeFeet"), 0x2a1d14, 0.5f);
		for (const double Fx : {-38.0, 36.0})
		{
			for (const double Fy : {-95.0, 0.0, 95.0})
			{
				Post(Feet, FVector(Fx, Fy, 0.0), 3.5, 9.5, Couch);
			}
		}
		Block(Velour, FVector(-45.5, -87.3, 9.5), FVector(42.0, 87.3, 31.0), Couch);
		Block(Velour, FVector(-45.5, -103.0, 9.5), FVector(-27.0, 103.0, 86.0), Couch);
		for (const double S : {-1.0, 1.0})
		{
			Block(Velour, FVector(-45.5, S * 87.3, 9.5), FVector(43.0, S * 103.0, 58.0), Couch);
			AddMesh(CylinderMesh, Velour, FVector(-1.0, S * 95.0, 58.0), FVector(0.17, 0.17, 0.88), FRotator(90.0f, 0.0f, 0.0f), Couch);
		}
		const double Cush = 2.0 * 87.3 / 3.0;
		for (int32 K = 0; K < 3; ++K)
		{
			const double Y0 = -87.3 + K * Cush;
			Block(Velour, FVector(-14.3, Y0 + 0.6, 31.0), FVector(43.0, Y0 + Cush - 0.6, K == 1 ? 42.0 : 47.0), Couch);
			Block(Velour, FVector(-27.0, Y0 + 0.6, 31.0), FVector(-9.0, Y0 + Cush - 0.6, K == 1 ? 80.0 : 83.0), Couch);
		}
		Block(Surface(TEXT("DeeThrow"), 0x7d5a3c, 0.95f, 0.0f, PatternFabric), FVector(-47.0, -62.0, 72.0), FVector(-24.0, 38.0, 87.5), Couch);
		Block(Surface(TEXT("DeePillowGold"), 0xc4952f, 0.9f, 0.0f, PatternFabric), FVector(-14.0, 58.0, 47.0), FVector(2.0, 84.0, 74.0), Couch);
		Block(Surface(TEXT("DeePillowRose"), 0xa8545a, 0.9f, 0.0f, PatternFabric), FVector(-10.0, 34.0, 47.0), FVector(4.0, 58.0, 70.0), Couch);
		Block(Surface(TEXT("DeeArmyBlanket"), 0x4b5233, 0.95f, 0.0f, PatternFabric), FVector(-12.0, -86.0, 47.0), FVector(40.0, -38.0, 52.0), Couch);
		Block(Surface(TEXT("DeeBedPillow"), 0xe6e1d6, 0.9f, 0.0f, PatternFabric), FVector(-9.0, -85.0, 52.0), FVector(26.0, -44.0, 63.0), Couch);
	}

	// --- The coffee table: honey oak, ring stains, a deck of cards and the ashtray with her keys; the laptop's place is
	// the middle of its top, a hand's width back from the couch's side.
	USceneComponent* Table = Frame(TableAt, 180.0f);
	if (!Prop(TEXT("SM_DR_CoffeeTable"), Table))
	{
		UMaterialInterface* Oak = Surface(TEXT("DeeOak"), 0x9a6a3a, 0.45f, 0.0f, PatternWood);
		Block(Oak, FVector(-30.0, -55.0, TableTop - 4.0), FVector(30.0, 55.0, TableTop), Table);
		Block(Oak, FVector(-26.0, -51.0, 10.0), FVector(26.0, 51.0, 12.0), Table);
		for (const double Lx : {-26.0, 26.0})
		{
			for (const double Ly : {-51.0, 51.0})
			{
				Block(Oak, FVector(Lx - 2.5, Ly - 2.5, 0.0), FVector(Lx + 2.5, Ly + 2.5, TableTop - 4.0), Table);
			}
		}
		// Clear of the laptop, the mouse, the phone and the empties: the deck at the near right corner, the ashtray at the
		// far right, magazines on the shelf.
		Block(Surface(TEXT("DeeDeck"), 0xb31b1b, 0.5f), FVector(22.0, -54.0, TableTop), FVector(28.0, -46.0, TableTop + 1.8), Table);
		Post(Surface(TEXT("DeeAshtray"), 0x2b3a3a, 0.08f, 0.2f), FVector(-22.0, -48.0, TableTop), 5.5, 2.4, Table);
		Block(Surface(TEXT("DeeMagazine"), 0x5c7a9c, 0.6f), FVector(-18.0, -30.0, 12.0), FVector(12.0, 8.0, 13.2), Table);
		Block(Surface(TEXT("DeeCrossword"), 0xd9d3c4, 0.8f), FVector(-12.0, 4.0, 13.2), FVector(16.0, 38.0, 13.6), Table);
	}

	// --- Her recliner, turned to the TV: brown vinyl, cracked where she sits, her cardigan over the left arm.
	ArmchairFrame = Frame(ArmchairAt, ArmchairYaw);
	if (!Prop(TEXT("SM_DR_Armchair"), ArmchairFrame))
	{
		UMaterialInterface* Vinyl = Surface(TEXT("DeeVinyl"), 0x4a3226, 0.42f);
		Block(Vinyl, FVector(-47.5, -40.0, 8.0), FVector(40.0, 40.0, 36.0), ArmchairFrame);
		Block(Vinyl, FVector(-30.0, -32.0, 36.0), FVector(42.0, 32.0, ArmchairSeat), ArmchairFrame);
		AddMesh(CubeMesh, Vinyl, FVector(-40.0, 0.0, 70.0), FVector(0.16, 0.8, 0.64), FRotator(-9.0f, 0.0f, 0.0f), ArmchairFrame);
		for (const double S : {-1.0, 1.0})
		{
			Block(Vinyl, FVector(-47.5, S * 32.0, 8.0), FVector(38.0, S * 45.0, 62.0), ArmchairFrame);
		}
		Block(Surface(TEXT("DeeChairBase"), 0x1c1712, 0.6f), FVector(-40.0, -36.0, 0.0), FVector(32.0, 36.0, 8.0), ArmchairFrame);
		Block(Surface(TEXT("DeeCardigan"), 0xc9b99a, 0.95f, 0.0f, PatternFabric), FVector(-30.0, -47.0, 50.0), FVector(20.0, -30.0, 63.5), ArmchairFrame);
	}

	// --- The TV in the corner: a 27-inch CRT on a laminate stand, the DVD player and the VCR blinking 12:00.
	USceneComponent* Tv = Frame(TvAt, TvYaw);
	UStaticMeshComponent* TvFace = nullptr;
	if (Prop(TEXT("SM_DR_TVStand"), Tv))
	{
		TvFace = Prop(TEXT("SM_DR_TVStand_Glow"), Tv);
	}
	else
	{
		UMaterialInterface* Laminate = Surface(TEXT("DeeLaminate"), 0x3b2a1e, 0.55f, 0.0f, PatternWood);
		UMaterialInterface* Plastic = Surface(TEXT("DeeTvPlastic"), 0x1c1d1f, 0.45f);
		Block(Laminate, FVector(-22.0, -45.0, 0.0), FVector(22.0, 45.0, 4.0), Tv);
		Block(Laminate, FVector(-22.0, -45.0, 51.0), FVector(22.0, 45.0, 55.0), Tv);
		Block(Laminate, FVector(-22.0, -45.0, 26.0), FVector(22.0, 45.0, 28.0), Tv);
		for (const double S : {-1.0, 1.0})
		{
			Block(Laminate, FVector(-22.0, S * 43.0, 0.0), FVector(22.0, S * 45.0, 55.0), Tv);
		}
		Block(Plastic, FVector(-16.0, -21.0, 28.0), FVector(18.0, 21.0, 35.0), Tv);
		Block(Plastic, FVector(-16.0, -21.0, 4.0), FVector(18.0, 21.0, 13.0), Tv);
		Block(Surface(TEXT("DeeVcr"), 0x48ff7a, 0.4f, 0.0f, 0.0f, 3.0f), FVector(18.05, 10.0, 8.0), FVector(18.3, 16.0, 9.6), Tv, false);
		Block(Plastic, FVector(-26.0, -32.0, 55.0), FVector(18.0, 32.0, 104.0), Tv);
		AddMesh(CylinderMesh, Plastic, FVector(-10.0, -8.0, 118.0), FVector(0.012, 0.012, 0.32), FRotator(0.0f, 0.0f, 28.0f), Tv);
		AddMesh(CylinderMesh, Plastic, FVector(-10.0, 8.0, 118.0), FVector(0.012, 0.012, 0.32), FRotator(0.0f, 0.0f, -28.0f), Tv);
		TvFace = Block(Plastic, FVector(18.0, -25.0, 60.0), FVector(18.6, 25.0, 99.0), Tv, false);
	}
	// The picture, on the CRT's face: dark glass while it's off (UpdateTv).
	TvPicture = Surface(TEXT("DeeTvPicture"), 0x101412, 0.15f);
	TvScreenAt = TvAt + FRotator(0.0f, TvYaw, 0.0f).RotateVector(FVector(19.0, 0.0, 80.0));
	if (TvFace)
	{
		for (int32 Slot = 0; Slot < FMath::Max(1, TvFace->GetNumMaterials()); ++Slot)
		{
			TvFace->SetMaterial(Slot, TvPicture);
		}
		TvFace->SetCastShadow(false);
		if (const UStaticMesh* FaceMesh = TvFace->GetStaticMesh())
		{
			TvScreenAt = GetActorTransform().InverseTransformPosition(TvFace->GetComponentTransform().TransformPosition(FaceMesh->GetBoundingBox().GetCenter()));
		}
	}

	// --- Her floor lamp at the couch's end: tarnished brass, a pleated shade knocked askew; its bulb and the shade's
	// lining glow (SM_DR_FloorLamp_Glow), the shade's outside in its own bake (dimmed with the lamp off, ApplyHour).
	USceneComponent* Lamp = Frame(LampAt, 30.0f);
	LampShade = Prop(TEXT("SM_DR_FloorLamp"), Lamp);
	LampGlow = Surface(TEXT("DeeLampGlow"), 0xffd9a8, 0.9f, 0.0f, 0.0f, 0.0f);
	FVector Bulb(0.0, 0.0, 148.0);
	if (LampShade)
	{
		if (UStaticMeshComponent* Lit = Prop(TEXT("SM_DR_FloorLamp_Glow"), Lamp))
		{
			for (int32 Slot = 0; Slot < FMath::Max(1, Lit->GetNumMaterials()); ++Slot)
			{
				Lit->SetMaterial(Slot, LampGlow);
			}
			Lit->SetCastShadow(false);
			if (const UStaticMesh* LitMesh = Lit->GetStaticMesh())
			{
				Bulb = Lamp->GetRelativeTransform().InverseTransformPosition(
					GetActorTransform().InverseTransformPosition(Lit->GetComponentTransform().TransformPosition(LitMesh->GetBoundingBox().GetCenter())));
			}
		}
	}
	else
	{
		UMaterialInterface* Brass = Surface(TEXT("DeeBrass"), 0x8a6d3b, 0.35f, 0.9f);
		Post(Brass, FVector::ZeroVector, 14.0, 3.0, Lamp);
		Post(Brass, FVector(0.0, 0.0, 3.0), 1.3, 140.0, Lamp);
		// The shade lets the light through its open top and bottom: it glows, and casts no shadow round the bulb.
		AddMesh(CylinderMesh, LampGlow, FVector(1.5, 0.0, 152.0), FVector(0.4, 0.4, 0.26), FRotator(0.0f, 0.0f, 4.0f), Lamp, false);
	}
	LampLight = NewPart<UPointLightComponent>(Lamp);
	LampLight->SetRelativeLocation(Bulb);
	LampLight->SetIntensityUnits(ELightUnits::Candelas);
	LampLight->SetLightColor(Srgb(0xffb46b));
	LampLight->SetAttenuationRadius(700.0f);
	LampLight->SetSourceRadius(5.0f);
	LampLight->SetCastShadows(true);
	LampLight->SetVolumetricScatteringIntensity(0.0f);

	// --- The radiator under the main window (a cast-iron column, painted over, a pan of water on it) and the rug.
	USceneComponent* Radiator = Frame(FVector(FrontX - 12.0, WinY[0], 0.0), 180.0f);
	if (!Prop(TEXT("SM_DR_Radiator"), Radiator))
	{
		UMaterialInterface* Iron = Surface(TEXT("DeeRadiator"), 0xb7b2a5, 0.5f, 0.6f);
		for (int32 K = 0; K < 13; ++K)
		{
			const double Fy = -42.0 + K * 7.0;
			Block(Iron, FVector(-7.0, Fy - 2.6, 9.0), FVector(7.0, Fy + 2.6, 70.0), Radiator);
		}
		Block(Iron, FVector(-5.0, -44.0, 12.0), FVector(5.0, 44.0, 16.0), Radiator);
		Block(Iron, FVector(-5.0, -44.0, 63.0), FVector(5.0, 44.0, 67.0), Radiator);
		for (const double Fy : {-40.0, 40.0})
		{
			Block(Iron, FVector(-6.0, Fy - 3.0, 0.0), FVector(6.0, Fy + 3.0, 9.0), Radiator);
		}
		Post(Surface(TEXT("DeePan"), 0x9a9a96, 0.3f, 0.9f), FVector(0.0, 18.0, 70.0), 11.0, 4.0, Radiator);
	}
	USceneComponent* Rug = Frame(FVector(-75.0, 0.0, 0.0), 180.0f);
	if (!Prop(TEXT("SM_DR_Rug"), Rug))
	{
		Block(Surface(TEXT("DeeRug"), 0x6e2a2a, 1.0f, 0.0f, PatternFabric), FVector(-85.0, -120.0, 0.0), FVector(85.0, 120.0, 1.0), Rug, false);
		Block(Surface(TEXT("DeeRugBorder"), 0x2c3a52, 1.0f, 0.0f, PatternFabric), FVector(-78.0, -113.0, 1.0), FVector(78.0, 113.0, 1.1), Rug, false);
		Block(Surface(TEXT("DeeRug"), 0x6e2a2a, 1.0f, 0.0f, PatternFabric), FVector(-72.0, -107.0, 1.1), FVector(72.0, 107.0, 1.2), Rug, false);
	}

	// --- Her shelves on the left wall: paperbacks, photographs, the chip frame, a dealer button, the plaque, a pothos.
	USceneComponent* Shelf = Frame(FVector(35.0, LeftY + 16.0, 0.0), 90.0f);
	if (!Prop(TEXT("SM_DR_Shelf"), Shelf))
	{
		UMaterialInterface* Pine = Surface(TEXT("DeePine"), 0x7a5532, 0.55f, 0.0f, PatternWood);
		Block(Pine, FVector(-15.0, -55.0, 0.0), FVector(15.0, -53.0, 190.0), Shelf);
		Block(Pine, FVector(-15.0, 53.0, 0.0), FVector(15.0, 55.0, 190.0), Shelf);
		Block(Pine, FVector(-15.0, -55.0, 0.0), FVector(-13.0, 55.0, 190.0), Shelf);
		FDice Dice;
		Dice.S = 1998;
		for (int32 Row = 0; Row < 5; ++Row)
		{
			const double Z = 4.0 + Row * 46.0;
			Block(Pine, FVector(-13.0, -53.0, Z - 2.0), FVector(15.0, 53.0, Z), Shelf);
			if (Row == 4)
			{
				break;
			}
			// Books along most of each shelf; the second shelf keeps room for the photographs and her mementos.
			double Y = -51.0;
			const double End = Row == 2 ? 5.0 : 50.0;
			while (Y < End)
			{
				const double W = 2.2 + 3.0 * Dice.Next();
				const double H = 18.0 + 9.0 * Dice.Next();
				static const uint32 Spines[6] = {0x6e2a2a, 0x2c4a6e, 0xc9b26b, 0x3f5a3a, 0x1c1c1c, 0xd8d2c0};
				const uint32 Spine = Spines[static_cast<int32>(Dice.Next() * 5.99)];
				Block(Surface(*FString::Printf(TEXT("DeeBook%06x"), Spine), Spine, 0.7f), FVector(-11.0, Y, Z), FVector(10.0, Y + W, Z + H), Shelf);
				Y += W + 0.3;
			}
			if (Row == 2)
			{
				// A framed photo of a younger Dee in a dealer's vest, the dealer button, the chip frame.
				Block(Surface(TEXT("DeeFrame"), 0x2a1a10, 0.4f), FVector(-6.0, 12.0, Z), FVector(-3.0, 28.0, Z + 21.0), Shelf);
				Block(Surface(TEXT("DeePhoto"), 0x8a8578, 0.6f), FVector(-2.95, 13.5, Z + 1.5), FVector(-2.9, 26.5, Z + 19.5), Shelf);
				Post(Surface(TEXT("DeeButton"), 0xf2efe6, 0.3f), FVector(4.0, 36.0, Z), 4.0, 1.2, Shelf);
				Block(Surface(TEXT("DeeChipFrame"), 0x1d1d1d, 0.35f), FVector(-6.0, 41.0, Z), FVector(-3.0, 51.0, Z + 13.0), Shelf);
			}
		}
		AddMesh(SphereMesh, Surface(TEXT("DeePothos"), 0x3d6b2f, 0.8f), FVector(0.0, -30.0, 196.0), FVector(0.34, 0.5, 0.18), FRotator::ZeroRotator, Shelf);
	}

	// --- The kitchenette at the back of the left wall (a painted base cabinet, the hot plate and kettle), her small
	// fridge beside it, the cat's bowls on the floor.
	USceneComponent* Kitchen = Frame(FVector(-165.0, LeftY + 30.0, 0.0), 90.0f);
	if (!Prop(TEXT("SM_DR_Kitchenette"), Kitchen))
	{
		UMaterialInterface* Cabinet = Surface(TEXT("DeeCabinet"), 0x6f8a86, 0.6f);
		Block(Cabinet, FVector(-29.0, -64.0, 0.0), FVector(28.0, 64.0, 88.0), Kitchen);
		Block(Surface(TEXT("DeeLaminateTop"), 0xb9b1a0, 0.4f), FVector(-30.0, -65.0, 88.0), FVector(30.0, 65.0, 91.0), Kitchen);
		Block(Surface(TEXT("DeeHotPlate"), 0x202224, 0.35f, 0.3f), FVector(-20.0, 8.0, 91.0), FVector(12.0, 48.0, 97.0), Kitchen);
		Post(Surface(TEXT("DeeKettle"), 0xb0aaa0, 0.25f, 0.9f), FVector(-4.0, 18.0, 97.0), 8.0, 17.0, Kitchen);
		Post(Surface(TEXT("DeeMug"), 0xd8d2c4, 0.3f), FVector(-10.0, -30.0, 91.0), 4.2, 9.5, Kitchen);
		Block(Cabinet, FVector(-30.0, -60.0, 150.0), FVector(-27.0, 60.0, 152.0), Kitchen);
		Block(Cabinet, FVector(-30.0, -60.0, 152.0), FVector(-6.0, 60.0, 154.0), Kitchen);
	}
	UMaterialInterface* Enamel = Surface(TEXT("DeeFridge"), 0xd8d4c8, 0.35f);
	Block(Enamel, FVector(-96.0, LeftY, 0.0), FVector(-40.0, LeftY + 62.0, 138.0));
	Block(Surface(TEXT("DeeChrome"), 0x9c9c98, 0.25f, 0.9f), FVector(-44.0, LeftY + 62.0, 70.0), FVector(-42.0, LeftY + 64.0, 110.0));
	Post(Surface(TEXT("DeeBowl"), 0xb03030, 0.4f), FVector(-60.0, LeftY + 84.0, 0.0), 7.0, 4.0);
	Post(Surface(TEXT("DeeBowlWater"), 0x3a6ea8, 0.3f), FVector(-76.0, LeftY + 86.0, 0.0), 7.0, 4.0);

	// --- A snake plant by the left window.
	USceneComponent* Plant = Frame(FVector(135.0, -365.0, 0.0), 200.0f);
	if (!Prop(TEXT("SM_DR_Plant"), Plant))
	{
		Post(Surface(TEXT("DeePot"), 0x2f5d5a, 0.25f), FVector::ZeroVector, 14.0, 30.0, Plant);
		UMaterialInterface* Leaf = Surface(TEXT("DeeLeaf"), 0x3d6b2f, 0.6f);
		for (int32 K = 0; K < 7; ++K)
		{
			const float A = K * 51.0f;
			AddMesh(CubeMesh, Leaf, FVector(FMath::Cos(FMath::DegreesToRadians(A)) * 4.0, FMath::Sin(FMath::DegreesToRadians(A)) * 4.0, 55.0 + K * 3.0),
			        FVector(0.06, 0.012, 0.5 + 0.05 * K), FRotator(4.0f * (K % 3), A, 6.0f * ((K % 2) * 2 - 1)), Plant);
		}
	}

	// --- Doors: the apartment's (chain, deadbolt, peephole) on the right wall by the back, and her bedroom's, cream, on
	// the back wall behind the couch's left.
	USceneComponent* Hall = Frame(HallDoorAt, -90.0f);
	if (!Prop(TEXT("SM_DR_Door"), Hall))
	{
		UMaterialInterface* Green = Surface(TEXT("DeeDoorGreen"), 0x3f4f43, 0.45f);
		UMaterialInterface* Trim = Surface(TEXT("DeeTrim"), 0xd9cfb6, 0.5f);
		UMaterialInterface* Brass = Surface(TEXT("DeeBrass"), 0x8a6d3b, 0.35f, 0.9f);
		Block(Green, FVector(0.0, -45.0, 0.0), FVector(4.5, 45.0, 206.0), Hall);
		Block(Trim, FVector(0.0, -54.0, 0.0), FVector(2.0, -45.0, 215.0), Hall);
		Block(Trim, FVector(0.0, 45.0, 0.0), FVector(2.0, 54.0, 215.0), Hall);
		Block(Trim, FVector(0.0, -56.0, 206.0), FVector(2.5, 56.0, 217.0), Hall);
		AddMesh(SphereMesh, Brass, FVector(7.0, 36.0, 98.0), FVector(0.055), FRotator::ZeroRotator, Hall);
		Post(Brass, FVector(5.0, 36.0, 120.0), 2.4, 3.0, Hall);
		Block(Brass, FVector(4.5, 28.0, 140.0), FVector(5.5, 42.0, 141.0), Hall);
		Post(Brass, FVector(4.6, 0.0, 152.0), 0.8, 0.5, Hall);
	}
	{
		USceneComponent* Bedroom = Frame(FVector(BackX, BedroomDoorY, 0.0), 0.0f);
		UMaterialInterface* Trim = Surface(TEXT("DeeTrim"), 0xd9cfb6, 0.5f);
		Block(Surface(TEXT("DeeBedroomDoor"), 0xcfc5ad, 0.5f, 0.0f, PatternWood), FVector(0.0, -42.0, 0.0), FVector(4.0, 42.0, 203.0), Bedroom);
		for (const double S : {-1.0, 1.0})
		{
			Block(Trim, FVector(0.0, S * 42.0, 0.0), FVector(2.0, S * 51.0, 212.0), Bedroom);
		}
		Block(Trim, FVector(0.0, -53.0, 203.0), FVector(2.5, 53.0, 214.0), Bedroom);
		AddMesh(SphereMesh, Surface(TEXT("DeeBrass"), 0x8a6d3b, 0.35f, 0.9f), FVector(6.5, -34.0, 96.0), FVector(0.05), FRotator::ZeroRotator, Bedroom);
	}

	// --- The player's boxes by the couch's right end: what's left of 1812 that isn't in storage.
	BoxesGroup = NewPart<USceneComponent>();
	AddCarton(FVector(-206.0, 134.0, 0.0), FVector(42.0, 52.0, 40.0), 6.0f, false, BoxesGroup);
	AddCarton(FVector(-204.0, 136.0, 40.0), FVector(36.0, 44.0, 32.0), 172.0f, true, BoxesGroup);
	BoxesGroup->SetVisibility(false, true);
}

void ADeeRoomStage::BuildDressing()
{
	// An open umbrella drying canopy-down by the door, a puddle under it.
	const FVector UmbrellaAt(-112.0, RightY - 50.0, 0.0);
	const FQuat Tip = FQuat(FRotator(0.0f, 180.0f, 0.0f)) * FQuat(FRotator(-118.0f, 0.0f, 0.0f));
	if (UStaticMesh* Umbrella = LoadProp(TEXT("SM_Umbrella")))
	{
		// Its origin is the grip and the canopy opens up +Z: tipped over, its lowest point set on the floor.
		const FTransform Pose(Tip * FQuat(PropFacing(0.0f)), FVector::ZeroVector);
		const FBox Box = Umbrella->GetBoundingBox().TransformBy(Pose);
		AddMesh(Umbrella, nullptr, UmbrellaAt - FVector(0.0, 0.0, Box.Min.Z), FVector(1.0), (Tip * FQuat(PropFacing(0.0f))).Rotator());
	}
	else
	{
		UMaterialInterface* Nylon = Surface(TEXT("DeeUmbrella"), 0x111214, 0.35f);
		// Tipped as it is, the canopy's rim (50 cm round, 50 cm up the shaft) comes down to the floor with the grip 68 up.
		USceneComponent* Brolly = NewPart<USceneComponent>();
		Brolly->SetRelativeLocationAndRotation(UmbrellaAt + FVector(0.0, 0.0, 68.0), Tip.Rotator());
		// The grip at the frame's origin, the shaft up to the canopy's crown (the engine cone's tip at its pivot + 50).
		AddMesh(CylinderMesh, Nylon, FVector(0.0, 0.0, 38.0), FVector(0.016, 0.016, 0.76), FRotator::ZeroRotator, Brolly);
		AddMesh(ConeMesh, Nylon, FVector(0.0, 0.0, 66.0), FVector(1.0, 1.0, 0.32), FRotator::ZeroRotator, Brolly);
	}
	Post(Surface(TEXT("DeePuddle"), 0x141618, 0.04f), UmbrellaAt + FVector(-52.0, 0.0, 0.0), 30.0, 0.15, nullptr, false);

	// Two pictures over the couch (a harbor, the ridge in snow), crooked as she hung them.
	UMaterialInterface* FrameWood = Surface(TEXT("DeeFrame"), 0x2a1a10, 0.4f);
	const struct
	{
		double Y, Z, W, H;
		uint32 Art;
		float Roll;
	} Pictures[] = {{-62.0, 150.0, 46.0, 34.0, 0x5d7486, 1.5f}, {40.0, 158.0, 30.0, 40.0, 0x9aa3a6, -1.0f}};
	for (const auto& P : Pictures)
	{
		USceneComponent* Hung = NewPart<USceneComponent>();
		Hung->SetRelativeLocationAndRotation(FVector(BackX, P.Y, P.Z), FRotator(0.0f, 0.0f, P.Roll));
		Block(FrameWood, FVector(0.0, -P.W * 0.5, -P.H * 0.5), FVector(2.5, P.W * 0.5, P.H * 0.5), Hung);
		Block(Surface(*FString::Printf(TEXT("DeeArt%06x"), P.Art), P.Art, 0.7f), FVector(2.5, -P.W * 0.5 + 3.0, -P.H * 0.5 + 3.0), FVector(2.6, P.W * 0.5 - 3.0, P.H * 0.5 - 3.0),
		      Hung);
	}
	// Her coat on a hook by the door.
	Block(Surface(TEXT("DeeCoat"), 0x3a2f2a, 0.9f, 0.0f, PatternFabric), FVector(-229.0, RightY - 14.0, 95.0), FVector(-208.0, RightY - 2.0, 170.0));

	// The Embercrest's wall calendar on the street wall right of the window (she deals their Sunday), drawn from the
	// career as the apartment's is; and Dee's note by the laptop when she isn't home.
	{
		const FVector CalAt(FrontX - 0.4, 112.0, 152.0);
		const FVector2D CalCm(50.0, 70.0);
		CalendarWidget = AddWidget(CalAt, Facing(FVector::BackwardVector, FVector::UpVector), CalCm, CalendarResolution, false, false);
		CalendarWidget->SetTintColorAndOpacity(FLinearColor(0.11f, 0.105f, 0.1f, 1.0f));
		AddMesh(CylinderMesh, Surface(TEXT("Nail"), 0x9a9890, 0.35f, 1.0f), FVector(FrontX - 0.6, CalAt.Y, CalAt.Z + CalCm.Y * 0.5 + 0.4), FVector(0.005, 0.005, 0.012),
		        FRotator(90.0f, 0.0f, 0.0f));
	}
	const double NoteTurn = 0.22;
	NoteWidget = AddWidget(FVector(LaptopAt.X - 21.0, -33.0, TableTop + 0.08), Facing(FVector::UpVector, FVector(FMath::Cos(NoteTurn), -FMath::Sin(NoteTurn), 0.0)),
		FVector2D(7.6, 7.6), FIntPoint(256, 256), true, false);
	NoteWidget->SetVisibility(false);
}

void ADeeRoomStage::BuildRoomLights()
{
	// Her lamp and the TV are set by the hour and by her (ApplyHour, UpdateTv). The TV's light comes off its face.
	TvLight = NewPart<URectLightComponent>();
	const FVector Out = FRotator(0.0f, TvYaw, 0.0f).Vector();
	TvLight->SetRelativeLocationAndRotation(TvScreenAt + Out * 4.0, Out.Rotation());
	TvLight->SetIntensityUnits(ELightUnits::Candelas);
	TvLight->SetIntensity(0.0f);
	TvLight->SetSourceWidth(48.0f);
	TvLight->SetSourceHeight(36.0f);
	TvLight->SetBarnDoorAngle(88.0f);
	TvLight->SetAttenuationRadius(600.0f);
	TvLight->SetCastShadows(false);
	TvLight->SetSpecularScale(0.3f);
	TvLight->SetVisibility(false);

	// Each window: the grey day through it (outside, clear of the wall, tipped down into the room) and the city's cold
	// glow at night; the first window's are the stage's own (ANightOneStage::ApplyHour), the second's follow them.
	auto Wash = [this](double Y) {
		URectLightComponent* L = NewPart<URectLightComponent>();
		L->SetRelativeLocationAndRotation(FVector(FrontX + WallT + 40.0, Y, WinSill + WinH * 0.5), FRotator(-10.0f, 180.0f, 0.0f));
		L->SetIntensityUnits(ELightUnits::Candelas);
		L->SetIntensity(0.0f);
		L->SetLightColor(Srgb(0xc9d3e0));
		L->SetSourceWidth(static_cast<float>(WinW));
		L->SetSourceHeight(static_cast<float>(WinH));
		L->SetBarnDoorAngle(80.0f);
		L->SetAttenuationRadius(900.0f);
		L->SetCastShadows(true);
		L->SetVisibility(false);
		return L;
	};
	auto Fill = [this](double Y) {
		URectLightComponent* L = NewPart<URectLightComponent>();
		L->SetRelativeLocationAndRotation(FVector(FrontX - 4.0, Y, WinSill + WinH * 0.6), FRotator(-5.0f, 180.0f, 0.0f));
		L->SetIntensityUnits(ELightUnits::Candelas);
		L->SetIntensity(1.7f);
		L->SetLightColor(Srgb(0x5a78c8));
		L->SetSourceWidth(static_cast<float>(WinW));
		L->SetSourceHeight(static_cast<float>(WinH));
		L->SetAttenuationRadius(600.0f);
		L->SetCastShadows(false);
		return L;
	};
	DayWash = Wash(WinY[0]);
	LeftWash = Wash(WinY[1]);
	CityFill = Fill(WinY[0]);
	LeftFill = Fill(WinY[1]);
	RoomFill = NewPart<UPointLightComponent>();
	RoomFill->SetRelativeLocation(FVector(-40.0, -40.0, 250.0));
	RoomFill->SetIntensityUnits(ELightUnits::Candelas);
	RoomFill->SetIntensity(RoomFillCandela);
	RoomFill->SetLightColor(Srgb(0x223047));
	RoomFill->SetAttenuationRadius(650.0f);
	RoomFill->SetCastShadows(false);
}

void ADeeRoomStage::BuildStreet()
{
	// Fifth Street below her windows, as the street has it (StreetStage.cpp BuildGround): her side's sidewalk, the curb,
	// the road 15 cm down with its double yellow, the far curb and 1812's sidewalk; parked cars along the curbs.
	UMaterialInterface* Walk = StreetSurface(TEXT("DeeSidewalk"), 0x6f6c66, 0.75f, StreetSlabs);
	UMaterialInterface* Road = StreetSurface(TEXT("DeeAsphalt"), 0x26272a, 0.6f, StreetAsphalt);
	UMaterialInterface* Curb = StreetSurface(TEXT("DeeCurb"), 0x8a867e, 0.7f, StreetConcrete);
	const double Y0 = -6600.0;
	const double Y1 = 6600.0;
	auto Ground = [&](UMaterialInterface* M, double X0, double X1, double Top) {
		// Street X runs the other way here: X0..X1 (the street's) become StreetX-X1..StreetX-X0.
		BoxMinMax(M, FVector(StreetX - X1, Y0, Top - 30.0 - StreetZ), FVector(StreetX - X0, Y1, Top - StreetZ));
	};
	Ground(Walk, 1640.0, 2040.0, 0.0);
	Ground(Curb, 1620.0, 1640.0, 0.0);
	Ground(Road, 420.0, 1620.0, -15.0);
	Ground(Curb, 400.0, 420.0, 0.0);
	Ground(Walk, 0.0, 400.0, 0.0);
	UMaterialInterface* Yellow = Surface(TEXT("DeeRoadYellow"), 0xd9a520, 0.35f);
	for (const double Line : {1008.0, 1030.0})
	{
		Block(Yellow, FVector(StreetX - Line - 12.0, Y0, -15.0 - StreetZ), FVector(StreetX - Line, Y1, -14.6 - StreetZ), nullptr, false);
	}
	const struct
	{
		double X, Y;
		float Yaw;
		uint32 Paint;
	} Parked[] = {{1500.0, 1430.0, -90.0f, 0x6e1b24}, {1500.0, -1270.0, -90.0f, 0x2a2b2e}, {540.0, -2650.0, 90.0f, 0x1f4a3a}, {540.0, 1150.0, 90.0f, 0x7b7f86}};
	for (const auto& Car : Parked)
	{
		if (UStaticMeshComponent* Sedan = Prop(TEXT("SM_Sedan"), nullptr, FromStreet(Car.X, Car.Y, -15.0), Car.Yaw + 180.0f))
		{
			// The sedan is baked light grey: its body (the first material) takes the paint.
			Sedan->EmptyOverrideMaterials();
			TintImported(Sedan, ImportedSlot(Sedan, 0), Srgb(Car.Paint));
		}
	}

	// The light on 1812's face: the city's cold glow at night, the overcast sky's by day (ApplyHour, as the apartment
	// lights the tenement).
	FacadeFill = NewPart<URectLightComponent>();
	FacadeFill->SetRelativeLocationAndRotation(FVector(1250.0, 370.0, 250.0), FRotator(-8.0f, 0.0f, 0.0f));
	FacadeFill->SetIntensityUnits(ELightUnits::Candelas);
	FacadeFill->SetSourceWidth(3000.0f);
	FacadeFill->SetSourceHeight(2400.0f);
	FacadeFill->SetAttenuationRadius(2600.0f);
	FacadeFill->SetCastShadows(false);
	FacadeFill->SetVolumetricScatteringIntensity(0.0f);

	// The Wash & Fold's neon, two floors over her left window: its pink thrown across the road and onto 1812 (the street's
	// own light for it, as bright, but set out over the sidewalk and aimed flatter: without shadows, a light tipped as
	// steeply from right over her ceiling would come through it), and the laundromat's window's light on her sidewalk.
	NeonPink = NewPart<URectLightComponent>();
	NeonPink->SetRelativeLocationAndRotation(FromStreet(2030.0 - 195.0, 640.0, 1140.0 - 10.0), FRotator(-22.0f, 0.0f, 0.0f));
	NeonPink->SetIntensityUnits(ELightUnits::Lumens);
	NeonPink->SetSourceWidth(640.0f);
	NeonPink->SetSourceHeight(170.0f);
	NeonPink->SetAttenuationRadius(2600.0f);
	NeonPink->SetLightColor(Srgb(0xff3a8c));
	NeonPink->SetCastShadows(false);
	NeonPink->SetVolumetricScatteringIntensity(0.0f);
	WashSpill = NewPart<URectLightComponent>();
	WashSpill->SetRelativeLocationAndRotation(FromStreet(2040.0 + 70.0, 920.0, 230.0), FRotator(-35.0f, 0.0f, 0.0f));
	WashSpill->SetIntensityUnits(ELightUnits::Lumens);
	WashSpill->SetSourceWidth(1720.0f);
	WashSpill->SetSourceHeight(180.0f);
	WashSpill->SetAttenuationRadius(1000.0f);
	WashSpill->SetLightColor(Srgb(0xdff0ff));
	WashSpill->SetSpecularScale(0.25f);
	WashSpill->SetCastShadows(false);
	WashSpill->SetVolumetricScatteringIntensity(0.0f);
}

void ADeeRoomStage::FrontWindow(const FVector& Center, const FVector2D& SizeCm, int32 Kind)
{
	const int32 K = FMath::Clamp(Kind, 0, 4);
	if (WindowSets.Num() == 0)
	{
		for (int32 I = 0; I < 5; ++I)
		{
			UMaterialInstanceDynamic* M = Surface(*FString::Printf(TEXT("DeeWindow%d"), I), LitHex[I], I == 0 ? 0.06f : 0.8f, 0.0f, 0.0f, LitGlow[I]);
			UInstancedStaticMeshComponent* Set = NewPart<UInstancedStaticMeshComponent>();
			Set->SetStaticMesh(CubeMesh);
			Set->SetMaterial(0, M);
			Set->SetCastShadow(false);
			Set->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			WindowSets.Add(Set);
			if (I > 0)
			{
				LitWindows.Add(M);
				LitLevels.Add(LitGlow[I]);
			}
		}
	}
	WindowSets[K]->AddInstance(FTransform(FRotator::ZeroRotator, Center, FVector(0.01, SizeCm.X / 100.0, SizeCm.Y / 100.0)));
}

void ADeeRoomStage::WindowGrid(double X, double Y0, double Y1, double Z0, double Z1, double Pitch, double FloorH, double LitShare, int64 Seed)
{
	FDice Dice;
	Dice.S = FMath::Max<int64>(1, Seed % 2147483646);
	for (double Z = Z0; Z + 180.0 < Z1; Z += FloorH)
	{
		for (double Y = Y0 + Pitch * 0.5; Y < Y1; Y += Pitch)
		{
			const double Roll = Dice.Next();
			const int32 Kind = Roll < LitShare * 0.6 ? 1 : (Roll < LitShare * 0.8 ? 2 : (Roll < LitShare ? 3 : 0));
			FrontWindow(FVector(X - 0.5, Y, Z + 90.0), FVector2D(115.0, 170.0), Kind);
		}
	}
}

void ADeeRoomStage::BuildFronts()
{
	// Across the street, straight out of her windows: 1812 Fifth Street (facades.py), its neighbours either side. Placed
	// as the street places them (no BlenderFacing: the home side's yaw 180 there is 0 here), each with its block behind.
	UMaterialInterface* Brick = StreetSurface(TEXT("DeeBrick"), 0x6b3427, 0.85f, StreetBrick);
	UMaterialInterface* BrickDark = StreetSurface(TEXT("DeeBrickDark"), 0x4a2a24, 0.85f, StreetBrick);
	UMaterialInterface* Buff = StreetSurface(TEXT("DeeBrickBuff"), 0x8a7155, 0.85f, StreetBrick);
	UMaterialInterface* Stone = StreetSurface(TEXT("DeeStone"), 0x8c8579, 0.8f, StreetConcrete);
	const double Face = StreetX;  // the home side's building line
	const double Back = StreetX + 1500.0;
	auto Front = [this](const TCHAR* Name, double StreetYAt) -> UStaticMeshComponent* {
		UStaticMesh* Mesh = LoadProp(Name);
		if (!Mesh)
		{
			return nullptr;
		}
		UStaticMeshComponent* C = AddMesh(Mesh, nullptr, FromStreet(0.0, StreetYAt, 0.0), FVector(1.0), FRotator::ZeroRotator);
		Fronts.Add(C);
		return C;
	};
	// 1812: the player's window on the third floor (facades.py f1812: storey 3, bay Y +3.2, 1.3 x 1.85 m from 7.75 m up)
	// glows with the laptop in the bake. Nobody lives there now: dark glass in its sashes, in front of the lit room.
	if (Front(TEXT("SM_Facade_1812"), 0.0))
	{
		const FVector Pane = FromStreet(-24.2, 320.0, 775.0 + 92.5);
		Block(Surface(TEXT("DeeDarkGlass"), LitHex[0], 0.06f), Pane - FVector(0.7, 63.0, 91.5), Pane + FVector(0.7, 63.0, 91.5), nullptr, false);
		Block(Buff, FVector(Face + 365.0, StreetY - 800.0, -StreetZ), FVector(Back, StreetY + 800.0, 1500.0 - StreetZ));
	}
	else
	{
		// Buff brick over a limestone base, its windows as facades.py lays them out (lit where its rooms are), the
		// player's dark; the entrance's transom and lamps.
		Block(Buff, FVector(Face, StreetY - 800.0, -StreetZ), FVector(Back, StreetY + 800.0, 1520.0 - StreetZ));
		Block(Stone, FVector(Face - 2.0, StreetY - 800.0, -StreetZ), FVector(Face, StreetY + 800.0, 360.0 - StreetZ));
		Block(Stone, FVector(Face - 18.0, StreetY - 810.0, 1380.0 - StreetZ), FVector(Face, StreetY + 810.0, 1460.0 - StreetZ));
		static const int32 Plan[3][5] = {{0, 1, 4, 0, 2}, {3, 0, 4, 0, 0}, {0, 1, 4, 0, 1}};
		const double Bays[5] = {-6.4, -3.2, 0.0, 3.2, 6.4};
		for (int32 Storey = 0; Storey < 3; ++Storey)
		{
			const double Sill = 360.0 + Storey * 340.0 + 75.0;
			for (int32 B = 0; B < 5; ++B)
			{
				const FVector At = FromStreet(0.0, Bays[B] * 100.0, Sill + 92.5);
				FrontWindow(FVector(Face - 0.5, At.Y, At.Z), FVector2D(B == 2 ? 100.0 : 130.0, 185.0), Plan[Storey][B]);
				Block(Stone, FVector(Face - 8.0, At.Y - 70.0, At.Z - 101.0), FVector(Face, At.Y + 70.0, At.Z - 92.5), nullptr, false);
			}
		}
		UMaterialInterface* Lantern = Surface(TEXT("DeeLantern"), 0xffd29a, 0.5f, 0.0f, 0.0f, 6.0f);
		Block(Lantern, FVector(Face - 1.0, StreetY - 90.0, 280.0 - StreetZ), FVector(Face, StreetY + 90.0, 320.0 - StreetZ), nullptr, false);
		for (const double S : {-1.0, 1.0})
		{
			Block(Lantern, FVector(Face - 22.0, StreetY + S * 135.0 - 9.0, 215.0 - StreetZ), FVector(Face - 4.0, StreetY + S * 135.0 + 9.0, 245.0 - StreetZ), nullptr, false);
		}
	}
	// North (her left): the pawn shop's front between 1812 and the walk-up, its window lit low behind its bars; the
	// walk-up over the barber's next to it; past them the block up to Market.
	Block(Brick, FVector(Face, StreetY - 2300.0, -StreetZ), FVector(Back, StreetY - 800.0, 1100.0 - StreetZ));
	WindowGrid(Face, StreetY - 2300.0 + 150.0, StreetY - 800.0 - 150.0, 420.0 - StreetZ, 1100.0 - StreetZ, 260.0, 330.0, 0.3, 101 * 7919);
	Block(Surface(TEXT("DeePawnDisplay"), 0xffd9a0, 0.6f, 0.0f, 0.0f, 1.3f), FVector(Face - 1.0, StreetY - 2150.0, 60.0 - StreetZ), FVector(Face, StreetY - 950.0, 270.0 - StreetZ),
	      nullptr, false);
	const double WalkupY = (2300.0 + 3700.0 - 20.0) * 0.5;
	if (Front(TEXT("SM_Facade_Walkup"), WalkupY))
	{
		Block(Brick, FVector(Face + 365.0, StreetY - 3680.0, -StreetZ), FVector(Back, StreetY - 2300.0, 1745.0 - StreetZ));
	}
	else
	{
		Block(Brick, FVector(Face, StreetY - 3680.0, -StreetZ), FVector(Back, StreetY - 2300.0, 1745.0 - StreetZ));
		WindowGrid(Face, StreetY - 3680.0, StreetY - 2300.0, 420.0 - StreetZ, 1745.0 - StreetZ, 250.0, 330.0, 0.28, 6007);
	}
	Block(BrickDark, FVector(Face, -6600.0, -StreetZ), FVector(Back, StreetY - 3680.0, 1250.0 - StreetZ));
	WindowGrid(Face, -6600.0, StreetY - 3680.0, 380.0 - StreetZ, 1250.0 - StreetZ, 250.0, 320.0, 0.3, 7001);
	// South (her right): two brownstones next door (the street's BrownstoneRow), then taller brick to the set's end.
	const double BrownW = 675.0;
	bool bBrownstones = false;
	for (int32 K = 0; K < 2; ++K)
	{
		bBrownstones |= Front(TEXT("SM_Facade_Brownstone"), -800.0 - 2.0 * BrownW + BrownW * (K + 0.5)) != nullptr;
	}
	if (bBrownstones)
	{
		Block(BrickDark, FVector(Face + 365.0, StreetY + 800.0, -StreetZ), FVector(Back, StreetY + 800.0 + 2.0 * BrownW, 1190.0 - StreetZ));
	}
	else
	{
		Block(BrickDark, FVector(Face, StreetY + 800.0, -StreetZ), FVector(Back, StreetY + 800.0 + 2.0 * BrownW, 1190.0 - StreetZ));
		WindowGrid(Face, StreetY + 800.0, StreetY + 800.0 + 2.0 * BrownW, 300.0 - StreetZ, 1190.0 - StreetZ, 225.0, 320.0, 0.3, 4421);
	}
	Block(Brick, FVector(Face, StreetY + 800.0 + 2.0 * BrownW, -StreetZ), FVector(Back, 6600.0, 1600.0 - StreetZ));
	WindowGrid(Face, StreetY + 800.0 + 2.0 * BrownW, 6600.0, 420.0 - StreetZ, 1600.0 - StreetZ, 260.0, 330.0, 0.3, 909);
}

void ADeeRoomStage::BuildStreetlights()
{
	// The street's posts within sight (StreetStage.cpp BuildStreetlights), their arms out over the road: sodium, and the
	// odd newer LED. The one in front of 1812 shadows (it rakes the face across the street); the rest light without.
	UMaterialInterface* Pole = Surface(TEXT("DeePole"), 0x2c3033, 0.5f, 0.7f);
	LampHeads = Surface(TEXT("DeeSodium"), 0xffb066, 0.3f, 0.0f, 0.0f, 14.0f);
	const struct
	{
		double X, Y;
		float Arm;
	} Posts[] = {{380.0, 700.0, 0.0f}, {1660.0, -600.0, 180.0f}, {1660.0, 1800.0, 180.0f}, {380.0, -2000.0, 0.0f}, {380.0, 2700.0, 0.0f},
		{1660.0, -3200.0, 180.0f}, {1660.0, 4200.0, 180.0f}, {380.0, -4300.0, 0.0f}, {380.0, 4700.0, 0.0f}};
	int32 Index = 0;
	for (const auto& P : Posts)
	{
		const FVector At = FromStreet(P.X, P.Y, 0.0);
		const float Arm = P.Arm + 180.0f;
		const FVector Out = FRotator(0.0f, Arm, 0.0f).Vector();
		const FVector Head = At + Out * 150.0 + FVector(0.0, 0.0, 790.0);
		if (!Prop(TEXT("SM_Streetlight"), nullptr, At, Arm))
		{
			Post(Pole, At, 9.0, 820.0);
			const FVector Tip = At + Out * 180.0;
			Block(Pole, FVector(FMath::Min(At.X, Tip.X) - 5.0, FMath::Min(At.Y, Tip.Y) - 5.0, At.Z + 805.0), FVector(FMath::Max(At.X, Tip.X) + 5.0, FMath::Max(At.Y, Tip.Y) + 5.0, At.Z + 815.0),
			      nullptr, false);
			Block(LampHeads, Head + FVector(-22.0, -22.0, 5.0), Head + FVector(22.0, 22.0, 15.0), nullptr, false);
		}
		USpotLightComponent* L = NewPart<USpotLightComponent>();
		L->SetRelativeLocationAndRotation(Head, FRotator(-90.0f, 0.0f, 0.0f));
		L->SetIntensityUnits(ELightUnits::Candelas);
		L->SetIntensity(900.0f);
		L->SetAttenuationRadius(2400.0f);
		L->SetOuterConeAngle(62.0f);
		L->SetInnerConeAngle(26.0f);
		L->SetSourceRadius(12.0f);
		L->SetUseTemperature(true);
		L->SetTemperature(Index % 3 == 2 ? 4000.0f : 2300.0f);
		L->SetVolumetricScatteringIntensity(0.0f);
		L->SetCastShadows(Index == 0);
		Streetlights.Add(L);
		++Index;
	}
}

// ------------------------------------------------------------------ the hour

uint32 ADeeRoomStage::HourKey() const
{
	return HashCombine(Super::HourKey(), HashCombine(GetTypeHash(CVarLamp.GetValueOnGameThread()), GetTypeHash(CVarFrontGlow.GetValueOnGameThread())));
}

void ADeeRoomStage::ApplyHour()
{
	// The first window's day and night light, the room's fill, the sky and the light on 1812 are the stage's own.
	Super::ApplyHour();
	const float D = Dawn;
	const float N = NightLevel();
	if (LeftWash && DayWash)
	{
		LeftWash->SetIntensity(DayWash->Intensity);
		LeftWash->SetVisibility(DayWash->IsVisible());
	}
	if (LeftFill && CityFill)
	{
		LeftFill->SetIntensity(CityFill->Intensity);
		LeftFill->SetLightColor(CityFill->GetLightColor());
	}
	// Her lamp: on from dusk, left on low for the couch all night; off in the day.
	const bool bLamp = D < 0.6f;
	if (LampLight)
	{
		LampLight->SetIntensity(bLamp ? FMath::Max(0.0f, CVarLamp.GetValueOnGameThread()) : 0.0f);
		LampLight->SetVisibility(bLamp);
	}
	if (LampGlow)
	{
		LampGlow->SetScalarParameterValue(TEXT("Emissive"), bLamp ? 4.0f : 0.0f);
	}
	DimBakedGlow(LampShade, bLamp ? 1.0f : 0.0f);
	// Outside: the streetlights' photocells (each a little apart), the neon's pink and the laundromat's light at night,
	// the lit rooms across the street (dimmed for this room's exposure, out by day).
	for (int32 I = 0; I < Streetlights.Num(); ++I)
	{
		if (USpotLightComponent* L = Streetlights[I])
		{
			const bool bOn = D < 0.45f + 0.04f * static_cast<float>(I % 4);
			if (L->IsVisible() != bOn)
			{
				L->SetVisibility(bOn);
			}
		}
	}
	if (LampHeads)
	{
		LampHeads->SetScalarParameterValue(TEXT("Emissive"), 14.0f * N);
	}
	if (NeonPink)
	{
		NeonPink->SetIntensity(600.0f * N);
		NeonPink->SetVisibility(N > 0.01f);
	}
	if (WashSpill)
	{
		WashSpill->SetIntensity(1000.0f * N);
		WashSpill->SetVisibility(N > 0.01f);
	}
	const float Glow = FMath::Max(0.0f, CVarFrontGlow.GetValueOnGameThread());
	for (UStaticMeshComponent* F : Fronts)
	{
		DimBakedGlow(F, Glow * N);
	}
	for (int32 I = 0; I < LitWindows.Num() && I < LitLevels.Num(); ++I)
	{
		if (UMaterialInstanceDynamic* M = LitWindows[I])
		{
			M->SetScalarParameterValue(TEXT("Emissive"), LitLevels[I] * N);
		}
	}
}

// ------------------------------------------------------------------ play

void ADeeRoomStage::AttachSlate()
{
	Super::AttachSlate();
	NoteSlate.Reset();
	if (!FSlateTextMeasurer::IsAvailable() || !NoteWidget)
	{
		return;
	}
	NoteSlate = SNew(SDrawListWidget).DesiredSize(FVector2D(ss::ui::props::NoteSize, ss::ui::props::NoteSize));
	NoteWidget->SetSlateWidget(NoteSlate);
	NoteWidget->SetManuallyRedraw(true);
	DrawNote();
}

void ADeeRoomStage::SetNote(const std::vector<std::string>& Lines)
{
	// Drawn again only when it says something else (AttachSlate draws what it says once play begins).
	if (Lines == NoteLines)
	{
		return;
	}
	NoteLines = Lines;
	DrawNote();
}

void ADeeRoomStage::DrawNote()
{
	if (!NoteWidget)
	{
		return;
	}
	NoteWidget->SetVisibility(!NoteLines.empty());
	if (!NoteSlate || NoteLines.empty())
	{
		return;
	}
	namespace P = ss::ui::props;
	FSlateTextMeasurer Measurer;
	TSharedPtr<ss::ui::DrawList> List = MakeShared<ss::ui::DrawList>();
	ss::ui::Canvas C(*List, Measurer, P::NoteSize, P::NoteSize, 1.0f);
	P::StickyNote(C, NoteLines, ss::ui::Hex(0xf6e27a));
	NoteSlate->SetDrawList(List);
	NoteWidget->RequestRedraw();
}

void ADeeRoomStage::SetBoxes(bool bShown)
{
	if (BoxesGroup && BoxesGroup->IsVisible() != bShown)
	{
		BoxesGroup->SetVisibility(bShown, true);
	}
}

void ADeeRoomStage::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// The room going (the hub moved to 1812): she goes with it. The world closing takes her with everything else.
	if (EndPlayReason == EEndPlayReason::Destroyed)
	{
		RemoveDee();
	}
	Dee = nullptr;
	if (DeeLoad.IsValid())
	{
		DeeLoad->CancelHandle();
		DeeLoad.Reset();
	}
	Super::EndPlay(EndPlayReason);
}

void ADeeRoomStage::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Dt = FMath::Clamp(DeltaSeconds, 0.0f, 0.1f);
	UpdateDee(Dt);
	UpdateTv(Dt);
}

FVector ADeeRoomStage::EyeLocal() const
{
	return Eye;
}

FVector ADeeRoomStage::ChairLocation() const
{
	return GetActorTransform().TransformPosition(ArmchairAt + FVector(0.0, 0.0, 70.0));
}

FVector ADeeRoomStage::DoorLocation() const
{
	return GetActorTransform().TransformPosition(HallDoorAt + FVector(0.0, -10.0, 100.0));
}

void ADeeRoomStage::MenuShot(double T, FVector& OutLocation, FVector& OutLookAt) const
{
	// Standing behind the couch at its left, drifting slowly: the coffee table and the laptop's glow, the rain on the
	// window and 1812 across the street, her chair and the TV in the corner on the right.
	const FVector From(-212.0 + 6.0 * FMath::Sin(T * 0.045), -70.0 + 9.0 * FMath::Sin(T * 0.037), 158.0 + 1.5 * FMath::Sin(T * 0.21));
	const FVector At(90.0 + 6.0 * FMath::Sin(T * 0.03), 70.0, 108.0 + 3.0 * FMath::Sin(T * 0.05));
	OutLocation = GetActorTransform().TransformPosition(From);
	OutLookAt = GetActorTransform().TransformPosition(At);
}

bool ADeeRoomStage::SleepShot(double T, FVector& OutLocation, FVector& OutLookAt) const
{
	// On the player's pillow at the couch's left end, lying back: the window across the room, its light changing with
	// the hours, slow breaths.
	const FVector Head(-176.0, -66.0, 72.0 + 0.6 * FMath::Sin(T * 1.15));
	const FVector At(160.0, 30.0, 175.0 + 4.0 * FMath::Sin(T * 0.11));
	OutLocation = GetActorTransform().TransformPosition(Head);
	OutLookAt = GetActorTransform().TransformPosition(At);
	return true;
}

void ADeeRoomStage::AddCan()
{
	// The empties: on the coffee table's left, past the note, then on the floor by the couch's left end.
	static const double Spots[10][3] = {
		{-52.0, -42.0, 0.4}, {-59.0, -51.0, 1.9}, {-47.0, -30.0, 2.8}, {-150.0, -114.0, 0.9}, {-158.0, -122.0, 2.2},
		{-146.0, -127.0, 0.1}, {-162.0, -108.0, 1.4}, {-121.0, -96.0, 2.6}, {-129.0, -104.0, 0.7}, {-136.0, -118.0, 1.7},
	};
	const int32 N = Cans.Num();
	if (N >= 10)
	{
		return;
	}
	const FVector Base(Spots[N][0], Spots[N][1], N < 3 ? TableTop : 0.0);
	if (CanMesh)
	{
		Cans.Add(AddMesh(CanMesh, nullptr, Base, FVector(1.0), FRotator(0.0, FMath::RadiansToDegrees(Spots[N][2]), 0.0)));
		return;
	}
	UStaticMeshComponent* Body = Post(Surface(TEXT("Can"), 0x0f1a12, 0.28f, 0.75f), Base, 3.32, 12.35);
	Post(Surface(TEXT("CanStripe"), 0xb8ff2e, 0.35f, 0.3f), Base + FVector(0.0, 0.0, 4.5), 3.34, 3.0);
	Post(Surface(TEXT("CanLid"), 0xb9bcc2, 0.3f, 1.0f), Base + FVector(0.0, 0.0, 12.35), 2.72, 0.1);
	Cans.Add(Body);
}

// ------------------------------------------------------------------ Dee

void ADeeRoomStage::SetDee(DeeRoom::EWhere InWhere, bool bAtOnce)
{
	bAtOnceNext |= bAtOnce || !bWhereKnown;
	Where = InWhere;
	bWhereKnown = true;
}

bool ADeeRoomStage::IsDeeHere() const
{
	return Dee != nullptr;
}

bool ADeeRoomStage::ConsumeDeeArrived()
{
	const bool bWas = bArrived;
	bArrived = false;
	return bWas;
}

bool ADeeRoomStage::ConsumeDeeLeft()
{
	const bool bWas = bLeft;
	bLeft = false;
	return bWas;
}

void ADeeRoomStage::DeeSay(const FString& Line)
{
	if (Dee)
	{
		Dee->Say(Line);
		Dee->TakeLines(); // the caption is the game mode's: nothing here reads the table's subtitles
	}
}

bool ADeeRoomStage::ChairInView() const
{
	const UWorld* World = GetWorld();
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	const APlayerCameraManager* Cam = PC ? PC->PlayerCameraManager.Get() : nullptr;
	if (!Cam)
	{
		return false;
	}
	// In the frame, with a margin: the chair's anywhere near the edge of the view counts.
	const FVector To = (ChairLocation() - Cam->GetCameraLocation()).GetSafeNormal();
	const float Half = FMath::Min(89.0f, Cam->GetFOVAngle() * 0.5f + 14.0f);
	return FVector::DotProduct(To, Cam->GetCameraRotation().Vector()) > FMath::Cos(FMath::DegreesToRadians(Half));
}

void ADeeRoomStage::UpdateDee(float DeltaSeconds)
{
	const bool bWant = bWhereKnown && Where == DeeRoom::EWhere::Home;
	if (bWant != (Dee != nullptr))
	{
		// She comes and goes when the player isn't looking (she's never seen to pop in or out of the chair).
		if (bAtOnceNext || !ChairInView())
		{
			const bool bQuiet = bAtOnceNext;
			if (bWant)
			{
				SpawnDee();
				if (Dee)
				{
					bArrived = !bQuiet;
					bAtOnceNext = false;
				}
			}
			else
			{
				RemoveDee();
				bLeft = !bQuiet;
				bAtOnceNext = false;
			}
		}
	}
	else
	{
		bAtOnceNext = false;
	}
	if (!Dee)
	{
		return;
	}
	// The seated body's idle habits are a card table's: her hands go back to her lap before any of them comes up.
	HabitLeft -= DeltaSeconds;
	if (HabitLeft <= 0.0f)
	{
		HabitLeft = 40.0f;
		Dee->TestHabit(5, 0);
	}
	// She watches the TV and glances at the player now and then; stare at her long enough and she looks back.
	const UWorld* World = GetWorld();
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (const APlayerCameraManager* Cam = PC ? PC->PlayerCameraManager.Get() : nullptr)
	{
		const FVector At = Cam->GetCameraLocation();
		Dee->HeroEyes = At;
		const FVector ToEyes = Dee->GetEyes() - At;
		const bool bStudied = ToEyes.Size() < 600.0 && FVector::DotProduct(ToEyes.GetSafeNormal(), Cam->GetCameraRotation().Vector()) > 0.985;
		Dee->SetStudied(bStudied ? 1.0f : 0.0f);
	}
}

void ADeeRoomStage::SpawnDee()
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}
	// MHC_Dee, as the Back Room casts her (BackRoomGameMode.cpp CastBlueprint); the MetaHuman archetype until she's built.
	TSoftClassPtr<AActor> Blueprint;
	const FString Package = TEXT("/Game/ShortStack/Cast/Built/MHC_Dee/BP_MHC_Dee");
	if (FPackageName::DoesPackageExist(Package))
	{
		Blueprint = TSoftClassPtr<AActor>(FSoftObjectPath(Package + TEXT(".BP_MHC_Dee_C")));
	}
	if (!Blueprint.IsNull() && !Blueprint.Get() && UAssetManager::IsInitialized())
	{
		// Her body, clothes and hair are big: they load in the background, and she sits down once they're in.
		if (!DeeLoad.IsValid())
		{
			DeeLoad = UAssetManager::GetStreamableManager().RequestAsyncLoad(Blueprint.ToSoftObjectPath(), FStreamableDelegate(), FStreamableManager::AsyncLoadHighPriority);
		}
		if (DeeLoad.IsValid() && DeeLoad->IsLoadingInProgress())
		{
			return;
		}
	}
	DeeLoad.Reset();
	if (!Blueprint.IsNull() && !Blueprint.LoadSynchronous())
	{
		Blueprint.Reset(); // her Blueprint didn't load: the archetype body sits in for her
	}
	// On the recliner's front edge, as the Back Room seats its players on their chairs (the pelvis SeatDepth behind it),
	// sunk into the cushion to its height.
	const FVector Edge = ArmchairAt + FRotator(0.0f, ArmchairYaw, 0.0f).RotateVector(FVector(41.5, 0.0, 0.0)) +
						 FVector(0.0, 0.0, ArmchairSeat - BackRoomPlayerDetail::SeatHeight);
	const FTransform At = FTransform(FRotator(0.0f, ArmchairYaw, 0.0f), Edge) * GetActorTransform();
	ABackRoomPlayer* P = World->SpawnActorDeferred<ABackRoomPlayer>(ABackRoomPlayer::StaticClass(), At, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!P)
	{
		return;
	}
	P->Persona = ABackRoomGameMode::PersonaFor(TEXT("Dee"));
	// At home: sat back in her chair, nothing to say to a table.
	P->Persona.Posture = 0.0f;
	P->Persona.Chatter = 0.0f;
	P->SeatRole = EBackRoomRole::Player;
	P->MetaHumanClass = Blueprint;
	// No card table in her front room: the arms' table is put far under the floor, out of their way.
	P->TableToWorld = FTransform(FVector(0.0, 0.0, -100000.0));
	// Where her eyes go: the TV most of the time (the table's pot and dealer, and most of the room's other faces, are
	// its screen here), the window now and then, the player a little less often than a table's players look at the hero.
	const FVector Tv = GetActorTransform().TransformPosition(TvScreenAt);
	P->PotAt = Tv;
	P->DealerAt = Tv;
	P->OthersAt = {Tv + FVector(0.0, 0.0, 12.0), Tv - FVector(0.0, 0.0, 10.0), Tv + FVector(0.0, 14.0, 2.0), Tv - FVector(0.0, 14.0, 0.0), Tv + FVector(0.0, 6.0, -4.0),
		Tv - FVector(0.0, 6.0, -6.0), GetActorTransform().TransformPosition(FVector(FrontX, WinY[0], 160.0))};
	P->HeroEyes = EyeLocation();
	P->FinishSpawning(At);
	// She sits in the recliner: the folding chair a table seat brings stays out of it.
	TInlineComponentArray<UStaticMeshComponent*> Statics(P);
	for (UStaticMeshComponent* S : Statics)
	{
		if (S && (S->GetFName() == TEXT("ChairSeat") || S->GetFName() == TEXT("ChairBack")))
		{
			S->SetVisibility(false);
			S->SetHiddenInGame(true);
		}
	}
	P->TestHabit(5, 0);
	HabitLeft = 40.0f;
	Dee = P;
}

void ADeeRoomStage::RemoveDee()
{
	if (Dee)
	{
		Dee->Destroy();
		Dee = nullptr;
	}
}

void ADeeRoomStage::UpdateTv(float DeltaSeconds)
{
	// On while she's in her chair, fading up and down as a set does; the picture cuts from shot to shot, a little
	// flicker in it, and throws its colour on her and the walls.
	const float Want = Dee ? 1.0f : 0.0f;
	if (TvLevel == Want && Want == 0.0f)
	{
		return;
	}
	TvLevel = FMath::FInterpConstantTo(TvLevel, Want, DeltaSeconds, 2.5f);
	TvCut -= DeltaSeconds;
	if (TvCut <= 0.0f)
	{
		const int32 Shots = static_cast<int32>(UE_ARRAY_COUNT(TvShots));
		TvCut = FMath::FRandRange(1.2f, 6.5f);
		TvScene = (TvScene + 1 + FMath::RandRange(0, Shots - 2)) % Shots;
		TvGoal = TvShots[TvScene];
	}
	TvColor = FMath::Lerp(TvColor, TvGoal, 1.0f - FMath::Exp(-DeltaSeconds * 16.0f));
	TvShimmer = 0.88f + 0.08f * FMath::Sin(Time * 9.3f) * FMath::Sin(Time * 2.1f) + 0.04f * FMath::FRand();
	const float Level = TvLevel * TvShimmer;
	if (TvPicture)
	{
		const FLinearColor Face = FMath::Lerp(Srgb(0x101412), TvColor, FMath::Min(1.0f, TvLevel * 4.0f));
		TvPicture->SetVectorParameterValue(TEXT("BaseColor"), Face);
		TvPicture->SetVectorParameterValue(TEXT("Color"), Face);
		TvPicture->SetScalarParameterValue(TEXT("Emissive"), 2.2f * Level);
	}
	if (TvLight)
	{
		TvLight->SetLightColor(TvColor);
		TvLight->SetIntensity(FMath::Max(0.0f, CVarTv.GetValueOnGameThread()) * Level);
		TvLight->SetVisibility(Level > 0.001f);
	}
}
