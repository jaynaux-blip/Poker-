// ue-auto's FAutoHost (BackRoomAuto.h) and ABackRoomGameMode's six autoplay functions (update 2 SPEC §2.6, §2.8, §2.12),
// which forward to it: the plan in the hero's seat at the Embercrest and at Dee's game. The pure rules (the room's read on
// the plan, the host text, a skipped hand's leftovers, Dee's clock) are in BackRoomAutoRules.h; the table's own side (the
// hero branch) is in BackRoomTableAuto.cpp. With the plan off every hook keeps today's behaviour: the seat is the player's,
// the clocks run as they did, the talk passes, the HUD and the host text carry nothing of it but what the room observed.
// ue-auto owns this file.

#include "BackRoomAuto.h"

#include "BackRoomGameMode.h"
#include "BackRoomLiveShared.h"
#include "BackRoomPlayer.h"
#include "BackRoomPlayerController.h"
#include "BackRoomStage.h"
#include "BackRoomTable.h"
#include "Camera/PlayerCameraManager.h"
#include "CardRoomSound.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"
#include "ShortStack/AI/Style.h"
#include "ShortStack/Game/Body.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/LiveAuto.h"
#include "ShortStack/Game/Room.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/UI/TableHud.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>

namespace BackRoomAutoDetail
{
// S-5 (the live skip's cost) isn't measured yet: the spec's defaults, as console variables so the lead can set them from it.
TAutoConsoleVariable<float> CVarAutoSkipSliceMs(TEXT("ss.Auto.SkipSliceMs"), 4.0f,
	TEXT("The live skip's slice: real milliseconds of live::SkipSlice a frame (update 2 §2.12, §9; S-5 sets it). It never changes what happens, only where a slice ends."));
TAutoConsoleVariable<int32> CVarAutoSkipCheckpointTicks(TEXT("ss.Auto.SkipCheckpointTicks"), 5,
	TEXT("The live skip checkpoints the night every this many ticks, and where it ends (R5; S-5 sets it)."));
// S-9 (Dee's hands at human pace) isn't measured yet: autoplay::DeeMinutesPerHand until it is.
TAutoConsoleVariable<float> CVarAutoDeeMinutesPerHand(TEXT("ss.Auto.DeeMinutesPerHand"), static_cast<float>(ss::autoplay::DeeMinutesPerHand),
	TEXT("World minutes Dee's night moves for each hand the plan plays (update 2 §1.1; S-9 replaces it)."));

/** The PLAY page's holds (pad A: on or off; Enter or pad X: skip), and the lock after a press takes the wheel. */
constexpr double AutoHoldSeconds = 0.6;
constexpr double AutoWheelLock = 0.25;
/** How long a note of the plan's stays up (the HUD's own notes fade at the same 2.6 s). */
constexpr double AutoNoteSeconds = 2.6;

FString AutoStr(const std::string& S)
{
	return FString(UTF8_TO_TCHAR(S.c_str()));
}

std::string AutoUtf8(const FString& S)
{
	return std::string(TCHAR_TO_UTF8(*S));
}

FString AutoName(ss::style::Style S)
{
	return AutoStr(ss::style::InfoOf(S).Name);
}

/** A reason as a sentence (style::Unavailable's are short labels: "Knockout events only"). */
FString AutoSentence(const std::string& S)
{
	FString Out = AutoStr(S);
	if (!Out.IsEmpty() && !Out.EndsWith(TEXT(".")) && !Out.EndsWith(TEXT("!")) && !Out.EndsWith(TEXT("?")))
	{
		Out += TEXT(".");
	}
	return Out;
}

/** The PLAY page's skip targets, in the order the keyboard's 1 to 5 pick them. */
ss::autoplay::Target AutoTargetAt(int32 Index)
{
	static const ss::autoplay::Target Targets[5] = {ss::autoplay::Target::NextBreak, ss::autoplay::Target::Bubble, ss::autoplay::Target::FinalTable, ss::autoplay::Target::Result,
		ss::autoplay::Target::Hands};
	return Targets[FMath::Clamp(Index, 0, 4)];
}

int32 AutoTargetIndex(ss::autoplay::Target To)
{
	for (int32 K = 0; K < 5; ++K)
	{
		if (AutoTargetAt(K) == To)
		{
			return K;
		}
	}
	return 1;
}

/** Where a skip runs to, as the end of "then skips …". */
FString AutoTargetWords(ss::autoplay::Target To, int32 Hands)
{
	switch (To)
	{
	case ss::autoplay::Target::NextBreak: return TEXT("to the break");
	case ss::autoplay::Target::Bubble: return TEXT("to the bubble");
	case ss::autoplay::Target::FinalTable: return TEXT("to the final table");
	case ss::autoplay::Target::Hands: return FMath::Max(1, Hands) == 1 ? FString(TEXT("the next hand")) : FString::Printf(TEXT("the next %d hands"), Hands);
	default: return TEXT("to the end of your night");
	}
}

int32 AutoSpeedOf(int32 Speed, bool bDees)
{
	const int32 S = Speed >= 8 ? 8 : Speed >= 4 ? 4 : Speed >= 2 ? 2 : 1;
	return bDees ? FMath::Min(S, 4) : S;
}

/** What a watch speed does, under its "WATCH 4x" note. At Dee's there's no floor and no table pace to leave alone, and her
 *  room's talk isn't thinned (TalkGate is the Embercrest director's), so it says only what's true there. */
FString AutoSpeedBody(int32 Speed, bool bDees)
{
	if (bDees)
	{
		switch (Speed)
		{
		case 2: return TEXT("Your plan plays it quicker. Dee deals to keep up.");
		case 4: return TEXT("Quicker still. That's as fast as Dee's room goes.");
		default: return TEXT("Your plan plays at the room's own pace.");
		}
	}
	switch (Speed)
	{
	case 2: return TEXT("Your plan plays it quicker. The table's own pace doesn't change.");
	case 4: return TEXT("Quicker still. Only the dealers and the floor get a word in.");
	case 8: return TEXT("Flat out. Only the floor's calls come through.");
	default: return TEXT("Your plan plays at the room's own pace.");
	}
}

/** Lower case words, split on spaces (ss.Auto's arguments). */
TArray<FString> AutoWords(const FString& Line)
{
	TArray<FString> Out;
	Line.ToLower().ParseIntoArrayWS(Out);
	return Out;
}

/** The plan's decision-level stops (§2.6.4): parity and a skip have nobody to hand them to. */
constexpr uint32 AutoDecisionStops = ss::autoplay::StopMyAllIn | ss::autoplay::StopBigPot | ss::autoplay::StopPremium;
} // namespace BackRoomAutoDetail


/** live::SkipSlice's sink for the card room (§2.12): each skipped hand and tick into the night, as the table's own would. */
struct FAutoSkipSink : ss::live::SkipSink
{
	explicit FAutoSkipSink(FAutoHost& InHost)
		: Host(InHost)
	{
	}
	FAutoHost& Host;
	virtual void HandDone(const ss::Hand& H, int HeroSeat) override { Host.SkippedHand(H, HeroSeat); }
	virtual void Events(const std::vector<ss::TEvent>& E) override { Host.SkippedEvents(E); }
	virtual void Checkpoint(int Tick) override
	{
		(void)Tick;
		Host.bSkipCheckpointDue = true;
	}
};

FAutoHost::FAutoHost(ABackRoomGameMode& InMode)
	: Mode(InMode)
	, Alive(MakeShared<bool>(true))
{
}

FAutoHost::~FAutoHost()
{
	// (The tournament may outlive the host on the table's shared pointer: the hook asks Alive first.)
	Alive.Reset();
}

FAutoHost* FAutoHost::Of(const ABackRoomGameMode* InMode)
{
	return InMode ? InMode->AutoHost.Get() : nullptr;
}

// ------------------------------------------------------------------ the night

bool FAutoHost::IsLive() const
{
	return Mode.bLive;
}

ss::Tournament* FAutoHost::Tourney() const
{
	return Mode.Tourney.Get();
}

ss::SaveData* FAutoHost::SaveOf() const
{
	return Mode.Save.Get();
}

bool FAutoHost::IsDeeNightOne() const
{
	// Dee's tutorial night: she explains everything on hands 1 to 9, and the player plays their own cards (§2.6.3).
	return !Mode.bLive && Mode.IsFirstVisit();
}

void FAutoHost::Init()
{
	bInit = true;
	ss::SaveData* S = SaveOf();
	Where = IsLive() ? 1 : 2;
	TiltGain = S ? FMath::Clamp(S->Life.Perks.TiltGain, 0.3, 2.5) : 1.0;
	// The read book as the night seats (a night come back to keeps the one it seated with: its autoread lines).
	if (S && !bBookRestored)
	{
		Book = BackRoomAutoRules::AutoLearnedOf(S->Life.Reads);
	}
	// The plan the PLAY page shows: the career's default (autoplay\tpref), off.
	if (S && !bRestoredAuto)
	{
		const ss::autoplay::TableAuto D = ss::autoplay::DefaultPlan(S->Life.Auto);
		Plan.S = D.S;
		Plan.Speed = BackRoomAutoDetail::AutoSpeedOf(D.Speed, !IsLive());
		Plan.To = D.To;
		Plan.Hands = D.Hands;
		Plan.Stops = D.Stops;
		Plan.M = ss::autoplay::Mode::Off;
	}
	TargetIndex = BackRoomAutoDetail::AutoTargetIndex(Plan.To);
	if (S)
	{
		NightNotes = S->Life.Auto.Notes;
		NightNoteSeen = S->Life.Auto.NoteSeen;
	}
	// Dee's mixes are keyed by the night (the day it began) and the hand number: drawn from nothing.
	const int32 Day = FMath::FloorToInt(Mode.LeftHomeAt / 1440.0);
	DeeSeed = BackRoomAutoRules::AutoHash("dee:" + std::to_string(Day) + ":" + (S ? S->HeroName : std::string()));
	// The room's read on the plan (§2.8): who reads shaking hands (their persona's HeroRead, by tournament id), and the
	// player's heart as tonight's event began.
	if (IsLive() && Tourney() && S)
	{
		const ss::Tournament& T = *Tourney();
		for (const ss::TPlayer& P : T.Players)
		{
			if (P.IsHero)
			{
				continue;
			}
			const double R = ABackRoomGameMode::PersonaFor(BackRoomAutoDetail::AutoStr(P.Name)).HeroRead;
			if (FMath::Abs(R) >= 0.05)
			{
				Reads.HeroRead[P.Id] = R;
			}
		}
		Reads.RestBpm = ss::body::TableOf(S->Life.Fit, Mode.LiveDayStart + T.Spec.StartClock).RestBpm;
		Reads.Sensitivity = FMath::Clamp(1.0 - 0.06 * static_cast<double>(Mode.PastNights), 0.65, 1.0);
	}
	if (bRestoredAuto && Plan.M != ss::autoplay::Mode::Off)
	{
		EnsureHook();
	}
	RefreshBrain();
	ApplyTable();
	UE_LOG(LogEmbercrest, Log, TEXT("Autoplay: %s, plan %s (%s)%s; %d players read hands; %d learned reads"), IsLive() ? TEXT("the Embercrest") : TEXT("Dee's game"), *BackRoomAutoDetail::AutoName(Plan.S),
		Plan.M == ss::autoplay::Mode::Off ? TEXT("off") : TEXT("on"), bResumedPaused ? TEXT(", paused from the checkpoint") : TEXT(""), static_cast<int32>(Reads.HeroRead.size()),
		static_cast<int32>(Book.Tells.size()));
}

