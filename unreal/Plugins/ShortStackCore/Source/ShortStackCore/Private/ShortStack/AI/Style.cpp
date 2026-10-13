#include "ShortStack/AI/Style.h"

#include "ModelDetail.h"

#include "ShortStack/Rng.h"

#include <algorithm>

// Autoplay's styles (update 2 §2.3, §2.4). Phase A, day 1 (the lead): the catalog and its copy, the profiles (§2.4.1),
// Mix and ThinkSeconds as specified, and Decide as a stub that plays every style's profile through the engine's own
// decision (Autopilot: today's Sprint exactly). core-styles owns this file from here: the slip, GTO, Exploitative, Push/Fold
// and ICM Shark land in it and its siblings (StyleGto.cpp, StyleExploit.cpp, StylePushFold.cpp).

namespace ss
{
namespace style
{
namespace style_detail
{
/** The profile a style is built on, before its own numbers (§2.4.1). */
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
	// Phase A's stub: the style's profile through the engine's own decision, as a bot plays it (no slips, no tilt, no
	// modules yet). Autopilot is today's Sprint exactly: the same profile, tilt 0, the bots' pressure, the same draws.
	const Profile P = ProfileOf(B.S);
	BotContext Ctx;
	Ctx.Prof = &P;
	Ctx.Tilt = 0.0;
	Ctx.IcmPressure = C.Pressure;
	Ctx.Fast = false;
	Ctx.R = &R;
	return ss::Decide(C.View, Ctx);
}

BotDecision DecideCash(const PlayerView& V, const HeroBrain& B, Rng& R, uint64_t MixSeed, int HandNo)
{
	// Phase A's stub: what the style plays as at a cash table, through the engine's own decision.
	(void)MixSeed;
	(void)HandNo;
	const Profile P = ProfileOf(InfoOf(B.S).AtCash);
	BotContext Ctx;
	Ctx.Prof = &P;
	Ctx.Tilt = 0.0;
	Ctx.IcmPressure = 0.0;
	Ctx.Fast = false;
	Ctx.R = &R;
	return ss::Decide(V, Ctx);
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
	return Out;
}
} // namespace style
} // namespace ss
