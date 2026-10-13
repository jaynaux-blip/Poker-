#include "BackRoomGameMode.h"
#include "CareerSave.h"
#include "BackRoomGameShared.h"

#include "BackRoomAuto.h"
#include "BackRoomCard.h"
#include "BackRoomChips.h"
#include "BackRoomPlayer.h"
#include "BackRoomPlayerController.h"
#include "BackRoomStage.h"
#include "BackRoomTable.h"
#include "BackRoomTableHost.h"
#include "CastLooks.h"
#include "HeroGrooms.h"
#include "ShortStackCharacter.h"
#include "ShortStack/Game/Body.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "CanvasItem.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/PackageName.h"
#include "NightOneAudio.h"
#include "NightOneSaveGame.h"
#include "SFrontEndWidget.h"
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/SOverlay.h"
#include "ShortStack/AI/Profiles.h"
#include "ShortStack/Audio/Synth.h"
#include "ShortStack/Game/ChipSet.h"
#include "ShortStack/Game/Coach.h"
#include "ShortStack/Game/Hero.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Tournament.h"
#include "ShortStack/UI/FrontEnd.h"

#include <algorithm>
#include <vector>

DEFINE_LOG_CATEGORY_STATIC(LogBackRoom, Log, All);

namespace BackRoomGameDetail
{
// A minute at the table is this many on the clock (a night is an hour or so of play).
const double ClockRate = 6.0;
// Energy a live game costs per hour on the clock.
const float EnergyPerHour = 5.0f;
// Dee calls the last hand at a quarter to five; the dryers go off at five.
const double LastHandAt = 4.0 * 60.0 + 45.0;

const TCHAR* WeekdayShort[7] = {TEXT("MON"), TEXT("TUE"), TEXT("WED"), TEXT("THU"), TEXT("FRI"), TEXT("SAT"), TEXT("SUN")};
const TCHAR* WeekdayLong[7] = {TEXT("Monday"), TEXT("Tuesday"), TEXT("Wednesday"), TEXT("Thursday"), TEXT("Friday"), TEXT("Saturday"), TEXT("Sunday")};

/** "11:42 PM" for minutes after midnight. */
FString Clock12(double MinutesOfDay)
{
	const int32 M = ((static_cast<int32>(FMath::FloorToDouble(MinutesOfDay)) % 1440) + 1440) % 1440;
	const int32 H24 = M / 60;
	const int32 H12 = H24 % 12 == 0 ? 12 : H24 % 12;
	return FString::Printf(TEXT("%d:%02d %s"), H12, M % 60, H24 < 12 ? TEXT("AM") : TEXT("PM"));
}

int32 Weekday(int32 Day)
{
	return ((Day % 7) + 7) % 7;
}

/** The assembled MetaHuman for a cast member, if backroom_cast.py has built it. */
TSoftClassPtr<AActor> CastBlueprint(const TCHAR* Name)
{
	const FString Package = FString::Printf(TEXT("/Game/ShortStack/Cast/Built/MHC_%s/BP_MHC_%s"), Name, Name);
	if (!FPackageName::DoesPackageExist(Package))
	{
		return nullptr;
	}
	return TSoftClassPtr<AActor>(FSoftObjectPath(FString::Printf(TEXT("%s.BP_MHC_%s_C"), *Package, Name)));
}

FBackRoomTell Tell(EBackRoomTell T, EBackRoomTellMeaning Means, float Reliability, float FalseRate)
{
	FBackRoomTell Out;
	Out.Tell = T;
	Out.Means = Means;
	Out.Reliability = Reliability;
	Out.FalseRate = FalseRate;
	return Out;
}

// The shoulder camera (V), tunable live: how far behind the eyes, how far over the shoulder and how high over the eyes
// (cm), how much of the head's turn it swings with, its lens, and how quickly it catches up with the head. Close and
// low, as a shoulder camera is: the head a little left of center and level with it, the pot and the board just clear of
// it below and to the right, the faces across the table above.
TAutoConsoleVariable<float> CVarShoulderBack(TEXT("ss.Table.ShoulderBack"), 90.0f, TEXT("Over the shoulder at the table: the camera's distance behind the eyes (cm)."));
TAutoConsoleVariable<float> CVarShoulderSide(TEXT("ss.Table.ShoulderSide"), 42.0f, TEXT("Over the shoulder at the table: how far out over the shoulder (cm)."));
TAutoConsoleVariable<float> CVarShoulderUp(TEXT("ss.Table.ShoulderUp"), 22.0f, TEXT("Over the shoulder at the table: the camera's height over the eyes (cm)."));
TAutoConsoleVariable<float> CVarShoulderFollow(TEXT("ss.Table.ShoulderFollow"), 0.55f, TEXT("Over the shoulder at the table: how much of the head's turn the camera swings with (0..1)."));
TAutoConsoleVariable<float> CVarShoulderFov(TEXT("ss.Table.ShoulderFov"), 62.0f, TEXT("Over the shoulder at the table: the field of view (degrees)."));
TAutoConsoleVariable<float> CVarShoulderLag(TEXT("ss.Table.ShoulderLag"), 9.0f, TEXT("Over the shoulder at the table: how quickly the camera catches up with the head (per second; 0: at once)."));

// The player's choice of view at the table, kept with the engine's own user settings (it isn't the career's).
const TCHAR* TableViewSection = TEXT("ShortStack.Table");
const TCHAR* TableViewKey = TEXT("ThirdPerson");
const TCHAR* TableShoulderKey = TEXT("LeftShoulder");

bool SavedTableView(const TCHAR* Key)
{
	bool bOn = false;
	if (GConfig)
	{
		GConfig->GetBool(TableViewSection, Key, bOn, GGameUserSettingsIni);
	}
	return bOn;
}

void SaveTableView(const TCHAR* Key, bool bOn)
{
	if (GConfig)
	{
		GConfig->SetBool(TableViewSection, Key, bOn, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}

// The Back Room's inside (stage space, from BackRoomStage.cpp's shell: the walls at X -330 and 390, Y -300 and 320, the
// ceiling at 285 with its pipes), less a margin the camera keeps from them.
const FBox BackRoomInside(FVector(-330.0 + 18.0, -300.0 + 18.0, 12.0), FVector(390.0 - 18.0, 320.0 - 18.0, 285.0 - 34.0));

/** Where the segment A -> B first goes into the sphere (C, R), as a fraction along it; 1 when it doesn't (or starts in it). */
double SegmentIntoSphere(const FVector& A, const FVector& B, const FVector& C, double R)
{
	const FVector D = B - A;
	const double L2 = D.SizeSquared();
	const FVector M = A - C;
	const double Outside = M.SizeSquared() - R * R;
	const double Toward = FVector::DotProduct(M, D);
	if (L2 < KINDA_SMALL_NUMBER || Outside <= 0.0 || Toward >= 0.0)
	{
		return 1.0;
	}
	const double Disc = Toward * Toward - L2 * Outside;
	return Disc < 0.0 ? 1.0 : FMath::Clamp((-Toward - FMath::Sqrt(Disc)) / L2, 0.0, 1.0);
}

/** How far along A -> B (0..1) stays inside Box; 1 when A isn't in it (nothing to keep in). */
double SegmentInBox(const FVector& A, const FVector& B, const FBox& Box)
{
	if (!Box.IsInside(A))
	{
		return 1.0;
	}
	double T = 1.0;
	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		const double D = B[Axis] - A[Axis];
		if (D > 0.0 && B[Axis] > Box.Max[Axis])
		{
			T = FMath::Min(T, (Box.Max[Axis] - A[Axis]) / D);
		}
		else if (D < 0.0 && B[Axis] < Box.Min[Axis])
		{
			T = FMath::Min(T, (Box.Min[Axis] - A[Axis]) / D);
		}
	}
	return FMath::Clamp(T, 0.0, 1.0);
}

/**
 * The hair material's melanin, redness and grey for the creator's hair colors (black, dark brown, brown, auburn, copper,
 * blonde, platinum, grey), as ShortStackCharacter.cpp's HairShadeOf colors the street's: kept in step with it by hand.
 */
void HeroHairShade(int32 Index, FBackRoomPersona& P)
{
	static const float Table[8][3] = {{0.95f, 0.08f, 0.0f}, {0.75f, 0.2f, 0.0f}, {0.55f, 0.28f, 0.0f}, {0.45f, 0.7f, 0.0f}, {0.3f, 0.95f, 0.0f},
		{0.13f, 0.3f, 0.0f}, {0.04f, 0.05f, 0.3f}, {0.35f, 0.05f, 0.85f}};
	const int32 I = FMath::Clamp(Index, 0, 7);
	P.HairMelanin = Table[I][0];
	P.HairRedness = Table[I][1];
	P.HairWhite = Table[I][2];
}

/**
 * This frame's turn of the head (degrees of yaw and pitch) from the mouse and the right stick, at the player's look
 * sensitivity and inverted look; Scale is the view's own (Focus slows it). The stick eases in from the middle, so a face
 * can be held, and turns about 150 degrees a second across and 100 up and down at full tilt.
 */
FVector2D LookDelta(const APlayerController* PC, const ABackRoomGameMode* GM, float Scale, float RealDt)
{
	float Dx = 0.0f, Dy = 0.0f;
	PC->GetInputMouseDelta(Dx, Dy);
	FVector2D Stick(PC->GetInputAnalogKeyState(EKeys::Gamepad_RightX), PC->GetInputAnalogKeyState(EKeys::Gamepad_RightY));
	const double Tilt = FMath::Min(1.0, Stick.Size());
	Stick = Tilt > 0.0 ? Stick.GetSafeNormal() * (Tilt * Tilt) : FVector2D::ZeroVector;
	const float Sens = (GM ? GM->GetLookScale() : 1.0f) * Scale;
	const float Yaw = (Dx * 0.9f + static_cast<float>(Stick.X) * 150.0f * RealDt) * Sens;
	const float Pitch = (Dy * 0.9f + static_cast<float>(Stick.Y) * 100.0f * RealDt) * Sens;
	return FVector2D(Yaw, GM && GM->IsLookInverted() ? -Pitch : Pitch);
}

/**
 * A camera's horizontal field of view for the player's Field of View setting (vertical degrees, 50 the default):
 * DefaultHorizontal at 50, scaled as the setting scales the view (the half-angles' tangents), as the street does.
 */
float TableFov(float DefaultHorizontal, int32 VerticalSetting)
{
	const double K = FMath::Tan(FMath::DegreesToRadians(FMath::Clamp(VerticalSetting, 30, 110) * 0.5)) / FMath::Tan(FMath::DegreesToRadians(25.0));
	const double Half = FMath::Atan(FMath::Tan(FMath::DegreesToRadians(DefaultHorizontal * 0.5)) * K);
	return FMath::Clamp(static_cast<float>(FMath::RadiansToDegrees(Half * 2.0)), 20.0f, 125.0f);
}

/** "2560 x 1440" to (2560, 1440); (0, 0) when empty or malformed (as the apartment reads it). */
FIntPoint TableParseResolution(const std::string& Text)
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
std::vector<std::string> TableResolutions()
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

void SetTableCvar(const TCHAR* Name, int32 Value)
{
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name))
	{
		Var->Set(Value, ECVF_SetByGameSetting);
	}
}

/** "$1,234.50" for cents, as the bank writes it. */
FString CentsText(int64 Cents)
{
	const int64 Abs = Cents < 0 ? -Cents : Cents;
	return FString::Printf(TEXT("%s$%s.%02lld"), Cents < 0 ? TEXT("-") : TEXT(""), *FText::AsNumber(Abs / 100).ToString(), Abs % 100);
}

/** "1st", "22nd", "113th". */
FString OrdinalText(int32 N)
{
	const int32 Tens = N % 100;
	const TCHAR* Suffix = (Tens >= 11 && Tens <= 13) ? TEXT("th") : (N % 10 == 1 ? TEXT("st") : (N % 10 == 2 ? TEXT("nd") : (N % 10 == 3 ? TEXT("rd") : TEXT("th"))));
	return FString::Printf(TEXT("%d%s"), N, Suffix);
}

std::string ToUtf8(const FString& S)
{
	return std::string(TCHAR_TO_UTF8(*S));
}

/**
 * The mix on an audio component: the room's voices (the card room's speech and the PA, next_segment_spec 4.9) as their own
 * level where the audio has a Voices bus (ue-room's UNightOneAudio::SetMix with four levels), and today's three where it
 * hasn't yet.
 */
template <typename TAudio>
void ApplyMix(TAudio& Audio, float Master, float Effects, float Ambience, float Voices)
{
	if constexpr (requires { Audio.SetMix(Master, Effects, Ambience, Voices); })
	{
		Audio.SetMix(Master, Effects, Ambience, Voices);
	}
	else
	{
		(void)Voices;
		Audio.SetMix(Master, Effects, Ambience);
	}
}
// The Voices bus is there now (ue-room's): keep the four-level SetMix, or the Voices slider would quietly stop working.
static_assert(requires(UNightOneAudio& A, float F) { A.SetMix(F, F, F, F); }, "UNightOneAudio::SetMix(Master, Effects, Ambience, Voices) is what the menu's Voices slider drives");

/**
 * The table's buttons that act on how long they're held (next_segment_spec 6.4): Y on a pad raises when let go under
 * 0.35 s, fills ALL IN past it and goes all in at 0.95 s (let go between: nothing); A on the keyboard is all in held for
 * 0.6 s. And the table card dismissed while its button is still down (Esc or B): it stays down until let go. The pawn's
 * own (one player), kept off the frozen header.
 */
struct FPawnTableInput
{
	ss::ui::HoldButton PadY;
	ss::ui::HoldButton KeyA;
	bool bPadYDown = false;
	bool bKeyADown = false;
	bool bCardDismissed = false;
	/** When the held button went down (the pawn's own clock, HoldNow), or -1. */
	double DownAt = -1.0;
	/** The engine frame the seated input last ran (GFrameCounter): after a gap (paused, walking, on foot, quitting),
	 *  whatever is already down is an old press, never the start of a hold. */
	uint64 LastFrame = 0;
};

TMap<const ABackRoomPawn*, FPawnTableInput>& TableInputs()
{
	static TMap<const ABackRoomPawn*, FPawnTableInput> Inputs;
	return Inputs;
}

FPawnTableInput& InputFor(const ABackRoomPawn* Pawn)
{
	FPawnTableInput& In = TableInputs().FindOrAdd(Pawn);
	In.PadY.CanTap = true;
	In.KeyA.CanTap = false;
	return In;
}

/** Real seconds the table's actions stay locked after the table card closes. */
constexpr double CardLockSeconds = 0.25;

} // namespace BackRoomGameDetail

using namespace BackRoomGameDetail;

// ------------------------------------------------------------------ pawn

ABackRoomPawn::ABackRoomPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	RootComponent = Camera;
	Camera->SetFieldOfView(72.0f);
	Camera->bUsePawnControlRotation = false;
	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	PeekLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PeekLight"));
	PeekLight->SetupAttachment(Camera);
	PeekLight->SetUsingAbsoluteLocation(true);
	PeekLight->SetIntensityUnits(ELightUnits::Candelas);
	PeekLight->SetIntensity(0.0f);
	PeekLight->SetLightColor(FLinearColor(1.0f, 0.86f, 0.68f));
	PeekLight->SetAttenuationRadius(35.0f);
	PeekLight->SetSourceRadius(4.0f);
	PeekLight->SetCastShadows(false);
	PeekLight->SetVolumetricScatteringIntensity(0.0f);
}

void ABackRoomPawn::BeginPlay()
{
	Super::BeginPlay();
	Seat = GetActorLocation();
	FirstEye = Seat;
	// The view the player kept: over the shoulder (and which one). A night's walk in is first person, and the camera eases
	// out once they sit (in practice, once the head it looks past is made).
	bThirdPerson = SavedTableView(TableViewKey);
	ThirdBlend = 0.0f;
	ShoulderSideWant = SavedTableView(TableShoulderKey) ? -1.0f : 1.0f;
	ShoulderSide = ShoulderSideWant;
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}
	// A close, shallow lens: what you look at is sharp, the rest of the room falls away a little.
	FPostProcessSettings& P = Camera->PostProcessSettings;
	P.bOverride_DepthOfFieldFstop = true;
	P.bOverride_DepthOfFieldFocalDistance = true;
	P.bOverride_DepthOfFieldSensorWidth = true;
	P.DepthOfFieldSensorWidth = 24.576f;
	P.bOverride_VignetteIntensity = true;
	// The heart's hold on the eyes: color drains and the edges smear as it races.
	P.bOverride_ColorSaturation = true;
	P.bOverride_SceneFringeIntensity = true;
}

void ABackRoomPawn::EndPlay(const EEndPlayReason::Type Reason)
{
	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
	TableInputs().Remove(this);
	Super::EndPlay(Reason);
}

ABackRoomGameMode* ABackRoomPawn::GetMode() const
{
	return GetWorld() ? GetWorld()->GetAuthGameMode<ABackRoomGameMode>() : nullptr;
}

ABackRoomTable* ABackRoomPawn::GetTable() const
{
	const ABackRoomGameMode* GM = GetMode();
	return GM ? GM->Table.Get() : nullptr;
}

void ABackRoomPawn::TestExtCam(FVector Pos, FVector At, float Fov, bool bOn)
{
	bExtCam = bOn;
	ExtPos = Pos;
	ExtAt = At;
	ExtFov = Fov;
}

FVector ABackRoomPawn::GetEye() const
{
	// (The camera is the root: over the shoulder it isn't where the eyes are, and a walk must set off from the body.)
	return FirstEye.IsZero() ? GetActorLocation() : FirstEye;
}

void ABackRoomPawn::ToggleThirdPerson()
{
	bThirdPerson = !bThirdPerson;
	ViewShownAt = FPlatformTime::Seconds();
	SaveTableView(TableViewKey, bThirdPerson);
}