FString FAutoHost::WhyNot() const
{
	if (!Mode.IsCareer() || !Mode.Table)
	{
		return TEXT("Your plan only plays a night of your career.");
	}
	if (IsDeeNightOne())
	{
		return BackRoomAutoDetail::AutoStr(ss::autoplay::Text(ss::autoplay::Words::DeeFirstNight));
	}
	if (Mode.bLive && (Mode.bLiveOver || Mode.Phase == EBackRoomPhase::Leaving))
	{
		return TEXT("Your night's over. There's nothing left for your plan to play.");
	}
	if (Mode.Phase == EBackRoomPhase::Busted)
	{
		return TEXT("You're out of chips. The reload's your call, not your plan's.");
	}
	if (Mode.Phase == EBackRoomPhase::Leaving)
	{
		return TEXT("You're racking up. The night's done.");
	}
	if (Mode.bHeroUp)
	{
		return TEXT("Sit down first. Your plan plays from the chair.");
	}
	return FString();
}

void FAutoHost::RefreshBrain()
{
	ss::SaveData* S = SaveOf();
	// At the Embercrest the notes as the career has them (folded at the settle); at Dee's the night's start's, because
	// tonight's hands are folded into the career's as they end (Dee's game has no settle of its own).
	const ss::reads::Table* Notes = IsLive() ? (S ? &S->Life.Auto.Notes : nullptr) : &NightNotes;
	Brain = ss::autoplay::BrainFor(Plan, Run, TiltGain, Where, Notes, &Book);
}

void FAutoHost::ApplyTable()
{
	if (ABackRoomTable* T = Mode.Table)
	{
		T->HeroBrain = bInit && Plan.M != ss::autoplay::Mode::Off && !bResumedPaused ? &Brain : nullptr;
	}
}

void FAutoHost::EnsureHook()
{
	if (bHooked || !IsLive() || !Tourney())
	{
		return;
	}
	bHooked = true;
	const TWeakPtr<bool> Token = Alive;
	FAutoHost* Self = this;
	Tourney()->HeroReadHook = [Token, Self](const ss::Hand& H, int Seat, ss::BotDecision& D) {
		if (Token.IsValid())
		{
			Self->ReadPlan(H, Seat, D);
		}
	};
}

bool FAutoHost::ReadsApply() const
{
	// The plan's bets are read in the engine while the plan holds the seat (the table's own ReadHero stands aside then:
	// HeroAutoHolds), and on every hand of a skip; the player's own are the table's to read, as always.
	const ABackRoomTable* T = Mode.Table;
	return bSkipping || (T && T->HeroBrain && !T->bHeroTakenOver && !T->bHeroAway && !T->IsBetweenHands());
}

void FAutoHost::ReadPlan(const ss::Hand& H, int32 Seat, ss::BotDecision& D)
{
	ss::Tournament* T = Tourney();
	if (!T || !ReadsApply())
	{
		return;
	}
	BackRoomAutoRules::AutoReadNote N;
	if (!BackRoomAutoRules::AutoApplyRead(Reads, Run.Tilt, *T, H, Seat, D, &N))
	{
		return;
	}
	++ReadsTaken;
	ReadsTonight.push_back(N);
	if (ReadsTonight.size() > 12)
	{
		ReadsTonight.erase(ReadsTonight.begin());
	}
	UE_LOG(LogEmbercrest, Log, TEXT("Autoplay: %s read the plan's hands (shake %.2f, %s) -> %s"), *BackRoomAutoDetail::AutoStr(N.Reader), N.Shake, N.Bluffing ? TEXT("bluffing") : TEXT("value"),
		N.Called ? TEXT("call") : TEXT("fold"));
}

void FAutoHost::MarkUsed()
{
	if (ss::SaveData* S = SaveOf())
	{
		// Any use of autoplay keeps its line in the save (§6.2).
		S->Life.Update2LinesRead |= ss::life::U2AutoPref;
	}
}

void FAutoHost::Note(const FString& Head, const FString& Body)
{
	NoteHead = Head;
	NoteBody = Body;
	NoteAt = FPlatformTime::Seconds();
	UE_LOG(LogEmbercrest, Log, TEXT("Autoplay: %s. %s"), *Head, *Body);
}

// ------------------------------------------------------------------ the game mode's six

bool FAutoHost::IsPlaying() const
{
	const ABackRoomTable* T = Mode.Table;
	return bInit && T && Mode.IsCareer() && Plan.M != ss::autoplay::Mode::Off && !bResumedPaused && !bPausedUp && !bSkipping && !T->bHeroTakenOver && !T->bHeroAway &&
		Mode.Phase == EBackRoomPhase::Playing;
}

float FAutoHost::WatchSpeed() const
{
	return IsPlaying() ? static_cast<float>(BackRoomAutoDetail::AutoSpeedOf(Plan.Speed, !IsLive())) : 1.0f;
}

void FAutoHost::Set(const ss::autoplay::TableAuto& NewPlan)
{
	const bool bWasOff = Plan.M == ss::autoplay::Mode::Off;
	Plan.S = NewPlan.S;
	Plan.Speed = BackRoomAutoDetail::AutoSpeedOf(NewPlan.Speed, !IsLive());
	Plan.Stops = NewPlan.Stops & (ss::autoplay::StopFinalTable | ss::autoplay::StopBubble | BackRoomAutoDetail::AutoDecisionStops);
	if (NewPlan.M == ss::autoplay::Mode::Off)
	{
		if (!bWasOff || bResumedPaused)
		{
			TurnOff(false);
		}
		return;
	}
	if (NewPlan.M == ss::autoplay::Mode::Skip)
	{
		Skip(NewPlan.To, NewPlan.Hands);
		return;
	}
	Plan.To = NewPlan.To;
	Plan.Hands = NewPlan.Hands;
	if (bWasOff || bResumedPaused)
	{
		TurnOn();
		return;
	}
	// Watching from a skip: the skip ends (between hands, where it only ever runs) and the plan watches.
	if (bSkipping)
	{
		EndSkip(static_cast<int32>(ss::live::SkipWhy::Stopped));
	}
	bSkipPending = false;
	Plan.M = ss::autoplay::Mode::Watch;
	MarkUsed();
	RefreshBrain();
	ApplyTable();
}

void FAutoHost::TakeOver()
{
	// ss.Auto.TakeOver and the game mode's AutoTakeOver (§2.6.5): mid-skip the skip stops; in the next hand's first 3 s, or
	// in a hand already the player's, the plan goes off; otherwise this hand is the player's.
	if (bSkipping || bSkipPending)
	{
		StopSkip();
		return;
	}
	if (Plan.M == ss::autoplay::Mode::Off || bResumedPaused)
	{
		return;
	}
	if (FPlatformTime::Seconds() < Run.ResumeWindowUntil || Run.TakenOver)
	{
		TurnOff(false);
		return;
	}
	TakeWheel(Mode.Table && Mode.Table->IsHeroAutoDeciding());
}

void FAutoHost::Skip(ss::autoplay::Target To, int32 Hands)
{
	using ss::autoplay::Target;
	using ss::autoplay::Words;
	if (!IsLive())
	{
		// P1 (cash::PlayHands): Dee's game has no skip yet.
		Note(TEXT("NO SKIPPING HERE"), TEXT("Dee's game plays hand by hand. A cash game has nothing to skip to."));
		return;
	}
	const FString Why = WhyNot();
	ss::Tournament* T = Tourney();
	if (!Why.IsEmpty() || !T)
	{
		Note(TEXT("NOT NOW"), Why.IsEmpty() ? FString(TEXT("There's no night to skip.")) : Why);
		return;
	}
	const char* Behind = To == Target::Closing ? ss::autoplay::Text(Words::ClosingIsCash)
		: To == Target::Bubble && T->InTheMoney() ? ss::autoplay::Text(Words::AlreadyInMoney)
		: To == Target::FinalTable && T->Tables.size() <= 1 ? ss::autoplay::Text(Words::AlreadyFinal)
		: nullptr;
	if (Behind)
	{
		Note(TEXT("NOT THERE"), BackRoomAutoDetail::AutoStr(Behind));
		return;
	}
	const std::string Greyed = ss::autoplay::CantPick(Plan.S, Where, T->Spec.BountyCents > 0);
	if (!Greyed.empty())
	{
		Note(BackRoomAutoDetail::AutoName(Plan.S).ToUpper(), BackRoomAutoDetail::AutoSentence(Greyed));
		return;
	}
	if (Plan.M == ss::autoplay::Mode::Off && !bResumedPaused)
	{
		StartRun();
	}
	bResumedPaused = false;
	Plan.M = ss::autoplay::Mode::Skip;
	Plan.To = To;
	Plan.Hands = To == Target::Hands ? (Hands > 0 ? Hands : 50) : 0;
	SkipHandsLeft = Plan.Hands;
	TargetIndex = BackRoomAutoDetail::AutoTargetIndex(To);
	bSkipPending = true;
	StopReason.clear();
	MarkUsed();
	EnsureHook();
	RefreshBrain();
	ApplyTable();
	if (Mode.Table && !Mode.Table->IsBetweenHands())
	{
		Note(TEXT("SKIP AHEAD"), FString::Printf(TEXT("Your plan finishes this hand, then skips %s."), *BackRoomAutoDetail::AutoTargetWords(To, Plan.Hands)));
	}
}

