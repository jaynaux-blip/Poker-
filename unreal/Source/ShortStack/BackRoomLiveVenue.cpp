// The Embercrest's card room as a venue (ABackRoomGameMode's live tournaments; the flow is BackRoomLive.cpp): the desk
// writing the player in (the world's field on a worker, then the entry), what's in reach on foot and what it says, the
// boards by the entrance and over the stage, and the next event tonight: offered, registered, called, opened.
//
// The next segment's venue (ue-venue; next_segment_spec 3.10, 5.7, 6.5, 6.6): the desk and the cage as one modal screen
// (ss::ui::DeskScreen: who pays, the Players Club card that prints on an upgrade, Meet the regulars, the seat card, the
// refund, the cage's receipt), carried out through DeskSignup, FinishSignup and LiveUnregister; the room's clock screens
// (ss::ui::DrawRoomClock into render targets about twice a second: CLOCK on the stage's, CLOCK then PAYOUTS, TODAY or
// LEADERS on the side walls), fed by live::Clock like the HUD's clock card; the badge atlas; the rail's prompt and the
// seat marker; Team Embercrest's banner, the crest on the champions' board and the bar's plate; the colour-up call at a
// scheduled break when no director speaks for the room.

#include "BackRoomLiveShared.h"

#include "BackRoomGameMode.h"
#include "BackRoomPlayerController.h"
#include "BackRoomStage.h"
#include "BackRoomTable.h"
#include "BadgeAtlas.h"
#include "CareerSave.h"
#include "CoreCanvasTarget.h"
#include "SlateDrawList.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Console.h"
#include "Engine/GameViewportClient.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Input/Events.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "NightOneAudio.h"
#include "Tasks/Task.h"
#include "TimerManager.h"
#include "Widgets/SViewport.h"
#include "ShortStack/Audio/Synth.h"
#include "ShortStack/Game/Badges.h"
#include "ShortStack/Game/ChipSet.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Room.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/Stake.h"
#include "ShortStack/Game/TableTalk.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Tournament.h"
#include "ShortStack/UI/BadgeArt.h"
#include "ShortStack/UI/Canvas.h"
#include "ShortStack/UI/SponsorArt.h"
#include "ShortStack/UI/TableHud.h"

#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace LiveDetail
{
/** The next event's seats open this long before its cards (world minutes): the floor calls them then. */
constexpr double SeatingLead = 10.0;
/** The desk stops writing players in this close to the end of late registration (there's the walk to the seat). */
constexpr double DeskClosesBefore = 5.0;
} // namespace LiveDetail

using namespace LiveDetail;

// ------------------------------------------------------------------ the venue's helpers (ue-venue)

namespace VenueDetail
{
/**
 * A word from the room's staff to the player (the desk, the cashier at the cage, the bartender, the floor at the door) or
 * the terrace's narration (no speaker), through the room (ABackRoomGameMode::PlayCue, ue-room's): heard from where it's
 * said (in what ships, a caption over a breath; narration has no voice), captioned in the core HUD and kept in the night
 * log. With no room host yet (practice, before the field sits) PlayCue gives it the table's old announcement.
 */
void StaffSays(ABackRoomGameMode& GM, const FString& Speaker, const FString& Text, int32 Tick)
{
	ss::room::Cue C;
	C.What = ss::room::Kind::Line;
	C.Where = Speaker == TEXT("Bartender") ? ss::room::From::Bar : ss::room::From::Desk;
	C.Speaker = TCHAR_TO_UTF8(*Speaker);
	C.Key = Speaker.IsEmpty() ? "host.narration" : Speaker == TEXT("Bartender") ? "host.bar" : Speaker == TEXT("Cashier") ? "host.cage" : "host.desk";
	C.Text = TCHAR_TO_UTF8(*Text);
	C.ToHero = true;
	C.Priority = 2;
	C.Tick = Tick;
	GM.PlayCue(C);
}

TAutoConsoleVariable<float> CVarScreenGlow(TEXT("ss.Venue.ScreenGlow"), 1.9f,
	TEXT("The card room's clock screens: how bright their pages read (M_ECX_Screen's Strength; the old text screens' was 1.9)."));
TAutoConsoleVariable<float> CVarPrintGlow(TEXT("ss.Venue.PrintGlow"), 1.15f,
	TEXT("Team Embercrest's banner by the desk and the crest on the champions' board: how bright their print reads in the room."));
TAutoConsoleVariable<int32> CVarClockPage(TEXT("ss.Venue.ClockPage"), -1,
	TEXT("The side walls' clock screens: -1 rotates (CLOCK 40 s, then PAYOUTS, TODAY or LEADERS 10 s in turn); 0 CLOCK, 1 PAYOUTS, 2 TODAY, 3 LEADERS held."));
TAutoConsoleVariable<int32> CVarClockScreens(TEXT("ss.Venue.ClockScreens"), 1,
	TEXT("1: the card room's clocks are core pages in render targets (next_segment_spec 6.5); 0: the old six text lines (read as the room opens)."));
TAutoConsoleVariable<int32> CVarAmbassador(TEXT("ss.Venue.Ambassador"), 0,
	TEXT("1: the room dresses for Team Embercrest's ambassador (the banner by the desk, the crest on the champions' board) without a contract: the art only; the bar's plate stays the contract's."));

/** The clock pages' size (spec 10: 1,024 x 576, two targets), the banner's (1:2) and the crest's. */
constexpr int32 ClockW = 1024;
constexpr int32 ClockH = 576;
constexpr int32 BannerW = 512;
constexpr int32 BannerH = 1024;
constexpr int32 CrestPx = 128;
/** The side screens' rotation (spec 5.6): CLOCK this long, then one other page this long. */
constexpr float ClockSeconds = 40.0f;
constexpr float PageSeconds = 10.0f;
/** A clock target painted every this long, the two in turn: each about twice a second (spec 6.5, 10). */
constexpr float RenderEvery = 0.25f;
/** The cage's receipt: the lines the settlement wrote (the prize, a sponsor's cut or bonus, a backer's share). */
constexpr int32 LedgerPrize = 4;

std::string Utf8(const FString& S)
{
	return std::string(TCHAR_TO_UTF8(*S));
}

std::string Upper(std::string S)
{
	for (char& Ch : S)
	{
		if (Ch >= 'a' && Ch <= 'z')
		{
			Ch = static_cast<char>(Ch - 'a' + 'A');
		}
	}
	return S;
}

std::string Price(ss::Chips Cents)
{
	return "$" + ss::Grouped(Cents / 100);
}

/** How the room calls the player: the person's name (the creator's), or the screen name for a career from before it. */
FString HeroCalled(const ss::SaveData& D)
{
	const std::string Full = D.Person.Created ? D.Person.FullName() : std::string();
	return Str(Full.empty() ? D.HeroName : Full);
}

/** Why the desk won't sell O now, whoever pays ("" when it would): registration closing or not open yet, or no legs for it. */
FString EventWhyNot(const ss::live::Occurrence& O, double Minutes, float Energy)
{
	const FString Short = Str(O.T->Short);
	if (Minutes + DeskClosesBefore > O.LateRegEnds)
	{
		return FString::Printf(TEXT("Registration for the %s is closing. There isn't time to get you to a seat, sorry."), *Short);
	}
	if (Minutes < O.DeskOpens)
	{
		return FString::Printf(TEXT("Registration for the %s opens at %s. Come see me then."), *Short, *Str(ss::net::TimeLabel(O.DeskOpens)));
	}
	// What the desk sells is the entry (the night's bus home is paid), and a long night needs the legs for it, as at home.
	const ss::life::Activity* Trip = ss::life::Find("embercrest");
	if (Trip && Energy < Trip->Energy)
	{
		return TEXT("A tournament's a long night, hon, and you look done in. Go home and get some sleep.");
	}
	return FString();
}

/**
 * Who could pay for O at the desk, each with why not ("" when it can): stake::FundingOptions as GoToLive builds them,
 * then the clerk's own check (live::CanRegister) on each that would, so money the player hasn't got greys out with the
 * reason. Both at the desk's booking time (the trip there is made: FinishSignup registers at Minutes less the bus), with
 * the night's two fares waived as the desk does, so what's drawn is exactly what live::Register is asked to take.
 */
std::vector<std::pair<ss::life::Funding, std::string>> DeskPay(const ss::SaveData& D, const ss::live::Occurrence& O, double Minutes)
{
	const double Booked = Minutes - ss::live::TravelMinutes;
	std::vector<std::pair<ss::life::Funding, std::string>> Pay = ss::stake::FundingOptions(D.Life, O, Booked);
	const ss::Chips Bank = D.BankrollCents + 2 * ss::live::BusFareCents;
	for (std::pair<ss::life::Funding, std::string>& P : Pay)
	{
		if (P.second.empty())
		{
			P.second = ss::live::CanRegister(Bank, D.Life, O, Booked, P.first);
		}
	}
	return Pay;
}

bool AnyPays(const std::vector<std::pair<ss::life::Funding, std::string>>& Pay)
{
	return std::any_of(Pay.begin(), Pay.end(), [](const std::pair<ss::life::Funding, std::string>& P) { return P.second.empty(); });
}

/** The ambassador's record on the banner: "Two-time Embercrest champion · 14 cashes". */
std::string AmbassadorRecord(const ss::life::State& L)
{
	const std::string Cashes = L.LiveCashes == 1 ? std::string("1 cash") : std::to_string(L.LiveCashes) + " cashes";
	if (L.LiveWins >= 2)
	{
		return ss::talk::Capitalized(ss::talk::NumberWords(L.LiveWins)) + "-time Embercrest champion \xC2\xB7 " + Cashes;
	}
	if (L.LiveWins == 1)
	{
		return "Embercrest champion \xC2\xB7 " + Cashes;
	}
	if (L.LiveCashes > 0)
	{
		return Cashes + " in this room \xC2\xB7 best " + ss::Ordinal(std::max(1, L.LiveBestPlace));
	}
	return L.LiveEvents > 0 ? std::to_string(L.LiveEvents) + (L.LiveEvents == 1 ? " event in this room" : " events in this room") : std::string("The Embercrest's own");
}

/**
 * The floor's colour-up at a scheduled break (spec 4.7, 6.4), as core-room's director says it (Room.cpp's break chain:
 * the chips the break's level no longer uses, talk::Floor "colorup"): "" when no chip leaves play here.
 */
std::string ColorUpLine(const ss::Tournament& T)
{
	if (!ss::live::IsEmbercrest(T.Spec.Id))
	{
		return std::string(); // (the Embercrest's chips: as the director, only its own events colour up)
	}
	const int Before = std::max(0, T.LevelIndex - 1);
	const std::vector<ss::chipset::Denom> Was = ss::chipset::InPlay(ss::chipset::Room::Embercrest, T.Levels, Before, T.Spec.BreakEvery);
	const std::vector<ss::chipset::Denom> Now = ss::chipset::InPlay(ss::chipset::Room::Embercrest, T.Levels, T.LevelIndex, T.Spec.BreakEvery);
	std::vector<std::string> Leaving;
	for (const ss::chipset::Denom& D : Was)
	{
		if (std::none_of(Now.begin(), Now.end(), [&D](const ss::chipset::Denom& X) { return X.Value == D.Value; }))
		{
			Leaving.push_back(D.Value == 100 ? "black hundreds" : D.Value == 500 ? "purple five-hundreds" : D.Value == 1000 ? "yellow thousands" : ss::talk::Count(D.Value) + "s");
		}
	}
	if (Leaving.empty())
	{
		return std::string();
	}
	std::map<std::string, std::string> V;
	V["Denoms"] = Leaving.size() == 1 ? Leaving[0] : Leaving[0] + " and the " + Leaving[1];
	return ss::talk::Floor("colorup", V, static_cast<uint32_t>(T.LevelIndex)).Text;
}

/** The desk's key names (ss::ui::DeskScreen::Key) for a key: the arrows (WASD, the d-pad, the left stick), confirm,
 *  back, who pays (Q/E, the bumpers), the refund (R, Y) and the meet (F, X); "" for anything else (the console's key). */
FString DeskKeyName(const FKey& K)
{
	if (K == EKeys::Up || K == EKeys::W || K == EKeys::Gamepad_DPad_Up || K == EKeys::Gamepad_LeftStick_Up)
	{
		return TEXT("Up");
	}
	if (K == EKeys::Down || K == EKeys::S || K == EKeys::Gamepad_DPad_Down || K == EKeys::Gamepad_LeftStick_Down)
	{
		return TEXT("Down");
	}
	if (K == EKeys::Left || K == EKeys::A || K == EKeys::Gamepad_DPad_Left || K == EKeys::Gamepad_LeftStick_Left)
	{
		return TEXT("Left");
	}
	if (K == EKeys::Right || K == EKeys::D || K == EKeys::Gamepad_DPad_Right || K == EKeys::Gamepad_LeftStick_Right)
	{
		return TEXT("Right");
	}
	if (K == EKeys::Enter || K == EKeys::SpaceBar || K == EKeys::Gamepad_FaceButton_Bottom)
	{
		return TEXT("Enter");
	}
	// P and Backspace double as Escape: in Play-In-Editor, Escape stops the session instead of reaching the game.
	if (K == EKeys::Escape || K == EKeys::P || K == EKeys::BackSpace || K == EKeys::Gamepad_FaceButton_Right || K == EKeys::Gamepad_Special_Right)
	{
		return TEXT("Escape");
	}
	if (K == EKeys::Q || K == EKeys::E || K == EKeys::R || K == EKeys::F)
	{
		return K.GetFName().ToString();
	}
	if (K == EKeys::Gamepad_LeftShoulder)
	{
		return TEXT("TabPrev");
	}
	if (K == EKeys::Gamepad_RightShoulder)
	{
		return TEXT("TabNext");
	}
	if (K == EKeys::Gamepad_FaceButton_Top)
	{
		return TEXT("Y");
	}
	if (K == EKeys::Gamepad_FaceButton_Left)
	{
		return TEXT("X");
	}
	return FString();
}

/**
 * The desk's screen: a full-screen draw list that takes the keyboard, the gamepad and the mouse while it's up (the pawn
 * stands still behind it: the game's input is off). Keys it doesn't use go on to the viewport, so the console still opens,
 * and while the console is open it has every key.
 */
class SVenueDeskWidget : public SDrawListWidget
{
public:
	SLATE_BEGIN_ARGS(SVenueDeskWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		SDrawListWidget::Construct(SDrawListWidget::FArguments().DesiredSize(FVector2D(1920.0, 1080.0)));
	}