void ABackRoomPawn::HandleInput(float RealDt)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	const ABackRoomPlayerController* Pad = Cast<ABackRoomPlayerController>(PC);
	ABackRoomTable* Table = GetTable();
	ABackRoomGameMode* GM = GetMode();
	ABackRoomPlayer* Me = Table ? Table->GetHeroPlayer() : nullptr;
	if (!PC)
	{
		return;
	}
	// V (the gamepad's View): over the shoulder, or back in your own head. B, the middle button or the right stick pressed
	// in: the other shoulder.
	if (Pressed(PC, EKeys::V) || Pressed(PC, EKeys::Gamepad_Special_Left))
	{
		ToggleThirdPerson();
	}
	if (bThirdPerson && (Pressed(PC, EKeys::B) || Pressed(PC, EKeys::MiddleMouseButton) || Pressed(PC, EKeys::Gamepad_RightThumbstick)))
	{
		// Away from the shoulder the camera is on now (even one it moved to by itself while they look across), and kept
		// there while they go on looking that way.
		const float SideNow = bShoulderFlipped ? -ShoulderSideWant : ShoulderSideWant;
		ShoulderSideWant = -SideNow;
		bShoulderFlipped = false;
		bShoulderHeld = static_cast<float>(Smoothed.Yaw) * ShoulderSideWant < -18.0f;
		SaveTableView(TableShoulderKey, ShoulderSideWant < 0.0f);
	}
	// Peek: hold to lift the corners (Space, the left button or the left trigger).
	const bool bPeek = bTestPeek || Held(PC, EKeys::SpaceBar) || Held(PC, EKeys::LeftMouseButton) || Held(PC, EKeys::Gamepad_LeftTrigger);
	PeekBlend = Ease(PeekBlend, bPeek ? 1.0f : 0.0f, 6.0f, RealDt);
	if (Me)
	{
		Me->SetHeroPeek(bPeek);
	}
	// The hand is known once the corners are really up (the fingers have taken them), not when the key goes down.
	if (bPeek && Me && Me->GetPeekAmount() > 0.55f && Table)
	{
		Table->HeroPeeked();
	}

	// Steady breathing (Shift, or the left stick held in): the heart settles. It draws on the same reserve as Focus.
	bSteadying = (Held(PC, EKeys::LeftShift) || Held(PC, EKeys::RightShift) || Held(PC, EKeys::Gamepad_LeftThumbstick)) && FocusLeft > 0.02f;
	if (Table)
	{
		Table->SetSteadying(bSteadying);
	}

	// Focus: hold to study a face (the right button, or the right trigger); it drains, and comes back slowly. Fitness
	// (body::TableOf, on the table) sets how fast it comes back, and Box Breathing (three sessions with Rosa) what a slow
	// breath costs of it.
	const bool bFocusing = (bTestFocus || Held(PC, EKeys::RightMouseButton) || Held(PC, EKeys::Gamepad_RightTrigger)) && FocusLeft > 0.02f && !bPeek;
	const float BreathCost = Table ? static_cast<float>(FMath::Clamp(Table->Fitness.FocusCostScale, 0.25, 2.0)) : 1.0f;
	const float Refill = Table ? static_cast<float>(FMath::Clamp(Table->Fitness.FocusRefill, 1.0 / 60.0, 1.0 / 4.0)) : 1.0f / 14.0f;
	const float Drain = (bFocusing ? 1.0f / 8.0f : 0.0f) + (bSteadying ? BreathCost / 12.0f : 0.0f);
	FocusLeft = FMath::Clamp(FocusLeft + (Drain > 0.0f ? -Drain * RealDt : RealDt * Refill), 0.0f, 1.0f);
	Focus = Ease(Focus, bFocusing ? 1.0f : 0.0f, 5.0f, RealDt);
	const bool bFolded = GM && GM->IsLive() && GM->GetPhase() == EBackRoomPhase::Playing && Table && !Table->IsHeroInHand();
	// The table card (next_segment_spec 6.3): held up with Tab or up on the d-pad, modal (the decision waits). Q and E or
	// the bumpers turn its pages, U (or Y while it's up) swaps big blinds and chips, Esc or B puts it down until the button
	// is let go. The table's actions stay locked a moment after it goes.
	FPawnTableInput& In = InputFor(this);
	const double RealNow = FPlatformTime::Seconds();
	const bool bCardKey = Held(PC, EKeys::Tab) || Held(PC, EKeys::Gamepad_DPad_Up);
	In.bCardDismissed = In.bCardDismissed && bCardKey;
	const bool bCardWas = bTableCard;
	bTableCard = bCardKey && !In.bCardDismissed;
	if (bTableCard)
	{
		const int32 Pages = static_cast<int32>(ss::ui::TableCardPages().size());
		if (Pressed(PC, EKeys::Q) || Pressed(PC, EKeys::Gamepad_LeftShoulder))
		{
			TableCardPage = (TableCardPage + Pages - 1) % Pages;
		}
		if (Pressed(PC, EKeys::E) || Pressed(PC, EKeys::Gamepad_RightShoulder))
		{
			TableCardPage = (TableCardPage + 1) % Pages;
		}
		if (GM && (Pressed(PC, EKeys::U) || Pressed(PC, EKeys::Gamepad_FaceButton_Top)))
		{
			GM->FlipStackUnits();
		}
		if (Pressed(PC, EKeys::Escape) || Pressed(PC, EKeys::Gamepad_FaceButton_Right))
		{
			In.bCardDismissed = true;
			bTableCard = false;
		}
	}
	if (bCardWas && !bTableCard)
	{
		TableCardClosedAt = RealNow;
	}
	// Up, or just put down: nothing at the table is yours to do (a held button starts over).
	const bool bCardLock = bTableCard || RealNow - TableCardClosedAt < CardLockSeconds;
	// Update 2 (§2.6.2): the plan's own keys (ue-auto's FAutoHost::PawnKeys): K, T while it holds the seat, the PLAY page,
	// and the first action press during its think, which only takes the wheel. While they're the plan's, the table's pace
	// and the actions below don't run on them.
	FAutoHost* Auto = FAutoHost::Of(GM);
	const bool bAutoKeys = Auto && Auto->PawnKeys(*this, PC, bCardLock);
	if (bCardLock || bAutoKeys)
	{
		In.PadY.Reset();
		In.KeyA.Reset();
	}
	// T (right on the d-pad): the table's pace. N, out of the hand (A on a gamepad: nothing else is yours to do then): the
	// rest of it at a glance, until you're dealt in again. Neither ever decides anything of yours; autoplay is separate.
	if (!bCardLock && !bAutoKeys && GM && GM->IsLive() && (Pressed(PC, EKeys::T) || Pressed(PC, EKeys::Gamepad_DPad_Right)))
	{
		GM->CycleTablePace();
	}
	if (!bCardLock && bFolded && !bFocusing && (Pressed(PC, EKeys::N) || Pressed(PC, EKeys::Gamepad_FaceButton_Bottom)))
	{
		bSkipHand = true;
	}
	// Q (left on the d-pad): up from the table, on foot (you're still dealt in).
	if (!bCardLock && GM && GM->IsLive() && (Pressed(PC, EKeys::Q) || Pressed(PC, EKeys::Gamepad_DPad_Left)))
	{
		GM->LiveStandUp();
		return;
	}
	if (bSkipHand && (!bFolded || bFocusing))
	{
		bSkipHand = false;
		Pace = 1.0f;
	}
	const float Out = GM ? GM->FoldedPace() : 1.9f;
	// (Update 2, §2.12: while the plan holds the seat its watch speed is the target for the whole hand, never slower than a
	// folded hand's; 1 when it doesn't.)
	const float Want = FMath::Max(bFolded && !bFocusing ? Out : 1.0f, GM ? GM->AutoWatchSpeed() : 1.0f);
	Pace = bSkipHand ? 6.0f : Ease(Pace, Want, 2.0f, RealDt);
	UGameplayStatics::SetGlobalTimeDilation(this, FMath::Lerp(Pace, 0.45f, Focus));
	if (bFocusing && Table)
	{
		// Whoever is nearest the middle of the view, and stays that way until you look well away.
		const FVector Eye = Camera->GetComponentLocation();
		const FVector Fwd = Camera->GetForwardVector();
		ABackRoomPlayer* Best = nullptr;
		float BestCos = 0.94f;
		for (ABackRoomPlayer* O : Table->GetOpponents())
		{
			const float C = FVector::DotProduct(Fwd, (O->GetEyes() - Eye).GetSafeNormal());
			const bool bCurrent = Studying.Get() == O;
			if (C > (bCurrent ? 0.85f : BestCos) && (C > BestCos || bCurrent))
			{
				Best = O;
				BestCos = C;
			}
		}
		Studying = Best;
	}
	else if (Focus < 0.05f)
	{
		Studying = nullptr;
	}

	// Sizing (next_segment_spec 6.2 D, 6.4): the fine steps on the wheel or the arrows, and on a bumper held (it keeps
	// stepping); a bumper tapped goes to the next preset; keys 1 to 6 pick one.
	const int32 Fine = (Pressed(PC, EKeys::MouseScrollUp) || Pressed(PC, EKeys::Up) ? 1 : 0) - (Pressed(PC, EKeys::MouseScrollDown) || Pressed(PC, EKeys::Down) ? 1 : 0) +
		(Pad ? Pad->RepeatSteps(EKeys::Gamepad_RightShoulder) - Pad->RepeatSteps(EKeys::Gamepad_LeftShoulder) : 0);
	const int32 Snap = (Pressed(PC, EKeys::Gamepad_RightShoulder) ? 1 : 0) - (Pressed(PC, EKeys::Gamepad_LeftShoulder) ? 1 : 0);
	// L, or down on the d-pad held: going (a held button, so a stray press can't).
	const bool bLeave = !bCardLock && (Pressed(PC, EKeys::L) || (Pad && Pad->HoldCompleted(EKeys::Gamepad_DPad_Down)));
	const bool bYDown = Held(PC, EKeys::Gamepad_FaceButton_Top);
	const bool bADown = Held(PC, EKeys::A);
	// The hold clock: the pawn's own real time, which stands still while the game is paused and moves at most 0.1 s a
	// frame, so neither a pause nor a hitch can turn a tap of Y into a hold (HoldButton reads the time between the press and
	// the release). A hold starts only on a press seen here, at the table: after any frame this didn't run (paused, a walk,
	// on foot, a quit), a button already down is an old one (A held to walk to the chair never arms the keyboard's all in).
	const double HoldNow = static_cast<double>(Time);
	if (In.LastFrame + 1 != GFrameCounter)
	{
		In.PadY.Reset();
		In.KeyA.Reset();
		In.bPadYDown = bYDown;
		In.bKeyADown = bADown;
		In.DownAt = -1.0;
	}
	In.LastFrame = GFrameCounter;

	// Felted: rebuy from the bankroll (R or Y; the sizing picks how much) or go home.
	if (GM && GM->GetPhase() == EBackRoomPhase::Busted)
	{
		In.PadY.Reset();
		In.KeyA.Reset();
		In.bPadYDown = bYDown;
		In.bKeyADown = bADown;
		RaiseHeld = -1.0f;
		AllInHold = 0.0f;
		if (bCardLock)
		{
			return;
		}
		if (Pressed(PC, EKeys::R) || Pressed(PC, EKeys::Gamepad_FaceButton_Top))
		{
			GM->Reload();
		}
		if (Fine + Snap != 0)
		{
			GM->AdjustReload(Fine + Snap);
		}
		if (bLeave)
		{
			GM->RequestLeave();
		}
		return;
	}

	// Actions, one control map (ss::ui::Controls; next_segment_spec 6.4): F or X folds; C or A checks or calls (A only
	// while you're in the hand: folded, it's the next hand); R bets or raises to the amount shown, and so does Y let go
	// within 0.35 s; held past that, Y fills ALL IN and goes all in at 0.95 s (let go between: nothing, and a hold never
	// raises first). On the keyboard all in is A held. B backs out (the table card) and does nothing to the hand.
	if (Table && !bCardLock && !bAutoKeys)
	{
		if (Pressed(PC, EKeys::F) || Pressed(PC, EKeys::Gamepad_FaceButton_Left))
		{
			Table->HeroFold();
		}
		if (Pressed(PC, EKeys::C) || (!bFolded && Pressed(PC, EKeys::Gamepad_FaceButton_Bottom)))
		{
			Table->HeroCheckCall();
		}
		if (Pressed(PC, EKeys::R))
		{
			Table->HeroRaise();
		}
		using EOut = ss::ui::HoldButton::Out;
		// Y: a tap raises on release, a hold goes all in.
		if (bYDown && !In.bPadYDown)
		{
			In.PadY.Press(HoldNow);
		}
		if (bYDown)
		{
			if (In.PadY.Tick(HoldNow) == EOut::Hold)
			{
				UE_LOG(LogBackRoom, Log, TEXT("Table input: Y held %.2f s: all in"), HoldNow - In.DownAt);
				Table->HeroAllIn();
			}
		}
		else if (In.bPadYDown && In.PadY.IsDown())
		{
			const EOut Let = In.PadY.Release(HoldNow);
			UE_LOG(LogBackRoom, Log, TEXT("Table input: Y let go after %.2f s: %s"), HoldNow - In.DownAt, Let == EOut::Tap ? TEXT("raise") : Let == EOut::Hold ? TEXT("all in") : TEXT("nothing"));
			if (Let == EOut::Tap)
			{
				Table->HeroRaise();
			}
			else if (Let == EOut::Hold)
			{
				Table->HeroAllIn();
			}
		}
		// A on the keyboard: all in, held.
		if (bADown && !In.bKeyADown)
		{
			In.KeyA.Press(HoldNow);
		}
		if (bADown)
		{
			if (In.KeyA.Tick(HoldNow) == EOut::Hold)
			{
				UE_LOG(LogBackRoom, Log, TEXT("Table input: A held %.2f s: all in"), HoldNow - In.DownAt);
				Table->HeroAllIn();
			}
		}
		else if (In.bKeyADown && In.KeyA.Release(HoldNow) == EOut::Hold)
		{
			Table->HeroAllIn();
		}
		// The presets (keys 1 to 6, the bumpers tapped) and the fine steps.
		static const FKey PresetKeys[6] = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six};
		for (int32 K = 0; K < 6; ++K)
		{
			if (Pressed(PC, PresetKeys[K]))
			{
				Table->HeroPreset(K);
			}
		}
		if (Snap != 0)
		{
			Table->HeroSnap(Snap);
		}
		if (Fine != 0)
		{
			Table->HeroAdjustRaise(Fine);
		}
		if (GM && Pressed(PC, EKeys::U))
		{
			GM->FlipStackUnits();
		}
	}
	// The ladder's fill, and how long the button's been down (for the HUD).
	if ((bYDown && !In.bPadYDown) || (bADown && !In.bKeyADown))
	{
		In.DownAt = HoldNow;
	}
	In.bPadYDown = bYDown;
	In.bKeyADown = bADown;
	const bool bHolding = In.PadY.IsDown() || In.KeyA.IsDown();
	AllInHold = FMath::Max(In.PadY.Progress(HoldNow), In.KeyA.Progress(HoldNow));
	RaiseHeld = bHolding && In.DownAt >= 0.0 ? static_cast<float>(HoldNow - In.DownAt) : -1.0f;
	if (GM && bLeave)
	{
		GM->RequestLeave();
	}
}

bool ABackRoomPawn::Held(const APlayerController* PC, const FKey& Key) const
{
	return (PC && PC->IsInputKeyDown(Key)) || TestHeld.Contains(Key.GetFName());
}

bool ABackRoomPawn::Pressed(const APlayerController* PC, const FKey& Key) const
{
	return (PC && PC->WasInputKeyJustPressed(Key)) || TestPressed.Contains(Key.GetFName());
}

void ABackRoomPawn::TestHold(const FString& Key, bool bDown)
{
	if (bDown)
	{
		TestHeld.Add(FName(*Key));
	}
	else
	{
		TestHeld.Remove(FName(*Key));
	}
}

void ABackRoomPawn::TestPress(const FString& Key)
{
	TestPressed.Add(FName(*Key));
}

void ABackRoomPawn::TestWalker(float RoomX, float RoomY, float InYaw, float InPitch)
{
	ABackRoomGameMode* GM = GetMode();
	if (!GM || !GM->Stage)
	{
		return;
	}
	// Seated at a live table, or still walking in to the seat: up through the stand-up path first, as Q does (the away rule
	// has the chair: checked when it's free, mucked when it isn't), and then on foot here at once. A walker put down past it
	// left the table waiting on the player's decision with nobody in the chair (acceptance F29).
	if (GM->IsLive() && !GM->IsHeroUp() && !bFreeWalk)
	{
		if (bWalking && !bWalkOut)
		{
			// Into the chair now: the walk in's own end (the night starts there).
			bWalking = false;
			TFunction<void()> Done = MoveTemp(WalkDone);
			WalkDone = nullptr;
			if (Done)
			{
				Done();
			}
		}
		GM->LiveStandUp();
		if (GM->IsHeroUp())
		{
			// ...and straight out of it, without the walk out: the body leaves the chair with the walker.
			bWalking = false;
			bWalkPending = false;
			bHoldWalkEnd = false;
			WalkDone = nullptr;
			if (ABackRoomTable* T = GetTable())
			{
				if (ABackRoomPlayer* Me = T->GetHeroPlayer())
				{
					Me->SetActorHiddenInGame(true);
				}
			}
		}
	}
	if (!bFreeWalk)
	{
		BeginFreeWalk();
	}
	const FVector At = GM->Stage->RoomToWorld(FVector(RoomX, RoomY, 0.0));
	WalkBase = FVector(At.X, At.Y, GM->Stage->CardRoomFloorZ(At) + 166.0);
	WalkVel = FVector::ZeroVector;
	WalkYaw = InYaw;
	WalkPitch = InPitch;
	Smoothed = FRotator(InPitch, InYaw, 0.0f);
	FirstEye = WalkBase;
	SetActorLocationAndRotation(WalkBase, Smoothed);
}

void ABackRoomPawn::BeginFreeWalk()
{
	bFreeWalk = true;
	bHoldWalkEnd = false;
	bWalkPending = false;
	// On foot is first person (the body walking is the camera's own).
	ThirdBlend = 0.0f;
	if (ABackRoomTable* T = GetTable())
	{
		if (ABackRoomPlayer* Me = T->GetHeroPlayer())
		{
			Me->ShowHeroHead(false);
		}
	}
	WalkBase = GetEye();
	FirstEye = WalkBase;
	WalkVel = FVector::ZeroVector;
	WalkYaw = Smoothed.Yaw;
	WalkPitch = FMath::Clamp(Smoothed.Pitch, -40.0f, 20.0f);
	Focus = 0.0f;
	Studying = nullptr;
	PeekBlend = 0.0f;
	bSteadying = false;
	bSkipHand = false;
	Pace = 1.0f;
	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
}

