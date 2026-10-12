#include "NightOneGameMode.h"

#include "Engine/Engine.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "DeeRoomCaption.h"
#include "DeeRoomStage.h"
#include "DeeRoomTone.h"
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
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "NightOneAudio.h"
#include "NightOneGame.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/World.h"
#include "NightOnePawn.h"
#include "NightOnePlayerController.h"
#include "NightOneSaveGame.h"
#include "NightOneStage.h"
#include "SFrontEndWidget.h"
#include "SNightOneOverlay.h"
#include "CastLooks.h"
#include "ShopStage.h"
#include "ShortStack.h"
#include "ShortStack/Cards.h"
#include "ShortStack/UI/SecondScreen.h"
#include "ShortStack/UI/ShopScreen.h"
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/SOverlay.h"

#include <algorithm>

namespace NightOneCloset
{
/**
 * The closet over the hub (spec 7.6): core-ui's WardrobeScreen in the apartment and on Dee's couch, on C while leaned back
 * from the laptop (leaned in, C calls at the table). Keyed and drawn as the menus are, over the room. One hub at a time, so
 * one closet, kept with the mode that opened it.
 */
struct FCloset
{
	TWeakObjectPtr<ANightOneGameMode> Owner;
	TUniquePtr<ss::ui::ShopScreen> Screen;
	TSharedPtr<SFrontEndWidget> Widget;
	TWeakObjectPtr<UGameViewportClient> Viewport;
	double Now = 0.0;
	bool bLeft = false;
};
FCloset GCloset;

bool IsOpen(const ANightOneGameMode* Mode)
{
	return GCloset.Owner.Get() == Mode && GCloset.Screen && GCloset.Screen->IsOpen();
}

/** The closet put away (its widget off the viewport too when bDrop: the level's ending). */
void Close(bool bDrop)
{
	if (GCloset.Screen && GCloset.Screen->IsOpen())
	{
		GCloset.Screen->Close(GCloset.Now);
	}
	GCloset.Screen.Reset();
	if (GCloset.Widget)
	{
		GCloset.Widget->SetVisibility(EVisibility::Collapsed);
		if (bDrop)
		{
			if (UGameViewportClient* Vp = GCloset.Viewport.Get())
			{
				Vp->RemoveViewportWidgetContent(GCloset.Widget.ToSharedRef());
			}
			GCloset.Widget.Reset();
		}
	}
	if (bDrop)
	{
		GCloset.Owner = nullptr;
	}
}

/** Opens the closet over the hub (Page: "apartment" or "dee", where it was opened from). False when core has no closet page. */
bool Open(ANightOneGameMode* Mode, ss::Session& S, double Now, const FString& Page)
{
	UGameViewportClient* Vp = Mode && Mode->GetWorld() ? Mode->GetWorld()->GetGameViewport() : nullptr;
	if (!Vp || !FSlateApplication::IsInitialized())
	{
		return false;
	}
	std::unique_ptr<ss::ui::ShopScreen> Made = ss::ui::MakeShopScreen("wardrobe", S);
	if (!Made)
	{
		return false;
	}
	if (GCloset.Owner.Get() != Mode)
	{
		Close(true);
	}
	GCloset.Owner = Mode;
	GCloset.Viewport = Vp;
	GCloset.Screen.Reset(Made.release());
	GCloset.bLeft = false;
	GCloset.Now = Now;
	// What the hero can be shown in (the street's 3D: the closet won't put on what can't be drawn there); no 3D preview here,
	// the hero's in the chair at the desk: the page previews on the portrait.
	ss::ui::HostCaps Caps;
	Caps.GarmentKinds = AShopStage::WearableKinds();
	Caps.Preview3D = false;
	// The look's lists as long as its options (spec 7.4: past the end reads as missing), what's on the hero's head now marked.
	using ss::hero::Slot;
	const ss::hero::Look& Worn = S.Person.Appearance;
	auto Sized = [](std::vector<bool>& V, Slot Of, int Now) {
		V.assign(static_cast<size_t>(std::max(0, ss::hero::OptionCount(Of))), false);
		if (Now >= 0 && Now < static_cast<int>(V.size()))
		{
			V[static_cast<size_t>(Now)] = true;
		}
	};
	Sized(Caps.HairStyles, Slot::Hair, Worn.Hair);
	Sized(Caps.FacialHair, Slot::FacialHair, Worn.FacialHair);
	Sized(Caps.Brows, Slot::Brows, Worn.Brows);
	GCloset.Screen->SetCaps(Caps);
	GCloset.Screen->Open(Now, TCHAR_TO_UTF8(*Page));
	if (!GCloset.Widget)
	{
		GCloset.Widget = SNew(SFrontEndWidget);
		GCloset.Widget->OnMenuKey = [](const FString& KeyName, bool bFromGamepad) {
			if (GCloset.Screen && GCloset.Screen->IsOpen())
			{
				GCloset.Screen->Gamepad = bFromGamepad;
				GCloset.Screen->Key(std::string(TCHAR_TO_UTF8(*KeyName)), GCloset.Now);
				GCloset.bLeft = GCloset.bLeft || GCloset.Screen->TakeLeave();
			}
		};
		GCloset.Widget->OnMenuPointer = [](const FVector2D& Logical, int32 Button, float WheelDelta) {
			if (!GCloset.Screen)
			{
				return;
			}
			ss::ui::Pointer& P = GCloset.Screen->Ptr;
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
		Vp->AddViewportWidgetContent(GCloset.Widget.ToSharedRef(), 25);
	}
	GCloset.Widget->SetVisibility(EVisibility::Visible);
	if (APlayerController* Pc = UGameplayStatics::GetPlayerController(Mode, 0))
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(GCloset.Widget);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Pc->SetInputMode(InputMode);
		Pc->SetShowMouseCursor(true);
	}
	FSlateApplication::Get().SetKeyboardFocus(GCloset.Widget, EFocusCause::SetDirectly);
	return true;
}

/** The closet each frame: drawn, its requests drained, closed once walked away from (true then: the mode takes the input back). */
bool Tick(ANightOneGameMode* Mode, const ss::ui::TextMeasurer& Measurer, double Now, TFunctionRef<void(const FString&, const FString&)> Toast)
{
	if (GCloset.Owner.Get() != Mode || !GCloset.Screen)
	{
		return false;
	}
	GCloset.Now = Now;
	if (GCloset.Screen->IsOpen() && GCloset.Widget)
	{
		FVector2D Size(1920.0, 1080.0);
		if (UGameViewportClient* Vp = GCloset.Viewport.Get())
		{
			FVector2D Measured;
			Vp->GetViewportSize(Measured);
			Size = Measured.X > 0.0 && Measured.Y > 0.0 ? Measured : Size;
		}
		const float H = ss::ui::ShopScreen::Height;
		TSharedPtr<ss::ui::DrawList> List = MakeShared<ss::ui::DrawList>();
		ss::ui::Canvas Cv(*List, Measurer, H * static_cast<float>(Size.X / Size.Y), H, static_cast<float>(Size.Y) / H);
		GCloset.Screen->Draw(Cv, Now);
		GCloset.Widget->SetDrawList(List);
	}
	// The hero isn't on screen in the hub: a change worn is the closet's (the session's); the street dresses it on the way out.
	for (const ss::ui::HostRequest& R : GCloset.Screen->TakeRequests())
	{
		if (R.K == ss::ui::HostRequest::Redress)
		{
			Toast(TEXT("Closet"), TEXT("Changed. It's on when you head out."));
		}
	}
	if (GCloset.bLeft || GCloset.Screen->TakeLeave() || !GCloset.Screen->IsOpen())
	{
		Close(false);
		return true;
	}
	return false;
}
} // namespace NightOneCloset

