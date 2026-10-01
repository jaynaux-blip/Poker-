#include "ShortStack/Game/Session.h"
#include "../StrictFloat.h"

#include "ShortStack/AI/Bot.h"
#include "ShortStack/AI/View.h"
#include "ShortStack/Cards.h"
#include "ShortStack/Equity.h"
#include "ShortStack/Evaluator.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <utility>

namespace ss
{
namespace session_detail
{
const uint32_t Gold = 0xf2c14e;
const uint32_t Red = 0xef4d5a;
const uint32_t Orange = 0xf28a3a;
const uint32_t Green = 0x3ecf6e;

std::string Escape(const std::string& In)
{
	std::string Out;
	for (const char Ch : In)
	{
		if (Ch == '\\')
		{
			Out += "\\\\";
		}
		else if (Ch == '\n')
		{
			Out += "\\n";
		}
		else if (Ch == '\t')
		{
			Out += "\\t";
		}
		else
		{
			Out.push_back(Ch);
		}
	}
	return Out;
}

std::string Unescape(const std::string& In)
{
	std::string Out;
	for (size_t I = 0; I < In.size(); ++I)
	{
		if (In[I] == '\\' && I + 1 < In.size())
		{
			const char N = In[++I];
			Out.push_back(N == 'n' ? '\n' : N == 't' ? '\t' : N);
		}
		else
		{
			Out.push_back(In[I]);
		}
	}
	return Out;
}

std::vector<std::string> SplitTabs(const std::string& Line)
{
	std::vector<std::string> Parts;
	std::string Cur;
	for (const char Ch : Line)
	{
		if (Ch == '\t')
		{
			Parts.push_back(Cur);
			Cur.clear();
		}
		else
		{
			Cur.push_back(Ch);
		}
	}
	Parts.push_back(Cur);
	return Parts;
}

std::string CardsText(const std::vector<Card>& Cards)
{
	std::string Out;
	for (size_t I = 0; I < Cards.size(); ++I)
	{
		if (I > 0)
		{
			Out += ' ';
		}
		Out += CardToString(Cards[I]);
	}
	return Out;
}

double OpenThreshold(Position P)
{
	switch (P)
	{
	case Position::UTG: return 0.16;
	case Position::UTG1: return 0.18;
	case Position::MP: return 0.2;
	case Position::LJ: return 0.24;
	case Position::HJ: return 0.28;
	case Position::CO: return 0.36;
	case Position::BTN: return 0.55;
	case Position::SB: return 0.5;
	case Position::BB: return 1.0;
	default: return 0.3;
	}
}
} // namespace session_detail

// ------------------------------------------------------------------ save data

std::string SaveData::Serialize() const
{
	std::ostringstream Out;
	Out << "shortstack.nightone.v1\n";
	Out << "bankroll\t" << BankrollCents << "\n";
	Out << "name\t" << session_detail::Escape(HeroName) << "\n";
	Out << "clock\t" << Fixed(ClockMinutes, 2) << "\n";
	for (const std::string& Key : TextsSeen)
	{
		Out << "text\t" << session_detail::Escape(Key) << "\n";
	}
	for (const HistoryEntry& H : History)
	{
		Out << "result\t" << session_detail::Escape(H.Name) << "\t" << H.Place << "\t" << H.Entrants << "\t" << H.Prize << "\t" << Fixed(H.AccuracyPct, 3) << "\t" << H.BuyInCents << "\t"
			<< session_detail::Escape(H.EventId) << "\n";
	}
	return Out.str();
}

bool SaveData::Parse(const std::string& Text, SaveData& Out)
{
	std::istringstream In(Text);
	std::string Line;
	if (!std::getline(In, Line) || Line != "shortstack.nightone.v1")
	{
		return false;
	}
	SaveData D;
	D.History.clear();
	D.TextsSeen.clear();
	while (std::getline(In, Line))
	{
		const std::vector<std::string> P = session_detail::SplitTabs(Line);
		if (P.size() == 2 && P[0] == "bankroll")
		{
			D.BankrollCents = std::strtoll(P[1].c_str(), nullptr, 10);
		}
		else if (P.size() == 2 && P[0] == "name")
		{
			D.HeroName = session_detail::Unescape(P[1]);
		}
		else if (P.size() == 2 && P[0] == "clock")
		{
			D.ClockMinutes = std::atof(P[1].c_str());
		}
		else if (P.size() == 2 && P[0] == "text")
		{
			D.TextsSeen.push_back(session_detail::Unescape(P[1]));
		}
		else if ((P.size() == 6 || P.size() == 8) && P[0] == "result")
		{
			HistoryEntry H;
			H.Name = session_detail::Unescape(P[1]);
			H.Place = std::atoi(P[2].c_str());
			H.Entrants = std::atoi(P[3].c_str());
			H.Prize = std::strtoll(P[4].c_str(), nullptr, 10);
			H.AccuracyPct = std::atof(P[5].c_str());
			if (P.size() == 8)
			{
				H.BuyInCents = std::strtoll(P[6].c_str(), nullptr, 10);
				H.EventId = session_detail::Unescape(P[7]);
			}
			D.History.push_back(H);
		}
	}
	Out = D;
	return true;
}

const char* SoundName(SoundId Id)
{
	static const char* const Names[] = {"chip", "chips", "card", "flip", "check", "fold", "turn", "win", "level", "bust", "click", "alert", "move", "allin", "bubble", "cash"};
	return Names[static_cast<int>(Id)];
}

// ------------------------------------------------------------------ session

Session::Session(SessionHooks& InHooks, const std::string& Seed, const SaveData* Loaded)
	: Hooks(InHooks), SeedBase(Seed), R(Seed)
{
	if (Loaded)
	{
		BankrollCents = Loaded->BankrollCents;
		HeroName = Loaded->HeroName;
		History = Loaded->History;
		LobbyMinutes = Loaded->ClockMinutes;
		TextsSeen.insert(Loaded->TextsSeen.begin(), Loaded->TextsSeen.end());
	}
}

void Session::Save()
{
	SaveData D;
	D.BankrollCents = BankrollCents;
	D.HeroName = HeroName;
	D.History = History;
	D.ClockMinutes = LobbyMinutes;
	D.TextsSeen.assign(TextsSeen.begin(), TextsSeen.end());
	Hooks.Save(D);
}

void Session::ResetSave()
{
	BankrollCents = 237;
	History.clear();
	TextsSeen.clear();
	LobbyMinutes = 2.0 * 60.0 + 7.0;
	Save();
}

// ------------------------------------------------------------------ story

void Session::StoryText(const std::string& Key, const std::string& From, const std::string& Body, bool Once)
{
	if (Once && TextsSeen.count(Key) > 0)
	{
		return;
	}
	TextsSeen.insert(Key);
	Hooks.Text(From, Body);
	Save();
}

void Session::OnBoot()
{
	StoryText("dee-intro", "Dee", "Heard about the notice on your door. Tuesday game at the laundromat is still on if you need it. Don't play scared.", false);
}

// ------------------------------------------------------------------ lobby

bool Session::CanAfford(const LobbyEvent& Ev) const
{
	return Ev.Joinable && BankrollCents >= Ev.BuyInCents;
}

void Session::Register(int Index)
{
	const std::vector<LobbyEvent>& L = Lobby();
	if (Index < 0 || Index >= static_cast<int>(L.size()))
	{
		return;
	}
	RegisterEvent(L[static_cast<size_t>(Index)]);
}

void Session::RegisterEvent(const LobbyEvent& Listing)
{
	if (!CanAfford(Listing))
	{
		return;
	}
	Joined = Listing;
	// The table opens now, or when the event starts if that's later (the wait passes at the desk).
	Joined.Spec.StartClock = std::max(Joined.Spec.StartClock, LobbyMinutes);
	const LobbyEvent& Ev = Joined;
	BankrollCents -= Ev.BuyInCents;
	Save();
	Event = &Joined;
	Grades.clear();
	Badges.clear();
	Chat.clear();
	HandsPlayed = 0;
	BiggestPot = 0;
	HeroTilt = 0.0;
	HasResults = false;
	HasBustInfo = false;
	RivalArrived = false;
	Sprinting = false;
	TimeBank = 30.0;
	TableId = 0;
	Moving = false;
	CurHand.reset();
	const std::string Seed = Ev.Spec.Id + ":" + SeedBase + ":" + std::to_string(RegisterCount++);
	LastTick = -1.0;
	T = std::make_unique<Tournament>(Ev.Spec, HeroName, Seed, std::vector<ReservedPlayer>{{RivalName, Archetype::Crusher}});
	CurrentScreen = Screen::Table;
	SystemLine("Welcome to " + Ev.Spec.Name + ". " + ChipsText(Ev.Spec.Entrants) + " players, " + std::to_string(T->PaidPlaces()) + " paid.");
	const Level& L0 = T->CurrentLevel();
	SystemLine("Blinds " + std::to_string(L0.Sb) + "/" + std::to_string(L0.Bb) + ", ante " + std::to_string(L0.Ante) + ". Good luck!");
	Hooks.Sound(SoundId::Alert, 1.0);
	StartNextHand();
}

// ------------------------------------------------------------------ chat helpers

void Session::Push(const std::string& Who, const std::string& Text, ChatKind Kind)
{
	ChatLine L;
	L.Who = Who;
	L.Text = Text;
	L.Kind = Kind;
	L.Time = Now;
	Chat.push_back(L);
	if (Chat.size() > 80)
	{
		Chat.erase(Chat.begin(), Chat.begin() + static_cast<std::ptrdiff_t>(Chat.size() - 80));
	}
}

void Session::DealerLine(const std::string& Text)
{
	Push("Dealer", Text, ChatKind::Dealer);
}

void Session::SystemLine(const std::string& Text)
{
	Push("", Text, ChatKind::System);
}

void Session::Say(const std::string& Who, const std::string& Text)
{
	Push(Who, Text, Who == RivalName ? ChatKind::Rival : ChatKind::Player);
}

// ------------------------------------------------------------------ hand lifecycle

std::string Session::SeatName(int Seat) const
{
	if (Seat >= 0 && Seat < static_cast<int>(Seats.size()) && Seats[static_cast<size_t>(Seat)].Present)
	{
		return Seats[static_cast<size_t>(Seat)].Name;
	}
	return "Seat " + std::to_string(Seat + 1);
}

const TPlayer* Session::PlayerById(const std::string& Id) const
{
	if (!T)
	{
		return nullptr;
	}
	const int Idx = T->PlayerIndex(Id);
	return Idx >= 0 ? &T->Players[static_cast<size_t>(Idx)] : nullptr;
}

void Session::BuildSeats()
{
	const TTable& Table = T->Tables.at(TableId);
	Seats.assign(Table.Seats.size(), SeatVis());
	for (size_t S = 0; S < Table.Seats.size(); ++S)
	{
		const int Idx = Table.Seats[S];
		if (Idx < 0)
		{
			continue;
		}
		const TPlayer& P = T->Players[static_cast<size_t>(Idx)];
		SeatVis& V = Seats[S];
		V.Present = true;
		V.Seat = static_cast<int>(S);
		V.Id = P.Id;
		V.Name = P.Name;
		V.IsHero = P.IsHero;
		V.IsRival = P.Name == RivalName;
		V.Stack = P.Stack;
	}
}

void Session::StartNextHand()
{
	if (!T || T->bFinished || T->Hero().Busted)
	{
		return;
	}
	const int PrevTable = TableId;

	// Story: seat the rival at the hero's table once things get serious.
	if (!RivalArrived && HandsPlayed >= 14 && !T->InTheMoney())
	{
		const TPlayer* Rival = PlayerById(RivalId());
		if (Rival && !Rival->Busted)
		{
			T->MoveToTable(RivalId(), T->Hero().TableId);
			RivalArrived = true;
		}
	}

	std::vector<TEvent> Events;
	std::unique_ptr<Hand> H = T->StartTick(Events);
	HandleTourneyEvents(Events);
	if (!H)
	{
		return;
	}
	CurHand = std::move(H);
	TableId = T->Hero().TableId;
	if (PrevTable != 0 && PrevTable != TableId)
	{
		Moving = true;
		MovingFrom = PrevTable;
		MovingTo = TableId;
		MovingAt = Now;
		SystemLine("You have been moved to Table " + std::to_string(TableId) + ".");
		Hooks.Sound(SoundId::Move, 1.0);
	}
	BuildSeats();
	if (RivalArrived)
	{
		bool RivalSeated = false;
		for (const SeatVis& S : Seats)
		{
			RivalSeated = RivalSeated || (S.Present && S.IsRival);
		}
		bool RivalSpoke = false;
		for (const ChatLine& C : Chat)
		{
			RivalSpoke = RivalSpoke || C.Kind == ChatKind::Rival;
		}
		if (RivalSeated && !RivalSpoke)
		{
			Say(RivalName, RivalLine(RivalMoment::Arrive, R));
		}
	}
	ButtonSeat = CurHand->ButtonSeat;
	Board.clear();
	BoardShownAt.clear();
	PotChips = 0;
	Flights.clear();
	Cursor = 0;
	NextAt = Now + (Moving && MovingAt == Now ? 1.6 : 0.25);
	BotPending = false;
	HandDone = false;
	Revealed = false;
	HeroFolded = false;
	HasPrompt = false;
	AutoFolded = false;
	HeroAllInReveal = false;
	SittingOutThisHand = SitOutNext;
	if (SitOutNext)
	{
		HeroTilt *= 0.5;
	}
	SitOutNext = false;
	HeroTilt *= 0.93;
	++HandsPlayed;
	const Level& L = T->CurrentLevel();
	DealerLine("Hand #" + Grouped(T->Tick + 1) + " \xC2\xB7 Blinds " + ChipsText(static_cast<double>(L.Sb)) + "/" + ChipsText(static_cast<double>(L.Bb)));
}

double Session::Speed() const
{
	if (CurrentPace == Pace::Full)
	{
		return 1.0;
	}
	bool HeroIn = false;
	for (const SeatVis& S : Seats)
	{
		HeroIn = HeroIn || (S.Present && S.IsHero && S.HasCards && !S.Folded);
	}
	return HeroFolded || !HeroIn ? 0.22 : 1.0;
}

int Session::HeroSeatIdx() const
{
	if (!CurHand)
	{
		return -1;
	}
	for (size_t I = 0; I < CurHand->Seats.size(); ++I)
	{
		if (CurHand->Seats[I].Id == HeroId)
		{
			return static_cast<int>(I);
		}
	}
	return -1;
}

void Session::Update(double InNow)
{
	// The lobby clock runs in real time; tournaments run on their own clock.
	if (LastTick >= 0.0 && !T && InNow > LastTick)
	{
		LobbyMinutes += std::min(InNow - LastTick, 1.0) / 60.0;
	}
	LastTick = InNow;
	Now = InNow;
	for (size_t I = Flights.size(); I-- > 0;)
	{
		if (!(Now < Flights[I].Start + Flights[I].Dur + 0.05))
		{
			Flights.erase(Flights.begin() + static_cast<std::ptrdiff_t>(I));
		}
	}
	for (size_t I = Badges.size(); I-- > 0;)
	{
		if (!(Now < Badges[I].At + 3.2))
		{
			Badges.erase(Badges.begin() + static_cast<std::ptrdiff_t>(I));
		}
	}
	if (CurrentBanner.Active && Now > CurrentBanner.At + 3.2)
	{
		CurrentBanner.Active = false;
	}
	if (Moving && Now > MovingAt + 2.2)
	{
		Moving = false;
	}

	if (CurrentScreen != Screen::Table || !T)
	{
		return;
	}
	if (Sprinting)
	{
		SprintStep();
		return;
	}
	if (HasBustInfo)
	{
		if (Now > BustAt)
		{
			ShowResults();
		}
		return;
	}
	if (!CurHand)
	{
		return;
	}
	Hand& H = *CurHand;

	// Idle chatter.
	if (Now - LastIdleChat > 50.0 && R.Chance(0.004))
	{
		LastIdleChat = Now;
		std::vector<const SeatVis*> Others;
		for (const SeatVis& S : Seats)
		{
			if (S.Present && !S.IsHero && !S.IsRival)
			{
				Others.push_back(&S);
			}
		}
		if (!Others.empty())
		{
			const std::string Who = R.Pick(Others)->Name;
			Say(Who, IdleLine(R));
		}
	}

	// Hero timer.
	if (HasPrompt)
	{
		if (Now > Prompt.Deadline)
		{
			if (Prompt.TimeBankUntil == 0.0 && TimeBank >= 1.0)
			{
				// Time bank kicks in automatically.
				Prompt.TimeBankUntil = Now + TimeBank;
				SystemLine("Time bank activated.");
				Hooks.Sound(SoundId::Alert, 1.0);
			}
			else if (Prompt.TimeBankUntil == 0.0 || Now > Prompt.TimeBankUntil)
			{
				HeroAct(Prompt.CanCheck ? PlayerAction::Check() : PlayerAction::Fold(), true);
			}
		}
		return;
	}

	int Guard = 0;
	while (Now >= NextAt && Guard++ < 50)
	{
		if (Cursor < H.Events.size())
		{
			const HandEvent Ev = H.Events[Cursor++];
			NextAt = Now + Consume(Ev) * Speed();
			continue;
		}
		if (H.bComplete)
		{
			if (!HandDone)
			{
				HandDone = true;
				NextAt = Now + 1.4 * Max(0.5, Speed());
				continue;
			}
			EndHand();
			return;
		}
		const int Idx = H.ToAct;
		if (Idx < 0)
		{
			return;
		}
		SeatVis& Vis = Seats[static_cast<size_t>(H.Seats[static_cast<size_t>(Idx)].Seat)];
		if (H.Seats[static_cast<size_t>(Idx)].Id == HeroId)
		{
			OpenHeroTurn();
			return;
		}
		if (!BotPending)
		{
			const BotDecision D = T->BotDecisionFor(H, false);
			const double Think = (D.ThinkMs / 1000.0) * (Speed() < 1.0 ? 0.12 : 0.55);
			BotPending = true;
			BotAction = D.Action;
			BotAt = Now + Think;
			BotSeatIdx = Idx;
			Vis.Acting = true;
			Vis.ActStart = Now;
			Vis.ActEnd = Now + Max(Think, 0.4);
			NextAt = Now + Think;
			continue;
		}
		Vis.Acting = false;
		BotPending = false;
		H.Act(BotAction);
	}
}

// ------------------------------------------------------------------ hero turn

bool Session::ShouldAutoFold()
{
	if (SittingOutThisHand)
	{
		return true;
	}
	if (CurrentPace == Pace::Full)
	{
		return false;
	}
	const Hand& H = *CurHand;
	const int Idx = HeroSeatIdx();
	if (H.CurrentStreet != Street::Preflop)
	{
		return false;
	}
	const PlayerView View = MakeView(H, Idx);
	const double Pct = HandPercentile(View.Hole[0], View.Hole[1]);
	const StreetSummary Sum = PreflopSummary(View);
	const double Bb = static_cast<double>(H.BigBlind);
	const PublicSeat& Me = View.Seats[static_cast<size_t>(Idx)];
	const double StackBB = static_cast<double>(Me.Stack + Me.StreetBet) / Bb;
	if (View.CanCheck)
	{
		return false; // free option: always let the hero decide
	}
	const Position Pos = PositionOf(View, Idx);
	if (StackBB <= 15.0 && Sum.Raises == 0)
	{
		return Pct > PushRange(StackBB, PlayersBehind(View, Idx)) * 1.4;
	}
	if (Sum.Raises == 0)
	{
		return Pct > session_detail::OpenThreshold(Pos);
	}
	if (Idx == H.BigBlindIndex())
	{
		return Pct > 0.5;
	}
	return Pct > 0.22;
}

void Session::OpenHeroTurn()
{
	Hand& H = *CurHand;
	const int Idx = HeroSeatIdx();
	const LegalActions Legal = H.GetLegalActions();
	if (SittingOutThisHand && Legal.CanCheck)
	{
		H.Act(PlayerAction::Check());
		NextAt = Now + 0.15;
		return;
	}
	if (ShouldAutoFold())
	{
		AutoFolded = true;
		AutoFoldedCards = H.Seats[static_cast<size_t>(Idx)].Hole;
		AutoFoldedAt = Now;
		H.Act(PlayerAction::Fold());
		if (!SittingOutThisHand)
		{
			DealerLine("Auto-folded " + session_detail::CardsText(AutoFoldedCards));
		}
		HeroFolded = true;
		NextAt = Now + 0.15;
		return;
	}
	const PlayerView View = MakeView(H, Idx);
	Prompt = HeroPrompt();
	Prompt.Analysis = AnalyzeDecision(View, R, 1200);
	const double Bb = static_cast<double>(H.BigBlind);
	double RaiseTo = 0.0;
	if (Legal.IsBet)
	{
		RaiseTo = Max(static_cast<double>(Legal.MinRaiseTo), JsRound((static_cast<double>(H.Pot()) * 0.5) / (Bb / 2.0)) * (Bb / 2.0));
	}
	else if (H.CurrentStreet == Street::Preflop)
	{
		RaiseTo = Max(static_cast<double>(Legal.MinRaiseTo), JsRound(static_cast<double>(H.CurrentBet) * 2.5));
	}
	else
	{
		RaiseTo = Max(static_cast<double>(Legal.MinRaiseTo), JsRound(static_cast<double>(H.CurrentBet) * 3.0));
	}
	RaiseTo = Min(RaiseTo, static_cast<double>(Legal.MaxRaiseTo));
	const double TiltPenalty = 1.0 - Min(0.4, HeroTilt * 0.5);
	Prompt.OpenedAt = Now;
	Prompt.Deadline = Now + 20.0 * TiltPenalty;
	Prompt.TimeBankUntil = 0.0;
	Prompt.RaiseTo = static_cast<Chips>(RaiseTo);
	Prompt.MinRaise = Legal.MinRaiseTo;
	Prompt.MaxRaise = Legal.MaxRaiseTo;
	Prompt.ToCall = Legal.CallAmount;
	Prompt.CanCheck = Legal.CanCheck;
	Prompt.CanRaise = Legal.CanRaise;
	Prompt.IsBet = Legal.IsBet;
	Prompt.Pot = H.Pot();
	Prompt.BigBlind = H.BigBlind;
	Prompt.OnStreet = H.CurrentStreet;
	HasPrompt = true;
	SeatVis& Vis = Seats[static_cast<size_t>(H.Seats[static_cast<size_t>(Idx)].Seat)];
	Vis.Acting = true;
	Vis.ActStart = Now;
	Vis.ActEnd = Prompt.Deadline;
	Hooks.Sound(SoundId::Turn, 1.0);
}

void Session::HeroAct(const PlayerAction& Action, bool TimedOut)
{
	if (!HasPrompt || !CurHand)
	{
		return;
	}
	Hand& H = *CurHand;
	const int Idx = HeroSeatIdx();
	const LegalActions Legal = H.GetLegalActions();
	PlayerAction A = Action;
	if (A.Type == PlayerAction::Kind::Raise)
	{
		if (!Legal.CanRaise)
		{
			A = Legal.CallAmount > 0 ? PlayerAction::Call() : PlayerAction::Check();
		}
		else
		{
			A = PlayerAction::RaiseTo(Max(static_cast<double>(Legal.MinRaiseTo), Min(static_cast<double>(Legal.MaxRaiseTo), JsRound(A.To))));
		}
	}
	if (A.Type == PlayerAction::Kind::Check && !Legal.CanCheck)
	{
		A = PlayerAction::Call();
	}
	if (Prompt.TimeBankUntil > 0.0)
	{
		TimeBank = Max(0.0, TimeBank - (Now - Prompt.Deadline));
	}
	GradeBadge Badge;
	Badge.G = GradeDecision(Prompt.Analysis, A);
	Badge.At = Now;
	Grades.push_back(Badge.G);
	Badges.push_back(Badge);
	if (TimedOut)
	{
		SystemLine("You ran out of time.");
	}
	HasPrompt = false;
	Seats[static_cast<size_t>(H.Seats[static_cast<size_t>(Idx)].Seat)].Acting = false;
	if (A.Type == PlayerAction::Kind::Fold)
	{
		HeroFolded = true;
	}
	H.Act(A);
	NextAt = Now;
}

void Session::RequestSitOut()
{
	SitOutNext = true;
	SystemLine("You will sit out the next hand. Breathe.");
}

// ------------------------------------------------------------------ event playback

double Session::Consume(const HandEvent& Ev)
{
	Hand& H = *CurHand;
	auto SeatAt = [&](int S) -> SeatVis& { return Seats[static_cast<size_t>(S)]; };
	auto AddFlight = [&](bool IsChips, FlightEnd From, FlightEnd To, Chips Amount, double Start, double Dur) {
		Flight F;
		F.IsChips = IsChips;
		F.From = From;
		F.To = To;
		F.Amount = Amount;
		F.Start = Start;
		F.Dur = Dur;
		Flights.push_back(F);
	};
	switch (Ev.Type)
	{
	case EventType::Blind:
	{
		SeatVis& S = SeatAt(Ev.Seat);
		S.Stack -= Ev.Amount;
		S.Bet += Ev.Amount;
		AddFlight(true, FlightEnd::AtSeat(Ev.Seat), FlightEnd::AtBet(Ev.Seat), Ev.Amount, Now, 0.3);
		return 0.12;
	}
	case EventType::Ante:
	{
		SeatVis& S = SeatAt(Ev.Seat);
		S.Stack -= Ev.Amount;
		PotChips += Ev.Amount;
		AddFlight(true, FlightEnd::AtSeat(Ev.Seat), FlightEnd::AtPot(), Ev.Amount, Now, 0.35);
		Hooks.Sound(SoundId::Chip, 0.5);
		return 0.25;
	}
	case EventType::Deal:
	{
		int K = 0;
		for (int Round = 0; Round < 2; ++Round)
		{
			for (const int SeatNo : Ev.Seats)
			{
				SeatVis& S = SeatAt(SeatNo);
				S.HasCards = true;
				S.DealtAt = Now;
				AddFlight(false, FlightEnd::AtDeck(), FlightEnd::AtSeat(SeatNo), Round, Now + K * 0.05 * Speed(), 0.28);
				++K;
			}
		}
		for (const HandSeat& HS : H.Seats)
		{
			if (HS.Id == HeroId)
			{
				SeatAt(HS.Seat).Hole = HS.Hole;
			}
		}
		Hooks.Sound(SoundId::Deal, 1.0);
		return 0.2 + K * 0.05;
	}
	case EventType::Action:
	{
		const ActionRecord& A = Ev.Action;
		SeatVis& S = SeatAt(A.Seat);
		S.Stack -= A.Added;
		S.Bet = A.To;
		S.AllIn = A.AllIn;
		S.LastActionAt = Now;
		std::string Verb;
		std::string Line;
		switch (A.Type)
		{
		case ActionType::Fold:
			Verb = "Fold";
			Line = S.Name + " folds";
			break;
		case ActionType::Check:
			Verb = "Check";
			Line = S.Name + " checks";
			break;
		case ActionType::Call:
			Verb = "Call " + ChipsText(static_cast<double>(A.Added));
			Line = S.Name + " calls " + ChipsText(static_cast<double>(A.Added));
			break;
		case ActionType::Bet:
			Verb = "Bet " + ChipsText(static_cast<double>(A.To));
			Line = S.Name + " bets " + ChipsText(static_cast<double>(A.To));
			break;
		default:
			Verb = "Raise " + ChipsText(static_cast<double>(A.To));
			Line = S.Name + " raises to " + ChipsText(static_cast<double>(A.To));
			break;
		}
		S.LastAction = A.AllIn ? "All-in" : Verb;
		if (A.Type == ActionType::Fold)
		{
			S.Folded = true;
			AddFlight(false, FlightEnd::AtSeat(A.Seat), FlightEnd::AtMuck(), 0, Now, 0.3);
			Hooks.Sound(SoundId::Fold, 0.6);
		}
		else if (A.Type == ActionType::Check)
		{
			Hooks.Sound(SoundId::Check, 1.0);
		}
		else
		{
			AddFlight(true, FlightEnd::AtSeat(A.Seat), FlightEnd::AtBet(A.Seat), A.Added, Now, 0.3);
			Hooks.Sound(A.AllIn ? SoundId::AllIn : SoundId::ChipStack, 1.0);
		}
		if (A.Type != ActionType::Fold || Speed() == 1.0)
		{
			DealerLine(Line + (A.AllIn ? " and is all-in" : ""));
		}
		return A.Type == ActionType::Fold ? 0.25 : 0.45;
	}
	case EventType::Street:
	{
		const bool Collected = CollectBets();
		const double Base = Collected ? 0.35 : 0.05;
		for (size_t I = 0; I < Ev.Cards.size(); ++I)
		{
			Board.push_back(Ev.Cards[I]);
			BoardShownAt.push_back(Now + Base * Speed() + static_cast<double>(I) * 0.12 * Speed());
		}
		for (SeatVis& S : Seats)
		{
			if (S.Present && !S.AllIn && !S.Folded)
			{
				S.LastAction.clear();
			}
		}
		DealerLine(std::string(StreetTitle(Ev.NewStreet)) + ": " + session_detail::CardsText(Ev.Board));
		Hooks.Sound(SoundId::Flip, 1.0);
		if (Revealed)
		{
			UpdateEquities();
		}
		return Base + 0.55 + static_cast<double>(Ev.Cards.size()) * 0.12 + (Revealed ? 1.1 : 0.0);
	}
	case EventType::Return:
	{
		SeatVis& S = SeatAt(Ev.Seat);
		S.Bet -= Ev.Amount;
		S.Stack += Ev.Amount;
		return 0.1;
	}
	case EventType::Reveal:
	{
		Revealed = true;
		CollectBets();
		bool HeroIn = false;
		for (const HandSeat& HS : H.Seats)
		{
			if (HS.Folded)
			{
				continue;
			}
			SeatAt(HS.Seat).Hole = HS.Hole;
			HeroIn = HeroIn || HS.Id == HeroId;
		}
		UpdateEquities();
		if (HeroIn)
		{
			HeroAllInReveal = true;
			Hooks.Heartbeat(true);
		}
		Hooks.Sound(SoundId::Flip, 1.0);
		return 1.6;
	}
	case EventType::Showdown:
	{
		CollectBets();
		for (const ShowdownHand& SH : Ev.Hands)
		{
			SeatVis& S = SeatAt(SH.Seat);
			S.Hole = SH.Hole;
			S.HandLabel = Describe(SH.Score);
		}
		Hooks.Sound(SoundId::Flip, 1.0);
		return 1.1;
	}
	case EventType::Award:
	{
		if (Ev.PotIndex == 0)
		{
			CollectBets();
			PotBeforeAward = PotChips;
		}
		const PotResult& P = Ev.Pot;
		bool HeroWon = false;
		std::string Names;
		for (size_t K = 0; K < P.Winners.size(); ++K)
		{
			const int W = P.Winners[K];
			SeatVis& S = SeatAt(W);
			S.Winner = true;
			S.Stack += P.Shares[K];
			AddFlight(true, FlightEnd::AtPot(), FlightEnd::AtSeat(W), P.Shares[K], Now, 0.6);
			HeroWon = HeroWon || S.IsHero;
			Names += (K > 0 ? " & " : "") + SeatName(W);
		}
		PotChips = MaxChips(0, PotChips - P.Amount);
		const std::string Label = P.Winners.size() == 1 ? SeatAt(P.Winners[0]).HandLabel : std::string();
		const std::string PotName = Ev.PotCount > 1 ? (Ev.PotIndex == 0 ? std::string("the main pot") : "side pot " + std::to_string(Ev.PotIndex)) : std::string("the pot");
		DealerLine(Names + " wins " + PotName + " (" + ChipsText(static_cast<double>(P.Amount)) + ")" + (Label.empty() ? "" : " with " + Label));
		Hooks.Sound(HeroWon ? SoundId::Win : SoundId::ChipStack, HeroWon ? 1.0 : 0.6);
		if (HeroWon)
		{
			BiggestPot = MaxChips(BiggestPot, P.Amount);
		}
		if (Ev.PotIndex == Ev.PotCount - 1)
		{
			AfterAward();
		}
		return 0.9;
	}
	case EventType::End:
	default:
		Hooks.Heartbeat(false);
		return 0.1;
	}
}

bool Session::CollectBets()
{
	bool Any = false;
	for (SeatVis& S : Seats)
	{
		if (S.Present && S.Bet > 0)
		{
			Flight F;
			F.IsChips = true;
			F.From = FlightEnd::AtBet(S.Seat);
			F.To = FlightEnd::AtPot();
			F.Amount = S.Bet;
			F.Start = Now;
			F.Dur = 0.35;
			Flights.push_back(F);
			PotChips += S.Bet;
			S.Bet = 0;
			Any = true;
		}
	}
	if (Any)
	{
		Hooks.Sound(SoundId::ChipStack, 0.5);
	}
	return Any;
}

void Session::UpdateEquities()
{
	const Hand& H = *CurHand;
	std::vector<std::vector<Card>> Holes;
	std::vector<int> SeatNos;
	for (const HandSeat& HS : H.Seats)
	{
		if (!HS.Folded)
		{
			Holes.push_back(HS.Hole);
			SeatNos.push_back(HS.Seat);
		}
	}
	const std::vector<double> Eq = EquityKnownHands(Holes, Board, R, 6000);
	for (size_t I = 0; I < SeatNos.size() && I < Eq.size(); ++I)
	{
		SeatVis& S = Seats[static_cast<size_t>(SeatNos[I])];
		S.HasEquity = true;
		S.Equity = Eq[I];
	}
}

/** Chat reactions, bad beats and tilt after a pot is decided. */
void Session::AfterAward()
{
	const Hand& H = *CurHand;
	std::set<int> Winners;
	for (const PotResult& P : H.PotResults)
	{
		Winners.insert(P.Winners.begin(), P.Winners.end());
	}
	const HandSeat* HeroSeat = nullptr;
	const HandSeat* RivalSeat = nullptr;
	for (const HandSeat& HS : H.Seats)
	{
		if (HS.Id == HeroId)
		{
			HeroSeat = &HS;
		}
		if (HS.Id == RivalId() && !HS.Folded)
		{
			RivalSeat = &HS;
		}
	}
	const bool HeroWasIn = HeroSeat && !HeroSeat->Folded;
	const bool BigPot = PotBeforeAward > H.BigBlind * 20;

	if (HeroWasIn)
	{
		const SeatVis& HeroVis = Seats[static_cast<size_t>(HeroSeat->Seat)];
		const bool Lost = Winners.count(HeroSeat->Seat) == 0;
		if (Lost && HeroVis.HasEquity && HeroVis.Equity >= 0.6)
		{
			HeroTilt = Min(1.0, HeroTilt + 0.45);
			SystemLine("Bad beat. You were " + std::to_string(static_cast<int>(JsRound(HeroVis.Equity * 100.0))) + "% to win.");
		}
		else if (Lost && BigPot)
		{
			HeroTilt = Min(1.0, HeroTilt + 0.2);
		}
		else if (!Lost)
		{
			HeroTilt = Max(0.0, HeroTilt - 0.15);
		}
		// Rival reactions.
		if (RivalSeat)
		{
			const bool RivalWon = Winners.count(RivalSeat->Seat) > 0;
			if (RivalWon && Lost)
			{
				Say(RivalName, RivalLine(RivalMoment::WinVsHero, R));
			}
			else if (!Lost && !RivalWon && R.Chance(0.6))
			{
				Say(RivalName, RivalLine(RivalMoment::LoseVsHero, R));
			}
		}
	}
	if (BigPot && R.Chance(0.55))
	{
		const HandSeat* Loser = nullptr;
		const HandSeat* WinnerNpc = nullptr;
		for (const HandSeat& HS : H.Seats)
		{
			const bool Npc = HS.Id != HeroId && HS.Id != RivalId();
			const bool Won = Winners.count(HS.Seat) > 0;
			if (!Loser && Npc && !HS.Folded && !Won)
			{
				Loser = &HS;
			}
			if (!WinnerNpc && Npc && Won)
			{
				WinnerNpc = &HS;
			}
		}
		if (Loser && R.Chance(0.6))
		{
			const std::string Who = SeatName(Loser->Seat);
			Say(Who, Salt(R));
		}
		else if (WinnerNpc)
		{
			const std::string Who = SeatName(WinnerNpc->Seat);
			Say(Who, Brag(R));
		}
		else if (HeroWasIn)
		{
			for (const HandSeat& HS : H.Seats)
			{
				if (HS.Id != HeroId && Winners.count(HS.Seat) == 0)
				{
					const std::string Who = SeatName(HS.Seat);
					Say(Who, Nice(R));
					break;
				}
			}
		}
	}
}

void Session::BustBanner(const TPlayer& Hero, const char* NoCashSub)
{
	const bool Cashed = Hero.PrizeCents > 0;
	CurrentBanner.Active = true;
	CurrentBanner.Title = Cashed ? "You finished " + Ordinal(Hero.Place) : "Eliminated in " + Ordinal(Hero.Place);
	CurrentBanner.Sub = Cashed ? "Won " + Money(Hero.PrizeCents) : NoCashSub;
	CurrentBanner.At = Now;
	CurrentBanner.Color = Cashed ? session_detail::Gold : session_detail::Red;
}

void Session::EndHand()
{
	Hand& H = *CurHand;
	const std::vector<TEvent> Events = T->FinishTick(&H);
	Hooks.Heartbeat(false);
	HandleTourneyEvents(Events);
	// Busted players at the hero's table say goodbye.
	for (const TEvent& E : Events)
	{
		if (E.Type == TEventType::Bust && !E.IsHero && E.TableId == TableId && R.Chance(0.45))
		{
			if (E.Name == RivalName)
			{
				Say(RivalName, RivalLine(RivalMoment::BustsSelf, R));
			}
			else
			{
				Say(E.Name, BustLine(R));
			}
		}
	}
	const TPlayer& Hero = T->Hero();
	if (Hero.Busted)
	{
		bool RivalWon = false;
		for (const PotResult& P : H.PotResults)
		{
			for (const int W : P.Winners)
			{
				const HandSeat* S = H.SeatByNumber(W);
				RivalWon = RivalWon || (S && S->Id == RivalId());
			}
		}
		if (RivalWon)
		{
			Say(RivalName, RivalLine(RivalMoment::HeroBust, R));
		}
		HasBustInfo = true;
		BustPlace = Hero.Place;
		BustPrize = Hero.PrizeCents;
		BustAt = Now + 3.0;
		Hooks.Sound(SoundId::Bust, 1.0);
		BustBanner(Hero, "Better luck next time");
		return;
	}
	if (T->bFinished)
	{
		HasBustInfo = true;
		BustPlace = 1;
		BustPrize = Hero.PrizeCents;
		BustAt = Now + 4.0;
		Hooks.Celebrate();
		CurrentBanner.Active = true;
		CurrentBanner.Title = "YOU WON THE TOURNAMENT";
		CurrentBanner.Sub = Money(Hero.PrizeCents);
		CurrentBanner.At = Now;
		CurrentBanner.Color = session_detail::Gold;
		StoryText("won-" + T->Spec.Id, "Dee", "You did WHAT?! Pay the rent. Then come see me. We need to talk about your future.", false);
		return;
	}
	// Hourly can of energy drink.
	const int HoursIn = static_cast<int>(std::floor((T->ClockMinutes() - T->Spec.StartClock) / 50.0));
	while (CansShown <= HoursIn)
	{
		++CansShown;
		Hooks.AddCan();
	}
	if (CurrentPace == Pace::Sprint)
	{
		BeginSprint();
		return;
	}
	StartNextHand();
}

void Session::HandleTourneyEvents(const std::vector<TEvent>& Events)
{
	for (const TEvent& E : Events)
	{
		switch (E.Type)
		{
		case TEventType::Level:
			SystemLine("Blinds are now " + ChipsText(static_cast<double>(E.Blinds.Sb)) + "/" + ChipsText(static_cast<double>(E.Blinds.Bb)) + ", ante " + ChipsText(static_cast<double>(E.Blinds.Ante)) + ".");
			TimeBank = Min(60.0, TimeBank + 5.0);
			LastLevelUpAt = Now;
			Hooks.Sound(SoundId::Level, 1.0);
			break;
		case TEventType::HandForHand:
			SystemLine("We are on the bubble. Hand-for-hand play.");
			CurrentBanner.Active = true;
			CurrentBanner.Title = "THE BUBBLE";
			CurrentBanner.Sub = std::to_string(T->Remaining - T->PaidPlaces()) + " away from the money";
			CurrentBanner.At = Now;
			CurrentBanner.Color = session_detail::Orange;
			Hooks.Sound(SoundId::Bubble, 1.0);
			if (!T->Hero().Busted)
			{
				StoryText("dee-bubble", "Dee", "Bubble? Breathe. Fold the trash, shove the good ones. Nobody remembers who min-cashed scared.");
			}
			break;
		case TEventType::Bubble:
			SystemLine(E.Name + " bursts the bubble. Everyone left is in the money!");
			if (!T->Hero().Busted)
			{
				CurrentBanner.Active = true;
				CurrentBanner.Title = "IN THE MONEY";
				CurrentBanner.Sub = "Min cash " + Money(T->Payouts.back());
				CurrentBanner.At = Now;
				CurrentBanner.Color = session_detail::Green;
				Hooks.Sound(SoundId::Cash, 1.0);
				StoryText("dee-itm", "Dee", "In the money. Now play to win, not to min-cash.");
			}
			break;
		case TEventType::FinalTable:
			if (!T->Hero().Busted)
			{
				SystemLine("Final table! Good luck to all nine players.");
				CurrentBanner.Active = true;
				CurrentBanner.Title = "FINAL TABLE";
				CurrentBanner.Sub = Ordinal(9) + " pays " + Money(T->PrizeFor(9)) + " \xC2\xB7 1st pays " + Money(T->PrizeFor(1));
				CurrentBanner.At = Now;
				CurrentBanner.Color = session_detail::Gold;
				Hooks.Sound(SoundId::Bubble, 1.0);
				StoryText("dee-ft", "Dee", "A final table? At this hour? Call me when it is over. Win or lose.");
			}
			break;
		default:
			break;
		}
	}
}

// ------------------------------------------------------------------ sprint

void Session::BeginSprint()
{
	Sprinting = true;
	SprintStopReason.clear();
	HasPrompt = false;
	SprintBubble = T->Remaining <= T->PaidPlaces() + 15;
	SprintFinal = T->Tables.size() == 1;
}

void Session::StopSprint(const std::string& Reason)
{
	Sprinting = false;
	CurrentPace = Pace::Smart;
	SprintStopReason = Reason;
	SystemLine("Sprint stopped: " + Reason + ".");
	if (!T->Hero().Busted && !T->bFinished)
	{
		StartNextHand();
	}
}

void Session::SprintStep()
{
	const Profile Autopilot = SprintProfile();
	const auto T0 = std::chrono::steady_clock::now();
	auto ElapsedMs = [&]() { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count(); };
	do
	{
		const std::vector<TEvent> Events = T->SimulateTick(&Autopilot);
		++HandsPlayed;
		HandleTourneyEvents(Events);
		const TPlayer& Hero = T->Hero();
		if (Hero.Busted)
		{
			Sprinting = false;
			HasBustInfo = true;
			BustPlace = Hero.Place;
			BustPrize = Hero.PrizeCents;
			BustAt = Now + 2.5;
			Hooks.Sound(SoundId::Bust, 1.0);
			BustBanner(Hero, "Busted while sprinting");
			return;
		}
		if (T->bFinished)
		{
			Sprinting = false;
			HasBustInfo = true;
			BustPlace = 1;
			BustPrize = Hero.PrizeCents;
			BustAt = Now + 3.0;
			Hooks.Celebrate();
			return;
		}
		if (!SprintBubble && T->Remaining <= T->PaidPlaces() + 15)
		{
			StopSprint("approaching the bubble");
			return;
		}
		if (!SprintFinal && T->Tables.size() == 1)
		{
			StopSprint("final table");
			return;
		}
	} while (ElapsedMs() < SprintBudgetMs);
}

// ------------------------------------------------------------------ results

void Session::ShowResults()
{
	BankrollCents += BustPrize;
	const double Acc = Grades.empty() ? 0.0 : Accuracy(Grades);
	LastResults = ss::Results();
	LastResults.EventName = T->Spec.Name;
	LastResults.Place = BustPlace;
	LastResults.Entrants = T->Spec.Entrants;
	LastResults.PrizeCents = BustPrize;
	LastResults.BuyInCents = T->Spec.BuyInCents;
	LastResults.Hands = HandsPlayed;
	LastResults.Grades = Grades;
	LastResults.AccuracyPct = Acc;
	LastResults.BiggestPot = BiggestPot;
	LastResults.Won = BustPlace == 1;
	HasResults = true;
	LobbyMinutes = std::max(LobbyMinutes, T->ClockMinutes());
	HistoryEntry Entry;
	Entry.Name = T->Spec.Name;
	Entry.Place = BustPlace;
	Entry.Entrants = T->Spec.Entrants;
	Entry.Prize = BustPrize;
	Entry.AccuracyPct = Acc;
	Entry.BuyInCents = T->Spec.BuyInCents;
	Entry.EventId = T->Spec.Id.find('@') != std::string::npos ? T->Spec.Id : std::string();
	History.insert(History.begin(), Entry);
	if (History.size() > 100)
	{
		History.resize(100);
	}
	HasBustInfo = false;
	CurHand.reset();
	CurrentScreen = Screen::Results;
	ResultsAt = Now;
	Hooks.Heartbeat(false);
	Save();
	if (BankrollCents < 25 && BustPrize == 0)
	{
		StoryText("broke", "Dee", "Broke? It happens to everybody. Freeroll's always running. Or come by Tuesday. Bring quarters for the dryers.", false);
	}
	else if (BustPrize > 0)
	{
		StoryText("first-cash", "Landlord", "Saw your light on all night. Rent + late fee is $1,225. Friday.");
	}
	// Series results reach the people who follow the boards.
	const bool SeriesEvent = T->Spec.Name.rfind("MM #", 0) == 0;
	if (SeriesEvent && BustPlace == 1)
	{
		StoryText("mm-title", "Dee", "Your name is on the Micro Madness leaderboard. A TITLE. The whole laundromat is refreshing the page.");
		StoryText("mm-title-rival", RivalName, "gg on the title. enjoy it. the RCOP satellites start at $2.20, see you on the steps");
	}
	else if (SeriesEvent && BustPlace <= 9)
	{
		StoryText("mm-final", "Dee", "Final table in a Micro Madness event?? They put those on the news page. Screenshot it.");
	}
}

void Session::LeaveResults()
{
	if (T)
	{
		LobbyMinutes = std::max(LobbyMinutes, T->ClockMinutes());
	}
	CurrentScreen = Screen::Lobby;
	HasResults = false;
	T.reset();
	CurHand.reset();
	Seats.clear();
}

double Session::ClockMinutes() const
{
	return T ? T->ClockMinutes() : LobbyMinutes;
}

double Session::WorldMinutes() const
{
	return net::MinutesPerDay * static_cast<double>(net::NightOneDay) + ClockMinutes();
}
} // namespace ss
