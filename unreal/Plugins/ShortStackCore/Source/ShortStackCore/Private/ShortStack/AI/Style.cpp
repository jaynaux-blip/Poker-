#include "ShortStack/AI/Style.h"
#include "../StrictFloat.h"

#include "StyleDetail.h"

#include "ShortStack/Equity.h"
#include "ShortStack/Rng.h"
#include "ShortStack/Tournament.h"

#include <algorithm>

// Autoplay's styles (update 2 §2.3 to §2.5): the catalog and its copy, each style's profile (§2.4.1), Mix, the plan's
// think time, and Decide: the slip (§2.5), the tilt the style feels, and the dispatch to the modules (StyleGto.cpp,
// StyleExploit.cpp, StylePushFold.cpp, IcmPlay.cpp). Autopilot is today's Sprint exactly: no slips, tilt 0, the bots'
// pressure, the same draws (U3: it's the bar). core-styles.

namespace ss
{
namespace style
{
namespace style_detail
{
/** The archetype a style is built on (§2.4.1). */
Archetype StyleBase(Style S)
{
	switch (S)
	{
	case Style::Lag:
	case Style::BountyHunter: return Archetype::Lag;
	case Style::Nit: return Archetype::Nit;
	case Style::Maniac: return Archetype::Maniac;
	case Style::Reg:
	case Style::IcmShark: return Archetype::Reg;
	case Style::Gto: return Archetype::Crusher;
	default: return Archetype::Tag;
	}
}

uint64_t StyleSplitMix(uint64_t X)
{
	X += 0x9E3779B97F4A7C15ull;
	X = (X ^ (X >> 30)) * 0xBF58476D1CE4E5B9ull;
	X = (X ^ (X >> 27)) * 0x94D049BB133111EBull;
	return X ^ (X >> 31);
}

/** "think" as a Mix salt. */
constexpr uint32_t StyleThinkSalt = 0x7468696eu;

BotDecision StyleDecideProfile(const HeroContext& C, const Profile& P, double Tilt, Rng& R)
{
	BotContext Ctx;
	Ctx.Prof = &P;
	Ctx.Tilt = Tilt;
	Ctx.IcmPressure = C.Pressure;
	Ctx.Fast = false;
	Ctx.R = &R;
	return ss::Decide(C.View, Ctx);
}

double StyleTiltSeen(const HeroBrain& B, const Profile& P)
{
	const double T = B.Tilt * P.TiltProne / 0.35;
	return T <= 0.0 ? 0.0 : T >= 1.0 ? 1.0 : T;
}

double StyleTiltWide(double Tilt)
{
	return 1.0 + 0.8 * Max(0.0, Tilt);
}

BotDecision StyleChoice(const PlayerView& V, const PlayerAction& A, double Equity)
{
	return bot_detail::MakeChoice(bot_detail::Legalize(V, A), 0.0, Equity);
}

/**
 * A decision worth a slip (§2.5): facing a raise or a bet, any decision after the flop, or a pot over 10 big blinds. An
 * unopened junk fold isn't: the plan never slips there, so the slip rate lands where it costs.
 */
bool StyleNonTrivial(const PlayerView& V)
{
	if (V.CurrentStreet != Street::Preflop)
	{
		return true;
	}
	const double Bb = bot_detail::BbOf(V);
	if (static_cast<double>(V.Pot) > 10.0 * Bb)
	{
		return true;
	}
	if (PreflopSummary(V).Raises > 0)
	{
		return true;
	}
	// Unopened: a hand the plan would play (the top half), or a stack short enough that the shove is the decision.
	const PublicSeat& Me = V.Seats[static_cast<size_t>(V.Me)];
	const double StackBb = static_cast<double>(Me.Stack + Me.StreetBet) / Bb;
	return V.Hole.size() == 2 && (HandPercentile(V.Hole[0], V.Hole[1]) <= 0.5 || StackBb <= 15.0);
}

/** The slip's profile: pushed toward the losing station direction (§2.5). Every style gets worse, the Nit included. */
Profile StyleSlipped(const Profile& P)
{
	Profile S = P;
	S.Aggression = P.Aggression * 0.5;
	S.Stickiness = Min(0.95, P.Stickiness + 0.30);
	S.CallWidth = P.CallWidth * 1.4;
	S.Bluff = P.Bluff * 0.5;
	S.PushFold = P.PushFold * 0.5;
	S.IcmAware = P.IcmAware * 0.3;
	return S;
}
} // namespace style_detail

const std::vector<Info>& Catalog()
{
	// Append-only (the ids are saved). Tempo: ThinkSeconds' base for each style (§2.4.7).
	static const std::vector<Info> All = {
		{Style::Autopilot, "autopilot", "Autopilot", "The old Sprint. Safe on paper, a step behind good play.", Swing::Even, "Getting through a night", false, false, Style::Autopilot, 1.5,
			false},
		{Style::Tag, "tag", "TAG", "Tight and aggressive. Few hands, played hard.", Swing::Even, "Anywhere", false, false, Style::Tag, 1.8, false},
		{Style::Lag, "lag", "LAG", "Loose and aggressive. More pots, more pressure, more swings.", Swing::Swingy, "Tables of nits; deep stacks", false, false, Style::Lag, 1.4,
			false},
		{Style::Nit, "nit", "Nit", "Premiums only. Rarely busts early. Rarely wins.", Swing::Steady, "Satellites; reaching the money", false, false, Style::Nit, 2.0, false},
		{Style::Maniac, "maniac", "Maniac", "Raises everything. Huge nights, early busts. Loses money.", Swing::Wild, "Entertainment", false, false, Style::Maniac, 0.9, false},
		{Style::Reg, "reg", "Balanced Reg", "A solid regular. Beats weak fields steadily; breaks even against good ones.", Swing::Steady, "Anywhere", false, false, Style::Reg,
			2.0, false},
		{Style::Gto, "gto", "GTO",
			"Balanced ranges from charts. About the same against anyone; leaves money against weak players. An approximation, not a solver.", Swing::Steady,
			"Tough fields", false, false, Style::Gto, 2.4, false},
		{Style::Exploit, "exploit", "Exploitative", "Learns the table, then attacks it. Plain TAG until it has reads.", Swing::Even,
			"Long sessions; soft fields; regulars it has notes on", false, false, Style::Exploit, 2.2, false},
		{Style::PushFold, "pushfold", "Push/Fold", "Short-stack charts. Best in turbos and hypers.", Swing::Swingy, "Hypers, turbos, any short stack", true, false, Style::Tag,
			1.2, false},
		{Style::IcmShark, "icm", "ICM Shark", "Plays the payout ladder. Best on bubbles and in satellites.", Swing::Even, "Satellites, bubbles, final tables", true, false,
			Style::Reg, 2.6, false},
		// P1: hidden from the panel until its module lands (§2.4.6).
		{Style::BountyHunter, "bounty", "Bounty Hunter", "Calls wider for a head. Knockout events only.", Swing::Swingy, "Knockout and bounty events", true, true, Style::Lag,
			1.6, true},
	};
	return All;
}

const Info& InfoOf(Style S)
{
	const std::vector<Info>& All = Catalog();
	const int I = static_cast<int>(S);
	return I >= 0 && I < static_cast<int>(All.size()) ? All[static_cast<size_t>(I)] : All.front();
}

bool FromId(const std::string& Id, Style& Out)
{
	for (const Info& I : Catalog())
	{
		if (Id == I.Id)
		{
			Out = I.S;
			return true;
		}
	}
	return false;
}

Profile ProfileOf(Style S)
{
	if (S == Style::Autopilot)
	{
		// Today's Sprint autopilot, number for number (it was session_detail::Autopilot): the bar (U3).
		Profile P = SprintProfile();
		P.Label = "Autopilot";
		P.CallWidth = 1.35;
		P.Stickiness = 0.35;
		P.Aggression = 0.6;
		P.Bluff = 0.12;
		P.PushFold = 0.65;
		P.IcmAware = 0.45;
		return P;
	}
	Profile P = profiles_detail::Base(style_detail::StyleBase(S));
	switch (S)
	{
	case Style::PushFold:
		P.OpenWidth = 0.9;
		P.CallWidth = 0.9;
		P.Bluff = 0.12;
		P.Cbet = 0.55;
		P.PushFold = 1.0;
		break;
	case Style::IcmShark: P.IcmAware = 1.0; break;
	case Style::BountyHunter:
		P.Bluff = 0.26;
		P.IcmAware = 0.5;
		break;
	default: break;
	}
	// Every hero profile: balanced timing and no think draw (ThinkTime draws nothing), the style's name.
	P.Timing = TimingStyle::Balanced;
	P.ThinkBase = 0.0;
	P.ThinkVar = 0.0;
	P.Label = InfoOf(S).Name;
	return P;
}

BotDecision Decide(const HeroContext& C, const HeroBrain& B, Rng& R)
{
	using namespace style_detail;
	if (B.S == Style::Autopilot)
	{
		// Today's Sprint, exactly: the same profile, tilt 0 (the hero never had one), the bots' pressure, the same draws.
		const Profile P = ProfileOf(Style::Autopilot);
		return StyleDecideProfile(C, P, 0.0, R);
	}
	const Profile P = ProfileOf(B.S);
	const double Tilt = StyleTiltSeen(B, P);
	// The slip (§2.5): drawn with Mix, so the watch and skip paths slip at the same decisions.
	const double SlipChance = SlipK * (1.0 - Max(0.0, Min(1.0, B.Sharp)));
	if (SlipChance > 0.0 && StyleNonTrivial(C.View) &&
		Mix(C.MixSeed, C.Tick, C.HeroIdx, static_cast<int>(C.View.History.size()), SaltSlip) < SlipChance)
	{
		return StyleDecideProfile(C, StyleSlipped(P), Tilt, R);
	}
	switch (B.S)
	{
	case Style::Gto: return StyleGtoDecide(C, B, P, Tilt, R);
	case Style::Exploit: return StyleExploitDecide(C, B, P, Tilt, R);
	case Style::PushFold: return StylePushFoldDecide(C, B, P, Tilt, R);
	case Style::IcmShark: return StyleIcmDecide(C, B, P, Tilt, R);
	default: return StyleDecideProfile(C, P, Tilt, R); // TAG, LAG, Nit, Maniac, Balanced Reg; Bounty Hunter's profile (P1)
	}
}

BotDecision DecideCash(const PlayerView& V, const HeroBrain& B, Rng& R, uint64_t MixSeed, int HandNo)
{
	// What the style plays as at a cash table (§2.3: Push/Fold as TAG, ICM Shark as Balanced Reg), with no tournament: no
	// bubble pressure, no payouts. The table's Rng plays the engine's draws; MixSeed and HandNo key the style's own.
	HeroBrain Cash = B;
	Cash.S = InfoOf(B.S).AtCash;
	HeroContext C;
	C.View = V;
	C.Pressure = 0.0;
	C.T = nullptr;
	C.HeroIdx = V.Me;
	C.MixSeed = MixSeed;
	C.Tick = HandNo;
	return Decide(C, Cash, R);
}

double Mix(uint64_t Seed, int Tick, int Seat, int Action, uint32_t Salt)
{
	uint64_t Z = style_detail::StyleSplitMix(Seed);
	Z = style_detail::StyleSplitMix(Z ^ static_cast<uint64_t>(static_cast<uint32_t>(Tick)));
	Z = style_detail::StyleSplitMix(Z ^ static_cast<uint64_t>(static_cast<uint32_t>(Seat)));
	Z = style_detail::StyleSplitMix(Z ^ static_cast<uint64_t>(static_cast<uint32_t>(Action)));
	Z = style_detail::StyleSplitMix(Z ^ static_cast<uint64_t>(Salt));
	return static_cast<double>(Z >> 11) * (1.0 / 9007199254740992.0);
}

double ThinkSeconds(const HeroBrain& B, const HeroContext& C, int Speed)
{
	// Tempo × (0.6 + 0.8h) × Hard ÷ Speed (§2.4.7): Hard 0.7 with a free check, 1.4 facing a bet, 2.0 for the stack.
	const PlayerView& V = C.View;
	const Chips Stack = V.Me >= 0 && V.Me < static_cast<int>(V.Seats.size()) ? V.Seats[static_cast<size_t>(V.Me)].Stack : 0;
	const bool ForStack = V.ToCall > 0 && V.ToCall * 100 >= Stack * 35; // a call of 35% of the stack or more (§2.4.2)
	const double Hard = ForStack ? 2.0 : V.ToCall > 0 ? 1.4 : 0.7;
	const double H = Mix(C.MixSeed, C.Tick, C.HeroIdx, static_cast<int>(V.History.size()), style_detail::StyleThinkSalt);
	const double Seconds = InfoOf(B.S).Tempo * (0.6 + 0.8 * H) * Hard / static_cast<double>(std::max(1, Speed));
	const double Floor = B.Where == 0 ? 0.15 : (Speed <= 1 ? 1.0 : 0.15);
	return std::max(Floor, Seconds);
}

std::vector<std::string> Inventory()
{
	std::vector<std::string> Out;
	for (const Info& I : Catalog())
	{
		Out.push_back(I.Name);
		Out.push_back(I.Edge);
		Out.push_back(I.BestIn);
	}
	for (int V = 0; V <= static_cast<int>(Swing::Wild); ++V)
	{
		Out.push_back(SwingName(static_cast<Swing>(V)));
	}
	for (const Info& I : Catalog())
	{
		for (int Where = 0; Where <= 2; ++Where)
		{
			const std::string Why = Unavailable(I.S, Where, false);
			if (!Why.empty() && std::find(Out.begin(), Out.end(), Why) == Out.end())
			{
				Out.push_back(Why);
			}
		}
	}
	return Out;
}

const char* SwingName(Swing V)
{
	switch (V)
	{
	case Swing::Steady: return "STEADY";
	case Swing::Swingy: return "SWINGY";
	case Swing::Wild: return "WILD";
	default: return "EVEN";
	}
}

std::string Unavailable(Style S, int Where, bool Knockout)
{
	// Dee's game: no payouts and a hundred big blinds deep, so the tournament styles play as their cash style (§2.6.3).
	const Info& I = InfoOf(S);
	if (I.TourneyOnly && Where == 2)
	{
		return std::string("Plays as ") + InfoOf(I.AtCash).Name + " here";
	}
	if (I.BountyOnly && !Knockout)
	{
		return "Knockout events only";
	}
	return std::string();
}
} // namespace style
} // namespace ss
