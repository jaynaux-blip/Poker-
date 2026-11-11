// Saving the living world: "world" lines in the save, versioned. Everything the simulation depends on is
// written at full precision, so a world that is saved and loaded carries on exactly as it would have.
#include "ShortStack/Game/World.h"

#include "ShortStack/Game/Sites.h"
#include "WorldSim.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <type_traits>

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
	const auto Steps = [](const std::vector<Chips>& V) {
		std::string Out;
		long long Last = 0;
		for (size_t K = 0; K < V.size(); ++K)
		{
			const long long Dollars = (V[K] >= 0 ? V[K] + 50 : V[K] - 50) / 100;
			Out += (K > 0 ? "~" : "") + I(Dollars - Last);
			Last = Dollars;
		}
		return Out;
	};
	L << Steps(T.Curve) << Steps(T.Spend);
	if (T.Freerolls > 0)
	{
		// (Only a page with freerolls carries them: the world's people never play one, so their lines stay as they were.)
		L << I(T.Freerolls) << I(T.FreerollPrizes);
	}
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
	const auto Steps = [&](std::vector<Chips>& Out) {
		long long Run = 0;
		for (const std::string& Step : Split(C.At < C.V.size() ? C.V[C.At] : std::string(), '~'))
		{
			Run += std::strtoll(Step.c_str(), nullptr, 10);
			Out.push_back(static_cast<Chips>(Run * 100));
		}
		++C.At;
	};
	Steps(T.Curve);
	Steps(T.Spend);
	if (C.At < C.V.size())
	{
		T.Freerolls = C.Small();
		T.FreerollPrizes = C.Int();
	}
	if (T.Spend.size() != T.Curve.size())
	{
		// Pages saved before the ABI graph: their buy-ins spread evenly.
		T.Spend.clear();
		for (size_t K = 0; K < T.Curve.size(); ++K)
		{
			T.Spend.push_back(T.Events > 0 ? T.BuyIns * static_cast<Chips>((K + 1) * static_cast<size_t>(T.Stride)) / T.Events : 0);
		}
	}
}

/** world\troom: npc (-1: the player), venue, events, cashes, final tables, wins, the last win, the month, its points,
 *  the month before's, the days (pipe-separated). */
Line RoomLine(int Id, const RoomRecord& R)
{
	Line L("room");
	L << I(Id) << Esc(R.Venue) << I(R.Events) << I(R.Cashes) << I(R.FinalTables) << I(R.Wins) << I(R.LastWin) << I(R.MonthKey) << D(R.MonthPoints) << D(R.LastMonthPoints);
	std::string Days;
	for (size_t K = 0; K < R.Days.size(); ++K)
	{
		Days += (K > 0 ? "|" : "") + I(R.Days[K]);
	}
	L << Days;
	return L;
}

void ReadRoom(Cursor& C, RoomRecord& R)
{
	R = RoomRecord();
	R.Venue = C.Str();
	R.Events = C.Small();
	R.Cashes = C.Small();
	R.FinalTables = C.Small();
	R.Wins = C.Small();
	R.LastWin = C.Small();
	R.MonthKey = C.Small();
	R.MonthPoints = C.Dbl();
	R.LastMonthPoints = C.Dbl();
	for (const std::string& Day : Split(C.At < C.V.size() ? C.V[C.At] : std::string(), '|'))
	{
		R.Days.push_back(static_cast<int>(std::strtol(Day.c_str(), nullptr, 10)));
	}
	++C.At;
}

// ---- Update 4 (SPEC v1.7 §1.7, §2.13, §3.21, §5.12; §6 PA-26): the optional lines' helpers.

