// The creator update (SPEC_CREATORS §10.2): Kast's Session glue, core-kastglue's file. Every body behind the lead's hooks in
// Session.cpp: KastInputsExtra (the deepest run over all tables with its event's facts, the front table, prestige, series,
// buy-in, the cached all-in equity, Attended, Rerun's Heat bumps, the live night, the seed, the rents paid, the deals' reads),
// the new studio verbs, KastBeforeStart (the migration's seeding), KastAfterStart (U3Kast, the one-time letter), KastNewCareer
// (U3Kast and Heat as of now for a new career), KastAfterStop (sponsor pay as LedgerSponsors lines, the first payout's toast,
// the milestone stings' notices), KastDaily, the forecast and the rank Studio shows, and DebugKast's body for ss.Kast.* (§10.5).
// Before an old save's first stream under this build, the forecast (and KastHeatAtGoLive, for Studio's Heat chip and why lines)
// previews what that go-live will seed, on a copy or on the numbers alone (the C-K gate's L8).
//
// The tables (Inputs::FrontTable, ThisTable, DeepTable) are the open tables' indices (Session::FocusedTable's numbering, 0 to
// TableLimit - 1): unique among the open tables at any moment and small enough for §2.12's keys (kind x 8 + table, stage x 64 +
// table). While a table behind takes its turn (Session::WithTable) it is the session's table for that call: ThisTable is its
// index and FrontTable is -1, so nothing it fires reads as the front table's.
#include "ShortStack/Game/Session.h"

#include "ShortStack/Game/Autoplay.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Handles.h"
#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/KastGrowth.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/VideoAlgo.h"
#include "ShortStack/Rng.h"
#include "ShortStack/Tournament.h"

