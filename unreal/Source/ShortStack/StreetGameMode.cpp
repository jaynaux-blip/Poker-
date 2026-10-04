#include "StreetGameMode.h"

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
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
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Store.h"
#include "ShortStackCharacter.h"
#include "StreetStage.h"

#include <algorithm>
#include <string>
#include <vector>

DEFINE_LOG_CATEGORY_STATIC(LogStreet, Log, All);

namespace StreetGameDetail
{
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
	FindOrSpawnStage();
	const FTransform Start = Stage ? Stage->StartAt(StartFrom) : FTransform(FRotator(0.0f, 65.0f, 0.0f), FVector(150.0, 80.0, 100.0));
	RestartPlayerAtTransform(NewPlayer, Start);
	DressHero();
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

void AStreetGameMode::SpawnClerk()
{
	if (!Stage || Clerk || !GetWorld())
	{
		return;
	}
	const FTransform Spot = Stage->ClerkSpot();
	// Dressed before he begins play, so he isn't built as the default hero first and then thrown away for Benny.
	Clerk = GetWorld()->SpawnActorDeferred<AShortStackCharacter>(AShortStackCharacter::StaticClass(), Spot, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Clerk)
	{
		return;
	}
	// Benny, nights, in the store's red polo.
	Clerk->AutoPossessAI = EAutoPossessAI::Disabled;
	Clerk->ApplyCast(TEXT("ExtraB"), FLinearColor::FromSRGBColor(FColor(0xd7, 0x26, 0x3d)));
	Clerk->FinishSpawning(Spot);
	// His feet on the store's floor (nothing drops him there: with no controller his movement never runs).
	FVector At = Clerk->GetActorLocation();
	At.Z = Stage->StoreFloorZ() + Clerk->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	Clerk->SetActorLocation(At, false, nullptr, ETeleportType::TeleportPhysics);
	UE_LOG(LogStreet, Log, TEXT("Benny behind the counter: body %s, face %s, %.0f cm"), *GetNameSafe(Clerk->GetMesh()->GetSkeletalMeshAsset()),
		*GetNameSafe(Clerk->Face ? Clerk->Face->GetSkeletalMeshAsset() : nullptr), Clerk->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2.0f);
	// He stands behind the counter: no movement to simulate, no camera of his own; his pose only while he's seen (the
	// character does as much for a cast member itself; this keeps him so whichever way he was dressed).
	if (UCharacterMovementComponent* Move = Clerk->GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->SetComponentTickEnabled(false);
	}
	if (Clerk->Boom)
	{
		Clerk->Boom->bDoCollisionTest = false;
		Clerk->Boom->SetComponentTickEnabled(false);
	}
	if (Clerk->Camera)
	{
		Clerk->Camera->Deactivate();
	}
	Clerk->GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	if (Clerk->Face)
	{
		Clerk->Face->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	}
}

void AStreetGameMode::BeginPlay()
{
	Super::BeginPlay();
	FindOrSpawnStage();
	DressHero();
	SpawnClerk();
	CreateWidgets();
	ReturnInputToGame();
	// The player's settings, as at the desk (the stage's lens and the hero's field of view are here now).
	ApplySettings(Game->Menu.Settings, false);
	if (Audio)
	{
		Audio->StartAmbience();
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
	if (Game && !bGoingHome)
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
	if (!IsUiOpen() && !bGoingHome && Stage)
	{
		if (const FStreetSpot* Spot = Stage->SpotFor(Hero->EyeLocation(), Hero->LookDirection()))
		{
			Info.Prompt = FStringToUtf8(Spot->Prompt);
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
		// The street's hour: the sky, the lamps' photocells, the windows and the exposure follow the clock.
		Stage->SetDaylight(static_cast<float>(Game->Session.Daylight()));
	}
	AShortStackCharacter* Hero = GetHero();
	if (Hero && Stage)
	{
		KeepOnTheSet();
		if (Stage->UpdateDoors(Hero->GetActorLocation(), static_cast<float>(Dt)) && Audio)
		{
			// The doors' chime as they part (the store's two notes, kept small).
			Audio->PlayEffect(ss::audio::Effect::Chime, 0.22f);
		}
		float Volume = 0.0f;
		if (Hero->TakeFootstep(Volume) && Audio)
		{
			Audio->PlayEffect(ss::audio::Effect::Step, Volume * (Stage->IsInsideStore(Hero->GetActorLocation()) ? 0.7f : 1.0f));
		}
		// Behind the store's glass the rain is quieter.
		const float Duck = FMath::Lerp(1.0f, 0.45f, Stage->CameraInside());
		if (Audio && FMath::Abs(Duck - AmbienceDuck) > 0.02f)
		{
			AmbienceDuck = Duck;
			const ss::ui::GameSettings& S = Game->Menu.Settings;
			Audio->SetMix(S.MasterVolume / 100.0f, S.EffectsVolume / 100.0f, S.AmbienceVolume / 100.0f * AmbienceDuck);
		}
		// Worn out shows in the walk: from 40 energy down the shoulders drop and the trunk sags.
		Hero->SetTired(static_cast<float>((40.0 - Game->Session.Life.Energy) / 40.0));
	}
	else if (!Hero && !bGoingHome && GetWorld())
	{
		// The pawn is gone (killed below the world's floor before KeepOnTheSet could catch it): a new one at the door.
		if (APlayerController* Pc = GetWorld()->GetFirstPlayerController(); Pc && !Pc->GetPawn())
		{
			UE_LOG(LogStreet, Warning, TEXT("The hero was lost: back at the building's door"));
			bDressed = false;
			StartFrom = TEXT("home");
			RestartPlayer(Pc);
			ApplySettings(Game->Menu.Settings, false);
		}
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
	if (bGoingHome && RealTime >= HomeAt)
	{
		bGoingHome = false;
		// The clock to the thousandth of a minute: rounded, a deadline at the stroke of midnight could be skipped.
		UGameplayStatics::OpenLevel(this, FName(TEXT("NightOne")), true, FString::Printf(TEXT("Home?Street?From=%.3f"), Game->Session.WorldMinutes()));
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
	if (!Hero || !Stage || bGoingHome || !Stage->IsOffTheSet(Hero->GetActorLocation()))
	{
		return;
	}
	UE_LOG(LogStreet, Warning, TEXT("Off the set at %s: back to the building's door"), *Hero->GetActorLocation().ToCompactString());
	const FTransform T = Stage->StartAt(TEXT("home"));
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

// ------------------------------------------------------------------ actions

void AStreetGameMode::OnMove(float Forward, float Right, bool bRun)
{
	if (AShortStackCharacter* Hero = GetHero(); Hero && !bGoingHome)
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
	if (!Hero || !Stage || bGoingHome || IsUiOpen())
	{
		return;
	}
	const FStreetSpot* Spot = Stage->SpotFor(Hero->EyeLocation(), Hero->LookDirection());
	if (!Spot)
	{
		return;
	}
	if (Spot->Id == TEXT("home"))
	{
		GoHome();
	}
	else
	{
		OpenCounter(Spot->Shelf);
	}
}

void AStreetGameMode::OnEat()
{
	if (!Game || bGoingHome)
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
	if (Game->Counter.IsOpen())
	{
		return;
	}
	Game->Menu.Info = ss::ui::DescribeSession(Game->Session, true);
	Game->Menu.Info.Resolutions = StreetResolutions();
	Game->Menu.Gamepad = bGamepad;
	const bool bInStore = Stage && GetHero() && Stage->IsInsideStore(GetHero()->GetActorLocation());
	Game->Menu.SetVenue(ss::ui::Venue::Street, bInStore ? std::string("LUCKY PENNY #212") : std::string());
	Game->Menu.Open(ss::ui::FrontEnd::Page::Pause, RealTime);
	FocusWidget(MenuWidget.Get());
}

void AStreetGameMode::GoHome()
{
	if (bGoingHome)
	{
		return;
	}
	bGoingHome = true;
	HomeAt = RealTime + 1.4;
	// A minute up the stairs, then the session's save carries the street back to the desk.
	Game->Session.Save();
	if (APlayerController* Pc = GetWorld()->GetFirstPlayerController())
	{
		if (Pc->PlayerCameraManager)
		{
			Pc->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, 1.2f, FLinearColor::Black, true, true);
		}
	}
	if (Audio)
	{
		Audio->PlayEffect(ss::audio::Effect::Thump, 0.7f);
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
		Hero->FirstPersonFov = StreetFov(84.0f, Settings.FieldOfView);
		Hero->ThirdPersonFov = StreetFov(72.0f, Settings.FieldOfView);
	}
	if (Audio)
	{
		Audio->SetMix(Settings.MasterVolume / 100.0f, Settings.EffectsVolume / 100.0f, Settings.AmbienceVolume / 100.0f * AmbienceDuck);
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

// ------------------------------------------------------------------ automation

FString AStreetGameMode::TestDescribe() const
{
	const AShortStackCharacter* Hero = GetHero();
	if (!Game || !Hero)
	{
		return TEXT("no game");
	}
	const ss::life::State& L = Game->Session.Life;
	const FStreetSpot* Spot = Stage ? Stage->SpotFor(Hero->EyeLocation(), Hero->LookDirection()) : nullptr;
	return FString::Printf(TEXT("%s at %s %s | %s view | spot %s | hunger %.0f thirst %.0f energy %.0f | $%.2f | bag %d | counter %s | inside %.2f | daylight %.2f"),
		*Utf8ToFString(ss::net::TimeLabel(Game->Session.WorldMinutes())), *Hero->GetActorLocation().ToCompactString(), Stage ? *Stage->PlaceName(Hero->GetActorLocation()) : TEXT("?"),
		Hero->IsFirstPerson() ? TEXT("first") : TEXT("third"), Spot ? *Spot->Id.ToString() : TEXT("none"), L.Hunger, L.Thirst, L.Energy, Game->Session.BankrollCents / 100.0,
		static_cast<int32>(L.Pantry.size()), Game->Counter.IsOpen() ? TEXT("open") : TEXT("closed"), Stage ? Stage->CameraInside() : 0.0f, Game->Session.Daylight());
}

void AStreetGameMode::TestTeleport(const FString& Where)
{
	AShortStackCharacter* Hero = GetHero();
	if (!Hero || !Stage)
	{
		return;
	}
	const FTransform T = Where == TEXT("counter") ? FTransform(FRotator(0.0f, -90.0f, 0.0f), FVector(-330.0, 4180.0, 100.0)) * Stage->GetActorTransform() : Stage->StartAt(Where);
	Hero->SetActorLocation(T.GetLocation(), false, nullptr, ETeleportType::TeleportPhysics);
	if (AController* C = Hero->GetController())
	{
		C->SetControlRotation(T.Rotator());
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
