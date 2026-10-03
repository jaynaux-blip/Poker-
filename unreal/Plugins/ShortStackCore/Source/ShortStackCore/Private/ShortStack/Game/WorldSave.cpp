// Saving the living world: "world" lines in the save, versioned. Everything the simulation depends on is
// written at full precision, so a world that is saved and loaded carries on exactly as it would have.
#include "ShortStack/Game/World.h"

#include "WorldSim.h"

#include <algorithm>
#include <charconv>
#include <cstdio>
#include <cstdlib>

namespace ss
{
namespace world
{
namespace worldsave_detail
{
std::string Esc(const std::string& S)
{
	std::string Out;
	Out.reserve(S.size());
	for (char C : S)
	{
		switch (C)
		{
		case '\\': Out += "\\\\"; break;
		case '\t': Out += "\\t"; break;
		case '\n': Out += "\\n"; break;
		case '|': Out += "\\p"; break;
		case '~': Out += "\\s"; break;
		default: Out += C; break;
		}
	}
	return Out;
}

std::string Unesc(const std::string& S)
{
	std::string Out;
	Out.reserve(S.size());
	for (size_t I = 0; I < S.size(); ++I)
	{
		if (S[I] == '\\' && I + 1 < S.size())
		{
			const char C = S[++I];
			Out += C == 't' ? '\t' : C == 'n' ? '\n' : C == 'p' ? '|' : C == 's' ? '~' : C;
		}
		else
		{
			Out += S[I];
		}
	}
	return Out;
}

std::vector<std::string> Split(const std::string& S, char Sep)
{
	std::vector<std::string> Out;
	if (S.empty())
	{
		return Out;
	}
	std::string Cur;
	for (size_t I = 0; I < S.size(); ++I)
	{
		if (S[I] == '\\' && I + 1 < S.size())
		{
			Cur += S[I];
			Cur += S[++I];
		}
		else if (S[I] == Sep)
		{
			Out.push_back(Cur);
			Cur.clear();
		}
		else
		{
			Cur += S[I];
		}
	}
	Out.push_back(Cur);
	return Out;
}

// The shortest text that reads back as exactly the same number (so a loaded world carries on exactly).
std::string D(double V)
{
	char Buf[40];
#if defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L
	const std::to_chars_result R = std::to_chars(Buf, Buf + sizeof(Buf), V);
	return std::string(Buf, R.ptr);
#else
	std::snprintf(Buf, sizeof(Buf), "%.17g", V);
	return Buf;
#endif
}

std::string F(float V)
{
	char Buf[32];
#if defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L
	const std::to_chars_result R = std::to_chars(Buf, Buf + sizeof(Buf), V);
	return std::string(Buf, R.ptr);
#else
	std::snprintf(Buf, sizeof(Buf), "%.9g", static_cast<double>(V));
	return Buf;
#endif
}

std::string I(long long V)
{
	return std::to_string(V);
}

/** A line being written: fields joined by tabs. */
struct Line
{
	std::string S;
	explicit Line(const char* Kind) : S(std::string("world\t") + Kind) {}
	Line& operator<<(const std::string& V)
	{
		S += '\t';
		S += V;
		return *this;
	}
};

/** A record inside a field: values joined by '~'. */
std::string Rec(std::initializer_list<std::string> Values)
{
	std::string S;
	bool First = true;
	for (const std::string& V : Values)
	{
		if (!First)
		{
			S += '~';
		}
		S += V;
		First = false;
	}
	return S;
}

/** Reads fields in order. */
struct Cursor
{
	const std::vector<std::string>& V;
	size_t At;
	std::string Str() { return At < V.size() ? Unesc(V[At++]) : std::string(); }
	long long Int() { return At < V.size() ? std::strtoll(V[At++].c_str(), nullptr, 10) : 0; }
	int Small() { return static_cast<int>(Int()); }
	double Dbl() { return At < V.size() ? std::strtod(V[At++].c_str(), nullptr) : 0.0; }
	float Flt() { return At < V.size() ? std::strtof(V[At++].c_str(), nullptr) : 0.0f; }
	bool Bool() { return Int() != 0; }
};

void WriteLedger(Line& L, const Ledger& G)
{
	L << I(G.Won) << I(G.Spent) << I(G.Events) << I(G.Cashes) << I(G.FinalTables) << I(G.Wins) << I(G.Best);
}

void ReadLedger(Cursor& C, Ledger& G)
{
	G.Won = C.Int();
	G.Spent = C.Int();
	G.Events = C.Small();
	G.Cashes = C.Small();
	G.FinalTables = C.Small();
	G.Wins = C.Small();
	G.Best = C.Int();
}

std::string JoinRecords(const std::vector<std::string>& Records)
{
	std::string S;
	for (size_t K = 0; K < Records.size(); ++K)
	{
		if (K > 0)
		{
			S += '|';
		}
		S += Records[K];
	}
	return S;
}

std::vector<std::string> AwardRecords(const std::vector<Award>& Awards)
{
	std::vector<std::string> R;
	for (const Award& A : Awards)
	{
		R.push_back(Rec({I(A.Day), I(A.Ring), I(A.Online), I(A.Main), Esc(A.Series), Esc(A.Event), I(A.Prize), I(A.Entries), I(A.Tourney)}));
	}
	return R;
}

std::vector<Award> ReadAwards(const std::string& Field)
{
	std::vector<Award> Out;
	for (const std::string& R : Split(Field, '|'))
	{
		const std::vector<std::string> V = Split(R, '~');
		Cursor Rc{V, 0};
		Award A;
		A.Day = Rc.Small();
		A.Ring = Rc.Bool();
		A.Online = Rc.Bool();
		A.Main = Rc.Bool();
		A.Series = Rc.Str();
		A.Event = Rc.Str();
		A.Prize = Rc.Int();
		A.Entries = Rc.Small();
		A.Tourney = Rc.Small();
		Out.push_back(A);
	}
	return Out;
}
/** A stats page: the totals, the breakdowns, the records, then the graph (dollars, as steps from the last point). */
void WriteTracker(Line& L, const Tracker& T)
{
	L << I(T.Events) << I(T.BuyIns) << I(T.Prizes) << I(T.Cashes) << I(T.FinalTables) << I(T.Podiums) << I(T.Wins);
	for (const TrackLine& X : T.ByStake)
	{
		L << I(X.Events) << I(X.BuyIns) << I(X.Prizes) << I(X.Cashes);
	}
	for (const TrackLine& X : T.ByFormat)
	{
		L << I(X.Events) << I(X.BuyIns) << I(X.Prizes) << I(X.Cashes);
	}
	L << I(T.Stride) << I(T.Net) << I(T.Peak) << I(T.PeakAt) << I(T.Downswing) << I(T.DownFrom) << I(T.DownTo) << I(T.Best) << I(T.BestAt) << I(T.Dry) << I(T.LongestDry)
	  << D(T.FinishSum) << I(T.Finished);
	std::string Steps;
	long long Last = 0;
	for (size_t K = 0; K < T.Curve.size(); ++K)
	{
		const long long Dollars = (T.Curve[K] >= 0 ? T.Curve[K] + 50 : T.Curve[K] - 50) / 100;
		Steps += (K > 0 ? "~" : "") + I(Dollars - Last);
		Last = Dollars;
	}
	L << Steps;
}

void ReadTracker(Cursor& C, Tracker& T)
{
	T = Tracker();
	T.Events = C.Small();
	T.BuyIns = C.Int();
	T.Prizes = C.Int();
	T.Cashes = C.Small();
	T.FinalTables = C.Small();
	T.Podiums = C.Small();
	T.Wins = C.Small();
	for (TrackLine& X : T.ByStake)
	{
		X.Events = C.Small();
		X.BuyIns = C.Int();
		X.Prizes = C.Int();
		X.Cashes = C.Small();
	}
	for (TrackLine& X : T.ByFormat)
	{
		X.Events = C.Small();
		X.BuyIns = C.Int();
		X.Prizes = C.Int();
		X.Cashes = C.Small();
	}
	T.Stride = std::max(1, C.Small());
	T.Net = C.Int();
	T.Peak = C.Int();
	T.PeakAt = C.Small();
	T.Downswing = C.Int();
	T.DownFrom = C.Small();
	T.DownTo = C.Small();
	T.Best = C.Int();
	T.BestAt = C.Small();
	T.Dry = C.Small();
	T.LongestDry = C.Small();
	T.FinishSum = C.Dbl();
	T.Finished = C.Small();
	long long Run = 0;
	for (const std::string& Step : Split(C.At < C.V.size() ? C.V[C.At] : std::string(), '~'))
	{
		Run += std::strtoll(Step.c_str(), nullptr, 10);
		T.Curve.push_back(static_cast<Chips>(Run * 100));
	}
	++C.At;
}
} // namespace worldsave_detail

using namespace worldsave_detail;

void World::Write(std::string& Out) const
{
	if (!Created)
	{
		return;
	}
	auto Emit = [&](const Line& L) {
		Out += L.S;
		Out += '\n';
	};
	{
		Line L("head");
		L << I(Version) << I(WorldSeed) << D(Now) << D(Origin) << I(Planned) << I(Rev) << I(Founding) << I(Week) << I(Month) << I(DiscoveredThisWeek) << I(NextAnon)
		  << I(Target) << D(HeroPoints) << D(HeroLivePoints) << D(NightKey) << D(NightHour) << Esc(SeriesKey) << Esc(HeroName);
		Emit(L);
	}
	{
		Line L("field");
		for (double V : Field)
		{
			L << D(V);
		}
		for (double V : FieldRef)
		{
			L << D(V);
		}
		Emit(L);
	}
	for (const Npc& N : Roster)
	{
		if (N.Faded)
		{
			Line G("ghost");
			G << I(N.Id) << Esc(N.Name) << Esc(N.Country) << I(N.Hue) << I(N.Born) << I(static_cast<int>(N.From)) << I(static_cast<int>(N.Home)) << I(N.Left) << I(N.Joined)
			  << I(N.LastDay) << I(N.Tier) << I(static_cast<int>(N.Is)) << I(static_cast<int>(N.Began)) << F(N.Skills[0]) << Esc(N.BestEvent) << I(N.BestDay) << I(N.ThisSeason.Year);
			for (const Ledger& Gd : N.Totals)
			{
				G << I(Gd.Won) << I(Gd.Events) << I(Gd.Cashes) << I(Gd.FinalTables) << I(Gd.Wins) << I(Gd.Best);
			}
			Emit(G);
			continue;
		}
		Line L("npc");
		L << I(N.Id) << Esc(N.Name) << Esc(N.Country) << I(N.Hue) << I(N.Born) << I(static_cast<int>(N.From)) << I(static_cast<int>(N.Home));
		for (float V : N.Skills)
		{
			L << F(V);
		}
		L << F(N.Peak) << F(N.Potential);
		for (float V : N.Traits)
		{
			L << F(V);
		}
		L << F(N.OnlineShare) << I(N.Formats) << I(static_cast<int>(N.Schedule)) << I(N.Professional);
		L << I(N.Bankroll) << I(N.PeakRoll) << I(N.Income) << I(N.Living) << I(N.Backer) << F(N.BackerShare) << I(N.Makeup) << I(N.BackedSince);
		L << I(N.Tier) << I(static_cast<int>(N.Live)) << I(N.TierSince);
		for (const Ledger& G : N.Totals)
		{
			WriteLedger(L, G);
		}
		const Season& S = N.ThisSeason;
		L << I(S.Year) << D(S.Points) << I(S.Won) << I(S.Wins) << I(S.FinalTables) << I(S.Cashes) << D(S.LivePoints) << I(S.LiveWon);
		for (float V : N.Weekly)
		{
			L << F(V);
		}
		L << F(N.WeekPoints) << Esc(N.BestEvent) << I(N.BestDay) << I(N.Bracelets) << I(N.Rings) << I(N.Titles) << I(N.Majors);
		L << I(static_cast<int>(N.Mood)) << I(N.MoodSince) << F(N.Confidence) << F(N.Fatigue) << F(N.Swing) << F(N.Tilt) << I(N.WeekEvents);
		for (int V : N.WeekKinds)
		{
			L << I(V);
		}
		L << I(N.WeekNet) << I(N.MonthNet) << I(N.WeeksBroke);
		for (float V : N.Fame)
		{
			L << F(V);
		}
		for (float V : N.Reps)
		{
			L << F(V);
		}
		L << I(N.Streams) << I(N.Followers) << Esc(N.Sponsor) << I(N.Pro) << I(N.Rival) << I(N.Anchored) << I(static_cast<int>(N.St)) << I(N.Until) << I(N.Joined)
		  << I(N.LastDay) << I(N.Left) << I(N.TripFrom) << I(N.TripUntil) << Esc(N.Trip) << I(static_cast<int>(N.Is)) << I(static_cast<int>(N.Began))
		  << I(static_cast<int>(N.Came)) << I(N.Arrived) << I(N.CameWith);
		Emit(L);
		if (!N.Recent.empty())
		{
			std::vector<std::string> R;
			for (const world::Finish& Fi : N.Recent)
			{
				R.push_back(Rec({I(Fi.Day), Esc(Fi.Event), I(Fi.Place), I(Fi.Entries), I(Fi.Prize), I(Fi.Where), I(Fi.Major), I(Fi.Seat)}));
			}
			Emit(Line("recent") << I(N.Id) << JoinRecords(R));
		}
		if (!N.Years.empty())
		{
			std::vector<std::string> R;
			for (const Year& Y : N.Years)
			{
				R.push_back(Rec({I(Y.Number), I(Y.Online), I(Y.Live), I(Y.Net), I(Y.Wins), I(Y.Events)}));
			}
			Emit(Line("years") << I(N.Id) << JoinRecords(R));
		}
		if (!N.Ties.empty())
		{
			std::vector<std::string> R;
			for (const Tie& T : N.Ties)
			{
				R.push_back(Rec({I(T.Other), I(static_cast<int>(T.Kind)), F(T.Strength), I(T.Since)}));
			}
			Emit(Line("ties") << I(N.Id) << JoinRecords(R));
		}
		if (!N.Tickets.empty())
		{
			std::vector<std::string> R;
			for (const auto& T : N.Tickets)
			{
				R.push_back(Rec({Esc(T.first), I(T.second)}));
			}
			Emit(Line("tickets") << I(N.Id) << JoinRecords(R));
		}
		if (!N.Path.empty())
		{
			std::vector<std::string> R;
			for (const Step& St : N.Path)
			{
				R.push_back(Rec({I(St.Day), I(static_cast<int>(St.Kind)), Esc(St.What), I(St.Place), I(St.Of), I(St.Amount)}));
			}
			Emit(Line("path") << I(N.Id) << JoinRecords(R));
		}
		if (!N.Awards.empty())
		{
			Emit(Line("awards") << I(N.Id) << JoinRecords(AwardRecords(N.Awards)));
		}
		if (N.Stats.Events > 0)
		{
			Line Tl("track");
			Tl << I(N.Id);
			WriteTracker(Tl, N.Stats);
			Emit(Tl);
		}
	}
	if (!HeroTrophies.empty())
	{
		Emit(Line("heroawards") << JoinRecords(AwardRecords(HeroTrophies)));
	}
	if (HeroBook.Events > 0)
	{
		Line Hl("herotrack");
		WriteTracker(Hl, HeroBook);
		Emit(Hl);
	}
	for (const Pending& P : Queue)
	{
		Line L("q");
		L << Esc(P.Id) << Esc(P.Name) << I(P.Online) << I(P.Template) << I(P.Kind) << D(P.Start) << D(P.End) << I(P.Entries) << I(P.Pool) << I(P.BuyIn) << I(P.Tier)
		  << I(P.Format) << I(P.TableSize) << I(P.Major) << I(P.Bracelet) << I(P.Ring) << Esc(P.Series) << Esc(P.Ticket) << I(P.SeatValue) << I(P.Stakes) << I(P.HeroPlace)
		  << I(P.HeroPrize) << D(P.Strength) << D(P.Luck) << I(P.Kinds) << I(P.Seats) << I(P.FinalSize);
		std::vector<std::string> R;
		for (const Entry& E : P.Who)
		{
			R.push_back(Rec({I(E.Npc), I(E.Ticket), I(E.Staked), I(E.Bullets), F(E.Share), I(E.Known), I(E.Better)}));
		}
		L << JoinRecords(R);
		Emit(L);
	}
	for (const auto& It : Results)
	{
		const auto End = ResultEnds.find(It.first);
		// The last couple of days, and the big ones (older small results aren't needed to carry on).
		const bool Recent = End == ResultEnds.end() || End->second >= Now - 3.0 * 1440.0;
		const bool Big = !It.second.FinalTable.empty() && It.second.FinalTable.front().Prize >= 2500000;
		if (!Recent && !Big)
		{
			continue;
		}
		Line L("res");
		L << Esc(It.first) << D(End == ResultEnds.end() ? Now : End->second);
		std::vector<std::string> R;
		for (const net::Placing& P : It.second.FinalTable)
		{
			R.push_back(Rec({I(P.Player), I(P.Place), I(P.Prize), Esc(P.Name), Esc(P.Country)}));
		}
		L << JoinRecords(R);
		Emit(L);
	}
	const size_t LogFrom = Log.size() > 2500 ? Log.size() - 2500 : 0;
	for (size_t K = LogFrom; K < Log.size(); ++K)
	{
		const WorldEvent& E = Log[K];
		Emit(Line("log") << D(E.At) << I(static_cast<int>(E.Kind)) << I(E.Npc) << I(E.Other) << I(E.Amount) << Esc(E.What) << I(E.Value) << I(E.Flags));
	}
	for (const Honor& H : Titles)
	{
		Emit(Line("honor") << I(H.Year) << Esc(H.Title) << Esc(H.EventId) << I(H.Npc) << Esc(H.Name) << I(H.Prize) << I(H.Entries) << D(H.At));
	}
	for (const auto& It : HeroBonds)
	{
		const Bond& B = It.second;
		Line L("bond");
		L << I(It.first) << F(B.Familiarity) << F(B.Respect) << F(B.Trust) << F(B.Rivalry) << F(B.Resentment) << I(B.Encounters) << I(B.LastDay);
		std::vector<std::string> R;
		for (const Memory& M : B.Memories)
		{
			R.push_back(Rec({I(M.Day), I(static_cast<int>(M.Kind)), Esc(M.Where), I(M.Amount)}));
		}
		L << JoinRecords(R);
		Emit(L);
	}
	for (const auto& It : HeroResults)
	{
		Emit(Line("heroresult") << Esc(It.first) << I(It.second.first) << I(It.second.second));
	}
	for (const std::string& Id : HeroIn)
	{
		Emit(Line("heroin") << Esc(Id));
	}
	auto Points = [&](const char* Kind, const std::map<int, double>& M) {
		if (M.empty())
		{
			return;
		}
		std::vector<std::string> R;
		for (const auto& It : M)
		{
			R.push_back(Rec({I(It.first), D(It.second)}));
		}
		Emit(Line(Kind) << JoinRecords(R));
	};
	Points("series", SeriesPoints);
	Points("night", NightPoints);
	Points("nightago", NightPointsHourAgo);
	if (!WeekPairs.empty())
	{
		std::vector<std::string> R;
		for (const auto& P : WeekPairs)
		{
			R.push_back(Rec({I(P.first), I(P.second)}));
		}
		Emit(Line("pairs") << JoinRecords(R));
	}
}

bool World::Read(const std::vector<std::string>& Fields)
{
	if (Fields.size() < 2 || Fields[0] != "world")
	{
		return false;
	}
	const std::string& Kind = Fields[1];
	Cursor C{Fields, 2};
	auto Person = [&](long long Id) -> Npc* { return Id >= 0 && static_cast<size_t>(Id) < Roster.size() ? &Roster[static_cast<size_t>(Id)] : nullptr; };
	if (Kind == "head")
	{
		const int Saved = C.Small();
		(void)Saved; // version 1: nothing to migrate yet
		WorldSeed = static_cast<uint32_t>(C.Int());
		Now = C.Dbl();
		Origin = C.Dbl();
		Planned = C.Small();
		Rev = C.Small();
		Founding = C.Small();
		Week = C.Small();
		Month = C.Small();
		DiscoveredThisWeek = C.Small();
		NextAnon = C.Small();
		Target = C.Small();
		HeroPoints = C.Dbl();
		HeroLivePoints = C.Dbl();
		NightKey = C.Dbl();
		NightHour = C.Dbl();
		SeriesKey = C.Str();
		HeroName = C.Str();
		Created = true;
	}
	else if (Kind == "field")
	{
		for (double& V : Field)
		{
			V = C.Dbl();
		}
		for (double& V : FieldRef)
		{
			V = C.Dbl();
		}
	}
	else if (Kind == "npc")
	{
		Npc N;
		N.Id = C.Small();
		N.Name = C.Str();
		N.Country = C.Str();
		N.Hue = C.Small();
		N.Born = C.Small();
		N.From = static_cast<world::Origin>(C.Small());
		N.Home = static_cast<Region>(C.Small());
		for (float& V : N.Skills)
		{
			V = C.Flt();
		}
		N.Peak = C.Flt();
		N.Potential = C.Flt();
		for (float& V : N.Traits)
		{
			V = C.Flt();
		}
		N.OnlineShare = C.Flt();
		N.Formats = C.Small();
		N.Schedule = static_cast<Plan>(C.Small());
		N.Professional = C.Bool();
		N.Bankroll = C.Int();
		N.PeakRoll = C.Int();
		N.Income = C.Int();
		N.Living = C.Int();
		N.Backer = C.Small();
		N.BackerShare = C.Flt();
		N.Makeup = C.Int();
		N.BackedSince = C.Small();
		N.Tier = C.Small();
		N.Live = static_cast<LiveLevel>(C.Small());
		N.TierSince = C.Small();
		for (Ledger& G : N.Totals)
		{
			ReadLedger(C, G);
		}
		Season& S = N.ThisSeason;
		S.Year = C.Small();
		S.Points = C.Dbl();
		S.Won = C.Int();
		S.Wins = C.Small();
		S.FinalTables = C.Small();
		S.Cashes = C.Small();
		S.LivePoints = C.Dbl();
		S.LiveWon = C.Int();
		for (float& V : N.Weekly)
		{
			V = C.Flt();
		}
		N.WeekPoints = C.Flt();
		N.BestEvent = C.Str();
		N.BestDay = C.Small();
		N.Bracelets = C.Small();
		N.Rings = C.Small();
		N.Titles = C.Small();
		N.Majors = C.Small();
		N.Mood = static_cast<Momentum>(C.Small());
		N.MoodSince = C.Small();
		N.Confidence = C.Flt();
		N.Fatigue = C.Flt();
		N.Swing = C.Flt();
		N.Tilt = C.Flt();
		N.WeekEvents = C.Small();
		for (int& V : N.WeekKinds)
		{
			V = C.Small();
		}
		N.WeekNet = C.Int();
		N.MonthNet = C.Int();
		N.WeeksBroke = C.Small();
		for (float& V : N.Fame)
		{
			V = C.Flt();
		}
		for (float& V : N.Reps)
		{
			V = C.Flt();
		}
		N.Streams = C.Bool();
		N.Followers = C.Small();
		N.Sponsor = C.Str();
		N.Pro = C.Bool();
		N.Rival = C.Bool();
		N.Anchored = C.Bool();
		N.St = static_cast<Status>(C.Small());
		N.Until = C.Small();
		N.Joined = C.Small();
		N.LastDay = C.Small();
		N.Left = C.Small();
		N.TripFrom = C.Small();
		N.TripUntil = C.Small();
		N.Trip = C.Str();
		N.Is = static_cast<Identity>(C.Small());
		N.Began = static_cast<Identity>(C.Small());
		// Newcomers' arrivals (saves from before them have none).
		const int Came = C.Small();
		N.Came = Came > 0 && Came < static_cast<int>(Arrival::Count) ? static_cast<Arrival>(Came) : Arrival::None;
		N.Arrived = C.Small();
		N.CameWith = C.Small();
		if (N.Came == Arrival::None)
		{
			N.Arrived = 0;
			N.CameWith = -1;
		}
		if (N.Id < 0 || N.Id > 1000000)
		{
			return true;
		}
		if (Roster.size() <= static_cast<size_t>(N.Id))
		{
			Roster.resize(static_cast<size_t>(N.Id) + 1);
		}
		Roster[static_cast<size_t>(N.Id)] = std::move(N);
	}
	else if (Kind == "ghost")
	{
		Npc N;
		N.Id = C.Small();
		N.Name = C.Str();
		N.Country = C.Str();
		N.Hue = C.Small();
		N.Born = C.Small();
		N.From = static_cast<world::Origin>(C.Small());
		N.Home = static_cast<Region>(C.Small());
		N.Left = C.Small();
		N.Joined = C.Small();
		N.LastDay = C.Small();
		N.Tier = C.Small();
		N.Is = static_cast<Identity>(C.Small());
		N.Began = static_cast<Identity>(C.Small());
		N.Skills.fill(C.Flt());
		N.BestEvent = C.Str();
		N.BestDay = C.Small();
		N.ThisSeason.Year = C.Small();
		for (Ledger& Gd : N.Totals)
		{
			Gd.Won = C.Int();
			Gd.Events = C.Small();
			Gd.Cashes = C.Small();
			Gd.FinalTables = C.Small();
			Gd.Wins = C.Small();
			Gd.Best = C.Int();
		}
		N.St = Status::Retired;
		N.Faded = true;
		if (N.Id < 0 || N.Id > 1000000)
		{
			return true;
		}
		if (Roster.size() <= static_cast<size_t>(N.Id))
		{
			Roster.resize(static_cast<size_t>(N.Id) + 1);
		}
		Roster[static_cast<size_t>(N.Id)] = std::move(N);
	}
	else if (Kind == "track")
	{
		if (Npc* N = Person(C.Int()))
		{
			ReadTracker(C, N->Stats);
		}
	}
	else if (Kind == "herotrack")
	{
		ReadTracker(C, HeroBook);
	}
	else if (Kind == "heroawards")
	{
		HeroTrophies = ReadAwards(C.At < Fields.size() ? Fields[C.At] : std::string());
	}
	else if (Kind == "awards")
	{
		if (Npc* N = Person(C.Int()))
		{
			N->Awards = ReadAwards(C.At < Fields.size() ? Fields[C.At] : std::string());
		}
	}
	else if (Kind == "recent" || Kind == "years" || Kind == "ties" || Kind == "tickets" || Kind == "path")
	{
		Npc* N = Person(C.Int());
		if (!N)
		{
			return true;
		}
		for (const std::string& R : Split(C.At < Fields.size() ? Fields[C.At] : std::string(), '|'))
		{
			const std::vector<std::string> V = Split(R, '~');
			Cursor Rc{V, 0};
			if (Kind == "recent")
			{
				world::Finish Fi;
				Fi.Day = Rc.Small();
				Fi.Event = Rc.Str();
				Fi.Place = Rc.Small();
				Fi.Entries = Rc.Small();
				Fi.Prize = Rc.Int();
				Fi.Where = Rc.Small();
				Fi.Major = Rc.Bool();
				Fi.Seat = Rc.Int();
				N->Recent.push_back(Fi);
			}
			else if (Kind == "years")
			{
				Year Y;
				Y.Number = Rc.Small();
				Y.Online = Rc.Int();
				Y.Live = Rc.Int();
				Y.Net = Rc.Int();
				Y.Wins = Rc.Small();
				Y.Events = Rc.Small();
				N->Years.push_back(Y);
			}
			else if (Kind == "ties")
			{
				Tie T;
				T.Other = Rc.Small();
				T.Kind = static_cast<TieKind>(Rc.Small());
				T.Strength = Rc.Flt();
				T.Since = Rc.Small();
				N->Ties.push_back(T);
			}
			else if (Kind == "path")
			{
				Step St;
				St.Day = Rc.Small();
				const int K = Rc.Small();
				St.Kind = K >= 0 && K < static_cast<int>(StepKind::Count) ? static_cast<StepKind>(K) : StepKind::Joined;
				St.What = Rc.Str();
				St.Place = Rc.Small();
				St.Of = Rc.Small();
				St.Amount = Rc.Int();
				N->Path.push_back(std::move(St));
			}
			else
			{
				const std::string Key = Rc.Str();
				N->Tickets[Key] = Rc.Small();
			}
		}
	}
	else if (Kind == "q")
	{
		Pending P;
		P.Id = C.Str();
		P.Name = C.Str();
		P.Online = C.Bool();
		P.Template = C.Small();
		P.Kind = C.Small();
		P.Start = C.Dbl();
		P.End = C.Dbl();
		P.Entries = C.Small();
		P.Pool = C.Int();
		P.BuyIn = C.Int();
		P.Tier = C.Small();
		P.Format = C.Small();
		P.TableSize = C.Small();
		P.Major = C.Bool();
		P.Bracelet = C.Bool();
		P.Ring = C.Bool();
		P.Series = C.Str();
		P.Ticket = C.Str();
		P.SeatValue = C.Int();
		P.Stakes = C.Int();
		P.HeroPlace = C.Small();
		P.HeroPrize = C.Int();
		P.Strength = C.Dbl();
		P.Luck = C.Dbl();
		P.Kinds = C.Small();
		P.Seats = C.Small();
		P.FinalSize = C.Small();
		for (const std::string& R : Split(C.At < Fields.size() ? Fields[C.At] : std::string(), '|'))
		{
			const std::vector<std::string> V = Split(R, '~');
			Cursor Rc{V, 0};
			Entry E;
			E.Npc = Rc.Small();
			E.Ticket = Rc.Bool();
			E.Staked = Rc.Bool();
			E.Bullets = Rc.Small();
			E.Share = Rc.Flt();
			E.Known = Rc.Small();
			E.Better = Rc.Small();
			P.Who.push_back(E);
		}
		Queue.push_back(std::move(P));
	}
	else if (Kind == "res")
	{
		const std::string Id = C.Str();
		const double End = C.Dbl();
		net::EventResult Res;
		for (const std::string& R : Split(C.At < Fields.size() ? Fields[C.At] : std::string(), '|'))
		{
			const std::vector<std::string> V = Split(R, '~');
			Cursor Rc{V, 0};
			net::Placing Pc;
			Pc.Player = Rc.Small();
			Pc.Place = Rc.Small();
			Pc.Prize = Rc.Int();
			Pc.Name = Rc.Str();
			Pc.Country = Rc.Str();
			Res.FinalTable.push_back(Pc);
		}
		Results[Id] = Res;
		ResultEnds[Id] = End;
	}
	else if (Kind == "log")
	{
		WorldEvent E;
		E.At = C.Dbl();
		E.Kind = static_cast<EventKind>(C.Small());
		E.Npc = C.Small();
		E.Other = C.Small();
		E.Amount = C.Int();
		E.What = C.Str();
		E.Value = C.Small();
		E.Flags = C.Small();
		Log.push_back(E);
	}
	else if (Kind == "honor")
	{
		Honor H;
		H.Year = C.Small();
		H.Title = C.Str();
		H.EventId = C.Str();
		H.Npc = C.Small();
		H.Name = C.Str();
		H.Prize = C.Int();
		H.Entries = C.Small();
		H.At = C.Dbl();
		Titles.push_back(H);
	}
	else if (Kind == "bond")
	{
		const int Id = C.Small();
		Bond B;
		B.Familiarity = C.Flt();
		B.Respect = C.Flt();
		B.Trust = C.Flt();
		B.Rivalry = C.Flt();
		B.Resentment = C.Flt();
		B.Encounters = C.Small();
		B.LastDay = C.Small();
		for (const std::string& R : Split(C.At < Fields.size() ? Fields[C.At] : std::string(), '|'))
		{
			const std::vector<std::string> V = Split(R, '~');
			Cursor Rc{V, 0};
			Memory M;
			M.Day = Rc.Small();
			M.Kind = static_cast<MemoryKind>(Rc.Small());
			M.Where = Rc.Str();
			M.Amount = Rc.Int();
			B.Memories.push_back(M);
		}
		HeroBonds[Id] = B;
	}
	else if (Kind == "heroresult")
	{
		const std::string Id = C.Str();
		const int Place = C.Small();
		HeroResults[Id] = {Place, C.Int()};
	}
	else if (Kind == "heroin")
	{
		HeroIn.insert(C.Str());
	}
	else if (Kind == "series" || Kind == "night" || Kind == "nightago" || Kind == "pairs")
	{
		for (const std::string& R : Split(C.At < Fields.size() ? Fields[C.At] : std::string(), '|'))
		{
			const std::vector<std::string> V = Split(R, '~');
			Cursor Rc{V, 0};
			const int A = Rc.Small();
			if (Kind == "pairs")
			{
				WeekPairs.push_back({A, Rc.Small()});
				continue;
			}
			std::map<int, double>& M = Kind == "series" ? SeriesPoints : Kind == "night" ? NightPoints : NightPointsHourAgo;
			M[A] = Rc.Dbl();
		}
	}
	return true;
}

void World::Finish()
{
	if (!Created)
	{
		return;
	}
	ByName.clear();
	for (size_t K = 0; K < Roster.size(); ++K)
	{
		Npc& N = Roster[K];
		N.Id = static_cast<int>(K);
		if (!N.Name.empty())
		{
			ByName[N.Name] = N.Id;
		}
	}
	// Saves from before the stats pages: the history so far is drawn from the lifetime numbers.
	for (Npc& N : Roster)
	{
		Sim::SeedStats(N);
	}
	// Saves from before the trophy case kept only counts: the titles in history say which, the rest stay plain.
	for (Npc& N : Roster)
	{
		int Bracelets = 0;
		int Rings = 0;
		for (const Award& A : N.Awards)
		{
			(A.Ring ? Rings : Bracelets) += 1;
		}
		if (Bracelets >= N.Bracelets && Rings >= N.Rings)
		{
			continue;
		}
		for (const Honor& H : Titles)
		{
			if (H.Npc != N.Id)
			{
				continue;
			}
			Award A;
			A.Day = sim::DayAt(H.At);
			A.Event = H.Title;
			A.Prize = H.Prize;
			A.Entries = H.Entries;
			const size_t At = H.EventId.find('@');
			const net::EventTemplate* T = net::Shared().FindTemplate(H.EventId.substr(0, At));
			if (T && (T->Bracelet || T->Ring))
			{
				A.Ring = T->Ring;
				A.Series = T->Series;
				const net::SeriesInfo* Sr = net::Shared().FindSeries(T->Series);
				A.Main = Sr && Sr->MainEvent == T->Id;
			}
			else if (!T && H.Title.rfind("The Championship ", 0) == 0)
			{
				A.Online = false;
				A.Series = H.Title.substr(0, H.Title.find(':'));
				A.Main = H.Title.find("Main Event") != std::string::npos;
			}
			else if (!T && H.Title.rfind("Grand Circuit ", 0) == 0)
			{
				A.Online = false;
				A.Ring = true;
				A.Series = H.Title.substr(0, H.Title.find(':'));
				A.Main = H.Title.find("Main Event") != std::string::npos;
			}
			else
			{
				continue;
			}
			int& Have = A.Ring ? Rings : Bracelets;
			if (Have < (A.Ring ? N.Rings : N.Bracelets))
			{
				++Have;
				N.Awards.push_back(A);
			}
		}
		for (; Bracelets < N.Bracelets; ++Bracelets)
		{
			N.Awards.push_back(Award());
		}
		for (; Rings < N.Rings; ++Rings)
		{
			Award A;
			A.Ring = true;
			N.Awards.push_back(A);
		}
		std::stable_sort(N.Awards.begin(), N.Awards.end(), [](const Award& A, const Award& B) { return A.Day < B.Day; });
	}
	std::stable_sort(Queue.begin(), Queue.end(), [](const Pending& A, const Pending& B) { return A.End < B.End; });
	RefreshAll();
	Sim::Ranks(*this);
}
} // namespace world
} // namespace ss
