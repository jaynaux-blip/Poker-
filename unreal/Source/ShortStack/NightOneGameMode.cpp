#include "NightOneGameMode.h"

#include "Engine/Engine.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/App.h"
#include "Misc/DateTime.h"
#include "NightOneAudio.h"
#include "NightOneGame.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/World.h"
#include "NightOnePawn.h"
#include "NightOnePlayerController.h"
#include "NightOneSaveGame.h"
#include "NightOneStage.h"
#include "SFrontEndWidget.h"
#include "SNightOneOverlay.h"
#include "ShortStack.h"
#include "ShortStack/Cards.h"
#include "ShortStack/UI/SecondScreen.h"
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/SOverlay.h"

#include <algorithm>

namespace NightOneModeDetail
{
const TCHAR* const SettingsSlot = UNightOneSaveGame::SettingsSlotName();

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

/** The living world's debug tools (non-shipping): inspect anyone, see the world's health, play it forward. */
#if !UE_BUILD_SHIPPING
ss::Session* WorldSession(UWorld* World)
{
	ANightOneGameMode* Mode = World ? World->GetAuthGameMode<ANightOneGameMode>() : nullptr;
	FNightOneGame* G = Mode ? Mode->GetGame() : nullptr;
	return G ? &G->Session : nullptr;
}

void LogLines(const std::string& Text)
{
	TArray<FString> Lines;
	FString(UTF8_TO_TCHAR(Text.c_str())).ParseIntoArrayLines(Lines, false);
	for (const FString& L : Lines)
	{
		UE_LOG(LogNightOne, Display, TEXT("%s"), *L);
	}
}

#if !UE_BUILD_SHIPPING
// The career save is compressed (CareerSave.h): a test bankroll is set from inside the game instead.
FAutoConsoleCommandWithWorldAndArgs CareerBankrollCmd(TEXT("ss.Career.Bankroll"), TEXT("ss.Career.Bankroll <dollars>: sets the bankroll and saves (testing)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (ss::Session* S = WorldSession(World))
		{
			if (Args.Num() > 0)
			{
				S->BankrollCents = static_cast<ss::Chips>(FMath::RoundToDouble(FCString::Atod(*Args[0]) * 100.0));
				S->Save();
			}
			UE_LOG(LogNightOne, Display, TEXT("Bankroll $%.2f"), static_cast<double>(S->BankrollCents) / 100.0);
		}
	}));

FAutoConsoleCommandWithWorldAndArgs LifeDoCmd(TEXT("ss.Life.Do"), TEXT("ss.Life.Do <activity id>: starts a shift, a nap or a night's sleep as the apps would (testing)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (ss::Session* S = WorldSession(World))
		{
			const std::string Why = S->StartActivity(Args.Num() > 0 ? std::string(TCHAR_TO_UTF8(*Args[0])) : std::string("sleep"));
			LogLines(Why.empty() ? std::string("Started\n") : Why + "\n");
		}
	}));

FAutoConsoleCommandWithWorldAndArgs LiveGoCmd(TEXT("ss.Live.Go"),
	TEXT("ss.Live.Go [occurrence id]: registers for one of the Embercrest's events (the next open one without an id) and heads out, as the Burner app's button does."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (ss::Session* S = WorldSession(World))
		{
			const std::string Why = S->GoToLive(Args.Num() > 0 ? std::string(TCHAR_TO_UTF8(*Args[0])) : std::string());
			LogLines(Why.empty() ? std::string("Heading out\n") : Why + "\n");
		}
	}));
#endif

std::string UsdText(ss::Chips Cents)
{
	return std::string(TCHAR_TO_UTF8(*FString::Printf(TEXT("$%.2f"), static_cast<double>(Cents) / 100.0)));
}

