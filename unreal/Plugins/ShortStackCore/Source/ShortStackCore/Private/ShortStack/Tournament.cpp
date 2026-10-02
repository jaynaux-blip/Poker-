#include "ShortStack/Tournament.h"
#include "StrictFloat.h"

#include "ShortStack/AI/View.h"
#include "ShortStack/Names.h"

#include <algorithm>
#include <cmath>
#include <climits>

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
	KeepFeature(Events);
	if (!bAnnouncedFinal && Tables.size() == 1 && Alive > 1)
	{
		bAnnouncedFinal = true;
		TEvent E;
		E.Type = TEventType::FinalTable;
		Events.push_back(E);
	}
	return Events;
}

void Tournament::KeepFeature(std::vector<TEvent>& Events)
{
	if (FeatureIds.empty() || Hero().Busted)
	{
		return;
	}
	const auto HomeIt = Tables.find(Hero().TableId);
	if (HomeIt == Tables.end())
	{
		return;
	}
	TTable& Home = HomeIt->second;
	auto IsFeatured = [&](int Idx) { return std::find(FeatureIds.begin(), FeatureIds.end(), Players[static_cast<size_t>(Idx)].Id) != FeatureIds.end(); };
	for (int K = 0; K < TableSize; ++K)
	{
		const int Idx = Home.Seats[static_cast<size_t>(K)];
		if (Idx < 0 || Idx == HeroIndex || IsFeatured(Idx))
		{
			continue;
		}
		// The featured player to bring over: someone the hero hasn't sat with, if anyone.
		int Best = -1;
		for (const std::string& Id : FeatureIds)
		{
			const int C = PlayerIndex(Id);
			if (C < 0 || Players[static_cast<size_t>(C)].Busted || Players[static_cast<size_t>(C)].TableId == Home.Id)
			{
				continue;
			}
			if (Best < 0 || (FeatureMet.count(Players[static_cast<size_t>(Best)].Id) > 0 && FeatureMet.count(Id) == 0))
			{
				Best = C;
			}
		}
		if (Best < 0)
		{
			break;
		}
		// Swap them seat for seat.
		TPlayer& Out = Players[static_cast<size_t>(Idx)];
		TPlayer& In = Players[static_cast<size_t>(Best)];
		TTable& Other = Tables[In.TableId];
		const int OutSeat = Out.Seat;
		const int InSeat = In.Seat;
		Home.Seats[static_cast<size_t>(OutSeat)] = Best;
		Other.Seats[static_cast<size_t>(InSeat)] = Idx;
		Out.TableId = Other.Id;
		Out.Seat = InSeat;
		In.TableId = Home.Id;
		In.Seat = OutSeat;
		TEvent A;
		A.Type = TEventType::Moved;
		A.Id = Out.Id;
		A.From = Home.Id;
		A.To = Other.Id;
		Events.push_back(A);
		TEvent B;
		B.Type = TEventType::Moved;
		B.Id = In.Id;
		B.From = Other.Id;
		B.To = Home.Id;
		Events.push_back(B);
	}
	for (int S : Home.Seats)
	{
		if (S >= 0 && S != HeroIndex)
		{
			FeatureMet.insert(Players[static_cast<size_t>(S)].Id);
		}
	}
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
} // namespace ss
