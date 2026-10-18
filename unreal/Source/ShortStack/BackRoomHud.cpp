// The table's HUD (ABackRoomHUD, Dee's game and the Embercrest), next_segment_spec 6.3. The core table HUD
// (ss::ui::DrawTableHud, core's visual language) drawn into an SDrawListWidget over the view from the game mode's
// FillTableHud; the canvas HUD it replaces stays for one milestone behind ss.Table.LegacyHud 1, and carries the AI stack
// plates too (P0a). The game mode's side is here as well: the seats read off the table each frame (GetSeatViews),
// projected into the HUD's 1,080-tall canvas, with what the plates keep clear (faces, cards, chips); the plate audit
// (ss.Table.PlateAudit); the toast lane held while it's the player's decision; the studying caption; the table card.

#include "BackRoomGameMode.h"
#include "BackRoomGameShared.h"
#include "BackRoomTableHost.h"

#include "BackRoomAuto.h"
#include "BackRoomCard.h"
#include "BackRoomChips.h"
#include "BackRoomLiveShared.h"
#include "BackRoomPlayer.h"
#include "BackRoomPlayerController.h"
#include "BackRoomStage.h"
#include "BackRoomTable.h"
#include "CanvasItem.h"
#include "Containers/Ticker.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#include "NightOneAudio.h"
#include "SceneView.h"
#include "ShortStack/Game/Badges.h"
#include "ShortStack/Game/ChipSet.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/TableTalk.h"
#include "ShortStack/Tournament.h"
#include "ShortStack/UI/Canvas.h"
#include "ShortStack/UI/TableHud.h"

#include <algorithm>
#include <string>
#include <vector>

DEFINE_LOG_CATEGORY_STATIC(LogTableHud, Log, All);

namespace BackRoomGameDetail
{
FString CardText(int32 Card)
{
	static const TCHAR* Ranks[13] = {TEXT("2"), TEXT("3"), TEXT("4"), TEXT("5"), TEXT("6"), TEXT("7"), TEXT("8"), TEXT("9"), TEXT("10"), TEXT("J"), TEXT("Q"), TEXT("K"), TEXT("A")};
	static const TCHAR* Suits[4] = {TEXT("♣"), TEXT("♦"), TEXT("♥"), TEXT("♠")};
	return FString(Ranks[(Card >> 2) % 13]) + Suits[Card & 3];
}

/** One beat of an ECG trace (P wave, the QRS spike, T wave) at phase P in 0..1. */
float Ecg(float P)
{
	auto G = [P](float Mu, float Sigma) { return FMath::Exp(-0.5f * FMath::Square((P - Mu) / Sigma)); };
	return 0.12f * G(0.12f, 0.025f) - 0.14f * G(0.232f, 0.008f) + 1.0f * G(0.25f, 0.009f) - 0.28f * G(0.27f, 0.01f) + 0.22f * G(0.48f, 0.045f);
}
} // namespace BackRoomGameDetail

namespace BackRoomHudDetail
{
// The core HUD is the table's HUD; the canvas HUD it replaced stays one milestone behind this (next_segment_spec 8.3).
TAutoConsoleVariable<int32> CVarLegacyHud(TEXT("ss.Table.LegacyHud"), 0,
	TEXT("The table's HUD: 0 the core HUD (ss::ui::DrawTableHud on Slate), 1 the legacy canvas HUD with the AI stack plates drawn on it (one milestone)."));
// Plates never lie (next_segment_spec 6.3): every plate against the engine's stack for its seat, each frame.
TAutoConsoleVariable<int32> CVarPlateAudit(TEXT("ss.Table.PlateAudit"), 0,
	TEXT("1: every plate is checked against the engine's stack for its seat each frame; a mismatch that outlasts the chips' landing is logged, ")
	TEXT("with a line a hand. 2: the same, and every plate every hand."));

// The HUD's cost on the game thread (next_segment_spec 10: the core draw list and its submit, 0.3 ms).
TAutoConsoleVariable<int32> CVarHudStats(TEXT("ss.Table.HudStats"), 0,
	TEXT("1: every 5 s, the core table HUD's cost (filling it, laying out the plates, drawing the list: ms average and peak), its plates and what they keep clear."));

/** Seconds a plate may differ from the engine while its chips land (a bet carried out, the pot pushed, a bet returned). */
constexpr float AuditGrace = 4.0f;
/** A long look counts a stack under THE ROOM'S WAY (next_segment_spec 6.2 A). */
constexpr float CountSeconds = 1.5f;
/** On foot, the plates of the table the player's near (the rail's view of it), within this many centimetres of it. */
constexpr double FootPlatesWithin = 700.0;
/** A face within about 6 degrees of the middle of the view is the one being looked at. */
const double LookedCos = FMath::Cos(FMath::DegreesToRadians(6.0));

std::string Utf8(const FString& S)
{
	return std::string(TCHAR_TO_UTF8(*S));
}

FString Str(const std::string& S)
{
	return FString(UTF8_TO_TCHAR(S.c_str()));
}

/** The one control map's gamepad buttons for a key the legacy canvas HUD's prompts name ("R" is Y, "Space" the left
 *  trigger), or the key itself. A control with no button on a pad (update 2's "auto": K is the keyboard's alone) isn't the
 *  pad's, so it's passed over rather than read out as "[]". */
FString PadNameFor(const FString& Key)
{
	FString K = Key.ToUpper();
	K = K == TEXT("RIGHT MOUSE") ? FString(TEXT("RMB")) : K;
	for (const ss::ui::ControlContext Ctx : {ss::ui::ControlContext::Table, ss::ui::ControlContext::CardRoomOnFoot})
	{
		for (const ss::ui::Control& C : ss::ui::Controls(Ctx))
		{
			if (C.Key && C.Pad && *C.Pad && K == UTF8_TO_TCHAR(C.Key))
			{
				return FString(UTF8_TO_TCHAR(C.Pad)) + (C.Hold ? TEXT(" held") : TEXT(""));
			}
		}
	}
	return Key;
}

/** A legacy prompt with its keys ("[N]" said with its brackets, "{Space}" bare): as written on the keyboard, the control
 *  map's buttons after a gamepad. */
FString KeyText(const FString& Line, bool bPad)
{
	FString Out;
	for (int32 I = 0; I < Line.Len(); ++I)
	{
		const TCHAR Ch = Line[I];
		const int32 End = Ch == TEXT('[') || Ch == TEXT('{') ? Line.Find(Ch == TEXT('[') ? TEXT("]") : TEXT("}"), ESearchCase::CaseSensitive, ESearchDir::FromStart, I + 1) : INDEX_NONE;
		if (End == INDEX_NONE)
		{
			Out.AppendChar(Ch);
			continue;
		}
		const FString Key = Line.Mid(I + 1, End - I - 1);
		I = End;
		const FString Shown = bPad ? PadNameFor(Key) : Key;
		Out += Ch == TEXT('[') ? TEXT("[") + Shown + TEXT("]") : Shown;
	}
	return Out;
}

/** World W on the HUD's canvas (1,080 tall): false behind the camera. */
bool Project(const FTableHost& H, const FVector& W, ss::ui::Vec2& Out)
{
	if (!H.bProjection)
	{
		return false;
	}
	FVector2D Px;
	if (!FSceneView::ProjectWorldToScreen(W, H.ViewRect, H.ViewProjection, Px))
	{
		return false;
	}
	Out.X = static_cast<float>(Px.X - H.ViewRect.Min.X) / H.PixelScale;
	Out.Y = static_cast<float>(Px.Y - H.ViewRect.Min.Y) / H.PixelScale;
	return true;
}

bool OnCanvas(const FTableHost& H, const ss::ui::Vec2& P, float Margin = 0.0f)
{
	return P.X >= -Margin && P.X <= H.CanvasW + Margin && P.Y >= -Margin && P.Y <= 1080.0f + Margin;
}

/** A box in the world as the canvas sees it (its eight corners), clipped to nothing when it's behind the camera. */
bool ProjectBox(const FTableHost& H, const FBox& B, ss::ui::Rect& Out)
{
	if (!B.IsValid)
	{
		return false;
	}
	float X0 = 1.0e9f, Y0 = 1.0e9f, X1 = -1.0e9f, Y1 = -1.0e9f;
	for (int32 K = 0; K < 8; ++K)
	{
		const FVector C((K & 1) ? B.Max.X : B.Min.X, (K & 2) ? B.Max.Y : B.Min.Y, (K & 4) ? B.Max.Z : B.Min.Z);
		ss::ui::Vec2 P;
		if (!Project(H, C, P))
		{
			return false;
		}
		X0 = FMath::Min(X0, P.X);
		Y0 = FMath::Min(Y0, P.Y);
		X1 = FMath::Max(X1, P.X);
		Y1 = FMath::Max(Y1, P.Y);
	}
	if (X1 < 0.0f || Y1 < 0.0f || X0 > H.CanvasW || Y0 > 1080.0f)
	{
		return false;
	}
	Out = {X0, Y0, X1 - X0, Y1 - Y0};
	return true;
}

/** A sphere (a head, a card's spot) as a rect on the canvas: Up and Down stretch it (a face is taller than it's wide). */
bool ProjectBlob(const FTableHost& H, const FVector& C, double Radius, float Up, float Down, ss::ui::Rect& Out)
{
	ss::ui::Vec2 P, Q;
	if (!Project(H, C, P) || !Project(H, C + H.CameraRight * Radius, Q))
	{
		return false;
	}
	const float R = FMath::Max(4.0f, FMath::Abs(Q.X - P.X) + FMath::Abs(Q.Y - P.Y) * 0.25f);
	if (P.X + R < 0.0f || P.X - R > H.CanvasW || P.Y + R * Down < 0.0f || P.Y - R * Up > 1080.0f)
	{
		return false;
	}
	Out = {P.X - R, P.Y - R * Up, 2.0f * R, R * (Up + Down)};
	return true;
}

/** "Nightly $120" from "Embercrest Nightly $120": the clock card's header. */
FString EventShort(const FString& LiveName)
{
	FString Event = LiveName;
	Event.RemoveFromStart(TEXT("Embercrest "));
	return Event;
}

/** "[E] Sit back down" split into the key and the words, for the street's prompt pill. */
void SplitPrompt(const FString& Prompt, std::string& OutKey, std::string& OutText)
{
	OutKey = "E";
	OutText = Utf8(Prompt);
	if (Prompt.StartsWith(TEXT("[")))
	{
		const int32 End = Prompt.Find(TEXT("]"));
		if (End > 1)
		{
			OutKey = Utf8(Prompt.Mid(1, End - 1));
			OutText = Utf8(Prompt.Mid(End + 1).TrimStartAndEnd());
		}
	}
}

/** The sponsor's bonus as the toast says it ("Final table bonus · +$50") from its kind-11 ledger line. */
bool BonusToast(const ss::life::LedgerEntry& L, FString& OutFrom, FString& OutLine)
{
	const FString Label = Str(L.Label);
	if (L.Kind != 11 || L.Amount <= 0 || !Label.Contains(TEXT("bonus")))
	{
		return false;
	}
	FString Brand, What;
	if (!Label.Split(TEXT(": "), &Brand, &What))
	{
		return false;
	}
	What = What.Left(1).ToUpper() + What.Mid(1);
	OutFrom = Brand;
	OutLine = FString::Printf(TEXT("%s · +$%s"), *What, *FText::AsNumber(L.Amount / 100).ToString());
	return true;
}
} // namespace BackRoomHudDetail

using namespace BackRoomGameDetail;
using namespace BackRoomHudDetail;

// ------------------------------------------------------------------ HUD

