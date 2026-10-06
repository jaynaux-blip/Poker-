// The street's test hooks (design spec 7.7): the console's ss.Street.* and ss.Zone.*, and their UFUNCTION twins on the
// game mode for the Python remote-control scripts (street_ctl.py: mode.test_describe(), mode.test_go("barber")...).
// ue-frame owns this file; each track adds its own hooks here (a console command calling a Test* twin).
//
//   ss.Street.Teleport <home|store|counter|washfold|wash|attendant|backdoor|dee>   The Block's places, as they were
//   ss.Street.Go <start|door|plot>      a registry start ("home", "corner", "barber"), a door ("store", "pawn") or a plot's
//                                       front (spec 2.3: "H-CASH", "MS3", "V-BANK"); one off today's set is refused
//   ss.Street.Use                       E where the player stands
//   ss.Street.Describe                  one paragraph: the hour, the place and zone, the use in reach, the needs, the open
//                                       screen, the frame budget, the registry, the shops
//   ss.Street.GameNight                 the clock on to Dee's next game night
//   ss.Street.Shot <W1..W9|I1..I6|shot id|off>   spec 9's cameras (I1..I6 once the shops register their shots)
//   ss.Zone.Show [seconds]              draws the zones, uses, stations, starts, doors and walkways
//   ss.Street.Preload                   the preload list: every registered path, and what the stage has saved
//   ss.Street.KeepOverrides 0|1         (StreetWorld.cpp) whether test overrides survive into the next Play session

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "ShopStage.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStackCharacter.h"
#include "StreetGameMode.h"
#include "StreetStage.h"
#include "StreetWorld.h"

DEFINE_LOG_CATEGORY_STATIC(LogStreetHooks, Log, All);