FString FAutoHost::Describe() const
{
	const ss::autoplay::Panel P = MakePanel();
	const ss::Tournament* T = Tourney();
	FString Out = FString::Printf(TEXT("Autoplay at %s: %s"), IsLive() ? *Mode.GetLiveName() : TEXT("Dee's game"), *BackRoomAutoDetail::AutoName(P.Now.S));
	if (bResumedPaused)
	{
		Out += FString::Printf(TEXT(", paused from the checkpoint (%s). [K] carries on."), Plan.M == ss::autoplay::Mode::Skip ? TEXT("it was skipping") : TEXT("it was watching"));
	}
	else if (!P.On)
	{
		Out += TEXT(", off (the plan the PLAY page shows).");
	}
	else if (P.Skipping)
	{
		Out += FString::Printf(TEXT(", %s to %s (%s)."), bSkipping ? TEXT("skipping") : TEXT("about to skip"), *BackRoomAutoDetail::AutoStr(ss::autoplay::TargetName(Plan.To)), *BackRoomAutoDetail::AutoStr(P.Status));
	}
	else
	{
		Out += FString::Printf(TEXT(", watching at %s%s%s."), *BackRoomAutoDetail::AutoStr(ss::autoplay::SpeedName(Plan.Speed)), P.TakenOver ? TEXT(", this hand taken over") : TEXT(""),
			bPausedUp ? TEXT(", paused while you're up") : TEXT(""));
	}
	Out += TEXT("\n") + BackRoomAutoDetail::AutoStr(ss::autoplay::SharpLine(P.Sharp, P.SharpWhy)) + (P.On || bResumedPaused ? TEXT(" (held for this run)") : TEXT(" (if it took the seat now)")) +
		(bSharpPinned ? TEXT(", pinned (dev).") : TEXT("."));
	const ss::SaveData* S = SaveOf();
	Out += FString::Printf(TEXT("\nForm %.1f, %d graded decisions of yours this week (live hands aren't graded); the plan's tilt %.3f."), P.Form,
		S ? S->Life.Auto.Pref.WeekDecisions : 0, Run.Tilt);
	Out += FString::Printf(TEXT("\nDecisions: the plan %d, you %d. Hands: the plan %d, you %d."), Run.AiDecisions, Run.OwnDecisions, Run.AiHands, Run.OwnHands);
	int32 Hands = 0;
	for (const auto& It : Run.Seen)
	{
		Hands += It.second.Hands;
	}
	int32 Noted = 0;
	const ss::reads::Table* Notes = IsLive() ? (S ? &S->Life.Auto.Notes : nullptr) : &NightNotes;
	if (const ss::Hand* H = Mode.Table ? Mode.Table->HeroHandForAuto() : nullptr)
	{
		for (const ss::HandSeat& Seat : H->Seats)
		{
			Noted += ss::autoplay::NoteFor(Notes, T, Seat.Id) ? 1 : 0;
		}
	}
	Out += FString::Printf(TEXT("\nObserved tonight: %d players over %d player-hands; notes on %d regulars, %d at this table; %d learned reads in the book."),
		static_cast<int32>(Run.Seen.size()), Hands, Notes ? static_cast<int32>(Notes->size()) : 0, Noted, static_cast<int32>(Book.Tells.size()));
	FString ReadLines;
	for (const BackRoomAutoRules::AutoReadNote& N : ReadsTonight)
	{
		ReadLines += FString::Printf(TEXT("%s%s %s (hand %d, shake %.2f, %s)"), ReadLines.IsEmpty() ? TEXT("") : TEXT("; "), *BackRoomAutoDetail::AutoStr(N.Reader), N.Called ? TEXT("called") : TEXT("folded"), N.Tick,
			N.Shake, N.Bluffing ? TEXT("a bluff") : TEXT("value"));
	}
	Out += FString::Printf(TEXT("\nReads on the plan tonight: %d%s%s%s."), ReadsTaken, ReadsTaken > static_cast<int32>(ReadsTonight.size()) ? TEXT(" (the last 12)") : TEXT(""),
		ReadLines.IsEmpty() ? TEXT("") : TEXT(": "), *ReadLines);
	FString Stops;
	for (const uint32 Bit : {ss::autoplay::StopFinalTable, ss::autoplay::StopBubble, ss::autoplay::StopMyAllIn, ss::autoplay::StopBigPot, ss::autoplay::StopPremium})
	{
		if ((Plan.Stops & Bit) != 0)
		{
			Stops += (Stops.IsEmpty() ? TEXT("") : TEXT(", ")) + BackRoomAutoDetail::AutoStr(ss::autoplay::StopName(Bit));
		}
	}
	Out += TEXT("\nStops: ") + (Stops.IsEmpty() ? FString(TEXT("none")) : Stops) + TEXT(".") + (StopReason.empty() ? FString() : TEXT(" Last skip ended: ") + BackRoomAutoDetail::AutoStr(StopReason));
	if (bParity || bParityArmed)
	{
		Out += FString::Printf(TEXT("\nParity: %d of %d hands watched."), ParityHands - ParityLeft, ParityHands);
	}
	return Out;
}

bool ABackRoomGameMode::IsAutoPlaying() const
{
	return AutoHost && AutoHost->IsPlaying();
}

float ABackRoomGameMode::AutoWatchSpeed() const
{
	return AutoHost ? AutoHost->WatchSpeed() : 1.0f;
}

void ABackRoomGameMode::SetAuto(const ss::autoplay::TableAuto& Plan)
{
	if (AutoHost)
	{
		AutoHost->Set(Plan);
	}
}

void ABackRoomGameMode::AutoTakeOver()
{
	if (AutoHost)
	{
		AutoHost->TakeOver();
	}
}

void ABackRoomGameMode::AutoSkip(ss::autoplay::Target To, int32 Hands)
{
	if (AutoHost)
	{
		AutoHost->Skip(To, Hands);
	}
}

FString ABackRoomGameMode::DescribeAuto() const
{
	return AutoHost ? AutoHost->Describe() : FString(TEXT("Autoplay: no table tonight."));
}

// ------------------------------------------------------------------ turning it on and off

void FAutoHost::StartRun()
{
	// Autoplay takes the seat (§2.5): Sharpness read once from the player's Form and condition now, and held for the run;
	// the plan's tilt starts from the player's. Form is read with its decay applied and kept (FormAt), as online.
	ss::SaveData* S = SaveOf();
	if (!S)
	{
		return;
	}
	const double World = Mode.Minutes;
	const double Form = ss::autoplay::FormAt(S->Life.Auto, World);
	const double Tilt = Mode.Table ? FMath::Clamp(static_cast<double>(Mode.Table->Composure.Tilt), 0.0, 1.0) : 0.0;
	const bool bExhausted = Mode.Energy < ss::body::EffectsOf(S->Life.Fit, false).ExhaustedAt;
	const bool bNeedy = S->Life.Hunger >= 85.0 || S->Life.Thirst >= 85.0;
	if (!bSharpPinned)
	{
		Run.Sharp = ss::autoplay::SharpnessOf(Form, Tilt, bExhausted, bNeedy);
	}
	Run.SharpSetAt = World;
	Run.Tilt = FMath::RoundToDouble(Tilt * 1000.0) / 1000.0;
	Run.TakenOver = false;
	Run.TakenMidThink = false;
	Run.ResumeWindowUntil = 0.0;
	bEverOn = true;
	MarkUsed();
	EnsureHook();
}

void FAutoHost::TurnOn()
{
	const FString Why = WhyNot();
	if (!Why.IsEmpty())
	{
		Note(BackRoomAutoDetail::AutoStr(ss::autoplay::Text(ss::autoplay::Words::PanelTitle)), Why);
		return;
	}
	const ss::Tournament* T = Tourney();
	const std::string Greyed = ss::autoplay::CantPick(Plan.S, Where, IsLive() && T && T->Spec.BountyCents > 0);
	if (!Greyed.empty())
	{
		Note(BackRoomAutoDetail::AutoName(Plan.S).ToUpper(), BackRoomAutoDetail::AutoSentence(Greyed));
		return;
	}
	if (bResumedPaused)
	{
		// A night come back to: the run carries on as it was (the same tilt, Sharpness and counts), watched at 1x.
		bResumedPaused = false;
		Plan.Speed = 1;
		Note(TEXT("YOUR PLAN CARRIES ON"), BackRoomAutoDetail::AutoStr(ss::autoplay::BackOnLine(Plan.S)));
	}
	else
	{
		if (Plan.M == ss::autoplay::Mode::Off)
		{
			StartRun();
		}
		Note(TEXT("AUTOPLAY · ") + BackRoomAutoDetail::AutoName(Plan.S).ToUpper(),
			FString::Printf(TEXT("Your plan has the seat. Sharpness %d%%."), FMath::RoundToInt(Run.Sharp * 100.0)));
	}
	Plan.M = ss::autoplay::Mode::Watch;
	EnsureHook();
	RefreshBrain();
	ApplyTable();
	if (Mode.Table)
	{
		Mode.Table->bHeroCardsKnownAuto = true;
	}
}

void FAutoHost::TurnOff(bool bQuiet)
{
	if (bSkipping)
	{
		EndSkip(static_cast<int32>(ss::live::SkipWhy::Stopped));
	}
	bSkipPending = false;
	if (Mode.Table && Mode.Table->IsHeroAutoDeciding())
	{
		// Off mid-think: the plan's pending move is dropped and the decision is the player's now (its draws happened).
		Mode.Table->HeroTakeOver();
		Run.TakenMidThink = true;
	}
	Plan.M = ss::autoplay::Mode::Off;
	bResumedPaused = false;
	Run.TakenOver = false;
	Run.ResumeWindowUntil = 0.0;
	MarkUsed();
	RefreshBrain();
	ApplyTable();
	if (!bQuiet)
	{
		Note(TEXT("AUTOPLAY OFF"), TEXT("The seat's yours from the next decision."));
	}
}

void FAutoHost::TakeWheel(bool bMidThink)
{
	// A take-over is this hand (§2.6.5): the plan's pending move is dropped, the decision opens for the player, and the next
	// hand is the plan's again (K in its first 3 s keeps the seat the player's).
	if (!Mode.Table)
	{
		return;
	}
	Mode.Table->HeroTakeOver();
	Run.TakenOver = true;
	Run.TakenMidThink = bMidThink;
	// Between hands it's the next hand that's taken (the table's StartHand clears its flag: OnNewHand sets it again).
	bTakeOverNext = Mode.Table->IsBetweenHands();
	LockUntil = FPlatformTime::Seconds() + BackRoomAutoDetail::AutoWheelLock;
	MarkUsed();
	UE_LOG(LogEmbercrest, Log, TEXT("Autoplay: the player took the wheel (hand %d%s)"), Mode.Table->GetHandNumber(), bMidThink ? TEXT(", mid-think") : TEXT(""));
}

void FAutoHost::OnK()
{
	if (IsDeeNightOne())
	{
		Note(BackRoomAutoDetail::AutoStr(ss::autoplay::Text(ss::autoplay::Words::PanelTitle)), BackRoomAutoDetail::AutoStr(ss::autoplay::Text(ss::autoplay::Words::DeeFirstNight)));
		return;
	}
	if (bResumedPaused || Plan.M == ss::autoplay::Mode::Off)
	{
		TurnOn();
		return;
	}
	if (bSkipping || bSkipPending)
	{
		StopSkip();
		return;
	}
	if (FPlatformTime::Seconds() < Run.ResumeWindowUntil || Run.TakenOver || bPausedUp)
	{
		TurnOff(false);
		return;
	}
	TakeWheel(Mode.Table && Mode.Table->IsHeroAutoDeciding());
}

void FAutoHost::CycleStyle(int32 Dir)
{
	// The PLAY page's arrows (pad A tapped): the styles on the menu, in the catalog's order. While the plan has the seat
	// only the ones it can play here; off, a greyed one shows why it can't.
	const ss::Tournament* T = Tourney();
	const bool bKnockout = IsLive() && T && T->Spec.BountyCents > 0;
	const bool bOn = Plan.M != ss::autoplay::Mode::Off && !bResumedPaused;
	const int32 Count = static_cast<int32>(ss::style::Style::Count);
	const int32 From = static_cast<int32>(Plan.S);
	for (int32 Step = 1; Step <= Count; ++Step)
	{
		const ss::style::Style S = static_cast<ss::style::Style>(((From + Dir * Step) % Count + Count) % Count);
		if (ss::style::InfoOf(S).Hidden || (bOn && !ss::autoplay::CantPick(S, Where, bKnockout).empty()))
		{
			continue;
		}
		Plan.S = S;
		break;
	}
	MarkUsed();
	RefreshBrain();
}

void FAutoHost::CycleSpeed()
{
	// T (right on the d-pad) while the plan holds the seat: 1x, 2x, 4x, 8x (no 8x at Dee's: a room of people you know).
	const bool bDees = !IsLive();
	const int32 Now = BackRoomAutoDetail::AutoSpeedOf(Plan.Speed, bDees);
	Plan.Speed = Now == 1 ? 2 : Now == 2 ? 4 : (Now == 4 && !bDees) ? 8 : 1;
	MarkUsed();
	Note(BackRoomAutoDetail::AutoStr(ss::autoplay::WatchToast(Plan.Speed)), BackRoomAutoDetail::AutoSpeedBody(Plan.Speed, bDees));
}

