// Update 4's debug commands (update 4 SPEC v1.7 §6.4 PA-50, §8.7): every gate's PIE hooks, registered once in Phase A. The
// Update2Commands.cpp pattern: each command passes its verb and its arguments on as one line, the verb in lower case and the
// arguments as typed ("ss.Fest.Moment feign belt" is "moment feign belt"), and the answer is logged a line at a time.
//
// They run at home only (Night One), where the career's session lives:
//   ss.Fest.Describe | .Calendar [year] | .Race [n] | .Hub <brand> [year] | .Look <brand|off> | .Moment <brand> [belt|event|title]
//          | .Pass | .Result <brand> <cash|ft|win|s1 seat>
//          to the session's DebugFest (F-world's at G1; §2.16). <brand> is a festival's key: rcop, ring, tco, slam, push, feign,
//          writ, grit, pennant or reign
//   ss.Fest.Shelf <all|off>   the game module's alone (no DebugFest body; G1 fix round 1): the apartment's festival shelf and
//          pin board show every piece (UFestShelf::SetDevShowAll: ten trophies, three belts, 40 pins; presentation only, never
//          saved) for §9.4's triangle and draw-call read and §2.16 step 11's mesh check, or the career's own again
//   ss.Scene.Status | .CapNow <n> | .Moment <kind> <key> | .Wake <reach|ft|win|title|room> | .Week <monday> | .Regulars
//          to DebugScene (W-arrive's at G2; §3.26)
//   ss.Turn.Budget <ms> | .Trace <on|off>   to DebugScene too, its line led by "turn" ("turn budget 0.5"): the held turn's switch is
//          W-arrive's
//   ss.Tpi.Status [name]   to DebugStory, its line led by "tpi" ("tpi status kettle.drum"): the TPI is W-story's
//   ss.Story.Arcs [name] | .Tip | .PassedYou <name> | .Roty | .Follow <name>   to DebugStory (W-story's at G2)
//   ss.Growth.Show <name> | .Pace <slow|fbb|fast>   to DebugGrowth (W-growth's at G2)
//   ss.Crowd.Status | .Field | .Card <name> | .Promote <name>   to DebugCrowd (S-crowd's and S-promote's at G4; §5.16)
//   ss.Big.Describe | .Unlock <on|off|auto> | .Register <templateId> [every|key|jump] [late <minutes>] | .To <remaining> | .Bust
//          | .Deck [eventId] | .Run [n] | .Bench <entries> [calib] | .Intro
//          to DebugBig (B-session's at G3; §4.20)
// The Notebook opens through Update3Commands.cpp's ss.Laptop.Open ("notebook [tab]", RiverLine::DebugOpen): it is never
// registered a second time.
//
// Every body behind them is a gate's track's (the session's DebugFest in SessionFest.cpp, DebugScene in WorldScene.cpp, DebugStory
// in SessionStory.cpp, DebugGrowth in WorldGrowth.cpp, DebugCrowd in SessionTable4.cpp, DebugBig in SessionBig.cpp): until its
// gate lands, a command answers that it isn't built yet.

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "FestShelf.h"
#include "HAL/IConsoleManager.h"
#include "NightOneGame.h"
#include "NightOneGameMode.h"
#include "ShortStack/Game/Festival.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/UI/FestivalArt.h"

#include <string>

DEFINE_LOG_CATEGORY_STATIC(LogUpdate4, Log, All);