#include "KastCopy.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace session_kast_detail
{
/** ss.Kast.*'s words: the verb as Update3Commands passes it (in lower case) and its arguments as typed. */
std::vector<std::string> KastWords(const std::string& Args)
{
	std::vector<std::string> Out;
	std::string Word;
	for (const char C : Args)
	{
		if (std::isspace(static_cast<unsigned char>(C)))
		{
			if (!Word.empty())
			{
				Out.push_back(Word);
				Word.clear();
			}
			continue;
		}
		Word.push_back(C);
	}
	if (!Word.empty())
	{
		Out.push_back(Word);
	}
	return Out;
}

std::string KastLower(std::string Text)
{
	for (char& C : Text)
	{
		C = static_cast<char>(std::tolower(static_cast<unsigned char>(C)));
	}
	return Text;
}

constexpr double KastDayMinutes = 1440.0;

/** A span of world days in minutes, drawn from R (ss.Kast.Grant's members: when they followed, when they last watched). */
double KastDebugDays(Rng& R, double Lo, double Hi)
{
	return R.Range(Lo, Hi) * KastDayMinutes;
}

/** Rerun's Heat bumps Kast hasn't seen yet are the ones since the last go-live; a live night that cashed reaches Dee's line
 *  for 48 world hours (§2.2). */
constexpr double KastLiveNightMinutes = 48.0 * 60.0;

/** §2.6's Series: 1.0 a series' main event or a weekly major (the calendar's Sunday events, outside any series), 0.5 any other
 *  series event (a bracelet or a ring is a series event), 0 the rest; from the event's template on its own site. */
double KastSeriesOf(const Tournament& Tn)
{
	const std::string& Id = Tn.Spec.Id;
	const size_t At = Id.find('@');
	const net::EventTemplate* Tmpl = At == std::string::npos ? nullptr : net::ForEvent(Id).FindTemplate(Id.substr(0, At));
	if (!Tmpl)
	{
		return 0.0;
	}
	const bool SundayOnly = Tmpl->Days == (1 << 6) && Tmpl->OnlyDay == -9999;
	const bool WeeklyMajor = SundayOnly && Tmpl->Series.empty() && Tmpl->Fmt != net::Format::Satellite;
	if (Tmpl->Main || WeeklyMajor)
	{
		return 1.0;
	}
	return !Tmpl->Series.empty() || Tmpl->Bracelet || Tmpl->Ring ? 0.5 : 0.0;
}

/** The last day the channel was live, from the days it kept and its log; -1 for one that never was. */
int KastLastDayLive(const kast::Channel& Ch)
{
	int Last = -1;
	for (const int D : Ch.DaysLive)
	{
		Last = std::max(Last, D);
	}
	if (!Ch.Log.empty())
	{
		Last = std::max(Last, static_cast<int>(std::floor(Ch.Log.front().Start / KastDayMinutes)));
	}
	return Last;
}

/** A channel that has never gone live (no stream, no log row): a new one, not migrated (PHASE_AK_EXIT §6 item 1). */
bool KastNeverLive(const kast::Channel& Ch, int StreamsBefore)
{
	return StreamsBefore <= 0 && Ch.Log.empty();
}

/** §8.3: an old channel at its first stream under the new model, seeded from what it kept. Heat 26 if it streamed in the last
 *  7 world days (else 15), as of now; Norm the last ten streams' average; Bias 0; RunNorm 0.15; the record average from the log;
 *  carries 0; the welcome raid done past stream 3; nothing pending. Its last day live too, so Heat's floor and Welcome back read
 *  the channel's real history at the first go-live, and its first stranger's tip already behind it when it had tips beyond
 *  Dee's first-night five (a sting must never claim a first that wasn't). */
void KastMigrate(kast::Channel& Ch, double World)
{
	const int LastDay = KastLastDayLive(Ch);
	// "Streamed in the last 7 world days" by world day, as growth::HeatShown's floor reads it (the same channel, one rule).
	const bool Recent = LastDay >= 0 && net::DayOf(World) - LastDay <= 7;
	Ch.Heat = Recent ? 26.0 : 15.0;
	Ch.HeatAt = World;
	Ch.Norm = Ch.AvgViewers();
	Ch.Bias = 0.0;
	Ch.RunNorm = 0.15;
	int Best = 0;
	for (const kast::StreamLog& Lg : Ch.Log)
	{
		Best = std::max(Best, Lg.Avg);
	}
	Ch.BestAvg = Best;
	Ch.LastStreamDay = LastDay;
	Ch.CarryFollow = Ch.CarryWord = Ch.CarrySub = Ch.CarryTip = Ch.CarryCheer = Ch.CarryRaid = Ch.CarryGift = 0.0;
	Ch.WelcomeRaidDone = Ch.Streams >= 3;
	Ch.PendingCents = 0;
	Ch.LiveSince = -1.0;
	Ch.GainSoFar = 0.0;
	if (Ch.EarnedTips > 500)
	{
		Ch.Firsts |= kast::growth::FirstStrangerTip;
	}
}

/** Whether the next go-live migrates the channel (§8.3): an old channel (it streamed before, or kept a log) this build hasn't put
 *  under the new model yet (no U3Kast). */
bool KastMigratesAtGoLive(const kast::Channel& Ch, int Update3LinesRead)
{
	return (Update3LinesRead & life::U3Kast) == 0 && !KastNeverLive(Ch, Ch.Streams);
}

/** What a go-live seeds before Stream::Start (KastBeforeStart's body; KastForecast previews it on a copy). A channel that never
 *  went live is new, not migrated (a new career's too, however many days its rig took): Heat as a new channel reads it (its
 *  start, 26), as of now, so the first night commits from there (§2.4); no letter. An old channel not under the new model yet is
 *  seeded from what it kept (KastMigrate). Anything else is the new model's already and is left as it is. */
void KastSeedAtGoLive(kast::Channel& Ch, int Update3LinesRead, double World)
{
	if (KastNeverLive(Ch, Ch.Streams))
	{
		Ch.Heat = kast::growth::HeatShown(Ch.Heat, Ch.HeatAt, World, -1);
		Ch.HeatAt = World;
		return;
	}
	if (KastMigratesAtGoLive(Ch, Update3LinesRead))
	{
		KastMigrate(Ch, World);
	}
}

/**
 * Heat as the next go-live will seed it, without touching the channel (the C-K gate's L8): for an old channel the next go-live
 * migrates, §8.3's Heat (26 if it streamed in the last 7 world days, else 15) as of World and its last day live from the days
 * it kept and its log; any other channel's own. Off air, Studio's Heat chip and why lines and ss.Kast.Describe read it, so an old
 * save dark 8+ days reads Cold before its first stream under this build, as it will once live. Returns whether it's a preview.
 */
bool KastHeatAtGoLive(const kast::Channel& Ch, int Update3LinesRead, double World, double& Heat, double& HeatAt, int& LastStreamDay)
{
	Heat = Ch.Heat;
	HeatAt = Ch.HeatAt;
	LastStreamDay = Ch.LastStreamDay;
	if (!KastMigratesAtGoLive(Ch, Update3LinesRead))
	{
		return false;
	}
	// KastMigrate's Heat lines, on the numbers alone (no copy of the community).
	LastStreamDay = KastLastDayLive(Ch);
	Heat = LastStreamDay >= 0 && net::DayOf(World) - LastStreamDay <= 7 ? 26.0 : 15.0;
	HeatAt = World;
	return true;
}

/** What the channel had at the go-live, for tonight's milestone stings at Stop (KastAfterStart writes it, KastAfterStop reads
 *  and drops it). Kept per session on the session's own thread: Session.h is frozen, and a stream lives on one thread. */
struct KastStartFacts
{
	int RaidsIn = 0;
	uint32_t Firsts = 0;
};
std::map<const void*, KastStartFacts>& KastStartsHere()
{
	thread_local std::map<const void*, KastStartFacts> Starts;
	return Starts;
}

/**
 * K17's end-card stings for tonight's one-time firsts that had no live alert of their own: the first stranger's tip and the
 * first raid in, each once a career. A first the Room posted live tonight (Summary::Firsts: its line, its alert and its KastShine)
 * has had its sound, so it gets no sting here (one sound a milestone; the C-K gate's M3, fix-room's live post).
 */
std::vector<std::string> KastEndStings(uint32_t FirstsBefore, int RaidsInBefore, const kast::Channel& Ch, const kast::Summary& Sum)
{
	bool TipLive = false;
	bool RaidLive = false;
	for (const std::string& Line : Sum.Firsts)
	{
		TipLive = TipLive || kast::copy::IsFirstStrangerTip(Line);
		RaidLive = RaidLive || kast::copy::IsFirstRaidIn(Line);
	}
	std::vector<std::string> Out;
	if (!TipLive && (FirstsBefore & kast::growth::FirstStrangerTip) == 0 && (Ch.Firsts & kast::growth::FirstStrangerTip) != 0)
	{
		Out.push_back(kast::copy::StingStrangerTip());
	}
	if (!RaidLive && RaidsInBefore == 0 && Ch.RaidsIn > 0)
	{
		Out.push_back(kast::copy::StingFirstRaid());
	}
	return Out;
}

kast::Notice KastSting(const std::string& Body)
{
	kast::Notice N;
	N.Type = kast::Notice::Kind::Milestone;
	N.From = "Kast";
	N.Body = Body;
	N.Sound = static_cast<int>(SoundId::KastShine);
	return N;
}

/** The channel's kast\t lines as the save would write them (ss.Kast.Advance's comparison: one step of N days against N of one). */
std::string KastLinesOf(const kast::Channel& Ch, const life::State& L, double World)
{
	SaveData D;
	D.Channel = Ch;
	D.Life.Update3LinesRead = L.Update3LinesRead;
	D.Life.RentsPaid = L.RentsPaid;
	D.ClockMinutes = World - net::MinutesPerDay * static_cast<double>(net::NightOneDay);
	std::istringstream In(D.Serialize());
	std::string Line;
	std::string Out;
	while (std::getline(In, Line))
	{
		if (Line.rfind("kast\t", 0) == 0)
		{
			Out += Line + "\n";
		}
	}
	return Out;
}

/** The first line two texts differ at (ss.Kast.Advance), "" when they're the same. */
std::string KastFirstDiff(const std::string& A, const std::string& B)
{
	std::istringstream Ia(A);
	std::istringstream Ib(B);
	std::string La;
	std::string Lb;
	while (true)
	{
		const bool Ga = static_cast<bool>(std::getline(Ia, La));
		const bool Gb = static_cast<bool>(std::getline(Ib, Lb));
		if (!Ga && !Gb)
		{
			return "";
		}
		if (!Ga || !Gb || La != Lb)
		{
			return (Ga ? La : std::string("(nothing)")).substr(0, 120) + "  vs  " + (Gb ? Lb : std::string("(nothing)")).substr(0, 120);
		}
	}
}
} // namespace session_kast_detail

