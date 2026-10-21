// Update 3's debug commands (update 3 SPEC §7.6): the laptop opened without a click, the rival sites' play book and Rerun
// 2.0's levers and numbers. Update2Commands.cpp's pattern: each command passes its verb and its arguments on as one line,
// the verb in lower case and the arguments as typed ("ss.Video.Repack v12 title 2" is "repack v12 title 2"), and the
// answer is logged a line at a time.
//
// They run at home only (Night One), where the career's session and the laptop live:
//   ss.Laptop.Open <app> [page] [site]   to the laptop's RiverLine::DebugOpen, as typed ("riverline lobby ignite",
//          "riverline league runittwice", "kast channel", "video studio", "bank", "burner sam"): the app, its page and a
//          site's client open without a click, so a PIE shot never depends on where a button sits (ke * TestLook 1 0 0
//          leans in to see it)
//   ss.Site.Play <verb> [args]   to the session's DebugSitePlay, the first word in lower case ("ss.Site.Play" alone is
//          "describe"): describe | fuse <tier> <m> | wheel | dares | league | gov | sparks <n> | flame <days> | level <n>
//          | scalp <npc> | league-draw | league-close | callout <npc> | grant <site> <cents> | rake <site> <cents>
//          (fuse, sparks, flame, level, scalp and rake are dev only; "ss.Site.Describe" and the rest stay Update 2's)
//   ss.Video.Stats <id> | .Checkin [id] | .Trends | .Ideas | .Sheet <group> <n> | .Repack <id> <title|thumb> <n>
//          | .Ads <id> <light|standard|heavy> | .Launch <id> <cut|kast|teaser|reply> | .Duel <id>
//          to the session's DebugVideo, as Update 2's ss.Video.* are (Update 2's ss.Video.Upload takes Update 3's
//          [length] [ads] [duel] through its own forward: core only). ss.Video.Sheet's contact sheet is drawn on the
//          laptop: after the session's answer it opens "video sheet <group> <n>" there (DebugOpen), for ui-rerun to draw
//          the thumbnails the session dealt
//
// Every body behind them is a feature track's (core-siteplay's DebugSitePlay in SessionSitePlay.cpp; core-videoalgo's and
// core-videocontent's verbs in SessionVideo.cpp and SessionVideo3.cpp; the laptop pages DebugOpen knows): until a track
// lands, its commands answer with the stub's line.

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "NightOneGame.h"
#include "NightOneGameMode.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/UI/RiverLine.h"

#include <string>

DEFINE_LOG_CATEGORY_STATIC(LogUpdate3, Log, All);

