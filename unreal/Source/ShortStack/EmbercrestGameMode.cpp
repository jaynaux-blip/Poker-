#include "EmbercrestGameMode.h"

#include "EmbercrestAudio.h"
#include "EmbercrestPaint.h"
#include "EmbercrestShared.h"
#include "EmbercrestStage.h"

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "CareerSave.h"
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
#include "ShortStack/Game/Network.h"
#include "ShortStackCharacter.h"

#include <algorithm>
#include <string>
#include <vector>

DEFINE_LOG_CATEGORY_STATIC(LogEmbercrestExterior, Log, All);

namespace EcxModeDetail
{
TAutoConsoleVariable<float> CVarHour(TEXT("ss.ECX.Hour"), -1.0f,
	TEXT("The Embercrest's hour (0..24) for the light, over the night's own (-1: the night's; with no save and no options, 19:00)."));

FString Str(const std::string& S)
{
	return FString(UTF8_TO_TCHAR(S.c_str()));
}

std::string Utf8(const FString& S)
{
	return std::string(TCHAR_TO_UTF8(*S));
}

FString Grouped(int64 N)
{
	FString Digits = FString::Printf(TEXT("%lld"), N < 0 ? -N : N);
	for (int32 At = Digits.Len() - 3; At > 0; At -= 3)
	{
		Digits.InsertAt(At, TEXT(","));
	}
	return N < 0 ? TEXT("-") + Digits : Digits;
}

/** "$120", "$1,960", "$2.90". */
FString Dollars(int64 Cents)
{
	if (Cents % 100 == 0)
	{
		return TEXT("$") + Grouped(Cents / 100);
	}
	return FString::Printf(TEXT("$%s.%02lld"), *Grouped(Cents / 100), static_cast<long long>(FMath::Abs(Cents % 100)));
}

FString Clock(double WorldMinutes)
{
	return Str(ss::net::TimeLabel(WorldMinutes));
}

/** An event's name without its price ("Embercrest Nightly $120" to "Embercrest Nightly"). */
FString Plain(const char* Name)
{
	FString S = Str(Name ? std::string(Name) : std::string());
	int32 Cut = INDEX_NONE;
	if (S.FindChar(TEXT('$'), Cut) && Cut > 0)
	{
		S = S.Left(Cut).TrimEnd();
	}
	return S;
}

/** When an event is, said from World: "TONIGHT", "TODAY", "TOMORROW NIGHT", "TOMORROW", or its weekday. */
FString DayWordOf(const ss::live::Occurrence& O, double World)
{
	const int32 Ahead = O.Day - ss::net::DayOf(World);
	const bool bEvening = O.Valid() && O.T->StartMinute >= 17 * 60;
	if (Ahead <= 0)
	{
		return bEvening ? TEXT("TONIGHT") : TEXT("TODAY");
	}
	if (Ahead == 1)
	{
		return bEvening ? TEXT("TOMORROW NIGHT") : TEXT("TOMORROW");
	}
	return Str(ss::net::WeekdayName(O.Day, true)).ToUpper();
}

/** The next event still taking players at World (late registration open), within two days; invalid when there's none. */
ss::live::Occurrence NextEvent(double World)
{
	for (int32 Day = ss::net::DayOf(World); Day <= ss::net::DayOf(World) + 2; ++Day)
	{
		for (const ss::live::Occurrence& O : ss::live::Occurrences(Day))
		{
			if (O.LateRegEnds > World)
			{
				return O;
			}
		}
	}
	return ss::live::Occurrence();
}

/** The options this level was given, as they came (without the leading "?"), less the one named Drop. */
FString Without(const FString& Options, const TCHAR* Drop)
{
	TArray<FString> Parts;
	Options.ParseIntoArray(Parts, TEXT("?"), true);
	FString Out;
	for (const FString& P : Parts)
	{
		if (Drop)
		{
			FString Key = P;
			int32 Eq = INDEX_NONE;
			if (P.FindChar(TEXT('='), Eq))
			{
				Key = P.Left(Eq);
			}
			if (Key.Equals(Drop, ESearchCase::IgnoreCase))
			{
				continue;
			}
		}
		Out += (Out.IsEmpty() ? TEXT("") : TEXT("?")) + P;
	}
	return Out;
}

FIntPoint ParseResolution(const std::string& Text)
{
	FString Left;
	FString Right;
	if (!Str(Text).Split(TEXT("x"), &Left, &Right))
	{
		return FIntPoint(0, 0);
	}
	const int32 Wd = FCString::Atoi(*Left.TrimStartAndEnd());
	const int32 Ht = FCString::Atoi(*Right.TrimStartAndEnd());
	return Wd > 0 && Ht > 0 ? FIntPoint(Wd, Ht) : FIntPoint(0, 0);
}

std::vector<std::string> Resolutions()
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

void SetGameCvar(const TCHAR* Name, int32 Value)
{
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name))
	{
		Var->Set(Value, ECVF_SetByGameSetting);
	}
}

/** A camera's horizontal field of view for the player's Field of View setting (as the street works it out). */
float FovFor(float DefaultHorizontal, int32 VerticalSetting)
{
	const double K = FMath::Tan(FMath::DegreesToRadians(FMath::Clamp(VerticalSetting, 30, 110) * 0.5)) / FMath::Tan(FMath::DegreesToRadians(25.0));
	const double Half = FMath::Atan(FMath::Tan(FMath::DegreesToRadians(DefaultHorizontal * 0.5)) * K);
	return FMath::Clamp(static_cast<float>(FMath::RadiansToDegrees(Half * 2.0)), 50.0f, 125.0f);
}

AEmbercrestGameMode* ModeIn(UWorld* World)
{
	return World ? World->GetAuthGameMode<AEmbercrestGameMode>() : nullptr;
}

FAutoConsoleCommandWithWorldAndArgs SkipCmd(TEXT("ss.ECX.Skip"), TEXT("Skips the Embercrest's arrival shots: the hero on the platform, the bus about to leave."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (AEmbercrestGameMode* M = ModeIn(World))
		{
			M->TestSkip();
		}
	}));

FAutoConsoleCommandWithWorldAndArgs ShotCmd(TEXT("ss.ECX.Shot"), TEXT("ss.ECX.Shot S1..S6 | off: the Embercrest's acceptance shots (spec 7)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (AEmbercrestGameMode* M = ModeIn(World))
		{
			M->TestShot(Args.Num() > 0 ? Args[0] : FString(TEXT("off")));
		}
	}));

FAutoConsoleCommandWithWorldAndArgs ModeCmd(TEXT("ss.ECX.Mode"), TEXT("ss.ECX.Mode arrive | leave: plays the Embercrest's arrival (the bus pulls in) or the way home again."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (AEmbercrestGameMode* M = ModeIn(World))
		{
			M->TestMode(Args.Num() > 0 ? Args[0] : FString(TEXT("arrive")));
		}
	}));

FAutoConsoleCommandWithWorldAndArgs TeleportCmd(TEXT("ss.ECX.Teleport"), TEXT("ss.ECX.Teleport bus | doors | canopy | overlook | landingA | arrive | leave"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (AEmbercrestGameMode* M = ModeIn(World))
		{
			M->TestTeleport(Args.Num() > 0 ? Args[0] : FString(TEXT("arrive")));
		}
	}));

FAutoConsoleCommandWithWorldAndArgs DescribeCmd(TEXT("ss.ECX.Describe"), TEXT("The Embercrest now: the flow, the hour, the sun, the exposure, what's in reach, the bus."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (AEmbercrestGameMode* M = ModeIn(World))
		{
			UE_LOG(LogEmbercrestExterior, Display, TEXT("%s"), *M->TestDescribe());
		}
	}));

/** The arrival's crane shot (spec 5.2 A): from out over the valley, in and down toward the resort. */
const FVector CraneFrom(30000.0, -10000.0, 3000.0);
const FVector CraneTo(26000.0, -7500.0, 2600.0);
const FVector CraneLook(0.0, 0.0, 6300.0);
/** Shot B: on the platform as the bus pulls in. */
const FVector StopFrom(4700.0, 750.0, -70.0);
const FVector StopLook(5300.0, 100.0, -110.0);
constexpr double ShotB = 3.8;
constexpr double StepOffAt = 6.9;
constexpr double BlendAt = 7.4;
constexpr double ControlAt = 8.2;
} // namespace EcxModeDetail

using namespace EcxModeDetail;

// ------------------------------------------------------------------ the pause menu's hooks

void FEmbercrestGame::UiSound(ss::SoundId Id, double Volume)
{
	if (UNightOneAudio* A = Mode.GetAudio())
	{
		A->Play(Id, static_cast<float>(Volume));
	}
}

void FEmbercrestGame::Resume()
{
	Mode.ReturnInputToGame();
}

void FEmbercrestGame::QuitToMenu()
{
	Mode.LeaveForMenu();
}

void FEmbercrestGame::QuitGame()
{
	Mode.LeaveGame();
}

void FEmbercrestGame::SettingsChanged(const ss::ui::GameSettings& Settings)
{
	Mode.ApplySettings(Settings, true);
}

// ------------------------------------------------------------------ the mode

