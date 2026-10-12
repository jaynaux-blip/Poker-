#include "StreetGameMode.h"

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/App.h"
#include "NightOneAudio.h"
#include "NightOneSaveGame.h"
#include "SFrontEndWidget.h"
#include "Camera/CameraActor.h"
#include "Misc/PackageName.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Shop.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/Store.h"
#include "ShortStack/Game/Town.h"
#include "ShortStack/Game/Wardrobe.h"
#include "ShortStack/Game/Weather.h"
#include "ShortStack/UI/TalkCard.h"
#include "CastLooks.h"
#include "ShopStage.h"
#include "ShortStackCharacter.h"
#include "StreetAudio.h"
#include "StreetNpcs.h"
#include "StreetStage.h"
#include "StreetWorld.h"
#include "ThriftStage.h"

// The street's pedestrians, once their module file is in (the street plays without them until then).
#if __has_include("StreetCrowd.h")
#include "StreetCrowd.h"
#define SHORTSTACK_STREET_CROWD 1
#else
#define SHORTSTACK_STREET_CROWD 0
#endif

#include <algorithm>
#include <string>
#include <vector>

DEFINE_LOG_CATEGORY_STATIC(LogStreet, Log, All);

namespace StreetGameDetail
{
/** Dee deals her game's last hand at five (her doors take new players until three: the session's own window, Life.cpp). */
constexpr double DeeLastHand = 5.0 * 60.0;
/** The Wash & Fold's attendant (it's open all hours), and her smock: the neon's pink, washed out. */
const TCHAR* const AttendantName = TEXT("Inez");
const FColor AttendantSmock(0xc7, 0x5b, 0x84);
FString Utf8ToFString(const std::string& S)
{
	return FString(UTF8_TO_TCHAR(S.c_str()));
}

std::string FStringToUtf8(const FString& S)
{
	return std::string(TCHAR_TO_UTF8(*S));
}

/** "2560 x 1440" to (2560, 1440); (0, 0) when empty or malformed (as the apartment reads it). */
FIntPoint StreetParseResolution(const std::string& Text)
{
	FString Left;
	FString Right;
	if (!FString(UTF8_TO_TCHAR(Text.c_str())).Split(TEXT("x"), &Left, &Right))
	{
		return FIntPoint(0, 0);
	}
	const int32 Wd = FCString::Atoi(*Left.TrimStartAndEnd());
	const int32 Ht = FCString::Atoi(*Right.TrimStartAndEnd());
	return Wd > 0 && Ht > 0 ? FIntPoint(Wd, Ht) : FIntPoint(0, 0);
}

/** Fullscreen resolutions the display supports, largest first, as "W x H" (the pause menu's Resolution list). */
std::vector<std::string> StreetResolutions()
{
	TArray<FIntPoint> Modes;
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(Modes);
	Modes.Sort([](const FIntPoint& L, const FIntPoint& R) { return L.X * L.Y > R.X * R.Y; });
	std::vector<std::string> Out;
	for (const FIntPoint& Mode : Modes)
	{
		const std::string Label = std::to_string(Mode.X) + " x " + std::to_string(Mode.Y);
		if (std::find(Out.begin(), Out.end(), Label) == Out.end())
		{
			Out.push_back(Label);
		}
	}
	return Out;
}

void SetStreetCvar(const TCHAR* Name, int32 Value)
{
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name))
	{
		Var->Set(Value, ECVF_SetByGameSetting);
	}
}

/**
 * A camera's horizontal field of view for the player's Field of View setting (vertical degrees, 50 the default):
 * DefaultHorizontal at 50, scaled as the setting scales the view (the half-angles' tangents), so its shape holds.
 */
float StreetFov(float DefaultHorizontal, int32 VerticalSetting)
{
	const double K = FMath::Tan(FMath::DegreesToRadians(FMath::Clamp(VerticalSetting, 30, 110) * 0.5)) / FMath::Tan(FMath::DegreesToRadians(25.0));
	const double Half = FMath::Atan(FMath::Tan(FMath::DegreesToRadians(DefaultHorizontal * 0.5)) * K);
	return FMath::Clamp(static_cast<float>(FMath::RadiansToDegrees(Half * 2.0)), 50.0f, 125.0f);
}

/**
 * What Dee texts once the player is back out on the street after her game, from what the Back Room passed back (cents,
 * reads learned, busted, stayed till close): she's just seen them out of the back room, so it's short.
 */
FString NightWordFrom(const FString& Options, bool bEvicted)
{
	const int64 Net = FCString::Atoi64(*UGameplayStatics::ParseOption(Options, TEXT("Net")));
	const int32 Learned = FCString::Atoi(*UGameplayStatics::ParseOption(Options, TEXT("Learned")));
	const FString Amount = FString::Printf(TEXT("$%lld"), FMath::Abs(Net) / 100);
	FString Word;
	if (UGameplayStatics::HasOption(Options, TEXT("Busted")))
	{
		Word = bEvicted ? TEXT("rough one. go up and sleep it off, the couch is made up.") : TEXT("rough one. go home, sleep, then come back and watch more than you play.");
	}
	else if (Net >= 10000)
	{
		Word = FString::Printf(TEXT("%s off my table. sal's still muttering. don't count it on the sidewalk."), *Amount);
	}
	else if (Net > 0)
	{
		Word = FString::Printf(TEXT("up %s. that's a good night at my game. don't tell lou i said so."), *Amount);
	}
	else if (Net == 0)
	{
		Word = TEXT("broke even. nobody got hurt.");
	}
	else
	{
		Word = FString::Printf(TEXT("down %s. it happens. watch sal's eyes when the flop comes."), *Amount);
	}
	if (UGameplayStatics::HasOption(Options, TEXT("Closed")))
	{
		Word += TEXT(" thanks for staying till close.");
	}
	if (Learned > 0)
	{
		Word += Learned == 1 ? TEXT(" and you caught a tell. most people never see one.") : TEXT(" and you're reading them now. they'll start to feel it.");
	}
	return Word;
}

/** How loud a door at At sounds at Ear: full within a few steps of it, gone some 25 m off (a pedestrian opening one a block away isn't heard). */
float HeardFrom(const FVector& Ear, const FVector& At)
{
	const double Far = FVector::Dist(Ear, At);
	return static_cast<float>(FMath::Square(FMath::Clamp(1.0 - (Far - 400.0) / 2100.0, 0.0, 1.0)));
}

/** How loud one of the stage's doors (AStreetStage::Doors: "store", "washfold") sounds at Ear. */
float DoorHeard(const AStreetStage& Stage, const FVector& Ear, const TCHAR* Door)
{
	for (const FStreetDoor& D : Stage.Doors())
	{
		if (D.Id == Door)
		{
			return HeardFrom(Ear, D.Threshold);
		}
	}
	return 1.0f;
}

/** How long a zone's camera profile takes to blend in or out as the hero crosses its door (spec 7.5.3). */
constexpr float ProfileBlendSeconds = 0.3f;
/** A screen's camera blends to its shot (spec 7.5.3), and back to the hero as it closes. */
constexpr float ShotBlendSeconds = 0.4f;
/** A service's cinematic ends on a fade through black (spec 7.5.3): out, held while the look changes, in. */
constexpr float CinemaFade = 0.3f;
/** The thrift's mirror: how far the camera orbits the hero either way (spec 2.10), and how fast the stick turns it. */
constexpr float OrbitLimit = 35.0f;
constexpr float OrbitStick = 70.0f;
/** How often the shops are shown the session (the boards, the pawn's shelf, the thrift's racks). */
constexpr double ShopSyncEvery = 2.0;
/** The keeper's word before closing (spec 5.0), and how near the hero must be to hear it (cm). */
constexpr double ClosingWarn = 10.0;
constexpr double ClosingHeard = 1500.0;

/** Who keeps a venue when town doesn't say (its people aren't in yet): spec 5.0's keepers. */
const TCHAR* KeeperFallback(FName Venue, double World)
{
	const double Tod = World - FMath::FloorToDouble(World / ss::net::MinutesPerDay) * ss::net::MinutesPerDay;
	if (Venue == TEXT("barber"))
	{
		return TEXT("Lonnie");
	}
	if (Venue == TEXT("thrift"))
	{
		return TEXT("Bea");
	}
	if (Venue == TEXT("gym"))
	{
		return TEXT("Rosa");
	}
	if (Venue == TEXT("diner"))
	{
		return Tod >= 6.0 * 60.0 && Tod < 18.0 * 60.0 ? TEXT("Ruby") : TEXT("Marlene");
	}
	if (Venue == TEXT("pawn"))
	{
		return TEXT("Augie");
	}
	return TEXT("The keeper");
}

/** What a keeper says at closing when town has no line yet (the spec's voices, 5.1-5.8). */
const TCHAR* ClosingFallback(FName Venue)
{
	if (Venue == TEXT("barber"))
	{
		return TEXT("Broom's out. That means you.");
	}
	if (Venue == TEXT("pawn"))
	{
		return TEXT("Buzzer's off in ten. Ticket or cash, make it quick.");
	}
	if (Venue == TEXT("thrift"))
	{
		return TEXT("Ten minutes, sweetheart. Bring it to the counter or put it back.");
	}
	return TEXT("Closing in ten.");
}
// The console's ss.Street.* hooks and their UFUNCTION twins (TestDescribe, TestGo...) are in StreetCommands.cpp.
} // namespace StreetGameDetail

using namespace StreetGameDetail;

// ------------------------------------------------------------------ the game and its hooks

FStreetGame::FStreetGame(AStreetGameMode& InMode, const ss::SaveData* Loaded, const std::string& Seed)
	: Mode(InMode)
	, bCareer(Loaded != nullptr)
	, Session(*this, Seed, Loaded)
	, Counter(Session)
	, Menu(*this)
{
	// Out of the apartment the clock runs as it does in the lobby: real minutes.
	Session.CurrentScreen = ss::Screen::Lobby;
	// Penny Drop orders wait at the apartment's door while the player is out here (they land in the bag back home).
	Session.DeferDeliveries = true;
}

void FStreetGame::Sound(ss::SoundId Id, double Volume)
{
	if (UNightOneAudio* A = Mode.GetAudio())
	{
		A->Play(Id, static_cast<float>(Volume));
	}
}

void FStreetGame::Text(const std::string& From, const std::string& Body)
{
	Mode.Toast(Utf8ToFString(From), Utf8ToFString(Body));
	Sound(ss::SoundId::Alert, 0.5);
}

void FStreetGame::Save(const ss::SaveData& Data)
{
	// The career file the desk reads when the player gets home (an old-style slot here would be a second, stale career).
	// Its world comes as a frozen snapshot (SavesInBackground): the saver writes its text on the worker.
	if (bCareer)
	{
		Saver.Submit(Data);
	}
}

bool FStreetGame::GoOut(const std::string& ActivityId, ss::Chips BuyInCents)
{
	return Mode.GoOut(Utf8ToFString(ActivityId), static_cast<int64>(BuyInCents));
}

void FStreetGame::UiSound(ss::SoundId Id, double Volume)
{
	Sound(Id, Volume);
}

void FStreetGame::Resume()
{
	Mode.ReturnInputToGame();
}

void FStreetGame::QuitToMenu()
{
	Session.Save();
	Saver.Flush();
	Mode.SaveSettingsNow();
	UGameplayStatics::OpenLevel(&Mode, FName(TEXT("NightOne")), true, TEXT("Menu"));
}

void FStreetGame::QuitGame()
{
	Session.Save();
	Saver.Flush();
	Mode.SaveSettingsNow();
	UKismetSystemLibrary::QuitGame(&Mode, UGameplayStatics::GetPlayerController(&Mode, 0), EQuitPreference::Quit, false);
}

void FStreetGame::SettingsChanged(const ss::ui::GameSettings& Settings)
{
	Mode.ApplySettings(Settings, true);
}

// ------------------------------------------------------------------ the mode

AStreetGameMode::AStreetGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	DefaultPawnClass = AShortStackCharacter::StaticClass();
	PlayerControllerClass = AStreetPlayerController::StaticClass();
	Audio = CreateDefaultSubobject<UNightOneAudio>(TEXT("Audio"));
}

void AStreetGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	// A test's overrides from the last Play session don't carry into this one (ss.Street.KeepOverrides 1 keeps them). Here,
	// not in BeginPlay: RestartPlayer builds the street (and reads ss.Street.WideMarket) before BeginPlay runs.
	FStreetOverrides::ResetAll();
	StartFrom = UGameplayStatics::HasOption(Options, TEXT("Start")) ? UGameplayStatics::ParseOption(Options, TEXT("Start")) : TEXT("home");
	ss::SaveData Loaded;
	std::string CareerText;
	const bool bLoaded = CareerSave::LoadText(CareerText) && ss::SaveData::Parse(CareerText, Loaded);
	const std::string Seed = FStringToUtf8(FString::Printf(TEXT("street:%lld"), FDateTime::Now().GetTicks()));
	Game = MakeUnique<FStreetGame>(*this, bLoaded ? &Loaded : nullptr, Seed);
	Game->Menu.SetVenue(ss::ui::Venue::Street); // the pause heading says FIFTH STREET, the quit copy is the street's
	if (UGameplayStatics::DoesSaveGameExist(UNightOneSaveGame::SettingsSlotName(), 0))
	{
		if (UNightOneSaveGame* Obj = Cast<UNightOneSaveGame>(UGameplayStatics::LoadGameFromSlot(UNightOneSaveGame::SettingsSlotName(), 0)))
		{
			ss::ui::GameSettings::Parse(FStringToUtf8(Obj->Data), Game->Menu.Settings);
		}
	}
	Sensitivity = 0.11f * static_cast<float>(Game->Menu.Settings.LookSensitivity) / 100.0f;
	bInvertLook = Game->Menu.Settings.InvertLook;
	// Evicted, the player comes and goes by Dee's door across the street, not 1812's; so too down her stairs whenever the
	// apartment says that's where they were (Door=Dee: its test hook can put them at hers).
	if (StartFrom == TEXT("home"))
	{
		StartFrom = UGameplayStatics::ParseOption(Options, TEXT("Door")) == TEXT("Dee") ? FString(TEXT("dee")) : HomeDoor();
	}
	// Out of Dee's game, walked into from here: the hours at her table still happen on the calendar (a deadline passing
	// at 3 AM, the needs climbing; the Back Room kept the player's energy itself), and she has a word about the night.
	if (UGameplayStatics::HasOption(Options, TEXT("Night")) && Game->bCareer)
	{
		// (Only a hand-off from the save's own past: a missing or stray From would replay the career's whole calendar.)
		const double Since = FCString::Atod(*UGameplayStatics::ParseOption(Options, TEXT("From")));
		if (Since > 0.0 && Since <= Game->Session.WorldMinutes())
		{
			Game->Session.ResumeCalendarFrom(Since);
		}
		NightOptions = Options;
		NightWordAt = 3.5;
	}
	LeftAt = Game->Session.WorldMinutes();
	UE_LOG(LogStreet, Log, TEXT("Out on Fifth as %s (%s), %s"), *Utf8ToFString(Game->Session.HeroName), *Utf8ToFString(Game->Session.Person.FullName()),
		*Utf8ToFString(ss::net::TimeLabel(Game->Session.WorldMinutes())));
}

void AStreetGameMode::FindOrSpawnStage()
{
	if (Stage || !GetWorld())
	{
		return;
	}
	for (TActorIterator<AStreetStage> It(GetWorld()); It; ++It)
	{
		Stage = *It;
		return;
	}
	Stage = GetWorld()->SpawnActor<AStreetStage>(AStreetStage::StaticClass(), FTransform::Identity);
}

