"""SM_Sedan: the parked cars along Fifth Street's curbs (one mesh; the game paints each a color).

A late-2000s mid-size four-door, 4.85 m long, 1.83 wide, 1.45 tall on a 2.80 m wheelbase:
- The body is lofted from feature-aligned sections (rocker, a lower character line, door side, a crisp
  shoulder crease, the ledge under the glass, hood and deck), so its lines run where a stamped panel's
  would; the wheel arches are cut and lined, with a rolled lip; black valances under the bumpers.
- The greenhouse is its own loft: raked A pillars, a gloss-black B pillar, the C pillar's sail in body
  color, a quarter light behind the rear door, the windshield and backlight curved in plan. The glass
  sits back in its seals behind chrome trim; it is dark tint, and the cabin (seats, headrests, the dash)
  is baked into it as seen straight through each pane, so it shows faintly in the game's opaque glass.
- Headlamp and tail-lamp clusters are lenses fitted onto the body's corners (found by rays cast out from
  inside), so they follow it, each in a black gasket. What's behind each lens is in its maps, measured in 3D
  on the lens so it stays round however the corner curves: the headlamp's silver housing, fluted chrome
  high-beam bowl and projector; the tail lamp's pillow optics, Fresnel-stepped round lamps and clear reverse
  lamp; both falling into shadow toward the opening's edge. Each lens is unwrapped whole into its own strip.
  The grille, intake, fog lamps and rear plate sit in pockets cut into the bumper covers.
- Mirrors on the sails, chrome handles in dark cups, chrome round the side glass and along the belt, the
  cowl and wipers, the plates (NORTHSIDE, OPEN ALL NIGHT), a trunk garnish, reflectors, one exhaust.
- Shut lines (doors, hood, trunk, bumper covers, fuel door) are drawn into the paint and its normal map.
- 17" five twin-spoke alloys on 225/50 tires with a lettered sidewall (reads clockwise from outside on
  either side), sitting a little squashed; brake discs with flash rust and calipers behind.
- The paint is a light grey metallic, glossy as a clear coat (the glTF has no coat layer, so the one layer
  carries it), with wet-road film and spray along the sills and behind the wheels, grime settled in its
  crevices, stone chips on the nose and sills: AStreetStage tints this first material per car (glTF
  BaseColorFactor).
- The plates carry their bolts and a registration sticker.

Coordinates (meters), front toward -Y, Z up: origin on the road under the middle of the car.
Material slots, in order: paint (tinted by the game), trim, glass, chrome, headlamp, amber, red, plate,
tire, rim, brake.
"""
import bisect
import math
import os

import bpy  # noqa: F401,I001
import bmesh
import numpy as np
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

from artkit import core, parts, street
from artkit.shade import image_surface, mix_float, noise, object_coords, ramp, scaled, smooth_less, vec
from artkit.sheet import Sheet

MESHES = ['SM_Sedan']
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
# The lamps' lenses are each unwrapped whole (lamp_uvs) into wide strips: 0.7-0.9 mm a texel, enough for their optics.
TEXTURE_SIZES = {'car_paint': 2048, 'car_trim': 1024, 'car_glass': 1024, 'car_chrome': 512, 'car_head': (1024, 512), 'car_amber': 128,
                 'car_red': (1024, 512), 'car_plate': 512, 'car_tire': 1024, 'car_rim': 1024, 'car_brake': 512}
AO_DISTANCE = 0.15
# The rear view comes in close enough to read the plate.
REVIEW_VIEWS = [('front', -35, 12, 2.4), ('rear', 150, 12, 0.85, (0.0, 1.9, 0.8)), ('wheel', -75, 5, 0.75, (-0.9, -1.45, 0.4))]

Y_F, Y_R = -2.425, 2.425
AXLES = (-1.42, 1.38)
TRACK = 0.785        # wheel center from the middle
WZ = 0.326           # wheel center height: the tires sit a little squashed
TIRE_R = 0.334
ARCH_R = 0.386
TUMBLE = 0.43        # side glass lean (run over rise)


# ------------------------------------------------------------------ curves

def pchip(keys):
    """A smooth, overshoot-free curve through (x, value) keys (monotone cubic); flat outside them."""
    xs = [k[0] for k in keys]
    ys = [k[1] for k in keys]
    n = len(xs)
    h = [xs[i + 1] - xs[i] for i in range(n - 1)]
    d = [(ys[i + 1] - ys[i]) / h[i] for i in range(n - 1)]
    m = [0.0] * n
    m[0], m[-1] = d[0], d[-1]
    for i in range(1, n - 1):
        if d[i - 1] * d[i] <= 0:
            m[i] = 0.0
        else:
            w1, w2 = 2 * h[i] + h[i - 1], h[i] + 2 * h[i - 1]
            m[i] = (w1 + w2) / (w1 / d[i - 1] + w2 / d[i])

    def f(x):
        if x <= xs[0]:
            return ys[0]
        if x >= xs[-1]:
            return ys[-1]
        i = min(bisect.bisect_right(xs, x) - 1, n - 2)
        t = (x - xs[i]) / h[i]
        t2, t3 = t * t, t * t * t
        return ((2 * t3 - 3 * t2 + 1) * ys[i] + (t3 - 2 * t2 + t) * h[i] * m[i] + (-2 * t3 + 3 * t2) * ys[i + 1] + (t3 - t2) * h[i] * m[i + 1])
    return f


def smoothstep(e0, e1, x):
    t = max(0.0, min(1.0, (x - e0) / (e1 - e0)))
    return t * t * (3 - 2 * t)


def cr_loop(pts, counts, alpha=0.5):
    """Centripetal Catmull-Rom through a closed loop of 2D points: counts[i] samples on the span from point i
    (point i itself first), so every loop built from the same counts has the same topology."""
    P = [Vector(p) for p in pts]
    n = len(P)
    out = []
    for i in range(n):
        p0, p1, p2, p3 = P[i - 1], P[i], P[(i + 1) % n], P[(i + 2) % n]
        t0 = 0.0
        t1 = t0 + max((p1 - p0).length, 1e-5) ** alpha
        t2 = t1 + max((p2 - p1).length, 1e-5) ** alpha
        t3 = t2 + max((p3 - p2).length, 1e-5) ** alpha
        for k in range(counts[i]):
            t = t1 + (t2 - t1) * k / counts[i]
            a1 = (t1 - t) / (t1 - t0) * p0 + (t - t0) / (t1 - t0) * p1
            a2 = (t2 - t) / (t2 - t1) * p1 + (t - t1) / (t2 - t1) * p2
            a3 = (t3 - t) / (t3 - t2) * p2 + (t - t2) / (t3 - t2) * p3
            b1 = (t2 - t) / (t2 - t0) * a1 + (t - t0) / (t2 - t0) * a2
            b2 = (t3 - t) / (t3 - t1) * a2 + (t - t1) / (t3 - t1) * a3
            c = (t2 - t) / (t2 - t1) * b1 + (t - t1) / (t2 - t1) * b2
            out.append((c.x, c.y))
    return out


def mirrored_loop(half, counts):
    """A section symmetric about x = 0 from its right half, listed bottom center to top center."""
    pts = list(half) + [(-x, z) for x, z in reversed(half[1:-1])]
    return cr_loop(pts, list(counts) + list(reversed(counts)))


# ------------------------------------------------------------------ the body

def slotted_object(name, bm, mats):
    """A mesh object from bm carrying the car's full material list (faces keep the slots bm gave them)."""
    me = bpy.data.meshes.new(name)
    for m in mats['order']:
        me.materials.append(m.m)
    bm.to_mesh(me)
    bm.free()
    return core.link(bpy.data.objects.new(name, me))


# Feature curves along the car (y): bottom; the side's widest; the shoulder crease (height, half width); the top
# edge where the shoulder turns onto the hood, the ledge under the glass or the deck (height, half width); the top's
# middle. The ends close in on the bumpers' noses.
ZB = pchip([(-2.425, 0.40), (-2.418, 0.35), (-2.40, 0.30), (-2.37, 0.27), (-2.32, 0.252), (-2.2, 0.24), (-1.95, 0.228), (-1.0, 0.188),
            (0.5, 0.181), (1.0, 0.19), (1.80, 0.24), (2.15, 0.28), (2.32, 0.31), (2.40, 0.345), (2.425, 0.40)])
WS = pchip([(-2.425, 0.36), (-2.421, 0.47), (-2.413, 0.56), (-2.40, 0.64), (-2.38, 0.71), (-2.35, 0.765), (-2.31, 0.81), (-2.25, 0.85),
            (-2.17, 0.878), (-2.06, 0.897), (-1.90, 0.907), (-1.42, 0.913), (0.4, 0.915), (1.38, 0.915), (1.85, 0.91), (2.10, 0.898), (2.24, 0.88),
            (2.32, 0.855), (2.37, 0.82), (2.40, 0.77), (2.415, 0.70), (2.422, 0.61), (2.425, 0.50)])
ZS = pchip([(-2.425, 0.56), (-2.40, 0.625), (-2.35, 0.68), (-2.25, 0.712), (-2.0, 0.735), (-1.4, 0.765), (-0.8, 0.79), (0.4, 0.815),
            (1.4, 0.845), (2.0, 0.865), (2.28, 0.872), (2.36, 0.85), (2.40, 0.80), (2.425, 0.72)])
WSH = pchip([(-2.425, 0.30), (-2.415, 0.47), (-2.40, 0.575), (-2.37, 0.665), (-2.33, 0.74), (-2.27, 0.80), (-2.19, 0.845), (-2.08, 0.875),
             (-1.9, 0.895), (-1.42, 0.905), (0.4, 0.909), (1.38, 0.909), (1.85, 0.903), (2.10, 0.89), (2.24, 0.868), (2.32, 0.84), (2.37, 0.80),
             (2.40, 0.745), (2.415, 0.67), (2.425, 0.50)])
ZT = pchip([(-2.425, 0.60), (-2.405, 0.665), (-2.375, 0.72), (-2.335, 0.762), (-2.28, 0.788), (-2.15, 0.81), (-1.9, 0.835), (-1.5, 0.875),
            (-1.1, 0.925), (-1.0, 0.95), (-0.92, 0.964), (-0.3, 0.972), (0.3, 0.981), (0.9, 0.993), (1.3, 1.004), (1.66, 1.02), (1.85, 1.032),
            (2.1, 1.04), (2.29, 1.045), (2.345, 1.03), (2.385, 0.98), (2.41, 0.90), (2.425, 0.78)])
WT = pchip([(-2.425, 0.26), (-2.41, 0.42), (-2.38, 0.53), (-2.33, 0.615), (-2.25, 0.68), (-2.1, 0.725), (-1.8, 0.75), (-1.3, 0.765),
            (-0.92, 0.779), (-0.6, 0.801), (-0.1, 0.815), (0.4, 0.819), (0.9, 0.812), (1.3, 0.799), (1.66, 0.776), (2.0, 0.765), (2.25, 0.745),
            (2.33, 0.715), (2.38, 0.66), (2.41, 0.58), (2.425, 0.46)])
ZC = pchip([(-2.425, 0.62), (-2.405, 0.69), (-2.375, 0.75), (-2.335, 0.792), (-2.28, 0.815), (-2.15, 0.836), (-1.9, 0.862), (-1.5, 0.902),
            (-1.1, 0.95), (-1.0, 0.965), (-0.92, 0.972), (-0.5, 0.974), (0.4, 0.985), (1.0, 0.997), (1.35, 1.007), (1.62, 1.024), (1.85, 1.045),
            (2.1, 1.056), (2.29, 1.058), (2.345, 1.045), (2.385, 0.99), (2.41, 0.91), (2.425, 0.79)])
# Samples per span of the body section (right half, bottom center to top center): the underbody, the rocker's turn,
# up to the lower character line (one tight span), the door side, the shoulder crease (one tight span), the shoulder,
# the top edge's turn, the top.
BODY_SPANS = [2, 2, 2, 2, 1, 2, 3, 2, 1, 3, 2, 3, 3]
LOW_SPANS = {1, 2, 3, 2 * len(BODY_SPANS) - 2, 2 * len(BODY_SPANS) - 1, 2 * len(BODY_SPANS)}   # under the car, round the rocker


def body_half(y):
    zb, ws, zs = ZB(y), WS(y), ZS(y)
    wsh = min(WSH(y), ws - 0.002)
    zt = ZT(y)
    wt = min(WT(y), wsh - 0.032)
    zc = max(ZC(y), zt + 0.004)
    zw = zb + 0.48 * (zs - zb)
    # The lower character line: the door tucks in beneath it toward the sill.
    zl = zb + 0.065 + 0.55 * (zw - zb - 0.065)
    crown = zc - zt
    return [(0.0, zb), (0.55 * ws, zb), (ws - 0.07, zb + 0.006), (ws - 0.03, zb + 0.065), (ws - 0.0115, zl - 0.007), (ws - 0.0035, zl + 0.007),
            (ws, zw), (ws - 0.003, zs - 0.07),
            (wsh, zs), (wsh - 0.006, zs + 0.014), (wt + 0.022, zt - 0.02), (wt - 0.01, zt + 0.003), (0.5 * wt, zt + 0.75 * crown), (0.0, zc)]


def body_stations():
    end = [0.0, 0.0015, 0.004, 0.0075, 0.012, 0.018, 0.025, 0.033, 0.042, 0.052, 0.063, 0.075, 0.088, 0.102, 0.117, 0.135, 0.155, 0.18,
           0.21, 0.245, 0.285, 0.33, 0.38, 0.435, 0.495]
    front = [Y_F + d for d in end]
    rear = [Y_R - d for d in end]
    a, b = front[-1], rear[-1]
    n = int(round((b - a) / 0.06))
    mid = [a + (b - a) * i / n for i in range(1, n)]
    return front + mid + list(reversed(rear))


