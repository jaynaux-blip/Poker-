#include "BackRoomTable.h"

#include "BackRoomAmbience.h"
#include "BackRoomCard.h"
#include "BackRoomChips.h"
#include "BackRoomPlayer.h"
#include "BackRoomStage.h"
#include "CardRoomAmbience.h"
#include "Engine/World.h"
#include "Misc/App.h"
#include "NightOneAudio.h"
#include "TimerManager.h"

#include "ShortStack/AI/Bot.h"
#include "ShortStack/AI/View.h"
#include "ShortStack/Cards.h"
#include "ShortStack/Equity.h"

namespace BackRoomTableDetail
{
// ABackRoomStage's layout: a seated player's chair front edge is this far out from the rail.
const double RailGap = ABackRoomStage::RailGap;
const int64 SmallBlind = 1;
const int64 BigBlind = 2;

std::vector<int> ToStd(const TArray<int32>& In)
{
	std::vector<int> Out;
	Out.reserve(In.Num());
	for (int32 C : In)
	{
		Out.push_back(C);
	}
	return Out;
}

FString CardName(int32 Card)
{
	return UTF8_TO_TCHAR(ss::CardToString(Card).c_str());
}

const TCHAR* MeaningWord(uint8 Means)
{
	switch (static_cast<EBackRoomTellMeaning>(Means))
	{
	case EBackRoomTellMeaning::Strong: return TEXT("strength");
	case EBackRoomTellMeaning::Weak: return TEXT("weakness");
	default: return TEXT("a bluff");
	}
}

/** What the sharper (or louder) regulars say when they catch the hero's hands shaking. */
FString ShakeLine(const FString& Name, bool bReadsWeak, FRandomStream& Fx)
{
	if (Name == TEXT("Mei"))
	{
		return bReadsWeak ? TEXT("Your hands are shaking. Call.") : TEXT("Hands like that? I'm out.");
	}
	if (Name == TEXT("Sal"))
	{
		return bReadsWeak ? TEXT("I've seen that shake before, kid. Call.") : TEXT("I know that shake. Take it.");
	}
	if (Name == TEXT("Big Lou"))
	{
		return Fx.FRand() < 0.5f ? TEXT("Look at those hands go! Lou calls!") : TEXT("You're shaking like a leaf. I'm in!");
	}
	return Fx.FRand() < 0.5f ? TEXT("You're nervous, man. I call.") : TEXT("Shaky shaky. Call.");
}

} // namespace BackRoomTableDetail

using namespace BackRoomTableDetail;

ABackRoomTable::ABackRoomTable()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Audio = CreateDefaultSubobject<UNightOneAudio>(TEXT("Audio"));
}

void ABackRoomTable::HookSounds(ABackRoomPlayer* Player)
{
	if (!Player)
	{
		return;
	}
	TWeakObjectPtr<UNightOneAudio> Out = Audio;
	Player->SoundHook = [Out](int32 Id, const FVector& At, float Volume) {
		if (UNightOneAudio* A = Out.Get())
		{
			A->PlayAt(static_cast<ss::SoundId>(Id), At, Volume);
		}
	};
}

ABackRoomTable::~ABackRoomTable() = default;

// ------------------------------------------------------------------ setup

FString ABackRoomTable::ReadKey(const ABackRoomPlayer* Player, uint8 Tell)
{
	const FString Name = Player ? Player->Persona.Name : TEXT("?");
	return Name + TEXT("/") + StaticEnum<EBackRoomTell>()->GetNameStringByValue(Tell);
}

FString ABackRoomTable::TellPhrase(uint8 Tell)
{
	switch (static_cast<EBackRoomTell>(Tell))
	{
	case EBackRoomTell::ChipGlance: return TEXT("the glance at the chips");
	case EBackRoomTell::BrowFlash: return TEXT("the eyebrow flash");
	case EBackRoomTell::Swallow: return TEXT("the dry swallow");
	case EBackRoomTell::LipPress: return TEXT("the pressed lips");
	case EBackRoomTell::Freeze: return TEXT("going still");
	case EBackRoomTell::StareDown: return TEXT("the stare");
	case EBackRoomTell::LookAway: return TEXT("looking away");
	case EBackRoomTell::Tremble: return TEXT("the shaking hands");
	case EBackRoomTell::NeckTouch: return TEXT("the hand to the neck");
	case EBackRoomTell::FalseSmile: return TEXT("the mouth-only smile");
	case EBackRoomTell::RealSmile: return TEXT("the smile that reaches the eyes");
	case EBackRoomTell::BlinkBurst: return TEXT("the blinking");
	case EBackRoomTell::Sigh: return TEXT("the big sigh");
	case EBackRoomTell::Recheck: return TEXT("checking the cards again");
	case EBackRoomTell::PupilFlare: return TEXT("the wide pupils");
	case EBackRoomTell::ChipReach: return TEXT("reaching for chips");
	default: return TEXT("that");
	}
}

void ABackRoomTable::AddPlayer(ABackRoomPlayer* Player, int32 TableSeat, ss::Archetype Archetype, int64 BuyIn)
{
	if (!Rng)
	{
		Rng = MakeUnique<ss::Rng>(TCHAR_TO_UTF8(*FString::Printf(TEXT("backroom-%lld"), FDateTime::Now().GetTicks())));
		Fx.Initialize(static_cast<int32>(FDateTime::Now().GetTicks() & 0x7fffffff));
	}
	FSeat& S = Seats.AddDefaulted_GetRef();
	S.TableSeat = TableSeat;
	S.HandSeat = TableSeat;
	S.Player = Player;
	S.bHero = Player && Player->SeatRole == EBackRoomRole::Hero;
	S.Id = Player ? Player->Persona.Name : FString::Printf(TEXT("Seat%d"), TableSeat);
	S.Stack = S.BuyIn = S.StartStack = BuyIn;
	S.Profile = ss::MakeProfile(Archetype, *Rng);
	const FTransform T = ABackRoomStage::SeatTransform(TableSeat);
	S.StackPile = NewPile(StackSpot(TableSeat), T.Rotator(), TableSeat * 17 + 3, static_cast<uint8>(EBackRoomChipStyle::Stack));
	S.StackPile->SetAmount(BuyIn);
	S.BetPile = NewPile(BetSpot(TableSeat), T.Rotator(), TableSeat * 29 + 5, static_cast<uint8>(EBackRoomChipStyle::Bet));
	HookSounds(Player);
	if (Player)
	{
		Player->StackPile = S.StackPile;
		Player->BetPile = S.BetPile;
		Player->Spots.Cards = CardSpot(TableSeat, 0).GetLocation();
		Player->Spots.Spread[0] = SpreadSpot(TableSeat, 0);
		Player->Spots.Spread[1] = SpreadSpot(TableSeat, 1);
		Player->Spots.Stack = StackSpot(TableSeat);
		Player->Spots.Bet = BetSpot(TableSeat);
		Player->Spots.Inward = T.GetRotation().GetForwardVector();
		if (!S.bHero)
		{
			Opponents.Add(Player);
			TWeakObjectPtr<ABackRoomTable> Self = this;
			Player->TellHook = [Self](ABackRoomPlayer* P, uint8 Tell, uint8 Means, bool bHonest, float Studied) {
				if (ABackRoomTable* T = Self.Get())
				{
					T->OnTellSeen(P, Tell, Means, bHonest, Studied);
				}
			};
		}
	}
	Seats.Sort([](const FSeat& A, const FSeat& B) { return A.TableSeat < B.TableSeat; });
}

void ABackRoomTable::SetDealer(ABackRoomPlayer* InDealer)
{
	Dealer = InDealer;
	HookSounds(Dealer);
}

void ABackRoomTable::Begin(float Delay)
{
	if (!Pot)
	{
		Pot = NewPile(PotSpot(), FRotator::ZeroRotator, 99, static_cast<uint8>(EBackRoomChipStyle::Pot));
	}
	bRunning = true;
	Wait = Delay;
	StartRoomTone();
}

