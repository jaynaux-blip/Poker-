// ABackRoomPlayer's tells and table manners: what the hand they hold does to them, what their body
// lets slip (each persona has 2 to 4 tells, each with a reliability), and what they say.
//
// A tell fires when something happens that it answers (a strong hand seen, a bluff made, a draw
// missed): with the tell's Reliability when it is honest about that moment, and its FalseRate when it
// isn't (noise; and a pro's "tell" that means the opposite is just a tell whose Means is reversed).

#include "BackRoomChips.h"
#include "BackRoomGameMode.h"
#include "BackRoomPlayer.h"
#include "BackRoomPlayerShared.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "Misc/App.h"
#include "ShortStack/Audio/Voice.h"
#include "ShortStack/Game/Room.h"
#include "ShortStack/Game/TableTalk.h"
#include "ShortStack/UI/Captions.h"

#include <string>

using namespace BackRoomPlayerDetail;

DEFINE_LOG_CATEGORY_STATIC(LogBackRoomTells, Log, All);

namespace BackRoomTellsDetail
{
/** A regular's lines for Dee's game: their chat, win, lose and noticed-staring lines, in their order. */
struct FLines
{
	TArray<FString> Chat;
	TArray<FString> Win;
	TArray<FString> Lose;
	TArray<FString> Noticed;
};

/**
 * The Back Room's eight, word for word (next_segment_spec 4.11): their lines live in core's talk::Cast now, frozen by
 * room_test, and Dee's game reads them from there in the same order, so its timers and its draws pick exactly what they
 * always picked. Anyone else has none. (Names match in any case, as they always did.)
 */
const FLines& LinesFor(const FString& Name)
{
	static TMap<FString, TUniquePtr<FLines>> Cache;
	const FString Key = Name.ToLower();
	if (const TUniquePtr<FLines>* Found = Cache.Find(Key))
	{
		return **Found;
	}
	TUniquePtr<FLines> L = MakeUnique<FLines>();
	const std::string Who(TCHAR_TO_UTF8(*Name));
	auto Fill = [&Who](ss::talk::Moment M, TArray<FString>& Out) {
		for (const std::string& Line : ss::talk::CastLines(Who, M))
		{
			Out.Add(FString(UTF8_TO_TCHAR(Line.c_str())));
		}
	};
	Fill(ss::talk::Moment::Small, L->Chat);
	Fill(ss::talk::Moment::Win, L->Win);
	Fill(ss::talk::Moment::Lose, L->Lose);
	Fill(ss::talk::Moment::Noticed, L->Noticed);
	const FLines& Out = *L;
	Cache.Add(Key, MoveTemp(L));
	return Out;
}

/** The Embercrest's room has its one voice (the director, next_segment_spec 1.5): this person's own chatter stays quiet there. */
ABackRoomGameMode* LiveRoomOf(const AActor* A)
{
	const UWorld* World = A ? A->GetWorld() : nullptr;
	ABackRoomGameMode* GM = World ? World->GetAuthGameMode<ABackRoomGameMode>() : nullptr;
	return GM && GM->IsLive() && GM->GetRoomSound() ? GM : nullptr;
}

/** What vowel the mouth is on at T into a plan: its rounding (back vowels, low F2) and its spread (front, high F2). */
void VowelAt(const ss::audio::SpeechPlan& P, float T, float& OutRound, float& OutSpread)
{
	OutRound = 0.0f;
	OutSpread = 0.0f;
	for (const ss::audio::Syllable& S : P.Syllables)
	{
		if (T >= S.Start && T < S.Start + S.Length)
		{
			OutRound = FMath::Clamp((1250.0f - S.F2) / 450.0f, 0.0f, 1.0f);
			OutSpread = FMath::Clamp((S.F2 - 1800.0f) / 600.0f, 0.0f, 1.0f);
			return;
		}
	}
}

/** A tell's length and the delay before it shows (a reaction is never instant). */
void Timing(EBackRoomTell Tell, FRandomStream& Rng, float& Duration, float& Delay)
{
	switch (Tell)
	{
	case EBackRoomTell::ChipGlance: Duration = 0.55f; Delay = Rng.FRandRange(0.3f, 0.7f); break;
	case EBackRoomTell::BrowFlash: Duration = 0.4f; Delay = Rng.FRandRange(0.1f, 0.25f); break;
	case EBackRoomTell::Swallow: Duration = 1.1f; Delay = Rng.FRandRange(0.6f, 1.6f); break;
	case EBackRoomTell::LipPress: Duration = Rng.FRandRange(2.5f, 4.0f); Delay = Rng.FRandRange(0.2f, 0.8f); break;
	case EBackRoomTell::Freeze: Duration = 14.0f; Delay = 0.2f; break;
	case EBackRoomTell::StareDown: Duration = 12.0f; Delay = Rng.FRandRange(0.5f, 1.0f); break;
	case EBackRoomTell::LookAway: Duration = Rng.FRandRange(4.0f, 7.0f); Delay = Rng.FRandRange(0.3f, 1.0f); break;
	case EBackRoomTell::Tremble: Duration = 7.0f; Delay = 0.0f; break;
	case EBackRoomTell::NeckTouch: Duration = 2.6f; Delay = Rng.FRandRange(0.5f, 1.5f); break;
	case EBackRoomTell::FalseSmile: Duration = 1.3f; Delay = Rng.FRandRange(0.2f, 0.5f); break;
	case EBackRoomTell::RealSmile: Duration = 2.4f; Delay = Rng.FRandRange(0.05f, 0.2f); break;
	case EBackRoomTell::BlinkBurst: Duration = 1.2f; Delay = 0.0f; break;
	case EBackRoomTell::Sigh: Duration = 2.0f; Delay = Rng.FRandRange(0.4f, 0.9f); break;
	case EBackRoomTell::Recheck: Duration = 2.2f; Delay = Rng.FRandRange(1.0f, 2.5f); break;
	case EBackRoomTell::PupilFlare: Duration = 9.0f; Delay = 0.2f; break;
	case EBackRoomTell::ChipReach: Duration = 10.0f; Delay = Rng.FRandRange(0.5f, 1.5f); break;
	default: Duration = 1.0f; Delay = 0.0f; break;
	}
}

/** Tells that last until the moment passes (the table ends them), not for a set time. */
bool IsSustained(EBackRoomTell Tell)
{
	return Tell == EBackRoomTell::Freeze || Tell == EBackRoomTell::StareDown || Tell == EBackRoomTell::Tremble || Tell == EBackRoomTell::ChipReach;
}
} // namespace BackRoomTellsDetail