namespace StreetCommandsDetail
{
FString Utf8(const std::string& S)
{
	return FString(UTF8_TO_TCHAR(S.c_str()));
}

/** A plot's front as spec 2.3 lays it out: where to stand on the sidewalk before it (stage cm, a meter up) and which way to face. */
struct FPlotFront
{
	const TCHAR* Id;
	double X;
	double Y;
	float Yaw;
};

// The home side's fronts face west from X 0 (stand at X 220 facing east, yaw 180); the far side's face east from X 2040
// (stand at X 1840, yaw 0); Market's south side faces north from Y 4950 (stand at Y 5175, yaw -90), its north side south
// from Y 6950 (stand at Y 6725, yaw 90). The landmarks from the near sidewalk across their cross street.
const FPlotFront Plots[] = {
	{TEXT("H-HOLLIS"), 220.0, -4900.0, 180.0f},
	{TEXT("H-KIM"), 220.0, -2875.0, 180.0f},
	{TEXT("1812"), 220.0, 0.0, 180.0f},
	{TEXT("H-CASH"), 220.0, 1550.0, 180.0f},
	{TEXT("WALKUP"), 220.0, 2990.0, 180.0f},
	{TEXT("PENNY"), 220.0, 4325.0, 180.0f},
	{TEXT("RUBY"), 220.0, 7475.0, 180.0f},
	{TEXT("H-WEXLER"), 220.0, 8600.0, 180.0f},
	{TEXT("F-HOLLIS"), 1840.0, -5250.0, 0.0f},
	{TEXT("F-LOBBY"), 1840.0, -3300.0, 0.0f},
	{TEXT("F-BOTANICA"), 1840.0, -1590.0, 0.0f},
	{TEXT("TENEMENT"), 1840.0, 920.0, 0.0f},
	{TEXT("F-RECORDS"), 1840.0, 5522.0, 0.0f},
	{TEXT("F-WEXLER"), 1840.0, 7750.0, 0.0f},
	{TEXT("M-S1"), -1900.0, 5175.0, -90.0f},
	{TEXT("M-S2"), -3050.0, 5175.0, -90.0f},
	{TEXT("M-S3"), -4100.0, 5175.0, -90.0f},
	{TEXT("M-S4"), -5550.0, 5175.0, -90.0f},
	{TEXT("M-S5"), -7150.0, 5175.0, -90.0f},
	{TEXT("M-S6"), -8650.0, 5175.0, -90.0f},
	{TEXT("M-N2"), -2150.0, 6725.0, 90.0f},
	{TEXT("M-N3"), -3200.0, 6725.0, 90.0f},
	{TEXT("M-N4"), -3950.0, 6725.0, 90.0f},
	{TEXT("M-N5"), -5300.0, 6725.0, 90.0f},
	{TEXT("M-N6"), -6950.0, 6725.0, 90.0f},
	{TEXT("M-N7"), -8600.0, 6725.0, 90.0f},
	{TEXT("V-BANK"), 1020.0, -6410.0, -90.0f},
	{TEXT("V-RIALTO"), 1020.0, 9410.0, 90.0f},
	{TEXT("V-LIBRARY"), -9810.0, 5950.0, 180.0f},
	{TEXT("C-S"), -9810.0, 3575.0, 180.0f},
	{TEXT("C-N"), -9810.0, 8375.0, 180.0f},
};

/** A plot id as typed or as an asset name writes it: "M-S3", "ms3", "MS3" are one. */
FString PlotKey(const FString& Id)
{
	return Id.ToUpper().Replace(TEXT("-"), TEXT("")).Replace(TEXT("_"), TEXT("")).Replace(TEXT(" "), TEXT(""));
}

/**
 * Whether the hero could stand at At (a meter over where they'd stand): ground within 4 m under it, and nothing solid
 * where their capsule would be (people aside). The set's zones run past its walls, so a plot's front can be inside them
 * and inside a building all the same: today's end buildings stand where the bank and the Rialto will, and Market's north
 * side stands where its sidewalk will be.
 */
bool RoomToStand(const APawn& Hero, const FVector& At, FString& OutWhy)
{
	UWorld* World = Hero.GetWorld();
	const UCapsuleComponent* Capsule = Hero.FindComponentByClass<UCapsuleComponent>();
	if (!World || !Capsule)
	{
		return true;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(StreetGo), false, &Hero);
	FHitResult Floor;
	if (!World->LineTraceSingleByChannel(Floor, At, At - FVector(0.0, 0.0, 400.0), ECC_Pawn, Params))
	{
		OutWhy = TEXT("nothing to stand on there");
		return false;
	}
	const float Radius = Capsule->GetScaledCapsuleRadius();
	const float Half = Capsule->GetScaledCapsuleHalfHeight();
	TArray<FOverlapResult> Hits;
	World->OverlapMultiByChannel(Hits, Floor.ImpactPoint + FVector(0.0, 0.0, Half + 2.0), FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(Radius, Half), Params);
	for (const FOverlapResult& Hit : Hits)
	{
		const AActor* Owner = Hit.GetActor();
		if (Hit.bBlockingHit && !(Owner && Owner->IsA<APawn>()))
		{
			OutWhy = FString::Printf(TEXT("no room to stand there (%s)"), Owner ? *Owner->GetName() : TEXT("something solid"));
			return false;
		}
	}
	return true;
}

/** One of spec 9's cameras by its stage cm (from, at, horizontal field of view). */
struct FAcceptanceShot
{
	const TCHAR* Id;
	FVector From;
	FVector At;
	float Fov;
};

const FAcceptanceShot WideShots[] = {
	{TEXT("W2"), FVector(1900.0, 4600.0, 170.0), FVector(200.0, 6200.0, 300.0), 70.0f},
	{TEXT("W3"), FVector(-300.0, 5200.0, 170.0), FVector(-11440.0, 5950.0, 600.0), 60.0f},
	{TEXT("W4"), FVector(-2900.0, 5300.0, 160.0), FVector(-1700.0, 5600.0, 200.0), 65.0f},
	{TEXT("W5"), FVector(300.0, -2000.0, 170.0), FVector(1020.0, -8040.0, 800.0), 55.0f},
	{TEXT("W6"), FVector(1800.0, 5400.0, 170.0), FVector(1020.0, 11040.0, 900.0), 55.0f},
	{TEXT("W7"), FVector(700.0, 2100.0, 30.0), FVector(420.0, 2400.0, -10.0), 50.0f},
	{TEXT("W8"), FVector(-6900.0, 6700.0, 170.0), FVector(-4500.0, 4950.0, 250.0), 60.0f},
	{TEXT("W9"), FVector(-9900.0, 6200.0, 170.0), FVector(-7800.0, 4950.0, 250.0), 60.0f},
};

/** The interiors' shots by their acceptance names (spec 9.1), registered by the shops. */
const TCHAR* const InteriorShots[][2] = {
	{TEXT("I1"), TEXT("barber-mirror")}, {TEXT("I2"), TEXT("diner-counter")}, {TEXT("I3"), TEXT("thrift-fitting")},
	{TEXT("I4"), TEXT("gym-floor")}, {TEXT("I5"), TEXT("pawn-counter")}, {TEXT("I6"), TEXT("pawn-cage")},
};

#if !UE_BUILD_SHIPPING
AStreetGameMode* StreetModeIn(UWorld* World)
{
	return World ? World->GetAuthGameMode<AStreetGameMode>() : nullptr;
}

FAutoConsoleCommandWithWorldAndArgs StreetTeleportCmd(TEXT("ss.Street.Teleport"),
	TEXT("ss.Street.Teleport home | store | counter | washfold | wash | attendant | backdoor | dee: puts the player there (testing)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (AStreetGameMode* M = StreetModeIn(World))
		{
			M->TestTeleport(Args.Num() > 0 ? Args[0] : FString(TEXT("washfold")));
		}
	}));