void ABackRoomTable::StartRoomTone()
{
	if (Audio && bCardRoomTone && !CardTone)
	{
		CardTone = MakeShared<FCardRoomAmbience>(0xcafeu);
		TSharedPtr<FCardRoomAmbience> Tone = CardTone;
		Audio->StartAmbience([Tone](float* Out, int32 N) { Tone->Render(Out, N); }, 0.4f);
		return;
	}
	if (Audio && !RoomTone && !CardTone)
	{
		RoomTone = MakeShared<FBackRoomAmbience>(0x5eedu);
		TSharedPtr<FBackRoomAmbience> Tone = RoomTone;
		Audio->StartAmbience([Tone](float* Out, int32 N) { Tone->Render(Out, N); }, 0.35f);
	}
}

ABackRoomPlayer* ABackRoomTable::GetHeroPlayer() const
{
	const FSeat* S = HeroSeat();
	return S ? S->Player.Get() : nullptr;
}

ABackRoomTable::FSeat* ABackRoomTable::SeatAt(int32 HandSeat)
{
	return HandSeat < 0 ? nullptr : Seats.FindByPredicate([HandSeat](const FSeat& S) { return S.HandSeat == HandSeat; });
}

const ABackRoomTable::FSeat* ABackRoomTable::SeatAt(int32 HandSeat) const
{
	return HandSeat < 0 ? nullptr : Seats.FindByPredicate([HandSeat](const FSeat& S) { return S.HandSeat == HandSeat; });
}

ABackRoomTable::FSeat& ABackRoomTable::EnsureSeat(int32 TableSeat)
{
	if (FSeat* Found = Seats.FindByPredicate([TableSeat](const FSeat& S) { return S.TableSeat == TableSeat; }))
	{
		return *Found;
	}
	FSeat& S = Seats.AddDefaulted_GetRef();
	S.TableSeat = TableSeat;
	const FTransform T = ABackRoomStage::SeatTransform(TableSeat);
	S.StackPile = NewPile(StackSpot(TableSeat), T.Rotator(), TableSeat * 17 + 3, static_cast<uint8>(EBackRoomChipStyle::Stack));
	S.BetPile = NewPile(BetSpot(TableSeat), T.Rotator(), TableSeat * 29 + 5, static_cast<uint8>(EBackRoomChipStyle::Bet));
	Seats.Sort([](const FSeat& A, const FSeat& B) { return A.TableSeat < B.TableSeat; });
	return *Seats.FindByPredicate([TableSeat](const FSeat& X) { return X.TableSeat == TableSeat; });
}

void ABackRoomTable::WireSeat(FSeat& S)
{
	ABackRoomPlayer* Player = S.Player;
	if (!Player)
	{
		return;
	}
	const FTransform T = ABackRoomStage::SeatTransform(S.TableSeat);
	HookSounds(Player);
	Player->StackPile = S.StackPile;
	Player->BetPile = S.BetPile;
	Player->Spots.Cards = CardSpot(S.TableSeat, 0).GetLocation();
	Player->Spots.Spread[0] = SpreadSpot(S.TableSeat, 0);
	Player->Spots.Spread[1] = SpreadSpot(S.TableSeat, 1);
	Player->Spots.Stack = StackSpot(S.TableSeat);
	Player->Spots.Bet = BetSpot(S.TableSeat);
	Player->Spots.Inward = T.GetRotation().GetForwardVector();
	if (!S.bHero)
	{
		Opponents.AddUnique(Player);
		TWeakObjectPtr<ABackRoomTable> Self = this;
		Player->TellHook = [Self](ABackRoomPlayer* P, uint8 Tell, uint8 Means, bool bHonest, float Studied) {
			if (ABackRoomTable* Tb = Self.Get())
			{
				Tb->OnTellSeen(P, Tell, Means, bHonest, Studied);
			}
		};
	}
}

ABackRoomTable::FSeat* ABackRoomTable::HeroSeat()
{
	return Seats.FindByPredicate([](const FSeat& S) { return S.bHero; });
}

const ABackRoomTable::FSeat* ABackRoomTable::HeroSeat() const
{
	return Seats.FindByPredicate([](const FSeat& S) { return S.bHero; });
}

// ------------------------------------------------------------------ layout

FTransform ABackRoomTable::CardSpot(int32 TableSeat, int32 Index) const
{
	const FTransform T = ABackRoomStage::SeatTransform(TableSeat);
	// The way a player holds two cards: fanned, one over the other, the long side toward them. The first card
	// dealt lies in front (nearer, to the right) and the second drops onto it, back and to the left: a peek lifts
	// each card's near-left corner (the one with an index), and the front card's curl then lies outside the back
	// card's, so both indices show.
	const float Side = ABackRoomCard::NearIndexSide();
	const double Y = -2.95 + Side * (Index == 0 ? -1.3 : 1.3);
	const double X = RailGap + 25.0 + (Index == 0 ? -0.7 : 0.7);
	const FVector At = T.TransformPosition(FVector(X, Y, ABackRoomStage::FeltZ + (Index == 0 ? 0.0 : 0.12)));
	return FTransform(FRotator(0.0f, T.Rotator().Yaw + 90.0f + Side * (Index == 0 ? -4.0f : 5.0f), 0.0f), At);
}

FTransform ABackRoomTable::SpreadSpot(int32 TableSeat, int32 Index) const
{
	const FTransform T = ABackRoomStage::SeatTransform(TableSeat);
	const float Side = ABackRoomCard::NearIndexSide();
	const FVector At = T.TransformPosition(FVector(RailGap + 25.0, -2.95 + Side * (Index == 0 ? -3.55 : 3.55), ABackRoomStage::FeltZ + 0.02 * Index));
	return FTransform(FRotator(0.0f, T.Rotator().Yaw + 90.0f + Side * (Index == 0 ? -2.0f : 2.0f), 0.0f), At);
}

FVector ABackRoomTable::StackSpot(int32 TableSeat) const
{
	return ABackRoomStage::SeatTransform(TableSeat).TransformPosition(FVector(RailGap + 21.0, 17.0, ABackRoomStage::FeltZ));
}

FVector ABackRoomTable::BetSpot(int32 TableSeat) const
{
	return ABackRoomStage::SeatTransform(TableSeat).TransformPosition(FVector(RailGap + 41.0, 5.0, ABackRoomStage::FeltZ));
}

FTransform ABackRoomTable::BoardSpot(int32 Index) const
{
	// Across the middle, readable from the hero's seat.
	return FTransform(FRotator(0.0f, 90.0f, 0.0f), FVector(-2.0, (Index - 2) * 7.5, ABackRoomStage::FeltZ + 0.02));
}

FVector ABackRoomTable::PotSpot() const
{
	return FVector(24.0, 0.0, ABackRoomStage::FeltZ);
}

FVector ABackRoomTable::MuckSpot() const
{
	return FVector(40.0, -22.0, ABackRoomStage::FeltZ + 0.1);
}

ABackRoomCard* ABackRoomTable::NewCard(int32 Card)
{
	const FTransform At = Dealer ? Dealer->GetDeckTop() : FTransform(PotSpot());
	ABackRoomCard* C = GetWorld()->SpawnActor<ABackRoomCard>(ABackRoomCard::StaticClass(), At);
	C->SetCard(Card);
	C->SetActorHiddenInGame(true);
	Cards.Add(C);
	return C;
}

ABackRoomChips* ABackRoomTable::NewPile(const FVector& At, const FRotator& Facing, int32 Seed, uint8 Style)
{
	ABackRoomChips* P = GetWorld()->SpawnActor<ABackRoomChips>(ABackRoomChips::StaticClass(), FTransform(Facing, At));
	P->SetStyle(static_cast<EBackRoomChipStyle>(Style), Seed);
	return P;
}

void ABackRoomTable::DealerSays(const FString& Line)
{
	Lines.Add({Dealer ? Dealer->Persona.Name : TEXT("Dee"), Line, Time});
	if (Dealer)
	{
		Dealer->Say(Line);
		Dealer->TakeLines();
	}
}