AEmbercrestGameMode::AEmbercrestGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	DefaultPawnClass = AShortStackCharacter::StaticClass();
	PlayerControllerClass = AEmbercrestPlayerController::StaticClass();
	Audio = CreateDefaultSubobject<UNightOneAudio>(TEXT("Audio"));
}

void AEmbercrestGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	OptionsString = Options;
	bLeaving = UGameplayStatics::HasOption(Options, TEXT("Leaving"));
	// The home options carry Live=1 too: only an arrival names an occurrence.
	bLiveOption = !bLeaving && UGameplayStatics::HasOption(Options, TEXT("Live"));
	LiveId = bLiveOption ? UGameplayStatics::ParseOption(Options, TEXT("Live")) : FString();
	Game = MakeUnique<FEmbercrestGame>(*this);
	Game->Menu.SetVenue(ss::ui::Venue::Street, "THE EMBERCREST");
	if (UGameplayStatics::DoesSaveGameExist(UNightOneSaveGame::SettingsSlotName(), 0))
	{
		if (UNightOneSaveGame* Obj = Cast<UNightOneSaveGame>(UGameplayStatics::LoadGameFromSlot(UNightOneSaveGame::SettingsSlotName(), 0)))
		{
			ss::ui::GameSettings::Parse(Utf8(Obj->Data), Game->Menu.Settings);
		}
	}
	Sensitivity = 0.11f * static_cast<float>(Game->Menu.Settings.LookSensitivity) / 100.0f;
	bInvertLook = Game->Menu.Settings.InvertLook;
	ReadTheNight();
	UE_LOG(LogEmbercrestExterior, Log, TEXT("The Embercrest, %s: %s, %s (%s)"), bLeaving ? TEXT("leaving") : bLiveOption ? TEXT("arriving") : TEXT("walking round"),
		Event.Valid() ? *Str(Event.T->Name) : TEXT("no event"), *Clock(BaseMinutes), bCareer ? TEXT("career") : TEXT("no career"));
}

void AEmbercrestGameMode::ReadTheNight()
{
	std::string Text;
	bCareer = CareerSave::LoadText(Text) && ss::SaveData::Parse(Text, Save);
	const double NightDay = ss::net::MinutesPerDay * ss::net::NightOneDay;
	if (bLeaving)
	{
		// The card room wrote the arrival home (its departure + 25): the walk to the bus is just before it.
		bWon = UGameplayStatics::HasOption(OptionsString, TEXT("Won"));
		bCancelled = UGameplayStatics::HasOption(OptionsString, TEXT("Cancelled"));
		PrizeCents = FCString::Atoi64(*UGameplayStatics::ParseOption(OptionsString, TEXT("Prize")));
		EventShort = UGameplayStatics::ParseOption(OptionsString, TEXT("Event")).Replace(TEXT("_"), TEXT(" "));
		BaseMinutes = bCareer ? Save.ClockMinutes + NightDay - 25.0 : NightDay + 23.0 * 60.0;
		return;
	}
	if (!bLiveOption)
	{
		// Opened on its own: an evening for walking round.
		const int32 Day = bCareer ? ss::net::DayOf(NightDay + Save.ClockMinutes) : ss::net::NightOneDay;
		BaseMinutes = ss::net::MinutesPerDay * Day + 19.0 * 60.0;
		return;
	}
	const double LeftHome = NightDay + (bCareer ? Save.ClockMinutes : 19.0 * 60.0 - ss::live::TravelMinutes);
	BaseMinutes = LeftHome + ss::live::TravelMinutes;
	Event = ss::live::FindOccurrence(Utf8(LiveId));
	if (!Event.Valid())
	{
		return;
	}
	const ss::life::LiveEntry* E = bCareer ? ss::live::EntryFor(Save.Life, Event.Id) : nullptr;
	if (E && E->State != ss::life::LiveEntry::Registered)
	{
		// Already settled: the screens show what's next (the card room gives the player the practice table).
		Event = ss::live::Occurrence();
		return;
	}
	bEntry = E != nullptr;
	if (!E)
	{
		// The desk registers them on arrival (it opens three hours before cards): the card room's own clock.
		BaseMinutes = FMath::Max(BaseMinutes, Event.DeskOpens);
	}
	else if (E->ArrivedAt > 0.0)
	{
		BaseMinutes = E->ArrivedAt;
	}
	bCancelled = E && ss::live::BelowMinimum(Event, E->Entrants);
	bStarted = BaseMinutes > Event.Start;
	if (E)
	{
		for (const std::string& Line : E->Board)
		{
			Board.Add(Str(Line));
		}
	}
}

double AEmbercrestGameMode::ShownMinutes() const
{
	return BaseMinutes + RealTime / 60.0;
}

double AEmbercrestGameMode::LightHour() const
{
	const float Override = CVarHour.GetValueOnGameThread();
	if (Override >= 0.0f)
	{
		return Override;
	}
	return FMath::Fmod(BaseMinutes / 60.0, 24.0);
}

void AEmbercrestGameMode::FindOrSpawnStage()
{
	if (Stage || !GetWorld())
	{
		return;
	}
	for (TActorIterator<AEmbercrestStage> It(GetWorld()); It; ++It)
	{
		Stage = *It;
		return;
	}
	Stage = GetWorld()->SpawnActor<AEmbercrestStage>(AEmbercrestStage::StaticClass(), FTransform::Identity);
}

void AEmbercrestGameMode::RestartPlayer(AController* NewPlayer)
{
	FindOrSpawnStage();
	if (Stage)
	{
		// The ground before the hero stands on it.
		Stage->EnsureBuilt();
	}
	// Arriving the hero first rides in on the bus; a hero lost later (below the world) comes back on the platform, the
	// recovery point (spec 2.9), not inside a bus that may have left.
	const EEcxSpawn Where = bLeaving ? EEcxSpawn::Leave : bLiveOption && !HasActorBegunPlay() ? EEcxSpawn::ArriveInside : EEcxSpawn::ArrivePlatform;
	const FTransform Start = Stage ? Stage->SpawnAt(Where) : FTransform(FRotator(0.0f, 180.0f, 0.0f), FVector(5140.0, 100.0, -125.0));
	RestartPlayerAtTransform(NewPlayer, Start);
	if (NewPlayer)
	{
		NewPlayer->SetControlRotation(FRotator(bLeaving ? -3.0f : 8.0f, Start.Rotator().Yaw, 0.0f));
	}
	DressHero();
}

AShortStackCharacter* AEmbercrestGameMode::GetHero() const
{
	APlayerController* Pc = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	return Pc ? Cast<AShortStackCharacter>(Pc->GetPawn()) : nullptr;
}

void AEmbercrestGameMode::DressHero()
{
	AShortStackCharacter* Hero = GetHero();
	if (!Hero || bDressed)
	{
		return;
	}
	bDressed = true;
	// The player as the creator made them (a career from before the creator, or none: the default person).
	Hero->ApplyLook(bCareer ? Save.Person : ss::hero::Character());
}

void AEmbercrestGameMode::SpawnPeople()
{
	if (!Stage || !GetWorld() || People.Num() > 0)
	{
		return;
	}
	struct FWho
	{
		const TCHAR* Cast;
		uint32 Shirt;
		FVector At;
		float Yaw;
		bool bEvening;
	};
	// The doorman in the Embercrest's navy, the valet in black behind his stand, a dealer on her break in white by the ash
	// urn; an arriving couple at the curb from late afternoon to half past midnight (spec 2.9).
	const FWho Who[5] = {{TEXT("ExtraJ"), 0x121c33, FVector(260.0, -260.0, 0.0), 15.0f, false}, {TEXT("ExtraD"), 0x16181d, FVector(575.0, -1100.0, 0.0), 0.0f, false},
		{TEXT("ExtraL"), 0xebe7df, FVector(2290.0, 2030.0, 0.0), 340.0f, false}, {TEXT("ExtraE"), 0xd9531e, FVector(720.0, -540.0, 0.0), 190.0f, true},
		{TEXT("ExtraF"), 0x2a2d33, FVector(700.0, -460.0, 0.0), 195.0f, true}};
	const double Hour = LightHour();
	const bool bEvening = Hour >= 16.0 || Hour < 0.5;
	for (const FWho& W : Who)
	{
		if (W.bEvening && !bEvening)
		{
			continue;
		}
		const FTransform Spot = FTransform(FRotator(0.0f, W.Yaw, 0.0f), W.At + FVector(0.0, 0.0, 100.0)) * Stage->GetActorTransform();
		AShortStackCharacter* P = GetWorld()->SpawnActorDeferred<AShortStackCharacter>(AShortStackCharacter::StaticClass(), Spot, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!P)
		{
			continue;
		}
		P->AutoPossessAI = EAutoPossessAI::Disabled;
		P->ApplyCast(W.Cast, Ecx::Srgb(W.Shirt));
		P->FinishSpawning(Spot);
		// Their feet on the stone (nothing drops them there: with no controller their movement never runs), as the street
		// stands Benny behind his counter.
		const float Half = P->GetCapsuleComponent() ? P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 89.0f;
		P->SetActorLocation(Stage->GetActorTransform().TransformPosition(FVector(W.At.X, W.At.Y, W.At.Z + Half)), false, nullptr, ETeleportType::TeleportPhysics);
		// Standing: no movement to simulate, no camera of their own, their pose only while they're seen.
		if (UCharacterMovementComponent* Move = P->GetCharacterMovement())
		{
			Move->StopMovementImmediately();
			Move->SetComponentTickEnabled(false);
		}
		if (P->Boom)
		{
			P->Boom->bDoCollisionTest = false;
			P->Boom->SetComponentTickEnabled(false);
		}
		if (P->Camera)
		{
			P->Camera->Deactivate();
		}
		P->GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		if (P->Face)
		{
			P->Face->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		}
		People.Add(P);
	}
}

