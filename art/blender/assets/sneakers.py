"""Shoes for the street cast, fitted to a MetaHuman's feet at run time.

- SM_Sneaker_L / _R: a low-top canvas court sneaker (the player's). A vulcanized rubber sole: a gum waffle tread
  with the brand molded in, a ribbed lower band, the foxing tape with its stripe, a rubber toe cap with a ribbed bumper and
  a heel label. The canvas upper over a real last: vamp, quarters (lapped over the vamp, a double-stitched seam down
  from the front of the eyestays), eyestays with six metal eyelets a side, a padded
  tongue with its woven label, a rolled collar, a heel tape and pull tab, the dark lining and sockliner inside.
  Flat cotton laces criss-crossed and tied in a bow, aglets on the tails. Worn: creases where the toe flexes,
  scuffs on the toe cap, grime along the sole.
- SM_WorkShoe_L / _R: Benny's black leather work shoes. A derby on a slip-resistant cupsole: a stitched welt
  ledge, grooved sidewall, siped lugs; smooth leather with creased vamp and scuffed toe, quarters stitched over the
  vamp, five pairs of black eyelets, waxed round laces, a padded collar and a back stay.

Material slots, in the order the game's outfit colors use (Shoe::ESlot):
  0 upper   canvas / leather. The sneaker's bakes light neutral grey: the game tints it (multiplies the base
            color) with the outfit's shoe color. The work shoe's bakes in its own black.
  1 rubber  foxing, toe cap, sidewall (tintable, light on the sneaker).
  2 accent  the sneaker's foxing stripe, heel tape and pull tab (tintable, light grey); the work shoe's back stay.
  3 lining  the inside, collar lining, tongue underside, sockliner, the tongue's woven label (not tinted).
  4 laces   laces, aglets and the eyelets (tintable: white laces, silver eyelets; dark laces, dark eyelets).
  5 outsole the tread (not tinted).

Coordinates (meters), the foot's own frame: origin at the ankle joint (the foot bone's pivot), forward along +Y,
Z up, the left shoe's outside (the little toe's side) toward -X. The ground is GROUND below the origin, the back
of the heel (of the foot) HEEL_BACK behind it. Made for a 27 cm foot (EU 42), whose ankle-to-ball length is
ANKLE_TO_BALL: scale a shoe uniformly by (a body's ankle-to-ball length / ANKLE_TO_BALL) to fit it. The right
shoe is the left one's mirror image (its prints mapped again, so they read the right way). In Unreal (the importer
mirrors Y, meters become cm): the toe toward -Y, up +Z, the left shoe's outside toward -X, the ground at Z = -8.5.
"""
import math

import bmesh
import bpy  # noqa: F401
import numpy as np
from mathutils import Matrix, Vector

from artkit import core, street
from artkit.shade import mix_float, noise, object_coords, ramp, scaled, smooth_less, vec

MESHES = ['SM_Sneaker_L', 'SM_Sneaker_R', 'SM_WorkShoe_L', 'SM_WorkShoe_R']
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
AO_DISTANCE = 0.012
REVIEW_VIEWS = [('front', 148, 22, 2.2), ('sneaker', 118, 20, 1.25, (0.0, 0.29, -0.05)), ('work', -128, 20, 1.25, (0.0, -0.13, -0.05))]

GROUND = -0.085        # the floor, below the ankle joint
HEEL_BACK = -0.055     # the back of the foot's heel
FOOT = 0.27            # heel to toe, EU 42
ANKLE_TO_BALL = 0.142  # the ball of the foot (first metatarsal head) ahead of the ankle, level

SLOTS = ('upper', 'rubber', 'accent', 'lining', 'laces', 'outsole')
UPPER, RUBBER, ACCENT, LINING, LACES, OUTSOLE = range(6)


# ------------------------------------------------------------------ curves

class Curve1D:
    """A smooth function through (t, v) knots: cubic Hermite with finite-difference slopes, flat past the ends."""

    def __init__(self, knots):
        k = sorted(knots)
        self.t = np.array([a for a, _ in k], float)
        self.v = np.array([b for _, b in k], float)
        n = len(self.t)
        self.m = np.array([(self.v[min(i + 1, n - 1)] - self.v[max(i - 1, 0)]) / max(self.t[min(i + 1, n - 1)] - self.t[max(i - 1, 0)], 1e-9)
                           for i in range(n)])

    def __call__(self, x):
        x = np.asarray(x, float)
        t, v, m = self.t, self.v, self.m
        i = np.clip(np.searchsorted(t, x) - 1, 0, len(t) - 2)
        h = t[i + 1] - t[i]
        u = np.clip((x - t[i]) / h, 0.0, 1.0)
        u2, u3 = u * u, u * u * u
        r = (2 * u3 - 3 * u2 + 1) * v[i] + (u3 - 2 * u2 + u) * h * m[i] + (-2 * u3 + 3 * u2) * v[i + 1] + (u3 - u2) * h * m[i + 1]
        return np.where(x <= t[0], v[0], np.where(x >= t[-1], v[-1], r))


def closed_spline(ctrl, per=24):
    """Dense points on a closed centripetal Catmull-Rom spline through 2D control points."""
    P = np.asarray(ctrl, float)
    n = len(P)
    out = []
    for i in range(n):
        p0, p1, p2, p3 = P[(i - 1) % n], P[i], P[(i + 1) % n], P[(i + 2) % n]
        t0 = 0.0
        t1 = t0 + np.linalg.norm(p1 - p0) ** 0.5
        t2 = t1 + np.linalg.norm(p2 - p1) ** 0.5
        t3 = t2 + np.linalg.norm(p3 - p2) ** 0.5
        for k in range(per):
            t = t1 + (t2 - t1) * k / per
            a1 = (t1 - t) / (t1 - t0) * p0 + (t - t0) / (t1 - t0) * p1
            a2 = (t2 - t) / (t2 - t1) * p1 + (t - t1) / (t2 - t1) * p2
            a3 = (t3 - t) / (t3 - t2) * p2 + (t - t2) / (t3 - t2) * p3
            b1 = (t2 - t) / (t2 - t0) * a1 + (t - t0) / (t2 - t0) * a2
            b2 = (t3 - t) / (t3 - t1) * a2 + (t - t1) / (t3 - t1) * a3
            out.append((t2 - t) / (t2 - t1) * b1 + (t - t1) / (t2 - t1) * b2)
    return np.array(out)


def spline3(points, n):
    """n points evenly spaced along a Catmull-Rom curve through 3D control points."""
    dense = np.array(core.catmull_rom([tuple(p) for p in points], 24))
    return resample(dense, n)


def resample(poly, n):
    """n points evenly spaced by arc length along a polyline (ends kept)."""
    seg = np.linalg.norm(np.diff(poly, axis=0), axis=1)
    cum = np.concatenate([[0.0], np.cumsum(seg)])
    s = np.linspace(0.0, cum[-1], n)
    return np.stack([np.interp(s, cum, poly[:, k]) for k in range(poly.shape[1])], axis=1)


