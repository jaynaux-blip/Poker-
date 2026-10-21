// Update 3 (SPEC §2.4, §3.4-§3.6, §7.3): the revamped sites' presentation helpers, core-sites-look's file. Pure functions
// over the world's and the player's own numbers; no save, no draw from any game Rng, no wall clock.
//
// - IgniteMood: Ignite's hour (Afterburner Hours inside its SiteDef night window, then Wake-Up, Daytime, Prime Time).
// - ProjectedOverlay / OverlayOf / LateRegMultiplier: RunItTwice's Overlay Watch (§3.6 R1). Registered so far x the
//   template's median late-registration multiplier: the network's own registration curve (Network::Live) at its median
//   (the curve's per-instance wobble is uniform on 0.9-1.1 and the share rises with it, so the median of final / so-far over
//   the template's instances is the curve at 1.0). Never the world's planned final field: "PROJ. ±" can be wrong both ways,
//   by up to about a tenth before the start and less as late registration runs; once it closes, the actual overlay.
// - ClubAccrual: the Second Run Club's month so far, through the same tier table and rounding SiteCloseMonth pays with.
// - SecondRun: spike S-D's rule (durable/update3/S-D.md): the hand's own deck stub past DeckPos, a burn before each street
//   still to come, from the hand's first Reveal; the keyed fallback only for a deck too short to deal it (tests).
//   SecondRecordOf / NoteSecond: the Settlement's record of it, every fact from the finished hand (the hands, the real board,
//   what the first run paid, AllInEquity), never from the table's display state.
// - Desk / DeskNet: History's rows on the site. A final table is the event's last table (FinalTableSeats: the last 8 at
//   the rivals' 8-max, not 9), and the rake is what the account actually paid (RakePaid: a ticket's entry pays none).
// - SundayLive / Registrations: the network's registrations at past minutes, its pool and payouts, the world's planned
//   entrants and its result. Facts only (W5): no NPC chip counts, nothing after World.
// - Welcome: Ignite's welcome bonus as SiteMoney.cpp releases it ($5 for every $20 of rake inside the 60 days).
#include "ShortStack/Game/SiteShow.h"