// ------------------------------------------------------------------ the studio's verbs (§2.9, §5.3)

bool Session::StreamNod(const std::string& Name)
{
	return Stream.Live && Stream.Nod(Channel, Name);
}

bool Session::StreamRailCall()
{
	if (!Stream.Live || !Stream.RailCall(Channel, StreamInputs()))
	{
		return false;
	}
	// Once a world day (GrantMask): saved now, so a reload the same day can't call the rail again.
	Save();
	return true;
}

bool Session::StreamWelcomeRaid()
{
	return Stream.Live && Stream.WelcomeRaid(Channel);
}

bool Session::StreamSetScene()
{
	return Stream.Live && Stream.SetTheScene(Channel);
}

bool Session::StreamCallIt(bool Holds)
{
	return Stream.Live && Stream.CallIt(Channel, Holds);
}

bool Session::StreamClipIt()
{
	// The end card's Clip of the Night: after the stream (the Stream decides whether it's open: Summary::ClipItOpen).
	if (Stream.Live || !Stream.ClipIt(Channel, StreamInputs()))
	{
		return false;
	}
	Save();
	return true;
}

kast::growth::Band Session::KastForecast() const
{
	// Off air before an old save's first stream under this build, the forecast reads the channel as the go-live will seed it
	// (§8.3, KastBeforeStart), on a copy: nothing here touches the channel (the C-K gate's L8). Only a migration moves what
	// Stream::Forecast reads (a never-live channel's seeded Heat is the start HeatShown already reads for it), so only then is the
	// community copied, once a world minute while Studio is open, and only until that first stream.
	if (!Stream.Live && session_kast_detail::KastMigratesAtGoLive(Channel, Life.Update3LinesRead))
	{
		kast::Channel AtGoLive = Channel;
		session_kast_detail::KastSeedAtGoLive(AtGoLive, Life.Update3LinesRead, WorldMinutes());
		return kast::Stream::Forecast(AtGoLive, StreamInputs());
	}
	return kast::Stream::Forecast(Channel, StreamInputs());
}

kast::growth::Standing Session::KastRank() const
{
	return kast::growth::RankAt(WorldMinutes(), Stream.Live ? Stream.Count() : 0);
}

// ------------------------------------------------------------------ the hooks (§10.2)