void ABackRoomPawn::TickFreeWalk(float RealDt)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	ABackRoomGameMode* GM = GetMode();
	ABackRoomStage* Stage = GM ? GM->Stage.Get() : nullptr;
	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
	// The stage rail's view of the feature table (spec 6.5): a fixed camera, the ON AIR lower third; E, A or B steps back.
	// (The press that steps back is spent on it: it doesn't also use the rail again, which would put the view straight
	// back up, or pause.)
	bool bSteppedBack = false;
	if (GM && GM->IsWatchingFeature())
	{
		if (Pressed(PC, EKeys::E) || Pressed(PC, EKeys::Gamepad_FaceButton_Bottom) || Pressed(PC, EKeys::Gamepad_FaceButton_Right) || Pressed(PC, EKeys::BackSpace))
		{
			GM->WatchFeature(false);
			bSteppedBack = true;
		}
		else
		{
			Camera->SetWorldLocationAndRotation(ExtPos, (ExtAt - ExtPos).Rotation());
			Camera->SetFieldOfView(ExtFov);
			Camera->PostProcessSettings.DepthOfFieldFocalDistance = static_cast<float>(FVector::Dist(ExtPos, ExtAt));
			Camera->PostProcessSettings.DepthOfFieldFstop = 8.0f;
			float PitchKick = 0.0f, Racing = 0.0f;
			TickHeart(RealDt, PitchKick, Racing);
			return;
		}
	}
	// The table card on foot too (held up: Tab or up on the d-pad), its pages and units; Esc or B puts it down.
	{
		FPawnTableInput& In = InputFor(this);
		const bool bCardKey = Held(PC, EKeys::Tab) || Held(PC, EKeys::Gamepad_DPad_Up);
		In.bCardDismissed = In.bCardDismissed && bCardKey;
		const bool bWas = bTableCard;
		// (Not over the desk's screen: up on the d-pad is its own there.)
		bTableCard = bCardKey && !In.bCardDismissed && GM && GM->IsLive() && !GM->IsDeskOpen();
		if (bTableCard)
		{
			const int32 Pages = static_cast<int32>(ss::ui::TableCardPages().size());
			if (Pressed(PC, EKeys::Q) || Pressed(PC, EKeys::Gamepad_LeftShoulder))
			{
				TableCardPage = (TableCardPage + Pages - 1) % Pages;
			}
			if (Pressed(PC, EKeys::E) || Pressed(PC, EKeys::Gamepad_RightShoulder))
			{
				TableCardPage = (TableCardPage + 1) % Pages;
			}
			if (Pressed(PC, EKeys::U) || Pressed(PC, EKeys::Gamepad_FaceButton_Top))
			{
				GM->FlipStackUnits();
			}
			if (Pressed(PC, EKeys::Escape) || Pressed(PC, EKeys::Gamepad_FaceButton_Right))
			{
				In.bCardDismissed = true;
				bTableCard = false;
			}
		}
		if (bWas && !bTableCard)
		{
			TableCardClosedAt = FPlatformTime::Seconds();
		}
	}
	// (The desk's screen has the keys while it's up: ue-venue's.)
	const bool bCardLock = bSteppedBack || bTableCard || FPlatformTime::Seconds() - TableCardClosedAt < CardLockSeconds || (GM && GM->IsDeskOpen());
	if (PC)
	{
		// The mouse or the right stick.
		const FVector2D Turn = LookDelta(PC, GM, 1.0f, RealDt);
		WalkYaw += static_cast<float>(Turn.X);
		WalkPitch = FMath::Clamp(WalkPitch + static_cast<float>(Turn.Y), -55.0f, 40.0f);
	}
	// Where the keys (or the left stick, as far as it's pushed) say, at a walk (Shift, the left stick held in or the left
	// bumper: in a hurry); the room stops you at its tables, counters and walls.
	double Fwd = 0.0, Right = 0.0;
	Fwd += Held(PC, EKeys::W) || Held(PC, EKeys::Up) ? 1.0 : 0.0;
	Fwd -= Held(PC, EKeys::S) || Held(PC, EKeys::Down) ? 1.0 : 0.0;
	Right += Held(PC, EKeys::D) || Held(PC, EKeys::Right) ? 1.0 : 0.0;
	Right -= Held(PC, EKeys::A) || Held(PC, EKeys::Left) ? 1.0 : 0.0;
	if (PC)
	{
		Fwd = FMath::Clamp(Fwd + PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftY), -1.0, 1.0);
		Right = FMath::Clamp(Right + PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX), -1.0, 1.0);
	}
	if (GM && GM->IsDeskOpen())
	{
		Fwd = 0.0;
		Right = 0.0;
	}
	const bool bHurry = Held(PC, EKeys::LeftShift) || Held(PC, EKeys::RightShift) || Held(PC, EKeys::Gamepad_LeftThumbstick) || Held(PC, EKeys::Gamepad_LeftShoulder);
	const FVector Ahead = FRotator(0.0f, WalkYaw, 0.0f).Vector();
	const FVector Side = FRotator(0.0f, WalkYaw + 90.0f, 0.0f).Vector();
	const FVector Wish = (Ahead * Fwd + Side * Right).GetClampedToMaxSize(1.0) * (bHurry ? 240.0 : 145.0);
	WalkVel = FMath::VInterpTo(WalkVel, Wish, RealDt, 9.0f);
	FVector Next = WalkBase + WalkVel * RealDt;
	if (Stage)
	{
		Next = Stage->CardRoomStep(WalkBase, Next);
		Next.Z = FMath::FInterpTo(WalkBase.Z, Stage->CardRoomFloorZ(Next) + 166.0, RealDt, 8.0f);
	}
	const double Moved = FVector::Dist2D(WalkBase, Next);
	if (RealDt > 0.0f)
	{
		// Stopped by something: the stride stops with you.
		WalkVel = FVector(Next.X - WalkBase.X, Next.Y - WalkBase.Y, 0.0) / RealDt;
	}
	WalkBase = Next;
	WalkDist += static_cast<float>(Moved);
	const float Moving = FMath::Clamp(static_cast<float>(WalkVel.Size2D() / 145.0), 0.0f, 1.0f);
	// A footfall each stride on the carpet, and the head's small bob and sway.
	const int32 Step = FMath::FloorToInt(WalkDist / 66.0f);
	if (Step != StepCount)
	{
		StepCount = Step;
		ABackRoomTable* Table = GetTable();
		if (UNightOneAudio* Sound = Table ? Table->GetAudio() : nullptr; Sound && Moving > 0.3f)
		{
			Sound->PlayEffect(ss::audio::Effect::Step, 0.2f + 0.3f * Moving);
		}
	}
	const float Steps = WalkDist / 66.0f * PI;
	FVector Eye = WalkBase;
	Eye.Z += Moving * (1.4f * FMath::Abs(FMath::Sin(Steps)) - 0.7f);
	const FRotator Look(WalkPitch, WalkYaw, Moving * 0.45f * FMath::Sin(Steps * 0.5f));
	Smoothed = FMath::RInterpTo(Smoothed, Look, RealDt, 16.0f);
	FirstEye = Eye;
	SetActorLocationAndRotation(Eye, Smoothed);
	Camera->SetFieldOfView(GM ? GM->SettingFov(74.0f) : 74.0f);
	FPostProcessSettings& P = Camera->PostProcessSettings;
	P.DepthOfFieldFocalDistance = 320.0f;
	P.DepthOfFieldFstop = 5.6f;
	P.SceneFringeIntensity = 0.0f;
	PeekLight->SetIntensity(0.0f);
	// The street's controls on foot (spec 6.4): E (A on a gamepad) uses whatever's in reach (your chair, the desk, the cage,
	// the bar, the deck, the door); V (Y, or View) picks the view for the table (on foot it's always your own eyes); B
	// pauses, as it does outside.
	if (!bCardLock && GM && (Pressed(PC, EKeys::E) || Pressed(PC, EKeys::Gamepad_FaceButton_Bottom)))
	{
		GM->LiveInteract();
	}
	if (!bCardLock && (Pressed(PC, EKeys::V) || Pressed(PC, EKeys::Gamepad_Special_Left) || Pressed(PC, EKeys::Gamepad_FaceButton_Top)))
	{
		ToggleThirdPerson();
	}
	if (!bCardLock && GM && Pressed(PC, EKeys::Gamepad_FaceButton_Right) && !GM->IsDeskOpen())
	{
		GM->OpenPauseMenu();
	}
	float PitchKick = 0.0f, Racing = 0.0f;
	TickHeart(RealDt, PitchKick, Racing);
}

void ABackRoomPawn::PlayWalk(const TArray<FVector>& Points, float Seconds, bool bOut, TFunction<void()> OnDone)
{
	WalkPoints = Points;
	WalkLengths.Reset();
	WalkTotal = 0.0f;
	for (int32 I = 1; I < WalkPoints.Num(); ++I)
	{
		const float L = static_cast<float>(FVector::Dist(WalkPoints[I - 1], WalkPoints[I]));
		WalkLengths.Add(L);
		WalkTotal += L;
	}
	WalkT = 0.0f;
	StepCount = 0;
	WalkSeconds = FMath::Max(Seconds, 0.1f);
	bWalkOut = bOut;
	ABackRoomTable* Table = GetTable();
	ABackRoomPlayer* Me = Table ? Table->GetHeroPlayer() : nullptr;
	// Going out, the body is left in the chair as the walk gets up (unless an earlier walk already took it: from the
	// cashier's window there's no chair to scrape).
	bBodyShown = bOut && Me && !Me->IsHidden();
	WalkDone = MoveTemp(OnDone);
	bHoldWalkEnd = false;
	bWalkPending = false;
	const bool bCanWalk = WalkPoints.Num() >= 2 && WalkTotal > 1.0f;
	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
	Focus = 0.0f;
	Studying = nullptr;
	// Nobody is studied from a walk, or from where one stops (the seated view sets it again on sitting down).
	if (Table)
	{
		for (ABackRoomPlayer* O : Table->GetOpponents())
		{
			O->SetStudied(0.0f);
		}
	}
	if (bCanWalk && bOut && ThirdBlend > 0.02f && !bFreeWalk)
	{
		// Over the shoulder: the camera comes back into the head first (Tick eases it), then the walk sets off from the eyes.
		bWalking = false;
		bWalkPending = true;
		return;
	}
	bWalking = bCanWalk;
	if (bWalking)
	{
		StartPendingWalk();
		if (!bOut)
		{
			SetActorLocation(WalkPoints[0]);
			FirstEye = WalkPoints[0];
			Smoothed = (WalkPoints[1] - WalkPoints[0]).Rotation();
			Smoothed.Pitch = -4.0f;
			// Back to the seat over the shoulder: the head is ready before the body is seen again (the actor is hidden
			// while it's made, as anyone's is between tables), and the camera eases out once they sit.
			if (Me && bThirdPerson)
			{
				Me->SetHeroHeadBuilt(true);
			}
		}
	}
	else if (WalkDone)
	{
		TFunction<void()> Done = MoveTemp(WalkDone);
		WalkDone = nullptr;
		Done();
	}
}

void ABackRoomPawn::StartPendingWalk()
{
	bWalkPending = false;
	bWalking = true;
	// First person from its first step: the camera in the head, the face (and over-the-shoulder hair) out of the view.
	ThirdBlend = 0.0f;
	bShoulderFresh = true;
	if (ABackRoomTable* Table = GetTable())
	{
		if (ABackRoomPlayer* Me = Table->GetHeroPlayer())
		{
			Me->ShowHeroHead(false);
		}
	}
	if (bWalkOut)
	{
		// From where the eyes are now: the path was asked for from them a moment ago (over the shoulder, before the camera
		// came back in; maybe mid-peek, down at the cards), and the head has moved since.
		const FVector Now = GetEye();
		if (WalkPoints.Num() >= 2 && WalkLengths.Num() >= 1 && FVector::DistSquared(Now, WalkPoints[0]) < FMath::Square(60.0))
		{
			WalkPoints[0] = Now;
			WalkTotal -= WalkLengths[0];
			WalkLengths[0] = static_cast<float>(FVector::Dist(WalkPoints[0], WalkPoints[1]));
			WalkTotal = FMath::Max(1.0f, WalkTotal + WalkLengths[0]);
		}
		SetActorLocationAndRotation(Now, Smoothed);
	}
}

FVector ABackRoomPawn::WalkAt(float Distance) const
{
	// Catmull-Rom through the points, parameterized by distance along the chords.
	const int32 N = WalkPoints.Num();
	float D = FMath::Clamp(Distance, 0.0f, WalkTotal);
	int32 I = 0;
	while (I < WalkLengths.Num() - 1 && D > WalkLengths[I])
	{
		D -= WalkLengths[I];
		++I;
	}
	const float U = WalkLengths[I] > 0.0f ? FMath::Clamp(D / WalkLengths[I], 0.0f, 1.0f) : 0.0f;
	const FVector P0 = WalkPoints[FMath::Max(I - 1, 0)];
	const FVector P1 = WalkPoints[I];
	const FVector P2 = WalkPoints[I + 1];
	const FVector P3 = WalkPoints[FMath::Min(I + 2, N - 1)];
	const double U1 = U, U2 = U1 * U1, U3 = U2 * U1;
	return 0.5 * ((2.0 * P1) + (P2 - P0) * U1 + (2.0 * P0 - 5.0 * P1 + 4.0 * P2 - P3) * U2 + (3.0 * P1 - P0 - 3.0 * P2 + P3) * U3);
}

void ABackRoomPawn::TickWalk(float RealDt)
{
	WalkT += RealDt;
	const float T = FMath::Clamp(WalkT / WalkSeconds, 0.0f, 1.0f);
	// A walking pace: setting off, an even stride, and slowing to a stop.
	const float E = 0.3f * T + 0.7f * Smooth(0.0f, 1.0f, T);
	const float D = E * WalkTotal;
	FVector At = WalkAt(D);
	const FVector Ahead = WalkAt(D + 80.0f);
	// Each step lifts and drops the head a little and sways it side to side; it fades sitting down
	// (or builds standing up).
	const float Stride = bWalkOut ? Smooth(0.04f, 0.22f, T) : 1.0f - Smooth(0.7f, 0.9f, T);
	const float Steps = D / 66.0f * PI;
	At.Z += Stride * (1.5f * FMath::Abs(FMath::Sin(Steps)) - 0.75f);
	ABackRoomTable* Table = GetTable();
	UNightOneAudio* Sound = Table ? Table->GetAudio() : nullptr;
	// A footfall each stride on the concrete.
	const int32 Step = FMath::FloorToInt(D / 66.0f);
	if (Step != StepCount)
	{
		StepCount = Step;
		if (Sound && Stride > 0.25f)
		{
			Sound->PlayEffect(ss::audio::Effect::Step, 0.3f + 0.45f * Stride);
		}
	}
	const FVector Along = (Ahead - At).GetSafeNormal2D();
	const FVector Side = FVector::CrossProduct(FVector::UpVector, Along.IsNearlyZero() ? FVector::ForwardVector : Along);
	At += Side * (Stride * 0.9f * FMath::Sin(Steps * 0.5f));

	ABackRoomPlayer* Me = Table ? Table->GetHeroPlayer() : nullptr;
	const FVector TableAt(0.0, 0.0, 96.0);
	FRotator Look;
	if (!bWalkOut)
	{
		// Through the door looking where you're going, then at the table and who is at it, then down
		// into the seat, facing Dee.
		const FVector Target = FMath::Lerp(Ahead + FVector(0.0, 0.0, -6.0), TableAt, Smooth(0.1f, 0.4f, T));
		const FQuat Q = FQuat::Slerp((Target - At).Rotation().Quaternion(), FRotator(-14.0f, 0.0f, 0.0f).Quaternion(), Smooth(0.78f, 1.0f, T));
		Look = Q.Rotator();
		if (Me)
		{
			At = FMath::Lerp(At, Me->GetEyes() + FVector(1.5, 0.0, 0.0), static_cast<double>(Smooth(0.86f, 1.0f, T)));
			if (!bBodyShown && T > 0.88f)
			{
				// Pull the chair in and sit (the face stays out of the view the camera is settling into).
				Me->ShowHeroHead(false);
				Me->SetActorHiddenInGame(false);
				bBodyShown = true;
				if (Sound)
				{
					Sound->PlayEffect(ss::audio::Effect::Scrape, 0.8f);
				}
			}
		}
	}
	else
	{
		// Up out of the chair, one look back at the table on the way round, then the door.
		const float Back = Smooth(0.04f, 0.16f, T) * (1.0f - Smooth(0.4f, 0.58f, T));
		const FVector Target = FMath::Lerp(Ahead + FVector(0.0, 0.0, -4.0), TableAt, static_cast<double>(0.85f * Back));
		Look = (Target - At).Rotation();
		if (Me && bBodyShown && T > 0.05f)
		{
			Me->SetActorHiddenInGame(true);
			bBodyShown = false;
			if (Sound)
			{
				Sound->PlayEffect(ss::audio::Effect::Scrape, 0.9f);
			}
		}
	}
	Look.Roll = Stride * 0.5f * FMath::Sin(Steps * 0.5f);
	Smoothed = FMath::RInterpTo(Smoothed, Look, RealDt, bWalkOut ? 6.0f : 8.0f);
	FirstEye = At;
	SetActorLocationAndRotation(At, Smoothed);
	const ABackRoomGameMode* GM = GetMode();
	Camera->SetFieldOfView(GM ? GM->SettingFov(72.0f) : 72.0f);
	FPostProcessSettings& P = Camera->PostProcessSettings;
	P.DepthOfFieldFocalDistance = 220.0f;
	P.DepthOfFieldFstop = 5.6f;
	P.VignetteIntensity = 0.5f;
	P.ColorSaturation = FVector4(1.0, 1.0, 1.0, 1.0);
	P.SceneFringeIntensity = 0.0f;
	PeekLight->SetIntensity(0.0f);
	if (T >= 1.0f)
	{
		bWalking = false;
		if (!bWalkOut)
		{
			Yaw = 0.0f;
			Pitch = -14.0f;
			Smoothed = FRotator(-14.0f, 0.0f, 0.0f);
		}
		// Out of the chair and somewhere else: stay there (not back in the empty chair) until whatever's next moves on.
		// A Done that starts another walk, or goes on foot, lets go of it.
		bHoldWalkEnd = bWalkOut;
		TFunction<void()> Done = MoveTemp(WalkDone);
		WalkDone = nullptr;
		if (Done)
		{
			Done();
		}
	}
}

void ABackRoomPawn::TickHold(float RealDt)
{
	// Where the walk stopped (the cashier's window, the aisle while the room turns): the head looks about, nothing else.
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		const FVector2D Turn = LookDelta(PC, GetMode(), 1.0f, RealDt);
		Smoothed.Yaw += Turn.X;
		Smoothed.Pitch = FMath::Clamp(Smoothed.Pitch + Turn.Y, -50.0, 35.0);
	}
	Smoothed.Roll = FMath::FInterpTo(Smoothed.Roll, 0.0f, RealDt, 4.0f);
	SetActorRotation(Smoothed);
	PeekLight->SetIntensity(0.0f);
	float PitchKick = 0.0f, Racing = 0.0f;
	TickHeart(RealDt, PitchKick, Racing);
}

void ABackRoomPawn::TickHeart(float RealDt, float& PitchKick, float& Intensity)
{
	ABackRoomTable* Table = GetTable();
	const float Bpm = Table ? Table->Composure.Bpm : 68.0f;
	// You start to hear it in the high eighties; past ninety it takes the edges of the view.
	const float Audible = FMath::Clamp((Bpm - 86.0f) / 44.0f, 0.0f, 1.0f);
	Intensity = FMath::Clamp((Bpm - 92.0f) / 48.0f, 0.0f, 1.0f);
	BeatPhase += RealDt * Bpm / 60.0f;
	if (BeatPhase >= 1.0f)
	{
		BeatPhase -= FMath::FloorToFloat(BeatPhase);
		if (Audible > 0.0f)
		{
			if (UNightOneAudio* A = Table ? Table->GetAudio() : nullptr)
			{
				A->PlayEffect(ss::audio::Effect::Thump, 0.12f + 0.85f * Audible);
			}
			Kick = 1.0f;
		}
	}
	Kick = FMath::Max(0.0f, Kick - RealDt * 7.0f);
	PitchKick = -0.25f * Kick * Audible;
	Steady = Ease(Steady, bSteadying ? 1.0f : 0.0f, 2.0f, RealDt);
	const float Before = BreathT;
	BreathT += RealDt * (bSteadying ? 0.18f : 0.27f);
	// Steadying, you hear each long breath out.
	if (Steady > 0.4f && FMath::Frac(Before) < 0.5f && FMath::Frac(BreathT) >= 0.5f)
	{
		if (UNightOneAudio* A = Table ? Table->GetAudio() : nullptr)
		{
			A->PlayEffect(ss::audio::Effect::Breath, 0.9f * Steady);
		}
	}
}

void ABackRoomPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ABackRoomTable* Table = GetTable();
	ABackRoomGameMode* GM = GetMode();
	// Paused: the view holds where it is and nothing of yours moves on (the pause menu has the input, and the night
	// stands still behind it).
	if (GM && GM->IsPauseMenuUp())
	{
		// (The player's brightness and grain still change on it as they set them, behind the menu.)
		GM->ApplyPlayerLens(Camera);
		TestPressed.Reset();
		return;
	}
	// On the way out of each frame: the player's own lens (brightness, grain, fringing, motion blur) over the view's.
	auto EndFrame = [this, GM]() {
		if (GM)
		{
			GM->ApplyPlayerLens(Camera);
		}
		TestPressed.Reset();
	};
	// A quit playing the hand out behind the fade: on foot, the player stays where they stood (walking puts the room back
	// at its own pace, and the quit plays it quicker).
	if (GM && GM->IsQuittingTable() && (bFreeWalk || bHoldWalkEnd))
	{
		EndFrame();
		return;
	}
	// Focus slows the world; the head and the meter run on real time.
	const float RealDt = FMath::Min(static_cast<float>(FApp::GetDeltaTime()), 0.1f);
	Time += RealDt;
	// Walked somewhere (or the night's over at the table): the table card goes down with the table.
	if (bTableCard && !bFreeWalk && (bWalking || bHoldWalkEnd || bWalkPending || (GM && (GM->GetPhase() == EBackRoomPhase::Leaving || GM->IsQuittingTable()))))
	{
		bTableCard = false;
		TableCardClosedAt = FPlatformTime::Seconds();
	}
	if (bWalkPending && ThirdBlend <= 0.02f)
	{
		StartPendingWalk();
	}
	if (bWalking)
	{
		TickWalk(RealDt);
		EndFrame();
		return;
	}
	if (bFreeWalk)
	{
		TickFreeWalk(RealDt);
		EndFrame();
		return;
	}
	if (bHoldWalkEnd)
	{
		TickHold(RealDt);
		EndFrame();
		return;
	}
	ABackRoomPlayer* Me = Table ? Table->GetHeroPlayer() : nullptr;
	// Racked up: the hands are off the table; the head still looks around while Dee says goodnight. Quitting from the
	// pause menu: the hand in play is being folded behind the fade, and nothing more is yours to decide.
	const bool bDone = GM && GM->GetPhase() == EBackRoomPhase::Leaving;
	const bool bQuitting = GM && GM->IsQuittingTable();
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		// The mouse or the right stick (Focus slows it).
		const FVector2D Turn = LookDelta(PC, GM, 1.0f - 0.55f * Focus, RealDt);
		Yaw = FMath::Clamp(Yaw + static_cast<float>(Turn.X), -MaxYaw, MaxYaw);
		Pitch = FMath::Clamp(Pitch + static_cast<float>(Turn.Y), MinPitch, MaxPitch);
	}
	if (!bDone && !bWalkPending && !bQuitting)
	{
		HandleInput(RealDt);
	}
	else if (bWalkPending || bQuitting)
	{
		// Getting up (the camera on its way back into the head), or quitting: nothing more to decide at the table.
		bSteadying = false;
		Focus = Ease(Focus, 0.0f, 5.0f, RealDt);
		PeekBlend = Ease(PeekBlend, 0.0f, 6.0f, RealDt);
		if (Me)
		{
			Me->SetHeroPeek(false);
		}
		if (!bQuitting)
		{
			// (A quit plays the hand out quickly: the game mode sets the pace then.)
			UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
		}
	}
	else
	{
		// Out of it: Q (left on the d-pad) stays to watch the rest (on foot), L (down on the d-pad, held) heads home now.
		APlayerController* LeavePC = Cast<APlayerController>(GetController());
		const ABackRoomPlayerController* Pad = Cast<ABackRoomPlayerController>(LeavePC);
		if (GM && GM->IsLive() && (Pressed(LeavePC, EKeys::Q) || Pressed(LeavePC, EKeys::Gamepad_DPad_Left)))
		{
			GM->LiveStay();
		}
		else if (GM && GM->IsLive() && (Pressed(LeavePC, EKeys::L) || (Pad && Pad->HoldCompleted(EKeys::Gamepad_DPad_Down))))
		{
			GM->LiveLeaveNow();
		}
		bSteadying = false;
		Focus = Ease(Focus, 0.0f, 5.0f, RealDt);
		PeekBlend = Ease(PeekBlend, 0.0f, 6.0f, RealDt);
		if (Me)
		{
			Me->SetHeroPeek(false);
		}
		UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
	}
	float PitchKick = 0.0f, Racing = 0.0f;
	TickHeart(RealDt, PitchKick, Racing);

	FVector Eye = Me ? Me->GetEyes() + FVector(1.5, 0.0, 0.0) : Seat;
	// A slow, deep breath while steadying.
	Eye.Z += Steady * 1.1f * FMath::Sin(BreathT * 2.0f * PI);
	// The head itself: what the shoulder camera pivots on (a peek takes the first-person eye down to the cards).
	const FVector HeadEye = Eye;
	FRotator Want(Pitch, Yaw, 0.0f);
	FVector FocusAt = Eye + Want.Vector() * 120.0;
	float PeekLit = 0.0f;
	if (Me && Me->Hole.Num() == 2 && Me->Hole[0] && Me->Hole[1] && PeekBlend > 0.01f)
	{
		// Peeking: you drop down to the table (your chin nearly on the rail) behind the lifted corner and look
		// across at it: low and nearly level, so the lifted face turns to you, the hands framing it either side.
		const FVector Cards = (Me->Hole[0]->GetActorLocation() + Me->Hole[1]->GetActorLocation()) * 0.5;
		const FVector Corners = Me->GetPeekFocus();
		const FVector Near = FMath::Lerp(Cards, Corners, 0.7) + FVector(0.0, 0.0, 0.8);
		Eye = FMath::Lerp(Eye, Near + FVector(-20.0, 2.5 * ABackRoomCard::NearIndexSide(), 15.0), 0.94 * PeekBlend);
		const FRotator ToCards = (Near - Eye).Rotation();
		Want = FMath::Lerp(Want, FRotator(ToCards.Pitch, ToCards.Yaw, 0.0f), 0.95f * PeekBlend);
		FocusAt = Near;
		PeekLight->SetWorldLocation(Near + FVector(-16.0, 0.0, 14.0));
		PeekLit = PeekBlend;
	}
	PeekLight->SetIntensity(4.0f * PeekLit);
	if (ABackRoomPlayer* S = Studying.Get())
	{
		// Focus pulls the view onto the face.
		const FRotator ToFace = (S->GetEyes() - Eye).Rotation();
		Want = FMath::Lerp(Want, FRotator(ToFace.Pitch, ToFace.Yaw, 0.0f), 0.4f * Focus);
		FocusAt = FMath::Lerp(FocusAt, S->GetEyes(), static_cast<double>(Focus));
	}
	Smoothed = FMath::RInterpTo(Smoothed, Want, RealDt, 14.0f);
	// The heart in the view: a kick with each beat, and a fine tremor when it races.
	const float Tremor = 0.12f * Racing;
	const FRotator Shaken = Smoothed + FRotator(PitchKick + Tremor * FMath::Sin(Time * 37.0f) * FMath::Sin(Time * 11.3f), Tremor * FMath::Sin(Time * 29.0f + 1.7f), 0.0f);
	if (Me)
	{
		FirstEye = Eye;
		SetActorLocationAndRotation(Eye, Shaken);
		Me->SetHeroLook(Eye + Smoothed.Vector() * 200.0);
	}
	else
	{
		// No body yet: a head that breathes.
		const float Breath = FMath::Sin(Time * 1.35f);
		FirstEye = Seat + FVector(0.25 * Breath, 0.0, 0.35 * Breath);
		SetActorLocationAndRotation(FirstEye, Shaken + FRotator(0.2f * Breath, 0.0f, 0.15f * FMath::Sin(Time * 0.37f)));
	}
	if (!Studying.IsValid() && PeekBlend < 0.01f)
	{
		// Without a target the eyes rest where the view meets the felt (or a couple of meters out).
		const FVector Dir = Smoothed.Vector();
		const double Down = -Dir.Z;
		const double ToFelt = Down > 0.05 ? (Eye.Z - ABackRoomStage::FeltZ) / Down : 250.0;
		FocusAt = Eye + Dir * FMath::Clamp(ToFelt, 40.0, 250.0);
	}

	// The lens: Focus narrows it and opens the aperture; a racing heart closes in the edges. At rest it's the player's
	// field of view; the peek and Focus frame what they look at whatever it is.
	const float RestFov = GM ? GM->SettingFov(72.0f) : 72.0f;
	Camera->SetFieldOfView(FMath::Lerp(FMath::Lerp(RestFov, 42.0f, PeekBlend), 34.0f, Focus) - 4.0f * Racing);
	FPostProcessSettings& P = Camera->PostProcessSettings;
	P.DepthOfFieldFocalDistance = FMath::Max(10.0f, static_cast<float>(FVector::Dist(Eye, FocusAt)));
	P.DepthOfFieldFstop = FMath::Lerp(FMath::Lerp(4.0f, 8.0f, PeekBlend), 1.4f, Focus);
	P.VignetteIntensity = 0.45f + 0.5f * Focus + 0.55f * Racing + 0.18f * Racing * Kick;
	P.ColorSaturation = FVector4(1.0, 1.0, 1.0, 1.0 - 0.38 * Racing);
	P.SceneFringeIntensity = 1.1f * Racing + 0.7f * Racing * Kick;

	// Over the shoulder (V): the camera eases out of the head along its boom, the hero's own head in the shot. The camera
	// is the pawn's root, put back at the eyes above each tick, so there's nothing to restore.
	PlaceShoulderCamera(HeadEye, Shaken - Smoothed, Racing, RealDt);
	// Where the player's view is from and looking (the shoulder camera's when it's out), for who feels watched.
	const FVector ViewFrom = Camera->GetComponentLocation();
	const FVector ViewFwd = Camera->GetForwardVector();

	if (bExtCam)
	{
		Camera->SetWorldLocationAndRotation(ExtPos, (ExtAt - ExtPos).Rotation());
		Camera->SetFieldOfView(ExtFov);
		Camera->PostProcessSettings.DepthOfFieldFstop = 32.0f;
	}

	// The opponents feel being looked at: more so through Focus. Whoever is in the middle of the view counts, measured
	// from where the view is (the eyes, or over the shoulder), so reads are gathered from either.
	if (Table)
	{
		const float Lo = FMath::Cos(FMath::DegreesToRadians(9.0f));
		const float Hi = FMath::Cos(FMath::DegreesToRadians(2.5f));
		for (ABackRoomPlayer* O : Table->GetOpponents())
		{
			const float C = FVector::DotProduct(ViewFwd, (O->GetEyes() - ViewFrom).GetSafeNormal());
			O->SetStudied(bDone || bWalkPending || bQuitting ? 0.0f : FMath::Clamp((C - Lo) / (Hi - Lo), 0.0f, 1.0f) * (0.6f + 0.4f * Focus));
		}
	}
	EndFrame();
}

void ABackRoomPawn::PlaceShoulderCamera(const FVector& Head, const FRotator& Shake, float Racing, float RealDt)
{
	ABackRoomTable* Table = GetTable();
	ABackRoomPlayer* Me = Table ? Table->GetHeroPlayer() : nullptr;
	// Peeking and Focus are the first person's business, and so is getting up (quicker then: the walk is waiting on it).
	const float Want = bThirdPerson && !bWalkPending ? FMath::Clamp(1.0f - PeekBlend - Focus, 0.0f, 1.0f) : 0.0f;
	ThirdBlend = Ease(ThirdBlend, Want, bWalkPending ? 9.0f : 5.0f, RealDt);
	if (!Me)
	{
		ThirdBlend = 0.0f;
		bShoulderFresh = true;
		return;
	}
	// The head is built (full detail, the hair copied) while the choice is over the shoulder, and dropped a moment
	// after it's back to first person, so a quick V and V again doesn't rebuild it.
	if (bThirdPerson)
	{
		HeadIdleT = 0.0f;
		if (!Me->IsHeroHeadBuilt())
		{
			// Made now and drawn a few frames on: the hair is bound to the face (back at full detail) before either shows,
			// never in the same frame it's registered.
			Me->SetHeroHeadBuilt(true);
			HeadWarmFrames = 3;
		}
	}
	else if (ThirdBlend < 0.01f && Me->IsHeroHeadBuilt())
	{
		HeadIdleT += RealDt;
		if (HeadIdleT > 1.5f)
		{
			Me->SetHeroHeadBuilt(false);
		}
	}
	// A head just made can't be drawn yet: the camera waits in it until it can (a night opened over the shoulder would
	// otherwise start out behind a body with no head on it), then eases out.
	if (HeadWarmFrames > 0)
	{
		--HeadWarmFrames;
		ThirdBlend = 0.0f;
	}
	if (ThirdBlend <= 0.001f)
	{
		Me->ShowHeroHead(false);
		bShoulderFresh = true;
		return;
	}
	const float B = ThirdBlend * ThirdBlend * (3.0f - 2.0f * ThirdBlend);

	// Which shoulder: the player's (B), or the other one while they look well across that way. The camera stays behind
	// the chair (not out over a neighbor's), and their own head never sits between them and whoever they look at.
	// (B pressed while looking across holds their pick until the look comes back.)
	const float Across = static_cast<float>(Smoothed.Yaw) * ShoulderSideWant;
	if (bShoulderHeld)
	{
		bShoulderHeld = Across < -18.0f;
	}
	else if (!bShoulderFlipped && Across < -32.0f)
	{
		bShoulderFlipped = true;
	}
	else if (bShoulderFlipped && Across > -18.0f)
	{
		bShoulderFlipped = false;
	}
	const float SideNow = bShoulderFlipped ? -ShoulderSideWant : ShoulderSideWant;
	ShoulderSide = bShoulderFresh ? SideNow : Ease(ShoulderSide, SideNow, 3.5f, RealDt);

	// The boom: back from the eyes, out over the shoulder and a little up. It swings with part of the head's turn and
	// rises as the look drops (over the shoulder and down at the felt), trailing the head a little so it has some weight.
	const float Follow = FMath::Clamp(CVarShoulderFollow.GetValueOnGameThread(), 0.0f, 1.0f);
	const FRotator Orbit(FMath::Clamp(static_cast<float>(Smoothed.Pitch) * 0.45f, -22.0f, 6.0f), static_cast<float>(Smoothed.Yaw) * Follow, 0.0f);
	const FVector Right = FRotationMatrix(FRotator(0.0f, Orbit.Yaw, 0.0f)).GetScaledAxis(EAxis::Y);
	const FVector Boom = -Orbit.Vector() * FMath::Max(20.0f, CVarShoulderBack.GetValueOnGameThread()) + Right * (CVarShoulderSide.GetValueOnGameThread() * ShoulderSide) +
		FVector(0.0, 0.0, CVarShoulderUp.GetValueOnGameThread());
	const float Lag = CVarShoulderLag.GetValueOnGameThread();
	ShoulderAt = bShoulderFresh || Lag <= 0.0f ? Boom : FMath::VInterpTo(ShoulderAt, Boom, RealDt, Lag);
	// Kept clear of the room and the people at the table: in at once when something's in the way, back out slowly, as a
	// spring arm does.
	const float Clear = BoomClearance(Head, Head + ShoulderAt);
	BoomFrac = bShoulderFresh || Clear < BoomFrac ? Clear : Ease(BoomFrac, Clear, 3.0f, RealDt);
	bShoulderFresh = false;
	const FVector Shoulder = Head + ShoulderAt * BoomFrac;

	// What the head looks at, in the middle of the frame: where the gaze meets the felt, or a couple of meters out (a face
	// across the table). The heart's kick and tremor stay in the view.
	const FVector Gaze = Smoothed.Vector();
	const double Down = -Gaze.Z;
	const double AimDist = Down > 0.05 ? FMath::Clamp((Head.Z - ABackRoomStage::FeltZ) / Down, 110.0, 260.0) : 260.0;
	const FVector LookAt = Head + Gaze * AimDist;
	const FQuat Aim = ((LookAt - Shoulder).Rotation() + Shake).Quaternion();

	// From the first-person eye (where Tick just put the camera) out along the boom. A peek's eye, down at the cards,
	// only comes into it once the camera is nearly back in the head, so the way between never passes through a shoulder.
	const FVector FirstAt = GetActorLocation();
	const FVector Pos = Head + (Shoulder - Head) * static_cast<double>(B) + (FirstAt - Head) * static_cast<double>(FMath::Square(1.0f - B));
	Camera->SetWorldLocationAndRotation(Pos, FQuat::Slerp(GetActorQuat(), Aim, B));
	// A slightly longer lens than the eyes' (at the player's field of view), still squeezed by a racing heart; the felt and
	// the faces sharp, your own head soft in the foreground.
	const ABackRoomGameMode* GM = GetMode();
	const float ShoulderFov = GM ? GM->SettingFov(CVarShoulderFov.GetValueOnGameThread()) : CVarShoulderFov.GetValueOnGameThread();
	Camera->SetFieldOfView(FMath::Lerp(Camera->FieldOfView, ShoulderFov - 4.0f * Racing, B));
	FPostProcessSettings& P = Camera->PostProcessSettings;
	P.DepthOfFieldFocalDistance = FMath::Lerp(P.DepthOfFieldFocalDistance, static_cast<float>(FVector::Dist(Pos, LookAt)), B);
	P.DepthOfFieldFstop = FMath::Lerp(P.DepthOfFieldFstop, 7.0f, B);

	// The head shows once the camera is clear of it (the eyes are at the front of the skull: the back of it, the hair and
	// a hat are about 25 cm behind them), and goes again before the camera is back inside it.
	const double Gap = FVector::Dist(Pos, Head);
	Me->ShowHeroHead(Me->IsHeroHeadShown() ? Gap > 34.0 : Gap > 44.0);
}

float ABackRoomPawn::BoomClearance(const FVector& From, const FVector& To) const
{
	const double Length = FVector::Dist(From, To);
	if (Length < 1.0)
	{
		return 1.0f;
	}
	double T = 1.0;
	const ABackRoomGameMode* GM = GetMode();
	// The Back Room's walls and ceiling (nothing collides there: the stage is drawn, not simulated). The Embercrest is a
	// big room whose tables stand well apart: around the player's own, the people are what's near.
	if (GM && GM->Stage && !GM->Stage->bCardRoom)
	{
		const FTransform& Room = GM->Stage->GetActorTransform();
		T = FMath::Min(T, SegmentInBox(Room.InverseTransformPosition(From), Room.InverseTransformPosition(To), BackRoomInside));
	}
	// The people at the table: a head, and the shoulders under it (a hand's width more, for the lens).
	const ABackRoomTable* Table = GetTable();
	const ABackRoomPlayer* Me = Table ? Table->GetHeroPlayer() : nullptr;
	auto Around = [&](const ABackRoomPlayer* Who) {
		if (!Who || Who == Me || Who->IsHidden())
		{
			return;
		}
		const FVector Eyes = Who->GetEyes();
		if (FVector::DistSquared(Eyes, From) > FMath::Square(Length + 90.0))
		{
			return;
		}
		// (A seat faces the table: the skull and the shoulders are a little behind the eyes.)
		const FVector Facing = Who->GetActorForwardVector();
		T = FMath::Min(T, SegmentIntoSphere(From, To, Eyes - Facing * 8.0, 26.0));
		T = FMath::Min(T, SegmentIntoSphere(From, To, Eyes - Facing * 6.0 - FVector(0.0, 0.0, 42.0), 34.0));
	};
	if (Table)
	{
		for (const ABackRoomPlayer* O : Table->GetOpponents())
		{
			Around(O);
		}
	}
	if (GM)
	{
		Around(GM->Dealer);
	}
	return static_cast<float>(T);
}

// ------------------------------------------------------------------ the pause menu

/**
 * The table's pause menu: ss::ui::FrontEnd's Pause page on a Slate widget, the room blurred behind it, and its hooks. It
 * keeps the player's settings as the table uses them (applied as they change, saved once they settle), holds the night
 * still while it's up, and carries a confirmed quit out: R5's fold, the checkpoint or the night settled, then the main
 * menu or the desktop.
 *
 * Holding still: the room's time dilation goes to its floor, so nothing the world's clock drives moves (the table's
 * hand, the people at it, the timers, a walk's fade), and the mode's own ticks (the night's clock, the tournament's, a
 * break's countdown) don't run. The heart (which the table runs on real time) is put back as it was on resuming: a
 * pause is no place to calm down. The menu's clicks play on its own voice, which keeps real time.
 */
struct ABackRoomGameMode::FTableMenu final : public ss::ui::FrontEndHooks
{
	explicit FTableMenu(ABackRoomGameMode& InMode)
		: Mode(InMode)
		, Menu(*this)
	{
	}

