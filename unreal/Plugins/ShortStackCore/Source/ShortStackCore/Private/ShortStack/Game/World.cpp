// The living world's people: who is in it on Night One, who joins later, and the views the screens read.
#include "ShortStack/Game/World.h"

#include "ShortStack/Game/Handles.h"
#include "ShortStack/Game/Kast.h"
#include "WorldSim.h"

#include <algorithm>
#include <cmath>

namespace ss
{
namespace world
{
namespace world_detail
{

double PlanVolume(Plan P)
{
	switch (P)
	{
	case Plan::Daily: return 0.9;
	case Plan::Nights: return 0.6;
	case Plan::Weekends: return 0.45;
	case Plan::Occasional: return 0.2;
	case Plan::SeriesFocus: return 0.4;
	case Plan::MajorsOnly: return 0.3;
	case Plan::LiveCircuit: return 0.35;
	default: return 0.5;
	}
}

float Trait01(Rng& R, double Mean, double Sd)
{
	return sim::Clampf(R.Gauss(Mean, Sd), 0.02, 0.98);
}

void SetTraits(Npc& N, Rng& R, double Discipline, double Ambition, double Risk, double Patience, double Sociability, double Grit, double Aggro)
{
	N.Traits[static_cast<size_t>(Trait::Discipline)] = Trait01(R, Discipline, 0.16);
	N.Traits[static_cast<size_t>(Trait::Ambition)] = Trait01(R, Ambition, 0.16);
	N.Traits[static_cast<size_t>(Trait::Risk)] = Trait01(R, Risk, 0.18);
	N.Traits[static_cast<size_t>(Trait::Patience)] = Trait01(R, Patience, 0.16);
	N.Traits[static_cast<size_t>(Trait::Sociability)] = Trait01(R, Sociability, 0.2);
	N.Traits[static_cast<size_t>(Trait::Grit)] = Trait01(R, Grit, 0.18);
	N.Traits[static_cast<size_t>(Trait::Aggro)] = Trait01(R, Aggro, 0.2);
}

int RandomLikes(Rng& R)
{
	int L = 0;
	L |= R.Chance(0.35) ? LikeBounty : 0;
	L |= R.Chance(0.2) ? LikeSatellite : 0;
	L |= R.Chance(0.3) ? LikeTurbo : 0;
	L |= R.Chance(0.25) ? LikeDeep : 0;
	L |= R.Chance(0.08) ? LikeOmaha : 0;
	L |= R.Chance(0.15) ? LikeMystery : 0;
	return L;
}

Plan PlanFrom(Rng& R, double Volume, bool Pro, double OnlineShare, int Tier)
{
	if (OnlineShare < 0.6)
	{
		return Tier >= 3 || R.Chance(0.3) ? Plan::LiveCircuit : Plan::Weekends;
	}
	if (Volume > 0.8)
	{
		return Plan::Daily;
	}
	if (Volume > 0.55)
	{
		return Pro && R.Chance(0.5) ? Plan::Daily : Plan::Nights;
	}
	if (Volume > 0.35)
	{
		return R.Chance(0.55) ? Plan::Nights : Plan::Weekends;
	}
	if (Volume > 0.22)
	{
		return R.Chance(0.5) ? Plan::SeriesFocus : Plan::MajorsOnly;
	}
	return Plan::Occasional;
}

Ledger& OnlineLedger(Npc& N)
{
	return N.Totals[static_cast<size_t>(Venue::Online)];
}
} // namespace world_detail

using namespace world_detail;

// ------------------------------------------------------------------ people

float Npc::Overall() const
{
	double S = 0.0;
	for (float V : Skills)
	{
		S += static_cast<double>(V);
	}
	return static_cast<float>(S / static_cast<double>(SkillCount));
}

int Npc::Age(int Day) const
{
	return YearOf(Day) - Born;
}

Chips Npc::CareerWon() const
{
	Chips S = 0;
	for (const Ledger& L : Totals)
	{
		S += L.Won;
	}
	return S;
}

void Sim::MakeSkills(Npc& N, Rng& R, double Base)
{
	auto T = [&](Trait K) { return (static_cast<double>(N.TraitOf(K)) - 0.5) * 2.0; };
	std::array<double, SkillCount> V{};
	for (double& X : V)
	{
		X = Base + R.Gauss(0.0, 0.045);
	}
	auto Bump = [&](Skill S, double D) { V[static_cast<size_t>(S)] += D; };
	// Style shapes what they're good at: two players with the same average play very differently.
	Bump(Skill::Aggression, 0.08 * T(Trait::Aggro));
	Bump(Skill::Bluffing, 0.06 * T(Trait::Aggro));
	Bump(Skill::Value, -0.04 * T(Trait::Aggro));
	Bump(Skill::DeepStack, 0.06 * T(Trait::Patience));
	Bump(Skill::Emotion, 0.08 * T(Trait::Patience) + 0.05 * T(Trait::Discipline) - 0.05 * T(Trait::Risk));
	Bump(Skill::ShortStack, -0.03 * T(Trait::Patience));
	Bump(Skill::Icm, 0.04 * T(Trait::Discipline));
	if (N.Formats & LikeBounty)
	{
		Bump(Skill::Bounty, 0.08);
	}
	if (N.Formats & LikeSatellite)
	{
		Bump(Skill::Satellite, 0.1);
		Bump(Skill::Icm, 0.03);
	}
	if (N.Formats & LikeTurbo)
	{
		Bump(Skill::ShortStack, 0.06);
		Bump(Skill::Preflop, 0.03);
	}
	if (N.Formats & LikeDeep)
	{
		Bump(Skill::DeepStack, 0.06);
		Bump(Skill::Postflop, 0.03);
	}
	Bump(Skill::Live, N.OnlineShare < 0.6f ? 0.1 : -0.05);
	for (int K = 0; K < 2; ++K)
	{
		const int Strong = R.Int(SkillCount);
		V[static_cast<size_t>(Strong)] += R.Range(0.04, 0.1);
	}
	const int Weak = R.Int(SkillCount);
	V[static_cast<size_t>(Weak)] -= R.Range(0.04, 0.1);
	// Keep the average where it was meant to be.
	double Mean = 0.0;
	for (double X : V)
	{
		Mean += X;
	}
	Mean /= static_cast<double>(SkillCount);
	for (size_t I = 0; I < V.size(); ++I)
	{
		N.Skills[I] = sim::Clampf(V[I] - Mean + Base, 0.03, 0.99);
	}
}

double Sim::Need(const Npc& N)
{
	// Buy-ins a player wants behind them: disciplined pros a few hundred, gamblers a few dozen.
	double B = 45.0 + 210.0 * static_cast<double>(N.TraitOf(Trait::Discipline)) - 55.0 * static_cast<double>(N.TraitOf(Trait::Risk));
	if (N.Professional)
	{
		B += 30.0;
	}
	return sim::Clamp(B, 25.0, 300.0);
}

Chips Sim::Comfort(const Npc& N)
{
	const double FromRoll = static_cast<double>(N.Bankroll) / Need(N);
	return static_cast<Chips>(std::llround(std::max(FromRoll, 25.0)));
}

int Sim::TierFor(Chips Abi)
{
	return Abi < 600 ? 1 : Abi < 4500 ? 2 : Abi < 32000 ? 3 : 4;
}

double Sim::Habit(const Npc& N)
{
	static const double Daily[static_cast<int>(Plan::Count)] = {10.0, 3.8, 2.0, 0.5, 3.0, 0.9, 1.5};
	return std::max(0.5, Daily[static_cast<int>(N.Schedule)] * static_cast<double>(N.OnlineShare) * (N.Professional ? 1.15 : 1.0));
}

std::string Sim::UniqueName(World& W, Rng& R, const std::string& Country, bool Pro)
{
	std::string Name = handles::Make(R, Country, Pro);
	for (int Try = 0; W.ByName.count(Name) > 0 || Name.size() > 17 || Name.empty(); ++Try)
	{
		Name = Try < 8 ? handles::Make(R, Country, Pro) : Name.substr(0, std::min<size_t>(Name.size(), 13)) + std::to_string(R.Int(1000));
	}
	return Name;
}

int Sim::Add(World& W, Npc&& N)
{
	N.Id = static_cast<int>(W.Roster.size());
	W.ByName[N.Name] = N.Id;
	W.Roster.push_back(std::move(N));
	W.Rows.emplace_back();
	W.Refresh(W.Roster.back().Id);
	return W.Roster.back().Id;
}

void Sim::Found(World& W, double Start)
{
	const net::Network& Net = net::Shared();
	const std::vector<net::Player>& F = Net.Founding();
	std::vector<double> Pts;
	std::vector<Chips> Money;
	std::vector<int> Wins;
	std::vector<int> Fts;
	Net.FoundingTally(Start, Pts, Money, Wins, Fts);
	const int Today = sim::DayAt(Start);
	const int Year0 = YearOf(Today);
	Rng Root("world/" + std::to_string(W.WorldSeed) + "/founding");
	for (size_t I = 0; I < F.size(); ++I)
	{
		const net::Player& P = F[I];
		Rng R = Root.Fork(P.Name);
		Npc N;
		N.Name = P.Name;
		N.Country = P.Country;
		N.Hue = P.Hue;
		N.From = Origin::Founding;
		N.Home = RegionOf(P.Country);
		const int Tier = sim::TierIndex(static_cast<int>(P.Stake));
		const double Sk = static_cast<double>(P.Skill);
		const double AgeMean = Tier <= 1 ? 27.0 : Tier == 2 ? 29.0 : Tier == 3 ? 31.0 : 33.0;
		const int Age = static_cast<int>(sim::Clamp(std::round(R.Gauss(AgeMean, 6.5)), 19.0, 66.0));
		N.Born = Year0 - Age;
		SetTraits(N, R, 0.42 + 0.06 * static_cast<double>(Tier), 0.3 + 0.4 * Sk, 0.45, 0.5, 0.42, 0.5, 0.5);
		// Online first, mostly; some mix in live poker, a few are live players who also click buttons.
		N.OnlineShare = 0.97f;
		if (Tier >= 3 && R.Chance(0.14))
		{
			N.OnlineShare = static_cast<float>(R.Range(0.45, 0.75));
		}
		else if (R.Chance(0.06))
		{
			N.OnlineShare = static_cast<float>(R.Range(0.6, 0.85));
		}
		N.Formats = RandomLikes(R);
		MakeSkills(N, R, Sk);
		const double Youth = Age < 26 ? 1.6 : Age < 32 ? 1.0 : Age < 40 ? 0.5 : 0.2;
		N.Peak = sim::Clampf(Sk + std::max(0.0, R.Gauss(0.07, 0.06)) * Youth, Sk, 0.97);
		N.Potential = sim::Clampf(R.Gauss(0.5, 0.2) * (0.6 + 0.4 * Youth), 0.05, 1.0);
		// Professionals: the winning regulars who play a lot (at low stakes, mostly where rent is cheap).
		N.Professional = Tier == 4 ? R.Chance(0.85) : Tier == 3 ? (Sk > 0.55 && R.Chance(0.3 + 0.5 * Sk)) : Tier == 2 ? (P.Volume > 0.5f && Sk > 0.55 && R.Chance(0.25 + 0.5 * Sk))
			: (P.Volume > 0.85f && Sk > 0.6 && R.Chance(0.3));
		N.Schedule = PlanFrom(R, static_cast<double>(P.Volume), N.Professional, static_cast<double>(N.OnlineShare), Tier);
		N.Tier = Tier;
		N.TierSince = Today - R.Int(400);
		// A bankroll that fits the stakes they play (private: the world never shows it).
		static const double Median[5] = {150.0, 350.0, 2500.0, 18000.0, 160000.0};
		const double Roll = Median[Tier] * std::exp(R.Gauss(0.0, 0.55)) * (0.7 + 0.6 * Sk);
		N.Bankroll = std::max(sim::Cents(Roll), static_cast<Chips>(Need(N) * static_cast<double>(sim::TierLow[Tier]) * 2.0));
		N.PeakRoll = N.Bankroll;
		if (N.Professional)
		{
			N.Living = sim::Cents(sim::Living[Tier] * R.Range(0.7, 1.5));
		}
		else
		{
			// What they put into poker a week (from a job, a business, a pension).
			static const double Pay[5] = {8.0, 10.0, 45.0, 250.0, 2500.0};
			N.Income = sim::Cents(Pay[Tier] * std::exp(R.Gauss(0.0, 0.45)));
		}
		// Live: some never play it; the rest go as far as their money takes them.
		static const double PlaysLive[5] = {0.0, 0.2, 0.45, 0.75, 0.9};
		N.Live = LiveLevel::None;
		if (N.OnlineShare < 0.9f || R.Chance(PlaysLive[Tier]))
		{
			const double Can = sim::Dollars(N.Bankroll + N.Income * 26) * 100.0 / Need(N);
			int L = 1;
			while (L < 5 && Can >= sim::LiveNeed[L + 1] * 0.8)
			{
				++L;
			}
			N.Live = static_cast<LiveLevel>(L);
		}
		// What they've done: the network's own numbers, plus the simulated days before the world began.
		Ledger& On = OnlineLedger(N);
		On.Won = P.Earnings + P.SeasonEarnings + Money[I];
		On.Wins = P.Wins + Wins[I];
		On.FinalTables = P.FinalTables + Fts[I];
		On.Cashes = P.Cashes + Fts[I];
		On.Best = P.Best;
		On.Events = static_cast<int>(static_cast<double>(On.Cashes) / R.Range(0.13, 0.19));
		const double Roi = sim::Clamp(0.9 * (Sk - 0.45), -0.4, 0.6);
		On.Spent = static_cast<Chips>(static_cast<double>(On.Won) / (1.0 + Roi));
		N.ThisSeason.Year = Year0;
		N.ThisSeason.Points = P.SeasonPoints + Pts[I];
		N.ThisSeason.Won = P.SeasonEarnings + Money[I];
		N.ThisSeason.Wins = P.SeasonWins + Wins[I];
		N.ThisSeason.FinalTables = P.SeasonFinalTables + Fts[I];
		N.ThisSeason.Cashes = N.ThisSeason.FinalTables * 7 + R.Int(20);
		Year Y;
		Y.Number = Year0;
		Y.Online = N.ThisSeason.Won;
		Y.Wins = N.ThisSeason.Wins;
		Y.Events = N.ThisSeason.Cashes * 6;
		Y.Net = static_cast<Chips>(static_cast<double>(N.ThisSeason.Won) * Roi / (1.0 + Roi));
		N.Years.push_back(Y);
		N.Weekly = P.Form;
		// Fame from the record so far (reputations come from these).
		const double Won = sim::Dollars(On.Won);
		N.Fame[static_cast<size_t>(Rep::Online)] = static_cast<float>(20.0 * std::log10(1.0 + Won / 20000.0));
		N.Fame[static_cast<size_t>(Rep::HighStakes)] = Tier == 4 ? static_cast<float>(8.0 + 10.0 * Sk) : Tier == 3 ? 1.0f : 0.0f;
		if (N.Live >= LiveLevel::Regional)
		{
			N.Fame[static_cast<size_t>(Rep::Live)] = static_cast<float>(R.Range(0.0, 3.0) * static_cast<double>(static_cast<int>(N.Live)));
			Ledger& L = N.Totals[static_cast<size_t>(Venue::Live)];
			L.Events = 8 * static_cast<int>(N.Live) + R.Int(30);
			L.Cashes = L.Events / 6;
			L.Spent = sim::Cents(static_cast<double>(L.Events) * (Tier >= 3 ? 1500.0 : 300.0));
			L.Won = static_cast<Chips>(static_cast<double>(L.Spent) * R.Range(0.5, 1.4) * (0.6 + Sk));
			L.Best = L.Won / 4;
		}
		N.Pro = P.Pro;
		if (P.Pro)
		{
			N.Sponsor = "Team RiverLine";
			N.Professional = true;
			N.Income = sim::Cents(1500.0);
		}
		N.Joined = Today - 365 * std::max(0, std::min(Age - 19, 1 + R.Int(10)));
		N.LastDay = Today - 1;
		N.Began = Identity::OnlineGrinder;
		if (P.Rival)
		{
			// The player's rival: a crusher on the night shift who never logs off.
			N.Rival = true;
			N.Anchored = true;
			N.Born = Year0 - 27;
			N.Professional = true;
			N.Schedule = Plan::Daily;
			N.OnlineShare = 0.93f;
			N.Live = LiveLevel::Local;
			N.Tier = 2;
			N.Bankroll = sim::Cents(9000.0);
			N.PeakRoll = N.Bankroll;
			N.Living = sim::Cents(380.0);
			N.Income = 0;
			N.Traits[static_cast<size_t>(Trait::Discipline)] = 0.9f;
			N.Traits[static_cast<size_t>(Trait::Ambition)] = 0.55f;
			N.Traits[static_cast<size_t>(Trait::Grit)] = 0.95f;
			N.Traits[static_cast<size_t>(Trait::Sociability)] = 0.2f;
			N.Traits[static_cast<size_t>(Trait::Risk)] = 0.25f;
			N.Skills[static_cast<size_t>(Skill::Emotion)] = 0.97f;
			N.Peak = 0.95f;
			N.BestEvent = "Summer Slam #41: $22 Six-Max";
			N.BestDay = -30;
			N.Streams = true;
		}
		N.Is = N.Began;
		Add(W, std::move(N));
	}
	W.Founding = static_cast<int>(F.size());

	// The people with faces: the Back Room's regulars and the Riverside's Sunday crowd.
	struct CastSpec
	{
		const char* Name;
		int Age;
		double Skill;
		double Peak;
		double Potential;
		double Discipline, Ambition, Risk, Patience, Sociability, Grit, Aggro;
		double OnlineShare;
		bool Professional;
		double Roll;
		double Weekly; // income (or living costs for a professional)
		int Tier;
		LiveLevel Live;
		Plan Schedule;
		int Likes;
		int Hue;
	};
	static const CastSpec Cast[] = {
		{"Sal", 58, 0.48, 0.5, 0.1, 0.85, 0.2, 0.15, 0.92, 0.6, 0.7, 0.12, 0.15, false, 6000.0, 120.0, 2, LiveLevel::Local, Plan::Weekends, LikeDeep, 32},
		{"Mrs. Park", 67, 0.5, 0.5, 0.05, 0.9, 0.15, 0.12, 0.95, 0.5, 0.8, 0.1, 0.06, false, 9000.0, 150.0, 2, LiveLevel::Local, Plan::Occasional, LikeDeep, 340},
		{"Rick", 34, 0.55, 0.62, 0.35, 0.3, 0.5, 0.72, 0.35, 0.75, 0.6, 0.82, 0.2, true, 15000.0, 380.0, 2, LiveLevel::Regional, Plan::Nights, LikeBounty | LikeTurbo, 12},
		{"Mei", 24, 0.6, 0.86, 0.85, 0.72, 0.92, 0.4, 0.7, 0.55, 0.8, 0.55, 0.6, false, 4200.0, 80.0, 2, LiveLevel::Local, Plan::Nights, LikeDeep | LikeSatellite, 200},
		{"Dre", 29, 0.36, 0.42, 0.25, 0.25, 0.3, 0.55, 0.3, 0.8, 0.4, 0.45, 0.4, false, 350.0, 40.0, 1, LiveLevel::Local, Plan::Weekends, LikeBounty, 95},
		{"Big Lou", 52, 0.34, 0.36, 0.1, 0.2, 0.55, 0.7, 0.4, 0.9, 0.6, 0.6, 0.2, false, 15000.0, 1000.0, 2, LiveLevel::Regional, Plan::MajorsOnly, LikeBounty, 22},
		{"Twitch", 22, 0.4, 0.62, 0.6, 0.1, 0.6, 0.92, 0.12, 0.85, 0.35, 0.95, 0.6, false, 1200.0, 60.0, 1, LiveLevel::Local, Plan::Nights, LikeTurbo | LikeBounty, 300},
	};
	for (const CastSpec& C : Cast)
	{
		Rng R = Root.Fork(std::string("cast/") + C.Name);
		Npc N;
		N.Name = C.Name;
		N.Country = "US";
		N.Hue = C.Hue;
		N.From = Origin::Cast;
		N.Home = Region::Americas;
		N.Born = Year0 - C.Age;
		N.Traits = {static_cast<float>(C.Discipline), static_cast<float>(C.Ambition), static_cast<float>(C.Risk), static_cast<float>(C.Patience), static_cast<float>(C.Sociability),
			static_cast<float>(C.Grit), static_cast<float>(C.Aggro)};
		N.OnlineShare = static_cast<float>(C.OnlineShare);
		N.Formats = C.Likes;
		MakeSkills(N, R, C.Skill);
		N.Skills[static_cast<size_t>(Skill::Live)] = sim::Clampf(C.Skill + 0.12, 0.0, 0.99);
		N.Peak = static_cast<float>(C.Peak);
		N.Potential = static_cast<float>(C.Potential);
		N.Professional = C.Professional;
		N.Bankroll = sim::Cents(C.Roll);
		N.PeakRoll = N.Bankroll;
		(C.Professional ? N.Living : N.Income) = sim::Cents(C.Weekly);
		N.Tier = C.Tier;
		N.TierSince = Today - 200;
		N.Live = C.Live;
		N.Schedule = C.Schedule;
		N.Anchored = true;
		N.Joined = Today - 365 * 3;
		N.LastDay = Today - 1;
		Ledger& L = N.Totals[static_cast<size_t>(Venue::Live)];
		L.Events = 60 + R.Int(200);
		L.Cashes = L.Events / 7;
		L.Spent = static_cast<Chips>(L.Events) * 15000;
		L.Won = static_cast<Chips>(static_cast<double>(L.Spent) * (0.55 + C.Skill));
		L.Best = L.Won / 6;
		N.Fame[static_cast<size_t>(Rep::Live)] = 1.0f;
		N.Fame[static_cast<size_t>(Rep::Underground)] = C.Risk > 0.6 ? 6.0f : 3.0f;
		N.ThisSeason.Year = Year0;
		Year Y;
		Y.Number = Year0;
		N.Years.push_back(Y);
		N.Began = N.Is = Identity::LiveRegular;
		Add(W, std::move(N));
	}

	// Kast's established poker streamers.
	struct StreamSpec
	{
		const char* Name;
		int Age;
		double Skill;
		int Tier;
		double Roll;
		double Discipline, Ambition, Risk, Sociability, Aggro;
		LiveLevel Live;
		int Likes;
	};
	static const StreamSpec Streamers[] = {
		{"VikingVolta", 31, 0.62, 2, 5000.0, 0.45, 0.7, 0.6, 0.97, 0.72, LiveLevel::Regional, LikeBounty | LikeTurbo},
		{"HighRollerHana", 29, 0.74, 4, 1400000.0, 0.6, 0.85, 0.55, 0.85, 0.6, LiveLevel::HighRoller, LikeDeep},
		{"MissFinch", 34, 0.76, 4, 900000.0, 0.92, 0.6, 0.2, 0.8, 0.45, LiveLevel::Circuit, LikeDeep},
		{"BluffSquadTV", 27, 0.63, 3, 40000.0, 0.5, 0.65, 0.6, 0.95, 0.75, LiveLevel::Regional, LikeBounty | LikeMystery},
		{"SuitedConnor", 25, 0.58, 2, 6500.0, 0.92, 0.75, 0.2, 0.85, 0.45, LiveLevel::Local, LikeSatellite},
		{"LaReinaDelRio", 28, 0.64, 3, 35000.0, 0.6, 0.7, 0.45, 0.9, 0.55, LiveLevel::Regional, LikeBounty},
		{"KatOnTheRiver", 26, 0.6, 2, 5000.0, 0.5, 0.65, 0.6, 0.9, 0.7, LiveLevel::Local, LikeBounty | LikeMystery},
		{"ThePokerMonk", 38, 0.69, 3, 60000.0, 0.95, 0.4, 0.15, 0.6, 0.3, LiveLevel::Regional, LikeDeep},
		{"NitNation", 33, 0.52, 2, 9000.0, 0.95, 0.25, 0.05, 0.92, 0.05, LiveLevel::Local, LikeDeep},
		{"chipleader_carla", 23, 0.6, 2, 5500.0, 0.6, 0.9, 0.5, 0.88, 0.6, LiveLevel::Local, LikeBounty | LikeTurbo},
		{"FoldEquityFred", 41, 0.66, 2, 9000.0, 0.85, 0.35, 0.2, 0.75, 0.35, LiveLevel::Local, LikeSatellite | LikeDeep},
	};
	for (const StreamSpec& S : Streamers)
	{
		if (W.ByName.count(S.Name))
		{
			continue;
		}
		Rng R = Root.Fork(std::string("kast/") + S.Name);
		Npc N;
		N.Name = S.Name;
		for (const kast::Streamer& K : kast::Directory())
		{
			if (K.Name == S.Name)
			{
				N.Country = K.Country;
				N.Followers = K.Followers;
				N.Hue = static_cast<int>((K.Color >> 16) % 360u);
			}
		}
		N.From = Origin::Directory;
		N.Home = RegionOf(N.Country);
		N.Born = Year0 - S.Age;
		SetTraits(N, R, S.Discipline, S.Ambition, S.Risk, 0.5, S.Sociability, 0.6, S.Aggro);
		N.Traits[static_cast<size_t>(Trait::Sociability)] = static_cast<float>(S.Sociability);
		N.OnlineShare = S.Live >= LiveLevel::Circuit ? 0.75f : 0.92f;
		N.Formats = S.Likes;
		MakeSkills(N, R, S.Skill);
		N.Peak = sim::Clampf(S.Skill + (S.Age < 30 ? 0.12 : 0.04), 0.0, 0.95);
		N.Potential = S.Age < 30 ? 0.7f : 0.4f;
		N.Professional = true;
		N.Streams = true;
		N.Bankroll = sim::Cents(S.Roll);
		N.PeakRoll = N.Bankroll;
		N.Living = sim::Cents(sim::Living[S.Tier]);
		N.Tier = S.Tier;
		N.TierSince = Today - 120;
		N.Live = S.Live;
		N.Schedule = Plan::Daily;
		N.Joined = Today - 365 * 4;
		N.LastDay = Today - 1;
		Ledger& On = OnlineLedger(N);
		On.Events = 4000 + R.Int(6000);
		On.Cashes = On.Events * 16 / 100;
		On.Spent = static_cast<Chips>(On.Events) * sim::TierAbi[S.Tier];
		On.Won = static_cast<Chips>(static_cast<double>(On.Spent) * (0.75 + S.Skill * 0.6));
		On.Wins = On.Cashes / 60;
		On.FinalTables = On.Wins * 7;
		On.Best = On.Won / 12;
		N.Fame[static_cast<size_t>(Rep::Online)] = static_cast<float>(20.0 * std::log10(1.0 + sim::Dollars(On.Won) / 20000.0));
		N.Fame[static_cast<size_t>(Rep::HighStakes)] = S.Tier == 4 ? 12.0f : 0.0f;
		N.ThisSeason.Year = Year0;
		Year Y;
		Y.Number = Year0;
		N.Years.push_back(Y);
		N.Began = N.Is = Identity::Streamer;
		Add(W, std::move(N));
	}
	for (Npc& N : W.Roster)
	{
		Reputation(N);
	}
}

Npc Sim::Rookie(World& W, Rng& R, int Day)
{
	Npc N;
	N.Country = handles::PickCountry(R);
	N.Name = UniqueName(W, R, N.Country, false);
	N.Hue = R.Int(360);
	N.From = Origin::Rookie;
	N.Home = RegionOf(N.Country);
	const int Age = static_cast<int>(sim::Clamp(std::round(18.0 + std::fabs(R.Gauss(0.0, 7.0))), 18.0, 58.0));
	N.Born = YearOf(Day) - Age;
	SetTraits(N, R, 0.4, 0.5, 0.5, 0.45, 0.45, 0.5, 0.5);
	N.OnlineShare = R.Chance(0.08) ? static_cast<float>(R.Range(0.25, 0.6)) : static_cast<float>(R.Range(0.9, 1.0));
	N.Formats = RandomLikes(R);
	// Most newcomers are ordinary; a few are the real thing.
	const bool Talent = R.Chance(0.035);
	const double Base = sim::Clamp(R.Gauss(Talent ? 0.52 : 0.36, 0.09), 0.1, 0.78);
	MakeSkills(N, R, Base);
	const double Youth = Age < 24 ? 1.6 : Age < 30 ? 1.1 : Age < 40 ? 0.5 : 0.2;
	N.Peak = sim::Clampf(Base + std::max(0.02, R.Gauss(Talent ? 0.28 : 0.12, 0.07)) * Youth, Base, 0.97);
	N.Potential = sim::Clampf((Talent ? R.Gauss(0.85, 0.1) : R.Gauss(0.45, 0.2)) * (0.6 + 0.4 * Youth), 0.05, 1.0);
	if (Talent)
	{
		N.Traits[static_cast<size_t>(Trait::Ambition)] = sim::Clampf(static_cast<double>(N.TraitOf(Trait::Ambition)) + 0.25, 0.0, 0.98);
	}
	N.Professional = false;
	N.Income = sim::Cents(R.Range(3.0, 25.0) * (R.Chance(0.04) ? 20.0 : 1.0)); // what goes into poker a week
	N.Bankroll = sim::Cents(std::exp(R.Gauss(std::log(120.0), 0.7)));
	N.PeakRoll = N.Bankroll;
	N.Tier = 1;
	N.TierSince = Day;
	N.Live = N.OnlineShare < 0.7f ? LiveLevel::Local : LiveLevel::None;
	const double P = R.Next();
	N.Schedule = N.OnlineShare < 0.7f ? Plan::Weekends : P < 0.35 ? Plan::Nights : P < 0.6 ? Plan::Weekends : P < 0.85 ? Plan::Occasional : P < 0.95 ? Plan::Daily : Plan::MajorsOnly;
	N.Joined = Day;
	N.ThisSeason.Year = YearOf(Day);
	Year Y;
	Y.Number = N.ThisSeason.Year;
	N.Years.push_back(Y);
	N.Began = N.Is = Identity::Recreational;
	return N;
}

Npc Sim::Discover(World& W, const std::string& Name, const std::string& Country, double Strength, int Day, uint32_t Salt)
{
	// An unknown who just won something: probably a decent player the world hadn't been following.
	Rng R("world/" + std::to_string(W.WorldSeed) + "/discover/" + Name + "/" + std::to_string(Salt));
	Npc N;
	N.Name = Name;
	N.Country = Country;
	N.Hue = R.Int(360);
	N.From = Origin::Discovered;
	N.Home = RegionOf(Country);
	const int Age = static_cast<int>(sim::Clamp(std::round(R.Gauss(30.0, 9.0)), 19.0, 70.0));
	N.Born = YearOf(Day) - Age;
	SetTraits(N, R, 0.45, 0.55, 0.5, 0.5, 0.45, 0.5, 0.5);
	N.OnlineShare = R.Chance(0.25) ? static_cast<float>(R.Range(0.3, 0.7)) : 0.95f;
	N.Formats = RandomLikes(R);
	const double Base = sim::Clamp(Strength + R.Gauss(-0.02, 0.08), 0.15, 0.85); // winners are often just lucky
	MakeSkills(N, R, Base);
	const double Youth = Age < 26 ? 1.5 : Age < 33 ? 1.0 : 0.4;
	N.Peak = sim::Clampf(Base + std::max(0.0, R.Gauss(0.08, 0.06)) * Youth, Base, 0.96);
	N.Potential = sim::Clampf(R.Gauss(0.5, 0.2), 0.05, 1.0);
	N.Professional = R.Chance(0.3);
	if (N.Professional)
	{
		N.Living = sim::Cents(sim::Living[2] * R.Range(0.7, 1.4));
	}
	else
	{
		N.Income = sim::Cents(R.Range(20.0, 150.0));
	}
	N.Bankroll = sim::Cents(std::exp(R.Gauss(std::log(3000.0), 0.8)));
	N.PeakRoll = N.Bankroll;
	N.Tier = TierFor(Comfort(N));
	N.TierSince = Day;
	N.Live = N.OnlineShare < 0.8f ? LiveLevel::Regional : LiveLevel::Local;
	N.Schedule = N.OnlineShare < 0.8f ? Plan::LiveCircuit : R.Chance(0.5) ? Plan::Nights : Plan::Weekends;
	N.Joined = Day - 365 * (1 + R.Int(6));
	N.ThisSeason.Year = YearOf(Day);
	Year Y;
	Y.Number = N.ThisSeason.Year;
	N.Years.push_back(Y);
	N.Began = N.Is = N.OnlineShare < 0.8f ? Identity::LiveRegular : Identity::Recreational;
	return N;
}

// ------------------------------------------------------------------ the world

World::World() = default;

void World::Create(uint32_t InSeed, double StartWorld)
{
	*this = World();
	Created = true;
	WorldSeed = InSeed;
	Now = StartWorld;
	Origin = StartWorld;
	const int Today = sim::DayAt(StartWorld);
	Week = Today - net::Weekday(Today);
	Month = YearOf(Today) * 12; // refined on the first month change
	Sim::Found(*this, StartWorld);
	Target = static_cast<int>(Roster.size());
	// The fields each tier plays against start where the network's regulars are.
	std::array<double, 5> Sum{};
	std::array<int, 5> Count{};
	for (const Npc& N : Roster)
	{
		Sum[static_cast<size_t>(N.Tier)] += static_cast<double>(N.Overall());
		++Count[static_cast<size_t>(N.Tier)];
	}
	static const double Base[5] = {0.3, 0.38, 0.5, 0.6, 0.7};
	double All = 0.0;
	int People = 0;
	for (size_t T = 0; T < 5; ++T)
	{
		FieldRef[T] = Count[T] > 0 ? Sum[T] / static_cast<double>(Count[T]) : Base[T];
		All += Sum[T];
		People += Count[T];
	}
	// [0] is the whole world's average skill: the fields move with it.
	FieldRef[0] = People > 0 ? All / static_cast<double>(People) : 0.5;
	Sim::RefreshField(*this);
	// Yesterday's late events still running, today's and tomorrow's registrations.
	Planned = Today - 2;
	Sim::PlanDay(*this, Today - 1);
	Sim::PlanDay(*this, Today);
	Sim::PlanDay(*this, Today + 1);
	Sim::Ranks(*this);
	++Rev;
}

int World::Find(const std::string& Name) const
{
	const auto It = ByName.find(Name);
	return It == ByName.end() ? -1 : It->second;
}

int World::ActiveCount() const
{
	int N = 0;
	for (const Npc& P : Roster)
	{
		N += P.Playing() ? 1 : 0;
	}
	return N;
}

void World::Refresh(int Id)
{
	if (Id < 0 || Id >= static_cast<int>(Roster.size()))
	{
		return;
	}
	const Npc& N = Roster[static_cast<size_t>(Id)];
	if (Rows.size() < Roster.size())
	{
		Rows.resize(Roster.size());
	}
	net::Player& P = Rows[static_cast<size_t>(Id)];
	const Ledger& On = N.Totals[static_cast<size_t>(Venue::Online)];
	P.Name = N.Name;
	P.Country = N.Country;
	P.Hue = N.Hue;
	P.Skill = N.Overall();
	P.Volume = static_cast<float>(PlanVolume(N.Schedule));
	P.Stake = static_cast<net::Tier>(sim::TierIndex(N.Tier));
	P.Pro = N.Pro;
	P.Rival = N.Rival;
	// With a world, the record is the whole career (the season is part of it, not on top of it).
	P.Earnings = On.Won;
	P.Wins = On.Wins;
	P.FinalTables = On.FinalTables;
	P.Cashes = On.Cashes;
	P.Best = On.Best;
	P.SeasonPoints = N.ThisSeason.Points;
	P.SeasonEarnings = N.ThisSeason.Won;
	P.SeasonWins = N.ThisSeason.Wins;
	P.SeasonFinalTables = N.ThisSeason.FinalTables;
	P.Form = N.Weekly;
}

void World::RefreshAll()
{
	Rows.resize(Roster.size());
	for (size_t I = 0; I < Roster.size(); ++I)
	{
		Refresh(static_cast<int>(I));
	}
}
} // namespace world
} // namespace ss
