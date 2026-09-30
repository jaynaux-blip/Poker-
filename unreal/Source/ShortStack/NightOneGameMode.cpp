#include "NightOneGameMode.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/DateTime.h"
#include "NightOneAudio.h"
#include "NightOneGame.h"
#include "NightOnePawn.h"
#include "NightOnePlayerController.h"
#include "NightOneSaveGame.h"
#include "NightOneStage.h"
#include "SFrontEndWidget.h"
#include "SNightOneOverlay.h"
#include "ShortStack.h"
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/SOverlay.h"

#include <algorithm>

namespace NightOneModeDetail
{
const TCHAR* const SettingsSlot = TEXT("Settings");

double SmoothStep(double A, double B, double X)
{
	const double T = FMath::Clamp((X - A) / (B - A), 0.0, 1.0);
	return T * T * (3.0 - 2.0 * T);
}

FString SanitizeName(const FString& In)
{
	FString Out;
	for (const TCHAR Ch : In)
	{
		if (FChar::IsAlnum(Ch) || Ch == TEXT('_') || Ch == TEXT('.') || Ch == TEXT('-'))
		{
			Out.AppendChar(Ch);
		}
	}
	return Out.Left(16);
}

/** "2560 x 1440" to (2560, 1440); (0, 0) when empty or malformed. */
FIntPoint ParseResolution(const std::string& Text)
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

/** Fullscreen resolutions the display supports, largest first, as "W x H". */
std::vector<std::string> SupportedResolutions()
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

void SetConsoleInt(const TCHAR* Name, int32 Value)
{
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name))
	{
		Var->Set(Value, ECVF_SetByGameSetting);
	}
}
} // namespace NightOneModeDetail

using namespace NightOneModeDetail;

ANightOneGameMode::ANightOneGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	DefaultPawnClass = ANightOnePawn::StaticClass();
	PlayerControllerClass = ANightOnePlayerController::StaticClass();
	Audio = CreateDefaultSubobject<UNightOneAudio>(TEXT("Audio"));
}

void ANightOneGameMode::RestartPlayer(AController* NewPlayer)
{
	if (!NewPlayer || NewPlayer->IsPendingKillPending())
	{
		return;
	}
	// No PlayerStart needed: the seat is positioned by the stage every frame.
	RestartPlayerAtTransform(NewPlayer, FTransform(FRotator::ZeroRotator, FVector(-14.0, 0.0, 117.0)));
}

ANightOnePawn* ANightOneGameMode::GetSeat() const
{
	return Cast<ANightOnePawn>(UGameplayStatics::GetPlayerPawn(this, 0));
}

bool ANightOneGameMode::IsMenuOpen() const
{
	return Game && Game->Menu.IsOpen();
}