TArray<FBackRoomLine> ABackRoomTable::TakeLines()
{
	TArray<FBackRoomLine> Out = MoveTemp(Lines);
	Lines.Reset();
	return Out;
}

// ------------------------------------------------------------------ the game loop

void ABackRoomTable::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	// The heart runs on real time (Focus slows the room, not you).
	UpdateComposure(FMath::Min(static_cast<float>(FApp::GetDeltaTime()), 0.1f));
	for (const FSeat& S : Seats)
	{
		if (S.Player)
		{
			for (const FString& L : S.Player->TakeLines())
			{
				Lines.Add({S.Player->Persona.Name, L, Time});
			}
		}
	}
	if (!bRunning)
	{
		return;
	}
	if (Wait > 0.0f)
	{
		Wait -= DeltaSeconds;
		return;
	}
	if (!Hand)
	{
		StartHand();
		return;
	}
	while (Cursor < Hand->Events.size())
	{
		const ss::HandEvent Ev = Hand->Events[Cursor++];
		Wait = Consume(Ev);
		if (Wait > 0.0f)
		{
			return;
		}
	}
	if (Hand->bComplete)
	{
		if (!bHandDone)
		{
			bHandDone = true;
			Wait = 1.0f;
			return;
		}
		EndHand();
		return;
	}
	const int32 Idx = Hand->ToAct;
	if (Idx < 0)
	{
		return;
	}
	const ss::HandSeat& HS = Hand->Seats[static_cast<size_t>(Idx)];
	FSeat* S = SeatAt(HS.Seat);
	if (!S)
	{
		return;
	}
	if (S->bHero)
	{
		if (!bHeroTurn)
		{
			OpenHeroTurn();
			AwayActAt = Time + 1.3f;
		}
		if (bHeroAway && Time >= AwayActAt)
		{
			// An empty seat: checked when it's free, mucked when it isn't.
			const ss::LegalActions L = Hand->GetLegalActions();
			if (!L.CanCheck && Time - AwayLineAt > 40.0f)
			{
				AwayLineAt = Time;
				DealerSays(TEXT("Player's away from the table. That hand's dead."));
			}
			HeroAct(static_cast<int32>(L.CanCheck ? ss::PlayerAction::Kind::Check : ss::PlayerAction::Kind::Fold), 0.0);
		}
		return;
	}
	if (!bBotPending)
	{
		const ss::PlayerView View = ss::MakeView(*Hand, Idx);
		ss::BotContext Ctx;
		Ctx.Prof = &S->Profile;
		Ctx.R = Rng.Get();
		const ss::BotDecision D = Tourney ? Tourney->BotDecisionFor(*Hand, false) : ss::Decide(View, Ctx);
		bBotPending = true;
		BotSeat = S->TableSeat;
		BotKind = static_cast<int32>(D.Action.Type);
		BotTo = D.Action.To;
		bBotBluff = D.Action.Type == ss::PlayerAction::Kind::Raise && D.Equity < 0.42;
		if (bHeroAggressedThisStreet && View.ToCall > 0)
		{
			ReadHero(*S, BotKind, BotTo, D.Equity);
		}
		// How long they sit with it: the engine's timing tell, slowed to a live table's pace.
		BotAt = Time + FMath::Clamp(static_cast<float>(D.ThinkMs) / 1000.0f * 0.6f * ThinkScale, FMath::Max(0.4f, 0.9f * ThinkScale), 6.5f * ThinkScale);
		if (S->Player)
		{
			S->Player->SetHandStrength(static_cast<float>(D.Equity), true);
			S->Player->BeginThink(View.ToCall, View.Pot, View.Seats[static_cast<size_t>(View.Me)].Stack);
		}
		return;
	}
	if (Time >= BotAt)
	{
		bBotPending = false;
		ss::PlayerAction A;
		A.Type = static_cast<ss::PlayerAction::Kind>(BotKind);
		A.To = BotTo;
		if (!Hand->Act(A))
		{
			// Never stall the table on a bad bot action.
			const ss::LegalActions L = Hand->GetLegalActions();
			Hand->Act(L.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Fold());
		}
	}
}

void ABackRoomTable::StartHand()
{
	if (bHolding)
	{
		return;
	}
	if (bLeaveRequested)
	{
		// Racked up: the hero is out of the game; the table waits for the next one.
		bHolding = true;
		if (!bLeftNoted)
		{
			bLeftNoted = true;
			if (OnNote)
			{
				OnNote(EBackRoomTableNote::HeroLeft);
			}
		}
		return;
	}
	if (const FSeat* H = HeroSeat(); H && H->Stack <= 0 && !bHeroAutoReload)
	{
		// Felted: the host decides (reload from the bankroll, or go home).
		bHolding = true;
		if (!bBustNoted)
		{
			bBustNoted = true;
			if (OnNote)
			{
				OnNote(EBackRoomTableNote::HeroBusted);
			}
		}
		return;
	}
	if (Tourney)
	{
		if (!PendingHand && !FetchTournamentHand())
		{
			return;
		}
		if (bHolding)
		{
			return;
		}
		SyncTournamentSeats(*PendingHand);
		Hand.Reset(PendingHand.Release());
		if (OnNote)
		{
			OnNote(EBackRoomTableNote::HandDealt);
		}
		for (FSeat& S : Seats)
		{
			S.StartStack = S.Stack;
			S.bDealt = false;
			S.bShown = false;
			S.Hole.Reset();
			if (S.Player && S.HandSeat >= 0)
			{
				S.Player->Hole.Reset();
				S.Player->BeginHand();
			}
		}
	}
	else
	{
	// Busted players buy back in (the Tuesday game is friendly like that).
	for (FSeat& S : Seats)
	{
		if (S.Stack <= 0)
		{
			S.Stack = S.BuyIn;
			S.StackPile->SetAmount(S.Stack);
			DealerSays(S.bHero ? FString::Printf(TEXT("You're back in for %lld. Easy, kid."), S.BuyIn) : FString::Printf(TEXT("%s's back in for %lld."), *S.Id, S.BuyIn));
		}
	}
	// The button moves one seat to the left.
	int32 Next = INDEX_NONE;
	for (int32 I = 0; I < Seats.Num(); ++I)
	{
		if (Seats[I].TableSeat > Button)
		{
			Next = I;
			break;
		}
	}
	Button = Seats[Next == INDEX_NONE ? 0 : Next].TableSeat;

	ss::HandConfig Config;
	for (FSeat& S : Seats)
	{
		ss::SeatInput In;
		In.Seat = S.TableSeat;
		In.Id = TCHAR_TO_UTF8(*S.Id);
		In.Stack = S.Stack;
		Config.Players.push_back(In);
		S.StartStack = S.Stack;
		S.bDealt = false;
		S.bShown = false;
		S.Hole.Reset();
		if (S.Player)
		{
			S.Player->Hole.Reset();
			S.Player->BeginHand();
		}
	}
	Config.ButtonSeat = Button;
	Config.SmallBlind = SmallBlind;
	Config.BigBlind = BigBlind;
	Hand = MakeUnique<ss::Hand>(Config, *Rng);
	}
	if (!Hand->IsValid())
	{
		Hand.Reset();
		bRunning = false;
		return;
	}
	Cursor = 0;
	bHandDone = false;
	bHeroTurn = false;
	bHeroPeeked = false;
	bBotPending = false;
	Board.Reset();
	++HandNumber;
	if (!Tourney || HandNumber == 1)
	{
		// (A tournament's hand clock runs from the end of the last hand: see EndHand.)
		HandStartedAt = Time;
	}
	if (const FSeat* H = HeroSeat())
	{
		HeroStartOfHand = H->Stack;
	}
	Composure.Bluff = 0.0f;
	HeroShakeAtBet = 0.0f;
	bHeroAggressedThisStreet = false;
	Sightings.Reset();
	WhisperedThisHand.Reset();

	// Dee's lessons, a few hands apart (the first night only).
	switch (bFirstVisit ? HandNumber : 0)
	{
	case 1: DealerSays(TEXT("Tuesday game, kid. One-two, no limit. Hold space to look at your cards. Keep 'em on the felt.")); break;
	case 2: DealerSays(TEXT("Right mouse, you can study a face. Don't stare too long. They notice.")); break;
	case 4: DealerSays(TEXT("Watch where their eyes go when the flop comes. Not the cards. The eyes.")); break;
	case 6: DealerSays(TEXT("Some hands shake when they've got it. Adrenaline don't lie.")); break;
	case 9: DealerSays(TEXT("And a pro acts. Weak means strong, strong means weak. Mostly.")); break;
	default: break;
	}
}