def loft_spans(name, rings, counts, mats):
    """A closed loft through the rings whose faces remember which span of the section they came from (1-based), so
    regions of the shell can be told apart after booleans (whose own faces come out 0)."""
    bm = bmesh.new()
    span_of = []
    for j, c in enumerate(list(counts) + list(reversed(counts))):
        span_of += [j + 1] * c
    layer = bm.faces.layers.int.new('span')
    verts = [[bm.verts.new(p) for p in ring] for ring in rings]
    m = len(rings[0])
    for a, b in zip(verts, verts[1:]):
        for k in range(m):
            f = bm.faces.new((a[k], a[(k + 1) % m], b[(k + 1) % m], b[k]))
            f[layer] = span_of[k]
    bm.faces.new(list(reversed(verts[0])))
    bm.faces.new(verts[-1])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    return slotted_object(name, bm, mats)


def prism_y(name, outline, y0, y1):
    """A prism along Y from an (x, z) outline: cutters for pockets in the bumpers."""
    bm = bmesh.new()
    a = [bm.verts.new((x, y0, z)) for x, z in outline]
    b = [bm.verts.new((x, y1, z)) for x, z in outline]
    bm.faces.new(a)
    bm.faces.new(list(reversed(b)))
    n = len(outline)
    for i in range(n):
        bm.faces.new((a[i], a[(i + 1) % n], b[(i + 1) % n], b[i]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    return core.mesh_object(name, bm)


def rrect(cx, cz, hw, hh, r, steps=5, taper=0.0):
    """A rounded rectangle outline (x, z) centered at (cx, cz), half sizes hw, hh; taper narrows the bottom."""
    pts = core.rounded_rect(cx - hw, cz - hh, cx + hw, cz + hh, r, steps)
    return [(x if z > cz else cx + (x - cx) * (1.0 - taper), z) for x, z in pts]


# Front-view openings in the bumper cover: the grille between the headlamps, the lower intake, fog lamps.
GRILLE = rrect(0.0, 0.712, 0.385, 0.078, 0.032, taper=0.15)
INTAKE = rrect(0.0, 0.345, 0.43, 0.052, 0.04, taper=-0.06)
FOG = [(0.665, 0.37)]
FOG_R = 0.043
PLATE_F = (0.0, 0.505)     # plate centers (x, z)
PLATE_R = (0.0, 0.60)


def body_shell(mats):
    rings = []
    for y in body_stations():
        rings.append([Vector((x, y, z)) for x, z in mirrored_loop(body_half(y), BODY_SPANS)])
    body = loft_spans('body', rings, BODY_SPANS, mats)
    idx = mats['index']
    # Where the bumper covers sit before anything is cut into them (the grille's bars follow it).
    front_bvh = BVHTree.FromObject(body, bpy.context.evaluated_depsgraph_get())
    cuts = []
    # The wheel wells: cut from the outside in to the inner liner, lined in black.
    for ay in AXLES:
        for sx in (-1, 1):
            cuts.append(parts.rod('arch_cut', (sx * 0.56, ay, WZ + 0.004), (sx * 1.3, ay, WZ + 0.004), ARCH_R, 72))
    cuts.append(prism_y('grille_cut', GRILLE, Y_F - 0.2, Y_F + 0.125))
    cuts.append(prism_y('intake_cut', INTAKE, Y_F - 0.2, Y_F + 0.06))
    for fx, fz in FOG:
        for sx in (-1, 1):
            cuts.append(parts.rod('fog_cut', (sx * fx, Y_F - 0.2, fz), (sx * fx, Y_F + 0.075, fz), FOG_R, 32))
    cuts.append(prism_y('plate_cut', rrect(PLATE_R[0], PLATE_R[1], 0.172, 0.088, 0.012), Y_R - 0.016, Y_R + 0.2))
    for c in cuts:
        core.assign(c, mats['trim'])
        core.boolean(body, c)
    # Black where a real car is black plastic: underneath, the air dam under the front bumper, the diffuser under the rear.
    span = body.data.attributes['span']
    for p in body.data.polygons:
        sp = span.data[p.index].value
        c = p.center
        low = sp in LOW_SPANS
        n = p.normal
        # The valances run round the bumpers' corners to the wheel arches.
        if (n.z < -0.9 and c.z < ZB(c.y) + 0.012) or (low and (c.y < AXLES[0] - ARCH_R or c.y > AXLES[1] + ARCH_R)):
            p.material_index = idx['car_trim']
    return body, front_bvh


# ------------------------------------------------------------------ the greenhouse

BELT = pchip([(-0.92, 0.968), (-0.3, 0.976), (0.3, 0.985), (0.9, 0.997), (1.3, 1.008), (1.66, 1.024)])
GH_XB = pchip([(-0.92, 0.765), (-0.6, 0.787), (-0.1, 0.801), (0.4, 0.805), (0.9, 0.798), (1.3, 0.785), (1.66, 0.762)])
# The drip line: the top of the side glass, up the A pillar, along the roof rail and down the backlight's edge.
DRIP = pchip([(-0.92, 0.968), (-0.77, 1.03), (-0.52, 1.16), (-0.22, 1.32), (-0.04, 1.37), (0.18, 1.39), (0.48, 1.392), (0.72, 1.378),
              (0.87, 1.33), (1.07, 1.215), (1.37, 1.09), (1.66, 1.024)])
# The roof's middle: windshield, roof, backlight.
TOP = pchip([(-0.92, 0.973), (-0.77, 1.095), (-0.52, 1.245), (-0.24, 1.385), (-0.10, 1.422), (0.13, 1.446), (0.38, 1.452), (0.63, 1.444),
             (0.80, 1.425), (0.93, 1.38), (1.18, 1.265), (1.43, 1.15), (1.66, 1.03)])
# The windshield and backlight curve in plan: their middles sit ahead of / behind their edges by this much.
BOW = pchip([(-0.92, -0.10), (-0.52, -0.085), (-0.17, -0.05), (0.18, 0.0), (0.68, 0.03), (1.08, 0.07), (1.66, 0.085)])
GH_Y0, GH_Y1 = -0.92, 1.66
Y_HDR, Y_RR = -0.16, 0.81      # the stations where the windshield meets the roof, and the roof the backlight
GH_SPANS = [2, 1, 1, 5, 2, 2, 3, 3]
# Side-view lines (y, z) - (y, z) that part the side glass: the mirror sail's back edge, the B pillar, the divider in
# the rear door, the C pillar's leading edge.
SAIL = ((-0.705, 0.965), (-0.82, 1.115))
B_FRONT = ((0.185, 0.97), (0.145, 1.42))
B_REAR = ((0.31, 0.97), (0.27, 1.42))
DIV_FRONT = ((0.885, 0.97), (0.855, 1.42))
DIV_REAR = ((0.91, 0.97), (0.88, 1.42))
C_EDGE = ((1.30, 0.97), (0.84, 1.42))


def band_f(y):
    return max(0.02, smoothstep(GH_Y0, GH_Y0 + 0.11, y) * smoothstep(GH_Y1, GH_Y1 - 0.13, y))


def gh_half(y):
    zb, xb = BELT(y), GH_XB(y)
    f = band_f(y)
    zd = max(DRIP(y), zb + 0.002)
    xd = xb - (zd - zb) * TUMBLE
    zr, xr = zd + 0.045 * f, xd - 0.05 * f
    zc = max(TOP(y), zr + 0.003)
    return [(0.0, zb - 0.06), (xb - 0.04, zb - 0.06), (xb + 0.003, zb - 0.025), (xb, zb), (xd, zd), (xd - 0.012 * f, zd + 0.03 * f), (xr, zr),
            (0.55 * xr, zc - 0.3 * (zc - zr)), (0.0, zc)]


def gh_stations():
    d = [0.0, 0.005, 0.013, 0.024, 0.038, 0.055, 0.075, 0.10, 0.13, 0.165]
    ys = [GH_Y0 + v for v in d] + [GH_Y1 - v for v in d]
    a, b = GH_Y0 + 0.2, GH_Y1 - 0.2
    n = int(round((b - a) / 0.055))
    ys += [a + (b - a) * i / n for i in range(n + 1)]
    ys += [Y_HDR, Y_RR]
    return sorted(set(round(v, 4) for v in ys))


def line_y(line, z):
    (y0, z0), (y1, z1) = line
    return y0 + (z - z0) * (y1 - y0) / (z1 - z0)


def greenhouse(mats):
    """The cabin's upper half: one loft whose faces are sorted into glass, pillars and roof, the glass set back in its
    seals. Returns the object and its drip-line points (for the window trim)."""
    idx = mats['index']
    stations = gh_stations()
    half_n = sum(GH_SPANS)
    ring_n = 2 * half_n
    # Ring indices: side glass runs from the belt (k_belt) to the drip (k_drip); the top band starts at k_roof.
    k_belt = sum(GH_SPANS[:3])
    k_drip = sum(GH_SPANS[:4])
    k_roof = sum(GH_SPANS[:6])
    bm = bmesh.new()
    band = bm.faces.layers.int.new('band')
    stn = bm.faces.layers.int.new('station')
    verts = []
    drip_pts = []
    for i, y in enumerate(stations):
        loop = mirrored_loop(gh_half(y), GH_SPANS)
        xr = gh_half(y)[6][0]
        bow = BOW(y)
        row = []
        for k, (x, z) in enumerate(loop):
            yy = y
            # The top band bows in plan (windshield and backlight curve; the header and the roof's back edge with them).
            kk = k if k <= half_n else ring_n - k
            if kk > k_roof:
                yy = y + bow * max(0.0, 1.0 - (x / max(xr, 1e-3)) ** 2)
            row.append(bm.verts.new((x, yy, z)))
        verts.append(row)
        drip_pts.append((Vector((loop[k_drip][0], y, loop[k_drip][1])), Vector((loop[ring_n - k_drip][0], y, loop[ring_n - k_drip][1]))))
    for i in range(len(stations) - 1):
        a, b = verts[i], verts[i + 1]
        for k in range(ring_n):
            f = bm.faces.new((a[k], a[(k + 1) % ring_n], b[(k + 1) % ring_n], b[k]))
            kk = k if k < half_n else ring_n - 1 - k
            f[band] = 0 if kk < k_belt else 1 if kk < k_drip else 2 if kk < k_roof else 3
            f[stn] = i
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    # Out of the belt: the hidden skirt inside the body goes.
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if f[band] == 0 and min(v.co.z for v in f.verts) < BELT(f.calc_center_median().y) - 0.03],
                     context='FACES')
    # Cut the side glass along the pillars' raked edges.
    for line in (SAIL, B_FRONT, B_REAR, DIV_FRONT, DIV_REAR, C_EDGE):
        (y0, z0), (y1, z1) = line
        d = Vector((0.0, y1 - y0, z1 - z0)).normalized()
        nrm = Vector((0.0, -d.z, d.y))
        geom = bm.verts[:] + bm.edges[:] + bm.faces[:]
        bmesh.ops.bisect_plane(bm, geom=geom, dist=1e-5, plane_co=(0.0, y0, z0), plane_no=nrm)
    i_hdr = stations.index(round(Y_HDR, 4))
    i_rr = stations.index(round(Y_RR, 4))
    glass_faces = []
    for f in bm.faces:
        c = f.calc_center_median()
        b = f[band]
        mask = 0.0
        if b == 0:
            mi = idx['car_trim']
        elif b == 1:
            if c.y < line_y(SAIL, c.z) and c.z < 1.13:
                mi = idx['car_trim']
            elif line_y(B_FRONT, c.z) < c.y < line_y(B_REAR, c.z) or line_y(DIV_FRONT, c.z) < c.y < line_y(DIV_REAR, c.z):
                mi = idx['car_glass']   # gloss black appliques: glass that shows nothing behind it
            elif c.y > line_y(C_EDGE, c.z):
                mi = idx['car_paint']
            else:
                mi, mask = idx['car_glass'], 1.0
        elif b == 2:
            mi = idx['car_paint']
        else:
            if f[stn] < i_hdr or f[stn] >= i_rr:
                mi, mask = idx['car_glass'], 1.0
            else:
                mi = idx['car_paint']
        f.material_index = mi
        if mask > 0.5:
            glass_faces.append(f)
    # The glass sits back in black seals.
    res = bmesh.ops.inset_region(bm, faces=glass_faces, thickness=0.006, depth=-0.007, use_even_offset=True, use_boundary=True)
    for f in res['faces']:
        f.material_index = idx['car_trim']
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-5)
    return slotted_object('greenhouse', bm, mats), drip_pts, stations


# ------------------------------------------------------------------ wheels

def closed_lathe(name, profile, matrix, segments=48):
    """A solid of revolution from a closed (radius, axial) outline, moved into place by matrix."""
    obj = core.lathe(name, list(profile) + [profile[0]], segments=segments)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-6)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.transform(matrix)
    return obj


TIRE = [(0.218, -0.099), (0.226, -0.107), (0.24, -0.1115), (0.265, -0.1135), (0.292, -0.1115), (0.313, -0.1055), (0.3255, -0.095), (0.332, -0.08),
        (0.3338, -0.06), (0.334, -0.03), (0.334, 0.03), (0.3338, 0.06), (0.332, 0.08), (0.3255, 0.095), (0.313, 0.1055), (0.292, 0.1115),
        (0.265, 0.1135), (0.24, 0.1115), (0.226, 0.107), (0.218, 0.099), (0.212, 0.094), (0.212, -0.094)]
RIM = [(0.2, 0.101), (0.211, 0.1045), (0.221, 0.1035), (0.2235, 0.099), (0.219, 0.094), (0.212, 0.09), (0.207, 0.08), (0.205, -0.08), (0.212, -0.09),
       (0.219, -0.095), (0.212, -0.099), (0.198, -0.094), (0.194, 0.06), (0.192, 0.074), (0.196, 0.092)]


def spoke_face(r):
    """How far out the spokes' faces stand at radius r: recessed at the hub, rising to the lip (a concave wheel)."""
    t = (r - 0.07) / (0.196 - 0.07)
    return 0.056 + 0.03 * t + 0.004 * math.sin(math.pi * t)