// ------------------------------------------------------------------ the hooks

void FAutoHost::Tick(float RealDt)
{
	(void)RealDt;
	ABackRoomTable* T = Mode.Table;
	if (!T || !Mode.IsCareer())
	{
		return;
	}
	if (!bInit)
	{
		Init();
	}
	const int32 HandNumber = T->GetHandNumber();
	if (HandNumber != HandSeen)
	{
		OnNewHand(HandNumber);
	}
	bAwayThisHand = bAwayThisHand || T->bHeroAway;
	// Dee's last hand (or the one she sends a falling-asleep player home after, or the one the player racks up after): the plan
	// plays it out, then stops.
	if (!IsLive() && Plan.M != ss::autoplay::Mode::Off && (Mode.bLastHandCalled || Mode.bSentHome || Mode.bLeaveAsked))
	{
		bStopAfterHand = true;
	}
	// ss.Auto.Parity: the watched hands start at the next gap between hands.
	ss::Tournament* Tour = Tourney();
	if (bParityArmed && Tour && T->IsBetweenHands() && !Tour->FinishPending() && !bSkipping && !bSkipPending && Mode.Phase == EBackRoomPhase::Playing)
	{
		bParityArmed = false;
		ParityWas = Plan;
		if (bResumedPaused)
		{
			bResumedPaused = false;
		}
		else if (Plan.M == ss::autoplay::Mode::Off)
		{
			StartRun();
		}
		Plan.M = ss::autoplay::Mode::Watch;
		Plan.Speed = 8;
		Plan.Stops &= ~BackRoomAutoDetail::AutoDecisionStops; // nobody's there to answer a hand-off
		EnsureHook();
		RefreshBrain();
		ApplyTable();
		T->bHeroCardsKnownAuto = true;
		ParityCk = Tour->Checkpoint();
		ParityRun = Run;
		ParityPlan = Plan;
		ParityLeft = ParityHands;
		bParity = true;
		UE_LOG(LogEmbercrest, Display, TEXT("ss.Auto.Parity: watching %d hands at 8x from hand %d (%s)"), ParityHands, Tour->Tick, *BackRoomAutoDetail::AutoName(Plan.S));
	}
	if ((bParity || bParityArmed) && (Mode.bLiveOver || Mode.Phase == EBackRoomPhase::Leaving || !Tour))
	{
		UE_LOG(LogEmbercrest, Display, TEXT("ss.Auto.Parity: stopped, the night ended %d hands short."), ParityLeft);
		bParity = false;
		bParityArmed = false;
	}
	ApplyTable();
}

void FAutoHost::OnNewHand(int32 HandNumber)
{
	HandSeen = HandNumber;
	PlanActsThisHand = 0;
	bAwayThisHand = Mode.Table && Mode.Table->bHeroAway;
	// A take-over pressed between hands is this new hand's: the player plays it, and the window opens after it. (The flag
	// lasts only until the next deal, so a stale one never takes a later hand.)
	const bool bCarry = Run.TakenOver && bTakeOverNext && Plan.M == ss::autoplay::Mode::Watch && !bResumedPaused && Mode.Table;
	bTakeOverNext = false;
	if (bCarry)
	{
		Mode.Table->HeroTakeOver();
	}
	// The hand after a take-over is the plan's again; for its first 3 s, K keeps the seat the player's (§2.6.5).
	else if (Run.TakenOver)
	{
		Run.TakenOver = false;
		Run.TakenMidThink = false;
		if (Plan.M == ss::autoplay::Mode::Watch && !bResumedPaused)
		{
			Run.ResumeWindowUntil = FPlatformTime::Seconds() + ss::autoplay::ResumeWindowSeconds;
		}
	}
	// The cards are known without a peek while the plan holds the seat (and for the rest of a hand it held at the deal).
	if (Mode.Table)
	{
		Mode.Table->bHeroCardsKnownAuto = Plan.M != ss::autoplay::Mode::Off && !bResumedPaused;
	}
}

bool FAutoHost::PawnKeys(ABackRoomPawn& Pawn, APlayerController* PC, bool bCardLock)
{
	ABackRoomTable* T = Mode.Table;
	if (!bInit || !T || !Mode.IsCareer() || !PC)
	{
		return false;
	}
	const double Now = FPlatformTime::Seconds();
	const std::vector<std::string>& Pages = ss::ui::TableCardPages();
	const bool bPlayPage = Pawn.bTableCard && Pawn.TableCardPage >= 0 && Pawn.TableCardPage < static_cast<int32>(Pages.size()) && Pages[static_cast<size_t>(Pawn.TableCardPage)] == "PLAY";
	const bool bPadA = Pawn.Held(PC, EKeys::Gamepad_FaceButton_Bottom);
	const bool bPadX = Pawn.Held(PC, EKeys::Gamepad_FaceButton_Left);
	const bool bEnter = Pawn.Held(PC, EKeys::Enter);
	if (bPlayPage)
	{
		// The PLAY page (§2.6.2), under the card's hold: the arrows or A tapped pick the style, K or A held turns the plan on
		// or off, 1 to 5 or X tapped pick the skip's target, Enter or X held skips. Dee's first night: nothing starts.
		if (!IsDeeNightOne())
		{
			if (Pawn.Pressed(PC, EKeys::Left))
			{
				CycleStyle(-1);
			}
			if (Pawn.Pressed(PC, EKeys::Right))
			{
				CycleStyle(1);
			}
			if (Pawn.Pressed(PC, EKeys::K))
			{
				if (Plan.M == ss::autoplay::Mode::Off || bResumedPaused)
				{
					TurnOn();
				}
				else
				{
					TurnOff(false);
				}
			}
			// Pad A: a tap cycles the style, a hold turns the plan on or off.
			if (bPadA && !bPadAWas)
			{
				PadADownAt = Now;
				bPadAFired = false;
			}
			if (bPadA && !bPadAFired && PadADownAt >= 0.0 && Now - PadADownAt >= BackRoomAutoDetail::AutoHoldSeconds)
			{
				bPadAFired = true;
				if (Plan.M == ss::autoplay::Mode::Off || bResumedPaused)
				{
					TurnOn();
				}
				else
				{
					TurnOff(false);
				}
			}
			if (!bPadA && bPadAWas && !bPadAFired && PadADownAt >= 0.0)
			{
				CycleStyle(1);
			}
			if (IsLive())
			{
				static const FKey Digits[5] = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five};
				for (int32 K = 0; K < 5; ++K)
				{
					if (Pawn.Pressed(PC, Digits[K]))
					{
						TargetIndex = K;
						MarkUsed();
					}
				}
				// Enter held skips; pad X tapped cycles the target, held skips.
				if (bEnter && EnterDownAt < 0.0 && Pawn.Pressed(PC, EKeys::Enter))
				{
					EnterDownAt = Now;
					bEnterFired = false;
				}
				if (bEnter && !bEnterFired && EnterDownAt >= 0.0 && Now - EnterDownAt >= BackRoomAutoDetail::AutoHoldSeconds)
				{
					bEnterFired = true;
					Skip(BackRoomAutoDetail::AutoTargetAt(TargetIndex), BackRoomAutoDetail::AutoTargetAt(TargetIndex) == ss::autoplay::Target::Hands ? 50 : 0);
				}
				if (bPadX && !bPadXWas)
				{
					PadXDownAt = Now;
					bPadXFired = false;
				}
				if (bPadX && !bPadXFired && PadXDownAt >= 0.0 && Now - PadXDownAt >= BackRoomAutoDetail::AutoHoldSeconds)
				{
					bPadXFired = true;
					Skip(BackRoomAutoDetail::AutoTargetAt(TargetIndex), BackRoomAutoDetail::AutoTargetAt(TargetIndex) == ss::autoplay::Target::Hands ? 50 : 0);
				}
				if (!bPadX && bPadXWas && !bPadXFired && PadXDownAt >= 0.0)
				{
					TargetIndex = (TargetIndex + 1) % 5;
					MarkUsed();
				}
			}
		}
		EnterDownAt = bEnter ? EnterDownAt : -1.0;
		bPadAWas = bPadA;
		bPadXWas = bPadX;
		return false;
	}
	// Off the PLAY page the holds start over (a button already down when the page opens is an old one).
	PadADownAt = -1.0;
	PadXDownAt = -1.0;
	EnterDownAt = -1.0;
	bPadAWas = bPadA;
	bPadXWas = bPadX;
	if (bCardLock)
	{
		return false; // the card is up, or just put down: nothing at the table is anyone's to do
	}
	// K at the table: the plan on; mid-skip, the skip stops; in the hand after a take-over (or one already the player's), the
	// plan off; otherwise this hand is the player's (§2.6.2, §2.6.5).
	if (Pawn.Pressed(PC, EKeys::K))
	{
		OnK();
		return true;
	}
	// B mid-skip: the skip stops between hands (it only ever runs between them).
	if ((bSkipping || bSkipPending) && Pawn.Pressed(PC, EKeys::Gamepad_FaceButton_Right))
	{
		StopSkip();
		return true;
	}
	// T (right on the d-pad) while the plan holds the seat: its watch speed; the table's own pace stays as it is.
	if (IsPlaying() && (Pawn.Pressed(PC, EKeys::T) || Pawn.Pressed(PC, EKeys::Gamepad_DPad_Right)))
	{
		CycleSpeed();
		return true;
	}
	static const FKey Wheel[] = {EKeys::F, EKeys::C, EKeys::R, EKeys::A, EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Gamepad_FaceButton_Left,
		EKeys::Gamepad_FaceButton_Bottom, EKeys::Gamepad_FaceButton_Top};
	bool bWheel = false;
	for (const FKey& Key : Wheel)
	{
		bWheel = bWheel || Pawn.Pressed(PC, Key);
	}
	// A night come back to mid-plan waits for K; an action of the player's own means they're playing it themselves.
	if (bResumedPaused && bWheel)
	{
		bResumedPaused = false;
		Plan.M = ss::autoplay::Mode::Off;
		RefreshBrain();
		ApplyTable();
		return false;
	}
	// The plan thinking: the first action press takes the wheel and does nothing else; the next one acts (after the lock).
	const bool bThinking = T->IsHeroAutoDeciding();
	if (bThinking && bWheel)
	{
		TakeWheel(true);
		return true;
	}
	if (Now < LockUntil)
	{
		return true;
	}
	return IsPlaying() && bThinking;
}

