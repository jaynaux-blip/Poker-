// ue-auto's own state on ABackRoomGameMode (update 2 SPEC §2.12): the plan playing the hero's seat at the Embercrest and at
// Dee's game, kept off the frozen BackRoomGameMode.h the way FRoomHost and FTableHost are. The lead declared it in Phase A
// with stub bodies; ue-auto filled them (BackRoomAuto.cpp, BackRoomTableAuto.cpp, the pure rules in BackRoomAutoRules.h).
//
// The hooks the hot files call, cut in Phase A (those files froze after it, so a hook's signature changes only with the lead):
//   BackRoomGameMode.cpp  StartPlay makes the host; Tick; the pawn's keys (PawnKeys) and its pace target (AutoWatchSpeed);
//                         OnTableNote's OnHandEnded and OnHeroBusted; Dee's clock (DeeMinutes)
//   BackRoomLive.cpp      the checkpoint's host text (HostLines, ReadHostLine); stand and sit (OnStand); the skip in
//                         LiveTick (TickSkip); the settle (OnSettle)
//   BackRoomLiveRoom.cpp  the director's cues (TalkGate)
//   BackRoomHud.cpp       FillTableHud (FillHud; YourTurn and the toast lane ask IsAutoPlaying), FillTableCard (FillCard),
//                         the legacy HUD's status line (StatusLine) and its ladder
//   BackRoomTable.cpp     the hero branch, the turn cue, the ReadHero bypass and EndHand's FinishTickWith (the table's own
//                         members, bodies in BackRoomTableAuto.cpp, which ask this host for the stop rules and the cash seed)
//   Update2Commands.cpp   ss.Auto.* in the card room (Command)
// FAutoHost is a friend of ABackRoomGameMode and ABackRoomPawn: its bodies read and move the night's own state (the
// tournament, the save, the room's feed, the pawn's keys) without either header changing again.
//
// The plan, in short. K (or holding A on the table card's PLAY page) hands the seat to the plan in the style shown; the table
// asks the plan at the hero's turn (Tournament::HeroDecisionFor, or style::DecideCash at Dee's), checks the stop rules, waits
// the plan's think and acts through HeroAct, so the body pushes the chips as it does for the player. Any action key during the
// think takes the wheel for the hand (the next press acts); K in the next hand's first 3 s turns the plan off. T steps the
// watch speed (the pawn's pace target). A skip (PLAY page) runs between hands through live::SkipSlice, a budgeted slice a
// frame, and the room hears it as a catch-up. The room reads the plan's bets through the engine's HeroReadHook (the same on
// both paths). The night's run rides in the checkpoint's host text, so a night closed mid-plan comes back paused at 1x.
#pragma once

#include "BackRoomAutoRules.h"
#include "CoreMinimal.h"
#include "ShortStack/Game/Autoplay.h"

#include <map>
#include <string>
#include <vector>

class ABackRoomGameMode;
class ABackRoomPawn;
class APlayerController;
struct FBackRoomPrompt;
namespace ss
{
struct SaveData;
class Tournament;
namespace recall
{
struct Record;
}
namespace room
{
struct Cue;
struct HandNote;
}
namespace ui
{
struct TableHudInfo;
struct TableCardInfo;
}
}

struct FAutoHost
{
	explicit FAutoHost(ABackRoomGameMode& InMode);
	~FAutoHost();
	/** The host on Mode (null before its StartPlay makes it): for the pawn, the HUDs and the console. */
	static FAutoHost* Of(const ABackRoomGameMode* Mode);

	ABackRoomGameMode& Mode;
	/** The table's plan (§2.6.2) and its run (the checkpoint host text's auto line). Off until the player turns it on. */
	ss::autoplay::TableAuto Plan;
	ss::autoplay::Run Run;

	// ---- ABackRoomGameMode's six (BackRoomAuto.cpp defines them there, forwarding here)
	bool IsPlaying() const;
	float WatchSpeed() const;
	void Set(const ss::autoplay::TableAuto& NewPlan);
	void TakeOver();
	void Skip(ss::autoplay::Target To, int32 Hands);
	FString Describe() const;