FAutoConsoleCommandWithWorldAndArgs StreetGameNightCmd(TEXT("ss.Street.GameNight"),
	TEXT("ss.Street.GameNight: the clock on to Dee's next game night, her doors just open (now, if they are); the calendar runs through the hours skipped (testing)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (AStreetGameMode* M = StreetModeIn(World))
		{
			UE_LOG(LogStreetHooks, Display, TEXT("%s"), *M->TestGameNight());
		}
	}));

FAutoConsoleCommandWithWorldAndArgs StreetUseCmd(TEXT("ss.Street.Use"),
	TEXT("ss.Street.Use: what E does where the player stands (testing without a keyboard: doors, the counter, Dee's game)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (AStreetGameMode* M = StreetModeIn(World))
		{
			UE_LOG(LogStreetHooks, Display, TEXT("%s"), *M->TestUse());
		}
	}));

FAutoConsoleCommandWithWorldAndArgs StreetDescribeCmd(TEXT("ss.Street.Describe"),
	TEXT("The street now: the hour, where the player is, the zone, what's in reach, the laundromat, Dee's game, the open screen, the frame budget, the registry."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (AStreetGameMode* M = StreetModeIn(World))
		{
			UE_LOG(LogStreetHooks, Display, TEXT("%s"), *M->TestDescribe());
		}
	}));

FAutoConsoleCommandWithWorldAndArgs StreetGoCmd(TEXT("ss.Street.Go"),
	TEXT("ss.Street.Go <start | door | plot>: to a registry start (home, corner, barber...), a door (store, washfold, pawn...) or a plot's front (H-CASH, M-S3, V-BANK...)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (AStreetGameMode* M = StreetModeIn(World))
		{
			UE_LOG(LogStreetHooks, Display, TEXT("%s"), *M->TestGo(Args.Num() > 0 ? Args[0] : FString(TEXT("home"))));
		}
	}));

FAutoConsoleCommandWithWorldAndArgs StreetShotCmd(TEXT("ss.Street.Shot"),
	TEXT("ss.Street.Shot <W1..W9 | I1..I6 | a registry shot | off>: the acceptance cameras (spec 9.1)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (AStreetGameMode* M = StreetModeIn(World))
		{
			UE_LOG(LogStreetHooks, Display, TEXT("%s"), *M->TestShot(Args.Num() > 0 ? Args[0] : FString(TEXT("off"))));
		}
	}));

FAutoConsoleCommandWithWorldAndArgs ZoneShowCmd(TEXT("ss.Zone.Show"), TEXT("ss.Zone.Show [seconds]: draws the registry's zones, uses, stations, starts, doors and walkways (10 s)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (AStreetGameMode* M = StreetModeIn(World))
		{
			UE_LOG(LogStreetHooks, Display, TEXT("%s"), *M->TestZones(Args.Num() > 0 ? FCString::Atof(*Args[0]) : 10.0f));
		}
	}));

FAutoConsoleCommandWithWorldAndArgs StreetPreloadCmd(TEXT("ss.Street.Preload"), TEXT("ss.Street.Preload: every path the street's stages register for preloading, and how many the stage saved."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		const TArray<FString> Paths = AStreetStage::PreloadPaths();
		int32 Saved = -1;
		for (TActorIterator<AStreetStage> It(World); It; ++It)
		{
			Saved = It->PreloadCount();
		}
		for (const FString& P : Paths)
		{
			UE_LOG(LogStreetHooks, Display, TEXT("  %s"), *P);
		}
		UE_LOG(LogStreetHooks, Display, TEXT("%d paths registered; the stage saved %d assets"), Paths.Num(), Saved);
	}));
#endif
} // namespace StreetCommandsDetail

using namespace StreetCommandsDetail;

// ------------------------------------------------------------------ the twins