void AStreetGameMode::RestartPlayer(AController* NewPlayer)
{
	// The street is built for play first (the player is placed before anything begins play), so its starts are registered.
	FindOrSpawnStage();
	if (Stage)
	{
		Stage->EnsureBuilt();
	}
	// The shops too: their starts ("barber", "pawn"...) are theirs to register.
	SpawnShops();
	const FTransform Start = StartTransform(StartFrom);
	UE_LOG(LogStreet, Log, TEXT("Out onto Fifth at %s"), *StartFrom);
	RestartPlayerAtTransform(NewPlayer, Start);
	DressHero();
}

UStreetWorld* AStreetGameMode::StreetWorld() const
{
	return UStreetWorld::Get(this);
}

FTransform AStreetGameMode::StartTransform(const FString& Id) const
{
	FTransform T;
	if (const UStreetWorld* Registry = StreetWorld(); Registry && Registry->Start(FName(*Id), T))
	{
		return T;
	}
	return Stage ? Stage->StartAt(Id) : FTransform(FRotator(0.0f, 65.0f, 0.0f), FVector(150.0, 80.0, 100.0));
}

const FStreetUse* AStreetGameMode::UseInReach() const
{
	const AShortStackCharacter* Hero = GetHero();
	const UStreetWorld* Registry = StreetWorld();
	return Hero && Registry ? Registry->Best(Hero->EyeLocation(), Hero->LookDirection()) : nullptr;
}

const FStreetZone* AStreetGameMode::ZoneAt(const FVector& At) const
{
	const UStreetWorld* Registry = StreetWorld();
	return Registry ? Registry->ZoneAt(At) : nullptr;
}

AShopStage* AStreetGameMode::ShopFor(FName Venue) const
{
	for (const TWeakObjectPtr<AShopStage>& S : Shops)
	{
		if (S.IsValid() && S->Venue == Venue)
		{
			return S.Get();
		}
	}
	return nullptr;
}

void AStreetGameMode::UpdateShops(const FVector& Ear, float Dt)
{
	// Found again every second (the street's and Market's stages spawn them; one may go).
	if (RealTime - ShopsFoundAt > 1.0)
	{
		ShopsFoundAt = RealTime;
		Shops.Reset();
		for (TActorIterator<AShopStage> It(GetWorld()); It; ++It)
		{
			Shops.Add(*It);
		}
	}
	AShortStackCharacter* Hero = GetHero();
	const double World = Game->Session.WorldMinutes();
	const float Light = static_cast<float>(Game->Session.Daylight());
	const bool bMember = Game->Session.Owns("gym");
	// The closed lines once a second (core's hours: "Fifth St. Barbers · Closed · Opens Tue 10 AM").
	const bool bLines = RealTime - HoursAt > 1.0;
	if (bLines)
	{
		HoursAt = RealTime;
	}
	for (const TWeakObjectPtr<AShopStage>& S : Shops)
	{
		AShopStage* Shop = S.Get();
		if (!Shop)
		{
			continue;
		}
		// The hour and whether it's open and staffed, from core's shop hours (spec 5.0). The gym is open to its members at any
		// hour (its lights on all night: the 2 AM runner in the window); its fob door locks out everyone else when Rosa's away.
		// A venue core doesn't know yet stays open and staffed (the flows work before the catalog lands).
		const ss::shop::Venue* V = ss::shop::FindVenue(FStringToUtf8(Shop->Venue.ToString()));
		const bool bVenueOpen = V ? ss::shop::IsOpen(*V, World) : true;
		const bool bOpen = bVenueOpen || (V && V->MembersAnyHour);
		const bool bStaffed = V ? ss::shop::IsStaffed(*V, World) : true;
		FString& Line = ClosedLines.FindOrAdd(Shop->Venue);
		if (bLines && V)
		{
			Line = bVenueOpen ? FString() : FString::Printf(TEXT("%s · Closed · %s"), *Utf8ToFString(V->Name), *Utf8ToFString(ss::shop::HoursLine(*V, World)));
		}
		Shop->SetMember(bMember);
		Shop->SetHour(World, Light, bOpen, bStaffed, Line);
		if (!Hero)
		{
			continue;
		}
		// Its door: its bell as it swings open, and again, softer, as its closer brings it shut; the pawn's lock clacks as the
		// buzzer lets it go, and the gym's reader beeps.
		const bool bWasBuzzing = Shop->IsBuzzing();
		const int32 Door = Shop->UpdateDoor(Hero->GetActorLocation(), Dt);
		const float Heard = HeardFrom(Ear, Shop->DoorMiddle());
		if (Door != 0 && StreetTone && Heard > 0.01f)
		{
			if (Shop->Venue == TEXT("pawn") || Shop->Venue == TEXT("gym"))
			{
				if (Door == 1 && Audio)
				{
					Audio->PlayEffect(ss::audio::Effect::Chime, (Shop->Venue == TEXT("gym") ? 0.12f : 0.2f) * Heard);
				}
			}
			else
			{
				StreetTone->RingBell((Door == 1 ? 1.0f : 0.4f) * Heard);
			}
		}
		if (bWasBuzzing && !Shop->IsBuzzing() && StreetTone && Heard > 0.01f)
		{
			StreetTone->TryHandle(); // the strike lets go with a clack
		}
	}
}

void AStreetGameMode::SpawnShops()
{
	UWorld* World = GetWorld();
	if (!World || !Stage)
	{
		return;
	}
	// Each enterable shop at the street's frame: found if the street's or Market's stage spawned it already, else spawned.
	for (const FName& Venue : AShopStage::Venues())
	{
		// Ruby's Market face is the widened Market's north building line: on The Block's street (ss.Street.WideMarket 0) the
		// old block still stands across it, so it waits for the wide street, as the street's own stage has it.
		if (Venue == TEXT("diner") && !Stage->IsWideMarket())
		{
			continue;
		}
		if (!ShopFor(Venue))
		{
			if (AShopStage* Shop = AShopStage::FindOrSpawnVenue(World, Venue, Stage->GetActorTransform()))
			{
				Shops.AddUnique(Shop);
			}
		}
	}
}

void AStreetGameMode::UpdateCameraProfile(float Dt)
{
	AShortStackCharacter* Hero = GetHero();
	if (!Hero || !Game)
	{
		return;
	}
	// Across a door into a zone with a camera profile (a shop's shorter boom and wider view, or first person) and back out:
	// from wherever the camera is to the zone's (or back to the player's own boom and view), over 0.3 s.
	const FStreetZone* Zone = ZoneAt(Hero->GetActorLocation());
	const FStreetZone* Profile = Zone && Zone->HasCameraProfile() ? Zone : nullptr;
	const FName Want = Profile ? Profile->Id : NAME_None;
	if (Want != ProfileZone)
	{
		if (ProfileZone.IsNone() && (ProfileBlend >= 1.0f || StreetArm <= 0.0f))
		{
			// The player's own boom out on the street, put back on the way out. Not while the camera is still easing back
			// out to it (in and out of a door within 0.3 s): that would keep a boom halfway between as the player's.
			StreetArm = Hero->ArmLength;
		}
		ProfileZone = Want;
		ProfileBlend = 0.0f;
		BlendFromArm = Hero->ArmLength;
		BlendFromFov = Hero->ThirdPersonFov;
		if (Profile && Profile->bFirstPerson && !Hero->IsFirstPerson())
		{
			Hero->SetFirstPerson(true);
			bProfileFirstPerson = true;
		}
		else if ((!Profile || !Profile->bFirstPerson) && bProfileFirstPerson)
		{
			if (Hero->IsFirstPerson())
			{
				Hero->SetFirstPerson(false);
			}
			bProfileFirstPerson = false;
		}
	}
	if (ProfileBlend >= 1.0f)
	{
		return;
	}
	ProfileBlend = FMath::Min(1.0f, ProfileBlend + Dt / ProfileBlendSeconds);
	const float A = FMath::SmoothStep(0.0f, 1.0f, ProfileBlend);
	const int32 FovSetting = Game->Menu.Settings.FieldOfView;
	const float Arm = Profile && Profile->BoomLength >= 0.0f ? Profile->BoomLength : (StreetArm > 0.0f ? StreetArm : BlendFromArm);
	const float Fov = Profile && Profile->Fov > 0.0f ? StreetFov(Profile->Fov, FovSetting) : StreetFov(72.0f, FovSetting);
	Hero->ArmLength = FMath::Lerp(BlendFromArm, Arm, A);
	Hero->ThirdPersonFov = FMath::Lerp(BlendFromFov, Fov, A);
}

AShortStackCharacter* AStreetGameMode::GetHero() const
{
	APlayerController* Pc = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	return Pc ? Cast<AShortStackCharacter>(Pc->GetPawn()) : nullptr;
}

void AStreetGameMode::DressHero()
{
	AShortStackCharacter* Hero = GetHero();
	if (!Hero || !Game || bDressed)
	{
		return;
	}
	bDressed = true;
	const ss::wardrobe::Closet& Closet = Game->Session.Life.Closet;
	Hero->ApplyLook(Game->Session.Person, Closet.Seeded || !Closet.Owned.empty() ? &Closet : nullptr);
}

AShortStackCharacter* AStreetGameMode::SpawnNpc(const FTransform& Spot, double FloorZ, const TCHAR* CastName, const FLinearColor& Shirt)
{
	// Dressed before they begin play, so they aren't built as the default hero first and then thrown away.
	AShortStackCharacter* Npc =
		GetWorld()->SpawnActorDeferred<AShortStackCharacter>(AShortStackCharacter::StaticClass(), Spot, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Npc)
	{
		return nullptr;
	}
	Npc->AutoPossessAI = EAutoPossessAI::Disabled;
	Npc->ApplyCast(CastName, Shirt);
	Npc->FinishSpawning(Spot);
	// Their feet on the floor (nothing drops them there: with no controller their movement never runs).
	FVector At = Npc->GetActorLocation();
	At.Z = FloorZ + Npc->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	Npc->SetActorLocation(At, false, nullptr, ETeleportType::TeleportPhysics);
	UE_LOG(LogStreet, Log, TEXT("%s at work: body %s, face %s, %.0f cm"), CastName, *GetNameSafe(Npc->GetMesh()->GetSkeletalMeshAsset()),
		*GetNameSafe(Npc->Face ? Npc->Face->GetSkeletalMeshAsset() : nullptr), Npc->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2.0f);
	// They stand behind their counters: no movement to simulate, no camera of their own; their pose only while they're seen
	// (the character does as much for a cast member itself; this keeps them so whichever way they were dressed).
	if (UCharacterMovementComponent* Move = Npc->GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->SetComponentTickEnabled(false);
	}
	if (Npc->Boom)
	{
		Npc->Boom->bDoCollisionTest = false;
		Npc->Boom->SetComponentTickEnabled(false);
	}
	if (Npc->Camera)
	{
		Npc->Camera->Deactivate();
	}
	Npc->GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	if (Npc->Face)
	{
		Npc->Face->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	}
	return Npc;
}

void AStreetGameMode::SpawnClerk()
{
	if (!Stage || Clerk || !GetWorld())
	{
		return;
	}
	// Benny, nights, in the store's red polo.
	Clerk = SpawnNpc(Stage->ClerkSpot(), Stage->StoreFloorZ(), StreetCast::Clerk, FLinearColor::FromSRGBColor(FColor(0xd7, 0x26, 0x3d)));
}

void AStreetGameMode::SpawnAttendant()
{
	if (!Stage || Attendant || !GetWorld())
	{
		return;
	}
	// Inez, the Wash & Fold's attendant (it's open all night), in its smock, folding at her counter.
	Attendant = SpawnNpc(Stage->AttendantSpot(), Stage->WashFloorZ(), StreetCast::Attendant, FLinearColor::FromSRGBColor(AttendantSmock));
}

void AStreetGameMode::SpawnCrowd()
{
#if SHORTSTACK_STREET_CROWD
	AShortStackCharacter* Hero = GetHero();
	if (bCrowdStarted || !Stage || !Hero || !GetWorld())
	{
		return;
	}
	bCrowdStarted = true;
	if (AStreetCrowd* Walkers = GetWorld()->SpawnActor<AStreetCrowd>())
	{
		Walkers->Begin(Stage, Hero);
		Crowd = Walkers;
		if (People)
		{
			People->AttachCrowd(Walkers);
		}
	}
#endif
}

void AStreetGameMode::SpawnPeople()
{
	AShortStackCharacter* Hero = GetHero();
	if (People || !Stage || !Hero || !GetWorld() || !Game)
	{
		return;
	}
	// One director on the map (the crowd starts one itself when nobody has by its first hour: found, not doubled).
	People = AStreetNpcs::FindOrSpawn(GetWorld());
	if (!People)
	{
		return;
	}
	People->bAutoSpawned = false;
	AStreetCrowd* Walkers = nullptr;
#if SHORTSTACK_STREET_CROWD
	Walkers = Cast<AStreetCrowd>(Crowd);
#endif
	People->Begin(Stage, Hero, Walkers);
	People->SetSession(Game->bCareer ? &Game->Session : nullptr);
	People->SetHour(Game->Session.WorldMinutes(), static_cast<float>(Game->Session.Daylight()), Stage->RainStrength(), Game->Menu.IsPaused());
}

void AStreetGameMode::BeginPlay()
{
	Super::BeginPlay();
	FindOrSpawnStage();
	if (Stage)
	{
		Stage->EnsureBuilt();
	}
	SpawnShops();
	if (Stage && Game)
	{
		// The weather, the windows' hash and the parked cars follow the career's seed (spec 0.7, 3.3).
		Stage->SetWorldSeed(Game->Session.WorldSeed());
	}
	if (Game && Game->bCareer)
	{
		// The console's ss.Closet.* and ss.Fit.Set act on this career while the street plays.
		CastLooks::BindSession(this, &Game->Session);
	}
	DressHero();
	SpawnClerk();
	SpawnAttendant();
	SpawnCrowd();
	SpawnPeople();
	CreateWidgets();
	ReturnInputToGame();
	// The player's settings, as at the desk (the stage's lens and the hero's field of view are here now).
	ApplySettings(Game->Menu.Settings, false);
	if (Audio)
	{
		// The street's own sound, not the apartment's room tone (Tick keeps it told where the camera is, the rain, the hour).
		StreetTone = MakeShared<FStreetAmbience>(static_cast<uint32>(FDateTime::Now().GetTicks()));
		TSharedPtr<FStreetAmbience> Tone = StreetTone;
		Audio->StartAmbience([Tone](float* Out, int32 N) { Tone->Render(Out, N); }, 0.75f);
		Audio->PlayEffect(ss::audio::Effect::Thump, 0.5f); // the building's door closing behind you
	}
	if (APlayerController* Pc = GetWorld()->GetFirstPlayerController())
	{
		if (Pc->PlayerCameraManager)
		{
			Pc->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 1.8f, FLinearColor::Black, true, false);
		}
	}
}

void AStreetGameMode::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Game && !bLeaving)
	{
		Game->Session.Save();
	}
	// The save being written (and any waiting) lands before the next scene reads it.
	if (Game)
	{
		Game->Saver.Flush();
	}
	if (SettingsDirtyAt >= 0.0)
	{
		SaveSettingsNow();
	}
	// A skip's hold on the corner never outlives the street.
	if (UStreetWorld* Registry = StreetWorld())
	{
		Registry->SetHold(false, 0.0);
	}
	CastLooks::BindSession(this, nullptr);
	if (People)
	{
		People->SetSession(nullptr); // the career goes with this mode
	}
	Card = nullptr;
	Screen.Reset();
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		for (TSharedPtr<SWidget> W : {StaticCastSharedPtr<SWidget>(HudWidget), StaticCastSharedPtr<SWidget>(CounterWidget), StaticCastSharedPtr<SWidget>(ShopWidget),
				 StaticCastSharedPtr<SWidget>(MenuWidget)})
		{
			if (W)
			{
				Viewport->RemoveViewportWidgetContent(W.ToSharedRef());
			}
		}
	}
	Super::EndPlay(Reason);
}

