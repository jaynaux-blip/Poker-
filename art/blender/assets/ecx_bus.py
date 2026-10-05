"""SM_ECX_Bus*: the 14, City Transit's 40-ft low-floor hybrid bus that climbs from Northside to the Embercrest's stop.

A late-2010s transit bus in the city's livery (spec v1 §3.3), 12.19 m bumper to bumper, 2.59 m wide over the skin,
3.165 m over the roof's crown, 3.30 m over the battery pod and the air-conditioning unit:
- The shell is one thin-walled solid (outer skin over an inner skin 45 mm in), lofted through horizontal plan
  sections whose front and rear profiles, corner radii and roof arc are functions of height: a nearly upright front
  cap with a slight rake to the two-piece windshield, a visor over it and the destination-sign box flush at the top;
  an upright rear with the rear window, the route sign and the engine door; a 240 mm roof radius into a gentle crown.
  The windows, doors, windshield, wheel arches and the sign and grille pockets are cut through it, so every opening
  has its reveal (black) and the inner skin is the interior's wall.
- Livery: white over a dark grey skirt below the aluminium rub rails, a broad transit-green band under the windows
  with a gold pinstripe that sweeps up over the rear quarter, the black window band (frit) wrapping round the
  windshield and the sign box. Panel shut lines and access doors drawn into the paint; road film, salt spray behind
  the wheels, snowmelt streaks from the windows and the roof edge, diesel soot at the back, a dirty roof.
- Decals are their own faces DECAL_E (1 mm) off the skin, following it (slot 5, a 4096 print atlas): CITY TRANSIT
  both sides and on the rear, fleet number 2314 front, rear, both sides and on the roof (reading from the curb side,
  i.e. from the plaza above the stop), the 14 EMBERCREST route cards in the first windows, ADA symbols by the doors, a
  kneeling-bus sticker, a yield triangle, the plates; inside, the ad cards (EMBERCREST POKER SERIES "Nightly at 7",
  the Lucky Penny's roller dogs, City Transit's owl service and courtesy cards), the 14's route strip, priority and
  wheelchair signs, exit labels, the driver's area signs, the farebox's plates, the floor's stand-behind line.
  Exterior decals take the livery, the weather and the shut lines of the skin under them (read DECAL_E back along
  their normal), so the paint runs on through them unbroken and they sit on it like vinyl; no shut line crosses a
  wordmark or a fleet number.
- Lamps (slot 4, emissive baked): LED headlamp clusters (two projectors and a light bar), amber park/turn lamps
  round the front corners (an array of LEDs in reflector cups behind the amber cover: by day a lamp, at night a field
  of glowing dots), round LED tail/turn/backup lamps in the rear clusters, the high-mount stop lamp, amber and red side
  markers, clearance and ID lamps, the plate lamp; inside, the next-stop display (CITY TRANSIT ROUTE 14), the
  farebox's screen and the dash's gauges. All on while the bus runs.
- Outside: the folded two-bike rack on the front (it stands 9 cm proud of the bumper, as a real one does), black
  energy-absorbing bumpers, wipers, the transit mirrors (flat and convex heads on arms, out to 1.55 m), the radiator
  grille on the street side, the engine door's vent, roof battery pod and air-conditioning unit with its fans, two
  roof hatches, antennas, the exhaust stack.
- Inside: grey speckled rubber floor with yellow nosings, the low floor from the windshield to the rear door, two
  steps up to the rear deck, wheel housings, forward-facing doubles in city-green moquette with grey shells and
  grab handles, the side-facing priority seats, the wheelchair bay with its flip seats folded and belt reels, the
  rear bench, yellow stanchions and overhead rails with stop buttons, modesty panels, heater ducts, the ad racks,
  the ceiling with its two LED channels, door operators, the driver's cab (seat, wheel, dash, barrier door), the
  farebox and a card validator.

The bus frame (meters): front toward -X, the curb (right) side toward +Y, origin on the road under the middle of the
bus. Front bumper x -6.10, rear +6.09; front axle x -3.80, rear +3.30; wheel centers z 0.51; floor z 0.36. Doors on
+Y: front x -5.85..-4.70, rear -0.075..+1.075, hinges at y +1.275.

Choices inside the spec's ranges: the low floor (0.36) runs from the windshield to just behind the rear door; two
steps (x 1.30, 1.60) climb to the rear deck at 0.70 over the rear axle and the engine, as on a US low-floor bus. The
driver's seat sits on the low floor where the spec's seated figure goes (frame (-4.95, -0.70, 0.36) facing -X: the
cushion's top at 0.82, centered at x -4.75). A passenger figure in the crowd's seated frame (seat 20 cm behind the
origin at 46 cm) fits any forward-facing seat with its origin at (row front edge + 0.005, seat center y, floor +
0.0145): seat centers at y +-0.6075 (aisle) and +-1.0225 (window); low-floor rows (floor 0.36) at x -1.40, -0.65,
0.10, 0.82 (left only); rear-deck rows (floor 0.70) at x 1.92 and 4.20 (both sides; the rows at 2.68 and 3.44 sit
raised on the wheel houses, floor 0.80 for the formula).
The folded bike rack stands proud of the bumper to x -6.277 (a real one does); the mirrors reach y +-1.55; the radio
whip on the roof reaches z 3.40.

Meshes, slots in order:
- SM_ECX_Bus: 0 paint, 1 trim, 2 metal, 3 interior, 4 lamps [lit], 5 prints.
- SM_ECX_Bus_Glass (unbaked): 0 ecx_glass (windshield, driver's window, sign covers, the barrier), 1 ecx_glass_tint
  (side and rear windows, the modesty panels).
- SM_ECX_Bus_DoorLeaf: 0 frame, 1 glass, 2 rubber. PROP: hinge axis at the leaf's bottom, leaf along +X, outside +Y.
- SM_ECX_Bus_Wheel / _WheelDual: 0 tire, 1 rim, 2 hub. PROP: wheel center, axle along Y, outer face +Y. The sidewall
  lettering reads clockwise from outside; a left wheel placed yawed 180 deg (instead of mirrored in Y) keeps it
  reading right.
- SM_ECX_Bus_Signs (unbaked): 0 ecx_sign_dest (front + side), 1 ecx_sign_route (rear); UV 0..1 per quad, (0,0)
  bottom-left as seen from the front.
- SM_ECX_Bus_GlowCool (unbaked, glow): the ceiling's LED channels, the stop-request strips, the step-well lights.

Also writes unreal/Art/Textures/ECX/T_ECX_BusSign_Up.png and _Down.png (1024 x 144: an amber LED dot matrix, 256 x 36
dots at 4 px, 14 EMBERCREST / 14 NORTHSIDE · FIFTH ST) and T_ECX_BusSign_Route.png (256 x 196, 64 x 49 dots: 14).
"""
import bisect
import math
import os
import struct
import sys
import zlib

import bpy  # noqa: F401,I001
import bmesh
import numpy as np
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree
from mathutils.interpolate import poly_3d_calc

from artkit import core, parts
from artkit.shade import convex_edges, mix_float, noise, object_coords, ramp, scaled, smooth_less, vec
from artkit.sheet import Sheet

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import brand  # noqa: E402

MESHES = ['SM_ECX_Bus', 'SM_ECX_Bus_Glass', 'SM_ECX_Bus_DoorLeaf', 'SM_ECX_Bus_Wheel', 'SM_ECX_Bus_WheelDual', 'SM_ECX_Bus_Signs',
          'SM_ECX_Bus_GlowCool']
DOUBLE_SIDED = False
TEXTURE_SIZE = 2048
TEXTURE_SIZES = {'ecx_bus_paint': 4096, 'ecx_bus_trim': 2048, 'ecx_bus_metal': 1024, 'ecx_bus_interior': 4096, 'ecx_bus_lamps': 1024,
                 'ecx_bus_prints': 4096, 'ecx_door_frame': 1024, 'ecx_door_glass': 512, 'ecx_door_rubber': 512,
                 'ecx_wheel_tire': 2048, 'ecx_wheel_rim': 1024, 'ecx_wheel_hub': 512, 'ecx_dual_tire': 2048, 'ecx_dual_rim': 1024,
                 'ecx_dual_hub': 512}
AO_DISTANCE = 0.12
REVIEW_VIEWS = [('curb', -128, 9, 2.15, (-0.4, 0.0, 1.6)), ('door', 158, 5, 0.40, (-4.7, 1.3, 1.55)), ('street', -42, 10, 2.15, (0.4, 0.0, 1.6))]
REVIEW_SCREEN_LIGHT = False
FAST = bool(os.environ.get('ECX_FAST'))
TEX_DIR = os.path.join(core.ROOT, 'unreal', 'Art', 'Textures', 'ECX')

# ------------------------------------------------------------------ dimensions

W = 1.295               # half width over the skin
T = 0.045               # the shell's thickness
Z_BOT = 0.28            # the body's underside
Z_FLOOR = 0.36
Z_RAISED = 0.70         # the rear deck
X_STEP1, X_STEP2, Z_STEP1 = 1.30, 1.60, 0.53
ZS = 2.90               # where the roof radius starts
R_ROOF = 0.24
TH_END = math.radians(85.0)
Z_CROWN = 3.165
X_FRONT, X_REAR = -6.10, 6.09
AXLES = (-3.80, 3.30)
WZ = 0.51
ARCH_R, ARCH_Z = 0.60, 0.55
LINER_Y = 0.74          # the wheel-house liners' inner ends
DOOR_F = (-5.885, -4.665)
DOOR_R = (-0.105, 1.105)
DOOR_Z = (0.34, 2.745)
HINGES = ((-5.85, False), (-4.70, True), (-0.075, False), (1.075, True))
HINGE_Y = 1.275
LEAF_W, LEAF_T, LEAF_H = 0.575, 0.05, 2.35
BB, BT = 1.14, 2.66     # the black window band on the sides
X_PILLAR = -5.95        # the windshield's side edges
# Window openings (x0, x1, z0, z1): right (curb) side, left (street) side.
R_WIN = [(-4.56, -3.06, 1.22, 2.36), (-2.99, -1.59, 1.22, 2.42), (-1.52, -0.18, 1.22, 2.42),
         (1.18, 2.54, 1.42, 2.42), (2.61, 3.97, 1.42, 2.42), (4.04, 5.06, 1.42, 2.42)]
L_WIN = [(-5.88, -4.62, 1.16, 2.42), (-4.55, -3.05, 1.22, 2.42), (-2.98, -1.48, 1.22, 2.42), (-1.41, 0.09, 1.22, 2.42),
         (0.16, 1.22, 1.22, 2.42), (1.29, 2.64, 1.42, 2.42), (2.71, 4.06, 1.42, 2.42)]
EXIT_WIN = (('R', 1), ('L', 2), ('R', 4), ('L', 6))
RADIATOR = (4.21, 5.54, 1.30, 2.50)
REAR_WIN = (0.86, 1.94, 2.55)
SIGN_F = (-6.08, 0.0, 2.95, 1.85, 0.26)
SIGN_S = (-4.0, 1.30, 2.55, 1.10, 0.22)
SIGN_R = (6.08, 0.0, 2.85, 0.34, 0.26)
CEIL_Z = 2.86
RAIL_Y, RAIL_Z = 0.50, 2.40
# Every exterior decal stands this far off the paint, along its normal (the prints shade the skin there). 1 mm, not the
# 2-3 mm of separate parts: at 2.5 mm the gap under the vinyl's edge (dark with the paint's baked occlusion) shows as
# a hairline round every decal from close by; at 1 mm it is under a pixel beyond 2 m, and reversed-Z depth resolves it.
DECAL_E = 0.001

# Interior parts (the 'ipart' face attribute the interior material reads).
IP_WALL, IP_CEIL, IP_FLOOR, IP_MOQ, IP_SHELL, IP_YELLOW, IP_STEEL, IP_HOUSING, IP_DASH, IP_VINYL, IP_YRUB, IP_ALU, IP_BLACK, IP_RED, \
    IP_BLUE, IP_FRAME, IP_DUCT, IP_CARD = range(18)


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


# ------------------------------------------------------------------ the shell's plan sections

NF, NR, NC, NE = 24, 24, 10, 4
X0, X1 = -5.6, 5.6
NS = 225
STATIONS = [X0 + (X1 - X0) * i / (NS - 1) for i in range(NS)]
O_CFR = NF
O_EFR = O_CFR + NC
O_SR = O_EFR + NE
O_ERR = O_SR + NS
O_CRR = O_ERR + NE - 1
O_RS = O_CRR + NC
O_CRL = O_RS + NR
O_ERL = O_CRL + NC
O_SL = O_ERL + NE
O_EFL = O_SL + NS
O_CFL = O_EFL + NE - 1
N_RING = O_CFL + NC
FRONT_IDX = [O_EFL - 1] + list(range(O_EFL, N_RING)) + list(range(0, O_SR + 1))
REAR_IDX = list(range(O_SR + NS - 1, O_SL + 1))


def ring_xy(w, xf, xr, rf, rr, bf, br):
    """One plan section: a rounded rectangle from xf (the front's middle, bowed forward by bf) to xr, half width w,
    corner radii rf and rr, sampled with fixed counts so every section has the same topology (the sides at fixed
    stations)."""
    af, ar = w - rf, w - rr
    xfe, xre = xf + bf, xr - br
    xcf, xcr = xfe + rf, xre - rr
    assert xcf < X0 - 0.004 and xcr > X1 + 0.004 and af > 0.005 and ar > 0.005, (w, xf, xr, rf, rr)
    pts = []
    for k in range(NF):
        y = -af + 2 * af * k / NF
        pts.append((xfe - bf * (1 - (y / af) ** 2), y))
    for k in range(NC):
        ph = math.pi - 0.5 * math.pi * k / NC
        pts.append((xcf + rf * math.cos(ph), af + rf * math.sin(ph)))
    for k in range(NE):
        pts.append((xcf + (X0 - xcf) * k / NE, w))
    for x in STATIONS:
        pts.append((x, w))
    for k in range(1, NE):
        pts.append((X1 + (xcr - X1) * k / NE, w))
    for k in range(NC):
        ph = 0.5 * math.pi - 0.5 * math.pi * k / NC
        pts.append((xcr + rr * math.cos(ph), ar + rr * math.sin(ph)))
    for k in range(NR):
        y = ar - 2 * ar * k / NR
        pts.append((xre + br * (1 - (y / ar) ** 2), y))
    for k in range(NC):
        ph = -0.5 * math.pi * k / NC
        pts.append((xcr + rr * math.cos(ph), -ar + rr * math.sin(ph)))
    for k in range(NE):
        pts.append((xcr + (X1 - xcr) * k / NE, -w))
    for x in reversed(STATIONS):
        pts.append((x, -w))
    for k in range(1, NE):
        pts.append((X0 + (xcf - X0) * k / NE, -w))
    for k in range(NC):
        ph = -0.5 * math.pi - 0.5 * math.pi * k / NC
        pts.append((xcf + rf * math.cos(ph), -af + rf * math.sin(ph)))
    assert len(pts) == N_RING
    return pts


_XF_O = pchip([(0.28, -5.975), (0.295, -6.012), (0.31, -6.030), (0.335, -6.040), (0.40, -6.046), (0.55, -6.050), (0.70, -6.054),
               (0.90, -6.060), (1.12, -6.064), (1.50, -6.058), (2.00, -6.049), (2.40, -6.042), (2.72, -6.036), (2.745, -6.040),
               (2.765, -6.055), (2.78, -6.072), (2.795, -6.085), (2.81, -6.09), (3.07, -6.09), (3.10, -6.083), (3.12, -6.068),
               (3.1392, -6.035)])
_RF_O = pchip([(0.28, 0.20), (0.31, 0.165), (0.345, 0.13), (0.36, 0.125), (2.75, 0.125), (2.79, 0.14), (2.82, 0.15), (3.07, 0.17),
               (3.1392, 0.20)])
_BF_O = pchip([(0.28, 0.008), (0.36, 0.012), (2.76, 0.012), (2.80, 0.004), (3.2, 0.004)])
_XR_O = pchip([(0.28, 5.965), (0.295, 6.005), (0.31, 6.022), (0.335, 6.034), (0.40, 6.040), (0.70, 6.045), (1.6, 6.050), (2.62, 6.058),
               (2.66, 6.062), (2.70, 6.07), (3.05, 6.07), (3.09, 6.06), (3.12, 6.045), (3.1392, 6.01)])
_RR_O = pchip([(0.28, 0.26), (0.33, 0.25), (3.05, 0.25), (3.1392, 0.28)])
_TUCK = pchip([(0.28, W - 0.022), (0.295, W - 0.008), (0.31, W - 0.002), (0.33, W)])
_XF_I = pchip([(0.325, -5.985), (0.40, -5.998), (0.70, -6.006), (1.12, -6.016), (2.00, -6.004), (2.72, -5.992), (2.80, -5.99), (3.2, -5.99)])


def arc_w(z, r):
    d = z - ZS
    return W - R_ROOF + math.sqrt(max(r * r - d * d, 0.0))


def outer_params(z):
    w = _TUCK(z) if z < 0.33 else (W if z <= ZS else arc_w(z, R_ROOF))
    return (w, _XF_O(z), _XR_O(z), _RF_O(z), _RR_O(z), _BF_O(z), 0.01)


def inner_params(z):
    w = W - T if z <= ZS else arc_w(z, R_ROOF - T)
    xr = _XR_O(z) - T if z <= 2.9 else 6.02
    return (w, _XF_I(z), xr, max(_RF_O(z) - T, 0.045), _RR_O(z) - T, _BF_O(z), 0.01)


Z_RINGS = [0.28, 0.287, 0.295, 0.305, 0.318, 0.333, 0.355, 0.39, 0.45, 0.52, 0.60, 0.655, 0.685, 0.715, 0.745, 0.775, 0.83, 0.89,
           0.96, 1.03, 1.085, 1.105, 1.13, 1.155, 1.19, 1.25, 1.35, 1.45, 1.55, 1.65, 1.75, 1.85, 1.95, 2.05, 2.15, 2.25, 2.35, 2.45,
           2.55, 2.63, 2.69, 2.71, 2.73, 2.752, 2.765, 2.778, 2.79, 2.80, 2.815, 2.84, 2.87, 2.90]
ARC_DEG = (5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85)
TOP_W = (0.92, 0.76, 0.60, 0.44, 0.30, 0.18, 0.08)
BOTTOM_W = (1.10, 0.85, 0.60, 0.36, 0.16, 0.06)


def main_rings(inner):
    """The plan sections from the underside to the end of the roof arc: (z, params)."""
    if inner:
        zs = [Z_BOT + T] + [z for z in Z_RINGS if Z_BOT + T + 0.015 < z <= ZS]
        zs += [ZS + (R_ROOF - T) * math.sin(math.radians(a)) for a in ARC_DEG]
        return [(z, inner_params(z)) for z in zs]
    zs = Z_RINGS + [ZS + R_ROOF * math.sin(math.radians(a)) for a in ARC_DEG]
    return [(z, outer_params(z)) for z in zs]


def nested(p, wk, recede, rf_max=None, rr_max=None):
    """A plan section inside p (for the roof's crown and the underside), wk wide, its ends receded."""
    w, xf, xr, rf, rr, bf, br = p
    f = 1.0 - wk / w
    xf, xr, bf, br = xf + recede * f, xr - recede * f, bf * wk / w, br * wk / w
    rf = min(rf if rf_max is None else rf_max, wk - 0.02, X0 - 0.01 - xf - bf)
    rr = min(rr if rr_max is None else rr_max, wk - 0.02, xr - br - X1 - 0.01)
    return (wk, xf, xr, rf, rr, bf, br)


def all_rings(inner):
    main = main_rings(inner)
    zb, pb = main[0]
    zt, pt = main[-1]
    crown_top = Z_CROWN - (T if inner else 0.0)
    out = [(zb, nested(pb, wk, 0.08, rr_max=0.24 if not inner else 0.2)) for wk in reversed(BOTTOM_W)]
    out += main
    for wk in TOP_W:
        out.append((crown_top - (crown_top - zt) * (wk / pt[0]) ** 2, nested(pt, wk, 0.18)))
    return out


def close_ring(bm, r):
    """Closes the innermost section: quads across between the side stations, an n-gon at each end."""
    R = [r[O_SR + i] for i in range(NS)]
    L = [r[O_SL + NS - 1 - i] for i in range(NS)]
    faces = [bm.faces.new((R[i], R[i + 1], L[i + 1], L[i])) for i in range(NS - 1)]
    front = [r[k] for k in [O_SL + NS - 1] + list(range(O_SL + NS, N_RING)) + list(range(0, O_SR + 1))]
    faces.append(bm.faces.new(front))
    faces.append(bm.faces.new([r[k] for k in range(O_SR + NS - 1, O_SL + 1)]))
    return faces


def shell_into(bm, rings, mat_index):
    vs = []
    for z, p in rings:
        vs.append([bm.verts.new((x, y, z)) for x, y in ring_xy(*p)])
    faces = []
    for a, b in zip(vs, vs[1:]):
        for k in range(N_RING):
            faces.append(bm.faces.new((a[k], a[(k + 1) % N_RING], b[(k + 1) % N_RING], b[k])))
    faces += close_ring(bm, vs[0])
    faces += close_ring(bm, vs[-1])
    for f in faces:
        f.material_index = mat_index
    bmesh.ops.recalc_face_normals(bm, faces=faces)
    return faces


class Surface:
    """The outer skin's sections, for fitting parts flush to it: the plan polyline at any height (between sections,
    the facets' own), its front and rear runs."""

    def __init__(self):
        self.rings = [(z, ring_xy(*p)) for z, p in main_rings(False)]
        self.zs = [z for z, _ in self.rings]

    def ring_at(self, z):
        i = max(0, min(bisect.bisect_right(self.zs, z) - 1, len(self.zs) - 2))
        z0, a = self.rings[i]
        z1, b = self.rings[i + 1]
        t = max(0.0, min(1.0, (z - z0) / (z1 - z0)))
        return [(pa[0] + (pb[0] - pa[0]) * t, pa[1] + (pb[1] - pa[1]) * t) for pa, pb in zip(a, b)]

    def front(self, z):
        r = self.ring_at(z)
        return [r[k] for k in FRONT_IDX]

    def rear(self, z):
        r = self.ring_at(z)
        return [r[k] for k in REAR_IDX]


def cum(pts):
    s = [0.0]
    for a, b in zip(pts, pts[1:]):
        s.append(s[-1] + math.hypot(b[0] - a[0], b[1] - a[1]))
    return s


def at_s(pts, S, s):
    i = max(0, min(bisect.bisect_right(S, s) - 1, len(S) - 2))
    t = (s - S[i]) / max(S[i + 1] - S[i], 1e-12)
    a, b = pts[i], pts[i + 1]
    return (a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t)


def s_where(pts, S, key, target, s0=0.0, s1=None):
    """The first arc length in [s0, s1] where key(point) crosses target."""
    s1 = S[-1] if s1 is None else s1
    for i in range(len(pts) - 1):
        if S[i + 1] < s0 or S[i] > s1:
            continue
        a, b = key(pts[i]) - target, key(pts[i + 1]) - target
        if a == 0.0:
            return S[i]
        if a * b < 0.0:
            return S[i] + (S[i + 1] - S[i]) * a / (a - b)
    raise ValueError(f'no crossing for {target}')


def grid_object(name, rows, toward, offset=0.0, uv=None, cap=False):
    """A quad grid through rows of 3D points (bottom to top, the same count each), its faces toward the given
    direction, moved off the surface by offset along the grid's own normals. uv: (u0, v0, u1, v1) laid on by index."""
    nr, nc = len(rows), len(rows[0])
    P = [[Vector(p) for p in row] for row in rows]
    hint = Vector(toward)
    N = []
    for i in range(nr):
        nrow = []
        for j in range(nc):
            du = P[i][min(j + 1, nc - 1)] - P[i][max(j - 1, 0)]
            dv = P[min(i + 1, nr - 1)][j] - P[max(i - 1, 0)][j]
            n = du.cross(dv)
            if n.length < 1e-12:
                n = hint.copy()
            n.normalize()
            if n.dot(hint) < 0:
                n = -n
            nrow.append(n)
        N.append(nrow)
    bm = bmesh.new()
    lay = bm.loops.layers.uv.new('UVMap')
    vs = [[bm.verts.new(P[i][j] + N[i][j] * offset) for j in range(nc)] for i in range(nr)]
    for i in range(nr - 1):
        for j in range(nc - 1):
            f = bm.faces.new((vs[i][j], vs[i][j + 1], vs[i + 1][j + 1], vs[i + 1][j]))
            if uv is not None:
                u0, v0, u1, v1 = uv
                for lp, (a, b) in zip(f.loops, ((j, i), (j + 1, i), (j + 1, i + 1), (j, i + 1))):
                    lp[lay].uv = (u0 + (u1 - u0) * a / (nc - 1), v0 + (v1 - v0) * b / (nr - 1))
    bm.normal_update()
    if sum(f.normal.dot(hint) for f in bm.faces) < 0:
        bmesh.ops.reverse_faces(bm, faces=bm.faces[:])
    return core.mesh_object(name, bm)


# ------------------------------------------------------------------ small geometry helpers