	ABackRoomGameMode& Mode;
	ss::ui::FrontEnd Menu;
	FSlateTextMeasurer Measurer;
	FDrawListHint Hint;
	TSharedPtr<SOverlay> Root;
	TSharedPtr<SBackgroundBlur> Blur;
	TSharedPtr<SFrontEndWidget> Widget;
	TWeakObjectPtr<UNightOneAudio> Voice;
	/** The menu's clock (real seconds since the night began), and when it last closed (it fades out after). */
	double RealTime = 0.0;
	double ClosedAt = -100.0;
	/** Where the pointer last was over the menu (negative: not since it opened). */
	FVector2D LastPointer = FVector2D(-1.0, -1.0);
	/** When the settings last changed (written once the player stops changing them), or negative. */
	double SettingsDirtyAt = -1.0;

	// The night held still behind the menu.
	bool bFrozen = false;
	float FrozenDilation = 1.0f;
	FBackRoomComposure FrozenHeart;

	// The player's settings as the table uses them.
	float LookScale = 1.0f;
	bool bInvertLook = false;
	bool bShowHints = true;
	int32 FieldOfView = 50;
	float BrightnessBias = 0.0f;
	float GrainScale = 1.0f;
	float FringeScale = 1.0f;
	bool bMotionBlur = false;
	/** The room's own exposure and film grain (the stage's lens, read from LensOf): the player's brightness and grain go over them. */
	float RoomBias = 0.0f;
	float RoomGrain = 0.2f;
	TWeakObjectPtr<const ABackRoomStage> LensOf;

	// A confirmed quit.
	bool bQuitting = false;
	bool bQuitToDesktop = false;
	bool bLeft = false;
	double QuitAt = 0.0;
	/** The Embercrest's table has dealt the player a hand it hasn't ended yet (from the table's notes: OnTableNote). */
	bool bHandDealt = false;

	/** The saved settings applied (the mix, the frame budget, the look, the lens, the table's pace), the widget and the voice. */
	void Begin();
	void End();
	void Open();
	void Tick(float RealDt);
	void SetGamepad(bool bOn);
	void Freeze();
	void Thaw();
	void ReturnInputToGame();
	/** The pause page's facts: where, when, how the night stands, and what quitting from here costs and keeps. */
	void Describe();
	/** bAll: everything, as when the menu changes it (the display and scalability too, which the apartment already set). */
	void Apply(const ss::ui::GameSettings& S, bool bAll);
	void SaveSettings();
	/**
	 * A hand dealt to the player that hasn't ended: at Dee's game one on the table, at the Embercrest one its checkpoint
	 * marked dealt (a hand seated for the next deal, while the player walks in or moves, isn't one yet).
	 */
	bool HandInPlay() const;
	void Quit(bool bToDesktop);
	/**
	 * A hand in play (R5): the menu goes, the screen goes black, and the engine plays the hand out quickly with the player
	 * folding at their next decision (TickQuit), so what they put in stays in the pot and a hand already decided stands.
	 */
	void PlayOut();
	/** The hand played out: Dee's game racks up from the engine's stack; the Embercrest leaves once the room has saved it. */
	void TickQuit();
	void Leave();

	// ss::ui::FrontEndHooks
	virtual void UiSound(ss::SoundId Id, double Volume) override
	{
		if (UNightOneAudio* A = Voice.Get())
		{
			A->Play(Id, static_cast<float>(Volume));
		}
	}
	virtual void Resume() override
	{
		ClosedAt = RealTime;
		Thaw();
		ReturnInputToGame();
	}
	virtual void QuitToMenu() override { Quit(false); }
	virtual void QuitGame() override { Quit(true); }
	virtual void SettingsChanged(const ss::ui::GameSettings& S) override
	{
		Apply(S, true);
		SettingsDirtyAt = RealTime; // written once the player stops changing things
	}
};

void ABackRoomGameMode::FTableMenu::Begin()
{
	if (UGameplayStatics::DoesSaveGameExist(UNightOneSaveGame::SettingsSlotName(), 0))
	{
		if (UNightOneSaveGame* Obj = Cast<UNightOneSaveGame>(UGameplayStatics::LoadGameFromSlot(UNightOneSaveGame::SettingsSlotName(), 0)))
		{
			ss::ui::GameSettings::Parse(ToUtf8(Obj->Data), Menu.Settings);
		}
	}
	UNightOneAudio* A = NewObject<UNightOneAudio>(&Mode, TEXT("MenuVoice"));
	A->RegisterComponent();
	Voice = A;
	Apply(Menu.Settings, false);

	UGameViewportClient* Viewport = Mode.GetWorld() ? Mode.GetWorld()->GetGameViewport() : nullptr;
	if (!Viewport || !FSlateApplication::IsInitialized())
	{
		return;
	}
	TWeakObjectPtr<ABackRoomGameMode> Weak = &Mode;
	Widget = SNew(SFrontEndWidget);
	Widget->SetVisibility(EVisibility::Collapsed);
	Widget->OnMenuKey = [Weak](const FString& KeyName, bool bFromGamepad) {
		if (Weak.IsValid() && Weak->TableMenu)
		{
			FTableMenu& M = *Weak->TableMenu;
			M.SetGamepad(bFromGamepad);
			M.Menu.Key(ToUtf8(KeyName), M.RealTime);
		}
	};
	Widget->OnMenuChar = [Weak](uint32 Codepoint) {
		if (Weak.IsValid() && Weak->TableMenu)
		{
			Weak->TableMenu->Menu.Char(Codepoint, Weak->TableMenu->RealTime);
		}
	};
	Widget->OnMenuPointer = [Weak](const FVector2D& Logical, int32 Button, float WheelDelta) {
		if (!Weak.IsValid() || !Weak->TableMenu)
		{
			return;
		}
		FTableMenu& M = *Weak->TableMenu;
		ss::ui::Pointer& P = M.Menu.Ptr;
		// The mouse in use (a click, the wheel, or really moved: Slate also reports it standing still as the menu appears
		// under it): the menu, and the table after it, show the keys again.
		const bool bMoved = M.LastPointer.X >= 0.0 && FVector2D::Distance(M.LastPointer, Logical) > 2.0;
		M.LastPointer = Logical;
		if (Button != 0 || WheelDelta != 0.0f || bMoved)
		{
			M.SetGamepad(false);
		}
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
	SAssignNew(Root, SOverlay)
	+ SOverlay::Slot()
	[
		SAssignNew(Blur, SBackgroundBlur)
		.BlurStrength(0.0f)
		.Visibility(EVisibility::Collapsed)
	]
	+ SOverlay::Slot()
	[
		Widget.ToSharedRef()
	];
	Viewport->AddViewportWidgetContent(Root.ToSharedRef(), 30);
}

void ABackRoomGameMode::FTableMenu::End()
{
	if (SettingsDirtyAt >= 0.0)
	{
		SaveSettings();
	}
	Thaw();
	if (Root)
	{
		if (UGameViewportClient* Viewport = Mode.GetWorld() ? Mode.GetWorld()->GetGameViewport() : nullptr)
		{
			Viewport->RemoveViewportWidgetContent(Root.ToSharedRef());
		}
	}
	Root.Reset();
	Blur.Reset();
	Widget.Reset();
}

void ABackRoomGameMode::FTableMenu::SetGamepad(bool bOn)
{
	Menu.Gamepad = bOn;
	if (ABackRoomPlayerController* PC = Cast<ABackRoomPlayerController>(UGameplayStatics::GetPlayerController(&Mode, 0)))
	{
		PC->SetUsingGamepad(bOn);
	}
}

void ABackRoomGameMode::FTableMenu::Open()
{
	if (!Widget || Menu.IsOpen() || bQuitting || Mode.bGoingHome)
	{
		return;
	}
	Describe();
	const ABackRoomPlayerController* PC = Cast<ABackRoomPlayerController>(UGameplayStatics::GetPlayerController(&Mode, 0));
	Menu.Gamepad = PC && PC->IsUsingGamepad();
	LastPointer = FVector2D(-1.0, -1.0);
	Menu.Open(ss::ui::FrontEnd::Page::Pause, RealTime);
	Freeze();
	Widget->SetVisibility(EVisibility::Visible);
	if (APlayerController* Pc = UGameplayStatics::GetPlayerController(&Mode, 0))
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(Widget);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Pc->SetInputMode(InputMode);
		Pc->SetShowMouseCursor(true);
		// What was held going in (a peek, Focus, a bumper) doesn't stay held on the way out.
		Pc->FlushPressedKeys();
	}
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetKeyboardFocus(Widget, EFocusCause::SetDirectly);
	}
	UE_LOG(LogBackRoom, Log, TEXT("Paused at %s%s"), *Mode.ClockLabel(), Mode.bLive ? TEXT(" (both clocks stopped)") : TEXT(""));
}

void ABackRoomGameMode::FTableMenu::ReturnInputToGame()
{
	if (Widget)
	{
		// Still painted while the menu fades out, but no longer takes input.
		Widget->SetVisibility(EVisibility::HitTestInvisible);
	}
	if (APlayerController* Pc = UGameplayStatics::GetPlayerController(&Mode, 0))
	{
		Pc->SetInputMode(FInputModeGameOnly());
		Pc->SetShowMouseCursor(false);
		// Keys let go while the menu had them would otherwise still read as held (a peek that never comes down).
		Pc->FlushPressedKeys();
	}
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
	}
}

void ABackRoomGameMode::FTableMenu::Freeze()
{
	UWorld* World = Mode.GetWorld();
	AWorldSettings* Ws = World ? World->GetWorldSettings() : nullptr;
	if (bFrozen || !Ws)
	{
		return;
	}
	bFrozen = true;
	FrozenDilation = UGameplayStatics::GetGlobalTimeDilation(&Mode);
	UGameplayStatics::SetGlobalTimeDilation(&Mode, FMath::Max(Ws->MinGlobalTimeDilation, 0.0001f));
	// The mode (and the menu's voice on it) keeps real time.
	Mode.CustomTimeDilation = 1.0f / FMath::Max(Ws->GetEffectiveTimeDilation(), 0.0001f);
	if (Mode.Table)
	{
		FrozenHeart = Mode.Table->Composure;
	}
}

void ABackRoomGameMode::FTableMenu::Thaw()
{
	if (!bFrozen)
	{
		return;
	}
	bFrozen = false;
	UGameplayStatics::SetGlobalTimeDilation(&Mode, FrozenDilation);
	Mode.CustomTimeDilation = 1.0f;
	if (Mode.Table)
	{
		Mode.Table->Composure = FrozenHeart;
	}
}

void ABackRoomGameMode::FTableMenu::Tick(float RealDt)
{
	RealTime += RealDt;
	if (SettingsDirtyAt >= 0.0 && RealTime - SettingsDirtyAt > 0.75)
	{
		SaveSettings();
	}
	// Drawn while it's up and while it fades out (the room blurred behind it as much as the menu says).
	if (Widget && (Menu.IsOpen() || Widget->GetVisibility() != EVisibility::Collapsed))
	{
		FVector2D ViewSize(1920.0, 1080.0);
		if (UGameViewportClient* Viewport = Mode.GetWorld() ? Mode.GetWorld()->GetGameViewport() : nullptr)
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
		TSharedPtr<ss::ui::DrawList> List = Hint.Make();
		ss::ui::Canvas Cv(*List, Measurer, LogicalW, LogicalH, static_cast<float>(ViewSize.Y) / LogicalH);
		Menu.Draw(Cv, RealTime);
		Hint.Note(*List);
		Widget->SetDrawList(List);
		const float Backdrop = Menu.Backdrop(RealTime);
		if (Blur)
		{
			Blur->SetBlurStrength(Backdrop * 14.0f);
			Blur->SetVisibility(Backdrop > 0.001f ? EVisibility::HitTestInvisible : EVisibility::Collapsed);
		}
		if (!Menu.IsOpen() && RealTime - ClosedAt > 0.65)
		{
			Widget->SetVisibility(EVisibility::Collapsed);
		}
	}
	if (bQuitting)
	{
		TickQuit();
	}
}

void ABackRoomGameMode::FTableMenu::Describe()
{
	ABackRoomGameMode& M = Mode;
	ss::ui::FrontEndInfo I;
	I.Resolutions = TableResolutions();
	if (M.Save.IsValid())
	{
		const ss::SaveData& D = *M.Save;
		I.HasSave = true;
		I.HeroName = D.HeroName;
		I.Person = D.Person;
		I.RentStage = D.Life.RentStage;
		I.RentDueCents = D.Life.RentDueCents;
		I.RentDeadline = D.Life.RentDeadline;
		I.StorageDue = D.Life.StorageDue;
	}
	I.World = M.Minutes;
	const FBackRoomPrompt P = M.Table ? M.Table->GetPrompt() : FBackRoomPrompt();
	const bool bInHand = M.Table && M.Table->IsHeroInHand();
	// What a quit does to the hand in play (R5): folded at the player's next decision; all in, there's nothing to fold
	// and the engine's hand plays out as dealt.
	const std::string HandCost = !bInHand ? "" : P.Stack <= 0 ? "You're all in: the hand plays out before you go." : "This hand will be folded.";
	const FString Clock = Clock12(M.TimeOfDay());
	auto Row = [&I](const FString& Name, const FString& Value) { I.Status.push_back({ToUtf8(Name), ToUtf8(Value)}); };
	if (M.bLive)
	{
		// The Embercrest: the event by its name on the heading, the tournament as it stands on the card.
		FString Event = M.LiveName;
		Event.RemoveFromStart(TEXT("Embercrest "));
		Menu.SetVenue(ss::ui::Venue::CardRoom, ToUtf8(TEXT("THE EMBERCREST · ") + Event.ToUpper()));
		Row(TEXT("EVENT"), M.LiveName);
		if (const ss::Tournament* T = M.Tourney.Get(); T && !M.bLiveOver && !M.bCancelled)
		{
			// The clock's own facts (live::Clock: the same numbers as the clock card and the room's screens; spec 6.7).
			const ss::Level& L = T->CurrentLevel();
			auto N = [](int64 V) { return FText::AsNumber(V).ToString(); };
			const int64 Stack = T->Hero().Stack;
			const FString Bbs = UTF8_TO_TCHAR(ss::chipset::InBb(Stack, L.Bb).c_str());
			Row(TEXT("YOUR STACK"), Bbs.IsEmpty() ? N(Stack) : FString::Printf(TEXT("%s   ·   %s"), *N(Stack), *Bbs));
			Row(TEXT("LEVEL"), L.Ante > 0 ? FString::Printf(TEXT("%d   ·   %s / %s   ·   ANTE %s"), T->LevelIndex + 1, *N(L.Sb), *N(L.Bb), *N(L.Ante))
										  : FString::Printf(TEXT("%d   ·   %s / %s"), T->LevelIndex + 1, *N(L.Sb), *N(L.Bb)));
			const FTableHost* Host = M.TableHost.Get();
			if (Host && Host->bHasClock && Host->Clock.BreakBeforeLevel > 0)
			{
				Row(TEXT("NEXT BREAK"), FString::Printf(TEXT("BEFORE LEVEL %d"), Host->Clock.BreakBeforeLevel));
			}
			Row(TEXT("PLAYERS LEFT"), FString::Printf(TEXT("%d of %d"), T->Remaining, T->Spec.Entrants));
			const int32 Paid = T->PaidPlaces();
			Row(TEXT("PAID"), T->InTheMoney() ? FString::Printf(TEXT("%d   ·   IN THE MONEY"), Paid) : FString::Printf(TEXT("%d   ·   %d TO GO"), Paid, FMath::Max(0, T->Remaining - Paid)));
			Row(TEXT("YOUR RANK"), OrdinalText(T->HeroRank()));
			I.PauseNote = "The tournament clock is stopped. Pausing isn't sitting out: nothing is dealt without you.";
			if (T->Hero().Busted || T->bFinished)
			{
				// Out by the engine's count, the table not yet at the deal that notes it: the quit settles it first.
				I.QuitKept = "Your finish and your bankroll are saved.";
			}
			else
			{
				I.QuitKept = HandInPlay() ? "The tournament is saved after this hand, and your seat waits: Continue takes you straight back to the table."
										  : "The tournament is saved after every hand, and your seat waits: Continue takes you straight back to the table.";
			}
			I.QuitWarning = HandCost;
		}
		else
		{
			if (!M.bCancelled && M.HeroPlace > 0)
			{
				Row(TEXT("FINISHED"), FString::Printf(TEXT("%s of %d"), *OrdinalText(M.HeroPlace), M.Tourney ? M.Tourney->Spec.Entrants : 0));
			}
			if (M.HeroPrizeCents > 0)
			{
				Row(TEXT("PRIZE"), CentsText(M.HeroPrizeCents));
			}
			I.PauseNote = M.bRailing ? "The room waits while you're paused." : "";
			I.QuitKept = M.bCancelled ? "Your refund and your bankroll are saved." : "Your finish and your bankroll are saved.";
			if (!M.NextEntryId.IsEmpty() && !M.bOpeningNext)
			{
				// Registered at the desk for the next event: leaving takes the player off its list, as the door does.
				I.QuitKept += ToUtf8(FString::Printf(TEXT(" The desk takes you off the list for the %s and refunds you."), *M.NextName));
			}
			else if (!M.NextEntryId.IsEmpty())
			{
				// Its seats called and the room opening for it: the entry stands, and Continue goes back to it.
				I.QuitKept += ToUtf8(FString::Printf(TEXT(" Your seat in the %s waits: Continue takes you to it."), *M.NextName));
			}
		}
		if (M.Save.IsValid())
		{
			Row(TEXT("BANKROLL"), CentsText(M.Save->BankrollCents));
		}
		Row(TEXT("TIME"), Clock);
	}
	else if (M.IsCareer())
	{
		// Dee's game: the money as it stands (the chips in front of you are bankroll money), the night so far.
		Menu.SetVenue(ss::ui::Venue::BackRoom, "THE BACK ROOM \xC2\xB7 WASH & FOLD");
		const int64 Behind = M.Phase == EBackRoomPhase::Leaving ? 0 : P.Stack;
		const int64 Bankroll = M.Phase == EBackRoomPhase::Leaving ? M.StartBankrollCents : M.BaseCents + Behind * 100;
		Row(TEXT("BANKROLL"), CentsText(Bankroll));
		if (M.Phase != EBackRoomPhase::Leaving)
		{
			Row(TEXT("ON THE TABLE"), Dollars(Behind));
		}
		const int64 Net = M.Phase == EBackRoomPhase::Leaving ? M.NetCents : Bankroll - M.StartBankrollCents;
		// (Under the card's TONIGHT: the night's net, as the summary says it.)
		Row(TEXT("NET"), (Net >= 0 ? TEXT("+") : TEXT("-")) + Dollars(Net / 100));
		Row(TEXT("HANDS"), FString::FromInt(M.HandsPlayed));
		Row(TEXT("TIME"), Clock);
		if (M.Phase == EBackRoomPhase::Leaving)
		{
			I.QuitKept = "The night is settled and saved.";
		}
		else if (M.Phase == EBackRoomPhase::Arriving || (M.HandsPlayed == 0 && M.Table && M.Table->GetHandNumber() == 0))
		{
			I.PauseNote = "The night's clock is stopped while you're paused.";
			I.QuitKept = "Nothing's been played yet: your bankroll and career are saved.";
		}
		else
		{
			I.PauseNote = "The night's clock is stopped while you're paused.";
			I.QuitKept = M.Phase == EBackRoomPhase::Busted ? "You're felted: the night is settled and saved on the way out."
						 : HandInPlay()					   ? "You rack up after this hand: your chips go back in your bankroll, and the night is saved."
														   : "You rack up on the way out: your chips go back in your bankroll, and the night is saved.";
			I.QuitWarning = HandCost;
		}
	}
	else
	{
		// Practice: the table on its own, no save and no clock.
		Menu.SetVenue(ss::ui::Venue::BackRoom, "THE BACK ROOM \xC2\xB7 PRACTICE TABLE");
		I.HeroName = "Practice table";
		I.Timeless = true;
		Row(TEXT("STACK"), Dollars(P.Stack));
		Row(TEXT("HANDS"), FString::FromInt(M.Table ? M.Table->GetHandNumber() : 0));
		I.QuitKept = "Nothing at the practice table is saved.";
	}
	Menu.Info = I;
}