// ------------------------------------------------------------------ widgets and input

void AStreetGameMode::CreateWidgets()
{
	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (!Viewport || !FSlateApplication::IsInitialized())
	{
		return;
	}
	HudWidget = SNew(SDrawListWidget);
	HudWidget->SetVisibility(EVisibility::HitTestInvisible);
	Viewport->AddViewportWidgetContent(HudWidget.ToSharedRef(), 10);

	TWeakObjectPtr<AStreetGameMode> WeakThis = this;
	auto Pointer = [WeakThis](ss::ui::Pointer& P, const FVector2D& Logical, int32 Button, float WheelDelta) {
		P.Active = true;
		P.X = static_cast<float>(Logical.X);
		P.Y = static_cast<float>(Logical.Y);
		if (Button == 1)
		{
			P.Down = true;
			P.Pressed = true;
		}
		else if (Button == 2)
		{
			P.Down = false;
			P.Released = true;
		}
		P.Wheel += WheelDelta;
	};
	CounterWidget = SNew(SFrontEndWidget);
	CounterWidget->SetVisibility(EVisibility::Collapsed);
	CounterWidget->OnMenuKey = [WeakThis](const FString& KeyName, bool bFromGamepad) {
		if (WeakThis.IsValid() && WeakThis->Game)
		{
			WeakThis->bGamepad = bFromGamepad;
			ss::ui::StoreCounter& Counter = WeakThis->Game->Counter;
			Counter.Gamepad = bFromGamepad;
			Counter.Key(FStringToUtf8(KeyName), WeakThis->RealTime);
			if (Counter.TakeLeave())
			{
				WeakThis->ReturnInputToGame();
			}
		}
	};
	CounterWidget->OnMenuPointer = [WeakThis, Pointer](const FVector2D& Logical, int32 Button, float WheelDelta) {
		if (WeakThis.IsValid() && WeakThis->Game)
		{
			Pointer(WeakThis->Game->Counter.Ptr, Logical, Button, WheelDelta);
		}
	};
	Viewport->AddViewportWidgetContent(CounterWidget.ToSharedRef(), 20);

	// The shops' screens (spec 7.5.3): one at a time, drawn and keyed as the counter is.
	ShopWidget = SNew(SFrontEndWidget);
	ShopWidget->SetVisibility(EVisibility::Collapsed);
	ShopWidget->OnMenuKey = [WeakThis](const FString& KeyName, bool bFromGamepad) {
		if (WeakThis.IsValid() && WeakThis->Screen && WeakThis->Screen->IsOpen())
		{
			WeakThis->bGamepad = bFromGamepad;
			WeakThis->Screen->Gamepad = bFromGamepad;
			// Its requests (a service begun, a seat) and whether it was walked away from are taken each frame, in that order
			// (UpdateShopScreen): closing it here would lose what the key asked for.
			WeakThis->Screen->Key(FStringToUtf8(KeyName), WeakThis->RealTime);
		}
	};
	ShopWidget->OnMenuPointer = [WeakThis, Pointer](const FVector2D& Logical, int32 Button, float WheelDelta) {
		if (WeakThis.IsValid() && WeakThis->Screen)
		{
			Pointer(WeakThis->Screen->Ptr, Logical, Button, WheelDelta);
		}
	};
	Viewport->AddViewportWidgetContent(ShopWidget.ToSharedRef(), 21);

	MenuWidget = SNew(SFrontEndWidget);
	MenuWidget->SetVisibility(EVisibility::Collapsed);
	MenuWidget->OnMenuKey = [WeakThis](const FString& KeyName, bool bFromGamepad) {
		if (WeakThis.IsValid() && WeakThis->Game)
		{
			WeakThis->bGamepad = bFromGamepad;
			WeakThis->Game->Menu.Gamepad = bFromGamepad;
			WeakThis->Game->Menu.Key(FStringToUtf8(KeyName), WeakThis->RealTime);
		}
	};
	MenuWidget->OnMenuChar = [WeakThis](uint32 Codepoint) {
		if (WeakThis.IsValid() && WeakThis->Game)
		{
			WeakThis->Game->Menu.Char(Codepoint, WeakThis->RealTime);
		}
	};
	MenuWidget->OnMenuPointer = [WeakThis, Pointer](const FVector2D& Logical, int32 Button, float WheelDelta) {
		if (WeakThis.IsValid() && WeakThis->Game)
		{
			Pointer(WeakThis->Game->Menu.Ptr, Logical, Button, WheelDelta);
		}
	};
	Viewport->AddViewportWidgetContent(MenuWidget.ToSharedRef(), 30);
}

void AStreetGameMode::FocusWidget(SFrontEndWidget* Widget)
{
	if (!Widget)
	{
		return;
	}
	Widget->SetVisibility(EVisibility::Visible);
	if (APlayerController* Pc = UGameplayStatics::GetPlayerController(this, 0))
	{
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(Widget->AsShared());
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Pc->SetInputMode(Mode);
		Pc->bShowMouseCursor = true;
	}
	FSlateApplication::Get().SetKeyboardFocus(Widget->AsShared());
}

void AStreetGameMode::ReturnInputToGame()
{
	if (CounterWidget && !Game->Counter.IsOpen())
	{
		CounterWidget->SetVisibility(EVisibility::Collapsed);
	}
	if (ShopWidget && !(Screen && Screen->IsOpen()))
	{
		ShopWidget->SetVisibility(EVisibility::Collapsed);
	}
	if (MenuWidget && !Game->Menu.IsOpen())
	{
		MenuWidget->SetVisibility(EVisibility::Collapsed);
	}
	if (APlayerController* Pc = UGameplayStatics::GetPlayerController(this, 0))
	{
		Pc->SetInputMode(FInputModeGameOnly());
		Pc->bShowMouseCursor = false;
	}
}

bool AStreetGameMode::IsUiOpen() const
{
	// A shop's screen, a service playing out (movement blocked, spec 7.5.3) or a moment under black hold the input too.
	return Game && (Game->Counter.IsOpen() || Game->Menu.IsOpen() || (Screen && Screen->IsOpen()) || Cinema.bActive || BlackAt >= 0.0);
}

FVector2D AStreetGameMode::ViewSize() const
{
	FVector2D Size(1920.0, 1080.0);
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		FVector2D Measured;
		Viewport->GetViewportSize(Measured);
		if (Measured.X > 0.0 && Measured.Y > 0.0)
		{
			Size = Measured;
		}
	}
	return Size;
}

void AStreetGameMode::DrawOverlay(SFrontEndWidget* Widget, TFunctionRef<void(ss::ui::Canvas&)> Paint)
{
	if (!Widget)
	{
		return;
	}
	const FVector2D Size = ViewSize();
	const float H = ss::ui::StoreCounter::Height;
	TSharedPtr<ss::ui::DrawList> List = MakeShared<ss::ui::DrawList>();
	ss::ui::Canvas Cv(*List, Game->Measurer, H * static_cast<float>(Size.X / Size.Y), H, static_cast<float>(Size.Y) / H);
	Paint(Cv);
	Widget->SetDrawList(List);
}

void AStreetGameMode::DrawHud()
{
	AShortStackCharacter* Hero = GetHero();
	if (!HudWidget || !Hero)
	{
		return;
	}
	ss::ui::StreetHudInfo Info;
	Info.Place = Stage ? FStringToUtf8(Stage->PlaceName(Hero->GetActorLocation())) : "FIFTH STREET";
	Info.Clock = ss::net::TimeLabel(Game->Session.WorldMinutes());
	// With the clock the HUD counts down to a Penny Drop order on its way (the player knows when to head home).
	Info.World = Game->Session.WorldMinutes();
	Info.BankrollCents = Game->Session.BankrollCents;
	Info.Life = &Game->Session.Life;
	Info.FirstPerson = Hero->IsFirstPerson();
	Info.HintsAt = Game->Menu.Settings.ShowHints ? HintsAt : -100.0;
	Info.Gamepad = bGamepad;
	for (const ss::ui::StreetHudInfo::Toast& T : Toasts)
	{
		Info.Toasts.push_back(T);
	}
	// The player's caption settings (next_segment_spec 4.9): off, the size, the box, the names (StreetHud draws two at most).
	const ss::ui::GameSettings& Cs = Game->Menu.Settings;
	Info.CaptionLook.Mode = FMath::Clamp(Cs.Captions, 0, 2);
	Info.CaptionLook.Size = FMath::Clamp(Cs.CaptionSize, 80, 200);
	Info.CaptionLook.Backdrop = FMath::Clamp(Cs.CaptionBackdrop, 0, 100);
	Info.CaptionLook.Names = Cs.SpeakerNames;
	// A sponsor's appearance the player owes in the next day (spec 3.1): the chip under the clock, the first window not
	// yet made (one going on now counts).
	for (const ss::sponsor::Window& Wd : ss::sponsor::Appearances(Game->Session.Life.Sponsors, Info.World, Info.World + ss::net::MinutesPerDay))
	{
		if (!Wd.Done)
		{
			Info.Due.Label = Wd.Label;
			Info.Due.From = Wd.From;
			Info.Due.To = Wd.To;
			break;
		}
	}
	const FVector2D Size = ViewSize();
	const float H = ss::ui::StoreCounter::Height;
	const FStreetUse* Use = !IsUiOpen() && !bLeaving && Stage ? UseInReach() : nullptr;
	if (Use)
	{
		Info.Prompt = FStringToUtf8(PromptFor(*Use));
	}
	else if (!IsUiOpen() && !bLeaving && !SeatedAt.IsNone())
	{
		Info.Prompt = "Stand up";
	}
	// The name over whoever the hero is looking at (a Talk use on a person): their name, and their bond or their job.
	if (Use && Use->Verb == EStreetVerb::Talk && Use->Anchor.IsValid() && Use->Anchor->GetOwner() && Use->Anchor->GetOwner()->IsA<APawn>())
	{
		const FName Id = Use->Param.IsEmpty() ? Use->Id : FName(*Use->Param);
		const FStreetPerson* P = People ? People->FindPerson(Id) : nullptr;
		const ss::town::Person* Known = ss::town::FindPerson(FStringToUtf8(Id.ToString()));
		const FString Name = P && !P->Name.IsEmpty() ? P->Name : Known && !Known->Name.empty() ? Utf8ToFString(Known->Name) : FString();
		FVector2D OnScreen;
		APlayerController* Pc = GetWorld()->GetFirstPlayerController();
		if (!Name.IsEmpty() && Pc && Pc->ProjectWorldLocationToScreen(UStreetWorld::UseLocation(*Use) + FVector(0.0, 0.0, 28.0), OnScreen, true))
		{
			const float Scale = static_cast<float>(Size.Y) / H;
			const std::string Tag = ss::town::Tag(FStringToUtf8(Id.ToString()), Game->Session);
			Info.NameTag.Text = FStringToUtf8(Name);
			Info.NameTag.Sub = !Tag.empty() ? Tag : P ? FStringToUtf8(P->Label) : std::string();
			Info.NameTag.X = static_cast<float>(OnScreen.X) / Scale;
			Info.NameTag.Y = static_cast<float>(OnScreen.Y) / Scale;
		}
	}
	// A service or a wait playing out: its skip's label, how far along, and the hours it runs between (spec 5.0).
	if (Cinema.bActive && Cinema.Phase == 0)
	{
		const ss::Session::Skip& K = Game->Session.TimeSkip;
		const FString Where = ShopFor(Cinema.Venue) ? ShopFor(Cinema.Venue)->VenueName : Cinema.Venue.ToString();
		Info.Skip.Active = true;
		Info.Skip.Label = K.Active && !K.Label.empty() ? K.Label : FStringToUtf8(FString::Printf(TEXT("At %s"), *Where));
		Info.Skip.Progress = static_cast<float>(FMath::Clamp((RealTime - Cinema.StartedAt) / FMath::Max(0.1, Cinema.Length), 0.0, 1.0));
		Info.Skip.From = K.Active ? K.From : Game->Session.WorldMinutes();
		Info.Skip.To = K.Active ? K.To : Game->Session.WorldMinutes();
	}
	// A locked door's line; what people said near the hero (not while the talk card says it); the weather.
	Info.Closed = FStringToUtf8(ClosedNow);
	Info.ClosedAt = ClosedNowAt;
	if (!Card)
	{
		for (const ss::ui::StreetHudInfo::Subtitle& Sub : Subtitles)
		{
			Info.Subtitles.push_back(Sub);
		}
	}
	if (Stage)
	{
		Info.Weather = static_cast<int>(Stage->WeatherNow().State);
		Info.Rain = Stage->RainStrength();
	}
	TSharedPtr<ss::ui::DrawList> List = MakeShared<ss::ui::DrawList>();
	ss::ui::Canvas Cv(*List, Game->Measurer, H * static_cast<float>(Size.X / Size.Y), H, static_cast<float>(Size.Y) / H);
	// Under the counter and the shops' full-screen pages it's hidden; the talk card sits over the street with the HUD.
	if (!Game->Counter.IsOpen() && (!(Screen && Screen->IsOpen()) || Card))
	{
		ss::ui::DrawStreetHud(Cv, Info, RealTime);
	}
	HudWidget->SetDrawList(List);
}

void AStreetGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Game)
	{
		return;
	}
	const double Dt = FMath::Clamp(static_cast<double>(DeltaSeconds), 0.0, 0.05);
	RealTime += Dt;
	if (!Game->bCareer)
	{
		// Nobody to walk out as: the title screen makes (or continues) a career.
		if (!bBackToTitle)
		{
			bBackToTitle = true;
			UE_LOG(LogStreet, Warning, TEXT("No career saved: back to the title"));
			UGameplayStatics::OpenLevel(this, FName(TEXT("NightOne")), true);
		}
		return;
	}
	if (!Game->Menu.IsPaused())
	{
		Game->Session.Update(RealTime);
	}
	Game->Saver.Tick();
	FrameBudget.Tick(DeltaSeconds);
	if (SettingsDirtyAt >= 0.0 && RealTime - SettingsDirtyAt > 0.75)
	{
		SaveSettingsNow();
	}
	if (Stage)
	{
		// The street's hour: the sky, the lamps' photocells, the windows and the exposure follow the clock (and the store's
		// clock on the wall shows it).
		Stage->SetDaylight(static_cast<float>(Game->Session.Daylight()));
		Stage->SetClock(Game->Session.WorldMinutes());
	}
	AShortStackCharacter* Hero = GetHero();
	// Where the street is heard from: the camera (the hero's eyes, or the door, before there's one).
	const APlayerController* Viewer = GetWorld()->GetFirstPlayerController();
	FVector Ear = Hero ? Hero->EyeLocation() : Stage ? Stage->StartAt(TEXT("home")).GetLocation() : FVector::ZeroVector;
	if (Viewer && Viewer->PlayerCameraManager)
	{
		Ear = Viewer->PlayerCameraManager->GetCameraLocation();
	}
	if (Stage && StreetTone)
	{
		// The street's sound from where the camera is: in or out of the store or the laundromat (their doors open or shut),
		// near the laundromat's front or under a sign or not; the rain as it shows; the traffic by the hour; nothing new
		// starting on the street while paused.
		StreetTone->SetInside(Stage->CameraInside(), Stage->DoorsOpen());
		// The shops' room tones by the zone the camera is in (spec 3.7), its door's opening letting the street in.
		const FStreetZone* Room = ZoneAt(Ear);
		const bool bShop = Room && Room->RoomTone >= StreetRoomTone::Barber;
		StreetTone->SetRoom(bShop ? Room->RoomTone : StreetRoomTone::Street, bShop && Room->DoorOpen ? Room->DoorOpen() : 0.0f);
		StreetTone->SetWash(Stage->CameraInWash(), Stage->WashDoorOpen(), Stage->WashNear(Ear));
		StreetTone->SetRain(Stage->RainStrength(), Stage->Wetness);
		StreetTone->SetDaylight(static_cast<float>(Game->Session.Daylight()));
		StreetTone->SetHum(Stage->SignHum(Ear));
		StreetTone->SetPaused(Game->Menu.IsPaused());
	}
	if (Hero && Stage)
	{
		KeepOnTheSet();
		// The store's doors open for the player (and for the pedestrians, who tell the stage where they are: NoteWalker).
		if (Stage->UpdateDoors(Hero->GetActorLocation(), static_cast<float>(Dt)) && Audio)
		{
			// The doors' chime as they part (the store's two notes, kept small), as near as they are to the camera.
			const float Heard = DoorHeard(*Stage, Ear, TEXT("store"));
			if (Heard > 0.01f)
			{
				Audio->PlayEffect(ss::audio::Effect::Chime, 0.22f * Heard);
			}
		}
		// The laundromat's door: its bells as it swings open, and again, softer, as its closer brings it shut.
		const int32 WashDoor = Stage->UpdateWashDoor(Hero->GetActorLocation(), static_cast<float>(Dt));
		const float BellHeard = WashDoor != 0 && StreetTone ? DoorHeard(*Stage, Ear, TEXT("washfold")) : 0.0f;
		if (BellHeard > 0.01f)
		{
			StreetTone->RingBell((WashDoor == 1 ? 1.0f : 0.4f) * BellHeard);
		}
		float Volume = 0.0f;
		if (Hero->TakeFootstep(Volume) && Audio)
		{
			// Softer indoors (the store, the laundromat, the shops: the registry's rooms).
			const FStreetZone* Zone = ZoneAt(Hero->GetActorLocation());
			Audio->PlayEffect(ss::audio::Effect::Step, Volume * (Zone && Zone->bInterior ? 0.7f : 1.0f));
		}
		// Worn out shows in the walk: from 40 energy down the shoulders drop and the trunk sags.
		Hero->SetTired(static_cast<float>((40.0 - Game->Session.Life.Energy) / 40.0));
		// Inside a shop the camera takes the room's profile.
		UpdateCameraProfile(static_cast<float>(Dt));
	}
	else if (!Hero && !bLeaving && GetWorld())
	{
		// The pawn is gone (killed below the world's floor before KeepOnTheSet could catch it): a new one at the door.
		if (APlayerController* Pc = GetWorld()->GetFirstPlayerController(); Pc && !Pc->GetPawn())
		{
			UE_LOG(LogStreet, Warning, TEXT("The hero was lost: back at the door"));
			// Whatever the old one was in (a screen, a seat, a shot's mark, a preview) goes with it.
			if (Screen)
			{
				CloseShop(false);
			}
			SeatedAt = NAME_None;
			SeatedVenue = NAME_None;
			bMarkMoved = false;
			bPreviewing = false;
			bDressed = false;
			StartFrom = HomeDoor();
			ProfileZone = NAME_None;
			ProfileBlend = 1.0f;
			bProfileFirstPerson = false;
			RestartPlayer(Pc);
			ApplySettings(Game->Menu.Settings, false);
		}
	}
	// The enterable shops: the hour, their doors; their screens, a service's cinematic, the moments under black, closing
	// time, and now and then the session's state on their shelves and boards.
	UpdateShops(Ear, static_cast<float>(Dt));
	UpdateShopScreen(static_cast<float>(Dt));
	// A skip a page began without a service's request (Ruby's "sit a while", the pawn's trip to storage): the same hold and
	// overlay, the camera as it is.
	if (Game->Session.TimeSkip.Active && !Cinema.bActive && !bLeaving)
	{
		BeginCinematic(!ScreenVenue.IsNone() ? ScreenVenue : SeatedVenue, FString());
	}
	UpdateCinematic(static_cast<float>(Dt));
	UpdateBlack(static_cast<float>(Dt));
	UpdateClosing();
	if (RealTime - ShopsSyncedAt > ShopSyncEvery)
	{
		SyncShops();
	}
	// A skip's outcome (a service, a wait) as a toast: the laptop's result card is the apartment's.
	if (Game->Session.HasOutcome && !(Cinema.bActive && Cinema.Phase == 0))
	{
		Game->Session.HasOutcome = false;
		const ss::life::Outcome& O = Game->Session.LastOutcome;
		if (!O.Title.empty() || !O.Body.empty())
		{
			Toast(O.Title.empty() ? FString(TEXT("Done")) : Utf8ToFString(O.Title), Utf8ToFString(O.Body));
		}
	}
	// The block's people: begun once the hero stands, told the hour, the rain and the pause each frame; what they say
	// near the hero to the HUD's subtitles (the talk card says its own).
	if (!People)
	{
		SpawnPeople();
	}
	if (People && Stage)
	{
		People->SetHour(Game->Session.WorldMinutes(), static_cast<float>(Game->Session.Daylight()), Stage->RainStrength(), Game->Menu.IsPaused());
		FString Speaker;
		FString Line;
		while (People->TakeSubtitle(Speaker, Line))
		{
			if (Card)
			{
				continue;
			}
			ss::ui::StreetHudInfo::Subtitle Sub;
			Sub.Speaker = FStringToUtf8(Speaker);
			Sub.Line = FStringToUtf8(Line);
			Sub.At = RealTime;
			Subtitles.Add(MoveTemp(Sub));
		}
	}
	Subtitles.RemoveAll([this](const ss::ui::StreetHudInfo::Subtitle& Sub) { return RealTime - Sub.At > 10.0; });
	while (Subtitles.Num() > 4)
	{
		Subtitles.RemoveAt(0);
	}
#if SHORTSTACK_STREET_CROWD
	// The pedestrians: started once the hero stands (then told the hour, the rain, and whether the game's paused).
	if (!bCrowdStarted)
	{
		SpawnCrowd();
	}
	if (AStreetCrowd* Walkers = Cast<AStreetCrowd>(Crowd); Walkers && Stage)
	{
		Walkers->SetHour(static_cast<float>(Game->Session.Daylight()), Game->Session.WorldMinutes(), Stage->RainStrength(), Game->Menu.IsPaused());
		// A word from someone the player nearly walked into, heard if they're still close: a subtitle with no name.
		FVector BarkAt;
		FString Bark;
		while (Walkers->TakeBark(BarkAt, Bark))
		{
			if (Hero && !Card && FVector::DistSquared(BarkAt, Hero->GetActorLocation()) < 700.0 * 700.0)
			{
				ss::ui::StreetHudInfo::Subtitle Sub;
				Sub.Line = FStringToUtf8(Bark);
				Sub.At = RealTime;
				Subtitles.Add(MoveTemp(Sub));
			}
		}
	}
#endif
	// Out of Dee's game: her word on the night, once the player has their feet on the sidewalk (the calendar caught up).
	if (NightWordAt >= 0.0 && RealTime >= NightWordAt)
	{
		NightWordAt = -1.0;
		Game->Text("Dee", FStringToUtf8(NightWordFrom(NightOptions, Game->Session.Evicted())));
	}
	DrawHud();
	WatchCounterSale();
	if (Game->Counter.IsOpen())
	{
		DrawOverlay(CounterWidget.Get(), [this](ss::ui::Canvas& Cv) { Game->Counter.Draw(Cv, RealTime); });
	}
	if (Game->Menu.IsOpen() || (MenuWidget && MenuWidget->GetVisibility() == EVisibility::Visible))
	{
		DrawOverlay(MenuWidget.Get(), [this](ss::ui::Canvas& Cv) { Game->Menu.Draw(Cv, RealTime); });
		if (!Game->Menu.IsOpen() && Game->Menu.Backdrop(RealTime) <= 0.001f)
		{
			ReturnInputToGame();
		}
	}
	if (bLeaving && RealTime >= LeaveAt)
	{
		bLeaving = false;
		// The clock to the thousandth of a minute when it's carried home: rounded, a deadline at the stroke of midnight could
		// be skipped.
		const FString Options = bLeaveWithClock ? FString::Printf(TEXT("%s?From=%.3f"), *LeaveOptions, Game->Session.WorldMinutes()) : LeaveOptions;
		UE_LOG(LogStreet, Log, TEXT("Off the street to %s?%s"), *LeaveLevel.ToString(), *Options);
		UGameplayStatics::OpenLevel(this, LeaveLevel, true, Options);
	}
	// Texts don't age while the counter or a shop has the screen (the HUD that shows them is hidden then); otherwise ten seconds.
	if (Game->Counter.IsOpen() || (Screen && Screen->IsOpen()))
	{
		for (ss::ui::StreetHudInfo::Toast& T : Toasts)
		{
			T.At += Dt;
		}
	}
	Toasts.RemoveAll([this](const ss::ui::StreetHudInfo::Toast& T) { return RealTime - T.At > 10.0; });
}