namespace Update4Detail
{
/** The career's session in this world: Night One's (the apartment), or none. */
ss::Session* Update4SessionIn(UWorld* World)
{
	ANightOneGameMode* Mode = World ? World->GetAuthGameMode<ANightOneGameMode>() : nullptr;
	FNightOneGame* Game = Mode ? Mode->GetGame() : nullptr;
	return Game ? &Game->Session : nullptr;
}

enum class EUpdate4Feature : uint8
{
	Fest,
	Scene,
	Turn,
	Tpi,
	Story,
	Growth,
	Crowd,
	Big,
};

const TCHAR* Update4FeatureName(EUpdate4Feature Feature)
{
	switch (Feature)
	{
	case EUpdate4Feature::Fest: return TEXT("ss.Fest");
	case EUpdate4Feature::Scene: return TEXT("ss.Scene");
	case EUpdate4Feature::Turn: return TEXT("ss.Turn");
	case EUpdate4Feature::Tpi: return TEXT("ss.Tpi");
	case EUpdate4Feature::Story: return TEXT("ss.Story");
	case EUpdate4Feature::Growth: return TEXT("ss.Growth");
	case EUpdate4Feature::Crowd: return TEXT("ss.Crowd");
	default: return TEXT("ss.Big");
	}
}

void Update4Log(const TCHAR* Name, const FString& Line, const FString& Answer)
{
	TArray<FString> Lines;
	Answer.ParseIntoArrayLines(Lines, false);
	UE_LOG(LogUpdate4, Display, TEXT("%s %s"), Name, *Line);
	for (const FString& L : Lines)
	{
		UE_LOG(LogUpdate4, Display, TEXT("  %s"), *L);
	}
}

/**
 * ss.Fest.Hub's laptop half (G1, F-hubs' fhubs_to_lead_3; LEAD_CALLS R-19): the edition <brand> [year] names (fest::All(): that
 * year's, else the brand's on-stage or next edition), opened on its host's SERIES page through the laptop (RiverLine::DebugOpen's
 * "riverline series <site>", then ShowSeries on the edition's series id, so the host routes it to FestivalHub, SPEC v1.7 §2.8.1).
 * A rival's hub shows once the player holds an account there (DebugOpen's site rule). "" when it opened, else why not.
 */
FString Update4OpenHub(UWorld* World, const TArray<FString>& Args)
{
	ANightOneGameMode* Mode = World ? World->GetAuthGameMode<ANightOneGameMode>() : nullptr;
	FNightOneGame* Game = Mode ? Mode->GetGame() : nullptr;
	if (!Game || Args.Num() < 1)
	{
		return TEXT("ss.Fest.Hub <brand> [year]: only at home (Night One), with a brand named");
	}
	const std::string Key(TCHAR_TO_UTF8(*Args[0].ToLower()));
	const int Year = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 0;
	const int Today = ss::net::DayOf(Game->Session.WorldMinutes());
	const ss::fest::Festival* Pick = nullptr;
	for (const ss::fest::Festival& F : ss::fest::All())
	{
		// All() is in FirstDay order: with no year, the first edition of the brand not yet past its Afterglow's reach.
		if (!Pick && Key == ss::fest::BrandKey(F.Which) && (Year > 0 ? F.Year == Year : F.LastDay + ss::fest::kAfterglowDays + 2 >= Today))
		{
			Pick = &F;
		}
	}
	if (!Pick)
	{
		return FString::Printf(TEXT("ss.Fest.Hub %s: no such edition (rcop, ring, tco, slam, push, feign, writ, grit, pennant, reign [year])"), *Args[0]);
	}
	if (!Game->Client.DebugOpen(std::string("riverline series ") + ss::net::KeyOf(Pick->Site)))
	{
		return TEXT("ss.Fest.Hub: the laptop couldn't open the SERIES page");
	}
	Game->Client.ShowSeries(Pick->SeriesId, 0.0);
	return FString();
}

void Update4Forward(EUpdate4Feature Feature, const TCHAR* Verb, const TArray<FString>& Args, UWorld* World)
{
	const TCHAR* Name = Update4FeatureName(Feature);
	// G1 (F-look's P5; SPEC v1.7 §2.16; LEAD_CALLS R-19): ss.Fest.Look dresses the site in front through the UI's dev look
	// (festart::DevLook: presentation only, never saved; SitesSkin and SitesTableLook read it ahead of the calendar), wherever the laptop
	// is, then answers through DebugFest as the other verbs do.
	if (Feature == EUpdate4Feature::Fest && FCString::Stricmp(Verb, TEXT("Look")) == 0)
	{
		const std::string Key = Args.Num() > 0 ? std::string(TCHAR_TO_UTF8(*Args[0].ToLower())) : std::string();
		const ss::fest::Brand Look = Key == "off" ? ss::fest::Brand::None : ss::fest::BrandOf(Key);
		if (Key != "off" && Look == ss::fest::Brand::None)
		{
			UE_LOG(LogUpdate4, Warning, TEXT("ss.Fest.Look: \"%s\" is no festival (rcop, ring, tco, slam, feign, push, writ, grit, pennant, reign, or off)"), UTF8_TO_TCHAR(Key.c_str()));
			return;
		}
		ss::ui::festart::DevLook(Look);
		UE_LOG(LogUpdate4, Display, TEXT("ss.Fest.Look %s: dev look (presentation only, never saved)"), UTF8_TO_TCHAR(Key.c_str()));
	}
	// ss.Turn.* and ss.Tpi.* borrow another family's debug body, so their line names their family first.
	FString Line = Feature == EUpdate4Feature::Turn ? FString(TEXT("turn ")) : Feature == EUpdate4Feature::Tpi ? FString(TEXT("tpi ")) : FString();
	Line += FString(Verb).ToLower();
	for (const FString& A : Args)
	{
		Line += TEXT(" ") + A;
	}
	ss::Session* S = Update4SessionIn(World);
	if (!S)
	{
		UE_LOG(LogUpdate4, Warning, TEXT("%s.%s: only at home (Night One), where the career's session lives"), Name, Verb);
		return;
	}
	const std::string In(TCHAR_TO_UTF8(*Line));
	std::string Out;
	switch (Feature)
	{
	case EUpdate4Feature::Fest: Out = S->DebugFest(In); break;
	case EUpdate4Feature::Scene:
	case EUpdate4Feature::Turn: Out = S->DebugScene(In); break;
	case EUpdate4Feature::Tpi:
	case EUpdate4Feature::Story: Out = S->DebugStory(In); break;
	case EUpdate4Feature::Growth: Out = S->DebugGrowth(In); break;
	case EUpdate4Feature::Crowd: Out = S->DebugCrowd(In); break;
	default: Out = S->DebugBig(In); break;
	}
	if (Feature == EUpdate4Feature::Fest && FCString::Stricmp(Verb, TEXT("Hub")) == 0)
	{
		// G1 (fhubs_to_lead_3): after the session's answer, the laptop opens the edition's hub.
		const FString Why = Update4OpenHub(World, Args);
		Out += "\n" + std::string(Why.IsEmpty() ? "The laptop shows the edition's hub." : TCHAR_TO_UTF8(*Why));
	}
	Update4Log(Name, Line, FString(UTF8_TO_TCHAR(Out.c_str())));
}

/** One console command, "ss.<Feature>.<Verb>", forwarding as above. */
struct FUpdate4Command
{
	FUpdate4Command(EUpdate4Feature Feature, const TCHAR* Command, const TCHAR* Verb, const TCHAR* Help)
		: Cmd(Command, Help, FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([Feature, Verb](const TArray<FString>& Args, UWorld* World) { Update4Forward(Feature, Verb, Args, World); }))
	{
	}
	FAutoConsoleCommandWithWorldAndArgs Cmd;
};

// Festival Season (G1, §2.16; F-world's DebugFest).
FUpdate4Command FestDescribe(EUpdate4Feature::Fest, TEXT("ss.Fest.Describe"), TEXT("Describe"),
	TEXT("Festival Season: this year's ten windows with their phases, the live race (top 10, the player, people you know), the last crown, the player's pins and marks, each rival's Pass."));
FUpdate4Command FestCalendar(EUpdate4Feature::Fest, TEXT("ss.Fest.Calendar"), TEXT("Calendar"),
	TEXT("ss.Fest.Calendar [year]: the ten windows, the clear days, the Main days and each crown's expected minute."));
FUpdate4Command FestRace(EUpdate4Feature::Fest, TEXT("ss.Fest.Race"), TEXT("Race"), TEXT("ss.Fest.Race [n]: the live race's top n with points and cashes, and the player's rank of scorers."));
FUpdate4Command FestHub(EUpdate4Feature::Fest, TEXT("ss.Fest.Hub"), TEXT("Hub"),
	TEXT("ss.Fest.Hub <brand> [year]: opens that edition's hub (rcop, ring, tco, slam, push, feign, writ, grit, pennant, reign; a rival needs an account) (testing)."));
FUpdate4Command FestLook(EUpdate4Feature::Fest, TEXT("ss.Fest.Look"), TEXT("Look"), TEXT("ss.Fest.Look <brand|off>: dresses the site in front in a festival's look for a screenshot (dev only)."));
FUpdate4Command FestMoment(EUpdate4Feature::Fest, TEXT("ss.Fest.Moment"), TEXT("Moment"),
	TEXT("ss.Fest.Moment <brand> [belt|event|title]: plays the champion's moment, the belt's or a title's, numbers marked DEV; grants nothing (dev only)."));
FUpdate4Command FestPass(EUpdate4Feature::Fest, TEXT("ss.Fest.Pass"), TEXT("Pass"), TEXT("Two rents paid and 20 small events on the record, for the Festival Pass (dev only)."));
FUpdate4Command FestResult(EUpdate4Feature::Fest, TEXT("ss.Fest.Result"), TEXT("Result"),
	TEXT("ss.Fest.Result <brand> <cash|ft|win|s1 seat>: plants one result in the live festival's next numbered event through the world's result path (dev only)."));

/**
 * ss.Fest.Shelf <all|off> (G1 fix round 1; SPEC v1.7 §9.4, §2.16 step 11): no §2.16 hook puts a trophy on a plinth (a trophy is the hero's
 * own Main win; ss.Fest.Moment writes no Honor), so the PIE read of the shelf with all ten trophies, and the fallback build's look at
 * every SM_FS mesh, come from this dev view: UFestShelf::SetDevShowAll (presentation only, never saved, off at the shelf's next
 * BeginPlay). The shelf's LogTemp budget line follows within half a second, marked dev.
 */
void Update4Shelf(const TArray<FString>& Args, UWorld* World)
{
	const FString Mode = Args.Num() > 0 ? Args[0].ToLower() : FString();
	if (Mode != TEXT("all") && Mode != TEXT("off"))
	{
		UE_LOG(LogUpdate4, Warning, TEXT("ss.Fest.Shelf <all|off>: every piece on the apartment's shelf and pin board, or the career's own (dev only)"));
		return;
	}
	if (!Update4SessionIn(World))
	{
		UE_LOG(LogUpdate4, Warning, TEXT("ss.Fest.Shelf %s: only at home (Night One), where the shelf stands"), *Mode);
		return;
	}
	UFestShelf::SetDevShowAll(Mode == TEXT("all"));
	UE_LOG(LogUpdate4, Display, TEXT("ss.Fest.Shelf %s: %s (dev: presentation only, never saved)"), *Mode,
		Mode == TEXT("all") ? TEXT("ten trophies, three belts and 40 pins on show") : TEXT("the career's own Honors and pins again"));
}

FAutoConsoleCommandWithWorldAndArgs FestShelfShow(TEXT("ss.Fest.Shelf"),
	TEXT("ss.Fest.Shelf <all|off>: the apartment's festival shelf and pin board show every piece (ten trophies, three belts, 40 pins) for the budget read and the fallback check, or the career's own again (dev only)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Update4Shelf));

// The Scene and the held turn (G2, §3.26; W-arrive's DebugScene).
FUpdate4Command SceneStatus(EUpdate4Feature::Scene, TEXT("ss.Scene.Status"), TEXT("Status"),
	TEXT("The Scene: the book (closed or its anchor), A0, CapNow, this week's scale, arrivals by cause this week and in all, the pinned regulars."));
FUpdate4Command SceneCapNow(EUpdate4Feature::Scene, TEXT("ss.Scene.CapNow"), TEXT("CapNow"), TEXT("ss.Scene.CapNow <n>: sets CapNow, re-anchoring only if the cap had bound (dev only)."));
FUpdate4Command SceneMoment(EUpdate4Feature::Scene, TEXT("ss.Scene.Moment"), TEXT("Moment"),
	TEXT("ss.Scene.Moment <kind> <key>: posts a moment now (a festival by series id, a ripple by event id) (dev only)."));
FUpdate4Command SceneWake(EUpdate4Feature::Scene, TEXT("ss.Scene.Wake"), TEXT("Wake"),
	TEXT("ss.Scene.Wake <reach|ft|win|title|room>: plants one of the player's moments as if that result had happened (dev only)."));
FUpdate4Command SceneWeek(EUpdate4Feature::Scene, TEXT("ss.Scene.Week"), TEXT("Week"), TEXT("ss.Scene.Week <monday>: that week's new faces on each site, grouped as the post would print them."));
FUpdate4Command SceneRegulars(EUpdate4Feature::Scene, TEXT("ss.Scene.Regulars"), TEXT("Regulars"), TEXT("The Embercrest's 64 pinned regulars, with each replacement and its reason."));
FUpdate4Command TurnBudget(EUpdate4Feature::Turn, TEXT("ss.Turn.Budget"), TEXT("Budget"), TEXT("ss.Turn.Budget <ms>: the world's turn budget for this session (testing)."));
FUpdate4Command TurnTrace(EUpdate4Feature::Turn, TEXT("ss.Turn.Trace"), TEXT("Trace"), TEXT("ss.Turn.Trace <on|off>: logs each turn unit's time and clones (testing)."));
// The stories and the TPI (G2, §3.26; W-story's DebugStory).
FUpdate4Command TpiStatus(EUpdate4Feature::Tpi, TEXT("ss.Tpi.Status"), TEXT("Status"),
	TEXT("ss.Tpi.Status [name]: the player's (or a regular's) TPI rank, score, best and first ranks and monthly ring."));
FUpdate4Command StoryArcs(EUpdate4Feature::Story, TEXT("ss.Story.Arcs"), TEXT("Arcs"), TEXT("ss.Story.Arcs [name]: the open arcs, or one regular's."));
FUpdate4Command StoryTip(EUpdate4Feature::Story, TEXT("ss.Story.Tip"), TEXT("Tip"), TEXT("Runs this month's tip now (testing)."));
FUpdate4Command StoryPassedYou(EUpdate4Feature::Story, TEXT("ss.Story.PassedYou"), TEXT("PassedYou"), TEXT("ss.Story.PassedYou <name>: arms and forces a pass (dev only)."));
FUpdate4Command StoryRoty(EUpdate4Feature::Story, TEXT("ss.Story.Roty"), TEXT("Roty"), TEXT("Runs Rookie of the Year now (dev only)."));
FUpdate4Command StoryFollow(EUpdate4Feature::Story, TEXT("ss.Story.Follow"), TEXT("Follow"), TEXT("ss.Story.Follow <name>: follows a regular (testing)."));
// Growth (G2, §3.26; W-growth's DebugGrowth).
FUpdate4Command GrowthShow(EUpdate4Feature::Growth, TEXT("ss.Growth.Show"), TEXT("Show"), TEXT("ss.Growth.Show <name>: a regular's phase, cause, band, dwell and flags (dev only)."));
FUpdate4Command GrowthPace(EUpdate4Feature::Growth, TEXT("ss.Growth.Pace"), TEXT("Pace"), TEXT("ss.Growth.Pace <slow|fbb|fast>: this session's pace override, never saved (testing)."));
// The crowd (G4, §5.16; S-crowd's DebugCrowd).
FUpdate4Command CrowdStatus(EUpdate4Feature::Crowd, TEXT("ss.Crowd.Status"), TEXT("Status"),
	TEXT("The crowd: its size, arrivals into it, promotions by trigger, forgotten names, the full tier's Active count."));
FUpdate4Command CrowdField(EUpdate4Feature::Crowd, TEXT("ss.Crowd.Field"), TEXT("Field"), TEXT("The table in front's crowd seats and how each was named."));
FUpdate4Command CrowdCard(EUpdate4Feature::Crowd, TEXT("ss.Crowd.Card"), TEXT("Card"), TEXT("ss.Crowd.Card <name>: a crowd regular's record as the light card prints it."));
FUpdate4Command CrowdPromote(EUpdate4Feature::Crowd, TEXT("ss.Crowd.Promote"), TEXT("Promote"), TEXT("ss.Crowd.Promote <name>: promotes a crowd regular now (dev only)."));
// The Big Field (G3, §4.20; B-session's DebugBig).
FUpdate4Command BigDescribe(EUpdate4Feature::Big, TEXT("ss.Big.Describe"), TEXT("Describe"),
	TEXT("The table in front's big field: entries, the ring, modelled tables, the player's rank, tick costs, memory, the mystery deck, the play-ons, the unlock and the pick."));
FUpdate4Command BigUnlock(EUpdate4Feature::Big, TEXT("ss.Big.Unlock"), TEXT("Unlock"), TEXT("ss.Big.Unlock <on|off|auto>: overrides the big-field unlock for this session, never saved (dev only)."));
FUpdate4Command BigRegister(EUpdate4Feature::Big, TEXT("ss.Big.Register"), TEXT("Register"),
	TEXT("ss.Big.Register <templateId> [every|key|jump] [late <minutes>]: registers the next instance (or the running one, late) with a pick; the strike gate and the money still apply (testing)."));
FUpdate4Command BigTo(EUpdate4Feature::Big, TEXT("ss.Big.To"), TEXT("To"), TEXT("ss.Big.To <remaining>: skips the table in front with the plan until that many remain; stops still apply (dev only)."));
FUpdate4Command BigBust(EUpdate4Feature::Big, TEXT("ss.Big.Bust"), TEXT("Bust"), TEXT("The plan shoves every hand until the player busts, to start a play-on (dev only)."));
FUpdate4Command BigDeck(EUpdate4Feature::Big, TEXT("ss.Big.Deck"), TEXT("Deck"), TEXT("ss.Big.Deck [eventId]: the event's mystery deck: size, sum, the bounty pool, the envelopes left."));
FUpdate4Command BigRun(EUpdate4Feature::Big, TEXT("ss.Big.Run"), TEXT("Run"), TEXT("ss.Big.Run [n]: the last n YOUR RUN records."));
FUpdate4Command BigBench(EUpdate4Feature::Big, TEXT("ss.Big.Bench"), TEXT("Bench"), TEXT("ss.Big.Bench <entries> [calib]: a model-only field to the end, printing tick costs (testing)."));
FUpdate4Command BigIntro(EUpdate4Feature::Big, TEXT("ss.Big.Intro"), TEXT("Intro"), TEXT("Replays the table in front's final-table introductions for a shot (dev only)."));
} // namespace Update4Detail
