// The table's HUD (ABackRoomHUD, Dee's game and the Embercrest): prompts, stacks, Focus, the heart, the clock, reads and
// the table talk, drawn over the view. Moved out of BackRoomGameMode.cpp as it stood (next_segment_spec 8.2); ue-table
// hosts the core HUD here (ss::ui::DrawTableHud through an SDrawListWidget) with this canvas HUD kept for one milestone.

#include "BackRoomGameMode.h"
#include "BackRoomGameShared.h"

#include "BackRoomPlayer.h"
#include "BackRoomPlayerController.h"
#include "BackRoomStage.h"
#include "BackRoomTable.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/App.h"
#include "NightOneAudio.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Tournament.h"

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

/**
 * The gamepad's buttons for the keys the table's prompts and hints name ("[N]", "{Space}"): A for the main action (check
 * or call, the next hand, using what's in reach), X fold, Y bet or raise, B held all in; on the d-pad left stands up,
 * right sets the pace, up shows the reads, down held leaves; the triggers peek and study, the left stick in breathes
 * (or hurries), View is the view and the right stick in the other shoulder. Several names: drawn side by side.
 */
struct FPadKey
{
	const TCHAR* Key;
	const TCHAR* Pad;
	bool bHold;
};
const FPadKey PadKeys[] = {
	{TEXT("C"), TEXT("A"), false}, {TEXT("N"), TEXT("A"), false}, {TEXT("E"), TEXT("A"), false}, {TEXT("F"), TEXT("X"), false}, {TEXT("R"), TEXT("Y"), false},
	{TEXT("A"), TEXT("B"), true}, {TEXT("Q"), TEXT("LEFT"), false}, {TEXT("T"), TEXT("RIGHT"), false}, {TEXT("L"), TEXT("DOWN"), true}, {TEXT("Tab"), TEXT("UP"), false},
	{TEXT("V"), TEXT("VIEW"), false}, {TEXT("B"), TEXT("R3"), false}, {TEXT("Space"), TEXT("LT"), false}, {TEXT("Right mouse"), TEXT("RT"), false},
	{TEXT("Shift"), TEXT("L3"), false}, {TEXT("WASD"), TEXT("LS"), false}, {TEXT("wheel"), TEXT("LB RB"), false},
};

const FPadKey* PadKeyFor(const FString& Key)
{
	for (const FPadKey& K : PadKeys)
	{
		if (Key == K.Key)
		{
			return &K;
		}
	}
	return nullptr;
}
} // namespace BackRoomGameDetail

