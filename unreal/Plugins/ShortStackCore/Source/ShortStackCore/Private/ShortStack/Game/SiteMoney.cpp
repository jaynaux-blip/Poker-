#include "SitesDetail.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Rep.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/World.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>

// The rival sites' money (update 2 §1.2, §5.4-§5.7): accounts and their wallets, the Cashier's one method each way, money in
// transit, the invitations, the welcome offers, the Second Run Club, RiverLine Rewards and the retention letter, the calendar's
// step, the rent card's warning, the marks, the Mail copy and the save lines.
//
// The rules, in short:
//   - RiverLine stays linked to the bank (U8). A rival's account is a wallet of its own, with its own lines (the last 40).
//   - A deposit leaves the bank at once: the amount as a transfer (kind 7) and the card fee (kind 14). A withdrawal leaves
//     the wallet at once and lands in the bank days later, net of its fee, as one transfer line; until then it's in neither.
//   - Buy-ins and prizes on a rival move the wallet in the session (RegisterEvent, ShowResults); OnBuyIn and OnPrize write its
//     lines and keep its record (rake, the month, the season). Site money never pays rent (N7, U11).
//   - Every payment is keyed: a transfer lands once (its State), a month closes once (its key moves on, and a done key).
// Copy is final and in the game's voice; Copy() hands the copy lint every sentence.