void ABackRoomGameMode::FTableMenu::Apply(const ss::ui::GameSettings& S, bool bAll)
{
	ABackRoomGameMode& M = Mode;
	if (bAll)
	{
		// As the apartment and the street apply them (NightOneGameMode::ApplySettings): scalability, the render scale and the
		// frame cap, the display, ray-traced Lumen. (Kept from scene to scene, so a level's start leaves them be.)
		if (UGameUserSettings* User = GEngine ? GEngine->GetGameUserSettings() : nullptr)
		{
			User->SetOverallScalabilityLevel(FMath::Clamp(S.Quality, 0, 4));
			User->SetResolutionScaleValueEx(static_cast<float>(FMath::Min(S.ResolutionScale, 100)));
			User->SetFrameRateLimit(static_cast<float>(S.FrameRateLimit));
			User->SetVSyncEnabled(S.VSync);
			if (GIsEditor)
			{
				// Play-In-Editor shares the editor's window: leave its size and mode alone.
				User->ApplyNonResolutionSettings();
			}
			else
			{
				const FIntPoint Res = TableParseResolution(S.Resolution);
				User->SetScreenResolution(Res.X > 0 ? Res : User->GetDesktopResolution());
				User->SetFullscreenMode(S.WindowMode == 0 ? EWindowMode::Fullscreen : S.WindowMode == 1 ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed);
				User->ApplySettings(false);
			}
		}
		SetTableCvar(TEXT("r.Lumen.HardwareRayTracing"), S.RayTracing ? 1 : 0);
		SetTableCvar(TEXT("r.Lumen.HardwareRayTracing.LightingMode"), S.RayTracing && S.Quality >= 4 ? 2 : 0);
	}
	// The render scale: fixed, or moving to hold the chosen frame rate (FrameBudget.h).
	M.FrameBudget.Configure(S.DynamicTarget, S.ResolutionScale);
	// The mix: the room's and the menu's own voice, with the room's voices (its speech and the PA) at their own level (spec
	// 4.9). The engine mutes a game whose window isn't in front unless told otherwise.
	for (UNightOneAudio* A : {M.Table ? M.Table->GetAudio() : nullptr, Voice.Get()})
	{
		if (A)
		{
			ApplyMix(*A, S.MasterVolume / 100.0f, S.EffectsVolume / 100.0f, S.AmbienceVolume / 100.0f, FMath::Clamp(S.VoicesVolume, 0, 100) / 100.0f);
		}
	}
	// The table's own: the turn cue as the player set it.
	if (M.Table)
	{
		M.Table->TurnCue = FMath::Clamp(S.TurnCue, 0, 2);
	}
	FApp::SetUnfocusedVolumeMultiplier(S.BackgroundAudio ? 1.0f : 0.0f);
	// The look, the controls reminder, the view and the lens (the pawn and the HUD read them each frame).
	LookScale = static_cast<float>(S.LookSensitivity) / 100.0f;
	bInvertLook = S.InvertLook;
	bShowHints = S.ShowHints;
	FieldOfView = S.FieldOfView;
	BrightnessBias = static_cast<float>(S.Brightness - 50) / 50.0f * 1.5f;
	GrainScale = S.FilmGrain == 0 ? 0.0f : S.FilmGrain == 1 ? 1.0f : 2.2f;
	FringeScale = S.ChromaticAberration ? 1.0f : 0.0f;
	bMotionBlur = S.MotionBlur;
	// The live table's pace.
	const int32 Pace = FMath::Clamp(S.TablePace, 0, 2);
	if (Pace != M.TablePace || !bAll)
	{
		M.TablePace = Pace;
		M.ApplyTablePace();
	}
}

void ABackRoomGameMode::FTableMenu::SaveSettings()
{
	SettingsDirtyAt = -1.0;
	if (UNightOneSaveGame* Obj = Cast<UNightOneSaveGame>(UGameplayStatics::CreateSaveGameObject(UNightOneSaveGame::StaticClass())))
	{
		Obj->Data = FString(UTF8_TO_TCHAR(Menu.Settings.Serialize().c_str()));
		UGameplayStatics::SaveGameToSlot(Obj, UNightOneSaveGame::SettingsSlotName(), 0);
	}
}

void ABackRoomGameMode::FTableMenu::Quit(bool bToDesktop)
{
	if (bQuitting)
	{
		return;
	}
	ABackRoomGameMode& M = Mode;
	bQuitting = true;
	bQuitToDesktop = bToDesktop;
	QuitAt = RealTime;
	SaveSettings();
	if (M.bLive)
	{
		if (M.Tourney && !M.bLiveOver && !M.bCancelled)
		{
			// (Out in the last hand by the engine's count, and the table not yet at the deal that notes it: settled first.)
			const bool bOutUnsettled = M.Tourney->Hero().Busted || M.Tourney->bFinished;
			if (HandInPlay() || bOutUnsettled)
			{
				// Never left dead to be dealt again from the checkpoint before it: that would hand back what the player put
				// in, and throw away a hand the engine had already decided. Played out, then saved as every hand is.
				PlayOut();
				UE_LOG(LogBackRoom, Log, TEXT("Quitting the Embercrest at %s: %s first"), *M.ClockLabel(), bOutUnsettled ? TEXT("the finish settled") : TEXT("the hand folded, played out and saved"));
				return;
			}
			// The last hand's save, if the room hadn't written it yet.
			if (M.bCheckpointDue && M.Table && M.Table->IsBetweenHands() && !M.Tourney->FinishPending())
			{
				M.LiveCheckpoint(false);
			}
		}
		else
		{
			// Out (or champion) and watching the rest: what they saw goes to the world, as walking out the door would; and, as
			// going home does, never with a seat waiting tonight (the desk takes the player off the next event's list).
			M.NoteRailedPlaces();
			if (!M.NextEntryId.IsEmpty() && !M.bOpeningNext)
			{
				M.LiveUnregister();
			}
		}
		if (M.LiveSaver)
		{
			M.LiveSaver->Flush();
		}
		UE_LOG(LogBackRoom, Log, TEXT("Quit from the Embercrest at %s"), *M.ClockLabel());
		Leave();
		return;
	}
	if (!M.IsCareer() || M.Phase == EBackRoomPhase::Leaving || !M.Table)
	{
		// Practice keeps nothing; a night racked up is settled already.
		Leave();
		return;
	}
	if (M.Phase == EBackRoomPhase::Arriving || (M.HandsPlayed == 0 && M.Table->GetHandNumber() == 0))
	{
		// Not sat down yet, or not dealt in yet: nothing was played, so there's no night to settle. The bankroll and the
		// clock as they stand.
		M.SaveCareer(false);
		Leave();
		return;
	}
	if (M.Table->IsBetweenHands())
	{
		// Racked up on the way out: the night settled (the chips into the bankroll, the ledger, what the regulars saw).
		M.BeginLeaving();
		Leave();
		return;
	}
	// A hand in play: the night settles from what's left once it's over (the chips they'd put in are the pot's).
	PlayOut();
	UE_LOG(LogBackRoom, Log, TEXT("Quitting Dee's game mid-hand: folding it, then settling the night"));
}

bool ABackRoomGameMode::FTableMenu::HandInPlay() const
{
	return Mode.Table && (Mode.bLive ? bHandDealt : !Mode.Table->IsBetweenHands());
}

void ABackRoomGameMode::FTableMenu::PlayOut()
{
	Menu.Close(RealTime);
	ClosedAt = RealTime;
	Thaw();
	if (APlayerController* Pc = UGameplayStatics::GetPlayerController(&Mode, 0))
	{
		// The closed menu keeps the input (still painted while it fades), so nothing reaches the table meanwhile.
		FInputModeUIOnly InputMode;
		if (Widget)
		{
			Widget->SetVisibility(EVisibility::HitTestInvisible);
			InputMode.SetWidgetToFocus(Widget);
		}
		Pc->SetInputMode(InputMode);
		Pc->SetShowMouseCursor(false);
		Pc->FlushPressedKeys();
	}
}

void ABackRoomGameMode::FTableMenu::TickQuit()
{
	ABackRoomGameMode& M = Mode;
	if (bLeft || !M.Table)
	{
		Leave();
		return;
	}
	// Black, and held there over any fade of the room's own (a blink at a bust, a move), until the level goes.
	if (APlayerController* Pc = UGameplayStatics::GetPlayerController(&M, 0); Pc && Pc->PlayerCameraManager)
	{
		Pc->PlayerCameraManager->SetManualCameraFade(static_cast<float>(FMath::Clamp((RealTime - QuitAt) / 0.35, 0.0, 1.0)), FLinearColor::Black, true);
	}
	// Quickly (nobody's watching), the hand's own pace otherwise: the cards and the engine are the same.
	UGameplayStatics::SetGlobalTimeDilation(&M, 8.0f);
	if (M.Table->GetPrompt().bYourTurn)
	{
		M.Table->HeroFold();
	}
	bool bOver = false;
	if (M.bLive)
	{
		// Over once the room has heard the hand's news and written its checkpoint (between hands, as it always does), or
		// the night is: a bust in it is settled first (the table notes it at its next deal; the engine says so already).
		const bool bOut = M.Tourney && (M.Tourney->Hero().Busted || M.Tourney->bFinished);
		bOver = M.bLiveOver || M.bCancelled || (!bHandDealt && !M.bCheckpointDue && !bOut);
	}
	else
	{
		bOver = M.Phase == EBackRoomPhase::Leaving || M.Table->IsBetweenHands();
	}
	if (!bOver && RealTime - QuitAt < 20.0)
	{
		return;
	}
	if (M.bLive)
	{
		// (Unfinished after all that: the checkpoint the deal wrote stands, and the hand is dead when the night comes back.)
		UE_CLOG(!bOver, LogBackRoom, Warning, TEXT("The hand didn't finish behind the fade (%s): leaving on the dealt checkpoint"), *M.Table->Describe());
		if (M.LiveSaver)
		{
			M.LiveSaver->Flush();
		}
	}
	else
	{
		UE_CLOG(!bOver, LogBackRoom, Warning, TEXT("The hand didn't finish behind the fade (%s): settling as it stands"), *M.Table->Describe());
		if (M.Phase != EBackRoomPhase::Leaving)
		{
			M.BeginLeaving();
		}
	}
	Leave();
}

void ABackRoomGameMode::FTableMenu::Leave()
{
	if (bLeft)
	{
		return;
	}
	bLeft = true;
	ABackRoomGameMode& M = Mode;
	// Nothing else takes the player anywhere now (the walk home, the door).
	M.bGoingHome = true;
	ABackRoomChips::ChipUnit = 1;
	if (bQuitToDesktop)
	{
		UKismetSystemLibrary::QuitGame(&M, UGameplayStatics::GetPlayerController(&M, 0), EQuitPreference::Quit, false);
	}
	else
	{
		UGameplayStatics::OpenLevel(&M, FName(TEXT("NightOne")), true, TEXT("Menu"));
	}
}

void ABackRoomGameMode::OpenPauseMenu()
{
	if (TableMenu)
	{
		TableMenu->Open();
	}
}

bool ABackRoomGameMode::IsPauseMenuUp() const
{
	return TableMenu && TableMenu->Menu.IsOpen();
}

bool ABackRoomGameMode::IsQuittingTable() const
{
	return TableMenu && TableMenu->bQuitting;
}

float ABackRoomGameMode::GetLookScale() const
{
	return TableMenu ? TableMenu->LookScale : 1.0f;
}

bool ABackRoomGameMode::IsLookInverted() const
{
	return TableMenu && TableMenu->bInvertLook;
}

bool ABackRoomGameMode::ShowsHints() const
{
	return !TableMenu || TableMenu->bShowHints;
}

const ss::ui::GameSettings* ABackRoomGameMode::GetSettings() const
{
	return TableMenu ? &TableMenu->Menu.Settings : nullptr;
}

void ABackRoomGameMode::FlipStackUnits()
{
	// U (or Y with the table card up): big blinds and chips swap places on the plates, the hero block and the card; kept
	// with the rest of the player's settings (the Stack units row), written once they stop changing things.
	if (!TableMenu)
	{
		return;
	}
	ss::ui::GameSettings& S = TableMenu->Menu.Settings;
	S.StackUnits = S.StackUnits == 0 ? 1 : 0;
	TableMenu->SettingsDirtyAt = TableMenu->RealTime;
}

float ABackRoomGameMode::SettingFov(float DefaultFov) const
{
	return TableMenu && TableMenu->FieldOfView != 50 ? TableFov(DefaultFov, TableMenu->FieldOfView) : DefaultFov;
}

void ABackRoomGameMode::ApplyPlayerLens(UCameraComponent* Camera) const
{
	if (!Camera || !TableMenu)
	{
		return;
	}
	FTableMenu& M = *TableMenu;
	if (Stage && M.LensOf.Get() != Stage.Get())
	{
		// The room's grade is the stage's unbound lens; the camera's settings win over it, so they carry its values on.
		M.LensOf = Stage.Get();
		TInlineComponentArray<UPostProcessComponent*> Lenses(Stage.Get());
		for (const UPostProcessComponent* Lens : Lenses)
		{
			if (Lens && Lens->bUnbound)
			{
				M.RoomBias = Lens->Settings.AutoExposureBias;
				M.RoomGrain = Lens->Settings.FilmGrainIntensity;
				break;
			}
		}
	}
	FPostProcessSettings& P = Camera->PostProcessSettings;
	P.bOverride_AutoExposureBias = true;
	P.AutoExposureBias = M.RoomBias + M.BrightnessBias;
	P.bOverride_FilmGrainIntensity = true;
	P.FilmGrainIntensity = M.RoomGrain * M.GrainScale;
	// (Whatever fringing the heart put on the view this frame; off, none of it.)
	P.SceneFringeIntensity *= M.FringeScale;
	P.bOverride_MotionBlurAmount = true;
	P.MotionBlurAmount = M.bMotionBlur ? 0.3f : 0.0f;
}

void ABackRoomGameMode::TestPause(bool bOpen)
{
	if (!TableMenu)
	{
		return;
	}
	if (bOpen)
	{
		OpenPauseMenu();
	}
	else if (TableMenu->Menu.IsOpen())
	{
		TableMenu->Menu.Close(TableMenu->RealTime);
		TableMenu->Resume();
	}
}

void ABackRoomGameMode::TestQuit(bool bToDesktop)
{
	if (TableMenu)
	{
		TableMenu->Describe();
		TableMenu->Quit(bToDesktop);
	}
}

FString ABackRoomGameMode::TestDescribeMenu() const
{
	if (!TableMenu)
	{
		return TEXT("no menu");
	}
	const FTableMenu& M = *TableMenu;
	TableMenu->Describe();
	return FString::Printf(TEXT("%s%s%s page %d | dilation %.4f | first row \"%s\" | note \"%s\" | warning \"%s\" | kept \"%s\" | look %.2f%s hints %d fov %d pace %d"),
		M.Menu.IsOpen() ? TEXT("paused") : TEXT("playing"), M.bFrozen ? TEXT(" frozen") : TEXT(""), M.bQuitting ? TEXT(" quitting") : TEXT(""), static_cast<int32>(M.Menu.Current()),
		UGameplayStatics::GetGlobalTimeDilation(this), UTF8_TO_TCHAR(M.Menu.Info.Status.empty() ? "" : M.Menu.Info.Status.front().second.c_str()),
		UTF8_TO_TCHAR(M.Menu.Info.PauseNote.c_str()), UTF8_TO_TCHAR(M.Menu.Info.QuitWarning.c_str()), UTF8_TO_TCHAR(M.Menu.Info.QuitKept.c_str()), M.LookScale,
		M.bInvertLook ? TEXT(" inverted") : TEXT(""), M.bShowHints ? 1 : 0, M.FieldOfView, TablePace);
}

// ------------------------------------------------------------------ game mode