void AEmbercrestGameMode::CullPeople(const FVector& Camera)
{
	// Nobody drawn past 45 m (spec 2.9).
	for (AShortStackCharacter* P : People)
	{
		if (P)
		{
			const bool bFar = FVector::DistSquared(P->GetActorLocation(), Camera) > 4500.0 * 4500.0;
			if (P->IsHidden() != bFar)
			{
				P->SetActorHiddenInGame(bFar);
			}
		}
	}
}

void AEmbercrestGameMode::BeginPlay()
{
	Super::BeginPlay();
	FindOrSpawnStage();
	if (Stage)
	{
		Stage->EnsureBuilt();
		Stage->SetHour(LightHour());
	}
	AppliedOverride = CVarHour.GetValueOnGameThread();
	DressHero();
	SpawnPeople();
	CreateWidgets();
	ReturnInputToGame();
	ApplySettings(Game->Menu.Settings, false);
	BuildSlides();
	if (Audio)
	{
		Tone = MakeShared<FEmbercrestAmbience>(static_cast<uint32>(FDateTime::Now().GetTicks()));
		TSharedPtr<FEmbercrestAmbience> T = Tone;
		Audio->StartAmbience([T](float* Out, int32 N) { T->Render(Out, N); }, 0.8f);
	}
	if (bLeaving)
	{
		StartLeaving();
	}
	else
	{
		StartArrival(bLiveOption);
	}
}

void AEmbercrestGameMode::EndPlay(const EEndPlayReason::Type Reason)
{
	if (SettingsDirtyAt >= 0.0)
	{
		SaveSettingsNow();
	}
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		for (TSharedPtr<SWidget> W : {StaticCastSharedPtr<SWidget>(HudWidget), StaticCastSharedPtr<SWidget>(MenuWidget)})
		{
			if (W)
			{
				Viewport->RemoveViewportWidgetContent(W.ToSharedRef());
			}
		}
	}
	Super::EndPlay(Reason);
}

// ------------------------------------------------------------------ the screens' slides (spec 5.6)

void AEmbercrestGameMode::BuildSlides()
{
	if (!Stage)
	{
		return;
	}
	TArray<FEcxSlide> Slides;
	FString Ticker;
	auto Series = []() {
		FEcxSlide S;
		S.Image = 1;
		return S;
	};
	auto Grill = []() {
		FEcxSlide S;
		S.Kicker = TEXT("EMBER GRILL");
		S.Title = TEXT("OPEN LATE");
		S.Detail = TEXT("STEAKS OVER THE FIRE  \xB7  THE BAR  \xB7  THE TERRACE");
		return S;
	};
	auto Schedule = [](int32 Day) {
		FEcxSlide S;
		S.Kicker = Str(ss::net::WeekdayName(Day, true)).ToUpper() + TEXT(" AT THE EMBERCREST");
		for (const ss::live::Occurrence& O : ss::live::Occurrences(Day))
		{
			S.Rows.Add(Clock(O.Start) + TEXT("\t") + Str(O.T->Short).ToUpper() + TEXT("\t") + Dollars(O.T->BuyInCents));
		}
		return S;
	};
	auto Cards = [](const ss::live::Occurrence& O) {
		return FString::Printf(TEXT("CARDS AT %s  \xB7  %s  \xB7  %s CHIPS"), *Clock(O.Start), *Dollars(O.T->BuyInCents), *Grouped(O.T->StartingStack));
	};
	const FString Hero = !bCareer ? FString(TEXT("YOU")) : Save.Person.Created ? Str(Save.Person.FullName()) : Str(Save.HeroName);
	if (bLeaving)
	{
		const FString EventName = EventShort.IsEmpty() ? FString(TEXT("TONIGHT'S EVENT")) : EventShort.ToUpper();
		const int32 Today = ss::net::DayOf(BaseMinutes);
		if (bWon)
		{
			FEcxSlide Win;
			Win.Kicker = TEXT("CONGRATULATIONS");
			Win.Title = Hero.ToUpper();
			Win.Detail = FString::Printf(TEXT("%s CHAMPION  \xB7  %s"), *EventName, *Dollars(PrizeCents));
			Slides.Add(Win);
			Slides.Add(Schedule(Today));
			Ticker = FString::Printf(TEXT("CONGRATULATIONS %s  \xB7  %s CHAMPION  \xB7  %s"), *Hero.ToUpper(), *EventName, *Dollars(PrizeCents));
		}
		else
		{
			Slides.Add(Series());
			FEcxSlide Thanks;
			Thanks.Kicker = TEXT("THANKS FOR PLAYING");
			Thanks.Title = TEXT("DRIVE SAFE");
			Thanks.Detail = TEXT("THE 14 RUNS TO NORTHSIDE ALL NIGHT");
			Slides.Add(Thanks);
			Ticker = TEXT("THANKS FOR PLAYING  \xB7  DRIVE SAFE");
			const std::vector<ss::live::Occurrence> Tomorrow = ss::live::Occurrences(Today + 1);
			if (!Tomorrow.empty())
			{
				FEcxSlide Next;
				Next.Kicker = TEXT("TOMORROW");
				Next.Title = Str(Tomorrow[0].T->Short).ToUpper();
				Next.Detail = Cards(Tomorrow[0]);
				Slides.Add(Next);
				Ticker += FString::Printf(TEXT("  \xB7  TOMORROW  \xB7  %s  \xB7  %s"), *Next.Title, *Clock(Tomorrow[0].Start));
			}
		}
	}
	else if (Event.Valid())
	{
		const ss::live::Occurrence& O = Event;
		const FString Name = Plain(O.T->Name).ToUpper();
		Slides.Add(Series());
		FEcxSlide Main;
		if (bCancelled)
		{
			Main.Kicker = DayWordOf(O, BaseMinutes);
			Main.Title = FString::Printf(TEXT("%s CANCELLED"), *Str(O.T->Short).ToUpper());
			Main.Detail = TEXT("ENTRIES REFUNDED AT THE DESK");
			Ticker = FString::Printf(TEXT("TONIGHT'S %s IS CANCELLED  \xB7  ENTRIES REFUNDED AT THE DESK"), *Str(O.T->Short).ToUpper());
		}
		else if (bStarted)
		{
			// The level running now (breaks every few levels).
			const double Elapsed = BaseMinutes - O.Start;
			const double Cycle = O.T->BreakEvery * O.T->LevelMinutes + O.T->BreakMinutes;
			const int32 Cycles = FMath::FloorToInt(Elapsed / FMath::Max(Cycle, 1.0));
			const double Rest = Elapsed - Cycles * Cycle;
			const int32 Level = Cycles * O.T->BreakEvery + FMath::Min(O.T->BreakEvery - 1, FMath::FloorToInt(Rest / FMath::Max(O.T->LevelMinutes, 1.0))) + 1;
			Main.Kicker = TEXT("NOW PLAYING");
			Main.Title = Name;
			Main.Detail = BaseMinutes < O.LateRegEnds ? FString::Printf(TEXT("LEVEL %d  \xB7  LATE REG UNTIL %s"), Level, *Clock(O.LateRegEnds))
													  : FString::Printf(TEXT("LEVEL %d  \xB7  REGISTRATION CLOSED"), Level);
			Ticker = TEXT("NOW PLAYING  \xB7  ") + Name + TEXT("  \xB7  ") + Main.Detail;
		}
		else
		{
			Main.Kicker = DayWordOf(O, BaseMinutes);
			Main.Title = Name;
			Main.Detail = Cards(O);
			Ticker = Main.Kicker + TEXT("  \xB7  ") + Name + TEXT("  \xB7  ") + Main.Detail;
		}
		Slides.Add(Main);
		Slides.Add(Schedule(O.Day));
		if (Board.Num() > 0)
		{
			// The champions' wall's latest line ("SAT NIGHTLY\tMei\t221500").
			TArray<FString> Parts;
			Board[0].ParseIntoArray(Parts, TEXT("\t"), false);
			if (Parts.Num() >= 3)
			{
				FEcxSlide Champ;
				Champ.Kicker = TEXT("LATEST CHAMPION");
				Champ.Title = Parts[1].ToUpper();
				Champ.Detail = Parts[0].ToUpper() + TEXT("  \xB7  ") + Dollars(FCString::Atoi64(*Parts[2]));
				Slides.Add(Champ);
			}
		}
		Slides.Add(Grill());
	}
	else
	{
		// No event to show (walking round, or an entry already settled): what's next.
		Slides.Add(Series());
		const ss::live::Occurrence Next = NextEvent(BaseMinutes);
		if (Next.Valid())
		{
			FEcxSlide Main;
			Main.Kicker = TEXT("NEXT  \xB7  ") + DayWordOf(Next, BaseMinutes);
			Main.Title = Plain(Next.T->Name).ToUpper();
			Main.Detail = Cards(Next);
			Slides.Add(Main);
			Slides.Add(Schedule(Next.Day));
			Ticker = Main.Kicker + TEXT("  \xB7  ") + Main.Title + TEXT("  \xB7  ") + Main.Detail;
		}
		Slides.Add(Grill());
		if (Ticker.IsEmpty())
		{
			Ticker = TEXT("EMBERCREST RESORT  \xB7  CASINO  \xB7  POKER SERIES");
		}
	}
	Stage->SetMarquee(Slides);
	Stage->SetTicker(Ticker);
}

