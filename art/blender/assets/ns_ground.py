"""SM_NS_Ground_* and the castings: Northside's ground (world realism spec §4.1): Fifth St and its two T's, Market St, Cardinal
Ave and the cross streets out to the haze, the castings set into them, and the tiling texture sets the ground is shaded with.

Frames (spec §0.3, §0.4). Stage meters here (the spec's cm / 100): X west (+X is the far side), Y north, Z up from the
sidewalk's top. The ground meshes are SITE meshes: Blender (0, 0, 0) is the stage origin, x = -X, y = Y, z = Z; they go in
at the stage's transform with yaw 180. The castings are PROP meshes (front toward Blender -X; a PROP placed to face stage
yaw F turns by -F about Blender Z in this frame).

THE GROUND (unbaked, complex-as-simple collision). Slots in this order: 0 ns_sidewalk, 1 ns_asphalt, 2 ns_patch, 3 ns_curb,
4 ns_gutter, 5 ns_concrete, 6 ns_tactile, 7 ns_marking, 8 ns_busbay. glTF drops a slot with no faces, so the glbs carry
only what they use, in that relative order: FifthS and FifthN all but ns_busbay; Market all but ns_tactile; Cardinal all
but ns_patch and ns_busbay; Far ns_sidewalk, ns_asphalt, ns_curb, ns_gutter, ns_marking. Assign the materials by slot NAME,
not index. The build log prints the slots each glb carries and its triangle count.
- UV0: meters. Horizontal faces u = X, v = Y (stage meters: the spec's u = X/100 in cm). Curbs: u = meters along the curb
  (continuous along each run, from its start), v = Z - top on the face (0 at the arris, -0.17 at the flow line) and v =
  meters back from the face on the top (0..0.2); T_NS_Curb is laid out for that (its steel curb angle is the band
  |v| < 0.05). Skirts and the tree pits' walls u along, v = Z.
- COLOR_0 (linear float RGBA). Ground slots: R puddle-prone, G grime (1 = filthy: the building bases, the gutters, the oil
  strip between the wheel paths; 0 = clean or polished: the wheel paths, the walking line), B shelter (1 under an awning, the
  shed or the shelter's roof), A 1. Markings (ns_marking): RGB the paint (linear: white 0xd8d4c8, yellow 0xd9a520; the valve
  lids' iron 0x2c2a27), A the wear (0 fresh .. 1 gone: wheel paths, then noise); T_NS_Thermo_Wear gives the pattern it wears
  in (worn where Wear < A).
- Levels: sidewalks flat at 0; curbs 20 cm wide, top 0, a 12 mm chamfer on the arris, a 6 mm round at the back, joints
  every 3 m (a 6 mm groove 4 mm deep across the face and top); gutter pans 45 cm, flow line -0.17 at the face, rising
  straight to the road's edge; the road RoadZ = -0.15 + 0.03 (1 - (2d/W)^2) on every street (Fifth W 12, Market 11, the
  cross streets 10). Fifth's crown fades out over the 6 m before each T (Hollis and Wexler are through streets), Market's
  over the 6 m before Fifth and before Cardinal (Cardinal is through). road_z() below is the function; ue-world puts cars
  and props on it.
- Corners: every curb return has R 1.5 m at the curb face (picked so each crosswalk's ramp sits on a straight curb and the
  14's right turn at 8 m path radius clears the corner).
- Sidewalk flags 1.5 m on the world grid (joints at multiples of 1.5 m in X and Y, as the texture tiles them); 6 % are
  patched (their cells in ns_concrete), 5 % sagged in a 1 cm dish (R up), 4 % cracked: a corner piece broken off along a
  jagged line and sunk 5-11 mm (tilted), with a little wall round it.
- Curb ramps (ns_concrete) with yellow truncated-dome pads (ns_tactile, 0.6 deep x 1.2 m, from the back of the curb; their
  edges on the 5 cm dome grid): the six at Fifth & Market, two across Fifth's mouth at each T, two across Market's mouth at
  Cardinal. Core 1.2 m wide, run 1.5 m (10 %), flares 0.75 m, bottom -0.165 (a 5 mm lip over the flow line). Core spans
  (along the curb): Fifth's S leg Y 50.8..52.0 and N leg Y 67.0..68.2 at both curbs (X 4.2, 16.2); Market's mouth X 1.2..2.4
  at both curbs; Hollis T Y -64.3..-63.1, Wexler T Y 93.1..94.3 (both curbs); Cardinal T X -97.9..-96.7 (Market's curbs).
- Driveway aprons (ns_concrete, curb dropped to -0.15 over a 1.2 m run, 0.6 m flares): F-WEXLER's garage on Wexler's south
  sidewalk, X 30.0..33.0; the Cardinal loading door on Cardinal's west sidewalk, Y 36.0..42.0.
- Tree pits 1.2 m square at the spec's 16 spots: a hole in the sidewalk, its walls to -0.035, a 4 cm seat at -0.035 (the
  grate's ledge), walls on down to the soil at -0.08 (mounded to -0.05 round the trunk), in ns_gutter (silt and dirt).
- Recesses for the castings (2 mm): manholes Fifth (7.8, -48), (7.8, -9), (12.6, 33), (11.0, 83); Market (-11, 58),
  (-43, 61), (-52, 60) (the steam stack's vent), (-82, 58). Curb inlets (the curb is left out over 1.2 m and the gutter
  under it sunk 2 mm): home curb X 4.2 at Y -59.0, 22.0; far curb X 16.2 at Y -18.5, 49.0, 89.0; Market south curb Y 54 at
  X -13.0, -64.0; north curb Y 65 at X -7.0, -93.0; Hollis's north curb Y -66.2 at X 30.0; Wexler's south curb Y 96.2 at
  X -10.0. Vault lights (1.2 x 2.4, the long side along the building line): H-CASH (1.5, 12.0), M-S1 (-17.0, 50.5), M-N4
  (-39.0, 68.5).
- Road features: utility patches (ns_patch) Fifth X 5.2..7.8 Y -40.0..-38.4, X 11.0..14.6 Y -12.0..-10.8, X 7.2..8.4 Y
  8.0..9.6, X 12.4..15.4 Y 26.0..29.5, X 4.65..8.2 Y 40.0..41.2, X 9.0..14.0 Y 80.0..81.0; Market X -11..-8 Y 56.0..58.0,
  X -31..-28 Y 60.0..63.0, X -55..-52 Y 55.0..57.3, X -70..-66 Y 61.0..62.5, X -86.5..-84 Y 57.0..59.0; the utility cut
  (SM_NS_RoadPlate goes on it) X -51..-45 Y 59.7..62.4. Potholes (3 cm, R 1): (13.0, -25.0), (7.0, 75.0), (-40.0, 63.5).
  The bus bay (ns_busbay) X -35..-15 Y 54.45..56.4.
- Markings (2 mm proud, cut into 25 cm cells): the double yellows (Fifth X 10.08..10.20 + 10.30..10.42 over Y -61.2..48.8
  and 70.2..91.2; Market Y 59.32..59.42 + 59.58..59.68 over X -94.9..-1.1; Hollis, Wexler, Cardinal 10 cm lines 10 cm apart
  on their centerlines, through); stop bars 30 cm, 1.2 m before the crosswalks (Fifth NB Y 48.5..48.8, SB 70.2..70.5;
  Market WB X -1.1..-0.8; Fifth SB at Hollis Y -61.2..-60.9; Fifth NB at Wexler Y 90.9..91.2; Market EB at Cardinal X
  -95.2..-94.9); the ladder crosswalks (20 cm rails at the edges, 60 cm bars with 60 cm gaps set out from the road's
  centerline): Fifth Y 50.0..53.8 and 65.2..69.0, Market's mouth X 0.4..3.8, across Fifth at Hollis Y -65.8..-62.4 and
  Wexler 92.4..95.8, across Market at Cardinal X -99.8..-96.4; the bay's outline and BUS ONLY (BUS nearest a driver heading
  east, 2.4 m letters); parking T-marks every 6 m on Market (south X -38..-92, north X -6..-90 clear of the return stop);
  iron valve lids (20-30 cm discs, marking slot) along the curbs.
- The meshes (faces go to a mesh by their centroid):
  SM_NS_Ground_FifthS: X -38..58.4, Y < 15 (Fifth and the Hollis T to 20 m past the walk limits). SM_NS_Ground_FifthN: the
  same, Y >= 15, with the corner's mouth X 0..4.65 Y 49.5..69.5 and the Wexler T. SM_NS_Ground_Market: X -96..0, Y
  49.5..69.5. SM_NS_Ground_Cardinal: X -114.4..-96, Y 14..104 (walk limits Y 34 and 84 plus 20 m; the spec's 24..96 would
  leave a gap to Far). SM_NS_Ground_Far: Hollis and Wexler out to X -90 and 130.2, Cardinal to Y -50 and 170, coarse.
  Cells: roads 0.1 m, sidewalks 0.1875 m (8 per flag), Far 1.0 / 0.75 m; skirts 0.3 m deep hang from every open edge
  (building lines, the meshes' ends).

THE CASTINGS (PROP, baked, front toward Blender -X):
- SM_NS_Manhole: cast iron, Ø 0.92 x 0.06 m: a frame flange (r 0.333..0.46, worn ribs) round a 0.66 m cover: an outer band
  of radial lettering NORTHSIDE · SEWER · 1931 (raised, the tops polished bright), a field of raised diamonds, a center
  boss, two pick holes; rust in the hollows. Origin: the cover's center at its top (Z 0 = the top; the iron runs down to
  -0.06). Slot: iron.
- SM_NS_ManholeVent: the same frame, a cover with 4 rings of vent holes (dark through them) for the steam stack.
- SM_NS_CurbInlet: 1.2 x 0.65 m: the curb piece (the ground's curb profile, top Z 0, 20 cm deep) with a 1.0 x 0.1 m throat
  under a cast-iron hood plate (four hex bolts) lettered NO DUMPING · DRAINS TO RIVER in 1 mm relief, the gutter apron (the
  pan's slope, -0.17 at the face) and a 0.9 x 0.42 bicycle-safe grate in it (14 bars across the flow on two ribs). Origin: the curb face's middle at Z 0 (x 0 = the face; the gutter at x < 0, the
  curb at x 0..0.2). Slots: concrete, iron. Face it to the road.
- SM_NS_TreeGrate: 1.2 m square, 3 cm, two halves split across Y, round-ended radial slots in three rings round a 0.46 m
  trunk hole (and short slots in the corners), ribs under them, a sunk hex bolt in each corner, heavily rusted. Origin: its
  center at its top.
- SM_NS_VaultLights: 1.2 x 2.4 m steel grid (long side along Blender y) with 210 round prism lenses (10 x 21 on 11 cm
  centers; some cracked, some aged purple), set flush; origin its center at its top. Slots: steel, glass (baked, opaque).

TEXTURE SETS (unreal/Art/Textures/NS; seamless; BaseColor sRGB, Normal OpenGL +Y (the setup flips green), ORM linear,
Height linear 0..1 over the range given): T_NS_Sidewalk_A / _B (3.0 m, 2048: 2 x 2 flags with tooled joints and smooth
edger margins, broom finish, per-flag tone, round gum 1-2 cm, stains, hairline cracks, spalls; Height -8..+2 mm),
T_NS_Asphalt (4 m, 2048: aggregate, oxidation, tar-sealed cracks), T_NS_Patch (4 m, 2048: newer, blacker, roller marks),
T_NS_Curb (2 m, 1024: the steel curb angle in v -0.05..0.05 (metallic in ORM.B), the face below it, the top above),
T_NS_Gutter (2 m, 1024: silt, tide marks, algae), T_NS_Concrete (2 m, 1024: new broom-finished concrete), T_NS_Tactile
(0.6 m, 512: 12 x 12 truncated domes on 50 mm centers, dome centers at 25 mm + 50 mm k; Height 0..5 mm), T_NS_Thermo (2 m,
1024: near-white, multiplied by the vertex paint; glass beads, alligator cracks, scuffs) plus T_NS_Thermo_Wear (the wear
pattern), T_NS_BusBay (4 m, 2048: red surfacing worn to asphalt). The 2 x 2 tiling check is art/review/ns_ground_textures.jpg.
T_NS_Decals (P1, 4096 RGBA, A = opacity; _Normal, _ORM; a 4 x 4 atlas of 1024 cells, row-major from the top left, each cell
about 1 m): 0 oil soak, 1 oil drip trail, 2 gum cluster, 3 wet leaves (sparse), 4 wet leaves (a drift), 5 cigarette butts,
6 wet newspaper, 7 flyer scraps, 8 tire skid (travel along v), 9 rust drip (wall, runs down v), 10 water tide stain, 11 soot
streak (wall, rises up v), 12 wheat-paste poster FIGHT NIGHT, 13 poster LOST CAT (555-0133), 14 tag KRAYO, 15 tag DUSK.
For deferred decals (DBuffer translucent, normal + roughness); art/review/ns_ground_decals.jpg shows it over the sidewalk.

NS_GROUND_ONLY=tex,ground,castings (or mesh keys: fifth,market,cardinal,far,manhole,vent,inlet,grate,vault) builds a subset;
NS_GROUND_REVIEW=site,props,tex picks the extra sheets (art/review/ns_ground_*.jpg).
"""
import math
import os
import random
import struct
import time
import traceback
import zlib

import bmesh
import bpy  # noqa: F401
import numpy as np
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

from artkit import core, parts, review
from artkit.shade import convex_edges, mix_float, noise, object_coords, ramp, scaled, smooth_less, vec
from artkit.sheet import Sheet

GROUND = ['SM_NS_Ground_FifthS', 'SM_NS_Ground_FifthN', 'SM_NS_Ground_Market', 'SM_NS_Ground_Cardinal', 'SM_NS_Ground_Far']
CASTINGS = ['SM_NS_Manhole', 'SM_NS_ManholeVent', 'SM_NS_CurbInlet', 'SM_NS_TreeGrate', 'SM_NS_VaultLights']
MESHES = GROUND + CASTINGS
UNBAKED = set(GROUND)
_ONLY = [s.strip() for s in os.environ.get('NS_GROUND_ONLY', '').split(',') if s.strip()]
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
# COLOR_0 must carry its alpha (the markings' wear): the exporter's default keeps a vertex color's alpha only when an
# exported material reads it as alpha, and none does, so the markings lost theirs and M_NS_Ground wore every one through.
GLTF_OPTIONS = {'export_vertex_color': 'ACTIVE'}
TEXTURE_SIZES = {'nsm_iron': 2048, 'nsv_iron': 2048, 'nsi_concrete': 1024, 'nsi_iron': 2048, 'nst_iron': 2048, 'nsl_steel': 2048,
                 'nsl_glass': 1024}
AO_DISTANCE = 0.05
REVIEW_VIEWS = None
REVIEW_SCREEN_LIGHT = False
TEX_DIR = os.path.join(core.ROOT, 'unreal', 'Art', 'Textures', 'NS')
SLOTS = ['ns_sidewalk', 'ns_asphalt', 'ns_patch', 'ns_curb', 'ns_gutter', 'ns_concrete', 'ns_tactile', 'ns_marking', 'ns_busbay']
SIDEWALK, ASPHALT, PATCH, CURB, GUTTER, CONCRETE, TACTILE, MARKING, BUSBAY = range(9)

PAINT_WHITE = 0xd8d4c8
PAINT_YELLOW = 0xd9a520
IRON_LID = 0x2c2a27


def H(h):
    return core.hex_linear(h)


def lin(hexc):
    return np.array(H(hexc)[:3], dtype=np.float32)


def log(*a):
    print('[ns_ground]', *a, flush=True)


def wanted(*keys):
    return not _ONLY or any(k in _ONLY for k in keys)


# ================================================================== shared helpers (ns_kit and ns_cars use these too)

def smoothstep(e0, e1, x):
    t = np.clip((np.asarray(x, dtype=np.float64) - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def _hash2(ix, iy, seed):
    ix = np.asarray(ix).astype(np.int64)
    iy = np.asarray(iy).astype(np.int64)
    with np.errstate(over='ignore'):           # the multiplies wrap on purpose (a scalar warns where an array doesn't)
        h = (ix * np.int64(73856093)) ^ (iy * np.int64(19349663)) ^ np.int64(seed * 83492791 + 12345)
        h = (h ^ (h >> 13)) * np.int64(1274126177)
        h = h ^ (h >> 16)
    return (h & 0xFFFFFF).astype(np.float64) / float(0xFFFFFF)


def vnoise2(X, Y, scale, seed=0, octaves=3, gain=0.5):
    """Smooth 2D value noise in 0..1 at arrays X, Y (meters); scale = feature size."""
    X = np.asarray(X, dtype=np.float64)
    Y = np.asarray(Y, dtype=np.float64)
    out = np.zeros_like(X)
    amp, tot, s = 1.0, 0.0, scale
    for o in range(octaves):
        qx, qy = X / s, Y / s
        ix, iy = np.floor(qx), np.floor(qy)
        fx, fy = qx - ix, qy - iy
        fx = fx * fx * fx * (fx * (fx * 6 - 15) + 10)
        fy = fy * fy * fy * (fy * (fy * 6 - 15) + 10)
        sd = seed + o * 31
        a = _hash2(ix, iy, sd)
        b = _hash2(ix + 1, iy, sd)
        c = _hash2(ix, iy + 1, sd)
        d = _hash2(ix + 1, iy + 1, sd)
        out += amp * ((a * (1 - fx) + b * fx) * (1 - fy) + (c * (1 - fx) + d * fx) * fy)
        tot += amp
        amp *= gain
        s *= 0.5
    return out / tot


def seg_distance(P, segs):
    """Distance from points P (N x 2) to a list of segments ((x0, y0), (x1, y1))."""
    P = np.asarray(P, dtype=np.float64)
    best = np.full(len(P), 1e9)
    if not segs:
        return best
    A = np.array([s[0] for s in segs], dtype=np.float64)
    B = np.array([s[1] for s in segs], dtype=np.float64)
    for c0 in range(0, len(A), 64):
        a = A[c0:c0 + 64][None, :, :]
        b = B[c0:c0 + 64][None, :, :]
        ab = b - a
        for p0 in range(0, len(P), 200000):
            pp = P[p0:p0 + 200000]
            pa = pp[:, None, :] - a
            t = np.clip((pa * ab).sum(2) / np.maximum((ab * ab).sum(2), 1e-12), 0.0, 1.0)
            d = np.linalg.norm(pa - t[..., None] * ab, axis=2)
            best[p0:p0 + 200000] = np.minimum(best[p0:p0 + 200000], d.min(axis=1))
    return best


def poly_area(p):
    a = 0.0
    for i in range(len(p)):
        x0, y0 = p[i]
        x1, y1 = p[(i + 1) % len(p)]
        a += x0 * y1 - x1 * y0
    return a / 2


def ccw(p):
    return list(p) if poly_area(p) >= 0 else list(reversed(p))


def clip_convex(subject, clip):
    """Sutherland-Hodgman: subject polygon clipped by a convex CCW polygon."""
    out = list(subject)
    n = len(clip)
    for i in range(n):
        if not out:
            break
        ax, ay = clip[i]
        bx, by = clip[(i + 1) % n]
        ex, ey = bx - ax, by - ay
        inp, out = out, []
        s = inp[-1]
        ss = ex * (s[1] - ay) - ey * (s[0] - ax)
        for p in inp:
            sp = ex * (p[1] - ay) - ey * (p[0] - ax)
            if sp >= -1e-12:
                if ss < -1e-12:
                    t = ss / (ss - sp)
                    out.append((s[0] + t * (p[0] - s[0]), s[1] + t * (p[1] - s[1])))
                out.append(p)
            elif ss >= -1e-12:
                t = ss / (ss - sp)
                out.append((s[0] + t * (p[0] - s[0]), s[1] + t * (p[1] - s[1])))
            s, ss = p, sp
    return out


def clean_poly(p, eps=1e-7):
    out = []
    for q in p:
        if not out or abs(q[0] - out[-1][0]) > eps or abs(q[1] - out[-1][1]) > eps:
            out.append(q)
    while len(out) > 1 and abs(out[0][0] - out[-1][0]) <= eps and abs(out[0][1] - out[-1][1]) <= eps:
        out.pop()
    res = []
    n = len(out)
    for i in range(n):
        a, b, c = out[i - 1], out[i], out[(i + 1) % n]
        cr = (b[0] - a[0]) * (c[1] - b[1]) - (b[1] - a[1]) * (c[0] - b[0])
        if abs(cr) > 1e-12:
            res.append(b)
    return res


def rectp(x0, x1, y0, y1):
    return [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]


def text_mesh(body, size, face='Black', align='CENTER', tracking=1.0, extrude=0.0, offset=0.0, bevel=0.0, res=3):
    """Text converted to a mesh in its own XY plane (reads +X, up +Y; extruded along Z by +-extrude)."""
    cu = bpy.data.curves.new('txt', 'FONT')
    cu.body = body
    cu.font = bpy.data.fonts.load(os.path.join(core.FONTS, f'Roboto-{face}.ttf'), check_existing=True)
    cu.size = size
    cu.align_x = align
    cu.space_character = tracking
    cu.extrude = extrude
    cu.offset = offset
    cu.bevel_depth = bevel
    cu.bevel_resolution = 1 if bevel > 0 else 0
    cu.resolution_u = res
    obj = core.link(bpy.data.objects.new('txt', cu))
    core.select_only([obj])
    bpy.ops.object.convert(target='MESH')
    obj = bpy.context.view_layer.objects.active
    obj.name = 'txt'
    me = obj.data
    bm = bmesh.new()
    bm.from_mesh(me)
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-6)
    bm.to_mesh(me)
    bm.free()
    return obj


def text_bounds(obj):
    xs = [v.co.x for v in obj.data.vertices]
    ys = [v.co.y for v in obj.data.vertices]
    return min(xs), max(xs), min(ys), max(ys)


def set_attr(obj, name, value):
    me = obj.data
    a = me.attributes.get(name) or me.attributes.new(name, 'FLOAT', 'FACE')
    a.data.foreach_set('value', [float(value)] * len(me.polygons))
    return obj


def lock_uvs(obj, uvfun):
    """Sets obj's UVs per loop from uvfun(co) and flags its faces to keep them through finish_mesh."""
    me = obj.data
    uv = me.uv_layers.get('UVMap') or me.uv_layers.new(name='UVMap')
    for poly in me.polygons:
        for li in poly.loop_indices:
            uv.data[li].uv = uvfun(me.vertices[me.loops[li].vertex_index].co)
    set_attr(obj, 'ns_uvlock', 1.0)
    return obj


def order_slots(obj, mats, tag='ns'):
    """Puts obj's material slots in the order of mats and remaps the faces."""
    me = obj.data
    old = [s.material for s in obj.material_slots]
    remap = []
    for mt in old:
        if mt not in mats:
            raise RuntimeError(f'{obj.name}: material {mt.name if mt else None} not in its slot list')
        remap.append(mats.index(mt))
    fi = np.zeros(len(me.polygons), dtype=np.int32)
    me.polygons.foreach_get('material_index', fi)
    new = np.array(remap, dtype=np.int32)[fi] if len(remap) else fi
    me.materials.clear()
    for mt in mats:
        me.materials.append(mt)
    me.polygons.foreach_set('material_index', new)
    used = set(new.tolist())
    for i, mt in enumerate(mats):
        if i not in used:
            print(f'[{tag}] WARNING {obj.name}: slot {i} ({mt.name}) has no faces')


def tri_count(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


def finish_mesh(objs, name, mats, angle=35.0, rects=None, weighted=True, tag='ns'):
    """Joins parts, orders the slots as mats, shades hard-surface, lays out UVs: faces flagged ns_uvlock keep the UVs they
    were given (prints), every other face is box-projected per slot (into rects[slot name] if given)."""
    for o in objs:
        if not o.data.uv_layers:
            o.data.uv_layers.new(name='UVMap')
    obj = core.join(objs, name) if len(objs) > 1 else objs[0]
    obj.name = name
    obj.data.name = name
    order_slots(obj, [m.m if isinstance(m, core.Mat) else m for m in mats], tag)
    core.finish_hard_surface(obj, angle, weighted)
    lock = obj.data.attributes.get('ns_uvlock')
    locked = set()
    if lock is not None:
        vals = np.zeros(len(obj.data.polygons), dtype=np.float32)
        lock.data.foreach_get('value', vals)
        locked = set(np.nonzero(vals > 0.5)[0].tolist())
    rects = rects or {}
    groups = []
    if locked:
        groups.append((lambda f: f.index in locked, 'keep', None, (0.0, 0.0, 1.0, 1.0)))
    for s in obj.material_slots:
        groups.append((core.material_is(obj, s.material.name), 'box', None, rects.get(s.material.name, (0.0, 0.0, 1.0, 1.0))))
    groups.append((lambda f: True, 'box', None, (0.0, 0.0, 1.0, 1.0)))
    core.uv_layout(obj, groups)
    for k in ('ns_uvlock',):
        if k in obj.data.attributes:
            obj.data.attributes.remove(obj.data.attributes[k])
    print(f'[{tag}] {name} slots: {[s.material.name for s in obj.material_slots]}  tris {tri_count(obj)}')
    return obj


def box(x0, x1, y0, y1, z0, z1, mat, r=0.0, seg=2, name='box'):
    o = parts.rbox(name, ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), (abs(x1 - x0), abs(y1 - y0), abs(z1 - z0)), r, seg)
    if mat is not None:
        core.assign(o, mat)
    return o


def lathe(name, profile, mat=None, seg=48, matrix=None):
    o = parts.lathe_at(name, profile, matrix if matrix is not None else Matrix(), seg)
    if mat is not None:
        core.assign(o, mat)
    return o


def rod(a, b, r, mat, n=16):
    o = parts.rod('rod', a, b, r, n)
    core.assign(o, mat)
    return o


def tube(points, r, mat, n=12, steps=8):
    o = parts.tube('tube', points, r, n, steps)
    core.assign(o, mat)
    return o


def prism_y(name, prof, y0, y1, mat=None):
    """A prism along Y from a closed (x, z) profile."""
    bm = bmesh.new()
    a = [bm.verts.new((x, y0, z)) for x, z in prof]
    b = [bm.verts.new((x, y1, z)) for x, z in prof]
    n = len(prof)
    bm.faces.new(a)
    bm.faces.new(list(reversed(b)))
    for i in range(n):
        bm.faces.new((a[i], a[(i + 1) % n], b[(i + 1) % n], b[i]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    o = core.mesh_object(name, bm)
    if mat is not None:
        core.assign(o, mat)
    return o


def prism_x(name, prof, x0, x1, mat=None):
    """A prism along X from a closed (y, z) profile."""
    o = core.prism_x(name, prof, x0, x1)
    if mat is not None:
        core.assign(o, mat)
    return o


def plain_material(name, hex_color, rough=0.5, metal=0.0, cull=True, emit=None, emit_strength=1.0):
    """An unbaked material named exactly `name` (Unreal finds the slot by it)."""
    m = core.Mat(name)
    m.set('Base Color', H(hex_color))
    m.set('Roughness', rough)
    m.set('Metallic', metal)
    if emit is not None:
        m.set('Emission Color', H(emit))
        m.set('Emission Strength', emit_strength)
    m.m.use_backface_culling = cull
    if m.m.name != name:
        raise RuntimeError(f'material name clash: wanted {name}, got {m.m.name}')
    return m


def ao_cavity(m, distance=0.03, gain=1.5, samples=12):
    """1 in hollows (outside occlusion), 0 on open surfaces."""
    ao = m.node('ShaderNodeAmbientOcclusion', Distance=distance)
    ao.samples = samples
    ao.only_local = True
    return m.math('MULTIPLY', m.math('SUBTRACT', 1.0, ao.outputs['AO']), gain, clamp=True)


def set_bump(m, height, strength, distance):
    b = m.node('ShaderNodeBump', Strength=strength, Distance=distance)
    m.link(height, b.inputs['Height'])
    m.set('Normal', b.outputs['Normal'])
    return b


def attr(m, name, out='Fac'):
    n = m.node('ShaderNodeAttribute')
    n.attribute_type = 'GEOMETRY'
    n.attribute_name = name
    return n.outputs[out]


def sheet_image(sheet, name, px_w, samples=16):
    """Renders a Sheet; returns (color path, surface path)."""
    return sheet.render(name, px_w, samples)


def _png(path, arr):
    """Writes an H x W (x C) float array (row 0 at the top) as an 8-bit PNG exactly as given."""
    a = np.asarray(arr, dtype=np.float32)
    if a.ndim == 2:
        a = a[:, :, None]
    h, w, c = a.shape
    data = (np.clip(a, 0.0, 1.0) * 255.0 + 0.5).astype(np.uint8)
    raw = b''.join(b'\x00' + data[y].tobytes() for y in range(h))

    def chunk(tag, body):
        return struct.pack('>I', len(body)) + tag + body + struct.pack('>I', zlib.crc32(tag + body) & 0xffffffff)
    kind = {1: 0, 2: 4, 3: 2, 4: 6}[c]
    png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, kind, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw, 6)) + chunk(b'IEND', b'')
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'wb') as f:
        f.write(png)


def lin2srgb(x):
    x = np.clip(x, 0.0, 1.0)
    return np.where(x <= 0.0031308, x * 12.92, 1.055 * np.power(np.maximum(x, 0.0031308), 1 / 2.4) - 0.055)


def compose_grid(name, paths, cols, tag='ns_ground'):
    """Lays renders (all the same size) out in a grid as art/review/<name>.jpg."""
    imgs = [review._pixels(p) for p in paths]
    h, w = imgs[0].shape[:2]
    rows = (len(imgs) + cols - 1) // cols
    gap = 6
    W = cols * w + (cols - 1) * gap
    Hh = rows * h + (rows - 1) * gap
    out = np.zeros((Hh, W, 4), np.float32)
    out[:, :, :3] = 0.05
    out[:, :, 3] = 1.0
    for k, im in enumerate(imgs):
        r, c = k // cols, k % cols
        y0 = Hh - (r + 1) * h - r * gap
        x0 = c * (w + gap)
        ih, iw = im.shape[:2]
        out[y0:y0 + min(h, ih), x0:x0 + min(w, iw), :3] = im[:min(h, ih), :min(w, iw), :3]
    os.makedirs(core.REVIEW_DIR, exist_ok=True)
    path = os.path.join(core.REVIEW_DIR, f'{name}.jpg')
    img = bpy.data.images.new(f'rv_{name}', W, Hh, alpha=False)
    img.pixels.foreach_set(out.ravel())
    img.filepath_raw = path
    img.file_format = 'JPEG'
    img.save(quality=90)
    bpy.data.images.remove(img)
    print(f'[{tag}] review sheet {os.path.relpath(path, core.ROOT)}')
    return path