void ABackRoomHUD::DrawHUD()
{
	Super::DrawHUD();
	ABackRoomGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ABackRoomGameMode>() : nullptr;
	ABackRoomTable* Table = GM ? GM->Table.Get() : nullptr;
	// The pause menu has the screen (or a quit is on its way out behind the fade): nothing of the table's over it, and
	// nothing of it ages meanwhile (a line said just before is still up when the menu goes).
	if (!Table || !Canvas || GM->IsPauseMenuUp() || GM->IsQuittingTable())
	{
		if (HudWidget)
		{
			HudWidget->SetDrawList(nullptr);
		}
		return;
	}
	const float Dt = FMath::Min(static_cast<float>(FApp::GetDeltaTime()), 0.1f);
	Clock += Dt;
	const double Now = FPlatformTime::Seconds();
	if (!HudHost)
	{
		HudHost = MakeShared<FTableHudHost>();
	}
	// The seats off the table (projected with this frame's camera), and the audit of what they show.
	const uint64 U0 = FPlatformTime::Cycles64();
	GM->UpdatePlates(Dt);
	GM->PlateAudit(Dt);
	const double UpdateMs = FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64() - U0);
	HudHost->UpdateMs = HudHost->Frames == 0 ? UpdateMs : HudHost->UpdateMs * 0.97 + UpdateMs * 0.03;
	// Table talk becomes subtitles; what your eye catches becomes a whisper. (The core HUD's captions are the room's own,
	// GM->GetCaptions(); the table's lines stand in only until the room has said anything, so nothing is said twice.)
	for (const FBackRoomLine& L : Table->TakeLines())
	{
		if (L.bRead)
		{
			Whispers.Add({FString(), L.Text, Clock + 4.8f + 0.045f * L.Text.Len(), Clock});
			ss::ui::StreetHudInfo::Subtitle W;
			W.Line = Utf8(L.Text);
			W.At = Now;
			HudHost->Whispers.push_back(W);
		}
		else
		{
			Subtitles.Add({L.Speaker, L.Text, Clock + 2.8f + 0.055f * L.Text.Len(), Clock});
			ss::ui::Caption C;
			C.Speaker = Utf8(L.Speaker);
			C.Line = Utf8(L.Text);
			C.Style = L.Speaker == TEXT("Floor") ? 1 : 0;
			C.At = Now;
			HudHost->Fallback.push_back(C);
		}
	}
	if (HudHost->Whispers.size() > 6)
	{
		HudHost->Whispers.erase(HudHost->Whispers.begin(), HudHost->Whispers.end() - 6);
	}
	if (HudHost->Fallback.size() > 8 || !GM->GetNightLog().empty())
	{
		HudHost->Fallback.erase(HudHost->Fallback.begin(), GM->GetNightLog().empty() ? HudHost->Fallback.end() - 8 : HudHost->Fallback.end());
	}
	// The heart: a trace that runs with it (240 samples a second, two seconds of it).
	{
		const float Bpm = Table->Composure.Bpm;
		const int32 Rate = 240;
		const int32 Keep = 2 * Rate;
		TraceAccum += Dt;
		while (TraceAccum >= 1.0f / Rate)
		{
			TraceAccum -= 1.0f / Rate;
			TracePhase += Bpm / 60.0f / Rate;
			TracePhase -= FMath::FloorToFloat(TracePhase);
			Trace.Add(Ecg(TracePhase));
		}
		if (Trace.Num() > Keep)
		{
			Trace.RemoveAt(0, Trace.Num() - Keep);
		}
		HeartAlpha = Ease(HeartAlpha, FMath::Clamp((Bpm - 72.0f) / 18.0f, 0.3f, 1.0f), 3.0f, Dt);
	}
	if (CVarLegacyHud.GetValueOnGameThread() == 0)
	{
		DrawCoreHud();
		if (HudHost->bCoreShown)
		{
			// Over the shoulder the eyes' tiredness is the room dimming (the core HUD's lids are the first-person eyes').
			const ABackRoomPawn* P = Cast<ABackRoomPawn>(GetOwningPawn());
			const float Out = P ? P->GetThirdBlend() : 0.0f;
			if (GM->GetEyelids() > 0.01f && Out > 0.01f)
			{
				DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.78f * GM->GetEyelids() * Out), 0.0f, 0.0f, Canvas->ClipX, Canvas->ClipY);
			}
			return;
		}
	}
	else if (HudWidget)
	{
		HudWidget->SetDrawList(nullptr);
		HudHost->bCoreShown = false;
	}

	// ================================================================ the legacy canvas HUD (ss.Table.LegacyHud 1)
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	const float S = H / 1080.0f;
	UFont* Big = GEngine->GetLargeFont();
	UFont* Small = GEngine->GetMediumFont();
	const FLinearColor Paper(0.93f, 0.9f, 0.82f, 0.92f);
	const FLinearColor Dim(0.93f, 0.9f, 0.82f, 0.55f);
	const FLinearColor Warm(1.0f, 0.78f, 0.45f, 0.95f);
	const FLinearColor Read(0.64f, 0.86f, 0.92f, 0.92f);
	const FLinearColor Blood(1.0f, 0.36f, 0.3f, 0.95f);
	const ABackRoomPawn* Pawn = Cast<ABackRoomPawn>(GetOwningPawn());
	const EBackRoomPhase Phase = GM->GetPhase();

	// Align: 0 left, 1 center, 2 right.
	auto Text = [&](const FString& T, const FLinearColor& Color, float X, float Y, UFont* Font, float Scale, int32 Align) {
		if (Color.A <= 0.005f)
		{
			return 0.0f;
		}
		float Tw = 0.0f, Th = 0.0f;
		GetTextSize(T, Tw, Th, Font, Scale * S);
		const float Px = Align == 1 ? X - Tw * 0.5f : (Align == 2 ? X - Tw : X);
		DrawText(T, FLinearColor(0.0f, 0.0f, 0.0f, Color.A * 0.6f), Px + 2.0f * S, Y + 2.0f * S, Font, Scale * S);
		DrawText(T, Color, Px, Y, Font, Scale * S);
		return Tw;
	};
	auto Fade = [](FLinearColor C, float A) {
		C.A *= FMath::Clamp(A, 0.0f, 1.0f);
		return C;
	};

	// After a gamepad, the prompts and hints name its buttons (the one control map's, ss::ui::Controls).
	const ABackRoomPlayerController* Pad = Cast<ABackRoomPlayerController>(PlayerOwner);
	const bool bPad = Pad && Pad->IsUsingGamepad();
	auto KeyLine = [&](const FString& Line, const FLinearColor& Color, float X, float Y, UFont* Font, float Scale, int32 Align) -> float {
		return Text(KeyText(Line, bPad), Color, X, Y, Font, Scale, Align);
	};

	// Tired: the lids come down from above and below, soft-edged. Over the shoulder they aren't the camera's: the room
	// dims instead, as the eyes it's watching go heavy.
	const float Outside = Pawn ? Pawn->GetThirdBlend() : 0.0f;
	if (GM->GetEyelids() > 0.01f && Outside > 0.01f)
	{
		DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.78f * GM->GetEyelids() * Outside), 0.0f, 0.0f, W, H);
	}
	if (GM->GetEyelids() > 0.01f && Outside < 0.99f)
	{
		const float Lid = GM->GetEyelids() * (1.0f - Outside) * H * 0.5f;
		DrawRect(FLinearColor::Black, 0.0f, 0.0f, W, Lid);
		DrawRect(FLinearColor::Black, 0.0f, H - Lid, W, Lid);
		const float Band = 10.0f * S;
		for (int32 I = 0; I < 7; ++I)
		{
			const FLinearColor Edge(0.0f, 0.0f, 0.0f, 1.0f - (I + 1) / 8.0f);
			DrawRect(Edge, 0.0f, Lid + I * Band, W, Band);
			DrawRect(Edge, 0.0f, H - Lid - (I + 1) * Band, W, Band);
		}
	}

	// The AI stack plates (P0a), under everything else.
	if (!(Pawn && Pawn->IsWalking()) && Phase != EBackRoomPhase::Moving)
	{
		DrawLegacyPlates(W, H, S);
	}

	Subtitles.RemoveAll([this](const FShown& Sh) { return Sh.Until < Clock; });
	while (Subtitles.Num() > 3)
	{
		Subtitles.RemoveAt(0);
	}
	float Y = H * 0.74f;
	for (const FShown& Sh : Subtitles)
	{
		const float A = FMath::Clamp(Sh.Until - Clock, 0.0f, 1.0f);
		Text(Sh.Speaker.IsEmpty() ? Sh.Text : Sh.Speaker + TEXT(":  ") + Sh.Text, Fade(Sh.Speaker.IsEmpty() ? Warm : Paper, A), W * 0.5f, Y, Small, 1.35f, 1);
		Y += 30.0f * S;
	}
	Whispers.RemoveAll([this](const FShown& Sh) { return Sh.Until < Clock; });
	while (Whispers.Num() > 3)
	{
		Whispers.RemoveAt(0);
	}
	float Wy = H * 0.15f;
	for (const FShown& Sh : Whispers)
	{
		const float A = FMath::Clamp(Sh.Until - Clock, 0.0f, 1.0f) * FMath::Clamp((Clock - Sh.From) / 0.4f, 0.0f, 1.0f);
		const bool bLearned = Sh.Text.StartsWith(TEXT("READ LEARNED"));
		Text(Sh.Text, Fade(bLearned ? Warm : Read, A), W * 0.5f, Wy, Small, bLearned ? 1.32f : 1.15f, 1);
		Wy += 30.0f * S;
	}

	// Walking in: where you are, and when.
	const float Arrive = GM->GetArrivalTime();
	if (Arrive >= 0.0f)
	{
		const float A = Smooth(0.6f, 1.8f, Arrive) * (1.0f - Smooth(5.0f, 6.4f, Arrive));
		Text(GM->IsLive() ? TEXT("E M B E R C R E S T    C A S I N O") : TEXT("W A S H   &   F O L D"), Fade(Paper, A), W * 0.5f, H * 0.4f, Big, 1.35f, 1);
		Text(GM->IsLive() ? TEXT("the card room   ·   ") + GM->GetLiveName().ToLower() : FString(TEXT("the back room")), Fade(Warm, A), W * 0.5f, H * 0.4f + 50.0f * S, Small, 1.45f, 1);
		Text(GM->ArrivalDay(), Fade(Dim, A), W * 0.5f, H * 0.4f + 88.0f * S, Small, 1.1f, 1);
	}

	// On foot: what's in reach, and how the night stands while you're up (or, out of it, the offer to stay).
	auto DrawOnFoot = [&]() {
		if (!GM->IsLive())
		{
			return;
		}
		if (Pawn && Pawn->IsFreeWalking())
		{
			// (The room's prompts name E as "[E]": a gamepad's A in its place.)
			const FString Prompt = GM->SpotPrompt();
			if (!Prompt.IsEmpty())
			{
				KeyLine(Prompt, Paper, W * 0.5f, H - 120.0f * S, Small, 1.3f, 1);
			}
			const ss::Tournament* T = GM->GetTourney();
			FString State;
			if (GM->IsRailing())
			{
				// Registered at the desk for the next event: that's what the night is waiting on now.
				const FString Next = GM->GetNextEventNote();
				if (!Next.IsEmpty())
				{
					State = T && !T->bFinished ? FString::Printf(TEXT("%s   ·   on the rail, %d left"), *Next, T->Remaining) : Next;
				}
				else
				{
					State = T && T->bFinished ? FString(TEXT("It's over.   [E] at the door to go home"))
											  : FString::Printf(TEXT("On the rail   ·   %d left   ·   [E] at the door to go home"), T ? T->Remaining : 0);
				}
			}
			else if (T)
			{
				State = FString::Printf(TEXT("Up from table %d   ·   still dealt in: checked when free, mucked when not   ·   [E] at your chair to sit"), T->Hero().TableId);
			}
			KeyLine(State, Fade(Dim, 0.9f), W * 0.5f, H - 78.0f * S, Small, 1.0f, 1);
			if (GM->ShowsHints())
			{
				KeyLine(TEXT("{WASD}  walk   ·   {Shift}  hurry   ·   {E}  use"), Fade(Dim, 0.6f), W * 0.5f, H - 46.0f * S, Small, 0.9f, 1);
			}
		}
		else if (GM->StayOffer() >= 0.0f)
		{
			const float A = FMath::Clamp((GM->StayOffer() - 1.0f) / 0.6f, 0.0f, 1.0f);
			KeyLine(TEXT("[Q] Stay and watch   ·   [L] Go home now"), Fade(Paper, A), W * 0.5f, H - 70.0f * S, Small, 1.15f, 1);
		}
	};
	// A big moment, across the room's attention.
	auto DrawBanner = [&]() {
		float Age = 0.0f;
		const FString& B = GM->GetBanner(Age);
		const float A = FMath::Clamp(Age / 0.3f, 0.0f, 1.0f) * (1.0f - FMath::Clamp((Age - 3.4f) / 0.8f, 0.0f, 1.0f));
		if (!B.IsEmpty() && A > 0.01f)
		{
			const float By = H * 0.085f;
			DrawRect(FLinearColor(Warm.R, Warm.G, Warm.B, 0.65f * A), W * 0.38f, By - 10.0f * S, W * 0.24f, 2.0f * S);
			Text(B, Fade(Paper, A), W * 0.5f, By, Big, 1.45f, 1);
			DrawRect(FLinearColor(Warm.R, Warm.G, Warm.B, 0.65f * A), W * 0.38f, By + 50.0f * S, W * 0.24f, 2.0f * S);
		}
	};
	// V: the view, named for a moment at Vy (on foot it's for when they sit again).
	auto DrawViewNote = [&](float Vy) {
		if (!Pawn || Pawn->ViewShownAge() >= 2.4f)
		{
			return;
		}
		const float A = FMath::Clamp((2.4f - Pawn->ViewShownAge()) / 0.5f, 0.0f, 1.0f);
		const bool bOver = Pawn->IsThirdPerson();
		Text(bOver ? TEXT("OVER THE SHOULDER") : TEXT("FIRST PERSON"), Fade(Warm, A), W * 0.5f, Vy, Small, 1.2f, 1);
		KeyLine(bOver ? (Pawn->IsFreeWalking() ? TEXT("back at the table   ·   peeks and Focus are still through your own eyes")
											 : TEXT("{B}  other shoulder   ·   peeks and Focus are still through your own eyes"))
					: TEXT("{V}  over the shoulder"),
			Fade(Dim, A), W * 0.5f, Vy + 30.0f * S, Small, 1.0f, 1);
	};
	// Down on the d-pad, held: what it's about to do, filling as it's held (seated: on foot it isn't the way out).
	auto DrawLeaveHold = [&]() {
		const float Held = bPad && Pad && Pawn && !Pawn->IsFreeWalking() && !Pawn->IsWalking() ? Pad->HoldProgress(EKeys::Gamepad_DPad_Down) : 0.0f;
		const bool bOut = Phase == EBackRoomPhase::Leaving && GM->StayOffer() >= 0.0f;
		if (Held <= 0.0f || !GM->IsCareer() || GM->IsLeaveRequested() || !(Phase == EBackRoomPhase::Playing || Phase == EBackRoomPhase::Busted || bOut))
		{
			return;
		}
		const TCHAR* What = bOut							? TEXT("{L}  go home now")
							: Phase == EBackRoomPhase::Busted ? TEXT("{L}  go home")
							: GM->IsLive()					? (GM->IsQuitPending() ? TEXT("{L}  walk away: your stack is blinded off") : TEXT("{L}  walk away"))
															: TEXT("{L}  rack up after this hand");
		const float Lw = KeyLine(What, Paper, W * 0.5f, H - 236.0f * S, Small, 1.1f, 1);
		DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.4f), W * 0.5f - Lw * 0.5f, H - 202.0f * S, Lw, 3.0f * S);
		DrawRect(Warm, W * 0.5f - Lw * 0.5f, H - 202.0f * S, Lw * Held, 3.0f * S);
	};

	// Racked up: the night, totted up, over the walk out.
	if (Phase == EBackRoomPhase::Leaving)
	{
		const float T = GM->GetLeaveTime();
		const TArray<FString>& Lines = GM->GetSummary();
		const float A = Smooth(1.8f, 2.8f, T) * (1.0f - Smooth(29.0f, 30.0f, T));
		if (A > 0.01f && Lines.Num() > 0)
		{
			const float Bw = 560.0f * S;
			const float Bh = (70.0f + 34.0f * Lines.Num()) * S;
			const float Bx = W * 0.5f - Bw * 0.5f, By = H * 0.3f;
			DrawRect(FLinearColor(0.02f, 0.02f, 0.02f, 0.62f * A), Bx, By, Bw, Bh);
			DrawRect(FLinearColor(Warm.R, Warm.G, Warm.B, 0.7f * A), Bx, By, Bw, 2.0f * S);
			float Ly = By + 22.0f * S;
			for (int32 I = 0; I < Lines.Num(); ++I)
			{
				const FString& L = Lines[I];
				if (I == 0)
				{
					Text(L, Fade(Paper, A), W * 0.5f, Ly, Big, 1.2f, 1);
					Ly += 52.0f * S;
					continue;
				}
				const bool bNet = L.StartsWith(TEXT("Net"));
				const bool bRead = L.StartsWith(TEXT("Read"));
				const FLinearColor C = bNet ? (GM->GetNetCents() >= 0 ? FLinearColor(0.55f, 0.92f, 0.6f, 1.0f) : Blood) : (bRead ? Warm : (I == 1 ? Dim : Paper));
				Text(L, Fade(C, A), W * 0.5f, Ly, Small, bNet ? 1.35f : 1.15f, 1);
				Ly += 34.0f * S;
			}
		}
		DrawLeaveHold();
		DrawOnFoot();
		DrawBanner();
		// (On the rail, below where the night's summary stands.)
		DrawViewNote(H * 0.62f);
		return;
	}
	DrawBanner();
	// Under the pace's note when both are up.
	DrawViewNote(H * 0.3f + (GM->IsLive() && GM->PaceShownAge() < 2.6f ? 76.0f * S : 0.0f));
	if ((Pawn && Pawn->IsWalking()) || Phase == EBackRoomPhase::Moving)
	{
		return;
	}

	// Your stack, the pot, and your cards once you've looked.
	const FBackRoomPrompt P = Table->GetPrompt();
	const float Left = 48.0f * S;
	const bool bLive = GM->IsLive();
	// Money as the table counts it: dollars at Dee's, tournament chips at the Embercrest.
	auto Amount = [bLive](int64 V) { return bLive ? FText::AsNumber(V).ToString() : FString::Printf(TEXT("$%lld"), V); };
	const float StackW = Text(Amount(P.Stack), Paper, Left, H - 128.0f * S, Big, 1.6f, 0);
	if (bLive && P.BigBlind > 0)
	{
		Text(Str(ss::chipset::InBb(P.Stack, P.BigBlind)), Dim, Left + StackW + 14.0f * S, H - 114.0f * S, Small, 1.1f, 0);
	}
	FString Under = TEXT("Pot ") + Amount(P.Pot);
	if (P.Known.Num() == 2)
	{
		Under += TEXT("     ") + CardText(P.Known[0]) + TEXT(" ") + CardText(P.Known[1]);
	}
	Text(Under, Dim, Left, H - 72.0f * S, Small, 1.2f, 0);

	// The heart: a trace that runs with it, bright when it races.
	{
		const float Bpm = Table->Composure.Bpm;
		const int32 Keep = Trace.Num();
		const float Tw = 190.0f * S, Th = 36.0f * S;
		const float Tx = Left, Ty = H - 206.0f * S;
		const FLinearColor Line = FMath::Lerp(FLinearColor(0.62f, 0.95f, 0.78f, 1.0f), Blood, FMath::Clamp((Bpm - 84.0f) / 30.0f, 0.0f, 1.0f));
		for (int32 I = 1; I < Keep; ++I)
		{
			const float X0 = Tx + Tw * (I - 1) / (Keep - 1.0f), X1 = Tx + Tw * I / (Keep - 1.0f);
			const float Y0 = Ty + Th * (0.72f - 0.62f * Trace[I - 1]), Y1 = Ty + Th * (0.72f - 0.62f * Trace[I]);
			// Older trace fades out; the line glows a little, like a monitor.
			const float Fresh = FMath::Square(static_cast<float>(I) / Keep);
			DrawLine(X0, Y0, X1, Y1, Fade(Line, HeartAlpha * 0.16f * Fresh), 5.0f * S);
			DrawLine(X0, Y0, X1, Y1, Fade(Line, HeartAlpha * (0.15f + 0.85f * Fresh)), 1.8f * S);
		}
		// The heart itself, kicking with each beat.
		const float Beat = FMath::Clamp(Ecg(TracePhase) * 1.4f, 0.0f, 1.0f);
		Text(TEXT("♥"), Fade(Line, HeartAlpha), Tx + Tw + 14.0f * S, Ty + 4.0f * S - 3.0f * S * Beat, Small, 1.1f + 0.35f * Beat, 0);
		Text(FString::Printf(TEXT("%d"), FMath::RoundToInt(Bpm)), Fade(Line, HeartAlpha), Tx + Tw + 44.0f * S, Ty - 2.0f * S, Big, 1.35f, 0);
		if (Pawn && Pawn->bSteadying)
		{
			Text(TEXT("breathing slow"), Fade(Dim, HeartAlpha), Tx, Ty - 24.0f * S, Small, 0.9f, 0);
		}
	}

	// Your turn: what you can do, a ladder of keys on the right, the call (or check) lit; the wheel sizes the raise.
	const FLinearColor Ember(1.0f, 0.42f, 0.14f, 0.95f);
	const FLinearColor Night(0.015f, 0.025f, 0.055f, 0.66f);
	// Where the ladder starts and the tournament panel ends, for what else shares the screen's sides.
	float LadderTop = H;
	float PanelBottom = 0.0f;
	if (P.bYourTurn && Phase != EBackRoomPhase::Busted && Phase != EBackRoomPhase::Moving && !GM->IsHeroUp() && !GM->IsAutoPlaying())
	{
		// The one control map's keys (ss::ui::Controls): X fold, A check or call, Y raise (on release), Y or A held all in.
		struct FRung
		{
			const char* Id;
			FString Label;
			FString Value;
			bool bMain;
		};
		TArray<FRung> Rungs;
		Rungs.Add({"fold", TEXT("FOLD"), FString(), false});
		Rungs.Add({"call", P.bCanCheck ? TEXT("CHECK") : TEXT("CALL"), P.bCanCheck ? FString() : Amount(P.ToCall), true});
		if (P.bCanRaise)
		{
			Rungs.Add({"raise", P.bIsBet ? TEXT("BET") : TEXT("RAISE TO"), Amount(P.RaiseTo), false});
			Rungs.Add({"allin", TEXT("ALL IN"), Amount(P.Stack), false});
		}
		const float Rw = 320.0f * S, Rh = 46.0f * S, Gap = 8.0f * S;
		const float Rx = W - 48.0f * S - Rw;
		float Ry = H - 96.0f * S - Rungs.Num() * (Rh + Gap);
		LadderTop = Ry - 46.0f * S;
		const FString Head = P.ToCall > 0 ? Amount(P.ToCall) + TEXT(" to call") : TEXT("Checks to you");
		Text(Head, Warm, Rx + Rw, Ry - 46.0f * S, Big, 1.05f, 2);
		for (const FRung& R : Rungs)
		{
			const ss::ui::Control* K = ss::ui::FindControl(ss::ui::ControlContext::Table, R.Id);
			const bool bAllIn = FCStringAnsi::Strcmp(R.Id, "allin") == 0;
			DrawRect(R.bMain ? FLinearColor(0.05f, 0.08f, 0.15f, 0.84f) : Night, Rx, Ry, Rw, Rh);
			if (bAllIn && Pawn && Pawn->GetAllInHold() > 0.0f)
			{
				// Held to go all in: the rung fills as it's held.
				DrawRect(FLinearColor(Ember.R, Ember.G, Ember.B, 0.32f), Rx, Ry, Rw * Pawn->GetAllInHold(), Rh);
			}
			if (R.bMain)
			{
				DrawRect(Ember, Rx, Ry, Rw, 2.0f * S);
				DrawRect(Ember, Rx, Ry + Rh - 2.0f * S, Rw, 2.0f * S);
				DrawRect(Ember, Rx, Ry, 2.0f * S, Rh);
				DrawRect(Ember, Rx + Rw - 2.0f * S, Ry, 2.0f * S, Rh);
			}
			const FString KeyName = K ? FString(UTF8_TO_TCHAR(bPad ? K->Pad : K->Key)) : FString();
			DrawRect(FLinearColor(0.93f, 0.9f, 0.82f, 0.14f), Rx + 10.0f * S, Ry + 9.0f * S, 28.0f * S, 28.0f * S);
			Text(KeyName, Paper, Rx + 24.0f * S, Ry + 11.0f * S, Small, KeyName.Len() > 1 ? 0.7f : 1.05f, 1);
			float LabelX = Rx + 52.0f * S;
			LabelX += Text(R.Label, Paper, LabelX, Ry + 11.0f * S, Small, 1.15f, 0);
			if (bAllIn)
			{
				const float Fill = Pawn ? Pawn->GetAllInHold() : 0.0f;
				Text(Fill > 0.0f && Fill < 1.0f ? TEXT("LET GO TO CANCEL") : TEXT("HOLD"), Fade(Paper, 0.6f), LabelX + 10.0f * S, Ry + 16.0f * S, Small, 0.75f, 0);
			}
			if (!R.Value.IsEmpty())
			{
				Text(R.Value, R.bMain ? Warm : Paper, Rx + Rw - 16.0f * S, Ry + 11.0f * S, Small, 1.15f, 2);
			}
			Ry += Rh + Gap;
		}
		if (P.bCanRaise)
		{
			Text(bPad ? TEXT("LB RB  size the raise") : TEXT("1-6 presets  ·  wheel to size the raise"), Dim, Rx + Rw, Ry + 2.0f * S, Small, 0.95f, 2);
		}
	}

	// Out of the hand at a live table: the room's pace, and the rest of the hand at a glance.
	if (bLive && !P.bYourTurn && Phase == EBackRoomPhase::Playing && !Table->IsHeroInHand() && !GM->IsHeroUp())
	{
		static const TCHAR* PaceNames[3] = {TEXT("LIVE"), TEXT("BRISK"), TEXT("FAST")};
		const bool bSkipping = Pawn && Pawn->IsSkippingHand();
		KeyLine(bSkipping ? FString(TEXT("to the next hand...")) : FString::Printf(TEXT("[N] Next hand     [T] Pace  %s"), PaceNames[FMath::Clamp(GM->GetTablePace(), 0, 2)]),
			Fade(Dim, 0.85f), W * 0.5f, H - 66.0f * S, Small, 1.0f, 1);
	}
	if (bLive && GM->PaceShownAge() < 2.6f)
	{
		static const TCHAR* PaceNotes[3] = {TEXT("TABLE PACE   LIVE   ·   the room's own time"), TEXT("TABLE PACE   BRISK   ·   quicker decisions around you"),
			TEXT("TABLE PACE   FAST   ·   the room acts at once")};
		const float A = FMath::Clamp((2.6f - GM->PaceShownAge()) / 0.5f, 0.0f, 1.0f);
		Text(PaceNotes[FMath::Clamp(GM->GetTablePace(), 0, 2)], Fade(Warm, A), W * 0.5f, H * 0.3f, Small, 1.2f, 1);
		Text(TEXT("your own decisions wait for you, unless you've handed the seat to autoplay"), Fade(Dim, A), W * 0.5f, H * 0.3f + 30.0f * S, Small, 1.0f, 1);
	}

	// Felted.
	if (Phase == EBackRoomPhase::Busted)
	{
		Text(TEXT("You're felted."), Warm, W * 0.5f, H * 0.42f, Big, 1.4f, 1);
		if (GM->CanReload())
		{
			KeyLine(FString::Printf(TEXT("[R] Buy back in for $%lld     [L] Go home"), GM->GetReloadChips()), Paper, W * 0.5f, H * 0.42f + 58.0f * S, Small, 1.3f, 1);
			const FString Spare = FString::Printf(TEXT("%s left in the bankroll"), *Dollars(GM->GetBankrollOffTable() / 100));
			KeyLine((bPad ? TEXT("{wheel}  to change  ·  ") : TEXT("wheel to change  ·  ")) + Spare, Dim, W * 0.5f, H * 0.42f + 96.0f * S, Small, 1.05f, 1);
		}
		else
		{
			KeyLine(TEXT("[L] Go home"), Paper, W * 0.5f, H * 0.42f + 58.0f * S, Small, 1.3f, 1);
			Text(FString::Printf(TEXT("%s left. Not enough to sit."), *Dollars(GM->GetBankrollOffTable() / 100)), Dim, W * 0.5f, H * 0.42f + 96.0f * S, Small, 1.05f, 1);
		}
	}
	else if (GM->IsLeaveRequested())
	{
		Text(TEXT("Racking up after this hand"), Dim, W * 0.5f, H - 190.0f * S, Small, 1.1f, 1);
	}
	else if (const FAutoHost* Auto = FAutoHost::Of(GM))
	{
		// Update 2 (§2.11 P0a): the plan's status line, the one the core HUD gets through FillTableHud (ue-auto's). It comes
		// with the device's own keys written in (K on the keyboard; on a pad K's jobs are different buttons, from the control
		// map), so it's drawn as it is rather than through KeyText.
		const FString AutoLine = Auto->StatusLine(bPad);
		if (!AutoLine.IsEmpty())
		{
			Text(AutoLine, Paper, W * 0.5f, H - 190.0f * S, Small, 1.1f, 1);
		}
	}

	// The clock, and how tired you are; at a tournament, its panel at the top left: the event, the level and its
	// time, the blinds, the field, your place, the money.
	if (GM->IsCareer())
	{
		Text(GM->ClockLabel(), Dim, W - 48.0f * S, 36.0f * S, Small, 1.15f, 2);
		float Ty = 66.0f * S;
		if (const ss::Tournament* T = GM->GetTourney())
		{
			const ss::Level& L = T->CurrentLevel();
			const int32 Secs = FMath::FloorToInt(GM->LevelTimeLeft());
			auto N = [](int64 V) { return FText::AsNumber(V).ToString(); };
			const int32 Rank = T->HeroRank();
			const TCHAR* Nth = (Rank % 100 >= 11 && Rank % 100 <= 13) ? TEXT("th") : Rank % 10 == 1 ? TEXT("st") : Rank % 10 == 2 ? TEXT("nd") : Rank % 10 == 3 ? TEXT("rd") : TEXT("th");
			const int32 Paid = T->PaidPlaces();
			const bool bMoney = T->InTheMoney();
			const TPair<FString, FString> Rows[] = {
				{TEXT("LEVEL"), FString::Printf(TEXT("%d     %d:%02d"), T->LevelIndex + 1, Secs / 60, Secs % 60)},
				{TEXT("BLINDS"), FString::Printf(TEXT("%s / %s"), *N(L.Sb), *N(L.Bb))},
				{TEXT("ANTE"), L.Ante > 0 ? N(L.Ante) : FString(TEXT("—"))},
				{TEXT("PLAYERS"), FString::Printf(TEXT("%d / %d"), T->Remaining, T->Spec.Entrants)},
				{TEXT("AVG STACK"), N(static_cast<int64>(T->AverageStack()))},
				{TEXT("YOU"), FString::Printf(TEXT("%d%s"), Rank, Nth)},
				{bMoney ? TEXT("NEXT OUT") : TEXT("PAID"), bMoney ? TEXT("$") + N(T->PrizeFor(T->Remaining) / 100) : FString::Printf(TEXT("%d   ·   %d to go"), Paid, T->Remaining - Paid)},
			};
			const int32 NumRows = UE_ARRAY_COUNT(Rows);
			const float Px = 40.0f * S, Py = 40.0f * S, Pw = 340.0f * S, RowH = 30.0f * S;
			DrawRect(Night, Px, Py, Pw, 54.0f * S + NumRows * RowH + 10.0f * S);
			PanelBottom = Py + 54.0f * S + NumRows * RowH + 10.0f * S;
			DrawRect(Ember, Px, Py, Pw, 2.0f * S);
			const FString Event = GM->GetLiveName().IsEmpty() ? FString(TEXT("TOURNAMENT")) : GM->GetLiveName().ToUpper();
			Text(Event, Warm, Px + 18.0f * S, Py + 14.0f * S, Small, 1.0f, 0);
			float Ry = Py + 50.0f * S;
			for (int32 I = 0; I < NumRows; ++I)
			{
				DrawRect(FLinearColor(0.93f, 0.9f, 0.82f, 0.06f), Px + 18.0f * S, Ry - 4.0f * S, Pw - 36.0f * S, 1.0f * S);
				Text(Rows[I].Key, Fade(Read, 0.8f), Px + 18.0f * S, Ry, Small, 0.95f, 0);
				Text(Rows[I].Value, I == NumRows - 1 && bMoney ? FLinearColor(0.55f, 0.92f, 0.6f, 0.95f) : Paper, Px + Pw - 18.0f * S, Ry, Small, 1.1f, 2);
				Ry += RowH;
			}
		}
		if (GM->GetEnergy() < 30.0f)
		{
			Text(FString::Printf(TEXT("tired  %d%%"), FMath::RoundToInt(GM->GetEnergy())), Fade(Warm, 0.8f), W - 48.0f * S, Ty, Small, 1.0f, 2);
		}
	}
	if (GM->IsQuitPending())
	{
		KeyLine(TEXT("[L] again to walk away   ·   your stack will be blinded off"), Warm, W * 0.5f, H - 190.0f * S, Small, 1.15f, 1);
	}
	DrawLeaveHold();
	DrawOnFoot();

	// Who you're studying: their name, and who they are to you.
	if (Pawn && Pawn->Focus > 0.35f && GM->IsLive())
	{
		if (ABackRoomPlayer* Studied = Pawn->Studying.Get())
		{
			const float A = FMath::Clamp((Pawn->Focus - 0.35f) / 0.4f, 0.0f, 1.0f);
			const FString Note = GM->FaceNote(Studied->Persona.Name);
			Text(Studied->Persona.Name.ToUpper(), Fade(Paper, A), W * 0.5f, H * 0.6f, Small, 1.3f, 1);
			if (!Note.IsEmpty())
			{
				Text(Note, Fade(Warm, 0.9f * A), W * 0.5f, H * 0.6f + 32.0f * S, Small, 1.0f, 1);
			}
		}
	}

	// Focus left, when it isn't full.
	if (Pawn && (Pawn->FocusLeft < 0.995f || Pawn->Focus > 0.01f))
	{
		const float Bw = 160.0f * S, Bh = 4.0f * S;
		const float Bx = W - Bw - 48.0f * S, By = H - 60.0f * S;
		DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.35f), Bx, By, Bw, Bh);
		DrawRect(FLinearColor(Warm.R, Warm.G, Warm.B, 0.85f), Bx, By, Bw * Pawn->FocusLeft, Bh);
		Text(TEXT("FOCUS"), Dim, Bx, By - 26.0f * S, Small, 0.9f, 0);
	}

	// How to play: early on the first night (with the controls reminder on), or with the table card up (Tab or up on the
	// d-pad, held): the read book.
	const bool bTab = Pawn && Pawn->IsTableCardUp();
	if (bTab)
	{
		// The read book: what you have seen of each of them, and what the cards said it meant.
		const float Rx = 48.0f * S;
		float Ry = FMath::Max(H * 0.2f, PanelBottom + 24.0f * S);
		Text(TEXT("R E A D S"), Warm, Rx, Ry, Small, 1.15f, 0);
		Ry += 34.0f * S;
		int32 Shown = 0;
		for (ABackRoomPlayer* O : Table->GetOpponents())
		{
			for (const FBackRoomTell& T : O->Persona.Tells)
			{
				const int32 N = Table->Reads.FindRef(ABackRoomTable::ReadKey(O, static_cast<uint8>(T.Tell)));
				if (N <= 0)
				{
					continue;
				}
				const TCHAR* Means = T.Means == EBackRoomTellMeaning::Strong ? TEXT("strength") : (T.Means == EBackRoomTellMeaning::Weak ? TEXT("weakness") : TEXT("a bluff"));
				const FString Line = N >= 2 ? FString::Printf(TEXT("%s:  %s means %s"), *O->Persona.Name, *ABackRoomTable::TellPhrase(static_cast<uint8>(T.Tell)), Means)
											: FString::Printf(TEXT("%s:  %s  (%s? seen once)"), *O->Persona.Name, *ABackRoomTable::TellPhrase(static_cast<uint8>(T.Tell)), Means);
				Text(Line, N >= 2 ? Paper : Dim, Rx, Ry, Small, 1.05f, 0);
				Ry += 27.0f * S;
				++Shown;
			}
		}
		if (Shown == 0)
		{
			Text(bPad ? TEXT("Nothing yet. Study a face (the right trigger) while the hand plays out,") : TEXT("Nothing yet. Study a face (right mouse) while the hand plays out,"), Dim,
				Rx, Ry, Small, 1.0f, 0);
			Text(TEXT("then see what they turn over."), Dim, Rx, Ry + 25.0f * S, Small, 1.0f, 0);
		}
	}
	const float HintA = bTab ? 1.0f : (GM->ShowsHints() && GM->IsFirstVisit() && Table->GetHandNumber() <= 3 ? 0.7f : 0.0f);
	if (HintA > 0.0f)
	{
		const float Hx = W - 48.0f * S;
		TArray<const TCHAR*> Hints = {TEXT("{Space}  look at your cards"), TEXT("{Right mouse}  study a face"), TEXT("{Shift}  breathe, slow the heart"), TEXT("{Tab}  the table card"),
			Pawn && Pawn->IsThirdPerson() ? TEXT("{V}  your own eyes   ·   {B}  other shoulder") : TEXT("{V}  over the shoulder")};
		if (GM->IsCareer())
		{
			Hints.Add(GM->IsLive() ? TEXT("{L}  walk away (blinded off)") : TEXT("{L}  rack up and go home"));
		}
		if (GM->IsLive())
		{
			Hints.Add(TEXT("{Q}  stand up (still dealt in)"));
			Hints.Add(TEXT("{N}  next hand (once you've folded)"));
			Hints.Add(TEXT("{T}  table pace"));
		}
		// Above the action ladder when it's up.
		const float RowH = 24.0f * S;
		float Hy = FMath::Min(H - 250.0f * S, LadderTop - 16.0f * S - Hints.Num() * RowH);
		for (const TCHAR* Hint : Hints)
		{
			KeyLine(Hint, Fade(Dim, HintA), Hx, Hy, Small, 0.95f, 2);
			Hy += RowH;
		}
	}
}

