#include "ShortStack/Game/Rep.h"
#include "../StrictFloat.h"
#include "SitesDetail.h"

#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/Game/World.h"

#include <algorithm>
#include <climits>
#include <cmath>

// The public record (next_segment_spec §1.1). Owned by the lead.
//
// What the world can know of the player, in one place, from the life record, the channel and the world. Private things
// (stakes, makeup, the bankroll, debts, tilt) never enter it (N3). The card room builds it from a SaveData with no world.

namespace ss
{
namespace rep
{
namespace rep_detail
{
constexpr double DayMinutes = 1440.0;

/** stake::Conduct::Marks kinds the world hears about (word of mouth): 1 late, 2 welched, 3 walked (core-stake writes them). */
constexpr int MarkLate = 1;
constexpr int MarkWelched = 2;
constexpr int MarkWalked = 3;

bool MarkedSince(const stake::Conduct& C, int Kind, double Since)
{
	return std::any_of(C.Marks.begin(), C.Marks.end(), [Kind, Since](const std::pair<double, int>& M) { return M.second == Kind && M.first >= Since; });
}

/** Rerun's month (update 2 §4.7: Views30). */
constexpr double RepViewsDays = 30.0;

/**
 * The views Rerun's channel gained in the 30 days to World, in the channel's own closed forms: every kept video and every
 * evergreen stub whole, and the folded tails from the oldest kept upload on. Every folded video went up before that one,
 * so from there the tails' sums are exact. Before it they aren't: a channel posting more than two a day folds videos
 * younger than a month, and a tail read back past their uploads runs their curves backwards (e^(age/tau) of views that
 * never happened). That stretch isn't counted, so the number is always views that happened: exact for any channel whose
 * folded videos are all older than the month, a little under for a heavy uploader, never over (a sponsor's letter
 * quotes it). An empty channel is 0.
 */
double RepViews30(const video::Channel& V, double World)
{
	const double From = World - RepViewsDays * DayMinutes;
	double Sum = 0.0;
	double Oldest = V.SteppedTo;
	for (const video::Video& X : V.Videos)
	{
		Sum += std::max(0.0, video::Views(X, World) - video::Views(X, From));
		Oldest = std::min(Oldest, X.At);
	}
	if (!V.Stubs.empty())
	{
		video::Channel Stubs;
		Stubs.Stubs = V.Stubs;
		Sum += std::max(0.0, video::TotalViews(Stubs, World) - video::TotalViews(Stubs, From));
	}
	video::Channel Tails;
	Tails.Tails = V.Tails;
	Tails.SteppedTo = V.SteppedTo;
	Sum += std::max(0.0, video::TotalViews(Tails, World) - video::TotalViews(Tails, std::max(From, Oldest)));
	return Sum;
}

/** A count of people for the record: whole ones, never negative, never past an int. */
int RepWhole(double Count)
{
	return Count > 0.0 ? static_cast<int>(std::min(static_cast<double>(INT_MAX), std::floor(Count))) : 0;
}
} // namespace rep_detail

Public Of(const life::State& L, const kast::Channel& C, const world::World* W, double World)
{
	using namespace rep_detail;
	Public P;
	const int Today = net::DayOf(World);

	// Online: the player's stats page, the live events it also counts taken out, and the freerolls too (S9): a result that
	// cost nothing has no ROI, so the count, the ROI and the ABI are the paid events' (Tracker::Paid*). Every freeroll is
	// online (the Embercrest has none), so the live line comes off the paid totals whole.
	if (W)
	{
		const world::Tracker& T = W->HeroStats();
		const world::TrackLine& Live = T.ByStake[static_cast<size_t>(world::TrackStake::Live)];
		P.OnlineEvents = std::max(0, T.PaidEvents() - Live.Events);
		const Chips BuyIns = T.BuyIns - Live.BuyIns;
		const Chips Prizes = T.PaidPrizes() - Live.Prizes;
		P.OnlineRoi = BuyIns > 0 ? static_cast<double>(Prizes - BuyIns) / static_cast<double>(BuyIns) : 0.0;
		P.Abi = P.OnlineEvents > 0 ? BuyIns / P.OnlineEvents : 0;
		P.ProfitAbi = P.Abi > 0 ? static_cast<double>(Prizes - BuyIns) / static_cast<double>(P.Abi) : 0.0;
		// Honors: the trophy case, and the titles the world records for the player (-1).
		for (const world::Award& A : W->HeroAwards())
		{
			(A.Ring ? P.Rings : P.Bracelets) += 1;
		}
		for (const world::Honor& H : W->Honors())
		{
			if (H.Npc != -1)
			{
				continue;
			}
			if (H.Title.find("Player of the Year") != std::string::npos)
			{
				++P.Poy;
			}
			else
			{
				++P.Majors;
			}
		}
	}

	// Live: the life record's totals and its days (not the kept entries alone: Register keeps only the last sixty).
	P.LiveEvents = L.LiveEvents;
	P.LiveCashes = L.LiveCashes;
	P.LiveBest = L.LiveBestPlace;
	P.Titles = L.LiveWins;
	P.LiveSample = L.LiveEvents; // every live event has a buy-in (no freerolls at the Embercrest)
	P.LiveRoi = L.LiveSpentCents > 0 ? static_cast<double>(L.LiveWonCents - L.LiveSpentCents) / static_cast<double>(L.LiveSpentCents) : 0.0;
	for (const life::LiveDay& D : L.LiveDays)
	{
		const int Ago = Today - D.Day;
		if (Ago < 0)
		{
			continue;
		}
		P.LivePrizes365 += Ago < 365 ? D.WonCents : 0;
		if (D.Embercrest > 0)
		{
			P.EmbercrestDays.push_back(D.Day);
		}
		if (Ago < 60)
		{
			P.Embercrest60 += D.Embercrest;
			P.Cashes60 += D.EmbercrestCashes;
		}
		if (Ago < 90)
		{
			P.Embercrest90 += D.Embercrest;
			P.Cashes90 += D.EmbercrestCashes;
		}
	}
	for (const life::LiveEntry& E : L.LiveEntries)
	{
		if (E.State == life::LiveEntry::Finished && E.Place > 0 && live::IsEmbercrest(E.Id))
		{
			P.BestEmbercrestPlace = P.BestEmbercrestPlace <= 0 ? E.Place : std::min(P.BestEmbercrestPlace, E.Place);
		}
	}

	// Reach: the channel.
	P.Followers = C.Followers;
	P.Superfans = C.Superfans();
	P.Partner = C.Partner;
	double Minutes = 0.0;
	double ViewerMinutes = 0.0;
	for (const kast::StreamLog& S : C.Log)
	{
		if (S.Start >= World - 30.0 * DayMinutes && S.Minutes > 0.0)
		{
			Minutes += S.Minutes;
			ViewerMinutes += S.Minutes * static_cast<double>(S.Avg);
		}
	}
	P.Avg30 = Minutes > 0.0 ? ViewerMinutes / Minutes : 0.0;

	// Reach on Rerun (update 2 §4.7): the channel as anyone could see it tonight, in closed form (its saved numbers are as
	// of the last day stepped). From the life record alone, so the card room's SaveData-only build gets the same. An old
	// save with no videos reads 0, 0 and no Partner, and its renown is what it was.
	const video::Channel& Rerun = L.Video;
	P.VideoSubs = RepWhole(video::SubsAt(Rerun, World));
	P.Views30 = RepWhole(RepViews30(Rerun, World));
	P.VideoPartner = Rerun.PartnerAt >= 0.0 && Rerun.PartnerAt <= World;

	// The rival sites (update 2 §5.4): the accounts the player holds. The invitation gates' last-hundred numbers come from
	// History, which the session holds (FromHistory, below; Session::Record and SiteStep add them).
	P.SiteAccounts = static_cast<int>(L.Sites.Accounts.size());

	// Conduct the world can know: the paper (a pickup, 90 days), the site (a restriction, 180), word of mouth (365).
	for (const life::LedgerEntry& X : L.Ledger)
	{
		P.PickedUp90 = P.PickedUp90 || (X.Kind == 2 && X.Label == "Fine (picked up)" && X.At >= World - 90.0 * DayMinutes);
	}
	P.Restricted180 = L.BannedUntil > 0.0 && L.BannedUntil >= World - 180.0 * DayMinutes;
	const stake::Conduct& K = L.Stakes.Record;
	P.Late90 = MarkedSince(K, MarkLate, World - 90.0 * DayMinutes);
	P.Welched180 = MarkedSince(K, MarkWelched, World - 180.0 * DayMinutes);
	P.Welched365 = MarkedSince(K, MarkWelched, World - 365.0 * DayMinutes);
	P.Walked365 = MarkedSince(K, MarkWalked, World - 365.0 * DayMinutes);

	P.Renown = RenownOf(P);
	return P;
}

void FromHistory(Public& P, const std::vector<HistoryEntry>& History)
{
	// History holds the last hundred results (update 2 §5.4): the average buy-in of the paid ones (a freeroll costs nothing and
	// an old save's unknown buy-in says nothing), and the online series titles among them, on any site (the template on the
	// event's own site; an old result with no id by its name, as StatsFrom reads them).
	Chips Spent = 0;
	int Paid = 0;
	int Titles = 0;
	for (const HistoryEntry& E : History)
	{
		if (E.BuyInCents > 0)
		{
			Spent += E.BuyInCents;
			++Paid;
		}
		if (E.Place != 1)
		{
			continue;
		}
		const size_t Cut = E.EventId.rfind('@');
		const net::EventTemplate* T = Cut == std::string::npos ? nullptr : net::ForEvent(E.EventId).FindTemplate(E.EventId.substr(0, Cut));
		Titles += (T && !T->Series.empty()) || (!T && (E.Name.rfind("MM #", 0) == 0 || E.Name.rfind("RCOP #", 0) == 0)) ? 1 : 0;
	}
	P.Abi100 = Paid > 0 ? Spent / Paid : 0;
	P.OnlineSeriesTitles = Titles;
	P.Paid100 = Paid;
}

double RenownOf(const Public& P)
{
	// Town.cpp's formula: followers on a log scale, superfans, live cashes, titles on the wall, trophies. Update 2 (§4.7):
	// Rerun's subscribers count in the follower term at half a Kast follower each (they know the face, not the voice), so
	// the street and the backers follow video fame too. With no subscribers the term is the old one to the bit.
	const double Reach = static_cast<double>(std::max(0, P.Followers)) + static_cast<double>(std::max(0, P.VideoSubs)) / 2.0;
	const double Followers = std::min(1.0, std::log10(1.0 + Reach) / 5.0);
	const double Fans = std::min(1.0, static_cast<double>(P.Superfans) / 40.0);
	const double Cashes = std::min(1.0, static_cast<double>(P.LiveCashes) / 15.0);
	const double Wins = std::min(1.0, static_cast<double>(P.Titles) / 4.0);
	const double Awards = std::min(1.0, static_cast<double>(P.Bracelets + P.Rings) / 2.0);
	return std::min(1.0, 0.35 * Followers + 0.15 * Fans + 0.15 * Cashes + 0.15 * Wins + 0.2 * Awards);
}

const char* Mood(double Margin)
{
	// How a counter reads before it's sent (§1.9 #25): Margin is how far inside what they'd take it is (stake::Margin,
	// sponsor::Leverage's room), positive inside.
	if (Margin >= 0.0)
	{
		return "Sounds open to it";
	}
	return Margin >= -0.15 ? "That's a stretch" : "Don't push it";
}
} // namespace rep
} // namespace ss
