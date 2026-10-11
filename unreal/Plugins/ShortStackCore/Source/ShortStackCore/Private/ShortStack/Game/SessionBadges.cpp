#include "ShortStack/Game/Session.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Badges.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/UI/BadgeArt.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <sstream>

// The session's side of pro badges (next_segment_spec §5, §7.2): the player's badges, the ones not shown yet, progress,
// the calendar's BadgeStep (the monthly review; also once on load, §7.5) and the debug command. core-badges.

namespace ss
{
namespace session_badges_detail
{
constexpr double DayMinutes = 1440.0;
/** A debug grant the rules can't make from the save (a card, a Player of the Year, the Hall): TextsSeen keeps it. */
constexpr const char* GrantPrefix = "badge-grant:";

/** The month key of the latest review moment (the 1st, 6 AM) at or before World. */
int ReviewDue(double World)
{
	const int Day = net::DayOf(World);
	const int First = world::MonthStart(Day);
	const double At = static_cast<double>(First) * DayMinutes + badge::ReviewMinute;
	return World >= At ? world::MonthKeyOf(Day) : world::MonthKeyOf(First - 1);
}

uint64_t Mix(uint64_t H, uint64_t V)
{
	H ^= V + 0x9e3779b97f4a7c15ull + (H << 6) + (H >> 2);
	return H;
}

uint64_t MixText(uint64_t H, const std::string& S)
{
	for (const char Ch : S)
	{
		H = (H ^ static_cast<unsigned char>(Ch)) * 1099511628211ull;
	}
	return Mix(H, S.size());
}

uint64_t Bits(double V)
{
	uint64_t U = 0;
	std::memcpy(&U, &V, sizeof(U));
	return U;
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

std::string Lower(std::string S)
{
	for (char& Ch : S)
	{
		Ch = Ch >= 'A' && Ch <= 'Z' ? static_cast<char>(Ch - 'A' + 'a') : Ch;
	}
	return S;
}

/** A debug grant's kind by the name typed ("KastPartner", "club", "wall"...). */
badge::Kind KindNamed(const std::string& Word)
{
	const std::string W = Lower(Word);
	if (W == "hall" || W == "halloffame" || W == "hof")
	{
		return badge::Kind::HallOfFame;
	}
	if (W == "bracelet")
	{
		return badge::Kind::Bracelet;
	}
	if (W == "ring")
	{
		return badge::Kind::Ring;
	}
	if (W == "poy" || W == "playeroftheyear" || W == "playerofyear")
	{
		return badge::Kind::PlayerOfYear;
	}
	if (W == "livepoy" || W == "liveplayeroftheyear" || W == "liveplayerofyear")
	{
		return badge::Kind::LivePlayerOfYear;
	}
	if (W == "teamriverline" || W == "riverline" || W == "team")
	{
		return badge::Kind::TeamRiverLine;
	}
	if (W == "sponsored" || W == "sponsor" || W == "patch")
	{
		return badge::Kind::Sponsored;
	}
	if (W == "card" || W == "circuitcard" || W == "circuit")
	{
		return badge::Kind::CircuitCard;
	}
	if (W == "verified" || W == "verifiedpro" || W == "pro")
	{
		return badge::Kind::VerifiedPro;
	}
	if (W == "kastpartner" || W == "kast" || W == "partner")
	{
		return badge::Kind::KastPartner;
	}
	if (W == "wall" || W == "onthewall")
	{
		return badge::Kind::OnTheWall;
	}
	if (W == "club" || W == "playersclub")
	{
		return badge::Kind::Club;
	}
	return badge::Kind::Count;
}

std::string Describe(const badge::Set& S, const badge::Holder& H)
{
	std::string Out;
	if (S.Held.empty())
	{
		Out += "  no badges\n";
	}
	for (const badge::Badge& B : S.Held)
	{
		Out += "  " + badge::Label(B) + " (" + badge::Title(B) + "): " + badge::HowEarned(B, H) + "\n";
	}
	const badge::Badge* Net = badge::Top(S, badge::Network, badge::FrameNone);
	const badge::Badge* Room = badge::Top(S, badge::Room, badge::FrameNone);
	const badge::Badge* Stream = badge::Top(S, badge::Stream, badge::FrameNone);
	Out += "  rows: network " + (Net ? badge::Label(*Net) : std::string("-")) + ", room " + (Room ? badge::Label(*Room) : std::string("-")) + ", stream " +
		(Stream ? badge::Label(*Stream) : std::string("-")) + "\n";
	const std::string Hon = badge::Honorific(S, badge::Room);
	Out += "  honorific: " + (Hon.empty() ? std::string("-") : Hon) + "; caption: " + badge::Caption(S, badge::Room) + "\n";
	Out += "  club: " + std::to_string(H.ClubVisitsNow) + " in 90 days (" + std::to_string(H.ClubVisitsAtMonth) + " at the 1st), tier " +
		std::to_string(badge::ClubTier(H.ClubVisitsNow, H.ClubVisitsAtMonth)) + "; wall " + std::to_string(H.WallWins) + "; cards " + std::to_string(H.CardBest) + "/" +
		std::to_string(H.CardLast) + "\n";
	if (H.Stats)
	{
		const world::Tracker& T = *H.Stats;
		const Chips Abi = T.PaidAbi();
		Out += "  record: " + std::to_string(T.PaidEvents()) + " paid tournaments (" + std::to_string(T.Freerolls) + " freerolls aside), ROI " + Fixed(T.PaidRoi() * 100.0, 1) +
			"%, " + (Abi > 0 ? Fixed(static_cast<double>(T.PaidProfit()) / static_cast<double>(Abi), 1) : std::string("0")) + " buy-ins up; professional " +
			(H.Professional ? "yes" : "no") + ", last played " + std::to_string(H.DaysSincePlayed) + " days ago; Verified now " +
			(badge::VerifiedNow(H) ? "yes" : "no") + ", held " + (H.VerifiedHeld ? "yes" : "no") + "\n";
	}
	return Out;
}
} // namespace session_badges_detail

const badge::Set& Session::HeroBadges() const
{
	using namespace session_badges_detail;
	// Rebuilt only when something it reads moved: the day, the world's revisions, the live record, the last shift, the
	// review, the channel's Partner, the running deals, a debug grant.
	const double Clock = WorldMinutes();
	uint64_t Stamp = 1469598103934665603ull;
	Stamp = Mix(Stamp, static_cast<uint64_t>(static_cast<int64_t>(net::DayOf(Clock))));
	Stamp = Mix(Stamp, static_cast<uint64_t>(LivingWorld.Ready() ? 1 : 0));
	Stamp = Mix(Stamp, static_cast<uint64_t>(static_cast<int64_t>(LivingWorld.Revision())));
	Stamp = Mix(Stamp, static_cast<uint64_t>(static_cast<int64_t>(LivingWorld.HeroRevision())));
	Stamp = Mix(Stamp, static_cast<uint64_t>(static_cast<int64_t>(LivingWorld.HeroAwards().size())));
	Stamp = Mix(Stamp, static_cast<uint64_t>(static_cast<int64_t>(Life.LiveEvents)));
	Stamp = Mix(Stamp, static_cast<uint64_t>(static_cast<int64_t>(Life.LiveWins)));
	Stamp = Mix(Stamp, static_cast<uint64_t>(Life.LiveDays.size()));
	Stamp = Mix(Stamp, Life.LiveDays.empty() ? 0ull : static_cast<uint64_t>(static_cast<int64_t>(Life.LiveDays.back().Day * 64 + Life.LiveDays.back().Embercrest)));
	Stamp = Mix(Stamp, Bits(Life.LastShiftAt));
	Stamp = Mix(Stamp, static_cast<uint64_t>(static_cast<int64_t>(Life.BadgeMonth * 2 + (Life.VerifiedHeld ? 1 : 0))));
	Stamp = Mix(Stamp, static_cast<uint64_t>(Channel.Partner ? 1 : 0));
	Stamp = Mix(Stamp, static_cast<uint64_t>(static_cast<int64_t>(Person.Age)));
	Stamp = Mix(Stamp, static_cast<uint64_t>(History.size()));
	for (const auto& Br : sponsor::Badges(Life.Sponsors, Clock))
	{
		Stamp = MixText(Stamp, Br.first);
	}
	for (auto It = TextsSeen.lower_bound(GrantPrefix); It != TextsSeen.end() && It->rfind(GrantPrefix, 0) == 0; ++It)
	{
		Stamp = MixText(Stamp, *It);
	}
	Stamp |= 1ull;
	if (HeroSet.Stamp != Stamp)
	{
		badge::Set Fresh = badge::Earned(badge::OfHero(*this), net::DayOf(Clock));
		for (auto It = TextsSeen.lower_bound(GrantPrefix); It != TextsSeen.end() && It->rfind(GrantPrefix, 0) == 0; ++It)
		{
			for (const badge::Badge& G : badge::Unpack(It->substr(std::strlen(GrantPrefix))).Held)
			{
				Fresh.Held.erase(std::remove_if(Fresh.Held.begin(), Fresh.Held.end(), [&G](const badge::Badge& B) { return B.K == G.K; }), Fresh.Held.end());
				Fresh.Held.push_back(G);
			}
		}
		std::stable_sort(Fresh.Held.begin(), Fresh.Held.end(), [](const badge::Badge& A, const badge::Badge& B) { return static_cast<int>(A.K) < static_cast<int>(B.K); });
		Fresh.Stamp = Stamp;
		// The cache behind a const call (the session itself is never const: only views of it are).
		const_cast<Session*>(this)->HeroSet = std::move(Fresh);
	}
	return HeroSet;
}

std::vector<badge::Badge> Session::Unseen()
{
	// Each earning is celebrated once (C10): its key goes into TextsSeen, which the save keeps. A tier below one already
	// celebrated (a club card after a drop on the 1st, from a card the player went straight past) is noted, never toasted.
	std::vector<badge::Badge> Out;
	const badge::Set Held = HeroBadges();
	for (const badge::Badge& B : Held.Held)
	{
		bool Below = false;
		if (B.K == badge::Kind::Club || B.K == badge::Kind::CircuitCard)
		{
			for (int Up = B.Tier + 1; Up <= 3 && !Below; ++Up)
			{
				badge::Badge Higher = B;
				Higher.Tier = Up;
				Below = TextsSeen.count(badge::SeenKey(Higher)) > 0;
			}
		}
		if (TextsSeen.insert(badge::SeenKey(B)).second && !Below)
		{
			Out.push_back(B);
		}
	}
	if (!Out.empty())
	{
		Save();
	}
	return Out;
}

std::vector<badge::Progress> Session::BadgeProgress(int Max) const
{
	return badge::Next(*this, Max);
}

void Session::BadgeStep(double /*From*/, double To)
{
	using namespace session_badges_detail;
	// The world learns the player's age for the Hall's rule (it doesn't save it): born the year the story began, less
	// the age the character was then.
	if (LivingWorld.Ready())
	{
		LivingWorld.HeroBorn = world::YearOf(net::DayOf(LivingWorld.StartedAt())) - Person.Age;
	}
	// The monthly review (§5.3): the 1st at 6 AM, or at once when the save is behind (a game closed through the 1st, a
	// save from before the segment). Verified Pro is the review's verdict until the next one; a reload never re-runs it.
	const int Due = ReviewDue(To);
	if (Life.BadgeMonth < Due)
	{
		Life.VerifiedHeld = badge::VerifiedNow(badge::OfHero(*this));
		Life.BadgeMonth = Due;
	}
	(void)HeroBadges();
}

std::string Session::DebugBadge(const std::string& Args)
{
	using namespace session_badges_detail;
	const std::vector<std::string> W = Words(Args);
	const std::string Cmd = W.empty() ? std::string() : Lower(W[0]);
	const double Clock = WorldMinutes();
	const int Today = net::DayOf(Clock);
	if (Cmd == "grant" && W.size() >= 2)
	{
		const badge::Kind K = KindNamed(W[1]);
		const int N = W.size() >= 3 ? std::max(1, std::atoi(W[2].c_str())) : 1;
		const bool Main = W.size() >= 4 ? Lower(W[3]) == "main" : (W.size() >= 3 && Lower(W[2]) == "main");
		switch (K)
		{
		case badge::Kind::Club:
		{
			// Real nights at the room (a debug record with no money in it), so the card room sees the tier in the save.
			const int Want = N >= 3 ? badge::ClubCrest : N == 2 ? badge::ClubEmber : badge::ClubRegular;
			int Have = badge::VisitsIn(badge::HeroRoomDays(Life), Today);
			for (int D = Today; D > Today - badge::ClubWindowDays && Have < Want; --D)
			{
				auto It = std::lower_bound(Life.LiveDays.begin(), Life.LiveDays.end(), D, [](const life::LiveDay& X, int Key) { return X.Day < Key; });
				if (It != Life.LiveDays.end() && It->Day == D)
				{
					if (It->Embercrest > 0)
					{
						continue;
					}
					It->Events += 1;
					It->Embercrest += 1;
				}
				else
				{
					life::LiveDay Fresh;
					Fresh.Day = D;
					Fresh.Events = 1;
					Fresh.Embercrest = 1;
					Life.LiveDays.insert(It, Fresh);
				}
				++Have;
			}
			Life.SegmentLinesRead |= life::SegLiveRecord;
			break;
		}
		case badge::Kind::OnTheWall:
			// The live record's titles; the save writes them once the segment keeps the record (a fresh career doesn't yet).
			Life.LiveWins = std::max(Life.LiveWins, N);
			Life.SegmentLinesRead |= life::SegLiveWins;
			break;
		case badge::Kind::VerifiedPro:
			Life.VerifiedHeld = true;
			Life.BadgeMonth = std::max(Life.BadgeMonth, ReviewDue(Clock));
			break;
		case badge::Kind::KastPartner: Channel.Partner = true; break;
		case badge::Kind::Bracelet:
		case badge::Kind::Ring:
			for (int I = 0; I < std::min(N, 12); ++I)
			{
				LivingWorld.GrantAward(-1, K == badge::Kind::Ring, Main);
			}
			break;
		case badge::Kind::TeamRiverLine: return DebugSponsor("sign riverline");
		case badge::Kind::Sponsored: return DebugSponsor("sign " + (W.size() >= 3 ? Lower(W[2]) : std::string("luckypenny")));
		case badge::Kind::HallOfFame:
		case badge::Kind::PlayerOfYear:
		case badge::Kind::LivePlayerOfYear:
		case badge::Kind::CircuitCard:
		{
			// What only the world's calendar grants (the standings, New Year, July 15): kept as a debug grant.
			badge::Badge G;
			G.K = K;
			const int Year = world::YearOf(Today);
			const int Given = W.size() >= 3 ? std::atoi(W[2].c_str()) : 0;
			if (K == badge::Kind::CircuitCard)
			{
				G.Tier = std::clamp(Given > 0 ? Given : 1, 1, 3);
				G.Year = Year;
				G.Color = G.Tier == 3 ? ui::badgeart::tok::Onyx : G.Tier == 2 ? ui::badgeart::tok::Gold : ui::badgeart::tok::CardSilver;
			}
			else
			{
				G.Year = Given > 1000 ? Given : K == badge::Kind::HallOfFame ? Year : Year - 1;
				G.Count = 1;
				G.Tier = K == badge::Kind::HallOfFame ? 0 : (G.Year == Year - 1 ? 1 : 0);
				G.Color = K == badge::Kind::PlayerOfYear ? ui::badgeart::tok::Teal : K == badge::Kind::LivePlayerOfYear ? ui::badgeart::tok::LiveGold : ui::badgeart::tok::Gold;
			}
			badge::Set One;
			One.Held.push_back(G);
			for (auto It = TextsSeen.lower_bound(GrantPrefix); It != TextsSeen.end() && It->rfind(GrantPrefix, 0) == 0;)
			{
				const badge::Set Old = badge::Unpack(It->substr(std::strlen(GrantPrefix)));
				It = !Old.Held.empty() && Old.Held[0].K == K ? TextsSeen.erase(It) : std::next(It);
			}
			TextsSeen.insert(GrantPrefix + badge::Pack(One));
			break;
		}
		case badge::Kind::Count: return "ss.Badge.Grant: no badge called " + W[1] + ". Try Club, Wall, Verified, KastPartner, Bracelet, Ring, TeamRiverLine, Sponsored, Card, POY, LivePOY or Hall.";
		}
		const badge::Badge* Got = HeroBadges().Find(K);
		Save();
		return Got ? "Granted: " + badge::Label(*Got) + ". " + badge::HowEarned(*Got, badge::OfHero(*this)) : "Granted, but the rules don't show it yet.";
	}
	if (Cmd == "clear")
	{
		for (auto It = TextsSeen.lower_bound(GrantPrefix); It != TextsSeen.end() && It->rfind(GrantPrefix, 0) == 0;)
		{
			It = TextsSeen.erase(It);
		}
		Save();
		return "Debug grants cleared.";
	}
	if (Cmd == "describe")
	{
		std::string Who;
		for (size_t K = 1; K < W.size(); ++K)
		{
			Who += (K > 1 ? " " : "") + W[K];
		}
		if (Who.empty() || Lower(Who) == "me")
		{
			return HeroName + ":\n" + Describe(HeroBadges(), badge::OfHero(*this));
		}
		const int Npc = LivingWorld.Find(Who);
		if (Npc < 0)
		{
			return "ss.Badge.Describe: nobody called " + Who + ".";
		}
		return Who + ":\n" + Describe(badge::Shared().Of(LivingWorld, Npc, Today), badge::Of(LivingWorld, Npc, Today));
	}
	if (Cmd == "atlas")
	{
		std::string Out = "The badge atlas: " + std::to_string(ui::badgeart::AtlasBadges().size()) + " cells, 8 a row.\n";
		int Cell = 0;
		for (const badge::Badge& B : ui::badgeart::AtlasBadges())
		{
			Out += "  " + std::to_string(Cell++) + ": " + badge::Label(B) + (B.Main ? " (Main)" : "") + "\n";
		}
		return Out;
	}
	if (Cmd == "review")
	{
		const badge::Holder H = badge::OfHero(*this);
		const bool Was = Life.VerifiedHeld;
		Life.VerifiedHeld = badge::VerifiedNow(H);
		Life.BadgeMonth = std::max(Life.BadgeMonth, ReviewDue(Clock));
		Save();
		return std::string("Review: Verified Pro ") + (Life.VerifiedHeld ? (Was ? "kept" : "granted") : (Was ? "lost" : "not yet")) + ".\n" + Describe(HeroBadges(), H);
	}
	if (Cmd == "next")
	{
		std::string Out;
		for (const badge::Progress& P : BadgeProgress(3))
		{
			Out += "  " + P.Line + " (" + P.Detail + ")\n";
		}
		return Out.empty() ? std::string("Nothing in progress.") : Out;
	}
	if (Cmd == "unseen")
	{
		std::string Out;
		for (const badge::Badge& B : HeroBadges().Held)
		{
			Out += "  " + badge::SeenKey(B) + (TextsSeen.count(badge::SeenKey(B)) ? " (seen)" : " (not yet)") + "\n";
		}
		return Out.empty() ? std::string("No badges.") : Out;
	}
	return "ss.Badge: Grant <kind> [tier|count] [main], Clear, Describe <name|me>, Atlas, Review, Next, Unseen.";
}
} // namespace ss