void ABackRoomHUD::BeginPlay()
{
	Super::BeginPlay();
}

void ABackRoomHUD::EndPlay(const EEndPlayReason::Type Reason)
{
	// The core HUD's widget comes off the viewport.
	if (HudWidget && HudHost)
	{
		if (UGameViewportClient* V = HudHost->Viewport.Get())
		{
			V->RemoveViewportWidgetContent(HudWidget.ToSharedRef());
		}
	}
	HudWidget.Reset();
	HudHost.Reset();
	Super::EndPlay(Reason);
}

void ABackRoomHUD::DrawCoreHud()
{
	ABackRoomGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ABackRoomGameMode>() : nullptr;
	if (!GM || !HudHost)
	{
		return;
	}
	FTableHudHost& Host = *HudHost;
	Host.bCoreShown = false;
	UGameViewportClient* Viewport = GetWorld()->GetGameViewport();
	if (!Viewport || !FSlateApplication::IsInitialized() || !FSlateTextMeasurer::IsAvailable())
	{
		return; // (no Slate: the canvas HUD draws instead)
	}
	if (!HudWidget)
	{
		// As the street's HUD: never under the pointer, never focused (the pawn keeps the keys); over the view, under the
		// pause menu (z 30).
		HudWidget = SNew(SDrawListWidget);
		HudWidget->SetVisibility(EVisibility::HitTestInvisible);
		Viewport->AddViewportWidgetContent(HudWidget.ToSharedRef(), 10);
		Host.Viewport = Viewport;
	}
	FVector2D Size(1920.0, 1080.0);
	Viewport->GetViewportSize(Size);
	if (Size.X < 8.0 || Size.Y < 8.0)
	{
		return;
	}
	const uint64 C0 = FPlatformTime::Cycles64();
	const float H = 1080.0f;
	const float Scale = static_cast<float>(Size.Y) / H;
	const float W = static_cast<float>(Size.X) / Scale;
	const double Now = FPlatformTime::Seconds();
	ss::ui::TableHudInfo I;
	GM->FillTableHud(I, W);
	I.Trace.assign(Trace.GetData(), Trace.GetData() + Trace.Num());
	I.Whispers = Host.Whispers;
	if (GM->GetNightLog().empty() && I.Captions.empty())
	{
		// (Until the room says anything itself: the table's own lines.)
		I.Captions = Host.Fallback;
	}
	// The table card, held up (Tab, or up on the d-pad): modal over the HUD; the decision waits. (The clock card steps aside
	// under it: the card says the level and its clock.)
	const ABackRoomPawn* Pawn = Cast<ABackRoomPawn>(GetOwningPawn());
	const bool bCardUp = Pawn && Pawn->IsTableCardUp();
	I.CardUp = bCardUp;
	TSharedPtr<ss::ui::DrawList> List = Host.Hint.Make();
	ss::ui::Canvas C(*List, Host.Measurer, W, H, Scale);
	ss::ui::DrawTableHud(C, I, Now);
	if (bCardUp)
	{
		ss::ui::TableCardInfo Card;
		GM->FillTableCard(Card);
		ss::ui::DrawTableCard(C, Card, Now);
	}
	Host.Hint.Note(*List);
	HudWidget->SetDrawList(List);
	Host.bCoreShown = true;
	const double Ms = FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64() - C0);
	Host.BuildMs = Host.Frames == 0 ? Ms : Host.BuildMs * 0.97 + Ms * 0.03;
	Host.BuildMsPeak = FMath::Max(Host.BuildMsPeak * 0.995, Ms);
	++Host.Frames;
	if (CVarHudStats.GetValueOnGameThread() > 0 && Now - Host.StatsAt >= 5.0)
	{
		Host.StatsAt = Now;
		UE_LOG(LogTableHud, Display, TEXT("ss.Table.HudStats: %.3f ms average, %.3f ms peak (fill + layout + draw list), plus %.3f ms the seats and their projection; %d plates, %d kept clear, %d vertices, canvas %.0f x 1080"),
			Host.BuildMs, Host.BuildMsPeak, Host.UpdateMs, static_cast<int32>(I.Plates.size()), static_cast<int32>(I.KeepClear.size()), static_cast<int32>(List->Vertices.size()), W);
	}
}