// ------------------------------------------------------------------ the flows

void AEmbercrestGameMode::PlaceHero(EEcxSpawn Where, float Pitch)
{
	AShortStackCharacter* Hero = GetHero();
	if (!Hero || !Stage)
	{
		return;
	}
	const FTransform T = Stage->SpawnAt(Where);
	Hero->SetActorLocationAndRotation(T.GetLocation(), FRotator(0.0f, T.Rotator().Yaw, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
	if (UCharacterMovementComponent* Move = Hero->GetCharacterMovement())
	{
		Move->StopMovementImmediately();
	}
	if (AController* C = Hero->GetController())
	{
		C->SetControlRotation(FRotator(Pitch, T.Rotator().Yaw, 0.0f));
	}
}

void AEmbercrestGameMode::StartArrival(bool bCinematic)
{
	bLeaving = false;
	Flow = EEcxFlow::Playing;
	FlowAt = RealTime;
	bBlendedIn = false;
	bGreetedCanopy = false;
	DepartAt = -1.0;
	LeaveAt = -1.0;
	if (!Stage)
	{
		return;
	}
	Stage->SetBusSign(false);
	Stage->SetRevolvingFast(false);
	APlayerController* Pc = GetWorld()->GetFirstPlayerController();
	AShortStackCharacter* Hero = GetHero();
	if (!bCinematic)
	{
		// Walking round (no event): on the platform, the bus at the stop with its doors open, about to leave.
		Stage->BusAtStop(true);
		BeginControl(true);
		if (Pc && Pc->PlayerCameraManager)
		{
			Pc->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 1.0f, FLinearColor::Black, true, false);
		}
		return;
	}
	Flow = EEcxFlow::Cinematic;
	Stage->SetBusBlocking(false);
	Stage->SetStepOffFloor(false);
	Stage->BusArrive();
	// The hero rides in on the bus: out of sight until its door opens.
	if (Hero)
	{
		PlaceHero(EEcxSpawn::ArriveInside, 8.0f);
		Hero->SetActorHiddenInGame(true);
		if (UCharacterMovementComponent* Move = Hero->GetCharacterMovement())
		{
			Move->DisableMovement();
		}
	}
	Stage->SetCineCamera(CraneFrom, CraneLook, 65.0f);
	if (Pc)
	{
		Pc->SetViewTarget(Stage);
		if (Pc->PlayerCameraManager)
		{
			Pc->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 1.0f, FLinearColor::Black, true, false);
		}
	}
}

void AEmbercrestGameMode::TickCinematic()
{
	AShortStackCharacter* Hero = GetHero();
	APlayerController* Pc = GetWorld()->GetFirstPlayerController();
	const double T = RealTime - FlowAt;
	if (T < ShotB)
	{
		// The crane, easing in and down over the valley.
		const float U = FMath::SmoothStep(0.0f, 1.0f, static_cast<float>(T / ShotB));
		Stage->SetCineCamera(FMath::Lerp(CraneFrom, CraneTo, static_cast<double>(U)), CraneLook, 65.0f);
	}
	else if (Flow == EEcxFlow::Cinematic)
	{
		Stage->SetCineCamera(StopFrom, StopLook, 55.0f);
	}
	if (Flow == EEcxFlow::Cinematic && T >= StepOffAt)
	{
		// The doors are open: the hero steps down onto the platform.
		Flow = EEcxFlow::StepOff;
		Stage->SetStepOffFloor(true);
		if (Hero)
		{
			PlaceHero(EEcxSpawn::ArriveInside, 8.0f);
			// Short before the camera blends to them (at 7.4 s): the arm eases in meanwhile, so it never sits inside the bus.
			HoldArmShort(true);
			Hero->SetActorHiddenInGame(false);
			if (UCharacterMovementComponent* Move = Hero->GetCharacterMovement())
			{
				Move->SetMovementMode(MOVE_Walking);
			}
		}
	}
	if (Flow != EEcxFlow::StepOff)
	{
		return;
	}
	if (Hero)
	{
		Hero->Drive(1.0f, 0.0f, false);
	}
	if (!bBlendedIn && T >= BlendAt && Pc && Hero)
	{
		bBlendedIn = true;
		Pc->SetViewTargetWithBlend(Hero, 0.8f, VTBlend_EaseInOut, 2.0f);
	}
	const FVector At = Hero && Stage ? Stage->GetActorTransform().InverseTransformPosition(Hero->GetActorLocation()) : FVector::ZeroVector;
	if (T >= ControlAt || (Hero && At.X <= 5150.0 && T >= BlendAt + 0.3))
	{
		BeginControl(false);
	}
}

void AEmbercrestGameMode::BeginControl(bool bCut)
{
	APlayerController* Pc = GetWorld()->GetFirstPlayerController();
	AShortStackCharacter* Hero = GetHero();
	if (bCut)
	{
		// Skipped (or nothing to show): the bus stopped with its doors open, the hero on the platform, the camera behind.
		if (Stage->GetBusPhase() != EEcxBusPhase::Stopped)
		{
			Stage->BusAtStop(true);
		}
		if (Hero)
		{
			PlaceHero(EEcxSpawn::ArrivePlatform, 8.0f);
			Hero->SetActorHiddenInGame(false);
			if (UCharacterMovementComponent* Move = Hero->GetCharacterMovement())
			{
				Move->SetMovementMode(MOVE_Walking);
			}
		}
		if (Pc && Hero)
		{
			Pc->SetViewTarget(Hero);
			if (Pc->PlayerCameraManager && Flow == EEcxFlow::Cinematic)
			{
				Pc->PlayerCameraManager->StopCameraFade();
			}
		}
	}
	else if (Pc && Hero && !bBlendedIn)
	{
		Pc->SetViewTargetWithBlend(Hero, 0.5f, VTBlend_EaseInOut, 2.0f);
	}
	bBlendedIn = true;
	Flow = EEcxFlow::Playing;
	FlowAt = RealTime;
	HintsAt = RealTime + 0.5;
	// The bus stands just behind the hero on the platform until it pulls away (Tick lets the arm out again).
	HoldArmShort(true);
	Stage->SetStepOffFloor(false);
	Stage->SetBusBlocking(true);
	// Three seconds after control the doors close and the 14 heads back down (spec 5.2).
	DepartAt = RealTime + 3.0;
}

void AEmbercrestGameMode::StartLeaving()
{
	bLeaving = true;
	Flow = EEcxFlow::Playing;
	FlowAt = RealTime;
	DepartAt = -1.0;
	LeaveAt = -1.0;
	HoldArmShort(false);
	if (!Stage)
	{
		return;
	}
	// Out of the revolving door, facing the valley; the 14 idling at the stop for the ride home, its doors shut.
	Stage->SetBusSign(true);
	Stage->BusAtStop(false);
	Stage->SetBusBlocking(true);
	Stage->SetStepOffFloor(false);
	Stage->SetRevolvingFast(false);
	AShortStackCharacter* Hero = GetHero();
	if (Hero)
	{
		PlaceHero(EEcxSpawn::Leave, -3.0f);
		Hero->SetActorHiddenInGame(false);
		if (UCharacterMovementComponent* Move = Hero->GetCharacterMovement())
		{
			Move->SetMovementMode(MOVE_Walking);
		}
	}
	if (APlayerController* Pc = GetWorld()->GetFirstPlayerController())
	{
		if (Hero)
		{
			Pc->SetViewTarget(Hero);
		}
		if (Pc->PlayerCameraManager)
		{
			Pc->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 1.5f, FLinearColor::Black, true, false);
		}
	}
	HintsAt = RealTime + 0.5;
}

void AEmbercrestGameMode::GoInside()
{
	AShortStackCharacter* Hero = GetHero();
	if (!Hero || !Stage || Flow != EEcxFlow::Playing)
	{
		return;
	}
	Flow = EEcxFlow::Entering;
	FlowAt = RealTime;
	LeaveAt = RealTime + 1.05;
	// Toward the nearest door: the revolving door speeds up, or the swing leaf opens; the casino's chimes swell.
	const int32 Door = Stage->NearestDoor(Hero->GetActorLocation(), Approach);
	if (Door == 0)
	{
		Stage->SetRevolvingFast(true);
	}
	else
	{
		Stage->OpenSwingDoor(Door);
	}
	if (APlayerController* Pc = GetWorld()->GetFirstPlayerController())
	{
		if (Pc->PlayerCameraManager)
		{
			Pc->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, 1.0f, FLinearColor::Black, true, true);
		}
	}
	if (Tone)
	{
		Tone->FadeOut(1.1f);
	}
	UE_LOG(LogEmbercrestExterior, Log, TEXT("Through door %d into the card room (%s)"), Door, *Without(OptionsString, nullptr));
}

