#include "GymStage.h"

#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Shop.h"

// IronHouse Gym (spec 5.3), in stage cm (spec 0.3), laid out from ns_gym's review plan (its LAYOUT). Picks recorded here:
// the high-bays 380 cd each at 5000 K (one MegaLight per lens, spec 7.5.5's eight), the camera's boom 170 cm at 80 degrees
// (a 20 m room), a belt runs at 2.7 m/s while someone's on it (a 6 mph jog), a blow on the bag swings it up to 9 degrees.

namespace GymStageDetail
{
constexpr double OpenX0 = -4560.0;
constexpr double OpenX1 = -6540.0;
constexpr double OpenY = 4950.0;
constexpr double OpenTop = 450.0;
constexpr double RoomY0 = 3450.0;
constexpr double GlassY = 4940.0;
constexpr double Roof = 520.0;
// The glass door's leaf on the west jamb, a fixed sidelite east of it.
const FVector DoorHinge(-4646.0, GlassY, 0.0);
constexpr double LeafW = 96.0;
constexpr double LeafH = 220.0;
// The window row of treadmills (their runners face the street) and their belts' middles.
const double TreadX[5] = {-4960.0, -5080.0, -5200.0, -5320.0, -5440.0};
constexpr double TreadY = 4725.0;
// The heavy bags' hook eyes on the east wall's brackets.
const double BagY[3] = {4580.0, 4350.0, 4120.0};
constexpr double BagX = -6462.0;
constexpr double BagHook = 247.0;
const FVector Desk(-4760.0, 4580.0, 0.0);
const FVector Cooler(-4603.0, 4420.0, 0.0);
const FVector BenchA(-5250.0, 3700.0, 0.0);

const TCHAR* const Meshes[] = {
	TEXT("SM_NS_Gym_Front"), TEXT("SM_NS_Gym_Front_Glass"), TEXT("SM_NS_Gym_Front_GlowCool"), TEXT("SM_NS_Gym_Front_GlowNeon"), TEXT("SM_NS_Gym_Door"),
	TEXT("SM_NS_Gym_Shell"), TEXT("SM_NS_Gym_Shell_GlowCool"), TEXT("SM_NS_Gym_Treadmill"), TEXT("SM_NS_Gym_Bike"), TEXT("SM_NS_Gym_Bench"),
	TEXT("SM_NS_Gym_SquatRack"), TEXT("SM_NS_Gym_Barbell"), TEXT("SM_NS_Gym_Dumbbell"), TEXT("SM_NS_Gym_DumbbellRack"), TEXT("SM_NS_Gym_PlateTree"),
	TEXT("SM_NS_Gym_HeavyBag"), TEXT("SM_NS_Gym_Desk"), TEXT("SM_NS_Gym_Cooler"), TEXT("SM_NS_Gym_SpeedBag"), TEXT("SM_NS_Gym_CableStation"),
	TEXT("SM_NS_Gym_Mats"), TEXT("SM_NS_Gym_Lockers"), TEXT("SM_NS_Gym_Whiteboard"), TEXT("SM_NS_Gym_Fan"), TEXT("SM_NS_Gym_Fan_Blades"),
	TEXT("SM_Stock_CoolerRow_Water"), TEXT("SM_Stock_CoolerRow_Energy"),
};

/** Which workout a service is, by its offer ids ("gym/treadmill", "gym/bag", "gym/weights", "gym/trainer"). */
enum class EWorkout : uint8
{
	Run,
	Bag,
	Lift,
	Train,
};
EWorkout WorkoutOf(FName Service)
{
	const FString S = Service.ToString().ToLower();
	if (S.Contains(TEXT("tread")) || S.Contains(TEXT("run")))
	{
		return EWorkout::Run;
	}
	if (S.Contains(TEXT("bag")) || S.Contains(TEXT("box")))
	{
		return EWorkout::Bag;
	}
	if (S.Contains(TEXT("weight")) || S.Contains(TEXT("lift")))
	{
		return EWorkout::Lift;
	}
	return EWorkout::Train;
}

/** "4 PM" for 16:00, "12 PM" for noon (the hour only, as a desk's sign has it). */
FString HourWord(double MinuteOfDay)
{
	const int32 H = FMath::FloorToInt32(MinuteOfDay / 60.0) % 24;
	const int32 Twelve = H % 12 == 0 ? 12 : H % 12;
	return FString::Printf(TEXT("%d %s"), Twelve, H < 12 ? TEXT("AM") : TEXT("PM"));
}
} // namespace GymStageDetail

