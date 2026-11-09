// Update 3 (SPEC §2.6 F7, §7.3): FUSE's pure parts, core-fuse's file. The prize tables at any edge (the x5-and-up rows
// fixed, x2 and x3 re-solved so each tier's table sums to 1 and to 3 x (1 - edge)), the fee the rake hooks see, the keyed
// draw per tier ordinal (tickets counted), the id, the synthetic 3-entrant event with the exact pool, the seats, the
// payout, the record's entry and result sides and its save line. Nothing here touches a session or a save on its own.
//
// The calibration (S-J.md §3 and §5, the lead's G2 call): the seat-fillers "high", the house edge 10%, the starting stack
// 300 chips. At that setting S-J measured coach-best P(1st) .3992, P(2nd) .3207, P(3rd) .2801 over 100,000 paired games
// (ROI +7.4%), and every autoplay style at or under +2.0%.
#include "ShortStack/Game/Fuse.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/SitePlay.h"
#include "ShortStack/Game/World.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

namespace ss
{
namespace fuse
{
namespace fuse_detail
{
// The knobs (§2.6 F7's order: the fillers, the edge, the stack).
constexpr double FuseEdge = 0.10;
constexpr Chips FuseStack = 300;
const char* const FuseFillers = "high";
constexpr Chips FuseBuyIns[TierCount] = {110, 330, 550};
constexpr int FuseTopLow = 1000; // the top row at $1.10
constexpr int FuseTopHigh = 250; // at $3.30 and $5.50
constexpr int FuseWallMax = 10;
constexpr int FuseHitsMax = 10;
constexpr int FuseHitFrom = 10; // the Wall and Big hits keep FUSEs of x10 and up
constexpr double FuseSiteDayShift = 6.0 * 60.0; // site days turn over at 6 AM (§1.1)

/** The fixed rows (x5 and up), their probabilities by column (0: $1.10, 1: $3.30 and $5.50). */
struct FuseFixed
{
	int M;
	double P0, P1;
};
constexpr FuseFixed FuseFixedRows[5] = {{5, 0.065, 0.065}, {10, 0.02, 0.02}, {25, 0.004, 0.004}, {100, 0.0005, 0.0006}, {FuseTopLow, 0.00005, 0.00005}};

/** x2 and x3 solved for an edge in one column: the table sums to 1 and its mean is 3 x (1 - edge). */
void FuseSolve(int Column, double Edge, double& P2, double& P3)
{
	double P = 0.0, E = 0.0;
	for (const FuseFixed& F : FuseFixedRows)
	{
		const double Pr = Column == 0 ? F.P0 : F.P1;
		const int M = F.M == FuseTopLow && Column == 1 ? FuseTopHigh : F.M;
		P += Pr;
		E += Pr * static_cast<double>(M);
	}
	const double Spare = 1.0 - P;
	const double Need = 3.0 * (1.0 - Edge) - E;
	P3 = Need - 2.0 * Spare;
	P2 = Spare - P3;
}

std::array<Row, 7> FuseBuild(double Edge)
{
	std::array<Row, 7> Rows{};
	double P2[2] = {0, 0}, P3[2] = {0, 0};
	FuseSolve(0, Edge, P2[0], P3[0]);
	FuseSolve(1, Edge, P2[1], P3[1]);
	Rows[0].M = 2;
	Rows[0].P[0] = P2[0];
	Rows[0].P[1] = P2[1];
	Rows[1].M = 3;
	Rows[1].P[0] = P3[0];
	Rows[1].P[1] = P3[1];
	for (size_t I = 0; I < 5; ++I)
	{
		Row& R = Rows[I + 2];
		R.M = FuseFixedRows[I].M;
		R.P[0] = FuseFixedRows[I].P0;
		R.P[1] = FuseFixedRows[I].P1;
	}
	for (Row& R : Rows)
	{
		R.Split = R.M >= 25;
	}
	return Rows;
}

/** murmur3's fmix32: S-J measured the raw 32-bit Fnv1a of sequential ordinals failing a chi-square against the table on
 *  every world seed tried (33-195 on 6 df over 2,000,000 draws); the finalizer brings it to 2-12. */
uint32_t FuseMix(uint32_t H)
{
	H ^= H >> 16;
	H *= 0x85ebca6bu;
	H ^= H >> 13;
	H *= 0xc2b2ae35u;
	H ^= H >> 16;
	return H;
}

double FuseUnit(const std::string& Key)
{
	return static_cast<double>(FuseMix(Fnv1a(Key))) / 4294967296.0;
}

bool FuseValidTier(int Tier)
{
	return Tier >= 0 && Tier < TierCount;
}

int FuseSiteDay(double World)
{
	return net::DayOf(World - FuseSiteDayShift);
}

/** Whether an Ignite member is at the tables at this hour, from how they play (World's Plan), keyed by day for the ones
 *  who play a night here and there. The early hours belong to the evening before. */
bool FuseOnline(const world::Npc& N, double World)
{
	const int Day = net::DayOf(World);
	const double Hour = (World - static_cast<double>(Day) * net::MinutesPerDay) / 60.0;
	const int Evening = Hour < 6.0 ? Day - 1 : Day;
	const int Wd = net::Weekday(Evening);
	if (N.TripUntil >= Day && Day >= N.TripFrom)
	{
		return false; // away at a live series
	}
	switch (N.Schedule)
	{
	case world::Plan::Daily: return Hour >= 10.0 || Hour < 4.0;
	case world::Plan::Nights: return Hour >= 17.0 || Hour < 3.0;
	case world::Plan::Weekends: return Wd >= 4 && (Hour >= 12.0 || Hour < 4.0);
	default: break;
	}
	// Occasional, series-focused, majors-only and circuit players: a night here and there.
	const bool Tonight = FuseMix(Fnv1a(N.Name + ":fuseonline:" + std::to_string(Evening))) % 3u == 0u;
	return Tonight && (Hour >= 19.0 || Hour < 2.0);
}

/** The record's own escape: the save's three, and ':' and ',' (the Wall's and Big hits' separators). */
std::string FuseEscape(const std::string& In)
{
	std::string Out;
	for (const char Ch : In)
	{
		switch (Ch)
		{
		case '\\': Out += "\\\\"; break;
		case '\t': Out += "\\t"; break;
		case '\n': Out += "\\n"; break;
		case ':': Out += "\\c"; break;
		case ',': Out += "\\m"; break;
		default: Out.push_back(Ch); break;
		}
	}
	return Out;
}

std::string FuseUnescape(const std::string& In)
{
	std::string Out;
	for (size_t I = 0; I < In.size(); ++I)
	{
		if (In[I] != '\\' || I + 1 >= In.size())
		{
			Out.push_back(In[I]);
			continue;
		}
		const char N = In[++I];
		Out.push_back(N == 't' ? '\t' : N == 'n' ? '\n' : N == 'c' ? ':' : N == 'm' ? ',' : N);
	}
	return Out;
}

std::vector<std::string> FuseSplit(const std::string& S, char Sep)
{
	std::vector<std::string> Out;
	if (S.empty())
	{
		return Out;
	}
	std::string Cur;
	for (const char Ch : S)
	{
		if (Ch == Sep)
		{
			Out.push_back(Cur);
			Cur.clear();
		}
		else
		{
			Cur.push_back(Ch);
		}
	}
	Out.push_back(Cur);
	return Out;
}

long long FuseInt(const std::string& S)
{
	return std::strtoll(S.c_str(), nullptr, 10);
}

/** The Wall's order: the biggest multiplier first, then the bigger prize, then the newer. */
bool FuseWallBefore(const site::play::WallRow& A, const site::play::WallRow& B)
{
	if (A.M != B.M)
	{
		return A.M > B.M;
	}
	if (A.Prize != B.Prize)
	{
		return A.Prize > B.Prize;
	}
	return A.Day > B.Day;
}
} // namespace fuse_detail

std::array<Row, 7> TableAt(double Edge)
{
	return fuse_detail::FuseBuild(Edge);
}

const std::array<Row, 7>& Table()
{
	static const std::array<Row, 7> FuseRows = fuse_detail::FuseBuild(fuse_detail::FuseEdge);
	return FuseRows;
}

double Edge()
{
	return fuse_detail::FuseEdge;
}

Chips BuyInOf(int Tier)
{
	return fuse_detail::FuseValidTier(Tier) ? fuse_detail::FuseBuyIns[Tier] : 0;
}

Chips FeeOf(int Tier)
{
	return static_cast<Chips>(std::llround(Edge() * static_cast<double>(BuyInOf(Tier))));
}

int TopOf(int Tier)
{
	return Tier <= 0 ? fuse_detail::FuseTopLow : fuse_detail::FuseTopHigh;
}

int MOf(const Row& R, int Tier)
{
	return R.M == fuse_detail::FuseTopLow ? TopOf(Tier) : R.M;
}

double POf(const Row& R, int Tier)
{
	return Tier <= 0 ? R.P[0] : R.P[1];
}

Chips StartingStack()
{
	return fuse_detail::FuseStack;
}

const char* Fillers()
{
	return fuse_detail::FuseFillers;
}

std::vector<ss::Level> Levels()
{
	std::vector<ss::Level> Out;
	const Chips Bb[] = {20, 30, 40, 60, 80, 100, 120, 160, 200};
	for (const Chips B : Bb)
	{
		ss::Level L;
		L.Sb = B / 2;
		L.Bb = B;
		L.Ante = 0;
		Out.push_back(L);
	}
	// Then +25% a level, rounded to 10 (a 300-chip FUSE never gets near the end of the list).
	while (Out.size() < 40)
	{
		ss::Level L;
		L.Bb = static_cast<Chips>(std::llround(static_cast<double>(Out.back().Bb) * 1.25 / 10.0)) * 10;
		L.Sb = L.Bb / 2;
		L.Ante = 0;
		Out.push_back(L);
	}
	return Out;
}

std::string NameOf(int Tier)
{
	return "FUSE " + Money(BuyInOf(fuse_detail::FuseValidTier(Tier) ? Tier : 0));
}

int TierOf(const std::string& EventId)
{
	static const std::string Prefix = "ignite.fuse";
	if (EventId.size() < Prefix.size() + 1 || EventId.compare(0, Prefix.size(), Prefix) != 0)
	{
		return -1;
	}
	const char D = EventId[Prefix.size()];
	if (D < '0' || D >= static_cast<char>('0' + TierCount))
	{
		return -1;
	}
	// The digit ends the template part: the id goes on with "@", or stops there (the template id RegisterEvent's ticket test
	// reads). "ignite.fusetkt0", a held ticket, isn't an event.
	if (EventId.size() > Prefix.size() + 1 && EventId[Prefix.size() + 1] != '@')
	{
		return -1;
	}
	return D - '0';
}

bool IsFuse(const std::string& EventId)
{
	return TierOf(EventId) >= 0;
}

int OrdinalOf(const std::string& EventId)
{
	if (!IsFuse(EventId))
	{
		return -1;
	}
	const size_t Hash = EventId.rfind('#');
	return Hash == std::string::npos ? 0 : std::atoi(EventId.c_str() + Hash + 1);
}

int Draw(uint64_t WorldSeed, int Tier, int N)
{
	const int T = fuse_detail::FuseValidTier(Tier) ? Tier : 0;
	double U = fuse_detail::FuseUnit(std::to_string(WorldSeed) + ":ignite:fuse:" + std::to_string(T) + ":" + std::to_string(N));
	for (const Row& R : Table())
	{
		const double P = POf(R, T);
		if (U < P)
		{
			return MOf(R, T);
		}
		U -= P;
	}
	return Table().front().M; // the rounding's last sliver: the commonest row
}

std::string IdFor(int Tier, double World, int N)
{
	return "ignite.fuse" + std::to_string(fuse_detail::FuseValidTier(Tier) ? Tier : 0) + "@" + std::to_string(static_cast<long long>(std::floor(World))) + "#" +
		std::to_string(N);
}

ss::LobbyEvent EventFor(int Tier, int M, double World, int N)
{
	const int T = fuse_detail::FuseValidTier(Tier) ? Tier : 0;
	const Chips BuyIn = BuyInOf(T);
	ss::LobbyEvent L;
	L.Joinable = true;
	L.Name = NameOf(T);
	L.Start = net::TimeLabel(World);
	L.BuyInLabel = Money(BuyIn);
	L.BuyInCents = BuyIn;
	L.Game = "Hold'em";
	L.Speed = "Hyper";
	L.EntrantsLabel = "3";
	L.GuaranteeLabel = Money(static_cast<Chips>(M) * BuyIn);
	L.Status = "Registering";
	L.Spec.Id = IdFor(T, World, N);
	L.Spec.Name = L.Name;
	L.Spec.BuyInCents = BuyIn;
	// The engine's pool is max(GTD, entrants x (buy-in - fee)): with the whole buy-in as the spec's fee the second term is 0
	// and the guarantee is the pool, exactly m x buy-in (Tournament.cpp:31; no engine change). The rake hooks never read this
	// fee: LightFuse hands RegisterEvent FeeOf(Tier) as the override.
	L.Spec.FeeCents = BuyIn;
	L.Spec.GuaranteeCents = static_cast<Chips>(M) * BuyIn;
	L.Spec.Entrants = 3;
	L.Spec.TableSize = 3;
	L.Spec.StartingStack = StartingStack();
	L.Spec.LevelMinutes = 3.0;
	L.Spec.SecondsPerHand = 30.0;
	L.Spec.Population = Fillers();
	L.Spec.Speed = "Hyper";
	L.Spec.Levels = Levels();
	return L;
}

std::array<Chips, 3> Payout(int Tier, int M)
{
	const Chips Pool = static_cast<Chips>(std::max(0, M)) * BuyInOf(fuse_detail::FuseValidTier(Tier) ? Tier : 0);
	if (M < 25)
	{
		return {Pool, 0, 0};
	}
	// 80/12/8 in whole cents, the third taking what's left, so the places sum to the pool exactly.
	const Chips First = Pool * 80 / 100, Second = Pool * 12 / 100;
	return {First, Second, Pool - First - Second};
}

std::vector<std::string> SeatsFor(const world::World* W, uint64_t WorldSeed, int Tier, int N, double World)
{
	// Two Ignite members at the micro and low stakes who are at the tables this hour, keyed on the tier and its ordinal
	// (never NameField's own Rng): the same world, ordinal and hour seat the same two. Too few online: anyone who plays there.
	std::vector<std::string> Out;
	if (!W)
	{
		return Out;
	}
	const unsigned IgniteBit = 1u << static_cast<unsigned>(net::SiteId::Ignite);
	std::vector<const world::Npc*> Online, Members;
	for (const world::Npc& P : W->People())
	{
		if (!P.Playing() || P.Rival || P.Pro || P.Tier > static_cast<int>(net::Tier::Low) || (P.Sites & IgniteBit) == 0)
		{
			continue;
		}
		Members.push_back(&P);
		if (fuse_detail::FuseOnline(P, World))
		{
			Online.push_back(&P);
		}
	}
	const std::vector<const world::Npc*>& Pool = Online.size() >= 2 ? Online : Members;
	if (Pool.empty())
	{
		return Out;
	}
	const std::string Key = std::to_string(WorldSeed) + ":ignite:fuseseat:" + std::to_string(fuse_detail::FuseValidTier(Tier) ? Tier : 0) + ":" + std::to_string(N);
	const size_t A = fuse_detail::FuseMix(Fnv1a(Key + ":0")) % Pool.size();
	Out.push_back(Pool[A]->Name);
	if (Pool.size() > 1)
	{
		size_t B = fuse_detail::FuseMix(Fnv1a(Key + ":1")) % (Pool.size() - 1);
		B += B >= A ? 1 : 0;
		Out.push_back(Pool[B]->Name);
	}
	return Out;
}

std::string TicketId(int Tier)
{
	return "ignite.fusetkt" + std::to_string(fuse_detail::FuseValidTier(Tier) ? Tier : 0);
}

int TierOfTicket(const std::string& TicketId)
{
	static const std::string Prefix = "ignite.fusetkt";
	if (TicketId.size() != Prefix.size() + 1 || TicketId.compare(0, Prefix.size(), Prefix) != 0)
	{
		return -1;
	}
	const int T = TicketId.back() - '0';
	return fuse_detail::FuseValidTier(T) ? T : -1;
}

double RevealSeconds(int M)
{
	return M <= 3 ? 1.5 : M <= 10 ? 3.4 : 6.3;
}

int MultiplierOf(const ss::TournamentSpec& Spec)
{
	return IsFuse(Spec.Id) && Spec.BuyInCents > 0 ? static_cast<int>(Spec.GuaranteeCents / Spec.BuyInCents) : 0;
}

Chips PrizeAt(const ss::TournamentSpec& Spec, int Place)
{
	if (!IsFuse(Spec.Id) || Place < 1 || Place > 3)
	{
		return 0;
	}
	return Payout(TierOf(Spec.Id), MultiplierOf(Spec))[static_cast<size_t>(Place - 1)];
}

std::vector<std::string> Inventory()
{
	// Every sentence FUSE's core says (LightFuse's answers and the table's dev line) and its labels.
	std::vector<std::string> Out = {
		"Pick a FUSE: $1.10, $3.30 or $5.50.",
		"Open an account with Ignite first.",
		"Two FUSEs at a time is the limit. Finish one first.",
		"There's no FUSE ticket at that stake on your account.",
		"The multiplier on this FUSE was set by hand for a test.",
		// The table's own lines (Session.cpp's FUSE branches, the lead's: patches/core-fuse_to_lead_1.diff).
		"Welcome to {Event}. Three players, a {Amount} prize.",
		"Blinds {Blinds}, up every {Level} minutes. Good luck!",
		"YOU WON THE FUSE",
		// The wallet's prize line (§1.3; SessionSites.cpp's SiteSettle, core-sites-g2's).
		"FUSE x{Mult}: {Place}",
	};
	for (int T = 0; T < TierCount; ++T)
	{
		Out.push_back(NameOf(T));
	}
	return Out;
}

// ------------------------------------------------------------------ the record

void NoteEntry(site::play::FuseRec& R, int Tier, int M, bool Paid)
{
	if (!fuse_detail::FuseValidTier(Tier))
	{
		return;
	}
	++R.N[static_cast<size_t>(Tier)];
	for (size_t I = R.Last10.size() - 1; I > 0; --I)
	{
		R.Last10[I] = R.Last10[I - 1];
	}
	R.Last10[0] = static_cast<int16_t>(std::min(M, 32767));
	R.BestM = std::max(R.BestM, M);
	if (Paid)
	{
		// BandOf's buy-ins (the newest first) and the money out; a ticket entry costs nothing and earns nothing (W4).
		for (size_t I = R.RecentBuyIns.size() - 1; I > 0; --I)
		{
			R.RecentBuyIns[I] = R.RecentBuyIns[I - 1];
		}
		R.RecentBuyIns[0] = BuyInOf(Tier);
		R.NetCents -= BuyInOf(Tier);
	}
}

void NoteResult(site::play::FuseRec& R, const site::play::SettleFacts& F, double World)
{
	const int Day = fuse_detail::FuseSiteDay(World);
	const int Tier = TierOf(F.EventId);
	++R.Played;
	R.Won += F.Place == 1 ? 1 : 0;
	R.NetCents += F.Prize;
	if (F.FuseM < fuse_detail::FuseHitFrom)
	{
		return;
	}
	site::play::WallRow W;
	W.M = static_cast<int16_t>(std::min(F.FuseM, 32767));
	W.Place = static_cast<int8_t>(std::max(0, std::min(F.Place, 3)));
	W.Prize = F.Prize;
	W.Day = Day;
	R.Wall.push_back(W);
	std::stable_sort(R.Wall.begin(), R.Wall.end(), fuse_detail::FuseWallBefore);
	if (R.Wall.size() > static_cast<size_t>(fuse_detail::FuseWallMax))
	{
		R.Wall.resize(static_cast<size_t>(fuse_detail::FuseWallMax));
	}
	// Big hits name the winner, so only a FUSE whose winner is known (the table played out) makes one: never a guess (U5).
	if (!F.FuseWinner.empty())
	{
		site::play::HitRow H;
		H.M = W.M;
		H.Winner = F.FuseWinner;
		H.Prize = Payout(Tier, F.FuseM)[0];
		H.Day = Day;
		R.Hits.insert(R.Hits.begin(), H);
		if (R.Hits.size() > static_cast<size_t>(fuse_detail::FuseHitsMax))
		{
			R.Hits.resize(static_cast<size_t>(fuse_detail::FuseHitsMax));
		}
	}
}

bool HasRecord(const site::play::FuseRec& R)
{
	bool Any = R.Played != 0 || R.Won != 0 || R.NetCents != 0 || R.BestM != 0 || !R.Wall.empty() || !R.Hits.empty();
	for (const int N : R.N)
	{
		Any = Any || N != 0;
	}
	// RecentBuyIns is BandOf's (every paid Ignite entry's, core-siteplay pushes the rest): an MTT-only player's band is saved too.
	for (const Chips B : R.RecentBuyIns)
	{
		Any = Any || B != 0;
	}
	return Any;
}

void WriteLine(const site::play::FuseRec& R, std::string& Out)
{
	if (!HasRecord(R))
	{
		return;
	}
	std::string Wall, Last, Hits, Recent;
	for (const site::play::WallRow& W : R.Wall)
	{
		Wall += (Wall.empty() ? "" : ",") + std::to_string(W.M) + ":" + std::to_string(W.Place) + ":" + std::to_string(W.Prize) + ":" + std::to_string(W.Day);
	}
	// Last10 and the buy-ins fill from the front, so their nonzero prefix is the whole of them.
	for (const int16_t M : R.Last10)
	{
		if (M == 0)
		{
			break;
		}
		Last += (Last.empty() ? "" : ",") + std::to_string(M);
	}
	for (const site::play::HitRow& H : R.Hits)
	{
		Hits += (Hits.empty() ? "" : ",") + std::to_string(H.M) + ":" + fuse_detail::FuseEscape(H.Winner) + ":" + std::to_string(H.Prize) + ":" + std::to_string(H.Day);
	}
	for (const Chips B : R.RecentBuyIns)
	{
		if (B == 0)
		{
			break;
		}
		Recent += (Recent.empty() ? "" : ",") + std::to_string(B);
	}
	Out += "site\tfuse\tignite\t" + std::to_string(R.N[0]) + "\t" + std::to_string(R.N[1]) + "\t" + std::to_string(R.N[2]) + "\t" + std::to_string(R.Played) + "\t" +
		std::to_string(R.Won) + "\t" + std::to_string(R.NetCents) + "\t" + std::to_string(R.BestM) + "\t" + Wall + "\t" + Last + "\t" + Hits + "\t" + Recent + "\n";
}

bool ReadLine(const std::vector<std::string>& P, site::play::FuseRec& R)
{
	if (P.size() < 14 || P[0] != "site" || P[1] != "fuse")
	{
		return false;
	}
	site::play::FuseRec In;
	for (size_t I = 0; I < 3; ++I)
	{
		In.N[I] = static_cast<int>(fuse_detail::FuseInt(P[3 + I]));
	}
	In.Played = static_cast<int>(fuse_detail::FuseInt(P[6]));
	In.Won = static_cast<int>(fuse_detail::FuseInt(P[7]));
	In.NetCents = static_cast<Chips>(fuse_detail::FuseInt(P[8]));
	In.BestM = static_cast<int>(fuse_detail::FuseInt(P[9]));
	for (const std::string& Item : fuse_detail::FuseSplit(P[10], ','))
	{
		const std::vector<std::string> F = fuse_detail::FuseSplit(Item, ':');
		if (F.size() >= 4 && In.Wall.size() < static_cast<size_t>(fuse_detail::FuseWallMax))
		{
			site::play::WallRow W;
			W.M = static_cast<int16_t>(fuse_detail::FuseInt(F[0]));
			W.Place = static_cast<int8_t>(fuse_detail::FuseInt(F[1]));
			W.Prize = static_cast<Chips>(fuse_detail::FuseInt(F[2]));
			W.Day = static_cast<int>(fuse_detail::FuseInt(F[3]));
			In.Wall.push_back(W);
		}
	}
	size_t K = 0;
	for (const std::string& Item : fuse_detail::FuseSplit(P[11], ','))
	{
		if (K < In.Last10.size())
		{
			In.Last10[K++] = static_cast<int16_t>(fuse_detail::FuseInt(Item));
		}
	}
	for (const std::string& Item : fuse_detail::FuseSplit(P[12], ','))
	{
		const std::vector<std::string> F = fuse_detail::FuseSplit(Item, ':');
		if (F.size() >= 4 && In.Hits.size() < static_cast<size_t>(fuse_detail::FuseHitsMax))
		{
			site::play::HitRow H;
			H.M = static_cast<int16_t>(fuse_detail::FuseInt(F[0]));
			H.Winner = fuse_detail::FuseUnescape(F[1]);
			H.Prize = static_cast<Chips>(fuse_detail::FuseInt(F[2]));
			H.Day = static_cast<int>(fuse_detail::FuseInt(F[3]));
			In.Hits.push_back(H);
		}
	}
	K = 0;
	for (const std::string& Item : fuse_detail::FuseSplit(P[13], ','))
	{
		if (K < In.RecentBuyIns.size())
		{
			In.RecentBuyIns[K++] = static_cast<Chips>(fuse_detail::FuseInt(Item));
		}
	}
	R = In;
	return true;
}

std::string Describe(const site::play::FuseRec& R)
{
	std::string Out = "FUSE: lit " + std::to_string(R.N[0]) + " at $1.10, " + std::to_string(R.N[1]) + " at $3.30, " + std::to_string(R.N[2]) + " at $5.50; played " +
		std::to_string(R.Played) + ", won " + std::to_string(R.Won) + ", net " + Money(R.NetCents) + ", best x" + std::to_string(R.BestM) + "\n";
	std::string Last;
	for (const int16_t M : R.Last10)
	{
		if (M == 0)
		{
			break;
		}
		Last += " x" + std::to_string(M);
	}
	Out += "  last 10:" + (Last.empty() ? std::string(" none") : Last) + "\n";
	Out += "  Fuse Wall:" + std::string(R.Wall.empty() ? " none" : "") + "\n";
	for (const site::play::WallRow& W : R.Wall)
	{
		Out += "    x" + std::to_string(W.M) + " " + (W.Place >= 1 ? Ordinal(W.Place) : std::string("?")) + " " + Money(W.Prize) + " (day " + std::to_string(W.Day) + ")\n";
	}
	Out += "  Big hits:" + std::string(R.Hits.empty() ? " none" : "") + "\n";
	for (const site::play::HitRow& H : R.Hits)
	{
		Out += "    " + H.Winner + " x" + std::to_string(H.M) + " " + Money(H.Prize) + " (day " + std::to_string(H.Day) + ")\n";
	}
	std::string Recent;
	for (const Chips B : R.RecentBuyIns)
	{
		if (B == 0)
		{
			break;
		}
		Recent += " " + Money(B);
	}
	Out += "  paid buy-ins (newest first):" + (Recent.empty() ? std::string(" none") : Recent) + "\n";
	return Out;
}
} // namespace fuse
} // namespace ss