def set_attr(obj, name, value):
    me = obj.data
    a = me.attributes.get(name) or me.attributes.new(name, 'FLOAT', 'FACE')
    a.data.foreach_set('value', [float(value)] * len(me.polygons))
    return obj


def skin_decal(obj, depth=0.002):
    """Lays a decal on the paint: its edge turned down into the body (DECAL_E + depth), so no gap under the vinyl's edge
    shows from a low angle (as the sedan's lenses are fitted). The rim's faces have no area in UV (each corner takes its
    edge's UV), so they take the decal's edge texels and claim none of the bake. Face attribute 'skin': 1 on the decal,
    2 on its rim (both take the paint's shading normals in skin_normals)."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    uv = bm.loops.layers.uv.active
    bm.normal_update()
    n0 = len(bm.faces)
    edges = [e for e in bm.edges if e.is_boundary]
    down = {v: bm.verts.new(v.co - v.normal * (DECAL_E + depth)) for v in {v for e in edges for v in e.verts}}
    for e in edges:
        lp = next(lo for lo in e.link_faces[0].loops if lo.edge == e)
        a, b = lp.vert, lp.link_loop_next.vert
        ua, ub = lp[uv].uv.copy(), lp.link_loop_next[uv].uv.copy()
        f = bm.faces.new((b, a, down[a], down[b]))
        for fl, t in zip(f.loops, (ub, ua, ua, ub)):
            fl[uv].uv = t
    bm.to_mesh(obj.data)
    bm.free()
    me = obj.data
    a = me.attributes.get('skin') or me.attributes.new('skin', 'FLOAT', 'FACE')
    a.data.foreach_set('value', [1.0 if i < n0 else 2.0 for i in range(len(me.polygons))])
    return obj


def put(obj, mat, ip=None):
    core.assign(obj, mat)
    if ip is not None:
        set_attr(obj, 'ipart', ip)
    return obj


def box(name, x0, x1, y0, y1, z0, z1, mat, r=0.0, seg=2, ip=None):
    o = parts.rbox(name, ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), (abs(x1 - x0), abs(y1 - y0), abs(z1 - z0)), r, seg)
    return put(o, mat, ip)


def rod(name, a, b, r, mat, n=12, ip=None):
    return put(parts.rod(name, a, b, r, n), mat, ip)


def prism_y(name, outline, y0, y1):
    """A prism along Y from an (x, z) outline."""
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


def prism_z(name, outline, z0, z1):
    """A prism along Z from an (x, y) outline."""
    return core.mesh_object(name, core.slab(outline, z0, z1))


def rr_xz(x0, x1, z0, z1, r, steps=5, radii=None):
    return core.rounded_rect(x0, z0, x1, z1, r, steps, radii)


def pane(name, outline3d, toward, uv=None):
    """A flat n-gon through 3D points, facing toward."""
    bm = bmesh.new()
    lay = bm.loops.layers.uv.new('UVMap')
    f = bm.faces.new([bm.verts.new(p) for p in outline3d])
    if uv is not None:
        for lp, t in zip(f.loops, uv):
            lp[lay].uv = t
    bm.normal_update()
    if f.normal.dot(Vector(toward)) < 0:
        bmesh.ops.reverse_faces(bm, faces=[f])
    return core.mesh_object(name, bm)


def quad(name, center, u_dir, v_dir, w, h, uv=(0.0, 0.0, 1.0, 1.0), nu=1, nv=1):
    """A flat w x h decal: u_dir is the way its print reads, v_dir its up; the face looks along u x v."""
    c, u, v = Vector(center), Vector(u_dir).normalized(), Vector(v_dir).normalized()
    rows = [[c + u * (w * (j / nu - 0.5)) + v * (h * (i / nv - 0.5)) for j in range(nu + 1)] for i in range(nv + 1)]
    return grid_object(name, rows, u.cross(v), 0.0, uv)


def frame_ring(name, outer, inner, a0, a1, axis):
    """A flat frame (outer and inner rectangles (p0, p1, q0, q1) in the plane across axis) from a0 to a1 along it."""
    po0, po1, qo0, qo1 = outer
    pi0, pi1, qi0, qi1 = inner
    outline = [(po0, qo0), (po1, qo0), (po1, qo1), (po0, qo1)]
    hole = [(pi0, qi0), (pi1, qi0), (pi1, qi1), (pi0, qi1)]
    o = parts.outline_prism(name, outline, [hole], a0, a1)
    if axis == 'X':
        o.data.transform(Matrix(((0, 0, 1, 0), (1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 0, 1))))
    elif axis == 'Y':
        o.data.transform(Matrix(((1, 0, 0, 0), (0, 0, 1, 0), (0, 1, 0, 0), (0, 0, 0, 1))))
    core.orient_normals(o)
    return o


def loft_rings(name, rings, caps=True):
    return core.loft(name, rings, caps)


def lathe_y(name, profile, segments=64, at=(0.0, 0.0, 0.0), closed=True):
    """A solid of revolution about the Y axis from (radius, y) points (a closed outline when closed)."""
    pts = list(profile) + ([profile[0]] if closed else [])
    obj = core.lathe(name, pts, segments=segments)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-6)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.transform(Matrix.Translation(at) @ Matrix.Rotation(math.radians(-90), 4, 'X'))
    return obj


def arc_sweep(name, cx, cz, R, phi0, phi1, n, y, side, section):
    """A trim swept round a wheel arch on the side face: section points (out, radial) from the skin at y = side * W."""
    rings = []
    for i in range(n + 1):
        ph = math.radians(phi0 + (phi1 - phi0) * i / n)
        d = Vector((math.cos(ph), 0.0, math.sin(ph)))
        rings.append([Vector((cx, 0.0, cz)) + d * (R + rad) + Vector((0.0, side * (y + out), 0.0)) for out, rad in section])
    return core.loft(name, rings)


def flatten(objs):
    out = []
    for o in objs:
        if isinstance(o, (list, tuple)):
            out += flatten(o)
        elif o is not None:
            out.append(o)
    return out


# ------------------------------------------------------------------ shader helpers

def ge(m, v, e, soft):
    """About 1 where v > e (smooth over +-soft); e a constant or a socket."""
    n = m.node('ShaderNodeMapRange')
    n.interpolation_type = 'SMOOTHSTEP'
    m.link(v, n.inputs['Value'])
    for name, k in (('From Min', -soft), ('From Max', soft)):
        if isinstance(e, (int, float)):
            n.inputs[name].default_value = e + k
        else:
            m.link(m.math('ADD', e, k), n.inputs[name])
    return n.outputs['Result']


def between(m, v, lo, hi, soft):
    return m.math('MULTIPLY', ge(m, v, lo, soft), m.math('SUBTRACT', 1.0, ge(m, v, hi, soft)))


def mul(m, *xs):
    out = xs[0]
    for x in xs[1:]:
        out = m.math('MULTIPLY', out, x)
    return out


def vmax(m, *xs):
    out = xs[0]
    for x in xs[1:]:
        out = m.math('MAXIMUM', out, x)
    return out


def geo_normal(m):
    geo = m.node('ShaderNodeNewGeometry')
    sep = m.node('ShaderNodeSeparateXYZ')
    m.link(geo.outputs['Normal'], sep.inputs['Vector'])
    return sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']


def attr(m, name):
    n = m.node('ShaderNodeAttribute')
    n.attribute_type = 'GEOMETRY'
    n.attribute_name = name
    return n.outputs['Fac']


def rrect2(m, a, b, ca, cb, ha, hb, r, soft=0.0015):
    """1 inside a rounded rectangle (half sizes ha, hb, radius r) about (ca, cb) in the coordinates a, b."""
    da = m.math('MAXIMUM', m.math('SUBTRACT', m.math('ABSOLUTE', m.math('SUBTRACT', a, ca)), ha - r), 0.0)
    db = m.math('MAXIMUM', m.math('SUBTRACT', m.math('ABSOLUTE', m.math('SUBTRACT', b, cb)), hb - r), 0.0)
    d = m.math('SQRT', m.math('ADD', m.math('MULTIPLY', da, da), m.math('MULTIPLY', db, db)))
    return smooth_less(m, d, r, soft)


def disc2(m, a, b, ca, cb, r, soft=0.0015):
    da, db = m.math('SUBTRACT', a, ca), m.math('SUBTRACT', b, cb)
    return smooth_less(m, m.math('SQRT', m.math('ADD', m.math('MULTIPLY', da, da), m.math('MULTIPLY', db, db))), r, soft)


def dist2(m, a, b, ca, cb):
    da, db = m.math('SUBTRACT', a, ca), m.math('SUBTRACT', b, cb)
    return m.math('SQRT', m.math('ADD', m.math('MULTIPLY', da, da), m.math('MULTIPLY', db, db)))


class Layers:
    """A finish built up in layers: color, roughness, metallic and a bump height, each laid over the last by a mask."""

    def __init__(self, m, color, rough, metal=0.0, height=0.0):
        self.m = m
        self.col = core.hex_linear(color) if isinstance(color, int) else color
        self.rough = rough
        self.metal = metal
        self.h = height

    def over(self, mask, color=None, rough=None, metal=None, height=None):
        m = self.m
        if color is not None:
            self.col = m.mix(mask, self.col, core.hex_linear(color) if isinstance(color, int) else color)
        if rough is not None:
            self.rough = mix_float(m, mask, self.rough, rough)
        if metal is not None:
            self.metal = mix_float(m, mask, self.metal, metal)
        if height is not None:
            self.h = m.math('ADD', self.h, m.math('MULTIPLY', mask, height)) if not isinstance(height, (int, float)) or height != 0.0 else self.h
        return self

    def apply(self, bump_strength=1.0, bump_distance=0.001, emission=None, strength=0.0):
        m = self.m
        m.set('Base Color', self.col)
        m.set('Roughness', self.rough)
        m.set('Metallic', self.metal)
        if not isinstance(self.h, (int, float)):
            b = m.node('ShaderNodeBump', Strength=bump_strength, Distance=bump_distance)
            m.link(self.h, b.inputs['Height'])
            m.set('Normal', b.outputs['Normal'])
        if emission is not None:
            m.set('Emission Color', emission)
            m.set('Emission Strength', strength)
        return m


# ------------------------------------------------------------------ livery and weather (shared by the paint and the decals)

WHITE, GREEN, GOLD, BLACK, SKIRT = 0xeceeed, 0x0f6b3a, 0xf2b705, 0x0c0d0f, 0x2a2d31


def livery(m, sep, nrm):
    """The city livery by position on the skin: (Layers, masks)."""
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    ay = m.math('ABSOLUTE', y)
    front_k = ramp(m, x, -5.80, -6.05)
    bb = m.math('SUBTRACT', BB, m.math('MULTIPLY', front_k, 0.06))
    g_top = m.math('SUBTRACT', bb, 0.045)
    sweep = ramp(m, x, 3.95, 5.75)
    gt_side = m.math('ADD', g_top, m.math('MULTIPLY', sweep, m.math('SUBTRACT', 2.95, g_top)))
    rear_gt = m.math('ADD', 1.70, m.math('MULTIPLY', ramp(m, ay, 0.90, 1.29), 1.25))
    gt = mix_float(m, ramp(m, x, 5.80, 6.00), gt_side, rear_gt)
    g0 = m.math('SUBTRACT', m.math('SUBTRACT', 0.80, m.math('MULTIPLY', ramp(m, x, -5.75, -6.00), 0.10)), m.math('MULTIPLY', ramp(m, x, 5.80, 6.0), 0.06))
    green = between(m, z, g0, gt, 0.0015)
    gold = between(m, z, m.math('ADD', gt, 0.010), m.math('ADD', gt, 0.032), 0.0012)
    bt = m.math('ADD', BT, m.math('MULTIPLY', ramp(m, x, -5.80, -5.97), 0.445))
    x_end = m.math('ADD', 4.13, m.math('MULTIPLY', m.math('GREATER_THAN', y, 0.0), 0.99))
    black = m.math('MULTIPLY', between(m, z, bb, bt, 0.0015), m.math('SUBTRACT', 1.0, ge(m, x, x_end, 0.002)))
    rear_face = ramp(m, x, 5.88, 5.95)
    rw = rrect2(m, y, z, 0.0, 2.245, 0.92, 0.365, 0.10)
    sb = rrect2(m, y, z, 0.0, 2.855, 0.25, 0.175, 0.03)
    black = m.math('MAXIMUM', black, m.math('MULTIPLY', m.math('MAXIMUM', rw, sb), rear_face))
    skirt = m.math('SUBTRACT', 1.0, ge(m, z, 0.70, 0.0015))
    L = Layers(m, WHITE, 0.17)
    L.over(green, GREEN, 0.19).over(gold, GOLD, 0.22).over(black, BLACK, 0.09).over(skirt, SKIRT, 0.36)
    return L, {'green': green, 'black': black, 'skirt': skirt, 'bb': bb}


def exterior_mask(m, sep):
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    ay = m.math('ABSOLUTE', y)
    return vmax(m, ramp(m, ay, 1.262, 1.285), ramp(m, m.math('MULTIPLY', x, -1.0), 5.995, 6.04), ramp(m, x, 5.99, 6.035), ramp(m, z, 3.10, 3.13))


def weather(m, L, sep, o, k=1.0, roof=True):
    """Road film rising from the bottom, salt spray behind the wheels and dried salt bloom, snowmelt streaks down
    from the window band and the roof edge, soot at the back, road grime and bugs on the nose, a dirty roof."""
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    blot = noise(m, o, scale=2.6, detail=5.0)
    film = mul(m, ramp(m, z, 1.0, 0.32), ramp(m, blot, 0.28, 0.78), k)
    streak = noise(m, scaled(m, sep, 9.0, 9.0, 0.55), detail=4.0)
    spray = None
    for ax in AXLES:
        d = m.math('SUBTRACT', x, ax)
        behind = m.math('MULTIPLY', ramp(m, d, 0.30, 0.60), ramp(m, d, 1.9, 0.9))
        front_sp = m.math('MULTIPLY', ramp(m, d, -0.35, -0.60), ramp(m, d, -1.3, -0.75))
        sp = mul(m, m.math('MAXIMUM', behind, m.math('MULTIPLY', front_sp, 0.5)), ramp(m, z, 1.25, 0.38), ramp(m, streak, 0.30, 0.72))
        spray = sp if spray is None else m.math('MAXIMUM', spray, sp)
    spray = m.math('MULTIPLY', spray, k)
    haze = mul(m, ramp(m, z, 1.0, 0.30), ramp(m, noise(m, o, scale=1.8, detail=4.0), 0.30, 0.80), 0.30)
    # Dried droplets: sparse, gathered in drifts and in the spray behind the wheels (not an even sprinkle).
    drift = ramp(m, noise(m, o, scale=3.2, detail=3.0), 0.42, 0.78)
    speck = mul(m, ramp(m, noise(m, o, scale=150.0, detail=2.0), 0.685, 0.735), ramp(m, z, 0.85, 0.32), 0.65)
    run = mul(m, ramp(m, noise(m, scaled(m, sep, 22.0, 22.0, 1.6), detail=3.0), 0.55, 0.75), ramp(m, z, 0.85, 0.40), 0.45)
    salt = mul(m, vmax(m, haze, m.math('MULTIPLY', speck, m.math('MINIMUM', m.math('ADD', m.math('MULTIPLY', drift, 0.55), spray), 1.0)),
                       m.math('MULTIPLY', run, spray)), k)
    drip = ramp(m, noise(m, scaled(m, sep, 34.0, 34.0, 0.9), detail=3.0), 0.56, 0.80)
    under_band = m.math('MULTIPLY', between(m, z, 0.78, BB - 0.01, 0.05), ramp(m, noise(m, scaled(m, sep, 3.0, 3.0, 0.3), detail=2.0), 0.35, 0.6))
    under_roof = between(m, z, 2.64, 2.93, 0.03)
    streaks = mul(m, drip, m.math('MAXIMUM', under_band, under_roof), 0.55, k)
    soot = mul(m, ramp(m, x, 5.55, 6.05), ramp(m, noise(m, o, scale=5.0, detail=4.0), 0.35, 0.85), ramp(m, z, 1.9, 0.5), k)
    bugs = mul(m, ramp(m, x, -5.92, -6.06), ramp(m, z, 1.25, 0.65), ramp(m, noise(m, o, scale=60.0, detail=2.0), 0.62, 0.72), k)
    L.over(film, 0x5d564b, 0.55).over(m.math('MULTIPLY', spray, 0.75), 0x4b453d, 0.62)
    L.over(salt, 0xc9c6bd, 0.74).over(streaks, 0x77736b, 0.4).over(m.math('MULTIPLY', soot, 0.35), 0x2b2926, 0.6).over(bugs, 0x3a2e22, 0.5)
    if roof:
        rmott = ramp(m, noise(m, o, scale=1.5, detail=5.0), 0.3, 0.85)
        rstreak = ramp(m, noise(m, scaled(m, sep, 0.6, 14.0, 1.0), detail=3.0), 0.4, 0.8)
        dirty = mul(m, ramp(m, z, 3.0, 3.10), m.math('MAXIMUM', m.math('MULTIPLY', rmott, 0.55), m.math('MULTIPLY', rstreak, 0.35)), k)
        L.over(dirty, 0x8b877e, 0.5)
    return L


def seam_sheets():
    """The shut lines and access doors drawn in four views (white on black), projected onto the paint by the way it
    faces (and into its normal map)."""
    LW = 4.0

    def stroke(s, pts, w=LW):
        for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
            L = math.hypot(x1 - x0, y1 - y0)
            if L < 1e-9:
                continue
            nx, ny = -(y1 - y0) / L * w / 2, (x1 - x0) / L * w / 2
            s.poly([(x0 + nx, y0 + ny), (x1 + nx, y1 + ny), (x1 - nx, y1 - ny), (x0 - nx, y0 - ny)], 0xffffff)

    def rect(s, x0, x1, z0, z1, r=30.0):
        pts = core.rounded_rect(x0, z0, x1, z1, r, 4)
        stroke(s, pts + [pts[0]])

    def side(name, right):
        s = Sheet(12400.0, 3400.0)
        s.rect(0, 0, 12400, 3400, 0x000000)

        def X(x):
            return (x + 6.2) * 1000.0
        seams = [-3.025, -1.555, 2.575, 4.005] if right else [-3.015, -1.445, 1.255, 2.675]
        # The lower panels under the CITY TRANSIT wordmark run unbroken (no joint through the lettering).
        wordmark = -1.555 if right else -1.445
        for sx in seams:
            if sx != wordmark:
                stroke(s, [(X(sx), 300.0), (X(sx), 1090.0)])
            stroke(s, [(X(sx), 2675.0), (X(sx), 2890.0)])
        for sx in ([-5.30, 0.55, 1.65, 4.75] if right else [-5.30, -4.70, 0.12, 4.40]):
            stroke(s, [(X(sx), 2675.0), (X(sx), 2890.0)])
        rect(s, X(4.30), X(5.76), 330.0, 980.0)       # the engine bay's side door
        stroke(s, [(X(4.33), 940.0), (X(5.73), 940.0)], 3.0)
        if right:
            rect(s, X(5.20), X(5.42), 1040.0, 1225.0, 25.0)     # the fuel door
        else:
            rect(s, X(-0.85), X(0.35), 330.0, 960.0)            # electrical compartment
            rect(s, X(-2.85), X(-1.75), 330.0, 960.0)           # the battery cooling compartment
            rect(s, X(-5.55), X(-4.70), 330.0, 960.0)           # the front service door
        return s.render(name, 2048 if FAST else 8192)[0]

    def face(name, rear):
        s = Sheet(2800.0, 3400.0)
        s.rect(0, 0, 2800, 3400, 0x000000)

        def Y(y):
            return (y + 1.4) * 1000.0
        if rear:
            rect(s, Y(-0.98), Y(0.98), 690.0, 1640.0, 40.0)    # the engine door
            stroke(s, [(Y(-0.95), 1600.0), (Y(0.95), 1600.0)], 3.0)
        else:
            rect(s, Y(-0.96), Y(0.96), 700.0, 1045.0, 30.0)    # the front cap's service panel
        stroke(s, [(Y(-1.2), 690.0 if not rear else 680.0), (Y(1.2), 690.0 if not rear else 680.0)])
        return s.render(name, 1024 if FAST else 2048)[0]

    def top(name):
        s = Sheet(12400.0, 2800.0)
        s.rect(0, 0, 12400, 2800, 0x000000)
        for x in (-5.0, -3.4, -1.9, -0.4, 1.40, 2.6, 4.1, 5.6):
            stroke(s, [((x + 6.2) * 1000.0, 300.0), ((x + 6.2) * 1000.0, 2500.0)], 5.0)
        return s.render(name, 2048 if FAST else 4096)[0]

    return {'side_r': side('ecx_bus_seams_r', True), 'side_l': side('ecx_bus_seams_l', False), 'front': face('ecx_bus_seams_f', False),
            'rear': face('ecx_bus_seams_b', True), 'top': top('ecx_bus_seams_t')}


def sample(m, path, u, v):
    n = m.image(path, non_color=True, vector=vec(m, u, v, 0.0), extension='CLIP')
    sp = m.node('ShaderNodeSeparateColor')
    m.link(n.outputs['Color'], sp.inputs['Color'])
    return sp.outputs['Red']


def seam_mask(m, sep, nrm, sheets):
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    nx, ny, nz = nrm
    us = m.math('DIVIDE', m.math('ADD', x, 6.2), 12.4)
    vs = m.math('DIVIDE', z, 3.4)
    side = mix_float(m, m.math('GREATER_THAN', y, 0.0), sample(m, sheets['side_l'], us, vs), sample(m, sheets['side_r'], us, vs))
    side = m.math('MULTIPLY', side, ramp(m, m.math('ABSOLUTE', ny), 0.5, 0.7))
    uf = m.math('DIVIDE', m.math('ADD', y, 1.4), 2.8)
    front = m.math('MULTIPLY', sample(m, sheets['front'], uf, vs), ramp(m, m.math('MULTIPLY', nx, -1.0), 0.5, 0.7))
    rear = m.math('MULTIPLY', sample(m, sheets['rear'], uf, vs), ramp(m, nx, 0.5, 0.7))
    top = m.math('MULTIPLY', sample(m, sheets['top'], us, uf), ramp(m, nz, 0.6, 0.8))
    return vmax(m, side, front, rear, top)


def paint_material(sheets):
    m = core.Mat('ecx_bus_paint')
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    nrm = geo_normal(m)
    L, masks = livery(m, sep, nrm)
    seams = seam_mask(m, sep, nrm, sheets)
    # Orange peel in the clear, and scuffs and chips on the corners and arch edges.
    peel = noise(m, o, scale=260.0, detail=1.0)
    edge = convex_edges(m, tc, distance=0.006, samples=12, breakup_scale=30.0, gain=4.0)
    chips = mul(m, ramp(m, edge, 0.45, 0.65), ramp(m, sep.outputs['Z'], 1.4, 0.9))
    L.over(chips, 0x9a9ea2, 0.5)
    weather(m, L, sep, o)
    L.over(seams, 0x1a1a1b, 0.6)
    L.h = m.math('ADD', m.math('MULTIPLY', seams, -0.0012), m.math('MULTIPLY', peel, 0.00006))
    return L.apply(1.0, 1.0)


def prints_material(atlas, sheets):
    """The decals: the atlas's ink by UV over the livery by position (exterior decals print on the paint). The livery,
    the weather and the shut lines are read at the skin under the decal (its position less DECAL_E along its normal), so
    they run on through it exactly as on the paint round it and the vinyl's edge doesn't show as a box; a shut line
    under a decal cuts it, as the vinyl is cut at the panel's joint."""
    m = core.Mat('ecx_bus_prints')
    tc, _ = object_coords(m)
    geo = m.node('ShaderNodeNewGeometry')
    skin = m.node('ShaderNodeVectorMath', _operation='MULTIPLY_ADD')
    m.link(geo.outputs['True Normal'], skin.inputs[0])
    skin.inputs[1].default_value = (-DECAL_E, -DECAL_E, -DECAL_E)
    m.link(tc.outputs['Object'], skin.inputs[2])
    o = skin.outputs['Vector']
    sep = m.node('ShaderNodeSeparateXYZ')
    m.link(o, sep.inputs['Vector'])
    nrm = geo_normal(m)
    img = m.image(atlas[0], vector=tc.outputs['UV'], extension='EXTEND')
    surf = m.image(atlas[1], non_color=True, vector=tc.outputs['UV'], extension='EXTEND')
    ss = m.node('ShaderNodeSeparateColor')
    m.link(surf.outputs['Color'], ss.inputs['Color'])
    L, _ = livery(m, sep, nrm)
    alpha = img.outputs['Alpha']
    L.col = m.mix(alpha, L.col, img.outputs['Color'])
    L.rough = mix_float(m, alpha, L.rough, ss.outputs['Green'])
    L.metal = mix_float(m, alpha, 0.0, ss.outputs['Red'])
    ext = exterior_mask(m, sep)
    weather(m, L, sep, o, k=ext, roof=True)
    seams = m.math('MULTIPLY', seam_mask(m, sep, nrm, sheets), ext)
    L.over(seams, 0x1a1a1b, 0.6)
    # The vinyl's edge stands a hair proud of the paint; the paint's orange peel and shut lines show through it.
    peel = m.math('MULTIPLY', noise(m, o, scale=260.0, detail=1.0), ext)
    L.h = sum_heights(m, [m.math('MULTIPLY', alpha, 0.00015), m.math('MULTIPLY', seams, -0.0012), m.math('MULTIPLY', peel, 0.00006)])
    return L.apply(1.0, 1.0)