ABackRoomGameMode::ABackRoomGameMode()
{
	DefaultPawnClass = ABackRoomPawn::StaticClass();
	HUDClass = ABackRoomHUD::StaticClass();
	PlayerControllerClass = ABackRoomPlayerController::StaticClass();
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

ABackRoomGameMode::~ABackRoomGameMode() = default;

void ABackRoomGameMode::EndPlay(const EEndPlayReason::Type Reason)
{
	// The night's last checkpoint lands before the next scene (or the desktop) reads the save.
	if (LiveSaver)
	{
		LiveSaver->Flush();
	}
	// The settings the player was still changing, and the menu off the screen.
	if (TableMenu)
	{
		TableMenu->End();
	}
	// The hero's grooms held for the shoulder camera (SeatEveryone's HoldWorn) don't outlive the table.
	HeroGrooms::ReleaseWorn();
	// (The next table starts from the club's chips; it sets its own.)
	ABackRoomChips::UseChips(false, {});
	Super::EndPlay(Reason);
}

ABackRoomStage* ABackRoomGameMode::FindOrSpawnStage()
{
	// (The player is placed before StartPlay knows tonight's venue: a stage for the wrong one is replaced.)
	if (Stage && Stage->bCardRoom == bLive)
	{
		return Stage;
	}
	Stage = nullptr;
	for (TActorIterator<ABackRoomStage> It(GetWorld()); It; ++It)
	{
		if (It->bCardRoom == bLive)
		{
			Stage = *It;
			return Stage;
		}
		// The map's Back Room makes way for the Embercrest (or the other way round).
		It->Destroy();
	}
	Stage = GetWorld()->SpawnActorDeferred<ABackRoomStage>(ABackRoomStage::StaticClass(), FTransform::Identity);
	Stage->bCardRoom = bLive;
	Stage->FinishSpawning(FTransform::Identity);
	return Stage;
}

void ABackRoomGameMode::RestartPlayer(AController* NewPlayer)
{
	if (!NewPlayer || NewPlayer->IsPendingKillPending())
	{
		return;
	}
	ABackRoomStage* S = FindOrSpawnStage();
	const FVector Eye = S ? S->EyeLocation() : FVector(-98.0, 0.0, 124.0);
	APawn* Pawn = SpawnDefaultPawnAtTransform(NewPlayer, FTransform(FRotator(-14.0f, 0.0f, 0.0f), Eye));
	if (Pawn)
	{
		NewPlayer->SetPawn(Pawn);
		NewPlayer->Possess(Pawn);
		NewPlayer->ClientSetRotation(Pawn->GetActorRotation(), true);
		FinishRestartPlayer(NewPlayer, Pawn->GetActorRotation());
	}
}

void ABackRoomGameMode::StartPlay()
{
	Super::StartPlay();
	ABackRoomChips::ChipUnit = 1;
	// Update 2 (§2.12): the plan's host (ue-auto's, BackRoomAuto.h), before LoadLive reads a checkpoint's host text into it.
	AutoHost = MakeShared<FAutoHost>(*this);
	bLive = LoadLive();
	FindOrSpawnStage();
	const bool bCareer = bLive || LoadCareer();
	// The room's chips (next_segment_spec 6.2 B): the Embercrest's tournament set, with what's in play at the level, or the
	// Wash & Fold's clay.
	if (bLive && Tourney)
	{
		ABackRoomChips::UseChips(true, ss::chipset::InPlay(ss::chipset::Room::Embercrest, Tourney->Levels, Tourney->LevelIndex, Tourney->Spec.BreakEvery));
	}
	else
	{
		ABackRoomChips::UseChips(false, {});
	}
	SeatEveryone();
	// The player's settings, as set in the apartment's menus (the mix, whether to keep playing behind another window, the
	// frame budget, the table's pace, the look and the lens), and the pause menu that changes them here.
	TableMenu = MakeShared<FTableMenu>(*this);
	TableMenu->Begin();
	if (bCareer)
	{
		BeginArrival();
	}
	else
	{
		Phase = EBackRoomPhase::Practice;
		Table->Begin(4.0f);
	}
}

void ABackRoomGameMode::ApplyTablePace()
{
	if (Table)
	{
		Table->ThinkScale = TablePace == 2 ? 0.35f : (TablePace == 1 ? 0.6f : 1.0f);
	}
}

void ABackRoomGameMode::CycleTablePace()
{
	TablePace = (TablePace + 1) % 3;
	ApplyTablePace();
	PaceShownAt = FPlatformTime::Seconds();
	// Kept with the rest of the player's settings (the pause menu and the apartment's show it too).
	if (TableMenu)
	{
		TableMenu->Menu.Settings.TablePace = TablePace;
		TableMenu->SaveSettings();
	}
}

// ------------------------------------------------------------------ the career

bool ABackRoomGameMode::LoadCareer()
{
	const FString BuyIn = UGameplayStatics::ParseOption(OptionsString, TEXT("BuyIn"));
	if (BuyIn.IsEmpty())
	{
		return false;
	}
	std::string CareerText;
	TSharedPtr<ss::SaveData> D = MakeShared<ss::SaveData>();
	if (!CareerSave::LoadText(CareerText) || !ss::SaveData::Parse(CareerText, *D))
	{
		UE_LOG(LogBackRoom, Warning, TEXT("BuyIn given but no career save to play from: practice table."));
		return false;
	}
	const int64 Want = FMath::Clamp<int64>(FCString::Atoi64(*BuyIn), ss::life::GameMinBuyInCents, ss::life::GameMaxBuyInCents);
	const int64 Chips = FMath::Min<int64>(Want, D->BankrollCents) / 100;
	if (Chips * 100 < ss::life::GameMinBuyInCents)
	{
		UE_LOG(LogBackRoom, Warning, TEXT("Bankroll %lld cents can't cover the game: practice table."), static_cast<int64>(D->BankrollCents));
		return false;
	}
	Save = D;
	HeroBuyInChips = Chips;
	StartBankrollCents = D->BankrollCents;
	BoughtInCents = Chips * 100;
	BaseCents = StartBankrollCents - BoughtInCents;
	LeftHomeAt = ss::net::MinutesPerDay * ss::net::NightOneDay + D->ClockMinutes;
	// Down the stairs, across the street in the rain, through the laundromat.
	Minutes = LeftHomeAt + 4.0;
	Energy = static_cast<float>(D->Life.Energy);
	PastNights = D->Life.BackRoomNights;
	PastNetCents = D->Life.BackRoomNetCents;
	bFirstVisit = PastNights == 0;
	ReloadChips = Chips;
	Phase = EBackRoomPhase::Arriving;
	ArrivalDayText = FString::Printf(TEXT("%s   %s"), WeekdayLong[Weekday(FMath::FloorToInt(Minutes / ss::net::MinutesPerDay))], *Clock12(TimeOfDay()));
	UE_LOG(LogBackRoom, Log, TEXT("Career night: bankroll %lld cents, buy-in %lld chips, night %d, energy %.0f, %s"), StartBankrollCents, Chips, PastNights + 1,
		Energy, *ClockLabel());
	return true;
}

FString ABackRoomGameMode::DescribeNight() const
{
	static const TCHAR* Phases[] = {TEXT("practice"), TEXT("arriving"), TEXT("playing"), TEXT("busted"), TEXT("leaving"), TEXT("moving")};
	return FString::Printf(TEXT("%s %s energy %.1f base %lld bought %lld stack %lld hands %d%s%s"), Phases[static_cast<int32>(Phase)], *ClockLabel(), Energy, BaseCents,
		BoughtInCents, Table ? Table->GetHeroStack() : 0, HandsPlayed, bLeaveAsked ? TEXT(" LEAVING") : TEXT(""), bLastHandCalled ? TEXT(" LAST-HAND") : TEXT(""));
}

void ABackRoomGameMode::TestAdvanceClock(float ClockMinutes, float EnergySpent)
{
	Minutes += ClockMinutes;
	Energy = FMath::Clamp(Energy - EnergySpent, 0.0f, 100.0f);
}

double ABackRoomGameMode::TimeOfDay() const
{
	return FMath::Fmod(FMath::Fmod(Minutes, ss::net::MinutesPerDay) + ss::net::MinutesPerDay, ss::net::MinutesPerDay);
}

int32 ABackRoomGameMode::NightWeekday() const
{
	// Past midnight the night still belongs to the day the doors opened.
	int32 Day = FMath::FloorToInt(Minutes / ss::net::MinutesPerDay);
	if (TimeOfDay() < 12.0 * 60.0)
	{
		--Day;
	}
	return Weekday(Day);
}

FString ABackRoomGameMode::ClockLabel() const
{
	return FString(WeekdayShort[Weekday(FMath::FloorToInt(Minutes / ss::net::MinutesPerDay))]) + TEXT("  ") + Clock12(TimeOfDay());
}

void ABackRoomGameMode::SaveCareer(bool bFinal)
{
	if (!Save.IsValid() || !Table)
	{
		return;
	}
	ss::SaveData D = *Save;
	D.BankrollCents = BaseCents + Table->GetHeroStack() * 100;
	// The walk home is a few minutes more.
	D.ClockMinutes = Minutes - ss::net::MinutesPerDay * ss::net::NightOneDay + (bFinal ? 4.0 : 0.0);
	D.Life.Energy = FMath::Clamp(static_cast<double>(Energy), 0.0, 100.0);
	for (const TPair<FString, int32>& R : Table->Reads)
	{
		D.Life.Reads[std::string(TCHAR_TO_UTF8(*R.Key))] = R.Value;
	}
	if (bFinal)
	{
		const int64 Net = D.BankrollCents - StartBankrollCents;
		D.Life.Record(Minutes, "Dee's game", Net, 0);
		D.Life.BackRoomNights += 1;
		D.Life.BackRoomNetCents += Net;
		// The living world hears about the night: Dee's regulars remember it.
		D.NoteBackRoom(Minutes, {"Sal", "Big Lou", "Twitch", "Mei"}, Net);
		// Settled: a second save can't count the night twice.
		*Save = D;
		StartBankrollCents = D.BankrollCents;
		BaseCents = D.BankrollCents - Table->GetHeroStack() * 100;
	}
	CareerSave::SaveNow(D.Serialize());
}

void ABackRoomGameMode::OnTableNote(uint8 Note)
{
	// The hand the player was dealt into, as they saw it (ss::recall): the table card's HANDS page, and for a hand they
	// played (chips in past the blinds, or a showdown), the last-hand card over the hero block for 6 s.
	ss::recall::Record Played;
	if (Table && static_cast<EBackRoomTableNote>(Note) == EBackRoomTableNote::HandEnded && Table->TakeRecall(Played))
	{
		// Played: chips put in by choice (a call, a bet, a raise, all in), or still in it at the end (a showdown, or the
		// blinds walked to them). A hand folded, by the player or for them while they were up, isn't one they played, even
		// when the others showed down.
		bool bChipsIn = false;
		bool bFoldedOut = false;
		for (const ss::recall::Line& L : Played.Lines)
		{
			if (L.Who == "You")
			{
				bFoldedOut = bFoldedOut || L.What == "folds";
				bChipsIn = bChipsIn || (L.What != "folds" && L.What != "checks");
			}
		}
		const bool bPlayed = bChipsIn || (!bFoldedOut && (Played.HeroNet != 0 || !Played.Shown.empty()));
		// Update 2 (§1.3): the hand's end for the plan's books, both rooms (ue-auto's FAutoHost::OnHandEnded: the observed
		// stats, the plan's tilt and counts, a highlight candidate), with the room's note the director hasn't taken yet.
		if (AutoHost)
		{
			AutoHost->OnHandEnded(Played, Table->PeekHandNote());
		}
		RecentHands.push_back(MoveTemp(Played));
		if (RecentHands.size() > 10)
		{
			RecentHands.erase(RecentHands.begin(), RecentHands.begin() + static_cast<std::ptrdiff_t>(RecentHands.size() - 10));
		}
		if (bPlayed && TableHost)
		{
			TableHost->LastHandAt = FPlatformTime::Seconds();
		}
	}
	if (Table && Save.IsValid() && static_cast<EBackRoomTableNote>(Note) == EBackRoomTableNote::HandEnded)
	{
		// A workout's clear head wears off as the night goes on: the table's fitness, again, at each hand's end, and the tilt
		// it scaled at the seating (TiltGain carries the perks and the needs too: only the fitness's part changes).
		const double WasScale = Table->Fitness.TiltGainScale;
		Table->Fitness = ss::body::TableOf(Save->Life.Fit, Minutes);
		if (WasScale > 0.01 && !FMath::IsNearlyEqual(WasScale, Table->Fitness.TiltGainScale))
		{
			Table->TiltGain = static_cast<float>(FMath::Clamp(Table->TiltGain / WasScale * Table->Fitness.TiltGainScale, 0.0, 2.0));
		}
	}
	if (bLive)
	{
		if (TableMenu)
		{
			// (A quit from the pause menu plays out a hand dealt and not yet ended: see FTableMenu::Quit.)
			const EBackRoomTableNote N = static_cast<EBackRoomTableNote>(Note);
			TableMenu->bHandDealt = N == EBackRoomTableNote::HandDealt || (TableMenu->bHandDealt && N != EBackRoomTableNote::HandEnded);
		}
		LiveNote(Note);
		return;
	}
	switch (static_cast<EBackRoomTableNote>(Note))
	{
	case EBackRoomTableNote::HandEnded:
		++HandsPlayed;
		if (IsCareer() && Phase != EBackRoomPhase::Leaving)
		{
			SaveCareer(false);
		}
		break;
	case EBackRoomTableNote::HeroBusted:
		// (Update 2, §2.6.3: felted, the plan stops; the reload is the player's money.)
		if (AutoHost)
		{
			AutoHost->OnHeroBusted();
		}
		if (!IsCareer() || Phase == EBackRoomPhase::Leaving)
		{
			break;
		}
		Phase = EBackRoomPhase::Busted;
		ReloadChips = FMath::Clamp<int64>((FMath::Min<int64>(HeroBuyInChips, BaseCents / 100) / 20) * 20, 40, 200);
		if (bLastHandCalled)
		{
			RequestLeave();
		}
		else
		{
			Table->DealerLine(CanReload() ? TEXT("You want back in, or you calling it a night?") : TEXT("That's all you brought, huh? Go home and sleep, kid."));
		}
		SaveCareer(false);
		break;
	case EBackRoomTableNote::HeroLeft:
		BeginLeaving();
		break;
	}
}

bool ABackRoomGameMode::CanReload() const
{
	return Phase == EBackRoomPhase::Busted && !bLeaveAsked && BaseCents >= ss::life::GameMinBuyInCents;
}

void ABackRoomGameMode::AdjustReload(int32 Steps)
{
	const int64 Max = FMath::Max<int64>(40, FMath::Min<int64>(200, BaseCents / 100));
	ReloadChips = FMath::Clamp<int64>(ReloadChips + Steps * 20, 40, Max);
}

void ABackRoomGameMode::Reload()
{
	if (!CanReload() || !Table)
	{
		return;
	}
	const int64 Chips = FMath::Min<int64>(ReloadChips, BaseCents / 100);
	BaseCents -= Chips * 100;
	BoughtInCents += Chips * 100;
	Phase = EBackRoomPhase::Playing;
	Table->HeroReload(Chips);
	SaveCareer(false);
}

void ABackRoomGameMode::RequestLeave()
{
	if (bLive)
	{
		LiveRequestLeave();
		return;
	}
	if (!IsCareer() || !Table || Phase == EBackRoomPhase::Leaving || Phase == EBackRoomPhase::Arriving)
	{
		return;
	}
	if (!bLeaveAsked)
	{
		bLeaveAsked = true;
		if (Phase == EBackRoomPhase::Playing && !bLastHandCalled && !bSentHome)
		{
			Table->DealerLine(TEXT("Alright. Racking you up after this one."));
		}
	}
	Table->RequestLeave(bLastHandCalled);
}

void ABackRoomGameMode::BeginArrival()
{
	ArrivalT = 0.0f;
	ArrivalBeat = 0;
	if (Table)
	{
		Table->StartRoomTone();
	}
	if (Hero)
	{
		// The body waits in the chair, unseen, until the camera sits down into it.
		Hero->SetActorHiddenInGame(true);
	}
	StartWalkIn(Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(this, 0)));
}

void ABackRoomGameMode::StartWalkIn(ABackRoomPawn* Pawn)
{
	if (ArrivalBeat != 0 || !Pawn)
	{
		return;
	}
	// In from the laundromat's white light, round the open steel door, behind Twitch, to the seat.
	ArrivalBeat = 1;
	ArrivalT = 0.0f;
	const FVector Eye = Stage ? Stage->EyeLocation() : FVector(-91.0, 0.0, 118.0);
	// Back after the game closed: a few steps up the aisle to the chair, not the whole room.
	const bool bShort = bLive && bBackInRoom && Stage;
	const TArray<FVector> Path = bShort ? Stage->CardRoomMoveIn(Eye, Stage->GetRoomAnchor()) : bLive && Stage ? Stage->CardRoomWalkIn(Eye) : TArray<FVector>{FVector(166.0, 540.0, 166.0), FVector(174.0, 360.0, 166.0), FVector(194.0, 258.0, 165.0), FVector(168.0, 176.0, 165.0),
		FVector(40.0, 210.0, 165.0), FVector(-90.0, 228.0, 164.0), FVector(-196.0, 168.0, 163.0), FVector(-206.0, 50.0, 161.0), FVector(-160.0, 6.0, 148.0), Eye};
	TWeakObjectPtr<ABackRoomGameMode> Self = this;
	Pawn->PlayWalk(Path, bShort ? 3.6f : 9.5f, false, [Self]() {
		if (ABackRoomGameMode* GM = Self.Get())
		{
			GM->Phase = EBackRoomPhase::Playing;
			GM->Table->Begin(1.4f);
		}
	});
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 1.8f, FLinearColor::Black, true, false);
		}
	}
}

void ABackRoomGameMode::BeginLeaving()
{
	if (!IsCareer() || Phase == EBackRoomPhase::Leaving || !Table)
	{
		return;
	}
	const int64 Stack = Table->GetHeroStack();
	bBustedOut = Stack <= 0;
	NetCents = BaseCents + Stack * 100 - StartBankrollCents;
	const int64 BoughtIn = BoughtInCents;
	// Which reads became yours tonight (seen twice and confirmed by the cards).
	TArray<FString> Learned;
	for (const TPair<FString, int32>& R : Table->Reads)
	{
		const auto Was = Save->Life.Reads.find(std::string(TCHAR_TO_UTF8(*R.Key)));
		const int32 Before = Was == Save->Life.Reads.end() ? 0 : Was->second;
		if (R.Value >= 2 && Before < 2)
		{
			FString Who, TellName;
			R.Key.Split(TEXT("/"), &Who, &TellName);
			const int64 Value = StaticEnum<EBackRoomTell>()->GetValueByNameString(TellName);
			Learned.Add(FString::Printf(TEXT("Read learned   %s: %s"), *Who, *ABackRoomTable::TellPhrase(static_cast<uint8>(FMath::Max<int64>(Value, 0)))));
		}
	}
	SaveCareer(true);
	Phase = EBackRoomPhase::Leaving;
	LeaveT = 0.0f;
	Summary.Reset();
	Summary.Add(FString::Printf(TEXT("THE %s GAME"), *FString(WeekdayLong[NightWeekday()]).ToUpper()));
	const int32 Sat = FMath::Max(0, FMath::FloorToInt(Minutes - LeftHomeAt - 4.0));
	const FString Stayed = Sat >= 60 ? FString::Printf(TEXT("%dh %02dm"), Sat / 60, Sat % 60) : FString::Printf(TEXT("%d min"), Sat);
	Summary.Add(FString::Printf(TEXT("%s   ·   %d %s   ·   %s at the table"), *Clock12(TimeOfDay()), HandsPlayed, HandsPlayed == 1 ? TEXT("hand") : TEXT("hands"), *Stayed));
	Summary.Add(FString::Printf(TEXT("Bought in   %s"), *Dollars(BoughtIn / 100)));
	Summary.Add(FString::Printf(TEXT("Cashed out   %s"), *Dollars(Stack)));
	Summary.Add(FString::Printf(TEXT("Net   %s%s"), NetCents >= 0 ? TEXT("+") : TEXT("-"), *Dollars(NetCents / 100)));
	Summary.Append(Learned);
	Table->DealerLine(GoodbyeLine());
	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
	UE_LOG(LogBackRoom, Log, TEXT("Leaving Dee's game: net %lld cents, %d hands, %d reads learned%s%s"), NetCents, HandsPlayed, Learned.Num(),
		bBustedOut ? TEXT(", busted") : TEXT(""), bClosed ? TEXT(", closed") : TEXT(""));
}

void ABackRoomGameMode::GoHome()
{
	if (bGoingHome)
	{
		return;
	}
	bGoingHome = true;
	FString Night = FString::Printf(TEXT("Net=%lld?Learned=%d?From=%.0f"), NetCents, Table ? Table->LearnedThisNight : 0, LeftHomeAt);
	if (bBustedOut)
	{
		Night += TEXT("?Busted");
	}
	if (bClosed)
	{
		Night += TEXT("?Closed");
	}
	// Walked in from the street, through the Wash & Fold's back door: back out the same way, onto Fifth at the laundromat
	// (the street catches up the calendar from From, as the desk does). From the Burner app, home to the desk.
	const bool bStreet = UGameplayStatics::HasOption(OptionsString, TEXT("FromStreet"));
	UGameplayStatics::OpenLevel(this, FName(bStreet ? TEXT("Street") : TEXT("NightOne")), true, (bStreet ? TEXT("Start=washfold?Night?") : TEXT("Home?")) + Night);
}

FString ABackRoomGameMode::ArrivalLine() const
{
	const double Tod = TimeOfDay();
	const bool bLate = Tod >= 60.0 && Tod < 12.0 * 60.0;
	const ss::life::State& L = Save->Life;
	const double ToRent = L.RentDeadline - Minutes;
	const bool bRentSoon = (L.RentStage == ss::life::Rent::Due || L.RentStage == ss::life::Rent::FinalNotice) && ToRent > 0.0 && ToRent < 2.0 * ss::net::MinutesPerDay;
	// Locked out: the kid sleeps on her couch, and she knows it.
	const bool bEvicted = L.RentStage == ss::life::Rent::Evicted;
	if (bFirstVisit)
	{
		return bEvicted ? TEXT("You sleep on my couch, you play at my table. Forty to two hundred, no phones, no crying.")
						: TEXT("There's the kid from across the street. Sit, sit. Forty to two hundred, no phones, no crying.");
	}
	if (bEvicted)
	{
		return bLate ? TEXT("Couch not comfy enough? Sit down, kid. Let's win you your key back.") : TEXT("Couch treating you okay? Sit. Let's win you your key back.");
	}
	if (bLate)
	{
		return TEXT("Little late, kid. Sit down before Lou eats your chair.");
	}
	if (bRentSoon)
	{
		return TEXT("Heard rent's due. Don't play scared, kid. Scared money don't make money.");
	}
	if (PastNetCents >= 10000)
	{
		return TEXT("Look who's back. Lou's been practicing his poker face. Didn't help.");
	}
	if (PastNetCents < -5000)
	{
		return TEXT("Back for more. I like that. Sit.");
	}
	return TEXT("Kid. Your seat's still warm.");
}

