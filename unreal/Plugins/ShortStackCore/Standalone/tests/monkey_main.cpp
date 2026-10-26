// Monkey test: drives the session and the RiverLine client with random clicks and keys across every screen, sits
// down at random events (up to four tables), and checks invariants every frame: the bankroll only moves through the
// ledger, tournament chips are conserved, the clock never runs backwards, tables never stall, saves round-trip.
// Update 2 (C1): the plan in the seat too: random autoplay changes, take-overs and skips on the open tables, and no
// decision the plan made is ever graded (each table's graded decisions are exactly the player's own); no frame writes
// more than 20 ledger lines.
//
// The segment's phase (the last segment's carried invariants, next_segment_spec §7.6; update 2 SPEC debt item 9): careers
// of Days days stepped an hour or a few at a time, with random staking (offers taken, countered or left; pieces listed and
// called off; debts paid; chops asked for; walking away), sponsors (offers signed, countered, declined; patches sewn and
// unpicked; notice; raises; appearances), the pawn, online events (the plan skipping, or Sprint) and nights at the
// Embercrest (played by the engine or the plan, or called off). After every step: the bankroll moves only through the
// ledger, never below zero; no backing or sponsor payment is written twice; at most one brand per exclusive category; at
// most two deals on the overlay; patches only for running contracts; sponsor property never sells at the pawn; an open
// offer is never dropped from Mail; no step writes more than 20 ledger lines; the staked roll never goes below zero; no AI
// decision graded. At the end the segment's lines are at most 80 KB of the save, update 2's non-world lines at most 60 KB,
// and the save reads back the same.
// Update 2 (C3): beside each step, on draws of their own, the rival sites and Rerun at random: invitations (as the console
// sends them) and Mail's answers, deposits and withdrawals (refused ones too), a rival's events by the plan or Sprint, uploads.
// After every step, SPEC §1.2's money invariants (SegSites): each wallet moves only by its lines; deposits out of the bank
// equal the wallets' deposits, their fees the card's rate; withdrawals asked for equal transit plus arrivals plus fees, to the
// cent; a transfer lands once; no month's Rewards or Rerun payout twice; no unanswered invitation dropped from Mail (opened,
// and Mail filled past its keep).
// Usage: monkey_test [seeds] [frames] [first seed] [segment careers] [days]
#include "ShortStack/AI/Style.h"
#include "ShortStack/Game/Autoplay.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/LiveAuto.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/Stake.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/Rng.h"
#include "ShortStack/UI/RiverLine.h"
#include "ShortStack/UI/ShopScreen.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace
{
int Failures = 0;
std::map<std::string, int> FailCounts;

void Fail(const std::string& What, int Seed, int Frame)
{
	if (FailCounts[What]++ < 3)
	{
		std::printf("FAIL seed %d frame %d: %s\n", Seed, Frame, What.c_str());
	}
	++Failures;
}

/**
 * Update 3 (W3, a deliberate rewrite at Phase A): the monkey presses a register button once or twice, at random (its own
 * draws, so every other draw is as before), and core's strike gate decides. A registration refused, under a clause without
 * the second press or for any other reason, leaves everything as it was: no table, no money moved, no ledger line, no strike.
 */
std::string MonkeyRegister(ss::Session& S, const ss::LobbyEvent& L, ss::Rng& Press, int Seed, int Frame)
{
	const bool Second = Press.Chance(0.5);
	auto Strikes = [&S]() {
		int N = 0;
		for (const ss::sponsor::Contract& C : S.Life.Sponsors.Contracts)
		{
			N += C.Strikes;
		}
		return N;
	};
	auto Wallets = [&S]() {
		ss::Chips Sum = 0;
		size_t Lines = 0;
		for (const ss::site::Account& A : S.Life.Sites.Accounts)
		{
			Sum += A.Cents;
			Lines += A.Lines.size();
		}
		return std::make_pair(Sum, Lines);
	};
	const ss::Chips Bank = S.BankrollCents;
	const std::pair<ss::Chips, size_t> Sites = Wallets();
	const int Tables = S.TableCount(), Struck = Strikes();
	const size_t Ledger = S.Life.Ledger.size(), Letters = S.Life.Inbox.size();
	const std::string Top = S.Life.Ledger.empty() ? std::string() : S.Life.Ledger.front().Label;
	const bool Playing = S.IsPlaying(L.Spec.Id);
	const std::string Why = S.RegisterEvent(L, ss::StrikeConfirm{Second});
	if (!Why.empty() && (S.BankrollCents != Bank || Wallets() != Sites || S.TableCount() != Tables || Strikes() != Struck || S.Life.Ledger.size() != Ledger ||
							(!S.Life.Ledger.empty() && S.Life.Ledger.front().Label != Top) || S.Life.Inbox.size() != Letters || S.IsPlaying(L.Spec.Id) != Playing))
	{
		Fail("strike gate: a refused registration changed something (" + Why + ")", Seed, Frame);
	}
	return Why;
}

struct Measurer : ss::ui::TextMeasurer
{
	float Width(const std::string& S, ss::ui::Font, float Size) const override { return static_cast<float>(S.size()) * Size * 0.55f; }
	float Ascent(ss::ui::Font, float Size) const override { return Size * 0.8f; }
};

struct Hooks : ss::SessionHooks
{
	ss::SaveData Last;
	bool HasSave = false;
	int Texts = 0;
	int GoOuts = 0;
	void Save(const ss::SaveData& D) override
	{
		Last = D;
		HasSave = true;
	}
	void Text(const std::string&, const std::string&) override { ++Texts; }
	bool GoOut(const std::string&, ss::Chips) override
	{
		++GoOuts;
		return false;
	}
};

struct Spot
{
	float X;
	float Y;
};

std::vector<Spot> Hotspots()
{
	std::vector<Spot> H = {{800, 550}, {270, 34}, {350, 34}, {500, 34}, {590, 34}, {690, 34}, {385, 259}, {1150, 539}, {1233, 539}, {1311, 539},
		{1300, 890}, {1200, 890}, {1450, 890}, {1330, 915}, {725, 895}, {905, 895}, {1085, 895}, {70, 892}, {166, 892}, {262, 892}, {89, 933}, {150, 837},
		{1250, 410}, {1382, 410}, {1514, 410}, {60, 981}, {170, 981}, {268, 981}, {360, 981}, {450, 981}, {800, 552}, {800, 600}, {800, 650}, {800, 700}};
	for (int K = 0; K < 10; ++K)
	{
		H.push_back({46.0f + 82.0f * static_cast<float>(K), 330});
		H.push_back({300, 431.0f + 60.0f * static_cast<float>(K)});
	}
	for (int K = 0; K < 5; ++K)
	{
		H.push_back({673.0f + 72.0f * static_cast<float>(K), 803});
	}
	// Names on the boards and in event panels (player cards), the card's close button and the backdrop around it.
	H.push_back({240.0f, 513.0f});
	H.push_back({1240.0f, 561.0f});
	H.push_back({1292.0f, 136.0f});
	H.push_back({60.0f, 950.0f});
	for (int K = 0; K < 8; ++K)
	{
		H.push_back({760.0f + 72.0f * static_cast<float>(K), 32}); // table tabs, +, tile toggle
	}
	for (int T = 0; T < 4; ++T)
	{
		const float Tx = 8.0f + static_cast<float>(T % 2) * 796.0f;
		const float Ty = 72.0f + static_cast<float>(T / 2) * 445.0f;
		for (int B = 0; B < 4; ++B)
		{
			H.push_back({Tx + 12.0f + 92.0f + static_cast<float>(B) * 193.0f, Ty + 407.0f});
		}
		H.push_back({Tx + 600.0f, Ty + 413.0f}); // pace chips
	}
	// GearDrop and Kast: the taskbar, the store's tabs, cards and order card, the studio's controls, chat, the channel page.
	const std::vector<Spot> Stream = {{470, 981}, {565, 981}, {60, 110}, {140, 110}, {235, 110}, {340, 110}, {460, 110}, {210, 436}, {480, 436}, {747, 436}, {1016, 436},
		{210, 790}, {480, 790}, {747, 790}, {1016, 790}, {887, 654}, {1080, 654}, {1340, 735}, {1513, 660}, {197, 30}, {283, 30}, {374, 30}, {130, 686}, {524, 547},
		{661, 686}, {697, 686}, {782, 698}, {880, 698}, {978, 698}, {110, 746}, {1085, 100}, {1160, 100}, {1230, 100}, {1530, 272}, {1530, 453}, {1530, 664},
		{1510, 917}, {1390, 213}, {898, 411}, {1028, 411}, {1040, 703}, {1240, 32}, {1335, 410}, {1520, 920}, {490, 30}, {130, 715}, {70, 763}, {202, 763}, {400, 763},
		{539, 763}, {741, 763}, {1010, 748}, {1252, 402}, {1333, 402}, {1414, 402}, {1508, 402}, {900, 286}, {900, 460}, {900, 634}, {1244, 223}, {846, 700}, {1201, 769}};
	H.insert(H.end(), Stream.begin(), Stream.end());
	// App screens: a coarse grid.
	for (int Y = 120; Y < 960; Y += 70)
	{
		for (int X = 80; X < 1600; X += 160)
		{
			H.push_back({static_cast<float>(X), static_cast<float>(Y)});
		}
	}
	return H;
}

bool SameEntry(const ss::life::LedgerEntry& A, const ss::life::LedgerEntry& B)
{
	// (The save keeps a line's minute to two places: the same line after a save and a load, update 2 C3's reload.)
	return std::fabs(A.At - B.At) <= 0.0051 && A.Label == B.Label && A.Amount == B.Amount && A.Kind == B.Kind;
}

/** How many of Cur's lines are new since Prev (the ledger keeps the newest 60: the shortest prefix after which it goes on as Prev). */
size_t FreshLines(const std::vector<ss::life::LedgerEntry>& Cur, const std::vector<ss::life::LedgerEntry>& Prev)
{
	size_t Fresh = Cur.size();
	if (Cur.size() < 60 && Cur.size() >= Prev.size())
	{
		Fresh = Cur.size() - Prev.size(); // not full yet: exact
	}
	for (size_t Nn = 0; Cur.size() >= 60 && Nn < Cur.size() && !Prev.empty(); ++Nn)
	{
		bool Match = true;
		for (size_t I = 0; Nn + I < Cur.size() && I < Prev.size() && Match; ++I)
		{
			Match = SameEntry(Cur[Nn + I], Prev[I]);
		}
		if (Match)
		{
			Fresh = Nn;
			break;
		}
	}
	return Fresh;
}

/** A plan for a table, drawn: a style that's on the menu, watch or skip (or off), a speed, a target and the stops. */
ss::autoplay::TableAuto RandomPlan(ss::Rng& R, bool Skip)
{
	namespace autoplay = ss::autoplay;
	std::vector<ss::style::Style> Menu;
	for (const ss::style::Info& I : ss::style::Catalog())
	{
		if (!I.Hidden && !I.BountyOnly)
		{
			Menu.push_back(I.S);
		}
	}
	static const int Speeds[] = {1, 2, 4, 8};
	static const autoplay::Target Targets[] = {autoplay::Target::NextBreak, autoplay::Target::Bubble, autoplay::Target::FinalTable, autoplay::Target::Result, autoplay::Target::Hands};
	autoplay::TableAuto A;
	A.S = Menu[static_cast<size_t>(R.Int(static_cast<int>(Menu.size())))];
	const double M = R.Next();
	A.M = Skip ? autoplay::Mode::Skip : M < 0.2 ? autoplay::Mode::Off : M < 0.6 ? autoplay::Mode::Watch : autoplay::Mode::Skip;
	A.Speed = Speeds[R.Int(4)];
	A.To = Targets[R.Int(5)];
	A.Hands = 5 + R.Int(60);
	A.Stops = static_cast<uint32_t>(R.Int(32));
	return A;
}

// ------------------------------------------------------------------ the segment's phase

struct SegHooks : ss::SessionHooks
{
	ss::SaveData Last;
	int Texts = 0;
	void Save(const ss::SaveData& D) override { Last = D; }
	void Text(const std::string&, const std::string&) override { ++Texts; }
	bool GoOut(const std::string&, ss::Chips) override { return true; }
};

/** The career's books, audited after every step (SegmentJourney's, session_main.cpp, as a monkey's). */
struct SegBooks
{
	ss::Chips Bank = 0;
	ss::Chips Injected = 0;
	std::vector<ss::life::LedgerEntry> Seen;
	std::vector<ss::life::LedgerEntry> Paid; // every backing and sponsor line once
	std::vector<ss::life::LedgerEntry> NewLines; // the last audit's new lines, oldest first (the sites' books read them)
	int MostLines = 0;

	void Start(const ss::Session& S)
	{
		Bank = S.BankrollCents;
		Seen = S.Life.Ledger;
	}
	void Give(ss::Session& S, ss::Chips Cents)
	{
		S.BankrollCents += Cents;
		Injected += Cents;
	}
	void Audit(ss::Session& S, int Seed, int Step, const std::string& Where)
	{
		namespace sponsor = ss::sponsor;
		const std::vector<ss::life::LedgerEntry>& Cur = S.Life.Ledger;
		const size_t Fresh = FreshLines(Cur, Seen);
		NewLines.assign(Cur.rend() - static_cast<std::ptrdiff_t>(Fresh), Cur.rend());
		ss::Chips Logged = 0;
		for (size_t K = 0; K < Fresh; ++K)
		{
			const ss::life::LedgerEntry& E = Cur[K];
			Logged += E.Amount;
			if (E.Kind == ss::life::LedgerBacking || E.Kind == ss::life::LedgerSponsors)
			{
				for (const ss::life::LedgerEntry& Old : Paid)
				{
					if (SameEntry(Old, E))
					{
						Fail("segment: a backing or sponsor payment written twice (" + E.Label + ", " + Where + ")", Seed, Step);
					}
				}
				Paid.push_back(E);
			}
		}
		MostLines = std::max(MostLines, static_cast<int>(Fresh));
		if (Fresh > 20)
		{
			Fail("segment: one step wrote more than 20 ledger lines (" + std::to_string(Fresh) + ", " + Where + ")", Seed, Step);
		}
		if (S.BankrollCents - Bank != Logged + Injected)
		{
			Fail("segment: the bankroll moved without the ledger (" + ss::Money(S.BankrollCents - Bank) + " vs " + ss::Money(Logged + Injected) + ", " + Where + ")", Seed, Step);
		}
		if (S.BankrollCents < 0)
		{
			Fail("segment: a negative bankroll (" + Where + ")", Seed, Step);
		}
		Bank = S.BankrollCents;
		Injected = 0;
		Seen = Cur;
		for (const ss::stake::Deal& D : S.Life.Stakes.Deals)
		{
			if (D.RollNow < 0)
			{
				Fail("segment: a staked roll below zero (" + Where + ")", Seed, Step);
			}
		}
		const double World = S.WorldMinutes();
		const sponsor::Book& B = S.Life.Sponsors;
		std::map<int, int> ByCategory;
		for (const sponsor::Brand& Br : sponsor::Brands())
		{
			if (sponsor::ExclusiveCategory(Br.Cat) && sponsor::Active(B, Br.Id, World) && ++ByCategory[static_cast<int>(Br.Cat)] > 1)
			{
				Fail("segment: two brands active in one exclusive category (" + Where + ")", Seed, Step);
			}
		}
		// The overlay's slots: the deals that take a spot on it (a deal can pay on stream without one).
		const std::vector<ss::kast::Deal> OnStream = sponsor::StreamView(B, World);
		if (std::count_if(OnStream.begin(), OnStream.end(), [](const ss::kast::Deal& D) { return D.Overlay; }) > ss::kast::MaxDeals)
		{
			Fail("segment: more than two deals on the stream's overlay (" + Where + ")", Seed, Step);
		}
		for (const std::string& Patch : S.Life.Closet.Patches)
		{
			if (!Patch.empty() && !sponsor::Running(B, Patch, World))
			{
				Fail("segment: a patch for a brand with no running contract (" + Patch + ", " + Where + ")", Seed, Step);
			}
		}
		for (const auto& Owned : std::map<std::string, double>(S.Gear.begin(), S.Gear.end()))
		{
			if (sponsor::IsProperty(B, Owned.first, World) && S.PawnSell("gear:" + Owned.first).empty())
			{
				Fail("segment: sponsor property sold at the pawn (" + Owned.first + ", " + Where + ")", Seed, Step);
			}
		}
		for (const sponsor::Contract& C : B.Contracts)
		{
			if (C.St == sponsor::State::Offered && C.ExpiresAt > World &&
				std::none_of(S.Life.Inbox.begin(), S.Life.Inbox.end(), [&C](const ss::life::Letter& L) { return L.Thread == C.Id; }))
			{
				Fail("segment: an open offer dropped from Mail (" + C.Id + ", " + Where + ")", Seed, Step);
			}
		}
	}
};

/**
 * The rival sites' books (update 2 SPEC §1.2, §6.7; C3), audited after every step beside SegBooks:
 *   - each wallet moves only by its own lines;
 *   - deposits out of the bank equal the wallets' deposit lines, and the bank's card fees are the card's rate on each;
 *   - withdrawals asked for equal what's in transit plus what landed plus the fees taken, to the cent;
 *   - a transfer lands once (one bank line each); no month's RiverLine Rewards or Rerun payout is paid twice;
 *   - an invitation not yet answered is never dropped from Mail.
 */
struct SegSites
{
	struct Wallet
	{
		ss::Chips Cents = 0;
		std::vector<ss::site::Line> Lines;
	};
	std::map<int, Wallet> Wallets;
	std::map<std::string, int> Xfers;
	ss::Chips DepositOut = 0, DepositIn = 0, Fees = 0, FeesDue = 0, Asked = 0, Landed = 0, LandedFees = 0;
	int Arrivals = 0, LandedXfers = 0;
	std::set<std::string> Months;

	static bool Same(const ss::site::Line& A, const ss::site::Line& B) { return A.At == B.At && A.Kind == B.Kind && A.Cents == B.Cents && A.Label == B.Label; }
	void Audit(const ss::Session& S, const std::vector<ss::life::LedgerEntry>& NewLines, int Seed, int Step, const std::string& Where)
	{
		namespace net = ss::net;
		namespace site = ss::site;
		for (const ss::life::LedgerEntry& E : NewLines)
		{
			if (E.Kind == 7 && E.Amount < 0 && E.Label.rfind("Deposit: ", 0) == 0)
			{
				DepositOut -= E.Amount;
				for (const net::SiteDef& D : net::Sites())
				{
					for (const net::CashMethod& M : D.Cashier)
					{
						const bool Card = D.Id != net::SiteId::RiverLine && E.Label == "Deposit: " + D.Name && M.Id == "card";
						FeesDue += Card ? static_cast<ss::Chips>(std::llround(static_cast<double>(-E.Amount) * static_cast<double>(M.FeePermille) / 1000.0)) : 0;
					}
				}
			}
			if (E.Kind == 7 && E.Amount > 0 && E.Label.rfind("Withdrawal: ", 0) == 0)
			{
				Landed += E.Amount;
				++Arrivals;
			}
			Fees -= E.Kind == ss::life::LedgerSites ? E.Amount : 0;
			const bool Monthly = (E.Kind == 4 && E.Label.rfind("RiverLine Rewards:", 0) == 0) || (E.Kind == ss::life::LedgerVideo && E.Label.rfind("Rerun payout:", 0) == 0);
			if (Monthly && !Months.insert(E.Label).second)
			{
				Fail("sites: a month paid twice (" + E.Label + ", " + Where + ")", Seed, Step);
			}
		}
		for (const site::Account& A : S.Life.Sites.Accounts)
		{
			Wallet& W = Wallets[static_cast<int>(A.Site)];
			size_t From = 0;
			for (size_t K = A.Lines.size(); !W.Lines.empty() && K-- > 0;)
			{
				if (Same(A.Lines[K], W.Lines.back()))
				{
					From = K + 1;
					break;
				}
			}
			ss::Chips Moved = 0;
			for (size_t K = From; K < A.Lines.size(); ++K)
			{
				const site::Line& L = A.Lines[K];
				Moved += L.Cents;
				DepositIn += L.Kind == 7 && L.Cents > 0 ? L.Cents : 0;
				Asked += L.Kind == 7 && L.Cents < 0 ? -L.Cents : 0;
			}
			if (A.Cents - W.Cents != Moved)
			{
				Fail("sites: a wallet moved without its lines (" + std::string(net::KeyOf(A.Site)) + " " + ss::Money(A.Cents - W.Cents) + " vs " + ss::Money(Moved) + ", " + Where + ")", Seed, Step);
			}
			if (A.Cents < 0)
			{
				Fail("sites: a wallet below zero (" + Where + ")", Seed, Step);
			}
			W.Cents = A.Cents;
			W.Lines = A.Lines;
		}
		ss::Chips Transit = 0;
		for (const site::Transfer& X : S.Life.Sites.Transfers)
		{
			const auto It = Xfers.find(X.Id);
			if (X.State == 1 && (It == Xfers.end() || It->second == 0))
			{
				++LandedXfers;
				LandedFees += X.FeeCents;
			}
			Transit += X.State == 0 && X.Out ? X.Cents : 0;
			Xfers[X.Id] = X.State;
		}
		if (DepositOut != DepositIn || Fees != FeesDue)
		{
			Fail("sites: deposits or their fees don't add up (" + ss::Money(DepositOut) + " out, " + ss::Money(DepositIn) + " in; fees " + ss::Money(Fees) + " vs " + ss::Money(FeesDue) + ", " +
					Where + ")",
				Seed, Step);
		}
		if (Asked != Transit + Landed + LandedFees || Arrivals != LandedXfers)
		{
			Fail("sites: withdrawals don't add up (" + ss::Money(Asked) + " asked, " + ss::Money(Transit) + " in transit, " + ss::Money(Landed) + " landed, " + ss::Money(LandedFees) +
					" in fees; " + std::to_string(Arrivals) + " bank lines for " + std::to_string(LandedXfers) + " landed, " + Where + ")",
				Seed, Step);
		}
		for (const site::Invitation& I : S.Life.Sites.Invites)
		{
			const std::string Thread = net::KeyOf(I.Site);
			if ((I.State == 0 || I.State == 3) &&
				std::none_of(S.Life.Inbox.begin(), S.Life.Inbox.end(), [&Thread](const ss::life::Letter& L) { return L.Source == "site" && L.Thread == Thread && L.Kind == ss::site::LetterInvite; }))
			{
				Fail("sites: an unanswered invitation dropped from Mail (" + Thread + ", " + Where + ")", Seed, Step);
			}
		}
	}
};

/** The bytes a save's lines of the given tags take. */
size_t LineBytes(const std::string& Text, std::initializer_list<const char*> Tags)
{
	size_t Bytes = 0;
	size_t Start = 0;
	while (Start < Text.size())
	{
		size_t End = Text.find('\n', Start);
		End = End == std::string::npos ? Text.size() : End + 1;
		for (const char* Tag : Tags)
		{
			if (Text.compare(Start, std::strlen(Tag), Tag) == 0)
			{
				Bytes += End - Start;
				break;
			}
		}
		Start = End;
	}
	return Bytes;
}

/** A hand the plan skipped at the Embercrest, into its run (the card room host's sink). */
struct SegPlanSink : ss::live::SkipSink
{
	ss::autoplay::Run* R = nullptr;
	ss::style::HeroBrain* Brain = nullptr;
	const ss::autoplay::TableAuto* A = nullptr;
	void HandDone(const ss::Hand& H, int HeroSeat) override
	{
		ss::autoplay::NoteSkippedHand(*R, H, HeroSeat, 1.0);
		*Brain = ss::autoplay::BrainFor(*A, *R, 1.0, 1, nullptr, nullptr);
	}
	void Events(const std::vector<ss::TEvent>&) override {}
	void Checkpoint(int) override {}
};

/**
 * One career of Days days, its steps drawn from Seed: the segment's features at random, the books audited after each.
 * Returns the number of steps.
 */
int SegmentCareer(int Seed, int Days, std::map<std::string, int>& Reached)
{
	namespace autoplay = ss::autoplay;
	namespace life = ss::life;
	namespace live = ss::live;
	namespace sponsor = ss::sponsor;
	namespace stake = ss::stake;
	const double Day = ss::net::MinutesPerDay;
	SegHooks H;
	std::unique_ptr<ss::Session> S(new ss::Session(H, "monkey-segment-" + std::to_string(Seed)));
	ss::hero::Character Who;
	Who.FirstName = "Sam";
	Who.LastName = "Ortiz";
	Who.Created = true;
	S->NewCareer(Who);
	S->CurrentScreen = ss::Screen::Lobby;
	ss::Rng R("monkey-segment-rng-" + std::to_string(Seed));
	double Now = 0.0;
	auto Frames = [&](double Seconds) {
		for (double T = 0.0; T < Seconds; T += 1.0 / 30.0)
		{
			Now += 1.0 / 30.0;
			S->Update(Now);
		}
	};
	auto Fed = [&]() {
		S->Life.Hunger = 10.0;
		S->Life.Thirst = 10.0;
		S->Life.Energy = 90.0;
	};
	Frames(0.1);
	SegBooks B;
	B.Start(*S);
	SegSites Books;
	ss::Rng X("monkey-sites-rng-" + std::to_string(Seed)); // the rival sites' and Rerun's draws (update 2 C3), apart from the segment's
	ss::Rng Press("monkey-press-" + std::to_string(Seed)); // the register button pressed once or twice (update 3): its own draws
	const double End = S->WorldMinutes() + static_cast<double>(Days) * Day;
	int Step = 0;
	// An event at a table, any site's: the plan skips it (any style, the stops drawn), or today's Sprint plays it; the event's
	// graded decisions are exactly the player's own (no AI decision graded).
	auto PlayOnline = [&](const ss::LobbyEvent& L, ss::Rng& D, int At, const char* PlanTag, const char* SprintTag) {
		MonkeyRegister(*S, L, Press, Seed, At);
		if (S->CurrentScreen != ss::Screen::Table)
		{
			return;
		}
		S->SprintBudgetMs = 1.0e9;
		const bool Plan = D.Chance(0.7);
		if (Plan)
		{
			S->SetAutoplay(0, RandomPlan(D, true));
		}
		for (int F = 0; F < 400000 && S->CurrentScreen == ss::Screen::Table; ++F)
		{
			if (!Plan || (S->TableCount() > 0 && !S->Glance(0).Busted && !S->AutoPanel(0).On))
			{
				S->CurrentPace = ss::Pace::Sprint;
			}
			else if (S->TableCount() > 0 && !S->Glance(0).Busted && !S->AutoPanel(0).Skipping && !S->HasPrompt)
			{
				S->SkipAhead(0, autoplay::Target::Result);
			}
			Now += 1.0 / 30.0;
			S->Update(Now);
			if (S->HasPrompt && Now - S->Prompt.OpenedAt > 0.3)
			{
				S->HeroAct(D.Chance(0.5) ? ss::PlayerAction::Fold() : S->Prompt.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Call());
			}
		}
		// No AI decision graded: the event's graded decisions are exactly the player's own.
		const autoplay::Played* P = autoplay::PlayedFor(S->Life.Auto, L.Spec.Id);
		if (S->CurrentScreen == ss::Screen::Results && P && static_cast<size_t>(P->OwnDecisions) != S->LastResults.Grades.size())
		{
			Fail("segment: a graded decision that wasn't the player's (" + std::to_string(S->LastResults.Grades.size()) + " graded, " + std::to_string(P->OwnDecisions) + " own)", Seed, At);
		}
		Reached[Plan ? PlanTag : SprintTag]++;
		S->LeaveResults();
	};
	// The rival sites and Rerun (update 2 C3, SPEC §6.7): an invitation forced as the console does it and Mail's answer,
	// deposits and withdrawals (some refused: under a minimum, a method the site doesn't take, more than the balance), a
	// rival's event, an upload. Returns what it did.
	auto SiteAction = [&](int At) -> std::string {
		namespace net = ss::net;
		namespace site = ss::site;
		const net::SiteId Site = X.Chance(0.5) ? net::SiteId::Ignite : net::SiteId::RunItTwice;
		const ss::Chips* Wallet = site::WalletOf(S->Life.Sites, Site);
		const double Pick = X.Next();
		if (!Wallet)
		{
			// No account there yet: its invitation (sent as the console sends it), or Mail's answer to one that's waiting.
			const bool Waiting = std::any_of(S->Life.Sites.Invites.begin(), S->Life.Sites.Invites.end(),
				[Site](const site::Invitation& I) { return I.Site == Site && (I.State == 0 || I.State == 3); });
			if (!Waiting)
			{
				const std::string Said = S->DebugSite(std::string("invite ") + net::KeyOf(Site));
				Reached[Said.find("invitation is in Mail") != std::string::npos ? "sites:invited" : "sites:not invited"]++;
				return "an invitation";
			}
			if (Pick < 0.5)
			{
				Reached[S->AnswerInvite(Site, X.Chance(0.85)).empty() ? "sites:answered" : "sites:answer refused"]++;
				return "Mail's buttons";
			}
			// The player opens the letter and leaves it (Mail marks a thread read when it shows it), and Mail fills past its keep
			// with letters that want no answer: the trim (life::Post) must still keep the open invitation, whose Accept lives only
			// on it (§1.7, §5.4; the g3 audit's blocker 4). A career's own Mail never reaches the keep in 60 days.
			for (ss::life::Letter& L : S->Life.Inbox)
			{
				L.Read = L.Read || (L.Source == "site" && L.Thread == net::KeyOf(Site));
			}
			for (size_t K = 0; K <= ss::life::InboxKeep; ++K)
			{
				ss::life::Letter F;
				F.At = S->WorldMinutes();
				F.Thread = "monkey-flood";
				F.Source = "monkey";
				F.From = "Monkey";
				F.Subject = "Nothing to answer";
				F.Body = "A letter that wants no answer.";
				F.Read = K % 2 == 0;
				ss::life::Post(S->Life, F);
			}
			Reached["sites:mail opened and filled"]++;
			return "Mail opened and filled";
		}
		if (Pick < 0.30)
		{
			// Some refused: under the $10 minimum, a method the site doesn't take in, more than the bank holds.
			static const ss::Chips Amounts[] = {999, 1000, 2500, 5000, 10000, 500000};
			const std::string Why = S->Deposit(Site, Amounts[X.Int(6)], X.Chance(0.85) ? "card" : "bank");
			Reached[Why.empty() ? "sites:deposit" : "sites:deposit refused"]++;
			return "a deposit";
		}
		if (Pick < 0.55)
		{
			// Some refused: under the $20 minimum, a method the site doesn't pay out by, more than the wallet holds.
			const ss::Chips Has = *Wallet;
			const ss::Chips Amounts[] = {1999, 2000, Has / 2, Has, Has + 100};
			const std::string Why = S->Withdraw(Site, Amounts[X.Int(5)], X.Chance(0.85) ? "bank" : "card");
			Reached[Why.empty() ? "sites:withdraw" : "sites:withdraw refused"]++;
			return "a withdrawal";
		}
		if (Pick < 0.85)
		{
			const net::Network& Net = net::Site(Site);
			const double W = S->WorldMinutes();
			std::vector<ss::LobbyEvent> Open;
			for (const net::EventInstance& E : Net.Window(W - 120.0, W + 60.0))
			{
				const ss::LobbyEvent L = Net.Listing(E, nullptr, S->Unlocks());
				if (L.Joinable && L.Spec.Entrants <= 400 && S->CanAfford(L) && !S->IsPlaying(L.Spec.Id))
				{
					Open.push_back(L);
				}
			}
			if (!Open.empty())
			{
				PlayOnline(Open[static_cast<size_t>(X.Int(static_cast<int>(Open.size())))], X, At, "sites:rival event by the plan", "sites:rival event sprinted");
			}
			return "a rival's event";
		}
		const std::vector<ss::video::Candidate> Cands = S->VideoCandidates();
		if (!Cands.empty())
		{
			ss::video::Choice Ch;
			Ch.Candidates = {Cands[static_cast<size_t>(X.Int(static_cast<int>(Cands.size())))].Id};
			Ch.F = X.Chance(0.5) ? ss::video::Format::Short : ss::video::Format::Breakdown;
			Fed();
			const std::string Why = S->Publish(Ch);
			for (int Guard = 0; Why.empty() && S->TimeSkip.Active && Guard < 200; ++Guard)
			{
				Frames(0.5);
			}
			Reached[Why.empty() ? "sites:upload" : "sites:upload refused"]++;
		}
		return "an upload";
	};
	static const char* const Backers[] = {"lou", "dee", "mei", "rick", "lanternrow", "sam"};
	for (; S->WorldMinutes() < End && Step < 4000; ++Step)
	{
		std::string Where;
		Fed();
		if (S->BankrollCents < 30000 && R.Chance(0.5))
		{
			B.Give(*S, 50000);
		}
		const double Pick = R.Next();
		const double World = S->WorldMinutes();
		if (Pick < 0.40)
		{
			// Time passes: an hour to most of a day (the calendar runs: rent, stipends, chops, reviews, offers).
			S->LobbyMinutes += 60.0 + R.Next() * 15.0 * 60.0;
			Frames(0.05);
			if (S->Evicted() && S->BankrollCents >= S->Life.RentDueCents + 20000)
			{
				S->MoveBackIn();
			}
			Where = "time";
		}
		else if (Pick < 0.48)
		{
			const std::string Said = S->DebugStake(std::string("offer ") + Backers[R.Int(6)]);
			Reached[Said.rfind("Offer ", 0) == 0 ? "seg:stake offered" : "seg:stake refused"]++;
			Where = "a stake offered";
		}
		else if (Pick < 0.56)
		{
			// The stake book: an offer taken or countered, a debt paid, a chop asked for, walking away.
			const std::vector<stake::Offer> Offers = S->Life.Stakes.Offers;
			const std::vector<stake::Deal> Deals = S->Life.Stakes.Deals;
			const double Act = R.Next();
			if (!Offers.empty() && Act < 0.55)
			{
				const stake::Offer& O = Offers[static_cast<size_t>(R.Int(static_cast<int>(Offers.size())))];
				Reached[(R.Chance(0.8) ? S->AcceptStake(O.Id) : S->CounterStake(O.Id, R.Int(4))).empty() ? "seg:stake taken" : "seg:stake not taken"]++;
			}
			else if (!Deals.empty() && Act < 0.75)
			{
				const stake::Deal& D = Deals[static_cast<size_t>(R.Int(static_cast<int>(Deals.size())))];
				Reached[(R.Chance(0.7) ? S->RequestChop(D.Id) : S->WalkAway(D.Id)).empty() ? "seg:deal chopped or left" : "seg:deal kept"]++;
			}
			else
			{
				Reached[S->DebugStake("pay").rfind("Paid ", 0) == 0 ? "seg:debt paid" : "seg:nothing owed"]++;
			}
			Where = "the stake book";
		}
		else if (Pick < 0.60)
		{
			// Pieces of an event at the Embercrest, listed or called off.
			const std::vector<live::Occurrence> Ahead = live::Reachable(World, 48.0);
			if (!Ahead.empty())
			{
				const live::Occurrence& O = Ahead[static_cast<size_t>(R.Int(static_cast<int>(Ahead.size())))];
				static const int Marks[] = {1000, 1100, 1250};
				const std::string Said = R.Chance(0.75) ? S->ListAction(O.Id, 50 * (1 + R.Int(3)), Marks[R.Int(3)]) : S->CancelListing(O.Id);
				Reached[Said.empty() ? "seg:pieces listed or called off" : "seg:pieces refused"]++;
			}
			Where = "pieces";
		}
		else if (Pick < 0.68)
		{
			// A sponsor's offer, made as the brand makes it; signed, countered or declined.
			const std::vector<sponsor::Brand>& Brands = sponsor::Brands();
			const sponsor::Brand& Br = Brands[static_cast<size_t>(R.Int(static_cast<int>(Brands.size())))];
			S->DebugSponsor("offer " + Br.Id);
			const std::string Id = sponsor::OpenOfferFor(S->Life.Sponsors, Br.Id, S->WorldMinutes());
			if (!Id.empty())
			{
				const double Act = R.Next();
				const std::string Said = Act < 0.6 ? S->SignDeal(Id) : Act < 0.8 ? S->CounterDeal(Id, R.Int(5)) : std::string("declined");
				if (Act >= 0.8)
				{
					S->DeclineDeal(Id);
				}
				Reached[Said.empty() ? "seg:sponsor signed or countered" : "seg:sponsor not signed"]++;
			}
			Where = "a sponsor's offer";
		}
		else if (Pick < 0.74)
		{
			// The sponsor book: a patch sewn or unpicked, notice, a raise asked for, an appearance in its window.
			std::vector<std::string> Running;
			for (const sponsor::Contract& C : S->Life.Sponsors.Contracts)
			{
				if (sponsor::Running(S->Life.Sponsors, C.Brand, World) == &C)
				{
					Running.push_back(C.Id);
				}
			}
			const double Act = R.Next();
			if (Act < 0.4)
			{
				const std::vector<std::string> Can = sponsor::PatchesFor(S->Life.Sponsors, R.Int(sponsor::PatchSpots), World);
				const int Spot = R.Int(sponsor::PatchSpots);
				const std::string Brand = Can.empty() || R.Chance(0.2) ? std::string() : Can[static_cast<size_t>(R.Int(static_cast<int>(Can.size())))];
				Reached[S->SewPatch(Spot, Brand).empty() ? "seg:patch sewn" : "seg:patch refused"]++;
			}
			else if (!Running.empty() && Act < 0.6)
			{
				const std::string& Id = Running[static_cast<size_t>(R.Int(static_cast<int>(Running.size())))];
				Reached[(R.Chance(0.5) ? S->GiveNotice(Id) : S->AskForRaise(Id)).empty() ? "seg:notice or raise" : "seg:notice or raise refused"]++;
			}
			else
			{
				for (const sponsor::Window& W : sponsor::Appearances(S->Life.Sponsors, World, World + 60.0))
				{
					if (World >= W.From && World < W.To - 25.0 && S->Appear(W.Venue).empty())
					{
						Frames(4.0);
						Reached["seg:appearance"]++;
						break;
					}
				}
			}
			Where = "the sponsor book";
		}
		else if (Pick < 0.77)
		{
			// The pawn: whatever gear the player has (sponsor property must be refused: the audit checks it every step).
			if (!S->Gear.empty())
			{
				auto It = S->Gear.begin();
				std::advance(It, R.Int(static_cast<int>(S->Gear.size())));
				Reached[S->PawnSell("gear:" + It->first).empty() ? "seg:pawned" : "seg:pawn refused"]++;
			}
			else if (R.Chance(0.5))
			{
				const std::vector<ss::gear::Item>& Cat = ss::gear::Catalog();
				const ss::gear::Item& It = Cat[static_cast<size_t>(R.Int(static_cast<int>(Cat.size())))];
				Reached[S->Buy(It.Id).empty() ? "seg:bought" : "seg:not bought"]++;
			}
			Where = "the pawn";
		}
		else if (Pick < 0.90)
		{
			// An online event: the plan skips it (any style, the stops drawn), or today's Sprint plays it.
			const ss::net::Network& Net = ss::net::Shared();
			std::vector<ss::LobbyEvent> Open;
			for (const ss::net::EventInstance& E : Net.Window(World - 120.0, World + 60.0))
			{
				const ss::LobbyEvent L = Net.Listing(E, nullptr, S->Unlocks());
				if (L.Joinable && L.Spec.Entrants <= 400 && S->CanAfford(L) && !S->IsPlaying(L.Spec.Id))
				{
					Open.push_back(L);
				}
			}
			if (!Open.empty())
			{
				PlayOnline(Open[static_cast<size_t>(R.Int(static_cast<int>(Open.size())))], R, Step, "seg:online by the plan", "seg:online sprinted");
			}
			Where = "an online event";
		}
		else
		{
			// A night at the Embercrest: the host's side (the desk; the engine or the plan plays it, or it's called off).
			const std::vector<live::Occurrence> Ahead = live::Reachable(World, 30.0);
			if (!Ahead.empty())
			{
				const live::Occurrence O = Ahead[static_cast<size_t>(R.Int(static_cast<int>(Ahead.size())))];
				S->LobbyMinutes += std::max(0.0, O.DeskOpens + 30.0 - S->WorldMinutes());
				Frames(0.05);
				B.Audit(*S, Seed, Step, "to the desk");
				Books.Audit(*S, B.NewLines, Seed, Step, "to the desk");
				Fed();
				if (S->GoToLive(O.Id).empty())
				{
					S->Save();
					ss::SaveData D = H.Last;
					D.Life = S->Life;
					D.BankrollCents = S->BankrollCents;
					if (D.WorldText.empty() && D.WorldSnapshot)
					{
						D.WorldSnapshot->Write(D.WorldText);
					}
					life::LiveEntry* E = live::EntryFor(D.Life, O.Id);
					double Ends = O.Start + 5.0;
					if (E && R.Chance(0.1))
					{
						live::Refund(D.BankrollCents, D.Life, O.Id, O.Start + 2.0, "not enough players");
						live::PayFare(D.BankrollCents, D.Life, O.Id, true, Ends);
						Reached["seg:night called off"]++;
					}
					else if (E)
					{
						E->ArrivedAt = O.Start - 15.0;
						std::vector<ss::ReservedPlayer> Known;
						for (const std::pair<std::string, int>& Rs : E->Roster)
						{
							ss::ReservedPlayer P;
							P.Name = Rs.first;
							P.Type = static_cast<ss::Archetype>(Rs.second);
							Known.push_back(P);
						}
						bool Restored = false;
						std::unique_ptr<ss::Tournament> T = live::ResumeField(O, *E, E->Entrants, D.HeroName, Known, false, Restored);
						const bool Plan = R.Chance(0.5);
						autoplay::TableAuto A = RandomPlan(R, true);
						A.To = autoplay::Target::Result;
						autoplay::Run Run;
						if (Plan)
						{
							ss::style::HeroBrain Brain = autoplay::BrainFor(A, Run, 1.0, 1, nullptr, nullptr);
							SegPlanSink Sink;
							Sink.R = &Run;
							Sink.Brain = &Brain;
							Sink.A = &A;
							for (int Guard = 0; Guard < 200 && !T->Hero().Busted && !T->bFinished; ++Guard)
							{
								ss::live::SkipSlice(*T, Brain, A, 1.0e9, Sink);
							}
						}
						else
						{
							const ss::Profile Tag = ss::MakeProfile(ss::Archetype::Tag, T->R);
							for (int Guard = 0; Guard < 6000 && !T->Hero().Busted && !T->bFinished; ++Guard)
							{
								T->SimulateTick(&Tag);
							}
						}
						const int Place = T->Hero().Place > 0 ? T->Hero().Place : T->HeroRank();
						const ss::Chips Prize = T->PrizeFor(Place);
						Ends = O.Start + T->ClockMinutes() - static_cast<double>(O.T->StartMinute);
						live::Settle(D.BankrollCents, D.Life, O.Id, Place, T->Spec.Entrants, Prize, Ends);
						live::PayFare(D.BankrollCents, D.Life, O.Id, true, Ends + 5.0);
						if (Plan && (Run.AiHands > 0 || Run.AiDecisions > 0))
						{
							autoplay::Record(D.Life.Auto, O.Id, 1, 0, A, Run);
							D.Life.Update2LinesRead |= life::U2AutoPlayed | life::U2AutoPref;
						}
						D.NoteLive(Ends, O.Id, Place, Prize, T->Spec.Entrants, {});
						Reached[Plan ? "seg:night by the plan" : "seg:night by the engine"]++;
					}
					D.ClockMinutes = Ends + 30.0 - Day;
					// Through the save's text and back (update 2 C3, §1.2): what the sites keep (a transfer's state, the done keys,
					// Rewards' month) has to survive a save and a load, or a transfer lands twice; SegSites' books run on across it.
					ss::SaveData Loaded;
					if (ss::SaveData::Parse(D.Serialize(), Loaded))
					{
						D = Loaded;
					}
					else
					{
						Fail("segment: the night's save doesn't read back", Seed, Step);
					}
					S.reset();
					S.reset(new ss::Session(H, "monkey-segment-" + std::to_string(Seed), &D));
					S->CurrentScreen = ss::Screen::Lobby;
					S->ResumeCalendarFrom(Ends + 30.0);
					Frames(0.05);
				}
			}
			Where = "a night";
		}
		B.Audit(*S, Seed, Step, Where);
		Books.Audit(*S, B.NewLines, Seed, Step, Where);
		// Update 2 C3: the rival sites and Rerun beside the step, on their own draws (SiteAction), the books audited again.
		if (X.Chance(0.35))
		{
			Fed();
			const std::string Did = SiteAction(Step);
			B.Audit(*S, Seed, Step, Did);
			Books.Audit(*S, B.NewLines, Seed, Step, Did);
		}
	}
	Reached["sites:landed"] += Books.LandedXfers;
	S->Save();
	const std::string Text = H.Last.Serialize();
	const size_t Segment = LineBytes(Text, {"livefund\t", "livevoice\t", "liveroom\t", "livebadge\t", "liveleader\t", "livepatch\t", "mail\t", "stake\t", "coach\t", "sponsor\t", "badge\t",
										   "life\tshift\t", "life\tliveday\t"});
	const size_t Update2 = LineBytes(Text, {"autoplay\t", "video\t", "site\t"});
	ss::SaveData Back;
	if (!ss::SaveData::Parse(Text, Back) || Back.Serialize() != Text)
	{
		Fail("segment: the save doesn't read back the same", Seed, Step);
	}
	if (Segment > 80 * 1024 || Update2 > 60 * 1024)
	{
		Fail("segment: the save's lines grew too big (" + std::to_string(Segment) + " segment, " + std::to_string(Update2) + " update 2)", Seed, Step);
	}
	std::printf("segment career %d: %d days, %d steps, bankroll %s, %zu results online, %d live, %zu stakes, %zu contracts, %zu letters, at most %d ledger lines a step, segment "
				"lines %zu bytes, update 2's %zu\n",
		Seed, Days, Step, ss::Money(S->BankrollCents).c_str(), S->History.size(), S->Life.LiveEvents, S->Life.Stakes.Deals.size(), S->Life.Sponsors.Contracts.size(),
		S->Life.Inbox.size(), B.MostLines, Segment, Update2);
	return Step;
}
} // namespace