float ABackRoomTable::Consume(const ss::HandEvent& Ev)
{
	switch (Ev.Type)
	{
	case ss::EventType::Blind:
	{
		FSeat* S = SeatAt(Ev.Seat);
		if (S && S->Player)
		{
			S->Player->GestureChips(Ev.Amount, false, false);
		}
		return 0.45f;
	}
	case ss::EventType::Deal:
	{
		TArray<int32> Order;
		for (int Seat : Ev.Seats)
		{
			Order.Add(Seat);
		}
		return DealHoles(Order);
	}
	case ss::EventType::Action:
	{
		const ss::ActionRecord& A = Ev.Action;
		FSeat* S = SeatAt(A.Seat);
		if (!S || !S->Player)
		{
			return 0.3f;
		}
		ABackRoomPlayer* P = S->Player;
		const bool bAggressive = A.Type == ss::ActionType::Bet || A.Type == ss::ActionType::Raise;
		const bool bBluff = !S->bHero && S->TableSeat == BotSeat && bAggressive && bBotBluff;
		float Took = 0.4f;
		switch (A.Type)
		{
		case ss::ActionType::Fold: Took = P->GestureFold(MuckSpot()); break;
		case ss::ActionType::Check: Took = P->GestureCheck(); break;
		default: Took = P->GestureChips(A.Added, A.AllIn, bBluff); break;
		}
		if (S->bHero)
		{
			for (ABackRoomPlayer* O : Opponents)
			{
				O->OnHeroThinking(false, 0);
			}
			if (bAggressive)
			{
				// The bet is out there now, with the hands that pushed it: whatever the heart was doing shows.
				Composure.Bluff = FeltEquity(S->HandSeat) < 0.42f ? 1.0f : 0.0f;
				HeroShakeAtBet = Composure.Shake();
				bHeroAggressedThisStreet = true;
			}
		}
		else
		{
			P->OnActed(bAggressive, bBluff);
			if (bAggressive)
			{
				bHeroAggressedThisStreet = false;
			}
		}
		if (A.Type == ss::ActionType::Fold)
		{
			P->SetHandStrength(0.0f, false);
		}
		// Everyone else looks at whoever acted; a raise draws more of them, for longer.
		const float Aggression = bAggressive ? 1.0f : (A.Type == ss::ActionType::Call ? 0.4f : 0.1f);
		for (const FSeat& Other : Seats)
		{
			if (Other.Player && Other.Player != P)
			{
				Other.Player->OnOtherAction(P->GetEyes(), Aggression, S->bHero);
			}
		}
		if (Dealer)
		{
			Dealer->OnOtherAction(P->GetEyes(), Aggression, S->bHero);
		}
		if (A.AllIn && bAggressive)
		{
			DealerSays(S->bHero ? TEXT("All in.") : FString::Printf(TEXT("%s is all in."), *S->Id));
		}
		return Took * 0.85f;
	}
	case ss::EventType::Street:
	{
		bHeroAggressedThisStreet = false;
		TArray<int32> New;
		for (int C : Ev.Cards)
		{
			New.Add(C);
		}
		const float Gather = GatherBets();
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, New]() { DealStreet(New); }), Gather + 0.05f, false);
		// Dealing takes about half a second a card, then the table takes it in.
		return Gather + 0.55f * (New.Num() + 1) + 1.2f;
	}
	case ss::EventType::Return:
	{
		FSeat* S = SeatAt(Ev.Seat);
		if (S && S->BetPile)
		{
			ABackRoomChips* Bet = S->BetPile;
			const int64 Amount = Ev.Amount;
			ABackRoomChips* Back = NewPile(Bet->GetActorLocation(), Bet->GetActorRotation(), 7, static_cast<uint8>(EBackRoomChipStyle::Bet));
			Back->SetAmount(Amount);
			Bet->SetAmount(FMath::Max<int64>(0, Bet->GetAmount() - Amount));
			Back->SlideTo(StackSpot(S->TableSeat), 0.5f);
			ABackRoomChips* Stack = S->StackPile;
			FTimerHandle Handle;
			GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [Back, Stack, Amount]() {
				Stack->SetAmount(Stack->GetAmount() + Amount);
				Back->Destroy();
			}), 0.55f, false);
		}
		return 0.6f;
	}
	case ss::EventType::Reveal:
	{
		float T = 0.0f;
		for (int Seat : Ev.Seats)
		{
			FSeat* S = SeatAt(Seat);
			if (S && S->Player && !S->bShown)
			{
				S->bShown = true;
				TWeakObjectPtr<ABackRoomPlayer> P = S->Player.Get();
				FTimerHandle Handle;
				GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, P]() {
					if (ABackRoomPlayer* Shown = P.Get())
					{
						Shown->GestureShow();
						ConfirmSightings(Shown);
					}
				}), FMath::Max(T, 0.01f), false);
				T += 0.35f;
			}
		}
		return T + 1.4f;
	}
	case ss::EventType::Showdown:
	{
		float T = 0.0f;
		for (const ss::ShowdownHand& H : Ev.Hands)
		{
			FSeat* S = SeatAt(H.Seat);
			if (S && S->Player && !S->bShown)
			{
				S->bShown = true;
				TWeakObjectPtr<ABackRoomPlayer> P = S->Player.Get();
				FTimerHandle Handle;
				GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, P]() {
					if (ABackRoomPlayer* Shown = P.Get())
					{
						Shown->GestureShow();
						ConfirmSightings(Shown);
					}
				}), FMath::Max(T, 0.01f), false);
				T += 1.1f;
			}
		}
		return T + 1.2f;
	}
	case ss::EventType::Award:
	{
		// Everyone folded to a bet: the bets were never pulled in. Pull them in first.
		const float Gather = GatherBets();
		if (Gather > 0.0f)
		{
			const ss::HandEvent Later = Ev;
			FTimerHandle Handle;
			GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, Later]() { Consume(Later); }), Gather, false);
			return Gather + 1.4f;
		}
		const ss::PotResult& R = Ev.Pot;
		for (size_t W = 0; W < R.Winners.size(); ++W)
		{
			FSeat* S = SeatAt(R.Winners[W]);
			if (!S || !Pot)
			{
				continue;
			}
			const int64 Share = R.Shares[W];
			ABackRoomChips* Push = NewPile(Pot->GetActorLocation(), FRotator::ZeroRotator, 31 + static_cast<int32>(W), static_cast<uint8>(EBackRoomChipStyle::Pot));
			Push->SetAmount(Share);
			Pot->SetAmount(FMath::Max<int64>(0, Pot->GetAmount() - Share));
			ABackRoomChips* Stack = S->StackPile;
			auto Land = [Push, Stack, Share]() {
				Stack->SetAmount(Stack->GetAmount() + Share);
				Push->Destroy();
			};
			if (Dealer)
			{
				Dealer->GestureSweep(Push, StackSpot(S->TableSeat), Land);
			}
			else
			{
				Push->SlideTo(StackSpot(S->TableSeat), 0.6f);
				FTimerHandle Handle;
				GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, Land), 0.65f, false);
			}
			if (S->bHero && R.Amount >= 20 * BigBlindNow())
			{
				DealerSays(Tourney ? TEXT("Pot to you, kid.") : TEXT("Ship it to the kid."));
			}
		}
		return 1.4f;
	}
	case ss::EventType::End:
		return 0.0f;
	default:
		return 0.0f;
	}
}

