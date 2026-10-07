"""SM_NS_Prop_*: the hand props the Northside people hold (spec world_realism_spec.md section 4.5): the barber's tools,
the waitress's carafe, plates and pad, the hero prop (the short stack the game is named for), and the things everyone
carries. Built with the shop kit in ns_barber.py (and the diner's food finishes in ns_diner.py).

FRAME (spec 4.5): PROP, origin at the GRIP (where the hand closes on it), +X along the tool (from the grip toward its
working end), Z up as the tool is held naturally; meters (centimeters in Unreal). Each is one baked slot (a few carry a
print slot for their labels); none has Nanite (spec 3.8). The game attaches them to the hand bones (spec 6.6); the
grip point is the palm's middle unless a prop says otherwise.

THE MESHES (length along X; P0 unless marked):
- SM_NS_Prop_Comb (0.18): a black barber's comb, coarse and fine teeth; the grip 4 cm from its back end; teeth down (-Z).
- SM_NS_Prop_Scissors (0.195): barber's shears, closed; the grip between the finger rings (the thumb ring at -Y, the
  finger ring and its tang at +Y); the blades along +X; the pivot screw's axis is Z.
- SM_NS_Prop_Clippers (0.17 + cord): cordless-shaped body with a cord stub coiling from its tail (-X); the blades at
  +X, the lever on top (+Z).
- SM_NS_Prop_Razor (0.21 open): a straight razor opened for use: the tang at the grip, the blade along +X (its edge
  down, -Z), the scales swung back and up behind the hand.
- SM_NS_Prop_NeckDuster (0.21): a turned wood handle, a chrome ferrule, the flat fan of soft bristles at +X (9 cm wide
  along Y, 3 cm thick along Z).
- SM_NS_Prop_SprayBottle (0.22 tall): the grip on the trigger housing; the nozzle toward +X, the bottle below (-Z).
- SM_NS_Prop_Carafe: a coffee decanter, half full; the grip on its handle, the spout toward +X.
- SM_NS_Prop_Plate (0.27): a heavy diner plate with a maroon band; the grip on its rim, the plate toward +X.
- SM_NS_Prop_PancakeStack: THE hero prop: three golden pancakes on the diner plate, a melting pat of butter, syrup
  pooled on top and running down the sides onto the plate. Origin and grip as SM_NS_Prop_Plate.
- SM_NS_Prop_Mug: a diner mug of coffee; the grip in its handle, the mug toward +X.
- SM_NS_Prop_Notepad (0.09 x 0.17): a guest-check pad, top bound; held flat in the palm (Z its face), +X up the pad.
- SM_NS_Prop_Pen (0.15): a click ballpoint; the grip near the tip, the tip at +X.
- SM_NS_Prop_Fork (0.225): a diner fork; the grip on the handle, the tines at +X, the bowl up.
- SM_NS_Prop_Rag: a white cotton bar towel, 40 x 30 cm, gripped by its middle: the grip's patch is gathered at the
  origin (inside the fist) and the rest hangs from it in soft folds (cloth simulated), down -Z 0.22 m (18 x 16 cm across).
- SM_NS_Prop_Broom (P1, 1.61 long): an angle broom; the grip 40 cm down from the handle's top end (the handle runs
  x -0.46..0.80), the red head and its bristles at +X (the bristles' slanted ends at x 1.06..1.13, the head 0.30 wide
  along Y, the bristles' faces toward +-Z). Hold it with +X down toward the floor to sweep.
- SM_NS_Prop_Newspaper: the Northside Herald folded in quarters; the grip on the fold, the paper toward +X.
- SM_NS_Prop_CoffeeCup: a paper to-go cup, lid and sleeve; the grip round the sleeve, the cup's axis along Z.
- SM_NS_Prop_Phone (P1, 0.15): a phone, the screen up (+Z), the top toward +X; its screen bakes a dim lock screen.
- SM_NS_Prop_Clipboard (0.32): a clipboard of forms; held at its side edge, +X up the board, Z its face.
- SM_NS_Prop_TagGun: a price-tag gun; the grip in its handle, the needle toward +X.
- SM_NS_Prop_Hanger (0.42): a wire hanger held by its hook; the bar along Y, the hook up.
- SM_NS_Prop_Cigarette (P1, 0.084): the grip 1.5 cm from the filter's end, the ember at +X (baked glow).
- SM_NS_Prop_HandMirror (extra, for the barber-reveal shot): the grip on the handle, the round mirror at +X, its face
  (ns_mirror) toward +Z.
- SM_NS_Prop_StyleBook (extra: the barber's style book): a closed ring binder of hairstyles; held by its spine
  (along X), the cover up.

NSH_ONLY=comb,scissors,... (keys of BUILDERS) builds a subset. The review: art/review/ns_handprops.jpg (the standard
sheet, all of them in a row) and ns_handprops_detail.jpg (each one close, on a counter, in shop light).
"""
import math
import os
import random

import bmesh
import bpy  # noqa: F401
import numpy as np
from mathutils import Matrix, Vector

from artkit import core, parts
from artkit.shade import noise, ramp, scaled
from assets import ns_barber as K
from assets import ns_diner as D  # noqa: F401 (registers the food finishes: crust, syrup, cream)
from assets.ns_barber import H, Kit, bend, bx, cushion, lathe_shell, lathe_solid, render_image, tube, unbaked

LOG = 'ns_handprops'
NAMES = ['Comb', 'Scissors', 'Clippers', 'Razor', 'NeckDuster', 'SprayBottle', 'Carafe', 'Plate', 'PancakeStack', 'Mug', 'Notepad', 'Pen', 'Fork',
         'Rag', 'Broom', 'Newspaper', 'CoffeeCup', 'Phone', 'Clipboard', 'TagGun', 'Hanger', 'Cigarette', 'HandMirror', 'StyleBook']
_ONLY = [s.strip().lower() for s in os.environ.get('NSH_ONLY', '').split(',') if s.strip()]
WHICH = [n for n in NAMES if not _ONLY or n.lower() in _ONLY]
MESHES = [f'SM_NS_Prop_{n}' for n in WHICH]
DOUBLE_SIDED = False
TEXTURE_SIZE = 512
TEXTURE_SIZES = {}
AO_DISTANCE = 0.01
REVIEW_SCREEN_LIGHT = False
REVIEW_VIEWS = None
ALL = []


def kit(short, slots=('body',), px=512, **kw):
    name = f'SM_NS_Prop_{short}'
    pre = f'nsh{short.lower()[:9]}'
    TEXTURE_SIZES[f'{pre}_body'] = px
    return Kit(pre, name, slots, sizes=TEXTURE_SIZES, anchor=(0.0, 0.0, 0.0), log=LOG, dust=kw.pop('dust', 0.0), low=kw.pop('low', 0.0),
               cavity=kw.pop('cavity', 0.004), **kw)


def rot_x(d):
    return Matrix.Rotation(math.radians(d), 4, 'X')


def rot_y(d):
    return Matrix.Rotation(math.radians(d), 4, 'Y')


def rot_z(d):
    return Matrix.Rotation(math.radians(d), 4, 'Z')


def T(x, y, z):
    return Matrix.Translation((x, y, z))


def place(o, M):
    o.data.transform(M)
    return o


@K.finish_kind('bristle')
def f_bristle(c):
    """Brush and broom bristles: fine strands running along the part's X, each a slightly different tone, a dull sheen."""
    m = c.m
    strands = noise(m, scaled(m, c.sep, 3.0, 900.0, 900.0), detail=2.0)
    fine = noise(m, scaled(m, c.sep, 6.0, 2600.0, 2600.0), detail=1.0)
    col = m.mix(ramp(m, strands, 0.3, 0.7), K.darken(m, c.col, 0.55), K.lighten(m, c.col, 0.25))
    col = m.mix(mul_(m, ramp(m, fine, 0.45, 0.6), 0.4), col, K.darken(m, c.col, 0.7))
    return col, K.add(m, 0.42, mul_(m, strands, 0.12)), 0.0, K.add(m, mul_(m, strands, 0.35), mul_(m, fine, 0.2))


