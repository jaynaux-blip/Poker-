#include "ShortStackTournamentSubsystem.h"

#include "Misc/Guid.h"
#include "ShortStackConvert.h"

DEFINE_LOG_CATEGORY_STATIC(LogShortStack, Log, All);

namespace ShortStackSubsystemDetail
{
const char* FieldName(EShortStackField Field)
{
	switch (Field)
	{
	case EShortStackField::Freeroll: return "freeroll";
	case EShortStackField::Low: return "low";
	case EShortStackField::High: return "high";
	case EShortStackField::Micro:
	default: return "micro";
	}
}

EShortStackEventType EventType(ss::TEventType Type)
{
	switch (Type)
	{
	case ss::TEventType::Bust: return EShortStackEventType::Bust;
	case ss::TEventType::Level: return EShortStackEventType::Level;
	case ss::TEventType::Moved: return EShortStackEventType::Moved;
	case ss::TEventType::TableBroken: return EShortStackEventType::TableBroken;
	case ss::TEventType::HandForHand: return EShortStackEventType::HandForHand;
	case ss::TEventType::Bubble: return EShortStackEventType::Bubble;
	case ss::TEventType::FinalTable: return EShortStackEventType::FinalTable;
	case ss::TEventType::Finished:
	default: return EShortStackEventType::Finished;
	}
}

void AppendEvents(const std::vector<ss::TEvent>& In, TArray<FShortStackEvent>& Out)
{
	for (const ss::TEvent& E : In)
	{
		FShortStackEvent& Ev = Out.AddDefaulted_GetRef();
		Ev.Type = EventType(E.Type);
		Ev.PlayerName = ShortStackConvert::ToUnreal(E.Name);
		Ev.bIsHero = E.IsHero;
		Ev.Place = E.Place;
		Ev.PrizeCents = E.PrizeCents;
		Ev.TableId = E.Type == ss::TEventType::Moved ? E.To : E.TableId;
		Ev.FromTableId = E.From;
		Ev.LevelNumber = E.LevelNumber;
		Ev.SmallBlind = E.Blinds.Sb;
		Ev.BigBlind = E.Blinds.Bb;
		Ev.Ante = E.Blinds.Ante;
	}
}
} // namespace ShortStackSubsystemDetail

void UShortStackTournamentSubsystem::Deinitialize()
{
	Tournament.Reset();
	Super::Deinitialize();
}

void UShortStackTournamentSubsystem::StartTournament(const FShortStackTournamentSettings& Settings)
{
	ss::TournamentSpec Spec;
	Spec.Id = ShortStackConvert::ToStd(Settings.Name);
	Spec.Name = Spec.Id;
	Spec.BuyInCents = FMath::Max<int64>(Settings.BuyInCents, 0);
	Spec.FeeCents = FMath::Clamp<int64>(Settings.FeeCents, 0, Spec.BuyInCents);
	Spec.GuaranteeCents = FMath::Max<int64>(Settings.GuaranteeCents, 0);
	Spec.Entrants = FMath::Max(Settings.Entrants, 2);
	Spec.StartingStack = FMath::Max<int64>(Settings.StartingStack, 1);
	Spec.LevelMinutes = FMath::Max(Settings.LevelMinutes, 0.5);
	Spec.SecondsPerHand = FMath::Max(Settings.SecondsPerHand, 1.0);
	Spec.Population = ShortStackSubsystemDetail::FieldName(Settings.Field);
	Spec.TableSize = FMath::Clamp(Settings.TableSize, 2, 10);
	Spec.StartClock = Settings.StartClockMinutes;

	std::vector<ss::ReservedPlayer> Reserved;
	if (!Settings.RivalName.IsEmpty() && Spec.Entrants >= 3)
	{
		Reserved.push_back({ShortStackConvert::ToStd(Settings.RivalName), ss::Archetype::Crusher});
	}
	const FString Seed = Settings.Seed.IsEmpty() ? FGuid::NewGuid().ToString() : Settings.Seed;
	const FString HeroName = Settings.HeroName.IsEmpty() ? TEXT("Hero") : Settings.HeroName;

	Tournament = MakeUnique<ss::Tournament>(Spec, ShortStackConvert::ToStd(HeroName), ShortStackConvert::ToStd(Seed), Reserved);
	HeroAutopilot = ss::SprintProfile();
	UE_LOG(LogShortStack, Log, TEXT("Started '%s': %d entrants, %d paid, seed %s"), *Settings.Name, Spec.Entrants, Tournament->PaidPlaces(), *Seed);
}

