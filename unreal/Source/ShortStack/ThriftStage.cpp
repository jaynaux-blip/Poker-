#include "ThriftStage.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ShortStack/Game/Hero.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Shop.h"
#include "ShortStack/Game/Wardrobe.h"

// Second Chance Thrift (spec 5.2), in stage cm (spec 0.3), laid out from ns_thrift's review plan (its LAYOUT), with one
// departure recorded here: the plan stands the fitting booth's middle on the back wall's line (Y 3650), so it stands 52 cm
// further in (Y 3702), its back against the wall. Picks: the troffers 90 cd each at 4000 K, the two window spots 120 cd at
// 3000 K (the plan's six lenses glow; two lights carry them, spec 7.5.5), the camera's boom 150 cm at 80 degrees. The racks
// hang core's week (wardrobe::ThriftWeek, restocked Thursdays) from the career's seed, as the thrift's screen sells it, each
// garment in its own color (wardrobe::Tone): the racks never show what the register won't ring up.

namespace ThriftStageDetail
{
constexpr double OpenX0 = -2440.0;
constexpr double OpenX1 = -3660.0;
constexpr double OpenY = 4950.0;
constexpr double OpenTop = 400.0;
constexpr double RoomX0 = -2460.0; // the west wall's face
constexpr double RoomX1 = -3640.0; // the east wall's face
constexpr double RoomY0 = 3650.0;  // the back wall's face
constexpr double RoomY1 = 4920.0;
constexpr double Ceiling = 330.0;
// The double doors at the middle, recessed 1.2 m (their plane Y 4830).
constexpr double DoorY = 4830.0;
const FVector DoorHingeW(-2962.0, DoorY, 0.0);
const FVector DoorHingeE(-3138.0, DoorY, 0.0);
constexpr double LeafW = 88.0;
constexpr double LeafH = 215.0;

// The plan (stage cm, facing F).
struct FPlace
{
	const TCHAR* Mesh;
	double X;
	double Y;
	float F;
	double Z;
};
const FPlace Counter = {TEXT("SM_NS_Thrift_Counter"), -2700.0, 4420.0, 180.0f, 0.0};
const FPlace Case = {TEXT("SM_NS_Thrift_Case"), -2700.0, 4267.0, 180.0f, 0.0};
const FPlace HatWall = {TEXT("SM_NS_Thrift_HatWall"), -2460.0, 3890.0, 180.0f, 0.0};
const FPlace ShoeWall = {TEXT("SM_NS_Thrift_ShoeWall"), -2592.0, 3650.0, 90.0f, 0.0};
const FPlace RoundA = {TEXT("SM_NS_Thrift_RackRound"), -3330.0, 4610.0, 0.0f, 0.0};
const FPlace RoundB = {TEXT("SM_NS_Thrift_RackRound"), -2850.0, 4740.0, 30.0f, 0.0};
const FPlace Table = {TEXT("SM_NS_Thrift_Table"), -3360.0, 4250.0, 90.0f, 0.0};
const FPlace RackOuter = {TEXT("SM_NS_Thrift_RackStraight"), -2800.0, 3990.0, 0.0f, 0.0};
const FPlace RackPants = {TEXT("SM_NS_Thrift_RackStraight"), -3300.0, 3990.0, 0.0f, 0.0};
const FPlace Booth = {TEXT("SM_NS_Thrift_Booth"), -3450.0, 3702.0, 90.0f, 0.0};
const FPlace Misc = {TEXT("SM_NS_Thrift_Misc"), -3640.0, 4200.0, 0.0f, 0.0};
const FPlace MannequinA = {TEXT("SM_NS_Thrift_Mannequin"), -2680.0, 4892.0, 90.0f, 50.0};
const FPlace MannequinB = {TEXT("SM_NS_Thrift_Mannequin_B"), -3440.0, 4892.0, 100.0f, 50.0};
// The hanging tag signs over the racks and walls (their faces core's words), and what each says.
struct FTag
{
	double X;
	double Y;
	float F;
	const TCHAR* Text;
};
const FTag TagSigns[] = {
	{-3330.0, 4610.0, 0.0f, TEXT("TEES")}, {-2800.0, 3990.0, 0.0f, TEXT("JACKETS")}, {-3300.0, 3990.0, 0.0f, TEXT("PANTS")},
	{-2592.0, 3880.0, 90.0f, TEXT("SHOES")}, {-2520.0, 3890.0, 0.0f, TEXT("HATS & GLASSES")},
};
// Where the hero stands to see the three-way mirror (the booth's MIRROR_AT, Blender -1.05, 1.32), facing it; the mirror.
const FVector MirrorAt(0.0, 1.32, 1.20);
const FVector MirrorMark(-1.05, 1.32, 0.0);
const FVector CurtainAt(-0.54, -0.50, 2.06);

const TCHAR* const Meshes[] = {
	TEXT("SM_NS_Thrift_Front"), TEXT("SM_NS_Thrift_Front_Glass"), TEXT("SM_NS_Thrift_Front_GlowWarm"), TEXT("SM_NS_Thrift_Door"), TEXT("SM_NS_Thrift_Door_R"),
	TEXT("SM_NS_Thrift_Shell"), TEXT("SM_NS_Thrift_Shell_GlowCool"), TEXT("SM_NS_Thrift_RackRound"), TEXT("SM_NS_Thrift_RackStraight"), TEXT("SM_NS_Thrift_Table"),
	TEXT("SM_NS_Thrift_ShoeWall"), TEXT("SM_NS_Thrift_HatWall"), TEXT("SM_NS_Thrift_Case"), TEXT("SM_NS_Thrift_Booth"), TEXT("SM_NS_Thrift_Booth_Curtain"),
	TEXT("SM_NS_Thrift_Counter"), TEXT("SM_NS_Thrift_Mannequin"), TEXT("SM_NS_Thrift_Mannequin_B"), TEXT("SM_NS_Thrift_Misc"), TEXT("SM_NS_Thrift_Tags"),
	TEXT("SM_NS_Thrift_Hung_Tee"), TEXT("SM_NS_Thrift_Hung_Hoodie"), TEXT("SM_NS_Thrift_Hung_Jacket"), TEXT("SM_NS_Thrift_Hung_Pants"),
	TEXT("SM_NS_Thrift_Folded_Tee"), TEXT("SM_NS_Thrift_Folded_Jeans"), TEXT("SM_NS_Thrift_Folded_Hoodie"),
	// looks' rack statics (spec 4.7), preferred over the stand-ins once they land.
	TEXT("SM_NS_Rack_Tee"), TEXT("SM_NS_Rack_Hoodie"), TEXT("SM_NS_Rack_Flannel"), TEXT("SM_NS_Rack_Denim"), TEXT("SM_NS_Rack_Leather"), TEXT("SM_NS_Rack_Track"),
	TEXT("SM_NS_Rack_Bomber"), TEXT("SM_NS_Rack_Jeans"), TEXT("SM_NS_Rack_Chinos"),
	// What stands on the walls: the shoes and the hats the closet's kinds draw with.
	TEXT("SM_Sneaker_L"), TEXT("SM_Sneaker_R"), TEXT("SM_Hat_Beanie"), TEXT("SM_Hat_Cap"), TEXT("SM_Hat_Bucket"), TEXT("SM_Wear_Trilby"),
};

/** Whether an imported mesh is there, asked once a session (a missing one would go back to the disk every time). */
bool Have(const TCHAR* Name)
{
	static TMap<FString, bool> Known;
	if (const bool* Found = Known.Find(Name))
	{
		return *Found;
	}
	const bool bThere = AShopStage::ShopMesh(Name) != nullptr;
	Known.Add(Name, bThere);
	return bThere;
}

/** The hung shape for a garment kind: looks' rack static when it's imported, else ns_thrift's stand-in. */
const TCHAR* HungFor(ss::wardrobe::Cut Kind, bool& bTinted)
{
	using ss::wardrobe::Cut;
	bTinted = true;
	switch (Kind)
	{
	case Cut::Tee:
		return Have(TEXT("SM_NS_Rack_Tee")) ? TEXT("SM_NS_Rack_Tee") : TEXT("SM_NS_Thrift_Hung_Tee");
	case Cut::Hoodie:
		return Have(TEXT("SM_NS_Rack_Hoodie")) ? TEXT("SM_NS_Rack_Hoodie") : TEXT("SM_NS_Thrift_Hung_Hoodie");
	case Cut::Bomber:
		return Have(TEXT("SM_NS_Rack_Bomber")) ? TEXT("SM_NS_Rack_Bomber") : TEXT("SM_NS_Thrift_Hung_Jacket");
	case Cut::Flannel:
		return Have(TEXT("SM_NS_Rack_Flannel")) ? TEXT("SM_NS_Rack_Flannel") : TEXT("SM_NS_Thrift_Hung_Jacket");
	case Cut::Denim:
		return Have(TEXT("SM_NS_Rack_Denim")) ? TEXT("SM_NS_Rack_Denim") : TEXT("SM_NS_Thrift_Hung_Jacket");
	case Cut::Leather:
		return Have(TEXT("SM_NS_Rack_Leather")) ? TEXT("SM_NS_Rack_Leather") : TEXT("SM_NS_Thrift_Hung_Jacket");
	case Cut::Track:
		return Have(TEXT("SM_NS_Rack_Track")) ? TEXT("SM_NS_Rack_Track") : TEXT("SM_NS_Thrift_Hung_Jacket");
	case Cut::Jeans:
		return Have(TEXT("SM_NS_Rack_Jeans")) ? TEXT("SM_NS_Rack_Jeans") : TEXT("SM_NS_Thrift_Hung_Pants");
	case Cut::Chinos:
		return Have(TEXT("SM_NS_Rack_Chinos")) ? TEXT("SM_NS_Rack_Chinos") : TEXT("SM_NS_Thrift_Hung_Pants");
	case Cut::WorkPants:
	case Cut::Shorts:
		return TEXT("SM_NS_Thrift_Hung_Pants");
	case Cut::Sneakers:
	case Cut::WorkShoes:
		bTinted = false;
		return TEXT("SM_Sneaker_L");
	case Cut::Beanie:
		bTinted = false;
		return TEXT("SM_Hat_Beanie");
	case Cut::BallCap:
	case Cut::CapBack:
		bTinted = false;
		return TEXT("SM_Hat_Cap");
	case Cut::Bucket:
		bTinted = false;
		return TEXT("SM_Hat_Bucket");
	case Cut::Trilby:
		bTinted = false;
		return TEXT("SM_Wear_Trilby");
	default:
		bTinted = false;
		return nullptr;
	}
}

/** A garment's cloth color: core's (wardrobe::Tone) for a catalog garment; for the plain stand-in stock, its palette index in
 *  the hero's outfit tones, denim and twill in their own families. */
FLinearColor TintFor(const ss::wardrobe::Garment& G, bool bCatalog)
{
	using ss::wardrobe::Cut;
	if (G.Kind == Cut::Jeans && Have(TEXT("SM_NS_Rack_Jeans")))
	{
		// looks' hung jeans bake their own mid-wash indigo: the wash is a multiply over it (white leaves it as baked), the same
		// four washes the worn jeans use (ShortStackCharacter.cpp's JeansWashes: 0 dark, 1 mid, 2 light, 3 black).
		static const FLinearColor Washes[4] = {FLinearColor(0.6f, 0.62f, 0.68f), FLinearColor::White, FLinearColor(1.38f, 1.34f, 1.28f), FLinearColor(0.27f, 0.26f, 0.27f)};
		return Washes[FMath::Abs(G.Color) % 4];
	}
	uint32 Hex = bCatalog ? ss::wardrobe::Tone(G) : ss::hero::OutfitTone(FMath::Clamp(G.Color, 0, 7));
	if (bCatalog)
	{
		return FLinearColor::FromSRGBColor(FColor(static_cast<uint8>((Hex >> 16) & 0xff), static_cast<uint8>((Hex >> 8) & 0xff), static_cast<uint8>(Hex & 0xff)));
	}
	if (G.Kind == Cut::Jeans || G.Kind == Cut::Denim)
	{
		static const uint32 Denim[4] = {0x2e4566, 0x3b5a86, 0x5b7799, 0x1f2a3a};
		Hex = Denim[FMath::Abs(G.Color) % 4];
	}
	else if (G.Kind == Cut::Chinos)
	{
		static const uint32 Twill[4] = {0xb49a6e, 0x6b6a54, 0x2f3540, 0x8a7a64};
		Hex = Twill[FMath::Abs(G.Color) % 4];
	}
	else if (G.Kind == Cut::WorkPants)
	{
		Hex = 0x8c6a3d;
	}
	return FLinearColor::FromSRGBColor(FColor(static_cast<uint8>((Hex >> 16) & 0xff), static_cast<uint8>((Hex >> 8) & 0xff), static_cast<uint8>(Hex & 0xff)));
}
} // namespace ThriftStageDetail