using namespace BackRoomTellsDetail;

// ------------------------------------------------------------------ firing tells

void ABackRoomPlayer::Provoke(EBackRoomTellMeaning Meaning, float Intensity)
{
	if (SeatRole != EBackRoomRole::Player)
	{
		return;
	}
	for (const FBackRoomTell& T : Persona.Tells)
	{
		const float P = T.Means == Meaning ? T.Reliability : T.FalseRate;
		if (Rng.FRand() < P * FMath::Clamp(Intensity, 0.3f, 1.2f) && !IsActive(T.Tell))
		{
			PlayTell(T.Tell, FMath::Clamp(Intensity, 0.4f, 1.0f));
			FActiveTell& Fired = Active.Last();
			Fired.bCounts = true;
			Fired.bHonest = T.Means == Meaning;
			Fired.Means = T.Means;
			UE_LOG(LogBackRoomTells, Display, TEXT("%s: %s (%s, strength %.2f%s)"), *Persona.Name, *UEnum::GetValueAsString(T.Tell),
				T.Means == Meaning ? TEXT("honest") : TEXT("false"), Strength, bBluffing ? TEXT(", bluffing") : TEXT(""));
		}
	}
}

void ABackRoomPlayer::PlayTell(EBackRoomTell Tell, float Intensity)
{
	FActiveTell& T = Active.AddDefaulted_GetRef();
	T.Tell = Tell;
	T.Intensity = Intensity;
	float Delay = 0.0f;
	Timing(Tell, Rng, T.Duration, Delay);
	T.T = -Delay;
}

bool ABackRoomPlayer::IsWatchingHero() const
{
	return NoticeT >= 0.0f || Active.ContainsByPredicate([](const FActiveTell& A) { return A.Tell == EBackRoomTell::StareDown && A.T >= 0.0f; });
}

bool ABackRoomPlayer::IsActive(EBackRoomTell Tell) const
{
	return Active.ContainsByPredicate([Tell](const FActiveTell& A) { return A.Tell == Tell; });
}

void ABackRoomPlayer::EndTell(EBackRoomTell Tell)
{
	for (FActiveTell& A : Active)
	{
		if (A.Tell == Tell && A.T < A.Duration - 0.4f)
		{
			// Let it fade rather than snap.
			A.Duration = FMath::Max(A.T, 0.0f) + 0.4f;
		}
	}
}