namespace NightOneModeDetail
{
const TCHAR* const SettingsSlot = UNightOneSaveGame::SettingsSlotName();

/**
 * The test hooks' overrides of the hub (ss.Home.Evicted, ss.Home.Hour): never saved, kept across the hub's own trips out
 * and back (the street, the Back Room), forgotten when the level starts fresh.
 */
int32 GHubEvicted = -1;
float GHubHour = -1.0f;
/**
 * What Dee says only once, kept the same way (never saved: a relaunch may hear it again): the first night's welcome (the
 * eviction it was said for), and her text that her game's on downstairs (the night it was sent for).
 */
double GWelcomedFor = -1.0;
int64 GGameTextNight = MIN_int64;

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
			// Who'd pay, as GoToLive chooses (next_segment_spec 1.2): the first of the session's funding options that can, else
			// the player's own money; the event is open or not on that funding.
			const std::vector<std::pair<ss::life::Funding, std::string>> Options = S->FundingOptions(O.Id);
			ss::life::Funding Fund;
			for (const std::pair<ss::life::Funding, std::string>& Option : Options)
			{
				if (Option.second.empty())
				{
					Fund = Option.first;
					break;
				}
			}
			const std::string Why = ss::live::CanRegister(S->BankrollCents, S->Life, O, Now, Fund);
			const std::string Paid = ss::live::FundingLine(Fund, O.T->Short);
			Out += "  " + O.Id + "  " + O.T->Name + "  cards " + ss::net::DateLabel(O.Day) + " " + ss::net::TimeLabel(O.Start) + ", desk " + ss::net::TimeLabel(O.DeskOpens) +
				", late reg " + ss::net::TimeLabel(O.LateRegEnds) + ", field " + std::to_string(S->Living().PlannedEntries(O.Id)) + ", the world's people " +
				std::to_string(S->Living().Registered(O.Id).size()) + "  -> " + (Why.empty() ? std::string("open") : Why) + (Paid.empty() ? std::string() : " (" + Paid + ")") + "\n";
			// Every way it could be paid for, once there's more than the player's own money.
			for (size_t K = 0; Options.size() > 1 && K < Options.size(); ++K)
			{
				const std::string Line = ss::live::FundingLine(Options[K].first, O.T->Short);
				Out += "      pay: " + (Line.empty() ? std::string("your own money") : Line) + "  -> " + (Options[K].second.empty() ? std::string("can") : Options[K].second) + "\n";
			}
		}
		for (const ss::life::LiveEntry& E : S->Life.LiveEntries)
		{
			static const char* States[3] = {"registered", "finished", "refunded"};
			Out += "  entry " + E.Id + " (" + E.Name + "): " + States[FMath::Clamp(E.State, 0, 2)] + ", paid " + UsdText(E.PaidCents) + ", field " + std::to_string(E.Entrants) +
				" (" + std::to_string(E.Roster.size()) + " followed), bus " + (E.FareThere ? "there" : "-") + "/" + (E.FareHome ? "home" : "-") +
				(E.State == ss::life::LiveEntry::Finished ? ", place " + std::to_string(E.Place) + " for " + UsdText(E.PrizeCents) : std::string()) +
				(ss::life::IsOwnMoney(E.Fund) ? std::string() : ", " + ss::live::FundingLine(E.Fund, E.Name)) + "\n";
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

FAutoConsoleCommandWithWorldAndArgs RentNextCmd(TEXT("ss.Rent.Next"),
	TEXT("ss.Rent.Next: the clock to a minute before the next rent deadline (evicted: the storage unit's renewal), to watch it land. Not with tables open."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World) {
		ss::Session* S = WorldSession(World);
		if (!S)
		{
			return;
		}
		static const char* Stages[4] = {"due", "paid", "final notice", "evicted"};
		const bool bEvicted = S->Evicted();
		const std::string Head = std::string("Rent: ") + Stages[FMath::Clamp(static_cast<int32>(S->Life.RentStage), 0, 3)] + ", $" + std::to_string(S->Life.RentDueCents / 100) +
								 (bEvicted ? " to move back in" : "") + " (bankroll $" + std::to_string(S->BankrollCents / 100) + ").";
		// The clock can't be moved under an open table, or during a nap, a shift or a night's sleep (the skip sets the clock
		// every frame from where it began).
		if (S->T)
		{
			LogLines(Head + " Not with a table open: the clock stays.\n");
			return;
		}
		if (S->TimeSkip.Active)
		{
			LogLines(Head + " Not during a time skip" + (S->TimeSkip.Label.empty() ? std::string() : " (" + S->TimeSkip.Label + ")") + ": the clock stays.\n");
			return;
		}
		const double Target = bEvicted ? S->Life.StorageDue : S->Life.RentDeadline;
		if (Target <= 0.0)
		{
			LogLines(Head + (bEvicted ? " Evicted, nothing in storage: no deadline to skip to.\n" : " No deadline to skip to.\n"));
			return;
		}
		const std::string What = bEvicted ? "the storage unit's renewal" : "the deadline";
		const double Left = Target - S->WorldMinutes();
		if (Left > 1.0)
		{
			S->LobbyMinutes += Left - 1.0;
			LogLines(Head + " A minute to " + What + ".\n");
			return;
		}
		LogLines(Head + (Left > 0.0 ? " Less than a minute to " + What + " already.\n" : " Already past " + What + ": it lands on the next update.\n"));
	}));

// The hub: Dee's front room or 1812, and the hour it shows, for testing (none of it saved).
ANightOneGameMode* WorldMode(UWorld* World)
{
	return World ? World->GetAuthGameMode<ANightOneGameMode>() : nullptr;
}

