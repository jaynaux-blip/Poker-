// Update 2's debug commands (update 2 SPEC §6.6): autoplay, Rerun, the rival sites and the fresh start. The SegmentCommands.cpp
// pattern: each command passes its verb and its arguments on as one line, the verb in lower case and the arguments as typed
// ("ss.Auto.Skip hands 50" is "skip hands 50"), and the answer is logged a line at a time.
//
// Where they run:
//   ss.Auto.*        at home (Night One) to the session's DebugAuto, for the laptop's table in front; in the card room (the
//                    Embercrest, Dee's game) to the table's own FAutoHost (ue-auto's, BackRoomAuto.cpp): the room has a
//                    save, not a session
//   ss.Video.*, ss.Site.*, ss.World.Start   at home only, to the session's DebugVideo, DebugSite and DebugWorld
//   ss.Career.New    on the title screen: a new career through the creator's path (ANightOneGameMode::TestNewCareer)
//
//   ss.Auto.Style <id> | .On | .Off | .Speed <1|2|4|8> | .Skip <break|bubble|final|result|hands N|closing> | .TakeOver
//          | .Sharp <0.5..1> | .Form <70..100> | .Describe | .Parity <hands>
//   ss.Video.Describe | .Candidates | .Fake <kind> <online|embercrest|dees|skip> | .Upload <cand> <format> <title> <frame>
//          | .Advance <days> | .Grant <subs> <hours> <views>
//   ss.Site.Describe | .Invite <site> | .Launch <site> | .Deposit <site> <dollars> <method>
//          | .Withdraw <site> <dollars> <method> | .Advance <hours>
//   ss.Career.New <name> <established|blank>
//   ss.World.Start
//
// Every body behind them is a feature track's (the session's Debug* in SessionAuto.cpp, SessionVideo.cpp, SessionSites.cpp
// and SessionWorld.cpp; FAutoHost::Command): until a track lands, its commands answer with the stub's line.

#include "BackRoomAuto.h"
#include "BackRoomGameMode.h"
#include "CoreMinimal.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "NightOneGame.h"
#include "NightOneGameMode.h"
#include "ShortStack/Game/Session.h"

#include <string>

DEFINE_LOG_CATEGORY_STATIC(LogUpdate2, Log, All);