FString ABackRoomGameMode::GreetingLine(FString& Who) const
{
	if (bFirstVisit)
	{
		Who = TEXT("Big Lou");
		return TEXT("Fresh money! Sit down, sit down!");
	}
	if (PastNetCents >= 10000)
	{
		Who = TEXT("Sal");
		return TEXT("You again. Lucky kid.");
	}
	if (PastNetCents < 0)
	{
		Who = TEXT("Twitch");
		return TEXT("Yo, it's my ATM! Sit down, man.");
	}
	Who = TEXT("Mei");
	return TEXT("Evening.");
}

FString ABackRoomGameMode::GoodbyeLine() const
{
	const int32 Day = NightWeekday();
	const TCHAR* Next = Day == 1 ? TEXT("Thursday") : (Day == 3 ? TEXT("Saturday") : TEXT("Tuesday"));
	// Locked out, "home" is her couch.
	const bool bEvicted = Save.IsValid() && Save->Life.RentStage == ss::life::Rent::Evicted;
	if (bClosed)
	{
		return TEXT("That's the game, folks. Dryers go off at five. Get home safe.");
	}
	if (bSentHome)
	{
		return bEvicted ? FString::Printf(TEXT("Go sleep, kid. Couch is made up. %s, nine o'clock."), Next) : FString::Printf(TEXT("Go sleep, kid. %s, nine o'clock."), Next);
	}
	if (bBustedOut)
	{
		return bEvicted ? FString::Printf(TEXT("Couch is made up, kid. Sleep. Game's back on %s."), Next) : FString::Printf(TEXT("Go home, kid. Sleep. Game's back on %s."), Next);
	}
	if (NetCents >= 5000)
	{
		if (bEvicted)
		{
			// Enough tonight to move back in (what's in the bankroll now), or a start on it.
			const bool bKeyMoney = StartBankrollCents + NetCents >= Save->Life.RentDueCents;
			return bKeyMoney ? FString(TEXT("That's your key back. Landlord first, kid. Then Lou."))
							 : FString(TEXT("That's a start on your key. Don't you dare spend it on Lou."));
		}
		return FString::Printf(TEXT("Taking my regulars' money home? Come back %s so they can win it back."), Next);
	}
	if (NetCents < 0)
	{
		return FString::Printf(TEXT("Night, kid. Watch more, play less. %s, nine o'clock."), Next);
	}
	return FString::Printf(TEXT("Night, kid. %s, nine o'clock."), Next);
}

void ABackRoomGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	FrameBudget.Tick(DeltaSeconds);
	const float RealDt = FMath::Min(static_cast<float>(FApp::GetDeltaTime()), 0.1f);
	// The pause menu drawn (and fading out), the settings kept, a quit under way. While it's up the night stands still:
	// neither its clock nor the tournament's moves (LIVE_TOURNAMENTS.md, Time: pausing isn't sitting out).
	if (TableMenu)
	{
		TableMenu->Tick(RealDt);
		if (TableMenu->bFrozen || TableMenu->bLeft)
		{
			return;
		}
	}
	if (!IsCareer() || !Table)
	{
		return;
	}
	// Update 2 (§2.12): the plan's own frame, in both rooms (ue-auto's FAutoHost::Tick); the pause menu above freezes it with
	// the night.
	if (AutoHost)
	{
		AutoHost->Tick(RealDt);
	}
	if (bLive)
	{
		LiveTick(RealDt);
		return;
	}
	ABackRoomPawn* Pawn = Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	TWeakObjectPtr<ABackRoomGameMode> Self = this;

	// The room turns to look at whoever is walking through it.
	auto Watch = [this, Pawn](float Dt) {
		AttentionTick -= Dt;
		if (!Pawn || AttentionTick > 0.0f)
		{
			return;
		}
		AttentionTick = 0.45f;
		for (ABackRoomPlayer* O : Opponents)
		{
			O->OnOtherAction(Pawn->GetEye(), 0.25f, false);
		}
		if (Dealer)
		{
			Dealer->OnOtherAction(Pawn->GetEye(), 0.25f, false);
		}
	};

	ArrivalT += RealDt;
	if (Phase == EBackRoomPhase::Arriving)
	{
		StartWalkIn(Pawn);
		if (ArrivalBeat == 1 && ArrivalT > 2.6f)
		{
			ArrivalBeat = 2;
			Table->DealerLine(ArrivalLine());
		}
		if (ArrivalBeat == 2 && ArrivalT > 5.4f)
		{
			ArrivalBeat = 3;
			FString Who;
			const FString Line = GreetingLine(Who);
			for (ABackRoomPlayer* O : Opponents)
			{
				if (O && O->Persona.Name == Who)
				{
					O->Say(Line);
				}
			}
		}
		Watch(RealDt);
	}

	// The night goes on: the clock, and what it takes out of you. (Update 2, §1.1: while the plan holds the seat the night
	// moves by the hand, never by real time, so watching it quicker never packs more hands into a night: ue-auto's
	// FAutoHost::DeeMinutes, which hands back the real-time minutes whenever the player decides.)
	if (Phase == EBackRoomPhase::Playing || Phase == EBackRoomPhase::Busted)
	{
		const double RealMinutes = RealDt * ClockRate / 60.0;
		const double Elapsed = AutoHost ? AutoHost->DeeMinutes(RealMinutes) : RealMinutes;
		Minutes += Elapsed;
		Energy = FMath::Max(0.0f, Energy - static_cast<float>(Elapsed / 60.0) * EnergyPerHour);
		const double Tod = TimeOfDay();
		if (!bLastHandCalled && Tod >= LastHandAt && Tod < 12.0 * 60.0)
		{
			bLastHandCalled = true;
			bClosed = true;
			Table->DealerLine(TEXT("Last hand, everybody. Dryers go off at five."));
			RequestLeave();
		}
		if (!bSentHome && !bLeaveAsked && Energy <= 4.0f)
		{
			bSentHome = true;
			Table->DealerLine(TEXT("Kid, you're falling asleep at my table. This one, then go home."));
			RequestLeave();
		}
		// Tired: the lids get heavy, more often and for longer the emptier you are.
		if (Energy < 22.0f)
		{
			const float Tired = 1.0f - Energy / 22.0f;
			NextDroop -= RealDt;
			if (DroopT < 0.0f && NextDroop <= 0.0f)
			{
				DroopT = 0.0f;
				DroopLength = FMath::FRandRange(0.5f, 0.8f) + 0.9f * Tired;
				DroopDepth = FMath::Min(1.0f, FMath::FRandRange(0.4f, 0.7f) + 0.3f * Tired);
				NextDroop = FMath::FRandRange(9.0f, 20.0f) * (1.2f - 0.7f * Tired);
			}
		}
	}
	if (DroopT >= 0.0f)
	{
		// Closing slowly, snapping open.
		DroopT += RealDt;
		const float U = DroopT / DroopLength;
		Eyelids = DroopDepth * (U < 0.7f ? Smooth(0.0f, 0.7f, U) : 1.0f - Smooth(0.7f, 1.0f, U));
		if (U >= 1.0f)
		{
			DroopT = -1.0f;
			Eyelids = 0.0f;
		}
	}

	// Racked up: Dee says goodnight, then up and out the way you came, into the white light.
	if (Phase == EBackRoomPhase::Leaving)
	{
		LeaveT += RealDt;
		Eyelids = 0.0f;
		if (!bWalkingOut && LeaveT > 2.4f)
		{
			bWalkingOut = true;
			if (Pawn)
			{
				const FVector Eye = Pawn->GetEye();
				const TArray<FVector> Path = {Eye, Eye + FVector(-34.0, 16.0, 44.0), FVector(-196.0, 84.0, 166.0), FVector(-150.0, 225.0, 166.0),
					FVector(30.0, 214.0, 166.0), FVector(168.0, 180.0, 166.0), FVector(194.0, 262.0, 166.0), FVector(172.0, 380.0, 166.0), FVector(166.0, 560.0, 166.0)};
				Pawn->PlayWalk(Path, 9.5f, true, [Self]() {
					if (ABackRoomGameMode* GM = Self.Get())
					{
						GM->GoHome();
					}
				});
			}
			else
			{
				GoHome();
			}
		}
		if (bWalkingOut && !bFadingOut && LeaveT > 2.4f + 7.6f && PC && PC->PlayerCameraManager)
		{
			bFadingOut = true;
			PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, 1.6f, FLinearColor::Black, true, true);
		}
		if (Pawn && Pawn->IsWalking())
		{
			Watch(RealDt);
		}
	}
}

// Who wears what, and in which body, is the cast's business, shared with the street and the Embercrest (CastLooks.h): these
// forward there.
FBackRoomPersona ABackRoomGameMode::PersonaFor(const FString& Name)
{
	return CastLooks::PersonaFor(Name);
}

FString ABackRoomGameMode::CastAssetFor(const FString& Name)
{
	return CastLooks::CastAssetFor(Name);
}

ABackRoomPlayer* ABackRoomGameMode::SpawnPerson(const FString& CastName, const FTransform& At, EBackRoomRole AtTableAs, const FBackRoomPersona& Persona)
{
	ABackRoomPlayer* P = GetWorld()->SpawnActorDeferred<ABackRoomPlayer>(ABackRoomPlayer::StaticClass(), At);
	P->Persona = Persona;
	P->SeatRole = AtTableAs;
	P->MetaHumanClass = CastBlueprint(*CastName);
	P->FinishSpawning(At);
	return P;
}

void ABackRoomGameMode::SeatEveryone()
{
	// Players placed in the map (for look development) stay out of the game.
	for (TActorIterator<ABackRoomPlayer> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}

	Table = GetWorld()->SpawnActor<ABackRoomTable>(ABackRoomTable::StaticClass(), FTransform::Identity);
	// The night's rules: in a career a bust is yours to deal with, Dee only teaches the first time,
	// and what you learned of the regulars on other nights is still yours.
	Table->bHeroAutoReload = !IsCareer();
	Table->bFirstVisit = !IsCareer() || bFirstVisit;
	if (Save.IsValid())
	{
		for (const auto& R : Save->Life.Reads)
		{
			Table->Reads.Add(FString(UTF8_TO_TCHAR(R.first.c_str())), R.second);
		}
		const ss::hero::Perks HeroPerks = ss::hero::PerksOf(Save->Person);
		// A coach's part (next_segment_spec 2.11): information and composure, never decisions. What the save's coaching
		// notes make of the background's read weight (Dee's morning-after: one more for a week) and tilt factor (Finch's
		// sessions), the same as online.
		Table->ReadWeight = FMath::Max(1, static_cast<int32>(ss::coach::ReadWeightNow(Save->Life.Study, HeroPerks.ReadWeight, Minutes)));
		const double CoachedTilt = ss::coach::TiltGainNow(Save->Life.Study, HeroPerks.TiltGain, Minutes);
		// How hard a painful loss tilts you: the Bouncer's thick skin takes some of it, and (as online) starving or parched
		// a bad beat lands a fifth harder.
		const double Needs = Save->Life.Hunger >= 85.0 || Save->Life.Thirst >= 85.0 ? 1.2 : 1.0;
		// IronHouse's fitness at the table (spec 7.5.4, body::TableOf): a fitter heart rests lower and settles sooner, steadies
		// further on a breath for less Focus, refills Focus quicker; a clear head after a workout tilts less.
		Table->Fitness = ss::body::TableOf(Save->Life.Fit, Minutes);
		Table->TiltGain = static_cast<float>(FMath::Clamp(CoachedTilt * Needs * Table->Fitness.TiltGainScale, 0.0, 2.0));
		Table->Composure.Bpm = static_cast<float>(Table->Fitness.RestBpm) + 2.0f;
		Table->Composure.Target = Table->Composure.Bpm;
		// Each night at the table, the pressure gets to you a little less.
		Table->Composure.Sensitivity = FMath::Clamp(1.0f - 0.06f * PastNights, 0.65f, 1.0f);
	}
	TWeakObjectPtr<ABackRoomGameMode> Self = this;
	Table->OnNote = [Self](EBackRoomTableNote Note) {
		if (ABackRoomGameMode* GM = Self.Get())
		{
			GM->OnTableNote(static_cast<uint8>(Note));
		}
	};
	// Dee deals her Tuesday game and the Embercrest's Sunday; the card room's other days have dealers of their own, the
	// same one every week (a vest and a bow tie on one of the room's faces).
	FString DealerBody = TEXT("Dee");
	FBackRoomPersona DealerPersona = PersonaFor(TEXT("Dee"));
	if (bLive && ((LiveDay % 7) + 7) % 7 != 6)
	{
		// Monday's dealer is Nell: the city's one Rosa is IronHouse's trainer (spec 6.1).
		static const TCHAR* Names[6] = {TEXT("Nell"), TEXT("Hank"), TEXT("Lupe"), TEXT("Vic"), TEXT("June"), TEXT("Tomas")};
		static const TCHAR* Bodies[6] = {TEXT("ExtraE"), TEXT("ExtraL"), TEXT("ExtraK"), TEXT("ExtraJ"), TEXT("ExtraG"), TEXT("ExtraF")};
		const int32 Day = ((LiveDay % 7) + 7) % 7;
		DealerBody = Bodies[Day];
		DealerPersona = PersonaFor(Names[Day]);
		DealerPersona.Shirt = FLinearColor(FColor(0x14, 0x14, 0x18));
		DealerPersona.Chatter = 0.2f;
		// The house dresses its dealers: a black shirt, nothing on the head.
		DealerPersona.ShirtPrint = 0;
		DealerPersona.ShirtGraphic = 0;
		DealerPersona.Headwear = 0;
		DealerPersona.Pants = FLinearColor(FColor(0x12, 0x12, 0x14));
	}
	Dealer = SpawnPerson(DealerBody, ABackRoomStage::SeatTransform(4), EBackRoomRole::Dealer, DealerPersona);
	// The player, as the character creator made them and the closet dresses them (as the street does, spec 7.5.4): the hero
	// MetaHuman nearest the look (body type, then skin tone in three bands; the default one until those are built), the
	// jacket's color on the shirt (a flannel's plaid in it; with no jacket the Top's own color, print or graphic) over the
	// trousers' tone, the hat and the glasses in theirs, the hairstyle, facial hair and brows from the face's groom library
	// (BackRoomPlayerView.cpp; as far as the MetaHuman's own grooms go without one), in the hair's color. Seen over the
	// shoulder; in first person, the hands' sleeves and the shadows. A career from before the creator sits down as the default.
	FString HeroBody = TEXT("Hero");
	FBackRoomPersona HeroPersona = PersonaFor(TEXT("You"));
	if (Save.IsValid() && Save->Person.Created)
	{
		const ss::hero::Look& L = Save->Person.Appearance;
		const FString Nearest = CastLooks::HeroFaceFor(L).ToString();
		if (!CastBlueprint(*Nearest).IsNull())
		{
			HeroBody = Nearest;
		}
		const FCastWear W = AShortStackCharacter::HeroWear(L, Save->Person.Age, Save->Life.Closet.Seeded ? &Save->Life.Closet : nullptr);
		HeroPersona.Shirt = W.Shirt;
		HeroPersona.Pants = W.Pants;
		HeroPersona.ShirtPrint = W.Print;
		HeroPersona.ShirtGraphic = W.Graphic;
		HeroPersona.ShirtB = W.InkA;
		HeroPersona.ShirtC = W.InkB;
		HeroPersona.ShirtD = W.InkC;
		// The hero's pieces at the table are the street's (ABackRoomPlayer::ApplyWear's creator indices): the beanie, the cap
		// (forwards, backwards), the bucket hat; round, square, wire, shades (the aviators wear as shades there). The trilby has no
		// creator index there yet: bare-headed at the table until ApplyWear reads the street's one set.
		using ss::wardrobe::Cut;
		HeroPersona.HeroHat = W.Hat == Cut::Beanie ? 1 : W.Hat == Cut::BallCap ? 2 : W.Hat == Cut::CapBack ? 3 : W.Hat == Cut::Bucket ? 4 : 0;
		HeroPersona.HeroGlasses =
			W.Eyes == Cut::Round ? 1 : W.Eyes == Cut::Square ? 2 : W.Eyes == Cut::Wire ? 3 : (W.Eyes == Cut::Shades || W.Eyes == Cut::Aviators) ? 4 : 0;
		HeroPersona.HeroHairStyle = FMath::Max(0, L.Hair);
		HeroPersona.HeroFacialHair = FMath::Max(0, L.FacialHair);
		HeroPersona.WearColor = W.HatColor;
		HeroPersona.FrameColor = W.FrameColor;
		// The sponsors' patches the closet sewed on, as the street shows them (next_segment_spec 3.10; ue-wear's ApplyWear
		// puts them on): chest, sleeve, cap.
		HeroPersona.PatchChest = W.Patches[0];
		HeroPersona.PatchSleeve = W.Patches[1];
		HeroPersona.PatchCap = W.Patches[2];
		HeroHairShade(L.HairColor, HeroPersona);
		// The look the hero's head reads its grooms from (brows included), with the hat as worn; its grooms start loading now
		// and are held, so going over the shoulder (V) finds them in memory.
		ss::hero::Look Worn = L;
		Worn.Hat = HeroPersona.HeroHat;
		CastLooks::BindHeroLook(this, Worn);
		HeroGrooms::HoldWorn(HeroGrooms::Resolve(Worn, FName(*HeroBody)));
	}
	Hero = SpawnPerson(HeroBody, ABackRoomStage::SeatTransform(0), EBackRoomRole::Hero, HeroPersona);
	Table->SetDealer(Dealer);
	Table->AddPlayer(Hero, 0, ss::Archetype::Tag, HeroBuyInChips);

	if (bLive)
	{
		// The Embercrest: the tournament seats the table (BackRoomLive.cpp).
		SeatLive();
	}
	else
	{
		// Dee's regulars, in their usual seats.
		struct FRegular
		{
			const TCHAR* Name;
			int32 Seat;
			ss::Archetype Style;
			int64 BuyIn;
		};
		const FRegular Regulars[] = {
			{TEXT("Sal"), 3, ss::Archetype::Nit, 240},
			{TEXT("Big Lou"), 5, ss::Archetype::Station, 300},
			{TEXT("Twitch"), 2, ss::Archetype::Maniac, 160},
			{TEXT("Mei"), 6, ss::Archetype::Reg, 220},
		};
		for (const FRegular& R : Regulars)
		{
			ABackRoomPlayer* P = SpawnPerson(CastAssetFor(R.Name), ABackRoomStage::SeatTransform(R.Seat), EBackRoomRole::Player, PersonaFor(R.Name));
			Opponents.Add(P);
			Table->AddPlayer(P, R.Seat, R.Style, R.BuyIn);
		}
	}

	// Everyone knows where the others sit: gaze targets at head height.
	ABackRoomStage* S = FindOrSpawnStage();
	const FVector HeroEye = S ? S->EyeLocation() : FVector(-98.0, 0.0, 124.0);
	TArray<ABackRoomPlayer*> Everyone = {Dealer, Hero};
	for (ABackRoomPlayer* O : Opponents)
	{
		Everyone.Add(O);
	}
	for (ABackRoomPlayer* P : Everyone)
	{
		P->HeroEyes = HeroEye;
		P->PotAt = FVector(10.0, 0.0, ABackRoomStage::FeltZ);
		P->DealerAt = ABackRoomStage::SeatTransform(4).TransformPosition(FVector(-14.0, 0.0, 112.0));
		P->OthersAt.Reset();
		for (ABackRoomPlayer* Other : Everyone)
		{
			if (Other != P && Other != Hero)
			{
				P->OthersAt.Add(Other->GetActorTransform().TransformPosition(FVector(-14.0, 0.0, 112.0)));
			}
		}
	}
}