/** A fixed-width base-64 number (URL-safe alphabet: never a tab, a '|' or a '~'), Width characters, 6 bits each, high first. */
void SaveB64Put(std::string& Out, uint64_t V, int Width)
{
	static const char SaveB64Digits[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
	for (int K = Width - 1; K >= 0; --K)
	{
		Out += SaveB64Digits[(V >> (6 * K)) & 63u];
	}
}

/** The reverse of SaveB64Put from S at At (At moves on); a field cut short reads as 0 from there. */
uint64_t SaveB64Get(const std::string& S, size_t& At, int Width)
{
	uint64_t V = 0;
	for (int K = 0; K < Width; ++K)
	{
		uint64_t D = 0;
		if (At < S.size())
		{
			const char Ch = S[At++];
			D = Ch >= 'A' && Ch <= 'Z' ? static_cast<uint64_t>(Ch - 'A')
				: Ch >= 'a' && Ch <= 'z' ? static_cast<uint64_t>(Ch - 'a' + 26)
				: Ch >= '0' && Ch <= '9' ? static_cast<uint64_t>(Ch - '0' + 52)
				: Ch == '-' ? 62u
				: 63u;
		}
		V = (V << 6) | D;
	}
	return V;
}

/** npc:points:cashes records, comma-joined (a festival race's and its final's scorers: points as integers). */
std::string SaveFestScores(const std::vector<std::pair<int, FestScore>>& Rows)
{
	std::string Out;
	for (const std::pair<int, FestScore>& R : Rows)
	{
		Out += (Out.empty() ? "" : ",") + I(R.first) + ":" + I(std::llround(R.second.Points)) + ":" + I(R.second.Cashes);
	}
	return Out;
}

std::vector<std::pair<int, FestScore>> ReadFestScores(const std::string& Field)
{
	std::vector<std::pair<int, FestScore>> Out;
	for (const std::string& R : Split(Field, ','))
	{
		const std::vector<std::string> V = Split(R, ':');
		Cursor Rc{V, 0};
		std::pair<int, FestScore> Row;
		Row.first = Rc.Small();
		Row.second.Points = static_cast<double>(Rc.Int());
		Row.second.Cashes = Rc.Small();
		Out.push_back(Row);
	}
	return Out;
}

/** A crowd finish: prize~place~of~template~day~site~kind. */
std::string SaveCrowdFinish(const CrowdFinish& F)
{
	return Rec({I(F.Prize), I(F.Place), I(F.Of), I(F.Template), I(F.Day), I(F.Site), I(F.Kind)});
}

CrowdFinish ReadCrowdFinish(const std::string& R)
{
	const std::vector<std::string> V = Split(R, '~');
	Cursor Rc{V, 0};
	CrowdFinish F;
	F.Prize = Rc.Int();
	F.Place = static_cast<int32_t>(Rc.Int());
	F.Of = static_cast<int32_t>(Rc.Int());
	F.Template = static_cast<int32_t>(Rc.Int());
	F.Day = static_cast<uint16_t>(Rc.Int());
	F.Site = static_cast<uint8_t>(Rc.Int());
	F.Kind = static_cast<uint8_t>(Rc.Int());
	return F;
}

/**
 * world\tcrowd's packed field (§5.12, rev-code M6): every small number of a crowd record in a fixed order, each fixed-width base-64
 * (an 8-bit value 2 characters, 16-bit 3, 32-bit 6; signed values offset to unsigned), the in-memory values themselves, so a loaded
 * crowd plays on exactly. SaveCrowdSmalls and ReadCrowdSmalls walk the same list.
 */
template <typename F>
void SaveCrowdFields(CrowdRec& R, F&& Field)
{
	Field(R.Country, 2);
	Field(R.Came, 2);
	Field(R.Arrived, 3);
	Field(R.CameWith, 6);
	Field(R.Moment, 6);
	Field(R.Born, 3);
	Field(R.Overall, 3);
	Field(R.Peak, 3);
	Field(R.Potential, 3);
	for (uint8_t& T : R.Traits)
	{
		Field(T, 2);
	}
	Field(R.PhWhy, 2);
	Field(R.Until, 3);
	Field(R.BandSince, 3);
	Field(R.CoachUntil, 3);
	Field(R.BusyUntil, 3);
	Field(R.GrindCool, 3);
	Field(R.SeriousAt, 3);
	Field(R.Band, 2);
	Field(R.GrindRun, 2);
	Field(R.BirthPeak, 3);
	Field(R.BirthPot, 3);
	Field(R.GFlags, 3);
	Field(R.Plan, 2);
	Field(R.OnlineShare, 2);
	Field(R.Tier, 2);
	Field(R.Live, 2);
	Field(R.Sites, 2);
	Field(R.Flags, 2);
	Field(R.St, 2);
	Field(R.Mood, 2);
	Field(R.Fatigue, 2);
	Field(R.LeftWhy, 2);
	Field(R.TierSince, 3);
	Field(R.Left, 3);
	Field(R.BreakUntil, 3);
	Field(R.Events, 6);
	Field(R.Cashes, 6);
	Field(R.FinalTables, 6);
	Field(R.Wins, 6);
	Field(R.Now, 3);
	Field(R.Prev, 3);
	Field(R.BestRank, 3);
	Field(R.First, 3);
	Field(R.FirstDay, 3);
	Field(R.BestDay, 3);
	for (int16_t& M : R.Monthly)
	{
		Field(M, 3);
	}
	Field(R.Run100, 2);
	Field(R.Run1000, 2);
	Field(R.PlayMask, 6);
	Field(R.Met, 2);
	Field(R.LastMet, 3);
	for (CrowdMilestone& M : R.Mile)
	{
		Field(M.Day, 3);
		Field(M.Value, 3);
	}
	Field(R.WorldId, 6);
}

/** A value as the unsigned number its width holds (signed ones offset by half the range). */
template <typename T>
uint64_t SaveCrowdPack(T V, int Width)
{
	const int Bits = 6 * Width;
	if constexpr (std::is_signed<T>::value)
	{
		return static_cast<uint64_t>(static_cast<int64_t>(V) + (int64_t{1} << (Bits - 1)));
	}
	else
	{
		(void)Bits;
		return static_cast<uint64_t>(V);
	}
}

template <typename T>
T SaveCrowdUnpack(uint64_t U, int Width)
{
	const int Bits = 6 * Width;
	if constexpr (std::is_signed<T>::value)
	{
		return static_cast<T>(static_cast<int64_t>(U) - (int64_t{1} << (Bits - 1)));
	}
	else
	{
		(void)Bits;
		return static_cast<T>(U);
	}
}

std::string SaveCrowdSmalls(const CrowdRec& R)
{
	std::string Out;
	CrowdRec Copy = R;
	SaveCrowdFields(Copy, [&Out](auto& V, int Width) { SaveB64Put(Out, SaveCrowdPack(V, Width), Width); });
	return Out;
}

void ReadCrowdSmalls(const std::string& S, CrowdRec& R)
{
	size_t At = 0;
	SaveCrowdFields(R, [&](auto& V, int Width) {
		using T = typename std::remove_reference<decltype(V)>::type;
		V = SaveCrowdUnpack<T>(SaveB64Get(S, At, Width), Width);
	});
}

/** The 26 TPI weeks run-length coded: value*count pairs, comma-joined (a crowd record's mostly empty weeks stay short). */
std::string SaveCrowdWeeks(const std::array<uint16_t, 26>& W)
{
	std::string Out;
	for (size_t K = 0; K < W.size();)
	{
		size_t Run = 1;
		while (K + Run < W.size() && W[K + Run] == W[K])
		{
			++Run;
		}
		Out += (Out.empty() ? "" : ",") + I(W[K]) + (Run > 1 ? "*" + I(static_cast<long long>(Run)) : std::string());
		K += Run;
	}
	return Out;
}

void ReadCrowdWeeks(const std::string& Field, std::array<uint16_t, 26>& W)
{
	W = {};
	size_t At = 0;
	for (const std::string& Part : Split(Field, ','))
	{
		const size_t Star = Part.find('*');
		const uint16_t V = static_cast<uint16_t>(std::strtol(Part.c_str(), nullptr, 10));
		const long Run = Star == std::string::npos ? 1 : std::strtol(Part.c_str() + Star + 1, nullptr, 10);
		for (long K = 0; K < Run && At < W.size(); ++K)
		{
			W[At++] = V;
		}
	}
}

/** The TPI's 26 weeks, 3 base-64 characters each (16 bits), trailing zero weeks trimmed (§3.21). */
std::string SaveTpiWeeks(const std::array<uint16_t, 26>& W)
{
	size_t Held = 0;
	for (size_t K = 0; K < W.size(); ++K)
	{
		Held = W[K] != 0 ? K + 1 : Held;
	}
	std::string Out;
	for (size_t K = 0; K < Held; ++K)
	{
		SaveB64Put(Out, W[K], 3);
	}
	return Out;
}

void ReadTpiWeeks(const std::string& Field, std::array<uint16_t, 26>& W)
{
	W = {};
	size_t At = 0;
	for (size_t K = 0; K < W.size() && At < Field.size(); ++K)
	{
		W[K] = static_cast<uint16_t>(SaveB64Get(Field, At, 3));
	}
}

/** Numbers comma-joined (the scene's rings, the hero's 24 monthly ranks). */
template <typename T, size_t N>
std::string SaveJoined(const std::array<T, N>& V)
{
	std::string Out;
	for (size_t K = 0; K < N; ++K)
	{
		Out += (K > 0 ? "," : "") + I(static_cast<long long>(V[K]));
	}
	return Out;
}

template <typename T, size_t N>
void ReadJoined(const std::string& Field, std::array<T, N>& V)
{
	V = {};
	size_t K = 0;
	for (const std::string& Part : Split(Field, ','))
	{
		if (K < N)
		{
			V[K++] = static_cast<T>(std::strtoll(Part.c_str(), nullptr, 10));
		}
	}
}
} // namespace worldsave_detail

using namespace worldsave_detail;

