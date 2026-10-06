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
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Store.h"
#include "ShopStage.h"
#include "ShortStackCharacter.h"
#include "StreetAudio.h"
#include "StreetStage.h"
#include "StreetWorld.h"

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
	for (const TWeakObjectPtr<AShopStage>& S : Shops)
	{
		AShopStage* Shop = S.Get();
		if (!Shop)
		{
			continue;
		}
		// The hour and the light (whether it's open and staffed comes with core's shop hours; until then as it was set).
		Shop->SetHour(Game->Session.WorldMinutes(), static_cast<float>(Game->Session.Daylight()), Shop->IsOpen(), Shop->IsStaffed());
		if (!Hero)
		{
			continue;
		}
		// Its door: its bell as it swings open, and again, softer, as its closer brings it shut (the buzzer's clack on the pawn's).
		const int32 Door = Shop->UpdateDoor(Hero->GetActorLocation(), Dt);
		if (Door != 0 && StreetTone)
		{
			const FStreetDoor* D = nullptr;
			const UStreetWorld* Registry = StreetWorld();
			const TArray<FStreetDoor> All = Registry ? Registry->Doors() : TArray<FStreetDoor>();
			for (const FStreetDoor& E : All)
			{
				D = E.Id == Shop->Venue ? &E : D;
			}
			const float Heard = D ? HeardFrom(Ear, D->Threshold) : 1.0f;
			if (Heard > 0.01f)
			{
				StreetTone->RingBell((Door == 1 ? 1.0f : 0.4f) * Heard);
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
	Hero->ApplyLook(Game->Session.Person);
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
	if (AStreetCrowd* People = GetWorld()->SpawnActor<AStreetCrowd>())
	{
		People->Begin(Stage, Hero);
		Crowd = People;
	}
#endif
}

void AStreetGameMode::BeginPlay()
{
	Super::BeginPlay();
	// A test's overrides from the last Play session don't carry into this one (ss.Street.KeepOverrides 1 keeps them).
	FStreetOverrides::ResetAll();
	FindOrSpawnStage();
	if (Stage)
	{
		Stage->EnsureBuilt();
	}
	DressHero();
	SpawnClerk();
	SpawnAttendant();
	SpawnCrowd();
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
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		for (TSharedPtr<SWidget> W : {StaticCastSharedPtr<SWidget>(HudWidget), StaticCastSharedPtr<SWidget>(CounterWidget), StaticCastSharedPtr<SWidget>(MenuWidget)})
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
	return Game && (Game->Counter.IsOpen() || Game->Menu.IsOpen());
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
	if (!IsUiOpen() && !bLeaving && Stage)
	{
		if (const FStreetUse* Use = UseInReach())
		{
			Info.Prompt = FStringToUtf8(PromptFor(*Use));
		}
	}
	const FVector2D Size = ViewSize();
	const float H = ss::ui::StoreCounter::Height;
	TSharedPtr<ss::ui::DrawList> List = MakeShared<ss::ui::DrawList>();
	ss::ui::Canvas Cv(*List, Game->Measurer, H * static_cast<float>(Size.X / Size.Y), H, static_cast<float>(Size.Y) / H);
	if (!Game->Counter.IsOpen())
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
			bDressed = false;
			StartFrom = HomeDoor();
			ProfileZone = NAME_None;
			ProfileBlend = 1.0f;
			bProfileFirstPerson = false;
			RestartPlayer(Pc);
			ApplySettings(Game->Menu.Settings, false);
		}
	}
	// The enterable shops: the hour, their doors.
	UpdateShops(Ear, static_cast<float>(Dt));
#if SHORTSTACK_STREET_CROWD
	// The pedestrians: started once the hero stands (then told the hour, the rain, and whether the game's paused).
	if (!bCrowdStarted)
	{
		SpawnCrowd();
	}
	if (AStreetCrowd* People = Cast<AStreetCrowd>(Crowd); People && Stage)
	{
		People->SetHour(static_cast<float>(Game->Session.Daylight()), Game->Session.WorldMinutes(), Stage->RainStrength(), Game->Menu.IsPaused());
		// A word from someone the player nearly walked into, heard if they're still close.
		FVector BarkAt;
		FString Bark;
		while (People->TakeBark(BarkAt, Bark))
		{
			if (Hero && FVector::DistSquared(BarkAt, Hero->GetActorLocation()) < 700.0 * 700.0)
			{
				Toast(TEXT("Passerby"), Bark);
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
	// Texts don't age while the counter has the screen (the HUD that shows them is hidden then); otherwise ten seconds.
	if (Game->Counter.IsOpen())
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
	if (Use.Verb == EStreetVerb::Door)
	{
		// A shop's door, open: walk on through (as the Wash & Fold's). From inside it's a door out, whatever its lock.
		if (const AShopStage* Shop = ShopFor(Use.Venue))
		{
			if (Shop->DoorOpenAmount() > 0.5f)
			{
				return FString();
			}
			if (const AShortStackCharacter* Hero = GetHero(); Hero && Shop->IsInside(Hero->GetActorLocation()))
			{
				return TEXT("Open the door");
			}
		}
	}
	return UStreetWorld::PromptOf(Use);
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
	case EStreetVerb::Sit:
	case EStreetVerb::Wait:
	case EStreetVerb::Board:
	case EStreetVerb::Wardrobe:
		// The shop screens, the seats, the shelter's wait, the 14 and the closet land with the shops (Phase B).
		NotYet(Use);
		break;
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
	// Open: it swings in (or the keeper buzzes it); shut: the handle tried, and when it opens again. The gym's fob comes
	// with the shops' session calls (until then the desk buzzes you in while it's staffed).
	const FString Why = Shop->OpenDoor(false);
	if (!Why.IsEmpty())
	{
		if (StreetTone)
		{
			StreetTone->TryHandle();
		}
		Toast(Shop->VenueName, Why);
	}
}

void AStreetGameMode::TalkTo(const FStreetUse& Use)
{
	if (Use.Param == TEXT("attendant"))
	{
		TalkToAttendant();
		return;
	}
	// The people's talk card (AStreetNpcs, the TalkCard) comes with them.
	NotYet(Use);
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
