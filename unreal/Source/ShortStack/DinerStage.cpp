#include "DinerStage.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Shop.h"

// Ruby's (spec 5.4), in stage cm (spec 0.3), from ns_diner's placements. Picks recorded here: the eight globes 45 cd each at
// 2700 K, the heat lamps 25 cd, both neons on round the clock (24 h), the camera's boom 150 cm at 80 degrees; each booth
// has its own camera (diner-booth1..5; diner-booth is the second booth's, spec 9.1's I2 family).

namespace DinerStageDetail
{
// The corner: the quarter-round glass (center, radius), the two faces, the room and the kitchen behind the pass wall.
const FVector2D CornerCenter(-200.0, 7150.0);
constexpr double FifthY1 = 8000.0;
constexpr double MarketX1 = -1500.0;
constexpr double RoomX0 = -40.0;
constexpr double RoomX1 = -1460.0;
constexpr double RoomY0 = 6990.0;
constexpr double PassY = 7700.0;
constexpr double KitchenY1 = 7960.0;
constexpr double Ceiling = 380.0;
constexpr double StoreyTop = 430.0;
// The corner's double door (each leaf's hinge, its stage facing F): shut, they meet on the 225-degree line; both swing out.
const FVector HingeW(-23.5, 7090.6, 0.0);
const FVector HingeS(-140.6, 6973.5, 0.0);
constexpr double LeafW = 82.0;
constexpr double LeafH = 222.0;
// The counter's stools (their axes) and the booths' tables.
const FVector Stools[9] = {
	FVector(-560.0, 7420.0, 0.0), FVector(-635.0, 7420.0, 0.0), FVector(-710.0, 7420.0, 0.0), FVector(-785.0, 7420.0, 0.0), FVector(-860.0, 7420.0, 0.0),
	FVector(-935.0, 7420.0, 0.0), FVector(-1010.0, 7420.0, 0.0), FVector(-372.0, 7495.0, 0.0), FVector(-372.0, 7565.0, 0.0),
};
constexpr double StoolSeat = 75.0;
constexpr double BoothY = 7050.0;
constexpr double BoothSeat = 46.0;
double BoothX(int32 K)
{
	return -500.0 - 190.0 * K;
}

const TCHAR* const Meshes[] = {
	TEXT("SM_NS_Ruby_Front"), TEXT("SM_NS_Ruby_Front_Glass"), TEXT("SM_NS_Ruby_Door"), TEXT("SM_NS_Ruby_Neon_Red"), TEXT("SM_NS_Ruby_Neon_Blue"),
	TEXT("SM_NS_Ruby_Shell"), TEXT("SM_NS_Ruby_Shell_GlowWarm"), TEXT("SM_NS_Ruby_Counter"), TEXT("SM_NS_Ruby_Stool"), TEXT("SM_NS_Ruby_Booth"),
	TEXT("SM_NS_Ruby_Backbar"), TEXT("SM_NS_Ruby_Backbar_GlowWarm"), TEXT("SM_NS_Ruby_PieCarousel"), TEXT("SM_NS_Ruby_Pass"), TEXT("SM_NS_Ruby_Pass_GlowWarm"),
	TEXT("SM_NS_Ruby_Pass_GlowCool"), TEXT("SM_NS_Ruby_MenuBoard"),
};
// The cook behind the pass (spec 6.1): a baked figure in a cook's pose (people-ue's pose booth, backroom_crowd.py, bakes it
// beside the card room's crowd), never a live MetaHuman. It faces +X as the crowd's figures do, its origin at its feet.
const TCHAR* const CookFigure = TEXT("/Game/ShortStack/Cast/Built/Crowd/SM_Crowd_Cook.SM_Crowd_Cook");
// Where the cook works: the diner-pass station, at the flat-top, the room at their back.
const FVector CookAt(-675.0, 7845.0, 0.0);
constexpr float CookYaw = 90.0f;
} // namespace DinerStageDetail

