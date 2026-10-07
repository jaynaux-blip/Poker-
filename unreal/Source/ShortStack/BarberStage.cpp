#include "BarberStage.h"

#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Shop.h"
#include "GameFramework/PlayerController.h"
#include "HeroGrooms.h"
#include "ShortStack/UI/PropArt.h"
#include "ShortStackCharacter.h"

// Fifth St. Barbers (spec 5.1), in stage cm (spec 0.3): the walk-up's north bay. Picks recorded here (spec header):
// the interior camera's boom 140 cm and 80 degrees (spec 3.8's "about"); the pole turns 0.35 times a second; the pendants
// 60 cd at 2700 K, the station's tubes 110 cd at 4000 K; the window's spill 30 cd.

namespace BarberStageDetail
{
// The room (spec 2.4) and the storefront (ns_barber's docstring).
constexpr double OpenY0 = 3112.0;
constexpr double OpenY1 = 3618.0;
constexpr double OpenTop = 335.0;
constexpr double RoomX0 = -900.0; // the back wall's face
constexpr double RoomX1 = -35.0;  // the storefront's inside
constexpr double RoomY0 = 3125.0;
constexpr double RoomY1 = 3610.0;
constexpr double Ceiling = 330.0;
// The door at the opening's south end, recessed 60 cm, hinged on its south jamb and swinging in.
constexpr double DoorX = -64.5;
constexpr double DoorY0 = 3132.0;
constexpr double DoorW = 93.0;
constexpr double DoorH = 228.0;
// The recess's north side (the glazed return between it and the display).
constexpr double ReturnY = 3225.0;
// The chairs (facing north, toward the mirrors) and their sockets (ns_barber: Seat 0.55 m, Head 19 cm behind it at 1.25 m).
const FVector Chair1(-260.0, 3460.0, 0.0);
const FVector Chair2(-460.0, 3460.0, 0.0);
constexpr double SeatTop = 55.0;
const FVector Head1(-260.0, 3441.0, 125.0);
const FVector Head2(-460.0, 3441.0, 125.0);
// The waiting chairs along the south wall, facing north (their seats 45 cm).
const double BenchX[4] = {-150.0, -205.0, -310.0, -365.0};
constexpr double BenchY = 3160.0;
// The pole on the north pier (its axis 30 cm out from the pier's face).
const FVector PoleAt(16.5, 3652.0, 0.0);
const FVector PoleAxis(46.5, 3652.0, 0.0);
// The bell on its spring over the door's latch side, inside.
const FVector BellAt(-72.0, 3205.0, 237.0);

const TCHAR* const Meshes[] = {
	TEXT("SM_NS_Barber_Front"), TEXT("SM_NS_Barber_Front_Glass"), TEXT("SM_NS_Barber_Front_GlowNeon"), TEXT("SM_NS_Barber_Door"), TEXT("SM_NS_DoorBell"),
	TEXT("SM_NS_Barber_Grille"), TEXT("SM_NS_Barber_Pole"), TEXT("SM_NS_Barber_Pole_GlowWarm"), TEXT("SM_NS_Barber_Shell"), TEXT("SM_NS_Barber_Chair"),
	TEXT("SM_NS_Barber_Station"), TEXT("SM_NS_Barber_Mirror"), TEXT("SM_NS_Barber_Waiting"), TEXT("SM_NS_Barber_Wall"), TEXT("SM_NS_Barber_Sink"),
	TEXT("SM_NS_Barber_Lights"), TEXT("SM_NS_Barber_Lights_GlowWarm"), TEXT("SM_NS_Barber_Lights_GlowCool"),
};

FString Utf8ToF(const std::string& S)
{
	return FString(UTF8_TO_TCHAR(S.c_str()));
}
} // namespace BarberStageDetail

using namespace BarberStageDetail;

SHORTSTACK_STREET_PRELOAD(ABarberStage)

ABarberStage::ABarberStage()
{
	Venue = TEXT("barber");
	VenueName = TEXT("Fifth St. Barbers");
}

void ABarberStage::AppendPreload(TArray<FString>& Paths)
{
	for (const TCHAR* Name : Meshes)
	{
		Paths.Add(FStreetPreload::MeshPath(Name));
	}
	AppendShopMaterials(Paths);
}

