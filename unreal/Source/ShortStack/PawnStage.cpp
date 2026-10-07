#include "PawnStage.h"

#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ShortStack/Game/Gear.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Shop.h"
#include "ShortStack/UI/ShopScreen.h"

// Northside Pawn & Loan (spec 5.8), in stage cm (spec 0.3), from ns_pawn's plan, marks and lights. Picks recorded here:
// the six strips 140 cd at 4000 K, the cases' LED strips 40 cd at 5000 K, the desk lamp 30 cd, the blue tube's light 10 cd
// all night, the gold balls' spot 20 cd; the ceiling fan at 2 turns a second; the camera's boom 140 cm at 80 degrees. The
// apartment's gear meshes stand on the shelves facing the room as they face the desk; a wide piece takes a pair of sockets.

namespace PawnStageDetail
{
constexpr double OpenX0 = -3760.0;
constexpr double OpenX1 = -4440.0;
constexpr double OpenY = 4950.0;
constexpr double OpenTop = 380.0;
constexpr double RoomX0 = -3740.0; // the west wall's face
constexpr double RoomX1 = -4460.0; // the east wall's face
constexpr double RoomY0 = 3600.0;  // the back wall's face
constexpr double RoomY1 = 4920.0;
constexpr double Ceiling = 360.0;
// The steel-framed glass door at the opening's west end, recessed, hinged on the west jamb, swinging in; on a buzzer.
const FVector DoorHinge(-3792.5, 4854.5, 0.0);
constexpr double LeafW = 94.0;
constexpr double LeafH = 217.0;
// The counter's line, the cage's, the partition's.
constexpr double CounterY0 = 4300.0;
constexpr double CounterY1 = 4380.0;
constexpr double CageX1 = -4100.0;
constexpr double PartitionY = 4050.0;
// The shelves' gear sockets (facing the room, west) and the curio case's (facing north, the customer).
const FVector GearSockets[12] = {
	FVector(-4428.0, 4677.2, 12.0), FVector(-4428.0, 4627.8, 12.0), FVector(-4428.0, 4677.2, 66.0), FVector(-4428.0, 4627.8, 66.0),
	FVector(-4428.0, 4677.2, 120.0), FVector(-4428.0, 4627.8, 120.0), FVector(-4428.0, 4572.2, 12.0), FVector(-4428.0, 4522.8, 12.0),
	FVector(-4428.0, 4572.2, 66.0), FVector(-4428.0, 4522.8, 66.0), FVector(-4428.0, 4572.2, 120.0), FVector(-4428.0, 4522.8, 120.0),
};
const FVector CurioSockets[6] = {
	FVector(-4062.0, 4357.0, 36.5), FVector(-4093.0, 4357.0, 36.5), FVector(-4124.0, 4357.0, 36.5),
	FVector(-4062.0, 4334.0, 36.5), FVector(-4093.0, 4334.0, 36.5), FVector(-4124.0, 4334.0, 36.5),
};
const FVector Tray(-4228.0, 4352.0, 99.1);

const TCHAR* const Meshes[] = {
	TEXT("SM_NS_Pawn_Front"), TEXT("SM_NS_Pawn_Front_Glass"), TEXT("SM_NS_Pawn_Door"), TEXT("SM_NS_Pawn_Grille"), TEXT("SM_NS_Pawn_Sign"),
	TEXT("SM_NS_Pawn_Sign_GlowNeon"), TEXT("SM_NS_Pawn_Sign_GlowWarm"), TEXT("SM_NS_Pawn_Shell"), TEXT("SM_NS_Pawn_Shell_GlowCool"), TEXT("SM_NS_Pawn_FanBlades"),
	TEXT("SM_NS_Pawn_Counter"), TEXT("SM_NS_Pawn_Counter_GlowCool"), TEXT("SM_NS_Pawn_Cage"), TEXT("SM_NS_Pawn_Cage_GlowWarm"), TEXT("SM_NS_Pawn_SafeDoor"),
	TEXT("SM_NS_Pawn_GuitarWall"), TEXT("SM_NS_Pawn_Shelves"), TEXT("SM_NS_Pawn_Tickets"), TEXT("SM_NS_Pawn_Loupe"), TEXT("SM_NS_Pawn_Bench"),
	TEXT("SM_NS_Pawn_Bench_GlowCool"), TEXT("SM_NS_Pawn_Floor"),
	// The stock: the apartment's gear (spec 4.8), the wearable curios, the cup.
	TEXT("SM_Monitor"), TEXT("SM_MonitorWide"), TEXT("SM_Tower"), TEXT("SM_MacroPad"), TEXT("SM_Headphones"), TEXT("SM_Webcam"), TEXT("SM_WebcamPro"),
	TEXT("SM_Mirrorless"), TEXT("SM_MicUsb"), TEXT("SM_MicArm"), TEXT("SM_RingLight"), TEXT("SM_KeyLight_L"), TEXT("SM_KeyLight_R"),
	TEXT("SM_Wear_Aviators"), TEXT("SM_Wear_Trilby"), TEXT("SM_Trophy"),
};

FString Utf8ToF(const std::string& S)
{
	return FString(UTF8_TO_TCHAR(S.c_str()));
}

/** A curio's or a memento's mesh in the case (null: a tag on a velvet pad). */
const TCHAR* CaseMesh(const FString& LotId)
{
	if (LotId == TEXT("curio:aviators"))
	{
		return TEXT("SM_Wear_Aviators");
	}
	if (LotId == TEXT("curio:trilby"))
	{
		return TEXT("SM_Wear_Trilby");
	}
	if (LotId == TEXT("memento:cup"))
	{
		return TEXT("SM_Trophy");
	}
	return nullptr;
}
} // namespace PawnStageDetail