FAutoConsoleCommandWithWorldAndArgs HomeEvictedCmd(TEXT("ss.Home.Evicted"),
	TEXT("ss.Home.Evicted [1|0|-1]: the hub as if evicted (Dee's front room) or not (1812), now; not saved. No argument toggles it, -1 follows the career again."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (ANightOneGameMode* Mode = WorldMode(World))
		{
			Mode->TestEvicted(Args.Num() > 0 ? FCString::Atoi(*Args[0]) : (Mode->IsAtDees() ? 0 : 1));
			UE_LOG(LogNightOne, Display, TEXT("%s"), *Mode->DescribeHub());
		}
	}));

FAutoConsoleCommandWithWorldAndArgs HomeHourCmd(TEXT("ss.Home.Hour"),
	TEXT("ss.Home.Hour <0-24|off>: the hub's light, its window and Dee as at that hour today; the career's clock doesn't move (testing, not saved)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (ANightOneGameMode* Mode = WorldMode(World))
		{
			const bool bOff = Args.Num() == 0 || Args[0].Equals(TEXT("off"), ESearchCase::IgnoreCase);
			Mode->TestHour(bOff ? -1.0f : FCString::Atof(*Args[0]));
			UE_LOG(LogNightOne, Display, TEXT("%s"), *Mode->DescribeHub());
		}
	}));

FAutoConsoleCommandWithWorldAndArgs HomeSkipCmd(TEXT("ss.Home.Skip"),
	TEXT("ss.Home.Skip <0-24>: the career's clock forward to the next time it's that hour, as if the hours passed (the needs, the bills). Not with tables open."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		ANightOneGameMode* Mode = WorldMode(World);
		if (!Mode || Args.Num() == 0)
		{
			LogLines("ss.Home.Skip <0-24>: the hour to move the clock forward to.\n");
			return;
		}
		UE_LOG(LogNightOne, Display, TEXT("%s"), *Mode->TestSkipTo(FCString::Atof(*Args[0])));
	}));

FAutoConsoleCommandWithWorld HomeDescribeCmd(TEXT("ss.Home.Describe"), TEXT("Where the hub is, the hour it shows, and where Dee is."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World) {
		if (const ANightOneGameMode* Mode = WorldMode(World))
		{
			UE_LOG(LogNightOne, Display, TEXT("%s"), *Mode->DescribeHub());
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
/**
 * What Dee texts after a night at her game, from what the Back Room passed back (cents, reads learned), and the career as
 * it stands when she sends it: locked out, the player sleeps on her couch, and a big night is the key back (or a start on it).
 */
FString HomeTextFromOptions(const FString& Options, const ss::Session& S)
{
	if (UGameplayStatics::HasOption(Options, TEXT("Street")))
	{
		return FString(); // back from the corner store: nothing for Dee to say
	}
	const bool bEvicted = S.Evicted();
	const bool bKeyMoney = S.BankrollCents >= S.Life.RentDueCents;
	const TCHAR* KeyBack = bKeyMoney ? TEXT("go get your key back") : TEXT("put it toward your key");
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
			const FString Then = bEvicted ? FString(KeyBack) : FString(TEXT("pay your rent"));
			Text = Event.IsEmpty() ? FString::Printf(TEXT("YOU WON THE EMBERCREST. $%lld. the whole floor's talking about you. %s, then call me."), Prize / 100, *Then)
								   : FString::Printf(TEXT("YOU WON THE %s. $%lld. your name's going on the wall. %s, then call me."), *Event.ToUpper(), Prize / 100, *Then);
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
		Text = bEvicted ? TEXT("Rough night. Everybody gets felted. Couch is yours, sleep. Then come back and watch more than you play.")
						: TEXT("Rough night. Everybody gets felted. Sleep, then come back and watch more than you play.");
	}
	else if (Net >= 10000)
	{
		Text = FString::Printf(TEXT("You took %s off my table. Sal's still muttering. %s before you get ideas."), *Dollars,
			!bEvicted ? TEXT("Pay your rent") : (bKeyMoney ? TEXT("Go get your key back") : TEXT("Put it toward your key")));
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
	// The closet (spec 7.6) holds the input as a menu does.
	return (Game && Game->Menu.IsOpen()) || NightOneCloset::IsOpen(this);
}

void ANightOneGameMode::StartPlay()
{
	Super::StartPlay();
	UWorld* World = GetWorld();
	// A fresh start of the level forgets the test hooks' overrides and what Dee has said once (a trip out and back, or
	// quitting to the menu, keeps them).
	if (!UGameplayStatics::HasOption(OptionsString, TEXT("Home")) && !UGameplayStatics::HasOption(OptionsString, TEXT("Menu")))
	{
		GHubEvicted = -1;
		GHubHour = -1.0f;
		GWelcomedFor = -1.0;
		GGameTextNight = MIN_int64;
	}

	// Progress from the last session.
	ss::SaveData Loaded;
	bool bLoaded = false;
	{
		std::string Text;
#if !UE_BUILD_SHIPPING
		// PIE acceptance (next_segment_spec 9.1 step 1): "open NightOne?Import=pre_segment_team_riverline" starts from a
		// plain-text save, a Tests/fixtures/<name>.ssave by name or any text save by its full path, packed over the career
		// file once it parses (the level opening reads it after the last one's EndPlay has saved, so it wins). The career
		// it replaces is kept beside it first (Saved/SaveGames/NightOne.ssave.before-import-<time>); if that copy can't be
		// made, nothing is imported.
		const FString Import = UGameplayStatics::ParseOption(OptionsString, TEXT("Import"));
		if (!Import.IsEmpty())
		{
			const FString Path = Import.Contains(TEXT("/")) || Import.Contains(TEXT("\\"))
				? Import
				: FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("ShortStackCore/Tests/fixtures"), Import.EndsWith(TEXT(".ssave")) ? Import : Import + TEXT(".ssave"));
			TArray<uint8> Bytes;
			ss::SaveData Probe;
			const bool bRead = FFileHelper::LoadFileToArray(Bytes, *Path, FILEREAD_Silent);
			const std::string Raw = bRead ? std::string(reinterpret_cast<const char*>(Bytes.GetData()), static_cast<size_t>(Bytes.Num())) : std::string();
			const bool bParsed = bRead && ss::SaveData::Parse(Raw, Probe);
			FString Kept;
			const bool bKept = bParsed && CareerSave::KeepCopy(TEXT("before-import-") + FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S")), Kept);
			if (bKept && CareerSave::SaveNow(Raw, false))
			{
				UE_LOG(LogNightOne, Display, TEXT("Import: %s is the career save now (%d KB)%s%s"), *Path, Bytes.Num() / 1024, Kept.IsEmpty() ? TEXT("") : TEXT("; the one it replaced is "),
					*Kept);
			}
			else
			{
				UE_LOG(LogNightOne, Warning, TEXT("Import: %s %s; the career save is untouched"), *Path,
					!bRead ? TEXT("can't be read") : !bParsed ? TEXT("isn't a save this build reads") : !bKept ? TEXT("wasn't imported (the career file couldn't be kept aside)") : TEXT("couldn't be written"));
			}
		}
#endif
		bLoaded = CareerSave::LoadText(Text) && ss::SaveData::Parse(Text, Loaded);
	}
	bHasSave = bLoaded;
	const std::string Seed = std::string(TCHAR_TO_UTF8(*FString::Printf(TEXT("%lld"), FDateTime::Now().GetTicks())));
	Game = MakeUnique<FNightOneGame>(*this, bLoaded ? &Loaded : nullptr, Seed);
	// The console's ss.Closet.* and ss.Fit.Set act on this career while the hub plays (the closet's tests, spec 7.7).
	CastLooks::BindSession(this, &Game->Session);
	// The hub: the apartment, or Dee's front room across the street while the career is evicted (from the title screen
	// on: it's where the player wakes). Moving back in returns it to 1812 the next time the level opens.
	UseRoom(HubEvicted());
	ArrivedWith = OptionsString;

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
	// A different first face in the creator each launch, and the pause menu's heading names the room.
	Game->Menu.CreatorSalt = Seed;
	Game->Menu.SetVenue(ss::ui::Venue::Home);

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
		// Back from Dee's game (the Back Room level opened this one with "?Home"): straight to the desk. Dee's text is
		// written when it's sent, once the hours away have been caught up (the locks may have changed at 3 AM).
		HomeOptions = OptionsString;
		HomeTextAt = 4.5;
		// The hours away are caught up on the first update: if the locks changed while out, the player comes in at Dee's
		// (the room changes while it's still black).
		ArrivalSwapUntil = 0.8;
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
		StartNewCareer(ScreenName, &Game->Menu.Draft);
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
	return Out + DescribeHub() + TEXT("\n");
}

void ANightOneGameMode::TestEvicted(int32 Evicted)
{
	GHubEvicted = Evicted < 0 ? -1 : (Evicted > 0 ? 1 : 0);
	const bool bWant = HubEvicted();
	// On the way out the door, the room stays as it is (the next load has the one wanted).
	if (!Game || bWant == bAtDees || bLeaving)
	{
		return;
	}
	UseRoom(bWant);
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 0.8f, FLinearColor::Black, true, false);
		}
	}
	if (bStarted && bAtDees)
	{
		// In the door as if from the street: she says hello.
		ArrivedWith.Reset();
		DeeHelloAt = RealTime + 2.0;
	}
}