using namespace GymStageDetail;

SHORTSTACK_STREET_PRELOAD(AGymStage)

AGymStage::AGymStage()
{
	Venue = TEXT("gym");
	VenueName = TEXT("IronHouse Gym");
}

void AGymStage::AppendPreload(TArray<FString>& Paths)
{
	for (const TCHAR* Name : Meshes)
	{
		Paths.Add(FStreetPreload::MeshPath(Name));
	}
	AppendShopMaterials(Paths);
}

void AGymStage::BuildInterior()
{
	Bags.Reset();
	DeskSign = INDEX_NONE;
	DeskShown.Reset();
	for (int32 K = 0; K < 5; ++K)
	{
		Running[K] = false;
	}
	bBagWorkout = false;
	Carve(FBox(FVector(OpenX1 - 15.0, RoomY0 - 15.0, 1.0), FVector(OpenX0 + 15.0, OpenY + 2.0, Roof + 10.0)));

	const bool bShell = Site(TEXT("SM_NS_Gym_Shell")) != nullptr;
	const bool bFront = Site(TEXT("SM_NS_Gym_Front")) != nullptr;
	Site(TEXT("SM_NS_Gym_Front_Glass"));
	UMaterialInterface* Rubber = Mat(TEXT("GymRubber"), 0x1d1e20, 0.9f, 4.0f);
	UMaterialInterface* Brick = Mat(TEXT("GymBrick"), 0xe8e6e0, 0.8f, 1.0f);
	UMaterialInterface* Charcoal = Mat(TEXT("GymCharcoal"), 0x2a2b2e, 0.8f, 1.0f);
	UMaterialInterface* Steel = Mat(TEXT("GymSteel"), 0x1a1b1d, 0.5f, 0.0f, 0.0f, 0.8f);
	UMaterialInterface* Red = Mat(TEXT("GymRed"), 0xa3191c, 0.5f);

	// --- the room: the rubber floor, painted brick, the deck and its trusses
	Solid(Rubber, FVector(OpenX1, RoomY0 - 15.0, -10.0), FVector(OpenX0, OpenY, 0.0), !bShell);
	Solid(Brick, FVector(OpenX0, RoomY0, 0.0), FVector(OpenX0 + 15.0, GlassY, Roof), !bShell);
	Solid(Brick, FVector(OpenX1 - 15.0, RoomY0, 0.0), FVector(OpenX1, GlassY, Roof), !bShell);
	Solid(Brick, FVector(OpenX1 - 15.0, RoomY0 - 15.0, 0.0), FVector(OpenX0 + 15.0, RoomY0, Roof), !bShell);
	if (!bShell)
	{
		Block(Charcoal, FVector(OpenX0 - 1.0, RoomY0, 0.0), FVector(OpenX0, GlassY, 120.0), false);
		Block(Charcoal, FVector(OpenX1, RoomY0, 0.0), FVector(OpenX1 + 1.0, GlassY, 120.0), false);
		Block(Red, FVector(OpenX1, RoomY0, 120.0), FVector(OpenX0, RoomY0 + 1.0, 128.0), false);
		Block(Mat(TEXT("GymDeck"), 0x3a3b3d, 0.7f), FVector(OpenX1, RoomY0, Roof), FVector(OpenX0, GlassY, Roof + 8.0), false);
		for (const double X : {-4900.0, -5230.0, -5560.0, -5890.0, -6220.0})
		{
			Block(Steel, FVector(X - 8.0, RoomY0, 455.0), FVector(X + 8.0, GlassY, 470.0), false)->SetCastShadow(false);
		}
	}
	// The mirror wall on the back wall (spec 3.4).
	Mirror(FVector(-5680.0, RoomY0 + 2.0, 140.0), FVector(0.0, 1.0, 0.0), FVector2D(1440.0, 210.0), !bShell);

	// --- the storefront: a steel front on a concrete curb, eight bays of glass, the sign band; the door and its sidelite
	Solid(Mat(TEXT("GymCurb"), 0x8a867e, 0.8f, 5.0f), FVector(OpenX1, GlassY, 0.0), FVector(DoorHinge.X - LeafW - 98.0, OpenY, 45.0), !bFront);
	Solid(Mat(TEXT("GymCurb"), 0x8a867e, 0.8f, 5.0f), FVector(DoorHinge.X + 2.0, GlassY, 0.0), FVector(OpenX0, OpenY, 45.0), !bFront);
	Solid(GlassMat(), FVector(OpenX1, GlassY, 45.0), FVector(DoorHinge.X - LeafW - 2.0, GlassY + 2.0, 369.0), !bFront)->SetCastShadow(false);
	Solid(GlassMat(), FVector(DoorHinge.X + 2.0, GlassY, 45.0), FVector(OpenX0, GlassY + 2.0, 369.0), !bFront)->SetCastShadow(false);
	Solid(Steel, FVector(DoorHinge.X - LeafW - 2.0, GlassY - 2.0, LeafH), FVector(DoorHinge.X + 2.0, OpenY, 369.0), !bFront);
	if (!bFront)
	{
		for (double X = OpenX0; X >= OpenX1; X -= 156.0)
		{
			Block(Steel, FVector(X - 5.0, GlassY - 4.0, 45.0), FVector(X + 5.0, OpenY, 372.0), false);
		}
		Block(Charcoal, FVector(OpenX1, GlassY, 372.0), FVector(OpenX0, OpenY + 4.0, OpenTop), false);
		TArray<FString> Under = {TEXT("24/7 · MEMBERS ANY HOUR")};
		Screen(FVector(-5430.0, OpenY + 6.0, 411.0), 90.0f, FVector2D(700.0, 70.0), FIntPoint(1400, 140), 2.0f,
			[Under](ss::ui::Canvas& C) { AShopStage::PaintCard(C, TEXT("IRONHOUSE"), Under, 0x000000, 0xeef4ff, true); });
	}
	Glow(Site(TEXT("SM_NS_Gym_Front_GlowCool")), EShopLight::SignAlways, 0xeef4ff, 2.5f);
	Glow(Site(TEXT("SM_NS_Gym_Front_GlowNeon")), EShopLight::SignAlways, 0xff2a1c, 4.0f);
	Storefront(FVector((OpenX0 + OpenX1) * 0.5, OpenY, 200.0), FVector(0.0, 1.0, 0.0));
	Porch(FBox(FVector(DoorHinge.X - 350.0, OpenY, 0.0), FVector(DoorHinge.X + 150.0, OpenY + 450.0, 0.0)), TEXT("MARKET STREET"));

	FShopDoorSpec D;
	D.Hinge = DoorHinge;
	D.ShutYaw = 180.0f;
	D.InYaw = -90.0f;
	D.OpenYaw = 90.0f;
	D.Width = LeafW;
	D.Height = LeafH;
	D.Kind = EShopDoorKind::Fob;
	D.bGlass = true;
	Door(D, Mesh(TEXT("SM_NS_Gym_Door"), DoorHinge, 90.0f));
	// The fob reader's LED by the door: green (it's on whether the desk is or not).
	UMaterialInstanceDynamic* Led = Mat(TEXT("FobLed"), 0x2bff6a, 0.3f, 0.0f, 8.0f);
	Glow(Led, EShopLight::SignAlways);
	Block(Led, FVector(DoorHinge.X - LeafW - 6.0, OpenY + 1.0, 118.0), FVector(DoorHinge.X - LeafW - 4.0, OpenY + 2.0, 120.0), false)->SetCastShadow(false);

	// --- the floor: the treadmill row and bikes at the windows, the desk and cooler by the door, the weights, the racks, the bags
	UMaterialInterface* Machine = Mat(TEXT("GymMachine"), 0x2b2c2f, 0.55f);
	for (int32 K = 0; K < 5; ++K)
	{
		const FVector At(TreadX[K], TreadY, 0.0);
		const bool bArt = Mesh(TEXT("SM_NS_Gym_Treadmill"), At, 90.0f) != nullptr;
		Solid(Machine, At + FVector(-43.0, -100.0, 0.0), At + FVector(43.0, 100.0, 22.0), !bArt);
		Solid(Machine, At + FVector(-43.0, 80.0, 22.0), At + FVector(43.0, 100.0, 140.0), !bArt);
		// Its console's screen, facing the runner: the speed while someone's on it.
		Screen(PropPoint(At, 90.0f, FVector(-0.98, 0.0, 1.30)), -90.0f, FVector2D(22.0, 13.0), FIntPoint(220, 130), 1.4f,
			[](ss::ui::Canvas& C) { AShopStage::PaintCard(C, TEXT("0.0"), {TEXT("MPH")}, 0x0b1a24, 0x5fd0ff); });
	}
	for (const double X : {-5600.0, -5720.0})
	{
		const FVector At(X, 4740.0, 0.0);
		Solid(Machine, At + FVector(-25.0, -60.0, 0.0), At + FVector(25.0, 60.0, 100.0), Mesh(TEXT("SM_NS_Gym_Bike"), At, 90.0f) == nullptr);
	}
	const bool bDesk = Mesh(TEXT("SM_NS_Gym_Desk"), Desk, 90.0f) != nullptr;
	Solid(Charcoal, Desk + FVector(-110.0, -35.0, 0.0), Desk + FVector(110.0, 35.0, 110.0), !bDesk);
	const bool bCooler = Mesh(TEXT("SM_NS_Gym_Cooler"), Cooler, 180.0f) != nullptr;
	Solid(GlassMat(), Cooler + FVector(-43.0, -39.0, 0.0), Cooler + FVector(43.0, 39.0, 204.0), !bCooler);
	if (bCooler)
	{
		// Its shelves stocked as the Lucky Penny's cooler (its rows' meshes; the drinks themselves from store::Catalog).
		for (const double Z : {28.0, 73.0, 118.0, 163.0})
		{
			Mesh(Z < 100.0 ? TEXT("SM_Stock_CoolerRow_Water") : TEXT("SM_Stock_CoolerRow_Energy"), Cooler + FVector(0.0, 0.0, Z), -90.0f);
		}
	}
	Solid(Steel, FVector(-5520.0, RoomY0 + 10.0, 0.0), FVector(-5280.0, RoomY0 + 90.0, 100.0), Mesh(TEXT("SM_NS_Gym_DumbbellRack"), FVector(-5400.0, 3500.0, 0.0), 90.0f) == nullptr);
	Solid(Machine, BenchA + FVector(-15.0, -60.0, 0.0), BenchA + FVector(15.0, 60.0, 45.0), Mesh(TEXT("SM_NS_Gym_Bench"), BenchA, -90.0f) == nullptr);
	Solid(Machine, FVector(-5575.0, 3670.0, 0.0), FVector(-5545.0, 3790.0, 45.0), Mesh(TEXT("SM_NS_Gym_Bench"), FVector(-5560.0, 3730.0, 0.0), -80.0f) == nullptr);
	for (const double X : {-5960.0, -6290.0})
	{
		Solid(Steel, FVector(X - 60.0, 3505.0, 0.0), FVector(X + 60.0, 3625.0, 230.0), Mesh(TEXT("SM_NS_Gym_SquatRack"), FVector(X, 3565.0, 0.0), 90.0f) == nullptr);
	}
	for (const double X : {-6125.0, -6440.0})
	{
		Solid(Steel, FVector(X - 30.0, 3730.0, 0.0), FVector(X + 30.0, 3790.0, 120.0), Mesh(TEXT("SM_NS_Gym_PlateTree"), FVector(X, 3760.0, 0.0), 90.0f) == nullptr);
	}
	for (int32 K = 0; K < 3; ++K)
	{
		// Each bag hangs from its hook on a pivot of its own, which the game swings.
		USceneComponent* Pivot = NewPart<USceneComponent>();
		Pivot->SetRelativeLocation(FVector(BagX, BagY[K], BagHook));
		if (UStaticMeshComponent* Bag = Mesh(TEXT("SM_NS_Gym_HeavyBag"), FVector(BagX, BagY[K], BagHook), 0.0f))
		{
			Bag->AttachToComponent(Pivot, FAttachmentTransformRules::KeepWorldTransform);
		}
		else
		{
			Block(Mat(TEXT("GymBag"), 0x111111, 0.55f), FVector(-18.0, -18.0, -188.0), FVector(18.0, 18.0, -62.0), false, Pivot);
			Block(Steel, FVector(-0.6, -0.6, -62.0), FVector(0.6, 0.6, 0.0), false, Pivot)->SetCastShadow(false);
		}
		Solid(nullptr, FVector(BagX - 20.0, BagY[K] - 20.0, BagHook - 190.0), FVector(BagX + 20.0, BagY[K] + 20.0, BagHook - 60.0), false);
		Bags.Add(Pivot);
	}
	// The P1 pieces, where they're imported (the plan's places): the speed bag, the cable station, the mats, the lockers, the
	// whiteboard with its TV, and the big fan with its blades turning.
	Mesh(TEXT("SM_NS_Gym_SpeedBag"), FVector(OpenX1, 3850.0, 0.0), 0.0f);
	Mesh(TEXT("SM_NS_Gym_CableStation"), FVector(-6250.0, 4000.0, 0.0), 0.0f);
	Mesh(TEXT("SM_NS_Gym_Mats"), FVector(-6280.0, 4760.0, 0.0), 90.0f);
	Mesh(TEXT("SM_NS_Gym_Lockers"), FVector(OpenX0, 3720.0, 0.0), 180.0f);
	Mesh(TEXT("SM_NS_Gym_Whiteboard"), FVector(OpenX0, 4150.0, 0.0), 180.0f);
	if (Mesh(TEXT("SM_NS_Gym_Fan"), FVector(-6400.0, RoomY0, 0.0), 90.0f))
	{
		Spinner(Mesh(TEXT("SM_NS_Gym_Fan_Blades"), PropPoint(FVector(-6400.0, RoomY0, 0.0), 90.0f, FVector(-0.32, 0.0, 3.2)), 90.0f), FRotator(0.0f, 0.0f, 300.0f), false);
	}
	// The TV over the whiteboard (a ball game's replay), lit while anyone's in.
	Screen(FVector(OpenX0 - 3.0, 4150.0, 235.0), 180.0f, FVector2D(122.0, 69.0), FIntPoint(488, 276), 1.3f, [](ss::ui::Canvas& C) {
		const float W = C.Width();
		const float H = C.Height();
		C.FillRect({0.0f, 0.0f, W, H}, ss::ui::Paint::Linear({0.0f, 0.0f}, {W, H}, ss::ui::Hex(0x1f2f45), ss::ui::Hex(0x7a1d1d)));
		AShopStage::PaintCard(C, TEXT("FIGHT NIGHT"), {TEXT("ROUND 7")}, 0x000000, 0xffffff, true);
	});

	// --- the lights (spec 7.5.5: eight high-bays, the TV, the cooler, the treadmill consoles' glow), the window's spill
	if (UStaticMeshComponent* Lenses = Site(TEXT("SM_NS_Gym_Shell_GlowCool")))
	{
		Glow(Lenses, EShopLight::Fixture, 0xfff4ea, 1.2f);
	}
	else
	{
		UMaterialInstanceDynamic* Lens = Mat(TEXT("HighBay"), 0xfff4ea, 0.3f, 0.0f, 4.0f);
		Glow(Lens, EShopLight::Fixture);
		for (const double X : {-4880.0, -5370.0, -5860.0, -6350.0})
		{
			for (const double Y : {4460.0, 3980.0})
			{
				Block(Lens, FVector(X - 25.0, Y - 25.0, 400.0), FVector(X + 25.0, Y + 25.0, 404.0), false)->SetCastShadow(false);
				Block(Steel, FVector(X - 0.5, Y - 0.5, 404.0), FVector(X + 0.5, Y + 0.5, Roof), false)->SetCastShadow(false);
			}
		}
	}
	for (const double X : {-4880.0, -5370.0, -5860.0, -6350.0})
	{
		for (const double Y : {4460.0, 3980.0})
		{
			SpotLight(FVector(X, Y, 398.0), FRotator(-90.0f, 0.0f, 0.0f), 380.0f, 0xfff4ea, 1300.0f, 40.0f, 70.0f, EShopLight::Fixture);
		}
	}
	PointLight(FVector(OpenX0 - 60.0, 4150.0, 230.0), 4.0f, 0x9fb8ff, 400.0f, EShopLight::Fixture);
	PointLight(Cooler + FVector(0.0, 0.0, 120.0), 8.0f, 0xeef4ff, 300.0f, EShopLight::Fixture);
	RectLight(FVector((OpenX0 + OpenX1) * 0.5, OpenY + 25.0, 330.0), FRotator(-35.0f, 90.0f, 0.0f), 1900.0f, 260.0f, 22.0f, 0xfff4ea, 1000.0f, EShopLight::Spill);
	PointLight(FVector(-4760.0, 4500.0, 300.0), 6.0f, 0xfff4ea, 600.0f, EShopLight::Night);

	// --- the uses, stations, shots and the room
	TWeakObjectPtr<AGymStage> Self(this);
	Use(TEXT("gym-desk"), EStreetVerb::Shop, Desk + FVector(0.0, 100.0, 100.0), 110.0f, TEXT("Front desk"), TEXT("gym-desk"), [Self]() {
		if (!Self.IsValid())
		{
			return FString();
		}
		return Self->IsStaffed() ? FString() : FString::Printf(TEXT("Back at %s"), Self->StaffedAgain.IsEmpty() ? TEXT("4 PM") : *Self->StaffedAgain);
	});
	for (int32 K = 0; K < 5; ++K)
	{
		Use(FName(*FString::Printf(TEXT("gym-treadmill%d"), K + 1)), EStreetVerb::Shop, FVector(TreadX[K], TreadY - 95.0, 100.0), 70.0f, TEXT("Run"),
			FString::Printf(TEXT("gym-treadmill%d"), K + 1));
		Station(FName(*FString::Printf(TEXT("gym-treadmill%d"), K + 1)), FVector(TreadX[K], TreadY - 20.0, 22.0), 90.0f, TEXT("Run"));
	}
	Use(TEXT("gym-bag"), EStreetVerb::Shop, FVector(BagX + 80.0, BagY[0], 120.0), 120.0f, TEXT("Hit the bag"), TEXT("gym-bag"));
	Use(TEXT("gym-weights"), EStreetVerb::Shop, FVector(-5400.0, 3640.0, 100.0), 160.0f, TEXT("Lift"), TEXT("gym-weights"));
	Use(TEXT("gym-cooler"), EStreetVerb::Shop, Cooler + FVector(-75.0, 0.0, 120.0), 90.0f, TEXT("Cooler"), TEXT("gym-cooler"));
	Station(TEXT("gym-desk"), Desk + FVector(0.0, -70.0, 0.0), 90.0f, TEXT("Desk"));
	Station(TEXT("gym-floor"), FVector(-5700.0, 4150.0, 0.0), 0.0f, TEXT("Train"));
	Station(TEXT("gym-bag"), FVector(BagX + 77.0, BagY[0], 0.0), 180.0f, TEXT("Bag"));
	Seat(TEXT("gym-bench"), PropPoint(BenchA, -90.0f, FVector(-0.42, 0.0, 0.0)), -90.0f, 45.0f, BenchA + FVector(70.0, -40.0, 0.0), TEXT("Lift"));
	Shot(TEXT("gym-floor"), FVector(-5880.0, 3920.0, 180.0), FVector(-6330.0, 4580.0, 130.0), 70.0f);
	Shot(TEXT("gym-run"), FVector(-5160.0, 4560.0, 150.0), FVector(TreadX[0], TreadY, 120.0), 55.0f);
	Shot(TEXT("gym-lift"), FVector(-5050.0, 3950.0, 170.0), FVector(BenchA.X, 3660.0, 60.0), 55.0f);
	Zone(FBox(FVector(OpenX1, RoomY0, -20.0), FVector(OpenX0, GlassY - 5.0, Roof)), TEXT("IRONHOUSE GYM"), StreetRoomTone::Gym, 170.0f, 80.0f);
}