#include "SitesDetail.h"
#include "ShortStack/Evaluator.h"
#include "ShortStack/Game/Autoplay.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Hand.h"
#include "ShortStack/Rng.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace site
{
namespace show
{
namespace siteshow_detail
{
// ------------------------------------------------------------------ the copy (Inventory: linted and screened by rival_sites)

const char* const SiteShowOpensGtd = "Registration opens for {Event}: {Buyin} buy-in, {Gtd} guaranteed.";
const char* const SiteShowOpens = "Registration opens for {Event}: {Buyin} buy-in.";
const char* const SiteShowCount = "{Count} registered for {Event}.";
const char* const SiteShowDeal = "Cards in the air: {Count} entries, late registration open until {Clock}.";
const char* const SiteShowLate = "Late registration: {Count} entries, {Pool} in the prize pool.";
const char* const SiteShowClosed = "Registration closed at {Clock}: {Count} entries, {Pool} in the prize pool, {Paid} places paid.";
const char* const SiteShowOverlay = "Overlay paid: {Amount} to make the {Gtd} guarantee.";
const char* const SiteShowFinalTable = "The final table is set: {Left} players left.";
const char* const SiteShowWinner = "{Who} wins {Event} for {Amount}.";
const char* const SiteShowPlace = "{Place}: {Who}, {Amount}.";
const char* const SiteShowTeam = "{Who} of {Team} is registered.";
const char* const SiteShowNotable = "{Who} is registered.";

/** A template's {Key}s filled (keys without a value stay as written). */
std::string SiteShowFill(const char* Template, const std::vector<std::pair<const char*, std::string>>& Vars)
{
	std::string Out = Template;
	for (const std::pair<const char*, std::string>& V : Vars)
	{
		const std::string Key = std::string("{") + V.first + "}";
		for (size_t At = Out.find(Key); At != std::string::npos; At = Out.find(Key, At + V.second.size()))
		{
			Out.replace(At, Key.size(), V.second);
		}
	}
	return Out;
}

/** Money as RunItTwice prints it: whole dollars grouped ("$100,000"), cents only when there are some ("$2,433.50"). */
std::string SiteShowDollars(Chips Cents)
{
	return Cents % 100 == 0 ? "$" + Grouped(Cents / 100) : Money(Cents);
}

/** A world minute on the 24-hour clock ("21:30"). */
std::string SiteShowClock(double World)
{
	const int Min = static_cast<int>(std::floor(std::fmod(std::fmod(World, net::MinutesPerDay) + net::MinutesPerDay, net::MinutesPerDay)));
	const int Hour = Min / 60;
	const int Rest = Min % 60;
	return (Hour < 10 ? "0" : "") + std::to_string(Hour) + (Rest < 10 ? ":0" : ":") + std::to_string(Rest);
}

// ------------------------------------------------------------------ registration and the overlay

/** Where an event's registration stands at World: 0 not open yet, 1 registering, 2 late registration, 3 closed. */
int SiteShowPhase(const net::EventTemplate& T, const net::EventInstance& E, double World)
{
	const double Opens = E.Start - static_cast<double>(T.OpensHours) * 60.0;
	if (World < Opens)
	{
		return 0;
	}
	if (World < E.Start)
	{
		return 1;
	}
	return World - E.Start < E.LateReg ? 2 : 3;
}

/**
 * The network's registration share at World with its wobble at the median (1.0): Network::Live's curve, the one place it is
 * restated (Network.cpp is frozen for Update 3; rival_sites checks the two agree on real instances).
 */
double SiteShowMedianShare(const net::EventTemplate& T, const net::EventInstance& E, double World)
{
	const int Phase = SiteShowPhase(T, E, World);
	if (Phase == 0)
	{
		return 0.0;
	}
	if (Phase == 3)
	{
		return 1.0;
	}
	if (Phase == 1)
	{
		const double Opens = E.Start - static_cast<double>(T.OpensHours) * 60.0;
		const double X = std::min(1.0, std::max(0.0, (World - Opens) / std::max(1.0, E.Start - Opens)));
		return 0.5 * (0.25 * X + 0.75 * std::pow(X, 6.0));
	}
	const double X = std::min(1.0, std::max(0.0, (World - E.Start) / std::max(1.0, E.LateReg)));
	return std::min(1.0, 0.5 + 0.5 * std::pow(X, 0.7));
}

/** What one paid entry puts in the pool, as Network::Make counts it (the site's prize share of the buy-in, to the cent). */
Chips SiteShowContribution(const net::Network& N, const net::EventTemplate& T)
{
	return static_cast<Chips>(std::llround(static_cast<double>(T.BuyInCents) * N.Definition().Rake.PrizeShare(T)));
}

// ------------------------------------------------------------------ the Second Board (spike S-D)

/** The board when the cards went on their backs (the hand's first Reveal): the all-in with cards to come. -1: no such moment. */
int SiteShowRevealBoard(const Hand& H)
{
	size_t SeenBoard = 0;
	for (const HandEvent& Ev : H.Events)
	{
		if (Ev.Type == EventType::Reveal)
		{
			return static_cast<int>(SeenBoard);
		}
		if (Ev.Type == EventType::Street)
		{
			SeenBoard = Ev.Board.size();
		}
	}
	return -1;
}

/** The player's share (0..1) of the chips in the pots they could win, the pots as the hand built them, on Cards. */
double SiteShowShare(const Hand& H, const std::vector<PotResult>& Pots, int HeroNumber, const std::vector<Card>& Cards)
{
	double Mine = 0.0;
	double Could = 0.0;
	for (const PotResult& P : Pots)
	{
		if (std::find(P.Eligible.begin(), P.Eligible.end(), HeroNumber) == P.Eligible.end())
		{
			continue;
		}
		int Best = -1;
		std::vector<int> Winners;
		for (const int Number : P.Eligible)
		{
			const HandSeat* Sat = H.SeatByNumber(Number);
			const int Score = Sat ? EvaluateHand(Sat->Hole, Cards) : -1;
			if (Score > Best)
			{
				Best = Score;
				Winners.clear();
			}
			if (Score == Best)
			{
				Winners.push_back(Number);
			}
		}
		Could += static_cast<double>(P.Amount);
		if (std::find(Winners.begin(), Winners.end(), HeroNumber) != Winners.end())
		{
			Mine += static_cast<double>(P.Amount) / static_cast<double>(Winners.size());
		}
	}
	return Could > 0.0 ? Mine / Could : 0.0;
}

/** The first run as the engine paid it: the player's share of the chips in the pots they could win. */
double SiteShowPaid(const Hand& H, int HeroNumber)
{
	double Mine = 0.0;
	double Could = 0.0;
	for (const PotResult& P : H.PotResults)
	{
		if (std::find(P.Eligible.begin(), P.Eligible.end(), HeroNumber) == P.Eligible.end())
		{
			continue;
		}
		Could += static_cast<double>(P.Amount);
		for (size_t K = 0; K < P.Winners.size() && K < P.Shares.size(); ++K)
		{
			Mine += P.Winners[K] == HeroNumber ? static_cast<double>(P.Shares[K]) : 0.0;
		}
	}
	return Could > 0.0 ? Mine / Could : 0.0;
}

/** A run's result from the player's share: 2 won it all, 0 none, 1 part. */
int SiteShowRunResult(double Share)
{
	return Share >= 1.0 - 1e-9 ? 2 : Share <= 1e-9 ? 0 : 1;
}

// ------------------------------------------------------------------ History

/** History's rows on Site that started by World and inside the last Days days (Days <= 0: all), oldest first. */
std::vector<const HistoryEntry*> SiteShowRows(const std::vector<HistoryEntry>& H, net::SiteId Site, int Days, double World)
{
	std::vector<const HistoryEntry*> Out;
	const double From = Days > 0 ? World - static_cast<double>(Days) * net::MinutesPerDay : -1e300;
	for (const HistoryEntry& Row : H)
	{
		const double Start = net::EntryStart(Row);
		if (net::SiteOf(Row.EventId) == Site && Start <= World && Start >= From)
		{
			Out.push_back(&Row);
		}
	}
	std::stable_sort(Out.begin(), Out.end(), [](const HistoryEntry* A, const HistoryEntry* B) { return net::EntryStart(*A) < net::EntryStart(*B); });
	return Out;
}

/** A placing's name as the boards show it. */
std::string SiteShowPlacingName(const net::Network& N, const net::Placing& P)
{
	if (P.Player >= 0 && P.Player < static_cast<int>(N.Players().size()))
	{
		return N.Players()[static_cast<size_t>(P.Player)].Name;
	}
	return P.Player == -1 ? N.HeroName() : P.Name;
}
} // namespace siteshow_detail

// ------------------------------------------------------------------ Ignite's hour

Mood IgniteMood(double World)
{
	const net::SiteDef& D = net::Def(net::SiteId::Ignite);
	double Minute = std::fmod(World, net::MinutesPerDay);
	Minute += Minute < 0.0 ? net::MinutesPerDay : 0.0;
	const double From = static_cast<double>(D.NightFromMin);
	const double To = static_cast<double>(D.NightToMin);
	const bool Night = From <= To ? Minute >= From && Minute < To : Minute >= From || Minute < To;
	if (Night)
	{
		return Mood::Afterburner;
	}
	return Minute < 720.0 ? Mood::WakeUp : Minute < 1080.0 ? Mood::Daytime : Mood::PrimeTime;
}

// ------------------------------------------------------------------ Overlay Watch

double LateRegMultiplier(const net::Network& N, const net::EventInstance& E, double World)
{
	using namespace siteshow_detail;
	const double Share = SiteShowMedianShare(N.TemplateOf(E), E, World);
	return Share > 0.0 ? 1.0 / Share : 0.0;
}

Chips ProjectedOverlay(const net::Network& N, const net::EventInstance& E, int RegisteredSoFar, double World, bool& Actual)
{
	using namespace siteshow_detail;
	const net::EventTemplate& T = N.TemplateOf(E);
	const int Phase = SiteShowPhase(T, E, World);
	Actual = Phase == 3;
	if (T.GtdCents <= 0 || T.BuyInCents <= 0)
	{
		return 0;
	}
	const Chips Each = SiteShowContribution(N, T);
	if (Actual)
	{
		return std::max<Chips>(0, T.GtdCents - Each * static_cast<Chips>(E.Entries));
	}
	if (Phase == 0 || RegisteredSoFar <= 0)
	{
		return 0; // nothing registered yet: nothing to project from
	}
	const double Projected = static_cast<double>(RegisteredSoFar) * LateRegMultiplier(N, E, World);
	return std::max<Chips>(0, T.GtdCents - static_cast<Chips>(std::llround(Projected * static_cast<double>(Each))));
}

OverlayView OverlayOf(const net::Network& N, const net::EventInstance& E, double World)
{
	using namespace siteshow_detail;
	OverlayView V;
	const net::EventTemplate& T = N.TemplateOf(E);
	const int Phase = SiteShowPhase(T, E, World);
	V.Registered = Phase == 0 ? 0 : Phase == 3 ? E.Entries : N.Live(E, World).Entries;
	V.Cents = ProjectedOverlay(N, E, V.Registered, World, V.Actual);
	V.Entries = V.Actual ? static_cast<double>(E.Entries) : static_cast<double>(V.Registered) * LateRegMultiplier(N, E, World);
	V.Shown = T.GtdCents > 0 && T.BuyInCents > 0 && (V.Actual || (Phase != 0 && V.Registered > 0));
	V.Amber = V.Shown && V.Cents >= 10000 && V.Cents * 20 >= T.GtdCents;
	return V;
}

// ------------------------------------------------------------------ the Club

Accrual ClubAccrual(const site::Account& A)
{
	const std::vector<TierInfo>& Tiers = SecondRunTiers();
	Accrual Out;
	Out.Rake = std::max<Chips>(0, A.RakeMonth);
	Out.Tier = TierFor(Tiers, Out.Rake);
	const TierInfo& At = Tiers[static_cast<size_t>(Out.Tier)];
	Out.Accrued = static_cast<Chips>(std::llround(static_cast<double>(Out.Rake) * static_cast<double>(At.Permille) / 1000.0));
	const bool Top = Out.Tier + 1 >= static_cast<int>(Tiers.size());
	Out.Next = Top ? Out.Tier : Out.Tier + 1;
	Out.ToNext = Top ? 0 : Tiers[static_cast<size_t>(Out.Next)].From - Out.Rake;
	return Out;
}

// ------------------------------------------------------------------ the Second Board

bool SecondRun(const Hand& H, int HeroSeat, const std::string& HandId, SecondBoard& Out)
{
	using namespace siteshow_detail;
	Out = SecondBoard();
	const int SharedCards = SiteShowRevealBoard(H);
	if (!H.bComplete || SharedCards < 0 || SharedCards >= 5 || H.Board.size() != 5)
	{
		return false; // no all-in with cards to come (a fold ended it, it went to the river, or it isn't over)
	}
	// The second run as a dealer deals it from the stub: past DeckPos, a burn before each street still to come.
	std::vector<Card> Run(H.Board.begin(), H.Board.begin() + SharedCards);
	const std::vector<Card>& Deck = H.DeckForShow();
	size_t At = static_cast<size_t>(std::max(0, H.DeckPosForShow()));
	bool Stub = true;
	auto DealStreet = [&](int Count) {
		if (At + 1 + static_cast<size_t>(Count) > Deck.size())
		{
			Stub = false;
			return;
		}
		++At; // the burn
		for (int K = 0; K < Count; ++K)
		{
			Run.push_back(Deck[At++]);
		}
	};
	if (SharedCards == 0)
	{
		DealStreet(3);
	}
	if (SharedCards <= 3)
	{
		DealStreet(1);
	}
	DealStreet(1);
	if (!Stub)
	{
		// The fallback (a short test deck only): a keyed draw from the cards the hand never dealt (no board card, no hole card,
		// a folded one included, as a real second run would deal none of them), never the hand's Rng.
		Run.assign(H.Board.begin(), H.Board.begin() + SharedCards);
		std::vector<Card> Unseen;
		for (Card Pick = 0; Pick < 52; ++Pick)
		{
			bool Seen = std::find(H.Board.begin(), H.Board.end(), Pick) != H.Board.end();
			for (const HandSeat& Sat : H.Seats)
			{
				Seen = Seen || std::find(Sat.Hole.begin(), Sat.Hole.end(), Pick) != Sat.Hole.end();
			}
			if (!Seen)
			{
				Unseen.push_back(Pick);
			}
		}
		Rng Keyed("rit/second/" + HandId);
		Keyed.Shuffle(Unseen);
		for (size_t K = 0; Run.size() < 5 && K < Unseen.size(); ++K)
		{
			Run.push_back(Unseen[K]);
		}
	}
	for (size_t K = 0; K < 5 && K < Run.size(); ++K)
	{
		Out.Cards[K] = Run[K];
	}
	Out.FromStub = Stub;
	Out.Shared = SharedCards;
	const HandSeat* Me = HeroSeat >= 0 && HeroSeat < static_cast<int>(H.Seats.size()) ? &H.Seats[static_cast<size_t>(HeroSeat)] : nullptr;
	bool InPot = false;
	for (const PotResult& P : H.PotResults)
	{
		InPot = InPot || (Me && std::find(P.Eligible.begin(), P.Eligible.end(), Me->Seat) != P.Eligible.end());
	}
	Out.HeroIn = Me && !Me->Folded && InPot;
	if (Out.HeroIn)
	{
		Out.First = SiteShowRunResult(SiteShowPaid(H, Me->Seat));
		Out.Second = SiteShowRunResult(SiteShowShare(H, H.BuildPots(), Me->Seat, Run));
		Out.Verdict = VerdictOf(Out.First, Out.Second);
	}
	return true;
}

int VerdictOf(int First, int Second)
{
	return First == 2 && Second == 2 ? 2 : First == 0 && Second == 0 ? 0 : 1;
}

bool SecondRecordOf(const Hand& H, int HeroSeat, const std::string& HandId, SecondRecord& Out)
{
	// Fix secondboard (the lead's PIE, 2026-10-21): the Settlement's record is the hand's, whole. The table used to fill it
	// from its display state at the showdown, before the award: the seats' Winner was still false (RAN "LOST" on a pot the
	// player won, under "you'd have WON BOTH") and their equities had run to the river's 100 / 0.
	Out = SecondRecord();
	Out.HandId = HandId;
	if (!SecondRun(H, HeroSeat, HandId, Out.Board) || !Out.Board.HeroIn)
	{
		Out.Board = SecondBoard();
		return false;
	}
	const HandSeat& Me = H.Seats[static_cast<size_t>(HeroSeat)];
	for (size_t K = 0; K < 2 && K < Me.Hole.size(); ++K)
	{
		Out.Hero[K] = Me.Hole[K];
	}
	for (size_t I = 0; I < H.Seats.size(); ++I)
	{
		const HandSeat& Sat = H.Seats[I];
		if (static_cast<int>(I) != HeroSeat && !Sat.Folded && Sat.Hole.size() == 2 && Out.Villain[0] < 0)
		{
			Out.Villain = {Sat.Hole[0], Sat.Hole[1]};
		}
	}
	for (size_t K = 0; K < 5 && K < H.Board.size(); ++K)
	{
		Out.Ran[K] = H.Board[K];
	}
	Out.Equity = autoplay::AllInEquity(H, HeroSeat);
	return true;
}

bool NoteSecond(std::vector<SecondRecord>& Log, const Hand& H, const std::string& EventId, const std::string& HandId, double At, size_t Keep)
{
	int Me = -1;
	for (size_t I = 0; I < H.Seats.size() && Me < 0; ++I)
	{
		Me = H.Seats[I].Id == HeroId ? static_cast<int>(I) : -1;
	}
	SecondRecord Rec;
	if (Me < 0 || !SecondRecordOf(H, Me, HandId, Rec))
	{
		return false;
	}
	Rec.EventId = EventId;
	Rec.At = At;
	Log.push_back(Rec);
	if (Log.size() > std::max<size_t>(1, Keep))
	{
		Log.erase(Log.begin(), Log.begin() + static_cast<std::ptrdiff_t>(Log.size() - std::max<size_t>(1, Keep)));
	}
	return true;
}

// ------------------------------------------------------------------ the Desk

DeskKpis Desk(const std::vector<HistoryEntry>& H, const site::Account& A, int Days, double World)
{
	using namespace siteshow_detail;
	DeskKpis K;
	Chips Spent = 0;
	Chips Won = 0;
	int Paid = 0;
	int Cashes = 0;
	int Tables = 0;
	for (const HistoryEntry* Row : SiteShowRows(H, A.Site, Days, World))
	{
		const Chips Cost = std::max<Chips>(0, Row->BuyInCents);
		++K.Events;
		Spent += Cost;
		Won += std::max<Chips>(0, Row->Prize);
		Paid += Cost > 0 ? 1 : 0;
		Cashes += Row->Prize > 0 ? 1 : 0;
		Tables += MadeFinalTable(*Row) ? 1 : 0;
	}
	// The rake actually paid (G1_AUDIT L5): the account's own tally, never a ticket's face value; History can't tell a
	// ticket's entry from a paid one, so it is the account's whatever the range (the Ledger's RAKE PAID is the same figure).
	K.Rake = RakePaid(A);
	K.Net = Won - Spent;
	K.Roi = Spent > 0 ? static_cast<double>(K.Net) / static_cast<double>(Spent) : 0.0;
	K.Itm = K.Events > 0 ? static_cast<double>(Cashes) / static_cast<double>(K.Events) : 0.0;
	K.Ft = K.Events > 0 ? static_cast<double>(Tables) / static_cast<double>(K.Events) : 0.0;
	K.Abi = Paid > 0 ? static_cast<Chips>(std::llround(static_cast<double>(Spent) / static_cast<double>(Paid))) : 0;
	K.Club = A.Site == net::SiteId::RunItTwice ? ClubAccrual(A).Accrued : 0;
	return K;
}

std::vector<NetPoint> DeskNet(const std::vector<HistoryEntry>& H, net::SiteId Site, int Days, double World)
{
	using namespace siteshow_detail;
	std::vector<NetPoint> Out;
	Chips Net = 0;
	for (const HistoryEntry* Row : SiteShowRows(H, Site, Days, World))
	{
		Net += std::max<Chips>(0, Row->Prize) - std::max<Chips>(0, Row->BuyInCents);
		Out.push_back({net::EntryStart(*Row), Net});
	}
	return Out;
}

// ------------------------------------------------------------------ Sunday Live

std::vector<RegPoint> Registrations(const net::Network& N, const net::EventInstance& E, double World, int Points)
{
	using namespace siteshow_detail;
	std::vector<RegPoint> Out;
	const net::EventTemplate& T = N.TemplateOf(E);
	const double Opens = E.Start - static_cast<double>(T.OpensHours) * 60.0;
	const double Last = std::min(World, E.Start + E.LateReg);
	if (Last < Opens)
	{
		return Out;
	}
	const int Count = std::max(2, std::min(240, Points));
	for (int I = 0; I < Count; ++I)
	{
		const double At = Opens + (Last - Opens) * static_cast<double>(I) / static_cast<double>(Count - 1);
		Out.push_back({At, SiteShowPhase(T, E, At) == 3 ? E.Entries : N.Live(E, At).Entries});
	}
	return Out;
}

namespace siteshow_detail
{
/** Sunday Live's rows as they happen, appended in no particular order (SundayLive sorts them by minute). */
void SiteShowLive(const net::Network& N, const world::World* W, const net::EventInstance& E, double World, std::vector<LiveRow>& Out)
{
	const net::EventTemplate& T = N.TemplateOf(E);
	const double Opens = E.Start - static_cast<double>(T.OpensHours) * 60.0;
	if (World < Opens)
	{
		return;
	}
	auto Fact = [&Out](double At, std::string Text, int Kind, int Npc) {
		LiveRow Row;
		Row.At = At;
		Row.Text = std::move(Text);
		Row.Kind = Kind;
		Row.Npc = Npc;
		Out.push_back(std::move(Row));
	};
	const std::string Name = T.Name;
	Fact(Opens,
		T.GtdCents > 0 ? SiteShowFill(SiteShowOpensGtd, {{"Event", Name}, {"Buyin", net::BuyIn(T.BuyInCents)}, {"Gtd", SiteShowDollars(T.GtdCents)}})
					   : SiteShowFill(SiteShowOpens, {{"Event", Name}, {"Buyin", net::BuyIn(T.BuyInCents)}}),
		LiveFact, -1);
	// The field as it built: the network's count at a few fixed minutes before the start (none while nobody is in).
	for (const double Before : {360.0, 180.0, 120.0, 60.0, 30.0, 10.0})
	{
		const double At = E.Start - Before;
		if (At <= Opens || At > World)
		{
			continue;
		}
		const int Count = N.Live(E, At).Entries;
		if (Count > 0)
		{
			Fact(At, SiteShowFill(SiteShowCount, {{"Count", Grouped(Count)}, {"Event", Name}}), LiveFact, -1);
		}
	}
	// Who's in that the site's chrome would name: its own team's pros, then the most-followed players (the world's plan).
	if (W && World < E.Start + E.Duration)
	{
		const std::string Team = N.Definition().TeamName;
		const double Listed = std::max(Opens, std::floor(E.Start / net::MinutesPerDay) * net::MinutesPerDay);
		std::vector<std::pair<int, int>> Followed; // (followers, id)
		int Pros = 0;
		for (const int Id : W->RegisteredNow(E.Id))
		{
			const world::Npc* P = W->Get(Id);
			if (!P)
			{
				continue;
			}
			if (!Team.empty() && P->Sponsor == Team && Pros < 6)
			{
				++Pros;
				Fact(std::min(Listed, World), SiteShowFill(SiteShowTeam, {{"Who", P->Name}, {"Team", Team}}), LiveTeam, Id);
			}
			else if (P->Followers >= 2500)
			{
				Followed.push_back({P->Followers, Id});
			}
		}
		std::sort(Followed.begin(), Followed.end(), [](const std::pair<int, int>& A, const std::pair<int, int>& B) { return A.first != B.first ? A.first > B.first : A.second < B.second; });
		for (size_t I = 0; I < Followed.size() && I < 4; ++I)
		{
			Fact(std::min(Listed, World), SiteShowFill(SiteShowNotable, {{"Who", W->Get(Followed[I].second)->Name}}), LiveNotable, Followed[I].second);
		}
	}
	if (World < E.Start)
	{
		return;
	}
	const double Closes = E.Start + E.LateReg;
	Fact(E.Start, SiteShowFill(SiteShowDeal, {{"Count", Grouped(N.Live(E, E.Start).Entries)}, {"Clock", SiteShowClock(Closes)}}), LiveFact, -1);
	for (double At = E.Start + 30.0; At < Closes && At <= World; At += 30.0)
	{
		const net::LiveState L = N.Live(E, At);
		Fact(At, SiteShowFill(SiteShowLate, {{"Count", Grouped(L.Entries)}, {"Pool", SiteShowDollars(L.Pool)}}), LiveFact, -1);
	}
	if (World < Closes)
	{
		return;
	}
	const std::vector<Chips> Pay = N.Payouts(E);
	Fact(Closes,
		SiteShowFill(SiteShowClosed,
			{{"Clock", SiteShowClock(Closes)}, {"Count", Grouped(E.Entries)}, {"Pool", SiteShowDollars(E.Pool)}, {"Paid", Grouped(static_cast<int64_t>(Pay.size()))}}),
		LiveFact, -1);
	bool Actual = false;
	const Chips Overlay = ProjectedOverlay(N, E, E.Entries, World, Actual);
	if (Actual && Overlay > 0)
	{
		Fact(Closes, SiteShowFill(SiteShowOverlay, {{"Amount", SiteShowDollars(Overlay)}, {"Gtd", SiteShowDollars(T.GtdCents)}}), LiveFact, -1);
	}
	// The final table: the first minute the network calls it one (the field only shrinks once late registration is over).
	const double Ends = E.Start + E.Duration;
	if (N.Live(E, std::min(World, Ends - 0.01)).St == net::Status::FinalTable)
	{
		double Lo = Closes;
		double Hi = std::min(World, Ends - 0.01);
		for (int Step = 0; Step < 24 && Hi - Lo > 0.5; ++Step)
		{
			const double Mid = 0.5 * (Lo + Hi);
			(N.Live(E, Mid).St == net::Status::FinalTable ? Hi : Lo) = Mid;
		}
		const double At = std::min(std::ceil(Hi), World);
		Fact(At, SiteShowFill(SiteShowFinalTable, {{"Left", Grouped(N.Live(E, At).Left)}}), LiveFact, -1);
	}
	if (World < Ends)
	{
		return;
	}
	const net::EventResult& R = N.Result(E);
	for (const net::Placing& P : R.FinalTable)
	{
		if (P.Place == 1)
		{
			Fact(Ends, SiteShowFill(SiteShowWinner, {{"Who", SiteShowPlacingName(N, P)}, {"Event", Name}, {"Amount", SiteShowDollars(P.Prize)}}), LiveFact, P.Player);
		}
	}
	for (const net::Placing& P : R.FinalTable)
	{
		if (P.Place >= 1 && P.Place <= 3)
		{
			Fact(Ends, SiteShowFill(SiteShowPlace, {{"Place", Ordinal(P.Place)}, {"Who", SiteShowPlacingName(N, P)}, {"Amount", SiteShowDollars(P.Prize)}}), LivePodium,
				P.Player);
		}
	}
}
} // namespace siteshow_detail

std::vector<LiveRow> SundayLive(const net::Network& N, const world::World* W, const net::EventInstance& E, double World)
{
	std::vector<LiveRow> Out;
	siteshow_detail::SiteShowLive(N, W, E, World, Out);
	std::stable_sort(Out.begin(), Out.end(), [](const LiveRow& A, const LiveRow& B) { return A.At < B.At; });
	return Out;
}

// ------------------------------------------------------------------ Ignite's welcome bonus

WelcomeView Welcome(const site::Account& A, double World)
{
	// SiteMoney.cpp's rule, restated for the meter: $5 released for every $20 of the account's rake inside the 60 days.
	constexpr Chips SiteShowPer = 2000;
	WelcomeView V;
	V.Pending = std::max<Chips>(0, A.BonusPending);
	V.Released = std::max<Chips>(0, A.BonusReleased);
	V.Ends = A.BonusExpires;
	const Chips Total = V.Pending + V.Released;
	V.NeedleT = Total > 0 ? static_cast<double>(V.Released) / static_cast<double>(Total) : 0.0;
	if (V.Pending > 0 && A.BonusExpires > 0.0 && World <= A.BonusExpires)
	{
		const Chips Rake = std::max<Chips>(0, A.RakeLife);
		V.NextIn = (Rake / SiteShowPer + 1) * SiteShowPer - Rake;
	}
	return V;
}

// ------------------------------------------------------------------ one final table, one rake (G1's fix pass, G1_AUDIT L5)

int FinalTableSeats(const std::string& EventId)
{
	const size_t At = EventId.find('@');
	const std::string Template = At == std::string::npos ? EventId : EventId.substr(0, At);
	const net::EventTemplate* T = Template.empty() ? nullptr : net::ForEvent(EventId).FindTemplate(Template);
	return T && T->TableSize >= 2 ? T->TableSize : 9;
}

bool MadeFinalTable(const HistoryEntry& Row)
{
	return Row.Place >= 1 && Row.Place <= FinalTableSeats(Row.EventId);
}

Chips RakePaid(const site::Account& A)
{
	return std::max<Chips>(0, A.RakeLife);
}

// ------------------------------------------------------------------ the copy

std::vector<std::string> Inventory()
{
	using namespace siteshow_detail;
	return {SiteShowOpensGtd, SiteShowOpens, SiteShowCount, SiteShowDeal, SiteShowLate, SiteShowClosed, SiteShowOverlay, SiteShowFinalTable, SiteShowWinner,
		SiteShowPlace, SiteShowTeam, SiteShowNotable};
}
} // namespace show
} // namespace site
} // namespace ss