void ANightOneGameMode::TestHour(float Hour)
{
	GHubHour = Hour < 0.0f ? -1.0f : FMath::Clamp(Hour, 0.0f, 24.0f);
}

FString ANightOneGameMode::TestSkipTo(float Hour)
{
	if (!Game)
	{
		return TEXT("no game");
	}
	ss::Session& S = Game->Session;
	// As ss.Rent.Next: not under an open table, or in a time skip (it sets the clock every frame from where it began).
	if (S.T)
	{
		return TEXT("Not with a table open: the clock stays.");
	}
	if (S.TimeSkip.Active)
	{
		return TEXT("Not during a time skip: the clock stays.");
	}
	const double Now = S.WorldMinutes();
	double Ahead = static_cast<double>(FMath::Clamp(Hour, 0.0f, 24.0f)) * 60.0 - FMath::Fmod(Now, 1440.0);
	if (Ahead < 1.0)
	{
		Ahead += 1440.0;
	}
	S.LobbyMinutes += Ahead;
	return FString::Printf(TEXT("The clock moves %.1f hours ahead. %s"), Ahead / 60.0, *DescribeHub());
}

FString ANightOneGameMode::DescribeHub() const
{
	const double World = HubWorldMinutes();
	const int32 M = FMath::FloorToInt(FMath::Fmod(FMath::Max(0.0, World), 1440.0));
	const int32 H = M / 60;
	const bool bCareerEvicted = Game && Game->Session.Evicted();
	return FString::Printf(TEXT("Hub: %s%s | %d:%02d %s%s, daylight %.2f | the career %s | Dee %s"), bAtDees ? TEXT("Dee's front room") : TEXT("1812, the apartment"),
		GHubEvicted >= 0 ? TEXT(" (ss.Home.Evicted)") : TEXT(""), H % 12 == 0 ? 12 : H % 12, M % 60, H < 12 ? TEXT("AM") : TEXT("PM"),
		GHubHour >= 0.0f ? TEXT(" (ss.Home.Hour)") : TEXT(""), ss::life::Daylight(World), bCareerEvicted ? TEXT("evicted") : TEXT("housed"),
		DeeRoom::WhereName(DeeRoom::WhereAt(World)));
}

// ------------------------------------------------------------------ the hub's room

bool ANightOneGameMode::HubEvicted() const
{
	return GHubEvicted >= 0 ? GHubEvicted == 1 : (Game && Game->Session.Evicted());
}

double ANightOneGameMode::HubWorldMinutes() const
{
	const double World = Game ? Game->Session.WorldMinutes() : 0.0;
	if (GHubHour < 0.0f)
	{
		return World;
	}
	return FMath::FloorToDouble(World / 1440.0) * 1440.0 + static_cast<double>(GHubHour) * 60.0;
}

ADeeRoomStage* ANightOneGameMode::DeeStage() const
{
	return bAtDees ? Cast<ADeeRoomStage>(Stage) : nullptr;
}

void ANightOneGameMode::UseRoom(bool bDees)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	// One room in the level at a time: the one wanted (the apartment placed in the level, or Dee's built now), the other
	// gone. An apartment saved in the level before its construction last changed (a cooked map not saved since) is built
	// anew rather than played half-built. Should the wanted room fail to build, the one there is stays.
	ANightOneStage* Keep = nullptr;
	TArray<ANightOneStage*> Stages;
	for (TActorIterator<ANightOneStage> It(World); It; ++It)
	{
		ANightOneStage* S = *It;
		Stages.Add(S);
		if (!Keep && S->IsA<ADeeRoomStage>() == bDees && S->IsSetCurrent())
		{
			Keep = S;
		}
	}
	if (!Keep)
	{
		Keep = World->SpawnActor<ANightOneStage>(bDees ? ADeeRoomStage::StaticClass() : ANightOneStage::StaticClass(), FTransform::Identity);
	}
	if (!Keep)
	{
		UE_LOG(LogNightOne, Warning, TEXT("%s couldn't be built: the hub stays where it is"), bDees ? TEXT("Dee's front room") : TEXT("The apartment"));
		Keep = IsValid(Stage) ? Stage.Get() : (Stages.Num() > 0 ? Stages[0] : nullptr);
	}
	for (ANightOneStage* S : Stages)
	{
		if (S != Keep)
		{
			S->Destroy();
		}
	}
	Stage = Keep;
	bAtDees = Keep && Keep->IsA<ADeeRoomStage>();
	bDeeKnown = false;
	PendingArrival = DeeRoom::FLine();
	WokeAt = -1.0;
	BindStage();
	UE_LOG(LogNightOne, Log, TEXT("The hub is %s"), bAtDees ? TEXT("Dee's front room") : TEXT("the apartment"));
}

