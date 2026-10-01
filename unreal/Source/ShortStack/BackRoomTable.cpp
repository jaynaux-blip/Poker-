#include "BackRoomTable.h"

#include "BackRoomAmbience.h"
#include "BackRoomCard.h"
#include "BackRoomChips.h"
#include "BackRoomPlayer.h"
#include "BackRoomStage.h"
#include "Engine/World.h"
#include "NightOneAudio.h"
#include "TimerManager.h"

#include "ShortStack/AI/Bot.h"
#include "ShortStack/AI/View.h"
#include "ShortStack/Cards.h"
#include "ShortStack/Equity.h"

namespace BackRoomTableDetail
{
// ABackRoomStage's layout: a seated player's chair front edge is this far out from the rail.
const double RailGap = 22.0;
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

void ABackRoomTable::AddPlayer(ABackRoomPlayer* Player, int32 TableSeat, ss::Archetype Archetype, int64 BuyIn)
{
	if (!Rng)
	{
		Rng = MakeUnique<ss::Rng>(TCHAR_TO_UTF8(*FString::Printf(TEXT("backroom-%lld"), FDateTime::Now().GetTicks())));
		Fx.Initialize(static_cast<int32>(FDateTime::Now().GetTicks() & 0x7fffffff));
	}
	FSeat& S = Seats.AddDefaulted_GetRef();
	S.TableSeat = TableSeat;
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
		Player->Spots.Stack = StackSpot(TableSeat);
		Player->Spots.Bet = BetSpot(TableSeat);
		Player->Spots.Inward = T.GetRotation().GetForwardVector();
		if (!S.bHero)
		{
			Opponents.Add(Player);
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
	if (Audio)
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

ABackRoomTable::FSeat* ABackRoomTable::SeatAt(int32 TableSeat)
{
	return Seats.FindByPredicate([TableSeat](const FSeat& S) { return S.TableSeat == TableSeat; });
}

const ABackRoomTable::FSeat* ABackRoomTable::SeatAt(int32 TableSeat) const
{
	return Seats.FindByPredicate([TableSeat](const FSeat& S) { return S.TableSeat == TableSeat; });
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
	// Two cards side by side, a little overlapped, the long side toward the player, the face's top away.
	const FVector At = T.TransformPosition(FVector(RailGap + 25.0, Index == 0 ? -5.9 : -0.1, ABackRoomStage::FeltZ + 0.04 * Index));
	return FTransform(FRotator(0.0f, T.Rotator().Yaw + 90.0f + (Index == 0 ? -4.0f : 3.0f), 0.0f), At);
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
		}
		return;
	}
	if (!bBotPending)
	{
		const ss::PlayerView View = ss::MakeView(*Hand, Idx);
		ss::BotContext Ctx;
		Ctx.Prof = &S->Profile;
		Ctx.R = Rng.Get();
		const ss::BotDecision D = ss::Decide(View, Ctx);
		bBotPending = true;
		BotSeat = S->TableSeat;
		BotKind = static_cast<int32>(D.Action.Type);
		BotTo = D.Action.To;
		bBotBluff = D.Action.Type == ss::PlayerAction::Kind::Raise && D.Equity < 0.42;
		// How long they sit with it: the engine's timing tell, slowed to a live table's pace.
		BotAt = Time + FMath::Clamp(static_cast<float>(D.ThinkMs) / 1000.0f * 0.6f, 0.9f, 6.5f);
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

	// Dee's lessons, a few hands apart.
	switch (HandNumber)
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
		}
		else
		{
			P->OnActed(bAggressive, bBluff);
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
				ABackRoomPlayer* P = S->Player;
				FTimerHandle Handle;
				GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(P, [P]() { P->GestureShow(); }), FMath::Max(T, 0.01f), false);
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
				ABackRoomPlayer* P = S->Player;
				FTimerHandle Handle;
				GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(P, [P]() { P->GestureShow(); }), FMath::Max(T, 0.01f), false);
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
			if (S->bHero && R.Amount >= 40)
			{
				DealerSays(TEXT("Ship it to the kid."));
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
			T += Dealer ? Dealer->GestureDeal(C, CardSpot(Seat, Round), false) : 0.3f;
			if (!Dealer)
			{
				C->SetActorHiddenInGame(false);
				C->PitchTo(CardSpot(Seat, Round), 0.3f, 3.0f);
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
		const ss::HandSeat* HS = Hand->SeatByNumber(S.TableSeat);
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
	Hand.Reset();
	bHeroTurn = false;
	Wait = T + 2.2f;
}

// ------------------------------------------------------------------ the hero

void ABackRoomTable::OpenHeroTurn()
{
	bHeroTurn = true;
	const ss::LegalActions L = Hand->GetLegalActions();
	const int64 Bb = BigBlind;
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
	P.BigBlind = BigBlind;
	const FSeat* Hero = HeroSeat();
	if (Hero)
	{
		P.Stack = Hero->StackPile ? Hero->StackPile->GetAmount() : Hero->Stack;
		if (bHeroPeeked && Hand)
		{
			if (const ss::HandSeat* HS = Hand->SeatByNumber(Hero->TableSeat))
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
	if (const ss::HandSeat* HS = Hero ? Hand->SeatByNumber(Hero->TableSeat) : nullptr)
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
	const int64 Step = HeroRaiseTo < 20 * BigBlind ? BigBlind : 5 * BigBlind;
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
	for (const FSeat& S : Seats)
	{
		Out += FString::Printf(TEXT(" | %s %lld"), *S.Id, S.StackPile ? S.StackPile->GetAmount() : S.Stack);
	}
	return Out;
}
