#include "ShortStack/Game/Session.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Coach.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Stake.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>

// The session's side of backing and coaching (next_segment_spec §2, §7.2): the Session.h block's backing calls, the
// calendar's StakeStep and the hook sites' seams (StakeEntered, StakeWorld, StakeConduct, StudyDone). core-stake.
//
// The session builds what backers can know (stake::View: the public record, the bonds the world keeps with the cast,
// their rolls, who the world has in the room's next events) and hands the book to stake's pure functions. People text
// the phone; Lantern Row mails; Sam keeps to the Burner. Money and memories reach the world once, through
// World::HeroMoney (core-world); until that lands, the book holds them.

namespace ss
{
namespace session_stake_detail
{
/** The people who back the player by name (§2.5), as the world knows them. */
const char* const Cast[] = {"Big Lou", "Mrs. Park", "Sal", "Dre", "Mei", "Rick"};

// World::HeroMoney and the memory kinds it takes are core-world's (§7.3). Found at compile time, so this file builds
// before and after they land: until then the flows wait in the book.
template <typename E>
auto KStaked(int) -> decltype(E::Staked)
{
	return E::Staked;
}
template <typename E>
E KStaked(long)
{
	return E::Count;
}
template <typename E>
auto KBoughtPiece(int) -> decltype(E::BoughtPiece)
{
	return E::BoughtPiece;
}
template <typename E>
E KBoughtPiece(long)
{
	return E::Count;
}
template <typename E>
auto KSwapped(int) -> decltype(E::Swapped)
{
	return E::Swapped;
}
template <typename E>
E KSwapped(long)
{
	return E::Count;
}
template <typename E>
auto KPaidUp(int) -> decltype(E::PaidUp)
{
	return E::PaidUp;
}
template <typename E>
E KPaidUp(long)
{
	return E::Count;
}
template <typename E>
auto KPaidLate(int) -> decltype(E::PaidLate)
{
	return E::PaidLate;
}
template <typename E>
E KPaidLate(long)
{
	return E::Count;
}
template <typename E>
auto KWelched(int) -> decltype(E::Welched)
{
	return E::Welched;
}
template <typename E>
E KWelched(long)
{
	return E::Count;
}
template <typename E>
auto KWalkedOut(int) -> decltype(E::WalkedOut)
{
	return E::WalkedOut;
}
template <typename E>
E KWalkedOut(long)
{
	return E::Count;
}

template <typename E>
E KindFor(int Memory)
{
	switch (static_cast<stake::Memory>(Memory))
	{
	case stake::Memory::Staked: return KStaked<E>(0);
	case stake::Memory::BoughtPiece: return KBoughtPiece<E>(0);
	case stake::Memory::Swapped: return KSwapped<E>(0);
	case stake::Memory::PaidUp: return KPaidUp<E>(0);
	case stake::Memory::PaidLate: return KPaidLate<E>(0);
	case stake::Memory::Welched: return KWelched<E>(0);
	case stake::Memory::WalkedOut: return KWalkedOut<E>(0);
	}
	return E::Count;
}

// The best form World::HeroMoney has: with Early (a payment inside a day of the result), without, or none yet.
template <typename W>
auto HeroMoney(W& Wd, int Npc, Chips Cents, int Memory, const std::string& Where, double At, bool Early, int)
	-> decltype(Wd.HeroMoney(Npc, Cents, KindFor<world::MemoryKind>(Memory), Where, At, Early), bool())
{
	const world::MemoryKind K = KindFor<world::MemoryKind>(Memory);
	if (K == world::MemoryKind::Count)
	{
		return false;
	}
	Wd.HeroMoney(Npc, Cents, K, Where, At, Early);
	return true;
}
template <typename W>
auto HeroMoney(W& Wd, int Npc, Chips Cents, int Memory, const std::string& Where, double At, bool, long)
	-> decltype(Wd.HeroMoney(Npc, Cents, KindFor<world::MemoryKind>(Memory), Where, At), bool())
{
	const world::MemoryKind K = KindFor<world::MemoryKind>(Memory);
	if (K == world::MemoryKind::Count)
	{
		return false;
	}
	Wd.HeroMoney(Npc, Cents, K, Where, At);
	return true;
}
template <typename W>
bool HeroMoney(W&, int, Chips, int, const std::string&, double, bool, ...)
{
	return false;
}

/** What backers can know (S1): the record, the cast's bonds and rolls, who the world has in the next few days' events. */
stake::View ViewOf(const Session& S, const rep::Public& Rec)
{
	stake::View V;
	V.Rec = &Rec;
	V.Bankroll = S.BankrollCents;
	V.World = S.WorldMinutes();
	V.Day = net::DayOf(V.World);
	const life::State& L = S.Life;
	V.RentDue = L.RentStage == life::Rent::Due || L.RentStage == life::Rent::FinalNotice ? L.RentDueCents : 0;
	V.RentDeadline = L.RentDeadline;
	V.Restricted = S.Restricted();
	V.SundayPlayed = std::any_of(L.LiveEntries.begin(), L.LiveEntries.end(), [](const life::LiveEntry& E) {
		const live::Occurrence O = live::FindOccurrence(E.Id);
		return E.State == life::LiveEntry::Finished && O.Valid() && std::string(O.T->Key) == "sunday";
	});
	const world::World& W = S.Living();
	if (!W.Ready())
	{
		return V;
	}
	for (const char* Name : Cast)
	{
		const int Id = W.Find(Name);
		if (Id < 0)
		{
			continue;
		}
		stake::Regard R;
		if (const world::Bond* Bd = W.BondWith(Id))
		{
			R.Trust = Bd->Trust;
			R.Respect = Bd->Respect;
			R.Resentment = Bd->Resentment;
			R.Familiarity = Bd->Familiarity;
			R.Known = !Bd->Label().empty();
			R.FinalTable = std::any_of(Bd->Memories.begin(), Bd->Memories.end(), [](const world::Memory& M) { return M.Kind == world::MemoryKind::FinalTable; });
		}
		V.Regards[Name] = R;
		if (const world::Npc* N = W.Get(Id))
		{
			V.Rolls[Name] = N->Bankroll;
		}
	}
	V.RoiRank = Rec.OnlineEvents >= 200 ? W.RoiRank(Rec.OnlineRoi) : 0.0;
	V.Cashes = W.HeroStats().Cashes + Rec.LiveCashes;
	// Who the world has down for the room's swap nights (the Sunday, the Big Stack, the Turbo) over the next three days.
	for (int Day = V.Day; Day <= V.Day + 3; ++Day)
	{
		for (const live::Occurrence& O : live::Occurrences(Day))
		{
			const std::string Key = O.T->Key;
			if (Key != "sunday" && Key != "bigstack" && Key != "turbo")
			{
				continue;
			}
			std::vector<std::string>& Names = V.Playing[O.Id];
			for (const int Npc : W.RegisteredNow(O.Id))
			{
				if (const world::Npc* N = W.Get(Npc))
				{
					Names.push_back(N->Name);
				}
			}
		}
	}
	// The world's regulars who'd back an online package (World::Backers applies the rule: regard, a roll of 400 buy-ins,
	// fewer than three horses).
	for (const int Id : W.Backers(3))
	{
		if (const world::Npc* N = W.Get(Id))
		{
			stake::Patron P;
			P.Name = N->Name;
			P.Roll = N->Bankroll;
			P.Horses = 0;
			P.Respects = true;
			V.Patrons.push_back(P);
		}
	}
	return V;
}

std::string Lower(std::string S)
{
	for (char& Ch : S)
	{
		if (Ch >= 'A' && Ch <= 'Z')
		{
			Ch = static_cast<char>(Ch - 'A' + 'a');
		}
	}
	return S;
}

std::vector<std::string> Words(const std::string& Args)
{
	std::vector<std::string> Out;
	std::istringstream In(Args);
	std::string W;
	while (In >> W)
	{
		Out.push_back(W);
	}
	return Out;
}
} // namespace session_stake_detail

// (No using-directive here: Unreal's unity builds would carry it into the files compiled after this one.)

std::string Session::AcceptStake(const std::string& OfferId)
{
	stake::Book& B = Life.Stakes;
	const auto It = std::find_if(B.Offers.begin(), B.Offers.end(), [&OfferId](const stake::Offer& O) { return O.Id == OfferId; });
	if (It == B.Offers.end())
	{
		return "That offer's gone.";
	}
	const stake::Offer Was = *It;
	const double World = WorldMinutes();
	const std::string Not = stake::Accept(B, BankrollCents, Life, Was, World);
	if (!Not.empty())
	{
		return Not;
	}
	// Their answer, the way they say things.
	const std::string From = stake::OfferFrom(Was);
	const std::string Said = stake::Speak(From, Was.K == stake::Kind::Swap ? "swap.yes" : "taken", Fnv1a(Was.Id), {{"Pct", Was.Permille % 10 == 0 ? std::to_string(Was.Permille / 10) + "%" : "5%"}});
	if (Was.By == stake::Outfit::Sam)
	{
		stake::ToBurner(B, World, Said);
	}
	else if (Was.By != stake::Outfit::LanternRow && !Said.empty())
	{
		Hooks.Text(From, Said);
	}
	Sound(SoundId::Click, 0.8);
	Save();
	return "";
}

std::string Session::CounterStake(const std::string& OfferId, int Lever)
{
	stake::Book& B = Life.Stakes;
	const auto It = std::find_if(B.Offers.begin(), B.Offers.end(), [&OfferId](const stake::Offer& O) { return O.Id == OfferId; });
	if (It == B.Offers.end())
	{
		return "That offer's gone.";
	}
	const stake::Offer Was = *It;
	const std::string Said = stake::Counter(B, BankrollCents, Life, OfferId, Lever, WorldMinutes());
	const auto After = std::find_if(B.Offers.begin(), B.Offers.end(), [&OfferId](const stake::Offer& O) { return O.Id == OfferId; });
	const bool Answered = After == B.Offers.end() || After->Counters != Was.Counters;
	if (!Answered)
	{
		return Said; // refused before it was sent (one a day, nothing to counter)
	}
	const std::string From = stake::OfferFrom(Was);
	if (Was.By == stake::Outfit::Sam)
	{
		stake::ToBurner(B, WorldMinutes(), Said);
	}
	else if (Was.By != stake::Outfit::LanternRow && !Said.empty())
	{
		Hooks.Text(From, Said); // (Lantern Row's answer is a letter: stake::Counter posted it)
	}
	Save();
	return "";
}

std::string Session::ListAction(const std::string& EventId, int Permille, int MarkupPermille)
{
	const rep::Public Rec = Record();
	const stake::View V = session_stake_detail::ViewOf(*this, Rec);
	const double World = WorldMinutes();
	const std::string Not = stake::List(Life.Stakes, V, EventId, Permille, MarkupPermille, World, 0.0);
	if (Not.empty())
	{
		Sound(SoundId::Click, 0.8);
		Save();
	}
	return Not;
}

std::string Session::CancelListing(const std::string& EventId)
{
	const std::string Not = stake::CancelListing(Life.Stakes, EventId, WorldMinutes());
	if (Not.empty())
	{
		Save();
	}
	return Not;
}

std::string Session::ProposeSwap(const std::string& EntryId, const std::string& Partner, int Permille)
{
	const rep::Public Rec = Record();
	const stake::View V = session_stake_detail::ViewOf(*this, Rec);
	const std::string Not = stake::ProposeSwap(Life.Stakes, V, EntryId, Partner, Permille);
	if (!Not.empty())
	{
		return Not;
	}
	const std::string Said = stake::Speak(Partner, "swap.yes", Fnv1a(EntryId + Partner), {{"Pct", std::to_string(Permille / 10) + "%"}});
	if (!Said.empty())
	{
		Hooks.Text(Partner, Said);
	}
	Save();
	return "";
}

std::string Session::PayOwed(const std::string& OwedId)
{
	stake::Book& B = Life.Stakes;
	const auto It = std::find_if(B.Owes.begin(), B.Owes.end(), [&OwedId](const stake::Owed& O) { return O.Id == OwedId; });
	const std::string Who = It == B.Owes.end() ? std::string() : It->Who;
	const bool Early = It != B.Owes.end() && It->State == stake::OwedOpen && WorldMinutes() <= It->Since + 24.0 * 60.0;
	const std::string Not = stake::Pay(B, BankrollCents, Life, OwedId, WorldMinutes());
	if (!Not.empty())
	{
		return Not;
	}
	if (Who != "Online buyers")
	{
		const std::string Said = stake::Speak(Who, Early ? "paid.early" : "paid", Fnv1a(OwedId));
		if (!Said.empty())
		{
			Hooks.Text(Who, Said);
		}
	}
	Sound(SoundId::Cash, 0.7);
	Save();
	return "";
}

std::string Session::RequestChop(const std::string& DealId)
{
	const std::string Not = stake::Chop(Life.Stakes, BankrollCents, Life, DealId, WorldMinutes());
	if (Not.empty())
	{
		Sound(SoundId::Cash, 0.8);
		Save();
	}
	return Not;
}

std::string Session::WalkAway(const std::string& DealId)
{
	const std::string Not = stake::WalkAway(Life.Stakes, BankrollCents, Life, DealId, WorldMinutes());
	if (Not.empty())
	{
		Save();
	}
	return Not;
}

std::string Session::BookCoach(const std::string& /*ActivityId*/)
{
	// Paid coaching on its own is P2 (§2.11): the coaches back you instead (Dee's piece, Fred's review with Lantern Row).
	return "Fred's review comes with a Lantern Row package. Nobody's selling lessons on their own yet.";
}

std::vector<std::pair<life::Funding, std::string>> Session::FundingOptions(const std::string& EventOrOccurrenceId) const
{
	const double World = WorldMinutes();
	const live::Occurrence O = live::FindOccurrence(EventOrOccurrenceId);
	if (O.Valid())
	{
		return stake::FundingOptions(Life, O, World);
	}
	net::EventInstance I;
	if (net::Shared().FindInstance(EventOrOccurrenceId, I))
	{
		return stake::OnlineFundingOptions(Life, EventOrOccurrenceId, net::Shared().TemplateOf(I).BuyInCents, World);
	}
	return {{life::Funding(), std::string()}};
}

void Session::StakeStep(double From, double To)
{
	stake::Book& B = Life.Stakes;
	std::vector<std::pair<std::string, std::string>> Texts;
	const size_t DoneBefore = B.Done.size();
	const Chips BankBefore = BankrollCents;
	rep::Public Rec;
	stake::View V;
	bool Viewed = false;
	// Who'd make an offer: asked every ten minutes of world time (the view reads the world; the calendar runs every frame).
	if (To - B.LookedAt >= 10.0 || B.LookedAt > To)
	{
		B.LookedAt = To;
		Rec = Record();
		V = session_stake_detail::ViewOf(*this, Rec);
		Viewed = true;
		for (const stake::Offer& O : stake::OffersFor(V, B, To))
		{
			B.Offers.push_back(O);
			stake::Announce(B, Life, O, To, Texts);
		}
	}
	// A package's period end reads the record (its review).
	if (!Viewed && std::any_of(B.Deals.begin(), B.Deals.end(), [To](const stake::Deal& D) { return D.K == stake::Kind::Package && D.State == stake::DealOpen && D.NextChop <= To; }))
	{
		Rec = Record();
		V = session_stake_detail::ViewOf(*this, Rec);
		Viewed = true;
	}
	stake::Step(B, BankrollCents, Life, From, To, Texts, Viewed ? &V : nullptr);
	for (const std::pair<std::string, std::string>& Tx : Texts)
	{
		if (Tx.first == "Sam")
		{
			stake::ToBurner(B, To, Tx.second);
		}
		else
		{
			Hooks.Text(Tx.first, Tx.second);
		}
	}
	if (!Texts.empty() || B.Done.size() != DoneBefore || BankrollCents != BankBefore)
	{
		Save();
	}
}

std::string Session::DebugStake(const std::string& Args)
{
	const std::vector<std::string> W = session_stake_detail::Words(Args);
	const std::string Cmd = W.empty() ? std::string("describe") : session_stake_detail::Lower(W[0]);
	stake::Book& B = Life.Stakes;
	const double World = WorldMinutes();
	std::ostringstream Out;
	if (Cmd == "describe")
	{
		Out << "bankroll " << Money(BankrollCents) << ", staked roll " << Money(stake::StakedRoll(B)) << ", makeup " << Money(stake::MakeupOf(B)) << ", you owe "
			<< Money(stake::YouOwe(B)) << ", owed to you " << Money(stake::OwedToYou(B)) << "\n";
		for (const stake::Offer& O : B.Offers)
		{
			Out << "offer " << O.Id << " from " << stake::OfferFrom(O) << ": " << stake::TermsLine(O) << "\n";
		}
		for (const stake::Deal& D : B.Deals)
		{
			Out << "deal " << D.Id << " (" << stake::BackerName(D) << ", state " << D.State << "): " << stake::DealLine(D) << ", roll " << Money(D.RollNow) << ", funded "
				<< Money(D.Funded) << "\n";
		}
		for (const stake::Listing& Ls : B.Listings)
		{
			Out << "listing " << Ls.EventId << ": " << Ls.Ask / 10 << "% at " << Ls.Markup << ", filled " << stake::FilledOn(B, Ls.EventId, World) / 10 << "%\n";
		}
		for (const stake::Swap& S : B.Swaps)
		{
			Out << "swap " << S.EntryId << " with " << S.Partner << " " << S.Permille / 10 << "% (state " << S.State << ")\n";
		}
		for (const stake::Owed& O : B.Owes)
		{
			Out << (O.ToYou ? "owed to you " : "you owe ") << O.Who << " " << Money(O.Cents) << " (" << O.Label << "), due " << net::DateLabel(net::DayOf(O.Due)) << " "
				<< net::TimeLabel(O.Due) << ", state " << O.State << "\n";
		}
		Out << "record: late " << B.Record.Late << ", welched " << B.Record.Welched << ", walked " << B.Record.Walked << ", shots " << B.Record.Shots << ", tilts "
			<< B.Record.Tilts << "\n";
		return Out.str();
	}
	if (Cmd == "advance")
	{
		if (T)
		{
			return "Not with tables open.";
		}
		const double Hours = W.size() > 1 ? std::max(0.0, std::strtod(W[1].c_str(), nullptr)) : 24.0;
		const double From = World;
		LobbyMinutes += Hours * 60.0;
		CheckCalendar(From, WorldMinutes(), true);
		CalendarAt = WorldMinutes();
		return "Advanced " + Fixed(Hours, 1) + " hours.";
	}
	if (Cmd == "offer" && W.size() > 1)
	{
		// The offer as its maker would make it, whatever the gates say.
		const std::string Who = session_stake_detail::Lower(W[1]);
		stake::Offer O;
		O.At = World;
		O.Expires = World + 3.0 * 1440.0;
		const char* Key = Who == "dee" ? "sunday" : Who == "mei" ? "sunday" : Who == "rick" ? "turbo" : "nightly";
		live::Occurrence Ev;
		for (int Day = net::DayOf(World); Day <= net::DayOf(World) + 7 && !Ev.Valid(); ++Day)
		{
			for (const live::Occurrence& C : live::Occurrences(Day))
			{
				if (std::string(C.T->Key) == Key && C.LateRegEnds > World + live::TravelMinutes)
				{
					Ev = C;
					break;
				}
			}
		}
		if (Who == "lou" || Who == "dee" || Who == "mei" || Who == "rick")
		{
			if (!Ev.Valid())
			{
				return "No event to offer on this week.";
			}
			O.EntryId = Ev.Id;
			O.BuyIn = Ev.T->BuyInCents;
			O.Expires = std::min(O.Expires, Ev.LateRegEnds);
		}
		std::map<std::string, std::string> Vars{{"Short", Ev.Valid() ? Ev.T->Short : "Nightly"}, {"Day", Ev.Valid() ? live::DayWord(Ev, World) : std::string("on Saturday")},
			{"Amount", Money(O.BuyIn)}, {"Pct", "10%"}};
		if (Who == "lou")
		{
			O.Id = "lou:" + Ev.Id + ":debug";
			O.K = stake::Kind::Event;
			O.Backer = "Big Lou";
			O.Line = stake::Speak("Big Lou", "offer", 0, Vars);
		}
		else if (Who == "dee")
		{
			O.Id = "dee:" + Ev.Id + ":debug";
			O.K = stake::Kind::Pieces;
			O.Backer = "Dee";
			O.Permille = 100;
			O.Line = stake::Speak("Dee", "offer", 0, Vars);
		}
		else if (Who == "mei" || Who == "rick")
		{
			O.Id = Who + ":" + Ev.Id + ":debug";
			O.K = stake::Kind::Swap;
			O.Backer = Who == "mei" ? "Mei" : "Rick";
			O.Permille = 100;
			O.Line = stake::Speak(O.Backer, "offer", 0, Vars);
		}
		else if (Who == "lanternrow" || Who == "sam")
		{
			O.Id = Who + ":debug:" + std::to_string(net::DayOf(World));
			O.K = stake::Kind::Package;
			if (Who == "sam")
			{
				O.By = stake::Outfit::Sam;
				O.Backer = "Sam";
				O.T.CutPermille = 350;
				O.T.CapOnline = 5500;
				O.T.CapLive = 12000;
				O.T.Roll = 150000;
				O.T.PeriodDays = 30;
				O.T.GhostsPerWeek = 2;
				O.T.MakeupIsDebt = true;
				O.T.BuyInBack = false;
				O.Line = stake::Speak("Sam", "offer", 1);
			}
			else
			{
				O.By = stake::Outfit::LanternRow;
				O.Backer = "Lantern Row Staking";
				O.Tier = 2;
				O.T.CutPermille = 450;
				O.T.CapOnline = 2200;
				O.T.Roll = 220000;
				O.T.PeriodDays = 30;
				O.T.Volume = 120;
				O.T.BuyInBack = false;
				O.Expires = World + 7.0 * 1440.0;
			}
			O.BuyIn = O.T.CapOnline;
		}
		else
		{
			return "ss.Stake.Offer lou|dee|mei|rick|lanternrow|sam";
		}
		B.Offers.erase(std::remove_if(B.Offers.begin(), B.Offers.end(), [&O](const stake::Offer& X) { return X.Id == O.Id; }), B.Offers.end());
		B.Done.erase("offer:" + O.Id);
		B.Done.erase("said:" + O.Id);
		B.Offers.push_back(O);
		std::vector<std::pair<std::string, std::string>> Texts;
		stake::Announce(B, Life, O, World, Texts);
		for (const std::pair<std::string, std::string>& Tx : Texts)
		{
			Hooks.Text(Tx.first, Tx.second);
		}
		Save();
		return "Offer " + O.Id + ": " + stake::TermsLine(O);
	}
	if (Cmd == "grant" && W.size() > 1)
	{
		const std::string Who = session_stake_detail::Lower(W[1]);
		if (Who == "finch")
		{
			return "Finch School isn't taking students yet (P2).";
		}
		if (Who != "lanternrow" && Who != "sam")
		{
			return "ss.Stake.Grant lanternrow|sam|finch";
		}
		const std::string Made = DebugStake("offer " + Who);
		for (const stake::Offer& O : std::vector<stake::Offer>(B.Offers))
		{
			if (O.K == stake::Kind::Package && O.Id.find(":debug:") != std::string::npos)
			{
				const std::string Not = AcceptStake(O.Id);
				return Not.empty() ? "Granted: " + stake::TermsLine(O) : Not;
			}
		}
		return Made;
	}
	if (Cmd == "list" && W.size() > 2)
	{
		const int Pct = std::atoi(W[2].c_str());
		const int Markup = W.size() > 3 ? static_cast<int>(std::strtod(W[3].c_str(), nullptr) * 1000.0 + 0.5) : 1000;
		const std::string Not = ListAction(W[1], Pct * 10, Markup);
		return Not.empty() ? stake::Projection(B, W[1], World) : Not;
	}
	if (Cmd == "pay")
	{
		for (const stake::Owed& O : std::vector<stake::Owed>(B.Owes))
		{
			if (!O.ToYou && O.State != stake::OwedPaid)
			{
				const std::string Not = PayOwed(O.Id);
				return Not.empty() ? "Paid " + O.Who + " " + Money(O.Cents) + "." : Not;
			}
		}
		return "You don't owe anyone.";
	}
	return "ss.Stake.Describe | Offer <who> | Advance <hours> | Grant <lanternrow|sam|finch> | List <event> <pct> <markup> | Pay";
}

void Session::StakeEntered(const std::string& EventId, Chips BuyInCents, const life::Funding& F)
{
	const std::string Name = Joined.Spec.Id == EventId ? Joined.Spec.Name : EventId;
	const Chips Before = F.From == life::Paid::Package ? BankrollCents : BankrollCents + BuyInCents;
	stake::OnlineEntered(Life.Stakes, BankrollCents, Life, EventId, Name, BuyInCents, F, Before, HeroTilt, WorldMinutes());
}

void Session::StakeWorld()
{
	stake::Book& B = Life.Stakes;
	if (B.ToWorld.empty() && B.Swaps.empty())
	{
		return;
	}
	// Money and memories to the people, once each (World::HeroMoney, core-world's); the applied ones go at the next prune.
	for (stake::Flow& F : B.ToWorld)
	{
		if (F.Applied)
		{
			continue;
		}
		const int Npc = LivingWorld.Find(F.Npc);
		if (Npc < 0)
		{
			F.Applied = true; // nobody the world follows
			continue;
		}
		F.Applied = session_stake_detail::HeroMoney(LivingWorld, Npc, F.Cents, F.Memory, F.Where, F.At, F.Early, 0);
	}
	if (B.ToWorld.size() > 200)
	{
		B.ToWorld.erase(B.ToWorld.begin(), B.ToWorld.begin() + static_cast<std::ptrdiff_t>(B.ToWorld.size() - 200));
	}
	// Swaps whose partner's result the world now knows (World::FinishIn): settled once both are in; a partner who
	// never played it (the event resolved without them) lapses the swap.
	for (const stake::Swap& S : std::vector<stake::Swap>(B.Swaps))
	{
		if ((S.State != stake::SwapLive && S.State != stake::SwapAgreed) || S.Mine < 0 || S.Theirs >= 0)
		{
			continue;
		}
		const int Npc = LivingWorld.Find(S.Partner);
		int Place = 0;
		Chips Prize = 0;
		if (Npc >= 0 && LivingWorld.FinishIn(S.EntryId, Npc, Place, Prize))
		{
			stake::SettleSwap(B, BankrollCents, Life, S.EntryId, S.Partner, Prize, WorldMinutes());
		}
		else if (Npc < 0 || LivingWorld.ResultOf(S.EntryId) != nullptr)
		{
			stake::SettleSwap(B, BankrollCents, Life, S.EntryId, S.Partner, -1, WorldMinutes());
		}
	}
}

void Session::StakeConduct(sponsor::Breach Why)
{
	stake::NoteConduct(Life.Stakes, Why == sponsor::Breach::PickedUp ? stake::MarkPickedUp : stake::MarkRestricted, WorldMinutes());
}

void Session::StudyDone(const life::Activity& /*A*/, const life::Outcome& /*O*/)
{
	// Paid coaching sessions on their own are P2 (§2.11): nothing in the catalog is a study session yet.
}
} // namespace ss