using namespace ThriftStageDetail;

SHORTSTACK_STREET_PRELOAD(AThriftStage)

AThriftStage::AThriftStage()
{
	Venue = TEXT("thrift");
	VenueName = TEXT("Second Chance Thrift");
}

void AThriftStage::AppendPreload(TArray<FString>& Paths)
{
	for (const TCHAR* Name : Meshes)
	{
		Paths.Add(FStreetPreload::MeshPath(Name));
	}
	AppendShopMaterials(Paths);
}

void AThriftStage::SetGarmentKinds(uint64 Kinds)
{
	GarmentKinds = Kinds;
}

void AThriftStage::BuildInterior()
{
	Hung.Reset();
	Curtain = nullptr;
	StockedWeek = -1;
	HoursDecal = INDEX_NONE;
	HoursShown.Reset();
	// Today's Market end wall and blocks stand where the room is: carved, off the ground. Until Market's stage lands the
	// doors open onto a stretch of sidewalk of the shop's own (its porch).
	// (To Y 4956: today's rolled-down shutter on the Market block's face, 4 cm proud of it, goes with the block.)
	Carve(FBox(FVector(OpenX1 - 5.0, RoomY0 - 15.0, 1.0), FVector(OpenX0 + 5.0, OpenY + 6.0, OpenTop + 5.0)));

	const bool bShell = Site(TEXT("SM_NS_Thrift_Shell")) != nullptr;
	const bool bFront = Site(TEXT("SM_NS_Thrift_Front")) != nullptr;
	Site(TEXT("SM_NS_Thrift_Front_Glass"));
	UMaterialInterface* Lino = Mat(TEXT("ThriftLino"), 0x8d8a7b, 0.5f, 4.0f);
	UMaterialInterface* Sage = Mat(TEXT("ThriftSage"), 0x9aa58c, 0.85f);
	UMaterialInterface* Cream = Mat(TEXT("ThriftCream"), 0xe4dcc8, 0.85f);
	UMaterialInterface* Tiles = Mat(TEXT("ThriftCeiling"), 0xe9e6dc, 0.9f, 4.0f);
	UMaterialInterface* Bronze = Mat(TEXT("ThriftBronze"), 0x2b2622, 0.35f, 0.0f, 0.0f, 1.0f);
	UMaterialInterface* Walk = Mat(TEXT("Sidewalk"), 0x6f6c66, 0.75f, 3.0f);

	// --- the room
	Solid(Lino, FVector(OpenX1, RoomY0 - 15.0, -10.0), FVector(OpenX0, OpenY, 0.0), !bShell);
	Solid(Cream, FVector(RoomX0, RoomY0, 0.0), FVector(RoomX0 + 15.0, RoomY1, Ceiling), !bShell);
	Solid(Cream, FVector(RoomX1 - 15.0, RoomY0, 0.0), FVector(RoomX1, RoomY1, Ceiling), !bShell);
	Solid(Cream, FVector(RoomX1 - 15.0, RoomY0 - 15.0, 0.0), FVector(RoomX0 + 15.0, RoomY0, Ceiling), !bShell);
	if (!bShell)
	{
		Block(Sage, FVector(RoomX0 - 1.0, RoomY0, 0.0), FVector(RoomX0, RoomY1, 90.0), false);
		Block(Sage, FVector(RoomX1, RoomY0, 0.0), FVector(RoomX1 + 1.0, RoomY1, 90.0), false);
		Block(Sage, FVector(RoomX1, RoomY0, 0.0), FVector(RoomX0, RoomY0 + 1.0, 90.0), false);
		Block(Tiles, FVector(RoomX1, RoomY0, Ceiling), FVector(RoomX0, RoomY1, Ceiling + 6.0), false);
	}

	// --- the storefront: two display windows either side of the recessed doors, their platforms and knee walls
	for (const double Side : {0.0, 1.0})
	{
		const double X0 = Side == 0.0 ? DoorHingeW.X - 3.0 : OpenX1;
		const double X1 = Side == 0.0 ? OpenX0 : DoorHingeE.X + 3.0;
		Solid(Bronze, FVector(X0, OpenY - 30.0, 0.0), FVector(X1, OpenY, 45.0), !bFront);
		Solid(GlassMat(), FVector(X0, OpenY - 12.0, 45.0), FVector(X1, OpenY - 10.0, 300.0), !bFront)->SetCastShadow(false);
		Solid(Mat(TEXT("ThriftCarpet"), 0x5b2a2a, 0.9f), FVector(X0, DoorY + 10.0, 0.0), FVector(X1, OpenY - 30.0, 50.0), !bFront);
		Solid(Mat(TEXT("ThriftPanel"), 0xd8cfb8, 0.7f), FVector(X0, DoorY, 0.0), FVector(X1, DoorY + 10.0, 108.0), !bFront);
	}
	// The recess's angled returns (as square ones) and the transom over the doors; the sign band over it all.
	Solid(Bronze, FVector(DoorHingeW.X - 3.0, DoorY, 0.0), FVector(DoorHingeW.X + 2.0, OpenY, 300.0), !bFront);
	Solid(Bronze, FVector(DoorHingeE.X - 2.0, DoorY, 0.0), FVector(DoorHingeE.X + 3.0, OpenY, 300.0), !bFront);
	Solid(Bronze, FVector(DoorHingeE.X, DoorY - 5.0, LeafH), FVector(DoorHingeW.X, DoorY + 5.0, 300.0), !bFront);
	if (!bFront)
	{
		Block(Mat(TEXT("ThriftPrism"), 0xcfd6d4, 0.2f, 0.0f, 0.4f), FVector(OpenX1, OpenY - 8.0, 300.0), FVector(OpenX0, OpenY, 330.0), false);
		TArray<FString> Under = {TEXT("St. Agnes Outreach")};
		Screen(FVector((OpenX0 + OpenX1) * 0.5, OpenY + 2.0, 366.0), 90.0f, FVector2D(1100.0, 66.0), FIntPoint(1650, 99), 0.0f,
			[Under](ss::ui::Canvas& C) { AShopStage::PaintCard(C, TEXT("SECOND CHANCE THRIFT"), Under, 0x2f4a3a, 0xf0e6c8); });
	}
	Storefront(FVector((OpenX0 + OpenX1) * 0.5, OpenY, 180.0), FVector(0.0, 1.0, 0.0));
	// The porch: the sidewalk outside the doors (the street's own lies over it once Market's stage builds it).
	Solid(Walk, FVector(DoorHingeE.X - 150.0, OpenY, -30.0), FVector(DoorHingeW.X + 150.0, OpenY + 450.0, -0.5), true);

	// --- the doors (a pair, swinging in, a bell on the frame) and the grille
	FShopDoorSpec D;
	D.Hinge = DoorHingeW;
	D.ShutYaw = 180.0f;
	D.InYaw = -90.0f;
	D.OpenYaw = 90.0f;
	D.Width = LeafW;
	D.Height = LeafH;
	D.Kind = EShopDoorKind::Swing;
	D.bGlass = true;
	D.bPair = true;
	D.Hinge2 = DoorHingeE;
	D.ShutYaw2 = 0.0f;
	D.OpenYaw2 = -90.0f;
	Door(D, Mesh(TEXT("SM_NS_Thrift_Door"), DoorHingeW, 90.0f), Mesh(TEXT("SM_NS_Thrift_Door_R"), DoorHingeE, 90.0f));
	// The recess between the doors and the windows' line: on the set (the room's zone stops at the doors, Market's at 49.0 m).
	Entry(FBox(FVector(DoorHingeE.X - 12.0, DoorY - 15.0, 0.0), FVector(DoorHingeW.X + 12.0, OpenY + 10.0, 0.0)), TEXT("MARKET STREET"));
	Grille(FVector(OpenX1, OpenY + 2.0, 0.0), FVector(OpenX0, OpenY + 6.0, OpenTop - 70.0));

	// --- the floor's furniture (the plan), each solid whether it's the art or its block-out
	auto Place = [this](const FPlace& P) { return Mesh(P.Mesh, FVector(P.X, P.Y, P.Z), P.F); };
	UMaterialInterface* Chrome = Mat(TEXT("Chrome"), 0xdfe3e6, 0.08f, 0.0f, 0.0f, 1.0f);
	UMaterialInterface* Wood = Mat(TEXT("ThriftWood"), 0x6b4a2e, 0.6f, 2.0f);
	UMaterialInterface* Slat = Mat(TEXT("ThriftSlat"), 0xd8d2c2, 0.7f);
	Solid(Wood, FVector(Counter.X - 30.5, Counter.Y - 91.5, 0.0), FVector(Counter.X + 30.5, Counter.Y + 91.5, 97.0), Place(Counter) == nullptr);
	Solid(GlassMat(), FVector(Case.X - 25.5, Case.Y - 61.0, 0.0), FVector(Case.X + 25.5, Case.Y + 61.0, 97.0), Place(Case) == nullptr);
	Solid(Slat, FVector(HatWall.X - 30.0, HatWall.Y - 61.0, 0.0), FVector(HatWall.X, HatWall.Y + 61.0, 230.0), Place(HatWall) == nullptr);
	Solid(Slat, FVector(ShoeWall.X - 122.0, ShoeWall.Y, 0.0), FVector(ShoeWall.X + 122.0, ShoeWall.Y + 30.0, 230.0), Place(ShoeWall) == nullptr);
	for (const FPlace& R : {RoundA, RoundB})
	{
		const bool bArt = Place(R) != nullptr;
		Solid(Chrome, FVector(R.X - 8.0, R.Y - 8.0, 0.0), FVector(R.X + 8.0, R.Y + 8.0, 137.0), !bArt);
		if (!bArt)
		{
			Block(Chrome, FVector(R.X - 60.0, R.Y - 60.0, 135.0), FVector(R.X + 60.0, R.Y + 60.0, 138.0), false)->SetCastShadow(false);
		}
	}
	for (const FPlace& R : {RackOuter, RackPants})
	{
		const bool bArt = Place(R) != nullptr;
		Solid(Chrome, FVector(R.X - 35.0, R.Y - 92.0, 0.0), FVector(R.X + 35.0, R.Y + 92.0, 140.0), false);
		if (!bArt)
		{
			Block(Chrome, FVector(R.X - 30.0, R.Y - 90.0, 135.0), FVector(R.X - 26.0, R.Y + 90.0, 139.0), false);
			Block(Chrome, FVector(R.X + 26.0, R.Y - 90.0, 135.0), FVector(R.X + 30.0, R.Y + 90.0, 139.0), false);
		}
	}
	Solid(Mat(TEXT("ThriftTable"), 0x9b8f78, 0.6f), FVector(Table.X - 91.5, Table.Y - 38.0, 0.0), FVector(Table.X + 91.5, Table.Y + 38.0, 74.0), Place(Table) == nullptr);
	const bool bBooth = Place(Booth) != nullptr;
	Solid(Mat(TEXT("ThriftBooth"), 0x7a5a6a, 0.7f), FVector(Booth.X - 50.0, Booth.Y - 50.0, 0.0), FVector(Booth.X + 50.0, Booth.Y - 45.0, 210.0), !bBooth);
	Solid(Mat(TEXT("ThriftBooth"), 0x7a5a6a, 0.7f), FVector(Booth.X + 45.0, Booth.Y - 50.0, 0.0), FVector(Booth.X + 50.0, Booth.Y + 50.0, 210.0), !bBooth);
	Solid(Mat(TEXT("ThriftBooth"), 0x7a5a6a, 0.7f), FVector(Booth.X - 50.0, Booth.Y - 50.0, 0.0), FVector(Booth.X - 45.0, Booth.Y + 50.0, 210.0), !bBooth);
	// The curtain on its rail (drawn while the hero changes), and the three-way mirror beside the booth.
	const FVector CurtainPoint = PropPoint(FVector(Booth.X, Booth.Y, 0.0), Booth.F, CurtainAt);
	if (UStaticMeshComponent* Drape = Mesh(TEXT("SM_NS_Thrift_Booth_Curtain"), CurtainPoint, Booth.F))
	{
		Curtain = Drape;
	}
	else
	{
		Curtain = NewPart<USceneComponent>();
		Curtain->SetRelativeLocationAndRotation(CurtainPoint, FRotator(0.0f, Booth.F + 180.0f, 0.0f));
		Block(Mat(TEXT("ThriftCurtain"), 0x6e2335, 0.9f), FVector(-2.0, -100.0, -196.0), FVector(2.0, 0.0, 0.0), false, Curtain);
	}
	CurtainNow = 0.25f;
	CurtainWant = 0.25f;
	Curtain->SetRelativeScale3D(FVector(1.0, CurtainNow, 1.0));
	const FVector MirrorPoint = PropPoint(FVector(Booth.X, Booth.Y, 0.0), Booth.F, MirrorAt);
	Mirror(MirrorPoint, FVector(0.0, 1.0, 0.0), FVector2D(70.0, 170.0), !bBooth);
	Solid(Mat(TEXT("ThriftWood"), 0x6b4a2e, 0.6f, 2.0f), MirrorPoint + FVector(-45.0, -18.0, -120.0), MirrorPoint + FVector(45.0, -4.0, 60.0), false);
	const bool bMisc = Place(Misc) != nullptr;
	Solid(Wood, FVector(Misc.X, Misc.Y - 225.0, 0.0), FVector(Misc.X + 60.0, Misc.Y + 225.0, 200.0), !bMisc);
	Place(MannequinA);
	Place(MannequinB);

	// --- the tag signs over the racks: the mesh where it's imported, its two faces core's words
	for (const FTag& T : TagSigns)
	{
		Mesh(TEXT("SM_NS_Thrift_Tags"), FVector(T.X, T.Y, Ceiling), T.F);
		const FString Text = T.Text;
		const FVector Face(FMath::Cos(FMath::DegreesToRadians(T.F)), FMath::Sin(FMath::DegreesToRadians(T.F)), 0.0);
		for (const double Sign : {1.0, -1.0})
		{
			Screen(FVector(T.X, T.Y, Ceiling - 57.5) + Face * (1.2 * Sign), Sign > 0.0 ? T.F : T.F + 180.0f, FVector2D(76.0, 25.0), FIntPoint(456, 150), 0.0f,
				[Text](ss::ui::Canvas& C) { AShopStage::PaintCard(C, Text, {}, 0xf1c232, 0x1d1a16); });
		}
	}

	// --- the lights (spec 7.5.5: ten troffers, two window spots), the sign's flood, the spill and the night light
	if (UStaticMeshComponent* Lenses = Site(TEXT("SM_NS_Thrift_Shell_GlowCool")))
	{
		Glow(Lenses, EShopLight::Fixture, 0xeef4ff, 1.0f);
	}
	else
	{
		UMaterialInstanceDynamic* Lens = Mat(TEXT("Troffer"), 0xeef4ff, 0.3f, 0.0f, 3.0f);
		Glow(Lens, EShopLight::Fixture);
		for (const double X : {-2776.0, -3324.0})
		{
			for (const double Y : {3744.0, 3988.0, 4232.0, 4476.0, 4720.0})
			{
				Block(Lens, FVector(X - 30.0, Y - 60.0, Ceiling - 1.0), FVector(X + 30.0, Y + 60.0, Ceiling), false)->SetCastShadow(false);
			}
		}
	}
	for (const double X : {-2776.0, -3324.0})
	{
		for (const double Y : {3744.0, 3988.0, 4232.0, 4476.0, 4720.0})
		{
			RectLight(FVector(X, Y, Ceiling - 3.0), FRotator(-90.0f, 0.0f, 0.0f), 60.0f, 120.0f, 90.0f, 0xffe9d0, 700.0f, EShopLight::Fixture);
		}
	}
	Glow(Site(TEXT("SM_NS_Thrift_Front_GlowWarm")), EShopLight::Sign, 0xffd2a1, 2.0f);
	SpotLight(FVector(-2700.0, 4895.0, 319.0), FRotator(-70.0f, 90.0f, 0.0f), 120.0f, 0xffd2a1, 450.0f, 20.0f, 40.0f, EShopLight::Fixture);
	SpotLight(FVector(-3400.0, 4895.0, 319.0), FRotator(-70.0f, 90.0f, 0.0f), 120.0f, 0xffd2a1, 450.0f, 20.0f, 40.0f, EShopLight::Fixture);
	RectLight(FVector(-3050.0, 5010.0, 405.0), FRotator(10.0f, -90.0f, 0.0f), 700.0f, 40.0f, 18.0f, 0xffcc90, 500.0f, EShopLight::Sign);
	RectLight(FVector(-3050.0, OpenY + 25.0, 300.0), FRotator(-35.0f, 90.0f, 0.0f), 1100.0f, 220.0f, 26.0f, 0xfff0dc, 900.0f, EShopLight::Spill);
	PointLight(FVector(-2700.0, 4420.0, 300.0), 5.0f, 0xeef4ff, 500.0f, EShopLight::Night);

	// --- the uses, stations, shots and the room
	Use(TEXT("thrift-rack-tops"), EStreetVerb::Shop, FVector(RoundA.X, RoundA.Y, 100.0), 110.0f, TEXT("Browse the tees"), TEXT("thrift-rack-tops"));
	Use(TEXT("thrift-rack-tops2"), EStreetVerb::Shop, FVector(RoundB.X, RoundB.Y, 100.0), 110.0f, TEXT("Browse the tees"), TEXT("thrift-rack-tops"));
	Use(TEXT("thrift-rack-outer"), EStreetVerb::Shop, FVector(RackOuter.X, RackOuter.Y, 100.0), 120.0f, TEXT("Browse the jackets"), TEXT("thrift-rack-outer"));
	Use(TEXT("thrift-rack-pants"), EStreetVerb::Shop, FVector(RackPants.X, RackPants.Y, 100.0), 120.0f, TEXT("Browse the pants"), TEXT("thrift-rack-pants"));
	Use(TEXT("thrift-shoes"), EStreetVerb::Shop, FVector(ShoeWall.X, ShoeWall.Y + 80.0, 100.0), 130.0f, TEXT("Look at the shoes"), TEXT("thrift-shoes"));
	Use(TEXT("thrift-hats"), EStreetVerb::Shop, FVector(HatWall.X - 70.0, HatWall.Y, 120.0), 110.0f, TEXT("Hats & glasses"), TEXT("thrift-hats"));
	Use(TEXT("thrift-booth"), EStreetVerb::Shop, FVector(Booth.X, Booth.Y + 95.0, 100.0), 100.0f, TEXT("Try on"), TEXT("thrift-booth"));
	Use(TEXT("thrift-register"), EStreetVerb::Shop, FVector(Counter.X - 75.0, Counter.Y, 100.0), 110.0f, TEXT("Pay"), TEXT("thrift-register"));
	Use(TEXT("thrift-donate"), EStreetVerb::Shop, FVector(Misc.X + 90.0, Misc.Y + 120.0, 90.0), 100.0f, TEXT("Donate"), TEXT("thrift-donate"));
	Station(TEXT("thrift-register"), FVector(Counter.X + 55.0, Counter.Y, 0.0), 180.0f, TEXT("Register"));
	Station(TEXT("thrift-fold"), FVector(Table.X, Table.Y - 70.0, 0.0), 90.0f, TEXT("Fold"));
	Station(TEXT("thrift-rack"), FVector(RackOuter.X, RackOuter.Y + 110.0, 0.0), -90.0f, TEXT("Hang"));
	const FVector Mark = PropPoint(FVector(Booth.X, Booth.Y, 0.0), Booth.F, MirrorMark);
	ShotMarkAt(TEXT("thrift-fitting"), Mark, -90.0f);
	Shot(TEXT("thrift-fitting"), Mark + FVector(60.0, 165.0, 168.0), MirrorPoint + FVector(0.0, 0.0, 5.0), 45.0f);
	Shot(TEXT("thrift-rack"), FVector(-2950.0, 4180.0, 170.0), FVector(RackOuter.X, RackOuter.Y, 120.0), 55.0f);
	Zone(FBox(FVector(RoomX1, RoomY0, -20.0), FVector(RoomX0, DoorY - 5.0, Ceiling)), TEXT("SECOND CHANCE THRIFT"), StreetRoomTone::Thrift, 150.0f, 80.0f);
}

