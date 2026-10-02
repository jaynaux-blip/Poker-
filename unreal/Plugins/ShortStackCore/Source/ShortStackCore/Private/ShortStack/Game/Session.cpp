#include "ShortStack/Game/Session.h"
#include "../StrictFloat.h"

#include "ShortStack/AI/Bot.h"
#include "ShortStack/AI/View.h"
#include "ShortStack/Cards.h"
#include "ShortStack/Equity.h"
#include "ShortStack/Evaluator.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Handles.h"
#include "ShortStack/Game/Live.h"
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

/**
 * The game playing the player's hands (Sprint): safe on paper and a step behind good play. It calls too wide and too
 * long, rarely bluffs, shoves short stacks by feel and half-ignores the bubble, so it finishes like an average player
 * while playing well finishes clearly ahead. The engine's SprintProfile stays as it is (TypeScript parity).
 */
Profile Autopilot()
{
	Profile P = SprintProfile();
	P.Label = "Autopilot";
	P.CallWidth = 1.35;
	P.Stickiness = 0.35;
	P.Aggression = 0.6;
	P.Bluff = 0.12;
	P.PushFold = 0.65;
	P.IcmAware = 0.45;
	return P;
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
	const life::State& L = Life;
	Out << "life\tenergy\t" << Fixed(L.Energy, 2) << "\n";
	Out << "life\theat\t" << Fixed(L.Heat, 2) << "\n";
	Out << "life\trent\t" << static_cast<int>(L.RentStage) << "\t" << L.RentDueCents << "\t" << Fixed(L.RentDeadline, 2) << "\t" << L.RentsPaid << "\n";
	Out << "life\tdebt\t" << L.DebtCents << "\n";
	Out << "life\tban\t" << Fixed(L.BannedUntil, 2) << "\n";
	Out << "life\tcount\t" << L.Shifts << "\t" << L.Runs << "\t" << L.Busts << "\t" << L.Ghosts << "\t" << L.Bans << "\n";
	Out << "life\tearned\t" << L.EarnedJobs << "\t" << L.EarnedHustles << "\n";
	Out << "life\tbackroom\t" << L.BackRoomNights << "\t" << L.BackRoomNetCents << "\n";
	Out << "life\tlive\t" << L.LiveEvents << "\t" << L.LiveCashes << "\t" << L.LiveBestPlace << "\t" << L.LiveWonCents << "\n";
	for (const auto& Rd : L.Reads)
	{
		Out << "read\t" << session_detail::Escape(Rd.first) << "\t" << Rd.second << "\n";
	}
	for (const std::string& U : L.Unlocks)
	{
		Out << "unlock\t" << session_detail::Escape(U) << "\n";
	}
	for (const auto& Tk : L.Tickets)
	{
		Out << "ticket\t" << session_detail::Escape(Tk.first) << "\t" << Tk.second << "\n";
	}
	for (const int Night : L.NightsPaid)
	{
		Out << "night\t" << Night << "\n";
	}
	for (auto It = L.Ledger.rbegin(); It != L.Ledger.rend(); ++It)
	{
		Out << "ledger\t" << Fixed(It->At, 2) << "\t" << It->Kind << "\t" << It->Amount << "\t" << session_detail::Escape(It->Label) << "\n";
	}
	for (const std::string& Key : TextsSeen)
	{
		Out << "text\t" << session_detail::Escape(Key) << "\n";
	}
	for (const auto& G : Gear)
	{
		Out << "gear\t" << session_detail::Escape(G.first) << "\t" << Fixed(G.second, 2) << "\n";
	}
	const kast::Channel& K = Channel;
	Out << "kast\tch\t" << K.Title << "\t" << K.Followers << "\t" << Fixed(K.FollowFrac, 4) << "\t" << Fixed(K.MinutesLive, 2) << "\t" << Fixed(K.ViewerMinutes, 1) << "\t" << K.Peak << "\t" << K.Streams << "\t"
		<< (K.Affiliate ? 1 : 0) << "\t" << (K.Partner ? 1 : 0) << "\t" << K.Milestone << "\t" << Fixed(K.LastOffline, 2) << "\t" << K.GiftedSubs << "\t" << K.RaidsIn << "\n";
	Out << "kast\tearn\t" << K.EarnedSubs << "\t" << K.EarnedBits << "\t" << K.EarnedTips << "\t" << K.EarnedAds << "\t" << K.EarnedSponsors << "\t" << K.UnpaidCents << "\t" << K.PaidCents << "\n";
	for (const kast::Subscriber& Sb : K.Subs)
	{
		Out << "kast\tsub\t" << session_detail::Escape(Sb.Name) << "\t" << Fixed(Sb.Since, 2) << "\t" << Fixed(Sb.Renews, 2) << "\t" << Sb.Months << "\t" << (Sb.Gift ? 1 : 0) << "\n";
	}
	for (const kast::Moderator& M : K.Mods)
	{
		Out << "kast\tmod\t" << session_detail::Escape(M.Name) << "\t" << Fixed(M.Since, 2) << "\t" << M.Actions << "\t" << Fixed(M.Online, 3) << "\n";
	}
	for (const kast::Clip& Cl : K.Clips)
	{
		Out << "kast\tclip\t" << Fixed(Cl.At, 2) << "\t" << Fixed(Cl.Views, 1) << "\t" << Fixed(Cl.Reach, 1) << "\t" << Cl.Kind << "\t" << session_detail::Escape(Cl.By) << "\t" << session_detail::Escape(Cl.Title) << "\n";
	}
	for (auto It = K.Log.rbegin(); It != K.Log.rend(); ++It)
	{
		Out << "kast\tlog\t" << Fixed(It->Start, 2) << "\t" << Fixed(It->Minutes, 2) << "\t" << It->Avg << "\t" << It->Peak << "\t" << It->Follows << "\t" << It->Subs << "\t" << It->Cents << "\t"
			<< session_detail::Escape(It->Title) << "\n";
	}
	for (const auto& Rg : K.Regulars)
	{
		Out << "kast\treg\t" << session_detail::Escape(Rg.first) << "\t" << Rg.second << "\n";
	}
	for (const kast::Deal& Dl : K.Deals)
	{
		Out << "kast\tdeal\t" << session_detail::Escape(Dl.Id) << "\t" << Fixed(Dl.Since, 2) << "\t" << Fixed(Dl.Until, 2) << "\t" << Fixed(Dl.ReadAt, 2) << "\t" << Dl.EarnedCents << "\n";
	}
	for (const std::string& O : K.Offers)
	{
		Out << "kast\toffer\t" << session_detail::Escape(O) << "\n";
	}
	for (const std::string& O : K.Declined)
	{
		Out << "kast\tdeclined\t" << session_detail::Escape(O) << "\n";
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
		else if (P.size() >= 3 && P[0] == "life")
		{
			life::State& L = D.Life;
			auto Num = [&](size_t I) { return I < P.size() ? std::atof(P[I].c_str()) : 0.0; };
			auto Int = [&](size_t I) { return I < P.size() ? std::atoi(P[I].c_str()) : 0; };
			auto Cents = [&](size_t I) { return I < P.size() ? static_cast<Chips>(std::strtoll(P[I].c_str(), nullptr, 10)) : static_cast<Chips>(0); };
			if (P[1] == "energy")
			{
				L.Energy = Num(2);
			}
			else if (P[1] == "heat")
			{
				L.Heat = Num(2);
			}
			else if (P[1] == "rent" && P.size() >= 6)
			{
				L.RentStage = static_cast<life::Rent>(Int(2));
				L.RentDueCents = Cents(3);
				L.RentDeadline = Num(4);
				L.RentsPaid = Int(5);
			}
			else if (P[1] == "debt")
			{
				L.DebtCents = Cents(2);
			}
			else if (P[1] == "ban")
			{
				L.BannedUntil = Num(2);
			}
			else if (P[1] == "count" && P.size() >= 7)
			{
				L.Shifts = Int(2);
				L.Runs = Int(3);
				L.Busts = Int(4);
				L.Ghosts = Int(5);
				L.Bans = Int(6);
			}
			else if (P[1] == "earned" && P.size() >= 4)
			{
				L.EarnedJobs = Cents(2);
				L.EarnedHustles = Cents(3);
			}
			else if (P[1] == "live" && P.size() >= 6)
			{
				L.LiveEvents = Int(2);
				L.LiveCashes = Int(3);
				L.LiveBestPlace = Int(4);
				L.LiveWonCents = Cents(5);
			}
			else if (P[1] == "backroom" && P.size() >= 4)
			{
				L.BackRoomNights = Int(2);
				L.BackRoomNetCents = Cents(3);
			}
		}
		else if (P.size() == 2 && P[0] == "unlock")
		{
			D.Life.Unlocks.insert(session_detail::Unescape(P[1]));
		}
		else if (P.size() == 3 && P[0] == "read")
		{
			D.Life.Reads[session_detail::Unescape(P[1])] = std::atoi(P[2].c_str());
		}
		else if (P.size() == 3 && P[0] == "ticket")
		{
			D.Life.Tickets[session_detail::Unescape(P[1])] = std::atoi(P[2].c_str());
		}
		else if (P.size() == 2 && P[0] == "night")
		{
			D.Life.NightsPaid.insert(std::atoi(P[1].c_str()));
		}
		else if (P.size() == 5 && P[0] == "ledger")
		{
			D.Life.Record(std::atof(P[1].c_str()), session_detail::Unescape(P[4]), static_cast<Chips>(std::strtoll(P[3].c_str(), nullptr, 10)), std::atoi(P[2].c_str()));
		}
		else if (P.size() == 2 && P[0] == "text")
		{
			D.TextsSeen.push_back(session_detail::Unescape(P[1]));
		}
		else if (P.size() == 3 && P[0] == "gear")
		{
			D.Gear[session_detail::Unescape(P[1])] = std::atof(P[2].c_str());
		}
		else if (P.size() >= 3 && P[0] == "kast")
		{
			kast::Channel& K = D.Channel;
			auto Num = [&](size_t I) { return I < P.size() ? std::atof(P[I].c_str()) : 0.0; };
			auto Int = [&](size_t I) { return I < P.size() ? std::atoi(P[I].c_str()) : 0; };
			auto Cents = [&](size_t I) { return I < P.size() ? static_cast<Chips>(std::strtoll(P[I].c_str(), nullptr, 10)) : static_cast<Chips>(0); };
			auto Str = [&](size_t I) { return I < P.size() ? session_detail::Unescape(P[I]) : std::string(); };
			const std::string& Key = P[1];
			if (Key == "ch" && P.size() >= 15)
			{
				K.Title = Int(2);
				K.Followers = Int(3);
				K.FollowFrac = Num(4);
				K.MinutesLive = Num(5);
				K.ViewerMinutes = Num(6);
				K.Peak = Int(7);
				K.Streams = Int(8);
				K.Affiliate = Int(9) != 0;
				K.Partner = Int(10) != 0;
				K.Milestone = Int(11);
				K.LastOffline = Num(12);
				K.GiftedSubs = Int(13);
				K.RaidsIn = Int(14);
			}
			else if (Key == "earn" && P.size() >= 9)
			{
				K.EarnedSubs = Cents(2);
				K.EarnedBits = Cents(3);
				K.EarnedTips = Cents(4);
				K.EarnedAds = Cents(5);
				K.EarnedSponsors = Cents(6);
				K.UnpaidCents = Cents(7);
				K.PaidCents = Cents(8);
			}
			else if (Key == "sub" && P.size() >= 7)
			{
				kast::Subscriber Sb;
				Sb.Name = Str(2);
				Sb.Since = Num(3);
				Sb.Renews = Num(4);
				Sb.Months = Int(5);
				Sb.Gift = Int(6) != 0;
				K.Subs.push_back(Sb);
			}
			else if (Key == "mod" && P.size() >= 6)
			{
				kast::Moderator M;
				M.Name = Str(2);
				M.Since = Num(3);
				M.Actions = Int(4);
				M.Online = Num(5);
				K.Mods.push_back(M);
			}
			else if (Key == "clip" && P.size() >= 8)
			{
				kast::Clip Cl;
				Cl.At = Num(2);
				Cl.Views = Num(3);
				Cl.Reach = Num(4);
				Cl.Kind = Int(5);
				Cl.By = Str(6);
				Cl.Title = Str(7);
				K.Clips.push_back(Cl);
			}
			else if (Key == "log" && P.size() >= 10)
			{
				kast::StreamLog Lg;
				Lg.Start = Num(2);
				Lg.Minutes = Num(3);
				Lg.Avg = Int(4);
				Lg.Peak = Int(5);
				Lg.Follows = Int(6);
				Lg.Subs = Int(7);
				Lg.Cents = Cents(8);
				Lg.Title = Str(9);
				K.Log.insert(K.Log.begin(), Lg);
			}
			else if (Key == "reg" && P.size() >= 4)
			{
				K.Regulars[Str(2)] = Int(3);
			}
			else if (Key == "deal" && P.size() >= 7)
			{
				kast::Deal Dl;
				Dl.Id = Str(2);
				Dl.Since = Num(3);
				Dl.Until = Num(4);
				Dl.ReadAt = Num(5);
				Dl.EarnedCents = Cents(6);
				K.Deals.push_back(Dl);
			}
			else if (Key == "offer")
			{
				K.Offers.insert(Str(2));
			}
			else if (Key == "declined")
			{
				K.Declined.insert(Str(2));
			}
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
	: Stream(Seed + ":kast"), Hooks(InHooks), SeedBase(Seed), R(Seed), LifeRng(Seed + ":life"), KastRng(Seed + ":kast-offline")
{
	if (Loaded)
	{
		BankrollCents = Loaded->BankrollCents;
		HeroName = Loaded->HeroName;
		History = Loaded->History;
		LobbyMinutes = Loaded->ClockMinutes;
		Life = Loaded->Life;
		Gear = Loaded->Gear;
		Channel = Loaded->Channel;
		TextsSeen.insert(Loaded->TextsSeen.begin(), Loaded->TextsSeen.end());
	}
	RefreshGear();
	// While the game loads: simulate the network's past results now, so the first leaderboard or page doesn't stall.
	net::Shared().Prewarm(WorldMinutes());
}

void Session::Save()
{
	SaveData D;
	D.BankrollCents = BankrollCents;
	D.HeroName = HeroName;
	D.History = History;
	D.ClockMinutes = LobbyMinutes;
	D.Life = Life;
	D.Gear = Gear;
	D.Channel = Channel;
	D.TextsSeen.assign(TextsSeen.begin(), TextsSeen.end());
	Hooks.Save(D);
}

void Session::ResetSave()
{
	BankrollCents = 237;
	History.clear();
	TextsSeen.clear();
	LobbyMinutes = 2.0 * 60.0 + 7.0;
	Life = life::State();
	CalendarAt = -1.0;
	if (Stream.Live)
	{
		Hooks.OnAir(false);
	}
	for (const auto& G : Gear)
	{
		Hooks.GearChanged(G.first, false);
	}
	Gear.clear();
	Channel = kast::Channel();
	Stream = kast::Stream(SeedBase + ":kast");
	StreamCard = false;
	RefreshGear();
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
	return Ev.Joinable && !Restricted() && !TimeSkip.Active && (BankrollCents >= Ev.BuyInCents || TicketsFor(Ev) > 0);
}

int Session::TicketsFor(const LobbyEvent& Ev) const
{
	const size_t At = Ev.Spec.Id.find('@');
	return At == std::string::npos ? 0 : Life.TicketsFor(Ev.Spec.Id.substr(0, At));
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
	if (!CanAfford(Listing) || IsPlaying(Listing.Spec.Id) || TableCount() >= MaxTables())
	{
		return;
	}
	// Another table: the one in front steps back and keeps playing; the new one opens in front.
	const double Clock = ClockMinutes();
	const ss::Pace PrevPace = CurrentPace == Pace::Sprint ? Pace::Smart : CurrentPace;
	const bool First = Runs.empty();
	if (!First)
	{
		SwapActive(*Runs[static_cast<size_t>(Active)]);
	}
	else
	{
		SessionStartBankroll = BankrollCents;
		SessionEvents = 0;
		Finished.clear();
	}
	Runs.push_back(std::make_unique<TableRun>());
	Active = static_cast<int>(Runs.size()) - 1;
	FocusHoldUntil = Now + 1.5;
	++SessionEvents;
	if (!First)
	{
		CurrentPace = PrevPace; // a new table plays like the one before it (the first keeps the session's pace, as always)
	}
	Joined = Listing;
	// The table opens now, or when the event starts if that's later (the wait passes at the desk).
	Joined.Spec.StartClock = std::max(Joined.Spec.StartClock, Clock);
	const LobbyEvent& Ev = Joined;
	if (TicketsFor(Ev) > 0)
	{
		const std::string Tid = Ev.Spec.Id.substr(0, Ev.Spec.Id.find('@'));
		if (--Life.Tickets[Tid] <= 0)
		{
			Life.Tickets.erase(Tid);
		}
		Life.Record(WorldMinutes(), "Ticket: " + Ev.Spec.Name, 0, 0);
	}
	else
	{
		BankrollCents -= Ev.BuyInCents;
		if (Ev.BuyInCents > 0)
		{
			Life.Record(WorldMinutes(), "Buy-in: " + Ev.Spec.Name, -Ev.BuyInCents, 0);
		}
	}
	Save();
	Event = &Joined;
	StreamMoment(kast::Moment::Register, Ev.Spec.Name);
	Bounties.clear();
	BountyWon = 0;
	Knockouts = 0;
	LastBountyAt = -100.0;
	SeatWon = false;
	Grades.clear();
	Badges.clear();
	Chat.clear();
	HandsPlayed = 0;
	BiggestPot = 0;
	if (First)
	{
		HeroTilt = 0.0; // a fresh sitting; another table doesn't calm you down
	}
	HasResults = false;
	HasBustInfo = false;
	RivalArrived = false;
	Sprinting = false;
	TimeBank = Life.Energy < 20.0 ? 15.0 : 30.0; // exhausted: slower to think
	TableId = 0;
	Moving = false;
	CurHand.reset();
	const std::string Seed = Ev.Spec.Id + ":" + SeedBase + ":" + std::to_string(RegisterCount++);
	LastTick = -1.0;
	T = std::make_unique<Tournament>(Ev.Spec, HeroName, Seed, std::vector<ReservedPlayer>{{RivalName, Archetype::Crusher}});
	CurrentScreen = Screen::Table;
	NameField();
	if (Ev.Spec.BountyCents > 0)
	{
		for (const TPlayer& P : T->Players)
		{
			Bounties[P.Id] = Ev.Spec.BountyCents;
		}
	}
	if (Ev.Spec.SeatValueCents > 0)
	{
		SystemLine("Welcome to " + Ev.Spec.Name + ". " + ChipsText(Ev.Spec.Entrants) + " players, " + std::to_string(SeatsInPlay()) + " seats.");
	}
	else
	{
		SystemLine("Welcome to " + Ev.Spec.Name + ". " + ChipsText(Ev.Spec.Entrants) + " players, " + std::to_string(T->PaidPlaces()) + " paid.");
	}
	if (Ev.Spec.BountyCents > 0)
	{
		SystemLine(Ev.Spec.MysteryBounty ? "Mystery bounties: knock players out in the money to open an envelope." : "Progressive knockout: " + Money(Ev.Spec.BountyCents) + " on every head. Half is paid, half goes on yours.");
	}
	const Level& L0 = T->CurrentLevel();
	SystemLine("Blinds " + std::to_string(L0.Sb) + "/" + std::to_string(L0.Bb) + ", ante " + std::to_string(L0.Ante) + ". Good luck!");
	Sound(SoundId::Alert, 1.0);
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
		const auto Country = FieldCountry.find(P.Id);
		V.Country = Country == FieldCountry.end() ? std::string() : Country->second;
		V.Regular = FieldRegulars.count(P.Id) > 0;
		V.Pro = FieldPros.count(P.Id) > 0 || (P.IsHero && TeamRiverLine());
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
			StreamMoment(kast::Moment::Rival);
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
		Sound(SoundId::Move, 1.0);
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
	double Calm = TableCount() > 1 ? std::pow(0.93, 1.0 / static_cast<double>(TableCount())) : 0.93; // per hand, wherever it's dealt
	if (Fx.Calm > 0.0)
	{
		Calm = std::pow(Calm, 1.0 + Fx.Calm); // the gym, the headphones
	}
	HeroTilt *= Calm;
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
	// The lobby clock runs in real time; tournaments run on their own clock; a shift or a night's sleep races by.
	const double DayOne = net::MinutesPerDay * static_cast<double>(net::NightOneDay);
	if (TimeSkip.Active)
	{
		const double K = Clamp01((InNow - TimeSkip.RealStart) / TimeSkip.RealSeconds);
		LobbyMinutes = TimeSkip.From + (TimeSkip.To - TimeSkip.From) * EaseInOut(K) - DayOne;
	}
	else if (LastTick >= 0.0 && !T && InNow > LastTick)
	{
		LobbyMinutes += std::min(InNow - LastTick, 1.0) / 60.0;
	}
	LastTick = InNow;
	Now = InNow;
	const double World = WorldMinutes();
	if (CalendarAt < 0.0)
	{
		CalendarAt = World;
	}
	if (World > CalendarAt)
	{
		CheckCalendar(CalendarAt, World, !TimeSkip.Active || TimeSkip.Result.ActivityId.empty());
		CalendarAt = World;
	}
	if (TimeSkip.Active && InNow - TimeSkip.RealStart >= TimeSkip.RealSeconds)
	{
		FinishSkip();
	}
	if (CurrentScreen != Screen::Boot && World >= DayOne + 129.0)
	{
		StoryText("marcus-intro", "Marcus", "heard you're behind on rent. I got work if you're not scared. check the burner app.");
	}
	// Every open table takes its turn; the one in front last-but-not-least, as it is. Tables behind share a small budget of
	// heavy steps (ending a hand, an equity run, a bot's postflop think) per frame, so they never pile into one long frame.
	BackgroundHeavy = 2;
	const int Front = Active;
	for (int K = 0; K < TableCount(); ++K)
	{
		if (K != Front)
		{
			Background = true;
			WithTable(K, [this]() { TableStep(); });
			Background = false;
		}
	}
	TableStep();
	if (T)
	{
		LobbyMinutes = std::max(LobbyMinutes, ClockMinutes());
	}
	CloseFinishedTables();
	AutoFocusStep();
	StreamStep();
}

void Session::TableStep()
{
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

	// Tables play on while the player browses the lobby; they stop for the results screen.
	if (!T || Closing || CurrentScreen == Screen::Results || CurrentScreen == Screen::Boot)
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
				Sound(SoundId::Alert, 1.0);
			}
			else if (Prompt.TimeBankUntil == 0.0 || Now > Prompt.TimeBankUntil)
			{
				HeroAct(Prompt.CanCheck ? PlayerAction::Check() : PlayerAction::Fold(), true);
			}
		}
		return;
	}

	// A table behind takes a heavy step only while the frame's budget lasts; otherwise it waits for the next frame.
	auto Heavy = [this]() {
		if (!Background)
		{
			return true;
		}
		if (BackgroundHeavy <= 0)
		{
			return false;
		}
		--BackgroundHeavy;
		return true;
	};
	// While a finished hand is shown, the rest of the field plays its hands a few tables a frame (EndHand wraps up).
	if (HandDone && T->FinishPending())
	{
		T->FinishSome(20);
	}
	int Guard = 0;
	while (Now >= NextAt && Guard++ < 50)
	{
		if (Cursor < H.Events.size())
		{
			const HandEvent& Next = H.Events[Cursor];
			const bool Equities = Next.Type == EventType::Reveal || (Next.Type == EventType::Street && Revealed);
			if (Equities && !Heavy())
			{
				return;
			}
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
				T->BeginFinish(&H);
				continue;
			}
			if (!Heavy())
			{
				return;
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
			if (!Heavy())
			{
				return;
			}
			OpenHeroTurn();
			return;
		}
		if (!BotPending)
		{
			if (H.CurrentStreet != Street::Preflop && !Heavy())
			{
				return;
			}
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
	// A cheap price (big-blind defence, completing, a short all-in) is a real decision: folding those blind gave away
	// playable spots about a quarter of the time, so they go to the player.
	const LegalActions Legal = H.GetLegalActions();
	const double Price = static_cast<double>(Legal.CallAmount) / static_cast<double>(std::max<Chips>(1, H.Pot() + Legal.CallAmount));
	if (Price < 0.25)
	{
		return false;
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
	Sound(SoundId::Turn, 1.0);
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
	if (Badge.G.Result == Grade::Blunder || (Badge.G.Result == Grade::Best && Prompt.Pot >= Prompt.BigBlind * 12))
	{
		StreamMoment(Badge.G.Result == Grade::Blunder ? kast::Moment::Blunder : kast::Moment::BestPlay);
	}
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
	FocusHoldUntil = Now + 0.35;
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
		Sound(SoundId::Chip, 0.5);
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
		Sound(SoundId::Deal, 1.0);
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
			Sound(SoundId::Fold, 0.6);
		}
		else if (A.Type == ActionType::Check)
		{
			Sound(SoundId::Check, 1.0);
		}
		else
		{
			AddFlight(true, FlightEnd::AtSeat(A.Seat), FlightEnd::AtBet(A.Seat), A.Added, Now, 0.3);
			Sound(A.AllIn ? SoundId::AllIn : SoundId::ChipStack, 1.0);
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
		Sound(SoundId::Flip, 1.0);
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
			Heartbeat(true);
			StreamMoment(kast::Moment::AllIn, std::string(), static_cast<double>(H.Pot()) / static_cast<double>(std::max<Chips>(1, H.BigBlind)) / 30.0);
		}
		Sound(SoundId::Flip, 1.0);
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
		Sound(SoundId::Flip, 1.0);
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
		Sound(HeroWon ? SoundId::Win : SoundId::ChipStack, HeroWon ? 1.0 : 0.6);
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
		Heartbeat(false);
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
		Sound(SoundId::ChipStack, 0.5);
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
		// On stream: the sweat pays off or it doesn't.
		const double PotBb = static_cast<double>(PotBeforeAward) / static_cast<double>(std::max<Chips>(1, H.BigBlind));
		if (HeroAllInReveal)
		{
			const kast::Moment M = !Lost ? kast::Moment::WonAllIn : HeroVis.HasEquity && HeroVis.Equity >= 0.6 ? kast::Moment::BadBeat : kast::Moment::LostAllIn;
			StreamMoment(M, ChipsText(static_cast<double>(PotBeforeAward)), PotBb / 30.0);
		}
		else if (!Lost && PotBb >= 30.0)
		{
			StreamMoment(kast::Moment::BigPot, ChipsText(static_cast<double>(PotBeforeAward)), PotBb / 40.0);
		}
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
	const std::vector<TEvent> Events = T->FinishPending() ? T->EndFinish() : T->FinishTick(&H);
	Heartbeat(false);
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
	if (CheckSatellite())
	{
		return;
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
		Sound(SoundId::Bust, 1.0);
		StreamMoment(Hero.PrizeCents > 0 ? kast::Moment::Cashed : kast::Moment::Bust, Ordinal(Hero.Place));
		if (Stream.Live)
		{
			Stream.Resolve(Channel, StreamInputs(), Hero.PrizeCents > 0);
		}
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
		StreamMoment(kast::Moment::Win, T->Spec.Name, 1.0 + static_cast<double>(Hero.PrizeCents) / 50000.0);
		if (Stream.Live)
		{
			Stream.Resolve(Channel, StreamInputs(), true);
		}
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
		if (!Background)
		{
			Hooks.AddCan();
		}
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
		case TEventType::Bust:
			HandleKnockout(E);
			break;
		case TEventType::Level:
			StreamMoment(kast::Moment::LevelUp);
			SystemLine("Blinds are now " + ChipsText(static_cast<double>(E.Blinds.Sb)) + "/" + ChipsText(static_cast<double>(E.Blinds.Bb)) + ", ante " + ChipsText(static_cast<double>(E.Blinds.Ante)) + ".");
			TimeBank = Min(60.0, TimeBank + 5.0);
			LastLevelUpAt = Now;
			Sound(SoundId::Level, 1.0);
			break;
		case TEventType::HandForHand:
			SystemLine("We are on the bubble. Hand-for-hand play.");
			CurrentBanner.Active = true;
			CurrentBanner.Title = "THE BUBBLE";
			CurrentBanner.Sub = std::to_string(T->Remaining - T->PaidPlaces()) + " away from the money";
			CurrentBanner.At = Now;
			CurrentBanner.Color = session_detail::Orange;
			Sound(SoundId::Bubble, 1.0);
			if (!T->Hero().Busted)
			{
				StreamMoment(kast::Moment::Bubble);
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
				Sound(SoundId::Cash, 1.0);
				StoryText("dee-itm", "Dee", "In the money. Now play to win, not to min-cash.");
				StreamMoment(kast::Moment::InTheMoney);
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
				Sound(SoundId::Bubble, 1.0);
				StoryText("dee-ft", "Dee", "A final table? At this hour? Call me when it is over. Win or lose.");
				StreamMoment(kast::Moment::FinalTable, T->Spec.Name, 1.0 + static_cast<double>(T->PrizeFor(1)) / 100000.0);
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
	const Profile Autopilot = session_detail::Autopilot();
	const auto T0 = std::chrono::steady_clock::now();
	auto ElapsedMs = [&]() { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count(); };
	do
	{
		const std::vector<TEvent> Events = T->SimulateTick(&Autopilot);
		++HandsPlayed;
		HandleTourneyEvents(Events);
		if (CheckSatellite())
		{
			return;
		}
		const TPlayer& Hero = T->Hero();
		if (Hero.Busted)
		{
			Sprinting = false;
			HasBustInfo = true;
			BustPlace = Hero.Place;
			BustPrize = Hero.PrizeCents;
			BustAt = Now + 2.5;
			Sound(SoundId::Bust, 1.0);
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
	} while (ElapsedMs() < SprintBudgetMs / static_cast<double>(std::max(1, TableCount())));
}

// ------------------------------------------------------------------ results

void Session::ShowResults()
{
	if (T->bFinished && BustPlace == 1 && T->Spec.BountyCents > 0 && !T->Spec.MysteryBounty)
	{
		BountyWon += Bounties[HeroId]; // the winner keeps their own head
	}
	BankrollCents += BustPrize + BountyWon;
	const double Acc = Grades.empty() ? 0.0 : Accuracy(Grades);
	// Another table still playing (one closing this same frame doesn't count): this one closes with a toast.
	bool OthersPlaying = false;
	for (int K = 0; K < TableCount(); ++K)
	{
		OthersPlaying = OthersPlaying || (K != Active && !Runs[static_cast<size_t>(K)]->Closing);
	}
	ss::Results Saved = LastResults;
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
	LastResults.Won = BustPlace == 1 && !SeatWon;
	LastResults.BountyCents = BountyWon;
	LastResults.Knockouts = Knockouts;
	if (SeatWon)
	{
		LastResults.SeatWon = T->Spec.SeatTicket;
		LastResults.SeatValueCents = T->Spec.SeatValueCents;
		++Life.Tickets[T->Spec.SeatTicket];
		Life.Record(WorldMinutes(), "Seat won: " + T->Spec.Name, 0, 4);
		StoryText("seat-" + T->Spec.SeatTicket, "Dee", T->Spec.SeatTicket == "rcop-main" ? "You WON A SEAT TO THE MAIN EVENT?? $5,250 for nothing. Twenty-five million guaranteed. Kid." : "A ticket up the ladder. Keep climbing.");
	}
	if (BustPrize + BountyWon > 0)
	{
		Life.Record(WorldMinutes(), (BountyWon > 0 && BustPrize == 0 ? "Bounties: " : "Prize: ") + T->Spec.Name, BustPrize + BountyWon, 0);
	}
	LastResults.SessionEvents = SessionEvents;
	LastResults.SessionNetCents = BankrollCents - SessionStartBankroll;
	if (OthersPlaying)
	{
		// The other tables keep going: this one closes with a toast, and the results screen waits for the last.
		Finished.push_back({LastResults, Now});
		LastResults = Saved;
		Closing = true;
	}
	else
	{
		HasResults = true;
	}
	LobbyMinutes = std::max(LobbyMinutes, T->ClockMinutes());
	HistoryEntry Entry;
	Entry.Name = T->Spec.Name;
	Entry.Place = BustPlace;
	Entry.Entrants = T->Spec.Entrants;
	Entry.Prize = BustPrize + BountyWon + (SeatWon ? T->Spec.SeatValueCents : 0);
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
	if (!OthersPlaying)
	{
		CurrentScreen = Screen::Results;
		ResultsAt = Now;
	}
	Heartbeat(false);
	Save();
	if (BankrollCents < 25 && BustPrize == 0)
	{
		StoryText("broke", "Dee", "Broke? It happens to everybody. Freeroll's always running. Or come by Tuesday. Bring quarters for the dryers.", false);
	}
	else if (BustPrize > 0)
	{
		StoryText("first-cash", "Landlord", "Saw your light on all night. Rent + late fee is $1,225. Friday.");
	}
	CheckUnlocks();
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
	Runs.clear();
	Active = -1;
	Closing = false;
}

double Session::ClockMinutes() const
{
	if (!T)
	{
		return LobbyMinutes;
	}
	// Each tournament keeps its own clock; the room's is the one furthest along, and never earlier than it has been
	// (Update keeps LobbyMinutes up with it, so a table closing doesn't wind the clock back).
	double Clock = std::max(LobbyMinutes, T->ClockMinutes());
	for (const std::unique_ptr<TableRun>& Run : Runs)
	{
		Clock = Run->T ? std::max(Clock, Run->T->ClockMinutes()) : Clock;
	}
	return Clock;
}

double Session::WorldMinutes() const
{
	return net::MinutesPerDay * static_cast<double>(net::NightOneDay) + ClockMinutes();
}
// ------------------------------------------------------------------ multi-tabling

void Session::SwapActive(TableRun& Run)
{
	// Event only ever points at Joined (or is null), so swapping the pointers keeps it right for whichever table is in front.
	using std::swap;
	swap(CurrentPace, Run.CurrentPace);
	swap(CurrentBanner, Run.CurrentBanner);
	swap(Event, Run.Event);
	swap(Joined, Run.Joined);
	swap(T, Run.T);
	swap(CurHand, Run.CurHand);
	swap(TableId, Run.TableId);
	swap(Seats, Run.Seats);
	swap(ButtonSeat, Run.ButtonSeat);
	swap(Board, Run.Board);
	swap(BoardShownAt, Run.BoardShownAt);
	swap(PotChips, Run.PotChips);
	swap(Flights, Run.Flights);
	swap(Chat, Run.Chat);
	swap(HasPrompt, Run.HasPrompt);
	swap(Prompt, Run.Prompt);
	swap(Badges, Run.Badges);
	swap(Grades, Run.Grades);
	swap(SitOutNext, Run.SitOutNext);
	swap(SittingOutThisHand, Run.SittingOutThisHand);
	swap(Sprinting, Run.Sprinting);
	swap(SprintStopReason, Run.SprintStopReason);
	swap(HandsPlayed, Run.HandsPlayed);
	swap(BiggestPot, Run.BiggestPot);
	swap(TimeBank, Run.TimeBank);
	swap(Moving, Run.Moving);
	swap(MovingFrom, Run.MovingFrom);
	swap(MovingTo, Run.MovingTo);
	swap(MovingAt, Run.MovingAt);
	swap(AutoFolded, Run.AutoFolded);
	swap(AutoFoldedCards, Run.AutoFoldedCards);
	swap(AutoFoldedAt, Run.AutoFoldedAt);
	swap(LastLevelUpAt, Run.LastLevelUpAt);
	swap(HeroAllInReveal, Run.HeroAllInReveal);
	swap(FieldCountry, Run.FieldCountry);
	swap(FieldRegulars, Run.FieldRegulars);
	swap(FieldPros, Run.FieldPros);
	swap(Bounties, Run.Bounties);
	swap(BountyWon, Run.BountyWon);
	swap(Knockouts, Run.Knockouts);
	swap(LastBountyAt, Run.LastBountyAt);
	swap(LastBountyCents, Run.LastBountyCents);
	swap(LastBountyName, Run.LastBountyName);
	swap(SeatWon, Run.SeatWon);
	swap(Cursor, Run.Cursor);
	swap(NextAt, Run.NextAt);
	swap(BotPending, Run.BotPending);
	swap(BotAction, Run.BotAction);
	swap(BotAt, Run.BotAt);
	swap(BotSeatIdx, Run.BotSeatIdx);
	swap(HandDone, Run.HandDone);
	swap(Revealed, Run.Revealed);
	swap(HeroFolded, Run.HeroFolded);
	swap(PotBeforeAward, Run.PotBeforeAward);
	swap(BustAt, Run.BustAt);
	swap(HasBustInfo, Run.HasBustInfo);
	swap(BustPlace, Run.BustPlace);
	swap(BustPrize, Run.BustPrize);
	swap(LastIdleChat, Run.LastIdleChat);
	swap(RivalArrived, Run.RivalArrived);
	swap(CansShown, Run.CansShown);
	swap(SprintBubble, Run.SprintBubble);
	swap(SprintFinal, Run.SprintFinal);
	swap(Closing, Run.Closing);
}

int Session::MaxTables() const
{
	// Screens decide how many tables fit (the laptop alone fits two); being exhausted caps it at two anyway.
	return std::min(Life.Energy < 20.0 ? 2 : TableLimit, std::max(1, Fx.Tables));
}

void Session::FocusTable(int Index)
{
	if (Index < 0 || Index >= TableCount() || Index == Active)
	{
		return;
	}
	SwapActive(*Runs[static_cast<size_t>(Active)]);
	Active = Index;
	SwapActive(*Runs[static_cast<size_t>(Active)]);
	FocusHoldUntil = Now + 1.5;
	Heartbeat(HeroAllInReveal && CurHand && !CurHand->bComplete);
}

void Session::WithTable(int Index, const std::function<void()>& Fn)
{
	if (Index < 0 || Index >= TableCount())
	{
		return;
	}
	if (Index == Active)
	{
		Fn();
		return;
	}
	const int Front = Active;
	SwapActive(*Runs[static_cast<size_t>(Front)]);
	SwapActive(*Runs[static_cast<size_t>(Index)]);
	Active = Index;
	Fn();
	SwapActive(*Runs[static_cast<size_t>(Index)]);
	SwapActive(*Runs[static_cast<size_t>(Front)]);
	Active = Front;
}

template <typename Src>
TableGlance Session::GlanceOf(const Src& From)
{
	TableGlance G;
	G.EventId = From.Joined.Spec.Id;
	G.Name = From.T ? From.T->Spec.Name : From.Joined.Spec.Name;
	G.YourTurn = From.HasPrompt;
	G.TurnOpenedAt = From.Prompt.OpenedAt;
	const bool Bank = From.Prompt.TimeBankUntil > 0.0;
	G.Deadline = Bank ? From.Prompt.TimeBankUntil : From.Prompt.Deadline;
	G.DeadlineStart = Bank ? From.Prompt.Deadline : From.Prompt.OpenedAt;
	G.Sprinting = From.Sprinting;
	if (!From.T)
	{
		return G;
	}
	const Tournament& Tn = *From.T;
	G.Stack = Tn.Hero().Stack;
	for (const SeatVis& Sv : From.Seats)
	{
		if (Sv.Present && Sv.IsHero)
		{
			G.Stack = Sv.Stack + Sv.Bet;
			G.AllIn = Sv.AllIn;
		}
	}
	const double Bb = static_cast<double>(std::max<Chips>(1, Tn.CurrentLevel().Bb));
	G.StackBb = static_cast<double>(G.Stack) / Bb;
	G.Rank = Tn.Hero().Busted ? Tn.Hero().Place : Tn.HeroRank();
	G.Remaining = Tn.Remaining;
	G.InMoney = Tn.InTheMoney();
	G.Busted = From.HasBustInfo || Tn.Hero().Busted;
	return G;
}

TableGlance Session::Glance(int Index) const
{
	if (Index < 0 || Index >= TableCount())
	{
		return TableGlance();
	}
	return Index == Active ? GlanceOf(*this) : GlanceOf(*Runs[static_cast<size_t>(Index)]);
}

void Session::HeroActAt(int Index, const PlayerAction& Action)
{
	FocusTable(Index);
	if (Index == Active)
	{
		HeroAct(Action);
	}
}

bool Session::IsPlaying(const std::string& EventId) const
{
	for (int K = 0; K < TableCount(); ++K)
	{
		const LobbyEvent& Ev = K == Active ? Joined : Runs[static_cast<size_t>(K)]->Joined;
		if (Ev.Spec.Id == EventId)
		{
			return true;
		}
	}
	return false;
}

int Session::TablesWaiting() const
{
	int Count = 0;
	for (int K = 0; K < TableCount(); ++K)
	{
		Count += (K == Active ? HasPrompt : Runs[static_cast<size_t>(K)]->HasPrompt) ? 1 : 0;
	}
	return Count;
}

void Session::ShowLobby()
{
	if (CurrentScreen == Screen::Table)
	{
		CurrentScreen = Screen::Lobby;
		ConfirmRegister = false;
	}
}

void Session::ShowTables()
{
	if (T && CurrentScreen == Screen::Lobby)
	{
		CurrentScreen = Screen::Table;
		FocusHoldUntil = Now + 0.8;
	}
}

void Session::CloseFinishedTables()
{
	for (int K = TableCount(); K-- > 0;)
	{
		const bool Done = K == Active ? Closing : Runs[static_cast<size_t>(K)]->Closing;
		if (!Done)
		{
			continue;
		}
		if (K == Active)
		{
			SwapActive(*Runs[static_cast<size_t>(K)]); // the finished table goes into its run, to be dropped
			Active = -1;
		}
		else if (K < Active)
		{
			--Active;
		}
		Runs.erase(Runs.begin() + K);
	}
	if (Active < 0 && !Runs.empty())
	{
		// The next table forward: the one that has waited longest for the player, else the first.
		int Pick = 0;
		double Oldest = 1e300;
		for (int K = 0; K < TableCount(); ++K)
		{
			const TableRun& Run = *Runs[static_cast<size_t>(K)];
			if (Run.HasPrompt && Run.Prompt.OpenedAt < Oldest)
			{
				Oldest = Run.Prompt.OpenedAt;
				Pick = K;
			}
		}
		Active = Pick;
		SwapActive(*Runs[static_cast<size_t>(Active)]);
		FocusHoldUntil = Now + 0.8;
	}
	if (Runs.empty() && Active < 0 && CurrentScreen == Screen::Table)
	{
		CurrentScreen = Screen::Lobby; // never strands the player on an empty table screen
	}
}

void Session::AutoFocusStep()
{
	if (!AutoFocus || TableCount() < 2 || CurrentScreen != Screen::Table || Now < FocusHoldUntil || HasPrompt || HasBustInfo)
	{
		return;
	}
	// The player's own all-in runs out in front unless another clock is nearly gone.
	const bool Sweating = HeroAllInReveal && CurHand && !CurHand->bComplete;
	int Pick = -1;
	double Oldest = 1e300;
	for (int K = 0; K < TableCount(); ++K)
	{
		const TableRun& Run = *Runs[static_cast<size_t>(K)];
		if (K == Active || !Run.HasPrompt)
		{
			continue;
		}
		if (Sweating && Run.Prompt.Deadline - Now > 8.0)
		{
			continue;
		}
		if (Run.Prompt.OpenedAt < Oldest)
		{
			Oldest = Run.Prompt.OpenedAt;
			Pick = K;
		}
	}
	if (Pick >= 0)
	{
		FocusTable(Pick);
		FocusHoldUntil = Now + 0.6;
	}
}

void Session::Sound(SoundId Id, double Volume)
{
	if (!Background)
	{
		Hooks.Sound(Id, Volume);
		return;
	}
	// A table in the background: its turn chime carries, the big moments are soft, chips and cards stay quiet.
	switch (Id)
	{
	case SoundId::Turn: Hooks.Sound(Id, Volume); break;
	case SoundId::Alert:
	case SoundId::Win:
	case SoundId::Bust:
	case SoundId::Cash:
	case SoundId::Bubble:
	case SoundId::Level:
	case SoundId::Move:
	case SoundId::AllIn: Hooks.Sound(Id, Volume * 0.35); break;
	default: break;
	}
}

void Session::Heartbeat(bool On)
{
	if (!Background)
	{
		Hooks.Heartbeat(On);
	}
}

// ------------------------------------------------------------------ life

life::Context Session::LifeContext() const
{
	life::Context Ctx;
	Ctx.World = WorldMinutes();
	Ctx.InTournament = T != nullptr;
	Ctx.Bankroll = BankrollCents;
	for (const HistoryEntry& H : History)
	{
		Ctx.Cashes += H.Prize > 0 ? 1 : 0;
	}
	return Ctx;
}

int Session::Unlocks() const
{
	return (Life.Unlocks.count("bounty") ? net::UnlockBounty : 0) | (Life.Unlocks.count("satellite") ? net::UnlockSatellite : 0) | (Life.Unlocks.count("sixmax") ? net::UnlockSixMax : 0);
}

double Session::Daylight() const
{
	return life::Daylight(WorldMinutes());
}

int Session::SeatsInPlay() const
{
	if (!T || T->Spec.SeatValueCents <= 0)
	{
		return 0;
	}
	int Count = 0;
	for (const Chips P : T->Payouts)
	{
		Count += P == T->Spec.SeatValueCents ? 1 : 0;
	}
	return Count;
}

std::string Session::GoToGame(const std::string& Id, Chips BuyInCents)
{
	const life::Activity* A = life::Find(Id);
	if (!A || (A->Type != life::Kind::Game && A->Type != life::Kind::Live))
	{
		return "Unknown game.";
	}
	if (A->Type == life::Kind::Live)
	{
		BuyInCents = live::RiversideBuyInCents;
	}
	if (TimeSkip.Active)
	{
		return "You're busy.";
	}
	const std::string Why = life::Blocked(*A, Life, LifeContext());
	if (!Why.empty())
	{
		return Why;
	}
	if (BuyInCents > BankrollCents || (A->Type == life::Kind::Game && (BuyInCents < life::GameMinBuyInCents || BuyInCents > life::GameMaxBuyInCents)))
	{
		return "Pick a buy-in you can cover.";
	}
	// The host takes the player to the table; it settles up in the save when they come home.
	if (Stream.Live)
	{
		EndStream();
	}
	Save();
	Sound(SoundId::Click, 0.8);
	return Hooks.GoOut(Id, BuyInCents) ? "" : "Can't get there right now.";
}

std::string Session::StartActivity(const std::string& Id)
{
	const life::Activity* A = life::Find(Id);
	if (!A)
	{
		return "Unknown activity.";
	}
	if (A->Type == life::Kind::Game || A->Type == life::Kind::Live)
	{
		return "Pick a buy-in first.";
	}
	if (TimeSkip.Active)
	{
		return "You're busy.";
	}
	const std::string Why = life::Blocked(*A, Life, LifeContext());
	if (!Why.empty())
	{
		return Why;
	}
	if (Stream.Live)
	{
		EndStream(); // nobody streams a shift, a run or a night's sleep
	}
	const double World = WorldMinutes();
	TimeSkip = Skip();
	TimeSkip.Active = true;
	TimeSkip.From = World;
	TimeSkip.Result = life::Resolve(*A, Life, World, LifeRng, BankrollCents);
	TimeSkip.To = TimeSkip.Result.End;
	TimeSkip.RealStart = Now;
	TimeSkip.RealSeconds = std::min(5.0, 2.0 + (TimeSkip.To - TimeSkip.From) / 60.0 * 0.22);
	switch (A->Type)
	{
	case life::Kind::Job: TimeSkip.Label = "Working a shift at " + A->Place; break;
	case life::Kind::Hustle: TimeSkip.Label = A->Id == "marcus-run" ? "On the long run for Marcus" : "On a drop-off for Marcus"; break;
	case life::Kind::Ghost: TimeSkip.Label = "Playing as whale_sam"; break;
	case life::Kind::Sleep: TimeSkip.Label = A->Hours >= 8.0 ? "Sleeping" : "Napping"; break;
	case life::Kind::Game:
	case life::Kind::Live: break;
	}
	HasOutcome = false;
	ConfirmRegister = false;
	Sound(SoundId::Click, 0.8);
	return "";
}

void Session::FinishSkip()
{
	const life::Outcome O = TimeSkip.Result;
	TimeSkip.Active = false;
	LobbyMinutes = O.End - net::MinutesPerDay * static_cast<double>(net::NightOneDay);
	CalendarAt = std::max(CalendarAt, O.End);
	const life::Activity* A = life::Find(O.ActivityId);
	const Chips Change = std::max(O.Money, -BankrollCents);
	BankrollCents += Change;
	const double EnergyGain = O.Energy > 0.0 ? O.Energy * (1.0 + Fx.Rest) : O.Energy; // a better bed, darker curtains
	Life.Energy = std::min(100.0, std::max(0.0, Life.Energy + EnergyGain));
	Life.Heat = std::min(100.0, std::max(0.0, Life.Heat + O.Heat));
	Life.DebtCents += O.Debt;
	Life.BannedUntil = std::max(Life.BannedUntil, O.BanUntil);
	if (A)
	{
		switch (A->Type)
		{
		case life::Kind::Job:
			++Life.Shifts;
			Life.EarnedJobs += Change;
			Life.Record(O.End, A->Place + " shift", Change, 1);
			StoryText("first-shift", "Dee", "Saw you through the window at " + A->Place + ". Honest money. Don't let it eat your nights.");
			break;
		case life::Kind::Hustle:
			if (O.Bad)
			{
				++Life.Busts;
				Life.Record(O.End, "Fine (picked up)", Change, 2);
				StoryText("first-bust", "Dee", "Heard you got picked up last night. What are you doing? Call me.");
				Hooks.Text("Marcus", "you lost my bag. that's " + Money(O.Debt) + ". pay up before you get more work.");
			}
			else
			{
				++Life.Runs;
				Life.EarnedHustles += Change;
				Life.Record(O.End, "Marcus: " + A->Title, Change, 2);
				if (Life.Runs == 2)
				{
					Hooks.Text("Marcus", "you're reliable. got a bigger one when you're ready. long drive, real money.");
				}
			}
			break;
		case life::Kind::Ghost:
			++Life.Ghosts;
			if (O.Bad)
			{
				++Life.Bans;
				Hooks.Text("RiverLine", "Security notice: your account is restricted for 24 hours while we review activity linked to another account.");
			}
			else
			{
				Life.EarnedHustles += Change;
				Life.Record(O.End, "Sam: account session", Change, 2);
			}
			break;
		case life::Kind::Sleep:
		case life::Kind::Game:
		case life::Kind::Live:
			break;
		}
	}
	LastOutcome = O;
	LastOutcome.Money = Change;
	LastOutcome.Energy = EnergyGain;
	HasOutcome = true;
	Sound(O.Bad ? SoundId::Bust : Change > 0 ? SoundId::Cash : SoundId::Click, 1.0);
	Save();
}

// ------------------------------------------------------------------ GearDrop

void Session::RefreshGear()
{
	Fx = gear::Sum(Gear);
}

std::string Session::CanBuy(const std::string& Id) const
{
	const gear::Item* I = gear::Find(Id);
	if (!I)
	{
		return "Unknown item.";
	}
	if (Owns(Id))
	{
		return I->Monthly ? "Subscribed." : "Owned.";
	}
	if (!I->Requires.empty() && !Owns(I->Requires))
	{
		const gear::Item* Need = gear::Find(I->Requires);
		return "Needs the " + (Need ? Need->Name : I->Requires) + " first.";
	}
	// Something better already fills the slot (an upgrade path, not a side-grade).
	if (I->Where != gear::Slot::None)
	{
		for (const auto& G : Gear)
		{
			const gear::Item* Have = gear::Find(G.first);
			if (Have && Have->Where == I->Where && Have->PriceCents > I->PriceCents)
			{
				return "You have better.";
			}
		}
	}
	if (BankrollCents < I->PriceCents)
	{
		return "Not enough in the bank.";
	}
	return "";
}

std::string Session::Buy(const std::string& Id)
{
	const std::string Why = CanBuy(Id);
	if (!Why.empty())
	{
		return Why;
	}
	const gear::Item& I = *gear::Find(Id);
	const double World = WorldMinutes();
	BankrollCents -= I.PriceCents;
	Life.Record(World, (I.Monthly ? "GearDrop: " + I.Name + " (month)" : "GearDrop: " + I.Name), -I.PriceCents, 5);
	Gear[Id] = I.Monthly ? World + 30.0 * net::MinutesPerDay : 0.0;
	RefreshGear();
	Hooks.GearChanged(Id, true);
	Sound(SoundId::Cash, 0.7);
	if (I.Where == gear::Slot::Pc && Fx.CanStream())
	{
		StoryText("gear-can-stream", "Kast", "Your setup can stream now. Open Kast on the taskbar and go live: chat, followers, subs and sponsors are waiting.");
	}
	if (I.Cat == gear::Category::Stream && !Channel.Streams)
	{
		StoryText("gear-first-stream", "Dee", "Saw the box at your door. A camera? You're going to stream it? Half the laundromat watches those poker streams. Don't punt on camera.");
	}
	if (I.Tables > 0)
	{
		StoryText("gear-monitor", "Dee", "More screens, more tables. More tables, more ways to go broke at once. Be smart.");
	}
	Save();
	return "";
}

bool Session::Cancel(const std::string& Id)
{
	const gear::Item* I = gear::Find(Id);
	if (!I || !I->Monthly || !Owns(Id))
	{
		return false;
	}
	Gear.erase(Id);
	RefreshGear();
	Hooks.GearChanged(Id, false);
	Save();
	return true;
}

void Session::RenewGear(double From, double To)
{
	(void)From;
	bool Changed = false;
	for (auto It = Gear.begin(); It != Gear.end();)
	{
		const gear::Item* I = gear::Find(It->first);
		bool Keep = true;
		while (I && I->Monthly && It->second > 0.0 && It->second <= To)
		{
			if (BankrollCents < I->PriceCents)
			{
				Hooks.Text(I->Brand, "We couldn't take this month's payment for " + I->Name + ", so your subscription has ended. Resubscribe on GearDrop any time.");
				Hooks.GearChanged(It->first, false);
				Keep = false;
				break;
			}
			BankrollCents -= I->PriceCents;
			Life.Record(It->second, "GearDrop: " + I->Name + " (renewal)", -I->PriceCents, 5);
			It->second += 30.0 * net::MinutesPerDay;
		}
		if (Keep)
		{
			++It;
		}
		else
		{
			It = Gear.erase(It);
			Changed = true;
		}
	}
	if (Changed)
	{
		RefreshGear();
	}
}

// ------------------------------------------------------------------ Kast

kast::Inputs Session::StreamInputs() const
{
	kast::Inputs In;
	In.World = WorldMinutes();
	In.Real = Now;
	In.AtTable = T != nullptr && CurrentScreen != Screen::Results;
	In.Results = CurrentScreen == Screen::Results;
	In.Tables = TableCount();
	In.Sprinting = Sprinting;
	In.Hero = HeroName;
	In.EventName = Event ? Event->Spec.Name : std::string();
	if (T && Active >= 0)
	{
		const TableGlance G = Glance(Active);
		In.Remaining = G.Remaining;
		In.Rank = G.Rank;
		In.InMoney = G.InMoney;
		In.StackBb = G.StackBb;
		In.Entrants = T->Spec.Entrants;
	}
	In.Tilt = HeroTilt;
	In.Bankroll = BankrollCents;
	In.RentDue = Life.RentStage == life::Rent::Paid ? 0 : Life.RentDueCents;
	In.Gear = Fx;
	return In;
}

std::string Session::GoLive()
{
	if (Stream.Live)
	{
		return "Already live.";
	}
	if (TimeSkip.Active)
	{
		return "You're busy.";
	}
	if (Life.RentStage == life::Rent::Evicted)
	{
		return "No apartment, no internet.";
	}
	if (!Fx.CanStream())
	{
		return "The laptop can't stream. It needs a PC upgrade (" + gear::FirstPcUpgrade().Name + " on GearDrop).";
	}
	StreamCard = false;
	StreamWorldAt = WorldMinutes();
	StreamRealAt = Now;
	Stream.Start(Channel, StreamInputs());
	Hooks.OnAir(true);
	Sound(SoundId::Alert, 0.7);
	PlayStreamNotices();
	if (Channel.Streams == 1)
	{
		StoryText("kast-first", "Dee", "You're LIVE? Mei just sent me the link. Say hi to the laundromat.");
	}
	Save();
	return "";
}

void Session::PayOut(const std::string& Label)
{
	const Chips Pay = Channel.UnpaidCents;
	if (Pay <= 0)
	{
		return;
	}
	BankrollCents += Pay;
	Channel.PaidCents += Pay;
	Channel.UnpaidCents = 0;
	Life.Record(WorldMinutes(), Label, Pay, 6);
}

void Session::EndStream()
{
	if (!Stream.Live)
	{
		return;
	}
	Stream.Stop(Channel, StreamInputs());
	PlayStreamNotices();
	PayOut("Kast payout");
	StreamCard = true;
	Hooks.OnAir(false);
	Sound(Stream.Last.Total() > 0 ? SoundId::Cash : SoundId::Click, 0.8);
	Save();
}

Chips Session::CashOut()
{
	const Chips Pay = Channel.UnpaidCents;
	PayOut("Kast payout");
	if (Pay > 0)
	{
		Sound(SoundId::Cash, 0.8);
		Save();
	}
	return Pay;
}

void Session::StreamMoment(kast::Moment M, const std::string& Detail, double Size)
{
	if (!Stream.Live)
	{
		return;
	}
	Stream.OnMoment(Channel, StreamInputs(), M, Detail, Size);
}

void Session::PlayStreamNotices()
{
	for (const kast::Notice& N : Stream.Notices)
	{
		if (N.Type == kast::Notice::Kind::Text)
		{
			Hooks.Text(N.From, N.Body);
			continue;
		}
		// One alert sound at a time; a burst of follows doesn't machine-gun the speakers.
		if (Now - StreamSoundAt < 1.2)
		{
			continue;
		}
		StreamSoundAt = Now;
		switch (N.Type)
		{
		case kast::Notice::Kind::Chime: Hooks.Sound(SoundId::Alert, 0.35); break;
		case kast::Notice::Kind::Sub:
		case kast::Notice::Kind::Tip: Hooks.Sound(SoundId::Cash, 0.55); break;
		case kast::Notice::Kind::Raid:
		case kast::Notice::Kind::Milestone: Hooks.Sound(SoundId::Win, 0.7); break;
		default: break;
		}
	}
	Stream.Notices.clear();
}

void Session::StreamStep()
{
	const double World = WorldMinutes();
	if (!Stream.Live)
	{
		// Offline, the channel still turns over: renewals, clip views, deals running out (once a game minute is plenty).
		if (OfflineAt < 0.0 || World - OfflineAt >= 1.0)
		{
			OfflineAt = World;
			kast::Offline(Channel, World, KastRng);
		}
		return;
	}
	const double DWorld = StreamWorldAt < 0.0 ? 0.0 : World - StreamWorldAt;
	const double DReal = StreamRealAt < 0.0 ? 0.0 : Now - StreamRealAt;
	StreamWorldAt = World;
	StreamRealAt = Now;
	const int MissedBefore = Stream.Missed;
	Stream.Tick(Channel, StreamInputs(), DWorld, DReal);
	if (Stream.Missed > MissedBefore)
	{
		// A troll gets through after a bad beat: it gets under the skin.
		HeroTilt = std::min(1.0, HeroTilt + 0.03 * static_cast<double>(Stream.Missed - MissedBefore) * (1.0 - Fx.Calm));
	}
	PlayStreamNotices();
}

bool Session::StreamAd(int Seconds)
{
	const bool Ok = Stream.RunAd(Channel, StreamInputs(), Seconds);
	Sound(Ok ? SoundId::Click : SoundId::Fold, 0.6);
	return Ok;
}

bool Session::StreamThank()
{
	return Stream.Thank(Channel, StreamInputs());
}

bool Session::StreamAnswer(int MsgId)
{
	return Stream.Answer(Channel, StreamInputs(), MsgId);
}

bool Session::StreamTimeout(int MsgId)
{
	return Stream.Timeout(Channel, StreamInputs(), MsgId);
}

bool Session::StreamPromote(const std::string& Name)
{
	const bool Ok = Stream.Promote(Channel, StreamInputs(), Name);
	if (Ok)
	{
		Save();
	}
	return Ok;
}

bool Session::StreamDemote(const std::string& Name)
{
	const bool Ok = Stream.Demote(Channel, Name);
	if (Ok)
	{
		Save();
	}
	return Ok;
}

Chips Session::StreamRead(const std::string& SponsorId)
{
	const Chips Paid = Stream.SponsorRead(Channel, StreamInputs(), SponsorId);
	if (Paid > 0)
	{
		Sound(SoundId::Cash, 0.5);
	}
	return Paid;
}

void Session::StreamTitle(int Title)
{
	const int Count = static_cast<int>(kast::Titles().size());
	Channel.Title = ((Title % Count) + Count) % Count;
}

std::string Session::AcceptDeal(const std::string& Id)
{
	const kast::Sponsor* S = kast::FindSponsor(Id);
	if (!S || !Channel.Offers.count(Id))
	{
		return "No offer.";
	}
	const double World = WorldMinutes();
	int Active2 = 0;
	for (const kast::Deal& D : Channel.Deals)
	{
		Active2 += D.Until > World ? 1 : 0;
	}
	if (Active2 >= kast::MaxDeals)
	{
		return "Both sponsor slots are taken.";
	}
	kast::Deal D;
	D.Id = Id;
	D.Since = World;
	D.Until = World + static_cast<double>(S->Days) * net::MinutesPerDay;
	Channel.Deals.push_back(D);
	Channel.Offers.erase(Id);
	Hooks.Text(S->Brand, "Welcome aboard! The deal runs " + std::to_string(S->Days) + " days. We pay " + Money(S->PerHourCents) + " for every hour you're live" + (S->ReadCents > 0 ? ", plus " + Money(S->ReadCents) + " a read." : "."));
	Sound(SoundId::Cash, 0.8);
	Save();
	return "";
}

void Session::DeclineDeal(const std::string& Id)
{
	if (Channel.Offers.erase(Id))
	{
		Channel.Declined.insert(Id);
		Save();
	}
}

bool Session::PayRent()
{
	if (Life.RentStage == life::Rent::Evicted || BankrollCents < Life.RentDueCents || T)
	{
		return false;
	}
	BankrollCents -= Life.RentDueCents;
	Life.Record(WorldMinutes(), "Rent", -Life.RentDueCents, 3);
	++Life.RentsPaid;
	Life.RentStage = life::Rent::Paid;
	Life.RentDueCents = 107500;
	// Next month: November 1, then every thirty days.
	Life.RentDeadline = Life.RentsPaid == 1 ? 27.0 * net::MinutesPerDay : Life.RentDeadline + 30.0 * net::MinutesPerDay;
	Hooks.Text("Landlord", "Got it. Don't make me come up there again.");
	StoryText("rent-paid", "Dee", "RENT PAID?! Look at you. Laundromat's buying the coffee.");
	Sound(SoundId::Cash, 1.0);
	Save();
	return true;
}

bool Session::PayDebt()
{
	if (Life.DebtCents <= 0 || BankrollCents < Life.DebtCents)
	{
		return false;
	}
	BankrollCents -= Life.DebtCents;
	Life.Record(WorldMinutes(), "Paid Marcus", -Life.DebtCents, 2);
	Life.DebtCents = 0;
	Hooks.Text("Marcus", "we're good. don't lose another one.");
	Save();
	return true;
}

void Session::HandleKnockout(const TEvent& E)
{
	if (T && E.EliminatedBy == HeroId)
	{
		StreamMoment(kast::Moment::Knockout, E.Name);
	}
	if (!T || T->Spec.BountyCents <= 0 || E.EliminatedBy.empty())
	{
		return;
	}
	const bool Mine = E.EliminatedBy == HeroId;
	Chips Cash = 0;
	if (T->Spec.MysteryBounty)
	{
		// Envelopes open only in the money.
		if (Mine && T->InTheMoney())
		{
			Cash = life::MysteryEnvelope(T->Spec.BountyCents, LifeRng);
		}
	}
	else
	{
		const Chips Head = Bounties[E.Id];
		Bounties[E.Id] = 0;
		Cash = Head / 2;
		Bounties[E.EliminatedBy] += Head - Cash;
	}
	if (!Mine)
	{
		return;
	}
	++Knockouts;
	if (Cash > 0)
	{
		BountyWon += Cash;
		LastBountyAt = Now;
		LastBountyCents = Cash;
		LastBountyName = E.Name;
		SystemLine((T->Spec.MysteryBounty ? "Mystery envelope: " : "Bounty: ") + Money(Cash) + " for knocking out " + E.Name + ".");
		Sound(SoundId::Cash, 1.0);
	}
}

void Session::NameField()
{
	// The engine names the field its own way (kept for parity with the TypeScript build); on RiverLine,
	// everyone has a screen name and a country, and some of them are the network's regulars at these stakes.
	FieldCountry.clear();
	FieldRegulars.clear();
	FieldPros.clear();
	const net::Network& Net = net::Shared();
	Rng Nr(T->Spec.Id + ":" + SeedBase + ":field");
	const std::string& Pop = T->Spec.Population;
	const net::Tier Stakes = Pop == "low" ? net::Tier::Low : Pop == "high" ? net::Tier::Mid : net::Tier::Micro;
	std::vector<int> Strong;
	std::vector<int> Weak;
	for (size_t I = 0; I < Net.Players().size(); ++I)
	{
		const net::Player& P = Net.Players()[I];
		const bool Match = Stakes == net::Tier::Mid ? (P.Stake == net::Tier::Mid || P.Stake == net::Tier::High) : P.Stake == Stakes;
		if (Match && !P.Rival && P.Name != HeroName)
		{
			(P.Skill >= 0.55f ? Strong : Weak).push_back(static_cast<int>(I));
		}
	}
	Nr.Shuffle(Strong);
	Nr.Shuffle(Weak);
	// Strong regulars take strong bots' seats; the fish take the fish's.
	std::vector<size_t> StrongSeats;
	std::vector<size_t> WeakSeats;
	for (size_t I = 0; I < T->Players.size(); ++I)
	{
		const TPlayer& P = T->Players[I];
		if (P.IsHero || P.Name == RivalName)
		{
			continue;
		}
		const Archetype A = P.Prof.Type;
		(A == Archetype::Reg || A == Archetype::Crusher || A == Archetype::Tag || A == Archetype::Lag ? StrongSeats : WeakSeats).push_back(I);
	}
	Nr.Shuffle(StrongSeats);
	Nr.Shuffle(WeakSeats);
	// Nobody else at the tables goes by a regular's name.
	std::set<std::string> Taken = {HeroName, RivalName};
	for (const net::Player& P : Net.Players())
	{
		Taken.insert(P.Name);
	}
	std::set<size_t> Named;
	const int Regulars = std::min(40, std::max(4, T->Spec.Entrants * 3 / 100));
	for (int K = 0; K < Regulars; ++K)
	{
		const bool Good = K % 2 == 0;
		std::vector<int>& From = Good ? (Strong.empty() ? Weak : Strong) : (Weak.empty() ? Strong : Weak);
		std::vector<size_t>& Into = Good ? (StrongSeats.empty() ? WeakSeats : StrongSeats) : (WeakSeats.empty() ? StrongSeats : WeakSeats);
		if (From.empty() || Into.empty())
		{
			break;
		}
		const net::Player& Reg = Net.Players()[static_cast<size_t>(From.back())];
		TPlayer& Seat = T->Players[Into.back()];
		From.pop_back();
		Named.insert(Into.back());
		Into.pop_back();
		Seat.Name = Reg.Name;
		FieldCountry[Seat.Id] = Reg.Country;
		FieldRegulars.insert(Seat.Id);
		if (Reg.Pro)
		{
			FieldPros.insert(Seat.Id);
		}
		Taken.insert(Reg.Name);
	}
	std::vector<size_t> Rest;
	for (size_t I = 0; I < T->Players.size(); ++I)
	{
		const TPlayer& P = T->Players[I];
		if (!P.IsHero && P.Name != RivalName && Named.count(I) == 0)
		{
			Rest.push_back(I);
		}
	}
	const std::vector<handles::Identity> Ids = handles::Field(static_cast<int>(Rest.size()), Nr, Taken);
	for (size_t K = 0; K < Rest.size() && K < Ids.size(); ++K)
	{
		TPlayer& P = T->Players[Rest[K]];
		P.Name = Ids[K].Name;
		FieldCountry[P.Id] = Ids[K].Country;
	}
	FieldCountry["npc:" + std::string(RivalName)] = "CA";
}

bool Session::CheckSatellite()
{
	if (!T || SeatWon || HasBustInfo || T->Spec.SeatValueCents <= 0 || T->Hero().Busted || T->Remaining > SeatsInPlay())
	{
		return false;
	}
	// Everyone left wins a seat: the satellite is over.
	SeatWon = true;
	Sprinting = false;
	HasPrompt = false;
	HasBustInfo = true;
	BustPlace = std::max(1, T->HeroRank());
	BustPrize = 0;
	BustAt = Now + 4.0;
	Hooks.Celebrate();
	CurrentBanner.Active = true;
	CurrentBanner.Title = "SEAT WON";
	CurrentBanner.Sub = "A " + Money(T->Spec.SeatValueCents) + " ticket \xC2\xB7 " + std::to_string(T->Remaining) + " seats awarded";
	CurrentBanner.At = Now;
	CurrentBanner.Color = session_detail::Gold;
	return true;
}

void Session::CheckUnlocks()
{
	int Cashes = 0;
	int Finals = 0;
	int Wins = 0;
	for (const HistoryEntry& H : History)
	{
		Cashes += H.Prize > 0 ? 1 : 0;
		Finals += H.Place >= 1 && H.Place <= 9 ? 1 : 0;
		Wins += H.Place == 1 ? 1 : 0;
	}
	auto Open = [&](const char* Id, bool When, const std::string& Body) {
		if (When && Life.Unlocks.insert(Id).second)
		{
			Hooks.Text("RiverLine", Body);
		}
	};
	Open("bounty", Cashes > 0, "New on your account: bounty tournaments. Progressive knockouts and mystery bounties, every head pays.");
	Open("satellite", Finals > 0, "New on your account: satellites. Win your way up the steps to the RCOP Main Event.");
	Open("sixmax", Wins > 0, "New on your account: six-max tournaments. Fewer seats, more action.");
	if (Cashes > 0)
	{
		StoryText("sam-intro", "Sam", "saw ur name on the riverline board. i pay ppl to play my account when im busy. hit me on the burner");
	}
}

void Session::CheckCalendar(double From, double To, bool Awake)
{
	const double Hours = (To - From) / 60.0;
	if (Awake)
	{
		// A good chair and real coffee slow the drain; being live on stream speeds it up.
		const double Drain = Fx.Drain > 0.0 || Stream.Live ? 4.0 * (1.0 - Fx.Drain) + (Stream.Live ? 1.0 : 0.0) : 4.0;
		Life.Energy = std::min(100.0, std::max(0.0, Life.Energy - Hours * Drain));
	}
	RenewGear(From, To);
	Life.Heat = std::min(100.0, std::max(0.0, Life.Heat - Hours * 1.0));
	// The Night Shift closes at 6 AM.
	for (double End = std::floor(From / net::MinutesPerDay) * net::MinutesPerDay + 6.0 * 60.0; End <= To; End += net::MinutesPerDay)
	{
		if (End > From)
		{
			PayNightShift(End);
		}
	}
	if (Life.RentStage != life::Rent::Evicted && Life.RentDeadline > From && Life.RentDeadline <= To)
	{
		RentDeadline();
	}
	// The landlord's reminders.
	const double Thursday = 3.0 * net::MinutesPerDay + 9.0 * 60.0;
	const double FridayEvening = 4.0 * net::MinutesPerDay + 18.0 * 60.0;
	if (Life.RentStage == life::Rent::Due && Life.RentsPaid == 0)
	{
		if (Thursday > From && Thursday <= To)
		{
			StoryText("rent-thursday", "Landlord", "Tomorrow. $1,225. I'm not asking again.");
		}
		if (FridayEvening > From && FridayEvening <= To)
		{
			StoryText("rent-friday", "Landlord", "Midnight. Have it or start packing.");
		}
	}
	if (Life.BannedUntil > From && Life.BannedUntil <= To)
	{
		Hooks.Text("RiverLine", "Review complete. Your account is active again. Further violations may result in permanent closure.");
	}
	// Sundays: Dee deals the Riverside's $150, and says so in the afternoon.
	if (const life::Activity* Live = life::Find("riverside"))
	{
		for (double At = std::floor(From / net::MinutesPerDay) * net::MinutesPerDay + 16.0 * 60.0; At <= To; At += net::MinutesPerDay)
		{
			if (At <= From || !life::InWindow(*Live, At + 2.0 * 60.0 + 1.0) || Life.RentStage == life::Rent::Evicted)
			{
				continue;
			}
			StoryText("dee-riverside-" + std::to_string(net::DayOf(At)), "Dee",
				Life.LiveEvents == 0 ? "i deal the riverside $150 on sundays. seven o'clock, forty-something players, deep stacks. sal and mei play it. you should too."
									 : "riverside tonight. seven. i've got the feature table.");
		}
	}
	// Game nights at Dee's: a heads-up half an hour before the doors open.
	if (const life::Activity* Game = life::Find("dee-game"))
	{
		for (double At = std::floor(From / net::MinutesPerDay) * net::MinutesPerDay + Game->Opens - 30.0; At <= To; At += net::MinutesPerDay)
		{
			if (At <= From || !life::InWindow(*Game, At + 31.0) || Life.RentStage == life::Rent::Evicted)
			{
				continue;
			}
			static const char* Nights[] = {
				"doors at nine. lou's already here, eating.",
				"game's on tonight. sal brought his lucky coffee.",
				"nine o'clock. twitch is wound up, should be a good one.",
				"tonight, nine. mei's back. watch yourself.",
			};
			const int Day = net::DayOf(At);
			StoryText("dee-night-" + std::to_string(Day), "Dee",
				Life.BackRoomNights == 0 ? "game's tonight at nine. back room of the laundromat, across the street. bring forty. open the burner if you're in." : Nights[Day % 4]);
		}
	}
}

void Session::PayNightShift(double End)
{
	const int Day = net::DayOf(End);
	if (Life.NightsPaid.count(Day) > 0)
	{
		return;
	}
	Life.NightsPaid.insert(Day);
	net::Network& Net = net::Shared();
	Net.SetHero(HeroName, History);
	const double At = End - 0.01;
	net::BoardRow Row;
	Net.Leaderboard(net::Board::NightShift, At, net::StatsFrom(HeroName, History, At), 0, &Row);
	if (Row.Rank >= 1 && Row.Prize > 0)
	{
		BankrollCents += Row.Prize;
		Life.Record(End, "Night Shift: " + Ordinal(Row.Rank), Row.Prize, 4);
		Hooks.Text("RiverLine", "You finished " + Ordinal(Row.Rank) + " on the Night Shift leaderboard. " + Money(Row.Prize) + " has been added to your balance.");
		Save();
	}
}

void Session::RentDeadline()
{
	if (BankrollCents >= Life.RentDueCents)
	{
		// The landlord comes up the stairs and takes it.
		BankrollCents -= Life.RentDueCents;
		Life.Record(Life.RentDeadline, "Rent (collected)", -Life.RentDueCents, 3);
		++Life.RentsPaid;
		Life.RentStage = life::Rent::Paid;
		Life.RentDueCents = 107500;
		Life.RentDeadline = Life.RentsPaid == 1 ? 27.0 * net::MinutesPerDay : Life.RentDeadline + 30.0 * net::MinutesPerDay;
		Hooks.Text("Landlord", "Came by for the rent. It's on the counter? Fine. Next month, don't make me climb the stairs.");
	}
	else if (Life.RentStage != life::Rent::FinalNotice)
	{
		Life.RentStage = life::Rent::FinalNotice;
		Life.RentDueCents += 15000;
		Life.RentDeadline += 3.0 * net::MinutesPerDay;
		Hooks.Text("Landlord", "FINAL NOTICE. " + Money(Life.RentDueCents) + " with the late fee. You have until Monday at midnight, then the locks change.");
	}
	else
	{
		Life.RentStage = life::Rent::Evicted;
		Hooks.Text("Landlord", "Locks change tomorrow. Leave the key on the counter.");
		Hooks.Text("Dee", "I heard. My couch is yours as long as you need it. Bring the laptop.");
	}
	Save();
}
} // namespace ss