def trim_material():
    """Black: textured plastic bumpers and housings, rubber seals and reveals, the wheel-house liners: scuffed on
    the corners, salt crusted low down."""
    m = core.Mat('ecx_bus_trim')
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    grain = noise(m, o, scale=900.0, detail=1.0)
    L = Layers(m, m.mix(m.math('MULTIPLY', grain, 0.5), core.hex_linear(0x121315), core.hex_linear(0x1a1b1e)), 0.62)
    edge = convex_edges(m, tc, distance=0.004, samples=12, breakup_scale=40.0, gain=4.0)
    L.over(mul(m, ramp(m, edge, 0.4, 0.7), 0.7), 0x3c3e42, 0.5)
    weather(m, L, sep, o, k=0.55, roof=False)
    L.h = m.math('MULTIPLY', grain, 0.00008)
    return L.apply(1.0, 1.0)


def metal_material():
    """Aluminium rub rails (brushed along the bus), stainless and chrome: the bike rack, the mirrors' glass, hinges,
    the exhaust. Salt and grime low down; the mirror glass polished."""
    m = core.Mat('ecx_bus_metal')
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    brush = noise(m, scaled(m, sep, 4.0, 400.0, 400.0), detail=4.0)
    L = Layers(m, m.mix(m.math('MULTIPLY', brush, 0.3), core.hex_linear(0xb9bdc1), core.hex_linear(0xd3d7da)),
               m.math('ADD', 0.24, m.math('MULTIPLY', brush, 0.08)), 1.0)
    smudge = ramp(m, noise(m, o, scale=8.0, detail=4.0), 0.5, 0.75)
    L.over(m.math('MULTIPLY', smudge, 0.5), 0x8f9397, 0.42)
    mirror = mul(m, ramp(m, m.math('ABSOLUTE', y), 1.33, 1.36), between(m, z, 1.28, 2.30, 0.01), ramp(m, x, -5.86, -5.92))
    L.over(mirror, 0xdfe3e6, 0.025, 1.0)
    salt = mul(m, ramp(m, z, 1.0, 0.4), ramp(m, noise(m, o, scale=18.0, detail=5.0), 0.55, 0.68))
    L.over(salt, 0xcfcbc2, 0.8, 0.2)
    grime = mul(m, ramp(m, z, 0.9, 0.35), ramp(m, noise(m, o, scale=3.0, detail=4.0), 0.3, 0.8), 0.6)
    L.over(grime, 0x55504a, 0.6, 0.4)
    return L.apply()


def lamps_material(display):
    """The lamps' lenses and what's behind them, by position, and what glows: headlamps and their light bars white,
    park and marker lamps amber, tail and rear markers red, the plate lamp; inside, the next-stop display, the
    farebox's screen and the dash's gauges."""
    m = core.Mat('ecx_bus_lamps')
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    ay = m.math('ABSOLUTE', y)
    L = Layers(m, 0x15161a, 0.3, 0.0)
    em = core.hex_linear(0x000000)
    front = ramp(m, x, -5.86, -5.92)
    rear = ramp(m, x, 5.86, 5.92)
    side = ramp(m, ay, 1.28, 1.30)
    top = ramp(m, z, 2.80, 2.86)
    # Front: the head lamp clusters (two projectors, a light bar), amber park lamps round the corners.
    head = mul(m, front, between(m, z, 0.695, 0.935, 0.002), between(m, ay, 0.62, 1.125, 0.002), m.math('SUBTRACT', 1.0, top))
    # A smoked lens over a black housing; a satin chrome strip along the top.
    L.over(head, 0x121417, 0.06, 0.2)
    L.over(mul(m, head, between(m, z, 0.905, 0.918, 0.001)), 0xb9bec3, 0.12, 1.0)
    proj, cores = None, None
    for cy in (0.80, 0.975):
        d = dist2(m, ay, z, cy, 0.83)
        rim = smooth_less(m, d, 0.064, 0.0012)
        bowl = smooth_less(m, d, 0.058, 0.0012)
        c = smooth_less(m, d, 0.028, 0.0012)
        ring = m.math('SUBTRACT', smooth_less(m, d, 0.034, 0.001), c)
        proj = bowl if proj is None else m.math('MAXIMUM', proj, bowl)
        cores = c if cores is None else m.math('MAXIMUM', cores, c)
        flutes = ramp(m, m.math('ABSOLUTE', m.math('SINE', m.math('MULTIPLY', m.math('ARCTAN2', m.math('SUBTRACT', z, 0.83), m.math('SUBTRACT', ay, cy)), 6.0))), 0.3, 0.9)
        L.over(m.math('MULTIPLY', head, m.math('SUBTRACT', rim, bowl)), 0x08090a, 0.3, 0.0)
        L.over(m.math('MULTIPLY', head, m.math('SUBTRACT', bowl, c)), m.mix(m.math('MULTIPLY', flutes, 0.35), core.hex_linear(0xd9dde1), core.hex_linear(0x9ea4aa)),
               0.05, 1.0, m.math('ADD', m.math('MULTIPLY', ramp(m, d, 0.058, 0.028), -0.006), m.math('MULTIPLY', flutes, 0.0004)))
        L.over(m.math('MULTIPLY', head, ring), 0x2a2d31, 0.2, 0.6)
    bar = vmax(m, mul(m, head, between(m, z, 0.708, 0.722, 0.0012), between(m, ay, 0.66, 1.105, 0.0015)),
               mul(m, head, between(m, z, 0.708, 0.895, 0.0012), between(m, ay, 1.092, 1.105, 0.0012)))
    L.over(m.math('MULTIPLY', head, cores), 0xeef4ff, 0.04, 0.0, m.math('MULTIPLY', m.math('MULTIPLY', head, cores), 0.003))
    L.over(bar, 0xf2f5f8, 0.12, 0.0)
    park = mul(m, front, between(m, z, 0.70, 0.925, 0.002), ge(m, ay, 1.13, 0.002), m.math('SUBTRACT', 1.0, top))
    # Behind the clear amber cover, an array of LEDs in reflector cups (about 20 mm pitch, ten rows), counted round the
    # corner along the lens (|y| on the front, then x on the side). Only the LEDs and their cups glow.
    cu = m.math('DIVIDE', m.math('ADD', ay, m.math('MULTIPLY', m.math('ADD', x, 6.06), 0.8)), 0.0205)
    cv = m.math('DIVIDE', m.math('SUBTRACT', z, 0.70), 0.0225)
    fu = m.math('SUBTRACT', m.math('FRACT', cu), 0.5)
    fv = m.math('SUBTRACT', m.math('FRACT', cv), 0.5)
    dcell = m.math('SQRT', m.math('ADD', m.math('MULTIPLY', fu, fu), m.math('MULTIPLY', fv, fv)))
    led = smooth_less(m, dcell, 0.17, 0.05)
    cup = m.math('SUBTRACT', smooth_less(m, dcell, 0.42, 0.05), led)
    lens = m.mix(cup, core.hex_linear(0x6e2804), core.hex_linear(0xd96a12))
    lens = m.mix(led, lens, core.hex_linear(0xffc066))
    prisms = ramp(m, m.math('ABSOLUTE', m.math('SINE', m.math('MULTIPLY', z, 420.0))), 0.2, 0.95)
    L.over(park, lens, 0.05, 0.0, m.math('MULTIPLY', park, m.math('MULTIPLY', prisms, 0.0002)))
    # Rear clusters: red stop/tail, amber turn, white backup, round LED lamps; the high-mount stop lamp.
    tail_d = dist2(m, ay, z, 0.91, 1.32)
    turn_d = dist2(m, ay, z, 0.91, 1.10)
    back_d = dist2(m, ay, z, 0.91, 0.88)
    leds = ramp(m, m.math('ABSOLUTE', m.math('SINE', m.math('MULTIPLY', ay, 260.0))), 0.6, 0.9)
    leds = m.math('MULTIPLY', leds, ramp(m, m.math('ABSOLUTE', m.math('SINE', m.math('MULTIPLY', z, 260.0))), 0.6, 0.9))
    tail = mul(m, rear, smooth_less(m, tail_d, 0.08, 0.002), m.math('SUBTRACT', 1.0, top))
    turn = mul(m, rear, smooth_less(m, turn_d, 0.08, 0.002))
    back = mul(m, rear, smooth_less(m, back_d, 0.08, 0.002))
    chmsl = mul(m, rear, between(m, z, 2.60, 2.68, 0.002))
    L.over(tail, 0x9c0a10, 0.1).over(turn, 0xc96f12, 0.1).over(back, 0xdadde0, 0.08).over(chmsl, 0x7a0a0e, 0.1)
    L.over(mul(m, m.math('MAXIMUM', m.math('MAXIMUM', tail, turn), back), leds, 0.35), 0xffffff, 0.2)
    # Markers: amber at the front and along the sides, red at the back; clearance and ID lamps on top.
    marker = m.math('MAXIMUM', side, top)
    reddish = ge(m, x, 4.8, 0.05)
    L.over(m.math('MULTIPLY', marker, m.math('SUBTRACT', 1.0, reddish)), 0xd68a22, 0.1)
    L.over(m.math('MULTIPLY', marker, reddish), 0xa30d12, 0.1)
    plate = mul(m, rear, between(m, z, 0.885, 0.925, 0.002), between(m, y, -0.62, -0.48, 0.002))
    L.over(plate, 0xe8ecef, 0.2)
    # Inside: the next-stop display over the cab, the farebox screen, the gauges.
    inside = m.math('SUBTRACT', 1.0, vmax(m, front, rear, side, top))
    disp = mul(m, between(m, x, -4.53, -4.48, 0.002), between(m, z, 2.70, 2.81, 0.002), m.math('SUBTRACT', 1.0, front))
    u = m.math('DIVIDE', m.math('ADD', y, 0.40), 0.80)
    v = m.math('DIVIDE', m.math('SUBTRACT', z, 2.715), 0.08)
    dimg = m.image(display, vector=vec(m, u, v, 0.0), extension='CLIP')
    L.over(disp, 0x050505, 0.15)
    fare = mul(m, inside, between(m, z, 1.02, 1.30, 0.002), between(m, x, -5.35, -4.90, 0.01), between(m, y, -0.20, 0.30, 0.01))
    gauge = mul(m, inside, between(m, z, 1.03, 1.17, 0.002), ge(m, m.math('MULTIPLY', x, -1.0), 5.50, 0.01), ge(m, m.math('MULTIPLY', y, -1.0), 0.40, 0.01))
    L.over(fare, 0x0b2a33, 0.1).over(gauge, 0x101418, 0.15)
    # What glows.
    glow = m.mix(m.math('MULTIPLY', head, cores), em, core.hex_linear(0xffffff))
    glow = m.mix(bar, glow, core.hex_linear(0xf2f6ff))
    glow = m.mix(mul(m, park, vmax(m, led, m.math('MULTIPLY', cup, 0.35))), glow, core.hex_linear(0xff7410))
    glow = m.mix(mul(m, tail, m.math('ADD', 0.30, m.math('MULTIPLY', leds, 0.7))), glow, core.hex_linear(0xff1a10))
    glow = m.mix(m.math('MULTIPLY', m.math('MULTIPLY', marker, m.math('SUBTRACT', 1.0, reddish)), 0.7), glow, core.hex_linear(0xffa030))
    glow = m.mix(m.math('MULTIPLY', m.math('MULTIPLY', marker, reddish), 0.7), glow, core.hex_linear(0xff2010))
    glow = m.mix(plate, glow, core.hex_linear(0xc8d0d8))
    glow = m.mix(disp, glow, m.mix(0.85, core.hex_linear(0x000000), dimg.outputs['Color'], blend='MIX'))
    glow = m.mix(m.math('MULTIPLY', fare, 0.6), glow, core.hex_linear(0x3fd6ff))
    glow = m.mix(m.math('MULTIPLY', gauge, 0.25), glow, core.hex_linear(0xffd28a))
    L.col = m.mix(disp, L.col, m.mix(0.5, core.hex_linear(0x050505), dimg.outputs['Color']))
    return L.apply(0.6, 1.0, glow, 6.0)


def interior_material():
    """Everything inside, told apart by the faces' 'ipart': walls, ceiling, the speckled rubber floor (worn along
    the aisle, salt and wet tracks from the doors), the moquette, seat shells, the yellow stanchions (worn where
    hands go), stainless, housings, the cab, rubber nosings, aluminium, small colored parts."""
    m = core.Mat('ecx_bus_interior')
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    nx, ny, nz = geo_normal(m)
    ip = attr(m, 'ipart')

    def is_(k):
        return m.math('SUBTRACT', 1.0, m.math('MINIMUM', m.math('MULTIPLY', m.math('ABSOLUTE', m.math('SUBTRACT', ip, float(k))), 4.0), 1.0))

    fine = noise(m, o, scale=700.0, detail=1.0)
    mott = noise(m, o, scale=6.0, detail=4.0)
    edge = convex_edges(m, tc, distance=0.003, samples=8, breakup_scale=60.0, gain=4.0)
    # Walls: light grey laminate, a darker kick panel low down; ceiling: off-white.
    wall = m.mix(m.math('MULTIPLY', mott, 0.3), core.hex_linear(0xbcbfc1), core.hex_linear(0xa9acae))
    kick = m.math('SUBTRACT', 1.0, ge(m, z, 0.74, 0.004))
    L = Layers(m, m.mix(kick, wall, core.hex_linear(0x63666a)), 0.55, 0.0, m.math('MULTIPLY', fine, 0.00006))
    L.over(mul(m, ramp(m, noise(m, o, scale=14.0, detail=5.0), 0.62, 0.75), ramp(m, z, 1.2, 0.5)), 0x7b7a76, 0.6)
    L.over(is_(IP_CEIL), m.mix(m.math('MULTIPLY', mott, 0.2), core.hex_linear(0xdedcd6), core.hex_linear(0xcfcdc7)), 0.62)
    # Floor: grey rubber with light and dark flecks; the aisle worn smoother; salt and wet tracks from the doors.
    vor = m.node('ShaderNodeTexVoronoi', Scale=180.0)
    m.link(o, vor.inputs['Vector'])
    fleck_c = m.node('ShaderNodeSeparateColor')
    m.link(vor.outputs['Color'], fleck_c.inputs['Color'])
    fleck = smooth_less(m, vor.outputs['Distance'], 0.22, 0.05)
    floor = m.mix(m.math('MULTIPLY', fleck, ramp(m, fleck_c.outputs['Red'], 0.55, 0.6)), core.hex_linear(0x515457), core.hex_linear(0x8d9093))
    floor = m.mix(m.math('MULTIPLY', fleck, ramp(m, fleck_c.outputs['Red'], 0.25, 0.2)), floor, core.hex_linear(0x2b2d2f))
    aisle = m.math('MULTIPLY', ramp(m, m.math('ABSOLUTE', y), 0.45, 0.2), ramp(m, mott, 0.2, 0.7))
    doors = vmax(m, mul(m, between(m, x, DOOR_F[0] - 0.2, DOOR_F[1] + 0.6, 0.3), ramp(m, y, -0.6, 0.9)),
                 mul(m, between(m, x, DOOR_R[0] - 0.4, DOOR_R[1] + 0.4, 0.3), ramp(m, y, -0.4, 0.9)))
    salt = mul(m, doors, ramp(m, noise(m, o, scale=9.0, detail=6.0), 0.48, 0.66))
    wet = mul(m, doors, ramp(m, noise(m, o, scale=4.0, detail=5.0), 0.55, 0.7))
    line = mul(m, between(m, x, -4.66, -4.61, 0.002), ramp(m, m.math('ABSOLUTE', y), 0.9, 0.85))
    fl = is_(IP_FLOOR)
    L.over(fl, floor, m.math('ADD', 0.62, m.math('MULTIPLY', fine, 0.08)), 0.0)
    L.over(m.math('MULTIPLY', fl, aisle), 0x6a6d70, 0.42).over(m.math('MULTIPLY', fl, salt), 0xc9c6bd, 0.8)
    L.over(m.math('MULTIPLY', fl, wet), 0x35373a, 0.12).over(m.math('MULTIPLY', fl, line), 0xe8b81c, 0.5)
    # Moquette: city green with grey diagonals, gold flecks and black rules, a ~45 mm motif; worn at the front edges.
    pz = ramp(m, m.math('ABSOLUTE', nz), 0.55, 0.7)
    px_ = ramp(m, m.math('ABSOLUTE', nx), 0.55, 0.7)
    pa = mix_float(m, pz, mix_float(m, px_, x, y), x)
    pb = mix_float(m, pz, z, y)
    diag = m.math('FRACT', m.math('DIVIDE', m.math('ADD', pa, pb), 0.09))
    stripe = m.math('MULTIPLY', ge(m, diag, 0.12, 0.02), m.math('SUBTRACT', 1.0, ge(m, diag, 0.34, 0.02)))
    rule = m.math('MULTIPLY', ge(m, diag, 0.62, 0.01), m.math('SUBTRACT', 1.0, ge(m, diag, 0.66, 0.01)))
    fa = m.math('FRACT', m.math('DIVIDE', pa, 0.045))
    fb = m.math('FRACT', m.math('DIVIDE', pb, 0.045))
    dot = m.math('MULTIPLY', between(m, fa, 0.42, 0.58, 0.02), between(m, fb, 0.42, 0.58, 0.02))
    moq = m.mix(stripe, core.hex_linear(0x16402a), core.hex_linear(0x585c60))
    moq = m.mix(rule, moq, core.hex_linear(0x0b0d0c))
    moq = m.mix(m.math('MULTIPLY', dot, m.math('SUBTRACT', 1.0, stripe)), moq, core.hex_linear(0xc9a227))
    moq = m.mix(m.math('MULTIPLY', ramp(m, edge, 0.35, 0.7), 0.5), moq, core.hex_linear(0x6e7570))
    weave = noise(m, o, scale=1400.0, detail=1.0)
    L.over(is_(IP_MOQ), moq, 0.92, 0.0, m.math('MULTIPLY', is_(IP_MOQ), m.math('MULTIPLY', weave, 0.0004)))
    # Plastics and metals.
    scuff = m.math('MULTIPLY', ramp(m, edge, 0.4, 0.7), 0.6)
    L.over(is_(IP_SHELL), m.mix(scuff, core.hex_linear(0x44474b), core.hex_linear(0x6d7075)), 0.48)
    yel = m.mix(m.math('MULTIPLY', between(m, z, 1.05, 1.85, 0.15), ramp(m, noise(m, o, scale=40.0, detail=3.0), 0.4, 0.7)),
                core.hex_linear(0xf0b414), core.hex_linear(0xf6cc5a))
    L.over(is_(IP_YELLOW), yel, mix_float(m, between(m, z, 1.05, 1.85, 0.15), 0.38, 0.22), 0.0)
    brushed = noise(m, scaled(m, sep, 300.0, 300.0, 4.0), detail=3.0)
    L.over(is_(IP_STEEL), m.mix(m.math('MULTIPLY', brushed, 0.3), core.hex_linear(0xa9adb1), core.hex_linear(0xc4c8cc)),
           m.math('ADD', 0.26, m.math('MULTIPLY', brushed, 0.08)), 1.0)
    L.over(is_(IP_HOUSING), m.mix(scuff, core.hex_linear(0x5b5f64), core.hex_linear(0x7c8085)), 0.55, 0.0)
    L.over(is_(IP_DASH), m.mix(m.math('MULTIPLY', fine, 0.4), core.hex_linear(0x18191c), core.hex_linear(0x222327)), 0.55, 0.0)
    L.over(is_(IP_VINYL), m.mix(m.math('MULTIPLY', scuff, 0.6), core.hex_linear(0x1f2023), core.hex_linear(0x3a3b3f)), 0.5, 0.0)
    ribs = ramp(m, m.math('ABSOLUTE', m.math('SINE', m.math('MULTIPLY', m.math('ADD', x, y), 320.0))), 0.4, 0.9)
    L.over(is_(IP_YRUB), m.mix(m.math('MULTIPLY', ribs, 0.3), core.hex_linear(0xe2b21b), core.hex_linear(0xb88f14)), 0.7, 0.0,
           m.math('MULTIPLY', is_(IP_YRUB), m.math('MULTIPLY', ribs, 0.0006)))
    L.over(is_(IP_ALU), 0xa7abaf, 0.32, 1.0).over(is_(IP_BLACK), 0x0e0f11, 0.4, 0.0)
    L.over(is_(IP_RED), 0xc21c26, 0.3, 0.0).over(is_(IP_BLUE), m.mix(m.math('MULTIPLY', fine, 0.5), core.hex_linear(0x22335a), core.hex_linear(0x2c4170)), 0.75, 0.0)
    L.over(is_(IP_FRAME), m.mix(scuff, core.hex_linear(0x2a2c30), core.hex_linear(0x55585d)), 0.42, 0.3)
    perf = mul(m, ramp(m, m.math('ABSOLUTE', m.math('SINE', m.math('MULTIPLY', x, 180.0))), 0.85, 0.95),
               ramp(m, m.math('ABSOLUTE', m.math('SINE', m.math('MULTIPLY', z, 180.0))), 0.85, 0.95))
    L.over(is_(IP_DUCT), m.mix(perf, core.hex_linear(0x7b7f84), core.hex_linear(0x24262a)), 0.5, 0.3)
    L.over(is_(IP_CARD), 0xe4e0d6, 0.7, 0.0)
    return L.apply(1.0, 1.0)


def door_materials():
    fr = core.Mat('ecx_door_frame')
    tc, sep = object_coords(fr)
    o = tc.outputs['Object']
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    brush = noise(fr, scaled(fr, sep, 300.0, 300.0, 3.0), detail=3.0)
    L = Layers(fr, fr.mix(fr.math('MULTIPLY', brush, 0.4), core.hex_linear(0x141518), core.hex_linear(0x1c1d21)), 0.42, 0.8)
    edge = convex_edges(fr, tc, distance=0.003, samples=8, breakup_scale=60.0, gain=4.0)
    L.over(fr.math('MULTIPLY', ramp(fr, edge, 0.4, 0.7), 0.6), 0x6b6e73, 0.35, 1.0)
    kicks = mul(fr, ramp(fr, z, 0.35, 0.08), ramp(fr, noise(fr, scaled(fr, sep, 40.0, 40.0, 6.0), detail=4.0), 0.5, 0.7))
    L.over(kicks, 0x55585c, 0.5, 0.6)
    L.over(mul(fr, ramp(fr, z, 0.45, 0.04), vmax(fr, fr.math('MULTIPLY', ramp(fr, noise(fr, o, scale=150.0, detail=2.0), 0.685, 0.735), 0.5),
                                                fr.math('MULTIPLY', ramp(fr, noise(fr, o, scale=3.0, detail=4.0), 0.4, 0.8), 0.3))), 0xc9c6bd, 0.8, 0.1)
    handle = mul(fr, ramp(fr, y, -0.03, -0.045), between(fr, x, 0.25, 0.46, 0.005), between(fr, z, 0.9, 1.8, 0.01))
    L.over(handle, 0xc0c4c8, 0.2, 1.0)
    L.apply()
    gl = core.Mat('ecx_door_glass')
    tc, sep = object_coords(gl)
    film = ramp(gl, noise(gl, tc.outputs['Object'], scale=8.0, detail=4.0), 0.5, 0.8)
    gl.set('Base Color', core.hex_linear(0x0b0e12))
    gl.set('Roughness', gl.math('ADD', 0.04, gl.math('MULTIPLY', film, 0.1)))
    gl.set('Specular IOR Level', 0.6)
    rb = core.Mat('ecx_door_rubber')
    tc, sep = object_coords(rb)
    g = noise(rb, tc.outputs['Object'], scale=600.0, detail=1.0)
    rb.set('Base Color', rb.mix(rb.math('MULTIPLY', g, 0.5), core.hex_linear(0x0e0e0f), core.hex_linear(0x18181a)))
    rb.set('Roughness', 0.78)
    return fr, gl, rb