void ANightOneGameMode::BindStage()
{
	if (!Stage)
	{
		return;
	}
	TWeakObjectPtr<UNightOneAudio> WeakAudio = Audio.Get();
	Stage->OnThunder = [WeakAudio](float Delay, float Strength) {
		if (WeakAudio.IsValid())
		{
			WeakAudio->Thunder(Delay, Strength);
		}
	};
	if (Game)
	{
		ApplyStageSettings(Game->Menu.Settings);
	}
	if (ANightOnePawn* Seat = GetSeat())
	{
		Seat->Configure(Stage->EyeLocation(), Stage->ScreenCenter(), Stage->ScreenNormal(), Stage->ScreenSize());
	}
	// The locks' dressing is the new room's from its first frame; Dee's room has the player's boxes by the couch.
	EvictedShown = -1;
	if (ADeeRoomStage* Room = DeeStage())
	{
		Room->SetBoxes(true);
	}
	// The room's sound: her radiator and her TV over the rain, or the apartment's own hum.
	if (Audio)
	{
		if (bAtDees)
		{
			RoomTone = MakeShared<FDeeRoomTone>(11u);
			TSharedPtr<FDeeRoomTone> Tone = RoomTone;
			Audio->SetAmbienceSource([Tone](float* Out, int32 Count) { Tone->Render(Out, Count); });
		}
		else if (RoomTone)
		{
			RoomTone.Reset();
			Audio->SetAmbienceSource(nullptr);
		}
	}
}

bool ANightOneGameMode::IsSleeping() const
{
	if (!Game || !Game->Session.TimeSkip.Active)
	{
		return false;
	}
	const ss::life::Activity* Doing = ss::life::Find(Game->Session.TimeSkip.Result.ActivityId);
	return Doing && Doing->Type == ss::life::Kind::Sleep;
}

void ANightOneGameMode::TickLockedOut()
{
	if (!Game)
	{
		return;
	}
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	APlayerCameraManager* Camera = PC ? PC->PlayerCameraManager.Get() : nullptr;
	// The room has gone dark: Dee's front room comes up.
	if (MoveSwapAt >= 0.0)
	{
		if (bLeaving)
		{
			MoveSwapAt = -1.0; // out of the door instead: the next load has the player at hers
			return;
		}
		if (RealTime < MoveSwapAt)
		{
			return;
		}
		MoveSwapAt = -1.0;
		if (!bAtDees && HubEvicted())
		{
			UseRoom(true);
			// In the door as from the street, with the laptop under an arm: she says hello once the camera's settled.
			ArrivedWith.Reset();
			DeeHelloAt = RealTime + 2.8;
		}
		if (Camera)
		{
			Camera->StartCameraFade(1.0f, 0.0f, 1.8f, FLinearColor::Black, true, false);
		}
		return;
	}
	if (MoveToDeesAt < 0.0 || RealTime < MoveToDeesAt)
	{
		return;
	}
	if (bAtDees || !HubEvicted())
	{
		MoveToDeesAt = -1.0;
		return;
	}
	// Not while anything holds the player at the laptop: a table or the results of one, a shift or a night's sleep or its
	// card, a menu, the eyes still opening.
	const ss::Session& S = Game->Session;
	const bool bAtTable = S.T || S.TableCount() > 0 || S.CurrentScreen == ss::Screen::Table || S.CurrentScreen == ss::Screen::Results;
	if (!bStarted || bLeaving || IsMenuOpen() || bAtTable || S.TimeSkip.Active || S.HasOutcome || SleepFade > 0.0f)
	{
		return;
	}
	// The coat on, the laptop under an arm, out of 1812 and across the street.
	MoveToDeesAt = -1.0;
	MoveSwapAt = RealTime + 1.4;
	if (Camera)
	{
		Camera->StartCameraFade(0.0f, 1.0f, 1.3f, FLinearColor::Black, true, true);
	}
	if (Audio)
	{
		Audio->PlayEffect(ss::audio::Effect::Scrape, 0.7f);
	}
}

void ANightOneGameMode::SayDee(const DeeRoom::FLine& Line)
{
	if (Line.IsEmpty() || !Game)
	{
		return;
	}
	switch (Line.Kind)
	{
	case DeeRoom::FLine::Spoken:
		if (ADeeRoomStage* Room = DeeStage())
		{
			Room->DeeSay(Line.Text);
		}
		if (Caption)
		{
			Caption->Show(TEXT("Dee"), Line.Text);
		}
		break;
	case DeeRoom::FLine::Narration:
		if (Caption)
		{
			Caption->Show(FString(), Line.Text);
		}
		break;
	case DeeRoom::FLine::Texted:
		Game->Text("Dee", std::string(TCHAR_TO_UTF8(*Line.Text)));
		break;
	case DeeRoom::FLine::None:
		break;
	}
}

void ANightOneGameMode::DeeDoor()
{
	ADeeRoomStage* Room = DeeStage();
	if (Room && Audio)
	{
		const FVector At = Room->DoorLocation();
		Audio->PlaySamples(FDeeRoomTone::RenderDoor(DoorSeed++), 0.6f, &At);
	}
}