void AEmbercrestGameMode::BoardBus()
{
	AShortStackCharacter* Hero = GetHero();
	if (!Hero || !Stage || Flow != EEcxFlow::Playing)
	{
		return;
	}
	Flow = EEcxFlow::Boarding;
	FlowAt = RealTime;
	LeaveAt = RealTime + 1.25;
	Approach = Stage->BusDoorPoint();
	if (Tone)
	{
		Tone->Hiss(0.6f, 0.8f);
		Tone->FadeOut(1.25f);
	}
	if (APlayerController* Pc = GetWorld()->GetFirstPlayerController())
	{
		if (Pc->PlayerCameraManager)
		{
			Pc->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, 1.2f, FLinearColor::Black, true, true);
		}
	}
	UE_LOG(LogEmbercrestExterior, Log, TEXT("On the 14 for home (%s)"), *Without(OptionsString, TEXT("Leaving")));
}

void AEmbercrestGameMode::LookOut(bool bViewer)
{
	APlayerController* Pc = GetWorld()->GetFirstPlayerController();
	if (!Stage || !Pc || Flow != EEcxFlow::Playing)
	{
		return;
	}
	LookReturn = Flow;
	Flow = EEcxFlow::Looking;
	LookUntil = RealTime + (bViewer ? 9.0 : 7.0);
	if (bViewer)
	{
		// Through the coin viewer: downtown, ten kilometers off and nine hundred meters down.
		Stage->SetCineCamera(FVector(6930.0, 900.0, Ecx::ZOverlook + 150.0), FVector(1050000.0, 100000.0, -90000.0), 9.0f);
		Toast(TEXT("The viewer"), TEXT("It clicks open. The city's grid of lights, close enough to read the river by."));
	}
	else
	{
		Stage->SetCineCamera(FVector(6900.0, 1300.0, -60.0), FVector(1000000.0, -100000.0, -90000.0), 60.0f);
		Toast(TEXT("The overlook"), TEXT("The city's lights, nine hundred meters down under the mist."));
	}
	Pc->SetViewTargetWithBlend(Stage, bViewer ? 0.4f : 1.2f, VTBlend_EaseInOut, 2.0f);
	if (Audio)
	{
		Audio->Play(ss::SoundId::Click, 0.35f);
	}
}

void AEmbercrestGameMode::EndLook()
{
	if (Flow != EEcxFlow::Looking)
	{
		return;
	}
	Flow = LookReturn;
	LookUntil = -1.0;
	if (APlayerController* Pc = GetWorld()->GetFirstPlayerController())
	{
		if (AShortStackCharacter* Hero = GetHero())
		{
			Pc->SetViewTargetWithBlend(Hero, 0.8f, VTBlend_EaseInOut, 2.0f);
		}
	}
}

void AEmbercrestGameMode::Greet(const FVector& At)
{
	if (!Stage)
	{
		return;
	}
	const FVector L = Stage->GetActorTransform().InverseTransformPosition(At);
	const bool bDay = Stage->GetSunElevation() > 4.0;
	if (!bLeaving)
	{
		// Under the canopy's columns on the way in: the doorman's word (spec 5.2, 5.5).
		if (!bGreetedCanopy && L.X < 1950.0 && FMath::Abs(L.Y) < Ecx::CanopyHalf)
		{
			bGreetedCanopy = true;
			// By the clock, not the light: the noon game's players come up the drive before twelve.
			const double Hour = LightHour();
			FString Line = Hour >= 4.0 && Hour < 12.0 ? TEXT("Morning. Poker room's upstairs.")
				: Hour >= 12.0 && Hour < 17.0		  ? TEXT("Afternoon. Poker room's upstairs.")
													  : TEXT("Evening. Poker room's upstairs, past the cage.");
			if (bCancelled)
			{
				Line = bDay ? TEXT("Desk's refunding today's entries, I'm afraid. Go on up anyway.") : TEXT("Desk's refunding tonight's entries, I'm afraid. Go on up anyway.");
			}
			else if (bStarted)
			{
				Line = TEXT("They're already shuffling up. Go on in.");
			}
			Toast(TEXT("Orlando, the doorman"), Line);
		}
		return;
	}
	// Leaving, the doors aren't for going back in (said when the hero turns back to them, not as they come out).
	const AShortStackCharacter* Hero = GetHero();
	const FVector Doors = Stage->GetActorTransform().TransformPosition(FVector(0.0, 0.0, 0.0));
	const bool bFacing = Hero && FVector::DotProduct(Hero->LookDirection().GetSafeNormal2D(), (Doors - At).GetSafeNormal2D()) > 0.4;
	if (bFacing && Stage->DoorSpotNear(At) && RealTime - GreetedAt > 12.0)
	{
		GreetedAt = RealTime;
		Toast(TEXT("Orlando, the doorman"), bWon ? TEXT("Heard you took it down. Congratulations.") : TEXT("Night's over for us. The 14's waiting."));
	}
}

// ------------------------------------------------------------------ the frame

void AEmbercrestGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Game)
	{
		return;
	}
	const double Dt = FMath::Clamp(static_cast<double>(DeltaSeconds), 0.0, 0.05);
	RealTime += Dt;
	FrameBudget.Tick(DeltaSeconds);
	if (SettingsDirtyAt >= 0.0 && RealTime - SettingsDirtyAt > 0.75)
	{
		SaveSettingsNow();
	}
	if (Stage)
	{
		// The light follows the night's hour (or ss.ECX.Hour, set live).
		Stage->SetHour(LightHour());
	}
	AShortStackCharacter* Hero = GetHero();
	APlayerController* Pc = GetWorld()->GetFirstPlayerController();
	if (Stage && IsCinematic() && !Game->Menu.IsPaused())
	{
		TickCinematic();
	}
	if (Stage && DepartAt >= 0.0 && RealTime >= DepartAt)
	{
		DepartAt = -1.0;
		Stage->BusDepart();
	}
	if (SavedArm >= 0.0f && Stage &&
		(Stage->GetBusPhase() == EEcxBusPhase::Gone || (Stage->GetBusPhase() == EEcxBusPhase::Departing && Stage->GetBusClock() > 7.0f)))
	{
		// Out in the lane and up the road (its rear past the platform's door): the arm eases back out (the hero's own easing).
		HoldArmShort(false);
	}
	if (Stage && Hero && bLeaving && Flow == EEcxFlow::Playing)
	{
		// The bus opens for someone coming to it, and shuts again if they wander off (spec 5.3).
		const double D = FVector::Dist2D(Hero->GetActorLocation(), Stage->BusDoorPoint());
		if (D < 800.0)
		{
			Stage->BusSetDoors(true);
		}
		else if (D > 1500.0)
		{
			Stage->BusSetDoors(false);
		}
	}
	if (Hero && (Flow == EEcxFlow::Entering || Flow == EEcxFlow::Boarding))
	{
		// A step or two toward the door as the screen goes dark.
		FVector To = Approach - Hero->GetActorLocation();
		To.Z = 0.0;
		if (To.Size() > 60.0)
		{
			if (AController* C = Hero->GetController())
			{
				C->SetControlRotation(FRotator(C->GetControlRotation().Pitch, To.Rotation().Yaw, 0.0f));
			}
			Hero->Drive(1.0f, 0.0f, false);
		}
	}
	if (Flow == EEcxFlow::Looking && RealTime >= LookUntil)
	{
		EndLook();
	}
	if (LeaveAt >= 0.0 && RealTime >= LeaveAt)
	{
		LeaveAt = -1.0;
		if (Flow == EEcxFlow::Entering)
		{
			// Into the card room with exactly the options this level was given (none: the practice table).
			UGameplayStatics::OpenLevel(this, FName(TEXT("BackRoom")), true, Without(OptionsString, nullptr));
			return;
		}
		if (Flow == EEcxFlow::Boarding)
		{
			// Home: the card room's options, byte for byte, less Leaving.
			UGameplayStatics::OpenLevel(this, FName(TEXT("NightOne")), true, Without(OptionsString, TEXT("Leaving")));
			return;
		}
	}
	if (Hero && Stage)
	{
		KeepOnTheSet();
		if (Flow == EEcxFlow::Playing)
		{
			Greet(Hero->GetActorLocation());
		}
		float Volume = 0.0f;
		if (Hero->TakeFootstep(Volume) && Audio && !Hero->IsHidden())
		{
			// Snow crunches; the heated stone is always wet.
			FHitResult Hit;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(EcxStep), true, Hero);
			Params.bReturnFaceIndex = true;
			const FVector From = Hero->GetActorLocation();
			bool bSnow = false;
			if (GetWorld()->LineTraceSingleByChannel(Hit, From, From - FVector(0.0, 0.0, 250.0), ECC_Visibility, Params))
			{
				bSnow = Stage->IsSnow(Hit);
			}
			if (bSnow && Tone)
			{
				Tone->Crunch(Volume);
			}
			else
			{
				Audio->PlayEffect(ss::audio::Effect::Step, Volume * 0.85f);
				if (Tone)
				{
					Tone->Splash(Volume * 0.7f);
				}
			}
		}
	}
	else if (!Hero && GetWorld() && Flow == EEcxFlow::Playing)
	{
		// The pawn is gone (fell below the world): a new one at the mode's spawn.
		if (Pc && !Pc->GetPawn())
		{
			UE_LOG(LogEmbercrestExterior, Warning, TEXT("The hero was lost: back at the spawn"));
			bDressed = false;
			SavedArm = -1.0f; // a new hero, at its own arm's length
			RestartPlayer(Pc);
			ApplySettings(Game->Menu.Settings, false);
		}
	}
	CullIn -= Dt;
	if (CullIn <= 0.0 && Pc && Pc->PlayerCameraManager)
	{
		CullIn = 0.25;
		CullPeople(Pc->PlayerCameraManager->GetCameraLocation());
	}
	UpdateSound(static_cast<float>(Dt));
	DrawHud();
	if (Game->Menu.IsOpen() || (MenuWidget && MenuWidget->GetVisibility() == EVisibility::Visible))
	{
		if (MenuWidget)
		{
			const FVector2D Size = ViewSize();
			const float H = ss::ui::FrontEnd::Height;
			TSharedPtr<ss::ui::DrawList> List = MakeShared<ss::ui::DrawList>();
			ss::ui::Canvas Cv(*List, Game->Measurer, H * static_cast<float>(Size.X / Size.Y), H, static_cast<float>(Size.Y) / H);
			Game->Menu.Draw(Cv, RealTime);
			MenuWidget->SetDrawList(List);
		}
		if (!Game->Menu.IsOpen() && Game->Menu.Backdrop(RealTime) <= 0.001f)
		{
			ReturnInputToGame();
		}
	}
	Toasts.RemoveAll([this](const ss::ui::StreetHudInfo::Toast& T) { return RealTime - T.At > 10.0; });
}

