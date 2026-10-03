#include "ShortStack/Tournament.h"
#include "StrictFloat.h"

#include "ShortStack/AI/View.h"
#include "ShortStack/Names.h"

#include <algorithm>
#include <cmath>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace ss
{
const char* const HeroId = "hero";

Tournament::Tournament(const TournamentSpec& InSpec, const std::string& HeroName, const std::string& Seed, const std::vector<ReservedPlayer>& Reserved)
	: Spec(InSpec), R(Seed)
{
	Levels = Spec.Levels.empty() ? StandardLevels() : Spec.Levels;
	TableSize = Spec.TableSize > 0 ? Spec.TableSize : 9;
	Remaining = Spec.Entrants;
	const Chips Pool = MaxChips(Spec.GuaranteeCents, static_cast<Chips>(Spec.Entrants) * (Spec.BuyInCents - Spec.FeeCents - Spec.BountyCents));
	PrizePoolCents = Pool;
	const Chips MinCash = Spec.BuyInCents > 0 ? static_cast<Chips>(JsRound(static_cast<double>(Spec.BuyInCents - Spec.FeeCents - Spec.BountyCents) * 1.4)) : 20;
	if (Spec.SeatValueCents > 0)
	{
		// Satellites: as many seats as the pool buys, and what is left to the next place.
		const Chips Seats = MaxChips(1, Pool / Spec.SeatValueCents);
		Payouts.assign(static_cast<size_t>(Seats), Spec.SeatValueCents);
		if (Pool > Seats * Spec.SeatValueCents)
		{
			Payouts.push_back(Pool - Seats * Spec.SeatValueCents);
		}
	}
	else
	{
		Payouts = PayoutTable(Pool, Spec.Entrants, MinCash);
	}

	Rng NameRng = R.Fork("names");
	std::vector<std::string> ReservedNames = {HeroName};
	for (const ReservedPlayer& Rp : Reserved)
	{
		ReservedNames.push_back(Rp.Name);
	}
	const std::vector<std::string> Names = UniqueNames(Spec.Entrants - 1 - static_cast<int>(Reserved.size()), NameRng, ReservedNames);
	const Population& Pop = GetPopulation(Spec.Population);

	auto Make = [&](const std::string& Id, const std::string& Name, bool IsHero) {
		TPlayer P;
		P.Id = Id;
		P.Name = Name;
		P.IsHero = IsHero;
		P.Stack = Spec.StartingStack;
		return P;
	};
	Players.push_back(Make(HeroId, HeroName, true));
	for (const ReservedPlayer& Rp : Reserved)
	{
		TPlayer P = Make("npc:" + Rp.Name, Rp.Name, false);
		P.Prof = MakeProfile(Rp.Type, R);
		P.HasProfile = true;
		Players.push_back(P);
	}
	for (size_t I = 0; I < Names.size(); ++I)
	{
		TPlayer P = Make("p" + std::to_string(I), Names[I], false);
		const Archetype A = RollArchetype(Pop, R);
		P.Prof = MakeProfile(A, R);
		P.HasProfile = true;
		Players.push_back(P);
	}
	for (size_t I = 0; I < Players.size(); ++I)
	{
		IndexById[Players[I].Id] = static_cast<int>(I);
	}
	HeroIndex = 0;

	// Seat everyone: fill tables evenly in random order.
	const int TableCount = (Spec.Entrants + TableSize - 1) / TableSize;
	for (int T = 1; T <= TableCount; ++T)
	{
		TTable Table;
		Table.Id = T;
		Table.Seats.assign(static_cast<size_t>(TableSize), -1);
		Tables[T] = Table;
	}
	std::vector<int> Order;
	for (size_t I = 0; I < Players.size(); ++I)
	{
		Order.push_back(static_cast<int>(I));
	}
	R.Shuffle(Order);
	for (size_t I = 0; I < Order.size(); ++I)
	{
		TTable& Table = Tables[static_cast<int>(I % static_cast<size_t>(TableCount)) + 1];
		std::vector<int> Empty;
		for (int K = 0; K < TableSize; ++K)
		{
			if (Table.Seats[static_cast<size_t>(K)] < 0)
			{
				Empty.push_back(K);
			}
		}
		const int Seat = R.Pick(Empty);
		Table.Seats[static_cast<size_t>(Seat)] = Order[I];
		TPlayer& P = Players[static_cast<size_t>(Order[I])];
		P.TableId = Table.Id;
		P.Seat = Seat;
	}
	for (auto& It : Tables)
	{
		std::vector<int> Occupied;
		for (int K = 0; K < TableSize; ++K)
		{
			if (It.second.Seats[static_cast<size_t>(K)] >= 0)
			{
				Occupied.push_back(K);
			}
		}
		It.second.ButtonSeat = R.Pick(Occupied);
	}
}

// ---------------------------------------------------------------- queries

const Level& Tournament::CurrentLevel() const
{
	const size_t Last = Levels.size() - 1;
	return Levels[static_cast<size_t>(LevelIndex) < Last ? static_cast<size_t>(LevelIndex) : Last];
}

const Level& Tournament::NextLevel() const
{
	const size_t Last = Levels.size() - 1;
	const size_t Next = static_cast<size_t>(LevelIndex) + 1;
	return Levels[Next < Last ? Next : Last];
}

bool Tournament::HandForHand() const
{
	const int Third = static_cast<int>((Tables.size() + 2) / 3);
	return !InTheMoney() && Remaining <= PaidPlaces() + (Third > 2 ? Third : 2);
}

double Tournament::LevelSecondsLeft() const
{
	const double Len = Spec.LevelMinutes * 60.0;
	return Len - std::fmod(ElapsedSeconds(), Len);
}

double Tournament::AverageStack() const
{
	return static_cast<double>(static_cast<Chips>(Spec.Entrants) * Spec.StartingStack) / static_cast<double>(Remaining > 1 ? Remaining : 1);
}

std::vector<const TPlayer*> Tournament::AlivePlayers() const
{
	std::vector<const TPlayer*> Out;
	for (const TPlayer& P : Players)
	{
		if (!P.Busted)
		{
			Out.push_back(&P);
		}
	}
	return Out;
}

std::vector<const TPlayer*> Tournament::Standings() const
{
	std::vector<const TPlayer*> Out = AlivePlayers();
	std::stable_sort(Out.begin(), Out.end(), [](const TPlayer* A, const TPlayer* B) { return A->Stack > B->Stack; });
	return Out;
}

int Tournament::HeroRank() const
{
	const TPlayer& H = Hero();
	if (H.Busted)
	{
		return H.Place;
	}
	int Rank = 1;
	for (const TPlayer& P : Players)
	{
		if (!P.Busted && P.Stack > H.Stack)
		{
			++Rank;
		}
	}
	return Rank;
}

Chips Tournament::PrizeFor(int Place) const
{
	return Place >= 1 && Place <= PaidPlaces() ? Payouts[static_cast<size_t>(Place - 1)] : 0;
}

int Tournament::PlayerIndex(const std::string& Id) const
{
	const auto It = IndexById.find(Id);
	return It == IndexById.end() ? -1 : It->second;
}

double Tournament::PressureFor(const TPlayer& P) const
{
	const int Rem = Remaining;
	const int Paid = PaidPlaces();
	if (Rem <= 1)
	{
		return 0.0;
	}
	const double Bbs = static_cast<double>(P.Stack) / static_cast<double>(CurrentLevel().Bb);
	if (Rem > Paid)
	{
		const double Window = Max(3.0, Paid * 0.25);
		const double Closeness = Max(0.0, Min(1.0, 1.0 - (Rem - Paid) / Window));
		if (Closeness == 0.0)
		{
			return 0.0;
		}
		const double StackFactor = Bbs < 8.0 ? 0.35 : static_cast<double>(P.Stack) > AverageStack() * 2.0 ? 0.25 : 1.0;
		return Closeness * StackFactor;
	}
	if (Rem <= TableSize)
	{
		return 0.45 * (Bbs < 8.0 ? 0.5 : 1.0);
	}
	return 0.12;
}

// ---------------------------------------------------------------- hands

std::unique_ptr<Hand> Tournament::MakeHand(TTable& Table)
{
	std::vector<int> Occupied;
	for (int K = 0; K < TableSize; ++K)
	{
		if (Table.Seats[static_cast<size_t>(K)] >= 0 && !(HeroAway && Table.Seats[static_cast<size_t>(K)] == HeroIndex))
		{
			Occupied.push_back(K);
		}
	}
	if (Occupied.size() < 2)
	{
		return nullptr;
	}
	// Move the button to the next occupied seat clockwise.
	int B = Table.ButtonSeat;
	for (int K = 1; K <= TableSize; ++K)
	{
		const int S = ((B + K) % TableSize + TableSize) % TableSize;
		if (std::find(Occupied.begin(), Occupied.end(), S) != Occupied.end())
		{
			B = S;
			break;
		}
	}
	Table.ButtonSeat = B;
	const Level& L = CurrentLevel();
	HandConfig Cfg;
	for (int Seat : Occupied)
	{
		const TPlayer& P = Players[static_cast<size_t>(Table.Seats[static_cast<size_t>(Seat)])];
		SeatInput In;
		In.Seat = Seat;
		In.Id = P.Id;
		In.Stack = P.Stack;
		Cfg.Players.push_back(In);
	}
	Cfg.ButtonSeat = B;
	Cfg.SmallBlind = L.Sb;
	Cfg.BigBlind = L.Bb;
	Cfg.Ante = L.Ante;
	std::unique_ptr<Hand> H(new Hand(Cfg, R));
	for (const HandSeat& S : H->Seats)
	{
		StartStacks[S.Id] = S.StartStack;
	}
	return H;
}

BotDecision Tournament::BotDecisionFor(const Hand& H, bool Fast, const Profile* ProfileOverride)
{
	const int Idx = H.ToAct;
	const TPlayer& P = Players[static_cast<size_t>(PlayerIndex(H.Seats[static_cast<size_t>(Idx)].Id))];
	BotContext Ctx;
	Ctx.Prof = ProfileOverride ? ProfileOverride : &P.Prof;
	Ctx.Tilt = P.Tilt;
	Ctx.IcmPressure = PressureFor(P);
	Ctx.Fast = Fast;
	Ctx.R = &R;
	return Decide(MakeView(H, Idx), Ctx);
}

void Tournament::PlayInstant(Hand& H, bool Fast, const Profile* HeroAuto)
{
	int Guard = 0;
	while (!H.bComplete)
	{
		const std::string& Id = H.Seats[static_cast<size_t>(H.ToAct)].Id;
		if (HeroSitsOut && Id == HeroId)
		{
			const LegalActions L = H.GetLegalActions();
			const bool Folded = H.Act(L.CanCheck ? PlayerAction::Check() : PlayerAction::Fold());
			SS_ASSERT(Folded);
			(void)Folded;
			if (++Guard > 1000)
			{
				break;
			}
			continue;
		}
		const BotDecision D = BotDecisionFor(H, Fast, Id == HeroId ? HeroAuto : nullptr);
		const bool Ok = H.Act(D.Action);
		SS_ASSERT(Ok);
		(void)Ok;
		// Bots always choose legal actions, so a hand ends well before this; the cap only guards against bugs.
		if (++Guard > 1000)
		{
			break;
		}
	}
}

void Tournament::ApplyHand(const Hand& H)
{
	std::vector<std::string> Vpip;
	std::vector<std::string> Pfr;
	for (const ActionRecord& A : H.History)
	{
		if (A.OnStreet != Street::Preflop)
		{
			break;
		}
		const std::string& Id = H.SeatByNumber(A.Seat)->Id;
		if (A.Type == ActionType::Call || A.Type == ActionType::Raise || A.Type == ActionType::Bet)
		{
			Vpip.push_back(Id);
		}
		if (A.Type == ActionType::Raise || A.Type == ActionType::Bet)
		{
			Pfr.push_back(Id);
		}
	}
	auto Has = [](const std::vector<std::string>& V, const std::string& Id) { return std::find(V.begin(), V.end(), Id) != V.end(); };
	for (const HandSeat& S : H.Seats)
	{
		TPlayer& P = Players[static_cast<size_t>(PlayerIndex(S.Id))];
		P.Stack = S.Stack;
		++P.Hands;
		if (Has(Vpip, S.Id))
		{
			++P.VpipHands;
		}
		if (Has(Pfr, S.Id))
		{
			++P.PfrHands;
		}
		P.Tilt *= 0.93;
		if (S.Stack <= 0 && P.KnockedOutBy.empty())
		{
			// The bounty goes to whoever won the last pot this player was in.
			for (const PotResult& Pot : H.PotResults)
			{
				if (std::find(Pot.Eligible.begin(), Pot.Eligible.end(), S.Seat) != Pot.Eligible.end() && !Pot.Winners.empty())
				{
					const HandSeat* W = H.SeatByNumber(Pot.Winners.front());
					P.KnockedOutBy = W && W->Id != S.Id ? W->Id : P.KnockedOutBy;
				}
			}
		}
		const double Lost = static_cast<double>(S.StartStack - S.Stack);
		const double Start = static_cast<double>(S.StartStack);
		if (P.HasProfile && S.Stack > 0 && Lost > Start * 0.4)
		{
			P.Tilt = Min(1.0, P.Tilt + P.Prof.TiltProne * 0.6 * (Lost / Start));
		}
	}
}

// ---------------------------------------------------------------- ticks

std::unique_ptr<Hand> Tournament::StartTick(std::vector<TEvent>& OutEvents)
{
	OutEvents = Balance();
	const TPlayer& H = Hero();
	if (!H.Busted && !bFinished && !HeroAway)
	{
		return MakeHand(Tables[H.TableId]);
	}
	return nullptr;
}

std::vector<TEvent> Tournament::FinishTick(Hand* HeroHand, const Profile* HeroAuto)
{
	BeginFinish(HeroHand, HeroAuto);
	FinishSome(INT_MAX);
	return EndFinish();
}

void Tournament::BeginFinish(Hand* HeroHand, const Profile* HeroAuto)
{
	FinishHeroTable = Hero().Busted ? -1 : Hero().TableId;
	FinishWithHero = HeroHand != nullptr;
	FinishNextTable = INT_MIN;
	bFinishing = true;
	if (HeroHand)
	{
		if (!HeroHand->bComplete)
		{
			PlayInstant(*HeroHand, false, HeroAuto);
		}
		ApplyHand(*HeroHand);
	}
}

bool Tournament::FinishSome(int Count)
{
	if (!bFinishing)
	{
		return true;
	}
	// Tables in id order, as FinishTick always played them; hands don't add or remove tables.
	for (auto It = Tables.lower_bound(FinishNextTable); It != Tables.end(); ++It)
	{
		if (Count <= 0)
		{
			FinishNextTable = It->first;
			return false;
		}
		TTable& Table = It->second;
		if (Table.Id == FinishHeroTable && FinishWithHero)
		{
			continue;
		}
		std::unique_ptr<Hand> H = MakeHand(Table);
		if (!H)
		{
			continue;
		}
		PlayInstant(*H, true, nullptr);
		ApplyHand(*H);
		--Count;
	}
	FinishNextTable = INT_MAX;
	return true;
}

std::vector<TEvent> Tournament::EndFinish()
{
	std::vector<TEvent> Events;
	if (!bFinishing)
	{
		return Events;
	}
	FinishSome(INT_MAX);
	bFinishing = false;
	const std::vector<TEvent> Busts = ProcessEliminations();
	Events.insert(Events.end(), Busts.begin(), Busts.end());
	++Tick;
	const int NewLevel = static_cast<int>(std::floor(ElapsedSeconds() / (Spec.LevelMinutes * 60.0)));
	if (NewLevel != LevelIndex && NewLevel < static_cast<int>(Levels.size()))
	{
		if (Spec.BreakEvery > 0 && NewLevel % Spec.BreakEvery == 0)
		{
			// The room stops between levels: the clock takes the break, then the new level starts.
			++BreaksTaken;
			TEvent B;
			B.Type = TEventType::Break;
			B.LevelNumber = NewLevel + 1;
			Events.push_back(B);
		}
		LevelIndex = NewLevel;
		TEvent E;
		E.Type = TEventType::Level;
		E.LevelNumber = NewLevel + 1;
		E.Blinds = CurrentLevel();
		Events.push_back(E);
	}
	if (!bAnnouncedH4H && HandForHand())
	{
		bAnnouncedH4H = true;
		TEvent E;
		E.Type = TEventType::HandForHand;
		Events.push_back(E);
	}
	return Events;
}

std::vector<TEvent> Tournament::SimulateTick(const Profile* HeroAuto)
{
	std::vector<TEvent> Events;
	std::unique_ptr<Hand> H = StartTick(Events);
	const std::vector<TEvent> More = FinishTick(H.get(), HeroAuto);
	Events.insert(Events.end(), More.begin(), More.end());
	return Events;
}

std::vector<TEvent> Tournament::ProcessEliminations()
{
	std::vector<TEvent> Events;
	std::vector<int> Busted;
	for (size_t I = 0; I < Players.size(); ++I)
	{
		if (!Players[I].Busted && Players[I].Stack <= 0)
		{
			Busted.push_back(static_cast<int>(I));
		}
	}
	if (Busted.empty())
	{
		return Events;
	}
	// Players eliminated in the same round: bigger starting stack finishes higher.
	auto StartOf = [&](int Idx) {
		const auto It = StartStacks.find(Players[static_cast<size_t>(Idx)].Id);
		return It == StartStacks.end() ? static_cast<Chips>(0) : It->second;
	};
	std::stable_sort(Busted.begin(), Busted.end(), [&](int A, int B) { return StartOf(A) < StartOf(B); });
	const bool BubbleBefore = Remaining > PaidPlaces();
	for (int Idx : Busted)
	{
		TPlayer& P = Players[static_cast<size_t>(Idx)];
		P.Busted = true;
		P.Place = Remaining;
		P.PrizeCents = PrizeFor(P.Place);
		--Remaining;
		const auto T = Tables.find(P.TableId);
		if (T != Tables.end())
		{
			T->second.Seats[static_cast<size_t>(P.Seat)] = -1;
		}
		TEvent E;
		E.Type = TEventType::Bust;
		E.Id = P.Id;
		E.Name = P.Name;
		E.Place = P.Place;
		E.PrizeCents = P.PrizeCents;
		E.TableId = P.TableId;
		E.IsHero = P.IsHero;
		E.EliminatedBy = P.KnockedOutBy;
		Events.push_back(E);
	}
	if (BubbleBefore && Remaining <= PaidPlaces() && !bBurstBubble)
	{
		bBurstBubble = true;
		const TPlayer* BubbleBoy = &Players[static_cast<size_t>(Busted.back())];
		for (int Idx : Busted)
		{
			if (Players[static_cast<size_t>(Idx)].Place == PaidPlaces() + 1)
			{
				BubbleBoy = &Players[static_cast<size_t>(Idx)];
				break;
			}
		}
		TEvent E;
		E.Type = TEventType::Bubble;
		E.Name = BubbleBoy->Name;
		Events.push_back(E);
	}
	if (Remaining == 1)
	{
		for (TPlayer& P : Players)
		{
			if (!P.Busted)
			{
				P.Place = 1;
				P.PrizeCents = PrizeFor(1);
				bFinished = true;
				TEvent E;
				E.Type = TEventType::Finished;
				E.Id = P.Id;
				E.Name = P.Name;
				Events.push_back(E);
				break;
			}
		}
	}
	return Events;
}

// ---------------------------------------------------------------- tables

int Tournament::Count(const TTable& T) const
{
	int N = 0;
	for (int S : T.Seats)
	{
		if (S >= 0)
		{
			++N;
		}
	}
	return N;
}

void Tournament::SeatPlayer(int PlayerIdx, TTable& Table)
{
	std::vector<int> Empty;
	for (int K = 0; K < TableSize; ++K)
	{
		if (Table.Seats[static_cast<size_t>(K)] < 0)
		{
			Empty.push_back(K);
		}
	}
	const int Seat = R.Pick(Empty);
	Table.Seats[static_cast<size_t>(Seat)] = PlayerIdx;
	TPlayer& P = Players[static_cast<size_t>(PlayerIdx)];
	P.TableId = Table.Id;
	P.Seat = Seat;
}

void Tournament::MovePlayer(int PlayerIdx, TTable& To, std::vector<TEvent>& Events)
{
	TPlayer& P = Players[static_cast<size_t>(PlayerIdx)];
	const auto From = Tables.find(P.TableId);
	if (From != Tables.end())
	{
		From->second.Seats[static_cast<size_t>(P.Seat)] = -1;
	}
	const int FromId = P.TableId;
	SeatPlayer(PlayerIdx, To);
	TEvent E;
	E.Type = TEventType::Moved;
	E.Id = P.Id;
	E.From = FromId;
	E.To = To.Id;
	E.IsHero = P.IsHero;
	Events.push_back(E);
}

std::vector<TEvent> Tournament::Balance()
{
	std::vector<TEvent> Events;
	if (bFinished)
	{
		return Events;
	}
	const int Alive = Remaining;
	const int TargetRaw = (Alive + TableSize - 1) / TableSize;
	const size_t Target = static_cast<size_t>(TargetRaw > 1 ? TargetRaw : 1);
	auto SortedTables = [&](bool TieBreakHighIdFirst) {
		std::vector<TTable*> List;
		for (auto& It : Tables)
		{
			List.push_back(&It.second);
		}
		std::stable_sort(List.begin(), List.end(), [&](const TTable* A, const TTable* B) {
			const int Ca = Count(*A);
			const int Cb = Count(*B);
			if (Ca != Cb)
			{
				return Ca < Cb;
			}
			return TieBreakHighIdFirst ? A->Id > B->Id : false;
		});
		return List;
	};
	while (Tables.size() > Target)
	{
		const TTable Victim = *SortedTables(true).front();
		Tables.erase(Victim.Id);
		TEvent Broken;
		Broken.Type = TEventType::TableBroken;
		Broken.TableId = Victim.Id;
		Events.push_back(Broken);
		for (int PlayerIdx : Victim.Seats)
		{
			if (PlayerIdx < 0)
			{
				continue;
			}
			TTable* Dest = SortedTables(false).front();
			TPlayer& P = Players[static_cast<size_t>(PlayerIdx)];
			const int FromId = P.TableId;
			SeatPlayer(PlayerIdx, *Dest);
			TEvent Moved;
			Moved.Type = TEventType::Moved;
			Moved.Id = P.Id;
			Moved.From = FromId;
			Moved.To = Dest->Id;
			Moved.IsHero = P.IsHero;
			Events.push_back(Moved);
		}
	}
	// Even out: no table more than one player bigger than another.
	for (int Guard = 0; Guard < 50; ++Guard)
	{
		const std::vector<TTable*> List = SortedTables(false);
		TTable* Small = List.front();
		TTable* Big = List.back();
		if (Count(*Big) - Count(*Small) <= 1)
		{
			break;
		}
		// Move the player due for the big blind next at the big table.
		std::vector<int> AfterButton;
		for (int K = 0; K < TableSize; ++K)
		{
			if (Big->Seats[static_cast<size_t>(K)] >= 0 && K > Big->ButtonSeat)
			{
				AfterButton.push_back(K);
			}
		}
		for (int K = 0; K < TableSize; ++K)
		{
			if (Big->Seats[static_cast<size_t>(K)] >= 0 && K <= Big->ButtonSeat)
			{
				AfterButton.push_back(K);
			}
		}
		const size_t Pick = AfterButton.size() > 1 ? 1 : AfterButton.size() - 1;
		const int Mover = Big->Seats[static_cast<size_t>(AfterButton[Pick])];
		MovePlayer(Mover, *Small, Events);
	}
	if (!bAnnouncedFinal && Tables.size() == 1 && Alive > 1)
	{
		bAnnouncedFinal = true;
		TEvent E;
		E.Type = TEventType::FinalTable;
		Events.push_back(E);
	}
	return Events;
}

std::vector<TEvent> Tournament::MoveToTable(const std::string& Id, int TableId)
{
	std::vector<TEvent> Events;
	const int PIdx = PlayerIndex(Id);
	const auto TIt = Tables.find(TableId);
	if (PIdx < 0 || TIt == Tables.end())
	{
		return Events;
	}
	TPlayer& P = Players[static_cast<size_t>(PIdx)];
	if (P.Busted || P.TableId == TableId)
	{
		return Events;
	}
	TTable& Table = TIt->second;
	if (Count(Table) >= TableSize)
	{
		// Table is full: swap a non-hero player over to the mover's old table.
		TTable& Old = Tables[P.TableId];
		int SwapIdx = -1;
		for (int S : Table.Seats)
		{
			if (S >= 0 && S != HeroIndex)
			{
				SwapIdx = S;
				break;
			}
		}
		TPlayer& Swap = Players[static_cast<size_t>(SwapIdx)];
		Old.Seats[static_cast<size_t>(P.Seat)] = -1;
		Table.Seats[static_cast<size_t>(Swap.Seat)] = -1;
		SeatPlayer(SwapIdx, Old);
		SeatPlayer(PIdx, Table);
		TEvent A;
		A.Type = TEventType::Moved;
		A.Id = Swap.Id;
		A.From = TableId;
		A.To = Old.Id;
		Events.push_back(A);
		TEvent B;
		B.Type = TEventType::Moved;
		B.Id = P.Id;
		B.From = Old.Id;
		B.To = TableId;
		B.IsHero = P.IsHero;
		Events.push_back(B);
		return Events;
	}
	MovePlayer(PIdx, Table, Events);
	return Events;
}

// ---------------------------------------------------------------- checkpoints

namespace
{
/** The field a checkpoint belongs to: who is in it, under which ids (FNV-1a). */
uint64_t FieldPrint(const TournamentSpec& Spec, const std::vector<TPlayer>& Players)
{
	uint64_t H = 1469598103934665603ull;
	auto Mix = [&H](const std::string& S) {
		for (const char Ch : S)
		{
			H = (H ^ static_cast<unsigned char>(Ch)) * 1099511628211ull;
		}
		H = (H ^ 0x1fu) * 1099511628211ull;
	};
	Mix(Spec.Id);
	Mix(std::to_string(Spec.Entrants));
	for (const TPlayer& P : Players)
	{
		Mix(P.Id);
		Mix(P.Name);
	}
	return H;
}

std::vector<std::string> SplitOn(const std::string& S, char Sep)
{
	std::vector<std::string> Out;
	size_t From = 0;
	for (;;)
	{
		const size_t At = S.find(Sep, From);
		Out.push_back(S.substr(From, At == std::string::npos ? std::string::npos : At - From));
		if (At == std::string::npos)
		{
			return Out;
		}
		From = At + 1;
	}
}

/** Strict parsing: the whole field, or it's not a checkpoint. */
bool ToInt(const std::string& S, long long& Out)
{
	if (S.empty())
	{
		return false;
	}
	char* End = nullptr;
	Out = std::strtoll(S.c_str(), &End, 10);
	return End && *End == '\0';
}

bool ToHex(const std::string& S, unsigned long long& Out)
{
	if (S.empty() || S[0] == '-')
	{
		return false;
	}
	char* End = nullptr;
	Out = std::strtoull(S.c_str(), &End, 16);
	return End && *End == '\0';
}

std::string Hex(unsigned long long V)
{
	char Buf[24];
	std::snprintf(Buf, sizeof(Buf), "%llx", V);
	return Buf;
}
} // namespace

std::string Tournament::Checkpoint() const
{
	if (bFinishing)
	{
		return std::string();
	}
	// Records split by ';', fields by ' ': nothing in it needs escaping. Doubles go as their bits, so they come back exact.
	std::string Out = "ck1 " + Hex(FieldPrint(Spec, Players)) + " " + std::to_string(Players.size()) + " " + std::to_string(Tables.size());
	uint32_t S[4];
	R.GetState(S);
	Out += ";r " + Hex(S[0]) + " " + Hex(S[1]) + " " + Hex(S[2]) + " " + Hex(S[3]);
	Out += ";c " + std::to_string(Tick) + " " + std::to_string(LevelIndex) + " " + std::to_string(Remaining) + " " + (bFinished ? "1" : "0") + " " + std::to_string(BreaksTaken) + " " +
		(HeroAway ? "1" : "0") + " " + (HeroSitsOut ? "1" : "0") + " " + (bAnnouncedFinal ? "1" : "0") + " " + (bAnnouncedH4H ? "1" : "0") + " " + (bBurstBubble ? "1" : "0");
	for (const TPlayer& P : Players)
	{
		uint64_t Tilt = 0;
		std::memcpy(&Tilt, &P.Tilt, sizeof(Tilt));
		const auto Start = StartStacks.find(P.Id);
		Out += ";p " + std::to_string(P.Stack) + " " + std::to_string(P.TableId) + " " + std::to_string(P.Seat) + " " + (P.Busted ? "1" : "0") + " " + std::to_string(P.Place) + " " +
			std::to_string(P.PrizeCents) + " " + Hex(Tilt) + " " + std::to_string(P.Hands) + " " + std::to_string(P.VpipHands) + " " + std::to_string(P.PfrHands) + " " +
			std::to_string(P.KnockedOutBy.empty() ? -1 : PlayerIndex(P.KnockedOutBy)) + " " + (Start == StartStacks.end() ? std::string("-") : std::to_string(Start->second));
	}
	for (const auto& It : Tables)
	{
		Out += ";t " + std::to_string(It.second.Id) + " " + std::to_string(It.second.ButtonSeat);
		for (const int Seat : It.second.Seats)
		{
			Out += " " + std::to_string(Seat);
		}
	}
	return Out;
}

bool Tournament::Restore(const std::string& Text)
{
	if (bFinishing)
	{
		return false;
	}
	const std::vector<std::string> Records = SplitOn(Text, ';');
	if (Records.size() < 3 + Players.size())
	{
		return false;
	}
	const std::vector<std::string> Head = SplitOn(Records[0], ' ');
	unsigned long long Print = 0;
	long long NPlayers = 0;
	long long NTables = 0;
	if (Head.size() != 4 || Head[0] != "ck1" || !ToHex(Head[1], Print) || Print != FieldPrint(Spec, Players) || !ToInt(Head[2], NPlayers) ||
		NPlayers != static_cast<long long>(Players.size()) || !ToInt(Head[3], NTables) || NTables < 0 || Records.size() != 3 + Players.size() + static_cast<size_t>(NTables))
	{
		return false;
	}
	// Everything is read into copies first: a bad record leaves the tournament as it was.
	const std::vector<std::string> RngF = SplitOn(Records[1], ' ');
	if (RngF.size() != 5 || RngF[0] != "r")
	{
		return false;
	}
	uint32_t State[4];
	for (int I = 0; I < 4; ++I)
	{
		unsigned long long V = 0;
		if (!ToHex(RngF[static_cast<size_t>(I + 1)], V) || V > 0xffffffffull)
		{
			return false;
		}
		State[I] = static_cast<uint32_t>(V);
	}
	const std::vector<std::string> Clock = SplitOn(Records[2], ' ');
	long long C[10];
	if (Clock.size() != 11 || Clock[0] != "c")
	{
		return false;
	}
	for (int I = 0; I < 10; ++I)
	{
		if (!ToInt(Clock[static_cast<size_t>(I + 1)], C[I]))
		{
			return false;
		}
	}
	if (C[0] < 0 || C[1] < 0 || C[2] < 0 || C[2] > static_cast<long long>(Players.size()) || C[4] < 0)
	{
		return false;
	}
	std::vector<TPlayer> NewPlayers = Players;
	std::unordered_map<std::string, Chips> NewStarts;
	for (size_t I = 0; I < Players.size(); ++I)
	{
		const std::vector<std::string> F = SplitOn(Records[3 + I], ' ');
		unsigned long long Tilt = 0;
		if (F.size() != 13 || F[0] != "p" || !ToHex(F[7], Tilt))
		{
			return false;
		}
		// Stack, table, seat, busted, place, prize, (tilt), hands, vpip, pfr, knocked out by.
		const int Fields[10] = {1, 2, 3, 4, 5, 6, 8, 9, 10, 11};
		long long V[10];
		for (int K = 0; K < 10; ++K)
		{
			if (!ToInt(F[static_cast<size_t>(Fields[K])], V[K]))
			{
				return false;
			}
		}
		if (V[0] < 0 || V[9] < -1 || V[9] >= static_cast<long long>(Players.size()))
		{
			return false;
		}
		TPlayer& P = NewPlayers[I];
		P.Stack = V[0];
		P.TableId = static_cast<int>(V[1]);
		P.Seat = static_cast<int>(V[2]);
		P.Busted = V[3] != 0;
		P.Place = static_cast<int>(V[4]);
		P.PrizeCents = V[5];
		const uint64_t TiltBits = Tilt;
		std::memcpy(&P.Tilt, &TiltBits, sizeof(TiltBits));
		P.Hands = static_cast<int>(V[6]);
		P.VpipHands = static_cast<int>(V[7]);
		P.PfrHands = static_cast<int>(V[8]);
		P.KnockedOutBy = V[9] < 0 ? std::string() : Players[static_cast<size_t>(V[9])].Id;
		if (F[12] != "-")
		{
			long long Start = 0;
			if (!ToInt(F[12], Start))
			{
				return false;
			}
			NewStarts[P.Id] = Start;
		}
	}
	std::map<int, TTable> NewTables;
	for (size_t I = 0; I < static_cast<size_t>(NTables); ++I)
	{
		const std::vector<std::string> F = SplitOn(Records[3 + Players.size() + I], ' ');
		long long Id = 0;
		long long Button = 0;
		if (F.size() != 3 + static_cast<size_t>(TableSize) || F[0] != "t" || !ToInt(F[1], Id) || !ToInt(F[2], Button) || Button < -1 || Button >= TableSize)
		{
			return false;
		}
		TTable T;
		T.Id = static_cast<int>(Id);
		T.ButtonSeat = static_cast<int>(Button);
		for (int K = 0; K < TableSize; ++K)
		{
			long long Seat = 0;
			if (!ToInt(F[static_cast<size_t>(3 + K)], Seat) || Seat < -1 || Seat >= static_cast<long long>(Players.size()))
			{
				return false;
			}
			T.Seats.push_back(static_cast<int>(Seat));
		}
		NewTables[T.Id] = T;
	}
	// Seats and tables have to agree: everyone still in sits where their table says.
	for (size_t I = 0; I < NewPlayers.size(); ++I)
	{
		const TPlayer& P = NewPlayers[I];
		if (P.Busted)
		{
			continue;
		}
		const auto T = NewTables.find(P.TableId);
		if (T == NewTables.end() || P.Seat < 0 || P.Seat >= TableSize || T->second.Seats[static_cast<size_t>(P.Seat)] != static_cast<int>(I))
		{
			return false;
		}
	}
	R.SetState(State);
	Players = std::move(NewPlayers);
	StartStacks = std::move(NewStarts);
	Tables = std::move(NewTables);
	Tick = static_cast<int>(C[0]);
	LevelIndex = static_cast<int>(C[1]);
	Remaining = static_cast<int>(C[2]);
	bFinished = C[3] != 0;
	BreaksTaken = static_cast<int>(C[4]);
	HeroAway = C[5] != 0;
	HeroSitsOut = C[6] != 0;
	bAnnouncedFinal = C[7] != 0;
	bAnnouncedH4H = C[8] != 0;
	bBurstBubble = C[9] != 0;
	return true;
}
} // namespace ss