	/** A key name for DeskScreen::Key, and whether a gamepad sent it. */
	TFunction<void(const FString&, bool)> OnDeskKey;
	/** The pointer in the screen's logical units (1,080 high): 0 moved, 1 pressed, 2 released. */
	TFunction<void(const FVector2D&, int32)> OnDeskPointer;
	TWeakObjectPtr<UGameViewportClient> Viewport;

	virtual bool SupportsKeyboardFocus() const override { return true; }

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override
	{
		if (ConsoleOpen())
		{
			return FReply::Unhandled();
		}
		const FKey Key = InKeyEvent.GetKey();
		const FString Name = DeskKeyName(Key);
		if (Name.IsEmpty())
		{
			return FReply::Unhandled();
		}
		// A held button's repeats move along the lists; confirming, choosing and backing out don't repeat (E or A held
		// from walking up to the desk would otherwise register).
		const bool bList = Name == TEXT("Up") || Name == TEXT("Down") || Name == TEXT("Left") || Name == TEXT("Right");
		if (InKeyEvent.IsRepeat() && !bList)
		{
			return FReply::Handled();
		}
		if (OnDeskKey)
		{
			OnDeskKey(Name, Key.IsGamepadKey());
		}
		return FReply::Handled();
	}
	virtual FReply OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override
	{
		return ConsoleOpen() || DeskKeyName(InKeyEvent.GetKey()).IsEmpty() ? FReply::Unhandled() : FReply::Handled();
	}
	virtual FReply OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent) override { return FReply::Unhandled(); }
	virtual FReply OnAnalogValueChanged(const FGeometry& MyGeometry, const FAnalogInputEvent& InAnalogInputEvent) override { return FReply::Handled(); }
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override { return Pointer(MyGeometry, MouseEvent, 0); }
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		return MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton ? Pointer(MyGeometry, MouseEvent, 1) : FReply::Handled();
	}
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		return MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton ? Pointer(MyGeometry, MouseEvent, 2) : FReply::Handled();
	}
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override { return FReply::Handled(); }

private:
	bool ConsoleOpen() const
	{
		const UGameViewportClient* V = Viewport.Get();
		return V && V->ViewportConsole && V->ViewportConsole->ConsoleActive();
	}
	FReply Pointer(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, int32 Button)
	{
		if (OnDeskPointer)
		{
			const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
			const FVector2D Size = MyGeometry.GetLocalSize();
			OnDeskPointer(Local * (Size.Y > 0.0 ? 1080.0 / Size.Y : 1.0), Button);
		}
		return FReply::Handled();
	}
};

/** A room console command's game mode: the Embercrest's, or nothing (Dee's game, practice, another map). */
ABackRoomGameMode* VenueMode(UWorld* World)
{
	ABackRoomGameMode* GM = World ? World->GetAuthGameMode<ABackRoomGameMode>() : nullptr;
	return GM && GM->IsLive() ? GM : nullptr;
}

FAutoConsoleCommandWithWorldAndArgs DeskCmd(TEXT("ss.Venue.Desk"),
	TEXT("ss.Venue.Desk [desk|cage]: at the Embercrest, the desk's screen (or the cashier's) opens as E there does, from anywhere in the room."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (ABackRoomGameMode* GM = VenueMode(World))
		{
			UE_LOG(LogEmbercrest, Log, TEXT("ss.Venue.Desk: %s"), *GM->TestDeskOpen(Args.Num() > 0 ? Args[0].ToLower() : FString(TEXT("desk"))));
		}
	}));
FAutoConsoleCommandWithWorldAndArgs KeyCmd(TEXT("ss.Venue.Key"),
	TEXT("ss.Venue.Key <Up|Down|Left|Right|Enter|Escape|Q|E|R|F>: a key on the desk's screen (R the refund, F Meet the regulars)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (ABackRoomGameMode* GM = VenueMode(World))
		{
			const FString Said = GM->TestDeskKey(Args.Num() > 0 ? Args[0] : FString(TEXT("Enter")));
			UE_CLOG(!Said.IsEmpty(), LogEmbercrest, Log, TEXT("ss.Venue.Key: %s"), *Said);
		}
	}));
FAutoConsoleCommandWithWorldAndArgs DescribeCmd(TEXT("ss.Venue.Describe"),
	TEXT("At the Embercrest: the venue as it stands (the desk, the clock screens' page and paint time, the badge atlas, the ambassador's banner and crest, the seat marker)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (ABackRoomGameMode* GM = VenueMode(World))
		{
			UE_LOG(LogEmbercrest, Log, TEXT("%s"), *GM->TestDeskKey(TEXT("Describe")));
		}
	}));
} // namespace VenueDetail

/**
 * ue-venue's own state on the game mode (BackRoomGameMode.h declares the pointer; this is its only definition): the desk
 * screen's widget and what it said, the funding the desk registered with on its way to the worker, the clock screens'
 * rotation and paint times, the ambassador's art, the bar's plate, the break the colour-up was said at, a save due.
 */
struct FVenueHost
{
	// The desk's screen.
	TSharedPtr<VenueDetail::SVenueDeskWidget> Widget;
	TWeakObjectPtr<UGameViewportClient> Viewport;
	FDelegateHandle Teardown;
	FSlateTextMeasurer Measurer;
	FDrawListHint Hint;
	FVector2D LastPointer = FVector2D(-1.0, -1.0);
	/** The clerk's answer to the last thing the player did at the desk ("" for the desk's own line), and when. */
	FString Clerk;
	float ClerkAt = -100.0f;
	/** A club card printing on this visit (an upgrade): its line, and the printer's chirp. */
	bool bPrinting = false;
	FString PrintLine;
	float PrintSoundAt = -1.0f;
	/** The option the player registered with, on its way through DeskSignup's worker to FinishSignup. */
	ss::life::Funding DeskFund;
	bool bDeskFund = false;
	ss::life::Funding SignupFund;
	bool bSignupFromDesk = false;
	// The clock screens.
	bool bClocksTried = false;
	float AppliedGlow = -1.0f;
	float RenderT = 0.0f;
	int32 RenderTurn = 0;
	int32 NextExtra = 0;
	bool bHoldClock = false;
	int32 StagePage = 0;
	double ClockMsSum = 0.0;
	double ClockMsPeak = 0.0;
	int32 ClockRenders = 0;
	/** Turns a target's page came out the same and wasn't painted again; each target's last page (as one number) and when. */
	int32 ClockSkips = 0;
	uint64 PaintedKey[2] = {0, 0};
	float PaintedAt[2] = {-100.0f, -100.0f};
	/** TODAY's lines, and a count of their changes (the page's number). */
	uint32 TodayRev = 0;
	std::vector<std::string> Today;
	std::vector<ss::life::RoomLeader> Leaders;
	bool bAtlasTried = false;
	// Team Embercrest's ambassador.
	bool bAmbassador = false;
	FString BannerKey;
	TWeakObjectPtr<UTextureRenderTarget2D> BannerTarget;
	TWeakObjectPtr<UTextureRenderTarget2D> CrestTarget;
	bool bPlateServed = false;
	// The colour-up (when no director speaks for the room).
	bool bBreakSeen = false;
	bool bWasOnBreak = false;
	/** Something the desk changed in the save (a card printed, a meet, the plate): kept at the next checkpoint, or now. */
	bool bSaveDue = false;
	/**
	 * The desk's view of tonight's own entry while the player plays it: sat, so the screen never offers the refund that's
	 * only for an entry that hasn't (spec 6.6). (The save in memory never carries the night's checkpoint: LiveCheckpoint
	 * writes it into the saver's copies, so the entry itself would read as fresh all night.)
	 */
	ss::life::LiveEntry Seated;
	/** What the desk's prompt last found open to the player, and when (refreshed twice a second, not every frame). */
	FString PromptOpen;
	float PromptAt = -100.0f;
	/** The desk screen's game-thread time a frame (the fill, the draw, the requests), for ss.Venue.Describe. */
	double DeskMsSum = 0.0;
	double DeskMsPeak = 0.0;
	int32 DeskFrames = 0;

	~FVenueHost()
	{
		if (Teardown.IsValid())
		{
			FWorldDelegates::OnWorldBeginTearDown.Remove(Teardown);
		}
	}
};

// ------------------------------------------------------------------ the desk's signup

void FLiveDeskSignup::Run()
{
	ss::world::World W;
	std::vector<std::string> Fields;
	for (size_t From = 0; From < WorldText.size();)
	{
		const size_t End = FMath::Min(WorldText.find('\n', From), WorldText.size());
		Fields.clear();
		for (size_t Cell = From, K = From; K <= End; ++K)
		{
			if (K == End || WorldText[K] == '\t')
			{
				Fields.emplace_back(WorldText, Cell, K - Cell);
				Cell = K + 1;
			}
		}
		W.Read(Fields);
		From = End + 1;
	}
	WorldText.clear();
	WorldText.shrink_to_fit();
	W.Finish();
	if (W.Ready() && O.Valid())
	{
		bWorld = true;
		W.HeroName = HeroName;
		// The field as the session's own registration describes it (Session::GoToLive calls the same; core's RoomField
		// test proves the desk's old copy gave the same notes): the people the world registered and how they play (the
		// room's faces as the Back Room knows them play their own way), the faces among them and what they'll say, the
		// champions' board, and their voices, badges, patches and the month's room leaders.
		Notes = ss::live::DescribeField(W, O, HeroName, Hero, At);
		// The world waits for the player's result in it (it never decides an event the player is in).
		W.HeroEntered(O.Id, O.T->Name);
		W.Write(WorldAfter);
	}
	bDone = true;
}

namespace LiveDetail
{
std::string RegisterFromSignup(ss::SaveData& D, const FLiveDeskSignup& S, double World, bool bInRoom, const ss::life::Funding& Fund)
{
	const ss::live::Occurrence& O = S.O;
	const int Entrants = S.bWorld ? FMath::Max(S.Notes.Planned, static_cast<int>(S.Notes.Roster.size())) + 1 : O.Field + 1;
	const ss::Chips Waived = bInRoom ? 2 * ss::live::BusFareCents : 0;
	ss::Chips Bank = D.BankrollCents + Waived;
	const std::string Why = ss::live::Register(Bank, D.Life, O, World, Entrants, S.Notes.Roster, Fund);
	D.BankrollCents = Bank - Waived;
	ss::life::LiveEntry* E = Why.empty() ? ss::live::EntryFor(D.Life, O.Id) : nullptr;
	if (!E)
	{
		return Why.empty() ? std::string("There's no such event.") : Why;
	}
	// As Session::GoToLive: the field's notes on the entry, then the patches the player wears in tonight.
	ss::live::Apply(S.Notes, *E);
	ss::sponsor::NoteWorn(*E, D.HeroName, D.Life.Closet, D.Life.Sponsors, World);
	if (S.bWorld)
	{
		D.WorldText = S.WorldAfter;
	}
	return std::string();
}
} // namespace LiveDetail

// ------------------------------------------------------------------ on foot: what's in reach

ECardRoomSpot ABackRoomGameMode::CurrentSpot() const
{
	const ABackRoomPawn* Pawn = Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	return Pawn && Stage && Pawn->IsFreeWalking() ? Stage->CardRoomSpot(Pawn->GetEye()) : ECardRoomSpot::None;
}