namespace Update3Detail
{
/** The career's game in this world: Night One's (the apartment: the session and the laptop), or none. */
FNightOneGame* Update3GameIn(UWorld* World)
{
	ANightOneGameMode* Mode = World ? World->GetAuthGameMode<ANightOneGameMode>() : nullptr;
	return Mode ? Mode->GetGame() : nullptr;
}

enum class EUpdate3Feature : uint8
{
	Laptop,
	SitePlay,
	Video,
};

void Update3Log(const TCHAR* Name, const FString& Line, const FString& Answer)
{
	TArray<FString> Lines;
	Answer.ParseIntoArrayLines(Lines, false);
	UE_LOG(LogUpdate3, Display, TEXT("%s %s"), Name, *Line);
	for (const FString& L : Lines)
	{
		UE_LOG(LogUpdate3, Display, TEXT("  %s"), *L);
	}
}

/** Opens the laptop's app or page by name, or says that nothing on it has that name yet. */
void Update3Open(FNightOneGame& Game, const FString& Line)
{
	if (Game.Client.DebugOpen(std::string(TCHAR_TO_UTF8(*Line))))
	{
		UE_LOG(LogUpdate3, Display, TEXT("ss.Laptop.Open %s: open"), *Line);
		return;
	}
	UE_LOG(LogUpdate3, Warning,
		TEXT("ss.Laptop.Open %s: nothing on the laptop by that name in this build (riverline [page] [site], kast [page], video [page], bank, mail, burner [marcus|sam], shiftlink, geardrop, pennydrop)"),
		*Line);
}

void Update3Forward(EUpdate3Feature Feature, const TCHAR* Verb, const TArray<FString>& Args, UWorld* World)
{
	// ss.Laptop.Open and ss.Site.Play name their verb in the first argument; ss.Video.* in the command.
	FString Line = Verb ? FString(Verb).ToLower() : FString();
	for (int32 I = 0; I < Args.Num(); ++I)
	{
		const bool bVerb = !Verb && I == 0 && Feature == EUpdate3Feature::SitePlay;
		Line += (Line.IsEmpty() ? FString() : FString(TEXT(" "))) + (bVerb ? Args[I].ToLower() : Args[I]);
	}
	const TCHAR* Name = Feature == EUpdate3Feature::Laptop ? TEXT("ss.Laptop.Open") : Feature == EUpdate3Feature::SitePlay ? TEXT("ss.Site.Play") : TEXT("ss.Video");
	FNightOneGame* Game = Update3GameIn(World);
	if (!Game)
	{
		UE_LOG(LogUpdate3, Warning, TEXT("%s %s: only at home (Night One), where the career's session and the laptop live"), Name, *Line);
		return;
	}
	if (Feature == EUpdate3Feature::Laptop)
	{
		Update3Open(*Game, Line);
		return;
	}
	if (Feature == EUpdate3Feature::SitePlay && Line.IsEmpty())
	{
		Line = TEXT("describe");
	}
	const std::string In(TCHAR_TO_UTF8(*Line));
	const std::string Out = Feature == EUpdate3Feature::SitePlay ? Game->Session.DebugSitePlay(In) : Game->Session.DebugVideo(In);
	Update3Log(Name, Line, FString(UTF8_TO_TCHAR(Out.c_str())));
	if (Feature == EUpdate3Feature::Video && Line.StartsWith(TEXT("sheet")))
	{
		Update3Open(*Game, TEXT("video ") + Line);
	}
}

/** One console command forwarding as above: Verb null for the commands whose verb is their first argument. */
struct FUpdate3Command
{
	FUpdate3Command(EUpdate3Feature Feature, const TCHAR* Command, const TCHAR* Verb, const TCHAR* Help)
		: Cmd(Command, Help, FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([Feature, Verb](const TArray<FString>& Args, UWorld* World) { Update3Forward(Feature, Verb, Args, World); }))
	{
	}
	FAutoConsoleCommandWithWorldAndArgs Cmd;
};

// The laptop (§7.6; SPEC §9's PIE runs open every page with it).
FUpdate3Command LaptopOpen(EUpdate3Feature::Laptop, TEXT("ss.Laptop.Open"), nullptr,
	TEXT("ss.Laptop.Open <app> [page] [site]: opens a laptop app, its page and a site's client without a click (\"riverline lobby ignite\", \"riverline league runittwice\", \"video studio\") (testing)."));
// The rival sites' play book (§2.6, §3.6).
FUpdate3Command SitePlay(EUpdate3Feature::SitePlay, TEXT("ss.Site.Play"), nullptr,
	TEXT("ss.Site.Play <describe|wheel|dares|league|gov|league-draw|league-close|callout <npc>|grant <site> <cents>>: the sites' play book, the Governor and the League (testing); ")
	TEXT("<fuse <tier> <m>|sparks <n>|flame <days>|level <n>|scalp <npc>|rake <site> <cents>> (dev only)."));
// Rerun 2.0 (§4-§6).
FUpdate3Command VideoStats(EUpdate3Feature::Video, TEXT("ss.Video.Stats"), TEXT("Stats"), TEXT("ss.Video.Stats <id>: one video's numbers, every field, and its components."));
FUpdate3Command VideoCheckin(EUpdate3Feature::Video, TEXT("ss.Video.Checkin"), TEXT("Checkin"),
	TEXT("ss.Video.Checkin [id]: each eligible video's next decision midnight and its score now."));
FUpdate3Command VideoTrends(EUpdate3Feature::Video, TEXT("ss.Video.Trends"), TEXT("Trends"), TEXT("This week's trends on Rerun, with their heat."));
FUpdate3Command VideoIdeas(EUpdate3Feature::Video, TEXT("ss.Video.Ideas"), TEXT("Ideas"), TEXT("Every video idea, with its lock and what it still needs."));
FUpdate3Command VideoSheet(EUpdate3Feature::Video, TEXT("ss.Video.Sheet"), TEXT("Sheet"),
	TEXT("ss.Video.Sheet <beats|reads|pots|runs|rival|stories|ideas> <n>: n generated thumbnails for that kind group as a contact sheet on the laptop (testing)."));
FUpdate3Command VideoRepack(EUpdate3Feature::Video, TEXT("ss.Video.Repack"), TEXT("Repack"),
	TEXT("ss.Video.Repack <id> <title|thumb> <n>: changes a video's title or thumbnail to option n without the laptop; the time still passes (testing)."));
FUpdate3Command VideoAds(EUpdate3Feature::Video, TEXT("ss.Video.Ads"), TEXT("Ads"),
	TEXT("ss.Video.Ads <id> <light|standard|heavy>: a video's ad load from the next midnight, as Studio sets it (testing)."));
FUpdate3Command VideoLaunch(EUpdate3Feature::Video, TEXT("ss.Video.Launch"), TEXT("Launch"),
	TEXT("ss.Video.Launch <id> <cut|kast|teaser|reply>: a launch action for a new video; the time still passes (testing)."));
FUpdate3Command VideoDuel(EUpdate3Feature::Video, TEXT("ss.Video.Duel"), TEXT("Duel"), TEXT("ss.Video.Duel <id>: the Thumbnail Duel without the laptop, as Studio's Duel picker sets it; the time still passes (testing)."));
} // namespace Update3Detail