def tire_material(name, text, outer_y, track):
    """Rubber: the ribs' grooves and sipes in the tread, the outer sidewall's raised lettering (read clockwise from
    outside), road dust and dried salt."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    r = m.math('SQRT', m.math('ADD', m.math('MULTIPLY', x, x), m.math('MULTIPLY', z, z)))
    ang = m.math('ARCTAN2', z, x)
    u = m.math('FRACT', m.math('DIVIDE', ang, 2 * math.pi / 2.0))
    v = m.math('DIVIDE', m.math('SUBTRACT', r, 0.385), 0.068)
    letters = m.math('MULTIPLY', sample(m, text, u, v), ge(m, y, outer_y, 0.004))
    tread = ramp(m, r, 0.498, 0.505)
    ly = m.math('ABSOLUTE', m.math('SUBTRACT', m.math('ABSOLUTE', y), track))
    sipe = mul(m, ramp(m, m.math('ABSOLUTE', m.math('SINE', m.math('ADD', m.math('MULTIPLY', ang, 48.0), m.math('MULTIPLY', y, 12.0)))), 0.94, 0.98),
               tread, ramp(m, ly, 0.13, 0.11))
    rib = m.math('MULTIPLY', smooth_less(m, m.math('ABSOLUTE', m.math('SUBTRACT', r, 0.315)), 0.004, 0.001), ge(m, m.math('ABSOLUTE', m.math('SUBTRACT', m.math('ABSOLUTE', y), track)), 0.07, 0.01))
    dust = mul(m, ramp(m, noise(m, o, scale=10.0, detail=4.0), 0.4, 0.85), ramp(m, r, 0.46, 0.33))
    salt = mul(m, vmax(m, m.math('MULTIPLY', ramp(m, noise(m, o, scale=160.0, detail=2.0), 0.69, 0.74), 0.35),
                       m.math('MULTIPLY', ramp(m, noise(m, o, scale=4.0, detail=4.0), 0.4, 0.8), 0.3)), ramp(m, r, 0.50, 0.36))
    L = Layers(m, 0x111112, mix_float(m, tread, 0.82, 0.7))
    L.over(m.math('MULTIPLY', dust, 0.6), 0x34302a, 0.9).over(m.math('MULTIPLY', salt, 0.7), 0xa9a69e, 0.9).over(letters, 0x2a2a2b, 0.75).over(sipe, 0x060606, 0.9)
    L.h = sum_heights(m, [m.math('MULTIPLY', letters, 0.0007), m.math('MULTIPLY', rib, 0.0006), m.math('MULTIPLY', sipe, -0.0012),
                          m.math('MULTIPLY', noise(m, o, scale=400.0, detail=1.0), 0.00008)])
    return L.apply(1.0, 1.0)


def sum_heights(m, hs):
    out = hs[0]
    for h in hs[1:]:
        out = m.math('ADD', out, h)
    return out


def rim_material(name, dual):
    """Polished aluminium (mirror bright), brake dust and road grime gathered in the dish and round the hand holes,
    salt spots; on the dual, the inner rim is painted dark grey."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    ao = m.node('ShaderNodeAmbientOcclusion', Distance=0.04)
    ao.samples = 16
    ao.only_local = True
    cav = ramp(m, m.math('SUBTRACT', 1.0, ao.outputs['AO']), 0.15, 0.6)
    L = Layers(m, 0xd3d7db, m.math('ADD', 0.05, m.math('MULTIPLY', noise(m, o, scale=60.0, detail=3.0), 0.04)), 1.0)
    L.over(m.math('MULTIPLY', cav, 0.4), 0x5a5249, 0.45, 0.6)
    L.over(mul(m, ramp(m, noise(m, o, scale=140.0, detail=2.0), 0.68, 0.74), 0.45), 0xc5c1b8, 0.7, 0.2)
    if dual:
        inner = m.math('SUBTRACT', 1.0, ge(m, y, 0.0, 0.004))
        L.over(inner, 0x2c2e31, 0.5, 0.2)
    return L.apply()


def hub_material(name):
    """Chrome nut covers and hub cap; the iron drum behind, rusty."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    r = m.math('SQRT', m.math('ADD', m.math('MULTIPLY', x, x), m.math('MULTIPLY', z, z)))
    L = Layers(m, 0xd9dde1, 0.06, 1.0)
    drum = m.math('MULTIPLY', ge(m, r, 0.165, 0.004), m.math('SUBTRACT', 1.0, ge(m, y, 0.05, 0.004)))
    rust = ramp(m, noise(m, o, scale=24.0, detail=5.0), 0.45, 0.7)
    L.over(drum, m.mix(rust, core.hex_linear(0x3b3c3e), core.hex_linear(0x5a3520)), 0.75, 0.4)
    L.over(mul(m, ramp(m, noise(m, o, scale=12.0, detail=4.0), 0.55, 0.75), 0.5), 0x6a645a, 0.5, 0.5)
    return L.apply()


def unbaked_materials():
    def glass(name, tint, rough=0.02):
        m = core.Mat(name)
        m.set('Base Color', core.hex_linear(tint))
        m.set('Roughness', rough)
        m.set('Transmission Weight', 1.0)
        m.set('IOR', 1.52)
        m.m.use_backface_culling = False
        return m

    def screen(name):
        m = core.Mat(name)
        m.set('Base Color', core.hex_linear(0x050505))
        m.set('Emission Color', core.hex_linear(0xffb000))
        m.set('Emission Strength', 1.0)
        m.set('Roughness', 0.3)
        m.m.use_backface_culling = True
        return m
    return glass('ecx_glass', 0xeef2f4), glass('ecx_glass_tint', 0x8f9497), screen('ecx_sign_dest'), screen('ecx_sign_route')


# ------------------------------------------------------------------ printed things

def save_png(path, rgb):
    """rgb: (h, w, 3) floats, row 0 at the top."""
    h, w, _ = rgb.shape
    data = (np.clip(rgb, 0.0, 1.0) * 255.0 + 0.5).astype(np.uint8)
    raw = b''.join(b'\x00' + data[r].tobytes() for r in range(h))

    def chunk(tag, body):
        return struct.pack('>I', len(body)) + tag + body + struct.pack('>I', zlib.crc32(tag + body) & 0xffffffff)
    png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b'')
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'wb') as f:
        f.write(png)


def coverage(name, cols, rows, items, ss=8):
    """Renders text items [(text, x, y, size, face, align, sx)] in dot units onto a cols x rows grid: the fraction of
    each dot covered, (rows, cols), row 0 at the top."""
    s = Sheet(float(cols), float(rows))
    s.rect(0, 0, cols, rows, 0x000000)
    for item in items:
        text, x, y, size, face, align, sx = item[:7]
        tracking = item[7] if len(item) > 7 else 1.0
        ob = s.text(text, x, y, size, 0xffffff, face=face, align=align, tracking=tracking)
        ob.scale = (sx, 1.0, 1.0)
    path = s.render(name, cols * ss, samples=16)[0]
    img = bpy.data.images.load(path, check_existing=False)
    w, h = img.size
    a = np.array(img.pixels[:], dtype=np.float32).reshape(h, w, 4)[::-1, :, 0]
    bpy.data.images.remove(img)
    a = a[:rows * ss, :cols * ss].reshape(rows, ss, cols, ss).mean(axis=(1, 3))
    return a


def dot_matrix(lit, pitch, amber=(1.0, 0.69, 0.0)):
    """An LED sign's image from a lit-dot grid: round amber LEDs with hot centers and a faint bloom, the dark dots
    between them just visible."""
    rows, cols = lit.shape
    H, W_ = rows * pitch, cols * pitch
    yy, xx = np.mgrid[0:H, 0:W_].astype(np.float32)
    cy = (np.floor(yy / pitch) + 0.5) * pitch
    cx = (np.floor(xx / pitch) + 0.5) * pitch
    d = np.hypot(yy + 0.5 - cy, xx + 0.5 - cx) / (pitch * 0.5)
    on = np.repeat(np.repeat(lit, pitch, axis=0), pitch, axis=1)
    disc = np.clip((0.86 - d) / 0.18, 0.0, 1.0)
    core_ = np.clip((0.45 - d) / 0.3, 0.0, 1.0)
    img = np.zeros((H, W_, 3), np.float32)
    amber = np.array(amber, np.float32)
    off = np.array((0.055, 0.042, 0.025), np.float32)
    img += (disc * (1.0 - on))[..., None] * off
    img += (disc * on)[..., None] * amber
    img += (core_ * on)[..., None] * (np.array((1.0, 0.92, 0.6), np.float32) - amber) * 0.6
    # Bloom: a soft halo round the lit dots.
    glow = on.astype(np.float32)
    for _ in range(3):
        glow = (glow + np.roll(glow, 1, 0) + np.roll(glow, -1, 0) + np.roll(glow, 1, 1) + np.roll(glow, -1, 1)) / 5.0
    img += (glow * (1.0 - disc) * 0.22)[..., None] * amber
    return np.clip(img, 0.0, 1.0)


def sign_textures():
    """The destination signs (T_ECX_BusSign_Up / _Down, 1024 x 144) and the rear route sign (_Route, 256 x 196)."""
    cols, rows = 256, 36

    def dest(name, text, size, sx, tracking, face):
        cap = size * 0.711
        cov = coverage(f'ecx_sign_{name}', cols, rows, [('14', 3.0, 3.5, 41.0, 'Black', 'LEFT', 0.80, 1.12),
                                                         (text, 150.0, (rows - cap) / 2 - 0.3, size, face, 'CENTER', sx, tracking)])
        lit = (cov > 0.5).astype(np.float32)
        save_png(os.path.join(TEX_DIR, f'T_ECX_BusSign_{name}.png'), dot_matrix(lit, 4))
    dest('Up', 'EMBERCREST', 30.0, 0.80, 1.14, 'Bold')
    dest('Down', 'NORTHSIDE · FIFTH ST', 22.5, 0.60, 1.24, 'Black')
    cov = coverage('ecx_sign_route', 64, 49, [('14', 32.0, 4.5, 58.0, 'Black', 'CENTER', 0.82, 1.15)])
    save_png(os.path.join(TEX_DIR, 'T_ECX_BusSign_Route.png'), dot_matrix((cov > 0.5).astype(np.float32), 4))


def display_image():
    """The next-stop display inside (amber dots): CITY TRANSIT · ROUTE 14."""
    cov = coverage('ecx_bus_display', 160, 16, [('CITY TRANSIT  ·  ROUTE 14', 80.0, 2.6, 14.0, 'Bold', 'CENTER', 0.66, 1.2)])
    img = dot_matrix((cov > 0.5).astype(np.float32), 4)
    path = os.path.join(core.BUILD_DIR, 'textures', 'ecx_bus_display.png')
    save_png(path, img)
    return path


def tire_sheet(name):
    """The sidewall's lettering, one of two repeats round the tire (white on black: a mask)."""
    s = Sheet(1000.0, 68.0)
    s.rect(0, 0, 1000, 68, 0x000000)
    s.text('NORTHLINE', 150, 22, 30, 0xffffff, face='Black', align='CENTER', tracking=1.25)
    s.text('METRO TRANSIT', 430, 26, 17, 0xffffff, face='Bold', align='CENTER', tracking=1.15)
    s.text('305/70R22.5  152/148J', 720, 26, 16, 0xffffff, face='Bold', align='CENTER', tracking=1.05)
    s.rect(285, 31, 24, 3, 0xffffff)
    s.rect(565, 31, 24, 3, 0xffffff)
    s.text('REGROOVABLE', 915, 28, 11, 0xffffff, face='Bold', align='CENTER')
    s.text('MAX LOAD 3550 KG  ·  TUBELESS  ·  RADIAL', 500, 6, 9, 0xffffff, face='Medium', align='CENTER', tracking=1.1)
    return s.render(name, 1024 if FAST else 4096)[0]


# The print atlas: (key, width m, height m, px per m, opaque). Exterior decals are ink only (the livery shows
# through); cards and signs carry their own grounds.
PRINTS = [
    ('wm_r', 2.20, 0.24, 700, False), ('wm_l', 2.20, 0.24, 700, False), ('wm_rear', 0.62, 0.11, 900, False),
    ('fl_rf', 0.42, 0.115, 900, False), ('fl_lf', 0.42, 0.115, 900, False), ('fl_rr', 0.46, 0.14, 800, False),
    ('fl_lr', 0.46, 0.14, 800, False), ('fl_front', 0.27, 0.09, 1000, False), ('fl_rear', 0.42, 0.13, 900, False),
    ('fl_roof', 1.50, 0.60, 300, False), ('ada_r1', 0.15, 0.15, 1300, False), ('ada_r2', 0.15, 0.15, 1300, False),
    ('yield', 0.26, 0.23, 1100, False),
    ('route', 0.60, 0.19, 1500, True), ('kneel', 0.28, 0.075, 1600, True), ('plate', 0.305, 0.152, 1800, True),
    ('ad_ec', 0.71, 0.28, 1350, True), ('ad_lp', 0.71, 0.28, 1350, True), ('ad_owl', 0.71, 0.28, 1350, True),
    ('ad_ct', 0.71, 0.28, 1350, True), ('map', 1.42, 0.28, 1350, True), ('priority', 0.71, 0.28, 1100, True),
    ('wc_sign', 0.30, 0.30, 1200, True), ('exit', 0.34, 0.07, 1500, True), ('hdr_l', 0.52, 0.11, 1500, True),
    ('hdr_r', 0.52, 0.11, 1500, True), ('fare', 0.24, 0.12, 1800, True), ('farebox', 0.26, 0.40, 1300, True),
    ('floor', 0.90, 0.10, 1000, True), ('rear_hdr', 1.00, 0.08, 1300, True), ('wc_floor', 0.62, 0.62, 500, True),
    ('validator', 0.10, 0.16, 1800, True),
]


def pack_prints(size=4096):
    """Shelf-packs the prints into the atlas at one scale on their densities, as large as fits: {key: (x, y, w, h)}."""
    def layout(scale):
        items = sorted(PRINTS, key=lambda p: -p[2] * p[3])
        x = y = shelf = 0.0
        out = {}
        gap = 8.0
        for key, w, h, d, _ in items:
            pw, ph = math.floor(w * d * scale), math.floor(h * d * scale)
            if pw > size:
                return None
            if x + pw > size:
                y += shelf + gap
                x = shelf = 0.0
            if y + ph > size:
                return None
            out[key] = (x, y, pw, ph)
            x += pw + gap
            shelf = max(shelf, ph)
        return out
    lo, hi = 0.5, 4.0
    for _ in range(30):
        mid = (lo + hi) / 2
        if layout(mid) is not None:
            lo = mid
        else:
            hi = mid
    return layout(lo)


def ada_symbol(s, x, y, w, h, fg=0xffffff, bg=0x1f4fa8):
    s.rect(x, y, w, h, bg)
    u = min(w, h) / 100.0
    cx, cy = x + w / 2, y + h / 2
    s.circle(cx - 6 * u, cy + 30 * u, 7 * u, fg)
    s.poly([(cx - 10 * u, cy + 20 * u), (cx - 2 * u, cy + 20 * u), (cx - 2 * u, cy - 2 * u), (cx + 14 * u, cy - 2 * u), (cx + 22 * u, cy - 30 * u),
            (cx + 30 * u, cy - 27 * u), (cx + 20 * u, cy + 6 * u), (cx - 10 * u, cy + 6 * u)], fg)
    s.ring(cx - 8 * u, cy - 14 * u, 17 * u, 23 * u, fg, n=64, a0=math.radians(-160), a1=math.radians(150))


def ct_logo(s, cx, cy, r, ink=0xffffff, ground=None):
    """City Transit's mark: a ring broken by a forward swoosh."""
    if ground is not None:
        s.circle(cx, cy, r * 1.08, ground, n=64)
    s.ring(cx, cy, r * 0.74, r, ink, n=96, a0=math.radians(35), a1=math.radians(325))
    s.poly([(cx - r * 0.55, cy - r * 0.28), (cx + r * 1.15, cy + r * 0.05), (cx + r * 1.15, cy + r * 0.32), (cx - r * 0.55, cy + 0.02 * r)], ink)


def penny_logo(s, cx, cy, r, ink=0x6b3412):
    """The Lucky Penny's coin with a clover (as on its sign)."""
    s.circle(cx, cy, r, 0xd08a4e, n=64)
    s.ring(cx, cy, r * 0.78, r * 0.86, ink, n=64)
    for k in range(4):
        a = k * math.pi / 2 + math.pi / 4
        s.circle(cx + math.cos(a) * r * 0.22, cy + r * 0.06 + math.sin(a) * r * 0.22, r * 0.2, 0x5a2a0c, n=24)
    s.poly([(cx - r * 0.03, cy), (cx + r * 0.03, cy), (cx + r * 0.1, cy - r * 0.42), (cx + r * 0.04, cy - r * 0.42)], 0x5a2a0c)


def fit(s, text, x, y, size, max_w, color, face='Black', align='CENTER', tracking=1.0):
    o = s.text(text, x, y, size, color, face=face, align=align, tracking=tracking)
    bpy.context.view_layer.update()
    if o.dimensions.x > max_w > 0:
        f = max_w / o.dimensions.x
        o.scale = (f, f, 1.0)
    return o


def draw_print(s, key, x, y, w, h):
    """Lays out one print in its atlas rectangle (layout units = atlas pixels)."""
    def X(f):
        return x + w * f

    def Y(f):
        return y + h * f
    if key in ('wm_r', 'wm_l'):
        ct_logo(s, X(0.055), Y(0.5), h * 0.36)
        fit(s, 'CITY TRANSIT', X(0.13), Y(0.17), h * 0.92, w * 0.86, 0xffffff, 'Black', 'LEFT', 1.08)
    elif key == 'wm_rear':
        ct_logo(s, X(0.08), Y(0.5), h * 0.36, ink=GREEN)
        fit(s, 'CITY TRANSIT', X(0.18), Y(0.18), h * 0.88, w * 0.80, GREEN, 'Black', 'LEFT', 1.05)
    elif key.startswith('fl_'):
        ink = {'fl_rf': 0x111214, 'fl_lf': 0x111214, 'fl_rear': 0x111214, 'fl_roof': 0x141516}.get(key, 0xffffff)
        fit(s, '2314', X(0.5), Y(0.08), h * 1.18, w * 0.96, ink, 'Black', 'CENTER', 1.04)
    elif key.startswith('ada_'):
        ada_symbol(s, x, y, w, h)
    elif key == 'yield':
        s.poly([(X(0.0), Y(1.0)), (X(1.0), Y(1.0)), (X(0.5), Y(0.0))], 0xc8202a)
        s.poly([(X(0.13), Y(0.92)), (X(0.87), Y(0.92)), (X(0.5), Y(0.17))], 0xffffff)
        fit(s, 'YIELD', X(0.5), Y(0.68), h * 0.16, w * 0.48, 0xc8202a, 'Black', 'CENTER')
        fit(s, 'TO BUS', X(0.5), Y(0.47), h * 0.12, w * 0.32, 0xc8202a, 'Black', 'CENTER')
    elif key == 'route':
        s.rect(x, y, w, h, 0xf3f1ea)
        s.rect(x, Y(0.86), w, h * 0.14, GREEN)
        fit(s, 'CITY TRANSIT  ·  ROUTE', X(0.5), Y(0.895), h * 0.085, w * 0.9, 0xffffff, 'Bold', 'CENTER', 1.3)
        s.circle(X(0.14), Y(0.42), h * 0.32, GREEN, n=96)
        fit(s, '14', X(0.14), Y(0.29), h * 0.36, h * 0.5, 0xffffff, 'Black', 'CENTER')
        fit(s, 'EMBERCREST', X(0.28), Y(0.40), h * 0.30, w * 0.69, 0x111214, 'Black', 'LEFT', 1.04)
        fit(s, 'RIDGE ROAD  ·  NORTHSIDE  ·  FIFTH ST', X(0.285), Y(0.17), h * 0.11, w * 0.68, 0x3d4247, 'Bold', 'LEFT', 1.1)
    elif key == 'kneel':
        s.rect(x, y, w, h, 0xf2c200)
        s.rect(x + h * 0.06, y + h * 0.06, w - h * 0.12, h - h * 0.12, 0x111111)
        s.rect(x + h * 0.11, y + h * 0.11, w - h * 0.22, h - h * 0.22, 0xf2c200)
        fit(s, 'CAUTION  ·  BUS KNEELS  ·  STAND CLEAR', X(0.5), Y(0.36), h * 0.32, w * 0.88, 0x111111, 'Black', 'CENTER')
    elif key == 'plate':
        s.rect(x, y, w, h, 0xf2f1ec)
        s.rect(x + h * 0.03, y + h * 0.03, w - h * 0.06, h - h * 0.06, GREEN)
        s.rect(x + h * 0.06, y + h * 0.06, w - h * 0.12, h - h * 0.12, 0xf2f1ec)
        fit(s, 'MUNICIPAL', X(0.5), Y(0.76), h * 0.13, w * 0.5, GREEN, 'Black', 'CENTER', 1.3)
        fit(s, '14 2314', X(0.5), Y(0.30), h * 0.40, w * 0.86, 0x16233f, 'Black', 'CENTER')
        fit(s, 'CITY OWNED  ·  EXEMPT', X(0.5), Y(0.11), h * 0.10, w * 0.6, GREEN, 'Bold', 'CENTER', 1.2)
        for bx in (0.08, 0.92):
            s.circle(X(bx), Y(0.86), h * 0.035, 0xb4b7ba)
    elif key == 'ad_ec':
        s.rect(x, y, w, h, brand.NAVY)
        for k in range(24):
            t = k / 23.0
            s.rect(x, Y(0.5 + 0.5 * t), w, h * 0.022 + 0.5, brand.mix(brand.NAVY, brand.MIDNIGHT, t))
        for k in range(16):
            t = k / 15.0
            s.rect(x, Y(0.0 + 0.10 * t), w, h * 0.008 + 0.5, brand.mix(brand.EMBER, brand.NAVY, t))
        brand.mark(s, X(0.145), Y(0.52), h * 0.66)
        brand.wordmark(s, X(0.27), Y(0.62), h * 0.17, align='LEFT', sub='POKER SERIES')
        fit(s, 'NIGHTLY AT 7', X(0.27), Y(0.22), h * 0.20, w * 0.44, brand.GOLD, 'Black', 'LEFT', 1.05)
        fit(s, 'TAKE THE 14 TO THE RIDGE', X(0.965), Y(0.33), h * 0.072, w * 0.27, brand.ASH, 'Bold', 'RIGHT', 1.25)
        fit(s, 'RESORT  ·  CASINO', X(0.965), Y(0.20), h * 0.072, w * 0.27, brand.ICE, 'Bold', 'RIGHT', 1.6)
        s.rect(X(0.72), Y(0.29), w * 0.245, h * 0.006 + 0.6, brand.EMBER_HOT)
    elif key == 'ad_lp':
        s.rect(x, y, w, h, 0xf5f1e8)
        s.rect(x, Y(0.80), w, h * 0.20, 0xd8262f)
        penny_logo(s, X(0.085), Y(0.80), h * 0.17)
        fit(s, 'LUCKY PENNY', X(0.17), Y(0.845), h * 0.12, w * 0.45, 0xf5f1e8, 'Black', 'LEFT', 1.08)
        fit(s, 'FIFTH & MARKET  ·  OPEN ALL NIGHT', X(0.97), Y(0.855), h * 0.06, w * 0.36, 0xf5f1e8, 'Bold', 'RIGHT', 1.2)
        fit(s, 'ROLLER DOGS', X(0.05), Y(0.40), h * 0.26, w * 0.56, 0x8a2a1a, 'Black', 'LEFT', 1.02)
        fit(s, 'hot off the grill, all night long', X(0.055), Y(0.18), h * 0.085, w * 0.5, 0x6b3412, 'Medium', 'LEFT')
        cx, cy, rr = X(0.80), Y(0.40), h * 0.30
        pts = []
        for k in range(28):
            a = 2 * math.pi * k / 28
            rad = rr * (1.0 if k % 2 == 0 else 0.82)
            pts.append((cx + rad * math.cos(a), cy + rad * math.sin(a)))
        s.poly(pts, 0xf2c14e)
        fit(s, '2 FOR', cx, cy + rr * 0.22, rr * 0.36, rr * 1.2, 0x8a2a1a, 'Black', 'CENTER')
        fit(s, '$3', cx, cy - rr * 0.48, rr * 0.72, rr * 1.2, 0xd8262f, 'Black', 'CENTER')
    elif key == 'ad_owl':
        s.rect(x, y, w, h, GREEN)
        s.rect(x, Y(0.93), w, h * 0.07, GOLD)
        ct_logo(s, X(0.085), Y(0.50), h * 0.17)
        fit(s, 'OWL SERVICE ON THE 14', X(0.205), Y(0.58), h * 0.16, w * 0.76, 0xffffff, 'Black', 'LEFT', 1.04)
        fit(s, '12:15  ·  1:15  ·  2:15  ·  3:15 AM', X(0.205), Y(0.35), h * 0.11, w * 0.6, GOLD, 'Bold', 'LEFT', 1.1)
        fit(s, 'Every night, Embercrest to Northside · Fifth St & Market', X(0.205), Y(0.16), h * 0.068, w * 0.76, 0xdfeee5, 'Medium', 'LEFT')
    elif key == 'ad_ct':
        s.rect(x, y, w, h, 0xf3f1ea)
        s.rect(x, y, w * 0.16, h, GREEN)
        ct_logo(s, X(0.07), Y(0.5), h * 0.15)
        fit(s, 'RIDE KIND.', X(0.20), Y(0.60), h * 0.20, w * 0.7, GREEN, 'Black', 'LEFT')
        fit(s, 'Let riders off first  ·  Hold on  ·  Offer your seat', X(0.20), Y(0.36), h * 0.085, w * 0.76, 0x2b2f33, 'Bold', 'LEFT')
        fit(s, 'CITY TRANSIT  ·  Fare $2.90  ·  Exact fare or pass', X(0.20), Y(0.15), h * 0.065, w * 0.76, 0x5a6066, 'Medium', 'LEFT')
    elif key == 'map':
        s.rect(x, y, w, h, 0xf6f5f0)
        s.rect(x, Y(0.80), w, h * 0.20, GREEN)
        s.circle(X(0.028), Y(0.90), h * 0.075, 0xffffff, n=48)
        fit(s, '14', X(0.028), Y(0.865), h * 0.08, h * 0.12, GREEN, 'Black', 'CENTER')
        fit(s, 'EMBERCREST  ·  RIDGE ROAD', X(0.06), Y(0.865), h * 0.09, w * 0.5, 0xffffff, 'Black', 'LEFT', 1.15)
        fit(s, 'CITY TRANSIT', X(0.985), Y(0.865), h * 0.08, w * 0.2, 0xffffff, 'Bold', 'RIGHT', 1.3)
        stops = ['NORTHSIDE · FIFTH & MARKET', 'MARKET & 9TH', 'CIVIC CENTER', 'UNION STATION', 'RIVERFRONT', 'OLD MILL RD',
                 'PINE FLATS', 'SWITCHBACK LOOKOUT', 'EMBERCREST RESORT']
        y0 = Y(0.47)
        s.rect(X(0.09), y0 - h * 0.025, w * 0.82, h * 0.05, GREEN)
        for i, name in enumerate(stops):
            sx_ = X(0.09 + 0.82 * i / (len(stops) - 1))
            big = i in (0, len(stops) - 1)
            transfer = i in (0, 3)
            s.circle(sx_, y0, h * (0.06 if big else 0.042), 0xffffff, n=40)
            s.ring(sx_, y0, h * (0.045 if big else 0.03), h * (0.06 if big else 0.042), 0x111214 if transfer else GREEN, n=40)
            fit(s, name, sx_, Y(0.18) if i % 2 else Y(0.62), h * 0.07, w * 0.17, 0x111214, 'Black' if big else 'Bold', 'CENTER')
        fit(s, 'Fare $2.90  ·  Exact fare or pass  ·  20 min to Fifth & Market  ·  Owl service 12:15 – 3:15 AM', X(0.5), Y(0.035), h * 0.06,
            w * 0.9, 0x3d4247, 'Medium', 'CENTER')
    elif key == 'priority':
        s.rect(x, y, w, h, 0x1f4fa8)
        ada_symbol(s, X(0.025), Y(0.15), h * 0.70, h * 0.70, bg=0x1f4fa8)
        tx = x + h * 0.80
        fit(s, 'PRIORITY SEATING', tx, Y(0.58), h * 0.19, x + w * 0.97 - tx, 0xffffff, 'Black', 'LEFT', 1.05)
        fit(s, 'Please offer these seats to seniors and riders with disabilities.', tx, Y(0.32), h * 0.075, x + w * 0.97 - tx, 0xe5ecf6, 'Bold', 'LEFT')
        fit(s, 'Federal law requires it.  ·  CITY TRANSIT', tx, Y(0.14), h * 0.06, x + w * 0.9 - tx, 0xc8d4e8, 'Medium', 'LEFT')
    elif key == 'wc_sign':
        s.rect(x, y, w, h, 0x1f4fa8)
        ada_symbol(s, X(0.2), Y(0.3), w * 0.6, h * 0.6, bg=0x1f4fa8)
        fit(s, 'WHEELCHAIR AREA', X(0.5), Y(0.15), h * 0.085, w * 0.88, 0xffffff, 'Black', 'CENTER')
        fit(s, 'Please fold seat up for a mobility device', X(0.5), Y(0.05), h * 0.045, w * 0.9, 0xe5ecf6, 'Bold', 'CENTER')
    elif key == 'exit':
        s.rect(x, y, w, h, 0xf2f1ec)
        fit(s, 'EMERGENCY EXIT', X(0.5), Y(0.48), h * 0.36, w * 0.9, 0xc8202a, 'Black', 'CENTER', 1.1)
        fit(s, 'PULL RED HANDLE  ·  PUSH WINDOW OUT', X(0.5), Y(0.12), h * 0.2, w * 0.9, 0x222222, 'Bold', 'CENTER')
    elif key == 'hdr_l':
        s.rect(x, y, w, h, 0x111214)
        fit(s, 'PLEASE DO NOT TALK TO THE OPERATOR', X(0.5), Y(0.56), h * 0.24, w * 0.92, 0xffffff, 'Bold', 'CENTER')
        fit(s, 'WHILE THE BUS IS IN MOTION', X(0.5), Y(0.20), h * 0.24, w * 0.92, 0xffffff, 'Bold', 'CENTER')
    elif key == 'hdr_r':
        s.rect(x, y, w, h, 0x111214)
        fit(s, 'STAND BEHIND THE YELLOW LINE', X(0.5), Y(0.56), h * 0.24, w * 0.92, 0xf2c200, 'Bold', 'CENTER')
        fit(s, 'NO FOOD  ·  NO DRINKS  ·  NO SMOKING', X(0.5), Y(0.20), h * 0.22, w * 0.92, 0xffffff, 'Bold', 'CENTER')
    elif key == 'fare':
        s.rect(x, y, w, h, 0x111214)
        fit(s, 'FARE', X(0.5), Y(0.66), h * 0.2, w * 0.8, 0xbfc4c9, 'Bold', 'CENTER', 1.4)
        fit(s, '$2.90', X(0.5), Y(0.22), h * 0.42, w * 0.86, 0xffffff, 'Black', 'CENTER')
    elif key == 'farebox':
        s.rect(x, y, w, h, 0x1a1b1e)
        s.rect(X(0.12), Y(0.70), w * 0.76, h * 0.2, 0x0a0b0c)
        fit(s, 'EXACT FARE', X(0.5), Y(0.62), h * 0.05, w * 0.8, 0xffffff, 'Black', 'CENTER', 1.2)
        s.rect(X(0.2), Y(0.50), w * 0.6, h * 0.035, 0x050505)
        fit(s, 'BILLS FACE UP', X(0.5), Y(0.44), h * 0.035, w * 0.7, 0xbfc4c9, 'Bold', 'CENTER')
        s.circle(X(0.5), Y(0.25), w * 0.18, 0x2a6a46, n=64)
        s.ring(X(0.5), Y(0.25), w * 0.10, w * 0.12, 0xffffff, n=64)
        s.ring(X(0.5), Y(0.25), w * 0.05, w * 0.065, 0xffffff, n=64)
        fit(s, 'TAP CARD HERE', X(0.5), Y(0.06), h * 0.04, w * 0.8, 0xffffff, 'Bold', 'CENTER')
    elif key == 'floor':
        s.rect(x, y, w, h, 0xe8b81c)
        for k in range(14):
            s.poly([(X(k / 14.0), Y(0.0)), (X(k / 14.0 + 0.03), Y(0.0)), (X(k / 14.0 + 0.05), Y(0.18)), (X(k / 14.0 + 0.02), Y(0.18))], 0x111111)
        fit(s, 'PLEASE STAND BEHIND THIS LINE', X(0.5), Y(0.36), h * 0.42, w * 0.94, 0x111111, 'Black', 'CENTER')
    elif key == 'rear_hdr':
        s.rect(x, y, w, h, 0x111214)
        fit(s, 'REAR DOOR  ·  TOUCH YELLOW TAPE TO OPEN  ·  STAND CLEAR', X(0.5), Y(0.3), h * 0.42, w * 0.94, 0xf2c200, 'Bold', 'CENTER')
    elif key == 'wc_floor':
        s.rect(x, y, w, h, 0x56595c)
        ada_symbol(s, X(0.12), Y(0.12), w * 0.76, h * 0.76, fg=0xe8ecf0, bg=0x1f4fa8)
    elif key == 'validator':
        s.rect(x, y, w, h, 0x1a1b1e)
        s.rect(X(0.12), Y(0.62), w * 0.76, h * 0.28, 0x0b2a33)
        fit(s, 'TAP', X(0.5), Y(0.72), h * 0.1, w * 0.6, 0x7fe8ff, 'Black', 'CENTER')
        s.ring(X(0.5), Y(0.33), w * 0.18, w * 0.24, 0xffffff, n=48)
        s.ring(X(0.5), Y(0.33), w * 0.07, w * 0.11, 0xffffff, n=48)