FString ABackRoomGameMode::SpotPrompt() const
{
	// The desk's screen has the room's attention while it's up.
	if (bDeskUp)
	{
		return FString();
	}
	switch (CurrentSpot())
	{
	case ECardRoomSpot::Seat: return bLiveOver ? FString() : FString(TEXT("[E] Sit back down"));
	case ECardRoomSpot::Desk:
	{
		if (Signup.IsValid())
		{
			return TEXT("[E] Tournament desk: writing you in");
		}
		if (!NextEntryId.IsEmpty())
		{
			return FString::Printf(TEXT("[E] Tournament desk: you're in the %s"), *NextName);
		}
		// Out of tonight's event: the desk can write the player into the next one. (Who could pay for each event is a few
		// lookups and strings: found twice a second, not every frame the prompt is drawn.)
		FString OpenId;
		if (bLiveOver)
		{
			FVenueHost* H = VenueHost.Get();
			if (H && H->PromptAt <= LiveT && LiveT - H->PromptAt < 0.5f)
			{
				OpenId = H->PromptOpen;
			}
			else
			{
				FString WhyNot;
				OpenId = NextOpenEvent(WhyNot);
				if (H)
				{
					H->PromptOpen = OpenId;
					H->PromptAt = LiveT;
				}
			}
		}
		const ss::live::Occurrence Open = bLiveOver ? ss::live::FindOccurrence(VenueDetail::Utf8(OpenId)) : ss::live::Occurrence();
		if (Open.Valid())
		{
			return FString::Printf(TEXT("[E] Tournament desk: the %s is open (%s)"), *Str(Open.T->Short), *Dollars(Open.T->BuyInCents));
		}
		return TEXT("[E] Tournament desk");
	}
	case ECardRoomSpot::Cage: return bLiveOver && HeroPrizeCents > 0 && !bCollected ? FString(TEXT("[E] Collect your winnings")) : FString(TEXT("[E] Cashier"));
	case ECardRoomSpot::Bar:
	{
		// Team Embercrest's ambassador eats once a night on the room (spec 3.4, 3.10).
		const std::string PlateKey = "ec-plate:" + std::to_string(LiveDay);
		const bool bPlate = Save.IsValid() && ss::sponsor::Active(Save->Life.Sponsors, "embercrest", Minutes) && !(VenueHost && VenueHost->bPlateServed) &&
							std::find(Save->TextsSeen.begin(), Save->TextsSeen.end(), PlateKey) == Save->TextsSeen.end();
		return bPlate ? FString(TEXT("[E] The bar: the ambassador's plate")) : FString(TEXT("[E] The bar"));
	}
	case ECardRoomSpot::Terrace:
		return !NextEntryId.IsEmpty() && !bNextCalled && !bWaitingOut ? FString::Printf(TEXT("[E] Step out on the terrace until the %s is called"), *NextName)
																	  : FString(TEXT("[E] Step out on the terrace"));
	case ECardRoomSpot::Exit:
		if (bLiveOver && !NextEntryId.IsEmpty())
		{
			return LiveT - ExitAskedAt < 5.0f ? FString::Printf(TEXT("[E] again: off the list for the %s, refunded, and home"), *NextName)
											  : FString::Printf(TEXT("[E] Go home (the desk refunds the %s)"), *NextName);
		}
		return bLiveOver ? FString(TEXT("[E] Go home")) : (LiveT - ExitAskedAt < 5.0f ? FString(TEXT("[E] again: leave the tournament (your stack is blinded off)")) : FString(TEXT("[E] Leave the tournament")));
	case ECardRoomSpot::Rail:
		// The stage's rail: the final table, from a fixed camera with the ON AIR lower third (spec 6.5; the view is ue-table's).
		if (IsWatchingFeature())
		{
			return TEXT("[E] Step back from the rail");
		}
		return bRailing && Tourney && !Tourney->bFinished && Tourney->Tables.size() == 1 ? FString(TEXT("[E] Watch the feature table")) : FString();
	default: return FString();
	}
}

void ABackRoomGameMode::LiveInteract()
{
	ABackRoomPawn* Pawn = Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!bLive || !Stage || !Pawn || !Pawn->IsFreeWalking() || !Tourney || bDeskUp)
	{
		return;
	}
	const ss::Level& L = Tourney->CurrentLevel();
	switch (Stage->CardRoomSpot(Pawn->GetEye()))
	{
	case ECardRoomSpot::Seat:
		LiveSitDown();
		break;
	case ECardRoomSpot::Desk:
	{
		// The desk's screen: the night as it stands, the club card, the events open now and who pays, the meet.
		OpenDesk(TEXT("desk"));
		if (bDeskUp)
		{
			break;
		}
		// (No screen to show it on: the desk says it out loud, and two presses register, on the player's own money.)
		FString Line;
		FString WhyNot;
		const FString Open = bLiveOver && NextEntryId.IsEmpty() && !Signup.IsValid() ? NextOpenEvent(WhyNot) : FString();
		const ss::live::Occurrence Offer = ss::live::FindOccurrence(VenueDetail::Utf8(Open));
		if (Signup.IsValid())
		{
			Line = TEXT("One second, I'm writing you in.");
		}
		else if (!NextEntryId.IsEmpty())
		{
			Line = bNextCalled ? FString::Printf(TEXT("They're seating the %s now. Go on, find your table."), *NextName)
							   : FString::Printf(TEXT("You're all set for the %s. Seats open at %s; the floor will call it. The bar's open, the terrace too."), *NextName,
									 *Str(ss::net::TimeLabel(NextSeatingAt)));
		}
		else if (Offer.Valid() && DeskOffer == Open && LiveT - DeskAskedAt < 8.0f)
		{
			DeskOffer.Empty();
			DeskSignup(Open);
			Line = TEXT("Let me write you in.");
		}
		else if (Offer.Valid())
		{
			DeskOffer = Open;
			DeskAskedAt = LiveT;
			const ss::live::EventTemplate& T = *Offer.T;
			const FString When = Minutes < Offer.Start ? TEXT("cards at ") + Str(ss::net::TimeLabel(Offer.Start)) : TEXT("late registration till ") + Str(ss::net::TimeLabel(Offer.LateRegEnds));
			Line = FString::Printf(TEXT("The %s is still open%s. %s: %s to the prize pool, %s to the house. %s in chips, %s. Want in?"), *Str(T.Short), *AddressTo(Save),
				*Dollars(T.BuyInCents), *Dollars(T.BuyInCents - T.FeeCents), *Dollars(T.FeeCents), *Num(T.StartingStack), *When);
		}
		else if (!WhyNot.IsEmpty())
		{
			Line = WhyNot;
		}
		else if (bLiveOver)
		{
			FString Next;
			for (int32 Day = LiveDay; Day <= LiveDay + 1 && Next.IsEmpty(); ++Day)
			{
				for (const ss::live::Occurrence& O : ss::live::Occurrences(Day))
				{
					if (O.Start > Minutes && Next.IsEmpty())
					{
						Next = FString::Printf(TEXT("the %s, %s %s"), *Str(O.T->Name), Day == LiveDay ? TEXT("tonight at") : TEXT("tomorrow at"), *Str(ss::net::TimeLabel(O.Start)));
					}
				}
			}
			Line = Next.IsEmpty() ? FString(TEXT("Thanks for coming in. Get home safe.")) : FString::Printf(TEXT("Thanks for playing. Next one's %s."), *Next);
		}
		else
		{
			const int32 Every = FMath::Max(1, Tourney->Spec.BreakEvery);
			const int32 NextBreak = (Tourney->LevelIndex / Every + 1) * Every + 1;
			const FString Status = FString::Printf(TEXT("%d left, %d paid. Level %d, %s and %s; the next break's before level %d."), Tourney->Remaining, Tourney->PaidPlaces(),
				Tourney->LevelIndex + 1, *Num(L.Sb), *Num(L.Bb), NextBreak);
			Line = PastLiveEvents == 0 ? TEXT("First time with us? Welcome to the Embercrest. ") + Status
				   : PastLiveEvents >= 4 ? FString::Printf(TEXT("There's my regular%s. "), *AddressTo(Save)) + Status
										 : Status;
		}
		VenueDetail::StaffSays(*this, TEXT("Desk"), Line, Tourney ? Tourney->Tick : 0);
		break;
	}
	case ECardRoomSpot::Cage:
		if (bLiveOver && HeroPrizeCents > 0 && !bCollected)
		{
			// The cage's screen: the receipt, the entry's own lines (a backer's share, a sponsor's cut or bonus), COLLECT.
			OpenDesk(TEXT("cage"));
			if (bDeskUp)
			{
				break;
			}
			bCollected = true;
			VenueDetail::StaffSays(*this, TEXT("Cashier"), FString::Printf(TEXT("Congratulations%s. %s, cash. Count it with me... there you go."), *AddressTo(Save), *Dollars(HeroPrizeCents)), Tourney ? Tourney->Tick : 0);
			if (Table->GetAudio())
			{
				Table->GetAudio()->Play(ss::SoundId::Cash, 0.8f);
			}
		}
		else if (bCollected)
		{
			VenueDetail::StaffSays(*this, TEXT("Cashier"), TEXT("You're all paid up, hon. Don't spend it all at once."), Tourney ? Tourney->Tick : 0);
		}
		else if (PastLiveCashes > 0)
		{
			VenueDetail::StaffSays(*this, TEXT("Cashier"), FString::Printf(TEXT("Back again%s. Your best here's %s place. Bring me something bigger tonight."), *AddressTo(Save),
				*Ordinal(FMath::Max(1, PastBestPlace))), Tourney ? Tourney->Tick : 0);
		}
		else
		{
			VenueDetail::StaffSays(*this, TEXT("Cashier"), PastLiveEvents == 0 ? FString(TEXT("Nothing to cash yet. First night? Good luck in there."))
																 : FString::Printf(TEXT("Nothing for you yet%s. Come back with good news."), *AddressTo(Save)), Tourney ? Tourney->Tick : 0);
		}
		break;
	case ECardRoomSpot::Bar:
	{
		// Team Embercrest's ambassador: a plate on the room, once a night (spec 3.4, 3.10: hunger down 35). Kept in the save
		// by the night's day, one key at a time, so a second event tonight doesn't feed them twice.
		const std::string PlateKey = "ec-plate:" + std::to_string(LiveDay);
		const bool bAmbassador = Save.IsValid() && ss::sponsor::Active(Save->Life.Sponsors, "embercrest", Minutes);
		const bool bServed = (VenueHost && VenueHost->bPlateServed) || (Save.IsValid() && std::find(Save->TextsSeen.begin(), Save->TextsSeen.end(), PlateKey) != Save->TextsSeen.end());
		if (bAmbassador && !bServed)
		{
			if (!VenueHost)
			{
				VenueHost = MakeShared<FVenueHost>();
			}
			VenueHost->bPlateServed = true;
			Save->TextsSeen.erase(std::remove_if(Save->TextsSeen.begin(), Save->TextsSeen.end(), [](const std::string& K) { return K.rfind("ec-plate:", 0) == 0; }),
				Save->TextsSeen.end());
			Save->TextsSeen.push_back(PlateKey);
			Save->Life.Hunger = FMath::Max(0.0, Save->Life.Hunger - 35.0);
			Energy = FMath::Min(100.0f, Energy + 2.0f);
			VenueHost->bSaveDue = true;
			VenueDetail::StaffSays(*this, TEXT("Bartender"), FString::Printf(TEXT("Ambassador's plate%s. Short rib and fries, on the house. Denise's orders."), *AddressTo(Save)), Tourney ? Tourney->Tick : 0);
			UE_LOG(LogEmbercrest, Log, TEXT("The bar: the ambassador's plate (hunger now %.0f)"), Save->Life.Hunger);
		}
		else if (LiveT - LastWater > 300.0f)
		{
			LastWater = LiveT;
			Energy = FMath::Min(100.0f, Energy + 3.0f);
			VenueDetail::StaffSays(*this, TEXT("Bartender"), TEXT("Club soda, lime. On the house for players."), Tourney ? Tourney->Tick : 0);
		}
		else
		{
			VenueDetail::StaffSays(*this, TEXT("Bartender"), TEXT("Another one? Pace yourself, the night's long."), Tourney ? Tourney->Tick : 0);
		}
		break;
	}
	case ECardRoomSpot::Terrace:
		if (!NextEntryId.IsEmpty() && !bNextCalled && !bWaitingOut)
		{
			// Waiting for the next event out in the cold: the wait passes (the room plays on without you) until the floor
			// calls it.
			bWaitingOut = true;
			APlayerController* PC = GetWorld()->GetFirstPlayerController();
			if (PC && PC->PlayerCameraManager)
			{
				PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, 0.9f, FLinearColor::Black, true, true);
			}
			FTimerHandle Handle;
			GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]() {
				const double Until = NextSeatingAt;
				RailCatchUp(Until);
				LastAir = LiveT;
				Energy = FMath::Min(100.0f, Energy + 6.0f);
				bWaitingOut = false;
				if (APlayerController* P = GetWorld()->GetFirstPlayerController(); P && P->PlayerCameraManager)
				{
					P->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 1.4f, FLinearColor::Black, true, false);
				}
				VenueDetail::StaffSays(*this, FString(), FString::Printf(TEXT("Out on the terrace until %s: the ridge, the cold, the resort's lights on the slopes. Inside, the floor's on the mic."),
											   *Str(ss::net::TimeLabel(Until))), Tourney ? Tourney->Tick : 0);
			}), 1.0f, false);
		}
		else if (LiveT - LastAir > 420.0f)
		{
			LastAir = LiveT;
			Energy = FMath::Min(100.0f, Energy + 6.0f);
			VenueDetail::StaffSays(*this, FString(), TEXT("Cold mountain air, the ridge white under the stars. You wake up a little."), Tourney ? Tourney->Tick : 0);
		}
		else
		{
			VenueDetail::StaffSays(*this, FString(), TEXT("The ranges, dark and huge. The room's warm behind you."), Tourney ? Tourney->Tick : 0);
		}
		break;
	case ECardRoomSpot::Exit:
		if (bLiveOver && !NextEntryId.IsEmpty())
		{
			// Registered for the next event: leaving takes the player off its list (refunded: they never sat), then home.
			if (bOpeningNext)
			{
				break;
			}
			if (LiveT - ExitAskedAt < 5.0f)
			{
				LiveUnregister();
				bExitNow = true;
			}
			else
			{
				ExitAskedAt = LiveT;
				VenueDetail::StaffSays(*this, TEXT("Floor"), FString::Printf(TEXT("You're on the list for the %s. Head out now and the desk takes you off it, money back. Press again to go."), *NextName), Tourney ? Tourney->Tick : 0);
			}
		}
		else if (bLiveOver)
		{
			bExitNow = true;
		}
		else if (LiveT - ExitAskedAt < 5.0f)
		{
			// Walking out on a live tournament: the seat's dealt in until the blinds take it (the published rule).
			bExitNow = true;
			QuitAskedAt = LiveT;
			LiveRequestLeave();
		}
		else
		{
			ExitAskedAt = LiveT;
			VenueDetail::StaffSays(*this, TEXT("Floor"), TEXT("Heading out? Your seat stays in and gets blinded off. Press again to go."), Tourney ? Tourney->Tick : 0);
		}
		break;
	case ECardRoomSpot::Rail:
		// The feature table from the rail (ue-table's view); E again steps back.
		if (IsWatchingFeature())
		{
			WatchFeature(false);
		}
		else if (bRailing && !Tourney->bFinished && Tourney->Tables.size() == 1)
		{
			WatchFeature(true);
		}
		break;
	default:
		break;
	}
}