void ABackRoomHUD::DrawLegacyPlates(float W, float H, float S)
{
	// P0a (next_segment_spec 6.3): the AI stack plates on the canvas HUD, laid out by core (ss::ui::LayoutPlates: by each
	// stack, never over a face, the cards or the chips), drawn in the canvas's own fonts.
	ABackRoomGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ABackRoomGameMode>() : nullptr;
	if (!GM || GM->GetPlates().empty() || S <= 0.0f)
	{
		return;
	}
	ss::ui::TableHudInfo I;
	GM->FillTableHud(I, W / S);
	if (I.Walking || I.Plates.empty())
	{
		return;
	}
	const double Now = FPlatformTime::Seconds();
	UFont* Font = GEngine->GetMediumFont();
	const FLinearColor Glass(0.02f, 0.035f, 0.07f, 0.84f);
	const FLinearColor Ash(0.92f, 0.9f, 0.86f, 1.0f);
	const FLinearColor Teal(0.15f, 0.83f, 0.76f, 1.0f);
	const FLinearColor Ice(0.64f, 0.86f, 0.92f, 1.0f);
	const FLinearColor Gold(0.96f, 0.69f, 0.29f, 1.0f);
	const FLinearColor Hot(0.95f, 0.51f, 0.18f, 1.0f);
	auto Fa = [](FLinearColor C, float A) {
		C.A *= FMath::Clamp(A, 0.0f, 1.0f);
		return C;
	};
	auto Line = [&](const FString& T, const FLinearColor& C, float X, float Y, float Size, bool bRight) {
		float Tw = 0.0f, Th = 0.0f;
		GetTextSize(T, Tw, Th, Font, Size * S);
		DrawText(T, C, bRight ? X - Tw : X, Y, Font, Size * S);
		return Tw;
	};
	const bool bDollars = I.Where == ss::ui::Venue::BackRoom;
	auto Count = [bDollars](int64 V) { return bDollars ? TEXT("$") + FText::AsNumber(V).ToString() : FText::AsNumber(V).ToString(); };
	for (const ss::ui::PlacedPlate& Pl : ss::ui::LayoutPlates(I, W / S, Now))
	{
		const ss::ui::SeatPlate& Sp = I.Plates[static_cast<size_t>(Pl.Index)];
		const float A = Pl.Alpha;
		const float X = Pl.Box.X * S, Y = Pl.Box.Y * S, Bw = Pl.Box.W * S, Bh = Pl.Box.H * S;
		DrawRect(Fa(Glass, A), X, Y, Bw, Bh);
		DrawRect(Fa(Sp.Acting ? Hot : FLinearColor(Ash.R, Ash.G, Ash.B, 0.18f), A), X, Y, Bw, (Sp.Acting ? 2.0f : 1.0f) * S);
		float Ty = Y + 6.0f * S;
		Line(Str(Sp.Name), Fa(Ash, A), X + 10.0f * S, Ty, 0.95f, false);
		if (Sp.Button)
		{
			Line(TEXT("D"), Fa(Gold, A), X + Bw - 10.0f * S, Ty, 0.95f, true);
		}
		Ty += 20.0f * S;
		if (!Sp.Label.empty() || Sp.Away)
		{
			Line(Sp.Away ? FString(TEXT("AWAY")) : Str(Sp.Label).ToUpper(), Fa(Sp.Away ? Ice : Teal, A), X + 10.0f * S, Ty, 0.75f, false);
			Ty += 18.0f * S;
		}
		// The stack, in the big unit with the other beside it (SHOWN); a glance's estimate (THE ROOM'S WAY); nothing (YOUR EYES).
		if (I.Readout != ss::ui::StackReadout::Eyes)
		{
			FString BigText, SmallText;
			FLinearColor BigCol = Ash;
			if (Sp.AllIn && Sp.Stack <= 0)
			{
				BigText = TEXT("ALL IN");
				SmallText = Sp.Bet > 0 ? Count(Sp.Bet) : FString();
				BigCol = Hot;
			}
			else if (I.Readout == ss::ui::StackReadout::Room && !Sp.Read.Exact)
			{
				const int64 Seen = Sp.Read.Shown;
				BigText = TEXT("≈ ") + (Seen >= 10000 ? FText::AsNumber((Seen + 500) / 1000).ToString() + TEXT("K") : Count(Seen));
				SmallText = I.Bb > 0 && !bDollars ? FText::AsNumber(Seen / I.Bb).ToString() + TEXT(" BB") : FString();
			}
			else
			{
				const FString Bbs = I.Bb > 0 && !bDollars ? Str(ss::chipset::InBb(Sp.Stack, I.Bb)) : FString();
				const bool bBbBig = I.Units == ss::ui::StackUnits::BigBlinds && !Bbs.IsEmpty();
				BigText = bBbBig ? Bbs : Count(Sp.Stack);
				SmallText = bBbBig ? Count(Sp.Stack) : Bbs;
				BigCol = I.Bb > 0 && !bDollars ? (Sp.Stack < 8 * I.Bb ? Hot : Sp.Stack < 15 * I.Bb ? Gold : Ash) : Ash;
				if (I.Readout == ss::ui::StackReadout::Room)
				{
					Line(TEXT("COUNTED"), Fa(Gold, A), X + Bw - 10.0f * S, Y + 26.0f * S, 0.7f, true);
				}
			}
			Line(BigText, Fa(BigCol, A), X + 10.0f * S, Ty, 1.0f, false);
			if (!SmallText.IsEmpty())
			{
				Line(SmallText, Fa(Ice, A), X + Bw - 10.0f * S, Ty + 2.0f * S, 0.8f, true);
			}
		}
		// The last action, under the plate, until the street ends.
		if (!Sp.LastAction.empty())
		{
			FString Tag = Str(Sp.LastAction);
			if (Sp.LastTo > 0 && Tag != TEXT("FOLD") && Tag != TEXT("CHECK"))
			{
				Tag += TEXT(" ") + Count(Sp.LastTo);
			}
			const FLinearColor Col = Tag.StartsWith(TEXT("FOLD")) ? FLinearColor(0.3f, 0.36f, 0.45f, 0.92f) : Tag.StartsWith(TEXT("CHECK")) ? FLinearColor(0.37f, 0.5f, 0.65f, 0.92f)
								   : Tag.StartsWith(TEXT("CALL")) ? FLinearColor(0.23f, 0.51f, 0.96f, 0.92f) : Tag.StartsWith(TEXT("ALL IN")) ? FLinearColor(0.94f, 0.3f, 0.35f, 0.92f)
																 : FLinearColor(0.95f, 0.54f, 0.23f, 0.92f);
			float Tw = 0.0f, Th = 0.0f;
			GetTextSize(Tag, Tw, Th, Font, 0.75f * S);
			const float Tx = X + Bw * 0.5f - (Tw + 16.0f * S) * 0.5f;
			DrawRect(Fa(Col, A), Tx, Y + Bh + 5.0f * S, Tw + 16.0f * S, 22.0f * S);
			DrawText(Tag, Fa(FLinearColor::White, A), Tx + 8.0f * S, Y + Bh + 7.0f * S, Font, 0.75f * S);
		}
		// An edge plate points at its seat (one on the bottom or top edge, TowardY, has no side to point to here).
		if (Pl.Edge && Pl.Toward != 0)
		{
			Line(Pl.Toward < 0 ? TEXT("‹") : TEXT("›"), Fa(Ash, A), Pl.Toward < 0 ? X - 14.0f * S : X + Bw + 4.0f * S, Y + Bh * 0.5f - 10.0f * S, 1.2f, false);
		}
	}
}

