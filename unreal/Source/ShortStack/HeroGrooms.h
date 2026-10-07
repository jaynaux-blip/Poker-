#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ShortStack/Game/Hero.h"

#include "HeroGrooms.generated.h"

class AShortStackCharacter;
class UGroomAsset;
class UGroomBindingAsset;
class UGroomComponent;

/**
 * One groom of a hero face's library: a style's card-only copy assembled on that face (groom_setup.py), its binding baked
 * against the face's own build (/Game/ShortStack/Cast/Built/MHC_<Face>/Face), or one of the build's own grooms.
 */
USTRUCT(BlueprintType)
struct FHeroGroomEntry
{
	GENERATED_BODY()

	/** The style as the engine's wardrobe names it ("Hair_S_BuzzCut", "Beard_S_Stubble", "Eyebrows_M_Thin"): the resolver's key. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Short Stack|Grooms")
	FName Style;
	/** HeroGrooms::EGroomKind: 0 scalp, 1 brows, 2 lashes, 3 beard, 4 mustache, 5 fuzz. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Short Stack|Grooms")
	int32 Kind = 0;
	/** One of the face's own build's grooms (its Natural brows are these), not an assembled copy. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Short Stack|Grooms")
	bool bOwn = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Short Stack|Grooms")
	TSoftObjectPtr<UGroomAsset> Groom;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Short Stack|Grooms")
	TSoftObjectPtr<UGroomBindingAsset> Binding;
};

/**
 * A hero face's groom library (spec 7.5.4): every style the barber can put on that face, as /Game/ShortStack/Cast/Grooms/
 * <Face>/HGL_<Face>. groom_setup.py makes the copies (SetWardrobe, then Assemble, keeping only Grooms/, each binding
 * retargeted to the face's own build) and calls Refresh, which lists them and the build's own grooms here.
 *
 * Soft references: the library is small and loads with the hero (LibraryFor); a style's groom loads when it's worn, and
 * the whole set asynchronously when the barber's stage wakes (Preload). Cook-safe: everything is under /Game/ShortStack,
 * which DefaultGame.ini always cooks, and the cooker follows the soft references of what it cooks (keep
 * /Game/ShortStack/Cast/Grooms in any narrowed cook list). Nothing references the plugin's strand grooms.
 */
UCLASS(BlueprintType)
class SHORTSTACK_API UHeroGroomLibrary : public UDataAsset
{
	GENERATED_BODY()

public:
	/** The hero face these belong to ("HeroA0"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Short Stack|Grooms")
	FName Face;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Short Stack|Grooms")
	TArray<FHeroGroomEntry> Entries;

	/** The entry for a style (by its name), or null. */
	const FHeroGroomEntry* Find(FName Style) const;
	/** The build's own groom of a kind (its brows: the Natural brows), or null. */
	const FHeroGroomEntry* Own(int32 Kind) const;

	/**
	 * Editor: lists every groom binding under /Game/ShortStack/Cast/Grooms/<Face> (the assembled copies, one folder per
	 * assembly) and in the face's own build (Cast/Built/MHC_<Face>/Grooms, marked bOwn) as entries, a copy winning over
	 * the build's for the same style; returns a summary line. A no-op outside the editor.
	 */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|Grooms")
	FString Refresh();