FString AStreetGameMode::TestDescribe() const
{
	const AShortStackCharacter* Hero = GetHero();
	if (!Game || !Hero)
	{
		return TEXT("no game");
	}
	static const TCHAR* const DeeStates[4] = {TEXT("off"), TEXT("tonight"), TEXT("open"), TEXT("last hands")};
	const ss::life::State& L = Game->Session.Life;
	const FStreetUse* Use = UseInReach();
	const FStreetZone* Zone = ZoneAt(Hero->GetActorLocation());
	const UStreetWorld* Registry = StreetWorld();
	const TCHAR* Screen = Game->Counter.IsOpen() ? TEXT("counter") : Game->Menu.IsOpen() ? TEXT("menu") : TEXT("none");
	FString ShopLines;
	for (const TWeakObjectPtr<AShopStage>& S : Shops)
	{
		if (S.IsValid())
		{
			ShopLines += TEXT(" | ") + S->Describe();
		}
	}
	// The Block's fields first, as its scripts read them; then the registry's.
	const FString Block = FString::Printf(
		TEXT("%s at %s %s | %s view | spot %s \"%s\" | hunger %.0f thirst %.0f energy %.0f | $%.2f | bag %d | counter %s | inside %.2f wash %.2f door %.2f | daylight %.2f | %s | dee's game %s"),
		*Utf8(ss::net::TimeLabel(Game->Session.WorldMinutes())), *Hero->GetActorLocation().ToCompactString(), Stage ? *Stage->PlaceName(Hero->GetActorLocation()) : TEXT("?"),
		Hero->IsFirstPerson() ? TEXT("first") : TEXT("third"), Use ? *Use->Id.ToString() : TEXT("none"), Use ? *PromptFor(*Use) : TEXT(""), L.Hunger, L.Thirst, L.Energy,
		Game->Session.BankrollCents / 100.0, static_cast<int32>(L.Pantry.size()), Game->Counter.IsOpen() ? TEXT("open") : TEXT("closed"), Stage ? Stage->CameraInside() : 0.0f,
		Stage ? Stage->CameraInWash() : 0.0f, Stage ? Stage->WashDoorOpen() : 0.0f, Game->Session.Daylight(), Game->Session.Evicted() ? TEXT("evicted") : TEXT("housed"),
		DeeStates[static_cast<int32>(DeeGameAt(Game->Session.WorldMinutes()))]);
	return FString::Printf(TEXT("%s | zone %s%s | verb %s | screen %s | camera boom %.0f fov %.0f%s | %s | %s%s"), *Block, Zone ? *Zone->Id.ToString() : TEXT("none"),
		Zone && Zone->bInterior ? TEXT(" (room)") : TEXT(""), Use ? *StaticEnum<EStreetVerb>()->GetNameStringByValue(static_cast<int64>(Use->Verb)) : TEXT("none"), Screen,
		Hero->ArmLength, Hero->ThirdPersonFov, ProfileZone.IsNone() ? TEXT("") : *FString::Printf(TEXT(" (%s's profile)"), *ProfileZone.ToString()), *FrameBudget.Describe(),
		Registry ? *Registry->Describe() : TEXT("no registry"), *ShopLines);
}

void AStreetGameMode::TestTeleport(const FString& Where)
{
	AShortStackCharacter* Hero = GetHero();
	if (!Hero || !Stage)
	{
		return;
	}
	FTransform T = StartTransform(Where);
	if (Where == TEXT("counter"))
	{
		T = FTransform(FRotator(0.0f, -90.0f, 0.0f), FVector(-330.0, 4180.0, 100.0)) * Stage->GetActorTransform();
	}
	else if (Where == TEXT("attendant"))
	{
		// Across her counter from her, facing her.
		const FTransform Her = Stage->AttendantSpot();
		T = FTransform(FRotator(0.0f, -90.0f, 0.0f) + Stage->GetActorRotation(), Her.GetLocation() + Stage->GetActorTransform().TransformVector(FVector(8.0, 160.0, 12.5)));
	}
	Hero->SetActorLocation(T.GetLocation(), false, nullptr, ETeleportType::TeleportPhysics);
	if (UCharacterMovementComponent* Move = Hero->GetCharacterMovement())
	{
		Move->StopMovementImmediately();
	}
	if (AController* C = Hero->GetController())
	{
		C->SetControlRotation(T.Rotator());
	}
}