void AEmbercrestGameMode::UpdateSound(float Dt)
{
	if (!Stage || !Tone)
	{
		return;
	}
	const APlayerController* Pc = GetWorld()->GetFirstPlayerController();
	const AShortStackCharacter* Hero = GetHero();
	const FVector Ear = Pc && Pc->PlayerCameraManager ? Pc->PlayerCameraManager->GetCameraLocation() : Hero ? Hero->GetActorLocation() : FVector::ZeroVector;
	// The cinematic's crane is far out over the valley: the place's sounds are the hero's, once there's a hero to hear them.
	Tone->SetPlace(Stage->FireNear(Ear), Stage->FireFlicker(Ear), Stage->WaterNear(Ear), Stage->DoorsNear(Ear), Stage->ValleyNear(Ear));
	Tone->SetDoorway(Stage->RevolvingSpeed(), Flow == EEcxFlow::Entering ? 1.0f : 0.0f);
	const EEcxBusPhase Bus = Stage->GetBusPhase();
	const float Pull = Bus == EEcxBusPhase::Departing ? FMath::Clamp((Stage->GetBusClock() - 1.6f) / 6.0f, 0.0f, 1.0f) : Bus == EEcxBusPhase::Arriving ? 0.5f : 0.0f;
	Tone->SetBus(Stage->BusNear(Ear), Bus != EEcxBusPhase::Gone, Pull);
	Tone->SetSun(static_cast<float>(Stage->GetSunElevation()));
	Tone->SetPaused(Game->Menu.IsPaused());
	const uint32 Events = Stage->TakeBusEvents();
	if (Events & EcxBusEvent::Brakes)
	{
		Tone->Hiss(1.1f, 1.0f);
	}
	if (Events & EcxBusEvent::Kneel)
	{
		Tone->Hiss(0.6f, 0.6f);
	}
	if (Events & (EcxBusEvent::DoorsOpen | EcxBusEvent::DoorsClose))
	{
		Tone->Hiss(0.5f, 0.85f);
	}
	if ((Events & EcxBusEvent::DoorsOpen) && Audio)
	{
		// The two-note chime as the doors part, at the bus's distance.
		Audio->PlayEffect(ss::audio::Effect::Chime, 0.3f * FMath::Max(0.15f, Stage->BusNear(Ear)));
	}
}

void AEmbercrestGameMode::KeepOnTheSet()
{
	AShortStackCharacter* Hero = GetHero();
	if (!Hero || !Stage || Flow != EEcxFlow::Playing || !Stage->IsOffTheSet(Hero->GetActorLocation()))
	{
		return;
	}
	UE_LOG(LogEmbercrestExterior, Warning, TEXT("Off the set at %s: back to the spawn"), *Hero->GetActorLocation().ToCompactString());
	PlaceHero(bLeaving ? EEcxSpawn::Leave : EEcxSpawn::ArrivePlatform, bLeaving ? -3.0f : 8.0f);
}

// ------------------------------------------------------------------ the HUD and the menu

void AEmbercrestGameMode::CreateWidgets()
{
	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (!Viewport || !FSlateApplication::IsInitialized())
	{
		return;
	}
	HudWidget = SNew(SDrawListWidget);
	HudWidget->SetVisibility(EVisibility::HitTestInvisible);
	Viewport->AddViewportWidgetContent(HudWidget.ToSharedRef(), 10);
	TWeakObjectPtr<AEmbercrestGameMode> WeakThis = this;
	MenuWidget = SNew(SFrontEndWidget);
	MenuWidget->SetVisibility(EVisibility::Collapsed);
	MenuWidget->OnMenuKey = [WeakThis](const FString& KeyName, bool bFromGamepad) {
		if (WeakThis.IsValid() && WeakThis->Game)
		{
			WeakThis->bGamepad = bFromGamepad;
			WeakThis->Game->Menu.Gamepad = bFromGamepad;
			WeakThis->Game->Menu.Key(Utf8(KeyName), WeakThis->RealTime);
		}
	};
	MenuWidget->OnMenuChar = [WeakThis](uint32 Codepoint) {
		if (WeakThis.IsValid() && WeakThis->Game)
		{
			WeakThis->Game->Menu.Char(Codepoint, WeakThis->RealTime);
		}
	};
	MenuWidget->OnMenuPointer = [WeakThis](const FVector2D& Logical, int32 Button, float WheelDelta) {
		if (WeakThis.IsValid() && WeakThis->Game)
		{
			ss::ui::Pointer& P = WeakThis->Game->Menu.Ptr;
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
		}
	};
	Viewport->AddViewportWidgetContent(MenuWidget.ToSharedRef(), 30);
}

void AEmbercrestGameMode::FocusWidget(SFrontEndWidget* Widget)
{
	if (!Widget)
	{
		return;
	}
	Widget->SetVisibility(EVisibility::Visible);
	if (APlayerController* Pc = UGameplayStatics::GetPlayerController(this, 0))
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(Widget->AsShared());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Pc->SetInputMode(InputMode);
		Pc->bShowMouseCursor = true;
	}
	FSlateApplication::Get().SetKeyboardFocus(Widget->AsShared());
}

void AEmbercrestGameMode::ReturnInputToGame()
{
	if (MenuWidget && Game && !Game->Menu.IsOpen())
	{
		MenuWidget->SetVisibility(EVisibility::Collapsed);
	}
	if (APlayerController* Pc = UGameplayStatics::GetPlayerController(this, 0))
	{
		Pc->SetInputMode(FInputModeGameOnly());
		Pc->bShowMouseCursor = false;
	}
}

bool AEmbercrestGameMode::IsUiOpen() const
{
	return Game && Game->Menu.IsOpen();
}

FVector2D AEmbercrestGameMode::ViewSize() const
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

void AEmbercrestGameMode::DrawHud()
{
	if (!HudWidget)
	{
		return;
	}
	AShortStackCharacter* Hero = GetHero();
	const FVector2D Size = ViewSize();
	const float H = ss::ui::StoreCounter::Height;
	const float W = H * static_cast<float>(Size.X / Size.Y);
	TSharedPtr<ss::ui::DrawList> List = MakeShared<ss::ui::DrawList>();
	if (IsCinematic())
	{
		// The arrival's shots in letterbox, the skip on offer after half a second.
		FEcxPaint P(*List, W, H);
		const float Bar = H * 0.09f * FMath::Clamp(static_cast<float>((ControlAt - (RealTime - FlowAt)) / 0.6), 0.0f, 1.0f);
		P.Rect(0.0f, 0.0f, W, Bar, FEcxPaint::Ink(0x000000));
		P.Rect(0.0f, H - Bar, W, Bar, FEcxPaint::Ink(0x000000));
		if (RealTime - FlowAt > 0.5 && Flow == EEcxFlow::Cinematic)
		{
			P.Text(bGamepad ? FString(TEXT("SKIP  (A)")) : FString(TEXT("SKIP  [E]")), W - 48.0f, H - Bar * 0.5f, 24.0f, 700, FEcxPaint::Ink(0xebe7df, 0.8f), 2);
		}
		HudWidget->SetDrawList(List);
		return;
	}
	ss::ui::Canvas Cv(*List, Game->Measurer, W, H, static_cast<float>(Size.Y) / H);
	if (!Hero || bShotHold)
	{
		HudWidget->SetDrawList(List);
		return;
	}
	ss::ui::StreetHudInfo Info;
	Info.Place = Stage ? Utf8(Stage->PlaceName(Hero->GetActorLocation())) : "THE EMBERCREST";
	Info.Clock = ss::net::TimeLabel(ShownMinutes());
	Info.World = ShownMinutes();
	Info.BankrollCents = bCareer ? Save.BankrollCents : 0;
	Info.Life = bCareer ? &Save.Life : nullptr;
	Info.FirstPerson = Hero->IsFirstPerson();
	Info.HintsAt = Game->Menu.Settings.ShowHints ? HintsAt : -100.0;
	Info.Gamepad = bGamepad;
	for (const ss::ui::StreetHudInfo::Toast& T : Toasts)
	{
		Info.Toasts.push_back(T);
	}
	if (!IsUiOpen() && Flow == EEcxFlow::Playing && Stage)
	{
		if (const FEcxSpot* Spot = Stage->SpotFor(Hero->EyeLocation(), Hero->LookDirection(), bLeaving, Stage->GetBusDoors() > 0.9f))
		{
			Info.Prompt = Utf8(Spot->Prompt);
		}
	}
	ss::ui::DrawStreetHud(Cv, Info, RealTime);
	HudWidget->SetDrawList(List);
}