void ANightOneGameMode::TickDeeRoom()
{
	ADeeRoomStage* Room = DeeStage();
	if (!Room || !Game)
	{
		return;
	}
	ss::Session& S = Game->Session;
	const double World = HubWorldMinutes();
	const DeeRoom::EWhere Where = DeeRoom::WhereAt(World);
	// While the hours race (a shift, a night on the couch) she comes and goes unseen and says nothing.
	const bool bSkipping = S.TimeSkip.Active;
	if (!bDeeKnown)
	{
		bDeeKnown = true;
		DeeWhere = Where;
		Room->SetDee(Where, true);
		// The note by the laptop says where she is.
		Room->SetNote(DeeRoom::Note(Where, World));
	}
	else if (Where != DeeWhere)
	{
		const DeeRoom::EWhere Was = DeeWhere;
		DeeWhere = Where;
		Room->SetDee(Where, bSkipping || !bStarted);
		Room->SetNote(DeeRoom::Note(Where, World));
		PendingArrival = DeeRoom::FLine();
		if (bStarted && !bSkipping)
		{
			const DeeRoom::FLine Line = DeeRoom::Change(Was, Where, World);
			if (Where == DeeRoom::EWhere::Home)
			{
				PendingArrival = Line; // once she's in her chair
			}
			else if (Was != DeeRoom::EWhere::Home || Room->IsDeeHere())
			{
				// Said as she gets up to go (she's still in her chair), or as she passes through (up the stairs to bed,
				// out for the day): her door, then. (Due home but never sat down, the player having had the chair in view
				// all along, she isn't coming in now and has nothing to say on her way.)
				SayDee(Line);
				if (Was != DeeRoom::EWhere::Home)
				{
					DeeDoor();
				}
			}
		}
	}
	if (Room->ConsumeDeeArrived())
	{
		DeeDoor();
		// To a player asleep on the couch she comes in quietly.
		if (!bSkipping)
		{
			SayDee(PendingArrival);
		}
		PendingArrival = DeeRoom::FLine();
	}
	if (Room->ConsumeDeeLeft())
	{
		DeeDoor();
	}
	// The room's sound has her TV, and the radiator's heat for the hour.
	if (RoomTone)
	{
		RoomTone->SetTv(Room->IsTvOn());
		RoomTone->SetHeat(1.0f - 0.65f * static_cast<float>(ss::life::Daylight(World)));
	}
	// As the player comes in (the camera settled): what she says, texts, or the room tells them. Home but not sat down
	// yet (still loading in), she's given a moment more.
	const bool bWaitForHer = DeeWhere == DeeRoom::EWhere::Home && !Room->IsDeeHere() && RealTime < DeeHelloAt + 8.0;
	if (DeeHelloAt >= 0.0 && RealTime >= DeeHelloAt && bSkipping)
	{
		// Straight down for a nap or onto a shift before she'd said a word: she lets it go.
		DeeHelloAt = -1.0;
		ArrivedWith.Reset();
	}
	if (DeeHelloAt >= 0.0 && RealTime >= DeeHelloAt && !bWaitForHer)
	{
		DeeHelloAt = -1.0;
		const DeeRoom::EArrival From = DeeRoom::ArrivalFrom(ArrivedWith);
		// The first night's welcome is said once (back from the corner store an hour later, the couch isn't made up again).
		const bool bWelcome = DeeRoom::FirstNight(S, World) && GWelcomedFor != S.Life.EvictedAt;
		DeeRoom::FLine Hello = DeeRoom::Arrival(DeeWhere, From, ArrivedWith, World, bWelcome);
		if (DeeWhere == DeeRoom::EWhere::Home && (From == DeeRoom::EArrival::BackRoom || From == DeeRoom::EArrival::Embercrest))
		{
			// Sitting right there, she says what she'd have texted about the night (her game's words as they are), to
			// the player's face, and doesn't text it.
			const FString Night = From == DeeRoom::EArrival::BackRoom ? HomeTextFromOptions(HomeOptions, S) : FString();
			if (!Night.IsEmpty())
			{
				Hello.Kind = DeeRoom::FLine::Spoken;
				Hello.Text = Night;
				Hello.bWelcome = false;
			}
			if (Hello.Kind == DeeRoom::FLine::Spoken)
			{
				HomeTextAt = -1.0;
			}
		}
		// Her game's on downstairs: she texts it once a night, not each time the player comes back up (the night is the
		// evening's until her last hand at five).
		if (DeeWhere == DeeRoom::EWhere::Game && Hello.Kind == DeeRoom::FLine::Texted)
		{
			const int64 Tonight = static_cast<int64>(FMath::FloorToDouble((World - 5.0 * 60.0) / 1440.0));
			if (Tonight == GGameTextNight)
			{
				Hello = DeeRoom::FLine();
			}
			GGameTextNight = Tonight;
		}
		if (Hello.bWelcome && !Hello.IsEmpty())
		{
			GWelcomedFor = S.Life.EvictedAt;
		}
		SayDee(Hello);
		ArrivedWith.Reset();
	}
	// Up off the couch with her home: a word about it.
	if (WokeAt >= 0.0 && RealTime >= WokeAt)
	{
		WokeAt = -1.0;
		if (DeeWhere == DeeRoom::EWhere::Home && Room->IsDeeHere())
		{
			SayDee(DeeRoom::Woke(World));
		}
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
	// What's said in the room (Dee, at hers), over the overlay and under the menus.
	Caption = SNew(SDeeRoomCaption);
	Viewport->AddViewportWidgetContent(Caption.ToSharedRef(), 15);

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

void ANightOneGameMode::StartNewCareer(const FString& ScreenName, const ss::hero::Character* Who)
{
	if (Game)
	{
		// The screen name first: the new career's living world is made (and seeded) for whoever it's about, and it knows
		// the player by it from the first night's results on.
		const FString Clean = SanitizeName(ScreenName);
		if (Clean.Len() >= 3)
		{
			Game->Session.HeroName = std::string(TCHAR_TO_UTF8(*Clean));
		}
		if (Who)
		{
			// A fresh save, then what the character's background starts them with.
			Game->Session.NewCareer(*Who);
		}
		else
		{
			Game->Session.ResetSave();
		}
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
		// A stream still live ends here (credited), as leaving the desk any other way ends it.
		Game->Session.EndStream();
		Game->Session.Save();
	}
	SaveSettingsNow();
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)), true, TEXT("Menu"));
}