// ------------------------------------------------------------------ the boards

void ABackRoomGameMode::UpdateRoomBoards()
{
	using namespace VenueDetail;
	if (!Stage || !Tourney || !Save.IsValid())
	{
		return;
	}
	if (!VenueHost)
	{
		VenueHost = MakeShared<FVenueHost>();
	}
	FVenueHost& V = *VenueHost;
	// The badge glyphs, once, as the room opens (SeatLive's call): the HUD's plates and the studying caption draw from it.
	if (!V.bAtlasTried)
	{
		V.bAtlasTried = true;
		TSharedPtr<FBadgeAtlas> Atlas = MakeShared<FBadgeAtlas>();
		if (Atlas->Build(this))
		{
			VenueObjects.Add(Atlas->GetTarget());
			BadgeAtlas = Atlas;
		}
	}
	// The month's leaders as the night's snapshot has them, kept for the night (settling the entry clears the snapshot).
	if (V.Leaders.empty())
	{
		if (const ss::life::LiveEntry* Mine = ss::live::EntryFor(Save->Life, Utf8(LiveEntryId)))
		{
			V.Leaders = Mine->Leaders;
		}
	}
	// Today's events: by the entrance with where each stands for the player, and on the clocks' TODAY page as the room's
	// public facts.
	TArray<FString> Lines;
	std::vector<std::string> Today;
	for (const ss::live::Occurrence& O : ss::live::Occurrences(LiveDay))
	{
		const bool bThis = Str(O.Id) == LiveEntryId;
		FString State;
		if (bThis && bLiveOver)
		{
			State = TEXT("YOUR NIGHT IS OVER");
		}
		else if (bThis)
		{
			State = bLastCalled && !bRegClosed ? TEXT("LAST CALL  ·  REG TO ") + Str(ss::net::TimeLabel(LiveLateRegEnds))
											   : FString::Printf(TEXT("PLAYING  ·  %d OF %d LEFT"), Tourney->Remaining, Tourney->Spec.Entrants);
		}
		else if (Str(O.Id) == NextEntryId)
		{
			// The event the desk wrote the player into tonight.
			State = bNextCalled ? FString(TEXT("YOU'RE IN  ·  SEATING NOW")) : TEXT("YOU'RE IN  ·  SEATS ") + Str(ss::net::TimeLabel(NextSeatingAt));
		}
		else if (Minutes < O.DeskOpens)
		{
			State = TEXT("REGISTRATION ") + Str(ss::net::TimeLabel(O.DeskOpens));
		}
		else if (Minutes < O.Start)
		{
			State = TEXT("REGISTERING NOW");
		}
		else if (Minutes < O.LateRegEnds)
		{
			State = (ss::live::LastCall(O, Minutes) ? TEXT("LAST CALL  ·  REG TO ") : TEXT("LATE REG TO ")) + Str(ss::net::TimeLabel(O.LateRegEnds));
		}
		else if (Minutes < O.Start + O.T->Hours * 60.0)
		{
			State = TEXT("RUNNING");
		}
		else
		{
			State = TEXT("FINISHED");
		}
		Lines.Add(FString::Printf(TEXT("%-9s %-24s %s"), *Str(ss::net::TimeLabel(O.Start)), *Str(O.T->Name).ToUpper(), *State));
		std::string Public;
		const std::string Until = Upper(ss::net::TimeLabel(O.LateRegEnds));
		if (bThis && bCancelled)
		{
			Public = "CANCELLED";
		}
		else if (bThis && Tourney->bFinished)
		{
			Public = "FINISHED";
		}
		else if (bThis)
		{
			Public = Minutes < O.LateRegEnds ? (ss::live::LastCall(O, Minutes) ? "LAST CALL \xC2\xB7 REG TO " + Until : "RUNNING \xC2\xB7 REG TO " + Until)
											 : "RUNNING \xC2\xB7 " + std::to_string(Tourney->Remaining) + " LEFT";
		}
		else if (Minutes < O.DeskOpens)
		{
			Public = "REGISTRATION " + Upper(ss::net::TimeLabel(O.DeskOpens));
		}
		else if (Minutes < O.Start)
		{
			Public = "REGISTERING NOW";
		}
		else if (Minutes < O.LateRegEnds)
		{
			Public = (ss::live::LastCall(O, Minutes) ? "LAST CALL \xC2\xB7 REG TO " : "LATE REG TO ") + Until;
		}
		else
		{
			Public = Minutes < O.Start + O.T->Hours * 60.0 ? "RUNNING" : "FINISHED";
		}
		Today.push_back(ss::net::TimeLabel(O.Start) + "\t" + Upper(O.T->Short) + " " + Price(O.T->BuyInCents) + "\t" + Public);
	}
	Stage->SetSchedule(Lines);
	if (Today != V.Today)
	{
		V.Today = MoveTemp(Today);
		++V.TodayRev;
	}

	// Team Embercrest's ambassador (spec 3.10): the banner by the desk with the player's name and record, and the crest
	// beside their lines on the champions' board. Painted again only when what it says changes.
	const bool bContract = ss::sponsor::Active(Save->Life.Sponsors, "embercrest", Minutes);
	const bool bShow = (bContract || CVarAmbassador.GetValueOnGameThread() != 0) && FSlateTextMeasurer::IsAvailable();
	if (bShow)
	{
		const float Glow = FMath::Max(0.0f, CVarPrintGlow.GetValueOnGameThread());
		const FString Name = HeroCalled(*Save);
		const std::string Record = AmbassadorRecord(Save->Life);
		const FString Key = FString::Printf(TEXT("%s|%s|%.3f"), *Name, *Str(Record), Glow);
		if (Key != V.BannerKey || !V.BannerTarget.IsValid() || !V.CrestTarget.IsValid())
		{
			V.BannerKey = Key;
			if (!V.BannerTarget.IsValid())
			{
				if (UTextureRenderTarget2D* T = FCoreCanvasTarget::MakeTarget(this, BannerW, BannerH))
				{
					VenueObjects.Add(T);
					V.BannerTarget = T;
				}
			}
			if (!V.CrestTarget.IsValid())
			{
				if (UTextureRenderTarget2D* T = FCoreCanvasTarget::MakeTarget(this, CrestPx, CrestPx))
				{
					VenueObjects.Add(T);
					V.CrestTarget = T;
				}
			}
			if (!RoomClockCanvas)
			{
				RoomClockCanvas = MakeShared<FCoreCanvasTarget>();
			}
			if (V.BannerTarget.IsValid())
			{
				TSharedPtr<ss::ui::DrawList> List = MakeShared<ss::ui::DrawList>();
				ss::ui::Canvas C(*List, V.Measurer, static_cast<float>(BannerW), static_cast<float>(BannerH), 1.0f);
				ss::ui::sponsorart::AmbassadorBanner(C, Utf8(Name), Record);
				RoomClockCanvas->Render(V.BannerTarget.Get(), List);
			}
			if (V.CrestTarget.IsValid())
			{
				TSharedPtr<ss::ui::DrawList> List = MakeShared<ss::ui::DrawList>();
				ss::ui::Canvas C(*List, V.Measurer, static_cast<float>(CrestPx), static_cast<float>(CrestPx), 1.0f);
				ss::ui::sponsorart::Mark(C, "embercrest", {4.0f, 4.0f, static_cast<float>(CrestPx) - 8.0f, static_cast<float>(CrestPx) - 8.0f});
				RoomClockCanvas->Render(V.CrestTarget.Get(), List);
			}
			Stage->SetAmbassadorBanner(V.BannerTarget.Get(), Glow);
			Stage->SetChampionCrest(V.CrestTarget.Get(), {Str(Save->HeroName), Name}, Glow);
			UE_LOG(LogEmbercrest, Log, TEXT("Team Embercrest's banner by the desk: %s (%s)%s; the crest on %d of the champions' lines"), *Name, *Str(Record),
				bContract ? TEXT("") : TEXT(" [ss.Venue.Ambassador]"), Stage->CrestedLines());
		}
	}
	else if (V.bAmbassador)
	{
		Stage->SetAmbassadorBanner(nullptr, 0.0f);
		Stage->SetChampionCrest(nullptr, {}, 0.0f);
		V.BannerKey.Empty();
	}
	V.bAmbassador = bShow;
}