void ABackRoomPlayer::UpdateTells(float Dt)
{
	float StillGoal = 0.0f;
	float TrembleGoal = 0.0f;
	for (int32 I = Active.Num() - 1; I >= 0; --I)
	{
		FActiveTell& A = Active[I];
		const bool bStarting = A.T < 0.0f && A.T + Dt >= 0.0f;
		A.T += Dt;
		if (A.T < 0.0f)
		{
			continue;
		}
		if (bStarting)
		{
			switch (A.Tell)
			{
			case EBackRoomTell::ChipGlance:
				// A flick of the eyes down to the stack and back: the head barely moves.
				GazeHold = StackPile ? StackPile->GetTop() : Spots.Stack;
				GazeHoldLeft = A.Duration;
				HeadFollow = 0.3f;
				break;
			case EBackRoomTell::StareDown:
				HeadFollow = 0.95f;
				break;
			case EBackRoomTell::LookAway:
				GazeHold = OthersAt.Num() > 0 ? OthersAt[Rng.RandRange(0, OthersAt.Num() - 1)] : GetActorTransform().TransformPosition(FVector(80.0, -120.0, 110.0));
				GazeHoldLeft = A.Duration;
				HeadFollow = 0.85f;
				if (Rng.FRand() < Persona.Chatter)
				{
					const FLines& L = LinesFor(Persona.Name);
					if (L.Chat.Num() > 0)
					{
						// (The same draw either way; at the Embercrest the director decides who talks.)
						const int32 Pick = Rng.RandRange(0, L.Chat.Num() - 1);
						if (!LiveRoomOf(this))
						{
							Say(L.Chat[Pick]);
						}
					}
				}
				break;
			case EBackRoomTell::NeckTouch:
				if (Steps[0].Num() == 0 && Body->GetBoneIndex(TEXT("neck_01")) != INDEX_NONE)
				{
					// The left hand to the side of the neck, rubbing.
					const FVector Neck = ToBody(Body->GetSocketLocation(TEXT("neck_01"))) + FVector(4.0, 4.0, 1.0);
					FHandPose P;
					P.Pos = Neck + FVector(4.0, 10.0, -12.0);
					P.Finger = FVector(-0.35, -0.3, 1.0).GetSafeNormal();
					P.Palm = FVector(-0.6, -0.8, 0.0).GetSafeNormal();
					P.Curl = 0.25f;
					FHandPose Rub = P;
					Rub.Pos += FVector(0.0, 0.0, 2.5);
					Queue(0, P, 0.5f, 4.0f);
					Queue(0, Rub, 0.45f);
					Queue(0, P, 0.45f);
					Queue(0, Rub, 0.45f);
					QueueRest(0, 0.55f);
				}
				break;
			case EBackRoomTell::BlinkBurst:
				BurstBlinks = 4 + Rng.RandRange(0, 2);
				break;
			case EBackRoomTell::Sigh:
				SighT = 0.0f;
				GazeHold = GetActorTransform().TransformPosition(FVector(160.0, Rng.FRandRange(-60.0f, 60.0f), 185.0));
				GazeHoldLeft = 1.1f;
				HeadFollow = 0.7f;
				break;
			case EBackRoomTell::Recheck:
				if (Steps[0].Num() == 0 && Steps[1].Num() == 0)
				{
					PeekHole();
				}
				break;
			case EBackRoomTell::ChipReach:
				if (Steps[1].Num() == 0 && StackPile && StackPile->GetAmount() > 0)
				{
					// Fingers on the chips, as if ready to call whatever comes.
					const FHandPose Ready = Touch(StackPile->GetTop() + FVector(0.0, 0.0, 0.4), FVector(-0.25, 1.0, -0.35), FVector(0.25, 0.0, -1.0), 0.45f, 0.4f);
					Queue(1, Ready, 0.6f, 2.0f);
					Queue(1, Ready, A.Duration);
				}
				break;
			default:
				break;
			}
		}
		// Sustained effects.
		const float Env = Envelope(A.T / A.Duration, 0.08f, 0.08f);
		switch (A.Tell)
		{
		case EBackRoomTell::Freeze: StillGoal = FMath::Max(StillGoal, A.Intensity * Env); break;
		case EBackRoomTell::Tremble: TrembleGoal = FMath::Max(TrembleGoal, 0.32f * A.Intensity * Env); break;
		case EBackRoomTell::StareDown:
			GazeHold = HeroEyes;
			GazeHoldLeft = 0.2f;
			StillGoal = FMath::Max(StillGoal, 0.5f * Env);
			break;
		default: break;
		}
		// What the hero saw of it: reported once clearly seen, or at the end with the best look they got.
		if (A.bCounts && !A.bReported && TellHook)
		{
			A.SeenMax = FMath::Max(A.SeenMax, Studied);
			if (Studied >= 0.45f || (A.T >= A.Duration && A.SeenMax >= 0.2f))
			{
				A.bReported = true;
				TellHook(this, static_cast<uint8>(A.Tell), static_cast<uint8>(A.Means), A.bHonest, A.SeenMax);
			}
		}
		if (A.T >= A.Duration)
		{
			const EBackRoomTell Ended = A.Tell;
			Active.RemoveAt(I);
			if (Ended == EBackRoomTell::Freeze)
			{
				// The breath comes back, and the eyes catch up on the blinks they owed.
				PlayTell(EBackRoomTell::BlinkBurst, 1.0f);
			}
			if (Ended == EBackRoomTell::ChipReach)
			{
				Steps[1].Reset();
				QueueRest(1, 0.6f);
			}
		}
	}
	TrembleGoal = FMath::Max(TrembleGoal, ExternalTremble);
	Stillness = Ease(Stillness, StillGoal, 3.0f, Dt);
	Tremble = Ease(Tremble, TrembleGoal, 4.0f, Dt);

	// Talking, and idle chatter between hands.
	TalkLeft = FMath::Max(0.0f, TalkLeft - Dt);
	if (SpeechNow.IsValid())
	{
		// A line's speech runs in real time (the room's sound isn't slowed by Focus), and the jaw with it.
		SpeechT += FMath::Min(static_cast<float>(FApp::GetDeltaTime()), 0.1f);
		if (SpeechT > SpeechNow->Seconds + 0.25f)
		{
			SpeechNow.Reset();
		}
	}
	NoticeCooldown -= Dt;
	NextChat -= Dt;
	const UWorld* World = GetWorld();
	if (SeatRole == EBackRoomRole::Player && World && World->IsGameWorld())
	{
		if (NextChat <= 0.0f)
		{
			NextChat = Rng.FRandRange(25.0f, 70.0f) / (0.3f + Persona.Chatter);
			const FLines& L = LinesFor(Persona.Name);
			if (!bThinking && !bBluffing && L.Chat.Num() > 0 && Rng.FRand() < 0.4f + 0.6f * Persona.Chatter)
			{
				// (The same draws either way: at the Embercrest the director decides who speaks, and this timer is quiet.)
				const int32 Pick = Rng.RandRange(0, L.Chat.Num() - 1);
				if (!LiveRoomOf(this))
				{
					Say(L.Chat[Pick]);
				}
			}
		}
		// Stare too long and they notice.
		StudiedFor = Studied > 0.7f ? StudiedFor + Dt : FMath::Max(0.0f, StudiedFor - 2.0f * Dt);
		if (StudiedFor > 3.2f && NoticeCooldown <= 0.0f)
		{
			NoticeCooldown = 30.0f;
			StudiedFor = 0.0f;
			NoticeT = 0.0f;
			GazeHold = HeroEyes;
			GazeHoldLeft = 2.4f;
			HeadFollow = 0.9f;
			const FLines& L = LinesFor(Persona.Name);
			if (L.Noticed.Num() > 0 && Rng.FRand() < 0.75f)
			{
				Say(L.Noticed[Rng.RandRange(0, L.Noticed.Num() - 1)]);
			}
		}
	}
	if (NoticeT >= 0.0f)
	{
		NoticeT += Dt / 2.4f;
		if (NoticeT >= 1.0f)
		{
			NoticeT = -1.0f;
		}
	}
}

