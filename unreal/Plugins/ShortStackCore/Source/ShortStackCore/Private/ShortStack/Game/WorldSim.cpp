// The living world's days: who plays what, how it goes, and what a week, a month and a year do to people.
//
// The cards are never touched here. Skill decides how likely a good finish is (a finishing place is drawn from
// a distribution that leans toward the front of the field the more a player out-skills it), and luck decides
// the rest. Form, confidence and fatigue change choices: how much people play, what they can afford, whether
// they take a shot or step away. Tilt and burnout cost them focus at the table, never luck.
#include "ShortStack/Game/World.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Handles.h"
#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Structure.h"
#include "WorldSim.h"

#include <algorithm>
#include <cmath>
#include <set>

namespace ss
{
namespace world
{
namespace worldsim_detail
{
// How strongly an edge over the field moves a finish toward the front (calibrated: +0.1 is roughly +20% ROI).
const double EdgeScale = 0.8;
const double MaxEdge = 0.35;
// A tier's field is a little softer than the regulars who started there (everyone else plays too).
const double FieldSoftness = 0.0;

bool Has(int Bits, int Bit)
{
	return (Bits & Bit) != 0;
}

std::string Seeded(const World& W, const char* What, const std::string& Key)
{
	return "world/" + std::to_string(W.Seed()) + "/" + What + "/" + Key;
}

/** An online option or a live event someone could enter today. */
struct Option
{
	Pending P;
	double LogBuy = 0.0;
	int Minute = 0; // of the day it starts
	bool Featured = false;
	bool Weekly = false; // a weekly major (not a daily)
	bool Underground = false;
	int Cap = 0;
	std::string Template; // online template id ("" live)
	std::string Final;    // satellites: the Main Event the chain of seats ends at
	LiveLevel Level = LiveLevel::None;
	Region Where = Region::Americas;
	std::set<int> Invited; // the Summit
};

double Lerp(double A, double B, double T)
{
	return A + (B - A) * T;
}

/** Points on the boards for a finish (the network's formula). */
double PointsFor(const Pending& P, int Place)
{
	return P.BuyIn > 0 ? net::Points(Place, P.Entries, P.BuyIn) : 0.0;
}

/** The ticket key an event uses (what a satellite seat into it is called). */
std::string TicketKey(const Pending& P)
{
	if (!P.Online)
	{
		return P.Kind == static_cast<int>(LiveKind::ChampionshipMain) ? "championship-main" : "";
	}
	const size_t At = P.Id.rfind('@');
	return net::Shared().TicketOf(At == std::string::npos ? P.Id : P.Id.substr(0, At));
}

/** Where a satellite's chain of seats ends (step 1 -> step 2 -> ... -> the Main Event). */
std::string FinalTarget(const std::string& Ticket)
{
	const net::Network& Net = net::Shared();
	std::string At = Ticket;
	for (int Guard = 0; Guard < 8; ++Guard)
	{
		const net::EventTemplate* T = Net.FindTemplate(At);
		if (!T || T->Fmt != net::Format::Satellite || T->SeatTicket.empty())
		{
			break;
		}
		At = T->SeatTicket;
	}
	return At;
}

Chips SeatValueOf(const std::string& Ticket)
{
	if (Ticket == "championship-main")
	{
		return 1000000;
	}
	return net::Shared().SeatValue(Ticket);
}

/** Notable enough for the record (a player card's recent results, the news). */
bool Notable(const Pending& P, int Place, Chips Prize, int Tier)
{
	return Place <= P.FinalSize || Prize >= sim::TierAbi[sim::TierIndex(Tier)] * 30 || (P.Major && Prize > 0 && (Place <= P.Entries / 50 || Prize >= P.BuyIn * 10)) || Prize >= 250000;
}

/** An anonymous finisher's name (the same in every replay of this save). */
void Anonymous(const World& W, const Pending& P, int Place, std::string& Name, std::string& Country)
{
	Rng A(Seeded(W, "anon", P.Id + "/" + std::to_string(Place)));
	Country = handles::PickCountry(A);
	Name = handles::Make(A, Country, P.Tier >= 3);
	for (int Try = 0; W.Find(Name) >= 0 && Try < 6; ++Try)
	{
		Name = handles::Make(A, Country, P.Tier >= 3);
	}
	if (W.Find(Name) >= 0)
	{
		Name += std::to_string(A.Int(90) + 10);
	}
}

int OnlineCount(const Npc& N, int Day, bool SeriesOn, Rng& R)
{
	const int Wd = net::Weekday(Day);
	double Base = 0.0;
	switch (N.Schedule)
	{
	case Plan::Daily: Base = (Wd >= 5 ? 11.0 : 9.0) * (N.Professional ? 1.8 : 1.0); break; // pros multi-table
	case Plan::Nights: Base = Wd == 6 ? 6.0 : Wd == 5 ? 4.0 : 3.0; break;
	case Plan::Weekends: Base = Wd == 6 ? 7.0 : Wd == 5 ? 5.0 : (R.Chance(0.15) ? 2.0 : 0.0); break;
	case Plan::Occasional: Base = R.Chance(Wd == 6 ? 0.45 : 0.16) ? 1.0 + static_cast<double>(R.Int(3)) : 0.0; break;
	case Plan::SeriesFocus: Base = SeriesOn ? 9.0 : Wd == 6 ? 4.0 : (R.Chance(0.3) ? 2.0 : 0.0); break;
	case Plan::MajorsOnly: Base = Wd == 6 ? 4.0 : (R.Chance(0.2) ? 1.0 : 0.0); break;
	case Plan::LiveCircuit: Base = R.Chance(0.5) ? 1.0 + static_cast<double>(R.Int(3)) : 0.0; break;
	default: break;
	}
	if (Base <= 0.0)
	{
		return 0;
	}
	double M = static_cast<double>(N.OnlineShare) / 0.95;
	switch (N.Mood)
	{
	case Momentum::Heater: M *= 1.2; break;
	case Momentum::Downswing: M *= N.TraitOf(Trait::Grit) > 0.6f ? 1.12 : 0.8; break;
	case Momentum::MajorDownswing: M *= 0.6; break;
	case Momentum::Burnout: M *= 0.3; break;
	case Momentum::Confident: M *= 1.1; break;
	case Momentum::Breakout: M *= 1.15; break;
	case Momentum::Rebuilding: M *= 0.85; break;
	default: break;
	}
	M *= 1.0 - 0.6 * static_cast<double>(N.Fatigue);
	if (N.TripUntil >= Day && Day >= N.TripFrom)
	{
		M *= 0.2;
	}
	if (N.Professional)
	{
		M *= 1.15;
	}
	M *= R.Range(0.6, 1.4);
	return std::min(N.Professional ? 24 : 14, static_cast<int>(std::lround(Base * M)));
}

/** Who goes to the room's own weekly, and the back-room games (the people with faces first). */
bool Regular(const Npc& N, const std::string& Id)
{
	if (N.From != Origin::Cast && !N.Rival)
	{
		return false;
	}
	if (Id.rfind("dees@", 0) == 0)
	{
		return N.Name == "Sal" || N.Name == "Big Lou" || N.Name == "Twitch" || N.Name == "Mei";
	}
	return false;
}

Ledger& LedgerOf(Npc& N, const Pending& P)
{
	const Venue V = P.Online ? Venue::Online : Has(P.Kinds, KindUnderground) ? Venue::Underground : Venue::Live;
	return N.Totals[static_cast<size_t>(V)];
}

void AddFame(Npc& N, Rep R, double Amount)
{
	N.Fame[static_cast<size_t>(R)] = static_cast<float>(static_cast<double>(N.Fame[static_cast<size_t>(R)]) + Amount);
}

void Milestones(World& W, Npc& N, Chips Before, Chips After, double At)
{
	static const double Marks[] = {100000.0, 1000000.0, 5000000.0, 10000000.0, 25000000.0};
	for (double M : Marks)
	{
		const Chips C = sim::Cents(M);
		if (Before < C && After >= C)
		{
			Sim::Post(W, At, EventKind::Milestone, N.Id, "career-won", C, 0);
		}
	}
}
} // namespace worldsim_detail

using namespace worldsim_detail;

// ------------------------------------------------------------------ small pieces

double Sim::Score(const Npc& N, int Kinds)
{
	// What the event rewards.
	std::array<double, SkillCount> Wt{};
	auto Set = [&](Skill S, double V) { Wt[static_cast<size_t>(S)] += V; };
	Set(Skill::Preflop, 0.14);
	Set(Skill::Postflop, 0.18);
	Set(Skill::Aggression, 0.1);
	Set(Skill::Bluffing, 0.07);
	Set(Skill::Value, 0.1);
	Set(Skill::Icm, 0.1);
	Set(Skill::ShortStack, 0.06);
	Set(Skill::DeepStack, 0.06);
	Set(Skill::HeadsUp, 0.04);
	Set(Skill::Adjust, 0.08);
	Set(Skill::Emotion, 0.07);
	if (Has(Kinds, KindBounty))
	{
		Set(Skill::Bounty, 0.22);
	}
	if (Has(Kinds, KindSatellite))
	{
		Set(Skill::Satellite, 0.25);
		Set(Skill::Icm, 0.1);
	}
	if (Has(Kinds, KindTurbo) || Has(Kinds, KindHyper))
	{
		Set(Skill::ShortStack, 0.15);
		Set(Skill::Preflop, 0.1);
	}
	if (Has(Kinds, KindDeep))
	{
		Set(Skill::DeepStack, 0.15);
		Set(Skill::Postflop, 0.1);
	}
	if (Has(Kinds, KindLive) || Has(Kinds, KindUnderground))
	{
		Set(Skill::Live, 0.25);
	}
	if (Has(Kinds, KindSixMax))
	{
		Set(Skill::Aggression, 0.06);
		Set(Skill::HeadsUp, 0.06);
	}
	double S = 0.0;
	double T = 0.0;
	for (size_t I = 0; I < Wt.size(); ++I)
	{
		S += Wt[I] * static_cast<double>(N.Skills[I]);
		T += Wt[I];
	}
	S /= T;
	// Omaha: most Hold'em players are lost in it.
	if (Has(Kinds, KindOmaha) && !(N.Formats & LikeOmaha))
	{
		S -= 0.08;
	}
	// Focus: tilt, fatigue and burnout cost decisions (never cards).
	const double Control = static_cast<double>(N.SkillOf(Skill::Emotion));
	S -= 0.09 * static_cast<double>(N.Tilt) * (1.0 - Control);
	S -= 0.05 * static_cast<double>(N.Fatigue);
	if (N.Mood == Momentum::Burnout)
	{
		S -= 0.04;
	}
	if (N.Mood == Momentum::Confident || N.Mood == Momentum::Heater)
	{
		S += 0.01; // playing their best: fewer second guesses
	}
	return S;
}

void Sim::Post(World& W, double At, EventKind K, int Who, const std::string& What, Chips Amount, int Value, int Other)
{
	WorldEvent E;
	E.At = At;
	E.Kind = K;
	E.Npc = Who;
	E.Other = Other;
	E.Amount = Amount;
	E.What = What;
	E.Value = Value;
	W.Log.push_back(std::move(E));
	if (W.Log.size() > sim::MaxLog)
	{
		W.Log.erase(W.Log.begin(), W.Log.begin() + 1000);
	}
	// The turns of a career go on their journey too.
	if (Who < 0 || Who >= static_cast<int>(W.Roster.size()))
	{
		return;
	}
	Npc& N = W.Roster[static_cast<size_t>(Who)];
	const int Day = sim::DayAt(At);
	switch (K)
	{
	case EventKind::MovedUp: Mark(N, Day, StepKind::MovedUp, What, Value); break;
	case EventKind::MovedDown: Mark(N, Day, StepKind::MovedDown, What, Value); break;
	case EventKind::WentBroke: Mark(N, Day, StepKind::WentBroke, What); break;
	case EventKind::Retired: Mark(N, Day, StepKind::Retired, What, Value, 0, Amount); break;
	case EventKind::Break: Mark(N, Day, StepKind::Break, What, Value); break;
	case EventKind::Returned: Mark(N, Day, StepKind::Returned, What, Value); break;
	case EventKind::Sponsored: Mark(N, Day, StepKind::Sponsored, What); break;
	case EventKind::StartedStreaming: Mark(N, Day, StepKind::StartedStreaming, What, Value); break;
	case EventKind::PlayerOfYear: Mark(N, Day, StepKind::PlayerOfYear, What, Value); break;
	case EventKind::Milestone:
		if (What == "turned-pro")
		{
			Mark(N, Day, StepKind::TurnedPro, What, Value);
		}
		break;
	default: break;
	}
}

void Sim::Mark(Npc& N, int Day, StepKind K, const std::string& What, int Place, int Of, Chips Amount)
{
	if (N.Faded)
	{
		return;
	}
	Step S;
	S.Day = Day;
	S.Kind = K;
	S.What = What;
	S.Place = Place;
	S.Of = Of;
	S.Amount = Amount;
	N.Path.push_back(std::move(S));
	// The beginning stays (how they arrived and their firsts); after that, the latest.
	const size_t Keep = 6;
	const size_t Most = 16;
	if (N.Path.size() > Most)
	{
		N.Path.erase(N.Path.begin() + static_cast<std::ptrdiff_t>(Keep));
	}
}

Award Sim::AwardOf(const Pending& P, int Day, Chips Prize)
{
	Award A;
	A.Day = Day;
	A.Ring = P.Ring && !P.Bracelet;
	A.Online = P.Online;
	A.Series = P.Series;
	A.Event = P.Name;
	A.Prize = Prize;
	A.Entries = P.Entries;
	if (P.Online)
	{
		// Online: the series' own Main Event (its id says which).
		const net::Network& Net = net::Shared();
		const net::EventTemplate* T = P.Template >= 0 && static_cast<size_t>(P.Template) < Net.Templates().size() ? &Net.Templates()[static_cast<size_t>(P.Template)] : nullptr;
		const net::SeriesInfo* Sr = T ? Net.FindSeries(T->Series) : nullptr;
		A.Main = Sr && Sr->MainEvent == T->Id;
	}
	else
	{
		A.Main = P.Kind == static_cast<int>(LiveKind::ChampionshipMain) || P.Name.find("Main Event") != std::string::npos;
	}
	return A;
}

void Sim::SetMood(Npc& N, Momentum M, int Day)
{
	if (N.Mood != M)
	{
		N.Mood = M;
		N.MoodSince = Day;
	}
}

Year& Sim::YearRow(Npc& N, int Day)
{
	const int Y = world::YearOf(Day);
	if (N.Years.empty() || N.Years.back().Number != Y)
	{
		Year Row;
		Row.Number = Y;
		N.Years.push_back(Row);
		if (N.Years.size() > 40)
		{
			N.Years.erase(N.Years.begin());
		}
	}
	return N.Years.back();
}

void Sim::Reputation(Npc& N)
{
	auto F = [&](Rep R) { return static_cast<double>(N.Fame[static_cast<size_t>(R)]); };
	auto Curve = [](double X, double Scale) { return static_cast<float>(100.0 * (1.0 - std::exp(-std::max(0.0, X) / Scale))); };
	N.Fame[static_cast<size_t>(Rep::Streaming)] = static_cast<float>(N.Streams || N.Followers > 0 ? static_cast<double>(N.Followers) / 2500.0 : 0.0);
	N.Reps[static_cast<size_t>(Rep::Online)] = Curve(F(Rep::Online), 35.0);
	N.Reps[static_cast<size_t>(Rep::Live)] = Curve(F(Rep::Live), 30.0);
	N.Reps[static_cast<size_t>(Rep::Underground)] = Curve(F(Rep::Underground), 18.0);
	N.Reps[static_cast<size_t>(Rep::Streaming)] = Curve(F(Rep::Streaming), 40.0);
	N.Reps[static_cast<size_t>(Rep::HighStakes)] = Curve(F(Rep::HighStakes), 30.0);
	// Overall: what the poker world at large knows them for (live titles travel furthest).
	const double All = F(Rep::Online) / 80.0 + F(Rep::Live) / 45.0 + F(Rep::Underground) / 150.0 + F(Rep::Streaming) / 150.0 + F(Rep::HighStakes) / 70.0;
	N.Reps[static_cast<size_t>(Rep::Overall)] = static_cast<float>(100.0 * (1.0 - std::exp(-All)));
}

bool Sim::Major(const Pending& P)
{
	return P.Major;
}

// ------------------------------------------------------------------ events

Pending Sim::OnlineEvent(World& W, const net::EventInstance& E)
{
	const net::Network& Net = net::Shared();
	const net::EventTemplate& T = Net.TemplateOf(E);
	Pending P;
	P.Id = E.Id;
	P.Name = !T.Series.empty() ? T.Name : net::BuyIn(T.BuyInCents) + " " + T.Name;
	P.Online = true;
	P.Template = E.Template;
	P.Start = E.Start;
	P.End = E.Start + E.Duration;
	P.Entries = E.Entries;
	P.Pool = E.Pool;
	P.BuyIn = T.BuyInCents;
	P.Tier = static_cast<int>(net::TierOf(T.BuyInCents));
	P.Format = static_cast<int>(T.Fmt);
	P.TableSize = T.TableSize;
	P.Series = T.Series;
	P.Ticket = T.SeatTicket;
	P.SeatValue = T.SeatValueCents;
	P.Seats = T.Fmt == net::Format::Satellite && T.SeatValueCents > 0 ? std::max<int>(1, static_cast<int>(E.Pool / T.SeatValueCents)) : 0;
	P.FinalSize = std::min(T.TableSize >= 8 ? 9 : T.TableSize, std::max(2, E.Entries));
	int K = 0;
	K |= T.Fmt == net::Format::Bounty || T.Fmt == net::Format::Mystery ? KindBounty : 0;
	K |= T.Fmt == net::Format::Mystery ? KindMystery : 0;
	K |= T.Fmt == net::Format::Satellite ? KindSatellite : 0;
	K |= T.Fmt == net::Format::ReEntry ? KindReEntry : 0;
	K |= T.Speed == "Turbo" ? KindTurbo : 0;
	K |= T.Speed == "Hyper" || T.Fmt == net::Format::Flip ? KindHyper : 0;
	K |= T.Speed == "Deep" ? KindDeep : 0;
	K |= T.Omaha ? KindOmaha : 0;
	K |= T.TableSize < 8 ? KindSixMax : 0;
	P.Kinds = K;
	P.Luck = Has(K, KindHyper) ? 0.55 : Has(K, KindTurbo) ? 0.8 : Has(K, KindDeep) ? 1.15 : 1.0;
	double S = W.Field[static_cast<size_t>(sim::TierIndex(P.Tier))];
	S -= E.Entries > 10000 ? 0.05 : E.Entries > 3000 ? 0.03 : 0.0;
	S += T.BuyInCents >= 100000 ? 0.04 : 0.0;
	S -= Has(K, KindSatellite) ? 0.02 : 0.0;
	S -= !T.Series.empty() ? 0.01 : 0.0;
	P.Strength = S;
	P.Major = T.Main || E.Pool >= 100000000;
	P.Bracelet = T.Bracelet;
	P.Ring = T.Ring;
	return P;
}

Pending Sim::LiveEventOf(World& W, const LiveEvent& E)
{
	Pending P;
	P.Id = E.Id;
	P.Name = E.Series.empty() ? E.Name : E.Series + ": " + E.Name;
	P.Online = false;
	P.Kind = static_cast<int>(E.Kind);
	P.Start = E.Start;
	P.End = E.Start + E.Duration;
	P.Entries = E.Field;
	P.BuyIn = E.BuyIn;
	P.Stakes = E.Stakes;
	P.Series = E.Series;
	P.Ticket = E.Ticket;
	P.Bracelet = E.Bracelet;
	P.Ring = E.Ring;
	P.TableSize = E.Kind == LiveKind::Summit ? 8 : live::IsRiverside(E.Id) ? live::RiversideTableSize : 9;
	P.FinalSize = std::min(P.TableSize, std::max(2, E.Field));
	const bool Summit = E.Kind == LiveKind::Summit;
	// The calendar is the same in every save; the crowds aren't (the Riverside's is the player's own room's).
	if (!Summit && E.Kind != LiveKind::Underground && !live::IsRiverside(E.Id))
	{
		const double Unit = static_cast<double>(Fnv1a(std::to_string(W.Seed()) + "/" + E.Id) % 10000u) / 10000.0;
		P.Entries = std::max(8, static_cast<int>(std::lround(static_cast<double>(E.Field) * (0.88 + 0.24 * Unit))));
	}
	P.Pool = static_cast<Chips>(P.Entries) * E.BuyIn * (Summit ? 100 : 90) / 100;
	P.Tier = static_cast<int>(net::TierOf(std::max<Chips>(1, E.BuyIn)));
	if (!E.Ticket.empty())
	{
		P.SeatValue = SeatValueOf(E.Ticket);
		P.Seats = P.SeatValue > 0 ? std::max<int>(1, static_cast<int>(P.Pool / P.SeatValue)) : 0;
	}
	int K = KindLive;
	K |= E.Kind == LiveKind::Underground ? KindUnderground : 0;
	K |= !E.Ticket.empty() ? KindSatellite : 0;
	K |= E.Name.find("Bounty") != std::string::npos ? KindBounty : 0;
	K |= E.Name.find("Turbo") != std::string::npos ? KindTurbo : 0;
	K |= E.Name.find("Deep") != std::string::npos || E.Name.find("Stack") != std::string::npos ? KindDeep : 0;
	K |= E.Name.find("Six-Max") != std::string::npos ? KindSixMax : 0;
	P.Kinds = K;
	P.Luck = Has(K, KindTurbo) ? 0.8 : Has(K, KindDeep) || E.Main ? 1.15 : 1.0;
	double S = 0.34;
	switch (E.Kind)
	{
	case LiveKind::Local: S = 0.38; break;
	case LiveKind::Underground: S = 0.33 + 0.03 * std::log2(std::max(1.0, sim::Dollars(E.Stakes) / 2.0)); break;
	case LiveKind::Regional: S = E.BuyIn >= 100000 ? 0.55 : 0.47; break;
	case LiveKind::Circuit: S = E.BuyIn >= 500000 ? 0.62 : 0.55; break;
	case LiveKind::HighRoller: S = 0.64 + 0.03 * std::log2(std::max(1.0, sim::Dollars(E.BuyIn) / 10000.0)); break;
	case LiveKind::Championship: S = E.BuyIn <= 100000 ? 0.42 : E.BuyIn <= 300000 ? 0.48 : 0.54; break;
	case LiveKind::ChampionshipMain: S = 0.44; break;
	case LiveKind::Summit: S = 0.76; break;
	}
	P.Strength = S;
	P.Major = E.Kind == LiveKind::ChampionshipMain || Summit || (E.Main && E.Level >= LiveLevel::Circuit) || (E.Bracelet && E.BuyIn >= 1000000) ||
		(E.Kind == LiveKind::HighRoller && E.BuyIn >= 2500000);
	return P;
}

void Sim::PlanDay(World& W, int Day)
{
	for (int D = W.Planned + 1; D <= Day; ++D)
	{
		W.Planned = D;
		const net::Network& Net = net::Shared();
		Rng R(Seeded(W, "plan", std::to_string(D)));

		// What's on: the network's schedule and the live calendar.
		std::vector<Option> Online;
		std::vector<Option> Live;
		bool SeriesOn = false;
		std::map<std::string, bool> Alive;
		for (const net::EventInstance& E : Net.Window(static_cast<double>(D) * 1440.0, static_cast<double>(D + 1) * 1440.0))
		{
			const net::EventTemplate& T = Net.TemplateOf(E);
			if (T.BuyInCents <= 0 || E.Start + E.Duration <= W.Now)
			{
				continue;
			}
			if (T.Fmt == net::Format::Satellite)
			{
				// No one grinds satellites to a Main Event that has already been played.
				auto Found = Alive.find(T.SeatTicket);
				if (Found == Alive.end())
				{
					net::EventInstance Next;
					Found = Alive.emplace(T.SeatTicket, Net.NextFor(FinalTarget(T.SeatTicket), E.Start, Next)).first;
				}
				if (!Found->second)
				{
					continue;
				}
			}
			Option O;
			O.P = OnlineEvent(W, E);
			O.LogBuy = std::log(static_cast<double>(T.BuyInCents));
			O.Minute = static_cast<int>(E.Start - static_cast<double>(D) * 1440.0);
			O.Featured = T.Featured;
			O.Weekly = T.OnlyDay == -9999 && T.Days != 0x7f;
			O.Template = T.Id;
			O.Final = T.Fmt == net::Format::Satellite ? FinalTarget(T.SeatTicket) : std::string();
			O.Cap = std::max(2, E.Entries * 7 / 10);
			SeriesOn = SeriesOn || !T.Series.empty();
			Online.push_back(std::move(O));
		}
		std::sort(Online.begin(), Online.end(), [](const Option& A, const Option& B) { return A.P.BuyIn < B.P.BuyIn || (A.P.BuyIn == B.P.BuyIn && A.P.Start < B.P.Start); });
		for (const LiveEvent& E : LiveCalendar(D))
		{
			if (E.Start + E.Duration <= W.Now)
			{
				continue;
			}
			Option O;
			O.P = LiveEventOf(W, E);
			O.LogBuy = std::log(static_cast<double>(std::max<Chips>(1, E.BuyIn)));
			O.Minute = static_cast<int>(E.Start - static_cast<double>(D) * 1440.0);
			O.Underground = E.Kind == LiveKind::Underground;
			O.Cap = O.Underground ? E.Field : std::max(2, E.Field * 85 / 100);
			O.Level = E.Level;
			O.Where = E.Where;
			if (E.Kind == LiveKind::Summit)
			{
				// The year's best by reputation, by invitation.
				std::vector<std::pair<float, int>> Best;
				for (const Npc& N : W.Roster)
				{
					if (N.Playing())
					{
						Best.push_back({N.RepOf(Rep::Overall), N.Id});
					}
				}
				std::sort(Best.begin(), Best.end(), [](const std::pair<float, int>& A, const std::pair<float, int>& B) { return A.first > B.first || (A.first == B.first && A.second < B.second); });
				for (size_t K = 0; K < Best.size() && K < 36; ++K)
				{
					O.Invited.insert(Best[K].second);
				}
			}
			Live.push_back(std::move(O));
		}

		// Everyone decides, in a different order every day (so the full events fill up fairly).
		std::vector<int> Order;
		Order.reserve(W.Roster.size());
		for (const Npc& N : W.Roster)
		{
			if (N.Playing())
			{
				Order.push_back(N.Id);
			}
		}
		R.Shuffle(Order);
		std::vector<double> Weight(Online.size(), 0.0);
		for (int Id : Order)
		{
			Npc& N = W.Roster[static_cast<size_t>(Id)];
			Rng Q = R.Fork(std::to_string(Id));
			const bool Backed = N.Backer != -1;
			Chips Avail = Backed ? std::max<Chips>(N.Bankroll, sim::TierAbi[sim::TierIndex(N.Tier)] * 40) : N.Bankroll;
			auto Enter = [&](Option& O, Chips Cost, bool Ticket, int Bullets, float Share) {
				Entry En;
				En.Npc = Id;
				En.Ticket = Ticket;
				En.Staked = Backed && !Ticket;
				En.Bullets = Bullets;
				En.Share = Share;
				O.P.Who.push_back(En);
				Avail -= Cost;
			};

			// Live: the room's weekly, the back-room games, a festival on a trip, the big ones.
			int LiveToday = 0;
			for (Option& O : Live)
			{
				if (static_cast<int>(O.P.Who.size()) >= O.Cap || LiveToday >= 1 + (Has(O.P.Kinds, KindSatellite) ? 1 : 0))
				{
					continue;
				}
				const LiveKind Kind = static_cast<LiveKind>(O.P.Kind);
				const bool Forced = Regular(N, O.P.Id);
				const bool OnTrip = !O.P.Series.empty() && N.Trip == O.P.Series && D >= N.TripFrom && D <= N.TripUntil;
				const std::string Key = TicketKey(O.P);
				const bool Ticket = !Key.empty() && N.Tickets.count(Key) > 0;
				bool Go = false;
				Chips Cost = O.P.BuyIn;
				float Share = 1.0f;
				switch (Kind)
				{
				case LiveKind::Local:
				{
					const bool Riverside = live::IsRiverside(O.P.Id);
					if (Riverside && (N.From == Origin::Cast || N.Rival))
					{
						// The room's faces have their games (live::Habit) and play them when the money's there: a broke
						// week keeps even Mrs. Park at home.
						const int H = live::Habit(N.Name, live::FindOccurrence(O.P.Id));
						const double Want = H >= 2 ? 0.7 : H == 1 ? 0.18 : 0.0;
						Go = Want > 0.0 && O.P.BuyIn * 4 <= Avail && Q.Chance(Want);
						break;
					}
					if (O.Where != N.Home || N.Live < LiveLevel::Local || (Riverside && N.Country != "US") || N.TripUntil >= D)
					{
						break;
					}
					// The Riverside runs most days: a local picks a night or two a week.
					const double Want = (N.OnlineShare < 0.7f ? 0.35 : 0.06) * (N.Schedule == Plan::Weekends ? 1.5 : 1.0) * (Riverside ? 0.12 : 1.0);
					Go = Q.Chance(Want) && O.P.BuyIn * 10 <= std::max(Avail, N.Income * 4);
					break;
				}
				case LiveKind::Underground:
				{
					if (Forced)
					{
						Go = true;
						break;
					}
					if (O.Where != N.Home || N.Live < LiveLevel::Local || N.TripUntil >= D)
					{
						break;
					}
					const double Rep = static_cast<double>(N.RepOf(Rep::Underground));
					const bool Invite = O.P.Stakes < 1000 || Rep >= 25.0 || N.Is == Identity::HighStakesGambler;
					const bool Fits = O.P.Stakes * 150 <= Avail;
					const double Want = N.Name == "Rick" ? 0.8 : (N.TraitOf(Trait::Risk) > 0.6f ? 0.04 + Rep / 400.0 : 0.005);
					Go = Invite && Fits && Q.Chance(Want);
					Cost = O.P.Stakes * 100;
					break;
				}
				case LiveKind::Summit:
				{
					if (O.Invited.count(Id) == 0)
					{
						break;
					}
					// Most of a $1M seat is sold to backers: they keep what they could afford.
					Share = static_cast<float>(sim::Clamp(static_cast<double>(N.Bankroll) * 0.25 / static_cast<double>(O.P.BuyIn), 0.05, 1.0));
					Cost = static_cast<Chips>(static_cast<double>(O.P.BuyIn) * static_cast<double>(Share));
					Go = Q.Chance(0.92);
					break;
				}
				default:
				{
					if (!OnTrip && !(Ticket && Kind == LiveKind::ChampionshipMain && N.TripUntil >= D))
					{
						break;
					}
					if (Ticket && TicketKey(O.P) == Key)
					{
						Go = true;
						Cost = 0;
						break;
					}
					// Live players stretch further than online ones (and furthest for a Main Event), but nobody plays a
					// $10,000 event on $2,000.
					const double Stretch = O.P.Major || O.P.Kind == static_cast<int>(LiveKind::ChampionshipMain) ? std::max(5.0, Need(N) * 0.08) : std::max(12.0, Need(N) * 0.3);
					const bool Affordable = static_cast<double>(O.P.BuyIn) * Stretch <= static_cast<double>(Avail + N.Income * 26);
					const double Want = O.P.Major || O.P.Kind == static_cast<int>(LiveKind::ChampionshipMain) ? 0.95 : Has(O.P.Kinds, KindSatellite) ? 0.35 : 0.5;
					Go = Affordable && Q.Chance(Want);
					if (!Go && O.P.Kind == static_cast<int>(LiveKind::ChampionshipMain) && Q.Chance(0.9))
					{
						// They flew out for it: savings, or a piece sold to friends and backers.
						const Chips Own = N.Professional ? Avail / 4 : std::max(Avail / 3, N.Income * 104);
						Share = static_cast<float>(sim::Clamp(static_cast<double>(Own) / static_cast<double>(O.P.BuyIn), 0.1, 1.0));
						Cost = static_cast<Chips>(static_cast<double>(O.P.BuyIn) * static_cast<double>(Share));
						Go = true;
					}
					break;
				}
				}
				if (Go && (Cost <= Avail || Backed || Cost == 0 || N.Income * 8 >= Cost))
				{
					if (Ticket && Cost == 0)
					{
						if (--N.Tickets[Key] <= 0)
						{
							N.Tickets.erase(Key);
						}
					}
					Enter(O, Cost, Ticket && Cost == 0, 1, Share);
					++LiveToday;
				}
			}

			// Online: how much they play today, at what buy-ins, in which formats.
			int Count = OnlineCount(N, D, SeriesOn, Q);
			// Seats won in satellites get used first.
			if (!N.Tickets.empty())
			{
				for (Option& O : Online)
				{
					auto T = N.Tickets.find(net::Shared().TicketOf(O.Template));
					if (T != N.Tickets.end() && static_cast<int>(O.P.Who.size()) < O.Cap)
					{
						Enter(O, 0, true, 1, 1.0f);
						if (--T->second <= 0)
						{
							N.Tickets.erase(T);
						}
						--Count;
						if (N.Tickets.empty())
						{
							break;
						}
					}
				}
			}
			if (Count <= 0 || Online.empty())
			{
				continue;
			}
			const double Comfort = static_cast<double>(Sim::Comfort(N));
			const int Tier = sim::TierIndex(N.Tier);
			double Target = sim::Clamp(Comfort, static_cast<double>(sim::TierLow[Tier]), static_cast<double>(sim::TierHigh[Tier]));
			const double Risk = static_cast<double>(N.TraitOf(Trait::Risk));
			double Shot = 1.6 + 2.5 * Risk;
			if (N.Mood == Momentum::Heater || N.Mood == Momentum::Confident || N.Mood == Momentum::Breakout)
			{
				Shot *= 1.5;
			}
			if (N.Mood == Momentum::Downswing || N.Mood == Momentum::MajorDownswing || N.Mood == Momentum::Rebuilding)
			{
				Target *= N.TraitOf(Trait::Discipline) > 0.5f ? 0.8 : 1.0;
				Shot *= N.TraitOf(Trait::Discipline) > 0.5f ? 0.7 : 1.3; // the disciplined tighten up; the rest chase it
			}
			const double MaxBuy = std::min(Target * Shot, static_cast<double>(Avail) * (0.04 + 0.2 * Risk));
			const double MinBuy = Target / (3.0 + 4.0 * static_cast<double>(N.TraitOf(Trait::Patience)));
			if (MaxBuy < MinBuy || MaxBuy < 25.0)
			{
				continue;
			}
			const double LogTarget = std::log(Target);
			// The candidates: buy-ins in range (the options are sorted by buy-in).
			const auto Lo = std::lower_bound(Online.begin(), Online.end(), MinBuy, [](const Option& O, double V) { return static_cast<double>(O.P.BuyIn) < V; });
			const auto Hi = std::upper_bound(Online.begin(), Online.end(), MaxBuy, [](double V, const Option& O) { return V < static_cast<double>(O.P.BuyIn); });
			const size_t First = static_cast<size_t>(Lo - Online.begin());
			const size_t Last = static_cast<size_t>(Hi - Online.begin());
			if (First >= Last)
			{
				continue;
			}
			double Total = 0.0;
			const bool Nights = N.Schedule == Plan::Nights;
			const bool MajorsOnly = N.Schedule == Plan::MajorsOnly;
			for (size_t K = First; K < Last; ++K)
			{
				Option& O = Online[K];
				double Wt = 0.0;
				if (static_cast<int>(O.P.Who.size()) < O.Cap)
				{
					const double Dl = O.LogBuy - LogTarget;
					Wt = std::exp(-Dl * Dl / (2.0 * 0.75 * 0.75));
					if (Nights && !(O.Minute >= 17 * 60 || O.Minute < 3 * 60))
					{
						Wt *= 0.15;
					}
					const int Kn = O.P.Kinds;
					if (Has(Kn, KindBounty))
					{
						Wt *= (N.Formats & LikeBounty) ? 2.2 : 0.8;
					}
					if (Has(Kn, KindMystery))
					{
						Wt *= (N.Formats & LikeMystery) ? 2.0 : 1.0;
					}
					if (Has(Kn, KindSatellite))
					{
						// Nobody keeps winning seats they already hold.
						const bool Holding = N.Tickets.count(O.P.Ticket) > 0 || (!O.Final.empty() && N.Tickets.count(O.Final) > 0);
						Wt *= Holding ? 0.0 : (N.Formats & LikeSatellite) ? 2.0 : 0.3;
					}
					if (Has(Kn, KindTurbo) || Has(Kn, KindHyper))
					{
						Wt *= (N.Formats & LikeTurbo) ? 2.0 : 1.0 - 0.6 * static_cast<double>(N.TraitOf(Trait::Patience));
					}
					if (Has(Kn, KindDeep))
					{
						Wt *= (N.Formats & LikeDeep) ? 2.0 : 1.0;
					}
					if (Has(Kn, KindOmaha))
					{
						Wt *= (N.Formats & LikeOmaha) ? 3.0 : 0.08;
					}
					Wt *= O.Featured ? 1.3 : 1.0;
					Wt *= O.Weekly ? 2.2 : 1.0;
					Wt *= !O.P.Series.empty() ? (N.Schedule == Plan::SeriesFocus ? 3.0 : 1.6) : 1.0;
					Wt *= O.P.Major ? 1.5 : 1.0;
					if (MajorsOnly && !(O.Weekly || O.P.Major || !O.P.Series.empty()))
					{
						Wt *= 0.05;
					}
				}
				Weight[K] = Wt;
				Total += Wt;
			}
			for (int Pick = 0; Pick < Count && Total > 1e-9; ++Pick)
			{
				double X = Q.Next() * Total;
				size_t Chosen = Last;
				for (size_t K = First; K < Last; ++K)
				{
					if (Weight[K] <= 0.0)
					{
						continue;
					}
					Chosen = K;
					X -= Weight[K];
					if (X < 0.0)
					{
						break;
					}
				}
				if (Chosen >= Last)
				{
					break;
				}
				Option& O = Online[Chosen];
				Total -= Weight[Chosen];
				Weight[Chosen] = 0.0;
				int Bullets = 1;
				if (Has(O.P.Kinds, KindReEntry))
				{
					Bullets += Q.Chance(0.12 + 0.45 * Risk) ? 1 : 0;
					Bullets += Risk > 0.7 && Q.Chance(0.3) ? 1 : 0;
				}
				const Chips Cost = O.P.BuyIn * Bullets;
				if (Cost > Avail && !Backed)
				{
					continue;
				}
				Enter(O, Cost, false, Bullets, 1.0f);
			}
			for (size_t K = First; K < Last; ++K)
			{
				Weight[K] = 0.0;
			}
		}

		// Into the queue: every event someone the world follows is playing (the player may already be in one).
		for (std::vector<Option>* List : {&Online, &Live})
		{
			for (Option& O : *List)
			{
				if (O.P.Who.empty())
				{
					continue;
				}
				auto Already = std::find_if(W.Queue.begin(), W.Queue.end(), [&](const Pending& Q) { return Q.Id == O.P.Id; });
				if (Already == W.Queue.end())
				{
					W.Queue.push_back(std::move(O.P));
					continue;
				}
				for (const Entry& E : O.P.Who)
				{
					if (std::none_of(Already->Who.begin(), Already->Who.end(), [&](const Entry& X) { return X.Npc == E.Npc; }))
					{
						Already->Who.push_back(E);
					}
				}
			}
		}
	}
	std::stable_sort(W.Queue.begin(), W.Queue.end(), [](const Pending& A, const Pending& B) { return A.End < B.End; });
}

// ------------------------------------------------------------------ results

namespace worldsim_detail
{
/** Finds the free place nearest to Want within [Lo, Hi] (worse first, then better). */
int FreePlace(const std::set<int>& Taken, int Want, int Lo, int Hi)
{
	Want = std::max(Lo, std::min(Hi, Want));
	for (int P = Want; P <= Hi; ++P)
	{
		if (Taken.count(P) == 0)
		{
			return P;
		}
	}
	for (int P = Want - 1; P >= Lo; --P)
	{
		if (Taken.count(P) == 0)
		{
			return P;
		}
	}
	return 0;
}

/** Bounty money for a finish: heads collected on the way, bigger the deeper the run (and a big one for the winner). */
Chips Bounties(Rng& R, const Pending& P, int Place, double BountySkill)
{
	const double Each = static_cast<double>(P.Pool) / 2.0 / static_cast<double>(std::max(1, P.Entries));
	// Heads collected: about ln(field / place) on average, more for the ones who hunt well (noise keeps the mean).
	double Heads = std::log(static_cast<double>(P.Entries) / static_cast<double>(Place)) * (0.6 + 0.8 * BountySkill) * std::exp(R.Gauss(-0.125, 0.5));
	if (Place == 1)
	{
		Heads = Heads * 1.6 + 2.0;
	}
	double Money = Each * Heads;
	if (Has(P.Kinds, KindMystery))
	{
		const double Envelope = R.Next();
		Money *= Envelope < 0.002 ? 120.0 : Envelope < 0.02 ? 8.0 : 0.62;
	}
	return static_cast<Chips>(std::llround(Money));
}
} // namespace worldsim_detail

/** One person's result: money, record, season, form, fame, the story. */
void Sim::Apply(World& W, Npc& N, const Pending& P, const Entry& E, int Place, Chips Prize, bool Seat)
{
	const int Day = sim::DayAt(P.End);
	const Chips Gross = P.BuyIn * static_cast<Chips>(std::max(1, E.Bullets));
	const Chips Paid = E.Ticket ? 0 : static_cast<Chips>(static_cast<double>(Gross) * static_cast<double>(E.Share));
	const Chips Kept = static_cast<Chips>(static_cast<double>(Prize) * static_cast<double>(E.Share));
	const Chips Before = N.CareerWon();
	const Chips BestBefore = std::max({N.Totals[0].Best, N.Totals[1].Best, N.Totals[2].Best});
	// Money: their own, or a backer's (makeup first, then the profit is split).
	if (E.Staked && N.Backer != -1)
	{
		Npc* B = N.Backer >= 0 ? &W.Roster[static_cast<size_t>(N.Backer)] : nullptr;
		if (B)
		{
			B->Bankroll -= Paid;
		}
		N.Makeup += Paid;
		const Chips Repay = std::min(Kept, N.Makeup);
		N.Makeup -= Repay;
		const Chips Rest = Kept - Repay;
		const Chips Cut = static_cast<Chips>(static_cast<double>(Rest) * static_cast<double>(N.BackerShare));
		if (B)
		{
			B->Bankroll += Repay + Cut;
		}
		N.Bankroll += Rest - Cut;
		N.WeekNet += Rest - Cut;
	}
	else
	{
		N.Bankroll += Kept - Paid;
		N.WeekNet += Kept - Paid;
	}
	if (Kept >= 5000000 && Kept > N.Bankroll / 3)
	{
		// A life-changing score: taxes, a house, a car, the bank. Some of it stays in poker.
		const double Ambition = static_cast<double>(N.TraitOf(Trait::Ambition));
		const Chips Banked = std::min(N.Bankroll, static_cast<Chips>(static_cast<double>(Kept) * (0.35 + 0.35 * (1.0 - Ambition))));
		N.Bankroll -= Banked;
		W.Flow[4] += sim::Dollars(Banked);
	}
	W.Flow[0] += sim::Dollars(Kept);
	W.Flow[1] += sim::Dollars(Paid);
	if (N.Totals[0].Events + N.Totals[1].Events + N.Totals[2].Events <= 1 && N.From == Origin::Discovered)
	{
		std::array<double, 3>& F = W.FlowBy["discovered"];
		F[0] += sim::Dollars(Kept);
		F[1] += sim::Dollars(Paid);
		F[2] += 1.0;
	}
	else
	{
		static const char* const Tiers[5] = {"free", "micro", "low", "mid", "high"};
		static const char* const Kinds[8] = {"local", "underground", "regional", "circuit", "highroller", "bracelet", "main", "summit"};
		const std::string Key = P.Online ? std::string("online-") + Tiers[sim::TierIndex(P.Tier)] + (Has(P.Kinds, KindBounty) ? "-pko" : "") + (Has(P.Kinds, KindSatellite) ? "-sat" : "")
										 : std::string("live-") + Kinds[std::max(0, std::min(7, P.Kind))];
		std::array<double, 3>& F = W.FlowBy[Key];
		F[0] += sim::Dollars(Kept);
		F[1] += sim::Dollars(Paid);
		F[2] += 1.0;
	}
	N.Bankroll = std::max<Chips>(0, N.Bankroll);
	N.PeakRoll = std::max(N.PeakRoll, N.Bankroll);
	// The record.
	Ledger& L = LedgerOf(N, P);
	++L.Events;
	L.Spent += Gross;
	L.Won += Prize;
	const bool Cash = Prize > 0 || Seat;
	const bool Ft = Place >= 1 && Place <= P.FinalSize;
	const bool Win = Place == 1;
	L.Cashes += Cash ? 1 : 0;
	L.FinalTables += Ft ? 1 : 0;
	L.Wins += Win ? 1 : 0;
	L.Best = std::max(L.Best, Prize);
	if (!Has(P.Kinds, KindUnderground))
	{
		// The stats page: every bullet, and a seat at its value.
		N.Stats.Add(Gross, Prize + (Seat ? P.SeatValue : 0), Place, P.Entries, P.FinalSize, StakeOf(P), FormatOf(P));
	}
	const double Points = PointsFor(P, Place);
	if (P.Online)
	{
		N.ThisSeason.Points += Points;
		N.ThisSeason.Won += Prize;
		N.ThisSeason.Wins += Win ? 1 : 0;
		N.ThisSeason.FinalTables += Ft ? 1 : 0;
		N.ThisSeason.Cashes += Cash ? 1 : 0;
		N.WeekPoints += static_cast<float>(Points);
	}
	else
	{
		N.ThisSeason.LivePoints += Points;
		N.ThisSeason.LiveWon += Prize;
	}
	Year& Y = Sim::YearRow(N, Day);
	(P.Online ? Y.Online : Y.Live) += Prize;
	Y.Net += Kept - Paid;
	Y.Wins += Win ? 1 : 0;
	++Y.Events;
	++N.WeekEvents;
	N.WeekKinds[0] += Has(P.Kinds, KindBounty) ? 1 : 0;
	N.WeekKinds[1] += Has(P.Kinds, KindSatellite) ? 1 : 0;
	N.WeekKinds[2] += P.Online ? 0 : 1;
	N.WeekKinds[3] += Has(P.Kinds, KindTurbo) || Has(P.Kinds, KindHyper) ? 1 : 0;
	N.Fatigue = sim::Clampf(static_cast<double>(N.Fatigue) + (P.Online ? 0.012 / Sim::Habit(N) : P.End - P.Start > 1440.0 ? 0.05 : 0.025), 0.0, 1.0);
	N.LastDay = std::max(N.LastDay, Day);
	// A deep run that ends just short stings (more for those who can't let it go).
	if (!Ft && Place > 0 && Place <= std::max(P.FinalSize * 3, P.Entries / 50))
	{
		N.Tilt = sim::Clampf(static_cast<double>(N.Tilt) + 0.12 * (1.0 - static_cast<double>(N.SkillOf(Skill::Emotion))), 0.0, 1.0);
	}
	// Fame: what people hear about.
	const double Dollars = sim::Dollars(Prize);
	const bool HighStakes = P.BuyIn >= 100000;
	if (Prize >= 1000000)
	{
		// Routine cashes don't make anyone famous: big scores and titles do.
		const double Gain = 2.0 * std::log10(Dollars / 10000.0 + 1.0) + (Win && P.Entries >= 300 ? 0.5 : 0.0);
		AddFame(N, P.Online ? Rep::Online : Rep::Live, P.Online ? Gain : Gain * 1.3);
	}
	if (P.Major && Ft)
	{
		AddFame(N, P.Online ? Rep::Online : Rep::Live, Win ? 10.0 : 3.0);
	}
	if (Win && P.Bracelet)
	{
		// An online bracelet is still a bracelet (the live ones travel further).
		AddFame(N, P.Online ? Rep::Online : Rep::Live, P.Online ? 8.0 : P.Kind == static_cast<int>(LiveKind::ChampionshipMain) ? 40.0 : 12.0);
	}
	if (Win && P.Ring)
	{
		AddFame(N, P.Online ? Rep::Online : Rep::Live, P.Major ? 8.0 : P.Online ? 3.0 : 4.0);
	}
	if (Win && !P.Series.empty() && P.Online)
	{
		AddFame(N, Rep::Online, 4.0);
	}
	if (HighStakes && Prize >= 2500000)
	{
		AddFame(N, Rep::HighStakes, std::log10(Dollars / 25000.0 + 1.0) * 3.0 + (Win ? 1.5 : 0.0));
	}
	if (P.Kind == static_cast<int>(LiveKind::Summit) && !P.Online)
	{
		AddFame(N, Rep::HighStakes, Win ? 25.0 : 4.0);
	}
	// Titles.
	if (Win)
	{
		N.Bracelets += P.Bracelet ? 1 : 0;
		N.Rings += P.Ring ? 1 : 0;
		if (P.Bracelet || P.Ring)
		{
			N.Awards.push_back(AwardOf(P, Day, Prize));
			N.Awards.back().Tourney = N.Stats.Events;
		}
		N.Titles += P.Online && !P.Series.empty() ? 1 : 0;
		N.Majors += P.Major ? 1 : 0;
	}
	// Their journey: the firsts, and the titles that last.
	{
		int Events = 0;
		int Cashes = 0;
		int Fts = 0;
		int Wins = 0;
		for (const Ledger& G : N.Totals)
		{
			Events += G.Events;
			Cashes += G.Cashes;
			Fts += G.FinalTables;
			Wins += G.Wins;
		}
		const int LiveEvents = N.Totals[static_cast<size_t>(Venue::Live)].Events + N.Totals[static_cast<size_t>(Venue::Underground)].Events;
		if (Events == 1)
		{
			Mark(N, Day, StepKind::FirstEvent, P.Name, Place, P.Entries, Prize);
		}
		if (!P.Online && LiveEvents == 1 && Events > 1)
		{
			Mark(N, Day, StepKind::FirstLive, P.Name, Place, P.Entries, Prize);
		}
		if (Cash && Cashes == 1 && Events > 1)
		{
			Mark(N, Day, StepKind::FirstCash, P.Name, Place, P.Entries, Prize);
		}
		if (Ft && Fts == 1 && !Win)
		{
			Mark(N, Day, StepKind::FirstFinalTable, P.Name, Place, P.Entries, Prize);
		}
		if (Win && Wins == 1)
		{
			Mark(N, Day, StepKind::FirstWin, P.Name, Place, P.Entries, Prize);
		}
		else if (Win && P.Bracelet)
		{
			Mark(N, Day, StepKind::Bracelet, P.Name, Place, P.Entries, Prize);
		}
		else if (Win && P.Ring)
		{
			Mark(N, Day, StepKind::Ring, P.Name, Place, P.Entries, Prize);
		}
		else if (Win && P.Major)
		{
			Mark(N, Day, StepKind::Major, P.Name, Place, P.Entries, Prize);
		}
		else if (Win && P.Online && !P.Series.empty() && N.Titles == 1)
		{
			Mark(N, Day, StepKind::FirstSeries, P.Name, Place, P.Entries, Prize);
		}
		else if (Prize > BestBefore && Prize >= 1000000 && Events > 1)
		{
			Mark(N, Day, StepKind::BigScore, P.Name, Place, P.Entries, Prize);
		}
	}
	// The record a card shows.
	if (Notable(P, Place, Prize, N.Tier))
	{
		Finish F;
		F.Day = Day;
		F.Event = P.Name;
		F.Place = Place;
		F.Entries = P.Entries;
		F.Prize = Prize;
		F.Where = P.Online ? 0 : 1;
		F.Major = P.Major;
		F.Seat = Seat ? P.SeatValue : 0;
		N.Recent.insert(N.Recent.begin(), F);
		if (N.Recent.size() > 10)
		{
			N.Recent.pop_back();
		}
	}
	if (Prize > BestBefore && Prize > 0)
	{
		N.BestEvent = P.Name;
		N.BestDay = Day;
	}
	// Moments: a breakout score, a title.
	const Chips Abi = std::max<Chips>(25, Sim::Comfort(N));
	if (Prize >= 500000 && Prize >= Abi * 150 && Prize >= BestBefore * 3)
	{
		Sim::SetMood(N, Momentum::Breakout, Day);
		N.Confidence = sim::Clampf(static_cast<double>(N.Confidence) + 0.4, -1.0, 1.0);
		Sim::Post(W, P.End, EventKind::Breakout, N.Id, P.Name, Prize, Place);
	}
	else if (Win && Prize >= Abi * 40 && N.Mood != Momentum::Breakout)
	{
		Sim::SetMood(N, Momentum::Confident, Day);
		N.Confidence = sim::Clampf(static_cast<double>(N.Confidence) + 0.25, -1.0, 1.0);
	}
	if (Win && L.Wins == 1 && (P.Major || Prize >= 1000000))
	{
		Sim::Post(W, P.End, EventKind::Milestone, N.Id, "first-title", Prize, Place);
	}
	Milestones(W, N, Before, N.CareerWon(), P.End);
	// The boards' running numbers.
	if (P.Online && Points > 0.0)
	{
		if (!P.Series.empty())
		{
			if (W.SeriesKey != P.Series)
			{
				W.SeriesKey = P.Series;
				W.SeriesPoints.clear();
			}
			W.SeriesPoints[N.Id] += Points;
		}
		if (P.Tier <= static_cast<int>(net::Tier::Micro))
		{
			const double Night = life::NightShiftStart(P.End);
			if (W.NightKey != Night)
			{
				W.NightKey = Night;
				W.NightPoints.clear();
				W.NightPointsHourAgo.clear();
			}
			if (P.End < Night + 12.0 * 60.0)
			{
				W.NightPoints[N.Id] += Points;
			}
		}
	}
	W.Refresh(N.Id);
}

void Sim::Resolve(World& W, Pending& P)
{
	Rng R(Seeded(W, "result", P.Id));
	const int Day = sim::DayAt(P.End);
	const bool HeroIn = P.HeroPlace > 0;

	// A back-room cash game: everyone wins or loses some, nobody "places".
	if (Has(P.Kinds, KindUnderground))
	{
		const double Hours = 3.0 + static_cast<double>(R.Int(6));
		for (const Entry& E : P.Who)
		{
			Npc& N = W.Roster[static_cast<size_t>(E.Npc)];
			const double Edge = sim::Clamp(Score(N, P.Kinds) - P.Strength, -MaxEdge, MaxEdge);
			const double Bb = R.Gauss(25.0 * Edge * Hours, 75.0 * std::sqrt(Hours));
			Chips Net = static_cast<Chips>(std::llround(Bb * static_cast<double>(P.Stakes)));
			Net = std::max(Net, -std::min(N.Bankroll, P.Stakes * 300));
			N.Bankroll = std::max<Chips>(0, N.Bankroll + Net);
			N.PeakRoll = std::max(N.PeakRoll, N.Bankroll);
			N.WeekNet += Net;
			W.Flow[Net > 0 ? 0 : 1] += sim::Dollars(Net > 0 ? Net : -Net);
			Ledger& L = N.Totals[static_cast<size_t>(Venue::Underground)];
			++L.Events;
			L.Won += std::max<Chips>(0, Net);
			L.Spent += std::max<Chips>(0, -Net);
			L.Cashes += Net > 0 ? 1 : 0;
			L.Best = std::max(L.Best, Net);
			Year& Y = YearRow(N, Day);
			Y.Net += Net;
			++Y.Events;
			++N.WeekEvents;
			++N.WeekKinds[2];
			AddFame(N, Rep::Underground, 0.25 + (Net > P.Stakes * 150 ? 0.6 : 0.0) + (P.Stakes >= 1000 ? 0.3 : 0.0));
			N.LastDay = std::max(N.LastDay, Day);
			N.Fatigue = sim::Clampf(static_cast<double>(N.Fatigue) + 0.02, 0.0, 1.0);
			W.Refresh(N.Id);
		}
		return;
	}

	// The Summit's field is exactly who accepted the invitation (nobody the world doesn't follow sits down).
	const bool Invitational = !P.Online && P.Kind == static_cast<int>(LiveKind::Summit);
	if (Invitational)
	{
		P.Entries = std::max(2, static_cast<int>(P.Who.size()) + (HeroIn ? 1 : 0));
		P.Pool = P.BuyIn * static_cast<Chips>(P.Entries);
		P.FinalSize = std::min(P.FinalSize, P.Entries);
	}
	// The field: the people the world follows draw their finishes; everyone else is the rest of the field.
	const int Field = std::max(P.Entries, static_cast<int>(P.Who.size()) + (HeroIn ? 1 : 0) + (Invitational ? 0 : 1));
	P.Entries = Field;
	const bool Bounty = Has(P.Kinds, KindBounty);
	const Chips PrizePool = Bounty ? P.Pool / 2 : P.Pool;
	const std::vector<Chips> Pay = Has(P.Kinds, KindSatellite) ? std::vector<Chips>() : PayoutTable(PrizePool, Field, std::max<Chips>(1, P.BuyIn * 3 / 2));
	std::set<int> Taken;
	if (HeroIn)
	{
		Taken.insert(P.HeroPlace);
	}
	std::vector<int> Place(P.Who.size(), 0);
	struct Draw
	{
		double Q;
		size_t I;
	};
	std::vector<Draw> Draws;
	for (size_t I = 0; I < P.Who.size(); ++I)
	{
		const Entry& E = P.Who[I];
		if (E.Known > 0 && Taken.count(E.Known) == 0 && E.Known <= Field)
		{
			Place[I] = E.Known;
			Taken.insert(E.Known);
			continue;
		}
		const Npc& N = W.Roster[static_cast<size_t>(E.Npc)];
		const double Edge = sim::Clamp(Score(N, P.Kinds) - P.Strength, -MaxEdge, MaxEdge);
		const double A = std::exp(-EdgeScale * P.Luck * Edge);
		double Q = 1.0;
		for (int B = 0; B < std::max(1, E.Bullets); ++B)
		{
			Q = std::min(Q, std::pow(R.Next(), 1.0 / A));
		}
		Draws.push_back({Q, I});
	}
	std::sort(Draws.begin(), Draws.end(), [](const Draw& A, const Draw& B) { return A.Q < B.Q || (A.Q == B.Q && A.I < B.I); });
	for (const Draw& Dr : Draws)
	{
		const Entry& E = P.Who[Dr.I];
		int Got = 0;
		if (E.Better > 1)
		{
			// Still in when the player went out: they finish ahead of the player.
			Got = FreePlace(Taken, 1 + static_cast<int>(Dr.Q * static_cast<double>(E.Better - 1)), 1, E.Better - 1);
		}
		if (Got == 0)
		{
			Got = FreePlace(Taken, 1 + static_cast<int>(Dr.Q * static_cast<double>(Field)), 1, Field);
		}
		Place[Dr.I] = Got;
		Taken.insert(Got);
	}

	// Money and records.
	std::map<int, int> AtPlace; // place -> npc
	for (size_t I = 0; I < P.Who.size(); ++I)
	{
		const Entry& E = P.Who[I];
		Npc& N = W.Roster[static_cast<size_t>(E.Npc)];
		const int Pl = Place[I];
		Chips Prize = Pl >= 1 && static_cast<size_t>(Pl - 1) < Pay.size() ? Pay[static_cast<size_t>(Pl - 1)] : 0;
		if (Bounty && Pl > 0)
		{
			Prize += Bounties(R, P, Pl, static_cast<double>(N.SkillOf(Skill::Bounty)));
		}
		const bool Seat = Has(P.Kinds, KindSatellite) && Pl >= 1 && Pl <= P.Seats;
		if (Seat && !P.Ticket.empty())
		{
			++N.Tickets[P.Ticket];
			const std::string Target = FinalTarget(P.Ticket);
			if (Target == "rcop-main" && P.Ticket == "rcop-main")
			{
				Post(W, P.End, EventKind::Qualified, N.Id, "RCOP Main Event", P.SeatValue, Pl);
			}
			else if (P.Ticket == "championship-main")
			{
				Post(W, P.End, EventKind::Qualified, N.Id, "Championship Main Event", P.SeatValue, Pl);
			}
		}
		Apply(W, N, P, E, Pl, Prize, Seat);
		if (Pl >= 1 && Pl <= P.FinalSize)
		{
			AtPlace[Pl] = E.Npc;
		}
	}

	// The final table, as the screens show it: the people the world follows, the player, and the rest.
	net::EventResult Res;
	const int Seats = std::min(P.FinalSize, Field);
	for (int Pl = 1; Pl <= Seats; ++Pl)
	{
		net::Placing Pc;
		Pc.Place = Pl;
		Pc.Prize = static_cast<size_t>(Pl - 1) < Pay.size() ? Pay[static_cast<size_t>(Pl - 1)] : 0;
		const auto Found = AtPlace.find(Pl);
		if (Found != AtPlace.end())
		{
			Pc.Player = Found->second;
		}
		else if (HeroIn && Pl == P.HeroPlace)
		{
			Pc.Player = -1;
			Pc.Prize = P.HeroPrize;
		}
		else
		{
			Pc.Player = -2;
			Anonymous(W, P, Pl, Pc.Name, Pc.Country);
		}
		Res.FinalTable.push_back(Pc);
	}

	// An unknown wins something big: the world starts following them (a few a week at most).
	if (!Res.FinalTable.empty() && Res.FinalTable.front().Player == -2)
	{
		// The biggest titles always make a name; other big scores now and then (the rest stay unknowns).
		const net::Placing& Wn = Res.FinalTable.front();
		const bool Always = (P.Kind == static_cast<int>(LiveKind::ChampionshipMain) && !P.Online) || (P.Online && P.Major && !P.Series.empty() && Wn.Prize >= 100000000);
		const bool Big = !P.Online && ((P.Bracelet && P.BuyIn >= 1000000) || (P.Ring && P.Major));
		if (Always || (Big && W.DiscoveredThisWeek < 2))
		{
			++W.DiscoveredThisWeek;
			Npc N = Discover(W, Wn.Name, Wn.Country, P.Strength, Day, Fnv1a(P.Id));
			const int Id = Add(W, std::move(N));
			Entry E;
			E.Npc = Id;
			// Big live fields are full of satellite qualifiers.
			E.Ticket = !P.Online && P.BuyIn >= 500000 && R.Chance(0.3);
			Npc& Found = W.Roster[static_cast<size_t>(Id)];
			Apply(W, Found, P, E, 1, Wn.Prize, false);
			Res.FinalTable.front().Player = Id;
			Res.FinalTable.front().Name.clear();
			Res.FinalTable.front().Country.clear();
			AtPlace[1] = Id;
			Post(W, P.End, EventKind::Discovered, Id, P.Name, Wn.Prize, P.Entries);
			W.Log.back().Flags |= FlagUnknown | (E.Ticket ? FlagQualifier : 0) | (P.Online ? 0 : FlagLive);
		}
	}

	// The story: who won, who made the final table of a major, who met whom.
	const int Winner = !Res.FinalTable.empty() ? Res.FinalTable.front().Player : -2;
	const int Second = Res.FinalTable.size() > 1 ? Res.FinalTable[1].Player : -2;
	if (Winner >= 0)
	{
		const Npc& N = W.Roster[static_cast<size_t>(Winner)];
		const Chips Prize = Res.FinalTable.front().Prize;
		const bool Report = P.Major || P.Bracelet || P.Ring || (!P.Series.empty() && P.Online) || Prize >= (P.Online ? 250000 : 1000000);
		if (Report)
		{
			Post(W, P.End, P.Major ? EventKind::Champion : EventKind::Won, Winner, P.Name, Prize, P.Entries, Second >= 0 ? Second : -1);
			int Flags = P.Online ? 0 : FlagLive;
			for (const Entry& E : P.Who)
			{
				if (E.Npc == Winner)
				{
					Flags |= E.Ticket ? FlagQualifier : 0;
					Flags |= E.Staked ? FlagStaked : 0;
				}
			}
			Flags |= HeroIn ? FlagHero : 0;
			Flags |= N.Totals[P.Online ? 0 : 1].Wins == 1 ? FlagFirst : 0;
			W.Log.back().Flags |= Flags;
		}
		// Titles that stay in history.
		const bool Lasting = P.Bracelet || (P.Kind == static_cast<int>(LiveKind::Summit) && !P.Online) || (P.Ring && P.Major) ||
			(P.Online && P.Major && !P.Series.empty());
		if (Lasting)
		{
			world::Honor H;
			H.Year = world::YearOf(Day);
			H.Title = P.Name;
			H.EventId = P.Id;
			H.Npc = Winner;
			H.Name = N.Name;
			H.Prize = Prize;
			H.Entries = P.Entries;
			H.At = P.End;
			W.Titles.push_back(H);
		}
	}
	else if (Winner == -2 && (P.Bracelet || Invitational || (P.Ring && P.Major) || (P.Online && P.Major && !P.Series.empty())))
	{
		// An unknown's title is still history.
		world::Honor H;
		H.Year = world::YearOf(Day);
		H.Title = P.Name;
		H.EventId = P.Id;
		H.Npc = -2;
		H.Name = Res.FinalTable.front().Name;
		H.Prize = Res.FinalTable.front().Prize;
		H.Entries = P.Entries;
		H.At = P.End;
		W.Titles.push_back(H);
	}
	else if (Winner == -1 && P.Major)
	{
		world::Honor H;
		H.Year = world::YearOf(Day);
		H.Title = P.Name;
		H.EventId = P.Id;
		H.Npc = -1;
		H.Name = W.HeroName;
		H.Prize = P.HeroPrize;
		H.Entries = P.Entries;
		H.At = P.End;
		W.Titles.push_back(H);
	}
	if (P.Major)
	{
		for (const net::Placing& Pc : Res.FinalTable)
		{
			if (Pc.Player >= 0 && Pc.Place > 1)
			{
				Post(W, P.End, EventKind::FinalTable, Pc.Player, P.Name, Pc.Prize, Pc.Place);
				W.Log.back().Flags |= P.Online ? 0 : FlagLive;
			}
		}
	}
	// Heads-up battles between people the world follows make rivalries.
	if (Winner >= 0 && Second >= 0 && (P.Major || P.BuyIn >= 10000 || P.Entries >= 300))
	{
		W.WeekPairs.push_back({std::min(Winner, Second), std::max(Winner, Second)});
	}
	if (P.Online || P.Major || P.Bracelet || P.Ring)
	{
		W.Results[P.Id] = std::move(Res);
		W.ResultEnds[P.Id] = P.End;
	}
}

// ------------------------------------------------------------------ the calendar's rhythm

void Sim::RefreshField(World& W)
{
	// The game gets tougher (or softer) as the poker world does: when the people the world follows improve,
	// so does everyone else they learned with. Who happens to sit at a level doesn't change the crowd there.
	double Sum = 0.0;
	int Count = 0;
	for (const Npc& N : W.Roster)
	{
		if (N.Playing())
		{
			Sum += static_cast<double>(N.Overall());
			++Count;
		}
	}
	const double Avg = Count > 0 ? Sum / static_cast<double>(Count) : W.FieldRef[0];
	for (size_t T = 1; T < 5; ++T)
	{
		W.Field[T] = W.FieldRef[T] - FieldSoftness + 0.6 * (Avg - W.FieldRef[0]);
	}
}

void Sim::Ranks(World& W)
{
	static const net::Board Boards[] = {net::Board::Earnings, net::Board::Season, net::Board::Wins, net::Board::FinalTables, net::Board::Series, net::Board::Live};
	const size_t N = W.Roster.size();
	for (net::Board B : Boards)
	{
		std::vector<double> V(N, 0.0);
		for (size_t I = 0; I < N; ++I)
		{
			V[I] = W.BoardValue(B, static_cast<int>(I), W.Now);
		}
		std::vector<int> Order(N);
		for (size_t I = 0; I < N; ++I)
		{
			Order[I] = static_cast<int>(I);
		}
		std::stable_sort(Order.begin(), Order.end(), [&](int A, int C) { return V[static_cast<size_t>(A)] > V[static_cast<size_t>(C)]; });
		std::vector<int>& Rk = W.RanksYesterday[static_cast<size_t>(B)];
		Rk.assign(N, 0);
		for (size_t K = 0; K < N; ++K)
		{
			Rk[static_cast<size_t>(Order[K])] = static_cast<int>(K) + 1;
		}
	}
}

void Sim::PlanTrips(World& W, int Day, Rng& R)
{
	struct Fest
	{
		std::string Series;
		int First = 1 << 30;
		int Last = -1;
		int MainDay = -1;
		Region Where = Region::Americas;
		LiveLevel Level = LiveLevel::Regional;
		Chips Main = 0;
		bool Championship = false;
	};
	std::map<std::string, Fest> Fests;
	// Six weeks ahead: long enough to see the Championship's Main Event from its first day.
	for (const LiveEvent& E : LiveFestivals(Day + 1, 45))
	{
		if (E.Kind == LiveKind::Summit)
		{
			continue;
		}
		Fest& F = Fests[E.Series];
		F.Series = E.Series;
		const int D = sim::DayAt(E.Start);
		F.First = std::min(F.First, D);
		F.Last = std::max(F.Last, D);
		F.Where = E.Where;
		F.Level = std::max(F.Level, E.Level == LiveLevel::HighRoller ? F.Level : E.Level);
		F.Championship = F.Championship || E.Kind == LiveKind::Championship || E.Kind == LiveKind::ChampionshipMain;
		if (E.Main)
		{
			F.Main = std::max(F.Main, E.BuyIn);
			F.MainDay = D;
		}
	}
	if (Fests.empty())
	{
		return;
	}
	for (Npc& N : W.Roster)
	{
		if (!N.Playing() || N.TripUntil >= Day || N.Live < LiveLevel::Local)
		{
			continue;
		}
		const int Cooldown = N.Schedule == Plan::LiveCircuit ? 6 : 24;
		if (Day - N.TripUntil < Cooldown)
		{
			continue;
		}
		Rng Q = R.Fork("trip/" + std::to_string(N.Id));
		const double Roll = static_cast<double>(N.Bankroll + N.Income * 26);
		const bool HasTicket = N.Tickets.count("championship-main") > 0;
		for (const auto& It : Fests)
		{
			const Fest& F = It.second;
			// Too late (or too far off to plan yet): the Championship can be joined until its Main Event.
			if (F.Championship ? (F.MainDay >= 0 ? F.MainDay <= Day + 1 : F.Last <= Day + 1) : (F.First <= Day || F.First > Day + 14))
			{
				continue;
			}
			bool Can = false;
			double Want = N.Schedule == Plan::LiveCircuit ? 0.75 : 0.12 + 0.5 * (1.0 - static_cast<double>(N.OnlineShare));
			if (F.Championship)
			{
				// The summer in Las Vegas: the one everybody wants to play. Those who can afford the Main Event go,
				// those who won a seat go, savers go once, and the well-known sell action to get there.
				const double Main = static_cast<double>(F.Main > 0 ? F.Main : 1000000);
				const bool Saved = !N.Professional && static_cast<double>(N.Income) * 104.0 >= Main;
				Can = HasTicket || (N.Live >= LiveLevel::Local && (Roll >= Main * 3.0 || Saved)) || (N.Live >= LiveLevel::Regional && N.RepOf(Rep::Overall) >= 30.0f);
				Want = HasTicket ? 1.0 : 0.15 + 0.35 * static_cast<double>(N.TraitOf(Trait::Ambition)) + (N.Professional && N.Live >= LiveLevel::Circuit ? 0.25 : 0.0);
			}
			else if (F.Level >= LiveLevel::Circuit)
			{
				Can = N.Live >= LiveLevel::Circuit || (N.Live >= LiveLevel::Regional && Roll >= 1700.0 * 100.0 * 40.0);
				Want *= F.Where == N.Home ? 1.0 : 0.3;
			}
			else
			{
				Can = (N.Live >= LiveLevel::Regional || F.Where == N.Home) && Roll >= 55000.0 * 20.0;
				Want *= F.Where == N.Home ? 1.0 : 0.15;
			}
			if (N.Anchored && N.From == Origin::Cast)
			{
				Want *= 0.5;
			}
			if (Can && Q.Chance(Want))
			{
				N.Trip = F.Series;
				if (F.Championship)
				{
					// Pros stay for weeks; most people fly in for the Main Event.
					const bool Long = N.Live >= LiveLevel::Circuit && N.Professional;
					const int Main = F.MainDay >= 0 ? F.MainDay : F.Last;
					N.TripFrom = std::max(Day + 1, Long ? F.First : std::max(F.First, Main - 3 - Q.Int(6)));
					N.TripUntil = std::max(Main + 2, N.TripFrom + 3);
				}
				else
				{
					N.TripFrom = F.First;
					N.TripUntil = F.Last;
				}
				const double Travel = F.Championship ? 2500.0 : F.Level >= LiveLevel::Circuit ? 1200.0 : 350.0;
				const Chips Cost = std::min(N.Bankroll, sim::Cents(Travel * (F.Where == N.Home ? 0.5 : 1.0)));
				N.Bankroll -= Cost;
				W.Flow[5] += sim::Dollars(Cost);
				break;
			}
		}
	}
}

void Sim::Finances(World& W, Npc& N, int Day, Rng& R)
{
	(void)W;
	(void)R;
	(void)Day;
	// Rent and groceries come out of a professional's roll; everyone else tops theirs up from a pay cheque.
	Chips In = 0;
	if (N.Professional)
	{
		const Chips Rent = std::min(N.Bankroll, N.Living);
		N.Bankroll -= Rent;
		W.Flow[3] += sim::Dollars(Rent);
	}
	if (N.Income > 0)
	{
		if (N.Professional)
		{
			In += N.Income; // a sponsor, a coaching gig
		}
		else
		{
			// Poker money from a pay cheque, topped up to about half a year's worth.
			In += std::min(N.Income, std::max<Chips>(0, N.Income * 26 - N.Bankroll));
		}
	}
	if (N.Streams && N.Followers > 0)
	{
		In += static_cast<Chips>(static_cast<double>(N.Followers) * 1.5); // subs, ads, a brand deal or two
	}
	if (N.Pro)
	{
		In += sim::Cents(1500.0);
	}
	N.Bankroll += In;
	W.Flow[2] += sim::Dollars(In);
	N.PeakRoll = std::max(N.PeakRoll, N.Bankroll);
}

void Sim::Stakes(World& W, Npc& N, int Day, Rng& R)
{
	const Chips C = Sim::Comfort(N);
	const int Fit = Sim::TierFor(C);
	const int Tier = sim::TierIndex(N.Tier);
	const double Ambition = static_cast<double>(N.TraitOf(Trait::Ambition));
	// Who hears about it: the people the world knows, and anyone reaching the top games.
	const bool Public = N.RepOf(Rep::Overall) >= 15.0f || N.Pro || N.Anchored || N.Streams;
	if (Fit > Tier && Day - N.TierSince >= 21 && N.Backer == -1)
	{
		const double Go = 0.15 + 0.6 * Ambition + (N.Mood == Momentum::Heater || N.Mood == Momentum::Breakout ? 0.2 : 0.0) - (N.Mood == Momentum::Downswing ? 0.3 : 0.0);
		if (R.Chance(sim::Clamp(Go, 0.02, 0.95)))
		{
			N.Tier = Tier + 1;
			N.TierSince = Day;
			if (Public || N.Tier == 4)
			{
				Sim::Post(W, static_cast<double>(Day) * 1440.0, EventKind::MovedUp, N.Id, net::TierName(static_cast<net::Tier>(N.Tier)), 0, N.Tier);
			}
			if (N.Professional)
			{
				N.Living = std::max(N.Living, sim::Cents(sim::Living[N.Tier] * R.Range(0.8, 1.2)));
			}
		}
	}
	else if (Tier > 1 && static_cast<double>(C) < static_cast<double>(sim::TierFloor[Tier]) * (0.35 + 0.4 * static_cast<double>(N.TraitOf(Trait::Discipline))))
	{
		// Below where the tier starts (the disciplined step down sooner; gamblers hang on).
		N.Tier = Tier - 1;
		N.TierSince = Day;
		N.PeakRoll = N.Bankroll + N.Bankroll / 8;
		Sim::SetMood(N, Momentum::Rebuilding, Day);
		if (Public)
		{
			Sim::Post(W, static_cast<double>(Day) * 1440.0, EventKind::MovedDown, N.Id, net::TierName(static_cast<net::Tier>(N.Tier)), 0, N.Tier);
		}
		if (N.Professional)
		{
			N.Living = std::max<Chips>(sim::Cents(120.0), N.Living * 3 / 4);
		}
	}
	// Live: what they can afford to travel for.
	if (N.OnlineShare < 0.99f && N.Live >= LiveLevel::Local)
	{
		const double Roll = sim::Dollars(N.Bankroll + N.Income * 26) * 100.0 / Sim::Need(N);
		const int L = static_cast<int>(N.Live);
		if (L < 5 && Roll >= sim::LiveNeed[L + 1] && R.Chance(0.1 + 0.3 * Ambition))
		{
			N.Live = static_cast<LiveLevel>(L + 1);
		}
		else if (L > 1 && Roll < sim::LiveNeed[L] * 0.4)
		{
			N.Live = static_cast<LiveLevel>(L - 1);
		}
	}
}

void Sim::Form(Npc& N, int Day)
{
	const double Abi = static_cast<double>(std::max<Chips>(25, Sim::Comfort(N)));
	N.MonthNet = N.MonthNet * 3 / 4 + N.WeekNet;
	// A downswing is measured from the recent peak (old peaks fade); for players who top up from a pay cheque,
	// it's the run of results that counts, not the roll.
	N.PeakRoll = std::max(N.Bankroll, N.Bankroll + (N.PeakRoll - N.Bankroll) * 95 / 100);
	const double Roll = N.PeakRoll > 0 ? 1.0 - static_cast<double>(N.Bankroll) / static_cast<double>(N.PeakRoll) : 0.0;
	const double Run = sim::Clamp(-static_cast<double>(N.MonthNet) / (150.0 * Abi), 0.0, 1.0);
	const double Draw = N.Professional ? Roll : Run;
	N.Confidence = sim::Clampf(0.8 * static_cast<double>(N.Confidence) + 0.25 * std::tanh(static_cast<double>(N.WeekNet) / (25.0 * Abi)), -1.0, 1.0);
	N.Tilt = sim::Clampf(static_cast<double>(N.Tilt) * 0.4, 0.0, 1.0);
	if (Draw > 0.3 && N.SkillOf(Skill::Emotion) < 0.45f)
	{
		N.Tilt = sim::Clampf(static_cast<double>(N.Tilt) + 0.15, 0.0, 1.0);
	}
	const int Since = Day - N.MoodSince;
	if (N.Fatigue > 0.85f)
	{
		Sim::SetMood(N, Momentum::Burnout, Day);
		return;
	}
	if (N.Mood == Momentum::Burnout && N.Fatigue > 0.4f)
	{
		return;
	}
	if (N.Mood == Momentum::Breakout && Since < 28)
	{
		return;
	}
	if (N.Mood == Momentum::Confident && Since < 14)
	{
		return;
	}
	if (N.Mood == Momentum::Rebuilding && Since < 28 && Draw > 0.05)
	{
		return;
	}
	if (Draw > 0.45)
	{
		Sim::SetMood(N, Momentum::MajorDownswing, Day);
	}
	else if (Draw > 0.25 || static_cast<double>(N.MonthNet) < -60.0 * Abi)
	{
		Sim::SetMood(N, Momentum::Downswing, Day);
	}
	else if (static_cast<double>(N.MonthNet) > 40.0 * Abi && Draw < 0.1)
	{
		Sim::SetMood(N, Momentum::Heater, Day);
	}
	else
	{
		Sim::SetMood(N, Momentum::Normal, Day);
	}
}

void Sim::Learn(World& W, Npc& N, int Day)
{
	const int Age = N.Age(Day);
	const double Practice = std::min(1.0, static_cast<double>(N.WeekEvents) / 15.0);
	double Study = 0.6 * static_cast<double>(N.TraitOf(Trait::Ambition)) + 0.2 * static_cast<double>(N.TraitOf(Trait::Discipline));
	for (const Tie& T : N.Ties)
	{
		Study += T.Kind == TieKind::TrainingPartner ? 0.2 * static_cast<double>(T.Strength) : 0.0;
	}
	const double Rate = 0.004 * static_cast<double>(N.Potential) * (0.3 + Practice) * (0.4 + Study) * (N.Playing() ? 1.0 : 0.2);
	const double Avg = static_cast<double>(N.Overall());
	for (size_t I = 0; I < N.Skills.size(); ++I)
	{
		double S = static_cast<double>(N.Skills[I]);
		const double Goal = static_cast<double>(N.Peak) + (S - Avg);
		if (Goal > S)
		{
			S += Rate * (Goal - S);
		}
		N.Skills[I] = sim::Clampf(S, 0.03, 0.99);
	}
	// What they play is what they get better at.
	auto Practise = [&](Skill S, int Events) {
		if (Events > 0)
		{
			N.Skills[static_cast<size_t>(S)] = sim::Clampf(static_cast<double>(N.Skills[static_cast<size_t>(S)]) + 0.0006 * std::min(1.0, static_cast<double>(Events) / 8.0) * static_cast<double>(N.Potential), 0.03, 0.99);
		}
	};
	Practise(Skill::Bounty, N.WeekKinds[0]);
	Practise(Skill::Satellite, N.WeekKinds[1]);
	Practise(Skill::Live, N.WeekKinds[2]);
	Practise(Skill::ShortStack, N.WeekKinds[3]);
	// Age: reads and composure keep improving; the rest slowly fades.
	if (Age > 50)
	{
		const double Fade = 0.00008 * static_cast<double>(Age - 50) / 10.0;
		for (size_t I = 0; I < N.Skills.size(); ++I)
		{
			if (I != static_cast<size_t>(Skill::Live) && I != static_cast<size_t>(Skill::Emotion))
			{
				N.Skills[I] = sim::Clampf(static_cast<double>(N.Skills[I]) - Fade, 0.03, 0.99);
			}
		}
	}
	if (Age > 45)
	{
		N.Peak = sim::Clampf(static_cast<double>(N.Peak) - 0.00006, 0.05, 0.99);
	}
	// Time away: a little rust.
	if (!N.Playing())
	{
		N.Skills[static_cast<size_t>(Skill::Adjust)] = sim::Clampf(static_cast<double>(N.SkillOf(Skill::Adjust)) - 0.0005, 0.03, 0.99);
		N.Skills[static_cast<size_t>(Skill::Postflop)] = sim::Clampf(static_cast<double>(N.SkillOf(Skill::Postflop)) - 0.0003, 0.03, 0.99);
	}
	(void)W;
}

void Sim::Broadcast(World& W, Npc& N, int Day, Rng& R)
{
	const double Social = static_cast<double>(N.TraitOf(Trait::Sociability));
	const double At = static_cast<double>(Day) * 1440.0;
	if (!N.Streams)
	{
		if (N.Playing() && Social > 0.65 && N.Age(Day) < 40)
		{
			const double Chance = 0.0006 * (Social - 0.6) * 10.0 * (N.Mood == Momentum::Breakout ? 6.0 : 1.0) * (N.RepOf(Rep::Overall) > 20.0f ? 2.0 : 1.0);
			if (R.Chance(Chance))
			{
				N.Streams = true;
				Sim::Post(W, At, EventKind::StartedStreaming, N.Id, "", 0, N.Followers);
			}
		}
		else if (N.Followers > 0)
		{
			N.Followers = N.Followers * 99 / 100;
		}
		return;
	}
	if (!N.Playing())
	{
		N.Followers = N.Followers * 99 / 100;
		return;
	}
	// A community takes time: hours on air, personality, results, and how far along they already are.
	static const double Hours[static_cast<int>(Plan::Count)] = {22.0, 12.0, 8.0, 3.0, 6.0, 4.0, 5.0};
	const double H = Hours[static_cast<int>(N.Schedule)] * (N.Mood == Momentum::Burnout ? 0.2 : 1.0);
	const double Cap = 20000.0 + 600000.0 * std::pow(Social, 3.0);
	const double F = static_cast<double>(N.Followers);
	const double Pull = (0.5 + Social) * (1.0 + static_cast<double>(N.RepOf(Rep::Overall)) / 25.0) * (N.Mood == Momentum::Heater || N.Mood == Momentum::Breakout ? 1.5 : 1.0);
	double Grow = H * 6.0 * Pull * (1.0 - F / Cap) + F * 0.004 * (1.0 - F / Cap) * Pull;
	Grow *= R.Range(0.5, 1.5);
	const int Before = N.Followers;
	N.Followers = std::max(0, N.Followers + static_cast<int>(std::lround(Grow)));
	static const int Marks[] = {1000, 10000, 50000, 100000, 250000, 500000};
	for (int M : Marks)
	{
		if (Before < M && N.Followers >= M)
		{
			Sim::Post(W, At, EventKind::StreamMilestone, N.Id, "followers", 0, M);
		}
	}
	// Brands find the bigger channels.
	if (N.Sponsor.empty() && N.Followers >= 25000 && R.Chance(0.04))
	{
		const std::vector<kast::Sponsor>& Brands = kast::Sponsors();
		if (!Brands.empty())
		{
			N.Sponsor = Brands[static_cast<size_t>(R.Int(static_cast<int>(Brands.size())))].Brand;
			Sim::Post(W, At, EventKind::Sponsored, N.Id, N.Sponsor, 0, N.Followers);
		}
	}
	// Some stop: no growth, bad results, burnout.
	if (N.Followers < 2000 && Day - N.Joined > 60 && R.Chance(0.012 + (N.Mood == Momentum::Burnout ? 0.03 : 0.0)))
	{
		N.Streams = false;
	}
}

void Sim::Lifecycle(World& W, Npc& N, int Day, Rng& R)
{
	const double At = static_cast<double>(Day) * 1440.0;
	const int Age = N.Age(Day);
	const double Grit = static_cast<double>(N.TraitOf(Trait::Grit));
	const bool Known = N.RepOf(Rep::Overall) >= 12.0f || N.Anchored;
	switch (N.St)
	{
	case Status::Active:
	{
		// Out of money?
		const Chips Floor = N.Professional ? std::max<Chips>(sim::Cents(40.0), N.Living * 2) : sim::Cents(25.0);
		if (N.Bankroll < Floor && N.Backer == -1)
		{
			if (N.Anchored)
			{
				// The people the story needs find the money somewhere (a shift, a loan, a sale).
				N.Bankroll += std::max(N.Income * 4, sim::Cents(300.0));
				Sim::SetMood(N, Momentum::Rebuilding, Day);
				break;
			}
			const double Sk = static_cast<double>(N.Overall());
			if (N.Professional && Sk >= 0.55 && R.Chance(0.35 + static_cast<double>(N.RepOf(Rep::Overall)) / 100.0))
			{
				// A stake: someone who knows them, or a staking stable.
				int Backer = -2;
				for (const Tie& T : N.Ties)
				{
					const Npc& O = W.Roster[static_cast<size_t>(T.Other)];
					const std::ptrdiff_t Backing = std::count_if(O.Ties.begin(), O.Ties.end(), [](const Tie& X) { return X.Kind == TieKind::Backer; });
					if ((T.Kind == TieKind::Friend || T.Kind == TieKind::TrainingPartner) && O.Playing() && O.Bankroll > sim::TierAbi[sim::TierIndex(N.Tier)] * 400 && Backing < 3)
					{
						Backer = T.Other;
						break;
					}
				}
				N.Backer = Backer;
				N.BackerShare = 0.5f;
				N.Makeup = 0;
				N.BackedSince = Day;
				N.Bankroll = std::max(N.Bankroll, N.Living * 4); // a little to live on while it runs
				Sim::Post(W, At, EventKind::Backed, N.Id, Backer >= 0 ? W.Roster[static_cast<size_t>(Backer)].Name : "", 0, 0, Backer >= 0 ? Backer : -1);
				if (Backer >= 0)
				{
					Bind(W, Backer, N.Id, TieKind::Backer, 0.6f, Day);
					Bind(W, N.Id, Backer, TieKind::Backed, 0.6f, Day);
				}
				Sim::SetMood(N, Momentum::Rebuilding, Day);
				break;
			}
			N.St = Status::Broke;
			N.Left = Day;
			N.WeeksBroke = 0;
			if (Known || N.Professional)
			{
				Sim::Post(W, At, EventKind::WentBroke, N.Id, "", 0, N.Tier);
			}
			if (N.Professional && R.Chance(0.6))
			{
				// Back to a job: poker becomes the thing they do after work.
				N.Professional = false;
				N.Income = sim::Cents(R.Range(20.0, 80.0));
				N.Living = 0;
			}
			break;
		}
		// Retirement: age, burnout, long losing, or simply enough.
		// Recreational players come and go; professionals stay longer.
		double Quit = (N.Professional ? 0.0008 : 0.0022) + std::max(0, Age - 50) * 0.0004;
		Quit += N.Mood == Momentum::Burnout ? 0.004 * (1.0 - Grit) : 0.0;
		Quit += N.Mood == Momentum::MajorDownswing ? 0.002 * (1.0 - Grit) : 0.0;
		Quit += N.Bankroll > sim::Cents(5.0e6) && Age > 45 ? 0.002 : 0.0;
		Quit += !N.Professional && N.Overall() < 0.35f && N.MonthNet < 0 ? 0.001 : 0.0;
		if (!N.Anchored && R.Chance(Quit))
		{
			N.St = Status::Retired;
			N.Left = Day;
			N.TripUntil = -1;
			if (Known)
			{
				Sim::Post(W, At, EventKind::Retired, N.Id, "", N.CareerWon(), Age);
			}
			break;
		}
		// A break: burnout, life, a bad run.
		double Away = N.Professional ? 0.003 : 0.006;
		Away += N.Mood == Momentum::Burnout ? 0.15 : 0.0;
		Away += N.Mood == Momentum::MajorDownswing ? 0.03 * (1.0 - Grit) : 0.0;
		if (N.TripUntil < Day && R.Chance(Away))
		{
			N.St = Status::Break;
			N.Left = Day;
			N.Until = Day + 14 + R.Int(N.Mood == Momentum::Burnout ? 120 : 60);
			if (Known)
			{
				Sim::Post(W, At, EventKind::Break, N.Id, "", 0, N.Until - Day);
			}
		}
		break;
	}
	case Status::Broke:
	{
		++N.WeeksBroke;
		if (N.Income > 0)
		{
			N.Bankroll += N.Income / 2;
		}
		if (N.Bankroll >= sim::Cents(60.0) && R.Chance(0.5))
		{
			N.St = Status::Active;
			N.Tier = std::min(N.Tier, Sim::TierFor(Sim::Comfort(N)));
			N.TierSince = Day;
			N.PeakRoll = N.Bankroll;
			Sim::SetMood(N, Momentum::Rebuilding, Day);
			if (Known)
			{
				Sim::Post(W, At, EventKind::Returned, N.Id, "broke", 0, 0);
			}
		}
		else if (N.WeeksBroke > 10 && R.Chance(0.15))
		{
			N.St = Status::Retired;
			N.Left = Day;
		}
		break;
	}
	case Status::Retired:
	{
		// Some come back: for one more run, or because they miss it.
		const double Back = Age < 60 ? 0.0012 + 0.002 * static_cast<double>(N.TraitOf(Trait::Ambition)) : 0.0003;
		if (!N.Faded && Day - N.Left > 90 && R.Chance(Back))
		{
			N.St = Status::Active;
			N.Bankroll = std::max(N.Bankroll, std::max(N.Income * 20, sim::Cents(1500.0)));
			N.PeakRoll = N.Bankroll;
			N.Tier = std::min(N.Tier, Sim::TierFor(Sim::Comfort(N)));
			N.TierSince = Day;
			N.Fatigue = 0.0f;
			Sim::SetMood(N, Momentum::Normal, Day);
			if (Known || N.CareerWon() >= sim::Cents(250000.0))
			{
				Sim::Post(W, At, EventKind::Returned, N.Id, "retired", 0, Day - N.Left);
			}
		}
		break;
	}
	case Status::Break: break;
	}
	// Staking: deals run about six months. Clear of makeup by then, they play on their own money; deep in it,
	// the backer walks away (the makeup goes with them).
	if (N.Backer != -1)
	{
		const Chips Abi = sim::TierAbi[sim::TierIndex(N.Tier)];
		const bool Over = Day - N.BackedSince >= 182;
		const bool Free = N.Makeup == 0 && (N.Bankroll >= Abi * 80 || Over);
		const bool Dropped = N.Makeup > Abi * 400 || (Over && N.Makeup > 0) || (N.Backer >= 0 && !W.Roster[static_cast<size_t>(N.Backer)].Playing());
		if (Free || Dropped)
		{
			for (Tie& T : N.Ties)
			{
				if (T.Kind == TieKind::Backed)
				{
					T.Kind = Free ? TieKind::Friend : TieKind::Rival;
					T.Strength = Free ? 0.5f : 0.2f;
				}
			}
			if (N.Backer >= 0)
			{
				for (Tie& T : W.Roster[static_cast<size_t>(N.Backer)].Ties)
				{
					if (T.Other == N.Id && T.Kind == TieKind::Backer)
					{
						T.Kind = TieKind::Friend;
					}
				}
			}
			N.Backer = -1;
			N.Makeup = 0;
		}
	}
}

void Sim::Bind(World& W, int A, int B, TieKind K, float Strength, int Day)
{
	{
		Npc& N = W.Roster[static_cast<size_t>(A)];
		for (Tie& T : N.Ties)
		{
			if (T.Other == B)
			{
				if (K == TieKind::Rival && T.Kind != TieKind::Backer && T.Kind != TieKind::Backed)
				{
					const float Old = T.Strength;
					T.Kind = TieKind::Rival;
					T.Strength = std::min(1.0f, T.Strength + Strength);
					if (Old < 0.6f && T.Strength >= 0.6f && A < B)
					{
						Sim::Post(W, static_cast<double>(Day) * 1440.0, EventKind::Rivalry, A, "", 0, 0, B);
					}
				}
				else
				{
					T.Strength = std::min(1.0f, T.Strength + Strength * 0.5f);
				}
				return;
			}
		}
		Tie T;
		T.Other = B;
		T.Kind = K;
		T.Strength = Strength;
		T.Since = Day;
		N.Ties.push_back(T);
		// A handful each: the weakest goes (a stake is never dropped this way while it runs).
		while (N.Ties.size() > 6)
		{
			auto Weakest = std::min_element(N.Ties.begin(), N.Ties.end(), [](const Tie& X, const Tie& Y) {
				const bool XKeep = X.Kind == TieKind::Backer || X.Kind == TieKind::Backed;
				const bool YKeep = Y.Kind == TieKind::Backer || Y.Kind == TieKind::Backed;
				return XKeep != YKeep ? YKeep : X.Strength < Y.Strength;
			});
			if (Weakest->Kind == TieKind::Backer || Weakest->Kind == TieKind::Backed)
			{
				break;
			}
			N.Ties.erase(Weakest);
		}
	}
}

void Sim::Connect(World& W, int Day, Rng& R)
{
	auto AddTie = [&](int A, int B, TieKind K, float Strength) { Bind(W, A, B, K, Strength, Day); };
	// Heads-up battles: rivals.
	for (const std::pair<int, int>& P : W.WeekPairs)
	{
		AddTie(P.first, P.second, TieKind::Rival, 0.3f);
		AddTie(P.second, P.first, TieKind::Rival, 0.3f);
	}
	W.WeekPairs.clear();
	// The social ones find training partners, travel partners, stream collabs.
	const int N = static_cast<int>(W.Roster.size());
	for (int I = 0; I < N; ++I)
	{
		const Npc& A = W.Roster[static_cast<size_t>(I)];
		if (!A.Playing() || A.TraitOf(Trait::Sociability) < 0.55f || !R.Chance(0.012))
		{
			continue;
		}
		// Someone at their level, from their part of the world.
		for (int Try = 0; Try < 12; ++Try)
		{
			const int J = R.Int(N);
			const Npc& B = W.Roster[static_cast<size_t>(J)];
			if (J == I || !B.Playing() || B.Home != A.Home || std::abs(B.Tier - A.Tier) > 1)
			{
				continue;
			}
			TieKind K = TieKind::Friend;
			if (A.Streams && B.Streams)
			{
				K = TieKind::StreamCollab;
			}
			else if (A.Live >= LiveLevel::Regional && B.Live >= LiveLevel::Regional && A.OnlineShare < 0.9f)
			{
				K = TieKind::TravelPartner;
			}
			else if (A.Professional && B.Professional && A.TraitOf(Trait::Ambition) > 0.5f)
			{
				K = TieKind::TrainingPartner;
			}
			AddTie(I, J, K, 0.35f);
			AddTie(J, I, K, 0.35f);
			break;
		}
	}
}

void Sim::Newcomers(World& W, int Day, Rng& R)
{
	const double Years = std::max(0.0, (static_cast<double>(Day) * 1440.0 - W.StartedAt()) / (365.0 * 1440.0));
	W.Target = static_cast<int>(static_cast<double>(sim::FoundingPopulation) * (1.0 + 0.015 * Years));
	const int Active = W.ActiveCount();
	const int Want = std::max(0, std::min(40, static_cast<int>(std::lround(static_cast<double>(W.Target - Active) * 0.3 + R.Gauss(3.0, 1.5)))));
	for (int K = 0; K < Want; ++K)
	{
		Npc N = Sim::Rookie(W, R, Day);
		const bool Talent = N.Potential > 0.75f || N.Overall() > 0.55f;
		const bool Friend = N.Came == Arrival::HomeGame && N.CameWith >= 0;
		const int With = N.CameWith;
		const int Id = Sim::Add(W, std::move(N));
		if (Friend)
		{
			// The friend who talked them into it: they know each other from the home game.
			Sim::Bind(W, Id, With, TieKind::HomeGame, 0.5f, Day);
			Sim::Bind(W, With, Id, TieKind::HomeGame, 0.5f, Day);
		}
		if (Talent)
		{
			Sim::Post(W, static_cast<double>(Day) * 1440.0, EventKind::Debut, Id, "", 0, 0);
		}
	}
}

void Sim::TeamRiverLine(World& W, int Day)
{
	int Pros = 0;
	for (Npc& N : W.Roster)
	{
		if (N.Pro && (!N.Playing() || N.RepOf(Rep::Overall) < 15.0f) && Day - N.Left > 60)
		{
			if (!N.Playing())
			{
				N.Pro = false;
				N.Sponsor.clear();
				N.Income = std::max<Chips>(0, N.Income - sim::Cents(1500.0));
			}
		}
		Pros += N.Pro ? 1 : 0;
	}
	// The site signs the best-known professional it doesn't already have.
	while (Pros < 10)
	{
		int Best = -1;
		for (const Npc& N : W.Roster)
		{
			if (!N.Pro && N.Playing() && N.Professional && N.Sponsor.empty() && N.RepOf(Rep::Overall) >= 35.0f &&
				(Best < 0 || N.RepOf(Rep::Overall) > W.Roster[static_cast<size_t>(Best)].RepOf(Rep::Overall)))
			{
				Best = N.Id;
			}
		}
		if (Best < 0)
		{
			break;
		}
		Npc& N = W.Roster[static_cast<size_t>(Best)];
		N.Pro = true;
		N.Sponsor = "Team RiverLine";
		N.Income += sim::Cents(1500.0);
		++Pros;
		Sim::Post(W, static_cast<double>(Day) * 1440.0, EventKind::Sponsored, N.Id, "Team RiverLine", 0, 0);
		W.Refresh(N.Id);
	}
}

namespace worldsim_detail
{
Identity Label(const Npc& N, int Day)
{
	const int Age = N.Age(Day);
	const double Sk = static_cast<double>(N.Overall());
	if (N.Streams && N.Followers >= 5000)
	{
		return Identity::Streamer;
	}
	if (N.Tier >= 4 && N.TraitOf(Trait::Risk) > 0.65f)
	{
		return Identity::HighStakesGambler;
	}
	if (!N.Professional && N.Income >= sim::Cents(1500.0))
	{
		return Identity::WealthyAmateur;
	}
	if (N.RepOf(Rep::Underground) >= 25.0f && N.OnlineShare < 0.7f)
	{
		return Identity::UndergroundSpecialist;
	}
	if (Age < 25 && Sk > 0.66)
	{
		return Identity::YoungCrusher;
	}
	if (Age < 30 && static_cast<double>(N.Peak) - Sk > 0.12 && Sk > 0.55 && N.Potential > 0.6f)
	{
		return Identity::RisingProspect;
	}
	if (Age >= 45 && N.Professional && Day - N.Joined > 365 * 8)
	{
		return Identity::Veteran;
	}
	if ((N.Formats & LikeSatellite) && N.SkillOf(Skill::Satellite) - N.Overall() > 0.1f && Sk > 0.5)
	{
		return Identity::SatelliteSpecialist;
	}
	if ((N.Formats & LikeBounty) && N.SkillOf(Skill::Bounty) - N.Overall() > 0.1f && Sk > 0.5)
	{
		return Identity::BountySpecialist;
	}
	if (N.Professional && N.TraitOf(Trait::Discipline) > 0.8f)
	{
		return N.TraitOf(Trait::Risk) < 0.2f ? Identity::BankrollNit : Identity::DisciplinedPro;
	}
	if (N.OnlineShare < 0.6f)
	{
		return Identity::LiveRegular;
	}
	if (!N.Professional)
	{
		return Identity::Recreational;
	}
	return Identity::OnlineGrinder;
}

Chips CashOut(Npc& N)
{
	// Winnings past what their stakes (and ambitions) need leave poker: a house, a car, savings.
	const int Tier = sim::TierIndex(N.Tier);
	const double Ambition = static_cast<double>(N.TraitOf(Trait::Ambition));
	const double Next = Tier < 4 ? static_cast<double>(sim::TierAbi[Tier + 1]) * 1.3 : static_cast<double>(sim::TierAbi[4]) * 4.0;
	double Ceiling = Sim::Need(N) * Next * (0.8 + Ambition);
	if (!N.Professional)
	{
		Ceiling = std::max(static_cast<double>(N.Income) * 40.0, Ceiling * 0.5);
	}
	if (N.Live >= LiveLevel::HighRoller)
	{
		Ceiling = std::max(Ceiling, 1.0e8 * 3.0);
	}
	if (static_cast<double>(N.Bankroll) > Ceiling)
	{
		const double Take = (static_cast<double>(N.Bankroll) - Ceiling) * (0.3 + 0.4 * (1.0 - Ambition));
		const double Keep = static_cast<double>(N.Bankroll) - Take;
		if (N.Bankroll > 0)
		{
			N.PeakRoll = static_cast<Chips>(static_cast<double>(N.PeakRoll) * Keep / static_cast<double>(N.Bankroll));
		}
		const Chips Before = N.Bankroll;
		N.Bankroll = static_cast<Chips>(Keep);
		return Before - N.Bankroll;
	}
	return 0;
}

void ExpireTickets(Npc& N, double Now)
{
	const net::Network& Net = net::Shared();
	for (auto It = N.Tickets.begin(); It != N.Tickets.end();)
	{
		net::EventInstance Next;
		const std::string Target = FinalTarget(It->first);
		if (It->first != "championship-main" && !Net.NextFor(It->first, Now, Next) && !Net.NextFor(Target, Now, Next))
		{
			// The event is gone: the site credits the seat's value.
			N.Bankroll += SeatValueOf(It->first) * It->second;
			It = N.Tickets.erase(It);
		}
		else
		{
			++It;
		}
	}
}
} // namespace worldsim_detail

void Sim::TurnPro(World& W, Npc& N, int Day, Rng& R)
{
	// Winning players with a roll that covers their stakes and half a year's rent quit the day job.
	if (N.Professional || N.Age(Day) >= 45 || N.Overall() < 0.53f || N.TraitOf(Trait::Ambition) < 0.45f || N.Tier < 2 || N.Years.empty() || N.Years.back().Net <= 0)
	{
		return;
	}
	const int Tier = sim::TierIndex(N.Tier);
	const double Rent = sim::Living[Tier] * 26.0 * 100.0;
	if (static_cast<double>(N.Bankroll) < Need(N) * static_cast<double>(sim::TierFloor[Tier]) + Rent || !R.Chance(0.15 * static_cast<double>(N.TraitOf(Trait::Ambition))))
	{
		return;
	}
	N.Professional = true;
	N.Income = N.Pro ? sim::Cents(1500.0) : 0;
	N.Living = sim::Cents(sim::Living[Tier] * R.Range(0.8, 1.3));
	Post(W, static_cast<double>(Day) * 1440.0, EventKind::Milestone, N.Id, "turned-pro", N.Bankroll, Tier);
}

void Sim::NewDay(World& W, int Day)
{
	const int Yr = world::YearOf(Day);
	if (Yr != world::YearOf(Day - 1))
	{
		NewYear(W, Yr, Day);
	}
	const int Month = sim::MonthKey(Day);
	if (Month != W.Month)
	{
		W.Month = Month;
		NewMonth(W, Day);
	}
	if (net::Weekday(Day) == 0 && Day != W.Week)
	{
		W.Week = Day;
		NewWeek(W, Day);
	}
	for (Npc& N : W.Roster)
	{
		// Rest: a day off does more than a normal one; a long downswing wears on people too.
		double Rest = N.LastDay < Day - 1 ? 0.05 : 0.012;
		if (N.Mood == Momentum::MajorDownswing)
		{
			Rest -= 0.006 * (1.0 - static_cast<double>(N.TraitOf(Trait::Grit)));
		}
		N.Fatigue = sim::Clampf(static_cast<double>(N.Fatigue) - Rest, 0.0, 1.0);
		if (N.St == Status::Break && N.Until <= Day)
		{
			N.St = Status::Active;
			N.Fatigue = 0.0f;
			if (N.Mood == Momentum::Burnout)
			{
				SetMood(N, Momentum::Normal, Day);
			}
			if (N.RepOf(Rep::Overall) >= 12.0f || N.Anchored)
			{
				Post(W, static_cast<double>(Day) * 1440.0, EventKind::Returned, N.Id, "break", 0, Day - N.Left);
			}
		}
		if (!N.Trip.empty() && N.TripUntil < Day)
		{
			N.Trip.clear();
		}
	}
	// Results the screens can still ask for: a week of everything, a year of the big ones.
	const double Now = static_cast<double>(Day) * 1440.0;
	for (auto It = W.ResultEnds.begin(); It != W.ResultEnds.end();)
	{
		const double Age = Now - It->second;
		bool Keep = Age < sim::KeepResults;
		if (!Keep && Age < sim::KeepMajors)
		{
			const net::EventResult& R = W.Results[It->first];
			Keep = !R.FinalTable.empty() && R.FinalTable.front().Prize >= 2500000;
		}
		if (!Keep)
		{
			W.Results.erase(It->first);
			It = W.ResultEnds.erase(It);
		}
		else
		{
			++It;
		}
	}
	Ranks(W);
}

void Sim::NewWeek(World& W, int Day)
{
	Rng R(Seeded(W, "week", std::to_string(Day)));
	for (Npc& N : W.Roster)
	{
		Rng Q = R.Fork(std::to_string(N.Id));
		// The week's form on the boards.
		for (size_t K = 0; K + 1 < N.Weekly.size(); ++K)
		{
			N.Weekly[K] = N.Weekly[K + 1];
		}
		N.Weekly.back() = N.WeekPoints;
		N.WeekPoints = 0.0f;
		if (N.Playing())
		{
			Finances(W, N, Day, Q);
			Form(N, Day);
			Stakes(W, N, Day, Q);
		}
		Learn(W, N, Day);
		// Fame fades without new results.
		for (size_t K = 0; K < N.Fame.size(); ++K)
		{
			const double Keep = K == static_cast<size_t>(Rep::Underground) ? 0.98 : 0.988;
			N.Fame[K] = static_cast<float>(static_cast<double>(N.Fame[K]) * Keep);
		}
		if (N.Tier >= 4 && N.Playing())
		{
			AddFame(N, Rep::HighStakes, 0.15);
		}
		Broadcast(W, N, Day, Q);
		Lifecycle(W, N, Day, Q);
		Reputation(N);
		N.WeekNet = 0;
		N.WeekEvents = 0;
		N.WeekKinds = {};
	}
	Connect(W, Day, R);
	PlanTrips(W, Day, R);
	Newcomers(W, Day, R);
	RefreshField(W);
	W.DiscoveredThisWeek = 0;
	W.RefreshAll();
}

void Sim::NewMonth(World& W, int Day)
{
	Rng R(Seeded(W, "month", std::to_string(Day)));
	for (Npc& N : W.Roster)
	{
		if (N.Playing())
		{
			W.Flow[4] += sim::Dollars(CashOut(N));
			TurnPro(W, N, Day, R);
		}
		N.Is = Label(N, Day);
		ExpireTickets(N, static_cast<double>(Day) * 1440.0);
		for (Tie& T : N.Ties)
		{
			if (T.Kind != TieKind::Backer && T.Kind != TieKind::Backed)
			{
				T.Strength *= 0.95f;
			}
		}
		N.Ties.erase(std::remove_if(N.Ties.begin(), N.Ties.end(), [](const Tie& T) { return T.Strength < 0.05f; }), N.Ties.end());
	}
	TeamRiverLine(W, Day);
	Fade(W);
}

void Sim::Fade(World& W, Npc& N)
{
	Npc G;
	G.Id = N.Id;
	G.Name = N.Name;
	G.Country = N.Country;
	G.Hue = N.Hue;
	G.Born = N.Born;
	G.From = N.From;
	G.Home = N.Home;
	G.St = Status::Retired;
	G.Faded = true;
	G.Left = N.Left;
	G.Joined = N.Joined;
	G.LastDay = N.LastDay;
	G.Tier = N.Tier;
	G.Is = N.Is;
	G.Began = N.Began;
	G.Skills.fill(N.Overall());
	for (size_t V = 0; V < G.Totals.size(); ++V)
	{
		G.Totals[V].Won = N.Totals[V].Won;
		G.Totals[V].Events = N.Totals[V].Events;
		G.Totals[V].Cashes = N.Totals[V].Cashes;
		G.Totals[V].FinalTables = N.Totals[V].FinalTables;
		G.Totals[V].Wins = N.Totals[V].Wins;
		G.Totals[V].Best = N.Totals[V].Best;
	}
	G.BestEvent = N.BestEvent;
	G.BestDay = N.BestDay;
	G.ThisSeason.Year = N.ThisSeason.Year;
	// Nobody keeps a tie to someone who's gone.
	for (const Tie& T : N.Ties)
	{
		if (Npc* O = T.Other >= 0 && static_cast<size_t>(T.Other) < W.Roster.size() ? &W.Roster[static_cast<size_t>(T.Other)] : nullptr)
		{
			O->Ties.erase(std::remove_if(O->Ties.begin(), O->Ties.end(), [&](const Tie& X) { return X.Other == N.Id; }), O->Ties.end());
		}
	}
	N = std::move(G);
}

void Sim::NewYear(World& W, int Calendar, int Day)
{
	const int Last = Calendar - 1;
	const double At = static_cast<double>(Day) * 1440.0 - 1.0;
	// Players of the Year: the most points online and live (the player included).
	int Best = -2;
	double Top = 0.0;
	int BestLive = -2;
	double TopLive = 0.0;
	for (const Npc& N : W.Roster)
	{
		if (N.ThisSeason.Year == Last && N.ThisSeason.Points > Top)
		{
			Top = N.ThisSeason.Points;
			Best = N.Id;
		}
		if (N.ThisSeason.Year == Last && N.ThisSeason.LivePoints > TopLive)
		{
			TopLive = N.ThisSeason.LivePoints;
			BestLive = N.Id;
		}
	}
	if (W.HeroPoints > Top && W.HeroPoints > 0.0)
	{
		Best = -1;
		Top = W.HeroPoints;
	}
	if (W.HeroLivePoints > TopLive && W.HeroLivePoints > 0.0)
	{
		BestLive = -1;
		TopLive = W.HeroLivePoints;
	}
	auto Crown = [&](int Who, const char* Title, double Points) {
		if (Who == -2)
		{
			return;
		}
		world::Honor H;
		H.Year = Last;
		H.Title = Title;
		H.Npc = Who;
		H.Name = Who >= 0 ? W.Roster[static_cast<size_t>(Who)].Name : W.HeroName;
		H.Entries = static_cast<int>(Points);
		H.At = At;
		W.Titles.push_back(H);
		if (Who >= 0)
		{
			Post(W, At, EventKind::PlayerOfYear, Who, Title, 0, static_cast<int>(Points));
			AddFame(W.Roster[static_cast<size_t>(Who)], Rep::Online, 12.0);
		}
	};
	Crown(Best, "Player of the Year", Top);
	Crown(BestLive, "Live Player of the Year", TopLive);
	for (Npc& N : W.Roster)
	{
		N.ThisSeason = Season();
		N.ThisSeason.Year = Calendar;
		// Long retired, nothing much to remember them by: keep the name and the record, let the rest go.
		const bool Remembered = N.Anchored || W.HeroBonds.count(N.Id) > 0 || N.CareerWon() >= sim::Cents(250000.0) || N.Bracelets + N.Rings + N.Majors + N.Titles > 0;
		if (N.St == Status::Retired && !N.Faded && Day - N.Left > 365 && !Remembered)
		{
			Fade(W, N);
		}
		W.Refresh(N.Id);
	}
	W.HeroPoints = 0.0;
	W.HeroLivePoints = 0.0;
}
} // namespace world
} // namespace ss