def wheel(cx, cy, mats):
    side = 1 if cx > 0 else -1
    axle = Matrix.Translation((cx, cy, WZ)) @ Matrix.Rotation(math.radians(90 * side), 4, 'Y')

    def at(r, ang, w):
        """Wheel-local (radius, angle, outward offset) to world: the angle runs round the axle in the YZ plane."""
        return Vector((cx + side * w, cy + r * math.cos(ang), WZ + r * math.sin(ang)))

    out = []
    tire = closed_lathe('tire', TIRE, axle, 64)
    # Sitting on its weight: the contact patch flat, the sidewall bulging a touch above it.
    for v in tire.data.vertices:
        if v.co.z < 0.0:
            v.co.z = 0.0
        if v.co.z < 0.07:
            lx = v.co.x - cx
            if abs(lx) > 0.07:
                v.co.x += math.copysign(0.005 * (1.0 - v.co.z / 0.07), lx)
    out.append((tire, mats['tire']))
    out.append((closed_lathe('rim', RIM, axle, 56), mats['rim']))
    # Hub: the center disc, the cap, five lug nuts.
    out.append((closed_lathe('hub', [(0.0, 0.05), (0.082, 0.05), (0.09, 0.044), (0.09, 0.02), (0.0, 0.02)], axle, 40), mats['rim']))
    out.append((closed_lathe('cap', [(0.0, 0.069), (0.022, 0.0685), (0.031, 0.065), (0.034, 0.058), (0.034, 0.05), (0.0, 0.05)], axle, 32), mats['rim']))
    for k in range(5):
        a = 2 * math.pi * k / 5 + math.pi / 10
        out.append((parts.rod('lug', at(0.0572, a, 0.048), at(0.0572, a, 0.064), 0.0105, 6), mats['chrome']))
    # Five twin spokes, each pair a V closing toward the hub.
    for k in range(5):
        base = 2 * math.pi * k / 5 + math.pi / 2
        for sgn in (-1, 1):
            rings = []
            for j in range(9):
                r = 0.072 + (0.197 - 0.072) * j / 8
                t = j / 8
                ang = base + sgn * (0.135 - 0.04 * t) * (0.33 + 0.67 * min(1.0, t * 2.0))
                hw = 0.0148 - 0.0018 * t
                wf = spoke_face(r)
                wb = wf - 0.032
                tdir = Vector((0.0, -math.sin(ang), math.cos(ang)))
                c = at(r, ang, 0.0)
                sec = [(-0.75 * hw, wf), (0.75 * hw, wf), (hw, wf - 0.006), (0.9 * hw, wb), (-0.9 * hw, wb), (-hw, wf - 0.006)]
                rings.append([c + tdir * t_ + Vector((side * w, 0.0, 0.0)) for t_, w in sec])
            out.append((core.loft('spoke', rings), mats['rim']))
    # Brake: the disc (its hat behind the hub), the caliper at the back of it.
    out.append((closed_lathe('rotor', [(0.0, 0.03), (0.08, 0.03), (0.082, 0.016), (0.150, 0.016), (0.152, 0.012), (0.152, -0.010), (0.150, -0.014),
                                       (0.07, -0.014), (0.0, -0.014)], axle, 36), mats['brake']))
    ca = math.radians(18.0)
    cal = parts.rbox('caliper', (0, 0, 0), (0.05, 0.11, 0.055), 0.014)
    rot = Matrix.Rotation(ca, 4, 'X')
    cal.data.transform(Matrix.Translation(at(0.135, ca, 0.004)) @ rot)
    out.append((cal, mats['brake']))
    return out


# ------------------------------------------------------------------ fitting parts to the body

def hit(bvh, origin, direction, reach=6.0):
    """Where a ray meets a surface: (point, normal), or (None, None)."""
    loc, nrm, _, _ = bvh.ray_cast(Vector(origin), Vector(direction).normalized(), reach)
    return loc, nrm


def resample(outline, n, su=1.0):
    """n points evenly spaced round a closed outline (its first coordinate scaled by su when measuring)."""
    pts = list(outline) + [outline[0]]
    seg = [math.hypot((b[0] - a[0]) * su, b[1] - a[1]) for a, b in zip(pts, pts[1:])]
    total = sum(seg)
    out, i, acc = [], 0, 0.0
    for k in range(n):
        t = total * k / n
        while acc + seg[i] < t:
            acc += seg[i]
            i += 1
        f = (t - acc) / seg[i] if seg[i] > 0 else 0.0
        a, b = pts[i], pts[i + 1]
        out.append((a[0] + (b[0] - a[0]) * f, a[1] + (b[1] - a[1]) * f))
    return out


LENS_STEPS = (1.0, 0.92, 0.83, 0.73, 0.62, 0.5, 0.38, 0.26, 0.14)


def drop_faces(obj, test):
    """Deletes the faces of obj whose index test accepts."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.faces.ensure_lookup_table()
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if test(f.index)], context='FACES')
    bm.to_mesh(obj.data)
    bm.free()


def lens_patch(name, bvh, outline, ray, lift=0.003, skirt=0.014, steps=(1.0, 0.84, 0.67, 0.5, 0.33, 0.17), seal=None):
    """A lens (or any skin) fitted to the body: outline is a closed list of 2D parameters, ray(param) gives the
    (origin, direction) that finds its place on the surface (or the place itself, as (point, normal)). Filled with
    shrinking copies of the outline, lifted off the surface a little, its rim turned down into the body so no gap shows.
    seal: a list that takes that rim as an object of its own (the caller makes it the black gasket a lamp sits in)."""
    n = len(outline)
    cu = sum(p[0] for p in outline) / n
    cv = sum(p[1] for p in outline) / n

    def locate(param):
        r = ray(param)
        return r if isinstance(r[0], Vector) or r[0] is None else hit(bvh, *r)

    rows = []
    for s in steps:
        row = []
        for u, v in outline:
            p, nr = locate((cu + (u - cu) * s, cv + (v - cv) * s))
            if p is None:
                p, nr = row[-1]
            row.append((p, nr))
        rows.append(row)
    pc, nc = locate((cu, cv))
    bm = bmesh.new()
    vrows = [[bm.verts.new(p - nr * skirt) for p, nr in rows[0]]]
    vrows += [[bm.verts.new(p + nr * lift) for p, nr in row] for row in rows]
    vc = bm.verts.new(pc + nc * lift)
    for a, b in zip(vrows, vrows[1:]):
        for k in range(n):
            bm.faces.new((a[k], a[(k + 1) % n], b[(k + 1) % n], b[k]))
    for k in range(n):
        bm.faces.new((vrows[-1][k], vrows[-1][(k + 1) % n], vc))
    obj = core.mesh_object(name, bm)
    core.orient_normals(obj, toward=nc)
    if seal is not None:
        # The rim band's faces came first.
        rim = obj.copy()
        rim.data = obj.data.copy()
        rim.name = name + '_seal'
        core.link(rim)
        drop_faces(rim, lambda i: i >= n)
        drop_faces(obj, lambda i: i < n)
        seal.append(rim)
    return obj


def ribbon(name, pts, nrms, profile, closed=False, sides=None):
    """A strip swept along a path of surface points (trim, seals, lips): profile is a closed polygon of (across, out)
    offsets, across along sides[i] (default: the surface normal crossed with the path), out along the normal."""
    n = len(pts)
    rings = []
    for i in range(n):
        p, nr = Vector(pts[i]), Vector(nrms[i]).normalized()
        if sides is not None:
            sd = Vector(sides[i]).normalized()
        else:
            a = Vector(pts[i - 1]) if (closed or i > 0) else p
            b = Vector(pts[(i + 1) % n]) if (closed or i < n - 1) else p
            sd = nr.cross((b - a).normalized()).normalized()
        rings.append([p + sd * u + nr * w for u, w in profile])
    if not closed:
        return core.loft(name, rings)
    bm = bmesh.new()
    vs = [[bm.verts.new(q) for q in ring] for ring in rings]
    m = len(profile)
    for i in range(n):
        a, b = vs[i], vs[(i + 1) % n]
        for k in range(m):
            bm.faces.new((a[k], a[(k + 1) % m], b[(k + 1) % m], b[k]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    return core.mesh_object(name, bm)


def strip(width, height, sunk=0.004):
    """A trim strip's section: rounded on top, its foot sunk into the surface it sits on."""
    w, h = width / 2, height
    return [(-w, -sunk), (-w, 0.4 * h), (-0.75 * w, 0.85 * h), (0.0, h), (0.75 * w, 0.85 * h), (w, 0.4 * h), (w, -sunk)]


def rear_ray(p):
    return (p[0], Y_R + 1.0, p[1]), (0.0, -1.0, 0.0)


def side_ray(sx):
    return lambda p: ((sx * 2.0, p[0], p[1]), (-sx, 0.0, 0.0))


def corner_spot(bvh, sx, cx, cy, z0, toward):
    """Places on the body round a corner (toward -1: front, +1: rear), found from inside: a ray out of the point
    (cx, cy, z0) at angle phi (degrees, 0 straight ahead/behind, 90 square to the side), its pitch nudged until it
    lands at height z. Lamps wrap round the corners this way, onto faces that slope back as steeply as a nose does."""
    o = Vector((sx * cx, cy, z0))

    def spot(p):
        a = math.radians(p[0])
        dh = Vector((sx * math.sin(a), toward * math.cos(a), 0.0))
        t, best = p[1], (None, None)
        for _ in range(6):
            q, nr = hit(bvh, o, dh * 0.45 + Vector((0.0, 0.0, t - z0)))
            if q is None:
                break
            best = (q, nr)
            reach = max(0.05, math.hypot(q.x - o.x, q.y - o.y))
            t = z0 + (p[1] - z0) * 0.45 / reach
        return best
    return spot


# Lamps, as (phi, z) outlines about their corner axes. The headlamp sweeps from the grille round onto the fender; the tail
# lamp from the trunk lid round onto the quarter panel.
HEAD_AXIS = (0.30, -2.05, 0.60)
HEAD = [(13, 0.695), (13.5, 0.765), (20, 0.788), (30, 0.802), (42, 0.81), (55, 0.813), (68, 0.812), (80, 0.808), (91, 0.802), (100, 0.795),
        (106, 0.786), (109, 0.774), (107, 0.762), (100, 0.752), (91, 0.738), (82, 0.72), (70, 0.70), (57, 0.68), (44, 0.668), (32, 0.666),
        (22, 0.672), (16, 0.682)]
TAIL_AXIS = (0.32, 2.05, 0.88)
TAIL = [(12, 0.80), (12, 0.985), (22, 1.005), (36, 1.015), (52, 1.02), (68, 1.016), (82, 1.006), (92, 0.99), (98, 0.968), (100, 0.945),
        (98, 0.905), (92, 0.88), (82, 0.86), (68, 0.835), (52, 0.81), (36, 0.795), (22, 0.79), (15, 0.792)]


def lamps(bvh, bvh0, mats):
    out = []
    seals = []
    for sx in (-1, 1):
        # Dense, so the lens's flat facets don't cut into the corner's curve. Each lens sits in a black gasket.
        out.append((lens_patch('headlamp', bvh, resample(HEAD, 72, 0.0075), corner_spot(bvh, sx, *HEAD_AXIS, -1), lift=0.0035, steps=LENS_STEPS,
                               seal=seals), mats['head']))
        out.append((lens_patch('taillamp', bvh, resample(TAIL, 72, 0.0075), corner_spot(bvh, sx, *TAIL_AXIS, 1), lift=0.0035, steps=LENS_STEPS,
                               seal=seals), mats['red']))
        # Side markers low on the bumper corners: amber at the front, red at the back.
        mk = [(y, z) for y, z in core.rounded_rect(-2.215, 0.585, -2.125, 0.612, 0.012, 3)]
        out.append((lens_patch('marker', bvh, mk, side_ray(sx), steps=(1.0, 0.5), seal=seals), mats['amber']))
        mk = [(y, z) for y, z in core.rounded_rect(2.14, 0.64, 2.235, 0.665, 0.011, 3)]
        out.append((lens_patch('marker', bvh, mk, side_ray(sx), steps=(1.0, 0.5), seal=seals), mats['red']))
        # Fog lamps in their pockets.
        fx, fz = FOG[0]
        p, nr = hit(bvh0, (sx * fx, Y_F - 1.0, fz), (0, 1, 0))
        if p is not None:
            out.append((parts.disc('fog', (sx * fx, p.y + 0.016, fz), (0, 1, 0), FOG_R - 0.004, 0.008, 32), mats['head']))
            out.append((closed_lathe('fog_ring', [(FOG_R - 0.008, -0.006), (FOG_R + 0.001, -0.006), (FOG_R + 0.001, 0.012), (FOG_R - 0.008, 0.012)],
                                     Matrix.Translation((sx * fx, p.y + 0.004, fz)) @ Matrix.Rotation(math.radians(90), 4, 'X'), 32), mats['chrome']))
    out += [(s, mats['trim']) for s in seals]
    return out