def mul_(m, a, b):
    return m.math('MULTIPLY', a, b)


# ================================================================== the barber's

def comb():
    k = kit('Comb', px=512)
    L0, L1 = -0.04, 0.14
    k.add(bx('spine', L0, L1, -0.0014, 0.0014, -0.004, 0.006, 0.0012), 'plastic', 0x101010)
    n = 30
    for i in range(n):
        x = L0 + 0.006 + (L1 - L0 - 0.012) * i / (n - 1)
        fine = i > n // 2
        w = 0.0009 if fine else 0.0013
        depth = 0.022 if fine else 0.025
        k.add(bx('tooth', x - w, x + w, -0.0012, 0.0012, -0.004 - depth, -0.003, 0.0004), 'plastic', 0x101010)
    return [k.finish(30.0)]


def scissors():
    k = kit('Scissors', px=512)
    C = lambda o: k.add(o, 'stainless', 0xffffff)
    # the blades: long, tapering, slightly crossed at the pivot (closed)
    for s in (-1, 1):
        prof = [(0.020, 0.0), (0.040, s * 0.0045), (0.095, s * 0.0055), (0.150, s * 0.0025), (0.165, 0.0), (0.150, -s * 0.0004), (0.040, -s * 0.0010)]
        z0, z1 = (0.0002, 0.0020) if s > 0 else (-0.0018, 0.0)
        o = parts.panel('blade', list(reversed(prof)) if s > 0 else prof, z0, z1)
        core.orient_normals(o)
        C(o)
    C(parts.rod('pivot', (0.03, 0.0, -0.003), (0.03, 0.0, 0.004), 0.0035, 16))
    # the shanks and the rings (the finger ring with its tang rest), black rubber inserts
    for s, rr in ((-1, 0.012), (1, 0.011)):
        cy = s * 0.016
        ring = [(rr * math.cos(a), cy + rr * 1.1 * math.sin(a), 0.0) for a in np.linspace(0, 2 * math.pi, 28, endpoint=False)]
        C(tube('ring', ring, 0.0028, 8, closed=True))
        k.add(tube('insert', [(p[0], cy + (p[1] - cy) * 0.86, 0.0) for p in ring], 0.0016, 6, closed=True), 'rubber', 0x111111)
        C(tube('shank', [(rr * 0.8, cy - s * 0.003, 0.0), (0.012, s * 0.006, 0.0), (0.03, 0.0, 0.0)], 0.0028, 8))
    C(tube('tang', bend([(-0.004, 0.028, 0.0), (-0.012, 0.042, 0.0), (-0.03, 0.044, 0.0)], 0.01, 4), 0.0022, 8))
    return [k.finish(30.0)]


def clippers():
    k = kit('Clippers', px=1024)
    body = lathe_solid('body', [(0.0, -0.075), (0.016, -0.072), (0.022, -0.05), (0.024, 0.0), (0.024, 0.05), (0.022, 0.07), (0.0, 0.075)], None, 32)
    body.data.transform(rot_y(90) @ Matrix.Scale(0.78, 4, (0, 1, 0)))
    k.add(body, 'plastic', 0x16161a)
    k.add(bx('grip_l', -0.05, 0.03, -0.0205, -0.0185, -0.012, 0.012, 0.004), 'rubber', 0x2a2a2a)
    k.add(bx('grip_r', -0.05, 0.03, 0.0185, 0.0205, -0.012, 0.012, 0.004), 'rubber', 0x2a2a2a)
    k.add(bx('blade_base', 0.072, 0.090, -0.024, 0.024, -0.012, 0.010, 0.003), 'chrome')
    k.add(bx('blade_teeth', 0.088, 0.096, -0.023, 0.023, -0.010, 0.002, 0.0008), 'chrome')
    for i in range(22):
        y = -0.021 + i * 0.002
        k.add(bx('tooth', 0.094, 0.099, y - 0.0005, y + 0.0005, -0.010, -0.002), 'chrome')
    k.add(bx('lever', 0.02, 0.05, -0.006, 0.006, 0.016, 0.022, 0.003), 'chrome')
    k.add(parts.rod('switch', (-0.01, 0.0, 0.018), (-0.01, 0.0, 0.024), 0.005, 12), 'plastic', 0x7a7a7e)
    k.add(parts.rod('tail', (-0.075, 0.0, 0.0), (-0.09, 0.0, 0.0), 0.006, 12), 'rubber', 0x111111)
    pts = []
    for i in range(120):
        t = i / 119
        ang = t * 2 * math.pi * 9
        pts.append((-0.09 - t * 0.14, 0.008 * math.cos(ang), -0.03 * t * t + 0.008 * math.sin(ang)))
    k.add(tube('cord', pts, 0.0022, 6), 'rubber', 0x151515)
    return [k.finish(30.0)]


def razor():
    """An open straight razor: a hollow-ground blade (spine 4 mm to a fine edge) on its tang, the black scales swung back."""
    k = kit('Razor', px=512)
    S = lambda o: k.add(o, 'stainless', 0xffffff)
    S(core.prism_x('blade', [(-0.0021, 0.0095), (0.0021, 0.0095), (0.0024, 0.006), (0.0004, -0.009), (0.0, -0.0105), (-0.0004, -0.009), (-0.0024, 0.006)],
                   0.0, 0.074))
    S(core.prism_x('nose', [(-0.0021, 0.0095), (0.0021, 0.0095), (0.0005, -0.0095), (-0.0005, -0.0095)], 0.074, 0.078))
    S(bx('shank', -0.022, 0.002, -0.0017, 0.0017, -0.002, 0.009, 0.0015))
    S(tube('tang', bend([(-0.002, 0.0, 0.0085), (-0.012, 0.0, 0.0115), (-0.024, 0.0, 0.0105), (-0.030, 0.0, 0.006)], 0.006, 4), 0.0022, 8))
    k.add(parts.rod('pivot', (-0.019, -0.0055, 0.002), (-0.019, 0.0055, 0.002), 0.0024, 12), 'brass')
    for sgn in (-1, 1):
        sc = bx('scale', -0.12, 0.004, -0.0011, 0.0011, -0.0065, 0.0065, 0.0028)
        sc.data.transform(T(-0.019, sgn * 0.0040, 0.002) @ rot_y(20) @ T(-0.002, 0.0, 0.0))
        k.add(sc, 'gloss', 0x101010)
    tx, tz = -0.019 - 0.115 * math.cos(math.radians(20)), 0.002 + 0.115 * math.sin(math.radians(20))
    k.add(parts.rod('tail_pin', (tx, -0.0055, tz), (tx, 0.0055, tz), 0.0018, 10), 'brass')
    return [k.finish(30.0)]


def neck_duster():
    """A barber's neck duster: a turned wood handle, a chrome ferrule, and a flat fan of soft cream bristles combed into
    fine streaks, its end feathered."""
    k = kit('NeckDuster', px=512)
    k.add(lathe_solid('handle', [(0.0, -0.06), (0.012, -0.06), (0.014, -0.05), (0.013, 0.0), (0.016, 0.03), (0.018, 0.045), (0.0, 0.046)], rot_y(90), 24), 'wood',
          0x3a2010, aux=0)
    k.add(lathe_solid('ferrule', [(0.0, 0.045), (0.019, 0.045), (0.0195, 0.05), (0.019, 0.06), (0.0, 0.06)], rot_y(90), 24), 'chrome')
    # the fan: rings along +X from the ferrule, an ellipse widening to 9 cm and 3 cm thick, rounded off at the tip
    rnd = random.Random(31)
    rings = []
    n_r, n_a = 14, 28
    for j in range(n_r + 1):
        t = j / n_r
        x = 0.056 + 0.092 * t
        wy = 0.017 + 0.030 * math.sin(min(1.0, t * 1.35) * math.pi / 2)
        wz = 0.014 + 0.004 * t
        end = math.sqrt(max(0.0, 1.0 - max(0.0, (t - 0.72) / 0.28) ** 2))
        ring = []
        for i in range(n_a):
            a = 2 * math.pi * i / n_a
            streak = 1.0 + 0.05 * math.sin(a * 23 + j * 0.4) * min(1.0, t * 3)
            ring.append((x + (0.004 * math.sin(a * 17 + j) * t if t > 0.8 else 0.0), wy * end * streak * math.cos(a) + 0.0006, wz * end * streak * math.sin(a)))
        rings.append(ring)
    head = core.loft('head', rings)
    core.subdivide(head, 1)
    head.data.shade_smooth()
    k.add(head, 'bristle', 0xeee4cf, soft=True)
    return [k.finish(30.0)]


