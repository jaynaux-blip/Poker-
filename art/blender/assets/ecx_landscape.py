"""THE EMBERCREST's landscape (spec ec_exterior_spec.md, section 3.4): the ground round the resort, the ridges, the
Crest massif, the valley and the city 900 m below, the mist, the slope lights, the trees and the rocks.

One world, many meshes. Every terrain mesh samples the same height function (world_height), so where two meshes meet
their surfaces meet; their open edges carry skirts (both faces) so no crack ever shows the sky. Map frame (spec 0.1),
meters: X west (out of the doors, toward the valley), Y north, Z up from the entrance threshold. Blender x = -X,
y = Y, z = Z: SITE meshes go in at (0,0,0) yaw 180; trees and rocks are PROP frame (base center at the origin).

The geography (picked inside the spec's ranges, deviations noted):
- The resort's shelf is cut into the ridge's west flank in a saddle: north the land rises 26 m (to +24 m at
  Y 300 m: the forested side, the north bench beyond), south a forested knoll (+11 m at Y -125) the road cuts past,
  then the ground falls with the road's 4 %. By the site the lip stays at the road's level (the saddle rises east of
  the road, not over the parapet). East the flank rises behind the building (+12 m at X -60 above a rock cut, +50 m at
  400 m, +105 m at 820 m, +205 m at 1.5 km, +405 m at 2.8 km at the Crest's foot) along a hanging valley that leads
  the eye up to the Crest; the benches either side rise more (+182 m at 820 m, the spec's figure). Deviation: the
  spec's "+180 m at 700 m behind the building" on the axis would hide the Crest below 14 deg; on the axis the bench
  stays under 9.3 deg, so the whole massif shows from its foot (8 deg) to the summit (21 deg).
- The lip (the parapet's outer face, X 64.45 for Y -205..125) meanders beyond the site; north of the resort a bench
  juts west (to X ~160 at Y 600) carrying the hamlet (10 chalets, modeled into Ridge_Near); south a rock spur
  (X 60..100, Y -230..-320) stands between the road's continuation and the drop.
- The drop: 38..40 deg to -95 m at 120 m out, 35 deg to -180 m at 240 m (crags, ledges, gullies), then forested
  slopes (30 -> 22 -> 10 deg) to the valley floor at -900 m about 2.1 km out.
- The switchbacks: 8 legs, 7 hairpins (R 16 m) from the ridge road at the plaza's south end (junction (60.5, -208))
  down to (1900, 600, -860); each leg runs along the face swinging with its folds; the road's height follows the land
  (smoothed over 160 m, always falling, grade capped at 12 %), benched in with ~40 deg cut and fill slopes and plowed
  berms. The valley road goes on west to the highway. The ridge road continues past both ends of the plaza's asphalt
  (terrain only): north climbing (2 %, then 9 %) and easing east (R 600 m) through the north stand into the bench;
  south falling 4 % into a cut, curving east (R 260 m) behind the spur.
- The Crest: one massif whose crest line rises to the central summit (-3500, -250, +1350), the north shoulder
  (-3300, +850, +940) and the south shoulder (-3700, -1310, +1035) (the mark's proportions) between notched cols
  (+830, +800); pyramidal aretes off each summit, the summit a horn (1.6 x steeper near its top); 55 deg west faces,
  buttresses, 15 couloirs, towers on the ridges; every steep face carved by fall-line erosion (erosion_noise: ribs
  and gullies that run down the slope and branch, 380 m cells to ~3 m, up to ~50 m deep), no regular banding (granite
  breaks along the fall line); the crest dies away into flanking ridges that climb again to +600..+900 at 4..11 km
  north and south. Forest line about +600 m (ragged).
- Behind (east): a high plateau at +700..+900 and pyramidal far peaks to +2400 m at 8..21 km (some show over the
  shoulders). West: the valley floor at -900 m (the river meandering through X 8.2..11.4 km, farmland in districts
  about 2.6 km across, each with its own orientation and field size, a muted October palette with margins, tree lines
  and farmsteads; woods, aspens gold on the slopes, the highway on the east bank) to the far wall rising from 16 km
  to about +300 m at 22 km, higher ranges beyond (+700 at 25..30 km, peaks to +1500). Up and down the valley ranges
  close the view at 16..30 km. The far wall, the far peaks and the high country beyond 3 km carry fall-line erosion
  too (_erode_far), so the ranges read as eroded mountains, not as noise.
- Snow above about -400 m (the resort is at 2,100 m), patchy down to -520 m; cliffs over ~40 deg bare (gullies and
  ledges hold it); high up (+450..+800 m and above) the early season's wet snow plasters the faces: it holds to ~50
  deg on open rock and past 60 deg in the gullies, the ribs and the steepest walls stay dark, with crisp edges; trees
  snow-laden above -200 m.

Meshes:
- SM_ECX_SiteTerrain (unbaked, ecx_terrain): a balanced quadtree over X, Y -448..448: 0.5 m cells within 25 m of the
  playable bounds, in the crag zone below the parapet (X 63..135) and on the cuts at the building's ends; 1 m to 60 m
  and along the roads; then 2, 4, 8, 16 m; 4 m along its outer rim. Exact holes (every edge on the 0.5 m grid): the
  hardscape outline shrunk 0.5 m (the terrain tucks 0.5 m under the ground's edge, 6 cm below it there, and meets it
  at the outline at ecx_plaza's grades: verges arm grade + 0.20, shoulders the road profile, the parapet's face
  road - 2.0, the overlook's -4.3) and the front 33 m of the building (X -29.5..2.5, Y -57.5..55.5), where the
  building's meshes stand. The back of the footprint (X -60..-30) is kept, flat at Z -0.25, under whatever covers it,
  so no void shows from the slopes at the building's ends (harmless if the building does fill it).
  UV0 = map meters / 10 (u = X/10 = -x/10, v = y/10). COLOR_0 (linear, RGB float): R rock exposure (slope, crags,
  the roads' plowed beds beyond the asphalt at 0.85: they read as dark gravel), G forest litter and shade (the
  stands, forest beds), B wind crust (exposed, west-facing, the lip). Skirts 2.5 m down its outer rim.
- SM_ECX_Ridge_Near (r < 2.5..3 km, all round, beyond the Site's square), SM_ECX_Ridge_Mid (east, 84.8..275.2 deg
  yaw, r to 8 km: the Crest and the flanking ridges), SM_ECX_Valley (west, |yaw| < 84.8 deg, r 2.5..20 km, the
  city's sector cut out), SM_ECX_ValleyCity (yaw -50..56 deg, X 6..16 km), SM_ECX_Ridge_Far (beyond all of them to
  the 60 km square): polar meshes round the plaza (spokes every 0.45 / 0.2 / 0.33 / 0.4 / 0.4 deg, rings growing
  1.1 / 0.5 / 0.78 / 2.8 / 0.8 % a step), their sectors exact at every boundary. Their textures (4096) are computed
  per texel in numpy, not baked: the same heights, slopes and concavities (15 and 40 m), forest crowns (one per 4.2 m
  cell, 9..18 m tall, snow on their tops high up; where the view from the plaza grazes a forested slope the snow
  between the crowns hides behind the trees), snow, granite, scree, fields, the river, roads (cleared corridors either
  side, bed edges as soft as a texel); laid out in banded
  polar UVs: u by azimuth, v outward with a density that follows the screen height the ground takes seen from the
  plaza, so a texel is about a screen pixel; 3 or 4 azimuth bands stacked in v with 24-texel gutters of real content.
  Normal maps (OpenGL) hold the relief finer than the mesh (crowns, crags, couloirs) in the mesh's tangent space.
  The meshes sample the roads' benches softened by their own vertex spacing (world_height's soft, never inside the
  Site's square nor at its seam), so a coarse mesh can't alias a road cut into a saw-tooth; the texture keeps the
  crisp bench.
  Material: the glTF material M_<mesh>, single slot, like a baked mesh.
- SM_ECX_ValleyCity [lit]: by day roofs, streets (a 100 x 120 m grid downtown, 170..270 m out, arterials every
  800 m), parks, the river, embankments, bridges, the rail yard, the freeway; by night the emissive (strength 3.5):
  sodium and LED street lights in dots every 35 m, arterials brighter, lit windows, freeway lines (headlights north,
  tails south), the bridges' strings; the city sprawls along the freeway into satellite towns. 44 downtown towers
  (45..200 m, some with a setback) with glass walls and lit windows in the texture's top 256 rows; aviation lights on
  the tallest.
- SM_ECX_ValleyMist (unbaked ecx_mist, alpha): three domed sheets over the valley (Z -735, -680, -630) and two banks
  along the slope below the parapet (Z -112, -78); UV 0..1 per sheet for the edge fade.
- SM_ECX_SlopeLights_GlowWarm: ~150 switchback lamps 7.5 m over the road's downhill edge every 60 m (the 10 nearest
  the plaza cobra heads, the rest 1.2 m octahedra), the hamlet's lit windows (quads 6 cm proud of the chalets' walls,
  over the window frames in their texture), the lit run's 38 floodlights. Hide it by day (an unlit octahedron
  floating over the slope would show).
- Trees (unbaked, slots 0 ecx_bark, 1 ecx_needles): Engelmann spruces as narrow spires of whorled limbs that droop
  low and lift high, each limb carrying a plate of side branchlets with sprays all along them (some turned on edge,
  a hanging under-layer, two broad fans hung under each limb's inner two thirds so the crown reads dense, not lacy),
  snow pillows riding the limbs, dead twigs low by the trunk, a leader and cones; the
  lodgepole a straight trunk bare to two thirds with stubs and a narrow crown of upswept limbs ending in clumps of
  tufts. Cards are cut to their spray's outline row by row (less overdraw), bent (droop), their normals bent outward
  from the trunk (55 %) for soft foliage light. Bark UVs: u round (3 repeats on the trunk), v = meters up (the tile
  is 1 m). The exported materials are plain; the review shows them with the atlas.
- SM_ECX_ForestClump_A/B: opaque tiered skirts, baked (clump 2048, bark 512). SM_ECX_Rock_A/B/C: faceted granite
  blocks shrink-wrapped by cube-spheres onto a signed distance (polytopes with crisp facets on A and C, rounded on the
  B boulders; joint sets, ledges, erosion), snow laid on the ledges with its edge cut along a smoothed contour as a
  pillow (a rounded bull-nose at the edge, domed inward, rolled over the rock along the normal), baked (rock
  4096/2048/1024, snow 2048/1024/512). Their granite: warm grey mottling and buff staining, low-contrast salt and
  pepper crystals, water streaks and black lichen down the steep faces, lobed grey-green and orange crusts on the
  tops, dark crevices, rime only in the tightest cracks.

Textures written to unreal/Art/Textures/ECX/ (spec 3.4):
- T_ECX_Needles_BaseColor (RGBA, A coverage), _Normal, _Mask (R AO, G roughness, B snow, A thickness), 4096: a 4 x 4
  grid of 1024 cells, each 0.52 m of spray, rendered from modeled 3D sprays (twigs, needles, snow lumps) in Cycles.
  Cells (row-major from the top-left): 0-2 frosted spruce sprays, 3-5 snow-laden spruce sprays, 6-7 pine tufts,
  8 a snowy pine tuft, 9-10 snow clumps, 11 young spruce, 12 dead twigs with beard lichen, 13 the leader, 14 a broad
  spruce fan, 15 cones. A spray's twig enters its cell at the bottom middle (u 0.5, v 0.035). B (snow) lets the
  material keep snow on the cards' upper faces only (the underside shows the same texel).
- T_ECX_Bark_* (1024 x 2048: 0.5 x 1.0 m), T_ECX_Snow_* (2048, 4 m), T_ECX_Rock_* (2048, 6 m): seamless.
- T_ECX_StarField (4096 x 2048): u = compass azimuth / 360 (0 north, 0.25 east, 0.5 south, 0.75 west); the top row
  is the zenith, the middle row the horizon. Linear RGB, about 1.0 for a magnitude-0 star; ~9,600 stars and a dust
  of faint ones along the Milky Way (SSW to NNE across the zenith).
- T_ECX_Moon (512, RGBA): a waxing gibbous (85 % lit), the lit limb to the right, maria; alpha the lit disc.
- T_ECX_ForestMask (2048, an extra): R forest density, G slope (0..60 deg), B above the snow line, over map X, Y
  -3000..3000 (column c: X = -3000 + 6000 (c + 0.5) / 2048; row r from the top: Y = 3000 - 6000 (r + 0.5) / 2048),
  for scattering the clumps where the forest is.
Normal maps are OpenGL (+Y = +V); the setup flips green for DirectX.

Review: art/review/ecx_landscape.jpg (the props), ecx_landscape_vistas.jpg (the landscape assembled as the game
places it, the building and the plaza imported when they exist: reveal, crane, dusk overlook, night valley, night
north, the north stand, the Crest at noon and in the alpenglow), ecx_landscape_textures.jpg.

Development: ECX_ONLY=SM_ECX_Rock_A,SM_ECX_Spruce_Tall builds a subset; ECX_FAST=1 renders textures at a quarter
size; ECX_REUSE_TEX=1 reads the needle atlas back instead of rendering it; ECX_CELLS=0,3 renders only those cells;
ECX_NO_VISTAS=1 skips the vistas.
"""
import math
import os
import random
import struct
import sys
import time
import zlib

import bmesh
import bpy  # noqa: F401
import numpy as np
from mathutils import Matrix, Vector

from artkit import core, parts
from artkit.shade import noise as snoise, object_coords, ramp, scaled, vec

ART = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if ART not in sys.path:
    sys.path.insert(0, ART)

MESHES = ['SM_ECX_SiteTerrain', 'SM_ECX_Ridge_Near', 'SM_ECX_Ridge_Mid', 'SM_ECX_Ridge_Far', 'SM_ECX_Valley', 'SM_ECX_ValleyCity',
          'SM_ECX_ValleyMist', 'SM_ECX_SlopeLights_GlowWarm', 'SM_ECX_Spruce_Tall', 'SM_ECX_Spruce_Mid', 'SM_ECX_Spruce_Young',
          'SM_ECX_Pine_Tall', 'SM_ECX_ForestClump_A', 'SM_ECX_ForestClump_B', 'SM_ECX_Rock_A', 'SM_ECX_Rock_B', 'SM_ECX_Rock_C']
DOUBLE_SIDED = False
TEXTURE_SIZE = 2048
# Baked props (material name -> size). The terrain meshes' textures are computed here, not baked (see TERRAIN_TEX).
TEXTURE_SIZES = {'ecx_clump_a': 2048, 'ecx_clump_b': 2048, 'ecx_clump_bark_a': 512, 'ecx_clump_bark_b': 512,
                 'ecx_rock_a': 4096, 'ecx_rock_snow_a': 2048, 'ecx_rock_b': 2048, 'ecx_rock_snow_b': 1024,
                 'ecx_rock_c': 1024, 'ecx_rock_snow_c': 512}
AO_DISTANCE = 2.0
REVIEW_SCREEN_LIGHT = False
REVIEW_VIEWS = None  # set by pose_for_review

FAST = bool(os.environ.get('ECX_FAST'))
ONLY = [s.strip() for s in os.environ.get('ECX_ONLY', '').split(',') if s.strip()]
TEX_OUT = os.path.join(core.ROOT, 'unreal', 'Art', 'Textures', 'ECX')
EYE = np.array([45.0, 0.0, 1.0])     # where the textures' texel density is measured from (the plaza, eye height)


def log(*a):
    print('[ecx_landscape]', *a, flush=True)


def wanted(name):
    return not ONLY or name in ONLY


# ====================================================================== noise (numpy)

_GX = np.array([math.cos(k * math.pi / 4 + math.pi / 8) for k in range(8)], dtype=np.float32)
_GY = np.array([math.sin(k * math.pi / 4 + math.pi / 8) for k in range(8)], dtype=np.float32)


def _hash(ix, iy, seed):
    """uint32 hash of integer lattice coordinates."""
    h = (ix.astype(np.int64) * 0x8DA6B343 + iy.astype(np.int64) * 0xD8163841 + (seed * 0x9E3779B9 + 0x7F4A7C15)) & 0xFFFFFFFF
    h = h.astype(np.uint32)
    h ^= h >> np.uint32(15)
    h *= np.uint32(0x2C1B3C6D)
    h ^= h >> np.uint32(12)
    h *= np.uint32(0x297A2D39)
    h ^= h >> np.uint32(15)
    return h


def hash01(ix, iy, seed):
    return (_hash(ix, iy, seed) >> np.uint32(8)).astype(np.float32) / float(1 << 24)


def _fade(t):
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0)


def _dfade(t):
    return 30.0 * t * t * (t * (t - 2.0) + 1.0)


def gnoise(x, y, seed=0, period=None, deriv=False):
    """2D gradient noise in about -1..1 at float64 coordinates (lattice units). period: tile every `period` cells.
    deriv: also return d/dx, d/dy (in lattice units)."""
    xf = np.floor(x)
    yf = np.floor(y)
    fx = (x - xf).astype(np.float32)
    fy = (y - yf).astype(np.float32)
    ix = xf.astype(np.int64)
    iy = yf.astype(np.int64)
    ix1 = ix + 1
    iy1 = iy + 1
    if period:
        ix, ix1, iy, iy1 = ix % period, ix1 % period, iy % period, iy1 % period
    h00 = _hash(ix, iy, seed) & np.uint32(7)
    h10 = _hash(ix1, iy, seed) & np.uint32(7)
    h01 = _hash(ix, iy1, seed) & np.uint32(7)
    h11 = _hash(ix1, iy1, seed) & np.uint32(7)
    gx00, gy00, gx10, gy10 = _GX[h00], _GY[h00], _GX[h10], _GY[h10]
    gx01, gy01, gx11, gy11 = _GX[h01], _GY[h01], _GX[h11], _GY[h11]
    n00 = gx00 * fx + gy00 * fy
    n10 = gx10 * (fx - 1.0) + gy10 * fy
    n01 = gx01 * fx + gy01 * (fy - 1.0)
    n11 = gx11 * (fx - 1.0) + gy11 * (fy - 1.0)
    u = _fade(fx)
    v = _fade(fy)
    nx0 = n00 + u * (n10 - n00)
    nx1 = n01 + u * (n11 - n01)
    n = (nx0 + v * (nx1 - nx0)) * 1.5
    if not deriv:
        return n
    du = _dfade(fx)
    dv = _dfade(fy)
    dx0 = gx00 + u * (gx10 - gx00) + du * (n10 - n00)
    dx1 = gx01 + u * (gx11 - gx01) + du * (n11 - n01)
    dy0 = gy00 + u * (gy10 - gy00)
    dy1 = gy01 + u * (gy11 - gy01)
    dndx = (dx0 + v * (dx1 - dx0)) * 1.5
    dndy = (dy0 + v * (dy1 - dy0) + dv * (nx1 - nx0)) * 1.5
    return n, dndx, dndy


def _octave_weight(lam, mw):
    """How much of an octave of wavelength lam a surface sampled every mw/2 meters keeps: 1 above mw, fading to 0 at
    mw/2; None when no element keeps any (mw may be an array: per-point cut-offs)."""
    if np.isscalar(mw):
        if lam < mw * 0.5:
            return None
        return 1.0 if lam >= mw else (lam - 0.5 * mw) / (0.5 * mw)
    if lam < 0.5 * float(np.min(mw)):
        return None
    return np.clip((lam - 0.5 * mw) / (0.5 * mw + 1e-9), 0.0, 1.0).astype(np.float32)


_ROT = [(math.cos(a), math.sin(a)) for a in [0.0] + [math.radians(37.0 + 61.0 * k) for k in range(1, 24)]]


def fbm(X, Y, wavelength, octaves, seed=0, gain=0.5, lacunarity=2.03, min_wavelength=0.0):
    """Fractal sum of gradient noise over world meters; octaves finer than min_wavelength are left out (the last one
    kept fades in over a factor of 2), so a mesh can take the coarse part and its texture the rest. About -1..1."""
    out = np.zeros(np.shape(X), dtype=np.float32)
    amp, total, lam = 1.0, 0.0, wavelength
    for o in range(octaves):
        total += amp
        w = _octave_weight(lam, min_wavelength)
        if w is None:
            break
        c, s = _ROT[o % len(_ROT)]
        xs = (X * c - Y * s) / lam + 17.31 * o
        ys = (X * s + Y * c) / lam - 9.73 * o
        out += (amp * w) * gnoise(xs, ys, seed + 101 * o)
        amp *= gain
        lam /= lacunarity
    return out / np.float32(total)


def fbm_split(X, Y, wavelength, octaves, seed, split, gain=0.5, lacunarity=2.03):
    """(coarse, fine) parts of fbm: octaves at or above `split` meters and those below it (sum = the full fbm)."""
    full = fbm(X, Y, wavelength, octaves, seed, gain, lacunarity)
    coarse = fbm(X, Y, wavelength, octaves, seed, gain, lacunarity, min_wavelength=split)
    return coarse, full - coarse


def ridged(X, Y, wavelength, octaves, seed=0, gain=0.5, lacunarity=2.1, min_wavelength=0.0):
    """Ridged multifractal (sharp crests, soft valleys), 0..1."""
    out = np.zeros(np.shape(X), dtype=np.float32)
    weight = np.ones(np.shape(X), dtype=np.float32)
    amp, total, lam = 1.0, 0.0, wavelength
    for o in range(octaves):
        total += amp
        w = _octave_weight(lam, min_wavelength)
        if w is None:
            break
        c, s = _ROT[(o + 5) % len(_ROT)]
        xs = (X * c - Y * s) / lam + 3.7 * o
        ys = (X * s + Y * c) / lam + 8.1 * o
        n = 1.0 - np.abs(gnoise(xs, ys, seed + 37 * o))
        n = n * n
        n *= weight
        weight = np.clip(n * 1.6, 0.0, 1.0)
        out += (amp * w) * n
        amp *= gain
        lam /= lacunarity
    return out / np.float32(total)


def eroded(X, Y, wavelength, octaves, seed=0, gain=0.5, lacunarity=2.0, min_wavelength=0.0, sharp=1.0):
    """Derivative-damped fractal noise: the octaves fade where the slope already is steep, which gives smooth valley
    floors and sharp, branching ridges, the look of eroded rock. About -1..1."""
    out = np.zeros(np.shape(X), dtype=np.float32)
    dx = np.zeros(np.shape(X), dtype=np.float32)
    dy = np.zeros(np.shape(X), dtype=np.float32)
    amp, total, lam = 1.0, 0.0, wavelength
    for o in range(octaves):
        total += amp
        w = _octave_weight(lam, min_wavelength)
        if w is None:
            break
        c, s = _ROT[(o + 11) % len(_ROT)]
        xs = (X * c - Y * s) / lam + 5.3 * o
        ys = (X * s + Y * c) / lam - 2.9 * o
        n, nx, ny = gnoise(xs, ys, seed + 53 * o, deriv=True)
        # Derivatives back to the unrotated frame, per unit of the first octave's wavelength.
        gx = (nx * c + ny * s) * (wavelength / lam)
        gy = (-nx * s + ny * c) * (wavelength / lam)
        dx += np.float32(amp) * gx
        dy += np.float32(amp) * gy
        out += (amp * w) * n / (1.0 + sharp * (dx * dx + dy * dy))
        amp *= gain
        lam /= lacunarity
    return out / np.float32(total)


def erosion_noise(X, Y, gx, gy, cell, octaves, seed, gain=0.5, lacunarity=2.0, slope_k=1.0, branch=1.5, min_wavelength=0.0):
    """Fall-line erosion (after Fewes' erosion noise) over world meters: each octave lays stripes whose crests run
    down the local slope (the big forms' gradient gx, gy in m/m, plus what the coarser octaves carved), phase-jittered
    per jittered cell and blended by a Gaussian, so ribs and gullies run downhill and branch. The stripes' spacing
    follows the steepness (none on the flat). About -1..1 (1 on the ribs, -1 in the gullies)."""
    X = np.asarray(X, dtype=np.float64)
    Y = np.asarray(Y, dtype=np.float64)
    val = np.zeros(X.shape, dtype=np.float32)
    ddx = np.zeros(X.shape, dtype=np.float32)
    ddy = np.zeros(X.shape, dtype=np.float32)
    amp, total, f = 1.0, 0.0, 1.0
    for o in range(octaves):
        lam = cell / f
        w_o = _octave_weight(lam * 0.7, min_wavelength)
        total += amp
        if w_o is None:
            break
        c, s = _ROT[(o + 3) % len(_ROT)]
        px = (X * c - Y * s) / lam + 11.7 * o
        py = (X * s + Y * c) / lam - 4.3 * o
        # The stripes' direction: along the contour of the big forms plus what the coarser octaves carved (rotated
        # into this octave's frame).
        ex = gy * slope_k + ddy * branch
        ey = -gx * slope_k - ddx * branch
        dx = (ex * c - ey * s).astype(np.float32)
        dy = (ex * s + ey * c).astype(np.float32)
        ix = np.floor(px + 0.5).astype(np.int64)
        iy = np.floor(py + 0.5).astype(np.int64)
        fx = (px - ix).astype(np.float32)
        fy = (py - iy).astype(np.float32)
        va = np.zeros(X.shape, dtype=np.float32)
        vb = np.zeros(X.shape, dtype=np.float32)
        wt = np.zeros(X.shape, dtype=np.float32)
        for oi in (-1, 0, 1):
            for oj in (-1, 0, 1):
                hx = (hash01(ix + oi, iy + oj, seed + 17 * o) - 0.5) * 0.5
                hy = (hash01(ix + oi, iy + oj, seed + 17 * o + 1) - 0.5) * 0.5
                qx = fx - oi - hx
                qy = fy - oj - hy
                w = np.exp(-2.0 * (qx * qx + qy * qy))
                ph = (qx * dx + qy * dy) * np.float32(2.0 * math.pi)
                va += np.cos(ph) * w
                vb -= np.sin(ph) * w
                wt += w
        va /= wt
        vb /= wt
        val += (np.float32(amp) * w_o) * va
        # This octave's carved slope back in the map frame, for the finer octaves' directions.
        ddx += (np.float32(amp * f) * vb * (dx * c + dy * s)).astype(np.float32)
        ddy += (np.float32(amp * f) * vb * (-dx * s + dy * c)).astype(np.float32)
        amp *= gain
        f *= lacunarity
    return val / np.float32(total)


def cells(X, Y, size, seed=0, jitter=0.85):
    """Jittered-grid Voronoi over world meters: (F1 distance, F2 distance, nearest cell's random 0..1, dx, dy to it)."""
    gx = X / size
    gy = Y / size
    cx = np.floor(gx).astype(np.int64)
    cy = np.floor(gy).astype(np.int64)
    f1 = np.full(np.shape(X), 1e9, dtype=np.float32)
    f2 = np.full(np.shape(X), 1e9, dtype=np.float32)
    rid = np.zeros(np.shape(X), dtype=np.float32)
    ddx = np.zeros(np.shape(X), dtype=np.float32)
    ddy = np.zeros(np.shape(X), dtype=np.float32)
    fx = (gx - cx).astype(np.float32)
    fy = (gy - cy).astype(np.float32)
    for ox in (-1, 0, 1):
        for oy in (-1, 0, 1):
            ix = cx + ox
            iy = cy + oy
            px = ox + 0.5 + jitter * (hash01(ix, iy, seed) - 0.5)
            py = oy + 0.5 + jitter * (hash01(ix, iy, seed + 1) - 0.5)
            ex = px - fx
            ey = py - fy
            d = np.sqrt(ex * ex + ey * ey) * np.float32(size)
            closer = d < f1
            f2 = np.where(closer, f1, np.minimum(f2, d))
            rid = np.where(closer, hash01(ix, iy, seed + 2), rid)
            ddx = np.where(closer, ex * np.float32(size), ddx)
            ddy = np.where(closer, ey * np.float32(size), ddy)
            f1 = np.where(closer, d, f1)
    return f1, f2, rid, ddx, ddy


def smoothstep(e0, e1, x):
    t = np.clip((np.asarray(x, dtype=np.float32) - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def pchip(xk, yk):
    """Monotone cubic (Fritsch-Carlson) through the knots; returns f(x), flat beyond the ends."""
    xk = np.asarray(xk, dtype=np.float64)
    yk = np.asarray(yk, dtype=np.float64)
    h = np.diff(xk)
    d = np.diff(yk) / h
    m = np.zeros_like(yk)
    m[0], m[-1] = d[0], d[-1]
    for i in range(1, len(xk) - 1):
        if d[i - 1] * d[i] <= 0:
            m[i] = 0.0
        else:
            w1 = 2 * h[i] + h[i - 1]
            w2 = h[i] + 2 * h[i - 1]
            m[i] = (w1 + w2) / (w1 / d[i - 1] + w2 / d[i])

    def f(x):
        x = np.asarray(x, dtype=np.float64)
        xc = np.clip(x, xk[0], xk[-1])
        i = np.clip(np.searchsorted(xk, xc) - 1, 0, len(xk) - 2)
        t = (xc - xk[i]) / h[i]
        t2, t3 = t * t, t * t * t
        y = (2 * t3 - 3 * t2 + 1) * yk[i] + (t3 - 2 * t2 + t) * h[i] * m[i] + (-2 * t3 + 3 * t2) * yk[i + 1] + (t3 - t2) * h[i] * m[i + 1]
        y = np.where(x < xk[0], yk[0] + (x - xk[0]) * m[0] * 0.0, y)
        return np.where(x > xk[-1], yk[-1], y).astype(np.float32)
    return f


def blur2(a, radius, axis_wrap=(False, False)):
    """Separable box blur (3 passes ~ Gaussian) over a 2D array, radius in cells (each axis (ry, rx) or one int)."""
    ry, rx = (radius, radius) if np.isscalar(radius) else radius
    out = a.astype(np.float32)
    for _ in range(3):
        for axis, r, wrap in ((0, int(ry), axis_wrap[0]), (1, int(rx), axis_wrap[1])):
            if r < 1:
                continue
            n = out.shape[axis]
            if wrap:
                pad = np.concatenate([np.take(out, range(n - r, n), axis=axis), out, np.take(out, range(0, r + 1), axis=axis)], axis=axis)
            else:
                first = np.take(out, [0], axis=axis)
                last = np.take(out, [n - 1], axis=axis)
                pad = np.concatenate([np.repeat(first, r, axis=axis), out, np.repeat(last, r + 1, axis=axis)], axis=axis)
            c = np.cumsum(pad, axis=axis, dtype=np.float64)
            hi = np.take(c, range(2 * r + 1, 2 * r + 1 + n), axis=axis)
            lo = np.take(c, range(0, n), axis=axis)
            out = ((hi - lo) / (2 * r + 1)).astype(np.float32)
    return out


# ====================================================================== geography (map meters)

FLOOR_Z = -900.0
LIP_X = 64.45                    # the parapet's outer face
SITE_HALF = 448.0                # SM_ECX_SiteTerrain covers X, Y in [-448, 448]


def road_z(Y):
    """The ridge road's surface along Y (spec 2.2): -2.40 at the stop, 4 % down south of Y -42, 2 % up north of +60."""
    Y = np.asarray(Y, dtype=np.float32)
    z = np.full(Y.shape, -2.40, dtype=np.float32)
    z = np.where(Y < -42.0, -2.40 - 0.04 * (-42.0 - Y), z)
    return np.where(Y > 60.0, -2.40 + 0.02 * (Y - 60.0), z)


def _bump(x, c, w):
    return np.exp(-((np.asarray(x, dtype=np.float32) - c) / w) ** 2)


def edge_x(Y):
    """The lip of the drop (map X) along Y: the parapet at the site, a bench jutting west north of the resort, the
    spur to the south, a slow meander beyond."""
    Y = np.asarray(Y, dtype=np.float32)
    fade_n = smoothstep(125.0, 300.0, Y)
    fade_s = smoothstep(-205.0, -330.0, Y)
    far = 46.0 * np.sin(Y / 1310.0 + 0.7) + 24.0 * np.sin(Y / 530.0 + 2.1) + 11.0 * np.sin(Y / 211.0 + 0.3)
    far = far - (46.0 * math.sin(0.7) + 24.0 * math.sin(2.1) + 11.0 * math.sin(0.3))
    x = LIP_X + (fade_n + fade_s) * far
    x = x + 95.0 * fade_n * _bump(Y, 620.0, 300.0)
    x = x + 22.0 * smoothstep(-210.0, -250.0, Y) * _bump(Y, -275.0, 55.0)
    return x


# The drop's profile: height below the lip (normalized to the floor) by distance west of it, meters.
_DROP = pchip([0.0, 30.0, 120.0, 240.0, 500.0, 900.0, 1400.0, 1900.0, 2400.0, 3200.0],
              [0.0, -24.5, -95.0, -180.0, -330.0, -520.0, -715.0, -858.0, -897.0, -900.0])


def saddle(Y):
    """The resort sits in a saddle on the ridge's flank: north the land rises 26 m (the north bench beyond); south a
    forested knoll (+11 m at Y -125) the road cuts past, then the ground falls with the road's 4 % south."""
    Y = np.asarray(Y, dtype=np.float32)
    north = 26.0 * (1.0 - np.exp(-(np.maximum(Y, 0.0) / 170.0) ** 2))
    knoll = 11.5 * np.exp(-((Y + 125.0) / 82.0) ** 2) * smoothstep(0.0, -55.0, Y)
    t = -42.0 - Y
    soft = 30.0 * np.logaddexp(0.0, t / 30.0) - 30.0 * np.logaddexp(0.0, (t - 420.0) / 30.0)
    fall = -0.04 * soft + 10.0 * smoothstep(-380.0, -900.0, Y)
    return north + knoll + fall




# How the flank rises east of the lip (distance east of it, m): along the corridor that leads the eye up to the Crest
# (a hanging valley draining its west faces toward the resort) and on the benches either side of it.
_RISE_CORR = pchip([0.0, 40.0, 124.0, 220.0, 400.0, 824.0, 1500.0, 2000.0, 2500.0, 2800.0, 3400.0],
                   [-3.0, -1.2, 12.0, 28.0, 50.0, 105.0, 205.0, 282.0, 358.0, 405.0, 470.0])
_RISE_SIDE = pchip([0.0, 40.0, 124.0, 220.0, 400.0, 824.0, 1500.0, 2300.0, 3000.0, 3600.0],
                   [-3.0, -1.2, 14.0, 42.0, 95.0, 182.0, 290.0, 395.0, 452.0, 500.0])
AXIS_SLOPE = -250.0 / -3550.0     # the sight line from the plaza (X 50) to the Crest's summit, dY/dX


def _flat(*arrays):
    shape = np.shape(arrays[0])
    return shape, [np.ascontiguousarray(np.asarray(a, dtype=np.float32).ravel()) for a in arrays]


PEAKS = [  # the Crest (map m): X, Y, Z, the face slope (tan), aretes' rotation
    (-3500.0, -250.0, 1350.0, 1.50, 0.42),
    (-3300.0, 850.0, 940.0, 1.32, 1.10),
    (-3700.0, -1310.0, 1035.0, 1.38, 0.66),
]
CREST_BOX = (-6200.0, -1500.0, -5200.0, 4200.0)     # X0, X1, Y0, Y1 where the massif is evaluated
FAR_PEAKS = [  # behind the Crest, up and down the valley, over the far wall: X, Y, Z, radius scale
    (-10000.0, 1300.0, 2350.0, 1.0), (-11000.0, 5000.0, 2400.0, 1.1), (-13000.0, -6500.0, 2300.0, 1.2),
    (-15500.0, 1800.0, 2250.0, 1.3), (-9000.0, -3600.0, 1900.0, 0.9), (-17000.0, -2500.0, 2150.0, 1.3),
    (-12500.0, 9500.0, 2100.0, 1.2), (-8500.0, 7800.0, 1700.0, 0.9), (-20000.0, 8000.0, 2300.0, 1.4),
    (-7800.0, -9200.0, 1650.0, 1.0), (-14500.0, -13000.0, 2200.0, 1.3), (-21000.0, -9000.0, 2350.0, 1.4),
    (6000.0, 26000.0, 1700.0, 1.4), (14000.0, 27500.0, 1900.0, 1.5), (-2000.0, 24000.0, 1500.0, 1.2),
    (3000.0, -25000.0, 1650.0, 1.3), (12000.0, -27000.0, 2000.0, 1.6), (-4000.0, -23000.0, 1400.0, 1.2),
    (28000.0, -6000.0, 1300.0, 1.5), (29000.0, 8000.0, 1450.0, 1.6), (31000.0, 1000.0, 1250.0, 1.4),
    (27000.0, 17000.0, 1500.0, 1.5), (26500.0, -16000.0, 1350.0, 1.4),
]


def smax(a, b, k):
    """Smooth maximum (k meters of blend)."""
    return np.logaddexp(a / k, b / k) * k


def _pyramid(X, Y, px, py, pz, slope, rot, k=24.0):
    """A pyramidal peak: four planar faces meeting in aretes, softened by k meters."""
    dx = X - px
    dy = Y - py
    acc = None
    for i in range(4):
        a = rot + 0.5 * math.pi * i
        d = dx * math.cos(a) + dy * math.sin(a)
        acc = d if acc is None else smax(acc, d, k)
    # A horn: steeper toward the summit (1.6 x the face slope at the top, the face slope 250 m down).
    return pz - slope * acc * (1.0 + 0.6 * np.exp(-np.maximum(acc, 0.0) / 250.0))


def _massif_base(X, Y):
    """The Crest's big forms (flat float32 arrays): the crest line through the three summits, the pyramidal peaks and
    their aretes, the buttresses off the west faces. Returns (height, warped X, warped Y, distance across the crest)."""
    wx = 210.0 * fbm(X, Y, 2400.0, 3, seed=71)
    wy = 160.0 * fbm(X + 911.0, Y, 2400.0, 3, seed=72)
    Xw, Yw = X + wx, Y + wy
    across = Xw - _crest_x(Yw)
    # West faces steeper than the east: a crest, not a dome.
    s = np.where(across > 0.0, 1.38, 1.05) * (1.0 + 0.18 * fbm(X, Y, 900.0, 2, seed=77))
    h = _CREST_Z(Yw) - s * np.abs(across)
    for (px, py, pz, sl, rot) in PEAKS:
        h = smax(h, _pyramid(Xw, Yw, px, py, pz + 15.0, sl, rot), 18.0)
    # Buttresses: spurs running west off the faces, gullies between them.
    spur = 70.0 * ridged(Yw / 1.0, Xw * 0.15, 520.0, 3, seed=78) * smoothstep(0.0, 300.0, across) * smoothstep(1400.0, 500.0, across)
    return h + spur, Xw, Yw, across


def massif(X, Y, lam=0.0):
    """The Crest (flat float32 arrays): one massif whose crest line rises to three summits (the mark's proportions)
    between notched cols, pyramidal aretes off each summit, towers on the ridges, the big couloirs from notches high on
    the crest; every steep face carved by fall-line erosion into ribs and branching gullies (no regular banding:
    granite breaks along the fall line); the crest dies away north and south into the flanking ridges."""
    h, Xw, Yw, across = _massif_base(X, Y)
    # The big forms' slope, for the erosion's direction (the base is smooth at this step).
    e = 12.0
    gx = (_massif_base(X + e, Y)[0] - h) / e
    gy = (_massif_base(X, Y + e)[0] - h) / e
    steep = np.sqrt(gx * gx + gy * gy)
    h = h + 30.0 * eroded(Y, X * 0.25, 420.0, 5, seed=76, min_wavelength=lam) * smoothstep(500.0, 800.0, _CREST_Z(Yw))
    # Couloirs down the west faces, from notches high on the crest.
    gully = np.zeros(X.shape, dtype=np.float32)
    rng = random.Random(1350)
    for k in range(15):
        gy0 = -2400.0 + 3500.0 * (k + 0.15 + rng.random() * 0.7) / 15.0
        top = _crest_x(np.float32(gy0)) + 40.0 + 80.0 * rng.random()
        wdt = 13.0 + 15.0 * rng.random()
        dep = 16.0 + 24.0 * rng.random()
        bend = 0.15 + 0.3 * rng.random()
        sel = np.abs(Yw - gy0) < 700.0
        if not np.any(sel):
            continue
        xs = Xw[sel]
        run = np.maximum(xs - top, 0.0)
        lat = Yw[sel] - (gy0 + bend * run * (1 if k % 2 else -1) + 45.0 * np.sin(run / 210.0 + k))
        down = smoothstep(top - 40.0, top + 80.0, xs) * (1.0 - smoothstep(top + 500.0, top + 820.0, xs))
        gully[sel] = np.maximum(gully[sel], dep * down * np.clip(1.0 - (lat / (wdt * (1.0 + run / 1400.0))) ** 2, 0.0, 1.0))
    h = h - gully * smoothstep(420.0, 650.0, h)
    relief = smoothstep(250.0, 900.0, h)
    h = h + (25.0 + 45.0 * relief) * eroded(X, Y, 1300.0, 6, seed=74, min_wavelength=lam, sharp=1.4)
    # Fall-line erosion on the steep faces, strongest high up; eased on the crest line itself (the gullies of both
    # faces would notch it like a saw) and varied in depth across the faces.
    on_crest = smoothstep(25.0, 140.0, np.abs(across))
    face = smoothstep(0.35, 0.9, steep) * smoothstep(380.0, 650.0, h) * (0.75 + 0.25 * on_crest)
    sel = face > 0.0
    if np.any(sel):
        ev = erosion_noise(X[sel], Y[sel], gx[sel], gy[sel], 380.0, 7, 1900, gain=0.55, slope_k=1.15, branch=1.6, min_wavelength=_sub(lam, sel))
        depth = 0.65 + 0.5 * smoothstep(-0.4, 0.4, fbm(X[sel], Y[sel], 700.0, 2, seed=1907))
        h[sel] += face[sel] * depth * 50.0 * (ev - 0.45)
    return h


def _far_peaks_raw(X, Y, west, lam):
    env = np.full(X.shape, -1e4, dtype=np.float32)
    amp = np.zeros(X.shape, dtype=np.float32)
    rng = random.Random(77 if west else 78)
    for (px, py, pz, sc) in FAR_PEAKS:
        if (px > 0) != west:
            continue
        rot = rng.random() * 1.6
        slope = (0.85 if west else 0.95) / sc + 0.15 * rng.random()
        p = _pyramid(X, Y, px, py, pz, slope, rot, k=80.0 * sc)
        env = smax(env, p, 60.0)
        d = np.sqrt((X - px) ** 2 + (Y - py) ** 2) / (3000.0 * sc)
        amp = np.maximum(amp, (240.0 if west else 300.0) * sc * np.exp(-d))
    out = env - 60.0
    sel = amp > 20.0
    if np.any(sel):
        out[sel] += amp[sel] * (ridged(X[sel], Y[sel], 2600.0, 8, seed=91 if west else 92, min_wavelength=_sub(lam, sel)) - 0.35)
    return out


def _erode_far(X, Y, z, base_fn, lam, seed, amp=200.0, cell=1300.0):
    """Fall-line erosion for the far ranges: the coarse land's slope (base_fn sampled smooth at 500 m) steers ribs and
    gullies down every steep flank, so the ranges read as eroded mountains, not as noise."""
    e = 60.0
    h0 = base_fn(X, Y, 500.0)
    gx = (base_fn(X + e, Y, 500.0) - h0) / e
    gy = (base_fn(X, Y + e, 500.0) - h0) / e
    face = smoothstep(0.12, 0.5, np.sqrt(gx * gx + gy * gy))
    sel = face > 0.0
    if np.any(sel):
        # slope_k high: these flanks are gentler than the Crest's, and their gullies must still be cell-sized.
        ev = erosion_noise(X[sel], Y[sel], gx[sel], gy[sel], cell, 6, seed, gain=0.55, slope_k=3.0, branch=1.4, min_wavelength=_sub(lam, sel))
        z = z.copy()
        z[sel] += face[sel] * amp * (ev - 0.4)
    return z


def _far_peaks(X, Y, west, lam):
    """The far peaks' heights (flat arrays; west: the valley's side (X > 0) or the east's): pyramids with eroded
    ridges, their flanks carved by fall-line erosion."""
    out = _far_peaks_raw(X, Y, west, lam)
    near = out > -400.0
    if np.any(near):
        out[near] = _erode_far(X[near], Y[near], out[near], lambda x, y, lm: _far_peaks_raw(x, y, west, lm), _sub(lam, near),
                               2300 if west else 2400)
    return out


def _upland_base(X, Y, lam=0.0):
    """The upland's land forms before the Crest and the far peaks (flat float32 arrays)."""
    xe = edge_x(Y)
    re = np.maximum(xe - X, 0.0)
    lat = np.abs(Y - AXIS_SLOPE * (X - 50.0))
    side = smoothstep(450.0, 1500.0, lat) * smoothstep(150.0, 450.0, re)
    # By the site the lip stays down at the road's level (the road runs along it); the saddle rises east of it.
    at_lip = 1.0 - (1.0 - smoothstep(0.0, 70.0, re)) * (1.0 - smoothstep(350.0, 750.0, np.abs(Y)))
    z = _RISE_CORR(re) * (1.0 - side) + _RISE_SIDE(re) * side + saddle(Y) * at_lip
    z = z + 9.0 * _bump(Y, 620.0, 260.0) * smoothstep(-320.0, -120.0, X)
    z = z + 12.0 * np.exp(-((X - 80.0) / 24.0) ** 2 - ((Y + 275.0) / 42.0) ** 2)
    dist = np.sqrt((X - 30.0) ** 2 + Y * Y)
    far = smoothstep(450.0, 2600.0, dist)
    calm = 0.3 + 0.7 * side
    z = z + (16.0 + 120.0 * far * calm) * fbm(X, Y, 3200.0, 3, seed=11) + 50.0 * far * calm * np.sin(Y / 1900.0 + 1.3)
    z = z + (2.5 + 14.0 * far) * fbm(X, Y, 340.0, 7, seed=12, min_wavelength=lam)
    ny = smoothstep(4000.0, 11000.0, np.abs(Y))
    z = z + ny * (360.0 + 240.0 * fbm(X, Y, 5000.0, 3, seed=15))
    east = smoothstep(-4200.0, -5800.0, X)
    sel = east > 0.0
    if np.any(sel):
        plateau = 720.0 + 160.0 * fbm(X[sel], Y[sel], 6000.0, 3, seed=13) + 110.0 * eroded(X[sel], Y[sel], 1800.0, 7, seed=14, min_wavelength=_sub(lam, sel))
        z[sel] = z[sel] * (1.0 - east[sel]) + plateau * east[sel]
    return z


def upland(X, Y, lam=0.0):
    """East of the lip (flat float32 arrays): the flank rising to the Crest, the corridor and the benches, the saddle, the
    north bench, the spur; the high country behind (its flanks carved by fall-line erosion beyond 3 km)."""
    z = _upland_base(X, Y, lam)
    dist = np.sqrt((X - 30.0) ** 2 + Y * Y)
    sel = dist > 3000.0
    if np.any(sel):
        hc = smoothstep(3000.0, 4500.0, dist[sel])
        ez = _erode_far(X[sel], Y[sel], z[sel], _upland_base, _sub(lam, sel), 2600, amp=70.0, cell=700.0)
        z[sel] = z[sel] + (ez - z[sel]) * hc
    x0, x1, y0, y1 = CREST_BOX
    sel = (X > x0) & (X < x1) & (Y > y0) & (Y < y1)
    if np.any(sel):
        m = massif(X[sel], Y[sel], _sub(lam, sel))
        fx = smoothstep(x0, x0 + 300.0, X[sel]) * (1.0 - smoothstep(x1 - 300.0, x1, X[sel]))
        fy = smoothstep(y0, y0 + 300.0, Y[sel]) * (1.0 - smoothstep(y1 - 300.0, y1, Y[sel]))
        m = m * fx * fy - 2000.0 * (1.0 - fx * fy)
        z[sel] = smax(z[sel], m, 25.0)
    sel = X < -5500.0
    if np.any(sel):
        z[sel] = np.maximum(z[sel], _far_peaks(X[sel], Y[sel], False, _sub(lam, sel)))
    return z


def river_x(Y):
    Y = np.asarray(Y, dtype=np.float32)
    return 9800.0 + 1400.0 * np.sin(Y / 4300.0 + 0.6) + 520.0 * np.sin(Y / 1700.0 + 2.0) + 160.0 * np.sin(Y / 610.0 + 0.9)


def _valley_wall(X, Y, lam):
    """The far valley wall and the ranges closing the valley up and down (heights above the floor, flat arrays)."""
    wall = smoothstep(16000.0, 22500.0, X)
    end = smoothstep(16000.0, 26000.0, np.abs(Y))
    rough = np.maximum(smoothstep(14500.0, 19000.0, X), end)
    z = wall * (1180.0 + 260.0 * fbm(X, Y, 4200.0, 3, seed=23))
    z = z + end * (950.0 + 300.0 * fbm(X, Y, 5200.0, 3, seed=26))
    sel = rough > 0.0
    if np.any(sel):
        z[sel] += rough[sel] * (260.0 + 240.0 * wall[sel]) * ridged(X[sel], Y[sel], 2600.0, 7, seed=24, min_wavelength=_sub(lam, sel))
    return z + smoothstep(25000.0, 30000.0, X) * 420.0


def valley_floor(X, Y, lam=0.0):
    """West of the drop (flat arrays): the floor and its river, the far wall, the ranges closing the valley (their
    flanks carved by fall-line erosion)."""
    z = FLOOR_Z + 6.0 * fbm(X, Y, 1800.0, 4, seed=21, min_wavelength=lam) + 2.0 * fbm(X, Y, 160.0, 4, seed=22, min_wavelength=lam)
    rd = np.abs(X - river_x(Y))
    z = z - 6.0 * (1.0 - smoothstep(35.0, 75.0, rd)) - 2.5 * (1.0 - smoothstep(75.0, 420.0, rd))
    w = _valley_wall(X, Y, lam)
    sel = (X > 14500.0) | (np.abs(Y) > 16000.0)
    if np.any(sel):
        w[sel] = _erode_far(X[sel], Y[sel], w[sel], _valley_wall, _sub(lam, sel), 2500)
    z = z + w
    sel = (X > 16000.0) | (np.abs(Y) > 16000.0)
    if np.any(sel):
        z[sel] = np.maximum(z[sel], _far_peaks(X[sel], Y[sel], True, _sub(lam, sel)))
    return z


def natural_height(X, Y, lam=0.0):
    """The land before anything was built: the upland east of the lip, the drop and the valley west of it."""
    shape, (X, Y) = _flat(X, Y)
    if not np.isscalar(lam):
        lam = np.ascontiguousarray(np.broadcast_to(np.asarray(lam, dtype=np.float32), shape).ravel())
    xe = edge_x(Y)
    d = X - xe
    out = np.empty(X.shape, dtype=np.float32)
    east = d <= 0.0
    if np.any(east):
        out[east] = upland(X[east], Y[east], _sub(lam, east))
    west = ~east
    if np.any(west):
        lw = _sub(lam, west)
        Xw, Yw, dw = X[west], Y[west], d[west]
        ze = upland(xe[west], Yw, lw)                  # the lip's height
        f = -_DROP(dw) / 900.0                         # 0 at the lip, 1 on the floor
        fl = np.full(Xw.shape, FLOOR_Z, dtype=np.float32)
        far = dw > 1500.0
        if np.any(far):
            fl[far] = valley_floor(Xw[far], Yw[far], _sub(lw, far))
        z = ze + (fl - ze) * f
        # The drop's face: crags and gullies on the steep part, forested spurs and draws below.
        steep = 1.0 - smoothstep(180.0, 420.0, dw)
        near = dw < 2600.0
        if np.any(near):
            ln = _sub(lw, near)
            Xn, Yn, dn, sn = Xw[near], Yw[near], dw[near], steep[near]
            face = (5.0 + 9.0 * sn) * ridged(Xn * 1.3, Yn, 240.0, 8, seed=31, min_wavelength=ln) - 3.5 * sn
            face += sn * 3.2 * (ridged(Xn * 1.6, Yn, 30.0, 5, seed=33, min_wavelength=ln) - 0.4)
            face += 28.0 * (1.0 - smoothstep(0.0, 1000.0, np.abs(dn - 1100.0))) * fbm(Xn, Yn, 900.0, 5, seed=32, min_wavelength=ln)
            z[near] += face * smoothstep(0.0, 22.0, dn) * (1.0 - smoothstep(1900.0, 2600.0, dn))
        out[west] = z
    return out.reshape(shape)


_CREST_ZK = ([-5200.0, -3000.0, -2200.0, -1650.0, -1310.0, -1050.0, -800.0, -620.0, -250.0, 60.0, 330.0, 560.0, 850.0, 1250.0, 2000.0, 3000.0, 4200.0],
                 [430.0, 560.0, 690.0, 860.0, 1035.0, 900.0, 830.0, 905.0, 1350.0, 1000.0, 800.0, 850.0, 940.0, 800.0, 660.0, 570.0, 450.0])


def _CREST_Z(Y):
    """The crest's height along Y: straight runs between the summits and the cols (sharp, not rounded)."""
    return np.interp(Y, _CREST_ZK[0], _CREST_ZK[1]).astype(np.float32)


def _crest_x(Y):
    """The crest line (map X) along Y: through the three summits."""
    return -3500.0 + 0.1887 * (Y + 250.0) + 90.0 * np.sin(Y / 1500.0 + 0.8) * smoothstep(1400.0, 2600.0, np.abs(Y + 250.0))


def _sub(lam, sel):
    """A per-point cut-off array follows a selection; a scalar stays as it is."""
    return lam if np.isscalar(lam) else lam[sel]


def arm_z(X):
    """The drive arms' grade (ecx_plaza's formula): -0.12 at the curve (X 21.25) to -2.40 at the road (X 56)."""
    X = np.asarray(X, dtype=np.float64)
    L, rmp = 34.75, 3.0
    g = 2.28 / (L - rmp)
    s = np.clip(X - 21.25, 0.0, L)
    z = np.where(s < rmp, g * s * s / (2 * rmp), np.where(s <= L - rmp, g * (s - rmp / 2), 2.28 - g * (L - s) ** 2 / (2 * rmp)))
    return (-0.12 - z).astype(np.float32)


def road_profile(Y):
    """The ridge road's surface along Y (ecx_plaza's formula, 10 m vertical curves at the grade breaks); continued
    at the same grades past the ground's ends."""
    Y = np.asarray(Y, dtype=np.float64)
    Lc = 10.0
    d = Y + 42.0
    fs = np.where(d < -Lc / 2, 0.04 * d, np.where(d > Lc / 2, 0.0, 0.04 * (d - (d + Lc / 2) ** 2 / (2 * Lc))))
    d = Y - 60.0
    fn = np.where(d < -Lc / 2, 0.0, np.where(d > Lc / 2, 0.02 * d, 0.02 * (d + Lc / 2) ** 2 / (2 * Lc)))
    return (-2.40 + fs + fn).astype(np.float32)


# The pad: the hardscape outline (spec 2.3) united with the building's footprint (X -60..3), map m, counter-clockwise,
# each edge with what the terrain meets along it.


PAD = [((-60.0, -58.0), 'yard'), ((3.0, -58.0), 'bldg'), ((3.0, -42.0), 'verge'), ((55.0, -42.0), 'road'), ((55.0, -200.0), 'end'),
       ((64.45, -200.0), 'parapet'), ((64.45, 5.55), 'over'), ((70.45, 5.55), 'over'), ((70.45, 20.45), 'over'), ((64.45, 20.45), 'parapet'),
       ((64.45, 120.0), 'end'), ((55.0, 120.0), 'road'), ((55.0, 42.0), 'verge'), ((3.0, 42.0), 'bldg'), ((3.0, 56.0), 'bldg'),
       ((-60.0, 56.0), 'yard')]
# The hole in SM_ECX_SiteTerrain: the outline shrunk 0.5 m (the terrain tucks under the ground's edge) and the front
# 33 m of the building (X -29.5..2.5), where the building's meshes stand. Every coordinate lies on the 0.5 m grid.


HOLE = [(-29.5, -57.5), (2.5, -57.5), (2.5, -41.5), (55.5, -41.5), (55.5, -199.5), (64.0, -199.5), (64.0, 6.0), (70.0, 6.0),
        (70.0, 20.0), (64.0, 20.0), (64.0, 119.5), (55.5, 119.5), (55.5, 41.5), (2.5, 41.5), (2.5, 55.5), (-29.5, 55.5)]


YARD = (-60.0, -29.5, -58.0, 56.0)      # the back of the footprint: terrain, flat at -0.25 under whatever covers it


YARD_Z = -0.25
# The playable bounds (spec 2.8).


BOUNDS = [(0.0, -18.5), (3.0, -18.5), (3.0, -37.0), (52.5, -37.0), (52.5, -48.0), (64.0, -48.0), (64.0, 6.0), (70.0, 6.0),
          (70.0, 20.0), (64.0, 20.0), (64.0, 48.0), (52.5, 48.0), (52.5, 37.0), (3.0, 37.0), (3.0, 18.5), (0.0, 18.5)]
# Per edge kind: (cut slope, fill slope, flat margin m, bank height m).


EDGE_KIND = {'yard': (0.95, 0.6, 2.5, 0.0), 'bldg': (0.9, 0.6, 3.0, 0.0), 'verge': (0.72, 0.55, 0.5, 0.28),
             'road': (0.8, 0.6, 0.9, 0.45), 'end': (0.8, 0.6, 0.0, 0.0), 'parapet': (0.9, 1.25, 0.0, 0.0), 'over': (0.9, 1.25, 0.0, 0.0)}


def pad_grade(kind, X, Y):
    """The ground's height along a pad edge of this kind, at the edge point (X, Y) (the terrain meets it there)."""
    if kind == 'verge':
        return arm_z(np.maximum(X, 21.25)) + 0.20
    if kind in ('road', 'end'):
        return road_profile(Y)
    if kind == 'parapet':
        return road_profile(Y) - 2.0
    if kind == 'over':
        return np.full(np.shape(X), -2.25 - 2.05, dtype=np.float32)
    if kind == 'bldg':
        return np.where(X < -30.0, YARD_Z, 0.0).astype(np.float32)
    return np.full(np.shape(X), YARD_Z, dtype=np.float32)


def seg_dist(PX, PY, ax, ay, bx, by):
    """Distance from points to the segment a-b, and the nearest point on it."""
    ex, ey = bx - ax, by - ay
    L2 = max(ex * ex + ey * ey, 1e-12)
    t = np.clip(((PX - ax) * ex + (PY - ay) * ey) / L2, 0.0, 1.0)
    qx = ax + t * ex
    qy = ay + t * ey
    return np.sqrt((PX - qx) ** 2 + (PY - qy) ** 2), qx, qy


def inside_poly(PX, PY, poly):
    """Even-odd point in polygon (vectorized)."""
    inside = np.zeros(np.shape(PX), dtype=bool)
    n = len(poly)
    for i in range(n):
        x0, y0 = poly[i]
        x1, y1 = poly[(i + 1) % n]
        if y0 == y1:
            continue
        cond = (PY >= min(y0, y1)) & (PY < max(y0, y1))
        xc = x0 + (PY - y0) * (x1 - x0) / (y1 - y0)
        inside ^= cond & (PX < xc)
    return inside


def poly_dist(PX, PY, poly):
    d = np.full(np.shape(PX), 1e9, dtype=np.float32)
    for i in range(len(poly)):
        (ax, ay), (bx, by) = poly[i], poly[(i + 1) % len(poly)]
        d = np.minimum(d, seg_dist(PX, PY, ax, ay, bx, by)[0])
    return d


def pad_fields(PX, PY):
    """Signed distance to the pad's outline (negative inside) and the edge parameters there, blended across nearby
    edges (inverse distance^4) so no crease runs off the pad's corners: (d, z_edge, cut, fill, margin, bank)."""
    pts = [p for p, _ in PAD]
    kinds = [k for _, k in PAD]
    acc = np.zeros((6,) + np.shape(PX), dtype=np.float64)
    wsum = np.zeros(np.shape(PX), dtype=np.float64)
    dmin = np.full(np.shape(PX), 1e9, dtype=np.float64)
    for i in range(len(pts)):
        (ax, ay), (bx, by) = pts[i], pts[(i + 1) % len(pts)]
        d, qx, qy = seg_dist(PX, PY, ax, ay, bx, by)
        dmin = np.minimum(dmin, d)
        w = 1.0 / (d ** 4 + 0.05)
        z = pad_grade(kinds[i], qx, qy)
        c, f, m, b = EDGE_KIND[kinds[i]]
        acc[0] += w * z
        acc[1] += w * c
        acc[2] += w * f
        acc[3] += w * m
        acc[4] += w * b
        wsum += w
    inside = inside_poly(PX, PY, pts)
    sd = np.where(inside, -dmin, dmin)
    v = acc[:5] / wsum
    return sd.astype(np.float32), v[0].astype(np.float32), v[1].astype(np.float32), v[2].astype(np.float32), v[3].astype(np.float32), v[4].astype(np.float32)


SITE_REACH = 650.0     # the site's shaping is computed within this square (it has faded out long before)


def site_shape(X, Y, z):
    """The resort's shelf on the natural ground (flat arrays near the site): cut and fill slopes off the pad, a plowed
    bank along the verges and shoulders, the yard behind the building, the 0.5 m overlap band dipping under the
    ground's edge."""
    sd, ze, kc, kf, mg, bank = pad_fields(X, Y)
    out = z.copy()
    o = sd > 0.0
    # The ground's own relief round the shelf: swells, hollows and wind drifts (elongated along the west wind).
    away = smoothstep(2.0, 9.0, sd) * (1.0 - smoothstep(260.0, 420.0, sd))
    z = z + away * (1.5 * fbm(X, Y, 55.0, 3, seed=311) + 0.45 * fbm(X, Y, 13.0, 3, seed=312) + 0.35 * fbm(X * 0.33, Y, 9.0, 2, seed=313))
    # Cut faces vary in pitch and carry rock outcrops.
    kc = kc * (1.0 + 0.3 * fbm(X, Y, 40.0, 2, seed=314))
    d = np.maximum(sd - mg * (1.0 + 0.5 * fbm(X, Y, 25.0, 2, seed=315)), 0.0)
    lo = ze - kf * d
    hi = ze + kc * d
    shaped = np.clip(z, lo, hi)
    cut = np.clip((z - hi) / 2.0, 0.0, 1.0)
    shaped = shaped + cut * (1.0 + 1.8 * smoothstep(0.6, 1.2, kc)) * (ridged(X, Y, 11.0, 4, seed=316) - 0.45)
    # The plowed bank: a soft ridge of snow just off the verge, before the slope takes over.
    b = bank * smoothstep(0.0, 1.4, sd) * (1.0 - smoothstep(2.2, 5.0, sd)) * (0.75 + 0.5 * fbm(X, Y, 7.0, 3, seed=301))
    out[o] = (shaped + b)[o]
    # Inside the pad's outline: under the ground (the 0.5 m band) and the yard.
    i = ~o
    out[i] = (ze + 0.12 * sd)[i]
    yx0, yx1, yy0, yy1 = YARD
    yard = (X > yx0 - 1e-3) & (X < yx1 + 1e-3) & (Y > yy0 - 1e-3) & (Y < yy1 + 1e-3)
    out[yard] = YARD_Z
    return out


# ---------------------------------------------------------------------- roads (terrain only, beyond the asphalt)


ROAD_HW = 3.75          # half the carriageway


ROAD_SH = 1.0           # the plowed shoulder each side


SW_R = 16.0             # the hairpins' radius


SW_PLAN = [(-655.0, -55.0), (575.0, -172.0), (-575.0, -286.0), (630.0, -402.0), (-500.0, -514.0), (650.0, -626.0), (-420.0, -734.0)]


SW_END = (1900.0, 600.0, -860.0)


def _arc_pts(cx, cy, r, a0, a1, step=2.0):
    n = max(2, int(abs(a1 - a0) * r / step) + 1)
    return [(cx + r * math.cos(a0 + (a1 - a0) * k / (n - 1)), cy + r * math.sin(a0 + (a1 - a0) * k / (n - 1))) for k in range(n)]


def _line_pts(a, b, step=2.0):
    L = math.hypot(b[0] - a[0], b[1] - a[1])
    n = max(2, int(L / step) + 1)
    return [(a[0] + (b[0] - a[0]) * k / (n - 1), a[1] + (b[1] - a[1]) * k / (n - 1)) for k in range(n)]


def _cat(*runs):
    out = []
    for r in runs:
        out += r[1:] if out and math.hypot(r[0][0] - out[-1][0], r[0][1] - out[-1][1]) < 1e-6 else r
    return out


def _arclen(pts):
    p = np.asarray(pts, dtype=np.float64)
    return np.concatenate([[0.0], np.cumsum(np.hypot(np.diff(p[:, 0]), np.diff(p[:, 1])))])


def _contour_x(Y, Z):
    """Where the drop's face stands at height Z along Y (map X): the switchbacks follow the slope."""
    Y = np.float32(Y)
    xe = float(edge_x(np.array([Y]))[0])
    zl = float(upland(np.array([xe], dtype=np.float32), np.array([Y], dtype=np.float32))[0])
    dd = np.linspace(0.0, 3200.0, 6401)
    zz = zl + (FLOOR_Z - zl) * (-_DROP(dd) / 900.0)
    return xe + float(np.interp(-Z, -zz, dd))


_ROADS = None


def roads():
    """The terrain's roads (map m): name -> (N, 3) centerline points (X, Y, Z), 2 m apart."""
    global _ROADS
    if _ROADS is not None:
        return _ROADS
    R = {}
    # North: on past the asphalt's end (Y 120), easing east into the north bench, climbing with the land.
    a = (59.5, 116.0)
    p = _cat(_line_pts(a, (59.5, 160.0)), _arc_pts(59.5 - 600.0, 160.0, 600.0, 0.0, math.radians(22.0)))
    e = p[-1]
    p = _cat(p, _line_pts(e, (e[0] - 300.0 * math.sin(math.radians(22.0)), e[1] + 300.0 * math.cos(math.radians(22.0)))))
    s = _arclen(p)
    z0 = float(road_profile(np.array([a[1]]))[0])
    z = z0 + 0.02 * np.minimum(s, 44.0) + 0.09 * np.maximum(s - 44.0, 0.0)
    R['north'] = np.column_stack([np.asarray(p), z])
    # South: past the asphalt's end (Y -200) into a cut behind the spur, curving east, on down at 4 %.
    a = (59.5, -196.0)
    p = _cat(_line_pts(a, (59.5, -215.0)), _arc_pts(59.5 - 260.0, -215.0, 260.0, 0.0, -math.radians(50.0)))
    e = p[-1]
    p = _cat(p, _line_pts(e, (e[0] - 250.0 * math.sin(math.radians(50.0)), e[1] - 250.0 * math.cos(math.radians(50.0)))))
    s = _arclen(p)
    z0 = float(road_profile(np.array([a[1]]))[0])
    R['south'] = np.column_stack([np.asarray(p), z0 - 0.04 * s])
    # The switchbacks: from the south road (60.5, -208) down the drop in 8 legs and 7 hairpins to the valley road.
    ys = float(road_profile(np.array([-208.0]))[0])
    start = [(60.5, -208.0), (66.0, -214.0), (72.0, -224.0), (76.0, -240.0)]
    pts = _cat(*[_line_pts(start[i], start[i + 1]) for i in range(3)])
    anchors = [(len(pts) - 1, ys - 1.4)]
    cur = start[-1]
    for k, (yh, zh) in enumerate(SW_PLAN):
        xh = _contour_x(yh, zh)
        south = yh < cur[1]
        # Run to the hairpin's start (swinging gently with the slope's folds), then round it (the next leg below, 2R
        # further out).
        run = _line_pts(cur, (xh, yh))
        n = len(run)
        amp = 8.0 + 5.0 * (k % 3)
        run = [(x + amp * math.sin(math.pi * i / (n - 1)) * math.sin(2.0 * math.pi * i / (n - 1) * (1.0 + 0.5 * (k % 2)) + k), y) for i, (x, y) in enumerate(run)]
        pts = _cat(pts, run)
        anchors.append((len(pts) - 1, zh))
        sg = -1.0 if south else 1.0
        arc = [(xh + SW_R + SW_R * math.cos(math.pi - t), yh + sg * SW_R * math.sin(t)) for t in np.linspace(0.0, math.pi, 26)]
        pts = _cat(pts, arc)
        cur = arc[-1]
    pts = _cat(pts, _line_pts(cur, SW_END[:2]))
    anchors.append((len(pts) - 1, SW_END[2]))
    s = _arclen(pts)
    pa = np.asarray(pts)
    zl = natural_height(pa[:, 0].astype(np.float32), pa[:, 1].astype(np.float32), 40.0).astype(np.float64)
    # The road follows the land, smoothed over 160 m, kept between the hairpins' plan and capped at a 12 % grade
    # (always falling toward the valley).
    zk = np.interp(s, [s[i] for i, _ in anchors], [z for _, z in anchors])
    zr = 0.6 * zl + 0.4 * zk
    k = 80
    pad = np.concatenate([np.full(k, zr[0]), zr, np.full(k, zr[-1])])
    zr = np.convolve(pad, np.ones(2 * k + 1) / (2 * k + 1), mode='same')[k:-k]
    zr[0] = ys - 1.4
    ds = np.diff(s)
    for i in range(1, len(zr)):
        zr[i] = min(zr[i], zr[i - 1] - 0.005 * ds[i - 1])
        zr[i] = max(zr[i], zr[i - 1] - 0.12 * ds[i - 1])
    # Land on the valley road's level at the foot.
    zr = zr + (SW_END[2] - zr[-1]) * smoothstep(s[-1] - 1500.0, s[-1], s)
    R['switchback'] = np.column_stack([np.asarray(pts), zr])
    # The valley road: from the switchbacks' foot west across the floor to the highway.
    p = _cat(_line_pts(SW_END[:2], (2600.0, 760.0), 20.0), _line_pts((2600.0, 760.0), (4300.0, 1150.0), 20.0))
    R['valley'] = np.column_stack([np.asarray(p), np.full(len(p), -862.0)])
    _ROADS = R
    return R


def lamp_points():
    """The switchbacks' lamps: every 60 m on the downhill side, 7.5 m over the road: (N, 3) map m, and each one's
    heading (the road's direction, radians)."""
    p = roads()['switchback']
    s = _arclen(p[:, :2])
    out, heads = [], []
    for t in np.arange(30.0, s[-1], 60.0):
        i = int(np.searchsorted(s, t))
        i = min(max(i, 1), len(p) - 1)
        f = (t - s[i - 1]) / max(s[i] - s[i - 1], 1e-6)
        q = p[i - 1] + (p[i] - p[i - 1]) * f
        dx, dy = p[i, 0] - p[i - 1, 0], p[i, 1] - p[i - 1, 1]
        L = math.hypot(dx, dy) or 1.0
        nx, ny = dy / L, -dx / L          # one side of the road
        if nx < 0:                        # the downhill side is to the west (+X)
            nx, ny = -nx, -ny
        out.append((q[0] + nx * (ROAD_HW + 1.3), q[1] + ny * (ROAD_HW + 1.3), q[2] + 7.5))
        heads.append(math.atan2(dy, dx))
    return np.asarray(out), np.asarray(heads)


class RoadRaster:
    """Distance to the nearest road centerline and that road's height, on a 1 m grid (X -700..2300, Y -1100..1100)."""
    X0, Y0, NX, NY, CELL = -700.0, -1100.0, 3000, 2200, 1.0
    REACH = 45.0

    def __init__(self):
        t0 = time.time()
        self.dist = np.full((self.NY, self.NX), 1e4, dtype=np.float32)
        self.zr = np.zeros((self.NY, self.NX), dtype=np.float32)
        self.kind = np.zeros((self.NY, self.NX), dtype=np.int8)
        for ki, name in enumerate(('north', 'south', 'switchback', 'valley')):
            p = roads()[name]
            for i in range(len(p) - 1):
                ax, ay, az = p[i]
                bx, by, bz = p[i + 1]
                x0 = int(max(0, math.floor((min(ax, bx) - self.REACH - self.X0) / self.CELL)))
                x1 = int(min(self.NX, math.ceil((max(ax, bx) + self.REACH - self.X0) / self.CELL) + 1))
                y0 = int(max(0, math.floor((min(ay, by) - self.REACH - self.Y0) / self.CELL)))
                y1 = int(min(self.NY, math.ceil((max(ay, by) + self.REACH - self.Y0) / self.CELL) + 1))
                if x0 >= x1 or y0 >= y1:
                    continue
                gx = self.X0 + (np.arange(x0, x1) + 0.0) * self.CELL
                gy = self.Y0 + (np.arange(y0, y1) + 0.0) * self.CELL
                PX, PY = np.meshgrid(gx, gy)
                ex, ey = bx - ax, by - ay
                L2 = max(ex * ex + ey * ey, 1e-9)
                t = np.clip(((PX - ax) * ex + (PY - ay) * ey) / L2, 0.0, 1.0)
                d = np.hypot(PX - (ax + t * ex), PY - (ay + t * ey)).astype(np.float32)
                blk = self.dist[y0:y1, x0:x1]
                better = d < blk
                blk[better] = d[better]
                self.zr[y0:y1, x0:x1][better] = (az + t * (bz - az))[better]
                self.kind[y0:y1, x0:x1][better] = ki + 1
        log(f'road raster in {time.time() - t0:.1f}s')

    def sample(self, X, Y):
        """(distance, road height, kind 1..4 or 0) at map points (bilinear, nearest kind)."""
        fx = (np.asarray(X, dtype=np.float64) - self.X0) / self.CELL
        fy = (np.asarray(Y, dtype=np.float64) - self.Y0) / self.CELL
        ok = (fx >= 0) & (fx < self.NX - 1) & (fy >= 0) & (fy < self.NY - 1)
        dist = np.full(np.shape(X), 1e4, dtype=np.float32)
        zr = np.zeros(np.shape(X), dtype=np.float32)
        kind = np.zeros(np.shape(X), dtype=np.int8)
        if np.any(ok):
            x, y = fx[ok], fy[ok]
            ix = np.floor(x).astype(np.int64)
            iy = np.floor(y).astype(np.int64)
            tx = (x - ix).astype(np.float32)
            ty = (y - iy).astype(np.float32)

            def bil(a):
                return (a[iy, ix] * (1 - tx) * (1 - ty) + a[iy, ix + 1] * tx * (1 - ty) + a[iy + 1, ix] * (1 - tx) * ty + a[iy + 1, ix + 1] * tx * ty)
            dist[ok] = bil(self.dist)
            zr[ok] = bil(self.zr)
            kind[ok] = self.kind[np.rint(y).astype(np.int64), np.rint(x).astype(np.int64)]
        return dist, zr, kind


_RASTER = None


def raster():
    global _RASTER
    if _RASTER is None:
        _RASTER = RoadRaster()
    return _RASTER


def road_blend(X, Y, z, snow_banks=True, soft=0.0):
    """Benches the roads into the ground: the carriageway and shoulders flat at the road's height, cut and fill slopes
    (about 40 deg) off them, plowed snow berms along the edges where there is snow. soft (m, scalar or per point):
    how much a coarse sampling (a far mesh's vertices) widens the bench's edges, so it can't alias into a saw-tooth;
    the texture keeps the crisp bench in its normals. Returns (z, distance, kind)."""
    dist, zr, kind = raster().sample(X, Y)
    on = dist < RoadRaster.REACH
    if not np.any(on):
        return z, dist, kind
    out = z.copy()
    d, r, zz = dist[on], zr[on], z[on]
    flat = ROAD_HW + ROAD_SH
    reach = flat + 1.0 + 1.2 * np.abs(r - zz)
    sf = soft if np.isscalar(soft) else soft[on]
    w = np.where(d <= flat - 0.5 * sf, 1.0, 1.0 - smoothstep(flat - 0.5 * sf, reach + sf, d))
    nz = zz + (r - zz) * w
    if snow_banks:
        snowy = smoothstep(-560.0, -380.0, r)
        nz = nz + 0.6 * snowy * np.exp(-((d - (flat + 1.4)) / 1.5) ** 2)
    out[on] = nz
    return out, dist, kind


def world_height(X, Y, lam=0.0):
    """The ground as it is (flat or shaped arrays): the natural land, the resort's shelf, the roads."""
    shape, (X, Y) = _flat(X, Y)
    if not np.isscalar(lam):
        lam = np.ascontiguousarray(np.broadcast_to(np.asarray(lam, dtype=np.float32), shape).ravel())
    # Where SiteTerrain meets Ridge_Near (the 448 m square) both sample a surface smooth at 32 m, so their different
    # tessellations meet without gaps.
    edge = np.abs(np.maximum(np.abs(X), np.abs(Y)) - SITE_HALF)
    seam = 1.0 - smoothstep(12.0, 40.0, edge)
    # A coarse sampling (beyond the Site's square) softens the roads' benches; never in the Site, nor at that seam,
    # where both sides must agree exactly.
    outside = np.maximum(np.abs(X), np.abs(Y)) > SITE_HALF
    soft = np.clip(np.asarray(lam, dtype=np.float32) - 4.0, 0.0, 24.0) * (1.0 - seam) * outside
    if np.any(seam > 0):
        lam = np.maximum(lam, 64.0 * seam) if not np.isscalar(lam) else np.maximum(lam, 64.0 * seam).astype(np.float32)
    z = natural_height(X, Y, lam)
    near = (np.abs(X - 30.0) < SITE_REACH) & (np.abs(Y) < SITE_REACH)
    if np.any(near):
        z[near] = site_shape(X[near], Y[near], z[near])
    z, _, _ = road_blend(X, Y, z, soft=soft)
    # Under the ground's edge the terrain stays below it, roads or not.
    if np.any(near):
        sel = np.nonzero(near)[0]
        sd, ze = pad_fields(X[sel], Y[sel])[:2]
        band = sd < 0.0
        z[sel[band]] = np.minimum(z[sel[band]], (ze + 0.12 * sd)[band])
        yx0, yx1, yy0, yy1 = YARD
        Xs, Ys = X[sel], Y[sel]
        yard = (Xs > yx0 - 1e-3) & (Xs < yx1 + 1e-3) & (Ys > yy0 - 1e-3) & (Ys < yy1 + 1e-3)
        z[sel[yard]] = YARD_Z
    return z.reshape(shape)


def quadtree(x0, y0, unit, root_units, nx, ny, levels, target_fn, hole=None):
    """A balanced (2:1) quadtree over a grid of root cells, refined until every leaf is no larger than target_fn(X, Y)
    (meters) and no leaf straddles the hole's outline (its vertices must lie on the finest grid); leaves inside the
    hole are dropped. Returns (vertices (N, 2) map m, triangles (M, 3), vertex cell size (N,) m): conforming
    triangles, two per plain leaf, a fan round the middle of a leaf with a smaller neighbor."""
    t0 = time.time()
    leaves = {}
    internal = set()
    ix, iy = np.meshgrid(np.arange(nx), np.arange(ny))
    cand = np.column_stack([ix.ravel(), iy.ravel()]).astype(np.int64)
    hole_v = np.asarray(hole, dtype=np.float64) if hole else None
    for lvl in range(levels + 1):
        if len(cand) == 0:
            break
        size_u = root_units >> lvl
        size_m = size_u * unit
        cx0 = x0 + cand[:, 0] * size_m
        cy0 = y0 + cand[:, 1] * size_m
        fr = np.array([0.0, 0.5, 1.0])
        sx = (cx0[:, None, None] + fr[None, :, None] * size_m).repeat(3, 2).reshape(len(cand), 9)
        sy = (cy0[:, None, None] + fr[None, None, :] * size_m).repeat(3, 1).reshape(len(cand), 9)
        tgt = target_fn(sx.ravel(), sy.ravel()).reshape(len(cand), 9).min(axis=1)
        split = size_m > tgt + 1e-6
        drop = np.zeros(len(cand), dtype=bool)
        if hole is not None:
            ins = inside_poly(sx.ravel() + 1e-4 * (sx.ravel() - (cx0[:, None] + 0.5 * size_m).repeat(9, 1).ravel()),
                              sy.ravel() + 1e-4 * (sy.ravel() - (cy0[:, None] + 0.5 * size_m).repeat(9, 1).ravel()), hole).reshape(len(cand), 9)
            centre_in = ins[:, 4]
            allin = ins.all(axis=1)
            anyin = ins.any(axis=1)
            # A hole vertex inside the cell (its corner can hide between the samples).
            vin = np.zeros(len(cand), dtype=bool)
            for hx, hy in hole_v:
                vin |= (hx > cx0 + 1e-6) & (hx < cx0 + size_m - 1e-6) & (hy > cy0 + 1e-6) & (hy < cy0 + size_m - 1e-6)
            straddle = (anyin & ~allin) | vin
            drop = allin & ~vin
            split = (split | straddle) & ~drop
            if lvl == levels:
                drop = drop | (straddle & centre_in)
        if lvl == levels:
            split[:] = False
        for k in np.nonzero(~split & ~drop)[0]:
            leaves[(lvl, int(cand[k, 0]), int(cand[k, 1]))] = True
        sp = cand[split]
        for k in range(len(sp)):
            internal.add((lvl, int(sp[k, 0]), int(sp[k, 1])))
        cand = np.concatenate([np.column_stack([sp[:, 0] * 2 + a, sp[:, 1] * 2 + b]) for a in (0, 1) for b in (0, 1)]) if len(sp) else np.zeros((0, 2), np.int64)
    log(f'quadtree: {len(leaves)} leaves before balancing ({time.time() - t0:.1f}s)')

    # 2:1 balance: a leaf whose neighbor's near half is split further than one level splits too.
    def node_split_deep(l, i, j, side):
        """Is node (l, i, j) internal with a child on `side` (the side facing the asking leaf) internal too?"""
        if (l, i, j) not in internal:
            return False
        kids = {'W': [(0, 0), (0, 1)], 'E': [(1, 0), (1, 1)], 'S': [(0, 0), (1, 0)], 'N': [(0, 1), (1, 1)]}[side]
        return any((l + 1, 2 * i + a, 2 * j + b) in internal for a, b in kids)

    changed = True
    while changed:
        changed = False
        for (l, i, j) in list(leaves.keys()):
            if l >= levels:
                continue
            for di, dj, face in ((-1, 0, 'E'), (1, 0, 'W'), (0, -1, 'N'), (0, 1, 'S')):
                if node_split_deep(l, i + di, j + dj, face):
                    del leaves[(l, i, j)]
                    internal.add((l, i, j))
                    for a in (0, 1):
                        for b in (0, 1):
                            leaves[(l + 1, 2 * i + a, 2 * j + b)] = True
                    changed = True
                    break
    # Children created by balancing may lie inside the hole: drop them (their parents straddled it).
    if hole is not None:
        keys = list(leaves.keys())
        L = np.array([k[0] for k in keys])
        I = np.array([k[1] for k in keys])
        J = np.array([k[2] for k in keys])
        sz = (root_units >> L) * unit
        cxm = x0 + (I + 0.5) * sz
        cym = y0 + (J + 0.5) * sz
        gone = inside_poly(cxm, cym, hole)
        for k in np.nonzero(gone)[0]:
            del leaves[keys[k]]
    log(f'quadtree: {len(leaves)} leaves balanced ({time.time() - t0:.1f}s)')

    # Vertices on the finest grid (integer keys), triangles.
    vid = {}
    verts = []
    vsize = []

    def vert(u, v, s):
        key = (u, v)
        k = vid.get(key)
        if k is None:
            k = len(verts)
            vid[key] = k
            verts.append(key)
            vsize.append(s)
        elif s < vsize[k]:
            vsize[k] = s
        return k

    fine = 1 << levels          # finest units per root unit... keys in finest-grid units
    for (l, i, j) in leaves:
        su = fine >> l
        u0, v0 = i * su, j * su
        s = (root_units >> l) * unit
        for du, dv in ((0, 0), (su, 0), (su, su), (0, su)):
            vert(u0 + du, v0 + dv, s)
    tris = []
    for (l, i, j) in leaves:
        su = fine >> l
        u0, v0 = i * su, j * su
        s = (root_units >> l) * unit
        c = [vid[(u0, v0)], vid[(u0 + su, v0)], vid[(u0 + su, v0 + su)], vid[(u0, v0 + su)]]
        h = su // 2
        mids = [vid.get((u0 + h, v0)), vid.get((u0 + su, v0 + h)), vid.get((u0 + h, v0 + su)), vid.get((u0, v0 + h))] if su > 1 else [None] * 4
        if all(m is None for m in mids):
            if (i + j) % 2:
                tris += [(c[0], c[1], c[2]), (c[0], c[2], c[3])]
            else:
                tris += [(c[0], c[1], c[3]), (c[1], c[2], c[3])]
            continue
        ring = []
        for k in range(4):
            ring.append(c[k])
            if mids[k] is not None:
                ring.append(mids[k])
        m = vert(u0 + h, v0 + h, s)
        for k in range(len(ring)):
            tris.append((m, ring[k], ring[(k + 1) % len(ring)]))
    fu = unit * root_units / fine
    V = np.array(verts, dtype=np.float64) * fu + np.array([x0, y0])
    log(f'quadtree: {len(V)} vertices, {len(tris)} triangles ({time.time() - t0:.1f}s)')
    return V, np.array(tris, dtype=np.int64), np.array(vsize, dtype=np.float32)


def boundary_edges(tris):
    """Edges used by one triangle only, in their triangle's winding order."""
    e = np.concatenate([tris[:, [0, 1]], tris[:, [1, 2]], tris[:, [2, 0]]])
    key = np.sort(e, axis=1)
    _, inv, cnt = np.unique(key, axis=0, return_inverse=True, return_counts=True)
    return e[cnt[inv.ravel()] == 1]


def vertex_normals(P, tris):
    """Area-weighted vertex normals of a triangle mesh (map frame)."""
    a, b, c = P[tris[:, 0]], P[tris[:, 1]], P[tris[:, 2]]
    fn = np.cross(b - a, c - a)
    vn = np.zeros_like(P)
    for k in range(3):
        np.add.at(vn, tris[:, k], fn)
    ln = np.linalg.norm(vn, axis=1, keepdims=True)
    return vn / np.maximum(ln, 1e-12)


def make_mesh(name, P, tris, uv=None, colors=None, skirts=None, normals_up=True):
    """A Blender mesh from map-frame points (N, 3) and triangles (counter-clockwise seen from above in map X, Y).
    x = -X turns the winding, so faces are reversed for Blender. uv: per vertex (N, 2) or per corner (M, 3, 2);
    colors: (N, 4) linear floats -> the 'Color' attribute (COLOR_0); skirts: (K, 4) quads of vertex indices into an
    appended point list (P must already hold them)."""
    me = bpy.data.meshes.new(name)
    Pb = np.column_stack([-P[:, 0], P[:, 1], P[:, 2]]).astype(np.float32)
    faces = tris[:, ::-1]
    nv, nf = len(Pb), len(faces)
    me.vertices.add(nv)
    me.vertices.foreach_set('co', Pb.ravel())
    me.loops.add(nf * 3)
    me.loops.foreach_set('vertex_index', faces.ravel().astype(np.int32))
    me.polygons.add(nf)
    me.polygons.foreach_set('loop_start', (np.arange(nf) * 3).astype(np.int32))
    me.update(calc_edges=True)
    if uv is not None:
        lay = me.uv_layers.new(name='UVMap')
        uv = np.asarray(uv, dtype=np.float32)
        if uv.ndim == 2:
            corner = uv[faces.ravel()]
        else:
            corner = uv[:, ::-1, :].reshape(-1, 2)
        lay.data.foreach_set('uv', corner.ravel())
    if colors is not None:
        attr = me.color_attributes.new('Color', 'FLOAT_COLOR', 'POINT')
        attr.data.foreach_set('color', np.asarray(colors, dtype=np.float32).ravel())
        me.color_attributes.active_color = attr
        me.color_attributes.render_color_index = me.color_attributes.active_color_index
    me.validate(clean_customdata=False)
    obj = bpy.data.objects.new(name, me)
    core.link(obj)
    me.shade_smooth()
    return obj


STANDS = [(5.0, 52.0, 39.0, 90.0), (-30.0, 5.0, 60.0, 90.0)]      # the spruce stands near the plaza (X0, X1, |Y|0, |Y|1)


def forest_density(X, Y, Z, slope_deg, road_dist=None):
    """Where conifers grow, 0..1 (flat arrays): below the forest line (+600 m, ragged), off cliffs (> 40 deg) and the
    roads, out of the clearings (the hamlet's, the lit run, avalanche paths, the valley's fields), off the pad; dense
    in the stands by the plaza."""
    patch = fbm(X, Y, 520.0, 4, seed=401)
    fine = fbm(X, Y, 60.0, 3, seed=402)
    d = smoothstep(-0.55, 0.0, patch + 0.25 * fine)
    line = 600.0 + 90.0 * fbm(X, Y, 900.0, 3, seed=403)
    d = d * (1.0 - smoothstep(line - 120.0, line + 60.0, Z))
    d = d * (1.0 - smoothstep(34.0, 44.0, slope_deg + 6.0 * fine))
    # Clearings: the hamlet on the north bench, the lit run, the valley floor's farmland (patchy woods remain).
    d = d * (1.0 - np.exp(-((X - 0.0) / 210.0) ** 4 - ((Y - 600.0) / 320.0) ** 4))
    run = np.exp(-((Y - (900.0 + (X + 400.0) * -0.62)) / 32.0) ** 2) * smoothstep(-380.0, -460.0, X) * smoothstep(-1260.0, -1180.0, X)
    d = d * (1.0 - run)
    floor = smoothstep(-760.0, -840.0, Z)
    woods = smoothstep(0.05, 0.3, fbm(X, Y, 1400.0, 3, seed=404) + 0.2 * fine)
    d = d * (1.0 - floor * (1.0 - woods))
    # Off the pad, dense in the stands.
    near = (np.abs(X - 30.0) < SITE_REACH) & (np.abs(Y) < SITE_REACH)
    if np.any(near):
        sd = np.full(X.shape, 1e9, dtype=np.float32)
        sd[near] = pad_fields(X[near], Y[near])[0]
        d = d * smoothstep(4.0, 22.0, sd)
        for (x0, x1, y0, y1) in STANDS:
            inside = (X > x0) & (X < x1) & (np.abs(Y) > y0) & (np.abs(Y) < y1)
            d = np.where(inside, np.maximum(d, 0.9 + 0.1 * fine), d)
    if road_dist is not None:
        d = d * smoothstep(ROAD_HW + 4.0, ROAD_HW + 12.0, road_dist)
    return np.clip(d, 0.0, 1.0)


def site_target(X, Y):
    """SiteTerrain's cell size (m): fine by the playable bounds, coarser away; crisp road benches; the crags under the
    parapet at 0.5 m."""
    X = np.asarray(X, dtype=np.float64)
    Y = np.asarray(Y, dtype=np.float64)
    inb = inside_poly(X, Y, BOUNDS)
    d = np.where(inb, 0.0, poly_dist(X, Y, BOUNDS))
    t = np.select([d < 25.0, d < 60.0, d < 120.0, d < 200.0, d < 300.0], [0.5, 1.0, 2.0, 4.0, 8.0], 16.0)
    crag = (X > 63.0) & (X < 135.0) & (Y > -215.0) & (Y < 135.0)
    t = np.where(crag, 0.5, t)
    cut = (np.abs(Y) > 50.0) & (np.abs(Y) < 95.0) & (X > -75.0) & (X < 10.0)
    t = np.where(cut, np.minimum(t, 0.5), t)
    dist, _, _ = raster().sample(X, Y)
    t = np.where((dist < 16.0) & (d < 450.0), np.minimum(t, 1.0), t)
    rim = SITE_HALF - np.maximum(np.abs(X), np.abs(Y))
    t = np.where(rim < 24.0, np.minimum(t, 4.0), t)
    return t


def site_colors(X, Y, Z, N, road_dist, road_kind):
    """COLOR_0 for SiteTerrain (linear): R rock exposure, G forest litter and shade, B wind crust."""
    slope = np.degrees(np.arccos(np.clip(N[:, 2], -1.0, 1.0))).astype(np.float32)
    n1 = fbm(X, Y, 9.0, 4, seed=411)
    n2 = fbm(X, Y, 2.2, 3, seed=412)
    rock = smoothstep(31.0, 47.0, slope + 7.0 * n1 + 3.0 * n2)
    # Bare rock where the cliff runs below the parapet and on the cuts' faces, even under a little snow.
    rock = np.maximum(rock, 0.6 * smoothstep(0.35, 0.7, ridged(X, Y, 14.0, 3, seed=413)) * smoothstep(28.0, 40.0, slope))
    # The roads beyond the asphalt: plowed, gritty, dark (read as rock and gravel).
    on_road = (road_kind >= 1) & (road_kind <= 3)
    bed = (1.0 - smoothstep(ROAD_HW + 0.2, ROAD_HW + 1.2, road_dist)) * on_road
    rock = np.maximum(rock, 0.85 * bed)
    forest = forest_density(X, Y, Z, slope, road_dist)
    litter = np.clip(forest * (0.75 + 0.35 * n1), 0.0, 1.0)
    # Wind crust: exposed, convex, treeless ground; the lip; scoured the more where the slope faces west (the wind).
    crust = (1.0 - forest) * smoothstep(-0.1, 0.5, n1 * 0.7 + 0.5 * np.clip(-N[:, 0], 0.0, 1.0) + 0.25)
    crust = crust * (1.0 - rock) * (1.0 - bed)
    lip = np.exp(-((X - edge_x(Y)) / 10.0) ** 2)
    crust = np.clip(np.maximum(crust, 0.8 * lip * (1.0 - rock)), 0.0, 1.0)
    return np.column_stack([rock, litter, crust, np.ones_like(rock)]).astype(np.float32)


def terrain_runtime_material():
    """ecx_terrain: the runtime material's stand-in (M_ECX_Terrain replaces it by name). The color attribute feeds Base
    Color directly: that is what makes the glTF exporter write it as COLOR_0 (used any other way, it writes a white
    COLOR_0 and the masks as COLOR_1)."""
    m = core.Mat('ecx_terrain')
    ca = m.node('ShaderNodeVertexColor')
    ca.layer_name = 'Color'
    m.link(ca.outputs['Color'], m.bsdf.inputs['Base Color'])
    m.set('Roughness', 0.62)
    m.m.use_backface_culling = True
    return m


def build_site_terrain():
    t0 = time.time()
    V2, T, vs = quadtree(-SITE_HALF, -SITE_HALF, 0.5, 32, 56, 56, 5, site_target, HOLE)
    X = V2[:, 0].astype(np.float32)
    Y = V2[:, 1].astype(np.float32)
    Z = world_height(X, Y, lam=np.maximum(2.0 * vs, 1.0))
    # Snow's own small relief near the plaza: drifts and wind ripples a few centimeters to a decimeter.
    d = np.hypot(np.maximum(np.abs(X - 30.0) - 40.0, 0.0), np.maximum(np.abs(Y) - 60.0, 0.0))
    sd = pad_fields(X, Y)[0]
    ripple = (0.09 * fbm(X, Y, 5.0, 3, seed=421) + 0.05 * fbm(X, Y, 1.6, 2, seed=422)) * (1.0 - smoothstep(60.0, 160.0, d))
    Z = Z + ripple * smoothstep(0.4, 1.5, sd)        # none at the ground's edge
    P = np.column_stack([X, Y, Z]).astype(np.float64)
    N = vertex_normals(P, T)
    rd, _, rk = raster().sample(X, Y)
    colors = site_colors(X, Y, Z, N, rd, rk)
    # Skirts down from the outer square's edge (2.5 m, both faces), where Ridge_Near takes over.
    be = boundary_edges(T)
    on_sq = lambda i: (np.abs(np.abs(X[i]) - SITE_HALF) < 1e-3) | (np.abs(np.abs(Y[i]) - SITE_HALF) < 1e-3)  # noqa: E731
    outer = be[on_sq(be[:, 0]) & on_sq(be[:, 1])]
    lo = {}
    extra = []
    for v in np.unique(outer):
        lo[int(v)] = len(P) + len(extra)
        extra.append((X[v], Y[v], Z[v] - 2.5))
    P2 = np.vstack([P, np.asarray(extra)])
    sk = []
    for a, b in outer:
        a2, b2 = lo[int(a)], lo[int(b)]
        sk += [(b, a, a2), (b, a2, b2), (a, b, b2), (a, b2, a2)]
    T2 = np.vstack([T, np.asarray(sk, dtype=np.int64)])
    col2 = np.vstack([colors, colors[[int(v) for v in np.unique(outer)]]])
    uv = np.column_stack([P2[:, 0] / 10.0, P2[:, 1] / 10.0])
    obj = make_mesh('SM_ECX_SiteTerrain', P2, T2, uv=uv, colors=col2)
    core.assign(obj, terrain_runtime_material())
    log(f'SM_ECX_SiteTerrain: {len(T2)} triangles, Z {Z.min():.1f}..{Z.max():.1f} in {time.time() - t0:.0f}s')
    return obj


ATLAS = 4096


CELL = 1024


CELL_M = 0.52            # each atlas cell covers 0.52 x 0.52 m of spray
# Cell index -> what it holds (row-major from the atlas's top-left; cell (c, r) spans u c/4..(c+1)/4, v 1-(r+1)/4..1-r/4).


CELLS = ['spruce_frost_a', 'spruce_frost_b', 'spruce_frost_c', 'spruce_snow_a', 'spruce_snow_b', 'spruce_snow_c',
         'pine_tuft_a', 'pine_tuft_b', 'pine_tuft_snow', 'snow_clump_a', 'snow_clump_b', 'spruce_young',
         'dead_twigs', 'spruce_leader', 'spruce_fan', 'spruce_cones']


SPRAY_BASE_V = 0.035     # where a spray's twig enters its cell (v from the cell's bottom); u = 0.5


def cell_rect(i):
    """(u0, v0, u1, v1) of atlas cell i in UV space (v up)."""
    c, r = i % 4, i // 4
    return c / 4.0, 1.0 - (r + 1) / 4.0, (c + 1) / 4.0, 1.0 - r / 4.0


class SprayMesh:
    """Accumulates the triangles of one spray (cell coordinates: meters, base at (0, 0), axis +Y, up +Z) with
    per-vertex albedo (linear), roughness, snow and thickness."""

    def __init__(self):
        self.P, self.F, self.A = [], [], []
        self.n = 0

    def add(self, P, F, alb, rough, snow, thick):
        P = np.asarray(P, dtype=np.float32)
        k = len(P)
        self.P.append(P)
        self.F.append(np.asarray(F, dtype=np.int64) + self.n)
        a = np.empty((k, 6), dtype=np.float32)
        a[:, 0:3] = np.broadcast_to(np.asarray(alb, dtype=np.float32), (k, 3)) if np.ndim(alb) == 1 else alb
        a[:, 3] = rough
        a[:, 4] = snow
        a[:, 5] = thick
        self.A.append(a)
        self.n += k

    def obj(self, name):
        P = np.concatenate(self.P)
        F = np.concatenate(self.F)
        A = np.concatenate(self.A)
        me = bpy.data.meshes.new(name)
        me.vertices.add(len(P))
        me.vertices.foreach_set('co', P.ravel())
        me.loops.add(F.size)
        me.loops.foreach_set('vertex_index', F.ravel().astype(np.int32))
        me.polygons.add(len(F))
        me.polygons.foreach_set('loop_start', (np.arange(len(F)) * 3).astype(np.int32))
        me.update(calc_edges=True)
        for nm, cols in (('alb', A[:, 0:3]), ('srt', A[:, 3:6])):
            at = me.color_attributes.new(nm, 'FLOAT_COLOR', 'POINT')
            c4 = np.ones((len(P), 4), dtype=np.float32)
            c4[:, :3] = cols
            at.data.foreach_set('color', c4.ravel())
        me.shade_smooth()
        o = bpy.data.objects.new(name, me)
        core.link(o)
        return o


def _frame_vecs(t):
    """Two unit vectors perpendicular to t (N x 3)."""
    ref = np.where(np.abs(t[:, 2:3]) < 0.9, np.array([[0.0, 0.0, 1.0]]), np.array([[1.0, 0.0, 0.0]]))
    a = np.cross(t, ref)
    a /= np.linalg.norm(a, axis=1, keepdims=True)
    b = np.cross(t, a)
    return a, b


def add_twig(sm, pts, r0, r1, col, sides=5):
    """A tapered twig along a polyline (cell meters)."""
    pts = np.asarray(pts, dtype=np.float64)
    n = len(pts)
    t = np.gradient(pts, axis=0)
    t /= np.maximum(np.linalg.norm(t, axis=1, keepdims=True), 1e-9)
    a, b = _frame_vecs(t)
    rad = np.linspace(r0, r1, n)
    ang = np.linspace(0.0, 2 * math.pi, sides, endpoint=False)
    ring = pts[:, None, :] + rad[:, None, None] * (np.cos(ang)[None, :, None] * a[:, None, :] + np.sin(ang)[None, :, None] * b[:, None, :])
    P = ring.reshape(-1, 3)
    F = []
    for i in range(n - 1):
        for k in range(sides):
            p0, p1 = i * sides + k, i * sides + (k + 1) % sides
            q0, q1 = p0 + sides, p1 + sides
            F += [(p0, p1, q1), (p0, q1, q0)]
    sm.add(P, F, col, 0.78, 0.0, 1.0)


def add_needles(sm, rng, pts, per_m, length, alb_fn, frost=0.0, up_bias=0.75, spread=(50.0, 68.0), width=0.00055, droop=0.0, flat=0.6):
    """Needles all round a twig (spiral phyllotaxis), pointing forward; those under the twig bend up and aside, as a
    spruce's do; flat < 1 lays them out sideways (a shade branch's spray). alb_fn(rng) -> linear color; frost whitens
    the tips of the upward needles."""
    pts = np.asarray(pts, dtype=np.float64)
    seg = np.linalg.norm(np.diff(pts, axis=0), axis=1)
    s = np.concatenate([[0.0], np.cumsum(seg)])
    L = s[-1]
    cnt = int(L * per_m)
    if cnt < 1:
        return
    ss = np.sort(rng_uniform(rng, 0.0, L, cnt))
    pos = np.column_stack([np.interp(ss, s, pts[:, k]) for k in range(3)])
    idx = np.clip(np.searchsorted(s, ss) - 1, 0, len(seg) - 1)
    t = pts[idx + 1] - pts[idx]
    t /= np.maximum(np.linalg.norm(t, axis=1, keepdims=True), 1e-9)
    a, b = _frame_vecs(t)
    phi = np.arange(cnt) * math.radians(137.508) + rng.random() * 6.28
    alpha = np.radians(rng_uniform(rng, spread[0], spread[1], cnt))
    around = np.cos(phi)[:, None] * a + np.sin(phi)[:, None] * b
    d = np.cos(alpha)[:, None] * t + np.sin(alpha)[:, None] * around
    down = d[:, 2] < -0.15
    d[down, 2] = d[down, 2] * (1.0 - up_bias)
    d[:, 2] = d[:, 2] * flat - droop
    d /= np.linalg.norm(d, axis=1, keepdims=True)
    lens = length[0] + (length[1] - length[0]) * rng_uniform(rng, 0.0, 1.0, cnt)
    lens *= (1.0 - 0.35 * (ss / L) ** 3)
    # A slight curve: each needle bends a little forward along the twig.
    base = pos + d * 0.0006
    mid = pos + d * lens[:, None] * 0.5 + t * lens[:, None] * 0.06
    tip = pos + d * lens[:, None] + t * lens[:, None] * 0.12
    ea, eb = _frame_vecs(d)
    w = width
    P = np.empty((cnt, 9, 3))
    P[:, 0] = base + (ea + eb) * w
    P[:, 1] = base + (ea - eb) * w
    P[:, 2] = base + (-ea - eb) * w
    P[:, 3] = base + (-ea + eb) * w
    P[:, 4] = mid + (ea + eb) * w * 0.85
    P[:, 5] = mid + (ea - eb) * w * 0.85
    P[:, 6] = mid + (-ea - eb) * w * 0.85
    P[:, 7] = mid + (-ea + eb) * w * 0.85
    P[:, 8] = tip
    F = []
    for k in range(4):
        k1 = (k + 1) % 4
        F += [(k, k1, 4 + k1), (k, 4 + k1, 4 + k), (4 + k, 4 + k1, 8)]
    F = np.array(F)
    Fall = (F[None, :, :] + 9 * np.arange(cnt)[:, None, None]).reshape(-1, 3)
    cols = np.array([alb_fn(rng) for _ in range(cnt)], dtype=np.float32)
    C = np.repeat(cols[:, None, :], 9, axis=1)
    if frost > 0.0:
        up = np.clip(d[:, 2] * 2.0 + 0.4, 0.0, 1.0) * frost * rng_uniform(rng, 0.4, 1.0, cnt)
        white = np.array([0.80, 0.85, 0.9], dtype=np.float32)
        C[:, 8] = C[:, 8] * (1.0 - up[:, None]) + white * up[:, None]
        C[:, 4:8] = C[:, 4:8] * (1.0 - 0.35 * up[:, None, None]) + white * 0.35 * up[:, None, None]
    sm.add(P.reshape(-1, 3), Fall, C.reshape(-1, 3), 0.42, 0.0, 0.5)


def rng_uniform(rng, lo, hi, n):
    return lo + (hi - lo) * np.array([rng.random() for _ in range(n)])


_ICO = {}


def add_snow_lump(sm, rng, center, rx, ry, rz, yaw=0.0, snow_col=(0.84, 0.88, 0.93)):
    """A lump of snow (an icosphere, squashed, lumpy, flat underneath); grains get fewer faces."""
    sub = 1 if max(rx, ry) < 0.006 else (2 if max(rx, ry) < 0.025 else 3)
    if sub not in _ICO:
        bm = bmesh.new()
        bmesh.ops.create_icosphere(bm, subdivisions=sub, radius=1.0)
        _ICO[sub] = (np.array([v.co[:] for v in bm.verts]), np.array([[v.index for v in f.verts] for f in bm.faces]))
        bm.free()
    V, F = _ICO[sub]
    V = V.copy()
    seed = rng.randrange(1 << 20)
    n = fbm(V[:, 0] * 2.0 + seed % 97, V[:, 1] * 2.0, 1.0, 3, seed=seed % 1000) + 0.5 * fbm(V[:, 2] * 3.0, V[:, 0] * 3.0, 1.0, 2, seed=seed % 1000 + 5)
    V = V * (1.0 + 0.28 * n[:, None])
    V[:, 2] = np.where(V[:, 2] < -0.2, -0.2 + (V[:, 2] + 0.2) * 0.25, V[:, 2])
    V = V * np.array([rx, ry, rz])
    c, s = math.cos(yaw), math.sin(yaw)
    V = V @ np.array([[c, s, 0.0], [-s, c, 0.0], [0.0, 0.0, 1.0]])
    V = V + np.asarray(center)
    sm.add(V, F, np.asarray(snow_col), 0.58, 1.0, 0.25)


def _curve(p0, p1, bulge, n=8):
    """Points along a gentle quadratic curve from p0 to p1, bulging by the vector `bulge` at its middle."""
    p0, p1, bulge = np.asarray(p0, float), np.asarray(p1, float), np.asarray(bulge, float)
    mid = (p0 + p1) / 2 + bulge
    t = np.linspace(0.0, 1.0, n)[:, None]
    return (1 - t) ** 2 * p0 + 2 * (1 - t) * t * mid + t ** 2 * p1


def spruce_spray(rng, kind):
    """One spruce spray (cell coordinates: base at the bottom, axis +Y, up +Z). kind: 'frost' (rimed, a dusting),
    'snow' (snow riding the twigs), 'young' (a nursery tree's: greener, denser, short needles), 'fan' (a broad
    branch end), 'dead' (the lower trunk's dead twigs with beard lichen), 'leader' (a treetop)."""
    sm = SprayMesh()
    young = kind == 'young'
    dead = kind == 'dead'
    L = {'fan': 0.47, 'young': 0.41, 'leader': 0.47, 'dead': 0.46}.get(kind, 0.46) * (0.94 + 0.08 * rng.random())
    W = {'fan': 0.47, 'young': 0.36, 'leader': 0.15, 'dead': 0.40}.get(kind, 0.42)
    base = np.array([0.0, SPRAY_BASE_V * CELL_M - CELL_M / 2, 0.0])
    tipx = (rng.random() - 0.5) * 0.05
    main = _curve(base, base + np.array([tipx, L, 0.012]), np.array([(rng.random() - 0.5) * 0.03, 0.0, 0.015]), 16)
    twig_col = np.array(core.hex_linear(0x6a4630)[:3]) if not dead else np.array(core.hex_linear(0x5d5752)[:3])
    twigs = [(main, 0.0024, 0.0009)]
    laterals = []
    nlat = {'leader': 8, 'young': 14}.get(kind, 14)
    for i in range(nlat):
        t = 0.05 + 0.88 * (i + 0.35 * rng.random()) / nlat
        side = 1 if i % 2 == 0 else -1
        p = main[int(t * (len(main) - 1))]
        ll = (W * 0.52) * (1.0 - t) ** 0.5 * (0.8 + 0.35 * rng.random())
        if kind == 'leader':
            ll *= 0.6
        ang = math.radians(46.0 + 16.0 * rng.random())
        d = np.array([side * math.sin(ang), math.cos(ang), -0.03 - 0.07 * rng.random()])
        end = p + d * ll
        lat = _curve(p, end, np.array([0.0, 0.012, 0.005]) * ll / 0.1, 9)
        twigs.append((lat, 0.0015, 0.0006))
        laterals.append(lat)
        if ll > 0.06:
            nt = 3 if ll > 0.13 else (2 if ll > 0.09 else 1)
            for k in range(nt):
                tt = 0.25 + 0.55 * (k + 0.5) / nt + 0.08 * (rng.random() - 0.5)
                q = lat[int(tt * (len(lat) - 1))]
                a2 = math.radians(38.0 + 16.0 * rng.random())
                dl = lat[-1] - lat[0]
                dl = dl / np.linalg.norm(dl)
                perp = np.array([-dl[1], dl[0], 0.0]) * (1 if k % 2 == 0 else -1)
                d2 = dl * math.cos(a2) + perp * math.sin(a2)
                l2 = ll * (0.3 + 0.18 * rng.random()) * (1.0 - 0.4 * tt)
                twigs.append((_curve(q, q + d2 * l2, np.array([0.0, 0.0, 0.003]), 6), 0.001, 0.0005))
    for pts, r0, r1 in twigs:
        add_twig(sm, pts, r0, r1, twig_col, 5 if r0 > 0.0018 else 4)
    if dead:
        for pts, r0, r1 in twigs[1:]:
            if rng.random() < 0.45:
                add_needles(sm, rng, pts, 260.0, (0.010, 0.016), lambda r: np.array(core.hex_linear(0x6b5232)[:3]) * (0.8 + 0.4 * r.random()), up_bias=0.2, droop=0.4)
            if rng.random() < 0.65:
                add_needles(sm, rng, pts, 420.0, (0.015, 0.04), lambda r: np.array(core.hex_linear(0x8f9a7c)[:3]) * (0.85 + 0.3 * r.random()), up_bias=0.0,
                            spread=(70.0, 110.0), width=0.00028, droop=0.6, flat=1.0)
        return sm

    def needle_col(r):
        h = core.hex_linear(0x30503f if not young else 0x3a5f3e)
        h2 = core.hex_linear(0x1c3225 if not young else 0x27472c)
        f = r.random()
        c = np.array(h[:3]) * (1 - f) + np.array(h2[:3]) * f
        return c * (0.82 + 0.34 * r.random())

    dens = 1350.0 if not young else 1500.0
    nl = (0.019, 0.029) if not young else (0.014, 0.021)
    frost = {'frost': 0.32, 'snow': 0.2, 'young': 0.12, 'fan': 0.25, 'leader': 0.25}.get(kind, 0.0)
    for k, (pts, r0, r1) in enumerate(twigs):
        add_needles(sm, rng, pts, dens, nl, needle_col, frost=frost, width=0.0006)
    # Snow riding the twigs (the 'snow' sprays carry it along the main twig and the laterals' inner parts, leaving the
    # needles showing round it); a fine dusting of grains on the rest.
    if kind in ('snow', 'fan'):
        heavy = kind == 'snow'
        # A continuous pillow along the main twig (wider at the base, gaps where the wind took it), pillows on the
        # laterals' inner halves, snow caught in the forks.
        for j, pts in enumerate([main] + laterals):
            reach = (0.92 if j == 0 else 0.55) * (1.0 if heavy else 0.6)
            seg = np.linalg.norm(np.diff(pts, axis=0), axis=1)
            sl = np.concatenate([[0.0], np.cumsum(seg)])
            total = sl[-1] * reach
            step = 0.009
            ph = rng.random() * 50.0
            for k in range(int(total / step)):
                t = (k + 0.5) * step
                # The pillow's width wanders along it; it breaks where the wind or a drip took it.
                wob = float(fbm(np.array([t * 40.0 + ph]), np.array([j * 3.1]), 1.0, 3, seed=611)[0])
                if wob < -0.32 or rng.random() < 0.05:
                    continue
                q = np.array([np.interp(t, sl, pts[:, c]) for c in range(3)])
                i2 = min(int(np.searchsorted(sl, t)), len(pts) - 1)
                d = pts[i2] - pts[max(i2 - 1, 0)]
                yaw = math.atan2(d[1], d[0]) - math.pi / 2
                f = 1.0 - t / max(sl[-1], 1e-6)
                w = (0.006 + 0.009 * f) * (1.0 if j == 0 else 0.8) * (0.75 + 0.7 * max(wob + 0.3, 0.0)) * (1.0 if heavy else 0.7)
                side = (rng.random() - 0.5) * 0.006 + 0.004 * wob
                q = q + np.array([math.cos(yaw) * side, math.sin(yaw) * side, 0.0])
                add_snow_lump(sm, rng, q + np.array([0.0, 0.0, 0.008 + 0.5 * w]), w, 0.011 + 0.006 * rng.random(), (0.45 + 0.3 * rng.random()) * w, yaw)
            if j > 0 and heavy:
                q = pts[0] + (pts[2] - pts[0]) * 0.5
                add_snow_lump(sm, rng, q + np.array([0.0, 0.0, 0.012]), 0.016, 0.02, 0.01, rng.random())
    ngr = {'frost': 110, 'snow': 90, 'young': 30, 'fan': 90, 'leader': 60}.get(kind, 0)
    for _ in range(ngr):
        pts = twigs[rng.randrange(len(twigs))][0]
        q = pts[rng.randrange(len(pts))] + np.array([(rng.random() - 0.5) * 0.03, (rng.random() - 0.5) * 0.03, 0.008 + 0.004 * rng.random()])
        g = 0.0007 + 0.0011 * rng.random()
        add_snow_lump(sm, rng, q, g, g * 1.3, g * 0.7, rng.random() * 6.0)
    return sm


def pine_tuft(rng, snow=False):
    """A lodgepole pine's branch end: a short stout twig whose outer half is a dense foxtail of long paired needles,
    two side shoots the same."""
    sm = SprayMesh()
    base = np.array([0.0, SPRAY_BASE_V * CELL_M - CELL_M / 2, 0.0])
    L = 0.42 + 0.05 * rng.random()
    main = _curve(base, base + np.array([(rng.random() - 0.5) * 0.08, L, 0.02]), np.array([0.02, 0.0, 0.02]), 12)
    twig_col = np.array(core.hex_linear(0x7a5a3e)[:3])
    twigs = [(main, 0.006, 0.0035)]
    for k in range(3):
        t = 0.42 + 0.17 * k + 0.05 * rng.random()
        p = main[int(t * (len(main) - 1))]
        side = 1 if k % 2 == 0 else -1
        d = np.array([side * 0.72, 0.69, 0.06])
        twigs.append((_curve(p, p + d * (0.14 + 0.05 * rng.random()), np.array([0.0, 0.012, 0.01]), 8), 0.004, 0.0025))
    for pts, r0, r1 in twigs:
        add_twig(sm, pts, r0, r1, twig_col, 6)

    def needle_col(r):
        c = np.array(core.hex_linear(0x31482a)[:3]) * (1 - r.random() * 0.5) + np.array(core.hex_linear(0x46603a)[:3]) * r.random() * 0.5
        return c * (0.82 + 0.34 * r.random())
    for j, (pts, r0, r1) in enumerate(twigs):
        n = len(pts)
        outer = pts[int(n * (0.3 if j == 0 else 0.15)):]
        add_needles(sm, rng, outer, 3400.0, (0.055, 0.082), needle_col, frost=0.2 if not snow else 0.4, up_bias=0.25,
                    spread=(16.0, 68.0), width=0.0007, droop=0.04, flat=0.8)
    if snow:
        for pts, r0, r1 in twigs:
            for q in (pts[-4], pts[-7]):
                add_snow_lump(sm, rng, q + np.array([0.0, 0.0, 0.03]), 0.022, 0.035, 0.016, rng.random())
    return sm


def snow_clump(rng, variant):
    """A pillow of snow as it lies along a branch, seen from above: a long mound and its satellites."""
    sm = SprayMesh()
    y0 = SPRAY_BASE_V * CELL_M - CELL_M / 2
    n = 9 if variant == 0 else 12
    for k in range(n):
        t = k / (n - 1)
        y = y0 + 0.03 + t * 0.4
        w = 0.07 * (1.0 - 0.6 * abs(t - 0.4)) * (0.8 + 0.4 * rng.random())
        add_snow_lump(sm, rng, (math.sin(t * 3.0 + variant) * 0.03, y, 0.0), w, 0.06, 0.035 + 0.02 * rng.random(), rng.random() * 0.5)
    for k in range(10 + 6 * variant):
        add_snow_lump(sm, rng, ((rng.random() - 0.5) * 0.26, y0 + 0.05 + 0.4 * rng.random(), -0.01), 0.02 + 0.025 * rng.random(),
                      0.02 + 0.03 * rng.random(), 0.015, rng.random() * 3)
    # A few needle tips poking through.
    tw = [_curve((0.0, y0, -0.03), (0.02, y0 + 0.44, -0.02), (0.0, 0.0, 0.0), 10)]
    add_needles(sm, rng, tw[0], 300.0, (0.015, 0.022), lambda r: np.array(core.hex_linear(0x34503f)[:3]) * (0.8 + 0.3 * r.random()), frost=0.4)
    return sm


def cone_cluster(rng):
    """Engelmann spruce cones (5 cm, light brown, papery scales) hanging in a bunch from a twig with needles."""
    sm = SprayMesh()
    y0 = SPRAY_BASE_V * CELL_M - CELL_M / 2
    tw = _curve((0.0, y0, 0.0), (0.0, y0 + 0.4, 0.02), (0.02, 0.0, 0.01), 10)
    add_twig(sm, tw, 0.004, 0.0015, np.array(core.hex_linear(0x6e4a32)[:3]))
    add_needles(sm, rng, tw, 600.0, (0.015, 0.022), lambda r: np.array(core.hex_linear(0x3a584c)[:3]) * (0.85 + 0.3 * r.random()), frost=0.4)
    for k in range(5):
        c = np.array([(rng.random() - 0.5) * 0.14, y0 + 0.12 + 0.24 * rng.random(), 0.01])
        ln = 0.045 + 0.015 * rng.random()
        # A cone: scales as small lumps round an axis (spiral), tapering at both ends.
        ax = np.array([rng.random() - 0.5, -0.8, -0.2])
        ax /= np.linalg.norm(ax)
        a, b = _frame_vecs(ax[None, :])
        for j in range(26):
            tt = j / 25.0
            r = 0.011 * math.sin(math.pi * (0.15 + 0.8 * tt))
            ph = j * 2.4
            p = c + ax * (tt - 0.5) * ln + r * (math.cos(ph) * a[0] + math.sin(ph) * b[0])
            add_snow_lump(sm, rng, p, 0.006, 0.008, 0.004, ph, snow_col=tuple(np.array(core.hex_linear(0x8a6a48)[:3]) * (0.8 + 0.3 * rng.random())))
    # The cone scales are not snow: mark them as wood (the lumps added above carried snow 1).
    a = sm.A[-26 * 5:]
    for arr in a:
        arr[:, 3] = 0.7
        arr[:, 4] = 0.0
        arr[:, 5] = 0.8
    add_snow_lump(sm, rng, (0.0, y0 + 0.2, 0.03), 0.05, 0.09, 0.02, 0.2)
    return sm


def _pass_material(kind):
    """Emission-only materials that write one map each: 'alb', 'nrm' (world normal -> 0..1), 'msk' (AO, roughness,
    snow), 'thk' (thickness)."""
    m = core.Mat('_atlas_' + kind)
    em = m.node('ShaderNodeEmission')
    if kind == 'alb':
        at = m.node('ShaderNodeVertexColor')
        at.layer_name = 'alb'
        m.link(at.outputs['Color'], em.inputs['Color'])
    elif kind == 'nrm':
        geo = m.node('ShaderNodeNewGeometry')
        vm = m.node('ShaderNodeVectorMath')
        vm.operation = 'MULTIPLY_ADD'
        vm.inputs[1].default_value = (0.5, 0.5, 0.5)
        vm.inputs[2].default_value = (0.5, 0.5, 0.5)
        m.link(geo.outputs['Normal'], vm.inputs[0])
        m.link(vm.outputs['Vector'], em.inputs['Color'])
    elif kind == 'msk':
        at = m.node('ShaderNodeVertexColor')
        at.layer_name = 'srt'
        sep = m.node('ShaderNodeSeparateColor')
        m.link(at.outputs['Color'], sep.inputs['Color'])
        ao = m.node('ShaderNodeAmbientOcclusion', Distance=0.025)
        ao.samples = 16
        comb = m.node('ShaderNodeCombineColor')
        m.link(ao.outputs['AO'], comb.inputs[0])
        m.link(sep.outputs['Red'], comb.inputs[1])
        m.link(sep.outputs['Green'], comb.inputs[2])
        m.link(comb.outputs['Color'], em.inputs['Color'])
    else:
        at = m.node('ShaderNodeVertexColor')
        at.layer_name = 'srt'
        sep = m.node('ShaderNodeSeparateColor')
        m.link(at.outputs['Color'], sep.inputs['Color'])
        comb = m.node('ShaderNodeCombineColor')
        for k in range(3):
            m.link(sep.outputs['Blue'], comb.inputs[k])
        m.link(comb.outputs['Color'], em.inputs['Color'])
    m.link(em.outputs['Emission'], m.out.inputs['Surface'])
    return m.m


def _read_exr(path):
    img = bpy.data.images.load(path, check_existing=False)
    w, h = img.size
    a = np.empty(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(a)
    bpy.data.images.remove(img)
    return a.reshape(h, w, 4)


def render_needle_atlas():
    """Builds each cell's spray, renders its maps from above (orthographic, emission passes), composes the 4096 atlas:
    returns {'col': (H, W, 4) linear albedo + coverage, 'nrm': (H, W, 3), 'msk': (H, W, 4)} (rows bottom-up)."""
    t0 = time.time()
    core.reset()
    sc = bpy.context.scene
    sc.render.film_transparent = True
    sc.view_settings.view_transform = 'Standard'
    sc.view_settings.look = 'None'
    sc.cycles.use_denoising = False
    sc.cycles.samples = 24 if FAST else 48
    sc.cycles.use_adaptive_sampling = False
    sc.render.filter_size = 1.0
    px = CELL // 4 if FAST else CELL
    sc.render.resolution_x = px
    sc.render.resolution_y = px
    sc.render.image_settings.file_format = 'OPEN_EXR'
    sc.render.image_settings.color_depth = '32'
    sc.render.image_settings.color_mode = 'RGBA'
    sc.world.color = (0.0, 0.0, 0.0)
    cam_d = bpy.data.cameras.new('atlas_cam')
    cam_d.type = 'ORTHO'
    cam_d.ortho_scale = CELL_M
    cam_d.clip_start = 0.01
    cam_d.clip_end = 10.0
    cam = core.link(bpy.data.objects.new('atlas_cam', cam_d))
    cam.location = (0.0, 0.0, 2.0)
    sc.camera = cam
    mats = {k: _pass_material(k) for k in ('alb', 'nrm', 'msk', 'thk')}
    tmp = os.path.join(core.BUILD_DIR, 'ecx_atlas')
    os.makedirs(tmp, exist_ok=True)
    W = 4 * px
    col = np.zeros((W, W, 4), dtype=np.float32)
    nrm = np.zeros((W, W, 3), dtype=np.float32)
    msk = np.zeros((W, W, 4), dtype=np.float32)
    rng = random.Random(4090)
    only_cells = [int(c) for c in os.environ.get('ECX_CELLS', '').split(',') if c.strip()]
    for i, kind in enumerate(CELLS):
        if only_cells and i not in only_cells:
            continue
        if kind.startswith('spruce_frost'):
            sm = spruce_spray(rng, 'frost')
        elif kind.startswith('spruce_snow'):
            sm = spruce_spray(rng, 'snow')
        elif kind == 'spruce_young':
            sm = spruce_spray(rng, 'young')
        elif kind == 'dead_twigs':
            sm = spruce_spray(rng, 'dead')
        elif kind == 'spruce_leader':
            sm = spruce_spray(rng, 'leader')
        elif kind == 'spruce_fan':
            sm = spruce_spray(rng, 'fan')
        elif kind == 'spruce_cones':
            sm = cone_cluster(rng)
        elif kind.startswith('pine_tuft'):
            sm = pine_tuft(rng, snow=kind.endswith('snow'))
        else:
            sm = snow_clump(rng, 0 if kind.endswith('a') else 1)
        o = sm.obj('spray_' + kind)
        out = {}
        for k in ('alb', 'nrm', 'msk', 'thk'):
            o.data.materials.clear()
            o.data.materials.append(mats[k])
            path = os.path.join(tmp, f'{i:02d}_{k}.exr')
            sc.render.filepath = path
            bpy.ops.render.render(write_still=True)
            out[k] = _read_exr(path)
        bpy.data.objects.remove(o)
        a = out['alb'][:, :, 3:4]
        inv = np.where(a > 1e-4, 1.0 / np.maximum(a, 1e-4), 0.0)
        u0, v0, u1, v1 = cell_rect(i)
        x0, y0 = int(round(u0 * W)), int(round(v0 * W))
        col[y0:y0 + px, x0:x0 + px, :3] = out['alb'][:, :, :3] * inv
        col[y0:y0 + px, x0:x0 + px, 3] = a[:, :, 0]
        nrm[y0:y0 + px, x0:x0 + px] = out['nrm'][:, :, :3] * inv
        msk[y0:y0 + px, x0:x0 + px, :3] = out['msk'][:, :, :3] * inv
        msk[y0:y0 + px, x0:x0 + px, 3] = (out['thk'][:, :, :1] * inv)[:, :, 0]
        log(f'atlas cell {i} {kind}: {sm.n} vertices, coverage {float(a.mean()):.2f}')
    # Fill every map out from the sprays' edges, so filtering and mipmaps never pull in black.
    valid = col[:, :, 3] > 0.02
    for i in range(16):
        u0, v0, u1, v1 = cell_rect(i)
        x0, y0 = int(round(u0 * W)), int(round(v0 * W))
        sl = (slice(y0, y0 + px), slice(x0, x0 + px))
        vv = valid[sl]
        col[sl][:, :, :3] = pushpull_fill(col[sl][:, :, :3], vv)
        # Normals: the sprays' own a few texels out, then easing to flat (no stray tilt bleeds into the mips).
        grown = pushpull_fill(nrm[sl], vv)
        near = blur2(vv.astype(np.float32), max(2, px // 128))
        w = np.clip(near * 3.0, 0.0, 1.0)[:, :, None]
        flat = np.array([0.5, 0.5, 1.0], dtype=np.float32)
        nrm[sl] = np.where(vv[:, :, None], nrm[sl], grown * w + flat * (1.0 - w))
        mk = msk[sl].copy()
        mk[:, :, :3] = pushpull_fill(mk[:, :, :3], vv)
        mk[:, :, 3:] = pushpull_fill(mk[:, :, 3:], vv)
        msk[sl] = mk
    log(f'needle atlas rendered in {time.time() - t0:.0f}s')
    core.reset()
    return {'col': col, 'nrm': nrm, 'msk': msk}


def pushpull_fill(img, valid):
    """Fills the invalid texels of an (H, W, C) image from its valid ones by a pull-push pyramid (smooth, fast)."""
    img = img.astype(np.float32)
    w = valid.astype(np.float32)[:, :, None]
    levels = []
    c, a = img * w, w
    while min(c.shape[0], c.shape[1]) > 1:
        levels.append((c, a))
        h2, w2 = c.shape[0] // 2, c.shape[1] // 2
        c = c[:h2 * 2, :w2 * 2].reshape(h2, 2, w2, 2, -1).sum(axis=(1, 3))
        a = a[:h2 * 2, :w2 * 2].reshape(h2, 2, w2, 2, 1).sum(axis=(1, 3))
    fill = c / np.maximum(a, 1e-8)
    for c, a in reversed(levels):
        up = np.repeat(np.repeat(fill, 2, axis=0), 2, axis=1)[:c.shape[0], :c.shape[1]]
        if min(up.shape[:2]) > 4:
            up = np.stack([blur2(up[:, :, k], 1) for k in range(up.shape[2])], axis=2)
        have = np.clip(a, 0.0, 1.0)
        fill = (c / np.maximum(a, 1e-8)) * have + up * (1.0 - have)
    return np.where(valid[:, :, None], img, fill)


def save_png(path, arr, srgb=False):
    """Writes an (H, W, 3|4) float array (rows bottom-up) as an 8-bit PNG; srgb: encode the color channels from linear
    (alpha stays linear)."""
    H, W, C = arr.shape
    px = np.ones((H, W, 4), dtype=np.float32)
    rgb = np.clip(arr[:, :, :3], 0.0, 1.0)
    if srgb:
        rgb = np.where(rgb <= 0.0031308, rgb * 12.92, 1.055 * np.power(np.maximum(rgb, 0.0031308), 1 / 2.4) - 0.055)
    px[:, :, :3] = rgb
    if C == 4:
        px[:, :, 3] = np.clip(arr[:, :, 3], 0.0, 1.0)
    img = bpy.data.images.new(os.path.basename(path), W, H, alpha=(C == 4))
    img.colorspace_settings.name = 'sRGB' if srgb else 'Non-Color'
    img.pixels.foreach_set(px.ravel())
    os.makedirs(os.path.dirname(path), exist_ok=True)
    img.filepath_raw = path
    img.file_format = 'PNG'
    img.save()
    bpy.data.images.remove(img)
    return path


def _pgn(u, v, cx, cy, seed):
    """One octave of periodic gradient noise with a cx x cy lattice over the unit tile."""
    x = u * cx
    y = v * cy
    xf = np.floor(x)
    yf = np.floor(y)
    fx = (x - xf).astype(np.float32)
    fy = (y - yf).astype(np.float32)
    ix = xf.astype(np.int64) % cx
    iy = yf.astype(np.int64) % cy
    ix1 = (ix + 1) % cx
    iy1 = (iy + 1) % cy
    g = []
    for a, b in ((ix, iy), (ix1, iy), (ix, iy1), (ix1, iy1)):
        h = _hash(a, b, seed) & np.uint32(7)
        g.append((_GX[h], _GY[h]))
    n00 = g[0][0] * fx + g[0][1] * fy
    n10 = g[1][0] * (fx - 1) + g[1][1] * fy
    n01 = g[2][0] * fx + g[2][1] * (fy - 1)
    n11 = g[3][0] * (fx - 1) + g[3][1] * (fy - 1)
    uu = _fade(fx)
    vv = _fade(fy)
    nx0 = n00 + uu * (n10 - n00)
    nx1 = n01 + uu * (n11 - n01)
    return (nx0 + vv * (nx1 - nx0)) * 1.5


def pfbm(u, v, nx, ny, octaves, seed=0, gain=0.5):
    out = np.zeros(np.shape(u), dtype=np.float32)
    amp, total = 1.0, 0.0
    for o in range(octaves):
        out += amp * _pgn(u, v, nx << o, ny << o, seed + 31 * o)
        total += amp
        amp *= gain
    return out / total


def pcells(u, v, cx, cy, seed=0, jitter=0.9):
    """Tileable Voronoi over the unit tile with a cx x cy grid: (F1, F2 in tile units scaled by cx, the cell's random,
    offsets du, dv to the point in cell units)."""
    gx = u * cx
    gy = v * cy
    ix0 = np.floor(gx).astype(np.int64)
    iy0 = np.floor(gy).astype(np.int64)
    fx = (gx - ix0).astype(np.float32)
    fy = (gy - iy0).astype(np.float32)
    f1 = np.full(np.shape(u), 1e9, dtype=np.float32)
    f2 = np.full(np.shape(u), 1e9, dtype=np.float32)
    rid = np.zeros(np.shape(u), dtype=np.float32)
    ddx = np.zeros(np.shape(u), dtype=np.float32)
    ddy = np.zeros(np.shape(u), dtype=np.float32)
    for ox in (-1, 0, 1):
        for oy in (-1, 0, 1):
            ix = (ix0 + ox) % cx
            iy = (iy0 + oy) % cy
            px = ox + 0.5 + jitter * (hash01(ix, iy, seed) - 0.5)
            py = oy + 0.5 + jitter * (hash01(ix, iy, seed + 1) - 0.5)
            ex = px - fx
            ey = py - fy
            d = np.sqrt(ex * ex + ey * ey)
            closer = d < f1
            f2 = np.where(closer, f1, np.minimum(f2, d))
            rid = np.where(closer, hash01(ix, iy, seed + 2), rid)
            ddx = np.where(closer, ex, ddx)
            ddy = np.where(closer, ey, ddy)
            f1 = np.where(closer, d, f1)
    return f1, f2, rid, ddx, ddy


def tile_normal(height, du_m, dv_m, strength=1.0):
    """Tangent-space normal (OpenGL, rows bottom-up) of a tileable height field (meters); du_m, dv_m: texel size (m)."""
    gx = (np.roll(height, -1, axis=1) - np.roll(height, 1, axis=1)) / (2 * du_m) * strength
    gy = (np.roll(height, -1, axis=0) - np.roll(height, 1, axis=0)) / (2 * dv_m) * strength
    n = np.stack([-gx, -gy, np.ones_like(gx)], axis=2)
    n /= np.linalg.norm(n, axis=2, keepdims=True)
    return n * 0.5 + 0.5


def tile_blur(a, r):
    """Box-blur (3 passes) a tileable 2D array with wrap-around."""
    return blur2(a, r, axis_wrap=(True, True))


def tile_grid(W, H):
    """u, v (0..1) texel centers of a W x H tile, rows bottom-up."""
    u = (np.arange(W, dtype=np.float64) + 0.5) / W
    v = (np.arange(H, dtype=np.float64) + 0.5) / H
    U, V = np.meshgrid(u, v)
    return U, V


def bark_maps(W=1024, H=2048):
    """Spruce bark, a 0.5 x 1.0 m tile: thin irregular scales lifted at their lower edges (shingled), grey-brown and
    weathered on top, reddish where a scale has flaked off, a few deeper fissures, lichen, a resin bead or two.
    Returns (base linear, normal, orm)."""
    U, V = tile_grid(W, H)
    wu = U + 0.018 * pfbm(U, V, 6, 12, 3, seed=500)
    wv = V + 0.012 * pfbm(U, V, 6, 12, 3, seed=509)
    f1, f2, rid, du, dv = pcells(wu, wv, 15, 19, seed=501, jitter=0.95)
    g1, g2, rid2, du2, dv2 = pcells(wu * 1.0 + 0.37, wv, 28, 30, seed=511, jitter=0.95)
    small = rid2 > 0.55                       # some areas split into smaller scales
    edge = np.where(small, g2 - g1, f2 - f1)
    dvv = np.where(small, dv2 * 0.6, dv)
    rr = np.where(small, rid2, rid)
    # Each scale rises toward its lower edge (where it lifts) and is cut off there; soft at its other edges.
    lift = np.clip(0.55 - dvv * 1.1, 0.0, 1.0) ** 1.5 * smoothstep(0.0, 0.12, edge)
    h = 0.0018 * lift + 0.0007 * pfbm(U, V, 20, 40, 4, seed=502)
    seam = 1.0 - smoothstep(0.0, 0.05, edge)
    h -= 0.0007 * seam
    # A few long vertical fissures on old bark.
    fis = np.abs(pfbm(U * 1.0, V, 5, 2, 3, seed=503))
    fissure = (1.0 - smoothstep(0.0, 0.06, fis)) * smoothstep(-0.1, 0.3, pfbm(U, V, 3, 4, 2, seed=512))
    h -= 0.003 * fissure
    h += 0.001 * pfbm(U, V, 3, 6, 3, seed=504)
    grey = np.array(core.hex_linear(0x726a62)[:3])
    brown = np.array(core.hex_linear(0x56463e)[:3])
    red = np.array(core.hex_linear(0x7a4a36)[:3])
    dark = np.array(core.hex_linear(0x2c2420)[:3])
    t = (0.5 + 0.5 * pfbm(U, V, 5, 10, 4, seed=505))[..., None]
    col = grey * t + brown * (1 - t)
    col = col * (0.86 + 0.24 * rr[..., None]) * (0.92 + 0.16 * (1 - lift[..., None]))
    flaked = (smoothstep(0.9, 0.97, rr) * smoothstep(0.15, 0.5, 1 - lift))[..., None] * 0.55
    col = col * (1 - flaked) + red * flaked
    under = (lift * 0.0 + seam * 0.55)[..., None]
    col = col * (1 - under) + dark * under
    col = col * (1 - (fissure * 0.8)[..., None]) + dark * (fissure * 0.8)[..., None]
    lichen = smoothstep(0.38, 0.62, pfbm(U, V, 4, 8, 5, seed=506)) * (1 - seam) * (1 - fissure)
    col = col * (1 - (lichen * 0.6)[..., None]) + np.array(core.hex_linear(0xa0a891)[:3]) * (lichen * 0.6)[..., None]
    rb = pcells(U, V, 5, 9, seed=507, jitter=0.9)
    resin = (1 - smoothstep(0.03, 0.06, rb[0])) * (rb[2] > 0.93)
    col = col * (1 - resin[..., None]) + np.array(core.hex_linear(0xb07a2c)[:3]) * resin[..., None]
    h += 0.0012 * resin
    nrm = tile_normal(h, 0.5 / W, 1.0 / H, 1.0)
    cav = h - tile_blur(h, 6)
    ao = np.clip(1.0 + cav / 0.0022, 0.3, 1.0)
    rough = np.clip(0.86 - 0.45 * resin - 0.06 * flaked[..., 0], 0.0, 1.0)
    orm = np.stack([ao, rough, np.zeros_like(ao)], axis=2)
    return col.astype(np.float32), nrm.astype(np.float32), orm.astype(np.float32)


def snow_maps(N=2048):
    """Fresh wind-crusted snow, a 4 m tile: long wind ripples (sastrugi) with sharp windward faces, soft swells, crust
    patches, fine grains (the sparkle's facets are in the normal). Returns (base linear, normal, orm)."""
    U, V = tile_grid(N, N)
    tile = 4.0
    # Swells (1-2 m), ripples across the wind (wind along u: crests run along v), grains.
    swell = 0.035 * pfbm(U, V, 3, 3, 3, seed=521)
    warp = 0.06 * pfbm(U, V, 4, 4, 3, seed=522)
    ph = (U + warp) * 18.0
    saw = (ph % 1.0)
    rip = np.where(saw < 0.75, saw / 0.75, (1.0 - saw) / 0.25) - 0.5          # gentle lee, steep windward face
    amp = 0.012 * smoothstep(-0.2, 0.4, pfbm(U, V, 5, 5, 3, seed=523))
    ripple = amp * rip * (0.6 + 0.4 * pfbm(U, V, 8, 4, 2, seed=524))
    fine = 0.0025 * pfbm(U, V, 40, 40, 3, seed=525)
    gx = np.floor(U * 1400).astype(np.int64)
    gy = np.floor(V * 1400).astype(np.int64)
    grains = 0.0006 * (hash01(gx % 1400, gy % 1400, 526) - 0.5)
    h = swell + ripple + fine + grains
    crust = smoothstep(0.1, 0.35, pfbm(U, V, 6, 6, 4, seed=527))
    base = np.array(core.hex_linear(0xe9eef5)[:3])
    col = base * (0.94 + 0.06 * pfbm(U, V, 12, 12, 3, seed=528)[..., None]) * (1.0 - 0.03 * crust[..., None])
    nrm = tile_normal(h, tile / N, tile / N, 1.0)
    cav = h - tile_blur(h, 10)
    ao = np.clip(1.0 + cav / 0.02, 0.6, 1.0)
    rough = 0.66 - 0.2 * crust + 0.05 * pfbm(U, V, 30, 30, 2, seed=529)
    orm = np.stack([ao, np.clip(rough, 0.0, 1.0), np.zeros_like(ao)], axis=2)
    return col.astype(np.float32), nrm.astype(np.float32), orm.astype(np.float32)


def rock_maps(N=2048):
    """Weathered granite, a 6 m tile: two joint sets of long, wandering cracks with stepped blocks between them and a
    sparse web of fine fractures; exfoliation sheets; speckled grain (quartz, feldspar, biotite); weathering stains,
    crustose lichen, frost in the cracks. Returns (base linear, normal, orm)."""
    U, V = tile_grid(N, N)
    tile = 6.0
    wa = 0.55 * pfbm(U, V, 2, 2, 4, seed=541)
    wb = 0.6 * pfbm(U, V, 2, 2, 4, seed=542)
    da, ka = _joints(U, V, 1, 1, 2, wa, 543)
    db, kb = _joints(U, V, 1, -2, 1, wb, 544)
    # The joints come and go along their length.
    ma = smoothstep(-0.25, 0.15, pfbm(U, V, 4, 4, 3, seed=545))
    mb = smoothstep(-0.15, 0.25, pfbm(U, V, 4, 4, 3, seed=546))
    ja = (1.0 - smoothstep(0.0, 0.006, da)) * ma
    jb = (1.0 - smoothstep(0.0, 0.006, db)) * mb
    joint = np.maximum(ja, jb)
    # Blocks between the joints stand at slightly different heights and tilts.
    bid = (ka.astype(np.int64) % 3) * 7 + (kb.astype(np.int64) % 2) * 3
    bh = hash01(bid, np.int64(5), 547) - 0.5
    bt = hash01(bid, np.int64(6), 547) - 0.5
    block = 0.008 * bh * ma * mb + 0.01 * bt * pfbm(U, V, 1, 1, 2, seed=565)
    edge_round = 0.018 * smoothstep(0.0, 0.035, np.minimum(da / np.maximum(ma, 0.05), db / np.maximum(mb, 0.05)))
    f1, f2, rid, du, dv = pcells(U + 0.03 * pfbm(U, V, 8, 8, 2, seed=548), V, 13, 13, seed=549, jitter=0.95)
    fine = (1.0 - smoothstep(0.0, 0.035, f2 - f1)) * smoothstep(0.25, 0.5, pfbm(U, V, 6, 6, 3, seed=550))
    # Exfoliation: thin sheets peeling off, their edges a small step.
    sheet = smoothstep(-0.02, 0.02, pfbm(U, V, 4, 4, 4, seed=551) - 0.1)
    h = 0.035 * pfbm(U, V, 2, 2, 3, seed=552) + block + edge_round + 0.006 * sheet - 0.03 * joint - 0.005 * fine
    h += 0.004 * pfbm(U, V, 16, 16, 4, seed=553) + 0.0012 * pfbm(U, V, 140, 140, 2, seed=554)
    # Grain: speckles at 2-6 mm.
    gx = np.floor(U * 1800).astype(np.int64)
    gy = np.floor(V * 1800).astype(np.int64)
    sp = hash01(gx % 1800, gy % 1800, 555)
    sp2 = hash01((gx // 2) % 900, (gy // 2) % 900, 556)
    col = np.broadcast_to(np.array(core.hex_linear(0x8d8a84)[:3]), U.shape + (3,)).copy()
    col = col * (0.82 + 0.3 * (0.5 + 0.5 * pfbm(U, V, 5, 5, 4, seed=557)))[..., None]
    # Each block between the joints weathered to its own tone.
    col = col * (0.86 + 0.28 * hash01(bid, np.int64(7), 547) * ma * mb + 0.14 * (1.0 - ma * mb))[..., None]
    biot = (sp > 0.88)[..., None]
    feld = (sp2 > 0.82)[..., None]
    col = np.where(biot, col * 0.32, col)
    col = np.where(feld & ~biot, col * 1.25 + np.array([0.02, 0.012, 0.005]), col)
    stain = smoothstep(-0.1, 0.5, pfbm(U, V, 2, 5, 5, seed=558))[..., None]
    col = col * (1 - 0.4 * stain) + np.array(core.hex_linear(0x5a5249)[:3]) * 0.4 * stain
    col = col * (1 - 0.15 * (1 - sheet))[..., None]
    # Crustose lichen with crisp, lobed edges: grey-green, a little orange, black on the steep weathered parts.
    lich = smoothstep(0.3, 0.36, pfbm(U, V, 9, 9, 6, seed=559)) * (1.0 - joint)
    lich2 = smoothstep(0.47, 0.51, pfbm(U, V, 14, 14, 6, seed=560)) * (1.0 - joint)
    lich3 = smoothstep(0.4, 0.46, pfbm(U, V, 10, 10, 6, seed=561)) * smoothstep(0.0, 0.4, pfbm(U, V, 3, 3, 2, seed=562)) * 0.8
    lich = lich * (0.6 + 0.4 * (0.5 + 0.5 * pfbm(U, V, 40, 40, 3, seed=566)))
    col = col * (1 - (0.5 * lich)[..., None]) + np.array(core.hex_linear(0x8f9b80)[:3]) * (0.5 * lich)[..., None]
    col = col * (1 - (0.8 * lich2)[..., None]) + np.array(core.hex_linear(0xa9823c)[:3]) * (0.8 * lich2)[..., None]
    col = col * (1 - (0.85 * lich3)[..., None]) + np.array(core.hex_linear(0x1e1e1c)[:3]) * (0.85 * lich3)[..., None]
    dark = np.maximum(joint, fine * 0.7)[..., None]
    col = col * (1 - 0.75 * dark) + np.array(core.hex_linear(0x1b1a19)[:3]) * 0.75 * dark
    frost = np.clip(joint * 0.9 + fine * 0.5, 0.0, 1.0) * smoothstep(-0.2, 0.3, pfbm(U, V, 6, 6, 3, seed=563))
    col = col * (1 - (0.35 * frost)[..., None]) + np.array(core.hex_linear(0xd5dde6)[:3]) * (0.35 * frost)[..., None]
    nrm = tile_normal(h, tile / N, tile / N, 1.0)
    cav = h - tile_blur(h, 8)
    ao = np.clip(1.0 + cav / 0.012, 0.3, 1.0)
    rough = np.clip(0.8 - 0.25 * frost - 0.08 * sheet + 0.05 * pfbm(U, V, 20, 20, 2, seed=564), 0.0, 1.0)
    orm = np.stack([ao, rough, np.zeros_like(ao)], axis=2)
    return col.astype(np.float32), nrm.astype(np.float32), orm.astype(np.float32)


def star_field(W=4096, H=2048):
    """The night sky, equirectangular: u = compass azimuth / 360 (0 north, 0.25 east), rows from the zenith (top) to
    the nadir (bottom); about 9,000 stars (magnitudes to 6.5, a few blue-white and orange) and a faint Milky Way
    from SSW to NNE across the zenith, with its dark rift. Linear RGB, about 1.0 for a first-magnitude star."""
    rng = np.random.default_rng(9000)
    img = np.zeros((H, W, 3), dtype=np.float32)
    # The galactic plane: a great circle through az 200 deg (SSW) and az 20 deg (NNE), passing near the zenith.
    pole_az, pole_el = math.radians(110.0), math.radians(12.0)
    pole = np.array([math.cos(pole_el) * math.sin(pole_az), math.cos(pole_el) * math.cos(pole_az), math.sin(pole_el)])
    U, V = tile_grid(W, H)
    az = U * 2 * math.pi
    el = (V - 0.5) * math.pi            # rows bottom-up: v 0 = nadir
    dirs = np.stack([np.cos(el) * np.sin(az), np.cos(el) * np.cos(az), np.sin(el)], axis=2)
    gl = np.arcsin(np.clip(dirs @ pole, -1, 1))          # galactic latitude
    band = np.exp(-(gl / math.radians(11.0)) ** 2)
    lon = np.arctan2(dirs @ np.cross(pole, [0, 0, 1]) / max(np.linalg.norm(np.cross(pole, [0, 0, 1])), 1e-9), dirs[..., 2])
    mott = 0.5 + 0.5 * pfbm(U, V, 24, 12, 6, seed=561)
    clouds = smoothstep(0.35, 0.75, 0.5 + 0.5 * pfbm(U, V, 10, 5, 5, seed=562))
    gl_r = gl - math.radians(2.2) * pfbm(U, V, 8, 4, 4, seed=563)
    rift = 1.0 - 0.55 * np.exp(-(gl_r / math.radians(2.8)) ** 2) * smoothstep(0.25, 0.75, 0.5 + 0.5 * pfbm(U, V, 6, 3, 4, seed=564))
    mw = np.exp(-(gl / math.radians(8.0)) ** 2) * (0.25 + 0.5 * mott + 0.6 * clouds) * rift * 0.0045
    img += mw[..., None] * np.array([0.92, 0.95, 1.0])
    # Stars: uniform on the sphere, denser toward the band; magnitudes from the counts' slope (N(m) ~ 10^0.45 m).
    n = 9600 + 30000
    zs = rng.uniform(-1, 1, n * 3)
    azs = rng.uniform(0, 2 * math.pi, n * 3)
    d = np.stack([np.sqrt(1 - zs ** 2) * np.sin(azs), np.sqrt(1 - zs ** 2) * np.cos(azs), zs], axis=1)
    glat = np.arcsin(np.clip(d @ pole, -1, 1))
    bright = np.arange(n * 3) < 9600 * 3
    keep = rng.uniform(0, 1, n * 3) < np.where(bright, 0.35 + 0.65 * np.exp(-(glat / math.radians(18.0)) ** 2), 0.05 + 0.95 * np.exp(-(glat / math.radians(7.0)) ** 2))
    faint = ~bright[keep][:n]
    d = d[keep][:n]
    m = 6.5 + np.log10(rng.uniform(1e-4, 1.0, len(d))) / 0.45
    m = np.clip(m, -1.4, 6.5)
    m = np.where(faint, 6.6 + 1.6 * rng.uniform(0, 1, len(d)), m)      # the band's unresolved dust of stars
    flux = 10 ** (-0.4 * m)
    bv = np.clip(rng.normal(0.6, 0.45, len(d)), -0.3, 1.8)
    t = 4600.0 * (1 / (0.92 * bv + 1.7) + 1 / (0.92 * bv + 0.62))
    r = np.clip(1.0 + 0.00012 * (6500 - t), 0.6, 1.25)
    b = np.clip(1.0 - 0.00016 * (6500 - t), 0.45, 1.3)
    g = np.full_like(r, 1.0)
    colr = np.stack([r, g, b], axis=1) / np.stack([r, g, b], axis=1).max(axis=1, keepdims=True)
    saz = np.arctan2(d[:, 0], d[:, 1]) % (2 * math.pi)
    sel = np.arcsin(d[:, 2])
    x = saz / (2 * math.pi) * W
    y = (sel / math.pi + 0.5) * H
    for k in range(len(d)):
        s = 0.55 + 0.25 * max(0.0, 2.0 - m[k])
        rad = int(math.ceil(3 * s)) + 1
        xi, yi = int(x[k]), int(y[k])
        xs = np.arange(xi - rad, xi + rad + 1)
        ys = np.arange(max(yi - rad, 0), min(yi + rad + 1, H))
        if len(ys) == 0:
            continue
        # Equirect pixels stretch toward the poles: widen the footprint in x by 1 / cos(el).
        sx = s / max(math.cos(sel[k]), 0.05)
        gx = np.exp(-0.5 * ((xs + 0.5 - x[k]) / sx) ** 2)
        gy = np.exp(-0.5 * ((ys + 0.5 - y[k]) / s) ** 2)
        w = np.outer(gy, gx)
        w = w / w.sum()
        img[ys[:, None], xs[None, :] % W] += (w[..., None] * flux[k] * 2.2) * colr[k]
    return img          # rows bottom-up (the nadir first), as the PNG writer expects


def moon_disc(N=512, lit=0.85):
    """A waxing gibbous moon (about 85 % lit), the lit limb to the right; maria and a few bright rayed craters;
    alpha = the lit disc. Linear RGBA, rows bottom-up."""
    U, V = tile_grid(N, N)
    x = (U - 0.5) * 2.0 / 0.96
    y = (V - 0.5) * 2.0 / 0.96
    r2 = x * x + y * y
    inside = r2 <= 1.0
    z = np.sqrt(np.clip(1.0 - r2, 0.0, 1.0))
    # Sun direction for the phase: illuminated fraction k = (1 + cos(phase)) / 2.
    ph = math.acos(2 * lit - 1)
    sun = np.array([math.sin(ph), 0.0, math.cos(ph)])
    lam = np.clip(x * sun[0] + y * sun[1] + z * sun[2], 0.0, 1.0)
    term = smoothstep(0.0, 0.06, lam)
    # Albedo: highlands, maria (lunar-like layout), craters.
    # The near side's maria (normalized disc coordinates, north up), their edges ragged.
    mare = np.zeros(U.shape, dtype=np.float32)
    wx = x + 0.06 * pfbm(U, V, 5, 5, 4, seed=574)
    wy = y + 0.06 * pfbm(U, V, 5, 5, 4, seed=575)
    for cx, cy, rx, ry in ((-0.30, 0.42, 0.30, 0.26), (0.18, 0.38, 0.16, 0.15), (0.30, 0.10, 0.20, 0.17), (0.66, 0.30, 0.12, 0.10),
                           (0.55, -0.12, 0.13, 0.16), (0.38, -0.27, 0.10, 0.09), (-0.20, -0.36, 0.18, 0.13), (-0.62, 0.10, 0.25, 0.42),
                           (0.02, 0.74, 0.40, 0.07), (-0.47, -0.42, 0.10, 0.09), (0.02, 0.0, 0.08, 0.10)):
        mare = np.maximum(mare, smoothstep(1.05, 0.55, np.hypot((wx - cx) / rx, (wy - cy) / ry)))
    mare = mare * (0.75 + 0.25 * pfbm(U, V, 8, 8, 4, seed=571))
    alb = 0.30 - 0.12 * mare + 0.03 * pfbm(U, V, 30, 30, 3, seed=572)
    c1, c2, crid, _, _ = pcells(U, V, 40, 40, seed=576, jitter=0.9)
    alb = alb + (1.0 - mare) * 0.03 * (smoothstep(0.08, 0.12, c1) - smoothstep(0.12, 0.2, c1)) * (crid > 0.6)
    for cx, cy, br, rr in ((-0.10, -0.72, 0.5, 0.45), (-0.25, 0.08, 0.3, 0.3), (-0.55, 0.18, 0.25, 0.2)):
        dd = np.hypot(x - cx, y - cy)
        alb = alb + br * 0.3 * np.exp(-(dd / 0.03) ** 2) + br * 0.08 * smoothstep(0.55, 0.85, pfbm(U, V, 60, 60, 2, seed=577)) * np.exp(-dd / rr)
    limb = 0.75 + 0.25 * z
    I = alb * lam ** 0.6 * limb * term
    rgb = np.stack([I * 1.0, I * 0.985, I * 0.95], axis=2)
    a = (inside & (term > 0.0)).astype(np.float32) * smoothstep(1.0, 0.985, np.sqrt(r2)) * term
    return np.concatenate([rgb / max(float(I.max()), 1e-6), a[..., None]], axis=2).astype(np.float32)


def _joints(U, V, ka, kb, count, warp, seed):
    """A family of near-parallel joints across a tile: (distance to the nearest joint in tile units, block index)."""
    s = (U * ka + V * kb) * count + warp
    k = np.floor(s + 0.5)
    d = np.abs(s - k) / (count * math.hypot(ka, kb))
    return d, k


CARD_ROWS = 6


def card_strips(alpha):
    """Per atlas cell, the strip a card is cut to: (CARD_ROWS, 3, 2) cell-normalized (u, v) for the left, middle and
    right of each row, fitted round the spray's alpha (less overdraw than a square card)."""
    W = alpha.shape[0]
    px = W // 4
    out = {}
    for i in range(16):
        u0, v0, u1, v1 = cell_rect(i)
        x0, y0 = int(round(u0 * W)), int(round(v0 * W))
        a = alpha[y0:y0 + px, x0:x0 + px] > 0.2
        rows = np.nonzero(a.any(axis=1))[0]
        if len(rows) == 0:
            continue
        vmin, vmax = rows[0], rows[-1] + 1
        pad = max(4, px // 128)
        strip = np.zeros((CARD_ROWS, 3, 2), dtype=np.float32)
        half = (vmax - vmin) / (2 * (CARD_ROWS - 1)) + pad
        for k in range(CARD_ROWS):
            vk = vmin + (vmax - vmin) * k / (CARD_ROWS - 1)
            r0, r1 = int(max(vmin, vk - half)), int(min(vmax, vk + half + 1))
            cols = np.nonzero(a[r0:r1].any(axis=0))[0]
            if len(cols) == 0:
                cols = np.array([px // 2])
            ul, ur = cols[0] - pad, cols[-1] + 1 + pad
            if ur - ul < px * 0.08:
                c = (ul + ur) / 2
                ul, ur = c - px * 0.04, c + px * 0.04
            vv = (vk + (-pad if k == 0 else (pad if k == CARD_ROWS - 1 else 0))) / px
            strip[k, 0] = (max(ul, 0) / px, vv)
            strip[k, 2] = (min(ur, px) / px, vv)
            strip[k, 1] = ((strip[k, 0, 0] + strip[k, 2, 0]) / 2, vv)
        strip[:, :, 1] = np.clip(strip[:, :, 1], 0.0, 1.0)
        out[i] = strip
    return out


class TreeBuilder:
    """Accumulates a tree: bark tubes (slot 0) and needle cards (slot 1), with corner UVs and per-vertex normals
    (cards' normals bent outward from the trunk)."""

    def __init__(self, strips):
        self.strips = strips
        self.P, self.N, self.F, self.UV, self.M = [], [], [], [], []
        self.n = 0
        self.cards = 0

    def _add(self, P, N, F, UV, mat):
        P = np.asarray(P, dtype=np.float64)
        self.P.append(P)
        self.N.append(np.asarray(N, dtype=np.float64))
        F = np.asarray(F, dtype=np.int64)
        self.F.append(F + self.n)
        self.UV.append(np.asarray(UV, dtype=np.float32))
        self.M.append(np.full(len(F), mat, dtype=np.int32))
        self.n += len(P)

    def tube(self, pts, radii, sides, u_rep=1.0, v_scale=1.0, v0=0.0, cap=False):
        """Bark along a path: rings of `sides`, u round (u_rep tiles), v along the length (meters / v_scale)."""
        pts = np.asarray(pts, dtype=np.float64)
        n = len(pts)
        t = np.gradient(pts, axis=0)
        t /= np.maximum(np.linalg.norm(t, axis=1, keepdims=True), 1e-9)
        # Parallel-transported frame.
        a = np.zeros_like(pts)
        ref = np.array([1.0, 0.0, 0.0]) if abs(t[0, 2]) > 0.9 else np.array([0.0, 0.0, 1.0])
        a0 = np.cross(t[0], ref)
        a0 /= np.linalg.norm(a0)
        a[0] = a0
        for i in range(1, n):
            v = a[i - 1] - t[i] * np.dot(a[i - 1], t[i])
            a[i] = v / max(np.linalg.norm(v), 1e-9)
        b = np.cross(t, a)
        s = np.concatenate([[0.0], np.cumsum(np.linalg.norm(np.diff(pts, axis=0), axis=1))])
        P, N, UVv = [], [], []
        for i in range(n):
            for k in range(sides + 1):
                ang = 2 * math.pi * k / sides
                dirv = math.cos(ang) * a[i] + math.sin(ang) * b[i]
                P.append(pts[i] + radii[i] * dirv)
                N.append(dirv)
                UVv.append((u_rep * k / sides, v0 + s[i] / v_scale))
        F = []
        UV = []
        for i in range(n - 1):
            for k in range(sides):
                p0 = i * (sides + 1) + k
                p1, q0, q1 = p0 + 1, p0 + sides + 1, p0 + sides + 2
                F += [(p0, p1, q1), (p0, q1, q0)]
        F = np.array(F)
        UV = np.asarray(UVv, dtype=np.float32)[F]
        self._add(P, N, F, UV, 0)
        return s[-1]

    def card(self, cell, at, d, up, scale, droop=0.0, roll=0.0, trunk_axis=(0.0, 0.0), bend_out=0.55, rows=CARD_ROWS):
        """A needle card cut to its cell's strip: its twig base at `at`, its axis along d, its face toward `up`
        (rolled by `roll` radians about d), drooping toward its tip; normals bent outward from the trunk."""
        strip = self.strips.get(cell)
        if strip is None:
            return
        if rows != CARD_ROWS:
            strip = strip[np.round(np.linspace(0, CARD_ROWS - 1, rows)).astype(int)]
        d = np.asarray(d, dtype=np.float64)
        d /= np.linalg.norm(d)
        n = np.asarray(up, dtype=np.float64) - d * np.dot(up, d)
        if np.linalg.norm(n) < 1e-6:
            n = np.cross(d, [1.0, 0.0, 0.0])
        n /= np.linalg.norm(n)
        if roll:
            side0 = np.cross(d, n)
            n = n * math.cos(roll) + side0 * math.sin(roll)
        side = np.cross(d, n)               # +u on the card; (side, d, n) right-handed: side x d = n
        S = CELL_M * scale
        u0, v0, u1, v1 = cell_rect(cell)
        P, NN, UVv = [], [], []
        at = np.asarray(at, dtype=np.float64)
        nr = len(strip)
        for k in range(nr):
            for j in range(3):
                u, v = strip[k, j]
                x = (u - 0.5) * S
                y = (v - SPRAY_BASE_V) * S
                p = at + side * x + d * y - n * droop * (y * y) / max(S, 1e-6) - n * 0.15 * droop * abs(x) * y / max(S, 1e-6)
                P.append(p)
                out = p.copy()
                out[0] -= trunk_axis[0]
                out[1] -= trunk_axis[1]
                out[2] = 0.0
                ln = np.linalg.norm(out)
                out = out / ln if ln > 1e-6 else np.zeros(3)
                nn = (1.0 - bend_out) * n + bend_out * (out + np.array([0.0, 0.0, 0.35]))
                NN.append(nn / np.linalg.norm(nn))
                UVv.append((u0 + (u1 - u0) * u, v0 + (v1 - v0) * v))
        F = []
        for k in range(nr - 1):
            for j in range(2):
                a = k * 3 + j
                b, c, e = a + 1, a + 4, a + 3
                F += [(a, b, c), (a, c, e)]
        F = np.array(F)
        UV = np.asarray(UVv, dtype=np.float32)[F]
        self._add(P, NN, F, UV, 1)
        self.cards += 1

    def obj(self, name, mats):
        P = np.concatenate(self.P)
        N = np.concatenate(self.N)
        F = np.concatenate(self.F)
        UV = np.concatenate(self.UV)
        M = np.concatenate(self.M)
        me = bpy.data.meshes.new(name)
        me.vertices.add(len(P))
        me.vertices.foreach_set('co', P.astype(np.float32).ravel())
        me.loops.add(F.size)
        me.loops.foreach_set('vertex_index', F.ravel().astype(np.int32))
        me.polygons.add(len(F))
        me.polygons.foreach_set('loop_start', (np.arange(len(F)) * 3).astype(np.int32))
        me.polygons.foreach_set('material_index', M)
        me.update(calc_edges=True)
        lay = me.uv_layers.new(name='UVMap')
        lay.data.foreach_set('uv', UV.reshape(-1, 2).ravel())
        for m in mats:
            me.materials.append(m)
        me.shade_smooth()
        me.validate(clean_customdata=False)
        Nn = N / np.maximum(np.linalg.norm(N, axis=1, keepdims=True), 1e-9)
        me.normals_split_custom_set_from_vertices([tuple(v) for v in Nn])
        o = bpy.data.objects.new(name, me)
        core.link(o)
        return o


def _limb_path(base, azim, length, pitch0, droop, upturn, n=7):
    """A limb from the trunk: out along azim, rising at pitch0, sagging (droop) and turning up at its tip (upturn)."""
    s = np.linspace(0.0, 1.0, n)
    h = np.array([math.cos(azim), math.sin(azim), 0.0])
    z = length * (math.tan(pitch0) * s - droop * s * s + upturn * np.maximum(s - 0.65, 0.0) ** 2 * 3.0)
    pts = np.asarray(base)[None, :] + h[None, :] * (length * s)[:, None]
    pts[:, 2] += z
    return pts


def build_spruce(name, strips, H, R0, hc, whorl, scale, snow, seed, young=False, mats=None):
    """An Engelmann spruce: a narrow spire of whorled limbs that droop low and lift high; each limb carries a broad
    plate of foliage on side branchlets (sprays all along them, snow-laden on the upper faces), a hanging under-layer,
    snow pillows riding its inner half, dead twigs low by the trunk; a leader and cones at the top."""
    rng = random.Random(seed)
    tb = TreeBuilder(strips)
    base_r = 0.011 * H + 0.02
    zs = np.concatenate([np.linspace(0.0, 0.6, 5), np.linspace(0.8, H - 0.2, max(6, int(H / 0.6)))])
    lean = (rng.random() - 0.5) * 0.02 * H
    pts = np.column_stack([lean * (zs / H) ** 2, 0.3 * lean * (zs / H) ** 2, zs])
    rad = base_r * (1.0 - zs / H) ** 0.9 + 0.006
    rad = rad * (1.0 + 0.5 * np.exp(-zs / 0.3))
    tb.tube(pts, rad, 12 if not young else 8, u_rep=3.0 if not young else 2.0, v_scale=1.0)
    snow_cells = [3, 4, 5]
    frost_cells = [0, 1, 2] if not young else [11, 11, 0]

    def trunk_xy(z):
        return lean * (z / H) ** 2, 0.3 * lean * (z / H) ** 2

    def radius_at(h):
        t = np.clip((H - h) / (H - hc), 0.0, 1.0)
        return R0 * t ** (0.85 if not young else 0.95) * (0.9 + 0.2 * rng.random())

    phase = rng.random() * 6.28
    h = hc
    while h < H - 0.3:
        t_up = (h - hc) / (H - hc)
        nl = 5 if t_up < 0.5 else (4 if t_up < 0.82 else 3)
        phase += 0.6 + 0.5 * rng.random()
        for j in range(nl):
            az = phase + 2 * math.pi * j / nl + (rng.random() - 0.5) * 0.45
            _spruce_limb(tb, rng, h + (rng.random() - 0.5) * 0.05, az, radius_at(h), t_up, scale, snow, snow_cells, frost_cells, trunk_xy, young)
        if rng.random() < (0.5 if not young else 0.8):
            hh = h + whorl * 0.5
            _spruce_limb(tb, rng, hh, phase + rng.random() * 6.28, radius_at(hh) * 0.7, (hh - hc) / (H - hc), scale * 0.85, snow,
                         snow_cells, frost_cells, trunk_xy, young)
        h += whorl * (0.85 + 0.3 * rng.random()) * (1.0 - 0.4 * t_up)
    tx, ty = trunk_xy(H)
    for a in (0.0, math.pi / 2):
        tb.card(13 if not young else 11, (tx, ty, H - 0.72 * scale), (0.0, 0.0, 1.0), (math.cos(a), math.sin(a), 0.0), scale, 0.0,
                trunk_axis=(tx, ty), bend_out=0.3)
    if not young:
        for c in range(10):
            z = H - 0.6 - rng.random() * 2.4
            az = rng.random() * 6.28
            r = radius_at(z) * (0.4 + 0.4 * rng.random())
            cx, cy = trunk_xy(z)
            tb.card(15, (cx + r * math.cos(az), cy + r * math.sin(az), z), (math.cos(az) * 0.3, math.sin(az) * 0.3, -1.0),
                    (math.cos(az), math.sin(az), 0.2), scale * 0.75, 0.0, trunk_axis=(cx, cy))
    return tb.obj(name, mats)


def _spruce_limb(tb, rng, h, az, L, t_up, scale, snow, snow_cells, frost_cells, trunk_xy, young):
    cx, cy = trunk_xy(h)
    pitch = math.radians(-20.0 + 40.0 * t_up + (rng.random() - 0.5) * 8.0)
    pts = _limb_path((cx, cy, h), az, L, pitch, 0.2 - 0.12 * t_up, 0.25 * (1.0 - t_up))
    tb.tube(pts, np.linspace(0.008 + 0.011 * L, 0.003, len(pts)), 5 if L > 0.7 else 4, u_rep=1.0, v_scale=1.0)
    seg = np.linalg.norm(np.diff(pts, axis=0), axis=1)
    sl = np.concatenate([[0.0], np.cumsum(seg)])
    total = sl[-1]
    rows = 4
    k_snow = snow if (t_up > 0.08 or young) else snow * 0.4

    def spray(p, d, sc, f, under=False):
        heavy = (not under) and k_snow > 0.0 and rng.random() < k_snow * (0.35 + 0.7 * f)
        cell = rng.choice(snow_cells) if heavy else rng.choice(frost_cells)
        roll = (rng.random() - 0.5) * 0.7
        if not heavy and rng.random() < 0.22:
            roll = (1.0 if rng.random() < 0.5 else -1.0) * (0.8 + 0.5 * rng.random())     # a spray turned on edge
        tb.card(cell, p, d, (0.0, 0.0, 1.0), sc, droop=0.12 + 0.12 * rng.random() + (0.15 if under else 0.0), roll=roll,
                trunk_axis=(cx, cy), rows=rows)

    step = (0.25 if not young else 0.125) * scale
    s = total * (0.1 if not young else 0.06)
    side = 1 if rng.random() < 0.5 else -1
    while s < total - 0.04:
        p = np.array([np.interp(s, sl, pts[:, c]) for c in range(3)])
        i = min(int(np.searchsorted(sl, s)), len(pts) - 1)
        tng = pts[i] - pts[max(i - 1, 0)]
        tng /= np.linalg.norm(tng)
        f = s / total
        # A side branchlet: longer near the trunk, the plate's breadth.
        ang = math.radians(48.0 + 18.0 * rng.random()) * side
        bd = _rot_z(tng, ang)
        bd[2] = tng[2] - 0.08 - 0.08 * rng.random()
        bd /= np.linalg.norm(bd)
        Lb = (0.12 + 0.42 * (total - s) * (0.7 + 0.4 * rng.random())) * (1.0 if not young else 0.8)
        Lb = min(Lb, 1.4 * scale)
        cstep = 0.25 * scale
        t = 0.0
        while True:
            q = p + bd * t
            fq = min(1.0, (s + t * 0.5) / total)
            d = _rot_z(bd, (rng.random() - 0.5) * 0.5)
            d[2] -= 0.05 + 0.1 * rng.random()
            sc = scale * (0.82 + 0.44 * fq) * (0.9 + 0.2 * rng.random())
            spray(q, d, sc, fq)
            if not young and rng.random() < 0.2:
                d2 = d.copy()
                d2[2] -= 0.5
                spray(q - np.array([0.0, 0.0, 0.04]), d2, sc * 0.85, fq, under=True)
            t += cstep
            if t > Lb:
                break
        side = -side
        s += step
    # The tip: a broad spray along the limb.
    tip = pts[-1]
    tng = pts[-1] - pts[-2]
    tng /= np.linalg.norm(tng)
    cell = 14 if (not young and L > 1.0) else rng.choice(frost_cells)
    tb.card(cell, tip - tng * 0.06, tng + np.array([0.0, 0.0, 0.05]), (0.0, 0.0, 1.0), scale * (1.0 + 0.2 * rng.random()), droop=0.15,
            trunk_axis=(cx, cy), rows=rows)
    # Broad fans hung under the limb's inner two thirds: they fill the crown's inside, so it reads dense and dark
    # rather than lacy (and hide the bare limbs between the sprays).
    if not young and L > 0.6:
        for f in (0.28, 0.6):
            i = int(f * (len(pts) - 1))
            tng = pts[min(i + 1, len(pts) - 1)] - pts[i]
            tng /= np.linalg.norm(tng)
            tb.card(14, pts[i] - np.array([0.0, 0.0, 0.06]) - tng * 0.1, tng + np.array([0.0, 0.0, -0.12]), (0.0, 0.0, 1.0),
                    scale * (0.9 + 0.5 * f) * min(1.0, 0.45 + 0.35 * L), droop=0.2, roll=(rng.random() - 0.5) * 0.5, trunk_axis=(cx, cy), rows=4)
    # Snow pillows riding the limb's inner half; dead twigs low by the trunk.
    if k_snow > 0.0 and L > 0.45:
        for k in range(1 if L < 1.5 else 2):
            if rng.random() > k_snow:
                continue
            f = 0.12 + 0.3 * k + 0.15 * rng.random()
            p = pts[int(f * (len(pts) - 1))] + np.array([0.0, 0.0, 0.04])
            tng = pts[-1] - pts[0]
            tng /= np.linalg.norm(tng)
            tb.card(9 + rng.randrange(2), p, tng, (0.0, 0.0, 1.0), scale * min(1.15, 0.5 + 0.45 * L), droop=0.05, trunk_axis=(cx, cy),
                    bend_out=0.2, rows=4)
    if not young and t_up < 0.35 and rng.random() < 0.75:
        tng = pts[2] - pts[0]
        tng /= np.linalg.norm(tng)
        tb.card(12, pts[0] + tng * 0.05, tng + np.array([0.0, 0.0, -0.25]), (0.0, 0.0, 1.0), scale * 0.9, droop=0.1, roll=(rng.random() - 0.5),
                trunk_axis=(cx, cy), rows=4)


def build_pine(name, strips, H, mats, seed=1707):
    """A lodgepole pine: a tall straight trunk, bare (a few dead stubs) to about two thirds of its height, a narrow
    crown of short upswept limbs, each ending in clumps of long-needled tufts (cards turned every way round the
    branch ends, so the clumps read from every side)."""
    rng = random.Random(seed)
    tb = TreeBuilder(strips)
    zs = np.concatenate([np.linspace(0.0, 0.5, 4), np.linspace(0.8, H - 0.1, int(H / 0.55))])
    pts = np.column_stack([0.06 * np.sin(zs / 3.0), 0.04 * np.sin(zs / 4.2 + 1.0), zs])
    rad = 0.17 * (1.0 - zs / H) ** 0.75 + 0.012
    rad = rad * (1.0 + 0.35 * np.exp(-zs / 0.3))
    tb.tube(pts, rad, 12, u_rep=3.0, v_scale=1.0)

    def axis(z):
        return 0.06 * math.sin(z / 3.0), 0.04 * math.sin(z / 4.2 + 1.0)
    for k in range(16):
        z = 1.8 + rng.random() * 9.0
        az = rng.random() * 6.28
        cx, cy = axis(z)
        Ls = 0.12 + 0.35 * rng.random()
        p = _limb_path((cx, cy, z), az, Ls, math.radians(-10 + 20 * rng.random()), 0.05, 0.0, 3)
        tb.tube(p, np.linspace(0.022, 0.007, 3), 4)

    def clump(e, dv, cx, cy, size):
        dv = dv / np.linalg.norm(dv)
        a0, b0 = _frame_vecs(dv[None, :])
        for c in range(9):
            a = 2 * math.pi * c / 9 + rng.random() * 0.6
            tilt = 0.35 + 0.5 * rng.random()
            dd = dv * (1.0 - tilt) + tilt * (math.cos(a) * a0[0] + math.sin(a) * b0[0]) + np.array([0.0, 0.0, 0.25])
            facing = math.cos(a + 1.57) * a0[0] + math.sin(a + 1.57) * b0[0] + np.array([0.0, 0.0, 0.5])
            cell = 8 if rng.random() < 0.3 else rng.choice([6, 7])
            tb.card(cell, e - dv * 0.15 * size, dd, facing, 0.8 * size * (0.85 + 0.3 * rng.random()), droop=0.06, trunk_axis=(cx, cy), bend_out=0.5, rows=4)

    crown0 = H * 0.63
    h = crown0
    phase = 0.0
    while h < H - 0.25:
        t_up = (h - crown0) / (H - crown0)
        R = 1.1 * (1.0 - t_up) ** 0.9 * (0.75 + 0.5 * rng.random()) + 0.2
        nl = 5 if t_up < 0.6 else 4
        phase += 1.1 + 0.6 * rng.random()
        for j in range(nl):
            az = phase + 2 * math.pi * j / nl + (rng.random() - 0.5) * 0.6
            cx, cy = axis(h)
            pitch = math.radians(15.0 + 28.0 * t_up + 12.0 * rng.random())
            lp = _limb_path((cx, cy, h), az, R, pitch, 0.14, 0.18)
            tb.tube(lp, np.linspace(0.025 + 0.012 * R, 0.007, len(lp)), 5)
            ends = [(lp[-1], lp[-1] - lp[-2], 1.0)]
            for k in range(3 if R > 1.0 else 2):
                f = 0.4 + 0.2 * k + 0.08 * rng.random()
                q = lp[int(f * (len(lp) - 1))]
                a2 = az + (0.9 if k % 2 == 0 else -0.9) + (rng.random() - 0.5) * 0.4
                sub = _limb_path(q, a2, 0.3 + 0.35 * R * (0.6 + 0.4 * rng.random()), pitch + 0.35, 0.1, 0.12, 4)
                tb.tube(sub, np.linspace(0.013, 0.005, len(sub)), 4)
                ends.append((sub[-1], sub[-1] - sub[-2], 0.85))
            for e, dv, sz in ends:
                clump(e, dv, cx, cy, sz)
                # Tufts back along the last stretch, too.
                dvn = dv / np.linalg.norm(dv)
                for c in range(2):
                    q = e - dvn * (0.3 + 0.18 * c)
                    a = rng.random() * 6.28
                    dd = dvn * 0.5 + np.array([math.cos(a), math.sin(a), 0.6])
                    tb.card(rng.choice([6, 7]), q, dd, (math.cos(a + 1.5), math.sin(a + 1.5), 0.6), 0.75 + 0.2 * rng.random(), droop=0.08,
                            trunk_axis=(cx, cy), rows=4)
        h += 0.42 * (0.85 + 0.3 * rng.random())
    tx, ty = axis(H)
    clump(np.array([tx, ty, H]), np.array([0.0, 0.0, 1.0]), tx, ty, 0.9)
    return tb.obj(name, mats)


def tree_materials(textured=False, tex=None):
    """ecx_bark and ecx_needles: plain for export (the game puts M_ECX_Bark and M_ECX_Conifer on them by name);
    textured with the atlas for the review."""
    bark = core.Mat('ecx_bark' if not textured else 'ecx_bark_review')
    needles = core.Mat('ecx_needles' if not textured else 'ecx_needles_review')
    if not textured:
        bark.set('Base Color', core.hex_linear(0x5f544c))
        bark.set('Roughness', 0.85)
        bark.m.use_backface_culling = True
        needles.set('Base Color', core.hex_linear(0x2f4a3e))
        needles.set('Roughness', 0.5)
        needles.m.use_backface_culling = False
        return bark.m, needles.m
    b = bark.image(tex['bark_c'])
    bark.link(b.outputs['Color'], bark.bsdf.inputs['Base Color'])
    bo = bark.image(tex['bark_o'], non_color=True)
    sep = bark.node('ShaderNodeSeparateColor')
    bark.link(bo.outputs['Color'], sep.inputs['Color'])
    bark.link(sep.outputs['Green'], bark.bsdf.inputs['Roughness'])
    bn = bark.image(tex['bark_n'], non_color=True)
    nm = bark.node('ShaderNodeNormalMap')
    bark.link(bn.outputs['Color'], nm.inputs['Color'])
    bark.link(nm.outputs['Normal'], bark.bsdf.inputs['Normal'])
    c = needles.image(tex['needle_c'])
    needles.link(c.outputs['Color'], needles.bsdf.inputs['Base Color'])
    needles.link(c.outputs['Alpha'], needles.bsdf.inputs['Alpha'])
    n = needles.image(tex['needle_n'], non_color=True)
    nm = needles.node('ShaderNodeNormalMap')
    needles.link(n.outputs['Color'], nm.inputs['Color'])
    needles.link(nm.outputs['Normal'], needles.bsdf.inputs['Normal'])
    mk = needles.image(tex['needle_m'], non_color=True)
    sp = needles.node('ShaderNodeSeparateColor')
    needles.link(mk.outputs['Color'], sp.inputs['Color'])
    needles.link(sp.outputs['Green'], needles.bsdf.inputs['Roughness'])
    needles.set('Subsurface Weight', 0.25)
    needles.set('Subsurface Radius', (0.01, 0.02, 0.01))
    try:
        needles.m.surface_render_method = 'DITHERED'
    except Exception:
        pass
    return bark.m, needles.m


def tex_path(name):
    return os.path.join(TEX_OUT, name + '.png')


def write_textures():
    """Writes every T_ECX_* this module owns (spec 3.4) and returns the needle atlas's card strips. With ECX_REUSE_TEX=1
    an existing atlas is read back instead of rendered (development only)."""
    t0 = time.time()
    os.makedirs(TEX_OUT, exist_ok=True)
    reuse = os.environ.get('ECX_REUSE_TEX') and os.path.exists(tex_path('T_ECX_Needles_BaseColor'))
    if reuse:
        img = bpy.data.images.load(tex_path('T_ECX_Needles_BaseColor'), check_existing=False)
        w, h = img.size
        a = np.empty(w * h * 4, dtype=np.float32)
        img.pixels.foreach_get(a)
        bpy.data.images.remove(img)
        alpha = a.reshape(h, w, 4)[:, :, 3]
    else:
        A = render_needle_atlas()
        save_png(tex_path('T_ECX_Needles_BaseColor'), A['col'], srgb=True)
        save_png(tex_path('T_ECX_Needles_Normal'), A['nrm'])
        save_png(tex_path('T_ECX_Needles_Mask'), A['msk'])
        alpha = A['col'][:, :, 3]
        del A
        q = 4 if FAST else 1
        for name, fn, args in (('Bark', bark_maps, (1024 // q, 2048 // q)), ('Snow', snow_maps, (2048 // q,)), ('Rock', rock_maps, (2048 // q,))):
            c, n, o = fn(*args)
            save_png(tex_path(f'T_ECX_{name}_BaseColor'), c, srgb=True)
            save_png(tex_path(f'T_ECX_{name}_Normal'), n)
            save_png(tex_path(f'T_ECX_{name}_ORM'), o)
        save_png(tex_path('T_ECX_StarField'), star_field(4096 // q, 2048 // q))
        save_png(tex_path('T_ECX_ForestMask'), forest_mask(2048 // q))
        save_png(tex_path('T_ECX_Moon'), moon_disc(512 // q), srgb=True)
        log(f'textures written in {time.time() - t0:.0f}s')
    return card_strips(alpha)


def review_tex():
    return {'bark_c': tex_path('T_ECX_Bark_BaseColor'), 'bark_o': tex_path('T_ECX_Bark_ORM'), 'bark_n': tex_path('T_ECX_Bark_Normal'),
            'needle_c': tex_path('T_ECX_Needles_BaseColor'), 'needle_n': tex_path('T_ECX_Needles_Normal'), 'needle_m': tex_path('T_ECX_Needles_Mask')}


def _rot_z(v, ang):
    c, s = math.cos(ang), math.sin(ang)
    return np.array([v[0] * c - v[1] * s, v[0] * s + v[1] * c, v[2]])


def _rotz3(p, ang):
    c, s = math.cos(ang), math.sin(ang)
    return np.stack([p[:, 0] * c + p[:, 1] * s, -p[:, 0] * s + p[:, 1] * c, p[:, 2]], axis=1)


def sd_box(p, c, half, rot, r):
    """Signed distance to a box with rounded edges (half extents, rotated by rot about Z)."""
    q = np.abs(_rotz3(p - np.asarray(c), rot)) - (np.asarray(half) - r)
    out = np.linalg.norm(np.maximum(q, 0.0), axis=1) + np.minimum(np.max(q, axis=1), 0.0)
    return out - r


def sd_ellipsoid(p, c, rad):
    q = (p - np.asarray(c)) / np.asarray(rad)
    k0 = np.linalg.norm(q, axis=1)
    k1 = np.linalg.norm(q / np.asarray(rad), axis=1)
    return k0 * (k0 - 1.0) / np.maximum(k1, 1e-9)


def smin(a, b, k):
    h = np.clip(0.5 + 0.5 * (b - a) / k, 0.0, 1.0)
    return b * (1 - h) + a * h - k * h * (1 - h)


def noise3(p, wavelength, octaves, seed, kind='fbm'):
    """3D-ish noise: the mean of three axis-plane projections of 2D noise."""
    f = fbm if kind == 'fbm' else ridged
    return (f(p[:, 0], p[:, 1], wavelength, octaves, seed=seed) + f(p[:, 1], p[:, 2], wavelength, octaves, seed=seed + 1) +
            f(p[:, 2], p[:, 0], wavelength, octaves, seed=seed + 2)) / 3.0


class RockShape:
    """A rock as a signed distance: faceted blocks joined smoothly, grooved by joint sets, eroded and roughened at
    several scales."""

    def __init__(self, seed, blocks, joints, rough, crack_depth=0.12, erode=0.0, edge=0.12):
        self.seed = seed
        self.edge = edge              # how much the blocks' facets are rounded where they meet (m)
        self.blocks = blocks          # (center, radii, normals list)
        self.joints = joints          # (normal, spacing, width, depth)
        self.rough = rough
        self.crack_depth = crack_depth
        self.erode = erode

    def __call__(self, p):
        d = None
        for c, radii, normals in self.blocks:
            db = sd_polytope(p, c, radii, normals, k=self.edge)
            d = db if d is None else smin(d, db, 0.25)
        warp = 0.3 * noise3(p, 3.0, 2, self.seed + 7)
        for n, spacing, width, depth in self.joints:
            n = np.asarray(n, dtype=np.float64)
            n = n / np.linalg.norm(n)
            t = (p @ n) / spacing + warp
            dist = np.abs(t - np.round(t)) * spacing
            vis = smoothstep(-0.25, 0.2, noise3(p, 2.5, 2, self.seed + int(spacing * 13)))
            d = d + depth * (1.0 - smoothstep(0.0, width, dist)) * vis
        d = d + self.erode * noise3(p, 4.0, 3, self.seed + 2)
        d = d + self.rough * (0.6 * noise3(p, 1.6, 4, self.seed + 3) + 0.4 * noise3(p, 0.4, 3, self.seed + 4))
        d = d + self.crack_depth * smoothstep(0.6, 0.85, noise3(p, 0.8, 3, self.seed + 5, 'ridged'))
        return d


def cube_sphere_rock(name, shape, center, rmax, N, ground=-0.4, snow=0.6, snow_depth=0.18, fit=None):
    """Shrink-wraps a cube-sphere (N x N cells a face, equal-angle) onto `shape` by bisection along rays from
    `center`; flattens it under `ground`; lays snow on the upward ledges (a smoothed mask, raised, its own slot).
    Returns (vertices, faces, corner UVs, snow per face, snow per vertex), the cube's seams welded."""
    t0 = time.time()
    a = np.tan(np.linspace(-1.0, 1.0, N + 1) * math.pi / 4)
    A, B = np.meshgrid(a, a)
    faces_dirs = []
    for axis in range(3):
        for sign in (1.0, -1.0):
            P = np.zeros((N + 1, N + 1, 3))
            P[:, :, axis] = sign
            o1, o2 = [k for k in range(3) if k != axis]
            if sign > 0:
                P[:, :, o1], P[:, :, o2] = A, B
            else:
                P[:, :, o1], P[:, :, o2] = B, A
            faces_dirs.append(P.reshape(-1, 3))
    D = np.concatenate(faces_dirs)
    D /= np.linalg.norm(D, axis=1, keepdims=True)
    c = np.asarray(center, dtype=np.float64)
    lo = np.zeros(len(D))
    hi = np.full(len(D), rmax)
    for _ in range(26):
        mid = 0.5 * (lo + hi)
        inside = shape(c + D * mid[:, None]) < 0.0
        lo = np.where(inside, mid, lo)
        hi = np.where(inside, hi, mid)
    P = c + D * (0.5 * (lo + hi))[:, None]
    below = P[:, 2] < ground
    P[below, 2] = ground + (P[below, 2] - ground) * 0.15
    F, UV = [], []
    n1 = N + 1
    m = 0.012
    ii, jj = np.meshgrid(np.arange(N), np.arange(N))
    ii, jj = ii.ravel(), jj.ravel()
    for fi in range(6):
        base = fi * n1 * n1
        cu, cv = fi % 3, fi // 3
        v00 = base + jj * n1 + ii
        v10, v01, v11 = v00 + 1, v00 + n1, v00 + n1 + 1

        def uv(i, j):
            return np.stack([(cu + m + (1 - 2 * m) * i / N) / 3.0, (cv + m + (1 - 2 * m) * j / N) / 2.0], axis=1)
        F.append(np.stack([np.stack([v00, v10, v11], 1), np.stack([v00, v11, v01], 1)], 1).reshape(-1, 3))
        UV.append(np.stack([np.stack([uv(ii, jj), uv(ii + 1, jj), uv(ii + 1, jj + 1)], 1),
                            np.stack([uv(ii, jj), uv(ii + 1, jj + 1), uv(ii, jj + 1)], 1)], 1).reshape(-1, 3, 2))
    F = np.concatenate(F).astype(np.int64)
    UV = np.concatenate(UV).astype(np.float32)
    fn = np.cross(P[F[:, 1]] - P[F[:, 0]], P[F[:, 2]] - P[F[:, 0]])
    wrong = np.einsum('ij,ij->i', fn, D[F[:, 0]] + D[F[:, 1]] + D[F[:, 2]]) < 0
    F[wrong] = F[wrong][:, ::-1]
    UV[wrong] = UV[wrong][:, ::-1]
    P, F, UV = weld(P, F, UV)
    if fit is not None:
        # Fit the footprint and the height above ground to the contract's size (X, Y, Z), about the center.
        lo, hi = P.min(axis=0), P.max(axis=0)
        cx, cy = (lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2
        P[:, 0] = (P[:, 0] - cx) * fit[0] / (hi[0] - lo[0])
        P[:, 1] = (P[:, 1] - cy) * fit[1] / (hi[1] - lo[1])
        P[:, 2] = ground + (P[:, 2] - ground) * (fit[2] + ground - snow_depth * 0.5) / (hi[2] - ground)
    VN = vertex_normals(P, F)
    n = noise3(P, 1.2, 3, 991)
    sv = smoothstep(0.6, 0.85, VN[:, 2] + 0.2 * n) * smoothstep(ground + 0.1, ground + 0.6, P[:, 2]) * snow
    sv = np.clip(_smooth_vertex_values(F, sv, len(P), 8) * 1.4 - 0.15, 0.0, 1.0)
    # The snow's edge runs along the mask's contour (cut through the triangles), the snow thickening inward from it.
    P, F, UV, sv, fs = split_by_contour(P, F, UV, sv, 0.42)
    # A pillow, not a slab: how deep inside the snow each vertex lies (the snow faces' mask spread over the mesh),
    # a rounded bull-nose at the edge (the thickness climbs as a square root), domed toward the middle, pushed out
    # along the surface's normal as much as up so the edge rolls over the rock instead of standing as a wall.
    on = np.zeros(len(P))
    on[np.unique(F[fs])] = 1.0
    deep = _smooth_vertex_values(F, on, len(P), 12)
    inside = np.clip((deep - 0.5) / 0.45, 0.0, 1.0)
    rise = snow_depth * (0.12 + 0.88 * np.sqrt(inside)) * (0.75 + 0.45 * noise3(P, 2.0, 2, 993)) * (sv >= 0.42)
    VN2 = vertex_normals(P, F)
    push = 0.45 * VN2 + np.array([0.0, 0.0, 0.55])
    push /= np.maximum(np.linalg.norm(push, axis=1, keepdims=True), 1e-9)
    P = P + push * rise[:, None]
    log(f'{name}: cube-sphere rock {len(F)} triangles in {time.time() - t0:.1f}s, snow on {fs.mean() * 100:.0f}% of faces')
    return P, F, UV, fs, sv


def rock_materials(tag, scale=1.0):
    """Granite and its snow. The granite: a light warm grey, mottled at two scales with buff iron staining; salt and
    pepper crystals (black biotite, white feldspar) that read within a few meters; dark water streaks down the steep
    faces and black lichen on them; pale green-grey and orange crustose lichen on the tops; darker crevices; rime only
    in the tightest cracks (no broad white smears, no crazed-mud networks: granite breaks along its joints, which the
    geometry carries). scale: the rock's size relative to Rock_A (its patterns scale with it)."""
    m = core.Mat(f'ecx_rock_{tag}')
    tc, sep = object_coords(m)
    o = tc.outputs['Object']

    def at(k, off):
        return scaled(m, sep, k, k, k, offset=off)
    geo = m.node('ShaderNodeNewGeometry')
    gsep = m.node('ShaderNodeSeparateXYZ')
    m.link(geo.outputs['Normal'], gsep.inputs['Vector'])
    top = ramp(m, gsep.outputs['Z'], 0.25, 0.8)
    side = m.math('SUBTRACT', 1.0, ramp(m, m.math('ABSOLUTE', gsep.outputs['Z']), 0.35, 0.75))
    # Occlusion over a reach several triangles wide: the joints and real crevices darken a little, not every lump of
    # the weathered surface nor the creases between its facets (the ORM's occlusion and Lumen do the rest).
    ao = m.node('ShaderNodeAmbientOcclusion', Distance=0.2 * scale)
    ao.samples = 16
    cavity = m.math('SUBTRACT', 1.0, ao.outputs['AO'])
    mott = snoise(m, o, scale=0.45 / scale, detail=4.0)
    mott2 = snoise(m, at(1.0, (5.3, 1.1, 7.9)), scale=1.8 / scale, detail=3.0)
    stain = ramp(m, snoise(m, at(1.0, (2.2, 9.4, 4.1)), scale=0.9 / scale, detail=4.0), 0.52, 0.68)
    # Crystals: millimeters to a centimeter or two (independent of the rock's size).
    biot = ramp(m, snoise(m, at(1.0, (0.7, 3.3, 8.1)), scale=230.0, detail=1.5), 0.62, 0.67)
    feld = ramp(m, snoise(m, at(1.0, (6.1, 2.9, 0.4)), scale=120.0, detail=1.5), 0.63, 0.69)
    patina = ramp(m, snoise(m, at(1.0, (7.7, 5.5, 3.3)), scale=1.1 / scale, detail=5.0), 0.42, 0.66)
    grain = snoise(m, o, scale=260.0, detail=2.0)
    # Water streaks: noise drawn out down the fall line, on the steep faces, under the tops.
    streak = m.math('MULTIPLY', ramp(m, snoise(m, scaled(m, sep, 4.0 / scale, 4.0 / scale, 0.35 / scale), detail=3.0), 0.5, 0.72), side)
    # Lichen: crustose patches with lobed, fractal edges (thresholded rough noise), on the tops and the sunny sides.
    spots = ramp(m, snoise(m, at(1.0, (3.1, 4.7, 1.9)), scale=5.0 / scale, detail=8.0, roughness=0.68), 0.6, 0.635)
    where = ramp(m, snoise(m, at(1.0, (8.8, 0.3, 2.6)), scale=0.8 / scale, detail=3.0), 0.36, 0.56)
    lich = m.math('MULTIPLY', m.math('MULTIPLY', spots, where), m.math('ADD', top, m.math('MULTIPLY', side, 0.35)))
    orange = m.math('MULTIPLY', lich, ramp(m, snoise(m, at(1.0, (1.4, 6.6, 9.2)), scale=1.6 / scale, detail=2.0), 0.6, 0.66))
    black = m.math('MULTIPLY', m.math('MULTIPLY', side, ramp(m, snoise(m, scaled(m, sep, 2.6 / scale, 2.6 / scale, 0.7 / scale, offset=(4.4, 4.4, 0.8)),
                                                                      detail=4.0), 0.6, 0.72)), 0.5)
    col = m.mix(mott, core.hex_linear(0x6c6862), core.hex_linear(0x9a958b))
    col = m.mix(m.math('MULTIPLY', ramp(m, mott2, 0.4, 0.7), 0.5), col, core.hex_linear(0x5f5c57))
    col = m.mix(m.math('MULTIPLY', patina, 0.45), col, core.hex_linear(0x56524c))
    col = m.mix(m.math('MULTIPLY', stain, 0.45), col, core.hex_linear(0x9a8470))
    col = m.mix(m.math('MULTIPLY', streak, 0.65), col, core.hex_linear(0x403d38))
    col = m.mix(black, col, core.hex_linear(0x262523))
    col = m.mix(m.math('MULTIPLY', feld, 0.22), col, core.hex_linear(0xcfcbc2))
    col = m.mix(m.math('MULTIPLY', biot, 0.4), col, core.hex_linear(0x262523))
    col = m.mix(m.math('MULTIPLY', lich, 0.85), col, core.hex_linear(0x949e7e))
    col = m.mix(m.math('MULTIPLY', m.math('MULTIPLY', lich, ramp(m, snoise(m, at(1.0, (9.9, 1.2, 3.4)), scale=2.4 / scale, detail=2.0), 0.55, 0.62)), 0.8),
                col, core.hex_linear(0x9d9b66))
    col = m.mix(m.math('MULTIPLY', orange, 0.85), col, core.hex_linear(0xb8732c))
    col = m.mix(m.math('MULTIPLY', ramp(m, cavity, 0.4, 0.85), 0.3), col, core.hex_linear(0x2a2724))
    # Rime: only the tightest, upward-facing cracks.
    rime = m.math('MULTIPLY', m.math('MULTIPLY', ramp(m, cavity, 0.75, 0.92), ramp(m, gsep.outputs['Z'], 0.0, 0.5)),
                  ramp(m, snoise(m, o, scale=2.0 / scale, detail=3.0), 0.55, 0.65))
    col = m.mix(m.math('MULTIPLY', rime, 0.3), col, core.hex_linear(0xd3dbe4))
    m.set('Base Color', col)
    rough = m.math('ADD', 0.76, m.math('MULTIPLY', grain, 0.08))
    rough = m.math('ADD', rough, m.math('MULTIPLY', lich, 0.08))
    m.set('Roughness', m.math('SUBTRACT', rough, m.math('MULTIPLY', m.math('ADD', rime, m.math('MULTIPLY', feld, 0.25)), 0.25)))
    # Relief: crystal grain, pitting, a mid-scale bumpiness; lichen crusts stand a hair proud.
    pits = ramp(m, snoise(m, at(1.0, (2.0, 2.0, 2.0)), scale=40.0, detail=2.0), 0.62, 0.7)
    h = m.math('ADD', m.math('MULTIPLY', grain, 0.35), m.math('MULTIPLY', biot, -0.25))
    h = m.math('SUBTRACT', h, m.math('MULTIPLY', pits, 0.35))
    h = m.math('ADD', h, m.math('MULTIPLY', snoise(m, o, scale=6.0 / scale, detail=5.0), 1.6))
    h = m.math('ADD', h, m.math('MULTIPLY', lich, 0.15))
    h = m.math('ADD', h, m.math('MULTIPLY', snoise(m, o, scale=22.0, detail=3.0), 0.5))
    bump = m.node('ShaderNodeBump', Strength=0.6, Distance=0.01 * scale)
    m.link(h, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    s = core.Mat(f'ecx_rock_snow_{tag}')
    tc, sep = object_coords(s)
    o = tc.outputs['Object']
    rip = snoise(s, scaled(s, sep, 0.8, 4.0, 4.0), detail=3.0)
    grains = snoise(s, o, scale=400.0, detail=1.0)
    s.set('Base Color', s.mix(s.math('MULTIPLY', rip, 0.08), core.hex_linear(0xe8edf5), core.hex_linear(0xc9d4e3)))
    s.set('Roughness', 0.62)
    b2 = s.node('ShaderNodeBump', Strength=0.25, Distance=0.004)
    s.link(s.math('ADD', rip, s.math('MULTIPLY', grains, 0.3)), b2.inputs['Height'])
    s.set('Normal', b2.outputs['Normal'])
    return m.m, s.m


def mesh_from_arrays(name, P, F, UV, mat_idx, mats, smooth=True):
    """A Blender mesh (PROP frame: P already in Blender coordinates) with corner UVs and material indices."""
    me = bpy.data.meshes.new(name)
    me.vertices.add(len(P))
    me.vertices.foreach_set('co', np.asarray(P, dtype=np.float32).ravel())
    me.loops.add(F.size)
    me.loops.foreach_set('vertex_index', F.ravel().astype(np.int32))
    me.polygons.add(len(F))
    me.polygons.foreach_set('loop_start', (np.arange(len(F)) * 3).astype(np.int32))
    me.polygons.foreach_set('material_index', np.asarray(mat_idx, dtype=np.int32))
    me.update(calc_edges=True)
    lay = me.uv_layers.new(name='UVMap')
    lay.data.foreach_set('uv', np.asarray(UV, dtype=np.float32).reshape(-1, 2).ravel())
    for m in mats:
        me.materials.append(m)
    me.validate(clean_customdata=False)
    if smooth:
        me.shade_smooth()
    o = bpy.data.objects.new(name, me)
    core.link(o)
    return o


def weld(P, F, UV, tol=1e-5):
    """Merges coincident vertices (the cube faces' shared edges), keeping corner UVs."""
    key = np.round(P / tol).astype(np.int64)
    _, idx, inv = np.unique(key, axis=0, return_index=True, return_inverse=True)
    return P[idx], inv.ravel()[F], UV


def build_rock_a():
    """A granite outcrop, a cliff chunk 14 x 8 x 6 m: a broad faceted base, two stacked blocks stepping up to a
    sloping top, a fallen slab at its foot; jointed (two vertical sets and the bedding: ledges that hold snow),
    eroded and cracked; lichen on the tops, frost in the cracks."""
    rng = random.Random(1401)
    blocks = [((0.0, 0.0, 1.4), (6.8, 3.8, 1.9), _rand_normals(rng, 16, 0.95)),
              ((-1.9, 0.5, 3.6), (4.3, 3.0, 1.9), _rand_normals(rng, 14, 0.92)),
              ((3.0, -0.5, 3.4), (3.0, 2.6, 1.7), _rand_normals(rng, 14, 0.95)),
              ((-0.4, 0.2, 5.0), (2.4, 1.9, 0.9), _rand_normals(rng, 12, 0.9)),
              ((-5.3, -1.6, 0.8), (1.6, 1.8, 1.1), _rand_normals(rng, 12, 0.9))]
    shape = RockShape(1401, blocks, joints=[((1.0, 0.35, 0.05), 2.4, 0.24, 0.16), ((-0.4, 1.0, 0.0), 2.9, 0.22, 0.15),
                                            ((0.05, 0.08, 1.0), 1.05, 0.12, 0.14)], rough=0.2, crack_depth=0.12, erode=0.3, edge=0.06)
    P, F, UV, fs, sv = cube_sphere_rock('SM_ECX_Rock_A', shape, (0.0, 0.0, 2.2), 12.0, 112, ground=-0.35, snow=1.0, snow_depth=0.22, fit=(14.0, 8.0, 6.0))
    mats = rock_materials('a', 1.0)
    return mesh_from_arrays('SM_ECX_Rock_A', P, F, UV, fs.astype(np.int32), mats)


def build_rock_b():
    """A cluster of granite boulders (5 x 4 x 2.5 m): one big, three smaller, rounded by weathering, snow capped."""
    rng = random.Random(1402)
    specs = [((0.0, 0.0, 0.9), (1.75, 1.4, 1.3)), ((-1.7, 0.95, 0.5), (0.95, 0.8, 0.75)), ((1.6, -0.9, 0.42), (0.85, 0.72, 0.6)),
             ((-0.9, -1.4, 0.3), (0.6, 0.55, 0.45))]
    allP, allF, allUV, allM = [], [], [], []
    off = 0
    for k, (c, rad) in enumerate(specs):
        shape = RockShape(1410 + k, [(c, rad, _rand_normals(rng, 14, 0.95))], joints=[((1.0, rng.random(), 0.1), 1.3, 0.05, 0.1),
                                                                               ((rng.random(), 1.0, 0.3), 0.9, 0.04, 0.06)],
                          rough=0.1, crack_depth=0.05, erode=0.18)
        n = 44 if k == 0 else (32 if k < 3 else 24)
        P, F, UV, fs, sv = cube_sphere_rock(f'SM_ECX_Rock_B[{k}]', shape, c, 4.0, n, ground=-0.25, snow=0.9, snow_depth=0.12)
        UV = UV * np.array([0.5, 1.0]) if k == 0 else UV * np.array([0.5, 1.0 / 3.0]) + np.array([0.5, (k - 1) / 3.0])
        allP.append(P)
        allF.append(F + off)
        allUV.append(UV)
        allM.append(fs.astype(np.int32))
        off += len(P)
    mats = rock_materials('b', 0.6)
    P = np.concatenate(allP)
    lo, hi = P.min(axis=0), P.max(axis=0)
    P[:, 0] = (P[:, 0] - (lo[0] + hi[0]) / 2) * 5.0 / (hi[0] - lo[0])
    P[:, 1] = (P[:, 1] - (lo[1] + hi[1]) / 2) * 4.0 / (hi[1] - lo[1])
    P[:, 2] = -0.25 + (P[:, 2] + 0.25) * 2.5 / (hi[2] + 0.25)
    return mesh_from_arrays('SM_ECX_Rock_B', P, np.concatenate(allF), np.concatenate(allUV), np.concatenate(allM), mats)


def build_rock_c():
    """A flat slab of granite (3 x 2 x 1 m), split along a bedding plane, snow on its top."""
    rng = random.Random(1420)
    shape = RockShape(1420, [((0.0, 0.0, 0.3), (1.5, 1.0, 0.45), _rand_normals(rng, 10, 0.9))],
                      joints=[((0.0, 0.1, 1.0), 0.42, 0.04, 0.06), ((1.0, -0.6, 0.0), 1.3, 0.05, 0.07)], rough=0.04, crack_depth=0.03, erode=0.06,
                      edge=0.05)
    P, F, UV, fs, sv = cube_sphere_rock('SM_ECX_Rock_C', shape, (0.0, 0.0, 0.3), 3.0, 40, ground=-0.2, snow=1.0, snow_depth=0.1, fit=(3.0, 2.0, 1.0))
    mats = rock_materials('c', 0.4)
    return mesh_from_arrays('SM_ECX_Rock_C', P, F, UV, fs.astype(np.int32), mats)


def sd_polytope(p, c, radii, normals, k=0.12):
    """A faceted block: the intersection of half-spaces whose planes touch an ellipsoid (radii) round c, their
    meeting edges softened by k meters."""
    q = p - np.asarray(c)
    acc = None
    for n, f in normals:
        off = math.sqrt(sum((radii[i] * n[i]) ** 2 for i in range(3))) * f
        d = q @ np.asarray(n) - off
        acc = d if acc is None else np.logaddexp(acc / k, d / k) * k
    return acc


def _rand_normals(rng, count, flat_top=0.0):
    out = []
    for i in range(count):
        z = rng.uniform(-0.75, 1.0)
        a = rng.uniform(0, 2 * math.pi)
        r = math.sqrt(1 - z * z)
        n = np.array([r * math.cos(a), r * math.sin(a), z])
        out.append((n / np.linalg.norm(n), rng.uniform(0.8, 1.0)))
    if flat_top:
        out.append((np.array([rng.uniform(-0.15, 0.15), rng.uniform(-0.15, 0.15), 1.0]) / 1.02, flat_top))
    return out


def _smooth_vertex_values(F, values, nverts, iters=6):
    """Averages per-vertex values over their mesh neighbors (edge-connected), `iters` times."""
    e = np.concatenate([F[:, [0, 1]], F[:, [1, 2]], F[:, [2, 0]]])
    e = np.concatenate([e, e[:, ::-1]])
    deg = np.bincount(e[:, 0], minlength=nverts).astype(np.float64)
    v = values.astype(np.float64)
    for _ in range(iters):
        s = np.bincount(e[:, 0], weights=v[e[:, 1]], minlength=nverts)
        v = 0.5 * v + 0.5 * s / np.maximum(deg, 1)
    return v


def split_by_contour(P, F, UV, s, thr=0.5):
    """Cuts every triangle that the iso-line s = thr crosses along it, so a material boundary drawn by s runs smooth
    instead of stepping along the triangles' edges. Returns (P, F, UV, s, above) with `above` per face."""
    cls = s[F] >= thr
    n_up = cls.sum(axis=1)
    mixed = (n_up == 1) | (n_up == 2)
    keepF, keepUV, keepA = [F[~mixed]], [UV[~mixed]], [n_up[~mixed] == 3]
    newP = []
    edge_vid = {}
    base = len(P)

    def cut(a, b):
        key = (a, b) if a < b else (b, a)
        if key not in edge_vid:
            t = (thr - s[a]) / (s[b] - s[a])
            edge_vid[key] = (base + len(newP), t if a < b else 1.0 - t)
            newP.append(P[a] + (P[b] - P[a]) * t)
        vid, t = edge_vid[key]
        return vid, (t if a < b else 1.0 - t)

    outF, outUV, outA = [], [], []
    for fi in np.nonzero(mixed)[0]:
        f = F[fi]
        uv = UV[fi]
        c = cls[fi]
        odd_up = c.sum() == 1
        k = int(np.nonzero(c == odd_up)[0][0])          # the odd vertex
        i0, i1, i2 = k, (k + 1) % 3, (k + 2) % 3
        a, b, cc = f[i0], f[i1], f[i2]
        pab, tab = cut(a, b)
        pca, tca = cut(cc, a)
        uab = uv[i0] + (uv[i1] - uv[i0]) * tab
        uca = uv[i2] + (uv[i0] - uv[i2]) * tca
        outF += [(a, pab, pca), (pab, b, cc), (pab, cc, pca)]
        outUV += [(uv[i0], uab, uca), (uab, uv[i1], uv[i2]), (uab, uv[i2], uca)]
        outA += [odd_up, not odd_up, not odd_up]
    P2 = np.vstack([P, np.asarray(newP)]) if newP else P
    s2 = np.concatenate([s, np.full(len(newP), thr)])
    F2 = np.vstack(keepF + [np.asarray(outF, dtype=np.int64)])
    UV2 = np.concatenate(keepUV + [np.asarray(outUV, dtype=np.float32)])
    A2 = np.concatenate(keepA + [np.asarray(outA, dtype=bool)])
    return P2, F2, UV2, s2, A2


def _skirt_tree(rng, x, y, H, R, tiers, M=16):
    """An opaque conifer for distance: tiers of jagged, drooping skirts round a trunk (closed underneath), narrowing to
    a spire. Returns (P, F, mat) in Blender coordinates (PROP frame), mat 0 = foliage, 1 = bark."""
    P, F, MAT = [], [], []

    def add(pts, faces, mat):
        base = len(P)
        P.extend(pts)
        F.extend([tuple(base + i for i in f) for f in faces])
        MAT.extend([mat] * len(faces))
    # Trunk (only its foot shows).
    tr = 0.03 * H + 0.05
    ring0 = [(x + tr * math.cos(2 * math.pi * k / 6), y + tr * math.sin(2 * math.pi * k / 6), -0.2) for k in range(6)]
    ring1 = [(x + tr * 0.7 * math.cos(2 * math.pi * k / 6), y + tr * 0.7 * math.sin(2 * math.pi * k / 6), H * 0.35) for k in range(6)]
    add(ring0 + ring1, [(k, (k + 1) % 6, 6 + (k + 1) % 6) for k in range(6)] + [(k, 6 + (k + 1) % 6, 6 + k) for k in range(6)], 1)
    hb = H * (0.025 + 0.04 * rng.random())          # the skirts reach nearly to the snow, as in a stand
    zs = [hb + (H - hb) * (1.0 - (1.0 - t) ** 1.25) for t in np.linspace(0.0, 1.0, tiers + 1)]
    for k in range(tiers):
        z0, z1 = zs[k], zs[k + 1]
        t = (z0 - hb) / (H - hb)
        r = R * (1.0 - t) ** 0.9 * (0.9 + 0.2 * rng.random()) + 0.15
        top = z1 + (z1 - z0) * 0.55            # skirts overlap: each tier's top hides under the next
        droop = (z1 - z0) * (0.6 + 0.4 * rng.random())
        ph = rng.random() * 6.28
        outer, mid, under = [], [], []
        for j in range(2 * M):
            # Ragged branch ends: tips of uneven reach and droop, notches between them, now and then a short limb.
            a = ph + math.pi * j / M + (rng.random() - 0.5) * 0.3
            tipr = (0.78 + 0.4 * rng.random()) * (0.6 if rng.random() < 0.12 else 1.0)
            rr = r * (tipr if j % 2 == 0 else 0.5 + 0.2 * rng.random()) * (0.9 + 0.2 * rng.random())
            zj = z0 - (droop * (0.2 + 0.35 * rng.random()) if j % 2 == 0 else -droop * 0.1) + (rng.random() - 0.5) * 0.25 * (z1 - z0)
            outer.append((x + rr * math.cos(a), y + rr * math.sin(a), zj))
            mid.append((x + rr * 0.55 * math.cos(a), y + rr * 0.55 * math.sin(a), top - (top - zj) * 0.42))
            under.append((x + rr * 0.35 * math.cos(a), y + rr * 0.35 * math.sin(a), zj + (top - zj) * 0.35))
        n = 2 * M
        pts = [(x, y, top)] + mid + outer + under + [(x, y, z0 + (top - z0) * 0.5)]
        faces = []
        for j in range(n):
            j1 = (j + 1) % n
            faces.append((0, 1 + j, 1 + j1))
            faces += [(1 + j, 1 + n + j, 1 + n + j1), (1 + j, 1 + n + j1, 1 + j1)]
            faces += [(1 + n + j, 1 + 2 * n + j, 1 + 2 * n + j1), (1 + n + j, 1 + 2 * n + j1, 1 + n + j1)]
            faces.append((1 + 2 * n + j, 1 + 3 * n, 1 + 2 * n + j1))
        add(pts, faces, 0)
    return P, F, MAT


def clump_materials(tag):
    """The clumps' foliage (near-black spruce green, needle clumps in the bump, snow on the upper faces in patches) and
    bark."""
    m = core.Mat(f'ecx_clump_{tag}')
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    geo = m.node('ShaderNodeNewGeometry')
    gs = m.node('ShaderNodeSeparateXYZ')
    m.link(geo.outputs['True Normal'], gs.inputs['Vector'])       # the facets' own facing (the shading normals are bent)
    vor = m.node('ShaderNodeTexVoronoi', Scale=3.5)
    m.link(o, vor.inputs['Vector'])
    tufts = ramp(m, vor.outputs['Distance'], 0.45, 0.05)
    fine = snoise(m, o, scale=14.0, detail=3.0)
    patch = ramp(m, snoise(m, o, scale=0.55, detail=4.0), 0.42, 0.58)
    snow = m.math('MULTIPLY', ramp(m, gs.outputs['Z'], 0.4, 0.78), m.math('MAXIMUM', patch, m.math('MULTIPLY', tufts, 0.85)))
    snow = m.math('MULTIPLY', snow, ramp(m, fine, 0.2, 0.5))
    under = ramp(m, gs.outputs['Z'], 0.1, -0.4)
    col = m.mix(m.math('MULTIPLY', tufts, 0.8), core.hex_linear(0x0e1810), core.hex_linear(0x22362a))
    col = m.mix(m.math('MULTIPLY', fine, 0.3), col, core.hex_linear(0x2c3f2c))
    col = m.mix(m.math('MULTIPLY', under, 0.7), col, core.hex_linear(0x080c08))
    col = m.mix(0.22, col, core.hex_linear(0x4a5f5a))                  # hoarfrost on the needles, as on the hero trees
    col = m.mix(snow, col, core.hex_linear(0xdde5ef))
    m.set('Base Color', col)
    m.set('Roughness', m.math('ADD', 0.75, m.math('MULTIPLY', snow, -0.12)))
    bump = m.node('ShaderNodeBump', Strength=0.8, Distance=0.08)
    m.link(m.math('ADD', tufts, m.math('MULTIPLY', fine, 0.5)), bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    b = core.Mat(f'ecx_clump_bark_{tag}')
    tc, sep = object_coords(b)
    bn = snoise(b, scaled(b, sep, 8.0, 8.0, 1.5), detail=4.0)
    b.set('Base Color', b.mix(bn, core.hex_linear(0x2a221d), core.hex_linear(0x4a3e35)))
    b.set('Roughness', 0.88)
    return m.m, b.m


def _clump_obj(name, trees, tag):
    P, F, MAT = [], [], []
    for (tp, tf, tm) in trees:
        base = len(P)
        P += tp
        F += [tuple(base + i for i in f) for f in tf]
        MAT += tm
    P = np.asarray(P, dtype=np.float64)
    F = np.asarray(F, dtype=np.int64)
    # Faces outward (the skirts' tops up, their undersides down) by the tree's local axis.
    mats = clump_materials(tag)
    me = bpy.data.meshes.new(name)
    me.from_pydata(P.tolist(), [], F.tolist())
    me.polygons.foreach_set('material_index', np.asarray(MAT, dtype=np.int32))
    for mm in mats:
        me.materials.append(mm)
    me.update()
    o = bpy.data.objects.new(name, me)
    core.link(o)
    bm = bmesh.new()
    bm.from_mesh(me)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    bm.to_mesh(me)
    bm.free()
    o = parts.finish([o], name, 30.0)
    # Foliage light: the skirts' normals bent outward from their tree's axis (and up), so each tree shades as one soft
    # mass, not as faceted cones (the bark keeps its own).
    centers = np.array([[np.mean([q[0] for q in tp]), np.mean([q[1] for q in tp])] for (tp, tf, tm) in trees])
    me = o.data
    nv = len(me.vertices)
    co = np.empty(nv * 3, dtype=np.float32)
    me.vertices.foreach_get('co', co)
    co = co.reshape(-1, 3)
    vn = np.empty(nv * 3, dtype=np.float32)
    me.vertices.foreach_get('normal', vn)
    vn = vn.reshape(-1, 3)
    pm = np.empty(len(me.polygons), dtype=np.int32)
    me.polygons.foreach_get('material_index', pm)
    lv = np.empty(len(me.loops), dtype=np.int32)
    me.loops.foreach_get('vertex_index', lv)
    ls = np.empty(len(me.polygons), dtype=np.int32)
    me.polygons.foreach_get('loop_total', ls)
    loop_mat = np.repeat(pm, ls)
    k = np.argmin(((co[:, None, :2] - centers[None, :, :]) ** 2).sum(axis=2), axis=1)
    rad = np.zeros_like(co)
    rad[:, :2] = co[:, :2] - centers[k]
    rad /= np.maximum(np.linalg.norm(rad, axis=1, keepdims=True), 1e-6)
    rad[:, 2] = 0.5
    rad /= np.linalg.norm(rad, axis=1, keepdims=True)
    bent = 0.4 * vn + 0.6 * rad
    bent /= np.maximum(np.linalg.norm(bent, axis=1, keepdims=True), 1e-6)
    ln = np.empty(len(me.loops) * 3, dtype=np.float32)
    me.loops.foreach_get('normal', ln)
    ln = ln.reshape(-1, 3)
    ln = np.where((loop_mat == 0)[:, None], bent[lv], ln)
    me.normals_split_custom_set([tuple(v) for v in ln])
    return o


def build_forest_clump_a():
    """Seven conifers for the far slopes (12 x 12 x 18 m): tiered jagged skirts, near-black green, snow on their tops."""
    rng = random.Random(2701)
    spots = [(0.0, 0.0, 18.0), (3.6, 2.4, 14.5), (-3.4, 2.8, 12.5), (2.6, -3.6, 11.0), (-2.8, -3.2, 9.0), (4.6, -0.6, 7.0), (-4.6, -0.2, 5.0)]
    trees = [_skirt_tree(rng, x, y, H, 0.2 * H + 0.4, 15 if H > 10 else 11, M=11 if H > 10 else 8) for x, y, H in spots]
    return _clump_obj('SM_ECX_ForestClump_A', trees, 'a')


def build_forest_clump_b():
    """A looser clump of five (16 x 10 x 14 m) and a snag: a dead spruce, grey, its limbs broken to stubs."""
    rng = random.Random(2702)
    spots = [(-4.5, 0.6, 14.0), (0.2, -2.2, 11.5), (4.8, 1.8, 10.0), (-0.8, 3.0, 8.0), (6.6, -2.6, 6.0)]
    trees = [_skirt_tree(rng, x, y, H, 0.21 * H + 0.4, 14 if H > 9 else 10, M=10 if H > 9 else 7) for x, y, H in spots]
    # The snag.
    P, F, MAT = [], [], []
    sx, sy, H = 2.2, 2.6, 11.0
    rings = []
    for k, z in enumerate(np.linspace(-0.2, H, 9)):
        r = 0.2 * (1.0 - z / (H * 1.15)) + 0.03
        rings.append([(sx + r * math.cos(2 * math.pi * j / 6) + 0.03 * k, sy + r * math.sin(2 * math.pi * j / 6), z) for j in range(6)])
    for k in range(len(rings)):
        P += rings[k]
    for k in range(len(rings) - 1):
        for j in range(6):
            a, b = k * 6 + j, k * 6 + (j + 1) % 6
            F += [(a, b, b + 6), (a, b + 6, a + 6)]
            MAT += [1, 1]
    for k in range(14):
        z = 2.0 + rng.random() * (H - 3.0)
        az = rng.random() * 6.28
        L = 0.3 + 0.9 * rng.random() * (1.0 - z / H)
        b0 = len(P)
        r = 0.035
        c0 = np.array([sx, sy, z])
        d = np.array([math.cos(az), math.sin(az), -0.25])
        c1 = c0 + d * L
        for c in (c0, c1):
            for j in range(4):
                a = 2 * math.pi * j / 4
                P.append(tuple(c + r * np.array([-math.sin(az) * math.cos(a), math.cos(az) * math.cos(a), math.sin(a)])))
        for j in range(4):
            F += [(b0 + j, b0 + (j + 1) % 4, b0 + 4 + (j + 1) % 4), (b0 + j, b0 + 4 + (j + 1) % 4, b0 + 4 + j)]
            MAT += [1, 1]
    trees.append((P, F, MAT))
    return _clump_obj('SM_ECX_ForestClump_B', trees, 'b')


DEG = math.pi / 180.0


VALLEY_EDGE = 84.8 * DEG          # Valley owns |theta| < 84.8 deg beyond Near; Mid and Far the rest


CITY_TH = (-50.0 * DEG, 56.0 * DEG)


def _sq(theta, half):
    """Distance from the origin to a square of half-size `half` along azimuth theta."""
    return half / np.maximum(np.abs(np.cos(theta)), np.abs(np.sin(theta)))


def r_site(theta):
    return _sq(theta, SITE_HALF)


def r_near(theta):
    """Near's outer edge: 3 km, drawing in to 2.5 km toward the Crest's foot (east)."""
    te = np.angle(np.exp(1j * (np.asarray(theta) - math.pi)))
    return 3000.0 - 500.0 * np.exp(-(te / (35.0 * DEG)) ** 2)


def r_mid(theta):
    return np.full(np.shape(theta), 8000.0)


def r_valley(theta):
    t = np.asarray(theta)
    return np.minimum(20000.0 / np.maximum(np.cos(t), 1e-6), 15000.0 / np.maximum(np.abs(np.sin(t)), 1e-6))


def r_city0(theta):
    return 6000.0 / np.cos(np.asarray(theta))


def r_city1(theta):
    t = np.asarray(theta)
    return np.minimum(16000.0 / np.cos(t), 14000.0 / np.maximum(np.abs(np.sin(t)), 1e-6))


def r_far(theta):
    return _sq(theta, 30000.0)


# Each terrain mesh: its sectors (theta0, theta1, r_in(theta), r_out(theta)), spoke spacing (radians), ring ratio, and
# its texture bands (theta0, theta1): stacked in v, each with its own radial mapping.


POLAR = {
    'SM_ECX_Ridge_Near': dict(
        sectors=[(-135 * DEG, -45 * DEG, r_site, r_near), (-45 * DEG, 45 * DEG, r_site, r_near), (45 * DEG, 135 * DEG, r_site, r_near),
                 (135 * DEG, 225 * DEG, r_site, r_near)],
        dtheta=0.45 * DEG, q=1.011, bands=[(-45 * DEG, 45 * DEG), (45 * DEG, 135 * DEG), (135 * DEG, 225 * DEG), (-135 * DEG, -45 * DEG)],
        usable=1.0 - 128.0 / 4096.0),
    'SM_ECX_Ridge_Mid': dict(
        sectors=[(VALLEY_EDGE, 2 * math.pi - VALLEY_EDGE, r_near, r_mid)],
        dtheta=0.2 * DEG, q=1.005, bands=[(150 * DEG, 210 * DEG), (VALLEY_EDGE, 150 * DEG), (210 * DEG, 2 * math.pi - VALLEY_EDGE)]),
    'SM_ECX_Valley': dict(
        sectors=[(-VALLEY_EDGE, CITY_TH[0], r_near, r_valley), (CITY_TH[0], CITY_TH[1], r_near, r_city0), (CITY_TH[0], CITY_TH[1], r_city1, r_valley),
                 (CITY_TH[1], VALLEY_EDGE, r_near, r_valley)],
        dtheta=0.33 * DEG, q=1.0078, bands=[(-28.3 * DEG, 28.3 * DEG), (28.3 * DEG, VALLEY_EDGE), (-VALLEY_EDGE, -28.3 * DEG)]),
    'SM_ECX_Ridge_Far': dict(
        sectors=[(-VALLEY_EDGE, VALLEY_EDGE, r_valley, r_far), (VALLEY_EDGE, 2 * math.pi - VALLEY_EDGE, r_mid, r_far)],
        dtheta=0.4 * DEG, q=1.008, bands=[(-45 * DEG, 45 * DEG), (45 * DEG, 135 * DEG), (135 * DEG, 225 * DEG), (225 * DEG, 315 * DEG)]),
    'SM_ECX_ValleyCity': dict(
        sectors=[(CITY_TH[0], CITY_TH[1], r_city0, r_city1)],
        dtheta=0.4 * DEG, q=1.028, bands=[(CITY_TH[0] + k * (CITY_TH[1] - CITY_TH[0]) / 3, CITY_TH[0] + (k + 1) * (CITY_TH[1] - CITY_TH[0]) / 3) for k in range(3)]),
}


RING0 = 200.0


def _spoke_radii(r_in, r_out, q):
    """r_in, the global rings strictly between, r_out."""
    k0 = math.ceil(math.log(r_in * 1.0005 / RING0) / math.log(q))
    k1 = math.floor(math.log(r_out * 0.9995 / RING0) / math.log(q))
    mid = [RING0 * q ** k for k in range(k0, k1 + 1)] if k1 >= k0 else []
    mid = [r for r in mid if r_in * 1.002 < r < r_out * 0.998]
    return [r_in] + mid + [r_out]


def polar_mesh(spec):
    """Triangulates a mesh's sectors spoke to spoke (a zipper between neighboring spokes' radii), exact at every sector
    boundary; sectors sharing a spoke are welded. Returns (theta (N,), r (N,), faces (M, 3), face's sector)."""
    verts = {}
    th_l, r_l = [], []
    faces = []

    def vid(t, r):
        key = (round(t, 9), round(r, 4))
        k = verts.get(key)
        if k is None:
            k = len(th_l)
            verts[key] = k
            th_l.append(t)
            r_l.append(r)
        return k

    for (t0, t1, fin, fout) in spec['sectors']:
        n = max(2, int(round((t1 - t0) / spec['dtheta'])))
        ts = np.linspace(t0, t1, n + 1)
        # Square corners (Near's inner edge) must be spokes: they are, at the sectors' ends (+-45, +-135 deg).
        spokes = []
        for t in ts:
            rs = _spoke_radii(float(fin(np.array([t]))[0]), float(fout(np.array([t]))[0]), spec['q'])
            spokes.append([vid(float(t), r) for r in rs])
        for a, b in zip(spokes[:-1], spokes[1:]):
            ra = [r_l[v] for v in a]
            rb = [r_l[v] for v in b]
            i = j = 0
            while i < len(a) - 1 or j < len(b) - 1:
                if j >= len(b) - 1 or (i < len(a) - 1 and math.log(ra[i + 1]) <= math.log(rb[j + 1])):
                    faces.append((a[i], b[j], a[i + 1]))
                    i += 1
                else:
                    faces.append((a[i], b[j], b[j + 1]))
                    j += 1
    return np.asarray(th_l), np.asarray(r_l), np.asarray(faces, dtype=np.int64)


class RadialMap:
    """One texture band's mapping: u by azimuth (linear across the band, gutters at both ends), v by distance with a
    density that follows the screen height the ground takes seen from the plaza (plus a log floor)."""

    def __init__(self, th0, th1, r0, r1, v0, v1, gu, gv, sample_fn):
        self.th0, self.th1, self.v0, self.v1, self.gu, self.gv = th0, th1, v0, v1, gu, gv
        rs = r0 * (r1 / r0) ** np.linspace(-0.2, 1.2, 4000)
        ths = np.linspace(th0, th1, 9)
        TH, RR = np.meshgrid(ths, rs)
        z = sample_fn(RR * np.cos(TH), RR * np.sin(TH))
        e = np.arctan2(z - EYE[2], RR)
        de = np.abs(np.gradient(e, rs, axis=0)).mean(axis=1)
        logd = 1.0 / rs
        w = np.trapezoid(de, rs) / max(np.trapezoid(logd, rs), 1e-9)
        rho = de + 0.35 * w * logd
        cum = np.concatenate([[0.0], np.cumsum(0.5 * (rho[1:] + rho[:-1]) * np.diff(rs))])
        c0, c1 = np.interp(r0, rs, cum), np.interp(r1, rs, cum)
        self.rs = rs
        self.t = (cum - c0) / (c1 - c0)

    def uv(self, th, r):
        su = (th - self.th0) / (self.th1 - self.th0)
        sv = np.interp(r, self.rs, self.t)
        return self.gu + (1 - 2 * self.gu) * su, self.v0 + (self.v1 - self.v0) * (self.gv + (1 - 2 * self.gv) * sv)

    def inverse(self, u, v):
        su = (u - self.gu) / (1 - 2 * self.gu)
        sv = ((v - self.v0) / (self.v1 - self.v0) - self.gv) / (1 - 2 * self.gv)
        return self.th0 + su * (self.th1 - self.th0), np.interp(sv, self.t, self.rs)


def make_bands(name, size, usable=1.0):
    """The texture bands of a polar mesh: RadialMaps stacked in v (equal heights) over the bottom `usable` of the
    texture, gutters of 24 texels (at 4096)."""
    spec = POLAR[name]
    bands = spec['bands']
    nb = len(bands)
    maps = []
    for k, (a, b) in enumerate(bands):
        tt = np.linspace(a, b, 64)
        r0 = min(float(np.min(fin(tt))) for (t0, t1, fin, fout) in spec['sectors'] if t1 > a and t0 < b)
        r1 = max(float(np.max(fout(tt))) for (t0, t1, fin, fout) in spec['sectors'] if t1 > a and t0 < b)
        g = 24.0 * size / 4096.0
        maps.append(RadialMap(a, b, r0, r1, usable * k / nb, usable * (k + 1) / nb, g / size, g * nb / (size * usable),
                              lambda X, Y: natural_height(X, Y, 60.0)))
    return maps


def band_of(theta, bands):
    """Index of the band each azimuth falls in (angles compared modulo a turn)."""
    out = np.zeros(np.shape(theta), dtype=np.int64)
    for k, (a, b) in enumerate(bands):
        d = np.mod(np.asarray(theta) - a + 1e-9, 2 * math.pi)
        out = np.where(d < (b - a) + 2e-9, k, out)
    return out


def _lin(h):
    return np.array(core.hex_linear(h)[:3], dtype=np.float32)


def highway_x(Y):
    Y = np.asarray(Y, dtype=np.float32)
    return river_x(Y) - 1650.0 + 260.0 * np.sin(Y / 5200.0 + 1.1)


def shade_landcover(X, Y, Z, nrm, texel, cav, cav_big):
    """The ground's look at map points (flat arrays): snow above the snow line (deep in gullies, gone from cliffs),
    granite where it's steep, conifer forest (crowns at the texel scale, snow on them high up), and below the snow
    line the valley in October: dark conifers and golden aspens on the slopes, the river and its yellow cottonwoods,
    a patchwork of fields, hedgerows, farms and roads. Returns (albedo (N, 3) linear, roughness, canopy height m)."""
    N = len(X)
    slope = np.degrees(np.arccos(np.clip(nrm[:, 2], -1.0, 1.0))).astype(np.float32)
    n1 = fbm(X, Y, 340.0, 4, seed=701)
    n2 = fbm(X, Y, 37.0, 3, seed=702)
    n3 = fbm(X, Y, 7.0, 2, seed=703)
    # Snow on the ground: above the snow line, off the cliffs (gullies and ledges hold it steeper), wind-scoured on the
    # sharpest crests.
    alt = smoothstep(-560.0, -360.0, Z + 70.0 * n1 + 20.0 * n2)
    high = smoothstep(450.0, 800.0, Z)
    # High up, the early season's wet snow plasters the faces: it holds steeper (to ~50 deg on open rock, past 60 in
    # the gullies and on the ledges), the ribs and the steepest walls stay dark, and its edges are crisp.
    hold_low = 41.0 + 8.0 * smoothstep(5.0, 16.0, cav_big) + 7.0 * smoothstep(1.0, 4.0, cav) + 5.0 * n2
    hold_high = (49.0 + 5.0 * smoothstep(-1.0, 5.0, cav) + 10.0 * smoothstep(-2.0, 8.0, cav_big) - 7.0 * smoothstep(6.0, 16.0, -cav_big)
                 + 2.5 * n2)
    hold = hold_low + (hold_high - hold_low) * high
    width = 10.0 - 6.0 * high
    snow = alt * (1.0 - smoothstep(hold, hold + width, slope))
    snow = snow * (1.0 - 0.6 * smoothstep(2.0, 8.0, -cav_big) * smoothstep(20.0, 40.0, slope))
    # Forest.
    # Forest, cleared along the roads (a plowed corridor each side, so the switchbacks read as clean lines).
    rdist, rz, rkind = raster().sample(X, Y)
    forest = forest_density(X, Y, Z, slope, road_dist=rdist)
    # Crowns: one tree per jittered cell, its radius and height random; at coarse texels, their mean.
    f1, f2, rid, ddx, ddy = cells(X, Y, 4.2, seed=711, jitter=0.9)
    rc = 1.8 + 1.4 * hash01(np.floor(rid * 4096).astype(np.int64), np.int64(3), 712)
    present = rid < forest
    crown = np.clip(1.0 - (f1 / rc) ** 2, 0.0, 1.0) * present           # a dome: 1 at the stem, 0 at the rim
    tall = 9.0 + 9.0 * hash01(np.floor(rid * 4096).astype(np.int64), np.int64(5), 713)
    struct = smoothstep(4.5, 1.6, texel)
    canopy = (np.sqrt(crown) * tall) * struct + forest * 7.5 * (1.0 - struct) * (0.8 + 0.4 * n3)
    disc = smoothstep(0.0, 0.12, crown)                                 # the crown seen from above: solid, soft-edged
    cover = np.clip(disc * struct + forest * 0.88 * (1.0 - struct), 0.0, 1.0)
    # The texture is the land as the plaza sees it: where the view grazes a forested slope the snow between the crowns
    # hides behind the trees (the farther and the flatter, the less of it shows).
    vx, vy, vz = EYE[0] - X, EYE[1] - Y, EYE[2] - Z
    inc = (vx * nrm[:, 0] + vy * nrm[:, 1] + vz * nrm[:, 2]) / (np.sqrt(vx * vx + vy * vy + vz * vz) + 1e-6)
    graze = np.clip(inc / 0.35, 0.2, 1.0).astype(np.float32)
    cover = np.clip(cover + (1.0 - cover) * forest * (1.0 - graze), 0.0, 1.0)
    # Colors.
    snow_c = _lin(0xe6ecf4) * (0.95 + 0.05 * n3[:, None])
    snow_c = snow_c * (1.0 - 0.04 * smoothstep(0.0, 1.0, n2)[:, None])
    rock_c = _lin(0x56534f) * (0.7 + 0.55 * (0.5 + 0.5 * fbm(X, Y, 90.0, 4, seed=721)))[:, None]
    rock_c = rock_c * (1.0 - 0.35 * smoothstep(0.0, 0.6, fbm(X * 0.6, Y, 25.0, 3, seed=722)))[:, None]
    # Streaks down the faces (water and ice run down the fall line, mostly west here) and lighter weathered rock.
    streak = smoothstep(0.1, 0.5, fbm(X * 0.08, Y, 14.0, 3, seed=723)) * smoothstep(30.0, 50.0, slope)
    rock_c = rock_c * (1.0 - 0.38 * streak)[:, None]
    pale = smoothstep(0.25, 0.6, fbm(X, Y, 160.0, 3, seed=724))
    rock_c = rock_c * (1.0 - 0.3 * pale)[:, None] + _lin(0x8c8780)[None, :] * (0.3 * pale)[:, None]
    scree = smoothstep(26.0, 34.0, slope) * (1.0 - smoothstep(38.0, 44.0, slope)) * smoothstep(0.0, 0.5, n2 + 0.3)
    rock_c = rock_c * (1.0 - scree[:, None]) + _lin(0x7d7a74)[None, :] * scree[:, None]
    # The valley (no snow): meadow and slope grass, fields on the floor, the river, roads, farms.
    meadow = _lin(0x5a4c2c) * (0.85 + 0.3 * (0.5 + 0.5 * n2))[:, None]
    ground = meadow.copy()
    floor = smoothstep(-835.0, -870.0, Z) * (1.0 - smoothstep(4.0, 8.0, slope))
    if np.any(floor > 0):
        # Farmland in October, seen from 900 m up: districts (Voronoi cells about 2.6 km across, each with its own
        # orientation near the valley's and its own field size), divided into blocks, each block into one to four
        # strips; each field its own crop in a muted autumn palette, mottled inside (moisture, wheel tracks, rows);
        # grass margins along every boundary, tree lines along about half of them and along the districts' edges
        # (dotted with crowns), a farmstead in some blocks.
        f1d, f2d, did, ddx_d, ddy_d = cells(X, Y, 2600.0, seed=745, jitter=0.8)
        a_d = 0.14 + 0.9 * (hash01(np.floor(did * 65536).astype(np.int64), np.int64(1), 746) - 0.5)
        bw = 300.0 + 200.0 * hash01(np.floor(did * 65536).astype(np.int64), np.int64(2), 746)
        bh = 190.0 + 150.0 * hash01(np.floor(did * 65536).astype(np.int64), np.int64(3), 746)
        lx0, ly0 = -ddx_d, -ddy_d                                   # from the district's center
        ca, sa = np.cos(a_d), np.sin(a_d)
        fx = (lx0 * ca - ly0 * sa + 40.0 * fbm(X, Y, 1500.0, 2, seed=731)) / bw
        fy = (lx0 * sa + ly0 * ca + 40.0 * fbm(Y, X, 1500.0, 2, seed=732)) / bh
        bx, by = np.floor(fx).astype(np.int64), np.floor(fy).astype(np.int64)
        dk = np.floor(did * 4096).astype(np.int64)
        bx, by = bx + dk * 1000, by - dk * 977                         # blocks are unique per district
        tx, ty = fx - np.floor(fx), fy - np.floor(fy)
        nsplit = 1 + np.floor(hash01(bx, by, 740) * 3.999).astype(np.int64)
        along = hash01(bx, by, 741) < 0.5
        tt = np.where(along, tx, ty)
        strip = np.floor(tt * nsplit).astype(np.int64)
        kid = (bx * 7919 + by * 104729) * 8 + strip
        kind = hash01(kid, np.int64(1), 733)
        shade = 0.92 + 0.16 * hash01(kid, np.int64(2), 734)
        e_block = np.minimum(np.minimum(tx, 1 - tx) * bw, np.minimum(ty, 1 - ty) * bh)
        st = tt * nsplit - strip
        e_strip = np.minimum(st, 1 - st) * np.where(along, bw, bh) / nsplit
        e_field = np.minimum(e_block, np.where(nsplit > 1, e_strip, 1e9))
        e_dist = 0.5 * (f2d - f1d)                                     # to the district's edge
        palette = [(0.22, 0x7b7152), (0.44, 0x66643f), (0.64, 0x55603a), (0.78, 0x57473a), (0.90, 0x6b6247), (1.01, 0x4b5a35)]
        fld = np.zeros((len(X), 3), dtype=np.float32)
        lo = 0.0
        for hi, hx in palette:
            sel = (kind >= lo) & (kind < hi)
            fld[sel] = _lin(hx)[None, :] * shade[sel, None]
            lo = hi
        ang = hash01(kid, np.int64(3), 738) * math.pi
        rows = 0.5 + 0.5 * np.sin((X * np.cos(ang) + Y * np.sin(ang)) * 2 * math.pi / 6.0)
        mott = 0.5 + 0.5 * fbm(X, Y, 45.0, 3, seed=750)
        wet = 0.5 + 0.5 * fbm(X, Y, 900.0, 2, seed=751)
        fld = fld * ((0.95 + 0.05 * rows) * (0.84 + 0.28 * mott) * (0.92 + 0.14 * wet) * (0.94 + 0.06 * (0.5 + 0.5 * n3)))[:, None]
        # Margins on every boundary; tree lines on some, and on the districts' edges.
        margin = 1.0 - smoothstep(1.0, 3.5, np.minimum(e_field, e_dist))
        fld = fld * (1.0 - 0.5 * margin[:, None]) + _lin(0x57553a)[None, :] * (0.5 * margin)[:, None]
        line = (1.0 - smoothstep(2.5, 6.0, e_field)) * (hash01(kid, np.int64(4), 735) < 0.45)
        line = np.maximum(line, 1.0 - smoothstep(4.0, 9.0, e_dist))
        crowns = smoothstep(0.0, 0.2, np.clip(1.0 - (f1 / rc) ** 2, 0.0, 1.0))
        hedge = line * (crowns * struct + 0.75 * (1.0 - struct))
        fld = fld * (1.0 - hedge[:, None]) + _lin(0x1c2414)[None, :] * hedge[:, None]
        farm = (hash01(bx, by, 736) < 0.12) & (tx > 0.03) & (tx < 0.12) & (ty > 0.04) & (ty < 0.16)
        roof = farm & (fbm(X, Y, 11.0, 2, seed=739) > 0.05)
        fld = np.where(farm[:, None], _lin(0x4a4436)[None, :], fld)
        fld = np.where(roof[:, None], np.where((hash01(bx, by, 737) < 0.5)[:, None], _lin(0x6a2a1e)[None, :], _lin(0x5a5a5e)[None, :]), fld)
        ground = ground * (1.0 - floor[:, None]) + fld * floor[:, None]
        canopy = canopy + 9.0 * hedge * floor * (0.6 + 0.4 * crowns)
    # Aspens and larches on the valley's slopes and floor edges: gold among the dark conifers.
    aspen = smoothstep(0.25, 0.55, fbm(X, Y, 260.0, 4, seed=741)) * smoothstep(-300.0, -560.0, Z) * forest
    # The river: dark water, gravel bars, cottonwoods along it.
    rd = np.abs(X - river_x(Y))
    water = 1.0 - smoothstep(38.0, 46.0, rd + 6.0 * n2)
    riparian = (1.0 - smoothstep(60.0, 190.0, rd + 40.0 * n2)) * (1.0 - water) * smoothstep(-700.0, -850.0, Z)
    # Roads on the floor: the highway, the road up to the switchbacks.
    hd = np.abs(X - highway_x(Y))
    road = (1.0 - smoothstep(6.0 - 0.5 * np.minimum(texel, 8.0), 9.0 + 0.5 * np.minimum(texel, 8.0), hd)) * smoothstep(-700.0, -860.0, Z)
    vr = roads()['valley']
    for i in range(len(vr) - 1):
        ax, ay, bx, by = vr[i, 0], vr[i, 1], vr[i + 1, 0], vr[i + 1, 1]
        sel = (X > min(ax, bx) - 15) & (X < max(ax, bx) + 15) & (Y > min(ay, by) - 15) & (Y < max(ay, by) + 15)
        if np.any(sel):
            d = seg_dist(X[sel], Y[sel], ax, ay, bx, by)[0]
            road[sel] = np.maximum(road[sel], 1.0 - smoothstep(4.0, 6.0, d))
    # The roads' beds, their edges as soft as a texel is wide (no stair-steps where a road runs across the texels).
    tw = 0.5 * np.minimum(texel, 6.0)
    bed = (1.0 - smoothstep(ROAD_HW - 0.5 - tw, ROAD_HW + 0.8 + tw, rdist)) * (rkind >= 1)
    # Compose: ground, snow, forest, rock, water, roads.
    col = ground
    col = col * (1.0 - snow[:, None]) + snow_c * snow[:, None]
    rock = smoothstep(hold - 4.0, hold + 6.0, slope) * (1.0 - 0.5 * snow)
    rock = np.maximum(rock, (1.0 - snow) * smoothstep(30.0, 40.0, slope))
    col = col * (1.0 - rock[:, None]) + rock_c * rock[:, None]
    tree_snow = smoothstep(-460.0, -180.0, Z) * 0.5 * (0.6 + 0.4 * n3)
    patches = smoothstep(0.66, 0.86, 0.5 + 0.5 * fbm(X, Y, 1.3, 2, seed=725) + 0.12 * crown)
    crown_top = patches * struct + 0.2 * (1.0 - struct)
    green = _lin(0x122019) * (0.8 + 0.4 * hash01(np.floor(rid * 4096).astype(np.int64), np.int64(9), 742))[:, None]
    green = green * (1.0 - aspen[:, None] * (rid > 0.45)[:, None]) + _lin(0x9a7a26)[None, :] * (aspen * (rid > 0.45))[:, None]
    tcol = green * (1.0 - (tree_snow * crown_top)[:, None]) + snow_c * 0.92 * (tree_snow * crown_top)[:, None]
    under = cover * (1.0 - struct) * 0.0
    col = col * (1.0 - cover[:, None]) + tcol * cover[:, None]
    # Shade under the canopy: the snow between crowns is darker.
    gap_dark = (1.0 - cover) * forest * 0.5
    col = col * (1.0 - gap_dark[:, None])
    rip_c = _lin(0xa98a2a) * (0.8 + 0.4 * (0.5 + 0.5 * n2))[:, None]
    rip = riparian * smoothstep(0.0, 0.5, n2 + 0.3)
    col = col * (1.0 - rip[:, None]) + rip_c * rip[:, None]
    canopy = canopy + rip * 10.0
    col = col * (1.0 - water[:, None]) + _lin(0x0f1a1e)[None, :] * water[:, None]
    roadc = _lin(0x2a2a2c)
    col = col * (1.0 - road[:, None]) + roadc[None, :] * road[:, None]
    bedc = np.where((Z > -400.0)[:, None], _lin(0x4a4846)[None, :], roadc[None, :])
    col = col * (1.0 - bed[:, None]) + bedc * bed[:, None]
    canopy = canopy * (1.0 - bed) * (1.0 - road) * (1.0 - water)
    rough = 0.62 * snow + 0.78 * rock + 0.88 * (1.0 - snow) * (1.0 - rock)
    rough = rough * (1.0 - cover) + 0.85 * cover
    rough = rough * (1.0 - water) + 0.08 * water
    return np.clip(col, 0.0, 1.0).astype(np.float32), np.clip(rough, 0.0, 1.0).astype(np.float32), canopy.astype(np.float32)


def terrain_texture(name, size):
    """Computes a polar mesh's maps texel by texel: base color (linear), ORM and a tangent-space normal map (OpenGL)
    holding the relief finer than the mesh (crowns, crags, gully walls). Returns (bands, base, orm, normal), rows
    bottom-up."""
    t0 = time.time()
    spec = POLAR[name]
    bands = make_bands(name, size, spec.get('usable', 1.0))
    base = np.zeros((size, size, 3), dtype=np.float32)
    orm = np.zeros((size, size, 3), dtype=np.float32)
    nmap = np.zeros((size, size, 3), dtype=np.float32)
    for k, bm in enumerate(bands):
        r0, r1 = int(round(bm.v0 * size)), int(round(bm.v1 * size))
        u = (np.arange(size) + 0.5) / size
        v = (np.arange(r0, r1) + 0.5) / size
        U, V = np.meshgrid(u, v)
        TH, R = bm.inverse(U, V)
        X = (R * np.cos(TH)).astype(np.float64)
        Y = (R * np.sin(TH)).astype(np.float64)
        dth = (bm.th1 - bm.th0) / ((1 - 2 * bm.gu) * size)
        tang = R * dth
        rad = np.abs(np.gradient(R, axis=0))
        texel = np.maximum(tang, rad).astype(np.float32)
        H0 = world_height(X.ravel(), Y.ravel(), lam=np.maximum(2.0 * texel.ravel(), 1.0)).reshape(X.shape)

        def normals(Hf):
            P = np.stack([X, Y, Hf.astype(np.float64)], axis=2)
            Pu = np.gradient(P, axis=1)
            Pv = np.gradient(P, axis=0)
            n = np.cross(Pu, Pv)
            n = n * np.sign(n[:, :, 2:3] + 1e-12)
            return n / np.maximum(np.linalg.norm(n, axis=2, keepdims=True), 1e-12), Pu, Pv
        n0, _, _ = normals(H0)
        # The mesh's own surface: the heights smoothed over its cell size (in texels across and along).
        cells_u = max(1, int(spec['dtheta'] / dth * 0.6))
        cells_v = max(1, int(np.median((R * (spec['q'] - 1.0)) / np.maximum(rad, 1e-6)) * 0.6))
        Hc = blur2(H0, (cells_v, cells_u))
        # Concavity at 15 m and 60 m (in meters, whatever the texture's size): gullies, the foot of walls.
        tu, tv = float(np.median(tang)), float(np.median(rad))
        cav = blur2(H0, (max(1, int(15.0 / max(tv, 1e-3) / 3)), max(1, int(15.0 / max(tu, 1e-3) / 3)))) - H0
        cav_big = blur2(H0, (max(1, int(40.0 / max(tv, 1e-3) / 3)), max(1, int(40.0 / max(tu, 1e-3) / 3)))) - H0
        col, rough, canopy = shade_landcover(X.ravel(), Y.ravel(), H0.ravel(), n0.reshape(-1, 3), texel.ravel(), cav.ravel(), cav_big.ravel())
        Hf = H0 + 0.6 * canopy.reshape(X.shape)
        nf, _, _ = normals(Hf)
        nc, Tu, Tv = normals(Hc)
        T = Tu - np.sum(Tu * nc, axis=2, keepdims=True) * nc
        T /= np.maximum(np.linalg.norm(T, axis=2, keepdims=True), 1e-12)
        B = Tv - np.sum(Tv * nc, axis=2, keepdims=True) * nc - np.sum(Tv * T, axis=2, keepdims=True) * T
        B /= np.maximum(np.linalg.norm(B, axis=2, keepdims=True), 1e-12)
        ts = np.stack([np.sum(nf * T, axis=2), np.sum(nf * B, axis=2), np.sum(nf * nc, axis=2)], axis=2)
        ts[:, :, 2] = np.maximum(ts[:, :, 2], 0.08)
        ts /= np.linalg.norm(ts, axis=2, keepdims=True)
        # Occlusion: the canopy's gaps, gullies, the foot of walls.
        hcav = blur2(Hf, (3, 3)) - Hf
        ao = np.clip(1.0 - np.maximum(hcav, 0.0) / 6.0 - 0.15 * np.maximum(cav_big, 0.0) / 10.0, 0.45, 1.0)
        base[r0:r1] = col.reshape(X.shape + (3,))
        orm[r0:r1, :, 0] = ao
        orm[r0:r1, :, 1] = rough.reshape(X.shape)
        nmap[r0:r1] = ts * 0.5 + 0.5
        log(f'{name}: band {k} ({r1 - r0} rows) in {time.time() - t0:.0f}s')
    return bands, base, orm, nmap


def build_polar_terrain(name, size, emissive=None):
    """A polar terrain mesh with its computed textures and a glTF material (M_<name>, like a baked single-slot mesh)."""
    t0 = time.time()
    spec = POLAR[name]
    th, r, F = polar_mesh(spec)
    X = r * np.cos(th)
    Y = r * np.sin(th)
    spacing = np.maximum(r * spec['dtheta'], r * (spec['q'] - 1.0))
    Z = world_height(X, Y, lam=2.0 * spacing)
    P = np.column_stack([X, Y, Z])
    # Faces counter-clockwise from above (map frame).
    a, b, c = P[F[:, 0]], P[F[:, 1]], P[F[:, 2]]
    cz = (b[:, 0] - a[:, 0]) * (c[:, 1] - a[:, 1]) - (b[:, 1] - a[:, 1]) * (c[:, 0] - a[:, 0])
    F[cz < 0] = F[cz < 0][:, ::-1]
    tex_size = size // 4 if FAST else size
    usable = spec.get('usable', 1.0)
    bands = make_bands(name, tex_size, usable)
    # Corner UVs: each face in the band of its middle.
    fth = np.angle(np.exp(1j * th[F]).mean(axis=1))
    fb = band_of(fth, spec['bands'])
    UV = np.zeros((len(F), 3, 2), dtype=np.float32)
    for k, bm in enumerate(bands):
        sel = fb == k
        if not np.any(sel):
            continue
        tt = th[F[sel]]
        # Unwrap each corner's azimuth into the band's range.
        tt = bm.th0 + np.mod(tt - bm.th0 + 1e-7, 2 * math.pi) - 1e-7
        tt = np.where(tt > bm.th1 + math.pi, tt - 2 * math.pi, tt)
        uu, vv = bm.uv(tt, r[F[sel]])
        UV[sel, :, 0] = uu
        UV[sel, :, 1] = vv
    # Skirts down from every open edge (2-5 % of the distance), both faces.
    be = boundary_edges(F)
    verts = np.unique(be)
    low = {int(v): len(P) + i for i, v in enumerate(verts)}
    depth = np.clip(0.015 * r[verts], 2.0, 400.0)
    P2 = np.vstack([P, np.column_stack([X[verts], Y[verts], Z[verts] - depth])])
    sk, skuv = [], []
    for (va, vb) in be:
        la, lb = low[int(va)], low[int(vb)]
        # The skirt takes the UVs of the face its edge belongs to: find them by a face lookup.
        sk += [(vb, va, la), (vb, la, lb), (va, vb, lb), (va, lb, la)]
    sk = np.asarray(sk, dtype=np.int64)
    # Skirt UVs: each top vertex's UV from any face that uses it (its band), the low copy the same.
    vuv = np.zeros((len(P2), 2), dtype=np.float32)
    vuv[F.ravel()] = UV.reshape(-1, 2)
    for v, lv in low.items():
        vuv[lv] = vuv[v]
    F2 = np.vstack([F, sk])
    UV2 = np.concatenate([UV, vuv[sk]])
    if name == 'SM_ECX_Ridge_Near':
        # The hamlet's chalets, their walls and roofs in the texture's top strip.
        cP, cF, cUV = chalet_geometry(usable)
        F2 = np.vstack([F2, cF + len(P2)])
        P2 = np.vstack([P2, cP])
        UV2 = np.concatenate([UV2, cUV])
    obj = make_mesh(name, P2, F2, uv=UV2)
    # Textures.
    _, base, orm, nrm = terrain_texture(name, tex_size)
    if name == 'SM_ECX_Ridge_Near':
        rows = tex_size - int(round(usable * tex_size))
        a, o, n = chalet_strip(tex_size, rows)
        base[tex_size - rows:], orm[tex_size - rows:], nrm[tex_size - rows:] = a, o, n
    tex_dir = os.path.join(core.BUILD_DIR, 'textures', name)
    paths = {}
    for key, arr, srgb in (('BaseColor', base, True), ('ORM', orm, False), ('Normal', nrm, False)):
        paths[key] = save_png(os.path.join(tex_dir, f'T_{name}_{key}.png'), arr, srgb=srgb)
    em_path = None
    if emissive is not None:
        em_path = save_png(os.path.join(tex_dir, f'T_{name}_Emissive.png'), emissive(bands, tex_size), srgb=True)
    mat = core.gltf_material(f'M_{name}', paths['BaseColor'], paths['ORM'], paths['Normal'], em_path, 1.0)
    mat.use_backface_culling = True
    core.assign(obj, mat)
    obj.data.calc_loop_triangles()
    log(f'{name}: {len(obj.data.loop_triangles)} triangles, Z {Z.min():.0f}..{Z.max():.0f}, {time.time() - t0:.0f}s')
    return obj


CITY_C = (10500.0, 1000.0)            # downtown


CITY_GLOW = 3.5                      # the emissive's strength (the engineer scales it with DimImportedGlow)


FACADE_ROWS = 256                    # the top rows of ValleyCity's texture: the towers' walls


def city_density(X, Y):
    """How built-up the valley floor is, 0..1: dense downtown, the city drawn out along the valley, sprawl along the
    freeway, satellite towns, ragged edges that thin out instead of stopping."""
    X = np.asarray(X, dtype=np.float32)
    Y = np.asarray(Y, dtype=np.float32)
    dx = (X - CITY_C[0]) / 3300.0
    dy = (Y - CITY_C[1]) / 5200.0
    d = np.exp(-(dx * dx + dy * dy) * 1.5)
    d = d + 0.25 * np.exp(-((X - CITY_C[0]) / 1100.0) ** 2 - ((Y - CITY_C[1]) / 1300.0) ** 2)
    hd = np.abs(X - highway_x(Y))
    d = d + 0.32 * np.exp(-(hd / 650.0) ** 2) * np.exp(-((Y - CITY_C[1]) / 9500.0) ** 2)
    for (tx, ty, tr) in ((7400.0, -6200.0, 900.0), (14200.0, 6800.0, 1100.0), (7900.0, 7600.0, 800.0), (12800.0, -5600.0, 950.0),
                         (6900.0, 3400.0, 600.0)):
        d = d + 0.5 * np.exp(-(((X - tx) ** 2 + (Y - ty) ** 2) / (tr * tr)))
    d = d * (0.6 + 0.7 * (0.5 + 0.5 * fbm(X, Y, 1600.0, 4, seed=801)))
    return np.clip(d, 0.0, 1.0).astype(np.float32)


def _grid_dist(c, period, width):
    """Distance (m) to the nearest grid line of a period along a coordinate, and the line's index."""
    k = np.floor(c / period + 0.5)
    return np.abs(c - k * period), k.astype(np.int64)


def city_layers(X, Y, texel):
    """The city at map points: (albedo (N, 3), roughness, emissive (N, 3) linear 0..1, canopy m). Streets in a grid
    (100 x 120 m downtown, 160-220 m out in the suburbs, arterials every 800 m), the river with its embankments and
    bridges, the freeway on the east bank, a rail yard, parks; by night sodium and LED street lights (dots every 35 m),
    the arterials and the freeway brighter, downtown's lit windows, the bridges' strings of lights."""
    N = len(X)
    dens = city_density(X, Y)
    n2 = fbm(X, Y, 37.0, 3, seed=811)
    n3 = fbm(X, Y, 7.0, 2, seed=812)
    down = smoothstep(0.55, 0.8, dens)
    # Street grids: downtown regular, suburbs coarser and warped.
    wx = 40.0 * fbm(X, Y, 1800.0, 2, seed=813) * (1.0 - down)
    wy = 40.0 * fbm(Y, X, 1800.0, 2, seed=814) * (1.0 - down)
    px = np.where(down > 0.5, 100.0, 170.0 + 50.0 * (1.0 - dens))
    py = np.where(down > 0.5, 120.0, 210.0 + 60.0 * (1.0 - dens))
    sx, ix = _grid_dist(X + wx, px, 0)
    sy, iy = _grid_dist(Y + wy, py, 0)
    street_w = np.where(down > 0.5, 9.0, 6.5)
    ax, iax = _grid_dist(X + wx, 800.0, 0)
    ay, iay = _grid_dist(Y + wy, 800.0, 0)
    art = (np.minimum(ax, ay) < 11.0) & (dens > 0.08)
    street = ((np.minimum(sx, sy) < street_w * 0.5) & (dens > 0.18 + 0.1 * n2)) | art
    # The river, its banks and bridges (on the arterials).
    rd = np.abs(X - river_x(Y))
    water = rd < 42.0 + 6.0 * n2
    bridge = water & (ay < 9.0)
    emb = (rd >= 42.0) & (rd < 60.0)
    # The freeway on the east bank, the rail yard north of downtown.
    hd = np.abs(X - highway_x(Y))
    freeway = hd < 14.0
    rail = (np.abs(X - (river_x(Y) + 300.0)) < 120.0) & (Y > 1800.0) & (Y < 3600.0)
    park = (fbm(X, Y, 900.0, 3, seed=815) > 0.38) & ~street & ~water
    block = (dens > 0.12 + 0.1 * n2) & ~street & ~water & ~freeway & ~park
    # Day colors.
    lin = _lin
    rid = hash01(np.floor((X + wx) / px * 3.0).astype(np.int64), np.floor((Y + wy) / py * 2.0).astype(np.int64), 816)
    roof = np.where((rid < 0.35)[:, None], lin(0x5a5a5c)[None, :], np.where((rid < 0.6)[:, None], lin(0x4a3e36)[None, :],
                    np.where((rid < 0.8)[:, None], lin(0x7a7672)[None, :], lin(0x3a3838)[None, :])))
    roof = roof * (0.8 + 0.4 * (0.5 + 0.5 * n3))[:, None]
    yards = (dens < 0.45) & (n3 > 0.15)
    trees_c = np.where((n2 > 0.1)[:, None], lin(0x7a5a1e)[None, :], lin(0x2a3320)[None, :])
    col = np.broadcast_to(lin(0x5a5034), (N, 3)).copy()                            # the floor's grass
    col = np.where(block[:, None], roof, col)
    col = np.where((block & yards)[:, None], trees_c, col)
    col = np.where(park[:, None], lin(0x3a4026)[None, :] * (0.8 + 0.4 * (0.5 + 0.5 * n2))[:, None], col)
    col = np.where(street[:, None], lin(0x2c2c2e)[None, :], col)
    col = np.where(rail[:, None], lin(0x4a4038)[None, :] * (0.85 + 0.3 * (0.5 + 0.5 * np.sin(Y * 1.3)))[:, None], col)
    col = np.where(freeway[:, None], lin(0x333436)[None, :], col)
    col = np.where(water[:, None], lin(0x0f1a1e)[None, :], col)
    col = np.where(emb[:, None], lin(0x6a6660)[None, :], col)
    col = np.where(bridge[:, None], lin(0x5a5856)[None, :], col)
    rough = np.where(water, 0.08, 0.85).astype(np.float32)
    canopy = np.where(block, 3.0 + 9.0 * down * rid, 0.0) + np.where(park | (block & yards), 8.0, 0.0)
    # Night: street lights at 35 m along every street (dots), the side's color by district.
    em = np.zeros((N, 3), dtype=np.float32)
    led = (hash01(ix // 6, iy // 6, 817) < 0.2 + 0.5 * down)
    lamp_c = np.where(led[:, None], lin(0xfff0dc)[None, :], lin(0xff9a3c)[None, :])
    along_x = np.abs(((Y + wy) % 35.0) - 17.5)
    along_y = np.abs(((X + wx) % 35.0) - 17.5)
    soft = np.maximum(texel, 1.5)
    dot = np.maximum(np.exp(-(along_x / soft) ** 2) * (sx < street_w * 0.5 + soft),
                     np.exp(-(along_y / soft) ** 2) * (sy < street_w * 0.5 + soft))
    # Residential streets are dim and broken (lamps through the yards' trees, whole stretches dark); downtown's and
    # the commercial blocks' are bright with a glow pool on the pavement.
    resid = 1.0 - smoothstep(0.4, 0.7, dens)
    pool = np.exp(-(np.minimum(sx, sy) / (street_w * 0.6 + soft)) ** 2) * (0.045 * (1.0 - 0.7 * resid))
    seg = np.where(sx < sy, hash01(ix, np.floor((Y + wy) / 140.0).astype(np.int64), 821), hash01(np.floor((X + wx) / 140.0).astype(np.int64), iy, 822))
    gap = 0.12 + 0.2 * resid
    lamps = (np.maximum(dot, pool) * (street | (np.minimum(sx, sy) < street_w + soft)) * (dens > 0.16)) * (0.35 + 0.65 * seg) * (seg > gap)
    lamps = lamps * (1.0 - 0.45 * resid * (block & yards | (n2 > 0.2)))
    em += lamp_c * (lamps * (0.42 + 0.2 * down) * (1.0 - 0.35 * resid))[:, None]
    # Commercial strips along the arterials: lots and signs, blobs of white LED and warm storefronts.
    near_art = (np.minimum(ax, ay) < 70.0) & (np.minimum(ax, ay) > 14.0) & (dens > 0.2) & ~water
    lot_id = hash01(np.floor((X + wx) / 45.0).astype(np.int64), np.floor((Y + wy) / 45.0).astype(np.int64), 823)
    lots = near_art & (lot_id < 0.22)
    lot_c = np.where((lot_id < 0.12)[:, None], lin(0xf4f6ff)[None, :], lin(0xffc27a)[None, :])
    em += lot_c * (lots * (0.18 + 0.22 * hash01(np.floor(X / 9.0).astype(np.int64), np.floor(Y / 9.0).astype(np.int64), 824)))[:, None]
    a25x = np.abs(((Y + wy) % 25.0) - 12.5)
    a25y = np.abs(((X + wx) % 25.0) - 12.5)
    artl = art * np.maximum(np.exp(-(a25x / soft) ** 2) * (ax < 11.0), np.exp(-(a25y / soft) ** 2) * (ay < 11.0))
    artl = np.maximum(artl, art * 0.07)
    em += lin(0xffb060)[None, :] * (artl * 1.0)[:, None]
    # Lit windows downtown and in the denser blocks: scattered texels, warm and cool.
    wnd = block & (hash01(np.floor(X / 3.0).astype(np.int64), np.floor(Y / 3.0).astype(np.int64), 818) < 0.012 + 0.09 * down)
    wc = np.where((hash01(np.floor(X / 9.0).astype(np.int64), np.floor(Y / 9.0).astype(np.int64), 819) < 0.6)[:, None], lin(0xffc890)[None, :], lin(0xdfe8ff)[None, :])
    em += wc * (wnd * (0.2 + 0.25 * down))[:, None]
    # The freeway: white headlights northbound, red tails southbound, sodium masts every 60 m.
    xl = X - highway_x(Y)
    lane_w = (np.abs(xl + 5.0) < 2.5) & freeway
    lane_r = (np.abs(xl - 5.0) < 2.5) & freeway
    traffic = 0.5 + 0.5 * np.sin(Y / 23.0 + 3.0 * fbm(Y, X, 400.0, 2, seed=820))
    em += lin(0xfff4e6)[None, :] * (lane_w * traffic ** 3 * 0.6)[:, None]
    em += lin(0xff2a18)[None, :] * (lane_r * traffic ** 3 * 0.5)[:, None]
    mast = freeway & (np.abs((Y % 60.0) - 30.0) < soft) & (np.abs(np.abs(xl) - 12.0) < soft)
    em += lin(0xffa040)[None, :] * mast[:, None] * 1.0
    # The bridges' strings of lights, lights along the embankments; the river stays dark.
    em += lin(0xffe0b0)[None, :] * (bridge & (np.abs(((X - river_x(Y)) % 12.0) - 6.0) < soft) & (np.abs(ay - 8.0) < soft))[:, None] * 1.0
    em += lin(0xffd090)[None, :] * (emb & (np.abs((Y % 30.0) - 15.0) < soft))[:, None] * 0.8
    em = np.where(water[:, None], 0.0, em)
    # Fade everything out with the city; whole neighborhoods brighter or dimmer than the next.
    em = em * (smoothstep(0.06, 0.25, dens) * (0.7 + 0.6 * (0.5 + 0.5 * fbm(X, Y, 1400.0, 3, seed=825))))[:, None]
    return np.clip(col, 0, 1).astype(np.float32), rough, np.clip(em, 0, 1).astype(np.float32), canopy.astype(np.float32)


TOWERS = None


def towers():
    """Downtown's towers: (x, y, w, d, h, setback) map m (footprint center, width X, depth Y, height, setback height)."""
    global TOWERS
    if TOWERS is None:
        rng = random.Random(8800)
        out = []
        for i in range(44):
            for _ in range(40):
                x = CITY_C[0] + rng.gauss(0.0, 520.0)
                y = CITY_C[1] + rng.gauss(0.0, 700.0)
                if abs(x - float(river_x(np.array([y]))[0])) > 120.0 and all(math.hypot(x - o[0], y - o[1]) > 70.0 for o in out):
                    break
            d0 = math.exp(-((x - CITY_C[0]) / 600.0) ** 2 - ((y - CITY_C[1]) / 800.0) ** 2)
            h = 45.0 + 150.0 * d0 * rng.uniform(0.5, 1.1) + rng.uniform(0, 30)
            w = rng.uniform(24.0, 46.0)
            dd = rng.uniform(24.0, 46.0)
            sb = h * rng.uniform(0.55, 0.8) if h > 90 and rng.random() < 0.5 else 0.0
            out.append((x, y, w, dd, h, sb))
        TOWERS = out
    return TOWERS


def facade_maps(width, rows):
    """The towers' walls laid along u (each tower's four walls, then the next tower), v up the height (0..1 of
    200 m): glass and mullions by day; by night windows lit floor by floor, a few whole floors of offices, aviation
    lights on the tallest. Returns (albedo, emissive, the u span per tower wall)."""
    T = towers()
    perim = [2 * (t[2] + t[3]) for t in T]
    total = sum(perim)
    alb = np.zeros((rows, width, 3), dtype=np.float32)
    em = np.zeros((rows, width, 3), dtype=np.float32)
    spans = []
    u0 = 0.0
    zz = (np.arange(rows) + 0.5) / rows * 200.0
    rng = random.Random(8801)
    for ti, (t, p) in enumerate(zip(T, perim)):
        x, y, w, d, h, sb = t
        walls = []
        for wi, L in enumerate((w, d, w, d)):
            uw = L / total
            walls.append((u0, u0 + uw))
            c0, c1 = int(u0 * width), max(int((u0 + uw) * width), int(u0 * width) + 1)
            cols = np.arange(c0, c1)
            m = (cols - c0) / max(c1 - c0, 1) * L                    # meters along the wall
            Mz, Mm = np.meshgrid(zz, m, indexing='ij')
            floor = np.floor(Mz / 3.6).astype(np.int64)
            bay = np.floor(Mm / 1.6).astype(np.int64)
            glass = rng.choice([0x2a3a4a, 0x34404a, 0x3c4450, 0x283038])
            base = _lin(glass)[None, None, :] * np.ones_like(Mz)[..., None]
            mull = ((Mm % 1.6) < 0.18) | ((Mz % 3.6) < 0.5)
            base = np.where(mull[..., None], _lin(0x6a6c70)[None, None, :], base)
            above = Mz > h
            base = np.where(above[..., None], 0.0, base)
            alb[:, c0:c1] = base
            lit_floor = hash01(floor + ti * 97, np.int64(wi), 8802) < 0.08
            lit = (hash01(floor * 131 + bay, np.int64(ti * 4 + wi), 8803) < 0.16) | lit_floor
            warm = hash01(floor, np.int64(ti), 8804) < 0.55
            wc = np.where(warm[..., None], _lin(0xffd4a0)[None, None, :], _lin(0xe6eeff)[None, None, :])
            e = wc * (lit & ~mull & ~above & (Mz > 4.0))[..., None] * 0.6
            if h > 150 and wi == 0:
                top = (np.abs(Mz - (h - 1.5)) < 1.2) & (np.abs(Mm - L / 2) < 1.2)
                e = np.where(top[..., None], _lin(0xff2010)[None, None, :], e)
            em[:, c0:c1] = e
            u0 += uw
        spans.append(walls)
    return alb, em, spans


def build_city(size=4096):
    """SM_ECX_ValleyCity: the city's sector of the valley floor (polar mesh, three bands of texture) and the towers,
    their walls in the texture's top strip. [lit]: the night lights in the emissive."""
    t0 = time.time()
    name = 'SM_ECX_ValleyCity'
    spec = POLAR[name]
    tex = size // 4 if FAST else size
    frows = FACADE_ROWS * tex // 4096
    th, r, F = polar_mesh(spec)
    X = r * np.cos(th)
    Y = r * np.sin(th)
    Z = world_height(X, Y, lam=80.0)
    P = np.column_stack([X, Y, Z])
    a, b, c = P[F[:, 0]], P[F[:, 1]], P[F[:, 2]]
    cz = (b[:, 0] - a[:, 0]) * (c[:, 1] - a[:, 1]) - (b[:, 1] - a[:, 1]) * (c[:, 0] - a[:, 0])
    F[cz < 0] = F[cz < 0][:, ::-1]
    # Bands in the rows below the facade strip.
    nb = len(spec['bands'])
    usable = (tex - frows) / tex
    bands = []
    for k, (ta, tb_) in enumerate(spec['bands']):
        tt = np.linspace(ta, tb_, 64)
        g = 24.0 * tex / 4096.0
        bands.append(RadialMap(ta, tb_, float(np.min(r_city0(tt))), float(np.max(r_city1(tt))), usable * k / nb, usable * (k + 1) / nb,
                               g / tex, g * nb / (tex * usable), lambda Xx, Yy: natural_height(Xx, Yy, 60.0)))
    fth = np.angle(np.exp(1j * th[F]).mean(axis=1))
    fb = band_of(fth, spec['bands'])
    UV = np.zeros((len(F), 3, 2), dtype=np.float32)
    for k, bm in enumerate(bands):
        sel = fb == k
        uu, vv = bm.uv(th[F[sel]], r[F[sel]])
        UV[sel, :, 0] = uu
        UV[sel, :, 1] = vv
    # The towers: boxes (and a setback tier) standing on the floor; walls -> the facade strip, roofs -> a dark corner.
    _, _, spans = facade_maps(64, 8)
    v0f = (tex - frows) / tex
    Pl = [P]
    Fl = [F]
    UVl = [UV]
    n = len(P)
    for (x, y, w, d, h, sb), walls in zip(towers(), spans):
        zg = float(world_height(np.array([x]), np.array([y]), 80.0)[0]) - 1.0
        tiers = [(w, d, zg, zg + (sb if sb else h)), (w * 0.7, d * 0.7, zg + sb, zg + h)] if sb else [(w, d, zg, zg + h)]
        for (tw, td, z0, z1) in tiers:
            corners = [(x - tw / 2, y - td / 2), (x + tw / 2, y - td / 2), (x + tw / 2, y + td / 2), (x - tw / 2, y + td / 2)]
            for wi in range(4):
                (x0, y0), (x1, y1) = corners[wi], corners[(wi + 1) % 4]
                ua, ub = walls[wi]
                va, vb = v0f + (z0 - zg) / 200.0 * (1 - v0f), v0f + (z1 - zg) / 200.0 * (1 - v0f)
                Pl.append(np.array([(x0, y0, z0), (x1, y1, z0), (x1, y1, z1), (x0, y0, z1)]))
                # Outward: the corners run counter-clockwise from above, so (a, b, top b) faces out.
                Fl.append(np.array([(n, n + 1, n + 2), (n, n + 2, n + 3)]))
                UVl.append(np.array([[(ua, va), (ub, va), (ub, vb)], [(ua, va), (ub, vb), (ua, vb)]], dtype=np.float32))
                n += 4
            Pl.append(np.array([(cx, cy, z1) for cx, cy in corners]))
            Fl.append(np.array([(n, n + 1, n + 2), (n, n + 2, n + 3)]))
            ru = np.array([[(0.001, v0f + 0.001), (0.004, v0f + 0.001), (0.004, v0f + 0.004)], [(0.001, v0f + 0.001), (0.004, v0f + 0.004), (0.001, v0f + 0.004)]], dtype=np.float32)
            UVl.append(ru)
            n += 4
    P2 = np.vstack(Pl)
    F2 = np.vstack(Fl)
    UV2 = np.concatenate(UVl)
    obj = make_mesh(name, P2, F2, uv=UV2)
    # Textures: the floor's bands, then the facade strip.
    base = np.zeros((tex, tex, 3), dtype=np.float32)
    orm = np.zeros((tex, tex, 3), dtype=np.float32)
    nmap = np.zeros((tex, tex, 3), dtype=np.float32)
    emis = np.zeros((tex, tex, 3), dtype=np.float32)
    for k, bm in enumerate(bands):
        r0, r1 = int(round(bm.v0 * tex)), int(round(bm.v1 * tex))
        u = (np.arange(tex) + 0.5) / tex
        v = (np.arange(r0, r1) + 0.5) / tex
        U, V = np.meshgrid(u, v)
        TH, R = bm.inverse(U, V)
        Xt = R * np.cos(TH)
        Yt = R * np.sin(TH)
        dth = (bm.th1 - bm.th0) / ((1 - 2 * bm.gu) * tex)
        texel = np.maximum(R * dth, np.abs(np.gradient(R, axis=0))).astype(np.float32)
        col, rough, em, canopy = city_layers(Xt.ravel(), Yt.ravel(), texel.ravel())
        Hf = world_height(Xt.ravel(), Yt.ravel(), lam=np.maximum(2 * texel.ravel(), 1.0)) + canopy * 0.5
        Hf = Hf.reshape(Xt.shape)
        Pp = np.stack([Xt, Yt, Hf.astype(np.float64)], axis=2)
        Pu, Pv = np.gradient(Pp, axis=1), np.gradient(Pp, axis=0)
        nf = np.cross(Pu, Pv)
        nf *= np.sign(nf[:, :, 2:3] + 1e-12)
        nf /= np.linalg.norm(nf, axis=2, keepdims=True)
        T = Pu / np.linalg.norm(Pu, axis=2, keepdims=True)
        nc = np.array([0.0, 0.0, 1.0])
        T = T - T[:, :, 2:3] * nc
        T /= np.linalg.norm(T, axis=2, keepdims=True)
        B = Pv - np.sum(Pv * T, axis=2, keepdims=True) * T
        B[:, :, 2] = 0.0
        B /= np.linalg.norm(B, axis=2, keepdims=True)
        ts = np.stack([np.sum(nf * T, axis=2), np.sum(nf * B, axis=2), nf[:, :, 2]], axis=2)
        nmap[r0:r1] = ts * 0.5 + 0.5
        base[r0:r1] = col.reshape(Xt.shape + (3,))
        orm[r0:r1, :, 0] = np.clip(1.0 - (blur2(Hf, 2) - Hf).clip(0) / 8.0, 0.5, 1.0)
        orm[r0:r1, :, 1] = rough.reshape(Xt.shape)
        emis[r0:r1] = em.reshape(Xt.shape + (3,))
    falb, fem, _ = facade_maps(tex, frows)
    base[tex - frows:] = falb
    emis[tex - frows:] = fem
    nmap[tex - frows:] = (0.5, 0.5, 1.0)
    orm[tex - frows:, :, 0] = 1.0
    orm[tex - frows:, :, 1] = 0.25
    base[tex - frows:tex - frows + max(2, tex // 256), :max(2, tex // 256)] = _lin(0x2a2a2c)
    tex_dir = os.path.join(core.BUILD_DIR, 'textures', name)
    paths = {}
    for key, arr, srgb in (('BaseColor', base, True), ('ORM', orm, False), ('Normal', nmap, False), ('Emissive', emis, True)):
        paths[key] = save_png(os.path.join(tex_dir, f'T_{name}_{key}.png'), arr, srgb=srgb)
    mat = core.gltf_material(f'M_{name}', paths['BaseColor'], paths['ORM'], paths['Normal'], paths['Emissive'], CITY_GLOW)
    mat.use_backface_culling = True
    core.assign(obj, mat)
    obj.data.calc_loop_triangles()
    log(f'{name}: {len(obj.data.loop_triangles)} triangles, {len(towers())} towers, {time.time() - t0:.0f}s')
    return obj


CHALET_ROWS = 128                   # the top rows of Ridge_Near's texture: the hamlet's chalets


CHALET_SIZE = (9.0, 12.0, 5.2)      # width (across the gable), length, wall height (m)


WIN_U = (0.36, 0.64)                # where the windows sit on a wall (fraction across it) ...


WIN_V = ((0.22, 0.46), (0.6, 0.84))  # ... and up it (ground floor, upper floor)


_CHALETS = None


def chalets():
    """The hamlet on the north bench: (x, y, z base, yaw) map m / radians, gables facing down the valley (west)."""
    global _CHALETS
    if _CHALETS is None:
        rng = random.Random(6001)
        out = []
        tries = 0
        while len(out) < 10 and tries < 4000:
            tries += 1
            x = rng.uniform(-170.0, 120.0)
            y = rng.uniform(480.0, 880.0)
            if any(math.hypot(x - o[0], y - o[1]) < 38.0 for o in out):
                continue
            d, _, _ = raster().sample(np.array([x]), np.array([y]))
            if d[0] < 14.0:
                continue
            if x > float(edge_x(np.array([y]))[0]) - 20.0:
                continue
            cs = [(x + a, y + b) for a in (-7, 7) for b in (-8, 8)]
            zs = world_height(np.array([c[0] for c in cs]), np.array([c[1] for c in cs]), 4.0)
            if zs.max() - zs.min() > 5.0:
                continue
            out.append((x, y, float(zs.min()) - 0.4, rng.uniform(-0.25, 0.25)))
        _CHALETS = out
    return _CHALETS


def chalet_geometry(v0):
    """The chalets' faces (map frame) with UVs in Ridge_Near's top strip (rows from v0 to 1): walls left half of the
    strip (gable walls in its first quarter, long walls the second), the roof right half. Returns (P, F, UV)."""
    W, L, Hw = CHALET_SIZE
    P, F, UV = [], [], []
    hv = lambda a: v0 + (1.0 - v0) * a  # noqa: E731
    ridge = Hw + (W / 2) * math.tan(math.radians(33.0))
    for (x, y, z, yaw) in chalets():
        c, s = math.cos(yaw), math.sin(yaw)
        def tr(px, py, pz):
            return (x + px * c - py * s, y + px * s + py * c, z + pz)
        n = len(P)
        # Walls: corners counter-clockwise from above; long walls along y, gables at +-x (west and east).
        cr = [(-W / 2, -L / 2), (W / 2, -L / 2), (W / 2, L / 2), (-W / 2, L / 2)]
        for i in range(4):
            (ax, ay), (bx, by) = cr[i], cr[(i + 1) % 4]
            gable = abs(ax - bx) < 1e-6
            u0, u1 = (0.0, 0.25) if gable else (0.25, 0.5)
            k = len(P)
            P += [tr(ax, ay, 0.0), tr(bx, by, 0.0), tr(bx, by, Hw), tr(ax, ay, Hw)]
            F += [(k, k + 1, k + 2), (k, k + 2, k + 3)]
            UV += [[(u0, hv(0.0)), (u1, hv(0.0)), (u1, hv(0.62))], [(u0, hv(0.0)), (u1, hv(0.62)), (u0, hv(0.62))]]
            if gable:
                k = len(P)
                P += [tr(ax, ay, Hw), tr(bx, by, Hw), tr(0.5 * (ax + bx) * 0 + ax, 0.0, ridge)]
                F += [(k, k + 1, k + 2)]
                UV += [[(u0, hv(0.62)), (u1, hv(0.62)), ((u0 + u1) / 2, hv(0.98))]]
        # Roof: two pitches with an overhang.
        o = 0.8
        for sgn in (-1, 1):
            k = len(P)
            P += [tr(sgn * (W / 2 + o), -L / 2 - o, Hw - o * 0.65), tr(0.0, -L / 2 - o, ridge + 0.15), tr(0.0, L / 2 + o, ridge + 0.15),
                  tr(sgn * (W / 2 + o), L / 2 + o, Hw - o * 0.65)]
            if sgn < 0:
                F += [(k, k + 1, k + 2), (k, k + 2, k + 3)]
            else:
                F += [(k, k + 3, k + 2), (k, k + 2, k + 1)]
            UV += [[(0.52, hv(0.05)), (0.98, hv(0.95)), (0.98, hv(0.05))], [(0.52, hv(0.05)), (0.52, hv(0.95)), (0.98, hv(0.95))]] if sgn < 0 else \
                [[(0.52, hv(0.05)), (0.98, hv(0.05)), (0.98, hv(0.95))], [(0.52, hv(0.05)), (0.98, hv(0.95)), (0.52, hv(0.95))]]
    return np.asarray(P, dtype=np.float64), np.asarray(F, dtype=np.int64), np.asarray(UV, dtype=np.float32)


def chalet_strip(width, rows):
    """The strip's maps: dark larch boards, a stone base, windows with white frames (dark glass by day) on the walls;
    snow on the roof with a dark eave. Returns (albedo, orm, normal)."""
    U, V = np.meshgrid((np.arange(width) + 0.5) / width, (np.arange(rows) + 0.5) / rows)
    alb = np.zeros((rows, width, 3), dtype=np.float32)
    wall = U < 0.5
    uw = np.where(U < 0.25, U / 0.25, (U - 0.25) / 0.25)
    board = 0.85 + 0.15 * np.sin(uw * 2 * math.pi * 24.0) + 0.06 * hash01(np.floor(uw * 24).astype(np.int64), np.int64(1), 6002)
    wood = _lin(0x3e2b1e)[None, None, :] * board[..., None]
    vv = V / 0.62
    stone = vv < 0.12
    alb = np.where(wall[..., None], np.where(stone[..., None], _lin(0x6a6660)[None, None, :], wood), alb)
    for (va, vb) in WIN_V:
        win = wall & (uw > WIN_U[0]) & (uw < WIN_U[1]) & (vv > va) & (vv < vb)
        frame = wall & (uw > WIN_U[0] - 0.03) & (uw < WIN_U[1] + 0.03) & (vv > va - 0.03) & (vv < vb + 0.03) & ~win
        alb = np.where(win[..., None], _lin(0x101418)[None, None, :], alb)
        alb = np.where(frame[..., None], _lin(0xd8d2c4)[None, None, :], alb)
    roof = ~wall
    alb = np.where(roof[..., None], _lin(0xe4eaf2)[None, None, :] * (0.95 + 0.05 * pfbm(U, V, 16, 4, 3, seed=6003))[..., None], alb)
    eave = roof & ((V < 0.12) | (V > 0.9))
    alb = np.where(eave[..., None], _lin(0x2a2420)[None, None, :], alb)
    orm = np.stack([np.ones_like(U), np.where(roof, 0.6, 0.8), np.zeros_like(U)], axis=2).astype(np.float32)
    nrm = np.zeros((rows, width, 3), dtype=np.float32) + np.array([0.5, 0.5, 1.0], dtype=np.float32)
    return alb.astype(np.float32), orm, nrm


def build_mist():
    """SM_ECX_ValleyMist: three broad, gently domed sheets over the valley (Z -780..-620) and two banks along the slope
    below the parapet (Z -160..-60), each UV 0..1 for the material's edge fade."""
    sheets = [((7800.0, -1800.0), 9000.0, 12500.0, 0.25, -735.0, 22), ((11500.0, 3800.0), 8000.0, 10000.0, -0.3, -680.0, 22),
              ((5600.0, 5600.0), 6000.0, 8000.0, 0.5, -630.0, 18), ((215.0, -350.0), 380.0, 1300.0, 0.0, -112.0, 10),
              ((190.0, 820.0), 320.0, 1000.0, 0.15, -78.0, 9)]
    P, F, UV = [], [], []
    for (cx, cy), w, l, rot, z, n in sheets:
        nu, nv = (n, n) if w > 2000 else (max(4, n // 3), n * 2)
        c, s = math.cos(rot), math.sin(rot)
        base = len(P)
        for j in range(nv + 1):
            for i in range(nu + 1):
                u, v = i / nu, j / nv
                lx, ly = (u - 0.5) * w, (v - 0.5) * l
                x = cx + lx * c - ly * s
                y = cy + lx * s + ly * c
                dome = (1.0 - (2 * u - 1) ** 2) * (1.0 - (2 * v - 1) ** 2)
                zz = z + (25.0 if w > 2000 else 6.0) * dome + (12.0 if w > 2000 else 3.0) * float(fbm(np.array([x]), np.array([y]), w * 0.4, 2, seed=901)[0])
                P.append((x, y, zz))
                UV.append((u, v))
        for j in range(nv):
            for i in range(nu):
                a = base + j * (nu + 1) + i
                F += [(a, a + 1, a + nu + 2), (a, a + nu + 2, a + nu + 1)]
    P = np.asarray(P)
    F = np.asarray(F, dtype=np.int64)
    obj = make_mesh('SM_ECX_ValleyMist', P, F, uv=np.asarray(UV, dtype=np.float32))
    m = core.Mat('ecx_mist')
    m.set('Base Color', (0.82, 0.85, 0.9, 1.0))
    m.set('Roughness', 1.0)
    m.set('Alpha', 0.35)
    m.m.use_backface_culling = False
    core.assign(obj, m)
    log(f'SM_ECX_ValleyMist: {len(F)} triangles')
    return obj


def _octa(c, r):
    P = [(c[0] + r, c[1], c[2]), (c[0] - r, c[1], c[2]), (c[0], c[1] + r, c[2]), (c[0], c[1] - r, c[2]), (c[0], c[1], c[2] + r), (c[0], c[1], c[2] - r)]
    F = [(0, 2, 4), (2, 1, 4), (1, 3, 4), (3, 0, 4), (2, 0, 5), (1, 2, 5), (3, 1, 5), (0, 3, 5)]
    return P, F


def _lamp_head(c, heading):
    """A cobra-head street lamp's glowing body (map frame): a flattened, tapered lozenge, its lens below."""
    P, F = [], []
    n_a, n_r = 14, 8
    ch, sh = math.cos(heading), math.sin(heading)
    for i in range(n_r + 1):
        t = i / n_r
        L = 0.75
        ax = (t - 0.5) * L
        rw = 0.24 * math.sin(math.pi * (0.08 + 0.88 * t)) + 0.02
        rh = 0.09 * math.sin(math.pi * (0.08 + 0.88 * t)) + 0.01
        for k in range(n_a):
            a = 2 * math.pi * k / n_a
            ly = rw * math.cos(a)
            lz = rh * math.sin(a) - (0.04 if math.sin(a) < 0 else 0.0)
            P.append((c[0] + ax * ch - ly * sh, c[1] + ax * sh + ly * ch, c[2] + lz))
    for i in range(n_r):
        for k in range(n_a):
            a = i * n_a + k
            b = i * n_a + (k + 1) % n_a
            F += [(a, b, b + n_a), (a, b + n_a, a + n_a)]
    for cap, rev in ((0, True), (n_r, False)):
        ring = [cap * n_a + k for k in range(n_a)]
        for k in range(1, n_a - 1):
            F.append((ring[0], ring[k + 1], ring[k]) if rev else (ring[0], ring[k], ring[k + 1]))
    return P, F


def build_slope_lights():
    """SM_ECX_SlopeLights_GlowWarm: the switchbacks' lamps (a lamp every 60 m; the 10 nearest the plaza real cobra
    heads, the rest 1.2 m octahedra that read from every side), the hamlet's windows (just proud of the chalets'
    walls, where their frames are), and the lit run's floodlights on the north slope."""
    P, F = [], []

    def add(pp, ff):
        b = len(P)
        P.extend(pp)
        F.extend([tuple(b + i for i in f) for f in ff])
    pts, heads = lamp_points()
    d = np.hypot(pts[:, 0] - EYE[0], pts[:, 1] - EYE[1])
    near = set(np.argsort(d)[:10].tolist())
    for i, (p, h) in enumerate(zip(pts, heads)):
        if i in near:
            add(*_lamp_head(p, h + math.pi / 2))
        else:
            add(*_octa(p, 0.6))
    # Chalet windows.
    W, L, Hw = CHALET_SIZE
    nwin = 0
    for (x, y, z, yaw) in chalets():
        c, s = math.cos(yaw), math.sin(yaw)
        cr = [(-W / 2, -L / 2), (W / 2, -L / 2), (W / 2, L / 2), (-W / 2, L / 2)]
        for i in range(4):
            (ax, ay), (bx, by) = cr[i], cr[(i + 1) % 4]
            ex, ey = bx - ax, by - ay
            ln = math.hypot(ex, ey)
            nx, ny = ey / ln, -ex / ln        # outward (right of a counter-clockwise edge)
            for fi, (va, vb) in enumerate(WIN_V):
                if (i + fi) % 3 == 2:          # not every window is lit
                    continue
                q = []
                for (uu, vz) in ((WIN_U[0], va), (WIN_U[1], va), (WIN_U[1], vb), (WIN_U[0], vb)):
                    lx = ax + ex * uu + nx * 0.06
                    ly = ay + ey * uu + ny * 0.06
                    q.append((x + lx * c - ly * s, y + lx * s + ly * c, z + vz * Hw))
                add(q, [(0, 1, 2), (0, 2, 3)])
                nwin += 1
    # The lit run: floodlights each side of the run every 50 m, 10 m up.
    for t in np.arange(0.0, 1.0001, 50.0 / 940.0):
        x = -400.0 - 800.0 * t
        y = 900.0 + (x + 400.0) * -0.62
        for side in (-1.0, 1.0):
            px, py = x + side * 26.0 * 0.53, y + side * 26.0 * 0.85
            pz = float(world_height(np.array([px]), np.array([py]), 8.0)[0]) + 10.0
            add(*_octa((px, py, pz), 0.6))
    P = np.asarray(P, dtype=np.float64)
    F = np.asarray(F, dtype=np.int64)
    obj = make_mesh('SM_ECX_SlopeLights_GlowWarm', P, F)
    me = obj.data
    # make_mesh assumes faces counter-clockwise from above; these are closed solids and quads: fix them outward.
    bm = bmesh.new()
    bm.from_mesh(me)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    bm.to_mesh(me)
    bm.free()
    obj = parts.glow_object([obj], 'SM_ECX_SlopeLights_GlowWarm')
    log(f'SM_ECX_SlopeLights_GlowWarm: {len(F)} triangles, {len(pts)} lamps, {nwin} windows')
    return obj


TERRAIN = ['SM_ECX_SiteTerrain', 'SM_ECX_Ridge_Near', 'SM_ECX_Ridge_Mid', 'SM_ECX_Ridge_Far', 'SM_ECX_Valley', 'SM_ECX_ValleyCity', 'SM_ECX_ValleyMist',
           'SM_ECX_SlopeLights_GlowWarm']


BAKED = ['SM_ECX_ForestClump_A', 'SM_ECX_ForestClump_B', 'SM_ECX_Rock_A', 'SM_ECX_Rock_B', 'SM_ECX_Rock_C']


def build():
    t0 = time.time()
    core.reset()
    strips = write_textures()
    core.reset()
    objs = []
    tex = 4096
    builders = [
        ('SM_ECX_SiteTerrain', build_site_terrain),
        ('SM_ECX_Ridge_Near', lambda: build_polar_terrain('SM_ECX_Ridge_Near', tex)),
        ('SM_ECX_Ridge_Mid', lambda: build_polar_terrain('SM_ECX_Ridge_Mid', tex)),
        ('SM_ECX_Ridge_Far', lambda: build_polar_terrain('SM_ECX_Ridge_Far', tex)),
        ('SM_ECX_Valley', lambda: build_polar_terrain('SM_ECX_Valley', tex)),
        ('SM_ECX_ValleyCity', lambda: build_city(tex)),
        ('SM_ECX_ValleyMist', build_mist),
        ('SM_ECX_SlopeLights_GlowWarm', build_slope_lights),
    ]
    for name, fn in builders:
        if wanted(name):
            objs.append(fn())
    mats = tree_materials()
    trees = [('SM_ECX_Spruce_Tall', lambda: build_spruce('SM_ECX_Spruce_Tall', strips, 20.0, 3.2, 1.2, 0.42, 1.0, 0.75, 11, mats=mats)),
             ('SM_ECX_Spruce_Mid', lambda: build_spruce('SM_ECX_Spruce_Mid', strips, 12.0, 2.75, 0.7, 0.38, 0.95, 0.7, 12, mats=mats)),
             ('SM_ECX_Spruce_Young', lambda: build_spruce('SM_ECX_Spruce_Young', strips, 4.5, 1.1, 0.25, 0.24, 0.62, 0.2, 13, young=True, mats=mats)),
             ('SM_ECX_Pine_Tall', lambda: build_pine('SM_ECX_Pine_Tall', strips, 17.0, mats))]
    for name, fn in trees:
        if wanted(name):
            objs.append(fn())
    for name, fn in (('SM_ECX_ForestClump_A', build_forest_clump_a), ('SM_ECX_ForestClump_B', build_forest_clump_b), ('SM_ECX_Rock_A', build_rock_a),
                     ('SM_ECX_Rock_B', build_rock_b), ('SM_ECX_Rock_C', build_rock_c)):
        if wanted(name):
            objs.append(fn())
    for o in objs:
        o.data.calc_loop_triangles()
        lo, hi = core.bounds([o])
        d = hi - lo
        log(f'{o.name}: {len(o.data.loop_triangles)} triangles, {d.x:.2f} x {d.y:.2f} x {d.z:.2f} m, slots {[s.material.name for s in o.material_slots]}')
    log(f'built in {time.time() - t0:.0f}s')
    return objs


def bake_parts(objs):
    # The terrain's textures are computed (their glTF materials are made already), the trees, the mist and the glow
    # are shaded by the game: only the clumps and the rocks bake.
    return [o for o in objs if o.name in BAKED]


# ---------------------------------------------------------------------- review


def _review_scatter():
    """Where the review puts the props (spec 2.5, 2.6): (mesh name, map X, Y, yaw radians, scale)."""
    rng = random.Random(77)
    out = []
    for x, y in ((29.5, 20.0), (29.5, -20.0), (42.5, 22.5), (42.5, -22.5), (4.2, 48.0), (4.2, -48.0)):
        out.append(('SM_ECX_Spruce_Mid', x, y, rng.random() * 6.28, 1.0))
    for x, y in ((49.9, 13.0), (49.9, 19.0), (49.9, -20.0), (4.6, 20.0), (4.6, -20.0), (20.0, 18.6), (20.0, -18.6)):
        out.append(('SM_ECX_Spruce_Young', x, y, rng.random() * 6.28, 1.0, 0.75))
    for x, y in ((44.5, 17.5), (38.0, 24.0)):
        out.append(('SM_ECX_Spruce_Young', x, y, rng.random() * 6.28, 1.0))
    pts = []
    tries = 0
    while len(pts) < 70 and tries < 20000:
        tries += 1
        if len(pts) < 40:
            if rng.random() < 0.8:
                x, y = rng.uniform(5.0, 52.0), rng.choice((-1, 1)) * rng.uniform(39.0, 90.0)
            else:
                x, y = rng.uniform(-30.0, 5.0), rng.choice((-1, 1)) * rng.uniform(60.0, 90.0)
        else:
            x = rng.uniform(40.0, 54.0)
            y = rng.uniform(-160.0, -90.0) if rng.random() < 0.5 else rng.uniform(60.0, 110.0)
        if any(math.hypot(x - p[0], y - p[1]) < 4.5 for p in pts):
            continue
        pts.append((x, y))
    for i, (x, y) in enumerate(pts):
        out.append(('SM_ECX_Spruce_Tall' if rng.random() < 0.6 else 'SM_ECX_Pine_Tall', x, y, rng.random() * 6.28, 0.85 + 0.3 * rng.random()))
    for x, y in ((3.0, -66.0), (-4.0, 63.0), (42.0, -165.0)):
        out.append(('SM_ECX_Rock_A', x, y, rng.random() * 6.28, 1.0))
    for x, y in ((39.5, 18.5), (76.0, -25.0), (74.0, 35.0)):
        out.append(('SM_ECX_Rock_B', x, y, rng.random() * 6.28, 1.0))
    for k in range(12):
        out.append(('SM_ECX_Rock_C', rng.uniform(70.0, 110.0), rng.uniform(-150.0, 120.0), rng.random() * 6.28, 0.8 + 0.4 * rng.random()))
    # Forest clumps on the slopes from 60 m to 1.5 km (the review's share of the game's 2,500), off the axis and the drop.
    k = 0
    while k < 700:
        a = rng.uniform(0.0, 2 * math.pi)
        r = 60.0 + 1440.0 * rng.random() ** 1.6
        x, y = 30.0 + r * math.cos(a), r * math.sin(a)
        if abs(y - AXIS_SLOPE * (x - 50.0)) < 40.0 and x < 60.0:
            continue
        if x > float(edge_x(np.array([y]))[0]) and rng.random() < 0.85:
            continue
        if inside_poly(np.array([x]), np.array([y]), [q for q, _ in PAD])[0] or (abs(y) < 60.0 and -70.0 < x < 75.0):
            continue
        z = world_height(np.array([x]), np.array([y]), 4.0)
        nz = world_height(np.array([x + 3.0, x - 3.0]), np.array([y, y]), 4.0)
        if abs(float(nz[0] - nz[1])) / 6.0 > 0.75:
            continue
        out.append(('SM_ECX_ForestClump_A' if rng.random() < 0.6 else 'SM_ECX_ForestClump_B', x, y, rng.random() * 6.28, 0.8 + 0.5 * rng.random()))
        k += 1
    return out


SITE_FRAME_GLBS = ['SM_ECX_Ground', 'SM_ECX_Entry', 'SM_ECX_Podium_Glass', 'SM_ECX_Lobby', 'SM_ECX_WingS', 'SM_ECX_WingN', 'SM_ECX_Canopy',
                   'SM_ECX_Tower', 'SM_ECX_Crown', 'SM_ECX_CrownCrest', 'SM_ECX_Marquee', 'SM_ECX_Cascade', 'SM_ECX_Podium_GlowEmber',
                   'SM_ECX_Tower_GlowEmber', 'SM_ECX_Crown_GlowEmber', 'SM_ECX_Lobby_GlowWarm', 'SM_ECX_Podium_GlowWarm', 'SM_ECX_Podium_GlowAsh']


def _import_context():
    """The other modules' SITE-frame meshes, if they have been exported: the building and the plaza, in place (their
    Blender frame is the map's, as the terrain's is)."""
    got = []
    # Our glTF output group (core.gltf_material's, with only an Occlusion input) would be reused by the importer and
    # break it: keep it out of the way while importing.
    ours = bpy.data.node_groups.get('glTF Material Output')
    if ours:
        ours.name = 'glTF Material Output (ecx)'
    for name in SITE_FRAME_GLBS:
        path = os.path.join(core.EXPORT_DIR, f'{name}.glb')
        if not os.path.exists(path):
            continue
        before = set(bpy.data.objects)
        try:
            bpy.ops.import_scene.gltf(filepath=path)
        except Exception as exc:  # a review aid only
            log(f'review: could not import {name}: {exc}')
            continue
        new = [o for o in bpy.data.objects if o not in before]
        for o in new:
            if o.type == 'MESH' and 'Glow' in name:
                for slot in o.material_slots:
                    m = slot.material
                    if m and m.use_nodes:
                        b = next((n for n in m.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None)
                        if b:
                            col = (1.0, 0.42, 0.14, 1.0) if 'Ember' in name else ((0.92, 0.9, 0.86, 1.0) if 'Ash' in name else (1.0, 0.8, 0.55, 1.0))
                            b.inputs['Emission Color'].default_value = col
                            b.inputs['Emission Strength'].default_value = 8.0
        got += new
    if ours:
        theirs = bpy.data.node_groups.get('glTF Material Output')
        if theirs:
            theirs.name = 'glTF Material Output (import)'
        ours.name = 'glTF Material Output'
    log(f'review: imported {len([o for o in got if o.type == "MESH"])} context meshes')
    return got


def _review_terrain_material(obj):
    """SiteTerrain for the review: the tiling snow and rock blended by COLOR_0, world-aligned (as M_ECX_Terrain will)."""
    m = core.Mat('ecx_terrain_review')
    tc, sep = object_coords(m)
    ca = m.node('ShaderNodeVertexColor')
    ca.layer_name = 'Color'
    cs = m.node('ShaderNodeSeparateColor')
    m.link(ca.outputs['Color'], cs.inputs['Color'])
    mapn = m.node('ShaderNodeMapping')
    m.link(tc.outputs['Object'], mapn.inputs['Vector'])
    mapn.inputs['Scale'].default_value = (0.25, 0.25, 0.25)
    sn = m.image(tex_path('T_ECX_Snow_BaseColor'))
    sn.projection = 'BOX'
    sn.projection_blend = 0.3
    m.link(mapn.outputs['Vector'], sn.inputs['Vector'])
    mp2 = m.node('ShaderNodeMapping')
    m.link(tc.outputs['Object'], mp2.inputs['Vector'])
    mp2.inputs['Scale'].default_value = (1 / 6.0, 1 / 6.0, 1 / 6.0)
    rk = m.image(tex_path('T_ECX_Rock_BaseColor'))
    rk.projection = 'BOX'
    rk.projection_blend = 0.3
    m.link(mp2.outputs['Vector'], rk.inputs['Vector'])
    rmask = ramp(m, cs.outputs['Red'], 0.3, 0.6)
    col = m.mix(rmask, sn.outputs['Color'], rk.outputs['Color'])
    col = m.mix(m.math('MULTIPLY', cs.outputs['Green'], 0.3), col, core.hex_linear(0x7d7466))
    m.set('Base Color', col)
    m.set('Roughness', 0.65)
    core.assign(obj, m)


def _review_mist_material(obj):
    m = core.Mat('ecx_mist_review')
    uvn = m.node('ShaderNodeUVMap')
    sep = m.node('ShaderNodeSeparateXYZ')
    m.link(uvn.outputs['UV'], sep.inputs['Vector'])
    fu = m.math('MULTIPLY', ramp(m, sep.outputs['X'], 0.0, 0.25), ramp(m, sep.outputs['X'], 1.0, 0.75))
    fv = m.math('MULTIPLY', ramp(m, sep.outputs['Y'], 0.0, 0.25), ramp(m, sep.outputs['Y'], 1.0, 0.75))
    tc, _ = object_coords(m)
    nz = ramp(m, snoise(m, tc.outputs['Object'], scale=0.0015, detail=4.0), 0.4, 0.75)
    a = m.math('MULTIPLY', m.math('MULTIPLY', fu, fv), m.math('MULTIPLY', nz, 0.55))
    m.set('Base Color', (0.85, 0.88, 0.93, 1.0))
    m.set('Roughness', 1.0)
    m.link(a, m.bsdf.inputs['Alpha'])
    core.assign(obj, m)


def _set_sun(sc, el, az_compass, energy, color, sky_strength, night=False):
    world = sc.world
    world.use_nodes = True
    nt = world.node_tree
    for n in list(nt.nodes):
        nt.nodes.remove(n)
    out = nt.nodes.new('ShaderNodeOutputWorld')
    bg = nt.nodes.new('ShaderNodeBackground')
    nt.links.new(bg.outputs[0], out.inputs[0])
    az, e = math.radians(az_compass), math.radians(el)
    d = Vector((math.sin(az) * math.cos(e), math.cos(az) * math.cos(e), math.sin(e)))
    if not night:
        sky = nt.nodes.new('ShaderNodeTexSky')
        try:
            sky.sky_type = 'MULTIPLE_SCATTERING'
            sky.sun_elevation = max(e, math.radians(-2.0))
            sky.sun_rotation = math.pi / 2 - math.atan2(d.y, d.x)
            sky.altitude = 2100.0
            sky.sun_disc = False
        except Exception:
            pass
        nt.links.new(sky.outputs[0], bg.inputs[0])
        bg.inputs[1].default_value = sky_strength
    else:
        bg.inputs[0].default_value = (0.0016, 0.0024, 0.0055, 1.0)
        bg.inputs[1].default_value = 1.0
    for o in [o for o in sc.objects if o.type == 'LIGHT' and o.name.startswith('ecx_sun')]:
        bpy.data.objects.remove(o)
    ld = bpy.data.lights.new('ecx_sun', 'SUN')
    ld.energy = energy
    ld.angle = math.radians(0.6 if not night else 1.0)
    ld.color = color
    so = core.link(bpy.data.objects.new('ecx_sun', ld))
    so.rotation_euler = (-d).to_track_quat('-Z', 'Y').to_euler()


def _shot(sc, path, eye, target, fov, w, h, samples):
    cam = sc.camera
    cam.data.lens_unit = 'FOV'
    cam.data.angle = math.radians(fov)
    cam.data.clip_start = 0.2
    cam.data.clip_end = 90000.0
    e = Vector((-eye[0], eye[1], eye[2]))
    t = Vector((-target[0], target[1], target[2]))
    cam.location = e
    cam.rotation_euler = (t - e).to_track_quat('-Z', 'Y').to_euler()
    sc.render.resolution_x, sc.render.resolution_y = w, h
    sc.cycles.samples = samples
    sc.render.filepath = path
    sc.render.image_settings.file_format = 'PNG'
    bpy.ops.render.render(write_still=True)
    return path


def _compose(path, tiles, w, h, cols=2):
    rows = (len(tiles) + cols - 1) // cols
    gap = 6
    W, H = cols * w + (cols - 1) * gap, rows * h + (rows - 1) * gap
    out = np.zeros((H, W, 4), dtype=np.float32)
    out[:, :, 3] = 1.0
    out[:, :, :3] = 0.05
    for k, p in enumerate(tiles):
        img = bpy.data.images.load(p, check_existing=False)
        a = np.empty(w * h * 4, dtype=np.float32)
        img.pixels.foreach_get(a)
        bpy.data.images.remove(img)
        r, c = k // cols, k % cols
        y0 = H - (r + 1) * h - r * gap
        x0 = c * (w + gap)
        out[y0:y0 + h, x0:x0 + w] = a.reshape(h, w, 4)
    img = bpy.data.images.new('ecx_vistas', W, H, alpha=False)
    img.pixels.foreach_set(out.ravel())
    img.filepath_raw = path
    img.file_format = 'JPEG'
    img.save(quality=90)
    bpy.data.images.remove(img)
    return path


def render_vistas(objs):
    """art/review/ecx_landscape_vistas.jpg: the landscape assembled as the game will place it (spec 2.5-2.7), the
    building and the plaza imported when they exist: the reveal from the platform by day, the crane at noon, the
    overlook at dusk, the valley at night (the city, the switchbacks' lamps, mist), the north at night (the hamlet),
    the stand by the north arm close up, and the Crest from over the podium at noon and in the alpenglow."""
    t0 = time.time()
    sc = bpy.context.scene
    by = {o.name: o for o in objs}
    rb, rn = tree_materials(True, review_tex())
    for nm in ('SM_ECX_Spruce_Tall', 'SM_ECX_Spruce_Mid', 'SM_ECX_Spruce_Young', 'SM_ECX_Pine_Tall'):
        if nm in by:
            by[nm].data.materials[0] = rb
            by[nm].data.materials[1] = rn
    if 'SM_ECX_SiteTerrain' in by and os.path.exists(tex_path('T_ECX_Snow_BaseColor')):
        _review_terrain_material(by['SM_ECX_SiteTerrain'])
    if 'SM_ECX_ValleyMist' in by:
        _review_mist_material(by['SM_ECX_ValleyMist'])
    glow = by.get('SM_ECX_SlopeLights_GlowWarm')
    if glow:
        b = next(n for n in glow.data.materials[0].node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
        b.inputs['Emission Color'].default_value = (1.0, 0.72, 0.38, 1.0)
        b.inputs['Base Color'].default_value = (0.1, 0.1, 0.1, 1.0)
    context = []
    placed = []
    col = None
    hide = []
    try:
        context = _import_context()
        # Props placed (instances: linked copies of each mesh).
        col = bpy.data.collections.new('ecx_review_props')
        sc.collection.children.link(col)
        for item in _review_scatter():
            name, x, y, yaw, scale = item[:5]
            if name not in by:
                continue
            zoff = item[5] if len(item) > 5 else 0.0
            z = float(world_height(np.array([x]), np.array([y]), 2.0)[0])
            if inside_poly(np.array([x]), np.array([y]), [q for q, _ in PAD])[0]:
                z = float(arm_z(np.array([max(x, 21.25)]))[0]) if abs(y) > 25 else (-2.25 if x > 47 else (0.0 if x < 23 else (-0.75 if x < 35 else -1.5)))
            inst = bpy.data.objects.new(name + '_rv', by[name].data)
            inst.location = (-x, y, z + zoff - 0.05)
            inst.rotation_euler = (0.0, 0.0, yaw)
            inst.scale = (scale, scale, scale)
            col.objects.link(inst)
            placed.append(inst)
        hide = [by[n] for n in by if n not in TERRAIN]
        for o in hide:
            o.hide_render = True
        cd = bpy.data.cameras.new('ecx_vista_cam')
        sc.camera = core.link(bpy.data.objects.new('ecx_vista_cam', cd))
        sc.view_settings.view_transform = 'AgX'
        sc.cycles.use_denoising = True
        tmp = os.path.join(core.BUILD_DIR, 'review', 'ecx_landscape')
        os.makedirs(tmp, exist_ok=True)
        w, h = 1280, 640
        S = 24 if FAST else 64
        tiles = []
        city = by.get('SM_ECX_ValleyCity')
        city_em = None
        if city:
            city_em = next((n for n in city.data.materials[0].node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None)

        def lights(on):
            if city_em is not None:
                city_em.inputs['Emission Strength'].default_value = CITY_GLOW if on else 0.0
            if glow:
                b = next(n for n in glow.data.materials[0].node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
                b.inputs['Emission Strength'].default_value = 400.0 if on else 0.0
        lights(False)
        _set_sun(sc, 40.0, 180.0, 4.0, (1.0, 0.97, 0.92), 0.3)
        tiles.append(_shot(sc, os.path.join(tmp, 'v1_reveal.png'), (51.4, 1.0, -0.6), (-60.0, -6.0, 15.0), 70, w, h, S))
        tiles.append(_shot(sc, os.path.join(tmp, 'v2_crane.png'), (300.0, -100.0, 30.0), (0.0, 0.0, 63.0), 65, w, h, S))
        _set_sun(sc, 4.0, 255.0, 3.0, (1.0, 0.72, 0.45), 0.25)
        lights(True)
        tiles.append(_shot(sc, os.path.join(tmp, 'v3_dusk_overlook.png'), (69.0, 13.0, -0.6), (10000.0, -1000.0, -950.0), 60, w, h, S))
        _set_sun(sc, 22.0, 235.0, 0.09, (0.62, 0.72, 0.9), 0.0, night=True)
        sc.view_settings.exposure = 2.5
        tiles.append(_shot(sc, os.path.join(tmp, 'v4_night_valley.png'), (69.0, 13.0, -0.6), (10000.0, 1500.0, -1100.0), 60, w, h, S))
        tiles.append(_shot(sc, os.path.join(tmp, 'v5_night_north.png'), (61.0, 46.0, 1.2), (-10.0, 650.0, 35.0), 50, w, h, S))
        sc.view_settings.exposure = 0.0
        lights(False)
        _set_sun(sc, 35.0, 200.0, 4.0, (1.0, 0.97, 0.92), 0.3)
        tiles.append(_shot(sc, os.path.join(tmp, 'v6_stand.png'), (36.0, 34.0, 1.0), (24.0, 70.0, 6.0), 60, w, h, S))
        # The Crest itself, from over the podium (the plaza sees its summit horn over the canopy): noon and alpenglow.
        _set_sun(sc, 40.0, 180.0, 4.0, (1.0, 0.97, 0.92), 0.3)
        tiles.append(_shot(sc, os.path.join(tmp, 'v7_crest_noon.png'), (54.0, 0.0, 60.0), (-3500.0, -250.0, 800.0), 45, w, h, S))
        _set_sun(sc, 3.0, 255.0, 3.0, (1.0, 0.62, 0.38), 0.25)
        tiles.append(_shot(sc, os.path.join(tmp, 'v8_crest_dusk.png'), (54.0, 0.0, 60.0), (-3500.0, -250.0, 800.0), 45, w, h, S))

        path = _compose(os.path.join(core.REVIEW_DIR, 'ecx_landscape_vistas.jpg'), tiles, w, h)
        log(f'review: vistas {os.path.relpath(path, core.ROOT)} in {time.time() - t0:.0f}s')
    finally:
        for o in placed:
            bpy.data.objects.remove(o)
        if col is not None:
            bpy.data.collections.remove(col)
        for o in context:
            if o.name in bpy.data.objects:
                bpy.data.objects.remove(o)
        for o in hide:
            o.hide_render = False
        for o in [o for o in sc.objects if (o.type == 'LIGHT' and o.name.startswith('ecx_sun')) or o.name.startswith('ecx_vista_cam')]:
            bpy.data.objects.remove(o)


def render_texture_sheet():
    """art/review/ecx_landscape_textures.jpg: the textures this module writes (the needle atlas, bark, snow, rock,
    stars, moon) side by side."""
    names = [('T_ECX_Needles_BaseColor', True), ('T_ECX_Needles_Normal', False), ('T_ECX_Bark_BaseColor', False), ('T_ECX_Snow_Normal', False),
             ('T_ECX_Rock_BaseColor', False), ('T_ECX_StarField', False), ('T_ECX_Moon', True), ('T_ECX_Snow_BaseColor', False)]
    tiles = []
    T = 1024
    for n, has_alpha in names:
        p = tex_path(n)
        if not os.path.exists(p):
            continue
        img = bpy.data.images.load(p, check_existing=False)
        img.scale(T, T // 2 if n == 'T_ECX_StarField' else T)
        a = np.empty(img.size[0] * img.size[1] * 4, dtype=np.float32)
        img.pixels.foreach_get(a)
        a = a.reshape(img.size[1], img.size[0], 4)
        bpy.data.images.remove(img)
        if n == 'T_ECX_StarField':
            a = np.concatenate([np.zeros((T // 4, T, 4), np.float32), np.clip(a * 3.0, 0, 1), np.zeros((T // 4, T, 4), np.float32)], 0)
        if has_alpha:
            bg = np.array([0.08, 0.1, 0.13, 1.0], dtype=np.float32)
            a[:, :, :3] = a[:, :, :3] * a[:, :, 3:4] + bg[:3] * (1 - a[:, :, 3:4])
        a[:, :, 3] = 1.0
        tiles.append(a)
    if not tiles:
        return
    cols = 4
    rows = (len(tiles) + cols - 1) // cols
    out = np.zeros((rows * T + (rows - 1) * 6, cols * T + (cols - 1) * 6, 4), dtype=np.float32)
    out[:, :, 3] = 1.0
    for k, a in enumerate(tiles):
        r, c = k // cols, k % cols
        y0 = out.shape[0] - (r + 1) * T - r * 6
        out[y0:y0 + T, c * (T + 6):c * (T + 6) + T] = a
    img = bpy.data.images.new('ecx_tex_sheet', out.shape[1], out.shape[0], alpha=False)
    img.pixels.foreach_set(out.ravel())
    img.filepath_raw = os.path.join(core.REVIEW_DIR, 'ecx_landscape_textures.jpg')
    img.file_format = 'JPEG'
    img.save(quality=88)
    bpy.data.images.remove(img)


def pose_for_review(objs):
    """Renders the vistas and the texture sheet, then hands the standard sheet just the props, side by side: the four
    trees (textured), the two clumps, the three rocks."""
    global REVIEW_VIEWS
    if os.environ.get('ECX_NO_VISTAS') is None:
        try:
            render_vistas(objs)
        except Exception as exc:  # the review must not cost the export
            import traceback
            traceback.print_exc()
            log(f'review: vistas failed: {exc}')
        try:
            render_texture_sheet()
        except Exception as exc:
            log(f'review: texture sheet failed: {exc}')
    keep = [o for o in objs if o.name not in TERRAIN]
    for o in objs:
        if o.name in TERRAIN:
            o.hide_render = True
            o.hide_viewport = True
            o.location.z = -10000.0
    objs[:] = keep
    if not keep:
        return
    rb, rn = tree_materials(True, review_tex()) if os.path.exists(tex_path('T_ECX_Needles_BaseColor')) else (None, None)
    order = ['SM_ECX_Rock_C', 'SM_ECX_Rock_B', 'SM_ECX_Spruce_Young', 'SM_ECX_Rock_A', 'SM_ECX_Spruce_Mid', 'SM_ECX_Pine_Tall', 'SM_ECX_Spruce_Tall',
             'SM_ECX_ForestClump_A', 'SM_ECX_ForestClump_B']
    keep.sort(key=lambda o: order.index(o.name) if o.name in order else 99)
    y = 0.0
    for o in keep:
        if rb is not None and o.name in ('SM_ECX_Spruce_Tall', 'SM_ECX_Spruce_Mid', 'SM_ECX_Spruce_Young', 'SM_ECX_Pine_Tall'):
            o.data.materials[0] = rb
            o.data.materials[1] = rn
        lo, hi = core.bounds([o])
        o.location.y += y - lo.y
        o.location.x -= (lo.x + hi.x) / 2
        y += (hi.y - lo.y) + 2.0
    bpy.context.view_layer.update()
    lo, hi = core.bounds(keep)
    size = (hi - lo).length
    cy = (lo.y + hi.y) / 2
    REVIEW_VIEWS = [('row', -90, 6, 0.75, (0.0, cy, 7.0)), ('angle', -55, 14, 0.62, (0.0, cy, 6.0)),
                    ('close', -70, 8, 9.0 / size, (0.0, lo.y + 6.0, 2.0))]



def forest_mask(N=2048):
    """T_ECX_ForestMask: where the forest is, for scattering the clumps and trees (an extra this module offers): R the
    forest's density 0..1, G the ground's slope (0 flat .. 1 at 60 deg), B 1 above the snow line. Square over map X, Y
    -3000..3000 m: column c is X = -3000 + 6000 (c + 0.5) / N, image row r (from the top) Y = 3000 - 6000 (r + 0.5) / N."""
    t0 = time.time()
    c = -3000.0 + 6000.0 * (np.arange(N) + 0.5) / N
    X, Y = np.meshgrid(c, c)                      # rows bottom-up = Y increasing: the saved PNG's top row is +3000
    Z = world_height(X.ravel(), Y.ravel(), lam=6000.0 / N * 2).reshape(X.shape)
    step = 6000.0 / N
    gy, gx = np.gradient(Z, step)
    slope = np.degrees(np.arctan(np.hypot(gx, gy))).astype(np.float32)
    f = forest_density(X.ravel().astype(np.float32), Y.ravel().astype(np.float32), Z.ravel(), slope.ravel()).reshape(X.shape)
    rd, _, rk = raster().sample(X.ravel(), Y.ravel())
    f = f * smoothstep(ROAD_HW + 4.0, ROAD_HW + 12.0, rd.reshape(X.shape))
    out = np.stack([f, np.clip(slope / 60.0, 0.0, 1.0), (Z > -450.0).astype(np.float32)], axis=2)
    log(f'forest mask in {time.time() - t0:.0f}s')
    return out.astype(np.float32)