void AStreetGameMode::KeepOnTheSet()
{
	AShortStackCharacter* Hero = GetHero();
	if (!Hero || !Stage || bLeaving || !Stage->IsOffTheSet(Hero->GetActorLocation()))
	{
		return;
	}
	UE_LOG(LogStreet, Warning, TEXT("Off the set at %s: back to the door"), *Hero->GetActorLocation().ToCompactString());
	const FTransform T = StartTransform(HomeDoor());
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

FString AStreetGameMode::HomeDoor() const
{
	return Game && Game->Session.Evicted() ? TEXT("dee") : TEXT("home");
}

EDeeGame AStreetGameMode::DeeGameAt(double World)
{
	const ss::life::Activity* G = ss::life::Find("dee-game");
	if (!G)
	{
		return EDeeGame::Off;
	}
	// Her doors are open as long as the session lets anyone sit (Life's window, nine to three, on her nights).
	if (ss::life::InWindow(*G, World))
	{
		return EDeeGame::Open;
	}
	// A night's game belongs to the day its doors opened (Day 0 a Monday, as Life counts them): before nine, last night's
	// may still be dealing its last hands (until five), or tonight's is still to come.
	const double Tod = World - FMath::FloorToDouble(World / ss::net::MinutesPerDay) * ss::net::MinutesPerDay;
	const int32 Day = FMath::FloorToInt32(World / ss::net::MinutesPerDay);
	auto IsGameNight = [G](int32 D) { return ((G->Days >> ss::net::Weekday(D)) & 1) != 0; };
	if (Tod < static_cast<double>(G->Opens))
	{
		if (Tod < DeeLastHand && IsGameNight(Day - 1))
		{
			return EDeeGame::LastHands;
		}
		if (Tod >= DeeLastHand && IsGameNight(Day))
		{
			return EDeeGame::Tonight;
		}
	}
	return EDeeGame::Off;
}

FString AStreetGameMode::PromptFor(const FStreetUse& Use) const
{
	const bool bEvicted = Game && Game->Session.Evicted();
	if (Use.Id == TEXT("home"))
	{
		return bEvicted ? TEXT("Try your key") : TEXT("Go home");
	}
	if (Use.Id == TEXT("dee"))
	{
		return bEvicted ? TEXT("Up to Dee's") : TEXT("Ring Dee's buzzer");
	}
	if (Use.Id == TEXT("washdoor"))
	{
		// Open, there's nothing to do but walk through it.
		return Stage && Stage->WashDoorOpen() > 0.5f ? FString() : Use.Prompt;
	}
	if (Use.Id == TEXT("backdoor") && Game && DeeGameAt(Game->Session.WorldMinutes()) == EDeeGame::Open)
	{
		return FString::Printf(TEXT("Sit in Dee's game ($%lld)"), static_cast<long long>(ss::life::GameMinBuyInCents / 100));
	}
	if (!SeatedAt.IsNone())
	{
		// Sitting (the barber's bench, a stool with nothing open): E gets up.
		return TEXT("Stand up");
	}
	if (Use.Verb == EStreetVerb::Door)
	{
		// A shop's door: open, walk on through (as the Wash & Fold's); from inside, a door out whatever its lock; outside, what
		// it takes ("Tap your fob", "Buzz in", "Buzz") or its closed line.
		if (const AShopStage* Shop = ShopFor(Use.Venue))
		{
			return Shop->DoorPromptNow();
		}
	}
	const FString Why = UStreetWorld::PromptOf(Use);
	if (Why == Use.Prompt && Use.Id == TEXT("barber-chair1") && Game)
	{
		// Somebody's ahead in the chair (spec 5.1): the bench first.
		const int32 Queue = ss::town::BarberQueue(Game->Session.WorldMinutes());
		if (Queue > 0)
		{
			return FString::Printf(TEXT("Wait (about %d min)"), FMath::Max(5, (Queue + 2) / 5 * 5));
		}
	}
	return Why;
}

// ------------------------------------------------------------------ actions

void AStreetGameMode::OnMove(float Forward, float Right, bool bRun)
{
	if (AShortStackCharacter* Hero = GetHero(); Hero && !bLeaving)
	{
		Hero->Drive(Forward, Right, bRun && Game->Session.Life.Energy > 8.0);
	}
}

void AStreetGameMode::OnLook(float DeltaYaw, float DeltaPitch)
{
	if (AShortStackCharacter* Hero = GetHero())
	{
		Hero->Look(DeltaYaw, bInvertLook ? -DeltaPitch : DeltaPitch);
	}
}

void AStreetGameMode::OnToggleView()
{
	if (AShortStackCharacter* Hero = GetHero())
	{
		Hero->ToggleView();
		Game->Sound(ss::SoundId::Click, 0.3);
	}
}

void AStreetGameMode::OnZoom(float Delta)
{
	if (AShortStackCharacter* Hero = GetHero(); Hero && !Hero->IsFirstPerson())
	{
		// Inside a room with its own boom the room's holds (the player's own is put back on the way out).
		const FStreetZone* Zone = ZoneAt(Hero->GetActorLocation());
		if (Zone && Zone->BoomLength >= 0.0f)
		{
			return;
		}
		if (ProfileZone.IsNone() && ProfileBlend < 1.0f && StreetArm > 0.0f)
		{
			// Easing back out of a room: the wheel moves where it's easing to (the blend would undo a change to the boom itself).
			StreetArm = FMath::Clamp(StreetArm + Delta, 160.0f, 460.0f);
			return;
		}
		Hero->ArmLength = FMath::Clamp(Hero->ArmLength + Delta, 160.0f, 460.0f);
	}
}

void AStreetGameMode::OpenCounter(int32 Shelf)
{
	Game->Counter.Gamepad = bGamepad;
	Game->Counter.Open(RealTime, FMath::Max(0, Shelf));
	FocusWidget(CounterWidget.Get());
}

void AStreetGameMode::OnInteract()
{
	AShortStackCharacter* Hero = GetHero();
	if (!Hero || !Stage || bLeaving || IsUiOpen())
	{
		return;
	}
	// Sitting with nothing open (the bench, a seat walked away from): E stands up.
	if (!SeatedAt.IsNone())
	{
		StandUp();
		return;
	}
	// The registry's use in reach, by its verb (spec 7.5.3). Copied: what it does may change the registry.
	const FStreetUse* InReach = UseInReach();
	if (!InReach || PromptFor(*InReach).IsEmpty())
	{
		return;
	}
	const FStreetUse Use = *InReach;
	switch (Use.Verb)
	{
	case EStreetVerb::Home:
		// 1812's door (home, or the new lock while evicted), or Dee's (up to her front room while evicted, else her buzzer).
		if (Use.Id == TEXT("dee"))
		{
			TryDeeDoor();
		}
		else
		{
			TryHomeDoor();
		}
		break;
	case EStreetVerb::Counter:
		OpenCounter(Use.Shelf);
		break;
	case EStreetVerb::Door:
		UseDoor(Use);
		break;
	case EStreetVerb::Talk:
		TalkTo(Use);
		break;
	case EStreetVerb::Shop:
	{
		// A shop's screen on the use's page (a rack, the counter, a machine); shut, its closed line.
		if (Use.WhyNot && !Use.WhyNot().IsEmpty())
		{
			ShowClosed(UStreetWorld::PromptOf(Use));
			break;
		}
		OpenShop(Use.Venue, Use.Param.IsEmpty() ? Use.Id.ToString() : Use.Param);
		break;
	}
	case EStreetVerb::Sit:
		UseSeat(Use);
		break;
	case EStreetVerb::Wait:
		UseWait(Use);
		break;
	case EStreetVerb::Wardrobe:
		OpenShop(TEXT("wardrobe"), Use.Param.IsEmpty() ? Use.Id.ToString() : Use.Param);
		break;
	case EStreetVerb::Board:
	{
		// The 14 (spec 7.6, P1): the session says whether a registration lets the player on; the ride itself comes with it.
		const std::string Why = Game->Session.BoardBus(FStringToUtf8(Use.Param));
		if (!Why.empty())
		{
			Toast(TEXT("The 14"), Utf8ToFString(Why));
		}
		else
		{
			NotYet(Use);
		}
		break;
	}
	}
}

void AStreetGameMode::UseDoor(const FStreetUse& Use)
{
	if (Use.Id == TEXT("washdoor"))
	{
		// A push, and its bells ring as it swings in (UpdateWashDoor tells Tick).
		Stage->OpenWashDoor();
		return;
	}
	if (Use.Id == TEXT("backdoor"))
	{
		TryBackDoor();
		return;
	}
	AShopStage* Shop = ShopFor(Use.Venue);
	if (!Shop)
	{
		// A door use with no shop stage behind it (yet): said so in the log, once.
		NotYet(Use);
		return;
	}
	// Open: it swings in (or the keeper buzzes it: the buzzer, then the lock's clack as it lets go); the gym's opens to a
	// member's fob at any hour. Shut, the handle tried (E rattles it), and the line says when it opens again.
	const bool bWasBuzzing = Shop->IsBuzzing();
	const FString Why = Shop->OpenDoor(Game->Session.Owns("gym"));
	if (!Why.IsEmpty())
	{
		if (StreetTone)
		{
			StreetTone->TryHandle();
		}
		ShowClosed(Why);
		return;
	}
	if (Shop->IsBuzzing() && !bWasBuzzing && StreetTone)
	{
		StreetTone->PressBuzzer();
	}
}

void AStreetGameMode::TalkTo(const FStreetUse& Use)
{
	if (Use.Param == TEXT("attendant"))
	{
		TalkToAttendant();
		return;
	}
	// Someone with a word (a keeper, one of the world's people, Augie at his guitar wall): the talk card with them.
	if (!OpenTalk(Use.Param.IsEmpty() ? Use.Id : FName(*Use.Param), Use.Venue))
	{
		NotYet(Use);
	}
}

void AStreetGameMode::NotYet(const FStreetUse& Use)
{
	if (!NotYetSaid.Contains(Use.Id))
	{
		NotYetSaid.Add(Use.Id);
		UE_LOG(LogStreet, Warning, TEXT("E at %s (%s, %s): its screen isn't in yet"), *Use.Id.ToString(), *StaticEnum<EStreetVerb>()->GetNameStringByValue(static_cast<int64>(Use.Verb)),
			*Use.Venue.ToString());
	}
}

void AStreetGameMode::TryHomeDoor()
{
	if (!Game->Session.Evicted())
	{
		GoHome();
		return;
	}
	// The locks changed while the rent went unpaid: the key goes in and won't turn (Session's eviction; moving back in is
	// paid for from the apartment's apps, at Dee's).
	if (StreetTone)
	{
		StreetTone->LockRattle();
	}
	Toast(TEXT("Door 1812"), TEXT("Your key doesn't turn. New lock. Dee's couch is across the street."));
}

void AStreetGameMode::TryDeeDoor()
{
	if (Game->Session.Evicted())
	{
		// Her spare key's on the player's ring now: up the stairs to her front room (the same hub as home).
		GoHome();
		return;
	}
	// Her buzzer, and nobody comes down: on her game nights she's in the back room across the hall from the dryers.
	if (StreetTone)
	{
		StreetTone->PressBuzzer();
	}
	const EDeeGame Dee = DeeGameAt(Game->Session.WorldMinutes());
	Toast(TEXT("Dee's buzzer"), Dee == EDeeGame::Open || Dee == EDeeGame::LastHands ? TEXT("Dee's place. She's working the game tonight.") : TEXT("No answer."));
}

void AStreetGameMode::TryBackDoor()
{
	const EDeeGame Dee = DeeGameAt(Game->Session.WorldMinutes());
	if (Dee != EDeeGame::Open)
	{
		// Locked: the handle tried. When her game runs, or that it's winding down.
		if (StreetTone)
		{
			StreetTone->TryHandle();
		}
		Toast(TEXT("Dee's game"), Dee == EDeeGame::LastHands ? TEXT("No new seats after three. Dee's dealing the last hands.")
			: Dee == EDeeGame::Tonight                       ? TEXT("Dee's game is tonight, from nine.")
															 : TEXT("Dee's game is Tuesday, Thursday and Saturday, from nine."));
		return;
	}
	// Her game: the session's own rules, as the Burner app's button has them (the buy-in from the bankroll, too tired to
	// sit, a tournament running); the session saves and sends the player through the door (GoOut) when they can sit.
	const std::string Why = Game->Session.GoToGame("dee-game", ss::life::GameMinBuyInCents);
	if (!Why.empty())
	{
		Toast(TEXT("Dee's game"), Utf8ToFString(Why));
	}
}

void AStreetGameMode::TalkToAttendant()
{
	// What she says goes with the hour and with Dee's game in the back; each time she's asked, the next line for it.
	static const TCHAR* const OpenLines[] = {
		TEXT("They're in the back. Steel door. Knock first, Dee hates surprises."),
		TEXT("Game's going. Sal's been here since eight, feeding my change machine."),
		TEXT("If you're sitting, go on back. Forty to start."),
	};
	static const TCHAR* const LastLines[] = {
		TEXT("Dee won't seat anybody new after three. They'll be out by five, they always are."),
		TEXT("Last hands back there. You can hear Lou from here when he loses."),
	};
	static const TCHAR* const TonightLines[] = {
		TEXT("Game night. Dee opens the back at nine."),
		TEXT("Dee's game is tonight. She comes down around nine to set up the table."),
	};
	static const TCHAR* const LateLines[] = {
		TEXT("Graveyard shift. It's me, the machines and whoever can't sleep."),
		TEXT("Number six eats quarters. Don't feed it."),
		TEXT("Nothing dries out there. The rain keeps the dryers going all night."),
	};
	static const TCHAR* const MorningLines[] = {
		TEXT("Mornings are the drop-offs. Wash and fold, a dollar sixty a pound."),
		TEXT("That's my coffee. The Lucky Penny's got a pot on, across the street at the corner."),
	};
	static const TCHAR* const AfternoonLines[] = {
		TEXT("Change machine takes ones and fives. No fifties, don't even try."),
		TEXT("Somebody left a red sock in twelve. Everything's pink now."),
	};
	static const TCHAR* const EveningLines[] = {
		TEXT("After-work crowd. Grab a dryer while there's one free."),
		TEXT("Dee's game? Tuesdays, Thursdays and Saturdays. Nine o'clock, in the back."),
	};
	const double World = Game->Session.WorldMinutes();
	const double Hour = (World - FMath::FloorToDouble(World / ss::net::MinutesPerDay) * ss::net::MinutesPerDay) / 60.0;
	const int32 Pick = AttendantTalks++;
	auto Line = [Pick](const TCHAR* const* Lines, int32 Count) { return Lines[Pick % Count]; };
	const TCHAR* Said = nullptr;
	switch (DeeGameAt(World))
	{
	case EDeeGame::Open:
		Said = Line(OpenLines, UE_ARRAY_COUNT(OpenLines));
		break;
	case EDeeGame::LastHands:
		Said = Line(LastLines, UE_ARRAY_COUNT(LastLines));
		break;
	case EDeeGame::Tonight:
		Said = Line(TonightLines, UE_ARRAY_COUNT(TonightLines));
		break;
	default:
		Said = Hour < 5.0 ? Line(LateLines, UE_ARRAY_COUNT(LateLines))
			: Hour < 11.0 ? Line(MorningLines, UE_ARRAY_COUNT(MorningLines))
			: Hour < 17.0 ? Line(AfternoonLines, UE_ARRAY_COUNT(AfternoonLines))
						  : Line(EveningLines, UE_ARRAY_COUNT(EveningLines));
		break;
	}
	// Evicted, she knows (the whole building does).
	if (Game->Session.Evicted() && Pick % 3 == 0)
	{
		Said = TEXT("You're the one on Dee's couch. She says you snore.");
	}
	Toast(AttendantName, Said);
}

void AStreetGameMode::OnEat()
{
	if (!Game || bLeaving)
	{
		return;
	}
	// The session picks what helps most and says why not when nothing would (an empty bag, nothing needed).
	const std::string Why = Game->Session.EatFromBag();
	if (!Why.empty())
	{
		Toast(TEXT("Your bag"), Utf8ToFString(Why));
		return;
	}
	const ss::Session::Eaten& Ate = Game->Session.LastEaten;
	const ss::store::Item* Item = ss::store::Find(Ate.ItemId);
	if (AShortStackCharacter* Hero = GetHero())
	{
		Hero->Sip();
	}
	if (Audio && Item && Item->Look == ss::store::Art::Can)
	{
		Audio->PlayEffect(ss::audio::Effect::CanOpen, 0.6f);
	}
	// "Drank the Cascade. Thirst -45, energy +2."
	Toast(TEXT("You"), Utf8ToFString(Ate.Line));
}

void AStreetGameMode::OnPause()
{
	// On the way out (home, or through Dee's door, the save already written) the level is about to change: no menu over it.
	if (Game->Counter.IsOpen() || bLeaving)
	{
		return;
	}
	Game->Menu.Info = ss::ui::DescribeSession(Game->Session, true);
	Game->Menu.Info.Resolutions = StreetResolutions();
	Game->Menu.Gamepad = bGamepad;
	// Indoors the heading names the room (the store, the laundromat, a shop: the registry's room the hero is in); out on the
	// street, the street.
	const AShortStackCharacter* Hero = GetHero();
	const FStreetZone* Zone = Hero ? ZoneAt(Hero->GetActorLocation()) : nullptr;
	const std::string Room = Zone && Zone->bInterior ? FStringToUtf8(Zone->Place) : std::string();
	Game->Menu.SetVenue(ss::ui::Venue::Street, Room);
	Game->Menu.Open(ss::ui::FrontEnd::Page::Pause, RealTime);
	FocusWidget(MenuWidget.Get());
}

void AStreetGameMode::GoHome()
{
	if (bLeaving)
	{
		return;
	}
	// A minute up the stairs (1812's, or Dee's while evicted: the hub is hers then), then the session's save carries the
	// street back to the desk, the clock with it.
	Game->Session.Save();
	Leave(FName(TEXT("NightOne")), TEXT("Home?Street"), true);
	if (Audio)
	{
		Audio->PlayEffect(ss::audio::Effect::Thump, 0.7f);
	}
}

bool AStreetGameMode::GoOut(const FString& ActivityId, int64 BuyInCents)
{
	// Only Dee's game is walked into from here (the Embercrest's bus is caught from the apartment's apps).
	if (bLeaving || !Game || ActivityId != TEXT("dee-game"))
	{
		return false;
	}
	// The session saved itself before calling out. Through the steel door: the Back Room deals the night, and sends the
	// player back out through the laundromat when it's over (FromStreet).
	if (StreetTone)
	{
		StreetTone->SteelDoor();
	}
	Leave(FName(TEXT("BackRoom")), FString::Printf(TEXT("BuyIn=%lld?FromStreet"), BuyInCents), false);
	UE_LOG(LogStreet, Log, TEXT("Through the back door into Dee's game with %lld cents"), BuyInCents);
	return true;
}

void AStreetGameMode::Leave(const FName& Level, const FString& Options, bool bWithClock)
{
	bLeaving = true;
	LeaveAt = RealTime + 1.4;
	LeaveLevel = Level;
	LeaveOptions = Options;
	bLeaveWithClock = bWithClock;
	if (APlayerController* Pc = GetWorld()->GetFirstPlayerController())
	{
		if (Pc->PlayerCameraManager)
		{
			Pc->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, 1.2f, FLinearColor::Black, true, true);
		}
	}
	if (StreetTone)
	{
		StreetTone->FadeOut(1.3f); // the street falls away behind the door
	}
}

void AStreetGameMode::Toast(const FString& From, const FString& Body)
{
	ss::ui::StreetHudInfo::Toast T;
	T.From = FStringToUtf8(From);
	T.Body = FStringToUtf8(Body);
	T.At = RealTime;
	Toasts.Add(T);
	while (Toasts.Num() > 4)
	{
		Toasts.RemoveAt(0);
	}
}

void AStreetGameMode::ApplySettings(const ss::ui::GameSettings& Settings, bool bSave)
{
	// As the apartment applies them (NightOneGameMode::ApplySettings): scalability, the render scale and the frame cap,
	// ray-traced Lumen, the lens, the field of view, the mix and the controls.
	if (UGameUserSettings* User = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		User->SetOverallScalabilityLevel(FMath::Clamp(Settings.Quality, 0, 4));
		User->SetResolutionScaleValueEx(static_cast<float>(FMath::Min(Settings.ResolutionScale, 100)));
		User->SetFrameRateLimit(static_cast<float>(Settings.FrameRateLimit));
		User->SetVSyncEnabled(Settings.VSync);
		if (GIsEditor)
		{
			// Play-In-Editor shares the editor's window: leave its size and mode alone.
			User->ApplyNonResolutionSettings();
		}
		else
		{
			const FIntPoint Res = StreetParseResolution(Settings.Resolution);
			User->SetScreenResolution(Res.X > 0 ? Res : User->GetDesktopResolution());
			User->SetFullscreenMode(Settings.WindowMode == 0 ? EWindowMode::Fullscreen : Settings.WindowMode == 1 ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed);
			User->ApplySettings(false);
		}
	}
	// The render scale: fixed, or moving to hold the chosen frame rate (FrameBudget.h).
	FrameBudget.Configure(Settings.DynamicTarget, Settings.ResolutionScale);
	SetStreetCvar(TEXT("r.Lumen.HardwareRayTracing"), Settings.RayTracing ? 1 : 0);
	SetStreetCvar(TEXT("r.Lumen.HardwareRayTracing.LightingMode"), Settings.RayTracing && Settings.Quality >= 4 ? 2 : 0);
	if (Stage)
	{
		Stage->SetLensOptions(static_cast<float>(Settings.Brightness - 50) / 50.0f * 1.5f, Settings.FilmGrain == 0 ? 0.0f : Settings.FilmGrain == 1 ? 1.0f : 2.2f,
			Settings.ChromaticAberration ? 1.0f : 0.0f, Settings.MotionBlur);
	}
	if (AShortStackCharacter* Hero = GetHero())
	{
		// The game's own views at the default 50, wider or narrower with the setting.
		const float WasFov = Hero->ThirdPersonFov;
		Hero->FirstPersonFov = StreetFov(84.0f, Settings.FieldOfView);
		Hero->ThirdPersonFov = StreetFov(72.0f, Settings.FieldOfView);
		// Inside a room with a camera profile, its view again on the new setting (UpdateCameraProfile blends there from the old).
		if (!ProfileZone.IsNone())
		{
			Hero->ThirdPersonFov = WasFov;
			ProfileBlend = 0.0f;
			BlendFromArm = Hero->ArmLength;
			BlendFromFov = WasFov;
		}
	}
	if (Audio)
	{
		// The street's own sound quiets behind the store's glass by itself (StreetAudio.h): the ambience level is the player's.
		Audio->SetMix(Settings.MasterVolume / 100.0f, Settings.EffectsVolume / 100.0f, Settings.AmbienceVolume / 100.0f);
	}
	// The engine mutes a game whose window isn't in front unless told otherwise.
	FApp::SetUnfocusedVolumeMultiplier(Settings.BackgroundAudio ? 1.0f : 0.0f);
	Sensitivity = 0.11f * static_cast<float>(Settings.LookSensitivity) / 100.0f;
	bInvertLook = Settings.InvertLook;
	if (bSave)
	{
		SettingsDirtyAt = RealTime; // written once the player stops changing things
	}
}