def import_glb(name, temp=None):
    """Imports an exported mesh (unreal/Art/Meshes/<name>.glb) for review context; the mesh object or None."""
    path = os.path.join(core.EXPORT_DIR, f'{name}.glb')
    if not os.path.exists(path):
        return None
    before = set(bpy.data.objects)
    grp = bpy.data.node_groups.get('glTF Material Output')
    if grp is not None:
        grp.name = 'glTF Material Output (bake)'
    try:
        bpy.ops.import_scene.gltf(filepath=path)
    except Exception:
        traceback.print_exc()
        return None
    finally:
        if grp is not None:
            imp = bpy.data.node_groups.get('glTF Material Output')
            if imp is not None:
                imp.name = 'glTF Material Output (import)'
            grp.name = 'glTF Material Output'
    new = [o for o in bpy.data.objects if o not in before]
    mesh = next((o for o in new if o.type == 'MESH'), None)
    for o in new:
        if o is not mesh:
            bpy.data.objects.remove(o)
    if mesh is None:
        return None
    mesh.parent = None
    mesh.matrix_world = Matrix()
    mesh.name = 'rv_' + name
    if temp is not None:
        temp.append(mesh)
    return mesh


# ================================================================== tileable texture tools (numpy)

def tnoise(n, cells, seed, octaves=4, gain=0.5, cells_y=None):
    """Tileable fractal value noise on an n x n tile (cells_y: a different lattice count down the rows)."""
    out = np.zeros((n, n), np.float32)
    amp, tot = 1.0, 0.0
    cy0 = cells_y if cells_y is not None else cells
    for o in range(octaves):
        px, py = int(cells * 2 ** o), int(cy0 * 2 ** o)
        if px > n or py > n:
            break
        rng = np.random.default_rng(seed * 1000 + o)
        lat = rng.random((py, px), dtype=np.float32)
        cx = np.arange(n, dtype=np.float32) * px / n
        cyy = np.arange(n, dtype=np.float32) * py / n
        ix0 = np.floor(cx).astype(np.int64)
        iy0 = np.floor(cyy).astype(np.int64)
        fx = cx - ix0
        fy = cyy - iy0
        fx = fx * fx * fx * (fx * (fx * 6 - 15) + 10)
        fy = fy * fy * fy * (fy * (fy * 6 - 15) + 10)
        ix1, iy1 = (ix0 + 1) % px, (iy0 + 1) % py
        ix0, iy0 = ix0 % px, iy0 % py
        a = lat[np.ix_(iy0, ix0)]
        b = lat[np.ix_(iy0, ix1)]
        c = lat[np.ix_(iy1, ix0)]
        d = lat[np.ix_(iy1, ix1)]
        fxx = fx[None, :]
        fyy = fy[:, None]
        out += amp * ((a * (1 - fxx) + b * fxx) * (1 - fyy) + (c * (1 - fxx) + d * fxx) * fyy)
        tot += amp
        amp *= gain
    return out / tot


def blur(a, passes=1):
    for _ in range(passes):
        a = (a * 2.0 + np.roll(a, 1, 0) + np.roll(a, -1, 0) + np.roll(a, 1, 1) + np.roll(a, -1, 1)) / 6.0
    return a


def grains(n, seed, soft=1, m=None):
    rng = np.random.default_rng(seed)
    return blur(rng.random((n, m or n), dtype=np.float32), soft)


def tworley(n, cells, seed, jitter=0.9, m=None, cells_y=None):
    """Tileable Worley noise: (F1, F2, id) in cell units on an n (rows) x m (columns) tile with cells x cells_y points."""
    m = m or n
    cy = cells_y or cells
    rng = np.random.default_rng(seed)
    pts = (rng.random((cy, cells, 2)) * jitter + (1 - jitter) / 2).astype(np.float32)
    cxv = ((np.arange(m, dtype=np.float32) + 0.5) * cells / m)[None, :]
    cyv = ((np.arange(n, dtype=np.float32) + 0.5) * cy / n)[:, None]
    ix = np.floor(cxv).astype(np.int32)
    iy = np.floor(cyv).astype(np.int32)
    F1 = np.full((n, m), 9.0, np.float32)
    F2 = np.full((n, m), 9.0, np.float32)
    ID = np.zeros((n, m), np.int32)
    for dy in (-1, 0, 1):
        jy = (iy + dy) % cy
        for dx in (-1, 0, 1):
            jx = (ix + dx) % cells
            px = (ix + dx) + pts[jy, jx, 0]
            py = (iy + dy) + pts[jy, jx, 1]
            d = np.sqrt((cxv - px) ** 2 + (cyv - py) ** 2)
            cid = jy * cells + jx
            closer = d < F1
            F2 = np.where(closer, F1, np.minimum(F2, d))
            ID = np.where(closer, cid, ID)
            F1 = np.where(closer, d, F1)
    return F1, F2, ID


def normal_from_height(h, texel, strength=1.0):
    gx = (np.roll(h, -1, 1) - np.roll(h, 1, 1)) / (2 * texel) * strength
    gr = (np.roll(h, -1, 0) - np.roll(h, 1, 0)) / (2 * texel) * strength
    nrm = np.stack([-gx, gr, np.ones_like(h)], 2)
    nrm /= np.linalg.norm(nrm, axis=2, keepdims=True)
    return nrm * 0.5 + 0.5


def cavity_ao(h, scale_px=(2, 6, 16), gain=180.0):
    ao = np.ones_like(h)
    for s in scale_px:
        b = h.copy()
        for _ in range(s):
            b = (b + np.roll(b, 1, 0) + np.roll(b, -1, 0) + np.roll(b, 1, 1) + np.roll(b, -1, 1)) / 5.0
        ao -= np.clip((b - h) * gain, 0.0, 0.35)
    return np.clip(ao, 0.25, 1.0)


def write_set(name, color_lin, rough, h, texel, metal=None, ao_gain=180.0, height=None, nstrength=1.0, tag='ns_ground'):
    base = os.path.join(TEX_DIR, f'T_NS_{name}')
    _png(base + '_BaseColor.png', lin2srgb(color_lin))
    _png(base + '_Normal.png', normal_from_height(h, texel, nstrength))
    ao = cavity_ao(h, gain=ao_gain)
    _png(base + '_ORM.png', np.stack([ao, np.clip(rough, 0.02, 1.0), np.zeros_like(rough) if metal is None else np.clip(metal, 0, 1)], 2))
    if height is not None:
        lo, hi = height
        _png(base + '_Height.png', (h - lo) / (hi - lo))
    print(f'[{tag}] wrote T_NS_{name}_* ({color_lin.shape[1]} x {color_lin.shape[0]})', flush=True)


def _grid_m(n, L):
    c = (np.arange(n, dtype=np.float64) + 0.5) * L / n
    return np.meshgrid(c, c)   # U (columns), V (rows)


def wrap_d(a, c, L):
    """Signed wrapped difference a - c on a tile of size L."""
    d = a - c
    return d - L * np.round(d / L)


def stamp_paths(n, L, paths, width_m):
    """A soft mask of polylines (meters, wrapped on the tile) of the given width."""
    mask = np.zeros((n, n), np.float32)
    px = n / L
    r = max(0.5, width_m * px / 2)
    ri = int(math.ceil(r)) + 1
    yy, xx = np.mgrid[-ri:ri + 1, -ri:ri + 1]
    for path in paths:
        for (x0, y0), (x1, y1) in zip(path[:-1], path[1:]):
            steps = int(max(abs(x1 - x0), abs(y1 - y0)) * px * 2) + 1
            for i in range(steps + 1):
                fx = (x0 + (x1 - x0) * i / steps) * px
                fy = (y0 + (y1 - y0) * i / steps) * px
                cx, cyy = int(math.floor(fx)), int(math.floor(fy))
                disc = np.clip(r + 0.5 - np.hypot(xx + cx - fx + 0.5, yy + cyy - fy + 0.5), 0.0, 1.0).astype(np.float32)
                rows = (np.arange(cyy - ri, cyy + ri + 1)) % n
                cols = (np.arange(cx - ri, cx + ri + 1)) % n
                mask[np.ix_(rows, cols)] = np.maximum(mask[np.ix_(rows, cols)], disc)
    return mask


def crack_paths(L, count, seed, curl=0.35, steps=(30, 90), step=(0.02, 0.05), angles=None, hold=0.0, starts=None, branch=0.0):
    """Random-walk crack lines (meters on the tile). angles: headings to start from (else random); hold pulls the heading
    back toward its start each step; starts: points to start from; branch: chance per step of a short side crack."""
    rng = np.random.default_rng(seed)
    paths = []
    for i in range(count):
        if starts is not None:
            x, y = starts[i % len(starts)]
        else:
            x, y = rng.uniform(0, L, 2)
        a0 = angles[i % len(angles)] + rng.normal(0, 0.12) if angles else rng.uniform(0, 2 * math.pi)
        ang = a0
        pts = [(x, y)]
        for _ in range(int(rng.uniform(*steps))):
            ang += rng.normal(0, curl) + (a0 - ang) * hold
            s = rng.uniform(*step)
            x += math.cos(ang) * s
            y += math.sin(ang) * s
            pts.append((x, y))
            if branch and rng.random() < branch:
                b_ang = ang + rng.choice([-1, 1]) * rng.uniform(0.5, 1.2)
                bx, by = x, y
                bp = [(bx, by)]
                for _ in range(int(rng.uniform(4, 14))):
                    b_ang += rng.normal(0, curl)
                    sb = rng.uniform(*step) * 0.7
                    bx += math.cos(b_ang) * sb
                    by += math.sin(b_ang) * sb
                    bp.append((bx, by))
                paths.append(bp)
        paths.append(pts)
    return paths


def blobs(n, L, rng, count, rmin, rmax, field=None, ragged=0.5, soft=(0.1, 0.7)):
    """count soft irregular blobs on the tile: (mask in 0..1, list of (cx, cy, r)). Each is a soft window whose edge the
    field breaks up (ragged 0..1) and whose inside fades (a stain is darkest where it soaked in most)."""
    U, V = _grid_m(n, L)
    mask = np.zeros((n, n), np.float32)
    out = []
    px = n / L
    for _ in range(count):
        cx, cy = rng.uniform(0, L, 2)
        r = rng.uniform(rmin, rmax)
        ax = rng.uniform(0.6, 1.0)
        th = rng.uniform(0, math.pi)
        rr = int(r * 1.9 * px) + 2
        c0, r0 = int(cx * px), int(cy * px)
        rows = np.arange(r0 - rr, r0 + rr + 1) % n
        cols = np.arange(c0 - rr, c0 + rr + 1) % n
        du = wrap_d(U[np.ix_(rows, cols)], cx, L)
        dv = wrap_d(V[np.ix_(rows, cols)], cy, L)
        a = du * math.cos(th) + dv * math.sin(th)
        b = (-du * math.sin(th) + dv * math.cos(th)) / ax
        d = np.hypot(a, b) / r
        f = field[np.ix_(rows, cols)] if field is not None else 0.5
        s = np.clip((1.0 - d) * 1.2 + (f - 0.5) * 2.2 * ragged, 0.0, 1.0)
        s = smoothstep(soft[0], soft[1], s).astype(np.float32)
        mask[np.ix_(rows, cols)] = np.maximum(mask[np.ix_(rows, cols)], s)
        out.append((cx, cy, r))
    return mask, out


# ================================================================== the texture sets

def concrete_surface(n, L, seed, base_hex, broom_amt=1.0, fines=1.0):
    """Shared broom-finished concrete: color (linear), height (m), roughness, and a (0..1) speckle for later use. Multi-scale
    tone, the sand and fine aggregate the float left at the surface, pores, a broom texture broken into strokes."""
    texel = L / n
    tone_l = tnoise(n, max(2, int(L / 1.2)), seed + 1, 5)
    tone_m = tnoise(n, int(L * 9), seed + 2, 3)
    tone_s = tnoise(n, int(L * 45), seed + 3, 2)
    col = np.broadcast_to(lin(base_hex), (n, n, 3)) * (0.9 + 0.2 * tone_l[..., None]) * (0.95 + 0.1 * tone_m[..., None]) * \
        (0.97 + 0.06 * tone_s[..., None])
    # fine aggregate: small sand grains and pebbles (2..4 mm) showing through the paste
    F1, F2, gid = tworley(n, int(L / 0.0035), seed + 4, 0.95)
    gr = _hash2(gid, 5, seed)
    grain = smoothstep(0.38, 0.22, F1) * (gr < 0.55 * fines)
    gtone = (0.78 + 0.45 * _hash2(gid, 6, seed))[..., None]
    gcol = np.where((_hash2(gid, 7, seed) < 0.25)[..., None], lin(0x6f665a), np.where((_hash2(gid, 8, seed) < 0.3)[..., None], lin(0xa9a49a),
                                                                                       lin(0x8a857c)))
    col = col * (1 - 0.55 * grain[..., None]) + gcol * gtone * 0.55 * grain[..., None]
    speck = grains(n, seed + 9, 0)
    col = col * (0.95 + 0.1 * speck[..., None])
    dark = np.clip((speck - 0.9) * 10, 0, 1)
    col = col * (1 - 0.3 * dark[..., None])
    # pores: small dark air voids
    pores = np.clip((grains(n, seed + 10, 0) - 0.94) * 16, 0, 1) * smoothstep(0.3, 0.6, tnoise(n, int(L * 6), seed + 11, 2))
    col = col * (1 - 0.45 * pores[..., None])
    # the broom: fine ridges along u, laid in strokes (each stroke a band of slightly different depth and angle)
    stroke = tnoise(n, 3, seed + 12, 2, cells_y=int(L * 8))
    ridges = tnoise(n, max(2, int(L * 2)), seed + 13, 2, cells_y=int(L * 180)) - 0.5
    ridges2 = tnoise(n, max(2, int(L * 3)), seed + 14, 1, cells_y=int(L * 420)) - 0.5
    br = (ridges * 0.7 + ridges2 * 0.5) * (0.4 + 0.6 * stroke) * broom_amt
    col = col * (1 + 0.025 * br[..., None])
    h = br * 0.00045 + (tone_m - 0.5) * 0.0008 + (tone_l - 0.5) * 0.0015 + grain * 0.00025 - pores * 0.0006 + (speck - 0.5) * 0.0001
    rough = 0.86 + 0.04 * (tone_s - 0.5) - 0.06 * grain * 0.5 + 0.04 * pores
    return col, h, rough, speck


def tex_sidewalk(variant, n=2048):
    """2 x 2 flags (1.5 m) of broom-finished concrete with tooled joints."""
    L = 3.0
    texel = L / n
    B = variant == 'B'
    seed = 211 if not B else 223
    rng = np.random.default_rng(seed)
    U, V = _grid_m(n, L)
    fu = np.floor(U / 1.5).astype(int)
    fv = np.floor(V / 1.5).astype(int)
    fid = fv * 2 + fu
    du = U - fu * 1.5
    dv = V - fv * 1.5
    e = np.minimum.reduce([du, 1.5 - du, dv, 1.5 - dv])
    col, h, rough, speck = concrete_surface(n, L, seed, 0x8a867e if not B else 0x837f77)
    # per-flag tone (poured on different days), a warm/cool cast
    tone = rng.uniform(0.93, 1.06, 4)
    warm = rng.uniform(-0.025, 0.025, 4)
    col = col * tone[fid][..., None] * (1 + np.concatenate([warm[fid][..., None], warm[fid][..., None] * 0.4, -warm[fid][..., None]], 2))
    # the edger's smooth margin either side of the joint (no broom: troweled), a little darker and glossier
    margin = 1.0 - smoothstep(0.032, 0.05, e)
    col = col * (1 - 0.04 * margin[..., None])
    rough = rough - 0.05 * margin
    # the tooled joint: a 6 mm deep groove with rounded shoulders
    groove = smoothstep(0.0045, 0.0022, e)
    h = h * (1 - 0.6 * margin) - 0.0062 * (1.0 - smoothstep(0.0022, 0.0105, e)) ** 1.6
    # dirt walked into the margins and the joints' shoulders, darker where water runs along them
    near = (1.0 - smoothstep(0.0, 0.09, e)) * (0.6 + 0.4 * tnoise(n, 30, seed + 20, 3))
    col = col * (1 - 0.12 * near[..., None])
    # wear: the walking line polished a little lighter and smoother, mid-flag
    wearf = smoothstep(0.5, 0.85, tnoise(n, 2, seed + 21, 3)) * (1 - margin)
    col = col * (1 + 0.05 * wearf[..., None])
    rough = rough - 0.05 * wearf
    # stains: grime, oil, rust, tannin (leaves), a salt bloom; ragged shapes that fade inward
    field = tnoise(n, 12, seed + 22, 5)
    kinds = [('grime', 0x4f4a42, 0.32, 0.0, 0.08, 0.45), ('oil', 0x2a2825, 0.42, -0.2, 0.05, 0.22), ('rust', 0x7d5434, 0.3, 0.0, 0.03, 0.12),
             ('tannin', 0x5d4a35, 0.22, 0.0, 0.02, 0.07), ('salt', 0xbab6ad, 0.18, 0.04, 0.1, 0.4), ('drip', 0x3e3a35, 0.25, -0.05, 0.01, 0.03)]
    count = {'grime': 6, 'oil': 2, 'rust': 1, 'tannin': 10, 'salt': 1, 'drip': 30} if not B else \
        {'grime': 10, 'oil': 3, 'rust': 2, 'tannin': 18, 'salt': 2, 'drip': 45}
    for k, hexc, amt, dr, r0, r1 in kinds:
        m, _ = blobs(n, L, rng, count[k], r0, r1, field, ragged=0.75 if k != 'drip' else 0.35, soft=(0.05, 0.8) if k != 'drip' else (0.2, 0.5))
        m = m * (0.55 + 0.45 * tnoise(n, 40, seed + 23 + len(k), 3))
        col = col * (1 - amt * m[..., None]) + lin(hexc) * amt * m[..., None]
        rough = rough + dr * m
    # spalls: chips broken out of the joints' edges (rougher, lighter, deeper)
    spall = np.zeros((n, n), np.float32)
    for _ in range(12 if not B else 20):
        along = rng.uniform(0, L)
        which = rng.integers(0, 2)
        line = rng.integers(0, 2) * 1.5
        side = rng.choice([-1, 1])
        r = rng.uniform(0.008, 0.03)
        cx, cy = (along, line + side * r * 0.5) if which == 0 else (line + side * r * 0.5, along)
        d = np.hypot(wrap_d(U, cx, L), wrap_d(V, cy, L)) / r
        spall = np.maximum(spall, smoothstep(1.0, 0.6, d + (field - 0.5) * 1.2))
    col = col * (1 - spall[..., None] * 0.15) + lin(0xa49f95) * 0.18 * spall[..., None] * (0.7 + 0.6 * speck[..., None])
    h = h - spall * 0.003 * (0.6 + 0.4 * speck)
    rough = rough + 0.05 * spall
    # hairline cracks running from a joint into the flag
    starts = [(rng.uniform(0.2, 1.3), 0.0), (1.5, rng.uniform(0.2, 1.3)), (rng.uniform(1.7, 2.8), 1.5), (0.0, rng.uniform(1.7, 2.8))]
    cr = crack_paths(L, 1 if not B else 3, seed + 24, curl=0.28, steps=(25, 70), step=(0.008, 0.02),
                     angles=[math.pi / 2, 0.0, -math.pi / 2, 0.0], hold=0.05, starts=starts, branch=0.04)
    crack = blur(stamp_paths(n, L, cr, 0.0015), 1)
    halo = blur(stamp_paths(n, L, cr, 0.012), 3) * 0.5
    col = col * (1 - 0.55 * crack[..., None]) * (1 - 0.08 * halo[..., None])
    h = h - crack * 0.0015
    # the joint: dark dirt and grit, a little moss
    jfill = lin(0x3a3631) * (0.6 + 0.8 * grains(n, seed + 25, 1)[..., None])
    moss = (smoothstep(0.7, 0.8, tnoise(n, 40, seed + 26, 3)) * groove)[..., None]
    jfill = jfill * (1 - moss) + lin(0x3b4628) * moss
    col = col * (1 - groove[..., None]) + jfill * groove[..., None]
    h = h + groove * (speck - 0.5) * 0.001
    rough = rough * (1 - groove) + 0.93 * groove
    # gum: round flattened blobs 0.8..2 cm, mostly old and dark grey, a few fresher; a dirty ring round each
    gum_cols = [0x2a2826, 0x353230, 0x403c38, 0x4d4944, 0x5b5650, 0x67625b, 0x726d66, 0x837d75]
    gcount = 110 if not B else 45
    gum = np.zeros((n, n), np.float32)
    gcol = np.zeros((n, n, 3), np.float32)
    halo = np.zeros((n, n), np.float32)
    px = n / L
    for i in range(gcount):
        cx, cy = rng.uniform(0, L, 2)
        r = rng.uniform(0.004, 0.0095) * (1.4 if rng.random() < 0.1 else 1.0)
        rr = int(r * 1.7 * px) + 2
        c0, r0 = int(cx * px), int(cy * px)
        rows = np.arange(r0 - rr, r0 + rr + 1) % n
        cols = np.arange(c0 - rr, c0 + rr + 1) % n
        du_ = wrap_d(U[np.ix_(rows, cols)], cx, L)
        dv_ = wrap_d(V[np.ix_(rows, cols)], cy, L)
        ang = np.arctan2(dv_, du_)
        wob = 1.0 + 0.12 * np.sin(ang * 3 + i) + 0.07 * np.sin(ang * 5 + 2 * i)
        d = np.hypot(du_, dv_) / (r * wob)
        g = smoothstep(1.0, 0.8, d)
        hl = smoothstep(1.6, 1.0, d) * (1 - g)
        fresh = rng.random() < 0.05
        c = lin(0x9b8a8e if fresh else gum_cols[rng.integers(0, len(gum_cols))])
        sub = gum[np.ix_(rows, cols)]
        upd = g > sub
        shade_ = (0.88 + 0.24 * smoothstep(0.0, 1.0, d))[..., None]
        gcol[np.ix_(rows, cols)] = np.where(upd[..., None], c * shade_, gcol[np.ix_(rows, cols)])
        gum[np.ix_(rows, cols)] = np.maximum(sub, g)
        halo[np.ix_(rows, cols)] = np.maximum(halo[np.ix_(rows, cols)], hl)
    col = col * (1 - 0.15 * halo[..., None])
    col = col * (1 - gum[..., None]) + gcol * gum[..., None]
    h = h + gum * 0.0007 * (1 - 0.3 * speck)
    rough = rough * (1 - gum) + 0.56 * gum
    write_set(f'Sidewalk_{variant}', col, rough, h, texel, height=(-0.008, 0.002))


def tex_asphalt(kind, n=2048):
    """kind: 'Asphalt' (old, oxidized, sealed cracks), 'Patch' (new, black, rolled) or 'BusBay' (red surfacing worn)."""
    L = 4.0
    texel = L / n
    seed = {'Asphalt': 301, 'Patch': 331, 'BusBay': 361}[kind]
    rng = np.random.default_rng(seed)
    tone = tnoise(n, 5, seed + 1, 5)
    tone_m = tnoise(n, 30, seed + 2, 3)
    fines = grains(n, seed + 3, 0)
    new = kind == 'Patch'
    # Binder: oxidized to a dark grey on old pavement, near black on new.
    binder = lin(0x1c1d20 if new else 0x2a2b2e)
    col = np.broadcast_to(binder, (n, n, 3)) * (0.88 + 0.24 * tone[..., None]) * (0.94 + 0.12 * tone_m[..., None])
    sand = np.clip((fines - 0.72) * 4, 0, 1)
    col = col * (1 - 0.3 * sand[..., None]) + lin(0x5b5853) * 0.3 * sand[..., None]
    h = (tone - 0.5) * 0.0025 + (tone_m - 0.5) * 0.0006 + (fines - 0.5) * 0.0002
    rough = 0.88 + 0.04 * (tone - 0.5)
    # Aggregate: coarse stones (7..12 mm) and fine (3..5 mm); worn binder leaves their tops showing, each its own grey.
    expo_field = tnoise(n, 9, seed + 4, 4)
    show_all = np.zeros((n, n), np.float32)
    for cells, rmin, rmax, frac, hgt, s in ((300, 0.26, 0.42, 0.85, 0.0016, 5), (720, 0.24, 0.4, 0.6, 0.0007, 6)):
        F1, F2, sid = tworley(n, cells, seed + s, 0.95)
        rad = rmin + (rmax - rmin) * _hash2(sid, 3, seed + s)
        # angular stones: the distance stretched by a per-stone direction
        inside = smoothstep(rad + 0.03, rad - 0.03, F1 * (1.0 + 0.25 * (_hash2(sid, 4, seed + s) - 0.5)))
        dome = np.clip(1.0 - (F1 / np.maximum(rad, 1e-3)) ** 2, 0, 1)
        expo = (0.25 if new else 0.55) + (0.25 if new else 0.4) * expo_field
        present = _hash2(sid, 7, seed + s) < frac
        show = inside * present * smoothstep(0.55, 0.85, dome * 0.7 + expo * 0.5 + _hash2(sid, 8, seed + s) * 0.25)
        pick = _hash2(sid, 9, seed + s)
        g = (0.7 + 0.5 * _hash2(sid, 10, seed + s))[..., None]
        stone_col = np.where((pick < 0.5)[..., None], lin(0x5f5c58), np.where((pick < 0.72)[..., None], lin(0x4a4845),
                             np.where((pick < 0.86)[..., None], lin(0x5e5448), np.where((pick < 0.97)[..., None], lin(0x7a766f), lin(0x9b968c))))) * g
        stone_col = stone_col * (0.9 + 0.2 * fines[..., None])
        if new:
            stone_col = stone_col * 0.6 + binder * 0.4
        col = col * (1 - show[..., None]) + stone_col * show[..., None]
        h = h + inside * present * dome * hgt * (0.6 + 0.4 * expo)
        rough = rough - 0.18 * show
        show_all = np.maximum(show_all, show)
        if s == 5:
            # raveling: a few stones plucked out, leaving their sockets
            voids = inside * (_hash2(sid, 11, seed + s) < (0.04 if not new else 0.01)) * smoothstep(0.5, 0.75, expo_field)
            col = col * (1 - 0.7 * voids[..., None])
            h = h - voids * 0.002
    if new:
        bands = tnoise(n, 1, seed + 12, 2, cells_y=5)
        col = col * (0.96 + 0.08 * bands[..., None])
        h = h + (bands - 0.5) * 0.0005
        rough = rough - 0.05
        hair = crack_paths(L, 1, seed + 13, curl=0.3, steps=(20, 40), step=(0.012, 0.025))
        hl = stamp_paths(n, L, hair, 0.003) * 0.35
        col = col * (1 - hl[..., None])
        h = h - hl * 0.001
    else:
        # one sealed crack running with the road (an overband of tar, 4-7 cm, squeegeed thin at its edges), low contrast;
        # and hairlines nobody sealed
        sealed = crack_paths(L, 1, seed + 14, curl=0.07, steps=(120, 160), step=(0.025, 0.035), angles=[math.pi / 2], hold=0.06)
        wid = stamp_paths(n, L, sealed, 0.055)
        band = blur(wid, 3) * (0.65 + 0.35 * tnoise(n, 60, seed + 15, 2))
        core_ = stamp_paths(n, L, sealed, 0.004)
        col = col * (1 - 0.55 * band[..., None]) + lin(0x121214) * 0.55 * band[..., None]
        h = h * (1 - 0.8 * band) + band * 0.0006 - core_ * 0.0004
        rough = rough * (1 - band) + 0.4 * band
        hair = crack_paths(L, 5, seed + 16, curl=0.32, steps=(15, 45), step=(0.012, 0.025), branch=0.05)
        hl = stamp_paths(n, L, hair, 0.003) * 0.45
        col = col * (1 - hl[..., None]) + lin(0x0c0c0d) * hl[..., None] * 0.45
        h = h - hl * 0.0015
        # oxidation and a salt bloom in patches
        ox = smoothstep(0.5, 0.75, 0.5 * tnoise(n, 7, seed + 17, 4) + 0.5 * tnoise(n, 5, seed + 18, 4))
        col = col * (1 - 0.22 * ox[..., None]) + lin(0x4a4a4b) * 0.22 * ox[..., None]
    if kind == 'BusBay':
        # Red methacrylate surfacing over the asphalt: gritty, sun-faded, worn back to the black in patches and where the
        # bus's wheels run (soft bands along u), its tires leaving faint dark tracks.
        grit = grains(n, seed + 20, 0)
        coat_col = lin(0x74302a) * (0.85 + 0.3 * tnoise(n, 9, seed + 21, 4)[..., None]) * (0.88 + 0.24 * grit[..., None])
        fade = smoothstep(0.5, 0.85, tnoise(n, 4, seed + 22, 4))[..., None]
        coat_col = coat_col * (1 - 0.3 * fade) + lin(0x92584c) * 0.3 * fade
        lanes = tnoise(n, 1, seed + 23, 3, cells_y=3)
        worn = smoothstep(0.66, 0.82, 0.6 * tnoise(n, 12, seed + 24, 5) + 0.4 * lanes)
        worn = np.maximum(worn, show_all * smoothstep(0.45, 0.75, tnoise(n, 20, seed + 25, 3)) * 0.85)
        coat = 1.0 - worn
        streak = smoothstep(0.62, 0.85, tnoise(n, 2, seed + 26, 3, cells_y=40)) * 0.18
        col = col * (1 - coat[..., None]) + coat_col * coat[..., None]
        col = col * (1 - streak[..., None]) + lin(0x1a1716) * streak[..., None]
        h = h * (1 - 0.6 * coat) + coat * (0.0005 + (grit - 0.5) * 0.0003)
        rough = rough * (1 - coat) + coat * (0.74 + 0.06 * (grit - 0.5))
    write_set(kind, col, rough, h, texel, ao_gain=110.0)


