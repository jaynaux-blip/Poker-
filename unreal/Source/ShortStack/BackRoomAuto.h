// ue-auto's own state on ABackRoomGameMode (update 2 SPEC §2.12): the plan playing the hero's seat at the Embercrest and at
// Dee's game, kept off the frozen BackRoomGameMode.h the way FRoomHost and FTableHost are. The lead declared it in Phase A
// with stub bodies (BackRoomAuto.cpp) that keep today's behaviour: nothing turns autoplay on yet, so every hook below is a
// no-op and every question answers "the player decides". ue-auto owns this file, BackRoomAuto.cpp and BackRoomTableAuto.cpp
// (the table's side: ABackRoomTable's autoplay members) from here.
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
//                         members, bodies in BackRoomTableAuto.cpp)
//   Update2Commands.cpp   ss.Auto.* in the card room (Command)
// FAutoHost is a friend of ABackRoomGameMode and ABackRoomPawn: its bodies read and move the night's own state (the
// tournament, the save, the room's feed, the pawn's keys) without either header changing again.
#pragma once

#include "CoreMinimal.h"
#include "ShortStack/Game/Autoplay.h"

#include <string>

class ABackRoomGameMode;
class ABackRoomPawn;
class APlayerController;
struct FBackRoomPrompt;
namespace ss
{
struct SaveData;
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
};