void ABackRoomPlayer::TellFace(TFunctionRef<void(const TCHAR*, float)> Add, TFunctionRef<void(const TCHAR*, float)> Both)
{
	for (const FActiveTell& A : Active)
	{
		if (A.T < 0.0f)
		{
			continue;
		}
		const float U = A.T / A.Duration;
		const float I = A.Intensity;
		switch (A.Tell)
		{
		case EBackRoomTell::BrowFlash:
		{
			const float E = Envelope(U, 0.25f, 0.5f) * I;
			Both(TEXT("browRaiseIn"), 0.6f * E);
			Both(TEXT("browRaiseOuter"), 0.5f * E);
			Both(TEXT("eyeUpperLidUp"), 0.35f * E);
			break;
		}
		case EBackRoomTell::Swallow:
		{
			// The four phases of a swallow, each a bump in turn, the lips sealed through it.
			const float Ph[4] = {Envelope((U - 0.0f) / 0.3f, 0.4f, 0.4f), Envelope((U - 0.2f) / 0.3f, 0.4f, 0.4f), Envelope((U - 0.45f) / 0.3f, 0.4f, 0.4f), Envelope((U - 0.7f) / 0.3f, 0.4f, 0.4f)};
			Add(TEXT("neckSwallowPh1"), Ph[0] * I);
			Add(TEXT("neckSwallowPh2"), Ph[1] * I);
			Add(TEXT("neckSwallowPh3"), Ph[2] * I);
			Add(TEXT("neckSwallowPh4"), Ph[3] * I);
			Both(TEXT("mouthLipsPress"), 0.35f * Envelope(U, 0.15f, 0.2f) * I);
			Both(TEXT("jawClench"), 0.2f * Envelope(U, 0.15f, 0.2f) * I);
			break;
		}
		case EBackRoomTell::LipPress:
		{
			// The lips pressed until they disappear.
			const float E = Envelope(U, 0.15f, 0.3f) * I;
			Both(TEXT("mouthLipsPress"), 0.6f * E);
			Add(TEXT("mouthLipsThinInwardUL"), 0.45f * E);
			Add(TEXT("mouthLipsThinInwardUR"), 0.45f * E);
			Add(TEXT("mouthLipsThinInwardDL"), 0.45f * E);
			Add(TEXT("mouthLipsThinInwardDR"), 0.45f * E);
			Both(TEXT("jawClench"), 0.15f * E);
			break;
		}
		case EBackRoomTell::Freeze:
		{
			const float E = Envelope(U, 0.05f, 0.05f) * I;
			Both(TEXT("eyeUpperLidUp"), 0.08f * E);
			Add(TEXT("mouthLipsTogetherUL"), 0.3f * E);
			Add(TEXT("mouthLipsTogetherUR"), 0.3f * E);
			break;
		}
		case EBackRoomTell::StareDown:
		{
			const float E = Envelope(U, 0.08f, 0.08f) * I;
			Both(TEXT("browDown"), 0.22f * E);
			Both(TEXT("eyeSquintInner"), 0.2f * E);
			Both(TEXT("jawClench"), 0.15f * E);
			break;
		}
		case EBackRoomTell::LookAway:
		{
			const float E = Envelope(U, 0.2f, 0.2f) * I;
			Both(TEXT("eyeRelax"), 0.2f * E);
			Add(TEXT("mouthCornerPullL"), 0.08f * E);
			break;
		}
		case EBackRoomTell::Tremble:
			Both(TEXT("noseNostrilDilate"), 0.25f * Envelope(U, 0.1f, 0.1f) * I);
			break;
		case EBackRoomTell::NeckTouch:
			Both(TEXT("mouthLipsPress"), 0.25f * Envelope(U, 0.2f, 0.3f) * I);
			Both(TEXT("browRaiseIn"), 0.15f * Envelope(U, 0.2f, 0.3f) * I);
			break;
		case EBackRoomTell::FalseSmile:
		{
			// The mouth only, lopsided, on and off like a switch: the eyes stay flat.
			const float E = Envelope(U, 0.1f, 0.15f) * I;
			Add(TEXT("mouthCornerPullR"), 0.5f * E);
			Add(TEXT("mouthCornerPullL"), 0.3f * E);
			Both(TEXT("mouthStretch"), 0.12f * E);
			break;
		}
		case EBackRoomTell::RealSmile:
		{
			// Rises slowly and reaches the eyes; then they catch it and press it away.
			const float E = Envelope(U, 0.35f, 0.35f) * I;
			const float Caught = FMath::Clamp((U - 0.45f) / 0.3f, 0.0f, 1.0f);
			Both(TEXT("mouthCornerPull"), 0.42f * E * (1.0f - 0.6f * Caught));
			Both(TEXT("eyeCheekRaise"), 0.28f * E);
			Both(TEXT("eyeSquintInner"), 0.08f * E);
			Both(TEXT("mouthLipsPress"), 0.35f * E * Caught);
			break;
		}
		case EBackRoomTell::Sigh:
		{
			const float Out = Envelope((U - 0.4f) / 0.6f, 0.2f, 0.4f) * I;
			Both(TEXT("browRaiseIn"), 0.35f * Envelope(U, 0.2f, 0.3f) * I);
			Add(TEXT("jawOpen"), 0.1f * Out);
			Both(TEXT("mouthLipsBlow"), 0.35f * Out);
			Both(TEXT("mouthCheekBlow"), 0.15f * Out);
			break;
		}
		case EBackRoomTell::PupilFlare:
			Both(TEXT("eyePupilWide"), 0.75f * Envelope(U, 0.15f, 0.3f) * I);
			break;
		default:
			break;
		}
	}

	// Noticing they're being stared at: a narrowed look and the hint of a smirk.
	if (NoticeT >= 0.0f)
	{
		const float E = Envelope(NoticeT, 0.15f, 0.3f);
		Both(TEXT("browDown"), 0.25f * E);
		Both(TEXT("eyeSquintInner"), 0.25f * E);
		Add(TEXT("mouthCornerPullR"), 0.2f * E);
	}

	// Talking: syllables in the jaw and lips. With the line's speech (the card room's), the jaw opens with each syllable's
	// vowel as the voice says it (ss::audio::MouthAt: it tracks F1), the lips round for the back vowels and spread for the
	// front ones; closed between syllables.
	if (SpeechNow.IsValid())
	{
		const float Open = ss::audio::MouthAt(*SpeechNow, SpeechT);
		float Round = 0.0f;
		float Spread = 0.0f;
		VowelAt(*SpeechNow, SpeechT, Round, Spread);
		const float Fade = FMath::Clamp((SpeechNow->Seconds + 0.25f - SpeechT) / 0.25f, 0.0f, 1.0f);
		Add(TEXT("jawOpen"), Fade * (0.03f + 0.3f * Open));
		Add(TEXT("mouthFunnelUL"), Fade * 0.32f * Round * (0.4f + 0.6f * Open));
		Add(TEXT("mouthFunnelUR"), Fade * 0.32f * Round * (0.4f + 0.6f * Open));
		Add(TEXT("mouthFunnelDL"), Fade * 0.32f * Round * (0.4f + 0.6f * Open));
		Add(TEXT("mouthFunnelDR"), Fade * 0.32f * Round * (0.4f + 0.6f * Open));
		Both(TEXT("mouthStretch"), Fade * 0.16f * Spread * Open);
		Both(TEXT("mouthUpperLipRaise"), Fade * 0.1f * Open);
	}
	else if (TalkLeft > 0.0f)
	{
		const float W = FMath::Clamp(TalkLeft / 0.2f, 0.0f, 1.0f);
		const float Syl = 0.5f + 0.5f * FMath::Sin(Time * 15.0f + 2.0f * FMath::Sin(Time * 3.3f));
		const float Shape = 0.5f + 0.5f * FMath::Sin(Time * 6.1f + 1.3f);
		Add(TEXT("jawOpen"), W * (0.06f + 0.16f * Syl));
		Add(TEXT("mouthFunnelUL"), W * 0.2f * Shape * (1.0f - Syl));
		Add(TEXT("mouthFunnelUR"), W * 0.2f * Shape * (1.0f - Syl));
		Add(TEXT("mouthFunnelDL"), W * 0.2f * Shape * (1.0f - Syl));
		Add(TEXT("mouthFunnelDR"), W * 0.2f * Shape * (1.0f - Syl));
		Both(TEXT("mouthStretch"), W * 0.12f * (1.0f - Shape) * Syl);
		Both(TEXT("mouthUpperLipRaise"), W * 0.1f * Syl);
	}
}