	/** "/Game/ShortStack/Cast/Grooms/HeroA0/HGL_HeroA0" (the package). */
	static FString PackageFor(FName Face);
};

/**
 * The street's and the table's grooms (spec 7.5.4, 6.7): which groom is which, the library and the one resolver
 * (HeroGrooms::Resolve) that says what a look wears on a hero face, the LOD each is held on by detail tier, the Style
 * Book's preload, and the day-one groom spike.
 *
 * The map (creator choice -> the engine wardrobe's groom; the face's skin band picks a hair texture variant, band 2 the
 * coily ones). groom_setup.py's PLAN mirrors these tables: change both together.
 *   Hair      0 Shaved: none · 1 Buzz: Hair_S_BuzzCut (band 2 Hair_S_CoilBuzzCut) · 2 Crew: Hair_S_BrushCut ·
 *             3 Textured Crop: Hair_S_Messy · 4 Side Part: Hair_S_SideSweptFringe · 5 Undercut: Hair_S_SweptUp ·
 *             6 Curls: Hair_S_CurlyFade (band 2 Hair_S_Coil) · 7 Afro: Hair_S_AfroFade (band 2 Hair_L_AfroCurly) ·
 *             8 Shoulder: Hair_L_Straight · 9 Ponytail: Hair_S_LowPonytail · 10 Braids: Hair_M_TwistedBraids (band 2
 *             Hair_S_Cornrows) · 11 Bob: Hair_M_BobStraight
 *   Facial    0 None · 1 Stubble: Beard_S_Stubble + Mustache_S_Stubble · 2 Mustache: Mustache_M_Gunslinger (the spec's
 *             "Mustache_M (pick)") · 3 Goatee: Goatee_M_Pointed + Mustache_S_Full (a goatee with its mustache) ·
 *             4 Short: Beard_S_Full + Mustache_S_Full · 5 Full: Beard_L_Full + Mustache_L_Full
 *   Brows     0 Thin: Eyebrows_S_FlatThin (type B Eyebrows_M_Thin) · 1 Natural: the face's own build's brows ·
 *             2 Thick: Eyebrows_M_Thick (type B Eyebrows_M_Full) · 3 Arched: Thin's (spec 7.5.4)
 * Under a hat only hair that lies close stays (as before the library): a fuller style comes off under it.
 *
 * Hair by tier (spec 6.7), for every MetaHuman on the street: Near holds the first cards LOD; Mid holds a lower cards LOD,
 * the first plus ss.Look.MidCardsStep (picked 1 of the spec's 1..2: at 8-25 m a groom's second cards LOD still reads as
 * the same hair, the third thins visibly), never released, since a groom let go without the Blueprint's LODSync falls
 * straight to its helmet; only Far shows the helmet (the last cards LOD when a groom has none). A groom with no cards and
 * no helmet (lashes, peach fuzz) is left as built at Near and Mid and hidden at Far.
 *
 * The spike (ss.Look.GroomSpike, AShortStackCharacter::TestGroomSpike): swaps the hero's scalp groom at run time with
 * UGroomComponent::SetGroomAsset(Groom, Binding), preloaded asynchronously first (as the barber's Style Book is, spec
 * 5.1), held on cards, and measures the frame around it: the call's game-thread time, every frame's time for 30 frames
 * before and 60 after (with the game thread, render thread and GPU times stat unit shows), whether the binding was
 * accepted and the hair bound to the face with a skinning LOD (so it follows the head), and the LOD it's on. A hitch is the
 * worst frame in the 60 after the swap less the higher of the median frame of the 30 before it and the settled frame (the
 * median of the last 30 after: a groom that costs more to draw, strands, raises every frame, which is its running cost and
 * is reported as "settled", not as the swap's hitch); the budget is 8 ms, half a 60 Hz frame
 * (the editor's PIE runs about 22 ms a frame at the Lucky Penny). Verdicts: PASS when every swap, the first included, is
 * within it, the hair bound and on cards; PASS WARM when the repeats are within it but a groom's first swap isn't (its GPU
 * resources are made the first time it's drawn: the Style Book's preload must draw each once, hidden); HITCH when the
 * median isn't; FAIL when the binding is refused, the hair isn't bound to the face or isn't on cards. Day one: PASS.
 *
 * Candidates: self (the hero's own groom set again), back (put back after a candidate), <Name> (another assembly's scalp
 * groom, HeroA1, ExtraH...), <path> (a groom asset, binding=<path> or <path>_Binding next to it: the library's copies),
 * all (self, then each other hero of the hero's body type, 3 times). Options: repeat=N (default 3), binding=<path|none>,
 * sync (load inside the timed call), shots (two HighResShots per candidate, Saved/Screenshots/GroomSpike_*), face=HeroA0
 * (dress the hero as that hero face for the spike, and back after). In PIE: ss.Look.GroomSpike all face=HeroA0 shots.
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

// ------------------------------------------------------------------ the library and the resolver (spec 7.5.4)

/** Hair that lies close to the scalp (a hat sits over it), by a groom's or style's name. */
SHORTSTACK_API bool IsTightHair(const FString& Style);
/** Hair cropped about as short as a buzz cut. */
SHORTSTACK_API bool IsCroppedHair(const FString& Style);

/** The scalp style a creator hairstyle (hero::Look::Hair) is on a face (None for Shaved, or past the table). */
SHORTSTACK_API FName ScalpStyle(int32 Hair, FName Face);
/** The beard and mustache styles a creator facial hair choice (hero::Look::FacialHair) wears (None where it has none). */
SHORTSTACK_API void FacialStyles(int32 FacialHair, FName& OutBeard, FName& OutMustache);
/** The brow style a creator brows choice (hero::Look::Brows) is on a face; Natural is the face's own (OwnBrows). */
SHORTSTACK_API FName BrowStyle(int32 Brows, FName Face, FName OwnBrows);

/** The library of a hero face (loaded the first time and kept), or null when groom_setup.py hasn't made one. */
SHORTSTACK_API const UHeroGroomLibrary* LibraryFor(FName Face);

/** One groom a look wears: its style and its assets (the soft references the library holds). */
struct FWornGroom
{
	FName Style;
	TSoftObjectPtr<UGroomAsset> Groom;
	TSoftObjectPtr<UGroomBindingAsset> Binding;
	bool IsSet() const { return !Style.IsNone(); }
};

/** What a look wears on a face: each managed groom (an unset one: none there). */
struct FGroomSet
{
	/** The face has a library: these are the grooms to wear. Without one the build's own grooms stay (WearsBuildGroom says which). */
	bool bLibrary = false;
	FWornGroom Scalp;
	FWornGroom Beard;
	FWornGroom Mustache;
	FWornGroom Brows;
	/** Styles the look asks for that the library doesn't have (that groom shows as none): PORTRAIT ONLY in the shops. */
	TArray<FName> Missing;
	const FWornGroom& Of(EGroomKind Kind) const;
};

/** The one resolver (street and table): what Look wears on Face. */
SHORTSTACK_API FGroomSet Resolve(const ss::hero::Look& Look, FName Face);
/** Without a library: whether a look keeps one of its build's own grooms (the rules from before the library). */
SHORTSTACK_API bool WearsBuildGroom(const ss::hero::Look& Look, EGroomKind Kind, const FString& Asset);
/** The soft paths a set needs (grooms and bindings), for loading it. */
SHORTSTACK_API TArray<FSoftObjectPath> PathsOf(const FGroomSet& Set);
/** Every set member is in memory (a swap needs no load). */
SHORTSTACK_API bool IsResident(const FGroomSet& Set);
/** Loads what a set needs without blocking, and calls Done on the game thread once it's in memory (at once when it is). */
SHORTSTACK_API void LoadAsync(const FGroomSet& Set, TFunction<void()> Done);
/**
 * Loads what a set needs without blocking and keeps it in memory until another set is held (or ReleaseWorn): for a head
 * built later than the load starts (the table's, built when the camera goes over the shoulder), so building it finds
 * the grooms resident instead of loading them on the key press.
 */
SHORTSTACK_API void HoldWorn(const FGroomSet& Set);
SHORTSTACK_API void ReleaseWorn();

/**
 * The Style Book's preload (spec 5.1): the face's whole library loaded asynchronously and held until ReleasePreload, so no
 * key press loads a groom. The barber's stage calls it as it wakes (AShortStackCharacter::PreloadGroomSet). Holding a
 * second face lets go of the first.
 */
SHORTSTACK_API void Preload(FName Face);
SHORTSTACK_API void ReleasePreload();
/** The face's whole set is in memory now. */
SHORTSTACK_API bool IsPreloaded(FName Face);

/**
 * What a face can show (HostCaps' HairStyles, FacialHair and Brows, indexed by the Look's value): with a library, each
 * choice whose grooms it holds; without one, only what its build wears (shaved, its own style, none, its own facial hair,
 * Natural brows). Missing choices read PORTRAIT ONLY.
 */
SHORTSTACK_API void FillCaps(FName Face, std::vector<bool>& OutHair, std::vector<bool>& OutFacial, std::vector<bool>& OutBrows);

/** A line about a face's library for ss.Look.Grooms: entries by kind, what's missing from the map, what's preloaded. */
SHORTSTACK_API FString Describe(FName Face);

// ------------------------------------------------------------------ the spike

/** Starts the groom spike on the hero (see the header comment); returns the plan, or why it can't start. */
SHORTSTACK_API FString StartSpike(AShortStackCharacter* Hero, const FString& Candidate, const FString& Options);
/** Stops a running spike, putting the hero's own groom back. */
SHORTSTACK_API void StopSpike();
SHORTSTACK_API bool IsSpikeRunning();
/** Every result line so far, and the summary once it's done. */
SHORTSTACK_API FString SpikeReport();
} // namespace HeroGrooms