void ANightOneGameMode::StartPlay()
{
	Super::StartPlay();
	UWorld* World = GetWorld();
	for (TActorIterator<ANightOneStage> It(World); It; ++It)
	{
		Stage = *It;
		break;
	}
	if (!Stage)
	{
		Stage = World->SpawnActor<ANightOneStage>(ANightOneStage::StaticClass(), FTransform::Identity);
	}
	if (Stage)
	{
		TWeakObjectPtr<UNightOneAudio> WeakAudio = Audio.Get();
		Stage->OnThunder = [WeakAudio](float Delay, float Strength) {
			if (WeakAudio.IsValid())
			{
				WeakAudio->Thunder(Delay, Strength);
			}
		};
	}

	// Progress from the last session.
	ss::SaveData Loaded;
	bool bLoaded = false;
	if (UGameplayStatics::DoesSaveGameExist(UNightOneSaveGame::SlotName(), 0))
	{
		if (UNightOneSaveGame* SaveObject = Cast<UNightOneSaveGame>(UGameplayStatics::LoadGameFromSlot(UNightOneSaveGame::SlotName(), 0)))
		{
			bLoaded = ss::SaveData::Parse(std::string(TCHAR_TO_UTF8(*SaveObject->Data)), Loaded);
		}
	}
	bHasSave = bLoaded;
	const std::string Seed = std::string(TCHAR_TO_UTF8(*FString::Printf(TEXT("%lld"), FDateTime::Now().GetTicks())));
	Game = MakeUnique<FNightOneGame>(*this, bLoaded ? &Loaded : nullptr, Seed);

	// Settings from the last session (defaults on the first run).
	ss::ui::GameSettings SavedSettings;
	if (UGameplayStatics::DoesSaveGameExist(SettingsSlot, 0))
	{
		if (UNightOneSaveGame* SettingsObject = Cast<UNightOneSaveGame>(UGameplayStatics::LoadGameFromSlot(SettingsSlot, 0)))
		{
			ss::ui::GameSettings::Parse(std::string(TCHAR_TO_UTF8(*SettingsObject->Data)), SavedSettings);
		}
	}
	Game->Menu.Settings = SavedSettings;
	Game->Menu.Info = ss::ui::DescribeSession(Game->Session, bHasSave);
	Game->Menu.Info.Resolutions = SupportedResolutions();
	Game->Menu.ScreenName = Game->Session.HeroName;

	if (ANightOnePawn* Seat = GetSeat())
	{
		Seat->Focus = 0.0f;
		Seat->TargetFocus = 0.0f;
		Seat->Yaw = 0.12f;
		Seat->Pitch = 0.05f;
	}
	ApplySettings(SavedSettings, false);
	if (Audio)
	{
		Audio->StartAmbience(); // rain under the title screen
	}

	CreateViewportWidgets();
	if (MenuWidget)
	{
		// Quitting to the menu reloads the level with "?Menu": skip the title screen then.
		const bool bToMenu = UGameplayStatics::HasOption(OptionsString, TEXT("Menu"));
		Game->Menu.Open(bToMenu ? ss::ui::FrontEnd::Page::Main : ss::ui::FrontEnd::Page::Attract, RealTime);
		FocusMenu();
	}
	else
	{
		Begin(FString());
	}
}

void ANightOneGameMode::CreateViewportWidgets()
{
	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (!Viewport || !FSlateApplication::IsInitialized())
	{
		return;
	}
	Overlay = SNew(SNightOneOverlay);
	Viewport->AddViewportWidgetContent(Overlay.ToSharedRef(), 10);

	MenuWidget = SNew(SFrontEndWidget);
	TWeakObjectPtr<ANightOneGameMode> WeakThis = this;
	MenuWidget->OnMenuKey = [WeakThis](const FString& KeyName, bool bFromGamepad) {
		if (WeakThis.IsValid() && WeakThis->Game)
		{
			WeakThis->Game->Menu.Gamepad = bFromGamepad;
			WeakThis->Game->Menu.Key(std::string(TCHAR_TO_UTF8(*KeyName)), WeakThis->RealTime);
		}
	};
	MenuWidget->OnMenuChar = [WeakThis](uint32 Codepoint) {
		if (WeakThis.IsValid() && WeakThis->Game)
		{
			WeakThis->Game->Menu.Char(Codepoint, WeakThis->RealTime);
		}
	};
	MenuWidget->OnMenuPointer = [WeakThis](const FVector2D& Logical, int32 Button, float WheelDelta) {
		if (!WeakThis.IsValid() || !WeakThis->Game)
		{
			return;
		}
		ss::ui::Pointer& P = WeakThis->Game->Menu.Ptr;
		WeakThis->Game->Menu.Gamepad = false;
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
	SAssignNew(MenuRoot, SOverlay)
	+ SOverlay::Slot()
	[
		SAssignNew(MenuBlur, SBackgroundBlur)
		.BlurStrength(0.0f)
		.Visibility(EVisibility::Collapsed)
	]
	+ SOverlay::Slot()
	[
		MenuWidget.ToSharedRef()
	];
	Viewport->AddViewportWidgetContent(MenuRoot.ToSharedRef(), 20);
}

void ANightOneGameMode::FocusMenu()
{
	if (!MenuWidget)
	{
		return;
	}
	MenuWidget->SetVisibility(EVisibility::Visible);
	if (APlayerController* Pc = UGameplayStatics::GetPlayerController(this, 0))
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(MenuWidget);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Pc->SetInputMode(InputMode);
		Pc->SetShowMouseCursor(true);
	}
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetKeyboardFocus(MenuWidget, EFocusCause::SetDirectly);
	}
}