void ABarberStage::BuildInterior()
{
	PoleSpin = nullptr;
	Bell = nullptr;
	PriceBoard = INDEX_NONE;
	HoursDecal = INDEX_NONE;
	PricesShown.Reset();
	HoursShown.Reset();
	// The walk-up's block and its face's blocking box stood where the room is (today's street): the room, its walls and its
	// opening carved out of them, off the sidewalk.
	Carve(FBox(FVector(RoomX0 - 12.0, OpenY0, 1.0), FVector(6.0, OpenY1, OpenTop + 10.0)));

	const bool bShell = Site(TEXT("SM_NS_Barber_Shell")) != nullptr;
	const bool bFront = Site(TEXT("SM_NS_Barber_Front")) != nullptr;
	Site(TEXT("SM_NS_Barber_Front_Glass"));

	// --- the room: floor, walls, ceiling (solid whatever the art; drawn by the block-out where the shell isn't imported)
	UMaterialInterface* Checker = Mat(TEXT("BarberChecker"), 0x8c8a84, 0.3f, 4.0f);
	UMaterialInterface* Mosaic = Mat(TEXT("BarberMosaic"), 0xe8e4da, 0.35f, 4.0f);
	UMaterialInterface* Cream = Mat(TEXT("BarberCream"), 0xe6dcc4, 0.85f);
	UMaterialInterface* Mint = Mat(TEXT("BarberMint"), 0x9fd3c7, 0.15f, 4.0f);
	UMaterialInterface* Tin = Mat(TEXT("BarberTin"), 0xd9d2bf, 0.6f);
	Solid(Checker, FVector(RoomX0 - 5.0, OpenY0, -10.0), FVector(DoorX, OpenY1, 0.0), !bShell);
	Solid(Mosaic, FVector(DoorX, OpenY0, -10.0), FVector(0.0, OpenY1, 0.0), !bFront);
	Solid(Cream, FVector(RoomX0, RoomY1, 0.0), FVector(RoomX1, RoomY1 + 15.0, Ceiling), !bShell);
	Solid(Cream, FVector(RoomX0, RoomY0 - 15.0, 0.0), FVector(DoorX - 2.0, RoomY0, Ceiling), !bShell);
	Solid(Cream, FVector(RoomX0 - 15.0, RoomY0 - 15.0, 0.0), FVector(RoomX0, RoomY1 + 15.0, Ceiling), !bShell);
	if (!bShell)
	{
		// The mint subway tile to 1.2 m, a cove base, the pressed tin overhead.
		Block(Mint, FVector(RoomX0, RoomY1 - 2.0, 0.0), FVector(RoomX1, RoomY1, 120.0), false);
		Block(Mint, FVector(RoomX0, RoomY0, 0.0), FVector(DoorX - 2.0, RoomY0 + 2.0, 120.0), false);
		Block(Mint, FVector(RoomX0, RoomY0, 0.0), FVector(RoomX0 + 2.0, RoomY1, 120.0), false);
		Block(Tin, FVector(RoomX0, RoomY0, Ceiling), FVector(RoomX1, RoomY1, Ceiling + 8.0), false);
	}

	// --- the storefront: the display on the X 0 plane north of the recess, the recess's return and jamb, the transom
	UMaterialInterface* Sash = Mat(TEXT("BarberSash"), 0x23402f, 0.45f);
	UMaterialInterface* Tile = Mat(TEXT("BarberBulkhead"), 0x1e4a34, 0.2f, 4.0f);
	Solid(Tile, FVector(-30.0, ReturnY + 7.0, 0.0), FVector(0.0, OpenY1, 60.0), !bFront);
	Solid(GlassMat(), FVector(-2.0, ReturnY + 7.0, 60.0), FVector(0.0, OpenY1, 300.0), !bFront)->SetCastShadow(false);
	Solid(Sash, FVector(DoorX, ReturnY, 0.0), FVector(0.0, ReturnY + 7.0, OpenTop), !bFront);
	Solid(Sash, FVector(DoorX, OpenY0, 0.0), FVector(0.0, DoorY0 - 2.0, OpenTop), !bFront);
	Solid(Sash, FVector(DoorX - 4.0, OpenY0, DoorH), FVector(0.0, ReturnY + 7.0, OpenTop), !bFront);
	if (!bFront)
	{
		Block(Sash, FVector(-6.0, ReturnY + 7.0, 296.0), FVector(0.0, OpenY1, 304.0), false);
		Block(Sash, FVector(-6.0, ReturnY + 7.0, 330.0), FVector(0.0, OpenY1, OpenTop), false);
		// FIFTH ST. BARBERS · EST. 1995 in gold leaf on the transom (spec 4.9), read from the street.
		TArray<FString> Gilt = {TEXT("EST. 1995 · WALK-INS WELCOME")};
		Screen(FVector(1.0, (ReturnY + OpenY1) * 0.5, 316.0), 0.0f, FVector2D(300.0, 26.0), FIntPoint(900, 78), 0.0f,
			[Gilt](ss::ui::Canvas& C) { AShopStage::PaintCard(C, TEXT("FIFTH ST. BARBERS"), Gilt, 0x0f2a1d, 0xd9b45a); });
	}
	Storefront(FVector(0.0, (OpenY0 + OpenY1) * 0.5, 160.0), FVector(1.0, 0.0, 0.0));

	// --- the door (the bell on its spring over it) and the grille
	FShopDoorSpec D;
	D.Hinge = FVector(DoorX, DoorY0, 0.0);
	D.ShutYaw = 90.0f;
	D.InYaw = 180.0f;
	D.OpenYaw = 90.0f;
	D.Width = DoorW;
	D.Height = DoorH;
	D.Kind = EShopDoorKind::Swing;
	D.bGlass = false;
	Door(D, Mesh(TEXT("SM_NS_Barber_Door"), D.Hinge, 0.0f));
	// Just inside, clear of the waiting chairs along the south wall (the door's own start would stand among them).
	Start(TEXT("barber"), FVector(-190.0, 3310.0, 0.0), 180.0f);
	Bell = NewPart<USceneComponent>();
	Bell->SetRelativeLocation(BellAt);
	if (UStaticMeshComponent* B = Mesh(TEXT("SM_NS_DoorBell"), BellAt, 0.0f))
	{
		B->AttachToComponent(Bell, FAttachmentTransformRules::KeepWorldTransform);
	}
	else
	{
		UStaticMeshComponent* Cup = NewPart<UStaticMeshComponent>(Bell);
		Cup->SetStaticMesh(SphereMesh);
		Cup->SetMaterial(0, Mat(TEXT("Brass"), 0xb08d3a, 0.25f, 0.0f, 0.0f, 1.0f));
		Cup->SetRelativeLocation(FVector(0.0, 0.0, -11.0));
		Cup->SetRelativeScale3D(FVector(0.07));
		Cup->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	Grille(FVector(2.0, OpenY0, 0.0), FVector(6.0, OpenY1, 307.0), Mesh(TEXT("SM_NS_Barber_Grille"), FVector(9.0, (OpenY0 + OpenY1) * 0.5, 307.0), 0.0f));

	// --- the pole on the north pier, turning while it's open, and the neon OPEN in the south pane
	if (UStaticMeshComponent* Pole = Mesh(TEXT("SM_NS_Barber_Pole"), PoleAt, 0.0f))
	{
		Pole->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Glow(Mesh(TEXT("SM_NS_Barber_Pole_GlowWarm"), PoleAt, 0.0f), EShopLight::Sign, 0xfff0d6, 3.0f);
	}
	else
	{
		UMaterialInterface* Iron = Mat(TEXT("Iron"), 0x1c1e21, 0.45f, 0.0f, 0.0f, 0.7f);
		Block(Iron, FVector(PoleAt.X, PoleAt.Y - 3.0, 196.0), FVector(PoleAxis.X, PoleAt.Y + 3.0, 202.0), false);
		Block(Iron, FVector(PoleAt.X, PoleAt.Y - 3.0, 252.0), FVector(PoleAxis.X, PoleAt.Y + 3.0, 258.0), false);
		// The striped insert as slanted bands round a spinning core (its turn reads as the stripes climbing).
		PoleSpin = NewPart<USceneComponent>();
		PoleSpin->SetRelativeLocation(PoleAxis + FVector(0.0, 0.0, 219.0));
		static const uint32 Bands[3] = {0xc8202c, 0xf0ece4, 0x2048a8};
		for (int32 K = 0; K < 12; ++K)
		{
			UStaticMeshComponent* Band = NewPart<UStaticMeshComponent>(PoleSpin);
			Band->SetStaticMesh(CubeMesh);
			Band->SetMaterial(0, Mat(*FString::Printf(TEXT("Pole%d"), K % 3), Bands[K % 3], 0.3f, 0.0f, 0.5f));
			Band->SetRelativeLocationAndRotation(FVector(0.0, 0.0, -27.0 + K * 5.0), FRotator(0.0f, K * 30.0f, 22.0f));
			Band->SetRelativeScale3D(FVector(0.13, 0.13, 0.05));
			Band->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Band->SetCastShadow(false);
		}
		UMaterialInstanceDynamic* Globe = Mat(TEXT("PoleGlobe"), 0xfff4e0, 0.2f, 0.0f, 4.0f);
		Glow(Globe, EShopLight::Sign);
		UStaticMeshComponent* Top = NewPart<UStaticMeshComponent>();
		Top->SetStaticMesh(SphereMesh);
		Top->SetMaterial(0, Globe);
		Top->SetRelativeLocation(PoleAxis + FVector(0.0, 0.0, 278.0));
		Top->SetRelativeScale3D(FVector(0.16));
		Top->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	PointLight(PoleAxis + FVector(0.0, 0.0, 278.0), 6.0f, 0xfff0d6, 350.0f, EShopLight::Sign);
	if (UStaticMeshComponent* Neon = Site(TEXT("SM_NS_Barber_Front_GlowNeon")))
	{
		Glow(Neon, EShopLight::Sign, 0xff2a1c, 4.0f);
	}
	else
	{
		UMaterialInstanceDynamic* Tube = Mat(TEXT("NeonOpen"), 0xff2a1c, 0.3f, 0.0f, 6.0f);
		Glow(Tube, EShopLight::Sign);
		Block(Tube, FVector(-4.0, 3300.0, 146.0), FVector(-2.5, 3350.0, 149.0), false)->SetCastShadow(false);
		Block(Tube, FVector(-4.0, 3300.0, 160.0), FVector(-2.5, 3350.0, 163.0), false)->SetCastShadow(false);
	}
	PointLight(FVector(-18.0, 3325.0, 155.0), 5.0f, 0xff2a1c, 300.0f, EShopLight::Sign);

	// --- the two chairs facing the backbar's mirrors
	for (const FVector& At : {Chair1, Chair2})
	{
		Solid(Mat(TEXT("Chrome"), 0xdfe3e6, 0.08f, 0.0f, 0.0f, 1.0f), At + FVector(-28.0, -28.0, 0.0), At + FVector(28.0, 28.0, 50.0), false);
		if (!Mesh(TEXT("SM_NS_Barber_Chair"), At, 90.0f))
		{
			UMaterialInterface* Vinyl = Mat(TEXT("Oxblood"), 0x4f1618, 0.45f);
			UMaterialInterface* Porcelain = Mat(TEXT("Porcelain"), 0xf2efe8, 0.2f);
			UMaterialInterface* Chrome = Mat(TEXT("Chrome"), 0xdfe3e6, 0.08f, 0.0f, 0.0f, 1.0f);
			Block(Porcelain, At + FVector(-22.0, -22.0, 0.0), At + FVector(22.0, 22.0, 8.0), false);
			Block(Chrome, At + FVector(-7.0, -7.0, 8.0), At + FVector(7.0, 7.0, 42.0), false);
			Block(Vinyl, At + FVector(-27.0, -25.0, 42.0), At + FVector(27.0, 28.0, SeatTop), false);
			Block(Vinyl, At + FVector(-25.0, -33.0, SeatTop), At + FVector(25.0, -22.0, 112.0), false);
			Block(Vinyl, At + FVector(-12.0, -31.0, 114.0), At + FVector(12.0, -20.0, 132.0), false);
			Block(Chrome, At + FVector(-30.0, -22.0, 74.0), At + FVector(-26.0, 22.0, 79.0), false);
			Block(Chrome, At + FVector(26.0, -22.0, 74.0), At + FVector(30.0, 22.0, 79.0), false);
			Block(Chrome, At + FVector(-18.0, 40.0, 14.0), At + FVector(18.0, 52.0, 20.0), false);
		}
	}

	// --- the backbar on the north wall, its mirrors over it, the sink at the back
	const bool bStation = Site(TEXT("SM_NS_Barber_Station")) != nullptr;
	Solid(Mat(TEXT("Walnut"), 0x4a2c1d, 0.5f, 2.0f), FVector(-565.0, 3550.0, 0.0), FVector(-155.0, RoomY1, 92.0), !bStation);
	if (!bStation)
	{
		Block(Mat(TEXT("Marble"), 0xe6e2da, 0.15f), FVector(-570.0, 3545.0, 88.0), FVector(-150.0, RoomY1, 92.0), false);
		Block(Mat(TEXT("Marble"), 0xe6e2da, 0.15f), FVector(-565.0, 3590.0, 108.0), FVector(-155.0, RoomY1, 111.0), false);
		Block(Mat(TEXT("Disinfectant"), 0x2a6bd6, 0.05f, 0.0f, 0.3f), FVector(-345.0, 3592.0, 111.0), FVector(-333.0, 3604.0, 131.0), false);
	}
	const bool bMirror = Site(TEXT("SM_NS_Barber_Mirror")) != nullptr;
	Mirror(FVector(-360.0, 3607.5, 172.0), FVector(0.0, -1.0, 0.0), FVector2D(390.0, 120.0), !bMirror);
	const bool bSink = Site(TEXT("SM_NS_Barber_Sink")) != nullptr;
	Solid(Mat(TEXT("Walnut"), 0x4a2c1d, 0.5f, 2.0f), FVector(-850.0, 3555.0, 0.0), FVector(-730.0, RoomY1, 85.0), !bSink);

	// --- the waiting chairs on the south wall, the side table, and the wall: the TV, the price board, the clock
	const bool bWaiting = Site(TEXT("SM_NS_Barber_Waiting")) != nullptr;
	Solid(Mat(TEXT("Chrome"), 0xdfe3e6, 0.08f, 0.0f, 0.0f, 1.0f), FVector(-390.0, RoomY0, 0.0), FVector(-125.0, BenchY + 22.0, 42.0), false);
	if (!bWaiting)
	{
		UMaterialInterface* Vinyl = Mat(TEXT("Oxblood"), 0x4f1618, 0.45f);
		for (const double X : BenchX)
		{
			Block(Vinyl, FVector(X - 22.0, BenchY - 22.0, 38.0), FVector(X + 22.0, BenchY + 22.0, 45.0), false);
			Block(Vinyl, FVector(X - 20.0, RoomY0 + 2.0, 45.0), FVector(X + 20.0, RoomY0 + 8.0, 88.0), false);
		}
		Block(Mat(TEXT("Walnut"), 0x4a2c1d, 0.5f, 2.0f), FVector(-285.0, RoomY0 + 4.0, 0.0), FVector(-230.0, BenchY + 18.0, 55.0), false);
	}
	const bool bWall = Site(TEXT("SM_NS_Barber_Wall")) != nullptr;
	if (!bWall)
	{
		// The clock's dial on the back wall, and the TV's set on its bracket.
		Block(Mat(TEXT("ClockDial"), 0xf4f1e8, 0.4f), FVector(RoomX0, 3351.0, 245.0), FVector(RoomX0 + 3.0, 3385.0, 279.0), false);
		Block(Mat(TEXT("TvBody"), 0x111214, 0.5f), FVector(RoomX0, 3462.0, 213.0), FVector(RoomX0 + 9.0, 3552.0, 267.0), false);
	}
	ClockHands(FVector(-892.9, 3368.0, 262.0), 0.0f, 17.0f);
	Screen(FVector(-873.0, 3507.0, 240.0), -18.0f, FVector2D(80.0, 45.0), FIntPoint(480, 270), 1.3f, [](ss::ui::Canvas& C) {
		// The set's picture: a ball game, the score in its corner.
		const float W = C.Width();
		const float H = C.Height();
		C.FillRect({0.0f, 0.0f, W, H}, ss::ui::Paint::Linear({0.0f, 0.0f}, {0.0f, H}, ss::ui::Hex(0x3a6b8f), ss::ui::Hex(0x2f6b2c)));
		C.FillEllipse(W * 0.5f, H * 0.82f, W * 0.42f, H * 0.18f, ss::ui::Paint(ss::ui::Hex(0x8a6a44)));
		C.FillRoundRect({W * 0.05f, H * 0.06f, W * 0.34f, H * 0.16f}, 4.0f, ss::ui::Paint(ss::ui::Hex(0x101418, 0.85f)));
		ss::ui::TextStyle T;
		T.Size = H * 0.1f;
		T.Weight = 800;
		T.Col = ss::ui::Hex(0xf2f2f2);
		T.VAlign = ss::ui::Baseline::Middle;
		C.Text("BOT 7  2-1", W * 0.08f, H * 0.14f, T);
	});

	// --- the lights (spec 7.5.5: 2 pendants, 2 tube lights, a TV, the pole's globe, the window neon), and the window's spill
	const bool bLights = Site(TEXT("SM_NS_Barber_Lights")) != nullptr;
	if (bLights)
	{
		Glow(Site(TEXT("SM_NS_Barber_Lights_GlowWarm")), EShopLight::Fixture, 0xffd9a8, 1.0f);
		Glow(Site(TEXT("SM_NS_Barber_Lights_GlowCool")), EShopLight::Fixture, 0xeef4ff, 1.0f);
	}
	else
	{
		UMaterialInstanceDynamic* Milk = Mat(TEXT("MilkGlass"), 0xfff1dc, 0.3f, 0.0f, 3.0f);
		UMaterialInstanceDynamic* Lens = Mat(TEXT("TubeLens"), 0xeef4ff, 0.3f, 0.0f, 3.0f);
		Glow(Milk, EShopLight::Fixture);
		Glow(Lens, EShopLight::Fixture);
		for (const FVector& At : {FVector(-260.0, 3440.0, 246.0), FVector(-460.0, 3440.0, 246.0)})
		{
			UStaticMeshComponent* Globe = NewPart<UStaticMeshComponent>();
			Globe->SetStaticMesh(SphereMesh);
			Globe->SetMaterial(0, Milk);
			Globe->SetRelativeLocation(At);
			Globe->SetRelativeScale3D(FVector(0.28));
			Globe->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Globe->SetCastShadow(false);
			Block(Mat(TEXT("Iron"), 0x1c1e21, 0.45f, 0.0f, 0.0f, 0.7f), At + FVector(-0.4, -0.4, 14.0), At + FVector(0.4, 0.4, Ceiling - At.Z), false)->SetCastShadow(false);
		}
		for (const double X0 : {-321.0, -521.0, -852.0})
		{
			Block(Lens, FVector(X0, 3550.0, Ceiling - 8.0), FVector(X0 + 122.0, 3560.0, Ceiling - 2.0), false)->SetCastShadow(false);
		}
	}
	PointLight(FVector(-260.0, 3440.0, 240.0), 60.0f, 0xffc58f, 650.0f, EShopLight::Fixture);
	PointLight(FVector(-460.0, 3440.0, 240.0), 60.0f, 0xffc58f, 650.0f, EShopLight::Fixture);
	RectLight(FVector(-260.0, 3555.0, Ceiling - 6.0), FRotator(-90.0f, 0.0f, 0.0f), 122.0f, 10.0f, 110.0f, 0xffe9d0, 700.0f, EShopLight::Fixture);
	RectLight(FVector(-460.0, 3555.0, Ceiling - 6.0), FRotator(-90.0f, 0.0f, 0.0f), 122.0f, 10.0f, 110.0f, 0xffe9d0, 700.0f, EShopLight::Fixture);
	RectLight(FVector(-791.0, 3555.0, Ceiling - 6.0), FRotator(-90.0f, 0.0f, 0.0f), 122.0f, 10.0f, 70.0f, 0xffe9d0, 550.0f, EShopLight::Fixture);
	PointLight(FVector(-850.0, 3500.0, 238.0), 3.0f, 0x9fc4ff, 300.0f, EShopLight::Fixture);
	// The window's light on the sidewalk, aimed down and out (spec 3.1).
	RectLight(FVector(25.0, 3420.0, 300.0), FRotator(-35.0f, 0.0f, 0.0f), 390.0f, 200.0f, 30.0f, 0xffe0b0, 900.0f, EShopLight::Spill);
	// After hours a light left on over the station.
	PointLight(FVector(-360.0, 3560.0, 300.0), 4.0f, 0xffd9a8, 450.0f, EShopLight::Night);

	// --- the uses, seats, stations, shots and the room
	Use(TEXT("barber-chair1"), EStreetVerb::Sit, Chair1 + FVector(0.0, 0.0, 100.0), 110.0f, TEXT("Get a cut"), TEXT("barber-chair1"));
	Use(TEXT("barber-bench"), EStreetVerb::Wait, FVector(-257.0, BenchY + 15.0, 90.0), 150.0f, TEXT("Wait for the chair"), TEXT("barber-bench2"));
	// The hero's seats (the chairs' station ids are the barber's own places beside them, spec 2.10).
	Seat(TEXT("barber-chair1"), Chair1, 90.0f, SeatTop, Chair1 + FVector(0.0, -120.0, 0.0), TEXT("Sit"), false);
	Seat(TEXT("barber-chair2"), Chair2, 90.0f, SeatTop, Chair2 + FVector(0.0, -120.0, 0.0), TEXT("Sit"), false);
	for (int32 K = 0; K < 4; ++K)
	{
		Seat(FName(*FString::Printf(TEXT("barber-bench%d"), K + 1)), FVector(BenchX[K], BenchY, 0.0), 90.0f, 45.0f, FVector(BenchX[K], BenchY + 110.0, 0.0));
	}
	// The barber's working ring at each chair's head (beside it, a step south of it), and the door at closing.
	Station(TEXT("barber-chair1"), FVector(Head1.X + 55.0, Head1.Y - 10.0, 0.0), 170.0f, TEXT("Cut"));
	Station(TEXT("barber-chair2"), FVector(Head2.X + 55.0, Head2.Y - 10.0, 0.0), 170.0f, TEXT("Cut"));
	Station(TEXT("barber-door"), FVector(30.0, 3170.0, 0.0), 0.0f, TEXT("Smoke"));
	// The cameras: from the mirror toward the seated hero; three for the cut; the hand mirror behind the head.
	Shot(TEXT("barber-mirror"), FVector(Chair1.X, 3600.0, 150.0), Head1, 40.0f);
	Shot(TEXT("barber-cut1"), FVector(-150.0, 3525.0, 172.0), Head1, 45.0f);
	Shot(TEXT("barber-cut2"), FVector(-385.0, 3375.0, 118.0), Head1 + FVector(0.0, 0.0, -4.0), 50.0f);
	Shot(TEXT("barber-cut3"), FVector(-196.0, 3448.0, 138.0), Head1 + FVector(0.0, 0.0, 2.0), 55.0f);
	Shot(TEXT("barber-reveal"), FVector(-262.0, 3318.0, 148.0), Head1 + FVector(0.0, 0.0, 4.0), 40.0f);
	Zone(FBox(FVector(RoomX0, RoomY0, -20.0), FVector(DoorX - 2.0, RoomY1, Ceiling)), TEXT("FIFTH ST. BARBERS"), StreetRoomTone::Barber, 140.0f, 80.0f);
}

TArray<TPair<FName, float>> ABarberStage::Cinematic(FName Service) const
{
	// The cut from three angles under the skip (Lonnie's comb, scissors and clippers at the head mark), then the reveal from
	// the hand mirror once the new look is on (spec 5.1).
	return {{TEXT("barber-cut1"), 3.0f}, {TEXT("barber-cut2"), 3.0f}, {TEXT("barber-cut3"), 2.0f}, {TEXT("barber-reveal"), -2.5f}};
}

void ABarberStage::OnShot(FName InShot, bool bActive)
{
	const FString Id = InShot.ToString();
	if (Id == TEXT("barber-mirror") || Id.StartsWith(TEXT("barber-cut")) || Id == TEXT("barber-reveal"))
	{
		SetMirrorLive(bActive);
	}
}

void ABarberStage::OnHour()
{
	for (UMaterialInstanceDynamic* Mid : PoleMaterials())
	{
		if (Mid)
		{
			Mid->SetScalarParameterValue(TEXT("Lit"), IsOpen() ? 1.0f : 0.0f);
		}
	}
}

void ABarberStage::HoldGrooms(bool bHold)
{
	if (bHold == bGroomsHeld)
	{
		return;
	}
	const UWorld* InWorld = GetWorld();
	const APlayerController* Pc = InWorld ? InWorld->GetFirstPlayerController() : nullptr;
	AShortStackCharacter* Hero = Pc ? Cast<AShortStackCharacter>(Pc->GetPawn()) : nullptr;
	if (!Hero)
	{
		return; // asked again as it animates or syncs, once the hero stands
	}
	bGroomsHeld = bHold;
	Hero->PreloadGroomSet(bHold);
}

void ABarberStage::OnAwake()
{
	HoldGrooms(IsAwake());
}

void ABarberStage::EndPlay(const EEndPlayReason::Type Reason)
{
	// The street going (home, Dee's game): the set let go with it (the hero may already be gone, so straight to the library).
	if (bGroomsHeld)
	{
		bGroomsHeld = false;
		HeroGrooms::ReleasePreload();
	}
	Super::EndPlay(Reason);
}

void ABarberStage::Animate(float Dt)
{
	Super::Animate(Dt);
	// Awake from the start (the hero began beside it), or the hero came after: the set held.
	if (!bGroomsHeld)
	{
		HoldGrooms(true);
	}
	// The pole turns while it's open (its stripes climb); shut, it stops where it is.
	if (IsOpen())
	{
		PolePhase = FMath::Fmod(PolePhase + Dt * 0.35f, 1000.0f);
		for (UMaterialInstanceDynamic* Mid : PoleMaterials())
		{
			if (Mid)
			{
				Mid->SetScalarParameterValue(TEXT("Phase"), PolePhase);
			}
		}
		if (PoleSpin)
		{
			PoleSpin->SetRelativeRotation(FRotator(0.0f, PolePhase * 360.0f, 0.0f));
		}
	}
	// The bell swings on its spring as the door's top rail strikes it, and settles.
	if (bDoorJustOpened)
	{
		BellAge = 0.0f;
	}
	BellAge += Dt;
	if (Bell && BellAge < 4.0f)
	{
		const float Swing = 24.0f * FMath::Exp(-2.6f * BellAge) * FMath::Sin(BellAge * 15.0f);
		Bell->SetRelativeRotation(FRotator(Swing, 0.0f, 0.0f));
	}
}

void ABarberStage::SyncSession(const ss::Session& S)
{
	HoldGrooms(IsAwake());
	// The price board from the barber's offers (core's prices: the board never promises what the register won't ring up), one
	// line for color whatever the shade.
	TArray<TPair<FString, FString>> Rows;
	bool bColor = false;
	for (const ss::shop::Offer& O : ss::shop::Offers("barber"))
	{
		if (O.Type == ss::shop::OfferKind::Color)
		{
			if (bColor)
			{
				continue;
			}
			bColor = true;
			Rows.Emplace(TEXT("Color"), Dollars(O.PriceCents));
			continue;
		}
		Rows.Emplace(Utf8ToF(O.Name), Dollars(O.PriceCents));
		if (Rows.Num() >= 18)
		{
			break;
		}
	}
	FString Key;
	for (const TPair<FString, FString>& R : Rows)
	{
		Key += R.Key + R.Value;
	}
	if (Rows.Num() > 0 && Key != PricesShown)
	{
		PricesShown = Key;
		auto Paint = [Rows](ss::ui::Canvas& C) { AShopStage::PaintBoard(C, TEXT("PRICES"), Rows, 0x1a1a1a, 0xf2efe6); };
		if (PriceBoard == INDEX_NONE)
		{
			PriceBoard = ScreenIndex(Screen(FVector(-257.5, RoomY0 + 1.5, 180.0), 90.0f, FVector2D(45.0, 60.0), FIntPoint(450, 600), 0.0f, Paint));
		}
		else
		{
			Redraw(PriceBoard, Paint);
		}
	}
	// The hours decal on the display's glass by the door (core's sign, spec 4.9), read from the street.
	if (const ss::shop::Venue* V = ss::shop::FindVenue("barber"))
	{
		const FString Sign = Utf8ToF(ss::shop::HoursSign(*V));
		if (!Sign.IsEmpty() && Sign != HoursShown)
		{
			HoursShown = Sign;
			TArray<FString> Lines;
			Sign.ParseIntoArray(Lines, TEXT(" · "));
			auto Paint = [Lines](ss::ui::Canvas& C) { AShopStage::PaintCard(C, TEXT("HOURS"), Lines, 0x000000, 0xf3ead2, true); };
			if (HoursDecal == INDEX_NONE)
			{
				HoursDecal = ScreenIndex(Screen(FVector(1.2, ReturnY + 45.0, 150.0), 0.0f, FVector2D(30.0, 22.0), FIntPoint(300, 220), 1.1f, Paint));
			}
			else
			{
				Redraw(HoursDecal, Paint);
			}
		}
	}
}