void Session::KastInputsExtra(kast::Inputs& In) const
{
	In.Seed = WorldSeed();
	In.RentsPaid = Life.RentsPaid;

	// The tables: the one this call is about is the session's (a table behind is swapped in while it takes its turn); the one in
	// front is Active, except during such a turn, when nothing the call fires is the front table's.
	const int Open = TableCount();
	In.ThisTable = Open > 0 ? Active : -1;
	In.FrontTable = Open > 0 && !Background ? Active : -1;

	// Attended (§2.9): the pace leaves the player real seconds. Every open table's clock counts (the room's clock is the one
	// furthest along), so one table skipping, sprinting or watched at 4x or faster is a heads-down night; the lobby, the results
	// screen, Smart, Full, watch 2x and an autoplay stop (the skip has stopped: Smart, watched at 1x) are attended.
	auto HeadsDown = [](const auto& Src) {
		return Src.Sprinting || Src.CurrentPace == Pace::Sprint || (Src.Auto.M == autoplay::Mode::Watch && Src.Auto.Speed >= 4);
	};
	// The deepest run (§2.6): among the tables whose hero is still in (a won event stays live until its results screen) with an
	// armed field, the largest P x depth; the first table on a tie.
	double BestScore = -1.0;
	auto Consider = [&](const auto& Src, int Index) {
		if (!Src.T)
		{
			return;
		}
		const Tournament& Tn = *Src.T;
		if (Tn.Hero().Busted || (Index == Active && CurrentScreen == Screen::Results))
		{
			return;
		}
		const int Entrants = Tn.Spec.Entrants;
		if (!kast::growth::Armed(Entrants))
		{
			return;
		}
		const double Series = session_kast_detail::KastSeriesOf(Tn);
		const double P = kast::growth::Prestige(Entrants, Tn.Spec.BuyInCents, Series);
		const double D = kast::growth::Depth(Entrants, Tn.Remaining);
		const double Score = P * D;
		if (Score <= BestScore)
		{
			return;
		}
		BestScore = Score;
		In.DeepTable = Index;
		In.DeepTournament = Tn.Spec.Id;
		In.DeepEvent = Tn.Spec.Name;
		In.DeepSite = static_cast<int>(net::SiteOf(Tn.Spec.Id));
		In.DeepEntrants = Entrants;
		In.DeepRemaining = Tn.Remaining;
		In.DeepTableSize = Tn.Spec.TableSize;
		In.DeepPaid = Tn.PaidPlaces();
		In.Depth = D;
		In.Prestige = P;
		In.Series = Series;
		In.BuyInCents = Tn.Spec.BuyInCents;
	};
	bool Down = TimeSkip.Active;
	for (int K = 0; K < Open; ++K)
	{
		if (K == Active)
		{
			Down = Down || (T && HeadsDown(*this));
			Consider(*this, K);
		}
		else if (const TableRun* Run = Runs[static_cast<size_t>(K)].get())
		{
			Down = Down || (Run->T && HeadsDown(*Run));
			Consider(*Run, K);
		}
	}
	In.Attended = !Down;

	// The hero's equity at the all-in, as UpdateEquities cached it on the seat at the reveal (never computed here).
	if (HeroAllInReveal)
	{
		for (const SeatVis& Sv : Seats)
		{
			if (Sv.Present && Sv.IsHero && Sv.HasEquity)
			{
				In.AllInEquity = Sv.Equity;
			}
		}
	}

	// The series running now (the Tide's x1.3 and "RCOP week: Poker is busy"): the calendar's, only while today is inside it.
	if (const net::SeriesInfo* S = net::Shared().CurrentSeries(In.World))
	{
		const int Today = net::DayOf(In.World);
		if (Today >= S->FirstDay && Today <= S->LastDay)
		{
			In.SeriesWindow = S->Short.empty() ? S->Name : S->Short;
		}
	}

	// A live Embercrest night that cashed in the last 48 world hours (Dee asks about it).
	for (const life::LiveEntry& E : Life.LiveEntries)
	{
		const double Ago = In.World - E.FinishedAt;
		if (E.State == life::LiveEntry::Finished && E.PrizeCents > 0 && live::IsEmbercrest(E.Id) && Ago >= 0.0 && Ago <= session_kast_detail::KastLiveNightMinutes)
		{
			In.LiveNightRecent = true;
		}
	}

	// Rerun's Heat bumps since the last go-live (Start applies them): tonight's while live; else since the latest go-live the
	// channel or this session knows of. After a reload a short stream leaves no log row (and a restart within 4 world hours no
	// ping), so its day is the floor: Rerun's bumps land on world midnights, and the last go-live took every one up to its own
	// day's (a bump handed twice would warm Heat twice).
	double Since = -1.0e18;
	if (Stream.Live)
	{
		Since = Stream.StartWorld;
	}
	else
	{
		Since = std::max(Since, Channel.PingAt);
		Since = std::max(Since, Channel.LiveSince);
		if (Channel.LastStreamDay >= 0)
		{
			Since = std::max(Since, static_cast<double>(Channel.LastStreamDay) * session_kast_detail::KastDayMinutes);
		}
		if (!Channel.Log.empty())
		{
			Since = std::max(Since, Channel.Log.front().Start);
		}
		if (Stream.Last.Valid)
		{
			Since = std::max(Since, Stream.StartWorld);
		}
	}
	In.RerunHeat = video::algo::KastHeatSince(Life.Video, Since);

	// C13's cap: each deal's paid reads this contract period and its reads ask, from the book (Sponsor.cpp is frozen).
	for (kast::Deal& Dl : In.Deals)
	{
		for (const sponsor::Contract& C : Life.Sponsors.Contracts)
		{
			if (C.Brand == Dl.Id && C.Since == Dl.Since)
			{
				Dl.ReadsPaid = C.Now.Reads;
				Dl.ReadsAsk = static_cast<int>(std::lround(C.T.Asks[static_cast<size_t>(sponsor::Ask::Reads)]));
				break;
			}
		}
	}
}

void Session::KastBeforeStart()
{
	if (Stream.Live)
	{
		return;
	}
	// A new channel's Heat as of now, or §8.3's seeding of an old one (KastSeedAtGoLive; KastForecast previews the same).
	session_kast_detail::KastSeedAtGoLive(Channel, Life.Update3LinesRead, WorldMinutes());
}

void Session::KastAfterStart()
{
	if (!Stream.Live)
	{
		return;
	}
	const double World = WorldMinutes();
	if ((Life.Update3LinesRead & life::U3Kast) == 0)
	{
		Life.Update3LinesRead |= life::U3Kast;
		// Start counted tonight's stream: the channel was an old one if it had streamed before, or kept a log (§8.3, C5, Q-K3).
		if (!session_kast_detail::KastNeverLive(Channel, Channel.Streams - 1))
		{
			life::Letter M;
			M.At = World;
			M.Thread = "kast";
			M.Source = "kast";
			M.From = kast::copy::LetterFrom();
			M.Subject = kast::copy::LetterSubject();
			M.Body = kast::copy::LetterBody();
			M.Kind = 1;
			life::Post(Life, M);
		}
	}
	session_kast_detail::KastStartFacts Facts;
	Facts.RaidsIn = Channel.RaidsIn;
	Facts.Firsts = Channel.Firsts;
	session_kast_detail::KastStartsHere()[this] = Facts;
}

void Session::KastNewCareer()
{
	Life.Update3LinesRead |= life::U3Kast;
	Channel.HeatAt = WorldMinutes();
}