void ANightOneGameMode::ReturnInputToGame()
{
	if (MenuWidget)
	{
		// Still painted while the menu fades out, but no longer takes input.
		MenuWidget->SetVisibility(EVisibility::HitTestInvisible);
	}
	if (APlayerController* Pc = UGameplayStatics::GetPlayerController(this, 0))
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		Pc->SetInputMode(InputMode);
		Pc->SetShowMouseCursor(true);
	}
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
	}
}

// ------------------------------------------------------------------ menu actions

void ANightOneGameMode::ContinueCareer()
{
	if (Game)
	{
		Begin(FString(UTF8_TO_TCHAR(Game->Session.HeroName.c_str())));
	}
}

void ANightOneGameMode::StartNewCareer(const FString& ScreenName)
{
	if (Game)
	{
		Game->Session.ResetSave();
		Begin(ScreenName);
	}
}

void ANightOneGameMode::ResumePlay()
{
	ReturnInputToGame();
}

void ANightOneGameMode::QuitToMainMenu()
{
	if (Game && bStarted)
	{
		Game->Session.Save();
	}
	SaveSettingsNow();
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)), true, TEXT("Menu"));
}

void ANightOneGameMode::QuitToDesktop()
{
	if (Game && bStarted)
	{
		Game->Session.Save();
	}
	SaveSettingsNow();
	UKismetSystemLibrary::QuitGame(this, UGameplayStatics::GetPlayerController(this, 0), EQuitPreference::Quit, false);
}

void ANightOneGameMode::OpenPauseMenu()
{
	if (!bStarted || !Game || !MenuWidget || Game->Menu.IsOpen())
	{
		return;
	}
	Game->Menu.Info = ss::ui::DescribeSession(Game->Session, true);
	Game->Menu.Info.Resolutions = SupportedResolutions();
	Game->Menu.Open(ss::ui::FrontEnd::Page::Pause, RealTime);
	FocusMenu();
}

void ANightOneGameMode::ApplySettings(const ss::ui::GameSettings& NewSettings, bool bSave)
{
	if (UGameUserSettings* User = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		User->SetOverallScalabilityLevel(FMath::Clamp(NewSettings.Quality, 0, 4));
		User->SetResolutionScaleValueEx(static_cast<float>(NewSettings.ResolutionScale));
		User->SetFrameRateLimit(static_cast<float>(NewSettings.FrameRateLimit));
		User->SetVSyncEnabled(NewSettings.VSync);
		if (GIsEditor)
		{
			// Play-In-Editor shares the editor's window: leave its size and mode alone.
			User->ApplyNonResolutionSettings();
		}
		else
		{
			const FIntPoint Res = ParseResolution(NewSettings.Resolution);
			User->SetScreenResolution(Res.X > 0 ? Res : User->GetDesktopResolution());
			User->SetFullscreenMode(NewSettings.WindowMode == 0 ? EWindowMode::Fullscreen : NewSettings.WindowMode == 1 ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed);
			User->ApplySettings(false);
		}
	}
	// Hardware ray-traced Lumen (the project enables ray tracing support); hit lighting for reflections at Cinematic.
	SetConsoleInt(TEXT("r.Lumen.HardwareRayTracing"), NewSettings.RayTracing ? 1 : 0);
	SetConsoleInt(TEXT("r.Lumen.HardwareRayTracing.LightingMode"), NewSettings.RayTracing && NewSettings.Quality >= 4 ? 2 : 0);
	if (Stage)
	{
		Stage->ExposureBias = static_cast<float>(NewSettings.Brightness - 50) / 50.0f * 1.5f;
		Stage->bMotionBlur = NewSettings.MotionBlur;
		Stage->GrainScale = NewSettings.FilmGrain == 0 ? 0.0f : NewSettings.FilmGrain == 1 ? 1.0f : 2.2f;
		Stage->FringeScale = NewSettings.ChromaticAberration ? 1.0f : 0.0f;
	}
	if (ANightOnePawn* Seat = GetSeat())
	{
		Seat->VerticalFov = static_cast<float>(NewSettings.FieldOfView);
	}
	if (Audio)
	{
		Audio->SetMix(static_cast<float>(NewSettings.MasterVolume) / 100.0f, static_cast<float>(NewSettings.EffectsVolume) / 100.0f, static_cast<float>(NewSettings.AmbienceVolume) / 100.0f);
	}
	LookSensitivity = static_cast<float>(NewSettings.LookSensitivity) / 100.0f;
	bInvertLook = NewSettings.InvertLook;
	bShowHints = NewSettings.ShowHints;
	if (bSave)
	{
		SettingsDirtyAt = RealTime; // written once the player stops changing things
	}
}

