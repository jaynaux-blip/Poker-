#pragma once

// Internal to the core (update 2 §2.4): what the styles' files share. AI/Style.cpp (the catalog, the slip, the dispatch),
// StyleGto.cpp, StyleExploit.cpp, StylePushFold.cpp, IcmPlay.cpp and the generated ChartTables.cpp. core-styles.

#include "ModelDetail.h"

#include "ShortStack/AI/Charts.h"
#include "ShortStack/AI/Reads.h"
#include "ShortStack/AI/Style.h"

#include <cstdint>

namespace ss
{
namespace charts
{
/** The generated tables (AI/ChartTables.cpp, from Tests/charts/*.txt by tools/chartgen). */
namespace chart_data
{
/** A set of the 169 classes. */
struct Bits169
{
	uint64_t W[3];
};
inline bool HasClass(const Bits169& B, int Cls)
{
	return Cls >= 0 && Cls < 169 && ((B.W[Cls / 64] >> (Cls % 64)) & 1u) != 0;
}
/** One GTO rule (gto_preflop.txt): the shares are fractions of the 1,326 combos, from the top of the preflop ranking. */
struct GtoRow
{
	int Spot; // charts::Spot
	int Pos;  // 9-max seat order (0 UTG .. 8 BB); Vs3Bet: 0 in position, 1 out of position
	int Vs;   // the opener's seat order (BbVsOpen, IpVsOpen)
	int Band; // 0 short (25 BB and under), 1 mid (25 to 40), 2 deep (over 40)
	double A, B, MixPct;
	Bits169 Bluff, Set, Mix;
};
/** The checksum of the chart texts the tables were compiled from (FNV-1a 64 over each file's name and LF text). */
extern const uint64_t TextChecksum;
/** [ante 0|1][first-in position 0..7][depth step 0..38: 1 to 20 BB by 0.5]. */
extern const Bits169 Push[2 * 8 * 39];
/** [ante][jammer 0..7][caller - jammer - 1, 0..7][depth step]. */
extern const Bits169 Call[2 * 8 * 8 * 39];
/** [ante][opener bucket: EP, MP, LP, SB][me: in position, SB, BB][depth step 0..20: 15 to 25 BB by 0.5]. */
extern const Bits169 Reshove[2 * 4 * 3 * 21];
/** The opener's call of that re-jam, same index. */
extern const Bits169 Recall[2 * 4 * 3 * 21];
extern const GtoRow Gto[];
extern const int GtoRows;
} // namespace chart_data

namespace charts_detail
{
/** Preflop's mixed frequencies with every share widened by Wide (tilt, §2.5; 1 = the chart as written). */
Mixed PreflopWide(Spot S, int Position, int VsPosition, double EffBb, int HandClass, double Wide);
/** The share of combos at or above the class's midpoint in the ranking (0 = AA's middle, 1 = the bottom). */
double ClassMid(int HandClass);
/** The opener bucket the re-shove tables use for a 9-max seat order: 0 early, 1 middle, 2 late, 3 the small blind. */
int OpenerBucket(int Position);
/**
 * The jam share of the push chart at (position, depth, ante), and each class's rank in the order hands join the jam
 * range as stacks get shorter (ICM Shark's scaled threshold, §2.4.5): 0 = jams deepest.
 */
double JamShare(int Position, double EffBb, double AnteBb);
double JamOrderShare(int Position, double AnteBb, int HandClass);
} // namespace charts_detail
} // namespace charts

namespace style
{
namespace style_detail
{
// ---------------------------------------------------------------------------------------------------------- knobs
// §2.5's calibration knobs, in the order they turn: (1) the slip coefficient; (2) each module's constants (here and in
// the modules); (3) SkillTerm's floor (core-autoplay's). Every change is recorded in the spec's §2.5.

/** A slip's chance at a non-trivial decision: k x (1 - Sharpness). */
constexpr double SlipK = 1.2;

// ---------------------------------------------------------------------------------------------------------- shared

/** Mix salts: one per use, so no two draws share a key. */
enum StyleSalt : uint32_t
{
	SaltSlip = 0x736c6970u,     // "slip"
	SaltPre = 0x70726566u,      // "pref"
	SaltRange = 0x72616e67u,    // "rang"
	SaltPost = 0x706f7374u,     // "post"
	SaltBluff = 0x626c7566u,    // "bluf"
	SaltPushFold = 0x70757368u, // "push"
	SaltIcm = 0x69636d21u,      // "icm!"
};

/** The style's profile through the engine's own decision (the tree every bot uses), at the bots' bubble pressure. */
BotDecision StyleDecideProfile(const HeroContext& C, const Profile& P, double Tilt, Rng& R);
/** What tilt the style feels: Tilt x TiltProne / 0.35, clamped to 1 (§2.5): GTO barely, Maniac double. */
double StyleTiltSeen(const HeroBrain& B, const Profile& P);
/** How much wider tilt plays (Tilted()'s open width): 1 + 0.8 x the tilt seen. */
double StyleTiltWide(double Tilt);
/** A decision made out of an action (Legalize'd, no think time: presentation is ThinkSeconds'). */
BotDecision StyleChoice(const PlayerView& V, const PlayerAction& A, double Equity);

/** The table seen as 9-max seats, from the start of the hand and a prefix of the history (what was known then). */
struct PreflopNode
{
	enum class Kind : int
	{
		Unopened, // nobody has put in more than the big blind (limpers may have)
		VsOpen,   // one raise, not all in
		Vs3Bet,   // two raises, the second not all in
		Vs4Bet,   // three or more raises, not all in
		VsJam,    // the last raise is all in
	};
	Kind K = Kind::Unopened;
	int Me = 0;        // 9-max seat order: 0 UTG .. 6 BTN, 7 SB, 8 BB (by how many are left to act behind)
	int Vs = 0;        // the opener's (VsOpen), the last raiser's (Vs3Bet, Vs4Bet, VsJam)
	int Opener = 0;    // the first raiser's
	bool IOpened = false;
	bool Ip = false;   // in position against the last raiser after the flop
	int Limpers = 0;
	int Callers = 0;   // players who called the last raise before me
	int Raises = 0;
	double E = 100;    // the effective stack in big blinds at the start of the hand (mine or the deepest other in it)
	double JamBb = 0;  // the jammer's stack at the start of the hand (VsJam)
	double Ante = 0;   // the ante over the big blind
	double LastRaiseBb = 0;
	double Pressure = 0; // the bubble as the style feels it: the bots' PressureFor x the style's IcmAware (set by the caller)
};
/** The node at history index Prefix (History.size() for the decision now). */
PreflopNode StylePreflopNode(const PlayerView& V, size_t Prefix);

/** The preflop chart decision shared by GTO and Push/Fold: a raise or jam, a call, or a fold, with its frequencies. */
struct PreflopPlan
{
	bool Known = false;  // false: no chart covers the spot (the caller falls back to its profile)
	charts::Mixed F;     // the frequencies for this class
	double RaiseToBb = 0; // the size when it raises (in big blinds; a jam is the whole stack)
	bool Jam = false;
};
/** Push/Fold's charts (§2.4.4): Nash jams, calls and re-shoves at 25 BB and under; Known is false deeper. */
PreflopPlan StylePushFoldPlan(const PreflopNode& N, int HandClass, double Wide);
/** GTO's charts (§2.4.2): Push/Fold's at 25 BB and under, the scenario charts deeper. */
PreflopPlan StyleGtoPlan(const PreflopNode& N, int HandClass, double Wide);
/** A plan into an action at the view: Mix draws the mixed frequency (U uniform in [0, 1)). */
PlayerAction StylePlanAction(const PlayerView& V, const PreflopPlan& Plan, double U);

// ---------------------------------------------------------------------------------------------------------- the modules

BotDecision StyleGtoDecide(const HeroContext& C, const HeroBrain& B, const Profile& P, double Tilt, Rng& R);
BotDecision StylePushFoldDecide(const HeroContext& C, const HeroBrain& B, const Profile& P, double Tilt, Rng& R);
BotDecision StyleExploitDecide(const HeroContext& C, const HeroBrain& B, const Profile& P, double Tilt, Rng& R);
BotDecision StyleIcmDecide(const HeroContext& C, const HeroBrain& B, const Profile& P, double Tilt, Rng& R);
} // namespace style_detail
} // namespace style
} // namespace ss