void AStreetGameMode::SaveSettingsNow()
{
	SettingsDirtyAt = -1.0;
	if (!Game)
	{
		return;
	}
	if (UNightOneSaveGame* Obj = Cast<UNightOneSaveGame>(UGameplayStatics::CreateSaveGameObject(UNightOneSaveGame::StaticClass())))
	{
		Obj->Data = Utf8ToFString(Game->Menu.Settings.Serialize());
		UGameplayStatics::SaveGameToSlot(Obj, UNightOneSaveGame::SettingsSlotName(), 0);
	}
}

// ------------------------------------------------------------------ the shops (spec 7.5.3)

namespace StreetGameDetail
{
/** The venue a registry shot belongs to, by its id ("barber-mirror" is the barber's; spec 2.10's ids lead with the venue). */
FName ShotVenue(FName Shot)
{
	FString Venue;
	FString Rest;
	return Shot.ToString().Split(TEXT("-"), &Venue, &Rest) ? FName(*Venue) : NAME_None;
}

/** What someone says when town has nothing yet (its lines not in): the spec's voices for the keepers (5.1-5.8). */
FString TalkFallback(FName Person)
{
	const FString P = Person.ToString().ToLower();
	if (P == TEXT("augie"))
	{
		return TEXT("Guitars I don't sell to card players. You'd hock it back to me inside a month.");
	}
	if (P == TEXT("lonnie"))
	{
		return TEXT("Sit down, sit down. Same as last time, or you finally gonna let me fix that?");
	}
	if (P == TEXT("bea"))
	{
		return TEXT("Booth's on the left. Curtain sticks. Pull, don't yank.");
	}
	if (P == TEXT("rosa"))
	{
		return TEXT("Towel's free. The shake isn't.");
	}
	if (P == TEXT("ruby"))
	{
		return TEXT("Sit anywhere that isn't Sal's.");
	}
	if (P == TEXT("marlene"))
	{
		return TEXT("Coffee, hon? You look like you lost a flip.");
	}
	return TEXT("...");
}

/** Puts a character's feet at Feet (its capsule's half height over it), facing as it faces; the controller looks that way. */
void PlaceAt(AShortStackCharacter& Hero, const FTransform& Feet, bool bFeetOnFloor)
{
	FVector At = Feet.GetLocation();
	if (bFeetOnFloor)
	{
		At.Z += Hero.GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}
	Hero.SetActorLocationAndRotation(At, FRotator(0.0f, Feet.Rotator().Yaw, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
	if (UCharacterMovementComponent* Move = Hero.GetCharacterMovement())
	{
		Move->StopMovementImmediately();
	}
	if (AController* C = Hero.GetController())
	{
		C->SetControlRotation(FRotator(0.0f, Feet.Rotator().Yaw, 0.0f));
	}
}
} // namespace StreetGameDetail

ss::ui::HostCaps AStreetGameMode::BuildCaps() const
{
	using ss::hero::Slot;
	// looks' answer for the hero's face and body (CastLooks::HostCapsFor: the groom library's styles, beards and brows, the
	// garments with a mesh on this body, the gear the pawn's shelf can stand up, the 3D preview).
	const AShortStackCharacter* Hero = GetHero();
	ss::ui::HostCaps Caps = CastLooks::HostCapsFor(Hero);
	if (!Hero)
	{
		Caps.GarmentKinds = AShopStage::WearableKinds();
	}
	// Each indexed list exactly as long as the Look's options for its slot (spec 7.4: past the end counts as missing).
	Caps.HairStyles.resize(static_cast<size_t>(FMath::Max(0, ss::hero::OptionCount(Slot::Hair))), false);
	Caps.FacialHair.resize(static_cast<size_t>(FMath::Max(0, ss::hero::OptionCount(Slot::FacialHair))), false);
	Caps.Brows.resize(static_cast<size_t>(FMath::Max(0, ss::hero::OptionCount(Slot::Brows))), false);
	if (!Game)
	{
		return Caps;
	}
	// What the hero wears on their head now renders whatever the library holds (a line-up keeps it; the razor's nothing,
	// shaved and clean-shaven, needs no groom).
	const ss::hero::Look& L = Game->Session.Person.Appearance;
	auto Mark = [](std::vector<bool>& V, int I) {
		if (I >= 0 && I < static_cast<int>(V.size()))
		{
			V[static_cast<size_t>(I)] = true;
		}
	};
	Mark(Caps.HairStyles, L.Hair);
	Mark(Caps.HairStyles, 0);
	Mark(Caps.FacialHair, L.FacialHair);
	Mark(Caps.FacialHair, 0);
	Mark(Caps.Brows, L.Brows);
	// What the shops' 3D can stand up: the pawn's shelves (GearKinds).
	for (const TWeakObjectPtr<AShopStage>& S : Shops)
	{
		if (S.IsValid())
		{
			S->FillCaps(Caps);
		}
	}
	return Caps;
}

bool AStreetGameMode::OpenShop(FName Venue, const FString& Page)
{
	if (!Game || bLeaving || Venue.IsNone())
	{
		return false;
	}
	if (Screen && Screen->IsOpen())
	{
		CloseShop(false);
	}
	std::unique_ptr<ss::ui::ShopScreen> Made = ss::ui::MakeShopScreen(FStringToUtf8(Venue.ToString()), Game->Session);
	if (!Made)
	{
		UE_LOG(LogStreet, Warning, TEXT("No shop screen for %s"), *Venue.ToString());
		return false;
	}
	if (Card)
	{
		// A talk card that closed itself and wasn't put away yet: its person let go before its screen is replaced.
		Card = nullptr;
		EndTalk();
	}
	Screen.Reset(Made.release());
	++ScreenSerial;
	ScreenVenue = Venue;
	ScreenPage = Page;
	// What it may sell, by what the 3D can show (spec 5.0), before it opens; the thrift's racks stock the same kinds.
	const ss::ui::HostCaps Caps = BuildCaps();
	if (AThriftStage* Thrift = Cast<AThriftStage>(ShopFor(TEXT("thrift"))))
	{
		Thrift->SetGarmentKinds(Caps.GarmentKinds);
	}
	Screen->SetCaps(Caps);
	Screen->Gamepad = bGamepad;
	Screen->Open(RealTime, FStringToUtf8(Page));
	FocusWidget(ShopWidget.Get());
	UE_LOG(LogStreet, Log, TEXT("The %s screen open on %s"), *Venue.ToString(), *Page);
	return true;
}

void AStreetGameMode::CloseShop(bool bStand)
{
	const FName Venue = ScreenVenue;
	if (Screen && Screen->IsOpen())
	{
		Screen->Close(RealTime);
	}
	Screen.Reset();
	++ScreenSerial;
	if (Card)
	{
		Card = nullptr;
		EndTalk();
	}
	ScreenVenue = NAME_None;
	ScreenPage.Reset();
	// The look as it was, the camera back on the hero, up out of the seat (a service under way keeps its own camera and seat).
	if (!Cinema.bActive)
	{
		if (bPreviewing)
		{
			if (AShortStackCharacter* Hero = GetHero())
			{
				Hero->EndPreview();
			}
			bPreviewing = false;
		}
		if (bStand && !SeatedAt.IsNone())
		{
			StandUp();
		}
		EndShot(ShotBlendSeconds);
	}
	// The shop shows what changed (the pawn's shelf after a sale, the racks after a buy).
	if (AShopStage* Shop = ShopFor(Venue))
	{
		Shop->SyncSession(Game->Session);
	}
	ReturnInputToGame();
}

void AStreetGameMode::UpdateShopScreen(float Dt)
{
	if (!Screen)
	{
		return;
	}
	if (Screen->IsOpen())
	{
		DrawOverlay(ShopWidget.Get(), [this](ss::ui::Canvas& Cv) { Screen->Draw(Cv, RealTime); });
	}
	DrainRequests();
	// Walked away (a key, or a click on the footer inside Draw); or closed by itself (a service begun).
	if (Screen && (Screen->TakeLeave() || !Screen->IsOpen()))
	{
		CloseShop(!Cinema.bActive);
		return;
	}
	// The thrift's three-way mirror: the camera orbits the hero (the right stick, or the mouse dragged with its right button).
	if (CameraShot == TEXT("thrift-fitting") && ShopCamera)
	{
		float Turn = 0.0f;
		if (const APlayerController* Pc = GetWorld()->GetFirstPlayerController())
		{
			Turn += Pc->GetInputAnalogKeyState(EKeys::Gamepad_RightX) * OrbitStick * Dt;
		}
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication& App = FSlateApplication::Get();
			const FVector2D Cursor = App.GetCursorPos();
			if (App.GetPressedMouseButtons().Contains(EKeys::RightMouseButton) && OrbitMouse.X >= 0.0)
			{
				Turn += static_cast<float>(Cursor.X - OrbitMouse.X) * 0.25f;
			}
			OrbitMouse = Cursor;
		}
		const float Was = OrbitYaw;
		OrbitYaw = FMath::Clamp(OrbitYaw + Turn, -OrbitLimit, OrbitLimit);
		if (OrbitYaw != Was)
		{
			const FQuat Around(FVector::UpVector, FMath::DegreesToRadians(OrbitYaw));
			ShopCamera->SetActorLocationAndRotation(OrbitPivot + Around.RotateVector(CameraBase.GetLocation() - OrbitPivot), Around * CameraBase.GetRotation());
		}
	}
}

void AStreetGameMode::DrainRequests()
{
	if (!Screen)
	{
		return;
	}
	AShortStackCharacter* Hero = GetHero();
	const int32 From = ScreenSerial;
	for (const ss::ui::HostRequest& R : Screen->TakeRequests())
	{
		switch (R.K)
		{
		case ss::ui::HostRequest::PreviewLook:
			// The draft look on the hero in place (looks' PreviewLook: only what differs changes, grooms async).
			if (Hero)
			{
				Hero->PreviewLook(R.Look, R.Closet);
				bPreviewing = true;
			}
			break;
		case ss::ui::HostRequest::EndPreview:
			if (Hero && bPreviewing)
			{
				Hero->EndPreview();
			}
			bPreviewing = false;
			break;
		case ss::ui::HostRequest::Camera:
			ShowShot(R.Name.empty() ? NAME_None : FName(*Utf8ToFString(R.Name)), ShotBlendSeconds);
			break;
		case ss::ui::HostRequest::BeginService:
			// The page has charged it and started the skip (Session::ShopService): the venue's cinematic plays under it.
			BeginCinematic(ScreenVenue, Utf8ToFString(R.Name));
			break;
		case ss::ui::HostRequest::Redress:
		{
			ss::hero::Look L = Game->Session.Person.Appearance;
			ss::wardrobe::ApplyTo(R.Closet, L);
			RedressHero(L, R.Closet);
			break;
		}
		case ss::ui::HostRequest::Sit:
			SitAt(ShopFor(ScreenVenue), R.Name.empty() ? FName(*ScreenPage) : FName(*Utf8ToFString(R.Name)));
			break;
		case ss::ui::HostRequest::Stand:
			StandUp();
			break;
		case ss::ui::HostRequest::Talk:
			AnswerTalk(Utf8ToFString(R.Name));
			break;
		case ss::ui::HostRequest::Leave:
			EndTalk();
			break;
		}
		if (ScreenSerial != From || !Screen)
		{
			break; // a request closed it or opened another (a service begun, the talk card's "Get a cut")
		}
	}
}

void AStreetGameMode::ShowShot(FName Shot, float Blend)
{
	APlayerController* Pc = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	AShortStackCharacter* Hero = GetHero();
	const UStreetWorld* Registry = StreetWorld();
	if (Shot.IsNone())
	{
		EndShot(Blend);
		return;
	}
	FTransform View;
	float Fov = 60.0f;
	if (!Pc || !Hero || !Registry || !Registry->Shot(Shot, View, Fov))
	{
		UE_LOG(LogStreet, Warning, TEXT("No shot %s in the registry"), *Shot.ToString());
		return;
	}
	if (!CameraShot.IsNone() && CameraShot != Shot)
	{
		if (AShopStage* Was = ShopFor(ShotVenue(CameraShot)))
		{
			Was->OnShot(CameraShot, false);
		}
	}
	AShopStage* Shop = ShopFor(ShotVenue(Shot));
	// The shot's mark takes the hero there (the thrift's mirror); a shot without one puts them back where they were.
	FTransform Mark;
	if (Shop && Shop->ShotMark(Shot, Mark))
	{
		if (!bMarkMoved)
		{
			BeforeMark = Hero->GetActorTransform();
			bMarkMoved = true;
		}
		PlaceAt(*Hero, Mark, false);
		OrbitPivot = Mark.GetLocation();
	}
	else if (bMarkMoved)
	{
		PlaceAt(*Hero, BeforeMark, false);
		bMarkMoved = false;
	}
	if (!ShopCamera)
	{
		FActorSpawnParameters Params;
		Params.ObjectFlags |= RF_Transient;
		ShopCamera = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), View, Params);
	}
	if (!ShopCamera)
	{
		return;
	}
	ShopCamera->SetActorTransform(View);
	if (UCameraComponent* Cam = ShopCamera->GetCameraComponent())
	{
		// The shot's view at the player's own field of view setting (as the street's cameras scale with it).
		Cam->SetFieldOfView(StreetFov(Fov, Game ? Game->Menu.Settings.FieldOfView : 50));
		Cam->SetConstraintAspectRatio(false);
	}
	CameraBase = View;
	OrbitYaw = 0.0f;
	OrbitMouse = FVector2D(-1.0, -1.0);
	const bool bNew = CameraShot != Shot;
	CameraShot = Shot;
	Pc->SetViewTargetWithBlend(ShopCamera, Blend);
	if (Shop && bNew)
	{
		Shop->OnShot(Shot, true);
	}
}

void AStreetGameMode::EndShot(float Blend)
{
	if (!CameraShot.IsNone())
	{
		if (AShopStage* Shop = ShopFor(ShotVenue(CameraShot)))
		{
			Shop->OnShot(CameraShot, false);
		}
	}
	CameraShot = NAME_None;
	AShortStackCharacter* Hero = GetHero();
	if (bMarkMoved && Hero)
	{
		PlaceAt(*Hero, BeforeMark, false);
	}
	bMarkMoved = false;
	APlayerController* Pc = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (Pc && Hero && Pc->GetViewTarget() != Hero && (!ShotCamera || Pc->GetViewTarget() != ShotCamera))
	{
		Pc->SetViewTargetWithBlend(Hero, Blend);
	}
}