TArray<TPair<FName, float>> AGymStage::Cinematic(FName Service) const
{
	switch (WorkoutOf(Service))
	{
	case EWorkout::Run:
		return {{TEXT("gym-run"), 1.0f}};
	case EWorkout::Lift:
		return {{TEXT("gym-lift"), 1.0f}};
	default:
		return {{TEXT("gym-floor"), 1.0f}};
	}
}

bool AGymStage::ServiceMark(FName Service, FTransform& OutFeet, float& OutSeat, FName& OutBeat) const
{
	const FTransform& T = GetActorTransform();
	OutSeat = 0.0f;
	switch (WorkoutOf(Service))
	{
	case EWorkout::Run:
		// On the first treadmill's belt, facing the street.
		OutFeet = FTransform(FRotator(0.0f, 90.0f, 0.0f), FVector(TreadX[0], TreadY - 20.0, 22.0)) * T;
		OutBeat = TEXT("treadmill");
		return true;
	case EWorkout::Bag:
		OutFeet = FTransform(FRotator(0.0f, 180.0f, 0.0f), FVector(BagX + 77.0, BagY[0], 0.0)) * T;
		OutBeat = TEXT("bag");
		return true;
	case EWorkout::Lift:
		OutFeet = FTransform(FRotator(0.0f, -90.0f, 0.0f), PropPoint(BenchA, -90.0f, FVector(-0.42, 0.0, 0.0))) * T;
		OutSeat = 45.0f;
		OutBeat = TEXT("weights");
		return true;
	default:
		OutFeet = FTransform(FRotator(0.0f, 0.0f, 0.0f), FVector(-5640.0, 4150.0, 0.0)) * T;
		OutBeat = TEXT("bag");
		return true;
	}
}