// ------------------------------------------------------------------ the HUD's data (ue-table, next_segment_spec 6)

void ABackRoomGameMode::UpdatePlates(float RealDt)
{
	if (!TableHost)
	{
		TableHost = MakeShared<FTableHost>();
	}
	FTableHost& H = *TableHost;
	const double Now = FPlatformTime::Seconds();
	H.Now = Now;
	Plates.clear();
	H.KeepClear.clear();
	H.bPotOnScreen = false;
	H.bHasClock = false;
	if (!Table || !GetWorld())
	{
		return;
	}
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	ABackRoomPawn* Pawn = PC ? Cast<ABackRoomPawn>(PC->GetPawn()) : nullptr;
	const ss::ui::GameSettings* Settings = GetSettings();
	// The feature table's view belongs to the rail on foot: anything that takes the player off it (a walk out, the night's
	// end) puts the view down too, so its camera and lower third never outlive it.
	if (bFeatureView && !(Pawn && Pawn->IsFreeWalking()))
	{
		WatchFeature(false);
	}

	// The view this frame: one projection for every point the HUD needs (the camera as it's drawn).
	H.bProjection = false;
	if (ULocalPlayer* LP = PC ? PC->GetLocalPlayer() : nullptr; LP && LP->ViewportClient && LP->ViewportClient->Viewport)
	{
		FSceneViewProjectionData Pd;
		if (LP->GetProjectionData(LP->ViewportClient->Viewport, Pd))
		{
			H.ViewProjection = Pd.ComputeViewProjectionMatrix();
			H.ViewRect = Pd.GetConstrainedViewRect();
			H.bProjection = H.ViewRect.Width() > 0 && H.ViewRect.Height() > 0;
		}
		FVector2D Size(1920.0, 1080.0);
		LP->ViewportClient->GetViewportSize(Size);
		if (Size.X > 8.0 && Size.Y > 8.0)
		{
			H.Viewport = Size;
		}
	}
	H.PixelScale = static_cast<float>(H.Viewport.Y) / 1080.0f;
	H.CanvasW = static_cast<float>(H.Viewport.X) / FMath::Max(H.PixelScale, 0.01f);
	if (PC && PC->PlayerCameraManager)
	{
		const FRotator R = PC->PlayerCameraManager->GetCameraRotation();
		H.CameraAt = PC->PlayerCameraManager->GetCameraLocation();
		H.CameraRight = FRotationMatrix(R).GetUnitAxis(EAxis::Y);
		H.CameraForward = R.Vector();
	}

	// The room's chips at the level (the colour-ups race the small ones off), and the turn cue as the player set it.
	const int32 LevelNow = bLive && Tourney ? Tourney->LevelIndex : -1;
	if (H.InPlayLevel != LevelNow)
	{
		H.InPlayLevel = LevelNow;
		if (bLive && Tourney)
		{
			H.InPlay = ss::chipset::InPlay(ss::chipset::Room::Embercrest, Tourney->Levels, Tourney->LevelIndex, Tourney->Spec.BreakEvery);
			ABackRoomChips::UseChips(true, H.InPlay);
		}
		else
		{
			H.InPlay = ss::chipset::Set(ss::chipset::Room::BackRoom);
		}
	}
	Table->TurnCue = Settings ? FMath::Clamp(Settings->TurnCue, 0, 2) : 2;

	// The night's phase, and when it changed (a note that holds counts from it).
	if (H.PhaseNow != static_cast<uint8>(Phase))
	{
		H.PhaseNow = static_cast<uint8>(Phase);
		H.PhaseAt = Now;
	}

	// The clock facts (one set for the clock card, the pause menu and the table card: live::Clock).
	if (bLive && Tourney && !Tourney->bFinished && !bCancelled)
	{
		if (H.OccurrenceFor != LiveEntryId)
		{
			H.OccurrenceFor = LiveEntryId;
			H.Occurrence = ss::live::FindOccurrence(std::string(TCHAR_TO_UTF8(*LiveEntryId)));
		}
		const ss::life::LiveEntry* E = Save.IsValid() && !bLiveOver ? ss::live::EntryFor(Save->Life, std::string(TCHAR_TO_UTF8(*LiveEntryId))) : nullptr;
		const double BreakLeft = bOnBreak ? LiveBreakMinutes * 60.0 * (1.0 - static_cast<double>(BreakT) / LiveDetail::BreakRealSeconds) : 0.0;
		H.Clock = ss::live::Clock(*Tourney, H.Occurrence, E, Minutes, LevelTimeLeft(), bOnBreak, BreakLeft);
		H.bHasClock = true;
		// The FIELD page's counts, posted at each break the way a floor posts them.
		if (bOnBreak && !H.bWasOnBreak)
		{
			std::vector<std::pair<std::string, std::string>> Known;
			for (const TPair<FString, TPair<FString, FString>>& F : LiveFaces)
			{
				Known.push_back({TCHAR_TO_UTF8(*F.Key), TCHAR_TO_UTF8(*F.Value.Key)});
			}
			H.AtBreak = ss::ui::FieldCounts(*Tourney, Known);
			H.CountsAt = ss::net::TimeLabel(Minutes);
		}
	}
	H.bWasOnBreak = bOnBreak;

	// The player's vitals for the time block and the on-foot block: the save's (what the vitals and the Penny Drop chip
	// read), with tonight's energy. A small copy, never the whole life.
	if (Save.IsValid())
	{
		const ss::life::State& L = Save->Life;
		if (!H.LifeView.IsValid())
		{
			H.LifeView = MakeShared<ss::life::State>();
			H.LifeView->Perks = L.Perks;
		}
		ss::life::State& V = *H.LifeView;
		V.Energy = FMath::Clamp(static_cast<double>(Energy), 0.0, 100.0);
		V.Hunger = L.Hunger;
		V.Thirst = L.Thirst;
		if (V.Deliveries.size() != L.Deliveries.size())
		{
			V.Deliveries = L.Deliveries;
		}
	}

	// The toast lane (spec 1.6): what arrived while the decision was the player's waits for it to be made. (Update 2: the
	// plan's decisions aren't the player's; nothing waits on them.)
	const FBackRoomPrompt Prompt = Table->GetPrompt();
	const bool bDeciding = Prompt.bYourTurn && !bHeroUp && !IsAutoPlaying();
	for (ss::ui::Toast& T : Toasts)
	{
		if (T.At < 0.0 && !bDeciding)
		{
			T.At = Now;
		}
	}

	// The sit-down (live): what the player is wearing for a sponsor, in the lane for 6 s; the one-time controls notice.
	if (bLive && Phase == EBackRoomPhase::Playing && !H.bSatDown && !(Pawn && Pawn->IsWalking()))
	{
		H.bSatDown = true;
		if (Save.IsValid())
		{
			const std::string Wearing = ss::sponsor::WearingLine(Save->Life.Closet, Save->Life.Sponsors, Minutes);
			if (!Wearing.empty())
			{
				PushToast(TEXT("Tonight's patches"), Str(Wearing));
			}
		}
		if (!H.bNoticeChecked)
		{
			H.bNoticeChecked = true;
			bool bSeen = false;
			if (GConfig)
			{
				GConfig->GetBool(TEXT("ShortStack.Table"), TEXT("ControlsNotice"), bSeen, GGameUserSettingsIni);
			}
			if (!bSeen)
			{
				const ABackRoomPlayerController* Pad = Cast<ABackRoomPlayerController>(PC);
				PushToast(TEXT("Controls"), Str(ss::ui::ControlsNotice(Pad && Pad->IsUsingGamepad())));
				if (GConfig)
				{
					GConfig->SetBool(TEXT("ShortStack.Table"), TEXT("ControlsNotice"), true, GGameUserSettingsIni);
					GConfig->Flush(false, GGameUserSettingsIni);
				}
			}
		}
	}
	// The first while at the table each night: the controls row (with the reminder on); on foot, the street's row.
	if (!H.bHintsShown && Phase == EBackRoomPhase::Playing && Pawn && !Pawn->IsWalking() && !Pawn->IsFreeWalking() && ShowsHints())
	{
		H.bHintsShown = true;
		H.HintsAt = Now;
	}
	if (!H.bFootHintsShown && Pawn && Pawn->IsFreeWalking() && ShowsHints())
	{
		H.bFootHintsShown = true;
		H.FootHintsAt = Now;
	}
	// A sponsor's bonus at the cage (the kind-11 lines settle writes): a toast as it lands in the ledger.
	if (Save.IsValid())
	{
		const std::vector<ss::life::LedgerEntry>& L = Save->Life.Ledger;
		if (H.LedgerCount < 0)
		{
			H.LedgerCount = static_cast<int32>(L.size());
			H.LedgerLastAt = L.empty() ? 0.0 : L.front().At;
		}
		else if (!L.empty() && L.front().At > H.LedgerLastAt)
		{
			for (const ss::life::LedgerEntry& Entry : L)
			{
				if (Entry.At <= H.LedgerLastAt)
				{
					break;
				}
				FString From, Line;
				if (BonusToast(Entry, From, Line))
				{
					PushToast(From, Line, 0xf6b04a);
				}
			}
			H.LedgerLastAt = L.front().At;
			H.LedgerCount = static_cast<int32>(L.size());
		}
	}

	// The seats as the felt shows them.
	Table->GetSeatViews(H.Seats);
	const bool bWalkingNow = Pawn && Pawn->IsWalking();
	const bool bOnFoot = Pawn && Pawn->IsFreeWalking();
	const FVector TableAt(0.0, 0.0, ABackRoomStage::FeltZ);
	const bool bNearTable = Pawn && FVector::Dist(Pawn->GetEye(), TableAt) < FootPlatesWithin;
	// Plates: seated at the table and playing (or out and still in the chair), on foot near the table (the rail's view of
	// it), and on the feature table's view; never on a scripted walk (in, out, a move), and not before cards.
	const bool bShowPlates = (bFeatureView || (!bWalkingNow && Phase != EBackRoomPhase::Moving && Phase != EBackRoomPhase::Arriving && (!bOnFoot || bNearTable)));
	ABackRoomPlayer* Studied = Pawn && Pawn->Focus > 0.35f ? Pawn->Studying.Get() : nullptr;
	for (const FBackRoomSeatView& V : H.Seats)
	{
		// THE ROOM'S WAY: a long look counts the stack (1.5 s of Focus on that face), as long as it's the stack it was.
		float& Look = H.StudyT.FindOrAdd(V.TableSeat);
		Look = V.Player && V.Player == Studied ? Look + RealDt : 0.0f;
		if (Look >= CountSeconds)
		{
			H.CountedStack.Add(V.TableSeat, V.Stack);
		}
		if (!bShowPlates || (!V.bHero && !V.Player) || (!V.bHero && V.Stack <= 0 && !V.bInHand))
		{
			continue;
		}
		ss::ui::SeatPlate P;
		P.Seat = V.HandSeat >= 0 ? V.HandSeat : V.TableSeat;
		P.Hero = V.bHero;
		// The name the room knows them by; who they are to the player; one badge glyph (Room scope).
		FString Raw = V.Player ? V.Player->Persona.Name : V.Id;
		if (Tourney && !V.bHero)
		{
			const int32 Idx = Tourney->PlayerIndex(std::string(TCHAR_TO_UTF8(*V.Id)));
			Raw = Idx >= 0 ? Str(Tourney->Players[static_cast<size_t>(Idx)].Name) : Raw;
		}
		P.Name = V.bHero ? std::string("You") : Utf8(V.Name);
		if (bLive && !V.bHero)
		{
			P.Label = Utf8(FaceNote(Raw));
			// Held by name first: Top points into the set, and a temporary set is gone before the glyph could be copied.
			const ss::badge::Set Held = FaceBadges(Raw);
			if (const ss::badge::Badge* B = ss::badge::Top(Held, ss::badge::Room, ss::badge::FrameNone))
			{
				P.Glyph = *B;
			}
		}
		P.Stack = V.Stack;
		P.Bet = V.Bet;
		const int64* Counted = H.CountedStack.Find(V.TableSeat);
		P.Read = ss::chipset::ReadStack(V.Stack, H.InPlay, V.bCounted || (Counted && *Counted == V.Stack));
		P.Folded = V.bFolded;
		P.AllIn = V.bAllIn;
		P.Acting = V.bActing;
		P.Button = V.bButton;
		P.Away = V.bAway;
		P.LastAction = Utf8(V.LastAction);
		P.LastTo = V.LastTo;
		P.ActedAt = V.ActedAt;
		// The seat the player is looking at (Focus on it, or the face in the middle of the view): its plate at full strength.
		P.Looked = V.Player && (V.Player == Studied || FVector::DotProduct(H.CameraForward, (V.Player->GetEyes() - H.CameraAt).GetSafeNormal()) > LookedCos);
		P.Alpha = V.Player && V.Player->IsHidden() ? 0.0f : 1.0f;
		// At the stack, on the canvas.
		ss::ui::Vec2 At;
		const bool bFront = Project(H, V.StackAt + FVector(0.0, 0.0, 3.0), At);
		P.OnScreen = bFront && OnCanvas(H, At);
		if (!bFront)
		{
			// Behind the camera: an edge plate on the side the seat is on, at the canvas's middle (the canvas is 1,080 units
			// tall at every window size, as Project's points are; the layout spreads plates that share it).
			const double Side = FVector::DotProduct(V.StackAt - H.CameraAt, H.CameraRight);
			At.X = Side < 0.0 ? 0.0f : H.CanvasW;
			At.Y = 0.5f * 1080.0f;
		}
		P.X = At.X;
		P.Y = At.Y;
		if (bOnFoot && !bFeatureView && !P.OnScreen && !P.Hero)
		{
			continue; // (on foot, only the plates in view)
		}
		Plates.push_back(P);
	}
	// The feature table from the rail while the room stands round another table: the lower third's rows from the engine's
	// final table, and no plate on the felt (the table the room follows isn't the stage's).
	if (bFeatureView && Tourney && Tourney->Tables.size() == 1 && Stage && Stage->GetRoomAnchor() != ABackRoomStage::StreamSlot())
	{
		Plates.clear();
		const std::vector<int>& Chairs = Tourney->Tables.begin()->second.Seats;
		for (int32 K = 0; K < static_cast<int32>(Chairs.size()); ++K)
		{
			const int32 Idx = Chairs[static_cast<size_t>(K)];
			if (Idx < 0 || Idx >= static_cast<int32>(Tourney->Players.size()))
			{
				continue;
			}
			const ss::TPlayer& TP = Tourney->Players[static_cast<size_t>(Idx)];
			ss::ui::SeatPlate P;
			P.Seat = K;
			P.Name = ss::talk::CaptionName(TP.Id, TP.Name, K);
			P.Stack = TP.Stack;
			P.Hero = true; // (a row of the lower third, never a plate on the felt)
			P.OnScreen = false;
			Plates.push_back(P);
		}
	}
	// What the plates keep clear: faces, cards and chips on screen (over the shoulder, the player's own head too).
	const bool bOwnHead = Pawn && Pawn->GetThirdBlend() > 0.3f;
	for (const FBackRoomSeatView& V : H.Seats)
	{
		ss::ui::Rect R;
		if (V.Player && (!V.bHero || bOwnHead) && !V.Player->IsHidden() && ProjectBlob(H, V.Player->GetEyes() + FVector(0.0, 0.0, -4.0), V.bHero ? 16.0 : 12.0, 1.3f, 1.6f, R))
		{
			H.KeepClear.push_back(R);
		}
		if (V.bHasCards && !V.bHero && ProjectBlob(H, V.CardsAt, 7.0, 0.8f, 0.8f, R))
		{
			H.KeepClear.push_back(R);
		}
		if (V.StackPile && V.Stack > 0 && ProjectBox(H, V.StackPile->GetComponentsBoundingBox(true), R))
		{
			H.KeepClear.push_back(R);
		}
		if (V.BetPile && V.Bet > 0 && ProjectBox(H, V.BetPile->GetComponentsBoundingBox(true), R))
		{
			H.KeepClear.push_back(R);
		}
	}
	// The player's own cards and the board (every card on the felt stays readable), and the dealer's face.
	Table->GetHeroCards(H.Cards);
	Table->GetBoardCards(H.Cards);
	for (const ABackRoomCard* Card : H.Cards)
	{
		ss::ui::Rect R;
		if (Card && !Card->IsHidden() && ProjectBlob(H, Card->GetActorLocation(), 6.0, 1.0f, 1.0f, R))
		{
			H.KeepClear.push_back(R);
		}
	}
	if (const ABackRoomPlayer* Dealing = Table->GetDealer(); Dealing && !Dealing->IsHidden())
	{
		ss::ui::Rect R;
		if (ProjectBlob(H, Dealing->GetEyes() + FVector(0.0, 0.0, -4.0), 12.0, 1.3f, 1.6f, R))
		{
			H.KeepClear.push_back(R);
		}
	}
	// The pot's tag, at the pot.
	{
		ss::ui::Vec2 At;
		if (Project(H, Table->GetPotSpot() + FVector(0.0, 0.0, 1.0), At) && OnCanvas(H, At, -40.0f))
		{
			H.PotAt = At;
			H.bPotOnScreen = true;
		}
		if (const ABackRoomChips* PotPile = Table->GetPotPile(); PotPile && PotPile->GetAmount() > 0)
		{
			ss::ui::Rect R;
			if (ProjectBox(H, PotPile->GetComponentsBoundingBox(true), R))
			{
				H.KeepClear.push_back(R);
			}
		}
	}
}

