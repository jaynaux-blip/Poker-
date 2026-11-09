// Update 3 (SPEC §2.6 F7, §7.2): FUSE's Session glue, core-fuse's file. LIGHT IT goes through RegisterEvent (the core
// strike gate, the table cap, the wallet or the ticket) with the real fee as the rake override; the tier's ordinal and the
// record's entry side are advanced before the call, so RegisterEvent's own Save() is the commit before the reveal, and put
// back if it refuses. The reveal's state is the table's own spec (the multiplier) and the presentation clock at the light;
// the table's first hand waits until the reveal has been seen (HoldFirstDeal, G2 M3).
#include "ShortStack/Game/Session.h"

#include "ShortStack/Game/Autoplay.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Fuse.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/SitePlay.h"
#include "ShortStack/Game/Sites.h"

#include <array>
#include <iterator>
#include <map>
#include <mutex>
#include <string>

namespace ss
{
namespace session_fuse_detail
{
/** One open FUSE's reveal, by its event id. */
struct FuseLit
{
	double At = 0.0;       // the presentation clock (Session::Now) when LIGHT IT went through
	bool Seated = false;   // TAKE YOUR SEAT, or the table opening itself (fuse::TakeSeat)
	double ShownAt = -1.0; // the last time the laptop drew the reveal (fuse::RevealShown), -1 never
	double DealtAt = -1.0; // when its first deal went (HoldFirstDeal's first "deal"), -1 until then
};

/** A reveal the laptop drew this recently is still up (the Odds drawer can keep it past its run); one it stopped drawing
 *  (the laptop shut mid-reveal) goes by the clock alone. A second covers a frame's hitch. */
constexpr double FuseShownFor = 1.0;

/**
 * What a session keeps for FUSE beyond the save (Session.h is frozen, so it lives here, keyed by the session): each open
 * FUSE's reveal (when it was lit, whether the seat was taken, when the first deal went) and a dev-forced next multiplier per
 * tier. Never saved: a reload drops the tables, and the ordinal it already committed stands.
 */
struct FuseSessionSide
{
	std::array<int, fuse::TierCount> Forced{};
	std::map<std::string, FuseLit> Open;
};

// Never destroyed: a session that outlives static destruction (a host's) can still Forget itself safely.
std::mutex& FuseSideLock()
{
	static std::mutex* const Lock = new std::mutex();
	return *Lock;
}

std::map<const Session*, FuseSessionSide>& FuseSides()
{
	static std::map<const Session*, FuseSessionSide>* const Sides = new std::map<const Session*, FuseSessionSide>();
	return *Sides;
}

/** The open FUSE with that event id in S's side state, or null (the caller holds FuseSideLock). */
FuseLit* FuseOpenOf(const Session& S, const std::string& EventId)
{
	const auto It = FuseSides().find(&S);
	if (It == FuseSides().end())
	{
		return nullptr;
	}
	const auto At = It->second.Open.find(EventId);
	return At == It->second.Open.end() ? nullptr : &At->second;
}

/** At most two FUSEs at once (§2.6 F7); they count toward the table cap too (RegisterEvent's own check). */
constexpr int FuseAtOnce = 2;
} // namespace session_fuse_detail

std::string Session::LightFuse(int Tier, bool Ticket, StrikeConfirm Confirm)
{
	namespace sfd = session_fuse_detail;
	if (Tier < 0 || Tier >= fuse::TierCount)
	{
		return "Pick a FUSE: $1.10, $3.30 or $5.50.";
	}
	if (!site::WalletOf(Life.Sites, net::SiteId::Ignite))
	{
		return "Open an account with Ignite first.";
	}
	// Two at once (RegisterEvent only knows MaxTables(), four with both monitors).
	int Lit = 0;
	for (int K = 0; K < TableCount(); ++K)
	{
		const bool Front = K == Active;
		const Tournament* Tt = Front ? T.get() : Runs[static_cast<size_t>(K)]->T.get();
		const bool Done = Front ? Closing : Runs[static_cast<size_t>(K)]->Closing;
		Lit += Tt && !Done && fuse::IsFuse(Tt->Spec.Id) ? 1 : 0;
	}
	if (Lit >= sfd::FuseAtOnce)
	{
		return "Two FUSEs at a time is the limit. Finish one first.";
	}
	// The tickets. RegisterEvent spends a ticket held under the entry's own id ("ignite.fuse<tier>") before any money,
	// whatever the caller meant, so a FUSE ticket lives under fuse::TicketId and only a ticket light moves one across.
	const std::string Held = fuse::TicketId(Tier);
	const std::string Slot = "ignite.fuse" + std::to_string(Tier);
	if (const int Stray = Life.TicketsFor(Slot))
	{
		// A ticket left under the entry's id (never by this file) goes where it belongs, so a paid light pays.
		Life.Tickets[Held] += Stray;
		Life.Tickets.erase(Slot);
	}
	if (Ticket && Life.TicketsFor(Held) <= 0)
	{
		return "There's no FUSE ticket at that stake on your account.";
	}
	// The draw: the tier's next ordinal, tickets counted (W4: a ticket FUSE advances it and earns nothing else).
	site::play::FuseRec& Rec = Life.SitePlay.Ig.Fuse;
	const site::play::FuseRec Before = Rec;
	const uint32_t BitsBefore = Life.Update3LinesRead;
	const int N = Rec.N[static_cast<size_t>(Tier)] + 1;
	int M = fuse::Draw(WorldSeed(), Tier, N);
	int Forced = 0;
	{
		std::lock_guard<std::mutex> Guard(sfd::FuseSideLock());
		const auto It = sfd::FuseSides().find(this);
		Forced = It == sfd::FuseSides().end() ? 0 : It->second.Forced[static_cast<size_t>(Tier)];
	}
	M = Forced > 0 ? Forced : M;
	const LobbyEvent Ev = fuse::EventFor(Tier, M, WorldMinutes(), N);
	// The commit point: the ordinal and the record's entry side go into the book now, so RegisterEvent's Save() (after the
	// debit, before the first hand) writes them before the reveal can show anything. The book exists from here (S10).
	fuse::NoteEntry(Rec, Tier, M, !Ticket);
	Life.Update3LinesRead |= life::U3SitePlay;
	if (Ticket)
	{
		if (--Life.Tickets[Held] <= 0)
		{
			Life.Tickets.erase(Held);
		}
		++Life.Tickets[Slot];
	}
	const std::string Why = RegisterEvent(Ev, Confirm, fuse::FeeOf(Tier));
	if (!Why.empty())
	{
		// Refused (the strike gate, the table cap, the wallet): nothing happened, so nothing stays.
		Rec = Before;
		Life.Update3LinesRead = BitsBefore;
		if (Ticket)
		{
			if (--Life.Tickets[Slot] <= 0)
			{
				Life.Tickets.erase(Slot);
			}
			++Life.Tickets[Held];
		}
		return Why;
	}
	{
		std::lock_guard<std::mutex> Guard(sfd::FuseSideLock());
		sfd::FuseSessionSide& Side = sfd::FuseSides()[this];
		Side.Forced[static_cast<size_t>(Tier)] = 0;
		// The reveal's clock for this table; ids of tables no longer open go (a session lights a few hundred a night at most).
		for (auto It = Side.Open.begin(); It != Side.Open.end();)
		{
			It = IsPlaying(It->first) ? std::next(It) : Side.Open.erase(It);
		}
		sfd::FuseLit Fresh;
		Fresh.At = Now;
		Side.Open[Ev.Spec.Id] = Fresh;
	}
	if (Forced > 0)
	{
		SystemLine("The multiplier on this FUSE was set by hand for a test.");
	}
	return std::string();
}

fuse::Reveal Session::FuseReveal(int Table) const
{
	namespace sfd = session_fuse_detail;
	const int K = Table < 0 ? Active : Table;
	const Tournament* Tt = nullptr;
	if (K >= 0 && K == Active)
	{
		Tt = T.get();
	}
	else if (K >= 0 && K < TableCount())
	{
		Tt = Runs[static_cast<size_t>(K)]->T.get();
	}
	fuse::Reveal Shown;
	if (!Tt || !fuse::IsFuse(Tt->Spec.Id) || Tt->Spec.BuyInCents <= 0)
	{
		return Shown;
	}
	Shown.Tier = fuse::TierOf(Tt->Spec.Id);
	Shown.M = static_cast<int>(Tt->Spec.GuaranteeCents / Tt->Spec.BuyInCents);
	bool Known = false;
	bool Seated = false;
	bool Drawn = false;
	{
		std::lock_guard<std::mutex> Guard(sfd::FuseSideLock());
		if (const sfd::FuseLit* Open = sfd::FuseOpenOf(*this, Tt->Spec.Id))
		{
			Known = true;
			Shown.PaidAt = Open->At;
			Seated = Open->Seated;
			Drawn = Open->ShownAt >= 0.0 && Now - Open->ShownAt < sfd::FuseShownFor;
		}
	}
	// A table this session didn't light (none should exist) has no reveal to play: Seen. Otherwise the seat taken (TAKE YOUR
	// SEAT, or the laptop opening the table itself) ends it; failing that, its run and a second (§2.4 D), unless the laptop
	// is still drawing it.
	Shown.Seen = !Known || Seated || (Now >= Shown.PaidAt + fuse::RevealSeconds(Shown.M) + 1.0 && !Drawn);
	return Shown;
}

namespace fuse
{
std::string DebugForce(Session& S, int Tier, int M)
{
	namespace sfd = session_fuse_detail;
	if (Tier < 0 || Tier >= TierCount)
	{
		return "fuse: the tier is 0, 1 or 2 ($1.10, $3.30, $5.50).";
	}
	bool OnTable = false;
	std::string Choices;
	for (const fuse::Row& Each : Table())
	{
		OnTable = OnTable || MOf(Each, Tier) == M;
		Choices += " x" + std::to_string(MOf(Each, Tier));
	}
	if (!OnTable)
	{
		return "fuse: x" + std::to_string(M) + " isn't on the " + Money(BuyInOf(Tier)) + " table (" + Choices.substr(1) + ").";
	}
	std::lock_guard<std::mutex> Guard(sfd::FuseSideLock());
	sfd::FuseSides()[&S].Forced[static_cast<size_t>(Tier)] = M;
	return std::string();
}

void Forget(const Session* S)
{
	namespace sfd = session_fuse_detail;
	std::lock_guard<std::mutex> Guard(sfd::FuseSideLock());
	sfd::FuseSides().erase(S);
}

void TakeSeat(Session& S, const std::string& EventId)
{
	namespace sfd = session_fuse_detail;
	std::lock_guard<std::mutex> Guard(sfd::FuseSideLock());
	if (sfd::FuseLit* Open = sfd::FuseOpenOf(S, EventId))
	{
		Open->Seated = true;
	}
}

void RevealShown(Session& S, const std::string& EventId)
{
	namespace sfd = session_fuse_detail;
	std::lock_guard<std::mutex> Guard(sfd::FuseSideLock());
	if (sfd::FuseLit* Open = sfd::FuseOpenOf(S, EventId))
	{
		Open->ShownAt = S.Now;
	}
}

bool HoldFirstDeal(const Session& S)
{
	namespace sfd = session_fuse_detail;
	// Only a FUSE's first hand, before anything of it is shown (TableStep asks while its first event waits).
	if (S.HandsPlayed != 1 || !S.T || !IsFuse(S.T->Spec.Id))
	{
		return false;
	}
	// A plan or a skip has the table (the player's clock never runs under it), so autoplay and skips go on as ever; the
	// player's own seat waits for the reveal.
	const int Front = S.FocusedTable();
	const bool Handed = S.Sprinting || S.CurrentPace == Pace::Sprint || S.Autoplay(Front).M != autoplay::Mode::Off;
	if (!Handed && !S.FuseReveal(Front).Seen)
	{
		return true;
	}
	std::lock_guard<std::mutex> Guard(sfd::FuseSideLock());
	sfd::FuseLit* Open = sfd::FuseOpenOf(S, S.T->Spec.Id);
	if (Open && Open->DealtAt < 0.0)
	{
		Open->DealtAt = S.Now;
	}
	return false;
}

double FirstDealAt(const Session& S, const std::string& EventId)
{
	namespace sfd = session_fuse_detail;
	std::lock_guard<std::mutex> Guard(sfd::FuseSideLock());
	const sfd::FuseLit* Open = sfd::FuseOpenOf(S, EventId);
	return Open ? Open->DealtAt : -1.0;
}
} // namespace fuse
} // namespace ss
