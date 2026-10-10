#pragma once

#include "ShortStack/Audio/Voice.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Tournament.h"

#include <string>
#include <utility>
#include <vector>

namespace ss
{
/** The tick just finished, table by table (Tournament::Rounds; Tournament.h gets it with Phase A's seams). */
struct TableRound;
namespace world
{
class World;
}

/**
 * The room's one voice (next_segment_spec §1.5, §4): the director is the only thing that speaks in the card room.
 * Other features hand it data (the rail's backers, the field's patches and badges, the clock); none of them calls the
 * table's Announce or the floor directly. It reads the engine (events, the rounds at every table, the hero's hands, the
 * clock) and hands the host cues: lines, dealer calls, the PA, the crowd's vocals and phone buzzes, with the mood that
 * sets the mix. Every choice is a pure function of saved state, world minutes and a seed built from ids (N5); caps and
 * rate limits count in engine ticks, never frames or real seconds, so the cue log is a pure function of the
 * tournament, the seed and the player's inputs. Write/Read keep what's been said across a checkpoint (liveroom).
 *
 * Bodies: core-room (Room.cpp). Phase A, day 1: stubs that compile and say nothing.
 */
namespace room
{
enum class Mood : int
{
	Arrival,
	Early,
	Running,
	Break,
	MoneyClose,
	HandForHand,
	Burst,
	LastTables,
	FinalTable,
	HeadsUp,
	Champion,
	After,
};
SHORTSTACKCORE_API Mood MoodOf(const Tournament& T, bool OnBreak, bool Dealt);
/** Levels in dB for audio::CardRoom::SetMix. */
struct Mix
{
	float Bed = 0.0f;
	float Talk = 0.0f;
	float Chips = 0.0f;
	float Calls = 0.0f;
	float Rail = -60.0f;
	float Crowd = 1.0f;
};
SHORTSTACKCORE_API Mix MixFor(Mood M, float OpenShare);
enum class Kind : int
{
	Line,
	Call,
	Pa,
	Vocal,
	Phone,
};
enum class From : int
{
	HeroTable,
	Table,
	FeatureTable,
	Rail,
	Bar,
	Desk,
	Pa,
};
struct Cue
{
	Kind What = Kind::Line;
	From Where = From::HeroTable;
	int TableId = 0;
	int Seat = -1;
	std::string Who;
	std::string Speaker;
	std::string Key;
	std::string Text;
	std::string Sound;
	audio::Vocal Noise = audio::Vocal::None;
	int Voices = 1;
	double Delay = 0.0;
	int Priority = 0;
	bool ToHero = false;
	bool Chime = false;
	float Effort = 0.6f;
};
/** What the host saw of a hand at the hero's table. */
struct HandNote
{
	int Hand = 0;
	std::vector<std::string> InHand;
	std::vector<std::string> Folded;
	std::string Aggressor;
	bool HeroFoldedToRaise = false;
	bool HeroShowedBluff = false;
	double HeroThink = 0.0;
	bool AllIn = false;
	bool Called = false;
	std::vector<std::pair<std::string, std::vector<Card>>> Shown;
	std::vector<Card> Board;
	std::vector<Card> BoardAtAllIn;
	std::vector<std::string> Winners;
	std::vector<std::string> Busted;
	Chips Pot = 0;
	Chips HeroStack = 0;
};
struct RailBacker
{
	std::string Name;
	int Permille = 0;
};
class Director
{
public:
	/** Seed: live::SeedFor(E, hero) + ":room". Voices: E.Voices, or VoicesFor(E). */
	SHORTSTACKCORE_API Director(const life::LiveEntry& E, const live::Occurrence& O, const std::string& Seed, const std::string& HeroFirstName);
	/** Nothing said twice. */
	SHORTSTACKCORE_API void Resume(const Tournament& T, const std::string& State, double World);
	/** stake::RailFor, from the host. */
	SHORTSTACKCORE_API void SetRail(const std::vector<RailBacker>& Backers);
	SHORTSTACKCORE_API void Events(const Tournament& T, const std::vector<TEvent>& E, double World);
	SHORTSTACKCORE_API void Rounds(const Tournament& T, const std::vector<TableRound>& R);
	SHORTSTACKCORE_API void Hand(const Tournament& T, const HandNote& H);
	SHORTSTACKCORE_API void Clock(const Tournament& T, double World, double BreakLeft);
	SHORTSTACKCORE_API std::vector<Cue> Take();
	Mood Now() const { return Current; }
	float TalkScale = 1.0f;
	SHORTSTACKCORE_API std::string Write() const;
	SHORTSTACKCORE_API bool Read(const std::string& Text);

private:
	Mood Current = Mood::Arrival;
};
/** Voices for an entry registered before they were kept: from its roster, the cast and the faces' labels; no story. */
SHORTSTACKCORE_API std::vector<life::LiveVoice> VoicesFor(const life::LiveEntry& E);
/** For live::DescribeField. */
SHORTSTACKCORE_API life::LiveVoice VoiceOf(const world::World& W, int Npc, const std::string& Hero);
} // namespace room
} // namespace ss
