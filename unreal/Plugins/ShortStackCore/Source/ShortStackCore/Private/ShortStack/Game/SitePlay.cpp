// Update 3 (SPEC §1.3, §1.4, §7.3): the rival sites' play book, core-siteplay's file. The Value Governor and the Guest
// List, the hooks the Session calls (a paid entry, a result, a hand), the calendar's step, the save lines and the pages'
// view. Ignite's loops are in SitePlayIgnite.cpp, RunItTwice's in SitePlayRit.cpp; the kit they share is SitePlayPriv.h.
//
// The Governor (§1.4): one 30-slot ring of (rake, promo) per rival site, by 6 AM world day, in thousandths of a cent (a
// Spark is 0.0157 cents: whole cents would round every small grant). Room = ceiling x Rake30 - Promo30 - Banked, the
// ceiling 0.50 (0.75 inside Ignite's 60-day welcome window), and the week's stake-band cap on top. A grant that fits pays
// now; one that doesn't waits on the Guest List, which is senior (while anything waits on a site, a new grant queues
// behind it) and moves at every step and paid entry: the spare Room goes to the oldest hold's Covered, a covered hold
// clears, and any later hold that fits whole in what's left clears too. Nothing on it expires.
#include "ShortStack/Game/SitePlay.h"

#include "SitePlayPriv.h"
#include "SitesDetail.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Fuse.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/World.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
#include <vector>