void Session::KastAfterStop()
{
	const double World = WorldMinutes();
	const kast::Summary& Sum = Stream.Last;

	// C13: tonight's sponsor pay, each deal on its own ledger lines (never inside the Kast payout, which pays only Kast money).
	for (const kast::Summary::SponsorPay& Pay : Sum.SponsorPaid)
	{
		const sponsor::Brand* Br = sponsor::FindBrand(Pay.Id);
		const std::string Short = Br ? Br->Short : Pay.Id;
		if (Pay.ReadCents > 0)
		{
			BankrollCents += Pay.ReadCents;
			Life.Record(World, kast::copy::LedgerRead(Short, Pay.Reads), Pay.ReadCents, life::LedgerSponsors);
		}
		if (Pay.HourCents > 0)
		{
			BankrollCents += Pay.HourCents;
			Life.Record(World, kast::copy::LedgerHours(Short), Pay.HourCents, life::LedgerSponsors);
		}
	}

	// C14: the payout below releases what waited for the second rent: once a career, so it says so.
	if ((Life.Update3LinesRead & life::U3Kast) != 0 && Channel.PendingCents > 0)
	{
		const kast::growth::Payout Split = kast::growth::PayoutOf(Channel.UnpaidCents, Channel.PendingCents, Life.RentsPaid);
		if (Split.Pending < Channel.PendingCents && Split.Pay > 0)
		{
			kast::Notice N;
			N.Type = kast::Notice::Kind::Text;
			N.From = kast::copy::LetterFrom();
			N.Body = kast::copy::FirstPayout(Split.Pay);
			Stream.Notices.push_back(N);
		}
	}

	// K17: tonight's first-time milestones without a live alert of their own (the first stranger's tip, the first raid in), a
	// KastShine through the cue queue. The follower steps, Affiliate and Partner play KastShine live with their alert and its
	// confetti (Stream::CheckGrowth; the C-K gate's ruling: one sound a milestone, never a Win live and a sting at the end card);
	// so does a first the Room posted live (Summary::Firsts), which KastEndStings leaves out.
	std::map<const void*, session_kast_detail::KastStartFacts>& Starts = session_kast_detail::KastStartsHere();
	const auto Was = Starts.find(this);
	if (Was == Starts.end() || !Sum.Valid)
	{
		return;
	}
	const session_kast_detail::KastStartFacts Before = Was->second;
	Starts.erase(Was);
	for (const std::string& Body : session_kast_detail::KastEndStings(Before.Firsts, Before.RaidsIn, Channel, Sum))
	{
		Stream.Notices.push_back(session_kast_detail::KastSting(Body));
	}
}

void Session::KastDaily(double /*From*/, double /*To*/)
{
	// P0's one job (the C-K gate): Offline holds renewals as PendingCents before the second rent (C14), but a channel this build
	// hasn't migrated yet (no U3Kast: an old save that hasn't streamed under it) writes no kast\tmo, so a held renewal would be
	// lost at the next load. Until its first stream migrates it, such a channel keeps today's rule: the renewal waits in
	// UnpaidCents for the next payout. (§5.2: the Monday letter, "Your week on Kast", is P1 and lands here too.)
	if ((Life.Update3LinesRead & life::U3Kast) == 0 && Channel.PendingCents > 0)
	{
		Channel.UnpaidCents += Channel.PendingCents;
		Channel.PendingCents = 0;
	}
}

// ------------------------------------------------------------------ ss.Kast.* (§10.5)

