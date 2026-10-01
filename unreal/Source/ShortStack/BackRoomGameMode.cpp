#include "BackRoomGameMode.h"

#include "BackRoomCard.h"
#include "BackRoomPlayer.h"
#include "BackRoomStage.h"
#include "BackRoomTable.h"
#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/PackageName.h"
#include "ShortStack/AI/Profiles.h"

namespace BackRoomGameDetail
{
float Ease(float X, float Target, float Rate, float Dt)
{
	return FMath::Lerp(X, Target, 1.0f - FMath::Exp(-Rate * Dt));
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

FString CardText(int32 Card)
{
	static const TCHAR* Ranks[13] = {TEXT("2"), TEXT("3"), TEXT("4"), TEXT("5"), TEXT("6"), TEXT("7"), TEXT("8"), TEXT("9"), TEXT("10"), TEXT("J"), TEXT("Q"), TEXT("K"), TEXT("A")};
	static const TCHAR* Suits[4] = {TEXT("♣"), TEXT("♦"), TEXT("♥"), TEXT("♠")};
	return FString(Ranks[(Card >> 2) % 13]) + Suits[Card & 3];
}
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
}

void ABackRoomPawn::EndPlay(const EEndPlayReason::Type Reason)
{
	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
	Super::EndPlay(Reason);
}

ABackRoomTable* ABackRoomPawn::GetTable() const
{
	const ABackRoomGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ABackRoomGameMode>() : nullptr;
	return GM ? GM->Table.Get() : nullptr;
}

void ABackRoomPawn::HandleInput(float RealDt)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	ABackRoomTable* Table = GetTable();
	ABackRoomPlayer* Me = Table ? Table->GetHeroPlayer() : nullptr;
	if (!PC)
	{
		return;
	}
	// Peek: hold to lift the corners.
	const bool bPeek = bTestPeek || PC->IsInputKeyDown(EKeys::SpaceBar) || PC->IsInputKeyDown(EKeys::LeftMouseButton);
	PeekBlend = Ease(PeekBlend, bPeek ? 1.0f : 0.0f, 6.0f, RealDt);
	if (Me)
	{
		Me->SetHeroPeek(bPeek);
	}
	if (bPeek && PeekBlend > 0.85f && Table)
	{
		Table->HeroPeeked();
	}

	// Focus: hold to study a face; it drains, and comes back slowly.
	const bool bFocusing = (bTestFocus || PC->IsInputKeyDown(EKeys::RightMouseButton)) && FocusLeft > 0.02f && !bPeek;
	FocusLeft = FMath::Clamp(FocusLeft + (bFocusing ? -RealDt / 8.0f : RealDt / 14.0f), 0.0f, 1.0f);
	Focus = Ease(Focus, bFocusing ? 1.0f : 0.0f, 5.0f, RealDt);
	UGameplayStatics::SetGlobalTimeDilation(this, FMath::Lerp(1.0f, 0.45f, Focus));
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

	// Actions.
	if (Table)
	{
		if (PC->WasInputKeyJustPressed(EKeys::F))
		{
			Table->HeroFold();
		}
		if (PC->WasInputKeyJustPressed(EKeys::C))
		{
			Table->HeroCheckCall();
		}
		if (PC->WasInputKeyJustPressed(EKeys::R))
		{
			Table->HeroRaise();
		}
		if (PC->WasInputKeyJustPressed(EKeys::A))
		{
			Table->HeroAllIn();
		}
		if (PC->WasInputKeyJustPressed(EKeys::MouseScrollUp) || PC->WasInputKeyJustPressed(EKeys::Up))
		{
			Table->HeroAdjustRaise(1);
		}
		if (PC->WasInputKeyJustPressed(EKeys::MouseScrollDown) || PC->WasInputKeyJustPressed(EKeys::Down))
		{
			Table->HeroAdjustRaise(-1);
		}
	}
}

void ABackRoomPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Focus slows the world; the head and the meter run on real time.
	const float RealDt = FMath::Min(static_cast<float>(FApp::GetDeltaTime()), 0.1f);
	Time += RealDt;
	ABackRoomTable* Table = GetTable();
	ABackRoomPlayer* Me = Table ? Table->GetHeroPlayer() : nullptr;
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		float Dx = 0.0f, Dy = 0.0f;
		PC->GetInputMouseDelta(Dx, Dy);
		const float Sens = 0.9f * (1.0f - 0.55f * Focus);
		Yaw = FMath::Clamp(Yaw + Dx * Sens, -MaxYaw, MaxYaw);
		Pitch = FMath::Clamp(Pitch + Dy * Sens, MinPitch, MaxPitch);
	}
	HandleInput(RealDt);

	FVector Eye = Me ? Me->GetEyes() + FVector(1.5, 0.0, 0.0) : Seat;
	FRotator Want(Pitch, Yaw, 0.0f);
	FVector FocusAt = Eye + Want.Vector() * 120.0;
	float PeekLit = 0.0f;
	if (Me && Me->Hole.Num() == 2 && Me->Hole[0] && Me->Hole[1] && PeekBlend > 0.01f)
	{
		// Peeking: you hunch down over the rail until the lifted corners face you, a hand's width away.
		const FVector Cards = (Me->Hole[0]->GetActorLocation() + Me->Hole[1]->GetActorLocation()) * 0.5;
		const FVector Near = Cards - FVector(3.5, 0.0, -1.0);
		Eye = FMath::Lerp(Eye, Cards + FVector(-30.0, 0.0, 18.0), 0.9 * PeekBlend);
		const FRotator ToCards = (Near - Eye).Rotation();
		Want = FMath::Lerp(Want, FRotator(ToCards.Pitch, ToCards.Yaw, 0.0f), 0.9f * PeekBlend);
		FocusAt = Near;
		PeekLight->SetWorldLocation(Near + FVector(-8.0, 3.0, 5.0));
		PeekLit = PeekBlend;
	}
	PeekLight->SetIntensity(3.5f * PeekLit);
	if (ABackRoomPlayer* S = Studying.Get())
	{
		// Focus pulls the view onto the face.
		const FRotator ToFace = (S->GetEyes() - Eye).Rotation();
		Want = FMath::Lerp(Want, FRotator(ToFace.Pitch, ToFace.Yaw, 0.0f), 0.4f * Focus);
		FocusAt = FMath::Lerp(FocusAt, S->GetEyes(), static_cast<double>(Focus));
	}
	Smoothed = FMath::RInterpTo(Smoothed, Want, RealDt, 14.0f);
	if (Me)
	{
		SetActorLocationAndRotation(Eye, Smoothed);
		Me->SetHeroLook(Eye + Smoothed.Vector() * 200.0);
	}
	else
	{
		// No body yet: a head that breathes.
		const float Breath = FMath::Sin(Time * 1.35f);
		SetActorLocationAndRotation(Seat + FVector(0.25 * Breath, 0.0, 0.35 * Breath), Smoothed + FRotator(0.2f * Breath, 0.0f, 0.15f * FMath::Sin(Time * 0.37f)));
	}
	if (!Studying.IsValid() && PeekBlend < 0.01f)
	{
		// Without a target the eyes rest where the view meets the felt (or a couple of meters out).
		const FVector Dir = Smoothed.Vector();
		const double Down = -Dir.Z;
		const double ToFelt = Down > 0.05 ? (Eye.Z - ABackRoomStage::FeltZ) / Down : 250.0;
		FocusAt = Eye + Dir * FMath::Clamp(ToFelt, 40.0, 250.0);
	}

	// The lens: Focus narrows it and opens the aperture.
	Camera->SetFieldOfView(FMath::Lerp(FMath::Lerp(72.0f, 58.0f, PeekBlend), 34.0f, Focus));
	FPostProcessSettings& P = Camera->PostProcessSettings;
	P.DepthOfFieldFocalDistance = FMath::Max(10.0f, static_cast<float>(FVector::Dist(Eye, FocusAt)));
	P.DepthOfFieldFstop = FMath::Lerp(FMath::Lerp(4.0f, 2.8f, PeekBlend), 1.4f, Focus);
	P.VignetteIntensity = 0.45f + 0.5f * Focus;

	// The opponents feel being looked at: more so through Focus.
	if (Table)
	{
		const FVector Fwd = Camera->GetForwardVector();
		const float Lo = FMath::Cos(FMath::DegreesToRadians(9.0f));
		const float Hi = FMath::Cos(FMath::DegreesToRadians(2.5f));
		for (ABackRoomPlayer* O : Table->GetOpponents())
		{
			const float C = FVector::DotProduct(Fwd, (O->GetEyes() - Eye).GetSafeNormal());
			O->SetStudied(FMath::Clamp((C - Lo) / (Hi - Lo), 0.0f, 1.0f) * (0.6f + 0.4f * Focus));
		}
	}
}