// ------------------------------------------------------------------ the hand being played

void ABackRoomPlayer::BeginHand()
{
	bInHand = true;
	bBluffing = bValue = bThinking = bHeroThinking = false;
	Strength = PrevStrength = 0.5f;
	GripSide = -1;
	TrackSide = -1;
	HeroLift = 0.0f;
	for (FActiveTell& A : Active)
	{
		if (IsSustained(A.Tell))
		{
			A.Duration = FMath::Max(A.T, 0.0f) + 0.3f;
		}
	}
}

void ABackRoomPlayer::SetHandStrength(float InStrength, bool bInHandNow)
{
	PrevStrength = Strength;
	Strength = FMath::Clamp(InStrength, 0.0f, 1.0f);
	if (bInHand && !bInHandNow)
	{
		// Out of it: the pressure goes, and with it the act.
		bBluffing = bValue = bThinking = false;
		for (FActiveTell& A : Active)
		{
			if (IsSustained(A.Tell))
			{
				A.Duration = FMath::Max(A.T, 0.0f) + 0.4f;
			}
		}
	}
	bInHand = bInHandNow;
}

void ABackRoomPlayer::ReactToHole()
{
	if (SeatRole != EBackRoomRole::Player || !bInHand)
	{
		return;
	}
	if (Strength > 0.66f)
	{
		ThrillGoal = FMath::Max(ThrillGoal, 0.3f + Strength - 0.66f);
		Provoke(EBackRoomTellMeaning::Strong, 0.5f + (Strength - 0.66f) * 3.0f);
	}
	else if (Strength < 0.36f)
	{
		GloomGoal = FMath::Max(GloomGoal, 0.1f);
		Provoke(EBackRoomTellMeaning::Weak, 0.6f);
	}
}