bool AStreetGameMode::SitAt(AShopStage* Shop, FName Seat)
{
	AShortStackCharacter* Hero = GetHero();
	FTransform Feet;
	FTransform Stand;
	float Height = 46.0f;
	FName SeatShot;
	if (!Hero || !Shop || !Shop->SeatFor(Seat, Feet, Height, Stand, &SeatShot))
	{
		return false;
	}
	if (!SeatedAt.IsNone())
	{
		Hero->SetSeated(false);
	}
	// Their feet at the seat's mark, facing as it faces; the seated pose puts them on it (people-ue's solve, spec 6.6).
	PlaceAt(*Hero, Feet, true);
	Hero->SetSeated(true, Height);
	SeatedAt = Seat;
	SeatedVenue = Shop->Venue;
	StandTo = Stand;
	if (!SeatShot.IsNone())
	{
		ShowShot(SeatShot, ShotBlendSeconds);
	}
	return true;
}

void AStreetGameMode::StandUp()
{
	AShortStackCharacter* Hero = GetHero();
	if (SeatedAt.IsNone())
	{
		return;
	}
	SeatedAt = NAME_None;
	SeatedVenue = NAME_None;
	if (Hero)
	{
		Hero->SetSeated(false);
		PlaceAt(*Hero, StandTo, false);
	}
	if (!(Screen && Screen->IsOpen()) && !Cinema.bActive)
	{
		EndShot(ShotBlendSeconds);
	}
}

void AStreetGameMode::UseSeat(const FStreetUse& Use)
{
	AShopStage* Shop = ShopFor(Use.Venue);
	if (!Shop)
	{
		NotYet(Use);
		return;
	}
	if (Use.WhyNot)
	{
		const FString Why = Use.WhyNot();
		if (!Why.IsEmpty())
		{
			ShowClosed(Why);
			return;
		}
	}
	// The barber's chair with somebody ahead in it (spec 5.1): the bench and the wait first, then the chair.
	if (Use.Id == TEXT("barber-chair1") && ss::town::BarberQueue(Game->Session.WorldMinutes()) > 0)
	{
		FStreetUse Bench = Use;
		Bench.Param = TEXT("barber-bench2");
		UseWait(Bench);
		return;
	}
	const FName Seat = Use.Param.IsEmpty() ? Use.Id : FName(*Use.Param);
	if (!SitAt(Shop, Seat))
	{
		NotYet(Use);
		return;
	}
	// Seated, the venue's screen on its page ("barber-chair1": the Style Book; "diner-stool3": the menu).
	if (!OpenShop(Use.Venue, Use.Id.ToString()))
	{
		// No screen for it: the seat alone (E stands up again).
		UE_LOG(LogStreet, Log, TEXT("Sat at %s"), *Seat.ToString());
	}
}

void AStreetGameMode::UseWait(const FStreetUse& Use)
{
	if (Use.WhyNot)
	{
		const FString Why = Use.WhyNot();
		if (!Why.IsEmpty())
		{
			ShowClosed(Why);
			return;
		}
	}
	const double Now = Game->Session.WorldMinutes();
	AShopStage* Shop = ShopFor(Use.Venue);
	const FName Seat = Use.Param.IsEmpty() ? NAME_None : FName(*Use.Param);
	double Until = Now;
	bool bThenChair = false;
	if (Use.Venue == TEXT("barber"))
	{
		// The bench while somebody's in the chair; with nobody ahead it's only a seat.
		const int32 Queue = ss::town::BarberQueue(Now);
		if (Queue <= 0)
		{
			if (Shop)
			{
				SitAt(Shop, Seat);
			}
			return;
		}
		Until = Now + Queue;
		bThenChair = true;
	}
	else
	{
		// The shelter: until the 14 comes (spec 7.6).
		const std::vector<double> Next = ss::town::BusTimes("market-east", Now, 1);
		Until = Next.empty() ? Now + 30.0 : Next.front();
	}
	if (Shop && !Seat.IsNone())
	{
		SitAt(Shop, Seat);
	}
	const std::string Why = Game->Session.WaitAt(FStringToUtf8(Use.Venue.ToString()), Until);
	if (!Why.empty())
	{
		Toast(Shop ? Shop->VenueName : FString(TEXT("Wait")), Utf8ToFString(Why));
		if (!bThenChair)
		{
			StandUp();
		}
		return;
	}
	BeginCinematic(Use.Venue, FString());
	Cinema.bThenChair = bThenChair;
}

void AStreetGameMode::BeginCinematic(FName Venue, const FString& Service, bool bDry, float Seconds)
{
	AShortStackCharacter* Hero = GetHero();
	AShopStage* Shop = ShopFor(Venue);
	if (!Game || !Hero)
	{
		return;
	}
	const ss::Session::Skip& K = Game->Session.TimeSkip;
	if (!bDry && !K.Active)
	{
		UE_LOG(LogStreet, Warning, TEXT("A service at %s (%s) with no skip running: played as it would look"), *Venue.ToString(), *Service);
	}
	Cinema = FCinema();
	Cinema.bActive = true;
	Cinema.bDry = bDry || !K.Active;
	Cinema.Venue = Venue;
	Cinema.Service = Service;
	Cinema.Page = ScreenPage;
	Cinema.Length = Cinema.bDry ? FMath::Max(1.0, static_cast<double>(Seconds > 0.0f ? Seconds : 4.0f)) : FMath::Max(0.5, K.RealSeconds);
	Cinema.StartedAt = Cinema.bDry ? RealTime : K.RealStart;
	// The shop's shots: those under the skip spread over its length, the after-shots (the reveal) as they are. A wait has
	// none: the camera stays as it is (a seat's shot, the hero's own).
	const bool bService = Shop && !Service.IsEmpty() && Service != TEXT("wait");
	if (bService)
	{
		double Total = 0.0;
		for (const TPair<FName, float>& S : Shop->Cinematic(FName(*Service)))
		{
			if (S.Value >= 0.0f)
			{
				Cinema.During.Add(S);
				Total += S.Value;
			}
			else
			{
				Cinema.After.Emplace(S.Key, -S.Value);
			}
		}
		for (TPair<FName, float>& S : Cinema.During)
		{
			S.Value = Total > 0.0 ? static_cast<float>(S.Value / Total * Cinema.Length) : static_cast<float>(Cinema.Length / FMath::Max(1, Cinema.During.Num()));
		}
	}
	// The old look through the cut (the new one goes on under black at its end, behind a cut: spec 5.1).
	if (bPreviewing)
	{
		Hero->EndPreview();
		bPreviewing = false;
	}
	// The hero's beat for it (people-ue's names, spec 6.6: "chair" at the barber, "treadmill", "bag" or "weights" at the gym,
	// "eat" or "coffee" at Ruby's; a wait at Ruby's is a coffee), and the hero to the service's place (a treadmill, the bag,
	// the bench) where it has one.
	const FName ServiceBeat = Service.IsEmpty() ? NAME_None
		: Service == TEXT("wait")				? (Venue == TEXT("diner") ? FName(TEXT("coffee")) : NAME_None)
												: AStreetNpcs::ServiceBeatFor(Venue, Service);
	FTransform Feet;
	float SeatHeight = 0.0f;
	FName Beat;
	if (Shop && !Service.IsEmpty() && Service != TEXT("wait") && Shop->ServiceMark(FName(*Service), Feet, SeatHeight, Beat))
	{
		Cinema.bMoved = true;
		Cinema.Before = SeatedAt.IsNone() ? Hero->GetActorTransform() : StandTo;
		if (!SeatedAt.IsNone())
		{
			Hero->SetSeated(false);
			SeatedAt = NAME_None;
		}
		PlaceAt(*Hero, Feet, true);
		if (SeatHeight > 0.0f)
		{
			Hero->SetSeated(true, SeatHeight);
			Cinema.bSeatedForIt = true;
		}
		Hero->SetWork(ServiceBeat.IsNone() ? Beat : ServiceBeat, Feet);
	}
	else if (!ServiceBeat.IsNone())
	{
		// Where they are (the barber's chair, a stool, a booth): the beat at their own feet.
		FTransform Here = Hero->GetActorTransform();
		Here.AddToTranslation(FVector(0.0, 0.0, -Hero->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
		Hero->SetWork(ServiceBeat, Here);
	}
	if (bService)
	{
		Shop->OnService(FName(*Service), true);
	}
	// The corner holds while the clock races (spec 2.5).
	if (UStreetWorld* Registry = StreetWorld())
	{
		Registry->SetHold(true, K.Active ? K.From : Game->Session.WorldMinutes());
	}
	Cinema.Phase = 0;
	Cinema.PhaseAt = RealTime;
	if (Cinema.During.Num() > 0)
	{
		Cinema.Shot = 0;
		ShowShot(Cinema.During[0].Key, 0.0f);
	}
	UE_LOG(LogStreet, Log, TEXT("A service at %s (%s): %d shots over %.1f s, %d after"), *Venue.ToString(), *Service, Cinema.During.Num(), Cinema.Length, Cinema.After.Num());
}

void AStreetGameMode::UpdateCinematic(float Dt)
{
	if (!Cinema.bActive || !Game)
	{
		return;
	}
	APlayerController* Pc = GetWorld()->GetFirstPlayerController();
	APlayerCameraManager* Cam = Pc ? Pc->PlayerCameraManager.Get() : nullptr;
	AShortStackCharacter* Hero = GetHero();
	AShopStage* Shop = ShopFor(Cinema.Venue);
	const double Since = RealTime - Cinema.PhaseAt;
	switch (Cinema.Phase)
	{
	case 0:
	{
		// Under the skip: the shots in turn, cut to (the clock in the room racing with it).
		const double T = RealTime - Cinema.StartedAt;
		int32 Want = Cinema.During.Num() - 1;
		double Acc = 0.0;
		for (int32 I = 0; I < Cinema.During.Num(); ++I)
		{
			Acc += Cinema.During[I].Value;
			if (T < Acc)
			{
				Want = I;
				break;
			}
		}
		if (Want >= 0 && Want != Cinema.Shot)
		{
			Cinema.Shot = Want;
			ShowShot(Cinema.During[Want].Key, 0.0f);
		}
		const bool bDone = Cinema.bDry ? T >= Cinema.Length : !Game->Session.TimeSkip.Active;
		if (bDone)
		{
			Cinema.Phase = 1;
			Cinema.PhaseAt = RealTime;
			if (Cam)
			{
				Cam->StartCameraFade(0.0f, 1.0f, CinemaFade, FLinearColor::Black, false, true);
			}
		}
		break;
	}
	case 1:
		// Black, held until the people the hour now wants are built (spec 6.7: this is one of the blacks they're built under;
		// never longer than a few seconds).
		if (Since >= CinemaFade && (!People || People->IsReady() || Since >= CinemaFade + 3.0))
		{
			// Under black: the hero off the machine, the corner let go, the look for keeps (the cut and the color landed with
			// the skip's end: Session::SettleShop), then the reveal or the hero's own camera.
			if (Shop && !Cinema.Service.IsEmpty() && Cinema.Service != TEXT("wait"))
			{
				Shop->OnService(FName(*Cinema.Service), false);
			}
			if (UStreetWorld* Registry = StreetWorld())
			{
				Registry->SetHold(false, 0.0);
			}
			if (Hero)
			{
				Hero->SetWork(NAME_None, FTransform::Identity);
				if (Cinema.bSeatedForIt)
				{
					Hero->SetSeated(false);
				}
				if (Cinema.bMoved)
				{
					PlaceAt(*Hero, Cinema.Before, false);
				}
				if (!Cinema.bDry)
				{
					// In place: only what the service changed (the cut, the beard, the color) is swapped.
					Hero->RefreshLook(Game->Session.Person.Appearance, Game->Session.Life.Closet);
				}
			}
			Cinema.Phase = Cinema.After.Num() > 0 ? 2 : 3;
			Cinema.PhaseAt = RealTime;
			if (Cinema.Phase == 2)
			{
				ShowShot(Cinema.After[0].Key, 0.0f);
			}
			else if (Cinema.During.Num() > 0 || Cinema.bMoved)
			{
				// Back from the service's cameras (a wait leaves the camera as it was: a seat's shot, the hero's own).
				EndShot(0.0f);
			}
			if (Cam)
			{
				Cam->StartCameraFade(1.0f, 0.0f, CinemaFade, FLinearColor::Black, false, false);
			}
		}
		break;
	case 2:
		if (Since >= Cinema.After[0].Value + CinemaFade)
		{
			Cinema.Phase = 3;
			Cinema.PhaseAt = RealTime;
			EndShot(ShotBlendSeconds);
		}
		break;
	default:
	{
		if (Since < ShotBlendSeconds)
		{
			break;
		}
		// Done: into the barber's chair after the wait; the diner's menu again at the seat (another cup, the check); up and
		// out of the chair after a cut.
		const FCinema Was = Cinema;
		Cinema = FCinema();
		if (Was.bThenChair && Shop)
		{
			StandUp();
			if (SitAt(Shop, TEXT("barber-chair1")))
			{
				OpenShop(Was.Venue, TEXT("barber-chair1"));
			}
		}
		else if (Screen && Screen->IsOpen())
		{
			// The page stayed up through it (a wait it began): it carries on.
			FocusWidget(ShopWidget.Get());
			break;
		}
		else if (!SeatedAt.IsNone() && Was.Venue == TEXT("diner") && !Was.bDry && !Was.Service.IsEmpty())
		{
			OpenShop(Was.Venue, Was.Page.IsEmpty() ? SeatedAt.ToString() : Was.Page);
		}
		else if (!SeatedAt.IsNone() && Was.Venue != TEXT("diner"))
		{
			StandUp();
		}
		ReturnInputToGame();
		break;
	}
	}
}

void AStreetGameMode::UnderBlack(TFunction<void()> Action, float Out)
{
	if (BlackAt >= 0.0 && BlackAction)
	{
		// Already going dark: both run there.
		TFunction<void()> First = MoveTemp(BlackAction);
		BlackAction = [First, Action]() {
			First();
			Action();
		};
		return;
	}
	BlackAction = MoveTemp(Action);
	BlackAt = RealTime + Out + 0.05;
	if (APlayerController* Pc = GetWorld()->GetFirstPlayerController(); Pc && Pc->PlayerCameraManager)
	{
		Pc->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, Out, FLinearColor::Black, false, true);
	}
}

void AStreetGameMode::UpdateBlack(float Dt)
{
	if (BlackAt < 0.0 || RealTime < BlackAt)
	{
		return;
	}
	BlackAt = -1.0;
	TFunction<void()> Action = MoveTemp(BlackAction);
	BlackAction = nullptr;
	if (Action)
	{
		Action();
	}
	if (APlayerController* Pc = GetWorld()->GetFirstPlayerController(); Pc && Pc->PlayerCameraManager)
	{
		Pc->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, CinemaFade, FLinearColor::Black, false, false);
	}
}

void AStreetGameMode::RedressHero(const ss::hero::Look& Look, const ss::wardrobe::Closet& Closet)
{
	AShortStackCharacter* Hero = GetHero();
	if (!Hero || !Game)
	{
		return;
	}
	// In place (looks' RefreshLook: only what differs changes: the garments, the tee, the hat and the glasses). The preview
	// that was up is the look now.
	bPreviewing = false;
	Hero->RefreshLook(Look, Closet);
}

void AStreetGameMode::SyncShops()
{
	ShopsSyncedAt = RealTime;
	if (!Game)
	{
		return;
	}
	const uint64 Kinds = BuildCaps().GarmentKinds;
	for (const TWeakObjectPtr<AShopStage>& S : Shops)
	{
		AShopStage* Shop = S.Get();
		if (!Shop)
		{
			continue;
		}
		if (AThriftStage* Thrift = Cast<AThriftStage>(Shop))
		{
			Thrift->SetGarmentKinds(Kinds);
		}
		Shop->SyncSession(Game->Session);
	}
}