def print_atlas():
    """Renders the print atlas (4096 px; 1 layout unit = 1 px): (color path, surface path, {key: uv rect})."""
    size = 4096
    rects = pack_prints(size)
    s = Sheet(float(size), float(size))
    for key, (x, y, w, h) in rects.items():
        draw_print(s, key, x, y, w, h)
    color, surface = s.render('ecx_bus_prints_atlas', 1024 if FAST else size, samples=16 if FAST else 24)
    uv = {k: (x / size, y / size, (x + w) / size, (y + h) / size) for k, (x, y, w, h) in rects.items()}
    return color, surface, uv


# ------------------------------------------------------------------ the kit

class Bus:
    def __init__(self):
        self.srf = Surface()
        self.body = []
        self.glass = []
        self.glow = []
        self.M = {}

    # -------------------------------------------------------------- materials

    def materials(self):
        M = self.M
        sheets = seam_sheets()
        M['paint'] = paint_material(sheets)
        M['trim'] = trim_material()
        M['metal'] = metal_material()
        M['interior'] = interior_material()
        M['lamps'] = lamps_material(display_image())
        self.atlas = print_atlas()
        M['prints'] = prints_material(self.atlas, sheets)
        M['glass'], M['tint'], M['dest'], M['route'] = unbaked_materials()
        M['order'] = [M[k] for k in ('paint', 'trim', 'metal', 'interior', 'lamps', 'prints')]

    def uvr(self, key):
        return self.atlas[2][key]

    def add(self, o, key=None, ip=None):
        if key is not None:
            put(o, self.M[key], ip)
        self.body.append(o)
        return o

    def decal(self, name, key, center, u_dir, v_dir, w, h, nu=1, nv=1, skin=False):
        """A print; skin: it lies on the paint (its shading normals are taken from the paint under it)."""
        o = self.add(quad(name, center, u_dir, v_dir, w, h, self.uvr(key), nu, nv), 'prints')
        if skin:
            skin_decal(o)
        return o

    # -------------------------------------------------------------- the shell

    def shell(self):
        me = bpy.data.meshes.new('shell')
        for m in self.M['order']:
            me.materials.append(m.m)
        bm = bmesh.new()
        shell_into(bm, all_rings(False), 0)
        inner = shell_into(bm, all_rings(True), 3)
        bmesh.ops.reverse_faces(bm, faces=inner)
        bm.to_mesh(me)
        bm.free()
        obj = core.link(bpy.data.objects.new('shell', me))
        cutters = []
        for side, wins in ((1, R_WIN), (-1, L_WIN)):
            for x0, x1, z0, z1 in wins:
                cutters.append(prism_y('win_cut', rr_xz(x0, x1, z0, z1, 0.06), side * 1.10, side * 1.50))
        for x0, x1 in (DOOR_F, DOOR_R):
            cutters.append(prism_y('door_cut', rr_xz(x0, x1, DOOR_Z[0], DOOR_Z[1], 0.05, radii=(0.0, 0.0, 0.05, 0.05)), 1.10, 1.50))
        cutters.append(parts.rbox('ws_cut', ((-6.40 + X_PILLAR) / 2, 0.0, (1.12 + 2.72) / 2), (abs(X_PILLAR + 6.40), 3.2, 1.60), 0.0))
        cutters.append(core.prism_x('rw_cut', core.rounded_rect(-REAR_WIN[0], REAR_WIN[1], REAR_WIN[0], REAR_WIN[2], 0.08, 5), 5.80, 6.40))
        cutters.append(parts.rbox('sign_cut', ((-6.40 - 6.06) / 2, 0.0, (2.81 + 3.09) / 2), (0.34, 1.95, 0.28), 0.0))
        x0, x1, z0, z1 = RADIATOR
        cutters.append(prism_y('rad_cut', rr_xz(x0, x1, z0, z1, 0.05), -1.50, -1.265))
        cutters.append(core.prism_x('vent_cut', core.rounded_rect(-0.70, 1.30, 0.70, 1.50, 0.03, 4), 6.025, 6.40))
        for ax in AXLES:
            for sy in (-1, 1):
                cutters.append(parts.rod('arch_cut', (ax, sy * LINER_Y, ARCH_Z), (ax, sy * 1.50, ARCH_Z), ARCH_R, 72))
        for c in cutters:
            core.assign(c, self.M['trim'])
        cut = core.join(cutters, 'cutters')
        core.boolean(obj, cut)
        for p in obj.data.polygons:
            if p.material_index == 0 and p.normal.z < -0.7 and p.center.z < 0.45:
                p.material_index = 1
        self.body.append(obj)
        return obj

    # -------------------------------------------------------------- outside

    def bumpers(self):
        srf = self.srf
        for front in (True, False):
            z = 0.50
            pts = srf.front(z) if front else srf.rear(z)
            # From the corners round the nose (front: from the A pillar's foot; rear: round onto the sides).
            lim = -5.905 if front else 5.76
            keep = [p for p in pts if (p[0] <= lim if front else p[0] >= lim)]
            S = cum(keep)
            n = 90
            path = [at_s(keep, S, S[-1] * i / n) for i in range(n + 1)]
            # Outward normals in plan.
            d = 0.040 if front else 0.036
            top = 0.660 if front else 0.640
            sec = [(0.330, -0.02), (0.330, d - 0.016), (0.342, d - 0.004), (0.40, d), (top - 0.06, d), (top - 0.018, d - 0.004), (top, d - 0.016), (top, -0.02)]
            strip = [(0.47, d - 0.004), (0.47, d + 0.008), (0.476, d + 0.012), (0.524, d + 0.012), (0.53, d + 0.008), (0.53, d - 0.004)]
            for prof, name in ((sec, 'bumper'), (strip, 'bumper_strip')):
                rings = []
                for i, (px, py) in enumerate(path):
                    a = path[max(i - 1, 0)]
                    b = path[min(i + 1, n)]
                    tx, ty = b[0] - a[0], b[1] - a[1]
                    L = math.hypot(tx, ty)
                    nx, ny = (ty / L, -tx / L)
                    # Outward: away from the bus's middle.
                    if nx * px + ny * py < 0:
                        nx, ny = -nx, -ny
                    rings.append([Vector((px + nx * out, py + ny * out, zz)) for zz, out in prof])
                self.add(core.loft(name, rings), 'trim')

    def rub_rails(self):
        segs = {1: [(-4.64, AXLES[0] - 0.64), (AXLES[0] + 0.64, -0.13), (1.13, AXLES[1] - 0.64), (AXLES[1] + 0.64, 5.80)],
                -1: [(-5.80, AXLES[0] - 0.64), (AXLES[0] + 0.64, AXLES[1] - 0.64), (AXLES[1] + 0.64, 5.80)]}
        rail = [(-0.003, 0.700), (0.012, 0.702), (0.018, 0.710), (0.020, 0.730), (0.018, 0.750), (0.012, 0.758), (-0.003, 0.760)]
        ins = [(0.015, 0.723), (0.026, 0.724), (0.027, 0.730), (0.026, 0.736), (0.015, 0.737)]
        for side, ss in segs.items():
            for x0, x1 in ss:
                for prof, key in ((rail, 'metal'), (ins, 'trim')):
                    o = core.prism_x('rub', [(side * (W + out), zz) for out, zz in prof], x0, x1)
                    self.add(o, key)

    def arch_trims(self):
        for ax in AXLES:
            for side in (1, -1):
                sec = [(-0.006, -0.004), (0.010, -0.004), (0.016, 0.012), (0.012, 0.040), (-0.006, 0.040)]
                phi0 = math.degrees(math.asin((Z_BOT + 0.02 - ARCH_Z) / ARCH_R))
                self.add(arc_sweep('arch_trim', ax, ARCH_Z, ARCH_R, phi0, 180.0 - phi0, 96, W, side, sec), 'trim')

    def liners(self):
        """The wheel houses: a black liner round each arch from the skin in to its inner wall, the inner wall."""
        R = ARCH_R + 0.004
        for ax in AXLES:
            for side in (1, -1):
                phi0 = math.degrees(math.asin((Z_BOT - 0.03 - ARCH_Z) / R))
                n = 48
                bm = bmesh.new()
                rings = []
                for yy in (LINER_Y, W - 0.008):
                    rings.append([bm.verts.new((ax + R * math.cos(math.radians(phi0 + (180 - 2 * phi0) * i / n)), side * yy,
                                                ARCH_Z + R * math.sin(math.radians(phi0 + (180 - 2 * phi0) * i / n)))) for i in range(n + 1)])
                tube = [bm.faces.new((rings[0][i], rings[0][i + 1], rings[1][i + 1], rings[1][i])) for i in range(n)]
                cap = bm.faces.new(rings[0])
                bm.normal_update()
                # Seen from the wheel's side: the tube's faces toward its axis, the end wall outward through the arch.
                for f in tube + [cap]:
                    c = f.calc_center_median()
                    want = Vector((0.0, side, 0.0)) if f is cap else Vector((ax - c.x, 0.0, ARCH_Z - c.z))
                    if f.normal.dot(want) < 0:
                        f.normal_flip()
                self.add(core.mesh_object('liner', bm), 'trim')
                # Mud flap behind the rear wheels, a splash guard behind the front ones.
                if ax > 0:
                    self.add(box('mudflap', ax + 0.66, ax + 0.675, side * 0.80, side * (W - 0.03), 0.10, 0.36, None, 0.004), 'trim')

    def front_end(self):
        srf = self.srf
        # The head lamp clusters: a black bezel fitted to the cap, the lens over it; amber park lamps round the corners.
        for sy in (1, -1):
            def cols(z, y0, y1, n):
                pts = srf.front(z)
                S = cum(pts)
                sc = s_where(pts, S, lambda p: p[1], 0.0)
                if sy > 0:
                    a = s_where(pts, S, lambda p: p[1], y0, sc)
                    b = s_where(pts, S, lambda p: p[1], y1, sc)
                else:
                    a = s_where(pts, S, lambda p: p[1], -y1)
                    b = s_where(pts, S, lambda p: p[1], -y0)
                return [(*at_s(pts, S, a + (b - a) * j / n), z) for j in range(n + 1)]
            rows = [cols(0.69 + 0.25 * i / 6, 0.615, 1.13, 24) for i in range(7)]
            self.add(grid_object('lamp_bezel', rows, (-1, 0, 0), 0.004), 'trim')
            rows = [cols(0.697 + 0.236 * i / 6, 0.625, 1.122, 24) for i in range(7)]
            self.add(grid_object('head_lens', rows, (-1, 0, 0), 0.009), 'lamps')
            # Park lamp: from the cluster's end round the corner onto the side.

            def corner_cols(z, n):
                pts = srf.front(z)
                S = cum(pts)
                sc = s_where(pts, S, lambda p: p[1], 0.0)
                if sy > 0:
                    a = s_where(pts, S, lambda p: p[1], 1.135, sc)
                    b = s_where(pts, S, lambda p: p[0], -5.965, sc)
                else:
                    a = s_where(pts, S, lambda p: p[0], -5.965)
                    b = s_where(pts, S, lambda p: p[1], -1.135)
                return [(*at_s(pts, S, a + (b - a) * j / n), z) for j in range(n + 1)]
            rows = [corner_cols(0.70 + 0.225 * i / 5, 12) for i in range(6)]
            self.add(grid_object('park_lens', rows, (-1, sy, 0), 0.006), 'lamps')
            rows = [corner_cols(0.693 + 0.239 * i / 5, 12) for i in range(6)]
            self.add(grid_object('park_bezel', rows, (-1, sy, 0), 0.0035), 'trim')
        # The sign box: a black bezel round the pocket; the windshield's center divider; the wipers.
        self.add(frame_ring('sign_bezel', (-0.995, 0.995, 2.795, 3.105), (-0.962, 0.962, 2.818, 3.082), -6.096, -6.088, 'X'), 'trim')
        self.add(box('sign_back', -6.065, -6.062, -0.97, 0.97, 2.812, 3.088, None), 'trim')
        for z0 in (1.10,):
            pts = []
            for i in range(17):
                z = 1.10 + (2.74 - 1.10) * i / 16
                p = srf.front(z)
                S = cum(p)
                c = at_s(p, S, s_where(p, S, lambda q: q[1], 0.0))
                pts.append((c[0], z))
            rings = [[Vector((x - 0.006, -0.008, z)), Vector((x - 0.006, 0.008, z)), Vector((x + 0.004, 0.008, z)), Vector((x + 0.004, -0.008, z))]
                     for x, z in pts]
            self.add(core.loft('ws_divider', rings), 'trim')
        # Wipers: two pantograph arms parked along the glass's foot.
        for (y0, y1, piv) in ((-1.05, -0.12, -0.62), (0.02, 0.95, 0.42)):
            z = 1.17
            p = srf.front(z)
            S = cum(p)
            blade = []
            for j in range(13):
                yy = y0 + (y1 - y0) * j / 12
                c = at_s(p, S, s_where(p, S, lambda q: q[1], yy, 0.0 if yy < 0 else s_where(p, S, lambda q: q[1], 0.0)))
                blade.append((c[0] - 0.018, yy, z + 0.01 * math.sin(math.pi * j / 12)))
            rings = [[Vector((bx - 0.004, by, bz - 0.012)), Vector((bx - 0.016, by, bz - 0.006)), Vector((bx - 0.016, by, bz + 0.006)),
                      Vector((bx - 0.004, by, bz + 0.012))] for bx, by, bz in blade]
            self.add(core.loft('wiper_blade', rings), 'trim')
            mid = blade[6]
            pv = (blade[6][0] - 0.01, piv, 1.06)
            self.add(rod('wiper_arm', pv, (mid[0] - 0.02, mid[1], mid[2]), 0.008, self.M['trim'], 8))
            self.add(rod('wiper_arm2', (pv[0], pv[1] + 0.06, pv[2]), (mid[0] - 0.02, mid[1] + 0.06, mid[2]), 0.006, self.M['trim'], 8))
            self.add(parts.disc('wiper_pivot', pv, (1, 0, 0), 0.025, 0.03, 16), 'trim')
        # Clearance and ID lamps on the roof's front edge.
        for yy in (-0.22, 0.0, 0.22):
            self.add(parts.rbox('id_lamp', (-6.062, yy, 3.118), (0.03, 0.07, 0.03), 0.01), 'lamps')
        for sy in (1, -1):
            self.add(parts.rbox('clear_lamp', (-5.99, sy * 1.10, 3.118), (0.08, 0.06, 0.03), 0.01), 'lamps')
        # The front plate on the bumper's right, the fleet number on the cap.
        self.decal('plate_f', 'plate', (-6.1085, 0.83, 0.495), (0, -1, 0), (0, 0, 1), 0.305, 0.152)
        self.add(box('plate_bracket_f', -6.106, -6.0985, 0.665, 0.995, 0.412, 0.578, None), 'trim')
        self.front_fleet()
        self.bike_rack()
        self.mirrors()

    def front_fleet(self):
        srf = self.srf
        rows = []
        for i in range(3):
            z = 0.935 + 0.09 * i / 2
            p = srf.front(z)
            S = cum(p)
            a = s_where(p, S, lambda q: q[1], -1.075)
            b = s_where(p, S, lambda q: q[1], -0.805)
            rows.append([(*at_s(p, S, a + (b - a) * j / 6), z) for j in range(7)])
        # The front reads toward -Y: the row runs from -1.075 to -0.805, i.e. along +Y; flip it so u runs toward -Y.
        rows = [list(reversed(r)) for r in rows]
        self.add(skin_decal(grid_object('fleet_front', rows, (-1, 0, 0), DECAL_E, self.uvr('fl_front'))), 'prints')

    def bike_rack(self):
        """A two-bike rack folded up against the front: its pivot bracket on the bumper, the frame, two wheel trays
        across the bus with their chocks, the support arms with their hooks folded down."""
        xm = 'metal'
        bx = -6.115
        self.add(box('rack_bracket', -6.135, -6.095, -0.30, 0.30, 0.56, 0.66, None, 0.01), 'trim')
        for sy in (-1, 1):
            self.add(box('rack_hinge', -6.15, -6.11, sy * 0.36 - 0.04, sy * 0.36 + 0.04, 0.60, 0.70, None, 0.008), 'trim')
            self.add(rod('rack_post', (bx - 0.035, sy * 0.36, 0.64), (bx - 0.035, sy * 0.36, 1.40), 0.022, self.M[xm], 14))
        self.add(rod('rack_top', (bx - 0.035, -0.38, 1.40), (bx - 0.035, 0.38, 1.40), 0.022, self.M[xm], 14))
        for sy in (-1, 1):
            self.add(parts.lathe_at('rack_elbow', [(0.0, -0.022), (0.026, -0.02), (0.026, 0.02), (0.0, 0.022)],
                                    Matrix.Translation((bx - 0.035, sy * 0.36, 1.40)) @ Matrix.Rotation(math.radians(90), 4, 'X'), 14), xm)
        for z in (0.90, 1.18):
            # A U-channel wheel tray, open to the front.
            sec = [(0.0, -0.045), (0.0, 0.045), (-0.055, 0.045), (-0.055, 0.039), (-0.006, 0.039), (-0.006, -0.039), (-0.055, -0.039), (-0.055, -0.045)]
            bm = bmesh.new()
            ring0 = [bm.verts.new((bx - 0.06 + a, -0.64, z + b)) for a, b in sec]
            ring1 = [bm.verts.new((bx - 0.06 + a, 0.64, z + b)) for a, b in sec]
            n = len(sec)
            for i in range(n):
                bm.faces.new((ring0[i], ring0[(i + 1) % n], ring1[(i + 1) % n], ring1[i]))
            bm.faces.new(ring0)
            bm.faces.new(list(reversed(ring1)))
            bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
            self.add(core.mesh_object('tray', bm), xm)
            for sy in (-1, 1):
                chock = [(sy * 0.64 + sy * 0.05 * math.sin(math.pi * k / 8), z - 0.04 + 0.08 * k / 8) for k in range(9)]
                self.add(parts.tube('tray_lip', [(bx - 0.09, yy, zz) for yy, zz in chock], 0.008, 8, 4), xm)
        # The support arms, folded down, their spring hooks at the top.
        for sy in (-1, 1):
            y = sy * 0.20
            self.add(rod('arm', (bx - 0.10, y, 0.70), (bx - 0.10, y, 1.28), 0.016, self.M['trim'], 10))
            self.add(parts.tube('hook', [(bx - 0.10, y, 1.28), (bx - 0.12, y, 1.33), (bx - 0.145, y, 1.32), (bx - 0.15, y, 1.28)], 0.012, 8, 4), 'trim')
            self.add(parts.rbox('arm_pad', (bx - 0.10, y, 1.18), (0.045, 0.05, 0.07), 0.012), 'trim')
        self.add(parts.rbox('rack_handle', (bx - 0.07, 0.0, 1.34), (0.04, 0.24, 0.035), 0.012), 'trim')

    def mirrors(self):
        """Transit mirrors: a flat and a convex head on a tubular arm, low on the curb side's corner, high on the
        street side's pillar."""
        for side in (1, -1):
            if side > 0:
                foot = (-6.045, 1.205, 1.06)
                arm = [(-6.05, 1.212, 1.06), (-6.08, 1.30, 1.10), (-6.085, 1.40, 1.20), (-6.062, 1.44, 1.32)]
                hz, cz = 1.76, 1.46
                stem = (1.32, 1.76)
            else:
                foot = (-5.90, -1.302, 2.20)
                arm = [(-5.91, -1.31, 2.21), (-5.97, -1.39, 2.245), (-6.04, -1.44, 2.225), (-6.062, -1.44, 2.14)]
                hz, cz = 1.97, 1.67
                stem = (1.67, 2.14)
            y = side * 1.44
            x = -6.01
            self.add(parts.rbox('mirror_foot', foot, (0.05, 0.05, 0.09), 0.012), 'trim')
            self.add(parts.tube('mirror_arm', arm, 0.016, 10, 6), 'trim')
            self.add(rod('mirror_stem', (-6.058, y, stem[0]), (-6.058, y, stem[1]), 0.013, self.M['trim'], 10))
            self.add(parts.rbox('mirror_head', (x, y, hz), (0.07, 0.22, 0.36), 0.03, 3), 'trim')
            self.add(parts.rbox('mirror_glass', (x + 0.034, y, hz), (0.004, 0.196, 0.336), 0.012), 'metal')
            self.add(parts.rbox('convex_head', (x, y, cz), (0.07, 0.19, 0.19), 0.06, 4), 'trim')
            g = parts.lathe_at('convex_glass', [(0.0, 0.012), (0.04, 0.0105), (0.07, 0.006), (0.084, 0.0), (0.0, 0.0)],
                               Matrix.Translation((x + 0.033, y, cz)) @ Matrix.Rotation(math.radians(90), 4, 'Y'), 32)
            self.add(g, 'metal')

    def side_details(self):
        # Side markers, turn repeaters, clearance lamps along the roof edge.
        for side in (1, -1):
            y = side * (W + 0.008)
            for x, z in ((-0.60, 0.62), (5.70, 0.62)) + (((-5.75, 0.62),) if side < 0 else ()):
                self.add(parts.rbox('side_marker', (x, y, z), (0.09, 0.016, 0.04), 0.008), 'lamps')
            self.add(parts.rbox('turn_rep', (-4.52 if side > 0 else -4.45, y, 1.02), (0.11, 0.016, 0.045), 0.01), 'lamps')
            for x in (-5.82, 0.0, 5.84):
                self.add(parts.rbox('clear_side', (x, side * (W - 0.004), 2.93), (0.07, 0.03, 0.03), 0.008), 'lamps')
            # Engine bay door latches.
            for x in (4.40, 5.66):
                self.add(parts.rbox('latch', (x, side * (W + 0.006), 0.80), (0.06, 0.014, 0.024), 0.006), 'metal')
        # The side destination sign's housing (curb side, over the first window).
        x, y, z, w, h = SIGN_S
        self.add(frame_ring('side_sign_bezel', (x - w / 2 - 0.025, x + w / 2 + 0.025, z - h / 2 - 0.022, z + h / 2 + 0.022),
                            (x - w / 2 - 0.004, x + w / 2 + 0.004, z - h / 2 - 0.003, z + h / 2 + 0.003), W, 1.318, 'Y'), 'trim')
        self.add(box('side_sign_back', x - w / 2 - 0.01, x + w / 2 + 0.01, W + 0.0008, W + 0.0028, z - h / 2 - 0.01, z + h / 2 + 0.01, None), 'trim')
        # The radiator's louvers in their pocket (street side, at the back).
        x0, x1, z0, z1 = RADIATOR
        n = 30
        for k in range(n):
            zz = z0 + 0.03 + (z1 - z0 - 0.06) * k / (n - 1)
            sl = core.prism_x('louver', [(-1.284, zz - 0.012), (-1.272, zz - 0.016), (-1.270, zz + 0.012), (-1.282, zz + 0.016)], x0 + 0.02, x1 - 0.02)
            self.add(sl, 'trim')
        for xx in (x0 + 0.45, x1 - 0.45):
            self.add(box('louver_rib', xx - 0.01, xx + 0.01, -1.288, -1.266, z0 + 0.01, z1 - 0.01, None), 'trim')

    def rear_end(self):
        # Tail lamp clusters: three round lamps in a black housing each side, chrome rings.
        for sy in (1, -1):
            self.add(parts.rbox('tail_housing', (6.058, sy * 0.91, 1.10), (0.024, 0.24, 0.70), 0.05, 3), 'trim')
            for z in (0.88, 1.10, 1.32):
                self.add(parts.disc('tail_lens', (6.077, sy * 0.91, z), (1, 0, 0), 0.080, 0.018, 40), 'lamps')
                ring = parts.annulus('tail_ring', 0.080, 0.092, 0.0, 0.012, 40, Matrix.Translation((6.072, sy * 0.91, z)) @ Matrix.Rotation(math.radians(90), 4, 'Y'))
                self.add(ring, 'metal')
            self.add(parts.rbox('rear_clear', (6.04, sy * 1.10, 3.06), (0.03, 0.07, 0.03), 0.01), 'lamps')
        for yy in (-0.22, 0.0, 0.22):
            self.add(parts.rbox('rear_id', (6.068, yy, 3.06), (0.03, 0.07, 0.03), 0.01), 'lamps')
        self.add(parts.rbox('chmsl', (6.068, 0.0, 2.64), (0.022, 0.70, 0.05), 0.012), 'lamps')
        # The rear sign's housing.
        x, y, z, w, h = SIGN_R
        self.add(frame_ring('rear_sign_bezel', (-w / 2 - 0.025, w / 2 + 0.025, z - h / 2 - 0.025, z + h / 2 + 0.025),
                            (-w / 2 - 0.004, w / 2 + 0.004, z - h / 2 - 0.003, z + h / 2 + 0.003), 6.069, 6.091, 'X'), 'trim')
        self.add(box('rear_sign_back', 6.0705, 6.0725, -w / 2 - 0.01, w / 2 + 0.01, z - h / 2 - 0.01, z + h / 2 + 0.01, None), 'trim')
        # The engine door's vent louvers, its handle and the plate with its lamp.
        for k in range(6):
            zz = 1.32 + 0.16 * k / 5
            self.add(core.prism_x('vent', [(-0.69, zz - 0.008), (0.69, zz - 0.008), (0.69, zz + 0.008), (-0.69, zz + 0.008)], 6.03, 6.045), 'trim')
        self.add(parts.rbox('engine_handle', (6.058, 0.30, 1.05), (0.025, 0.16, 0.035), 0.01), 'metal')
        self.add(parts.rbox('plate_pocket', (6.049, -0.55, 0.805), (0.008, 0.33, 0.18), 0.006), 'trim')
        self.decal('plate_r', 'plate', (6.0548, -0.55, 0.805), (0, 1, 0), (0, 0, 1), 0.305, 0.152)
        self.add(parts.rbox('plate_lamp', (6.06, -0.55, 0.905), (0.025, 0.12, 0.022), 0.008), 'lamps')

    def roof(self):
        # The battery pod: a long fairing with a sloped nose.
        def pod(x0, x1, hw, h, nose, tail, name):
            rings = []
            xs = [x0, x0 + nose * 0.25, x0 + nose * 0.55, x0 + nose, x1 - tail, x1 - tail * 0.4, x1]
            hs = [0.03, h * 0.45, h * 0.8, h, h, h * 0.75, 0.04]
            for x, hh in zip(xs, hs):
                zt = 3.11 + hh
                prof = core.rounded_rect(-hw, 3.10, hw, zt, min(0.06, hh * 0.45), 4)
                rings.append([Vector((x, yy, zz)) for yy, zz in prof])
            return core.loft(name, rings)
        self.add(pod(-3.95, -0.45, 0.92, 0.19, 0.55, 0.20, 'battery_pod'), 'paint')
        # The air conditioning unit: a box with its condenser fans on top, louvers down the sides.
        self.add(parts.rbox('hvac', (4.0, 0.0, 3.20), (2.90, 2.10, 0.20), 0.05, 3), 'paint')
        self.add(parts.rbox('hvac_nose', (2.46, 0.0, 3.15), (0.22, 2.0, 0.10), 0.04, 3), 'paint')
        for x in (3.35, 4.65):
            self.add(parts.annulus('fan_ring', 0.27, 0.31, 3.298, 3.312, 48, Matrix.Translation((x, 0.0, 0.0))), 'metal')
            self.add(parts.disc('fan_well', (x, 0.0, 3.303), (0, 0, 1), 0.272, 0.004, 48), 'trim')
            for k in range(8):
                a = math.pi * k / 8
                d = Vector((math.cos(a), math.sin(a), 0.0)) * 0.27
                self.add(rod('fan_bar', Vector((x, 0, 3.3095)) - d, Vector((x, 0, 3.3095)) + d, 0.004, self.M['metal'], 6))
            self.add(parts.disc('fan_hub', (x, 0.0, 3.311), (0, 0, 1), 0.05, 0.012, 24), 'metal')
        for side in (1, -1):
            for k in range(12):
                x = 2.75 + 2.5 * k / 11
                self.add(box('hvac_louver', x - 0.06, x + 0.06, side * 1.052, side * 1.062, 3.14, 3.27, None), 'trim')
        # Roof hatches, antennas, the exhaust stack.
        for x in (-4.85, 1.95):
            self.add(parts.rbox('hatch', (x, 0.0, 3.178), (0.68, 0.66, 0.03), 0.02, 2), 'trim')
            self.add(parts.rbox('hatch_lid', (x, 0.0, 3.2), (0.56, 0.54, 0.025), 0.015, 2), 'trim')
        self.add(parts.lathe_at('gps', [(0.0, 3.16), (0.07, 3.16), (0.065, 3.18), (0.04, 3.195), (0.0, 3.2)], Matrix.Translation((-5.35, 0.35, 0.0)), 24), 'trim')
        self.add(rod('whip', (-5.45, -0.35, 3.15), (-5.45, -0.35, 3.40), 0.004, self.M['trim'], 6))
        self.add(parts.lathe_at('whip_base', [(0.0, 3.15), (0.025, 3.15), (0.02, 3.19), (0.0, 3.2)], Matrix.Translation((-5.45, -0.35, 0.0)), 16), 'trim')
        self.add(parts.lathe_at('stack', [(0.0, 3.08), (0.06, 3.08), (0.06, 3.27), (0.065, 3.28), (0.065, 3.29), (0.052, 3.29), (0.052, 3.10), (0.0, 3.10)],
                                Matrix.Translation((5.72, -0.80, 0.0)), 24), 'metal')

    def rear_decal(self, name, key, yc, zc, w, h, nu=8, nv=3):
        """A print on the rear face, following its slight bow (DECAL_E off it), reading toward +Y."""
        srf = self.srf
        rows = []
        for i in range(nv + 1):
            z = zc - h / 2 + h * i / nv
            p = srf.rear(z)
            S = cum(p)
            rows.append([(*at_s(p, S, s_where(p, S, lambda q: q[1], yc - w / 2 + w * j / nu)), z) for j in range(nu + 1)])
        return self.add(skin_decal(grid_object(name, rows, (1, 0, 0), DECAL_E, self.uvr(key))), 'prints')

    def exterior_decals(self):
        e = DECAL_E
        # CITY TRANSIT on the green band, both sides (in lengths, so the paint's normals carry through them).
        zc = 0.80 + (BB - 0.045 - 0.80) / 2
        self.decal('wm_r', 'wm_r', (-1.72, W + e, zc), (-1, 0, 0), (0, 0, 1), 2.20, 0.24, 8, 1, skin=True)
        self.decal('wm_l', 'wm_l', (-0.45, -W - e, zc), (1, 0, 0), (0, 0, 1), 2.20, 0.24, 8, 1, skin=True)
        # Fleet numbers (the front pair between the roof panels' joints at x -5.30 and -4.70).
        self.decal('fl_rf', 'fl_rf', (-5.02, W + e, 2.818), (-1, 0, 0), (0, 0, 1), 0.42, 0.115, 2, 2, skin=True)
        self.decal('fl_lf', 'fl_lf', (-5.02, -W - e, 2.79), (1, 0, 0), (0, 0, 1), 0.42, 0.115, 2, 2, skin=True)
        self.decal('fl_rr', 'fl_rr', (5.52, W + e, 2.78), (-1, 0, 0), (0, 0, 1), 0.46, 0.14, 2, 2, skin=True)
        self.decal('fl_lr', 'fl_lr', (5.52, -W - e, 2.78), (1, 0, 0), (0, 0, 1), 0.46, 0.14, 2, 2, skin=True)
        self.rear_decal('fl_rear', 'fl_rear', 0.70, 2.79, 0.42, 0.13)
        self.rear_decal('wm_rear', 'wm_rear', -0.66, 2.79, 0.62, 0.11)
        self.rear_decal('yield', 'yield', -0.49, 1.10, 0.26, 0.23, 4, 4)
        # The roof number on the crown between the battery pod and the rear hatch, reading from the curb side (from the
        # plaza above the stop): its top toward the street side (-Y), running toward the front (-X).
        u0, v0, u1, v1 = self.uvr('fl_roof')
        wr, hr, xc = 1.50, 0.60, 0.45
        nu, nv = 16, 8
        rows = []
        for i in range(nv + 1):
            yy = hr / 2 - hr * i / nv
            zz = Z_CROWN - (Z_CROWN - (ZS + R_ROOF * math.sin(TH_END))) * (abs(yy) / (W - R_ROOF + R_ROOF * math.cos(TH_END))) ** 2
            rows.append([(xc + wr / 2 - wr * j / nu, yy, zz) for j in range(nu + 1)])
        self.add(skin_decal(grid_object('fl_roof', rows, (0, 0, 1), DECAL_E, (u0, v0, u1, v1))), 'prints')
        # ADA symbols by the doors (the front one a sticker on the first window's glass, the rear one on the frit), the
        # kneeling sticker, the route cards in the first windows.
        self.decal('ada_r1', 'ada_r1', (-4.53, W + e, 1.38), (-1, 0, 0), (0, 0, 1), 0.15, 0.15)
        self.decal('ada_r2', 'ada_r2', (1.20, W + e, 1.29), (-1, 0, 0), (0, 0, 1), 0.15, 0.15, skin=True)
        self.decal('kneel', 'kneel', (-4.48, W + e, 0.92), (-1, 0, 0), (0, 0, 1), 0.28, 0.075, skin=True)
        for side, (x0, x1, z0, z1) in ((1, R_WIN[0]), (-1, L_WIN[1])):
            xc = x0 + 0.36 if side > 0 else x1 - 0.36
            y = side * (W - 0.03)
            self.decal('route_card', 'route', (xc, y, z0 + 0.105), (-side, 0, 0), (0, 0, 1), 0.60, 0.19)
            self.add(quad('route_back', (xc, y - side * 0.0015, z0 + 0.105), (side, 0, 0), (0, 0, 1), 0.60, 0.19), 'interior', IP_CARD)

    # -------------------------------------------------------------- inside

    def floors(self):
        ip = IP_FLOOR
        hw = W - T
        fa, fb = AXLES[0] - 0.66, AXLES[0] + 0.66
        # The low floor, notched round the front wheel houses, out to the skin at the doors' thresholds.
        outline = [(-5.99, -1.08), (-5.90, -hw), (fa, -hw), (fa, -0.72), (fb, -0.72), (fb, -hw), (X_STEP1, -hw), (X_STEP1, hw),
                   (DOOR_R[1], hw), (DOOR_R[1], W - 0.004), (DOOR_R[0], W - 0.004), (DOOR_R[0], hw), (fb, hw), (fb, 0.72), (fa, 0.72), (fa, hw),
                   (DOOR_F[1], hw), (DOOR_F[1], W - 0.004), (DOOR_F[0], W - 0.004), (-5.90, hw), (-5.99, 1.08)]
        self.add(prism_z('floor', outline, Z_FLOOR - 0.05, Z_FLOOR), 'interior', ip)
        # The steps and the rear deck, notched round the rear wheel houses.
        self.add(box('step1', X_STEP1, X_STEP2, -hw, hw, Z_FLOOR - 0.05, Z_STEP1, None), 'interior', ip)
        ra, rb = AXLES[1] - 0.66, AXLES[1] + 0.66
        deck = [(X_STEP2, -hw), (ra, -hw), (ra, -0.72), (rb, -0.72), (rb, -hw), (5.92, -hw), (5.995, -1.10), (5.995, 1.10), (5.92, hw), (rb, hw),
                (rb, 0.72), (ra, 0.72), (ra, hw), (X_STEP2, hw)]
        self.add(prism_z('deck', deck, Z_FLOOR - 0.05, Z_RAISED), 'interior', ip)
        # Yellow nosings: the steps, the doors' thresholds.
        for x, z in ((X_STEP1, Z_STEP1), (X_STEP2, Z_RAISED)):
            self.add(box('nosing', x - 0.002, x + 0.05, -hw + 0.01, hw - 0.01, z - 0.003, z + 0.004, None, 0.002), 'interior', IP_YRUB)
            self.add(box('nosing_f', x - 0.006, x - 0.001, -hw + 0.01, hw - 0.01, z - 0.045, z + 0.004, None), 'interior', IP_YRUB)
        for x0, x1 in (DOOR_F, DOOR_R):
            self.add(box('threshold', x0 + 0.01, x1 - 0.01, W - 0.06, W - 0.003, Z_FLOOR - 0.002, Z_FLOOR + 0.004, None, 0.002), 'interior', IP_YRUB)
        # The wheelchair ramp folded into the floor at the front door.
        self.add(box('ramp', DOOR_F[0] + 0.05, DOOR_F[1] - 0.05, 0.30, W - 0.07, Z_FLOOR, Z_FLOOR + 0.012, None, 0.004), 'interior', IP_HOUSING)
        for yy in (0.31, W - 0.08):
            self.add(box('ramp_edge', DOOR_F[0] + 0.05, DOOR_F[1] - 0.05, yy - 0.012, yy + 0.012, Z_FLOOR + 0.002, Z_FLOOR + 0.015, None, 0.003),
                     'interior', IP_YRUB)
        # The engine hump under the rear bench.
        hump = [(5.38, -hw), (5.90, -hw), (5.99, -1.10), (5.99, 1.10), (5.90, hw), (5.38, hw)]
        self.add(prism_z('hump', hump, Z_RAISED - 0.02, 1.05), 'interior', IP_HOUSING)

    def housings(self):
        hw = W - T
        for ax, z0 in ((AXLES[0], Z_FLOOR), (AXLES[1], Z_RAISED)):
            for sy in (1, -1):
                # An open cowl over the liner: from the wall across the top, rounded down to the floor at the aisle side,
                # closed at its ends (the liner shows from below through the arch, not a floor of this).
                r = 0.08
                sec = [(hw + 0.002, 1.18)] + [(0.72 + r - r * math.sin(math.pi / 2 * k / 6), 1.18 - r + r * math.cos(math.pi / 2 * k / 6)) for k in range(7)] + \
                      [(0.72, z0 - 0.02)]
                bm = bmesh.new()
                xa, xb = ax - 0.66, ax + 0.66
                ra = [bm.verts.new((xa, sy * yy, zz)) for yy, zz in sec]
                rb = [bm.verts.new((xb, sy * yy, zz)) for yy, zz in sec]
                for i in range(len(sec) - 1):
                    bm.faces.new((ra[i], ra[i + 1], rb[i + 1], rb[i]))
                ca = bm.faces.new(ra + [bm.verts.new((xa, sy * (hw + 0.002), z0 - 0.02))])
                cb = bm.faces.new(rb + [bm.verts.new((xb, sy * (hw + 0.002), z0 - 0.02))])
                bm.normal_update()
                for f in bm.faces:
                    c = f.calc_center_median()
                    want = Vector((-1.0 if f is ca else 1.0, 0.0, 0.0)) if f in (ca, cb) else Vector((0.0, -sy * 0.7, 0.7))
                    if f.normal.dot(want) < 0:
                        f.normal_flip()
                self.add(core.mesh_object('housing', bm), 'interior', IP_HOUSING)

    def walls(self):
        hw = W - T
        # Window sills inside, the heater ducts along the floor, door operators over the doors.
        for side, wins in ((1, R_WIN), (-1, L_WIN)):
            for i, (x0, x1, z0, z1) in enumerate(wins):
                if side < 0 and i == 0:
                    continue
                self.add(box('sill', x0 - 0.01, x1 + 0.01, side * (hw - 0.04), side * (hw + 0.004), z0 - 0.035, z0 - 0.003, None, 0.006), 'interior', IP_HOUSING)
        duct = {1: [(AXLES[0] + 0.68, -0.20), (X_STEP2, AXLES[1] - 0.68), (AXLES[1] + 0.68, 5.30)],
                -1: [(AXLES[0] + 0.68, X_STEP1), (X_STEP2, AXLES[1] - 0.68), (AXLES[1] + 0.68, 5.30)]}
        for side, ss in duct.items():
            for x0, x1 in ss:
                z0 = Z_FLOOR if x1 <= X_STEP1 + 0.01 else Z_RAISED
                self.add(box('duct', x0, x1, side * (hw - 0.09), side * (hw + 0.003), z0 - 0.005, z0 + 0.13, None, 0.01), 'interior', IP_DUCT)
        for x0, x1 in (DOOR_F, DOOR_R):
            self.add(box('door_op', x0 - 0.06, x1 + 0.06, 1.02, hw + 0.004, 2.755, CEIL_Z + 0.005, None, 0.01), 'interior', IP_HOUSING)
        # Emergency exit labels under the exit windows.
        for sd, i in EXIT_WIN:
            side = 1 if sd == 'R' else -1
            x0, x1, z0, z1 = (R_WIN if side > 0 else L_WIN)[i]
            self.decal('exit_label', 'exit', ((x0 + x1) / 2, side * (hw - 0.0025), z0 - 0.09), (side, 0, 0), (0, 0, 1), 0.34, 0.07)
            # The exit's red release bar along the top of the opening, on two brackets.
            yb = side * (hw - 0.03)
            self.add(rod('exit_bar', (x0 + 0.12, yb, z1 - 0.03), (x1 - 0.12, yb, z1 - 0.03), 0.011, self.M['interior'], 12, IP_RED))
            for xx in (x0 + 0.14, x1 - 0.14):
                self.add(box('exit_bracket', xx - 0.015, xx + 0.015, side * (hw - 0.035), side * (hw + 0.002), z1 - 0.045, z1 - 0.015, None, 0.004),
                         'interior', IP_BLACK)
        # The doors' pivot shafts run up into bearing blocks in the headers, and down into the step.
        for hx, _ in HINGES:
            self.add(box('pivot_top', hx - 0.03, hx + 0.03, HINGE_Y - 0.028, HINGE_Y + 0.028, Z_FLOOR + LEAF_H + 0.002, DOOR_Z[1] + 0.012, None, 0.004), 'trim')
            self.add(box('pivot_foot', hx - 0.028, hx + 0.028, HINGE_Y - 0.026, HINGE_Y + 0.026, DOOR_Z[0] - 0.004, Z_FLOOR - 0.002, None, 0.004), 'metal')

    def ceiling(self):
        """The ceiling's center panel and side panels, the ad racks' slanted backs over the windows (not over the
        doors), the cab's header box."""
        hw = W - T
        prof = [(-1.09, 2.762), (-0.62, 2.852), (-0.605, 2.852), (-0.605, CEIL_Z), (0.605, CEIL_Z), (0.605, 2.852), (0.62, 2.852), (1.09, 2.762)]
        bm = bmesh.new()
        x0, x1 = -4.60, 5.985
        a = [bm.verts.new((x0, y, z)) for y, z in prof]
        b = [bm.verts.new((x1, y, z)) for y, z in prof]
        for i in range(len(prof) - 1):
            bm.faces.new((a[i], a[i + 1], b[i + 1], b[i]))
        bm.normal_update()
        if sum(f.normal.z for f in bm.faces) > 0:
            bmesh.ops.reverse_faces(bm, faces=bm.faces[:])
        o = core.mesh_object('ceiling', bm)
        self.add(o, 'interior', IP_CEIL)
        # Racks: each side, a slanted back from the wall over the windows up to the side panel.
        racks = {1: [(-4.60, DOOR_R[0] - 0.06), (DOOR_R[1] + 0.06, 5.78)], -1: [(-4.60, 5.78)]}
        self.rack_runs = racks
        for side, ss in racks.items():
            for xa, xb in ss:
                bm = bmesh.new()
                sec = [(side * (hw + 0.002), 2.455), (side * 1.12, 2.74), (side * 1.09, 2.762)]
                a = [bm.verts.new((xa, y, z)) for y, z in sec]
                b = [bm.verts.new((xb, y, z)) for y, z in sec]
                for i in range(len(sec) - 1):
                    bm.faces.new((a[i], a[i + 1], b[i + 1], b[i]))
                bm.normal_update()
                if sum(f.normal.y * side for f in bm.faces) > 0:
                    bmesh.ops.reverse_faces(bm, faces=bm.faces[:])
                self.add(core.mesh_object('rack', bm), 'interior', IP_WALL)
                # The card holders' lips.
                for (yy, zz) in ((side * (hw - 0.012), 2.468), (side * 1.13, 2.722)):
                    self.add(core.prism_x('rack_lip', [(yy - 0.008, zz - 0.006), (yy + 0.008, zz - 0.006), (yy + 0.008, zz + 0.006), (yy - 0.008, zz + 0.006)], xa, xb),
                             'interior', IP_ALU)
                # Gable ends where a run stops short of a door.
                for xe in (xa, xb):
                    if -4.0 < xe < 5.9:
                        self.add(pane('rack_end', [(xe, side * (hw + 0.002), 2.455), (xe, side * 1.09, 2.762), (xe, side * (hw + 0.002), 2.762)],
                                      (1 if xe == xa else -1, 0, 0)), 'interior', IP_WALL)
        # Air diffusers along the side panels: an aluminium frame on the slope, three louvers in it.
        for side in (1, -1):
            t = Vector((0.0, side * (0.62 - 1.09), 2.852 - 2.762)).normalized()
            n = Vector((1.0, 0.0, 0.0)).cross(t)
            if n.z > 0:
                n = -n
            rot = Matrix((Vector((1.0, 0.0, 0.0)), t, n)).transposed()
            mid = Vector((0.0, side * 0.855, 2.807))
            for k in range(8):
                xc = -4.0 + 1.27 * k
                c = Vector((xc, 0.0, 0.0)) + mid + n * 0.004
                self.add(parts.rbox('diffuser', c, (0.92, 0.075, 0.008), 0.003, 2, rot=rot), 'interior', IP_ALU)
                for j in (-1, 0, 1):
                    self.add(parts.rbox('louver', c + t * (0.02 * j) + n * 0.005, (0.88, 0.012, 0.006), 0.002, 1, rot=rot), 'interior', IP_DUCT)
        # Over the rear bench, a flat panel closes the ceiling to the rear wall's rounded corners.
        ring = ring_xy(*inner_params(2.762))
        tail = [(x, y) for x, y in ring if x > 5.78]
        tail = sorted(tail, key=lambda p: math.atan2(p[1], p[0] - 5.70))
        outline = [(5.78, -hw)] + [(x - 0.004 * (x - 5.7) / max(abs(x - 5.7), 1e-6), y * 0.996) for x, y in tail] + [(5.78, hw)]
        self.add(pane('ceiling_tail', [(x, y, 2.762) for x, y in outline], (0, 0, -1)), 'interior', IP_CEIL)
        # The cab's header: the sign's enclosure over the windshield and the front door.
        self.add(box('header', -6.0, -4.60, -hw, hw, 2.70, CEIL_Z + 0.02, None, 0.01), 'interior', IP_CEIL)
        self.add(box('display', -4.60, -4.50, -0.43, 0.43, 2.70, 2.815, None, 0.008), 'interior', IP_BLACK)
        self.add(box('display_face', -4.505, -4.495, -0.40, 0.40, 2.715, 2.795, None), 'lamps')
        self.decal('hdr_l', 'hdr_l', (-4.5975, -0.82, 2.758), (0, 1, 0), (0, 0, 1), 0.52, 0.11)
        self.decal('hdr_r', 'hdr_r', (-4.5975, 0.82, 2.758), (0, 1, 0), (0, 0, 1), 0.52, 0.11)
        self.decal('rear_hdr', 'rear_hdr', ((DOOR_R[0] + DOOR_R[1]) / 2, 1.0175, 2.808), (1, 0, 0), (0, 0, 1), 1.00, 0.08)
        # The roller blind over the windshield and the inside mirror.
        self.add(box('blind', -6.0, -5.93, -1.15, 0.30, 2.62, 2.70, None, 0.02), 'interior', IP_DASH)
        self.add(parts.rbox('in_mirror', (-5.93, 0.0, 2.58), (0.03, 0.28, 0.08), 0.02), 'interior', IP_DASH)
        self.add(rod('in_mirror_stem', (-5.95, 0.0, 2.62), (-5.95, 0.0, 2.70), 0.008, self.M['interior'], 8, IP_DASH))

    def ad_cards(self):
        """The cards in the racks: each 0.71 x 0.28 on the slanted back, reading from inside."""
        hw = W - T
        plan = {1: ['map', 'priority', 'ad_ec', 'ad_lp', 'ad_owl', 'ad_ec', 'ad_ct', 'ad_lp', 'ad_ec', 'ad_owl', 'ad_ct', 'ad_ec'],
                -1: ['priority', 'ad_lp', 'ad_ec', 'ad_ct', 'ad_owl', 'ad_ec', 'ad_lp', 'map', 'ad_ec', 'ad_ct', 'ad_lp', 'ad_owl', 'ad_ec', 'ad_lp']}
        for side, ss in self.rack_runs.items():
            keys = list(plan[side])
            for xa, xb in ss:
                x = xa + 0.08
                while keys:
                    key = keys[0]
                    w = 1.42 if key == 'map' else 0.71
                    if x + w > xb - 0.06:
                        break
                    keys.pop(0)
                    up = Vector((0.0, side * (1.12 - (hw + 0.002)), 2.74 - 2.455)).normalized()
                    inward = Vector((0.0, -side, 0.0))
                    base = Vector((x + w / 2, side * (hw + 0.002), 2.455)) + up * 0.168
                    nrm = up.cross(Vector((1.0, 0.0, 0.0))) * side
                    if nrm.dot(inward) < 0:
                        nrm = -nrm
                    u_dir = Vector((side, 0.0, 0.0))
                    self.decal('card', key, base + nrm * 0.0025, u_dir, up, w - 0.01, 0.275)
                    x += w + 0.035

    def seat(self, x, y0, y1, z, n=None, back_h=0.56, flip=False, bench=False):
        """A row of forward-facing transit seats from y0 to y1, its front edge at x, on a floor at height z: a grey
        shell pan and back (leaning back 8 deg), moquette pads, a yellow grab loop on the aisle corner of the back, a
        pedestal at the aisle and a bracket to the wall. flip: the pans folded up (the wheelchair bay's)."""
        M = self.M['interior']
        width = abs(y1 - y0)
        n = n or max(1, round(width / 0.44))
        sw = width / n
        ym = (y0 + y1) / 2
        hy = width / 2
        place = Matrix.Translation((x, ym, z))
        back = place @ Matrix.Translation((0.47, 0.0, 0.43)) @ Matrix.Rotation(math.radians(8), 4, 'Y')

        def add(o, ip, mtx=place):
            o.data.transform(mtx)
            self.body.append(put(o, M, ip))
        # The aisle end of the row (local y): the end nearer the bus's middle.
        aisle = -hy if ym > 0 else hy
        a_in = aisle + (0.04 if ym > 0 else -0.04)
        if flip:
            for k in range(n):
                yc = -hy + sw * (k + 0.5)
                add(parts.rbox('pan_up', (0.36, yc, 0.66), (0.05, sw - 0.02, 0.42), 0.02, 2), IP_SHELL)
                add(parts.rbox('hinge', (0.36, yc, 0.42), (0.06, sw - 0.10, 0.04), 0.012), IP_FRAME)
        else:
            add(parts.rbox('pan', (0.21, 0.0, 0.385), (0.44, width - 0.01, 0.05), 0.02, 2), IP_SHELL)
        for k in range(n):
            yc = -hy + sw * (k + 0.5)
            if not flip:
                add(parts.rbox('pad', (0.205, yc, 0.442), (0.40, sw - 0.035, 0.065), 0.03, 5), IP_MOQ)
            add(parts.rbox('back_pad', (-0.028, yc, 0.07 + back_h * 0.42), (0.035, sw - 0.05, back_h * 0.74), 0.016, 4), IP_MOQ, back)
        add(parts.rbox('back_shell', (0.0, 0.0, (back_h + 0.08) / 2), (0.03, width - 0.01, back_h + 0.08), 0.014, 2), IP_SHELL, back)
        add(parts.rbox('back_cap', (0.0, 0.0, back_h + 0.08), (0.05, width - 0.008, 0.03), 0.014, 2), IP_SHELL, back)
        for k in range(n):
            yc = -hy + sw * (k + 0.5)
            add(parts.rbox('back_mold', (0.016, yc, back_h * 0.5 + 0.02), (0.012, sw - 0.09, back_h * 0.62), 0.02, 3), IP_SHELL, back)
        if bench:
            return
        add(parts.rbox('pedestal', (0.22, a_in + (0.03 if ym > 0 else -0.03), 0.18), (0.05, 0.05, 0.36), 0.01), IP_FRAME)
        add(parts.rbox('foot', (0.22, a_in + (0.03 if ym > 0 else -0.03), 0.008), (0.16, 0.08, 0.016), 0.006), IP_FRAME)
        wall = hy if ym > 0 else -hy
        add(parts.rbox('bracket', (0.22, wall - (0.06 if ym > 0 else -0.06), 0.30), (0.08, 0.14, 0.10), 0.01), IP_FRAME)
        # The grab loop on the back's aisle corner.
        top = back_h + 0.08
        loop = [(0.0, a_in, top - 0.06), (-0.01, a_in, top + 0.06), (0.05, a_in, top + 0.11), (0.11, a_in, top + 0.06), (0.10, a_in, top - 0.04)]
        add(parts.tube('grab', loop, 0.013, 10, 5), IP_YELLOW, back)

    def seats(self):
        hw = W - T
        s = self.seat
        # Over the front wheel houses: a forward-facing double each side, raised.
        for sy in (1, -1):
            s(AXLES[0] - 0.30, sy * 0.40, sy * (hw - 0.02), 0.80)
        # Left: the wheelchair bay (flip seats folded), forward-facing doubles to the steps.
        s(-3.02, -0.40, -(hw - 0.02), Z_FLOOR, flip=True)
        for x in (-1.40, -0.65, 0.10):
            s(x, -0.40, -(hw - 0.02), Z_FLOOR)
        s(0.82, -0.40, -(hw - 0.02), Z_FLOOR)
        # Right: three side-facing priority seats against the wall, looking across the aisle.
        self.side_seats(-3.05, -1.70)
        # The rear deck: doubles both sides (over the rear wheel houses, raised), and the rear bench.
        for x in (1.92, 2.68, 3.44, 4.20):
            zz = 0.80 if abs(x + 0.21 - AXLES[1]) < 0.7 else Z_RAISED
            for sy in (1, -1):
                s(x, sy * 0.40, sy * (hw - 0.02), zz)
        s(5.40, -(hw - 0.03), hw - 0.03, Z_RAISED, n=5, back_h=0.62, bench=True)
        self.driver_seat()

    def side_seats(self, x0, x1):
        """Side-facing priority seats on the curb side: three, their backs to the wall, looking across (-Y)."""
        M = self.M['interior']
        hw = W - T
        z = Z_FLOOR
        n = 3
        sw = (x1 - x0) / n
        add = self.body.append
        add(put(box('ps_pan', x0, x1, hw - 0.46, hw - 0.02, z + 0.36, z + 0.41, None, 0.02), M, IP_SHELL))
        for k in range(n):
            xa = x0 + sw * k + 0.015
            xb = x0 + sw * (k + 1) - 0.015
            add(put(box('ps_pad', xa, xb, hw - 0.45, hw - 0.04, z + 0.41, z + 0.47, None, 0.028, 3), M, IP_MOQ))
            add(put(box('ps_back', xa + 0.01, xb - 0.01, hw - 0.075, hw - 0.03, z + 0.50, z + 0.98, None, 0.022, 3), M, IP_MOQ))
        add(put(box('ps_shell', x0, x1, hw - 0.03, hw - 0.005, z + 0.40, z + 1.02, None, 0.012), M, IP_SHELL))
        add(put(box('ps_leg', x0 + 0.05, x1 - 0.05, hw - 0.42, hw - 0.38, z, z + 0.36, None, 0.01), M, IP_FRAME))

    def driver_seat(self):
        """The driver's seat where the game's seated figure sits (spec 2.9: its frame at (-4.95, -0.70, 0.36) facing -X, the
        seat 20 cm behind that at 46 cm: the cushion's top at 0.82, centered at x -4.75), the wheel where its hands rest,
        the dash, the gauges, the switch console, the barrier door and the partition behind."""
        M = self.M['interior']
        x, y, z = -4.75, -0.70, Z_FLOOR
        add = self.body.append
        add(put(parts.rbox('dseat_post', (x + 0.02, y, z + 0.13), (0.18, 0.18, 0.26), 0.02), M, IP_FRAME))
        add(put(parts.rbox('dseat_bellows', (x + 0.02, y, z + 0.29), (0.26, 0.26, 0.08), 0.03, 3), M, IP_BLACK))
        add(put(parts.rbox('dseat_base', (x, y, z + 0.355), (0.44, 0.42, 0.05), 0.02), M, IP_FRAME))
        add(put(parts.rbox('dseat_pad', (x, y, z + 0.415), (0.50, 0.50, 0.095), 0.045, 4), M, IP_VINYL))
        tilt = Matrix.Translation((x + 0.23, y, z + 0.44)) @ Matrix.Rotation(math.radians(12), 4, 'Y')
        for nm, c, sz, r in (('dseat_back', (0.0, 0.0, 0.34), (0.11, 0.50, 0.66), 0.05), ('dseat_head', (0.01, 0.0, 0.80), (0.10, 0.30, 0.20), 0.045),
                             ('dseat_bolster_l', (-0.035, -0.22, 0.30), (0.08, 0.07, 0.50), 0.03), ('dseat_bolster_r', (-0.035, 0.22, 0.30), (0.08, 0.07, 0.50), 0.03)):
            o = parts.rbox(nm, c, sz, r, 4)
            o.data.transform(tilt)
            add(put(o, M, IP_VINYL))
        add(put(parts.rbox('dseat_arm', (x + 0.02, y + 0.29, z + 0.66), (0.34, 0.06, 0.06), 0.025, 3), M, IP_VINYL))
        add(put(parts.rod('dseat_arm_post', (x + 0.16, y + 0.29, z + 0.44), (x + 0.16, y + 0.29, z + 0.64), 0.012, 8), M, IP_FRAME))
        # The wheel, nearly flat as a bus's is, on its column out of the dash.
        wc = Vector((-5.28, y, 1.22))
        tw = Matrix.Rotation(math.radians(35), 4, 'Y')
        nrm = tw @ Vector((0.0, 0.0, 1.0))
        ring = [wc + tw @ Vector((0.25 * math.cos(2 * math.pi * k / 48), 0.25 * math.sin(2 * math.pi * k / 48), 0.0)) for k in range(48)]
        add(put(core.sweep('wheel', ring, parts.circle(0.018, 12), closed=True), M, IP_DASH))
        for a in (90, 210, 330):
            d = tw @ Vector((math.cos(math.radians(a)), math.sin(math.radians(a)), 0.0))
            add(put(parts.rod('spoke', wc - nrm * 0.02, wc + d * 0.24, 0.014, 8), M, IP_DASH))
        add(put(parts.disc('hub_pad', wc + nrm * 0.005, nrm, 0.075, 0.045, 32), M, IP_DASH))
        add(put(parts.rod('column', wc - nrm * 0.02, wc - nrm * 0.52, 0.042, 16), M, IP_DASH))
        add(put(parts.rod('column_boot', wc - nrm * 0.30, wc - nrm * 0.55, 0.07, 16), M, IP_BLACK))
        add(put(box('dash', -6.0, -5.62, -(W - T) + 0.02, -0.26, Z_FLOOR - 0.01, 1.04, None, 0.03), M, IP_DASH))
        add(put(parts.rbox('gauge_pod', (-5.60, y, 1.10), (0.12, 0.48, 0.16), 0.04, 3), M, IP_DASH))
        self.add(box('gauge_face', -5.542, -5.536, y - 0.20, y + 0.20, 1.045, 1.155, None), 'lamps')
        add(put(box('console', -5.98, -5.30, -0.30, -0.06, Z_FLOOR - 0.01, 0.95, None, 0.02), M, IP_DASH))
        for k in range(3):
            for j in range(5):
                add(put(box('switch', -5.70 + 0.11 * k, -5.66 + 0.11 * k, -0.27 + 0.045 * j, -0.24 + 0.045 * j, 0.95, 0.962, None, 0.003), M, IP_BLACK))
        for px, w_ in ((-5.70, 0.05), (-5.55, 0.08)):
            add(put(box('pedal', px - 0.04, px + 0.04, y - w_, y + w_, Z_FLOOR + 0.02, Z_FLOOR + 0.05, None, 0.01), M, IP_BLACK))
        add(put(box('shelf', -6.02, -5.92, -0.26, 1.18, 0.95, 1.10, None, 0.015), M, IP_DASH))
        for k in range(12):
            yy = -0.18 + 1.28 * k / 11
            add(put(box('vent', -6.0, -5.94, yy - 0.04, yy + 0.04, 1.098, 1.103, None), M, IP_BLACK))
        # The barrier door to the driver's right and the partition behind the seat (glass upper, stainless lower).
        add(put(box('barrier_low', -5.24, -4.46, -0.29, -0.25, Z_FLOOR, 1.10, None, 0.01), M, IP_STEEL))
        add(put(box('partition_low', -4.45, -4.41, -(W - T) + 0.01, -0.30, Z_FLOOR, 1.25, None, 0.01), M, IP_STEEL))
        for pts in ([(-5.24, -0.27, 1.10), (-5.24, -0.27, 1.96), (-4.46, -0.27, 1.96), (-4.46, -0.27, 1.10)],
                    [(-4.43, -1.23, 1.25), (-4.43, -1.23, 1.96), (-4.43, -0.30, 1.96), (-4.43, -0.30, 1.25)]):
            add(put(parts.tube('barrier_frame', pts, 0.016, 10, 2), M, IP_STEEL))
        self.glass.append((pane('barrier_glass', [(-5.23, -0.27, 1.10), (-4.47, -0.27, 1.10), (-4.47, -0.27, 1.95), (-5.23, -0.27, 1.95)], (0, 1, 0)), 0))
        self.glass.append((pane('partition_glass', [(-4.43, -1.22, 1.25), (-4.43, -0.31, 1.25), (-4.43, -0.31, 1.95), (-4.43, -1.22, 1.95)], (1, 0, 0)), 0))
        # The farebox, beside the driver, its face to the door; the card validator on the stanchion behind the door.
        fx, fy = -5.12, 0.02
        rot = Matrix.Translation((fx, fy, 0.0)) @ Matrix.Rotation(math.radians(20), 4, 'Z')
        for nm, c, sz, r, ipk in (('fb_ped', (0, 0, Z_FLOOR + 0.42), (0.24, 0.22, 0.84), 0.02, IP_STEEL),
                                  ('fb_head', (0, 0.0, 1.06), (0.30, 0.30, 0.36), 0.03, IP_STEEL),
                                  ('fb_top', (0, 0.0, 1.245), (0.28, 0.28, 0.03), 0.01, IP_BLACK)):
            o = parts.rbox(nm, c, sz, r, 2)
            o.data.transform(rot)
            add(put(o, M, ipk))
        q = quad('farebox_face', (0.0, 0.1525, 1.04), (-1, 0, 0), (0, 0, 1), 0.26, 0.30, self.uvr('farebox'))
        q.data.transform(rot)
        self.add(q, 'prints')
        q = quad('fare_plate', (0.0, 0.0, 1.2625), (-1, 0, 0), (0, -1, 0), 0.24, 0.12, self.uvr('fare'))
        q.data.transform(rot)
        self.add(q, 'prints')
        sc = box('fb_screen', -0.08, 0.08, 0.13, 0.155, 1.15, 1.205, None)
        sc.data.transform(rot)
        self.add(sc, 'lamps')

    def stanchions(self):
        M = self.M['interior']
        hw = W - T
        add = self.body.append
        # Overhead rails both sides of the aisle, their hangers to the ceiling.
        for sy in (1, -1):
            x0 = -4.40 if sy < 0 else -4.62
            add(put(parts.rod('rail', (x0, sy * RAIL_Y, RAIL_Z), (5.30, sy * RAIL_Y, RAIL_Z), 0.016, 20), M, IP_YELLOW))
            for x in [x0 + 0.05] + [x0 + 0.05 + 1.25 * k for k in range(1, 9) if x0 + 0.05 + 1.25 * k < 5.25] + [5.25]:
                add(put(parts.rod('hanger', (x, sy * RAIL_Y, RAIL_Z), (x, sy * RAIL_Y, 2.80 if abs(sy * RAIL_Y) > 0.6 else CEIL_Z), 0.012, 10), M, IP_YELLOW))
                add(put(parts.rbox('clamp', (x, sy * RAIL_Y, RAIL_Z), (0.05, 0.04, 0.045), 0.012), M, IP_YELLOW))
        # Vertical stanchions from the floor (or the deck) to the rails, a stop button on each.
        poles = [(-4.35, -1), (-3.10, -1), (-1.70, -1), (-0.62, -1), (0.48, -1), (1.62, -1), (2.95, -1), (4.40, -1),
                 (-4.62, 1), (-3.10, 1), (-1.66, 1), (1.62, 1), (2.95, 1), (4.40, 1)]
        for x, sy in poles:
            z0 = Z_FLOOR if x < X_STEP1 else Z_RAISED
            add(put(parts.rod('pole', (x, sy * RAIL_Y, z0), (x, sy * RAIL_Y, RAIL_Z), 0.019, 20), M, IP_YELLOW))
            add(put(parts.lathe_at('pole_foot', [(0.0, z0), (0.045, z0), (0.04, z0 + 0.02), (0.022, z0 + 0.04), (0.0, z0 + 0.04)],
                                   Matrix.Translation((x, sy * RAIL_Y, 0.0)), 20), M, IP_YELLOW))
            add(put(parts.rbox('stop_btn', (x, sy * RAIL_Y - sy * 0.022, 1.42), (0.045, 0.02, 0.075), 0.008), M, IP_RED))
        # Door poles at the portals, floor to the operator housings.
        for xx in (DOOR_F[1] + 0.05, DOOR_R[0] - 0.06, DOOR_R[1] + 0.06):
            add(put(parts.rod('door_pole', (xx, hw - 0.11, Z_FLOOR), (xx, hw - 0.11, 2.755), 0.019, 14), M, IP_YELLOW))
        # Modesty panels at the doors: a stainless frame round smoked glass, a grab rail along the top.
        for xx, sgn in ((DOOR_F[1] + 0.08, 1), (DOOR_R[0] - 0.09, -1), (DOOR_R[1] + 0.09, 1)):
            pts = [(xx, hw - 0.01, 0.42), (xx, 0.80, 0.42), (xx, 0.80, 1.62), (xx, hw - 0.01, 1.62)]
            add(put(parts.tube('modesty_frame', pts, 0.018, 10, 2), M, IP_STEEL))
            self.glass.append((pane('modesty_glass', [(xx, hw - 0.02, 0.43), (xx, 0.81, 0.43), (xx, 0.81, 1.61), (xx, hw - 0.02, 1.61)], (sgn, 0, 0)), 1))
        # The card validator on the pole behind the front door.
        add(put(parts.rbox('validator', (DOOR_F[1] + 0.05, hw - 0.11 - 0.06, 1.30), (0.06, 0.10, 0.17), 0.012), M, IP_BLACK))
        self.decal('validator_face', 'validator', (DOOR_F[1] + 0.05 - 0.0305, hw - 0.17, 1.30), (0, -1, 0), (0, 0, 1), 0.09, 0.15)
        # Wheelchair bay: a padded backboard, belt reels on the floor, the wall sign and the floor symbol.
        add(put(box('wc_board', -3.12, -3.08, -1.10, -0.58, 0.80, 1.55, None, 0.02), M, IP_BLUE))
        for xx, yy in ((-2.95, -0.55), (-2.95, -1.05), (-1.85, -0.55), (-1.85, -1.05)):
            add(put(parts.rbox('belt_reel', (xx, yy, Z_FLOOR + 0.03), (0.12, 0.08, 0.06), 0.012), M, IP_BLACK))
        self.decal('wc_sign', 'wc_sign', (-2.25, -(hw - 0.0025), 1.02), (-1, 0, 0), (0, 0, 1), 0.30, 0.30)
        self.decal('wc_floor', 'wc_floor', (-2.20, -0.80, Z_FLOOR + 0.002), (-1, 0, 0), (0, -1, 0), 0.62, 0.62)
        self.decal('floor_line', 'floor', (-4.55, 0.0, Z_FLOOR + 0.002), (0, 1, 0), (-1, 0, 0), 0.90, 0.10)

    # -------------------------------------------------------------- glass and signs

    def glazing(self):
        srf = self.srf
        g = []
        # The windshield: two panes on the cap's own surface, split at the divider.
        for left in (True, False):
            rows = []
            for i in range(25):
                z = 1.10 + (2.74 - 1.10) * i / 24
                p = srf.front(z)
                S = cum(p)
                sc = s_where(p, S, lambda q: q[1], 0.0)
                if left:
                    a = s_where(p, S, lambda q: q[0], X_PILLAR + 0.02)
                    af = s_where(p, S, lambda q: q[1], -(W - 0.13), a)
                    b = sc - 0.005
                    ss_ = [a + (af - a) * j / 18 for j in range(18)] + [af + (b - af) * j / 24 for j in range(25)]
                else:
                    a = sc + 0.005
                    bf_ = s_where(p, S, lambda q: q[1], W - 0.13, sc)
                    b = s_where(p, S, lambda q: q[0], X_PILLAR + 0.02, sc)
                    ss_ = [a + (bf_ - a) * j / 24 for j in range(24)] + [bf_ + (b - bf_) * j / 18 for j in range(19)]
                rows.append([(*at_s(p, S, s), z) for s in ss_])
            g.append((grid_object('windshield', rows, (-1, 0, 0), 0.0025), 0))
        # The side windows: flat panes over the openings and onto the frit.
        for side, wins in ((1, R_WIN), (-1, L_WIN)):
            for i, (x0, x1, z0, z1) in enumerate(wins):
                pts = [(x, side * (W + 0.004), z) for x, z in core.rounded_rect(x0 - 0.03, z0 - 0.03, x1 + 0.03, z1 + 0.03, 0.08, 10)]
                tint = 0 if (side < 0 and i == 0) else 1
                g.append((pane('side_glass', pts, (0, side, 0)), tint))
        # The rear window on the rear face.
        hw, z0, z1 = REAR_WIN
        rows = []
        for i in range(9):
            z = z0 - 0.03 + (z1 - z0 + 0.06) * i / 8
            p = srf.rear(z)
            S = cum(p)
            a = s_where(p, S, lambda q: q[1], hw + 0.03)
            b = s_where(p, S, lambda q: q[1], -hw - 0.03)
            rows.append([(*at_s(p, S, a + (b - a) * j / 16), z) for j in range(17)])
        g.append((grid_object('rear_glass', rows, (1, 0, 0), 0.0025), 1))
        # The signs' covers.
        g.append((quad('cover_f', (-6.0845, 0.0, 2.95), (0, -1, 0), (0, 0, 1), 1.93, 0.275), 0))
        x, y, z, w, h = SIGN_S
        g.append((quad('cover_s', (x, 1.312, z), (-1, 0, 0), (0, 0, 1), w + 0.03, h + 0.03), 0))
        x, y, z, w, h = SIGN_R
        g.append((quad('cover_r', (6.0865, 0.0, z), (0, 1, 0), (0, 0, 1), w + 0.03, h + 0.03), 0))
        return g + self.glass

    def glass_mesh(self):
        g = self.glazing()
        objs = []
        for o, tint in g:
            core.assign(o, self.M['tint'] if tint else self.M['glass'])
            objs.append(o)
        obj = core.join(objs, 'SM_ECX_Bus_Glass')
        order = [self.M['glass'].m, self.M['tint'].m]
        reorder_slots(obj, order)
        obj.data.shade_smooth()
        return obj

    def signs_mesh(self):
        x, y, z, w, h = SIGN_F
        a = quad('sign_f', (x, y, z), (0, -1, 0), (0, 0, 1), w, h)
        core.assign(a, self.M['dest'])
        x, y, z, w, h = SIGN_S
        b = quad('sign_s', (x, y, z), (-1, 0, 0), (0, 0, 1), w, h)
        core.assign(b, self.M['dest'])
        x, y, z, w, h = SIGN_R
        c = quad('sign_r', (x, y, z), (0, 1, 0), (0, 0, 1), w, h)
        core.assign(c, self.M['route'])
        obj = core.join([a, b, c], 'SM_ECX_Bus_Signs')
        reorder_slots(obj, [self.M['dest'].m, self.M['route'].m])
        return obj

    def glow_mesh(self):
        hw = W - T

        def strip(name, x0, x1, yc, zc, hw_, hh, down=True, n=8):
            """A diffuser along X: a half-round lens (hw_ wide, hh deep) under (or beside) its channel, its ends closed."""
            if down:
                sec = [(yc - hw_ * math.cos(math.pi * k / n), zc - hh * math.sin(math.pi * k / n)) for k in range(n + 1)]
            else:
                sec = [(yc + hh * math.sin(math.pi * k / n) * (1 if yc < 0 else -1), zc - hw_ * math.cos(math.pi * k / n)) for k in range(n + 1)]
            return core.prism_x(name, sec, x0, x1)
        lit = []
        for sy in (1, -1):
            # The ceiling's LED channels, in lengths between the hangers' clamps.
            xs = [-4.55, -2.95, -1.35, 0.25, 1.85, 3.45, 5.05, 5.95]
            for xa, xb in zip(xs, xs[1:]):
                lit.append(strip('ceiling_led', xa + 0.01, xb - 0.01, sy * 0.6125, 2.8525, 0.0075, 0.005))
            for xa, xb in self.rack_runs[sy]:
                lit.append(strip('stop_strip', xa + 0.05, xb - 0.05, sy * (hw - 0.002), 2.437, 0.008, 0.006, down=False))
        for x0, x1 in (DOOR_F, DOOR_R):
            for xx, sx in ((x0, 1), (x1, -1)):
                lit.append(parts.lathe_at('stepwell', [(0.0, 0.0), (0.022, 0.0), (0.02, 0.004), (0.012, 0.007), (0.0, 0.008)],
                                          Matrix.Translation((xx, W - 0.022, 0.46)) @ Matrix.Rotation(math.radians(90 * sx), 4, 'Y'), 20))
            lit.append(strip('door_header_led', x0 + 0.05, x1 - 0.05, 1.18, 2.754, 0.012, 0.006))
        obj = parts.glow_object(lit, 'SM_ECX_Bus_GlowCool')
        obj.data.materials[0].use_backface_culling = True
        return obj