	// ---- the hooks
	/** Each frame of a career night (both rooms), after the pause menu's freeze and before the night's own tick. */
	void Tick(float RealDt);
	/**
	 * The seated pawn's keys, after the table card's (bCardLock: the card is up, or just put down): K, T while the plan holds
	 * the seat, the PLAY page's picks, the first action press during the plan's think (§2.6.2). True: this frame's table
	 * keys are the plan's, so neither the table's pace nor the actions run on them.
	 */
	bool PawnKeys(ABackRoomPawn& Pawn, APlayerController* PC, bool bCardLock);
	/**
	 * A hand the player was dealt ended (OnTableNote's HandEnded): its recall record, before the HANDS page takes it, and the
	 * room's note for it (ABackRoomTable::PeekHandNote, still there for the director; null when there's none).
	 */
	void OnHandEnded(const ss::recall::Record& Rec, const ss::room::HandNote* Note);
	/** Felted at Dee's (OnTableNote's HeroBusted): the plan stops there; the reload is the player's money. */
	void OnHeroBusted();
	/** Dee's clock: the minutes the night moves this frame, RealMinutes being today's (real seconds × 6). */
	double DeeMinutes(double RealMinutes);
	/** The checkpoint's host text: the auto, autoseen and autoread lines ("" when there's nothing to keep). */
	FString HostLines() const;
	/** One host-text line, split on tabs: true when it was one of the plan's (read back). */
	bool ReadHostLine(const TArray<FString>& Fields);
	/** Up from the table (bUp: the away rule has the chair) or back in it. */
	void OnStand(bool bUp);
	/** LiveTick, each frame before the test fast-forwards: the plan's skip, a budgeted slice between hands. True: the night's
	 *  tick stops here this frame (the skip ended the night: LiveOver has run). */
	bool TickSkip(float RealDt);
	/** The settle, just before the world hears of the night (NoteLive): the played line and the night's recap, into D. */
	void OnSettle(ss::SaveData& D, const std::string& EntryId);
	/** A cue of the room's director about to be voiced: false thins it out at the plan's watch speed. */
	bool TalkGate(const ss::room::Cue& Cue) const;
	/** The core HUD's plan fields and the P0a lines (§2.11), at the end of FillTableHud (Prompt: the table's this frame). */
	void FillHud(ss::ui::TableHudInfo& Out, const FBackRoomPrompt& Prompt) const;
	/** The table card's PLAY page (TableCardInfo::Play), at the end of FillTableCard. */
	void FillCard(ss::ui::TableCardInfo& Out) const;
	/** The legacy canvas HUD's status line (the P0a one), or "". */
	FString StatusLine() const;
	/** ss.Auto.* in the card room: the verb in lower case and its arguments ("style gto", "skip hands 50"). The answer. */
	FString Command(const FString& Line);