int main(int Argc, char** Argv)
{
	const int Seeds = Argc > 1 ? std::atoi(Argv[1]) : 4;
	const int Frames = Argc > 2 ? std::atoi(Argv[2]) : 15000;
	const int First = Argc > 3 ? std::atoi(Argv[3]) : 0;
	const int Careers = Argc > 4 ? std::atoi(Argv[4]) : 1;
	const int CareerDays = Argc > 5 ? std::atoi(Argv[5]) : 60;
	const std::vector<Spot> Spots = Hotspots();
	const char* Keys[] = {"f", "c", "x", "r", "b", "a", "ArrowUp", "ArrowDown"};
	Measurer M;
	double WorstMs = 0.0;
	std::string WorstWhere;
	long long TotalFrames = 0;
	std::map<std::string, int> Reached;
	for (int Seed = First; Seed < First + Seeds; ++Seed)
	{
		Hooks H;
		ss::Session S(H, "monkey-" + std::to_string(Seed));
		ss::ui::RiverLine RL(S);
		ss::Rng R("monkey-rng-" + std::to_string(Seed));
		ss::Rng Dr(std::string("monkey-drop-") + std::to_string(Seed)); // Penny Drop's own draws, so the rest of the run is as before
		ss::Rng Sr(std::string("monkey-shops-") + std::to_string(Seed)); // Northside's shop screens: their own draws too
		ss::Rng Ar(std::string("monkey-auto-") + std::to_string(Seed));  // autoplay's (update 2): its own draws as well
		ss::Rng Pr(std::string("monkey-press-") + std::to_string(Seed)); // the register button's presses (update 3): its own too
		std::unique_ptr<ss::ui::ShopScreen> Shop;
		double ShopUntil = 0.0;
		if (Seed % 3 == 1)
		{
			S.BankrollCents = 5000;
		}
		if (Seed % 3 == 2)
		{
			S.BankrollCents = 60000;
			S.Life.Unlocks.insert("bounty");
			S.Life.Unlocks.insert("satellite");
			S.Life.Unlocks.insert("sixmax");
		}
		// The rich seeds have the screens for four tables; the others start on the laptop's two.
		if (Seed % 3 == 2)
		{
			S.BankrollCents += 14900 + 28900;
			S.Buy("monitor-24");
			S.Buy("monitor-27");
		}
		// The fourth seed plays it locked out: rent missed twice, the rig in storage, Dee's couch, and the money to move
		// back in (if the monkey finds the Bank's button).
		if (Seed % 4 == 3)
		{
			S.BankrollCents = 50000;
			S.Buy("ram-32");
			S.Buy("monitor-24");
			S.BankrollCents = 0;
			S.Update(0.0);
			S.LobbyMinutes += 8.0 * ss::net::MinutesPerDay + 1.0 - S.WorldMinutes();
			S.Update(0.0);
			if (!S.Evicted())
			{
				Fail("the evicted seed isn't evicted", Seed, 0);
			}
			S.BankrollCents = 250000;
		}
		double Now = 0.0;
		ss::Chips PrevBank = S.BankrollCents;
		std::vector<ss::life::LedgerEntry> PrevLedger = S.Life.Ledger;
		double PrevWorld = S.WorldMinutes();
		// The creator update (SPEC_CREATORS §9.3): Kast's pending money and the honest count, frame by frame.
		ss::Chips KastPending = S.Channel.PendingCents;
		int KastRents = S.Life.RentsPaid;
		int KastCount = 0;
		int KastChatId = -1;
		double KastWorld = S.WorldMinutes();
		bool Releasing = false;
		Spot Down{0, 0};
		std::map<const ss::Tournament*, std::pair<long long, double>> Progress; // signature, since
		for (int F = 0; F < Frames; ++F)
		{
			// Input.
			ss::ui::Pointer& P = RL.UI.Ptr;
			P.Pressed = false;
			P.Released = false;
			P.Wheel = 0.0f;
			P.Active = true;
			if (Releasing)
			{
				P.Down = false;
				P.Released = true;
				Releasing = false;
			}
			else
			{
				const double Roll = R.Next();
				if (Roll < 0.08)
				{
					const bool Hot = R.Chance(0.75);
					Down = Hot ? Spots[static_cast<size_t>(R.Int(static_cast<int>(Spots.size())))] : Spot{static_cast<float>(R.Next() * 1600.0), static_cast<float>(R.Next() * 1000.0)};
					Down.X += static_cast<float>(R.Next() * 8.0 - 4.0);
					Down.Y += static_cast<float>(R.Next() * 8.0 - 4.0);
					P.X = Down.X;
					P.Y = Down.Y;
					P.Down = true;
					P.Pressed = true;
					Releasing = true;
				}
				else if (Roll < 0.095)
				{
					RL.Key(Keys[R.Int(8)]);
				}
				else if (Roll < 0.10)
				{
					P.Wheel = static_cast<float>(R.Next() * 400.0 - 200.0);
				}
			}
			// Steering: now and then sit down in a random open event, or change a table's pace.
			// The last stretch winds the sitting down: no new tables, and the open ones sprint to their results.
			const bool Wrapping = F > Frames * 6 / 10;
			if (Wrapping && F % 300 == 0)
			{
				for (int I = 0; I < S.TableCount(); ++I)
				{
					S.WithTable(I, [&]() { S.CurrentPace = ss::Pace::Sprint; });
				}
			}
			if (!Wrapping && (S.CurrentScreen == ss::Screen::Lobby || S.CurrentScreen == ss::Screen::Table) && S.TableCount() < S.MaxTables() && R.Chance(0.003))
			{
				const ss::net::Network& Net = ss::net::Shared();
				const double W = S.WorldMinutes();
				std::vector<ss::LobbyEvent> Open;
				for (const ss::net::EventInstance& E : Net.Window(W - 240.0, W + 60.0))
				{
					const ss::LobbyEvent Lv = Net.Listing(E, nullptr, S.Unlocks());
					if (Lv.Joinable && S.CanAfford(Lv) && !S.IsPlaying(Lv.Spec.Id))
					{
						Open.push_back(Lv);
					}
				}
				if (!Open.empty())
				{
					MonkeyRegister(S, Open[static_cast<size_t>(R.Int(static_cast<int>(Open.size())))], Pr, Seed, F);
					++Reached["registered"];
				}
			}
			if (S.TableCount() > 0 && R.Chance(0.0008))
			{
				const int K = R.Int(S.TableCount());
				const ss::Pace Pc = static_cast<ss::Pace>(R.Int(3));
				S.WithTable(K, [&]() { S.CurrentPace = Pc; });
			}
			// Autoplay (update 2 C1): now and then a table's plan changes (a style, watch or skip or off, a speed, a target,
			// the stops), the player takes over, a skip starts or stops. Its own draws, so the clicks and keys are as before.
			if (!Wrapping && S.TableCount() > 0 && Ar.Chance(0.003))
			{
				const int K = Ar.Int(S.TableCount());
				const double Pick = Ar.Next();
				if (Pick < 0.45)
				{
					Reached[S.SetAutoplay(K, RandomPlan(Ar, false)).empty() ? "auto:plan" : "auto:plan refused"]++;
				}
				else if (Pick < 0.7)
				{
					Reached[S.TakeOver(K).empty() ? "auto:takeover" : "auto:takeover refused"]++;
				}
				else if (Pick < 0.9)
				{
					const ss::autoplay::TableAuto A = RandomPlan(Ar, true);
					Reached[S.SkipAhead(K, A.To, A.Hands).empty() ? "auto:skip" : "auto:skip refused"]++;
				}
				else
				{
					S.StopSkip(K);
					++Reached["auto:stop"];
				}
			}
			// Streaming: shop now and then (the PC upgrade first), go live and off again, work the studio.
			if (R.Chance(0.002))
			{
				// Only what the bankroll can spare (a quarter of it), so there's still money for buy-ins.
				const std::vector<ss::gear::Item>& Cat = ss::gear::Catalog();
				const ss::gear::Item& It = !S.GearFx().CanStream() && R.Chance(0.6) ? ss::gear::FirstPcUpgrade() : Cat[static_cast<size_t>(R.Int(static_cast<int>(Cat.size())))];
				if (It.PriceCents * 4 <= S.BankrollCents)
				{
					Reached["bought"] += S.Buy(It.Id).empty() ? 1 : 0;
				}
			}
			if (S.CurrentScreen != ss::Screen::Boot && R.Chance(0.0006))
			{
				const double Pick = R.Next();
				RL.OpenApp(Pick < 0.5 ? ss::ui::RiverLine::App::RiverLine : Pick < 0.75 ? ss::ui::RiverLine::App::Kast : ss::ui::RiverLine::App::GearDrop, Now);
			}
			if (S.CurrentScreen != ss::Screen::Boot && Dr.Chance(0.0002))
			{
				RL.OpenApp(ss::ui::RiverLine::App::PennyDrop, Now);
			}
			// Penny Drop: order now and then (what the bankroll can spare), eat and drink from the bag, the tap.
			if (Dr.Chance(0.0015))
			{
				ss::store::Basket B;
				const std::vector<ss::store::Item>& Cat = ss::store::Catalog();
				for (int K = 1 + Dr.Int(4); K > 0; --K)
				{
					B.Add(Cat[static_cast<size_t>(Dr.Int(static_cast<int>(Cat.size())))].Id);
				}
				if (ss::store::DeliveryTotal(B) * 6 <= S.BankrollCents)
				{
					Reached["ordered"] += S.PlaceOrder(B).empty() ? 1 : 0;
				}
			}
			if (!S.Life.Pantry.empty() && Dr.Chance(0.003))
			{
				auto It = S.Life.Pantry.begin();
				std::advance(It, Dr.Int(static_cast<int>(S.Life.Pantry.size())));
				Reached["ate"] += S.Consume(It->first).empty() ? 1 : 0;
			}
			if (Dr.Chance(0.002))
			{
				S.DrinkTapWater();
			}
			if (S.Life.Deliveries.size() > 3 || S.Life.Hunger < 0.0 || S.Life.Hunger > 100.0 || S.Life.Thirst < 0.0 || S.Life.Thirst > 100.0)
			{
				Fail("Penny Drop or the needs left their ranges", Seed, F);
			}
			for (const auto& Held : S.Life.Pantry)
			{
				if (Held.second <= 0 || !ss::store::Find(Held.first))
				{
					Fail("something impossible in the bag", Seed, F);
				}
			}
			for (const ss::life::State::Delivery& Dv : S.Life.Deliveries)
			{
				if (Dv.ArriveAt < Dv.PlacedAt + 25.0 || Dv.ArriveAt > Dv.PlacedAt + 60.0 || Dv.ArriveAt + 1.0 < S.WorldMinutes() - 1440.0)
				{
					Fail("a delivery that can't arrive", Seed, F);
				}
			}
			if (!S.GearFx().CanStream() && S.Streaming())
			{
				Fail("streaming without the PC upgrade", Seed, F);
			}
			if (!S.Streaming() && R.Chance(0.002))
			{
				const std::string Why = S.GoLive();
				if (Why.empty() != S.GearFx().CanStream() && Why != "You're busy." && Why != "No apartment, no internet.")
				{
					Fail("going live doesn't follow the PC upgrade", Seed, F);
				}
				Reached["live"] += Why.empty() ? 1 : 0;
			}
			if (S.Streaming())
			{
				++Reached["streaming"];
				const ss::kast::Stream& St = S.Stream;
				if (!(St.Viewers >= 0.0 && St.Viewers < 1e7) || St.Health < 0.0 || St.Health > 1.0 || St.Hype < 0.0 || St.Hype > 100.0 || S.Channel.UnpaidCents < 0)
				{
					Fail("the stream's numbers left their ranges", Seed, F);
				}
				if (R.Chance(0.003))
				{
					S.StreamAd(R.Chance(0.5) ? 60 : 180);
				}
				if (R.Chance(0.004))
				{
					S.StreamThank();
				}
				if (!St.Chat.empty() && R.Chance(0.004))
				{
					const ss::kast::ChatMsg& Msg = St.Chat[static_cast<size_t>(R.Int(static_cast<int>(St.Chat.size())))];
					const int Id = Msg.Id;
					if (R.Chance(0.5))
					{
						S.StreamAnswer(Id);
					}
					else
					{
						S.StreamTimeout(Id);
					}
				}
				if (R.Chance(0.001))
				{
					const std::vector<std::string> Cand = St.ModCandidates(S.Channel, S.WorldMinutes());
					if (!Cand.empty())
					{
						S.StreamPromote(Cand.front());
					}
				}
				if (R.Chance(0.0005))
				{
					// Half the time, end it the way communities like: a raid on a small channel.
					const std::vector<ss::kast::SmallChannel> Net = ss::kast::Network(S.WorldMinutes());
					const bool Raid = !Net.empty() && R.Chance(0.5);
					const std::string Target = Raid ? Net[static_cast<size_t>(R.Int(static_cast<int>(Net.size())))].Name : std::string();
					const int RaidsBefore = S.Channel.RaidsOut;
					const auto Tokens = [&S](const std::string& Name) {
						const auto It = S.Channel.Goodwill.find(Name);
						return It == S.Channel.Goodwill.end() ? 0 : It->second;
					};
					const int TokensBefore = Tokens(Target);
					S.EndStream(Target);
					++Reached[Raid ? "raided" : "ended"];
					// A raid out always lands; it gives the target a token only from a stream of 60+ world minutes, one a raid
					// (SPEC_CREATORS §2.7.1: no restart farming; a raid back during the stream's last minute may spend one).
					const int TokensAfter = Tokens(Target);
					if (Raid && (S.Stream.Last.RaidedOut != Target || S.Channel.RaidsOut != RaidsBefore + 1 || TokensAfter > TokensBefore + (S.Stream.Last.Minutes < 60.0 ? 0 : 1)))
					{
						Fail("a raid out didn't land", Seed, F);
					}
				}
			}
			// The LED kit: a colour, the power switch, sync.
			if (S.Owns(ss::gear::LedKitId) && R.Chance(0.0008))
			{
				const double Pick = R.Next();
				if (Pick < 0.6)
				{
					S.SetLedPreset(R.Int(ss::gear::LedPresetCount));
				}
				else if (Pick < 0.8)
				{
					S.SetLedsOn(!S.Leds.On);
				}
				else
				{
					S.SetLedSync(!S.Leds.Sync);
				}
				++Reached["leds"];
			}
			{
				const ss::gear::Glow G = S.RoomGlow(Now);
				const bool Should = S.Owns(ss::gear::LedKitId) && S.Leds.On && !S.Evicted();
				if (G.On != Should || G.On != S.GearFx().Leds || (G.On && (G.Level < 0.3 || G.Level > 2.0 || G.Preset != S.Leds.Preset)) || S.Leds.Preset < 0 || S.Leds.Preset >= ss::gear::LedPresetCount)
				{
					Fail("the room's LEDs don't match the kit and its settings", Seed, F);
				}
			}
			if (R.Chance(0.0003))
			{
				const int Days = R.Int(128);
				const int Start = R.Int(48) * 30;
				S.StreamSchedule(Days, Start);
				++Reached["schedule"];
			}
			{
				// The community stays in its ranges.
				const ss::kast::Channel& Ch = S.Channel;
				bool Bad = Ch.Members.size() > static_cast<size_t>(ss::kast::MaxMembers) || Ch.ScheduleDays < 0 || Ch.ScheduleDays > 127 || Ch.ScheduleStart < 0 || Ch.ScheduleStart >= 1440 ||
					Ch.Stage < 0 || Ch.Stage >= static_cast<int>(ss::kast::Stages().size()) || Ch.Xp < 0.0 || Ch.Followers < 0 || Ch.Superfans() > Ch.Regulars();
				for (const ss::kast::Member& Mb : Ch.Members)
				{
					Bad = Bad || !(Mb.Loyalty >= 0.0 && Mb.Loyalty <= 1.0) || !(Mb.Affinity >= 0.0 && Mb.Affinity <= 1.0) || Mb.Streams < 0;
				}
				if (Bad)
				{
					Fail("the community left its ranges", Seed, F);
				}
			}
			if (R.Chance(0.0005))
			{
				S.CashOut();
			}
			// The shops on the block (Northside's screens): now and then one opens on the session's clock as it is, takes
			// random keys and clicks for a few seconds, and is walked away from. What they charge goes through the ledger
			// like everything else, which the invariants below check.
			if (!Shop && S.CurrentScreen != ss::Screen::Boot && Sr.Chance(0.004))
			{
				static const char* const Venues[] = {"barber", "thrift", "gym", "diner", "pawn", "wardrobe"};
				static const char* const Opens[] = {"barber-chair1", "thrift-rack-outer", "gym-desk", "diner-stool2", "pawn-counter", "apartment"};
				const int V = Sr.Int(6);
				Shop = ss::ui::MakeShopScreen(Venues[V], S);
				ss::ui::HostCaps Caps;
				Caps.HairStyles.assign(12, true);
				Caps.FacialHair.assign(6, true);
				Caps.Brows.assign(4, true);
				Caps.GarmentKinds = ~0ull;
				Caps.GearKinds = ~0ull;
				Caps.Curios = Sr.Chance(0.5) ? 3u : 0u;
				Caps.Preview3D = Sr.Chance(0.5);
				Shop->SetCaps(Caps);
				Shop->Open(Now, Opens[V]);
				ShopUntil = Now + 2.0 + Sr.Next() * 6.0;
				++Reached[std::string("shop:") + Venues[V]];
			}
			if (Shop)
			{
				static const char* const ShopKeys[] = {"Up", "Down", "Left", "Right", "Enter", "Enter", "Tab", "Q", "E", "PutBack", "Alt"};
				if (Sr.Chance(0.2))
				{
					Shop->Key(ShopKeys[Sr.Int(11)], Now);
				}
				ss::ui::Pointer& Sp = Shop->Ptr;
				Sp.Pressed = Sr.Chance(0.02);
				Sp.Released = !Sp.Pressed && Sp.Down;
				Sp.Down = Sp.Pressed;
				Sp.Active = true;
				if (Sp.Pressed)
				{
					Sp.X = static_cast<float>(Sr.Next() * 1920.0);
					Sp.Y = static_cast<float>(Sr.Next() * 1080.0);
				}
				ss::ui::DrawList Ls;
				ss::ui::Canvas Cs(Ls, M, 1920.0f, ss::ui::ShopScreen::Height, 1.0f);
				Shop->Draw(Cs, Now);
				for (const ss::ui::HostRequest& Q : Shop->TakeRequests())
				{
					Reached["shop-service"] += Q.K == ss::ui::HostRequest::BeginService ? 1 : 0;
				}
				if (Shop->IsOpen() && Now > ShopUntil)
				{
					Shop->Key("Escape", Now);
					Shop->Key("Escape", Now);
					Shop->Key("Escape", Now);
				}
				if (!Shop->IsOpen())
				{
					Shop->TakeLeave();
					Shop.reset();
				}
			}
			// A frame.
			Now += 1.0 / 30.0;
			const auto T0 = std::chrono::steady_clock::now();
			S.Update(Now);
			ss::ui::DrawList L;
			ss::ui::Canvas C(L, M, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, 1.0f);
			RL.Draw(C, Now);
			const double Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count();
			++TotalFrames;
			const char* ScreenNames[] = {"boot", "lobby", "table", "results"};
			std::string Where = std::string(ScreenNames[static_cast<int>(S.CurrentScreen)]) + (S.TableCount() > 1 ? (S.Tiled ? "/tiles" : "/multi") : "") +
				"/app" + std::to_string(static_cast<int>(RL.CurrentApp())) + "/page" + std::to_string(static_cast<int>(RL.CurrentPage()));
			++Reached[std::string(ScreenNames[static_cast<int>(S.CurrentScreen)]) + (S.TableCount() > 1 ? "+" + std::to_string(S.TableCount()) + (S.Tiled ? "tiled" : "") : "")];
			if (S.TimeSkip.Active)
			{
				++Reached["timeskip"];
			}
			if (Ms > WorstMs && F > 5)
			{
				WorstMs = Ms;
				WorstWhere = Where + " seed " + std::to_string(Seed);
			}

			// Invariants.
			if (S.BankrollCents < 0)
			{
				Fail("negative bankroll", Seed, F);
			}
			{
				// New entries: the shortest prefix after which the ledger continues with last frame's.
				const std::vector<ss::life::LedgerEntry>& Cur = S.Life.Ledger;
				size_t Fresh = Cur.size();
				if (Cur.size() < 60 && Cur.size() >= PrevLedger.size())
				{
					Fresh = Cur.size() - PrevLedger.size(); // not full yet: exact
				}
				for (size_t Nn = 0; Cur.size() >= 60 && Nn < Cur.size() && !PrevLedger.empty(); ++Nn)
				{
					bool Match = true;
					for (size_t I = 0; Nn + I < Cur.size() && I < PrevLedger.size() && Match; ++I)
					{
						Match = SameEntry(Cur[Nn + I], PrevLedger[I]);
					}
					if (Match)
					{
						Fresh = Nn;
						break;
					}
				}
				ss::Chips Logged = 0;
				for (size_t I = 0; I < Fresh; ++I)
				{
					Logged += Cur[I].Amount;
				}
				if (S.BankrollCents - PrevBank != Logged)
				{
					Fail("bankroll moved without the ledger (" + ss::Money(S.BankrollCents - PrevBank) + " vs " + ss::Money(Logged) + ")", Seed, F);
				}
				if (Fresh > 20)
				{
					Fail("a single frame wrote more than 20 ledger lines (" + std::to_string(Fresh) + ")", Seed, F);
				}
				// The creator update (SPEC_CREATORS §9.3, C13, C14): Kast payouts conserve (the check above: every cent the bankroll
				// moves is a ledger line, the Kast payout kind 6 and the sponsors' LedgerSponsors lines apart), and the money held for
				// the second rent is paid exactly once: never below zero, released only with a Kast payout line that carries it, and
				// never building again off air once the second rent is paid (Offline holds renewals only before it).
				{
					ss::Chips Payouts = 0;
					for (size_t I = 0; I < Fresh; ++I)
					{
						Payouts += Cur[I].Kind == 6 && Cur[I].Amount > 0 ? Cur[I].Amount : 0;
					}
					const ss::Chips Pending = S.Channel.PendingCents;
					const ss::Chips Released = KastPending - Pending;
					if (Pending < 0 || (Released > 0 && Payouts < Released) || (KastRents >= 2 && !S.Streaming() && Pending > KastPending))
					{
						Fail("kast: the money held for the second rent was lost, paid without a payout line, or held again after the rent (" + ss::Money(KastPending) + " -> " +
								 ss::Money(Pending) + ", payouts " + ss::Money(Payouts) + ", rents " + std::to_string(S.Life.RentsPaid) + ")",
							Seed, F);
					}
					KastPending = Pending;
					KastRents = S.Life.RentsPaid;
				}
				PrevBank = S.BankrollCents;
				PrevLedger = S.Life.Ledger;
			}
			// The creator update (SPEC_CREATORS R1, §9.3): no named line lands while the count shows nobody. A viewer's line this
			// frame (not Kast's own system lines, not the streamer's) with the count at 0 when the frame began and when it ended is
			// a line from nobody; frames that ran a world minute or more (a skip, a sprint) are kast_growth's HonestCount's, where
			// someone can come, talk and go inside one frame.
			if (S.Streaming())
			{
				const ss::kast::Stream& Ks = S.Stream;
				const int CountNow = Ks.Count();
				const bool Short = S.WorldMinutes() - KastWorld < 1.0;
				int Newest = KastChatId;
				for (const ss::kast::ChatMsg& Line : Ks.Chat)
				{
					Newest = std::max(Newest, Line.Id);
					if (Line.Id > KastChatId && KastChatId >= 0 && Short && KastCount == 0 && CountNow == 0 && !Line.Who.empty() && Line.Kind != ss::kast::LineKind::System &&
						Line.Kind != ss::kast::LineKind::Streamer)
					{
						Fail("kast: a named line while the count showed nobody (" + Line.Who + ": " + Line.Text.substr(0, 40) + ")", Seed, F);
					}
				}
				KastChatId = Newest;
				KastCount = CountNow;
			}
			else
			{
				KastChatId = -1;
				KastCount = 0;
			}
			KastWorld = S.WorldMinutes();
			// The living world keeps up with the clock, and the network shows it.
			if (S.Living().Clock() + 1e-6 < S.WorldMinutes() - 60.0 || ss::net::Shared().Attached() != &S.Living())
			{
				Fail("the living world fell behind the clock or came loose from the network", Seed, F);
			}
			if (RL.PlayerShown() >= static_cast<int>(ss::net::Shared().Players().size()))
			{
				Fail("a player card for nobody", Seed, F);
			}
			const double World = S.WorldMinutes();
			if (World + 1e-6 < PrevWorld)
			{
				Fail("the clock ran backwards (" + ss::Fixed(PrevWorld - World, 3) + " min)", Seed, F);
			}
			PrevWorld = World;
			{
				// Rent: a stage in range, nothing left due behind the clock, and while evicted the rig in storage.
				const ss::life::State& Lf = S.Life;
				const int Stage = static_cast<int>(Lf.RentStage);
				const bool Out = S.Evicted();
				if (Stage < 0 || Stage > 3 || Lf.RentDueCents <= 0 || (!Out && Lf.RentDeadline <= World - 1e-6) || (!Out && Lf.StorageDue != 0.0) ||
					(Lf.StorageDue > 0.0 && Lf.StorageDue <= World - 1e-6))
				{
					Fail("the rent left its rules", Seed, F);
				}
				if (Out && (S.GearFx().CanStream() || S.GearFx().Tables != 2 || S.Streaming() || S.RestFactor() >= 1.0))
				{
					Fail("evicted, with the rig still set up", Seed, F);
				}
				Reached["evicted"] += Out ? 1 : 0;
			}
			if (S.CurrentScreen == ss::Screen::Table && !S.T)
			{
				Fail("table screen without a tournament", Seed, F);
			}
			if (S.CurrentScreen == ss::Screen::Results && !S.HasResults)
			{
				Fail("results screen without results", Seed, F);
			}
			if (S.TableCount() > ss::Session::TableLimit || (S.TableCount() > 0 && (S.FocusedTable() < 0 || S.FocusedTable() >= S.TableCount())))
			{
				Fail("table count or focus out of range", Seed, F);
			}
			if ((S.TableCount() > 0) != (S.T != nullptr) && S.CurrentScreen != ss::Screen::Results)
			{
				Fail("open tables and the front tournament disagree", Seed, F);
			}
			std::set<std::string> Ids;
			{
				// Forget tournaments that closed (their addresses get reused).
				std::set<const ss::Tournament*> Open;
				for (int I = 0; I < S.TableCount(); ++I)
				{
					S.WithTable(I, [&]() { Open.insert(S.T.get()); });
				}
				for (auto It = Progress.begin(); It != Progress.end();)
				{
					It = Open.count(It->first) ? std::next(It) : Progress.erase(It);
				}
			}
			for (int I = 0; I < S.TableCount(); ++I)
			{
				S.WithTable(I, [&]() {
					if (!S.T)
					{
						Fail("an open table without a tournament", Seed, F);
						return;
					}
					if (!Ids.insert(S.T->Spec.Id).second)
					{
						Fail("two tables in one event", Seed, F);
					}
					if (S.HasPrompt && (!S.CurHand || S.CurHand->ToAct != S.HeroSeatIdx()))
					{
						Fail("a prompt when it isn't the player's turn", Seed, F);
					}
					// No AI decision graded (update 2 §2.9): the table's graded decisions are exactly the player's own (both only
					// ever grow, so a look every 15 frames sees any that slipped in; the panel is too dear to build every frame).
					if (F % 15 == 0)
					{
						const ss::autoplay::Panel Ap = S.AutoPanel(I);
						if (S.Grades.size() != static_cast<size_t>(Ap.OwnDecisions))
						{
							Fail("a graded decision that wasn't the player's (" + std::to_string(S.Grades.size()) + " graded, " + std::to_string(Ap.OwnDecisions) + " own)", Seed, F);
						}
						Reached["auto:on"] += Ap.On ? 1 : 0;
						Reached["auto:skipping"] += Ap.Skipping ? 1 : 0;
					}
					ss::Chips Sum = 0;
					for (const ss::TPlayer* Pl : S.T->AlivePlayers())
					{
						Sum += Pl->Stack;
						if (Pl->Stack < 0)
						{
							Fail("negative stack", Seed, F);
						}
					}
					const ss::Chips Expected = static_cast<ss::Chips>(S.T->Players.size()) * S.T->Spec.StartingStack;
					if (Sum != Expected && !S.T->bFinished)
					{
						Fail("tournament chips not conserved", Seed, F);
					}
					for (const ss::SeatVis& V : S.Seats)
					{
						if (V.Present && (V.Stack < 0 || V.Bet < 0))
						{
							Fail("a seat shows negative chips", Seed, F);
						}
					}
					// Progress: a tournament that stops moving for two minutes of play is stuck.
					auto& Pr = Progress.emplace(S.T.get(), std::make_pair(-1LL, Now)).first->second;
					const long long Sig = static_cast<long long>(S.T->Tick) * 100000 + static_cast<long long>(S.HandProgress()) * 10 + (S.HasPrompt ? 1 : 0) +
						static_cast<long long>(S.Sprinting ? S.HandsPlayed : 0) * 1000000000LL;
					if (Pr.first != Sig || S.T->bFinished || S.Finishing() || S.CurrentScreen == ss::Screen::Results)
					{
						Pr = {Sig, Now};
					}
					else if (Now - Pr.second > 120.0)
					{
						std::string Hand = "none";
						if (S.CurHand)
						{
							Hand = std::string(S.CurHand->bComplete ? "complete" : "live") + " toact " + std::to_string(S.CurHand->ToAct) + " events " + std::to_string(S.CurHand->Events.size());
						}
						Fail("a table stopped moving (" + S.T->Spec.Name + ", screen " + std::to_string(static_cast<int>(S.CurrentScreen)) + ", table " + std::to_string(I) +
								", hand " + Hand + ", busted " + std::to_string(S.T->Hero().Busted) + ", left " + std::to_string(S.T->Remaining) + ", tables " +
								std::to_string(S.T->Tables.size()) + ", moving " + std::to_string(S.Moving) + ", start " + ss::Fixed(S.T->Spec.StartClock, 1) + ", clock " + ss::Fixed(S.T->ClockMinutes(), 1) + ")",
							Seed, F);
						Pr.second = Now;
					}
				});
			}
			if (F % 600 == 0 && H.HasSave)
			{
				ss::SaveData Back;
				if (!ss::SaveData::Parse(H.Last.Serialize(), Back) || Back.Serialize() != H.Last.Serialize())
				{
					Fail("a save doesn't survive a round trip", Seed, F);
				}
			}
		}
		std::printf("seed %d: %s, bankroll %s, %zu results, %d texts, rent %d paid, energy %.0f, %d tables open\n", Seed, ss::ClockString(S.ClockMinutes()).c_str(), ss::Money(S.BankrollCents).c_str(),
			S.History.size(), H.Texts, S.Life.RentsPaid, S.Life.Energy, S.TableCount());
	}
	// The segment's phase: careers stepped over Days days with the segment's features at random.
	const auto SegStart = std::chrono::steady_clock::now();
	int SegSteps = 0;
	for (int K = 0; K < Careers; ++K)
	{
		SegSteps += SegmentCareer(First + K, CareerDays, Reached);
	}
	std::printf("segment phase: %d careers, %d steps, %.1f s\n", Careers, SegSteps, std::chrono::duration<double>(std::chrono::steady_clock::now() - SegStart).count());
	std::printf("\nreached:");
	for (const auto& R : Reached)
	{
		std::printf(" %s=%d", R.first.c_str(), R.second);
	}
	std::printf("\nworst frame %.2f ms (%s), %lld frames\n", WorstMs, WorstWhere.c_str(), TotalFrames);
	if (Seeds >= 3 && Frames >= 15000 && (Reached["live"] == 0 || Reached["streaming"] == 0 || Reached["bought"] == 0))
	{
		++Failures;
		std::printf("FAIL: the monkey never bought gear and went live\n");
	}
	if (Seeds >= 3 && Frames >= 15000 && (Reached["table+4"] == 0 || Reached["results"] == 0 || Reached["registered"] == 0))
	{
		std::printf("FAIL: the monkey didn't reach four tables and a results screen\n");
		++Failures;
	}
	if (Seeds >= 3 && Frames >= 15000 && (Reached["auto:plan"] == 0 || Reached["auto:takeover"] == 0 || Reached["auto:skip"] == 0 || Reached["auto:on"] == 0 || Reached["auto:skipping"] == 0))
	{
		std::printf("FAIL: the monkey never handed a table to the plan, took over and skipped\n");
		++Failures;
	}
	if (Careers >= 1 && CareerDays >= 60 &&
		(Reached["seg:stake taken"] == 0 || Reached["seg:sponsor signed or countered"] == 0 || Reached["seg:online by the plan"] == 0 || Reached["seg:night by the plan"] == 0 ||
			Reached["seg:night by the engine"] == 0))
	{
		std::printf("FAIL: the segment's careers never took a stake, signed a sponsor, or played online and live\n");
		++Failures;
	}
	if (Careers >= 1 && CareerDays >= 60 &&
		(Reached["sites:deposit"] == 0 || Reached["sites:withdraw"] == 0 || Reached["sites:landed"] == 0 || Reached["sites:upload"] == 0 ||
			Reached["sites:rival event by the plan"] + Reached["sites:rival event sprinted"] == 0 || Reached["sites:mail opened and filled"] == 0))
	{
		std::printf("FAIL: the segment's careers never moved money to a site and back, played a rival's event, uploaded, or filled Mail past its keep with an "
					"invitation open\n");
		++Failures;
	}
	std::printf("%s: %d failures\n", Failures == 0 ? "monkey passed" : "MONKEY FAILED", Failures);
	return Failures == 0 ? 0 : 1;
}