def spray_bottle():
    k = kit('SprayBottle', px=512)
    bm = T(-0.01, 0.0, -0.18)
    k.add(lathe_solid('bottle', [(0.0, 0.0), (0.034, 0.0), (0.037, 0.008), (0.037, 0.13), (0.03, 0.155), (0.013, 0.17), (0.013, 0.18), (0.0, 0.18)], bm, 32),
          'gloss', 0x7aa8c0)
    k.add(lathe_solid('water', [(0.0, 0.006), (0.033, 0.006), (0.033, 0.09), (0.0, 0.09)], bm, 24), 'liquid', 0x4a7898)
    k.add(lathe_solid('collar', [(0.0, 0.0), (0.017, 0.0), (0.017, 0.016), (0.0, 0.016)], T(-0.01, 0.0, -0.004), 24), 'plastic', 0x1a1a1a)
    k.add(bx('head', -0.03, 0.045, -0.012, 0.012, 0.008, 0.035, 0.008), 'plastic', 0x1a1a1a)
    k.add(parts.rod('nozzle', (0.045, 0.0, 0.026), (0.058, 0.0, 0.026), 0.006, 12), 'plastic', 0xd8261e)
    k.add(bx('trigger', 0.012, 0.024, -0.007, 0.007, -0.04, 0.012, 0.004), 'plastic', 0x1a1a1a)
    k.add(tube('dip', [(-0.01, 0.0, -0.004), (-0.01, 0.0, -0.17)], 0.0016, 6), 'plastic', 0xd8d8d0)
    return [k.finish(30.0)]


# ================================================================== the diner's

def carafe():
    k = kit('Carafe', px=1024)
    cm = T(0.11, 0.0, -0.08)
    k.add(lathe_solid('glass', [(0.0, 0.0), (0.075, 0.0), (0.085, 0.02), (0.085, 0.07), (0.06, 0.14), (0.045, 0.17), (0.0, 0.17)], cm, 40), 'glassdark', 0x3a2414)
    k.add(lathe_solid('collar', [(0.0, 0.15), (0.05, 0.15), (0.058, 0.17), (0.058, 0.21), (0.03, 0.22), (0.0, 0.22)], cm, 32), 'plastic', 0x111111)
    k.add(bx('spout', 0.155, 0.185, -0.012, 0.012, 0.12, 0.135, 0.005), 'plastic', 0x111111)
    k.add(tube('handle', core.catmull_rom([(0.065, 0.0, 0.12), (0.02, 0.0, 0.10), (0.0, 0.0, 0.03), (0.005, 0.0, -0.045), (0.04, 0.0, -0.07)], 6), 0.011, 12),
          'plastic', 0x111111)
    return [k.finish(30.0)]


def plate_parts(k, cx):
    """A heavy diner plate (Ø 0.27) centered at (cx, 0), its maroon band."""
    M = T(cx, 0.0, 0.0)
    k.add(lathe_solid('plate', [(0.0, 0.0), (0.075, 0.0), (0.08, 0.004), (0.095, 0.010), (0.128, 0.020), (0.135, 0.024), (0.133, 0.027), (0.12, 0.025),
                                (0.093, 0.014), (0.0, 0.012)], M, 40), 'ceramic', 0xf4f1ea)
    # the maroon band: a glaze stripe lying on the plate's well slope, 0.3 mm proud
    k.add(lathe_shell('band', [(0.112, 0.02200), (0.117, 0.02405), (0.123, 0.02576)], M, 40, toward=(0, 0, 1)), 'ceramic', 0x7a1c22)


def plate():
    k = kit('Plate', px=512)
    plate_parts(k, 0.13)
    return [k.finish(30.0)]


def pancake(name, cx, cy, z, r, h, seed):
    """A pancake: a soft-edged disc, its top domed and dimpled with bubbles, its rim a little irregular."""
    rnd = random.Random(seed)
    prof = [(0.0, 0.0), (r * 0.9, 0.0), (r * 0.97, h * 0.08), (r * 0.995, h * 0.3), (r, h * 0.55), (r * 0.985, h * 0.8), (r * 0.95, h * 0.95),
            (r * 0.82, h * 1.02), (r * 0.55, h * 1.05), (r * 0.25, h * 1.06), (0.0, h * 1.06)]
    o = core.lathe(name, prof, segments=32)
    weld(o)
    core.orient_normals(o)
    phase = [rnd.uniform(0, 6.28) for _ in range(4)]
    for v in o.data.vertices:
        p = v.co
        rr = math.hypot(p.x, p.y)
        a = math.atan2(p.y, p.x)
        wob = 1.0 + 0.025 * math.sin(3 * a + phase[0]) + 0.015 * math.sin(5 * a + phase[1])
        p.x *= wob
        p.y *= wob
        if p.z > h * 0.9:
            p.z -= 0.0010 * (0.5 + 0.5 * math.sin(a * 7 + rr * 140 + phase[2])) * min(1.0, rr / (r * 0.3))
    o.data.transform(T(cx, cy, z) @ rot_z(rnd.uniform(0, 360)))
    o.data.shade_smooth()
    return o


def weld(obj, dist=1e-6):
    K.weld(obj, dist)


def stack_bvh(objs):
    """A BVH of the pancakes (their mesh data is already in place), for laying the syrup on them."""
    from mathutils.bvhtree import BVHTree
    verts, polys = [], []
    for o in objs:
        base = len(verts)
        verts += [v.co.copy() for v in o.data.vertices]
        polys += [[base + i for i in p.vertices] for p in o.data.polygons]
    return BVHTree.FromPolygons(verts, polys)


def drip(bvh, cx, cy, a, z_top, z_bot, w, seed):
    """A syrup run from the pool over the stack's rim at angle a and down its side, laid on the pancakes' real surface
    (ray cast): a flat lens of syrup thickest down its middle, narrowing as it runs and beading at its foot; it
    bridges the grooves between the cakes as syrup does."""
    rnd = random.Random(seed)
    up = Vector((0.0, 0.0, 1.0))
    C = Vector((cx, cy, 0.0))
    # stations along the run: (kind, value, half-width, thickness): 'top' = a radius on the top face, 'side' = a height
    path = [('top', 0.060, w * 1.2, 0.0011), ('top', 0.068, w * 1.15, 0.0015), ('rim', 0.0, w * 1.05, 0.0019)]
    n = 6
    for i in range(1, n + 1):
        t = i / n
        bead = max(0.0, (t - 0.8) / 0.2)
        path.append(('side', z_top - 0.004 + (z_bot - (z_top - 0.004)) * t, w * (0.95 - 0.4 * t) + w * 0.5 * bead, 0.0019 + 0.0017 * bead))
    rings = []
    env = 0.0
    for i, (kind, val, hw, th) in enumerate(path):
        wig = 0.02 * math.sin(i * 1.3 + seed) + rnd.uniform(-0.004, 0.004)
        o_ = Vector((math.cos(a + wig), math.sin(a + wig), 0.0))
        s_ = Vector((-math.sin(a + wig), math.cos(a + wig), 0.0))
        if kind == 'top':
            hit = bvh.ray_cast(C + o_ * val + up * 0.2, -up)
            P, nrm = (hit[0], hit[1]) if hit[0] is not None else (C + o_ * val + up * z_top, up)
        else:
            z = z_top - 0.0015 if kind == 'rim' else val
            hit = bvh.ray_cast(C + o_ * 0.2 + up * z, -o_)
            if hit[0] is not None:
                r_hit = (hit[0] - C - up * z).length
                env = max(env, r_hit) if kind == 'side' else r_hit
            r_use = max(env, (hit[0] - C - up * z).length if hit[0] is not None else env)
            P = C + o_ * r_use + up * z
            nrm = (o_ * 0.85 + up * 0.15).normalized() if kind == 'rim' else o_
        P = Vector(P) - Vector(nrm) * 0.0004
        ring = []
        for q in np.linspace(0, 2 * math.pi, 8, endpoint=False):
            c, s = math.cos(q), math.sin(q)
            ring.append(tuple(P + s_ * hw * c + Vector(nrm) * th * max(s, -0.15)))
        rings.append(ring)
    o = core.loft('drip', rings)
    o.data.shade_smooth()
    return o


