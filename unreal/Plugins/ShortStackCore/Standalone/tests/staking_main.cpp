// staking_backing (next_segment_spec §7.6, suite 11): core-stake's tests.
//   1 event-stake vectors (Tests/staking_vectors.txt) and Lou's stake through register and settle;
//   2 pieces: rounding, deterministic fill plans, the one line at registration, refund with markup, owed created once;
//     2b Mrs. Park's gate with the online rank unknown (update 2 bug 6: she reads the sample);
//   3 swaps both ways, NPC payment timing;
//   4 a scripted 40-event package: makeup carried, top-ups, the chop, the 90-day cut-loose, Sam's garnish;
//   5 conservation to the cent; 6 idempotence with a save round trip between repeats;
//   7 due dates: the rent reserve, late and welched at exact minutes, Lou's 14 days;
//   8 negotiation answers across seeds, the same after a reload; 9 ROI shrinkage and ceilings against §2.6;
//   10 backer EV: a fixed-seed Monte Carlo over 5,000 periods per tier (and Lou's stake's EV to the player);
//   11 the save round trip and migration; 12 coaching tallies, the review's choice, the TiltGain floor;
//   13 exploits; plus the funding policy's order, the rail, the marks and the copy lint over every line.
// Goldens and fixtures are read from SS_TESTS_DIR (unreal/Plugins/ShortStackCore/Tests, set by CMakeLists.txt).
#include "CopyLint.h"
#include "ShortStack/AI/Grading.h"
#include "ShortStack/Game/Coach.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Rep.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Stake.h"
#include "ShortStack/Game/TableTalk.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Rng.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace stake_test
{
namespace life = ss::life;
namespace live = ss::live;
namespace stake = ss::stake;
namespace coach = ss::coach;
using ss::Chips;

int Failures = 0;
const double Day = 1440.0;
const double Hour = 60.0;

void Expect(bool Condition, const char* What)
{
	if (!Condition)
	{
		++Failures;
		std::printf("FAILED: %s\n", What);
	}
}

void Expect(bool Condition, const std::string& What)
{
	Expect(Condition, What.c_str());
}

std::string ReadFile(const std::string& Path)
{
	std::string Out;
	FILE* F = std::fopen(Path.c_str(), "rb");
	if (!F)
	{
		return Out;
	}
	char Buf[65536];
	size_t N = 0;
	while ((N = std::fread(Buf, 1, sizeof(Buf), F)) > 0)
	{
		Out.append(Buf, N);
	}
	std::fclose(F);
	Out.erase(std::remove(Out.begin(), Out.end(), '\r'), Out.end());
	return Out;
}

std::vector<std::string> Split(const std::string& Line, char Sep)
{
	std::vector<std::string> Out;
	std::string Cur;
	for (const char Ch : Line)
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

std::vector<std::vector<std::string>> Vectors(const char* Kind)
{
	std::vector<std::vector<std::string>> Out;
	for (const std::string& Line : Split(ReadFile(std::string(SS_TESTS_DIR) + "/staking_vectors.txt"), '\n'))
	{
		if (Line.empty() || Line[0] == '#')
		{
			continue;
		}
		std::vector<std::string> P = Split(Line, '\t');
		if (P[0] == Kind)
		{
			Out.push_back(P);
		}
	}
	return Out;
}

Chips C(const std::string& S)
{
	return static_cast<Chips>(std::strtoll(S.c_str(), nullptr, 10));
}

/** The sum of the ledger lines written since it held Before (the newest first; the ledger keeps 60). */
Chips LedgerSince(const life::State& L, size_t Before, size_t Now)
{
	Chips Sum = 0;
	for (size_t I = 0; I + Before < Now && I < L.Ledger.size(); ++I)
	{
		Sum += L.Ledger[I].Amount;
	}
	return Sum;
}

const life::LedgerEntry* FindLine(const life::State& L, const std::string& Part)
{
	for (const life::LedgerEntry& X : L.Ledger)
	{
		if (X.Label.find(Part) != std::string::npos)
		{
			return &X;
		}
	}
	return nullptr;
}

stake::Offer LouOffer(const live::Occurrence& O, double At)
{
	stake::Offer Of;
	Of.Id = "lou:" + O.Id;
	Of.K = stake::Kind::Event;
	Of.Backer = "Big Lou";
	Of.EntryId = O.Id;
	Of.Expires = O.LateRegEnds;
	Of.At = At;
	Of.BuyIn = O.T->BuyInCents;
	return Of;
}

stake::Deal PackageDeal(int Tier, double At)
{
	stake::Deal D;
	D.Id = "deal:lanternrow:test";
	D.K = stake::Kind::Package;
	D.By = stake::Outfit::LanternRow;
	D.Backer = "Lantern Row Staking";
	D.T = stake::LanternRowTerms(Tier);
	D.Tier = Tier;
	D.Since = At;
	D.NextChop = At + 30.0 * Day;
	D.RollNow = D.T.Roll;
	D.Funded = D.T.Roll;
	return D;
}

life::Funding RollFunding(const stake::Deal& D, Chips BuyIn)
{
	life::Funding F;
	F.From = life::Paid::Package;
	F.Ref = D.Id;
	F.Who = "Lantern Row Staking";
	F.Cents = BuyIn;
	return F;
}

/** An online event as the session plays it: the buy-in (the roll's, or the player's), the prize to the bankroll, then the stake's settle. */
void OnlineEvent(stake::Book& B, Chips& Bank, life::State& L, const std::string& Id, Chips BuyIn, Chips Prize, double At)
{
	const life::Funding F = stake::FundFor(B, Id, BuyIn, false, At);
	const Chips Before = Bank;
	if (F.From != life::Paid::Package)
	{
		Bank -= BuyIn;
		L.Record(At, "Buy-in: " + Id, -BuyIn, 0);
	}
	stake::OnlineEntered(B, Bank, L, Id, Id, BuyIn, F, Before, 0.0, At);
	if (Prize > 0)
	{
		Bank += Prize;
		L.Record(At + 60.0, "Prize: " + Id, Prize, 0);
	}
	stake::OnlineSettle(B, Bank, L, Id, Prize, At + 60.0);
}

std::string Save(const life::State& L, Chips Bank)
{
	ss::SaveData D;
	D.Life = L;
	D.BankrollCents = Bank;
	return D.Serialize();
}

bool Load(const std::string& Text, life::State& L, Chips& Bank)
{
	ss::SaveData D;
	if (!ss::SaveData::Parse(Text, D))
	{
		return false;
	}
	L = D.Life;
	Bank = D.BankrollCents;
	return true;
}

std::string StakeLines(const std::string& Save)
{
	std::string Out;
	for (const std::string& Line : Split(Save, '\n'))
	{
		if (Line.rfind("stake\t", 0) == 0 || Line.rfind("coach\t", 0) == 0)
		{
			Out += Line + "\n";
		}
	}
	return Out;
}

ss::rep::Public Record(int OnlineEvents, double OnlineRoi, Chips Abi, int LiveEvents, double LiveRoi, double Renown)
{
	ss::rep::Public P;
	P.OnlineEvents = OnlineEvents;
	P.OnlineRoi = OnlineRoi;
	P.Abi = Abi;
	P.LiveEvents = LiveEvents;
	P.LiveSample = LiveEvents;
	P.LiveRoi = LiveRoi;
	P.LiveCashes = LiveEvents / 6;
	P.Renown = Renown;
	return P;
}

stake::View ViewFor(const ss::rep::Public& P, double World)
{
	stake::View V;
	V.Rec = &P;
	V.World = World;
	V.Day = ss::net::DayOf(World);
	V.Bankroll = 100000;
	return V;
}

stake::Regard Knows(float Trust = 0.0f, float Respect = 0.0f)
{
	stake::Regard R;
	R.Known = true;
	R.Trust = Trust;
	R.Respect = Respect;
	R.Familiarity = 0.3f;
	return R;
}

// ------------------------------------------------------------------ 1. event stakes

void EventStakes()
{
	for (const std::vector<std::string>& V : Vectors("event"))
	{
		const live::Occurrence O = live::FindOccurrence(V[1]);
		const Chips Prize = C(V[2]);
		const Chips Lou = C(V[3]);
		const Chips Mine = C(V[4]);
		life::State L;
		Chips Bank = 100000;
		const double At = O.DeskOpens + 10.0;
		Expect(stake::Accept(L.Stakes, Bank, L, LouOffer(O, At), At).empty(), "event: Lou's stake taken");
		const std::vector<std::pair<life::Funding, std::string>> Opts = stake::FundingOptions(L, O, At);
		Expect(Opts.size() == 2 && Opts[0].first.From == life::Paid::Backer && Opts[0].first.Who == "Big Lou" && Opts[0].first.Permille == 1000 && Opts[0].second.empty() &&
				   Opts[1].first.From == life::Paid::Own && life::IsOwnMoney(Opts[1].first),
			"event: the options are Lou, then the player's own money");
		Expect(live::Register(Bank, L, O, At, 88, {}, Opts[0].first).empty(), "event: registered on Lou's money");
		// The three lines (§2.1 scene 1): Lou's stake, then the buy-in and the fee, the bankroll untouched.
		Expect(Bank == 100000 && L.Ledger.size() == 3 && L.Ledger[2].Label == "Big Lou: stake, Embercrest Nightly" && L.Ledger[2].Amount == 12000 &&
				   L.Ledger[2].Kind == life::LedgerBacking && L.Ledger[1].Amount == -10500 && L.Ledger[0].Amount == -1500,
			"event: Big Lou: stake +$120, the buy-in -$105, the fee -$15");
		const size_t Before = L.Ledger.size();
		live::Settle(Bank, L, O.Id, Prize > 0 ? 13 : 40, 88, Prize, O.Start + 300.0);
		Expect(Bank == 100000 + Mine, "event vector " + V[2] + ": the player keeps " + V[4]);
		const life::LedgerEntry* Share = FindLine(L, "Big Lou: his share");
		Expect(Lou == 0 ? Share == nullptr : (Share && Share->Amount == -Lou && Share->Kind == life::LedgerBacking), "event vector " + V[2] + ": Lou's share " + V[3]);
		Expect(LedgerSince(L, Before, L.Ledger.size()) == Mine, "event: the ledger sums to the bankroll's change");
		Expect(L.Stakes.NetOnYou["Big Lou"] == Lou - 12000, "event: Lou's running net");
	}
}

// ------------------------------------------------------------------ 2. pieces

stake::Listing Manual(const live::Occurrence& O, int Markup, std::vector<std::pair<std::string, int>> Who, double At)
{
	stake::Listing Ls;
	Ls.EventId = O.Id;
	Ls.Markup = Markup;
	Ls.Opens = At;
	Ls.Closes = O.LateRegEnds;
	Ls.BuyIn = O.T->BuyInCents;
	Ls.Live = true;
	double T = At + 30.0;
	for (const std::pair<std::string, int>& W : Who)
	{
		Ls.Plan.push_back(std::make_tuple(T, W.first, W.second));
		Ls.Ask += W.second;
		T += 30.0;
	}
	return Ls;
}

void Pieces()
{
	// The vectors: price per 1% = round(buy-in x markup), the money in one line at registration, the shares rounding down.
	for (const std::vector<std::string>& V : Vectors("pieces"))
	{
		const live::Occurrence O = live::FindOccurrence(V[1]);
		life::State L;
		Chips Bank = 100000;
		const double At = O.Start - 2.0 * Day;
		L.Stakes.Listings.push_back(Manual(O, std::atoi(V[2].c_str()), {{"", std::atoi(V[3].c_str())}}, At));
		const double Desk = O.DeskOpens + 5.0;
		Expect(live::Register(Bank, L, O, Desk, 104, {}, stake::FundFor(L.Stakes, O.Id, O.T->BuyInCents, true, Desk)).empty(), "pieces: registered");
		Expect(Bank == 100000 - O.T->BuyInCents + C(V[4]), "pieces vector: collected " + V[4]);
		const Chips Was = Bank;
		live::Settle(Bank, L, O.Id, 4, 104, C(V[5]), O.Start + 400.0);
		Chips Owed = 0;
		for (const stake::Owed& W : L.Stakes.Owes)
		{
			Owed += W.ToYou ? 0 : W.Cents;
		}
		Expect(Owed == C(V[6]) && Bank == Was + C(V[5]), "pieces vector: the buyers' shares " + V[6]);
	}

	// Scene 2 (§2.1): 35% of the Sunday at 1.05 to Mrs. Park, Dre, Big Lou and someone off the board.
	const live::Occurrence O = live::FindOccurrence("embercrest@6");
	life::State L;
	Chips Bank = 15000 - 5530 + 2 * live::BusFareCents;
	const double At = O.Start - 2.0 * Day;
	L.Stakes.Listings.push_back(Manual(O, 1050, {{"Mrs. Park", 100}, {"Dre", 50}, {"Big Lou", 150}, {"", 50}}, At));
	const double Desk = O.DeskOpens + 5.0;
	const std::vector<std::pair<life::Funding, std::string>> Opts = stake::FundingOptions(L, O, Desk);
	Expect(Opts.size() == 1 && Opts[0].first.From == life::Paid::Own && Opts[0].first.Permille == 350 && Opts[0].first.Cents == 5530 && Opts[0].first.Who == "4 backers",
		"pieces: the player's own money, 35% sold, $55.30 coming, 4 backers");
	Expect(live::FundingLine(Opts[0].first, "Sunday") == "You've sold 35% of your Sunday.", "pieces: the funding line");
	Expect(live::CanRegister(Bank, L, O, Desk, Opts[0].first).empty() && !live::CanRegister(Bank - 1, L, O, Desk, Opts[0].first).empty(),
		"pieces: the buyers' money pays part of the buy-in, to the cent");
	const size_t Lines = L.Ledger.size();
	Expect(live::Register(Bank, L, O, Desk, 104, {}, Opts[0].first).empty(), "pieces: registered");
	Expect(L.Ledger.size() == Lines + 3 && L.Ledger[2].Label == "Pieces sold: Embercrest Sunday, 35% at 1.05" && L.Ledger[2].Amount == 5530, "pieces: one line, $55.30, before the buy-in");
	Expect(L.Stakes.Pieces.size() == 4 && L.Stakes.Listings[0].Collected, "pieces: four pieces on the entry, the listing collected");
	live::Settle(Bank, L, O.Id, 4, 104, 119000, O.Start + 400.0);
	const stake::Owed* Park = nullptr;
	const stake::Owed* LouOwe = nullptr;
	Chips Total = 0;
	for (const stake::Owed& W : L.Stakes.Owes)
	{
		Park = W.Who == "Mrs. Park" ? &W : Park;
		LouOwe = W.Who == "Big Lou" ? &W : LouOwe;
		Total += W.Cents;
	}
	Expect(Total == 41650, "pieces: $416.50 in other people's pockets");
	Expect(Park && Park->Cents == 11900 && std::fabs(Park->Due - (O.Start + 400.0 + 72.0 * Hour)) < 0.01, "pieces: Mrs. Park's $119.00, due in 72 hours");
	Expect(LouOwe && LouOwe->Cents == 17850 && std::fabs(LouOwe->Due - (O.Start + 400.0 + 14.0 * Day)) < 0.01, "pieces: Lou's $178.50, 14 days");
	const size_t OwesNow = L.Stakes.Owes.size();
	stake::OnSettle(L.Stakes, Bank, L, *live::EntryFor(L, O.Id), 119000, O.Start + 500.0);
	Expect(L.Stakes.Owes.size() == OwesNow, "pieces: owed records are made once");
	// The texts after the cage: Mrs. Park's to the cent.
	std::vector<std::pair<std::string, std::string>> Texts;
	stake::Step(L.Stakes, Bank, L, O.Start + 400.0, O.Start + 420.0, Texts);
	bool ParkSaid = false;
	for (const std::pair<std::string, std::string>& T : Texts)
	{
		ParkSaid = ParkSaid || (T.first == "Mrs. Park" && T.second.find("$119.00") != std::string::npos && T.second.find("4th") != std::string::npos);
	}
	Expect(ParkSaid, "pieces: Mrs. Park texts her ten percent, to the cent");

	// A cancelled event: the buyers repaid, markup included, once.
	{
		life::State L2;
		Chips Bank2 = 50000;
		L2.Stakes.Listings.push_back(Manual(O, 1050, {{"Mrs. Park", 100}, {"", 250}}, At));
		live::Register(Bank2, L2, O, Desk, 104, {}, stake::FundFor(L2.Stakes, O.Id, O.T->BuyInCents, true, Desk));
		Expect(live::Refund(Bank2, L2, O.Id, O.Start, "not enough players") && Bank2 == 50000, "pieces: a refund puts everything back");
		const life::LedgerEntry* Back = FindLine(L2, "Pieces bought back");
		Expect(Back && Back->Amount == -5530 && L2.Stakes.Pieces.empty(), "pieces: the buyers repaid $55.30, markup included");
		const Chips Was = Bank2;
		stake::OnRefund(L2.Stakes, Bank2, L2, *live::EntryFor(L2, O.Id), O.Start + 10.0);
		Expect(Bank2 == Was, "pieces: a refund repays once");
	}

	// The fill plan: drawn once, the same every time; named buyers by the rules of §2.5.
	{
		const ss::rep::Public P = Record(0, 0.0, 0, 31, 0.44, 0.0);
		stake::View V = ViewFor(P, At);
		V.Regards["Big Lou"] = Knows(0.2f);
		V.Regards["Mrs. Park"] = Knows(0.1f);
		V.Regards["Dre"] = Knows(0.1f);
		V.Regards["Sal"] = Knows(0.1f);
		V.Rolls["Big Lou"] = 1500000;
		V.Rolls["Mrs. Park"] = 900000;
		V.Rolls["Dre"] = 35000;
		V.Rolls["Sal"] = 600000;
		stake::Book A;
		stake::Book B;
		Expect(stake::List(A, V, O.Id, 400, 1050, At, 0.0).empty() && stake::List(B, V, O.Id, 400, 1050, At, 0.0).empty(), "pieces: listed");
		Expect(A.Listings[0].Plan == B.Listings[0].Plan, "pieces: the same plan every time (S7)");
		int LouPct = 0;
		int ParkPct = 0;
		int DrePct = 0;
		int SalPass = 0;
		for (const auto& Pl : A.Listings[0].Plan)
		{
			LouPct += std::get<1>(Pl) == "Big Lou" ? std::get<2>(Pl) : 0;
			ParkPct += std::get<1>(Pl) == "Mrs. Park" ? std::get<2>(Pl) : 0;
			DrePct += std::get<1>(Pl) == "Dre" ? std::get<2>(Pl) : 0;
			SalPass += std::get<1>(Pl) == "Sal" && std::get<2>(Pl) == 0 ? 1 : 0;
		}
		Expect((LouPct == 100 || LouPct == 150 || LouPct == 200) && ParkPct == 100 && DrePct == 50 && SalPass == 1, "pieces: Lou 10 to 20%, Mrs. Park 10%, Dre 5%; Sal passes on the markup");
		// Somebody the world knows is broke buys nothing (World::HeroMoney never takes them below $0, so their piece
		// would be money from nowhere); somebody the view knows nothing of (the card room, no world) buys as ever.
		{
			stake::View Vb = V;
			Vb.Rolls["Dre"] = 0;
			stake::View Vn = V;
			Vn.Rolls.erase("Dre");
			stake::Book Bb;
			stake::Book Bn;
			const bool Listed = stake::List(Bb, Vb, O.Id, 400, 1050, At, 0.0).empty() && stake::List(Bn, Vn, O.Id, 400, 1050, At, 0.0).empty();
			int Broke = 0;
			int Unknown = 0;
			for (const auto& Pl : Listed ? Bb.Listings[0].Plan : std::vector<std::tuple<double, std::string, int>>())
			{
				Broke += std::get<1>(Pl) == "Dre" ? std::get<2>(Pl) : 0;
			}
			for (const auto& Pl : Listed ? Bn.Listings[0].Plan : std::vector<std::tuple<double, std::string, int>>())
			{
				Unknown += std::get<1>(Pl) == "Dre" ? std::get<2>(Pl) : 0;
			}
			Expect(Listed && Broke == 0 && Unknown == 50, "pieces: a buyer the world knows is broke buys nothing; one it doesn't know buys");
		}
		Expect(stake::Ceiling(V, A, std::string(), true, 15000) == 1080, "pieces: strangers believe +8%: 1.08 (§2.1 scene 2)");
		Expect(stake::CeilingWhy(V, true, 15000) == "31 live events at +44%: they believe +8% of it.", "pieces: why buyers expect what they do");
		std::printf("  pieces: %s\n", stake::Projection(A, O.Id, At).c_str());
		// Steps text each named buyer once, when their fill comes due.
		life::State Lt;
		Lt.Stakes = A;
		Chips Bt = 0;
		std::vector<std::pair<std::string, std::string>> Tx;
		stake::Step(Lt.Stakes, Bt, Lt, At, At + 2.0 * Day, Tx);
		std::vector<std::pair<std::string, std::string>> Again;
		stake::Step(Lt.Stakes, Bt, Lt, At + 2.0 * Day, At + 2.0 * Day + 10.0, Again);
		bool SalSaid = false;
		for (const auto& T : Tx)
		{
			SalSaid = SalSaid || (T.first == "Sal" && T.second.find("par") != std::string::npos);
		}
		Expect(Tx.size() == 4 && Again.empty() && SalSaid, "pieces: Lou, Mrs. Park and Dre text as they buy, Sal says why he won't, once each");
	}
}

// ------------------------------------------------------------------ 2b. Mrs. Park's gate (update 2 bug 6)

/**
 * Whether Mrs. Park takes 10% of the Sunday at par from a player she trusts (Trust 0.4: a friend buys at par whatever the
 * price model believes), given the public record P (null: no record at all) and the online ROI's rank among the
 * regulars (-1: unknown, too few regulars on record to rank against). Nobody else is in the view, and with no live
 * sample the strangers expect a loss, so the plan holds her 10% or nothing.
 */
bool ParkBuys(const ss::rep::Public* P, double RoiRank)
{
	const live::Occurrence O = live::FindOccurrence("embercrest@6");
	const double At = O.Start - 2.0 * Day;
	stake::View V;
	V.Rec = P;
	V.World = At;
	V.Day = ss::net::DayOf(At);
	V.Bankroll = 100000;
	V.RoiRank = RoiRank;
	V.Regards["Mrs. Park"] = Knows(0.4f);
	V.Rolls["Mrs. Park"] = 900000;
	stake::Book B;
	if (!stake::List(B, V, O.Id, 100, 1000, At, 0.0).empty() || B.Listings.size() != 1)
	{
		return false;
	}
	int Pct = 0;
	for (const auto& Pl : B.Listings[0].Plan)
	{
		Pct += std::get<1>(Pl) == "Mrs. Park" ? std::get<2>(Pl) : 0;
	}
	return Pct == 100;
}

void ParkGate()
{
	// Mrs. Park wants a record (next_segment_spec §2.5): twenty live events, or an online ROI in the regulars' top half.
	// World::RoiRank says -1 when fewer than 20 regulars have 200 events on record (a fresh world's first months). That's
	// no verdict either way (update 2 SPEC §3.6): she reads the sample itself, 200 paid online events in profit.
	const ss::rep::Public Profit = Record(220, 0.06, 550, 0, 0.0, 0.0);
	const ss::rep::Public Loss = Record(220, -0.04, 550, 0, 0.0, 0.0);
	const ss::rep::Public Even = Record(220, 0.0, 550, 0, 0.0, 0.0);
	const ss::rep::Public Short = Record(199, 0.40, 550, 0, 0.0, 0.0);
	const ss::rep::Public Exactly = Record(200, 0.001, 550, 0, 0.0, 0.0);
	const ss::rep::Public Nothing = Record(0, 0.0, 0, 0, 0.0, 0.0);
	const ss::rep::Public Live20 = Record(0, 0.0, 0, 20, 0.0, 0.0);
	const ss::rep::Public Live19 = Record(220, -0.04, 550, 19, 0.0, 0.0);
	// Unknown rank: the sample decides.
	Expect(ParkBuys(&Profit, -1.0), "park: unknown rank, 220 paid events at +6%: she reads the sample and buys");
	Expect(ParkBuys(&Exactly, -1.0), "park: unknown rank, exactly 200 paid events just in profit: she buys");
	Expect(!ParkBuys(&Loss, -1.0), "park: unknown rank, 220 paid events at -4%: no piece");
	Expect(!ParkBuys(&Even, -1.0), "park: unknown rank, 220 paid events at break-even: no piece (in profit means in profit)");
	Expect(!ParkBuys(&Short, -1.0), "park: unknown rank, 199 paid events at +40%: the sample's too small");
	Expect(!ParkBuys(&Nothing, -1.0) && !ParkBuys(nullptr, -1.0), "park: unknown rank and no record: no piece");
	// A known rank decides as it always has: the sample never overrides it, either way.
	Expect(ParkBuys(&Loss, 0.62) && ParkBuys(&Loss, 0.5), "park: a top-half rank (the median included) at -4%: she buys");
	Expect(!ParkBuys(&Profit, 0.49) && !ParkBuys(&Profit, 0.0), "park: a bottom-half rank at +6%: no piece");
	Expect(!ParkBuys(&Short, 0.0), "park: under 200 online events (the session passes 0): no piece");
	// Twenty live events need no online record at all, ranked or not; nineteen leave it to the online record.
	Expect(ParkBuys(&Live20, -1.0) && ParkBuys(&Live20, 0.0), "park: twenty live events: she buys whatever the online rank");
	Expect(!ParkBuys(&Live19, -1.0) && ParkBuys(&Live19, 0.7), "park: nineteen live events: the online record decides");
	// The 90-day pause after a late payment holds whatever the record says.
	{
		const live::Occurrence O = live::FindOccurrence("embercrest@6");
		const double At = O.Start - 2.0 * Day;
		stake::View V = ViewFor(Profit, At);
		V.RoiRank = -1.0;
		V.Regards["Mrs. Park"] = Knows(0.4f);
		V.Rolls["Mrs. Park"] = 900000;
		stake::Book B;
		B.PausedUntil["Mrs. Park"] = At + 30.0 * Day;
		Expect(!stake::List(B, V, O.Id, 100, 1000, At, 0.0).empty() && B.Listings.empty(), "park: unknown rank, a good sample, but she's paused: no piece");
	}
	// Why break-even. A fresh world ranks again once 20 regulars have 200 events on record (its second week, seed 7: day
	// 12). Then the regulars' median sits near break-even, so the gate doesn't jump when the rank takes over from the
	// sample. (Later the field's losers reach 200 events too and the median falls well below it; an established world's
	// 1,600 regulars, survivors all, have a median near +5%, but an established world always has the regulars to rank.)
	{
		ss::world::World W;
		W.Create(7u, 1440.0 + 127.0, ss::world::Start::Blank);
		int Today = 1;
		while (W.RoiRank(0.0) < 0.0 && Today < 60)
		{
			++Today;
			W.AdvanceTo(static_cast<double>(Today) * Day + 600.0);
		}
		const double Breakeven = W.RoiRank(1e-9);
		std::printf("  park: a fresh world ranks the regulars again on day %d; break-even then beats %s of them\n", Today, ss::Fixed(Breakeven, 3).c_str());
		Expect(Today >= 3 && Today < 60 && Breakeven >= 0.3 && Breakeven <= 0.7, "park: when a fresh world ranks again, break-even sits near the regulars' median");
	}
}

// ------------------------------------------------------------------ 3. swaps

void Swaps()
{
	const live::Occurrence O = live::FindOccurrence("embercrest-turbo@5");
	for (const std::vector<std::string>& V : Vectors("swap"))
	{
		life::State L;
		Chips Bank = 50000;
		stake::Swap S;
		S.EntryId = O.Id;
		S.Partner = "Rick";
		S.Permille = std::atoi(V[1].c_str());
		S.Closes = O.LateRegEnds;
		L.Stakes.Swaps.push_back(S);
		const double Desk = O.DeskOpens + 5.0;
		live::Register(Bank, L, O, Desk, 70, {{"Rick", 4}}, life::Funding());
		live::Settle(Bank, L, O.Id, 9, 70, C(V[2]), O.Start + 200.0);
		stake::SettleSwap(L.Stakes, Bank, L, O.Id, "Rick", C(V[3]), O.Start + 260.0);
		const Chips Net = C(V[4]);
		const stake::Owed* W = L.Stakes.Owes.empty() ? nullptr : &L.Stakes.Owes[0];
		Expect(Net == 0 ? W == nullptr : (W && W->Cents == (Net < 0 ? -Net : Net) && W->ToYou == (Net > 0)), "swap vector " + V[2] + "/" + V[3] + ": net " + V[4]);
		if (W && W->ToYou)
		{
			// Rick pays in two to nine days.
			Expect(W->Due >= O.Start + 260.0 + 2.0 * Day && W->Due <= O.Start + 260.0 + 9.0 * Day, "swap: Rick pays in 2 to 9 days");
			const Chips Was = Bank;
			std::vector<std::pair<std::string, std::string>> Tx;
			stake::Step(L.Stakes, Bank, L, O.Start + 300.0, W->Due + 1.0, Tx);
			Expect(Bank == Was + (Net > 0 ? Net : 0), "swap: Rick paid up when he said");
		}
	}
	// Mei pays inside a day; the swap needs both in it; 70% shared with pieces.
	{
		const live::Occurrence Sun = live::FindOccurrence("embercrest@6");
		const ss::rep::Public P = Record(0, 0.0, 0, 20, 0.6, 0.0);
		stake::View V = ViewFor(P, Sun.Start - Day);
		V.Regards["Mei"] = Knows(0.2f, 0.35f);
		V.Playing[Sun.Id] = {"Mei"};
		stake::Book B;
		Expect(!stake::ProposeSwap(B, V, Sun.Id, "Rick", 100).empty(), "swap: Rick isn't in the Sunday");
		Expect(stake::ProposeSwap(B, V, Sun.Id, "Mei", 100).empty() && stake::SwappedOn(B, Sun.Id) == 100, "swap: 10% with Mei");
		Expect(!stake::ProposeSwap(B, V, Sun.Id, "Mei", 50).empty(), "swap: once per partner per event");
		Expect(!stake::List(B, V, Sun.Id, 650, 1000, Sun.Start - Day, 0.0).empty() && stake::List(B, V, Sun.Id, 600, 1000, Sun.Start - Day, 0.0).empty(),
			"swap: pieces stop at 70% less the swap");
		life::State L;
		L.Stakes = B;
		Chips Bank = 50000;
		live::Register(Bank, L, Sun, Sun.DeskOpens + 5.0, 100, {{"Mei", 5}}, stake::FundFor(L.Stakes, Sun.Id, 15000, true, Sun.DeskOpens + 5.0));
		live::Settle(Bank, L, Sun.Id, 50, 100, 0, Sun.Start + 100.0);
		stake::SettleSwap(L.Stakes, Bank, L, Sun.Id, "Mei", 300000, Sun.Start + 400.0);
		const stake::Owed* W = nullptr;
		for (const stake::Owed& X : L.Stakes.Owes)
		{
			W = X.Who == "Mei" ? &X : W;
		}
		Expect(W && W->ToYou && W->Cents == 30000 && W->Due <= Sun.Start + 400.0 + 24.0 * Hour, "swap: Mei owes $300 and pays inside a day");
		// A partner who never played: the swap lapses, nothing owed.
		stake::Book B2;
		stake::Swap S;
		S.EntryId = Sun.Id;
		S.Partner = "Mei";
		S.Permille = 100;
		S.State = stake::SwapLive;
		S.Mine = 50000;
		B2.Swaps.push_back(S);
		Chips Bk = 0;
		stake::SettleSwap(B2, Bk, L, Sun.Id, "Mei", -1, Sun.Start + 400.0);
		Expect(B2.Swaps[0].State == stake::SwapLapsed && B2.Owes.empty(), "swap: lapses when the partner didn't play");
	}
}

// ------------------------------------------------------------------ 4. a scripted package

void Packages()
{
	// The vectors (§2.8): a period with makeup carried into a cash; and a losing period that carries.
	for (const std::vector<std::string>& V : Vectors("package"))
	{
		life::State L;
		Chips Bank = 10000;
		const double T0 = 30.0 * Day;
		const Chips Roll = C(V[1]);
		stake::Deal D = PackageDeal(Roll == 220000 ? 2 : 1, T0);
		D.RollNow = Roll - C(V[2]);
		L.Stakes.Deals.push_back(D);
		OnlineEvent(L.Stakes, Bank, L, "vec-1", C(V[3]), C(V[4]), T0 + Day);
		const stake::Deal& P = L.Stakes.Deals[0];
		Expect(P.RollNow - P.Funded == C(V[6]), "package vector: the period nets " + V[6]);
		std::vector<std::pair<std::string, std::string>> Tx;
		stake::Step(L.Stakes, Bank, L, T0 + Day, T0 + 31.0 * Day, Tx);
		const life::LedgerEntry* Chop = FindLine(L, "the chop");
		Expect(C(V[7]) == 0 ? Chop == nullptr : (Chop && Chop->Amount == C(V[7])), "package vector: the chop pays " + V[7]);
		Expect(L.Stakes.Deals[0].RollNow == C(V[8]), "package vector: the roll after is " + V[8]);
		Expect(Bank == 10000 + C(V[7]), "package vector: the bankroll gets only the chop");
	}

	// Forty events on Lantern Row's $22 tier: the roll pays, prizes go into it, makeup carries, top-ups at 40 buy-ins.
	life::State L;
	Chips Bank = 5000;
	const double T0 = 60.0 * Day;
	L.Stakes.Deals.push_back(PackageDeal(2, T0));
	ss::Rng R("package40");
	Chips Prizes = 0;
	Chips BuyIns = 0;
	for (int K = 0; K < 40; ++K)
	{
		const Chips Prize = K == 37 ? 390000 : R.Int(10) == 0 ? 4400 : 0;
		OnlineEvent(L.Stakes, Bank, L, "ev-" + std::to_string(K), 2200, Prize, T0 + Day + K * 300.0);
		Prizes += Prize;
		BuyIns += 2200;
	}
	const stake::Deal& P = L.Stakes.Deals[0];
	Expect(Bank == 5000 && P.Events == 40 && P.PeriodEvents == 40, "package: forty events on the roll, the bankroll untouched");
	Expect(P.RollNow - P.Funded == Prizes - BuyIns, "package: the roll holds every prize and paid every buy-in");
	// Re-entries: two more bullets on the roll, the fourth on the player.
	OnlineEvent(L.Stakes, Bank, L, "reentry", 2200, 0, T0 + 20.0 * Day);
	OnlineEvent(L.Stakes, Bank, L, "reentry", 2200, 0, T0 + 20.0 * Day + 70.0);
	OnlineEvent(L.Stakes, Bank, L, "reentry", 2200, 0, T0 + 20.0 * Day + 140.0);
	Expect(Bank == 5000 && stake::FundFor(L.Stakes, "reentry", 2200, false, T0 + 20.0 * Day + 200.0).From == life::Paid::Own,
		"package: the entry and two re-entries on the roll; a fourth bullet is the player's own money");
	const std::vector<std::pair<life::Funding, std::string>> Opts = stake::OnlineFundingOptions(L, "reentry", 2200, T0 + 20.0 * Day + 200.0);
	Expect(Opts.size() == 2 && Opts[0].first.From == life::Paid::Package && Opts[0].second == "Lantern Row Staking covers two re-entries. This bullet's on you.",
		"package: the desk says so");
	Expect(stake::FundFor(L.Stakes, "big", 5500, false, T0).From == life::Paid::Own, "package: over the cap is the player's own money");
	Expect(stake::FundFor(L.Stakes, "free", 0, false, T0).From == life::Paid::Own, "package: a freeroll isn't on the roll");

	// A cold run: makeup, top-ups, a losing period carried, then the 90-day review over 150 buy-ins cuts it loose.
	{
		life::State Lc;
		Chips Bc = 0;
		const double Tc = 100.0 * Day;
		Lc.Stakes.Deals.push_back(PackageDeal(2, Tc));
		int Topups = 0;
		for (int Period = 0; Period < 3; ++Period)
		{
			for (int K = 0; K < 120; ++K)
			{
				// At the cap the package is at now (two losing periods step it down a tier).
				OnlineEvent(Lc.Stakes, Bc, Lc, "cold-" + std::to_string(Period) + "-" + std::to_string(K), Lc.Stakes.Deals[0].T.CapOnline, 0,
					Tc + Period * 30.0 * Day + 60.0 + K * 300.0);
			}
			std::vector<std::pair<std::string, std::string>> Tx;
			stake::Step(Lc.Stakes, Bc, Lc, Tc + Period * 30.0 * Day, Tc + (Period + 1) * 30.0 * Day + 1.0, Tx);
		}
		for (const stake::Line& Ln : Lc.Stakes.Deals[0].Lines)
		{
			Topups += Ln.Label.find("Topped up") != std::string::npos ? 1 : 0;
		}
		const stake::Deal& D = Lc.Stakes.Deals[0];
		// (The roll keeps its last 40 lines: the top-ups show in what the backer has in.)
		Expect(Topups >= 1 && D.Funded >= 220000 + 4 * 88000 && D.Makeup == 300 * 2200 && D.Periods == 3 && D.State == stake::DealEnded && D.Tier == 1,
			"package: topped up, stepped down after two losing periods, cut loose at the 90-day review");
		bool Letter = false;
		for (const life::Letter& M : Lc.Inbox)
		{
			Letter = Letter || (M.Source == "stake" && M.Body == "We're ending the package. The makeup is ours to eat. Good luck out there.");
		}
		Expect(Letter && Bc == 0 && stake::YouOwe(Lc.Stakes) == 0, "package: the makeup is theirs to eat; the player owes nothing");
		Expect(Lc.Stakes.PausedUntil["lanternrow"] >= Tc + 90.0 * Day + 179.0 * Day, "package: their offers stay away 180 days");
	}

	// Sam: makeup becomes a debt if you quit, a quarter of every cash and shift until it's paid.
	{
		life::State Ls;
		Chips Bs = 20000;
		const double Ts = 200.0 * Day;
		stake::Deal D;
		D.Id = "deal:sam:test";
		D.K = stake::Kind::Package;
		D.By = stake::Outfit::Sam;
		D.Backer = "Sam";
		D.T.CutPermille = 350;
		D.T.CapOnline = 5500;
		D.T.CapLive = 12000;
		D.T.Roll = 150000;
		D.T.MakeupIsDebt = true;
		D.T.GhostsPerWeek = 2;
		D.NextChop = Ts + 30.0 * Day;
		D.RollNow = 150000;
		D.Funded = 150000;
		Ls.Stakes.Deals.push_back(D);
		for (int K = 0; K < 10; ++K)
		{
			OnlineEvent(Ls.Stakes, Bs, Ls, "sam-" + std::to_string(K), 5500, 0, Ts + 60.0 + K * 200.0);
		}
		Expect(stake::Chop(Ls.Stakes, Bs, Ls, D.Id, Ts + Day) == "i dont do chops on request.", "sam: no chop on request");
		Expect(stake::WalkAway(Ls.Stakes, Bs, Ls, D.Id, Ts + Day).empty() && Ls.Stakes.Deals[0].Debt == 55000 && stake::YouOwe(Ls.Stakes) == 55000,
			"sam: walking with $550 of makeup makes it a debt");
		// A live cash: 25% of it to Sam.
		const live::Occurrence O = live::FindOccurrence("embercrest-nightly@201");
		live::Register(Bs, Ls, O, O.DeskOpens + 5.0, 80, {}, life::Funding());
		const Chips Was = Bs;
		live::Settle(Bs, Ls, O.Id, 3, 80, 80000, O.Start + 400.0);
		Expect(Bs == Was + 80000 - 20000 && Ls.Stakes.Deals[0].Debt == 35000, "sam: a quarter of the $800 cash");
		// A shift (ShiftLink pays kind 1): a quarter of it too, once.
		Ls.Record(O.Start + Day, "ShiftLink: night cashier", 8000, 1);
		Bs += 8000;
		std::vector<std::pair<std::string, std::string>> Tx;
		stake::Step(Ls.Stakes, Bs, Ls, O.Start + Day - 10.0, O.Start + Day + 10.0, Tx);
		stake::Step(Ls.Stakes, Bs, Ls, O.Start + Day + 10.0, O.Start + Day + 20.0, Tx);
		Expect(Ls.Stakes.Deals[0].Debt == 33000, "sam: a quarter of the shift, once");
		Expect(!Ls.Stakes.Burner.empty() && Ls.Stakes.Burner.back().second.find("left on the debt") != std::string::npos, "sam: he keeps you posted on the Burner");
	}

	// A cap won in a counter holds at the period's end: only a move between tiers rewrites the terms (and its letter).
	{
		life::State Lt;
		Chips Bt = 0;
		stake::Deal D = PackageDeal(1, 0.0);
		D.T.CapOnline = 2200;
		Lt.Stakes.Deals.push_back(D);
		std::vector<std::pair<std::string, std::string>> Tx;
		stake::Step(Lt.Stakes, Bt, Lt, 0.0, 30.0 * Day + 1.0, Tx);
		bool Moved = false;
		for (const life::Letter& M : Lt.Inbox)
		{
			Moved = Moved || M.Subject == "Stepping you down" || M.Subject == "Your numbers moved";
		}
		Expect(Lt.Stakes.Deals[0].T.CapOnline == 2200 && Lt.Stakes.Deals[0].Tier == 1 && !Moved && Lt.Stakes.Deals[0].RollNow == 110000,
			"package: a countered cap holds through the period's end, no tier letter");
	}
}

// ------------------------------------------------------------------ 5. conservation, 6. idempotence

void Conservation()
{
	// A Lou stake, a pieces sale, a swap and a package run: the player's bankroll, the roll and everyone's flows add up
	// to prizes less buy-ins, to the cent.
	life::State L;
	Chips Bank = 200000;
	const Chips Start = Bank;
	Chips Prizes = 0;
	Chips Costs = 0;
	// Lou's stake: a cash.
	const live::Occurrence N1 = live::FindOccurrence("embercrest-nightly@9");
	stake::Accept(L.Stakes, Bank, L, LouOffer(N1, N1.DeskOpens), N1.DeskOpens);
	live::Register(Bank, L, N1, N1.DeskOpens + 5.0, 88, {}, stake::FundFor(L.Stakes, N1.Id, 12000, true, N1.DeskOpens + 5.0));
	live::Settle(Bank, L, N1.Id, 2, 88, 150000, N1.Start + 400.0);
	Prizes += 150000;
	Costs += 12000;
	// Pieces of the Sunday, a market buyer included, and a swap with Mei.
	const live::Occurrence Sun = live::FindOccurrence("embercrest@13");
	L.Stakes.Listings.push_back(Manual(Sun, 1100, {{"Mrs. Park", 100}, {"", 150}}, Sun.Start - Day));
	stake::Swap S;
	S.EntryId = Sun.Id;
	S.Partner = "Mei";
	S.Permille = 100;
	S.Closes = Sun.LateRegEnds;
	L.Stakes.Swaps.push_back(S);
	live::Register(Bank, L, Sun, Sun.DeskOpens + 5.0, 100, {{"Mei", 5}}, stake::FundFor(L.Stakes, Sun.Id, 15000, true, Sun.DeskOpens + 5.0));
	live::Settle(Bank, L, Sun.Id, 7, 100, 61000, Sun.Start + 400.0);
	stake::SettleSwap(L.Stakes, Bank, L, Sun.Id, "Mei", 22000, Sun.Start + 500.0);
	Prizes += 61000;
	Costs += 15000;
	// Everything owed paid, both ways.
	std::vector<std::pair<std::string, std::string>> Tx;
	stake::Step(L.Stakes, Bank, L, Sun.Start + 500.0, Sun.Start + 20.0 * Day, Tx);
	// A package month.
	L.Stakes.Deals.push_back(PackageDeal(1, Sun.Start + 20.0 * Day));
	const Chips RollIn = L.Stakes.Deals.back().T.Roll;
	Chips PkgPrizes = 0;
	Chips PkgCosts = 0;
	for (int K = 0; K < 120; ++K)
	{
		const Chips Prize = K == 60 ? 160000 : K % 9 == 0 ? 2500 : 0;
		OnlineEvent(L.Stakes, Bank, L, "c-" + std::to_string(K), 1100, Prize, Sun.Start + 20.0 * Day + 60.0 + K * 300.0);
		PkgPrizes += Prize;
		PkgCosts += 1100;
	}
	stake::Step(L.Stakes, Bank, L, Sun.Start + 20.0 * Day, Sun.Start + 51.0 * Day, Tx);
	Chips Flows = 0;
	for (const stake::Flow& F : L.Stakes.ToWorld)
	{
		Flows += F.Cents;
	}
	// The market: what it paid for its pieces, and what it was paid back.
	Chips Market = 0;
	for (const stake::Piece& P : L.Stakes.Pieces)
	{
		Market += P.Buyer.empty() ? P.Share - P.Paid : 0;
	}
	// The package: what's in the roll now, less what the backer put in, plus what the backer took at chops.
	const stake::Deal& D = L.Stakes.Deals.back();
	Chips Injected = RollIn;
	Chips Taken = 0;
	for (const stake::Line& Ln : D.Lines)
	{
		Injected += Ln.Label.find("Topped up") != std::string::npos || Ln.Label.find("sized") != std::string::npos ? Ln.Amount : 0;
		Taken += Ln.Label.find("the rest to") != std::string::npos ? -Ln.Amount : 0;
	}
	const Chips Player = Bank - Start;
	Expect(Player + Flows + Market + (D.RollNow - Injected + Taken) == Prizes - Costs + PkgPrizes - PkgCosts,
		"conservation: the bankroll, the roll, every backer and buyer: prizes less buy-ins, to the cent");
	// A swap settling the player's way: Mei's payment reaches the world as a swap settling (Memory::Swapped), never as the
	// player paying her what was owed.
	{
		life::State Ls;
		Chips Bs = 100000;
		stake::Swap Sw;
		Sw.EntryId = Sun.Id;
		Sw.Partner = "Mei";
		Sw.Permille = 100;
		Sw.Closes = Sun.LateRegEnds;
		Ls.Stakes.Swaps.push_back(Sw);
		live::Register(Bs, Ls, Sun, Sun.DeskOpens + 5.0, 100, {{"Mei", 5}}, stake::FundFor(Ls.Stakes, Sun.Id, 15000, true, Sun.DeskOpens + 5.0));
		live::Settle(Bs, Ls, Sun.Id, 60, 100, 0, Sun.Start + 400.0);
		stake::SettleSwap(Ls.Stakes, Bs, Ls, Sun.Id, "Mei", 90000, Sun.Start + 500.0);
		std::vector<std::pair<std::string, std::string>> Tm;
		stake::Step(Ls.Stakes, Bs, Ls, Sun.Start + 500.0, Sun.Start + 20.0 * Day, Tm);
		const stake::Flow* Paid = nullptr;
		for (const stake::Flow& F : Ls.Stakes.ToWorld)
		{
			Paid = F.Id.rfind("paidyou:", 0) == 0 ? &F : Paid;
		}
		Expect(Paid && Paid->Npc == "Mei" && Paid->Cents == -9000 && Paid->Memory == static_cast<int>(stake::Memory::Swapped), "swap: Mei paying you is a swap settling, to the world");
	}
	std::printf("  conservation: player %+lld, people %+lld, market %+lld, the roll's backer %+lld, prizes - buy-ins %+lld\n", static_cast<long long>(Player),
		static_cast<long long>(Flows), static_cast<long long>(Market), static_cast<long long>(D.RollNow - Injected + Taken),
		static_cast<long long>(Prizes - Costs + PkgPrizes - PkgCosts));
	// The ledger is the bankroll's story: every kind-10 line and prize line adds up.
	Chips Sum = 0;
	for (const life::LedgerEntry& X : L.Ledger)
	{
		Sum += X.Amount;
	}
	Expect(L.Ledger.size() < 60 ? Sum == Bank - Start : true, "conservation: the ledger sums to the bankroll's change");
}

void Idempotence()
{
	const live::Occurrence O = live::FindOccurrence("embercrest@6");
	life::State L;
	Chips Bank = 100000;
	L.Stakes.Listings.push_back(Manual(O, 1050, {{"Mrs. Park", 100}, {"Big Lou", 100}}, O.Start - Day));
	live::Register(Bank, L, O, O.DeskOpens + 5.0, 104, {}, stake::FundFor(L.Stakes, O.Id, 15000, true, O.DeskOpens + 5.0));
	// The registration's line again, after a save and a load: nothing.
	std::string Text = Save(L, Bank);
	Expect(Load(Text, L, Bank), "idempotence: loads");
	const Chips B1 = Bank;
	stake::OnRegister(L.Stakes, Bank, L, *live::EntryFor(L, O.Id), O.DeskOpens + 6.0);
	Expect(Bank == B1, "idempotence: the pieces' money moves once");
	live::Settle(Bank, L, O.Id, 2, 104, 300000, O.Start + 500.0);
	const size_t Owes = L.Stakes.Owes.size();
	Text = Save(L, Bank);
	Expect(Load(Text, L, Bank) && Save(L, Bank) == Text, "idempotence: the book round-trips byte for byte");
	const Chips B2 = Bank;
	stake::OnSettle(L.Stakes, Bank, L, *live::EntryFor(L, O.Id), 300000, O.Start + 600.0);
	Expect(Bank == B2 && L.Stakes.Owes.size() == Owes, "idempotence: a settle after a reload moves nothing");
	// The calendar over the same span twice, with a save between: paid once.
	std::vector<std::pair<std::string, std::string>> Tx;
	stake::Step(L.Stakes, Bank, L, O.Start + 500.0, O.Start + 4.0 * Day, Tx);
	const Chips B3 = Bank;
	Text = Save(L, Bank);
	Load(Text, L, Bank);
	stake::Step(L.Stakes, Bank, L, O.Start + 500.0, O.Start + 4.0 * Day, Tx);
	Expect(Bank == B3 && B3 < B2, "idempotence: the owed paid once, across a reload");
}

// ------------------------------------------------------------------ 7. due dates

void DueDates()
{
	const double T0 = 40.0 * Day + 21.0 * Hour;
	auto Fresh = [T0](const std::string& Who, Chips Cents, double Due) {
		stake::Owed O;
		O.Id = "owe:test:" + Who;
		O.Who = Who;
		O.Label = "10% of your Sunday, 4th";
		O.Cents = Cents;
		O.Since = T0;
		O.Due = Due;
		O.EntryId = "embercrest@41";
		return O;
	};
	// Rent of $1,075 due inside seven days: auto-pay keeps it back (§2.1 scene 2), and the payment goes late at the minute.
	{
		life::State L;
		L.RentStage = life::Rent::Due;
		L.RentDueCents = 107500;
		L.RentDeadline = T0 + 5.0 * Day;
		Chips Bank = 118000;
		const double Due = T0 + 72.0 * Hour;
		L.Stakes.Owes.push_back(Fresh("Mrs. Park", 11900, Due));
		std::vector<std::pair<std::string, std::string>> Tx;
		stake::Step(L.Stakes, Bank, L, T0, Due - 1.0, Tx);
		Expect(L.Stakes.Owes[0].State == stake::OwedOpen, "due: not late a minute early");
		stake::Step(L.Stakes, Bank, L, Due - 1.0, Due, Tx);
		Expect(L.Stakes.Owes[0].State == stake::OwedLate && Bank == 118000 && !L.Stakes.Record.Marks.empty() && L.Stakes.Record.Marks.back().first == Due &&
				   L.Stakes.Record.Marks.back().second == stake::MarkLate,
			"due: the rent reserve holds auto-pay; late at the due minute");
		Expect(L.Stakes.PausedUntil["Mrs. Park"] >= Due + 89.0 * Day, "due: one late payment and Mrs. Park stops for 90 days");
		stake::Step(L.Stakes, Bank, L, Due, Due + 7.0 * Day - 1.0, Tx);
		Expect(L.Stakes.Owes[0].State == stake::OwedLate, "due: not welched a minute early");
		stake::Step(L.Stakes, Bank, L, Due + 7.0 * Day - 1.0, Due + 7.0 * Day, Tx);
		Expect(L.Stakes.Owes[0].State == stake::OwedWelched && L.Stakes.Record.Welched == 1 && L.Stakes.Record.Marks.back().first == Due + 7.0 * Day,
			"due: welched seven days past due, at the minute");
		// Rent paid: the money covers it and it's paid at the next step.
		L.RentStage = life::Rent::Paid;
		stake::Step(L.Stakes, Bank, L, Due + 7.0 * Day, Due + 7.0 * Day + 1.0, Tx);
		Expect(L.Stakes.Owes[0].State == stake::OwedPaid && Bank == 118000 - 11900, "due: still owed, paid once the money covers it");
	}
	// Lou: fourteen days to pay, and fourteen more before it's a welch.
	{
		life::State L;
		Chips Bank = 0;
		const double Due = T0 + 14.0 * Day;
		L.Stakes.Owes.push_back(Fresh("Big Lou", 17850, Due));
		std::vector<std::pair<std::string, std::string>> Tx;
		stake::Step(L.Stakes, Bank, L, T0, Due + 13.0 * Day, Tx);
		Expect(L.Stakes.Owes[0].State == stake::OwedLate, "due: Lou's late after fourteen days");
		stake::Step(L.Stakes, Bank, L, Due + 13.0 * Day, Due + 14.0 * Day, Tx);
		Expect(L.Stakes.Owes[0].State == stake::OwedWelched, "due: welched on Lou fourteen days after that");
		bool Late = false;
		bool Welch = false;
		for (const auto& T : Tx)
		{
			Late = Late || (T.first == "Big Lou" && T.second.find("Fourteen days was the deal") != std::string::npos);
			Welch = Welch || (T.first == "Big Lou" && T.second.find("covered your check") != std::string::npos);
		}
		Expect(Late && Welch, "due: Lou says so, both times");
	}
	// Auto-pay when the money's there; early payment by hand counts as early.
	{
		life::State L;
		L.RentStage = life::Rent::Paid;
		Chips Bank = 50000;
		L.Stakes.Owes.push_back(Fresh("Sal", 5950, T0 + 72.0 * Hour));
		L.Stakes.Owes.push_back(Fresh("Dre", 5950, T0 + 72.0 * Hour));
		Expect(stake::Pay(L.Stakes, Bank, L, "owe:test:Dre", T0 + 2.0 * Hour).empty() && Bank == 50000 - 5950, "due: paid by hand");
		std::vector<std::pair<std::string, std::string>> Tx;
		stake::Step(L.Stakes, Bank, L, T0, T0 + 72.0 * Hour, Tx);
		Expect(L.Stakes.Owes[0].State == stake::OwedPaid && Bank == 50000 - 11900 && L.Stakes.Record.Honored == 2, "due: auto-paid at the due time");
		Expect(!stake::Pay(L.Stakes, Bank, L, "owe:test:Sal", T0 + 80.0 * Hour).empty(), "due: nothing pays twice");
	}
	// A welch that sits unpaid for months: the result was texted once, and the pruning never lets it be texted again.
	{
		life::State L;
		Chips Bank = 0;
		L.Stakes.Owes.push_back(Fresh("Mrs. Park", 11900, T0 + 72.0 * Hour));
		std::vector<std::pair<std::string, std::string>> First;
		stake::Step(L.Stakes, Bank, L, T0, T0 + 5.0, First);
		std::vector<std::pair<std::string, std::string>> Months;
		stake::Step(L.Stakes, Bank, L, T0 + 5.0, T0 + 200.0 * Day, Months);
		std::vector<std::pair<std::string, std::string>> Later;
		stake::Step(L.Stakes, Bank, L, T0 + 200.0 * Day, T0 + 200.0 * Day + 90.0, Later);
		bool Again = false;
		for (const auto& T : Later)
		{
			Again = Again || T.second.find("Congratulations") != std::string::npos;
		}
		Expect(First.size() == 1 && L.Stakes.Owes[0].State == stake::OwedWelched && !Again, "due: an old welch is never re-texted as news");
	}
}

// ------------------------------------------------------------------ 8. negotiation, 9. prices

void Negotiation()
{
	const live::Occurrence O = live::FindOccurrence("embercrest-nightly@16");
	stake::Offer Of = LouOffer(O, O.Start - Day);
	// Across seeds and days: an answer per (offer, day), the same every time.
	int Yes = 0;
	int Asked = 0;
	for (int Dy = 0; Dy < 40; ++Dy)
	{
		Of.Id = "lou:test:" + std::to_string(Dy);
		const ss::stake::Terms Ask = stake::CounterTerms(Of, 0);
		const int A = stake::Answer(Of, Ask, 100 + Dy);
		Expect(A == stake::Answer(Of, Ask, 100 + Dy), "negotiation: the same answer twice");
		Yes += A == 1 ? 1 : 0;
		++Asked;
		const double M = stake::Margin(Of, Ask);
		Expect(M > 0.0, "negotiation: five points more inside Lou's walk-away (ten better than his offer)");
	}
	Expect(Yes > Asked / 2 && Yes < Asked, "negotiation: Lou says yes to most counters inside his walk-away, not all");
	Expect(std::string(ss::rep::Mood(stake::Margin(Of, stake::CounterTerms(Of, 0)))) == "Sounds open to it", "negotiation: the mood line before it's sent");
	stake::Offer Far = Of;
	Far.T.CutPermille = 650;
	Expect(std::string(ss::rep::Mood(stake::Margin(Of, Far.T))) == "Don't push it", "negotiation: too far is too far");
	// The same after a reload.
	life::State L;
	Chips Bank = 0;
	Of.Id = "lou:" + O.Id;
	L.Stakes.Offers.push_back(Of);
	life::State Back;
	Chips BankBack = 0;
	Expect(Load(Save(L, Bank), Back, BankBack) && Back.Stakes.Offers.size() == 1, "negotiation: the offer survives a save");
	const ss::stake::Terms Ask = stake::CounterTerms(Of, 0);
	Expect(stake::Answer(Of, Ask, 77) == stake::Answer(Back.Stakes.Offers[0], Ask, 77), "negotiation: the same answer after a reload");
	// One counter a day; a refusal leaves it standing 24 hours; a second refusal withdraws it for a week.
	int Day0 = 0;
	for (int Dy = 0; Dy < 400; ++Dy)
	{
		stake::Offer Probe = Of;
		Probe.Id = "lou:refuse:" + std::to_string(Dy);
		if (stake::Answer(Probe, stake::CounterTerms(Probe, 0), 50) == 0)
		{
			Day0 = Dy;
			break;
		}
	}
	life::State Lr;
	Chips Br = 0;
	stake::Offer Probe = Of;
	Probe.Id = "lou:refuse:" + std::to_string(Day0);
	Probe.Expires = 60.0 * Day;
	Lr.Stakes.Offers.push_back(Probe);
	const std::string First = stake::Counter(Lr.Stakes, Br, Lr, Probe.Id, 0, 50.0 * Day + 600.0);
	Expect(First == "Take it or leave it, kid." && Lr.Stakes.Offers[0].Counters == 1 && Lr.Stakes.Offers[0].Expires <= 50.0 * Day + 600.0 + 24.0 * Hour,
		"negotiation: a refused counter stands for 24 hours");
	Expect(stake::Counter(Lr.Stakes, Br, Lr, Probe.Id, 0, 50.0 * Day + 700.0) == "One counter a day. Give them till tomorrow.", "negotiation: one counter a day");
	Expect(!stake::Counter(Lr.Stakes, Br, Lr, Probe.Id, 1, 50.0 * Day + 700.0).empty(), "negotiation: a stake has no markup to counter");
	// Lantern Row answers a counter by letter; a second refusal shuts its door for the week.
	{
		life::State Lw;
		Chips Bw = 0;
		stake::Offer Pkg;
		Pkg.K = stake::Kind::Package;
		Pkg.By = stake::Outfit::LanternRow;
		Pkg.Backer = "Lantern Row Staking";
		Pkg.Tier = 1;
		Pkg.T = stake::LanternRowTerms(1);
		Pkg.Expires = 90.0 * Day;
		int Refused = -1;
		for (int K = 0; K < 400 && Refused < 0; ++K)
		{
			Pkg.Id = "lanternrow:probe:" + std::to_string(K);
			Refused = stake::Answer(Pkg, stake::CounterTerms(Pkg, 0), 60) == 0 && stake::Answer(Pkg, stake::CounterTerms(Pkg, 0), 61) != 1 ? K : -1;
		}
		Pkg.Id = "lanternrow:probe:" + std::to_string(Refused);
		Lw.Stakes.Offers.push_back(Pkg);
		const std::string Stands = stake::Counter(Lw.Stakes, Bw, Lw, Pkg.Id, 0, 60.0 * Day + 600.0);
		Pkg.Counters = 1;
		stake::Counter(Lw.Stakes, Bw, Lw, Pkg.Id, 0, 61.0 * Day + 600.0);
		Expect(Refused >= 0 && Stands == "The offer stands as written for another day." && Lw.Inbox.size() == 2 && Lw.Inbox[0].Subject == "Your counter" &&
				   Lw.Inbox[0].Source == "stake" && Lw.Stakes.Offers.empty() && Lw.Stakes.PausedUntil["lanternrow"] >= 68.0 * Day,
			"negotiation: Lantern Row writes back, and a second no shuts its door for a week");
	}
}

void Prices()
{
	// §2.6's table.
	const double Micro = stake::OnlinePrior(1100);
	Expect(std::fabs(Micro + 100.0 / 1100.0) < 1e-12 && std::fabs(stake::OnlinePrior(330) - Micro) < 1e-12 && std::fabs(stake::OnlinePrior(5500) - Micro) < 1e-12,
		"prices: the online prior is the rake, 9.1% from $3.30 to $55");
	Expect(std::fabs(stake::LivePrior(12000) + 0.125) < 1e-12 && std::fabs(stake::LivePrior(15000) + 0.10) < 1e-12 && std::fabs(stake::LivePrior(6000) + 0.1333) < 0.001,
		"prices: the live prior is the fee, 10% to 13.3%");
	stake::Book B;
	struct Row
	{
		int N;
		double Roi;
		Chips Abi;
		double Renown;
		bool Live;
		double Believe;
		int Ceiling;
	};
	const Row Rows[] = {
		{40, 0.60, 1100, 0.0, false, -0.048, 1000},
		{300, 0.25, 1100, 0.0, false, 0.023, 1020},
		{1000, 0.30, 2200, 0.0, false, 0.153, 1150},
		{1000, 0.30, 2200, 0.6, false, 0.153, 1210},
		{40, 0.60, 0, 0.0, true, 0.165, 1170},
	};
	for (const Row& R : Rows)
	{
		const ss::rep::Public P = R.Live ? Record(0, 0.0, 0, R.N, R.Roi, R.Renown) : Record(R.N, R.Roi, R.Abi, 0, 0.0, R.Renown);
		const stake::View V = ViewFor(P, 30.0 * Day);
		const double Exp = stake::ExpRoiOf(V, R.Live, R.Live ? 12000 : R.Abi);
		Expect(std::fabs(Exp - R.Believe) < 0.0015, "prices: buyers believe " + ss::Fixed(R.Believe * 100.0, 1) + "% (got " + ss::Fixed(Exp * 100.0, 2) + ")");
		Expect(stake::Ceiling(V, B, std::string(), R.Live, R.Live ? 12000 : R.Abi) == R.Ceiling, "prices: the ceiling " + std::to_string(R.Ceiling));
	}
	// Caps: 1.20 local, 1.35 online, 1.50 at a regional main.
	const ss::rep::Public Hot = Record(5000, 2.0, 2200, 400, 3.0, 1.0);
	const stake::View V = ViewFor(Hot, 0.0);
	Expect(stake::Ceiling(V, B, std::string(), true, 12000) == 1200 && stake::Ceiling(V, B, std::string(), false, 2200) == 1350 &&
			   stake::Ceiling(V, B, std::string(), true, 300000) == 1500,
		"prices: capped at 1.20 local, 1.35 online, 1.50 regional");
	// Conduct: a welch in the last 180 days takes 0.10 off every ceiling.
	ss::rep::Public Welched = Record(1000, 0.30, 2200, 0, 0.0, 0.0);
	Welched.Welched180 = true;
	Expect(stake::Ceiling(ViewFor(Welched, 0.0), B, std::string(), false, 2200) == 1050, "prices: word of a welch costs 0.10");
}

// ------------------------------------------------------------------ 10. backer EV (Monte Carlo)

/** A 300-runner field, 15% paid, the rake from FeeOf, the payouts falling off as 1/place. */
struct Field
{
	int N = 300;
	int Paid = 45;
	std::vector<double> Pay; // prize per place, in buy-ins
	double A = 1.0;          // the finish skew: P(top fraction x) = x^A (A < 1: better than the field)
	double Expected(double Skew) const
	{
		double E = 0.0;
		for (int K = 1; K <= Paid; ++K)
		{
			const double P = std::pow(static_cast<double>(K) / N, Skew) - std::pow(static_cast<double>(K - 1) / N, Skew);
			E += P * Pay[static_cast<size_t>(K - 1)];
		}
		return E;
	}
};

Field MakeField(Chips BuyIn, double Edge, int N = 300, int Paid = 45)
{
	Field F;
	F.N = N;
	F.Paid = Paid;
	const double Pool = static_cast<double>(N) * static_cast<double>(BuyIn - ss::net::FeeOf(BuyIn)) / static_cast<double>(BuyIn);
	double Sum = 0.0;
	for (int K = 1; K <= Paid; ++K)
	{
		Sum += 1.0 / K;
	}
	for (int K = 1; K <= Paid; ++K)
	{
		F.Pay.push_back(Pool * (1.0 / K) / Sum);
	}
	// The skew that makes the expected prize (1 + Edge) buy-ins.
	double Lo = 0.05;
	double Hi = 1.5;
	for (int I = 0; I < 80; ++I)
	{
		const double Mid = (Lo + Hi) / 2.0;
		(F.Expected(Mid) > 1.0 + Edge ? Lo : Hi) = Mid;
	}
	F.A = (Lo + Hi) / 2.0;
	return F;
}

/** A finish drawn from the field: the prize in cents for a BuyIn entry (0 out of the money). */
Chips PrizeOf(const Field& F, ss::Rng& R, Chips BuyIn)
{
	const double U = R.Next();
	const double Frac = std::pow(U, 1.0 / F.A);
	const int Place = std::min(F.N, static_cast<int>(Frac * F.N) + 1);
	return Place <= F.Paid ? static_cast<Chips>(std::llround(F.Pay[static_cast<size_t>(Place - 1)] * static_cast<double>(BuyIn))) : 0;
}

/**
 * Lantern Row's package the way Stake.cpp runs it, in cents: the top-ups under 40 buy-ins, the chop only when the period
 * ends up, makeup carried, the 90-day cut-loose over 150 buy-ins (a fresh package after it), and, when the horse asks for
 * every one he can get, the early chop at twenty buy-ins up. The Monte Carlo runs this; CodeMatchesModel checks it against
 * the code, cent for cent, so the economics the sims promise are the code's.
 */
struct PackageModel
{
	stake::Terms T;
	Chips RollNow = 0;
	Chips Funded = 0;
	Chips Backer = 0; // the backer's money: out when the roll is funded or topped up, back at chops and cut-loose
	Chips Horse = 0;  // the player's chops
	int Periods = 0;
	int CutLoose = 0;
	explicit PackageModel(const stake::Terms& Terms) : T(Terms)
	{
		Fund();
	}
	void Fund()
	{
		RollNow = T.Roll;
		Funded = T.Roll;
		Backer -= T.Roll;
		Periods = 0;
	}
	void TopUp()
	{
		const Chips Floor = std::max(std::min<Chips>(40 * T.CapOnline, T.Roll / 2), T.CapOnline);
		if (RollNow < Floor && RollNow < T.Roll)
		{
			const Chips Add = T.Roll - RollNow;
			RollNow += Add;
			Funded += Add;
			Backer -= Add;
		}
	}
	void Chop()
	{
		const Chips Profit = RollNow - Funded;
		const Chips Theirs = Profit * static_cast<Chips>(1000 - T.CutPermille) / 1000;
		Horse += Profit - Theirs;
		Backer += RollNow - (Profit - Theirs) - T.Roll;
		RollNow = T.Roll;
		Funded = T.Roll;
	}
	void Event(Chips BuyIn, Chips Prize, bool Early)
	{
		if (RollNow < BuyIn)
		{
			TopUp();
		}
		RollNow -= BuyIn;
		TopUp();
		RollNow += Prize;
		if (Early && RollNow - Funded >= 20 * T.CapOnline)
		{
			Chop();
		}
	}
	void PeriodEnd()
	{
		++Periods;
		if (RollNow - Funded > 0)
		{
			Chop();
		}
		if (Periods % 3 == 0 && Funded - RollNow > 150 * T.CapOnline)
		{
			Backer += RollNow;
			++CutLoose;
			Fund();
		}
	}
	/** The backer's money once the roll comes home. */
	Chips Net() const
	{
		return Backer + RollNow;
	}
};

/** The backer's EV a year (12 periods) in buy-ins, running the package model for Periods periods at a true edge. */
double BackerEv(const stake::Terms& T, double Edge, int Periods, bool Early, const char* Seed)
{
	const Field F = MakeField(T.CapOnline, Edge);
	PackageModel M(T);
	ss::Rng R(Seed);
	for (int P = 0; P < Periods; ++P)
	{
		for (int E = 0; E < T.Volume; ++E)
		{
			M.Event(T.CapOnline, PrizeOf(F, R, T.CapOnline), Early);
		}
		M.PeriodEnd();
	}
	return static_cast<double>(M.Net()) / static_cast<double>(T.CapOnline) / (static_cast<double>(Periods) / 12.0);
}

/**
 * The same draws through the code (FundFor, OnlineEntered, OnlineSettle, Step's period ends, Chop when asked) and the
 * model: the player's chops and the backer's money agree to the cent. A cut-loose starts a fresh package, as the model does.
 */
void CodeMatchesModel(double Edge, int Periods, bool Early, const char* Seed, const char* What)
{
	const stake::Terms T = stake::LanternRowTerms(1);
	const Field F = MakeField(T.CapOnline, Edge);
	PackageModel M(T);
	life::State L;
	Chips Bank = 0;
	const double T0 = 10.0 * Day;
	int Made = 0;
	auto NewDeal = [&L, &Made](double At) {
		stake::Deal D = PackageDeal(1, At);
		D.Id = "deal:lanternrow:mc:" + std::to_string(Made++);
		L.Stakes.Deals.push_back(D);
	};
	NewDeal(T0);
	ss::Rng R(Seed);
	Chips Prizes = 0;
	Chips BuyIns = 0;
	int Chops = 0;
	for (int P = 0; P < Periods; ++P)
	{
		const double Start = T0 + P * 30.0 * Day;
		for (int E = 0; E < T.Volume; ++E)
		{
			const Chips Prize = PrizeOf(F, R, T.CapOnline);
			const double At = Start + 60.0 + E * 340.0;
			OnlineEvent(L.Stakes, Bank, L, "mc-" + std::to_string(P) + "-" + std::to_string(E), T.CapOnline, Prize, At);
			const stake::Deal* D = stake::ActivePackage(L.Stakes);
			if (Early && D && D->RollNow - D->Funded >= 20 * T.CapOnline)
			{
				Chops += stake::Chop(L.Stakes, Bank, L, D->Id, At + 61.0).empty() ? 1 : 0;
			}
			M.Event(T.CapOnline, Prize, Early);
			Prizes += Prize;
			BuyIns += T.CapOnline;
		}
		std::vector<std::pair<std::string, std::string>> Tx;
		stake::Step(L.Stakes, Bank, L, Start, Start + 30.0 * Day + 1.0, Tx);
		M.PeriodEnd();
		if (!stake::ActivePackage(L.Stakes))
		{
			NewDeal(Start + 30.0 * Day + 1.0);
		}
	}
	// Everything the roll ever held is the backer's or the player's: prizes less buy-ins, split.
	const Chips CodeBacker = Prizes - BuyIns - Bank;
	Expect(Bank == M.Horse && CodeBacker == M.Net() && Made - 1 == M.CutLoose, std::string("backer EV: the code runs the model's package, cent for cent (") + What + ")");
	std::printf("  code vs model (%s): the player's chops %s, the backer %s, %d cut loose%s\n", What, ss::Money(Bank).c_str(), ss::Money(CodeBacker).c_str(), M.CutLoose,
		Early ? (", " + std::to_string(Chops) + " early chops").c_str() : "");
}

void BackerEv()
{
	// The model is the code: the same draws through both, cent for cent (a winner, a winner chopping early every time he
	// can, and a loser who gets cut loose).
	CodeMatchesModel(0.05, 36, false, "mc-code-a", "+5%, monthly chops");
	CodeMatchesModel(0.05, 36, true, "mc-code-b", "+5%, every early chop");
	CodeMatchesModel(-0.30, 12, false, "mc-code-c", "-30%, cut loose");
	// Each Lantern Row tier at its floor's shrunk ROI as the true edge (§2.7): never negative-EV for the backer, even
	// when the horse takes every early chop he's allowed. v1's +2% at 40% is kept as the negative control.
	struct Tier
	{
		const char* Name;
		double Edge;
		int Tier;
		int Cut;
		bool Positive;
		double Spec; // the review's Monte Carlo (§2.7), buy-ins a year
	};
	const Tier Tiers[] = {
		{"+5% at 35%", 0.05, 1, 350, true, 14.0},
		{"+8% at 45%", 0.08, 2, 450, true, 29.0},
		{"+15% at 50%", 0.15, 3, 500, true, 67.0},
		{"v1: +2% at 40%", 0.02, 1, 400, false, -24.0},
	};
	for (const Tier& T : Tiers)
	{
		stake::Terms Terms = stake::LanternRowTerms(T.Tier);
		Terms.CutPermille = T.Cut;
		const double Ev = BackerEv(Terms, T.Edge, 5000, false, "lantern-ev");
		const double Greedy = BackerEv(Terms, T.Edge, 5000, true, "lantern-ev");
		std::printf("  backer EV %s: %+.1f buy-ins a year (the spec's sim %+.0f); %+.1f when the horse chops early every time\n", T.Name, Ev, T.Spec, Greedy);
		Expect(T.Positive ? Ev > 0.0 : Ev < 0.0, std::string("backer EV: ") + T.Name + (T.Positive ? " is positive" : " loses (the control)"));
		Expect(std::fabs(Ev - T.Spec) <= 0.4 * std::fabs(T.Spec), std::string("backer EV: ") + T.Name + " lands near the spec's sim");
		Expect(!T.Positive || Greedy > 0.0, std::string("backer EV: ") + T.Name + " stays positive with every early chop");
	}
	// The policy's own numbers: each tier's floor, cut, cap and roll (§2.7).
	Expect(stake::LanternRowTier(0.049) == 0 && stake::LanternRowTier(0.05) == 1 && stake::LanternRowTier(0.08) == 2 && stake::LanternRowTier(0.15) == 3, "backer EV: the tiers' floors");
	Expect(stake::LanternRowTerms(1).CutPermille == 350 && stake::LanternRowTerms(2).CapOnline == 2200 && stake::LanternRowTerms(3).Roll == 550000 &&
			   stake::LanternRowTerms(3).Volume == 100,
		"backer EV: the tiers' terms");

	// Lou's stake to the player (§1.11, Q34): an 88-runner Nightly, 13 paid, no makeup: about $50 at 0% ROI, $60 at +20%.
	for (const double Edge : {0.0, 0.20})
	{
		const Field N = MakeField(12000, Edge, 88, 13);
		double Mine = 0.0;
		double Own = 0.0;
		for (int K = 1; K <= N.Paid; ++K)
		{
			const double P = std::pow(static_cast<double>(K) / N.N, N.A) - std::pow(static_cast<double>(K - 1) / N.N, N.A);
			const double Prize = N.Pay[static_cast<size_t>(K - 1)] * 120.0;
			Mine += P * (Prize > 120.0 ? (Prize - 120.0) / 2.0 : 0.0);
			Own += P * Prize;
		}
		Own -= 120.0;
		std::printf("  Lou's stake at %+.0f%%: the player's EV $%.2f staked (no money of theirs at risk), $%.2f on their own\n", Edge * 100.0, Mine, Own);
		Expect(Mine > 40.0 && Mine < 70.0, "Lou's stake hands the player about $50 to $60 of EV");
		Expect(Mine * stake::LouMonthCap <= 300.0, "Lou's stakes, one a week and four a month, move at most $300 of EV in any 30 days");
	}
}

// ------------------------------------------------------------------ 11. save and migration, 12. coaching

void SaveAndMigration()
{
	// A busy book: every line kind, round-tripped byte for byte.
	life::State L;
	Chips Bank = 77700;
	const live::Occurrence O = live::FindOccurrence("embercrest-nightly@30");
	stake::Accept(L.Stakes, Bank, L, LouOffer(O, O.DeskOpens), O.DeskOpens);
	live::Register(Bank, L, O, O.DeskOpens + 5.0, 88, {{"Big Lou", 2}}, stake::FundFor(L.Stakes, O.Id, 12000, true, O.DeskOpens + 5.0));
	const live::Occurrence Sun = live::FindOccurrence("embercrest@34");
	L.Stakes.Listings.push_back(Manual(Sun, 1050, {{"Mrs. Park", 100}, {"", 50}}, Sun.Start - Day));
	stake::Swap S;
	S.EntryId = Sun.Id;
	S.Partner = "Mei";
	S.Permille = 50;
	S.Closes = Sun.LateRegEnds;
	L.Stakes.Swaps.push_back(S);
	L.Stakes.Deals.push_back(PackageDeal(1, 20.0 * Day));
	OnlineEvent(L.Stakes, Bank, L, "save-ev", 1100, 0, 21.0 * Day);
	stake::Offer Off = LouOffer(Sun, Sun.Start - 2.0 * Day);
	Off.Id = "test:offer\twith tab";
	Off.Line = "A line with a tab\tand a newline\nand a \\ backslash.";
	L.Stakes.Offers.push_back(Off);
	stake::ToBurner(L.Stakes, 22.0 * Day, "need my two sessions this week. or ur cut drops.");
	stake::NoteConduct(L.Stakes, stake::MarkPickedUp, 23.0 * Day);
	L.Stakes.NetOnYou["Big Lou"] = -24000;
	L.Stakes.PausedUntil["lanternrow"] = 40.0 * Day;
	ss::DecisionGrade G;
	G.LossBB = 0.6;
	G.Result = ss::Grade::Mistake;
	coach::Add(L.Study, coach::Spot::Bubble, G);
	L.Study.Worsts[static_cast<size_t>(coach::Spot::Bubble)].LossMilliBb = 600;
	L.Study.Worsts[static_cast<size_t>(coach::Spot::Bubble)].Hand = "A-T off, 22.0 big blinds";
	L.Study.TiltFactor = 0.95;
	const std::string Text = Save(L, Bank);
	life::State Back;
	Chips BankBack = 0;
	Expect(Load(Text, Back, BankBack) && Save(Back, BankBack) == Text, "save: every stake and coach line round-trips byte for byte");
	Expect(Back.Stakes.Offers.size() == 1 && Back.Stakes.Offers[0].Id == Off.Id && Back.Stakes.Offers[0].Line == Off.Line, "save: escapes survive");
	Expect(Back.Stakes.Deals.size() == 2 && Back.Stakes.Deals[1].Lines.size() == L.Stakes.Deals[1].Lines.size() && Back.Stakes.Listings[0].Plan == L.Stakes.Listings[0].Plan,
		"save: deals, their lines and the plans");
	Expect(Back.Study.All[static_cast<size_t>(coach::Spot::Bubble)].Decisions == 1 && std::fabs(Back.Study.TiltFactor - 0.95) < 1e-9, "save: the coach's notes");
	std::printf("  save: the stake and coach lines of a busy book: %zu bytes\n", StakeLines(Text).size());

	// Migration: an old save loads with an empty book and writes back as it was read.
	const char* Fixtures[] = {"pre_segment_fresh.ssave", "pre_segment_mid_nightly.ssave", "pre_segment_team_riverline.ssave", "pre_segment_evicted.ssave"};
	for (const char* Name : Fixtures)
	{
		const std::string Old = ReadFile(std::string(SS_TESTS_DIR) + "/fixtures/" + Name);
		ss::SaveData D;
		Expect(ss::SaveData::Parse(Old, D), std::string("migration: ") + Name + " parses");
		Expect(D.Life.Stakes.Deals.empty() && D.Life.Stakes.Offers.empty() && D.Life.Stakes.Owes.empty() && D.Life.Stakes.Record.Seeded, std::string("migration: ") + Name + ": an empty book, the record seeded");
		const std::string Again = D.Serialize();
		ss::SaveData D2;
		Expect(ss::SaveData::Parse(Again, D2) && D2.Serialize() == Again, std::string("migration: ") + Name + " loads a second time the same");
		Expect(StakeLines(Again).empty(), std::string("migration: ") + Name + ": nothing of the stake's is written on load (the life keeps its own record)");
	}
	// A session that never reloads keeps the same book as one that did: an evicted, picked-up career writes back as read.
	{
		ss::SaveData D;
		ss::SaveData::Parse(ReadFile(std::string(SS_TESTS_DIR) + "/fixtures/pre_segment_evicted.ssave"), D);
		D.Life.Busts = 2;
		D.Life.Record(D.Life.EvictedAt - 100.0, "Fine (picked up)", -20000, 2);
		const std::string Once = D.Serialize();
		ss::SaveData D2;
		Expect(ss::SaveData::Parse(Once, D2) && D2.Serialize() == Once && D2.Life.Evictions > 0, "migration: the record's past is the life's; the save round-trips");
		// A pickup from here on goes on the record, and is written.
		stake::NoteConduct(D2.Life.Stakes, stake::MarkPickedUp, D2.Life.EvictedAt + 10.0 * Day);
		const std::string Twice = D2.Serialize();
		ss::SaveData D3;
		Expect(StakeLines(Twice).find("stake\tconduct\t") != std::string::npos && ss::SaveData::Parse(Twice, D3) && D3.Serialize() == Twice, "migration: new conduct is kept");
	}
}

void Coaching()
{
	coach::Notes N;
	Expect(coach::ReviewOf(N, "FoldEquityFred").Body.find("Play me two hundred hands") == 0, "coaching: an empty review says so honestly");
	ss::Rng R("coach");
	for (int K = 0; K < 240; ++K)
	{
		ss::DecisionGrade G;
		const coach::Spot S = static_cast<coach::Spot>(K % 12);
		G.LossBB = S == coach::Spot::Bubble ? 0.6 : S == coach::Spot::RiverCall ? 0.35 : 0.05 * R.Next();
		G.Result = G.LossBB > 0.5 ? ss::Grade::Mistake : ss::Grade::Good;
		coach::Add(N, S, G);
	}
	const coach::Review Rv = coach::ReviewOf(N, "FoldEquityFred");
	Expect(Rv.Enough && Rv.Worst == coach::Spot::Bubble && Rv.Next == coach::Spot::RiverCall && Rv.Counts[0] == 20 && std::fabs(Rv.BbPer[0] - 0.6) < 0.001,
		"coaching: the review names the two leaks by bb lost per decision");
	Expect(Rv.Body.find("Bubble calls: 20 decisions, -0.6 bb each.") == 0 && Rv.Body.size() <= 600, "coaching: the review in Fred's words, under 600 characters");
	coach::Reviewed(N, Rv, 50.0 * Day);
	Expect(N.Homework == static_cast<int>(coach::Spot::Bubble) && N.SinceReview[static_cast<size_t>(coach::Spot::Bubble)].Decisions == 0 &&
			   N.All[static_cast<size_t>(coach::Spot::Bubble)].Decisions == 20,
		"coaching: the tallies start over, the homework kept");
	for (int K = 0; K < 240; ++K)
	{
		ss::DecisionGrade G;
		G.LossBB = K % 12 == static_cast<int>(coach::Spot::Bubble) ? 0.2 : 0.05;
		coach::Add(N, static_cast<coach::Spot>(K % 12), G);
	}
	Expect(coach::ReviewOf(N, "FoldEquityFred").Body.find("Last month's homework, bubble calls: 0.6 bb a decision then, 0.2 now. That's the work paying.") != std::string::npos,
		"coaching: the next review checks the homework");
	// Composure: x0.95 a session, never under x0.80, fading back 2.5% a month without one.
	coach::Notes T;
	T.TiltFactor = 0.5;
	T.TiltSince = 10.0 * Day;
	Expect(std::fabs(coach::TiltGainNow(T, 1.2, 10.0 * Day) - 0.96) < 1e-9, "coaching: never below x0.80 of the background's");
	T.TiltFactor = 0.9;
	Expect(std::fabs(coach::TiltGainNow(T, 1.0, 10.0 * Day + 65.0 * Day) - 0.95) < 1e-9, "coaching: fading back 2.5% every 30 days");
	Expect(coach::TiltGainNow(coach::Notes(), 1.3, 0.0) == 1.3 && coach::ReadWeightNow(coach::Notes(), 2, 0.0) == 2, "coaching: no coach, the background's own");
	coach::Notes D;
	D.ReadsUntil = 7.0 * Day;
	Expect(coach::ReadWeightNow(D, 2, 6.0 * Day) == 3 && coach::ReadWeightNow(D, 2, 7.0 * Day) == 2, "coaching: a read counts one more for the week");
}

// ------------------------------------------------------------------ 13. exploits

void Exploits()
{
	const live::Occurrence N1 = live::FindOccurrence("embercrest-nightly@9");
	// A listing that lapses moves no money.
	{
		life::State L;
		Chips Bank = 10000;
		L.Stakes.Listings.push_back(Manual(N1, 1050, {{"Mrs. Park", 100}, {"", 200}}, N1.Start - Day));
		const size_t Lines = L.Ledger.size();
		std::vector<std::pair<std::string, std::string>> Tx;
		stake::Step(L.Stakes, Bank, L, N1.Start - Day, N1.LateRegEnds + 1.0, Tx);
		Expect(L.Stakes.Listings.empty() && Bank == 10000 && L.Ledger.size() == Lines, "exploit: a listing you never play lapses with nothing in the ledger");
	}
	// Pieces and swaps share 70%.
	{
		stake::Book B;
		const ss::rep::Public P = Record(0, 0.0, 0, 20, 0.6, 0.0);
		stake::View V = ViewFor(P, N1.Start - Day);
		Expect(stake::List(B, V, N1.Id, 700, 1000, V.World, 0.0).empty() && !stake::List(B, V, N1.Id, 50, 1000, V.World, 0.0).empty(), "exploit: 70% and no more");
		Expect(!stake::List(B, V, N1.Id, 30, 1000, V.World, 0.0).empty(), "exploit: live pieces in fives");
	}
	// Lou: one stake a week; none below -$360.
	{
		life::State L;
		Chips Bank = 100000;
		Expect(stake::Accept(L.Stakes, Bank, L, LouOffer(N1, N1.DeskOpens), N1.DeskOpens).empty(), "exploit: Lou's first stake this week");
		const live::Occurrence N2 = live::FindOccurrence("embercrest-nightly@11");
		Expect(stake::Accept(L.Stakes, Bank, L, LouOffer(N2, N1.DeskOpens + Day), N1.DeskOpens + Day) == "One a week, kid. I've got a mortgage too.",
			"exploit: Lou's second stake in a week is refused, with his line");
		life::State Lp;
		Chips Bp = 100000;
		double At = 50.0 * Day;
		for (int K = 0; K < 3; ++K)
		{
			const live::Occurrence O = live::FindOccurrence("embercrest-nightly@" + std::to_string(51 + 7 * K));
			stake::Accept(Lp.Stakes, Bp, Lp, LouOffer(O, O.DeskOpens), O.DeskOpens);
			live::Register(Bp, Lp, O, O.DeskOpens + 5.0, 88, {}, stake::FundFor(Lp.Stakes, O.Id, 12000, true, O.DeskOpens + 5.0));
			live::Settle(Bp, Lp, O.Id, 60, 88, 0, O.Start + 120.0);
			At = O.Start + 200.0;
		}
		Expect(Lp.Stakes.NetOnYou["Big Lou"] == -36000 && Lp.Stakes.PausedUntil["Big Lou"] > At + 29.0 * Day, "exploit: three bricks (-$360) and Lou pauses 30 days");
		const live::Occurrence O4 = live::FindOccurrence("embercrest-nightly@73");
		Expect(stake::Accept(Lp.Stakes, Bp, Lp, LouOffer(O4, O4.DeskOpens), O4.DeskOpens) == "Let's give it a month, kid. Nothing personal. The numbers are the numbers.",
			"exploit: no stake while he pauses");
		std::vector<std::pair<std::string, std::string>> Tx;
		stake::Step(Lp.Stakes, Bp, Lp, At, At + 10.0, Tx);
		bool Said = false;
		for (const auto& T : Tx)
		{
			Said = Said || (T.first == "Big Lou" && T.second.find("Let's give it a month") != std::string::npos);
		}
		Expect(Said, "exploit: he says so");
		// A cash in an event where he holds a piece lifts his net back above -$360: the pause ends.
		const live::Occurrence Sun = live::FindOccurrence("embercrest@69");
		Lp.Stakes.Listings.push_back(Manual(Sun, 1000, {{"Big Lou", 200}}, Sun.Start - Day));
		live::Register(Bp, Lp, Sun, Sun.DeskOpens + 5.0, 100, {}, stake::FundFor(Lp.Stakes, Sun.Id, 15000, true, Sun.DeskOpens + 5.0));
		live::Settle(Bp, Lp, Sun.Id, 1, 100, 400000, Sun.Start + 500.0);
		Expect(Lp.Stakes.NetOnYou["Big Lou"] > -36000 && Lp.Stakes.PausedUntil["Big Lou"] <= Sun.Start + 500.0, "exploit: a cash that lifts his net ends the pause");
	}
	// A refunded listing repays markup once (above) and a lapsed event stake moves nothing.
	{
		life::State L;
		Chips Bank = 10000;
		stake::Accept(L.Stakes, Bank, L, LouOffer(N1, N1.DeskOpens), N1.DeskOpens);
		std::vector<std::pair<std::string, std::string>> Tx;
		stake::Step(L.Stakes, Bank, L, N1.DeskOpens, N1.LateRegEnds + 1.0, Tx);
		Expect(L.Stakes.Deals[0].State == stake::DealLapsed && Bank == 10000 && L.Ledger.empty(), "exploit: a stake you don't play lapses; nobody's money moved");
		Expect(stake::FundFor(L.Stakes, N1.Id, 12000, true, N1.LateRegEnds + 2.0).From == life::Paid::Own, "exploit: a lapsed stake pays for nothing");
	}
	// Walking away, and asking again inside a year: refused.
	{
		life::State L;
		Chips Bank = 0;
		L.Stakes.Deals.push_back(PackageDeal(1, 10.0 * Day));
		for (int K = 0; K < 20; ++K)
		{
			OnlineEvent(L.Stakes, Bank, L, "w-" + std::to_string(K), 1100, 0, 10.0 * Day + 60.0 + K * 100.0);
		}
		Expect(stake::WalkAway(L.Stakes, Bank, L, "deal:lanternrow:test", 12.0 * Day).empty() && Bank == 0 && stake::YouOwe(L.Stakes) == 0, "exploit: walking away from makeup owes Lantern Row nothing");
		stake::Offer Again;
		Again.Id = "lanternrow:again";
		Again.K = stake::Kind::Package;
		Again.By = stake::Outfit::LanternRow;
		Again.Backer = "Lantern Row Staking";
		Again.T = stake::LanternRowTerms(1);
		Expect(stake::Accept(L.Stakes, Bank, L, Again, 100.0 * Day) == "Lantern Row doesn't take back people who walk.", "exploit: walking away and re-applying inside a year is refused");
		ss::rep::Public P = Record(900, 0.40, 1100, 0, 0.0, 0.0);
		P.Walked365 = true;
		stake::View V = ViewFor(P, 100.0 * Day + 12.0 * Hour);
		bool Offered = false;
		for (const stake::Offer& O : stake::OffersFor(V, L.Stakes, V.World))
		{
			Offered = Offered || O.By == stake::Outfit::LanternRow;
		}
		Expect(!Offered, "exploit: and they don't offer");
	}
	// Quitting the stable on an up-swing chops and ends clean, and the door stays shut a month: no quit-and-resign at
	// every peak.
	{
		life::State L;
		Chips Bank = 0;
		L.Stakes.Deals.push_back(PackageDeal(1, 10.0 * Day));
		OnlineEvent(L.Stakes, Bank, L, "up-1", 1100, 50000, 10.0 * Day + 60.0);
		Expect(stake::WalkAway(L.Stakes, Bank, L, "deal:lanternrow:test", 11.0 * Day).empty() && Bank == 17115, "exploit: walking away up is a final chop");
		bool Closed = false;
		for (const life::Letter& M : L.Inbox)
		{
			Closed = Closed || M.Subject == "Package closed";
		}
		ss::rep::Public P = Record(900, 0.40, 1100, 0, 0.0, 0.0);
		stake::View V = ViewFor(P, 12.0 * Day + 12.0 * Hour);
		bool Offered = false;
		for (const stake::Offer& O : stake::OffersFor(V, L.Stakes, V.World))
		{
			Offered = Offered || O.By == stake::Outfit::LanternRow;
		}
		Expect(Closed && !Offered && L.Stakes.PausedUntil["lanternrow"] >= 41.0 * Day, "exploit: and Lantern Row's door is shut for a month");
	}
	// Re-entries past the package's limit are the player's own money (Packages). A freeroll is nobody's business but the
	// player's: no pieces of it, no roll for it, no shot on the record (the public record leaves it out too: rep::Of, S9).
	{
		ss::net::EventInstance Free;
		bool Found = false;
		for (const ss::net::EventInstance& I : ss::net::Shared().Window(2.0 * Day, 4.0 * Day))
		{
			if (!Found && ss::net::Shared().TemplateOf(I).BuyInCents == 0)
			{
				Free = I;
				Found = true;
			}
		}
		const ss::rep::Public A = Record(500, 0.30, 1100, 0, 0.0, 0.0);
		stake::Book Fb;
		Expect(Found && stake::List(Fb, ViewFor(A, 2.0 * Day), Free.Id, 100, 1000, 2.0 * Day, 0.0) == "Nobody buys a piece of a freeroll." && Fb.Listings.empty(),
			"exploit: nobody buys a piece of a freeroll");
		life::State Lf;
		Chips Bf = 100;
		stake::OnlineEntered(Lf.Stakes, Bf, Lf, Free.Id, "Midnight Freeroll", 0, life::Funding(), 100, 0.0, 2.0 * Day);
		Expect(Lf.Stakes.Record.Shots == 0 && Lf.Stakes.Record.Marks.empty() && Bf == 100, "exploit: a freeroll is never a shot");
		life::State L;
		Chips Bank = 1000;
		L.Stakes.Deals.push_back(PackageDeal(1, 0.0));
		OnlineEvent(L.Stakes, Bank, L, "freeroll", 0, 5000, Day);
		Expect(L.Stakes.Deals[0].RollNow == L.Stakes.Deals[0].T.Roll && L.Stakes.Deals[0].Events == 0 && Bank == 6000, "exploit: a freeroll isn't the package's, and its prize is the player's");
	}
	// A covered entry goes in clean: with pieces sold on the event, the sponsor's option says why and the desk refuses the
	// cover (live::CanRegister); and an entry that comes in covered anyway lapses the listing, nothing collected.
	{
		life::State L;
		Chips Bank = 100000;
		L.Stakes.Listings.push_back(Manual(N1, 1000, {{"", 100}}, N1.Start - Day));
		ss::sponsor::Contract Team;
		Team.Id = "c:embercrest:clean";
		Team.Brand = "embercrest";
		Team.St = ss::sponsor::State::Active;
		Team.Since = N1.Start - 10.0 * Day;
		Team.Until = N1.Start + 20.0 * Day;
		Team.T.CoveredEntries = 2;
		Team.T.CoveredCapCents = 12000;
		Team.T.CoversFares = true;
		L.Sponsors.Contracts.push_back(Team);
		const double Desk = N1.DeskOpens + 5.0;
		const life::Funding Cover = ss::sponsor::CoverFunding(Team, N1);
		const std::string Clean = "Covered entries go in clean. You've sold pieces of this one.";
		const std::vector<std::pair<life::Funding, std::string>> Opts = stake::FundingOptions(L, N1, Desk);
		const bool Listed = std::any_of(Opts.begin(), Opts.end(), [&Clean](const std::pair<life::Funding, std::string>& X) { return X.first.From == life::Paid::Sponsor && X.second == Clean; });
		Expect(Listed && live::Register(Bank, L, N1, Desk, 88, {}, Cover) == Clean && Bank == 100000 && L.LiveEntries.empty() && L.Stakes.Listings.size() == 1,
			"exploit: covered entries go in clean (the desk refuses a cover on an event with pieces sold)");
		life::LiveEntry E;
		E.Id = N1.Id;
		E.Name = N1.T->Name;
		E.PaidCents = N1.T->BuyInCents;
		E.FeeCents = N1.T->FeeCents;
		E.State = life::LiveEntry::Registered;
		E.Fund = Cover;
		stake::OnRegister(L.Stakes, Bank, L, E, Desk);
		Expect(L.Stakes.Listings.empty() && L.Stakes.Pieces.empty() && FindLine(L, "Pieces sold") == nullptr && Bank == 100000, "exploit: a covered entry lapses the listing, nothing collected");
	}
}

// ------------------------------------------------------------------ the policy's order, the rail, the marks, the offers

void Policy()
{
	const live::Occurrence N1 = live::FindOccurrence("embercrest-nightly@9");
	life::State L;
	Expect(stake::FundingOptions(L, N1, N1.DeskOpens).size() == 1 && life::IsOwnMoney(stake::FundingOptions(L, N1, N1.DeskOpens)[0].first), "policy: nobody but the player");
	// Lou's standing offer for the event is a choice at the desk; registering on it is the handshake.
	stake::Offer Of = LouOffer(N1, N1.DeskOpens);
	L.Stakes.Offers.push_back(Of);
	const std::vector<std::pair<life::Funding, std::string>> Opts = stake::FundingOptions(L, N1, N1.DeskOpens + 1.0);
	Expect(Opts.size() == 2 && Opts[0].first.From == life::Paid::Backer && Opts[0].first.Ref == Of.Id && Opts[0].second.empty(), "policy: Lou'll put you in");
	Chips Bank = 1000;
	Expect(live::Register(Bank, L, N1, N1.DeskOpens + 2.0, 88, {{"Big Lou", 2}}, Opts[0].first).empty() && Bank == 1000 && L.Stakes.Offers.empty() &&
			   L.Stakes.Deals.size() == 1 && L.Stakes.Deals[0].State == stake::DealPlaying,
		"policy: registering on the standing offer takes it");
	// The rail: Lou is in the field (his roster line), so he stands at the rail only once he's out.
	const std::vector<std::pair<std::string, int>> In = stake::RailFor(L.Stakes, N1.Id, {});
	const std::vector<std::pair<std::string, int>> Out = stake::RailFor(L.Stakes, N1.Id, {"Big Lou"});
	Expect(In.empty() && Out.size() == 1 && Out[0].first == "Big Lou" && Out[0].second == 1000, "rail: a backer still in the field plays straight; out, he's at the rail");
	// The cage splits a night registered on the handshake like any other stake: 13th for $236 is Lou's $178 and your $58
	// (the entry keeps the offer's id; the deal it became is the one that settles).
	live::Settle(Bank, L, N1.Id, 13, 88, 23600, N1.Start + 300.0);
	const life::LedgerEntry* Share = FindLine(L, "Big Lou: his share");
	Expect(Bank == 1000 + 5800 && Share && Share->Amount == -17800 && L.Stakes.Deals[0].State == stake::DealSettled && L.Stakes.NetOnYou["Big Lou"] == 5800,
		"policy: the handshake's night settles: Lou's $178, your $58");
	// A night on the handshake that's called off gives Lou his $120 back, and nothing else moves.
	{
		life::State Lr;
		Chips Br = 1000;
		Lr.Stakes.Offers.push_back(LouOffer(N1, N1.DeskOpens));
		const std::vector<std::pair<life::Funding, std::string>> Or = stake::FundingOptions(Lr, N1, N1.DeskOpens + 1.0);
		live::Register(Br, Lr, N1, N1.DeskOpens + 2.0, 88, {}, Or[0].first);
		const life::LedgerEntry* Back = nullptr;
		Expect(live::Refund(Br, Lr, N1.Id, N1.Start, "not enough players") && Br == 1000 && (Back = FindLine(Lr, "Big Lou: stake back")) != nullptr && Back->Amount == -12000 &&
				   Lr.Stakes.Deals[0].State == stake::DealRefunded,
			"policy: a cancelled night on the handshake gives Lou his $120 back, once");
	}
	// Lou's stake accepted, and the player goes in on their own money anyway: the stake is off, quietly (no "missed you"
	// while they're sitting in the Nightly), and only their own buy-in moved.
	{
		life::State Lo;
		Chips Bo = 50000;
		stake::Accept(Lo.Stakes, Bo, Lo, LouOffer(N1, N1.DeskOpens), N1.DeskOpens);
		live::Register(Bo, Lo, N1, N1.DeskOpens + 5.0, 88, {}, life::Funding());
		std::vector<std::pair<std::string, std::string>> Tx;
		stake::Step(Lo.Stakes, Bo, Lo, N1.DeskOpens + 5.0, N1.LateRegEnds + 10.0, Tx);
		Expect(Lo.Stakes.Deals[0].State == stake::DealLapsed && Tx.empty() && Bo == 50000 - 12000 && FindLine(Lo, "Big Lou") == nullptr,
			"policy: your own money at the desk calls Lou's stake off quietly");
	}
	Expect(stake::RailLine(L.Stakes, N1.Id, "Big Lou", static_cast<int>(ss::talk::Moment::Bubble), 0) == "Fold your way in, kid. I didn't come up here for the view.",
		"rail: Lou's bubble sweat");
	Expect(!stake::RailLine(stake::Book(), "", "Big Lou", static_cast<int>(ss::talk::Moment::Break), 3).empty() &&
			   stake::RailLine(stake::Book(), "", "Someone", static_cast<int>(ss::talk::Moment::Win), 0) == "Champion! Unbelievable!",
		"rail: lines without the book (the director has only names)");
	// At most two faces, the biggest share first.
	stake::Book Rail;
	for (const char* Who : {"Mrs. Park", "Sal", "Dre"})
	{
		stake::Piece P;
		P.EntryId = "embercrest@6";
		P.Buyer = Who;
		P.Permille = std::string(Who) == "Mrs. Park" ? 100 : 50;
		Rail.Pieces.push_back(P);
	}
	const std::vector<std::pair<std::string, int>> Two = stake::RailFor(Rail, "embercrest@6", {});
	Expect(Two.size() == 2 && Two[0].first == "Mrs. Park" && Two[1].first == "Dre", "rail: two faces at most, the biggest piece first");
	// The marks: payments due, chop days, a staked event.
	stake::Book M;
	stake::Owed W;
	W.Id = "owe:m";
	W.Who = "Mrs. Park";
	W.Cents = 11900;
	W.Due = 12.0 * Day;
	M.Owes.push_back(W);
	M.Deals.push_back(PackageDeal(2, 0.0));
	const std::vector<ss::life::Mark> Marks = stake::Marks(M, 0.0, 40.0 * Day);
	Expect(Marks.size() == 2 && Marks[0].Label == "$119.00 to Mrs. Park" && Marks[1].Label == "Chop day: Lantern Row Staking" && Marks[0].Kind == stake::DatePay &&
			   Marks[1].Kind == stake::DateChop,
		"marks: what's due and the chop day, in order, each by its kind (the calendar never reads their words)");
	// Money owed to the player: its own kind too.
	stake::Owed Y = W;
	Y.Id = "owe:y";
	Y.ToYou = true;
	M.Owes.push_back(Y);
	const std::vector<ss::life::Mark> More = stake::Marks(M, 0.0, 40.0 * Day);
	Expect(std::count_if(More.begin(), More.end(), [](const ss::life::Mark& K) { return K.Kind == stake::DateOwed && K.Label.find(" owes you ") != std::string::npos; }) == 1,
		"marks: money owed to the player is its own kind");
}

void Offers()
{
	// Lou offers once the player has cashed live and he knows them, in the daytime, for a Nightly a few hours to three
	// days out, at most once a week; Dee offers her ten percent of the first Sunday; Lantern Row at 500 events and +5%.
	const double Noon = 11.0 * Day + 12.0 * Hour; // Friday
	ss::rep::Public P = Record(0, 0.0, 0, 3, 0.4, 0.0);
	P.LiveCashes = 1;
	stake::View V = ViewFor(P, Noon);
	V.Regards["Big Lou"] = Knows(0.1f);
	V.Rolls["Big Lou"] = 1500000;
	stake::Book B;
	std::vector<stake::Offer> Made = stake::OffersFor(V, B, Noon);
	const stake::Offer* Lou = nullptr;
	const stake::Offer* Dee = nullptr;
	for (const stake::Offer& O : Made)
	{
		Lou = O.Backer == "Big Lou" ? &O : Lou;
		Dee = O.Backer == "Dee" ? &O : Dee;
	}
	Expect(Lou && Lou->K == stake::Kind::Event && Lou->BuyIn == 12000 && Lou->Line.find("I'm putting you in") != std::string::npos, "offers: Lou puts you in the Nightly");
	Expect(Lou && stake::TermsLine(*Lou) == "100% of the Embercrest Nightly $120 \xC2\xB7 buy-in back first, then 50/50 \xC2\xB7 nothing carried over \xC2\xB7 one a week",
		"offers: the card's terms in one line (§2.1)");
	Expect(Dee && Dee->K == stake::Kind::Pieces && Dee->Permille == 100 && Dee->EntryId.rfind("embercrest@", 0) == 0, "offers: Dee's ten percent of the first Sunday");
	Expect(stake::OffersFor(V, B, 11.0 * Day + 3.0 * Hour).empty(), "offers: nobody texts at three in the morning (but Sam)");
	{
		stake::View Played = V;
		Played.SundayPlayed = true;
		bool DeeAgain = false;
		for (const stake::Offer& O : stake::OffersFor(Played, B, Noon))
		{
			DeeAgain = DeeAgain || O.Backer == "Dee";
		}
		Expect(!DeeAgain, "offers: Dee's piece is of the first Sunday, not one after it");
	}
	// Lantern Row: 500 events at a shrunk +5% (+22% observed over 500 events at $11: they believe +5.0%).
	ss::rep::Public Grinder = Record(499, 0.22, 1100, 0, 0.0, 0.0);
	stake::View G = ViewFor(Grinder, Noon);
	bool Lr = false;
	for (const stake::Offer& O : stake::OffersFor(G, B, Noon))
	{
		Lr = Lr || O.By == stake::Outfit::LanternRow;
	}
	Expect(!Lr, "offers: not at 499 events");
	Grinder.OnlineEvents = 500;
	G = ViewFor(Grinder, Noon);
	const stake::Offer* Pkg = nullptr;
	Made = stake::OffersFor(G, B, Noon);
	for (const stake::Offer& O : Made)
	{
		Pkg = O.By == stake::Outfit::LanternRow ? &O : Pkg;
	}
	Expect(Pkg && Pkg->Tier == 1 && Pkg->T.CutPermille == 350 && Pkg->T.CapOnline == 1100, "offers: Lantern Row's first tier at 500 events and a shrunk +5%");
	if (Pkg)
	{
		life::State L;
		Chips Bank = 0;
		L.Stakes.Offers.push_back(*Pkg);
		std::vector<std::pair<std::string, std::string>> Tx;
		stake::Announce(L.Stakes, L, *Pkg, Noon, Tx);
		stake::Announce(L.Stakes, L, *Pkg, Noon, Tx);
		Expect(L.Inbox.size() == 1 && L.Inbox[0].Source == "stake" && L.Inbox[0].Kind == 20 && L.Inbox[0].Thread == Pkg->Id && Tx.empty(), "offers: Lantern Row mails, once");
		Expect(stake::Accept(L.Stakes, Bank, L, *Pkg, Noon + 1.0).empty() && stake::ActivePackage(L.Stakes) && stake::StakedRoll(L.Stakes) == 110000 && Bank == 0,
			"offers: the package taken: a $1,100 roll that isn't the bankroll");
	}
	// Sam: broke, rent due, has cashed, at three in the morning, on the Burner with the fine print.
	ss::rep::Public Broke = Record(50, -0.2, 330, 2, -0.5, 0.0);
	stake::View S = ViewFor(Broke, 20.0 * Day + 3.0 * Hour);
	S.Bankroll = 4000;
	S.Cashes = 2;
	S.RentDue = 107500;
	S.RentDeadline = 26.0 * Day;
	const stake::Offer* Sam = nullptr;
	Made = stake::OffersFor(S, B, S.World);
	for (const stake::Offer& O : Made)
	{
		Sam = O.By == stake::Outfit::Sam ? &O : Sam;
	}
	Expect(Sam && Sam->T.MakeupIsDebt && stake::TermsLine(*Sam).find("makeup is a debt if you quit") != std::string::npos, "offers: Sam spells out the debt before you sign");
	if (Sam)
	{
		life::State L;
		std::vector<std::pair<std::string, std::string>> Tx;
		stake::Announce(L.Stakes, L, *Sam, S.World, Tx);
		Expect(Tx.empty() && L.Stakes.Burner.size() == 2 && L.Stakes.Burner[1].second.find("25% of every cash and every shift") != std::string::npos,
			"offers: Sam keeps to the Burner, fine print and all");
	}
	{
		// Still paying off his last one: Sam doesn't offer another.
		stake::Book Owing;
		stake::Deal Old;
		Old.Id = "deal:sam:old";
		Old.K = stake::Kind::Package;
		Old.By = stake::Outfit::Sam;
		Old.Backer = "Sam";
		Old.State = stake::DealWalked;
		Old.Debt = 30000;
		Owing.Deals.push_back(Old);
		bool Again = false;
		for (const stake::Offer& O : stake::OffersFor(S, Owing, S.World))
		{
			Again = Again || O.By == stake::Outfit::Sam;
		}
		Expect(!Again, "offers: no new package from Sam while you owe him");
	}
}

void Copy()
{
	// Texts at most 200 characters; rail lines and break texts 84; letters 600 with 200 for subjects (§7.6).
	std::vector<std::pair<std::string, std::string>> Texts;
	std::vector<std::pair<std::string, std::string>> Rail;
	for (const std::pair<std::string, std::string>& L : stake::Inventory())
	{
		(L.first.find("/rail.") != std::string::npos || L.first.find("/break") != std::string::npos ? Rail : Texts).push_back(L);
	}
	const std::map<std::string, std::string> Vars{{"Pct", "10%"}, {"Cap", "$22"}, {"Count", "84"}, {"Volume", "120"}, {"Line", "We'll pass for now. Our door stays open in a week."}};
	Failures += copylint::Run("stake texts", Texts, copylint::Pa, Vars);
	Failures += copylint::Run("stake rail", Rail, copylint::TableTalk, Vars);
	std::vector<std::pair<std::string, std::string>> Subjects;
	std::vector<std::pair<std::string, std::string>> Bodies;
	for (const std::pair<std::string, std::string>& L : stake::LetterInventory())
	{
		(L.first.rfind("subject/", 0) == 0 ? Subjects : Bodies).push_back(L);
	}
	// A subject is a title, not a sentence: it gets a full stop for the lint's ending rule.
	for (std::pair<std::string, std::string>& S : Subjects)
	{
		S.second += ".";
	}
	Failures += copylint::Run("stake subjects", Subjects, copylint::MailSubject, Vars);
	Failures += copylint::Run("stake letters", Bodies, copylint::LetterBody, Vars);
	const std::map<std::string, std::string> CoachVars{{"Count", "31"}, {"Bb", "0.6"}, {"Spot", "bubble calls"}};
	Failures += copylint::Run("coaching", coach::Inventory(), copylint::LetterBody, CoachVars);
	// Every backer has the six kinds of line the track owes (§8.3): offer, sweat, result, reminder, late, welch.
	struct Need
	{
		const char* Who;
		const char* Kinds[6];
	};
	const Need Needs[] = {
		{"Big Lou", {"offer", "rail.bubble", "result.cash", "reminder", "late", "welch"}},
		{"Mrs. Park", {"piece.buy", "rail.bubble", "piece.owed", "reminder", "late", "welch"}},
		{"Sal", {"piece.buy", "rail.bubble", "piece.owed", "reminder", "late", "welch"}},
		{"Mei", {"offer", "rail.bubble", "swap.owe", "reminder", "late", "welch"}},
		{"Rick", {"offer", "rail.bubble", "swap.owed", "reminder", "late", "welch"}},
		{"Dee", {"offer", "break", "piece.owed", "reminder", "late", "welch"}},
	};
	for (const Need& N : Needs)
	{
		for (const char* K : N.Kinds)
		{
			bool Has = false;
			for (const std::pair<std::string, std::string>& L : stake::Inventory())
			{
				Has = Has || L.first == std::string(N.Who) + "/" + K;
			}
			Expect(Has, std::string("copy: ") + N.Who + " has a " + K + " line");
		}
	}
	for (const char* K : {"offer", "chop", "nochop", "reminder", "volume", "cut.makeup", "walked", "result.big"})
	{
		bool Has = false;
		for (const std::pair<std::string, std::string>& L : stake::LetterInventory())
		{
			Has = Has || L.first == std::string("body/") + K;
		}
		Expect(Has, std::string("copy: Lantern Row writes a ") + K + " letter");
	}
}
} // namespace stake_test

int main()
{
	using namespace stake_test;
	Failures += copylint::SelfCheck();
	EventStakes();
	Pieces();
	ParkGate();
	Swaps();
	Packages();
	Conservation();
	Idempotence();
	DueDates();
	Negotiation();
	Prices();
	BackerEv();
	SaveAndMigration();
	Coaching();
	Exploits();
	Policy();
	Offers();
	Copy();
	if (Failures == 0)
	{
		std::printf("staking tests: all passed\n");
	}
	return Failures == 0 ? 0 : 1;
}
