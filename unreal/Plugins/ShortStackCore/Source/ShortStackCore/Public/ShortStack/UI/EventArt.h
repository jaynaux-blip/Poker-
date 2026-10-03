#pragma once

#include "ShortStack/Game/Network.h"
#include "ShortStack/UI/Canvas.h"

namespace ss
{
namespace ui
{
/**
 * Every tournament's emblem, all vector. The schedule's tournaments get an icon tile in their brand's colours
 * (an owl for the Night Owl, a crosshair for the Headhunter, a coffee cup for Sunday Brunch...). Series events get
 * a crest: the series' own frame (RCOP's shield, Micro Madness's hexagon, the winter series' snowflake-cut octagon,
 * The Championship Online's medallion...), its motif, a rim whose metal says the stakes (bronze, silver, gold,
 * platinum), and what makes the event matter: a Main Event's rays, laurel, crown and ribbon; a bracelet wrapped
 * around a bracelet event; a ring hanging over a ring event; a diamond set on a high roller.
 */
namespace eventart
{
enum class Glyph : int
{
	Owl,
	Bolt,
	Moon,
	Eye,
	Crosshair,
	Gear,
	Crown,
	Diamond,
	Wave,
	Envelope,
	Coin,
	Stopwatch,
	Fan,
	Chips,
	Bricks,
	Tombstone,
	Sunrise,
	Claw,
	Stairs,
	Spiral,
	Rocket,
	Flame,
	Glove,
	SixSeats,
	Laurel,
	Showdown,
	Star,
	Storm,
	Skull,
	Ticket,
	Snowflake,
	Ring,
	Flower,
	Bracelet,
	Skyline,
	Sun,
	Gift,
	Mountain,
	Aurora,
	Crystal,
	Thermometer,
	TopHat,
	Sevens,
	AnteUp,
	Firework,
	Horseshoe,
	Cup,
	Slingshot,
	Vault,
	Bell,
	Candle,
	Hourglass,
	RainCloud,
	Droplet,
	Cherries,
	Umbrella,
	Columns,
	Tower,
	Card,
	Rope,
	Ornament,
	Stocking,
	Glass,
	Clock,
	Lock,
	Mistletoe,
	Equinox,
	TrafficLight,
	Trophy,
	Spade,
	Sprout,
	Count,
};

/** One glyph, white on whatever is behind it with its stones in Jewel (the gallery draws them all). */
SHORTSTACKCORE_API void DrawGlyph(Canvas& C, Glyph G, float Cx, float Cy, float Size, uint32_t Jewel = 0xf2c14e);
/** The emblem of an event (a tile or, for a series event, a crest), Size across, centered at (Cx, Cy). */
SHORTSTACKCORE_API void Emblem(Canvas& C, const net::EventTemplate& T, float Cx, float Cy, float Size, double Time);
/** A series' crest (the series page's banner, the picker, the home page). */
SHORTSTACKCORE_API void SeriesCrest(Canvas& C, const net::SeriesInfo& S, float Cx, float Cy, float Size, double Time);
/** The glyph a series is known by (its motif). */
SHORTSTACKCORE_API Glyph SeriesGlyph(const net::SeriesInfo& S);
} // namespace eventart
} // namespace ui
} // namespace ss