using namespace DinerStageDetail;

SHORTSTACK_STREET_PRELOAD(ADinerStage)

ADinerStage::ADinerStage()
{
	Venue = TEXT("diner");
	VenueName = TEXT("Ruby's");
}

void ADinerStage::AppendPreload(TArray<FString>& Paths)
{
	for (const TCHAR* Name : Meshes)
	{
		Paths.Add(FStreetPreload::MeshPath(Name));
	}
	Paths.Add(CookFigure);
	AppendShopMaterials(Paths);
}

void ADinerStage::BuildInterior()
{
	MenuBoard = INDEX_NONE;
	MenuShown.Reset();
	// Today's corner block stands where Ruby's ground storey is: carved, off the sidewalk.
	Carve(FBox(FVector(MarketX1 - 5.0, 6948.0, 1.0), FVector(6.0, FifthY1 + 5.0, StoreyTop)));

	const bool bShell = Site(TEXT("SM_NS_Ruby_Shell")) != nullptr;
	const bool bFront = Site(TEXT("SM_NS_Ruby_Front")) != nullptr;
	Site(TEXT("SM_NS_Ruby_Front_Glass"));
	UMaterialInterface* Terrazzo = Mat(TEXT("RubyTerrazzo"), 0xc9c2b4, 0.3f, 4.0f);
	UMaterialInterface* Cream = Mat(TEXT("RubyCream"), 0xeee3c9, 0.85f);
	UMaterialInterface* RubyRed = Mat(TEXT("RubyRed"), 0x9c1420, 0.35f);
	UMaterialInterface* Stainless = Mat(TEXT("Stainless"), 0xb8bcc0, 0.28f, 0.0f, 0.0f, 1.0f);
	UMaterialInterface* Laminate = Mat(TEXT("RubyLaminate"), 0xd9cfbd, 0.25f);
	UMaterialInterface* Vinyl = Mat(TEXT("RubyVinyl"), 0x7a1418, 0.45f);
	UMaterialInterface* Chrome = Mat(TEXT("Chrome"), 0xdfe3e6, 0.08f, 0.0f, 0.0f, 1.0f);

	// --- the ground under it all (the corner's sidewalk outside the curve; the floors lie a hair over it), the room
	Solid(Mat(TEXT("Sidewalk"), 0x6f6c66, 0.75f, 3.0f), FVector(MarketX1, 6950.0, -30.0), FVector(0.0, FifthY1, -0.3), true);
	Solid(Terrazzo, FVector(RoomX1, RoomY0 - 5.0, -10.0), FVector(RoomX0 + 10.0, KitchenY1, 0.0), !bShell);
	Solid(Cream, FVector(RoomX1 - 15.0, 6950.0, 0.0), FVector(RoomX1, KitchenY1, Ceiling), !bShell);
	Solid(Cream, FVector(RoomX1, PassY, 0.0), FVector(RoomX0, PassY + 15.0, Ceiling), !bShell);
	if (!bShell)
	{
		Block(RubyRed, FVector(RoomX1, PassY - 2.0, 0.0), FVector(RoomX0, PassY, 95.0), false);
		Block(Stainless, FVector(RoomX1, PassY - 2.5, 95.0), FVector(RoomX0, PassY, 100.0), false);
		Block(Mat(TEXT("RubyCeiling"), 0xf1ead8, 0.9f), FVector(RoomX1, RoomY0, Ceiling), FVector(RoomX0, PassY, Ceiling + 6.0), false);
		Block(RubyRed, FVector(-1160.0, 7440.0, 325.0), FVector(-410.0, 7700.0, Ceiling), false);
	}

	// --- the faces: Fifth's and Market's glass on stainless bulkheads, the curved corner (in short straight runs), the fascia
	Solid(Stainless, FVector(-30.0, 7150.0, 0.0), FVector(0.0, PassY, 62.0), !bFront);
	Solid(GlassMat(), FVector(-3.0, 7150.0, 62.0), FVector(0.0, PassY, 305.0), !bFront)->SetCastShadow(false);
	Solid(Mat(TEXT("GlassBlock"), 0xd6e2e4, 0.2f, 4.0f, 0.6f), FVector(-30.0, PassY, 0.0), FVector(0.0, FifthY1, 305.0), !bFront);
	Solid(Stainless, FVector(MarketX1, 6950.0, 0.0), FVector(-200.0, 6980.0, 62.0), !bFront);
	Solid(GlassMat(), FVector(MarketX1, 6950.0, 62.0), FVector(-200.0, 6953.0, 305.0), !bFront)->SetCastShadow(false);
	// The curve's two short runs either side of the door (the door's chord stops whoever walks into it while it's shut).
	Solid(GlassMat(), FVector(-30.0, HingeW.Y - 5.0, 0.0), FVector(0.0, 7150.0, 305.0), !bFront)->SetCastShadow(false);
	Solid(GlassMat(), FVector(-205.0, 6950.0, 0.0), FVector(HingeS.X + 5.0, 6980.0, 305.0), !bFront)->SetCastShadow(false);
	if (!bFront)
	{
		Block(RubyRed, FVector(-4.0, 7150.0, 315.0), FVector(4.0, FifthY1, StoreyTop), false);
		Block(RubyRed, FVector(MarketX1, 6946.0, 315.0), FVector(-200.0, 6954.0, StoreyTop), false);
		Block(RubyRed, FVector(-200.0, 6946.0, 315.0), FVector(4.0, 7150.0, StoreyTop), false);
		Block(Stainless, FVector(-6.0, HingeS.Y, LeafH), FVector(4.0, HingeW.Y, 315.0), false);
	}
	Storefront(FVector(-80.0, 7030.0, 180.0), FVector(0.7071, -0.7071, 0.0));

	FShopDoorSpec D;
	D.Hinge = HingeW;
	D.ShutYaw = 225.0f;
	D.InYaw = 135.0f;
	D.OpenYaw = 90.0f;
	D.Width = LeafW;
	D.Height = LeafH;
	D.Kind = EShopDoorKind::Swing;
	D.bGlass = true;
	D.bPair = true;
	D.Hinge2 = HingeS;
	D.ShutYaw2 = 45.0f;
	D.OpenYaw2 = -90.0f;
	Door(D, Mesh(TEXT("SM_NS_Ruby_Door"), HingeW, 135.0f), Mesh(TEXT("SM_NS_Ruby_Door"), HingeS, -45.0f));

	// --- the neons: RUBY'S in red script on the corner's fascia, OPEN 24 HOURS in blue in the Fifth face's first pane
	if (UStaticMeshComponent* Red = Site(TEXT("SM_NS_Ruby_Neon_Red")))
	{
		Glow(Red, EShopLight::Sign, 0xff1f3d, 6.0f);
	}
	else
	{
		Screen(FVector(55.0, 6905.0, 372.0), -45.0f, FVector2D(260.0, 62.0), FIntPoint(1040, 248), 6.0f, [](ss::ui::Canvas& C) {
			ss::ui::TextStyle T;
			T.Size = C.Height() * 0.78f;
			T.Weight = 700;
			T.Col = ss::ui::Hex(0xff1f3d);
			T.HAlign = ss::ui::Align::Center;
			T.VAlign = ss::ui::Baseline::Middle;
			C.Text("Ruby's", C.Width() * 0.5f, C.Height() * 0.5f, T);
		});
	}
	if (UStaticMeshComponent* Blue = Site(TEXT("SM_NS_Ruby_Neon_Blue")))
	{
		Glow(Blue, EShopLight::Sign, 0x3aa8ff, 5.0f);
	}
	else
	{
		Screen(FVector(-5.0, 7240.0, 160.0), 0.0f, FVector2D(80.0, 44.0), FIntPoint(400, 220), 5.0f,
			[](ss::ui::Canvas& C) { AShopStage::PaintCard(C, TEXT("OPEN"), {TEXT("24 HOURS")}, 0x000000, 0x3aa8ff, true); });
	}
	PointLight(FVector(70.0, 6880.0, 375.0), 22.0f, 0xff1f3d, 700.0f, EShopLight::Sign);
	PointLight(FVector(-18.0, 7240.0, 160.0), 6.0f, 0x3aa8ff, 350.0f, EShopLight::Sign);

	// --- the counter and its nine stools, the five booths, the backbar with the pie case, the pass, the menu board
	const bool bCounter = Site(TEXT("SM_NS_Ruby_Counter")) != nullptr;
	Solid(Stainless, FVector(-1160.0, 7455.0, 0.0), FVector(-485.0, 7530.0, 103.0), !bCounter);
	Solid(Stainless, FVector(-485.0, 7455.0, 0.0), FVector(-410.0, 7590.0, 103.0), !bCounter);
	if (!bCounter)
	{
		Block(Laminate, FVector(-1165.0, 7450.0, 103.0), FVector(-485.0, 7535.0, 106.0), false);
		Block(Laminate, FVector(-490.0, 7450.0, 103.0), FVector(-405.0, 7595.0, 106.0), false);
	}
	for (int32 K = 0; K < 9; ++K)
	{
		const FVector& At = Stools[K];
		const float Facing = K < 7 ? 90.0f : 180.0f;
		const bool bArt = Mesh(TEXT("SM_NS_Ruby_Stool"), At, Facing) != nullptr;
		Solid(Chrome, At + FVector(-6.0, -6.0, 0.0), At + FVector(6.0, 6.0, StoolSeat - 5.0), !bArt);
		if (!bArt)
		{
			Block(Vinyl, At + FVector(-19.0, -19.0, StoolSeat - 7.0), At + FVector(19.0, 19.0, StoolSeat), false);
		}
		const FName Id(*FString::Printf(TEXT("diner-stool%d"), K + 1));
		const FVector Toward = K < 7 ? FVector(0.0, -80.0, 0.0) : FVector(70.0, 0.0, 0.0);
		Use(Id, EStreetVerb::Sit, At + Toward * 0.6 + FVector(0.0, 0.0, 100.0), 45.0f, TEXT("Sit at the counter"), Id.ToString(),
			[]() { return FString(); });
		Seat(Id, At, Facing, StoolSeat, At + Toward, TEXT("Sit"), true, TEXT("diner-counter"));
	}
	for (int32 K = 0; K < 5; ++K)
	{
		const FVector At(BoothX(K), BoothY, 0.0);
		const bool bArt = Mesh(TEXT("SM_NS_Ruby_Booth"), At, 90.0f) != nullptr;
		Solid(Laminate, At + FVector(-38.0, -55.0, 0.0), At + FVector(38.0, 60.0, 76.0), !bArt);
		Solid(Vinyl, At + FVector(-92.0, -55.0, 0.0), At + FVector(-80.0, 70.0, 120.0), !bArt);
		Solid(Vinyl, At + FVector(80.0, -55.0, 0.0), At + FVector(92.0, 70.0, 120.0), !bArt);
		if (!bArt)
		{
			Block(Vinyl, At + FVector(-80.0, -55.0, 40.0), At + FVector(-40.0, 70.0, BoothSeat), false);
			Block(Vinyl, At + FVector(40.0, -55.0, 40.0), At + FVector(80.0, 70.0, BoothSeat), false);
		}
		const FString Booth = FString::Printf(TEXT("diner-booth%d"), K + 1);
		const FName BoothShot(*Booth);
		Use(FName(*Booth), EStreetVerb::Sit, At + FVector(-60.0, 110.0, 100.0), 70.0f, TEXT("Take a booth"), Booth + TEXT("a"), []() { return FString(); });
		Seat(FName(*(Booth + TEXT("a"))), At + FVector(-60.0, 5.0, 0.0), 0.0f, BoothSeat, At + FVector(-60.0, 125.0, 0.0), TEXT("Sit"), true, BoothShot);
		Seat(FName(*(Booth + TEXT("b"))), At + FVector(60.0, 5.0, 0.0), 180.0f, BoothSeat, At + FVector(60.0, 125.0, 0.0), TEXT("Sit"), true, BoothShot);
		Shot(BoothShot, At + FVector(0.0, 185.0, 150.0), At + FVector(0.0, 0.0, 95.0), 60.0f);
		// The globe over its table.
		PointLight(FVector(At.X, 7045.0, 238.0), 45.0f, 0xffc58f, 650.0f, EShopLight::Fixture);
	}
	const bool bBackbar = Site(TEXT("SM_NS_Ruby_Backbar")) != nullptr;
	Solid(Stainless, FVector(-1160.0, 7630.0, 0.0), FVector(-440.0, PassY, 140.0), !bBackbar);
	if (bBackbar)
	{
		Glow(Site(TEXT("SM_NS_Ruby_Backbar_GlowWarm")), EShopLight::Fixture, 0xffb469, 2.0f);
	}
	Spinner(Mesh(TEXT("SM_NS_Ruby_PieCarousel"), FVector(-583.0, 7665.0, 98.0), 0.0f), FRotator(0.0f, 12.0f, 0.0f), true);
	PointLight(FVector(-583.0, 7665.0, 135.0), 6.0f, 0xffe2b8, 160.0f, EShopLight::Fixture);
	if (Site(TEXT("SM_NS_Ruby_Pass")))
	{
		Glow(Site(TEXT("SM_NS_Ruby_Pass_GlowWarm")), EShopLight::Fixture, 0xff8a4a, 3.0f);
		Glow(Site(TEXT("SM_NS_Ruby_Pass_GlowCool")), EShopLight::Fixture, 0xeef4ff, 1.5f);
	}
	else
	{
		// The pass through the wall: a stainless shelf in its opening; the kitchen's glow beyond.
		Block(Stainless, FVector(-920.0, PassY - 20.0, 106.0), FVector(-620.0, PassY + 35.0, 110.0), false);
		Block(Mat(TEXT("KitchenTile"), 0xf2f2ee, 0.2f, 4.0f), FVector(RoomX1, KitchenY1, 0.0), FVector(RoomX0, KitchenY1 + 10.0, 300.0), false);
	}
	for (const double X : {-670.0, -770.0, -870.0})
	{
		SpotLight(FVector(X, PassY - 8.0, 172.0), FRotator(-90.0f, 0.0f, 0.0f), 25.0f, 0xff8a4a, 220.0f, 25.0f, 50.0f, EShopLight::Fixture);
	}
	RectLight(FVector(-760.0, 7840.0, 295.0), FRotator(-90.0f, 0.0f, 0.0f), 200.0f, 14.0f, 80.0f, 0xeef4ff, 500.0f, EShopLight::Fixture);
	Site(TEXT("SM_NS_Ruby_MenuBoard"));

	// --- the globes over the counter (the booths' with their tables), the spills on both sidewalks
	if (UStaticMeshComponent* Globes = Site(TEXT("SM_NS_Ruby_Shell_GlowWarm")))
	{
		Glow(Globes, EShopLight::Fixture, 0xffe2c0, 1.2f);
	}
	else
	{
		UMaterialInstanceDynamic* Milk = Mat(TEXT("MilkGlass"), 0xfff1dc, 0.3f, 0.0f, 3.0f);
		Glow(Milk, EShopLight::Fixture);
		for (const FVector& At : {FVector(-550.0, 7495.0, 255.0), FVector(-770.0, 7495.0, 255.0), FVector(-990.0, 7495.0, 255.0)})
		{
			UStaticMeshComponent* Globe = NewPart<UStaticMeshComponent>();
			Globe->SetStaticMesh(SphereMesh);
			Globe->SetMaterial(0, Milk);
			Globe->SetRelativeLocation(At);
			Globe->SetRelativeScale3D(FVector(0.3));
			Globe->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Globe->SetCastShadow(false);
		}
	}
	for (const double X : {-550.0, -770.0, -990.0})
	{
		PointLight(FVector(X, 7495.0, 248.0), 45.0f, 0xffc58f, 650.0f, EShopLight::Fixture);
	}
	RectLight(FVector(28.0, 7425.0, 300.0), FRotator(-35.0f, 0.0f, 0.0f), 550.0f, 240.0f, 30.0f, 0xffe0b0, 900.0f, EShopLight::Spill);
	RectLight(FVector(-850.0, 6925.0, 300.0), FRotator(-35.0f, -90.0f, 0.0f), 1300.0f, 240.0f, 30.0f, 0xffe0b0, 900.0f, EShopLight::Spill);

	// --- the stations, the shots and the room
	Station(TEXT("diner-counter"), FVector(-760.0, 7580.0, 0.0), -90.0f, TEXT("Wait"));
	Station(TEXT("diner-pass"), CookAt, CookYaw, TEXT("Cook"));
	// The cook at the flat-top, back to the room, seen through the pass (spec 6.1), once the pose booth has baked one; until then
	// the kitchen's sizzle and the bell are all there is of them. No collision: the kitchen isn't walkable.
	if (UStaticMesh* Figure = LoadObject<UStaticMesh>(nullptr, CookFigure, nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		UStaticMeshComponent* Cook = NewPart<UStaticMeshComponent>();
		Cook->SetStaticMesh(Figure);
		Cook->SetRelativeLocationAndRotation(CookAt, FRotator(0.0f, CookYaw, 0.0f));
		Cook->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		// Behind the pass it's seen at a distance through a slot: no ray-traced refits for it (spec 3.8's Mid rule).
		Cook->SetVisibleInRayTracing(false);
	}
	Shot(TEXT("diner-counter"), FVector(-1120.0, 7385.0, 165.0), FVector(-300.0, 7520.0, 125.0), 55.0f);
	Shot(TEXT("diner-booth"), FVector(BoothX(1), BoothY + 185.0, 150.0), FVector(BoothX(1), BoothY, 95.0), 60.0f);
	Zone(FBox(FVector(RoomX1, RoomY0 - 20.0, -20.0), FVector(RoomX0, PassY, Ceiling)), TEXT("RUBY'S"), StreetRoomTone::Diner, 150.0f, 80.0f);
}

void ADinerStage::SyncSession(const ss::Session& S)
{
	// The letter board over the pass from the menu (core's prices: what's on the board is what's on the check).
	TArray<TPair<FString, FString>> Rows;
	for (const ss::shop::Offer& O : ss::shop::Offers("diner"))
	{
		Rows.Emplace(FString(UTF8_TO_TCHAR(O.Name.c_str())), Dollars(O.PriceCents));
		if (Rows.Num() >= 12)
		{
			break;
		}
	}
	FString Key;
	for (const TPair<FString, FString>& R : Rows)
	{
		Key += R.Key + R.Value;
	}
	if (Rows.Num() == 0 || Key == MenuShown)
	{
		return;
	}
	MenuShown = Key;
	auto Paint = [Rows](ss::ui::Canvas& C) { AShopStage::PaintBoard(C, TEXT("RUBY'S"), Rows, 0x141414, 0xf4f1e8); };
	if (MenuBoard == INDEX_NONE)
	{
		MenuBoard = ScreenIndex(Screen(FVector(-770.0, PassY - 3.0, 250.0), -90.0f, FVector2D(340.0, 100.0), FIntPoint(1360, 400), 0.0f, Paint));
	}
	else
	{
		Redraw(MenuBoard, Paint);
	}
}