void FAutoHost::OnHandEnded(const ss::recall::Record& Rec, const ss::room::HandNote* RoomNote)
{
	// The hand's end for the plan's books (§1.3), both rooms: the plan's tilt and counts and what the player saw of the table
	// (reads::Count, read from the next hand on), a highlight candidate, Dee's per-hand clock and her notes.
	ABackRoomTable* T = Mode.Table;
	ss::SaveData* S = SaveOf();
	const ss::Hand* H = T ? T->HeroHandForAuto() : nullptr;
	if (!H || !S)
	{
		return;
	}
	const int HeroIdx = BackRoomAutoRules::AutoHeroIndex(*H);
	if (HeroIdx < 0)
	{
		return;
	}
	const bool bByPlan = Plan.M != ss::autoplay::Mode::Off && !Run.TakenOver && !bResumedPaused && !bAwayThisHand && !bPausedUp;
	int32 Mine = 0;
	const int Seat = H->Seats[static_cast<size_t>(HeroIdx)].Seat;
	for (const ss::ActionRecord& A : H->History)
	{
		Mine += A.Seat == Seat ? 1 : 0;
	}
	ss::autoplay::NoteHand(Run, *H, HeroIdx, bByPlan, TiltGain);
	if (!bByPlan && !bAwayThisHand)
	{
		// The player's own decisions this hand (a taken-over hand's start was the plan's, counted as it acted).
		Run.OwnDecisions += FMath::Max(0, Mine - PlanActsThisHand);
	}
	RefreshBrain();
	// A highlight candidate (core-video's NoteLiveHand: the Embercrest's, or Dee's as storytime). Draws nothing.
	ss::video::LiveContext Ctx;
	Ctx.Source = IsLive() ? 1 : 2;
	Ctx.World = Mode.Minutes;
	Ctx.Auto = bByPlan;
	Ctx.Style = bByPlan ? static_cast<int>(Plan.S) : 0;
	if (const ss::Tournament* Tour = IsLive() ? Tourney() : nullptr)
	{
		Ctx.EntryId = BackRoomAutoDetail::AutoUtf8(Mode.LiveEntryId);
		Ctx.Event = BackRoomAutoDetail::AutoUtf8(Mode.LiveName);
		Ctx.BuyIn = Tour->Spec.BuyInCents;
		Ctx.Field = Tour->Spec.Entrants;
		Ctx.Remaining = Tour->Remaining;
		Ctx.Bubble = Tour->HandForHand();
		Ctx.Final = Tour->Tables.size() <= 1;
	}
	ss::video::NoteLiveHand(S->Life, Rec, RoomNote, Ctx);
	if (!IsLive())
	{
		// Dee's clock moves by the hand under the plan (§1.1); her game has no settle, so the night's notes and played line
		// are kept up as each hand ends.
		if (bByPlan)
		{
			DeePending += FMath::Max(0.5, static_cast<double>(BackRoomAutoDetail::CVarAutoDeeMinutesPerHand.GetValueOnGameThread()));
		}
		FoldDeeNotes();
	}
	if (bParity && --ParityLeft <= 0)
	{
		FinishParity();
	}
	if (bStopAfterHand)
	{
		bStopAfterHand = false;
		if (Plan.M != ss::autoplay::Mode::Off)
		{
			TurnOff(true);
		}
	}
}

void FAutoHost::FoldDeeNotes()
{
	ss::SaveData* S = SaveOf();
	if (!S || IsLive())
	{
		return;
	}
	// Tonight's observed stats on the cast into the career's notes at half weight, once: folded again from the night's start
	// at each hand's end, so the save after any hand holds exactly one fold of the night so far.
	ss::autoplay::Book& B = S->Life.Auto;
	B.Notes = NightNotes;
	B.NoteSeen = NightNoteSeen;
	ss::reads::Table Named;
	for (const auto& It : Run.Seen)
	{
		if (It.first != "You" && It.first != ss::HeroId)
		{
			Named[ss::autoplay::NoteKey(nullptr, It.first)] = It.second;
		}
	}
	ss::autoplay::FoldNotes(B, Named, Mode.Minutes);
	if (!B.Notes.empty())
	{
		S->Life.Update2LinesRead |= ss::life::U2Notes;
	}
	if (Run.AiHands > 0 || Run.AiDecisions > 0)
	{
		const int32 Day = FMath::FloorToInt(Mode.LeftHomeAt / 1440.0);
		ss::autoplay::Record(B, "dee:" + std::to_string(Day), 2, 0, Plan, Run);
		S->Life.Update2LinesRead |= ss::life::U2AutoPlayed | ss::life::U2AutoPref;
	}
}

void FAutoHost::OnHeroBusted()
{
	// Felted at Dee's (§2.6.3): the plan stops; "buy back in?" is the player's call, never the plan's.
	if (Plan.M != ss::autoplay::Mode::Off || bSkipPending)
	{
		TurnOff(true);
		UE_LOG(LogEmbercrest, Log, TEXT("Autoplay: felted, the plan stops; the reload is the player's"));
	}
}

double FAutoHost::DeeMinutes(double RealMinutes)
{
	if (IsLive())
	{
		return RealMinutes;
	}
	const bool bPerHand = Plan.M != ss::autoplay::Mode::Off && !bResumedPaused && !Run.TakenOver && Mode.Phase == EBackRoomPhase::Playing;
	return BackRoomAutoRules::AutoDeeClock(bPerHand, RealMinutes, DeePending);
}

FString FAutoHost::HostLines() const
{
	const bool bWithRun = bEverOn || Plan.M != ss::autoplay::Mode::Off || Run.AiHands > 0 || Run.AiDecisions > 0;
	return BackRoomAutoDetail::AutoStr(BackRoomAutoRules::AutoHostText(Plan, Run, Book, bWithRun));
}

bool FAutoHost::ReadHostLine(const TArray<FString>& Fields)
{
	if (Fields.Num() == 0 || !Fields[0].StartsWith(TEXT("auto")))
	{
		return false;
	}
	std::vector<std::string> F;
	F.reserve(static_cast<size_t>(Fields.Num()));
	for (const FString& Field : Fields)
	{
		F.push_back(BackRoomAutoDetail::AutoUtf8(Field));
	}
	switch (BackRoomAutoRules::AutoReadHostLine(F, Plan, Run, Book))
	{
	case BackRoomAutoRules::AutoHostLine::Run:
		// A night closed mid-plan comes back in the same mode, paused at 1x until K (§2.12).
		bRestoredAuto = true;
		bEverOn = true;
		bResumedPaused = Plan.M != ss::autoplay::Mode::Off;
		TargetIndex = BackRoomAutoDetail::AutoTargetIndex(Plan.To);
		return true;
	case BackRoomAutoRules::AutoHostLine::Seen: return true;
	case BackRoomAutoRules::AutoHostLine::Read:
		bBookRestored = true;
		return true;
	default: return false;
	}
}

void FAutoHost::OnStand(bool bUp)
{
	// Standing up pauses the plan (§2.6.2): the empty chair is the away rule's. Sitting back down, the plan's back on.
	if (bUp)
	{
		bPausedUp = true;
		if (bSkipping)
		{
			EndSkip(static_cast<int32>(ss::live::SkipWhy::Stopped));
		}
		if (bSkipPending)
		{
			bSkipPending = false;
			Plan.M = ss::autoplay::Mode::Watch;
			RefreshBrain();
			ApplyTable();
		}
		if (Plan.M != ss::autoplay::Mode::Off && !bResumedPaused)
		{
			Note(TEXT("PLAN PAUSED"), BackRoomAutoDetail::AutoStr(ss::autoplay::Text(ss::autoplay::Words::Paused)));
		}
		return;
	}
	bPausedUp = false;
	if (Plan.M != ss::autoplay::Mode::Off && !bResumedPaused)
	{
		Note(TEXT("PLAN BACK ON"), BackRoomAutoDetail::AutoStr(ss::autoplay::BackOnLine(Plan.S)));
	}
}

void FAutoHost::OnSettle(ss::SaveData& D, const std::string& EntryId)
{
	// The night's end (§1.3, §2.9): what the plan played (the played line, for a night it played at all), what the player's
	// tables saw of the world's people into the notes, and the night's recap candidate.
	if (Run.AiHands > 0 || Run.AiDecisions > 0)
	{
		ss::autoplay::Record(D.Life.Auto, EntryId, 1, 0, Plan, Run);
		D.Life.Update2LinesRead |= ss::life::U2AutoPlayed | ss::life::U2AutoPref;
	}
	const ss::Tournament* T = Tourney();
	ss::reads::Table Named;
	for (const auto& It : Run.Seen)
	{
		if (It.first.rfind("npc:", 0) == 0)
		{
			Named[ss::autoplay::NoteKey(T, It.first)] = It.second;
		}
	}
	ss::autoplay::FoldNotes(D.Life.Auto, Named, Mode.Minutes);
	if (!D.Life.Auto.Notes.empty())
	{
		D.Life.Update2LinesRead |= ss::life::U2Notes;
	}
	if (T)
	{
		ss::video::LiveContext Ctx;
		Ctx.Source = 1;
		Ctx.EntryId = EntryId;
		Ctx.Event = BackRoomAutoDetail::AutoUtf8(Mode.LiveName);
		Ctx.BuyIn = T->Spec.BuyInCents;
		Ctx.Field = T->Spec.Entrants;
		Ctx.Remaining = T->Remaining;
		Ctx.Final = T->Tables.size() <= 1;
		Ctx.Auto = Run.AiHands > 0;
		Ctx.Style = Ctx.Auto ? static_cast<int>(Plan.S) : 0;
		Ctx.World = Mode.Minutes;
		ss::video::NoteLiveRecap(D.Life, Ctx, Mode.HeroPlace, Mode.HeroPrizeCents);
	}
	UE_LOG(LogEmbercrest, Log, TEXT("Autoplay settle: the plan %d hands and %d decisions, you %d and %d; %d players observed; %d reads on the plan"), Run.AiHands, Run.AiDecisions,
		Run.OwnHands, Run.OwnDecisions, static_cast<int32>(Run.Seen.size()), ReadsTaken);
}

bool FAutoHost::TalkGate(const ss::room::Cue& Cue) const
{
	// Watching at speed the talk thins before it's voiced (§2.12): at 4x and up the dealers and the PA, at 8x the PA only
	// (a text to the player always gets through), so the captions keep up with the hands.
	const float Speed = WatchSpeed();
	if (Speed < 4.0f)
	{
		return true;
	}
	switch (Cue.What)
	{
	case ss::room::Kind::Pa:
	case ss::room::Kind::Phone: return true;
	case ss::room::Kind::Call: return Speed < 8.0f;
	default: return false;
	}
}

// ------------------------------------------------------------------ the HUD and the PLAY page