std::string Session::DebugKast(const std::string& Args)
{
	using session_kast_detail::KastLower;
	const std::vector<std::string> W = session_kast_detail::KastWords(KastLower(Args));
	const std::string Verb = W.empty() ? std::string("describe") : W[0];
	const double World = WorldMinutes();
	const int Today = net::DayOf(World);
	auto Arg = [&W](size_t I, double Default) { return I < W.size() ? std::atof(W[I].c_str()) : Default; };

	if (Verb == "describe")
	{
		const kast::Channel& Ch = Channel;
		int Casual = 0;
		int Regular = 0;
		int Superfan = 0;
		int Tracked = 0;
		for (const kast::Member& M : Ch.Members)
		{
			Superfan += M.Loyalty >= kast::SuperfanLoyalty ? 1 : 0;
			Regular += M.Loyalty >= kast::RegularLoyalty && M.Loyalty < kast::SuperfanLoyalty ? 1 : 0;
			Casual += M.Loyalty < kast::RegularLoyalty ? 1 : 0;
			Tracked += M.Follower ? 1 : 0;
		}
		std::string Out = "Kast: " + Grouped(Ch.Followers) + " followers, " + Grouped(static_cast<int64_t>(Ch.Members.size())) + " in the community (" + Grouped(Casual) + " casual, " +
			Grouped(Regular) + " regulars, " + Grouped(Superfan) + " superfans); the lurker pool " + Grouped(std::max(0, Ch.Followers - Tracked)) + " untracked followers.";
		Out += "\nStreams " + std::to_string(Ch.Streams) + ", " + Fixed(Ch.MinutesLive / 60.0, 1) + " hours live; peak " + std::to_string(Ch.Peak) + ", average " +
			Fixed(Ch.AvgViewers(), 1) + " over the last ten, record average " + std::to_string(Ch.BestAvg) + ".";
		Out += "\nLevel " + std::to_string(Ch.Level()) + ", stage " + std::to_string(Ch.Stage) + ", streak " + std::to_string(Ch.Streak) + "; " +
			(Ch.Partner ? std::string("Partner") : Ch.Affiliate ? std::string("Affiliate") : std::string("not Affiliate yet")) + ".";
		Out += "\nSubs " + std::to_string(Ch.ActiveSubs(World)) + " active, mods " + std::to_string(Ch.Mods.size()) + ", clips " + std::to_string(Ch.Clips.size()) + "; balance " +
			Money(Ch.UnpaidCents) + ", pending " + Money(Ch.PendingCents) + " (rents paid " + std::to_string(Life.RentsPaid) + "), paid out " + Money(Ch.PaidCents) + ".";
		const double Raw = kast::growth::HeatAt(Ch.Heat, Ch.HeatAt, World);
		const double Shown = kast::growth::HeatShown(Ch.Heat, Ch.HeatAt, World, Ch.LastStreamDay);
		Out += "\nHeat " + Fixed(Raw, 1) + " raw, " + Fixed(Shown, 1) + " shown (" + kast::copy::TierName(kast::growth::Tier(Shown)) + "); this week's gains " +
			Fixed(Ch.GainUsed, 1) + " (week " + std::to_string(Ch.GainWeek) + "); Norm " + Fixed(Ch.Norm, 2) + ", Bias " + Fixed(Ch.Bias, 3) + ", RunNorm " + Fixed(Ch.RunNorm, 3) +
			"; last live day " + std::to_string(Ch.LastStreamDay) + " (today " + std::to_string(Today) + ").";
		// An old save before its first stream under this build (L8): what that go-live seeds, which the forecast and the why lines
		// below already read. Off air, Heat's why lines as Studio's Heat tile shows them (L10); live, the room's (below).
		double SeedHeat = Ch.Heat;
		double SeedAt = Ch.HeatAt;
		int SeedDay = Ch.LastStreamDay;
		if (session_kast_detail::KastHeatAtGoLive(Ch, Life.Update3LinesRead, World, SeedHeat, SeedAt, SeedDay))
		{
			const double SeedShown = kast::growth::HeatShown(SeedHeat, SeedAt, World, SeedDay);
			Out += "\nAn old channel, not under the new model yet: its next go-live seeds Heat " + Fixed(SeedHeat, 1) + " (" + kast::copy::TierName(kast::growth::Tier(SeedShown)) +
				"), Norm " + Fixed(Ch.AvgViewers(), 2) + ", last live day " + std::to_string(SeedDay) + "; the forecast and the why lines read that.";
		}
		if (!Stream.Live)
		{
			const std::array<std::string, 2> Why = kast::copy::WhyOffline(Ch, SeedDay, World);
			Out += "\nWhy (off air): " + (Why[0].empty() ? std::string("nothing to say yet") : Why[0] + (Why[1].empty() ? std::string() : " / " + Why[1])) + ".";
		}
		const double Hours7 = kast::growth::H7(Ch.LiveMin, Ch.LiveDay, Today);
		std::string Ring;
		for (const int M : Ch.LiveMin)
		{
			Ring += (Ring.empty() ? "" : " ") + std::to_string(M);
		}
		int Tokens = 0;
		for (const auto& G : Ch.Goodwill)
		{
			Tokens += std::min(3, G.second);
		}
		// Channel::RerunHeat holds the bumps waiting for the next go-live and (a negative amount) what this world week has
		// already given against its +12 (the Room's, review 2026-10-25).
		int RerunWaiting = 0;
		double RerunGiven = 0.0;
		for (const std::pair<double, double>& B : Ch.RerunHeat)
		{
			RerunWaiting += B.second > 0.0 ? 1 : 0;
			RerunGiven += B.second < 0.0 ? -B.second : 0.0;
		}
		Out += "\nStamina " + Fixed(kast::growth::Stamina(Hours7), 2) + " (" + Fixed(Hours7, 1) + " hours live in 7 days; the ring " + Ring + " to day " + std::to_string(Ch.LiveDay) +
			"); raid tokens " + std::to_string(Tokens) + " across " + std::to_string(Ch.Goodwill.size()) + " channels; the welcome raid " +
			(Ch.WelcomeRaidDone ? "done" : "still to come") + "; Rerun's Heat waiting " + std::to_string(RerunWaiting) + ", given this week " + Fixed(RerunGiven, 1) + " of 12.";
		const kast::growth::Band Fc = KastForecast();
		Out += "\nForecast for a stream now: " + Fixed(Fc.Lo, 1) + " to " + Fixed(Fc.Hi, 1) + " (centre " + Fixed(Fc.Centre, 1) + ").";
		if (Stream.Live)
		{
			const kast::View& V = Stream.GetView();
			std::string Sources;
			for (size_t I = 0; I < V.Sources.size(); ++I)
			{
				if (V.Sources[I] > 0)
				{
					Sources += (Sources.empty() ? "" : ", ") + std::to_string(V.Sources[I]) + " " + kast::copy::SourceTag(static_cast<kast::View::Source>(I));
				}
			}
			const kast::growth::Standing Rk = KastRank();
			Out += "\nLive " + Fixed(Stream.Uptime(World), 0) + " world minutes: " + std::to_string(Stream.Count()) + " watching (" + (Sources.empty() ? std::string("nobody") : Sources) +
				"), " + std::to_string(V.Chatting) + " chatting, peak " + std::to_string(Stream.Peak) + "; #" + std::to_string(Rk.Rank) + " of " + std::to_string(Rk.Of) +
				"; tonight's gain so far " + Fixed(V.GhostGain, 1) + (V.Why[0].empty() ? std::string() : "; why: " + V.Why[0] + (V.Why[1].empty() ? std::string() : " / " + V.Why[1])) + ".";
		}
		return Out;
	}
	if (Verb == "grant")
	{
		// ss.Kast.Grant <followers> [regulars] [subs]: the channel grows to at least that many followers, regulars and active
		// subscribers: members at a loyalty spread (regulars 0.35 up, the rest casual), followers past the member cap as the
		// lurker pool, subscribers from the most loyal; Affiliate once it has the bar's followers (subs need it).
		const int WantFollowers = std::max(0, static_cast<int>(Arg(1, 0.0)));
		const int WantRegulars = std::max(0, static_cast<int>(Arg(2, 0.0)));
		const int WantSubs = std::max(0, static_cast<int>(Arg(3, 0.0)));
		if (Channel.Streams == 0 && Channel.Members.empty())
		{
			// A channel that never went live gets its first night's two friends first, as Stream::Start seats them in an empty
			// community, so a grant before the first stream keeps First Faces (§2.3.3: Dee and Mei, there from the first night).
			for (const std::pair<const char*, std::pair<double, double>>& F : {std::make_pair("dee_spincycle", std::make_pair(0.85, 0.95)), std::make_pair("mei_ng", std::make_pair(0.7, 0.85))})
			{
				kast::Member M;
				M.Name = F.first;
				M.Loyalty = F.second.first;
				M.Affinity = F.second.second;
				M.FirstSeen = World;
				M.LastSeen = World;
				M.Follower = true;
				M.Friend = true;
				Channel.Members.push_back(M);
			}
			Channel.Followers += 2;
		}
		Rng Pick("kast-grant:" + std::to_string(WorldSeed()) + ":" + std::to_string(Channel.Members.size()) + ":" + std::to_string(Channel.Followers));
		std::set<std::string> Taken = {HeroName};
		for (const kast::Member& M : Channel.Members)
		{
			Taken.insert(M.Name);
		}
		auto NewName = [&Pick, &Taken]() {
			for (int Tries = 0; Tries < 50; ++Tries)
			{
				const std::string Name = handles::Make(Pick, handles::PickCountry(Pick));
				if (Taken.insert(Name).second)
				{
					return Name;
				}
			}
			const std::string Name = "kastfan_" + std::to_string(Taken.size());
			Taken.insert(Name);
			return Name;
		};
		auto Add = [&](double Loyalty) {
			kast::Member M;
			M.Name = NewName();
			M.Loyalty = Loyalty;
			M.Affinity = std::min(1.0, Loyalty + 0.05 + 0.2 * Pick.Next());
			M.Follower = true;
			M.FirstSeen = World - session_kast_detail::KastDebugDays(Pick, 10.0, 90.0);
			M.LastSeen = World - session_kast_detail::KastDebugDays(Pick, 0.5, Loyalty >= kast::RegularLoyalty ? 6.0 : 30.0);
			M.Streams = 1 + static_cast<int>(std::lround(Loyalty * 40.0));
			M.WatchMinutes = static_cast<double>(M.Streams) * 45.0 * (0.3 + Loyalty);
			Channel.Members.push_back(M);
			++Channel.Followers;
		};
		int Regulars = 0;
		for (const kast::Member& M : Channel.Members)
		{
			Regulars += M.Loyalty >= kast::RegularLoyalty ? 1 : 0;
		}
		const int Followers0 = Channel.Followers;
		const size_t Members0 = Channel.Members.size();
		int AddedRegulars = 0;
		while (Regulars < WantRegulars && static_cast<int>(Channel.Members.size()) < kast::MaxMembers)
		{
			Add(kast::RegularLoyalty + 0.6 * std::pow(Pick.Next(), 1.5));
			++Regulars;
			++AddedRegulars;
		}
		while (Channel.Followers < WantFollowers && static_cast<int>(Channel.Members.size()) < kast::MaxMembers)
		{
			Add(0.02 + 0.3 * Pick.Next() * Pick.Next());
		}
		const int Lurkers = std::max(0, WantFollowers - Channel.Followers);
		Channel.Followers += Lurkers;
		if (Channel.Followers >= kast::AffiliateFollowersC8 || WantSubs > 0)
		{
			Channel.Affiliate = true;
		}
		int AddedSubs = 0;
		if (WantSubs > Channel.ActiveSubs(World))
		{
			std::vector<const kast::Member*> ByLoyalty;
			for (const kast::Member& M : Channel.Members)
			{
				if (M.Follower && !Channel.IsSub(M.Name, World))
				{
					ByLoyalty.push_back(&M);
				}
			}
			std::stable_sort(ByLoyalty.begin(), ByLoyalty.end(), [](const kast::Member* A, const kast::Member* B) { return A->Loyalty > B->Loyalty; });
			for (size_t I = 0; I < ByLoyalty.size() && Channel.ActiveSubs(World) < WantSubs; ++I)
			{
				kast::Subscriber Sub;
				Sub.Name = ByLoyalty[I]->Name;
				Sub.Since = World - session_kast_detail::KastDebugDays(Pick, 1.0, 25.0);
				Sub.Renews = Sub.Since + 30.0 * session_kast_detail::KastDayMinutes;
				Channel.Subs.push_back(Sub);
				++AddedSubs;
			}
		}
		Save();
		return "ss.Kast.Grant: " + Grouped(Channel.Followers - Followers0) + " followers added (" + Grouped(static_cast<int64_t>(Channel.Members.size() - Members0)) + " members, " +
			Grouped(AddedRegulars) + " of them regulars; " + Grouped(Lurkers) + " past the member cap, as lurkers), " + Grouped(AddedSubs) + " subscribers. Now " +
			Grouped(Channel.Followers) + " followers, " + Grouped(Regulars) + " regulars, " + std::to_string(Channel.ActiveSubs(World)) + " active subs" +
			(Channel.Affiliate ? ", Affiliate." : ".");
	}
	if (Verb == "heat")
	{
		if (W.size() < 2)
		{
			return "ss.Kast.Heat <h>: Heat from 8 to 100, as of now.";
		}
		Channel.Heat = std::min(100.0, std::max(8.0, Arg(1, 26.0)));
		Channel.HeatAt = World;
		Save();
		const double Shown = kast::growth::HeatShown(Channel.Heat, Channel.HeatAt, World, Channel.LastStreamDay);
		return "ss.Kast.Heat: " + Fixed(Channel.Heat, 1) + " as of now (shown " + Fixed(Shown, 1) + ", " + kast::copy::TierName(kast::growth::Tier(Shown)) + ").";
	}
	if (Verb == "moment" || Verb == "raid" || Verb == "room" || Verb == "forecast")
	{
		const bool NeedsLive = Verb != "forecast";
		if (NeedsLive && !Stream.Live)
		{
			return "ss.Kast." + std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(Verb[0])))) + Verb.substr(1) + ": go live first.";
		}
		std::string Line;
		for (const std::string& Word : W)
		{
			Line += (Line.empty() ? "" : " ") + Word;
		}
		const std::string Room = Stream.DebugRoom(Channel, StreamInputs(), Line);
		PlayStreamNotices();
		if (Verb == "forecast")
		{
			const kast::growth::Band Fc = KastForecast();
			return (Room.empty() ? std::string() : Room + "\n") + "ss.Kast.Forecast: likely " + Fixed(Fc.Lo, 1) + " to " + Fixed(Fc.Hi, 1) + " (centre " + Fixed(Fc.Centre, 1) + ").";
		}
		return Room.empty() ? "ss.Kast." + Verb + ": the room doesn't take it in this build." : Room;
	}
	if (Verb == "advance")
	{
		// The skip-safety check by hand (§8.4): one Offline step of N days against N steps of one, on copies of the channel (the
		// channel itself and the clock stay where they are), compared line for line as the save writes them.
		const int Days = std::max(1, std::min(400, static_cast<int>(Arg(1, 1.0))));
		const double Base = std::max(Channel.LastOffline, World);
		kast::Channel Once = Channel;
		kast::Channel Daily = Channel;
		Rng OnceRng = KastRng;
		Rng DailyRng = KastRng;
		// Both start from the channel turned over to now, as the session's offline branch keeps it every game minute: a channel's
		// first Offline only marks its day (it steps no day before it), and that first call is not the comparison.
		kast::Offline(Once, Base, OnceRng, WorldSeed(), Life.RentsPaid);
		kast::Offline(Daily, Base, DailyRng, WorldSeed(), Life.RentsPaid);
		if (Once.ProcessedDay < 0)
		{
			Once.ProcessedDay = net::DayOf(Base);
			Daily.ProcessedDay = Once.ProcessedDay;
		}
		kast::Offline(Once, Base + static_cast<double>(Days) * session_kast_detail::KastDayMinutes, OnceRng, WorldSeed(), Life.RentsPaid);
		for (int D = 1; D <= Days; ++D)
		{
			kast::Offline(Daily, Base + static_cast<double>(D) * session_kast_detail::KastDayMinutes, DailyRng, WorldSeed(), Life.RentsPaid);
		}
		const double End = Base + static_cast<double>(Days) * session_kast_detail::KastDayMinutes;
		const std::string A = session_kast_detail::KastLinesOf(Once, Life, End);
		const std::string B = session_kast_detail::KastLinesOf(Daily, Life, End);
		const std::string Diff = session_kast_detail::KastFirstDiff(A, B);
		std::string Apart;
		if (!Diff.empty())
		{
			Apart = " (daily: " + Grouped(Daily.Followers) + " followers, " + std::to_string(Daily.ActiveSubs(End)) + " active subs, renewals " + Money(Daily.EarnedSubs - Channel.EarnedSubs) +
				" against one step's " + Money(Once.EarnedSubs - Channel.EarnedSubs) + ")";
		}
		return "ss.Kast.Advance " + std::to_string(Days) + " (on copies; the channel is unchanged): one step and " + std::to_string(Days) + " daily steps " +
			(Diff.empty() ? std::string("agree to the byte") : "differ: " + Diff + Apart) + ". After them: " + Grouped(Once.Followers) + " followers (now " +
			Grouped(Channel.Followers) + "), " + std::to_string(Once.ActiveSubs(End)) + " active subs (now " + std::to_string(Channel.ActiveSubs(World)) + "), balance " +
			Money(Once.UnpaidCents) + ", pending " + Money(Once.PendingCents) + ", Heat " + Fixed(kast::growth::HeatAt(Once.Heat, Once.HeatAt, End), 1) + ".";
	}
	if (Verb == "rent")
	{
		Life.RentsPaid = std::max(2, Life.RentsPaid);
		Save();
		const kast::growth::Payout Split = kast::growth::PayoutOf(Channel.UnpaidCents, Channel.PendingCents, Life.RentsPaid);
		return "ss.Kast.Rent: the second rent counts as paid (" + std::to_string(Life.RentsPaid) + " paid). The next payout pays " + Money(Split.Pay) + " (pending " +
			Money(Channel.PendingCents) + ").";
	}
	return "ss.Kast.<Describe | Grant <followers> [regulars] [subs] | Heat <h> | Moment <bubble|itm|ft|top3|win|allin> [prestige] [entrants] | Raid <size> [broadcast] | Room | "
		   "Advance <days> | Rent | Forecast>";
}
} // namespace ss