float ABackRoomTable::DealHoles(const TArray<int32>& Order)
{
	float T = 0.0f;
	for (int32 Round = 0; Round < 2; ++Round)
	{
		for (int32 Seat : Order)
		{
			FSeat* S = SeatAt(Seat);
			const ss::HandSeat* HS = Hand->SeatByNumber(Seat);
			if (!S || !HS || HS->Hole.size() < 2)
			{
				continue;
			}
			ABackRoomCard* C = NewCard(HS->Hole[static_cast<size_t>(Round)]);
			S->Hole.Add(C);
			if (S->Player)
			{
				S->Player->Hole.Add(C);
			}
			T += Dealer ? Dealer->GestureDeal(C, CardSpot(S->TableSeat, Round), false) : 0.3f;
			if (!Dealer)
			{
				C->SetActorHiddenInGame(false);
				C->PitchTo(CardSpot(S->TableSeat, Round), 0.3f, 3.0f);
			}
		}
	}
	// Each opponent looks at their cards in their own time once they've landed, and feels it.
	UpdateStrengths(false);
	for (FSeat& S : Seats)
	{
		S.bDealt = true;
		if (S.bHero || !S.Player)
		{
			continue;
		}
		ABackRoomPlayer* P = S.Player;
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(P, [P]() { P->PeekHole(); }), T + Fx.FRandRange(0.4f, 2.8f), false);
	}
	return T + 0.6f;
}

float ABackRoomTable::GatherBets()
{
	float T = 0.0f;
	for (FSeat& S : Seats)
	{
		ABackRoomChips* Bet = S.BetPile;
		if (!Bet || Bet->GetAmount() <= 0 || !Pot)
		{
			continue;
		}
		const FVector Home = BetSpot(S.TableSeat);
		ABackRoomChips* PotPile = Pot;
		auto Merge = [Bet, PotPile, Home]() {
			PotPile->SetAmount(PotPile->GetAmount() + Bet->GetAmount());
			Bet->SetAmount(0);
			Bet->SetActorLocation(Home);
		};
		if (Dealer)
		{
			// Sweeps queue on Dee's hand: the last one says when they're all done.
			T = FMath::Max(T, Dealer->GestureSweep(Bet, PotSpot(), Merge));
		}
		else
		{
			Merge();
		}
	}
	return T > 0.0f ? T + 0.1f : 0.0f;
}

float ABackRoomTable::DealStreet(const TArray<int32>& New)
{
	// Burn one, then turn them.
	float T = 0.0f;
	if (Dealer)
	{
		ABackRoomCard* Burn = NewCard(-1);
		T += Dealer->GestureDeal(Burn, FTransform(FRotator(0.0f, 70.0f, 0.0f), MuckSpot() + FVector(4.0, 6.0, 0.0)), false);
	}
	for (int32 C : New)
	{
		ABackRoomCard* Card = NewCard(C);
		const int32 Index = Board.Num();
		Board.Add(Card);
		if (Dealer)
		{
			T += Dealer->GestureDeal(Card, BoardSpot(Index), true);
		}
		else
		{
			Card->SetActorHiddenInGame(false);
			Card->PitchTo(BoardSpot(Index), 0.3f, 3.0f, 0.0f, true);
		}
	}
	// The table reacts once the cards are face up.
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]() { UpdateStrengths(true); }), T + 0.7f, false);
	return T;
}

float ABackRoomTable::FeltEquity(int32 HandSeat)
{
	const ss::HandSeat* Me = Hand ? Hand->SeatByNumber(HandSeat) : nullptr;
	if (!Me || Me->Hole.size() < 2)
	{
		return 0.5f;
	}
	std::vector<int> Visible;
	for (const ABackRoomCard* C : Board)
	{
		if (C && C->GetCard() >= 0)
		{
			Visible.push_back(C->GetCard());
		}
	}
	int32 InHand = 0;
	for (const ss::HandSeat& HS : Hand->Seats)
	{
		InHand += HS.Folded ? 0 : 1;
	}
	const std::vector<ss::RangeBand> Ranges(static_cast<size_t>(FMath::Max(1, InHand - 1)), ss::RangeBand{0.0, 1.0});
	const float Eq = static_cast<float>(ss::EquityVsRanges(Me->Hole, Visible, Ranges, 300, *Rng));
	return FMath::Clamp(Eq * (0.6f + 0.4f * FMath::Max(1, InHand - 1)), 0.0f, 1.0f);
}

void ABackRoomTable::UpdateStrengths(bool bBoardChanged)
{
	if (!Hand)
	{
		return;
	}
	std::vector<int> Visible;
	for (const ABackRoomCard* C : Board)
	{
		if (C && C->GetCard() >= 0)
		{
			Visible.push_back(C->GetCard());
		}
	}
	int32 InHand = 0;
	for (const ss::HandSeat& HS : Hand->Seats)
	{
		InHand += HS.Folded ? 0 : 1;
	}
	for (const ss::HandSeat& HS : Hand->Seats)
	{
		FSeat* S = SeatAt(HS.Seat);
		if (!S || !S->Player || S->bHero || HS.Folded || HS.Hole.size() < 2)
		{
			continue;
		}
		const std::vector<ss::RangeBand> Ranges(static_cast<size_t>(FMath::Max(1, InHand - 1)), ss::RangeBand{0.0, 1.0});
		const float Eq = static_cast<float>(ss::EquityVsRanges(HS.Hole, Visible, Ranges, 400, *Rng));
		// Against several opponents a hand's raw equity runs low; what a player feels is closer to
		// how it stands against one.
		const float Felt = FMath::Clamp(Eq * (0.6f + 0.4f * FMath::Max(1, InHand - 1)), 0.0f, 1.0f);
		if (bBoardChanged)
		{
			S->Player->OnBoard(Felt);
		}
		else
		{
			S->Player->SetHandStrength(Felt, true);
		}
	}
}

void ABackRoomTable::EndHand()
{
	bool bShowdown = false;
	for (const ss::HandEvent& Ev : Hand->Events)
	{
		bShowdown = bShowdown || Ev.Type == ss::EventType::Showdown;
	}
	int64 PotSize = 0;
	for (const ss::PotResult& R : Hand->PotResults)
	{
		PotSize += R.Amount;
	}
	for (FSeat& S : Seats)
	{
		const ss::HandSeat* HS = S.HandSeat >= 0 ? Hand->SeatByNumber(S.HandSeat) : nullptr;
		if (!HS)
		{
			continue;
		}
		S.Stack = HS->Stack;
		if (S.Player)
		{
			S.Player->OnResult(S.Stack - S.StartStack, PotSize, bShowdown && !HS->Folded);
		}
		// Make sure the piles say what the engine says.
		S.StackPile->SetAmount(S.Stack);
		S.BetPile->SetAmount(0);
	}
	if (Pot)
	{
		Pot->SetAmount(0);
	}
	if (const FSeat* H = HeroSeat())
	{
		// A loss that stings stays with you for a while; a win settles you.
		const int64 Delta = H->Stack - HeroStartOfHand;
		if (Delta < -20)
		{
			const float Hurt = static_cast<float>(-Delta) / static_cast<float>(FMath::Max<int64>(HeroStartOfHand, 1));
			Composure.Tilt = FMath::Min(1.0f, Composure.Tilt + FMath::Clamp(Hurt * 1.2f, 0.15f, 0.9f) * (bShowdown ? 1.0f : 0.6f));
		}
		else if (Delta > 20)
		{
			Composure.Tilt *= 0.5f;
		}
		if (H->Stack <= 0 && !bHeroAutoReload && !Tourney)
		{
			DealerSays(TEXT("That's the felt, kid."));
		}
	}
	Sightings.Reset();
	// Dee gathers the cards in; then they go back in the deck.
	TArray<ABackRoomCard*> All;
	for (ABackRoomCard* C : Cards)
	{
		if (C)
		{
			All.Add(C);
		}
	}
	float T = Dealer ? Dealer->GestureCollect(All, MuckSpot()) : 0.0f;
	TArray<TWeakObjectPtr<ABackRoomCard>> Doomed;
	for (ABackRoomCard* C : All)
	{
		Doomed.Add(C);
	}
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [Doomed]() {
		for (const TWeakObjectPtr<ABackRoomCard>& C : Doomed)
		{
			if (C.IsValid())
			{
				C->Destroy();
			}
		}
	}), T + 0.4f, false);
	Cards.Reset();
	Board.Reset();
	for (FSeat& S : Seats)
	{
		S.Hole.Reset();
		if (S.Player)
		{
			S.Player->Hole.Reset();
		}
	}
	if (Tourney)
	{
		// The rest of the room plays its hand; eliminations, the clock and the levels follow.
		for (const ss::TEvent& E : Tourney->FinishTick(Hand.Get()))
		{
			Events.Add(E);
		}
		HandStartedAt = Time;
	}
	Hand.Reset();
	bHeroTurn = false;
	Wait = T + 2.2f;
	if (OnNote)
	{
		OnNote(EBackRoomTableNote::HandEnded);
	}
}