void ANightOneGameMode::SaveSettingsNow()
{
	SettingsDirtyAt = -1.0;
	if (!Game)
	{
		return;
	}
	if (UNightOneSaveGame* SettingsObject = Cast<UNightOneSaveGame>(UGameplayStatics::CreateSaveGameObject(UNightOneSaveGame::StaticClass())))
	{
		SettingsObject->Data = FString(UTF8_TO_TCHAR(Game->Menu.Settings.Serialize().c_str()));
		UGameplayStatics::SaveGameToSlot(SettingsObject, SettingsSlot, 0);
	}
}

void ANightOneGameMode::Begin(const FString& Name)
{
	if (bStarted || !Game)
	{
		return;
	}
	bStarted = true;
	bHasSave = true;
	BeganAt = RealTime;
	const FString Clean = SanitizeName(Name);
	if (Clean.Len() >= 3)
	{
		Game->Session.HeroName = std::string(TCHAR_TO_UTF8(*Clean));
	}
	Game->Session.Save();
	ReturnInputToGame();
	UE_LOG(LogNightOne, Log, TEXT("Night One started as %s"), *Clean);
}

void ANightOneGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (SettingsDirtyAt >= 0.0)
	{
		SaveSettingsNow();
	}
	if (UWorld* World = GetWorld())
	{
		if (UGameViewportClient* Viewport = World->GetGameViewport())
		{
			if (Overlay)
			{
				Viewport->RemoveViewportWidgetContent(Overlay.ToSharedRef());
			}
			if (MenuRoot)
			{
				Viewport->RemoveViewportWidgetContent(MenuRoot.ToSharedRef());
			}
		}
	}
	Overlay.Reset();
	MenuRoot.Reset();
	MenuBlur.Reset();
	MenuWidget.Reset();
	Game.Reset();
	Super::EndPlay(EndPlayReason);
}

void ANightOneGameMode::ShowToast(const FString& From, const FString& Body)
{
	if (Overlay)
	{
		Overlay->ShowToast(From, Body);
	}
}

void ANightOneGameMode::DrawMenu()
{
	if (!Game || !MenuWidget)
	{
		return;
	}
	FVector2D ViewSize(1920.0, 1080.0);
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		FVector2D Measured;
		Viewport->GetViewportSize(Measured);
		if (Measured.X > 0.0 && Measured.Y > 0.0)
		{
			ViewSize = Measured;
		}
	}
	const float LogicalH = ss::ui::FrontEnd::Height;
	const float LogicalW = LogicalH * static_cast<float>(ViewSize.X / ViewSize.Y);
	TSharedPtr<ss::ui::DrawList> List = MakeShared<ss::ui::DrawList>();
	ss::ui::Canvas Cv(*List, Game->Measurer, LogicalW, LogicalH, static_cast<float>(ViewSize.Y) / LogicalH);
	Game->Menu.Draw(Cv, RealTime);
	MenuWidget->SetDrawList(List);
	if (MenuBlur)
	{
		const float Blur = Game->Menu.Backdrop(RealTime);
		MenuBlur->SetBlurStrength(Blur * 14.0f);
		MenuBlur->SetVisibility(Blur > 0.001f ? EVisibility::HitTestInvisible : EVisibility::Collapsed);
	}
}

void ANightOneGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Game || !Stage)
	{
		return;
	}
	const double Dt = FMath::Min(0.05, FMath::Max(0.0, static_cast<double>(DeltaSeconds)));
	RealTime += Dt;
	Game->RealNow = RealTime;
	ANightOnePawn* Seat = GetSeat();
	if (Seat && !Seat->IsConfigured())
	{
		Seat->Configure(Stage->EyeLocation(), Stage->ScreenCenter(), Stage->ScreenNormal(), Stage->ScreenSize());
	}

	DrawMenu();
	const bool bPaused = Game->Menu.IsPaused();

	// After Begin: lean in to the laptop and show the controls hint.
	if (bStarted && BeganAt >= 0.0 && RealTime - BeganAt > 2.2)
	{
		BeganAt = -1.0;
		if (Seat)
		{
			Seat->TargetFocus = 1.0f;
		}
		if (Overlay && bShowHints)
		{
			Overlay->ShowHint();
		}
	}

	const bool bBeat = Audio && Audio->ConsumeBeat();
	ss::Session& S = Game->Session;
	if (!bPaused)
	{
		GameTime += Dt * TimeScale;
		S.Update(GameTime);
	}

	// The client at up to 30 fps, at the resolution of the laptop screen.
	UiAccum += Dt;
	if (UiAccum >= 1.0 / 30.0)
	{
		UiAccum = 0.0;
		TSharedPtr<ss::ui::DrawList> List = MakeShared<ss::ui::DrawList>();
		ss::ui::Canvas Cv(*List, Game->Measurer, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, static_cast<float>(Stage->ScreenResolution.X) / ss::ui::RiverLine::Width);
		Game->Client.Draw(Cv, GameTime);
		Stage->SetScreenDrawList(List);
	}
	if (Game->Client.LeanBackRequested)
	{
		Game->Client.LeanBackRequested = false;
		if (Seat)
		{
			Seat->ToggleLean();
		}
	}

	// The title screen frames the room with a slow establishing shot; starting play flies the camera to the seat.
	if (Seat)
	{
		FVector ShotPos;
		FVector ShotLook;
		Stage->MenuShot(RealTime, ShotPos, ShotLook);
		Seat->SetEstablishingShot(ShotPos, ShotLook, Game->Menu.WantsEstablishingShot());
	}

	// The world reacts to the game.
	const double Clock = S.ClockMinutes();
	Stage->SetDawn(static_cast<float>(SmoothStep(4.6 * 60.0, 6.3 * 60.0, Clock)));
	const float PhoneLevel = static_cast<float>(Game->Phone.Brightness(RealTime));
	Stage->SetPhoneBrightness(PhoneLevel);
	PhoneAccum += Dt;
	if (PhoneLevel > 0.0f && PhoneAccum >= 0.1)
	{
		PhoneAccum = 0.0;
		TSharedPtr<ss::ui::DrawList> List = MakeShared<ss::ui::DrawList>();
		ss::ui::Canvas Cv(*List, Game->Measurer, ss::ui::PhoneScreen::Width, ss::ui::PhoneScreen::Height, 1.0f);
		Game->Phone.Draw(Cv, Clock);
		Stage->SetPhoneDrawList(List);
	}
	Tilt += (static_cast<float>(S.HeroTilt) * 0.85f - Tilt) * static_cast<float>(FMath::Min(1.0, Dt * 2.0));
	if (bBeat)
	{
		Pulse = 1.0f;
	}
	Pulse = FMath::Max(0.0f, Pulse - static_cast<float>(Dt) * 3.5f);
	if (Seat)
	{
		Seat->Shake = Pulse * 1.5f;
		Stage->SetLens(Seat->Focus, Tilt, Pulse);
	}
	Stage->SetScreenGlow(S.CurrentScreen == ss::Screen::Table ? FLinearColor(0.55f, 0.9f, 0.8f) : FLinearColor(0.72f, 0.84f, 1.0f), 1.0f);

	if (SettingsDirtyAt >= 0.0 && RealTime - SettingsDirtyAt > 0.75)
	{
		SaveSettingsNow();
	}
}