void AGymStage::OnService(FName Service, bool bRunning)
{
	switch (WorkoutOf(Service))
	{
	case EWorkout::Run:
		SetTreadmillRunning(0, bRunning);
		break;
	case EWorkout::Bag:
	case EWorkout::Train:
		bBagWorkout = bRunning;
		NextBlow = 0.3f;
		break;
	default:
		break;
	}
}

void AGymStage::SetTreadmillRunning(int32 Index, bool bRunning)
{
	if (Index >= 0 && Index < 5)
	{
		Running[Index] = bRunning;
	}
}

void AGymStage::HitBag(int32 Index, float Strength)
{
	if (Index >= 0 && Index < 3)
	{
		// A blow from the room's side (+X) shoves the bag toward the wall.
		BagVel[Index] -= 42.0f * FMath::Clamp(Strength, 0.0f, 1.0f);
	}
}

void AGymStage::Animate(float Dt)
{
	Super::Animate(Dt);
	const TArray<TObjectPtr<UMaterialInstanceDynamic>>& Belts = BeltMaterials();
	for (int32 K = 0; K < 5; ++K)
	{
		if (!Running[K])
		{
			continue;
		}
		// The belt's tread runs back under the runner at a jog (its texture tiles along the run).
		BeltPan[K] = FMath::Fmod(BeltPan[K] + Dt * 2.7f, 1000.0f);
		if (Belts.IsValidIndex(K) && Belts[K])
		{
			Belts[K]->SetScalarParameterValue(TEXT("Pan"), BeltPan[K]);
		}
	}
	if (bBagWorkout)
	{
		NextBlow -= Dt;
		if (NextBlow <= 0.0f)
		{
			NextBlow = FMath::FRandRange(0.35f, 0.85f);
			HitBag(0, FMath::FRandRange(0.5f, 1.0f));
		}
	}
	for (int32 K = 0; K < Bags.Num() && K < 3; ++K)
	{
		// A heavy pendulum on its chain: a slow swing, damped.
		BagVel[K] += (-9.0f * BagSwing[K] - 1.4f * BagVel[K]) * Dt;
		BagSwing[K] = FMath::Clamp(BagSwing[K] + BagVel[K] * Dt, -9.0f, 9.0f);
		if (Bags[K] && FMath::Abs(BagSwing[K]) + FMath::Abs(BagVel[K]) > 0.01f)
		{
			Bags[K]->SetRelativeRotation(FRotator(BagSwing[K], 0.0f, 0.0f));
		}
	}
}