// ------------------------------------------------------------------ the tournament

void ABackRoomTable::SetTournament(TSharedPtr<ss::Tournament> InTourney)
{
	Tourney = InTourney;
	bHeroAutoReload = false;
}

TArray<ss::TEvent> ABackRoomTable::TakeEvents()
{
	TArray<ss::TEvent> Out = MoveTemp(Events);
	Events.Reset();
	return Out;
}

int64 ABackRoomTable::BigBlindNow() const
{
	if (Hand)
	{
		return Hand->BigBlind;
	}
	return Tourney ? Tourney->CurrentLevel().Bb : BigBlind;
}

bool ABackRoomTable::IsHeroInHand() const
{
	const FSeat* H = HeroSeat();
	const ss::HandSeat* HS = (H && Hand && !Hand->bComplete && H->HandSeat >= 0) ? Hand->SeatByNumber(H->HandSeat) : nullptr;
	return HS && !HS->Folded;
}

ABackRoomPlayer* ABackRoomTable::PlayerById(const FString& Id) const
{
	const FSeat* S = Seats.FindByPredicate([&Id](const FSeat& X) { return X.HandSeat >= 0 && X.Id == Id; });
	return S ? S->Player.Get() : nullptr;
}

void ABackRoomTable::SetCrowd(float Crowd)
{
	if (CardTone)
	{
		CardTone->SetCrowd(Crowd);
	}
}

void ABackRoomTable::PrepareNext()
{
	if (!Tourney || Hand)
	{
		return;
	}
	if (!PendingHand)
	{
		FetchTournamentHand();
	}
	if (PendingHand)
	{
		SyncTournamentSeats(*PendingHand);
		bHolding = false;
	}
}

void ABackRoomTable::Resume(float Delay)
{
	bHolding = false;
	Wait = FMath::Max(Wait, Delay);
	bRunning = true;
}

bool ABackRoomTable::FetchTournamentHand()
{
	std::vector<ss::TEvent> Ev;
	std::unique_ptr<ss::Hand> H = Tourney->StartTick(Ev);
	bool bHeroMoved = false;
	for (const ss::TEvent& E : Ev)
	{
		Events.Add(E);
		bHeroMoved = bHeroMoved || (E.Type == ss::TEventType::Moved && E.IsHero);
	}
	if (!H)
	{
		// Out (or the hero is away): nothing to deal; the host plays out the rest of the night.
		const ss::TPlayer& Me = Tourney->Hero();
		int32 AtMine = 0;
		if (Tourney->Tables.count(Me.TableId) > 0)
		{
			for (int32 Idx : Tourney->Tables.at(Me.TableId).Seats)
			{
				AtMine += Idx >= 0 ? 1 : 0;
			}
		}
		UE_CLOG(!Me.Busted && !Tourney->bFinished && !Tourney->HeroAway, LogTemp, Warning, TEXT("Tournament hand: none for a player still in (busted %d, finished %d, away %d, table %d with %d seated, %d tables, tick %d)"), Me.Busted ? 1 : 0,
			Tourney->bFinished ? 1 : 0, Tourney->HeroAway ? 1 : 0, Me.TableId, AtMine, static_cast<int32>(Tourney->Tables.size()), Tourney->Tick);
		bHolding = true;
		if (!bTournamentOverNoted && OnNote)
		{
			bTournamentOverNoted = true;
			OnNote(EBackRoomTableNote::TournamentOver);
		}
		return false;
	}
	PendingHand.Reset(H.release());
	if (bHeroMoved)
	{
		// The host walks the hero to the new table first.
		bHolding = true;
		if (OnNote)
		{
			OnNote(EBackRoomTableNote::HeroMoved);
		}
	}
	return true;
}

void ABackRoomTable::SyncTournamentSeats(const ss::Hand& H)
{
	// Six-max around the hero: the engine's seats in order from the hero's, on the table's seats with
	// the dealer's (4) and one at the hero's left (7) left out.
	static const int32 Physical[6] = {0, 1, 2, 3, 5, 6};
	const int32 Size = FMath::Max(1, Tourney->TableSize);
	const int32 HeroSeatNo = Tourney->Hero().Seat;
	TMap<int32, const ss::HandSeat*> Want;
	for (const ss::HandSeat& HS : H.Seats)
	{
		const int32 Rel = ((HS.Seat - HeroSeatNo) % Size + Size) % Size;
		Want.Add(Physical[FMath::Clamp(Rel, 0, 5)], &HS);
	}
	// Whoever isn't sitting here this hand leaves (busted, or moved to another table).
	for (FSeat& S : Seats)
	{
		const ss::HandSeat* const* W = Want.Find(S.TableSeat);
		const FString WantId = W ? FString(UTF8_TO_TCHAR((*W)->Id.c_str())) : FString();
		if (S.bHero || S.Id == WantId)
		{
			continue;
		}
		if (S.Player)
		{
			const int32 Idx = Tourney->PlayerIndex(std::string(TCHAR_TO_UTF8(*S.Id)));
			const bool bBusted = Idx >= 0 && Tourney->Players[static_cast<size_t>(Idx)].Busted;
			Opponents.Remove(S.Player);
			if (OnUnseatPlayer)
			{
				OnUnseatPlayer(S.Player, S.Id, bBusted);
			}
		}
		S.Player = nullptr;
		S.Id.Empty();
		S.HandSeat = -1;
		S.Stack = 0;
		S.StackPile->SetAmount(0);
		S.BetPile->SetAmount(0);
	}
	// And whoever is sits down (or stays).
	for (const TPair<int32, const ss::HandSeat*>& It : Want)
	{
		const ss::HandSeat& HS = *It.Value;
		const FString Id = UTF8_TO_TCHAR(HS.Id.c_str());
		FSeat& S = Id == UTF8_TO_TCHAR(ss::HeroId) ? *HeroSeat() : EnsureSeat(It.Key);
		S.HandSeat = HS.Seat;
		S.Stack = HS.Stack;
		if (!S.bHero && S.Id != Id)
		{
			const int32 Idx = Tourney->PlayerIndex(HS.Id);
			S.Id = Id;
			S.Player = Idx >= 0 && OnSeatPlayer ? OnSeatPlayer(Tourney->Players[static_cast<size_t>(Idx)], S.TableSeat) : nullptr;
			WireSeat(S);
		}
		S.StackPile->SetAmount(S.Stack);
		S.BetPile->SetAmount(0);
	}
}

// ------------------------------------------------------------------ the night

void ABackRoomTable::RequestLeave(bool bInLastHand)
{
	bLastHand = bLastHand || bInLastHand;
	if (bLeaveRequested)
	{
		return;
	}
	bLeaveRequested = true;
	if (bHolding && !bLeftNoted)
	{
		// Busted and going home: nothing to wait for.
		bLeftNoted = true;
		if (OnNote)
		{
			OnNote(EBackRoomTableNote::HeroLeft);
		}
	}
}