// ------------------------------------------------------------------ HUD

void ABackRoomHUD::DrawHUD()
{
	Super::DrawHUD();
	Clock += FMath::Min(static_cast<float>(FApp::GetDeltaTime()), 0.1f);
	const ABackRoomGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ABackRoomGameMode>() : nullptr;
	ABackRoomTable* Table = GM ? GM->Table.Get() : nullptr;
	if (!Table || !Canvas)
	{
		return;
	}
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	const float S = H / 1080.0f;
	UFont* Big = GEngine->GetLargeFont();
	UFont* Small = GEngine->GetMediumFont();
	const FLinearColor Paper(0.93f, 0.9f, 0.82f, 0.92f);
	const FLinearColor Dim(0.93f, 0.9f, 0.82f, 0.55f);
	const FLinearColor Warm(1.0f, 0.78f, 0.45f, 0.95f);

	auto Text = [&](const FString& T, const FLinearColor& Color, float X, float Y, UFont* Font, float Scale, bool bCenter) {
		float Tw = 0.0f, Th = 0.0f;
		GetTextSize(T, Tw, Th, Font, Scale * S);
		const float Px = bCenter ? X - Tw * 0.5f : X;
		DrawText(T, FLinearColor(0.0f, 0.0f, 0.0f, Color.A * 0.6f), Px + 2.0f * S, Y + 2.0f * S, Font, Scale * S);
		DrawText(T, Color, Px, Y, Font, Scale * S);
		return Tw;
	};

	// New table talk joins the subtitles; the newest three stay a few seconds.
	for (const FBackRoomLine& L : Table->TakeLines())
	{
		Subtitles.Add({L.Speaker, L.Text, Clock + 2.8f + 0.055f * L.Text.Len()});
	}
	Subtitles.RemoveAll([this](const FShown& Sh) { return Sh.Until < Clock; });
	while (Subtitles.Num() > 3)
	{
		Subtitles.RemoveAt(0);
	}
	float Y = H * 0.74f;
	for (const FShown& Sh : Subtitles)
	{
		const float A = FMath::Clamp(Sh.Until - Clock, 0.0f, 1.0f);
		const FString Line = Sh.Speaker + TEXT(":  ") + Sh.Text;
		Text(Line, FLinearColor(Paper.R, Paper.G, Paper.B, Paper.A * A), W * 0.5f, Y, Small, 1.35f, true);
		Y += 30.0f * S;
	}

	// Your stack, the pot, and your cards once you've looked.
	const FBackRoomPrompt P = Table->GetPrompt();
	const float Left = 48.0f * S;
	Text(FString::Printf(TEXT("$%lld"), P.Stack), Paper, Left, H - 128.0f * S, Big, 1.6f, false);
	FString Under = FString::Printf(TEXT("Pot $%lld"), P.Pot);
	if (P.Known.Num() == 2)
	{
		Under += TEXT("     ") + CardText(P.Known[0]) + TEXT(" ") + CardText(P.Known[1]);
	}
	Text(Under, Dim, Left, H - 72.0f * S, Small, 1.2f, false);

	// Your turn: what you can do.
	if (P.bYourTurn)
	{
		const FString Head = P.ToCall > 0 ? FString::Printf(TEXT("$%lld to call"), P.ToCall) : TEXT("Checks to you");
		Text(Head, Warm, W * 0.5f, H - 150.0f * S, Big, 1.25f, true);
		FString Keys = TEXT("[F] Fold     ");
		Keys += P.bCanCheck ? TEXT("[C] Check") : FString::Printf(TEXT("[C] Call $%lld"), P.ToCall);
		if (P.bCanRaise)
		{
			Keys += FString::Printf(TEXT("     [R] %s $%lld     [A] All in"), P.bIsBet ? TEXT("Bet") : TEXT("Raise to"), P.RaiseTo);
		}
		Text(Keys, Paper, W * 0.5f, H - 100.0f * S, Small, 1.25f, true);
		if (P.bCanRaise)
		{
			Text(TEXT("wheel to size"), Dim, W * 0.5f, H - 66.0f * S, Small, 1.0f, true);
		}
	}

	// Focus left, when it isn't full.
	if (const ABackRoomPawn* Pawn = Cast<ABackRoomPawn>(GetOwningPawn()))
	{
		if (Pawn->FocusLeft < 0.995f || Pawn->Focus > 0.01f)
		{
			const float Bw = 160.0f * S, Bh = 4.0f * S;
			const float Bx = W - Bw - 48.0f * S, By = H - 60.0f * S;
			DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.35f), Bx, By, Bw, Bh);
			DrawRect(FLinearColor(Warm.R, Warm.G, Warm.B, 0.85f), Bx, By, Bw * Pawn->FocusLeft, Bh);
			Text(TEXT("FOCUS"), Dim, Bx, By - 26.0f * S, Small, 0.9f, false);
		}
	}
}