namespace Update2Detail
{
/** The career's session in this world: Night One's (the apartment), or none. */
ss::Session* Update2SessionIn(UWorld* World)
{
	ANightOneGameMode* Mode = World ? World->GetAuthGameMode<ANightOneGameMode>() : nullptr;
	FNightOneGame* Game = Mode ? Mode->GetGame() : nullptr;
	return Game ? &Game->Session : nullptr;
}

enum class EUpdate2Feature : uint8
{
	Auto,
	Video,
	Site,
	World,
};

const TCHAR* Update2FeatureName(EUpdate2Feature Feature)
{
	switch (Feature)
	{
	case EUpdate2Feature::Auto: return TEXT("ss.Auto");
	case EUpdate2Feature::Video: return TEXT("ss.Video");
	case EUpdate2Feature::Site: return TEXT("ss.Site");
	default: return TEXT("ss.World");
	}
}

void Update2Log(const TCHAR* Name, const FString& Line, const FString& Answer)
{
	TArray<FString> Lines;
	Answer.ParseIntoArrayLines(Lines, false);
	UE_LOG(LogUpdate2, Display, TEXT("%s %s"), Name, *Line);
	for (const FString& L : Lines)
	{
		UE_LOG(LogUpdate2, Display, TEXT("  %s"), *L);
	}
}

void Update2Forward(EUpdate2Feature Feature, const TCHAR* Verb, const TArray<FString>& Args, UWorld* World)
{
	const TCHAR* Name = Update2FeatureName(Feature);
	FString Line = FString(Verb).ToLower();
	for (const FString& A : Args)
	{
		Line += TEXT(" ") + A;
	}
	if (ss::Session* S = Update2SessionIn(World))
	{
		const std::string In(TCHAR_TO_UTF8(*Line));
		const std::string Out = Feature == EUpdate2Feature::Auto	? S->DebugAuto(In)
							  : Feature == EUpdate2Feature::Video ? S->DebugVideo(In)
							  : Feature == EUpdate2Feature::Site  ? S->DebugSite(In)
																  : S->DebugWorld(In);
		Update2Log(Name, Line, FString(UTF8_TO_TCHAR(Out.c_str())));
		return;
	}
	// The card room: the plan at the table is the room's own.
	ABackRoomGameMode* Room = World ? World->GetAuthGameMode<ABackRoomGameMode>() : nullptr;
	if (FAutoHost* Host = Feature == EUpdate2Feature::Auto ? FAutoHost::Of(Room) : nullptr)
	{
		Update2Log(Name, Line, Host->Command(Line));
		return;
	}
	UE_LOG(LogUpdate2, Warning, TEXT("%s.%s: %s"), Name, Verb,
		Feature == EUpdate2Feature::Auto ? TEXT("at home (Night One: the laptop's table in front) or at a card room table")
										 : TEXT("only at home (Night One), where the career's session lives"));
}

/** One console command, "ss.<Feature>.<Verb>", forwarding as above. */
struct FUpdate2Command
{
	FUpdate2Command(EUpdate2Feature Feature, const TCHAR* Command, const TCHAR* Verb, const TCHAR* Help)
		: Cmd(Command, Help, FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([Feature, Verb](const TArray<FString>& Args, UWorld* World) { Update2Forward(Feature, Verb, Args, World); }))
	{
	}
	FAutoConsoleCommandWithWorldAndArgs Cmd;
};

// Autoplay (update 2 §2.6, §2.12).
FUpdate2Command AutoStyle(EUpdate2Feature::Auto, TEXT("ss.Auto.Style"), TEXT("Style"),
	TEXT("ss.Auto.Style <autopilot|tag|lag|nit|maniac|reg|gto|exploit|pushfold|icm|bounty>: the plan's style at the table in front (testing)."));
FUpdate2Command AutoOn(EUpdate2Feature::Auto, TEXT("ss.Auto.On"), TEXT("On"), TEXT("Autoplay on at the table in front, in its style: the plan takes the seat from the next decision (testing)."));
FUpdate2Command AutoOff(EUpdate2Feature::Auto, TEXT("ss.Auto.Off"), TEXT("Off"), TEXT("Autoplay off at the table in front: the seat is the player's from the next decision (testing)."));
FUpdate2Command AutoSpeed(EUpdate2Feature::Auto, TEXT("ss.Auto.Speed"), TEXT("Speed"), TEXT("ss.Auto.Speed <1|2|4|8>: how fast the plan's hands are watched (testing)."));
FUpdate2Command AutoSkip(EUpdate2Feature::Auto, TEXT("ss.Auto.Skip"), TEXT("Skip"),
	TEXT("ss.Auto.Skip <break|bubble|final|result|hands N|closing>: the plan skips ahead to there, stopping between hands (testing)."));
FUpdate2Command AutoTakeOver(EUpdate2Feature::Auto, TEXT("ss.Auto.TakeOver"), TEXT("TakeOver"), TEXT("Takes the wheel at the table in front: this hand is the player's (testing)."));
FUpdate2Command AutoSharp(EUpdate2Feature::Auto, TEXT("ss.Auto.Sharp"), TEXT("Sharp"), TEXT("ss.Auto.Sharp <0.5..1>: pins the plan's Sharpness (dev only)."));
FUpdate2Command AutoForm(EUpdate2Feature::Auto, TEXT("ss.Auto.Form"), TEXT("Form"), TEXT("ss.Auto.Form <70..100>: sets the player's Form (dev only)."));
FUpdate2Command AutoDescribe(EUpdate2Feature::Auto, TEXT("ss.Auto.Describe"), TEXT("Describe"),
	TEXT("The plan at the table in front: style, mode, speed, Sharpness and its causes, Form, tilt, decisions by the plan and by the player, the observed stats and notes, the reads applied tonight."));
FUpdate2Command AutoParity(EUpdate2Feature::Auto, TEXT("ss.Auto.Parity"), TEXT("Parity"),
	TEXT("ss.Auto.Parity <hands>: plays the next hands watched at 8x, restores the checkpoint from before them, skips the same hands headless, and prints both hashes (testing)."));
// Rerun (§4).
FUpdate2Command VideoDescribe(EUpdate2Feature::Video, TEXT("ss.Video.Describe"), TEXT("Describe"), TEXT("Rerun: the channel, the partner gate's bars, the balance and the videos."));
FUpdate2Command VideoCandidates(EUpdate2Feature::Video, TEXT("ss.Video.Candidates"), TEXT("Candidates"), TEXT("Rerun: the open highlight candidates, best first (the newest of a tie)."));
FUpdate2Command VideoFake(EUpdate2Feature::Video, TEXT("ss.Video.Fake"), TEXT("Fake"),
	TEXT("ss.Video.Fake <kind> <online|embercrest|dees|skip>: a candidate of that kind and source, cut from a scripted hand (dev only)."));
FUpdate2Command VideoUpload(EUpdate2Feature::Video, TEXT("ss.Video.Upload"), TEXT("Upload"),
	TEXT("ss.Video.Upload <candidate> <format> <title> <frame>: publishes without the laptop; the edit's time still passes (testing)."));
FUpdate2Command VideoAdvance(EUpdate2Feature::Video, TEXT("ss.Video.Advance"), TEXT("Advance"), TEXT("ss.Video.Advance <days>: the channel's views move on (testing)."));
FUpdate2Command VideoGrant(EUpdate2Feature::Video, TEXT("ss.Video.Grant"), TEXT("Grant"),
	TEXT("ss.Video.Grant <subs> <hours> <views>: subscribers and watch hours now, and a video that gains the views over 30 days (dev only)."));
// The rival sites (§5).
FUpdate2Command SiteDescribe(EUpdate2Feature::Site, TEXT("ss.Site.Describe"), TEXT("Describe"), TEXT("The poker sites: accounts, money in transit, each invitation's gate and how close it is."));
FUpdate2Command SiteInvite(EUpdate2Feature::Site, TEXT("ss.Site.Invite"), TEXT("Invite"), TEXT("ss.Site.Invite <site>: that site's invitation arrives now, past its gate (testing)."));
FUpdate2Command SiteLaunch(EUpdate2Feature::Site, TEXT("ss.Site.Launch"), TEXT("Launch"), TEXT("ss.Site.Launch <site>: that site launches in the world today (testing)."));
FUpdate2Command SiteDeposit(EUpdate2Feature::Site, TEXT("ss.Site.Deposit"), TEXT("Deposit"), TEXT("ss.Site.Deposit <site> <dollars> <method>: money from the bank onto a site, as the Cashier does (testing)."));
FUpdate2Command SiteWithdraw(EUpdate2Feature::Site, TEXT("ss.Site.Withdraw"), TEXT("Withdraw"),
	TEXT("ss.Site.Withdraw <site> <dollars> <method>: a withdrawal requested, as the Cashier does; it arrives days later (testing)."));
FUpdate2Command SiteAdvance(EUpdate2Feature::Site, TEXT("ss.Site.Advance"), TEXT("Advance"), TEXT("ss.Site.Advance <hours>: the sites' calendar moves on (transfers, promotions, invitations) (testing)."));
// The fresh start (§3).
FUpdate2Command WorldStart(EUpdate2Feature::World, TEXT("ss.World.Start"), TEXT("Start"), TEXT("The world's start (established or a fresh start), its version, the sites' launch days and the teams' sizes."));

FAutoConsoleCommandWithWorldAndArgs CareerNewCmd(TEXT("ss.Career.New"),
	TEXT("ss.Career.New <name> <established|blank>: on the title screen, a new career through the creator's path, in an established world or a fresh start (testing)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		ANightOneGameMode* Mode = World ? World->GetAuthGameMode<ANightOneGameMode>() : nullptr;
		const FString How = Args.Num() > 1 ? Args[1].ToLower() : FString(TEXT("established"));
		if (!Mode || Args.Num() < 1 || (How != TEXT("established") && How != TEXT("blank")))
		{
			UE_LOG(LogUpdate2, Warning, TEXT("ss.Career.New <name> <established|blank>: on Night One's title screen"));
			return;
		}
		const bool bBlank = How == TEXT("blank");
		UE_LOG(LogUpdate2, Display, TEXT("ss.Career.New %s: %s (nothing happens once a career is under way)"), *Args[0],
			bBlank ? TEXT("a fresh start") : TEXT("an established world"));
		Mode->TestNewCareer(Args[0], bBlank);
	}));
} // namespace Update2Detail