void ABackRoomGameMode::PlateAudit(float RealDt)
{
	if (!Table || !TableHost)
	{
		return;
	}
	FTableHost& H = *TableHost;
	// (The piles a hand's end had to put right are drained either way: they're this hand's, not a later one's.)
	const TArray<FBackRoomPileFix> Fixes = Table->TakePileFixes();
	const int32 Mode = CVarPlateAudit.GetValueOnGameThread();
	if (Mode <= 0)
	{
		PlateMismatchFor.Reset();
		H.AuditLogged.Reset();
		return;
	}
	for (const FBackRoomPileFix& F : Fixes)
	{
		++H.AuditFixes;
		UE_LOG(LogTableHud, Warning, TEXT("PlateAudit: hand %d, chair %d (%s): the felt showed %lld and the engine had %lld; the hand's end put it right (a plate that lied)"),
			F.HandNumber, F.TableSeat, *F.Id, F.Felt, F.Engine);
	}
	// Every seat with chips or a hand: its plate (the felt) against the engine's stack for it as of the hand's events. A
	// difference while chips are on their way is the animation; one that outlasts them is a lie, logged once.
	int32 Seats = 0;
	for (const FBackRoomSeatView& V : H.Seats)
	{
		if (!V.Player && !V.bHero)
		{
			continue;
		}
		++Seats;
		++H.AuditChecks;
		float& T = PlateMismatchFor.FindOrAdd(V.TableSeat);
		if (V.Stack == V.EngineStack)
		{
			if (H.AuditLogged.Contains(V.TableSeat))
			{
				UE_LOG(LogTableHud, Warning, TEXT("PlateAudit: chair %d (%s) agrees again after %.1f s"), V.TableSeat, *V.Name, T);
				H.AuditLogged.Remove(V.TableSeat);
			}
			T = 0.0f;
			continue;
		}
		T += V.bChipsMoving ? 0.0f : RealDt;
		if (T >= AuditGrace && !H.AuditLogged.Contains(V.TableSeat))
		{
			H.AuditLogged.Add(V.TableSeat);
			++H.AuditMismatches;
			UE_LOG(LogTableHud, Warning, TEXT("PlateAudit: hand %d, chair %d (%s): the plate shows %lld, the engine has %lld (%lld now), %.1f s past the chips' landing"),
				Table->GetHandNumber(), V.TableSeat, *V.Name, V.Stack, V.EngineStack, V.EngineNow, T);
		}
	}
	// A line a hand, so a quiet audit is seen to be running.
	if (Table->GetHandNumber() != H.AuditHand)
	{
		if (H.AuditHand >= 0)
		{
			UE_LOG(LogTableHud, Display, TEXT("PlateAudit: hand %d done: %d seats, %lld checks tonight, %d mismatches, %d piles put right at a hand's end"), H.AuditHand, Seats,
				H.AuditChecks, H.AuditMismatches, H.AuditFixes);
			if (Mode >= 2)
			{
				for (const ss::ui::SeatPlate& P : Plates)
				{
					UE_LOG(LogTableHud, Display, TEXT("PlateAudit:   seat %d %s: %lld (%s)%s%s"), P.Seat + 1, *Str(P.Name), static_cast<int64>(P.Stack), *Str(P.LastAction),
						P.OnScreen ? TEXT("") : TEXT(" off screen"), P.Hero ? TEXT(" (you)") : TEXT(""));
				}
			}
		}
		H.AuditHand = Table->GetHandNumber();
	}
}

