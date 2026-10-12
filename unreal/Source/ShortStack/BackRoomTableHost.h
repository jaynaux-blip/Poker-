// ue-table's own state for the table HUD and its input (next_segment_spec 6.3, 6.4), kept off the frozen
// BackRoomGameMode.h: FTableHost on the game mode (the plates, what they keep clear, the projection, the field's counts,
// the audit), FTableHudHost on the HUD (the draw list's widget hint and measurer, the whispers). Made with MakeShared by
// the files that fill them (BackRoomHud.cpp, BackRoomGameMode.cpp). Also the recall lines a checkpoint's host text
// carries (the table card's HANDS page after a night comes back).
#pragma once

#include "CoreMinimal.h"
#include "BackRoomTable.h"
#include "SlateDrawList.h"
#include "ShortStack/Game/Recall.h"
#include "ShortStack/UI/TableHud.h"

#include <string>
#include <vector>

class UGameViewportClient;

struct FTableHost
{
	/** FPlatformTime seconds as of this frame's UpdatePlates: the HUD's clock (captions, toasts, tags all run on it). */
	double Now = 0.0;
	/** The view this frame: the viewport in pixels, the HUD's canvas (1,080 tall, the viewport's aspect), the projection. */
	FVector2D Viewport = FVector2D(1920.0, 1080.0);
	float CanvasW = 1920.0f;
	float PixelScale = 1.0f;
	bool bProjection = false;
	FMatrix ViewProjection = FMatrix::Identity;
	FIntRect ViewRect;
	FVector CameraAt = FVector::ZeroVector;
	FVector CameraRight = FVector::RightVector;
	FVector CameraForward = FVector::ForwardVector;

	/** The table's seats as the felt shows them this frame. */
	TArray<FBackRoomSeatView> Seats;
	/** Faces, cards and chips on screen (the HUD's units): plates never cover them. */
	std::vector<ss::ui::Rect> KeepClear;
	/** The player's cards and the board this frame (kept, so reading them each frame allocates nothing). */
	TArray<const ABackRoomCard*> Cards;
	/** The pot's tag. */
	ss::ui::Vec2 PotAt;
	bool bPotOnScreen = false;

	/** The clock facts this frame (TableHudInfo::Clock and TableCardInfo::Clock point here), and tonight's occurrence. */
	ss::live::ClockFacts Clock;
	bool bHasClock = false;
	ss::live::Occurrence Occurrence;
	FString OccurrenceFor;
	/** The player's life for the HUD's vitals (the save's, with tonight's energy), refreshed now and then. */
	TSharedPtr<ss::life::State> LifeView;
	double LifeAt = -100.0;
	/** The night's phase as of last frame, and when it changed (a held note's clock). */
	uint8 PhaseNow = 255;
	double PhaseAt = 0.0;
	/** The chips in play (THE ROOM'S WAY reads stacks in them), as of the level they were worked out for. */
	std::vector<ss::chipset::Denom> InPlay;
	int32 InPlayLevel = -2;

	/** THE ROOM'S WAY (next_segment_spec 6.2 A): how long Focus has held each seat (by chair), and the stack a long look
	 *  counted there (counted until it changes). */
	TMap<int32, float> StudyT;
	TMap<int32, int64> CountedStack;

	/** The FIELD page: the counts posted at the last break, and when ("9:00"). */
	std::vector<ss::ui::FieldRow> AtBreak;
	std::string CountsAt;
	bool bWasOnBreak = false;

	/** When the last hand was recorded (the last-hand card's clock). */
	double LastHandAt = -100.0;
	/** The first table's hints (seated, and on foot), and the one-time controls notice. */
	double HintsAt = -100.0;
	bool bHintsShown = false;
	double FootHintsAt = -100.0;
	bool bFootHintsShown = false;
	std::string Notice;
	std::string NoticeSub;
	double NoticeAt = -100.0;
	bool bNoticeChecked = false;
	/** The player's sit-down (the toast of what they wear), once a night. */
	bool bSatDown = false;
	/** The ledger as last seen (a sponsor's bonus line arriving is a toast). */
	int32 LedgerCount = -1;
	double LedgerLastAt = 0.0;

	/** The plate audit (ss.Table.PlateAudit): checks, mismatches past the chips' landing, piles a hand's end put right. */
	int64 AuditChecks = 0;
	int32 AuditMismatches = 0;
	int32 AuditFixes = 0;
	int32 AuditHand = -1;
	TSet<int32> AuditLogged;
};

struct FTableHudHost
{
	FSlateTextMeasurer Measurer;
	FDrawListHint Hint;
	TWeakObjectPtr<UGameViewportClient> Viewport;
	/** What the player's eye caught (the table's whispers), HUD clock. */
	std::vector<ss::ui::StreetHudInfo::Subtitle> Whispers;
	/** The table's own lines as captions, only until the room's captions start (ue-room's GetCaptions is the one source). */
	std::vector<ss::ui::Caption> Fallback;
	/** The draw list's cost (ms, averaged) for ss.Table.HudStats. */
	double BuildMs = 0.0;
	double BuildMsPeak = 0.0;
	int32 Frames = 0;
	double StatsAt = -100.0;
	/** The seats read, projected and audited each frame (GM's UpdatePlates and PlateAudit), ms averaged. */
	double UpdateMs = 0.0;
	bool bCoreShown = false;
};

/** The table card's HANDS across a night that comes back (the checkpoint's host text; spec 6.3: "recall in the host text"). */
namespace TableRecall
{
/** "recall\t<n>\t<record>\n" for the last ten hands (recall::Write is printable: no tabs or line breaks in it). */
inline FString HostLines(const std::vector<ss::recall::Record>& Hands)
{
	FString Out;
	const size_t From = Hands.size() > 10 ? Hands.size() - 10 : 0;
	for (size_t I = From; I < Hands.size(); ++I)
	{
		FString Line = UTF8_TO_TCHAR(ss::recall::Write(Hands[I]).c_str());
		Line.ReplaceInline(TEXT("\t"), TEXT(" "));
		Line.ReplaceInline(TEXT("\n"), TEXT(" "));
		Out += FString::Printf(TEXT("recall\t%d\t%s\n"), static_cast<int32>(I - From), *Line);
	}
	return Out;
}

/** One host-text line's fields (split on tabs): a recall line read back into Hands (true), else false. */
inline bool ReadHostLine(const TArray<FString>& Fields, std::vector<ss::recall::Record>& Hands)
{
	if (Fields.Num() < 3 || Fields[0] != TEXT("recall"))
	{
		return false;
	}
	ss::recall::Record R;
	if (ss::recall::Read(std::string(TCHAR_TO_UTF8(*Fields[2])), R))
	{
		Hands.push_back(R);
		if (Hands.size() > 10)
		{
			Hands.erase(Hands.begin());
		}
	}
	return true;
}
} // namespace TableRecall