void ABackRoomGameMode::UpdateBoard(float RealDt)
{
	if (!VenueHost)
	{
		VenueHost = MakeShared<FVenueHost>();
	}
	FVenueHost& V = *VenueHost;
	// The desk's screen (every frame while it's up), the clock screens (painted in turn about twice a second each).
	TickDesk(RealDt);
	UpdateRoomClocks(RealDt);
	// What the desk changed in the save (a card printed, a meet, the plate) is kept: during the player's own event with the
	// next checkpoint (the save on disk always pairs the night with its checkpoint), after it at once.
	if (V.bSaveDue && Save.IsValid())
	{
		V.bSaveDue = false;
		if (bLiveOver)
		{
			if (LiveSaver)
			{
				LiveSaver->Flush();
			}
			CareerSave::SaveNow(Save->Serialize());
		}
		else
		{
			bCheckpointDue = true;
		}
	}
	// A scheduled break's colour-up (spec 4.7, 6.4): core-room's director says it in its break chain; a room without the
	// director hears the same line from the floor after the break's call.
	if (Tourney)
	{
		if (!V.bBreakSeen)
		{
			V.bBreakSeen = true; // (a night resumed on a break heard it before the game closed)
		}
		else if (bOnBreak && !V.bWasOnBreak && !RoomDirector.IsValid())
		{
			const std::string Line = VenueDetail::ColorUpLine(*Tourney);
			if (!Line.empty())
			{
				const FString Said = Str(Line);
				FTimerHandle Handle;
				GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, Said]() {
					if (bOnBreak)
					{
						Floor(Said, false);
					}
				}), 5.5f, false);
				UE_LOG(LogEmbercrest, Log, TEXT("Colour-up at the break before level %d: %s"), Tourney->LevelIndex + 1, *Said);
			}
		}
		V.bWasOnBreak = bOnBreak;
	}
	BoardTick -= RealDt;
	if (BoardTick > 0.0f || !Stage || !Tourney)
	{
		return;
	}
	BoardTick = 0.25f;
	RoomBoardTick -= 0.25f;
	if (RoomBoardTick <= 0.0f)
	{
		RoomBoardTick = 3.0f;
		UpdateRoomBoards();
	}
	if (Stage->HasClockScreens())
	{
		return; // the pages carry it (UpdateRoomClocks)
	}
	// The old text screens, from the same clock facts as the HUD's clock card and the pages (live::Clock).
	const ss::live::Occurrence O = ss::live::FindOccurrence(VenueDetail::Utf8(LiveEntryId));
	const double BreakLeft = bOnBreak ? LiveBreakMinutes * 60.0 * (1.0 - static_cast<double>(BreakT) / BreakRealSeconds) : 0.0;
	const ss::live::ClockFacts F = ss::live::Clock(*Tourney, O, nullptr, Minutes, LevelTimeLeft(), bOnBreak, FMath::Max(0.0, BreakLeft));
	const bool bBeforeCards = Phase == EBackRoomPhase::Arriving && !bAnnouncedStart && Tourney->Tick == 0;
	// In the last minutes of late registration the title takes turns with the last call.
	const bool bLastCallUp = bLastCalled && !bRegClosed && !bCancelled && FMath::FloorToInt(LiveT / 3.0f) % 2 == 1;
	const FString Title = bFinalTable ? FString(TEXT("FINAL TABLE"))
					  : F.OnBreak	  ? FString(TEXT("BREAK"))
					  : bLastCallUp	  ? TEXT("LAST CALL  ·  REG TO ") + Str(ss::net::TimeLabel(LiveLateRegEnds))
									  : LiveName.ToUpper();
	const int32 Secs = FMath::FloorToInt(F.OnBreak ? F.BreakSecondsLeft : F.LevelSecondsLeft);
	const FString Clock = bBeforeCards ? FString(TEXT("SOON")) : FString::Printf(TEXT("%02d:%02d"), Secs / 60, Secs % 60);
	const FString Blinds = F.Now.Ante > 0 ? FString::Printf(TEXT("%s / %s   ANTE %s"), *Num(F.Now.Sb), *Num(F.Now.Bb), *Num(F.Now.Ante)) : FString::Printf(TEXT("%s / %s"), *Num(F.Now.Sb), *Num(F.Now.Bb));
	const FString Next = FString::Printf(TEXT("NEXT   %s / %s"), *Num(F.Next.Sb), *Num(F.Next.Bb));
	// The champion as the room knows them: the person's name (the creator's), or the screen name for a career from before it.
	const FString Champion = !Save.IsValid() ? FString() : Save->Person.Created ? Str(Save->Person.FullName()).ToUpper() : Str(Save->HeroName);
	const FString Field = bHeroWon ? FString::Printf(TEXT("CHAMPION   %s"), *Champion)
								   : FString::Printf(TEXT("PLAYERS %d / %d     AVG %s     PAID %d"), F.Remaining, F.Entrants, *Num(F.Average), F.Paid);
	Stage->SetBoard(Title, FString::Printf(TEXT("LEVEL %d"), F.Level), Clock, Blinds, Next, Field);
}

// ------------------------------------------------------------------ the next event

FString ABackRoomGameMode::NextOpenEvent(FString& OutWhyNot) const
{
	OutWhyNot.Empty();
	if (!Save.IsValid() || !bLiveOver || bCancelled || ss::live::ActiveEntry(Save->Life))
	{
		return FString();
	}
	// Today's events the player hasn't played, while the walk to the seat still makes late registration, that someone can
	// pay for (the player's own money, a stake, a cover, a package's roll: the desk's own options).
	for (const ss::live::Occurrence& O : ss::live::Occurrences(LiveDay))
	{
		if (Str(O.Id) == LiveEntryId || ss::live::EntryFor(Save->Life, O.Id) || Minutes >= O.LateRegEnds)
		{
			continue;
		}
		const FString Why = VenueDetail::EventWhyNot(O, Minutes, Energy);
		if (!Why.IsEmpty())
		{
			OutWhyNot = OutWhyNot.IsEmpty() ? Why : OutWhyNot;
			continue;
		}
		if (!VenueDetail::AnyPays(VenueDetail::DeskPay(*Save, O, Minutes)))
		{
			OutWhyNot = OutWhyNot.IsEmpty() ? FString::Printf(TEXT("The %s is %s. Come back when you've got it."), *Str(O.T->Short), *Dollars(O.T->BuyInCents)) : OutWhyNot;
			continue;
		}
		return Str(O.Id);
	}
	return FString();
}

void ABackRoomGameMode::DeskSignup(const FString& OccurrenceId)
{
	const ss::live::Occurrence O = ss::live::FindOccurrence(std::string(TCHAR_TO_UTF8(*OccurrenceId)));
	if (!O.Valid() || !Save.IsValid() || Signup.IsValid() || !NextEntryId.IsEmpty())
	{
		return;
	}
	// Who pays: the option the desk's screen registered with (exactly as drawn), else the player's own money (two presses of
	// E with no screen, ss.Live.Next).
	if (!VenueHost)
	{
		VenueHost = MakeShared<FVenueHost>();
	}
	VenueHost->bSignupFromDesk = VenueHost->bDeskFund;
	VenueHost->SignupFund = VenueHost->bDeskFund ? VenueHost->DeskFund : ss::life::Funding();
	VenueHost->bDeskFund = false;
	// The world's field for it, read on a worker (the save's world is megabytes of text); the entry when it's done.
	TSharedPtr<FLiveDeskSignup> S = MakeShared<FLiveDeskSignup>();
	S->WorldText = Save->WorldText;
	S->HeroName = Save->HeroName;
	S->Hero = Save->Life;
	S->O = O;
	S->At = Minutes;
	Signup = S;
	UE::Tasks::Launch(UE_SOURCE_LOCATION, [S]() { S->Run(); });
	UE_LOG(LogEmbercrest, Log, TEXT("The desk writes the player in for %s (%s), %s"), *OccurrenceId, *ClockLabel(),
		VenueHost->SignupFund.From == ss::life::Paid::Own ? TEXT("on their own money") : *FString::Printf(TEXT("paid by %s"), *Str(VenueHost->SignupFund.Who)));
}

void ABackRoomGameMode::FinishSignup()
{
	const TSharedPtr<FLiveDeskSignup> S = Signup;
	Signup.Reset();
	const ss::life::Funding Fund = VenueHost && VenueHost->bSignupFromDesk ? VenueHost->SignupFund : ss::life::Funding();
	if (VenueHost)
	{
		VenueHost->bSignupFromDesk = false;
	}
	if (!S.IsValid() || !Save.IsValid() || bGoingHome || bExitNow || bOpeningNext)
	{
		return; // on the way out: the desk never wrote it down
	}
	// The clerk says it on the desk's screen when it's up, out loud otherwise.
	auto Clerk = [this](const FString& Line) {
		if (bDeskUp && VenueHost)
		{
			VenueHost->Clerk = Line;
			VenueHost->ClerkAt = LiveT;
		}
		else
		{
			VenueDetail::StaffSays(*this, TEXT("Desk"), Line, Tourney ? Tourney->Tick : 0);
		}
	};
	const ss::live::Occurrence& O = S->O;
	ss::SaveData D = *Save;
	const std::string WorldWas = D.WorldText;
	// At the desk, here already (the core's check counts the trip there: it's made). Whoever pays, as drawn: live::Register
	// refuses money that isn't on offer as it stands now (an offer lapsed, a cover used up), and its line is the clerk's.
	const double Booked = Minutes - ss::live::TravelMinutes;
	const std::string Why = RegisterFromSignup(D, *S, Booked, true, Fund);
	ss::life::LiveEntry* E = Why.empty() ? ss::live::EntryFor(D.Life, O.Id) : nullptr;
	if (!E)
	{
		Clerk(Str(Why));
		UE_LOG(LogEmbercrest, Warning, TEXT("The desk can't register the player for %s: %s"), *Str(O.Id), *Str(Why));
		return;
	}
	// Registered and arrived both now: no bus there, and none home of its own (the night's ticket covers it). The ledger's
	// lines carry the desk's own time (the funding's line, the pieces', the buy-in and the fee: newest first).
	E->RegisteredAt = Minutes;
	E->ArrivedAt = Minutes;
	E->FareThere = true;
	// The night's own departure, kept on the entry until its first checkpoint carries it (a game closed while waiting goes
	// back to this room, and Night One still catches its calendar up from the night's first bus).
	E->CheckpointHost = std::string(TCHAR_TO_UTF8(*FString::Printf(TEXT("from\t%.2f\n"), NightLeftHomeAt)));
	for (size_t K = 0; K < D.Life.Ledger.size() && K < 4; ++K)
	{
		if (D.Life.Ledger[K].At == Booked)
		{
			D.Life.Ledger[K].At = Minutes;
		}
	}
	D.ClockMinutes = Minutes - ss::net::MinutesPerDay * ss::net::NightOneDay;
	D.Life.Energy = FMath::Clamp(static_cast<double>(Energy), 0.0, 100.0);
	if (S->bWorld)
	{
		WorldBeforeSignup = WorldWas;
	}
	*Save = D;
	if (LiveSaver)
	{
		LiveSaver->Flush();
	}
	CareerSave::SaveNow(D.Serialize());
	NextEntryId = Str(O.Id);
	NextName = Str(O.T->Short);
	NextSeatingAt = FMath::Max(Minutes, O.Start - SeatingLead);
	bNextCalled = false;
	NextCallT = 0.0f;
	const FString Paid = Fund.From == ss::life::Paid::Own ? FString() : TEXT(" ") + Str(ss::live::FundingLine(Fund, O.T->Short));
	Clerk(Minutes >= NextSeatingAt
			  ? FString::Printf(TEXT("You're in%s. %s.%s They're seating it now; the floor will call you over."), *AddressTo(Save), *Str(O.T->Name), *Paid)
			  : FString::Printf(TEXT("You're in%s. %s.%s Seats open at %s; the floor will call it. The bar's open, the terrace too."), *AddressTo(Save), *Str(O.T->Name), *Paid,
					*Str(ss::net::TimeLabel(NextSeatingAt))));
	if (Table->GetAudio())
	{
		Table->GetAudio()->Play(ss::SoundId::Cash, 0.5f);
	}
	UpdateRoomBoards();
	UE_LOG(LogEmbercrest, Log, TEXT("Registered at the desk for %s: %d entrants (%d the world follows, %d faces), bankroll %lld; seats at %s"), *NextEntryId, E->Entrants,
		static_cast<int32>(E->Roster.size()), static_cast<int32>(E->Faces.size()), static_cast<int64>(D.BankrollCents), *Str(ss::net::TimeLabel(NextSeatingAt)));
}

void ABackRoomGameMode::LiveUnregister()
{
	if (NextEntryId.IsEmpty() || !Save.IsValid())
	{
		return;
	}
	// Off the list before sitting down: the buy-in and fee back (their own ledger lines), and the world as it was before the
	// desk told it. The entry goes with them: a refunded entry reads as an event the room called off (the desk turns the
	// player away from it as cancelled, the phone's card says so), and this one wasn't. The player can buy in again from
	// home while its registration's open.
	ss::SaveData D = *Save;
	const std::string Id(TCHAR_TO_UTF8(*NextEntryId));
	if (ss::live::Refund(D.BankrollCents, D.Life, Id, Minutes, "off the list before sitting down"))
	{
		D.Life.LiveEntries.erase(std::remove_if(D.Life.LiveEntries.begin(), D.Life.LiveEntries.end(), [&Id](const ss::life::LiveEntry& E) { return E.Id == Id; }),
			D.Life.LiveEntries.end());
		if (!WorldBeforeSignup.empty())
		{
			D.WorldText = WorldBeforeSignup;
		}
	}
	*Save = D;
	if (LiveSaver)
	{
		LiveSaver->Flush();
	}
	CareerSave::SaveNow(D.Serialize());
	UE_LOG(LogEmbercrest, Log, TEXT("Off the list for %s, refunded"), *NextEntryId);
	WorldBeforeSignup.clear();
	NextEntryId.Empty();
	NextName.Empty();
	bNextCalled = false;
	UpdateRoomBoards();
}

void ABackRoomGameMode::LiveCallNext()
{
	bNextCalled = true;
	NextCallT = 0.0f;
	const ss::live::Occurrence O = ss::live::FindOccurrence(std::string(TCHAR_TO_UTF8(*NextEntryId)));
	if (!O.Valid())
	{
		return;
	}
	Floor(Minutes >= O.Start
			  ? FString::Printf(TEXT("Players registered for the %s, your seats are ready. Late registration closes at %s."), *NextName, *Str(ss::net::TimeLabel(O.LateRegEnds)))
			  : FString::Printf(TEXT("Players registered for the %s, your seats are open. Please find your table and seat; cards go in the air at %s."), *NextName,
					*Str(ss::net::TimeLabel(O.Start))));
	RaiseBanner(FString::Printf(TEXT("%s   SEATS OPEN"), *NextName.ToUpper()));
	UpdateRoomBoards();
	UE_LOG(LogEmbercrest, Log, TEXT("The floor calls %s (%s)"), *NextEntryId, *ClockLabel());
}