void World::Write(std::string& Out) const
{
	// Update 4 (SPEC v1.7 §3.7 R3, §6 PA-35): the HoldGuard (A3; World.h's A3 block): not while a turn is held, outside a drain.
	HoldGuard("Write");
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
		L << D(HeroCircuit) << I(HeroCardTop) << I(HeroCardPrev); // version 2
		// Version 3 (update 2 §3.8): how the world began. Until the version moves to 3, only a fresh start writes it (an
		// established world's text stays exactly what it was; a head without it reads Established).
		bool WithStart = Mode != Start::Established;
		if constexpr (Version >= 3)
		{
			WithStart = true;
		}
		if (WithStart)
		{
			L << I(static_cast<int>(Mode));
		}
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
		// Version 4 (update 2 §5.9): the sites they play, appended (read by the cursor, so an older line reads RiverLine's
		// alone). Before the version moves to 4, only someone who plays elsewhere writes it, so a world the sites never
		// reached writes back exactly as it was read.
		bool WithSites = N.Sites != 1;
		if constexpr (Version >= 4)
		{
			WithSites = true;
		}
		if (WithSites)
		{
			L << I(N.Sites);
			// Update 4 (SPEC v1.7 §3.21, §5.12; §6 PA-26): the book's moment, why they left, and G4's Noticed and crowd serial, trailing
			// zeros omitted (so every line written before them stays byte for byte as it was).
			const long long Appended[4] = {N.Moment, N.LeftWhy, N.Noticed, static_cast<long long>(N.CrowdSerial)};
			size_t Held = 0;
			for (size_t K = 0; K < 4; ++K)
			{
				Held = Appended[K] != 0 ? K + 1 : Held;
			}
			for (size_t K = 0; K < Held; ++K)
			{
				L << I(Appended[K]);
			}
		}
		Emit(L);
		if (!N.Recent.empty())
		{
			std::vector<std::string> R;
			for (const world::Finish& Fi : N.Recent)
			{
				// Update 4 (SPEC v1.7 §2.4.3; §6 PA-14): a 9th field, the curve, only when it isn't Standard (old records unchanged).
				R.push_back(Fi.Curve == PayoutCurve::Standard ? Rec({I(Fi.Day), Esc(Fi.Event), I(Fi.Place), I(Fi.Entries), I(Fi.Prize), I(Fi.Where), I(Fi.Major), I(Fi.Seat)})
															   : Rec({I(Fi.Day), Esc(Fi.Event), I(Fi.Place), I(Fi.Entries), I(Fi.Prize), I(Fi.Where), I(Fi.Major), I(Fi.Seat),
																	 I(static_cast<int>(Fi.Curve))}));
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
		// Version 2: the room, the circuit, the review (only the people with something to keep).
		if (N.Room.Events > 0)
		{
			Emit(RoomLine(N.Id, N.Room));
		}
		if (N.CircuitPoints != 0.0 || N.CardBest != CardNone || N.CardLast != CardNone || N.Verified || N.ReviewMonth != -1)
		{
			Emit(Line("badge") << I(N.Id) << D(N.CircuitPoints) << I(N.CardBest) << I(N.CardLast) << I(N.Verified) << I(N.ReviewMonth));
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
	if (HeroRoomRec.Events > 0)
	{
		Emit(RoomLine(-1, HeroRoomRec));
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
		if (!P.KnownFt.empty())
		{
			// Update 4 (SPEC v1.7 §4.10.2, §4.16; §6 PA-26): the play-on's final table, place~name~country~prize~npc records, only while the
			// event waits to resolve (the house record encoding, so a name can't break the line).
			std::vector<std::string> Ft;
			for (const KnownSeat& S : P.KnownFt)
			{
				Ft.push_back(Rec({I(S.Place), Esc(S.Name), Esc(S.Country), I(S.Prize), I(S.Npc)}));
			}
			L << JoinRecords(Ft);
		}
		Emit(L);
	}
	// The last couple of days' results, and the big ones (older small results aren't needed to carry on).
	auto Kept = [&](const std::string& Id) {
		const auto End = ResultEnds.find(Id);
		const auto Res = Results.find(Id);
		const bool Recent = End == ResultEnds.end() || End->second >= Now - 3.0 * 1440.0;
		const bool Big = Res != Results.end() && !Res->second.FinalTable.empty() && Res->second.FinalTable.front().Prize >= 2500000;
		return Recent || Big;
	};
	for (const auto& It : Results)
	{
		const auto End = ResultEnds.find(It.first);
		if (!Kept(It.first))
		{
			continue;
		}
		Line L("res");
		L << Esc(It.first) << D(End == ResultEnds.end() ? Now : End->second);
		std::vector<std::string> R;
		for (const net::Placing& P : It.second.FinalTable)
		{
			// (Update 4, §5.12, §6 PA-26: a crowd seat's serial appended, only when non-zero.)
			R.push_back(P.Crowd != 0 ? Rec({I(P.Player), I(P.Place), I(P.Prize), Esc(P.Name), Esc(P.Country), I(P.Crowd)})
									 : Rec({I(P.Player), I(P.Place), I(P.Prize), Esc(P.Name), Esc(P.Country)}));
		}
		L << JoinRecords(R);
		Emit(L);
	}
	// Where the people the world follows finished in the player's live events, with those events' results.
	for (const auto& It : FieldPlaces)
	{
		if (Results.count(It.first) == 0 || !Kept(It.first))
		{
			continue;
		}
		Line L("fieldplaces");
		L << Esc(It.first);
		std::vector<std::string> R;
		for (const Placed& P : It.second)
		{
			R.push_back(Rec({I(P.Npc), I(P.Place), I(P.Prize)}));
		}
		L << JoinRecords(R);
		Emit(L);
	}
	for (const auto& It : Cancelled)
	{
		Emit(Line("cancelled") << Esc(It.first) << D(It.second.At) << I(It.second.Field));
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
		// Update 4 (SPEC v1.7 §3.21; §6 PA-26): the hero's notes and PASSED YOU, after the memories, only when any is non-zero.
		const int Notes[11] = {B.NoteDay0, B.N1Hands, B.N1Vpip, B.N1Pfr, B.NoteDayR, B.NrHands, B.NrVpip, B.NrPfr, B.ArmHero, B.ArmThem, B.PassedDay};
		if (std::any_of(std::begin(Notes), std::end(Notes), [](int V) { return V != 0; }))
		{
			for (const int V : Notes)
			{
				L << I(V);
			}
		}
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
	// A fresh world's first four weeks (update 2 §3.3 F9): the Season board's ranks at the day's turn, which the board report
	// reads its biggest riser against, so a world loaded in the middle of a day reports the same morning as one that wasn't.
	const std::vector<int>& Season = RanksYesterday[static_cast<size_t>(net::Board::Season)];
	if (Mode == Start::Blank && sim::DayAt(Now) - Sim::BooksOpened(*this) <= 28)
	{
		std::vector<std::string> R;
		for (size_t K = 0; K < Season.size(); ++K)
		{
			if (Season[K] > 0)
			{
				R.push_back(Rec({I(static_cast<long long>(K)), I(Season[K])}));
			}
		}
		// (Written even when nobody had points at the last turn, night one: a world loaded then must not measure the
		// morning's riser from the minute it was loaded.)
		Emit(Line("seasonranks") << JoinRecords(R));
	}
	// Version 4 (update 2 §5.9, §6.4): the rival sites. When each opened in this world; the sparse records (only the people
	// who played there); each rival's running race (RunItTwice's series, Ignite's month). A world the sites never reached has
	// none of these.
	for (const auto& It : SiteLaunches)
	{
		Emit(Line("sitelaunch") << I(static_cast<int>(It.first)) << D(It.second));
	}
	for (const auto& Site : SiteRecords)
	{
		for (const auto& It : Site.second)
		{
			const SiteRecord& Sr = It.second;
			Emit(Line("site") << I(static_cast<int>(Site.first)) << I(It.first) << D(Sr.Points) << I(Sr.Won) << I(Sr.Wins) << I(Sr.FinalTables) << I(Sr.Events)
							  << I(Sr.Lifetime));
		}
	}
	for (const auto& Site : SiteSeries)
	{
		for (const auto& It : Site.second.second)
		{
			Emit(Line("siteseries") << I(static_cast<int>(Site.first)) << Esc(Site.second.first) << I(It.first) << D(It.second));
		}
	}
	// Update 3 (§3.6 R2, §7.4): the player's League group on a site, only once one was drawn (optional: no version bump, and
	// an older build drops the line). The 19 ids (-1 for none), then each member as npc:days:form:todayKey:todayCount and
	// its best days as day/points x 10 (up to the last one held), the members comma-joined.
	for (const auto& Lg : Leagues)
	{
		const LeagueBook& Lb = Lg.second;
		Line L("league");
		L << I(static_cast<int>(Lg.first)) << I(Lb.MonthKey) << I(Lb.Div);
		for (size_t K = 0; K < 19; ++K)
		{
			L << I(K < Lb.Members.size() ? Lb.Members[K].Npc : -1);
		}
		std::string Members;
		for (const LeagueMember& M : Lb.Members)
		{
			Members += (Members.empty() ? "" : ",") + I(M.Npc) + ":" + I(M.Days) + ":" + I(M.Form) + ":" + I(M.TodayKey) + ":" + I(M.TodayCount);
			size_t Held = 0;
			for (size_t K = 0; K < M.Best.size(); ++K)
			{
				Held = M.Best[K].first != 0 || M.Best[K].second != 0 ? K + 1 : Held;
			}
			for (size_t K = 0; K < Held; ++K)
			{
				Members += ":" + I(M.Best[K].first) + "/" + I(M.Best[K].second);
			}
		}
		Emit(L << Members);
	}
	// Update 4 (SPEC v1.7 §1.7, §2.13, §3.21, §5.12; §6 PA-26): the optional lines, after the league's. Each is written only when it
	// has something to keep and never from Finish; Read reads each when present and ignores a sub-kind it doesn't know. Nothing has
	// anything to keep before the gates, so every save writes as it did at Phase 0.
	if (!Fest.Key.empty())
	{
		// The live festival's race: its key, the scorers npc:points:cashes (the hero as -1), and G4's crowd scorers name~points~cashes.
		Line L("fest");
		L << u4line::FestRace << Esc(Fest.Key);
		L << SaveFestScores(std::vector<std::pair<int, FestScore>>(Fest.Scores.begin(), Fest.Scores.end()));
		if (!Fest.CrowdScores.empty())
		{
			std::vector<std::string> R;
			for (const auto& It : Fest.CrowdScores)
			{
				R.push_back(Rec({Esc(It.first), I(std::llround(It.second.Points)), I(It.second.Cashes)}));
			}
			L << JoinRecords(R);
		}
		Emit(L);
	}
	if (!FestLast.Key.empty())
	{
		// The last crown, through the Afterglow: key, the crown's minute, the scorers, the top ten.
		Emit(Line("fest") << u4line::FestFinal << Esc(FestLast.Key) << D(FestLast.At) << I(FestLast.Scorers) << SaveFestScores(FestLast.Top));
	}
	for (const HeroFestMark& M : HeroFest)
	{
		// One per edition the hero scored in; a trailing t while its crown ticket is unpaid.
		Line L("fest");
		L << u4line::FestMarkLine << Esc(M.Key) << I(static_cast<int>(M.Mark)) << I(M.Rank) << I(M.Scorers) << D(M.Points);
		if (M.Ticket)
		{
			L << std::string("t");
		}
		Emit(L);
	}
	if (Scene.AnchorDay >= 0)
	{
		// The Scene Book's keeper, once the book is open (the TPI's book rides on its tail).
		Line L(u4line::Scene);
		L << I(Scene.AnchorDay) << D(Scene.A0) << I(Scene.CapNow) << D(Scene.Scale) << D(Scene.Shift) << I(Scene.LastSummer) << I(Scene.WeekArrivals)
		  << I(Scene.NextClosure) << I(Scene.ReachDone) << I(Scene.TitleWaveUntil) << SaveJoined(Scene.HeroTrail) << SaveJoined(Scene.WeekRing);
		for (size_t T = 1; T < Scene.TierRef.size(); ++T)
		{
			L << D(Scene.TierRef[T]);
		}
		L << D(Scene.AvgAtAnchor) << D(Scene.SteadyScale);
		for (const int Md : Tpi.MonthlyDay)
		{
			L << I(Md);
		}
		L << I(Tpi.LastWeek) << I(Tpi.RankedCount);
		Emit(L);
	}
	for (const SceneMoment& M : Scene.Moments)
	{
		Emit(Line(u4line::Moment) << I(M.Id) << I(static_cast<int>(M.Kind)) << I(M.Day) << I(M.Start) << I(M.End) << F(M.Pull) << D(M.Carry) << I(M.Emitted) << I(M.Npc)
								  << Esc(M.What) << Esc(M.Key) << I(M.Site) << I(M.Region) << I(M.Flags) << F(M.Half) << I(M.Mix));
	}
	for (const Npc& N : Roster)
	{
		if (N.Grow.Default())
		{
			continue;
		}
		// A Growth record, trailing defaults omitted.
		const Growth& G = N.Grow;
		const Growth Def;
		std::vector<std::string> V = {I(static_cast<int>(G.Ph)), I(static_cast<int>(G.Why)), I(G.Until), I(G.Coach), I(G.CoachUntil), I(G.BusyUntil), I(G.GrindCool),
			I(G.TiltCool), I(G.GrindRun), I(G.TiltRun), I(G.Band), I(G.BandSince), F(G.BirthPeak), F(G.BirthPot), I(G.Flags), I(G.SeriousAt), F(G.SeriousTop)};
		const std::vector<std::string> Defaults = {I(static_cast<int>(Def.Ph)), I(static_cast<int>(Def.Why)), I(Def.Until), I(Def.Coach), I(Def.CoachUntil),
			I(Def.BusyUntil), I(Def.GrindCool), I(Def.TiltCool), I(Def.GrindRun), I(Def.TiltRun), I(Def.Band), I(Def.BandSince), F(Def.BirthPeak), F(Def.BirthPot),
			I(Def.Flags), I(Def.SeriousAt), F(Def.SeriousTop)};
		size_t Held = 0;
		for (size_t K = 0; K < V.size(); ++K)
		{
			Held = V[K] != Defaults[K] ? K + 1 : Held;
		}
		Line L(u4line::Grow);
		L << I(N.Id);
		for (size_t K = 0; K < Held; ++K)
		{
			L << V[K];
		}
		Emit(L);
	}
	auto TpiLine = [&](int Id, const TpiRec& T, bool Hero) {
		const bool AnyWeek = std::any_of(T.Week.begin(), T.Week.end(), [](uint16_t X) { return X != 0; });
		const bool AnyHist = Hero && std::any_of(HeroHist.begin(), HeroHist.end(), [](int16_t X) { return X != 0; });
		if (!T.Ranked() && !AnyWeek && !AnyHist)
		{
			return;
		}
		Line L(u4line::Tpi);
		L << I(Id) << I(T.Now) << I(T.Prev) << I(T.Best) << I(T.BestDay) << I(T.First) << I(T.FirstDay) << SaveJoined(T.Monthly) << I(T.Run100) << I(T.Run1000)
		  << I(T.PlayMask) << SaveTpiWeeks(T.Week);
		if (Hero)
		{
			L << SaveJoined(HeroHist);
		}
		Emit(L);
	};
	for (const Npc& N : Roster)
	{
		TpiLine(N.Id, N.Tpi, false);
	}
	TpiLine(-1, HeroTpi, true);
	for (const Arc& A : Arcs.Arcs)
	{
		std::vector<std::string> B;
		for (size_t K = 0; K < A.BeatCount && K < A.Beats.size(); ++K)
		{
			const ArcBeat& Bt = A.Beats[K];
			B.push_back(Rec({I(Bt.Day), I(static_cast<int>(Bt.Kind)), I(Bt.Value), I(Bt.Ref), I(Bt.Surfaced)}));
		}
		Emit(Line(u4line::ArcLine) << I(A.Id) << I(A.Npc) << I(static_cast<int>(A.Kind)) << I(A.Began) << I(static_cast<int>(A.State)) << F(A.Score) << I(A.Other)
								   << I(A.ClosedDay) << I(A.BestRank) << I(A.BestDay) << JoinRecords(B));
	}
	{
		const bool AnyWatch = std::any_of(Watch.begin(), Watch.end(), [](const WatchPick& P) { return P.Npc >= 0; });
		if (!Follows.empty() || AnyWatch || !Feed.empty() || Tip.Npc >= 0)
		{
			// Follows npc~since~h0..h23, the twelve Names to Watch npc~why~day, the feed day~arc~beat, Dee's tip npc~day~closed.
			std::string Fo;
			for (const FollowRec& F0 : Follows)
			{
				std::string R = I(F0.Npc) + "~" + I(F0.Since);
				for (const int16_t H : F0.Hist)
				{
					R += "~" + I(H);
				}
				Fo += (Fo.empty() ? "" : ",") + R;
			}
			std::string Wa;
			for (size_t K = 0; K < Watch.size(); ++K)
			{
				Wa += (K > 0 ? "," : "") + Rec({I(Watch[K].Npc), I(Watch[K].Why), I(Watch[K].Day)});
			}
			std::string Fe;
			for (const FeedItem& It : Feed)
			{
				Fe += (Fe.empty() ? "" : ",") + Rec({I(It.Day), I(It.ArcId), I(It.Beat)});
			}
			Emit(Line(u4line::Follow) << Fo << Wa << Fe << Rec({I(Tip.Npc), I(Tip.Day), I(Tip.Closed)}));
		}
	}
	if (!Regulars.empty())
	{
		Line L(u4line::Regulars);
		for (const int Id : Regulars)
		{
			L << I(Id);
		}
		Emit(L);
	}
	for (int K = 0; K < Crowd.Count; ++K)
	{
		const CrowdRec& R = Crowd.Get(K);
		if (R.WorldId >= 0)
		{
			continue; // a tombstone (promoted: the full record carries them now)
		}
		std::vector<std::string> Fin = {SaveCrowdFinish(R.Best)};
		for (const CrowdFinish& Fi : R.Last)
		{
			Fin.push_back(SaveCrowdFinish(Fi));
		}
		Emit(Line(u4line::Crowd) << I(R.Serial) << Esc(std::string(R.Name, strnlen(R.Name, sizeof(R.Name)))) << SaveCrowdSmalls(R) << I(R.Bankroll) << I(R.PeakRoll)
								 << I(R.Income) << I(R.Followers) << I(R.Won) << I(R.Spent) << F(R.Season) << F(R.Notice) << JoinRecords(Fin) << SaveCrowdWeeks(R.Week));
	}
	if (Crowd.Count > 0)
	{
		// The crowd's day: its anonymous entries per (site, tier), so a world loaded mid-day names the same seats.
		Line L(u4line::Crowd);
		L << u4line::CrowdDay << I(CrowdToday.Day);
		std::string A;
		for (size_t K = 0; K < CrowdToday.Anon.size(); ++K)
		{
			A += (K > 0 ? "," : "") + D(CrowdToday.Anon[K]);
		}
		Emit(L << A);
	}
	for (const Pending& P : Queue)
	{
		if (P.CrowdIn.empty())
		{
			continue;
		}
		std::vector<std::string> R;
		for (const CrowdEntry& E : P.CrowdIn)
		{
			R.push_back(Rec({I(E.Serial), I(E.Known), I(E.Better), I(E.Bullets)}));
		}
		Emit(Line(u4line::QCrowd) << Esc(P.Id) << JoinRecords(R));
	}
}

void World::Write(SharedText& Out) const
{
	// Update 4 (§6 PA-34): the save's shared text, appended to copy-on-write (a copy someone kept never changes).
	Write(Out.Edit());
}

bool World::Read(const std::vector<std::string>& Fields)
{
	if (Fields.size() < 2 || Fields[0] != "world")
	{
		return false;
	}
	const std::string& Kind = Fields[1];
	Cursor C{Fields, 2};
	auto Person = [&](long long Id) -> Npc* { return Id >= 0 && static_cast<size_t>(Id) < Roster.size() ? &Edit(static_cast<int>(Id)) : nullptr; };
	if (Kind == "head")
	{
		LoadedVersion = C.Small(); // an older world migrates in Finish (Sim::Migrate)
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
		// Version 2 (a version 1 head has none of these: the cursor gives 0).
		HeroCircuit = C.Dbl();
		HeroCardTop = C.Small();
		HeroCardPrev = C.Small();
		// Version 3: how the world began (an older head has none: the cursor gives 0, Established; so does anything unknown).
		Mode = C.Small() == static_cast<int>(Start::Blank) ? Start::Blank : Start::Established;
		Created = true;
		Instance = sim::NextWorldSerial(); // Update 4 (§6 PA-34): a loaded world's stamps are its own
		// What only a session keeps (who played what lately, who was bumped) starts over with the world it reads, and so do
		// the boards' ranks a day ago (Finish works them out; a fresh world's first weeks read the Season's back).
		Played.clear();
		Bumps.clear();
		for (std::vector<int>& Rk : RanksYesterday)
		{
			Rk.clear();
		}
		SiteRanksYesterday.clear();
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
		// Version 4: the sites they play (an older line has none: RiverLine's alone).
		const int Sites = C.Small();
		N.Sites = static_cast<uint8_t>(Sites > 0 && Sites < 256 ? Sites : 1);
		// Update 4 (§6 PA-26): appended after Sites (a line without them reads 0, as every line before them).
		N.Moment = C.Small();
		N.LeftWhy = static_cast<int8_t>(C.Small());
		N.Noticed = C.Small();
		N.CrowdSerial = static_cast<uint32_t>(C.Int());
		if (N.CrowdSerial != 0)
		{
			Crowd.NextSerial = std::max(Crowd.NextSerial, N.CrowdSerial + 1); // a promoted record's serial is never reused
		}
		if (N.Id < 0 || N.Id > 1000000)
		{
			return true;
		}
		if (Roster.size() <= static_cast<size_t>(N.Id))
		{
			// Update 4 (§6 PA-33): reserve ahead (the ids come in order, so the page table grows a few times, not once a line).
			Roster.reserve(std::max(Roster.size() * 2, static_cast<size_t>(N.Id) + 1));
			Roster.resize(static_cast<size_t>(N.Id) + 1);
		}
		Edit(N.Id) = std::move(N);
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
			Roster.reserve(std::max(Roster.size() * 2, static_cast<size_t>(N.Id) + 1));
			Roster.resize(static_cast<size_t>(N.Id) + 1);
		}
		Edit(N.Id) = std::move(N);
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
	else if (Kind == "room")
	{
		const long long Id = C.Int();
		if (Id == -1)
		{
			ReadRoom(C, HeroRoomRec);
		}
		else if (Npc* N = Person(Id))
		{
			ReadRoom(C, N->Room);
		}
	}
	else if (Kind == "badge")
	{
		if (Npc* N = Person(C.Int()))
		{
			N->CircuitPoints = C.Dbl();
			N->CardBest = C.Small();
			N->CardLast = C.Small();
			N->Verified = C.Bool();
			N->ReviewMonth = C.Small();
		}
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
				Fi.Curve = Rc.Small() == static_cast<int>(PayoutCurve::Main) ? PayoutCurve::Main : PayoutCurve::Standard; // the 9th, when present
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
		// Update 4 (§6 PA-26): the trailing ft records, when present.
		++C.At;
		for (const std::string& R : Split(C.At < Fields.size() ? Fields[C.At] : std::string(), '|'))
		{
			const std::vector<std::string> V = Split(R, '~');
			Cursor Rc{V, 0};
			KnownSeat S;
			S.Place = Rc.Small();
			S.Name = Rc.Str();
			S.Country = Rc.Str();
			S.Prize = Rc.Int();
			S.Npc = Rc.Small();
			P.KnownFt.push_back(S);
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
			Pc.Crowd = static_cast<uint32_t>(Rc.Int()); // Update 4 (§6 PA-26): 0 when absent
			Res.FinalTable.push_back(Pc);
		}
		Results[Id] = Res;
		ResultEnds[Id] = End;
	}
	else if (Kind == "fieldplaces")
	{
		std::vector<Placed>& Out = FieldPlaces[C.Str()];
		Out.clear();
		for (const std::string& R : Split(C.At < Fields.size() ? Fields[C.At] : std::string(), '|'))
		{
			const std::vector<std::string> V = Split(R, '~');
			Cursor Rc{V, 0};
			Placed P;
			P.Npc = Rc.Small();
			P.Place = Rc.Small();
			P.Prize = Rc.Int();
			Out.push_back(P);
		}
	}
	else if (Kind == "cancelled")
	{
		const std::string Id = C.Str();
		Cancellation Cn;
		Cn.At = C.Dbl();
		Cn.Field = C.Small();
		Cancelled[Id] = Cn;
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
		// Update 4 (§6 PA-26): past the memories field (it was read in place), then the appended note fields (0 when absent).
		++C.At;
		B.NoteDay0 = C.Small();
		B.N1Hands = C.Small();
		B.N1Vpip = C.Small();
		B.N1Pfr = C.Small();
		B.NoteDayR = C.Small();
		B.NrHands = C.Small();
		B.NrVpip = C.Small();
		B.NrPfr = C.Small();
		B.ArmHero = C.Small();
		B.ArmThem = C.Small();
		B.PassedDay = C.Small();
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
	else if (Kind == "sitelaunch" || Kind == "site" || Kind == "siteseries")
	{
		// Version 4 (update 2 §5.9): a rival's launch, someone's record there, their points in its running race.
		const int Raw = C.Small();
		if (Raw <= 0 || Raw >= static_cast<int>(net::SiteId::Count))
		{
			return true; // RiverLine keeps no rows; a site this build doesn't know is dropped
		}
		const net::SiteId Site = static_cast<net::SiteId>(Raw);
		if (Kind == "sitelaunch")
		{
			SiteLaunches[Site] = C.Dbl();
		}
		else if (Kind == "site")
		{
			const int Id = C.Small();
			if (Id < 0 || Id > 1000000)
			{
				return true;
			}
			SiteRecord& Sr = SiteRecords[Site][Id];
			Sr.Points = C.Dbl();
			Sr.Won = C.Int();
			Sr.Wins = C.Small();
			Sr.FinalTables = C.Small();
			Sr.Events = C.Small();
			Sr.Lifetime = C.Int();
		}
		else
		{
			std::pair<std::string, std::map<int, double>>& Race = SiteSeries[Site];
			Race.first = C.Str();
			const int Id = C.Small();
			if (Id >= 0 && Id <= 1000000)
			{
				Race.second[Id] = C.Dbl();
			}
		}
	}
	else if (Kind == "league")
	{
		// Update 3 (§3.6 R2): the player's League group on a site (World::Write's line; core-world's records from G2).
		const int Raw = C.Small();
		if (Raw <= 0 || Raw >= static_cast<int>(net::SiteId::Count))
		{
			return true;
		}
		LeagueBook& Lb = Leagues[static_cast<net::SiteId>(Raw)];
		Lb = LeagueBook();
		Lb.MonthKey = C.Small();
		Lb.Div = static_cast<int8_t>(C.Small());
		for (int K = 0; K < 19; ++K)
		{
			const int Who = C.Small();
			if (Who >= 0)
			{
				LeagueMember M;
				M.Npc = Who;
				Lb.Members.push_back(M);
			}
		}
		for (const std::string& Entry : Split(C.At < Fields.size() ? Fields[C.At] : std::string(), ','))
		{
			const std::vector<std::string> V = Split(Entry, ':');
			Cursor Rc{V, 0};
			const int Who = Rc.Small();
			for (LeagueMember& M : Lb.Members)
			{
				if (M.Npc != Who)
				{
					continue;
				}
				M.Days = static_cast<int8_t>(Rc.Small());
				M.Form = static_cast<uint16_t>(Rc.Small());
				M.TodayKey = Rc.Small();
				M.TodayCount = static_cast<int8_t>(Rc.Small());
				for (size_t K = 0; K < M.Best.size() && Rc.At < V.size(); ++K)
				{
					const std::string& Pair = V[Rc.At++];
					const size_t Slash = Pair.find('/');
					M.Best[K].first = static_cast<int8_t>(std::strtol(Pair.c_str(), nullptr, 10));
					M.Best[K].second = static_cast<int16_t>(Slash == std::string::npos ? 0 : std::strtol(Pair.c_str() + Slash + 1, nullptr, 10));
				}
				break;
			}
		}
	}
	else if (Kind == "seasonranks")
	{
		// (Finish keeps them for a fresh world's board report: the ranks at the last day's turn, not at the load. Everyone
		// unlisted had nothing on the board then: rank 0, even when nobody is listed.)
		std::vector<int>& Season = RanksYesterday[static_cast<size_t>(net::Board::Season)];
		Season.assign(std::max<size_t>(Roster.size(), 1), 0);
		for (const std::string& R : Split(C.At < Fields.size() ? Fields[C.At] : std::string(), '|'))
		{
			const std::vector<std::string> V = Split(R, '~');
			Cursor Rc{V, 0};
			const int Id = Rc.Small();
			const int Rank = Rc.Small();
			if (Id >= 0 && Id < 1000000)
			{
				if (Season.size() <= static_cast<size_t>(Id))
				{
					Season.resize(static_cast<size_t>(Id) + 1, 0);
				}
				Season[static_cast<size_t>(Id)] = Rank;
			}
		}
	}
	// Update 4 (SPEC v1.7 §6 PA-26): the optional lines (written only when they have something to keep; an unknown sub-kind is
	// ignored, as an unknown kind always was).
	else if (Kind == u4line::Fest)
	{
		const std::string Sub = C.Str();
		if (Sub == u4line::FestRace)
		{
			Fest = FestRace();
			Fest.Key = C.Str();
			for (const std::pair<int, FestScore>& R : ReadFestScores(C.At < Fields.size() ? Fields[C.At] : std::string()))
			{
				Fest.Scores[R.first] = R.second;
			}
			++C.At;
			for (const std::string& R : Split(C.At < Fields.size() ? Fields[C.At] : std::string(), '|'))
			{
				const std::vector<std::string> V = Split(R, '~');
				Cursor Rc{V, 0};
				const std::string Name = Rc.Str();
				FestScore S;
				S.Points = static_cast<double>(Rc.Int());
				S.Cashes = Rc.Small();
				Fest.CrowdScores[Name] = S;
			}
		}
		else if (Sub == u4line::FestFinal)
		{
			FestLast = FestFinal();
			FestLast.Key = C.Str();
			FestLast.At = C.Dbl();
			FestLast.Scorers = C.Small();
			FestLast.Top = ReadFestScores(C.At < Fields.size() ? Fields[C.At] : std::string());
		}
		else if (Sub == u4line::FestMarkLine)
		{
			HeroFestMark M;
			M.Key = C.Str();
			const int Mark = C.Small();
			M.Mark = Mark > 0 && Mark < static_cast<int>(FestMark::Count) ? static_cast<FestMark>(Mark) : FestMark::None;
			M.Rank = C.Small();
			M.Scorers = C.Small();
			M.Points = C.Dbl();
			M.Ticket = C.Str() == "t";
			HeroFest.push_back(M);
		}
	}
	else if (Kind == u4line::Scene)
	{
		Scene.AnchorDay = C.Small();
		Scene.A0 = C.Dbl();
		Scene.CapNow = C.Small();
		Scene.Scale = C.Dbl();
		Scene.Shift = C.Dbl();
		Scene.LastSummer = C.Small();
		Scene.WeekArrivals = C.Small();
		Scene.NextClosure = C.Small();
		Scene.ReachDone = static_cast<uint32_t>(C.Int());
		Scene.TitleWaveUntil = C.Small();
		ReadJoined(C.At < Fields.size() ? Fields[C.At] : std::string(), Scene.HeroTrail);
		++C.At;
		ReadJoined(C.At < Fields.size() ? Fields[C.At] : std::string(), Scene.WeekRing);
		++C.At;
		for (size_t T = 1; T < Scene.TierRef.size(); ++T)
		{
			Scene.TierRef[T] = C.Dbl();
		}
		Scene.AvgAtAnchor = C.Dbl();
		Scene.SteadyScale = C.Dbl();
		for (int& Md : Tpi.MonthlyDay)
		{
			Md = C.Small();
		}
		Tpi.LastWeek = C.Small();
		Tpi.RankedCount = C.Small();
	}
	else if (Kind == u4line::Moment)
	{
		SceneMoment M;
		M.Id = C.Small();
		const int MomentKindRead = C.Small();
		M.Kind = MomentKindRead >= 0 && MomentKindRead < static_cast<int>(MomentKind::Count) ? static_cast<MomentKind>(MomentKindRead) : MomentKind::Everyday;
		M.Day = C.Small();
		M.Start = C.Small();
		M.End = C.Small();
		M.Pull = C.Flt();
		M.Carry = C.Dbl();
		M.Emitted = C.Small();
		M.Npc = C.Small();
		M.What = C.Str();
		M.Key = C.Str();
		M.Site = C.Small();
		M.Region = C.Small();
		M.Flags = C.Small();
		M.Half = C.Flt();
		M.Mix = static_cast<int16_t>(C.Small());
		Scene.NextId = std::max(Scene.NextId, M.Id + 1);
		Scene.Moments.push_back(M);
	}
	else if (Kind == u4line::Grow)
	{
		if (Npc* N = Person(C.Int()))
		{
			// A missing (trailing default) field reads as its default.
			Growth G;
			auto GrowInt = [&](int Default) { return C.At < Fields.size() ? C.Small() : Default; };
			auto GrowReal = [&](float Default) { return C.At < Fields.size() ? C.Flt() : Default; };
			G.Ph = static_cast<GrowthPhase>(GrowInt(static_cast<int>(G.Ph)));
			G.Why = static_cast<Cause>(GrowInt(static_cast<int>(G.Why)));
			G.Until = GrowInt(G.Until);
			G.Coach = GrowInt(G.Coach);
			G.CoachUntil = GrowInt(G.CoachUntil);
			G.BusyUntil = GrowInt(G.BusyUntil);
			G.GrindCool = GrowInt(G.GrindCool);
			G.TiltCool = GrowInt(G.TiltCool);
			G.GrindRun = static_cast<uint8_t>(GrowInt(G.GrindRun));
			G.TiltRun = static_cast<uint8_t>(GrowInt(G.TiltRun));
			G.Band = static_cast<int8_t>(GrowInt(G.Band));
			G.BandSince = GrowInt(G.BandSince);
			G.BirthPeak = GrowReal(G.BirthPeak);
			G.BirthPot = GrowReal(G.BirthPot);
			G.Flags = static_cast<uint16_t>(GrowInt(G.Flags));
			G.SeriousAt = GrowInt(G.SeriousAt);
			G.SeriousTop = GrowReal(G.SeriousTop);
			N->Grow = G;
		}
	}
	else if (Kind == u4line::Tpi)
	{
		const long long Id = C.Int();
		TpiRec* T = Id == -1 ? &HeroTpi : nullptr;
		if (!T)
		{
			if (Npc* N = Person(Id))
			{
				T = &N->Tpi;
			}
		}
		if (T)
		{
			*T = TpiRec();
			T->Now = static_cast<int16_t>(C.Small());
			T->Prev = static_cast<int16_t>(C.Small());
			T->Best = static_cast<int16_t>(C.Small());
			T->BestDay = C.Small();
			T->First = static_cast<int16_t>(C.Small());
			T->FirstDay = C.Small();
			ReadJoined(C.At < Fields.size() ? Fields[C.At] : std::string(), T->Monthly);
			++C.At;
			T->Run100 = static_cast<uint8_t>(C.Small());
			T->Run1000 = static_cast<uint8_t>(C.Small());
			T->PlayMask = static_cast<uint32_t>(C.Int());
			ReadTpiWeeks(C.At < Fields.size() ? Fields[C.At] : std::string(), T->Week);
			++C.At;
			if (Id == -1)
			{
				ReadJoined(C.At < Fields.size() ? Fields[C.At] : std::string(), HeroHist);
			}
		}
	}
	else if (Kind == u4line::ArcLine)
	{
		Arc A;
		A.Id = C.Small();
		A.Npc = C.Small();
		const int ArcKindRead = C.Small();
		A.Kind = ArcKindRead >= 0 && ArcKindRead < static_cast<int>(ArcKind::Count) ? static_cast<ArcKind>(ArcKindRead) : ArcKind::Climb;
		A.Began = C.Small();
		const int ArcStateRead = C.Small();
		A.State = ArcStateRead >= 0 && ArcStateRead <= static_cast<int>(ArcState::Closed) ? static_cast<ArcState>(ArcStateRead) : ArcState::Open;
		A.Score = C.Flt();
		A.Other = C.Small();
		A.ClosedDay = C.Small();
		A.BestRank = static_cast<int16_t>(C.Small());
		A.BestDay = C.Small();
		for (const std::string& R : Split(C.At < Fields.size() ? Fields[C.At] : std::string(), '|'))
		{
			if (A.BeatCount >= A.Beats.size())
			{
				break;
			}
			const std::vector<std::string> V = Split(R, '~');
			Cursor Rc{V, 0};
			ArcBeat& B = A.Beats[A.BeatCount++];
			B.Day = Rc.Small();
			const int BeatRead = Rc.Small();
			B.Kind = BeatRead >= 0 && BeatRead < static_cast<int>(BeatKind::Count) ? static_cast<BeatKind>(BeatRead) : BeatKind::Debut;
			B.Value = Rc.Small();
			B.Ref = Rc.Small();
			B.Surfaced = static_cast<uint8_t>(Rc.Small());
		}
		Arcs.NextId = std::max(Arcs.NextId, A.Id + 1);
		Arcs.Arcs.push_back(A);
	}
	else if (Kind == u4line::Follow)
	{
		Follows.clear();
		for (const std::string& R : Split(C.At < Fields.size() ? Fields[C.At] : std::string(), ','))
		{
			const std::vector<std::string> V = Split(R, '~');
			Cursor Rc{V, 0};
			FollowRec F0;
			F0.Npc = Rc.Small();
			F0.Since = Rc.Small();
			for (int16_t& H : F0.Hist)
			{
				H = static_cast<int16_t>(Rc.Small());
			}
			Follows.push_back(F0);
		}
		++C.At;
		Watch = {};
		size_t K = 0;
		for (const std::string& R : Split(C.At < Fields.size() ? Fields[C.At] : std::string(), ','))
		{
			const std::vector<std::string> V = Split(R, '~');
			Cursor Rc{V, 0};
			if (K < Watch.size())
			{
				Watch[K].Npc = Rc.Small();
				Watch[K].Why = static_cast<uint8_t>(Rc.Small());
				Watch[K].Day = Rc.Small();
				++K;
			}
		}
		++C.At;
		Feed.clear();
		for (const std::string& R : Split(C.At < Fields.size() ? Fields[C.At] : std::string(), ','))
		{
			const std::vector<std::string> V = Split(R, '~');
			Cursor Rc{V, 0};
			FeedItem It;
			It.Day = Rc.Small();
			It.ArcId = Rc.Small();
			It.Beat = static_cast<uint8_t>(Rc.Small());
			Feed.push_back(It);
		}
		++C.At;
		const std::vector<std::string> V = Split(C.At < Fields.size() ? Fields[C.At] : std::string(), '~');
		Cursor Rc{V, 0};
		Tip = DeeTip();
		Tip.Npc = V.empty() ? -1 : Rc.Small();
		Tip.Day = Rc.Small();
		Tip.Closed = Rc.Bool();
	}
	else if (Kind == u4line::Regulars)
	{
		Regulars.clear();
		while (C.At < Fields.size())
		{
			Regulars.push_back(C.Small());
		}
	}
	else if (Kind == u4line::Crowd)
	{
		if (C.At < Fields.size() && Fields[C.At] == u4line::CrowdDay)
		{
			++C.At;
			CrowdToday = CrowdDayWeights();
			CrowdToday.Day = C.Small();
			for (const std::string& A : Split(C.At < Fields.size() ? Fields[C.At] : std::string(), ','))
			{
				CrowdToday.Anon.push_back(std::strtod(A.c_str(), nullptr));
			}
		}
		else
		{
			CrowdRec R;
			R.Serial = static_cast<uint32_t>(C.Int());
			const std::string Name = C.Str();
			std::snprintf(R.Name, sizeof(R.Name), "%s", Name.c_str());
			ReadCrowdSmalls(C.At < Fields.size() ? Fields[C.At] : std::string(), R);
			++C.At;
			R.Bankroll = static_cast<int32_t>(C.Int());
			R.PeakRoll = static_cast<int32_t>(C.Int());
			R.Income = static_cast<int32_t>(C.Int());
			R.Followers = static_cast<int32_t>(C.Int());
			R.Won = C.Int();
			R.Spent = C.Int();
			R.Season = C.Flt();
			R.Notice = C.Flt();
			const std::vector<std::string> Fin = Split(C.At < Fields.size() ? Fields[C.At] : std::string(), '|');
			++C.At;
			for (size_t K = 0; K < Fin.size() && K < 4; ++K)
			{
				(K == 0 ? R.Best : R.Last[K - 1]) = ReadCrowdFinish(Fin[K]);
			}
			ReadCrowdWeeks(C.At < Fields.size() ? Fields[C.At] : std::string(), R.Week);
			if (Crowd.Count % CrowdBook::PageSize == 0)
			{
				Crowd.Pages.push_back(std::make_shared<std::array<CrowdRec, CrowdBook::PageSize>>());
			}
			Crowd.Edit(Crowd.Count) = R;
			Crowd.ByName[Name] = Crowd.Count;
			++Crowd.Count;
			Crowd.NextSerial = std::max(Crowd.NextSerial, R.Serial + 1);
		}
	}
	else if (Kind == u4line::QCrowd)
	{
		const std::string Id = C.Str();
		for (Pending& P : Queue.Mut())
		{
			if (P.Id != Id)
			{
				continue;
			}
			P.CrowdIn.clear();
			for (const std::string& R : Split(C.At < Fields.size() ? Fields[C.At] : std::string(), '|'))
			{
				const std::vector<std::string> V = Split(R, '~');
				Cursor Rc{V, 0};
				CrowdEntry E;
				E.Serial = static_cast<uint32_t>(Rc.Int());
				E.Known = Rc.Small();
				E.Better = Rc.Small();
				E.Bullets = Rc.Small();
				P.CrowdIn.push_back(E);
			}
			break;
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
	// Update 4 (§6 PA-33): World::NameUsed's lowercased index, every name the roster holds; and nothing kept from before the load.
	std::unordered_set<std::string>& Lower = NamesLower.Mut();
	Lower.clear();
	Headlined = HeadlinerCache();
	for (size_t K = 0; K < Roster.size(); ++K)
	{
		Npc& N = Edit(static_cast<int>(K));
		N.Id = static_cast<int>(K);
		if (!N.Name.empty())
		{
			ByName[N.Name] = N.Id;
		}
		Lower.insert(sim::NameLower(N.Name));
	}
	// Saves from before the stats pages: the history so far is drawn from the lifetime numbers.
	for (Npc& N : EditAll())
	{
		Sim::SeedStats(N);
	}
	// Saves from before the trophy case kept only counts: the titles in history say which, the rest stay plain.
	for (Npc& N : EditAll())
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
			// Update 4 (SPEC v1.7 §2.6.1, §6 PA-25): a Player of the Series belt is no bracelet ("The Championship Online 2027 Player of
			// the Series" would read as one below); no such Honor exists before G1.
			if (IsCrownHonor(H))
			{
				continue;
			}
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
			const net::Network& Net = net::ForEvent(H.EventId);
			const net::EventTemplate* T = Net.FindTemplate(H.EventId.substr(0, At));
			if (T && (T->Bracelet || T->Ring))
			{
				A.Ring = T->Ring;
				A.Series = T->Series;
				const net::SeriesInfo* Sr = Net.FindSeries(T->Series);
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
	// Saves from before the Embercrest had regulars of its own: they join (once; the same people as in a new world). A fresh
	// start's books opened without them, so they join it with nothing on record (update 2 §3.7: no history on load).
	const size_t Known = Roster.size();
	Sim::RoomLocals(*this, sim::DayAt(Now));
	if (Mode == Start::Blank)
	{
		for (size_t K = Known; K < Roster.size(); ++K)
		{
			Sim::WipeRecord(Edit(static_cast<int>(K)), Sim::BooksOpened(*this));
		}
	}
	// An older version: brought up to this one, once (it writes as this version from now on).
	if (LoadedVersion < Version)
	{
		Sim::Migrate(*this, LoadedVersion);
		LoadedVersion = Version;
	}
	std::stable_sort(Queue.Mut().begin(), Queue.Mut().end(), [](const Pending& A, const Pending& B) { return A.End < B.End; });
	RefreshAll();
	// The boards' ranks a day ago; a fresh world's first weeks keep the Season's from the save (its board report reads them).
	const std::vector<int> Season = RanksYesterday[static_cast<size_t>(net::Board::Season)];
	Sim::Ranks(*this);
	if (Mode == Start::Blank && !Season.empty())
	{
		std::vector<int>& Rk = RanksYesterday[static_cast<size_t>(net::Board::Season)];
		for (size_t K = 0; K < Rk.size(); ++K)
		{
			Rk[K] = K < Season.size() ? Season[K] : 0;
		}
	}
}

void Sim::Migrate(World& W, int From)
{
	const int Today = sim::DayAt(W.Now);
	// (Every step that seeds history asks how the world began first: a fresh start has none to seed, update 2 §3.7.)
	const bool Seeds = W.Began() != Start::Blank;
	if (From < 2)
	{
		// Version 2 (next_segment_spec §7.5 step 5).
		// The room's records: its regulars', locals' and cast's last months from their habits (no results: the wall only
		// ever shows what was played). Cards appear at the first standings after load ("Standings post Monday.").
		if (Seeds)
		{
			SeedRoomHistory(W, Today);
			// The titles won before the story began (§11 Q23), as a new world has them.
			SeedChampions(W);
		}
		for (Npc& N : W.EditAll())
		{
			// The site's own deal only ever belongs to its ten (§1.9 #21; Kast's catalog could hand it to anyone).
			if (!N.Pro && N.Sponsor == "RiverLine")
			{
				N.Sponsor.clear();
			}
			// Team RiverLine pays what its terms say now: signing added $1,500 a week, and leaving takes off TeamWeekly, so
			// the pros signed before take the difference off now (or one who leaves would keep the rest for ever).
			const Chips Less = sim::Cents(1500.0) - TeamWeekly();
			if (N.Pro && Less > 0)
			{
				N.Income = std::max<Chips>(0, N.Income - Less);
			}
			// Live Player of the Year counts Regional events and up (§11 Q3): this season's points held the room's own
			// weeklies too, and which were which wasn't kept, so the live race starts again from here.
			N.ThisSeason.LivePoints = 0.0;
		}
		W.HeroLivePoints = 0.0;
		// The month's Verified review, at once.
		Review(W, Today);
	}
	// Version 3 (update 2 §6.5): a world from before the fresh start began as every world did then, Established. Its head
	// has no Start, and the head's read gives a missing one Established, so the step has nothing left to set (a fresh start
	// written before the version moved carries its Start and keeps it). Nothing else in a version 2 world changes.
	if constexpr (Version >= 4)
	{
		if (From < 4)
		{
			// Version 4 (update 2 §5.9, §6.5): the rival sites reach a world that had none. Every NPC keeps RiverLine and gets
			// the accounts the rule gives them as of today (Team RiverLine's pros play only on RiverLine); RunItTwice's lobby has
			// been open since Night One, Ignite opens on its own day or, past it, on the first day the world hasn't planned yet
			// ("Ignite launches" in that morning's news, no history before); Team RunItTwice signs at the next 1st (on results in
			// a fresh world). No rival has a record yet, so RiverLine's boards read exactly as they did.
			Sim::OpenSites(W, Today, true);
		}
	}
}
} // namespace world
} // namespace ss