void AThriftStage::OnShot(FName InShot, bool bActive)
{
	if (InShot == TEXT("thrift-fitting"))
	{
		// The curtain draws while the hero changes, then opens on the three-way mirror (live while they stand at it).
		SetMirrorLive(bActive);
		CurtainWant = 1.0f;
		CurtainHold = 1.1f;
	}
}

void AThriftStage::Animate(float Dt)
{
	Super::Animate(Dt);
	if (CurtainHold > 0.0f)
	{
		CurtainHold -= Dt;
		if (CurtainHold <= 0.0f)
		{
			CurtainWant = 0.25f;
		}
	}
	if (Curtain && CurtainNow != CurtainWant)
	{
		CurtainNow = FMath::FInterpConstantTo(CurtainNow, CurtainWant, Dt, 2.4f);
		Curtain->SetRelativeScale3D(FVector(1.0, CurtainNow, 1.0));
	}
}

void AThriftStage::SyncSession(const ss::Session& S)
{
	const int32 Week = ss::wardrobe::ThriftWeek(S.WorldMinutes());
	if (Week != StockedWeek || GarmentKinds != StockedKinds)
	{
		Restock(Week, S.WorldSeed());
	}
	if (const ss::shop::Venue* V = ss::shop::FindVenue("thrift"))
	{
		const FString Sign = FString(UTF8_TO_TCHAR(ss::shop::HoursSign(*V).c_str()));
		if (!Sign.IsEmpty() && Sign != HoursShown)
		{
			HoursShown = Sign;
			TArray<FString> Lines;
			Sign.ParseIntoArray(Lines, TEXT(" · "));
			auto Paint = [Lines](ss::ui::Canvas& C) { AShopStage::PaintCard(C, TEXT("HOURS"), Lines, 0xf6f2e4, 0x1d1a16); };
			if (HoursDecal == INDEX_NONE)
			{
				HoursDecal = ScreenIndex(Screen(FVector(DoorHingeE.X - 30.0, OpenY - 9.0, 150.0), 90.0f, FVector2D(30.0, 22.0), FIntPoint(300, 220), 0.0f, Paint));
			}
			else
			{
				Redraw(HoursDecal, Paint);
			}
		}
	}
}