void ABackRoomTable::HeroReload(int64 Chips)
{
	FSeat* H = HeroSeat();
	if (!H || Chips <= 0)
	{
		return;
	}
	H->Stack = H->BuyIn = Chips;
	bHolding = false;
	bBustNoted = false;
	Wait = 2.4f;
	DealerSays(FString::Printf(TEXT("Back in for %lld. Breathe, kid."), Chips));
	// Dee counts out a fresh stack and pushes it across.
	ABackRoomChips* Rack = NewPile(PotSpot() + FVector(26.0, 0.0, 0.0), FRotator::ZeroRotator, 77, static_cast<uint8>(EBackRoomChipStyle::Stack));
	Rack->SetAmount(Chips);
	// Held weakly: the sweep lands later, and either pile may be gone by then.
	const TWeakObjectPtr<ABackRoomChips> Stack = H->StackPile;
	const TWeakObjectPtr<ABackRoomChips> RackPile = Rack;
	auto Land = [RackPile, Stack, Chips]() {
		if (ABackRoomChips* S = Stack.Get())
		{
			S->SetAmount(Chips);
		}
		if (ABackRoomChips* Pile = RackPile.Get())
		{
			Pile->Destroy();
		}
	};
	if (Dealer)
	{
		Dealer->GestureSweep(Rack, StackSpot(H->TableSeat), Land);
	}
	else
	{
		Rack->SlideTo(StackSpot(H->TableSeat), 0.6f);
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, Land), 0.65f, false);
	}
}

int64 ABackRoomTable::GetHeroStack() const
{
	const FSeat* H = HeroSeat();
	return H ? H->Stack : 0;
}

void ABackRoomTable::UpdateComposure(float Dt)
{
	FBackRoomComposure& C = Composure;
	const FSeat* H = HeroSeat();
	const ss::HandSeat* HS = (H && Hand && !Hand->bComplete && H->HandSeat >= 0) ? Hand->SeatByNumber(H->HandSeat) : nullptr;
	const bool bIn = HS && !HS->Folded && H->Hole.Num() == 2;
	float Pressure = 0.0f;
	if (bIn)
	{
		const float Stack = static_cast<float>(FMath::Max<int64>(HeroStartOfHand, 1));
		const float InPot = static_cast<float>(Hand->Pot());
		Pressure += 6.0f;
		Pressure += 30.0f * FMath::Min(InPot / Stack, 1.5f) / 1.5f;
		if (bHeroTurn)
		{
			const ss::LegalActions L = Hand->GetLegalActions();
			const float Price = static_cast<float>(L.CallAmount) / static_cast<float>(FMath::Max<int64>(HS->Stack, 1));
			Pressure += 22.0f * FMath::Clamp(Price * 2.0f, 0.0f, 1.0f);
			Pressure += L.CallAmount >= HS->Stack && L.CallAmount > 0 ? 28.0f : 0.0f;
		}
		Pressure += HS->AllIn ? 28.0f : 0.0f;
		Pressure += 18.0f * C.Bluff;
	}
	for (const ABackRoomPlayer* O : Opponents)
	{
		if (O && O->IsWatchingHero())
		{
			Pressure += 8.0f;
			break;
		}
	}
	Pressure += 20.0f * C.Tilt;
	Pressure += ExtraPressure * (bIn ? 1.0f : 0.5f);
	C.Target = FMath::Clamp(66.0f + C.Sensitivity * Pressure - (C.bSteadying ? 16.0f : 0.0f), 56.0f, 168.0f);
	// The heart jumps fast and settles slowly; steady breathing brings it down quicker.
	const float Rate = C.Target > C.Bpm ? 0.55f : (C.bSteadying ? 0.32f : 0.12f);
	C.Bpm = FMath::Lerp(C.Bpm, C.Target, 1.0f - FMath::Exp(-Rate * Dt));
	C.Tilt = FMath::Max(0.0f, C.Tilt - Dt / 240.0f);
	if (ABackRoomPlayer* Me = H ? H->Player.Get() : nullptr)
	{
		Me->SetExternalTremble(0.3f * C.Shake());
	}
}

void ABackRoomTable::ReadHero(const FSeat& Reader, int32& Kind, double& To, double Equity)
{
	const ABackRoomPlayer* P = Reader.Player;
	const float R = P ? P->Persona.HeroRead : 0.0f;
	if (FMath::Abs(R) < 0.05f || HeroShakeAtBet < 0.08f)
	{
		return;
	}
	// Did they see it? Shakier hands and sharper eyes, more often.
	if (Fx.FRand() > FMath::Clamp(HeroShakeAtBet * FMath::Abs(R) * 1.4f, 0.0f, 0.9f))
	{
		return;
	}
	const bool bHeroBluffing = Composure.Bluff > 0.5f;
	// The sharp ones read it for what it is; the rest think nerves mean a bluff.
	const bool bReadsWeak = R > 0.0f ? bHeroBluffing : true;
	const int32 Fold = static_cast<int32>(ss::PlayerAction::Kind::Fold);
	const int32 Call = static_cast<int32>(ss::PlayerAction::Kind::Call);
	bool bChanged = false;
	if (bReadsWeak && Kind == Fold && Equity > 0.12)
	{
		Kind = Call;
		To = 0.0;
		bChanged = true;
	}
	else if (!bReadsWeak && Kind == Call && Equity < 0.6)
	{
		Kind = Fold;
		To = 0.0;
		bChanged = true;
	}
	if (!bChanged)
	{
		return;
	}
	UE_LOG(LogTemp, Display, TEXT("BackRoom: %s read the hero's hands (shake %.2f, %s) -> %s"), *P->Persona.Name, HeroShakeAtBet,
		bHeroBluffing ? TEXT("bluffing") : TEXT("value"), Kind == Call ? TEXT("call") : TEXT("fold"));
	if (Reader.Player && Fx.FRand() < 0.7f)
	{
		Reader.Player->Say(ShakeLine(P->Persona.Name, bReadsWeak, Fx));
	}
	Whisper(FString::Printf(TEXT("%s caught your hands shaking."), *P->Persona.Name));
}

// ------------------------------------------------------------------ the read book

void ABackRoomTable::Whisper(const FString& Text)
{
	FBackRoomLine L;
	L.Text = Text;
	L.At = Time;
	L.bRead = true;
	Lines.Add(L);
	UE_LOG(LogTemp, Display, TEXT("BackRoom read: %s"), *Text);
}

void ABackRoomTable::OnTellSeen(ABackRoomPlayer* Player, uint8 Tell, uint8 Means, bool bHonest, float Studied)
{
	if (!Player || !Hand)
	{
		return;
	}
	// The pupils only show up close, through Focus.
	const bool bPupils = static_cast<EBackRoomTell>(Tell) == EBackRoomTell::PupilFlare;
	const float Need = bPupils ? 0.8f : 0.45f;
	const FString Key = ReadKey(Player, Tell);
	const int32 Known = Reads.FindRef(Key);
	const FString& Name = Player->Persona.Name;
	if (Known >= 2 && Studied >= (bPupils ? 0.7f : 0.2f))
	{
		// A read you own: the eye catches it without trying.
		if (!WhisperedThisHand.Contains(Key))
		{
			WhisperedThisHand.Add(Key);
			Whisper(FString::Printf(TEXT("%s: %s. That's %s."), *Name, *TellPhrase(Tell), MeaningWord(Means)));
		}
		return;
	}
	if (Studied < Need || WhisperedThisHand.Contains(Key))
	{
		return;
	}
	WhisperedThisHand.Add(Key);
	FSighting& S = Sightings.AddDefaulted_GetRef();
	S.Player = Player;
	S.Key = Key;
	S.Tell = Tell;
	S.Means = Means;
	S.bHonest = bHonest;
	Whisper(FString::Printf(TEXT("%s: %s."), *Name, *TellPhrase(Tell)));
}