void ABackRoomGameMode::LiveOpenNext()
{
	if (bOpeningNext || bGoingHome || NextEntryId.IsEmpty() || !Save.IsValid())
	{
		return;
	}
	CloseDesk();
	bOpeningNext = true;
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController(); PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, 1.2f, FLinearColor::Black, true, true);
	}
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]() {
		if (bGoingHome)
		{
			return; // quit from the pause menu meanwhile: that travel stands (Continue comes back to the entry)
		}
		// What the rail saw, the champion for the next event's board if the room crowned one, and the clock as it stands:
		// the room opens again for the next event with no bus and no trip, and the night's departure for Night One.
		ss::SaveData D = *Save;
		NoteRailed(D);
		ss::life::LiveEntry* Next = ss::live::EntryFor(D.Life, std::string(TCHAR_TO_UTF8(*NextEntryId)));
		const ss::TPlayer* Champion = nullptr;
		for (const ss::TPlayer& P : Tourney->Players)
		{
			Champion = Tourney->bFinished && P.Place == 1 ? &P : Champion;
		}
		if (Next && Champion)
		{
			static const TCHAR* Days[7] = {TEXT("MON"), TEXT("TUE"), TEXT("WED"), TEXT("THU"), TEXT("FRI"), TEXT("SAT"), TEXT("SUN")};
			const FString Who = Champion->IsHero ? Str(D.HeroName) : Str(Champion->Name);
			const FString Line = FString::Printf(TEXT("%s %s\t%s\t%lld"), Days[((LiveDay % 7) + 7) % 7], *LiveShort.ToUpper(), *Who, static_cast<int64>(Tourney->PrizeFor(1)));
			Next->Board.insert(Next->Board.begin(), std::string(TCHAR_TO_UTF8(*Line)));
			Next->Board.resize(FMath::Min<size_t>(Next->Board.size(), 8));
		}
		D.ClockMinutes = Minutes - ss::net::MinutesPerDay * ss::net::NightOneDay;
		D.Life.Energy = FMath::Clamp(static_cast<double>(Energy), 0.0, 100.0);
		*Save = D;
		if (LiveSaver)
		{
			LiveSaver->Flush();
		}
		CareerSave::SaveNow(D.Serialize());
		UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
		const FString Options = FString::Printf(TEXT("Live=%s?Here=1?At=%.2f?From=%.2f"), *NextEntryId, Minutes, NightLeftHomeAt);
		UE_LOG(LogEmbercrest, Log, TEXT("On to the next event: %s"), *Options);
		UGameplayStatics::OpenLevel(this, FName(TEXT("BackRoom")), true, Options);
	}), 1.3f, false);
}

FString ABackRoomGameMode::GetNextEventNote() const
{
	if (NextEntryId.IsEmpty())
	{
		return FString();
	}
	return bNextCalled ? FString::Printf(TEXT("Registered: %s, seating now"), *NextName)
					   : FString::Printf(TEXT("Registered: %s, seats at %s"), *NextName, *Str(ss::net::TimeLabel(NextSeatingAt)));
}

// ------------------------------------------------------------------ the desk screen (ue-venue, next_segment_spec 6.6)

void ABackRoomGameMode::OpenDesk(const FString& Window)
{
	using namespace VenueDetail;
	UWorld* W = GetWorld();
	UGameViewportClient* Viewport = W ? W->GetGameViewport() : nullptr;
	if (!bLive || !Save.IsValid() || !Tourney || !Viewport || !FSlateApplication::IsInitialized() || (Window != TEXT("desk") && Window != TEXT("cage")))
	{
		return;
	}
	if (!VenueHost)
	{
		VenueHost = MakeShared<FVenueHost>();
	}
	FVenueHost& V = *VenueHost;
	if (!V.Widget)
	{
		TWeakObjectPtr<ABackRoomGameMode> Weak = this;
		V.Widget = SNew(SVenueDeskWidget);
		V.Widget->Viewport = Viewport;
		V.Widget->OnDeskKey = [Weak](const FString& Name, bool bFromGamepad) {
			ABackRoomGameMode* GM = Weak.Get();
			if (!GM || !GM->bDeskUp)
			{
				return;
			}
			GM->Desk.Gamepad = bFromGamepad;
			if (ABackRoomPlayerController* PC = Cast<ABackRoomPlayerController>(UGameplayStatics::GetPlayerController(GM, 0)))
			{
				PC->SetUsingGamepad(bFromGamepad);
			}
			GM->Desk.Key(Utf8(Name), GM->LiveT);
		};
		V.Widget->OnDeskPointer = [Weak](const FVector2D& Logical, int32 Button) {
			ABackRoomGameMode* GM = Weak.Get();
			if (!GM || !GM->bDeskUp || !GM->VenueHost)
			{
				return;
			}
			FVenueHost& H = *GM->VenueHost;
			// The mouse in use (a click, or really moved: Slate also reports it standing still as the screen appears under
			// it): the screen shows the keys again.
			const bool bMoved = H.LastPointer.X >= 0.0 && FVector2D::Distance(H.LastPointer, Logical) > 2.0;
			H.LastPointer = Logical;
			if (Button != 0 || bMoved)
			{
				GM->Desk.Gamepad = false;
			}
			ss::ui::Pointer& P = GM->Desk.Ptr;
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
		};
		V.Widget->SetVisibility(EVisibility::Collapsed);
		// Over the HUD, under the pause menu (30).
		Viewport->AddViewportWidgetContent(V.Widget.ToSharedRef(), 20);
		V.Viewport = Viewport;
		DeskWidget = V.Widget;
		TWeakPtr<SVenueDeskWidget> WeakWidget = V.Widget;
		TWeakObjectPtr<UGameViewportClient> WeakViewport = Viewport;
		TWeakObjectPtr<UWorld> WeakWorld = W;
		V.Teardown = FWorldDelegates::OnWorldBeginTearDown.AddLambda([Weak, WeakWidget, WeakViewport, WeakWorld](UWorld* Gone) {
			if (!Gone || Gone != WeakWorld.Get())
			{
				return;
			}
			// A level opened with the screen still up (a walk out, a console travel): the input goes back to the game first.
			// The viewport client outlives the map, and UI-only input left on it would have the next map ignore every key.
			if (ABackRoomGameMode* GM = Weak.Get(); GM && GM->bDeskUp)
			{
				GM->CloseDesk();
			}
			const TSharedPtr<SVenueDeskWidget> Wd = WeakWidget.Pin();
			UGameViewportClient* Vp = WeakViewport.Get();
			if (Wd && Vp)
			{
				Vp->RemoveViewportWidgetContent(Wd.ToSharedRef());
			}
		});
	}
	DeskWindow = Window;
	bDeskUp = true;
	Desk.Open(LiveT, Utf8(Window));
	const ABackRoomPlayerController* PC = Cast<ABackRoomPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	Desk.Gamepad = PC && PC->IsUsingGamepad();
	V.Clerk.Empty();
	V.bPrinting = false;
	V.PrintSoundAt = -1.0f;
	V.LastPointer = FVector2D(-1.0, -1.0);
	// The Players Club card: on an upgrade the desk prints the new one (the printer's chirp, the upgrade's line, once per
	// tier: badge::DeskKey in the save); otherwise it's the member's card in the slot.
	if (Window == TEXT("desk"))
	{
		const ss::ui::badgeart::ClubCardInfo Card = ss::ui::badgeart::ClubCardOf(*Save, Minutes);
		if (Card.New && Card.Tier >= 1)
		{
			V.bPrinting = true;
			V.PrintSoundAt = LiveT + 0.35f;
			V.PrintLine = Str(ss::badge::DeskLine(Card.Tier, true, Utf8(AddressTo(Save))));
			Save->TextsSeen.push_back(ss::badge::DeskKey(Card.Tier));
			V.bSaveDue = true;
			UE_LOG(LogEmbercrest, Log, TEXT("The desk prints a new Players Club card: tier %d, %d visits in 90 days"), Card.Tier, Card.Visits);
		}
	}
	V.Widget->SetVisibility(EVisibility::Visible);
	if (APlayerController* Pc = UGameplayStatics::GetPlayerController(this, 0))
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(V.Widget);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Pc->SetInputMode(InputMode);
		Pc->SetShowMouseCursor(true);
		// What was held walking up (E, A, a stick) doesn't stay held behind the screen.
		Pc->FlushPressedKeys();
	}
	FSlateApplication::Get().SetKeyboardFocus(V.Widget, EFocusCause::SetDirectly);
	UE_LOG(LogEmbercrest, Log, TEXT("The %s's screen opens (%s)"), *Window, *ClockLabel());
}

void ABackRoomGameMode::CloseDesk()
{
	const bool bWasUp = bDeskUp;
	bDeskUp = false;
	DeskWindow.Empty();
	if (VenueHost)
	{
		VenueHost->bPrinting = false;
		VenueHost->PrintSoundAt = -1.0f;
		VenueHost->Clerk.Empty();
		if (VenueHost->Widget)
		{
			VenueHost->Widget->SetVisibility(EVisibility::Collapsed);
		}
	}
	if (!bWasUp)
	{
		return;
	}
	if (APlayerController* Pc = UGameplayStatics::GetPlayerController(this, 0))
	{
		Pc->SetInputMode(FInputModeGameOnly());
		Pc->SetShowMouseCursor(false);
		// Keys let go while the screen had them would otherwise still read as held (a walk that never stops).
		Pc->FlushPressedKeys();
	}
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
	}
}

FVector ABackRoomGameMode::SeatMarkerTarget() const
{
	// The player's own chair while they're up from it (on a break, dealt in while walking about); none once their night's
	// over or before they've sat (spec 6.5: "TABLE 12 · SEAT 4 ↗ 18 m").
	if (!bLive || !Stage || bLiveOver || !bHeroUp)
	{
		return FVector::ZeroVector;
	}
	return Stage->HeroSeatMarker();
}

FString ABackRoomGameMode::TestDeskOpen(const FString& Window)
{
	if (bDeskUp)
	{
		CloseDesk();
	}
	OpenDesk(Window);
	return bDeskUp ? FString::Printf(TEXT("the %s's screen is up"), *Window) : FString::Printf(TEXT("the %s's screen can't open here (live: %d, a save: %d)"), *Window, bLive ? 1 : 0, Save.IsValid() ? 1 : 0);
}

FString ABackRoomGameMode::TestDeskKey(const FString& Key)
{
	if (Key == TEXT("Describe"))
	{
		// ss.Venue.Describe: the venue as it stands.
		const FVenueHost* V = VenueHost.Get();
		FString Out = FString::Printf(TEXT("Venue at %s: desk %s"), *ClockLabel(), bDeskUp ? *FString::Printf(TEXT("up (%s, event %d, pay %d)"), *DeskWindow, Desk.Selected(), Desk.PayChoice()) : TEXT("down"));
		if (Signup.IsValid())
		{
			Out += TEXT(", writing the player in");
		}
		if (!NextEntryId.IsEmpty())
		{
			Out += FString::Printf(TEXT(", next %s (%s)"), *NextEntryId, bNextCalled ? TEXT("called") : *Str(ss::net::TimeLabel(NextSeatingAt)));
		}
		static const TCHAR* Pages[4] = {TEXT("CLOCK"), TEXT("PAYOUTS"), TEXT("TODAY"), TEXT("LEADERS")};
		if (Stage && Stage->HasClockScreens() && V)
		{
			Out += FString::Printf(TEXT("\n  clocks: pages (stage %s, side %s for %.0f s%s), %d paints (%d turns unchanged, skipped), avg %.3f ms, peak %.3f ms a paint"),
				Pages[FMath::Clamp(V->StagePage, 0, 3)], Pages[FMath::Clamp(RoomClockPage, 0, 3)], RoomClockT, V->bHoldClock ? TEXT(", held on CLOCK") : TEXT(""), V->ClockRenders,
				V->ClockSkips, V->ClockRenders > 0 ? V->ClockMsSum / V->ClockRenders : 0.0, V->ClockMsPeak);
		}
		else
		{
			Out += TEXT("\n  clocks: the text lines");
		}
		Out += BadgeAtlas.IsValid() && BadgeAtlas->IsReady() ? FString::Printf(TEXT("\n  badge atlas: %d cells, %.2f ms to paint"), BadgeAtlas->NumCells(), BadgeAtlas->BuildMs)
															 : FString(TEXT("\n  badge atlas: none"));
		Out += FString::Printf(TEXT("\n  ambassador: %s, the crest on %d champions' lines"), Stage && Stage->HasAmbassadorBanner() ? TEXT("banner up") : TEXT("no banner"),
			Stage ? Stage->CrestedLines() : 0);
		const FVector M = SeatMarkerTarget();
		Out += M.IsZero() ? FString(TEXT("\n  seat marker: none")) : FString::Printf(TEXT("\n  seat marker: %s"), *M.ToCompactString());
		Out += FString::Printf(TEXT("\n  colour-up: %s"), RoomDirector.IsValid() ? TEXT("the director's") : TEXT("the floor's (no director)"));
		if (V)
		{
			Out += FString::Printf(TEXT("\n  today: %d events, %d leaders"), static_cast<int32>(V->Today.size()), static_cast<int32>(V->Leaders.size()));
			Out += FString::Printf(TEXT("\n  desk screen: %d frames drawn, avg %.3f ms, peak %.3f ms (the fill and the draw)"), V->DeskFrames,
				V->DeskFrames > 0 ? V->DeskMsSum / V->DeskFrames : 0.0, V->DeskMsPeak);
		}
		return Out;
	}
	if (!bDeskUp)
	{
		return TEXT("The desk screen isn't up");
	}
	Desk.Key(std::string(TCHAR_TO_UTF8(*Key)), LiveT);
	return FString();
}