void AGymStage::OnHour()
{
	if (DeskSign != INDEX_NONE)
	{
		if (UWidgetComponent* Wc = ScreenAt(DeskSign))
		{
			Wc->SetVisibility(!IsStaffed());
		}
	}
}

FString AGymStage::ClosedPrompt() const
{
	// Spec 5.0: the door's line for anyone without a fob after hours.
	return FString::Printf(TEXT("Members only after hours · Staffed again %s"), StaffedAgain.IsEmpty() ? TEXT("4 PM") : *StaffedAgain);
}

void AGymStage::SyncSession(const ss::Session& S)
{
	const ss::shop::Venue* V = ss::shop::FindVenue("gym");
	if (!V)
	{
		return;
	}
	// When Rosa is next at the desk (core's staffed hours, in quarter hours over the next day).
	const double Now = S.WorldMinutes();
	FString Again;
	for (double T = Now; T < Now + 1440.0; T += 15.0)
	{
		if (ss::shop::IsStaffed(*V, T))
		{
			Again = HourWord(T - FMath::FloorToDouble(T / 1440.0) * 1440.0);
			break;
		}
	}
	StaffedAgain = Again;
	const FString Hour = Again.Left(Again.Find(TEXT(" ")));
	const FString Want = FString::Printf(TEXT("BACK AT %s"), Hour.IsEmpty() ? TEXT("4") : *Hour);
	if (Want != DeskShown)
	{
		DeskShown = Want;
		auto Paint = [Want](ss::ui::Canvas& C) { AShopStage::PaintCard(C, Want, {TEXT("Members: tap your fob")}, 0xf4f1e8, 0xa3191c); };
		if (DeskSign == INDEX_NONE)
		{
			DeskSign = ScreenIndex(Screen(Desk + FVector(60.0, 37.0, 118.0), 90.0f, FVector2D(26.0, 17.0), FIntPoint(260, 170), 0.0f, Paint));
		}
		else
		{
			Redraw(DeskSign, Paint);
		}
		OnHour();
	}
}