void ABackRoomGameMode::FillTableHud(ss::ui::TableHudInfo& Out, float CanvasWidth) const
{
	if (!Table || !TableHost || !GetWorld())
	{
		return;
	}
	(void)CanvasWidth;
	const FTableHost& H = *TableHost;
	const double Now = H.Now;
	const ss::ui::GameSettings* Settings = GetSettings();
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const ABackRoomPawn* Pawn = PC ? Cast<ABackRoomPawn>(PC->GetPawn()) : nullptr;
	const ABackRoomPlayerController* Pad = Cast<ABackRoomPlayerController>(PC);
	const bool bPad = Pad && Pad->IsUsingGamepad();
	const FBackRoomPrompt P = Table->GetPrompt();
	const bool bInWalk = Pawn && Pawn->IsWalking();
	const bool bOnFoot = Pawn && Pawn->IsFreeWalking();
	// On foot (and on the scripted walks, and while the night's summary is up): the street's block, not the hero's.
	const bool bWalking = !bFeatureView && (bOnFoot || bInWalk || Phase == EBackRoomPhase::Moving || Phase == EBackRoomPhase::Leaving || Phase == EBackRoomPhase::Arriving);

	Out.Where = bLive ? ss::ui::Venue::CardRoom : ss::ui::Venue::BackRoom;
	Out.Clock = H.bHasClock ? &H.Clock : nullptr;
	Out.Event = bLive ? Utf8(EventShort(LiveName)) : std::string();
	Out.HeroStack = P.Stack;
	Out.Pot = P.Pot;
	Out.Bb = P.BigBlind;
	Out.PotAt = H.PotAt;
	Out.PotOnScreen = H.bPotOnScreen && P.Pot > 0 && !bWalking;
	// The decision: the ladder and its presets (keys 1-6, the bumpers), the all-in hold's fill. Never while the plan holds
	// the seat (update 2, §2.11): its turn isn't the player's, so the ladder stays down and the toasts aren't held.
	const bool bDecide = P.bYourTurn && Phase != EBackRoomPhase::Busted && Phase != EBackRoomPhase::Moving && !bHeroUp && !bWalking && !(Pawn && Pawn->IsTableCardUp()) &&
		!IsAutoPlaying();
	Out.YourTurn = bDecide;
	Out.CanCheck = P.bCanCheck;
	Out.CanRaise = P.bCanRaise;
	Out.ToCall = P.ToCall;
	Out.RaiseTo = P.RaiseTo;
	Out.AllInHold = Pawn ? Pawn->GetAllInHold() : 0.0f;
	int32 PresetNow = -1;
	if (Table->GetHeroSizing(Out.Spot, Out.Presets, PresetNow))
	{
		Out.Preset = PresetNow;
	}
	Out.Plates = Plates;
	Out.Readout = Settings ? static_cast<ss::ui::StackReadout>(FMath::Clamp(Settings->StackReadout, 0, 2)) : ss::ui::StackReadout::Shown;
	Out.Units = Settings ? static_cast<ss::ui::StackUnits>(FMath::Clamp(Settings->StackUnits, 0, 1)) : ss::ui::StackUnits::BigBlinds;
	// The heart and the eyes.
	Out.Bpm = Table->Composure.Bpm;
	Out.Focus = Pawn ? Pawn->FocusLeft : 1.0f;
	Out.Studying = Pawn && Pawn->Focus > 0.05f;
	Out.Eyelids = GetEyelids() * (1.0f - (Pawn ? Pawn->GetThirdBlend() : 0.0f));
	Out.Steadying = Pawn && Pawn->bSteadying;
	// The room's captions (the director's, ue-room's: one source), as the player set them.
	Out.Captions = GetCaptions();
	if (Settings)
	{
		Out.CaptionLook.Mode = FMath::Clamp(Settings->Captions, 0, 2);
		Out.CaptionLook.Size = FMath::Clamp(Settings->CaptionSize, 80, 200);
		Out.CaptionLook.Backdrop = FMath::Clamp(Settings->CaptionBackdrop, 0, 100);
		Out.CaptionLook.Names = Settings->SpeakerNames;
	}
	// The lane: what's arrived and been released (never during the decision).
	for (const ss::ui::Toast& T : Toasts)
	{
		if (T.At >= 0.0)
		{
			Out.Toasts.push_back(T);
		}
	}
	// Who's being studied: the name, who they are to the player, their badges, the line under them.
	if (Pawn && Pawn->Focus > 0.35f)
	{
		if (const ABackRoomPlayer* St = Pawn->Studying.Get())
		{
			FString Raw = St->Persona.Name;
			FString Shown = Raw;
			for (const FBackRoomSeatView& V : H.Seats)
			{
				if (V.Player == St)
				{
					Shown = V.Name;
					if (Tourney)
					{
						const int32 Idx = Tourney->PlayerIndex(std::string(TCHAR_TO_UTF8(*V.Id)));
						Raw = Idx >= 0 ? Str(Tourney->Players[static_cast<size_t>(Idx)].Name) : Raw;
					}
				}
			}
			Out.Study.Name = Utf8(Shown);
			Out.Study.Alpha = FMath::Clamp((Pawn->Focus - 0.35f) / 0.4f, 0.0f, 1.0f);
			if (bLive)
			{
				Out.Study.Label = Utf8(FaceNote(Raw));
				const ss::badge::Set Held = FaceBadges(Raw);
				std::vector<ss::badge::Badge> Row = ss::badge::InOrder(Held, ss::badge::Room, ss::badge::FrameNone);
				Row.resize(FMath::Min<size_t>(Row.size(), 3));
				Out.Study.Badges = Row;
				Out.Study.Honorific = ss::badge::Caption(Held, ss::badge::Room);
			}
		}
	}
	// The banner.
	float Shown = 0.0f;
	const FString& B = GetBanner(Shown);
	if (!B.IsEmpty())
	{
		Out.Banner = Utf8(B);
		Out.BannerAt = Now - Shown;
	}
	// The time and the vitals (seated: the top right; on foot: the street's block, with the room's prompt).
	ss::ui::StreetHudInfo& Foot = Out.OnFoot;
	Foot.Place = bLive ? "THE EMBERCREST \xC2\xB7 CARD ROOM" : "THE BACK ROOM \xC2\xB7 WASH & FOLD";
	Foot.Clock = IsCareer() ? Utf8(ClockLabel()) : std::string();
	Foot.World = IsCareer() ? Minutes : -1.0;
	Foot.BankrollCents = Save.IsValid() ? (bLive ? Save->BankrollCents : BaseCents) : 0;
	Foot.Life = H.LifeView.Get();
	Foot.FirstPerson = true;
	Foot.Gamepad = bPad;
	Foot.CaptionLook = Out.CaptionLook;
	// (The desk's screen, ue-venue's, has the player's attention while it's up: no prompt, marker or plates under it.)
	const bool bDesk = IsDeskOpen();
	if (bDesk)
	{
		Out.Plates.clear();
	}
	if (bOnFoot && bLive && !bDesk)
	{
		SplitPrompt(SpotPrompt(), Foot.PromptKey, Foot.Prompt);
	}
	Out.Walking = bWalking;
	// On foot and still dealt in: where the chair is.
	if (bOnFoot && bLive && !bLiveOver && !bRailing && Tourney && !bDesk)
	{
		// (World space: the venue's target, else the stage's own point over the chair; SeatTransform alone is table-local.)
		const FVector Marked = SeatMarkerTarget();
		const FVector Target = !Marked.IsZero() ? Marked : Stage ? Stage->HeroSeatMarker() : FVector::ZeroVector;
		ss::ui::Vec2 At;
		const bool bFront = Project(H, Target, At);
		Out.SeatMarker.Text = "TABLE " + std::to_string(Tourney->Hero().TableId) + " \xC2\xB7 SEAT " + std::to_string(ss::live::SeatLabel(Tourney->Hero().Seat));
		Out.SeatMarker.Sub = H.bHasClock && H.Clock.OnBreak ? "BREAK " + Utf8(FString::Printf(TEXT("%d:%02d"), static_cast<int32>(H.Clock.BreakSecondsLeft) / 60, static_cast<int32>(H.Clock.BreakSecondsLeft) % 60))
														  : std::string("DEALT IN WHILE YOU'RE UP");
		if (!bFront)
		{
			const double Side = FVector::DotProduct(Target - H.CameraAt, H.CameraRight);
			At.X = Side < 0.0 ? 0.0f : H.CanvasW;
			At.Y = 0.5f * 1080.0f; // the canvas's middle (1,080 units tall at every window size)
		}
		Out.SeatMarker.X = At.X;
		Out.SeatMarker.Y = At.Y;
		Out.SeatMarker.OnScreen = bFront && OnCanvas(H, At);
		Out.SeatMarker.Metres = Pawn ? static_cast<float>(FVector::Dist2D(Pawn->GetEye(), Target) / 100.0) : 0.0f;
	}
	// The last hand the player played (the card over the hero block for 6 s).
	if (!RecentHands.empty())
	{
		Out.LastHand = &RecentHands.back();
		Out.LastHandAt = H.LastHandAt;
	}
	Out.Gamepad = bPad;
	Out.HintsAt = bOnFoot ? H.FootHintsAt : bWalking ? -100.0 : H.HintsAt;
	Out.OnAir = bFeatureView;
	Out.Scale = Settings ? FMath::Clamp(Settings->HudScale, 80, 130) / 100.0f : 1.0f;
	Out.KeepClear = H.KeepClear;
	for (const int32 Card : P.Known)
	{
		Out.HeroCards.push_back(static_cast<ss::Card>(Card));
	}
	Out.InHand = Table->IsHeroInHand();
	Out.Pace = bLive && Phase == EBackRoomPhase::Playing && !bHeroUp && !bWalking ? TablePace : -1;
	Out.Skipping = Pawn && Pawn->IsSkippingHand();
	// Walking in: where, and when.
	const float Arrive = GetArrivalTime();
	if (Arrive >= 0.0f)
	{
		Out.Arrive.Title = bLive ? "EMBERCREST CASINO" : "WASH & FOLD";
		Out.Arrive.Sub = bLive ? "the card room \xC2\xB7 " + Utf8(LiveName.ToLower()) : std::string("the back room");
		Out.Arrive.Day = Utf8(ArrivalDay());
		Out.Arrive.At = Now - Arrive;
	}
	// A note across the middle: felted (it holds, with its choices), the view (V), the pace (T).
	if (Phase == EBackRoomPhase::Busted)
	{
		Out.Note = "You're felted.";
		Out.NoteHolds = true;
		Out.NoteAt = H.PhaseAt;
		if (CanReload())
		{
			Out.Choices.push_back({"raise", Utf8(FString::Printf(TEXT("Buy back in for $%lld"), GetReloadChips()))});
			Out.Choices.push_back({"leave", "Go home"});
			Out.NoteSub = Utf8(FString::Printf(TEXT("%s · %s left in the bankroll"), bPad ? TEXT("LB and RB change it") : TEXT("The wheel changes it"), *Dollars(GetBankrollOffTable() / 100)));
		}
		else
		{
			Out.Choices.push_back({"leave", "Go home"});
			Out.NoteSub = Utf8(FString::Printf(TEXT("%s left. Not enough to sit."), *Dollars(GetBankrollOffTable() / 100)));
		}
	}
	else if (Pawn && Pawn->ViewShownAge() < 2.4f)
	{
		const bool bOver = Pawn->IsThirdPerson();
		Out.Note = bOver ? "OVER THE SHOULDER" : "FIRST PERSON";
		Out.NoteSub = bOver ? (bOnFoot ? "Back at the table. Peeks and Focus are still through your own eyes."
									   : bPad ? "R3 for the other shoulder. Peeks and Focus are still through your own eyes."
											  : "B for the other shoulder. Peeks and Focus are still through your own eyes.")
							: (bPad ? "VIEW to go over the shoulder." : "V to go over the shoulder.");
		Out.NoteAt = Now - Pawn->ViewShownAge();
	}
	else if (bLive && PaceShownAge() < 2.6f)
	{
		static const char* const Paces[3] = {"TABLE PACE \xC2\xB7 LIVE", "TABLE PACE \xC2\xB7 BRISK", "TABLE PACE \xC2\xB7 FAST"};
		static const char* const Subs[3] = {"The room's own time. Your own decisions wait for you, unless you've handed the seat to autoplay.",
			"Quicker decisions around you. Yours wait for you, unless you've handed the seat to autoplay.",
			"The room acts at once. Your own decisions wait for you, unless you've handed the seat to autoplay."};
		const int32 Pace = FMath::Clamp(TablePace, 0, 2);
		Out.Note = Paces[Pace];
		Out.NoteSub = Subs[Pace];
		Out.NoteAt = Now - PaceShownAge();
	}
	if (Phase == EBackRoomPhase::Leaving && StayOffer() >= 1.0f)
	{
		// Out of it: stay on the rail, or go home now.
		Out.Choices.push_back({"stand", "Stay and watch"});
		Out.Choices.push_back({"leave", "Go home now"});
	}
	// A line over the bottom, and a hold in progress (down on the d-pad: walk away, rack up, go home).
	if (IsQuitPending())
	{
		Out.Status = bPad ? "Hold DOWN again to walk away \xC2\xB7 your stack will be blinded off" : "L again to walk away \xC2\xB7 your stack will be blinded off";
	}
	else if (IsLeaveRequested() && Phase != EBackRoomPhase::Busted && Phase != EBackRoomPhase::Leaving)
	{
		Out.Status = "Racking up after this hand";
	}
	const float Held = bPad && Pad && !bOnFoot && !bInWalk ? Pad->HoldProgress(EKeys::Gamepad_DPad_Down) : 0.0f;
	const bool bOut = Phase == EBackRoomPhase::Leaving && StayOffer() >= 0.0f;
	if (Held > 0.0f && IsCareer() && !IsLeaveRequested() && (Phase == EBackRoomPhase::Playing || Phase == EBackRoomPhase::Busted || bOut))
	{
		Out.HoldLabel = bOut ? "Go home now"
				  : Phase == EBackRoomPhase::Busted ? "Go home"
				  : bLive ? (IsQuitPending() ? "Walk away: your stack is blinded off" : "Walk away")
						  : "Rack up after this hand";
		Out.HoldProgress = Held;
	}
	// On foot: how the night stands while the player's up.
	if (bOnFoot && bLive && !bDesk)
	{
		const ss::Tournament* T = Tourney.Get();
		if (bRailing)
		{
			const FString Next = GetNextEventNote();
			if (!Next.IsEmpty())
			{
				Out.FootNote = Utf8(T && !T->bFinished ? FString::Printf(TEXT("%s · on the rail, %d left"), *Next, T->Remaining) : Next);
			}
			else
			{
				Out.FootNote = Utf8(T && T->bFinished ? FString(TEXT("It's over. The door's the way home.")) : FString::Printf(TEXT("On the rail · %d left · the door's the way home"), T ? T->Remaining : 0));
			}
		}
		else if (T && !bLiveOver)
		{
			Out.FootNote = Utf8(FString::Printf(TEXT("Up from table %d · still dealt in: checked when it's free, mucked when it isn't"), T->Hero().TableId));
		}
	}
	// The night, totted up, over the walk out.
	if (Phase == EBackRoomPhase::Leaving)
	{
		const float T = GetLeaveTime();
		for (const FString& L : GetSummary())
		{
			Out.Summary.push_back(Utf8(L));
		}
		Out.SummaryAlpha = Smooth(1.8f, 2.8f, T) * (1.0f - Smooth(29.0f, 30.0f, T));
		Out.Net = GetNetCents();
	}
	// The feature table's lower third: the next place out pays.
	if (bFeatureView && Tourney && Tourney->InTheMoney())
	{
		Out.NextOutCents = Tourney->PrizeFor(Tourney->Remaining);
	}
	// Update 2 (§2.11): the plan's fields and its P0a lines over all of the above (ue-auto's FAutoHost::FillHud).
	if (AutoHost)
	{
		AutoHost->FillHud(Out, P);
	}
}