	// ---- the table's side (ABackRoomTable's members in BackRoomTableAuto.cpp call these)
	/** Dee's game's key for the plan's own mixes (style::DecideCash's MixSeed): the night, drawn from nothing. */
	uint64 CashMixSeed() const { return DeeSeed; }
	/** The stop rules against the decision the plan just drew (§2.6.4): true hands it to the player (a take-over, noted). */
	bool StopsHere(const ss::Hand& H, const ss::BotDecision& Planned, int32 HeroIdx);
	/** The plan acted at the table (one of its decisions, counted). */
	void NotePlanActed();
	/** The plan drew a decision it now holds through its think (the hand's history as it stood then). */
	void NotePlanDrew(const ss::Hand& H);
	/** The held decision is still this spot's: nobody acted since it was drawn (the seat is the only one to act meanwhile). */
	bool PlanDecisionFresh(const ss::Hand& H) const;

private:
	// ---- the night
	bool bInit = false;
	/** 1 the Embercrest, 2 Dee's game (style::HeroBrain::Where). */
	int32 Where = 2;
	double TiltGain = 1.0;
	uint64 DeeSeed = 0;
	/** The plan's brain (the table's HeroBrain points here while the plan has the seat) and the read book it seated with. */
	ss::style::HeroBrain Brain;
	ss::style::Learned Book;
	bool bBookRestored = false;
	/** The room's read on the plan (§2.8), the hook on the tournament, and the reads it took tonight. */
	BackRoomAutoRules::AutoReadModel Reads;
	bool bHooked = false;
	TSharedPtr<bool> Alive;
	std::vector<BackRoomAutoRules::AutoReadNote> ReadsTonight; // the last 12
	int32 ReadsTaken = 0;
	/** Dee's notes as the night began (the plan reads these, not the ones tonight's hands are folded into as they end). */
	ss::reads::Table NightNotes;
	std::map<std::string, double> NightNoteSeen;
	// ---- the run
	bool bEverOn = false;
	bool bRestoredAuto = false;
	/** A night come back to mid-plan: the plan waits, paused at 1x, until K (§2.12). */
	bool bResumedPaused = false;
	bool bSharpPinned = false;
	bool bPausedUp = false;
	/** The hand the table is on, how many of its decisions were the plan's, and whether the player was up during it. */
	int32 HandSeen = -1;
	int32 PlanActsThisHand = 0;
	int32 LastActHand = -1;
	/** Where the held decision was drawn: the table's hand number and the hand's history length then. */
	int32 DrewHand = -1;
	int32 DrewHistory = -1;
	/** The take-over was pressed between hands: it's the next hand's (OnNewHand hands that one to the player). */
	bool bTakeOverNext = false;
	bool bAwayThisHand = false;
	/** Dee's: the plan finishes the hand Dee called last (or the one she sends the player home after), then stops. */
	bool bStopAfterHand = false;
	double DeePending = 0.0;
	// ---- the skip
	bool bSkipPending = false;
	bool bSkipping = false;
	int32 SkipHandsLeft = 0;
	int32 SkipHands = 0;
	int32 SkipBreakTick = -1;
	int32 LastCheckpointTick = -1;
	bool bSkipMoved = false;
	bool bSkipCheckpointDue = false;
	std::string StopReason;
	// ---- keys (real seconds: FPlatformTime)
	double LockUntil = 0.0;
	double PadADownAt = -1.0, PadXDownAt = -1.0, EnterDownAt = -1.0;
	bool bPadAFired = false, bPadXFired = false, bEnterFired = false;
	bool bPadAWas = false, bPadXWas = false;
	int32 TargetIndex = 1;
	// ---- the note the HUD shows a moment (a toggle, a speed, a skip's end, the stand)
	FString NoteHead, NoteBody;
	double NoteAt = -100.0;
	// ---- ss.Auto.Parity
	bool bParityArmed = false;
	bool bParity = false;
	int32 ParityHands = 0, ParityLeft = 0;
	std::string ParityCk;
	ss::autoplay::TableAuto ParityPlan, ParityWas;
	ss::autoplay::Run ParityRun;

	void Init();
	bool IsLive() const;
	ss::Tournament* Tourney() const;
	ss::SaveData* SaveOf() const;
	bool IsDeeNightOne() const;
	/** Why the plan can't take the seat now ("" when it can). */
	FString WhyNot() const;
	void RefreshBrain();
	/** The table's HeroBrain and cards-known flags as the plan stands. */
	void ApplyTable();
	void EnsureHook();
	void ReadPlan(const ss::Hand& H, int32 Seat, ss::BotDecision& D);
	bool ReadsApply() const;
	/** Autoplay takes the seat's run (§2.5): Sharpness read now and held, the plan's tilt from the player's. */
	void StartRun();
	void TurnOn();
	void TurnOff(bool bQuiet);
	void TakeWheel(bool bMidThink);
	void OnK();
	void CycleStyle(int32 Dir);
	void CycleSpeed();
	void StopSkip();
	bool BeginSkip();
	bool EndSkip(int32 Why);
	void BlinkToTable();
	void OnNewHand(int32 HandNumber);
	void Note(const FString& Head, const FString& Body);
	void MarkUsed();
	ss::autoplay::Panel MakePanel() const;
	void SkippedHand(const ss::Hand& H, int32 HeroIdx);
	void SkippedEvents(const std::vector<ss::TEvent>& Events);
	void FoldDeeNotes();
	void FinishParity();
	friend struct FAutoSkipSink;
};