def front_end(bvh0, mats):
    """The grille (chrome surround and three bars over a black mesh), the intake's slats, the plate, the emblem."""
    out = []

    def surface_y(x, z):
        p, _ = hit(bvh0, (x, Y_F - 1.0, z), (0, 1, 0))
        return p.y if p is not None else Y_F

    # Surround: a chrome lip round the opening, on the bumper's face.
    pts, nrms = [], []
    for x, z in GRILLE:
        p, nr = hit(bvh0, (x, Y_F - 1.0, z), (0, 1, 0))
        pts.append(p)
        nrms.append(nr)
    out.append((ribbon('grille_ring', pts, nrms, strip(0.016, 0.006, 0.01), closed=True), mats['chrome']))
    # Three bars across the opening, set back in it and following the bumper's curve and slope.
    def bar(name, z, half, back, prof, m):
        pts, nrms = [], []
        for k in range(17):
            x = -half + 2 * half * k / 16
            p, nr = hit(bvh0, (x, Y_F - 1.0, z), (0, 1, 0))
            pts.append(p + Vector((0, back, 0)))
            nrms.append(nr)
        out.append((ribbon(name, pts, nrms, prof, sides=[(0, 0, 1)] * len(pts)), m))

    slat = [(-0.011, -0.006), (0.011, -0.006), (0.011, 0.003), (0.008, 0.006), (-0.008, 0.006), (-0.011, 0.003)]
    for z in (0.76, 0.712, 0.664):
        half = 0.385 * (1.0 - 0.15 * max(0.0, 0.712 - z) / 0.078) - 0.012
        bar('grille_bar', z, half, 0.016, slat, mats['chrome'])
    out.append((parts.disc('emblem', (0, surface_y(0, 0.712) + 0.004, 0.712), (0, 1, 0), 0.045, 0.012, 32), mats['chrome']))
    out[-1][0].data.transform(Matrix.Translation((0, 0, 0.712)) @ Matrix.Diagonal((1.0, 1.0, 0.62, 1.0)) @ Matrix.Translation((0, 0, -0.712)))
    # Intake slats.
    for z in (0.327, 0.364):
        bar('intake_slat', z, 0.41, 0.022, [(-0.006, -0.02), (0.006, -0.02), (0.006, 0.004), (-0.006, 0.004)], mats['trim'])
    # Front plate on its bracket.
    y = surface_y(0.0, PLATE_F[1])
    out.append((parts.rbox('plate_bracket', (0, y - 0.003, PLATE_F[1]), (0.33, 0.008, 0.17), 0.004), mats['trim']))
    out.append((parts.rbox('plate_f', (0, y - 0.009, PLATE_F[1]), (0.30, 0.006, 0.15), 0.003), mats['plate']))
    return out


def rear_end(bvh, mats):
    """The plate in its recess with its lamps, the trunk lid's chrome garnish and emblem, the exhaust."""
    out = []
    out.append((parts.rbox('plate_r', (0, Y_R - 0.012, PLATE_R[1]), (0.30, 0.006, 0.15), 0.003), mats['plate']))
    for sx in (-1, 1):
        out.append((parts.rbox('plate_lamp', (sx * 0.075, Y_R - 0.012, PLATE_R[1] + 0.082), (0.04, 0.012, 0.008), 0.003), mats['head']))
    # Garnish across the trunk lid between the tail lamps.
    pts, nrms = [], []
    for k in range(25):
        x = -0.40 + 0.80 * k / 24
        p, nr = hit(bvh, (x, Y_R + 1.0, 0.87), (0, -1, 0))
        pts.append(p)
        nrms.append(nr)
    out.append((ribbon('garnish', pts, nrms, [(-0.016, -0.004), (-0.016, 0.002), (-0.012, 0.004), (0.012, 0.004), (0.016, 0.002), (0.016, -0.004)],
                       sides=[(0, 0, 1)] * len(pts)), mats['chrome']))
    p, nr = hit(bvh, (0, Y_R + 1.0, 0.94), (0, -1, 0))
    if p is not None:
        squash = Matrix.Translation((0, 0, 0.94)) @ Matrix.Diagonal((1.0, 1.0, 0.62, 1.0)) @ Matrix.Translation((0, 0, -0.94))
        ring = closed_lathe('emblem', [(0.026, 0.0), (0.037, 0.0), (0.037, 0.004), (0.034, 0.007), (0.029, 0.007), (0.026, 0.004)],
                            Matrix.Translation((0, p.y - 0.001, 0.94)) @ Matrix.Rotation(math.radians(90), 4, 'X'), 40)
        ring.data.transform(squash)
        out.append((ring, mats['chrome']))
        field = parts.disc('emblem_field', (0, p.y - 0.002, 0.94), (0, 1, 0), 0.027, 0.004, 32)
        field.data.transform(squash)
        out.append((field, mats['trim']))
    # Red reflectors low on the bumper's corners.
    seals = []
    for sx in (-1, 1):
        rf = core.rounded_rect(sx * 0.63 - 0.075, 0.468, sx * 0.63 + 0.075, 0.49, 0.01, 3)
        out.append((lens_patch('reflector', bvh, rf, rear_ray, steps=(1.0, 0.5), seal=seals), mats['red']))
    out += [(s, mats['trim']) for s in seals]
    # One exhaust, under the right side of the bumper.
    out.append((closed_lathe('exhaust', [(0.026, -0.08), (0.034, -0.08), (0.036, 0.0), (0.03, 0.004), (0.026, 0.0)],
                             Matrix.Translation((-0.52, Y_R - 0.045, 0.305)) @ Matrix.Rotation(math.radians(-90), 4, 'X'), 24), mats['chrome']))
    out.append((parts.disc('exhaust_in', (-0.52, Y_R - 0.06, 0.305), (0, 1, 0), 0.027, 0.01, 24), mats['trim']))
    return out


def side_details(bvh, gh_bvh, mats):
    """Mirrors on the sails, door handles in their cups, the arch lips."""
    out = []
    for sx in (-1, 1):
        # Mirror: a D-section housing in body color (round in front, flat behind) on a black arm, growing taller and
        # deeper outboard, its top line rising and the whole swept back a little, the outer end squared off and rounded
        # over; the glass set in its back follows the taper.
        p, nr = hit(gh_bvh, (sx * 2.0, -0.82, 1.02), (-sx, 0, 0))
        x0 = abs(p.x) if p is not None else 0.79
        yb0, zc0 = -0.772, 1.060
        rings, glass = [], []
        for t in (0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.78, 0.84, 0.89, 0.93, 0.96, 0.98, 0.995, 1.0):
            x = x0 + 0.03 + 0.205 * t
            k = max(0.08, (1.0 - ((t - 0.76) / 0.24) ** 4) ** 0.25) if t > 0.76 else 1.0
            hz, d, rc = (0.044 + 0.026 * t) * k, (0.046 + 0.018 * t) * k, 0.012 * k
            zc, yb = zc0 + 0.014 * t, yb0 + 0.01 * t
            sec = [(yb - rc + d * math.cos(math.pi / 2 + math.pi * q / 12), zc + hz * math.sin(math.pi / 2 + math.pi * q / 12)) for q in range(13)]
            sec += [(yb - rc + rc * math.cos(a_), zc - hz + rc + rc * math.sin(a_)) for a_ in (-math.pi / 3, -math.pi / 6, 0.0)]
            sec += [(yb - rc + rc * math.cos(a_), zc + hz - rc + rc * math.sin(a_)) for a_ in (0.0, math.pi / 6, math.pi / 3)]
            rings.append([Vector((sx * x, y_, z_)) for y_, z_ in sec])
            # The glass stops short of the ends, its corners eased.
            g = (hz - 0.011) * (0.82 if t in (0.1, 0.96) else 1.0)
            if 0.03 < t < 0.97:
                glass.append((Vector((sx * x, yb + 0.0012, zc + g)), Vector((sx * x, yb + 0.0012, zc - g))))
        out.append((core.loft('mirror', rings), mats['paint']))
        bm = bmesh.new()
        vs = [(bm.verts.new(a), bm.verts.new(b)) for a, b in glass]
        for (a0, b0), (a1, b1) in zip(vs, vs[1:]):
            bm.faces.new((b0, b1, a1, a0))
        gl = core.mesh_object('mirror_glass', bm)
        core.orient_normals(gl, toward=(0, 1, 0))
        out.append((gl, mats['chrome']))
        out.append((parts.rbox('mirror_arm', (sx * (x0 + 0.018), -0.792, 1.035), (0.05, 0.045, 0.028), 0.01), mats['trim']))
        out.append((parts.rbox('mirror_base', (sx * (x0 - 0.004), -0.80, 1.03), (0.025, 0.11, 0.065), 0.01), mats['trim']))
        # Door handles, front and rear doors, under the crease, each in a dark cup.
        for hy in (0.03, 0.83):
            z = ZS(hy) - 0.045
            p, nr = hit(bvh, (sx * 2.0, hy, z), (-sx, 0, 0))
            if p is None:
                continue
            cup = [(hy + 0.085 * math.cos(2 * math.pi * q / 24), z - 0.004 + 0.024 * math.sin(2 * math.pi * q / 24)) for q in range(24)]
            out.append((lens_patch('handle_cup', bvh, cup, side_ray(sx), lift=0.0008, skirt=0.006, steps=(1.0, 0.5)), mats['trim']))
            rot = Vector((1, 0, 0)).rotation_difference(nr if sx > 0 else -nr).to_matrix().to_4x4()
            h = parts.rbox('handle', (0, 0, 0), (0.02, 0.19, 0.03), 0.0095)
            h.data.transform(Matrix.Translation(p + nr * 0.012) @ rot)
            out.append((h, mats['chrome']))
            if sx > 0 and hy < 0.5:
                out.append((parts.disc('lock', p + nr * 0.012 + Vector((0, -0.11, 0)), nr, 0.011, 0.016, 16), mats['chrome']))
        # The arch lips: a rolled edge round each wheel opening.
        for ay in AXLES:
            pts, nrms, sides = [], [], []
            for q in range(49):
                th = math.radians(-12 + 204 * q / 48)
                y, z = ay + ARCH_R * math.cos(th), WZ + 0.004 + ARCH_R * math.sin(th)
                if z < ZB(y) + 0.035:
                    continue
                pr = Vector((0.0, math.cos(th), math.sin(th)))
                p, nr = hit(bvh, (sx * 2.0, y + pr.y * 0.02, z + pr.z * 0.02), (-sx, 0, 0))
                if p is None:
                    continue
                pts.append(Vector((p.x, y, z)))
                nrms.append(Vector((nr.x, 0.0, nr.z * 0.5)))
                sides.append(pr)
            if len(pts) > 3:
                prof = [(-0.008, -0.014), (-0.006, 0.0), (0.0, 0.006), (0.012, 0.0075), (0.024, 0.0045), (0.034, -0.002), (0.03, -0.014)]
                out.append((ribbon('arch_lip', pts, nrms, prof, sides=sides), mats['paint']))
    return out


def window_trim(gh_bvh, body_bvh, drip, stations, mats):
    """Chrome round the side glass: along the drip line, down the C pillar's leading edge, along the belt."""
    out = []
    for sx in (1, -1):
        # Along the drip, from the A pillar's foot to where the C pillar's edge meets it.
        pts, nrms = [], []
        for (pr, pl), y in zip(drip, stations):
            p = pr if sx > 0 else pl
            if y < GH_Y0 + 0.015 or y > line_y(C_EDGE, p.z) + 0.004:
                continue
            q, nr = gh_bvh.find_nearest(p)[:2]
            # Sit on the rail just above the glass.
            up = Vector((0, 0, 1)) - nr * nr.z
            pts.append(q + up.normalized() * 0.006)
            nrms.append(nr)
        if len(pts) > 2:
            out.append((ribbon('drip_trim', pts, nrms, strip(0.011, 0.0035)), mats['chrome']))
        # Down the C pillar's leading edge.
        pts, nrms = [], []
        (y0, z0), (y1, z1) = C_EDGE
        for k in range(16):
            z = BELT(1.3) + 0.004 + (DRIP(0.86) - BELT(1.3)) * k / 15
            y = line_y(C_EDGE, z) + 0.008
            p, nr = hit(gh_bvh, (sx * 2.0, y, z), (-sx, 0, 0))
            if p is not None and abs(p.x) > 0.4:
                pts.append(p)
                nrms.append(nr)
        if len(pts) > 2:
            out.append((ribbon('c_trim', pts, nrms, strip(0.011, 0.0035)), mats['chrome']))
        # The belt molding along the foot of the glass.
        pts, nrms = [], []
        for k in range(40):
            y = GH_Y0 + 0.02 + (line_y(C_EDGE, BELT(1.3)) - 0.02 - GH_Y0 - 0.02) * k / 39
            p, nr = body_bvh.find_nearest(Vector((sx * (GH_XB(y) + 0.007), y, BELT(y) - 0.002)))[:2]
            pts.append(p)
            nrms.append(nr)
        out.append((ribbon('belt_trim', pts, nrms, strip(0.013, 0.005)), mats['chrome']))
    return out


