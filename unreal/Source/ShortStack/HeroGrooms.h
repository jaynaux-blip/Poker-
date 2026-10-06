#pragma once

#include "CoreMinimal.h"

class AShortStackCharacter;
class UGroomAsset;
class UGroomComponent;

/**
 * The street's grooms (spec 7.5.4): which groom is which, the LOD each is held on by detail tier, and the day-one groom
 * spike. Phase B adds the library (card-only copies under /Game/ShortStack/Cast/Grooms/<Hero>, UHeroGroomLibrary) and the
 * one resolver HeroGrooms(Look, Face) for the street and the table.
 *
 * Hair by tier (spec 6.7), for every MetaHuman on the street: Near holds the first cards LOD; Mid holds a lower cards LOD,
 * the first plus ss.Look.MidCardsStep (picked 1 of the spec's 1..2: at 8-25 m a groom's second cards LOD still reads as
 * the same hair, the third thins visibly), never released, since a groom let go without the Blueprint's LODSync falls
 * straight to its helmet; only Far shows the helmet (the last cards LOD when a groom has none). A groom with no cards and
 * no helmet (lashes, peach fuzz) is left as built at Near and Mid and hidden at Far.
 *
 * The spike (ss.Look.GroomSpike, AShortStackCharacter::TestGroomSpike): swaps the hero's scalp groom at run time with
 * UGroomComponent::SetGroomAsset(Groom, Binding), preloaded asynchronously first (as the barber's Style Book will be,
 * spec 5.1), held on cards, and measures the frame around it: the call's game-thread time, every frame's time for 30
 * frames before and 60 after (with the game thread, render thread and GPU times stat unit shows), whether the binding was
 * accepted and the hair bound to the face with a skinning LOD (so it follows the head), and the LOD it's on. A hitch is the
 * worst frame in the 60 after the swap less the higher of the median frame of the 30 before it and the settled frame (the
 * median of the last 30 after: a groom that costs more to draw, strands, raises every frame, which is its running cost and
 * is reported as "settled", not as the swap's hitch); the budget is 8 ms, half a 60 Hz frame
 * (the editor's PIE runs about 22 ms a frame at the Lucky Penny). Verdicts: PASS when every swap, the first included, is
 * within it, the hair bound and on cards; PASS WARM when the repeats are within it but a groom's first swap isn't (its GPU
 * resources are made the first time it's drawn: the Style Book's preload must draw each once, hidden); HITCH when the
 * median isn't; FAIL when the binding is refused, the hair isn't bound to the face or isn't on cards.
 *
 * Candidates (until groom_setup.py builds the library, every hero face has exactly one scalp groom, so no "groom from
 * another assembly of the same face" exists yet; these are what's on disk):
 *   self     the groom the hero wears, set again with its own binding (the pure re-initialization cost)
 *   back     the hero's own groom and binding, set back after a candidate: a card groom bound to this very face, the
 *            barber's real case
 *   <Name>   the scalp groom of another assembly (HeroA1, ExtraH, Sal...) with that assembly's binding: baked against
 *            another face of the same topology, which the engine accepts (only the LOD counts are checked), so it fits
 *            approximately; its cost is the same as a same-face copy's
 *   <path>   a groom asset path, with binding=<path> (or <path>_Binding next to it): the library's copies once they exist
 *   all      self, then each other hero of the hero's body type (and back after each), 3 times
 * Options: repeat=N (default 3), binding=<path|none>, sync (load inside the timed call: the cost without the Style Book's
 * preload; in the editor a groom loaded that way still builds from the DDC, so it says little there), shots (two HighResShots
 * per candidate, the head turned 40 degrees each way, Saved/Screenshots/GroomSpike_*), face=HeroA0 (dress the hero as that
 * hero face, bare-headed and with its own scalp hair, for the spike, the spec's case, and back after).
 * Measuring: every candidate stays loaded for the whole run (a repeat is a warm swap, not a reload); a garbage collection is
 * forced in each step's settle frames, so the editor's own can't land in a measured window; each line gives the baseline's
 * worst frame too, and the summary notes noisy baselines and a throttled editor (keep the PIE viewport focused).
 * In PIE: ss.Look.GroomSpike all face=HeroA0 shots, about a minute; ss.Look.GroomSpike report prints every line again.
 */
namespace HeroGrooms
{
/** What a MetaHuman groom is, by its Blueprint variable (Hair, Eyebrows, Beard, ...) or its asset's name (Beard_S_Stubble). */
enum class EGroomKind : uint8
{
	Scalp,
	Brows,
	Lashes,
	Beard,
	Mustache,
	Fuzz,
};
SHORTSTACK_API EGroomKind KindOf(const FString& Variable, const FString& Asset);

/** A groom's LODs: the first and last drawn with cards, the first drawn as a mesh (the helmet); INDEX_NONE where it has none. */
struct FGroomLods
{
	int32 FirstCards = INDEX_NONE;
	int32 LastCards = INDEX_NONE;
	int32 Helmet = INDEX_NONE;
	int32 Count = 0;
};
SHORTSTACK_API FGroomLods LodsOf(const UGroomAsset* Asset);

/** The LOD a groom is held on at a detail tier (0 Near, 1 Mid, 2 Far), or INDEX_NONE to leave it as it is. */
SHORTSTACK_API int32 LodForTier(const UGroomAsset* Asset, int32 Tier);
/** Holds a groom on its tier's LOD (see the header comment); returns false for a groom with nothing to hold at that tier. */
SHORTSTACK_API bool HoldForTier(UGroomComponent* Groom, int32 Tier);
/** Near's hold: the first cards LOD (a groom without cards is left alone). The Blueprint's LODSync isn't copied with it. */
SHORTSTACK_API void HoldOnCards(UGroomComponent* Groom);

/** Starts the groom spike on the hero (see the header comment); returns the plan, or why it can't start. */
SHORTSTACK_API FString StartSpike(AShortStackCharacter* Hero, const FString& Candidate, const FString& Options);
/** Stops a running spike, putting the hero's own groom back. */
SHORTSTACK_API void StopSpike();
SHORTSTACK_API bool IsSpikeRunning();
/** Every result line so far, and the summary once it's done. */
SHORTSTACK_API FString SpikeReport();
} // namespace HeroGrooms