ss::autoplay::Panel FAutoHost::MakePanel() const
{
	ss::autoplay::Panel P;
	P.Styles = ss::style::Catalog();
	const ss::SaveData* S = SaveOf();
	const ss::Tournament* T = Tourney();
	const ABackRoomTable* Tb = Mode.Table;
	const double World = Mode.Minutes;
	bool bRusty = false;
	const double Form = S ? ss::autoplay::FormNow(S->Life.Auto, World, &bRusty) : ss::autoplay::FormStart;
	const double Tilt = Tb ? FMath::Clamp(static_cast<double>(Tb->Composure.Tilt), 0.0, 1.0) : 0.0;
	const bool bExhausted = S && Mode.Energy < ss::body::EffectsOf(S->Life.Fit, false).ExhaustedAt;
	const bool bNeedy = S && (S->Life.Hunger >= 85.0 || S->Life.Thirst >= 85.0);
	P.Form = Form;
	P.Rusty = bRusty;
	P.SharpWhy = ss::autoplay::SharpWhy(Form, bRusty, Tilt, bExhausted, bNeedy);
	P.On = Plan.M != ss::autoplay::Mode::Off && !bResumedPaused;
	P.Now = Plan;
	if (!P.On)
	{
		P.Now.M = ss::autoplay::Mode::Off;
	}
	if (P.On || bResumedPaused || bSharpPinned)
	{
		P.Sharp = Run.Sharp;
		P.SharpSetAt = Run.SharpSetAt;
	}
	else
	{
		P.Sharp = ss::autoplay::SharpnessOf(Form, Tilt, bExhausted, bNeedy);
		P.SharpSetAt = World;
	}
	const double Now = FPlatformTime::Seconds();
	P.Thinking = Tb && Tb->IsHeroAutoDeciding();
	P.ActsIn = P.Thinking ? static_cast<double>(Tb->HeroAutoActsIn()) / static_cast<double>(FMath::Max(1.0f, WatchSpeed())) : 0.0;
	P.TakenOver = Run.TakenOver;
	P.ResumeWindow = P.On && Plan.M == ss::autoplay::Mode::Watch && Now < Run.ResumeWindowUntil;
	P.ResumeLeft = P.ResumeWindow ? Run.ResumeWindowUntil - Now : 0.0;
	P.Skipping = P.On && (bSkipping || bSkipPending);
	P.LastAction = Tb && LastActHand == Tb->GetHandNumber() ? BackRoomAutoDetail::AutoUtf8(Tb->HeroAutoLastAction()) : std::string();
	ss::autoplay::TableAuto Shown = P.Now;
	Shown.Hands = SkipHandsLeft;
	if (P.Skipping)
	{
		Shown.M = ss::autoplay::Mode::Skip;
	}
	P.Status = ss::autoplay::TileStatus(Shown, T);
	P.SkipLine = bSkipping && T ? BackRoomAutoRules::AutoSkipCardLine(*T, Shown) : std::string();
	P.StopReason = StopReason;
	P.AiDecisions = Run.AiDecisions;
	P.OwnDecisions = Run.OwnDecisions;
	P.AiHands = Run.AiHands;
	P.OwnHands = Run.OwnHands;
	const bool bKnockout = IsLive() && T && T->Spec.BountyCents > 0;
	for (const ss::style::Info& I : P.Styles)
	{
		P.Greyed.push_back(ss::autoplay::CantPick(I.S, Where, bKnockout));
	}
	return P;
}

void FAutoHost::FillHud(ss::ui::TableHudInfo& Out, const FBackRoomPrompt& Prompt) const
{
	(void)Prompt;
	const ABackRoomTable* T = Mode.Table;
	if (!bInit || !T)
	{
		return;
	}
	const double Now = FPlatformTime::Seconds();
	// The plan's fields (core-tableui draws its P0a lines from them: the status, the think, the last action, the next hand's
	// window and the skip card). AutoOn: the plan holds the seat now.
	Out.AutoOn = IsPlaying();
	Out.AutoStyle = ss::style::InfoOf(Plan.S).Name;
	Out.AutoSharp = Run.Sharp;
	Out.AutoDeciding = T->IsHeroAutoDeciding();
	Out.AutoLastAction = LastActHand == T->GetHandNumber() ? BackRoomAutoDetail::AutoUtf8(T->HeroAutoLastAction()) : std::string();
	Out.AutoActsIn = T->HeroAutoActsIn() / FMath::Max(1.0f, WatchSpeed());
	Out.AutoSpeed = BackRoomAutoDetail::AutoSpeedOf(Plan.Speed, !IsLive());
	Out.SkipOn = bSkipping;
	if (bSkipping && Tourney())
	{
		ss::autoplay::TableAuto Shown = Plan;
		Shown.Hands = SkipHandsLeft;
		Out.SkipLine = BackRoomAutoRules::AutoSkipCardLine(*Tourney(), Shown);
	}
	Out.AutoResumeWindow = Out.AutoOn && Plan.M == ss::autoplay::Mode::Watch && Now < Run.ResumeWindowUntil ? static_cast<float>(Run.ResumeWindowUntil - Now) : 0.0f;
	// A night come back to mid-plan: the note stays until K (or the player plays on), when the player is in the chair.
	if (bResumedPaused && !Out.NoteHolds && Mode.Phase == EBackRoomPhase::Playing && !Mode.bHeroUp)
	{
		Out.Note = BackRoomAutoDetail::AutoUtf8(TEXT("YOUR PLAN WAS PLAYING"));
		Out.NoteSub = ss::autoplay::WasPlayingLine(Plan.S);
		Out.NoteAt = Now - 1.0;
		Out.NoteHolds = true;
		return;
	}
	// The plan's own note a moment (a toggle, a speed, a skip's end, the stand), unless the night's own is newer or holds.
	if (Now - NoteAt < BackRoomAutoDetail::AutoNoteSeconds && !Out.NoteHolds && (Out.Note.empty() || Out.NoteAt < NoteAt))
	{
		Out.Note = BackRoomAutoDetail::AutoUtf8(NoteHead);
		Out.NoteSub = BackRoomAutoDetail::AutoUtf8(NoteBody);
		Out.NoteAt = NoteAt;
	}
}

void FAutoHost::FillCard(ss::ui::TableCardInfo& Out) const
{
	if (!bInit)
	{
		return;
	}
	Out.Play.Panel = MakePanel();
	if (IsLive())
	{
		for (int32 K = 0; K < 5; ++K)
		{
			Out.Play.Targets.push_back(ss::autoplay::TargetName(BackRoomAutoDetail::AutoTargetAt(K)));
		}
		Out.Play.Target = FMath::Clamp(TargetIndex, 0, 4);
	}
	if (IsDeeNightOne())
	{
		Out.Play.Locked = ss::autoplay::Text(ss::autoplay::Words::DeeFirstNight);
	}
}

FString FAutoHost::StatusLine() const
{
	// The legacy HUD's P0a line (ss.Table.LegacyHud 1): the same facts the core HUD draws from the fields.
	if (!bInit)
	{
		return FString();
	}
	if (bSkipping && Tourney())
	{
		ss::autoplay::TableAuto Shown = Plan;
		Shown.Hands = SkipHandsLeft;
		return TEXT("SKIPPING · ") + BackRoomAutoDetail::AutoName(Plan.S).ToUpper() + TEXT(" · ") + BackRoomAutoDetail::AutoStr(BackRoomAutoRules::AutoSkipCardLine(*Tourney(), Shown)) + TEXT(" · [K] Stop");
	}
	if (bResumedPaused)
	{
		return BackRoomAutoDetail::AutoStr(ss::autoplay::WasPlayingLine(Plan.S));
	}
	if (bPausedUp && Plan.M != ss::autoplay::Mode::Off)
	{
		return BackRoomAutoDetail::AutoStr(ss::autoplay::Text(ss::autoplay::Words::Paused));
	}
	if (!IsPlaying())
	{
		return FString();
	}
	if (Plan.M == ss::autoplay::Mode::Watch && FPlatformTime::Seconds() < Run.ResumeWindowUntil)
	{
		return BackRoomAutoDetail::AutoStr(ss::autoplay::Text(ss::autoplay::Words::Resumes));
	}
	ss::autoplay::TableAuto Shown = Plan;
	Shown.M = ss::autoplay::Mode::Watch;
	return BackRoomAutoDetail::AutoStr(ss::autoplay::StatusLine(Shown, Run.Sharp));
}

// ------------------------------------------------------------------ the table's side

bool FAutoHost::StopsHere(const ss::Hand& H, const ss::BotDecision& Planned, int32 HeroIdx)
{
	// The decision-level stop rules (§2.6.4) against the move the plan drew: one that fires hands the decision to the player.
	if (!ss::autoplay::ShouldStop(Plan, H, Planned, HeroIdx, Tourney()))
	{
		return false;
	}
	Run.TakenOver = true;
	Run.TakenMidThink = false;
	MarkUsed();
	Note(TEXT("YOUR DECISION"), BackRoomAutoDetail::AutoStr(ss::autoplay::Text(ss::autoplay::Words::HandedOver)));
	return true;
}

void FAutoHost::NotePlanActed()
{
	++Run.AiDecisions;
	++PlanActsThisHand;
	LastActHand = Mode.Table ? Mode.Table->GetHandNumber() : -1;
	MarkUsed();
}

void FAutoHost::NotePlanDrew(const ss::Hand& H)
{
	DrewHand = Mode.Table ? Mode.Table->GetHandNumber() : -1;
	DrewHistory = static_cast<int32>(H.History.size());
}

bool FAutoHost::PlanDecisionFresh(const ss::Hand& H) const
{
	return DrewHand == (Mode.Table ? Mode.Table->GetHandNumber() : -1) && DrewHistory == static_cast<int32>(H.History.size());
}

// ------------------------------------------------------------------ the live skip (§2.12)

void FAutoHost::StopSkip()
{
	if (bSkipping)
	{
		EndSkip(static_cast<int32>(ss::live::SkipWhy::Stopped));
		return;
	}
	if (bSkipPending)
	{
		bSkipPending = false;
		Plan.M = ss::autoplay::Mode::Watch;
		Plan.Speed = 1;
		RefreshBrain();
		ApplyTable();
		Note(TEXT("SKIP CALLED OFF"), TEXT("Your plan keeps the seat at 1x."));
	}
}

bool FAutoHost::BeginSkip()
{
	ss::Tournament* T = Tourney();
	ABackRoomTable* Tb = Mode.Table;
	if (!T || !Tb || Mode.Phase != EBackRoomPhase::Playing || Mode.bHeroUp || Mode.bLiveOver)
	{
		return false;
	}
	// Only ever between hands, once the room's last round has been finished.
	if (!Tb->IsBetweenHands() || T->FinishPending() || T->Hero().Busted || T->bFinished)
	{
		return false;
	}
	bSkipPending = false;
	bSkipping = true;
	// A take-over is a hand's (§2.6.5): the skip's hands are all the plan's, so none carries over to show "AUTO RESUMES"
	// at the first hand the table deals after it.
	Run.TakenOver = false;
	Run.TakenMidThink = false;
	Run.ResumeWindowUntil = 0.0;
	SkipHands = 0;
	SkipBreakTick = -1;
	bSkipMoved = false;
	bSkipCheckpointDue = false;
	LastCheckpointTick = T->Tick;
	StopReason.clear();
	Tb->Hold();
	// The table holds until the skip ends: LiveTick's hand-for-hand wait (MoveT counting up to 0 while the table holds) mustn't
	// deal the next hand under it. -1000 is that wait's own "nothing pending"; a move's clock (0 and up) is left alone.
	if (Mode.MoveT < 0.0f)
	{
		Mode.MoveT = -1000.0f;
	}
	// A break under way goes with the skip: its minutes are on the engine's clock already, its real seconds aren't sat through.
	if (Mode.bOnBreak)
	{
		Mode.bOnBreak = false;
		Mode.BreakT = 0.0f;
	}
	if (Mode.RoomHost.IsValid() && Mode.RoomDirector.IsValid())
	{
		Mode.RoomDirector->HeroUp = Mode.bHeroUp;
		RoomFeed::BeginCatchUp(*Mode.RoomHost);
	}
	UE_LOG(LogEmbercrest, Log, TEXT("Autoplay skip: %s to %s from hand %d, level %d, %d left"), *BackRoomAutoDetail::AutoName(Plan.S), *BackRoomAutoDetail::AutoStr(ss::autoplay::TargetName(Plan.To)), T->Tick, T->LevelIndex + 1,
		T->Remaining);
	return true;
}