def tex_curb(n=1024):
    """Along u (2 m tile): v_world in [-0.25, 0.75] mapped to the tile by the curb UVs (v = Z - top on the face, v = meters
    back on the top): the steel curb angle |v| < 0.05 (the arris polished bright by tires), the face below (grime rising,
    tire scuffs, rust bled from the angle), the top above."""
    L = 2.0
    texel = L / n
    seed = 401
    rng = np.random.default_rng(seed)
    U, V = _grid_m(n, L)
    Vt = L - V                                  # the PNG's row 0 is the top of the tile (v = L)
    vw = np.where(Vt > 1.0, Vt - L, Vt)         # the curb's v (the tile wraps): <0 the face, >0 the top
    sand = grains(n, seed + 1, 0)
    mott = tnoise(n, 6, seed + 2, 4)
    conc = lin(0x8f8b84) * (0.9 + 0.2 * mott[..., None]) * (0.93 + 0.14 * sand[..., None])
    face = (vw < -0.05) & (vw > -0.30)
    top = vw > 0.05
    steel = np.abs(vw) <= 0.05
    # Face grime rising from the gutter, splash, scuffs along u.
    gr = smoothstep(-0.06, -0.19, vw) * face
    splash = smoothstep(0.55, 0.8, tnoise(n, 40, seed + 3, 3)) * smoothstep(-0.05, -0.16, vw)
    scuff = smoothstep(0.6, 0.85, tnoise(n, 3, seed + 4, 3, cells_y=150)) * smoothstep(-0.05, -0.08, vw) * smoothstep(-0.14, -0.1, vw)
    col = conc.copy()
    col = col * (1 - 0.55 * gr[..., None]) + lin(0x3d3a35) * 0.55 * gr[..., None]
    col = col * (1 - 0.3 * splash[..., None]) + lin(0x4a453e) * 0.3 * splash[..., None]
    col = col * (1 - 0.7 * scuff[..., None]) + lin(0x161515) * 0.7 * scuff[..., None]
    # Rust bled down the face from the angle.
    bleed = smoothstep(0.55, 0.8, tnoise(n, 9, seed + 5, 3, cells_y=2)) * smoothstep(-0.12, -0.05, vw) * face
    col = col * (1 - 0.45 * bleed[..., None]) + lin(0x7a4a2a) * 0.45 * bleed[..., None]
    # The top: worn, a little cleaner, chips at the back.
    wear = smoothstep(0.4, 0.8, tnoise(n, 12, seed + 6, 3)) * top
    col = col * (1 + 0.06 * wear[..., None])
    # The steel angle: polished at the arris, rusty dark away from it; scratches along u.
    scratch = tnoise(n, 2, seed + 7, 3, cells_y=400)
    polish = smoothstep(0.03, 0.006, np.abs(vw)) * (0.7 + 0.3 * scratch)
    rust = smoothstep(0.4, 0.7, tnoise(n, 30, seed + 8, 4)) * (1 - polish)
    st = lin(0x3a3632) * (0.85 + 0.3 * sand[..., None])
    st = st * (1 - rust[..., None]) + lin(0x5c341c) * rust[..., None] * (0.8 + 0.4 * mott[..., None])
    st = st * (1 - polish[..., None]) + lin(0x8b8781) * (0.85 + 0.3 * scratch[..., None]) * polish[..., None]
    sm = steel.astype(np.float32)
    col = col * (1 - sm[..., None]) + st * sm[..., None]
    metal = sm * (0.35 + 0.55 * polish) * (1 - 0.7 * rust)
    rough = 0.85 + 0.05 * (sand - 0.5) - 0.1 * scuff - 0.05 * wear
    rough = rough * (1 - sm) + sm * (0.62 - 0.32 * polish + 0.2 * rust)
    h = (sand - 0.5) * 0.0004 + (mott - 0.5) * 0.001 + scuff * 0.0002
    h = h * (1 - sm) + sm * (0.0008 * rust * (tnoise(n, 60, seed + 9, 2) - 0.3))
    # The angle's lower edge: a small step down to the face (its flange stands 6 mm proud).
    h = h + 0.0015 * smoothstep(-0.052, -0.047, vw) * smoothstep(0.052, 0.047, vw)
    write_set('Curb', col, rough, h, texel, metal=metal)


def tex_gutter(n=1024):
    """The gutter pan: smooth-troweled concrete, darker, silted where water slows, damp-stained, a film of algae and oil."""
    L = 2.0
    texel = L / n
    seed = 431
    col, h, rough, speck = concrete_surface(n, L, seed, 0x726e67, broom_amt=0.25, fines=0.7)
    mott = tnoise(n, 4, seed + 30, 5)
    col = col * (0.85 + 0.3 * mott[..., None])
    silt = smoothstep(0.55, 0.78, 0.6 * tnoise(n, 7, seed + 31, 5) + 0.4 * tnoise(n, 23, seed + 32, 3))
    sandg = grains(n, seed + 33, 0)
    col = col * (1 - 0.55 * silt[..., None]) + lin(0x86796a) * (0.8 + 0.4 * sandg[..., None]) * 0.55 * silt[..., None]
    damp = smoothstep(0.45, 0.7, tnoise(n, 5, seed + 34, 4))
    col = col * (1 - 0.18 * damp[..., None])
    alg = smoothstep(0.62, 0.82, tnoise(n, 6, seed + 35, 4)) * (1 - silt)
    col = col * (1 - 0.22 * alg[..., None]) + lin(0x3a4630) * 0.22 * alg[..., None]
    oil = smoothstep(0.72, 0.86, tnoise(n, 10, seed + 36, 4))
    col = col * (1 - 0.35 * oil[..., None]) + lin(0x24221e) * 0.35 * oil[..., None]
    hair = crack_paths(L, 2, seed + 37, curl=0.3, steps=(20, 50), step=(0.01, 0.02))
    hl = stamp_paths(n, L, hair, 0.0025) * 0.5
    col = col * (1 - hl[..., None])
    h = h + silt * 0.0005 * sandg - hl * 0.001 + (mott - 0.5) * 0.001
    rough = rough - 0.06 - 0.12 * oil + 0.05 * silt - 0.05 * damp
    write_set('Gutter', col, rough, h, texel)


def tex_concrete(n=1024):
    """New concrete (ramps, aprons, replaced flags): lighter, a crisp broom, little wear."""
    L = 2.0
    texel = L / n
    seed = 461
    col, h, rough, speck = concrete_surface(n, L, seed, 0x9c988f, broom_amt=1.3, fines=0.6)
    dirt = smoothstep(0.6, 0.85, tnoise(n, 8, seed + 5, 4))
    col = col * (1 - 0.12 * dirt[..., None])
    rough = rough + 0.02
    write_set('Concrete', col, rough, h, texel)


def tex_tactile(n=512):
    """Truncated domes, 12 x 12 on 50 mm centers (25 mm base, 13 mm top, 5 mm tall), safety yellow, worn."""
    L = 0.6
    texel = L / n
    seed = 491
    U, V = _grid_m(n, L)
    du = (U % 0.05) - 0.025
    dv = (V % 0.05) - 0.025
    r = np.hypot(du, dv)
    dome = np.clip((0.0125 - r) / (0.0125 - 0.0068), 0, 1)
    top = r < 0.0068
    hdome = 0.005 * smoothstep(0.0, 1.0, dome)
    hdome = np.where(top, 0.005 - 0.0006 * (r / 0.0068) ** 2, hdome)
    sand = grains(n, seed + 1, 0)
    mott = tnoise(n, 4, seed + 2, 4)
    col = lin(0xcf9f1c) * (0.9 + 0.2 * mott[..., None]) * (0.95 + 0.1 * sand[..., None])
    # Dirt between the domes, the dome tops scuffed paler and greyer.
    low = smoothstep(0.003, 0.0, hdome)
    col = col * (1 - 0.35 * low[..., None]) + lin(0x4e4532) * 0.35 * low[..., None]
    scuff = top.astype(np.float32) * (0.4 + 0.6 * tnoise(n, 12, seed + 3, 2))
    col = col * (1 - 0.4 * scuff[..., None]) + lin(0xb4a77a) * 0.4 * scuff[..., None]
    h = hdome + (sand - 0.5) * 0.0002
    rough = 0.62 - 0.12 * scuff + 0.1 * low
    write_set('Tactile', col, rough, h, texel, height=(0.0, 0.0055), ao_gain=120.0)


def tex_thermo(n=1024):
    """Thermoplastic, near white (the vertex paint multiplies it): bumpy, glass beads, alligator cracks, tire scuffs; and
    T_NS_Thermo_Wear: the pattern it wears off in (worn where Wear < the vertex A)."""
    L = 2.0
    texel = L / n
    seed = 521
    sand = grains(n, seed + 1, 0)
    lump = tnoise(n, 60, seed + 2, 3)
    mott = tnoise(n, 5, seed + 3, 4)
    col = np.ones((n, n, 3), np.float32) * (0.9 + 0.08 * mott[..., None]) * (0.95 + 0.05 * sand[..., None])
    F1, F2, sid = tworley(n, 26, seed + 4)
    crack = smoothstep(0.035, 0.0, F2 - F1) * smoothstep(0.5, 0.7, tnoise(n, 6, seed + 5, 3))
    col = col * (1 - 0.55 * crack[..., None])
    scuff = smoothstep(0.66, 0.88, tnoise(n, 3, seed + 6, 3, cells_y=30)) * 0.22
    col = col * (1 - scuff[..., None]) + np.array([0.06, 0.06, 0.06]) * scuff[..., None]
    dirt = smoothstep(0.55, 0.8, tnoise(n, 10, seed + 7, 4)) * 0.25
    col = col * (1 - dirt[..., None]) + lin(0x6b665e) * dirt[..., None]
    beads = np.clip((grains(n, seed + 8, 0) - 0.93) * 14, 0, 1)
    h = (lump - 0.5) * 0.0006 + (sand - 0.5) * 0.0002 - crack * 0.0008 + beads * 0.0002
    rough = 0.55 - 0.3 * beads + 0.08 * dirt + 0.1 * crack
    write_set('Thermo', col, rough, h, texel)
    wearf = 0.55 * tnoise(n, 7, seed + 9, 5) + 0.3 * tnoise(n, 30, seed + 10, 3) + 0.15 * sand
    wearf = (wearf - wearf.min()) / (wearf.max() - wearf.min())
    _png(os.path.join(TEX_DIR, 'T_NS_Thermo_Wear.png'), wearf)
    log('wrote T_NS_Thermo_Wear')


def _sheet_np(s, name, px):
    """Renders a Sheet and returns its pixels (H, W, 4), row 0 at the top, linear-ish 0..1 (sRGB-encoded color)."""
    color, _ = s.render(name, px, samples=8)
    img = bpy.data.images.load(color)
    w, h = img.size
    a = np.array(img.pixels[:], dtype=np.float32).reshape(h, w, 4)[::-1].copy()
    bpy.data.images.remove(img)
    for o in [o for o, *_ in s.items] + [o for o in bpy.data.objects if o.name.startswith('sheet_cam')]:
        bpy.data.objects.remove(o)
    return a


def _srgb2lin(c):
    c = np.clip(c, 0, 1)
    return np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055) ** 2.4)


def _cell_grid(S):
    c = (np.arange(S, dtype=np.float32) + 0.5) / S
    U, V = np.meshgrid(c, 1.0 - c)       # V up (row 0 at the top)
    return U, V


def _blob_mask(U, V, cx, cy, r, field, ragged=0.6, soft=(0.1, 0.7), aspect=1.0):
    d = np.hypot((U - cx) / aspect, V - cy) / r
    s = np.clip((1.0 - d) * 1.2 + (field - 0.5) * 2.2 * ragged, 0, 1)
    return smoothstep(soft[0], soft[1], s).astype(np.float32)


def _leaf_poly(cx, cy, size, ang, rng):
    """A plane leaf's outline: five pointed, toothed lobes (the lower pair smaller), the stem notch at local -y."""
    pts = []
    ca, sa = math.cos(ang), math.sin(ang)
    for k in range(160):
        t = 2 * math.pi * k / 160
        lobe = abs(math.cos(2.5 * (t - math.pi / 2))) ** 1.6
        r = size * (0.56 + 0.44 * lobe) * (1.0 + 0.03 * math.sin(t * 46.0) + rng.uniform(-0.015, 0.015))
        if math.sin(t) < -0.2:
            r *= 0.75 + 0.25 * (1 + math.sin(t)) / 0.8
        if math.sin(t) < -0.93:
            r *= 0.72
        x, y = r * math.cos(t), r * math.sin(t)
        pts.append((cx + x * ca - y * sa, cy + x * sa + y * ca))
    return pts


def _window(U, V, x0, x1, y0, y1):
    """Row and column slices of the cell grid covering u x0..x1, v y0..y1 (row 0 at the top)."""
    S = U.shape[1]
    c0, c1 = max(0, int(x0 * S) - 2), min(S, int(x1 * S) + 3)
    r0, r1 = max(0, int((1 - y1) * S) - 2), min(S, int((1 - y0) * S) + 3)
    return slice(r0, r1), slice(c0, c1)


def _pip_np(px, py, poly):
    inside = np.zeros(px.shape, bool)
    n = len(poly)
    for i in range(n):
        x0, y0 = poly[i]
        x1, y1 = poly[(i + 1) % n]
        cond = (y0 > py) != (y1 > py)
        xi = x0 + (py - y0) * (x1 - x0) / ((y1 - y0) if abs(y1 - y0) > 1e-12 else 1e-12)
        inside ^= cond & (px < xi)
    return inside


def decal_cells(S=1024):
    """The 16 cells of T_NS_Decals, each (rgb linear, alpha, height (0..1 scale ~2 mm), roughness)."""
    rng = np.random.default_rng(4096)
    pr = random.Random(4096)
    U, V = _cell_grid(S)
    cells = []
    f1 = tnoise(S, 6, 5001, 5)
    f2 = tnoise(S, 14, 5002, 4)
    f3 = tnoise(S, 40, 5003, 3)
    grain = grains(S, 5004, 0)

    def empty():
        return np.zeros((S, S, 3), np.float32), np.zeros((S, S), np.float32), np.zeros((S, S), np.float32), np.full((S, S), 0.8, np.float32)

    # 0, 1: oil stains (a big soak and a drip trail)
    for k in range(2):
        rgb, a, h, r = empty()
        if k == 0:
            m = _blob_mask(U, V, 0.5, 0.5, 0.38, 0.6 * f1 + 0.4 * f2, 0.8, (0.05, 0.85))
        else:
            m = np.zeros_like(U)
            for j in range(9):
                m = np.maximum(m, _blob_mask(U, V, 0.5 + 0.03 * math.sin(j), 0.15 + 0.09 * j, 0.06 + 0.04 * (j % 3), f2, 0.7))
        m = m * (0.75 + 0.25 * f3)
        rgb[:] = lin(0x161412)
        rgb = rgb * (1 + 0.6 * (f3[..., None] - 0.5))
        a = np.clip(m * 0.85, 0, 1)
        r = 0.2 + 0.3 * (1 - m)
        cells.append((rgb, a, h, r))
    # 2: a gum cluster (flattened blots: fresh ones dark and glossy, old ones grey, dusty and cracked at the rim)
    rgb, a, h, r = empty()
    r[:] = 0.8
    for j in range(38):
        cx, cy = rng.uniform(0.12, 0.88, 2)
        rr = rng.uniform(0.012, 0.032)
        ys, xs = _window(U, V, cx - 2 * rr, cx + 2 * rr, cy - 2 * rr, cy + 2 * rr)
        u, v = U[ys, xs], V[ys, xs]
        d = np.hypot(u - cx, (v - cy) * rng.uniform(0.8, 1.2)) / rr + (f3[ys, xs] - 0.5) * 0.9 + (grain[ys, xs] - 0.5) * 0.3
        g = smoothstep(1.0, 0.85, d)
        age = (j % 5) / 4.0
        c = lin(0x262422) * (1 - age) + lin(0x6f6a63) * age
        c = c * (0.85 + 0.3 * f3[ys, xs, None]) * (1 + 0.25 * smoothstep(0.6, 1.0, d)[..., None])
        sel = (g > a[ys, xs])[..., None]
        rgb[ys, xs] = np.where(sel, c, rgb[ys, xs])
        a[ys, xs] = np.maximum(a[ys, xs], g)
        h[ys, xs] = np.maximum(h[ys, xs], g * (0.35 + 0.25 * (1 - age)) * smoothstep(1.0, 0.5, d))
        r[ys, xs] = np.where(g > 0.5, 0.35 + 0.45 * age, r[ys, xs])
    cells.append((rgb, a, h, r))
    # 3, 4: wet leaves (plane leaves: palmate veins, a petiole, mottled browns and yellows, wet and glossy)
    for k in range(2):
        rgb, a, h, r = empty()
        r[:] = 0.8
        for j in range(16 if k == 0 else 26):
            cx, cy = rng.uniform(0.15, 0.85, 2)
            size = rng.uniform(0.07, 0.12)
            ang = rng.uniform(0, 2 * math.pi)
            poly = _leaf_poly(cx, cy, size, ang, pr)
            ys, xs = _window(U, V, cx - 1.6 * size, cx + 1.6 * size, cy - 1.6 * size, cy + 1.6 * size)
            u, v = U[ys, xs], V[ys, xs]
            m = blur(_pip_np(u, v, poly).astype(np.float32), 1)
            ca, sa = math.cos(-ang), math.sin(-ang)
            lu, lv = (u - cx) * ca - (v - cy) * sa, (u - cx) * sa + (v - cy) * ca
            by = lv + 0.32 * size                     # the veins radiate from the base of the blade
            vein = np.zeros_like(lu)
            for th in (90, 18, 162, -40, 220):
                dx, dy = math.cos(math.radians(th)), math.sin(math.radians(th))
                along = lu * dx + by * dy
                perp = np.abs(-lu * dy + by * dx)
                w = 0.0035 * np.clip(1 - along / (1.3 * size), 0.25, 1)
                vein = np.maximum(vein, smoothstep(w, 0.0, perp) * (along > 0))
            pet = smoothstep(0.003, 0.0, np.abs(lu + 0.03 * (by / size) ** 2 * size)) * (by < 0.02 * size) * (by > -0.5 * size)
            vein = vein * m
            rad = np.hypot(lu, lv) / size
            base = lin([0x8a5e24, 0xa8822e, 0x6a4a22, 0xbf9636, 0x553f22, 0x9a6a2a][(j + 3 * k) % 6])
            mott = tnoise(S, 22, 6100 + j + 40 * k, 3)[ys, xs]
            c = base[None, None] * (0.8 + 0.4 * mott[..., None])
            c = c * (1 - 0.35 * smoothstep(0.55, 1.0, rad)[..., None])                 # the wet edges darker
            spots = smoothstep(0.72, 0.8, tnoise(S, 60, 6200 + j, 2)[ys, xs])
            c = c * (1 - 0.5 * spots[..., None])
            c = c * (1 - 0.25 * vein[..., None]) + lin(0xc8a050) * 0.25 * vein[..., None] * (k == 1)
            c = np.where((pet > 0.3)[..., None], lin(0x5a4020), c)
            mm = np.maximum(m, pet)
            sel = (mm > 0.5)[..., None]
            rgb[ys, xs] = np.where(sel, c, rgb[ys, xs])
            a[ys, xs] = np.maximum(a[ys, xs], mm)
            h[ys, xs] = np.where(mm > 0.5, np.maximum(h[ys, xs] + 0.08, 0.25 + 0.2 * mott + 0.25 * vein + 0.3 * pet), h[ys, xs])
            r[ys, xs] = np.where(mm > 0.5, 0.22 + 0.15 * mott, r[ys, xs])
        cells.append((rgb, a, np.clip(h, 0, 1), r))
    # 5: cigarette butts
    rgb, a, h, r = empty()
    for j in range(14):
        cx, cy = rng.uniform(0.15, 0.85, 2)
        ang = rng.uniform(0, math.pi)
        ca, sa = math.cos(ang), math.sin(ang)
        lu = (U - cx) * ca + (V - cy) * sa
        lv = -(U - cx) * sa + (V - cy) * ca
        L, W = rng.uniform(0.05, 0.08), 0.014
        body = (np.abs(lu) < L / 2) & (np.abs(lv) < W / 2)
        filt = body & (lu < -L / 2 + L * 0.45)
        ash = body & (lu > L / 2 - 0.006)
        c = np.where(filt[..., None], lin(0xc99a5a), lin(0xe8e2d6))
        c = np.where(ash[..., None], lin(0x3a3530), c)
        rgb = np.where(body[..., None], c * (0.85 + 0.25 * f3[..., None]), rgb)
        a = np.maximum(a, body.astype(np.float32))
        h = np.maximum(h, body * 0.5)
    r = np.full((S, S), 0.7, np.float32)
    cells.append((rgb, blur(a, 1), h, r))
    # 6: wet newspaper (a page lying askew: masthead, headline, photo, five columns of type, the fold, a torn corner, soggy)
    rgb, a, h, r = empty()
    ang = math.radians(8.0)
    su = (U - 0.5) * math.cos(ang) + (V - 0.5) * math.sin(ang)
    sv = -(U - 0.5) * math.sin(ang) + (V - 0.5) * math.cos(ang)
    rag = (tnoise(S, 90, 5011, 2) - 0.5) * 0.008
    paper = ((np.abs(su) < 0.37 + rag) & (np.abs(sv) < 0.45 + rag)).astype(np.float32)
    torn = (su + sv * 0.8) > 0.56 + (tnoise(S, 40, 5012, 3) - 0.5) * 0.08
    paper = paper * (1 - torn)
    pu, pv = (su + 0.37) / 0.74, (sv + 0.45) / 0.90           # 0..1 across the page, pv up
    ink = np.zeros_like(U)
    ink = np.maximum(ink, ((pv > 0.93) & (pv < 0.975) & (np.abs(pu - 0.5) < 0.4)) * 0.85)          # the masthead
    ink = np.maximum(ink, ((pv > 0.915) & (pv < 0.92)) * 0.7)                                       # its rule
    for k2, (y0, y1) in enumerate(((0.84, 0.885), (0.79, 0.83))):                                   # headline bars
        ink = np.maximum(ink, ((pv > y0) & (pv < y1) & (pu > 0.05) & (pu < 0.95 - 0.2 * k2)) * (0.8 - 0.1 * k2))
    photo = (pu > 0.05) & (pu < 0.58) & (pv > 0.5) & (pv < 0.77)
    col = np.floor(pu * 5)
    inc = (pu * 5 - col > 0.06) & (pu * 5 - col < 0.94) & (pu > 0.03) & (pu < 0.97)
    row = np.floor(pv * 120)
    hsh = (np.sin(row * 12.9898 + col * 78.233) * 43758.5453) % 1.0
    lineend = 0.2 + 0.78 * np.where(hsh > 0.15, 1.0, hsh * 4)
    typeset = inc & ((pv * 120 - row) < 0.5) & (pv > 0.04) & (pv < 0.77) & ~photo & (((pu * 5 - col) - 0.06) / 0.88 < lineend)
    ink = np.maximum(ink, typeset * (0.55 + 0.2 * grain))
    ink = np.where(photo, 0.25 + 0.6 * tnoise(S, 12, 5013, 4), ink)
    fold = smoothstep(0.006, 0.0, np.abs(su - 0.0))
    soggy = smoothstep(0.5, 0.75, 0.6 * f2 + 0.4 * f1)
    rim = smoothstep(0.3, 0.0, np.minimum(0.37 - np.abs(su), 0.45 - np.abs(sv)) * 20)
    paperc = lin(0xcfc8b6) * (0.9 + 0.12 * f3[..., None]) * (1 - 0.12 * rim[..., None])
    rgb = paperc * (1 - ink[..., None] * 0.8) * (1 - 0.32 * soggy[..., None]) * (1 - 0.15 * fold[..., None])
    a = blur(paper, 1)
    h = a * (0.25 + 0.35 * tnoise(S, 10, 5014, 4) + 0.3 * fold - 0.15 * soggy)
    r = 0.75 - 0.45 * soggy
    cells.append((rgb, a, np.clip(h, 0, 1), r))
    # 7: flyer scraps
    rgb, a, h, r = empty()
    for j in range(22):
        cx, cy = rng.uniform(0.1, 0.9, 2)
        poly = [(cx + rng.uniform(-0.07, 0.07), cy + rng.uniform(-0.07, 0.07)) for _ in range(6)]
        poly = sorted(poly, key=lambda p: math.atan2(p[1] - cy, p[0] - cx))
        m = _pip_np(U, V, poly)
        c = lin([0xf0e4a0, 0xe8a8b8, 0xa8d0e8, 0xf2f0ea, 0xf0c060][j % 5]) * (0.8 + 0.2 * f3[..., None])
        rgb = np.where(m[..., None], c, rgb)
        a = np.maximum(a, m.astype(np.float32))
    h = a * 0.2
    cells.append((rgb, blur(a, 1), h, np.full((S, S), 0.65, np.float32)))
    # 8: a tire skid (two long smears of rubber, striated along the travel, fading in and out)
    rgb, a, h, r = empty()
    m = np.zeros_like(U)
    stri = tnoise(S, 160, 5015, 2, cells_y=2)
    blot = tnoise(S, 8, 5016, 4)
    for off in (-0.17, 0.17):
        d = np.abs(U - 0.5 - off - 0.05 * (V - 0.5) ** 2 * np.sign(off))
        band = smoothstep(0.075, 0.055, d) * smoothstep(0.02, 0.35, V + (blot - 0.5) * 0.2) * smoothstep(1.0, 0.55, V + (blot - 0.5) * 0.2)
        m = np.maximum(m, band * (0.45 + 0.55 * stri) * (0.7 + 0.3 * blot))
    rgb[:] = lin(0x0d0d0e)
    a = np.clip(m * 0.85, 0, 1)
    r = 0.75 - 0.25 * a
    cells.append((rgb, a, h, r))
    # 9: a rust drip (from a bolt or a fire escape's foot, running down a wall)
    rgb, a, h, r = empty()
    streak = tnoise(S, 30, 5020, 3, cells_y=2)
    m = smoothstep(0.55, 0.85, streak) * smoothstep(0.0, 0.85, V) * smoothstep(0.05, 0.25, 1 - np.abs(U - 0.5) * 2)
    m = np.maximum(m, _blob_mask(U, V, 0.5, 0.9, 0.08, f3, 0.5))
    rgb[:] = lin(0x7a4a26)
    rgb = rgb * (0.7 + 0.5 * f3[..., None])
    a = np.clip(m * 0.85, 0, 1)
    cells.append((rgb, a, h, np.full((S, S), 0.85, np.float32)))
    # 10: a water stain (a dried tide ring, lighter at its edge)
    rgb, a, h, r = empty()
    m = _blob_mask(U, V, 0.5, 0.5, 0.36, f1 * 0.6 + f2 * 0.4, 0.8, (0.05, 0.4))
    edge = m * (1 - smoothstep(0.3, 0.6, m))
    rgb[:] = lin(0x55524b)
    rgb = rgb * (1 - edge[..., None]) + lin(0xb8b4a8) * edge[..., None]
    a = np.clip(m * 0.45 + edge * 0.4, 0, 1)
    cells.append((rgb, a, h, np.full((S, S), 0.75, np.float32)))
    # 11: a soot streak (rising from a vent, or under an AC unit)
    rgb, a, h, r = empty()
    streak = tnoise(S, 12, 5030, 4, cells_y=3)
    m = smoothstep(0.15, 0.45, 1 - np.abs(U - 0.5) * 2 * (1.2 - 0.4 * V)) * smoothstep(0.0, 0.9, V) * (0.6 + 0.6 * streak)
    rgb[:] = lin(0x161514)
    a = np.clip(m * 0.75, 0, 1)
    cells.append((rgb, a, h, np.full((S, S), 0.9, np.float32)))
    # 12, 13: wheat-pasted posters (a boxing night; a lost cat), wrinkled, torn, wet
    for k in range(2):
        s = Sheet(700, 1000)
        if k == 0:
            s.rect(0, 0, 700, 1000, 0xe2c84a)
            s.text('FIGHT', 350, 800, 150, 0x161414, 'Black', 'CENTER')
            s.text('NIGHT', 350, 650, 150, 0xb3121b, 'Black', 'CENTER')
            s.text('IRONHOUSE vs. ALL COMERS', 350, 540, 38, 0x161414, 'Black', 'CENTER')
            s.text('SATURDAY · 9 PM', 350, 420, 60, 0x161414, 'Black', 'CENTER')
            s.text('MARKET ST · DOORS 8', 350, 340, 44, 0x161414, 'Bold', 'CENTER')
            s.rect(60, 120, 580, 120, 0x161414)
            s.text('$10 AT THE DOOR', 350, 155, 60, 0xe2c84a, 'Black', 'CENTER')
        else:
            s.rect(0, 0, 700, 1000, 0xf2f0ea)
            s.text('LOST CAT', 350, 860, 110, 0x161414, 'Black', 'CENTER')
            s.rect(140, 380, 420, 420, 0x6b6560)
            s.circle(350, 600, 120, 0x2b2826)
            s.poly([(250, 680), (285, 760), (310, 690)], 0x2b2826)
            s.poly([(450, 680), (415, 760), (390, 690)], 0x2b2826)
            s.circle(310, 610, 14, 0xc8c060)
            s.circle(390, 610, 14, 0xc8c060)
            s.text('"BISCUIT"  grey tabby, shy', 350, 300, 40, 0x161414, 'Bold', 'CENTER')
            s.text('Last seen Fifth & Market', 350, 240, 36, 0x161414, 'Bold', 'CENTER')
            s.text('555-0133  ·  REWARD', 350, 160, 50, 0xb3121b, 'Black', 'CENTER')
        px = _sheet_np(s, f'nsg_poster_{k}', 680)
        rgb, a, h, r = empty()
        ph, pw = px.shape[:2]
        y0, x0 = (S - ph) // 2, (S - pw) // 2
        art = np.zeros((S, S, 3), np.float32)
        art[y0:y0 + ph, x0:x0 + pw] = _srgb2lin(px[..., :3])
        paper = np.zeros((S, S), np.float32)
        paper[y0:y0 + ph, x0:x0 + pw] = 1.0
        pu, pv = (U - 0.5) / (pw / S / 2), (V - 0.5) / (ph / S / 2)       # -1..1 over the poster
        edge = np.maximum(np.abs(pu), np.abs(pv)) + (tnoise(S, 70, 5040 + k, 3) - 0.5) * 0.06
        paper = paper * smoothstep(1.0, 0.985, edge)
        chunk = 0.55 * (pu if k == 0 else -pu) - 0.8 * pv + (tnoise(S, 14, 5045 + k, 4) - 0.5) * 0.35
        gone = smoothstep(0.95, 1.0, chunk)
        fibre = smoothstep(0.85, 0.95, chunk) * (1 - gone)
        paper = paper * (1 - gone)
        wr = 0.6 * tnoise(S, 26, 5050 + k, 4, cells_y=10) + 0.4 * tnoise(S, 70, 5055 + k, 2)
        paste = smoothstep(0.62, 0.75, tnoise(S, 40, 5060 + k, 3, cells_y=3)) * smoothstep(0.2, 1.0, pv)
        fade = 0.05 + 0.07 * f1
        art = art * (1 - fade[..., None]) + lin(0xe8e2d2) * fade[..., None]
        rgb = art * (0.94 + 0.1 * wr[..., None]) * (1 - 0.18 * paste[..., None]) * (1 - 0.15 * smoothstep(0.55, 0.85, f2)[..., None])
        rgb = rgb * (1 - fibre[..., None]) + lin(0xe6e0d4) * fibre[..., None]
        a = paper
        h = paper * (0.2 + 0.7 * wr + 0.2 * fibre)
        r = 0.72 - 0.25 * paste - 0.15 * smoothstep(0.55, 0.85, f2)
        cells.append((rgb, a, h, r))
    # 14, 15: tags (fictional names), spray paint with overspray and a drip
    for k, (word, hexc) in enumerate((('KRAYO', 0xe8e8e8), ('DUSK', 0xd84a8a))):
        s = Sheet(1000, 1000)
        s.rect(0, 0, 1000, 1000, 0x000000)
        s.text(word, 500, 420, 300 if len(word) < 5 else 240, 0xffffff, 'Black', 'CENTER', rot=0.1, tracking=0.75)
        s.text('2026', 720, 280, 70, 0xffffff, 'Black', 'CENTER', rot=0.1)
        px = _sheet_np(s, f'nsg_tag_{k}', S)
        m = px[:S, :S, 0]
        m = blur(m, 2)
        over = blur(m, 6) * 0.35
        drip = np.zeros_like(m)
        for j in range(6):
            x = rng.uniform(0.25, 0.75)
            col = int(x * S)
            rows = np.nonzero(m[:, col] > 0.5)[0]
            if not len(rows):
                continue
            bot = int(rows.max())          # paint runs down from the letter's lowest edge (row 0 is the top)
            ln = int(rng.uniform(0.05, 0.18) * S)
            drip[bot:bot + ln, col - 2:col + 3] = 1.0
            drip[min(S - 1, bot + ln):min(S, bot + ln + 6), col - 3:col + 4] = 1.0
        a = np.clip(np.maximum(m, drip) + over, 0, 1)
        rgb = np.broadcast_to(lin(hexc), (S, S, 3)).copy()
        cells.append((rgb, a, a * 0.05, np.full((S, S), 0.55, np.float32)))
    return cells