// ------------------------------------------------------------------ game mode

ABackRoomGameMode::ABackRoomGameMode()
{
	DefaultPawnClass = ABackRoomPawn::StaticClass();
	HUDClass = ABackRoomHUD::StaticClass();
}

ABackRoomStage* ABackRoomGameMode::FindOrSpawnStage()
{
	if (Stage)
	{
		return Stage;
	}
	for (TActorIterator<ABackRoomStage> It(GetWorld()); It; ++It)
	{
		Stage = *It;
		return Stage;
	}
	Stage = GetWorld()->SpawnActor<ABackRoomStage>(ABackRoomStage::StaticClass(), FTransform::Identity);
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
	FindOrSpawnStage();
	SeatEveryone();
}

void ABackRoomGameMode::SeatEveryone()
{
	// Players placed in the map (for look development) stay out of the game.
	for (TActorIterator<ABackRoomPlayer> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}

	auto Spawn = [this](const TCHAR* CastName, int32 Seat, EBackRoomRole AtTableAs, const FBackRoomPersona& Persona) {
		const FTransform T = ABackRoomStage::SeatTransform(Seat);
		ABackRoomPlayer* P = GetWorld()->SpawnActorDeferred<ABackRoomPlayer>(ABackRoomPlayer::StaticClass(), T);
		P->Persona = Persona;
		P->SeatRole = AtTableAs;
		P->MetaHumanClass = CastBlueprint(CastName);
		P->FinishSpawning(T);
		return P;
	};
	auto Persona = [](const TCHAR* Name, float Nerves, float Expressive, float Restless, float Fidget, float Posture, float Chatter, int32 Seed,
		TArray<FBackRoomTell> Tells, uint32 ShirtSrgb = 0x8a8a8a) {
		FBackRoomPersona P;
		P.Shirt = FLinearColor(FColor((ShirtSrgb >> 16) & 0xff, (ShirtSrgb >> 8) & 0xff, ShirtSrgb & 0xff));
		P.Name = Name;
		P.Nervousness = Nerves;
		P.Expressiveness = Expressive;
		P.Restlessness = Restless;
		P.ChipFidget = Fidget;
		P.Posture = Posture;
		P.Chatter = Chatter;
		P.Seed = Seed;
		P.Tells = MoveTemp(Tells);
		return P;
	};
	using T = EBackRoomTell;
	using M = EBackRoomTellMeaning;

	Table = GetWorld()->SpawnActor<ABackRoomTable>(ABackRoomTable::StaticClass(), FTransform::Identity);
	Dealer = Spawn(TEXT("Dee"), 4, EBackRoomRole::Dealer, Persona(TEXT("Dee"), 0.1f, 0.3f, 0.3f, 0.0f, 0.6f, 0.2f, 37, {}, 0x1f2a3d));
	Hero = Spawn(TEXT("Hero"), 0, EBackRoomRole::Hero, Persona(TEXT("You"), 0.3f, 0.5f, 0.5f, 0.3f, 0.5f, 0.0f, 7, {}, 0x3c4a44));
	Table->SetDealer(Dealer);
	Table->AddPlayer(Hero, 0, ss::Archetype::Tag, 100);

	// The regulars, and what gives them away.
	struct FRegular
	{
		const TCHAR* Cast;
		int32 Seat;
		ss::Archetype Style;
		int64 BuyIn;
		FBackRoomPersona Persona;
	};
	const FRegular Regulars[] = {
		// Old-timer, plays tight. Honest tells: he glances at his chips when he connects, and a bluff
		// makes him rub his neck.
		{TEXT("Sal"), 3, ss::Archetype::Nit, 240, Persona(TEXT("Sal"), 0.2f, 0.35f, 0.4f, 0.6f, 0.3f, 0.35f, 11,
			{Tell(T::ChipGlance, M::Strong, 0.85f, 0.05f), Tell(T::Recheck, M::Weak, 0.55f, 0.08f), Tell(T::NeckTouch, M::Bluff, 0.75f, 0.08f)}, 0x6b6a4e)},
		// Loud, calls everything. His smile is real when he has it; when he bluffs it's mouth only.
		{TEXT("BigLou"), 5, ss::Archetype::Station, 300, Persona(TEXT("Big Lou"), 0.25f, 0.75f, 0.35f, 0.5f, 0.2f, 0.8f, 53,
			{Tell(T::RealSmile, M::Strong, 0.8f, 0.1f), Tell(T::FalseSmile, M::Bluff, 0.75f, 0.08f), Tell(T::ChipReach, M::Weak, 0.6f, 0.12f)}, 0x1c1c1e)},
		// Wired, bets too much. Shakes when he has it; bluffing, he stares you down and swallows.
		{TEXT("Twitch"), 2, ss::Archetype::Maniac, 160, Persona(TEXT("Twitch"), 0.85f, 0.7f, 0.9f, 0.9f, 0.7f, 0.5f, 41,
			{Tell(T::Tremble, M::Strong, 0.85f, 0.12f), Tell(T::StareDown, M::Bluff, 0.75f, 0.12f), Tell(T::Swallow, M::Bluff, 0.6f, 0.1f),
				Tell(T::BlinkBurst, M::Bluff, 0.45f, 0.15f)}, 0x8c2a24)},
		// Quiet and sharp. She acts: a sigh and a look away mean she's strong. Her pupils don't act.
		{TEXT("Mei"), 6, ss::Archetype::Reg, 220, Persona(TEXT("Mei"), 0.15f, 0.25f, 0.3f, 0.3f, 0.55f, 0.15f, 23,
			{Tell(T::Sigh, M::Strong, 0.65f, 0.08f), Tell(T::LookAway, M::Strong, 0.5f, 0.15f), Tell(T::PupilFlare, M::Strong, 0.6f, 0.05f),
				Tell(T::LipPress, M::Bluff, 0.45f, 0.12f)}, 0x34383d)},
	};
	for (const FRegular& R : Regulars)
	{
		ABackRoomPlayer* P = Spawn(R.Cast, R.Seat, EBackRoomRole::Player, R.Persona);
		Opponents.Add(P);
		Table->AddPlayer(P, R.Seat, R.Style, R.BuyIn);
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
	Table->Begin(4.0f);
}