def cowl_and_wipers(bvh, gh_bvh, mats):
    """The black cowl panel between the hood and the windshield, and the wipers parked along the glass's foot."""
    out = []

    def ws_base(x):
        return GH_Y0 + BOW(GH_Y0) * max(0.0, 1.0 - (x / GH_XB(GH_Y0)) ** 2)

    outline = []
    for k in range(13):
        x = -0.70 + 1.40 * k / 12
        outline.append((x, ws_base(x) - 0.095))
    for k in range(13):
        x = 0.70 - 1.40 * k / 12
        outline.append((x, ws_base(x) - 0.012))
    out.append((lens_patch('cowl', bvh, outline, lambda p: ((p[0], p[1], 3.0), (0, 0, -1)), lift=0.004, skirt=0.01, steps=(1.0, 0.5)), mats['trim']))
    for x0, x1 in ((-0.06, 0.60), (-0.66, -0.12)):
        pts, nrms = [], []
        for k in range(12):
            x = x0 + (x1 - x0) * k / 11
            y = ws_base(x) + 0.05
            p, nr = hit(gh_bvh, (x, y, 3.0), (0, 0, -1))
            if p is not None:
                pts.append(p + nr * 0.012)
                nrms.append(nr)
        if len(pts) > 2:
            out.append((ribbon('wiper', pts, nrms, [(-0.008, -0.008), (0.008, -0.008), (0.006, 0.006), (-0.006, 0.006)]), mats['trim']))
            piv = Vector((x0 + 0.04 if x0 > -0.3 else x1 - 0.04, ws_base(x0) - 0.04, 0.0))
            p, nr = hit(bvh, (piv.x, piv.y, 3.0), (0, 0, -1))
            if p is not None:
                mid = pts[len(pts) // 2]
                out.append((parts.rod('wiper_arm', p + Vector((0, 0, 0.012)), mid + nr * 0.008, 0.007, 8), mats['trim']))
                out.append((parts.disc('wiper_pivot', p + Vector((0, 0, 0.01)), (0, 0, 1), 0.016, 0.02, 12), mats['trim']))
    return out


# ------------------------------------------------------------------ the cabin (baked into the glass)

def cabin(mats):
    """Seats, headrests, the dash and wheel, door cards, the parcel shelf: seen faintly through the tint. Only baked into
    the glass (the game's glass is opaque), then thrown away."""
    def mat(name, hex_color, rough=0.85, glow=0.0):
        m = core.Mat(name)
        m.set('Base Color', core.hex_linear(hex_color))
        m.set('Roughness', rough)
        if glow:
            m.set('Emission Color', core.hex_linear(hex_color))
            m.set('Emission Strength', glow)
        return m

    cloth = mat('cab_cloth', 0x3f4144)
    dark = mat('cab_dark', 0x1d1e20, 0.6)
    card = mat('cab_card', 0x333437, 0.7)
    liner = mat('cab_liner', 0x5e5c58)
    floor_m = mat('cab_floor', 0x232324)
    lamp = mat('cab_lamp', 0x6a0c0c, 0.3)
    out = []

    def box(name, c, size, r, m, tilt=0.0):
        o = parts.rbox(name, (0, 0, 0), size, r)
        o.data.transform(Matrix.Translation(c) @ Matrix.Rotation(math.radians(tilt), 4, 'X'))
        core.assign(o, m)
        out.append(o)

    for sx in (-1, 1):
        x = sx * 0.37
        box('seat', (x, -0.22, 0.50), (0.50, 0.52, 0.13), 0.05, cloth, -6)
        box('seat_back', (x, 0.02, 0.82), (0.50, 0.13, 0.62), 0.06, cloth, -14)
        box('headrest', (x, 0.10, 1.20), (0.27, 0.11, 0.19), 0.05, cloth, -10)
        box('door_card', (sx * 0.735, -0.38, 0.76), (0.05, 1.05, 0.48), 0.02, card)
        box('door_card', (sx * 0.73, 0.78, 0.78), (0.05, 0.86, 0.46), 0.02, card)
        box('armrest', (sx * 0.70, -0.30, 0.80), (0.08, 0.40, 0.05), 0.02, dark)
        box('rear_headrest', (sx * 0.46, 1.10, 1.06), (0.25, 0.10, 0.15), 0.045, cloth, -18)
    box('rear_seat', (0, 0.66, 0.49), (1.34, 0.50, 0.15), 0.06, cloth, -5)
    box('rear_back', (0, 0.98, 0.76), (1.34, 0.15, 0.58), 0.06, cloth, -22)
    box('rear_headrest', (0, 1.10, 1.03), (0.22, 0.09, 0.12), 0.04, cloth, -18)
    box('console', (0, -0.30, 0.56), (0.24, 0.85, 0.26), 0.04, dark)
    box('dash', (0, -0.78, 0.84), (1.46, 0.42, 0.26), 0.07, dark)
    box('dash_top', (0, -0.86, 0.95), (1.40, 0.30, 0.06), 0.03, dark, 12)
    box('binnacle', (0.37, -0.64, 1.0), (0.38, 0.16, 0.07), 0.03, dark)
    box('floor', (0, 0.15, 0.30), (1.50, 2.5, 0.04), 0.01, floor_m)
    box('parcel_shelf', (0, 1.38, 0.985), (1.40, 0.50, 0.03), 0.01, card)
    box('brake_lamp', (0, 1.56, 1.012), (0.32, 0.05, 0.03), 0.01, lamp)
    # The mirror on the windshield and the visors folded up against the headliner.
    box('rv_mirror', (0, -0.29, 1.285), (0.25, 0.035, 0.068), 0.02, dark, 8)
    box('rv_stem', (0, -0.31, 1.33), (0.025, 0.025, 0.06), 0.008, dark)
    for sx in (-1, 1):
        box('visor', (sx * 0.36, -0.13, 1.352), (0.38, 0.17, 0.024), 0.01, liner, 6)
    # The steering wheel, tilted toward the driver.
    ring = [Vector((0.37 + 0.185 * math.cos(2 * math.pi * k / 32), 0.0, 0.185 * math.sin(2 * math.pi * k / 32))) for k in range(32)]
    wheel_o = core.sweep('steering', ring, parts.circle(0.017, 8), closed=True)
    wheel_o.data.transform(Matrix.Translation((0.0, -0.50, 0.92)) @ Matrix.Rotation(math.radians(25), 4, 'X'))
    core.assign(wheel_o, dark)
    out.append(wheel_o)
    return out


def bake_cabin(obj, mats):
    """Bakes what each pane of glass looks onto straight through it (the cabin, lit through the windows), into an image
    the glass then shows faintly under its tint: a selected-to-active bake from the cabin's parts onto a copy of the
    glass, its rays cast inward along each pane's normal."""
    sc = bpy.context.scene
    gi = mats['index']['car_glass']
    stuff = cabin(mats)
    me = obj.data.copy()
    bm = bmesh.new()
    bm.from_mesh(me)
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if f.material_index != gi], context='FACES')
    bm.to_mesh(me)
    bm.free()
    dup = core.link(bpy.data.objects.new('glass_bake', me))
    size = TEXTURE_SIZES['car_glass']
    img = bpy.data.images.new('T_car_cabin_view', size, size, alpha=False, float_buffer=True)
    # The copy lets the light through (it is only where the rays start from).
    see = core.Mat('cabin_see')
    tr = see.node('ShaderNodeBsdfTransparent')
    see.link(tr.outputs[0], see.out.inputs['Surface'])
    tex = see.nt.nodes.new('ShaderNodeTexImage')
    tex.image = img
    see.nt.nodes.active = tex
    me.materials.clear()
    me.materials.append(see.m)
    for p in me.polygons:
        p.material_index = 0
    clear = core.Mat('cabin_clear')
    tr2 = clear.node('ShaderNodeBsdfTransparent')
    clear.link(tr2.outputs[0], clear.out.inputs['Surface'])
    keep = obj.data.materials[gi]
    obj.data.materials[gi] = clear.m
    # An even, overcast light from every side (through the glass, which is clear for this).
    world = sc.world
    bg = next((n for n in world.node_tree.nodes if n.type == 'BACKGROUND'), None) if world.node_tree else None
    world_color = tuple(world.color)
    world.color = (0.55, 0.57, 0.6)
    if bg is not None:
        bg_keep = (tuple(bg.inputs['Color'].default_value), bg.inputs['Strength'].default_value)
        bg.inputs['Color'].default_value = (0.55, 0.57, 0.6, 1.0)
        bg.inputs['Strength'].default_value = 1.0
    sc.cycles.samples = 96
    sc.render.bake.margin = 6
    sc.render.bake.use_clear = True
    sc.cycles.use_auto_tile = False
    bk = sc.render.bake
    bk.use_selected_to_active = True
    bk.use_cage = False
    bk.cage_extrusion = 0.02
    bk.max_ray_distance = 2.5
    core.select_only(stuff + [dup], dup)
    try:
        bpy.ops.object.bake(type='COMBINED')
    finally:
        bk.use_selected_to_active = False
        bk.cage_extrusion = 0.0
        bk.max_ray_distance = 0.0
    obj.data.materials[gi] = keep
    world.color = world_color
    if bg is not None:
        bg.inputs['Color'].default_value = bg_keep[0]
        bg.inputs['Strength'].default_value = bg_keep[1]
    bpy.data.objects.remove(dup)
    for o in stuff:
        bpy.data.objects.remove(o)
    # Seen through tint the cabin is soft anyway: a light blur (inside the bake's margin) also takes out its grain.
    w, h = img.size
    a = np.empty(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(a)
    a = a.reshape(h, w, 4)
    for _ in range(2):
        a = (a + np.roll(a, 1, 0) + np.roll(a, -1, 0)) / 3.0
        a = (a + np.roll(a, 1, 1) + np.roll(a, -1, 1)) / 3.0
    img.pixels.foreach_set(a.ravel())
    img.update()
    return img


# ------------------------------------------------------------------ prints

def plate_sheet():
    s = Sheet(300, 150)
    s.rect(0, 0, 300, 150, 0xf2f1ec, rough=0.3)
    s.rect(4, 4, 292, 142, 0x1e3a6e, rough=0.3)
    s.rect(8, 8, 284, 134, 0xf2f1ec, rough=0.3)
    s.text('NORTHSIDE', 150, 118, 22, 0xb5262f, face='Black', align='CENTER', tracking=1.3, rough=0.3)
    s.text('7FTH 212', 150, 40, 62, 0x1e3a6e, face='Black', align='CENTER', rough=0.3)
    # Not "the river city": River City is the town down the line where the Classic is played.
    s.text('OPEN ALL NIGHT', 150, 14, 13, 0x1e3a6e, face='Bold', align='CENTER', tracking=1.3, rough=0.3)
    # The two bolts that hold it, a little rust bled round them; the registration sticker in the corner.
    for bx in (24, 276):
        s.ring(bx, 131, 5.5, 8.5, 0x8c7a62, rough=0.7, alpha=0.45)
        s.circle(bx, 131, 5.5, 0xb4b7ba, metal=0.9, rough=0.3)
        s.rect(bx - 3.8, 130.2, 7.6, 1.6, 0x45484b, metal=0.5, rough=0.5)
    s.rect(238, 11, 42, 21, 0xe0b422, rough=0.35)
    s.text('10', 249, 16, 11, 0x1c1c1c, face='Black', align='CENTER', rough=0.35)
    s.text('27', 270, 16, 11, 0x1c1c1c, face='Black', align='CENTER', rough=0.35)
    return s.render('car_plate', 1024)


def tire_sheet():
    """The sidewall's lettering, one of two repeats round the tire (white on black: a mask)."""
    s = Sheet(1000, 46)
    s.rect(0, 0, 1000, 46, 0x000000)
    s.text('NORTHLINE', 170, 14, 21, 0xffffff, face='Black', align='CENTER', tracking=1.25)
    s.text('TOURING A/S', 470, 17, 13, 0xffffff, face='Bold', align='CENTER', tracking=1.15)
    s.text('225/50R17  94V', 760, 17, 13, 0xffffff, face='Bold', align='CENTER', tracking=1.1)
    s.rect(330, 21, 20, 2.5, 0xffffff)
    s.rect(605, 21, 20, 2.5, 0xffffff)
    s.text('M+S', 920, 18, 10, 0xffffff, face='Bold', align='CENTER')
    return s.render('car_tire_text', 4096)[0]


LINE_W = 0.0036


def line_sheets():
    """The shut lines, drawn in four views (white lines on black) and projected onto the paint by the way it faces."""
    def stroke(s, pts, scale, off, w=LINE_W):
        q = [((a + off[0]) * scale, (b + off[1]) * scale) for a, b in pts]
        hw = w * scale / 2
        for (x0, y0), (x1, y1) in zip(q, q[1:]):
            L = math.hypot(x1 - x0, y1 - y0)
            if L < 1e-9:
                continue
            nx, ny = -(y1 - y0) / L * hw, (x1 - x0) / L * hw
            s.poly([(x0 + nx, y0 + ny), (x1 + nx, y1 + ny), (x1 - nx, y1 - ny), (x0 - nx, y0 - ny)], 0xffffff)
        for x, y in q:
            s.circle(x, y, hw, 0xffffff, n=12)

    def arc(cy, cz, r, a0, a1, n=24):
        return [(cy + r * math.cos(math.radians(a0 + (a1 - a0) * k / n)), cz + r * math.sin(math.radians(a0 + (a1 - a0) * k / n))) for k in range(n + 1)]

    paths = {}
    # Side views (y, z), y from -2.5, z from 0; 1 unit = 1 mm.
    side_lines = [
        [(-0.938, 0.965), (-0.94, 0.80), (-0.946, 0.55), (-0.952, 0.268)],                     # front door, leading edge
        [(-0.952, 0.268), (0.924, 0.268)],                                                     # door bottoms
        [(0.246, 0.985), (0.246, 0.268)],                                                      # between the doors
        [(1.296, 1.002), (1.283, 0.77)] + arc(AXLES[1], WZ, 0.456, 102.7, 180.0) + [(0.924, 0.268)],   # rear door round the arch
        [(-1.875, 0.758), (-1.86, 0.66), (-1.79, 0.535)],                                      # front bumper cover / fender
        [(2.135, 0.885), (2.02, 0.70), (1.75, 0.53)],                                          # rear bumper cover / quarter
    ]
    for key, fuel in (('side_l', False), ('side_r', True)):
        s = Sheet(5000, 1500)
        s.rect(0, 0, 5000, 1500, 0x000000)
        for ln in side_lines:
            stroke(s, ln, 1000, (2.5, 0.0))
        if fuel:
            fd = core.rounded_rect(1.62, 0.745, 1.79, 0.865, 0.022, 4)
            stroke(s, fd + [fd[0]], 1000, (2.5, 0.0))
        paths[key] = s.render(f'car_lines_{key}', 4096)[0]
    # Top view (x, y): the hood and the trunk lid.
    s = Sheet(2000, 5000)
    s.rect(0, 0, 2000, 5000, 0x000000)

    def ws_base(x):
        return GH_Y0 + BOW(GH_Y0) * max(0.0, 1.0 - (x / GH_XB(GH_Y0)) ** 2)

    def bl_base(x):
        return GH_Y1 + BOW(GH_Y1) * max(0.0, 1.0 - (x / GH_XB(GH_Y1)) ** 2)

    hood_rear = [(x, ws_base(x) - 0.095) for x in [-0.70 + 1.4 * k / 28 for k in range(29)]]
    stroke(s, hood_rear, 1000, (1.0, 2.5))
    for sx in (-1, 1):
        stroke(s, [(sx * 0.70, ws_base(0.70) - 0.095), (sx * 0.682, -1.5), (sx * 0.652, -1.95), (sx * 0.615, -2.15), (sx * 0.55, -2.245),
                   (sx * 0.42, -2.29), (sx * 0.22, -2.312), (0.0, -2.318)], 1000, (1.0, 2.5))
    trunk = [(x, bl_base(x) + 0.05) for x in [-0.655 + 1.31 * k / 26 for k in range(27)]]
    stroke(s, trunk, 1000, (1.0, 2.5))
    for sx in (-1, 1):
        stroke(s, [(sx * 0.655, bl_base(0.655) + 0.05), (sx * 0.655, 2.0), (sx * 0.64, 2.25), (sx * 0.60, 2.36)], 1000, (1.0, 2.5))
    paths['top'] = s.render('car_lines_top', 1640)[0]
    # Front (x, z): the hood's leading edge over the grille.
    s = Sheet(2000, 1500)
    s.rect(0, 0, 2000, 1500, 0x000000)
    stroke(s, [(-0.40, 0.80), (-0.2, 0.804), (0.0, 0.805), (0.2, 0.804), (0.40, 0.80)], 1000, (1.0, 0.0))
    paths['front'] = s.render('car_lines_front', 1640)[0]
    # Rear (x, z): the trunk lid's lower edge and its sides up past the tail lamps.
    s = Sheet(2000, 1500)
    s.rect(0, 0, 2000, 1500, 0x000000)
    stroke(s, [(-0.40, 0.99), (-0.405, 0.80), (0.405, 0.80), (0.40, 0.99), (0.42, 1.035)], 1000, (1.0, 0.0))
    stroke(s, [(-0.40, 0.99), (-0.42, 1.035)], 1000, (1.0, 0.0))
    paths['rear'] = s.render('car_lines_rear', 1640)[0]
    return paths


# ------------------------------------------------------------------ materials

def sample_mask(m, path, u, v):
    n = m.image(path, non_color=True, vector=vec(m, u, v, 0.0), extension='CLIP')
    sep = m.node('ShaderNodeSeparateColor')
    m.link(n.outputs['Color'], sep.inputs['Color'])
    return sep.outputs['Red']


def normal_parts(m):
    geo = m.node('ShaderNodeNewGeometry')
    sep = m.node('ShaderNodeSeparateXYZ')
    m.link(geo.outputs['Normal'], sep.inputs['Vector'])
    return sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']


def paint_material(lines):
    """Light grey metallic, glossy as a clear coat (the game tints it), the shut lines cut into it, wet-road film and the
    wheels' spray along the sills, grime in the crevices, stone chips where the road throws them."""
    m = core.Mat('car_paint')
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    nx, ny, nz = normal_parts(m)
    flakes = noise(m, o, scale=900.0, detail=1.0)
    col = m.mix(m.math('MULTIPLY', flakes, 0.3), core.hex_linear(0xd8dadc), core.hex_linear(0xe6e8ea))
    # Shut lines from the four views, each where the paint faces that way.
    u_side, v_side = m.math('DIVIDE', m.math('ADD', y, 2.5), 5.0), m.math('DIVIDE', z, 1.5)
    left = sample_mask(m, lines['side_l'], u_side, v_side)
    right = sample_mask(m, lines['side_r'], u_side, v_side)
    side = mix_float(m, m.math('GREATER_THAN', x, 0.0), right, left)
    side = m.math('MULTIPLY', side, ramp(m, m.math('ABSOLUTE', nx), 0.4, 0.62))
    top = m.math('MULTIPLY', sample_mask(m, lines['top'], m.math('DIVIDE', m.math('ADD', x, 1.0), 2.0), m.math('DIVIDE', m.math('ADD', y, 2.5), 5.0)),
                 ramp(m, nz, 0.4, 0.62))
    ux = m.math('DIVIDE', m.math('ADD', x, 1.0), 2.0)
    front = m.math('MULTIPLY', sample_mask(m, lines['front'], ux, v_side), ramp(m, m.math('MULTIPLY', ny, -1.0), 0.35, 0.6))
    rear = m.math('MULTIPLY', sample_mask(m, lines['rear'], ux, v_side), ramp(m, ny, 0.35, 0.6))
    lines_m = m.math('MAXIMUM', m.math('MAXIMUM', side, top), m.math('MAXIMUM', front, rear))
    # Road film: rising from the sills, heaviest in the spray thrown back off each tire.
    blot = noise(m, o, scale=7.0, detail=4.0)
    streak = noise(m, scaled(m, sep, 3.0, 9.0, 40.0), detail=3.0)
    film = m.math('MULTIPLY', ramp(m, z, 0.5, 0.17), ramp(m, blot, 0.3, 0.75))
    spray = None
    for ay in AXLES:
        d = m.math('SUBTRACT', y, ay)
        behind = m.math('MULTIPLY', ramp(m, d, 0.25, 0.42), ramp(m, d, 1.0, 0.55))
        sp = m.math('MULTIPLY', m.math('MULTIPLY', behind, ramp(m, z, 0.58, 0.2)), ramp(m, streak, 0.35, 0.7))
        spray = sp if spray is None else m.math('MAXIMUM', spray, sp)
    dirt = m.math('MINIMUM', m.math('ADD', m.math('MULTIPLY', film, 0.55), m.math('MULTIPLY', spray, 0.6)), 1.0)
    # Grime settled where the paint tucks under something: round the lamps' gaskets, the mirror feet, the handle cups,
    # the cowl and the foot of the glass, inside the arch lips.
    ao = m.node('ShaderNodeAmbientOcclusion', Distance=0.05)
    ao.samples = 16
    ao.only_local = True
    crevice = m.math('MULTIPLY', ramp(m, m.math('SUBTRACT', 1.0, ao.outputs['AO']), 0.2, 0.65), ramp(m, noise(m, o, scale=40.0, detail=3.0), 0.3, 0.65))
    # Stone chips: the nose, the hood's leading edge, and the sills behind the front wheels.
    vor = m.node('ShaderNodeTexVoronoi', Scale=70.0)
    m.link(o, vor.inputs['Vector'])
    pick = m.node('ShaderNodeSeparateColor')
    m.link(vor.outputs['Color'], pick.inputs['Color'])
    d_front = m.math('SUBTRACT', y, AXLES[0])
    zone = m.math('MAXIMUM', m.math('MULTIPLY', ramp(m, m.math('MULTIPLY', ny, -1.0), 0.45, 0.7), ramp(m, z, 0.95, 0.8)),
                  m.math('MULTIPLY', ramp(m, y, -2.12, -2.26), ramp(m, nz, 0.3, 0.55)))
    zone = m.math('MAXIMUM', zone, m.math('MULTIPLY', m.math('MULTIPLY', ramp(m, d_front, 0.36, 0.46), ramp(m, d_front, 1.0, 0.8)),
                                          m.math('MULTIPLY', ramp(m, z, 0.42, 0.3), ramp(m, m.math('ABSOLUTE', nx), 0.5, 0.7))))
    chips = m.math('MULTIPLY', m.math('MULTIPLY', smooth_less(m, vor.outputs['Distance'], 0.16, 0.04), ramp(m, pick.outputs['Red'], 0.9, 0.92)), zone)
    # Smudges and dried drops on the clear coat show in its reflections, not its color.
    smudge = ramp(m, noise(m, o, scale=4.0, detail=5.0), 0.4, 0.75)
    col = m.mix(m.math('MULTIPLY', crevice, 0.5), col, core.hex_linear(0x4a443b))
    col = m.mix(m.math('MULTIPLY', dirt, 0.85), col, core.hex_linear(0x4d4840))
    col = m.mix(chips, col, core.hex_linear(0x5d5b57))
    col = m.mix(lines_m, col, core.hex_linear(0x0a0a0b))
    m.set('Base Color', col)
    # The glTF material has no clear coat, so the paint's one layer carries it: dielectric enough to keep a white
    # reflection, smooth enough to keep a sharp one (the flakes only tint it).
    rough = m.math('ADD', 0.07, m.math('ADD', m.math('MULTIPLY', flakes, 0.02), m.math('MULTIPLY', smudge, 0.05)))
    rough = mix_float(m, m.math('MULTIPLY', crevice, 0.7), rough, 0.45)
    rough = mix_float(m, dirt, rough, 0.6)
    rough = mix_float(m, chips, rough, 0.55)
    m.set('Roughness', rough)
    bare = m.math('MAXIMUM', m.math('MAXIMUM', dirt, lines_m), chips)
    m.set('Metallic', mix_float(m, bare, m.math('ADD', 0.2, m.math('MULTIPLY', flakes, 0.1)), 0.03))
    bump = m.node('ShaderNodeBump', Strength=0.7, Distance=0.002)
    m.link(m.math('SUBTRACT', m.math('SUBTRACT', 1.0, lines_m), m.math('MULTIPLY', chips, 0.35)), bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def vmath(m, op, a, b=None):
    """A Vector Math node: sockets or constant vectors in; the Value output for dot products and lengths."""
    n = m.node('ShaderNodeVectorMath')
    n.operation = op
    for i, v in enumerate((a, b)):
        if v is None:
            continue
        if isinstance(v, (tuple, list, Vector)):
            n.inputs[i].default_value = tuple(v)
        else:
            m.link(v, n.inputs[i])
    return n.outputs['Value'] if op in ('DOT_PRODUCT', 'DISTANCE', 'LENGTH') else n.outputs['Vector']


def surface_frame(bvh, axis, toward, phi, z, lift=0.0035):
    """A point on a lamp's lens, found as the lens was (corner_spot), with the tangents there: across (level) and up
    the surface. Features drawn round it are measured in 3D on the lens, so they come out round however it curves."""
    p, n = corner_spot(bvh, 1, *axis, toward)((phi, z))
    n = n.normalized()
    t1 = Vector((0.0, 0.0, 1.0)).cross(n).normalized()
    t2 = n.cross(t1).normalized()
    return p + n * lift, t1, t2


def lens_point(m):
    """(Separate XYZ, |x|, the point folded onto the right side): one set of features serves both lamps."""
    tc, sep = object_coords(m)
    ax = m.math('ABSOLUTE', sep.outputs['X'])
    return sep, ax, vec(m, ax, sep.outputs['Y'], sep.outputs['Z'])


def polar(m, P, frame):
    """Distance from the frame's point in its tangent plane, and the angle round it."""
    c, t1, t2 = frame
    d = vmath(m, 'SUBTRACT', P, c)
    a = vmath(m, 'DOT_PRODUCT', d, t1)
    b = vmath(m, 'DOT_PRODUCT', d, t2)
    r = m.math('SQRT', m.math('ADD', m.math('MULTIPLY', a, a), m.math('MULTIPLY', b, b)))
    return r, m.math('ARCTAN2', b, a)


def dish(m, r, R, depth):
    """A paraboloid: depth below the rim at the center (negative depth: a dome), level from R out."""
    t = m.math('DIVIDE', r, R, clamp=True)
    return m.math('MULTIPLY', m.math('SUBTRACT', 1.0, m.math('MULTIPLY', t, t)), -depth)


def flutes(m, r, th, R, count, amp):
    """Facets round a reflector: count lobes, rising from nothing at the center to amp at the rim."""
    lobe = m.math('ABSOLUTE', m.math('SINE', m.math('MULTIPLY', th, count / 2.0)))
    return m.math('MULTIPLY', m.math('MULTIPLY', lobe, m.math('DIVIDE', r, R, clamp=True)), amp)


def ripples(m, d, period, amp):
    return m.math('MULTIPLY', m.math('ABSOLUTE', m.math('SINE', m.math('MULTIPLY', d, math.pi / period))), amp)


def pillows(m, P, period, amp):
    """Optics molded into a lens or reflector: a field of small cushions (cut through by the lens's own curve)."""
    s = vmath(m, 'MULTIPLY', P, (math.pi / period,) * 3)
    sep = m.node('ShaderNodeSeparateXYZ')
    m.link(s, sep.inputs['Vector'])
    tot = None
    for k in ('X', 'Y', 'Z'):
        w = m.math('ABSOLUTE', m.math('SINE', sep.outputs[k]))
        tot = w if tot is None else m.math('ADD', tot, w)
    return m.math('MULTIPLY', tot, amp / 3.0)


def band(m, v, lo, hi, soft):
    """1 between lo and hi (soft edges)."""
    return m.math('MULTIPLY', ramp(m, v, lo - soft, lo + soft), m.math('SUBTRACT', 1.0, ramp(m, v, hi - soft, hi + soft)))


def layered(m, base, layers):
    """Folds (mask, color hex, metallic, roughness) layers over a base (color hex, metallic, roughness), in order."""
    col, met, rou = core.hex_linear(base[0]), base[1], base[2]
    for mask, c, mt, r in layers:
        col = m.mix(mask, col, core.hex_linear(c))
        met = mix_float(m, mask, met, mt)
        rou = mix_float(m, mask, rou, r)
    return col, met, rou


def sum_of(m, terms):
    tot = None
    for t in terms:
        tot = t if tot is None else m.math('ADD', tot, t)
    return tot


def masked(m, mask, h):
    return m.math('MULTIPLY', mask, h)


def bumped(m, height):
    """The height (meters) as the surface's normal."""
    bump = m.node('ShaderNodeBump', Strength=1.0, Distance=1.0)
    m.link(height, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])


def edge_map(name, outline, phi0, phi1, z0, z1, scale, w=512, h=128, reach=0.02):
    """How deep inside a lamp's (phi, z) outline each point lies, 0 at the edge to 1 at reach (meters, phi taken as
    arc length at scale), as a small grey image over [phi0, phi1] x [z0, z1]: the housing falls into shadow toward
    the body's opening, which is what makes a lamp read as deep behind its lens."""
    P = np.array([(math.radians(p) * scale, z) for p, z in outline])
    U, Z = np.meshgrid(np.linspace(math.radians(phi0) * scale, math.radians(phi1) * scale, w), np.linspace(z0, z1, h))
    d = np.full(U.shape, 1e9)
    inside = np.zeros(U.shape, dtype=bool)
    for i in range(len(P)):
        a, b = P[i], P[(i + 1) % len(P)]
        ab = b - a
        t = np.clip(((U - a[0]) * ab[0] + (Z - a[1]) * ab[1]) / max(float(ab @ ab), 1e-12), 0.0, 1.0)
        d = np.minimum(d, np.hypot(U - a[0] - t * ab[0], Z - a[1] - t * ab[1]))
        # Even-odd rule: a ray along +u crosses this edge?
        if abs(ab[1]) > 1e-12:
            inside ^= ((a[1] > Z) != (b[1] > Z)) & (U < a[0] + (Z - a[1]) * ab[0] / ab[1])
    val = np.where(inside, np.clip(d / reach, 0.0, 1.0), 0.0).astype(np.float32)
    px = np.ones((h, w, 4), dtype=np.float32)
    px[..., 0] = px[..., 1] = px[..., 2] = val
    img = bpy.data.images.new(name, w, h, alpha=False)
    img.pixels.foreach_set(px.ravel())
    path = os.path.join(core.BUILD_DIR, 'textures', f'{name}.png')
    os.makedirs(os.path.dirname(path), exist_ok=True)
    img.filepath_raw = path
    img.file_format = 'PNG'
    img.save()
    bpy.data.images.remove(img)
    return path


def edge_depth(m, phi, z, outline, phi0, phi1, z0, z1, scale, name):
    """The edge_map sampled at the shading point (0 at the lens's edge, 1 well inside it)."""
    path = edge_map(name, outline, phi0, phi1, z0, z1, scale)
    u = m.math('DIVIDE', m.math('SUBTRACT', phi, phi0), phi1 - phi0)
    v = m.math('DIVIDE', m.math('SUBTRACT', z, z0), z1 - z0)
    n = m.image(path, non_color=True, vector=vec(m, u, v, 0.0), extension='EXTEND')
    sep = m.node('ShaderNodeSeparateColor')
    m.link(n.outputs['Color'], sep.inputs['Color'])
    return sep.outputs['Red']


def headlamp_shading(m, bvh):
    """The headlamp behind its clear lens: a silver-painted housing; the high beam's deep chrome bowl, faceted round
    its bulb shield; the projector (a dark lens domed in a bright ring, a black shroud, its own fluted chrome bowl);
    the position lamp's and the amber turn lamp's textured reflectors where the lamp runs back along the fender; a
    dark bezel with a chrome bead along the bottom. Real depth would need a clear lens the game can't show, so the
    bowls and domes are in the normal map, and the reflectors catch the street's light the way real ones do: in
    patches. The fog lamps (low on the bumper) and the plate lamps (behind) share the material."""
    sep, ax, P = lens_point(m)
    y, z = sep.outputs['Y'], sep.outputs['Z']
    phi = m.math('MULTIPLY', m.math('ARCTAN2', m.math('SUBTRACT', ax, HEAD_AXIS[0]), m.math('SUBTRACT', HEAD_AXIS[1], y)), 180.0 / math.pi)
    r1, th1 = polar(m, P, surface_frame(bvh, HEAD_AXIS, -1, 27, 0.733))
    r2, th2 = polar(m, P, surface_frame(bvh, HEAD_AXIS, -1, 50, 0.743))
    soft = 0.0008
    hb = smooth_less(m, r1, 0.041, soft)
    cap = smooth_less(m, r1, 0.011, soft)
    pj_bowl = smooth_less(m, r2, 0.053, soft)
    pj_shroud = smooth_less(m, r2, 0.039, soft)
    pj_ring = smooth_less(m, r2, 0.031, soft)
    pj_lens = smooth_less(m, r2, 0.0245, soft)
    amber = ramp(m, phi, 100.5, 102.0)
    pos = m.math('MULTIPLY', ramp(m, phi, 64.0, 65.5), m.math('SUBTRACT', 1.0, amber))
    inner = ramp(m, phi, 17.8, 16.8)
    outer_end = ramp(m, phi, 72.0, 74.0)
    bezel = m.math('MAXIMUM', m.math('MULTIPLY', ramp(m, z, 0.6895, 0.6875), m.math('SUBTRACT', 1.0, outer_end)), inner)
    bead = m.math('MULTIPLY', band(m, z, 0.6895, 0.6935, 0.0005), m.math('SUBTRACT', 1.0, m.math('MAXIMUM', outer_end, inner)))
    fog = m.math('MAXIMUM', ramp(m, z, 0.5, 0.48), m.math('GREATER_THAN', y, 0.0))
    plate_lamp = m.math('GREATER_THAN', y, 0.0)
    # The fog lamps: a fluted chrome bowl round a bulb, square to the bumper.
    fx = m.math('SUBTRACT', ax, FOG[0][0])
    fz = m.math('SUBTRACT', z, FOG[0][1])
    df = m.math('SQRT', m.math('ADD', m.math('MULTIPLY', fx, fx), m.math('MULTIPLY', fz, fz)))
    fog_cap = m.math('MULTIPLY', smooth_less(m, df, 0.009, 0.0008), m.math('SUBTRACT', 1.0, plate_lamp))
    col, met, rou = layered(m, (0x8d9298, 0.45, 0.2), [
        (pos, 0xb9bec4, 0.8, 0.24),
        (amber, 0xc8761a, 0.55, 0.16),
        (pj_bowl, 0xd3d7db, 1.0, 0.07),
        (pj_shroud, 0x17181b, 0.0, 0.42),
        (pj_ring, 0xe2e5e8, 1.0, 0.1),
        (pj_lens, 0x0a0d12, 0.0, 0.03),
        (hb, 0xd3d7db, 1.0, 0.07),
        (cap, 0xc4c8cc, 1.0, 0.12),
        (bezel, 0x202225, 0.0, 0.4),
        (bead, 0xd3d7db, 1.0, 0.08),
        (fog, 0xc9cdd1, 0.9, 0.1),
        (fog_cap, 0xb9bdc2, 1.0, 0.14),
        (plate_lamp, 0xd6d8da, 0.25, 0.22),
    ])
    # In shadow toward the opening's edge, where the housing turns back into the body.
    edge = edge_depth(m, phi, z, HEAD, 8.0, 114.0, 0.655, 0.825, 0.45, 'T_car_head_edge')
    shadow = m.math('MULTIPLY', m.math('SUBTRACT', 1.0, ramp(m, edge, 0.0, 0.55)), m.math('SUBTRACT', 1.0, m.math('MAXIMUM', fog, bezel)))
    col = m.mix(m.math('MULTIPLY', shadow, 0.7), col, core.hex_linear(0x0d0e10))
    m.set('Base Color', col)
    m.set('Metallic', met)
    m.set('Roughness', rou)
    h = sum_of(m, [
        masked(m, m.math('MULTIPLY', hb, m.math('SUBTRACT', 1.0, cap)), m.math('ADD', dish(m, r1, 0.041, 0.014), flutes(m, r1, th1, 0.041, 10, 0.003))),
        masked(m, cap, m.math('ADD', dish(m, r1, 0.041, 0.014), dish(m, r1, 0.011, -0.004))),
        masked(m, m.math('SUBTRACT', pj_bowl, pj_shroud), m.math('ADD', dish(m, r2, 0.053, 0.012), flutes(m, r2, th2, 0.053, 12, 0.0025))),
        masked(m, m.math('SUBTRACT', pj_ring, pj_lens), m.math('ADD', -0.004, dish(m, r2, 0.031, -0.002))),
        masked(m, pj_lens, m.math('ADD', -0.006, dish(m, r2, 0.0245, -0.007))),
        masked(m, m.math('SUBTRACT', pj_shroud, pj_ring), -0.007),
        masked(m, m.math('MAXIMUM', pos, amber), pillows(m, P, 0.006, 0.0005)),
        masked(m, bead, 0.0008),
    ])
    h = m.math('MULTIPLY', h, m.math('SUBTRACT', 1.0, fog))
    fog_h = m.math('ADD', dish(m, df, 0.039, 0.008), ripples(m, fx, 0.005, 0.0004))
    h = m.math('ADD', h, masked(m, m.math('SUBTRACT', fog, plate_lamp), fog_h))
    h = m.math('ADD', h, masked(m, plate_lamp, ripples(m, ax, 0.004, 0.0003)))
    bumped(m, h)


def taillamp_shading(m, bvh):
    """The tail lamp: a red lens over red-silvered reflectors molded with pillow optics; two round lamps, each a
    deep red bowl ringed with Fresnel steps round its bulb, in a dark raised bezel; the reverse lamp (a clear,
    ribbed lens over chrome) at the inner end; smoked along the top. The rear side markers and the bumper's
    reflectors (low) share it: finer prisms."""
    sep, ax, P = lens_point(m)
    y, z = sep.outputs['Y'], sep.outputs['Z']
    phi = m.math('MULTIPLY', m.math('ARCTAN2', m.math('SUBTRACT', ax, TAIL_AXIS[0]), m.math('SUBTRACT', y, TAIL_AXIS[1])), 180.0 / math.pi)
    ra, tha = polar(m, P, surface_frame(bvh, TAIL_AXIS, 1, 46, 0.905))
    rb, thb = polar(m, P, surface_frame(bvh, TAIL_AXIS, 1, 72, 0.928))
    soft = 0.0008
    e1, e2 = smooth_less(m, ra, 0.042, soft), smooth_less(m, rb, 0.036, soft)
    o1, o2 = smooth_less(m, ra, 0.056, soft), smooth_less(m, rb, 0.049, soft)
    c1, c2 = smooth_less(m, ra, 0.009, soft), smooth_less(m, rb, 0.008, soft)
    elem = m.math('MAXIMUM', e1, e2)
    bezel = m.math('MAXIMUM', m.math('SUBTRACT', o1, e1), m.math('SUBTRACT', o2, e2))
    caps = m.math('MAXIMUM', c1, c2)
    low = ramp(m, z, 0.73, 0.71)
    reverse = m.math('MULTIPLY', ramp(m, phi, 19.6, 18.6), m.math('SUBTRACT', 1.0, low))
    smoke = m.math('MULTIPLY', ramp(m, z, 0.998, 1.012), 0.75)
    col, met, rou = layered(m, (0x7a070c, 0.35, 0.1), [
        (elem, 0xb3121a, 0.75, 0.1),
        (caps, 0xd2262c, 0.6, 0.14),
        (bezel, 0x2a0305, 0.1, 0.25),
        (reverse, 0xd3d6d9, 0.5, 0.1),
        (low, 0xa3121a, 0.4, 0.08),
    ])
    col = m.mix(smoke, col, core.hex_linear(0x170203))
    edge = edge_depth(m, phi, z, TAIL, 6.0, 106.0, 0.775, 1.035, 0.42, 'T_car_red_edge')
    shadow = m.math('MULTIPLY', m.math('SUBTRACT', 1.0, ramp(m, edge, 0.0, 0.5)), m.math('SUBTRACT', 1.0, low))
    col = m.mix(m.math('MULTIPLY', shadow, 0.6), col, core.hex_linear(0x1a0204))
    m.set('Base Color', col)
    m.set('Metallic', m.math('MULTIPLY', met, m.math('SUBTRACT', 1.0, m.math('MULTIPLY', smoke, 0.6))))
    m.set('Roughness', rou)
    field = m.math('SUBTRACT', 1.0, m.math('MAXIMUM', m.math('MAXIMUM', o1, o2), m.math('MAXIMUM', reverse, low)))
    h = sum_of(m, [
        masked(m, e1, m.math('ADD', dish(m, ra, 0.042, 0.008), ripples(m, ra, 0.0045, 0.0003))),
        masked(m, e2, m.math('ADD', dish(m, rb, 0.036, 0.007), ripples(m, rb, 0.0045, 0.0003))),
        masked(m, c1, dish(m, ra, 0.009, -0.003)),
        masked(m, c2, dish(m, rb, 0.008, -0.003)),
        masked(m, bezel, 0.0015),
        masked(m, field, pillows(m, P, 0.009, 0.0005)),
        masked(m, reverse, ripples(m, z, 0.006, 0.0004)),
        masked(m, low, pillows(m, P, 0.0035, 0.00025)),
    ])
    bumped(m, h)


def plate_material(img):
    m = core.Mat('car_plate')
    tc, sep = object_coords(m)
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    rear = m.math('GREATER_THAN', y, 0.0)
    # The front plate faces -Y (reads along +X), the rear +Y (reads along -X).
    u = m.math('ADD', m.math('MULTIPLY', m.math('DIVIDE', x, 0.3), m.math('SUBTRACT', 1.0, m.math('MULTIPLY', rear, 2.0))), 0.5)
    zc = m.math('ADD', PLATE_F[1], m.math('MULTIPLY', rear, PLATE_R[1] - PLATE_F[1]))
    v = m.math('ADD', m.math('DIVIDE', m.math('SUBTRACT', z, zc), 0.15), 0.5)
    ink, alpha, metal, rough = image_surface(m, img, vec(m, u, v, 0.0))
    grime = ramp(m, noise(m, tc.outputs['Object'], scale=30.0, detail=4.0), 0.45, 0.8)
    m.set('Base Color', m.mix(m.math('MULTIPLY', grime, 0.35), ink, core.hex_linear(0x5a5246)))
    m.set('Roughness', m.math('ADD', rough, m.math('MULTIPLY', grime, 0.3)))
    m.set('Metallic', 0.3)
    return m


def tire_material(text):
    """Rubber: the tread's grooves and sipes, the outer sidewall's raised lettering, road dust."""
    m = core.Mat('car_tire')
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    a = m.math('ADD', AXLES[0], m.math('MULTIPLY', m.math('GREATER_THAN', y, 0.0), AXLES[1] - AXLES[0]))
    ly, lz = m.math('SUBTRACT', y, a), m.math('SUBTRACT', z, WZ)
    r = m.math('SQRT', m.math('ADD', m.math('MULTIPLY', ly, ly), m.math('MULTIPLY', lz, lz)))
    ang = m.math('ARCTAN2', lz, ly)
    lx = m.math('SUBTRACT', m.math('ABSOLUTE', x), TRACK)
    # Read clockwise over the top from outside, on either side of the car; twice round.
    u = m.math('FRACT', m.math('MULTIPLY', m.math('MULTIPLY', ang, m.math('SIGN', x)), -2.0 / (2 * math.pi)))
    v = m.math('DIVIDE', m.math('SUBTRACT', r, 0.255), 0.046)
    letters = m.math('MULTIPLY', sample_mask(m, text, u, v), ramp(m, lx, 0.07, 0.09))
    tread = ramp(m, r, 0.322, 0.329)
    circ = None
    for c in (-0.062, -0.022, 0.022, 0.062):
        g = smooth_less(m, m.math('ABSOLUTE', m.math('SUBTRACT', lx, c)), 0.0045, 0.0012)
        circ = g if circ is None else m.math('MAXIMUM', circ, g)
    sipe = ramp(m, m.math('ABSOLUTE', m.math('SINE', m.math('ADD', m.math('MULTIPLY', ang, 30.0), m.math('MULTIPLY', lx, 9.0)))), 0.93, 0.97)
    grooves = m.math('MULTIPLY', m.math('MAXIMUM', circ, m.math('MULTIPLY', sipe, ramp(m, m.math('ABSOLUTE', lx), 0.035, 0.045))), tread)
    # A raised rib round the rim protector.
    rib = m.math('MULTIPLY', smooth_less(m, m.math('ABSOLUTE', m.math('SUBTRACT', r, 0.236)), 0.0035, 0.001), ramp(m, lx, 0.06, 0.08))
    # Wet road: the rubber stays near black, a little road dust dried into the sidewall low down.
    dust = m.math('MULTIPLY', ramp(m, noise(m, o, scale=12.0, detail=4.0), 0.45, 0.85), ramp(m, z, 0.45, 0.1))
    col = m.mix(m.math('MULTIPLY', dust, 0.35), core.hex_linear(0x121212), core.hex_linear(0x2e2a25))
    col = m.mix(letters, col, core.hex_linear(0x262626))
    col = m.mix(grooves, col, core.hex_linear(0x060606))
    m.set('Base Color', col)
    m.set('Roughness', mix_float(m, dust, mix_float(m, tread, 0.78, 0.66), 0.9))
    bump = m.node('ShaderNodeBump', Strength=0.5, Distance=0.002)
    h = m.math('ADD', m.math('ADD', m.math('MULTIPLY', letters, 0.6), m.math('MULTIPLY', rib, 0.5)), m.math('MULTIPLY', grooves, -1.0))
    m.link(m.math('ADD', h, m.math('MULTIPLY', noise(m, o, scale=300.0, detail=1.0), 0.05)), bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def rim_material():
    """Silver-painted alloy under clear coat (aluminum flake, so it catches light broadly), brake dust gathered toward
    the barrel and round the spokes' roots."""
    m = core.Mat('car_rim')
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    lx = m.math('SUBTRACT', m.math('ABSOLUTE', sep.outputs['X']), TRACK)
    blot = noise(m, o, scale=28.0, detail=4.0)
    dust = m.math('MAXIMUM', m.math('MULTIPLY', ramp(m, lx, 0.07, 0.0), 0.75), m.math('MULTIPLY', ramp(m, blot, 0.55, 0.78), 0.45))
    flake = noise(m, o, scale=1200.0, detail=1.0)
    col = m.mix(m.math('MULTIPLY', flake, 0.25), core.hex_linear(0xb4b8bd), core.hex_linear(0xc8ccd0))
    m.set('Base Color', m.mix(dust, col, core.hex_linear(0x3b342d)))
    m.set('Metallic', mix_float(m, dust, 0.8, 0.25))
    m.set('Roughness', mix_float(m, dust, 0.3, 0.72))
    m.set('Coat Weight', 0.6)
    m.set('Coat Roughness', 0.05)
    return m


def brake_material():
    """Cast iron: the discs' faces scored round by the pads, a bloom of flash rust from the rain, the calipers dull."""
    m = core.Mat('car_brake')
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    y, z = sep.outputs['Y'], sep.outputs['Z']
    a = m.math('ADD', AXLES[0], m.math('MULTIPLY', m.math('GREATER_THAN', y, 0.0), AXLES[1] - AXLES[0]))
    ly, lz = m.math('SUBTRACT', y, a), m.math('SUBTRACT', z, WZ)
    r = m.math('SQRT', m.math('ADD', m.math('MULTIPLY', ly, ly), m.math('MULTIPLY', lz, lz)))
    score = noise(m, vec(m, m.math('MULTIPLY', r, 400.0), 0.0, 0.0), detail=2.0)
    rust = m.math('MULTIPLY', ramp(m, noise(m, o, scale=30.0, detail=5.0), 0.5, 0.75), 0.75)
    col = m.mix(m.math('MULTIPLY', score, 0.4), core.hex_linear(0x55585c), core.hex_linear(0x75787c))
    col = m.mix(rust, col, core.hex_linear(0x5e3720))
    m.set('Base Color', col)
    m.set('Metallic', mix_float(m, rust, 0.9, 0.3))
    m.set('Roughness', mix_float(m, rust, m.math('ADD', 0.38, m.math('MULTIPLY', score, 0.12)), 0.82))
    return m


def materials():
    lines = line_sheets()
    paint = paint_material(lines)
    trim = street.shop_plastic('car_trim', 0x111214, rough=0.6, grime=0.35)
    glass = street.glass('car_glass', 0x0b0e12, rough=0.03)
    chrome = street.bare_metal('car_chrome', 0xd2d6da, rough=0.07, grime=0.25)
    # The lamps' shading is measured on the body, so it's filled in once the body is built (headlamp_shading).
    head = core.Mat('car_head')
    amber = street.lens('car_amber', 0xd98a24, rough=0.08)
    red = core.Mat('car_red')
    plate = plate_material(plate_sheet())
    tire = tire_material(tire_sheet())
    rim = rim_material()
    brake = brake_material()
    order = [paint, trim, glass, chrome, head, amber, red, plate, tire, rim, brake]
    mats = {'paint': paint, 'trim': trim, 'glass': glass, 'chrome': chrome, 'head': head, 'amber': amber, 'red': red, 'plate': plate,
            'tire': tire, 'rim': rim, 'brake': brake, 'order': order}
    mats['index'] = {m.m.name: i for i, m in enumerate(order)}
    return mats


CABIN_GAIN = 0.3


def show_cabin(glass, img):
    """The glass's tint with the baked view through it laid in faintly; not on the gloss-black appliques (the B pillars
    and the rear doors' dividers: side-facing glass between their edges)."""
    m = glass
    tex = m.nt.nodes.new('ShaderNodeTexImage')
    tex.image = img
    tc, sep = object_coords(m)
    y, z = sep.outputs['Y'], sep.outputs['Z']
    nx, _, _ = normal_parts(m)

    def edge_y(line):
        (y0, z0), (y1, z1) = line
        return m.math('ADD', m.math('MULTIPLY', m.math('SUBTRACT', z, z0), (y1 - y0) / (z1 - z0)), y0)

    def between(front, rear):
        return m.math('MULTIPLY', ramp(m, m.math('SUBTRACT', y, edge_y(front)), -0.002, 0.002), ramp(m, m.math('SUBTRACT', edge_y(rear), y), -0.002, 0.002))

    applique = m.math('MULTIPLY', ramp(m, m.math('ABSOLUTE', nx), 0.45, 0.6), m.math('MAXIMUM', between(B_FRONT, B_REAR), between(DIV_FRONT, DIV_REAR)))
    k = m.math('MULTIPLY', m.math('SUBTRACT', 1.0, applique), CABIN_GAIN)
    m.set('Base Color', m.mix(k, core.hex_linear(0x0b0e12), tex.outputs['Color'], blend='ADD'))


# ------------------------------------------------------------------ assembly

def finish(objs, mats):
    obj = core.join(objs, 'SM_Sedan')
    obj.name = obj.data.name = 'SM_Sedan'
    core.finish_hard_surface(obj, 38.0)
    tire_i = mats['index']['car_tire']

    def outer_wall(f):
        c = f.calc_center_median()
        return f.material_index == tire_i and abs(c.x) > TRACK + 0.05 and f.normal.x * c.x > 0 and abs(f.normal.x) > 0.5

    # The tires' outer walls (lettered) get most of the tire texture.
    groups = [(outer_wall, 'box', None, (0.0, 0.36, 1.0, 1.0)), (lambda f: f.material_index == tire_i, 'box', None, (0.0, 0.0, 1.0, 0.35))]
    # The head and tail lamps' lenses are unwrapped after (lamp_uvs); the fog, plate and marker lamps share a band below.
    groups += [(lamp_lens(mats), 'keep', None, (0.0, 0.0, 0.01, 0.01)),
               (lambda f: f.material_index in (mats['index']['car_head'], mats['index']['car_red']), 'box', None, (0.0, 0.0, 1.0, LAMP_REST))]
    groups += [(lambda f, i=i: f.material_index == i, 'box', None, (0.0, 0.0, 1.0, 1.0)) for i in range(len(obj.material_slots))]
    groups.append((lambda f: True, 'box', None, (0.0, 0.0, 1.0, 1.0)))
    core.uv_layout(obj, groups)
    lamp_uvs(obj, mats)
    return obj


LAMP_REST = 0.095
LAMP_STRIPS = {1: (0.0, 0.105, 1.0, 0.545), -1: (0.0, 0.555, 1.0, 0.995)}


def lamp_lens(mats, side=0):
    """A face test: the headlamps' and tail lamps' lenses (one side's, given side)."""
    head_i, red_i = mats['index']['car_head'], mats['index']['car_red']

    def test(f):
        c = f.calc_center_median()
        if side and c.x * side < 0.2:
            return False
        return (f.material_index == head_i and c.z > 0.6 and c.y < -1.5) or (f.material_index == red_i and c.z > 0.75 and c.y > 1.5)
    return test


def lamp_uvs(obj, mats):
    """Each head and tail lamp lens unwrapped whole (angle based, so true to its surface: no seam through the projector
    or a lamp ring), turned to lie along its length and fitted to its strip with square texels on the 1024 x 512 maps."""
    me = obj.data
    for name, front in (('car_head', True), ('car_red', False)):
        size = TEXTURE_SIZES[name]
        for side, rect in LAMP_STRIPS.items():
            test = lamp_lens(mats, side)
            core.select_only([obj])
            bpy.ops.object.mode_set(mode='EDIT')
            bpy.ops.mesh.select_mode(type='FACE')
            bm = bmesh.from_edit_mesh(me)
            for f in bm.faces:
                c = f.calc_center_median()
                f.select_set(test(f) and (c.y < 0) == front)
            bmesh.update_edit_mesh(me)
            # Not aspect-corrected: the operator would take the aspect of an image in the lamp's material (its edge map).
            bpy.ops.uv.unwrap(method='ANGLE_BASED', margin=0.0, correct_aspect=False)
            bm = bmesh.from_edit_mesh(me)
            uv = bm.loops.layers.uv.verify()
            loops = [lp for f in bm.faces if f.select for lp in f.loops]
            if loops:
                pts = [lp[uv].uv.copy() for lp in loops]
                cx = sum(p.x for p in pts) / len(pts)
                cy = sum(p.y for p in pts) / len(pts)
                sxx = sum((p.x - cx) ** 2 for p in pts)
                syy = sum((p.y - cy) ** 2 for p in pts)
                sxy = sum((p.x - cx) * (p.y - cy) for p in pts)
                ang = -0.5 * math.atan2(2 * sxy, sxx - syy)
                ca, sa = math.cos(ang), math.sin(ang)
                rot = [((p.x - cx) * ca - (p.y - cy) * sa, (p.x - cx) * sa + (p.y - cy) * ca) for p in pts]
                a0, a1 = min(p[0] for p in rot), max(p[0] for p in rot)
                b0, b1 = min(p[1] for p in rot), max(p[1] for p in rot)
                W, H = size
                u0, v0, u1, v1 = rect
                s = min((u1 - u0) * W / (a1 - a0), (v1 - v0) * H / (b1 - b0))
                ox = u0 * W + ((u1 - u0) * W - (a1 - a0) * s) / 2
                oy = v0 * H + ((v1 - v0) * H - (b1 - b0) * s) / 2
                for lp, (a, b) in zip(loops, rot):
                    lp[uv].uv = ((ox + (a - a0) * s) / W, (oy + (b - b0) * s) / H)
                bmesh.update_edit_mesh(me)
            bpy.ops.object.mode_set(mode='OBJECT')


def build():
    core.reset()
    mats = materials()
    out = []
    body, bvh0 = body_shell(mats)
    out.append((body, None))
    gh, drip, stations = greenhouse(mats)
    out.append((gh, None))
    dg = bpy.context.evaluated_depsgraph_get()
    bvh = BVHTree.FromObject(body, dg)
    gh_bvh = BVHTree.FromObject(gh, dg)
    headlamp_shading(mats['head'], bvh)
    taillamp_shading(mats['red'], bvh)
    out += lamps(bvh, bvh0, mats)
    out += front_end(bvh0, mats)
    out += rear_end(bvh, mats)
    out += side_details(bvh, gh_bvh, mats)
    out += window_trim(gh_bvh, bvh, drip, stations, mats)
    out += cowl_and_wipers(bvh, gh_bvh, mats)
    for ay in AXLES:
        for sx in (-1, 1):
            out += wheel(sx * TRACK, ay, mats)
    for o, m in out:
        if m is not None:
            core.assign(o, m)
    obj = finish([o for o, _ in out], mats)
    show_cabin(mats['glass'], bake_cabin(obj, mats))
    # The faces' bookkeeping (section spans, greenhouse bands) stays here: the export carries only the car.
    for name in ('span', 'band', 'station'):
        if name in obj.data.attributes:
            obj.data.attributes.remove(obj.data.attributes[name])
    return [obj]
