#pragma once

#include "ShortStack/Common.h"
#include "ShortStack/Game/Video.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ss
{
namespace world
{
class World;
}

/**
 * Update 3 (SPEC §6, §7.3): Rerun 2.0's library. About 2,000 title templates behind a stable picker that never offers one of
 * the last 20 titles' templates, thumbnail recipes over 36 archetypes (captions only from facts), 175 video ideas from the
 * player's own career (locked until their facts exist), 20 comment archetypes and the world's creators. Declared by the
 * lead in Phase A; bodies by core-videocontent (VideoTitles.cpp, VideoThumbs.cpp, VideoIdeas.cpp, VideoComments.cpp,
 * VideoWorld.cpp; G3).
 */
namespace video
{
/** One of Studio's three title options: the template's key, the filled text, its promise and hidden traits. */
struct TitleOption
{
	std::string Key, Text;
	uint8_t Promise = 0;
	uint16_t Traits = 0;
};
/** An idea on the IDEAS board: its facts, its formats, whether it's ready, and why not. */
struct IdeaCard
{
	int Id = 0;
	std::string Name, Fact, Hint;
	uint8_t Group = 0;
	std::vector<Format> Formats;
	int EditMin = 0, Energy = 0;
	bool Ready = false, Trending = false, Limited = false, Strike = false;
	int CooldownDays = 0, Have = 0, Need = 0;
};
/** What the ideas read (built by the Session from History, the ledger, vsnap, grades, moments, Kast, life, live entries, the world log, site play). */
struct IdeaCtx;
/** A repackage: a new title template, a new recipe, or both. */
struct Repack
{
	std::string TitleKey;
	ThumbSpec Spec;
};
/** Count options for a choice (the Shuffle-th deal); skips templates in the last 20 titles. */
SHORTSTACKCORE_API std::vector<TitleOption> TitleOptions(const Choice& Ch, const std::vector<Candidate>& Cands, const Channel& C, int Shuffle, int Count);
/** Count recipes in hash order, at least 4 archetypes a deal. */
SHORTSTACKCORE_API std::vector<ThumbSpec> ThumbOptions(const Choice& Ch, const std::vector<Candidate>& Cands, const Channel& C, int Shuffle, int Count);
/** How well an archetype fits the moment (§4.6.2). */
SHORTSTACKCORE_API double ArchetypeFit(int Arch, int KindGroup, const Candidate& Lead);
/** An archetype's Update 2 frame (§4.4). */
SHORTSTACKCORE_API Frame FrameOf(int Arch);
/** Pinned by rerun_content --write. */
SHORTSTACKCORE_API double TraitMean(int KindGroup);
/** The recipe's traits over TraitMean. */
SHORTSTACKCORE_API double ThumbTraits(const ThumbSpec& S, const Candidate& Lead, const Channel& C);
SHORTSTACKCORE_API uint8_t ThumbPromise(const ThumbSpec& S);
/** The recipe's visual signature (no caption text): rerun_content counts distinct looks with it. */
SHORTSTACKCORE_API uint64_t ThumbSignature(const ThumbSpec& S);
SHORTSTACKCORE_API std::vector<IdeaCard> Ideas(const IdeaCtx& X, const Channel& C, double World);
SHORTSTACKCORE_API bool MakeIdeaCandidate(const IdeaCtx& X, int IdeaId, double World, Candidate& Out);
/** A generated comment (procedural and read-only in P0). */
struct Comment
{
	std::string Author, Text;
	int Likes = 0;
	double At = 0;
	uint8_t Archetype = 0;
};
SHORTSTACKCORE_API std::vector<Comment> CommentsOf(const Video& V, double World, int From, int Count);
/** A video by one of the world's creators (Home, Next up, Research). */
struct WorldVideo
{
	std::string Creator, Title;
	ThumbSpec Spec;
	double At = 0, Views = 0;
};
SHORTSTACKCORE_API std::vector<WorldVideo> WorldVideos(const world::World& W, double World, int Count);
/** Every sentence the library says, for the copy lint and the trademark screen. */
SHORTSTACKCORE_API std::vector<std::string> InventoryU3();
} // namespace video
} // namespace ss