void ABackRoomGameMode::FillDeskInfo(ss::ui::DeskInfo& Out) const
{
	using namespace VenueDetail;
	if (!Save.IsValid() || !Tourney)
	{
		return;
	}
	const FVenueHost* V = VenueHost.Get();
	Out.World = Minutes;
	Out.Bankroll = Save->BankrollCents;
	Out.PastEvents = PastLiveEvents;
	Out.Address = Utf8(AddressTo(Save));
	Out.Card = ss::ui::badgeart::ClubCardOf(*Save, Minutes);
	if (Out.Card.Name.empty())
	{
		Out.Card.Name = Save->HeroName;
	}
	Out.CardPrinting = V && V->bPrinting;
	Out.Card.New = Out.CardPrinting; // the foil catches the light as it comes out
	Out.Duties = ss::sponsor::Appearances(Save->Life.Sponsors, Minutes - ss::net::MinutesPerDay, Minutes + ss::net::MinutesPerDay);
	const bool bAnswer = V && !V->Clerk.IsEmpty() && LiveT - V->ClerkAt < 8.0f;
	const ss::life::LiveEntry* Mine = ss::live::EntryFor(Save->Life, Utf8(LiveEntryId));
	if (DeskWindow == TEXT("cage"))
	{
		// The receipt: place, field, prize, when; then the lines the settlement wrote past the prize (a backer's share, a
		// sponsor's cut or bonus), oldest first, as the cage counts them out.
		Out.Entry = Mine;
		Out.PrizeCents = HeroPrizeCents;
		Out.Place = HeroPlace;
		Out.Entrants = Tourney->Spec.Entrants;
		Out.FinishedAt = Mine && Mine->FinishedAt > 0.0 ? Mine->FinishedAt : BustMinutes;
		const double At = Out.FinishedAt;
		for (auto It = Save->Life.Ledger.rbegin(); It != Save->Life.Ledger.rend(); ++It)
		{
			const bool bSettle = FMath::Abs(It->At - At) < 0.01 || FMath::Abs(It->At - BustMinutes) < 0.01;
			if (bSettle && (It->Kind == LedgerPrize || It->Kind == ss::life::LedgerBacking || It->Kind == ss::life::LedgerSponsors))
			{
				Out.Receipt.push_back(*It);
			}
		}
		Out.ClerkLine = bAnswer ? Utf8(V->Clerk)
					  : bCollected ? std::string("You're all paid up, hon. Don't spend it all at once.")
								   : Utf8(FString::Printf(TEXT("Congratulations%s. Let me count it out for you."), *AddressTo(Save)));
		return;
	}
	// The desk: tonight's own event while the player's in it (how it's paid, the night as it stands), the next one they're
	// in, or what's open to them now with who could pay for each.
	FString Line;
	const ss::life::LiveEntry* Next = NextEntryId.IsEmpty() ? nullptr : ss::live::EntryFor(Save->Life, Utf8(NextEntryId));
	if (!bLiveOver)
	{
		ss::ui::DeskEvent E;
		E.O = ss::live::FindOccurrence(Utf8(LiveEntryId));
		if (Mine)
		{
			E.Pay.push_back({Mine->Fund, std::string()});
		}
		Out.Open.push_back(E);
		Out.Fields.push_back(Tourney->Spec.Entrants);
		Out.Entry = Mine;
		if (Mine && VenueHost)
		{
			// Tonight's entry as played (YOU'RE IN, who paid, no refund): the save in memory never holds the night's
			// checkpoint, so the entry itself would read as one that hasn't sat. (Copied into the host's own, a few fields.)
			ss::life::LiveEntry& Sat = VenueHost->Seated;
			Sat.Id = Mine->Id;
			Sat.Name = Mine->Name;
			Sat.State = Mine->State;
			Sat.Fund = Mine->Fund;
			Sat.PaidCents = Mine->PaidCents;
			Sat.Checkpoint = "sat";
			Out.Entry = &Sat;
		}
		const ss::TPlayer& Me = Tourney->Hero();
		Out.Seat = Utf8(FString::Printf(TEXT("TABLE %d · SEAT %d · %s CHIPS"), Me.TableId, ss::live::SeatLabel(Me.Seat), *Num(Tourney->Spec.StartingStack)));
		const ss::Level& L = Tourney->CurrentLevel();
		const int32 Every = FMath::Max(1, Tourney->Spec.BreakEvery);
		const int32 NextBreak = (Tourney->LevelIndex / Every + 1) * Every + 1;
		const FString Status = bCancelled ? FString(TEXT("Not enough of us tonight, so it's off. Your money's back with you."))
										  : FString::Printf(TEXT("%d left, %d paid. Level %d, %s and %s; the next break's before level %d."), Tourney->Remaining,
												Tourney->PaidPlaces(), Tourney->LevelIndex + 1, *Num(L.Sb), *Num(L.Bb), NextBreak);
		const FString Hello = Out.Card.Tier >= 1 ? Str(ss::badge::DeskLine(Out.Card.Tier, false, Utf8(AddressTo(Save)))) + TEXT(" ")
							  : PastLiveEvents == 0 ? FString(TEXT("First time with us? Welcome to the Embercrest. "))
													: FString();
		Line = Hello + Status;
	}
	else if (Next)
	{
		ss::ui::DeskEvent E;
		E.O = ss::live::FindOccurrence(Utf8(NextEntryId));
		E.Pay.push_back({Next->Fund, std::string()});
		Out.Open.push_back(E);
		Out.Fields.push_back(Next->Entrants);
		Out.Entry = Next;
		if (E.O.Valid())
		{
			Out.Seat = Utf8(FString::Printf(TEXT("SEATS AT %s · %s CHIPS"), *Str(ss::net::TimeLabel(NextSeatingAt)), *Num(E.O.T->StartingStack)));
		}
		Line = bNextCalled ? FString::Printf(TEXT("They're seating the %s now. Go on, find your table."), *NextName)
						   : FString::Printf(TEXT("You're all set for the %s. Seats open at %s; the floor will call it. The bar's open, the terrace too."), *NextName,
								 *Str(ss::net::TimeLabel(NextSeatingAt)));
	}
	else
	{
		FString WhyNot;
		if (!bCancelled && !ss::live::ActiveEntry(Save->Life))
		{
			for (const ss::live::Occurrence& O : ss::live::Occurrences(LiveDay))
			{
				if (Str(O.Id) == LiveEntryId || ss::live::EntryFor(Save->Life, O.Id) || Minutes >= O.LateRegEnds)
				{
					continue;
				}
				const FString Why = EventWhyNot(O, Minutes, Energy);
				if (!Why.IsEmpty())
				{
					WhyNot = WhyNot.IsEmpty() ? Why : WhyNot;
					continue;
				}
				ss::ui::DeskEvent E;
				E.O = O;
				E.Pay = DeskPay(*Save, O, Minutes);
				Out.Open.push_back(E);
				Out.Fields.push_back(0);
			}
		}
		if (!Out.Open.empty())
		{
			const ss::live::Occurrence& O = Out.Open[std::clamp(Desk.Selected(), 0, static_cast<int>(Out.Open.size()) - 1)].O;
			const ss::live::EventTemplate& T = *O.T;
			const FString When = Minutes < O.Start ? TEXT("cards at ") + Str(ss::net::TimeLabel(O.Start)) : TEXT("late registration till ") + Str(ss::net::TimeLabel(O.LateRegEnds));
			Line = FString::Printf(TEXT("The %s is still open%s. %s in chips, %s. Want in?"), *Str(T.Short), *AddressTo(Save), *Num(T.StartingStack), *When);
		}
		else if (!WhyNot.IsEmpty())
		{
			// (The clerk says why; the events' side keeps the desk's own "nothing's open" with the schedule's board.)
			Line = WhyNot;
		}
		else
		{
			FString NextUp;
			for (int32 Day = LiveDay; Day <= LiveDay + 1 && NextUp.IsEmpty(); ++Day)
			{
				for (const ss::live::Occurrence& O : ss::live::Occurrences(Day))
				{
					if (O.Start > Minutes && NextUp.IsEmpty())
					{
						NextUp = FString::Printf(TEXT("the %s, %s %s"), *Str(O.T->Name), Day == LiveDay ? TEXT("tonight at") : TEXT("tomorrow at"), *Str(ss::net::TimeLabel(O.Start)));
					}
				}
			}
			Line = bCancelled ? FString(TEXT("Not enough of us tonight, so it's off. Your money's back with you."))
			   : NextUp.IsEmpty() ? FString(TEXT("Thanks for coming in. Get home safe."))
								  : FString::Printf(TEXT("Thanks for playing. Next one's %s."), *NextUp);
		}
	}
	Out.ClerkLine = Utf8(bAnswer ? V->Clerk : Signup.IsValid() ? FString(TEXT("One second, I'm writing you in.")) : V && V->bPrinting ? V->PrintLine : Line);
}