FAutoConsoleCommandWithWorldAndArgs LiveDescribeCmd(TEXT("ss.Live.Describe"),
	TEXT("The Embercrest as the player sees it now: the coming events (desk, cards, late reg, field, why not), the player's entry and the last ledger lines."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		ss::Session* S = WorldSession(World);
		if (!S)
		{
			return;
		}
		const double Now = S->WorldMinutes();
		std::string Out = "Embercrest at " + ss::net::DateLabel(ss::net::DayOf(Now)) + " " + ss::net::TimeLabel(Now) + " (world " + std::to_string(static_cast<long long>(Now)) +
			"), bankroll " + UsdText(S->BankrollCents) + ", energy " + std::to_string(static_cast<int>(S->Life.Energy)) + "\n";
		for (const ss::live::Occurrence& O : ss::live::Reachable(Now, 30.0))
		{
			const std::string Why = ss::live::CanRegister(S->BankrollCents, S->Life, O, Now);
			Out += "  " + O.Id + "  " + O.T->Name + "  cards " + ss::net::DateLabel(O.Day) + " " + ss::net::TimeLabel(O.Start) + ", desk " + ss::net::TimeLabel(O.DeskOpens) +
				", late reg " + ss::net::TimeLabel(O.LateRegEnds) + ", field " + std::to_string(S->Living().PlannedEntries(O.Id)) + ", the world's people " +
				std::to_string(S->Living().Registered(O.Id).size()) + "  -> " + (Why.empty() ? std::string("open") : Why) + "\n";
		}
		for (const ss::life::LiveEntry& E : S->Life.LiveEntries)
		{
			static const char* States[3] = {"registered", "finished", "refunded"};
			Out += "  entry " + E.Id + " (" + E.Name + "): " + States[FMath::Clamp(E.State, 0, 2)] + ", paid " + UsdText(E.PaidCents) + ", field " + std::to_string(E.Entrants) +
				" (" + std::to_string(E.Roster.size()) + " followed), bus " + (E.FareThere ? "there" : "-") + "/" + (E.FareHome ? "home" : "-") +
				(E.State == ss::life::LiveEntry::Finished ? ", place " + std::to_string(E.Place) + " for " + UsdText(E.PrizeCents) : std::string()) + "\n";
		}
		for (size_t K = 0; K < S->Life.Ledger.size() && K < 8; ++K)
		{
			const ss::life::LedgerEntry& L = S->Life.Ledger[K];
			Out += "  ledger " + ss::net::TimeLabel(L.At) + "  " + L.Label + "  " + (L.Amount < 0 ? "-" : "+") + UsdText(L.Amount < 0 ? -L.Amount : L.Amount) + "\n";
		}
		LogLines(Out);
	}));

FAutoConsoleCommandWithWorldAndArgs WorldReportCmd(TEXT("ss.World.Report"), TEXT("The living world's health: population, stakes, bankrolls, moods, reputations."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World) {
		if (ss::Session* S = WorldSession(World))
		{
			LogLines(S->Living().Report());
		}
	}));

FAutoConsoleCommandWithWorldAndArgs WorldNpcCmd(TEXT("ss.World.Npc"), TEXT("ss.World.Npc <name>: everything about someone (bankroll, skills, results, reputation, schedule, ties, history)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		ss::Session* S = WorldSession(World);
		if (!S || Args.Num() == 0)
		{
			return;
		}
		const std::string Name(TCHAR_TO_UTF8(*FString::Join(Args, TEXT(" "))));
		const int32 Id = S->Living().Find(Name);
		LogLines(Id >= 0 ? S->Living().Describe(Id) : "Nobody called " + Name + "\n");
	}));

FAutoConsoleCommandWithWorldAndArgs WorldLeadersCmd(TEXT("ss.World.Leaders"), TEXT("The 20 best-known players and what they're known for."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World) {
		if (ss::Session* S = WorldSession(World))
		{
			std::string Out;
			for (const int Id : S->Living().Leaders(ss::world::Rep::Overall, 20))
			{
				const ss::world::Npc* N = S->Living().Get(Id);
				Out += N->Name + " (" + ss::world::IdentityName(N->Is) + ", reputation " + std::to_string(static_cast<int>(N->RepOf(ss::world::Rep::Overall))) + ")\n";
			}
			LogLines(Out);
		}
	}));

FAutoConsoleCommandWithWorldAndArgs WorldSimulateCmd(TEXT("ss.World.Simulate"), TEXT("ss.World.Simulate <days> (1, 7, 30, 365): sleep through the days; the world and the clock move on."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (ss::Session* S = WorldSession(World))
		{
			const int32 Days = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 1;
			S->WorldSkip(FMath::Clamp(Days, 1, 3650));
			LogLines(S->Living().Report());
		}
	}));

