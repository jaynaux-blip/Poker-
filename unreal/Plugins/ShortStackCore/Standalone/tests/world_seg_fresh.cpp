// living_world: core-world's fresh-start checks (update 2 SPEC §3.8): FreshSamePeople, FreshDayOne, FreshNetwork,
// FreshEmerges (a 90-day seed by default; 365 days x 3 seeds behind "world_test long"), FreshSaveRoundTrip, FreshHonesty,
// and the "both modes" fixes this track owns (§3.6): unknown RoiRank below 20 regulars, rank 0 (NEW) for nothing on a board.
// Phase A, day 1 (the lead): the entry point; core-world's checks, its helpers in world_test::fresh_seg.
#include "WorldSeg.h"

#include "CopyLint.h"
#include "ShortStack/Game/Badges.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/World.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace world_test
{
// world_main.cpp's: "world_test long", and the suite's invariants (their failures counted).
extern bool LongRun;
int CheckInvariants(const ss::world::World& W, const std::string& When);

namespace fresh_seg
{
namespace world = ss::world;
namespace net = ss::net;
namespace badge = ss::badge;

int Failures = 0;

void Check(bool Ok, const std::string& What)
{
	if (!Ok)
	{
		++Failures;
		std::printf("FAIL: %s\n", What.c_str());
	}
}

const double NightOne = 1440.0 + 127.0;

double Seconds(std::chrono::steady_clock::time_point Since)
{
	return std::chrono::duration<double>(std::chrono::steady_clock::now() - Since).count();
}

std::vector<std::string> Tabs(const std::string& Line)
{
	std::vector<std::string> Out(1);
	for (const char C : Line)
	{
		if (C == '\t')
		{
			Out.emplace_back();
		}
		else if (C != '\r')
		{
			Out.back() += C;
		}
	}
	return Out;
}

void Load(world::World& W, const std::string& Text)
{
	std::istringstream In(Text);
	std::string Line;
	while (std::getline(In, Line))
	{
		W.Read(Tabs(Line));
	}
	W.Finish();
}

std::string Text(const world::World& W)
{
	std::string Out;
	W.Write(Out);
	return Out;
}

/** The world's lines one by one. */
std::vector<std::string> Lines(const std::string& T)
{
	std::vector<std::string> Out;
	std::istringstream In(T);
	std::string Line;
	while (std::getline(In, Line))
	{
		Out.push_back(Line);
	}
	return Out;
}

std::string Lower(std::string S)
{
	for (char& C : S)
	{
		C = C >= 'A' && C <= 'Z' ? static_cast<char>(C - 'A' + 'a') : C;
	}
	return S;
}

/** A character a word or a handle is made of ("Rafael_Coringa", "charlie.h", "SqueezePlay11" are one each). */
bool WordChar(char C)
{
	return std::isalnum(static_cast<unsigned char>(C)) || C == '_' || C == '.' || C == '-';
}

/** Whether a word (no word character on either side) is in the text: case-blind, or exactly as written (a handle). */
bool HasWord(const std::string& Text, const std::string& Word, bool Exact = false)
{
	const std::string T = Exact ? Text : Lower(Text);
	const std::string W = Exact ? Word : Lower(Word);
	for (size_t At = T.find(W); At != std::string::npos; At = T.find(W, At + 1))
	{
		// (A sentence's full stop isn't part of the word before it.)
		const bool Left = At == 0 || !WordChar(T[At - 1]);
		const bool Right = At + W.size() >= T.size() || !WordChar(T[At + W.size()]) || (T[At + W.size()] == '.' && (At + W.size() + 1 >= T.size() || T[At + W.size() + 1] == ' '));
		if (Left && Right)
		{
			return true;
		}
	}
	return false;
}

bool Zero(const world::Ledger& L)
{
	return L.Won == 0 && L.Spent == 0 && L.Events == 0 && L.Cashes == 0 && L.FinalTables == 0 && L.Wins == 0 && L.Best == 0;
}

int Day(const world::World& W)
{
	return net::DayOf(W.Clock());
}

int BooksOpened(const world::World& W)
{
	return net::DayOf(W.StartedAt());
}

/** Team RiverLine's pay a week (the catalog's, or the flat $1,500 before it). */
ss::Chips TeamWeekly()
{
	const ss::Chips Weekly = ss::sponsor::NpcWeekly("riverline");
	return Weekly > 0 ? Weekly : 150000;
}

// ------------------------------------------------------------------ FreshSamePeople (U6, F1)

void SamePeople(const world::World& Old, const world::World& Fresh)
{
	Check(Old.Began() == world::Start::Established && Fresh.Began() == world::Start::Blank, "fresh: Began() says how each world began");
	Check(Old.People().size() == Fresh.People().size(), "fresh: the same roster size (" + std::to_string(Old.People().size()) + ", " + std::to_string(Fresh.People().size()) + ")");
	bool Same = Old.People().size() == Fresh.People().size();
	std::string First;
	for (size_t I = 0; Same && I < Old.People().size(); ++I)
	{
		const world::Npc& A = Old.People()[I];
		const world::Npc& B = Fresh.People()[I];
		const bool Pay = A.Pro ? B.Income == std::max<ss::Chips>(0, A.Income - TeamWeekly()) : B.Income == A.Income;
		const bool Here = A.Id == B.Id && A.Name == B.Name && A.Country == B.Country && A.Born == B.Born && A.Hue == B.Hue && A.From == B.From && A.Skills == B.Skills &&
			A.Traits == B.Traits && A.Peak == B.Peak && A.Potential == B.Potential && A.OnlineShare == B.OnlineShare && A.Formats == B.Formats && A.Schedule == B.Schedule &&
			A.Professional == B.Professional && A.Tier == B.Tier && A.Live == B.Live && A.Bankroll == B.Bankroll && A.PeakRoll == B.PeakRoll && A.Living == B.Living && Pay &&
			A.Followers == B.Followers && A.Streams == B.Streams && A.Rival == B.Rival && A.Anchored == B.Anchored && A.Sites == B.Sites &&
			ss::live::PlayStyle(A) == ss::live::PlayStyle(B);
		if (!Here)
		{
			Same = false;
			First = A.Name;
		}
	}
	Check(Same, "fresh: the same people (names, ids, skills, traits, tiers, live levels, bankrolls, schedules, followers, table styles)" + (First.empty() ? "" : ": " + First + " differs"));
	Check(Old.EmbercrestRegulars() == Fresh.EmbercrestRegulars(), "fresh: the same Embercrest regulars (their best bankrolls are kept, F1)");
}

// ------------------------------------------------------------------ FreshDayOne (§3.4)

void DayOne(const world::World& W)
{
	const int Today = Day(W);
	bool Records = true;
	bool Kept = true;
	bool Since = true;
	int Team = 0;
	int Streamers = 0;
	std::string Who;
	for (const world::Npc& N : W.People())
	{
		bool Clean = std::all_of(N.Totals.begin(), N.Totals.end(), Zero) && N.ThisSeason.Points == 0.0 && N.ThisSeason.Won == 0 && N.ThisSeason.Wins == 0 &&
			N.ThisSeason.FinalTables == 0 && N.ThisSeason.Cashes == 0 && N.ThisSeason.LivePoints == 0.0 && N.ThisSeason.Year == world::YearOf(Today) && N.Stats.Events == 0 &&
			N.Awards.empty() && N.Bracelets == 0 && N.Rings == 0 && N.Titles == 0 && N.Majors == 0 && N.Room.Events == 0 && N.Room.Days.empty() && N.CircuitPoints == 0.0 &&
			N.CardBest == world::CardNone && N.CardLast == world::CardNone && !N.Verified && N.ReviewMonth == -1 && N.BestEvent.empty() && N.BestDay == 0 && N.Recent.empty() &&
			N.WeekPoints == 0.0f && N.Years.size() == 1 && N.Years.front().Number == world::YearOf(Today) && N.Years.front().Online == 0 && N.Years.front().Events == 0;
		for (const float F : N.Weekly)
		{
			Clean = Clean && F == 0.0f;
		}
		if (!Clean && Who.empty())
		{
			Who = N.Name;
		}
		Records = Records && Clean;
		Team += N.Pro ? 1 : 0;
		Kept = Kept && N.Sponsor != "Team RiverLine";
		Since = Since && (N.Came != world::Arrival::None || (N.Joined == Today && N.TierSince == Today));
		Streamers += N.Streams && N.Followers >= 100000 ? 1 : 0;
	}
	Check(Records, "fresh day one: every record is zero (totals, season, stats, titles, the room, the circuit, Verified, best events)" + (Who.empty() ? "" : ": " + Who));
	Check(Team == 0 && Kept, "fresh day one: Team RiverLine starts empty (F3)");
	Check(Since, "fresh day one: everyone who came with the world is on the scene since the first night (F4)");
	Check(Streamers >= 2, "fresh day one: Kast's big streamers keep their audiences (F5)");
	// Fame came from the record: on day one only a Kast audience makes anyone known (F5), so nobody without one has a name.
	bool Unknown = true;
	std::string Famous;
	for (const world::Npc& N : W.People())
	{
		if (N.Followers == 0 && N.RepOf(world::Rep::Overall) > 0.0f)
		{
			Unknown = false;
			Famous = Famous.empty() ? N.Name : Famous;
		}
	}
	Check(Unknown, "fresh day one: nobody without a Kast audience has a reputation yet" + (Famous.empty() ? std::string() : ": " + Famous));
	Check(W.Honors().empty() && W.Events().empty(), "fresh day one: no honors, no news");
	bool Boards = true;
	for (const net::Board B : {net::Board::Earnings, net::Board::Season, net::Board::Wins, net::Board::FinalTables, net::Board::Series, net::Board::Live, net::Board::NightShift})
	{
		for (const world::Npc& N : W.People())
		{
			Boards = Boards && W.BoardValue(B, N.Id, W.Clock()) == 0.0 && W.RankBefore(B, N.Id) == 0;
		}
	}
	Check(Boards, "fresh day one: every board (the series' and the Night Shift's too) is empty, and nobody has a rank on one");
	Check(W.HeroSeasonPoints() == 0.0 && W.HeroStats().Events == 0, "fresh day one: the player starts from nothing too");
	// Bug 6: too few regulars on record to rank anyone against.
	Check(W.RoiRank(0.25) == -1.0 && W.RoiPlace(0.25, true) == -1 && W.RoiRank(-0.5, 1) == -1.0, "fresh day one: RoiRank and RoiPlace are unknown (-1), never \"beats 0%\"");
	// F4: nobody is Hall-eligible until 2036.
	bool Hall = true;
	for (const world::Npc& N : W.People())
	{
		Hall = Hall && !W.HallEligible(N.Id, 2035);
	}
	Check(Hall, "fresh day one: nobody is Hall of Fame-eligible before ten years on the books");
	Failures += CheckInvariants(W, "a fresh world's day one");
}

// ------------------------------------------------------------------ the both-modes fixes this track owns (§3.6)

void Ranks(const world::World& W, const std::string& What)
{
	// Rank 0 for nothing on a board; the rest numbered 1..n in order.
	bool Ok = true;
	for (const net::Board B : {net::Board::Earnings, net::Board::Season, net::Board::Wins, net::Board::FinalTables, net::Board::Series, net::Board::Live})
	{
		std::vector<std::pair<double, int>> On;
		for (const world::Npc& N : W.People())
		{
			const double V = W.BoardValue(B, N.Id, W.Clock());
			const int R = W.RankBefore(B, N.Id);
			Ok = Ok && (V > 0.0) == (R > 0);
			if (R > 0)
			{
				On.push_back({V, R});
			}
		}
		std::sort(On.begin(), On.end(), [](const std::pair<double, int>& A, const std::pair<double, int>& C) { return A.second < C.second; });
		for (size_t K = 0; K < On.size(); ++K)
		{
			Ok = Ok && On[K].second == static_cast<int>(K) + 1 && (K == 0 || On[K].first <= On[K - 1].first);
		}
	}
	Check(Ok, What + ": a board's ranks a day ago are 1..n for what's on it and 0 (NEW tomorrow) for nothing");
}

// ------------------------------------------------------------------ FreshNetwork (F7, bug 5, the NEW arrows)

void Network(world::World& W)
{
	net::Network& Net = net::Shared();
	const world::World* Was = Net.Attached();
	Net.Attach(&W);
	net::HeroStats Hero;
	Hero.Name = "Tester";
	// Events that ended before the books opened went to handles the world doesn't know.
	int Before = 0;
	bool Nobody = true;
	for (const net::EventInstance& E : Net.Window(W.StartedAt() - 2.0 * 1440.0, W.StartedAt() - 600.0))
	{
		if (E.Start + E.Duration > W.StartedAt())
		{
			continue;
		}
		const net::EventResult& R = Net.Result(E);
		Before += R.FinalTable.empty() ? 0 : 1;
		for (const net::Placing& P : R.FinalTable)
		{
			Nobody = Nobody && P.Player == -2 && !P.Name.empty() && W.Find(P.Name) < 0;
		}
	}
	Check(Before > 20 && Nobody, "fresh network: a result from before the books opened places nobody the world follows (" + std::to_string(Before) + " events)");
	// The news on the first night names nobody the world follows, and tells the team's date.
	std::string Named;
	bool Team = false;
	for (const net::NewsItem& It : Net.News(W.Clock(), Hero, 60))
	{
		if (It.Player >= 0)
		{
			Named = It.Title;
		}
		for (const world::Npc& N : W.People())
		{
			if (N.Name.size() >= 4 && (HasWord(It.Title, N.Name, true) || HasWord(It.Body, N.Name, true)))
			{
				Named = N.Name + " in \"" + It.Title + "\"";
			}
		}
		Team = Team || It.Title == "RiverLine signs its first team pros on Nov 1";
	}
	Check(Named.empty() && Team, "fresh network: the first night's news names nobody the world follows, and says when the team signs" + (Named.empty() ? std::string() : ": " + Named));
	// Bug 5: no board lists a $0 row; the first night's boards are empty.
	Check(Net.Leaderboard(net::Board::Earnings, W.Clock(), Hero, 60, nullptr).empty() && Net.Leaderboard(net::Board::Season, W.Clock(), Hero, 60, nullptr).empty(),
		"fresh network: Earnings and Season are empty at 2:07 AM on the first night");
	// The first night's last hour: the boards have filled, and every arrow reads NEW (nobody was on a board when the day
	// began; the arrows measure from the day's turn).
	W.AdvanceTo(static_cast<double>(BooksOpened(W) + 1) * 1440.0 - 60.0);
	Net.Attach(&W);
	const std::vector<net::BoardRow> Season = Net.Leaderboard(net::Board::Season, W.Clock(), Hero, 60, nullptr);
	const std::vector<net::BoardRow> Earnings = Net.Leaderboard(net::Board::Earnings, W.Clock(), Hero, 60, nullptr);
	bool Positive = true;
	bool New = true;
	for (const net::BoardRow& R : Season)
	{
		Positive = Positive && R.Value > 0.0;
		New = New && R.Player >= 0 && R.Move == -R.Rank; // RankBefore 0: NEW
	}
	for (const net::BoardRow& R : Earnings)
	{
		Positive = Positive && R.Value > 0.0;
		New = New && R.Player >= 0 && R.Move == -R.Rank;
	}
	Check(Season.size() >= 20 && !Earnings.empty() && Positive, "fresh network: by the first night's end, Season and Earnings have rows, none at zero (" + std::to_string(Season.size()) + " Season rows)");
	Check(New, "fresh network: on the first night, every arrow on Season and Earnings reads NEW (yesterday's rank 0)");
	// A day later the arrows measure from the second day's turn: a NEW row is one with nothing on the board then.
	W.AdvanceTo(static_cast<double>(BooksOpened(W) + 2) * 1440.0 - 60.0);
	Net.Attach(&W);
	bool Moves = true;
	int NewRows = 0;
	for (const net::BoardRow& R : Net.Leaderboard(net::Board::Season, W.Clock(), Hero, 60, nullptr))
	{
		const int Then = W.RankBefore(net::Board::Season, R.Player);
		Moves = Moves && R.Move == Then - R.Rank && (Then == 0) == (R.Move == -R.Rank);
		NewRows += Then == 0 ? 1 : 0;
	}
	Check(Moves, "fresh network: on the second night, the arrows move from the day's turn, NEW only for what wasn't on the board then (" + std::to_string(NewRows) + " NEW)");
	Net.Attach(Was);
}

// ------------------------------------------------------------------ FreshEmerges (§3.5)

double Spearman(const std::vector<double>& X, const std::vector<double>& Y)
{
	const auto RankOf = [](const std::vector<double>& V) {
		std::vector<size_t> Order(V.size());
		for (size_t K = 0; K < V.size(); ++K)
		{
			Order[K] = K;
		}
		std::sort(Order.begin(), Order.end(), [&](size_t A, size_t B) { return V[A] < V[B]; });
		std::vector<double> R(V.size(), 0.0);
		for (size_t K = 0; K < Order.size();)
		{
			size_t End = K;
			while (End + 1 < Order.size() && V[Order[End + 1]] == V[Order[K]])
			{
				++End;
			}
			const double Mid = (static_cast<double>(K) + static_cast<double>(End)) / 2.0 + 1.0;
			for (size_t J = K; J <= End; ++J)
			{
				R[Order[J]] = Mid;
			}
			K = End + 1;
		}
		return R;
	};
	const std::vector<double> A = RankOf(X);
	const std::vector<double> B = RankOf(Y);
	const double N = static_cast<double>(A.size());
	if (N < 3.0)
	{
		return 0.0;
	}
	double Ma = 0.0;
	double Mb = 0.0;
	for (size_t K = 0; K < A.size(); ++K)
	{
		Ma += A[K] / N;
		Mb += B[K] / N;
	}
	double Sab = 0.0;
	double Saa = 0.0;
	double Sbb = 0.0;
	for (size_t K = 0; K < A.size(); ++K)
	{
		Sab += (A[K] - Ma) * (B[K] - Mb);
		Saa += (A[K] - Ma) * (A[K] - Ma);
		Sbb += (B[K] - Mb) * (B[K] - Mb);
	}
	return Saa > 0.0 && Sbb > 0.0 ? Sab / std::sqrt(Saa * Sbb) : 0.0;
}

void Emerges(uint32_t Seed, int Days)
{
	const auto T0 = std::chrono::steady_clock::now();
	world::World W;
	W.Create(Seed, NightOne, world::Start::Blank);
	const int Opened = BooksOpened(W);
	const int Nov1 = world::DayOn(2026, 11, 1);
	const int Dec1 = world::DayOn(2026, 12, 1);
	const std::string Tag = "fresh, seed " + std::to_string(Seed);
	bool BoardsByDayOne = false;
	int ClubBy30 = -1;
	int VerifiedAt = -1;
	int TeamByDec1 = -1;
	int MostBreakouts = 0;
	size_t Seen = 0;
	double Spear = 0.0;
	double TopOverall = 0.0;
	double Cut = 0.0;
	bool Reported = true;
	bool Stars = false;
	bool AskNews = false;
	bool NewsMoney = true;
	int NewsSeen = 0;
	int Risers = 0;
	int Later = 0;
	for (int D = 1; D <= Days; ++D)
	{
		// (The Season's ranks at the last turn: a board report's riser climbs from these.)
		std::vector<int> Prev;
		if (D <= 30)
		{
			for (const world::Npc& N : W.People())
			{
				Prev.push_back(W.RankBefore(net::Board::Season, N.Id));
			}
		}
		W.Simulate(1);
		const int Today = Day(W);
		// Breakouts a day in week one (F9's gate: a breakout needs 30 events on record), and each board report against the
		// board as it stands (a report at the day's turn, the leader's points only grown since).
		std::map<int, int> Breakouts;
		for (size_t K = Seen; K < W.Events().size(); ++K)
		{
			const world::WorldEvent& E = W.Events()[K];
			if (E.Kind == world::EventKind::Breakout)
			{
				++Breakouts[net::DayOf(E.At)];
			}
			if (E.Kind == world::EventKind::BoardReport)
			{
				// (The ranks a day ago are the ones the report read: Ranks runs right after it, at the same turn.)
				// What: "day,rank,climb"; Amount stays 0 (the news reads it as money).
				const double Points = E.Npc == -1 ? W.HeroSeasonPoints() : W.BoardValue(net::Board::Season, E.Npc, W.Clock());
				const size_t C1 = E.What.find(',');
				const size_t C2 = C1 == std::string::npos ? std::string::npos : E.What.find(',', C1 + 1);
				const int Rank = C1 == std::string::npos ? -1 : std::atoi(E.What.c_str() + C1 + 1);
				const int Climb = C2 == std::string::npos ? -1 : std::atoi(E.What.c_str() + C2 + 1);
				const int Was = E.Other >= 0 && static_cast<size_t>(E.Other) < Prev.size() ? Prev[static_cast<size_t>(E.Other)] : 0;
				Reported = Reported && E.Amount == 0 && C2 != std::string::npos && E.Value >= 1 && static_cast<double>(E.Value) <= Points + 0.5 &&
					(E.Npc == -1 || W.RankBefore(net::Board::Season, E.Npc) == 1) &&
					(E.Other < 0 ? Climb == 0 : Climb > 0 && W.RankBefore(net::Board::Season, E.Other) == Rank && Was > 0 && Was - Rank == Climb && Rank <= 60);
				// (From the second morning there's a board a day old to climb: the report names a riser, one a reader can
				// find on the board's 60 rows.)
				Risers += std::atoi(E.What.c_str()) >= 2 && E.Other >= 0 ? 1 : 0;
				Later += std::atoi(E.What.c_str()) >= 2 ? 1 : 0;
				AskNews = true;
			}
			AskNews = AskNews || (E.Kind == world::EventKind::Sponsored && E.What == "Team RiverLine");
		}
		Seen = W.Events().size();
		if (AskNews)
		{
			// The news as RiverLine shows it, the morning of a board report or a signing: no money where nobody won any (the
			// ticker and the news card draw a world event's Amount as money, "($3.12)").
			AskNews = false;
			net::Network& Net = net::Shared();
			const world::World* Was = Net.Attached();
			Net.Attach(&W);
			net::HeroStats Hero;
			Hero.Name = "Tester";
			for (const net::NewsItem& It : Net.News(W.Clock(), Hero, 60))
			{
				const bool Ours = It.Tag == "BOARD REPORT" || (It.Tag == "SPONSOR" && It.Title.find("joins Team RiverLine") != std::string::npos);
				NewsMoney = NewsMoney && (!Ours || It.Amount == 0);
				NewsSeen += Ours ? 1 : 0;
			}
			Net.Attach(Was);
		}
		for (const auto& It : Breakouts)
		{
			MostBreakouts = It.first - Opened <= 7 ? std::max(MostBreakouts, It.second) : MostBreakouts;
		}
		if (D == 1)
		{
			for (const world::Npc& N : W.People())
			{
				BoardsByDayOne = BoardsByDayOne || W.BoardValue(net::Board::Season, N.Id, W.Clock()) > 0.0;
			}
		}
		if (ClubBy30 < 0 && D <= 30 && D % 5 == 0)
		{
			for (const world::Npc& N : W.People())
			{
				const badge::Set S = badge::Earned(badge::Of(W, N.Id, Today), Today);
				if (S.Find(badge::Kind::Club))
				{
					ClubBy30 = D;
					break;
				}
			}
		}
		if (VerifiedAt < 0 && D % 10 == 0)
		{
			for (const world::Npc& N : W.People())
			{
				VerifiedAt = N.Verified ? D : VerifiedAt;
			}
		}
		int Team = 0;
		for (const world::Npc& N : W.People())
		{
			Team += N.Pro ? 1 : 0;
		}
		if (Today < Nov1)
		{
			Check(Team == 0, Tag + ": Team RiverLine signs nobody before Nov 1 (day " + std::to_string(D) + ")");
		}
		if (TeamByDec1 < 0 && Today == Dec1)
		{
			TeamByDec1 = Team;
		}
		if (Today == world::DayOn(2026, 12, 31))
		{
			// Stars emerge: results follow skill, early ("by day 90": the 2026 season's points on its last day, day 86 of a
			// world begun on Night One; the season starts again on Jan 1).
			std::vector<double> Skill;
			std::vector<double> Points;
			std::vector<std::pair<double, double>> ByPoints;
			std::vector<double> Everyone;
			for (const world::Npc& N : W.People())
			{
				if (N.Faded)
				{
					continue;
				}
				Everyone.push_back(static_cast<double>(N.Overall()));
				ByPoints.push_back({N.ThisSeason.Points, static_cast<double>(N.Overall())});
				if (N.Totals[static_cast<size_t>(world::Venue::Online)].Events >= 30)
				{
					Skill.push_back(static_cast<double>(N.Overall()));
					Points.push_back(N.ThisSeason.Points);
				}
			}
			Spear = Spearman(Skill, Points);
			std::sort(ByPoints.begin(), ByPoints.end(), [](const std::pair<double, double>& A, const std::pair<double, double>& B) { return A.first > B.first; });
			for (size_t K = 0; K < 20 && K < ByPoints.size(); ++K)
			{
				TopOverall += ByPoints[K].second / 20.0;
			}
			std::sort(Everyone.begin(), Everyone.end());
			Cut = Everyone.empty() ? 1.0 : Everyone[static_cast<size_t>(0.7 * static_cast<double>(Everyone.size() - 1))];
			Check(Spear >= 0.25, Tag + ": by day 90 Season points follow skill (Spearman " + ss::Fixed(Spear, 3) + " >= 0.25, " + std::to_string(Skill.size()) + " regulars)");
			Check(TopOverall >= Cut, Tag + ": the top 20 by points average in the roster's top 30% of skill (" + ss::Fixed(TopOverall, 3) + " against " + ss::Fixed(Cut, 3) + ")");
			Stars = true;
		}
		if (D == 7 || D == 30 || D == 90 || D == Days)
		{
			Failures += CheckInvariants(W, Tag + ", day " + std::to_string(D));
		}
	}
	Check(BoardsByDayOne, Tag + ": the Season board has rows after the first night");
	Check(Reported, Tag + ": every board report names the Season leader and the biggest riser (and the places they climbed) as the board stood that morning");
	Check(Later >= 6 && Risers * 4 >= Later * 3, Tag + ": the board reports name a riser from the second morning on (" + std::to_string(Risers) + " of " + std::to_string(Later) + ")");
	Check(NewsMoney && NewsSeen >= 7, Tag + ": the news shows no money on a board report or a Team RiverLine signing (" + std::to_string(NewsSeen) + " items read)");
	Check(Stars || Days < 86, Tag + ": the 2026 season's points were read on its last day");
	Check(ClubBy30 > 0, Tag + ": a Club Regular by day 30");
	Check(MostBreakouts <= 10, Tag + ": at most 10 breakout headlines a day in week one (" + std::to_string(MostBreakouts) + ")");
	if (Days >= 60)
	{
		Check(TeamByDec1 >= 1 && TeamByDec1 <= 10, Tag + ": Team RiverLine's first class has signed by Dec 1 (" + std::to_string(TeamByDec1) + ")");
	}
	if (Days >= 200)
	{
		Check(VerifiedAt > 0 && VerifiedAt <= 200, Tag + ": a Verified Pro by day 200 (day " + std::to_string(VerifiedAt) + ")");
	}
	// The news of the first weeks: the board report every morning of week one and every Monday to week four, the first
	// winner of each event family once, Team RiverLine signing only on results.
	std::map<int, int> Reports;
	std::map<std::string, int> Families;
	int Named = 0;
	bool Real = true;
	bool Signed = true;
	bool Headlined = true;
	std::set<std::pair<int, double>> Majors;
	std::vector<std::pair<int, double>> EarlyFirsts;
	for (const world::WorldEvent& E : W.Events())
	{
		std::string Title;
		std::string Body;
		std::string Kind;
		if (E.Kind == world::EventKind::BoardReport)
		{
			const int K = std::atoi(E.What.c_str());
			++Reports[K];
			Real = Real && net::DayOf(E.At) == Opened + K;
			Headlined = Headlined && W.Headline(E, Title, Body, Kind) && Kind == "BOARD REPORT" &&
				Title.rfind(K == 1 ? "Night one on the books: " : K < 7 ? "Night " : "Week ", 0) == 0;
		}
		else if (E.Kind == world::EventKind::FirstWinner)
		{
			++Families[E.What];
			Real = Real && net::DayOf(E.At) - Opened < 60;
			if (const world::Npc* N = W.Get(E.Npc))
			{
				int Wins = 0;
				for (const world::Ledger& L : N->Totals)
				{
					Wins += L.Wins;
				}
				Real = Real && Wins >= 1;
				Named += 1;
				Headlined = Headlined && W.Headline(E, Title, Body, Kind) && Title == "The first " + E.What + " title in the record books goes to " + N->Name;
			}
			else
			{
				Headlined = Headlined && (E.Npc != -2 || !W.Headline(E, Title, Body, Kind));
			}
		}
		else if (E.Kind == world::EventKind::Sponsored && E.What == "Team RiverLine")
		{
			// (Value: the Season rank at the signing plus 1,000 a final table on record; Amount 0, the news reads it as money.)
			const int Fts = E.Value / 1000;
			const int Rank = E.Value % 1000;
			const world::Npc* N = W.Get(E.Npc);
			Signed = Signed && N && net::DayOf(E.At) >= Nov1 && E.Amount == 0 && (Fts >= 1 || (Rank >= 1 && Rank <= 100)) &&
				N->Totals[static_cast<size_t>(world::Venue::Online)].FinalTables >= Fts;
		}
		else if (E.Kind == world::EventKind::Champion)
		{
			Majors.insert({E.Npc, E.At});
		}
		else if (E.Kind == world::EventKind::Milestone && E.What == "first-title" && net::DayOf(E.At) - Opened < 60)
		{
			EarlyFirsts.push_back({E.Npc, E.At});
		}
	}
	// F9: in the first 60 days a "first title" milestone needs a major (its Champion headline posts at the same minute).
	bool FirstsOnMajors = true;
	for (const std::pair<int, double>& F : EarlyFirsts)
	{
		FirstsOnMajors = FirstsOnMajors && Majors.count(F) == 1;
	}
	Check(FirstsOnMajors, Tag + ": every first-title milestone of the first 60 days is a major's (" + std::to_string(EarlyFirsts.size()) + ")");
	bool EveryMorning = true;
	for (int K = 1; K <= 7; ++K)
	{
		EveryMorning = EveryMorning && Reports[K] == 1;
	}
	bool Mondays = true;
	for (int K = 8; K <= 28 && K < Days; ++K)
	{
		Mondays = Mondays && Reports[K] == (net::Weekday(Opened + K) == 0 ? 1 : 0);
	}
	bool Once = true;
	for (const auto& It : Families)
	{
		Once = Once && It.second == 1;
	}
	Check(EveryMorning && Mondays && Reports.count(29) == 0, Tag + ": a board report every morning of week one and every Monday to week four");
	Check(Once && Families.size() >= 25 && Families.count("Embercrest Nightly") == 1 && Families.count("Micro Madness") == 1 && (Days < 60 || Families.count("RCOP 2026") == 1),
		Tag + ": a first winner once for each event family (" + std::to_string(Families.size()) + " families, " + std::to_string(Named) + " named)");
	Check(Real && Headlined, Tag + ": every board report and first winner names a real result, and reads as one");
	Check(Signed, Tag + ": every Team RiverLine signing has an online final table or a top-100 Season rank, from Nov 1");
	std::printf("  %s, %d days (%.1f s): Spearman %.3f, top 20 at %.3f (cut %.3f), Club by day %d, Team %d on Dec 1, Verified by day %d, %zu families, %d breakouts at most a day\n",
		Tag.c_str(), Days, Seconds(T0), Spear, TopOverall, Cut, ClubBy30, TeamByDec1, VerifiedAt, Families.size(), MostBreakouts);
}

// ------------------------------------------------------------------ FreshSaveRoundTrip (§3.8)

void SaveRoundTrip()
{
	world::World W;
	W.Create(2468u, NightOne, world::Start::Blank);
	// The head: Start after HeroCardPrev (version 3); before the version moves, only a fresh start writes it.
	{
		world::World Old;
		Old.Create(2468u, NightOne);
		const std::vector<std::string> Head = Tabs(Lines(Text(W)).front());
		const std::vector<std::string> OldHead = Tabs(Lines(Text(Old)).front());
		const std::string Version = std::to_string(world::Version);
		Check(Head.size() == 24 && Head[1] == "head" && Head[2] == Version && Head.back() == "1", "fresh save: the head carries Start (1) after HeroCardPrev");
		Check(OldHead[2] == Version && OldHead.size() == (world::Version >= 3 ? 24u : 23u) && (world::Version < 3 || OldHead.back() == "0"),
			"fresh save: an established world's head carries Start (0) from version 3, and nothing before it");
	}
	W.Simulate(3);
	W.AdvanceTo(W.Clock() + 700.0); // the middle of a day, the Season's ranks a day ago in the save
	const std::string Saved = Text(W);
	world::World Copy;
	Load(Copy, Saved);
	const std::string Again = Text(Copy);
	Check(Copy.Began() == world::Start::Blank && Again == Saved, "fresh save: written, read and written again, the same text, still a fresh start");
	bool Ranked = false;
	for (const std::string& L : Lines(Saved))
	{
		Ranked = Ranked || L.rfind("world\tseasonranks\t", 0) == 0;
	}
	Check(Ranked, "fresh save: the first weeks keep the Season's ranks at the day's turn (the board report's riser)");
	// Nothing is seeded on load, and the world plays on exactly as one never saved (the board reports included).
	W.Simulate(6);
	Copy.Simulate(6);
	Check(Text(W) == Text(Copy), "fresh save: a loaded fresh world plays on exactly as the original (its board reports too)");
	Failures += CheckInvariants(Copy, "a fresh world, loaded");
	// Saved on night one, before the first turn (nobody had points at the last one): the next morning's report is the same
	// as a world never saved, with no riser measured from the minute it was loaded.
	{
		world::World Night;
		Night.Create(2468u, NightOne, world::Start::Blank);
		Night.AdvanceTo(static_cast<double>(BooksOpened(Night) + 1) * 1440.0 - 60.0);
		world::World Loaded;
		Load(Loaded, Text(Night));
		const double Morning = static_cast<double>(BooksOpened(Night) + 1) * 1440.0 + 180.0;
		Night.AdvanceTo(Morning);
		Loaded.AdvanceTo(Morning);
		int Reports = 0;
		for (const world::WorldEvent& E : Loaded.Events())
		{
			Reports += E.Kind == world::EventKind::BoardReport && E.Other < 0 ? 1 : 0;
		}
		Check(Text(Night) == Text(Loaded) && Reports == 1, "fresh save: saved on night one, the first board report is the one a world never saved writes");
	}
	// A head with no Start reads Established (every save from before version 3).
	{
		std::string Old;
		for (const std::string& L : Lines(Saved))
		{
			Old += (L.rfind("world\thead\t", 0) == 0 ? L.substr(0, L.rfind('\t')) : L) + "\n";
		}
		world::World Before;
		Load(Before, Old);
		Check(Before.Ready() && Before.Began() == world::Start::Established, "fresh save: a head with no Start reads Established");
	}
	// A fresh world that lost one of the room's locals gets them back on load with nothing on record (the load-time guard:
	// RoomLocals adds a local with a made-up history, which a fresh start never had).
	{
		world::World Day0;
		Day0.Create(1357u, NightOne, world::Start::Blank);
		const std::string Name = ss::live::RoomLocals().back().Name;
		const int Id = Day0.Find(Name);
		std::string Without;
		for (const std::string& L : Lines(Text(Day0)))
		{
			if (!(L.rfind("world\tnpc\t" + std::to_string(Id) + "\t", 0) == 0))
			{
				Without += L + "\n";
			}
		}
		world::World Back;
		Load(Back, Without);
		const world::Npc* N = Back.Get(Back.Find(Name));
		const world::Npc* Was = Day0.Get(Id);
		Check(Id == static_cast<int>(Day0.People().size()) - 1 && N && Was && N->Id == Id && N->Skills == Was->Skills && N->Bankroll == Was->Bankroll &&
				  std::all_of(N->Totals.begin(), N->Totals.end(), Zero) && N->Stats.Events == 0 && N->Joined == BooksOpened(Back) && N->Room.Events == 0,
			"fresh save: a local added on load joins a fresh world with nothing on record");
		Check(Text(Back) == Text(Day0), "fresh save: and the world reads as it was written");
	}
}

// ------------------------------------------------------------------ FreshHonesty (U5, §3.5)

/** Words that would claim a record a fresh world's day one doesn't have. */
bool Claims(const std::string& Line)
{
	for (const char* Word : {"title", "titles", "champion", "champions", "bracelet", "bracelets", "ring", "rings", "won", "wins", "winner", "winnings", "career", "years"})
	{
		if (HasWord(Line, Word))
		{
			return true;
		}
	}
	return Line.find('$') != std::string::npos || Line.find("201") != std::string::npos || Line.find("2025") != std::string::npos;
}

/** In-world copy never says the game played for anyone (U1). */
bool Machine(const std::string& Line)
{
	return HasWord(Line, "ai") || HasWord(Line, "bot") || HasWord(Line, "bots") || HasWord(Line, "autoplay");
}

void Honesty(world::World& W)
{
	// Day one: every card, its arrival line and its journey, and the first night's news.
	bool Clean = true;
	std::string Who;
	for (const world::Npc& N : W.People())
	{
		const world::Profile P = W.ProfileOf(N.Id);
		bool Here = P.Story.empty() && P.Recent.empty() && P.Awards.empty() && P.BestEvent.empty() && P.Best == 0 && P.Bracelets + P.Rings + P.Titles + P.Majors == 0 &&
			!Claims(P.Came) && !P.Came.empty();
		for (const world::Profile::Moment& M : P.Journey)
		{
			Here = Here && !Claims(M.Text);
		}
		for (const world::Ledger& L : P.Totals)
		{
			Here = Here && Zero(L);
		}
		if (!Here && Who.empty())
		{
			Who = N.Name + ": " + P.Came;
		}
		Clean = Clean && Here;
	}
	Check(Clean, "fresh honesty: no card on day one claims a title, a prize or a past (" + Who + ")");
	const world::Profile Founder = W.ProfileOf(W.Find("gh0stfold"));
	Check(Founder.Came == "One of RiverLine's regulars since the first night." && !Founder.Journey.empty() && Founder.Journey.front().Text == "On the books since the first night.",
		"fresh honesty: a founder's card reads \"since the first night\" (" + Founder.Came + ")");
	// The headlines a fresh world writes, on samples of every kind it changed, and the copy lint over them.
	W.HeroName = "Jesse";
	const int Mei = W.Find("Mei");
	const int Rick = W.Find("Rick");
	const int Volta = W.Find("VikingVolta");
	auto Event = [](world::EventKind K, int Npc, const std::string& What, ss::Chips Amount, int Value, int Other, int Flags, double At) {
		world::WorldEvent E;
		E.Kind = K;
		E.Npc = Npc;
		E.What = What;
		E.Amount = Amount;
		E.Value = Value;
		E.Other = Other;
		E.Flags = Flags;
		E.At = At;
		return E;
	};
	const double Now = W.Clock();
	const std::vector<std::pair<world::WorldEvent, std::string>> Samples = {
		{Event(world::EventKind::FirstWinner, Mei, "Night Owl Turbo", 41250, 812, -1, 0, Now), "The first Night Owl Turbo title in the record books goes to Mei"},
		{Event(world::EventKind::FirstWinner, -1, "Embercrest Nightly", 168000, 41, -1, world::FlagHero | world::FlagLive, Now), "The first Embercrest Nightly title in the record books goes to Jesse"},
		{Event(world::EventKind::BoardReport, Mei, "1,0,0", 0, 214, -1, 0, Now), "Night one on the books: Mei tops the Season board with 214 points"},
		{Event(world::EventKind::BoardReport, -1, "3,9,312", 0, 461, Rick, world::FlagHero, Now), "Night three on the books: Jesse tops the Season board with 461 points"},
		{Event(world::EventKind::BoardReport, Rick, "13,4,1", 0, 1, Mei, 0, Now), "Week two on the books: Rick tops the Season board with 1 point"},
		{Event(world::EventKind::Sponsored, Volta, "Team RiverLine", 0, 2 * 1000 + 37, -1, world::FlagFirst, Now), "VikingVolta joins Team RiverLine"},
		{Event(world::EventKind::Sponsored, Rick, "Team RiverLine", 0, 88, -1, 0, Now), "Rick joins Team RiverLine"},
		{Event(world::EventKind::Won, Mei, "$5.50 Night Owl Turbo", 41250, 812, Rick, world::FlagFirst, Now), "Mei wins $5.50 Night Owl Turbo"},
		{Event(world::EventKind::Breakout, Rick, "$22 Deep River", 1250000, 3, -1, 0, Now), "Rick's breakout"},
		{Event(world::EventKind::Retired, Rick, "", 0, 34, -1, 0, Now), "Rick retires"},
		{Event(world::EventKind::Retired, Mei, "", 4512000, 24, -1, 0, Now), "Mei retires"},
		{Event(world::EventKind::Milestone, Mei, "career-won", 10000000, 0, -1, 0, Now), "Mei passes $100K in winnings on the books"},
		{Event(world::EventKind::Milestone, Mei, "first-title", 2500000, 1, -1, 0, Now), "Mei wins a first major title in the new record books"},
		{Event(world::EventKind::PlayerOfYear, Mei, "Player of the Year", 0, 1840, -1, 0, static_cast<double>(world::DayOn(2027, 1, 1)) * 1440.0 - 1.0), "Mei is Player of the Year 2026"},
	};
	std::vector<std::pair<std::string, std::string>> Bodies;
	bool Read = true;
	bool Human = true;
	for (const auto& S : Samples)
	{
		std::string Title;
		std::string Body;
		std::string Tag;
		const bool Ok = W.Headline(S.first, Title, Body, Tag) && Title == S.second && !Tag.empty();
		if (!Ok)
		{
			std::printf("  fresh headline: \"%s\" / \"%s\" (wanted \"%s\")\n", Title.c_str(), Body.c_str(), S.second.c_str());
		}
		Read = Read && Ok && Title.find("  ") == std::string::npos && Title.find('{') == std::string::npos && copylint::Characters(Title) <= 120;
		Human = Human && !Machine(Title) && !Machine(Body);
		Bodies.push_back({Title, Body});
	}
	Check(Read, "fresh honesty: a fresh world's headlines read as written");
	Check(Human, "fresh honesty: no headline says the game played for anyone (U1)");
	Check(copylint::Run("world", Bodies, copylint::Pa) == 0, "fresh honesty: a fresh world's headlines pass the copy lint");
	// What the samples promise, word for word where it matters.
	{
		std::string Title;
		std::string Body;
		std::string Tag;
		W.Headline(Samples[5].first, Title, Body, Tag);
		Check(Body == "One of the site's first team pros, signed on results: 2 final tables on record and No. 37 on the Season board.", "fresh honesty: the first class says what it signed on (" + Body + ")");
		W.Headline(Samples[6].first, Title, Body, Tag);
		Check(Body == "The site's newest sponsored pro, signed on results: No. 88 on the Season board.", "fresh honesty: a signing on its Season rank alone says so (" + Body + ")");
		W.Headline(Samples[3].first, Title, Body, Tag);
		Check(Body == "Rick climbs 312 places to No. 9.", "fresh honesty: the board report names the biggest riser (" + Body + ")");
		W.Headline(Samples[4].first, Title, Body, Tag);
		Check(Body == "Mei climbs 1 place to No. 4.", "fresh honesty: the board report counts one place as one (" + Body + ")");
		W.Headline(Samples[2].first, Title, Body, Tag);
		Check(Body == "The record books opened on Oct 6. Every final table since counts toward Player of the Year.", "fresh honesty: night one's report says when the books opened (" + Body + ")");
		W.Headline(Samples[9].first, Title, Body, Tag);
		Check(Body == "Rick steps away from the game.", "fresh honesty: a retirement with nothing on the books claims no winnings (" + Body + ")");
		const world::WorldEvent Stranger = Event(world::EventKind::FirstWinner, -2, "Night Owl Turbo", 41250, 812, -1, world::FlagUnknown, Now);
		Check(!W.Headline(Stranger, Title, Body, Tag), "fresh honesty: a stranger's first never makes a headline");
	}
	W.HeroName.clear();
}

// ------------------------------------------------------------------ the session's side (§3.7) and ss.World.Start

void SessionSide()
{
	ss::SessionHooks Hooks;
	ss::hero::Character Who;
	{
		ss::Session S(Hooks, "fresh-world-start");
		S.NewCareer(Who, world::Start::Blank);
		const std::string Line = S.DebugWorld("");
		int Team = 0;
		for (const world::Npc& N : S.Living().People())
		{
			Team += N.Pro ? 1 : 0;
		}
		Check(S.WorldStart() == world::Start::Blank && S.Living().Began() == world::Start::Blank && Team == 0 && S.Living().Events().empty(),
			"fresh session: a new career in a fresh start gets a wiped world");
		Check(Line.find("World: Fresh start, version " + std::to_string(world::Version) + ", began Oct 6 ") == 0 &&
				  Line.find("Team RiverLine: 0 pros (signs on results from Nov 1).") != std::string::npos,
			"fresh session: ss.World.Start says how the world began, its version and when the team signs (" + Line + ")");
	}
	{
		ss::Session S(Hooks, "established-world-start");
		S.NewCareer(Who);
		const std::string Line = S.DebugWorld("");
		Check(S.WorldStart() == world::Start::Established && Line.find("World: Established, version ") == 0 && Line.find("Team RiverLine: 10 pros.") != std::string::npos,
			"established session: ss.World.Start (" + Line + ")");
	}
}

// ------------------------------------------------------------------ an established world: the fixes both modes share

void Established(const world::World& W)
{
	// Plenty of regulars to rank against: RoiRank stays a share.
	const double Rank = W.RoiRank(0.25);
	Check(Rank >= 0.0 && Rank <= 1.0 && W.RoiPlace(0.25, true) >= 1, "established: RoiRank ranks against the regulars (" + ss::Fixed(Rank, 3) + ")");
	// An established world's copy is the one it always had (the fresh start's variants beside it change nothing here).
	{
		const int Rick = W.Find("Rick");
		const world::Npc* N = W.Get(Rick);
		world::WorldEvent E;
		E.Kind = world::EventKind::Retired;
		E.Npc = Rick;
		E.Amount = 4500000;
		E.At = W.Clock();
		std::string Title;
		std::string Body;
		std::string Tag;
		const std::string Years = N ? std::to_string(std::max(1, (net::DayOf(W.Clock()) - N->Joined) / 365)) : std::string("?");
		const bool Retired = N && W.Headline(E, Title, Body, Tag);
		Check(Retired && Body == "After " + Years + " years and $45K in winnings, Rick steps away from the game.", "established: a retirement reads as it always has (" + Body + ")");
		E.Kind = world::EventKind::Sponsored;
		E.What = "Team RiverLine";
		E.Amount = 0;
		E.Value = 0;
		const bool Signed = W.Headline(E, Title, Body, Tag);
		Check(Signed && Body == "The site's newest sponsored pro.", "established: a Team RiverLine signing reads as it always has (" + Body + ")");
	}
	Ranks(W, "established, day one");
}
} // namespace fresh_seg

int SegFresh()
{
	namespace fs = fresh_seg;
	const auto T0 = std::chrono::steady_clock::now();
	fs::Failures = 0;
	// The same seed both ways (and how long each takes: a fresh start is no slower).
	const auto A = std::chrono::steady_clock::now();
	ss::world::World Old;
	Old.Create(7u, fs::NightOne);
	const double OldCreate = fs::Seconds(A);
	const auto B = std::chrono::steady_clock::now();
	ss::world::World Fresh;
	Fresh.Create(7u, fs::NightOne, ss::world::Start::Blank);
	const double FreshCreate = fs::Seconds(B);
	fs::SamePeople(Old, Fresh);
	fs::Established(Old);
	fs::DayOne(Fresh);
	fs::Ranks(Fresh, "fresh, day one");
	fs::Honesty(Fresh);
	fs::Network(Fresh);
	{
		// The first 30 days, both ways.
		const auto C = std::chrono::steady_clock::now();
		Old.Simulate(30);
		const double OldMonth = fs::Seconds(C);
		const auto D = std::chrono::steady_clock::now();
		Fresh.Simulate(29);
		const double FreshMonth = fs::Seconds(D);
		std::printf("  fresh: created in %.2f s (established %.2f s), 30 days in %.2f s (established %.2f s)\n", FreshCreate, OldCreate, FreshMonth, OldMonth);
		// (Wall clock on a shared machine, the suites running three at a time: a bound that catches a real slowdown, not a
		// busy second.)
		fs::Check(FreshCreate <= OldCreate * 3.0 + 0.25 && FreshMonth <= OldMonth * 3.0 + 1.0, "fresh: no slower than an established world to create and to run a month");
		// (The ranks a day ago are worked out when the day turns and when a world loads: checked on loaded copies.)
		ss::world::World FreshCopy;
		fs::Load(FreshCopy, fs::Text(Fresh));
		fs::Ranks(FreshCopy, "fresh, a month on");
		ss::world::World OldCopy;
		fs::Load(OldCopy, fs::Text(Old));
		fs::Ranks(OldCopy, "established, a month on");
	}
	fs::SaveRoundTrip();
	fs::SessionSide();
	if (LongRun)
	{
		for (const uint32_t Seed : {7u, 1234u, 98765u})
		{
			fs::Emerges(Seed, 365);
		}
	}
	else
	{
		fs::Emerges(7u, 90);
	}
	std::printf("fresh: %d checks failed (%.1f s)\n", fs::Failures, fs::Seconds(T0));
	return fs::Failures;
}
} // namespace world_test