using namespace BackRoomGameDetail;

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
		return;
	}
	const float Dt = FMath::Min(static_cast<float>(FApp::GetDeltaTime()), 0.1f);
	Clock += Dt;
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

	// After a gamepad, the prompts and hints show its buttons (the controller keeps which device was used last).
	const ABackRoomPlayerController* Pad = Cast<ABackRoomPlayerController>(PlayerOwner);
	const bool bPad = Pad && Pad->IsUsingGamepad();
	float LetterW = 0.0f, LetterH = 1.0f;
	GetTextSize(TEXT("A"), LetterW, LetterH, Small, 1.0f);
	LetterH = FMath::Max(LetterH, 1.0f);
	// How wide Button draws Name.
	auto ButtonWidth = [&](const FString& Name, float Size) -> float {
		if (Name.Len() <= 1 || Name == TEXT("UP") || Name == TEXT("DOWN") || Name == TEXT("LEFT") || Name == TEXT("RIGHT"))
		{
			return Size;
		}
		float Tw = 0.0f, Th = 0.0f;
		GetTextSize(Name, Tw, Th, Small, Size * 0.5f / LetterH);
		return FMath::Max(Size, Tw + Size * 0.55f);
	};
	// A gamepad button Size tall, its middle at height Cy: the face buttons round, in their colors; the d-pad a cross with
	// the pressed arm lit; the bumpers, triggers, sticks and View labelled keys. Returns its width.
	auto Button = [&](const FString& Name, float X, float Cy, float Size, float Alpha) -> float {
		const float R = Size * 0.5f;
		float Tw = 0.0f, Th = 0.0f;
		if (Name == TEXT("A") || Name == TEXT("B") || Name == TEXT("X") || Name == TEXT("Y"))
		{
			const FColor Hue = Name == TEXT("A") ? FColor(0x3e, 0xcf, 0x6e) : (Name == TEXT("B") ? FColor(0xef, 0x4d, 0x5a) : (Name == TEXT("X") ? FColor(0x3b, 0x82, 0xf6) : FColor(0xf2, 0xc1, 0x4e)));
			FLinearColor Fill = FLinearColor::FromSRGBColor(Hue);
			Fill.A = 0.92f * Alpha;
			FCanvasNGonItem Disc(FVector2D(X + R, Cy), FVector2D(R, R), 28, Fill);
			Disc.BlendMode = SE_BLEND_Translucent;
			Canvas->DrawItem(Disc);
			const float Sc = Size * 0.62f / LetterH;
			GetTextSize(Name, Tw, Th, Small, Sc);
			DrawText(Name, FLinearColor(0.03f, 0.035f, 0.05f, Alpha), X + R - Tw * 0.5f, Cy - Th * 0.5f, Small, Sc);
			return Size;
		}
		const FLinearColor Line(0.93f, 0.9f, 0.82f, 0.6f * Alpha);
		if (Name == TEXT("UP") || Name == TEXT("DOWN") || Name == TEXT("LEFT") || Name == TEXT("RIGHT"))
		{
			const float Arm = Size * 0.34f;
			const float Reach = R - Arm * 0.5f;
			const float Cx = X + R;
			const FLinearColor Rest(Line.R, Line.G, Line.B, 0.3f * Alpha);
			const FLinearColor Lit(0.93f, 0.9f, 0.82f, 0.95f * Alpha);
			DrawRect(Rest, Cx - Arm * 0.5f, Cy - R, Arm, Size);
			DrawRect(Rest, Cx - R, Cy - Arm * 0.5f, Size, Arm);
			if (Name == TEXT("UP"))
			{
				DrawRect(Lit, Cx - Arm * 0.5f, Cy - R, Arm, Reach);
			}
			else if (Name == TEXT("DOWN"))
			{
				DrawRect(Lit, Cx - Arm * 0.5f, Cy + Arm * 0.5f, Arm, Reach);
			}
			else if (Name == TEXT("LEFT"))
			{
				DrawRect(Lit, Cx - R, Cy - Arm * 0.5f, Reach, Arm);
			}
			else
			{
				DrawRect(Lit, Cx + Arm * 0.5f, Cy - Arm * 0.5f, Reach, Arm);
			}
			return Size;
		}
		const float Sc = Size * 0.5f / LetterH;
		GetTextSize(Name, Tw, Th, Small, Sc);
		const float Bw = ButtonWidth(Name, Size);
		const float Edge = FMath::Max(1.0f, 1.5f * S);
		DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.35f * Alpha), X, Cy - R, Bw, Size);
		DrawRect(Line, X, Cy - R, Bw, Edge);
		DrawRect(Line, X, Cy + R - Edge, Bw, Edge);
		DrawRect(Line, X, Cy - R, Edge, Size);
		DrawRect(Line, X + Bw - Edge, Cy - R, Edge, Size);
		DrawText(Name, FLinearColor(0.93f, 0.9f, 0.82f, 0.95f * Alpha), X + (Bw - Tw) * 0.5f, Cy - Th * 0.5f, Small, Sc);
		return Bw;
	};
	// A line naming the table's keys, "[N]" (said with its brackets) or "{Space}" (bare): on the keyboard as written; after
	// a gamepad, its buttons in their place (a held one says so). Align as Text; returns the width.
	struct FPiece
	{
		FString Text;
		FString Pad;
		bool bHold = false;
	};
	auto Keyed = [&](const FString& Line, const FLinearColor& Color, float X, float Y, UFont* Font, float Scale, int32 Align) -> float {
		TArray<FPiece> Pieces;
		FString Run;
		for (int32 I = 0; I < Line.Len(); ++I)
		{
			const TCHAR Ch = Line[I];
			const int32 End = Ch == TEXT('[') || Ch == TEXT('{') ? Line.Find(Ch == TEXT('[') ? TEXT("]") : TEXT("}"), ESearchCase::CaseSensitive, ESearchDir::FromStart, I + 1) : INDEX_NONE;
			if (End == INDEX_NONE)
			{
				Run.AppendChar(Ch);
				continue;
			}
			const FString Key = Line.Mid(I + 1, End - I - 1);
			I = End;
			const FPadKey* Mapped = bPad ? PadKeyFor(Key) : nullptr;
			if (!Mapped)
			{
				Run += Ch == TEXT('[') ? TEXT("[") + Key + TEXT("]") : Key;
				continue;
			}
			Pieces.Add({Run, FString(), false});
			Run.Reset();
			TArray<FString> Names;
			FString(Mapped->Pad).ParseIntoArray(Names, TEXT(" "));
			for (int32 N = 0; N < Names.Num(); ++N)
			{
				Pieces.Add({FString(), Names[N], Mapped->bHold && N == Names.Num() - 1});
			}
		}
		Pieces.Add({Run, FString(), false});
		if (Pieces.Num() == 1)
		{
			return Text(Pieces[0].Text, Color, X, Y, Font, Scale, Align);
		}
		if (Color.A <= 0.005f)
		{
			return 0.0f;
		}
		// Measured first (the line is placed whole), then drawn: the buttons as tall as the text, centered on it.
		float LineW = 0.0f, LineH = 0.0f;
		GetTextSize(TEXT("A"), LineW, LineH, Font, Scale * S);
		const float Size = LineH * 0.95f;
		const float Gap = 5.0f * S;
		const float HoldScale = Scale * S * 0.62f;
		auto Width = [&](const FPiece& P) {
			float Tw = 0.0f, Th = 0.0f;
			if (P.Pad.IsEmpty())
			{
				GetTextSize(P.Text, Tw, Th, Font, Scale * S);
				return Tw;
			}
			const float Bw = ButtonWidth(P.Pad, Size);
			float Hw = 0.0f;
			if (P.bHold)
			{
				GetTextSize(TEXT("HOLD"), Hw, Th, Font, HoldScale);
				Hw += Gap;
			}
			return Gap + Bw + Hw + Gap;
		};
		float Total = 0.0f;
		for (const FPiece& P : Pieces)
		{
			Total += Width(P);
		}
		float Px = Align == 1 ? X - Total * 0.5f : (Align == 2 ? X - Total : X);
		const float Cy = Y + LineH * 0.5f;
		for (const FPiece& P : Pieces)
		{
			if (P.Pad.IsEmpty())
			{
				if (!P.Text.IsEmpty())
				{
					DrawText(P.Text, FLinearColor(0.0f, 0.0f, 0.0f, Color.A * 0.6f), Px + 2.0f * S, Y + 2.0f * S, Font, Scale * S);
					DrawText(P.Text, Color, Px, Y, Font, Scale * S);
				}
				Px += Width(P);
				continue;
			}
			const float Next = Px + Width(P);
			float Bx = Px + Gap;
			Bx += Button(P.Pad, Bx, Cy, Size, Color.A);
			if (P.bHold)
			{
				float Hw = 0.0f, Hh = 0.0f;
				GetTextSize(TEXT("HOLD"), Hw, Hh, Font, HoldScale);
				DrawText(TEXT("HOLD"), FLinearColor(Color.R, Color.G, Color.B, Color.A * 0.7f), Bx + Gap, Cy - Hh * 0.5f, Font, HoldScale);
			}
			Px = Next;
		}
		return Total;
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

	// Table talk becomes subtitles; what your eye catches becomes a whisper.
	for (const FBackRoomLine& L : Table->TakeLines())
	{
		if (L.bRead)
		{
			Whispers.Add({FString(), L.Text, Clock + 4.8f + 0.045f * L.Text.Len(), Clock});
		}
		else
		{
			Subtitles.Add({L.Speaker, L.Text, Clock + 2.8f + 0.055f * L.Text.Len(), Clock});
		}
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
		Text(GM->IsLive() ? TEXT("E M B E R C R E S T    C A S I N O") : TEXT("S P I N    C Y C L E    L A U N D R O M A T"), Fade(Paper, A), W * 0.5f, H * 0.4f, Big, 1.35f, 1);
		Text(GM->IsLive() ? TEXT("the card room   \u00b7   ") + GM->GetLiveName().ToLower() : FString(TEXT("the back room")), Fade(Warm, A), W * 0.5f, H * 0.4f + 50.0f * S, Small, 1.45f, 1);
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
				Keyed(Prompt, Paper, W * 0.5f, H - 120.0f * S, Small, 1.3f, 1);
			}
			const ss::Tournament* T = GM->GetTourney();
			FString State;
			if (GM->IsRailing())
			{
				// Registered at the desk for the next event: that's what the night is waiting on now.
				const FString Next = GM->GetNextEventNote();
				if (!Next.IsEmpty())
				{
					State = T && !T->bFinished ? FString::Printf(TEXT("%s   \u00b7   on the rail, %d left"), *Next, T->Remaining) : Next;
				}
				else
				{
					State = T && T->bFinished ? FString(TEXT("It's over.   [E] at the door to go home"))
											  : FString::Printf(TEXT("On the rail   \u00b7   %d left   \u00b7   [E] at the door to go home"), T ? T->Remaining : 0);
				}
			}
			else if (T)
			{
				State = FString::Printf(TEXT("Up from table %d   \u00b7   still dealt in: checked when free, mucked when not   \u00b7   [E] at your chair to sit"), T->Hero().TableId);
			}
			Keyed(State, Fade(Dim, 0.9f), W * 0.5f, H - 78.0f * S, Small, 1.0f, 1);
			if (GM->ShowsHints())
			{
				Keyed(TEXT("{WASD}  walk   \u00b7   {Shift}  hurry   \u00b7   {E}  use"), Fade(Dim, 0.6f), W * 0.5f, H - 46.0f * S, Small, 0.9f, 1);
			}
		}
		else if (GM->StayOffer() >= 0.0f)
		{
			const float A = FMath::Clamp((GM->StayOffer() - 1.0f) / 0.6f, 0.0f, 1.0f);
			Keyed(TEXT("[Q] Stay and watch   \u00b7   [L] Go home now"), Fade(Paper, A), W * 0.5f, H - 70.0f * S, Small, 1.15f, 1);
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
		Keyed(bOver ? (Pawn->IsFreeWalking() ? TEXT("back at the table   ·   peeks and Focus are still through your own eyes")
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
		const float Lw = Keyed(What, Paper, W * 0.5f, H - 236.0f * S, Small, 1.1f, 1);
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
		Text(FString::Printf(TEXT("%.0f BB"), static_cast<double>(P.Stack) / static_cast<double>(P.BigBlind)), Dim, Left + StackW + 14.0f * S, H - 114.0f * S, Small, 1.1f, 0);
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
		const float Tw = 190.0f * S, Th = 36.0f * S;
		const float Tx = Left, Ty = H - 206.0f * S;
		const FLinearColor Line = FMath::Lerp(FLinearColor(0.62f, 0.95f, 0.78f, 1.0f), Blood, FMath::Clamp((Bpm - 84.0f) / 30.0f, 0.0f, 1.0f));
		for (int32 I = 1; I < Trace.Num(); ++I)
		{
			const float X0 = Tx + Tw * (I - 1) / (Keep - 1.0f), X1 = Tx + Tw * I / (Keep - 1.0f);
			const float Y0 = Ty + Th * (0.72f - 0.62f * Trace[I - 1]), Y1 = Ty + Th * (0.72f - 0.62f * Trace[I]);
			// Older trace fades out; the line glows a little, like a monitor.
			const float Fresh = FMath::Square(static_cast<float>(I) / Trace.Num());
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
	if (P.bYourTurn && Phase != EBackRoomPhase::Busted && Phase != EBackRoomPhase::Moving && !GM->IsHeroUp())
	{
		struct FRung
		{
			const TCHAR* Key;
			FString Label;
			FString Value;
			bool bMain;
		};
		TArray<FRung> Rungs;
		Rungs.Add({TEXT("F"), TEXT("FOLD"), FString(), false});
		Rungs.Add({TEXT("C"), P.bCanCheck ? TEXT("CHECK") : TEXT("CALL"), P.bCanCheck ? FString() : Amount(P.ToCall), true});
		if (P.bCanRaise)
		{
			Rungs.Add({TEXT("R"), P.bIsBet ? TEXT("BET") : TEXT("RAISE TO"), Amount(P.RaiseTo), false});
			Rungs.Add({TEXT("A"), TEXT("ALL IN"), Amount(P.Stack), false});
		}
		const float Rw = 320.0f * S, Rh = 46.0f * S, Gap = 8.0f * S;
		const float Rx = W - 48.0f * S - Rw;
		float Ry = H - 96.0f * S - Rungs.Num() * (Rh + Gap);
		LadderTop = Ry - 46.0f * S;
		const FString Head = P.ToCall > 0 ? Amount(P.ToCall) + TEXT(" to call") : TEXT("Checks to you");
		Text(Head, Warm, Rx + Rw, Ry - 46.0f * S, Big, 1.05f, 2);
		for (const FRung& R : Rungs)
		{
			DrawRect(R.bMain ? FLinearColor(0.05f, 0.08f, 0.15f, 0.84f) : Night, Rx, Ry, Rw, Rh);
			const FPadKey* PadKey = bPad ? PadKeyFor(R.Key) : nullptr;
			if (PadKey && PadKey->bHold && Pad)
			{
				// Held to go all in: the rung fills as it's held.
				DrawRect(FLinearColor(Ember.R, Ember.G, Ember.B, 0.32f), Rx, Ry, Rw * Pad->HoldProgress(EKeys::Gamepad_FaceButton_Right), Rh);
			}
			if (R.bMain)
			{
				DrawRect(Ember, Rx, Ry, Rw, 2.0f * S);
				DrawRect(Ember, Rx, Ry + Rh - 2.0f * S, Rw, 2.0f * S);
				DrawRect(Ember, Rx, Ry, 2.0f * S, Rh);
				DrawRect(Ember, Rx + Rw - 2.0f * S, Ry, 2.0f * S, Rh);
			}
			float LabelX = Rx + 52.0f * S;
			if (PadKey)
			{
				Button(PadKey->Pad, Rx + 10.0f * S, Ry + Rh * 0.5f, 28.0f * S, 1.0f);
			}
			else
			{
				DrawRect(FLinearColor(0.93f, 0.9f, 0.82f, 0.14f), Rx + 10.0f * S, Ry + 9.0f * S, 28.0f * S, 28.0f * S);
				Text(R.Key, Paper, Rx + 24.0f * S, Ry + 11.0f * S, Small, 1.05f, 1);
			}
			LabelX += Text(R.Label, Paper, LabelX, Ry + 11.0f * S, Small, 1.15f, 0);
			if (PadKey && PadKey->bHold)
			{
				Text(TEXT("HOLD"), Fade(Paper, 0.6f), LabelX + 10.0f * S, Ry + 16.0f * S, Small, 0.75f, 0);
			}
			if (!R.Value.IsEmpty())
			{
				Text(R.Value, R.bMain ? Warm : Paper, Rx + Rw - 16.0f * S, Ry + 11.0f * S, Small, 1.15f, 2);
			}
			Ry += Rh + Gap;
		}
		if (P.bCanRaise)
		{
			if (bPad)
			{
				Keyed(TEXT("{wheel}  size the raise"), Dim, Rx + Rw, Ry + 2.0f * S, Small, 0.95f, 2);
			}
			else
			{
				Text(TEXT("wheel to size the raise"), Dim, Rx + Rw, Ry + 2.0f * S, Small, 0.95f, 2);
			}
		}
	}

	// Out of the hand at a live table: the room's pace, and the rest of the hand at a glance.
	if (bLive && !P.bYourTurn && Phase == EBackRoomPhase::Playing && !Table->IsHeroInHand() && !GM->IsHeroUp())
	{
		static const TCHAR* PaceNames[3] = {TEXT("LIVE"), TEXT("BRISK"), TEXT("FAST")};
		const bool bSkipping = Pawn && Pawn->IsSkippingHand();
		Keyed(bSkipping ? FString(TEXT("to the next hand...")) : FString::Printf(TEXT("[N] Next hand     [T] Pace  %s"), PaceNames[FMath::Clamp(GM->GetTablePace(), 0, 2)]),
			Fade(Dim, 0.85f), W * 0.5f, H - 66.0f * S, Small, 1.0f, 1);
	}
	if (bLive && GM->PaceShownAge() < 2.6f)
	{
		static const TCHAR* PaceNotes[3] = {TEXT("TABLE PACE   LIVE   \u00b7   the room's own time"), TEXT("TABLE PACE   BRISK   \u00b7   quicker decisions around you"),
			TEXT("TABLE PACE   FAST   \u00b7   the room acts at once")};
		const float A = FMath::Clamp((2.6f - GM->PaceShownAge()) / 0.5f, 0.0f, 1.0f);
		Text(PaceNotes[FMath::Clamp(GM->GetTablePace(), 0, 2)], Fade(Warm, A), W * 0.5f, H * 0.3f, Small, 1.2f, 1);
		Text(TEXT("your own decisions always wait for you"), Fade(Dim, A), W * 0.5f, H * 0.3f + 30.0f * S, Small, 1.0f, 1);
	}

	// Felted.
	if (Phase == EBackRoomPhase::Busted)
	{
		Text(TEXT("You're felted."), Warm, W * 0.5f, H * 0.42f, Big, 1.4f, 1);
		if (GM->CanReload())
		{
			Keyed(FString::Printf(TEXT("[R] Buy back in for $%lld     [L] Go home"), GM->GetReloadChips()), Paper, W * 0.5f, H * 0.42f + 58.0f * S, Small, 1.3f, 1);
			const FString Spare = FString::Printf(TEXT("%s left in the bankroll"), *Dollars(GM->GetBankrollOffTable() / 100));
			Keyed((bPad ? TEXT("{wheel}  to change  ·  ") : TEXT("wheel to change  ·  ")) + Spare, Dim, W * 0.5f, H * 0.42f + 96.0f * S, Small, 1.05f, 1);
		}
		else
		{
			Keyed(TEXT("[L] Go home"), Paper, W * 0.5f, H * 0.42f + 58.0f * S, Small, 1.3f, 1);
			Text(FString::Printf(TEXT("%s left. Not enough to sit."), *Dollars(GM->GetBankrollOffTable() / 100)), Dim, W * 0.5f, H * 0.42f + 96.0f * S, Small, 1.05f, 1);
		}
	}
	else if (GM->IsLeaveRequested())
	{
		Text(TEXT("Racking up after this hand"), Dim, W * 0.5f, H - 190.0f * S, Small, 1.1f, 1);
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
				{TEXT("ANTE"), L.Ante > 0 ? N(L.Ante) : FString(TEXT("\u2014"))},
				{TEXT("PLAYERS"), FString::Printf(TEXT("%d / %d"), T->Remaining, T->Spec.Entrants)},
				{TEXT("AVG STACK"), N(static_cast<int64>(T->AverageStack()))},
				{TEXT("YOU"), FString::Printf(TEXT("%d%s"), Rank, Nth)},
				{bMoney ? TEXT("NEXT OUT") : TEXT("PAID"), bMoney ? TEXT("$") + N(T->PrizeFor(T->Remaining) / 100) : FString::Printf(TEXT("%d   \u00b7   %d to go"), Paid, T->Remaining - Paid)},
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
		Keyed(TEXT("[L] again to walk away   \u00b7   your stack will be blinded off"), Warm, W * 0.5f, H - 190.0f * S, Small, 1.15f, 1);
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

	// How to play: early on the first night (with the controls reminder on), or holding Tab (up on the d-pad).
	const bool bTab = PlayerOwner && (PlayerOwner->IsInputKeyDown(EKeys::Tab) || PlayerOwner->IsInputKeyDown(EKeys::Gamepad_DPad_Up));
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
		TArray<const TCHAR*> Hints = {TEXT("{Space}  look at your cards"), TEXT("{Right mouse}  study a face"), TEXT("{Shift}  breathe, slow the heart"), TEXT("{Tab}  your reads"),
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
		// Above the action ladder when it's up (a little more room for the gamepad's buttons).
		const float RowH = (bPad ? 28.0f : 24.0f) * S;
		float Hy = FMath::Min(H - 250.0f * S, LadderTop - 16.0f * S - Hints.Num() * RowH);
		for (const TCHAR* Hint : Hints)
		{
			Keyed(Hint, Fade(Dim, HintA), Hx, Hy, Small, 0.95f, 2);
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
	// (ue-table: the core HUD's widget comes off the viewport here.)
	HudWidget.Reset();
	HudHost.Reset();
	Super::EndPlay(Reason);
}

void ABackRoomHUD::DrawCoreHud()
{
	// ue-table (next_segment_spec 6.3): ss::ui::DrawTableHud from ABackRoomGameMode::FillTableHud, through HudWidget.
}

void ABackRoomHUD::DrawLegacyPlates(float /*W*/, float /*H*/, float /*S*/)
{
	// ue-table, P0a (next_segment_spec 6.3): the AI stack plates (ABackRoomGameMode::GetPlates) on this canvas.
}

// ------------------------------------------------------------------ the HUD's data (ue-table, next_segment_spec 6)

void ABackRoomGameMode::FillTableHud(ss::ui::TableHudInfo& /*Out*/, float /*CanvasWidth*/) const
{
}

void ABackRoomGameMode::FillTableCard(ss::ui::TableCardInfo& /*Out*/) const
{
}

void ABackRoomGameMode::UpdatePlates(float /*RealDt*/)
{
}

void ABackRoomGameMode::PlateAudit(float /*RealDt*/)
{
}

void ABackRoomGameMode::FlipStackUnits()
{
}

void ABackRoomGameMode::PushToast(const FString& From, const FString& Line, uint32 Accent)
{
	ss::ui::Toast T;
	T.From = TCHAR_TO_UTF8(*From);
	T.Line = TCHAR_TO_UTF8(*Line);
	T.Accent = Accent;
	T.At = LiveT;
	Toasts.push_back(T);
	// The lane shows a few; the rest have been read or missed.
	if (Toasts.size() > 8)
	{
		Toasts.erase(Toasts.begin(), Toasts.begin() + static_cast<std::ptrdiff_t>(Toasts.size() - 8));
	}
}

void ABackRoomGameMode::WatchFeature(bool bOn)
{
	bFeatureView = bOn;
}