void ABackRoomTable::ConfirmSightings(const ABackRoomPlayer* Shown)
{
	for (int32 I = Sightings.Num() - 1; I >= 0; --I)
	{
		const FSighting S = Sightings[I];
		if (S.Player.Get() != Shown)
		{
			continue;
		}
		Sightings.RemoveAt(I);
		const FString& Name = Shown->Persona.Name;
		if (!S.bHonest)
		{
			Whisper(FString::Printf(TEXT("%s's cards say %s meant nothing this time."), *Name, *TellPhrase(S.Tell)));
			continue;
		}
		int32& Count = Reads.FindOrAdd(S.Key);
		const int32 Before = Count;
		Count += FMath::Max(1, ReadWeight);
		if (Before < 2 && Count >= 2)
		{
			++LearnedThisNight;
			Whisper(FString::Printf(TEXT("READ LEARNED  %s: %s means %s."), *Name, *TellPhrase(S.Tell), MeaningWord(S.Means)));
			if (Audio)
			{
				Audio->Play(ss::SoundId::Level, 0.45f);
			}
		}
		else if (Count < 2)
		{
			Whisper(FString::Printf(TEXT("%s showed down after %s: %s. See it again to be sure."), *Name, *TellPhrase(S.Tell), MeaningWord(S.Means)));
		}
	}
}

// ------------------------------------------------------------------ the hero

void ABackRoomTable::OpenHeroTurn()
{
	bHeroTurn = true;
	const ss::LegalActions L = Hand->GetLegalActions();
	const int64 Bb = BigBlindNow();
	int64 Want = L.MinRaiseTo;
	if (Hand->CurrentStreet == ss::Street::Preflop)
	{
		Want = FMath::Max<int64>(L.MinRaiseTo, Hand->CurrentBet * 3);
	}
	else if (L.IsBet)
	{
		Want = FMath::Max<int64>(L.MinRaiseTo, (Hand->Pot() * 2 / 3 / Bb) * Bb);
	}
	else
	{
		Want = FMath::Max<int64>(L.MinRaiseTo, Hand->CurrentBet * 3);
	}
	HeroRaiseTo = FMath::Clamp<int64>(Want, L.MinRaiseTo, FMath::Max(L.MinRaiseTo, L.MaxRaiseTo));
	for (ABackRoomPlayer* O : Opponents)
	{
		O->OnHeroThinking(true, L.CallAmount);
	}
}

FBackRoomPrompt ABackRoomTable::GetPrompt() const
{
	FBackRoomPrompt P;
	P.HandNumber = HandNumber;
	P.BigBlind = BigBlindNow();
	const FSeat* Hero = HeroSeat();
	if (Hero)
	{
		P.Stack = Hero->StackPile ? Hero->StackPile->GetAmount() : Hero->Stack;
		if (bHeroPeeked && Hand)
		{
			if (const ss::HandSeat* HS = Hand->SeatByNumber(Hero->HandSeat))
			{
				for (int C : HS->Hole)
				{
					P.Known.Add(C);
				}
			}
		}
	}
	P.Pot = Pot ? Pot->GetAmount() : 0;
	for (const FSeat& S : Seats)
	{
		P.Pot += S.BetPile ? S.BetPile->GetAmount() : 0;
	}
	if (!bHeroTurn || !Hand || Hand->bComplete)
	{
		return P;
	}
	const ss::LegalActions L = Hand->GetLegalActions();
	P.bYourTurn = true;
	P.bCanCheck = L.CanCheck;
	P.ToCall = L.CallAmount;
	P.bCanRaise = L.CanRaise;
	P.bIsBet = L.IsBet;
	P.MinRaiseTo = L.MinRaiseTo;
	P.MaxRaiseTo = L.MaxRaiseTo;
	P.RaiseTo = HeroRaiseTo;
	if (const ss::HandSeat* HS = Hero ? Hand->SeatByNumber(Hero->HandSeat) : nullptr)
	{
		P.Stack = HS->Stack;
	}
	return P;
}

void ABackRoomTable::HeroAct(int32 Kind, double To)
{
	if (!bHeroTurn || !Hand || Hand->bComplete || Wait > 0.0f || Cursor < Hand->Events.size())
	{
		return;
	}
	ss::PlayerAction A;
	A.Type = static_cast<ss::PlayerAction::Kind>(Kind);
	A.To = To;
	if (Hand->Act(A))
	{
		bHeroTurn = false;
	}
}

void ABackRoomTable::HeroFold()
{
	HeroAct(static_cast<int32>(ss::PlayerAction::Kind::Fold), 0.0);
}

void ABackRoomTable::HeroCheckCall()
{
	if (!Hand)
	{
		return;
	}
	const ss::LegalActions L = Hand->GetLegalActions();
	HeroAct(static_cast<int32>(L.CanCheck ? ss::PlayerAction::Kind::Check : ss::PlayerAction::Kind::Call), 0.0);
}

void ABackRoomTable::HeroRaise()
{
	if (!Hand || !Hand->GetLegalActions().CanRaise)
	{
		return;
	}
	HeroAct(static_cast<int32>(ss::PlayerAction::Kind::Raise), static_cast<double>(HeroRaiseTo));
}

void ABackRoomTable::HeroAllIn()
{
	if (!Hand)
	{
		return;
	}
	const ss::LegalActions L = Hand->GetLegalActions();
	if (L.CanRaise)
	{
		HeroAct(static_cast<int32>(ss::PlayerAction::Kind::Raise), static_cast<double>(L.MaxRaiseTo));
	}
	else
	{
		HeroAct(static_cast<int32>(ss::PlayerAction::Kind::Call), 0.0);
	}
}

void ABackRoomTable::HeroAdjustRaise(int32 Steps)
{
	if (!bHeroTurn || !Hand)
	{
		return;
	}
	const ss::LegalActions L = Hand->GetLegalActions();
	const int64 Bb = BigBlindNow();
	const int64 Step = HeroRaiseTo < 20 * Bb ? Bb : 5 * Bb;
	HeroRaiseTo = FMath::Clamp<int64>(HeroRaiseTo + Steps * Step, L.MinRaiseTo, FMath::Max(L.MinRaiseTo, L.MaxRaiseTo));
}

void ABackRoomTable::HeroPeeked()
{
	bHeroPeeked = Hand.IsValid() && HeroSeat() && HeroSeat()->Hole.Num() == 2;
}

FString ABackRoomTable::Describe() const
{
	FString Out = FString::Printf(TEXT("hand %d"), HandNumber);
	if (Hand)
	{
		static const TCHAR* Streets[] = {TEXT("preflop"), TEXT("flop"), TEXT("turn"), TEXT("river"), TEXT("showdown")};
		Out += FString::Printf(TEXT(" %s pot %lld"), Streets[FMath::Clamp(static_cast<int32>(Hand->CurrentStreet), 0, 4)], static_cast<int64>(Hand->Pot()));
		if (Hand->ToAct >= 0)
		{
			Out += FString::Printf(TEXT(" to act seat %d"), Hand->Seats[static_cast<size_t>(Hand->ToAct)].Seat);
		}
		Out += Hand->bComplete ? TEXT(" complete") : TEXT("");
	}
	Out += bHeroTurn ? TEXT(" HERO TURN") : TEXT("");
	Out += FString::Printf(TEXT(" | bpm %.0f tilt %.2f reads %d%s"), Composure.Bpm, Composure.Tilt, Reads.Num(), bHolding ? TEXT(" HOLDING") : TEXT(""));
	if (Tourney)
	{
		Out += FString::Printf(TEXT(" | MTT level %d %lld/%lld left %d/%d table %d"), Tourney->LevelIndex + 1, static_cast<int64>(Tourney->CurrentLevel().Sb),
			static_cast<int64>(Tourney->CurrentLevel().Bb), Tourney->Remaining, Tourney->Spec.Entrants, Tourney->Hero().TableId);
	}
	for (const FSeat& S : Seats)
	{
		Out += FString::Printf(TEXT(" | %s %lld"), *S.Id, S.StackPile ? S.StackPile->GetAmount() : S.Stack);
	}
	return Out;
}