FString AStreetGameMode::TestGameNight()
{
	if (!Game || !Game->bCareer)
	{
		return TEXT("No career on the street.");
	}
	const ss::life::Activity* G = ss::life::Find("dee-game");
	ss::Session& S = Game->Session;
	if (!G)
	{
		return TEXT("No dee-game in the catalog.");
	}
	if (S.TimeSkip.Active)
	{
		return TEXT("Not during a time skip: the clock stays.");
	}
	const double Now = S.WorldMinutes();
	if (!ss::life::InWindow(*G, Now))
	{
		// To five past nine on her next night: the calendar runs through the hours skipped on the next update (texts, the
		// needs, a deadline), as the apartment's ss.Rent.Next moves its clock.
		S.LobbyMinutes += ss::life::NextOpen(*G, Now) + 5.0 - Now;
	}
	const std::string Why = ss::life::Blocked(*G, S.Life, S.LifeContext());
	return FString::Printf(TEXT("%s, Dee's game %s. The back door: %s"), *Utf8(ss::net::TimeLabel(S.WorldMinutes())),
		DeeGameAt(S.WorldMinutes()) == EDeeGame::Open ? TEXT("open") : TEXT("not open"), Why.empty() ? TEXT("sit in (E at the steel door)") : *Utf8(Why));
}

FString AStreetGameMode::TestUse()
{
	const FStreetUse* Use = UseInReach();
	if (!Use)
	{
		return TEXT("Nothing in reach.");
	}
	const FString Said = FString::Printf(TEXT("E at %s [%s, %s]: \"%s\""), *Use->Id.ToString(), *StaticEnum<EStreetVerb>()->GetNameStringByValue(static_cast<int64>(Use->Verb)),
		Use->Venue.IsNone() ? TEXT("-") : *Use->Venue.ToString(), *PromptFor(*Use));
	OnInteract();
	return Said;
}

FString AStreetGameMode::TestGo(const FString& Where)
{
	AShortStackCharacter* Hero = GetHero();
	const UStreetWorld* Registry = StreetWorld();
	if (!Hero || !Stage || !Registry)
	{
		return TEXT("No street to go about.");
	}
	FTransform T;
	FString What;
	if (Where == TEXT("counter") || Where == TEXT("attendant"))
	{
		TestTeleport(Where);
		return FString::Printf(TEXT("To The Block's %s."), *Where);
	}
	if (Registry->Start(FName(*Where), T))
	{
		What = TEXT("start");
	}
	if (What.IsEmpty())
	{
		for (const FStreetDoor& D : Registry->Doors())
		{
			if (D.Id == FName(*Where))
			{
				// Out on the sidewalk in front of it, facing it (the door's own mark is on the sidewalk's top: a meter up).
				T = FTransform(D.Outside.GetRotation(), D.Outside.GetLocation() + FVector(0.0, 0.0, 100.0));
				What = TEXT("door");
			}
		}
	}
	if (What.IsEmpty())
	{
		const FString Key = PlotKey(Where);
		for (const FPlotFront& P : Plots)
		{
			if (PlotKey(P.Id) == Key)
			{
				T = FTransform(FRotator(0.0f, P.Yaw, 0.0f), FVector(P.X, P.Y, 100.0)) * Stage->GetActorTransform();
				What = FString::Printf(TEXT("plot %s"), P.Id);
			}
		}
	}
	if (What.IsEmpty())
	{
		FString Starts;
		for (const FName& Id : Registry->StartIds())
		{
			Starts += (Starts.IsEmpty() ? TEXT("") : TEXT(", ")) + Id.ToString();
		}
		FString Doors;
		for (const FStreetDoor& D : Registry->Doors())
		{
			Doors += (Doors.IsEmpty() ? TEXT("") : TEXT(", ")) + D.Id.ToString();
		}
		return FString::Printf(TEXT("Nowhere called %s. Starts: %s. Doors: %s. Plots: spec 2.3's ids (H-CASH, M-S3, V-BANK...)."), *Where, *Starts, *Doors);
	}
	if (Registry->IsOffTheSet(T.GetLocation()))
	{
		return FString::Printf(TEXT("The %s %s is off today's set (%s): the street doesn't reach there yet."), *What, *Where, *T.GetLocation().ToCompactString());
	}
	FString Why;
	if (!RoomToStand(*Hero, T.GetLocation(), Why))
	{
		return FString::Printf(TEXT("The %s %s isn't on today's set (%s): %s."), *What, *Where, *T.GetLocation().ToCompactString(), *Why);
	}
	Hero->SetActorLocation(T.GetLocation(), false, nullptr, ETeleportType::TeleportPhysics);
	if (UCharacterMovementComponent* Move = Hero->GetCharacterMovement())
	{
		Move->StopMovementImmediately();
	}
	if (AController* C = Hero->GetController())
	{
		C->SetControlRotation(T.Rotator());
	}
	return FString::Printf(TEXT("To the %s %s at %s."), *What, *Where, *T.GetLocation().ToCompactString());
}