FAutoConsoleCommandWithWorldAndArgs WorldAwardCmd(TEXT("ss.World.Award"),
	TEXT("ss.World.Award <bracelet|ring> [main] [name]: a bracelet or a ring for the player (or someone by name), to see the trophy case and the champion's frame."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		ss::Session* S = WorldSession(World);
		if (!S || Args.Num() == 0)
		{
			return;
		}
		const bool Ring = Args[0].Equals(TEXT("ring"), ESearchCase::IgnoreCase);
		const bool Main = Args.Num() > 1 && Args[1].Equals(TEXT("main"), ESearchCase::IgnoreCase);
		TArray<FString> Rest;
		for (int32 I = Main ? 2 : 1; I < Args.Num(); ++I)
		{
			Rest.Add(Args[I]);
		}
		const std::string Name(TCHAR_TO_UTF8(*FString::Join(Rest, TEXT(" "))));
		const int32 Id = Name.empty() ? -1 : S->Living().Find(Name);
		if (!Name.empty() && Id < 0)
		{
			LogLines("Nobody called " + Name + "\n");
			return;
		}
		S->Living().GrantAward(Id, Ring, Main);
		LogLines(std::string(Ring ? "A ring" : "A bracelet") + " for " + (Name.empty() ? std::string("you") : Name) + "\n");
	}));

FAutoConsoleCommandWithWorldAndArgs WorldPlayerResultsCmd(TEXT("ss.World.PlayerResults"),
	TEXT("ss.World.PlayerResults <count>: plays that many small tournaments onto your stats page (Career > Your stats), to preview it."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (ss::Session* S = WorldSession(World))
		{
			S->Living().GrantHeroResults(FMath::Clamp(Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 200, 1, 5000));
			LogLines("Your stats page has " + std::to_string(S->Living().HeroStats().Events) + " tournaments\n");
		}
	}));

FAutoConsoleCommandWithWorldAndArgs WorldPreviewCmd(TEXT("ss.World.Preview"), TEXT("ss.World.Preview <days>: how the world would look then (a copy is played forward; nothing changes)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (ss::Session* S = WorldSession(World))
		{
			LogLines(S->WorldPreview(FMath::Clamp(Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 30, 1, 3650)));
		}
	}));
#endif