using namespace PawnStageDetail;

SHORTSTACK_STREET_PRELOAD(APawnStage)

APawnStage::APawnStage()
{
	Venue = TEXT("pawn");
	VenueName = TEXT("Northside Pawn & Loan");
}

void APawnStage::AppendPreload(TArray<FString>& Paths)
{
	for (const TCHAR* Name : Meshes)
	{
		Paths.Add(FStreetPreload::MeshPath(Name));
	}
	AppendShopMaterials(Paths);
}

const TCHAR* APawnStage::GearMesh(const FString& GearId, int32& OutSockets)
{
	struct FMap
	{
		const TCHAR* Id;
		const TCHAR* Mesh;
		int32 Sockets;
	};
	// The chair, the green screen, the espresso machine and what he won't take have no place on his shelves.
	static const FMap Maps[] = {
		{TEXT("monitor-24"), TEXT("SM_Monitor"), 1}, {TEXT("monitor-27"), TEXT("SM_MonitorWide"), 2}, {TEXT("tower-mid"), TEXT("SM_Tower"), 1},
		{TEXT("tower-dual"), TEXT("SM_Tower"), 2}, {TEXT("macro-pad"), TEXT("SM_MacroPad"), 1}, {TEXT("headphones"), TEXT("SM_Headphones"), 1},
		{TEXT("webcam-720"), TEXT("SM_Webcam"), 1}, {TEXT("webcam-1080"), TEXT("SM_WebcamPro"), 1}, {TEXT("mirrorless"), TEXT("SM_Mirrorless"), 1},
		{TEXT("mic-usb"), TEXT("SM_MicUsb"), 1}, {TEXT("mic-xlr"), TEXT("SM_MicArm"), 1}, {TEXT("ring-light"), TEXT("SM_RingLight"), 1},
		{TEXT("key-lights"), TEXT("SM_KeyLight_L"), 2},
	};
	for (const FMap& M : Maps)
	{
		if (GearId == M.Id)
		{
			OutSockets = M.Sockets;
			return M.Mesh;
		}
	}
	OutSockets = 0;
	return nullptr;
}

void APawnStage::FillCaps(ss::ui::HostCaps& Caps) const
{
	// A bit for each gear::Catalog item a mesh of the apartment's stands up on these shelves (spec 7.4). Worked out once a
	// session: the meshes don't come and go in play, and a missing one would go back to the disk every time it's asked.
	static uint64 Kinds = 0;
	static bool bKnown = false;
	if (!bKnown)
	{
		bKnown = true;
		const std::vector<ss::gear::Item>& Catalog = ss::gear::Catalog();
		for (size_t I = 0; I < Catalog.size() && I < 64; ++I)
		{
			int32 Sockets = 0;
			const TCHAR* Name = GearMesh(Utf8ToF(Catalog[I].Id), Sockets);
			if (Name && Sockets > 0 && ShopMesh(Name))
			{
				Kinds |= 1ull << I;
			}
		}
	}
	Caps.GearKinds |= Kinds;
}