FString AStreetGameMode::TestShot(const FString& Which)
{
	APlayerController* Pc = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	AShortStackCharacter* Hero = GetHero();
	if (!Pc || !Hero || !Stage)
	{
		return TEXT("No street to shoot.");
	}
	auto Off = [this, Pc, Hero]() {
		Pc->SetViewTargetWithBlend(Hero, 0.0f);
		if (ShotCamera)
		{
			ShotCamera->Destroy();
			ShotCamera = nullptr;
		}
		ShotName.Reset();
	};
	const FString Id = Which.ToUpper();
	if (Id == TEXT("OFF"))
	{
		Off();
		return TEXT("Shot off: the hero's camera.");
	}
	if (Id == TEXT("W1"))
	{
		// The gameplay camera at the door, looking north up Fifth.
		Off();
		TestTeleport(TEXT("home"));
		if (AController* C = Hero->GetController())
		{
			C->SetControlRotation(FRotator(0.0f, 90.0f, 0.0f) + Stage->GetActorRotation());
		}
		return TEXT("W1: the hero at 1812's door, looking north.");
	}
	FTransform View;
	float Fov = 60.0f;
	bool bFound = false;
	for (const FAcceptanceShot& S : WideShots)
	{
		if (Id == S.Id)
		{
			const FTransform& T = Stage->GetActorTransform();
			const FVector From = T.TransformPosition(S.From);
			View = FTransform((T.TransformPosition(S.At) - From).Rotation(), From);
			Fov = S.Fov;
			bFound = true;
		}
	}
	FString ShotId = Which;
	for (const auto& Pair : InteriorShots)
	{
		if (Id == Pair[0])
		{
			ShotId = Pair[1];
		}
	}
	if (!bFound)
	{
		const UStreetWorld* Registry = StreetWorld();
		bFound = Registry && Registry->Shot(FName(*ShotId), View, Fov);
		if (!bFound)
		{
			return FString::Printf(TEXT("No shot %s yet (W1..W9 now; I1..I6 and the shops' shots once the shops register them)."), *Which);
		}
	}
	if (!ShotCamera)
	{
		FActorSpawnParameters Params;
		Params.ObjectFlags |= RF_Transient;
		ShotCamera = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), View, Params);
	}
	if (!ShotCamera)
	{
		return TEXT("Couldn't place a camera.");
	}
	ShotCamera->SetActorTransform(View);
	if (UCameraComponent* Cam = ShotCamera->GetCameraComponent())
	{
		Cam->SetFieldOfView(Fov);
		Cam->SetConstraintAspectRatio(false);
	}
	Pc->SetViewTargetWithBlend(ShotCamera, 0.0f);
	ShotName = Which;
	return FString::Printf(TEXT("%s: from %s, %.0f degrees."), *Which, *View.GetLocation().ToCompactString(), Fov);
}

FString AStreetGameMode::TestZones(float Seconds)
{
	const UStreetWorld* Registry = StreetWorld();
	if (!Registry)
	{
		return TEXT("No registry.");
	}
	Registry->DrawDebug(FMath::Max(0.1f, Seconds));
	FString Out = Registry->Describe();
	for (const FStreetZone& Z : Registry->AllZones())
	{
		Out += FString::Printf(TEXT("\n  zone %s \"%s\" p%d%s%s %s"), *Z.Id.ToString(), *Z.Place, Z.Priority, Z.bInterior ? TEXT(" room") : TEXT(""), Z.bDry ? TEXT(" dry") : TEXT(""),
			*Z.Box.ToString());
	}
	for (const TPair<int32, const FStreetUse*>& U : Registry->AllUses())
	{
		Out += FString::Printf(TEXT("\n  use %s [%s] \"%s\" at %s"), *U.Value->Id.ToString(), *StaticEnum<EStreetVerb>()->GetNameStringByValue(static_cast<int64>(U.Value->Verb)),
			*U.Value->Prompt, *UStreetWorld::UseLocation(*U.Value).ToCompactString());
	}
	for (const FName& Id : Registry->Stations(NAME_None))
	{
		Out += FString::Printf(TEXT("\n  station %s"), *Id.ToString());
	}
	return Out;
}