namespace ss
{
namespace site
{
namespace play
{
namespace siteplay_detail
{
namespace
{
int SitePlaySlot(int Day)
{
	const int M = Day % 30;
	return M < 0 ? M + 30 : M;
}

/** The hold's site, from its key ("1:s:240", "2:t:runittwice.the-sweat:league:24323"). */
net::SiteId SitePlayHoldSite(const Hold& H)
{
	return !H.Key.empty() && H.Key[0] == '2' ? net::SiteId::RunItTwice : net::SiteId::Ignite;
}

char SitePlaySiteChar(net::SiteId S)
{
	return S == net::SiteId::RunItTwice ? '2' : '1';
}

/** A hold's payload, read back from its key. */
SitePlayPayload SitePlayPayloadOf(const Hold& H)
{
	SitePlayPayload P;
	const std::vector<std::string> Parts = SitePlaySplit(H.Key, ':');
	if (Parts.size() >= 3 && Parts[1] == "s")
	{
		P.Sparks = SitePlayInt(Parts[2]);
	}
	else if (Parts.size() >= 3 && Parts[1] == "t")
	{
		P.TicketId = Parts[2];
		P.Label = H.What;
	}
	else if (Parts.size() >= 3 && Parts[1] == "w")
	{
		P.WalletCents = SitePlayInt(Parts[2]);
		P.Label = H.What;
	}
	return P;
}

int64_t SitePlaySat(int64_t A, int64_t B)
{
	const int64_t Lim = std::numeric_limits<int32_t>::max();
	const int64_t Sum = A + B;
	return Sum > Lim ? Lim : Sum < -Lim ? -Lim : Sum;
}

/** The world's seed as the Session spreads it (Session::WorldSeed: splitmix64's finalizer over World::Seed). */
uint64_t SitePlaySeedOf(const world::World* W)
{
	uint64_t Z = static_cast<uint64_t>(W ? W->Seed() : 0u) + 0x9e3779b97f4a7c15ull;
	Z = (Z ^ (Z >> 30)) * 0xbf58476d1ce4e5b9ull;
	Z = (Z ^ (Z >> 27)) * 0x94d049bb133111ebull;
	return Z ^ (Z >> 31);
}

// The Governor's copy (sentences: each passes the copy lint; labels: no closing stop).
const char* const SitePlayGovSentences[] = {
	"{Site} gave back {Promo} of your {Rake} rake in the last 30 days ({Pct}).",
	"Nothing waiting on the Guest List.",
	"It waits on the Guest List and clears as you play: nothing on it expires.",
	"Clears after {Amount} more rake (about {Entries} {Short}s).",
	"Clears at the next step.",
	"The Guest List is full: this waits in place until a line clears.",
	"Monday bonus: +{Sparks} Sparks banked.",
	"It stays on {Site}: it never pays rent.",
};
const char* const SitePlayGovLabels[] = {
	"Sparks owed",
	"Guest List",
	"Fine print",
	"Statement",
	"Rewards: {Count} tickets",
	"Promotion: {Amount}",
	"Grant {Amount}",
};
} // namespace

SitePlayScratch& SitePlayScratchOf(const Book& B)
{
	// UI state only: what the last entry, the last open and the last moments did, for the pages to animate. Keyed by the
	// book's address (the Session's own book; a test's), so a copy of the state (the save worker's) never reads another's.
	static SitePlayScratch SitePlayScratchSlots[4];
	static int SitePlayScratchNext = 0;
	for (SitePlayScratch& S : SitePlayScratchSlots)
	{
		if (S.For == &B)
		{
			return S;
		}
	}
	SitePlayScratch& Fresh = SitePlayScratchSlots[SitePlayScratchNext];
	SitePlayScratchNext = (SitePlayScratchNext + 1) % 4;
	Fresh = SitePlayScratch();
	Fresh.For = &B;
	return Fresh;
}

int SitePlayGovIndex(net::SiteId S)
{
	const int I = static_cast<int>(S);
	return I >= 1 && I <= 2 ? I : 0;
}

void SitePlayRoll(Book& B, net::SiteId S, int Day)
{
	GovRing& G = B.Gov[static_cast<size_t>(SitePlayGovIndex(S))];
	if (G.Day0 < 0)
	{
		G.Rake.fill(0);
		G.Promo.fill(0);
		G.Day0 = Day;
	}
	else if (Day > G.Day0)
	{
		const int Steps = std::min(Day - G.Day0, 30);
		for (int K = 1; K <= Steps; ++K)
		{
			const size_t Slot = static_cast<size_t>(SitePlaySlot(G.Day0 + K));
			G.Rake[Slot] = 0;
			G.Promo[Slot] = 0;
		}
		G.Day0 = Day;
	}
	const int Week = PlayWeek(Day);
	// (Forward only: a call for an earlier day never moves the week back, which would empty the week's cap a second time.)
	if (Week > G.WeekKey)
	{
		const bool Had = G.WeekKey >= 0;
		G.WeekKey = Week;
		G.WeekFace = 0;
		// The Monday bonus (§1.4, F1): base Sparks banked past last week's band cap credit now, outside the new week's cap.
		if (Had && S == net::SiteId::Ignite && (G.Banked > 0 || B.Ig.S.Deferred > 0))
		{
			// Spread with the rake that earned it, like any grant (SitePlayBook's rule; it can't be called from here).
			const int64_t Rake = SitePlayRake30(G);
			int64_t Left = G.Banked;
			for (size_t K = 0; K < G.Rake.size() && Rake > 0; ++K)
			{
				const int64_t Part = static_cast<int64_t>(G.Banked) * static_cast<int64_t>(G.Rake[K]) / Rake;
				G.Promo[K] = static_cast<int32_t>(SitePlaySat(G.Promo[K], Part));
				Left -= Part;
			}
			const size_t Slot = static_cast<size_t>(SitePlaySlot(G.Day0));
			G.Promo[Slot] = static_cast<int32_t>(SitePlaySat(G.Promo[Slot], Left));
			B.Ig.S.Balance += B.Ig.S.Deferred;
			B.Ig.S.Deferred = 0;
			G.Banked = 0;
		}
	}
}

int64_t SitePlayRake30(const GovRing& G)
{
	int64_t Sum = 0;
	for (const int32_t R : G.Rake)
	{
		Sum += R;
	}
	return Sum;
}

int64_t SitePlayPromo30(const GovRing& G)
{
	int64_t Sum = 0;
	for (const int32_t P : G.Promo)
	{
		Sum += P;
	}
	return Sum;
}

bool SitePlayWelcome(const life::State& L, net::SiteId S, double World)
{
	// One definition of Ignite's 60-day welcome window (G2 fix L11): SiteMoney.cpp's, the one the release itself reads.
	if (S != net::SiteId::Ignite)
	{
		return false;
	}
	const Account* A = AccountOf(L.Sites, S);
	return A && site::WelcomeWindow(*A, World);
}

double SitePlayCeiling(const life::State& L, net::SiteId S, double World)
{
	return SitePlayWelcome(L, S, World) ? 0.75 : 0.50;
}

int64_t SitePlayWeekCap(const life::State& L, net::SiteId S, double World)
{
	// The week's average paid buy-in on the site (§1.4), from the wallet's own buy-in lines this week (the last 40 kept: a
	// heavy week reads its most recent entries); none this week: the most recent ones; none at all: micro.
	const Account* A = AccountOf(L.Sites, S);
	Chips Sum = 0;
	int N = 0;
	Chips AllSum = 0;
	int AllN = 0;
	const double WeekStart = PlayDayStart(PlayWeek(PlayDay(World)) * 7);
	if (A)
	{
		for (const Line& Ln : A->Lines)
		{
			if (Ln.Kind != 0 || Ln.Cents >= 0 || Ln.Label.compare(0, 8, "Buy-in: ") != 0)
			{
				continue;
			}
			AllSum += -Ln.Cents;
			++AllN;
			if (Ln.At >= WeekStart)
			{
				Sum += -Ln.Cents;
				++N;
			}
		}
	}
	const Chips Avg = N > 0 ? Sum / N : AllN > 0 ? AllSum / AllN : 0;
	if (Avg <= 550)
	{
		return 1200 * ValuePerCent; // micro: $12 a week
	}
	if (Avg <= 5500)
	{
		return 6000 * ValuePerCent; // low: $60
	}
	if (Avg <= 53000)
	{
		return 30000 * ValuePerCent; // mid: $300
	}
	return 0; // high: the 50% rule alone
}

int64_t SitePlaySecondRunPending(const GovRing& G, const life::State& L, net::SiteId S, double World)
{
	// RunItTwice's Second Run is never held and is booked on the 1st into the slots of the month that earned it, so the
	// month's accruing rakeback (the Club's rate so far on this month's rake still in the ring) is reserved Room now: the
	// 1st's booking then lands inside the ceiling instead of lifting the ring past it.
	if (S != net::SiteId::RunItTwice || G.Day0 < 0)
	{
		return 0;
	}
	const Account* A = AccountOf(L.Sites, S);
	if (!A)
	{
		return 0;
	}
	const std::vector<TierInfo>& Tiers = SecondRunTiers();
	const int Permille = Tiers[static_cast<size_t>(TierFor(Tiers, A->RakeMonth))].Permille;
	if (Permille <= 0)
	{
		return 0;
	}
	const int Month = PlayMonth(PlayDay(World));
	int64_t Rake = 0;
	for (int D = G.Day0 - 29; D <= G.Day0; ++D)
	{
		Rake += PlayMonth(D) == Month ? G.Rake[static_cast<size_t>(SitePlaySlot(D))] : 0;
	}
	return Rake * Permille / 1000;
}

int64_t SitePlayRoom(const Book& B, const life::State& L, net::SiteId S, double World)
{
	const GovRing& G = B.Gov[static_cast<size_t>(SitePlayGovIndex(S))];
	const double Share = SitePlayCeiling(L, S, World) * static_cast<double>(SitePlayRake30(G));
	int64_t Room = static_cast<int64_t>(std::floor(Share)) - SitePlayPromo30(G) - G.Banked - SitePlaySecondRunPending(G, L, S, World);
	const int64_t Cap = SitePlayWeekCap(L, S, World);
	if (Cap > 0)
	{
		Room = std::min(Room, Cap - static_cast<int64_t>(G.WeekFace));
	}
	return std::max<int64_t>(0, Room);
}

void SitePlayBook(Book& B, net::SiteId S, int Day, int64_t Value, bool CountsInWeek, bool Spread)
{
	SitePlayRoll(B, S, Day);
	GovRing& G = B.Gov[static_cast<size_t>(SitePlayGovIndex(S))];
	if (Day <= G.Day0 - 30 || Value == 0)
	{
		return;
	}
	if (CountsInWeek)
	{
		G.WeekFace = static_cast<int32_t>(SitePlaySat(G.WeekFace, Value));
	}
	const size_t Today = static_cast<size_t>(SitePlaySlot(std::min(Day, G.Day0)));
	const int64_t Rake = SitePlayRake30(G);
	if (!Spread || Rake <= 0)
	{
		G.Promo[Today] = static_cast<int32_t>(SitePlaySat(G.Promo[Today], Value));
		return;
	}
	// A grant rides with the rake that earned it: its value spread over the ring's days in proportion to their rake (the
	// Second Run's rule, §1.3), so a lump never sits in one day while the rake around it leaves the window, and a player
	// who stops never reads past the ceiling. The remainder of the division lands today.
	int64_t Left = Value;
	for (size_t K = 0; K < G.Rake.size(); ++K)
	{
		const int64_t Part = Value * static_cast<int64_t>(G.Rake[K]) / Rake;
		G.Promo[K] = static_cast<int32_t>(SitePlaySat(G.Promo[K], Part));
		Left -= Part;
	}
	G.Promo[Today] = static_cast<int32_t>(SitePlaySat(G.Promo[Today], Left));
}

bool SitePlayHasItemRoom(const Book& B)
{
	int Items = 0;
	for (const Hold& H : B.Holds)
	{
		Items += !H.SparksOwed && !H.Reset ? 1 : 0;
	}
	// The "Sparks owed" line keeps its slot among the 12 even before it exists, so a Sparks grant can always queue: a
	// claimed ticket can take the 11 others. The resets' 3 reserved slots are on top (15 in all).
	return Items < SitePlayHoldItems - 1;
}

bool SitePlayGrant(Book& B, life::State& L, net::SiteId S, Kind K, const std::string& What, int64_t Value, const SitePlayPayload& Pay,
	const std::string& Key, int Day, double World, bool Reset)
{
	SitePlayTouch(L);
	SitePlayRoll(B, S, Day);
	const bool IsSparks = Pay.Sparks > 0 && Pay.TicketId.empty() && Pay.WalletCents == 0;
	// A ticket already waiting under this key never queues twice.
	const std::string HoldKey = IsSparks ? std::string() : std::string(1, SitePlaySiteChar(S)) + (Pay.TicketId.empty() ? ":w:" + std::to_string(Pay.WalletCents) : ":t:" + Pay.TicketId) + ":" + Key;
	bool Waiting = false;
	for (const Hold& H : B.Holds)
	{
		if (!HoldKey.empty() && H.Key == HoldKey)
		{
			return false;
		}
		Waiting = Waiting || SitePlayHoldSite(H) == S;
	}
	if (!Waiting && Value <= SitePlayRoom(B, L, S, World))
	{
		SitePlayBook(B, S, Day, Value, true, true);
		SitePlayPay(B, L, S, Pay, World);
		return true;
	}
	if (IsSparks)
	{
		for (Hold& H : B.Holds)
		{
			if (H.SparksOwed && SitePlayHoldSite(H) == S)
			{
				H.Cents += Value;
				H.Key = std::string(1, SitePlaySiteChar(S)) + ":s:" + std::to_string(SitePlayPayloadOf(H).Sparks + Pay.Sparks);
				return false;
			}
		}
		Hold H;
		H.Key = std::string(1, SitePlaySiteChar(S)) + ":s:" + std::to_string(Pay.Sparks);
		H.What = SitePlayGovLabels[0];
		H.K = K;
		H.Cents = Value;
		H.Day = Day;
		H.SparksOwed = true;
		B.Holds.push_back(H);
		return false;
	}
	Hold H;
	H.Key = HoldKey;
	H.What = What;
	H.K = K;
	H.Cents = Value;
	H.Day = Day;
	H.Reset = Reset;
	B.Holds.push_back(H);
	return false;
}

void SitePlayClearHolds(Book& B, life::State& L, net::SiteId S, int Day, double World)
{
	if (B.Holds.empty())
	{
		return;
	}
	SitePlayRoll(B, S, Day);
	// The oldest first: it takes all the spare Room until it's covered, and the next oldest goes on from what's left.
	for (size_t I = 0; I < B.Holds.size();)
	{
		Hold& H = B.Holds[I];
		if (SitePlayHoldSite(H) != S)
		{
			++I;
			continue;
		}
		const int64_t Spare = SitePlayRoom(B, L, S, World);
		const int64_t Take = std::min<int64_t>(Spare, H.Cents - H.Covered);
		if (Take > 0)
		{
			H.Covered += Take;
			SitePlayBook(B, S, Day, Take, true, true);
		}
		if (H.Covered < H.Cents)
		{
			break;
		}
		const SitePlayPayload Pay = SitePlayPayloadOf(H);
		const bool Owed = H.SparksOwed && S == net::SiteId::Ignite;
		B.Holds.erase(B.Holds.begin() + static_cast<std::ptrdiff_t>(I));
		SitePlayPay(B, L, S, Pay, World);
		if (Owed)
		{
			IgniteOwedCleared(B); // the claims that queued on it read paid now (M4)
		}
	}
	// Then any later hold that fits whole in what's left clears too, so nothing waits behind one that can't fit.
	bool Oldest = true;
	for (size_t I = 0; I < B.Holds.size();)
	{
		Hold& H = B.Holds[I];
		if (SitePlayHoldSite(H) != S)
		{
			++I;
			continue;
		}
		if (Oldest)
		{
			Oldest = false;
			++I;
			continue;
		}
		const int64_t Rest = H.Cents - H.Covered;
		if (Rest <= SitePlayRoom(B, L, S, World))
		{
			SitePlayBook(B, S, Day, Rest, true, true);
			const SitePlayPayload Pay = SitePlayPayloadOf(H);
			const bool Owed = H.SparksOwed && S == net::SiteId::Ignite;
			B.Holds.erase(B.Holds.begin() + static_cast<std::ptrdiff_t>(I));
			SitePlayPay(B, L, S, Pay, World);
			if (Owed)
			{
				IgniteOwedCleared(B);
			}
			continue;
		}
		++I;
	}
}

void SitePlayPay(Book& B, life::State& L, net::SiteId S, const SitePlayPayload& Pay, double World)
{
	if (Pay.Sparks > 0)
	{
		B.Ig.S.Balance += Pay.Sparks;
	}
	if (!Pay.TicketId.empty())
	{
		++L.Tickets[Pay.TicketId];
		SitePlayDailyLine(L, S, World, "Rewards: ", Pay.Label.empty() ? std::string("a ticket") : Pay.Label);
	}
	if (Pay.WalletCents > 0)
	{
		if (Account* A = AccountOf(L.Sites, S))
		{
			A->Cents += Pay.WalletCents;
			AddLine(L.Sites, S, World, 4, Pay.WalletCents, Pay.Label.empty() ? FillCopy(SitePlayGovLabels[5], {{"Amount", Money(Pay.WalletCents)}}) : Pay.Label);
		}
	}
}

void SitePlayDailyLine(life::State& L, net::SiteId S, double World, const std::string& Prefix, const std::string& One)
{
	Account* A = AccountOf(L.Sites, S);
	if (!A)
	{
		return;
	}
	const int Day = PlayDay(World);
	for (auto It = A->Lines.rbegin(); It != A->Lines.rend(); ++It)
	{
		if (PlayDay(It->At) != Day)
		{
			break;
		}
		if (It->Cents == 0 && It->Label.compare(0, Prefix.size(), Prefix) == 0)
		{
			const std::string Rest = It->Label.substr(Prefix.size());
			const int Was = !Rest.empty() && Rest[0] >= '0' && Rest[0] <= '9' ? std::max(1, std::atoi(Rest.c_str())) : 1;
			It->Label = Prefix + std::to_string(Was + 1) + " tickets";
			return;
		}
	}
	AddLine(L.Sites, S, World, 0, 0, Prefix + One);
}

void SitePlayTouch(life::State& L)
{
	L.Update3LinesRead |= life::U3SitePlay;
}

bool SitePlayIgniteStarted(const Book& B)
{
	const Ignite& I = B.Ig;
	return I.S.DayKey >= 0 || I.S.Balance != 0 || I.S.BaseLife != 0 || !I.Boxes.empty() || I.BandMonth >= 0 || I.F.LastDay >= 0 || fuse::HasRecord(I.Fuse);
}

bool SitePlayRitStarted(const Book& B)
{
	const Rit& R = B.Rt;
	return R.L.Month != -1 || !R.Stars.empty() || !R.Hist.empty() || R.Sc.Year != 0 || R.Lk.AllIns != 0 || R.Creds != 0 || !R.H.empty();
}

std::string SitePlayDated(const std::string& Key, double World)
{
	return Key + "@" + std::to_string(net::DayOf(World));
}

bool SitePlayDone(const site::Book& Sites, const std::string& Key)
{
	const std::string Prefix = Key + "@";
	const auto It = Sites.Done.lower_bound(Prefix);
	return Sites.Done.count(Key) > 0 || (It != Sites.Done.end() && It->compare(0, Prefix.size(), Prefix) == 0);
}

void SitePlayMarkDone(life::State& L, const std::string& Key, double World)
{
	L.Sites.Done.insert(SitePlayDated(Key, World));
}

double SitePlayUnit(uint64_t WorldSeed, const std::string& What)
{
	return static_cast<double>(Fnv1a(std::to_string(WorldSeed) + ":" + What)) / 4294967296.0;
}

double SitePlayDraw(uint64_t WorldSeed, const std::string& What)
{
	// FNV-1a's top bits barely move with a key's last characters (a day or an id's counter: "ignite:wheel:24350"), so its raw
	// unit clusters by the last digit and a prize table's chances drift (G2 fix M2: Neon's 3,000 Sparks came 7.0% of 6,000
	// keyed opens against the 5% its Odds show). The murmur3 finalizer (FUSE's FuseMix) spreads it; the key is §1.9's.
	uint32_t H = Fnv1a(std::to_string(WorldSeed) + ":" + What);
	H ^= H >> 16;
	H *= 0x85ebca6bu;
	H ^= H >> 13;
	H *= 0xc2b2ae35u;
	H ^= H >> 16;
	return static_cast<double>(H) / 4294967296.0;
}

std::string SitePlayCount(int64_t N)
{
	return Grouped(N);
}

void SitePlayMarkPaid(life::State& L, const std::string& EventId, double World)
{
	L.Sites.Done.insert(SitePlayDated("play:in:" + EventId, World));
}

bool SitePlayTakePaid(life::State& L, const std::string& EventId)
{
	const std::string Prefix = "play:in:" + EventId + "@";
	const auto It = L.Sites.Done.lower_bound(Prefix);
	if (It != L.Sites.Done.end() && It->compare(0, Prefix.size(), Prefix) == 0 && It->find('@', Prefix.size()) == std::string::npos)
	{
		L.Sites.Done.erase(It);
		return true;
	}
	return false;
}

Chips SitePlayCheapRake(net::SiteId S)
{
	const net::SiteDef& D = net::Def(S);
	return std::max<Chips>(1, D.Rake.Fee(S == net::SiteId::RunItTwice ? 330 : 110));
}

const char* SitePlayCheapName(net::SiteId S)
{
	return S == net::SiteId::RunItTwice ? "Hourly" : "Flip & Run";
}

std::string SitePlayTemplate(const std::string& EventId)
{
	const size_t At = EventId.find('@');
	std::string T = At == std::string::npos ? EventId : EventId.substr(0, At);
	const size_t Dot = T.find('.');
	if (Dot != std::string::npos && net::SiteOf(T) != net::SiteId::RiverLine)
	{
		T = T.substr(Dot + 1);
	}
	return T;
}

double SitePlayStartOf(const std::string& EventId)
{
	const size_t At = EventId.find('@');
	if (At == std::string::npos)
	{
		return -1.0;
	}
	return std::atof(EventId.c_str() + At + 1);
}

std::string SitePlayEscape(const std::string& In)
{
	std::string Out;
	for (const char Ch : In)
	{
		Out += Ch == '\\' ? std::string("\\\\") : Ch == '\n' ? std::string("\\n") : Ch == '\t' ? std::string("\\t") : Ch == ',' ? std::string("\\c") : std::string(1, Ch);
	}
	return Out;
}

std::string SitePlayUnescape(const std::string& In)
{
	std::string Out;
	for (size_t I = 0; I < In.size(); ++I)
	{
		if (In[I] == '\\' && I + 1 < In.size())
		{
			const char N = In[++I];
			Out.push_back(N == 'n' ? '\n' : N == 't' ? '\t' : N == 'c' ? ',' : N);
		}
		else
		{
			Out.push_back(In[I]);
		}
	}
	return Out;
}

int64_t SitePlayInt(const std::string& S)
{
	return S.empty() ? 0 : static_cast<int64_t>(std::strtoll(S.c_str(), nullptr, 10));
}

double SitePlayNum(const std::string& S)
{
	return S.empty() ? 0.0 : std::strtod(S.c_str(), nullptr);
}

std::string SitePlayNumText(double X)
{
	if (X == 0.0)
	{
		return "0";
	}
	char Buf[40];
	for (int Precision = 6; Precision <= 17; ++Precision)
	{
		std::snprintf(Buf, sizeof(Buf), "%.*g", Precision, X);
		if (std::strtod(Buf, nullptr) == X)
		{
			break;
		}
	}
	return Buf;
}

std::vector<std::string> SitePlaySplit(const std::string& S, char Sep)
{
	std::vector<std::string> Out;
	if (S.empty())
	{
		return Out;
	}
	size_t From = 0;
	while (true)
	{
		const size_t At = S.find(Sep, From);
		Out.push_back(S.substr(From, At == std::string::npos ? std::string::npos : At - From));
		if (At == std::string::npos)
		{
			break;
		}
		From = At + 1;
	}
	return Out;
}
} // namespace siteplay_detail

// ------------------------------------------------------------------ the loops' clock

int PlayDay(double World)
{
	return net::DayOf(World - siteplay_detail::SitePlayDayStartsAt);
}

int PlayWeek(int Day)
{
	return Day >= 0 ? Day / 7 : -((-Day + 6) / 7);
}

int PlayMonth(int Day)
{
	return world::MonthKeyOf(Day);
}

double PlayDayStart(int Day)
{
	return static_cast<double>(Day) * siteplay_detail::SitePlayDayMinutes + siteplay_detail::SitePlayDayStartsAt;
}

int64_t SparksValue(int64_t Sparks)
{
	// 7,000 Sparks buy a $1.10 ticket: a Spark is 110 / 7000 cents, 110,000 / 7000 thousandths.
	return (Sparks * 110 * ValuePerCent + 3500) / 7000;
}

Chips RakeCents30(const Book& B, net::SiteId S)
{
	return static_cast<Chips>(siteplay_detail::SitePlayRake30(B.Gov[static_cast<size_t>(siteplay_detail::SitePlayGovIndex(S))]) / ValuePerCent);
}

Chips PromoCents30(const Book& B, net::SiteId S)
{
	return static_cast<Chips>(siteplay_detail::SitePlayPromo30(B.Gov[static_cast<size_t>(siteplay_detail::SitePlayGovIndex(S))]) / ValuePerCent);
}

Chips PromoCentsOn(const Book& B, net::SiteId S, int Day)
{
	const GovRing& G = B.Gov[static_cast<size_t>(siteplay_detail::SitePlayGovIndex(S))];
	if (G.Day0 < 0 || Day > G.Day0 || Day <= G.Day0 - 30)
	{
		return 0;
	}
	const int M = Day % 30;
	return static_cast<Chips>(G.Promo[static_cast<size_t>(M < 0 ? M + 30 : M)] / ValuePerCent);
}

bool PaidEntry(const life::State& L, const std::string& EventId)
{
	const std::string Prefix = "play:in:" + EventId + "@";
	const auto It = L.Sites.Done.lower_bound(Prefix);
	return It != L.Sites.Done.end() && It->compare(0, Prefix.size(), Prefix) == 0 && It->find('@', Prefix.size()) == std::string::npos;
}

// ------------------------------------------------------------------ the Governor (§1.4)

bool Grant(Book& B, life::State& L, net::SiteId S, Kind K, const std::string& What, Chips ValueCents, const std::string& Key, int Day, double World)
{
	using namespace siteplay_detail;
	if (S != net::SiteId::Ignite && S != net::SiteId::RunItTwice)
	{
		return false;
	}
	// The two that are never held and never paid here (their cents move in SiteMoney.cpp, pinned): the Governor only counts
	// them. The Second Run in the slots of the month that earned it (Day: that month's last 6 AM day; the account's RakeMonth
	// is still the closing month's when the close calls); Ignite's welcome release on the day of the rake that released it.
	if (K == Kind::SecondRun)
	{
		const Account* A = AccountOf(L.Sites, S);
		siteplay_detail::RitBookSecondRunPay(B, L, ValueCents, PlayMonth(Day), A ? static_cast<double>(A->RakeMonth) : 0.0, World);
		return true;
	}
	if (K == Kind::Welcome)
	{
		siteplay_detail::SitePlayTouch(L);
		siteplay_detail::SitePlayBook(B, S, Day, ValueCents * ValuePerCent, false, false);
		return true;
	}
	// A cash-valued grant at face: Sparks kinds pay Sparks at Prize Counter value (Ignite only), a ticket kind the ticket
	// named before the key's "|" ("runittwice.the-sweat|league:24323"), anything else cents into the site's wallet.
	SitePlayPayload Pay;
	const int64_t Value = ValueCents * ValuePerCent;
	const bool SparksKind = K == Kind::BaseSparks || K == Kind::DareSparks || K == Kind::WheelSparks || K == Kind::CrackerSparks || K == Kind::OuchSparks ||
		K == Kind::PaybackSparks || K == Kind::ScalpSparks;
	const size_t Bar = Key.find('|');
	std::string Idem = Key;
	if (SparksKind && S == net::SiteId::Ignite)
	{
		Pay.Sparks = std::max<int64_t>(1, ValueCents * 7000 / 110);
	}
	else if ((K == Kind::Ticket || K == Kind::LeagueTicket || K == Kind::ScalpTicket) && Bar != std::string::npos)
	{
		Pay.TicketId = Key.substr(0, Bar);
		Pay.Label = What;
		Idem = Key.substr(Bar + 1);
	}
	else
	{
		Pay.WalletCents = ValueCents;
		Pay.Label = What;
	}
	// Keyed: pays once (the contract). The loops' own grants key their last-day integers inside the book; this door's
	// callers (the dev verb, any later caller) get a dated done key, so a repeat of the same key pays nothing (review: two
	// calls with one key paid two tickets).
	const std::string Done = "play:g:" + std::string(1, SitePlaySiteChar(S)) + ":" + Idem;
	if (SitePlayDone(L.Sites, Done))
	{
		return false;
	}
	SitePlayMarkDone(L, Done, World);
	return SitePlayGrant(B, L, S, K, What, Value, Pay, Idem, Day, World, false);
}

double GivenBack30(const Book& B, net::SiteId S)
{
	using namespace siteplay_detail;
	const GovRing& G = B.Gov[static_cast<size_t>(SitePlayGovIndex(S))];
	const int64_t Rake = SitePlayRake30(G);
	return Rake > 0 ? static_cast<double>(SitePlayPromo30(G)) / static_cast<double>(Rake) : 0.0;
}

// ------------------------------------------------------------------ the hooks

void OnPaidEntry(Book& B, life::State& L, net::SiteId S, Chips Fee, Chips BuyIn, int Format, const std::string& EventId, double World)
{
	using namespace siteplay_detail;
	(void)Format; // the loops read the event's template themselves (FUSE's id has none)
	if ((S != net::SiteId::Ignite && S != net::SiteId::RunItTwice) || BuyIn <= 0)
	{
		return;
	}
	Account* A = AccountOf(L.Sites, S);
	if (!A)
	{
		return;
	}
	SitePlayTouch(L);
	const int Day = PlayDay(World);
	SitePlayRoll(B, S, Day);
	GovRing& G = B.Gov[static_cast<size_t>(SitePlayGovIndex(S))];
	const size_t Slot = static_cast<size_t>(SitePlaySlot(Day));
	G.Rake[Slot] = static_cast<int32_t>(std::min<int64_t>(std::numeric_limits<int32_t>::max(), static_cast<int64_t>(G.Rake[Slot]) + std::max<Chips>(0, Fee) * ValuePerCent));
	SitePlayMarkPaid(L, EventId, World);
	// Ignite's welcome release this rake triggered (site::OnBuyIn ran just before, §1.3): booked on the rake's day, never held.
	if (A->Lines.size() >= 2)
	{
		const Line& Bonus = A->Lines.back();
		const Line& Entry = A->Lines[A->Lines.size() - 2];
		if (Bonus.Kind == 4 && Bonus.At == World && Bonus.Cents > 0 && Bonus.Label == CopyText("label:wallet:bonus") && Entry.Kind == 0 && Entry.At == World && Entry.Cents == -BuyIn)
		{
			SitePlayBook(B, S, Day, Bonus.Cents * ValuePerCent, false, false);
		}
	}
	if (S == net::SiteId::Ignite)
	{
		IgnitePaidEntry(B, L, Fee, BuyIn, fuse::IsFuse(EventId), EventId, World);
	}
	else
	{
		RitPaidEntry(B, L, Fee, BuyIn, EventId, World);
	}
	SitePlayClearHolds(B, L, S, Day, World);
}

void OnResult(Book& B, life::State& L, const SettleFacts& F, double World)
{
	using namespace siteplay_detail;
	if (F.Site != net::SiteId::Ignite && F.Site != net::SiteId::RunItTwice)
	{
		return;
	}
	const bool Paid = SitePlayTakePaid(L, F.EventId);
	if (F.Fuse)
	{
		// FUSE's record (core-fuse's NoteResult): Played, Won, the Wall and Big hits, paid or on a ticket.
		SitePlayTouch(L);
		fuse::NoteResult(B.Ig.Fuse, F, World);
	}
	if (!Paid)
	{
		// A ticket, a freeroll or a package's entry moves no loop (W4); RunItTwice's credentials still read the result.
		if (F.Site == net::SiteId::RunItTwice && SitePlayRitStarted(B))
		{
			RitResult(B, L, F, false, World);
		}
		return;
	}
	if (F.Site == net::SiteId::Ignite)
	{
		IgniteResult(B, L, F, true, World);
	}
	else
	{
		RitResult(B, L, F, true, World);
	}
	SitePlayClearHolds(B, L, F.Site, PlayDay(World), World);
}

void OnHand(Book& B, net::SiteId S, const HandNote& N, double World)
{
	using namespace siteplay_detail;
	// The book's own look at a hand (the Luck Ledger, the hand Dares, the stickers): no grant, so no life needed.
	if (!N.Paid)
	{
		return;
	}
	if (S == net::SiteId::Ignite && SitePlayIgniteStarted(B))
	{
		IgniteHand(B, nullptr, N, World);
	}
	else if (S == net::SiteId::RunItTwice && SitePlayRitStarted(B))
	{
		RitHand(B, nullptr, N, World);
	}
}

void OnHandPaid(Book& B, life::State& L, net::SiteId S, const HandNote& N, double World)
{
	using namespace siteplay_detail;
	if (!N.Paid)
	{
		return;
	}
	if (S == net::SiteId::Ignite)
	{
		IgniteHand(B, &L, N, World);
	}
	else if (S == net::SiteId::RunItTwice)
	{
		RitHand(B, &L, N, World);
	}
}

namespace siteplay_detail
{
namespace
{
/** One instant of a step: the book brought up to At (Step runs it at each 6 AM a window crosses, then at the window's end). */
void SitePlayStepAt(Book& B, life::State& L, const world::World* W, const sponsor::Book& Deals, uint64_t WorldSeed, bool Ig, bool Rt, double From, double At,
	bool End, StepOut& Out)
{
	const int Day = PlayDay(At);
	if (Ig)
	{
		IgniteRoll(B, L, &Deals, WorldSeed, Day, At);
	}
	if (Rt || B.Gov[2].Day0 >= 0)
	{
		RitBookSecondRun(B, L, From, At);
	}
	if (Rt)
	{
		RitStep(B, L, W, &Deals, WorldSeed, From, At, End, Out);
	}
	for (const net::SiteId S : {net::SiteId::Ignite, net::SiteId::RunItTwice})
	{
		const GovRing& G = B.Gov[static_cast<size_t>(SitePlayGovIndex(S))];
		if (G.Day0 >= 0 && Day >= G.Day0)
		{
			SitePlayRoll(B, S, Day);
			SitePlayClearHolds(B, L, S, Day, At);
		}
	}
}
} // namespace
} // namespace siteplay_detail

StepOut Step(Book& B, life::State& L, const site::Book& Sites, const world::World* W, const sponsor::Book& Deals, uint64_t WorldSeed, double From, double To)
{
	using namespace siteplay_detail;
	(void)Sites; // the life's own (L.Sites): one book
	StepOut Out;
	if (To < From)
	{
		return Out;
	}
	// Never creates state (S10): only a book a paid entry or a claim already made moves. Idempotent over any span: every
	// rule below keys a last-day integer (the day, the week, the month, the Matchweek), so overlapping windows and a
	// WorldSkip's catch-up step pay nothing twice. Each 6 AM the window crosses is stepped at its own minute before To, so a
	// skip of N days is N one-day steps: the 6 AM Dare credit, the Wristband's 1st, an anniversary's Cracker, Monday's bank,
	// the League's 1st and each day's spare Room for the Guest List land on their own day, against that day's ring (review:
	// one step over 40 days held the Dares a day-by-day player was paid and lost the anniversary Cracker in the gap).
	const bool Ig = SitePlayIgniteStarted(B);
	const bool Rt = SitePlayRitStarted(B);
	if (!Ig && !Rt && B.Gov[1].Day0 < 0 && B.Gov[2].Day0 < 0)
	{
		return Out;
	}
	const int Day = PlayDay(To);
	for (int D = std::max(PlayDay(From) + 1, Day - 400); D <= Day; ++D)
	{
		const double At = PlayDayStart(D);
		if (At > From && At < To)
		{
			SitePlayStepAt(B, L, W, Deals, WorldSeed, Ig, Rt, From, At, false, Out);
		}
	}
	SitePlayStepAt(B, L, W, Deals, WorldSeed, Ig, Rt, From, To, true, Out);
	return Out;
}

// ------------------------------------------------------------------ the claims (Ignite's in SitePlayIgnite.cpp, RunItTwice's
// in SitePlayRit.cpp; these are the doors, each keyed and idempotent)

std::string Spin(Book& B, life::State& L, uint64_t WorldSeed, double World)
{
	return siteplay_detail::IgniteSpin(B, L, WorldSeed, World);
}

std::string Open(Book& B, life::State& L, uint64_t WorldSeed, const std::string& BoxId, double World)
{
	return siteplay_detail::IgniteOpen(B, L, WorldSeed, BoxId, World);
}

std::string ClaimDare(Book& B, life::State& L, int Slot, double World)
{
	return siteplay_detail::IgniteClaimDare(B, L, Slot, World);
}

std::string ClaimTier(Book& B, life::State& L, int Tier, double World)
{
	return siteplay_detail::IgniteClaimTier(B, L, Tier, World);
}

std::string Buy(Book& B, life::State& L, int Item, double World)
{
	return siteplay_detail::IgniteBuy(B, L, Item, World);
}

std::string Equip(Book& B, int Slot, int Look)
{
	return siteplay_detail::IgniteEquip(B, Slot, Look);
}

std::string Star(Book& B, const std::string& TemplateId, bool On)
{
	return siteplay_detail::RitStar(B, TemplateId, On);
}

std::string CallOut(Book& B, int Npc, double World)
{
	return siteplay_detail::RitCallOut(B, Npc, World);
}

// ------------------------------------------------------------------ the view

View ViewOf(const Book& B, const life::State& L, const site::Book& Sites, const world::World* W, net::SiteId S, double World)
{
	using namespace siteplay_detail;
	(void)Sites;
	View V;
	if (S != net::SiteId::Ignite && S != net::SiteId::RunItTwice)
	{
		return V;
	}
	// The Governor's figure at this instant (the Fine print, the Statement): a copy of the ring rolled to today, so a
	// draw never moves the book (S10).
	Book Rolled;
	Rolled.Gov = B.Gov;
	Rolled.Ig.S.Deferred = B.Ig.S.Deferred;
	const int Day = PlayDay(World);
	const int Gi = SitePlayGovIndex(S);
	if (Rolled.Gov[static_cast<size_t>(Gi)].Day0 >= 0)
	{
		SitePlayRoll(Rolled, S, Day);
	}
	const GovRing& G = Rolled.Gov[static_cast<size_t>(Gi)];
	GovView& Gv = V.Gov;
	Gv.Rake30 = static_cast<Chips>(SitePlayRake30(G) / ValuePerCent);
	Gv.Promo30 = static_cast<Chips>(SitePlayPromo30(G) / ValuePerCent);
	Gv.Share = SitePlayRake30(G) > 0 ? static_cast<double>(SitePlayPromo30(G)) / static_cast<double>(SitePlayRake30(G)) : 0.0;
	Gv.Welcome = SitePlayWelcome(L, S, World);
	Gv.Ceiling = SitePlayCeiling(L, S, World);
	Gv.WeekFace = G.WeekKey == PlayWeek(Day) ? static_cast<Chips>(G.WeekFace / ValuePerCent) : 0;
	Gv.WeekCap = static_cast<Chips>(SitePlayWeekCap(L, S, World) / ValuePerCent);
	Gv.MondayBank = S == net::SiteId::Ignite ? Rolled.Ig.S.Deferred : 0;
	Gv.Room = static_cast<Chips>(SitePlayRoom(Rolled, L, S, World) / ValuePerCent);
	const double Ceil = Gv.Ceiling;
	int64_t Ahead = 0; // value queued ahead of each hold, still to cover
	int Items = 0;
	for (const Hold& H : B.Holds)
	{
		Items += !H.SparksOwed && !H.Reset ? 1 : 0;
		if (SitePlayHoldSite(H) != S)
		{
			continue;
		}
		HoldView Hv;
		Hv.What = H.What;
		Hv.Cents = H.Cents / ValuePerCent;
		Hv.Covered = H.Covered / ValuePerCent;
		Hv.Day = H.Day;
		Hv.SparksOwed = H.SparksOwed;
		Hv.Reset = H.Reset;
		Hv.Sparks = SitePlayPayloadOf(H).Sparks;
		Ahead += H.Cents - H.Covered;
		// Each cent of rake adds the ceiling's share of Room: what this line and everything ahead of it still need.
		const int64_t Need = std::max<int64_t>(0, Ahead - SitePlayRoom(Rolled, L, S, World));
		Hv.RakeLeft = static_cast<Chips>(std::ceil(static_cast<double>(Need) / Ceil / static_cast<double>(ValuePerCent)));
		const Chips Cheap = SitePlayCheapRake(S);
		Hv.Entries = Hv.RakeLeft > 0 ? static_cast<int>((Hv.RakeLeft + Cheap - 1) / Cheap) : 0;
		Gv.Holds.push_back(Hv);
	}
	Gv.ItemsFree = std::max(0, SitePlayHoldItems - 1 - Items);
	// The months the ring holds (the Statement's lines, each dated by the month that earned it) and the rake by day.
	if (G.Day0 >= 0)
	{
		Gv.FirstDay = G.Day0 - 29;
		for (int K = 0; K < 30; ++K)
		{
			const int D = G.Day0 - 29 + K;
			const size_t Slot = static_cast<size_t>(D % 30 < 0 ? D % 30 + 30 : D % 30);
			Gv.RakeByDay[static_cast<size_t>(K)] = G.Rake[Slot] / ValuePerCent;
			const int Mk = PlayMonth(D);
			if (Gv.Months.empty() || Gv.Months.back().MonthKey != Mk)
			{
				GovMonth M;
				M.MonthKey = Mk;
				M.Name = MonthName(MonthKeyAt(PlayDayStart(D)));
				Gv.Months.push_back(M);
			}
			Gv.Months.back().Rake += G.Rake[Slot] / ValuePerCent;
			Gv.Months.back().Promo += G.Promo[Slot] / ValuePerCent;
		}
	}
	const int Pct = static_cast<int>(std::llround(Gv.Share * 100.0));
	Gv.Line = FillCopy(SitePlayGovSentences[0], {{"Site", net::Def(S).Name}, {"Promo", Money(Gv.Promo30)}, {"Rake", Money(Gv.Rake30)}, {"Pct", std::to_string(Pct) + "%"}});
	const uint64_t Seed = SitePlaySeedOf(W);
	if (S == net::SiteId::Ignite)
	{
		IgniteFillView(B, L, &L.Sponsors, W, Seed, World, V.Ig);
	}
	else
	{
		RitFillView(B, L, &L.Sponsors, W, Seed, World, V.Rt);
	}
	return V;
}

std::vector<std::string> Inventory()
{
	using namespace siteplay_detail;
	std::vector<std::string> Sentences(std::begin(SitePlayGovSentences), std::end(SitePlayGovSentences));
	std::vector<std::string> Labels;
	IgniteCopy(Sentences, Labels);
	RitCopy(Sentences, Labels);
	return Sentences;
}

std::vector<std::string> InventoryLabels()
{
	using namespace siteplay_detail;
	std::vector<std::string> Sentences;
	std::vector<std::string> Labels(std::begin(SitePlayGovLabels), std::end(SitePlayGovLabels));
	IgniteCopy(Sentences, Labels);
	RitCopy(Sentences, Labels);
	return Labels;
}

// ------------------------------------------------------------------ the save lines (§7.4)

void WriteLines(const life::State& L, std::string& Out)
{
	using namespace siteplay_detail;
	// Pure const (the save worker runs it); each line only when it holds something, so an untouched book writes nothing.
	const Book& B = L.SitePlay;
	IgniteWrite(B, Out);
	RitWrite(B, Out);
	for (int S = 1; S <= 2; ++S)
	{
		const GovRing& G = B.Gov[static_cast<size_t>(S)];
		if (G.Day0 < 0)
		{
			continue;
		}
		Out += "site\tgov\t" + std::string(net::KeyOf(static_cast<net::SiteId>(S))) + "\t" + std::to_string(G.Day0);
		for (const int32_t R : G.Rake)
		{
			Out += "\t" + std::to_string(R);
		}
		for (const int32_t P : G.Promo)
		{
			Out += "\t" + std::to_string(P);
		}
		Out += "\t" + std::to_string(G.WeekKey) + "\t" + std::to_string(G.WeekFace) + "\t" + std::to_string(G.Banked) + "\n";
	}
	for (const Hold& H : B.Holds)
	{
		Out += "site\thold\t" + std::string(net::KeyOf(SitePlayHoldSite(H))) + "\t" + SitePlayEscape(H.Key) + "\t" + std::to_string(static_cast<int>(H.K)) + "\t" +
			SitePlayEscape(H.What) + "\t" + std::to_string(H.Cents) + "\t" + std::to_string(H.Covered) + "\t" + std::to_string(H.Day) + "\t" + (H.SparksOwed ? "1" : "0") +
			"\t" + (H.Reset ? "1" : "0") + "\n";
	}
}

bool ReadLine(const std::vector<std::string>& P, life::State& L)
{
	using namespace siteplay_detail;
	if (P.size() < 3 || P[0] != "site")
	{
		return false;
	}
	Book& B = L.SitePlay;
	const std::string& Tag = P[1];
	if (Tag == "fuse")
	{
		return fuse::ReadLine(P, B.Ig.Fuse);
	}
	if (Tag == "gov" && P.size() >= 3 + 1 + 60 + 3)
	{
		const net::SiteId S = P[2] == net::KeyOf(net::SiteId::RunItTwice) ? net::SiteId::RunItTwice : net::SiteId::Ignite;
		GovRing& G = B.Gov[static_cast<size_t>(SitePlayGovIndex(S))];
		G.Day0 = static_cast<int>(SitePlayInt(P[3]));
		for (size_t K = 0; K < 30; ++K)
		{
			G.Rake[K] = static_cast<int32_t>(SitePlayInt(P[4 + K]));
			G.Promo[K] = static_cast<int32_t>(SitePlayInt(P[34 + K]));
		}
		G.WeekKey = static_cast<int>(SitePlayInt(P[64]));
		G.WeekFace = static_cast<int32_t>(SitePlayInt(P[65]));
		G.Banked = static_cast<int32_t>(SitePlayInt(P[66]));
		return true;
	}
	if (Tag == "hold" && P.size() >= 11)
	{
		Hold H;
		H.Key = SitePlayUnescape(P[3]);
		const int K = static_cast<int>(SitePlayInt(P[4]));
		H.K = K >= 0 && K < static_cast<int>(Kind::Count) ? static_cast<Kind>(K) : Kind::Ticket;
		H.What = SitePlayUnescape(P[5]);
		H.Cents = SitePlayInt(P[6]);
		H.Covered = SitePlayInt(P[7]);
		H.Day = static_cast<int>(SitePlayInt(P[8]));
		H.SparksOwed = P[9] == "1";
		H.Reset = P[10] == "1";
		B.Holds.push_back(H);
		return true;
	}
	return IgniteRead(P, B) || RitRead(P, B);
}
} // namespace play
} // namespace site
} // namespace ss
