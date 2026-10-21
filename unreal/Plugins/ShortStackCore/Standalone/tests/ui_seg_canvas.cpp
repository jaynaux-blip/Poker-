// rerun_ui: Phase A's canvas seams for Rerun 2.0's thumbnails (update 3 SPEC §6.1, §7.2, §8.2, §10), the lead's file.
// Canvas::IsCached and the cache pools: Shared behaves exactly as today's one map (a hit replays what the miss drew, the
// whole map starts over past its budget), a least-recently-used pool evicts its oldest past its own budget and never
// flushes another; and DrawPortrait's default stays byte-identical (the ID card, the creator and the barber, pinned here
// by the hash of their draw lists, with the expression overload at the neutral face drawing the same). Compiled into
// rerun_ui_test and called from rerun_ui_main.cpp; the helpers go in ui_test::canvas_seg.
#include "UiSeg.h"

#include "ShortStack/Common.h"
#include "ShortStack/Game/Hero.h"
#include "ShortStack/Rng.h"
#include "ShortStack/UI/Canvas.h"
#include "ShortStack/UI/Portrait.h"

#include <cstdio>
#include <string>

namespace ui_test
{
namespace canvas_seg
{
using ss::ui::Canvas;
using ss::ui::DrawList;
using Pool = ss::ui::Canvas::CachePool;

/** A small fragment: one filled rect (9 vertices), its colour from N so each key paints something of its own. */
void Tile(Canvas& C, int N)
{
	C.FillRect({0.0f, 0.0f, 40.0f, 20.0f}, ss::ui::Rgba(20 + N, 40, 60, 1.0f));
}

/** Draws key K (through Pool) at (X, Y) on a fresh list and returns the list's JSON. */
std::string Through(uint64_t K, Pool P, int N, float X = 10.0f, float Y = 20.0f)
{
	TableMeasurer M;
	DrawList L;
	Canvas C(L, M, 1600.0f, 1000.0f, 1.0f);
	C.Cached(K, X, Y, [N](Canvas& Cv) { Tile(Cv, N); }, P);
	return L.ToJson();
}

bool Cached(uint64_t K)
{
	TableMeasurer M;
	DrawList L;
	const Canvas C(L, M, 1600.0f, 1000.0f, 1.0f);
	return C.IsCached(K);
}

void Pools()
{
	// Keys of this suite's own (the caches are per thread and live for the run).
	const uint64_t Base = 0x5e9a0c11u;
	// Shared: the four-argument Cached and the pool's are one; a hit replays exactly what the miss drew.
	Expect(!Cached(Base + 1), "nothing is cached before it's drawn");
	const std::string Miss = Through(Base + 1, Pool::Shared, 1);
	Expect(Cached(Base + 1), "IsCached sees a drawn key");
	Expect(Through(Base + 1, Pool::Shared, 99) == Miss, "a hit replays the fragment (the paint isn't run again)");
	{
		TableMeasurer M;
		DrawList L;
		Canvas C(L, M, 1600.0f, 1000.0f, 1.0f);
		C.Cached(Base + 1, 10.0f, 20.0f, [](Canvas& Cv) { Tile(Cv, 7); });
		Expect(L.ToJson() == Miss, "the four-argument Cached is the Shared pool");
		C.Save();
		C.Scale(2.0f, 2.0f);
		Expect(!C.IsCached(Base + 1), "IsCached keys by scale, as Cached does");
		C.Rotate(0.3f);
		Expect(!C.IsCached(Base + 1), "a rotated canvas never caches");
		C.Restore();
	}
	// Rerun: least-recently-used past its budget (three tiles' worth here), and the Shared key above is never touched.
	Canvas::SetPoolBudget(Pool::Rerun, 27);
	Through(Base + 10, Pool::Rerun, 10);
	Through(Base + 11, Pool::Rerun, 11);
	Through(Base + 12, Pool::Rerun, 12);
	Expect(Cached(Base + 10) && Cached(Base + 11) && Cached(Base + 12), "three tiles fit Rerun's budget");
	Through(Base + 10, Pool::Rerun, 10); // used again: now the newest
	Through(Base + 13, Pool::Rerun, 13);
	Expect(Cached(Base + 10) && !Cached(Base + 11) && Cached(Base + 12) && Cached(Base + 13), "Rerun's pool evicts its least recently used");
	for (int K = 0; K < 40; ++K)
	{
		Through(Base + 100 + static_cast<uint64_t>(K), Pool::Rerun, K);
	}
	Expect(Cached(Base + 1), "a full Rerun pool never flushes the Shared pool");
	Expect(Cached(Base + 139) && Cached(Base + 137) && !Cached(Base + 136) && !Cached(Base + 10), "Rerun keeps its newest three");
	Canvas::SetPoolBudget(Pool::Rerun, 9);
	Expect(Cached(Base + 139) && !Cached(Base + 138), "a smaller budget evicts at once");
	// Boardwalk is a pool of its own too: its keys and Rerun's never meet.
	Through(Base + 200, Pool::Boardwalk, 3);
	Expect(Cached(Base + 200) && Cached(Base + 139) && Cached(Base + 1), "Boardwalk caches beside the others");
	// Shared, past its budget, starts over whole (today's behaviour), and leaves the other pools be.
	Canvas::SetPoolBudget(Pool::Shared, 18);
	Through(Base + 2, Pool::Shared, 2); // starts the map over if the suite's earlier pages left it fuller than that
	Through(Base + 1, Pool::Shared, 1);
	Expect(Cached(Base + 1) && Cached(Base + 2), "two tiles fit a Shared budget of 18");
	Through(Base + 3, Pool::Shared, 3);
	Expect(!Cached(Base + 1) && !Cached(Base + 2) && Cached(Base + 3), "the Shared pool starts over whole past its budget, as it always has");
	Expect(Cached(Base + 139) && Cached(Base + 200), "and never flushes another pool");
	// Back to the defaults (0) for whatever runs next on this thread.
	Canvas::SetPoolBudget(Pool::Shared, 0);
	Canvas::SetPoolBudget(Pool::Rerun, 0);
	Canvas::SetPoolBudget(Pool::Boardwalk, 0);
}

/** One portrait as a screen draws it, through the default overload or the expression one at the neutral face. */
std::string Portrait(const ss::hero::Character& Who, float Scale, double Time, const ss::ui::PortraitLight& Light, bool Expression)
{
	TableMeasurer M;
	DrawList L;
	Canvas C(L, M, 1600.0f, 1000.0f, 1.0f);
	if (Expression)
	{
		ss::ui::DrawPortrait(C, Who, 400.0f, 300.0f, Scale, Time, Light, ss::ui::ExpressionOf(0), ss::ui::PortraitPose::None, false);
	}
	else
	{
		ss::ui::DrawPortrait(C, Who, 400.0f, 300.0f, Scale, Time, Light);
	}
	return L.ToJson();
}

void Portraits()
{
	// The three screens the default must never move for (§8.2): the ID card (0.38, its rim light, mid-blink time), the
	// creator's live preview (1.5, the creator's light) and the barber's chair (the default light, a still). Pinned: the
	// FNV-1a of each draw list as Phase A drew it.
	struct Use
	{
		const char* Name;
		const char* Seed;
		float Scale;
		double Time;
		ss::ui::PortraitLight Light;
		uint32_t Pinned;
	};
	ss::ui::PortraitLight IdCard;
	IdCard.Rim = ss::ui::Hex(0xf2c14e);
	IdCard.RimStrength = 0.6f;
	ss::ui::PortraitLight Creator;
	Creator.Key = ss::ui::Hex(0xfff0dc);
	Creator.RimStrength = 0.9f;
	const Use Uses[3] = {
		{"the ID card", "u3-idcard", 0.38f, 3.25, IdCard, 0x57c5436eu},
		{"the creator", "u3-creator", 1.5f, 1.7, Creator, 0x130bfb16u},
		{"the barber", "u3-barber", 1.15f, 0.0, ss::ui::PortraitLight(), 0x537f99d3u},
	};
	for (const Use& U : Uses)
	{
		ss::Rng R(U.Seed);
		const ss::hero::Character Who = ss::hero::Random(R);
		const std::string Default = Portrait(Who, U.Scale, U.Time, U.Light, false);
		const uint32_t Hash = ss::Fnv1a(Default);
		std::printf("  portrait       %-12s %8zu bytes, fnv1a %08x\n", U.Name, Default.size(), Hash);
		Expect(Hash == U.Pinned, "DrawPortrait's default is byte-identical to Phase A's");
		Expect(Portrait(Who, U.Scale, U.Time, U.Light, true) == Default, "the expression overload at the neutral face draws the default");
	}
	const ss::ui::PortraitExpression& N = ss::ui::ExpressionOf(0);
	Expect(N.BrowLift == 0.0f && N.EyeOpen == 1.0f && N.Smile == 0.0f && !N.Teeth && !N.Tear, "expression 0 is the neutral face");
	Expect(&ss::ui::ExpressionOf(-1) == &ss::ui::ExpressionOf(0) || ss::ui::ExpressionOf(-1).EyeOpen == 1.0f, "an expression out of range is the neutral face");
}
} // namespace canvas_seg

void SegCanvas()
{
	canvas_seg::Pools();
	canvas_seg::Portraits();
}
} // namespace ui_test