namespace ss
{
namespace site
{
namespace site_money_detail
{
constexpr double SiteDay = 1440.0;
constexpr double SiteHour = 60.0;
constexpr size_t SiteLinesKept = 40;
constexpr double SiteBonusDays = 60.0;
constexpr Chips SiteBonusMax = 10000;  // Ignite matches up to $100
constexpr Chips SiteBonusPiece = 500;  // released $5 at a time
constexpr Chips SiteBonusRake = 2000;  // for each $20 of rake
constexpr double SiteBoostDays = 60.0; // the retention letter's tier up
constexpr double SiteResendDays = 30.0;
constexpr double SiteKeepTransferDays = 30.0;
constexpr double SiteKeepDoneDays = 120.0;
constexpr Chips SiteTicketDeposit = 5000; // RunItTwice's tickets: a first deposit of $50 or more
constexpr double SiteRickAfterDays = 4.0; // word of mouth, a few nights after Ignite opens
constexpr double SiteRickWithinDays = 30.0;

/** The copy: every sentence, subject, title and label the sites write (§1.7's voice: companies mail, people text). */
struct SiteCopyLine
{
	const char* Key;
	const char* Text;
	bool Sentence; // linted as a sentence (ends in a stop); else a title, subject or label
};
const SiteCopyLine SiteCopyTable[] = {
	// Invitations (LetterInvite: Accept and Decline in Mail, §5.4).
	{"invite:ignite:subject", "Poker that's actually fun", false},
	{"invite:ignite",
		"Hi, it's Bree from Ignite Poker. We hear you play a lot of tournaments, so here's ours: Flip & Go every hour, the Spark Jackpot every night at 8 PM, and "
		"tables a lot friendlier than the ones you're used to. Open an account and we'll match your first deposit 100%, up to $100. The match comes off as you play, "
		"$5 for every $20 of rake inside 60 days. Accept below and your Cashier's ready.",
		true},
	{"invite2:ignite:subject", "Still a seat for you at Ignite", false},
	{"invite2:ignite",
		"Bree again. No pressure. The offer's still open: 100% of your first deposit, up to $100, released as you play. The Spark Jackpot runs every night at "
		"8 PM. Whenever you're ready, the button's below.",
		true},
	{"invite:runittwice:subject", "An invitation from RunItTwice", false},
	{"invite:runittwice",
		"This is Ingrid Vale at RunItTwice. {Why} We run tournaments for grown-ups: bigger guarantees, lower rake at the top, and fields that make you earn it. "
		"Make a first deposit of $50 or more and we'll put $33 in tickets on your account: a $22 Sunday Grind and two $5.50 Supper Stacks. Accept below and the "
		"Cashier's yours.",
		true},
	{"why:0", "Three hundred tournaments and counting, at eleven dollars or more on average, is a serious schedule, and we noticed.", true},
	{"why:1", "A series title gets noticed around here.", true},
	{"why:2", "We watch the streams, and yours is worth watching.", true},
	{"invite2:runittwice:subject", "The door's still open at RunItTwice", false},
	{"invite2:runittwice",
		"Ingrid again. The tickets are still on the table: $33 for a first deposit of $50 or more. The Sunday Grind goes off every Sunday at 5 PM. Accept "
		"whenever it suits you.",
		true},
	// Welcome letters, the morning an account opens (routine).
	{"welcome:ignite:subject", "You're in", false},
	{"welcome:ignite",
		"Welcome to Ignite. Your account's open. Deposits go in by card for a 3% fee, $10 minimum, and we match the first one up to $100. Withdrawals reach "
		"your bank in three to five days, $3 each. Have fun out there. That's the whole idea.",
		true},
	{"welcome:runittwice:subject", "Your RunItTwice account is open", false},
	{"welcome:runittwice",
		"Welcome to RunItTwice. Card deposits cost 2.5%, $10 minimum. Withdrawals reach your bank in 48 hours for a $5 fee. A first deposit of $50 or more brings your $33 "
		"in tickets, and the Second Run Club pays rakeback on the 1st once a month's rake passes $50. Good luck at the tables.",
		true},
	// The welcome offers.
	{"tickets:subject", "Your tickets are in", false},
	{"tickets",
		"Thanks for the deposit. A $22 Sunday Grind ticket and two $5.50 Supper Stack tickets are on your account. Register for either and the ticket goes in "
		"before your balance does.",
		true},
	{"bonusend:subject", "Your welcome bonus has ended", false},
	{"bonusend",
		"The 60 days on your Ignite welcome bonus are up. {Released} of it came off in play, and the {Left} still pending has expired. The tables aren't going "
		"anywhere.",
		true},
	// Rakeback, monthly (routine).
	{"secondrun:subject", "Second Run Club: {Month}", false},
	{"secondrun", "Your rake on RunItTwice in {Month} came to {Rake}. That puts you at {Tier}, {Rate} back: {Amount} is in your balance.", true},
	{"rewards:subject", "RiverLine Rewards: {Tier}, {Month}", false},
	{"rewards", "Your rake on RiverLine in {Month} came to {Rake}. That's {Tier}, {Rate} back: {Amount} is in your bank. Thanks for playing with us.", true},
	{"rewards:boost",
		"Your rake on RiverLine in {Month} came to {Rake}. With the bump from our last letter that's {Tier}, {Rate} back: {Amount} is in your bank. Thanks "
		"for sticking with us.",
		true},
	// RiverLine pushes back (§5.7): the retention letter, on the first deposit anywhere else.
	{"retention:subject", "We noticed", false},
	{"retention",
		"We saw some of your bankroll head to another site. No hard feelings, everybody shops around. But you've played with us since your first night, so for the next "
		"60 days your RiverLine Rewards sit one tier up, on us. The Night Shift's still the Night Shift.",
		true},
	// Word of mouth (people text, §1.7).
	{"rick:ignite", "ignite is a ATM rn. dont tell anyone.", true},
	// The Cashier's answers.
	{"err:riverline", "RiverLine's money is your bank's. There's nothing to move.", true},
	{"err:site", "That site isn't taking players yet.", true},
	{"err:account", "Open an account with {Site} first.", true},
	{"err:method:in", "{Site} doesn't take deposits that way yet.", true},
	{"err:method:out", "{Site} doesn't pay out that way.", true},
	{"err:min:in", "The smallest deposit is {Min}.", true},
	{"err:bank", "The bank can't cover {Amount} and the {Fee} fee.", true},
	{"err:min:out", "The smallest withdrawal is {Min}.", true},
	{"err:balance", "That's more than your {Site} balance.", true},
	{"err:reverse", "A withdrawal on its way can't be called back yet.", true},
	{"err:invite", "There's no invitation from {Site} to answer.", true},
	{"err:joined", "You already have an account with {Site}.", true},
	// The rent card (§5.5): the bank alone is short of the deadline, the bank and the sites aren't.
	{"rent", "Rent is due {Day}. {Money} Site money can't pay rent.", true},
	{"rent:transit", "{Amount} is on its way from {Site} ({Window})", false},
	{"rent:parked", "{Amount} sits on {Site}", false},
	// The rivals' News (§5.10).
	{"news:launch:ignite:title", "Ignite Poker is live", false},
	{"news:launch:ignite",
		"Flip & Go every hour, the Spark Jackpot every night at 8 PM, and a {Gtd} Sunday Blaze to close a launch week of guarantees half again as big.", true},
	{"news:launch:ignite:late", "Flip & Go every hour, the Spark Jackpot every night at 8 PM and the Sunday Blaze at 7 PM on Sundays.", true},
	{"news:launch:runittwice:title", "RunItTwice opens its new lobby", false},
	{"news:launch:runittwice", "Tournaments for grown-ups: lower rake at the top, the Sunday 100K every week, and a points season that starts tonight.", true},
	{"news:promo:ignite:title", "Ignite doubles your first deposit", false},
	{"news:promo:ignite", "A first deposit is matched 100%, up to $100, and the match comes off as you play: $5 for every $20 of rake inside 60 days.", true},
	{"news:promo:runittwice:title", "The Second Run Club pays on the 1st", false},
	{"news:promo:runittwice", "Rakeback every month: 15% once a month's rake passes $50, rising to 30% from $4,000.", true},
	{"news:overlay:title", "{Event} overlays by {Amount}", false},
	{"news:overlay", "{Entries} entries left the {Gtd} guarantee {Amount} short. {Site} covered the difference.", true},
	{"news:series:title", "{Series} starts {Day}", false},
	{"news:series", "{Events} events over {Days} days, {Gtd} guaranteed. The Main Event closes it at {Buyin}.", true},
	// Ledger and wallet labels, marks.
	{"label:deposit", "Deposit: {Site}", false},
	{"label:fee", "{Site} {Method} fee", false},
	{"label:withdrawal", "Withdrawal: {Site}", false},
	{"label:wallet:deposit", "Deposit", false},
	{"label:wallet:withdraw", "Withdrawal requested ({Fee} fee)", false},
	{"label:wallet:bonus", "Welcome bonus released", false},
	{"label:wallet:secondrun", "Second Run Club: {Month}", false},
	{"label:rewards", "RiverLine Rewards: {Tier}, {Month}", false},
	{"mark:transfer", "{Site} withdrawal {Amount} arrives", false},
	{"mark:rewards", "RiverLine Rewards", false},
	{"mark:bonus", "{Site} welcome bonus ends", false},
	{"mark:series", "{Series} starts", false},
};

const char* SiteText(const std::string& Key)
{
	for (const SiteCopyLine& C : SiteCopyTable)
	{
		if (Key == C.Key)
		{
			return C.Text;
		}
	}
	return "";
}

std::string SiteEscape(const std::string& In)
{
	std::string Out;
	for (const char Ch : In)
	{
		Out += Ch == '\\' ? std::string("\\\\") : Ch == '\n' ? std::string("\\n") : Ch == '\t' ? std::string("\\t") : std::string(1, Ch);
	}
	return Out;
}

std::string SiteUnescape(const std::string& In)
{
	std::string Out;
	for (size_t I = 0; I < In.size(); ++I)
	{
		if (In[I] == '\\' && I + 1 < In.size())
		{
			const char N = In[++I];
			Out.push_back(N == 'n' ? '\n' : N == 't' ? '\t' : N);
		}
		else
		{
			Out.push_back(In[I]);
		}
	}
	return Out;
}

/** A number as the shortest text that reads back to the same double (so a save round-trips byte for byte). */
std::string SiteNum(double X)
{
	if (X == 0.0)
	{
		return "0";
	}
	char Buf[40];
	for (int Precision = 15; Precision <= 17; ++Precision)
	{
		std::snprintf(Buf, sizeof(Buf), "%.*g", Precision, X);
		if (std::strtod(Buf, nullptr) == X)
		{
			break;
		}
	}
	return Buf;
}

double SiteToNum(const std::string& S)
{
	return S.empty() ? 0.0 : std::strtod(S.c_str(), nullptr);
}

Chips SiteToInt(const std::string& S)
{
	return S.empty() ? 0 : static_cast<Chips>(std::strtoll(S.c_str(), nullptr, 10));
}

const char* SiteName(net::SiteId S)
{
	return net::Def(S).Name.c_str();
}

/** A line in a rival's own ledger: the newest last, the last 40 kept. */
void SiteLineTo(Account& A, double At, int Kind, Chips Cents, const std::string& Label)
{
	Line L;
	L.At = At;
	L.Kind = Kind;
	L.Cents = Cents;
	L.Label = Label;
	A.Lines.push_back(L);
	if (A.Lines.size() > SiteLinesKept)
	{
		A.Lines.erase(A.Lines.begin(), A.Lines.begin() + static_cast<std::ptrdiff_t>(A.Lines.size() - SiteLinesKept));
	}
}

/** A done key, kept for good ("retention") or dated ("rewards:2026-10@57", SiteDated): either answers for Key. */
bool SiteDone(const Book& B, const std::string& Key)
{
	if (B.Done.count(Key) > 0)
	{
		return true;
	}
	const auto It = B.Done.lower_bound(Key + "@");
	return It != B.Done.end() && It->compare(0, Key.size() + 1, Key + "@") == 0;
}

/** A done key that can go 120 days after Day ("rewards:2026-10@1520"); keys without a day are kept for good. */
std::string SiteDated(const std::string& Key, double World)
{
	return Key + "@" + std::to_string(net::DayOf(World));
}

/** Rate as a letter says it: "8%", "5.5%". */
std::string SiteRate(int Permille)
{
	return Permille % 10 == 0 ? std::to_string(Permille / 10) + "%" : Fixed(static_cast<double>(Permille) / 10.0, 1) + "%";
}

/** Whether a day boundary was crossed from From to To (the gates are checked once a day). */
bool SiteNewDay(double From, double To)
{
	return net::DayOf(From) != net::DayOf(To);
}

/** A routine letter from a site, held to one a week a company (§1.7). Returns whether it went. */
bool SiteRoutine(life::State& L, net::SiteId S, double World, const std::string& Subject, const std::string& Body)
{
	if (WroteThisWeek(L, S, World))
	{
		return false;
	}
	PostLetter(L, S, World, LetterRoutine, Subject, Body);
	return true;
}

/** Closes one calendar month (its 1st is Payday) for RiverLine Rewards and every account: rakeback in, keys moved on. */
void SiteCloseMonth(Book& B, life::State& L, Chips& Bankroll, double Payday)
{
	const std::string Closing = MonthKeyAt(Payday - 1.0);
	const std::string Opening = MonthKeyAt(Payday);
	// RiverLine Rewards: the month's rake, its tier (one up while the retention letter's boost runs), paid into the bank as a
	// prize (kind 4) on the 1st. Bronze pays nothing and writes nothing.
	Rewards& R = B.RiverLine;
	if (!R.MonthKey.empty() && R.MonthKey <= Closing)
	{
		const std::vector<TierInfo>& Tiers = RewardsTiers();
		const int Earned = TierFor(Tiers, R.Rake);
		const bool Boosted = R.BoostUntil > 0.0 && Payday <= R.BoostUntil && Earned + 1 < static_cast<int>(Tiers.size());
		const int Tier = Boosted ? Earned + 1 : Earned;
		const Chips Pay = static_cast<Chips>(std::llround(static_cast<double>(R.Rake) * static_cast<double>(Tiers[static_cast<size_t>(Tier)].Permille) / 1000.0));
		const std::string Key = "rewards:" + R.MonthKey;
		if (Pay > 0 && !SiteDone(B, Key))
		{
			const std::vector<std::pair<std::string, std::string>> Vars = {{"Tier", Tiers[static_cast<size_t>(Tier)].Name}, {"Month", MonthName(R.MonthKey)},
				{"Rake", Money(R.Rake)}, {"Rate", SiteRate(Tiers[static_cast<size_t>(Tier)].Permille)}, {"Amount", Money(Pay)}};
			Bankroll += Pay;
			L.Record(Payday, FillCopy(SiteText("label:rewards"), Vars), Pay, 4);
			SiteRoutine(L, net::SiteId::RiverLine, Payday, FillCopy(SiteText("rewards:subject"), Vars), FillCopy(SiteText(Boosted ? "rewards:boost" : "rewards"), Vars));
			B.Done.insert(SiteDated(Key, Payday));
		}
		R.Tier = Tier;
		R.Rake = 0;
		R.MonthKey = Opening;
	}
	// An idle book keeps nothing: the boost spent, Bronze last month, no rake yet: no line until the next RiverLine buy-in.
	R.BoostUntil = R.BoostUntil > Payday ? R.BoostUntil : 0.0;
	if (R.Rake == 0 && R.Tier == 0 && R.BoostUntil == 0.0)
	{
		R.MonthKey.clear();
	}
	// Each account's month: RunItTwice's Second Run Club pays the closed month's rakeback into the wallet; Ignite's only rolls.
	for (Account& A : B.Accounts)
	{
		if (A.MonthKey.empty() || A.MonthKey > Closing)
		{
			A.MonthKey = A.MonthKey.empty() ? Opening : A.MonthKey;
			continue;
		}
		if (A.Site == net::SiteId::RunItTwice)
		{
			const std::vector<TierInfo>& Tiers = SecondRunTiers();
			const int Tier = TierFor(Tiers, A.RakeMonth);
			const Chips Pay = static_cast<Chips>(std::llround(static_cast<double>(A.RakeMonth) * static_cast<double>(Tiers[static_cast<size_t>(Tier)].Permille) / 1000.0));
			const std::string Key = "secondrun:" + A.MonthKey;
			if (Pay > 0 && !SiteDone(B, Key))
			{
				const std::vector<std::pair<std::string, std::string>> Vars = {{"Tier", Tiers[static_cast<size_t>(Tier)].Name}, {"Month", MonthName(A.MonthKey)},
					{"Rake", Money(A.RakeMonth)}, {"Rate", SiteRate(Tiers[static_cast<size_t>(Tier)].Permille)}, {"Amount", Money(Pay)}};
				A.Cents += Pay;
				SiteLineTo(A, Payday, 4, Pay, FillCopy(SiteText("label:wallet:secondrun"), Vars));
				SiteRoutine(L, A.Site, Payday, FillCopy(SiteText("secondrun:subject"), Vars), FillCopy(SiteText("secondrun"), Vars));
				B.Done.insert(SiteDated(Key, Payday));
			}
			A.Vip = Tier;
		}
		A.RakeMonth = 0;
		A.MonthKey = Opening;
	}
}
} // namespace site_money_detail

// ------------------------------------------------------------------ the shared helpers (SitesDetail.h)

const std::vector<TierInfo>& RewardsTiers()
{
	static const std::vector<TierInfo> T = {{"Bronze", 0, 0}, {"Silver", 4000, 80}, {"Gold", 20000, 140}, {"Platinum", 80000, 200}, {"Diamond", 300000, 270}};
	return T;
}

const std::vector<TierInfo>& SecondRunTiers()
{
	static const std::vector<TierInfo> T = {{"no tier", 0, 0}, {"Member", 5000, 150}, {"Regular", 25000, 200}, {"High Table", 100000, 250}, {"Inner Circle", 400000, 300}};
	return T;
}

int TierFor(const std::vector<TierInfo>& Tiers, Chips Rake)
{
	int Tier = 0;
	for (size_t I = 0; I < Tiers.size(); ++I)
	{
		Tier = Rake >= Tiers[I].From ? static_cast<int>(I) : Tier;
	}
	return Tier;
}

std::string MonthKeyAt(double World)
{
	int Y = 0;
	int M = 1;
	int D = 1;
	world::CivilDate(net::DayOf(World), Y, M, D);
	return std::to_string(Y) + (M < 10 ? "-0" : "-") + std::to_string(M);
}

std::string MonthName(const std::string& Key)
{
	static const char* const Names[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
	const int M = Key.size() >= 7 ? std::atoi(Key.c_str() + 5) : 0;
	return M >= 1 && M <= 12 ? Names[M - 1] : Key;
}

double NextMonthStart(double World)
{
	int Y = 0;
	int M = 1;
	int D = 1;
	world::CivilDate(net::DayOf(World), Y, M, D);
	return static_cast<double>(M == 12 ? world::DayOn(Y + 1, 1, 1) : world::DayOn(Y, M + 1, 1)) * site_money_detail::SiteDay;
}

Account* AccountOf(Book& B, net::SiteId S)
{
	for (Account& A : B.Accounts)
	{
		if (A.Site == S)
		{
			return &A;
		}
	}
	return nullptr;
}

const Account* AccountOf(const Book& B, net::SiteId S)
{
	for (const Account& A : B.Accounts)
	{
		if (A.Site == S)
		{
			return &A;
		}
	}
	return nullptr;
}

bool IsRival(net::SiteId S)
{
	if (S == net::SiteId::RiverLine)
	{
		return false;
	}
	for (const net::SiteDef& D : net::Sites())
	{
		if (D.Id == S)
		{
			return !D.P1 && !D.Linked;
		}
	}
	return false;
}

std::string FromLine(net::SiteId S)
{
	const net::SiteDef& D = net::Def(S);
	return D.RepTitle.empty() || D.Rep.find(D.RepTitle) != std::string::npos ? D.Rep : D.Rep + ", " + D.RepTitle;
}

const net::CashMethod* MethodFor(net::SiteId S, const std::string& Id, bool Out)
{
	if (!IsRival(S))
	{
		return nullptr;
	}
	for (const net::CashMethod& M : net::Def(S).Cashier)
	{
		const bool Way = Out ? M.Withdraws : !M.P1; // P0 deposits: the methods whose deposit side isn't P1 (the card)
		if (Way && (Id.empty() || Id == M.Id))
		{
			return &M;
		}
	}
	return nullptr;
}

bool GateMet(net::SiteId S, const rep::Public& P, int RentsPaid, double World, int* Why)
{
	if (!IsRival(S))
	{
		return false;
	}
	const net::SiteDef& D = net::Def(S);
	if (World < static_cast<double>(D.LaunchDay) * site_money_detail::SiteDay || RentsPaid < D.Gate.MinRentsPaid)
	{
		return false;
	}
	const bool Volume = D.Gate.MinEvents > 0 && P.OnlineEvents >= D.Gate.MinEvents && P.Abi100 >= D.Gate.MinAbi100;
	const bool Title = D.Gate.SeriesTitle && P.OnlineSeriesTitles > 0;
	const bool Partner = D.Gate.KastPartner && P.Partner;
	if (Why)
	{
		*Why = Volume ? 0 : Title ? 1 : 2;
	}
	return Volume || Title || Partner;
}

void PostLetter(life::State& L, net::SiteId S, double World, int Kind, const std::string& Subject, const std::string& Body)
{
	life::Letter M;
	M.At = World;
	M.Thread = net::KeyOf(S);
	M.Source = "site";
	M.From = FromLine(S);
	M.Subject = Subject;
	M.Body = Body;
	M.Kind = Kind;
	life::Post(L, M);
}

bool WroteThisWeek(const life::State& L, net::SiteId S, double World)
{
	const std::string Thread = net::KeyOf(S);
	for (const life::Letter& M : L.Inbox)
	{
		if (M.Source == "site" && M.Thread == Thread && M.Kind >= LetterRoutine && M.At > World - 7.0 * site_money_detail::SiteDay)
		{
			return true;
		}
	}
	return false;
}

std::string FillCopy(const std::string& Template, const std::vector<std::pair<std::string, std::string>>& Vars)
{
	std::string Out;
	size_t I = 0;
	while (I < Template.size())
	{
		const size_t Open = Template.find('{', I);
		const size_t Close = Open == std::string::npos ? std::string::npos : Template.find('}', Open + 1);
		if (Close == std::string::npos)
		{
			Out.append(Template, I, std::string::npos);
			break;
		}
		Out.append(Template, I, Open - I);
		const std::string Name = Template.substr(Open + 1, Close - Open - 1);
		bool Found = false;
		for (const std::pair<std::string, std::string>& V : Vars)
		{
			if (V.first == Name && !Found)
			{
				Out += V.second;
				Found = true;
			}
		}
		Out += Found ? std::string() : Template.substr(Open, Close - Open + 1);
		I = Close + 1;
	}
	return Out;
}

void Invite(Book& B, life::State& L, net::SiteId S, double World, int Why, bool Second)
{
	using namespace site_money_detail;
	Invitation* Found = nullptr;
	for (Invitation& I : B.Invites)
	{
		Found = I.Site == S ? &I : Found;
	}
	if (!Found)
	{
		B.Invites.push_back(Invitation());
		Found = &B.Invites.back();
		Found->Site = S;
	}
	Found->At = World;
	Found->State = Second ? 3 : 0;
	const std::string Key = std::string(Second ? "invite2:" : "invite:") + net::KeyOf(S);
	const std::vector<std::pair<std::string, std::string>> Vars = {{"Why", SiteText("why:" + std::to_string(std::max(0, std::min(2, Why))))}};
	PostLetter(L, S, World, LetterInvite, FillCopy(SiteText(Key + ":subject"), Vars), FillCopy(SiteText(Key), Vars));
	L.Update2LinesRead |= life::U2Sites;
}

const char* CopyText(const std::string& Key)
{
	return site_money_detail::SiteText(Key);
}

std::vector<std::string> Copy()
{
	std::vector<std::string> Out;
	for (const site_money_detail::SiteCopyLine& C : site_money_detail::SiteCopyTable)
	{
		if (C.Sentence)
		{
			Out.push_back(C.Text);
		}
	}
	return Out;
}

std::vector<std::string> CopyLabels()
{
	std::vector<std::string> Out;
	for (const site_money_detail::SiteCopyLine& C : site_money_detail::SiteCopyTable)
	{
		if (!C.Sentence)
		{
			Out.push_back(C.Text);
		}
	}
	return Out;
}

// ------------------------------------------------------------------ the money (Sites.h)

std::string Join(Book& B, net::SiteId S, double World)
{
	using namespace site_money_detail;
	if (!IsRival(S))
	{
		return SiteText("err:site");
	}
	if (AccountOf(B, S))
	{
		return std::string();
	}
	Account A;
	A.Site = S;
	A.JoinedAt = World;
	A.MonthKey = MonthKeyAt(World);
	A.Hero.Year = world::YearOf(net::DayOf(World));
	B.Accounts.push_back(A);
	return std::string();
}

void AddLine(Book& B, net::SiteId S, double At, int Kind, Chips Cents, const std::string& Label)
{
	// A line only: whoever moved the wallet (stake::Move on a rival Pot) moved it through WalletOf's pointer.
	if (Account* A = AccountOf(B, S))
	{
		site_money_detail::SiteLineTo(*A, At, Kind, Cents, Label);
	}
}

std::string Deposit(Book& B, life::State& L, Chips& Bankroll, net::SiteId S, Chips Cents, const std::string& Method, double World)
{
	using namespace site_money_detail;
	if (S == net::SiteId::RiverLine)
	{
		return SiteText("err:riverline");
	}
	const std::vector<std::pair<std::string, std::string>> Site = {{"Site", IsRival(S) ? SiteName(S) : "That site"}};
	Account* A = AccountOf(B, S);
	if (!IsRival(S) || !A)
	{
		return IsRival(S) ? FillCopy(SiteText("err:account"), Site) : std::string(SiteText("err:site"));
	}
	const net::CashMethod* M = MethodFor(S, Method, false);
	if (!M)
	{
		return FillCopy(SiteText("err:method:in"), Site);
	}
	if (Cents < M->MinCents)
	{
		return FillCopy(SiteText("err:min:in"), {{"Min", Money(M->MinCents)}});
	}
	const Chips Fee = static_cast<Chips>(std::llround(static_cast<double>(Cents) * static_cast<double>(M->FeePermille) / 1000.0));
	if (Bankroll < Cents + Fee)
	{
		return FillCopy(SiteText("err:bank"), {{"Amount", Money(Cents)}, {"Fee", Money(Fee)}});
	}
	const bool First = A->Deposited == 0;
	// Out of the bank at once: the amount as a transfer (the Bank skips kind 7 in its In and Out), the fee as the sites' own kind.
	Bankroll -= Cents + Fee;
	L.Record(World, FillCopy(SiteText("label:deposit"), Site), -Cents, 7);
	if (Fee > 0)
	{
		std::string Way = M->Name;
		std::transform(Way.begin(), Way.end(), Way.begin(), [](char Ch) { return Ch >= 'A' && Ch <= 'Z' ? static_cast<char>(Ch - 'A' + 'a') : Ch; });
		L.Record(World, FillCopy(SiteText("label:fee"), {{"Site", SiteName(S)}, {"Method", Way}}), -Fee, life::LedgerSites);
	}
	A->Cents += Cents;
	A->Deposited += Cents;
	SiteLineTo(*A, World, 7, Cents, SiteText("label:wallet:deposit"));
	// The welcome offers (§5.6), on the first deposit: Ignite matches it up to $100 (pending, released by rake inside 60 days);
	// RunItTwice puts $33 in tickets on a first deposit of $50 or more.
	if (First && S == net::SiteId::Ignite)
	{
		A->BonusPending = std::min(Cents, SiteBonusMax);
		A->BonusReleased = 0;
		A->BonusExpires = World + SiteBonusDays * SiteDay;
	}
	if (First && S == net::SiteId::RunItTwice && Cents >= SiteTicketDeposit && !SiteDone(B, "tickets:runittwice"))
	{
		L.Tickets["runittwice.sunday-grind"] += 1;
		L.Tickets["runittwice.supper-stack"] += 2;
		B.Done.insert("tickets:runittwice");
		SiteRoutine(L, S, World, SiteText("tickets:subject"), SiteText("tickets"));
	}
	// RiverLine notices (§5.7): the first deposit on any rival brings "We noticed." and a tier up for 60 days.
	if (!SiteDone(B, "retention"))
	{
		B.Done.insert("retention");
		B.RiverLine.BoostUntil = World + SiteBoostDays * SiteDay;
		if (B.RiverLine.MonthKey.empty())
		{
			B.RiverLine.MonthKey = MonthKeyAt(World);
		}
		PostLetter(L, net::SiteId::RiverLine, World, LetterRoutine, SiteText("retention:subject"), SiteText("retention"));
		L.Update2LinesRead |= life::U2Rewards;
	}
	L.Update2LinesRead |= life::U2Sites;
	return std::string();
}

std::string Withdraw(Book& B, net::SiteId S, Chips Cents, const std::string& Method, double World)
{
	using namespace site_money_detail;
	if (S == net::SiteId::RiverLine)
	{
		return SiteText("err:riverline");
	}
	const std::vector<std::pair<std::string, std::string>> Site = {{"Site", IsRival(S) ? SiteName(S) : "That site"}};
	Account* A = AccountOf(B, S);
	if (!IsRival(S) || !A)
	{
		return IsRival(S) ? FillCopy(SiteText("err:account"), Site) : std::string(SiteText("err:site"));
	}
	const net::CashMethod* M = MethodFor(S, Method, true);
	if (!M)
	{
		return FillCopy(SiteText("err:method:out"), Site);
	}
	const Chips Min = std::max(net::Def(S).MinWithdraw, M->WithdrawFeeCents + 1);
	if (Cents < Min)
	{
		return FillCopy(SiteText("err:min:out"), {{"Min", Money(Min)}});
	}
	if (Cents > A->Cents)
	{
		return FillCopy(SiteText("err:balance"), Site);
	}
	// Out of the wallet at once; in transit (in neither balance) until SiteStep lands it in the bank, net of the fee. The wait
	// is drawn once, from the request's id, between the method's hours, to the minute.
	Transfer T;
	T.Id = std::string(net::KeyOf(S)) + ":" + std::to_string(std::llround(World));
	for (int N = 2; std::any_of(B.Transfers.begin(), B.Transfers.end(), [&T](const Transfer& X) { return X.Id == T.Id; }); ++N)
	{
		T.Id = std::string(net::KeyOf(S)) + ":" + std::to_string(std::llround(World)) + "-" + std::to_string(N);
	}
	T.Site = S;
	T.Out = true;
	T.Method = M->Id;
	T.Cents = Cents;
	T.FeeCents = M->WithdrawFeeCents;
	T.RequestedAt = World;
	const double Draw = static_cast<double>(Fnv1a(T.Id) % 10001u) / 10000.0;
	const double Hours = M->WithdrawHoursMin + (M->WithdrawHoursMax - M->WithdrawHoursMin) * Draw;
	T.ArrivesAt = World + std::round(Hours * SiteHour);
	T.State = 0;
	A->Cents -= Cents;
	A->Withdrawn += Cents;
	SiteLineTo(*A, World, 7, -Cents, FillCopy(SiteText("label:wallet:withdraw"), {{"Fee", Money(T.FeeCents)}}));
	B.Transfers.push_back(T);
	return std::string();
}

std::string Reverse(Book& B, const std::string& TransferId, double World)
{
	// P1 (Ignite's "Change your mind? Play it instead."): no method can be reversed in P0.
	(void)B;
	(void)TransferId;
	(void)World;
	return site_money_detail::SiteText("err:reverse");
}

Chips* WalletOf(Book& B, net::SiteId S)
{
	// RiverLine has no wallet: its money is the bank's.
	Account* A = S == net::SiteId::RiverLine ? nullptr : AccountOf(B, S);
	return A ? &A->Cents : nullptr;
}

void OnBuyIn(Book& B, net::SiteId S, Chips BuyIn, Chips Fee, const std::string& Label, double World)
{
	using namespace site_money_detail;
	if (S == net::SiteId::RiverLine)
	{
		// RiverLine Rewards (§5.7): the month's rake, every RiverLine event entered with money, played by hand or by the plan.
		Rewards& R = B.RiverLine;
		if (R.MonthKey.empty())
		{
			R.MonthKey = MonthKeyAt(World);
		}
		R.Rake += std::max<Chips>(0, Fee);
		return;
	}
	Account* A = AccountOf(B, S);
	if (!A)
	{
		return;
	}
	// The session took the buy-in out of the wallet; the line says so. The rake counts toward the month (rakeback) and the
	// account's life (Ignite's welcome bonus comes off $5 for every $20 of it, inside the 60 days).
	if (BuyIn > 0)
	{
		SiteLineTo(*A, World, 0, -BuyIn, Label);
	}
	if (A->MonthKey.empty())
	{
		A->MonthKey = MonthKeyAt(World);
	}
	A->RakeMonth += std::max<Chips>(0, Fee);
	A->RakeLife += std::max<Chips>(0, Fee);
	if (A->BonusPending > 0 && World <= A->BonusExpires)
	{
		const Chips Total = A->BonusPending + A->BonusReleased;
		const Chips Earned = std::min(Total, (A->RakeLife / SiteBonusRake) * SiteBonusPiece);
		const Chips Due = Earned - A->BonusReleased;
		if (Due > 0)
		{
			A->Cents += Due;
			A->BonusPending -= Due;
			A->BonusReleased += Due;
			SiteLineTo(*A, World, 4, Due, SiteText("label:wallet:bonus"));
		}
	}
}

void OnPrize(Book& B, net::SiteId S, Chips Cents, const std::string& Label, double World, const Season& Add)
{
	using namespace site_money_detail;
	Account* A = S == net::SiteId::RiverLine ? nullptr : AccountOf(B, S);
	if (!A)
	{
		return;
	}
	// The session put the prize in the wallet; the line says so, and the player's record on the site moves.
	if (Cents > 0)
	{
		SiteLineTo(*A, World, 0, Cents, Label);
		A->LifetimeWon += Cents;
	}
	const int Year = Add.Year != 0 ? Add.Year : world::YearOf(net::DayOf(World));
	if (A->Hero.Year != Year)
	{
		A->Hero = Season();
		A->Hero.Year = Year;
	}
	A->Hero.Points += Add.Points;
	A->Hero.Won += Add.Won;
	A->Hero.Wins += Add.Wins;
	A->Hero.FinalTables += Add.FinalTables;
	A->Hero.Events += Add.Events;
	A->LifetimeEvents += Add.Events;
}

void Step(Book& B, life::State& L, Chips& Bankroll, const rep::Public& P, int SponsorTier, double From, double To, std::vector<std::pair<std::string, std::string>>& Mail)
{
	using namespace site_money_detail;
	(void)SponsorTier; // Halcyon's door (P1)
	if (To < From)
	{
		return;
	}
	// Withdrawals landing, in the order they arrive: one bank line each, net of the fee (it shows on the request's line).
	std::vector<Transfer*> Landing;
	for (Transfer& T : B.Transfers)
	{
		if (T.State == 0 && T.Out && T.ArrivesAt <= To)
		{
			Landing.push_back(&T);
		}
	}
	std::stable_sort(Landing.begin(), Landing.end(), [](const Transfer* X, const Transfer* Y) { return X->ArrivesAt < Y->ArrivesAt; });
	for (Transfer* T : Landing)
	{
		const Chips Net = T->Cents - T->FeeCents;
		Bankroll += Net;
		L.Record(T->ArrivesAt, FillCopy(SiteText("label:withdrawal"), {{"Site", SiteName(T->Site)}}), Net, 7);
		T->State = 1;
		L.Update2LinesRead |= life::U2Sites;
	}
	// Every 1st crossed: RiverLine Rewards into the bank, the Second Run Club into RunItTwice's wallet, the months moved on.
	for (double Payday = NextMonthStart(From); Payday <= To; Payday = NextMonthStart(Payday))
	{
		if (Payday > From)
		{
			SiteCloseMonth(B, L, Bankroll, Payday);
		}
	}
	// Ignite's welcome bonus: what's still pending when the 60 days are up expires.
	for (Account& A : B.Accounts)
	{
		if (A.BonusPending > 0 && A.BonusExpires > 0.0 && A.BonusExpires <= To)
		{
			const Chips Left = A.BonusPending;
			A.BonusPending = 0;
			SiteRoutine(L, A.Site, std::max(From, A.BonusExpires), SiteText("bonusend:subject"),
				FillCopy(SiteText("bonusend"), {{"Released", Money(A.BonusReleased)}, {"Left", Money(Left)}}));
		}
	}
	// The welcome letter, the moment an account opens (AnswerInvite steps the calendar right after).
	for (const Account& A : B.Accounts)
	{
		const std::string Key = std::string("welcome:") + net::KeyOf(A.Site);
		if (!SiteDone(B, Key))
		{
			B.Done.insert(Key);
			PostLetter(L, A.Site, To, LetterRoutine, SiteText(Key + ":subject"), SiteText(Key));
		}
	}
	// Once a day: the invitation gates (the first day one is met, §5.4), and a declined invitation's second letter 30 days on.
	if (SiteNewDay(From, To))
	{
		for (const net::SiteDef& D : net::Sites())
		{
			if (!IsRival(D.Id) || AccountOf(B, D.Id))
			{
				continue;
			}
			const Invitation* Sent = nullptr;
			for (const Invitation& I : B.Invites)
			{
				Sent = I.Site == D.Id ? &I : Sent;
			}
			int Why = 0;
			const std::string Again = std::string("resent:") + D.Key;
			if (!Sent && GateMet(D.Id, P, L.RentsPaid, To, &Why))
			{
				Invite(B, L, D.Id, To, Why, false);
			}
			else if (Sent && Sent->State == 2 && Sent->At + SiteResendDays * SiteDay <= To && !SiteDone(B, Again))
			{
				B.Done.insert(Again);
				Invite(B, L, D.Id, To, 0, true);
			}
		}
		// Housekeeping: transfers that landed a month ago, and dated keys 120 days on.
		B.Transfers.erase(std::remove_if(B.Transfers.begin(), B.Transfers.end(), [To](const Transfer& T) { return T.State != 0 && T.ArrivesAt + SiteKeepTransferDays * SiteDay < To; }),
			B.Transfers.end());
		for (auto It = B.Done.begin(); It != B.Done.end();)
		{
			const size_t At = It->rfind('@');
			const bool Old = At != std::string::npos && std::atof(It->c_str() + At + 1) + SiteKeepDoneDays < static_cast<double>(net::DayOf(To));
			It = Old ? B.Done.erase(It) : std::next(It);
		}
	}
	// Word of mouth (people text, §1.7): a few nights after Ignite opens, someone from Dee's game has found it.
	const double Opened = static_cast<double>(net::Def(net::SiteId::Ignite).LaunchDay) * SiteDay;
	const double Rick = Opened + SiteRickAfterDays * SiteDay + 22.0 * SiteHour;
	if (IsRival(net::SiteId::Ignite) && To >= Rick && To < Opened + SiteRickWithinDays * SiteDay && L.BackRoomNights > 0 && !SiteDone(B, "rick:ignite"))
	{
		B.Done.insert("rick:ignite");
		Mail.emplace_back("Rick", SiteText("rick:ignite"));
		L.Update2LinesRead |= life::U2Sites;
	}
	if (!B.Done.empty() || !B.Accounts.empty() || !B.Invites.empty())
	{
		L.Update2LinesRead |= life::U2Sites;
	}
}

bool RentWarning(const Book& B, Chips Bankroll, Chips RentDue, double Deadline, double World, std::string& Line)
{
	using namespace site_money_detail;
	if (RentDue <= 0 || World >= Deadline || Deadline - World > 72.0 * SiteHour || Bankroll >= RentDue)
	{
		return false;
	}
	Chips OnSites = 0;
	std::vector<std::string> Parts;
	for (const Transfer& T : B.Transfers)
	{
		if (T.State != 0 || !T.Out)
		{
			continue;
		}
		const net::CashMethod* M = MethodFor(T.Site, T.Method, true);
		const double First = T.RequestedAt + (M ? M->WithdrawHoursMin : 0.0) * SiteHour;
		const double Last = T.RequestedAt + (M ? M->WithdrawHoursMax : 0.0) * SiteHour;
		const std::string Window = net::DayOf(First) == net::DayOf(T.ArrivesAt) && net::DayOf(Last) == net::DayOf(T.ArrivesAt)
			? std::string(net::WeekdayName(net::DayOf(T.ArrivesAt)))
			: std::string(net::WeekdayName(net::DayOf(First))) + " to " + net::WeekdayName(net::DayOf(Last));
		OnSites += T.Cents - T.FeeCents;
		Parts.push_back(FillCopy(SiteText("rent:transit"), {{"Amount", Money(T.Cents - T.FeeCents)}, {"Site", SiteName(T.Site)}, {"Window", Window}}));
	}
	for (const Account& A : B.Accounts)
	{
		if (A.Cents > 0)
		{
			OnSites += A.Cents;
			Parts.push_back(FillCopy(SiteText("rent:parked"), {{"Amount", Money(A.Cents)}, {"Site", SiteName(A.Site)}}));
		}
	}
	if (Parts.empty() || Bankroll + OnSites < RentDue)
	{
		return false; // nothing on the sites, or not even all of it would cover it: the rent card's own warning says enough
	}
	std::string Money;
	for (size_t I = 0; I < Parts.size(); ++I)
	{
		Money += (I == 0 ? std::string() : I + 1 == Parts.size() ? std::string(", and ") : std::string(", ")) + Parts[I];
	}
	Line = FillCopy(SiteText("rent"), {{"Day", net::WeekdayName(net::DayOf(Deadline - 1.0))}, {"Money", Money + "."}});
	return true;
}

std::vector<life::Mark> Marks(const Book& B, double From, double To)
{
	using namespace site_money_detail;
	std::vector<life::Mark> Out;
	auto Add = [&Out](double At, int Kind, const std::string& Label, const std::string& Where) {
		life::Mark M;
		M.At = At;
		M.Kind = Kind;
		M.Label = Label;
		M.Where = Where;
		Out.push_back(M);
	};
	for (const Transfer& T : B.Transfers)
	{
		if (T.State == 0 && T.Out && T.ArrivesAt >= From && T.ArrivesAt <= To)
		{
			Add(T.ArrivesAt, MarkTransfer, FillCopy(SiteText("mark:transfer"), {{"Site", SiteName(T.Site)}, {"Amount", Money(T.Cents - T.FeeCents)}}), SiteName(T.Site));
		}
	}
	// RiverLine Rewards' payday: the 1st after the month the rake is counting for, when there's rake to pay on.
	if (!B.RiverLine.MonthKey.empty() && B.RiverLine.Rake > 0)
	{
		// "2026-11" (no sscanf: Unreal's build warns C4996 on it).
		const std::string& Key = B.RiverLine.MonthKey;
		const int Y = Key.size() >= 7 ? std::atoi(Key.c_str()) : 0;
		const int M = Key.size() >= 7 ? std::atoi(Key.c_str() + 5) : 0;
		const double Payday = Y > 0 && M >= 1 && M <= 12 ? static_cast<double>(M == 12 ? world::DayOn(Y + 1, 1, 1) : world::DayOn(Y, M + 1, 1)) * SiteDay : -1.0;
		if (Payday >= From && Payday <= To)
		{
			Add(Payday, MarkRewards, SiteText("mark:rewards"), "RiverLine");
		}
	}
	for (const Account& A : B.Accounts)
	{
		if (A.BonusPending > 0 && A.BonusExpires >= From && A.BonusExpires <= To)
		{
			Add(A.BonusExpires, MarkBonus, FillCopy(SiteText("mark:bonus"), {{"Site", SiteName(A.Site)}}), SiteName(A.Site));
		}
		// A joined rival's series start days.
		for (const net::SeriesInfo& S : net::Site(A.Site).Series())
		{
			const double Starts = static_cast<double>(S.FirstDay) * SiteDay;
			if (Starts >= From && Starts <= To)
			{
				Add(Starts, MarkSeries, FillCopy(SiteText("mark:series"), {{"Series", S.Name}}), SiteName(A.Site));
			}
		}
	}
	std::stable_sort(Out.begin(), Out.end(), [](const life::Mark& X, const life::Mark& Y) { return X.At < Y.At; });
	return Out;
}

std::vector<std::string> Inventory()
{
	// The copy table's sentences, and the rivals' own words on their pages: their taglines, their events' blurbs, their series'.
	std::vector<std::string> Out = Copy();
	std::set<std::string> Seen(Out.begin(), Out.end());
	for (const net::SiteDef& D : net::Sites())
	{
		if (!IsRival(D.Id))
		{
			continue;
		}
		std::vector<std::string> Words = {D.Tagline};
		for (const net::EventTemplate& T : net::Site(D.Id).Templates())
		{
			Words.push_back(T.Blurb);
		}
		for (const net::SeriesInfo& S : net::Site(D.Id).Series())
		{
			Words.push_back(S.Tagline);
		}
		for (const std::string& W : Words)
		{
			if (!W.empty() && Seen.insert(W).second)
			{
				Out.push_back(W);
			}
		}
	}
	return Out;
}

// ------------------------------------------------------------------ the save lines (§6.4)

void WriteLines(const life::State& L, std::string& Out)
{
	// Pure const (it runs on the save worker). Only what the book holds: an old save nobody touched, and a new career, have
	// nothing to write, and every line read back is written back as it was (Serialize(Parse(S)) == S).
	using namespace site_money_detail;
	const Book& B = L.Sites;
	for (const Account& A : B.Accounts)
	{
		Out += "site\tacct\t" + std::to_string(static_cast<int>(A.Site)) + "\t" + SiteNum(A.JoinedAt) + "\t" + std::to_string(A.Cents) + "\t" + SiteEscape(A.MonthKey) + "\t" +
			std::to_string(A.RakeMonth) + "\t" + std::to_string(A.RakeLife) + "\t" + std::to_string(A.Vip) + "\t" + SiteNum(A.DealUntil) + "\t" + std::to_string(A.BonusPending) +
			"\t" + std::to_string(A.BonusReleased) + "\t" + SiteNum(A.BonusExpires) + "\t" + std::to_string(A.Deposited) + "\t" + std::to_string(A.Withdrawn) + "\t" +
			SiteNum(A.Hero.Points) + "\t" + std::to_string(A.Hero.Won) + "\t" + std::to_string(A.Hero.Wins) + "\t" + std::to_string(A.Hero.FinalTables) + "\t" +
			std::to_string(A.Hero.Events) + "\t" + std::to_string(A.Hero.Year) + "\t" + std::to_string(A.LifetimeWon) + "\t" + std::to_string(A.LifetimeEvents) + "\n";
		for (const Line& X : A.Lines)
		{
			Out += "site\tline\t" + std::to_string(static_cast<int>(A.Site)) + "\t" + SiteNum(X.At) + "\t" + std::to_string(X.Kind) + "\t" + std::to_string(X.Cents) + "\t" +
				SiteEscape(X.Label) + "\n";
		}
	}
	for (const Transfer& T : B.Transfers)
	{
		Out += "site\txfer\t" + SiteEscape(T.Id) + "\t" + std::to_string(static_cast<int>(T.Site)) + "\t" + (T.Out ? "1" : "0") + "\t" + SiteEscape(T.Method) + "\t" +
			std::to_string(T.Cents) + "\t" + std::to_string(T.FeeCents) + "\t" + SiteNum(T.RequestedAt) + "\t" + SiteNum(T.ArrivesAt) + "\t" + std::to_string(T.State) + "\n";
	}
	for (const Invitation& I : B.Invites)
	{
		Out += "site\tinvite\t" + std::to_string(static_cast<int>(I.Site)) + "\t" + SiteNum(I.At) + "\t" + std::to_string(I.State) + "\n";
	}
	// RiverLine Rewards once it has anything to keep (a month's rake, a tier, the boost): a new career sets every bit and has
	// nothing yet, so it writes no line; a line this code wrote always has its month.
	const Rewards& R = B.RiverLine;
	if (!R.MonthKey.empty() || R.Rake != 0 || R.Tier != 0 || R.BoostUntil != 0.0)
	{
		Out += "site\trewards\t" + SiteEscape(R.MonthKey) + "\t" + std::to_string(R.Rake) + "\t" + std::to_string(R.Tier) + "\t" + SiteNum(R.BoostUntil) + "\n";
	}
	for (const std::string& K : B.Done)
	{
		Out += "site\tdone\t" + SiteEscape(K) + "\n";
	}
}

bool ReadLine(const std::vector<std::string>& P, life::State& L)
{
	using namespace site_money_detail;
	if (P.size() < 2 || P[0] != "site")
	{
		return false;
	}
	Book& B = L.Sites;
	const std::string& Tag = P[1];
	auto SiteAt = [&P](size_t I) { return static_cast<net::SiteId>(std::atoi(P[I].c_str())); };
	if (Tag == "acct" && P.size() >= 23)
	{
		Account A;
		A.Site = SiteAt(2);
		A.JoinedAt = SiteToNum(P[3]);
		A.Cents = SiteToInt(P[4]);
		A.MonthKey = SiteUnescape(P[5]);
		A.RakeMonth = SiteToInt(P[6]);
		A.RakeLife = SiteToInt(P[7]);
		A.Vip = static_cast<int>(SiteToInt(P[8]));
		A.DealUntil = SiteToNum(P[9]);
		A.BonusPending = SiteToInt(P[10]);
		A.BonusReleased = SiteToInt(P[11]);
		A.BonusExpires = SiteToNum(P[12]);
		A.Deposited = SiteToInt(P[13]);
		A.Withdrawn = SiteToInt(P[14]);
		A.Hero.Points = SiteToNum(P[15]);
		A.Hero.Won = SiteToInt(P[16]);
		A.Hero.Wins = static_cast<int>(SiteToInt(P[17]));
		A.Hero.FinalTables = static_cast<int>(SiteToInt(P[18]));
		A.Hero.Events = static_cast<int>(SiteToInt(P[19]));
		A.Hero.Year = static_cast<int>(SiteToInt(P[20]));
		A.LifetimeWon = SiteToInt(P[21]);
		A.LifetimeEvents = static_cast<int>(SiteToInt(P[22]));
		if (!AccountOf(B, A.Site))
		{
			B.Accounts.push_back(A);
		}
	}
	else if (Tag == "line" && P.size() >= 7)
	{
		if (Account* A = AccountOf(B, SiteAt(2)))
		{
			Line X;
			X.At = SiteToNum(P[3]);
			X.Kind = static_cast<int>(SiteToInt(P[4]));
			X.Cents = SiteToInt(P[5]);
			X.Label = SiteUnescape(P[6]);
			A->Lines.push_back(X);
		}
	}
	else if (Tag == "xfer" && P.size() >= 11)
	{
		Transfer T;
		T.Id = SiteUnescape(P[2]);
		T.Site = SiteAt(3);
		T.Out = P[4] == "1";
		T.Method = SiteUnescape(P[5]);
		T.Cents = SiteToInt(P[6]);
		T.FeeCents = SiteToInt(P[7]);
		T.RequestedAt = SiteToNum(P[8]);
		T.ArrivesAt = SiteToNum(P[9]);
		T.State = static_cast<int>(SiteToInt(P[10]));
		B.Transfers.push_back(T);
	}
	else if (Tag == "invite" && P.size() >= 5)
	{
		Invitation I;
		I.Site = SiteAt(2);
		I.At = SiteToNum(P[3]);
		I.State = static_cast<int>(SiteToInt(P[4]));
		B.Invites.push_back(I);
	}
	else if (Tag == "rewards" && P.size() >= 6)
	{
		B.RiverLine.MonthKey = SiteUnescape(P[2]);
		B.RiverLine.Rake = SiteToInt(P[3]);
		B.RiverLine.Tier = static_cast<int>(SiteToInt(P[4]));
		B.RiverLine.BoostUntil = SiteToNum(P[5]);
		L.Update2LinesRead |= life::U2Rewards;
	}
	else if (Tag == "done" && P.size() >= 3)
	{
		B.Done.insert(SiteUnescape(P[2]));
	}
	// Every site line is ours, a tag this build doesn't know included (a newer build's): nothing else reads it.
	L.Update2LinesRead |= life::U2Sites;
	return true;
}

void AfterParse(SaveData& D)
{
	// An old save has an empty book: nothing to migrate (§6.5). From G3 the first SiteStep over a midnight checks the gates,
	// so a veteran past Act I may find an invitation the first morning after updating.
	(void)D;
}
} // namespace site
} // namespace ss