void ABackRoomPlayer::OnBoard(float NewStrength)
{
	PrevStrength = Strength;
	Strength = FMath::Clamp(NewStrength, 0.0f, 1.0f);
	// Everyone looks at the new cards.
	Attention = PotAt;
	AttentionLeft = 1.6f;
	GazeLeft = FMath::Min(GazeLeft, Rng.FRandRange(0.05f, 0.3f));
	// A new street: whatever act was on is over (the stress it built lingers on its own).
	bBluffing = false;
	bValue = false;
	EndTell(EBackRoomTell::StareDown);
	EndTell(EBackRoomTell::Freeze);
	if (SeatRole != EBackRoomRole::Player || !bInHand)
	{
		return;
	}
	const float D = Strength - PrevStrength;
	if (Strength > 0.7f && D > 0.12f)
	{
		ThrillGoal = FMath::Max(ThrillGoal, 0.4f + D);
		Provoke(EBackRoomTellMeaning::Strong, 0.6f + D);
	}
	else if (D < -0.2f && Strength < 0.4f)
	{
		GloomGoal = FMath::Max(GloomGoal, 0.25f);
		Provoke(EBackRoomTellMeaning::Weak, 0.7f);
	}
}

void ABackRoomPlayer::BeginThink(int64 ToCall, int64 Pot, int64 Stack)
{
	bThinking = true;
	Attention = PotAt;
	AttentionLeft = 1.2f;
	// The price weighs on a weak hand.
	const float Price = Stack > 0 ? FMath::Clamp(static_cast<float>(ToCall) / static_cast<float>(Stack), 0.0f, 1.0f) : 0.0f;
	StressGoal = FMath::Max(StressGoal, 0.2f + 0.6f * Price * (1.0f - Strength));
	// While deciding the hand drifts to the chips, or a chin comes to rest on a fist (or the hands stay home).
	PickHabit(true);
}