bool FAutoHost::TickSkip(float RealDt)
{
	(void)RealDt;
	if (!bSkipPending && !bSkipping)
	{
		return false;
	}
	ss::Tournament* T = Tourney();
	if (!IsLive() || !T || Mode.bLiveOver || Mode.Phase == EBackRoomPhase::Leaving)
	{
		if (bSkipping && Mode.RoomHost.IsValid())
		{
			RoomFeed::EndCatchUp(*Mode.RoomHost);
		}
		bSkipPending = false;
		bSkipping = false;
		return false;
	}
	if (!bSkipping && (bPausedUp || !BeginSkip()))
	{
		return false;
	}
	// A slice: whole ticks until the budget's spent or the skip ends; the budget never changes what happens (§2.12).
	FAutoSkipSink Sink(*this);
	ss::autoplay::TableAuto A = Plan;
	A.M = ss::autoplay::Mode::Skip;
	A.Hands = SkipHandsLeft;
	const double Budget = FMath::Clamp(static_cast<double>(BackRoomAutoDetail::CVarAutoSkipSliceMs.GetValueOnGameThread()), 0.25, 100.0);
	const ss::live::SkipOut O = ss::live::SkipSlice(*T, Brain, A, Budget, Sink);
	SkipHandsLeft = FMath::Max(0, SkipHandsLeft - O.Hands);
	// A break the skip passed is consumed; the one it stopped at (Next break) plays out on screen.
	if (Mode.bOnBreak && O.Why != ss::live::SkipWhy::Break)
	{
		Mode.bOnBreak = false;
		Mode.BreakT = 0.0f;
	}
	const bool bEnds = O.Why != ss::live::SkipWhy::Budget;
	const bool bOut = T->Hero().Busted || T->bFinished;
	if (!bEnds && Mode.MoveT < 0.0f)
	{
		Mode.MoveT = -1000.0f; // (still skipping: see BeginSkip)
	}
	const int32 Cadence = FMath::Max(1, BackRoomAutoDetail::CVarAutoSkipCheckpointTicks.GetValueOnGameThread());
	if (!bOut && bSkipCheckpointDue && (bEnds || T->Tick - LastCheckpointTick >= Cadence))
	{
		// R5: the night saved where the skip stands, on the background saver (LiveCheckpoint's own log line counts them).
		bSkipCheckpointDue = false;
		LastCheckpointTick = T->Tick;
		Mode.LiveCheckpoint(false);
	}
	return bEnds ? EndSkip(static_cast<int32>(O.Why)) : false;
}

bool FAutoHost::EndSkip(int32 WhyCode)
{
	using ss::autoplay::Words;
	using ss::live::SkipWhy;
	const SkipWhy Why = static_cast<SkipWhy>(WhyCode);
	bSkipping = false;
	bSkipPending = false;
	if (Mode.RoomHost.IsValid() && Mode.RoomDirector.IsValid())
	{
		RoomFeed::EndCatchUp(*Mode.RoomHost);
	}
	ss::Tournament* T = Tourney();
	if (T)
	{
		UE_LOG(LogEmbercrest, Log, TEXT("Autoplay skip over: %d hands, why %d, hand %d, level %d, %d left, stack %lld"), SkipHands, WhyCode, T->Tick, T->LevelIndex + 1, T->Remaining,
			static_cast<int64>(T->Hero().Stack));
	}
	// Out (or champion): the night ends as it would have at the table (LiveOver: the summary, the cage, the doors).
	if (T && (T->Hero().Busted || T->bFinished) && !Mode.bLiveOver)
	{
		Plan.M = ss::autoplay::Mode::Watch;
		Mode.LiveOver();
		return true;
	}
	const char* Reason = nullptr;
	switch (Why)
	{
	case SkipWhy::Break: Reason = ss::autoplay::Text(T && T->Spec.BreakEvery > 0 ? Words::AtBreak : Words::AtLevel); break;
	case SkipWhy::Bubble: Reason = ss::autoplay::Text(Words::AtBubble); break;
	case SkipWhy::HandForHand: Reason = ss::autoplay::Text(Words::AtHandForHand); break;
	case SkipWhy::FinalTable: Reason = ss::autoplay::Text(Words::AtFinalTable); break;
	case SkipWhy::Stopped: Reason = ss::autoplay::Text(Words::StoppedByYou); break;
	case SkipWhy::Target:
		Reason = Plan.To == ss::autoplay::Target::Hands ? ss::autoplay::Text(Words::AtHands)
			: Plan.To == ss::autoplay::Target::Bubble ? ss::autoplay::Text(Words::AtBubble)
			: Plan.To == ss::autoplay::Target::FinalTable ? ss::autoplay::Text(Words::AtFinalTable)
			: ss::autoplay::Text(Words::WantToSee);
		break;
	default: break;
	}
	StopReason = Reason ? Reason : "";
	// It stops and switches to watching at 1x with the plan holding the seat ("You'll want to see this", §2.6.4).
	Plan.M = ss::autoplay::Mode::Watch;
	Plan.Speed = 1;
	RefreshBrain();
	ApplyTable();
	if (T)
	{
		const FString Tail = Why == SkipWhy::Stopped ? FString::Printf(TEXT(" %d left."), T->Remaining) : FString::Printf(TEXT(" %d left. Your plan has the seat at 1x."), T->Remaining);
		Note(FString::Printf(TEXT("SKIPPED %d %s"), SkipHands, SkipHands == 1 ? TEXT("HAND") : TEXT("HANDS")), BackRoomAutoDetail::AutoStr(StopReason) + Tail);
	}
	Mode.PlaceExtras();
	if (Mode.Phase == EBackRoomPhase::Playing && Mode.Table)
	{
		if (bSkipMoved)
		{
			BlinkToTable();
		}
		else if (!Mode.bOnBreak)
		{
			Mode.Table->Resume(1.0f);
		}
	}
	bSkipMoved = false;
	return false;
}

void FAutoHost::BlinkToTable()
{
	// Moved during the skip: a blink, and the player is at the new table (the walk is for a move the player sees, §2.12).
	ss::Tournament* T = Tourney();
	if (!T || !Mode.Stage || !Mode.Table)
	{
		return;
	}
	const ss::TPlayer& H = T->Hero();
	Mode.Stage->SetRoomAnchor(Mode.SlotForTable(H.TableId));
	Mode.Table->PrepareNext();
	Mode.PlaceExtras();
	if (Mode.bOnBreak)
	{
		// The skip stopped at a break: the new table sits it out with the room (LiveTick deals again when it's over), as
		// EndSkip keeps an unmoved table held through one.
		Mode.Table->Hold();
	}
	else
	{
		Mode.Table->Resume(1.0f);
	}
	if (APlayerController* PC = Mode.GetWorld() ? Mode.GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 0.35f, FLinearColor::Black, false, false);
		}
	}
	Mode.RaiseBanner(FString::Printf(TEXT("TABLE %d   SEAT %d"), H.TableId, ss::live::SeatLabel(H.Seat)));
	UE_LOG(LogEmbercrest, Log, TEXT("Autoplay skip: moved during it, blinked to table %d, seat %d"), H.TableId, ss::live::SeatLabel(H.Seat));
}

void FAutoHost::SkippedHand(const ss::Hand& H, int32 HeroIdx)
{
	// A hand the skip played, into the night as the table's own would go (§2.12): the plan's tilt, hand and decisions and
	// the observed stats (NoteSkippedHand), the hands played and who sat with the player (for the world), a big pot shown down
	// with one of the world's people, the HANDS page and a highlight candidate.
	ss::Tournament* T = Tourney();
	ss::SaveData* S = SaveOf();
	if (!T)
	{
		return;
	}
	ss::autoplay::NoteSkippedHand(Run, H, HeroIdx, TiltGain);
	RefreshBrain();
	++Mode.HandsPlayed;
	++SkipHands;
	for (const std::string& Id : BackRoomAutoRules::AutoTablemates(H, HeroIdx))
	{
		Mode.LiveMet.Add(BackRoomAutoDetail::AutoStr(Id));
	}
	std::string Across;
	ss::Chips Net = 0;
	ss::Chips Pot = 0;
	if (BackRoomAutoRules::AutoShowdownAcross(H, HeroIdx, Across, Net, Pot) && Across.rfind("npc:", 0) == 0 && Pot >= 40 * FMath::Max<ss::Chips>(1, H.BigBlind))
	{
		int64& Was = Mode.LiveBigPots.FindOrAdd(BackRoomAutoDetail::AutoStr(Across));
		Was = FMath::Abs(static_cast<int64>(Net)) > FMath::Abs(Was) ? static_cast<int64>(Net) : Was;
	}
	ss::recall::Record Rec = BackRoomAutoRules::AutoSkippedRecord(*T, H, HeroIdx, T->Tick);
	if (S)
	{
		ss::video::LiveContext Ctx;
		Ctx.Source = 1;
		// (A skipped hand is numbered by the engine's tick, the table's by its own count: ":skip" keeps their candidate ids
		// apart, so a hand the table deals later never reads as one noted already.)
		Ctx.EntryId = BackRoomAutoDetail::AutoUtf8(Mode.LiveEntryId) + ":skip";
		Ctx.Event = BackRoomAutoDetail::AutoUtf8(Mode.LiveName);
		Ctx.BuyIn = T->Spec.BuyInCents;
		Ctx.Field = T->Spec.Entrants;
		Ctx.Remaining = T->Remaining;
		Ctx.Bubble = T->HandForHand();
		Ctx.Final = T->Tables.size() <= 1;
		Ctx.Auto = true;
		Ctx.Style = static_cast<int>(Plan.S);
		Ctx.World = Mode.LiveDayStart + T->ClockMinutes();
		ss::video::NoteLiveHand(S->Life, Rec, nullptr, Ctx);
	}
	Mode.RecentHands.push_back(MoveTemp(Rec));
	if (Mode.RecentHands.size() > 10)
	{
		Mode.RecentHands.erase(Mode.RecentHands.begin(), Mode.RecentHands.begin() + static_cast<std::ptrdiff_t>(Mode.RecentHands.size() - 10));
	}
}

void FAutoHost::SkippedEvents(const std::vector<ss::TEvent>& Events)
{
	// A tick's news (busts, levels, breaks, moves, the bubble, the final table): the room hears it as a catch-up (only the
	// last tick's cues go out), and the host does its part (banners, holds, the stage move at the final table).
	ss::Tournament* T = Tourney();
	if (!T)
	{
		return;
	}
	if (Mode.RoomHost.IsValid() && Mode.RoomDirector.IsValid())
	{
		RoomFeed::Simulated(*Mode.RoomHost, *Mode.RoomDirector, Mode.RoomSound.Get(), *T, Events, Mode.LiveDayStart + T->ClockMinutes());
	}
	for (const ss::TEvent& E : Events)
	{
		if (E.Type == ss::TEventType::Moved && E.IsHero)
		{
			bSkipMoved = true;
		}
		if (E.Type == ss::TEventType::Break)
		{
			SkipBreakTick = T->Tick;
		}
		// The room's other tables are put right once, where the skip ends (EndSkip's PlaceExtras), not at every table that
		// breaks or player who moves on the way (LiveEvent's own answer to those two is that and nothing else).
		if (E.Type == ss::TEventType::TableBroken || (E.Type == ss::TEventType::Moved && !E.IsHero))
		{
			continue;
		}
		Mode.LiveEvent(E);
	}
}

// ------------------------------------------------------------------ ss.Auto.* (Update2Commands.cpp forwards here)

