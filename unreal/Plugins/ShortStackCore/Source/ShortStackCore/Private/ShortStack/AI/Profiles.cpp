#include "ShortStack/AI/Profiles.h"

#include "ShortStack/Rng.h"

namespace ss
{
namespace profiles_detail
{
Profile Base(Archetype A)
{
	Profile P;
	P.Type = A;
	switch (A)
	{
	case Archetype::Fish:
		P.Label = "Fish"; P.OpenWidth = 1.6; P.LimpRate = 0.55; P.ThreeBet = 0.02; P.CallWidth = 2.2; P.Aggression = 0.45; P.Bluff = 0.12; P.Stickiness = 0.55; P.Cbet = 0.5; P.Sizing = 0.9; P.TiltProne = 0.8; P.PushFold = 0.2; P.IcmAware = 0.1; P.ThinkBase = 1400; P.ThinkVar = 2600;
		break;
	case Archetype::Station:
		P.Label = "Calling Station"; P.OpenWidth = 1.3; P.LimpRate = 0.4; P.ThreeBet = 0.015; P.CallWidth = 2.6; P.Aggression = 0.3; P.Bluff = 0.05; P.Stickiness = 0.85; P.Cbet = 0.4; P.Sizing = 0.8; P.TiltProne = 0.5; P.PushFold = 0.3; P.IcmAware = 0.1; P.ThinkBase = 1200; P.ThinkVar = 1800;
		break;
	case Archetype::Nit:
		P.Label = "Nit"; P.OpenWidth = 0.6; P.LimpRate = 0.05; P.ThreeBet = 0.015; P.CallWidth = 0.6; P.Aggression = 0.6; P.Bluff = 0.04; P.Stickiness = 0.05; P.Cbet = 0.55; P.Sizing = 1.0; P.TiltProne = 0.3; P.PushFold = 0.6; P.IcmAware = 0.8; P.ThinkBase = 1600; P.ThinkVar = 2200;
		break;
	case Archetype::Tag:
		P.Label = "TAG"; P.OpenWidth = 1.0; P.LimpRate = 0.02; P.ThreeBet = 0.04; P.CallWidth = 1.0; P.Aggression = 0.75; P.Bluff = 0.18; P.Stickiness = 0.2; P.Cbet = 0.65; P.Sizing = 1.0; P.TiltProne = 0.35; P.PushFold = 0.85; P.IcmAware = 0.6; P.ThinkBase = 1500; P.ThinkVar = 2800;
		break;
	case Archetype::Lag:
		P.Label = "LAG"; P.OpenWidth = 1.45; P.LimpRate = 0.02; P.ThreeBet = 0.075; P.CallWidth = 1.3; P.Aggression = 0.85; P.Bluff = 0.32; P.Stickiness = 0.3; P.Cbet = 0.75; P.Sizing = 1.1; P.TiltProne = 0.45; P.PushFold = 0.8; P.IcmAware = 0.4; P.ThinkBase = 1200; P.ThinkVar = 2600;
		break;
	case Archetype::Maniac:
		P.Label = "Maniac"; P.OpenWidth = 3.2; P.LimpRate = 0.05; P.ThreeBet = 0.2; P.CallWidth = 2.2; P.Aggression = 0.95; P.Bluff = 0.5; P.Stickiness = 0.45; P.Cbet = 0.9; P.Sizing = 1.45; P.TiltProne = 0.7; P.PushFold = 0.25; P.IcmAware = 0.05; P.ThinkBase = 700; P.ThinkVar = 1200;
		break;
	case Archetype::Reg:
		P.Label = "Reg"; P.OpenWidth = 1.1; P.LimpRate = 0.01; P.ThreeBet = 0.05; P.CallWidth = 1.05; P.Aggression = 0.8; P.Bluff = 0.24; P.Stickiness = 0.22; P.Cbet = 0.62; P.Sizing = 1.0; P.TiltProne = 0.25; P.PushFold = 0.95; P.IcmAware = 0.85; P.ThinkBase = 1800; P.ThinkVar = 3200;
		break;
	case Archetype::Crusher:
		P.Label = "Crusher"; P.OpenWidth = 1.2; P.LimpRate = 0.0; P.ThreeBet = 0.06; P.CallWidth = 1.1; P.Aggression = 0.82; P.Bluff = 0.28; P.Stickiness = 0.24; P.Cbet = 0.58; P.Sizing = 1.0; P.TiltProne = 0.1; P.PushFold = 1.0; P.IcmAware = 1.0; P.ThinkBase = 2000; P.ThinkVar = 3000;
		break;
	}
	return P;
}

double Jitter(Rng& R, double V, double Amt)
{
	const double G = R.Gauss(0.0, Amt);
	return Max(0.0, V * (1.0 + G));
}
} // namespace profiles_detail

const char* ArchetypeName(Archetype A)
{
	switch (A)
	{
	case Archetype::Fish: return "fish";
	case Archetype::Station: return "station";
	case Archetype::Nit: return "nit";
	case Archetype::Tag: return "tag";
	case Archetype::Lag: return "lag";
	case Archetype::Maniac: return "maniac";
	case Archetype::Reg: return "reg";
	default: return "crusher";
	}
}

bool ArchetypeFromName(const std::string& Name, Archetype& Out)
{
	for (int I = 0; I <= static_cast<int>(Archetype::Crusher); ++I)
	{
		if (Name == ArchetypeName(static_cast<Archetype>(I)))
		{
			Out = static_cast<Archetype>(I);
			return true;
		}
	}
	return false;
}

const char* TimingName(TimingStyle T)
{
	switch (T)
	{
	case TimingStyle::Honest: return "honest";
	case TimingStyle::Reverse: return "reverse";
	default: return "balanced";
	}
}

Profile MakeProfile(Archetype A, Rng& R)
{
	using profiles_detail::Jitter;
	const Profile B = profiles_detail::Base(A);
	const double TimingRoll = R.Next();
	TimingStyle Timing;
	if (A == Archetype::Crusher || A == Archetype::Reg)
	{
		Timing = TimingRoll < 0.75 ? TimingStyle::Balanced : TimingStyle::Honest;
	}
	else
	{
		Timing = TimingRoll < 0.65 ? TimingStyle::Honest : TimingRoll < 0.85 ? TimingStyle::Reverse : TimingStyle::Balanced;
	}
	// Same field order as the TypeScript object literal: each Jitter draws from the RNG.
	Profile P = B;
	P.Timing = Timing;
	P.OpenWidth = Jitter(R, B.OpenWidth, 0.12);
	P.LimpRate = Min(1.0, Jitter(R, B.LimpRate, 0.12));
	P.ThreeBet = Jitter(R, B.ThreeBet, 0.2);
	P.CallWidth = Jitter(R, B.CallWidth, 0.12);
	P.Aggression = Min(1.0, Jitter(R, B.Aggression, 0.08));
	P.Bluff = Min(1.0, Jitter(R, B.Bluff, 0.2));
	P.Stickiness = Min(0.95, Jitter(R, B.Stickiness, 0.15));
	P.Cbet = Min(1.0, Jitter(R, B.Cbet, 0.1));
	P.Sizing = Jitter(R, B.Sizing, 0.08);
	P.TiltProne = Min(1.0, Jitter(R, B.TiltProne, 0.2));
	P.PushFold = Min(1.0, Jitter(R, B.PushFold, 0.1));
	P.IcmAware = Min(1.0, Jitter(R, B.IcmAware, 0.15));
	P.ThinkBase = Jitter(R, B.ThinkBase, 0.2);
	P.ThinkVar = Jitter(R, B.ThinkVar, 0.2);
	return P;
}

const Population& GetPopulation(const std::string& Name)
{
	static const Population Freeroll = {{Archetype::Fish, 38}, {Archetype::Station, 18}, {Archetype::Maniac, 14}, {Archetype::Nit, 12}, {Archetype::Tag, 10}, {Archetype::Lag, 5}, {Archetype::Reg, 3}};
	static const Population Micro = {{Archetype::Fish, 26}, {Archetype::Station, 18}, {Archetype::Nit, 15}, {Archetype::Tag, 16}, {Archetype::Lag, 8}, {Archetype::Maniac, 7}, {Archetype::Reg, 10}};
	static const Population Low = {{Archetype::Fish, 16}, {Archetype::Station, 12}, {Archetype::Nit, 16}, {Archetype::Tag, 24}, {Archetype::Lag, 12}, {Archetype::Maniac, 4}, {Archetype::Reg, 15}, {Archetype::Crusher, 1}};
	static const Population High = {{Archetype::Fish, 4}, {Archetype::Nit, 6}, {Archetype::Tag, 20}, {Archetype::Lag, 15}, {Archetype::Reg, 35}, {Archetype::Crusher, 20}};
	if (Name == "freeroll") return Freeroll;
	if (Name == "low") return Low;
	if (Name == "high") return High;
	return Micro;
}

Archetype RollArchetype(const Population& Pop, Rng& R)
{
	double Total = 0.0;
	for (const auto& E : Pop)
	{
		Total += E.second;
	}
	double Roll = R.Next() * Total;
	for (const auto& E : Pop)
	{
		Roll -= E.second;
		if (Roll <= 0.0)
		{
			return E.first;
		}
	}
	return Pop.back().first;
}

Profile SprintProfile()
{
	Profile P = profiles_detail::Base(Archetype::Tag);
	P.Timing = TimingStyle::Balanced;
	P.ThinkBase = 0.0;
	P.ThinkVar = 0.0;
	return P;
}
} // namespace ss