def pancake_stack():
    """The hero prop: three pancakes on the diner plate, butter melting, syrup pooled on top and running down."""
    k = kit('PancakeStack', px=2048)
    cx = 0.13
    plate_parts(k, cx)
    heights = [0.0135, 0.0145, 0.0140]
    radii = [0.077, 0.075, 0.073]
    z = 0.012
    tops = []
    cakes = []
    for i, (r, h) in enumerate(zip(radii, heights)):
        dx, dy = random.Random(i).uniform(-0.004, 0.004), random.Random(i + 9).uniform(-0.004, 0.004)
        cakes.append(k.add(pancake(f'pancake{i}', cx + dx, dy, z, r, h, 30 + i), 'crust', 0x96501a, soft=True))
        z += h * 1.02
        tops.append((cx + dx, dy, z, r))
    bvh = stack_bvh(cakes)
    tx, ty, tz, tr = tops[-1]
    # syrup pooled on top: an irregular glossy sheet, thicker at the middle
    rnd = random.Random(77)
    pool = []
    n = 64
    rad = [tr * (0.78 + 0.11 * math.sin(3 * a + 1.0) + 0.05 * math.sin(7 * a + 0.4) + 0.02 * math.sin(11 * a))
           for a in np.linspace(0, 2 * math.pi, n, endpoint=False)]
    bm = bmesh.new()
    center = bm.verts.new((tx, ty, tz + 0.0034))
    # a rounded meniscus: the sheet's top stays high to just inside its edge, then rolls down onto the cake
    rings_ = []
    for f, dz in ((0.55, 0.0031), (0.86, 0.0025), (0.96, 0.0015), (1.0, 0.0004)):
        rings_.append([bm.verts.new((tx + rad[i] * f * math.cos(2 * math.pi * i / n), ty + rad[i] * f * math.sin(2 * math.pi * i / n), tz + dz))
                       for i in range(n)])
    for i in range(n):
        j = (i + 1) % n
        bm.faces.new((center, rings_[0][i], rings_[0][j]))
        for ra, rb in zip(rings_, rings_[1:]):
            bm.faces.new((ra[i], rb[i], rb[j], ra[j]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    sy = core.mesh_object('syrup_pool', bm)
    core.orient_normals(sy, (0, 0, 1))
    sy.data.shade_smooth()
    k.add(sy, 'syrup', 0x6e3010, soft=True)
    # runs down the sides where the pool reaches the edge, and a puddle on the plate
    for i, a in enumerate((0.3, 1.25, 2.6, 3.9, 5.2)):
        zb = 0.016 + rnd.uniform(0.0, 0.014)
        k.add(drip(bvh, tx, ty, a, tz, zb, 0.0040 + rnd.uniform(0, 0.0018), 11 + i), 'syrup', 0x6e3010, soft=True)
    bm = bmesh.new()
    n = 36
    pc = (cx + 0.055, -0.04)
    rp = [0.045 * (0.75 + 0.2 * math.sin(2 * a + 0.5) + 0.1 * math.sin(5 * a)) for a in np.linspace(0, 2 * math.pi, n, endpoint=False)]
    c0 = bm.verts.new((pc[0], pc[1], 0.0135))
    rv = [bm.verts.new((pc[0] + rp[i] * math.cos(2 * math.pi * i / n), pc[1] + rp[i] * math.sin(2 * math.pi * i / n), 0.0123)) for i in range(n)]
    for i in range(n):
        bm.faces.new((c0, rv[i], rv[(i + 1) % n]))
    pud = core.mesh_object('puddle', bm)
    core.orient_normals(pud, (0, 0, 1))
    pud.data.shade_smooth()
    k.add(pud, 'syrup', 0x6e3010, soft=True)
    # the butter: a pat slumping as it melts, a glossy melt pool round it
    bu = bx('butter', -0.014, 0.014, -0.012, 0.012, 0.0, 0.012, 0.0035, 1)
    core.subdivide(bu, 1)
    for v in bu.data.vertices:
        v.co.z *= 1.0 - 0.25 * (abs(v.co.x) / 0.014)
        v.co.x *= 1.0 + 0.15 * (1.0 - v.co.z / 0.012)
        v.co.y *= 1.0 + 0.15 * (1.0 - v.co.z / 0.012)
    bu.data.transform(T(tx - 0.006, ty + 0.004, tz + 0.0028) @ rot_z(18))
    bu.data.shade_smooth()
    k.add(bu, 'cream', 0xf4dc7a, soft=True)
    melt = parts.annulus('melt', 0.0, 0.026, tz + 0.0029, tz + 0.0034, 32, T(tx - 0.006, ty + 0.004, 0.0))
    k.add(melt, 'syrup', 0xe8b84a, soft=True)
    return [k.finish(30.0)]


def mug():
    k = kit('Mug', px=512)
    M = T(0.075, 0.0, -0.05)
    k.add(lathe_solid('mug', [(0.0, 0.0), (0.038, 0.0), (0.042, 0.006), (0.044, 0.095), (0.04, 0.098), (0.036, 0.012), (0.0, 0.012)], M, 40), 'ceramic',
          0xf4f1ea)
    k.add(lathe_solid('coffee', [(0.0, 0.07), (0.037, 0.07), (0.037, 0.078), (0.0, 0.078)], M, 32), 'liquid', 0x24140a)
    k.add(tube('handle', core.catmull_rom([(0.036, 0.0, 0.036), (0.012, 0.0, 0.034), (-0.003, 0.0, 0.012), (-0.003, 0.0, -0.012), (0.012, 0.0, -0.034),
                                           (0.036, 0.0, -0.036)], 6), 0.0075, 12), 'ceramic', 0xf4f1ea)
    return [k.finish(30.0)]


def draw_check(s, w, h):
    s.rect(0, 0, w, h, 0xdfe9d6, rough=0.8)
    s.rect(0, h - 14, w, 14, 0x2a5a3a, rough=0.7)
    s.mid('GUEST CHECK', w / 2, h - 7, 7, w - 10, 0xdfe9d6, 'Black', tracking=1.3)
    s.text('TABLE', 4, h - 24, 4, 0x2a5a3a, 'Bold')
    s.text('GUESTS', w / 2, h - 24, 4, 0x2a5a3a, 'Bold')
    for i in range(14):
        y = h - 32 - i * 9
        s.rect(4, y, w - 8, 0.4, 0x8aa88a)
    s.rect(w * 0.7, 4, 0.4, h - 30, 0x8aa88a)
    for i, ln in enumerate(('2 SS', '1 cof', '1 OJ')):
        s.text(ln, 6, h - 40 - i * 9, 6, 0x1a2a6a, 'Medium')
    s.mid('No 04417', w / 2, 2, 4, w - 10, 0xb3201c, 'Bold')


def notepad():
    k = kit('Notepad', slots=('body', 'print'), px=512, print_px=512)
    k.panel('check', 0.09, 0.17, draw_check, density=1.5)
    k.atlas.render()
    k.add(bx('pad', -0.02, 0.15, -0.045, 0.045, -0.006, 0.0, 0.0008), 'paper', 0xdfe9d6)
    k.add(bx('backing', -0.02, 0.15, -0.045, 0.045, -0.008, -0.006, 0.0008), 'paper', 0x7a6a50)
    k.add(bx('binding', 0.135, 0.155, -0.046, 0.046, -0.0085, 0.0015, 0.001), 'paper', 0x2a5a3a)
    k.decal('check', (0.065, 0.0, 0.0002), u=(0, -1, 0), v=(1, 0, 0), w=0.09, h=0.17, lift=0.0)
    return [k.finish(30.0)]


def pen():
    k = kit('Pen', px=512)
    M = rot_y(90)
    k.add(lathe_solid('barrel', [(0.0, -0.10), (0.0045, -0.10), (0.005, -0.06), (0.005, 0.02), (0.0042, 0.03), (0.0, 0.031)], T(-0.0, 0.0, 0.0) @ M, 20),
          'gloss', 0x1a3a8a)
    k.add(lathe_solid('grip', [(0.0, 0.0), (0.0052, 0.0), (0.0052, 0.025), (0.0, 0.025)], M, 20), 'rubber', 0x1a1a2a)
    k.add(lathe_solid('cone', [(0.0, 0.025), (0.0042, 0.025), (0.0016, 0.038), (0.0, 0.040)], M, 16), 'chrome')
    k.add(lathe_solid('button', [(0.0, -0.112), (0.003, -0.112), (0.003, -0.10), (0.0, -0.10)], M, 12), 'chrome')
    k.add(bx('clip', -0.095, -0.05, -0.0012, 0.0012, 0.0050, 0.0068, 0.0006), 'chrome')
    return [k.finish(30.0)]


def fork():
    k = kit('Fork', px=512)
    S = lambda o: k.add(o, 'stainless', 0xffffff)
    hand = [(-0.06, 0.0, 0.0), (0.0, 0.0, 0.002), (0.06, 0.0, 0.006), (0.09, 0.0, 0.004)]
    rings = []
    for i, x in enumerate(np.linspace(-0.07, 0.095, 16)):
        w = 0.010 - 0.004 * math.exp(-((x - 0.07) / 0.015) ** 2) + 0.002 * (x < -0.03)
        z = 0.004 * (x / 0.1) ** 2
        rings.append([(x, w / 2 * math.cos(a), z + 0.0012 * math.sin(a)) for a in np.linspace(0, 2 * math.pi, 10, endpoint=False)])
    S(core.loft('handle', rings))
    S(bx('bowl', 0.09, 0.115, -0.011, 0.011, 0.002, 0.0035, 0.002))
    for t_ in range(4):
        y = -0.0085 + t_ * 0.0057
        S(bx('tine', 0.112, 0.155, y - 0.0012, y + 0.0012, 0.0025 + 0.0, 0.0038, 0.0008))
    return [k.finish(30.0)]


# ================================================================== everyone's

def rag():
    """A white cotton bar towel (40 x 30 cm) gripped by its middle: the grip's patch gathered into the fist and pinned,
    the rest cloth-simulated hanging from it in soft folds (self-collision on), then given its 3 mm thickness."""
    nx, ny = 32, 24
    bpy.ops.mesh.primitive_grid_add(x_subdivisions=nx, y_subdivisions=ny, size=1.0)
    sheet = bpy.context.view_layer.objects.active
    sheet.data.transform(Matrix.Diagonal((0.40, 0.30, 1.0, 1.0)))
    rnd = random.Random(4)
    pin = []
    for v in sheet.data.vertices:
        e = (v.co.x / 0.035) ** 2 + (v.co.y / 0.05) ** 2
        if e < 1.0:
            # the fist gathers the grip's patch: pull it in toward the middle (the cloth around it buckles)
            v.co.x *= 0.45
            v.co.y *= 0.55
            v.co.z = -0.004 * e
            pin.append(v.index)
        else:
            v.co.z += rnd.uniform(-0.0015, 0.0015)
    vg = sheet.vertex_groups.new(name='pin')
    vg.add(pin, 1.0, 'REPLACE')
    mod = sheet.modifiers.new('cloth', 'CLOTH')
    st = mod.settings
    st.quality = 7
    st.mass = 0.12
    st.tension_stiffness = 12.0
    st.compression_stiffness = 12.0
    st.shear_stiffness = 4.0
    st.bending_stiffness = 0.35
    st.air_damping = 1.5
    st.vertex_group_mass = 'pin'
    cs = mod.collision_settings
    cs.use_self_collision = True
    cs.self_distance_min = 0.0035
    cs.collision_quality = 3
    frames = 80
    sc = bpy.context.scene
    mod.point_cache.frame_start = 1
    mod.point_cache.frame_end = frames
    sc.frame_start, sc.frame_end = 1, frames
    for f in range(1, frames + 1):
        sc.frame_set(f)
    dg = bpy.context.evaluated_depsgraph_get()
    me = bpy.data.meshes.new_from_object(sheet.evaluated_get(dg))
    sc.frame_set(1)
    bpy.data.objects.remove(sheet)
    o = core.link(bpy.data.objects.new('towel', me))
    K.solidify(o, 0.003, 0.0)
    o.data.shade_smooth()
    k = kit('Rag', px=512)
    k.add(o, 'cloth', 0xece8dc, soft=True)
    return [k.finish(60.0)]


def broom():
    """An angle broom (P1): a lacquered handle with a hang cap, the red head on a threaded socket, and a dense fan of
    flagged bristle tufts splaying a little and cut on the slant (the long end at +Y)."""
    k = kit('Broom', px=1024)
    R = rot_y(90)                 # lathe axis Z -> +X
    k.add(lathe_solid('handle', [(0.0, -0.40), (0.0115, -0.40), (0.0115, 0.80), (0.0, 0.80)], R, 16), 'enamel', 0x2a5a8a)
    k.add(lathe_solid('cap', [(0.0, -0.455), (0.009, -0.455), (0.0135, -0.447), (0.0135, -0.40), (0.0, -0.40)], R, 16), 'plastic', 0x1a1a1a)
    k.add(tube('hang', [(-0.468 + 0.010 * math.cos(a), 0.0, 0.010 * math.sin(a)) for a in np.linspace(0, 2 * math.pi, 14, endpoint=False)], 0.0018, 6,
               closed=True), 'chrome')
    # the threaded socket on the head
    k.add(lathe_solid('socket', [(0.0, 0.78), (0.0135, 0.78), (0.017, 0.80), (0.020, 0.85), (0.0, 0.85)], R, 16), 'plastic', 0xc42018)
    for i in range(3):
        x = 0.80 + i * 0.012
        k.add(lathe_solid('rib', [(0.0, x), (0.0185 + i * 0.0008, x), (0.0185 + i * 0.0008, x + 0.004), (0.0, x + 0.004)], R, 16), 'plastic', 0xc42018)
    # the head: a trapezoid in the (x, y) plane (its bristle edge slanted), 5 cm thick
    head = parts.panel('head', [(0.848, -0.150), (0.905, -0.150), (0.932, 0.150), (0.848, 0.150)], -0.026, 0.026, 0.008, 2)
    core.orient_normals(head)
    k.add(head, 'plastic', 0xd8261e)

    def x_base(y):
        return 0.905 + 0.027 * (y + 0.15) / 0.30

    # a dark core so the fan reads dense, then the tufts in two rows, splaying out at the sides
    rnd = random.Random(9)
    core_o = parts.panel('core', [(x_base(-0.146), -0.146), (x_base(-0.146) + 0.135, -0.150), (x_base(0.146) + 0.175, 0.152), (x_base(0.146), 0.146)],
                         -0.016, 0.016, 0.0, 1)
    core.orient_normals(core_o)
    k.add(core_o, 'plastic', 0x1e1e20)
    n = 22
    n = 24
    for row, zc in enumerate((-0.015, 0.0, 0.015)):
        for i in range(n):
            y = -0.140 + 0.280 * (i + 0.33 * row) / (n - 0.34)
            xb = x_base(y)
            L = 0.150 + 0.040 * (y + 0.15) / 0.30 + rnd.uniform(-0.007, 0.007)
            spl = 0.12 * (y / 0.15) * L + rnd.uniform(-0.002, 0.002)
            zs = 0.012 * zc / 0.015 + rnd.uniform(-0.003, 0.003)
            hw0, hz0 = 0.0042, 0.0050

            def ring(x, dy, dz, f):
                return [(x, y + dy - hw0 * f, zc + dz - hz0 * f), (x, y + dy + hw0 * f, zc + dz - hz0 * f), (x, y + dy + hw0 * f, zc + dz + hz0 * f),
                        (x, y + dy - hw0 * f, zc + dz + hz0 * f)]
            ring_b, ring_m, ring_e = ring(xb, 0.0, 0.0, 1.0), ring(xb + L * 0.82, spl * 0.8, zs * 0.8, 1.25), ring(xb + L, spl, zs, 1.45)
            tuft = core.loft('tuft', [ring_b, ring_m])
            core.orient_normals(tuft)
            k.add(tuft, 'bristle', 0x2a2a2d)
            tip = core.loft('tip', [ring_m, ring_e])
            core.orient_normals(tip)
            k.add(tip, 'bristle', 0x8a877e)
    return [k.finish(30.0)]


def draw_herald(s, w, h):
    s.rect(0, 0, w, h, 0xe8e2d0, rough=0.85)
    s.mid('NORTHSIDE HERALD', w / 2, h - 16, 15, w - 20, 0x161616, 'Black')
    s.rect(8, h - 30, w - 16, 0.8, 0x161616)
    s.mid('CITY COUNCIL OKS MARKET ST. REPAVING', w / 2, h - 46, 9.5, w - 20, 0x161616, 'Black')
    r = random.Random(8)
    s.rect(10, h * 0.30, w * 0.45, h * 0.32, 0x6a6862)
    s.circle(10 + w * 0.22, h * 0.48, h * 0.07, 0x8a8880)
    for x0, x1 in ((w * 0.52, w - 10), (10, w * 0.47)):
        y = h - 62 if x0 > 20 else h * 0.27
        while y > 8:
            s.rect(x0, y, (x1 - x0) * r.uniform(0.7, 1.0), 1.5, 0x3a3a36)
            y -= 4.0


def newspaper():
    k = kit('Newspaper', slots=('body', 'print'), px=512, print_px=1024)
    k.panel('herald', 0.30, 0.20, draw_herald)
    k.atlas.render()
    n = 8
    rows = []
    for j in range(n + 1):
        t = j / n
        x = t * 0.30
        rows.append([(x, y, 0.004 * math.sin(t * 3.0) + 0.003 * (y / 0.1) ** 2) for y in np.linspace(-0.10, 0.10, 7)])
    bm = bmesh.new()
    V = [[bm.verts.new(p) for p in r] for r in rows]
    for j in range(n):
        for i in range(6):
            bm.faces.new((V[j][i], V[j + 1][i], V[j + 1][i + 1], V[j][i + 1]))
    o = core.mesh_object('paper', bm)
    K.solidify(o, 0.006, 0.0)
    k.add(o, 'paper', 0xe0dac8)
    k.decal('herald', (0.15, 0.0, 0.0072), u=(1, 0, 0), v=(0, 1, 0), w=0.30, h=0.20, lift=0.0, nu=8, nv=2,
            disp=lambda s_, t_: 0.004 * math.sin(s_ * 3.0) + 0.003 * ((t_ - 0.5) * 2) ** 2 - 0.0)
    return [k.finish(30.0)]


def coffee_cup():
    k = kit('CoffeeCup', slots=('body', 'print'), px=512, print_px=512)
    k.panel('sleeve', 0.20, 0.05, lambda s, w, h: (s.rect(0, 0, w, h, 0x9a7a52, rough=0.9),
                                                     s.mid('HOT! · LUCKY PENNY #212 · HOT!', w / 2, h / 2, h * 0.32, w * 0.95, 0x3a2414, 'Black')))
    k.atlas.render()
    M = T(0.0, 0.0, -0.06)
    k.add(lathe_solid('cup', [(0.0, 0.0), (0.028, 0.0), (0.03, 0.004), (0.041, 0.13), (0.043, 0.132), (0.0, 0.131)], M, 40), 'paper', 0xf2efe8)
    k.add(lathe_solid('lid', [(0.0, 0.128), (0.045, 0.128), (0.046, 0.136), (0.04, 0.142), (0.036, 0.15), (0.0, 0.152)], M, 40), 'plastic', 0xf0efe8)
    k.add(bx('sip', 0.024, 0.034, -0.006, 0.006, 0.089, 0.093, 0.002), 'plastic', 0x1a1a1a)
    sl = core.lathe('sleeve', [(0.0355, 0.045), (0.0392, 0.10)], segments=40)
    sl.data.transform(M)
    K.solidify(sl, 0.0015, 1.0)
    k.add(sl, 'paper', 0x9a7a52)
    K.wrap_cyl(k, 'sleeve', 0.0, 0.0, -0.06 + 0.055, -0.06 + 0.09, 0.0395, -1.6, 1.6)
    return [k.finish(30.0)]


def draw_lock(s, w, h):
    s.rect(0, 0, w, h, 0x0a0c14, rough=0.1)
    s.rect(0, 0, w, h * 0.5, 0x1a2a4a, rough=0.1, alpha=0.8)
    s.mid('11:42', w / 2, h * 0.78, h * 0.09, w * 0.7, 0xd8dce8, 'Medium')
    s.mid('MONDAY, OCTOBER 5', w / 2, h * 0.70, h * 0.022, w * 0.7, 0xb8bccc, 'Bold')
    s.rect(w * 0.1, h * 0.52, w * 0.8, h * 0.08, 0x2a2e3a, rough=0.1)
    s.text('Mom: eat something real', w * 0.14, h * 0.555, h * 0.02, 0xd8dce8, 'Medium')


def phone():
    k = kit('Phone', slots=('body', 'print'), px=512, print_px=512)
    k.panel('screen', 0.07, 0.145, draw_lock, glow=0.6, density=2.0)
    k.atlas.render()
    k.add(bx('body', -0.075, 0.075, -0.036, 0.036, -0.004, 0.004, 0.0035, 3), 'iron', 0xffffff)
    k.add(bx('back', -0.074, 0.074, -0.035, 0.035, -0.0042, -0.0035, 0.003, 2), 'gloss', 0x1a1c22)
    k.add(bx('glass', -0.0735, 0.0735, -0.0345, 0.0345, 0.0035, 0.0041, 0.003, 2), 'glassdark', 0x060708)
    k.decal('screen', (0.0, 0.0, 0.0042), u=(0, -1, 0), v=(1, 0, 0), w=0.068, h=0.142, lift=0.0001)
    k.add(bx('camera', -0.06, -0.04, 0.012, 0.03, -0.0055, -0.004, 0.003), 'glassdark', 0x101010)
    return [k.finish(30.0)]


def draw_form(s, w, h):
    s.rect(0, 0, w, h, 0xf6f4ee, rough=0.8)
    s.mid('IRONHOUSE · TRAINING LOG', w / 2, h - 12, 9, w - 20, 0x1a1a1a, 'Black')
    for i in range(12):
        y = h - 30 - i * 16
        s.rect(10, y, w - 20, 0.5, 0x9a9a9a)
        s.text(['NAME', 'DATE', 'SETS', 'REPS', 'WEIGHT', 'NOTES'][i % 6], 12, y + 3, 5, 0x5a5a5a, 'Bold')


def clipboard():
    k = kit('Clipboard', slots=('body', 'print'), px=512, print_px=512)
    k.panel('form', 0.21, 0.28, draw_form)
    k.atlas.render()
    k.add(bx('board', -0.01, 0.31, -0.115, 0.115, -0.003, 0.0, 0.004), 'wood', 0x8a6a40, aux=0)
    for i in range(4):
        k.add(bx('sheet', 0.01 + i * 0.002, 0.29 - i * 0.001, -0.105 + i * 0.001, 0.105, 0.0, 0.0004 + i * 0.0002), 'paper', 0xf6f4ee)
    k.decal('form', (0.15, 0.0, 0.0013), u=(0, -1, 0), v=(1, 0, 0), w=0.21, h=0.28, lift=0.0)
    k.add(bx('clip_base', 0.26, 0.30, -0.05, 0.05, 0.0, 0.006, 0.002), 'chrome')
    k.add(tube('clip_wire', [(0.29, -0.04, 0.006), (0.25, -0.04, 0.012), (0.25, 0.04, 0.012), (0.29, 0.04, 0.006)], 0.0018, 6), 'chrome')
    return [k.finish(30.0)]


def side_panel(name, outline, t, r, dx=0.0, dz=0.0):
    """A part drawn in side view: an (x, z) outline extruded +-t across Y, its rims rounded by r."""
    o = parts.panel(name, [(x + dx, z + dz) for x, z in outline], -t, t, r, 3)
    core.orient_normals(o)
    o.data.transform(rot_x(90))
    return o


def tag_gun():
    """A garment tagging gun: a rounded head housing with the hollow needle forward, a raked pistol grip with rubber
    side pads, the big trigger lever in front of it, and a clip of plastic fasteners loaded through the top."""
    k = kit('TagGun', px=512)
    ox, oz = 0.022, 0.045          # moves the drawing so the grip's middle sits on the origin
    head = [(-0.050, 0.012), (0.070, 0.012), (0.080, 0.020), (0.084, 0.040), (0.074, 0.056), (0.040, 0.063), (-0.030, 0.063), (-0.052, 0.052),
            (-0.056, 0.030)]
    k.add(side_panel('head', head, 0.0165, 0.006, -ox, -oz), 'plastic', 0x2457a8)
    grip = [(-0.050, 0.016), (-0.010, 0.016), (-0.004, -0.010), (-0.008, -0.060), (-0.016, -0.092), (-0.040, -0.096), (-0.050, -0.084),
            (-0.046, -0.040), (-0.054, -0.004)]
    k.add(side_panel('grip', grip, 0.0135, 0.007, -ox, -oz), 'plastic', 0x2457a8)
    for sg in (-1, 1):
        pad = side_panel('pad', [(-0.042, -0.014), (-0.018, -0.014), (-0.020, -0.074), (-0.037, -0.080), (-0.042, -0.048)], 0.0012, 0.0008, -ox, -oz)
        pad.data.transform(Matrix.Translation((0.0, sg * 0.0136, 0.0)))
        k.add(pad, 'rubber', 0x1a1a1c)
    trig = [(0.010, 0.016), (0.046, 0.016), (0.044, 0.002), (0.036, -0.030), (0.034, -0.070), (0.026, -0.090), (0.012, -0.088), (0.016, -0.050),
            (0.012, -0.010), (0.004, 0.006)]
    k.add(side_panel('trigger', trig, 0.0085, 0.004, -ox, -oz), 'plastic', 0x1a1a1c)
    k.add(parts.rod('pivot', (0.040 - ox, -0.018, 0.020 - oz), (0.040 - ox, 0.018, 0.020 - oz), 0.0035, 12), 'chrome')
    # the nose: a cone to the needle guard, the slotted hollow needle
    k.add(lathe_solid('nose', [(0.0, 0.0), (0.009, 0.0), (0.009, 0.006), (0.005, 0.020), (0.0035, 0.024), (0.0, 0.024)],
                      T(0.080 - ox, 0.0, 0.034 - oz) @ rot_y(90), 16), 'plastic', 0x1a1a1c)
    k.add(lathe_solid('needle', [(0.0, 0.0), (0.0013, 0.0), (0.0013, 0.040), (0.0004, 0.050), (0.0, 0.050)], T(0.100 - ox, 0.0, 0.034 - oz) @ rot_y(90), 8),
          'chrome')
    # the fastener clip: a runner bar with forty T-barbs, fed down into the head's slot from the top
    clip = Matrix.Translation((-0.012 - ox, 0.0, 0.058 - oz)) @ rot_y(-32)
    parts_ = [bx('runner', -0.002, 0.002, -0.0012, 0.0012, 0.0, 0.070, 0.0006)]
    for i in range(40):
        zc = 0.004 + i * 0.0016
        parts_.append(bx('barb', 0.002, 0.024, -0.0003, 0.0003, zc - 0.0003, zc + 0.0003))
        parts_.append(bx('t_end', 0.024, 0.0255, -0.0018, 0.0018, zc - 0.0004, zc + 0.0004))
    for o in parts_:
        o.data.transform(clip)
        k.add(o, 'plastic', 0xeeeae0)
    # the slot's black mouth and the adjusting screw on the side
    k.add(bx('slot', -0.020 - ox, 0.004 - ox, -0.006, 0.006, 0.0615 - oz, 0.0645 - oz, 0.001), 'plastic', 0x0c0c0e)
    k.add(parts.rod('screw', (0.020 - ox, 0.0166, 0.040 - oz), (0.020 - ox, 0.0185, 0.040 - oz), 0.004, 12), 'chrome')
    return [k.finish(30.0)]


def hanger():
    k = kit('Hanger', px=256)
    pts = [(0.0, 0.0, 0.0), (0.0, 0.012, 0.012), (0.0, 0.0, 0.03), (0.0, -0.015, 0.02), (0.0, -0.012, -0.01), (0.0, 0.0, -0.035), (0.0, 0.0, -0.05)]
    k.add(tube('hook', core.catmull_rom(pts, 6), 0.0012, 6), 'chrome')
    k.add(tube('body', bend([(0.0, 0.0, -0.05), (0.0, -0.20, -0.16), (0.0, -0.21, -0.17), (0.0, 0.21, -0.17), (0.0, 0.20, -0.16), (0.0, 0.0, -0.05)], 0.01, 4),
               0.0012, 6), 'chrome')
    k.add(tube('twist', [(0.0, -0.004 * math.cos(a), -0.05 - 0.002 * a) for a in np.linspace(0, 12, 20)], 0.0014, 6), 'chrome')
    return [k.finish(30.0)]


def cigarette():
    k = kit('Cigarette', slots=('body', 'print'), px=256, print_px=128)
    k.panel('ember', 0.008, 0.008, lambda s, w, h: s.circle(w / 2, h / 2, w / 2, 0xff5a10), glow=6.0, density=4.0)
    k.atlas.render()
    M = rot_y(90)
    k.add(lathe_solid('filter', [(0.0, -0.015), (0.0039, -0.015), (0.0039, 0.010), (0.0, 0.010)], M, 16), 'paper', 0xc89a5a)
    k.add(lathe_solid('paper', [(0.0, 0.010), (0.0039, 0.010), (0.0039, 0.064), (0.0, 0.064)], M, 16), 'paper', 0xf4f2ec)
    k.add(lathe_solid('ash', [(0.0, 0.064), (0.0038, 0.064), (0.0036, 0.068), (0.0025, 0.0695), (0.0, 0.0698)], M, 16), 'felt', 0x7a7a74)
    k.decal('ember', (0.0699, 0.0, 0.0), u=(0, 1, 0), v=(0, 0, 1), w=0.006, h=0.006, lift=0.0002,
            outline=[(0.5 + 0.5 * math.cos(2 * math.pi * q / 12), 0.5 + 0.5 * math.sin(2 * math.pi * q / 12)) for q in range(12)])
    return [k.finish(30.0)]


def hand_mirror():
    """The barber's hand mirror (barber-reveal): a black frame with a chrome bezel, a tapered handle, the face ns_mirror."""
    unbaked('ns_mirror')
    k = kit('HandMirror', slots=('body', 'ns_mirror'), px=512)
    k.add(lathe_solid('handle', [(0.0, -0.065), (0.010, -0.065), (0.013, -0.055), (0.0115, -0.01), (0.0105, 0.03), (0.014, 0.05), (0.0, 0.052)], rot_y(90), 24),
          'gloss', 0x161414)
    k.add(lathe_solid('ferrule', [(0.0, 0.048), (0.0155, 0.048), (0.0155, 0.062), (0.0, 0.062)], rot_y(90), 24), 'chrome')
    cm = T(0.145, 0.0, 0.0)
    k.add(lathe_solid('back', [(0.0, -0.010), (0.088, -0.010), (0.094, -0.006), (0.096, 0.002), (0.091, 0.004), (0.0, 0.003)], cm, 64), 'gloss', 0x161414)
    k.add(tube('bezel', [(0.145 + 0.0905 * math.cos(a), 0.0905 * math.sin(a), 0.0052) for a in np.linspace(0, 2 * math.pi, 64, endpoint=False)], 0.0032, 8,
               closed=True), 'chrome')
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new('UVMap')
    n = 64
    c = bm.verts.new((0.145, 0.0, 0.0050))
    vs = [bm.verts.new((0.145 + 0.088 * math.cos(2 * math.pi * i / n), 0.088 * math.sin(2 * math.pi * i / n), 0.0050)) for i in range(n)]
    for i in range(n):
        f = bm.faces.new((c, vs[i], vs[(i + 1) % n]))
        for l in f.loops:
            l[uvl].uv = (0.5 + l.vert.co.y / 0.176, 0.5 + (l.vert.co.x - 0.145) / 0.176)
    face = core.mesh_object('mirror', bm)
    core.orient_normals(face, (0, 0, 1))
    k.special(face, 'ns_mirror')
    return [k.finish(30.0)]


def draw_style_cover(s, w, h):
    s.rect(0, 0, w, h, 0x141414, rough=0.55)
    s.rect(w * 0.12, h * 0.55, w * 0.76, h * 0.3, 0xe8e2d4, rough=0.6)
    s.mid('STYLES', w / 2, h * 0.73, h * 0.11, w * 0.6, 0x1a1a1a, 'Black', tracking=1.3)
    s.mid('FIFTH ST. BARBERS', w / 2, h * 0.61, h * 0.045, w * 0.66, 0x7a1c1c, 'Bold', tracking=1.2)


def style_book():
    k = kit('StyleBook', slots=('body', 'print'), px=512, print_px=512)
    k.panel('cover', 0.25, 0.30, draw_style_cover)
    k.atlas.render()
    k.add(bx('cover_b', -0.005, 0.30, -0.13, 0.13, 0.0, 0.003, 0.002), 'vinyl', 0x141414)
    k.add(bx('cover_t', -0.005, 0.30, -0.13, 0.13, 0.037, 0.040, 0.002), 'vinyl', 0x141414)
    k.add(bx('spine', -0.012, 0.0, -0.13, 0.13, 0.0, 0.040, 0.006), 'vinyl', 0x141414)
    k.add(bx('pages', 0.005, 0.295, -0.125, 0.125, 0.003, 0.037, 0.001), 'paper', 0xefe9da)
    k.decal('cover', (0.15, 0.0, 0.0402), u=(0, -1, 0), v=(1, 0, 0), w=0.25, h=0.28, lift=0.0)
    return [k.finish(30.0)]


BUILDERS = {'Comb': comb, 'Scissors': scissors, 'Clippers': clippers, 'Razor': razor, 'NeckDuster': neck_duster, 'SprayBottle': spray_bottle,
            'Carafe': carafe, 'Plate': plate, 'PancakeStack': pancake_stack, 'Mug': mug, 'Notepad': notepad, 'Pen': pen, 'Fork': fork, 'Rag': rag,
            'Broom': broom, 'Newspaper': newspaper, 'CoffeeCup': coffee_cup, 'Phone': phone, 'Clipboard': clipboard, 'TagGun': tag_gun,
            'Hanger': hanger, 'Cigarette': cigarette, 'HandMirror': hand_mirror, 'StyleBook': style_book}


def build():
    core.reset()
    random.seed(212)
    K.UNB.clear()
    K.PROXIES.clear()
    ALL.clear()
    for n in WHICH:
        ALL.extend(BUILDERS[n]())
    for o in ALL:
        o.data.name = o.name
    return list(ALL)


def bake_parts(objs):
    return K.split_for_bake(objs)


def assemble(objs):
    return K.join_after_bake(ALL, MESHES)


# ================================================================== review

def pose_for_review(objs):
    """ns_handprops_detail.jpg: each prop on a dark counter in warm shop light, framed by its bounds; then the
    standard sheet's row."""
    global REVIEW_VIEWS
    sc = bpy.context.scene
    tmp_dir = os.path.join(core.BUILD_DIR, 'review', 'ns_handprops')
    os.makedirs(tmp_dir, exist_ok=True)
    temp = []
    paths = []
    try:
        top = K.rv_mat('rv_counter', 0x2a2420, 0.35)
        bpy.ops.mesh.primitive_plane_add(size=6.0, location=(0, 0, 0))
        fl = bpy.context.view_layer.objects.active
        core.assign(fl, top)
        temp.append(fl)
        lights = [K.rv_light('rv_key', 'AREA', (-0.6, -0.8, 0.9), 40.0, (1.0, 0.92, 0.82), size=0.6, target=(0, 0, 0)),
                  K.rv_light('rv_fill', 'AREA', (0.8, -0.4, 0.4), 10.0, (0.8, 0.88, 1.0), size=0.8, target=(0, 0, 0)),
                  K.rv_light('rv_rim', 'AREA', (0.2, 0.9, 0.7), 30.0, (1.0, 1.0, 1.0), size=0.4, target=(0, 0, 0))]
        temp += lights
        sc.world.color = (0.02, 0.02, 0.025)
        cd = bpy.data.cameras.new('rv_cam')
        cam = core.link(bpy.data.objects.new('rv_cam', cd))
        cd.clip_start = 0.005
        cd.clip_end = 50.0
        sc.camera = cam
        temp.append(cam)
        for o in objs:
            o.hide_render = True
        for o in objs:
            o.hide_render = False
            lo, hi = core.bounds([o])
            o.location = (-(lo.x + hi.x) / 2, -(lo.y + hi.y) / 2, -lo.z + 0.001)
            bpy.context.view_layer.update()
            size = max((hi - lo).length, 0.03)
            for l in lights:
                l.data.energy = l.data.energy
            cz = (hi.z - lo.z) / 2
            paths.append(K.rv_shot(cam, os.path.join(tmp_dir, f'{o.name}.png'), (-size * 0.75, -size * 0.95, cz + size * 0.62), (0.0, 0.0, cz), 50, 420, 420, 64))
            if o.name == 'SM_NS_Prop_PancakeStack':
                paths.append(K.rv_shot(cam, os.path.join(tmp_dir, 'hero.png'), (-0.10, -0.22, 0.12), (0.0, 0.0, 0.035), 60, 420, 420, 128))
            if hi.x - lo.x > 0.8:
                # a long prop (the broom): its working end close
                ex = (hi.x - lo.x) / 2 - 0.15
                paths.append(K.rv_shot(cam, os.path.join(tmp_dir, f'{o.name}_end.png'), (ex - 0.30, -0.42, 0.30), (ex, 0.0, cz), 50, 420, 420, 64))
            o.hide_render = True
            o.location = (0.0, 0.0, 0.0)
        if paths:
            K.rv_compose('ns_handprops_detail', paths, 6, LOG)
    finally:
        for o in temp:
            try:
                bpy.data.objects.remove(o)
            except ReferenceError:
                pass
        sc.camera = None
        sc.world.color = (0.006, 0.0065, 0.008)
    # the standard sheet: the props in a grid, six to a row, 32 cm apart; anything long (the broom) laid along the back
    cols, cell = 6, 0.32
    i = 0
    # matrix_world is stale until the view layer updates (the detail shots moved every prop): refresh it before
    # measuring the props, and again after the row is laid out so the sheet frames all of it.
    bpy.context.view_layer.update()
    for o in objs:
        o.hide_render = False
        lo, hi = core.bounds([o])
        if hi.x - lo.x > 0.8:
            o.location = (cell * (cols - 1) / 2 - (lo.x + hi.x) / 2, cell * 1.1 - (lo.y + hi.y) / 2, -lo.z)
            continue
        r, c = divmod(i, cols)
        o.location = (c * cell - (lo.x + hi.x) / 2, -r * cell - (lo.y + hi.y) / 2, -lo.z)
        i += 1
    bpy.context.view_layer.update()
    REVIEW_VIEWS = None