def reorder_slots(obj, order):
    """Puts obj's material slots in the given order (materials, by identity), remapping the faces."""
    me = obj.data
    old = [s.material for s in obj.material_slots]
    remap = {}
    for i, m in enumerate(old):
        remap[i] = next(j for j, o in enumerate(order) if o == m)
    idx = [remap[p.material_index] for p in me.polygons]
    me.materials.clear()
    for m in order:
        me.materials.append(m)
    me.polygons.foreach_set('material_index', idx)
    me.update()


# ------------------------------------------------------------------ the door leaf and the wheels

def door_leaf(mats):
    """One leaf: a black anodized frame (a pivot shaft along the hinge edge), upper and lower glass, the rubber nose
    seal along the free edge, a stainless grab handle inside."""
    fr, gl, rb = mats
    out = []
    t = LEAF_T / 2
    W_, H = LEAF_W, LEAF_H
    seal = 0.03
    xe = W_ - seal
    out.append(put(parts.rod('shaft', (0.0, 0.0, 0.0), (0.0, 0.0, H), 0.024, 20), fr))
    for nm, x0, x1, z0, z1 in (('stile_h', 0.0, 0.07, 0.0, H), ('stile_f', xe - 0.05, xe, 0.0, H), ('rail_b', 0.0, xe, 0.0, 0.11),
                               ('rail_m', 0.0, xe, 0.96, 1.06), ('rail_t', 0.0, xe, H - 0.08, H)):
        out.append(put(parts.rbox(nm, ((x0 + x1) / 2, 0.0, (z0 + z1) / 2), (x1 - x0, LEAF_T, z1 - z0), 0.006, 2), fr))
    for z0, z1 in ((0.11, 0.96), (1.06, H - 0.08)):
        out.append(put(parts.rbox('glass', ((0.07 + xe - 0.05) / 2, 0.0, (z0 + z1) / 2), (xe - 0.05 - 0.07 + 0.02, 0.008, z1 - z0 + 0.02), 0.0), gl))
        # Rubber glazing beads, both faces.
        for sy in (1, -1):
            for (a, b, c, d) in ((0.07, xe - 0.05, z0, z0 + 0.012), (0.07, xe - 0.05, z1 - 0.012, z1), (0.07, 0.082, z0, z1), (xe - 0.062, xe - 0.05, z0, z1)):
                out.append(put(parts.rbox('bead', ((a + b) / 2, sy * 0.008, (c + d) / 2), (b - a, 0.008, d - c), 0.002), rb))
    # The nose seal: a soft bulb along the free edge.
    sec = [(xe - 0.004, -0.02), (xe + 0.012, -0.022), (W_ - 0.004, -0.012), (W_, 0.0), (W_ - 0.004, 0.012), (xe + 0.012, 0.022), (xe - 0.004, 0.02)]
    out.append(put(prism_z('nose', sec, 0.02, H - 0.02), rb))
    out.append(put(parts.rbox('sweep', (xe / 2, 0.0, -0.012), (xe - 0.02, 0.03, 0.026), 0.006), rb))
    # The grab handle inside (-Y), on two standoffs.
    for z in (1.0, 1.65):
        out.append(put(parts.rod('standoff', (0.36, -t, z), (0.36, -t - 0.05, z), 0.012, 10), fr))
    out.append(put(parts.rod('handle', (0.36, -t - 0.05, 0.97), (0.36, -t - 0.05, 1.68), 0.016, 14), fr))
    obj = parts.finish(out, 'SM_ECX_Bus_DoorLeaf', 35.0)
    reorder_slots(obj, [fr.m, gl.m, rb.m])
    return obj