void AEmbercrestGameMode::Toast(const FString& From, const FString& Body)
{
	ss::ui::StreetHudInfo::Toast T;
	T.From = Utf8(From);
	T.Body = Utf8(Body);
	T.At = RealTime;
	Toasts.Add(T);
	while (Toasts.Num() > 4)
	{
		Toasts.RemoveAt(0);
	}
	if (Audio)
	{
		Audio->Play(ss::SoundId::Click, 0.25f);
	}
}

// ------------------------------------------------------------------ input

void AEmbercrestGameMode::OnMove(float Forward, float Right, bool bRun)
{
	if (AShortStackCharacter* Hero = GetHero(); Hero && Flow == EEcxFlow::Playing)
	{
		Hero->Drive(Forward, Right, bRun);
	}
}

void AEmbercrestGameMode::OnLook(float DeltaYaw, float DeltaPitch)
{
	if (AShortStackCharacter* Hero = GetHero(); Hero && (Flow == EEcxFlow::Playing || Flow == EEcxFlow::Entering || Flow == EEcxFlow::Boarding))
	{
		Hero->Look(DeltaYaw, bInvertLook ? -DeltaPitch : DeltaPitch);
	}
}

void AEmbercrestGameMode::OnToggleView()
{
	if (AShortStackCharacter* Hero = GetHero(); Hero && Flow == EEcxFlow::Playing)
	{
		Hero->ToggleView();
		if (Audio)
		{
			Audio->Play(ss::SoundId::Click, 0.3f);
		}
	}
}

void AEmbercrestGameMode::OnZoom(float Delta)
{
	if (AShortStackCharacter* Hero = GetHero(); Hero && !Hero->IsFirstPerson())
	{
		// The player's wheel wins over the arm held short for the bus (the bus's box still keeps the camera out of it).
		SavedArm = -1.0f;
		Hero->ArmLength = FMath::Clamp(Hero->ArmLength + Delta, 160.0f, 460.0f);
	}
}

void AEmbercrestGameMode::HoldArmShort(bool bShort)
{
	AShortStackCharacter* Hero = GetHero();
	if (!Hero)
	{
		SavedArm = -1.0f;
		return;
	}
	if (bShort && SavedArm < 0.0f)
	{
		// From the platform spawn (X 5140) to the bus's side (X 5265): 1.1 m of room, less the arm's probe.
		SavedArm = Hero->ArmLength;
		Hero->ArmLength = FMath::Min(Hero->ArmLength, 105.0f);
	}
	else if (!bShort && SavedArm >= 0.0f)
	{
		Hero->ArmLength = SavedArm;
		SavedArm = -1.0f;
	}
}

void AEmbercrestGameMode::OnSkip()
{
	if (IsCinematic())
	{
		UE_LOG(LogEmbercrestExterior, Log, TEXT("The arrival skipped at %.1f s"), RealTime - FlowAt);
		BeginControl(true);
	}
}

void AEmbercrestGameMode::OnInteract()
{
	if (Flow == EEcxFlow::Looking)
	{
		EndLook();
		return;
	}
	AShortStackCharacter* Hero = GetHero();
	if (!Hero || !Stage || Flow != EEcxFlow::Playing || IsUiOpen())
	{
		return;
	}
	const FEcxSpot* Spot = Stage->SpotFor(Hero->EyeLocation(), Hero->LookDirection(), bLeaving, Stage->GetBusDoors() > 0.9f);
	if (!Spot)
	{
		return;
	}
	const FString Id = Spot->Id.ToString();
	if (Id.StartsWith(TEXT("doors")))
	{
		GoInside();
	}
	else if (Id == TEXT("bus"))
	{
		BoardBus();
	}
	else if (Id == TEXT("overlook") || Id == TEXT("viewer"))
	{
		LookOut(Id == TEXT("viewer"));
	}
}

void AEmbercrestGameMode::OnPause()
{
	if (!Game || Flow == EEcxFlow::Entering || Flow == EEcxFlow::Boarding)
	{
		return;
	}
	// The arrival's shots skip first (spec 5.4); a look out over the valley ends.
	OnSkip();
	EndLook();
	ss::ui::FrontEndInfo& I = Game->Menu.Info;
	I = ss::ui::FrontEndInfo();
	I.HasSave = bCareer;
	if (bCareer)
	{
		I.HeroName = Save.HeroName;
		I.Person = Save.Person;
		I.BankrollCents = Save.BankrollCents;
		I.Tournaments = static_cast<int>(Save.History.size());
		I.RentStage = Save.Life.RentStage;
		I.RentDueCents = Save.Life.RentDueCents;
		I.RentDeadline = Save.Life.RentDeadline;
		I.StorageDue = Save.Life.StorageDue;
		I.Status.push_back({"BANKROLL", Utf8(Dollars(Save.BankrollCents))});
	}
	I.World = ShownMinutes();
	I.Status.push_back({"TIME", ss::net::TimeLabel(ShownMinutes())});
	if (Event.Valid() && !bLeaving)
	{
		I.Status.push_back({"EVENT", Event.T->Name});
	}
	I.Resolutions = Resolutions();
	// What quitting here keeps (spec 5.4): the seat held for an arrival, the settled night for the way home.
	if (bLeaving)
	{
		I.QuitKept = "The night's already settled. You'll wake up at home.";
	}
	else if (Event.Valid())
	{
		I.QuitKept = "Your seat's held until late registration closes at " + ss::net::TimeLabel(Event.LateRegEnds) + ".";
	}
	Game->Menu.Gamepad = bGamepad;
	Game->Menu.SetVenue(ss::ui::Venue::Street, "THE EMBERCREST");
	Game->Menu.Open(ss::ui::FrontEnd::Page::Pause, RealTime);
	FocusWidget(MenuWidget.Get());
}

void AEmbercrestGameMode::LeaveForMenu()
{
	SaveSettingsNow();
	UGameplayStatics::OpenLevel(this, FName(TEXT("NightOne")), true, TEXT("Menu"));
}

void AEmbercrestGameMode::LeaveGame()
{
	SaveSettingsNow();
	UKismetSystemLibrary::QuitGame(this, UGameplayStatics::GetPlayerController(this, 0), EQuitPreference::Quit, false);
}

// ------------------------------------------------------------------ settings

void AEmbercrestGameMode::ApplySettings(const ss::ui::GameSettings& Settings, bool bSave)
{
	// As the street applies them: scalability, the render scale and the frame cap, ray-traced Lumen, the lens, the field
	// of view, the mix and the controls.
	if (UGameUserSettings* User = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		User->SetOverallScalabilityLevel(FMath::Clamp(Settings.Quality, 0, 4));
		User->SetResolutionScaleValueEx(static_cast<float>(FMath::Min(Settings.ResolutionScale, 100)));
		User->SetFrameRateLimit(static_cast<float>(Settings.FrameRateLimit));
		User->SetVSyncEnabled(Settings.VSync);
		if (GIsEditor)
		{
			User->ApplyNonResolutionSettings();
		}
		else
		{
			const FIntPoint Res = ParseResolution(Settings.Resolution);
			User->SetScreenResolution(Res.X > 0 ? Res : User->GetDesktopResolution());
			User->SetFullscreenMode(Settings.WindowMode == 0 ? EWindowMode::Fullscreen : Settings.WindowMode == 1 ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed);
			User->ApplySettings(false);
		}
	}
	FrameBudget.Configure(Settings.DynamicTarget, Settings.ResolutionScale);
	SetGameCvar(TEXT("r.Lumen.HardwareRayTracing"), Settings.RayTracing ? 1 : 0);
	SetGameCvar(TEXT("r.Lumen.HardwareRayTracing.LightingMode"), Settings.RayTracing && Settings.Quality >= 4 ? 2 : 0);
	if (Stage)
	{
		Stage->SetLensOptions(static_cast<float>(Settings.Brightness - 50) / 50.0f * 1.5f, Settings.FilmGrain == 0 ? 0.0f : Settings.FilmGrain == 1 ? 1.0f : 2.2f,
			Settings.ChromaticAberration ? 1.0f : 0.0f, Settings.MotionBlur);
	}
	if (AShortStackCharacter* Hero = GetHero())
	{
		Hero->FirstPersonFov = FovFor(84.0f, Settings.FieldOfView);
		Hero->ThirdPersonFov = FovFor(72.0f, Settings.FieldOfView);
	}
	if (Audio)
	{
		Audio->SetMix(Settings.MasterVolume / 100.0f, Settings.EffectsVolume / 100.0f, Settings.AmbienceVolume / 100.0f);
	}
	FApp::SetUnfocusedVolumeMultiplier(Settings.BackgroundAudio ? 1.0f : 0.0f);
	Sensitivity = 0.11f * static_cast<float>(Settings.LookSensitivity) / 100.0f;
	bInvertLook = Settings.InvertLook;
	if (bSave)
	{
		SettingsDirtyAt = RealTime;
	}
}