void ABackRoomGameMode::TickDesk(float RealDt)
{
	using namespace VenueDetail;
	if (!bDeskUp || !VenueHost)
	{
		return;
	}
	FVenueHost& V = *VenueHost;
	// Away from the desk now: the night ending in a walk (the walk out to the doors too), the room opening again for the
	// next event, the way home.
	if (!bLive || !Save.IsValid() || !Tourney || bGoingHome || bOpeningNext || bExitNow || bWaitingOut || bWalkingOut || !V.Widget)
	{
		CloseDesk();
		return;
	}
	const double T0 = FPlatformTime::Seconds();
	UGameViewportClient* Viewport = V.Viewport.Get();
	// The game took the focus back (the pause menu closing hands it to the viewport): the screen takes it again. Focus
	// anywhere else (the editor's own windows in Play-In-Editor) is left where it is.
	if (Viewport && FSlateApplication::IsInitialized())
	{
		const TSharedPtr<SWidget> Focused = FSlateApplication::Get().GetKeyboardFocusedWidget();
		const TSharedPtr<SWidget> GameWidget = Viewport->GetGameViewportWidget();
		if (!Focused.IsValid() || Focused == GameWidget)
		{
			if (APlayerController* Pc = UGameplayStatics::GetPlayerController(this, 0))
			{
				FInputModeUIOnly InputMode;
				InputMode.SetWidgetToFocus(V.Widget);
				InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
				Pc->SetInputMode(InputMode);
				Pc->SetShowMouseCursor(true);
			}
			FSlateApplication::Get().SetKeyboardFocus(V.Widget, EFocusCause::SetDirectly);
		}
	}
	// The printer's chirp as the new card starts out of the slot.
	if (V.PrintSoundAt >= 0.0f && LiveT >= V.PrintSoundAt)
	{
		V.PrintSoundAt = -1.0f;
		if (Table && Table->GetAudio())
		{
			Table->GetAudio()->PlayEffect(ss::audio::Effect::CardPrint, 0.8f);
		}
	}
	// The screen, drawn on the HUD's logical canvas (1,080 tall, as wide as the window).
	FVector2D ViewSize(1920.0, 1080.0);
	if (Viewport)
	{
		FVector2D Measured;
		Viewport->GetViewportSize(Measured);
		if (Measured.X > 0.0 && Measured.Y > 0.0)
		{
			ViewSize = Measured;
		}
	}
	ss::ui::DeskInfo Info;
	FillDeskInfo(Info);
	const float LogicalH = 1080.0f;
	const float LogicalW = LogicalH * static_cast<float>(ViewSize.X / ViewSize.Y);
	TSharedPtr<ss::ui::DrawList> List = V.Hint.Make();
	{
		ss::ui::Canvas Cv(*List, V.Measurer, LogicalW, LogicalH, static_cast<float>(ViewSize.Y) / LogicalH);
		Desk.Draw(Cv, Info, LiveT);
	}
	V.Hint.Note(*List);
	V.Widget->SetDrawList(List);
	// The frame's cost (the fill and the draw; what a request then does, a save or the signup's copy, is the action's).
	const double Ms = (FPlatformTime::Seconds() - T0) * 1000.0;
	V.DeskMsSum += Ms;
	V.DeskMsPeak = FMath::Max(V.DeskMsPeak, Ms);
	++V.DeskFrames;
	// What the player asked for, carried out through the desk's own paths.
	auto Say = [this, &V](const FString& Line) {
		V.Clerk = Line;
		V.ClerkAt = LiveT;
	};
	for (const ss::ui::DeskRequest& R : Desk.TakeRequests())
	{
		switch (R.K)
		{
		case ss::ui::DeskRequest::Register:
		{
			const ss::live::Occurrence O = ss::live::FindOccurrence(R.Id);
			FString Why;
			if (Signup.IsValid())
			{
				Why = TEXT("One second, I'm still writing you in.");
			}
			else if (!bLiveOver || !NextEntryId.IsEmpty() || ss::live::ActiveEntry(Save->Life))
			{
				Why = TEXT("One event at a time, hon. You've got a seat already.");
			}
			else if (!O.Valid())
			{
				Why = TEXT("There's no such event.");
			}
			else
			{
				// Whoever pays exactly as the screen drew it: the clerk's check now, and live::Register's own when the
				// field's in (an offer that lapsed meanwhile, a cover used up: refused there with its line).
				Why = EventWhyNot(O, Minutes, Energy);
				if (Why.IsEmpty())
				{
					Why = Str(ss::live::CanRegister(Save->BankrollCents + 2 * ss::live::BusFareCents, Save->Life, O, Minutes - ss::live::TravelMinutes, R.Fund));
				}
			}
			if (!Why.IsEmpty())
			{
				Say(Why);
				UE_LOG(LogEmbercrest, Log, TEXT("The desk won't register %s: %s"), *Str(R.Id), *Why);
				break;
			}
			V.DeskFund = R.Fund;
			V.bDeskFund = true;
			DeskSignup(Str(O.Id));
			// (DeskSignup took the option if it started; it never carries over to a later signup that didn't come from here.)
			V.bDeskFund = false;
			Say(Signup.IsValid() ? FString(TEXT("Let me write you in.")) : FString(TEXT("I can't write you into that one right now.")));
			break;
		}
		case ss::ui::DeskRequest::Unregister:
			if (!NextEntryId.IsEmpty() && Str(R.Id) == NextEntryId && !Signup.IsValid())
			{
				const FString Name = NextName;
				const ss::life::LiveEntry* E = ss::live::EntryFor(Save->Life, R.Id);
				const int64 Back = E ? E->PaidCents : 0;
				const bool bOwn = !E || E->Fund.From == ss::life::Paid::Own;
				LiveUnregister();
				Say(bOwn && Back > 0 ? FString::Printf(TEXT("Off the list for the %s. %s back, same as it came in."), *Name, *Dollars(Back))
									 : FString::Printf(TEXT("Off the list for the %s. Whoever put you in gets it back."), *Name));
			}
			else
			{
				Say(Signup.IsValid() ? FString(TEXT("One second, I'm still writing you in.")) : FString(TEXT("Once you've played a hand, that money's in the game.")));
			}
			break;
		case ss::ui::DeskRequest::Collect:
			if (bLiveOver && HeroPrizeCents > 0 && !bCollected)
			{
				bCollected = true;
				Say(FString::Printf(TEXT("Congratulations%s. %s, cash. Count it with me... there you go."), *AddressTo(Save), *Dollars(HeroPrizeCents)));
				if (Table && Table->GetAudio())
				{
					Table->GetAudio()->Play(ss::SoundId::Cash, 0.8f);
				}
				UE_LOG(LogEmbercrest, Log, TEXT("The cage counts out %s"), *Dollars(HeroPrizeCents));
			}
			break;
		case ss::ui::DeskRequest::Appear:
		{
			// Meet the regulars (spec 3.4): Denise walks the ambassador round the room. On the rail the room plays on for the
			// twenty minutes; dealt in and up from the chair, it's a quick lap and back.
			const std::string Why = ss::sponsor::Appear(Save->Life.Sponsors, R.Id, Minutes);
			if (!Why.empty())
			{
				Say(Str(Why));
				break;
			}
			V.bSaveDue = true;
			Say(TEXT("Come on, I'll walk you round. Handshakes at every table, a picture by the board, and they'll know your face."));
			UE_LOG(LogEmbercrest, Log, TEXT("Meet the regulars at %s (%s)"), *Str(R.Id), *ClockLabel());
			if (bRailing && Tourney && !Tourney->bFinished && NextEntryId.IsEmpty())
			{
				RailCatchUp(Minutes + 20.0);
			}
			break;
		}
		case ss::ui::DeskRequest::Tip:
			break; // (P2: tipping at the cage)
		case ss::ui::DeskRequest::Leave:
		default:
			CloseDesk();
			return;
		}
	}
	if (!Desk.IsOpen())
	{
		CloseDesk();
	}
}

// ------------------------------------------------------------------ the room's clock screens (next_segment_spec 6.5)

void ABackRoomGameMode::UpdateRoomClocks(float RealDt)
{
	using namespace VenueDetail;
	if (!VenueHost || !bLive || !Stage || !Tourney || !Save.IsValid())
	{
		return;
	}
	FVenueHost& V = *VenueHost;
	if (!V.bClocksTried)
	{
		V.bClocksTried = true;
		// Two pages, 1,024 x 576 each (spec 10): one for the clocks either side of the stage, one for the side walls'.
		if (CVarClockScreens.GetValueOnGameThread() != 0 && FSlateTextMeasurer::IsAvailable())
		{
			UTextureRenderTarget2D* OnStage = FCoreCanvasTarget::MakeTarget(this, ClockW, ClockH);
			UTextureRenderTarget2D* OnSide = FCoreCanvasTarget::MakeTarget(this, ClockW, ClockH);
			if (OnStage && OnSide)
			{
				RoomClockTargets = {OnStage, OnSide};
				if (!RoomClockCanvas)
				{
					RoomClockCanvas = MakeShared<FCoreCanvasTarget>();
				}
				RoomClockPage = 0;
				RoomClockT = 0.0f;
				V.RenderT = 0.0f;
				V.RenderTurn = 1; // both painted below before the screens show them
			}
		}
	}
	if (RoomClockTargets.Num() < 2 || !RoomClockTargets[0] || !RoomClockTargets[1])
	{
		return;
	}
	const bool bFinished = Tourney->bFinished || bCancelled;
	// The side walls' rotation: CLOCK 40 s, then PAYOUTS, TODAY or LEADERS 10 s in turn; never off CLOCK in a level's last
	// minute or hand for hand (spec 5.6, 6.5). Once the event's over the base page is the results (PAYOUTS, posted).
	const int32 Base = bFinished ? 1 : 0;
	RoomClockT += RealDt;
	const int32 Forced = CVarClockPage.GetValueOnGameThread();
	if (Forced >= 0 && Forced <= 3)
	{
		RoomClockPage = Forced;
	}
	else if (RoomClockPage == Base)
	{
		if (RoomClockT >= ClockSeconds && !V.bHoldClock)
		{
			// The next page in turn that has something to show (LEADERS only with the month's leaders in the snapshot).
			for (int32 Tries = 0; Tries < 3; ++Tries)
			{
				const int32 Page = 1 + (V.NextExtra++ % 3);
				if (Page != Base && (Page != 3 || !V.Leaders.empty()))
				{
					RoomClockPage = Page;
					break;
				}
			}
			RoomClockT = 0.0f;
		}
	}
	else if (RoomClockT >= PageSeconds || V.bHoldClock)
	{
		RoomClockPage = Base;
		RoomClockT = 0.0f;
	}
	V.StagePage = Base;
	// Painted in turn, each about twice a second.
	V.RenderT -= RealDt;
	if (V.RenderT > 0.0f)
	{
		return;
	}
	V.RenderT = RenderEvery;
	const double T0 = FPlatformTime::Seconds();
	const ss::live::Occurrence O = ss::live::FindOccurrence(Utf8(LiveEntryId));
	const double BreakLeft = bOnBreak ? LiveBreakMinutes * 60.0 * (1.0 - static_cast<double>(BreakT) / BreakRealSeconds) : 0.0;
	// The room's public facts (no entry: no funding row, no seat), the same live::Clock as the HUD's clock card.
	const ss::live::ClockFacts F = ss::live::Clock(*Tourney, O, nullptr, Minutes, LevelTimeLeft(), bOnBreak, FMath::Max(0.0, BreakLeft));
	const bool bStarted = !(Phase == EBackRoomPhase::Arriving && !bAnnouncedStart && Tourney->Tick == 0);
	V.bHoldClock = !bFinished && (F.HandForHand || (bStarted && !F.OnBreak && F.LevelSecondsLeft < 60.0));
	std::string Event = O.Valid() ? Upper(O.T->Short) + " " + Price(O.T->BuyInCents) : std::string();
	if (Tourney->bFinished)
	{
		for (const ss::TPlayer& P : Tourney->Players)
		{
			if (P.Place == 1)
			{
				Event += " \xC2\xB7 CHAMPION " + Upper(P.IsHero ? Utf8(VenueDetail::HeroCalled(*Save)) : P.Name);
			}
		}
	}
	const bool bFirst = V.ClockRenders == 0;
	// What a page shows, as one number: a target whose page would come out the same isn't painted again (the clock's
	// second ticks once a second; PAYOUTS, TODAY and LEADERS change rarely). HAND FOR HAND's pill pulses, so it always
	// paints, and every target is painted at least every few seconds whatever the number says.
	auto PageKey = [&](int32 Page) {
		uint64 H = 1469598103934665603ull;
		auto Mix = [&H](int64 X) {
			for (int32 B = 0; B < 8; ++B)
			{
				H = (H ^ static_cast<uint64>((X >> (B * 8)) & 0xff)) * 1099511628211ull;
			}
		};
		auto MixText = [&H, &Mix](const std::string& S) {
			for (const char Ch : S)
			{
				H = (H ^ static_cast<uint8>(Ch)) * 1099511628211ull;
			}
			Mix(static_cast<int64>(S.size()));
		};
		Mix(Page);
		MixText(Event);
		switch (Page)
		{
		case 0:
			Mix(F.Level);
			Mix(F.OnBreak ? 1 : 0);
			Mix(FMath::FloorToInt64(F.OnBreak ? F.BreakSecondsLeft : F.LevelSecondsLeft));
			Mix(F.Now.Sb);
			Mix(F.Now.Bb);
			Mix(F.Now.Ante);
			Mix(F.Next.Sb);
			Mix(F.Next.Bb);
			Mix(F.Next.Ante);
			MixText(F.ColorUp);
			Mix(F.Remaining);
			Mix(F.Entrants);
			Mix(F.Average);
			Mix(F.Projected ? 1 : 0);
			Mix(F.PoolCents);
			Mix(F.BreakBeforeLevel);
			Mix(FMath::FloorToInt64(F.SecondsToBreak));
			Mix(F.Paid);
			break;
		case 1:
			Mix(F.Paid);
			Mix(F.PoolCents);
			Mix(F.Projected ? 1 : 0);
			Mix(F.MinCashCents);
			Mix(F.Remaining);
			break;
		case 2:
			Mix(V.TodayRev);
			break;
		default:
			Mix(static_cast<int64>(V.Leaders.size()));
			break;
		}
		return H;
	};
	int32 Painted = 0;
	auto Paint = [&](int32 Which, int32 Page) {
		const uint64 Key = PageKey(Page);
		const bool bPulse = Page == 0 && F.HandForHand;
		if (!bFirst && !bPulse && Key == V.PaintedKey[Which] && LiveT - V.PaintedAt[Which] < 5.0f && V.PaintedAt[Which] <= LiveT)
		{
			return;
		}
		V.PaintedKey[Which] = Key;
		V.PaintedAt[Which] = LiveT;
		UTextureRenderTarget2D* Target = RoomClockTargets[Which];
		TSharedPtr<ss::ui::DrawList> List = MakeShared<ss::ui::DrawList>();
		{
			ss::ui::Canvas C(*List, V.Measurer, static_cast<float>(ClockW), static_cast<float>(ClockH), 1.0f);
			const std::vector<ss::live::PayRow> Pay = Page == 1 ? ss::live::PayLadder(*Tourney) : std::vector<ss::live::PayRow>();
			ss::ui::DrawRoomClock(C, F, Page, V.Today, V.Leaders, Save->HeroName, LiveT, Event, Pay);
		}
		RoomClockCanvas->Render(Target, List);
		++Painted;
	};
	if (bFirst)
	{
		// The first paint: both pages, then the screens switch over to them (and their text lines go).
		Paint(0, V.StagePage);
		Paint(1, RoomClockPage);
	}
	else
	{
		V.RenderTurn = 1 - V.RenderTurn;
		Paint(V.RenderTurn, V.RenderTurn == 0 ? V.StagePage : RoomClockPage);
	}
	// (Both targets hold a page from the first turn on, so the screens can take them, and a new glow, on any turn.)
	const float Glow = FMath::Max(0.0f, CVarScreenGlow.GetValueOnGameThread());
	if (!Stage->HasClockScreens() || !FMath::IsNearlyEqual(Glow, V.AppliedGlow))
	{
		V.AppliedGlow = Glow;
		Stage->SetClockScreens(RoomClockTargets[0], RoomClockTargets[1], Glow);
	}
	if (Painted == 0)
	{
		++V.ClockSkips;
		return;
	}
	const double Ms = (FPlatformTime::Seconds() - T0) * 1000.0;
	V.ClockMsSum += Ms;
	V.ClockMsPeak = FMath::Max(V.ClockMsPeak, Ms);
	++V.ClockRenders;
}