def tire_profile(width, r_out=0.51, r_bead=0.288, a_bead=0.108):
    """The tire's closed (radius, axial) outline: bead, sidewall bulging to the section's width, shoulders, a tread
    with four circumferential grooves."""
    k = width / 0.31
    wall = [(r_bead, a_bead), (0.296, 0.121), (0.312, 0.136), (0.335, 0.148), (0.365, 0.1545), (0.395, 0.155), (0.425, 0.1525), (0.452, 0.147),
            (0.474, 0.139), (0.490, 0.130), (0.501, 0.119), (0.507, 0.108), (r_out - 0.0005, 0.098)]
    left = [(r, -a * k) for r, a in wall]
    tread = []
    for g in (-0.068, -0.022, 0.022, 0.068):
        tread += [(r_out, g - 0.0085), (r_out - 0.013, g - 0.0055), (r_out - 0.013, g + 0.0055), (r_out, g + 0.0085)]
    right = [(r, a * k) for r, a in reversed(wall)]
    return left + tread + right


def wheel_mesh(dual, mats):
    tire_m, rim_m, hub_m = mats
    out = []
    tw = 0.31
    centers = (-0.168, 0.168) if dual else (0.0,)
    for k, c in enumerate(centers):
        prof = [(r, a + c) for r, a in tire_profile(tw)]
        out.append(put(lathe_y('tire', prof, 96), tire_m))
        polished = (not dual) or c > 0
        # The rim's barrel and flanges.
        sgn = 1.0 if c >= 0 else -1.0
        barrel = [(0.300, c - 0.118), (0.296, c - 0.122), (0.286, c - 0.112), (0.286, c - 0.104), (0.262, c - 0.085), (0.262, c + 0.085),
                  (0.286, c + 0.104), (0.286, c + 0.112), (0.296, c + 0.122), (0.300, c + 0.118), (0.292, c + 0.112), (0.276, c + 0.104),
                  (0.252, c + 0.085), (0.252, c - 0.085), (0.276, c - 0.104), (0.292, c - 0.112)]
        out.append(put(lathe_y('barrel', barrel, 96), rim_m))
        # The disc: near the outer flange on the steer wheel; on the duals, at the hub face between the two.
        if dual:
            da = 0.004 * sgn
            disc_prof = [(0.256, da + sgn * 0.010), (0.256, da + sgn * 0.030), (0.200, da + sgn * 0.022), (0.110, da + sgn * 0.018),
                         (0.110, da), (0.200, da + sgn * 0.004)]
        else:
            disc_prof = [(0.256, 0.050), (0.256, 0.072), (0.210, 0.078), (0.150, 0.082), (0.112, 0.082), (0.112, 0.062), (0.150, 0.062),
                         (0.210, 0.058)]
        disc = lathe_y('disc', disc_prof, 96)
        holes = []
        for h in range(10):
            a = 2 * math.pi * (h + 0.5) / 10
            cx, cz = 0.198 * math.cos(a), 0.198 * math.sin(a)
            hole = parts.rod('hole', (cx, -0.4, cz), (cx, 0.4, cz), 0.034, 20)
            hole.data.transform(Matrix.Translation((cx, 0, cz)) @ Matrix.Rotation(a, 4, 'Y').inverted() @ Matrix.Diagonal((1.45, 1.0, 1.0, 1.0)) @
                                Matrix.Rotation(a, 4, 'Y') @ Matrix.Translation((-cx, 0, -cz)))
            holes.append(hole)
        core.boolean(disc, core.join(holes, 'holes'))
        out.append(put(disc, rim_m))
        if polished:
            out.append(put(parts.rod('valve', (0.27, c + 0.10 * sgn, 0.05), (0.30, c + 0.13 * sgn, 0.06), 0.005, 8), rim_m))
    # The drum behind, the hub, the studs and chrome nut covers, the cap.
    if dual:
        out.append(put(lathe_y('drum', [(0.0, -0.30), (0.215, -0.30), (0.215, -0.02), (0.0, -0.02)], 48), hub_m))
        hub = [(0.0, 0.004), (0.118, 0.004), (0.118, 0.10), (0.105, 0.115), (0.09, 0.118), (0.09, 0.145), (0.0, 0.145)]
        out.append(put(lathe_y('hub', hub, 48), hub_m))
        face = 0.034
        nut_len = 0.06
        for b in range(8):
            a = 2 * math.pi * b / 8
            out.append(put(parts.rod('cap_bolt', (0.065 * math.cos(a), 0.144, 0.065 * math.sin(a)), (0.065 * math.cos(a), 0.152, 0.065 * math.sin(a)), 0.009, 6), hub_m))
    else:
        out.append(put(lathe_y('drum', [(0.0, -0.13), (0.215, -0.13), (0.215, 0.05), (0.0, 0.05)], 48), hub_m))
        out.append(put(lathe_y('hub_face', [(0.0, 0.06), (0.112, 0.06), (0.112, 0.088), (0.0, 0.088)], 48), hub_m))
        out.append(put(lathe_y('cap', [(0.0, 0.088), (0.085, 0.088), (0.085, 0.10), (0.078, 0.12), (0.055, 0.135), (0.0, 0.14)], 40), hub_m))
        face = 0.082
        nut_len = 0.05
    for b in range(10):
        a = 2 * math.pi * b / 10
        cx, cz = 0.1429 * math.cos(a), 0.1429 * math.sin(a)
        prof = [(0.0, face), (0.0185, face), (0.0185, face + nut_len - 0.012), (0.014, face + nut_len - 0.004), (0.006, face + nut_len), (0.0, face + nut_len)]
        out.append(put(lathe_y('nut_cover', prof, 16, at=(cx, 0.0, cz)), hub_m))
    name = 'SM_ECX_Bus_WheelDual' if dual else 'SM_ECX_Bus_Wheel'
    obj = core.join(out, name)
    obj.name = obj.data.name = name
    core.finish_hard_surface(obj, 40.0)
    tire_i = next(i for i, s in enumerate(obj.material_slots) if s.material == tire_m.m)
    outer_y = 0.30 if dual else 0.12

    def outer_wall(f):
        c = f.calc_center_median()
        return f.material_index == tire_i and c.y > outer_y and f.normal.y > 0.5

    groups = [(outer_wall, 'box', None, (0.0, 0.40, 1.0, 1.0)), (lambda f: f.material_index == tire_i, 'box', None, (0.0, 0.0, 1.0, 0.395))]
    groups += [(lambda f, i=i: f.material_index == i, 'box', None, (0.0, 0.0, 1.0, 1.0)) for i in range(len(obj.material_slots))]
    core.uv_layout(obj, groups)
    reorder_slots(obj, [tire_m.m, rim_m.m, hub_m.m])
    return obj