void APawnStage::BuildInterior()
{
	Stock.Reset();
	StockKey.Reset();
	StockLines.Reset();
	HoursDecal = INDEX_NONE;
	HoursShown.Reset();
	Carve(FBox(FVector(RoomX1 - 15.0, RoomY0 - 15.0, 1.0), FVector(RoomX0 + 15.0, OpenY + 2.0, OpenTop + 2.0)));

	const bool bShell = Site(TEXT("SM_NS_Pawn_Shell")) != nullptr;
	const bool bFront = Site(TEXT("SM_NS_Pawn_Front")) != nullptr;
	Site(TEXT("SM_NS_Pawn_Front_Glass"));
	UMaterialInterface* Vinyl = Mat(TEXT("PawnVinyl"), 0x8c8676, 0.55f, 4.0f);
	UMaterialInterface* Painted = Mat(TEXT("PawnWall"), 0xd8cfb8, 0.8f, 1.0f);
	UMaterialInterface* Tin = Mat(TEXT("PawnTin"), 0xe6dcc2, 0.6f);
	UMaterialInterface* Black = Mat(TEXT("PawnBlack"), 0x1a1b1d, 0.3f, 0.0f, 0.0f, 0.8f);
	UMaterialInterface* Diamond = Mat(TEXT("Diamond"), 0x8c9096, 0.35f, 0.0f, 0.0f, 1.0f);
	UMaterialInterface* Steel = Mat(TEXT("PawnSteel"), 0x5a5e63, 0.5f, 0.0f, 0.0f, 0.7f);
	UMaterialInterface* Iron = Mat(TEXT("Iron"), 0x1c1e21, 0.45f, 0.0f, 0.0f, 0.7f);

	// --- the room
	Solid(Vinyl, FVector(RoomX1 - 15.0, RoomY0 - 15.0, -10.0), FVector(RoomX0 + 15.0, OpenY, 0.0), !bShell);
	Solid(Painted, FVector(RoomX0, RoomY0, 0.0), FVector(RoomX0 + 15.0, RoomY1, Ceiling), !bShell);
	Solid(Painted, FVector(RoomX1 - 15.0, RoomY0, 0.0), FVector(RoomX1, RoomY1, Ceiling), !bShell);
	Solid(Painted, FVector(RoomX1 - 15.0, RoomY0 - 15.0, 0.0), FVector(RoomX0 + 15.0, RoomY0, Ceiling), !bShell);
	if (!bShell)
	{
		Block(Tin, FVector(RoomX1, RoomY0, Ceiling), FVector(RoomX0, RoomY1, Ceiling + 6.0), false);
		// The signs on the walls (spec 4.9).
		TArray<FString> Rules = {TEXT("NO ID NO LOAN"), TEXT("30-DAY LOANS"), TEXT("TICKET REQUIRED TO REDEEM")};
		Screen(FVector(-4250.0, PartitionY + 12.0, 300.0), 90.0f, FVector2D(150.0, 60.0), FIntPoint(600, 240), 0.0f,
			[Rules](ss::ui::Canvas& C) { AShopStage::PaintCard(C, TEXT("ALL SALES FINAL"), Rules, 0xf4f1e8, 0x9c1420); });
	}

	// --- the storefront: the west jamb, the recessed door's vestibule, the barred display window east of it
	Solid(Black, FVector(DoorHinge.X, DoorHinge.Y, 0.0), FVector(RoomX0 + 15.0, OpenY, OpenTop), !bFront);
	Solid(Black, FVector(DoorHinge.X - LeafW - 14.0, DoorHinge.Y, 0.0), FVector(DoorHinge.X - LeafW, OpenY, OpenTop), !bFront);
	Solid(Black, FVector(DoorHinge.X - LeafW - 2.0, DoorHinge.Y - 4.0, LeafH), FVector(DoorHinge.X + 2.0, OpenY, OpenTop), !bFront);
	Solid(Mat(TEXT("PawnDisplay"), 0x3a1e22, 0.85f), FVector(RoomX1 - 15.0, 4830.0, 0.0), FVector(DoorHinge.X - LeafW - 14.0, OpenY, 55.0), !bFront);
	Solid(GlassMat(), FVector(RoomX1 - 15.0, 4928.0, 55.0), FVector(DoorHinge.X - LeafW - 14.0, 4932.0, OpenTop - 30.0), !bFront)->SetCastShadow(false);
	Solid(nullptr, FVector(RoomX1 - 15.0, 4830.0, 55.0), FVector(DoorHinge.X - LeafW - 14.0, 4840.0, 300.0), false);
	if (!bFront)
	{
		// The bars inside the glass, the black-enamel frame, the vestibule's terrazzo.
		for (double X = DoorHinge.X - LeafW - 24.0; X > RoomX1; X -= 12.0)
		{
			Block(Iron, FVector(X - 0.8, 4918.0, 55.0), FVector(X + 0.8, 4921.0, OpenTop - 30.0), false)->SetCastShadow(false);
		}
		Block(Black, FVector(OpenX1, 4930.0, OpenTop - 30.0), FVector(OpenX0, OpenY, OpenTop), false);
		Block(Mat(TEXT("PawnTerrazzo"), 0xb9b2a4, 0.3f, 4.0f), FVector(DoorHinge.X - LeafW, DoorHinge.Y, -2.0), FVector(DoorHinge.X, OpenY, 0.2), false);
		TArray<FString> Buys = {TEXT("WE BUY GOLD · LOANS"), TEXT("GUITARS · ELECTRONICS")};
		Screen(FVector(-4170.0, 4934.0, 290.0), 90.0f, FVector2D(420.0, 50.0), FIntPoint(1260, 150), 0.0f,
			[Buys](ss::ui::Canvas& C) { AShopStage::PaintCard(C, FString(), Buys, 0x000000, 0xd9b45a, true); });
	}
	Storefront(FVector((OpenX0 + OpenX1) * 0.5, OpenY, 180.0), FVector(0.0, 1.0, 0.0));
	Porch(FBox(FVector(-3960.0, OpenY, 0.0), FVector(-3700.0, OpenY + 450.0, 0.0)), TEXT("MARKET STREET"));

	FShopDoorSpec D;
	D.Hinge = DoorHinge;
	D.ShutYaw = 180.0f;
	D.InYaw = -90.0f;
	D.OpenYaw = 90.0f;
	D.Width = LeafW;
	D.Height = LeafH;
	D.Kind = EShopDoorKind::Buzzer;
	D.bGlass = true;
	Door(D, Mesh(TEXT("SM_NS_Pawn_Door"), DoorHinge, 90.0f));
	// The vestibule between the door and the window's line: on the set (the room's zone stops at the door, Market's at 49.0 m).
	Entry(FBox(FVector(DoorHinge.X - LeafW - 14.0, DoorHinge.Y - 15.0, 0.0), FVector(DoorHinge.X + 12.0, OpenY + 10.0, 0.0)), TEXT("MARKET STREET"));
	Grille(FVector(OpenX1, 4940.0, 0.0), FVector(OpenX0, 4943.0, 345.0), Mesh(TEXT("SM_NS_Pawn_Grille"), FVector(-4100.0, 4941.5, 345.0), 90.0f));

	// --- the sign: the gold balls on their bracket off the west pier, the blue tube round the window (lit all night)
	const FVector SignAt(-3730.0, OpenY, 425.0);
	if (Mesh(TEXT("SM_NS_Pawn_Sign"), SignAt, 90.0f))
	{
		Glow(Mesh(TEXT("SM_NS_Pawn_Sign_GlowNeon"), SignAt, 90.0f), EShopLight::SignAlways, 0x2f7dff, 5.0f);
		Glow(Mesh(TEXT("SM_NS_Pawn_Sign_GlowWarm"), SignAt, 90.0f), EShopLight::Sign, 0xffd9a0, 2.0f);
	}
	else
	{
		UMaterialInstanceDynamic* Tube = Mat(TEXT("NeonBlue"), 0x3aa8ff, 0.3f, 0.0f, 5.0f);
		Glow(Tube, EShopLight::SignAlways);
		const double Y = 4935.0;
		Block(Tube, FVector(-4430.0, Y, 262.0), FVector(-3910.0, Y + 2.0, 265.0), false)->SetCastShadow(false);
		Block(Tube, FVector(-4430.0, Y, 62.0), FVector(-3910.0, Y + 2.0, 65.0), false)->SetCastShadow(false);
		Block(Tube, FVector(-4430.0, Y, 62.0), FVector(-4427.0, Y + 2.0, 265.0), false)->SetCastShadow(false);
		Block(Tube, FVector(-3913.0, Y, 62.0), FVector(-3910.0, Y + 2.0, 265.0), false)->SetCastShadow(false);
		UMaterialInstanceDynamic* Gold = Mat(TEXT("PawnGold"), 0xd9a520, 0.25f, 0.0f, 1.5f, 0.9f);
		Glow(Gold, EShopLight::Sign);
		Block(Iron, FVector(-3734.0, OpenY, 392.0), FVector(-3726.0, OpenY + 80.0, 398.0), false);
		for (int32 Ball = 0; Ball < 3; ++Ball)
		{
			const FVector At(-3730.0 + (Ball - 1) * 16.0, OpenY + 62.0, Ball == 1 ? 362.0 : 376.0);
			UStaticMeshComponent* B = NewPart<UStaticMeshComponent>();
			B->SetStaticMesh(SphereMesh);
			B->SetMaterial(0, Gold);
			B->SetRelativeLocation(At);
			B->SetRelativeScale3D(FVector(0.15));
			B->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Block(Iron, At + FVector(-0.5, -0.5, 7.0), FVector(At.X + 0.5, At.Y + 0.5, 392.0), false)->SetCastShadow(false);
		}
	}
	RectLight(FVector(-4170.0, 4933.0, 163.0), FRotator(0.0f, 90.0f, 0.0f), 520.0f, 200.0f, 10.0f, 0x2f7dff, 700.0f, EShopLight::SignAlways);
	SpotLight(FVector(-3730.0, 5012.0, 446.0), FRotator(-90.0f, 0.0f, 0.0f), 20.0f, 0xffd9a0, 300.0f, 20.0f, 40.0f, EShopLight::Sign);

	// --- the guitar wall, the shelves, the counter and its cases, the cage with its window, the partition and the bench
	Solid(Mat(TEXT("PawnSlat"), 0x4a3a2c, 0.7f), FVector(-3790.0, 4400.0, 0.0), FVector(RoomX0, 4820.0, 334.0), Site(TEXT("SM_NS_Pawn_GuitarWall")) == nullptr);
	Solid(Steel, FVector(RoomX1, 4390.0, 0.0), FVector(-4400.0, 4810.0, 249.0), Site(TEXT("SM_NS_Pawn_Shelves")) == nullptr);
	const bool bCounter = Site(TEXT("SM_NS_Pawn_Counter")) != nullptr;
	Solid(GlassMat(), FVector(RoomX1, CounterY0, 0.0), FVector(-4040.0, CounterY1, 100.0), !bCounter);
	Solid(Black, FVector(-4040.0, CounterY0, 0.0), FVector(-3980.0, CounterY1, 100.0), !bCounter);
	if (!bCounter)
	{
		Block(Black, FVector(RoomX1, CounterY0, 0.0), FVector(-4040.0, CounterY1, 30.0), false);
		Block(Mat(TEXT("Velvet"), 0x101010, 0.95f), FVector(RoomX1, CounterY0 + 5.0, 30.0), FVector(-4040.0, CounterY1 - 5.0, 36.0), false);
	}
	else
	{
		Glow(Site(TEXT("SM_NS_Pawn_Counter_GlowCool")), EShopLight::Fixture, 0xfff4ea, 2.0f);
	}
	const bool bCage = Site(TEXT("SM_NS_Pawn_Cage")) != nullptr;
	Solid(Diamond, FVector(CageX1, CounterY0 - 10.0, 0.0), FVector(RoomX0, CounterY0, Ceiling), !bCage);
	Solid(Diamond, FVector(CageX1, RoomY0, 0.0), FVector(CageX1 + 10.0, CounterY0, Ceiling), !bCage);
	Solid(Painted, FVector(RoomX1, PartitionY, 0.0), FVector(CageX1, PartitionY + 10.0, Ceiling), !bShell);
	if (bCage)
	{
		Glow(Site(TEXT("SM_NS_Pawn_Cage_GlowWarm")), EShopLight::Fixture, 0xffc58f, 2.0f);
	}
	Mesh(TEXT("SM_NS_Pawn_SafeDoor"), FVector(-3892.6, 4209.2, 0.0), 90.0f);
	Site(TEXT("SM_NS_Pawn_Tickets"));
	Mesh(TEXT("SM_NS_Pawn_Loupe"), Tray + FVector(14.0, 0.0, 0.5), 90.0f);
	if (Site(TEXT("SM_NS_Pawn_Bench")))
	{
		Glow(Site(TEXT("SM_NS_Pawn_Bench_GlowCool")), EShopLight::Fixture, 0xeef4ff, 1.5f);
	}
	if (Site(TEXT("SM_NS_Pawn_Floor")))
	{
		// The island's pieces (the slot machine, the tower case, the tool chest, the bicycle, the bust): solid.
		Solid(nullptr, FVector(-4260.0, 4500.0, 0.0), FVector(-4080.0, 4790.0, 180.0), false);
	}
	Spinner(Mesh(TEXT("SM_NS_Pawn_FanBlades"), FVector(-4100.0, 4650.0, 312.0), 90.0f), FRotator(0.0f, 720.0f, 0.0f), true);
	// The two clocks in the wall of clocks that keep the world's time.
	ClockHands(FVector(-4330.0, 4055.5, 248.0), 90.0f, 13.3f);
	ClockHands(FVector(-4398.0, 4055.5, 258.0), 90.0f, 16.3f);

	// --- the lights (spec 7.5.5): six strips, the cases' LEDs, the desk lamp, the blue tube, the balls' spot, the spill
	if (UStaticMeshComponent* Tubes = Site(TEXT("SM_NS_Pawn_Shell_GlowCool")))
	{
		Glow(Tubes, EShopLight::Fixture, 0xeef4ff, 1.2f);
	}
	else
	{
		UMaterialInstanceDynamic* Lens = Mat(TEXT("PawnStrip"), 0xeef4ff, 0.3f, 0.0f, 3.0f);
		Glow(Lens, EShopLight::Fixture);
		for (const FVector2D& At : {FVector2D(-3940.0, 4760.0), FVector2D(-4260.0, 4760.0), FVector2D(-3940.0, 4520.0), FVector2D(-4260.0, 4520.0), FVector2D(-4280.0, 4180.0),
				 FVector2D(-3920.0, 3960.0)})
		{
			Block(Lens, FVector(At.X - 5.0, At.Y - 122.0, 300.0), FVector(At.X + 5.0, At.Y + 122.0, 304.0), false)->SetCastShadow(false);
		}
	}
	for (const FVector2D& At : {FVector2D(-3940.0, 4760.0), FVector2D(-4260.0, 4760.0), FVector2D(-3940.0, 4520.0), FVector2D(-4260.0, 4520.0), FVector2D(-4280.0, 4180.0),
			 FVector2D(-3920.0, 3960.0)})
	{
		RectLight(FVector(At.X, At.Y, 298.0), FRotator(-90.0f, 0.0f, 0.0f), 244.0f, 12.0f, 140.0f, 0xffe9d0, 650.0f, EShopLight::Fixture);
	}
	RectLight(FVector(-4250.0, 4376.0, 93.5), FRotator(-90.0f, 90.0f, 0.0f), 414.0f, 4.0f, 40.0f, 0xfff4ea, 200.0f, EShopLight::Fixture);
	RectLight(FVector(-4250.0, 4312.0, 93.5), FRotator(-90.0f, 90.0f, 0.0f), 414.0f, 4.0f, 40.0f, 0xfff4ea, 200.0f, EShopLight::Fixture);
	SpotLight(FVector(-3938.0, 4255.0, 134.5), FRotator(-90.0f, 0.0f, 0.0f), 30.0f, 0xffc58f, 260.0f, 25.0f, 55.0f, EShopLight::Fixture);
	RectLight(FVector((OpenX0 + OpenX1) * 0.5, OpenY + 25.0, 300.0), FRotator(-35.0f, 90.0f, 0.0f), 650.0f, 200.0f, 20.0f, 0xfff0dc, 800.0f, EShopLight::Spill);
	PointLight(FVector(-3900.0, 4200.0, 250.0), 3.0f, 0xffc58f, 350.0f, EShopLight::Night);

	// --- the uses, stations, shots and the room
	Use(TEXT("pawn-counter"), EStreetVerb::Shop, FVector(-4200.0, 4440.0, 100.0), 120.0f, TEXT("Sell or pawn"), TEXT("pawn-counter"));
	Use(TEXT("pawn-cage"), EStreetVerb::Shop, FVector(-3860.0, 4370.0, 100.0), 90.0f, TEXT("Tickets"), TEXT("pawn-cage"));
	Use(TEXT("pawn-cases"), EStreetVerb::Shop, FVector(-4093.0, 4430.0, 100.0), 80.0f, TEXT("Look in the cases"), TEXT("pawn-cases"));
	Use(TEXT("pawn-electronics"), EStreetVerb::Shop, FVector(-4330.0, 4600.0, 100.0), 140.0f, TEXT("Used gear"), TEXT("pawn-electronics"));
	Use(TEXT("pawn-guitars"), EStreetVerb::Talk, FVector(-3830.0, 4610.0, 120.0), 140.0f, TEXT("Look at the guitars"), TEXT("augie"));
	Station(TEXT("pawn-counter"), FVector(-4200.0, 4250.0, 0.0), 90.0f, TEXT("Appraise"));
	Station(TEXT("pawn-cage"), FVector(-3860.0, 4240.0, 0.0), 90.0f, TEXT("Cage"));
	Station(TEXT("pawn-bench"), FVector(-4330.0, 4115.0, 0.0), -90.0f, TEXT("Bench"));
	Station(TEXT("pawn-door"), FVector(-3994.0, 4262.0, 0.0), 90.0f, TEXT("Buzz"));
	Shot(TEXT("pawn-counter"), FVector(-4130.0, 4580.0, 172.0), FVector(-4200.0, 4300.0, 115.0), 45.0f);
	Shot(TEXT("pawn-case"), FVector(-4093.0, 4440.0, 150.0), FVector(-4093.0, 4345.0, 38.0), 50.0f);
	Shot(TEXT("pawn-cage"), FVector(-3860.0, 4430.0, 160.0), FVector(-3860.0, 4250.0, 110.0), 45.0f);
	Zone(FBox(FVector(RoomX1, RoomY0, -20.0), FVector(RoomX0, DoorHinge.Y - 5.0, Ceiling)), TEXT("NORTHSIDE PAWN & LOAN"), StreetRoomTone::Pawn, 140.0f, 80.0f);
}