void AStreetGameMode::WatchCounterSale()
{
	if (!Game)
	{
		return;
	}
	if (!Game->Counter.IsOpen())
	{
		Game->bCounterTop = false;
		return;
	}
	const std::vector<ss::life::LedgerEntry>& Ledger = Game->Session.Life.Ledger;
	const ss::life::LedgerEntry Top = Ledger.empty() ? ss::life::LedgerEntry() : Ledger.front();
	const bool bNew = Game->bCounterTop && !Ledger.empty()
		&& (Top.At != Game->CounterTop.At || Top.Amount != Game->CounterTop.Amount || Top.Label != Game->CounterTop.Label || Top.Kind != Game->CounterTop.Kind);
	Game->CounterTop = Top;
	Game->bCounterTop = true;
	if (bNew && Top.Kind == 8 && People)
	{
		People->PlayBeat(FName(*KeeperAt(TEXT("store"))), TEXT("ring"), 3.0f);
	}
}

FString AStreetGameMode::KeeperAt(FName Venue, FString* OutName) const
{
	const double World = Game ? Game->Session.WorldMinutes() : 0.0;
	std::string Id = ss::town::OnShift(FStringToUtf8(Venue.ToString()), World);
	if (Id.empty())
	{
		Id = ss::shop::KeeperOf(FStringToUtf8(Venue.ToString()), World);
	}
	if (OutName)
	{
		const ss::town::Person* P = Id.empty() ? nullptr : ss::town::FindPerson(Id);
		*OutName = P && !P->Name.empty() ? Utf8ToFString(P->Name) : FString(KeeperFallback(Venue, World));
	}
	return Id.empty() ? FString(KeeperFallback(Venue, World)).ToLower() : Utf8ToFString(Id);
}

bool AStreetGameMode::OpenTalk(FName Person, FName Venue)
{
	if (!Game || Person.IsNone() || bLeaving)
	{
		return false;
	}
	if (Screen && Screen->IsOpen())
	{
		CloseShop(false);
	}
	const std::string Id = FStringToUtf8(Person.ToString());
	ss::town::Talk Line;
	// The director's person: engaged (they stop, turn and hold the hero's eyes), their opening line said and heard.
	bTalkByDirector = People && People->OpenTalk(Person, Line);
	TalkContext.reset();
	if (!bTalkByDirector)
	{
		// Nobody out by that id (the director hasn't built them yet, Augie's guitar wall from across the room): town's
		// context alone, its line heard all the same.
		TalkContext = ss::town::ContextFor(Game->Session, Id, FStringToUtf8(Venue.ToString()));
		if (TalkContext)
		{
			Line = ss::town::Open(*TalkContext);
			if (!Line.Line.empty())
			{
				Game->Session.Heard(Id, Line);
			}
		}
		const ss::town::Person* P = ss::town::FindPerson(Id);
		if (Line.Name.empty())
		{
			FString Name = P && !P->Name.empty() ? Utf8ToFString(P->Name) : Person.ToString();
			if (Name.Len() > 0 && FChar::IsLower(Name[0]))
			{
				Name[0] = FChar::ToUpper(Name[0]);
			}
			Line.Name = FStringToUtf8(Name);
		}
		if (Line.Line.empty())
		{
			Line.Line = FStringToUtf8(TalkFallback(Person));
		}
	}
	if (Line.Choices.empty())
	{
		Line.Choices.push_back({"bye", "See you."});
	}
	// The card (core-ui's TalkCard) is the one ActiveScreen: keyed and drawn as the shops' pages are, over the street.
	TUniquePtr<ss::ui::TalkCard> Made = MakeUnique<ss::ui::TalkCard>();
	Card = Made.Get();
	Card->Gamepad = bGamepad;
	Card->Open(RealTime, Id);
	Card->Show(RealTime, Line);
	Screen = MoveTemp(Made);
	++ScreenSerial;
	ScreenVenue = TEXT("talk");
	ScreenPage = Person.ToString();
	TalkingTo = Person;
	TalkingAt = Venue;
	FocusWidget(ShopWidget.Get());
	UE_LOG(LogStreet, Log, TEXT("Talking with %s at %s%s"), *Person.ToString(), Venue.IsNone() ? TEXT("the street") : *Venue.ToString(),
		bTalkByDirector ? TEXT("") : TEXT(" (town's context alone)"));
	return true;
}

void AStreetGameMode::AnswerTalk(const FString& Choice)
{
	if (!Card || TalkingTo.IsNone() || !Game)
	{
		return;
	}
	if (Choice == TEXT("service"))
	{
		// "Get a cut", "Menu", "Day pass": the card goes and the person's shop opens on its counter, chair or desk.
		OpenServiceOf(TalkingTo, TalkingAt);
		return;
	}
	const std::string Id = FStringToUtf8(TalkingTo.ToString());
	ss::town::Talk Line;
	if (bTalkByDirector && People)
	{
		People->AnswerTalk(TalkingTo, Choice, Line);
	}
	else if (TalkContext)
	{
		Line = ss::town::Answer(*TalkContext, FStringToUtf8(Choice));
		if (!Line.Line.empty())
		{
			Game->Session.Heard(Id, Line);
		}
	}
	const ss::town::Talk& Was = Card->Current();
	if (Line.Name.empty())
	{
		Line.Name = Was.Name;
	}
	if (Line.Label.empty())
	{
		Line.Label = Was.Label;
	}
	if (Line.Line.empty())
	{
		Line.Line = "...";
	}
	// Their last word (no choices back): only goodbye is left.
	if (Line.Choices.empty())
	{
		Line.Choices.push_back({"bye", "See you."});
	}
	Card->Show(RealTime, Line);
}

void AStreetGameMode::EndTalk()
{
	if (People && bTalkByDirector && !TalkingTo.IsNone())
	{
		People->Release(TalkingTo);
	}
	TalkContext.reset();
	TalkingTo = NAME_None;
	TalkingAt = NAME_None;
	bTalkByDirector = false;
}

void AStreetGameMode::OpenServiceOf(FName Person, FName Venue)
{
	// Their venue: the talk's, else the director's or town's for them.
	FName Where = Venue;
	if (Where.IsNone() && People)
	{
		if (const FStreetPerson* P = People->FindPerson(Person))
		{
			Where = P->Venue;
		}
	}
	if (Where.IsNone())
	{
		if (const ss::town::Person* P = ss::town::FindPerson(FStringToUtf8(Person.ToString())))
		{
			Where = FName(*Utf8ToFString(P->Venue));
		}
	}
	CloseShop(false);
	if (Where == TEXT("store"))
	{
		OpenCounter(0);
		return;
	}
	if (Where == TEXT("barber"))
	{
		// Into the chair (the bench first while somebody's ahead in it).
		if (const UStreetWorld* Registry = StreetWorld())
		{
			if (const FStreetUse* Chair = Registry->FindUse(TEXT("barber-chair1")))
			{
				const FStreetUse Use = *Chair;
				UseSeat(Use);
				return;
			}
		}
	}
	static const TPair<const TCHAR*, const TCHAR*> Pages[] = {
		{TEXT("barber"), TEXT("barber-chair1")}, {TEXT("thrift"), TEXT("thrift-rack-tops")}, {TEXT("gym"), TEXT("gym-desk")},
		{TEXT("diner"), TEXT("")}, {TEXT("pawn"), TEXT("pawn-counter")},
	};
	for (const TPair<const TCHAR*, const TCHAR*>& P : Pages)
	{
		if (Where == P.Key)
		{
			OpenShop(Where, !SeatedAt.IsNone() && Where == TEXT("diner") ? SeatedAt.ToString() : FString(P.Value));
			return;
		}
	}
}

void AStreetGameMode::ShowClosed(const FString& Line)
{
	ClosedNow = Line;
	ClosedNowAt = RealTime;
}

void AStreetGameMode::UpdateClosing()
{
	AShortStackCharacter* Hero = GetHero();
	if (!Game || !Hero)
	{
		return;
	}
	const double World = Game->Session.WorldMinutes();
	for (const TWeakObjectPtr<AShopStage>& S : Shops)
	{
		AShopStage* Shop = S.Get();
		const ss::shop::Venue* V = Shop ? ss::shop::FindVenue(FStringToUtf8(Shop->Venue.ToString())) : nullptr;
		if (!V || V->MembersAnyHour)
		{
			continue;
		}
		const bool bOpenNow = ss::shop::IsOpen(*V, World);
		bool& bWas = WasOpen.FindOrAdd(Shop->Venue, bOpenNow);
		const bool bNear = Shop->IsInside(Hero->GetActorLocation()) || FVector::Dist(Hero->GetActorLocation(), Shop->DoorMiddle()) < ClosingHeard;
		// Ten minutes out, the keeper says so (once a closing; spec 5.0).
		if (bOpenNow && bNear)
		{
			const double Close = ss::shop::ClosesAt(*V, World);
			double& Said = ClosingSaid.FindOrAdd(Shop->Venue, -1.0);
			if (Close > World && Close - World <= ClosingWarn && Said != Close)
			{
				Said = Close;
				FString Name;
				const FString Keeper = KeeperAt(Shop->Venue, &Name);
				std::string Line;
				if (std::shared_ptr<const ss::town::TalkContext> Ctx = ss::town::ContextFor(Game->Session, FStringToUtf8(Keeper), V->Id))
				{
					Line = ss::town::KeeperLine(*Ctx, "closing");
				}
				const FString Word = Line.empty() ? FString(ClosingFallback(Shop->Venue)) : Utf8ToFString(Line);
				if (People)
				{
					People->Speak(People->FindPerson(FName(*Keeper)) ? FName(*Keeper) : FName(*Name), Word);
				}
				else
				{
					Toast(Name, Word);
				}
			}
		}
		// At close: its screen ends, and whoever's still inside is walked out (a fade, then outside the door).
		if (bWas && !bOpenNow)
		{
			if (ScreenVenue == Shop->Venue)
			{
				CloseShop(true);
			}
			if (Shop->IsInside(Hero->GetActorLocation()) && !Cinema.bActive)
			{
				FString Name;
				const FString Keeper = KeeperAt(Shop->Venue, &Name);
				const FTransform Out = Shop->OutsideDoor();
				TWeakObjectPtr<AStreetGameMode> Self(this);
				UnderBlack([Self, Out]() {
					if (Self.IsValid())
					{
						Self->StandUp();
						if (AShortStackCharacter* H = Self->GetHero())
						{
							PlaceAt(*H, Out, false);
						}
					}
				}, 0.6f);
				const FString Word = TEXT("That's it, we're closed. Come back tomorrow.");
				if (People)
				{
					People->Speak(People->FindPerson(FName(*Keeper)) ? FName(*Keeper) : FName(*Name), Word);
				}
				else
				{
					Toast(Name, Word);
				}
			}
		}
		bWas = bOpenNow;
	}
}

// ------------------------------------------------------------------ the controller

AStreetPlayerController::AStreetPlayerController()
{
	bShowMouseCursor = false;
}

void AStreetPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	AStreetGameMode* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<AStreetGameMode>() : nullptr;
	if (!Mode || Mode->IsUiOpen())
	{
		return;
	}
	// Move: WASD or the left stick; Shift or the left shoulder runs.
	float Forward = (IsInputKeyDown(EKeys::W) ? 1.0f : 0.0f) - (IsInputKeyDown(EKeys::S) ? 1.0f : 0.0f) + GetInputAnalogKeyState(EKeys::Gamepad_LeftY);
	float Right = (IsInputKeyDown(EKeys::D) ? 1.0f : 0.0f) - (IsInputKeyDown(EKeys::A) ? 1.0f : 0.0f) + GetInputAnalogKeyState(EKeys::Gamepad_LeftX);
	Forward = FMath::Clamp(Forward, -1.0f, 1.0f);
	Right = FMath::Clamp(Right, -1.0f, 1.0f);
	const bool bRun = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::Gamepad_LeftShoulder) || IsInputKeyDown(EKeys::Gamepad_LeftThumbstick);
	Mode->OnMove(Forward, Right, bRun);
	// Look: the mouse, or the right stick.
	float Mx = 0.0f;
	float My = 0.0f;
	GetInputMouseDelta(Mx, My);
	const float StickYaw = GetInputAnalogKeyState(EKeys::Gamepad_RightX) * 170.0f * DeltaTime;
	const float StickPitch = GetInputAnalogKeyState(EKeys::Gamepad_RightY) * 120.0f * DeltaTime;
	Mode->OnLook(Mx * Mode->Sensitivity * 10.0f + StickYaw, My * Mode->Sensitivity * 10.0f + StickPitch);
	// The HUD's buttons follow the last device used: a gamepad's button or a stick pushed, or the keyboard and mouse.
	static const FKey PadButtons[] = {EKeys::Gamepad_FaceButton_Bottom, EKeys::Gamepad_FaceButton_Right, EKeys::Gamepad_FaceButton_Left, EKeys::Gamepad_FaceButton_Top,
		EKeys::Gamepad_LeftShoulder, EKeys::Gamepad_RightShoulder, EKeys::Gamepad_LeftTrigger, EKeys::Gamepad_RightTrigger, EKeys::Gamepad_DPad_Up, EKeys::Gamepad_DPad_Down,
		EKeys::Gamepad_DPad_Left, EKeys::Gamepad_DPad_Right, EKeys::Gamepad_Special_Left, EKeys::Gamepad_Special_Right, EKeys::Gamepad_LeftThumbstick, EKeys::Gamepad_RightThumbstick};
	static const FKey DeskKeys[] = {EKeys::W, EKeys::A, EKeys::S, EKeys::D, EKeys::E, EKeys::F, EKeys::V, EKeys::P, EKeys::Escape, EKeys::LeftShift, EKeys::LeftMouseButton};
	bool bPad = FMath::Abs(GetInputAnalogKeyState(EKeys::Gamepad_LeftX)) > 0.25f || FMath::Abs(GetInputAnalogKeyState(EKeys::Gamepad_LeftY)) > 0.25f ||
		FMath::Abs(GetInputAnalogKeyState(EKeys::Gamepad_RightX)) > 0.25f || FMath::Abs(GetInputAnalogKeyState(EKeys::Gamepad_RightY)) > 0.25f;
	for (const FKey& Key : PadButtons)
	{
		bPad = bPad || WasInputKeyJustPressed(Key);
	}
	bool bDesk = FMath::Abs(Mx) + FMath::Abs(My) > 0.5f;
	for (const FKey& Key : DeskKeys)
	{
		bDesk = bDesk || WasInputKeyJustPressed(Key);
	}
	if (bPad)
	{
		Mode->SetGamepad(true);
	}
	else if (bDesk)
	{
		Mode->SetGamepad(false);
	}
	if (WasInputKeyJustPressed(EKeys::V) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Top))
	{
		Mode->OnToggleView();
	}
	if (WasInputKeyJustPressed(EKeys::E) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom))
	{
		Mode->OnInteract();
	}
	if (WasInputKeyJustPressed(EKeys::F) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Left))
	{
		Mode->OnEat();
	}
	if (WasInputKeyJustPressed(EKeys::MouseScrollUp))
	{
		Mode->OnZoom(-30.0f);
	}
	if (WasInputKeyJustPressed(EKeys::MouseScrollDown))
	{
		Mode->OnZoom(30.0f);
	}
	// Pause. In Play-In-Editor, Escape stops the session before the game sees it, so P works too. On a gamepad Start
	// or B (the HUD shows B: it's free out here).
	if (WasInputKeyJustPressed(EKeys::Escape) || WasInputKeyJustPressed(EKeys::P) || WasInputKeyJustPressed(EKeys::Gamepad_Special_Right) ||
		WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Right))
	{
		Mode->OnPause();
	}
}