void ANightOneGameMode::QuitToDesktop()
{
	if (Game && bStarted)
	{
		Game->Session.EndStream();
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
	ApplyStageSettings(NewSettings);
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

void ANightOneGameMode::ApplyStageSettings(const ss::ui::GameSettings& NewSettings)
{
	if (Stage)
	{
		Stage->BrightnessBias = static_cast<float>(NewSettings.Brightness - 50) / 50.0f * 1.5f;
		Stage->bMotionBlur = NewSettings.MotionBlur;
		Stage->GrainScale = NewSettings.FilmGrain == 0 ? 0.0f : NewSettings.FilmGrain == 1 ? 1.0f : 2.2f;
		Stage->FringeScale = NewSettings.ChromaticAberration ? 1.0f : 0.0f;
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
	// The room as this career has it from the first frame (a new career's may differ from the title screen's): no door.
	EvictedShown = -1;
	const FString Clean = SanitizeName(Name);
	if (Clean.Len() >= 3)
	{
		Game->Session.HeroName = std::string(TCHAR_TO_UTF8(*Clean));
		// The living world names the player in its results (a new career's was made with the name already).
		Game->Session.Living().HeroName = Game->Session.HeroName;
	}
	Game->Session.Save();
	ReturnInputToGame();
	UE_LOG(LogNightOne, Log, TEXT("Night One started as %s"), *Clean);
	// The career the title screen chose decides the room (a new one isn't evicted: back to 1812, as the fly-in starts).
	if (HubEvicted() != bAtDees)
	{
		UseRoom(HubEvicted());
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (PC->PlayerCameraManager)
			{
				PC->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 1.2f, FLinearColor::Black, true, false);
			}
		}
	}
	if (bAtDees)
	{
		// Coming in at Dee's: she says hello (or texts it, or isn't there), once the camera has settled on the couch.
		DeeHelloAt = RealTime + 3.4;
	}
	// The game closed during a night at the Embercrest: straight back to the seat.
	if (!Game->Session.LiveInProgress().empty())
	{
		UE_LOG(LogNightOne, Log, TEXT("Back to %s, in progress"), UTF8_TO_TCHAR(Game->Session.LiveInProgress().c_str()));
		Game->Session.ResumeLive();
	}
}

bool ANightOneGameMode::GoOutside()
{
	if (bLeaving || !bStarted || !Game)
	{
		return false;
	}
	const ss::Session& S = Game->Session;
	if (S.TableCount() > 0 || S.TimeSkip.Active)
	{
		// Not with tables running: the tournament plays on without you, and the blinds don't wait.
		Game->Text("RiverLine", "You have tables open. Finish or unregister before you head out.");
		return false;
	}
	// Live on Kast: the stream ends as the player gets up, as it does on every other way out, so its hours, viewers and
	// payout are credited (an unended stream isn't saved: it would just vanish with the level).
	if (Game->Session.Streaming())
	{
		Game->Session.EndStream();
	}
	// Coat on, down the stairs, out onto Fifth (the Street level).
	Game->Session.Save();
	bLeaving = true;
	LeaveAt = RealTime + 1.4;
	LeaveFor = TEXT("street");
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, 1.3f, FLinearColor::Black, true, true);
		}
	}
	if (Audio)
	{
		Audio->PlayEffect(ss::audio::Effect::Scrape, 0.7f);
	}
	return true;
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
	// The closet's widget leaves with the level (its page with it), and the console lets go of the career.
	if (NightOneCloset::GCloset.Owner.Get() == this || !NightOneCloset::GCloset.Owner.IsValid())
	{
		NightOneCloset::Close(true);
	}
	CastLooks::BindSession(this, nullptr);
	// The save being written (and any waiting) lands before the next scene reads it.
	if (Game)
	{
		if (bStarted)
		{
			Game->Session.Save();
		}
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
			if (Caption)
			{
				Viewport->RemoveViewportWidgetContent(Caption.ToSharedRef());
			}
			if (MenuRoot)
			{
				Viewport->RemoveViewportWidgetContent(MenuRoot.ToSharedRef());
			}
		}
	}
	Overlay.Reset();
	Caption.Reset();
	MenuRoot.Reset();
	MenuBlur.Reset();
	MenuWidget.Reset();
	RoomTone.Reset();
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
	// The closet over the room while it's open; walked away from, the input goes back to the room.
	if (NightOneCloset::Tick(this, Game->Measurer, RealTime, [this](const FString& From, const FString& Body) { ShowToast(From, Body); }))
	{
		ReturnInputToGame();
	}
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
		if (LeaveFor == TEXT("street"))
		{
			// From Dee's the player comes down her stairs and out of her door across the street (Door=Dee: the street
			// puts them there while evicted; it says so even when the test hook, not the career, put them at hers).
			UGameplayStatics::OpenLevel(this, FName(TEXT("Street")), true,
				FString::Printf(TEXT("From=%.0f%s"), Game->Session.WorldMinutes(), bAtDees ? TEXT("?Door=Dee") : TEXT("")));
			return;
		}
		// The Back Room for Dee's game; the same map turns into the Embercrest's card room for a tournament (the event
		// the session registered the player for).
		const std::string LeaveId(TCHAR_TO_UTF8(*LeaveFor));
		const bool bEmbercrest = ss::live::IsEmbercrest(LeaveId);
		const FString Options = bEmbercrest ? FString::Printf(TEXT("Live=%s"), *LeaveFor) : FString::Printf(TEXT("BuyIn=%lld"), LeaveBuyInCents);
		// To the Embercrest the 14 brings the player up the ridge to the resort's doors (the exterior, where it's been
		// built), unless the night is already under way: back after a crash in the room, straight to the seat as before.
		const ss::life::LiveEntry* Entry = bEmbercrest ? ss::live::EntryFor(Game->Session.Life, LeaveId) : nullptr;
		const bool bBackInRoom = Entry && Entry->ArrivedAt > 0.0;
		if (bEmbercrest && !bBackInRoom && FPackageName::DoesPackageExist(TEXT("/Game/Maps/Embercrest")))
		{
			UGameplayStatics::OpenLevel(this, FName(TEXT("Embercrest")), true, Options);
			return;
		}
		UGameplayStatics::OpenLevel(this, FName(TEXT("BackRoom")), true, Options);
		return;
	}
	// At Dee's the text waits for her hello: she may be sitting right there and say it instead.
	if (HomeTextAt >= 0.0 && RealTime >= HomeTextAt && !(bAtDees && DeeHelloAt >= 0.0))
	{
		HomeTextAt = -1.0;
		const FString HomeText = HomeTextFromOptions(HomeOptions, Game->Session);
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
	if (ArrivalSwapUntil >= 0.0)
	{
		if (RealTime >= ArrivalSwapUntil)
		{
			ArrivalSwapUntil = -1.0;
		}
		else if (HubEvicted() && !bAtDees)
		{
			ArrivalSwapUntil = -1.0;
			UseRoom(true);
			DeeHelloAt = RealTime + 3.0;
		}
	}
	TickLockedOut();

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

	// The title screen frames the room with a slow establishing shot; starting play flies the camera to the seat. Asleep
	// where the room has a bed for it (Dee's couch), the camera lies down there, and gets up again after.
	const bool bSleeping = IsSleeping();
	if (bSleeping)
	{
		CouchShotUntil = RealTime + 4.0;
	}
	else if (bWasSleeping)
	{
		WokeAt = RealTime + 2.6;
	}
	bWasSleeping = bSleeping;
	bool bCouchShot = false;
	if (Seat)
	{
		FVector ShotPos;
		FVector ShotLook;
		bCouchShot = !Game->Menu.WantsEstablishingShot() && RealTime < CouchShotUntil && Stage->SleepShot(RealTime, ShotPos, ShotLook);
		if (bCouchShot)
		{
			Seat->SetEstablishingShot(ShotPos, ShotLook, bSleeping);
		}
		else
		{
			Stage->MenuShot(RealTime, ShotPos, ShotLook);
			Seat->SetEstablishingShot(ShotPos, ShotLook, Game->Menu.WantsEstablishingShot());
		}
	}
	// Lying down, the eyes close most of the way: the room's light through them as the hours go by.
	{
		const float Want = bCouchShot && bSleeping ? 0.62f : 0.0f;
		const float Was = SleepFade;
		SleepFade = FMath::FInterpConstantTo(SleepFade, Want, static_cast<float>(Dt), 0.45f);
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		if (PC && PC->PlayerCameraManager && !bLeaving && (SleepFade > 0.0f || Was > 0.0f))
		{
			if (SleepFade > 0.0f)
			{
				PC->PlayerCameraManager->SetManualCameraFade(SleepFade, FLinearColor::Black, false);
			}
			else
			{
				PC->PlayerCameraManager->StopCameraFade();
			}
		}
	}

	// The player's hands follow the head and the pointer; they sit out the establishing shot (and a night on the couch).
	if (Seat)
	{
		Stage->UpdateArms(static_cast<float>(Dt), Seat->GetActorLocation(), Seat->Camera ? Seat->Camera->GetForwardVector() : Seat->GetActorForwardVector(), ArmsPointer,
		                  bStarted && !Game->Menu.WantsEstablishingShot() && !bCouchShot);
	}

	// The world reacts to the game.
	const double Clock = S.ClockMinutes();
	// Daylight from the time of day (the hub's: ss.Home.Hour can set it): dawn after a long night, the afternoon after a
	// day shift, dusk again.
	Stage->SetDawn(static_cast<float>(ss::life::Daylight(HubWorldMinutes())));
	const float PhoneLevel = static_cast<float>(Game->Phone.Brightness(RealTime));
	Stage->SetPhoneBrightness(PhoneLevel);
	PhoneAccum += Dt;
	if (PhoneLevel > 0.0f && PhoneAccum >= 0.1)
	{
		PhoneAccum = 0.0;
		TSharedPtr<ss::ui::DrawList> List = PhoneHint.Make();
		ss::ui::Canvas Cv(*List, Game->Measurer, ss::ui::PhoneScreen::Width, ss::ui::PhoneScreen::Height, 1.0f);
		Game->Phone.Draw(Cv, Clock, S.WorldMinutes());
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
	// The GearDrop LED kit: the room in the chosen colour, flashing with the stream when synced. Evicted, the kit is in
	// the storage unit with the rest of the rig: dark. (The hub's eviction: ss.Home.Evicted can stand in for the career's.)
	// At Dee's none of it is in the room, even once the key's back (it's 1812's again on the next load).
	{
		const bool bStored = HubEvicted() || bAtDees;
		const ss::gear::Glow Leds = S.RoomGlow(GameTime);
		const bool bLedsOn = Leds.On && !bStored;
		const FLinearColor LedColor = FLinearColor::FromSRGBColor(FColor(static_cast<uint8>((Leds.Rgb >> 16) & 0xff), static_cast<uint8>((Leds.Rgb >> 8) & 0xff), static_cast<uint8>(Leds.Rgb & 0xff)));
		Stage->SetRoomLights(bLedsOn, LedColor, static_cast<float>(Leds.Level));
		// The locks changing (or the key back) while the player is in the room: the door, heard; the room changes with it.
		// At Dee's, the key back (the Bank's Move back in) is hers to hear about: 1812 is the hub again the next time the
		// level opens, the desk refilled from storage.
		const int32 EvictedNow = S.Evicted() ? 1 : 0;
		if (EvictedShown >= 0 && EvictedShown != EvictedNow)
		{
			if (bAtDees)
			{
				if (EvictedNow == 0)
				{
					SayDee(DeeRoom::KeyBack(DeeWhere));
				}
			}
			else
			{
				if (Audio)
				{
					Audio->PlayEffect(ss::audio::Effect::Scrape, EvictedNow ? 0.9f : 0.6f);
				}
				// Locked out at the desk: the cartons and the texts first, then over to Dee's couch (TickLockedOut).
				MoveToDeesAt = EvictedNow ? RealTime + 6.0 : -1.0;
			}
		}
		EvictedShown = EvictedNow;
		// What the player owns, set up in the room, and what their career has left on the windowsill. The PC's RGB
		// follows the kit (or cycles through the rainbow on its own); the streaming lights come on while live.
		// Evicted, the gear is in a storage unit: the desk is down to the laptop (GearFx already is), what's left of the
		// room is packed in boxes, and it isn't theirs any more.
		const ss::gear::Effects& Fx = S.GearFx();
		FRoomGear Room;
		Room.bEvicted = bStored;
		Room.bMonitor = !bStored && S.Owns("monitor-24");
		Room.bMonitorWide = !bStored && S.Owns("monitor-27");
		Room.Towers = Fx.PcTier >= 3 ? 2 : (Fx.PcTier >= 2 ? 1 : 0);
		Room.Cam = Fx.CamTier;
		Room.Mic = Fx.MicTier;
		Room.Lights = Fx.Lights;
		Room.bMacroPad = Fx.MacroPad;
		Room.bHeadphones = !bStored && S.Owns("headphones");
		Room.bPlant = !bStored && S.Owns("plant");
		Room.bCurtains = !bStored && S.Owns("curtains");
		Room.bRouter = Fx.Fiber;
		// The Embercrest's Sunday cup on the sill, unless it's in Augie's cage or sold (spec 5.8).
		Room.bTrophy = S.Life.LiveBestPlace == 1 && !S.PawnHolds("memento:cup");
		Room.bDeeChip = S.Life.BackRoomNetCents > 0;
		Stage->SetGear(Room);
		const FLinearColor Rgb = bLedsOn ? LedColor : FLinearColor::MakeFromHSV8(static_cast<uint8>(FMath::Fmod(RealTime * 12.0, 256.0)), 190, 255);
		Stage->SetGearGlow(Rgb, bLedsOn ? static_cast<float>(Leds.Level) : 1.0f, S.Streaming() && !bStored);
	}
	// Dee's front room: Dee, her TV and her radiator, the note by the laptop, what she says.
	if (bAtDees)
	{
		TickDeeRoom();
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
	// Not while the coat's going on (the leave fade): a click then would land on the laptop of a scene that's ending.
	if (!bStarted || !Game || !Seat || IsMenuOpen() || bLeaving)
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
	if (Game && !IsMenuOpen() && !bLeaving)
	{
		Game->Client.UI.Ptr.Wheel += Delta;
	}
}

void ANightOneGameMode::OnKey(const FString& Key)
{
	if (!bStarted || !Game || IsMenuOpen() || bLeaving)
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
	if (Key == TEXT("g"))
	{
		GoOutside();
		return;
	}
	// The closet (spec 7.6): C while leaned back from the laptop and no hand to act in (leaned in at a table, C calls).
	if (Key == TEXT("c") && IsLeanedBack() && Game->Session.CurrentScreen != ss::Screen::Table)
	{
		if (NightOneCloset::Open(this, Game->Session, RealTime, bAtDees ? TEXT("dee") : TEXT("apartment")))
		{
			return;
		}
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