def smoothstep(e0, e1, x):
    t = np.clip((np.asarray(x, float) - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def unit(v):
    v = np.asarray(v, float)
    return v / np.maximum(np.linalg.norm(v, axis=-1, keepdims=True), 1e-12)


class Loop:
    """A closed plan outline resampled evenly: points and outward normals by arc fraction s (0 at the first
    control point, increasing counter-clockwise seen from above)."""

    def __init__(self, pts, n=1440):
        pts = np.asarray(pts, float)
        closed = np.vstack([pts, pts[:1]])
        seg = np.linalg.norm(np.diff(closed, axis=0), axis=1)
        cum = np.concatenate([[0.0], np.cumsum(seg)])
        s = np.linspace(0.0, cum[-1], n + 1)[:-1]
        self.p = np.stack([np.interp(s, cum, closed[:, k]) for k in range(2)], axis=1)
        self.length = cum[-1]
        t = np.roll(self.p, -1, 0) - np.roll(self.p, 1, 0)
        t = unit(t)
        self.nrm = np.stack([t[:, 1], -t[:, 0]], axis=1)  # outward for a counter-clockwise loop
        self.n = n

    def at(self, s):
        f = (np.asarray(s, float) % 1.0) * self.n
        i = np.floor(f).astype(int) % self.n
        w = (f - np.floor(f))[:, None]
        j = (i + 1) % self.n
        p = self.p[i] * (1 - w) + self.p[j] * w
        nr = unit(self.nrm[i] * (1 - w) + self.nrm[j] * w)
        return p, nr


def _g(d, p):
    """The last's section: 0 at its edge rising vertically, 1 with zero slope at its ridge (a superellipse quadrant)."""
    d = np.clip(d, 0.0, 1.0)
    return np.power(np.maximum(1.0 - np.power(1.0 - d, p), 0.0), 1.0 / p)


# ------------------------------------------------------------------ styles

SNEAKER_SOLE = [
    # (out from the outline, height, height from the foxing's top?, the band's slot up to the next ring)
    (-0.0035, 0.0, 0, OUTSOLE), (-0.0014, 0.0003, 0, OUTSOLE), (-0.0004, 0.0013, 0, OUTSOLE),   # the tread's rounded edge
    (0.0, 0.0030, 0, RUBBER), (0.0004, 0.0050, 0, RUBBER), (0.0006, 0.0085, 0, RUBBER),          # the ribbed band
    (0.0001, 0.0093, 0, RUBBER), (-0.0001, 0.0100, 0, RUBBER),                                     # a step in
    (-0.0001, 0.0150, 0, RUBBER), (0.0002, 0.0153, 0, ACCENT), (0.0002, 0.0180, 0, RUBBER),       # the stripe
    (-0.0001, 0.0183, 0, RUBBER), (-0.0003, -0.0018, 1, RUBBER), (-0.0008, -0.0004, 1, RUBBER),   # the foxing tape
    (-0.0016, 0.0, 1, RUBBER), (-0.0026, -0.0008, 1, RUBBER)]                                       # its lip over the canvas

SNEAKER = dict(
    name='SM_Sneaker',
    # The sole's outline at its sidewall, counter-clockwise from the back of the heel, the inside of the left foot
    # toward +X. A 29.5 cm outsole for a 27 cm foot: 10.5 cm across the ball, 7.6 across the heel.
    outline=[(0.000, -0.0655), (0.0185, -0.0625), (0.0310, -0.0520), (0.0365, -0.0340), (0.0370, -0.0120), (0.0352, 0.0150),
             (0.0345, 0.0450), (0.0395, 0.0800), (0.0480, 0.1150), (0.0520, 0.1400), (0.0515, 0.1650), (0.0465, 0.1900),
             (0.0360, 0.2120), (0.0180, 0.2265), (0.0020, 0.2295), (-0.0160, 0.2265), (-0.0320, 0.2140), (-0.0440, 0.1930),
             (-0.0510, 0.1650), (-0.0535, 0.1350), (-0.0510, 0.1000), (-0.0450, 0.0600), (-0.0400, 0.0200), (-0.0385, -0.0120),
             (-0.0375, -0.0340), (-0.0325, -0.0520), (-0.0200, -0.0625)],
    inset=0.002,       # the upper's base inside the sole's outline: the foxing tape laps over it
    base=0.019,        # where the upper starts, above the ground (hidden behind the foxing)
    # The last's ridge across its width, a little toward the big toe; its height above the ground along it.
    center=[(-0.07, 0.0), (0.0, 0.002), (0.05, 0.004), (0.12, 0.006), (0.2, 0.005), (0.235, 0.002)],
    crown=[(-0.07, 0.088), (-0.02, 0.089), (0.012, 0.092), (0.034, 0.0925), (0.060, 0.0865), (0.100, 0.0755), (0.130, 0.0665),
           (0.160, 0.0585), (0.190, 0.0510), (0.215, 0.0460), (0.235, 0.0430)],
    expo=[(-0.07, 2.15), (-0.01, 2.3), (0.035, 3.1), (0.07, 3.0), (0.12, 2.6), (0.2, 2.35)],
    r_toe=0.036, p_toe=2.2, r_heel=0.030, p_heel=2.6,
    # The collar's top line above the ground, from the heel round to where it meets the eyestays.
    collar=[(-0.075, 0.0725), (-0.050, 0.0710), (-0.028, 0.0662), (-0.004, 0.0615), (0.008, 0.0640), (0.018, 0.0690), (0.027, 0.0760)],
    corner_y=0.036,                                         # the collar meets the eyestay at the top eyelet
    throat=[(0.036, 0.0195), (0.070, 0.0150), (0.104, 0.0105)],  # half the gap between the eyestays
    throat_y=0.104,                                         # the round front of the gap
    eyelets=6, eyelet_y=(0.0995, 0.0435), eyelet_in=0.0062, eyelet_r=(0.0017, 0.0034),
    lace=(0.0068, 0.0016),
    sole=SNEAKER_SOLE,
    foxing=[(-0.07, 0.0285), (-0.04, 0.0270), (0.0, 0.0248), (0.10, 0.0243), (0.17, 0.0250), (0.235, 0.0265)],
    toe_spring=0.012, spring_from=0.150,
    tongue_top=(0.024, 0.1070),
    columns=104, rows=32,
    # Overlays and trims.
    toe_cap=((0.002, 0.236, GROUND), (0.066, 0.0645, 0.075)),   # the rubber cap: the canvas inside this ellipsoid
    heel_tape=(ACCENT, 1.0, 0.0008), pull_tab=True, heel_label=True,
    # Where the vamp creases: (y, depth, width, how far the arc bends back at the sides, half its length, off center).
    creases=[(0.121, 0.6, 0.0034, 3.0, 0.022, 0.004), (0.131, 1.0, 0.0040, 3.5, 0.031, -0.002), (0.1415, 0.75, 0.0042, 4.0, 0.025, 0.007),
             (0.152, 0.45, 0.0038, 4.5, 0.019, -0.006), (0.160, 0.3, 0.0035, 5.0, 0.014, 0.010)],
    crease_depth=0.0009,
    # Materials.
    tag='snk', brand='STOOP', leather=False,
    stitch_eye=(0.0026, 0.0122), stitch_collar=(0.0030, 0.0058), thread=0.0007, counter=(0.060, 0.28),
    # The quarters lapped over the vamp, double-stitched from the front of the eyestays down and back to the foxing.
    derby=(0.107, 0.070, 0.42),
    lining=(0x1f1f22, 0x29292d), lace_color=0xebe9e3, lace_rough=0.86, eyelet_color=0xbfc2c6, eyelet_metal=1.0, eyelet_rough=0.3,
    outsole=0x4a3b30, tread=('waffle', 0.0058, 0.0065),
)

WORK_SOLE = [
    (-0.0040, 0.0, 0, OUTSOLE), (-0.0016, 0.0004, 0, OUTSOLE), (-0.0004, 0.0018, 0, OUTSOLE),     # the tread's edge
    (0.0, 0.0040, 0, RUBBER), (0.0002, 0.0105, 0, RUBBER), (0.0003, 0.0165, 0, RUBBER),           # the cupsole's wall
    (0.0004, -0.0068, 1, RUBBER), (0.0011, -0.0054, 1, RUBBER), (0.0012, -0.0012, 1, RUBBER),     # the welt's bead
    (0.0007, 0.0, 1, RUBBER), (-0.0010, 0.0003, 1, RUBBER), (-0.0035, 0.0002, 1, RUBBER),         # the ledge round the upper
    (-0.0047, -0.0006, 1, RUBBER)]

WORK = dict(
    name='SM_WorkShoe',
    # A plain-toe derby on a cupsole: the sole a welt's width (4.5 mm) bigger than the upper all round. The toe
    # rounded off toward the big toe (its point a little inside the middle), not a broad clog's.
    outline=[(0.000, -0.0675), (0.0200, -0.0645), (0.0330, -0.0540), (0.0385, -0.0350), (0.0390, -0.0120), (0.0372, 0.0150),
             (0.0365, 0.0450), (0.0415, 0.0800), (0.0500, 0.1150), (0.0543, 0.1400), (0.0535, 0.1650), (0.0488, 0.1890),
             (0.0400, 0.2095), (0.0262, 0.2262), (0.0085, 0.2342), (-0.0095, 0.2330), (-0.0265, 0.2245), (-0.0405, 0.2065),
             (-0.0505, 0.1800), (-0.0555, 0.1500), (-0.0537, 0.1000), (-0.0477, 0.0600), (-0.0427, 0.0200), (-0.0407, -0.0120),
             (-0.0397, -0.0350), (-0.0347, -0.0540), (-0.0217, -0.0645)],
    inset=0.0047, base=0.0215,
    center=[(-0.07, 0.0), (0.0, 0.002), (0.05, 0.004), (0.12, 0.005), (0.2, 0.005), (0.24, 0.004)],
    # The toe kept low (4.8 cm at its front, sole included) and sloping down to the welt: a work oxford, not a clog.
    crown=[(-0.07, 0.093), (-0.02, 0.094), (0.012, 0.097), (0.034, 0.098), (0.060, 0.0915), (0.100, 0.0805), (0.130, 0.0715),
           (0.160, 0.0635), (0.190, 0.0565), (0.215, 0.0515), (0.240, 0.0480)],
    expo=[(-0.07, 2.2), (-0.01, 2.3), (0.035, 3.0), (0.07, 2.9), (0.12, 2.6), (0.2, 2.3)],
    r_toe=0.036, p_toe=2.3, r_heel=0.030, p_heel=2.6,
    collar=[(-0.075, 0.0795), (-0.050, 0.0780), (-0.028, 0.0735), (-0.004, 0.0695), (0.010, 0.0712), (0.022, 0.0765), (0.031, 0.0830)],
    corner_y=0.040,
    throat=[(0.040, 0.0105), (0.070, 0.0085), (0.098, 0.0068)],
    throat_y=0.098,
    eyelets=5, eyelet_y=(0.0925, 0.0470), eyelet_in=0.0072, eyelet_r=(0.0013, 0.0026),
    lace=(0.0029, 0.0029), round_laces=True,
    sole=WORK_SOLE,
    # The cupsole's top: 3.4 cm at the heel, down to 2.6 under the forefoot.
    foxing=[(-0.07, 0.0340), (-0.03, 0.0332), (0.02, 0.0306), (0.08, 0.0272), (0.13, 0.0262), (0.235, 0.0260)],
    toe_spring=0.014, spring_from=0.145,
    tongue_top=(0.026, 0.1095), bow=0.85,
    columns=104, rows=32,
    toe_cap=None,
    heel_tape=(ACCENT, 3.0, 0.0011), pull_tab=False, heel_label=False,
    creases=[(0.123, 0.75, 0.0038, 2.5, 0.026, 0.003), (0.1355, 1.0, 0.0046, 3.0, 0.033, -0.002), (0.148, 0.6, 0.0042, 3.5, 0.024, 0.006),
             (0.157, 0.3, 0.0035, 4.0, 0.016, -0.008)],
    crease_depth=0.0009,
    tag='work', brand='GRIPWELL', leather=True,
    stitch_eye=(0.0024, 0.0042), stitch_collar=(0.0028, 0.0050), thread=0.0006, counter=(0.066, 0.20),
    derby=(0.101, 0.075, 0.70),
    lining=(0x2a2724, 0x34302c), lace_color=0x151517, lace_rough=0.42, eyelet_color=0x2b2b2e, eyelet_metal=0.7, eyelet_rough=0.35,
    outsole=0x18181a, tread=('lugs', 0.0068, 0.0055),
)

# Texture size per baked material (the rest TEXTURE_SIZE): the sneaker's canvas and lining (its label) the largest.
TEXTURE_SIZES = {}
for _side in ('l', 'r'):
    TEXTURE_SIZES.update({f'snk_upper_{_side}': 1024, f'snk_rubber_{_side}': 1024, f'snk_accent_{_side}': 512, f'snk_lining_{_side}': 1024,
                          f'snk_laces_{_side}': 512, f'snk_outsole_{_side}': 512,
                          f'work_upper_{_side}': 1024, f'work_rubber_{_side}': 512, f'work_accent_{_side}': 128, f'work_lining_{_side}': 512,
                          f'work_laces_{_side}': 512, f'work_outsole_{_side}': 512})


# ------------------------------------------------------------------ the last

class Last:
    """The shape the upper is made over: a height field over the sole's plan. Across, a superellipse section rising
    from the edge to a ridge; along, the crown height; the toe and heel rounded in side view."""

    def __init__(self, st):
        self.st = st
        self.sole = Loop(closed_spline(st['outline']))
        s = np.linspace(0.0, 1.0, 961)[:-1]
        P, N = self.sole.at(s)
        self.poly = P - N * st['inset']
        self.base = GROUND + st['base']
        self.y0 = float(self.poly[:, 1].min())
        self.y1 = float(self.poly[:, 1].max())
        self.ys = np.linspace(self.y0, self.y1, 1201)
        self.xmin, self.xmax = self._span(self.ys)
        self.xc = Curve1D(st['center'])
        self.crown = Curve1D([(y, GROUND + z) for y, z in st['crown']])
        self.expo = Curve1D(st['expo'])

    def _span(self, ys):
        A = self.poly
        B = np.roll(A, -1, 0)
        Y = ys[:, None]
        ay, by = A[None, :, 1], B[None, :, 1]
        hit = ((ay - Y) * (by - Y) <= 0.0) & (np.abs(by - ay) > 1e-12)
        x = A[None, :, 0] + (Y - ay) * (B[None, :, 0] - A[None, :, 0]) / np.where(np.abs(by - ay) > 1e-12, by - ay, 1.0)
        xmin = np.where(hit, x, np.inf).min(axis=1)
        xmax = np.where(hit, x, -np.inf).max(axis=1)
        xmin[~np.isfinite(xmin)] = 0.0
        xmax[~np.isfinite(xmax)] = 0.0
        return xmin, xmax

    def height(self, P):
        P = np.asarray(P, float)
        x, y = P[..., 0], P[..., 1]
        st = self.st
        xmin = np.interp(y, self.ys, self.xmin)
        xmax = np.interp(y, self.ys, self.xmax)
        xc = np.clip(self.xc(y), xmin + 1e-5, xmax - 1e-5)
        w = np.where(x >= xc, xmax - xc, xc - xmin)
        dx = 1.0 - np.abs(x - xc) / np.maximum(w, 1e-6)
        dt = (self.y1 - y) / st['r_toe']
        dh = (y - self.y0) / st['r_heel']
        H = (self.crown(y) - self.base) * _g(dx, self.expo(y)) * _g(dt, st['p_toe']) * _g(dh, st['p_heel'])
        H = np.where((y > self.y0) & (y < self.y1) & (dx > 0.0), H, 0.0)
        return self.base + H

    def point(self, x, y, lift=0.0):
        """The surface over plan point(s), lifted along its normal."""
        P = np.stack(np.broadcast_arrays(np.asarray(x, float), np.asarray(y, float)), axis=-1)
        z = self.height(P)
        p = np.concatenate([P, z[..., None]], axis=-1)
        return p + self.normal(P) * np.asarray(lift, float)[..., None] if np.any(lift) else p

    def normal(self, P, eps=2e-4):
        P = np.asarray(P, float)
        ex = np.array([eps, 0.0])
        ey = np.array([0.0, eps])
        hx = (self.height(P + ex) - self.height(P - ex)) / (2 * eps)
        hy = (self.height(P + ey) - self.height(P - ey)) / (2 * eps)
        return unit(np.stack([-hx, -hy, np.ones_like(hx)], axis=-1))

    def side_point(self, y, z, side, lift=0.0):
        """The point on the side wall (side +1 toward +X) at height z over station y."""
        lo = self.xc(np.asarray(y, float))
        hi = np.where(side > 0, np.interp(y, self.ys, self.xmax), np.interp(y, self.ys, self.xmin))
        for _ in range(40):
            mid = (lo + hi) / 2
            above = self.height(np.stack([mid, np.broadcast_to(y, np.shape(mid))], axis=-1)) > z
            lo = np.where(above, mid, lo)
            hi = np.where(above, hi, mid)
        x = (lo + hi) / 2
        return self.point(x, y, lift)


# ------------------------------------------------------------------ mesh parts

class Part:
    """Geometry for one piece of the shoe: vertices, faces and each face's material slot, plus what the UV
    layout needs (how to unwrap it, which vertices lie on seams) and what the materials read (the panel UV:
    meters along the top line and down from it; a part id)."""

    def __init__(self, name, verts, faces, slots, mode='angle', seam=None, panel=None, pid=0.0, **attrs):
        self.name = name
        self.verts = np.asarray(verts, float).reshape(-1, 3)
        self.faces = [tuple(int(i) for i in f) for f in faces]
        self.slots = list(slots) if np.ndim(slots) else [int(slots)] * len(self.faces)
        self.mode = mode
        nv = len(self.verts)
        self.seam = np.zeros(nv, bool) if seam is None else np.asarray(seam, bool)
        self.panel = np.tile([0.0, 1.0], (nv, 1)) if panel is None else np.asarray(panel, float)
        self.attrs = {'pid': pid}
        self.attrs.update(attrs)
        for k, d in ATTR_DEFAULTS.items():
            v = self.attrs.get(k, d)
            self.attrs[k] = np.full(nv, float(v)) if np.ndim(v) == 0 else np.asarray(v, float).ravel()

    @property
    def pid(self):
        return self.attrs['pid']


# Per-vertex values the materials read: a part's id within its slot; the distance along the upper from the back
# of the heel (or, on the tread, in from its edge); how much of the collar a point of the upper is (1) rather than
# the lacing (0). And for the UV layout: how many times the usual texel density a part gets (its print stays crisp).
ATTR_DEFAULTS = {'pid': 0.0, 'hu': 1.0, 'zone': 0.0, 'uvw': 1.0}


def sweep_panel(path, m):
    """Panel coordinates for a swept profile: meters along the path, the fraction round the profile."""
    seg = np.linalg.norm(np.diff(np.asarray(path), axis=0), axis=1)
    arc = np.concatenate([[0.0], np.cumsum(seg)])
    return np.stack([np.repeat(arc, m), np.tile(np.arange(m) / m, len(arc))], axis=1)

    def flip(self):
        self.faces = [tuple(reversed(f)) for f in self.faces]
        return self


def grid_faces(nu, nv, closed_u=False, offset=0, flip=False):
    """Quads over a (nu, nv) vertex grid indexed u * nv + v."""
    faces = []
    for j in range(nu if closed_u else nu - 1):
        j1 = (j + 1) % nu
        for i in range(nv - 1):
            f = (offset + j * nv + i, offset + j1 * nv + i, offset + j1 * nv + i + 1, offset + j * nv + i + 1)
            faces.append(tuple(reversed(f)) if flip else f)
    return faces


def grid_normals(G, closed_u=True):
    """Outward normals of a (u, v) grid of points whose u runs counter-clockwise round and v up."""
    if closed_u:
        Tu = np.roll(G, -1, 0) - np.roll(G, 1, 0)
    else:
        Tu = np.gradient(G, axis=0)
    Tv = np.gradient(G, axis=1)
    return unit(np.cross(Tu, Tv))


def sweep(path, ups, profile, closed=False, cap=False, profile_closed=True):
    """Vertices and faces of a profile swept along a 3D path. Each section is placed in the frame (A, B): A the
    path's up (made perpendicular to the path), B = T x A; profile points are (a, b) or one list per path point."""
    path = np.asarray(path, float)
    n = len(path)
    T = np.roll(path, -1, 0) - np.roll(path, 1, 0) if closed else np.gradient(path, axis=0)
    T = unit(T)
    A = np.asarray(ups, float)
    A = unit(A - T * np.sum(A * T, axis=1, keepdims=True))
    B = np.cross(T, A)
    prof = np.asarray(profile, float)
    if prof.ndim == 2:
        prof = np.broadcast_to(prof, (n,) + prof.shape)
    m = prof.shape[1]
    V = path[:, None, :] + prof[:, :, 0:1] * A[:, None, :] + prof[:, :, 1:2] * B[:, None, :]
    faces = []
    for i in range(n if closed else n - 1):
        i1 = (i + 1) % n
        for k in range(m if profile_closed else m - 1):
            k1 = (k + 1) % m
            faces.append((i * m + k, i1 * m + k, i1 * m + k1, i * m + k1))
    verts = V.reshape(-1, 3)
    if cap and not closed:
        c0 = len(verts)
        verts = np.vstack([verts, path[0], path[-1]])
        for k in range(m):
            k1 = (k + 1) % m
            faces.append((c0, k, k1))
            faces.append((c0 + 1, (n - 1) * m + k1, (n - 1) * m + k))
    seam = np.zeros(len(verts), bool)
    seam[np.arange(n) * m] = True
    if closed:
        seam[:m] = True
    return verts, faces, seam


def oriented(verts, faces, outside):
    """Faces turned so their normals agree, on the whole, with outside(face centers) -> directions."""
    V = np.asarray(verts)
    score = 0.0
    for f in faces[:: max(1, len(faces) // 400)]:
        p = V[list(f)]
        nrm = np.cross(p[1] - p[0], p[2] - p[0])
        score += float(np.dot(nrm, outside(p.mean(axis=0))))
    return faces if score >= 0 else [tuple(reversed(f)) for f in faces]


def disc_fill(ring, center, rings=(0.9, 0.76, 0.6, 0.42, 0.24), z_of=None):
    """Concentric rings from a closed outline in toward center, and a fan at the middle."""
    ring = np.asarray(ring, float)
    n = len(ring)
    c = np.asarray(center, float)
    verts = [ring]
    for f in rings:
        verts.append(c + (ring - c) * f)
    V = np.concatenate(verts)
    if z_of is not None:
        V[:, 2] = z_of(V)
    faces = []
    for k in range(len(rings)):
        a, b = k * n, (k + 1) * n
        for j in range(n):
            j1 = (j + 1) % n
            faces.append((a + j, b + j, b + j1, a + j1))
    ci = len(V)
    V = np.vstack([V, c if z_of is None else np.array([c[0], c[1], z_of(c[None])[0]])])
    last = len(rings) * n
    for j in range(n):
        faces.append((last + j, ci, last + (j + 1) % n))
    return V, faces


# ------------------------------------------------------------------ the upper

class Upper:
    """The upper's outer surface as a grid: columns round the last's outline (from the back of the heel,
    counter-clockwise), rows from the base up to the top line. Behind the heel and along the sides a column climbs
    the wall to the collar's height; under the lacing it climbs to the eyestay's edge; round the forefoot it climbs
    over the toe box to the round front of the gap between the eyestays."""

    def __init__(self, last):
        st = last.st
        self.last = last
        n = st['columns']
        tape = 0.0085 / last.sole.length
        mid = np.linspace(tape, 1.0 - tape, n - 3)
        self.s = np.concatenate([[0.0, tape / 2], mid, [1.0 - tape / 2]])
        self.tape = tape
        O, N = last.sole.at(self.s)
        L = O - N * st['inset']
        self.L, self.N2 = L, N
        n = len(self.s)
        self.n = n
        xc = last.xc
        e = Curve1D(st['throat'])
        self.e = e
        yc, yu = st['corner_y'], st['throat_y']
        eu, xu = float(e(yu)), float(xc(yu))
        kind = np.where(L[:, 1] > yu, 2, np.where(L[:, 1] >= yc, 1, 0))
        toe = np.nonzero(kind == 2)[0]
        K = 480
        t = (np.arange(K + 1) / K) ** 2
        paths = np.empty((n, K + 1, 2))
        for j in range(n):
            if kind[j] == 0:
                paths[j] = L[j] - N[j] * (0.05 * t)[:, None]
                continue
            if kind[j] == 1:
                y = L[j, 1]
                c = float(xc(y))
                q = np.array([c + (1.0 if L[j, 0] > c else -1.0) * float(e(y)), y])
            else:
                k = int(np.searchsorted(toe, j))
                phi = math.pi * (k + 1) / (len(toe) + 1)
                q = np.array([xu + eu * math.cos(phi), yu + eu * math.sin(phi)])
            paths[j] = L[j] + (q - L[j]) * t[:, None]
        Z = last.height(paths.reshape(-1, 2)).reshape(n, K + 1)
        collar = {}
        for side in (1.0, -1.0):
            c = float(xc(yc))
            zc = float(last.height(np.array([[c + side * float(e(yc)), yc]]))[0])
            collar[side] = Curve1D([(y, GROUND + z) for y, z in st['collar']] + [(yc, zc)])
        rows = st['rows'] + 1
        G = np.empty((n, rows, 3))
        lengths = np.empty(n)
        for j in range(n):
            P3 = np.concatenate([paths[j], Z[j][:, None]], axis=1)
            if kind[j] == 0:
                side = 1.0 if L[j, 0] >= float(xc(L[j, 1])) else -1.0
                zt = collar[side](paths[j][:, 1])
                above = np.nonzero(Z[j] >= zt)[0]
                k = int(above[0]) if len(above) else K
                k = max(k, 1)
                d0, d1 = zt[k - 1] - Z[j][k - 1], zt[k] - Z[j][k]
                f = float(np.clip(d0 / max(d0 - d1, 1e-12), 0.0, 1.0))
                end = P3[k - 1] + (P3[k] - P3[k - 1]) * f
                P3 = np.vstack([P3[:k], end])
            G[j] = resample(P3, rows)
        # Round the top eyelet's corner, where the collar's rising line turns down the eyestay: smooth the top rows
        # along the loop there.
        corner = np.exp(-((L[:, 1] - yc) / 0.010) ** 2)
        for r in range(rows - 6, rows):
            wr = ((r - (rows - 7)) / 6.0) ** 1.5
            for _ in range(6):
                avg = (np.roll(G[:, r], 1, 0) + 2 * G[:, r] + np.roll(G[:, r], -1, 0)) / 4
                G[:, r] += (avg - G[:, r]) * (corner * wr)[:, None]
        for j in range(n):
            lengths[j] = np.sum(np.linalg.norm(np.diff(G[j], axis=0), axis=1))
        self.G = G
        self.kind = kind
        self.lengths = lengths
        self.top = G[:, -1]
        self.normals = grid_normals(G)

    def flex(self, P, nrm):
        """Creases across the vamp where the toe bends: inward along the normal, deepest on top."""
        st = self.last.st
        x = P[:, 0] - 0.005
        y = P[:, 1]
        top = smoothstep(0.2, 0.75, nrm[:, 2])
        d = np.zeros(len(P))
        for i, (y0, depth, width, bend, half, xo) in enumerate(st['creases']):
            yw = y0 + 0.12 * x - bend * x * x + 0.0016 * np.sin(x * 210.0 + 1.3 * i)
            d += depth * st['crease_depth'] * np.exp(-((y - yw) / width) ** 2) * np.exp(-((x - xo) / half) ** 2)
        return P - nrm * (d * top)[:, None]

    def part(self):
        """The outer surface (slot 0). Its first column is repeated at the end, so the panel UV can run from 0
        round to the top line's whole length (the copy welds back)."""
        G = self.G
        n, rows, _ = G.shape
        Gc = np.concatenate([G, G[:1]], axis=0)
        Nc = np.concatenate([self.normals, self.normals[:1]], axis=0)
        flat = Gc.reshape(-1, 3)
        V = self.flex(flat, Nc.reshape(-1, 3))
        # Panel UV: U along each row from the back of the heel, V down from the top line (meters).
        seg_v = np.linalg.norm(np.diff(Gc, axis=1), axis=2)
        down = np.concatenate([np.cumsum(seg_v[:, ::-1], axis=1)[:, ::-1], np.zeros((n + 1, 1))], axis=1)
        seg_u = np.linalg.norm(np.diff(Gc, axis=0), axis=2)
        along = np.concatenate([np.zeros((1, rows)), np.cumsum(seg_u, axis=0)], axis=0)
        panel = np.stack([along, down], axis=2).reshape(-1, 2)
        hu = np.minimum(along, along[-1:] - along)
        zone = np.repeat(np.append(self.collar_weight(), self.collar_weight()[0]), rows)
        faces = grid_faces(n + 1, rows)
        seam = np.zeros((n + 1, rows), bool)
        seam[0] = seam[n] = True
        tip = int(np.argmax(self.L[:, 1]))
        seam[tip] = True
        self.top_length = float(along[-1, -1])
        return Part('upper', V, faces, UPPER, 'angle', seam.ravel(), panel, hu=hu.ravel(), zone=zone)

    def lining(self, thick=0.0025):
        """The inside: the outer surface moved in by the canvas's thickness, at half the resolution."""
        G = self.G[::2, ::2] - self.normals[::2, ::2] * thick
        top = (self.G[::2, -1] - self.normals[::2, -1] * thick)[:, None]
        if (self.G.shape[1] - 1) % 2:
            G = np.concatenate([G, top], axis=1)
        nu, nv = G.shape[:2]
        seam = np.zeros((nu, nv), bool)
        seam[0] = True
        seam[int(np.argmax(self.L[::2, 1]))] = True
        return Part('lining', G.reshape(-1, 3), grid_faces(nu, nv, closed_u=True, flip=True), LINING, 'angle', seam.ravel())

    def collar_weight(self):
        y = self.top[:, 1]
        yc = self.last.st['corner_y']
        return 1.0 - smoothstep(yc - 0.030, yc - 0.008, y)

    def bead(self):
        """The rolled edge round the opening: padded behind the heel and along the collar, a plain folded edge
        along the eyestays and the throat."""
        top = self.top
        nrm = self.normals[:, -1]
        up = unit(self.G[:, -1] - self.G[:, -2])
        up = unit(up - nrm * np.sum(up * nrm, axis=1, keepdims=True))
        throat = np.array([(0.0001, -0.0045), (0.0002, -0.0025), (0.0002, -0.0008), (0.0, 0.0004), (-0.0006, 0.0010), (-0.0014, 0.0012),
                           (-0.0022, 0.0007), (-0.0027, -0.0004), (-0.0029, -0.0022), (-0.0029, -0.0045)])
        collar = np.array([(0.0002, -0.0100), (0.0010, -0.0062), (0.0015, -0.0030), (0.0012, 0.0004), (0.0002, 0.0028), (-0.0020, 0.0039),
                           (-0.0042, 0.0030), (-0.0058, 0.0004), (-0.0060, -0.0060), (-0.0032, -0.0150)])
        zone = self.collar_weight()
        w = zone[:, None, None]
        prof = throat[None] * (1 - w) + collar[None] * w
        # Profile points (a along the normal, b up the wall); the first column repeated at the end (see part()).
        n = len(top)
        m = prof.shape[1]
        V = top[:, None, :] + prof[:, :, 0:1] * nrm[:, None, :] + prof[:, :, 1:2] * up[:, None, :]
        V = np.concatenate([V, V[:1]], axis=0)
        faces, slots = [], []
        for j in range(n):
            for k in range(m - 1):
                faces.append((j * m + k, (j + 1) * m + k, (j + 1) * m + k + 1, j * m + k + 1))
                slots.append(UPPER if k < 6 else LINING)
        seam = np.zeros((n + 1, m), bool)
        seam[0] = seam[n] = True
        # Panel: U along the top line, V down from it on the outside (the collar's stitching runs on the roll).
        seg = np.linalg.norm(np.diff(np.vstack([top, top[:1]]), axis=0), axis=1)
        along = np.concatenate([[0.0], np.cumsum(seg)])
        down = np.maximum(-np.concatenate([prof, prof[:1]], axis=0)[:, :, 1], 0.0)
        down[:, 6:] = 1.0
        panel = np.stack([np.repeat(along[:, None], m, axis=1), down], axis=2).reshape(-1, 2)
        hu = np.repeat(np.minimum(along, along[-1] - along)[:, None], m, axis=1)
        zn = np.repeat(np.append(zone, zone[0])[:, None], m, axis=1)
        return Part('bead', V.reshape(-1, 3), faces, slots, 'angle', seam.ravel(), panel, hu=hu.ravel(), zone=zn.ravel())

    def toe_cap(self, ellipsoid, lift=0.0012):
        """The rubber toe cap laid over the canvas: the part of each forefoot column inside the ellipsoid."""
        c, ax = np.asarray(ellipsoid[0]), np.asarray(ellipsoid[1])
        G = self.G
        cols, edges = [], []
        for j in range(self.n):
            g = G[j]
            E = np.sum(((g - c) / ax) ** 2, axis=1)
            seg = np.linalg.norm(np.diff(g, axis=0), axis=1)
            arc = np.concatenate([[0.0], np.cumsum(seg)])
            out = np.nonzero(E >= 1.0)[0]
            if not len(out) or out[0] == 0:
                continue
            k = int(out[0])
            f = (1.0 - E[k - 1]) / max(E[k] - E[k - 1], 1e-9)
            edge = arc[k - 1] + (arc[k] - arc[k - 1]) * f
            if edge < 0.008:
                continue
            cols.append(j)
            edges.append(edge)
        if not cols:
            return None
        rows = 12
        nrows = rows + 2
        V = []
        for j, edge in zip(cols, edges):
            g = G[j]
            nr = self.normals[j]
            seg = np.linalg.norm(np.diff(g, axis=0), axis=1)
            arc = np.concatenate([[0.0], np.cumsum(seg)])
            at = list(np.linspace(0.0, edge, rows + 1)) + [edge + 0.0005]
            offs = [lift] * (rows + 1) + [0.0002]
            for a, o in zip(at, offs):
                p = np.array([np.interp(a, arc, g[:, k]) for k in range(3)])
                q = unit(np.array([np.interp(a, arc, nr[:, k]) for k in range(3)]))
                V.append(p + q * o)
        # Its two ends closed down onto the canvas.
        nc = len(cols)
        for j in (cols[0], cols[-1]):
            g = G[j]
            nr = self.normals[j]
            seg = np.linalg.norm(np.diff(g, axis=0), axis=1)
            arc = np.concatenate([[0.0], np.cumsum(seg)])
            edge = edges[cols.index(j)]
            for a in list(np.linspace(0.0, edge, rows + 1)) + [edge + 0.0005]:
                pt = np.array([np.interp(a, arc, g[:, k]) for k in range(3)])
                q = unit(np.array([np.interp(a, arc, nr[:, k]) for k in range(3)]))
                V.append(pt + q * 0.0001)
        V = np.array(V)
        faces = grid_faces(nc, nrows)
        e0, e1 = nc * nrows, nc * nrows + nrows
        for i in range(nrows - 1):
            faces.append((e0 + i, i, i + 1, e0 + i + 1))
            last = (nc - 1) * nrows
            faces.append((last + i, e1 + i, e1 + i + 1, last + i + 1))
        faces = faces[:len(grid_faces(nc, nrows))] + oriented(V, faces[len(grid_faces(nc, nrows)):],
                                                                lambda q: q - V[:nc * nrows].mean(axis=0))
        return Part('toe_cap', V, faces, RUBBER, 'angle', pid=1.0)

    def heel_tape(self, lift=0.0008, z_from=None, slot=ACCENT, pid=1.0):
        """The tape up the back of the heel (accent): the columns within the tape's width, from behind the foxing
        to under the collar's roll."""
        idx = [self.n - 2, self.n - 1, 0, 1, 2]  # the tape's edges, halfway to them, the middle
        G = self.G[idx]
        nr = self.normals[idx]
        z0 = GROUND + (z_from if z_from is not None else self.last.st['base'] + 0.003)
        rows = []
        for g, q in zip(G, nr):
            seg = np.linalg.norm(np.diff(g, axis=0), axis=1)
            arc = np.concatenate([[0.0], np.cumsum(seg)])
            a0 = float(np.interp(z0, g[:, 2], arc))
            a1 = arc[-1] - 0.0095
            at = np.linspace(a0, a1, 16)
            rows.append([(np.array([np.interp(a, arc, g[:, k]) for k in range(3)]), unit(np.array([np.interp(a, arc, q[:, k]) for k in range(3)])))
                         for a in at] + [None])
        nv = 17
        V = []
        # Columns: the outer edge dropped to the canvas, the tape's face, the other edge dropped; the top row (the
        # extra one) dropped too, closing the tape's end.
        layout = [(0, 0.0001)] + [(i, lift) for i in range(5)] + [(4, 0.0001)]
        for ci, o in layout:
            for k, pq in enumerate(rows[ci]):
                if pq is None:
                    pt, q = rows[ci][k - 1]
                    V.append(pt + q * 0.00012 - unit(rows[ci][k - 1][0] - rows[ci][k - 2][0]) * 0.0003)
                else:
                    V.append(pq[0] + pq[1] * o)
        V = np.array(V)
        faces = grid_faces(len(layout), nv)
        return Part('heel_tape', V, faces, slot, 'angle', pid=pid)


# ------------------------------------------------------------------ the tongue

class Tongue:
    """The padded tongue under the lacing: on the last (just inside the eyestays, swelling up into the gap
    between them) from under the vamp back to the top eyelet, then standing up in front of the ankle."""

    def __init__(self, last, up, st):
        self.last, self.up, self.st = last, up, st
        e, xc = up.e, last.xc
        yc, yu = st['corner_y'], st['throat_y']
        self.y_front = yu + float(e(yu)) + 0.013
        self.y_leave = yc - 0.002
        ty, tz = st['tongue_top']
        ys = np.linspace(self.y_front, self.y_leave, 60)
        on = np.stack([xc(ys), ys, self.top(xc(ys), ys)], axis=1)
        p1 = on[-1]
        d1 = unit(on[-1] - on[-3])
        self.z_leave = float(p1[2])
        top = np.array([float(xc(ty)), ty, GROUND + tz])
        # Off the last in one easy curve (a cubic from the lacing's slope round to the top's lean back against the
        # shin), not a knee: in profile a padded tongue reads as a sweep, never as a hook.
        lean = math.radians(st.get('tongue_lean', 24.0))
        t1 = np.array([0.0, -math.sin(lean), math.cos(lean)])
        chord = float(np.linalg.norm(top - p1))
        b = [p1, p1 + d1 * chord * 0.42, top - t1 * chord * 0.42, top]
        u = np.linspace(0.0, 1.0, 41)[1:, None]
        rise = (1 - u) ** 3 * b[0] + 3 * (1 - u) ** 2 * u * b[1] + 3 * (1 - u) * u ** 2 * b[2] + u ** 3 * b[3]
        self.center = resample(np.vstack([on, rise]), 200)
        seg = np.linalg.norm(np.diff(self.center, axis=0), axis=1)
        self.arc = np.concatenate([[0.0], np.cumsum(seg)])
        self.length = self.arc[-1]
        T = unit(np.gradient(self.center, axis=0))
        self.nc = unit(np.stack([np.zeros(len(T)), T[:, 2], -T[:, 1]], axis=1))
        self.width = Curve1D([(0.0, 0.040), (0.15, 0.049), (0.5, 0.052), (0.8, 0.050), (1.0, 0.047)])

    def bulge(self, x, y):
        e, xc = self.up.e, self.last.xc
        yu = self.st['throat_y']
        gap = smoothstep(yu + float(e(yu)) + 0.001, yu, y)
        q = np.clip(1.0 - ((x - xc(y)) / np.maximum(e(y), 1e-4)) ** 2, 0.0, 1.0)
        return 0.0022 * np.power(q, 0.6) * gap

    def top(self, x, y):
        """The tongue's top surface over plan points where it lies on the last: its own gentle arch, tucked under
        the canvas where the eyestays cover it."""
        x = np.asarray(x, float)
        y = np.asarray(y, float)
        xc = self.last.xc(y)
        under = self.last.height(np.stack([x, y], axis=-1)) - 0.0028
        mid = self.last.height(np.stack([xc, y], axis=-1)) - 0.0028
        arch = mid - 6.0 * (x - xc) ** 2
        yc = self.st['corner_y']
        w = smoothstep(yc - 0.007, yc + 0.001, y)
        return arch + (np.minimum(arch, under) - arch) * w + self.bulge(x, y)

    def at(self, a, v):
        """Points on the top surface: a across (-1 the outside of the left foot .. 1), v along (0 the front)."""
        a = np.asarray(a, float)
        v = np.asarray(v, float)
        s = np.clip(v, 0.0, 1.0) * self.length
        c = np.stack([np.interp(s, self.arc, self.center[:, k]) for k in range(3)], axis=-1)
        nc = unit(np.stack([np.interp(s, self.arc, self.nc[:, k]) for k in range(3)], axis=-1))
        x = a * self.width(v) / 2
        on = c[..., 1] >= self.y_leave
        p_on = np.stack([c[..., 0] + x, c[..., 1], self.top(c[..., 0] + x, c[..., 1])], axis=-1)
        yl = np.full(np.shape(x), self.y_leave)
        xl = self.last.xc(yl)
        # Above the lacing the tongue flattens out from the last's section to a gentle arch.
        last_dome = self.top(xl + x, yl) - self.top(xl, yl)
        arch = -6.0 * x * x + self.bulge(xl + x, yl) - self.bulge(xl, yl)
        w = smoothstep(0.0, 0.012, (self.y_leave - c[..., 1]) + (c[..., 2] - self.z_leave))
        dome = last_dome * (1 - w) + arch * w
        p_up = c + np.stack([x, np.zeros_like(x), np.zeros_like(x)], axis=-1) + nc * dome[..., None]
        return np.where(on[..., None], p_on, p_up)

    def rows(self, nv, end=1.0):
        """Row positions along the tongue: as many in its last fifth (standing up, curving) as in the rest."""
        t = np.linspace(0.0, 1.0, nv)
        knee = 0.5
        vk = 0.78
        return np.where(t < knee, t / knee * vk, vk + (t - knee) / (1 - knee) * (end - vk))

    def parts(self, na=17, nv=34):
        a = np.linspace(-1.0, 1.0, na)
        ends = 1.0 - 0.07 * np.abs(a) ** 2.2
        V = np.zeros((na, nv, 3))
        for i in range(na):
            V[i] = self.at(np.full(nv, a[i]), self.rows(nv, ends[i]))
        N = unit(np.cross(np.gradient(V, axis=0), np.gradient(V, axis=1)))
        if N[:, :, 2].mean() < 0:
            N = -N
        self.grid, self.grid_n = V, N
        vv = self.rows(nv)
        thick = (0.0018 + 0.0027 * (1.0 - a ** 2) ** 1.5)[:, None] * (1.0 + 0.2 * smoothstep(0.86, 1.0, vv))[None, :]
        thick = thick[:, :, None]
        B = V - N * thick
        top_faces = oriented(V.reshape(-1, 3), grid_faces(na, nv), lambda p: np.array([0.0, 0.25, 1.0]))
        bot_faces = [tuple(reversed(f)) for f in top_faces]
        # The rim: round the boundary, the top's edge, a middle ring pushed out, the bottom's edge.
        loop = [(0, i) for i in range(nv)] + [(j, nv - 1) for j in range(1, na)] + [(na - 1, i) for i in range(nv - 2, -1, -1)] + \
               [(j, 0) for j in range(na - 2, 0, -1)]
        lt = np.array([V[j, i] for j, i in loop])
        lb = np.array([B[j, i] for j, i in loop])
        ln = np.array([N[j, i] for j, i in loop])
        tang = unit(np.roll(lt, -1, 0) - np.roll(lt, 1, 0))
        outw = unit(np.cross(tang, ln))
        cen = V.reshape(-1, 3).mean(axis=0)
        if np.sum(outw * (lt - cen)) < 0:
            outw = -outw
        th = np.array([thick[j, i, 0] for j, i in loop])
        L = len(loop)
        # A rounded edge: half a circle from the top's edge out and round to the bottom's.
        rings = [lt]
        mid = (lt + lb) / 2
        half = (lt - lb) / 2
        for phi in (0.25 * math.pi, 0.5 * math.pi, 0.75 * math.pi):
            rings.append(mid + half * math.cos(phi) + outw * (th * 0.5 * math.sin(phi))[:, None])
        rings.append(lb)
        rim_v = np.concatenate(rings)
        rim_f = []
        for r in range(len(rings) - 1):
            for k in range(L):
                k1 = (k + 1) % L
                rim_f.append((r * L + k, r * L + k1, (r + 1) * L + k1, (r + 1) * L + k))
        rim_f = oriented(rim_v, rim_f, lambda p: p - cen)
        seam = np.zeros(len(rim_v), bool)
        seam[np.arange(len(rings)) * L] = True
        out = [Part('tongue', V.reshape(-1, 3), top_faces, UPPER), Part('tongue_under', B.reshape(-1, 3), bot_faces, LINING),
               Part('tongue_rim', rim_v, rim_f, UPPER, seam=seam)]
        out.append(self.label())
        return out

    def label(self):
        """The woven label near the top: black, its print read from in front."""
        na, nv = 9, 4
        a = np.linspace(-0.47, 0.47, na)
        v0 = 1.0 - 0.0128 / self.length
        v = np.linspace(v0, v0 + 0.0102 / self.length, nv)
        P = np.array([[self.at(ai, vi) for vi in v] for ai in a])
        N = unit(np.cross(np.gradient(P, axis=0), np.gradient(P, axis=1)))
        if N[:, :, 2].mean() + N[:, :, 1].mean() < 0:
            N = -N
        face = P + N * 0.00045
        mid = P[na // 2, nv // 2]
        self.label_frame = dict(center=tuple(P[na // 2, :].mean(axis=0) + N[na // 2, nv // 2] * 0.00045), up=tuple(unit(P[na // 2, -1] - P[na // 2, 0])),
                                w=float(np.linalg.norm(P[-1, nv // 2] - P[0, nv // 2])), h=float(np.linalg.norm(P[na // 2, -1] - P[na // 2, 0])))
        # The patch's face and a skirt round it down to the tongue.
        ring = [(0, i) for i in range(nv)] + [(j, nv - 1) for j in range(1, na)] + [(na - 1, i) for i in range(nv - 2, -1, -1)] + \
               [(j, 0) for j in range(na - 2, 0, -1)]
        Vf = face.reshape(-1, 3)
        faces = grid_faces(na, nv)
        faces = oriented(Vf, faces, lambda p: N.reshape(-1, 3).mean(axis=0))
        base = len(Vf)
        skirt = np.array([P[j, i] + N[j, i] * 0.00005 for j, i in ring])
        V = np.vstack([Vf, skirt])
        L = len(ring)
        sk = []
        for k in range(L):
            k1 = (k + 1) % L
            j0, i0 = ring[k]
            j1, i1 = ring[k1]
            sk.append((j0 * nv + i0, j1 * nv + i1, base + k1, base + k))
        sk = oriented(V, sk, lambda p: p - Vf.mean(axis=0))
        return Part('tongue_label', V, faces + sk, LINING, pid=1.0, uvw=3.0)


# ------------------------------------------------------------------ eyelets and laces

def frame_z(n):
    """A rotation taking +Z to the direction n."""
    return Vector(tuple(n)).to_track_quat('Z', 'Y').to_matrix().to_4x4()


def lathe_part(name, profile, seg, matrix, slot, pid=0.0, flip=False):
    m = len(profile)
    V = []
    for k in range(seg):
        a = 2 * math.pi * k / seg
        for r, h in profile:
            V.append(matrix @ Vector((r * math.cos(a), r * math.sin(a), h)))
    faces = []
    for k in range(seg):
        k1 = (k + 1) % seg
        for i in range(m - 1):
            f = (k * m + i, k1 * m + i, k1 * m + i + 1, k * m + i + 1)
            faces.append(tuple(reversed(f)) if flip else f)
    seam = np.zeros(len(V), bool)
    seam[:m] = True
    return Part(name, np.array([tuple(v) for v in V]), faces, slot, 'angle', seam, pid=pid)


def eyelet_points(last, up, st):
    ys = np.linspace(st['eyelet_y'][0], st['eyelet_y'][1], st['eyelets'])
    out = {}
    for s in (1.0, -1.0):
        x = last.xc(ys) + s * (up.e(ys) + st['eyelet_in'])
        P = last.point(x, ys)
        N = last.normal(np.stack([x, ys], axis=1))
        out[s] = (P, N)
    return ys, out


def eyelets(last, up, st, seg=10):
    ys, pts = eyelet_points(last, up, st)
    r_in, r_out = st['eyelet_r']
    parts = []
    prof = [(r_in, -0.0014), (r_in, 0.0002), (r_in + 0.0003, 0.00075), (r_in + 0.0009, 0.0009), (r_out - 0.0005, 0.00085), (r_out, 0.0004),
            (r_out + 0.0001, -0.0003)]
    for s, (P, N) in pts.items():
        for p, n in zip(P, N):
            M = Matrix.Translation(Vector(tuple(p))) @ frame_z(n)
            parts.append(lathe_part('eyelet', prof, seg, M, LACES, pid=2.0, flip=True))
            # The hole: dark, the lace goes down through it.
            hole = [(0.0, 0.0001), (r_in, 0.0001)]
            parts.append(lathe_part('eyelet_hole', hole, seg, M, LINING, pid=3.0, flip=True))
    return parts


def lace_profile(w, t, m=8):
    out = []
    for k in range(m):
        th = 2 * math.pi * (k + 0.5) / m
        c, s = math.cos(th), math.sin(th)
        out.append((t / 2 * math.copysign(abs(s) ** 0.75, s), w / 2 * math.copysign(abs(c) ** 0.45, c)))
    return out


class Laces:
    """Flat cotton laces, criss-crossed from the bottom pair up (each run comes up through an eyelet, crosses the
    tongue and slips under the far eyestay to the next eyelet up), tied at the top in a bow."""

    def __init__(self, last, up, tongue, st):
        self.last, self.up, self.tongue, self.st = last, up, tongue, st
        self.w, self.t = st['lace']
        self.round = st.get('round_laces', False)
        self.aglet_r = (0.0011, 0.0016) if not self.round else (self.t / 2 + 0.0003, self.t / 2 + 0.0003)
        self.ys, self.eye = eyelet_points(last, up, st)

    def on_tongue(self, x, y, lift):
        z = self.tongue.top(np.asarray(x, float), np.asarray(y, float))
        n = self.last.normal(np.stack([np.asarray(x, float), np.asarray(y, float)], axis=-1))
        return np.stack([x, y, z], axis=-1) + n * lift

    def under(self, s, y):
        x = float(self.last.xc(y)) + s * (float(self.up.e(y)) + 0.0045)
        return np.array([x, y, float(self.last.height(np.array([[x, y]]))[0]) - 0.0032])

    def edge_in(self, s, y, lift=None):
        x = float(self.last.xc(y)) + s * (float(self.up.e(y)) - 0.0012)
        return self.on_tongue(x, y, self.t / 2 + 0.0002 if lift is None else lift)

    def emerge(self, s, k, toward):
        """Up out of eyelet k on side s and over the eyestay's edge, heading toward (dx, dy)."""
        P, N = self.eye[s]
        p, n = P[k], N[k]
        y = self.ys[k]
        x_edge = float(self.last.xc(y)) + s * (float(self.up.e(y)) + 0.0004)
        dy = toward * 0.0018
        edge = self.last.point(x_edge, y + dy, 0.0022)
        return [p - n * 0.0016, p + n * 0.0011 - np.array([s * 0.0012, -dy * 0.3, 0.0]), edge]

    def profile(self, scale=1.0):
        if self.round:
            # Eight sides: 45 degrees between faces stays under the mesh's 50-degree sharp split, so it shades round.
            return [(self.t / 2 * math.sin(2 * math.pi * (k + 0.5) / 8), self.t / 2 * math.cos(2 * math.pi * (k + 0.5) / 8)) for k in range(8)]
        return lace_profile(self.w * scale, self.t, 6)

    def run(self, pts, ups=None, n=None):
        path = spline3(np.array(pts), n or max(12, int(np.sum(np.linalg.norm(np.diff(np.array(pts), axis=0), axis=1)) / 0.0026)))
        if ups is None:
            ups = self.last.normal(path[:, :2])
        prof = self.profile()
        V, F, seam = sweep(path, ups, prof, cap=True)
        return Part('lace', V, F, LACES, 'smart', seam, np.vstack([sweep_panel(path, len(prof)), np.zeros((2, 2))]))

    def parts(self):
        ys = self.ys
        n = len(ys)
        xc, e = self.last.xc, self.up.e
        out = []
        # The bottom bar, straight across on top.
        y0 = ys[0]
        pts = self.emerge(1.0, 0, 0.0) + [self.on_tongue(float(xc(y0)) + f * float(e(y0)), y0, self.t / 2 + 0.0003) for f in (0.6, 0.0, -0.6)]
        pts += list(reversed(self.emerge(-1.0, 0, 0.0)))
        out.append(self.run(pts))
        for k in range(n - 1):
            ya, yb = ys[k], ys[k + 1]
            for s in (1.0, -1.0):
                over = (k + (1 if s > 0 else 0)) % 2 == 0
                if k == 0:
                    pts = [self.under(s, ya), self.edge_in(s, ya)]
                else:
                    pts = self.emerge(s, k, -1.0)
                for f, frac in ((0.3, 0.55), (0.5, 0.0), (0.7, -0.55)):
                    y = ya + (yb - ya) * f
                    bump = (self.t * 1.05) * math.exp(-((f - 0.5) / 0.2) ** 2) if over else 0.0
                    out_x = float(xc(y)) + s * frac * float(e(y))
                    pts.append(self.on_tongue(out_x, y, self.t / 2 + 0.0003 + bump))
                pts += [self.edge_in(-s, yb), self.under(-s, yb)]
                out.append(self.run(pts))
        # The top pair up to the knot.
        yt = ys[-1]
        yk = yt - 0.0045
        xk = float(xc(yk))
        knot = self.on_tongue(xk, yk, 0.0036)
        for s in (1.0, -1.0):
            pts = self.emerge(s, n - 1, -1.0) + [self.on_tongue(xk + s * 0.55 * float(e(yt)), yt - 0.004, self.t / 2 + 0.0008),
                                                   knot + np.array([s * 0.0035, 0.0006, -0.0012])]
            out.append(self.run(pts))
        out += self.bow(knot, xk, yk)
        return out

    def surf(self, xk, yk, dx, dy, lift):
        """A point lying on the upper, dx measured along its surface across the shoe from the knot (so a loop
        flops over the shoulder and down the quarter by its own length, rather than off the last's plan)."""
        y = yk + dy
        edge = float(np.interp(y, self.last.ys, self.last.xmax if dx >= 0 else self.last.xmin))
        xs = np.linspace(xk, edge, 240)
        zs = self.last.height(np.stack([xs, np.full_like(xs, y)], axis=1))
        arc = np.concatenate([[0.0], np.cumsum(np.hypot(np.diff(xs), np.diff(zs)))])
        x = float(np.interp(abs(dx), arc, xs))
        return self.last.point(x, y, lift)

    def bow(self, knot, xk, yk):
        out = []
        # The knot: a wrap round the loops' roots, its lace running round the X axis.
        ring = []
        nk = self.last.normal(np.array([[xk, yk]]))[0]
        fy = unit(np.cross(np.array([1.0, 0.0, 0.0]), nk))
        for k in range(14):
            a = 2 * math.pi * k / 14
            ring.append(knot + fy * (0.0040 * math.cos(a)) + nk * (0.0030 * math.sin(a)))
        ring = np.array(ring)
        radial = unit(ring - knot)
        prof = self.profile(1.05)
        V, F, seam = sweep(ring, radial, prof, closed=True)
        out.append(Part('knot', V, F, LACES, 'smart', seam, sweep_panel(ring, len(prof))))
        # Two loops lying out over the eyestays toward the toe, one a little longer.
        bow = self.st.get('bow', 1.0)
        for s, scale in ((1.0, bow), (-1.0, 1.12 * bow)):
            loop = [(0.0, 0.0, 0.0034), (0.006, 0.0015, 0.0052), (0.015, 0.0045, 0.0062), (0.024, 0.0085, 0.0052), (0.032, 0.0135, 0.0036),
                    (0.0375, 0.0195, 0.0026), (0.0365, 0.0245, 0.0026), (0.030, 0.0245, 0.0034), (0.021, 0.0195, 0.0046), (0.012, 0.013, 0.0056),
                    (0.0045, 0.0065, 0.0048), (0.0005, 0.0015, 0.0034)]
            pts = [self.surf(xk, yk, s * dx * scale, dy * scale, lift) for dx, dy, lift in loop]
            pts[0] = knot + np.array([s * 0.002, -0.001, 0.0])
            pts[-1] = knot + np.array([s * 0.001, 0.0015, -0.0006])
            out.append(self.run(pts))
        # Tails from under the loops: one lying forward along the lacing, one off the side and down the quarter.
        side_pts = [(0.0, 0.0, 0.0030), (0.004, 0.0045, 0.0042), (0.006, 0.013, 0.0046), (0.0045, 0.024, 0.0044), (0.0010, 0.036, 0.0040),
                    (-0.0025, 0.047, 0.0038)]
        pts = [self.surf(xk, yk, bow * dx, bow * dy, lift) for dx, dy, lift in side_pts]
        pts[0] = knot + np.array([0.0015, 0.0005, -0.001])
        out += self.tail(pts)
        down = [(0.0, 0.0, 0.0030), (-0.006, 0.004, 0.0038), (-0.014, 0.0085, 0.0034), (-0.022, 0.012, 0.0024)]
        pts = [self.surf(xk, yk, bow * dx, bow * dy, lift) for dx, dy, lift in down]
        pts[0] = knot + np.array([-0.0015, 0.0005, -0.001])
        for y, z in ((yk + 0.019, 0.073), (yk + 0.026, 0.064), (yk + 0.031, 0.055), (yk + 0.034, 0.047)):
            pts.append(self.last.side_point(np.array([y]), np.array([GROUND + z]), -1.0, 0.0022)[0])
        out += self.tail(pts)
        return out

    def tail(self, pts, aglet=0.014):
        path = spline3(np.array(pts), 36)
        seg = np.linalg.norm(np.diff(path, axis=0), axis=1)
        arc = np.concatenate([[0.0], np.cumsum(seg)])
        cut = int(np.searchsorted(arc, arc[-1] - aglet))
        ups = self.last.normal(path[:, :2])
        prof = self.profile()
        V, F, seam = sweep(path[:cut + 1], ups[:cut + 1], prof, cap=True)
        lace = Part('tail', V, F, LACES, 'smart', seam, np.vstack([sweep_panel(path[:cut + 1], len(prof)), np.zeros((2, 2))]))
        # The aglet: a hard sleeve, rounder, its end capped.
        ag = path[cut - 1:]
        r = self.aglet_r
        prof = [(r[0] * math.sin(2 * math.pi * (k + 0.5) / 8), r[1] * math.cos(2 * math.pi * (k + 0.5) / 8)) for k in range(8)]
        Va, Fa, seam_a = sweep(ag, ups[cut - 1:], prof, cap=True)
        pa = np.vstack([sweep_panel(ag, 8), np.zeros((2, 2))])
        return [lace, Part('aglet', Va, Fa, LACES, 'smart', seam_a, pa, pid=1.0)]


def pull_tab(up, width=0.0135, below=0.010, above=0.0145, thick=0.0015, lean=12.0):
    """The heel's pull tab (accent): webbing stitched over the collar's roll, standing up and back a little."""
    p = up.top[0]
    n = up.normals[0, -1]
    u = unit(up.G[0, -1] - up.G[0, -2])
    u = unit(u - n * np.dot(u, n))
    x = unit(np.cross(u, n))
    r = 0.0045
    outline = core.rounded_rect(-width / 2, -below, width / 2, above, r, steps=5, radii=(0.0, 0.0, r, r))
    bm = core.slab(outline, 0.0, thick)
    V = np.array([tuple(v.co) for v in bm.verts])
    F = [tuple(v.index for v in f.verts) for f in bm.faces]
    bm.free()
    la = math.radians(lean)
    out = []
    for vx, vy, vz in V:
        t = max(vy, 0.0) / above
        bend = la * t
        uu = u * math.cos(bend) + n * math.sin(bend)
        nn = n * math.cos(bend) - u * math.sin(bend)
        out.append(p + x * vx + u * min(vy, 0.0) + uu * max(vy, 0.0) + nn * (0.0016 + vz))
    out = np.array(out)
    F = oriented(out, F, lambda q: q - (p + n * (0.0016 + thick / 2)))
    # Where its print goes: on the back of the part above the collar, read from behind.
    t = 0.45
    bend = la * t
    uu = u * math.cos(bend) + n * math.sin(bend)
    nn = n * math.cos(bend) - u * math.sin(bend)
    c = p + uu * (above * t) + nn * (0.0016 + thick)
    frame = dict(center=tuple(c), up=tuple(uu), w=width * 0.86, h=width * 0.86 * 60 / 135)
    return Part('pull_tab', out, F, ACCENT, 'smart', pid=2.0, uvw=2.0), frame


def heel_label(last, st, width=0.024, z0=0.0193, z1=0.0272, lift=0.0006):
    """A rubber label on the back of the foxing, raised a little (its lettering is in the rubber's material)."""
    half = width / 2 / last.sole.length
    s = np.linspace(-half, half, 11)
    O, N = last.sole.at(s)
    zs = GROUND + np.linspace(z0, z1, 5)
    face = np.array([[np.append(O[j] + N[j] * lift, z) for z in zs] for j in range(len(s))])
    nu, nv = face.shape[:2]
    ring = [(0, i) for i in range(nv)] + [(j, nv - 1) for j in range(1, nu)] + [(nu - 1, i) for i in range(nv - 2, -1, -1)] + \
           [(j, 0) for j in range(nu - 2, 0, -1)]
    Vf = face.reshape(-1, 3)
    faces = oriented(Vf, grid_faces(nu, nv), lambda q: np.array([q[0], q[1] + 0.03, 0.0]))
    base = len(Vf)
    skirt = np.array([np.append(O[j] - N[j] * 0.0002, zs[i]) for j, i in ring])
    V = np.vstack([Vf, skirt])
    L = len(ring)
    sk = [(ring[k][0] * nv + ring[k][1], ring[(k + 1) % L][0] * nv + ring[(k + 1) % L][1], base + (k + 1) % L, base + k) for k in range(L)]
    sk = oriented(V, sk, lambda q: q - Vf.mean(axis=0))
    c = O[len(s) // 2] + N[len(s) // 2] * lift
    frame = dict(center=(float(c[0]), float(c[1]), GROUND + (z0 + z1) / 2), w=width * 0.92, h=(z1 - z0) * 0.92)
    return Part('heel_label', V, faces + sk, RUBBER, pid=2.0, uvw=2.0), frame


def sole_part(last, st):
    """The vulcanized sole: the tread's rounded edge, a ribbed band, the foxing tape with its stripe, the lip
    that laps the canvas; the tread itself filled in to the middle."""
    n = 132
    s = np.linspace(0.0, 1.0, n + 1)
    O, N = last.sole.at(s)
    zf = Curve1D(st['foxing'])(O[:, 1])
    prof = st['sole']
    nr = len(prof)
    rings = []
    for off, z, rel, _ in prof:
        zz = GROUND + z + (zf if rel else 0.0)
        rings.append(np.concatenate([O + N * off, np.broadcast_to(zz, (n + 1,))[:, None]], axis=1))
    R = np.stack(rings)  # (rings, n + 1, 3): the last column repeats the first
    V = R.reshape(-1, 3)
    m = n + 1
    faces, slots = [], []
    for r in range(nr - 1):
        for j in range(n):
            faces.append((r * m + j, r * m + j + 1, (r + 1) * m + j + 1, (r + 1) * m + j))
            slots.append(prof[r][3])
    # Panel: U along the sole's outline (meters), V the height above the ground; hu the distance in from the edge.
    along = np.concatenate([[0.0], np.cumsum(np.linalg.norm(np.diff(O, axis=0), axis=1))])
    panel = [np.stack([along, R[r, :, 2] - GROUND], axis=1) for r in range(nr)]
    hu = [np.full(m, max(0.0, -prof[r][0]) if prof[r][1] < 0.002 and not prof[r][2] else 0.0) for r in range(nr)]
    # The tread: rings in from the edge's first ring to the middle.
    c = np.array([R[0, :n, 0].mean(), R[0, :n, 1].mean(), GROUND])
    fills = (0.94, 0.86, 0.75, 0.6, 0.44, 0.27, 0.12)
    FV, FF = disc_fill(R[0, :n], c, rings=fills)
    off = len(V)
    remap = np.concatenate([np.arange(n), off + np.arange(len(FV) - n)])  # the fill's outer ring is the edge's first ring
    V = np.vstack([V, FV[n:]])
    for f in FF:
        faces.append(tuple(int(remap[i]) for i in f))
        slots.append(OUTSOLE)
    edge_in = np.linalg.norm(R[0, :n, :2] - c[:2], axis=1)
    fill_hu = np.concatenate([edge_in * (1 - f) - prof[0][0] for f in fills] + [[0.06]])
    panel = np.vstack(panel + [np.stack([np.zeros(len(FV) - n), np.zeros(len(FV) - n)], axis=1)])
    hu = np.concatenate(hu + [fill_hu])
    # zone: the height relative to the foxing's top line there (the welt's stitching follows it as it slopes).
    zone = np.concatenate([R[r, :, 2] - GROUND - zf for r in range(nr)] + [np.full(len(FV) - n, -0.05)])
    seam = np.zeros(len(V), bool)
    # Seams every 1/12 of the way round: the bands (and the stripe) unwrap in short pieces that pack well.
    for q in np.linspace(0.0, 1.0, 13):
        seam[np.arange(nr) * m + int(round(q * n))] = True
    faces = oriented(V, faces, lambda p: np.array([p[0] - c[0], p[1] - c[1], (p[2] - (GROUND + 0.012)) * 4.0]))
    return Part('sole', V, faces, slots, 'angle', seam, panel, hu=hu, zone=zone)



def insole_part(last):
    n = 96
    s = np.linspace(0.0, 1.0, n + 1)[:-1]
    O, N = last.sole.at(s)
    ring = O - N * (last.st['inset'] + 0.0032)
    z = last.base + 0.0022
    ring3 = np.concatenate([ring, np.full((n, 1), z)], axis=1)
    c = np.array([ring[:, 0].mean(), ring[:, 1].mean(), z])
    V, F = disc_fill(ring3, c)
    F = oriented(V, F, lambda p: np.array([0.0, 0.0, 1.0]))
    return Part('insole', V, F, LINING, 'angle', pid=2.0)


# ------------------------------------------------------------------ objects and UVs

def to_object(part, mats, name=None):
    me = bpy.data.meshes.new(name or part.name)
    me.from_pydata(part.verts.tolist(), [], part.faces)
    for m in mats:
        me.materials.append(m)
    me.polygons.foreach_set('material_index', part.slots)
    me.uv_layers.new(name='UVMap')
    pan = me.uv_layers.new(name='panel')
    loops_v = np.empty(len(me.loops), dtype=np.int32)
    me.loops.foreach_get('vertex_index', loops_v)
    pan.data.foreach_set('uv', part.panel[loops_v].astype(np.float32).ravel())
    for k, v in part.attrs.items():
        a = me.attributes.new(k, 'FLOAT', 'POINT')
        a.data.foreach_set('value', v.astype(np.float32))
    a = me.attributes.new('seamv', 'FLOAT', 'POINT')
    a.data.foreach_set('value', part.seam.astype(np.float32))
    a = me.attributes.new('uvmode', 'INT', 'FACE')
    a.data.foreach_set('value', np.full(len(part.faces), 1 if part.mode == 'smart' else 0, dtype=np.int32))
    me.update()
    return core.link(bpy.data.objects.new(name or part.name, me))


def unwrap(obj):
    """Seams where the parts asked for them and between materials; smooth surfaces unwrapped by angle, small
    hard pieces (laces, the pull tab) smart-projected."""
    me = obj.data
    bm = bmesh.new()
    bm.from_mesh(me)
    sv = bm.verts.layers.float['seamv']
    for e in bm.edges:
        a, b = e.verts
        lf = e.link_faces
        e.seam = bool((a[sv] > 0.5 and b[sv] > 0.5) or (len(lf) == 2 and lf[0].material_index != lf[1].material_index))
    bm.to_mesh(me)
    bm.free()
    me.uv_layers.active = me.uv_layers['UVMap']
    me.uv_layers['UVMap'].active_render = True
    core.select_only([obj])
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_mode(type='FACE')
    for mode in (0, 1):
        bpy.ops.mesh.select_all(action='DESELECT')
        bm = bmesh.from_edit_mesh(me)
        lay = bm.faces.layers.int['uvmode']
        for f in bm.faces:
            if f[lay] == mode:
                f.select_set(True)
        bmesh.update_edit_mesh(me)
        if mode == 0:
            bpy.ops.uv.unwrap(method='ANGLE_BASED', margin=0.002)
        else:
            bpy.ops.uv.smart_project(angle_limit=math.radians(60.0), island_margin=0.002)
    bpy.ops.object.mode_set(mode='OBJECT')


def _area2(pts):
    a = 0.0
    for i in range(len(pts)):
        x0, y0 = pts[i]
        x1, y1 = pts[(i + 1) % len(pts)]
        a += x0 * y1 - x1 * y0
    return abs(a) / 2


def pack_uvs(obj, margin=0.005):
    """Every material's islands packed into its own unit square at one texel density (each material bakes to
    its own textures)."""
    me = obj.data
    bm = bmesh.new()
    bm.from_mesh(me)
    uv = bm.loops.layers.uv['UVMap']
    uvw = bm.verts.layers.float['uvw']
    bm.faces.ensure_lookup_table()
    parent = list(range(len(bm.faces)))

    def find(i):
        r = i
        while parent[r] != r:
            r = parent[r]
        while parent[i] != r:
            parent[i], i = r, parent[i]
        return r

    for e in bm.edges:
        lf = e.link_faces
        if len(lf) != 2 or lf[0].material_index != lf[1].material_index:
            continue
        same = True
        for v in e.verts:
            u0 = next(l[uv].uv for l in lf[0].loops if l.vert == v)
            u1 = next(l[uv].uv for l in lf[1].loops if l.vert == v)
            if (u0 - u1).length_squared > 1e-14:
                same = False
                break
        if same:
            parent[find(lf[0].index)] = find(lf[1].index)
    islands = {}
    for f in bm.faces:
        islands.setdefault(find(f.index), []).append(f)
    by_slot = {}
    for fl in islands.values():
        by_slot.setdefault(fl[0].material_index, []).append(fl)
    for slot, isl in by_slot.items():
        boxes = []
        for fl in isl:
            a3 = sum(f.calc_area() for f in fl)
            auv = sum(_area2([tuple(l[uv].uv) for l in f.loops]) for f in fl)
            k = math.sqrt(a3 / auv) if auv > 1e-14 else 1.0
            k *= max(v[uvw] for f in fl for v in f.verts)
            us = [l[uv].uv.x * k for f in fl for l in f.loops]
            vs = [l[uv].uv.y * k for f in fl for l in f.loops]
            u0, u1, v0, v1 = min(us), max(us), min(vs), max(vs)
            rot = (v1 - v0) > (u1 - u0)
            w, h = (v1 - v0, u1 - u0) if rot else (u1 - u0, v1 - v0)
            boxes.append((fl, k, u0, u1, v0, v1, rot, max(w, 1e-6), max(h, 1e-6)))
        order = sorted(range(len(boxes)), key=lambda i: -boxes[i][8])

        def pack(s):
            x = y = shelf = 0.0
            pos = [None] * len(boxes)
            for i in order:
                w, h = boxes[i][7] * s, boxes[i][8] * s
                if w > 1.0:
                    return None
                if x + w > 1.0:
                    y += shelf + margin
                    x = shelf = 0.0
                if y + h > 1.0:
                    return None
                pos[i] = (x, y)
                x += w + margin
                shelf = max(shelf, h)
            return pos

        total = sum(b[7] * b[8] for b in boxes)
        lo, hi = 0.0, math.sqrt(1.0 / total) * 1.2
        for _ in range(40):
            mid = (lo + hi) / 2
            if pack(mid) is not None:
                lo = mid
            else:
                hi = mid
        pos = pack(lo)
        for (fl, k, u0, u1, v0, v1, rot, w, h), (px, py) in zip(boxes, pos):
            for f in fl:
                for l in f.loops:
                    u, v = l[uv].uv.x * k, l[uv].uv.y * k
                    a, b = (v - v0, u1 - u) if rot else (u - u0, v - v0)
                    l[uv].uv = (px + a * lo, py + b * lo)
    bm.to_mesh(me)
    bm.free()


# ------------------------------------------------------------------ materials

def attr(m, name):
    n = m.node('ShaderNodeAttribute')
    n.attribute_type = 'GEOMETRY'
    n.attribute_name = name
    return n.outputs['Fac']


def panel_uv(m):
    n = m.node('ShaderNodeUVMap')
    n.uv_map = 'panel'
    sep = m.node('ShaderNodeSeparateXYZ')
    m.link(n.outputs['UV'], sep.inputs['Vector'])
    return sep.outputs['X'], sep.outputs['Y']


def less(m, d, edge, width):
    """About 1 where d < edge (a constant or a socket), fading across edge +/- width."""
    if not isinstance(edge, (int, float)):
        d = m.math('SUBTRACT', d, edge)
        edge = 0.0
    return smooth_less(m, d, edge, width)


def near(m, d, c, half, soft):
    return less(m, m.math('ABSOLUTE', m.math('SUBTRACT', d, c)), half, soft)


def part_is(m, pid, k):
    return near(m, pid, float(k), 0.4, 0.05)


def add(m, *terms):
    out = terms[0]
    for t in terms[1:]:
        out = m.math('ADD', out, t)
    return out


def mul(m, a, b):
    return m.math('MULTIPLY', a, b)


def inv(m, a):
    return m.math('SUBTRACT', 1.0, a)


def normal_z(m):
    g = m.node('ShaderNodeNewGeometry')
    s = m.node('ShaderNodeSeparateXYZ')
    m.link(g.outputs['Normal'], s.inputs['Vector'])
    return s.outputs['Z']


def stitch_row(m, across, center, along, period=0.0032, width=0.0007, duty=0.6):
    """A row of lock stitches: (the thread, a raised pillow 0..1; the needle line pressed into the material)."""
    f = m.math('SUBTRACT', m.math('FRACT', m.math('DIVIDE', along, period)), 0.5)
    a = m.math('DIVIDE', m.math('ABSOLUTE', f), duty / 2)
    b = m.math('DIVIDE', m.math('ABSOLUTE', m.math('SUBTRACT', across, center)), width / 2)
    thread = m.math('SUBTRACT', 1.0, m.math('ADD', mul(m, a, a), mul(m, b, b)), clamp=True)
    groove = near(m, across, center, width * 0.3, width * 0.25)
    return thread, groove


def bump(m, height):
    """The surface's relief from a height in meters."""
    b = m.node('ShaderNodeBump', Strength=1.0, Distance=1.0)
    m.link(height, b.inputs['Height'])
    m.set('Normal', b.outputs['Normal'])


def frame_uv(m, sep, center, right, up, w, h):
    """Print coordinates on a label: center (meters), its right and up directions, its width and height."""
    cx, cy, cz = center
    dx = m.math('SUBTRACT', sep.outputs['X'], cx)
    dy = m.math('SUBTRACT', sep.outputs['Y'], cy)
    dz = m.math('SUBTRACT', sep.outputs['Z'], cz)

    def dot(d):
        return add(m, mul(m, dx, float(d[0])), mul(m, dy, float(d[1])), mul(m, dz, float(d[2])))
    u = m.math('ADD', m.math('DIVIDE', dot(right), w), 0.5)
    v = m.math('ADD', m.math('DIVIDE', dot(up), h), 0.5)
    return vec(m, u, v, 0.0)


def print_at(m, img, uv_vec):
    """(color, alpha) of a print sampled at uv, nothing outside it."""
    n = m.image(img[0], vector=uv_vec, extension='CLIP')
    return n.outputs['Color'], n.outputs['Alpha']


def crease_field(m, sep, info, sx, depth=1.0):
    """The creases where the shoe flexes over the ball: arcs across the vamp, the inside a little ahead of the
    outside, wobbling; and finer wrinkles between them. Returns (creases, fine)."""
    x, y = sep.outputs['X'], sep.outputs['Y']
    xr = m.math('SUBTRACT', x, info['ridge'] * sx)
    xr2 = mul(m, xr, xr)
    total = None
    for i, (y0, d, w, bend, half, xo) in enumerate(info['creases']):
        # The same arcs the geometry dents (Upper.flex), mirrored with the shoe.
        wob = mul(m, m.math('SINE', m.math('ADD', mul(m, xr, 210.0 * sx), 1.3 * i)), 0.0016)
        yw = add(m, y0, mul(m, xr, 0.12 * sx), mul(m, xr2, -bend), wob)
        t = m.math('DIVIDE', m.math('SUBTRACT', y, yw), w)
        along = m.math('DIVIDE', m.math('SUBTRACT', xr, xo * sx), half)
        g = mul(m, m.math('EXPONENT', mul(m, add(m, mul(m, t, t), mul(m, along, along)), -1.0)), d * depth)
        total = g if total is None else m.math('ADD', total, g)
    wob = noise(m, scaled(m, sep, 30.0, 30.0, 30.0), detail=2.0)
    top = ramp(m, normal_z(m), 0.2, 0.75)
    total = mul(m, mul(m, total, m.math('ADD', 0.45, wob)), top)
    zone = mul(m, ramp(m, y, info['creases'][0][0] - 0.012, info['creases'][0][0]), inv(m, ramp(m, y, info['creases'][-1][0], info['creases'][-1][0] + 0.014)))
    fine = ramp(m, noise(m, scaled(m, sep, 25.0, 420.0, 25.0), detail=3.0), 0.55, 0.75)
    fine = mul(m, mul(m, fine, zone), top)
    return total, fine


def upper_material(name, info, sx):
    """Slot 0. The sneaker: cotton canvas, baked light neutral grey for the game to tint. The work shoe: black
    leather. Stitching runs in the panel UV (along the top line and down from it); the heel panel's edge leans
    forward down the quarters; creases where the toe bends; grime from the ground up and on the toe."""
    st = info['st']
    leather = st['leather']
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    U, V = panel_uv(m)
    hu, zone = attr(m, 'hu'), attr(m, 'zone')
    zr = m.math('SUBTRACT', z, GROUND)
    if leather:
        grain = noise(m, o, scale=2600.0, detail=2.0, distortion=0.4)
        pores = ramp(m, noise(m, o, scale=6000.0, detail=1.0), 0.6, 0.7)
        texture = add(m, mul(m, grain, 0.00003), mul(m, pores, -0.00001))
        heather = noise(m, o, scale=60.0, detail=3.0)
    else:
        p = 0.0013
        wa = m.math('SINE', mul(m, m.math('ADD', x, z), 2 * math.pi / p))
        wb = m.math('SINE', mul(m, m.math('SUBTRACT', y, z), 2 * math.pi / p))
        weave = mul(m, m.math('ADD', mul(m, wa, wb), 1.0), 0.5)
        slub = noise(m, scaled(m, sep, 140.0, 900.0, 140.0), detail=2.0)
        texture = add(m, mul(m, weave, 0.00003), mul(m, slub, 0.00002))
        heather = noise(m, o, scale=420.0, detail=3.0)
    # Stitching along the top line: two rows on the collar's roll; on the eyestay one at its edge, one outside the
    # eyelets.
    c1 = mix_float(m, zone, st['stitch_eye'][0], st['stitch_collar'][0])
    c2 = mix_float(m, zone, st['stitch_eye'][1], st['stitch_collar'][1])
    t1, g1 = stitch_row(m, V, c1, U, width=st['thread'])
    t2, g2 = stitch_row(m, V, c2, U, width=st['thread'])
    # The eyestay is a panel of its own, lapped over the quarter: a step down at its outer edge.
    eye = mul(m, less(m, V, st['stitch_eye'][1] + 0.0022, 0.00025), inv(m, zone))
    # The heel panel over the quarters, its front edge leaning forward down toward the sole.
    edge = m.math('ADD', st['counter'][0], mul(m, V, st['counter'][1]))
    dh = m.math('SUBTRACT', hu, edge)
    below_roll = ramp(m, V, 0.009, 0.012)
    counter = mul(m, less(m, dh, 0.0, 0.00025), below_roll)
    t3, g3 = stitch_row(m, dh, -0.0026, V, width=st['thread'])
    t4, g4 = stitch_row(m, dh, -0.0048, V, width=st['thread'])
    t3, g3, t4, g4 = (mul(m, s, below_roll) for s in (t3, g3, t4, g4))
    threads = add(m, t1, t2, t3, t4)
    grooves = add(m, g1, g2, g3, g4)
    overlap = add(m, mul(m, eye, 0.00022), mul(m, counter, 0.00026))
    if st.get('derby'):
        # The derby's quarters lap over the vamp: their front edge runs from the throat down and back to the welt.
        y0, z0, slope = st['derby']
        side = ramp(m, m.math('ABSOLUTE', m.math('SUBTRACT', x, info['ridge'] * sx)), 0.016, 0.022)
        dq = m.math('SUBTRACT', y, m.math('ADD', y0, mul(m, m.math('SUBTRACT', zr, z0), slope)))
        quarter = mul(m, less(m, dq, 0.0, 0.00025), side)
        t5, g5 = stitch_row(m, dq, -0.0022, z, width=st['thread'])
        t6, g6 = stitch_row(m, dq, -0.0042, z, width=st['thread'])
        threads = add(m, threads, mul(m, t5, side), mul(m, t6, side))
        grooves = add(m, grooves, mul(m, g5, side), mul(m, g6, side))
        overlap = add(m, overlap, mul(m, quarter, 0.0003))
    creases, fine = crease_field(m, sep, info, sx, depth=1.6 if leather else 1.35)
    # Grime: road dirt up from the sole, on the toe; a general dinge; scuffs.
    breakup = noise(m, scaled(m, sep, 30.0, 30.0, 90.0), detail=4.0)
    low = mul(m, inv(m, ramp(m, zr, 0.028, 0.050)), ramp(m, breakup, 0.25, 0.75))
    toe = mul(m, ramp(m, y, 0.17, 0.215), ramp(m, noise(m, o, scale=45.0, detail=4.0), 0.4, 0.75))
    dinge = noise(m, o, scale=16.0, detail=4.0)
    scuff = mul(m, ramp(m, noise(m, scaled(m, sep, 90.0, 35.0, 90.0), detail=4.0), 0.64, 0.7),
                m.math('MAXIMUM', ramp(m, y, 0.16, 0.2), inv(m, ramp(m, zr, 0.03, 0.045))))
    if leather:
        base = m.mix(mul(m, heather, 0.5), core.hex_linear(0x121214), core.hex_linear(0x1b1b1e))
        col = m.mix(mul(m, threads, 0.9), base, core.hex_linear(0x0b0b0c))
        # Worn leather goes grey-brown where it creases and scuffs, and the polish dulls.
        col = m.mix(mul(m, ramp(m, creases, 0.25, 0.9), 0.55), col, core.hex_linear(0x2c2925))
        col = m.mix(mul(m, fine, 0.25), col, core.hex_linear(0x2a2724))
        col = m.mix(mul(m, scuff, 0.7), col, core.hex_linear(0x4a443d))
        col = m.mix(mul(m, low, 0.55), col, core.hex_linear(0x3a352e))
        col = m.mix(mul(m, toe, 0.3), col, core.hex_linear(0x332f2a))
        rough = add(m, 0.36, mul(m, heather, 0.08), mul(m, creases, 0.18), mul(m, scuff, 0.25), mul(m, low, 0.3),
                    mul(m, ramp(m, y, 0.2, 0.15), -0.1))
        rough = mix_float(m, threads, rough, 0.55)
        m.set('Specular IOR Level', 0.5)
    else:
        base = m.mix(add(m, mul(m, heather, 0.6), mul(m, ramp(m, slub, 0.55, 0.8), 0.4)), core.hex_linear(0xcfcdc8), core.hex_linear(0xdad8d3))
        col = m.mix(mul(m, threads, 0.8), base, core.hex_linear(0xe4e2dd))
        col = m.mix(mul(m, ramp(m, creases, 0.15, 0.9), 0.45), col, core.hex_linear(0x958f84))
        col = m.mix(mul(m, fine, 0.18), col, core.hex_linear(0xa29c92))
        col = m.mix(mul(m, ramp(m, dinge, 0.45, 0.8), 0.18), col, core.hex_linear(0xb3ab9d))
        col = m.mix(mul(m, low, 0.6), col, core.hex_linear(0x7a7366))
        col = m.mix(mul(m, toe, 0.35), col, core.hex_linear(0x8c8578))
        col = m.mix(mul(m, scuff, 0.4), col, core.hex_linear(0x6f695e))
        rough = add(m, 0.86, mul(m, heather, 0.05), mul(m, low, 0.05))
        rough = mix_float(m, threads, rough, 0.62)
        m.set('Specular IOR Level', 0.35)
        m.set('Sheen Weight', 0.25)
    m.set('Base Color', col)
    m.set('Roughness', rough)
    height = add(m, texture, mul(m, threads, 0.00022), mul(m, grooves, -0.0001), overlap, mul(m, creases, -0.00055),
                 mul(m, fine, -0.00012), mul(m, scuff, -0.00002))
    bump(m, height)
    return m


def rubber_material(name, info, sx):
    """Slot 1: the foxing, toe cap and heel label. The sneaker's off-white vulcanized rubber (light, the game
    may tint it): a knurled band low down, ribs across the toe's bumper, the heel label's raised lettering, yellowed
    and dirty from the ground up, the toe cap scuffed. The work shoe's black cupsole with its welt stitching."""
    st = info['st']
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    U, H = panel_uv(m)
    pid = attr(m, 'pid')
    zr = m.math('SUBTRACT', z, GROUND)
    ground = mul(m, inv(m, ramp(m, zr, 0.003, 0.012)), ramp(m, noise(m, scaled(m, sep, 40.0, 40.0, 120.0), detail=4.0), 0.2, 0.7))
    splash = mul(m, ramp(m, noise(m, o, scale=70.0, detail=3.0), 0.6, 0.7), inv(m, ramp(m, zr, 0.01, 0.03)))
    yellow = noise(m, o, scale=5.0, detail=3.0)
    if st['leather']:
        # The welt: a stitched band round the top of the sidewall, the stitches pressed into it (on the bead, which
        # follows the cupsole's top line down from the heel to the forefoot).
        welt = near(m, attr(m, 'zone'), -0.0033, 0.0021, 0.0003)
        dash = ramp(m, m.math('SINE', mul(m, U, 2 * math.pi / 0.0034)), -0.2, 0.5)
        stitches = mul(m, welt, dash)
        texture = noise(m, o, scale=900.0, detail=2.0)
        # Two grooves molded round the cupsole's wall.
        grooves = add(m, near(m, H, 0.0088, 0.00045, 0.0003), near(m, H, 0.0150, 0.00045, 0.0003))
        col = m.mix(mul(m, texture, 0.3), core.hex_linear(0x121214), core.hex_linear(0x18181b))
        col = m.mix(mul(m, stitches, 0.8), col, core.hex_linear(0x0b0b0c))
        col = m.mix(mul(m, grooves, 0.5), col, core.hex_linear(0x080809))
        col = m.mix(mul(m, ground, 0.6), col, core.hex_linear(0x3e3a33))
        col = m.mix(mul(m, splash, 0.4), col, core.hex_linear(0x4a453c))
        rough = add(m, 0.62, mul(m, texture, 0.08), mul(m, ground, 0.2), mul(m, stitches, -0.1))
        height = add(m, mul(m, stitches, 0.0002), mul(m, welt, -0.00012), mul(m, texture, 0.00001), mul(m, grooves, -0.0003))
    else:
        band = near(m, H, 0.0059, 0.0027, 0.0003)
        p = 0.0026
        d1 = m.math('SINE', mul(m, m.math('ADD', U, H), 2 * math.pi / p))
        d2 = m.math('SINE', mul(m, m.math('SUBTRACT', U, H), 2 * math.pi / p))
        knurl = mul(m, ramp(m, mul(m, d1, d2), -0.15, 0.45), band)
        front = ramp(m, y, 0.196, 0.211)
        bumper = mul(m, mul(m, near(m, H, 0.0162, 0.0055, 0.0004), front), ramp(m, m.math('SINE', mul(m, H, 2 * math.pi / 0.0016)), -0.3, 0.6))
        # The heel label's lettering.
        hl = info['heel_label']
        cx, cy, cz = hl['center']
        letters = print_at(m, info['prints']['heel'], frame_uv(m, sep, (cx * sx, cy, cz), (1.0, 0.0, 0.0), (0.0, 0.0, 1.0), hl['w'], hl['h']))[1]
        letters = mul(m, letters, part_is(m, pid, 2))
        cap = part_is(m, pid, 1)
        pebble = mul(m, noise(m, o, scale=1600.0, detail=1.0), cap)
        scuff = mul(m, ramp(m, noise(m, scaled(m, sep, 140.0, 50.0, 140.0), detail=5.0), 0.655, 0.685), m.math('MAXIMUM', cap, front))
        valleys = add(m, mul(m, inv(m, knurl), band), mul(m, inv(m, bumper), mul(m, front, near(m, H, 0.0162, 0.0055, 0.0004))))
        col = m.mix(mul(m, yellow, 0.5), core.hex_linear(0xe3dfd4), core.hex_linear(0xd9d1bf))
        col = m.mix(mul(m, valleys, 0.35), col, core.hex_linear(0x9d968a))
        col = m.mix(mul(m, ground, 0.75), col, core.hex_linear(0x5f584d))
        col = m.mix(mul(m, splash, 0.45), col, core.hex_linear(0x847c6f))
        col = m.mix(mul(m, scuff, 0.4), col, core.hex_linear(0x958d80))
        rough = add(m, 0.5, mul(m, band, 0.18), mul(m, ground, 0.25), mul(m, scuff, 0.15), mul(m, letters, -0.1))
        height = add(m, mul(m, knurl, 0.00028), mul(m, bumper, 0.00022), mul(m, letters, 0.00035), mul(m, pebble, 0.000015),
                     mul(m, scuff, -0.00004))
    m.set('Base Color', col)
    m.set('Roughness', rough)
    m.set('Specular IOR Level', 0.4)
    bump(m, height)
    return m


def accent_material(name, info, sx):
    """Slot 2 (the game may tint it): the sneaker's foxing stripe (painted rubber), heel tape and pull tab
    (webbing, the brand printed on the tab); the work shoe's back stay (leather)."""
    st = info['st']
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    pid = attr(m, 'pid')
    zr = m.math('SUBTRACT', z, GROUND)
    ground = mul(m, inv(m, ramp(m, zr, 0.012, 0.03)), ramp(m, noise(m, o, scale=60.0, detail=4.0), 0.35, 0.75))
    if st['leather']:
        grain = noise(m, o, scale=2600.0, detail=2.0)
        col = m.mix(mul(m, grain, 0.4), core.hex_linear(0x121214), core.hex_linear(0x1c1c1f))
        col = m.mix(mul(m, ground, 0.6), col, core.hex_linear(0x3a352e))
        rough = add(m, 0.4, mul(m, grain, 0.08), mul(m, ground, 0.25))
        height = mul(m, grain, 0.00002)
    else:
        web = m.math('SINE', mul(m, m.math('ADD', x, mul(m, z, 1.6)), 2 * math.pi / 0.0012))
        web = mul(m, m.math('ADD', web, 1.0), 0.5)
        webbing = m.math('MAXIMUM', part_is(m, pid, 1), part_is(m, pid, 2))
        tab = info['tab']
        ink = print_at(m, info['prints']['tab'], frame_uv(m, sep, (tab['center'][0] * sx, tab['center'][1], tab['center'][2]),
                                                          (1.0, 0.0, 0.0), tab['up'], tab['w'], tab['h']))[1]
        # On the tab's outside only (the print would show through on the side facing the leg).
        g = m.node('ShaderNodeNewGeometry')
        gs = m.node('ShaderNodeSeparateXYZ')
        m.link(g.outputs['Normal'], gs.inputs['Vector'])
        ink = mul(m, mul(m, ink, part_is(m, pid, 2)), ramp(m, mul(m, gs.outputs['Y'], -1.0), 0.15, 0.45))
        col = m.mix(mul(m, web, mul(m, webbing, 0.12)), core.hex_linear(0xd3d3d1), core.hex_linear(0xb9b9b6))
        col = m.mix(ink, col, core.hex_linear(0x1d1d1f))
        col = m.mix(mul(m, ground, 0.5), col, core.hex_linear(0x6a645a))
        rough = mix_float(m, webbing, 0.42, 0.82)
        height = add(m, mul(m, mul(m, web, webbing), 0.00004), mul(m, ink, 0.00003))
    m.set('Base Color', col)
    m.set('Roughness', rough)
    bump(m, height)
    return m


def lining_material(name, info, sx):
    """Slot 3 (not tinted): the dark textile lining, the sockliner with its print, the tongue's woven label, the
    dark of the eyelet holes."""
    st = info['st']
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    pid = attr(m, 'pid')
    knit = mul(m, m.math('ADD', mul(m, m.math('SINE', mul(m, add(m, x, y, z), 2 * math.pi / 0.0011)),
                                       m.math('SINE', mul(m, m.math('SUBTRACT', x, z), 2 * math.pi / 0.0011))), 1.0), 0.5)
    heather = noise(m, o, scale=300.0, detail=3.0)
    lab = info['label']
    lc = (lab['center'][0] * sx, lab['center'][1], lab['center'][2])
    lup = (lab['up'][0] * sx, lab['up'][1], lab['up'][2])
    label_col, label_a = print_at(m, info['prints']['label'], frame_uv(m, sep, lc, (-1.0, 0.0, 0.0), lup, lab['w'], lab['h']))
    is_label = part_is(m, pid, 1)
    ins = info['insole']
    sock_col, sock_a = print_at(m, info['prints']['insole'], frame_uv(m, sep, (ins['center'][0] * sx, ins['center'][1], ins['center'][2]),
                                                                      (1.0, 0.0, 0.0), (0.0, 1.0, 0.0), ins['w'], ins['h']))
    is_sock = part_is(m, pid, 2)
    hole = part_is(m, pid, 3)
    base = m.mix(mul(m, heather, 0.5), core.hex_linear(st['lining'][0]), core.hex_linear(st['lining'][1]))
    col = m.mix(mul(m, sock_a, is_sock), base, core.hex_linear(0x7d7c78))
    col = m.mix(is_label, col, label_col)
    col = m.mix(hole, col, core.hex_linear(0x050505))
    heel_wear = mul(m, mul(m, is_sock, inv(m, ramp(m, y, -0.045, -0.01))), ramp(m, noise(m, o, scale=50.0, detail=3.0), 0.3, 0.7))
    col = m.mix(mul(m, heel_wear, 0.3), col, core.hex_linear(0x3c3a36))
    m.set('Base Color', col)
    m.set('Roughness', add(m, 0.9, mul(m, is_label, -0.15), mul(m, heel_wear, -0.15)))
    m.set('Specular IOR Level', 0.3)
    bump(m, add(m, mul(m, mul(m, knit, inv(m, is_label)), 0.00003), mul(m, mul(m, label_a, is_label), 0.00003),
                mul(m, mul(m, sock_a, is_sock), 0.00004)))
    return m


def laces_material(name, info, sx):
    """Slot 4 (the game may tint it): the laces (flat cotton, or the work shoe's waxed round laces), their aglets,
    and the eyelets (bare metal, or black on the work shoe)."""
    st = info['st']
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    y = sep.outputs['Y']
    U, P = panel_uv(m)
    pid = attr(m, 'pid')
    aglet = part_is(m, pid, 1)
    eyelet = part_is(m, pid, 2)
    # A flat lace's herringbone: chevrons along it; a round lace's twist.
    if st.get('round_laces'):
        # A waxed cord's fine braid: shallow (a deep twist on a 3 mm lace reads as a knobbly rope).
        rib = m.math('SINE', mul(m, m.math('ADD', mul(m, U, 2 * math.pi / 0.0016), mul(m, P, 2 * math.pi * 3)), 1.0))
        rib_h = 0.000018
    else:
        rib_h = 0.00006
        rib = m.math('SINE', mul(m, m.math('ADD', U, mul(m, m.math('ABSOLUTE', m.math('SUBTRACT', m.math('FRACT', mul(m, P, 2.0)), 0.5)), 0.0016)),
                                 2 * math.pi / 0.0011))
    rib = mul(m, m.math('ADD', rib, 1.0), 0.5)
    fuzz = noise(m, o, scale=900.0, detail=3.0)
    dirt = mul(m, ramp(m, noise(m, o, scale=35.0, detail=4.0), 0.35, 0.8), ramp(m, y, 0.04, 0.11))
    lace_hex, metal_hex = st['lace_color'], st['eyelet_color']
    col = m.mix(mul(m, fuzz, 0.3), core.hex_linear(lace_hex), core.hex_linear(street._shade(lace_hex, 0.93)))
    col = m.mix(mul(m, inv(m, rib), 0.25), col, core.hex_linear(street._shade(lace_hex, 0.8)))
    col = m.mix(mul(m, dirt, 0.35), col, core.hex_linear(street._shade(lace_hex, 0.62) if lace_hex > 0x808080 else 0x2a2826))
    col = m.mix(aglet, col, core.hex_linear(street._shade(lace_hex, 0.86)))
    scratch = ramp(m, noise(m, o, scale=2200.0, detail=2.0, distortion=2.0), 0.6, 0.72)
    col = m.mix(eyelet, col, m.mix(mul(m, scratch, 0.4), core.hex_linear(metal_hex), core.hex_linear(street._shade(metal_hex, 1.25))))
    lace_rough = st['lace_rough']
    rough = mix_float(m, aglet, add(m, lace_rough, mul(m, fuzz, 0.05)), 0.28)
    rough = mix_float(m, eyelet, rough, add(m, st['eyelet_rough'], mul(m, scratch, 0.1)))
    m.set('Base Color', col)
    m.set('Roughness', rough)
    m.set('Metallic', mul(m, eyelet, st['eyelet_metal']))
    m.set('Specular IOR Level', mix_float(m, eyelet, 0.35, 0.5))
    bump(m, mul(m, add(m, mul(m, rib, rib_h), mul(m, fuzz, 0.00002 if rib_h > 0.00003 else 0.000006)), inv(m, m.math('MAXIMUM', aglet, eyelet))))
    return m


def outsole_material(name, info, sx):
    """Slot 5 (not tinted): the tread. The sneaker's gum waffle, the work shoe's black slip-resistant lugs; a
    smooth rim, the brand molded in the middle, worn smooth at the heel and the ball, dust in the grooves."""
    st = info['st']
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    x, y = sep.outputs['X'], sep.outputs['Y']
    hu = attr(m, 'hu')
    p = st['tread'][1]
    a = mul(m, m.math('ADD', x, y), 0.7071 / p)
    b = mul(m, m.math('SUBTRACT', x, y), 0.7071 / p)
    fa = m.math('ABSOLUTE', m.math('SUBTRACT', m.math('FRACT', a), 0.5))
    fb = m.math('ABSOLUTE', m.math('SUBTRACT', m.math('FRACT', b), 0.5))
    if st['tread'][0] == 'waffle':
        # Vans-style waffle: square lugs on the diagonal, narrow grooves between.
        lug = less(m, m.math('MAXIMUM', fa, fb), 0.37, 0.025)
    else:
        # Slip-resistant: rounded lugs cut by wavy siping channels across the sole.
        lug = less(m, m.math('MAXIMUM', fa, fb), 0.35, 0.02)
        yw = m.math('ADD', y, mul(m, m.math('ABSOLUTE', m.math('SUBTRACT', m.math('FRACT', mul(m, x, 1.0 / 0.008)), 0.5)), 0.003))
        sipe = near(m, m.math('FRACT', mul(m, yw, 1.0 / 0.0125)), 0.5, 0.035, 0.02)
        lug = mul(m, lug, inv(m, sipe))
    lg = info['logo']
    pad_uv = frame_uv(m, sep, (lg['center'][0] * sx, lg['center'][1], GROUND), (-1.0, 0.0, 0.0), (0.0, 1.0, 0.0), lg['w'], lg['h'])
    logo_col, logo_a = print_at(m, info['prints']['sole'], pad_uv)
    px = m.math('ABSOLUTE', m.math('DIVIDE', m.math('SUBTRACT', x, lg['center'][0] * sx), lg['w'] * 0.58))
    py = m.math('ABSOLUTE', m.math('DIVIDE', m.math('SUBTRACT', y, lg['center'][1]), lg['h'] * 0.66))
    box = m.math('MAXIMUM', px, py)
    pad = less(m, box, 1.0, 0.03)
    border = near(m, box, 0.93, 0.025, 0.015)
    rim = less(m, hu, st['tread'][2], 0.0005)
    # The tread's face is flush everywhere but the grooves: the lugs, the rim round the edge, the logo's pad.
    groove = mul(m, mul(m, inv(m, lug), inv(m, rim)), inv(m, pad))
    letters = mul(m, logo_a, pad)
    wear_zone = m.math('MAXIMUM', inv(m, ramp(m, y, -0.05, -0.025)),
                       mul(m, mul(m, ramp(m, y, 0.105, 0.125), inv(m, ramp(m, y, 0.16, 0.18))), ramp(m, mul(m, x, sx), -0.01, 0.02)))
    wear = mul(m, wear_zone, ramp(m, noise(m, o, scale=40.0, detail=4.0), 0.3, 0.7))
    dust = mul(m, groove, ramp(m, noise(m, o, scale=25.0, detail=4.0), 0.3, 0.8))
    film = ramp(m, noise(m, o, scale=8.0, detail=4.0), 0.4, 0.8)
    base_hex = st['outsole']
    col = m.mix(mul(m, noise(m, o, scale=300.0, detail=2.0), 0.3), core.hex_linear(base_hex), core.hex_linear(street._shade(base_hex, 1.1)))
    col = m.mix(mul(m, groove, 0.6), col, core.hex_linear(street._shade(base_hex, 0.72)))
    col = m.mix(mul(m, dust, 0.3), col, core.hex_linear(0x6a635b))
    col = m.mix(mul(m, film, 0.15), col, core.hex_linear(0x77706a))
    col = m.mix(mul(m, mul(m, wear, inv(m, groove)), 0.3), col, core.hex_linear(street._shade(base_hex, 1.35)))
    col = m.mix(mul(m, letters, 0.35), col, core.hex_linear(street._shade(base_hex, 1.3)))
    m.set('Base Color', col)
    m.set('Roughness', add(m, 0.8, mul(m, wear, -0.18), mul(m, dust, 0.1)))
    m.set('Specular IOR Level', 0.4)
    relief = mul(m, inv(m, groove), m.math('SUBTRACT', 1.0, mul(m, wear, 0.35)))
    bump(m, add(m, mul(m, relief, 0.0009), mul(m, letters, 0.00035), mul(m, border, 0.0003)))
    return m


def materials(info, side):
    sx = 1.0 if side == 'l' else -1.0
    tag = info['st']['tag']
    builders = (upper_material, rubber_material, accent_material, lining_material, laces_material, outsole_material)
    return [b(f'{tag}_{slot}_{side}', info, sx).m for b, slot in zip(builders, SLOTS)]


# ------------------------------------------------------------------ prints

def spade(s, cx, cy, size, color, rough=0.5):
    pts = []
    for k in range(48):
        t = 2 * math.pi * k / 48
        x = 16 * math.sin(t) ** 3
        y = 13 * math.cos(t) - 5 * math.cos(2 * t) - 2 * math.cos(3 * t) - math.cos(4 * t)
        pts.append((cx + x * size / 34, cy + 0.12 * size - y * size / 34))
    s.poly(pts, color, rough=rough)
    s.poly([(cx, cy - 0.05 * size), (cx - 0.22 * size, cy - 0.5 * size), (cx + 0.22 * size, cy - 0.5 * size)], color, rough=rough)


def prints(brand, tag):
    """The brand's prints: the tongue's woven label, the pull tab's ink, the sockliner, the tread's molding, the
    heel label's lettering. Read the right way round on both shoes (mapped by position, after mirroring)."""
    from artkit.sheet import Sheet
    out = {}
    k = min(1.0, 5.5 / len(brand))  # longer names set smaller, to fit the same label
    s = Sheet(260, 100)
    s.rect(0, 0, 260, 100, 0x111113, rough=0.7)
    s.rect(7, 7, 246, 2, 0x55524c, rough=0.7)
    s.rect(7, 91, 246, 2, 0x55524c, rough=0.7)
    spade(s, 34, 50, 38, 0xd8d3c8)
    spade(s, 226, 50, 38, 0xd8d3c8)
    s.text(brand, 130, 38 + (1 - k) * 8, 44 * k, 0xefeae0, face='Black', align='CENTER', tracking=1.12, rough=0.7)
    s.text('NORTHSIDE  ·  EST. 1987', 130, 18, 12, 0xa8a398, face='Bold', align='CENTER', tracking=1.3, rough=0.7)
    out['label'] = s.render(f'{tag}_label', 520)
    s = Sheet(135, 60)
    s.text(brand, 67.5, 22, 30 * k, 0x1a1a1a, face='Black', align='CENTER', tracking=1.05)
    out['tab'] = s.render(f'{tag}_tab', 400)
    s = Sheet(480, 300)
    spade(s, 240, 205, 120, 0x8a8986)
    s.text(brand, 240, 40, 92 * k, 0x8a8986, face='Black', align='CENTER', tracking=1.1)
    out['insole'] = s.render(f'{tag}_insole', 512)
    s = Sheet(500, 220)
    s.text(brand, 250, 92, 120 * k, 0x202020, face='Black', align='CENTER', tracking=1.1)
    s.text('NON-MARKING  ·  SLIP-RESISTANT' if tag == 'work' else 'VULCANIZED  ·  NORTHSIDE', 250, 30, 30, 0x202020, face='Bold', align='CENTER', tracking=1.3)
    out['sole'] = s.render(f'{tag}_sole', 768)
    s = Sheet(240, 80)
    s.text(brand, 120, 22, 46 * k, 0x202020, face='Black', align='CENTER', tracking=1.12)
    out['heel'] = s.render(f'{tag}_heel', 512)
    return out


# ------------------------------------------------------------------ build

def build_parts(st):
    """Every piece of one (left) shoe, and what its materials need to know about where things are."""
    last = Last(st)
    up = Upper(last)
    parts = [up.part(), up.lining(), up.bead()]
    if st.get('toe_cap'):
        parts.append(up.toe_cap(st['toe_cap']))
    slot, pid, lift = st['heel_tape']
    parts.append(up.heel_tape(lift=lift, slot=slot, pid=pid))
    info = dict(st=st, last=last, up=up, ridge=0.005, creases=st['creases'])
    if st.get('pull_tab'):
        tab, frame = pull_tab(up)
        parts.append(tab)
        info['tab'] = frame
    parts.append(sole_part(last, st))
    if st.get('heel_label'):
        hl, frame = heel_label(last, st)
        parts.append(hl)
        info['heel_label'] = frame
    parts.append(insole_part(last))
    tongue = Tongue(last, up, st)
    parts += tongue.parts()
    info['label'] = tongue.label_frame
    parts += eyelets(last, up, st)
    parts += Laces(last, up, tongue, st).parts()
    yi, yl = -0.034, 0.062
    info['insole'] = dict(center=(float(last.xc(yi)), yi, last.base + 0.0022), w=0.046, h=0.0288)
    info['logo'] = dict(center=(float(last.xc(yl)), yl), w=0.058, h=0.0255)
    return info, parts


def finish(parts, name, mats, st, last):
    dbg = debug_parts(parts)
    if dbg:
        for p in parts:
            p.slots = [0] * len(p.faces)
        objs = [to_object(p, [dbg[p.name]]) for p in parts]
    else:
        objs = [to_object(p, mats) for p in parts]
    obj = core.join(objs, name)
    me = obj.data
    # The toe spring: the forefoot turned up from the ball.
    co = np.empty(len(me.vertices) * 3)
    me.vertices.foreach_get('co', co)
    co = co.reshape(-1, 3)
    t = np.clip((co[:, 1] - st['spring_from']) / (last.y1 + 0.002 - st['spring_from']), 0.0, None)
    co[:, 2] += st['toe_spring'] * t * t
    me.vertices.foreach_set('co', co.ravel())
    bm = bmesh.new()
    bm.from_mesh(me)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-7)
    bm.to_mesh(me)
    bm.free()
    me.shade_smooth()
    try:
        me.set_sharp_from_angle(angle=math.radians(50.0))
    except AttributeError:
        pass
    me.update()
    if not dbg:
        unwrap(obj)
        pack_uvs(obj)
    return obj


def mirror(obj, name, mats):
    """The other foot: the mesh mirrored across X (its UVs kept), with its own materials to bake."""
    me = obj.data.copy()
    me.name = name
    me.transform(Matrix.Scale(-1.0, 4, (1.0, 0.0, 0.0)))
    me.flip_normals()
    for i, m in enumerate(mats):
        me.materials[i] = m
    return core.link(bpy.data.objects.new(name, me))


def debug_parts(parts):
    """SNK_PARTS=1: each kind of part in its own color (for checking the geometry)."""
    import colorsys
    import os
    if not os.environ.get('SNK_PARTS'):
        return None
    names = sorted({p.name for p in parts})
    mats = {}
    for k, nm in enumerate(names):
        r, g, b = colorsys.hsv_to_rgb(k / len(names), 0.55, 0.9)
        m = core.Mat('dbg_' + nm)
        m.set('Base Color', (r, g, b, 1.0))
        m.set('Roughness', 0.6)
        mats[nm] = m.m
    print('[pv] parts:', ', '.join(f'{nm}={k}/{len(names)}' for k, nm in enumerate(names)))
    return mats


def build():
    import os
    core.reset()
    out = []
    only = os.environ.get('SNK_ONLY')
    for st in (SNEAKER, WORK):
        if only and only != st['tag']:
            continue
        info, parts = build_parts(st)
        info['prints'] = prints(st['brand'], st['tag'])
        left = finish(parts, st['name'] + '_L', materials(info, 'l'), st, info['last'])
        out.append(left)
        if not os.environ.get('SNK_PARTS'):
            out.append(mirror(left, st['name'] + '_R', materials(info, 'r')))
    return out


def assemble(objs):
    """After the bake: drop what only the procedural materials read (the panel UV, the part attributes)."""
    for o in objs:
        me = o.data
        if 'panel' in me.uv_layers:
            me.uv_layers.remove(me.uv_layers['panel'])
        for a in ('pid', 'hu', 'zone', 'uvw', 'seamv', 'uvmode'):
            if a in me.attributes:
                me.attributes.remove(me.attributes[a])
    return objs


# Where the review stands them: the sneakers in front, the work shoes behind, toes turned out a little.
REVIEW_PLACES = {'SM_Sneaker_L': (-0.068, 0.20, 6.0), 'SM_Sneaker_R': (0.068, 0.20, -6.0),
                 'SM_WorkShoe_L': (-0.07, -0.22, 6.0), 'SM_WorkShoe_R': (0.07, -0.22, -6.0)}


# The review shows the sneakers as the game dresses them by default: their accent (stripe, heel tape, pull tab)
# multiplied by the outfit's red, as Shoe slot 2's tint would (the exported files are untouched).
REVIEW_TINTS = {'SM_Sneaker': {ACCENT: (0x8f1d2c, 1.45)}}


def _tint(mat, hexc, lift):
    nt = mat.node_tree
    bsdf = next(n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED')
    if not bsdf.inputs['Base Color'].is_linked:
        return
    src = bsdf.inputs['Base Color'].links[0].from_socket
    mix = nt.nodes.new('ShaderNodeMix')
    mix.data_type = 'RGBA'
    mix.blend_type = 'MULTIPLY'
    mix.inputs[0].default_value = 1.0
    nt.links.new(src, mix.inputs[6])
    r, g, b, _ = core.hex_linear(hexc)
    mix.inputs[7].default_value = (r * lift, g * lift, b * lift, 1.0)
    nt.links.new(mix.outputs[2], bsdf.inputs['Base Color'])


def pose_for_review(objs):
    for o in objs:
        x, y, a = REVIEW_PLACES.get(o.name, (0.0, 0.0, 0.0))
        o.location = (x, y, 0.0)
        o.rotation_mode = 'XYZ'
        o.rotation_euler = (0.0, 0.0, math.radians(a))
        for prefix, tints in REVIEW_TINTS.items():
            if o.name.startswith(prefix):
                for slot, (hexc, lift) in tints.items():
                    if slot < len(o.material_slots) and o.material_slots[slot].material:
                        _tint(o.material_slots[slot].material, hexc, lift)
    bpy.context.view_layer.update()