TArray<FShortStackEvent> UShortStackTournamentSubsystem::SimulateRound()
{
	TArray<FShortStackEvent> Events;
	if (!Tournament || Tournament->bFinished)
	{
		return Events;
	}
	ShortStackSubsystemDetail::AppendEvents(Tournament->SimulateTick(&HeroAutopilot), Events);
	return Events;
}

TArray<FShortStackEvent> UShortStackTournamentSubsystem::SimulateRounds(int32 Count)
{
	TArray<FShortStackEvent> Events;
	for (int32 I = 0; I < Count && Tournament && !Tournament->bFinished; ++I)
	{
		ShortStackSubsystemDetail::AppendEvents(Tournament->SimulateTick(&HeroAutopilot), Events);
	}
	return Events;
}

bool UShortStackTournamentSubsystem::IsFinished() const
{
	return Tournament && Tournament->bFinished;
}

FString UShortStackTournamentSubsystem::GetTournamentName() const
{
	return Tournament ? ShortStackConvert::ToUnreal(Tournament->Spec.Name) : FString();
}

int32 UShortStackTournamentSubsystem::GetRound() const
{
	return Tournament ? Tournament->Tick : 0;
}

int32 UShortStackTournamentSubsystem::GetEntrants() const
{
	return Tournament ? Tournament->Spec.Entrants : 0;
}

int32 UShortStackTournamentSubsystem::GetPlayersRemaining() const
{
	return Tournament ? Tournament->Remaining : 0;
}

int32 UShortStackTournamentSubsystem::GetPaidPlaces() const
{
	return Tournament ? Tournament->PaidPlaces() : 0;
}

int64 UShortStackTournamentSubsystem::GetPrizePoolCents() const
{
	return Tournament ? Tournament->PrizePoolCents : 0;
}

int64 UShortStackTournamentSubsystem::GetPrizeForPlace(int32 Place) const
{
	return Tournament ? Tournament->PrizeFor(Place) : 0;
}

bool UShortStackTournamentSubsystem::IsInTheMoney() const
{
	return Tournament && Tournament->InTheMoney();
}

bool UShortStackTournamentSubsystem::IsHandForHand() const
{
	return Tournament && Tournament->HandForHand();
}

int32 UShortStackTournamentSubsystem::GetLevelNumber() const
{
	return Tournament ? Tournament->LevelIndex + 1 : 0;
}

void UShortStackTournamentSubsystem::GetBlinds(int64& SmallBlind, int64& BigBlind, int64& Ante) const
{
	SmallBlind = BigBlind = Ante = 0;
	if (Tournament)
	{
		const ss::Level& L = Tournament->CurrentLevel();
		SmallBlind = L.Sb;
		BigBlind = L.Bb;
		Ante = L.Ante;
	}
}

double UShortStackTournamentSubsystem::GetLevelSecondsLeft() const
{
	return Tournament ? Tournament->LevelSecondsLeft() : 0.0;
}

double UShortStackTournamentSubsystem::GetClockMinutes() const
{
	return Tournament ? Tournament->ClockMinutes() : 0.0;
}

int64 UShortStackTournamentSubsystem::GetAverageStack() const
{
	return Tournament ? static_cast<int64>(Tournament->AverageStack()) : 0;
}

int32 UShortStackTournamentSubsystem::GetHeroRank() const
{
	return Tournament ? Tournament->HeroRank() : 0;
}

int64 UShortStackTournamentSubsystem::GetHeroStack() const
{
	return Tournament ? Tournament->Hero().Stack : 0;
}

bool UShortStackTournamentSubsystem::IsHeroBusted() const
{
	return Tournament && Tournament->Hero().Busted;
}

TArray<FShortStackStanding> UShortStackTournamentSubsystem::GetStandings(int32 MaxCount) const
{
	TArray<FShortStackStanding> Out;
	if (!Tournament)
	{
		return Out;
	}
	const std::vector<const ss::TPlayer*> Standings = Tournament->Standings();
	const int32 Count = MaxCount > 0 ? FMath::Min(MaxCount, static_cast<int32>(Standings.size())) : static_cast<int32>(Standings.size());
	Out.Reserve(Count);
	for (int32 I = 0; I < Count; ++I)
	{
		const ss::TPlayer& P = *Standings[static_cast<size_t>(I)];
		FShortStackStanding& S = Out.AddDefaulted_GetRef();
		S.Rank = I + 1;
		S.PlayerId = ShortStackConvert::ToUnreal(P.Id);
		S.Name = ShortStackConvert::ToUnreal(P.Name);
		S.Stack = P.Stack;
		S.TableId = P.TableId;
		S.bIsHero = P.IsHero;
	}
	return Out;
}