# ------------------------------------------------------------------ assembly

BUS = {}


def skin_normals(obj):
    """The exterior decals (face attribute 'skin', with their rims: see skin_decal) take the shading normals of the
    paint under them (interpolated at the nearest point, DECAL_E away), so a decal catches the light exactly as the
    paint round it does: no box shows round it where the body curves (the rear cap, the roof's radius). A corner whose
    nearest paint doesn't lie along the decal (across its rim) keeps its own normal."""
    me = obj.data
    sk = me.attributes.get('skin')
    if sk is None:
        return 0
    flags = np.zeros(len(me.polygons), np.float32)
    sk.data.foreach_get('value', flags)
    cn = np.zeros(len(me.loops) * 3, np.float32)
    me.corner_normals.foreach_get('vector', cn)
    cn = cn.reshape(-1, 3)
    lv = np.zeros(len(me.loops), np.int32)
    me.loops.foreach_get('vertex_index', lv)
    co = np.zeros(len(me.vertices) * 3, np.float32)
    me.vertices.foreach_get('co', co)
    co = co.reshape(-1, 3)
    verts = [Vector(c) for c in co]
    paint = [p for p in me.polygons if p.material_index == 0]
    bvh = BVHTree.FromPolygons(verts, [[int(lv[li]) for li in p.loop_indices] for p in paint])
    done = 0
    for p in me.polygons:
        if flags[p.index] < 0.5:
            continue
        rim = flags[p.index] > 1.5
        for li in p.loop_indices:
            loc, fn, k, _ = bvh.find_nearest(verts[lv[li]], 0.02)
            # The decal's face lies along the paint; its rim stands across it.
            if loc is None or (abs(fn.dot(p.normal)) > 0.5 if rim else fn.dot(p.normal) < 0.8):
                continue
            q = paint[k]
            ws = poly_3d_calc([verts[lv[l]] for l in q.loop_indices], loc)
            n = Vector((0.0, 0.0, 0.0))
            for l, wgt in zip(q.loop_indices, ws):
                n += Vector(cn[l]) * wgt
            if n.length > 1e-6:
                cn[li] = n.normalized()
                done += 1
    me.normals_split_custom_set([tuple(v) for v in cn])
    return done


def finish_body(objs, M):
    obj = core.join(objs, 'SM_ECX_Bus')
    obj.name = obj.data.name = 'SM_ECX_Bus'
    reorder_slots(obj, [m.m for m in M['order']])
    core.finish_hard_surface(obj, 32.0)
    print(f'[ecx_bus] decal corners on the paint\'s normals: {skin_normals(obj)}')
    ia = obj.data.attributes.get('ipart')
    ipv = np.zeros(len(obj.data.polygons), np.float32)
    if ia is not None:
        ia.data.foreach_get('value', ipv)
    hidden_ip = set()

    def is_hidden_inner(f):
        if f.material_index != 3 or ipv[f.index] != 0.0:
            return False
        c = f.calc_center_median()
        return c.z > CEIL_Z + 0.02 or c.z < Z_FLOOR - 0.02 or c.x < -5.95 and c.z > 2.70

    groups = [(lambda f: f.material_index == 5, 'keep', None, (0.0, 0.0, 1.0, 1.0)),
              (lambda f: f.material_index == 1 and f.normal.z < -0.7 and f.calc_center_median().z < 0.45, 'box', None, (0.0, 0.0, 1.0, 0.09)),
              (lambda f: f.material_index == 1, 'box', None, (0.0, 0.095, 1.0, 1.0)),
              (is_hidden_inner, 'box', None, (0.0, 0.0, 1.0, 0.05)),
              (lambda f: f.material_index == 3, 'box', None, (0.0, 0.055, 1.0, 1.0))]
    groups += [(lambda f, i=i: f.material_index == i, 'box', None, (0.0, 0.0, 1.0, 1.0)) for i in (0, 2, 4)]
    groups.append((lambda f: True, 'box', None, (0.0, 0.0, 1.0, 1.0)))
    core.uv_layout(obj, groups)
    return obj


def build():
    core.reset()
    if not FAST:
        sign_textures()
    B = Bus()
    B.materials()
    B.shell()
    B.bumpers()
    B.rub_rails()
    B.arch_trims()
    B.liners()
    B.front_end()
    B.side_details()
    B.rear_end()
    B.roof()
    B.exterior_decals()
    B.floors()
    B.housings()
    B.walls()
    B.ceiling()
    B.ad_cards()
    B.seats()
    B.stanchions()
    body = finish_body(flatten(B.body), B.M)
    glass = B.glass_mesh()
    door = door_leaf(door_materials())
    wheel = wheel_mesh(False, (tire_material('ecx_wheel_tire', tire_sheet('ecx_tire_text'), 0.12, 0.0), rim_material('ecx_wheel_rim', False),
                               hub_material('ecx_wheel_hub')))
    dual = wheel_mesh(True, (tire_material('ecx_dual_tire', tire_sheet('ecx_dual_text'), 0.30, 0.168), rim_material('ecx_dual_rim', True),
                             hub_material('ecx_dual_hub')))
    signs = B.signs_mesh()
    glow = B.glow_mesh()
    objs = [body, glass, door, wheel, dual, signs, glow]
    for o in objs:
        print(f'[ecx_bus] {o.name}: {core.stats([o])[0]} tris, slots {[s.material.name for s in o.material_slots]}')
    BUS['objs'] = objs
    return objs


UNBAKED = ('SM_ECX_Bus_Glass', 'SM_ECX_Bus_Signs', 'SM_ECX_Bus_GlowCool')


def bake_parts(objs):
    # The glass, the signs and the glow are shaded by the game: exported unbaked, and kept out of the bakes' light.
    for o in objs:
        if o.name in UNBAKED:
            o.hide_render = True
    return [o for o in objs if o.name not in UNBAKED]


def pose_for_review(objs):
    """Puts the wheels and door leaves on the bus (the left wheels yawed 180 deg, the front doors open, the rear
    shut), shows the signs' textures and the glass as glass."""
    by = {o.name: o for o in objs}
    for o in objs:
        o.hide_render = False
        o.rotation_mode = 'XYZ'     # imported .glb objects come in with quaternions (--review-only)

    def copy(o, loc, rot_z=0.0, scale=(1, 1, 1)):
        c = o.copy()
        core.link(c)
        c.rotation_mode = 'XYZ'
        c.location = loc
        c.rotation_euler = (0.0, 0.0, math.radians(rot_z))
        c.scale = scale
        return c
    w, d = by.get('SM_ECX_Bus_Wheel'), by.get('SM_ECX_Bus_WheelDual')
    if w:
        w.location = (AXLES[0], 1.06, WZ)
        copy(w, (AXLES[0], -1.06, WZ), 180.0)
    if d:
        d.location = (AXLES[1], 0.93, WZ)
        copy(d, (AXLES[1], -0.93, WZ), 180.0)
    leaf = by.get('SM_ECX_Bus_DoorLeaf')
    if leaf:
        leaf.location = (HINGES[0][0], HINGE_Y, Z_FLOOR)
        leaf.rotation_euler = (0, 0, math.radians(90))
        copy(leaf, (HINGES[1][0], HINGE_Y, Z_FLOOR), -90.0, (-1, 1, 1))
        copy(leaf, (HINGES[2][0], HINGE_Y, Z_FLOOR))
        copy(leaf, (HINGES[3][0], HINGE_Y, Z_FLOOR), 0.0, (-1, 1, 1))
    sg = by.get('SM_ECX_Bus_Signs')
    if sg:
        for slot, tex in zip(sg.material_slots, ('T_ECX_BusSign_Up.png', 'T_ECX_BusSign_Route.png')):
            path = os.path.join(TEX_DIR, tex)
            if not os.path.exists(path):
                continue
            m = core.Mat(slot.material.name + '_review')
            img = m.image(path, extension='EXTEND')
            m.link(img.outputs['Color'], m.bsdf.inputs['Emission Color'])
            m.set('Emission Strength', 4.0)
            m.set('Base Color', (0.0, 0.0, 0.0, 1.0))
            slot.material = m.m
    gl = by.get('SM_ECX_Bus_Glass')
    if gl:
        for slot, tint in zip(gl.material_slots, (0xf2f6f8, 0x8a8f92)):
            m = core.Mat(slot.material.name + '_review')
            m.set('Base Color', core.hex_linear(tint))
            m.set('Roughness', 0.02)
            m.set('Transmission Weight', 1.0)
            m.set('IOR', 1.5)
            slot.material = m.m
    gw = by.get('SM_ECX_Bus_GlowCool')
    if gw:
        m = core.Mat('glow_review')
        m.set('Emission Color', core.hex_linear(0xe2ecff))
        m.set('Emission Strength', 3.0)
        gw.material_slots[0].material = m.m
    bpy.context.view_layer.update()