// ------------------------------------------------------------------ input

void ANightOneGameMode::OnMouse(bool bOverScreen, const FVector2D& Client, float Nx, float Ny)
{
	ANightOnePawn* Seat = GetSeat();
	if (!Game || !Seat || IsMenuOpen())
	{
		bOverScreenNow = false;
		return;
	}
	bOverScreenNow = bOverScreen;
	ss::ui::Pointer& P = Game->Client.UI.Ptr;
	if (Seat->Focus > 0.85f)
	{
		P.Active = bOverScreen;
		if (bOverScreen)
		{
			P.X = static_cast<float>(Client.X);
			P.Y = static_cast<float>(Client.Y);
		}
	}
	else
	{
		P.Active = false;
		if (bStarted)
		{
			// Look around by pointing.
			Seat->Yaw = Nx * 2.4f * LookSensitivity;
			Seat->Pitch = (bInvertLook ? Ny : -Ny) * 1.1f * LookSensitivity - 0.05f;
		}
	}
}

void ANightOneGameMode::OnPress(bool bOverScreen)
{
	ANightOnePawn* Seat = GetSeat();
	if (!bStarted || !Game || !Seat || IsMenuOpen())
	{
		return;
	}
	if (Seat->Focus < 0.5f && Seat->TargetFocus < 0.5f)
	{
		// Clicking toward the laptop leans back in.
		if (bOverScreen)
		{
			Seat->TargetFocus = 1.0f;
		}
		return;
	}
	ss::ui::Pointer& P = Game->Client.UI.Ptr;
	P.Down = true;
	P.Pressed = true;
	if (Audio)
	{
		Audio->Play(ss::SoundId::Click, 0.5f);
	}
}

void ANightOneGameMode::OnRelease()
{
	if (!Game || IsMenuOpen())
	{
		return;
	}
	ss::ui::Pointer& P = Game->Client.UI.Ptr;
	if (P.Down)
	{
		P.Released = true;
	}
	P.Down = false;
}

void ANightOneGameMode::OnWheel(float Delta)
{
	if (Game && !IsMenuOpen())
	{
		Game->Client.UI.Ptr.Wheel += Delta;
	}
}

void ANightOneGameMode::OnKey(const FString& Key)
{
	if (!bStarted || !Game || IsMenuOpen())
	{
		return;
	}
	if (Key == TEXT(" "))
	{
		if (ANightOnePawn* Seat = GetSeat())
		{
			Seat->ToggleLean();
		}
		return;
	}
	if (Key == TEXT("m"))
	{
		if (Audio)
		{
			Audio->SetMuted(!Audio->IsMuted());
		}
		return;
	}
	Game->Client.Key(std::string(TCHAR_TO_UTF8(*Key)));
}

bool ANightOneGameMode::HideCursor(bool bOverScreen) const
{
	const ANightOnePawn* Seat = GetSeat();
	return bStarted && !IsMenuOpen() && bOverScreen && Seat && Seat->Focus > 0.85f;
}

bool ANightOneGameMode::IsLeanedBack() const
{
	const ANightOnePawn* Seat = GetSeat();
	return bStarted && !IsMenuOpen() && Seat && Seat->Focus <= 0.85f;
}