def tex_decals(n=4096):
    """T_NS_Decals_BaseColor (RGBA, A = opacity), _Normal, _ORM: a 4 x 4 atlas of 1024 cells (row-major from the top left):
    0-1 oil stains, 2 gum cluster, 3-4 wet leaves, 5 cigarette butts, 6 wet newspaper, 7 flyer scraps, 8 tire skid, 9 rust
    drip, 10 water stain, 11 soot streak, 12 wheat-paste poster (FIGHT NIGHT), 13 poster (LOST CAT), 14-15 tags (KRAYO, DUSK)."""
    S = n // 4
    cells = decal_cells(S)
    col = np.zeros((n, n, 3), np.float32)
    alpha = np.zeros((n, n), np.float32)
    hgt = np.zeros((n, n), np.float32)
    rough = np.ones((n, n), np.float32) * 0.8
    for k, (rgb, a, h, r) in enumerate(cells):
        rr, cc = k // 4, k % 4
        col[rr * S:(rr + 1) * S, cc * S:(cc + 1) * S] = rgb
        alpha[rr * S:(rr + 1) * S, cc * S:(cc + 1) * S] = a
        hgt[rr * S:(rr + 1) * S, cc * S:(cc + 1) * S] = h
        rough[rr * S:(rr + 1) * S, cc * S:(cc + 1) * S] = r
    base = os.path.join(TEX_DIR, 'T_NS_Decals')
    _png(base + '_BaseColor.png', np.dstack([lin2srgb(col), alpha]))
    _png(base + '_Normal.png', normal_from_height(hgt * 0.002, 4.0 / n, 1.0))
    _png(base + '_ORM.png', np.stack([np.ones_like(rough), np.clip(rough, 0.02, 1), np.zeros_like(rough)], 2))
    log('wrote T_NS_Decals_* (4096, 4 x 4 cells)')


def make_textures():
    t0 = time.time()
    tex_sidewalk('A')
    tex_sidewalk('B')
    for k in ('Asphalt', 'Patch', 'BusBay'):
        tex_asphalt(k)
    tex_curb()
    tex_gutter()
    tex_concrete()
    tex_tactile()
    tex_thermo()
    tex_decals()
    log(f'texture sets in {time.time() - t0:.0f}s')


# ================================================================== the plan (stage meters)

RX0, RX1, FAR = 4.2, 16.2, 20.4
FC = 10.2
HN, HR0, HR1, HSL = -62.0, -66.2, -76.2, -80.4
HC = -71.2
WSL, WR0, WR1, WNL = 92.0, 96.2, 106.2, 110.4
WC = 101.2
MSL, MS, MN, MNL = 49.5, 54.0, 65.0, 69.5
MC = 59.5
MEAST = -96.0
CR0, CR1, CEL = -100.2, -110.2, -114.4
CC = -105.2
XM0, XM1 = -38.0, 58.4          # the Fifth meshes along the cross streets
XF0, XF1 = -90.0, 130.2         # Far's ends on Hollis and Wexler
YC0, YC1 = 14.0, 104.0          # the Cardinal mesh
YF0, YF1 = -50.0, 170.0         # Far's ends on Cardinal
CR = 1.5        # curb return radius at the face
CW = 0.2        # curb width
GW = 0.45       # gutter pan width
FLOW = -0.17
RECESS = 0.002
ZB_RAMP = -0.165

# Corners: center, the arc's angles (deg, from a0 to a1 as the chain runs).
CORNERS = {
    'hollis_home': ((RX0 - CR, HR0 + CR), -90.0, 0.0),
    'hollis_far': ((RX1 + CR, HR0 + CR), 180.0, 270.0),
    'lp': ((RX0 - CR, MS - CR), 0.0, 90.0),
    'ruby': ((RX0 - CR, MN + CR), -90.0, 0.0),
    'wexler_home': ((RX0 - CR, WR0 - CR), 0.0, 90.0),
    'wexler_far': ((RX1 + CR, WR0 - CR), 90.0, 180.0),
    'card_s': ((CR0 + CR, MS - CR), 90.0, 180.0),
    'card_n': ((CR0 + CR, MN + CR), 180.0, 270.0),
}

TREE_PITS = [(3.3, -47.0), (3.3, -29.0), (3.3, 15.0), (17.3, -32.0), (17.3, -16.0), (17.3, 55.0)] + \
            [(x, 53.1) for x in (-33.0, -43.0, -51.0, -60.0, -78.0)] + [(x, 65.9) for x in (-19.0, -38.0, -48.0, -57.0, -66.0)]
MANHOLES = [(7.8, -48.0), (7.8, -9.0), (12.6, 33.0), (11.0, 83.0), (-11.0, 58.0), (-43.0, 61.0), (-52.0, 60.0), (-82.0, 58.0)]
VENT_MANHOLES = [(-52.0, 60.0)]
# Curb inlets: (curb key, along coordinate): 'x' curbs give Y, 'y' curbs give X.
INLETS = [('home', -59.0), ('home', 22.0), ('far', -18.5), ('far', 49.0), ('far', 89.0), ('ms', -13.0), ('ms', -64.0), ('mn', -7.0),
          ('mn', -93.0), ('hollis_n', 30.0), ('wexler_s', -10.0)]
VAULTS = [(1.5, 12.0, 'x'), (-17.0, 50.5, 'y'), (-39.0, 68.5, 'y')]   # center, the long side along Y ('x' fronts) or X
PATCHES = [(5.2, 7.8, -40.0, -38.4), (11.0, 14.6, -12.0, -10.8), (7.2, 8.4, 8.0, 9.6), (12.4, 15.4, 26.0, 29.5), (4.65, 8.2, 40.0, 41.2),
           (9.0, 14.0, 80.0, 81.0), (-11.0, -8.0, 56.0, 58.0), (-31.0, -28.0, 60.0, 63.0), (-55.0, -52.0, 55.0, 57.3), (-70.0, -66.0, 61.0, 62.5),
           (-86.5, -84.0, 57.0, 59.0), (-51.0, -45.0, 59.7, 62.4)]
POTHOLES = [(13.0, -25.0, 0.34), (7.0, 75.0, 0.28), (-40.0, 63.5, 0.4)]
BAY = (-35.0, -15.0, MS + GW, 56.4)


class Ramp:
    """A curb ramp (or a driveway apron) cut into the sidewalk at a straight curb: axis 'x' = the curb face is the line
    X = c (Fifth, Cardinal), 'y' = the line Y = c (Market, the cross streets); inward = +1 if the sidewalk lies toward
    +X/+Y of the face; core [a0, a1] along the curb."""

    def __init__(self, axis, c, inward, a0, a1, flare=0.75, run=1.5, zb=ZB_RAMP, pad=True):
        self.axis, self.c, self.inward, self.a0, self.a1 = axis, c, inward, a0, a1
        self.F, self.L, self.zb, self.pad = flare, run, zb, pad

    def la(self, X, Y):
        if self.axis == 'x':
            return (np.asarray(X) - self.c) * self.inward, np.asarray(Y)
        return (np.asarray(Y) - self.c) * self.inward, np.asarray(X)

    def z(self, X, Y):
        l, a = self.la(X, Y)
        w = np.clip(1.0 - np.maximum(np.maximum(self.a0 - a, a - self.a1), 0.0) / self.F, 0.0, 1.0)
        return self.zb * w * np.clip(1.0 - l / self.L, 0.0, 1.0) * (l > -0.05)

    def lat_line(self, l):
        """The stage coordinate of the line l meters into the sidewalk."""
        return self.c + self.inward * l

    def rect(self, l0, l1, a0, a1):
        """(x0, x1, y0, y1) of the region l0..l1 into the sidewalk, a0..a1 along it."""
        p, q = sorted((self.lat_line(l0), self.lat_line(l1)))
        return (p, q, a0, a1) if self.axis == 'x' else (a0, a1, p, q)

    def area(self):
        return self.rect(CW, self.L, self.a0 - self.F, self.a1 + self.F)

    def pad_rect(self):
        return self.rect(CW, CW + 0.6, self.a0, self.a1) if self.pad else None

    def breaks(self):
        """(x breakpoints, y breakpoints) so cells meet the ramp's creases."""
        al = [self.a0 - self.F, self.a0, self.a1, self.a1 + self.F]
        lat = [self.lat_line(CW + 0.6), self.lat_line(self.L)] if self.pad else [self.lat_line(self.L)]
        return (lat, al) if self.axis == 'x' else (al, lat)


RAMPS = [
    Ramp('x', RX0, -1, 50.8, 52.0), Ramp('x', RX0, -1, 67.0, 68.2), Ramp('x', RX1, 1, 50.8, 52.0), Ramp('x', RX1, 1, 67.0, 68.2),
    Ramp('y', MS, -1, 1.2, 2.4), Ramp('y', MN, 1, 1.2, 2.4),
    Ramp('x', RX0, -1, -64.3, -63.1), Ramp('x', RX1, 1, -64.3, -63.1), Ramp('x', RX0, -1, 93.1, 94.3), Ramp('x', RX1, 1, 93.1, 94.3),
    Ramp('y', MS, -1, -97.9, -96.7), Ramp('y', MN, 1, -97.9, -96.7),
    # driveway aprons
    Ramp('y', WR0, -1, 30.0, 33.0, flare=0.6, run=1.2, zb=-0.15, pad=False),
    Ramp('x', CR0, 1, 36.0, 42.0, flare=0.6, run=1.2, zb=-0.15, pad=False),
]


def ramp_z(X, Y):
    X = np.asarray(X, dtype=np.float64)
    Y = np.asarray(Y, dtype=np.float64)
    z = np.zeros_like(X)
    for r in RAMPS:
        z = np.minimum(z, r.z(X, Y))
    return z


def in_rects(X, Y, rects, margin=0.0):
    X = np.asarray(X)
    Y = np.asarray(Y)
    m = np.zeros(X.shape, dtype=bool)
    for (x0, x1, y0, y1) in rects:
        m |= (X >= x0 - margin) & (X <= x1 + margin) & (Y >= y0 - margin) & (Y <= y1 + margin)
    return m


RAMP_AREAS = [r.area() for r in RAMPS]
PAD_RECTS = [r.pad_rect() for r in RAMPS if r.pad]
PIT_RECTS = [(x - 0.6, x + 0.6, y - 0.6, y + 0.6) for x, y in TREE_PITS]


def vault_rect(v):
    x, y, ax = v
    return (x - 0.6, x + 0.6, y - 1.2, y + 1.2) if ax == 'x' else (x - 1.2, x + 1.2, y - 0.6, y + 0.6)


VAULT_RECTS = [vault_rect(v) for v in VAULTS]


# ---- flags: patched, sagged, cracked (decided per flag on the world grid)

def flag_state(fx, fy):
    h = _hash2(np.asarray(fx), np.asarray(fy), 777)
    return np.where(h < 0.06, 1, np.where(h < 0.11, 2, np.where(h < 0.15, 3, 0)))   # 1 patched, 2 sagged, 3 cracked


_CLEAR = {}


def flag_clear(fx, fy):
    """True if the flag (fx, fy) sits wholly on plain sidewalk (no ramp, pad, pit or vault in or next to it)."""
    k = (int(fx), int(fy))
    v = _CLEAR.get(k)
    if v is None:
        x0, y0 = k[0] * 1.5, k[1] * 1.5
        r = (x0, x0 + 1.5, y0, y0 + 1.5)
        v = True
        for q in RAMP_AREAS + PIT_RECTS + VAULT_RECTS:
            if q[0] < r[1] + 0.3 and q[1] > r[0] - 0.3 and q[2] < r[3] + 0.3 and q[3] > r[2] - 0.3:
                v = False
                break
        _CLEAR[k] = v
    return v