void APawnStage::SyncSession(const ss::Session& S)
{
	// The hours decal on the display's glass by the door (core's sign), read from the street.
	if (const ss::shop::Venue* V = ss::shop::FindVenue("pawn"))
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
				HoursDecal = ScreenIndex(Screen(FVector(-3940.0, 4931.0, 152.0), 90.0f, FVector2D(30.0, 20.0), FIntPoint(300, 200), 1.1f, Paint));
			}
			else
			{
				Redraw(HoursDecal, Paint);
			}
		}
	}
	// The shelf: what's for sale now, by the session (its V; the auction's lots tagged as they came).
	const std::vector<ss::shop::Lot> Shelf = S.PawnShelf();
	FString Key;
	for (const ss::shop::Lot& L : Shelf)
	{
		Key += Utf8ToF(L.Id) + FString::Printf(TEXT("=%lld;"), static_cast<long long>(L.ValueCents));
	}
	if (Key == StockKey)
	{
		return;
	}
	StockKey = Key;
	for (UActorComponent* Part : Stock)
	{
		DropPart(Part);
	}
	Stock.Reset();
	StockLines.Reset();
	auto IsAuction = [&S](const std::string& Lot) {
		for (const ss::shop::ShelfLot& On : S.Life.Pawn.Shelf)
		{
			if (On.Lot == Lot && On.Auction)
			{
				return true;
			}
		}
		return false;
	};
	auto Tag = [this](const FVector& At, float Yaw, const FVector2D& Size, const FString& Price, const FString& Under) {
		if (UWidgetComponent* Wc = Screen(At, Yaw, Size, FIntPoint(FMath::RoundToInt32(Size.X * 20.0), FMath::RoundToInt32(Size.Y * 20.0)), 0.0f,
				[Price, Under](ss::ui::Canvas& C) { AShopStage::PaintTag(C, Price, Under); }))
		{
			Stock.Add(Wc);
		}
	};
	auto Stand = [this](const TCHAR* Name, const FVector& At, float Yaw, float Scale) {
		if (UStaticMeshComponent* C = Mesh(Name, At, Yaw))
		{
			C->SetRelativeScale3D(FVector(Scale));
			Stock.Add(C);
			return true;
		}
		return false;
	};
	bool bTaken[12] = {false, false, false, false, false, false, false, false, false, false, false, false};
	int32 Curio = 0;
	// Wide pieces first (each a pair of sockets), then the rest one to a socket; the case's curios and mementos in order.
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		for (const ss::shop::Lot& L : Shelf)
		{
			const FString Id = Utf8ToF(L.Id);
			const FString Price = Dollars(L.ValueCents);
			const FString Under = IsAuction(L.Id) ? FString(TEXT("LOT 114 · NINTH ST.")) : FString();
			if (L.Kind == ss::shop::LotKind::Gear && Id.StartsWith(TEXT("gear:")))
			{
				int32 Sockets = 0;
				const TCHAR* Name = GearMesh(Id.Mid(5), Sockets);
				if (!Name || Sockets == 0 || (Pass == 0) != (Sockets == 2))
				{
					continue;
				}
				int32 Slot = INDEX_NONE;
				for (int32 G = 0; G < 12 && Slot == INDEX_NONE; G += Sockets == 2 ? 2 : 1)
				{
					if (!bTaken[G] && (Sockets == 1 || !bTaken[G + 1]))
					{
						Slot = G;
					}
				}
				if (Slot == INDEX_NONE)
				{
					StockLines.Add(FString::Printf(TEXT("%s %s (no room on the shelf)"), *Id, *Price));
					continue;
				}
				bTaken[Slot] = true;
				const FVector At = Sockets == 2 ? (GearSockets[Slot] + GearSockets[Slot + 1]) * 0.5 : GearSockets[Slot];
				if (Sockets == 2)
				{
					bTaken[Slot + 1] = true;
				}
				// The apartment's gear faces the room (west); the key lights and the dual rig stand as their pair.
				if (Id == TEXT("gear:key-lights"))
				{
					Stand(TEXT("SM_KeyLight_L"), At + FVector(0.0, 12.0, 0.0), 0.0f, 1.0f);
					Stand(TEXT("SM_KeyLight_R"), At + FVector(0.0, -12.0, 0.0), 0.0f, 1.0f);
				}
				else if (Id == TEXT("gear:tower-dual"))
				{
					Stand(Name, At + FVector(0.0, 13.0, 0.0), 0.0f, 1.0f);
					Stand(Name, At + FVector(0.0, -13.0, 0.0), 0.0f, 1.0f);
				}
				else
				{
					Stand(Name, At, 0.0f, 1.0f);
				}
				Tag(At + FVector(14.0, -20.0, 10.0), 0.0f, FVector2D(9.0, 5.0), Price, Under);
				StockLines.Add(FString::Printf(TEXT("G%d %s %s%s"), Slot, *Id, *Price, Under.IsEmpty() ? TEXT("") : TEXT(" (lot 114)")));
			}
			else if (Pass == 1 && L.Kind != ss::shop::LotKind::Gear && L.Kind != ss::shop::LotKind::Garment)
			{
				if (Curio >= 6)
				{
					StockLines.Add(FString::Printf(TEXT("%s %s (no room in the case)"), *Id, *Price));
					continue;
				}
				const FVector At = CurioSockets[Curio++];
				// The aviators and the trilby on their pillows, the cup on its base; anything without a mesh on a velvet pad.
				const TCHAR* Name = CaseMesh(Id);
				if (!Name || !Stand(Name, At, 90.0f, Id == TEXT("memento:cup") ? 0.55f : 1.0f))
				{
					if (UStaticMeshComponent* Pad = Block(Mat(TEXT("Velvet"), 0x101010, 0.95f), At + FVector(-11.0, -9.0, 0.0), At + FVector(11.0, 9.0, 3.0), false))
					{
						Stock.Add(Pad);
					}
				}
				Tag(At + FVector(0.0, 12.0, 2.0), 90.0f, FVector2D(8.0, 4.5), Price, Under);
				StockLines.Add(FString::Printf(TEXT("C%d %s %s"), Curio - 1, *Id, *Price));
			}
		}
	}
}

FString APawnStage::DescribeStock() const
{
	return StockLines.Num() == 0 ? FString(TEXT("nothing on the shelves")) : FString::Join(StockLines, TEXT(", "));
}