void ABackRoomPlayer::OnActed(bool bAggressive, bool bBluff)
{
	bThinking = false;
	if (!bAggressive || SeatRole != EBackRoomRole::Player)
	{
		return;
	}
	bBluffing = bBluff;
	bValue = !bBluff && Strength > 0.66f;
	if (bBluffing)
	{
		StressGoal = FMath::Max(StressGoal, 0.65f);
		Provoke(EBackRoomTellMeaning::Bluff, 1.0f);
	}
	else if (bValue)
	{
		ThrillGoal = FMath::Max(ThrillGoal, 0.5f);
		Provoke(EBackRoomTellMeaning::Strong, 0.9f);
	}
}

void ABackRoomPlayer::OnOtherAction(const FVector& Who, float Aggression, bool bHeroActing)
{
	Attention = Who;
	AttentionLeft = 1.2f + Aggression;
	GazeLeft = FMath::Min(GazeLeft, Rng.FRandRange(0.1f, 0.4f));
	if (bInHand && Aggression > 0.5f)
	{
		StressGoal = FMath::Max(StressGoal, StressGoal + 0.15f * (1.0f - Strength));
	}
}

void ABackRoomPlayer::OnHeroThinking(bool bThinkingNow, int64 HeroToCall)
{
	const bool bWas = bHeroThinking;
	bHeroThinking = bThinkingNow;
	if (bThinkingNow && !bWas)
	{
		Attention = HeroEyes;
		AttentionLeft = 2.0f;
		// Weak and the hero might bet: some try to stop it.
		if (SeatRole == EBackRoomRole::Player && bInHand && !bBluffing && !bValue && HeroToCall == 0 && Strength < 0.45f)
		{
			Provoke(EBackRoomTellMeaning::Weak, 0.7f);
		}
	}
	else if (!bThinkingNow && bWas)
	{
		EndTell(EBackRoomTell::ChipReach);
		EndTell(EBackRoomTell::Freeze);
		EndTell(EBackRoomTell::StareDown);
	}
}

