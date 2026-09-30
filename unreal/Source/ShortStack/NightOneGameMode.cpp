#include "NightOneGameMode.h"

#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/DateTime.h"
#include "NightOneAudio.h"
#include "NightOneGame.h"
#include "NightOnePawn.h"
#include "NightOnePlayerController.h"
#include "NightOneSaveGame.h"
#include "NightOneStage.h"
#include "SNightOneOverlay.h"
#include "ShortStack.h"
#include "Widgets/Input/SEditableTextBox.h"

namespace NightOneModeDetail
{
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

ANightOneGameMode::~ANightOneGameMode() = default;

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
	const std::string Seed = std::string(TCHAR_TO_UTF8(*FString::Printf(TEXT("%lld"), FDateTime::Now().GetTicks())));
	Game = MakeUnique<FNightOneGame>(*this, bLoaded ? &Loaded : nullptr, Seed);

	// Opening shot: sitting back, looking at the rain.
	if (ANightOnePawn* Seat = GetSeat())
	{
		Seat->Focus = 0.0f;
		Seat->TargetFocus = 0.0f;
		Seat->Yaw = 0.12f;
		Seat->Pitch = 0.05f;
	}

	UGameViewportClient* Viewport = World->GetGameViewport();
	if (Viewport && FSlateApplication::IsInitialized())
	{
		Overlay = SNew(SNightOneOverlay);
		Overlay->SetName(FString(UTF8_TO_TCHAR(Game->Session.HeroName.c_str())));
		TWeakObjectPtr<ANightOneGameMode> WeakThis = this;
		Overlay->OnBegin = [WeakThis](const FString& Name) {
			if (WeakThis.IsValid())
			{
				WeakThis->Begin(Name);
			}
		};
		Viewport->AddViewportWidgetContent(Overlay.ToSharedRef(), 10);
		if (APlayerController* Pc = UGameplayStatics::GetPlayerController(this, 0))
		{
			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(Overlay->GetNameBox());
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			Pc->SetInputMode(InputMode);
		}
	}
	else
	{
		Begin(FString());
	}
}

void ANightOneGameMode::Begin(const FString& Name)
{
	if (bStarted || !Game)
	{
		return;
	}
	bStarted = true;
	BeganAt = RealTime;
	const FString Clean = SanitizeName(Name);
	if (Clean.Len() >= 3)
	{
		Game->Session.HeroName = std::string(TCHAR_TO_UTF8(*Clean));
	}
	Game->Session.Save();
	if (Audio)
	{
		Audio->StartAmbience();
	}
	if (Overlay)
	{
		Overlay->HideIntro();
	}
	if (APlayerController* Pc = UGameplayStatics::GetPlayerController(this, 0))
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		Pc->SetInputMode(InputMode);
	}
	UE_LOG(LogNightOne, Log, TEXT("Night One started as %s"), *Clean);
}

void ANightOneGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Overlay)
	{
		if (UWorld* World = GetWorld())
		{
			if (UGameViewportClient* Viewport = World->GetGameViewport())
			{
				Viewport->RemoveViewportWidgetContent(Overlay.ToSharedRef());
			}
		}
		Overlay.Reset();
	}
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
	// After Begin: lean in to the laptop and show the controls hint.
	if (bStarted && BeganAt >= 0.0 && RealTime - BeganAt > 1.8)
	{
		BeganAt = -1.0;
		if (Seat)
		{
			Seat->TargetFocus = 1.0f;
		}
		if (Overlay)
		{
			Overlay->ShowHint();
		}
	}

	const bool bBeat = Audio && Audio->ConsumeBeat();
	GameTime += Dt * TimeScale;
	ss::Session& S = Game->Session;
	S.Update(GameTime);

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
}

// ------------------------------------------------------------------ input

void ANightOneGameMode::OnMouse(bool bOverScreen, const FVector2D& Client, float Nx, float Ny)
{
	bOverScreenNow = bOverScreen;
	ANightOnePawn* Seat = GetSeat();
	if (!Game || !Seat)
	{
		return;
	}
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
			Seat->Yaw = Nx * 2.4f;
			Seat->Pitch = -Ny * 1.1f - 0.05f;
		}
	}
}

void ANightOneGameMode::OnPress(bool bOverScreen)
{
	ANightOnePawn* Seat = GetSeat();
	if (!bStarted || !Game || !Seat)
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
	if (!Game)
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
	if (Game)
	{
		Game->Client.UI.Ptr.Wheel += Delta;
	}
}

void ANightOneGameMode::OnKey(const FString& Key)
{
	if (!bStarted || !Game)
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
	return bStarted && bOverScreen && Seat && Seat->Focus > 0.85f;
}

bool ANightOneGameMode::IsLeanedBack() const
{
	const ANightOnePawn* Seat = GetSeat();
	return bStarted && Seat && Seat->Focus <= 0.85f;
}