void AEmbercrestGameMode::SaveSettingsNow()
{
	SettingsDirtyAt = -1.0;
	if (!Game)
	{
		return;
	}
	if (UNightOneSaveGame* Obj = Cast<UNightOneSaveGame>(UGameplayStatics::CreateSaveGameObject(UNightOneSaveGame::StaticClass())))
	{
		Obj->Data = Str(Game->Menu.Settings.Serialize());
		UGameplayStatics::SaveGameToSlot(Obj, UNightOneSaveGame::SettingsSlotName(), 0);
	}
}

// ------------------------------------------------------------------ automation

FString AEmbercrestGameMode::TestDescribe() const
{
	static const TCHAR* const Flows[6] = {TEXT("cinematic"), TEXT("step-off"), TEXT("playing"), TEXT("entering"), TEXT("boarding"), TEXT("looking")};
	const AShortStackCharacter* Hero = GetHero();
	const FEcxSpot* Spot = Hero && Stage ? Stage->SpotFor(Hero->EyeLocation(), Hero->LookDirection(), bLeaving, Stage->GetBusDoors() > 0.9f) : nullptr;
	return FString::Printf(TEXT("%s, %s | %s at %s %s | spot %s | event %s%s%s | %s"), bLeaving ? TEXT("leaving") : TEXT("arriving"), Flows[FMath::Clamp(static_cast<int32>(Flow), 0, 5)],
		*Clock(ShownMinutes()), Hero ? *Hero->GetActorLocation().ToCompactString() : TEXT("(no hero)"), Hero && Stage ? *Stage->PlaceName(Hero->GetActorLocation()) : TEXT(""),
		Spot ? *Spot->Id.ToString() : TEXT("none"), Event.Valid() ? *Str(Event.Id) : TEXT("none"), bCancelled ? TEXT(" (cancelled)") : TEXT(""), bStarted ? TEXT(" (started)") : TEXT(""),
		Stage ? *Stage->Describe() : TEXT("no stage"));
}

void AEmbercrestGameMode::TestTeleport(const FString& Where)
{
	AShortStackCharacter* Hero = GetHero();
	if (!Hero || !Stage)
	{
		return;
	}
	if (Where == TEXT("arrive") || Where == TEXT("leave"))
	{
		PlaceHero(Where == TEXT("leave") ? EEcxSpawn::Leave : EEcxSpawn::ArrivePlatform, Where == TEXT("leave") ? -3.0f : 8.0f);
		return;
	}
	struct FPlace
	{
		const TCHAR* Name;
		FVector At;
		float Yaw;
	};
	const FPlace Places[5] = {{TEXT("bus"), FVector(5150.0, 100.0, Ecx::ZPlatform + 100.0), 0.0f}, {TEXT("doors"), FVector(300.0, 0.0, 100.0), 180.0f},
		{TEXT("canopy"), FVector(1500.0, 0.0, 100.0), 180.0f}, {TEXT("overlook"), FVector(6700.0, 1300.0, Ecx::ZOverlook + 100.0), 0.0f},
		{TEXT("landingA"), FVector(4140.0, 0.0, Ecx::ZLandA + 100.0), 180.0f}};
	for (const FPlace& P : Places)
	{
		if (Where.Equals(P.Name, ESearchCase::IgnoreCase))
		{
			const FTransform T = FTransform(FRotator(0.0f, P.Yaw, 0.0f), P.At) * Stage->GetActorTransform();
			Hero->SetActorLocation(T.GetLocation(), false, nullptr, ETeleportType::TeleportPhysics);
			if (AController* C = Hero->GetController())
			{
				C->SetControlRotation(FRotator(0.0f, T.Rotator().Yaw, 0.0f));
			}
			return;
		}
	}
}

void AEmbercrestGameMode::TestShot(const FString& Shot)
{
	APlayerController* Pc = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	AShortStackCharacter* Hero = GetHero();
	if (!Pc || !Stage)
	{
		return;
	}
	const int32 N = Shot.Len() >= 2 && (Shot[0] == TEXT('S') || Shot[0] == TEXT('s')) ? FCString::Atoi(*Shot.Mid(1)) : 0;
	if (IsCinematic())
	{
		BeginControl(true);
	}
	if (N == 1 || N == 2)
	{
		// The gameplay cameras: on the platform facing up the walk, or out of the door facing the valley (at the arm's own
		// length: the bus isn't behind the hero there).
		bShotHold = false;
		if (N == 2)
		{
			HoldArmShort(false);
		}
		PlaceHero(N == 1 ? EEcxSpawn::ArrivePlatform : EEcxSpawn::Leave, N == 1 ? 8.0f : -3.0f);
		if (Hero)
		{
			Pc->SetViewTarget(Hero);
		}
		return;
	}
	FVector Loc;
	FVector Look;
	float Fov = 60.0f;
	if (AEmbercrestStage::ShotFor(N, Loc, Look, Fov))
	{
		bShotHold = true;
		Stage->SetCineCamera(Loc, Look, Fov);
		Pc->SetViewTarget(Stage);
		return;
	}
	// "off": the hero's own camera again.
	bShotHold = false;
	if (Hero)
	{
		Pc->SetViewTarget(Hero);
	}
}

void AEmbercrestGameMode::TestMode(const FString& Mode)
{
	bShotHold = false;
	if (Mode.Equals(TEXT("leave"), ESearchCase::IgnoreCase))
	{
		StartLeaving();
	}
	else
	{
		StartArrival(true);
	}
	BuildSlides();
}

// ------------------------------------------------------------------ the controller

AEmbercrestPlayerController::AEmbercrestPlayerController()
{
	bShowMouseCursor = false;
}

void AEmbercrestPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	AEmbercrestGameMode* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<AEmbercrestGameMode>() : nullptr;
	if (!Mode || Mode->IsUiOpen())
	{
		return;
	}
	const bool bPause = WasInputKeyJustPressed(EKeys::Escape) || WasInputKeyJustPressed(EKeys::P) || WasInputKeyJustPressed(EKeys::Gamepad_Special_Right);
	if (Mode->IsCinematic())
	{
		// Only a skip: E, Space, Enter, Escape, or the gamepad's A or B (pausing skips first, then pauses).
		if (bPause)
		{
			Mode->OnPause();
		}
		else if (WasInputKeyJustPressed(EKeys::E) || WasInputKeyJustPressed(EKeys::SpaceBar) || WasInputKeyJustPressed(EKeys::Enter) ||
				 WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Right))
		{
			Mode->OnSkip();
		}
		return;
	}
	if (Mode->IsLooking())
	{
		if (bPause || WasInputKeyJustPressed(EKeys::E) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Right))
		{
			Mode->OnInteract();
		}
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
	// The HUD's buttons follow the last device used.
	static const FKey PadButtons[] = {EKeys::Gamepad_FaceButton_Bottom, EKeys::Gamepad_FaceButton_Right, EKeys::Gamepad_FaceButton_Left, EKeys::Gamepad_FaceButton_Top,
		EKeys::Gamepad_LeftShoulder, EKeys::Gamepad_RightShoulder, EKeys::Gamepad_LeftTrigger, EKeys::Gamepad_RightTrigger, EKeys::Gamepad_DPad_Up, EKeys::Gamepad_DPad_Down,
		EKeys::Gamepad_DPad_Left, EKeys::Gamepad_DPad_Right, EKeys::Gamepad_Special_Left, EKeys::Gamepad_Special_Right, EKeys::Gamepad_LeftThumbstick, EKeys::Gamepad_RightThumbstick};
	static const FKey DeskKeys[] = {EKeys::W, EKeys::A, EKeys::S, EKeys::D, EKeys::E, EKeys::V, EKeys::P, EKeys::Escape, EKeys::LeftShift, EKeys::LeftMouseButton};
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
	if (WasInputKeyJustPressed(EKeys::MouseScrollUp))
	{
		Mode->OnZoom(-30.0f);
	}
	if (WasInputKeyJustPressed(EKeys::MouseScrollDown))
	{
		Mode->OnZoom(30.0f);
	}
	// Pause: Escape (P in Play-In-Editor, where Escape stops the session), Start or B.
	if (bPause || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Right))
	{
		Mode->OnPause();
	}
}