void ABackRoomGameMode::FillTableCard(ss::ui::TableCardInfo& Out) const
{
	const FTableHost* H = TableHost.Get();
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	const ABackRoomPawn* Pawn = PC ? Cast<ABackRoomPawn>(PC->GetPawn()) : nullptr;
	const ABackRoomPlayerController* Pad = Cast<ABackRoomPlayerController>(PC);
	const ss::ui::GameSettings* Settings = GetSettings();
	Out.Page = Pawn ? FMath::Clamp(Pawn->GetTableCardPage(), 0, static_cast<int32>(ss::ui::TableCardPages().size()) - 1) : 0;
	Out.Gamepad = Pad && Pad->IsUsingGamepad();
	Out.Units = Settings ? static_cast<ss::ui::StackUnits>(FMath::Clamp(Settings->StackUnits, 0, 1)) : ss::ui::StackUnits::BigBlinds;
	Out.Where = bLive ? ss::ui::Venue::CardRoom : ss::ui::Venue::BackRoom;
	Out.HeroName = "You";
	// READS: what the player has seen of each of them, and what the cards said it meant.
	if (Table)
	{
		for (ABackRoomPlayer* O : Table->GetOpponents())
		{
			for (const FBackRoomTell& T : O->Persona.Tells)
			{
				const int32 N = Table->Reads.FindRef(ABackRoomTable::ReadKey(O, static_cast<uint8>(T.Tell)));
				if (N <= 0)
				{
					continue;
				}
				const TCHAR* Means = T.Means == EBackRoomTellMeaning::Strong ? TEXT("strength") : (T.Means == EBackRoomTellMeaning::Weak ? TEXT("weakness") : TEXT("a bluff"));
				Out.Reads.push_back(Utf8(N >= 2 ? FString::Printf(TEXT("%s: %s means %s"), *O->Persona.Name, *ABackRoomTable::TellPhrase(static_cast<uint8>(T.Tell)), Means)
												: FString::Printf(TEXT("%s: %s (%s? seen once)"), *O->Persona.Name, *ABackRoomTable::TellPhrase(static_cast<uint8>(T.Tell)), Means)));
			}
		}
		Out.HeroStack = Table->GetHeroStack();
	}
	// HANDS: the last ten, newest first.
	for (auto It = RecentHands.rbegin(); It != RecentHands.rend(); ++It)
	{
		Out.Hands.push_back(*It);
	}
	// PAYOUTS and FIELD: the clock's facts, the ladder, the people the player knows and their counts at the last break.
	if (H && H->bHasClock && Tourney)
	{
		Out.Clock = &H->Clock;
		Out.Pay = ss::live::PayLadder(*Tourney);
		Out.HeroPlace = !Tourney->Hero().Busted && !Tourney->bFinished ? Tourney->Remaining : 0;
		std::vector<std::pair<std::string, std::string>> Known;
		for (const TPair<FString, TPair<FString, FString>>& F : LiveFaces)
		{
			Known.push_back({TCHAR_TO_UTF8(*F.Key), TCHAR_TO_UTF8(*F.Value.Key)});
		}
		Out.Field = ss::ui::FieldNow(*Tourney, Known, H->AtBreak);
		Out.CountsAt = H->CountsAt;
	}
	// NIGHT: everything the room said tonight (the director's log, ue-room's).
	Out.Night = GetNightLog();
	Out.NightTitle = bLive ? "THE NIGHT \xC2\xB7 " + Utf8(LiveName.ToUpper()) : std::string("THE NIGHT \xC2\xB7 THE BACK ROOM");
	// PLAY (update 2, §2.6.2): the styles, the plan, Sharpness and Form, the skip's targets (ue-auto's FAutoHost::FillCard).
	if (AutoHost)
	{
		AutoHost->FillCard(Out);
	}
}

void ABackRoomGameMode::PushToast(const FString& From, const FString& Line, uint32 Accent)
{
	ss::ui::Toast T;
	T.From = TCHAR_TO_UTF8(*From);
	T.Line = TCHAR_TO_UTF8(*Line);
	T.Accent = Accent;
	// Released (stamped) by UpdatePlates once it isn't the player's decision: nothing lands on screen while the action is
	// on them (spec 1.6).
	T.At = -1.0;
	Toasts.push_back(T);
	// The lane shows a few; the rest have been read or missed.
	if (Toasts.size() > 8)
	{
		Toasts.erase(Toasts.begin(), Toasts.begin() + static_cast<std::ptrdiff_t>(Toasts.size() - 8));
	}
}

void ABackRoomGameMode::WatchFeature(bool bOn)
{
	// The stage's rail (spec 6.5): the stream table from a fixed camera, with the ON AIR lower third. Only once the final
	// table plays there (the room's anchor on the stage, the table the room follows), and only for someone on the rail.
	ABackRoomPawn* Pawn = Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	const bool bCan = bOn && Pawn && Stage && Tourney && bRailing && !Tourney->bFinished && Tourney->Tables.size() == 1;
	if (bOn && !bCan)
	{
		UE_LOG(LogTableHud, Log, TEXT("The feature table's view: only from the rail once the final table plays on the stage"));
	}
	bFeatureView = bCan;
	if (!Pawn)
	{
		return;
	}
	if (bFeatureView)
	{
		// High on the rail's side of the stage's table, looking down across it: the broadcast's wide shot.
		const FVector Center = Stage->TableSlot(ABackRoomStage::StreamSlot()).GetLocation() + FVector(0.0, 0.0, ABackRoomStage::FeltZ);
		FVector Away = (Pawn->GetEye() - Center).GetSafeNormal2D();
		Away = Away.IsNearlyZero() ? FVector(-1.0, 0.0, 0.0) : Away;
		Pawn->TestExtCam(Center + Away * 320.0 + FVector(0.0, 0.0, 190.0), Center + FVector(0.0, 0.0, -10.0), 58.0f, true);
	}
	else
	{
		Pawn->TestExtCam(FVector::ZeroVector, FVector::ForwardVector, 72.0f, false);
	}
}

// ------------------------------------------------------------------ console

namespace BackRoomHudDetail
{
ABackRoomGameMode* TableModeNow()
{
	if (!GEngine)
	{
		return nullptr;
	}
	for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
	{
		UWorld* W = Ctx.World();
		if (W && (Ctx.WorldType == EWorldType::PIE || Ctx.WorldType == EWorldType::Game))
		{
			if (ABackRoomGameMode* GM = W->GetAuthGameMode<ABackRoomGameMode>())
			{
				return GM;
			}
		}
	}
	return nullptr;
}

/** ss.Table.Plates: every plate as the HUD has it now, against the engine. */
FAutoConsoleCommand PlatesCmd(TEXT("ss.Table.Plates"), TEXT("ss.Table.Plates: the AI stack plates as the HUD has them now (seat, name, label, the felt's stack, the engine's, BB, last action, on screen)."),
	FConsoleCommandDelegate::CreateLambda([]() {
		ABackRoomGameMode* GM = TableModeNow();
		if (!GM)
		{
			UE_LOG(LogTableHud, Warning, TEXT("ss.Table.Plates: not at a card table"));
			return;
		}
		const std::vector<ss::ui::SeatPlate>& Ps = GM->GetPlates();
		const int64 Bb = GM->Table ? GM->Table->BigBlindNow() : 0;
		UE_LOG(LogTableHud, Display, TEXT("ss.Table.Plates: %d plates, big blind %lld, %s"), static_cast<int32>(Ps.size()), Bb, GM->Table ? *GM->Table->Describe() : TEXT(""));
		for (const ss::ui::SeatPlate& P : Ps)
		{
			UE_LOG(LogTableHud, Display, TEXT("  seat %d  %-14s  %-20s  %10lld  %8s  %-10s %s%s%s%s at (%.0f, %.0f)%s"), P.Seat + 1, *Str(P.Name), *Str(P.Label), static_cast<int64>(P.Stack),
				*Str(ss::chipset::InBb(P.Stack, Bb)), *Str(P.LastAction), P.Hero ? TEXT("you ") : TEXT(""), P.Folded ? TEXT("folded ") : TEXT(""), P.AllIn ? TEXT("all in ") : TEXT(""),
				P.Button ? TEXT("button ") : TEXT(""), P.X, P.Y, P.OnScreen ? TEXT("") : TEXT(" OFF SCREEN"));
		}
	}));

/** ss.Table.HoldY <seconds> [A]: Y on a pad (or A on the keyboard) held that long, then let go, as a thumb would (spec
 *  6.4's ControlParity in the game: 0.2 raises, 0.5 does nothing, 1.1 goes all in; A held 0.7 goes all in). */
FAutoConsoleCommand HoldYCmd(TEXT("ss.Table.HoldY"), TEXT("ss.Table.HoldY <seconds> [A]: hold Y (or the keyboard's A) that long and let go: 0.2 raises, 0.5 does nothing, 1.1 is all in."),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args) {
		ABackRoomGameMode* GM = TableModeNow();
		ABackRoomPawn* Pawn = GM ? Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(GM, 0)) : nullptr;
		if (!Pawn)
		{
			UE_LOG(LogTableHud, Warning, TEXT("ss.Table.HoldY: not at a card table"));
			return;
		}
		const float Seconds = FMath::Clamp(Args.Num() > 0 ? FCString::Atof(*Args[0]) : 0.2f, 0.02f, 5.0f);
		const FString Key = Args.Num() > 1 && Args[1].Equals(TEXT("A"), ESearchCase::IgnoreCase) ? FString(TEXT("A")) : FString(TEXT("Gamepad_FaceButton_Top"));
		Pawn->TestHold(Key, true);
		UE_LOG(LogTableHud, Display, TEXT("ss.Table.HoldY: %s down for %.2f s (%s)"), *Key, Seconds, GM->Table ? *GM->Table->Describe() : TEXT(""));
		const TWeakObjectPtr<ABackRoomPawn> Weak = Pawn;
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak, Key](float) {
			if (Weak.IsValid())
			{
				Weak->TestHold(Key, false);
			}
			return false;
		}), Seconds);
	}));

/** ss.Table.Feature [0|1]: the stage rail's view of the feature table (spec 6.5), as [E] at the stage rail gives it. */
FAutoConsoleCommand FeatureCmd(TEXT("ss.Table.Feature"), TEXT("ss.Table.Feature [0|1]: the feature table from the stage rail (on the rail, once the final table plays on the stage)."),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args) {
		if (ABackRoomGameMode* GM = TableModeNow())
		{
			GM->WatchFeature(Args.Num() == 0 || Args[0] != TEXT("0"));
			UE_LOG(LogTableHud, Display, TEXT("ss.Table.Feature: %s"), GM->IsWatchingFeature() ? TEXT("watching the feature table") : TEXT("off"));
		}
	}));
} // namespace BackRoomHudDetail
