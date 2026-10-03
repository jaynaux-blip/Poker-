#include "StreetGameMode.h"

#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NightOneAudio.h"
#include "NightOneSaveGame.h"
#include "SFrontEndWidget.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Store.h"
#include "ShortStackCharacter.h"
#include "StreetStage.h"

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
} // namespace StreetGameDetail

using namespace StreetGameDetail;

// ------------------------------------------------------------------ the game and its hooks

FStreetGame::FStreetGame(AStreetGameMode& InMode, const ss::SaveData* Loaded, const std::string& Seed)
	: Mode(InMode)
	, Session(*this, Seed, Loaded)
	, Counter(Session)
	, Menu(*this)
{
	// Out of the apartment the clock runs as it does in the lobby: real minutes.
	Session.CurrentScreen = ss::Screen::Lobby;
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
	if (UNightOneSaveGame* Obj = Cast<UNightOneSaveGame>(UGameplayStatics::CreateSaveGameObject(UNightOneSaveGame::StaticClass())))
	{
		Obj->Data = Utf8ToFString(Data.Serialize());
		UGameplayStatics::SaveGameToSlot(Obj, UNightOneSaveGame::SlotName(), 0);
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
	UGameplayStatics::OpenLevel(&Mode, FName(TEXT("NightOne")), true, TEXT("Menu"));
}

void FStreetGame::QuitGame()
{
	Session.Save();
	UKismetSystemLibrary::QuitGame(&Mode, UGameplayStatics::GetPlayerController(&Mode, 0), EQuitPreference::Quit, false);
}

void FStreetGame::SettingsChanged(const ss::ui::GameSettings& Settings)
{
	Mode.SaveSettings(Settings);
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
	bool bLoaded = false;
	if (UGameplayStatics::DoesSaveGameExist(UNightOneSaveGame::SlotName(), 0))
	{
		if (UNightOneSaveGame* Obj = Cast<UNightOneSaveGame>(UGameplayStatics::LoadGameFromSlot(UNightOneSaveGame::SlotName(), 0)))
		{
			bLoaded = ss::SaveData::Parse(FStringToUtf8(Obj->Data), Loaded);
		}
	}
	const std::string Seed = FStringToUtf8(FString::Printf(TEXT("street:%lld"), FDateTime::Now().GetTicks()));
	Game = MakeUnique<FStreetGame>(*this, bLoaded ? &Loaded : nullptr, Seed);
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
	if (!Stage || Clerk)
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Clerk = GetWorld()->SpawnActor<AShortStackCharacter>(AShortStackCharacter::StaticClass(), Stage->ClerkSpot(), Params);
	if (Clerk)
	{
		// Benny, nights, in the store's red polo.
		Clerk->AutoPossessAI = EAutoPossessAI::Disabled;
		Clerk->ApplyCast(TEXT("ExtraB"), FLinearColor::FromSRGBColor(FColor(0xd7, 0x26, 0x3d)));
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
	if (Audio)
	{
		const ss::ui::GameSettings& S = Game->Menu.Settings;
		Audio->SetMix(S.MasterVolume / 100.0f, S.EffectsVolume / 100.0f, S.AmbienceVolume / 100.0f);
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
	if (!Game->Menu.IsPaused())
	{
		Game->Session.Update(RealTime);
	}
	AShortStackCharacter* Hero = GetHero();
	if (Hero && Stage)
	{
		Stage->UpdateDoors(Hero->GetActorLocation(), static_cast<float>(Dt));
		if (APlayerController* Pc = GetWorld()->GetFirstPlayerController())
		{
			if (Pc->PlayerCameraManager)
			{
				Stage->FollowCamera(Pc->PlayerCameraManager->GetCameraLocation(), Pc->PlayerCameraManager->GetCameraRotation());
			}
		}
		float Volume = 0.0f;
		if (Hero->TakeFootstep(Volume) && Audio)
		{
			Audio->PlayEffect(ss::audio::Effect::Step, Volume * (Stage->IsInsideStore(Hero->GetActorLocation()) ? 0.7f : 1.0f));
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
		UGameplayStatics::OpenLevel(this, FName(TEXT("NightOne")), true, FString::Printf(TEXT("Home?Street?From=%.0f"), Game->Session.WorldMinutes()));
	}
	// Toasts older than ten seconds go.
	Toasts.RemoveAll([this](const ss::ui::StreetHudInfo::Toast& T) { return RealTime - T.At > 10.0; });
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
	AShortStackCharacter* Hero = GetHero();
	const std::string Id = Game->Session.BagPick();
	if (Id.empty())
	{
		Toast(TEXT("Your bag"), TEXT("Nothing in it. The Lucky Penny's on the corner."));
		return;
	}
	const ss::store::Item* Item = ss::store::Find(Id);
	if (Game->Session.Consume(Id).empty() && Item)
	{
		if (Hero)
		{
			Hero->Sip();
		}
		if (Audio && !Item->Food())
		{
			Audio->PlayEffect(ss::audio::Effect::CanOpen, 0.6f);
		}
		Toast(TEXT("You"), FString::Printf(TEXT("%s the %s."), Item->Food() ? TEXT("Eat") : TEXT("Drink"), *Utf8ToFString(Item->Name)));
	}
}

void AStreetGameMode::OnPause()
{
	if (Game->Counter.IsOpen())
	{
		return;
	}
	Game->Menu.Info = ss::ui::DescribeSession(Game->Session, true);
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

void AStreetGameMode::SaveSettings(const ss::ui::GameSettings& Settings)
{
	Sensitivity = 0.11f * static_cast<float>(Settings.LookSensitivity) / 100.0f;
	bInvertLook = Settings.InvertLook;
	if (Audio)
	{
		Audio->SetMix(Settings.MasterVolume / 100.0f, Settings.EffectsVolume / 100.0f, Settings.AmbienceVolume / 100.0f);
	}
	if (UNightOneSaveGame* Obj = Cast<UNightOneSaveGame>(UGameplayStatics::CreateSaveGameObject(UNightOneSaveGame::StaticClass())))
	{
		Obj->Data = Utf8ToFString(Settings.Serialize());
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
	return FString::Printf(TEXT("%s at %s %s | %s view | spot %s | hunger %.0f thirst %.0f energy %.0f | $%.2f | bag %d | counter %s"), *Utf8ToFString(ss::net::TimeLabel(Game->Session.WorldMinutes())),
		*Hero->GetActorLocation().ToCompactString(), Stage ? *Stage->PlaceName(Hero->GetActorLocation()) : TEXT("?"), Hero->IsFirstPerson() ? TEXT("first") : TEXT("third"),
		Spot ? *Spot->Id.ToString() : TEXT("none"), L.Hunger, L.Thirst, L.Energy, Game->Session.BankrollCents / 100.0, static_cast<int32>(L.Pantry.size()),
		Game->Counter.IsOpen() ? TEXT("open") : TEXT("closed"));
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
	// Pause. In Play-In-Editor, Escape stops the session before the game sees it, so P works too.
	if (WasInputKeyJustPressed(EKeys::Escape) || WasInputKeyJustPressed(EKeys::P) || WasInputKeyJustPressed(EKeys::Gamepad_Special_Right))
	{
		Mode->OnPause();
	}
}