void SetConsoleInt(const TCHAR* Name, int32 Value)
{
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name))
	{
		Var->Set(Value, ECVF_SetByGameSetting);
	}
}
/** What Dee texts after a night at her game, from what the Back Room passed back (cents, reads learned). */
FString HomeTextFromOptions(const FString& Options)
{
	if (UGameplayStatics::HasOption(Options, TEXT("Live")))
	{
		// Home from the Embercrest: Dee saw it all from the box.
		const int32 Place = FCString::Atoi(*UGameplayStatics::ParseOption(Options, TEXT("Place")));
		const int32 Of = FCString::Atoi(*UGameplayStatics::ParseOption(Options, TEXT("Of")));
		const int64 Prize = FCString::Atoi64(*UGameplayStatics::ParseOption(Options, TEXT("Prize")));
		const int32 Learned = FCString::Atoi(*UGameplayStatics::ParseOption(Options, TEXT("Learned")));
		const int32 Tens = Place % 100;
		const TCHAR* Suffix = (Tens >= 11 && Tens <= 13) ? TEXT("th") : (Place % 10 == 1 ? TEXT("st") : (Place % 10 == 2 ? TEXT("nd") : (Place % 10 == 3 ? TEXT("rd") : TEXT("th"))));
		const FString Event = UGameplayStatics::ParseOption(Options, TEXT("Event")).Replace(TEXT("_"), TEXT(" ")).ToLower();
		FString Text;
		if (UGameplayStatics::HasOption(Options, TEXT("Cancelled")))
		{
			Text = FString::Printf(TEXT("they cancelled the %s? not enough people. happens on a slow night. you got your money back at least."), *Event);
		}
		else if (UGameplayStatics::HasOption(Options, TEXT("Won")))
		{
			Text = Event.IsEmpty() ? FString::Printf(TEXT("YOU WON THE EMBERCREST. $%lld. the whole floor's talking about you. pay your rent, then call me."), Prize / 100)
								   : FString::Printf(TEXT("YOU WON THE %s. $%lld. your name's going on the wall. pay your rent, then call me."), *Event.ToUpper(), Prize / 100);
		}
		else if (Prize > 0)
		{
			Text = FString::Printf(TEXT("%d%s of %d and a cash. $%lld. told you strangers are easier."), Place, Suffix, Of, Prize / 100);
		}
		else if (Place > 0 && Place <= Of / 3)
		{
			Text = FString::Printf(TEXT("%d%s of %d. close. you're better than half that room already."), Place, Suffix, Of);
		}
		else
		{
			Text = FString::Printf(TEXT("%d%s of %d. tournaments are long. come play my game tuesday and get your reps in."), Place, Suffix, Of);
		}
		if (Learned > 0)
		{
			Text += TEXT(" and you picked up a read. good.");
		}
		return Text;
	}
	const int64 Net = FCString::Atoi64(*UGameplayStatics::ParseOption(Options, TEXT("Net")));
	const int32 Learned = FCString::Atoi(*UGameplayStatics::ParseOption(Options, TEXT("Learned")));
	const bool bBusted = UGameplayStatics::HasOption(Options, TEXT("Busted"));
	const bool bClosed = UGameplayStatics::HasOption(Options, TEXT("Closed"));
	const FString Dollars = FString::Printf(TEXT("$%lld"), FMath::Abs(Net) / 100);
	FString Text;
	if (bBusted)
	{
		Text = TEXT("Rough night. Everybody gets felted. Sleep, then come back and watch more than you play.");
	}
	else if (Net >= 10000)
	{
		Text = FString::Printf(TEXT("You took %s off my table. Sal's still muttering. Pay your rent before you get ideas."), *Dollars);
	}
	else if (Net > 0)
	{
		Text = FString::Printf(TEXT("Up %s. That's a good night at my game. Don't tell Lou I said so."), *Dollars);
	}
	else if (Net == 0)
	{
		Text = TEXT("Broke even. Nobody got hurt. You'll do better when you watch their hands.");
	}
	else
	{
		Text = FString::Printf(TEXT("Down %s. It happens. Next time watch Sal's eyes when the flop comes."), *Dollars);
	}
	if (bClosed)
	{
		Text += TEXT(" Thanks for staying till close.");
	}
	if (Learned > 0)
	{
		Text += Learned == 1 ? TEXT(" And you caught one of their tells. Most people never see one.") : TEXT(" And you're reading them now. They'll start to feel it.");
	}
	return Text;
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
	{
		std::string Text;
		bLoaded = CareerSave::LoadText(Text) && ss::SaveData::Parse(Text, Loaded);
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
	if (UGameplayStatics::HasOption(OptionsString, TEXT("Home")) && bLoaded)
	{
		// Back from Dee's game (the Back Room level opened this one with "?Home"): straight to the desk.
		HomeText = HomeTextFromOptions(OptionsString);
		HomeTextAt = 4.5;
		// What happened on the calendar while out (a deadline passing at 3 AM) still happens.
		const FString From = UGameplayStatics::ParseOption(OptionsString, TEXT("From"));
		if (!From.IsEmpty())
		{
			Game->Session.ResumeCalendarFrom(FCString::Atod(*From));
		}
		Begin(FString(UTF8_TO_TCHAR(Game->Session.HeroName.c_str())));
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (PC->PlayerCameraManager)
			{
				PC->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 2.5f, FLinearColor::Black, true, false);
			}
		}
	}
	else if (MenuWidget)
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

void ANightOneGameMode::TestLook(float Lean, float Yaw, float Pitch)
{
	// A script is looking: the real pointer no longer steers the head.
	bTestInput = true;
	if (ANightOnePawn* Seat = GetSeat())
	{
		Seat->TargetFocus = FMath::Clamp(Lean, 0.0f, 1.0f);
		Seat->Yaw = Yaw;
		Seat->Pitch = Pitch;
	}
}