FString FAutoHost::Command(const FString& Line)
{
	using ss::autoplay::Target;
	const TArray<FString> W = BackRoomAutoDetail::AutoWords(Line);
	const FString Verb = W.Num() > 0 ? W[0] : FString(TEXT("describe"));
	const FString Arg = W.Num() > 1 ? W[1] : FString();
	if (!bInit && Mode.Table && Mode.IsCareer())
	{
		Init();
	}
	if (!bInit)
	{
		return TEXT("Autoplay: no career night at this table (practice).");
	}
	const double NoteBefore = NoteAt;
	auto Answer = [this, NoteBefore]() {
		return NoteAt != NoteBefore ? NoteHead + TEXT(": ") + NoteBody + TEXT("\n") + Describe() : Describe();
	};
	if (Verb == TEXT("describe"))
	{
		return Describe();
	}
	if (Verb == TEXT("form"))
	{
		ss::SaveData* S = SaveOf();
		if (!S)
		{
			return TEXT("No career here.");
		}
		const double Form = FMath::Clamp(FCString::Atod(*Arg), 0.0, 100.0);
		ss::autoplay::FormAt(S->Life.Auto, Mode.Minutes);
		S->Life.Auto.Pref.Form = FMath::RoundToDouble(Form * 1000.0) / 1000.0;
		MarkUsed();
		return FString::Printf(TEXT("Form set to %d (dev). Toggle autoplay off and on to read Sharpness again."), FMath::RoundToInt(Form));
	}
	if (Verb == TEXT("style"))
	{
		ss::style::Style S = ss::style::Style::Autopilot;
		if (!ss::style::FromId(BackRoomAutoDetail::AutoUtf8(Arg), S))
		{
			return FString::Printf(TEXT("No style called \"%s\"."), *Arg);
		}
		const ss::Tournament* T = Tourney();
		const std::string Greyed = ss::autoplay::CantPick(S, Where, IsLive() && T && T->Spec.BountyCents > 0);
		if (!Greyed.empty() && Plan.M != ss::autoplay::Mode::Off && !bResumedPaused)
		{
			return BackRoomAutoDetail::AutoName(S) + TEXT(": ") + BackRoomAutoDetail::AutoSentence(Greyed);
		}
		Plan.S = S;
		MarkUsed();
		RefreshBrain();
		return FString::Printf(TEXT("Style: %s.%s"), *BackRoomAutoDetail::AutoName(S), Greyed.empty() ? TEXT("") : *(TEXT(" ") + BackRoomAutoDetail::AutoSentence(Greyed)));
	}
	if (Verb == TEXT("on"))
	{
		if (Plan.M == ss::autoplay::Mode::Off || bResumedPaused)
		{
			TurnOn();
		}
		return Answer();
	}
	if (Verb == TEXT("off"))
	{
		if (Plan.M != ss::autoplay::Mode::Off || bResumedPaused)
		{
			TurnOff(false);
		}
		return Answer();
	}
	if (Verb == TEXT("speed"))
	{
		Plan.Speed = BackRoomAutoDetail::AutoSpeedOf(FCString::Atoi(*Arg), !IsLive());
		MarkUsed();
		return FString::Printf(TEXT("Watch speed %s%s."), *BackRoomAutoDetail::AutoStr(ss::autoplay::SpeedName(Plan.Speed)), Plan.M == ss::autoplay::Mode::Off ? TEXT(" (autoplay is off)") : TEXT(""));
	}
	if (Verb == TEXT("skip"))
	{
		Target To = Target::Result;
		int32 Hands = 0;
		if (Arg == TEXT("break"))
		{
			To = Target::NextBreak;
		}
		else if (Arg == TEXT("bubble"))
		{
			To = Target::Bubble;
		}
		else if (Arg == TEXT("final"))
		{
			To = Target::FinalTable;
		}
		else if (Arg == TEXT("hands"))
		{
			To = Target::Hands;
			Hands = W.Num() > 2 ? FCString::Atoi(*W[2]) : 50;
		}
		else if (Arg == TEXT("closing"))
		{
			To = Target::Closing;
		}
		Skip(To, Hands);
		return Answer();
	}
	if (Verb == TEXT("takeover"))
	{
		TakeOver();
		return Answer();
	}
	if (Verb == TEXT("stop"))
	{
		StopSkip();
		return Answer();
	}
	if (Verb == TEXT("sharp"))
	{
		Run.Sharp = FMath::RoundToDouble(FMath::Clamp(FCString::Atod(*Arg), 0.5, 1.0) * 100.0) / 100.0;
		Run.SharpSetAt = Mode.Minutes;
		bSharpPinned = true;
		RefreshBrain();
		return FString::Printf(TEXT("Sharpness pinned at %d%% for this night (dev)."), FMath::RoundToInt(Run.Sharp * 100.0));
	}
	if (Verb == TEXT("run"))
	{
		return BackRoomAutoDetail::AutoStr(BackRoomAutoRules::AutoHostText(Plan, Run, Book, true));
	}
	if (Verb == TEXT("parity"))
	{
		// §2.12: the next N hands watched through the actor at 8x, then the same hands headless from the checkpoint saved
		// before them (live::SkipSlice on a copy of the night), both hashed; the room carries on from the watched hands.
		if (!IsLive() || !Tourney())
		{
			return TEXT("ss.Auto.Parity runs at the Embercrest, seated at a live table.");
		}
		if (bSkipping || bSkipPending || bParity || bParityArmed)
		{
			return TEXT("ss.Auto.Parity needs the table playing, not skipping or checking already.");
		}
		const FString Why = WhyNot();
		if (!Why.IsEmpty())
		{
			return Why;
		}
		const std::string Greyed = ss::autoplay::CantPick(Plan.S, Where, Tourney()->Spec.BountyCents > 0);
		if (!Greyed.empty())
		{
			return BackRoomAutoDetail::AutoName(Plan.S) + TEXT(": ") + BackRoomAutoDetail::AutoSentence(Greyed);
		}
		ParityHands = FMath::Clamp(FCString::Atoi(*Arg), 1, 500);
		ParityLeft = ParityHands;
		bParityArmed = true;
		return FString::Printf(TEXT("ss.Auto.Parity %d: the plan watches the next %d hands at 8x from the next gap between hands; both hashes print when they're done."), ParityHands,
			ParityHands);
	}
	return FString::Printf(TEXT("ss.Auto %s: not a verb here. Try style, on, off, speed, skip, takeover, stop, sharp, form, describe, parity."), *Verb);
}

void FAutoHost::FinishParity()
{
	// The watched hands are done (the actor's path: the tick finished by FinishTickWith, the run noted): the same hands from
	// the checkpoint saved before them, headless (live::SkipSlice on a copy of the night drawn as this one was), then both
	// hashes. The room carries on from the watched hands; the copy is thrown away.
	bParity = false;
	ss::Tournament* T = Tourney();
	ss::SaveData* S = SaveOf();
	const ss::autoplay::TableAuto Restore = ParityWas;
	auto Done = [this, Restore]() {
		Plan = Restore;
		if (Plan.M == ss::autoplay::Mode::Skip)
		{
			Plan.M = ss::autoplay::Mode::Watch;
		}
		RefreshBrain();
		ApplyTable();
	};
	if (!T || !S)
	{
		Done();
		return;
	}
	const uint64 Watched = BackRoomAutoRules::AutoHash(BackRoomAutoRules::AutoParityState(*T, ParityPlan, Run));
	const std::string EntryId = BackRoomAutoDetail::AutoUtf8(Mode.LiveEntryId);
	const ss::live::Occurrence O = ss::live::FindOccurrence(EntryId);
	const ss::life::LiveEntry* E = ss::live::EntryFor(S->Life, EntryId);
	if (!O.Valid() || !E)
	{
		UE_LOG(LogEmbercrest, Display, TEXT("ss.Auto.Parity: no entry for tonight to draw the night again from."));
		Done();
		return;
	}
	ss::life::LiveEntry Copy = *E;
	Copy.Checkpoint = ParityCk;
	std::vector<ss::ReservedPlayer> Known;
	for (const std::pair<std::string, int>& Who : Copy.Roster)
	{
		ss::ReservedPlayer P;
		P.Name = Who.first;
		P.Type = static_cast<ss::Archetype>(Who.second);
		Known.push_back(P);
	}
	bool bFits = false;
	std::unique_ptr<ss::Tournament> Night = ss::live::ResumeField(O, Copy, FMath::Max(Copy.Entrants, 2), S->HeroName, Known, true, bFits);
	if (!Night || !bFits)
	{
		UE_LOG(LogEmbercrest, Display, TEXT("ss.Auto.Parity: the checkpoint from before the watched hands didn't fit a fresh draw of the night."));
		Done();
		return;
	}
	ss::autoplay::Run Run2 = ParityRun;
	ss::autoplay::TableAuto Plan2 = ParityPlan;
	Plan2.M = ss::autoplay::Mode::Skip;
	Plan2.To = ss::autoplay::Target::Hands;
	Plan2.Stops = 0;
	const ss::reads::Table* Notes = &S->Life.Auto.Notes;
	const double Gain = TiltGain;
	const ss::style::Learned* LearnedBook = &Book;
	ss::style::HeroBrain Brain2 = ss::autoplay::BrainFor(Plan2, Run2, Gain, 1, Notes, LearnedBook);
	const BackRoomAutoRules::AutoReadModel Model = Reads;
	ss::Tournament* Copied = Night.get();
	Night->HeroReadHook = [&Model, &Run2, Copied](const ss::Hand& H, int Seat, ss::BotDecision& D) {
		BackRoomAutoRules::AutoApplyRead(Model, Run2.Tilt, *Copied, H, Seat, D, nullptr);
	};
	struct FParitySink : ss::live::SkipSink
	{
		ss::autoplay::Run* R = nullptr;
		ss::autoplay::TableAuto* A = nullptr;
		ss::style::HeroBrain* B = nullptr;
		const ss::reads::Table* N = nullptr;
		const ss::style::Learned* L = nullptr;
		double G = 1.0;
		virtual void HandDone(const ss::Hand& H, int HeroSeat) override
		{
			ss::autoplay::NoteSkippedHand(*R, H, HeroSeat, G);
			*B = ss::autoplay::BrainFor(*A, *R, G, 1, N, L);
		}
		virtual void Events(const std::vector<ss::TEvent>& E) override { (void)E; }
		virtual void Checkpoint(int Tick) override { (void)Tick; }
	} Sink;
	Sink.R = &Run2;
	Sink.A = &Plan2;
	Sink.B = &Brain2;
	Sink.N = Notes;
	Sink.L = LearnedBook;
	Sink.G = Gain;
	int32 Left = ParityHands;
	for (int32 Guard = 0; Guard < 100000 && Left > 0 && !Night->Hero().Busted && !Night->bFinished; ++Guard)
	{
		ss::autoplay::TableAuto A = Plan2;
		A.Hands = Left;
		const ss::live::SkipOut Out = ss::live::SkipSlice(*Night, Brain2, A, 1.0e9, Sink);
		Left -= Out.Hands;
		if (Out.Ticks == 0)
		{
			break;
		}
	}
	const uint64 Skipped = BackRoomAutoRules::AutoHash(BackRoomAutoRules::AutoParityState(*Night, ParityPlan, Run2));
	Night->HeroReadHook = nullptr;
	UE_LOG(LogEmbercrest, Display, TEXT("ss.Auto.Parity over %d hands: watched %s, skipped %s: %s"), ParityHands, *BackRoomAutoDetail::AutoStr(BackRoomAutoRules::AutoHex(Watched)),
		*BackRoomAutoDetail::AutoStr(BackRoomAutoRules::AutoHex(Skipped)), Watched == Skipped ? TEXT("equal.") : TEXT("DIFFERENT."));
	Done();
}