void ABackRoomPlayer::OnResult(int64 Delta, int64 PotSize, bool bShowdown)
{
	SetHandStrength(Strength, false);
	const FLines& L = LinesFor(Persona.Name);
	if (Delta > 0)
	{
		ThrillGoal = FMath::Min(1.0f, 0.4f + static_cast<float>(Delta) / FMath::Max<float>(1.0f, static_cast<float>(PotSize)));
		GloomGoal = 0.0f;
		if (SeatRole == EBackRoomRole::Player)
		{
			PlayTell(EBackRoomTell::RealSmile, 1.0f);
			if (L.Win.Num() > 0 && Rng.FRand() < 0.35f + 0.5f * Persona.Chatter)
			{
				// (At the Embercrest a win's line is the director's: the same draw, nothing said here.)
				const int32 Pick = Rng.RandRange(0, L.Win.Num() - 1);
				if (!LiveRoomOf(this))
				{
					Say(L.Win[Pick]);
				}
			}
		}
	}
	else if (Delta < -20)
	{
		GloomGoal = FMath::Min(1.0f, 0.3f + static_cast<float>(-Delta) / 200.0f);
		ThrillGoal = 0.0f;
		if (SeatRole == EBackRoomRole::Player && bShowdown && L.Lose.Num() > 0 && Rng.FRand() < 0.3f + 0.6f * Persona.Chatter)
		{
			const int32 Pick = Rng.RandRange(0, L.Lose.Num() - 1);
			if (!LiveRoomOf(this))
			{
				Say(L.Lose[Pick]);
			}
		}
	}
}

void ABackRoomPlayer::Say(const FString& Line)
{
	if (Line.IsEmpty())
	{
		return;
	}
	ABackRoomGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ABackRoomGameMode>() : nullptr;
	if (GM && GM->IsLive() && GM->GetRoomSound() && SeatRole != EBackRoomRole::Hero)
	{
		// The Embercrest has one voice (next_segment_spec 1.5): a line of the host's own (a greeting, the dealer's host
		// lines, a read of the player's shaking hands) goes to the room, which says it with this person's voice from
		// where they are, moves the jaw on its syllables and captions it with its sound (ABackRoomGameMode::PlayCue).
		ss::room::Cue C;
		C.What = SeatRole == EBackRoomRole::Dealer ? ss::room::Kind::Call : ss::room::Kind::Line;
		C.Where = SeatRole == EBackRoomRole::Extra ? ss::room::From::Table : ss::room::From::HeroTable;
		C.Who = "actor:" + std::to_string(GetUniqueID());
		C.Speaker = std::string(TCHAR_TO_UTF8(*Persona.Name)); // (the room names them properly once it finds them)
		C.Key = SeatRole == EBackRoomRole::Dealer ? "host.dealer" : "host.line";
		C.Text = std::string(TCHAR_TO_UTF8(*Line));
		C.Effort = 0.6f;
		GM->PlayCue(C);
		return;
	}
	Lines.Add(Line);
	TalkLeft = 0.5f + 0.055f * Line.Len();
	// The core HUD's caption zone and the night log (GetCaptions): every line said at the table, Dee's game too. (Dee's
	// own lines are captioned here as hers; the legacy HUD reads the table's lines as it always did.)
	if (GM && SeatRole != EBackRoomRole::Hero)
	{
		ss::ui::Caption Cap;
		Cap.Speaker = std::string(TCHAR_TO_UTF8(*Persona.Name));
		Cap.Line = std::string(TCHAR_TO_UTF8(*Line));
		Cap.At = FPlatformTime::Seconds();
		Cap.Hold = ss::ui::CaptionHold(Cap.Line, 0.0f);
		Cap.Clock = std::string(TCHAR_TO_UTF8(*GM->ClockLabel().RightChop(5).TrimStart()));
		GM->PushCaption(Cap);
	}
}

void ABackRoomPlayer::Say(const FString& Line, const ss::audio::SpeechPlan* Plan)
{
	// The room's own path (next_segment_spec 4.11): the line is heard now, from this person; the jaw follows the plan's
	// syllables (ss::audio::MouthAt). Its sound is the host's (one engine voice at the head, or the room's stream), so is its
	// caption. Null: the jaw flaps for the line's length, as ever. Never routed back to the room (it came from there).
	Lines.Add(Line);
	if (Plan && !Plan->Syllables.empty())
	{
		SpeechNow = MakeShared<ss::audio::SpeechPlan>(*Plan);
		SpeechT = 0.0f;
		TalkLeft = Plan->Seconds + 0.15f;
	}
	else
	{
		SpeechNow.Reset();
		TalkLeft = 0.5f + 0.055f * Line.Len();
	}
}

TArray<FString> ABackRoomPlayer::TakeLines()
{
	TArray<FString> Out = MoveTemp(Lines);
	Lines.Reset();
	return Out;
}