FString ANightOneGameMode::TestStream(bool bLive)
{
	if (!Game)
	{
		return TEXT("no game");
	}
	if (!bLive)
	{
		Game->Session.EndStream();
		return TEXT("ended");
	}
	return UTF8_TO_TCHAR(Game->Session.GoLive().c_str());
}

void ANightOneGameMode::TestNewCareer(const FString& ScreenName)
{
	if (Game && !bStarted)
	{
		// As the title screen does: the menu closes, then the career starts.
		Game->Menu.Close(RealTime);
		StartNewCareer(ScreenName);
	}
}

void ANightOneGameMode::TestClick(float X, float Y)
{
	if (!Game)
	{
		return;
	}
	bTestInput = true;
	ss::ui::Pointer& P = Game->Client.UI.Ptr;
	P.Active = true;
	P.X = X;
	P.Y = Y;
	OnPress(true);
	TestReleaseAt = RealTime + 0.12;
}

void ANightOneGameMode::TestKey(const FString& Key)
{
	OnKey(Key);
}

void ANightOneGameMode::TestWheel(float Delta)
{
	OnWheel(Delta);
}

FString ANightOneGameMode::TestDescribe() const
{
	if (!Game)
	{
		return TEXT("no game");
	}
	const ss::Session& S = Game->Session;
	auto Dollars = [](int64 Cents) { return FString::Printf(TEXT("%s$%.2f"), Cents < 0 ? TEXT("-") : TEXT(""), FMath::Abs(Cents) / 100.0); };
	auto Clock = [](double Minutes) {
		const int32 M = FMath::FloorToInt(FMath::Fmod(FMath::Max(0.0, Minutes), 1440.0));
		const int32 H = M / 60;
		return FString::Printf(TEXT("%d:%02d %s"), H % 12 == 0 ? 12 : H % 12, M % 60, H < 12 ? TEXT("AM") : TEXT("PM"));
	};
	auto Cards = [](const std::vector<ss::Card>& Cs) {
		FString Out;
		for (ss::Card C : Cs)
		{
			Out += FString(UTF8_TO_TCHAR(ss::CardToString(C).c_str())) + TEXT(" ");
		}
		return Out.TrimEnd();
	};
	static const TCHAR* Screens[4] = {TEXT("boot"), TEXT("lobby"), TEXT("table"), TEXT("results")};
	static const TCHAR* Rents[4] = {TEXT("due"), TEXT("paid"), TEXT("final notice"), TEXT("evicted")};
	const ss::life::State& L = S.Life;
	FString Out = FString::Printf(TEXT("started=%d menu=%d leaned=%d screen=%s | %s | bankroll %s | %s (world %.0f, day %d) | energy %.0f | rent %s %s, %.1f h left | streaming=%d\n"),
		bStarted ? 1 : 0, IsMenuOpen() ? 1 : 0, IsLeanedBack() ? 0 : 1, Screens[FMath::Clamp(static_cast<int32>(S.CurrentScreen), 0, 3)],
		UTF8_TO_TCHAR(S.HeroName.c_str()), *Dollars(S.BankrollCents), *Clock(S.ClockMinutes()), S.WorldMinutes(), static_cast<int32>(S.WorldMinutes() / 1440.0),
		L.Energy, Rents[FMath::Clamp(static_cast<int32>(L.RentStage), 0, 3)], *Dollars(L.RentDueCents), (L.RentDeadline - S.WorldMinutes()) / 60.0, S.Streaming() ? 1 : 0);
	if (S.TableCount() > 0)
	{
		Out += FString::Printf(TEXT("tables open %d (in front %d, waiting on you %d)\n"), S.TableCount(), S.FocusedTable(), S.TablesWaiting());
	}
	if (S.T)
	{
		const ss::Level& Lv = S.T->CurrentLevel();
		Out += FString::Printf(TEXT("event: %s | level %d %lld/%lld ante %lld | %d of %d left | hands %d\n"), UTF8_TO_TCHAR(S.Joined.Name.c_str()), S.T->LevelIndex + 1,
			static_cast<int64>(Lv.Sb), static_cast<int64>(Lv.Bb), static_cast<int64>(Lv.Ante), S.T->Remaining, S.T->Spec.Entrants, S.HandsPlayed);
		for (const ss::SeatVis& V : S.Seats)
		{
			if (!V.Present)
			{
				continue;
			}
			Out += FString::Printf(TEXT("  seat %d%s %s: %lld%s%s%s%s%s%s\n"), V.Seat, V.Seat == S.ButtonSeat ? TEXT(" (button)") : TEXT(""), UTF8_TO_TCHAR(V.Name.c_str()),
				static_cast<int64>(V.Stack), V.Bet > 0 ? *FString::Printf(TEXT(" bet %lld"), static_cast<int64>(V.Bet)) : TEXT(""),
				V.Folded ? TEXT(" folded") : TEXT(""), V.AllIn ? TEXT(" ALL-IN") : TEXT(""),
				V.LastAction.empty() ? TEXT("") : *FString::Printf(TEXT(" [%s]"), UTF8_TO_TCHAR(V.LastAction.c_str())),
				V.Hole.empty() ? TEXT("") : *FString::Printf(TEXT(" cards %s"), *Cards(V.Hole)), V.IsHero ? TEXT("  <- you") : TEXT(""));
		}
		Out += FString::Printf(TEXT("board: %s | pot %lld\n"), S.Board.empty() ? TEXT("(none)") : *Cards(S.Board), static_cast<int64>(S.PotChips));
		if (S.HasPrompt)
		{
			const ss::HeroPrompt& P = S.Prompt;
			Out += FString::Printf(TEXT("YOUR TURN: to call %lld%s | pot %lld | raise %s (min %lld, max %lld, set to %lld) | bb %lld | %.0f s left\n"),
				static_cast<int64>(P.ToCall), P.CanCheck ? TEXT(" (can check)") : TEXT(""), static_cast<int64>(P.Pot), P.CanRaise ? (P.IsBet ? TEXT("bet") : TEXT("to")) : TEXT("not allowed"),
				static_cast<int64>(P.MinRaise), static_cast<int64>(P.MaxRaise), static_cast<int64>(P.RaiseTo), static_cast<int64>(P.BigBlind), P.Deadline - S.Now);
		}
	}
	if (S.HasResults)
	{
		const ss::Results& R = S.LastResults;
		Out += FString::Printf(TEXT("results: %s, %d of %d, prize %s (buy-in %s), %d hands, accuracy %.0f%%\n"), UTF8_TO_TCHAR(R.EventName.c_str()), R.Place, R.Entrants,
			*Dollars(R.PrizeCents), *Dollars(R.BuyInCents), R.Hands, R.AccuracyPct);
	}
	for (size_t I = 0; I < L.Ledger.size() && I < 3; ++I)
	{
		Out += FString::Printf(TEXT("ledger: %s %s\n"), UTF8_TO_TCHAR(L.Ledger[I].Label.c_str()), *Dollars(L.Ledger[I].Amount));
	}
	return Out;
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
		User->SetResolutionScaleValueEx(static_cast<float>(FMath::Min(NewSettings.ResolutionScale, 100)));
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
	// The render scale: fixed, or moving to hold the chosen frame rate (FrameBudget.h).
	FrameBudget.Configure(NewSettings.DynamicTarget, NewSettings.ResolutionScale);
	// Hardware ray-traced Lumen (the project enables ray tracing support); hit lighting for reflections at Cinematic.
	SetConsoleInt(TEXT("r.Lumen.HardwareRayTracing"), NewSettings.RayTracing ? 1 : 0);
	SetConsoleInt(TEXT("r.Lumen.HardwareRayTracing.LightingMode"), NewSettings.RayTracing && NewSettings.Quality >= 4 ? 2 : 0);
	if (Stage)
	{
		Stage->BrightnessBias = static_cast<float>(NewSettings.Brightness - 50) / 50.0f * 1.5f;
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
	// The engine mutes a game whose window isn't in front unless told otherwise.
	FApp::SetUnfocusedVolumeMultiplier(NewSettings.BackgroundAudio ? 1.0f : 0.0f);
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
	// The game closed during a night at the Embercrest: straight back to the seat.
	if (!Game->Session.LiveInProgress().empty())
	{
		UE_LOG(LogNightOne, Log, TEXT("Back to %s, in progress"), UTF8_TO_TCHAR(Game->Session.LiveInProgress().c_str()));
		Game->Session.ResumeLive();
	}
}

bool ANightOneGameMode::GoOut(const FString& ActivityId, int64 BuyInCents)
{
	if (bLeaving || !bStarted || !Game)
	{
		return false;
	}
	// The session saved itself before calling out. Grab the coat: the room fades, the street, then the game.
	bLeaving = true;
	LeaveAt = RealTime + 1.6;
	LeaveBuyInCents = BuyInCents;
	LeaveFor = ActivityId;
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, 1.5f, FLinearColor::Black, true, true);
		}
	}
	if (Audio)
	{
		Audio->PlayEffect(ss::audio::Effect::Thump);
	}
	UE_LOG(LogNightOne, Log, TEXT("Heading out to %s with %lld cents"), *ActivityId, BuyInCents);
	return true;
}

void ANightOneGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (SettingsDirtyAt >= 0.0)
	{
		SaveSettingsNow();
	}
	// The save being written (and any waiting) lands before the next scene reads it.
	if (Game)
	{
		Game->Saver.Flush();
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
	TSharedPtr<ss::ui::DrawList> List = MenuHint.Make();
	ss::ui::Canvas Cv(*List, Game->Measurer, LogicalW, LogicalH, static_cast<float>(ViewSize.Y) / LogicalH);
	Game->Menu.Draw(Cv, RealTime);
	MenuHint.Note(*List);
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

	if (bLeaving && RealTime >= LeaveAt)
	{
		bLeaving = false;
		// The Back Room for Dee's game; the same map turns into the Embercrest's card room for a tournament (the event
		// the session registered the player for).
		const FString Options = ss::live::IsEmbercrest(std::string(TCHAR_TO_UTF8(*LeaveFor))) ? FString::Printf(TEXT("Live=%s"), *LeaveFor)
																							   : FString::Printf(TEXT("BuyIn=%lld"), LeaveBuyInCents);
		UGameplayStatics::OpenLevel(this, FName(TEXT("BackRoom")), true, Options);
		return;
	}
	if (HomeTextAt >= 0.0 && RealTime >= HomeTextAt)
	{
		HomeTextAt = -1.0;
		if (!HomeText.IsEmpty())
		{
			Game->Text("Dee", std::string(TCHAR_TO_UTF8(*HomeText)));
		}
	}

	if (TestReleaseAt >= 0.0 && RealTime >= TestReleaseAt)
	{
		TestReleaseAt = -1.0;
		OnRelease();
	}
	const bool bBeat = Audio && Audio->ConsumeBeat();
	Game->Saver.Tick();
	FrameBudget.Tick(static_cast<float>(Dt));
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
		TSharedPtr<ss::ui::DrawList> List = ClientHint.Make();
		ss::ui::Canvas Cv(*List, Game->Measurer, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, static_cast<float>(Stage->ScreenResolution.X) / ss::ui::RiverLine::Width);
		Game->Client.Draw(Cv, GameTime);
		ClientHint.Note(*List);
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

	// The player's hands follow the head and the pointer; they sit out the establishing shot.
	if (Seat)
	{
		Stage->UpdateArms(static_cast<float>(Dt), Seat->GetActorLocation(), Seat->Camera ? Seat->Camera->GetForwardVector() : Seat->GetActorForwardVector(), ArmsPointer,
		                  bStarted && !Game->Menu.WantsEstablishingShot());
	}

	// The world reacts to the game.
	const double Clock = S.ClockMinutes();
	// Daylight from the time of day: dawn after a long night, the afternoon after a day shift, dusk again.
	Stage->SetDawn(static_cast<float>(S.Daylight()));
	const float PhoneLevel = static_cast<float>(Game->Phone.Brightness(RealTime));
	Stage->SetPhoneBrightness(PhoneLevel);
	PhoneAccum += Dt;
	if (PhoneLevel > 0.0f && PhoneAccum >= 0.1)
	{
		PhoneAccum = 0.0;
		TSharedPtr<ss::ui::DrawList> List = PhoneHint.Make();
		ss::ui::Canvas Cv(*List, Game->Measurer, ss::ui::PhoneScreen::Width, ss::ui::PhoneScreen::Height, 1.0f);
		Game->Phone.Draw(Cv, Clock);
		PhoneHint.Note(*List);
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
	// The GearDrop LED kit: the room in the chosen colour, flashing with the stream when synced.
	{
		const ss::gear::Glow Leds = S.RoomGlow(GameTime);
		const FLinearColor LedColor = FLinearColor::FromSRGBColor(FColor(static_cast<uint8>((Leds.Rgb >> 16) & 0xff), static_cast<uint8>((Leds.Rgb >> 8) & 0xff), static_cast<uint8>(Leds.Rgb & 0xff)));
		Stage->SetRoomLights(Leds.On, LedColor, static_cast<float>(Leds.Level));
		// What the player owns, set up in the room, and what their career has left on the windowsill. The PC's RGB
		// follows the kit (or cycles through the rainbow on its own); the streaming lights come on while live.
		const ss::gear::Effects& Fx = S.GearFx();
		FRoomGear Room;
		Room.bMonitor = S.Owns("monitor-24");
		Room.bMonitorWide = S.Owns("monitor-27");
		Room.Towers = Fx.PcTier >= 3 ? 2 : (Fx.PcTier >= 2 ? 1 : 0);
		Room.Cam = Fx.CamTier;
		Room.Mic = Fx.MicTier;
		Room.Lights = Fx.Lights;
		Room.bMacroPad = Fx.MacroPad;
		Room.bHeadphones = S.Owns("headphones");
		Room.bPlant = S.Owns("plant");
		Room.bCurtains = S.Owns("curtains");
		Room.bRouter = Fx.Fiber;
		Room.bTrophy = S.Life.LiveBestPlace == 1;
		Room.bDeeChip = S.Life.BackRoomNetCents > 0;
		Stage->SetGear(Room);
		const FLinearColor Rgb = Leds.On ? LedColor : FLinearColor::MakeFromHSV8(static_cast<uint8>(FMath::Fmod(RealTime * 12.0, 256.0)), 190, 255);
		Stage->SetGearGlow(Rgb, Leds.On ? static_cast<float>(Leds.Level) : 1.0f, S.Streaming());
	}
	// The monitors' pictures, ten times a second (ss::ui::secondscreen).
	MonitorAccum += Dt;
	if (MonitorAccum >= 0.1 && (Stage->MonitorShown(0) || Stage->MonitorShown(1)))
	{
		MonitorAccum = 0.0;
		for (int32 M = 0; M < 2; ++M)
		{
			if (Stage->MonitorShown(M))
			{
				TSharedPtr<ss::ui::DrawList> List = MonitorHint[M].Make();
				ss::ui::Canvas Cv(*List, Game->Measurer, ss::ui::secondscreen::Width, ss::ui::secondscreen::Height,
				                  static_cast<float>(Stage->MonitorResolution.X) / ss::ui::secondscreen::Width);
				ss::ui::secondscreen::Draw(Cv, S, M, GameTime);
				MonitorHint[M].Note(*List);
				Stage->SetMonitorDrawList(M, List);
			}
		}
	}

	if (SettingsDirtyAt >= 0.0 && RealTime - SettingsDirtyAt > 0.75)
	{
		SaveSettingsNow();
	}
}

// ------------------------------------------------------------------ input

void ANightOneGameMode::OnMouse(bool bOverScreen, const FVector2D& Client, float Nx, float Ny)
{
	ArmsPointer = FVector2D(Nx + 0.5f, Ny + 0.5f);
	ANightOnePawn* Seat = GetSeat();
	if (!Game || !Seat || IsMenuOpen())
	{
		bOverScreenNow = false;
		return;
	}
	bOverScreenNow = bOverScreen;
	ss::ui::Pointer& P = Game->Client.UI.Ptr;
	if (bTestInput)
	{
		return; // a script is playing: its clicks place the pointer
	}
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
	if (Stage)
	{
		Stage->ArmsClick();
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
	if (Stage)
	{
		Stage->ArmsKey(Key);
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
