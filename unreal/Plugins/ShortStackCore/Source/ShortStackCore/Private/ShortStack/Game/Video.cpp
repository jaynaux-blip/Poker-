#include "ShortStack/Game/Video.h"

#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/Life.h"

// Rerun (update 2 §4). Phase A, day 1 (the lead): stubs that keep today's behaviour (no candidates, no videos, no views,
// no payouts, no video line written or read). core-video owns this file from here (and VideoCopy.cpp beside it).

namespace ss
{
namespace video
{
Kind Classify(const recall::Record& Rec, double EquityAtAllIn, bool Bubble, bool Showdown, double AvgStackBb, bool& Is)
{
	(void)Rec;
	(void)EquityAtAllIn;
	(void)Bubble;
	(void)Showdown;
	(void)AvgStackBb;
	Is = false;
	return Kind::BigPot;
}

void NoteHand(Channel& C, const Candidate& Cand)
{
	(void)C;
	(void)Cand;
}

void NoteLiveHand(life::State& L, const recall::Record& Rec, const room::HandNote* Note, const LiveContext& Ctx)
{
	(void)L;
	(void)Rec;
	(void)Note;
	(void)Ctx;
}

void NoteStory(Channel& C, const std::string& Key, double World)
{
	(void)C;
	(void)Key;
	(void)World;
}

std::string Title(const Choice& Ch, const std::vector<Candidate>& Cands, uint32_t Seed)
{
	(void)Ch;
	(void)Cands;
	(void)Seed;
	return std::string();
}

std::string Upload(Channel& C, const Choice& Ch, double World, const std::string& Seed, const UploadFacts& F)
{
	(void)C;
	(void)Ch;
	(void)World;
	(void)Seed;
	(void)F;
	return std::string();
}

double Views(const Video& V, double World)
{
	(void)V;
	(void)World;
	return 0.0;
}

void Step(Channel& C, life::State& L, Chips& Bankroll, kast::Channel& K, double From, double To)
{
	// Phase A's stub: nothing to step (a no-op until G2).
	(void)C;
	(void)L;
	(void)Bankroll;
	(void)K;
	(void)From;
	(void)To;
}

ChannelView View(const Channel& C, double World)
{
	(void)World;
	ChannelView V;
	V.Subs = C.Subs;
	V.Partner = C.PartnerAt >= 0.0;
	V.Balance = C.BalanceCents;
	return V;
}

std::vector<life::Mark> Marks(const Channel& C, double From, double To)
{
	(void)C;
	(void)From;
	(void)To;
	return {};
}

void Grant(Channel& C, double Subs, double Hours, double ViewCount, double World)
{
	(void)C;
	(void)Subs;
	(void)Hours;
	(void)ViewCount;
	(void)World;
}

std::vector<std::string> Inventory()
{
	return {};
}

void WriteLines(const life::State& L, std::string& Out)
{
	// Phase A's stub: no video line yet, so every save writes back as it was read.
	(void)L;
	(void)Out;
}

bool ReadLine(const std::vector<std::string>& P, life::State& L)
{
	(void)P;
	(void)L;
	return false;
}

void AfterParse(SaveData& D)
{
	(void)D;
}
} // namespace video
} // namespace ss