def flags_clear(fx, fy):
    """flag_clear over arrays of flag indices."""
    fx = np.asarray(fx).astype(np.int64)
    fy = np.asarray(fy).astype(np.int64)
    key = (fx + 100000) * 200000 + (fy + 100000)
    uk, inv = np.unique(key, return_inverse=True)
    vals = np.array([flag_clear(int(k // 200000) - 100000, int(k % 200000) - 100000) for k in uk], dtype=bool)
    return vals[inv].reshape(fx.shape)


def sag_z(X, Y, strip_rects):
    """The sagged flags' 1 cm dishes (only flags wholly inside a sidewalk strip, clear of everything)."""
    X = np.asarray(X, dtype=np.float64)
    Y = np.asarray(Y, dtype=np.float64)
    fx = np.floor(X / 1.5)
    fy = np.floor(Y / 1.5)
    st = flag_state(fx, fy)
    du = X / 1.5 - fx
    dv = Y / 1.5 - fy
    dish = np.sin(np.pi * np.clip(du, 0, 1)) * np.sin(np.pi * np.clip(dv, 0, 1))
    whole = np.zeros(X.shape, dtype=bool)
    for (x0, x1, y0, y1) in strip_rects:
        whole |= (fx * 1.5 >= x0 - 1e-6) & ((fx + 1) * 1.5 <= x1 + 1e-6) & (fy * 1.5 >= y0 - 1e-6) & ((fy + 1) * 1.5 <= y1 + 1e-6)
    sel = (st == 2) & whole
    if sel.any():
        sel = sel & flags_clear(fx, fy)
    return np.where(sel, -0.010 * dish ** 0.8, 0.0), np.where(sel, dish, 0.0)


def vault_dip(X, Y):
    d = np.zeros(np.shape(X))
    for (x0, x1, y0, y1) in VAULT_RECTS:
        dx = np.maximum(np.maximum(x0 - X, X - x1), 0.0)
        dy = np.maximum(np.maximum(y0 - Y, Y - y1), 0.0)
        d = np.minimum(d, -RECESS * smoothstep(0.05, 0.01, np.hypot(dx, dy)))
    return d


def sidewalk_z_fn(strip_rects):
    def zf(X, Y):
        s, _ = sag_z(X, Y, strip_rects)
        return np.minimum(ramp_z(X, Y), 0.0) + s + vault_dip(X, Y)
    return zf


# ---- the road

def crown(d, W):
    return np.clip(1.0 - (2.0 * np.asarray(d) / W) ** 2, 0.0, 1.0)


def road_crown_z(X, Y):
    """RoadZ = -0.15 + 0.03 c on every street; through streets keep their crown, the T'ing street's fades out."""
    X = np.asarray(X, dtype=np.float64)
    Y = np.asarray(Y, dtype=np.float64)
    c = np.zeros_like(X)
    inF = (X >= RX0 - 0.02) & (X <= RX1 + 0.02) & (Y > HR0 - 1.0) & (Y < WR0 + 1.0)
    fF = smoothstep(HR0, HR0 + 6.0, Y) * smoothstep(WR0, WR0 - 6.0, Y)
    c = np.where(inF, np.maximum(c, crown(X - FC, 12.0) * fF), c)
    for yc in (HC, WC):
        inH = np.abs(Y - yc) <= 5.02
        c = np.where(inH, np.maximum(c, crown(Y - yc, 10.0)), c)
    inM = (Y >= MS - 0.02) & (Y <= MN + 0.02) & (X < RX0 + 0.02) & (X > CR0 - 0.02)
    fM = smoothstep(RX0, RX0 - 6.0, X) * smoothstep(CR0, CR0 + 6.0, X)
    c = np.where(inM, np.maximum(c, crown(Y - MC, 11.0) * fM), c)
    inC = np.abs(X - CC) <= 5.02
    c = np.where(inC, np.maximum(c, crown(X - CC, 10.0)), c)
    return -0.15 + 0.03 * c


def road_dips(X, Y):
    X = np.asarray(X, dtype=np.float64)
    Y = np.asarray(Y, dtype=np.float64)
    z = np.zeros_like(X)
    for (px, py, r) in POTHOLES:
        d = np.hypot(X - px, Y - py)
        ang = np.arctan2(Y - py, X - px)
        re = r * (0.82 + 0.12 * np.sin(3 * ang + px) + 0.08 * np.sin(5 * ang + py) + 0.12 * vnoise2(X, Y, 0.12, 41, 2))
        z = z - 0.03 * smoothstep(1.0, 0.6, d / re) * (0.8 + 0.2 * vnoise2(X, Y, 0.06, 43, 1))
    for (mx, my) in MANHOLES:
        d = np.hypot(X - mx, Y - my)
        z = z - RECESS * smoothstep(0.53, 0.47, d)
    return z


def road_z(X, Y):
    return road_crown_z(X, Y) + road_dips(X, Y)


def pothole_w(X, Y):
    w = np.zeros(np.shape(X))
    for (px, py, r) in POTHOLES:
        w = np.maximum(w, smoothstep(1.25 * r, 0.7 * r, np.hypot(X - px, Y - py)))
    return w


# ---- curb chains

def arc(center, r, a0, a1, n=24):
    return [(center[0] + r * math.cos(math.radians(a0 + (a1 - a0) * i / n)), center[1] + r * math.sin(math.radians(a0 + (a1 - a0) * i / n)))
            for i in range(n + 1)]


def chains():
    """The curb face lines: (key, points (dense), sidewalk on the left?)."""
    C = CORNERS

    def ac(k, rad=CR):
        c, a0, a1 = C[k]
        return arc(c, rad, a0, a1)
    home = [(XF0, HR0), (RX0 - CR, HR0)] + ac('hollis_home')[1:] + [(RX0, MS - CR)] + ac('lp')[1:] + [(CR0 + CR, MS)] + ac('card_s')[1:] + [(CR0, YF0)]
    ruby = [(CR0, YF1), (CR0, MN + CR)] + ac('card_n')[1:] + [(RX0 - CR, MN)] + ac('ruby')[1:] + [(RX0, WR0 - CR)] + ac('wexler_home')[1:] + [(XF0, WR0)]
    far = [(XF1, HR0), (RX1 + CR, HR0)] + ac('hollis_far')[1:] + [(RX1, WR0 - CR)] + ac('wexler_far')[1:] + [(XF1, WR0)]
    return [('home', home, True), ('ruby', ruby, True), ('far', far, False), ('hollis_s', [(XF0, HR1), (XF1, HR1)], False),
            ('wexler_n', [(XF0, WR1), (XF1, WR1)], True), ('card_e', [(CR1, YF0), (CR1, YF1)], True)]


def in_main(X, Y):
    """Inside the detailed meshes (not Far)."""
    return (XM0 <= X <= XM1 and HSL - 1 <= Y <= WNL + 1) or (MEAST - 20 <= X <= 0 and MSL - 1 <= Y <= MNL + 1) or \
        (CEL - 1 <= X <= MEAST and YC0 <= Y <= YC1)


def densify(pts, breaks=()):
    """Resamples a polyline of axis-aligned runs and arc points: the runs at the world grid's 0.1 m lines inside the detailed
    meshes (0.5 m outside), so the gutter's outer edge meets the road's cells vertex for vertex; the arcs' points kept; the
    breakpoints (stage points on a run) added exactly."""
    out = [np.array(pts[0], float)]
    for a, b in zip(pts[:-1], pts[1:]):
        a = np.array(a, float)
        b = np.array(b, float)
        L = float(np.linalg.norm(b - a))
        if L < 1e-9:
            continue
        cand = []
        axis = 0 if abs(b[1] - a[1]) < 1e-9 else (1 if abs(b[0] - a[0]) < 1e-9 else None)
        if L > 0.3 and axis is not None:
            lo, hi = sorted((a[axis], b[axis]))
            for k in range(int(math.floor(lo / 0.1)) + 1, int(math.ceil(hi / 0.1))):
                v = k * 0.1
                if not (lo + 1e-7 < v < hi - 1e-7):
                    continue
                p = a.copy()
                p[axis] = v
                if k % 5 != 0 and not in_main(p[0], p[1]):
                    continue
                cand.append(((v - a[axis]) / (b[axis] - a[axis]), p))
        elif L > 0.3:
            n = max(1, int(round(L / 0.1)))
            cand += [(i / n, a + (b - a) * i / n) for i in range(1, n)]
        d = (b - a) / L
        for bp in breaks:
            bp = np.array(bp, float)
            t = float((bp - a) @ d) / L
            off = np.linalg.norm(a + d * t * L - bp)
            if 1e-6 < t < 1 - 1e-6 and off < 1e-4:
                p = bp.copy()
                if axis is not None:
                    p[1 - axis] = a[1 - axis]
                cand.append((t, p))
        cand.sort(key=lambda c: c[0])
        for t, p in cand:
            out.append(p)
        out.append(b)
    res = [out[0]]
    for p in out[1:]:
        if np.linalg.norm(p - res[-1]) > 1e-6:
            res.append(p)
    return res


def inlet_spans():
    """The stage segments (centers +-0.6 m) the inlets take out of each curb chain."""
    spans = []
    for key, a in INLETS:
        if key == 'home':
            spans.append(((RX0, a - 0.6), (RX0, a + 0.6)))
        elif key == 'far':
            spans.append(((RX1, a - 0.6), (RX1, a + 0.6)))
        elif key == 'ms':
            spans.append(((a - 0.6, MS), (a + 0.6, MS)))
        elif key == 'mn':
            spans.append(((a - 0.6, MN), (a + 0.6, MN)))
        elif key == 'hollis_n':
            spans.append(((a - 0.6, HR0), (a + 0.6, HR0)))
        elif key == 'wexler_s':
            spans.append(((a - 0.6, WR0), (a + 0.6, WR0)))
    return spans


def in_inlet(X, Y):
    for (a, b) in inlet_spans():
        if abs(a[0] - b[0]) < 1e-6:
            if abs(X - a[0]) < 0.01 and min(a[1], b[1]) - 1e-6 <= Y <= max(a[1], b[1]) + 1e-6:
                return True
        elif abs(Y - a[1]) < 0.01 and min(a[0], b[0]) - 1e-6 <= X <= max(a[0], b[0]) + 1e-6:
            return True
    return False


def chain_breaks():
    """Points to keep exactly on the chains: the ramps' creases, the inlets' ends, the meshes' seams."""
    pts = []
    for r in RAMPS:
        for a in (r.a0 - r.F, r.a0, r.a1, r.a1 + r.F):
            pts.append((r.c, a) if r.axis == 'x' else (a, r.c))
    for (a, b) in inlet_spans():
        pts += [a, b]
    for y in (15.0, YC0, YC1):
        pts += [(RX0, y), (RX1, y), (CR0, y), (CR1, y)]
    for x in (0.0, XM0, XM1, MEAST):
        pts += [(x, MS), (x, MN), (x, HR0), (x, HR1), (x, WR0), (x, WR1)]
    return pts


# ================================================================== the ground builder

class Ground:
    """Collects the ground's faces in stage meters as numpy chunks: loops' positions, UVs and colors, faces' sizes and
    slots; then makes the Blender meshes (x = -X, windings reversed so normals stay up)."""

    def __init__(self):
        self.chunks = []
        self.curb_segs = []
        self.build_segs = []      # building lines (grime)

    def add(self, P, UV, C, sizes, mats):
        self.chunks.append((np.asarray(P, np.float64), np.asarray(UV, np.float32), np.asarray(C, np.float32), np.asarray(sizes, np.int32),
                            np.asarray(mats, np.int32)))

    # ---- horizontal regions
    def region(self, pieces, cell, zf, matf, colf, uvf=None):
        """pieces: ('rect', x0, x1, y0, y1) (fast: gridded directly) or ('poly', [(x, y), ...]) (convex or gently concave:
        each cell is clipped by it). Cells on the world grid (origin 0)."""
        cx, cy = cell if isinstance(cell, tuple) else (cell, cell)
        polys = []
        rect_P = []
        for pc in pieces:
            if pc[0] == 'rect':
                _, x0, x1, y0, y1 = pc
                if x1 - x0 < 1e-6 or y1 - y0 < 1e-6:
                    continue
                xs = sorted({x0, x1} | {k * cx for k in range(int(math.floor(x0 / cx)) + 1, int(math.ceil(x1 / cx))) if x0 + 1e-7 < k * cx < x1 - 1e-7})
                ys = sorted({y0, y1} | {k * cy for k in range(int(math.floor(y0 / cy)) + 1, int(math.ceil(y1 / cy))) if y0 + 1e-7 < k * cy < y1 - 1e-7})
                xs = np.array(xs)
                ys = np.array(ys)
                xa, ya = np.meshgrid(xs[:-1], ys[:-1])
                xb, yb = np.meshgrid(xs[1:], ys[1:])
                q = np.stack([np.stack([xa, ya], -1), np.stack([xb, ya], -1), np.stack([xb, yb], -1), np.stack([xa, yb], -1)], 2)
                rect_P.append(q.reshape(-1, 4, 2))
            else:
                piece = ccw(clean_poly(pc[1]))
                if len(piece) < 3 or abs(poly_area(piece)) < 1e-8:
                    continue
                xs_ = [p[0] for p in piece]
                ys_ = [p[1] for p in piece]
                for i in range(int(math.floor(min(xs_) / cx)), int(math.ceil(max(xs_) / cx))):
                    for j in range(int(math.floor(min(ys_) / cy)), int(math.ceil(max(ys_) / cy))):
                        cellp = [(i * cx, j * cy), ((i + 1) * cx, j * cy), ((i + 1) * cx, (j + 1) * cy), (i * cx, (j + 1) * cy)]
                        qd = clean_poly(clip_convex(piece, cellp))
                        if len(qd) >= 3 and abs(poly_area(qd)) > 1e-7:
                            polys.append(ccw(qd))
        sizes = []
        pts = []
        if rect_P:
            R = np.concatenate(rect_P, 0)
            pts.append(R.reshape(-1, 2))
            sizes.append(np.full(len(R), 4, np.int32))
        if polys:
            pts.append(np.array([p for q in polys for p in q], np.float64))
            sizes.append(np.array([len(q) for q in polys], np.int32))
        if not pts:
            return
        P2 = np.concatenate(pts, 0)
        S = np.concatenate(sizes, 0)
        starts = np.concatenate([[0], np.cumsum(S)[:-1]])
        cen = np.add.reduceat(P2, starts, axis=0) / S[:, None]
        M = matf(cen[:, 0], cen[:, 1]) if callable(matf) else np.full(len(S), matf, np.int32)
        Ml = np.repeat(M, S)
        # heights and colors once per distinct point
        key = np.round(P2 * 4000.0).astype(np.int64)
        key = (key[:, 0] + (1 << 24)) * (1 << 26) + (key[:, 1] + (1 << 24))
        uk, first, inv = np.unique(key, return_index=True, return_inverse=True)
        Pu = P2[first]
        Zu = zf(Pu[:, 0], Pu[:, 1])
        Cu = colf(Pu[:, 0], Pu[:, 1], Zu, Ml[first])
        Z = Zu[inv]
        Pm = np.concatenate([P2, Z[:, None]], 1)
        C = Cu[inv]
        UV = P2 if uvf is None else uvf(P2[:, 0], P2[:, 1])
        self.add(Pm, UV, C, S, M)

    # ---- swept curbs and open strips
    def faces_from_list(self, faces):
        """faces: [(mat, [(X, Y, Z)], [(u, v)], [(r, g, b, a)])]."""
        if not faces:
            return
        P = [p for f in faces for p in f[1]]
        UV = [t for f in faces for t in f[2]]
        C = [c for f in faces for c in f[3]]
        self.add(P, UV, C, [len(f[1]) for f in faces], [f[0] for f in faces])


def _cross2(a, b):
    return a[0] * b[1] - a[1] * b[0]


def frames_along(pts, upper_left):
    """Per point: (tangent, normal toward the sidewalk). Exact: a run's direction on runs, the circle's tangent on the curb
    returns (they meet tangentially, so the frames agree at every junction)."""
    n = len(pts)
    out = []
    centers = [c for c, _, _ in CORNERS.values()]
    for i in range(n):
        a = pts[max(i - 1, 0)]
        b = pts[min(i + 1, n - 1)]
        t = b - a
        t = t / max(np.linalg.norm(t), 1e-12)
        P = pts[i]
        for c in centers:
            r = np.array((P[0] - c[0], P[1] - c[1]))
            if abs(np.linalg.norm(r) - CR) < 1e-6:
                tt = np.array((-r[1], r[0])) / CR
                t = tt if tt @ t > 0 else -tt
                break
        else:
            # on a run: the run's own direction (axis-aligned)
            if abs(t[0]) > abs(t[1]):
                t = np.array((math.copysign(1.0, t[0]), 0.0))
            else:
                t = np.array((0.0, math.copysign(1.0, t[1])))
        nl = np.array((-t[1], t[0])) if upper_left else np.array((t[1], -t[0]))
        out.append((t, nl))
    return out


def curb_profile(top_face, top_back, low, joint=False):
    """The curb's closed section, (lat toward the sidewalk, z), counterclockwise in (lat, z); and per point its v."""
    ch = min(0.012, max(top_face - low, 0.0) * 0.5 + 0.002)
    g = 0.004 if joint else 0.0
    pts = [(0.0 + g, low - 0.06), (g, top_face - ch - g * 0.5), (ch + g * 0.5, top_face - g), (CW - 0.006, top_back - g * 0.25),
           (CW, top_back - 0.006), (CW, min(top_back, top_face) - 0.08)]
    # v: the face's points Z - top (<= 0), the top's points meters back from the face.
    vs = [low - 0.06 - top_face, -ch, ch, CW - 0.006, CW, -0.08]
    pts = list(reversed(pts))       # as listed it runs clockwise in (lat, z)
    vs = list(reversed(vs))
    return pts, vs


def build_curbs(G, colf_curb, colf_gutter):
    """Every curb and gutter along the chains. Returns {chain key: (points, normals, gutter outer points)}."""
    faces = []
    gfaces = []
    info = {}
    spans = inlet_spans()
    br = chain_breaks()

    def in_span(P):
        for (a, b) in spans:
            a = np.array(a)
            b = np.array(b)
            d = b - a
            L = np.linalg.norm(d)
            t = float((P - a) @ d) / (L * L)
            if -1e-6 <= t <= 1 + 1e-6 and np.linalg.norm(a + d * t - P) < 1e-3:
                return True
        return False

    for key, raw, left in chains():
        pts = densify(raw, breaks=br)
        fr = frames_along(pts, left)
        n = len(pts)
        s_acc = [0.0]
        for i in range(1, n):
            s_acc.append(s_acc[-1] + float(np.linalg.norm(pts[i] - pts[i - 1])))
        outer = []
        rings = []
        inl = [in_span(p) for p in pts]
        for i in range(n):
            P = pts[i]
            t, nl = fr[i]
            # the gutter: from the face (lat 0) out to -GW toward the road
            edge = P - nl * GW
            ze = float(road_z(np.array([edge[0]]), np.array([edge[1]]))[0])
            sunk = RECESS if inl[i] else 0.0
            gl = [0.0, -0.1125, -0.225, -0.3375, -GW]
            gp = [(P[0] + nl[0] * l, P[1] + nl[1] * l, FLOW + (ze - FLOW) * (-l / GW) - (sunk if l > -GW + 1e-6 else 0.0)) for l in gl]
            outer.append((edge[0], edge[1], ze))
            # the curb's top: the sidewalk's level there (ramps lower it)
            tf = float(ramp_z(np.array([P[0]]), np.array([P[1]]))[0])
            bk = P + nl * CW
            tb = float(ramp_z(np.array([bk[0]]), np.array([bk[1]]))[0])
            rings.append((gp, tf, tb))
        info[key] = (pts, fr, outer)
        # joints every 3 m along each chain (inside the detailed meshes)
        jpos = [k * 3.0 for k in range(1, int(s_acc[-1] / 3.0) + 1)]
        # --- gutter strips
        for i in range(n - 1):
            a, b = rings[i][0], rings[i + 1][0]
            for k in range(len(a) - 1):
                quad = [a[k], a[k + 1], b[k + 1], b[k]]
                if _cross2((quad[1][0] - quad[0][0], quad[1][1] - quad[0][1]), (quad[3][0] - quad[0][0], quad[3][1] - quad[0][1])) < 0:
                    quad = quad[::-1]
                gfaces.append((GUTTER, quad, [(q[0], q[1]) for q in quad], [None] * 4))
        # --- curbs (skipping the inlets' spans), with joints
        segs = []
        cur = []
        for i in range(n):
            if inl[i] and i + 1 < n and inl[i + 1]:
                if cur:
                    cur.append(i)
                    segs.append(cur)
                    cur = []
                continue
            cur.append(i)
        if len(cur) > 1:
            segs.append(cur)
        for seg in segs:
            seg = sorted(set(seg))
            if len(seg) < 2:
                continue
            ring_list = []
            for i in seg:
                P = pts[i]
                t, nl = fr[i]
                gp, tf, tb = rings[i]
                low = gp[0][2]
                prof, vs = curb_profile(tf, tb, low)
                mat = CONCRETE if min(tf, tb) < -0.02 else CURB
                ring_list.append((s_acc[i], P, nl, prof, vs, mat))
            # insert joint rings
            full = []
            for idx in range(len(ring_list)):
                full.append(ring_list[idx])
                if idx + 1 < len(ring_list):
                    s0, s1 = ring_list[idx][0], ring_list[idx + 1][0]
                    for js in jpos:
                        if s0 + 0.012 < js < s1 - 0.012 and in_main(*ring_list[idx][1]):
                            f = (js - s0) / (s1 - s0)
                            P0, P1 = ring_list[idx][1], ring_list[idx + 1][1]
                            nl0 = ring_list[idx][2]
                            for ds, jt in ((-0.003, False), (0.0, True), (0.003, False)):
                                ff = (js + ds - s0) / (s1 - s0)
                                Pj = P0 + (P1 - P0) * ff
                                gp, tf, tb = rings[seg[idx]]
                                prof, vs = curb_profile(tf, tb, gp[0][2], joint=jt)
                                full.append((js + ds, Pj, nl0, prof, vs, ring_list[idx][5]))
            m = len(full[0][3])
            for a, b in zip(full[:-1], full[1:]):
                sa, Pa, na, pa, va, ma = a
                sb, Pb, nb, pb, vb, mb = b
                mat = ma if ma == mb else CONCRETE
                for k in range(m):
                    k1 = (k + 1) % m
                    A = (Pa[0] + na[0] * pa[k][0], Pa[1] + na[1] * pa[k][0], pa[k][1])
                    B = (Pa[0] + na[0] * pa[k1][0], Pa[1] + na[1] * pa[k1][0], pa[k1][1])
                    Cc = (Pb[0] + nb[0] * pb[k1][0], Pb[1] + nb[1] * pb[k1][0], pb[k1][1])
                    D = (Pb[0] + nb[0] * pb[k][0], Pb[1] + nb[1] * pb[k][0], pb[k][1])
                    quad = [A, B, Cc, D]
                    uvs = [(sa, va[k]), (sa, va[k1]), (sb, vb[k1]), (sb, vb[k])]
                    if not left:
                        quad = quad[::-1]
                        uvs = uvs[::-1]
                    faces.append((CURB if mat == CURB else CONCRETE, quad, uvs, [None] * 4))
            # caps
            for end, rev in ((full[0], True), (full[-1], False)):
                s, P, nl, pr, vv, mt = end
                poly = [(P[0] + nl[0] * l, P[1] + nl[1] * l, z) for l, z in pr]
                if rev:
                    poly = poly[::-1]
                if not left:
                    poly = poly[::-1]
                faces.append((CURB, poly, [(q[0] + q[1], q[2]) for q in poly], [None] * 4))
        G.curb_segs += [(tuple(map(float, p)), tuple(map(float, q))) for p, q in zip(raw[:-1], raw[1:])]
    # colors, vectorized
    for flist, cf in ((faces, colf_curb), (gfaces, colf_gutter)):
        if not flist:
            continue
        P = np.array([p for f in flist for p in f[1]], np.float64)
        sizes = np.array([len(f[1]) for f in flist], np.int32)
        mats = np.array([f[0] for f in flist], np.int32)
        UV = np.array([t for f in flist for t in f[2]], np.float64)
        C = cf(P[:, 0], P[:, 1], P[:, 2], np.repeat(mats, sizes))
        G.add(P, UV, C, sizes, mats)
    return info


# ================================================================== vertex colors

AWNINGS = [(0.0, 1.5, 31.1, 36.3), (0.0, 1.5, -34.0, -24.0), (-23.5, -14.5, 49.5, 51.1), (-35.5, -25.5, 49.5, 51.1), (-76.0, -67.0, 49.5, 51.1),
           (-43.0, -36.2, 67.9, 69.5), (18.9, 20.4, -60.0, -50.0), (-14.5, -2.5, 67.9, 69.5), (-27.0, -16.0, 67.9, 69.5),
           (-96.0, -76.0, 66.9, 69.5), (-24.8, -20.2, 50.95, 52.65)]


def building_segments():
    return [((0.0, HN), (0.0, MSL)), ((0.0, MNL), (0.0, WSL)), ((FAR, HN), (FAR, WSL)), ((MEAST, MSL), (0.0, MSL)), ((MEAST, MNL), (0.0, MNL)),
            ((XF0, HN), (0.0, HN)), ((FAR, HN), (XF1, HN)), ((XF0, HSL), (XF1, HSL)), ((XF0, WSL), (0.0, WSL)), ((FAR, WSL), (XF1, WSL)),
            ((XF0, WNL), (XF1, WNL)), ((MEAST, YF0), (MEAST, MSL)), ((MEAST, MNL), (MEAST, YF1)), ((CEL, YF0), (CEL, YF1))]


def wheel_paths(X, Y):
    """1 on the wheel paths (lane centers +-0.8 m), for every street."""
    X = np.asarray(X)
    Y = np.asarray(Y)
    w = np.zeros(X.shape)
    inF = (X > RX0) & (X < RX1) & (Y > HR0) & (Y < WR0)
    for c in (8.4, 12.0):
        for s in (-0.8, 0.8):
            w = np.maximum(w, np.where(inF, smoothstep(0.35, 0.05, np.abs(X - (c + s))), 0.0))
    inM = (Y > MS) & (Y < MN) & (X < RX0)
    for c in (57.95, 61.05):
        for s in (-0.8, 0.8):
            w = np.maximum(w, np.where(inM, smoothstep(0.35, 0.05, np.abs(Y - (c + s))), 0.0))
    for yc in (HC, WC):
        inH = np.abs(Y - yc) < 5.0
        for c in (yc - 2.5, yc + 2.5):
            for s in (-0.8, 0.8):
                w = np.maximum(w, np.where(inH, smoothstep(0.35, 0.05, np.abs(Y - (c + s))), 0.0))
    inC = np.abs(X - CC) < 5.0
    for c in (CC - 2.5, CC + 2.5):
        for s in (-0.8, 0.8):
            w = np.maximum(w, np.where(inC, smoothstep(0.35, 0.05, np.abs(X - (c + s))), 0.0))
    return w


def lane_oil(X, Y):
    """The oil strip down the middle of each travel lane."""
    X = np.asarray(X)
    Y = np.asarray(Y)
    w = np.zeros(X.shape)
    inF = (X > RX0) & (X < RX1) & (Y > HR0) & (Y < WR0)
    for c in (8.4, 12.0):
        w = np.maximum(w, np.where(inF, smoothstep(0.5, 0.1, np.abs(X - c)), 0.0))
    inM = (Y > MS) & (Y < MN) & (X < RX0)
    for c in (57.95, 61.05):
        w = np.maximum(w, np.where(inM, smoothstep(0.5, 0.1, np.abs(Y - c)), 0.0))
    for yc in (HC, WC):
        for c in (yc - 2.5, yc + 2.5):
            w = np.maximum(w, np.where(np.abs(Y - yc) < 5.0, smoothstep(0.5, 0.1, np.abs(Y - c)), 0.0))
    for c in (CC - 2.5, CC + 2.5):
        w = np.maximum(w, np.where(np.abs(X - CC) < 5.0, smoothstep(0.5, 0.1, np.abs(X - c)), 0.0))
    return w


def pud(X, Y, scale=3.0, lo=0.55, hi=0.75, seed=11):
    return smoothstep(lo, hi, vnoise2(X, Y, scale, seed, 3))


class Colors:
    def __init__(self, G, strip_rects):
        self.G = G
        self.strips = strip_rects

    def shelter(self, X, Y):
        return in_rects(X, Y, AWNINGS).astype(np.float64)

    def sidewalk(self, X, Y, Z, M):
        P = np.stack([X, Y], 1)
        db = seg_distance(P, building_segments())
        dc = seg_distance(P, self.G.curb_segs) if self.G.curb_segs else np.full(len(X), 9.0)
        n1 = vnoise2(X, Y, 1.4, 21, 3)
        _, dish = sag_z(X, Y, self.strips)
        rz = ramp_z(X, Y)
        R = np.clip(0.9 * dish + 0.55 * smoothstep(-0.08, -0.16, rz) + 0.3 * pud(X, Y, 2.0, 0.6, 0.8, 23) * smoothstep(1.2, 0.2, dc), 0, 1)
        pits = np.zeros(len(X))
        for (px, py) in TREE_PITS:
            pits = np.maximum(pits, smoothstep(1.2, 0.6, np.maximum(np.abs(X - px), np.abs(Y - py))))
        Gc = 0.12 + 0.55 * smoothstep(0.9, 0.0, db) + 0.22 * smoothstep(0.7, 0.0, dc) + 0.18 * (n1 - 0.5) + 0.3 * pits
        Gc = Gc - 0.1 * smoothstep(1.2, 2.2, db) * smoothstep(1.2, 2.0, dc)
        B = self.shelter(X, Y)
        return np.stack([R, np.clip(Gc, 0, 1), B, np.ones(len(X))], 1)

    def road(self, X, Y, Z, M):
        P = np.stack([X, Y], 1)
        dc = seg_distance(P, self.G.curb_segs) if self.G.curb_segs else np.full(len(X), 9.0)
        edge = smoothstep(1.6, 0.4, dc)
        wp = wheel_paths(X, Y)
        oil = lane_oil(X, Y)
        R = np.clip(0.55 * pud(X, Y, 3.2, 0.58, 0.78, 11) * (0.45 + 0.55 * edge) + 0.4 * edge * pud(X, Y, 1.1, 0.5, 0.8, 13)
                    + 1.0 * pothole_w(X, Y) + 0.25 * wp * pud(X, Y, 5.0, 0.5, 0.8, 17), 0, 1)
        R = np.where(M == PATCH, np.clip(R + 0.15, 0, 1), R)
        Gc = 0.28 + 0.45 * edge + 0.35 * oil - 0.25 * wp + 0.15 * (vnoise2(X, Y, 2.0, 19, 3) - 0.5)
        return np.stack([R, np.clip(Gc, 0, 1), np.zeros(len(X)), np.ones(len(X))], 1)

    def gutter(self, X, Y, Z, M):
        P = np.stack([X, Y], 1)
        dc = seg_distance(P, self.G.curb_segs) if self.G.curb_segs else np.zeros(len(X))
        R = np.clip(0.35 + 0.65 * smoothstep(0.3, 0.02, dc) * (0.45 + 0.55 * vnoise2(X, Y, 1.5, 29, 2)), 0, 1)
        Gc = np.clip(0.55 + 0.3 * smoothstep(0.3, 0.0, dc) + 0.15 * (vnoise2(X, Y, 0.8, 31, 2) - 0.5), 0, 1)
        return np.stack([R, Gc, self.shelter(X, Y) * 0.0, np.ones(len(X))], 1)

    def curb(self, X, Y, Z, M):
        Gc = np.clip(0.35 + 0.5 * smoothstep(-0.03, -0.16, Z) + 0.15 * (vnoise2(X, Y, 1.0, 37, 2) - 0.5), 0, 1)
        return np.stack([np.zeros(len(X)), Gc, self.shelter(X, Y), np.ones(len(X))], 1)

    def pit(self, X, Y, Z, M):
        return np.stack([np.full(len(X), 0.3), np.full(len(X), 0.95), self.shelter(X, Y), np.ones(len(X))], 1)


# ================================================================== the ground's layout

def sidewalk_rects():
    """(x0, x1, y0, y1) strips and bands of plain sidewalk (corner sectors are added separately)."""
    hb = HR0 + CW        # Hollis north band's curb back
    wb = WR0 - CW
    R = []
    # Fifth home and far, the T corners
    R += [(0.0, RX0 - CW, HN, MS - CR), (0.0, RX0 - CW, MN + CR, WSL)]
    R += [(RX1 + CW, FAR, HN, WSL)]
    R += [(XF0, RX0 - CR, hb, HN), (RX0 - CR, RX0 - CW, HR0 + CR, HN)]
    R += [(RX1 + CR, XF1, hb, HN), (RX1 + CW, RX1 + CR, HR0 + CR, HN)]
    R += [(XF0, RX0 - CR, WSL, wb), (RX0 - CR, RX0 - CW, WSL, WR0 - CR)]
    R += [(RX1 + CR, XF1, WSL, wb), (RX1 + CW, RX1 + CR, WSL, WR0 - CR)]
    R += [(XF0, XF1, HSL, HR1 - CW), (XF0, XF1, WR1 + CW, WNL)]
    # Market south and north, with the corner pieces at Fifth
    R += [(MEAST, 0.0, MSL, MS - CW), (0.0, RX0 - CR, MS - CR, MS - CW)]
    R += [(MEAST, 0.0, MN + CW, MNL), (0.0, RX0 - CR, MN + CW, MN + CR)]
    # Cardinal: the west sidewalk and Market's corners onto it (the curb returns' straight bits); the east sidewalk
    R += [(CR0 + CW, MEAST, YF0, MS - CR), (CR0 + CR, MEAST, MS - CR, MS - CW)]
    R += [(CR0 + CW, MEAST, MN + CR, YF1), (CR0 + CR, MEAST, MN + CW, MN + CR)]
    R += [(CEL, CR1 - CW, YF0, YF1)]
    return R


def sector_polys():
    """The sidewalk inside each curb return (fans to the curb's back, r = CR - CW)."""
    out = []
    for key, (c, a0, a1) in CORNERS.items():
        pts = arc(c, CR - CW, a0, a1)
        for p, q in zip(pts[:-1], pts[1:]):
            out.append(('poly', [c, p, q]))
    return out


def cracked_flags(strips):
    """The cracked flags (whole flags inside the strips, clear of everything): (flag rect, piece A, piece B (sunk))."""
    out = []
    for (x0, x1, y0, y1) in strips:
        for fx in range(int(math.ceil(x0 / 1.5 - 1e-9)), int(math.floor(x1 / 1.5 + 1e-9))):
            for fy in range(int(math.ceil(y0 / 1.5 - 1e-9)), int(math.floor(y1 / 1.5 + 1e-9))):
                if (fx + 1) * 1.5 > x1 + 1e-9 or (fy + 1) * 1.5 > y1 + 1e-9:
                    continue
                if int(flag_state(fx, fy)) != 3 or not flag_clear(fx, fy):
                    continue
                if not in_main(fx * 1.5 + 0.75, fy * 1.5 + 0.75):
                    continue
                h = float(_hash2(fx, fy, 991))
                h2 = float(_hash2(fx, fy, 993))
                X0, Y0 = fx * 1.5, fy * 1.5
                corner = int(h * 4)
                # a jagged line from one edge to the adjacent one, cutting off a corner piece
                a = 0.45 + 0.6 * h2
                b = 0.45 + 0.6 * float(_hash2(fx, fy, 995))
                n = 7
                line = []
                for k in range(n + 1):
                    t = k / n
                    px, py = a * (1 - t), b * t
                    if 0 < k < n:
                        j = (float(_hash2(fx * 31 + k, fy, 997)) - 0.5) * 0.06
                        px += j
                        py += j
                    line.append((px, py))
                # local corner (0, 0) at the flag's corner `corner`
                def to_world(p, corner=corner):
                    u, v = p
                    if corner == 0:
                        return (X0 + u, Y0 + v)
                    if corner == 1:
                        return (X0 + 1.5 - u, Y0 + v)
                    if corner == 2:
                        return (X0 + 1.5 - u, Y0 + 1.5 - v)
                    return (X0 + u, Y0 + 1.5 - v)
                Bp = ccw([to_world((0.0, 0.0))] + [to_world(p) for p in line])
                cl = [to_world(p) for p in line]
                # piece A: the flag's other corners with the crack line reversed
                corners_local = [(1.5, 0.0), (1.5, 1.5), (0.0, 1.5)]
                Ap = ccw([to_world(line[-1])] + [to_world((0.0, 1.5))] + [to_world((1.5, 1.5))] + [to_world((1.5, 0.0))] + [to_world(line[0])] +
                         [to_world(p) for p in line[1:-1]])
                out.append(((X0, X0 + 1.5, Y0, Y0 + 1.5), Ap, Bp, cl, (to_world((0.0, 0.0)), h)))
                del corners_local
    return out


def decompose(rect, holes, xbreaks, ybreaks):
    """A rect cut at the breakpoints, minus the holes: a list of sub-rects."""
    x0, x1, y0, y1 = rect
    xs = {x0, x1}
    ys = {y0, y1}
    for hx0, hx1, hy0, hy1 in holes:
        if hx0 < x1 and hx1 > x0 and hy0 < y1 and hy1 > y0:
            xs |= {min(max(hx0, x0), x1), min(max(hx1, x0), x1)}
            ys |= {min(max(hy0, y0), y1), min(max(hy1, y0), y1)}
    xs |= {x for x in xbreaks if x0 < x < x1}
    ys |= {y for y in ybreaks if y0 < y < y1}
    xs = sorted(xs)
    ys = sorted(ys)
    out = []
    for xa, xb in zip(xs[:-1], xs[1:]):
        if xb - xa < 1e-6:
            continue
        for ya, yb in zip(ys[:-1], ys[1:]):
            if yb - ya < 1e-6:
                continue
            cx, cy = (xa + xb) / 2, (ya + yb) / 2
            if any(hx0 < cx < hx1 and hy0 < cy < hy1 for hx0, hx1, hy0, hy1 in holes):
                continue
            out.append((xa, xb, ya, yb))
    return out


def sidewalk_mat(cx, cy):
    m = np.full(len(cx), SIDEWALK, np.int32)
    st = flag_state(np.floor(cx / 1.5), np.floor(cy / 1.5))
    patched = (st == 1)
    clear = np.array([flag_clear(int(math.floor(a / 1.5)), int(math.floor(b / 1.5))) for a, b in zip(cx, cy)]) if patched.any() else np.zeros(len(cx), bool)
    m = np.where(patched & clear, CONCRETE, m)
    m = np.where(in_rects(cx, cy, RAMP_AREAS), CONCRETE, m)
    m = np.where(in_rects(cx, cy, PAD_RECTS), TACTILE, m)
    return m


def road_mat(cx, cy):
    m = np.full(len(cx), ASPHALT, np.int32)
    m = np.where(in_rects(cx, cy, PATCHES), PATCH, m)
    m = np.where(in_rects(cx, cy, [BAY]), BUSBAY, m)
    return m


def build_ground():
    t0 = time.time()
    G = Ground()
    G.build_segs = building_segments()
    strips = sidewalk_rects()
    col = Colors(G, strips)

    # ---------------- curbs and gutters first (their lines feed the colors)
    info = build_curbs(G, col.curb, col.gutter)
    log(f'ground: curbs and gutters in {time.time() - t0:.1f}s')

    # ---------------- sidewalks
    t1 = time.time()
    cracked = cracked_flags(strips)
    crack_holes = [c[0] for c in cracked]
    xb = [XM0, XM1, 0.0, MEAST]
    yb = [15.0, YC0, YC1]
    for r in RAMPS:
        bx, by = r.breaks()
        xb += bx
        yb += by
    sw_main = []
    sw_far = []
    for rc in strips:
        subs = decompose(rc, PIT_RECTS + crack_holes, xb, yb)
        for s in subs:
            cx, cy = (s[0] + s[1]) / 2, (s[2] + s[3]) / 2
            (sw_main if in_main(cx, cy) else sw_far).append(('rect',) + s)
    zf = sidewalk_z_fn(strips)
    G.region(sw_main, 1.5 / 8, zf, sidewalk_mat, col.sidewalk)
    G.region(sw_far, 0.75, lambda X, Y: np.zeros(len(X)), SIDEWALK, col.sidewalk)
    G.region(sector_polys(), 1.5 / 8, zf, sidewalk_mat, col.sidewalk)
    # cracked flags: piece A at the level, piece B sunk and tilted, a wall round B
    walls = []
    for (frect, Ap, Bp, cl, (corner_w, h)) in cracked:
        G.region([('poly', Ap)], 1.5 / 8, zf, SIDEWALK, col.sidewalk)
        cxw, cyw = corner_w
        depth0 = 0.004 + 0.003 * h
        tilt = 0.006

        def zb_fn(X, Y, cxw=cxw, cyw=cyw, depth0=depth0, tilt=tilt):
            d = np.hypot(X - cxw, Y - cyw)
            return zf(X, Y) - depth0 - tilt * np.clip(1.0 - d / 1.0, 0.0, 1.0)
        G.region([('poly', Bp)], 1.5 / 8, zb_fn, SIDEWALK, col.sidewalk)
        # the wall: along the crack line and B's two flag edges, from the neighbors' level down to B's
        Bc = ccw(Bp)
        for p, q in zip(Bc, Bc[1:] + Bc[:1]):
            L = math.hypot(q[0] - p[0], q[1] - p[1])
            nseg = max(1, int(math.ceil(L / 0.1)))
            for k in range(nseg):
                a = (p[0] + (q[0] - p[0]) * k / nseg, p[1] + (q[1] - p[1]) * k / nseg)
                b = (p[0] + (q[0] - p[0]) * (k + 1) / nseg, p[1] + (q[1] - p[1]) * (k + 1) / nseg)
                za_top = float(zf(np.array([a[0]]), np.array([a[1]]))[0])
                zb_top = float(zf(np.array([b[0]]), np.array([b[1]]))[0])
                za_bot = float(zb_fn(np.array([a[0]]), np.array([a[1]]))[0])
                zb_bot = float(zb_fn(np.array([b[0]]), np.array([b[1]]))[0])
                # B is counterclockwise: its inside is on the left of p->q; the wall faces it.
                quad = [(a[0], a[1], za_top), (b[0], b[1], zb_top), (b[0], b[1], zb_bot), (a[0], a[1], za_bot)]
                walls.append((SIDEWALK, quad, [(0.0, za_top), (L / nseg, zb_top), (L / nseg, zb_bot), (0.0, za_bot)], None))
    # tree pits: walls, the grate's seat, the soil
    pit_faces = []
    for (px, py) in TREE_PITS:
        if not in_main(px, py):
            continue
        rings = [(0.6, 0.0), (0.6, -0.035), (0.56, -0.035), (0.56, -0.08)]

        def sq(hw, z):
            c = [(px - hw, py - hw), (px + hw, py - hw), (px + hw, py + hw), (px - hw, py + hw)]
            pts = []
            for p, q in zip(c, c[1:] + c[:1]):
                L = math.hypot(q[0] - p[0], q[1] - p[1])
                # vertices at the sidewalk grid's crossings on the outer ring, so the wall meets the sidewalk's cells
                ts = [0.0]
                if hw == 0.6:
                    cell = 1.5 / 8
                    if abs(q[0] - p[0]) > 1e-9:
                        lo, hi = sorted((p[0], q[0]))
                        ts += [abs(k * cell - p[0]) / L for k in range(int(math.floor(lo / cell)) + 1, int(math.ceil(hi / cell))) if lo < k * cell < hi]
                    else:
                        lo, hi = sorted((p[1], q[1]))
                        ts += [abs(k * cell - p[1]) / L for k in range(int(math.floor(lo / cell)) + 1, int(math.ceil(hi / cell))) if lo < k * cell < hi]
                else:
                    ts += [k / 4 for k in range(1, 4)]
                for t in sorted(ts):
                    pts.append((p[0] + (q[0] - p[0]) * t, p[1] + (q[1] - p[1]) * t))
            return pts
        outer_top = sq(0.6, 0.0)
        top_z = zf(np.array([p[0] for p in outer_top]), np.array([p[1] for p in outer_top]))
        # outer wall (faces into the pit: the square runs counterclockwise, the pit on its left)
        for i in range(len(outer_top)):
            j = (i + 1) % len(outer_top)
            p, q = outer_top[i], outer_top[j]
            quad = [(p[0], p[1], float(top_z[i])), (q[0], q[1], float(top_z[j])), (q[0], q[1], -0.035), (p[0], p[1], -0.035)]
            pit_faces.append((GUTTER, quad, [(0, float(top_z[i])), (1, float(top_z[j])), (1, -0.035), (0, -0.035)], None))
        # the seat, the inner wall, the soil
        oc = [(px - 0.6, py - 0.6), (px + 0.6, py - 0.6), (px + 0.6, py + 0.6), (px - 0.6, py + 0.6)]
        ic = [(px - 0.56, py - 0.56), (px + 0.56, py - 0.56), (px + 0.56, py + 0.56), (px - 0.56, py + 0.56)]
        for k in range(4):
            a, b = oc[k], oc[(k + 1) % 4]
            c, d = ic[(k + 1) % 4], ic[k]
            quad = [(a[0], a[1], -0.035), (b[0], b[1], -0.035), (c[0], c[1], -0.035), (d[0], d[1], -0.035)]
            pit_faces.append((GUTTER, quad, [(q[0], q[1]) for q in quad], None))
            p, q = ic[k], ic[(k + 1) % 4]
            quad = [(p[0], p[1], -0.035), (q[0], q[1], -0.035), (q[0], q[1], -0.08), (p[0], p[1], -0.08)]
            pit_faces.append((GUTTER, quad, [(0, -0.035), (1, -0.035), (1, -0.08), (0, -0.08)], None))
        # soil: a 7 x 7 grid, mounded round the trunk
        N = 7
        for i in range(N):
            for j in range(N):
                xa, xb2 = px - 0.56 + 1.12 * i / N, px - 0.56 + 1.12 * (i + 1) / N
                ya, yb2 = py - 0.56 + 1.12 * j / N, py - 0.56 + 1.12 * (j + 1) / N
                quad = [(xa, ya), (xb2, ya), (xb2, yb2), (xa, yb2)]
                q3 = []
                for (x, y) in quad:
                    d = math.hypot(x - px, y - py)
                    z = -0.08 + 0.03 * max(0.0, 1.0 - d / 0.45) + 0.006 * math.sin(x * 37.0 + y * 23.0)
                    if max(abs(x - px), abs(y - py)) > 0.559:
                        z = -0.08
                    q3.append((x, y, z))
                pit_faces.append((GUTTER, q3, [(q[0], q[1]) for q in q3], None))
    for flist, cf in ((walls, col.sidewalk), (pit_faces, col.pit)):
        if not flist:
            continue
        P = np.array([p for f in flist for p in f[1]], np.float64)
        sizes = np.array([len(f[1]) for f in flist], np.int32)
        mats = np.array([f[0] for f in flist], np.int32)
        UV = np.array([t for f in flist for t in f[2]], np.float64)
        G.add(P, UV, cf(P[:, 0], P[:, 1], P[:, 2], np.repeat(mats, sizes)), sizes, mats)
    log(f'ground: sidewalks in {time.time() - t1:.1f}s ({len(cracked)} cracked flags)')

    # ---------------- roads
    t1 = time.time()
    roads = [(RX0 + GW, RX1 - GW, HR0 - GW, WR0 + GW),                    # Fifth
             (XF0, XF1, HR1 + GW, HR0 - GW), (XF0, XF1, WR0 + GW, WR1 - GW),  # Hollis, Wexler
             (CR0 - GW, RX0 + GW, MS + GW, MN - GW),                          # Market
             (CR1 + GW, CR0 - GW, YF0, YF1)]                                   # Cardinal
    pbx = [v for p in PATCHES for v in p[:2]] + [BAY[0], BAY[1]]
    pby = [v for p in PATCHES for v in p[2:]] + [BAY[2], BAY[3]]
    rd_main, rd_far = [], []
    for rc in roads:
        subs = decompose(rc, [], pbx + [XM0, XM1, 0.0, MEAST], pby + [15.0, YC0, YC1])
        for s in subs:
            cx, cy = (s[0] + s[1]) / 2, (s[2] + s[3]) / 2
            (rd_main if in_main(cx, cy) else rd_far).append(('rect',) + s)
    G.region(rd_main, 0.1, road_z, road_mat, col.road)
    G.region(rd_far, 1.0, road_z, ASPHALT, col.road)
    # wedges at the curb returns: fans from the corner square's outer corner to the gutter's outer edge
    wedges = []
    for ck, (c, a0, a1) in CORNERS.items():
        dx = 1.0 if math.cos(math.radians((a0 + a1) / 2)) > 0 else -1.0
        dy = 1.0 if math.sin(math.radians((a0 + a1) / 2)) > 0 else -1.0
        apex = (c[0] + dx * (CR + GW), c[1] + dy * (CR + GW))
        # the gutter's outer points on this arc (from the curb build, exactly)
        arc_out = []
        for key, (pts, fr, outer) in info.items():
            for P, o in zip(pts, outer):
                if abs(math.hypot(P[0] - c[0], P[1] - c[1]) - CR) < 1e-4:
                    arc_out.append((o[0], o[1]))
        arc_out.sort(key=lambda p: math.atan2(p[1] - c[1], p[0] - c[0]))
        for p, q in zip(arc_out[:-1], arc_out[1:]):
            wedges.append(('poly', [apex, p, q]))
    G.region(wedges, 0.1, road_z, ASPHALT, col.road)
    log(f'ground: roads in {time.time() - t1:.1f}s')

    # ---------------- markings
    t1 = time.time()
    build_markings(G)
    log(f'ground: markings in {time.time() - t1:.1f}s')

    # ---------------- meshes
    t1 = time.time()
    objs = make_meshes(G)
    log(f'ground: meshes in {time.time() - t1:.1f}s; total {time.time() - t0:.1f}s')
    return objs


# ================================================================== markings

def marking_wear(X, Y):
    X = np.asarray(X)
    Y = np.asarray(Y)
    return np.clip(0.06 + 0.3 * (vnoise2(X, Y, 0.9, 51, 3) - 0.35) + 0.6 * wheel_paths(X, Y) * (0.6 + 0.4 * vnoise2(X, Y, 0.4, 53, 2)), 0.0, 1.0)


class Markings:
    def __init__(self):
        self.polys = []    # (poly, rgb hex, wear override or None, lift)

    def poly(self, p, paint, wear=None, lift=0.002):
        self.polys.append((ccw(p), paint, wear, lift))

    def rect(self, x0, x1, y0, y1, paint, wear=None):
        if x1 - x0 > 1e-6 and y1 - y0 > 1e-6:
            self.poly(rectp(x0, x1, y0, y1), paint, wear)

    def disc(self, cx, cy, r, paint, wear=0.0, n=20):
        self.poly([(cx + r * math.cos(2 * math.pi * k / n), cy + r * math.sin(2 * math.pi * k / n)) for k in range(n)], paint, wear, 0.0015)

    def emit(self, G, cell=0.25):
        loops_P, loops_UV, loops_C, sizes = [], [], [], []
        for p, paint, wear, lift in self.polys:
            xs_ = [q[0] for q in p]
            ys_ = [q[1] for q in p]
            cells = []
            for i in range(int(math.floor(min(xs_) / cell)), int(math.ceil(max(xs_) / cell))):
                for j in range(int(math.floor(min(ys_) / cell)), int(math.ceil(max(ys_) / cell))):
                    cp = [(i * cell, j * cell), ((i + 1) * cell, j * cell), ((i + 1) * cell, (j + 1) * cell), (i * cell, (j + 1) * cell)]
                    q = clean_poly(clip_convex(p, cp))
                    if len(q) >= 3 and abs(poly_area(q)) > 1e-8:
                        cells.append(ccw(q))
            if not cells:
                continue
            P2 = np.array([v for q in cells for v in q], np.float64)
            Z = road_z(P2[:, 0], P2[:, 1]) + lift
            rgb = np.array(H(paint)[:3], np.float32)
            A = marking_wear(P2[:, 0], P2[:, 1]) if wear is None else np.full(len(P2), wear)
            C = np.concatenate([np.broadcast_to(rgb, (len(P2), 3)), A[:, None]], 1)
            loops_P.append(np.concatenate([P2, Z[:, None]], 1))
            loops_UV.append(P2)
            loops_C.append(C)
            sizes += [len(q) for q in cells]
        if sizes:
            G.add(np.concatenate(loops_P), np.concatenate(loops_UV), np.concatenate(loops_C), sizes, [MARKING] * len(sizes))


def build_markings(G):
    M = Markings()
    W, Y_ = PAINT_WHITE, PAINT_YELLOW
    # double yellows
    for y0, y1 in ((-61.2, 48.8), (70.2, 91.2)):
        M.rect(10.08, 10.20, y0, y1, Y_)
        M.rect(10.30, 10.42, y0, y1, Y_)
    M.rect(-94.9, -1.1, 59.32, 59.42, Y_)
    M.rect(-94.9, -1.1, 59.58, 59.68, Y_)
    for yc in (HC, WC):
        M.rect(XF0, XF1, yc - 0.15, yc - 0.05, Y_)
        M.rect(XF0, XF1, yc + 0.05, yc + 0.15, Y_)
    M.rect(CC - 0.15, CC - 0.05, YF0, YF1, Y_)
    M.rect(CC + 0.05, CC + 0.15, YF0, YF1, Y_)
    # stop bars
    M.rect(RX0 + GW + 0.1, 9.98, 48.5, 48.8, W)
    M.rect(10.52, RX1 - GW - 0.1, 70.2, 70.5, W)
    M.rect(-1.1, -0.8, 59.78, MN - GW - 0.1, W)
    M.rect(10.52, RX1 - GW - 0.1, -61.2, -60.9, W)
    M.rect(RX0 + GW + 0.1, 9.98, 90.9, 91.2, W)
    M.rect(-95.2, -94.9, MS + GW + 0.1, 59.22, W)

    # ladder crosswalks
    def across_fifth(y0, y1):
        x0, x1 = RX0 + GW + 0.1, RX1 - GW - 0.1
        M.rect(x0, x1, y0, y0 + 0.2, W)
        M.rect(x0, x1, y1 - 0.2, y1, W)
        for k in range(-6, 7):
            c = FC + 0.6 + 1.2 * k
            M.rect(max(c - 0.3, x0), min(c + 0.3, x1), y0 + 0.3, y1 - 0.3, W)

    def across_market(x0, x1):
        y0, y1 = MS + GW + 0.1, MN - GW - 0.1
        M.rect(x0, x0 + 0.2, y0, y1, W)
        M.rect(x1 - 0.2, x1, y0, y1, W)
        for k in range(-6, 7):
            c = MC + 0.6 + 1.2 * k
            M.rect(x0 + 0.3, x1 - 0.3, max(c - 0.3, y0), min(c + 0.3, y1), W)
    across_fifth(50.0, 53.8)
    across_fifth(65.2, 69.0)
    across_fifth(-65.8, -62.4)
    across_fifth(92.4, 95.8)
    across_market(0.4, 3.8)
    across_market(-99.8, -96.4)
    # the bay's outline and BUS ONLY
    bx0, bx1, by0, by1 = BAY
    M.rect(bx0, bx1, by1 - 0.15, by1, W)
    M.rect(bx1 - 0.15, bx1, by0 + 0.05, by1, W)
    M.rect(bx0, bx0 + 0.15, by0 + 0.05, by1, W)
    bus_only(M, W)
    # parking T-marks on Market
    for x in np.arange(-38.0, -92.1, -6.0):
        M.rect(x - 0.3, x + 0.3, 56.35, 56.45, W)
        M.rect(x - 0.05, x + 0.05, 55.75, 56.35, W)
    for x in np.arange(-6.0, -90.1, -6.0):
        if -37.5 < x < -26.5:
            continue
        M.rect(x - 0.3, x + 0.3, 62.55, 62.65, W)
        M.rect(x - 0.05, x + 0.05, 62.65, 63.25, W)
    # valve lids along the curbs (iron, flush-ish), seeded
    rng = np.random.default_rng(1931)
    spots = []
    for y in np.arange(-58.0, 90.0, 9.5):
        spots.append((rng.uniform(0.8, 3.5), y + rng.uniform(-2, 2)))
        spots.append((rng.uniform(17.0, 19.8), y + rng.uniform(-2, 2) + 4.0))
    for x in np.arange(-6.0, -94.0, -11.0):
        spots.append((x + rng.uniform(-3, 3), rng.uniform(50.2, 53.2)))
        spots.append((x + rng.uniform(-3, 3) - 5.0, rng.uniform(65.8, 68.8)))
    lids = []
    for (x, y) in spots:
        if in_rects([x], [y], RAMP_AREAS + PIT_RECTS + VAULT_RECTS, 0.4)[0]:
            continue
        r = rng.choice([0.1, 0.12, 0.15])
        lids.append((x, y, r))
    sid_lids = Markings()
    for (x, y, r) in lids:
        sid_lids.disc(x, y, r, IRON_LID)
    M.emit(G)
    # the lids sit on the sidewalk (its own z, not the road's)
    emit_sidewalk_marks(G, sid_lids, sidewalk_z_fn(sidewalk_rects()))


def emit_sidewalk_marks(G, M, zf):
    for p, paint, wear, lift in M.polys:
        q = ccw(p)
        P2 = np.array(q, np.float64)
        Z = zf(P2[:, 0], P2[:, 1]) + lift
        rgb = np.array(H(paint)[:3], np.float32)
        C = np.concatenate([np.broadcast_to(rgb, (len(P2), 3)), np.full((len(P2), 1), wear if wear is not None else 0.0)], 1)
        G.add(np.concatenate([P2, Z[:, None]], 1), P2, C, [len(q)], [MARKING])


def bus_only(M, paint):
    """BUS ONLY in the bay for a driver heading east (-X): letters 2.4 m long along the travel direction, reading from north
    (the driver's left) to south; BUS nearest the driver (west, larger X)."""
    for word, xc in (('BUS', -19.4), ('ONLY', -23.6)):
        t = text_mesh(word, 1.0, 'Bold', 'CENTER', 1.1)
        bm = bmesh.new()
        bm.from_mesh(t.data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        x0, x1, y0, y1 = text_bounds(t)
        width = 1.75 if word == 'ONLY' else 1.5
        sx = width / (x1 - x0)    # across the bay (Y)
        sy = 2.4 / (y1 - y0)      # along it (X)
        yc = (BAY[2] + BAY[3] - 0.15) / 2
        for f in bm.faces:
            poly = []
            for v in f.verts:
                tx = (v.co.x - (x0 + x1) / 2) * sx
                ty = (v.co.y - (y0 + y1) / 2) * sy
                # text +x (reading) -> stage -Y; text +y (up) -> stage -X (the direction of travel)
                poly.append((xc - ty, yc - tx))
            M.poly(poly, paint)
        bm.free()
        bpy.data.objects.remove(t)


# ================================================================== meshes

def which_mesh(X, Y):
    """Index into GROUND for face centroids."""
    X = np.asarray(X)
    Y = np.asarray(Y)
    out = np.full(len(X), 4, np.int32)                               # Far
    fifth = (X >= XM0) & (X <= XM1) & (Y >= HSL - 1.0) & (Y <= WNL + 1.0)
    out = np.where(fifth & (Y < 15.0), 0, out)
    out = np.where(fifth & (Y >= 15.0), 1, out)
    market = (X < 0.0) & (X >= MEAST) & (Y > MSL - 0.5) & (Y < MNL + 0.5)
    out = np.where(market, 2, out)
    card = (X < MEAST) & (X >= CEL - 1.0) & (Y >= YC0) & (Y <= YC1)
    out = np.where(card, 3, out)
    return out


def ground_material(name, hex_color, rough):
    """An unbaked ground slot (named for Unreal) whose base color reads the vertex colors, so the glTF carries them as
    COLOR_0."""
    m = core.Mat(name)
    va = m.node('ShaderNodeVertexColor')
    va.layer_name = 'Color'
    mx = m.nt.nodes.new('ShaderNodeMix')
    mx.data_type = 'RGBA'
    mx.blend_type = 'MULTIPLY'
    mx.inputs[0].default_value = 1.0
    mx.inputs[6].default_value = H(hex_color)
    m.link(va.outputs['Color'], mx.inputs[7])
    m.link(mx.outputs[2], m.bsdf.inputs['Base Color'])
    m.set('Roughness', rough)
    m.m.use_backface_culling = True
    if m.m.name != name:
        raise RuntimeError(f'material name clash: {m.m.name}')
    return m


_GROUND_MATS = None


def ground_mats():
    global _GROUND_MATS
    if _GROUND_MATS is None:
        spec = [(0x8a867e, 0.85), (0x2a2b2e, 0.75), (0x1c1d1f, 0.7), (0x8f8b84, 0.8), (0x6f6b64, 0.8), (0x9c988f, 0.85), (0xcf9f1c, 0.6),
                (0xffffff, 0.55), (0x7a2b24, 0.72)]
        _GROUND_MATS = [ground_material(n, h, r) for n, (h, r) in zip(SLOTS, spec)]
    return _GROUND_MATS


def make_meshes(G):
    P = np.concatenate([c[0] for c in G.chunks], 0)
    UV = np.concatenate([c[1] for c in G.chunks], 0)
    C = np.concatenate([c[2] for c in G.chunks], 0)
    S = np.concatenate([c[3] for c in G.chunks], 0)
    Mt = np.concatenate([c[4] for c in G.chunks], 0)
    starts = np.concatenate([[0], np.cumsum(S)[:-1]]).astype(np.int64)
    cen = np.add.reduceat(P[:, :2], starts, axis=0) / S[:, None]
    which = which_mesh(cen[:, 0], cen[:, 1])
    mats = ground_mats()
    objs = []
    keymap = {'SM_NS_Ground_FifthS': ('fifth',), 'SM_NS_Ground_FifthN': ('fifth',), 'SM_NS_Ground_Market': ('market',),
              'SM_NS_Ground_Cardinal': ('cardinal',), 'SM_NS_Ground_Far': ('far',)}
    for mi, name in enumerate(GROUND):
        if _ONLY and not any(k in _ONLY for k in ('ground',) + keymap[name]):
            continue
        fsel = np.nonzero(which == mi)[0]
        if len(fsel) == 0:
            continue
        Ss = S[fsel]
        lsel = np.repeat(starts[fsel], Ss) + (np.arange(Ss.sum()) - np.repeat(np.cumsum(Ss) - Ss, Ss))
        obj = mesh_from_loops(name, P[lsel], UV[lsel], C[lsel], Ss, Mt[fsel], mats)
        n_sk = add_skirts(obj)
        log(f'{name}: {n_sk} skirt quads')
        objs.append(obj)
        fi = np.zeros(len(obj.data.polygons), dtype=np.int32)
        obj.data.polygons.foreach_get('material_index', fi)
        used = sorted(set(fi.tolist()))
        log(f'{name}: {len(obj.data.polygons)} faces, {tri_count(obj)} tris, slots in the glb {[obj.material_slots[i].material.name for i in used]}')
    return objs


def mesh_from_loops(name, P, UV, C, S, M, mats):
    # weld by position (0.25 mm)
    q = np.round(P * 4000.0).astype(np.int64)
    key = ((q[:, 0] + (1 << 20)) << 42) | ((q[:, 1] + (1 << 20)) << 21) | (q[:, 2] + (1 << 20))
    uk, first, inv = np.unique(key, return_index=True, return_inverse=True)
    V = P[first]
    Vb = np.stack([-V[:, 0], V[:, 1], V[:, 2]], 1)
    # reverse each face's loops (the mirror flips windings)
    starts = np.concatenate([[0], np.cumsum(S)[:-1]]).astype(np.int64)
    idx = np.repeat(starts + S - 1, S) - (np.arange(S.sum()) - np.repeat(starts, S))
    loop_v = inv[idx]
    UVr = UV[idx]
    Cr = C[idx]
    # drop faces that collapsed (repeated vertices)
    me = bpy.data.meshes.new(name)
    me.vertices.add(len(Vb))
    me.vertices.foreach_set('co', Vb.astype(np.float32).ravel())
    me.loops.add(len(loop_v))
    me.loops.foreach_set('vertex_index', loop_v.astype(np.int32))
    me.polygons.add(len(S))
    me.polygons.foreach_set('loop_start', starts.astype(np.int32))
    try:
        me.polygons.foreach_set('loop_total', S.astype(np.int32))
    except (AttributeError, TypeError, RuntimeError):
        pass
    me.polygons.foreach_set('material_index', M.astype(np.int32))
    uvl = me.uv_layers.new(name='UVMap')
    uvl.data.foreach_set('uv', UVr.astype(np.float32).ravel())
    ca = me.color_attributes.new('Color', 'FLOAT_COLOR', 'CORNER')
    ca.data.foreach_set('color', Cr.astype(np.float32).ravel())
    me.update(calc_edges=True)
    me.validate(clean_customdata=False)
    for mt in mats:
        me.materials.append(mt.m)
    me.color_attributes.active_color = me.color_attributes['Color']
    me.color_attributes.render_color_index = me.color_attributes.find('Color')
    obj = core.link(bpy.data.objects.new(name, me))
    me.shade_smooth()
    try:
        me.set_sharp_from_angle(angle=math.radians(32.0))
    except AttributeError:
        pass
    return obj


SEAM_X = (XM0, XM1, 0.0, MEAST, XF0, XF1, CEL)
SEAM_Y = (15.0, YC0, YC1, YF0, YF1, HSL, WNL)


def skirt_edge(X, Y):
    """An open edge gets a skirt where it lies on a building line or a mesh's end (not at the curbs' backs, where the
    sidewalk meets the curb across its 6 mm joint)."""
    for (a, b) in building_segments():
        ax, ay = a
        bx, by = b
        if abs(ax - bx) < 1e-6 and abs(X - ax) < 0.01 and min(ay, by) - 0.01 <= Y <= max(ay, by) + 0.01:
            return True
        if abs(ay - by) < 1e-6 and abs(Y - ay) < 0.01 and min(ax, bx) - 0.01 <= X <= max(ax, bx) + 0.01:
            return True
    return any(abs(X - v) < 0.01 for v in SEAM_X) or any(abs(Y - v) < 0.01 for v in SEAM_Y)


def add_skirts(obj, depth=0.3):
    """A skirt down from every open edge that isn't a marking's, on the building lines and the meshes' ends."""
    me = obj.data
    bm = bmesh.new()
    bm.from_mesh(me)
    uvl = bm.loops.layers.uv.active
    col = bm.loops.layers.float_color.get('Color')
    made = 0
    for e in [e for e in bm.edges if e.is_boundary]:
        f = e.link_faces[0]
        if f.material_index in (MARKING, CURB):
            continue
        a, b = e.verts
        mid = (a.co + b.co) / 2
        if not skirt_edge(-mid.x, mid.y):
            continue
        loops = {l.vert: l for l in f.loops}
        if loops[a].link_loop_next.vert is not b:
            a, b = b, a
        a2 = bm.verts.new((a.co.x, a.co.y, a.co.z - depth))
        b2 = bm.verts.new((b.co.x, b.co.y, b.co.z - depth))
        try:
            nf = bm.faces.new((b, a, a2, b2))
        except ValueError:
            continue
        nf.material_index = f.material_index
        L = (a.co.xy - b.co.xy).length
        for loop, uv in zip(nf.loops, ((0.0, b.co.z), (L, a.co.z), (L, a2.co.z), (0.0, b2.co.z))):
            loop[uvl].uv = uv
            if col is not None:
                loop[col] = loops[a][col] if loop.vert in (a, a2) else loops[b][col]
        made += 1
    bm.to_mesh(me)
    bm.free()
    return made


# ================================================================== the castings

def iron_material(name, seed=0, letters=None, diamonds=True, rust=0.5, center_r=0.33, vents=None):
    """Cast iron: dark, rough, rust in the hollows, the raised high points worn bright by traffic. letters: (sheet path,
    half-size of the sheet in meters) mapped from above; diamonds: the raised diamond tread inside center_r."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    mott = noise(m, o, 9.0, 4.0)
    fine = noise(m, o, 220.0, 2.0)
    r = m.math('SQRT', m.math('ADD', m.math('MULTIPLY', x, x), m.math('MULTIPLY', y, y)))
    height = m.math('MULTIPLY', fine, 0.15)
    raised = None
    if diamonds:
        # raised diamond lugs on a 28 mm square grid (each a square turned 45 deg), grooves between
        u = m.math('DIVIDE', x, 0.028)
        v = m.math('DIVIDE', y, 0.028)
        fu = m.math('ABSOLUTE', m.math('SUBTRACT', m.math('FRACT', u), 0.5))
        fv = m.math('ABSOLUTE', m.math('SUBTRACT', m.math('FRACT', v), 0.5))
        dia = m.math('ADD', fu, fv)
        dmask = ramp(m, dia, 0.42, 0.34)
        inner = smooth_less(m, r, center_r, 0.002)
        out_c = ramp(m, r, 0.048, 0.052)
        raised = m.math('MULTIPLY', m.math('MULTIPLY', dmask, inner), out_c)
        height = m.math('ADD', height, raised)
    if letters is not None:
        path, half = letters
        uvv = vec(m, m.math('ADD', m.math('DIVIDE', x, 2 * half), 0.5), m.math('ADD', m.math('DIVIDE', y, 2 * half), 0.5), 0.0)
        img = m.image(path, non_color=True, vector=uvv, extension='CLIP')
        sp = m.node('ShaderNodeSeparateColor')
        m.link(img.outputs['Color'], sp.inputs['Color'])
        let = sp.outputs['Red']
        height = m.math('ADD', height, let)
        raised = let if raised is None else m.math('MAXIMUM', raised, let)
    # the tops of raised work and the top surface's high points polished; hollows rusty and dirty
    is_top = m.math('GREATER_THAN', z, -0.004)
    worn = m.math('MULTIPLY', ramp(m, mott, 0.55, 0.8), 0.35)
    if raised is not None:
        worn = m.math('MAXIMUM', worn, m.math('MULTIPLY', raised, 0.85))
    polish = m.math('MULTIPLY', is_top, worn)
    hollow = m.math('MULTIPLY', ao_cavity(m, 0.012, 2.0), 1.0)
    rusty = m.math('MAXIMUM', m.math('MULTIPLY', hollow, rust), m.math('MULTIPLY', ramp(m, noise(m, o, 14.0, 5.0), 0.58, 0.75), rust * 0.6))
    rusty = m.math('MULTIPLY', rusty, m.math('SUBTRACT', 1.0, polish))
    base = m.mix(ramp(m, mott, 0.3, 0.7), H(0x232324), H(0x2f2e2c))
    col = m.mix(rusty, base, m.mix(ramp(m, fine, 0.3, 0.8), H(0x4a2c1a), H(0x6b3b1f)))
    col = m.mix(m.math('MULTIPLY', hollow, 0.5), col, H(0x1a1916))
    col = m.mix(polish, col, m.mix(fine, H(0x6a6864), H(0x8a8782)))
    # everything below the cover's top skin is the dark of the shaft (seen through vent holes and pick holes)
    col = m.mix(ramp(m, z, -0.012, -0.03), col, H(0x060606))
    col = m.mix(attr(m, 'ns_dark'), col, H(0x050505))
    if vents is not None:
        col = m.mix(vents, col, H(0x050505))
    m.set('Base Color', col)
    m.set('Metallic', mix_float(m, polish, 0.15, 0.85))
    m.set('Roughness', mix_float(m, polish, mix_float(m, rusty, 0.62, 0.85), 0.32))
    set_bump(m, height, 0.6, 0.0035)
    return m


def manhole_letters(name, text, r_text, half, size=0.04):
    s = Sheet(2 * half * 1000, 2 * half * 1000)
    s.rect(0, 0, 2 * half * 1000, 2 * half * 1000, 0x000000)
    c = half * 1000
    s.text_on_arc(text, c, c, r_text * 1000, size * 1000, 0xffffff, face='Black', center_angle=math.pi / 2, tracking=1.25)
    # the lower half reads too: the year at the bottom
    return s


def build_manhole(vent=False):
    name = 'SM_NS_ManholeVent' if vent else 'SM_NS_Manhole'
    tag = 'nsv' if vent else 'nsm'
    half = 0.34
    s = Sheet(680, 680)
    s.rect(0, 0, 680, 680, 0x000000)
    # One run of letters round the band, reading clockwise from outside (the year comes out upside down at the bottom,
    # as on the real castings).
    s.text_on_arc('NORTHSIDE  ·  SEWER  ·  1931  ·', 340, 340, 252, 44, 0xffffff, face='Black', center_angle=math.pi / 2 + 0.35,
                  tracking=1.28)
    s.ring(340, 340, 300, 309, 0xffffff)
    s.ring(340, 340, 222, 230, 0xffffff)
    if not vent:
        s.circle(340, 340, 30, 0xffffff)
        s.ring(340, 340, 40, 46, 0xffffff)
    color, _ = s.render(f'{tag}_letters', 1400, samples=8)
    m = iron_material(f'{tag}_iron', letters=(color, half), diamonds=not vent, center_r=0.214)
    parts_ = []
    # frame: flange r 0.333..0.46 (top at 0, its outer edge chamfered down), body down to -0.06
    frame = [(0.333, -0.06), (0.46, -0.06), (0.46, -0.012), (0.452, -0.002), (0.444, 0.0), (0.338, 0.0), (0.333, -0.004)]
    seg = 144 if vent else 256
    parts_.append(lathe('frame', frame + [frame[0]], m, seg))
    # cover: r 0.33, top slightly crowned, a 3 mm gap to the frame
    cov = [(0.0, -0.05), (0.33, -0.05), (0.33, -0.004), (0.327, -0.001), (0.31, 0.0), (0.0, 0.0015)]
    cover = lathe('cover', cov + [(0.0, -0.05)], m, seg)
    parts_.append(cover)
    # pick holes: two slots near the edge (dark pockets)
    for ang in (0.0, math.pi):
        cx, cy = 0.285 * math.cos(ang), 0.285 * math.sin(ang)
        pocket = box(cx - 0.012, cx + 0.012, cy - 0.022, cy + 0.022, -0.03, 0.0005, None)
        core.boolean(cover, pocket)
        parts_.append(set_attr(box(cx - 0.012, cx + 0.012, cy - 0.022, cy + 0.022, -0.0019, -0.0017, m), 'ns_dark', 1.0))
    if vent:
        # the dark of the shaft under the vented cover (just above the recess floor, so it shows through the holes)
        parts_.append(set_attr(lathe('shaft', [(0.0, -0.0019), (0.33, -0.0019), (0.33, -0.0017), (0.0, -0.0017)], m, 64), 'ns_dark', 1.0))
        for ring_r, cnt in ((0.07, 6), (0.12, 10), (0.17, 14), (0.215, 18)):
            for k in range(cnt):
                a = 2 * math.pi * (k + 0.5 * (ring_r > 0.1)) / cnt
                cut = parts.disc('vent', (ring_r * math.cos(a), ring_r * math.sin(a), -0.02), (0, 0, 1), 0.016, 0.08, 10)
                core.boolean(cover, cut)
    obj = finish_mesh(parts_, name, [m], angle=40.0, tag='ns_ground')
    return obj


def build_curb_inlet():
    """The curb piece with its throat, the hood, the gutter apron and the grate (front -X = the road)."""
    conc = concrete_material('nsi_concrete')
    s = Sheet(1000, 60)
    s.rect(0, 0, 1000, 60, 0x000000)
    s.text('NO DUMPING  ·  DRAINS TO RIVER', 500, 18, 30, 0xffffff, face='Black', align='CENTER', tracking=1.15)
    lpath, _ = s.render('nsi_letters', 2000, samples=8)
    iron = core.Mat('nsi_iron')
    tc, sep = object_coords(iron)
    o = tc.outputs['Object']
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    # the hood's face is the plane x = -0.004 (facing -X): letters mapped across y (reading left to right from the road:
    # the viewer at -X sees +y on their right? facing +X, +Y is on the left) -> u = (0.5 - y), v from z
    uvv = vec(iron, m_add(iron, m_div(iron, m_neg(iron, y), 1.0), 0.5), m_div(iron, m_add(iron, z, 0.07), 0.06), 0.0)
    img = iron.image(lpath, non_color=True, vector=uvv, extension='CLIP')
    sp = iron.node('ShaderNodeSeparateColor')
    iron.link(img.outputs['Color'], sp.inputs['Color'])
    front = ramp(iron, m_neg(iron, x), 0.0, 0.003)
    let = iron.math('MULTIPLY', sp.outputs['Red'], front)
    mott = noise(iron, o, 9.0, 4.0)
    fine = noise(iron, o, 220.0, 2.0)
    rust = iron.math('MAXIMUM', ao_cavity(iron, 0.01, 2.0), ramp(iron, noise(iron, o, 16.0, 5.0), 0.55, 0.72))
    polish = iron.math('MAXIMUM', iron.math('MULTIPLY', let, 0.7), iron.math('MULTIPLY', ramp(iron, z, -0.16, -0.14),
                                                                              iron.math('MULTIPLY', ramp(iron, mott, 0.5, 0.8), 0.5)))
    base = iron.mix(ramp(iron, mott, 0.3, 0.7), H(0x232324), H(0x302e2b))
    col = iron.mix(iron.math('MULTIPLY', rust, iron.math('SUBTRACT', 1.0, polish)), base, iron.mix(fine, H(0x4a2c1a), H(0x6e3d20)))
    col = iron.mix(polish, col, H(0x75726d))
    # silt in the grate's slots and on the apron end
    iron.set('Base Color', col)
    iron.set('Metallic', mix_float(iron, polish, 0.2, 0.8))
    iron.set('Roughness', mix_float(iron, polish, 0.75, 0.35))
    iron.set('Base Color', iron.mix(attr(iron, 'ns_dark'), col, H(0x050505)))
    set_bump(iron, iron.math('ADD', let, iron.math('MULTIPLY', fine, 0.2)), 0.5, 0.002)
    out = []
    # the curb piece: the ground's profile (top 0, chamfered arris, 20 cm deep), 1.2 m long, the throat cut in its face
    prof = [(0.0, -0.23), (0.0, -0.012), (0.012, 0.0), (CW - 0.006, 0.0), (CW, -0.006), (CW, -0.25)]
    curbp = prism_y('curb', [(x_, z_) for x_, z_ in prof], -0.6, 0.6, conc)
    throat = box(-0.05, 0.16, -0.5, 0.5, -0.17, -0.06, None)
    core.boolean(curbp, throat)
    out.append(curbp)
    # the throat's dark inside: a back wall and a floor sloping into the dark
    out.append(box(0.15, 0.18, -0.5, 0.5, -0.4, -0.05, iron))
    out.append(box(0.0, 0.16, -0.5, 0.5, -0.30, -0.27, conc))
    # the hood: a cast-iron plate on the face over the throat, its letters cast in
    hood = box(-0.008, 0.002, -0.55, 0.55, -0.07, -0.012, iron, r=0.003, seg=3)
    out.append(hood)
    # the letters cast in relief (1 mm proud), where the material's letter map polishes them
    txt = text_mesh('NO DUMPING  ·  DRAINS TO RIVER', 0.03, face='Black', align='CENTER', tracking=1.15, extrude=0.001, res=4)
    txt.data.transform(Matrix(((0, 0, -1, -0.008), (-1, 0, 0, 0.0), (0, 1, 0, -0.052), (0, 0, 0, 1))))
    core.assign(txt, iron)
    out.append(txt)
    for yy in (-0.52, 0.52):
        for zz in (-0.058, -0.024):
            out.append(lathe('bolt', [(0.0, 0.0), (0.008, 0.0), (0.008, 0.005), (0.006, 0.006), (0.0, 0.0065)], iron, 6,
                             Matrix.Translation((-0.008, yy, zz)) @ Matrix.Rotation(math.radians(-90), 4, 'Y')))
    # gutter apron (concrete) with the grate's frame and bars
    za, zb = FLOW, -0.1455
    ap = []
    bm = bmesh.new()
    xs = [0.0, -0.02, -0.435, -0.45]
    rows = [-0.6, -0.48, 0.48, 0.6]
    vs = {}
    for i, xx in enumerate(xs):
        for j, yy in enumerate(rows):
            zz = za + (zb - za) * (-xx / 0.45)
            vs[(i, j)] = bm.verts.new((xx, yy, zz))
    for i in range(len(xs) - 1):
        for j in range(len(rows) - 1):
            if i == 1 and j == 1:
                continue   # the grate's opening
            f = bm.faces.new((vs[(i, j)], vs[(i, j + 1)], vs[(i + 1, j + 1)], vs[(i + 1, j)]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    apron = core.mesh_object('apron', bm)
    core.orient_normals(apron, (0, 0, 1))
    core.assign(apron, conc)
    # give it thickness down (so its edges read): a skirt
    out.append(apron)
    out.append(box(-0.45, 0.0, -0.6, 0.6, -0.3, -0.2, conc))
    # the grate: a frame and 14 bars across the flow (bicycle safe), sloped with the pan
    gx0, gx1 = -0.43, -0.025
    tilt = math.atan2(zb - za, 0.45)

    def pan_z(xx):
        return za + (zb - za) * (-xx / 0.45)
    fr_w = 0.02
    for (a0, a1, b0, b1) in ((gx0, gx1, -0.48, -0.48 + fr_w), (gx0, gx1, 0.48 - fr_w, 0.48), (gx0, gx0 + fr_w, -0.48, 0.48), (gx1 - fr_w, gx1, -0.48, 0.48)):
        b = box(a0, a1, b0, b1, -0.035, 0.0, iron, r=0.003, seg=3)
        for v in b.data.vertices:
            v.co.z += pan_z(v.co.x)
        out.append(b)
    nb = 14
    for k in range(nb):
        yy = -0.46 + 0.92 * (k + 0.5) / nb
        b = box(gx0 + fr_w * 0.5, gx1 - fr_w * 0.5, yy - 0.011, yy + 0.011, -0.03, 0.0, iron, r=0.003, seg=3)
        for v in b.data.vertices:
            v.co.z += pan_z(v.co.x)
        out.append(b)
    # two cross ribs under the bars (along the flow)
    for xx in (-0.31, -0.15):
        b = box(xx - 0.008, xx + 0.008, -0.47, 0.47, -0.06, -0.028, iron, r=0.002, seg=2)
        for v in b.data.vertices:
            v.co.z += pan_z(v.co.x)
        out.append(b)
    # the catch basin's dark below the grate: a plate along the pan, 1.6 mm under it (the ground's gutter is sunk 2 mm here)
    dk = box(gx0, gx1, -0.48, 0.48, -0.0017, -0.0015, iron)
    for v in dk.data.vertices:
        v.co.z += pan_z(v.co.x)
    out.append(set_attr(dk, 'ns_dark', 1.0))
    for (a0, a1, b0, b1) in ((gx0 - 0.01, gx0, -0.48, 0.48), (gx1, gx1 + 0.01, -0.48, 0.48), (gx0, gx1, -0.49, -0.48), (gx0, gx1, 0.48, 0.49)):
        out.append(box(a0, a1, b0, b1, -0.6, -0.17, conc))
    obj = finish_mesh(out, 'SM_NS_CurbInlet', [conc, iron], angle=40.0, tag='ns_ground')
    return obj


def m_add(m, a, b):
    return m.math('ADD', a, b)


def m_div(m, a, b):
    return m.math('DIVIDE', a, b)


def m_neg(m, a):
    return m.math('MULTIPLY', a, -1.0)


def concrete_material(name, hex_color=0x8f8b84, grime=0.6):
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    z = sep.outputs['Z']
    blot = noise(m, o, 6.0, 5.0)
    sand = noise(m, o, 400.0, 1.0)
    pits = ramp(m, noise(m, o, 260.0, 1.0), 0.7, 0.74)
    col = m.mix(ramp(m, blot, 0.3, 0.7), H(hex_color), H(0x7a766f))
    col = m.mix(m.math('MULTIPLY', sand, 0.25), col, H(0xa8a49c))
    col = m.mix(m.math('MULTIPLY', pits, 0.6), col, H(0x3a3936))
    dirt = m.math('MULTIPLY', ramp(m, z, -0.04, -0.17), grime)
    col = m.mix(dirt, col, H(0x3b3833))
    edge = convex_edges(m, tc, distance=0.006, samples=8, breakup_scale=40.0, gain=4.0)
    col = m.mix(m.math('MULTIPLY', edge, 0.3), col, H(0xaaa59c))
    m.set('Base Color', col)
    m.set('Roughness', m.math('ADD', 0.84, m.math('MULTIPLY', blot, 0.06)))
    set_bump(m, m.math('SUBTRACT', m.math('ADD', m.math('MULTIPLY', blot, 0.4), m.math('MULTIPLY', sand, 0.2)), pits), 0.3, 0.001)
    return m


def build_tree_grate():
    m = iron_material('nst_iron', diamonds=False, rust=1.0)
    # two halves (split along x = 0 ... across Y), radial slots, the trunk hole (r 0.23)
    out = []
    for side in (-1, 1):
        # the half's outline: x from 0.004 to 0.6 (times side), y -0.6..0.6, minus the trunk half-disc
        outer = []
        n = 48
        if side > 0:
            outer = [(0.004, -0.23)] + [(0.004, -0.6), (0.6, -0.6), (0.6, 0.6), (0.004, 0.6), (0.004, 0.23)]
            hole = [(0.23 * math.cos(a), 0.23 * math.sin(a)) for a in np.linspace(math.pi / 2, -math.pi / 2, n)]
            outline = outer + hole[1:-1]
        else:
            outer = [(-0.004, 0.23), (-0.004, 0.6), (-0.6, 0.6), (-0.6, -0.6), (-0.004, -0.6), (-0.004, -0.23)]
            hole = [(0.23 * math.cos(a), 0.23 * math.sin(a)) for a in np.linspace(-math.pi / 2, -3 * math.pi / 2, n)]
            outline = outer + hole[1:-1]
        holes = []
        for ring_r0, ring_r1, cnt in ((0.27, 0.33, 9), (0.37, 0.44, 12), (0.48, 0.555, 15)):
            for k in range(cnt):
                a_mid = (k + 0.5) / cnt * math.pi - math.pi / 2
                a_mid = a_mid if side > 0 else a_mid + math.pi
                hw = 0.011
                pts = []
                ca_, sa_ = math.cos(a_mid), math.sin(a_mid)
                ra, rb = ring_r0 + hw, ring_r1 - hw
                # a stadium: straight sides along the radius, half-round ends (5 points each)
                for t in np.linspace(0, 1, 4)[1:-1]:
                    rr = ra + (rb - ra) * t
                    pts.append((rr * ca_ + hw * sa_, rr * sa_ - hw * ca_))
                for k in range(5):
                    a = -math.pi / 2 + math.pi * k / 4
                    pts.append((rb * ca_ + hw * (math.cos(a) * ca_ - math.sin(a) * sa_), rb * sa_ + hw * (math.cos(a) * sa_ + math.sin(a) * ca_)))
                for t in np.linspace(1, 0, 4)[1:-1]:
                    rr = ra + (rb - ra) * t
                    pts.append((rr * ca_ - hw * sa_, rr * sa_ + hw * ca_))
                for k in range(5):
                    a = math.pi / 2 + math.pi * k / 4
                    pts.append((ra * ca_ + hw * (math.cos(a) * ca_ - math.sin(a) * sa_), ra * sa_ + hw * (math.cos(a) * sa_ + math.sin(a) * ca_)))
                # corners of the square beyond the outer ring get slots only where they fit
                if max(abs(p[0]) for p in pts) < 0.585 and max(abs(p[1]) for p in pts) < 0.585:
                    holes.append(ccw(pts))
        # corner slots (the square's corners)
        for (cxx, cyy) in ((0.5, 0.5), (0.5, -0.5)):
            cxx *= side
            for k in range(3):
                off = (k - 1) * 0.035
                hole_pts = [(cxx - 0.06 + off * 0.0, cyy - 0.012 + off), (cxx + 0.06, cyy - 0.012 + off), (cxx + 0.06, cyy + 0.012 + off),
                            (cxx - 0.06, cyy + 0.012 + off)]
                if all(0.03 < abs(p[0]) < 0.585 and abs(p[1]) < 0.585 for p in hole_pts):
                    if min(math.hypot(*p) for p in hole_pts) > 0.57:
                        holes.append(ccw(hole_pts))
        half = parts.outline_prism('half', ccw(outline), holes, -0.03, 0.0)
        core.assign(half, m)
        out.append(half)
        # ribs under it (seen through the slots): one along the split, radial ones between the slot rings
        out.append(box(min(side * 0.24, side * 0.58), max(side * 0.24, side * 0.58), -0.012, 0.012, -0.06, -0.03, m, r=0.003))
        for k in range(5):
            a = (k + 1) / 6 * math.pi - math.pi / 2 + (0 if side > 0 else math.pi)
            c_, s_ = math.cos(a), math.sin(a)
            rib = box(0.25, 0.57, -0.008, 0.008, -0.055, -0.03, m, r=0.003)
            rib.data.transform(Matrix.Rotation(a, 4, 'Z'))
            out.append(rib)
        # the bolt-down lugs in the outer corners: hex bolts, sunk flush
        for cy_ in (-0.555, 0.555):
            cx_ = side * 0.555
            out.append(lathe('bolt', [(0.0, -0.004), (0.014, -0.004), (0.014, 0.0), (0.0, 0.0)], m, 6, Matrix.Translation((cx_, cy_, 0.0))))
            out.append(lathe('washer', [(0.0, -0.006), (0.022, -0.006), (0.022, -0.003), (0.0, -0.003)], m, 24, Matrix.Translation((cx_, cy_, 0.0))))
    obj = finish_mesh(out, 'SM_NS_TreeGrate', [m], angle=40.0, tag='ns_ground')
    return obj


def build_vault_lights():
    steel = iron_material('nsl_steel', diamonds=False, rust=0.7)
    glass = core.Mat('nsl_glass')
    tc, sep = object_coords(glass)
    o = tc.outputs['Object']
    x, y = sep.outputs['X'], sep.outputs['Y']
    # each lens: aged purple or pale green-grey by a hash of its cell, cracked in some
    cell = vec(glass, glass.math('FLOOR', glass.math('DIVIDE', glass.math('ADD', x, 0.55), 0.11)),
               glass.math('FLOOR', glass.math('DIVIDE', glass.math('ADD', y, 1.155), 0.11)), 0.0)
    wn = glass.node('ShaderNodeTexWhiteNoise')
    wn.noise_dimensions = '3D'
    glass.link(cell, wn.inputs['Vector'])
    h = wn.outputs['Value']
    # (about a quarter turned amethyst by a century of sun; the rest a dull green-grey, dark with the vault below them)
    purple = ramp(glass, h, 0.72, 0.73)
    col = glass.mix(purple, H(0x5c6863), H(0x66587c))
    col = glass.mix(ramp(glass, noise(glass, o, 40.0, 3.0), 0.4, 0.8), col, glass.mix(purple, H(0x2c3330), H(0x342b42)))
    crack = glass.math('MULTIPLY', ramp(glass, h, 0.85, 0.86), ramp(glass, noise(glass, o, 90.0, 6.0, distortion=4.0), 0.48, 0.5))
    col = glass.mix(crack, col, H(0x8f928c))
    dirt = ramp(glass, noise(glass, o, 12.0, 4.0), 0.5, 0.8)
    col = glass.mix(glass.math('MULTIPLY', dirt, 0.6), col, H(0x2a2824))
    # the lens reads round: past r 30 mm (the octagon's corners) it's the grimy seal in its hole
    fx = glass.math('SUBTRACT', glass.math('FRACT', glass.math('DIVIDE', glass.math('ADD', x, 0.55), 0.11)), 0.5)
    fy = glass.math('SUBTRACT', glass.math('FRACT', glass.math('DIVIDE', glass.math('ADD', y, 1.155), 0.11)), 0.5)
    rr = glass.math('MULTIPLY', glass.math('SQRT', glass.math('ADD', glass.math('MULTIPLY', fx, fx), glass.math('MULTIPLY', fy, fy))), 0.11)
    seal = ramp(glass, rr, 0.0293, 0.0303)
    col = glass.mix(seal, col, H(0x1f1d1a))
    glass.set('Base Color', col)
    glass.set('Roughness', glass.math('ADD', glass.math('ADD', 0.12, glass.math('MULTIPLY', dirt, 0.5)), glass.math('MULTIPLY', seal, 0.45)))
    glass.set('Specular IOR Level', 0.6)
    out = []
    # the steel slab: 1.2 x 2.4, 3 cm, holes for 10 x 21 lenses (r 0.034 on 11 cm centers)
    outline = rectp(-0.6, 0.6, -1.2, 1.2)
    holes = []
    lenses = []
    for i in range(10):
        for j in range(21):
            cx = -0.495 + 0.11 * i
            cy = -1.1 + 0.11 * j
            holes.append([(cx + 0.034 * math.cos(2 * math.pi * k / 8), cy + 0.034 * math.sin(2 * math.pi * k / 8)) for k in range(8)])
            lenses.append((cx, cy))
    slab = parts.outline_prism('slab', outline, holes, -0.03, -0.0005)
    bm = bmesh.new()
    bm.from_mesh(slab.data)
    bmesh.ops.delete(bm, geom=[fc for fc in bm.faces if fc.calc_center_median().z < -0.0295 and fc.normal.z < -0.9], context='FACES')
    bm.to_mesh(slab.data)
    bm.free()
    core.assign(slab, steel)
    out.append(slab)
    # a raised rim round the panel
    for (a0, a1, b0, b1) in ((-0.6, 0.6, -1.2, -1.18), (-0.6, 0.6, 1.18, 1.2), (-0.6, -0.58, -1.2, 1.2), (0.58, 0.6, -1.2, 1.2)):
        out.append(box(a0, a1, b0, b1, -0.03, 0.0, steel, r=0.002))
    # lenses: low domes sitting in the holes, a hair under the slab's top
    bm = bmesh.new()
    for (cx, cy) in lenses:
        ring0 = [bm.verts.new((cx + 0.034 * math.cos(2 * math.pi * k / 8), cy + 0.034 * math.sin(2 * math.pi * k / 8), -0.0019)) for k in range(8)]
        ring1 = [bm.verts.new((cx + 0.024 * math.cos(2 * math.pi * k / 8), cy + 0.024 * math.sin(2 * math.pi * k / 8), -0.0007)) for k in range(8)]
        top = bm.verts.new((cx, cy, 0.0003))
        for k in range(8):
            k1 = (k + 1) % 8
            bm.faces.new((ring0[k], ring0[k1], ring1[k1], ring1[k]))
            bm.faces.new((ring1[k], ring1[k1], top))
    lens_obj = core.mesh_object('lenses', bm)
    core.orient_normals(lens_obj, (0, 0, 1))
    core.assign(lens_obj, glass)
    out.append(lens_obj)
    obj = finish_mesh(out, 'SM_NS_VaultLights', [steel, glass], angle=40.0, tag='ns_ground')
    return obj


# ================================================================== build

BUILDERS = {'manhole': lambda: [build_manhole(False)], 'vent': lambda: [build_manhole(True)], 'inlet': lambda: [build_curb_inlet()],
            'grate': lambda: [build_tree_grate()], 'vault': lambda: [build_vault_lights()]}


def build():
    core.reset()
    if wanted('tex') and os.environ.get('NS_GROUND_TEX', '1') != '0':
        make_textures()
    objs = []
    if wanted('ground', 'fifth', 'market', 'cardinal', 'far'):
        objs += build_ground()
    for key, fn in BUILDERS.items():
        if _ONLY and key not in _ONLY and 'castings' not in _ONLY:
            continue
        t0 = time.time()
        made = fn()
        objs += made
        log(f'built {key} ({", ".join(o.name for o in made)}) in {time.time() - t0:.1f}s')
    for o in objs:
        o.data.name = o.name
    return objs


def bake_parts(objs):
    bake = []
    for o in objs:
        if o.name in UNBAKED or '_Glow' in o.name:
            o.hide_render = True
        else:
            bake.append(o)
    return bake


# ================================================================== review

def _bl(p):
    return Vector((-p[0], p[1], p[2]))


def _which_reviews():
    v = os.environ.get('NS_GROUND_REVIEW', 'site,props,tex')
    return {s.strip() for s in v.split(',') if s.strip()}


TILE = {'Sidewalk_A': 3.0, 'Asphalt': 4.0, 'Patch': 4.0, 'Curb': 2.0, 'Gutter': 2.0, 'Concrete': 2.0, 'Tactile': 0.6, 'Thermo': 2.0, 'BusBay': 4.0}


def texture_sheet():
    names = ['Sidewalk_A', 'Sidewalk_B', 'Asphalt', 'Patch', 'BusBay', 'Curb', 'Gutter', 'Concrete', 'Tactile', 'Thermo']
    tiles = []
    d = os.path.join(core.BUILD_DIR, 'review', 'ns_ground')
    os.makedirs(d, exist_ok=True)
    for nm in names:
        p = os.path.join(TEX_DIR, f'T_NS_{nm}_BaseColor.png')
        q = os.path.join(TEX_DIR, f'T_NS_{nm}_Normal.png')
        if not os.path.exists(p):
            continue
        a = review._pixels(p)[:, :, :3]
        nrm = review._pixels(q)[:, :, :3]
        lit = np.clip((nrm[..., 0] - 0.5) * 1.3 + (nrm[..., 1] - 0.5) * 1.3 + 0.85, 0.3, 1.4)[..., None]
        a = np.clip(a * lit, 0, 1)
        t = np.tile(a, (2, 2, 1))
        S = 640
        f = t.shape[0] // S
        t = t[:f * S, :f * S].reshape(S, f, S, f, 3).mean(axis=(1, 3))
        img = bpy.data.images.new('rv_tex', S, S, alpha=False)
        img.pixels.foreach_set(np.dstack([t, np.ones((S, S))]).astype(np.float32).ravel())
        path = os.path.join(d, f'tex_{nm}.png')
        img.filepath_raw = path
        img.file_format = 'PNG'
        img.save()
        bpy.data.images.remove(img)
        tiles.append(path)
    if tiles:
        return compose_grid('ns_ground_textures', tiles, 5)


def decal_sheet():
    """art/review/ns_ground_decals.jpg: the decal atlas laid over T_NS_Sidewalk_A (alpha over, lit by its normal)."""
    p = os.path.join(TEX_DIR, 'T_NS_Decals_BaseColor.png')
    q = os.path.join(TEX_DIR, 'T_NS_Sidewalk_A_BaseColor.png')
    if not (os.path.exists(p) and os.path.exists(q)):
        return
    dec = review._pixels(p)
    nrm = review._pixels(os.path.join(TEX_DIR, 'T_NS_Decals_Normal.png'))
    g = review._pixels(q)[:, :, :3]
    n = dec.shape[0]
    g = np.tile(g, (max(1, n // g.shape[0]), max(1, n // g.shape[1]), 1))[:n, :n]
    a = dec[..., 3:4]
    lit = np.clip((nrm[..., 0:1] - 0.5) * 1.3 + (nrm[..., 1:2] - 0.5) * 1.3 + 1.0, 0.5, 1.4)
    out = g * (1 - a) + np.clip(dec[..., :3] * lit, 0, 1) * a
    S = n // 2
    out = out.reshape(S, 2, S, 2, 3).mean(axis=(1, 3))
    path = os.path.join(core.REVIEW_DIR, 'ns_ground_decals.jpg')
    img = bpy.data.images.new('rv_decals', S, S, alpha=False)
    img.pixels.foreach_set(np.dstack([out, np.ones((S, S))]).astype(np.float32).ravel())
    img.filepath_raw = path
    img.file_format = 'JPEG'
    img.save(quality=90)
    bpy.data.images.remove(img)
    log('review sheet', os.path.relpath(path, core.ROOT))


class Review:
    """The site renders: the ground with tiling previews of the sets (wet or dry from COLOR_0 as M_NS_Ground will),
    the castings placed, context meshes (the fronts, the lamps, the sedan) and stand-in blocks, a grey day and a wet night."""

    def __init__(self, objs):
        self.sc = bpy.context.scene
        self.objs = {o.name: o for o in objs}
        self.temp = []
        self.day = []
        self.night = []
        self.glows = []
        self.wet_nodes = []
        self.dir = os.path.join(core.BUILD_DIR, 'review', 'ns_ground')
        os.makedirs(self.dir, exist_ok=True)
        self.saved = {o.name: (o.location.copy(), o.rotation_euler.copy(), o.scale.copy(), o.hide_render) for o in objs}

    def _mat(self, name, hexc, rough=0.5, emit=None, strength=0.0, metal=0.0):
        m = core.Mat(name)
        m.set('Base Color', H(hexc))
        m.set('Roughness', rough)
        m.set('Metallic', metal)
        if emit is not None:
            m.set('Emission Color', H(emit))
            m.set('Emission Strength', strength)
        return m

    def dup(self, o):
        d = o.copy()
        self.sc.collection.objects.link(d)
        self.temp.append(d)
        return d

    def place(self, o, X, Y, Z, F, frame='prop'):
        o.location = _bl((X, Y, Z))
        if frame == 'prop':
            o.rotation_euler = (0.0, 0.0, math.radians(-F))
        else:   # car / -Y fronts
            o.rotation_euler = (0.0, 0.0, math.radians(-F - 90.0))

    # ---- materials
    def ground_preview(self):
        """Every ground mesh's slots get preview shading: the tiling set at its size, the A/B sidewalk blend, wet darkening
        and puddles from COLOR_0 (R x puddle), grime (G), markings' paint and wear."""
        self.wet = []
        cache = {}
        for name in GROUND:
            g = self.objs.get(name)
            if g is None:
                continue
            for i, slot in enumerate(g.material_slots):
                if i not in cache:
                    cache[i] = self._slot_preview(i)
                g.material_slots[i].material = cache[i].m

    def _tex(self, m, nm, tile, vector=None):
        base = os.path.join(TEX_DIR, f'T_NS_{nm}')
        uv = m.node('ShaderNodeUVMap')
        uv.uv_map = 'UVMap'
        mp = m.node('ShaderNodeMapping')
        mp.inputs['Scale'].default_value = (1.0 / tile, 1.0 / tile, 1.0)
        m.link(uv.outputs['UV'], mp.inputs['Vector'])
        col = m.image(base + '_BaseColor.png', vector=mp.outputs['Vector'])
        orm = m.image(base + '_ORM.png', non_color=True, vector=mp.outputs['Vector'])
        nrm = m.image(base + '_Normal.png', non_color=True, vector=mp.outputs['Vector'])
        sp = m.node('ShaderNodeSeparateColor')
        m.link(orm.outputs['Color'], sp.inputs['Color'])
        return col.outputs['Color'], sp.outputs['Green'], sp.outputs['Blue'], nrm.outputs['Color']

    def _slot_preview(self, i):
        nm = SLOTS[i]
        m = core.Mat('rv_' + nm)
        va = m.node('ShaderNodeVertexColor')
        va.layer_name = 'Color'
        vs = m.node('ShaderNodeSeparateColor')
        m.link(va.outputs['Color'], vs.inputs['Color'])
        wet_v = m.node('ShaderNodeValue')
        pud_v = m.node('ShaderNodeValue')
        self.wet.append((wet_v, pud_v))
        if i == SIDEWALK:
            ca, ra, ma, na = self._tex(m, 'Sidewalk_A', 3.0)
            cb, rb, mb, nb = self._tex(m, 'Sidewalk_B', 3.0)
            tc = m.node('ShaderNodeTexCoord')
            blendf = ramp(m, noise(m, tc.outputs['Object'], 0.12, 3.0), 0.42, 0.58)
            c = m.mix(blendf, ca, cb)
            r = mix_float(m, blendf, ra, rb)
            met = ma
            nmap = m.nt.nodes.new('ShaderNodeMix')
            nmap.data_type = 'RGBA'
            m.link(blendf, nmap.inputs[0])
            m.link(na, nmap.inputs[6])
            m.link(nb, nmap.inputs[7])
            n = nmap.outputs[2]
        elif i == MARKING:
            c0, r, met, n = self._tex(m, 'Thermo', 2.0)
            c = m.mix(1.0, c0, va.outputs['Color'], blend='MULTIPLY')
            # worn: the paint thins to the asphalt where the wear pattern is under the vertex alpha
            wear_img = m.image(os.path.join(TEX_DIR, 'T_NS_Thermo_Wear.png'), non_color=True)
            uv = m.node('ShaderNodeUVMap')
            mp = m.node('ShaderNodeMapping')
            mp.inputs['Scale'].default_value = (0.5, 0.5, 1.0)
            m.link(uv.outputs['UV'], mp.inputs['Vector'])
            m.link(mp.outputs['Vector'], wear_img.inputs['Vector'])
            alpha_n = m.node('ShaderNodeVertexColor')
            alpha_n.layer_name = 'Color'
            worn = ramp(m, m.math('SUBTRACT', alpha_n.outputs['Alpha'], wear_img.outputs['Color']), -0.05, 0.05)
            c = m.mix(worn, c, H(0x26272a))
        else:
            key = {ASPHALT: ('Asphalt', 4.0), PATCH: ('Patch', 4.0), CURB: ('Curb', 2.0), GUTTER: ('Gutter', 2.0), CONCRETE: ('Concrete', 2.0),
                   TACTILE: ('Tactile', 0.6), BUSBAY: ('BusBay', 4.0)}[i]
            c, r, met, n = self._tex(m, *key)
        if i != MARKING:
            grime = vs.outputs['Green']
            c = m.mix(m.math('MULTIPLY', ramp(m, grime, 0.25, 1.0), 0.45), c, m.mix(1.0, c, H(0x56504a), blend='MULTIPLY'))
            dry_cover = m.math('SUBTRACT', 1.0, m.math('MULTIPLY', vs.outputs['Blue'], 0.8))
            wet = m.math('MULTIPLY', wet_v.outputs['Value'], dry_cover)
            puddle = ramp(m, m.math('MULTIPLY', vs.outputs['Red'], pud_v.outputs['Value']), 0.42, 0.5)
        else:
            wet = wet_v.outputs['Value']
            puddle = ramp(m, m.math('MULTIPLY', 0.0, pud_v.outputs['Value']), 0.42, 0.5)
        c = m.mix(m.math('MULTIPLY', wet, 0.5), c, m.mix(1.0, c, (0.45, 0.45, 0.45, 1.0), blend='MULTIPLY'))
        c = m.mix(m.math('MULTIPLY', puddle, 0.6), c, m.mix(1.0, c, (0.25, 0.25, 0.27, 1.0), blend='MULTIPLY'))
        m.set('Base Color', c)
        rr = m.math('MULTIPLY', r, m.math('SUBTRACT', 1.0, m.math('MULTIPLY', wet, 0.65)))
        m.set('Roughness', mix_float(m, puddle, rr, 0.03))
        m.set('Metallic', mix_float(m, puddle, met, 0.0))
        nmn = m.node('ShaderNodeNormalMap')
        m.link(n, nmn.inputs['Color'])
        m.link(m.math('SUBTRACT', 1.0, puddle), nmn.inputs['Strength'])
        m.set('Normal', nmn.outputs['Normal'])
        return m

    def set_wet(self, wet, puddle):
        for a, b in self.wet:
            a.outputs[0].default_value = wet
            b.outputs[0].default_value = puddle

    # ---- context
    def context(self):
        self.ctx = []
        brick = self._brick('rv_brick', 0x5b3328)
        brick2 = self._brick('rv_brick2', 0x7a6a52)
        brick3 = self._brick('rv_brick3', 0x3b3330)
        self.bricks = [brick, brick2, brick3]
        stone = self._mat('rv_limestone', 0xa59c8a, 0.7)
        have = {}
        # the legacy fronts, placed as AStreetStage::Facade does
        fronts = [('SM_Facade_1812', 0.0, 0.0, 0.0), ('SM_Facade_Brownstone', 0.0, -18.125, 0.0), ('SM_Facade_Brownstone', 0.0, -11.375, 0.0),
                  ('SM_Facade_Walkup', 0.0, 29.9, 0.0), ('SM_Tenement', FAR, 9.2, 180.0), ('SM_Facade_Brownstone', FAR, 30.575, 180.0),
                  ('SM_Facade_Brownstone', FAR, 37.325, 180.0), ('SM_Facade_Brownstone', FAR, 44.075, 180.0)]
        cache = {}
        for nm, X, Y, F in fronts:
            if nm not in cache:
                cache[nm] = import_glb(nm, self.temp)
                if cache[nm] is not None:
                    self._fix_lit(cache[nm])
                o = cache[nm]
            else:
                o = self.dup(cache[nm]) if cache[nm] is not None else None
            if o is None:
                continue
            self.place(o, X, Y, 0.0, F)
            have[(X, Y)] = True
        # stand-in blocks for the plots still to come (heights by floors)
        blocks = [
            (-26.0, 0.0, -62.0, -36.0, 15.0, 0), (-26.0, 0.0, -36.0, -21.5, 17.0, 1), (-14.0, 0.0, 8.0, 23.0, 11.0, 2), (-14.0, 0.0, 37.0, 49.5, 4.6, 99),
            (-14.0, 0.0, 69.5, 80.0, 14.0, 0), (-14.0, 0.0, 80.0, 92.0, 17.0, 1),
            (FAR, FAR + 18.0, -62.0, -43.0, 14.0, 2), (FAR, FAR + 18.0, -43.0, -23.0, 22.0, 1), (FAR, FAR + 18.0, -23.0, -8.8, 17.0, 0),
            (FAR, FAR + 18.0, 47.45, 63.0, 8.0, 1), (FAR, FAR + 18.0, 63.0, 92.0, 25.0, 2),
            (-24.0, -14.0, 24.0, 49.5, 14.0, 0), (-37.0, -24.0, 30.0, 49.5, 8.5, 2), (-45.0, -37.0, 30.0, 49.5, 14.0, 1), (-55.0, -45.0, 30.0, 49.5, 17.0, 0),
            (-66.0, -55.0, 30.0, 49.5, 8.0, 2), (-96.0, -66.0, 30.0, 49.5, 9.0, 0),
            (-15.0, 0.0, 69.5, 86.0, 14.0, 2), (-28.0, -15.0, 69.5, 86.0, 17.0, 0), (-36.0, -28.0, 69.5, 86.0, 14.0, 1), (-43.0, -36.0, 69.5, 86.0, 14.0, 0),
            (-63.0, -43.0, 69.5, 86.0, 21.0, 1), (-76.0, -63.0, 69.5, 86.0, 8.0, 2), (-96.0, -76.0, 69.5, 86.0, 14.0, 0),
            (-38.0, 0.0, -100.0, -80.4, 6.0, 9), (-1.8, 22.2, -100.0, -80.4, 14.0, 9), (22.2, 58.4, -100.0, -80.4, 15.0, 1),
            (-38.0, 0.0, 110.4, 130.0, 16.0, 2), (-0.8, 21.2, 110.4, 130.0, 18.0, 0), (21.2, 58.4, 110.4, 130.0, 15.0, 1),
            (-38.0, -26.0, -62.0, 0.0, 15.0, 1), (FAR + 18.0, 58.4, -62.0, 92.0, 16.0, 0), (-38.0, -14.0, 92.0, 110.4, 0.0, 0),
            (-130.0, -114.4, 0.0, 120.0, 14.0, 9),
        ]
        self.store_glow = self._mat('rv_store', 0x14171b, 0.3, 0xe8f0ff, 0.0)
        for (x0, x1, y0, y1, hgt, kind) in blocks:
            if hgt <= 0:
                continue
            if kind == 9:
                mat = stone
            elif kind == 99:
                mat = self.store_glow
            else:
                mat = self.bricks[kind % 3]
            o = box(-x1, -x0, y0, y1, 0.0, hgt, mat)
            self.temp.append(o)
        # lamps (Fifth's HPS heads) and parked cars, hydrant
        lamp = import_glb('SM_Streetlight', self.temp)
        self.lamp_spots = [(3.6, y, 0.0) for y in (-50.0, -20.0, 10.0, 40.5, 75.0)] + [(16.8, y, 180.0) for y in (-35.0, -5.0, 25.0, 57.5, 86.0)]
        if lamp is not None:
            for i, (X, Y, F) in enumerate(self.lamp_spots):
                o = lamp if i == 0 else self.dup(lamp)
                o.location = _bl((X, Y, 0.0))
                o.rotation_euler = (0.0, 0.0, math.radians(-F - 90.0))
        # parked cars: the sedan and the car kit as exported (each body on its four wheels at the hubs; ns_cars' Spec values)
        hubs = {'SM_Sedan': (0.785, (-1.42, 1.38), 0.326), 'SM_NS_Hatchback': (0.755, (-1.30, 1.25), 0.312),
                'SM_NS_SUV': (0.82, (-1.42, 1.36), 0.37), 'SM_NS_Cab': (0.785, (-1.42, 1.38), 0.326), 'SM_NS_Van': (0.865, (-1.86, 1.80), 0.345)}
        models = []
        for nm in ('SM_Sedan', 'SM_NS_Hatchback', 'SM_NS_SUV', 'SM_NS_Cab', 'SM_NS_Van'):
            body = import_glb(nm, self.temp)
            wheel = import_glb(f'{nm}_Wheel', self.temp) if body is not None else None
            if body is not None:
                body.location = (0.0, 0.0, -50.0)
                if wheel is not None:
                    wheel.location = (0.0, 0.0, -50.0)
                models.append((nm, body, wheel))
        spots = [(5.4, -30.0, 90.0), (5.4, 3.0, 90.0), (5.4, 33.0, 90.0), (15.0, 18.0, -90.0), (15.0, -40.0, -90.0), (15.0, 40.0, -90.0),
                 (-44.0, 55.2, 180.0), (-60.0, 55.2, 180.0), (-10.0, 63.8, 0.0), (-20.0, 63.8, 0.0), (-50.0, 63.8, 0.0)]
        tints = [0xd8d8d6, 0x1a1c20, 0x7d8288, 0x2c3e5c, 0x9a1f1c, 0xb9bcbf, 0x3a3f44, 0xe8e6e0, 0x5b4636, 0x22262b, 0xa7aaad]
        for i, (X, Y, F) in enumerate(spots if models else []):
            nm, body, wheel = models[i % len(models)]
            o = self.dup(body)
            o.data = o.data.copy()
            z = float(road_z(np.array([X]), np.array([Y]))[0])
            self.place(o, X, Y, z, F, frame='car')
            if nm not in ('SM_NS_Cab', 'SM_NS_Van'):
                m0 = o.data.materials[0].copy()
                b = next((n for n in m0.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None)
                if b is not None:
                    src = b.inputs['Base Color'].links[0].from_socket if b.inputs['Base Color'].is_linked else None
                    mx = m0.node_tree.nodes.new('ShaderNodeMix')
                    mx.data_type = 'RGBA'
                    mx.blend_type = 'MULTIPLY'
                    mx.inputs[0].default_value = 1.0
                    if src is not None:
                        m0.node_tree.links.new(src, mx.inputs[6])
                    mx.inputs[7].default_value = H(tints[i % len(tints)])
                    m0.node_tree.links.new(mx.outputs[2], b.inputs['Base Color'])
                o.data.materials[0] = m0
            if wheel is not None:
                track, axles, wz = hubs[nm]
                for ay in axles:
                    for sx in (-1, 1):
                        w = self.dup(wheel)
                        w.parent = o
                        w.matrix_parent_inverse = Matrix()
                        w.location = (sx * track, ay, wz)
                        w.rotation_euler = (0.0, 0.0, 0.0 if sx > 0 else math.pi)
        for _, body, wheel in models:
            body.hide_render = True
            if wheel is not None:
                wheel.hide_render = True
        hyd = import_glb('SM_Hydrant', self.temp)
        if hyd is not None:
            self.place(hyd, 3.6, 13.0, 0.0, 0.0, frame='car')

    def _brick(self, name, hexc):
        """A stand-in masonry block: brick coursing, a grid of windows (some lit at night)."""
        m = core.Mat(name)
        tc = m.node('ShaderNodeTexCoord')
        sep = m.node('ShaderNodeSeparateXYZ')
        m.link(tc.outputs['Object'], sep.inputs['Vector'])
        x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
        along = m.math('ADD', x, y)
        wx = m.math('FRACT', m.math('DIVIDE', along, 2.4))
        wz = m.math('FRACT', m.math('DIVIDE', z, 3.2))
        win = m.math('MULTIPLY', m.math('MULTIPLY', ramp(m, wx, 0.3, 0.32), ramp(m, wx, 0.72, 0.7)),
                     m.math('MULTIPLY', ramp(m, wz, 0.25, 0.27), ramp(m, wz, 0.82, 0.8)))
        win = m.math('MULTIPLY', win, ramp(m, z, 4.0, 4.2))
        cell = vec(m, m.math('FLOOR', m.math('DIVIDE', along, 2.4)), m.math('FLOOR', m.math('DIVIDE', z, 3.2)), 0.0)
        wn = m.node('ShaderNodeTexWhiteNoise')
        wn.noise_dimensions = '3D'
        m.link(cell, wn.inputs['Vector'])
        lit = m.math('MULTIPLY', win, ramp(m, wn.outputs['Value'], 0.62, 0.63))
        br = noise(m, tc.outputs['Object'], 3.0, 4.0)
        col = m.mix(ramp(m, br, 0.3, 0.7), H(hexc), H(core_shade(hexc, 0.75)))
        col = m.mix(win, col, H(0x0b0d10))
        m.set('Base Color', col)
        m.set('Roughness', mix_float(m, win, 0.85, 0.08))
        m.set('Emission Color', H(0xffbf78))
        e = m.node('ShaderNodeValue')
        e.outputs[0].default_value = 0.0
        m.link(m.math('MULTIPLY', lit, e.outputs[0]), m.bsdf.inputs['Emission Strength'])
        self.glows.append(e)
        return m

    def _fix_lit(self, o):
        """The legacy fronts' lit and printed slots: their emissive scaled by the hour (night 1, day 0.15), as
        DimImportedGlow does."""
        if not hasattr(self, 'lit'):
            self.lit = []
        for s in o.material_slots:
            mt = s.material
            if mt is None or not mt.node_tree:
                continue
            b = next((n for n in mt.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None)
            if b is not None and b.inputs['Emission Color'].is_linked:
                self.lit.append((b.inputs['Emission Strength'], b.inputs['Emission Strength'].default_value))

    def place_castings(self):
        put = []
        for name, spots in (('SM_NS_Manhole', [(x, y) for (x, y) in MANHOLES if (x, y) not in VENT_MANHOLES]),
                            ('SM_NS_ManholeVent', VENT_MANHOLES)):
            o = self.objs.get(name)
            if o is None:
                continue
            for i, (x, y) in enumerate(spots):
                inst = o if i == 0 else self.dup(o)
                z = float(road_crown_z(np.array([x]), np.array([y]))[0])
                self.place(inst, x, y, z, 0.0)
                put.append(inst)
        o = self.objs.get('SM_NS_CurbInlet')
        if o is not None:
            facing = {'home': 0.0, 'far': 180.0, 'ms': 90.0, 'mn': -90.0, 'hollis_n': -90.0, 'wexler_s': 90.0}
            for i, (key, a) in enumerate(INLETS):
                inst = o if i == 0 else self.dup(o)
                X, Y = {'home': (RX0, a), 'far': (RX1, a), 'ms': (a, MS), 'mn': (a, MN), 'hollis_n': (a, HR0), 'wexler_s': (a, WR0)}[key]
                self.place(inst, X, Y, 0.0, facing[key])
        o = self.objs.get('SM_NS_TreeGrate')
        if o is not None:
            for i, (x, y) in enumerate(TREE_PITS):
                inst = o if i == 0 else self.dup(o)
                self.place(inst, x, y, 0.0, 0.0 if x > 0 else 90.0)
        o = self.objs.get('SM_NS_VaultLights')
        if o is not None:
            for i, (x, y, ax) in enumerate(VAULTS):
                inst = o if i == 0 else self.dup(o)
                self.place(inst, x, y, 0.0, 0.0 if ax == 'x' else 90.0)
        for name in GROUND:
            g = self.objs.get(name)
            if g is not None:
                g.location = (0.0, 0.0, 0.0)
                g.rotation_euler = (0.0, 0.0, 0.0)
                g.hide_render = False

    # ---- light
    def lights(self):
        self.sunlight = []
        d = bpy.data.lights.new('rv_sun', 'SUN')
        d.energy = 1.6
        d.angle = math.radians(18.0)
        d.color = (0.95, 0.96, 1.0)
        o = core.link(bpy.data.objects.new('rv_sun', d))
        o.rotation_euler = Vector((0.4, -0.5, 0.75)).normalized().to_track_quat('Z', 'Y').to_euler()
        self.temp.append(o)
        self.day.append(o)
        for (X, Y, F) in getattr(self, 'lamp_spots', []):
            reach = 1.5 if X < 10 else -1.5
            p = (X + reach, Y, 7.85)
            d = bpy.data.lights.new('rv_hps', 'SPOT')
            d.energy = 900.0
            d.color = (1.0, 0.70, 0.36)
            d.spot_size = math.radians(150.0)
            d.spot_blend = 0.6
            d.shadow_soft_size = 0.15
            o = core.link(bpy.data.objects.new('rv_hps', d))
            o.location = _bl(p)
            o.rotation_euler = (0.0, 0.0, 0.0)
            self.temp.append(o)
            self.night.append(o)
        # storefront spills (the Lucky Penny, the corner) and Market's LEDs
        for (p, tgt, size, pw, colr) in (((1.0, 43.0, 3.0), (4.0, 43.0, 0.0), (10.0, 2.5), 900.0, (0.86, 0.92, 1.0)),
                                         ((-7.0, 50.5, 3.0), (-7.0, 53.0, 0.0), (12.0, 2.5), 700.0, (0.86, 0.92, 1.0)),
                                         ((-8.0, 68.5, 3.0), (-8.0, 66.0, 0.0), (10.0, 2.5), 500.0, (1.0, 0.35, 0.3)),
                                         ((-30.0, 50.5, 3.0), (-30.0, 53.0, 0.0), (10.0, 2.5), 400.0, (1.0, 0.85, 0.65))):
            d = bpy.data.lights.new('rv_spill', 'AREA')
            d.shape = 'RECTANGLE'
            d.size, d.size_y = size
            d.energy = pw
            d.color = colr
            o = core.link(bpy.data.objects.new('rv_spill', d))
            o.location = _bl(p)
            o.rotation_euler = (_bl(tgt) - _bl(p)).to_track_quat('-Z', 'Y').to_euler()
            self.temp.append(o)
            self.night.append(o)
        for (X, Y) in ((-29.0, 53.4), (-56.5, 53.4), (-14.5, 65.6), (-42.5, 65.6), (-70.5, 65.6), (-84.0, 53.4)):
            d = bpy.data.lights.new('rv_led', 'SPOT')
            d.energy = 1100.0
            d.color = (1.0, 0.93, 0.85)
            d.spot_size = math.radians(130.0)
            d.spot_blend = 0.4
            o = core.link(bpy.data.objects.new('rv_led', d))
            o.location = _bl((X, Y + (1.6 if Y < 60 else -1.6), 8.8))
            self.temp.append(o)
            self.night.append(o)

    def world(self, night):
        w = self.sc.world
        w.use_nodes = True
        nt = w.node_tree
        nt.nodes.clear()
        out = nt.nodes.new('ShaderNodeOutputWorld')
        bg = nt.nodes.new('ShaderNodeBackground')
        tc = nt.nodes.new('ShaderNodeTexCoord')
        sep = nt.nodes.new('ShaderNodeSeparateXYZ')
        nt.links.new(tc.outputs['Generated'], sep.inputs['Vector'])
        mr = nt.nodes.new('ShaderNodeMapRange')
        mr.inputs['From Min'].default_value = -0.05
        mr.inputs['From Max'].default_value = 0.6
        nt.links.new(sep.outputs['Z'], mr.inputs['Value'])
        mx = nt.nodes.new('ShaderNodeMix')
        mx.data_type = 'RGBA'
        nt.links.new(mr.outputs['Result'], mx.inputs[0])
        if night:
            mx.inputs[6].default_value = H(0x3a2a22)
            mx.inputs[7].default_value = H(0x0a0c12)
            bg.inputs['Strength'].default_value = 0.08
        else:
            mx.inputs[6].default_value = H(0xb9bec4)
            mx.inputs[7].default_value = H(0x8c949e)
            bg.inputs['Strength'].default_value = 1.1
        nt.links.new(mx.outputs[2], bg.inputs['Color'])
        nt.links.new(bg.outputs['Background'], out.inputs['Surface'])

    def set_time(self, night):
        self.world(night)
        for o in self.day:
            o.hide_render = night
        for o in self.night:
            o.hide_render = not night
        for e in self.glows:
            e.outputs[0].default_value = 3.0 if night else 0.0
        for sock, s0 in getattr(self, 'lit', []):
            sock.default_value = s0 * (1.0 if night else 0.15)
        if hasattr(self, 'store_glow'):
            self.store_glow.set('Emission Strength', 4.0 if night else 0.6)
        self.set_wet(0.95 if night else 0.35, 0.85 if night else 0.25)
        self.sc.view_settings.view_transform = 'AgX'
        self.sc.view_settings.exposure = 0.4 if night else 0.0

    def shot(self, name, cam_p, look_p, fov, w=1280, h=720, samples=64):
        cam = self.sc.camera
        if cam is None or cam.name != 'rv_cam':
            cd = bpy.data.cameras.new('rv_cam')
            cam = core.link(bpy.data.objects.new('rv_cam', cd))
            self.temp.append(cam)
            self.sc.camera = cam
        cam.data.sensor_fit = 'HORIZONTAL'
        cam.data.angle = math.radians(fov)
        cam.data.clip_start = 0.03
        cam.data.clip_end = 2000.0
        cam.location = _bl(cam_p)
        cam.rotation_euler = (_bl(look_p) - _bl(cam_p)).to_track_quat('-Z', 'Y').to_euler()
        sc = self.sc
        sc.render.resolution_x = w
        sc.render.resolution_y = h
        sc.render.resolution_percentage = 100
        sc.cycles.samples = int(os.environ.get('NS_REVIEW_SAMPLES', samples))
        sc.cycles.use_denoising = True
        sc.cycles.adaptive_threshold = 0.03
        sc.render.film_transparent = False
        path = os.path.join(self.dir, f'{name}.png')
        sc.render.filepath = path
        sc.render.image_settings.file_format = 'PNG'
        sc.render.image_settings.color_mode = 'RGB'
        bpy.ops.render.render(write_still=True)
        return path

    def cleanup(self):
        for o in self.temp:
            try:
                bpy.data.objects.remove(o)
            except ReferenceError:
                pass
        self.temp = []
        for name, (loc, rot, scl, hid) in self.saved.items():
            o = self.objs[name]
            o.location, o.rotation_euler, o.scale, o.hide_render = loc, rot, scl, hid
        self.sc.view_settings.exposure = 0.0
        w = self.sc.world
        if w.node_tree is not None:
            nt = w.node_tree
            nt.nodes.clear()
            out = nt.nodes.new('ShaderNodeOutputWorld')
            bg = nt.nodes.new('ShaderNodeBackground')
            bg.inputs['Color'].default_value = (0.006, 0.0065, 0.008, 1.0)
            nt.links.new(bg.outputs['Background'], out.inputs['Surface'])


def core_shade(h, f):
    r, g, b = (h >> 16) & 255, (h >> 8) & 255, h & 255
    r, g, b = (max(0, min(255, int(c * f))) for c in (r, g, b))
    return (r << 16) | (g << 8) | b


# name, camera (stage m), look-at, fov
SITE_SHOTS = [
    ('door1812', (1.8, -0.5, 1.6), (12.0, 40.0, 0.8), 70.0),
    ('corner', (2.2, 46.5, 1.65), (2.0, 62.0, 0.0), 72.0),
    ('market', (1.0, 57.0, 1.7), (-40.0, 58.0, 0.0), 65.0),
    ('bay', (-12.0, 51.8, 1.6), (-26.0, 55.5, -0.1), 62.0),
    ('ramp', (5.6, 49.4, 1.2), (3.2, 51.6, -0.1), 58.0),
    ('inlet', (6.5, 20.4, 0.9), (4.4, 22.0, -0.15), 55.0),
    ('hollis', (10.0, -52.0, 1.7), (10.0, -78.0, 1.0), 72.0),
    ('crane', (34.0, 30.0, 22.0), (2.0, 58.0, 0.0), 60.0),
    ('pit', (2.0, 13.6, 1.3), (3.3, 15.0, -0.05), 50.0),
]


def site_review(objs):
    which = _which_reviews()
    if 'tex' in which:
        try:
            texture_sheet()
            decal_sheet()
        except Exception:
            traceback.print_exc()
    if 'site' not in which or not any(n in {o.name for o in objs} for n in GROUND):
        return
    R = Review(objs)
    try:
        R.place_castings()
        R.context()
        R.ground_preview()
        R.lights()
        shots = {s[0]: s for s in SITE_SHOTS}
        day = ['door1812', 'corner', 'market', 'bay', 'ramp', 'inlet', 'hollis', 'crane', 'pit']
        night = ['door1812', 'corner', 'market', 'bay', 'ramp', 'inlet', 'hollis', 'crane', 'pit']
        for label, names, nite in (('day', day, False), ('night', night, True)):
            R.set_time(nite)
            paths = []
            for nm in names:
                _, cp, lp, fov = shots[nm]
                paths.append(R.shot(f'{label}_{nm}', cp, lp, fov, 1120, 630, 40))
            compose_grid(f'ns_ground_site_{label}', paths, 3)
    except Exception:
        traceback.print_exc()
    finally:
        R.cleanup()


PROP_SHOTS = [
    ('manhole', 'SM_NS_Manhole', (-0.9, -0.5, 0.9), (0.0, 0.0, 0.0), 45),
    ('manhole_top', 'SM_NS_Manhole', (0.0, -0.01, 1.6), (0.0, 0.0, 0.0), 38),
    ('vent', 'SM_NS_ManholeVent', (-0.9, 0.5, 0.9), (0.0, 0.0, 0.0), 45),
    ('inlet', 'SM_NS_CurbInlet', (-1.3, -0.8, 0.6), (0.0, 0.0, -0.1), 45),
    ('inlet_face', 'SM_NS_CurbInlet', (-1.2, 0.0, -0.05), (0.0, 0.0, -0.08), 40),
    ('grate', 'SM_NS_TreeGrate', (-1.2, -0.9, 1.2), (0.0, 0.0, 0.0), 45),
    ('vault', 'SM_NS_VaultLights', (-1.6, -1.4, 1.3), (0.0, 0.0, 0.0), 50),
    ('vault_close', 'SM_NS_VaultLights', (-0.4, -0.2, 0.35), (0.0, 0.1, 0.0), 45),
]


def props_review(objs):
    by = {o.name: o for o in objs}
    if not any(n in by for n in CASTINGS):
        return
    sc = bpy.context.scene
    saved = {o.name: (o.hide_render, o.location.copy(), o.rotation_euler.copy()) for o in objs}
    temp = []
    R = Review(objs)
    try:
        floor = core.Mat('rv_pfloor')
        floor.set('Base Color', H(0x55534f))
        floor.set('Roughness', 0.8)
        fl = box(-6, 6, -6, 6, -0.4, -0.0025, floor)
        temp.append(fl)
        d = bpy.data.lights.new('rv_psun', 'SUN')
        d.energy = 3.0
        d.angle = math.radians(3.0)
        sun = core.link(bpy.data.objects.new('rv_psun', d))
        sun.rotation_euler = Vector((-0.5, -0.4, 0.75)).normalized().to_track_quat('Z', 'Y').to_euler()
        temp.append(sun)
        R.world(False)
        sc.view_settings.exposure = -0.3
        paths = []
        for label, name, cam, tgt, fov in PROP_SHOTS:
            if name not in by:
                continue
            for o in objs:
                o.hide_render = o.name != name
                if o.name == name:
                    o.location = (0, 0, 0)
                    o.rotation_euler = (0, 0, 0)
            fl.hide_render = name in ('SM_NS_CurbInlet',)
            c = Vector(cam)
            t = Vector(tgt)
            # cameras given in Blender coordinates here
            cd = bpy.data.cameras.new('rv_pcam')
            co = core.link(bpy.data.objects.new('rv_pcam', cd))
            temp.append(co)
            cd.angle = math.radians(fov)
            cd.clip_start = 0.01
            co.location = c
            co.rotation_euler = (t - c).to_track_quat('-Z', 'Y').to_euler()
            sc.camera = co
            sc.render.resolution_x = 900
            sc.render.resolution_y = 900
            sc.cycles.samples = 48
            path = os.path.join(R.dir, f'p_{label}.png')
            sc.render.filepath = path
            bpy.ops.render.render(write_still=True)
            paths.append(path)
        if paths:
            compose_grid('ns_ground_props', paths, 4)
    except Exception:
        traceback.print_exc()
    finally:
        for o in temp:
            try:
                bpy.data.objects.remove(o)
            except ReferenceError:
                pass
        for n, (h, loc, rot) in saved.items():
            by[n].hide_render, by[n].location, by[n].rotation_euler = h, loc, rot
        R.cleanup()


def pose_for_review(objs):
    """Renders the extra sheets (art/review/ns_ground_site_day/night, _props, _textures), then lines the meshes up for the
    standard sheet (the castings in a row, the ground shrunk behind)."""
    global REVIEW_VIEWS
    t0 = time.time()
    for o in objs:
        o.hide_render = False
    site_review(objs)
    if 'props' in _which_reviews():
        try:
            props_review(objs)
        except Exception:
            traceback.print_exc()
    log(f'review renders in {time.time() - t0:.0f}s')
    by = {o.name: o for o in objs}
    x = 0.0
    y = -3.0
    for n in CASTINGS:
        if n in by:
            o = by[n]
            o.location = (0.0, y, 0.0)
            o.rotation_euler = (0, 0, 0)
            o.scale = (1, 1, 1)
            y += 1.6
    for n in GROUND:
        if n in by:
            o = by[n]
            o.scale = (0.01, 0.01, 0.01)
            o.location = (3.0, 0.0, 0.0)
            o.rotation_euler = (0, 0, 0)
    REVIEW_VIEWS = [('front', -60, 30, 1.2), ('top', 0, 70, 1.0), ('detail', -40, 25, 0.5)]