void AThriftStage::Restock(int32 Week, uint64 Seed)
{
	StockedWeek = Week;
	StockedKinds = GarmentKinds;
	for (TPair<FName, TObjectPtr<UInstancedStaticMeshComponent>>& H : Hung)
	{
		if (H.Value)
		{
			H.Value->ClearInstances();
		}
	}
	// What hangs where: this week's stock by slot (tops on the round racks, outerwear and pants on the straight ones, shoes
	// and hats on their walls); with no stock yet (core's catalog not in), a plain assortment so the racks aren't bare.
	struct FItem
	{
		ss::wardrobe::Cut Kind = ss::wardrobe::Cut::Tee;
		FLinearColor Tint = FLinearColor::White;
	};
	TArray<FItem> Tops;
	TArray<FItem> Outer;
	TArray<FItem> Bottoms;
	TArray<FItem> Shoes;
	TArray<FItem> Hats;
	for (const std::string& Id : ss::wardrobe::ThriftStock(Week, Seed, GarmentKinds))
	{
		const ss::wardrobe::Garment* G = ss::wardrobe::Find(Id);
		if (!G)
		{
			continue;
		}
		const FItem Item{G->Kind, TintFor(*G, true)};
		switch (G->Where)
		{
		case ss::wardrobe::Slot::Top:
			Tops.Add(Item);
			break;
		case ss::wardrobe::Slot::Outer:
			Outer.Add(Item);
			break;
		case ss::wardrobe::Slot::Bottom:
			Bottoms.Add(Item);
			break;
		case ss::wardrobe::Slot::Shoes:
			Shoes.Add(Item);
			break;
		case ss::wardrobe::Slot::Hat:
			Hats.Add(Item);
			break;
		default:
			break;
		}
	}
	FRandomStream Dice(static_cast<int32>((Seed ^ static_cast<uint64>(Week) * 0x9e3779b97f4a7c15ull) & 0x7fffffff));
	if (Tops.Num() + Outer.Num() + Bottoms.Num() == 0)
	{
		using ss::wardrobe::Cut;
		auto Tone = [&Dice]() {
			const uint32 Hex = ss::hero::OutfitTone(Dice.RandRange(0, 7));
			return FLinearColor::FromSRGBColor(FColor(static_cast<uint8>((Hex >> 16) & 0xff), static_cast<uint8>((Hex >> 8) & 0xff), static_cast<uint8>(Hex & 0xff)));
		};
		for (int32 K = 0; K < 26; ++K)
		{
			Tops.Add({K % 5 == 4 ? Cut::Hoodie : Cut::Tee, Tone()});
		}
		for (int32 K = 0; K < 14; ++K)
		{
			Outer.Add({Cut::Bomber, Tone()});
		}
		for (int32 K = 0; K < 16; ++K)
		{
			ss::wardrobe::Garment G;
			G.Kind = K % 3 == 0 ? Cut::Chinos : Cut::Jeans;
			G.Color = Dice.RandRange(0, 3);
			Bottoms.Add({G.Kind, TintFor(G, false)});
		}
		for (int32 K = 0; K < 8; ++K)
		{
			Shoes.Add({Cut::Sneakers, FLinearColor::White});
			Hats.Add({K % 3 == 0 ? Cut::Beanie : K % 3 == 1 ? Cut::BallCap : Cut::Bucket, FLinearColor::White});
		}
	}
	auto Instances = [this](const TCHAR* Name, bool bTinted) -> UInstancedStaticMeshComponent* {
		if (!Name)
		{
			return nullptr;
		}
		const FName Key(Name);
		if (TObjectPtr<UInstancedStaticMeshComponent>* Found = Hung.Find(Key))
		{
			return *Found;
		}
		UStaticMesh* Asset = Have(Name) ? ShopMesh(Name) : nullptr;
		if (!Asset)
		{
			return nullptr;
		}
		UInstancedStaticMeshComponent* Ism = NewPart<UInstancedStaticMeshComponent>();
		Ism->SetStaticMesh(Asset);
		Ism->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Ism->SetNumCustomDataFloats(bTinted ? 3 : 0);
		if (bTinted)
		{
			// The cloth slot takes its tint from each instance (M_NS_ClothTint); the hanger keeps its own.
			if (UMaterialInterface* Cloth = ClothMaterial(Asset->GetMaterial(0)))
			{
				Ism->SetMaterial(0, Cloth);
			}
		}
		Hung.Add(Key, Ism);
		return Ism;
	};
	auto Hang = [&](const FItem& Item, const FTransform& At) {
		bool bTinted = true;
		UInstancedStaticMeshComponent* Ism = Instances(HungFor(Item.Kind, bTinted), bTinted);
		if (!Ism)
		{
			return;
		}
		const int32 I = Ism->AddInstance(At, false);
		if (bTinted)
		{
			Ism->SetCustomDataValue(I, 0, Item.Tint.R, false);
			Ism->SetCustomDataValue(I, 1, Item.Tint.G, false);
			Ism->SetCustomDataValue(I, 2, Item.Tint.B, false);
		}
	};
	// A PROP's point (Blender meters) and a hung garment's Blender yaw on it, as a transform in the shop's frame.
	auto OnProp = [](const FPlace& P, const FVector& Point, float BlenderYaw) {
		return FTransform(FRotator(0.0f, P.F + 180.0f - BlenderYaw, 0.0f), AShopStage::PropPoint(FVector(P.X, P.Y, P.Z), P.F, Point));
	};
	// The round racks: every other of their 62 hooks (H_i at 0.6 m, garment yaw a - 90).
	int32 Top = 0;
	for (const FPlace& R : {RoundA, RoundB})
	{
		for (int32 H = 0; H < 62 && Top < Tops.Num(); H += 2)
		{
			const double A = FMath::DegreesToRadians(H * 360.0 / 62.0);
			Hang(Tops[Top++], OnProp(R, FVector(0.6 * FMath::Cos(A), 0.6 * FMath::Sin(A), 1.386), static_cast<float>(H * 360.0 / 62.0 - 90.0)));
		}
	}
	// The straight racks: outerwear on one, pants on the other, every other hook of each rail (garment yaw 90).
	auto Rails = [&](const FPlace& R, const TArray<FItem>& Items) {
		int32 N = 0;
		for (int32 H = 0; H < 58 && N < Items.Num(); H += 2)
		{
			const double X = H < 29 ? -0.28 : 0.28;
			const double Y = -0.84 + 0.06 * (H % 29);
			Hang(Items[N++], OnProp(R, FVector(X, Y, 1.3835), 90.0f));
		}
	};
	Rails(RackOuter, Outer);
	Rails(RackPants, Bottoms);
	// The tees table: the tops left over folded in its eight stacks (or plain tees).
	for (int32 T = 0; T < 8; ++T)
	{
		const double X = T % 2 == 0 ? -0.19 : 0.19;
		const double Y = -0.66 + 0.44 * (T / 2);
		const FItem Item = Top < Tops.Num() ? Tops[Top++] : FItem{ss::wardrobe::Cut::Tee, FLinearColor(0.8f, 0.8f, 0.78f)};
		const TCHAR* Folded = Item.Kind == ss::wardrobe::Cut::Hoodie ? TEXT("SM_NS_Thrift_Folded_Hoodie") : TEXT("SM_NS_Thrift_Folded_Tee");
		if (UInstancedStaticMeshComponent* Ism = Instances(Folded, true))
		{
			const int32 I = Ism->AddInstance(OnProp(Table, FVector(X, Y, 0.74), 0.0f), false);
			Ism->SetCustomDataValue(I, 0, Item.Tint.R, false);
			Ism->SetCustomDataValue(I, 1, Item.Tint.G, false);
			Ism->SetCustomDataValue(I, 2, Item.Tint.B, false);
		}
	}
	// The shoe wall's shelves (a sneaker at each, toe along the wall) and the hat wall's forms (yaw -90 on each).
	for (int32 K = 0; K < Shoes.Num() && K < 25; ++K)
	{
		const double Z = 0.62 + 0.34 * (K / 5);
		const double Y = -0.96 + 0.48 * (K % 5);
		bool bTinted = false;
		const TCHAR* Shoe = K % 2 == 0 ? HungFor(Shoes[K].Kind, bTinted) : TEXT("SM_Sneaker_R");
		if (UInstancedStaticMeshComponent* Ism = Instances(Shoe, false))
		{
			Ism->AddInstance(OnProp(ShoeWall, FVector(-0.11, Y, Z + 0.085), 0.0f), false);
		}
	}
	for (int32 K = 0; K < Hats.Num() && K < 12; ++K)
	{
		const double Y = -0.42 + 0.28 * (K % 4);
		const double Z = 1.295 + 0.36 * (K / 4);
		bool bTinted = false;
		if (UInstancedStaticMeshComponent* Ism = Instances(HungFor(Hats[K].Kind, bTinted), false))
		{
			Ism->AddInstance(OnProp(HatWall, FVector(-0.27, Y, Z), -90.0f), false);
		}
	}
	for (TPair<FName, TObjectPtr<UInstancedStaticMeshComponent>>& H : Hung)
	{
		if (H.Value)
		{
			H.Value->MarkRenderStateDirty();
		}
	}
}
