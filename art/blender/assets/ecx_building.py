"""SM_ECX_*: the Embercrest resort casino's building, seen from its arrival plaza (/Game/Maps/Embercrest).

Built to the exterior spec (ec_exterior_spec.md, section 3.1): the entrance pavilion and its glass, the lobby behind it,
the casino (south) and grill / poker room (north) wings, the porte-cochere, the hotel tower with its crown of three
lit peaks and the crest on it, the LED marquee, the doors, and every glow line the game colors at runtime.

Frames (spec 0.2). SITE meshes: Blender origin = the map origin (the doors' threshold), the building's front faces
Blender -X, Z up, meters; placed in Unreal at (0,0,0) yaw 180, so UE (X, Y, Z) cm = (-100x, +100y, +100z). The code
works in MAP meters (X out of the building toward the plaza, Y north, Z up) and converts with V(): Blender
(x, y, z) = (-X, Y, Z). PROP meshes (the revolving door, its wings, the swing door leaf) are built in their own frame.

Choices made inside the spec's ranges or where it is silent (section 0 asks for them here):
- The porte-cochere's slat soffit and roof start 1.0 m off the entrance glass (map X 1.0); the steel beam spans that
  gap (X 0.98..1.42), so the larch fins (X 0.38..0.95) stand clear of the canopy all the way up. The fins run from
  Z 7.0 to the cornice soffit at 12.0 (the cornice projects to X 1.05 over them).
- The canopy roof folds along its center line (a 0.22 m valley) as it rises from 8.4 m at the back to 9.9 m at the
  front; snow 0.18-0.24 m deep with a rolled lip 0.12 m over the fascias.
- The wings' LED reveals run through the slot windows and the grill storefront as bronze transoms at the same height.
- The podium behind the lobby and wings (map X -18.5/-30 .. -60, Z 0..16.6), which the contract leaves to no one, is in
  SM_ECX_Tower (its basalt and roof slots, low texel density), so the tower stands on something and no reachable
  camera sees into the building. SM_ECX_Lobby carries the lobby's own roof (Z 12.8).
- The lobby mezzanine's glass rail is in SM_ECX_Podium_Glass with the rest of the podium glazing.
- The crown: three faceted pyramids (apexes at X -41, spec heights and Ys, valleys where the mark puts them) on a 4 m
  vertical fin band (Z 62..66); bronze ribs follow every constant-Y section of the faceted envelope at 0.6 m pitch
  (they read as vertical fins from the plaza and the city), on a dark steel frame. The band stays dark: its glow is
  the mark's ground line along its top (parted by the crest on the front), in SM_ECX_Crown_GlowEmber with the peaks'
  edge lines and the inner glow facets behind the ribs.
- SM_ECX_Podium_GlowEmber also carries the rear podium's two reveal lines (they continue the wings' returns).
- Flat roofs (the wings', the lobby's, the entrance's) are snow-covered in the basalt bake (ecx_k 1); the rear podium
  roof is gravel ballast (roof slot); the tower roof under the crown is pavers with plant.
- Lit slots bake their interiors' emission at strengths 1.6 (hotel rooms), 2.2 (casino), 2.0 (grill), 2.5 (lobby),
  3.0 (poker room, the grill's halo, the wine room), 2.2 (the crest's ember): scale them with DimImportedGlow.
- Art-direction pass: the canopy's front fascia is clad in 20 bronze panels (1.82 m, 10 mm shadow gaps over a
  blackened backing, each panel its own patina via ecx_ptone) with a top band under the coping; the side fascias stop
  behind the front skin. The north wing's blank ground floor under the terrace (Y 34..56) carries the Ember Grill's
  wine room: five deep bronze-boxed windows (1.6 x 3.4 m, Y 38.25..51.85 at 3.4 m, Z 0.95..4.35) whose dark glass
  bakes a backlit wall of bottles (WingN slot 3, look 2), each with a warm lens under its head (Podium_GlowWarm). The
  grill's diners are rounded figures in their own clothes, skin and hair. The crest's ground is midnight-navy enamel.
  The door props' [glass] slots bake as dark clear glass (a missed M_ECX_Glass swap still reads as glass). Bronze's
  hand-height wear is a soft rubbed polish, not speckle.
- Also written: unreal/Art/Textures/ECX/T_ECX_MarqueeBg.png and T_ECX_MarqueeSeries.png (2048 x 768, brand.py).

Geometry: stone is clad in real 1.2 x 0.6 m panels (rounded rims, 6 mm joints over a dark backing, a hair of lippage,
a tone each), so the joints read at 1 m whatever the texel density; hard-surface parts have rounded edges (MB._rounded).
Printed text follows spec 0.3 (it reads correctly in Blender from outside its face, so it does in the game).
ECX_ONLY=Entry,Canopy (environment) builds just those meshes (short names), for iterating.
"""
import math
import os
import random
import sys

import bmesh
import bpy  # noqa: F401
import numpy as np
from mathutils import Matrix, Vector

from artkit import core, parts
from artkit.shade import convex_edges, mix_float, noise, object_coords, ramp, scaled, vec
from artkit.sheet import Sheet

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import brand  # noqa: E402

ALL = ['SM_ECX_Entry', 'SM_ECX_Podium_Glass', 'SM_ECX_Lobby', 'SM_ECX_Lobby_GlowWarm', 'SM_ECX_WingS', 'SM_ECX_WingN', 'SM_ECX_Canopy',
       'SM_ECX_Podium_GlowEmber', 'SM_ECX_Podium_GlowWarm', 'SM_ECX_Podium_GlowAsh', 'SM_ECX_RevolvingDoor', 'SM_ECX_RevolvingDoor_Wings',
       'SM_ECX_SwingDoor', 'SM_ECX_Tower', 'SM_ECX_Tower_GlowEmber', 'SM_ECX_Crown', 'SM_ECX_Crown_GlowEmber', 'SM_ECX_CrownCrest',
       'SM_ECX_Marquee', 'SM_ECX_Marquee_Screen']
_ONLY = [s.strip() for s in os.environ.get('ECX_ONLY', '').split(',') if s.strip()]
MESHES = [m for m in ALL if not _ONLY or m.replace('SM_ECX_', '') in _ONLY]
UNBAKED = {'SM_ECX_Podium_Glass', 'SM_ECX_Marquee_Screen'}

DOUBLE_SIDED = False
TEXTURE_SIZE = 2048
AO_DISTANCE = 0.45
REVIEW_SCREEN_LIGHT = False
REVIEW_VIEWS = None   # set by pose_for_review
TEXTURE_SIZES = {
    'en_basalt': 4096, 'en_bronze': 2048, 'en_timber': 2048, 'en_granite': 1024, 'en_frame': 1024,
    'lb_floor': 4096, 'lb_basalt': 2048, 'lb_bronze': 2048, 'lb_timber': 2048, 'lb_leather': 2048, 'lb_lit': 2048,
    'ws_basalt': (4096, 2048), 'ws_bronze': 2048, 'ws_glass': 2048, 'ws_granite': 1024,
    'wn_basalt': (4096, 2048), 'wn_bronze': 2048, 'wn_timber': 2048, 'wn_glass': 2048, 'wn_lit': 2048, 'wn_prints': 1024,
    'cn_timber': 4096, 'cn_bronze': 4096, 'cn_basalt': 2048, 'cn_roof': 1024, 'cn_snow': 2048, 'cn_ice': 512, 'cn_frame': 1024,
    'rd_bronze': 2048, 'rd_glass': 128, 'rd_floor': 1024, 'rd_frame': 1024,
    'rw_bronze': 1024, 'rw_glass': 128, 'rw_rubber': 256,
    'sd_bronze': 1024, 'sd_glass': 128, 'sd_frame': 512,
    'tw_basalt': (4096, 2048), 'tw_bronze': 2048, 'tw_glass': 4096, 'tw_roof': 1024,
    'cr_bronze': 2048, 'cr_frame': 1024, 'cr_roof': 1024,
    'cc_peaks': 2048, 'cc_ember': 2048, 'cc_bronze': 2048,
    'mq_bronze': 2048, 'mq_frame': 1024, 'mq_basalt': 1024,
}
TEX_DIR = os.path.join(core.ROOT, 'unreal', 'Art', 'Textures', 'ECX')
RNG = random.Random(2100)
# Bevels: chamfers of at least min_round meters are rounded with seg segments; stone panels round their rims too.
DETAIL = {"seg": 2, "min_round": 0.005, "panel_seg": 3}

# Brand and game colors (spec 0.5).
EMBER_LED = 0xff6a24
WARM = 0xffc98a
ASH = brand.ASH


def V(X, Y, Z):
    """Map meters (X out of the building, Y north, Z up) -> Blender meters."""
    return Vector((-X, Y, Z))


def VN(nx, ny, nz):
    """A direction in map axes -> Blender axes."""
    return Vector((-nx, ny, nz))


def hx(h):
    return core.hex_linear(h)


# ------------------------------------------------------------------ the mesh builder

class MB:
    """Builds a mesh fast from Python lists: shared vertices per part, per-face attributes, one object at the end.
    Points are Blender coordinates (call sites convert map coordinates with V)."""

    def __init__(self):
        self.verts = []
        self.faces = []
        self.fattr = {}     # name -> list of values, one per face
        self.cur = {}       # attribute values stamped on new faces

    def set(self, **kw):
        self.cur.update(kw)
        return self

    def v(self, p):
        self.verts.append((float(p[0]), float(p[1]), float(p[2])))
        return len(self.verts) - 1

    def f(self, idx, center=None, want=None, **attrs):
        """A face from vertex indices; oriented away from center, or toward the direction want."""
        if center is not None or want is not None:
            pts = [Vector(self.verts[i]) for i in idx]
            n = Vector((0.0, 0.0, 0.0))
            for a, b in zip(pts, pts[1:] + pts[:1]):
                n.x += (a.y - b.y) * (a.z + b.z)
                n.y += (a.z - b.z) * (a.x + b.x)
                n.z += (a.x - b.x) * (a.y + b.y)
            if center is not None:
                c = sum(pts, Vector()) / len(pts)
                d = c - Vector(center)
            else:
                d = Vector(want)
            if n.dot(d) < 0.0:
                idx = list(reversed(idx))
        k = len(self.faces)
        self.faces.append(list(idx))
        vals = dict(self.cur)
        vals.update(attrs)
        for name, val in vals.items():
            if name not in self.fattr:
                self.fattr[name] = [None] * k
            self.fattr[name].append(val)
        for name, lst in self.fattr.items():
            if len(lst) < k + 1:
                lst.append(None)
        return k

    def poly(self, pts, want=None, center=None, **attrs):
        return self.f([self.v(p) for p in pts], center=center, want=want, **attrs)

    def quad_map(self, pts_map, want_map, **attrs):
        """A face from map-coordinate points, facing the map direction want_map."""
        return self.poly([V(*p) for p in pts_map], want=VN(*want_map), **attrs)

    def obox(self, center, axes, half, cham=0.0, skip=(), seg=None, **attrs):
        """An oriented box (Blender coords): center, three unit axes, half sizes, every edge chamfered by cham.
        skip: faces to leave out, as (axis index, sign) pairs (a side pressed against something)."""
        c = Vector(center)
        ax = [Vector(a) for a in axes]
        h = list(half)
        cm = min(cham, min(h) * 0.45)

        def P(s, o):
            return c + ax[0] * (s[0] * o[0]) + ax[1] * (s[1] * o[1]) + ax[2] * (s[2] * o[2])
        if cm <= 1e-6:
            idx = {}
            for sx in (-1, 1):
                for sy in (-1, 1):
                    for sz in (-1, 1):
                        idx[(sx, sy, sz)] = self.v(P((sx, sy, sz), h))
            for a in range(3):
                for s in (-1, 1):
                    if (a, s) in skip:
                        continue
                    b, d = (a + 1) % 3, (a + 2) % 3
                    ring = []
                    for (u, w) in ((-1, -1), (1, -1), (1, 1), (-1, 1)):
                        key = [0, 0, 0]
                        key[a], key[b], key[d] = s, u, w
                        ring.append(idx[tuple(key)])
                    self.f(ring, center=c, **attrs)
            return
        if seg is None:
            seg = DETAIL['seg'] if cm >= DETAIL['min_round'] else 1
        if seg >= 2:
            self._rounded(c, ax, h, cm, seg, skip, attrs)
            return
        vi = {}
        for sx in (-1, 1):
            for sy in (-1, 1):
                for sz in (-1, 1):
                    s = (sx, sy, sz)
                    vi[(s, 0)] = self.v(P(s, (h[0], h[1] - cm, h[2] - cm)))
                    vi[(s, 1)] = self.v(P(s, (h[0] - cm, h[1], h[2] - cm)))
                    vi[(s, 2)] = self.v(P(s, (h[0] - cm, h[1] - cm, h[2])))
        for a in range(3):
            for sgn in (-1, 1):
                if (a, sgn) in skip:
                    continue
                b, d = (a + 1) % 3, (a + 2) % 3
                ring = []
                for (u, w) in ((-1, -1), (1, -1), (1, 1), (-1, 1)):
                    key = [0, 0, 0]
                    key[a], key[b], key[d] = sgn, u, w
                    ring.append(vi[(tuple(key), a)])
                self.f(ring, center=c, **attrs)
        for a in range(3):
            b, d = (a + 1) % 3, (a + 2) % 3
            for sb in (-1, 1):
                for sd in (-1, 1):
                    k0 = [0, 0, 0]
                    k1 = [0, 0, 0]
                    k0[a], k0[b], k0[d] = -1, sb, sd
                    k1[a], k1[b], k1[d] = 1, sb, sd
                    self.f([vi[(tuple(k0), b)], vi[(tuple(k0), d)], vi[(tuple(k1), d)], vi[(tuple(k1), b)]], center=c, **attrs)
        for sx in (-1, 1):
            for sy in (-1, 1):
                for sz in (-1, 1):
                    s = (sx, sy, sz)
                    self.f([vi[(s, 0)], vi[(s, 1)], vi[(s, 2)]], center=c, **attrs)

    def _rounded(self, c, ax, h, r, s, skip, attrs):
        """A box with every edge rounded (radius r, s segments per quarter): a lattice on each face pushed out onto the
        rounded shape (flat middles, cylindrical edges, spherical corners), vertices shared along the seams."""
        hi = [hh - r for hh in h]
        th = [math.tan((math.pi / 4) * i / s) for i in range(1, s + 1)]

        def samples(k):
            pos = [hi[k] + r * t for t in th]
            return [-p for p in reversed(pos)] + [-hi[k], hi[k]] + pos
        cache = {}

        def vert(p):
            key = (round(p[0], 7), round(p[1], 7), round(p[2], 7))
            if key in cache:
                return cache[key]
            q = [min(max(p[i], -hi[i]), hi[i]) for i in range(3)]
            d = [p[i] - q[i] for i in range(3)]
            L = math.sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2])
            if L > 1e-12:
                q = [q[i] + d[i] * r / L for i in range(3)]
            idx = self.v(c + ax[0] * q[0] + ax[1] * q[1] + ax[2] * q[2])
            cache[key] = idx
            return idx
        for a in range(3):
            for sgn in (-1, 1):
                if (a, sgn) in skip:
                    continue
                b, d = (a + 1) % 3, (a + 2) % 3
                Sb, Sd = samples(b), samples(d)
                grid = []
                for yb in Sb:
                    row = []
                    for zd in Sd:
                        p = [0.0, 0.0, 0.0]
                        p[a], p[b], p[d] = sgn * h[a], yb, zd
                        row.append(vert(p))
                    grid.append(row)
                for i in range(len(Sb) - 1):
                    for j in range(len(Sd) - 1):
                        self.f([grid[i][j], grid[i + 1][j], grid[i + 1][j + 1], grid[i][j + 1]], center=c, **attrs)

    def box(self, X0, X1, Y0, Y1, Z0, Z1, cham=0.0, skip=(), seg=None, **attrs):
        """An axis-aligned box from MAP coordinates (skip uses Blender axes: (0, +1) is the map -X side)."""
        self.obox(V((X0 + X1) / 2, (Y0 + Y1) / 2, (Z0 + Z1) / 2), ((1, 0, 0), (0, 1, 0), (0, 0, 1)),
                  (abs(X1 - X0) / 2, abs(Y1 - Y0) / 2, abs(Z1 - Z0) / 2), cham, skip, seg, **attrs)

    def bbox(self, x0, x1, y0, y1, z0, z1, cham=0.0, skip=(), seg=None, **attrs):
        """An axis-aligned box from BLENDER coordinates (props)."""
        self.obox(((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), ((1, 0, 0), (0, 1, 0), (0, 0, 1)),
                  (abs(x1 - x0) / 2, abs(y1 - y0) / 2, abs(z1 - z0) / 2), cham, skip, seg, **attrs)

    def beam(self, a, b, w, h, up=(0.0, 0.0, 1.0), cham=0.0, seg=None, **attrs):
        """A rectangular member from a to b (Blender coords), w wide and h tall about its axis."""
        a, b = Vector(a), Vector(b)
        d = b - a
        L = d.length
        t = d / L
        u = Vector(up)
        if abs(t.dot(u)) > 0.98:
            u = Vector((1.0, 0.0, 0.0)) if abs(t.x) < 0.9 else Vector((0.0, 1.0, 0.0))
        s = t.cross(u).normalized()
        n = s.cross(t).normalized()
        self.obox((a + b) / 2, (t, s, n), (L / 2, w / 2, h / 2), cham, (), seg, **attrs)

    def tube(self, a, b, r0, r1=None, n=12, caps=True, phase=0.0, **attrs):
        """A (tapered) cylinder from a to b, Blender coords, smooth sides."""
        r1 = r0 if r1 is None else r1
        a, b = Vector(a), Vector(b)
        t = (b - a).normalized()
        ref = Vector((0, 0, 1)) if abs(t.z) < 0.9 else Vector((1, 0, 0))
        u = t.cross(ref).normalized()
        w = t.cross(u)
        ra, rb = [], []
        for i in range(n):
            ang = phase + 2 * math.pi * i / n
            d = u * math.cos(ang) + w * math.sin(ang)
            ra.append(self.v(a + d * r0))
            rb.append(self.v(b + d * r1))
        c = (a + b) / 2
        for i in range(n):
            j = (i + 1) % n
            self.f([ra[i], ra[j], rb[j], rb[i]], center=c, **attrs)
        if caps:
            self.f(ra, want=-t, **attrs)
            self.f(rb, want=t, **attrs)

    def lathe(self, profile, center=(0.0, 0.0, 0.0), n=32, a0=0.0, a1=2 * math.pi, **attrs):
        """Revolves (r, z) points (bottom to top) about the vertical through center (Blender); outward normals for a
        profile climbing the outside. A partial sweep (a0..a1) leaves its ends open."""
        cx, cy, cz = center
        full = abs((a1 - a0) - 2 * math.pi) < 1e-6
        steps = n if full else n + 1
        rings = []
        for r, z in profile:
            if r <= 1e-7:
                rings.append([self.v((cx, cy, cz + z))])
            else:
                rings.append([self.v((cx + r * math.cos(a0 + (a1 - a0) * i / n), cy + r * math.sin(a0 + (a1 - a0) * i / n), cz + z))
                              for i in range(steps)])
        for j in range(len(rings) - 1):
            A, B = rings[j], rings[j + 1]
            for i in range(n):
                i1 = (i + 1) % steps if full else i + 1
                ang = a0 + (a1 - a0) * (i + 0.5) / n
                # Orient by the profile's own outward direction (left of travel up the profile).
                dr = profile[j + 1][0] - profile[j][0]
                dz = profile[j + 1][1] - profile[j][1]
                out = Vector((math.cos(ang) * dz, math.sin(ang) * dz, -dr))
                if len(A) == 1 and len(B) == 1:
                    continue
                if len(A) == 1:
                    self.f([A[0], B[i1], B[i]], want=out, **attrs)
                elif len(B) == 1:
                    self.f([A[i], A[i1], B[0]], want=out, **attrs)
                else:
                    self.f([A[i], A[i1], B[i1], B[i]], want=out, **attrs)

    def merge(self, other):
        """Appends another builder's geometry."""
        off = len(self.verts)
        self.verts += other.verts
        k0 = len(self.faces)
        for fc in other.faces:
            self.faces.append([i + off for i in fc])
        names = set(self.fattr) | set(other.fattr)
        for name in names:
            mine = self.fattr.setdefault(name, [None] * k0)
            if len(mine) < k0:
                mine += [None] * (k0 - len(mine))
            mine += other.fattr.get(name, [None] * len(other.faces))

    def obj(self, name):
        me = bpy.data.meshes.new(name)
        me.from_pydata(self.verts, [], self.faces)
        me.update()
        n = len(self.faces)
        for aname, vals in self.fattr.items():
            vals = vals + [None] * (n - len(vals))
            is_vec = any(isinstance(v, (tuple, list, Vector)) for v in vals if v is not None)
            if is_vec:
                a = me.attributes.new(aname, 'FLOAT_VECTOR', 'FACE')
                flat = []
                for v in vals:
                    flat += list(v) if v is not None else [0.0, 0.0, 0.0]
                a.data.foreach_set('vector', flat)
            else:
                a = me.attributes.new(aname, 'FLOAT', 'FACE')
                a.data.foreach_set('value', [float(v) if v is not None else 0.0 for v in vals])
        o = bpy.data.objects.new(name, me)
        core.link(o)
        return o


def set_attr(obj, name, value):
    """A per-face attribute on every face of obj (float or 3-vector)."""
    me = obj.data
    n = len(me.polygons)
    is_vec = isinstance(value, (tuple, list, Vector))
    a = me.attributes.get(name)
    if a is None:
        a = me.attributes.new(name, 'FLOAT_VECTOR' if is_vec else 'FLOAT', 'FACE')
    if is_vec:
        a.data.foreach_set('vector', [float(value[i % 3]) for i in range(n * 3)])
    else:
        a.data.foreach_set('value', [float(value)] * n)


def face_attr(obj, name):
    a = obj.data.attributes.get(name)
    n = len(obj.data.polygons)
    if a is None:
        return np.zeros(n, dtype=np.float32)
    out = np.empty(n, dtype=np.float32)
    a.data.foreach_get('value', out)
    return out


# ------------------------------------------------------------------ a mesh under construction: slots, walls, UVs

def tex_px(mat_name):
    s = TEXTURE_SIZES.get(mat_name, TEXTURE_SIZE)
    return (s, s) if isinstance(s, int) else tuple(s)


def pack_rects(items, region, px, gap_px=4.0):
    """Shelf-packs walls [(w_m, h_m, weight)] into region (u0, v0, u1, v1) of a px = (W, H) texture at one texel size
    per meter times each weight (square texels). Returns ([(u0, v0, u1, v1, rotated)], px per meter)."""
    W, H = px
    rx0, ry0, rx1, ry1 = region[0] * W, region[1] * H, region[2] * W, region[3] * H
    RW, RH = rx1 - rx0, ry1 - ry0

    def dims(i, s):
        w, h, k = items[i]
        a, b = w * s * k, h * s * k
        return (b, a, True) if b > a * 1.05 else (a, b, False)

    def attempt(s):
        order = sorted(range(len(items)), key=lambda i: -dims(i, s)[1])
        x = y = shelf = 0.0
        pos = [None] * len(items)
        for i in order:
            w, h, rot = dims(i, s)
            if w > RW:
                return None
            if x + w > RW:
                y += shelf + gap_px
                x = shelf = 0.0
            if y + h > RH:
                return None
            pos[i] = (x, y, w, h, rot)
            x += w + gap_px
            shelf = max(shelf, h)
        return pos
    lo, hi = 0.0, 8192.0
    for _ in range(50):
        mid = (lo + hi) / 2
        if attempt(mid) is not None:
            lo = mid
        else:
            hi = mid
    pos = attempt(lo)
    out = []
    for x, y, w, h, rot in pos:
        out.append(((rx0 + x) / W, (ry0 + y) / H, (rx0 + x + w) / W, (ry0 + y + h) / H, rot))
    return out, lo


class Kit:
    """One baked mesh: parts per slot (in the contract's slot order), planar walls, box-packed rest, low-detail faces.

    Face attributes the UV pass reads: ecx_wall (a planar wall's id), ecx_low (packed small: undersides, far
    returns, roofs), ecx_joint (panel joints and backings: they all sample one baked patch per slot)."""

    def __init__(self, name, mats):
        self.name = name
        self.mats = mats
        self.mbs = {}
        self.objs = []
        self.walls = []
        self.region = {}
        self.angle = 32.0

    def mb(self, slot):
        if slot not in self.mbs:
            self.mbs[slot] = MB()
        return self.mbs[slot]

    def add(self, obj, slot, **attrs):
        for k, v in attrs.items():
            set_attr(obj, k, v)
        obj['ecx_slot'] = slot
        self.objs.append(obj)
        return obj

    def new_wall(self, slot, frame, rect, weight=1.0):
        wid = len(self.walls) + 1
        self.walls.append(dict(id=wid, slot=slot, frame=frame, rect=rect, weight=weight))
        return wid

    def finish(self):
        mats = [m.m if isinstance(m, core.Mat) else m for m in self.mats]
        parts_ = []
        for slot, mb in sorted(self.mbs.items()):
            if not mb.faces:
                continue
            o = mb.obj(f'{self.name}_{slot}')
            o['ecx_slot'] = slot
            parts_.append(o)
        parts_ += self.objs
        # Every slot needs at least one face, or the slot order breaks: a sliver under the floor stands in.
        used = {int(o['ecx_slot']) for o in parts_}
        for s in range(len(mats)):
            if s not in used:
                mb = MB()
                mb.poly([V(-0.2, 0.0, -0.02), V(-0.2, 0.05, -0.02), V(-0.25, 0.05, -0.02), V(-0.25, 0.0, -0.02)], want=(0, 0, -1), ecx_low=1)
                o = mb.obj(f'{self.name}_pad{s}')
                o['ecx_slot'] = s
                parts_.append(o)
        # Joint patches: one baked square per slot that has joints, which every joint face samples.
        for s in range(len(mats)):
            has_joint = any(int(o['ecx_slot']) == s and face_attr(o, 'ecx_joint').max(initial=0.0) > 0.5 for o in parts_)
            if has_joint:
                mb = MB()
                mb.poly([V(-0.3, 0.3, -0.03), V(-0.3, 0.6, -0.03), V(-0.6, 0.6, -0.03), V(-0.6, 0.3, -0.03)], want=(0, 0, -1), ecx_joint=1, ecx_patch=1)
                o = mb.obj(f'{self.name}_patch{s}')
                o['ecx_slot'] = s
                parts_.append(o)
        for o in parts_:
            s = int(o['ecx_slot'])
            o.data.materials.clear()
            for mt in mats:
                o.data.materials.append(mt)
            o.data.polygons.foreach_set('material_index', [s] * len(o.data.polygons))
        obj = core.join(parts_, self.name) if len(parts_) > 1 else parts_[0]
        obj.name = obj.data.name = self.name
        core.finish_hard_surface(obj, self.angle)
        self.layout_uvs(obj)
        tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
        print(f'[ecx_building] {self.name}: {tris} triangles; slots ' + ', '.join(f'{i} {s.material.name}' for i, s in enumerate(obj.material_slots)))
        return obj

    def layout_uvs(self, obj):
        me = obj.data
        n = len(me.polygons)
        mi = np.empty(n, dtype=np.int32)
        me.polygons.foreach_get('material_index', mi)
        wall = face_attr(obj, 'ecx_wall').astype(np.int32)
        low = face_attr(obj, 'ecx_low') > 0.5
        joint = face_attr(obj, 'ecx_joint') > 0.5
        patch = face_attr(obj, 'ecx_patch') > 0.5
        mats = [s.material.name for s in obj.material_slots]
        regions = {}
        groups = []
        for s, mname in enumerate(mats):
            px = tex_px(mname)
            has_walls = any(w['slot'] == s for w in self.walls)
            has_low = bool(np.any((mi == s) & (low | patch) & (wall == 0)))
            reg = self.region.get(s)
            if reg is None:
                if has_walls:
                    reg = {'walls': (0.0, 0.0, 1.0, 0.8), 'rest': (0.0, 0.805, 0.86 if has_low else 1.0, 1.0), 'low': (0.865, 0.805, 1.0, 1.0)}
                else:
                    reg = {'rest': (0.0, 0.0, 0.9 if has_low else 1.0, 1.0), 'low': (0.905, 0.0, 1.0, 1.0)}
            regions[s] = (reg, px)
            gap = 3.0 / px[0]
            rest_idx = set(np.nonzero((mi == s) & (wall == 0) & ~low & ~joint & ~patch)[0].tolist())
            low_idx = set(np.nonzero((mi == s) & (wall == 0) & (~joint | patch) & (low | patch))[0].tolist())
            groups.append((lambda f, r=rest_idx: f.index in r, 'box', gap, reg['rest']))
            groups.append((lambda f, r=low_idx: f.index in r, 'box', gap, reg['low']))
        core.uv_layout(obj, groups)
        # Planar walls, packed per slot.
        bm = bmesh.new()
        bm.from_mesh(me)
        bm.faces.ensure_lookup_table()
        uv = bm.loops.layers.uv.verify()
        for s in range(len(mats)):
            ws = [w for w in self.walls if w['slot'] == s]
            if not ws:
                continue
            reg, px = regions[s]
            items = [((w['rect'][1] - w['rect'][0]), (w['rect'][3] - w['rect'][2]), w['weight']) for w in ws]
            rects, dens = pack_rects(items, reg['walls'], px)
            print(f'[ecx_building] {self.name} slot {s}: {len(ws)} walls at {dens:.0f} px/m (weight 1)')
            byid = {w['id']: (w, r) for w, r in zip(ws, rects)}
            for f in bm.faces:
                wid = int(wall[f.index])
                if wid == 0 or wid not in byid:
                    continue
                w, (a0, b0, a1, b1, rot) = byid[wid]
                O, U, Vv, N = w['frame']
                u0, u1, v0, v1 = w['rect']
                for lp in f.loops:
                    d = lp.vert.co - O
                    tu = (d.dot(U) - u0) / (u1 - u0)
                    tv = (d.dot(Vv) - v0) / (v1 - v0)
                    if rot:
                        lp[uv].uv = (a0 + tv * (a1 - a0), b0 + (1.0 - tu) * (b1 - b0))
                    else:
                        lp[uv].uv = (a0 + tu * (a1 - a0), b0 + tv * (b1 - b0))
        # Joints: every joint face samples the middle of its slot's patch (a tiny projection keeps tangents valid).
        for s in range(len(mats)):
            pf = [f for f in bm.faces if patch[f.index] and f.material_index == s]
            if not pf:
                continue
            us = [lp[uv].uv.copy() for lp in pf[0].loops]
            cu = sum((u.x for u in us)) / len(us)
            cv = sum((u.y for u in us)) / len(us)
            span = max(max(u.x for u in us) - min(u.x for u in us), 1e-5) * 0.25
            for f in bm.faces:
                if not joint[f.index] or patch[f.index] or f.material_index != s:
                    continue
                nrm = f.normal
                ax = max(range(3), key=lambda i: abs(nrm[i]))
                ia, ib = [(1, 2), (0, 2), (0, 1)][ax]
                for lp in f.loops:
                    co = lp.vert.co
                    lp[uv].uv = (cu + span * math.sin(co[ia] * 7.0), cv + span * math.sin(co[ib] * 7.0 + 1.0))
        # A guard: no face may span a large part of its texture (a face the layout missed keeps a default 0..1 square).
        bad = 0
        for f in bm.faces:
            us = [lp[uv].uv for lp in f.loops]
            if (max(u.x for u in us) - min(u.x for u in us)) * (max(u.y for u in us) - min(u.y for u in us)) > 0.9:
                bad += 1
        if bad:
            print(f"[ecx_building] WARNING {self.name}: {bad} faces span their whole texture (missed by the layout?)")
        bm.to_mesh(me)
        bm.free()


# ------------------------------------------------------------------ cladding: panels with real joints

def frame_of(origin, normal_map):
    """A wall frame from a map-space point and outward map normal: (O, U, Vv, N) in Blender coords, right-handed;
    on a vertical face U runs left to right as seen from outside and Vv is up."""
    N = VN(*normal_map).normalized()
    if abs(N.z) > 0.9:
        U = Vector((0.0, -1.0, 0.0))
        Vv = N.cross(U).normalized()
    else:
        U = Vector((-N.y, N.x, 0.0)).normalized()
        Vv = Vector((0.0, 0.0, 1.0))
    return (V(*origin), U, Vv, N)


def rect_minus(r, holes):
    """Rectangle (u0, u1, v0, v1) minus hole rectangles, as disjoint rectangles."""
    u0, u1, v0, v1 = r
    hs = [(max(h[0], u0), min(h[1], u1), max(h[2], v0), min(h[3], v1)) for h in holes]
    hs = [h for h in hs if h[0] < h[1] - 1e-6 and h[2] < h[3] - 1e-6]
    if not hs:
        return [r]
    us = sorted({u0, u1} | {h[0] for h in hs} | {h[1] for h in hs})
    out = []
    for a, b in zip(us, us[1:]):
        if b - a < 1e-6:
            continue
        m = (a + b) / 2
        cuts = sorted((h[2], h[3]) for h in hs if h[0] < m < h[1])
        v = v0
        for c, d in cuts:
            if c > v + 1e-6:
                out.append((a, b, v, c))
            v = max(v, d)
        if v < v1 - 1e-6:
            out.append((a, b, v, v1))
    return out


def fpt(frame, u, v, w):
    O, U, Vv, N = frame
    return O + U * u + Vv * v + N * w


def panel(mb, frame, a, b, c, d, joint, cham, lip, seg=None, **attrs):
    """One stone panel: a flat front with a rounded rim (seg segments), its outer edge half a joint in from (a, b, c, d)."""
    N = frame[3]
    seg = DETAIL['panel_seg'] if seg is None else seg
    oa, ob, oc, od = a + joint / 2, b - joint / 2, c + joint / 2, d - joint / 2
    if ob - oa < 0.004 or od - oc < 0.004:
        return
    ia, ib, ic, id_ = oa + cham, ob - cham, oc + cham, od - cham
    if ib - ia < 0.004 or id_ - ic < 0.004:
        mb.poly([fpt(frame, oa, oc, lip), fpt(frame, ob, oc, lip), fpt(frame, ob, od, lip), fpt(frame, oa, od, lip)], want=N, **attrs)
        return
    rings = []
    for i in range(seg + 1):
        t = (math.pi / 2) * i / seg
        ins = cham - cham * math.sin(t)
        w = lip - cham + cham * math.cos(t)
        rings.append([mb.v(fpt(frame, u, v, w)) for u, v in ((oa + ins, oc + ins), (ob - ins, oc + ins), (ob - ins, od - ins), (oa + ins, od - ins))])
    mb.f(rings[0], want=N, **attrs)
    for r0, r1 in zip(rings, rings[1:]):
        for i in range(4):
            j = (i + 1) % 4
            mb.f([r1[i], r1[j], r0[j], r0[i]], want=N, **attrs)


def clad(kit, slot, frame, rect, holes=(), mod=(1.2, 0.6), joint=0.006, cham=0.004, depth=0.025, stagger=True, weight=1.0,
         backing=True, lip=0.0008, seed=None, wall=True, **attrs):
    """Stone panels over a wall rectangle (u0, u1, v0, v1) in frame, minus holes: running courses of mod (w, h), each
    panel its own tone (ecx_tone) and a hair of lippage; a dark backing in the joints. Returns the planar wall id."""
    mb = kit.mb(slot)
    u0, u1, v0, v1 = rect
    wid = kit.new_wall(slot, frame, rect, weight) if wall else 0
    rng = random.Random(seed if seed is not None else 7919 * (wid + 1) + slot)
    nc = max(1, round((v1 - v0) / mod[1]))
    ch = (v1 - v0) / nc
    for k in range(nc):
        cv0 = v0 + k * ch
        cv1 = cv0 + ch
        off = mod[0] / 2 if (stagger and k % 2) else 0.0
        edges = [u0]
        x = u0 + mod[0] - off
        while x < u1 - 1e-6:
            edges.append(x)
            x += mod[0]
        edges.append(u1)
        if len(edges) > 2 and edges[1] - edges[0] < 0.2:
            edges.pop(1)
        if len(edges) > 2 and edges[-1] - edges[-2] < 0.2:
            edges.pop(-2)
        for a, b in zip(edges, edges[1:]):
            tone = rng.random()
            lp = lip * rng.uniform(-1.0, 1.0)
            for (pa, pb, pc, pd) in rect_minus((a, b, cv0, cv1), holes):
                panel(mb, frame, pa, pb, pc, pd, joint, cham, lp, ecx_wall=wid, ecx_tone=tone, **attrs)
    if backing:
        for (pa, pb, pc, pd) in rect_minus(rect, holes):
            mb.poly([fpt(frame, pa, pc, -depth), fpt(frame, pb, pc, -depth), fpt(frame, pb, pd, -depth), fpt(frame, pa, pd, -depth)],
                    want=frame[3], ecx_joint=1, ecx_wall=0)
    return wid


def reveal(mb, frame, hole, depth, w0=0.0, sides=(True, True, True, True), **attrs):
    """Lining quads round a rectangular hole (u0, u1, v0, v1), from w0 back to w0 - depth, facing into the hole."""
    a, b, c, d = hole
    U, Vv = frame[1], frame[2]
    if sides[0]:
        mb.poly([fpt(frame, a, c, w0), fpt(frame, a, d, w0), fpt(frame, a, d, w0 - depth), fpt(frame, a, c, w0 - depth)], want=U, **attrs)
    if sides[1]:
        mb.poly([fpt(frame, b, c, w0), fpt(frame, b, d, w0), fpt(frame, b, d, w0 - depth), fpt(frame, b, c, w0 - depth)], want=-U, **attrs)
    if sides[2]:
        mb.poly([fpt(frame, a, c, w0), fpt(frame, b, c, w0), fpt(frame, b, c, w0 - depth), fpt(frame, a, c, w0 - depth)], want=Vv, **attrs)
    if sides[3]:
        mb.poly([fpt(frame, a, d, w0), fpt(frame, b, d, w0), fpt(frame, b, d, w0 - depth), fpt(frame, a, d, w0 - depth)], want=-Vv, **attrs)


def fquad(mb, frame, a, b, c, d, w, **attrs):
    """A quad in a wall frame's plane at offset w, facing out."""
    return mb.poly([fpt(frame, a, c, w), fpt(frame, b, c, w), fpt(frame, b, d, w), fpt(frame, a, d, w)], want=frame[3], **attrs)


def fbox(mb, frame, a, b, c, d, w0, w1, cham=0.0, seg=None, **attrs):
    """A box in a wall frame: u a..b, v c..d, w w0..w1 (out of the face)."""
    O, U, Vv, N = frame
    center = fpt(frame, (a + b) / 2, (c + d) / 2, (w0 + w1) / 2)
    mb.obox(center, (U, Vv, N), (abs(b - a) / 2, abs(d - c) / 2, abs(w1 - w0) / 2), cham, (), seg, **attrs)


# ------------------------------------------------------------------ type

_CAP = {}


def _font(face):
    return bpy.data.fonts.load(os.path.join(core.FONTS, f'Roboto-{face}.ttf'), check_existing=True)


def cap_ratio(face):
    """Cap height of Roboto <face> per unit of type size."""
    if face not in _CAP:
        cu = bpy.data.curves.new('cap', 'FONT')
        cu.body = 'H'
        cu.font = _font(face)
        cu.size = 1.0
        o = core.link(bpy.data.objects.new('cap', cu))
        bpy.context.view_layer.update()
        _CAP[face] = o.dimensions.y
        bpy.data.objects.remove(o)
    return _CAP[face]


def text_obj(name, s, face, cap, tracking=1.0, depth=0.02, align='CENTER', bevel=0.0, resolution=6):
    """Text as a mesh in its own frame: x along the line, y up from the baseline, z out of the face (extruded from
    -depth/2 to +depth/2). cap: the capital height in meters."""
    cu = bpy.data.curves.new(name, 'FONT')
    cu.body = s
    cu.font = _font(face)
    cu.size = cap / cap_ratio(face)
    cu.space_character = tracking
    cu.align_x = align
    cu.extrude = depth / 2
    cu.bevel_depth = bevel
    cu.bevel_resolution = 1 if bevel > 0 else 0
    cu.resolution_u = resolution
    o = core.link(bpy.data.objects.new(name, cu))
    core.select_only([o])
    bpy.ops.object.convert(target='MESH')
    o = bpy.context.view_layer.objects.active
    o.name = name
    o.data.name = name
    return o


def face_matrix(origin_map, normal_map):
    """Text frame -> Blender: text x runs left to right as seen from outside the face (spec 0.3), y up, z out."""
    N = VN(*normal_map).normalized()
    if abs(N.z) > 0.9:
        t = Vector((0.0, -1.0, 0.0))
        upv = N.cross(t).normalized()
    else:
        t = Vector((-N.y, N.x, 0.0)).normalized()
        upv = Vector((0.0, 0.0, 1.0))
    m = Matrix((t, upv, N)).transposed().to_4x4()
    m.translation = V(*origin_map)
    return m


def split_text(obj):
    """Splits an extruded text mesh (still in its own frame) into (front cap, sides) objects; drops the back."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.normal_update()
    zmax = max(v.co.z for v in bm.verts)
    out = []
    for nm, test in (('front', lambda f: f.normal.z > 0.5 and all(abs(v.co.z - zmax) < 1e-4 for v in f.verts)),
                     ('sides', lambda f: abs(f.normal.z) <= 0.5)):
        nb = bmesh.new()
        vmap = {}
        for f in bm.faces:
            if not test(f):
                continue
            vs = []
            for v in f.verts:
                if v.index not in vmap:
                    vmap[v.index] = nb.verts.new(v.co)
                vs.append(vmap[v.index])
            try:
                nb.faces.new(vs)
            except ValueError:
                pass
        out.append(core.mesh_object(obj.name + '_' + nm, nb))
    bm.free()
    bpy.data.objects.remove(obj)
    return out


# ------------------------------------------------------------------ shader helpers

def A(m, name, out='Fac'):
    n = m.node('ShaderNodeAttribute')
    n.attribute_type = 'GEOMETRY'
    n.attribute_name = name
    return n.outputs[out]


def lerp(m, f, a, b):
    return mix_float(m, f, a, b)


def is_k(m, s, k):
    return m.math('LESS_THAN', m.math('ABSOLUTE', m.math('SUBTRACT', s, float(k))), 0.5)


def cmul(m, col, s):
    if isinstance(s, (int, float)):
        return m.mix(1.0, col, (s, s, s, 1.0), blend='MULTIPLY')
    return m.mix(1.0, col, vec(m, s, s, s), blend='MULTIPLY')


def cadd(m, a, b):
    return m.mix(1.0, a, b, blend='ADD')


def vmath(m, op, a, b=None):
    n = m.node('ShaderNodeVectorMath')
    n.operation = op
    for i, v in enumerate((a, b)):
        if v is None:
            continue
        if isinstance(v, (tuple, list)):
            n.inputs[i].default_value = v
        else:
            m.link(v, n.inputs[i])
    return n.outputs['Value'] if op in ('DOT_PRODUCT', 'LENGTH', 'DISTANCE') else n.outputs['Vector']


def split3(m, v):
    n = m.node('ShaderNodeSeparateXYZ')
    m.link(v, n.inputs['Vector'])
    return n.outputs['X'], n.outputs['Y'], n.outputs['Z']


def vor(m, vector, scale, randomness=1.0):
    n = m.node('ShaderNodeTexVoronoi', Scale=scale, Randomness=randomness)
    m.link(vector, n.inputs['Vector'])
    return n.outputs['Distance'], n.outputs['Color']


def bump_n(m, height, strength=0.3, distance=0.002, normal=None):
    b = m.node('ShaderNodeBump', Distance=distance)
    if isinstance(strength, (int, float)):
        b.inputs['Strength'].default_value = strength
    else:
        m.link(strength, b.inputs['Strength'])
    m.link(height, b.inputs['Height'])
    if normal is not None:
        m.link(normal, b.inputs['Normal'])
    return b.outputs['Normal']


def rampx(m, x, lo, hi):
    """ramp() whose bounds may be sockets: 0 below lo, 1 above hi, smooth in between."""
    n = m.node('ShaderNodeMapRange')
    n.interpolation_type = 'SMOOTHSTEP'
    for sock, v in ((n.inputs['From Min'], lo), (n.inputs['From Max'], hi)):
        if isinstance(v, (int, float)):
            sock.default_value = v
        else:
            m.link(v, sock)
    m.link(x, n.inputs['Value'])
    return n.outputs['Result']


def band(m, x, a, b, soft):
    """1 between a and b (floats or sockets), soft edges."""
    def off(v, d):
        return v + d if isinstance(v, (int, float)) else m.math('ADD', v, d)
    return m.math('MULTIPLY', rampx(m, x, off(a, -soft), off(a, soft)), m.math('SUBTRACT', 1.0, rampx(m, x, off(b, -soft), off(b, soft))))


class C:
    """The sockets every surface starts from: object coordinates (Blender = map with X negated), the normal."""

    def __init__(self, m):
        tc, sep = object_coords(m)
        self.tc, self.sep = tc, sep
        self.P = tc.outputs['Object']
        self.x, self.y, self.z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
        self.N = m.node('ShaderNodeNewGeometry').outputs['Normal']
        self.nx, self.ny, self.nz = split3(m, self.N)

    def face_u(self, m):
        """Along a vertical face, left to right as you face it: dot(P, (-N.y, N.x, 0))."""
        t = vmath(m, 'NORMALIZE', vec(m, m.math('MULTIPLY', self.ny, -1.0), self.nx, 0.0))
        return vmath(m, 'DOT_PRODUCT', self.P, t)


def weathering(m, c, col, rough, streaks=1.0, salt=1.0, ground=0.0):
    """Snowmelt streaks running down the stone, salt bloom and a tide line near the ground, splash-wet at its foot."""
    st = noise(m, scaled(m, c.sep, 4.5, 4.5, 0.16), 1.0, 3.0)
    streak = m.math('MULTIPLY', ramp(m, st, 0.56, 0.76), 0.5 * streaks)
    col = m.mix(streak, col, cmul(m, col, 0.6))
    rough = m.math('SUBTRACT', rough, m.math('MULTIPLY', streak, 0.3))
    if salt > 0.0:
        h = m.math('SUBTRACT', c.z, ground)
        patch = ramp(m, noise(m, c.P, 2.2, 4.0, 0.6), 0.42, 0.66)
        crust = ramp(m, noise(m, c.P, 260.0, 2.0), 0.45, 0.7)
        low = m.math('MULTIPLY', ramp(m, h, 0.85, 0.05), patch)
        tide_h = m.math('ADD', 0.32, m.math('MULTIPLY', noise(m, scaled(m, c.sep, 1.4, 1.4, 0.0), 1.0, 2.0), 0.2))
        tide = m.math('MULTIPLY', band(m, h, 0.0, 1.0, 0.0), ramp(m, m.math('ABSOLUTE', m.math('SUBTRACT', h, tide_h)), 0.035, 0.0))
        s = m.math('MINIMUM', m.math('ADD', m.math('MULTIPLY', low, m.math('ADD', 0.35, m.math('MULTIPLY', crust, 0.5))), m.math('MULTIPLY', tide, 0.55)), 1.0)
        s = m.math('MULTIPLY', s, salt)
        col = m.mix(s, col, hx(0x86847f))
        rough = lerp(m, s, rough, 0.85)
        wet = m.math('MULTIPLY', ramp(m, h, 0.28, 0.0), m.math('SUBTRACT', 1.0, s))
        col = m.mix(m.math('MULTIPLY', wet, 0.6), col, cmul(m, col, 0.55))
        rough = lerp(m, m.math('MULTIPLY', wet, 0.8), rough, 0.12)
    return col, rough


# ------------------------------------------------------------------ the shared recipes (spec 0.5)

def basalt(name, streaks=1.0, salt=1.0):
    """Honed dark basalt: a warm-grey cloud, fine vesicles, per-panel tone (ecx_tone); polished bands (ecx_polish),
    split-face plinths (ecx_split), the joints' dark (ecx_joint). ecx_k: 1 roof membrane, 2 flamed deck paver."""
    m = core.Mat(name)
    c = C(m)
    tone, polish, split, jnt, kind = A(m, 'ecx_tone'), A(m, 'ecx_polish'), A(m, 'ecx_split'), A(m, 'ecx_joint'), A(m, 'ecx_k')
    cloud = noise(m, c.P, 0.45, 4.0)
    fine = noise(m, c.P, 70.0, 3.0)
    col = m.mix(ramp(m, cloud, 0.3, 0.72), hx(0x23252a), hx(0x2e3036))
    col = m.mix(m.math('MULTIPLY', ramp(m, noise(m, c.P, 0.12, 3.0), 0.5, 0.82), 0.55), col, hx(0x37342f))
    col = cmul(m, col, m.math('ADD', 0.88, m.math('MULTIPLY', tone, 0.24)))
    col = m.mix(m.math('MULTIPLY', fine, 0.22), col, hx(0x3d3f44))
    ves_d, _ = vor(m, c.P, 48.0)
    ves = ramp(m, ves_d, 0.075, 0.02)
    col = m.mix(m.math('MULTIPLY', ves, 0.65), col, hx(0x0f1012))
    col = m.mix(m.math('MULTIPLY', polish, 0.6), col, cmul(m, col, 0.7))
    sf = noise(m, scaled(m, c.sep, 7.0, 7.0, 11.0), 1.0, 6.0, 0.65, 0.4)
    sfc, _ = vor(m, scaled(m, c.sep, 5.0, 5.0, 8.0), 1.0)
    col = m.mix(split, col, m.mix(ramp(m, sf, 0.3, 0.75), hx(0x2a2c30), hx(0x4a4c51)))
    rough = m.math('ADD', 0.55, m.math('MULTIPLY', m.math('SUBTRACT', fine, 0.5), 0.1))
    rough = lerp(m, polish, rough, 0.33)
    rough = lerp(m, split, rough, 0.8)
    col, rough = weathering(m, c, col, rough, streaks, salt)
    # Roof membrane and deck pavers.
    roofc = m.mix(ramp(m, noise(m, c.P, 0.35, 4.0), 0.3, 0.75), hx(0xd9e1ec), hx(0xeef2f7))
    roofc = m.mix(m.math('MULTIPLY', ramp(m, noise(m, c.P, 3.0, 3.0), 0.62, 0.75), 0.35), roofc, hx(0x9aa6b8))
    col = m.mix(is_k(m, kind, 1), col, roofc)
    rough = lerp(m, is_k(m, kind, 1), rough, 0.62)
    deck = m.mix(ramp(m, noise(m, c.P, 40.0, 2.0), 0.4, 0.6), hx(0x3d3e41), hx(0x4b4c50))
    pav_u = m.math('FRACT', m.math('DIVIDE', c.x, 0.6))
    pav_v = m.math('FRACT', m.math('DIVIDE', c.y, 0.6))
    pj = m.math('MAXIMUM', ramp(m, m.math('MINIMUM', pav_u, m.math('SUBTRACT', 1.0, pav_u)), 0.012, 0.004),
                ramp(m, m.math('MINIMUM', pav_v, m.math('SUBTRACT', 1.0, pav_v)), 0.012, 0.004))
    deck = m.mix(pj, deck, hx(0x111214))
    deck = cmul(m, deck, 0.7)   # always a little wet up here
    col = m.mix(is_k(m, kind, 2), col, deck)
    rough = lerp(m, is_k(m, kind, 2), rough, m.math('ADD', 0.32, m.math('MULTIPLY', pj, 0.4)))
    col = m.mix(jnt, col, hx(0x0b0c0e))
    rough = lerp(m, jnt, rough, 0.85)
    m.set('Base Color', col)
    m.set('Roughness', rough)
    h = m.math('ADD', m.math('MULTIPLY', ves, -0.5), m.math('MULTIPLY', m.math('MULTIPLY', split, m.math('ADD', sf, m.math('MULTIPLY', sfc, 0.6))), 2.5))
    m.set('Normal', bump_n(m, h, 0.35, 0.004))
    return m


def bronze(name, outdoor=1.0, engrave=None):
    """Dark statuary bronze, brushed vertically, lighter rubbed edges, dark patina in the hollows; outdoors, salt and
    snowmelt leave verdigris streaks. ecx_cu: the brand's copper instead (accents); ecx_dk: blackened (shadow lines);
    ecx_ptone: a cladding panel's own patina (0..1, a little darker and duller); ecx_enamel: midnight-navy vitreous
    enamel (the crest's ground)."""
    m = core.Mat(name)
    c = C(m)
    cu, dk = A(m, 'ecx_cu'), A(m, 'ecx_dk')
    ptone, enamel = A(m, 'ecx_ptone'), A(m, 'ecx_enamel')
    brush = noise(m, scaled(m, c.sep, 90.0, 90.0, 1.4), 1.0, 3.0)
    mott = noise(m, c.P, 1.6, 4.0)
    base = m.mix(ramp(m, mott, 0.28, 0.76), hx(0x4a3423), hx(0x6b4a30))
    base = m.mix(m.math('MULTIPLY', brush, 0.35), base, hx(0x5a3e28))
    edges = convex_edges(m, c.tc, distance=0.012, samples=8, breakup_scale=5.0, gain=4.0)
    base = m.mix(m.math('MULTIPLY', edges, 0.8), base, hx(0x9c744c))
    ao = m.node('ShaderNodeAmbientOcclusion', Distance=0.12)
    ao.samples = 8
    cav = m.math('SUBTRACT', 1.0, ao.outputs['AO'])
    base = m.mix(m.math('MULTIPLY', cav, 0.7), base, hx(0x20150d))
    vg = m.math('MULTIPLY', ramp(m, noise(m, scaled(m, c.sep, 12.0, 12.0, 0.45), 1.0, 3.0), 0.64, 0.8),
                m.math('MULTIPLY', m.math('ADD', 0.2, m.math('MULTIPLY', cav, 0.8)), 0.5 * outdoor))
    base = m.mix(vg, base, hx(0x44584e))
    # Hand height: soft, broad polish where hands and coats rub (no speckle: it read as dirt in the sun).
    scuff = m.math('MULTIPLY', ramp(m, noise(m, scaled(m, c.sep, 9.0, 9.0, 2.2), 1.0, 2.0), 0.5, 0.74), band(m, c.z, 0.8, 1.6, 0.18))
    base = m.mix(m.math('MULTIPLY', scuff, 0.32), base, hx(0x8c6844))
    # Each cladding panel weathers on its own (ecx_ptone 0..1): a shade darker, a touch duller.
    base = cmul(m, base, m.math('SUBTRACT', 1.0, m.math('MULTIPLY', ptone, 0.13)))
    copper = m.mix(m.math('MULTIPLY', edges, 0.6), hx(0xb8743f), hx(0xd8a070))
    copper = m.mix(m.math('MULTIPLY', cav, 0.5), copper, hx(0x5a2c14))
    base = m.mix(cu, base, copper)
    base = m.mix(dk, base, hx(0x1b130c))
    navy = m.mix(ramp(m, noise(m, c.P, 3.0, 3.0), 0.3, 0.7), hx(0x0d1528), hx(0x111b33))
    base = m.mix(enamel, base, navy)
    rough = m.math('ADD', 0.38, m.math('MULTIPLY', m.math('SUBTRACT', brush, 0.5), 0.1))
    rough = m.math('SUBTRACT', rough, m.math('MULTIPLY', m.math('ADD', edges, scuff), 0.1))
    rough = m.math('ADD', rough, m.math('ADD', m.math('MULTIPLY', vg, 0.35), m.math('MULTIPLY', cav, 0.12)))
    rough = m.math('ADD', rough, m.math('MULTIPLY', ptone, 0.05))
    rough = lerp(m, cu, rough, 0.3)
    rough = lerp(m, enamel, rough, 0.24)
    if engrave is not None:
        # An engraving on the faces toward -X (path, y0, y1, z0, z1): the print's white is cut in, dark and matte.
        path, y0, y1, z0, z1 = engrave
        uu = m.math('DIVIDE', m.math('SUBTRACT', y1, c.y), y1 - y0)
        vv = m.math('DIVIDE', m.math('SUBTRACT', c.z, z0), z1 - z0)
        img = m.image(path, vector=vec(m, uu, vv, 0.0), extension='CLIP')
        lum = split3(m, img.outputs['Color'])[0]
        cut = m.math('MULTIPLY', m.math('MULTIPLY', img.outputs['Alpha'], lum), ramp(m, c.nx, -0.4, -0.7))
        base = m.mix(m.math('MULTIPLY', cut, 0.85), base, hx(0x15100b))
        rough = m.math('ADD', rough, m.math('MULTIPLY', cut, 0.3))
    m.set('Base Color', base)
    m.set('Roughness', rough)
    metal = m.math('SUBTRACT', 1.0, m.math('ADD', m.math('MULTIPLY', vg, 0.7), m.math('MULTIPLY', cav, 0.15)))
    m.set('Metallic', m.math('MULTIPLY', metal, m.math('SUBTRACT', 1.0, enamel)))
    bump_h = m.math('ADD', m.math('MULTIPLY', brush, 0.3), m.math('MULTIPLY', vg, 0.4))
    m.set('Normal', bump_n(m, m.math('MULTIPLY', bump_h, m.math('SUBTRACT', 1.0, enamel)), 0.12, 0.0008))
    return m


def timber(name):
    """Oiled larch: honey, the grain along the member (ecx_grain: 0 along Blender X, 1 Y, 2 Z), +-8% per board
    (ecx_tone); ecx_weather silvers it where the weather reaches."""
    m = core.Mat(name)
    c = C(m)
    tone, gax, wth = A(m, 'ecx_tone'), A(m, 'ecx_grain'), A(m, 'ecx_weather')
    gs = []
    for ax in range(3):
        s = [36.0, 36.0, 36.0]
        s[ax] = 0.7
        gs.append(noise(m, scaled(m, c.sep, *s, offset=(ax * 3.1, ax * 1.7, 0.0)), 1.0, 5.0, 0.6, 1.4))
    grain = lerp(m, is_k(m, gax, 1), gs[0], gs[1])
    grain = lerp(m, is_k(m, gax, 2), grain, gs[2])
    lines = ramp(m, m.math('ABSOLUTE', m.math('SUBTRACT', m.math('FRACT', m.math('MULTIPLY', grain, 9.0)), 0.5)), 0.09, 0.0)
    col = m.mix(ramp(m, grain, 0.3, 0.75), hx(0x8a5a33), hx(0xa8703f))
    col = m.mix(m.math('MULTIPLY', lines, 0.5), col, hx(0x663c1e))
    col = cmul(m, col, m.math('ADD', 0.92, m.math('MULTIPLY', tone, 0.16)))
    kd, _ = vor(m, scaled(m, c.sep, 1.6, 1.6, 1.6), 1.0)
    knot = ramp(m, kd, 0.035, 0.01)
    col = m.mix(m.math('MULTIPLY', knot, 0.7), col, hx(0x3a220f))
    silver = m.math('MULTIPLY', wth, ramp(m, noise(m, c.P, 2.5, 3.0), 0.25, 0.65))
    col = m.mix(m.math('MULTIPLY', silver, 0.55), col, hx(0x847d70))
    col = m.mix(m.math('MULTIPLY', wth, m.math('MULTIPLY', ramp(m, noise(m, scaled(m, c.sep, 6.0, 6.0, 0.3), 1.0, 3.0), 0.55, 0.75), 0.4)), col, hx(0x3e3226))
    m.set('Base Color', col)
    m.set('Roughness', m.math('ADD', 0.45, m.math('ADD', m.math('MULTIPLY', lines, 0.1), m.math('MULTIPLY', silver, 0.25))))
    m.set('Specular IOR Level', 0.35)
    m.set('Normal', bump_n(m, m.math('SUBTRACT', grain, m.math('MULTIPLY', lines, 0.5)), 0.12, 0.001))
    return m


def granite(name):
    """Flamed light-grey granite with black and white speckle; ecx_wet: the wet look; ecx_k 1: honed (interior)."""
    m = core.Mat(name)
    c = C(m)
    wet, kind = A(m, 'ecx_wet'), A(m, 'ecx_k')
    o = c.P
    col = m.mix(ramp(m, noise(m, o, 3.0, 3.0), 0.3, 0.7), hx(0x868480), hx(0x93918d))
    col = m.mix(ramp(m, noise(m, o, 240.0, 2.0), 0.6, 0.66), col, hx(0x26262a))
    col = m.mix(ramp(m, noise(m, vmath(m, 'ADD', o, (3.3, 1.1, 7.7)), 300.0, 1.0), 0.68, 0.72), col, hx(0xd2d0ca))
    col = m.mix(m.math('MULTIPLY', ramp(m, noise(m, o, 90.0, 2.0), 0.62, 0.7), 0.6), col, hx(0x5d5853))
    col, _r = weathering(m, c, col, 0.75, 0.4, 0.8)
    col = m.mix(m.math('MULTIPLY', wet, 0.85), col, cmul(m, col, 0.66))
    rough = lerp(m, wet, m.math('ADD', 0.75, m.math('MULTIPLY', noise(m, o, 30.0, 2.0), 0.08)), 0.3)
    rough = lerp(m, is_k(m, kind, 1), rough, 0.38)
    m.set('Base Color', col)
    m.set('Roughness', rough)
    m.set('Normal', bump_n(m, noise(m, o, 160.0, 3.0), lerp(m, is_k(m, kind, 1), 0.25, 0.05), 0.0006))
    return m


def frame_dark(name):
    """Black anodized aluminium; ecx_k 1: an opal diffuser, 2: black rubber, 3: a ribbed walk-off mat."""
    m = core.Mat(name)
    c = C(m)
    kind = A(m, 'ecx_k')
    g = noise(m, c.P, 900.0, 2.0)
    dust = m.math('MULTIPLY', ramp(m, c.nz, 0.4, 0.9), ramp(m, noise(m, c.P, 6.0, 3.0), 0.35, 0.75))
    col = m.mix(m.math('MULTIPLY', g, 0.3), hx(0x15161a), hx(0x1e1f24))
    col = m.mix(m.math('MULTIPLY', dust, 0.4), col, hx(0x3a3936))
    rib = ramp(m, m.math('SINE', m.math('MULTIPLY', c.y, 2 * math.pi / 0.012)), -0.2, 0.4)
    col = m.mix(is_k(m, kind, 1), col, hx(0xe4e0d8))
    col = m.mix(is_k(m, kind, 2), col, hx(0x101010))
    col = m.mix(is_k(m, kind, 3), col, m.mix(rib, hx(0x16171a), hx(0x2a2b2e)))
    rough = lerp(m, is_k(m, kind, 1), m.math('ADD', 0.45, m.math('MULTIPLY', dust, 0.3)), 0.3)
    rough = lerp(m, m.math('MAXIMUM', is_k(m, kind, 2), is_k(m, kind, 3)), rough, 0.88)
    metal = lerp(m, m.math('MAXIMUM', is_k(m, kind, 1), m.math('MAXIMUM', is_k(m, kind, 2), is_k(m, kind, 3))), 0.85, 0.0)
    m.set('Base Color', col)
    m.set('Roughness', rough)
    m.set('Metallic', metal)
    m.set('Normal', bump_n(m, m.math('MULTIPLY', rib, is_k(m, kind, 3)), 0.4, 0.002))
    return m


def snow(name):
    """Fallback snow (the game swaps in M_ECX_Snow): white-blue, bluer in the hollows, wind ripples and sparkle grains."""
    m = core.Mat(name)
    c = C(m)
    warp = noise(m, c.P, 0.6, 3.0)
    rip = m.math('SINE', m.math('MULTIPLY', m.math('ADD', m.math('ADD', c.x, m.math('MULTIPLY', c.y, 0.35)), m.math('MULTIPLY', warp, 0.9)), 2 * math.pi / 0.32))
    gd, _ = vor(m, c.P, 380.0)
    grain = ramp(m, gd, 0.06, 0.0)
    ao = m.node('ShaderNodeAmbientOcclusion', Distance=0.3)
    ao.samples = 8
    col = m.mix(ramp(m, noise(m, c.P, 0.8, 3.0), 0.3, 0.7), hx(0xe8edf5), hx(0xdde6f2))
    col = m.mix(m.math('MULTIPLY', m.math('SUBTRACT', 1.0, ao.outputs['AO']), 0.8), col, hx(0xa9bdd8))
    col = m.mix(m.math('MULTIPLY', ramp(m, c.nz, 0.2, -0.4), 0.5), col, hx(0xb8c8de))
    m.set('Base Color', col)
    m.set('Roughness', m.math('SUBTRACT', 0.62, m.math('MULTIPLY', grain, 0.3)))
    m.set('Normal', bump_n(m, m.math('ADD', m.math('MULTIPLY', rip, 0.15), m.math('ADD', m.math('MULTIPLY', grain, 0.5), m.math('MULTIPLY', warp, 0.6))), 0.3, 0.004))
    return m


def ice(name):
    m = core.Mat(name)
    c = C(m)
    col = m.mix(ramp(m, noise(m, scaled(m, c.sep, 30.0, 30.0, 2.0), 1.0, 3.0), 0.3, 0.7), hx(0xcfe3f0), hx(0xa9c8de))
    m.set('Base Color', col)
    m.set('Roughness', m.math('ADD', 0.05, m.math('MULTIPLY', noise(m, c.P, 60.0, 2.0), 0.08)))
    m.set('Specular IOR Level', 0.6)
    m.set('Normal', bump_n(m, noise(m, scaled(m, c.sep, 40.0, 40.0, 6.0), 1.0, 3.0), 0.25, 0.002))
    return m


def roof_metal(name):
    """Dark standing-seam zinc: a little patina, streaks."""
    m = core.Mat(name)
    c = C(m)
    st = ramp(m, noise(m, scaled(m, c.sep, 1.0, 10.0, 1.0), 1.0, 3.0), 0.5, 0.75)
    col = m.mix(ramp(m, noise(m, c.P, 1.2, 3.0), 0.3, 0.7), hx(0x383b40), hx(0x45484d))
    col = m.mix(m.math('MULTIPLY', st, 0.4), col, hx(0x5c5f62))
    m.set('Base Color', col)
    m.set('Metallic', 0.75)
    m.set('Roughness', m.math('ADD', 0.42, m.math('MULTIPLY', st, 0.15)))
    return m


def roof_works(name):
    """The roof's stuff: ecx_k 0 concrete pavers, 1 painted plant (units, ducts), 2 gravel ballast, 3 membrane."""
    m = core.Mat(name)
    c = C(m)
    kind = A(m, 'ecx_k')
    pu = m.math('FRACT', m.math('DIVIDE', c.x, 0.6))
    pv = m.math('FRACT', m.math('DIVIDE', c.y, 0.6))
    pj = m.math('MAXIMUM', ramp(m, m.math('MINIMUM', pu, m.math('SUBTRACT', 1.0, pu)), 0.015, 0.005),
                ramp(m, m.math('MINIMUM', pv, m.math('SUBTRACT', 1.0, pv)), 0.015, 0.005))
    conc = m.mix(ramp(m, noise(m, c.P, 2.0, 4.0), 0.3, 0.7), hx(0x6f6c67), hx(0x84817b))
    conc = m.mix(pj, conc, hx(0x3a3835))
    plant = m.mix(ramp(m, noise(m, c.P, 3.0, 3.0), 0.4, 0.8), hx(0x868b90), hx(0x6c7176))
    gd, _ = vor(m, c.P, 70.0)
    grav = m.mix(ramp(m, gd, 0.0, 0.5), hx(0x4d4a46), hx(0x77736c))
    memb = hx(0x2c2d2f)
    col = conc
    col = m.mix(is_k(m, kind, 1), col, plant)
    col = m.mix(is_k(m, kind, 2), col, grav)
    col = m.mix(is_k(m, kind, 3), col, memb)
    m.set('Base Color', col)
    m.set('Roughness', lerp(m, is_k(m, kind, 1), 0.88, 0.5))
    m.set('Metallic', lerp(m, is_k(m, kind, 1), 0.0, 0.4))
    m.set('Normal', bump_n(m, m.math('ADD', m.math('MULTIPLY', is_k(m, kind, 2), m.math('SUBTRACT', 1.0, gd)), m.math('MULTIPLY', pj, -0.5)), 0.5, 0.01))
    return m


def leather(name, hex_color=0x1d2947):
    m = core.Mat(name)
    c = C(m)
    wr = noise(m, c.P, 38.0, 4.0, 0.6, 1.5)
    edges = convex_edges(m, c.tc, distance=0.01, samples=8, breakup_scale=20.0, gain=3.0)
    col = m.mix(m.math('MULTIPLY', wr, 0.3), hx(hex_color), hx(0x26335a))
    col = m.mix(m.math('MULTIPLY', edges, 0.6), col, hx(0x3a4a78))
    m.set('Base Color', col)
    m.set('Roughness', m.math('ADD', 0.48, m.math('MULTIPLY', wr, 0.12)))
    m.set('Sheen Weight', 0.2)
    m.set('Normal', bump_n(m, wr, 0.25, 0.001))
    return m


def marble(name):
    """Polished black marble (Nero): fine white veins, faint slab lines every 1.2 m."""
    m = core.Mat(name)
    c = C(m)
    warp = noise(m, c.P, 0.9, 4.0, 0.5, 3.0)
    v = noise(m, vec(m, m.math('ADD', c.x, m.math('MULTIPLY', warp, 0.8)), m.math('ADD', c.y, warp), c.z), 2.2, 9.0)
    vein = ramp(m, m.math('ABSOLUTE', m.math('SUBTRACT', v, 0.5)), 0.01, 0.0)
    vein2 = ramp(m, m.math('ABSOLUTE', m.math('SUBTRACT', noise(m, vmath(m, 'ADD', c.P, (5.0, 2.0, 0.0)), 5.0, 8.0), 0.5)), 0.006, 0.0)
    su = m.math('FRACT', m.math('DIVIDE', c.x, 1.2))
    sv = m.math('FRACT', m.math('DIVIDE', c.y, 1.2))
    sl = m.math('MAXIMUM', ramp(m, m.math('MINIMUM', su, m.math('SUBTRACT', 1.0, su)), 0.002, 0.0),
                ramp(m, m.math('MINIMUM', sv, m.math('SUBTRACT', 1.0, sv)), 0.002, 0.0))
    col = m.mix(ramp(m, noise(m, c.P, 1.0, 3.0), 0.3, 0.7), hx(0x090a0c), hx(0x111216))
    col = m.mix(m.math('MULTIPLY', vein, 0.7), col, hx(0xb8b4ac))
    col = m.mix(m.math('MULTIPLY', vein2, 0.4), col, hx(0x8a8680))
    col = m.mix(m.math('MULTIPLY', sl, 0.6), col, hx(0x2a2a2c))
    m.set('Base Color', col)
    m.set('Roughness', m.math('ADD', 0.12, m.math('MULTIPLY', noise(m, c.P, 8.0, 3.0), 0.05)))
    m.set('Coat Weight', 0.3)
    return m


def glass_plain(name, tint=0x1a2226):
    """Unbaked glass for the runtime's M_ECX_Glass (the review renders it transmissive)."""
    m = core.Mat(name)
    m.set('Base Color', hx(0xe8eef0))
    m.set('Roughness', 0.02)
    m.set('Transmission Weight', 1.0)
    m.set('IOR', 1.5)
    m.m.use_backface_culling = False
    return m


def glass_fallback(name):
    """A baked [glass] slot on the door props (the game swaps in M_ECX_Glass): baked as dark, clear-looking glass, so a
    missed swap reads as glass, not as the milky white a transmissive material bakes to."""
    m = core.Mat(name)
    c = C(m)
    smear = noise(m, c.P, 4.0, 3.0)
    col = m.mix(m.math('MULTIPLY', smear, 0.5), hx(0x0c1014), hx(0x141a20))
    m.set('Base Color', col)
    m.set('Roughness', m.math('ADD', 0.03, m.math('MULTIPLY', smear, 0.04)))
    m.set('Specular IOR Level', 0.5)
    return m


# ------------------------------------------------------------------ what glows behind the glass (baked emissive)

def color_ramp(m, x, stops):
    """A ColorRamp of [(pos, hex)] driven by x."""
    n = m.node('ShaderNodeValToRGB')
    els = n.color_ramp.elements
    while len(els) > 1:
        els.remove(els[-1])
    els[0].position = stops[0][0]
    els[0].color = hx(stops[0][1])
    for pos, h in stops[1:]:
        e = els.new(pos)
        e.color = hx(h)
    m.link(x, n.inputs['Fac'])
    return n.outputs['Color']


def win_coords(m, c):
    """A window's own coordinates from its faces' attributes: (uu 0..1 across, vv 0..1 up, seed, look, width, height)."""
    u = c.face_u(m)
    wc, wz0, seed = split3(m, A(m, 'ecx_win', 'Vector'))
    ww, wh, look = split3(m, A(m, 'ecx_win2', 'Vector'))
    ww = m.math('MAXIMUM', ww, 0.01)
    wh = m.math('MAXIMUM', wh, 0.01)
    uu = m.math('ADD', m.math('DIVIDE', m.math('SUBTRACT', u, wc), ww), 0.5)
    vv = m.math('DIVIDE', m.math('SUBTRACT', c.z, wz0), wh)
    return uu, vv, seed, look, ww, wh, u


def glass_base(m, c, vv):
    """Dark glass: a film of grime low, the sky's sheen high (in the base color only; Lumen does the real reflection)."""
    smear = noise(m, c.P, 3.0, 3.0)
    col = m.mix(m.math('MULTIPLY', smear, 0.6), hx(0x080b0f), hx(0x10151b))
    col = m.mix(m.math('MULTIPLY', ramp(m, vv, 0.6, 1.0), 0.35), col, hx(0x1a2230))
    return col


def casino_glass(name):
    """The casino's slot windows: warm light from a double-height gaming floor seen through 9 m slots: the glow of the
    slot banks low (colored, blurred), the wall wash, chandelier sparkle above, the ceiling cove at the head."""
    m = core.Mat(name)
    c = C(m)
    uu, vv, seed, look, ww, wh, u = win_coords(m, c)
    z = c.z
    warm = hx(0xffb070)
    sv = vec(m, m.math('MULTIPLY', u, 1.1), m.math('MULTIPLY', z, 1.6), m.math('MULTIPLY', seed, 13.0))
    hue = noise(m, vec(m, m.math('MULTIPLY', u, 0.7), m.math('MULTIPLY', z, 0.9), m.math('MULTIPLY', seed, 7.0)), 1.0, 1.0)
    pal = color_ramp(m, hue, [(0.0, 0xff3a24), (0.25, 0xffb02e), (0.45, 0xff6a24), (0.6, 0xc24cff), (0.75, 0x3a7cff), (1.0, 0xffd060)])
    slots = m.math('MULTIPLY', band(m, z, 1.45, 3.1, 0.3), ramp(m, noise(m, sv, 1.8, 2.0), 0.42, 0.72))
    wash = m.math('ADD', 0.06, m.math('MULTIPLY', ramp(m, noise(m, vec(m, u, m.math('MULTIPLY', z, 0.25), seed), 0.8, 2.0), 0.3, 0.8), 0.16))
    wash = m.math('ADD', wash, m.math('MULTIPLY', ramp(m, z, 4.5, 2.0), 0.2))
    sd, _ = vor(m, vec(m, m.math('MULTIPLY', u, 2.2), m.math('MULTIPLY', z, 2.2), m.math('MULTIPLY', seed, 5.0)), 1.0)
    sparkle = m.math('MULTIPLY', ramp(m, sd, 0.12, 0.02), band(m, z, 6.2, 9.0, 0.3))
    cove = band(m, z, 9.75, 10.5, 0.12)
    beams = m.math('MULTIPLY', ramp(m, m.math('ABSOLUTE', m.math('SUBTRACT', m.math('FRACT', m.math('DIVIDE', z, 1.35)), 0.5)), 0.42, 0.5), band(m, z, 5.3, 9.6, 0.2))
    em = cmul(m, warm, m.math('ADD', wash, m.math('MULTIPLY', beams, 0.12)))
    em = cadd(m, em, cmul(m, pal, m.math('MULTIPLY', slots, 1.1)))
    em = cadd(m, em, cmul(m, hx(0xffe4b8), m.math('ADD', m.math('MULTIPLY', sparkle, 1.6), m.math('MULTIPLY', cove, 0.9))))
    vign = ramp(m, m.math('MINIMUM', uu, m.math('SUBTRACT', 1.0, uu)), 0.0, 0.18)
    em = cmul(m, em, m.math('ADD', 0.35, m.math('MULTIPLY', vign, 0.65)))
    m.set('Base Color', glass_base(m, c, vv))
    m.set('Roughness', 0.04)
    m.set('Emission Color', em)
    m.set('Emission Strength', 2.2)
    return m


def poker_glass(name, z0=8.0):
    """The poker room band, seen from the plaza: navy walls with copper reveals lit in pools by the linear pendants
    over the tables, the ember cove line, players' silhouettes at the rails; the plenum above the ceiling is dark.
    ecx_win2.z (look) 1: a terrace door (the room seen through it, brighter low)."""
    m = core.Mat(name)
    c = C(m)
    u = c.face_u(m)
    h = m.math('SUBTRACT', c.z, z0)
    seed = noise(m, vec(m, m.math('FLOOR', m.math('DIVIDE', u, 3.4)), 0.0, 0.0), 1.0, 1.0)
    pu = m.math('SUBTRACT', m.math('FRACT', m.math('DIVIDE', u, 3.4)), 0.5)
    pool = m.math('DIVIDE', 1.0, m.math('ADD', 1.0, m.math('ADD', m.math('MULTIPLY', m.math('MULTIPLY', pu, pu), 28.0),
                                                                  m.math('MULTIPLY', m.math('POWER', m.math('SUBTRACT', h, 1.4), 2.0), 1.6))))
    navy = hx(0x1d2947)
    wall = cmul(m, m.mix(ramp(m, noise(m, vec(m, u, h, 0.0), 6.0, 2.0), 0.3, 0.7), navy, hx(0x26335a)), m.math('ADD', 0.35, m.math('MULTIPLY', pool, 2.4)))
    wall = ctimes_hex(m, wall, 0xffead2)
    ru = m.math('FRACT', m.math('DIVIDE', m.math('ADD', u, 1.0), 2.0))
    reveal = m.math('MULTIPLY', ramp(m, m.math('ABSOLUTE', m.math('SUBTRACT', ru, 0.5)), 0.012, 0.004), band(m, h, 0.2, 2.75, 0.02))
    em = m.mix(reveal, wall, cmul(m, hx(0xd08a50), m.math('ADD', 0.4, pool)))
    cove = ramp(m, m.math('ABSOLUTE', m.math('SUBTRACT', h, 2.8)), 0.03, 0.012)
    em = m.mix(cove, em, cmul(m, hx(EMBER_LED), 1.8))
    slats = m.math('MULTIPLY', ramp(m, m.math('SINE', m.math('MULTIPLY', u, 2 * math.pi / 0.1)), -0.3, 0.6), band(m, h, 2.86, 4.0, 0.01))
    em = m.mix(band(m, h, 2.84, 4.0, 0.01), em, cmul(m, hx(0x3a2c22), m.math('ADD', 0.15, m.math('MULTIPLY', slats, 0.25))))
    pend = m.math('MULTIPLY', ramp(m, m.math('ABSOLUTE', pu), 0.25, 0.23), ramp(m, m.math('ABSOLUTE', m.math('SUBTRACT', h, 2.15)), 0.035, 0.02))
    pend2u = m.math('SUBTRACT', m.math('FRACT', m.math('DIVIDE', m.math('ADD', u, 1.7), 3.4)), 0.5)
    pend2 = m.math('MULTIPLY', ramp(m, m.math('ABSOLUTE', pend2u), 0.16, 0.145), ramp(m, m.math('ABSOLUTE', m.math('SUBTRACT', h, 2.4)), 0.022, 0.012))
    em = m.mix(m.math('MAXIMUM', pend, m.math('MULTIPLY', pend2, 0.8)), em, cmul(m, hx(0xffead2), 2.6))
    rail = m.math('MULTIPLY', band(m, h, 0.72, 0.84, 0.01), ramp(m, m.math('ABSOLUTE', pu), 0.36, 0.33))
    hd, hc = vor(m, vec(m, m.math('MULTIPLY', u, 1.6), 0.0, 0.0), 1.0)
    heads = m.math('MULTIPLY', ramp(m, m.math('ADD', m.math('MULTIPLY', m.math('POWER', m.math('SUBTRACT', h, 1.18), 2.0), 9.0), m.math('MULTIPLY', hd, hd)), 0.06, 0.03),
                   m.math('GREATER_THAN', m.math('FRACT', m.math('MULTIPLY', seed, 9.0)), 0.25))
    shoulders = m.math('MULTIPLY', band(m, h, 0.8, 1.05, 0.03), ramp(m, hd, 0.32, 0.22))
    sil = m.math('MAXIMUM', rail, m.math('MAXIMUM', heads, shoulders))
    em = m.mix(m.math('MULTIPLY', sil, 0.9), em, hx(0x050608))
    floor = band(m, h, -0.5, 0.3, 0.02)
    em = m.mix(floor, em, cmul(m, navy, m.math('ADD', 0.08, m.math('MULTIPLY', pool, 0.5))))
    em = m.mix(ramp(m, h, 4.0, 4.05), em, hx(0x000000))
    # ecx_win2.z 2: the grill's wine room on the ground floor (its windows carry their own ecx_win frames).
    wine, wvv = wine_room(m, c)
    is_wine = is_k(m, split3(m, A(m, 'ecx_win2', 'Vector'))[2], 2)
    em = m.mix(is_wine, em, wine)
    m.set('Base Color', glass_base(m, c, lerp(m, is_wine, ramp(m, h, 2.0, 6.5), wvv)))
    m.set('Roughness', 0.04)
    m.set('Emission Color', em)
    m.set('Emission Strength', 3.0)
    return m


def wine_room(m, c):
    """The Ember Grill's wine room seen straight through a window: backlit amber onyx behind glass shelves (0.42 m
    apart, their edges lit), bottles standing on each shelf in silhouette (body, shoulders, neck; dark green, brown and
    a few lit amber spirits), gaps here and there; a dark walnut counter low with a brass edge, a cove line at the head,
    the light pooling in the middle. Returns (emission, the window's vv)."""
    uu, vv, seed, look, ww, wh, u = win_coords(m, c)
    zc = m.math('MULTIPLY', vv, wh)                  # meters above the sill
    P, slot = 0.42, 0.105
    zs = m.math('SUBTRACT', zc, 0.93)                # meters above the counter top
    t = m.math('FRACT', m.math('DIVIDE', zs, P))
    y = m.math('MULTIPLY', t, P)                     # meters above this shelf
    row = m.math('FLOOR', m.math('DIVIDE', zs, P))
    s = m.math('FRACT', m.math('DIVIDE', u, slot))
    sid = vec(m, m.math('FLOOR', m.math('DIVIDE', u, slot)), row, m.math('MULTIPLY', seed, 31.0))
    wn = m.node('ShaderNodeTexWhiteNoise')
    wn.noise_dimensions = '3D'
    m.link(sid, wn.inputs['Vector'])
    r = wn.outputs['Value']
    r2 = m.math('FRACT', m.math('MULTIPLY', r, 7.31))
    r3 = m.math('FRACT', m.math('MULTIPLY', r, 13.7))
    xm = m.math('ABSOLUTE', m.math('MULTIPLY', m.math('SUBTRACT', s, 0.5), slot))
    bw = m.math('ADD', 0.032, m.math('MULTIPLY', r2, 0.01))
    bh = m.math('ADD', 0.19, m.math('MULTIPLY', r3, 0.06))
    nw = 0.012
    body = m.math('MULTIPLY', ramp(m, m.math('SUBTRACT', xm, bw), 0.002, -0.002), ramp(m, m.math('SUBTRACT', y, bh), 0.002, -0.002))
    sh = m.math('DIVIDE', m.math('SUBTRACT', y, bh), 0.045)          # 0..1 up the shoulder
    sh_w = m.math('ADD', nw, m.math('MULTIPLY', m.math('SUBTRACT', bw, nw), m.math('SUBTRACT', 1.0, m.math('MINIMUM', m.math('MAXIMUM', sh, 0.0), 1.0))))
    shoulder = m.math('MULTIPLY', ramp(m, m.math('SUBTRACT', xm, sh_w), 0.002, -0.002), band(m, sh, 0.0, 1.0, 0.02))
    neck = m.math('MULTIPLY', ramp(m, m.math('SUBTRACT', xm, nw), 0.002, -0.002), band(m, y, bh, m.math('ADD', bh, 0.13), 0.003))
    present = m.math('MULTIPLY', ramp(m, r, 0.14, 0.15), ramp(m, y, 0.02, 0.024))
    bottle = m.math('MULTIPLY', m.math('MAXIMUM', body, m.math('MAXIMUM', shoulder, neck)), present)
    # Light: brightest in the middle of the wall and just above each shelf (its LED), the jambs darker.
    glow = m.math('MULTIPLY', ramp(m, m.math('ABSOLUTE', m.math('SUBTRACT', vv, 0.6)), 0.48, 0.05),
                  m.math('ADD', 0.5, m.math('MULTIPLY', ramp(m, m.math('MINIMUM', uu, m.math('SUBTRACT', 1.0, uu)), 0.0, 0.3), 0.5)))
    bay = m.math('ADD', 0.45, m.math('MULTIPLY', ramp(m, y, 0.3, 0.03), 0.55))
    onyx = noise(m, scaled(m, c.sep, 2.5, 2.5, 6.0), 1.0, 5.0, 0.6, 2.0)
    back = cmul(m, color_ramp(m, onyx, [(0.0, 0xc8601c), (0.45, 0xff8a3a), (0.65, 0xffa850), (1.0, 0xd8702a)]),
                m.math('MULTIPLY', m.math('ADD', 0.05, m.math('MULTIPLY', glow, 0.32)), bay))
    lit_b = m.math('GREATER_THAN', r2, 0.82)        # a few amber spirits glow with the backlight through them
    glass_c = color_ramp(m, r3, [(0.0, 0x0c1408), (0.35, 0x1a0c06), (0.6, 0x121008), (0.8, 0x1c1410), (1.0, 0x0e0e0c)])
    bcol = cmul(m, glass_c, m.math('ADD', 0.06, m.math('MULTIPLY', glow, 0.12)))
    bcol = m.mix(lit_b, bcol, cmul(m, hx(0xc8701c), m.math('ADD', 0.05, m.math('MULTIPLY', glow, 0.22))))
    rim = m.math('MULTIPLY', m.math('MULTIPLY', bottle, ramp(m, m.math('SUBTRACT', bw, xm), 0.006, 0.0)), ramp(m, y, 0.03, 0.08))
    bcol = m.mix(m.math('MULTIPLY', rim, 0.6), bcol, cmul(m, hx(0xffb070), m.math('ADD', 0.06, m.math('MULTIPLY', glow, 0.25))))
    em = m.mix(bottle, back, bcol)
    shelf = ramp(m, y, 0.016, 0.01)
    em = m.mix(shelf, em, cmul(m, hx(0xffd0a0), m.math('ADD', 0.12, m.math('MULTIPLY', glow, 0.5))))
    # The counter (the lower 0.9 m): dark walnut lit from above, its brass edge; the cove at the head; a dark plinth.
    counter = ramp(m, zc, 0.92, 0.88)
    walnut = cmul(m, m.mix(noise(m, scaled(m, c.sep, 1.0, 40.0, 3.0), 1.0, 4.0), hx(0x3a2214), hx(0x5a341c)),
                  m.math('ADD', 0.04, m.math('MULTIPLY', ramp(m, zc, 0.3, 0.88), 0.16)))
    em = m.mix(counter, em, ctimes_hex(m, walnut, 0xffb070))
    em = m.mix(band(m, zc, 0.88, 0.92, 0.004), em, cmul(m, hx(0xe0a050), 0.45))
    em = m.mix(ramp(m, vv, 0.955, 0.975), em, cmul(m, hx(0xffd8a8), 0.7))
    em = m.mix(ramp(m, vv, 0.985, 1.0), em, hx(0x050403))
    edge = ramp(m, m.math('MINIMUM', uu, m.math('SUBTRACT', 1.0, uu)), 0.0, 0.025)
    return cmul(m, em, edge), vv


def ctimes_hex(m, col, h):
    return m.mix(1.0, col, hx(h), blend='MULTIPLY')


def hotel_glass(name):
    """Hotel rooms, each window as seen straight on (spec B14): 55% dark (curtains drawn or the room empty), 35% lit
    warm by a bedside lamp over the bed with the curtains half drawn, 10% blue with the TV. ecx_win2.z: 0 dark, 1 warm,
    2 TV; the seed varies the walls, the lamp's side, the curtains and the bed."""
    m = core.Mat(name)
    c = C(m)
    uu, vv, seed, look, ww, wh, u = win_coords(m, c)
    s2 = m.math('FRACT', m.math('MULTIPLY', seed, 7.13))
    s3 = m.math('FRACT', m.math('MULTIPLY', seed, 3.71))
    s4 = m.math('FRACT', m.math('MULTIPLY', seed, 11.3))
    lu = lerp(m, m.math('GREATER_THAN', s2, 0.5), 0.18, 0.82)
    du = m.math('SUBTRACT', uu, lu)
    dv = m.math('SUBTRACT', vv, 0.4)
    e = m.math('DIVIDE', 1.0, m.math('ADD', 1.0, m.math('MULTIPLY', m.math('ADD', m.math('MULTIPLY', m.math('MULTIPLY', du, du), 2.5),
                                                                                   m.math('MULTIPLY', m.math('MULTIPLY', dv, dv), 5.0)), 10.0)))
    walls = color_ramp(m, s3, [(0.0, 0xd8c4a8), (0.35, 0xc9b9a0), (0.6, 0xb8b4ac), (0.85, 0xd2c0b0), (1.0, 0xa89c8c)])
    room = cmul(m, ctimes_hex(m, walls, 0xffb070), m.math('ADD', 0.18, m.math('MULTIPLY', e, 1.4)))
    bedside = m.math('GREATER_THAN', s2, 0.5)
    b0 = lerp(m, bedside, 0.32, 0.12)
    b1 = lerp(m, bedside, 0.9, 0.7)
    bed = m.math('MULTIPLY', band(m, uu, b0, b1, 0.01), ramp(m, vv, 0.27, 0.25))
    duvet = cmul(m, hx(0xf2ece2), m.math('ADD', 0.25, m.math('MULTIPLY', e, 1.1)))
    duvet = m.mix(ramp(m, vv, 0.2, 0.06), duvet, cmul(m, duvet, 0.55))
    room = m.mix(bed, room, duvet)
    head = m.math('MULTIPLY', band(m, uu, b0, b1, 0.01), band(m, vv, 0.27, 0.43, 0.006))
    room = m.mix(head, room, cmul(m, hx(0x3a2a20), m.math('ADD', 0.15, e)))
    shade = ramp(m, m.math('ADD', m.math('MULTIPLY', m.math('MULTIPLY', du, du), 60.0), m.math('MULTIPLY', m.math('POWER', m.math('SUBTRACT', vv, 0.48), 2.0), 90.0)), 1.0, 0.6)
    room = m.mix(shade, room, cmul(m, hx(0xffd49a), 3.0))
    room = m.mix(ramp(m, vv, 0.9, 0.97), room, cmul(m, room, 0.45))
    tv = cmul(m, m.mix(ramp(m, uu, 0.0, 1.0), hx(0x3a5ad0), hx(0x6f92ff)), m.math('ADD', 0.12, m.math('MULTIPLY', ramp(m, noise(m, vec(m, uu, vv, m.math('MULTIPLY', seed, 9.0)), 2.0, 2.0), 0.3, 0.8), 0.4)))
    tv = m.mix(m.math('MULTIPLY', bed, 0.7), tv, cmul(m, hx(0x8aa0e0), 0.3))
    lit = m.mix(is_k(m, look, 2), room, tv)
    cl = m.math('ADD', 0.04, m.math('MULTIPLY', s4, 0.34))
    cr = m.math('ADD', 0.04, m.math('MULTIPLY', m.math('FRACT', m.math('MULTIPLY', seed, 17.7)), 0.3))
    cur = m.math('MAXIMUM', m.math('SUBTRACT', 1.0, rampx(m, uu, cl, m.math('ADD', cl, 0.004))), rampx(m, uu, m.math('SUBTRACT', 1.0, m.math('ADD', cr, 0.004)), m.math('SUBTRACT', 1.0, cr)))
    folds = m.math('ADD', 0.6, m.math('MULTIPLY', m.math('SINE', m.math('MULTIPLY', m.math('MULTIPLY', uu, ww), 2 * math.pi / 0.11)), 0.25))
    sheer = cmul(m, ctimes_hex(m, hx(0xe8dcc6), 0xffc890), m.math('MULTIPLY', folds, m.math('ADD', 0.12, m.math('MULTIPLY', e, 0.5))))
    sheer = m.mix(is_k(m, look, 2), sheer, cmul(m, hx(0x6f86c8), m.math('MULTIPLY', folds, 0.18)))
    lit = m.mix(cur, lit, sheer)
    # Dark rooms: drawn blackout, a hint of the corridor light under nothing; a few with the sheers catching the city.
    dark = cmul(m, hx(0x1a1c22), m.math('MULTIPLY', m.math('GREATER_THAN', s3, 0.7), m.math('MULTIPLY', folds, 0.05)))
    em = m.mix(is_k(m, look, 0), lit, dark)
    edge = ramp(m, m.math('MINIMUM', m.math('MINIMUM', uu, m.math('SUBTRACT', 1.0, uu)), m.math('MINIMUM', vv, m.math('SUBTRACT', 1.0, vv))), 0.0, 0.03)
    em = cmul(m, em, edge)
    base = glass_base(m, c, vv)
    base = m.mix(m.math('MULTIPLY', is_k(m, look, 0), m.math('MULTIPLY', cur, 0.6)), base, hx(0x1e1d1c))
    m.set('Base Color', base)
    m.set('Roughness', 0.04)
    m.set('Emission Color', em)
    m.set('Emission Strength', 1.6)
    return m


def disc_glow(m, c, kind_disc, kind_halo):
    """The ember disc (EMBER_DEEP at the rim to FLAME at a low heart) and its halo wash, from ecx_c (center, Blender) and
    ecx_r (radius). Returns (emission color, mask)."""
    cen = A(m, 'ecx_c', 'Vector')
    r = m.math('MAXIMUM', A(m, 'ecx_r'), 0.01)
    d = m.math('DIVIDE', vmath(m, 'LENGTH', vmath(m, 'SUBTRACT', vmath(m, 'SUBTRACT', c.P, cen), vec(m, 0.0, 0.0, m.math('MULTIPLY', r, -0.18)))), r)
    disc = color_ramp(m, m.math('SUBTRACT', 1.0, d), [(0.0, brand.EMBER_DEEP), (0.25, brand.EMBER), (0.5, brand.EMBER_HOT), (0.75, brand.GOLD), (1.0, brand.FLAME)])
    d0 = m.math('DIVIDE', vmath(m, 'LENGTH', vmath(m, 'SUBTRACT', c.P, cen)), r)
    halo = cmul(m, hx(brand.EMBER_HOT), m.math('MULTIPLY', ramp(m, d0, 1.58, 1.0), 0.45))
    em = m.mix(kind_disc, hx(0x000000), cmul(m, disc, 0.9))
    em = m.mix(kind_halo, em, halo)
    return em


def lobby_lit(name):
    """Everything that glows in the lobby. ecx_k: 1 the crest's ember disc, 2 its halo on the wall, 3 the slot floor
    beyond the casino portal (blurred colored panels), 4 the desk's backlit onyx, 5 lit letter faces, 6 spruce
    needles (not lit), 7 soil, 8 the portal recess's warm walls."""
    m = core.Mat(name)
    c = C(m)
    kind = A(m, 'ecx_k')
    em = disc_glow(m, c, is_k(m, kind, 1), is_k(m, kind, 2))
    hue = noise(m, vec(m, m.math('MULTIPLY', c.x, 0.5), m.math('MULTIPLY', c.z, 0.7), 0.0), 1.0, 1.0)
    pal = color_ramp(m, hue, [(0.0, 0xff3a24), (0.3, 0xffb02e), (0.5, 0xff6a24), (0.65, 0xc24cff), (0.8, 0x3a7cff), (1.0, 0xffd060)])
    blobs = m.math('ADD', 0.25, m.math('MULTIPLY', ramp(m, noise(m, vec(m, m.math('MULTIPLY', c.x, 1.3), m.math('MULTIPLY', c.z, 1.8), 2.0), 1.4, 2.0), 0.35, 0.75), 1.1))
    slotfloor = m.mix(0.35, cmul(m, pal, blobs), cmul(m, hx(0xffb070), 0.5))
    em = m.mix(is_k(m, kind, 3), em, slotfloor)
    onyx = noise(m, scaled(m, c.sep, 3.0, 3.0, 9.0), 1.0, 6.0, 0.6, 2.5)
    onyx_c = color_ramp(m, onyx, [(0.0, 0x8a4a14), (0.4, 0xe0a050), (0.6, 0xffd090), (1.0, 0xb06a24)])
    em = m.mix(is_k(m, kind, 4), em, cmul(m, onyx_c, 0.9))
    em = m.mix(is_k(m, kind, 5), em, cmul(m, hx(0xfff0dc), 1.6))
    em = m.mix(is_k(m, kind, 8), em, cmul(m, hx(0xffb070), m.math('ADD', 0.15, m.math('MULTIPLY', noise(m, c.P, 1.5, 2.0), 0.2))))
    needles = m.mix(ramp(m, noise(m, c.P, 25.0, 3.0), 0.3, 0.7), hx(0x16261a), hx(0x24402a))
    base = m.mix(is_k(m, kind, 6), hx(0x121110), needles)
    base = m.mix(is_k(m, kind, 7), base, hx(0x1a120c))
    base = m.mix(is_k(m, kind, 4), base, hx(0x6a4a2a))
    m.set('Base Color', base)
    m.set('Roughness', lerp(m, is_k(m, kind, 6), 0.35, 0.8))
    m.set('Emission Color', em)
    m.set('Emission Strength', 2.5)
    return m


def grill_lit(name):
    """The Ember Grill seen through its storefront: each surface lit by the pendants hung over the tables (every 2.4 m
    along the room, 1.5 m in from the glass, at 2.9 m) over its own albedo. ecx_k: 1 plaster, 2 oak floor, 3 table
    walnut, 4 leather, 5 the back bar's lit shelves, 6 a pendant globe, 7 the dark timber ceiling, 8 the bar's copper
    front and marble top, 9 a diner's clothes, 10 a diner's head (skin, hair above 1.27 m); diners take colors from ecx_tone."""
    m = core.Mat(name)
    c = C(m)
    kind = A(m, 'ecx_k')
    X = m.math('MULTIPLY', c.x, -1.0)
    dy = m.math('MULTIPLY', m.math('SUBTRACT', m.math('FRACT', m.math('DIVIDE', m.math('SUBTRACT', c.y, 20.0), 2.4)), 0.5), 2.4)
    dx = m.math('SUBTRACT', X, 1.5)
    dz = m.math('SUBTRACT', c.z, 2.9)
    d2 = m.math('ADD', m.math('ADD', m.math('MULTIPLY', dx, dx), m.math('MULTIPLY', dy, dy)), m.math('MULTIPLY', dz, dz))
    light = m.math('ADD', 0.12, m.math('DIVIDE', 1.6, m.math('ADD', 1.0, m.math('DIVIDE', d2, 0.9))))
    lamp = hx(0xffb070)

    def lit(h, gain=1.0):
        return cmul(m, ctimes_hex(m, hx(h), 0xffb070), m.math('MULTIPLY', light, gain))
    plaster = lit(0xc8a888)
    oak = cmul(m, m.mix(noise(m, scaled(m, c.sep, 1.0, 30.0, 1.0), 1.0, 4.0), ctimes_hex(m, hx(0x6a4422), 0xffb070), ctimes_hex(m, hx(0x8a5a30), 0xffb070)), light)
    em = plaster
    shelves = m.math('ADD', 0.3, m.math('MULTIPLY', ramp(m, noise(m, scaled(m, c.sep, 1.0, 14.0, 3.0), 1.0, 2.0), 0.3, 0.8), 1.0))
    bottles = m.mix(ramp(m, noise(m, scaled(m, c.sep, 1.0, 22.0, 1.0), 1.0, 1.0), 0.4, 0.6), hx(0xffa040), hx(0xffd8a0))
    # Diners (9: clothes, 10: heads), each their own colors (ecx_tone), lit by the pendant over their table.
    tone = A(m, 'ecx_tone')
    cloth = color_ramp(m, tone, [(0.0, 0x1e2438), (0.2, 0x2a2f45), (0.35, 0x5a2a22), (0.5, 0x7a6450), (0.65, 0x3a3a3a), (0.8, 0x8a8478), (1.0, 0x23302a)])
    cloth = cmul(m, ctimes_hex(m, cloth, 0xffb070), m.math('MULTIPLY', light, 0.9))
    skin = color_ramp(m, m.math('FRACT', m.math('MULTIPLY', tone, 3.7)), [(0.0, 0x5a3a2a), (0.4, 0x9a6a50), (0.7, 0xc89a7a), (1.0, 0xd8b090)])
    hair = color_ramp(m, m.math('FRACT', m.math('MULTIPLY', tone, 5.3)), [(0.0, 0x0e0a08), (0.5, 0x2a1c12), (0.8, 0x5a4030), (1.0, 0x8a8278)])
    head = m.mix(ramp(m, c.z, 1.25, 1.29), skin, hair)
    head = cmul(m, ctimes_hex(m, head, 0xffb070), m.math('MULTIPLY', light, 0.9))
    for k, col in ((2, oak), (3, lit(0x4a2c1a)), (4, lit(0x2a1a14, 0.7)), (5, cmul(m, bottles, shelves)), (6, cmul(m, hx(0xffd8a8), 2.4)),
                   (7, lit(0x2a1c12, 0.5)), (8, lit(0xb87a48, 1.2)), (9, cloth), (10, head)):
        em = m.mix(is_k(m, kind, k), em, col)
    del lamp
    m.set('Base Color', hx(0x0e0d0c))
    m.set('Roughness', 0.4)
    m.set('Emission Color', em)
    m.set('Emission Strength', 2.0)
    return m


def decal_material(m, c, signs, base, glow_gain=1.0):
    """Printed signs (ecx_sign = id) laid on by position along their face (face u, z); returns (color, emission)."""
    sid = A(m, 'ecx_sign')
    u = c.face_u(m)
    col, emit = base, hx(0x000000)
    for s in signs:
        uu = m.math('DIVIDE', m.math('SUBTRACT', u, s['a0']), s['a1'] - s['a0'])
        vv = m.math('DIVIDE', m.math('SUBTRACT', c.z, s['z0']), s['z1'] - s['z0'])
        img = m.image(s['color'], vector=vec(m, uu, vv, 0.0), extension='EXTEND')
        mk = is_k(m, sid, s['id'])
        if s.get('ink') is not None:
            ink = m.mix(img.outputs['Alpha'], s['ground'], img.outputs['Color'])
            col = m.mix(mk, col, ink)
        if s.get('glow', 0.0) > 0.0:
            e = cmul(m, img.outputs['Color'], m.math('MULTIPLY', img.outputs['Alpha'], s['glow'] * glow_gain))
            emit = m.mix(mk, emit, e)
    return col, emit


# ------------------------------------------------------------------ the shared glow and glass collectors

GLOW = {}
GLASS = MB()


def reset_collectors():
    global GLASS
    GLOW.clear()
    for k in ('ember', 'warm', 'ash', 'lobby', 'tower', 'crown'):
        GLOW[k] = MB()
    GLASS = MB()


def pane(X0, X1, Y0, Y1, Z0, Z1):
    """A 2 cm glass pane (any axis-aligned box) in the podium glazing."""
    GLASS.box(X0, X1, Y0, Y1, Z0, Z1)


def u_of(frame, X, Y, Z=0.0):
    return (V(X, Y, Z) - frame[0]).dot(frame[1])


def urect(frame, p0, p1, z0, z1):
    """A wall rectangle from two map (X, Y) points along the face and a Z range."""
    a, b = u_of(frame, *p0), u_of(frame, *p1)
    return (min(a, b), max(a, b), z0, z1)


def disc_map(mb, X, Y, Z, r, n=24, down=True, **attrs):
    """A horizontal disc (a lens) at map (X, Y, Z), facing down (or up)."""
    c = V(X, Y, Z)
    idx = [mb.v((c.x + r * math.cos(2 * math.pi * i / n), c.y + r * math.sin(2 * math.pi * i / n), c.z)) for i in range(n)]
    mb.f(idx, want=(0, 0, -1 if down else 1), **attrs)


def ring_map(mb, X, Y, Z0, Z1, r0, r1, n=24, **attrs):
    """A downlight trim: a short flanged ring at map (X, Y) from Z0 up to Z1."""
    c = V(X, Y, 0.0)
    mb.lathe([(r0, Z1), (r0, Z0), (r1, Z0), (r1, Z0 + 0.006), (r0 + 0.01, Z1)], center=(c.x, c.y, 0.0), n=n, **attrs)


def rnd():
    return RNG.random()


# ------------------------------------------------------------------ B1 the entrance pavilion

def entry():
    k = Kit('SM_ECX_Entry', [basalt('en_basalt'), bronze('en_bronze'), timber('en_timber'), granite('en_granite'), frame_dark('en_frame')])
    BA, BR, TI, GR, FR = 0, 1, 2, 3, 4
    brz, tim, gra, frm = k.mb(BR), k.mb(TI), k.mb(GR), k.mb(FR)
    E, Wm = GLOW['ember'], GLOW['warm']
    n = 0.06   # the LED notch at the piers' corners
    for s in (-1, 1):
        yi, yo = 16.0 * s, 18.5 * s
        fr = frame_of((3.0, 0.0, 0.0), (1, 0, 0))
        r = urect(fr, (3.0, yi + n * s), (3.0, yo - n * s), 0.0, 0.6)
        clad(k, BA, fr, r, ecx_split=1, weight=1.2)
        clad(k, BA, fr, (r[0], r[1], 0.6, 12.8), weight=1.2)
        fs = frame_of((0.0, yi, 0.0), (0, -s, 0))
        r = urect(fs, (0.0, yi), (3.0 - n, yi), 0.0, 0.6)
        clad(k, BA, fs, r, ecx_split=1)
        clad(k, BA, fs, (r[0], r[1], 0.6, 12.8))
        r = urect(fs, (-4.5, yi), (0.0, yi), 0.0, 12.8)
        clad(k, BA, fs, r, weight=0.45)
        # The notches, lined dark, with the ember line in each.
        for yc, side in ((yi, -s), (yo, s)):
            ye = yc + n * s * (1 if yc == yi else -1)
            frm.quad_map([(3.0 - n, yc, 0.0), (3.0 - n, ye, 0.0), (3.0 - n, ye, 12.8), (3.0 - n, yc, 12.8)], (1, 0, 0))
            frm.quad_map([(3.0 - n, ye, 0.0), (3.0, ye, 0.0), (3.0, ye, 12.8), (3.0 - n, ye, 12.8)], (0, side, 0))
            if yc == yo:
                frm.quad_map([(3.0 - n, yc, 0.0), (3.0, yc, 0.0), (3.0, yc, 12.8), (3.0 - n, yc, 12.8)], (0, -s, 0))
            ym = (yc + ye) / 2
            E.box(3.0 - n, 3.0 - n + 0.022, ym - 0.011, ym + 0.011, 0.22, 12.58)
        # The pier's bronze cap.
        brz.box(-2.0, 3.05, min(yi - 0.004 * s, yo), max(yi - 0.004 * s, yo), 12.76, 12.86, cham=0.012)   # 4 mm proud of the inner face
        # Inside, the lobby's side walls run on from the pier's back to the lobby.
    # The glass wall: mullions on a 2 m grid, the door bays below the head.
    def mullion(Y, z0, z1):
        brz.box(-0.12, 0.14, Y - 0.05, Y + 0.05, z0, z1, cham=0.006)
        brz.box(0.14, 0.30, Y - 0.032, Y + 0.032, z0, z1, cham=0.01)
        # Black glazing gaskets where the glass meets the mullion.
        for sy in (-1, 1):
            frm.box(0.011, 0.022, Y + sy * 0.05, Y + sy * 0.057, z0, z1, ecx_k=2)
    for Y in range(6, 16, 2):
        for s in (-1, 1):
            mullion(Y * s, 0.15, 12.0)
    for Y in (-4, -2, 0, 2, 4):
        mullion(float(Y), 3.3, 12.0)
    for s in (-1, 1):
        brz.box(-0.12, 0.22, 15.9 * s, 16.0 * s, 0.15, 12.0, cham=0.006)
    for zc in (6.6, 9.8):
        brz.box(-0.10, 0.22, -16.0, 16.0, zc - 0.06, zc + 0.06, cham=0.008)
        for sz in (-1, 1):
            frm.box(0.011, 0.022, -16.0, 16.0, zc + sz * 0.06, zc + sz * 0.067, ecx_k=2)
    for s in (-1, 1):
        brz.box(-0.16, 0.26, 1.75 * s, 16.0 * s, 2.88, 3.3, cham=0.01)
        brz.box(0.255, 0.265, 1.8 * s, 15.95 * s, 3.05, 3.09, ecx_dk=1)
        # Sills under the fixed lites, the granite wet with melt.
        gra.box(-0.25, 0.42, 4.6 * s, 16.0 * s, 0.0, 0.15, cham=0.012, ecx_wet=1)
        # Door frames: the swing pairs' jambs and heads, the posts beside the drum.
        for Y in (1.75, 2.5, 4.6):
            frm.box(-0.08, 0.08, Y * s - 0.04, Y * s + 0.04, 0.012, 2.88, cham=0.005)
        frm.box(-0.08, 0.08, 1.75 * s, 4.6 * s, 2.76, 2.88, cham=0.004)
        frm.box(-0.05, 0.05, 1.79 * s, 2.46 * s, 0.012, 0.09, cham=0.004)
        # Floor pivots and closer covers at the swing doors' hinges.
        for Y in (2.5, 4.6):
            frm.box(-0.06, 0.06, Y * s - 0.09 * s, Y * s, 0.012, 0.018)
    gra.box(-0.62, 0.62, -4.6, 4.6, -0.01, 0.012, cham=0.004, ecx_wet=1)
    # The larch fins outside the clerestory, on bronze rails off the mullion caps.
    for i in range(32):
        Y = -15.5 + i
        tim.box(0.38, 0.95, Y - 0.0375, Y + 0.0375, 7.0, 12.0, cham=0.008, ecx_grain=2, ecx_weather=0.7, ecx_tone=rnd())
        for zc in (7.12, 9.8, 11.88):
            frm.box(0.30, 0.40, Y - 0.05, Y + 0.05, zc - 0.035, zc + 0.035, cham=0.004)
    for zc in (7.12, 9.8, 11.88):
        brz.box(0.30, 0.37, -16.0, 16.0, zc - 0.022, zc + 0.022, cham=0.006)
    # The bronze cornice: a stepped fascia with shadow lines, its soffit over the fins.
    brz.box(-0.3, 1.0, -16.0, 16.0, 12.0, 12.8, cham=0.012)
    brz.box(0.98, 1.07, -16.0, 16.0, 12.64, 12.82, cham=0.01)
    brz.box(0.98, 1.045, -16.0, 16.0, 12.0, 12.15, cham=0.008)
    brz.box(0.995, 1.004, -16.0, 16.0, 12.34, 12.37, ecx_dk=1)
    # The vestibule: basalt returns, a timber ceiling at 3.0 with downlights, a bronze fascia round its top.
    for s in (-1, 1):
        fo = frame_of((0.0, 5.1 * s, 0.0), (0, s, 0))
        clad(k, BA, fo, urect(fo, (-1.8, 5.1 * s), (-0.12, 5.1 * s), 0.0, 3.0), mod=(0.9, 0.5), weight=0.6)
        fi = frame_of((0.0, 4.6 * s, 0.0), (0, -s, 0))
        clad(k, BA, fi, urect(fi, (-1.8, 4.6 * s), (-0.12, 4.6 * s), 0.0, 3.0), mod=(0.9, 0.5), weight=0.6)
        k.mb(BA).box(-1.82, -1.8, 4.6 * s, 5.1 * s, 0.0, 3.0)
        k.mb(BA).box(-0.14, -0.12, 4.6 * s, 5.1 * s, 0.0, 3.0)
        for X in (-1.8, -3.1, -4.4):
            brz.box(X - 0.04, X + 0.04, 4.9 * s, 5.0 * s, 0.0, 3.0, cham=0.004)
        brz.box(-4.55, -0.12, 4.6 * s, 5.12 * s, 3.0, 3.36, cham=0.008)
        pane(-4.4, -1.8, 4.94 * s, 4.96 * s, 0.01, 2.99)
    brz.box(-4.6, -4.4, -5.12, 5.12, 3.0, 3.36, cham=0.008)
    k.mb(BA).box(-4.55, -0.12, -5.12, 5.12, 3.36, 3.4, ecx_polish=1, ecx_low=1)
    for Y in (-5.0, -2.0, 0.0, 2.0, 5.0):
        brz.box(-4.54, -4.46, Y - 0.04, Y + 0.04, 0.0, 3.0, cham=0.004)
    for a, b in ((-5.0, -2.0), (-2.0, 0.0), (0.0, 2.0), (2.0, 5.0)):
        pane(-4.51, -4.49, a, b, 0.01, 2.99)
    brz.box(-4.56, -4.44, -5.0, 5.0, 2.86, 3.0)
    # Ceiling slats along X, cut round the drum's canopy.
    pitch = 0.11
    nsl = int(9.2 / pitch)
    for i in range(nsl):
        Y = -4.6 + (i + 0.5) * 9.2 / nsl
        x_back = -4.5
        half = math.sqrt(max(1.74 ** 2 - Y * Y, 0.0)) if abs(Y) < 1.74 else 0.0
        x_front = -0.2 - half - 0.01 if half > 0 else -0.14
        tim.box(x_back, x_front, Y - 0.04, Y + 0.04, 2.98, 3.02, cham=0.006, ecx_grain=0, ecx_tone=rnd())
    frm.box(-4.5, -0.14, -4.6, 4.6, 3.03, 3.05, ecx_k=0)
    for X in (-2.75, -3.85):
        for Y in (-3.0, 0.0, 3.0):
            ring_map(frm, X, Y, 2.962, 2.982, 0.06, 0.085, n=24)
            disc_map(Wm, X, Y, 2.966, 0.062, n=24)
    # Floors behind the glass: honed granite in the vestibule and the side bays, a recessed walk-off mat.
    for X0, X1, Y0, Y1 in ((-4.5, -0.62, -4.6, 4.6), (-4.5, -0.25, 5.12, 16.0), (-4.5, -0.25, -16.0, -5.12)):
        nx_ = max(1, round((X1 - X0) / 0.6))
        ny_ = max(1, round((Y1 - Y0) / 0.6))
        for i in range(nx_):
            for j in range(ny_):
                a = X0 + (X1 - X0) * i / nx_
                b = X0 + (X1 - X0) * (i + 1) / nx_
                c = Y0 + (Y1 - Y0) * j / ny_
                d = Y0 + (Y1 - Y0) * (j + 1) / ny_
                gra.box(a + 0.002, b - 0.002, c + 0.002, d - 0.002, -0.02, 0.004, cham=0.002, skip=((2, -1),), ecx_k=1, ecx_tone=rnd())
    frm.box(-4.1, -2.1, -2.2, 2.2, -0.02, 0.006, skip=((2, -1),), ecx_k=3)
    # Inside: the slat ceiling at 12 m between the glass and the lobby, the roof over it.
    nsl = int(32.0 / 0.15)
    for i in range(nsl):
        Y = -16.0 + (i + 0.5) * 32.0 / nsl
        tim.box(-4.5, -0.3, Y - 0.03, Y + 0.03, 11.9, 12.0, cham=0.006, ecx_grain=0, ecx_tone=rnd(), ecx_low=1)
    frm.box(-4.5, -0.3, -16.0, 16.0, 12.0, 12.05, ecx_low=1)
    k.mb(BA).box(-4.5, -0.3, -16.0, 16.0, 12.05, 12.8, skip=((2, -1),), ecx_k=1, ecx_low=1)
    return k.finish()


def entry_glass():
    """The entrance's glass wall: fixed lites between the mullions and transoms, none where the doors and drum are."""
    zones = [(3.3, 6.54), (6.66, 9.74), (9.86, 12.0)]
    for b in range(16):
        Y0, Y1 = -16.0 + 2 * b, -14.0 + 2 * b
        for z0, z1 in zones:
            pane(-0.01, 0.01, Y0, Y1, z0, z1)
    for s in (-1, 1):
        for Y0 in (4.6, 6.0, 8.0, 10.0, 12.0, 14.0):
            Y1 = 6.0 if Y0 == 4.6 else Y0 + 2.0
            Y1 = min(Y1, 16.0)
            pane(-0.01, 0.01, min(Y0 * s, Y1 * s), max(Y0 * s, Y1 * s), 0.15, 2.88)
        pane(-0.01, 0.01, min(1.75 * s, 2.5 * s), max(1.75 * s, 2.5 * s), 0.09, 2.76)


# ------------------------------------------------------------------ the mark as geometry

def mark_parts(u, cx=0.0, cy=0.0):
    """brand.mark's shapes (same coordinates), in a 100-unit box centered at (cx, cy), unit u: 2D polygons."""
    base = cy - 28 * u

    def P(x, y):
        return (cx + x * u, base + y * u)
    Lb, Lp, V1, Cc, V2, Rp, Rb = P(-48, 0), P(-26, 34), P(-15, 22), P(0, 60), P(13, 29), P(25, 40), P(48, 0)
    return dict(rim=[P(-51, -1.5), P(-26, 37.5), P(-15, 25.5), P(0, 64), P(13, 32.5), P(25, 43.5), P(51, -1.5)],
                light=[Lb, Lp, V1, Cc, V2, Rp, Rb],
                shade=[[Lp, V1, P(-13, 0), P(-21, 0)], [Cc, V2, P(15, 0), P(3, 0)], [Rp, Rb, P(31, 0)]],
                snow=[Cc, P(-8, 43), P(-4.5, 46.5), P(-1.5, 42), P(1.6, 49)],
                ground=[(cx - 48 * u, base - 6.5 * u, cx - 3 * u, base - 4.1 * u), (cx + 3 * u, base - 6.5 * u, cx + 48 * u, base - 4.1 * u)],
                disc=(cx, base + 42 * u, 34 * u))


def plate(mb, pts2d, M, w0, w1, sides=True, **attrs):
    """A flat 2D outline (counter-clockwise, x along the face, y up) extruded from w0 to w1 out of a face: M maps
    (x, y, w) -> Blender (a face frame matrix: x along, y up, z out)."""
    n = len(pts2d)
    area = sum(pts2d[i][0] * pts2d[(i + 1) % n][1] - pts2d[(i + 1) % n][0] * pts2d[i][1] for i in range(n))
    if area < 0:
        pts2d = list(reversed(pts2d))
    a = [mb.v(M @ Vector((x, y, w0))) for x, y in pts2d]
    b = [mb.v(M @ Vector((x, y, w1))) for x, y in pts2d]
    out = (M.to_3x3() @ Vector((0.0, 0.0, 1.0))).normalized()
    mb.f(b, want=out, **attrs)
    if sides:
        for i in range(n):
            j = (i + 1) % n
            e = Vector(pts2d[j]) - Vector(pts2d[i])
            nrm = M.to_3x3() @ Vector((e.y, -e.x, 0.0))
            mb.f([a[i], a[j], b[j], b[i]], want=nrm, **attrs)


def circle2(cx, cy, r, n=48):
    return [(cx + r * math.cos(2 * math.pi * i / n), cy + r * math.sin(2 * math.pi * i / n)) for i in range(n)]


def annulus_plate(mb, cx, cy, r0, r1, M, w0, w1, n=64, **attrs):
    """A flat ring (r0..r1) extruded from w0 to w1 out of a face (as plate)."""
    out = (M.to_3x3() @ Vector((0.0, 0.0, 1.0))).normalized()
    ring = []
    for i in range(n):
        a = 2 * math.pi * i / n
        ring.append([mb.v(M @ Vector((cx + r * math.cos(a), cy + r * math.sin(a), w))) for r, w in ((r0, w1), (r1, w1), (r1, w0), (r0, w0))])
    for i in range(n):
        j = (i + 1) % n
        a, b = ring[i], ring[j]
        mb.f([a[0], a[1], b[1], b[0]], want=out, **attrs)
        rad = M.to_3x3() @ Vector((math.cos(2 * math.pi * (i + 0.5) / n), math.sin(2 * math.pi * (i + 0.5) / n), 0.0))
        mb.f([a[1], a[2], b[2], b[1]], want=rad, **attrs)
        mb.f([a[3], a[0], b[0], b[3]], want=-rad, **attrs)


def relief_mark(mbs, M, size, w, cx=0.0, cy=0.0, ember=True, attrs=None):
    """The mark in relief on a face: mbs = {'light', 'shade', 'snow', 'ground', 'disc', 'back'} builders (any may be
    None), M the face matrix, size the mark's box, w the depth of the light faces off the face."""
    u = size / 100.0
    mp = mark_parts(u, cx, cy)
    a = attrs or {}
    if mbs.get('back') is not None:
        plate(mbs['back'], mp['rim'], M, 0.0, w * 0.55, **a.get('back', {}))
    if mbs.get('light') is not None:
        plate(mbs['light'], mp['light'], M, w * 0.55, w, **a.get('light', {}))
    if mbs.get('shade') is not None:
        for poly in mp['shade']:
            plate(mbs['shade'], poly, M, w * 0.55, w + 0.004 * size / 6.0, sides=False, **a.get('shade', {}))
    if mbs.get('snow') is not None:
        plate(mbs['snow'], mp['snow'], M, w * 0.55, w + 0.008 * size / 6.0, sides=False, **a.get('snow', {}))
    if mbs.get('ground') is not None:
        for x0, y0, x1, y1 in mp['ground']:
            plate(mbs['ground'], [(x0, y0), (x1, y0), (x1, y1), (x0, y1)], M, 0.0, w * 0.8, **a.get('ground', {}))
    if ember and mbs.get('disc') is not None:
        dx, dy, r = mp['disc']
        plate(mbs['disc'], circle2(dx, dy, r, 160), M, 0.0, w * 0.3, **a.get('disc', {}))
    return mp


# ------------------------------------------------------------------ B3 the lobby, B4 its glow

def lobby():
    k = Kit('SM_ECX_Lobby', [marble('lb_floor'), basalt('lb_basalt', streaks=0.0, salt=0.0), bronze('lb_bronze', outdoor=0.0), timber('lb_timber'),
                             leather('lb_leather'), lobby_lit('lb_lit')])
    FL, BA, BR, TI, LE, LI = range(6)
    fl, bas, brz, tim, lit = k.mb(FL), k.mb(BA), k.mb(BR), k.mb(TI), k.mb(LI)
    G = GLOW['lobby']
    X0, X1 = -18.5, -4.5
    # The floor: polished black marble slabs, in four planar quarters.
    up = frame_of((0.0, 0.0, 0.0), (0, 0, 1))
    for ya, yb in ((-16.0, -8.0), (-8.0, 0.0), (0.0, 8.0), (8.0, 16.0)):
        clad(k, FL, up, urect(up, (X0, ya), (X1, yb), 0.0, 0.0)[:2] + (min(-X0, -X1), max(-X0, -X1)), mod=(1.2, 1.2), joint=0.003, cham=0.0015,
             stagger=False, lip=0.0003, depth=0.01)
    # The medallion: a bronze ring, the mark inlaid (it reads from the doors), copper rings.
    Mf = Matrix(((0.0, 1.0, 0.0, -(-11.5)), (-1.0, 0.0, 0.0, 0.0), (0.0, 0.0, 1.0, 0.0), (0.0, 0.0, 0.0, 1.0)))
    annulus_plate(brz, 0.0, 0.0, 2.32, 2.5, Mf, 0.0, 0.0016, n=96)
    annulus_plate(brz, 0.0, 0.0, 1.98, 2.02, Mf, 0.0, 0.0016, n=96, ecx_cu=1)
    for i in range(16):
        a = 2 * math.pi * i / 16
        L = 0.18 if i % 2 else 0.3
        plate(brz, [(math.cos(a) * 2.04 - math.sin(a) * 0.012, math.sin(a) * 2.04 + math.cos(a) * 0.012),
                    (math.cos(a) * 2.04 + math.sin(a) * 0.012, math.sin(a) * 2.04 - math.cos(a) * 0.012),
                    (math.cos(a) * (2.04 + L) + math.sin(a) * 0.004, math.sin(a) * (2.04 + L) - math.cos(a) * 0.004),
                    (math.cos(a) * (2.04 + L) - math.sin(a) * 0.004, math.sin(a) * (2.04 + L) + math.cos(a) * 0.004)], Mf, 0.0, 0.0016)
    relief_mark({'light': brz, 'snow': brz, 'ground': brz, 'disc': None}, Mf, 3.4, 0.0018, 0.0, 0.25, ember=False,
                attrs={'snow': {'ecx_cu': 1}, 'ground': {'ecx_cu': 1}})
    mp = mark_parts(0.034, 0.0, 0.25)
    annulus_plate(brz, mp['disc'][0], mp['disc'][1], mp['disc'][2] - 0.04, mp['disc'][2], Mf, 0.0, 0.0017, n=96, ecx_cu=1)
    # The east feature wall: stacked split-face basalt, the bronze crest with its ember disc lit behind as a halo.
    fw = frame_of((X0, 0.0, 0.0), (1, 0, 0))
    wid = k.new_wall(BA, fw, urect(fw, (X0, -16.0), (X0, 16.0), 0.0, 12.0), 1.0)
    rr = random.Random(77)
    z = 0.0
    while z < 12.0 - 1e-6:
        h = min(rr.choice((0.14, 0.18, 0.22, 0.26, 0.3, 0.36)), 12.0 - z)
        if 12.0 - z - h < 0.1:
            h = 12.0 - z
        y = -16.0
        while y < 16.0 - 1e-6:
            L = min(rr.uniform(0.35, 1.5), 16.0 - y)
            if 16.0 - y - L < 0.2:
                L = 16.0 - y
            d = rr.uniform(0.0, 0.055)
            a0, a1 = sorted((u_of(fw, X0, y), u_of(fw, X0, y + L)))
            panel(bas, fw, a0, a1, z, z + h, 0.008, min(d * 0.8 + 0.006, h * 0.3), d + 0.006, ecx_wall=wid, ecx_split=1, ecx_tone=rr.random())
            y += L
        z += h
    bas.poly([V(X0, -16.0, 0.0), V(X0, 16.0, 0.0), V(X0, 16.0, 12.0), V(X0, -16.0, 12.0)], want=VN(1, 0, 0), ecx_joint=1)
    Mw = face_matrix((X0 + 0.07, 0.0, 0.0), (1, 0, 0))
    dx, dy, r = mark_parts(0.06, 0.0, 6.0)['disc']
    cen = tuple(Mw @ Vector((dx, dy, 0.0)))
    # The disc's gradient and the halo read their center (ecx_c) and radius (ecx_r) from attributes.
    relief_mark({'back': None, 'light': brz, 'shade': brz, 'snow': brz, 'ground': brz, 'disc': lit},
                face_matrix((X0 + 0.16, 0.0, 0.0), (1, 0, 0)), 6.0, 0.22, 0.0, 6.0,
                attrs={'snow': {'ecx_cu': 1}, 'ground': {'ecx_cu': 1}, 'disc': {'ecx_k': 1, 'ecx_c': cen, 'ecx_r': r}})
    plate(lit, circle2(dx, dy, r * 1.6, 96), Mw, 0.0, 0.01, ecx_k=2, ecx_c=cen, ecx_r=r)
    # Standoffs behind the crest.
    for sx, sy in ((-1.5, 4.8), (1.5, 4.8), (0.0, 7.0), (-1.8, 6.3), (1.8, 6.4)):
        brz.tube(Mw @ Vector((sx, sy, 0.0)), Mw @ Vector((sx, sy, 0.12)), 0.03, n=10)
    # North and south walls: timber boards below the galleries, basalt above.
    for s in (-1, 1):
        fs = frame_of((-11.5, 16.0 * s, 0.0), (0, -s, 0))
        clad(k, BA, fs, urect(fs, (X0, 16.0 * s), (X1, 16.0 * s), 6.05, 12.0), weight=0.7)
        nb = int(14.0 / 0.16)
        for i in range(nb):
            Xb = X0 + (i + 0.5) * 14.0 / nb
            if s < 0 and -14.6 < Xb < -8.4:
                continue
            tim.box(Xb - 0.07, Xb + 0.07, 16.0 * s, (16.0 - 0.035) * s, 0.0, 5.55, cham=0.006, ecx_grain=2, ecx_tone=rnd())
        # The gallery: slab, bronze fascia, timber soffit, marble deck, glass rail with a bronze cap.
        yb = 12.5 * s
        brz.box(X0, X1, yb - 0.04 * s, yb + 0.03 * s, 5.5, 6.08, cham=0.006)
        fl.box(X0, X1, yb, 16.0 * s, 5.98, 6.0, ecx_low=1)
        nsl = int(3.5 / 0.12)
        for i in range(nsl):
            Yc = (12.55 + (i + 0.5) * 3.45 / nsl) * s
            tim.box(X0, X1, Yc - 0.035, Yc + 0.035, 5.5, 5.56, cham=0.006, ecx_grain=0, ecx_tone=rnd(), ecx_low=1)
        for i in range(10):
            xa = X0 + i * 1.4
            GLASS.box(xa + 0.02, xa + 1.38, (12.56 - 0.01) * s, (12.56 + 0.01) * s, 6.12, 7.08)
            brz.box(xa - 0.02, xa + 0.02, (12.53) * s, (12.6) * s, 6.08, 7.1, cham=0.003)
        brz.box(X0, X1, 12.52 * s, 12.6 * s, 7.08, 7.15, cham=0.01)
        brz.box(X0, X1, 12.53 * s, 12.59 * s, 6.06, 6.14, cham=0.004)
        G.box(X0, X1, 12.51 * s, 12.52 * s, 5.505, 5.53)
    # CASINO over the portal (on the south gallery's fascia), HOTEL on the wall behind the desk.
    t = text_obj('casino', 'CASINO', 'Bold', 0.3, tracking=1.35, depth=0.04, bevel=0.002)
    front, sides = split_text(t)
    Mt = face_matrix((-11.5, -12.46, 5.64), (0, 1, 0))
    front.data.transform(Mt)
    sides.data.transform(Mt)
    k.add(front, LI, ecx_k=5)
    k.add(sides, BR)
    t = text_obj('hotel', 'HOTEL', 'Bold', 0.42, tracking=1.4, depth=0.05, bevel=0.003)
    t.data.transform(face_matrix((-11.5, 15.94, 3.0), (0, -1, 0)))
    k.add(t, BR, ecx_cu=0)
    # The hotel desk: backlit onyx front, marble top, bronze base and edges.
    lit.box(-15.0, -8.0, 11.95, 12.0, 0.1, 1.0, ecx_k=4)
    brz.box(-15.05, -7.95, 11.98, 12.9, 0.0, 0.1, cham=0.006)
    brz.box(-15.05, -7.95, 11.92, 11.95, 0.08, 1.04, cham=0.004)
    bas.box(-15.0, -8.0, 12.0, 12.9, 0.1, 1.04, ecx_polish=1)
    fl.box(-15.1, -7.9, 11.88, 12.95, 1.04, 1.1, cham=0.008)
    # The casino portal: a bronze frame, and through it the glow of the slot floor.
    brz.box(-14.8, -14.5, -16.05, -15.6, 0.0, 4.9, cham=0.01)
    brz.box(-8.5, -8.2, -16.05, -15.6, 0.0, 4.9, cham=0.01)
    brz.box(-14.8, -8.2, -16.05, -15.6, 4.6, 4.9, cham=0.01)
    lit.box(-14.5, -8.5, -18.35, -18.3, 0.0, 4.6, ecx_k=3)
    lit.quad_map([(-14.5, -18.3, 0.0), (-14.5, -16.05, 0.0), (-14.5, -16.05, 4.6), (-14.5, -18.3, 4.6)], (1, 0, 0), ecx_k=8)
    lit.quad_map([(-8.5, -18.3, 0.0), (-8.5, -16.05, 0.0), (-8.5, -16.05, 4.6), (-8.5, -18.3, 4.6)], (-1, 0, 0), ecx_k=8)
    lit.quad_map([(-14.5, -18.3, 4.6), (-8.5, -18.3, 4.6), (-8.5, -16.05, 4.6), (-14.5, -16.05, 4.6)], (0, 0, -1), ecx_k=8)
    fl.box(-14.5, -8.5, -18.3, -16.0, -0.02, 0.0, skip=((2, -1),))
    for xx in (-13.5, -11.5, -9.5):
        G.box(xx - 0.4, xx + 0.4, -17.2, -17.1, 4.585, 4.595)
    # Armchairs in two pairs, a low table between each; two potted spruces by the feature wall.
    for ys in (-7.6, 7.6):
        for xs, face in ((-6.9, -1), (-9.3, 1)):
            chair(k, xs, ys, face)
        brz.box(-8.4, -7.8, ys - 0.5, ys + 0.5, 0.0, 0.36, cham=0.01)
        fl.box(-8.55, -7.65, ys - 0.65, ys + 0.65, 0.36, 0.42, cham=0.01)
    for ys in (-11.0, 11.0):
        spruce(k, -16.6, ys)
    # The ceiling: timber slats along X under a dark plenum, the chandelier's wires and canopy, downlight trims.
    nsl = int(32.0 / 0.15)
    for i in range(nsl):
        Y = -16.0 + (i + 0.5) * 32.0 / nsl
        tim.box(X0, X1, Y - 0.03, Y + 0.03, 11.88, 12.0, cham=0.006, ecx_grain=0, ecx_tone=rnd(), ecx_low=1)
    bas.box(X0, X1, -16.0, 16.0, 12.0, 12.8, skip=((2, -1),), ecx_k=1, ecx_low=1)
    # The roof strips between the lobby and the wings (behind the piers).
    for s in (-1, 1):
        bas.box(X0, -2.0, 16.03 * s, 18.5 * s, 12.4, 12.8, skip=((2, -1),), ecx_k=1, ecx_low=1)
    for X in (-6.0, -8.5, -14.5, -17.0):
        for Y in (-13.5, -10.0, -6.0, 6.0, 10.0, 13.5):
            ring_map(brz, X, Y, 11.84, 11.88, 0.07, 0.1, n=20)
            disc_map(G, X, Y, 11.845, 0.07, n=20)
    for X in (X0 + 0.05, X1 - 0.05):
        G.box(X - 0.02, X + 0.02, -15.9, 15.9, 11.84, 11.86)
    chandelier(brz, G)
    return k.finish()


def chair(k, X, Y, face):
    """A navy leather lounge chair at map (X, Y), facing map +X (face 1) or -X (face -1)."""
    LE, BR = 4, 2
    out = []

    def rb(name, cx, cy, cz, sx, sy, sz, r):
        o = parts.rbox(name, V(cx, cy, cz), (sx, sy, sz), r, 3)
        out.append(o)
        return o
    bx = X - face * 0.32
    rb('seat', X, Y, 0.38, 0.72, 0.62, 0.16, 0.06)
    rb('base', X, Y, 0.22, 0.74, 0.7, 0.2, 0.05)
    rb('back', bx, Y, 0.66, 0.16, 0.74, 0.52, 0.07)
    for s in (-1, 1):
        rb('arm', X - face * 0.02, Y + s * 0.34, 0.47, 0.76, 0.12, 0.3, 0.05)
    for o in out:
        k.add(o, LE)
    brz = k.mb(BR)
    for sx in (-1, 1):
        for sy in (-1, 1):
            p = V(X + sx * 0.3, Y + sy * 0.3, 0.0)
            brz.tube(p, p + Vector((0.0, 0.0, 0.13)), 0.018, 0.012, n=10)


def spruce(k, X, Y, h=3.6):
    """A potted spruce: a bronze planter, soil, a trunk and tiers of drooping needle skirts."""
    BR, LI = 2, 5
    brz, lit = k.mb(BR), k.mb(LI)
    c = V(X, Y, 0.0)
    brz.lathe([(0.0, 0.0), (0.5, 0.0), (0.62, 0.78), (0.6, 0.8), (0.55, 0.74), (0.0, 0.74)], center=(c.x, c.y, 0.0), n=40)
    lit.lathe([(0.0, 0.72), (0.56, 0.72), (0.5, 0.78), (0.0, 0.8)], center=(c.x, c.y, 0.0), n=24, ecx_k=7)
    tiers = 9
    rr = random.Random(int(abs(Y) * 10) + 3)
    for i in range(tiers):
        t = i / (tiers - 1)
        z0 = 1.0 + t * (h - 1.0) * 0.92
        rad = 0.85 * (1.0 - t) + 0.06
        prof = [(0.03, z0 + 0.12), (rad * 0.92, z0 - 0.12), (rad, z0 - 0.02), (rad * 0.55, z0 + 0.2), (0.03, z0 + 0.42)]
        lit.lathe(prof, center=(c.x + rr.uniform(-0.03, 0.03), c.y + rr.uniform(-0.03, 0.03), 0.0), n=14, ecx_k=6)
    lit.lathe([(0.0, h - 0.2), (0.05, h - 0.1), (0.0, h + 0.3)], center=(c.x, c.y, 0.0), n=8, ecx_k=6)


def chandelier(brz, G):
    """About 600 ember-glass rods in an 8 x 5 m cloud over the medallion, 7.5-11 m, each on a bronze wire."""
    rr = random.Random(1812)
    cx, cy = -11.5, 0.0
    n = 0
    tries = 0
    while n < 600 and tries < 20000:
        tries += 1
        ex, ey = rr.uniform(-1, 1), rr.uniform(-1, 1)
        q = ex * ex + ey * ey
        if q > 1.0:
            continue
        X, Y = cx + ex * 2.5, cy + ey * 4.0
        bottom = 7.5 + 1.9 * q + rr.uniform(-0.15, 0.15)
        L = rr.uniform(0.35, 1.15) * (1.0 - 0.45 * q)
        top = min(bottom + L, 10.9)
        a, b = V(X, Y, bottom), V(X, Y, top)
        G.tube(a, b, 0.016, n=14)
        brz.tube(b, V(X, Y, 11.86), 0.0018, n=4, caps=False)
        n += 1
    c = V(cx, cy, 11.86)
    brz.lathe([(2.6, 0.0), (2.62, 0.04), (2.4, 0.08)], center=(c.x, c.y, c.z - 0.08), n=72)


# ------------------------------------------------------------------ B5 / B6 the wings

WING_ZONES = ((0.45, 4.96), (5.04, 15.16), (15.24, 16.45))
REVEALS = ((4.96, 5.04), (15.16, 15.24))
# The Ember Grill's wine-room windows, north of the storefront under the terrace (centered between it and the corner).
WINE_Y = (38.25, 41.65, 45.05, 48.45, 51.85)
WINE_W, WINE_Z = 1.6, (0.95, 4.35)


def front_holes(fr, holes_map):
    """Map-space (Y0, Y1, Z0, Z1) holes on a front (X = const) frame -> frame rects."""
    out = []
    for Y0, Y1, Z0, Z1 in holes_map:
        a, b = u_of(fr, 3.0, Y0), u_of(fr, 3.0, Y1)
        out.append((min(a, b), max(a, b), Z0, Z1))
    return out


def channel(k, BA, BR, fr, u0, u1, z0, z1, depth=0.07, skip=(), led=True):
    """A recessed reveal channel (z0..z1) along a wall frame from u0 to u1 (minus skip ranges in u): a blackened bronze
    back, basalt top and bottom, the ember LED at its back."""
    spans = [(u0, u1)]
    for a, b in skip:
        nxt = []
        for c, d in spans:
            if b <= c or a >= d:
                nxt.append((c, d))
                continue
            if a > c:
                nxt.append((c, a))
            if b < d:
                nxt.append((b, d))
        spans = nxt
    for c, d in spans:
        if d - c < 0.01:
            continue
        fquad(k.mb(BR), fr, c, d, z0, z1, -depth, ecx_dk=1)
        k.mb(BA).poly([fpt(fr, c, z0, 0.0), fpt(fr, d, z0, 0.0), fpt(fr, d, z0, -depth), fpt(fr, c, z0, -depth)], want=fr[2])
        k.mb(BA).poly([fpt(fr, c, z1, 0.0), fpt(fr, d, z1, 0.0), fpt(fr, d, z1, -depth), fpt(fr, c, z1, -depth)], want=-fr[2])
        if led:
            zc = (z0 + z1) / 2
            fbox(GLOW['ember'], fr, c, d, zc - 0.014, zc + 0.014, -depth, -depth + 0.02)


def wing_shell(k, s, BA, BR, front_holes_map=(), granite=None, front_weight=1.0):
    """A wing's stone shell: the front (X 3.0), the outer return, the side over the entry's roof, the coping, the
    roof. Panels in three zones split by the two ember reveals."""
    yi, yo = 18.5 * s, (58.0 if s < 0 else 56.0) * s
    E = GLOW['ember']
    ff = frame_of((3.0, 0.0, 0.0), (1, 0, 0))
    ua, ub = sorted((u_of(ff, 3.0, yi), u_of(ff, 3.0, yo)))
    holes = front_holes(ff, front_holes_map)
    for z0, z1 in WING_ZONES:
        clad(k, BA, ff, (ua, ub, z0, z1), holes, weight=front_weight)
    for z0, z1 in REVEALS:
        # A reveal stops at every opening it would cross (the openings carry the line on their own transoms).
        skips = [(h[0], h[1]) for h in holes if h[2] < z1 and h[3] > z0]
        channel(k, BA, BR, ff, ua, ub, z0, z1, skip=skips)
    if granite is not None:
        lo_, hi_ = sorted((yi, yo))
        n_ = max(1, round((hi_ - lo_) / 1.2))
        for i_ in range(n_):
            y0_ = lo_ + (hi_ - lo_) * i_ / n_
            k.mb(granite).box(2.98, 3.06, y0_ + 0.004, y0_ + (hi_ - lo_) / n_ - 0.004, 0.0, 0.45, cham=0.012, ecx_wet=1, ecx_tone=rnd())
    else:
        clad(k, BA, ff, (ua, ub, 0.0, 0.45), ecx_split=1, weight=front_weight)
    # The outer return, 33 m deep.
    fo = frame_of((0.0, yo, 0.0), (0, s, 0))
    ra, rb = sorted((u_of(fo, -30.0, yo), u_of(fo, 3.0, yo)))
    for z0, z1 in WING_ZONES:
        clad(k, BA, fo, (ra, rb, z0, z1), weight=0.4)
    for z0, z1 in REVEALS:
        channel(k, BA, BR, fo, ra, rb, z0, z1)
    if granite is not None:
        a_, b_ = -30.0, 3.06
        n_ = max(1, round((b_ - a_) / 1.2))
        for i_ in range(n_):
            x0_ = a_ + (b_ - a_) * i_ / n_
            k.mb(granite).box(x0_ + 0.004, x0_ + (b_ - a_) / n_ - 0.004, yo - 0.02 * s, yo + 0.06 * s, 0.0, 0.45, cham=0.012, ecx_wet=1, ecx_tone=rnd())
    else:
        clad(k, BA, fo, (ra, rb, 0.0, 0.45), ecx_split=1, weight=0.4)
    # The side over the entry's roof, with the upper reveal.
    fi = frame_of((0.0, yi, 0.0), (0, -s, 0))
    ia, ib = sorted((u_of(fi, -30.0, yi), u_of(fi, 3.0, yi)))
    clad(k, BA, fi, (ia, ib, 12.8, 15.16), weight=0.3)
    clad(k, BA, fi, (ia, ib, 15.24, 16.45), weight=0.3)
    channel(k, BA, BR, fi, ia, ib, 15.16, 15.24)
    # Coping, roof.
    brz = k.mb(BR)
    brz.box(2.9, 3.08, min(yi, yo), max(yi, yo), 16.45, 16.62, cham=0.01)
    brz.box(-30.0, 3.08, yo - 0.18 * s, yo + 0.02 * s, 16.45, 16.62, cham=0.01)
    brz.box(-30.0, 3.08, yi - 0.02 * s, yi + 0.18 * s, 16.45, 16.62, cham=0.01)
    lo, hi = sorted((yi + 0.18 * s, yo - 0.18 * s))
    k.mb(BA).box(-30.0, 2.9, lo, hi, 15.9, 16.0, skip=((2, -1),), ecx_k=1, ecx_low=1)
    for Yp, sg in ((lo, 1), (hi, -1)):
        k.mb(BA).quad_map([(-30.0, Yp, 16.0), (2.9, Yp, 16.0), (2.9, Yp, 16.45), (-30.0, Yp, 16.45)], (0, sg, 0), ecx_low=1)
    k.mb(BA).quad_map([(2.9, lo, 16.0), (2.9, hi, 16.0), (2.9, hi, 16.45), (2.9, lo, 16.45)], (-1, 0, 0), ecx_low=1)
    del E
    return ff


def slot_window(k, ff, Yc, z0, z1, w, glass_slot, BR, seed, look=1, depth=0.4, transom=True):
    """A tall slot window in a front at X 3.0: a projecting bronze surround, bronze-lined reveal, a transom on the
    lower reveal line carrying the ember LED, dark glass with the room baked into it (ecx_win attributes)."""
    brz = k.mb(BR)
    gl = k.mb(glass_slot)
    Y0, Y1 = Yc - w / 2, Yc + w / 2
    f = 0.08
    for (a, b, c, d) in ((Y0 - f, Y0, z0 - f, z1 + f), (Y1, Y1 + f, z0 - f, z1 + f), (Y0, Y1, z0 - f, z0), (Y0, Y1, z1, z1 + f)):
        brz.box(3.0, 3.06, a, b, c, d, cham=0.008, seg=3)
    for (a, b, c, d) in ((Y0, Y0 + 0.02, z0, z1), (Y1 - 0.02, Y1, z0, z1), (Y0, Y1, z1 - 0.02, z1)):
        brz.box(3.0 - depth, 3.0, a, b, c, d)
    # The sill slopes out and down to shed the melt.
    brz.poly([V(3.0 - depth, Y0, z0 + 0.02), V(3.0 - depth, Y1, z0 + 0.02), V(3.0, Y1, z0), V(3.0, Y0, z0)], want=VN(0.05, 0, 1))
    wc = (V(3.0 - depth, Yc, 0.0)).dot(Vector((0.0, -1.0, 0.0)))
    attrs = dict(ecx_win=(wc, z0, seed), ecx_win2=(w, z1 - z0, look))
    gx = 3.0 - depth
    if transom:
        for (c, d) in ((z0, 4.94), (5.06, z1)):
            gl.quad_map([(gx, Y0, c), (gx, Y1, c), (gx, Y1, d), (gx, Y0, d)], (1, 0, 0), **attrs)
        brz.box(gx, 2.93, Y0, Y1, 4.94, 5.06, cham=0.006, seg=3)
        GLOW['ember'].box(2.93, 2.95, Y0 + 0.01, Y1 - 0.01, 4.986, 5.014)
    else:
        gl.quad_map([(gx, Y0, z0), (gx, Y1, z0), (gx, Y1, z1), (gx, Y0, z1)], (1, 0, 0), **attrs)
    # Glazing beads round the pane.
    for (a, b, c, d) in ((Y0, Y0 + 0.035, z0, z1), (Y1 - 0.035, Y1, z0, z1), (Y0, Y1, z0, z0 + 0.035), (Y0, Y1, z1 - 0.035, z1)):
        brz.box(gx, gx + 0.03, a, b, c, d, cham=0.006, seg=3)


def wing_s():
    k = Kit('SM_ECX_WingS', [basalt('ws_basalt'), bronze('ws_bronze'), casino_glass('ws_glass'), granite('ws_granite')])
    BA, BR, GL, GR = 0, 1, 2, 3
    k.region[0] = {'walls': (0.0, 0.0, 1.0, 0.86), 'rest': (0.0, 0.865, 0.88, 1.0), 'low': (0.885, 0.865, 1.0, 1.0)}
    wins = [-28.35, -26.15, -23.95, -21.75, -19.55, -56.7, -54.5, -52.3, -50.1, -47.9]
    holes = [(Y - 0.68, Y + 0.68, 1.42, 10.58) for Y in wins]
    holes.append((-46.6, -29.4, 4.4, 11.6))
    ff = wing_shell(k, -1, BA, BR, holes, granite=GR)
    rr = random.Random(5)
    for i, Y in enumerate(wins):
        slot_window(k, ff, Y, 1.5, 10.5, 1.2, GL, BR, rr.random())
    # Bronze fins between the windows.
    for grp in (wins[:5], wins[5:]):
        g = sorted(grp)
        for a, b in zip(g, g[1:]):
            Yf = (a + b) / 2
            k.mb(BR).box(3.0, 3.52, Yf - 0.04, Yf + 0.04, 1.2, 10.8, cham=0.012, seg=3)
            k.mb(BR).box(3.0, 3.08, Yf - 0.09, Yf + 0.09, 1.2, 10.8, cham=0.006, seg=3)
    # The marquee's recess: 4 cm deep, lined.
    rec = (-46.6, -29.4, 4.4, 11.6)
    a, b = sorted((u_of(ff, 3.0, rec[0]), u_of(ff, 3.0, rec[1])))
    fquad(k.mb(BA), ff, a, b, rec[2], rec[3], -0.04, ecx_low=1)
    reveal(k.mb(BA), ff, (a, b, rec[2], rec[3]), 0.04)
    return k.finish()


def gauss_blur(a, sigma):
    r = max(1, int(sigma * 3))
    x = np.arange(-r, r + 1, dtype=np.float32)
    kern = np.exp(-x * x / (2 * sigma * sigma))
    kern /= kern.sum()
    pad = np.pad(a, r, mode='constant')
    out = np.zeros_like(pad)
    for i, kv in enumerate(kern):
        out += kv * np.roll(pad, i - r, axis=0)
    pad = out
    out = np.zeros_like(pad)
    for i, kv in enumerate(kern):
        out += kv * np.roll(pad, i - r, axis=1)
    return out[r:-r, r:-r]


def sheet_png(s, name, px):
    """Renders a Sheet, deletes its objects, returns its color image as an (h, w, 4) array (row 0 at the bottom)."""
    path, _surface = s.render(name, px, samples=16)
    for obj, *_ in s.items:
        bpy.data.objects.remove(obj)
    img = bpy.data.images.load(path, check_existing=False)
    w, h = img.size
    a = np.empty(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(a)
    bpy.data.images.remove(img)
    return a.reshape(h, w, 4), path


def save_rgba(arr, path):
    h, w, _ = arr.shape
    img = bpy.data.images.new(os.path.basename(path), w, h, alpha=True)
    img.pixels.foreach_set(np.clip(arr, 0.0, 1.0).astype(np.float32).ravel())
    img.filepath_raw = path
    img.file_format = 'PNG'
    img.save()
    bpy.data.images.remove(img)
    return path


def halo_image(name, text, face, cap, tracking, w, h, base_y, color=0xff9a4a, sigma=0.12, px=1024):
    """A halo-lit sign's glow on the wall behind its letters: the text's silhouette, blurred (meters in, PNG out)."""
    s = Sheet(w * 1000.0, h * 1000.0)
    s.text(text, w * 500.0, base_y * 1000.0, cap * 1000.0 / cap_ratio(face), 0xffffff, face=face, align='CENTER', tracking=tracking)
    arr, _ = sheet_png(s, f'ecx_{name}_mask', px)
    a = arr[:, :, 3]
    sig = sigma * cap * px / w
    g = gauss_blur(a, sig)
    g2 = gauss_blur(a, sig * 3.0)
    glow = np.clip(g * 1.6 + g2 * 0.8, 0.0, 1.0) * (1.0 - 0.85 * a)
    out = np.zeros_like(arr)
    rgb = core.hex_linear(color)[:3]
    for i in range(3):
        out[:, :, i] = (rgb[i] ** (1 / 2.2))
    out[:, :, 3] = glow
    return save_rgba(out, os.path.join(core.BUILD_DIR, 'textures', f'ecx_{name}_halo.png'))


def plaque_image(name, w, h, lines, px=1024):
    """A cast bronze plaque: a raised border, the mark, ash letters."""
    s = Sheet(w * 1000.0, h * 1000.0)
    W, H = w * 1000.0, h * 1000.0
    s.rect(0, 0, W, H, 0x5d4129)
    s.rect(14, 14, W - 28, H - 28, 0x3a281a)
    s.rect(22, 22, W - 44, H - 44, 0x5a3f28)
    brand.mark(s, H * 0.55, H * 0.52, H * 0.62, mono=0xd8c8a8, ground=0x5a3f28, mono_ember=0xd88848)
    y = H * 0.56
    for txt, size, face in lines:
        s.text(txt, H * 0.98, y, size, 0xebe7df, face=face, align='LEFT', tracking=1.25)
        y -= size * 1.35
    arr, path = sheet_png(s, f'ecx_{name}_plaque', px)
    return path


def prints_material(name, signs):
    m = core.Mat(name)
    c = C(m)
    base = m.mix(ramp(m, noise(m, c.P, 0.6, 3.0), 0.3, 0.7), hx(0x24262b), hx(0x2b2d32))
    col, emit = decal_material(m, c, signs, base)
    m.set('Base Color', col)
    m.set('Roughness', 0.5)
    m.set('Metallic', lerp(m, is_k(m, A(m, 'ecx_sign'), 2), 0.0, 0.85))
    m.set('Emission Color', emit)
    m.set('Emission Strength', 3.0)
    return m


def wing_n():
    BA, BR, TI, GL, LI, PR = range(6)
    # The halo behind EMBER GRILL, and the POKER ROOM plaque.
    cap, trk = 0.62, 1.22
    t = text_obj('ember_grill', 'EMBER GRILL', 'Bold', cap, tracking=trk, depth=0.06, bevel=0.004)
    tw = t.dimensions.x
    hw, hh = tw + 1.4, cap + 1.0
    Yc, zb = 27.0, 6.25
    halo_p = halo_image('grill', 'EMBER GRILL', 'Bold', cap, trk, hw, hh, 0.5)
    plaque_p = plaque_image('poker', 0.9, 0.34, [('POKER ROOM', 92.0, 'Bold'), ('TERRACE  ·  LEVEL 2', 42.0, 'Medium')])
    signs = [dict(id=1, color=halo_p, a0=-(Yc + hw / 2), a1=-(Yc - hw / 2), z0=zb - 0.5, z1=zb - 0.5 + hh, glow=1.0),
             dict(id=2, color=plaque_p, a0=-(37.4 + 0.45), a1=-(37.4 - 0.45), z0=9.05, z1=9.39, ink=True, ground=hx(0x5d4129), glow=0.0)]
    k = Kit('SM_ECX_WingN', [basalt('wn_basalt'), bronze('wn_bronze'), timber('wn_timber'), poker_glass('wn_glass'), grill_lit('wn_lit'),
                             prints_material('wn_prints', signs)])
    k.region[0] = {'walls': (0.0, 0.0, 1.0, 0.86), 'rest': (0.0, 0.865, 0.88, 1.0), 'low': (0.885, 0.865, 1.0, 1.0)}
    brz, tim, gl, lit, pr = k.mb(BR), k.mb(TI), k.mb(GL), k.mb(LI), k.mb(PR)
    Wm = GLOW['warm']
    holes = [(19.9, 34.1, 0.0, 5.6), (20.0, 36.8, 8.0, 14.5), (38.0, 54.0, 8.0, 14.5)]
    holes += [(Y - WINE_W / 2 - 0.08, Y + WINE_W / 2 + 0.08, WINE_Z[0] - 0.08, WINE_Z[1] + 0.08) for Y in WINE_Y]
    ff = wing_shell(k, 1, BA, BR, holes)
    # --- The grill's wine room under the terrace: five deep bronze-boxed windows onto a backlit wall of bottles.
    rw = random.Random(73)
    for Y in WINE_Y:
        slot_window(k, ff, Y, WINE_Z[0], WINE_Z[1], WINE_W, GL, BR, rw.random(), look=2, depth=0.4, transom=False)
        y0, y1, z0, z1 = Y - WINE_W / 2 - 0.08, Y + WINE_W / 2 + 0.08, WINE_Z[0] - 0.08, WINE_Z[1] + 0.08
        for (a, b, c, d) in ((y0 - 0.1, y0, z0 - 0.1, z1 + 0.1), (y1, y1 + 0.1, z0 - 0.1, z1 + 0.1), (y0, y1, z1, z1 + 0.1)):
            brz.box(3.0, 3.26, a, b, c, d, cham=0.01, seg=3)
        brz.box(3.0, 3.3, y0 - 0.1, y1 + 0.1, z0 - 0.14, z0, cham=0.012, seg=3)
        brz.box(3.26, 3.264, y0 - 0.06, y1 + 0.06, z1 + 0.04, z1 + 0.052, ecx_cu=1)
        Wm.box(3.08, 3.24, Y - WINE_W / 2 + 0.05, Y + WINE_W / 2 - 0.05, z1 - 0.006, z1 - 0.001)
    # --- The Ember Grill's storefront, recessed 0.45 m, and its room.
    sf = (19.9, 34.1, 0.0, 5.6)
    a, b = sorted((u_of(ff, 3.0, sf[0]), u_of(ff, 3.0, sf[1])))
    reveal(brz, ff, (a, b, sf[2], sf[3]), 0.45, sides=(True, True, False, True))
    gx = 2.55
    for Y in range(20, 35, 2):
        brz.box(gx - 0.06, gx + 0.14, Y - 0.035, Y + 0.035, 0.0, 5.6, cham=0.006, seg=3)
    for z0, z1 in ((0.0, 0.12), (2.98, 3.1), (5.5, 5.6)):
        brz.box(gx - 0.05, gx + 0.12, 20.0, 34.0, z0, z1, cham=0.006)
    brz.box(gx - 0.05, 2.95, 20.0, 34.0, 4.94, 5.06, cham=0.006)
    Wm_e = GLOW['ember']
    Wm_e.box(2.95, 2.97, 19.95, 34.05, 4.986, 5.014)
    for Y0 in range(20, 34, 2):
        for z0, z1 in ((3.1, 4.94), (5.06, 5.5)):
            pane(gx - 0.01, gx + 0.01, Y0, Y0 + 2, z0, z1)
        if Y0 == 26:
            for ya, yb in ((26.03, 26.99), (27.01, 27.97)):
                pane(gx + 0.03, gx + 0.05, ya, yb, 0.03, 2.95)
                for yy in (ya, yb - 0.06):
                    brz.box(gx + 0.02, gx + 0.06, yy, yy + 0.06, 0.02, 2.96, cham=0.004)
                py = yb - 0.16 if ya < 27 else ya + 0.16
                brz.tube(V(gx + 0.13, py, 0.6), V(gx + 0.13, py, 1.9), 0.018, n=12)
                for zz in (0.75, 1.75):
                    brz.tube(V(gx + 0.06, py, zz), V(gx + 0.13, py, zz), 0.01, n=8)
        else:
            pane(gx - 0.01, gx + 0.01, Y0, Y0 + 2, 0.12, 2.98)
    for Y in range(21, 35, 2):
        ring_map(brz, 2.8, float(Y), 5.555, 5.6, 0.06, 0.085, n=20)
        disc_map(Wm, 2.8, float(Y), 5.56, 0.06, n=20)
    # The room: oak floor, plaster walls, a dark timber ceiling, the bar, tables under pendants, a few diners.
    X0, X1 = -0.4, gx - 0.07
    lit.quad_map([(X0, 20.0, 0.0), (X1, 20.0, 0.0), (X1, 34.0, 0.0), (X0, 34.0, 0.0)], (0, 0, 1), ecx_k=2)
    lit.quad_map([(X0, 20.0, 0.0), (X0, 34.0, 0.0), (X0, 34.0, 5.5), (X0, 20.0, 5.5)], (1, 0, 0), ecx_k=1)
    lit.quad_map([(X0, 20.0, 0.0), (X1, 20.0, 0.0), (X1, 20.0, 5.5), (X0, 20.0, 5.5)], (0, 1, 0), ecx_k=1)
    lit.quad_map([(X0, 34.0, 0.0), (X1, 34.0, 0.0), (X1, 34.0, 5.5), (X0, 34.0, 5.5)], (0, -1, 0), ecx_k=1)
    lit.quad_map([(X0, 20.0, 5.5), (X1, 20.0, 5.5), (X1, 34.0, 5.5), (X0, 34.0, 5.5)], (0, 0, -1), ecx_k=7)
    for i in range(14):
        lit.box(X0, X1, 20.0 + i + 0.45, 20.0 + i + 0.55, 5.3, 5.5, ecx_k=7)
    lit.box(X0 + 0.01, X0 + 0.03, 26.4, 33.6, 1.25, 2.7, ecx_k=5)
    for zz in (1.25, 1.7, 2.15, 2.6):
        lit.box(X0, X0 + 0.25, 26.4, 33.6, zz - 0.02, zz, ecx_k=8)
    lit.box(0.2, 0.75, 26.4, 33.4, 0.0, 1.05, ecx_k=8)
    lit.box(0.15, 0.85, 26.35, 33.45, 1.05, 1.11, ecx_k=8)
    for Y in (27.2, 28.4, 29.6, 30.8, 32.0, 33.0):
        lit.tube(V(1.1, Y, 0.0), V(1.1, Y, 0.72), 0.025, n=8, ecx_k=4)
        lit.tube(V(1.1, Y, 0.72), V(1.1, Y, 0.78), 0.19, n=14, ecx_k=4)
    rr = random.Random(41)
    rt = random.Random(4141)
    for Y in (21.2, 23.6, 30.8, 33.0):
        lit.tube(V(1.7, Y, 0.0), V(1.7, Y, 0.7), 0.04, n=8, ecx_k=3)
        lit.tube(V(1.7, Y, 0.7), V(1.7, Y, 0.75), 0.42, n=20, ecx_k=3)
        for dy in (-0.62, 0.62):
            lit.box(1.45, 1.95, Y + dy - 0.22, Y + dy + 0.22, 0.0, 0.46, ecx_k=4)
            lit.box(1.45, 1.95, Y + dy * 1.45 - 0.04, Y + dy * 1.45 + 0.04, 0.46, 0.95, ecx_k=4)
            if rr.random() < 0.55:
                # A seated diner facing the table (along Y): a torso broad across the shoulders, a neck and a head, their
                # own clothes, skin and hair (ecx_tone).
                cen = V(1.7, Y + dy * 1.15, 0.0)
                tn = rt.random()
                oval_lathe(lit, [(0.16, 0.12, 0.46), (0.155, 0.11, 0.72), (0.2, 0.115, 0.95), (0.215, 0.11, 1.03), (0.17, 0.095, 1.09),
                                 (0.07, 0.06, 1.12)], cen, n=16, ecx_k=9, ecx_tone=tn)
                lit.tube(cen + Vector((0.0, 0.0, 1.11)), cen + Vector((0.0, 0.0, 1.19)), 0.048, n=10, caps=False, ecx_k=10, ecx_tone=tn)
                lit.lathe([(0.0, 1.165), (0.06, 1.18), (0.092, 1.22), (0.1, 1.27), (0.092, 1.32), (0.06, 1.36), (0.0, 1.372)],
                          center=tuple(cen), n=14, ecx_k=10, ecx_tone=tn)
    for Y in (21.2, 23.6, 26.0, 28.4, 30.8, 33.2):
        lit.lathe([(0.0, 2.77), (0.11, 2.8), (0.14, 2.9), (0.11, 3.0), (0.0, 3.03)], center=tuple(V(1.5, Y, 0.0)), n=16, ecx_k=6)
        lit.tube(V(1.5, Y, 3.03), V(1.5, Y, 5.3), 0.006, n=4, caps=False, ecx_k=7)
    # --- EMBER GRILL: bronze letters off the wall on pins, the halo panel behind them.
    t.data.transform(face_matrix((3.0 + 0.05 + 0.03, Yc, zb), (1, 0, 0)))
    k.add(t, BR)
    fquad(pr, ff, -(Yc + hw / 2), -(Yc - hw / 2), zb - 0.5, zb - 0.5 + hh, 0.003, ecx_sign=1, ecx_low=1)
    # --- The poker room band: recessed dark glass with the card room in it, bronze mullions, larch louvres.
    for (Y0, Y1) in ((20.0, 36.8), (38.0, 54.0)):
        a, b = sorted((u_of(ff, 3.0, Y0), u_of(ff, 3.0, Y1)))
        reveal(brz, ff, (a, b, 8.0, 14.5), 0.3)
        gl.quad_map([(2.7, Y0, 8.0), (2.7, Y1, 8.0), (2.7, Y1, 14.5), (2.7, Y0, 14.5)], (1, 0, 0), ecx_win2=(1.0, 1.0, 0.0))
        Y = Y0
        while Y <= Y1 + 1e-6:
            brz.box(2.7, 2.86, Y - 0.03, Y + 0.03, 8.0, 14.5, cham=0.006)
            Y += 2.0 if Y1 - Y > 2.4 or abs(Y1 - Y) < 1e-6 else (Y1 - Y)
        for zc in (8.03, 12.0, 14.47):
            brz.box(2.7, 2.86, Y0, Y1, zc - 0.035, zc + 0.035, cham=0.006)
    for Y in (38.4, 40.2, 42.0, 43.8, 45.6):
        brz.box(2.7, 2.9, Y - 0.04, Y + 0.04, 8.0, 11.0, cham=0.006)
    brz.box(2.7, 2.9, 38.4, 45.6, 10.96, 11.06, cham=0.006)
    for Y in (39.1, 40.9, 42.7, 44.5):
        brz.tube(V(2.95, Y, 8.9), V(2.95, Y, 10.1), 0.016, n=10)
    rr = random.Random(9)
    Y = 20.25
    while Y < 54.0:
        if 36.75 < Y < 38.05:
            Y += 0.5
            continue
        z0 = 11.1 if 38.4 < Y < 45.6 else 8.08
        tim.box(3.05, 3.38, Y - 0.025, Y + 0.025, z0, 14.42, cham=0.006, seg=3, ecx_grain=2, ecx_weather=0.6, ecx_tone=rr.random())
        Y += 0.5
    for (Y0, Y1) in ((20.0, 36.8), (38.0, 54.0)):
        for zc in (8.04, 14.46):
            brz.box(3.0, 3.42, Y0, Y1, zc - 0.035, zc + 0.035, cham=0.008)
    brz.box(3.0, 3.42, 38.4, 45.6, 11.03, 11.09, cham=0.006)
    # The POKER ROOM plaque on the pier beside the terrace doors.
    k.mb(PR).box(3.0, 3.014, 36.95, 37.85, 9.05, 9.39, cham=0.003, ecx_sign=2)
    # --- The card room's terrace: a stone deck on bronze brackets, larch soffit, glass rail, two lanterns.
    k.mb(BA).box(3.0, 6.92, 34.0, 50.0, 7.86, 8.0, cham=0.01, ecx_k=2)
    k.mb(BA).box(3.0, 6.9, 34.02, 49.98, 7.72, 7.86, skip=((2, -1),), ecx_low=1)
    nb = int(16.0 / 0.12)
    for i in range(nb):
        Yb = 34.0 + (i + 0.5) * 16.0 / nb
        tim.box(3.0, 6.88, Yb - 0.045, Yb + 0.045, 7.68, 7.72, cham=0.006, ecx_grain=0, ecx_weather=0.3, ecx_tone=rnd())
    brz.box(6.88, 7.0, 34.0, 50.0, 7.6, 8.08, cham=0.008)
    for Y in (34.0, 50.0):
        brz.box(3.0, 7.0, Y - 0.06, Y + 0.06, 7.6, 8.08, cham=0.008)
    # Tapered brackets carrying the deck (clockwise outline in X, Z: outward is left of travel).
    for Y in (35.0, 42.0, 49.0):
        pts = [(3.0, 6.0), (3.0, 7.6), (6.7, 7.6), (6.7, 7.45)]
        ring = [brz.v(V(x, Y - 0.05, z)) for (x, z) in pts] + [brz.v(V(x, Y + 0.05, z)) for (x, z) in pts]
        n4 = len(pts)
        brz.f(ring[:n4], want=VN(0, -1, 0))
        brz.f(ring[n4:], want=VN(0, 1, 0))
        for i in range(n4):
            j = (i + 1) % n4
            e = (pts[j][0] - pts[i][0], pts[j][1] - pts[i][1])
            brz.f([ring[i], ring[j], ring[n4 + j], ring[n4 + i]], want=VN(-e[1], 0, e[0]))
    for i in range(10):
        ya = 34.08 + i * 1.584
        GLASS.box(6.86, 6.88, ya + 0.01, ya + 1.574, 8.1, 9.12)
    for Y in (34.08, 49.92):
        GLASS.box(3.06, 6.84, Y - 0.01, Y + 0.01, 8.1, 9.12)
    brz.box(6.8, 6.94, 34.06, 49.94, 8.0, 8.12, cham=0.006)
    brz.box(6.82, 6.92, 34.04, 49.96, 9.1, 9.17, cham=0.012)
    for Y in (34.08, 49.92):
        brz.box(3.0, 6.92, Y - 0.05, Y + 0.05, 9.1, 9.17, cham=0.012)
        brz.box(3.0, 6.94, Y - 0.06, Y + 0.06, 8.0, 8.12, cham=0.006)
    for Y in (36.0, 48.0):
        lantern(brz, Wm, 6.5, Y, 8.0)
    return k.finish()


def oval_lathe(mb, prof, cen, n=16, **attrs):
    """A closed solid of elliptical sections [(rx, ry, z)] (Blender x and y half-axes, bottom to top) about cen."""
    rings = [[mb.v((cen.x + rx * math.cos(2 * math.pi * i / n), cen.y + ry * math.sin(2 * math.pi * i / n), cen.z + z)) for i in range(n)]
             for rx, ry, z in prof]
    for A_, B_ in zip(rings, rings[1:]):
        for i in range(n):
            j = (i + 1) % n
            a = 2 * math.pi * (i + 0.5) / n
            mb.f([A_[i], A_[j], B_[j], B_[i]], want=(math.cos(a), math.sin(a), 0.0), **attrs)
    mb.f(rings[0], want=(0, 0, -1), **attrs)
    mb.f(rings[-1], want=(0, 0, 1), **attrs)


def lantern(brz, Wm, X, Y, z0):
    """A terrace lantern: a slim bronze post, a glass head (warm glow) in a bronze frame with a cap."""
    brz.box(X - 0.05, X + 0.05, Y - 0.05, Y + 0.05, z0, z0 + 1.22, cham=0.008)
    brz.box(X - 0.12, X + 0.12, Y - 0.12, Y + 0.12, z0, z0 + 0.03, cham=0.006)
    h0, h1 = z0 + 1.25, z0 + 1.7
    Wm.box(X - 0.12, X + 0.12, Y - 0.12, Y + 0.12, h0 + 0.02, h1 - 0.02)
    for sx in (-1, 1):
        for sy in (-1, 1):
            brz.box(X + sx * 0.14 - 0.015, X + sx * 0.14 + 0.015, Y + sy * 0.14 - 0.015, Y + sy * 0.14 + 0.015, h0, h1, cham=0.004)
    brz.box(X - 0.16, X + 0.16, Y - 0.16, Y + 0.16, h0 - 0.03, h0 + 0.01, cham=0.006)
    brz.box(X - 0.17, X + 0.17, Y - 0.17, Y + 0.17, h1, h1 + 0.04, cham=0.006)
    c = V(X, Y, 0.0)
    brz.lathe([(0.17, h1 + 0.04), (0.06, h1 + 0.16), (0.0, h1 + 0.18)], center=(c.x, c.y, 0.0), n=4, a0=math.pi / 4, a1=math.pi / 4 + 2 * math.pi)


# ------------------------------------------------------------------ B7 the porte-cochere (and its glow: B8-B10)

CN_X0, CN_X1, CN_Y = 1.0, 24.3, 18.2
SOFFIT = 7.0


def roof_z(X, Y):
    """The canopy's folded roof: 8.4 m at the back, 9.9 m at the front edge, a 0.22 m valley down its middle."""
    return 8.4 + 1.5 * (X - CN_X0) / (CN_X1 - CN_X0) - 0.22 * (1.0 - min(abs(Y), CN_Y) / CN_Y)


def smoothstep(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


def graded(a, b, fine, coarse, band, ends=(True, True)):
    """Sample positions from a to b: fine within band of the flagged ends, coarse in between (graded)."""
    out = [a]
    x = a
    while x < b - 1e-9:
        d = min(x - a if ends[0] else 1e9, b - x if ends[1] else 1e9)
        step = fine if d < band else min(coarse, fine + (d - band) * 0.35)
        x = min(x + step, b)
        out.append(x)
    return out


def snow_noise(X, Y):
    from mathutils import noise as mn
    return mn.noise(Vector((X * 0.35, Y * 0.35, 0.5))) * 0.6 + mn.noise(Vector((X * 1.7, Y * 1.7, 3.1))) * 0.25 + mn.noise(Vector((X * 6.0, Y * 6.0, 7.7))) * 0.08


def canopy():
    k = Kit('SM_ECX_Canopy', [timber('cn_timber'), bronze('cn_bronze'), basalt('cn_basalt', salt=1.0), roof_metal('cn_roof'), snow('cn_snow'),
                              ice('cn_ice'), frame_dark('cn_frame')])
    TI, BR, BA, RF, SN, IC, FR = range(7)
    tim, brz, bas, rf, sn, ic, frm = (k.mb(i) for i in range(7))
    E, Wm, Ash = GLOW['ember'], GLOW['warm'], GLOW['ash']
    lights = [(X, Y) for X in (4.5, 10.0, 15.5, 21.0) for Y in (-15.0, -9.0, -3.0, 3.0, 9.0, 15.0)]
    coves = (-17.2, -12.0, -6.0, 6.0, 12.0, 17.2)
    # --- The soffit: larch slats along X, 60 x 40 mm at 100 mm, over a black acoustic backing; six cove slots.
    pitch = 0.1
    Y = -CN_Y + 0.15 + 0.03
    rr = random.Random(24)
    while Y < CN_Y - 0.15 - 0.03 + 1e-6:
        if any(abs(Y - c) < 0.06 for c in coves):
            Y += pitch
            continue
        tim.box(CN_X0 + 0.05, CN_X1 - 0.16, Y - 0.03, Y + 0.03, SOFFIT, SOFFIT + 0.04, cham=0.005, ecx_grain=0, ecx_tone=rr.random(), ecx_weather=0.15)
        Y += pitch
    frm.box(CN_X0 + 0.05, CN_X1 - 0.16, -CN_Y + 0.15, CN_Y - 0.15, SOFFIT + 0.1, SOFFIT + 0.12, skip=((2, 1),), ecx_k=0)
    for c in coves:
        frm.box(CN_X0 + 0.3, CN_X1 - 0.4, c - 0.05, c - 0.04, SOFFIT, SOFFIT + 0.1)
        frm.box(CN_X0 + 0.3, CN_X1 - 0.4, c + 0.04, c + 0.05, SOFFIT, SOFFIT + 0.1)
        Wm.box(CN_X0 + 0.35, CN_X1 - 0.45, c - 0.018, c + 0.018, SOFFIT + 0.085, SOFFIT + 0.095)
    for X, Y in lights:
        ring_map(frm, X, Y, SOFFIT - 0.028, SOFFIT - 0.002, 0.09, 0.13, n=32)
        disc_map(Wm, X, Y, SOFFIT - 0.02, 0.09, n=32)
    # --- The bronze fascia: front 2.9 m, sides tapering to 1.4 m at the back; the ember line under its bottom edge.
    # The front is clad in 20 bronze panels (1.82 m) with 10 mm shadow gaps over a blackened backing, each panel its
    # own patina (ecx_ptone), and a continuous top band under the coping with a shadow line below it.
    skin = 0.05
    brz.box(CN_X1 - 0.15, CN_X1 - skin - 0.004, -CN_Y + 0.02, CN_Y - 0.02, SOFFIT + 0.045, 9.88, ecx_dk=1)
    npan = 20
    rp = random.Random(2430)
    for i in range(npan):
        ya = -CN_Y + 2 * CN_Y * i / npan
        yb = -CN_Y + 2 * CN_Y * (i + 1) / npan
        ga = 0.0 if i == 0 else 0.005
        gb = 0.0 if i == npan - 1 else 0.005
        brz.box(CN_X1 - skin, CN_X1, ya + ga, yb - gb, SOFFIT + 0.05, 9.735, cham=0.004, seg=2, ecx_ptone=rp.random())
    brz.box(CN_X1 - skin, CN_X1, -CN_Y, CN_Y, 9.745, 9.88, cham=0.004, seg=2, ecx_ptone=0.35)
    frm.box(CN_X1 - 0.16, CN_X1 - 0.03, -CN_Y + 0.03, CN_Y - 0.03, SOFFIT, SOFFIT + 0.045)
    E.box(CN_X1 - 0.032, CN_X1 - 0.008, -CN_Y + 0.02, CN_Y - 0.02, SOFFIT + 0.003, SOFFIT + 0.04)
    brz.box(CN_X1 - 0.17, CN_X1 + 0.022, -CN_Y - 0.022, CN_Y + 0.022, 9.88, 9.94, cham=0.008)
    xs_ = CN_X1 - skin - 0.002          # the side fascias stop behind the front skin (no coplanar corner faces)
    zs_ = roof_z(CN_X0, CN_Y) + (9.9 - roof_z(CN_X0, CN_Y)) * (xs_ - CN_X0) / (CN_X1 - CN_X0)
    for s in (-1, 1):
        yo = CN_Y * s
        yi = (CN_Y - 0.15) * s
        ring = []
        pts = [(CN_X0, SOFFIT + 0.045), (xs_, SOFFIT + 0.045), (xs_, zs_), (CN_X0, roof_z(CN_X0, CN_Y))]
        a = [brz.v(V(x, yi, z)) for x, z in pts]
        b = [brz.v(V(x, yo, z)) for x, z in pts]
        brz.f(b, want=VN(0, s, 0))
        brz.f(a, want=VN(0, -s, 0))
        for i in range(4):
            j = (i + 1) % 4
            e = (pts[j][0] - pts[i][0], pts[j][1] - pts[i][1])
            brz.f([a[i], a[j], b[j], b[i]], want=VN(e[1], 0, -e[0]))
        del ring
        frm.box(CN_X0 + 0.02, CN_X1 - 0.02, yo - 0.145 * s, yo - 0.03 * s, SOFFIT, SOFFIT + 0.045)
        E.box(CN_X0 + 0.04, CN_X1 - 0.02, yo - 0.032 * s, yo - 0.008 * s, SOFFIT + 0.003, SOFFIT + 0.04)
        # The coping along the side, sloping with the roof.
        za, zb = roof_z(CN_X0, CN_Y), 9.9
        brz.beam(V(CN_X0, yo - 0.075 * s, za + 0.03), V(CN_X1, yo - 0.075 * s, zb + 0.03), 0.19, 0.06, cham=0.006)
        for X in (4.0, 8.0, 12.0, 16.0, 20.0):
            brz.box(X - 0.006, X + 0.006, yo, yo + 0.003 * s, SOFFIT + 0.05, roof_z(X, CN_Y) - 0.02, ecx_dk=1)
    # --- The channel letters: lit faces 4 cm proud (GlowAsh), bronze returns.
    for txt, face, cap, trk, base in (('EMBERCREST', 'Black', 1.40, 1.22, 8.05), ('RESORT  ·  CASINO', 'Bold', 0.42, 1.9, 7.35)):
        t = text_obj('cn_' + txt[:6], txt, face, cap, tracking=trk, depth=0.04, bevel=0.0, resolution=10)
        print(f'[ecx_building] canopy letters {txt!r}: {t.dimensions.x:.2f} m wide')
        front, sides = split_text(t)
        M = face_matrix((CN_X1 + 0.02, 0.0, base), (1, 0, 0))
        front.data.transform(M)
        sides.data.transform(M)
        Ash.merge(mb_from(front))
        bpy.data.objects.remove(front)
        k.add(sides, BR)
    # --- The tree columns: split basalt plinths, tapering bronze trunks with an ember line, V-struts into the soffit.
    for Xc, Yc in ((18.5, -15.0), (18.5, -6.0), (18.5, 6.0), (18.5, 15.0)):
        bas.box(Xc - 0.7, Xc + 0.7, Yc - 0.7, Yc + 0.7, 0.0, 1.06, cham=0.03, ecx_split=1, ecx_tone=rnd())
        bas.box(Xc - 0.72, Xc + 0.72, Yc - 0.72, Yc + 0.72, 1.06, 1.1, cham=0.012, ecx_polish=1)
        c = V(Xc, Yc, 0.0)
        brz.lathe([(0.0, 1.1), (0.42, 1.1), (0.42, 1.14), (0.36, 1.18), (0.34, 1.32), (0.325, 1.36), (0.225, 4.45), (0.27, 4.5), (0.29, 4.62),
                   (0.27, 4.78), (0.16, 4.86), (0.0, 4.88)], center=(c.x, c.y, 0.0), n=64)
        r_at = lambda z: 0.325 + (0.225 - 0.325) * (z - 1.36) / (4.45 - 1.36)  # noqa: E731
        za, zb = 1.42, 4.38
        frm.beam(V(Xc + r_at(za) - 0.006, Yc, za), V(Xc + r_at(zb) - 0.006, Yc, zb), 0.05, 0.02, up=(-1, 0, 0))
        E.beam(V(Xc + r_at(za) + 0.006, Yc, za + 0.02), V(Xc + r_at(zb) + 0.006, Yc, zb - 0.02), 0.022, 0.012, up=(-1, 0, 0))
        for sx in (-1, 1):
            for sy in (-1, 1):
                top = V(Xc + 1.6 * sx, Yc + 1.6 * sy, SOFFIT - 0.06)
                bot = V(Xc + 0.12 * sx, Yc + 0.12 * sy, 4.74)
                brz.tube(bot, top, 0.12, 0.085, n=32)
                brz.box(Xc + 1.6 * sx - 0.24, Xc + 1.6 * sx + 0.24, Yc + 1.6 * sy - 0.24, Yc + 1.6 * sy + 0.24, SOFFIT - 0.07, SOFFIT - 0.004, cham=0.01)
                brz.tube(top, top + Vector((0.0, 0.0, 0.02)), 0.15, 0.15, n=24)
    # --- The steel beam where the canopy meets the entry, outriggers back to the mullions, a back plate.
    frm.box(CN_X0 - 0.02, CN_X0 + 0.42, -CN_Y, CN_Y, SOFFIT - 0.05, SOFFIT, cham=0.004)
    frm.box(CN_X0 - 0.02, CN_X0 + 0.42, -CN_Y + 0.01, CN_Y - 0.01, 7.55, 7.6, cham=0.004)
    frm.box(CN_X0 + 0.185, CN_X0 + 0.215, -CN_Y + 0.01, CN_Y - 0.01, SOFFIT, 7.55)
    frm.box(CN_X0 - 0.03, CN_X0, -CN_Y, CN_Y, 7.6, roof_z(CN_X0, 0.0) + 0.02)
    for Ys in (-14.0, -10.0, -6.0, -2.0, 2.0, 6.0, 10.0, 14.0):
        frm.box(0.30, CN_X0 - 0.02, Ys - 0.05, Ys + 0.05, 7.22, 7.4, cham=0.004)
    # --- The roof: dark standing-seam zinc on the fold (seen at its back edge), under the snow.
    nx = 24
    for side in (-1, 1):
        for i in range(nx):
            xa = CN_X0 + (CN_X1 - 0.15 - CN_X0) * i / nx
            xb = CN_X0 + (CN_X1 - 0.15 - CN_X0) * (i + 1) / nx
            ya, yb = 0.0, (CN_Y - 0.15) * side
            rf.poly([V(xa, ya, roof_z(xa, ya)), V(xb, ya, roof_z(xb, ya)), V(xb, yb, roof_z(xb, yb)), V(xa, yb, roof_z(xa, yb))], want=(0, 0, 1))
    Ys = -CN_Y + 0.4
    while Ys < CN_Y - 0.3:
        rf.beam(V(CN_X0, Ys, roof_z(CN_X0, Ys) + 0.02), V(CN_X0 + 0.6, Ys, roof_z(CN_X0 + 0.6, Ys) + 0.02), 0.025, 0.05)
        Ys += 0.5
    snow_cover(sn, ic)
    return k.finish()


def mb_from(obj, **attrs):
    """A builder holding obj's faces (transforms applied)."""
    mb = MB()
    me = obj.data
    base = 0
    for v in me.vertices:
        mb.v(obj.matrix_world @ v.co)
    for p in me.polygons:
        mb.f([base + i for i in p.vertices], **attrs)
    return mb


def snow_cover(sn, ic):
    """Snow 18-24 cm deep on the canopy's roof, deeper in the lee of the entry, rolled over the fascias in a soft lip
    that hangs 9 cm past them; icicles only at the two front corners."""
    over = 0.12
    xs = graded(CN_X0 + 0.06, CN_X1 + over, 0.025, 0.11, 0.5)
    ys = graded(-CN_Y - over, CN_Y + over, 0.025, 0.11, 0.5)

    def edge_d(X, Y):
        return min(CN_X1 - X, CN_Y - abs(Y))

    def base_z(X, Y):
        Xc = min(X, CN_X1)
        Yc = max(-CN_Y, min(CN_Y, Y))
        d = edge_d(Xc, Yc)
        cop = 9.94 if (CN_X1 - Xc) <= (CN_Y - abs(Yc)) else roof_z(Xc, CN_Y) + 0.06
        rz = roof_z(Xc, Yc)
        return rz + (max(cop, rz) - rz) * smoothstep(0.9, 0.0, d)

    def thick(X, Y):
        d = edge_d(X, Y)
        h = 0.2 + 0.035 * snow_noise(X, Y) + 0.07 * math.exp(-(X - CN_X0) / 2.5)
        t = h * math.sqrt(smoothstep(-over, 0.22, d))
        t *= math.sqrt(smoothstep(CN_X0 + 0.06, CN_X0 + 0.3, X)) * 0.9 + 0.1
        return t

    def droop(X, Y):
        d = edge_d(X, Y)
        return 0.09 * (max(-d, 0.0) / over) ** 1.5

    grid = []
    for X in xs:
        row = []
        for Y in ys:
            z = base_z(X, Y) - droop(X, Y) + thick(X, Y)
            row.append(sn.v(V(X, Y, z)))
        grid.append(row)
    for i in range(len(xs) - 1):
        for j in range(len(ys) - 1):
            sn.f([grid[i][j], grid[i + 1][j], grid[i + 1][j + 1], grid[i][j + 1]], want=(0, 0, 1))
    # The lip's underside, from each fascia's outer face out to the snow's edge; a skirt at the back.
    for i in range(len(xs) - 1):
        for j in range(len(ys) - 1):
            Xm, Ym = (xs[i] + xs[i + 1]) / 2, (ys[j] + ys[j + 1]) / 2
            if edge_d(Xm, Ym) >= 0.0:
                continue
            q = []
            for X, Y in ((xs[i], ys[j]), (xs[i + 1], ys[j]), (xs[i + 1], ys[j + 1]), (xs[i], ys[j + 1])):
                q.append(sn.v(V(X, Y, base_z(X, Y) - droop(X, Y) - 0.004)))
            sn.f(q, want=(0, 0, -1))
    X = xs[0]
    for j in range(len(ys) - 1):
        a, b = ys[j], ys[j + 1]
        sn.poly([V(X, a, roof_z(X, a)), V(X, b, roof_z(X, b)), V(X, b, base_z(X, b) + thick(X, b)), V(X, a, base_z(X, a) + thick(X, a))], want=VN(-1, 0, 0))
    # Icicles at the front corners only.
    rr = random.Random(99)
    for s in (-1, 1):
        spots = [(CN_X1 + 0.06, (CN_Y - rr.uniform(0.02, 1.3)) * s) for _ in range(9)]
        spots += [(CN_X1 - rr.uniform(0.0, 0.9), (CN_Y + 0.06) * s) for _ in range(6)]
        for X, Y in spots:
            ztop = base_z(X, Y) - droop(X, Y) + 0.01
            near = 1.0 - min(abs(abs(Y) - CN_Y) + abs(X - CN_X1), 1.3) / 1.3
            L = rr.uniform(0.08, 0.22) + near * rr.uniform(0.1, 0.42)
            r0 = 0.012 + L * 0.06
            ph = rr.uniform(0, 6.28)
            prof = []
            n = 9
            for kk in range(n + 1):
                t = kk / n
                r = r0 * (1.0 - t) ** 1.25 * (1.0 + 0.16 * math.sin(t * 15.0 + ph))
                prof.append((max(r, 0.0), ztop - L * t))
            prof = [(r, z) for r, z in reversed(prof)]
            prof[0] = (0.0, prof[0][1])
            c = V(X, Y, 0.0)
            ic.lathe(prof + [(0.0, ztop + 0.01)], center=(c.x, c.y, 0.0), n=10)


# ------------------------------------------------------------------ B11-B13 the doors (PROP frames, Blender coords)

def arc_band(mb, r0, r1, z0, z1, a0, a1, n, caps=True, **attrs):
    """A curved band (part of a cylinder wall r0..r1, z0..z1) from angle a0 to a1 (radians from +X)."""
    rings = []
    for i in range(n + 1):
        a = a0 + (a1 - a0) * i / n
        ca, sa = math.cos(a), math.sin(a)
        rings.append([mb.v((r * ca, r * sa, z)) for r, z in ((r1, z0), (r1, z1), (r0, z1), (r0, z0))])
    for i in range(n):
        A, B = rings[i], rings[i + 1]
        am = a0 + (a1 - a0) * (i + 0.5) / n
        rad = Vector((math.cos(am), math.sin(am), 0.0))
        mb.f([A[0], B[0], B[1], A[1]], want=rad, **attrs)
        mb.f([A[2], B[2], B[3], A[3]], want=-rad, **attrs)
        mb.f([A[1], B[1], B[2], A[2]], want=(0, 0, 1), **attrs)
        mb.f([A[3], B[3], B[0], A[0]], want=(0, 0, -1), **attrs)
    if caps:
        for R, a in ((rings[0], a0), (rings[-1], a1)):
            t = Vector((-math.sin(a), math.cos(a), 0.0))
            mb.f(R, want=-t if R is rings[0] else t, **attrs)


def mark_mask(name, px=1024, mono_ember=True):
    """The mark in white on transparent (an engraving's mask): the crest and the ember's ring, the rim parting them."""
    s = Sheet(100.0, 100.0)
    brand.mark(s, 50.0, 46.0, 92.0, mono=0xffffff, ground=0x000000, mono_ember=0xffffff if mono_ember else None)
    arr, path = sheet_png(s, f'ecx_{name}_mask', px)
    return path


def revolving_door():
    mask = mark_mask('rd')
    # The engraving: on the canopy ring's front (-X), the mark 0.36 m tall, centered at z 3.05.
    k = Kit('SM_ECX_RevolvingDoor', [bronze('rd_bronze', outdoor=0.5, engrave=(mask, -0.2, 0.2, 2.86, 3.26)), glass_fallback('rd_glass'),
                                     door_floor('rd_floor'), frame_dark('rd_frame')])
    BR, GLs, FL, FR = range(4)
    brz, gl, fl, frm = (k.mb(i) for i in range(4))
    R = 1.62
    q = math.pi / 4
    # Floor: honed granite with a brass compass ring and rose; the bronze ring under the walls.
    fl.lathe([(0.0, 0.0), (R - 0.03, 0.0), (R - 0.03, 0.02), (0.0, 0.02)], n=160)
    for r0, r1 in ((1.18, 1.22), (0.5, 0.52)):
        fl.lathe([(r0, 0.02), (r1, 0.02), (r1, 0.0215), (r0, 0.0215), (r0, 0.02)], n=160, ecx_cu=1)
    for i in range(8):
        a = 2 * math.pi * i / 8
        L = 0.48 if i % 2 == 0 else 0.3
        ca, sa = math.cos(a), math.sin(a)
        pts = [(0.1 * -sa, 0.1 * ca), (L * ca, L * sa), (0.1 * sa, -0.1 * ca)]
        fl.poly([(x, y, 0.0216) for x, y in pts], want=(0, 0, 1), ecx_cu=1)
    brz.lathe([(R - 0.03, 0.0), (R + 0.1, 0.0), (R + 0.1, 0.022), (R - 0.03, 0.022)], n=160)
    # The curved glass walls (sides +Y and -Y), bronze bottom rails, stiles at their ends.
    for a0, a1 in ((q, 3 * q), (5 * q, 7 * q)):
        arc_band(gl, R - 0.01, R + 0.01, 0.13, 2.79, a0, a1, 64)
        arc_band(brz, R - 0.03, R + 0.03, 0.022, 0.13, a0, a1, 64)
        for a in (a0, a1):
            c = Vector((math.cos(a) * R, math.sin(a) * R, 0.0))
            t = Vector((-math.sin(a), math.cos(a), 0.0))
            rad = Vector((math.cos(a), math.sin(a), 0.0))
            brz.obox(c + Vector((0, 0, 1.41)), (rad, t, Vector((0, 0, 1))), (0.045, 0.04, 1.39), 0.012, (), 3)
    # The canopy ring 2.8-3.3 m (the mark engraved on its front), its top, the black ceiling and the ring downlight.
    brz.lathe([(R + 0.1, 2.79), (R + 0.125, 2.81), (R + 0.125, 2.86), (R + 0.1, 2.88), (R + 0.1, 3.2), (R + 0.13, 3.225), (R + 0.13, 3.28), (R + 0.1, 3.3)], n=160)
    brz.lathe([(R + 0.1, 3.3), (R + 0.06, 3.32), (0.0, 3.32)], n=160)
    brz.lathe([(R - 0.02, 2.79), (R + 0.1, 2.79)], n=160)
    frm.lathe([(0.0, 2.79), (R - 0.02, 2.79)], n=160)
    frm.lathe([(1.08, 2.775), (1.22, 2.775), (1.22, 2.79), (1.08, 2.79), (1.08, 2.775)], n=160, ecx_k=1)
    frm.lathe([(0.0, 2.7), (0.13, 2.7), (0.13, 2.79), (0.0, 2.79)], n=32)
    brz.lathe([(0.0, 0.022), (0.15, 0.022), (0.14, 0.035), (0.0, 0.035)], n=32)
    return k.finish()


def door_floor(name):
    """The drum's floor: honed granite, brass inlays (ecx_cu)."""
    m = core.Mat(name)
    c = C(m)
    cu = A(m, 'ecx_cu')
    col = m.mix(ramp(m, noise(m, c.P, 240.0, 2.0), 0.6, 0.66), hx(0x8a8884), hx(0x2a2a2c))
    col = m.mix(ramp(m, noise(m, vmath(m, 'ADD', c.P, (3.3, 1.1, 7.7)), 300.0, 1.0), 0.68, 0.72), col, hx(0xd2d0ca))
    col = m.mix(cu, col, m.mix(ramp(m, noise(m, c.P, 30.0, 2.0), 0.3, 0.7), hx(0xb8913f), hx(0xd0a84c)))
    m.set('Base Color', col)
    m.set('Roughness', lerp(m, cu, 0.32, 0.22))
    m.set('Metallic', cu)
    return m


def revolving_wings():
    k = Kit('SM_ECX_RevolvingDoor_Wings', [bronze('rw_bronze', outdoor=0.3), glass_fallback('rw_glass'), parts.rubber('rw_rubber', 0x121212, 0.9)])
    BR, GLs, RU = range(3)
    brz, gl, ru = (k.mb(i) for i in range(3))
    brz.lathe([(0.0, 0.02), (0.08, 0.02), (0.08, 2.62), (0.0, 2.62)], n=40)
    for i in range(4):
        a = math.pi / 4 + i * math.pi / 2
        rad = Vector((math.cos(a), math.sin(a), 0.0))
        tan = Vector((-math.sin(a), math.cos(a), 0.0))
        up = Vector((0.0, 0.0, 1.0))

        def piece(mbx, r0, r1, z0, z1, t, cham=0.006, **attrs):
            mbx.obox(rad * ((r0 + r1) / 2) + up * ((z0 + z1) / 2), (rad, tan, up), ((r1 - r0) / 2, t / 2, (z1 - z0) / 2), cham, (), 3 if cham > 0 else None, **attrs)
        piece(brz, 0.08, 1.42, 0.03, 0.2, 0.05)
        piece(brz, 0.08, 1.42, 2.5, 2.6, 0.05)
        piece(brz, 1.36, 1.43, 0.03, 2.6, 0.05)
        piece(brz, 0.08, 0.15, 0.03, 2.6, 0.05)
        piece(gl, 0.15, 1.36, 0.2, 2.5, 0.012, 0.0)
        piece(ru, 1.43, 1.48, 0.05, 2.58, 0.02, 0.0)
        piece(ru, 0.1, 1.46, 0.0, 0.03, 0.02, 0.0)
        piece(ru, 0.1, 1.46, 2.6, 2.63, 0.02, 0.0)
        for sgn in (-1, 1):
            off = tan * (sgn * 0.065)
            brz.tube(rad * 0.4 + up * 1.05 + off, rad * 1.22 + up * 1.05 + off, 0.016, n=24)
            for r in (0.46, 1.16):
                brz.tube(rad * r + up * 1.05 + tan * (sgn * 0.006), rad * r + up * 1.05 + off, 0.009, n=8)
    return k.finish()


def swing_door():
    """One leaf: 100 mm bronze stiles, a 250 mm kick rail, full glass, a 1.2 m pull with a copper inlay outside, a push
    bar inside; pivots at the hinge. Hinge axis on the floor at the origin, the leaf along +Y, its outside facing -X."""
    k = Kit('SM_ECX_SwingDoor', [bronze('sd_bronze', outdoor=0.6), glass_fallback('sd_glass'), frame_dark('sd_frame')])
    BR, GLs, FR = range(3)
    brz, gl, frm = (k.mb(i) for i in range(3))
    W, H, T = 1.05, 2.75, 0.03
    brz.bbox(-T, T, 0.0, 0.1, 0.0, H, cham=0.006, seg=3)
    brz.bbox(-T, T, W - 0.1, W, 0.0, H, cham=0.006, seg=3)
    brz.bbox(-T, T, 0.1, W - 0.1, H - 0.1, H, cham=0.006, seg=3)
    brz.bbox(-T, T, 0.1, W - 0.1, 0.0, 0.25, cham=0.006, seg=3)
    gl.bbox(-0.006, 0.006, 0.1, W - 0.1, 0.25, H - 0.1)
    for (y0, y1, z0, z1) in ((0.1, 0.12, 0.25, H - 0.1), (W - 0.12, W - 0.1, 0.25, H - 0.1), (0.1, W - 0.1, 0.25, 0.27), (0.1, W - 0.1, H - 0.12, H - 0.1)):
        for x0, x1 in ((-T, -0.006), (0.006, T)):
            brz.bbox(x0 + (0.006 if x0 < 0 else 0.0), x1 - (0.006 if x1 > 0 else 0.0), y0, y1, z0, z1)
    py = W - 0.17
    brz.bbox(-T - 0.075, -T - 0.035, py - 0.02, py + 0.02, 0.55, 1.75, cham=0.008)
    brz.bbox(-T - 0.077, -T - 0.074, py - 0.006, py + 0.006, 0.6, 1.7, ecx_cu=1)
    for z in (0.62, 1.68):
        brz.bbox(-T - 0.04, -T, py - 0.012, py + 0.012, z - 0.012, z + 0.012, cham=0.003)
    frm.tube((T + 0.06, 0.15, 1.05), (T + 0.06, W - 0.15, 1.05), 0.016, n=14)
    for y in (0.2, W - 0.2):
        frm.tube((T, y, 1.05), (T + 0.06, y, 1.05), 0.01, n=8)
    frm.bbox(-0.035, 0.035, 0.0, 0.07, -0.004, 0.02, cham=0.003)
    frm.bbox(-0.035, 0.035, 0.0, 0.07, H - 0.02, H + 0.004, cham=0.003)
    return k.finish()


# ------------------------------------------------------------------ B14 the hotel tower (and the podium it stands on), B15

TW_X0, TW_X1 = -36.0, -56.0      # the tower's west (front) and east faces
TW_Y0, TW_Y1 = 10.0, 52.0
FLOORS = 13
F0, FH = 16.0, 3.5
PIER, REC_SP, REC_GL = 0.4, 0.15, 0.35   # pier width; spandrel and glass set back from the pier faces


def tower():
    k = Kit('SM_ECX_Tower', [basalt('tw_basalt', salt=0.0), bronze('tw_bronze'), hotel_glass('tw_glass'), roof_works('tw_roof')])
    BA, BR, GL, RF = range(4)
    bas, brz, gl, rf = (k.mb(i) for i in range(4))
    k.region[0] = {'walls': (0.0, 0.0, 1.0, 0.7), 'rest': (0.0, 0.705, 0.9, 1.0), 'low': (0.905, 0.705, 1.0, 1.0)}
    Et = GLOW['tower']
    E = GLOW['ember']
    rr = random.Random(1313)
    looks = []
    top = F0 + FLOORS * FH   # 61.5

    def look():
        r = rr.random()
        return 1 if r < 0.35 else (2 if r < 0.45 else 0)

    def window(frame, ua, ub, z0, z1, w_gl):
        """A window in a face frame between u = ua..ub at floor heights z0..z1, its glass at w_gl (frame offset)."""
        seed = rr.random()
        lk = look()
        c = fpt(frame, (ua + ub) / 2, z0, w_gl)
        n = frame[3]
        t = Vector((-n.y, n.x, 0.0)).normalized()
        attrs = dict(ecx_win=(c.dot(t), z0, seed), ecx_win2=(ub - ua, z1 - z0, lk))
        fquad(gl, frame, ua, ub, z0, z1, w_gl, **attrs)
        # Frame: perimeter and a vent mullion a third across.
        for a, b, c_, d in ((ua, ua + 0.05, z0, z1), (ub - 0.05, ub, z0, z1), (ua, ub, z0, z0 + 0.05), (ua, ub, z1 - 0.05, z1)):
            fbox(brz, frame, a, b, c_, d, w_gl, w_gl + 0.06, cham=0.006, seg=1)
        um = ua + (ub - ua) * (0.34 if seed > 0.5 else 0.66)
        fbox(brz, frame, um - 0.025, um + 0.025, z0, z1, w_gl, w_gl + 0.07, cham=0.006, seg=1)
        fbox(brz, frame, ua, ub, z0 + 0.95, z0 + 0.99, w_gl, w_gl + 0.05, cham=0.004)

    # The west face and the two long sides share one routine: piers, spandrels, windows; glass corner suites at the
    # two front corners (the corner pier left out); the east corners solid.
    faces = [((TW_X0, 0.0, 0.0), (1, 0, 0), (TW_X0, TW_Y1), (TW_X0, TW_Y0)),     # west: from the north corner to the south
             ((0.0, TW_Y0, 0.0), (0, -1, 0), (TW_X0, TW_Y0), (TW_X1, TW_Y0)),    # south: from the front to the back
             ((0.0, TW_Y1, 0.0), (0, 1, 0), (TW_X1, TW_Y1), (TW_X0, TW_Y1))]     # north: from the back to the front
    for origin, nrm, pa, pb in faces:
        fr = frame_of(origin, nrm)
        ua, ub = u_of(fr, *pa), u_of(fr, *pb)
        lo, hi = min(ua, ub), max(ua, ub)
        L = hi - lo
        # Where along u the front (west) corner is: for the west face both ends are front corners.
        if nrm[0] != 0:
            corners = (lo, hi)
            piers = [lo + 3.5 * j for j in range(1, 12)]
        else:
            fc = u_of(fr, TW_X0, origin[1])
            ec = u_of(fr, TW_X1, origin[1])
            corners = (fc,)
            step = 3.5 if ec > fc else -3.5
            piers = [fc + step * j for j in range(1, 6)]
            # The solid east end: a basalt pier block from the last pier to the corner.
            last = piers[-1]
            a, b = (last - PIER / 2, ec - 0.004) if ec > last else (ec + 0.004, last + PIER / 2)   # 4 mm inside the east face's panels
            for f in range(FLOORS):
                z0 = F0 + f * FH
                fbox(bas, fr, a, b, z0 + 0.001 * (f % 2), z0 + FH, -REC_GL - 0.05, 0.0, cham=0.012, ecx_tone=rr.random())
            Et.box(*_corner_led(origin, nrm, ec, fr, top))
        bays = sorted([*corners, *piers] if nrm[0] != 0 else [corners[0], *piers])
        # The bands stop flush with the other face's band at a glass corner.
        if nrm[0] != 0:
            blo, bhi = lo + REC_SP, hi - REC_SP
        else:
            blo, bhi = (lo + REC_SP, hi) if abs(fc - lo) < 1e-6 else (lo, hi - REC_SP)
        for f in range(FLOORS):
            z0 = F0 + f * FH
            for p in (piers if nrm[0] != 0 else piers[:-1]):
                fbox(bas, fr, p - PIER / 2, p + PIER / 2, z0 + 0.0005 * (f % 2), z0 + FH, -REC_GL - 0.05, 0.0, cham=0.012, ecx_tone=rr.random())
            # Spandrel band over the slab, its drip ledge.
            fbox(brz, fr, blo, bhi, z0 - (0.4 if f else 0.0), z0 + 0.4, -REC_GL, -REC_SP, cham=0.008)
            fbox(brz, fr, blo, bhi, z0 + 0.32, z0 + 0.4, -REC_SP, -REC_SP + 0.1, cham=0.006)
            for a, b in zip(bays, bays[1:]):
                ga = a + (REC_GL if a in corners else PIER / 2)
                gb = b - (REC_GL if b in corners else PIER / 2)
                if gb - ga < 0.5:
                    continue
                window(fr, ga, gb, z0 + 0.4, z0 + FH - 0.4, -REC_GL)
        fbox(brz, fr, blo, bhi, top - 0.4, top + 0.5, -REC_GL, -REC_SP + 0.02, cham=0.01)
        # The roof edge's ember line.
        fbox(Et, fr, blo, bhi, top - 0.03, top + 0.01, -REC_SP + 0.02, -REC_SP + 0.04)
    # Glass corner suites: a slim bronze corner post with the ember line, at the two front corners.
    for Yc in (TW_Y0, TW_Y1):
        sy = 1 if Yc == TW_Y0 else -1
        cx, cy = TW_X0 - REC_GL, Yc + REC_GL * sy
        brz.box(cx - 0.04, cx + 0.06, cy - 0.06 * sy, cy + 0.04 * sy, F0, top, cham=0.01)
        Et.box(cx + 0.06, cx + 0.08, cy - 0.01, cy + 0.01, F0 + 0.05, top - 0.05)
    # The plain east face.
    fe = frame_of((TW_X1, 0.0, 0.0), (-1, 0, 0))
    ea, eb = sorted((u_of(fe, TW_X1, TW_Y0), u_of(fe, TW_X1, TW_Y1)))
    clad(k, BA, fe, (ea, eb, F0, top + 0.5), mod=(1.75, 0.875), weight=0.25)
    # Roof: pavers, plant, the lift overrun; the crown stands over it.
    rf.box(TW_X1, TW_X0 - REC_GL, TW_Y0 + REC_GL, TW_Y1 - REC_GL, top + 0.3, top + 0.5, skip=((2, -1),), ecx_k=0)
    for (x0, x1, y0, y1, h) in ((-44.0, -49.0, 18.0, 24.0, 2.4), (-44.0, -49.0, 38.0, 44.0, 2.4), (-50.0, -53.5, 27.0, 35.0, 3.2)):
        rf.box(x1, x0, y0, y1, top + 0.5, top + 0.5 + h, cham=0.02, skip=((2, -1),), ecx_k=1)
        for i in range(int(abs(y1 - y0) / 0.25)):
            yy = y0 + 0.125 + i * 0.25
            rf.box(x0, x0 + 0.04, yy - 0.08, yy + 0.08, top + 0.8, top + 0.5 + h - 0.3, ecx_k=1)
    for Yc in (28.0, 34.0):
        c = V(-40.0, Yc, top + 0.5)
        rf.lathe([(1.3, 0.0), (1.3, 1.9), (1.1, 2.1), (0.0, 2.1)], center=(c.x, c.y, c.z), n=32, ecx_k=1)
    bas.box(-39.0, -42.0, 13.0, 17.0, top + 0.5, top + 4.6, cham=0.02, skip=((2, -1),), ecx_low=1)
    podium(k, BA, BR, RF)
    return k.finish()


def _corner_led(origin, nrm, ec, fr, top):
    """The ember line up a solid east corner of the tower (a box in map coords)."""
    p = fpt(fr, ec, 0.0, 0.0)
    X, Y = -p.x, p.y
    sx = 1 if X > (TW_X0 + TW_X1) / 2 else -1
    sy = 1 if Y > (TW_Y0 + TW_Y1) / 2 else -1
    del sx, sy
    return (X - 0.016, X + 0.016, Y - 0.016, Y + 0.016, F0 + 0.05, top - 0.05)


def podium(k, BA, BR, RF):
    """The podium behind the lobby and the wings (X -18.5/-30 .. -60): stone walls with the wings' two reveals carried
    round, a bronze coping, its roof. Seen from the far drive and the ridge, never up close."""
    E = GLOW['ember']
    walls = [((0.0, -58.0, 0.0), (0, -1, 0), (-30.0, -58.0), (-60.0, -58.0), 0.0),
             ((0.0, 56.0, 0.0), (0, 1, 0), (-60.0, 56.0), (-30.0, 56.0), 0.0),
             ((-60.0, 0.0, 0.0), (-1, 0, 0), (-60.0, -58.0), (-60.0, 56.0), 0.0),
             ((-18.5, 0.0, 0.0), (1, 0, 0), (-18.5, 18.5), (-18.5, -18.5), 12.8)]
    for origin, nrm, pa, pb, zb in walls:
        fr = frame_of(origin, nrm)
        a, b = sorted((u_of(fr, *pa), u_of(fr, *pb)))
        zones = [(0.45, 4.96), (5.04, 15.16), (15.24, 16.45)] if zb < 1.0 else [(zb, 15.16), (15.24, 16.45)]
        for z0, z1 in zones:
            clad(k, BA, fr, (a, b, z0, z1), mod=(1.2, 0.6), weight=0.12)
        for z0, z1 in REVEALS:
            if z0 > zb:
                channel(k, BA, BR, fr, a, b, z0, z1)
        if zb < 1.0:
            clad(k, BA, fr, (a, b, 0.0, 0.45), weight=0.12, ecx_split=1)
        fbox(k.mb(BR), fr, a, b, 16.45, 16.62, -0.16, 0.02, cham=0.01)
    # The roof at 16 m: membrane and gravel ballast, a few units.
    rf = k.mb(RF)
    for (x0, x1, y0, y1) in ((-30.0, -60.0, -57.84, 55.84), (-18.5, -30.0, -18.34, 18.34)):
        rf.box(x1, x0, y0, y1, 15.9, 16.0, skip=((2, -1),), ecx_k=2, ecx_low=1)
    for (x0, y0) in ((-40.0, -40.0), (-48.0, -20.0), (-44.0, -50.0)):
        rf.box(x0 - 3.0, x0, y0, y0 + 5.0, 16.0, 18.2, cham=0.02, skip=((2, -1),), ecx_k=1, ecx_low=1)
    del E


# ------------------------------------------------------------------ B16 the crown, B17 its glow, B18 the crest

CR_XF, CR_XB, CR_XA = -35.6, -56.4, -41.0    # the crown's front and back, and the line of its apexes
CR_Z0, CR_ZB = 62.0, 66.0                     # the tower roof, the top of the fin band
CR_Y0, CR_Y1 = 10.0, 52.0
# Peaks (apex Y, apex Z, south foot Y, north foot Y): the spec's heights; the feet put the valleys where the mark has them.
PEAKS = [(17.0, 72.67, 10.0, 17.0 + 6.67 / 0.44), (31.0, 78.0, 31.0 - 12.0 / 1.13, 31.0 + 12.0 / 1.25), (45.0, 71.07, 45.0 - 5.07 / 0.54, 52.0)]
CREST = dict(X=-35.5, depth=0.6, Y=31.0, Z=66.1, W=6.2, H=5.0)


def pyr(X, Y, p):
    Ya, Za, Y0, Y1 = p
    h = Za - CR_ZB
    t = max(h * max(X - CR_XA, 0.0) / (CR_XF - CR_XA), h * max(CR_XA - X, 0.0) / (CR_XA - CR_XB),
            h * max(Ya - Y, 0.0) / (Ya - Y0), h * max(Y - Ya, 0.0) / (Y1 - Ya))
    return Za - t


def crown_z(X, Y):
    return max(CR_ZB, max(pyr(X, Y, p) for p in PEAKS))


def simplify(pts, tol=1e-4):
    out = [pts[0]]
    for i in range(1, len(pts) - 1):
        a, b, c = out[-1], pts[i], pts[i + 1]
        cross = (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0])
        if abs(cross) > tol:
            out.append(b)
    out.append(pts[-1])
    return out


def rib(mb, Y, x_start=CR_XF, depth=0.3, half=0.03, **attrs):
    """A bronze rib following the crown's envelope along X at Y, depth thick under it (mitered), half wide either side."""
    n = int((x_start - CR_XB) / 0.05) + 1
    pts = simplify([(x_start - (x_start - CR_XB) * i / (n - 1), crown_z(x_start - (x_start - CR_XB) * i / (n - 1), Y)) for i in range(n)])
    segn = []
    for a, b in zip(pts, pts[1:]):
        dx, dz = b[0] - a[0], b[1] - a[1]
        L = math.hypot(dx, dz)
        segn.append((-dz / L, dx / L) if dx < 0 else (dz / L, -dx / L))
    bottom = []
    for i, p in enumerate(pts):
        if i == 0:
            nx, nz = segn[0]
        elif i == len(pts) - 1:
            nx, nz = segn[-1]
        else:
            ax, az = segn[i - 1]
            bx, bz = segn[i]
            sx, sz = ax + bx, az + bz
            L = math.hypot(sx, sz)
            sx, sz = sx / L, sz / L
            cosv = max(sx * bx + sz * bz, 0.35)
            nx, nz = sx / cosv, sz / cosv
        if nz < 0:
            nx, nz = -nx, -nz
        bottom.append((p[0] - nx * depth, p[1] - nz * depth))
    poly = pts + list(reversed(bottom))
    a = [mb.v(V(x, Y - half, z)) for x, z in poly]
    b = [mb.v(V(x, Y + half, z)) for x, z in poly]
    mb.f(a, want=(0, -1, 0), **attrs)
    mb.f(b, want=(0, 1, 0), **attrs)
    m = len(poly)
    area = sum(poly[i][0] * poly[(i + 1) % m][1] - poly[(i + 1) % m][0] * poly[i][1] for i in range(m))
    for i in range(m):
        j = (i + 1) % m
        dX, dZ = poly[j][0] - poly[i][0], poly[j][1] - poly[i][1]
        nX, nZ = (dZ, -dX) if area > 0 else (-dZ, dX)
        mb.f([a[i], a[j], b[j], b[i]], want=VN(nX, 0.0, nZ), **attrs)
    return pts


def crown():
    k = Kit('SM_ECX_Crown', [bronze('cr_bronze'), frame_dark('cr_frame'), roof_works('cr_roof')])
    BR, FR, RF = range(3)
    brz, frm, rf = (k.mb(i) for i in range(3))
    G = GLOW['crown']
    cy0, cy1 = CREST['Y'] - CREST['W'] / 2 - 0.1, CREST['Y'] + CREST['W'] / 2 + 0.1
    # The ribs: one at every 0.6 m across, on the faceted envelope; behind the crest they start clear of it.
    Ys = [CR_Y0 + 0.3 + 0.6 * i for i in range(70)]
    for Y in Ys:
        xs = CR_XF
        if cy0 < Y < cy1:
            xs = -36.25
        rib(brz, Y, xs)
        # Fixing cleats under the rib, every 1.4 m.
        X = xs - 0.7
        while X > CR_XB + 0.5:
            z = crown_z(X, Y)
            if z > CR_ZB + 0.6:
                frm.box(X - 0.05, X + 0.05, Y - 0.07, Y + 0.07, z - 0.42, z - 0.28)
            X -= 1.4
    # The fin band 62-66 m all round, its inner frame.
    for i, Y in enumerate(Ys):
        ztop = CR_ZB if not (cy0 < Y < cy1) else CREST['Z'] - CREST['H'] / 2 - 0.05
        brz.box(CR_XF - 0.3, CR_XF, Y - 0.03, Y + 0.03, CR_Z0, ztop, cham=0.008, seg=3)
        brz.box(CR_XB, CR_XB + 0.3, Y - 0.03, Y + 0.03, CR_Z0, CR_ZB, cham=0.008, seg=3)
    Xs = [CR_XF - 0.6 - 0.6 * j for j in range(34)]
    for X in Xs:
        for Yb, sg in ((CR_Y0, 1), (CR_Y1, -1)):
            brz.box(X - 0.03, X + 0.03, Yb, Yb + 0.3 * sg, CR_Z0, CR_ZB, cham=0.008, seg=3)
    for z0, z1 in ((CR_Z0, CR_Z0 + 0.22), (CR_ZB - 0.22, CR_ZB)):
        frm.box(CR_XF - 0.32, CR_XF - 0.52, CR_Y0 + 0.3, CR_Y1 - 0.3, z0, z1, cham=0.01)
        frm.box(CR_XB + 0.32, CR_XB + 0.52, CR_Y0 + 0.3, CR_Y1 - 0.3, z0, z1, cham=0.01)
        for Yb, sg in ((CR_Y0, 1), (CR_Y1, -1)):
            frm.box(CR_XB + 0.3, CR_XF - 0.3, Yb + 0.32 * sg, Yb + 0.52 * sg, z0, z1, cham=0.01)
    for X in (CR_XF - 0.42, CR_XB + 0.42):
        for Yb in (CR_Y0 + 0.42, CR_Y1 - 0.42):
            frm.box(X - 0.13, X + 0.13, Yb - 0.13, Yb + 0.13, CR_Z0, CR_ZB, cham=0.01)
    # Purlins: frame lines following the envelope along Y at fixed X, carrying the ribs.
    for Xp in (-37.2, -39.0, -41.0, -43.8, -47.5, -51.2, -54.6):
        prev = None
        for Y in [CR_Y0 + 0.3 + 0.3 * i for i in range(140)]:
            z = crown_z(Xp, Y) - 0.42
            p = V(Xp, Y, max(z, CR_ZB - 0.15))
            if prev is not None:
                frm.beam(prev, p, 0.1, 0.12)
            prev = p
    # Posts from the roof up into the peaks.
    for p in PEAKS:
        Ya, Za = p[0], p[1]
        for dy in (-3.0, 0.0, 3.0):
            for X in (-39.0, -43.8, -47.5):
                z = crown_z(X, Ya + dy) - 0.5
                if z > CR_Z0 + 1.0:
                    frm.box(X - 0.1, X + 0.1, Ya + dy - 0.1, Ya + dy + 0.1, CR_Z0, z, cham=0.008)
    # A catwalk inside the band.
    for (x0, x1, y0, y1) in ((CR_XF - 0.6, CR_XF - 1.6, CR_Y0 + 0.6, CR_Y1 - 0.6), (CR_XB + 0.6, CR_XB + 1.6, CR_Y0 + 0.6, CR_Y1 - 0.6)):
        rf.box(x1, x0, y0, y1, CR_Z0 + 0.05, CR_Z0 + 0.1, ecx_k=2)
    # The glow: the band stays dark bronze with the mark's ground line along its top (parted by the crest on the
    # front), the facets' inner glow shows between the ribs, the peaks' edge lines trace them.
    zl0, zl1 = CR_ZB - 0.16, CR_ZB - 0.08
    cy0, cy1 = CREST['Y'] - CREST['W'] / 2 - 0.05, CREST['Y'] + CREST['W'] / 2 + 0.05
    for a, b in ((CR_Y0 + 0.05, cy0), (cy1, CR_Y1 - 0.05)):
        G.box(CR_XF + 0.005, CR_XF + 0.04, a, b, zl0, zl1)
    G.box(CR_XB - 0.04, CR_XB - 0.005, CR_Y0 + 0.05, CR_Y1 - 0.05, zl0, zl1)
    for Yb, sg in ((CR_Y0, -1), (CR_Y1, 1)):
        G.box(CR_XB + 0.05, CR_XF - 0.05, Yb + 0.005 * sg, Yb + 0.04 * sg, zl0, zl1)
    inset = 0.55
    for Ya, Za, Y0, Y1 in PEAKS:
        apex = (CR_XA - 0.1, Ya, Za - 0.75)
        cs = [(CR_XF - inset, Y0 + inset, CR_ZB - 0.2), (CR_XF - inset, Y1 - inset, CR_ZB - 0.2),
              (CR_XB + inset, Y1 - inset, CR_ZB - 0.2), (CR_XB + inset, Y0 + inset, CR_ZB - 0.2)]
        for a, b in zip(cs, cs[1:] + cs[:1]):
            G.poly([V(*apex), V(*a), V(*b)], want=(0, 0, 1))
    for p in PEAKS:
        Ya, Za, Y0, Y1 = p
        for cX, cY in ((CR_XF, Y0), (CR_XF, Y1), (CR_XB, Y0), (CR_XB, Y1)):
            seg = []
            for i in range(25):
                t = i / 24
                X = CR_XA + (cX - CR_XA) * t
                Y = Ya + (cY - Ya) * t
                z = Za + (CR_ZB - Za) * t
                if z >= crown_z(X, Y) - 2e-3 and z > CR_ZB + 0.05:
                    seg.append(V(X, Y, z + 0.05))
                elif len(seg) > 1:
                    _line(G, seg)
                    seg = []
                else:
                    seg = []
            if len(seg) > 1:
                _line(G, seg)
    for (pa, pb) in ((PEAKS[0], PEAKS[1]), (PEAKS[1], PEAKS[2])):
        seg = []
        for i in range(41):
            X = CR_XF + (CR_XB - CR_XF) * i / 40
            lo, hi = pa[0], pb[0]
            for _ in range(40):
                mid = (lo + hi) / 2
                if pyr(X, mid, pa) > pyr(X, mid, pb):
                    lo = mid
                else:
                    hi = mid
            z = pyr(X, lo, pa)
            if z > CR_ZB + 0.05:
                seg.append(V(X, lo, z + 0.05))
            elif len(seg) > 1:
                _line(G, seg)
                seg = []
        if len(seg) > 1:
            _line(G, seg)
    return k.finish()


def _line(G, pts, r=0.035):
    for a, b in zip(pts, pts[1:]):
        if (b - a).length > 1e-4:
            G.tube(a, b, r, n=8, caps=False)


def crest_peaks(name):
    m = core.Mat(name)
    c = C(m)
    kind = A(m, 'ecx_k')
    n = noise(m, c.P, 30.0, 3.0)
    col = m.mix(n, hx(0xe4e0d8), hx(0xebe7df))
    col = m.mix(is_k(m, kind, 1), col, m.mix(n, hx(0x8590aa), hx(0x8d9ab4)))
    col = m.mix(is_k(m, kind, 2), col, hx(0xf8f8f6))
    m.set('Base Color', col)
    m.set('Roughness', m.math('ADD', 0.34, m.math('MULTIPLY', n, 0.08)))
    m.set('Coat Weight', 0.2)
    return m


def crest_ember(name):
    m = core.Mat(name)
    c = C(m)
    kind = A(m, 'ecx_k')
    em = disc_glow(m, c, is_k(m, kind, 1), m.math('MULTIPLY', 0.0, kind))
    em = m.mix(is_k(m, kind, 2), em, cmul(m, hx(brand.EMBER_HOT), 1.2))
    m.set('Base Color', hx(0x3a1406))
    m.set('Roughness', 0.3)
    m.set('Emission Color', em)
    m.set('Emission Strength', 2.2)
    return m


def crown_crest():
    """The crest on the crown's front: brand.mark in relief (ash and ice peaks, the snow facet), the ember disc lit with
    its gradient behind them, the ground lines lit ember, in a deep bronze frame round a midnight-navy enamel ground
    (the brand's own ground; blackened bronze read as a hole by day)."""
    k = Kit('SM_ECX_CrownCrest', [crest_peaks('cc_peaks'), crest_ember('cc_ember'), bronze('cc_bronze')])
    PK, EM, BR = range(3)
    pk, em, brz = (k.mb(i) for i in range(3))
    Xb = CREST['X'] - CREST['depth']
    W, H, Yc, Zc = CREST['W'], CREST['H'], CREST['Y'], CREST['Z']
    M = face_matrix((Xb, Yc, 0.0), (1, 0, 0))
    b = 0.18
    for (x0, x1, y0, y1) in ((-W / 2, W / 2, Zc - H / 2, Zc - H / 2 + b), (-W / 2, W / 2, Zc + H / 2 - b, Zc + H / 2),
                             (-W / 2, -W / 2 + b, Zc - H / 2 + b, Zc + H / 2 - b), (W / 2 - b, W / 2, Zc - H / 2 + b, Zc + H / 2 - b)):
        cxy = M @ Vector(((x0 + x1) / 2, (y0 + y1) / 2, CREST['depth'] / 2))
        brz.obox(cxy, (M.col[0].xyz, M.col[1].xyz, M.col[2].xyz), ((x1 - x0) / 2, (y1 - y0) / 2, CREST['depth'] / 2), 0.03, seg=3)
    plate(brz, [(-W / 2 + b, Zc - H / 2 + b), (W / 2 - b, Zc - H / 2 + b), (W / 2 - b, Zc + H / 2 - b), (-W / 2 + b, Zc + H / 2 - b)], M, 0.0, 0.04, ecx_enamel=1)
    # A copper line inset round the frame's face.
    i0, i1 = 0.07, 0.095
    for (x0, x1, y0, y1) in ((-W / 2 + i0, W / 2 - i0, Zc - H / 2 + i0, Zc - H / 2 + i1), (-W / 2 + i0, W / 2 - i0, Zc + H / 2 - i1, Zc + H / 2 - i0),
                             (-W / 2 + i0, -W / 2 + i1, Zc - H / 2 + i0, Zc + H / 2 - i0), (W / 2 - i1, W / 2 - i0, Zc - H / 2 + i0, Zc + H / 2 - i0)):
        plate(brz, [(x0, y0), (x1, y0), (x1, y1), (x0, y1)], M, CREST['depth'], CREST['depth'] + 0.004, ecx_cu=1)
    size = 5.49
    u = size / 100.0
    cy = Zc - 6.75 * u
    dx, dy, r = mark_parts(u, 0.0, cy)['disc']
    cen = tuple(M @ Vector((dx, dy, 0.08)))
    relief_mark({'back': None, 'light': pk, 'shade': pk, 'snow': pk, 'ground': em, 'disc': em}, M @ Matrix.Translation((0.0, 0.0, 0.04)), size, 0.36, 0.0, cy,
                attrs={'light': {'ecx_k': 0}, 'shade': {'ecx_k': 1}, 'snow': {'ecx_k': 2}, 'ground': {'ecx_k': 2}, 'disc': {'ecx_k': 1, 'ecx_c': cen, 'ecx_r': r}})
    mp = mark_parts(u, 0.0, cy)
    plate(brz, mp['rim'], M, 0.04 + 0.36 * 0.3 + 0.004, 0.04 + 0.36 * 0.55 - 0.004, ecx_enamel=1)
    # Brackets back to the crown's frame.
    for y in (-2.2, 2.2):
        for z in (Zc - 1.6, Zc + 1.6):
            p0 = M @ Vector((y, z, -0.32))
            p1 = M @ Vector((y, z, 0.0))
            brz.tube(p0, p1, 0.06, n=12, ecx_dk=1)
    return k.finish()


# ------------------------------------------------------------------ B19 the marquee, B20 its screen

MQ = dict(Y0=-46.6, Y1=-29.4, Z0=4.4, Z1=11.6, sY0=-46.0, sY1=-30.0, sZ0=5.0, sZ1=11.0, back=2.97, face=3.6, screen=3.58)


def marquee():
    k = Kit('SM_ECX_Marquee', [bronze('mq_bronze'), frame_dark('mq_frame'), basalt('mq_basalt')])
    BR, FR, BA = range(3)
    brz, frm, bas = (k.mb(i) for i in range(3))
    E = GLOW['ember']
    q = MQ
    fz = q['face']
    # The bezel: a deep bronze frame, chamfered into the opening, copper lines inset on its face.
    for (a, b, c, d) in ((q['Y0'], q['Y1'], q['Z0'], q['sZ0']), (q['Y0'], q['Y1'], q['sZ1'], q['Z1']),
                         (q['Y0'], q['sY0'], q['sZ0'], q['sZ1']), (q['sY1'], q['Y1'], q['sZ0'], q['sZ1'])):
        brz.box(fz - 0.15, fz, a, b, c, d, cham=0.012)
    for (a, b, c, d) in ((q['sY0'] - 0.02, q['sY1'] + 0.02, q['sZ0'] - 0.02, q['sZ0']), (q['sY0'] - 0.02, q['sY1'] + 0.02, q['sZ1'], q['sZ1'] + 0.02),
                         (q['sY0'] - 0.02, q['sY0'], q['sZ0'], q['sZ1']), (q['sY1'], q['sY1'] + 0.02, q['sZ0'], q['sZ1'])):
        # The liner stands 3 mm into the opening, so its inner faces never share a plane with the bezel's.
        frm.box(q['screen'] - 0.01, fz - 0.005, a - 0.003 * (a == q['sY1']), b + 0.003 * (b == q['sY0']), c - 0.003 * (c == q['sZ1']), d + 0.003 * (d == q['sZ0']))
    for inset in (0.08, 0.52):
        y0, y1, z0, z1 = q['Y0'] + inset, q['Y1'] - inset, q['Z0'] + inset, q['Z1'] - inset
        for (a, b, c, d) in ((y0, y1, z0, z0 + 0.025), (y0, y1, z1 - 0.025, z1), (y0, y0 + 0.025, z0, z1), (y1 - 0.025, y1, z0, z1)):
            brz.box(fz, fz + 0.003, a, b, c, d, ecx_cu=1)
    # The ember line inside the bezel, round the screen.
    for (a, b, c, d) in ((q['sY0'], q['sY1'], q['sZ0'] + 0.005, q['sZ0'] + 0.025), (q['sY0'], q['sY1'], q['sZ1'] - 0.025, q['sZ1'] - 0.005),
                         (q['sY0'] + 0.005, q['sY0'] + 0.025, q['sZ0'], q['sZ1']), (q['sY1'] - 0.025, q['sY1'] - 0.005, q['sZ0'], q['sZ1'])):
        E.box(q['screen'] + 0.004, q['screen'] + 0.016, a, b, c, d)
    # The mark in bronze on the header, copper ground lines.
    M = face_matrix((fz, (q['Y0'] + q['Y1']) / 2, 0.0), (1, 0, 0))
    relief_mark({'light': brz, 'shade': brz, 'snow': brz, 'ground': brz, 'disc': None, 'back': None}, M, 0.5, 0.012, 0.0, (q['sZ1'] + q['Z1']) / 2 + 0.02,
                ember=False, attrs={'shade': {'ecx_dk': 1}, 'ground': {'ecx_cu': 1}, 'snow': {'ecx_cu': 1}})
    annulus_plate(brz, 0.0, (q['sZ1'] + q['Z1']) / 2 + 0.02 - 28 * 0.005 + 42 * 0.005, 0.15, 0.17, M, 0.0, 0.008, n=48, ecx_cu=1)
    # The louvred back box behind the bezel.
    frm.box(q['back'], fz - 0.15, q['Y0'] + 0.1, q['Y1'] - 0.1, q['Z0'] + 0.1, q['Z1'] - 0.1, cham=0.01)
    for i in range(int((q['Y1'] - q['Y0'] - 0.4) / 0.11)):
        y = q['Y0'] + 0.25 + i * 0.11
        frm.box(q['back'] + 0.05, fz - 0.2, y - 0.012, y + 0.012, q['Z0'] + 0.06, q['Z0'] + 0.1, cham=0.004)
        frm.box(q['back'] + 0.05, fz - 0.2, y - 0.012, y + 0.012, q['Z1'] - 0.1, q['Z1'] - 0.06, cham=0.004)
    for i in range(int((q['Z1'] - q['Z0'] - 0.4) / 0.09)):
        z = q['Z0'] + 0.25 + i * 0.09
        for yb, sg in ((q['Y0'] + 0.1, -1), (q['Y1'] - 0.1, 1)):
            frm.box(q['back'] + 0.05, fz - 0.2, yb + sg * 0.01, yb + sg * 0.04, z - 0.012, z + 0.012, cham=0.004)
    # A basalt sill under it, with a bronze drip.
    bas.box(q['back'], 3.78, q['Y0'] - 0.2, q['Y1'] + 0.2, 4.18, 4.4, cham=0.015, ecx_polish=1)
    brz.box(3.7, 3.8, q['Y0'] - 0.2, q['Y1'] + 0.2, 4.15, 4.2, cham=0.006)
    return k.finish()


def marquee_screen():
    """One quad, 16 x 6 m, facing the plaza; UV u = 0 at its north (left) edge, v = 0 at its foot."""
    q = MQ
    me = bpy.data.meshes.new('SM_ECX_Marquee_Screen')
    vs = [V(q['screen'], q['sY1'], q['sZ0']), V(q['screen'], q['sY0'], q['sZ0']), V(q['screen'], q['sY0'], q['sZ1']), V(q['screen'], q['sY1'], q['sZ1'])]
    me.from_pydata([tuple(v) for v in vs], [], [(0, 1, 2, 3)])
    me.update()
    if me.polygons[0].normal.x > 0:
        me.polygons[0].flip()
        me.update()
    uv = me.uv_layers.new(name='UVMap')
    for lp in me.loops:
        co = me.vertices[lp.vertex_index].co
        uv.data[lp.index].uv = ((co.y - q['sY1']) / (q['sY0'] - q['sY1']), (co.z - q['sZ0']) / (q['sZ1'] - q['sZ0']))
    o = bpy.data.objects.new('SM_ECX_Marquee_Screen', me)
    core.link(o)
    m = core.Mat('ecx_screen')
    m.set('Base Color', hx(0x050608))
    m.set('Emission Color', hx(0x101826))
    m.set('Emission Strength', 1.0)
    m.set('Roughness', 0.3)
    m.m.use_backface_culling = True
    core.assign(o, m)
    print('[ecx_building] SM_ECX_Marquee_Screen: 2 triangles (screen)')
    return o


def marquee_textures():
    """T_ECX_MarqueeBg (the base slide: night over the ranges, the lockup in the left third, frame and ticker band; the
    middle and right left for the game's text) and T_ECX_MarqueeSeries (the series lockup slide), 2048 x 768."""
    os.makedirs(TEX_DIR, exist_ok=True)
    w, h = 2048, 768
    old = brand.OUT
    brand.OUT = TEX_DIR
    try:
        img = brand.night_ground(w, h, brand.MIDNIGHT, 0x16223d, ember=0.45, ranges=((11, 0.12, 0.12, 0x101a30, 0.5), (5, 0.05, 0.08, brand.MIDNIGHT, 0.0)))
        s = Sheet(266.667, 100.0)
        brand.lockup(s, 44.0, 52.0, 66.0, sub='POKER SERIES')
        s.rect(88.0, 14.0, 0.35, 74.0, brand.STEEL)
        s.rect(4.0, 11.0, 258.667, 0.35, brand.EMBER_HOT)
        for x, y, ww, hh in ((2, 2, 262.667, 0.4), (2, 97.6, 262.667, 0.4), (2, 2, 0.4, 96), (264.267, 2, 0.4, 96)):
            s.rect(x, y, ww, hh, brand.STEEL)
        for x, y in ((2, 2), (2, 97.6), (254.667, 2), (254.667, 97.6)):
            s.rect(x, y, 10, 0.4, brand.EMBER_HOT)
        fg = brand.sheet_rgba('ecx_marquee_bg', s, w)
        for obj, *_ in s.items:
            bpy.data.objects.remove(obj)
        brand.save_png('T_ECX_MarqueeBg', brand.over(img, fg))
        img = brand.night_ground(w, h, brand.MIDNIGHT, 0x1a2644, ember=0.85, ranges=((21, 0.09, 0.11, 0x17223c, 0.7), (8, 0.05, 0.06, 0x0f1729, 0.4),
                                                                                     (2, 0.03, 0.04, brand.MIDNIGHT, 0.0)))
        s = Sheet(266.667, 100.0)
        brand.mark(s, 64.0, 54.0, 62.0)
        brand.wordmark(s, 102.0, 52.0, 18.0, align='LEFT', sub='POKER SERIES')
        s.text('PLAY  ·  COMPETE  ·  RISE', 104.0, 22.0, 6.0, brand.GOLD, face='Bold', align='LEFT', tracking=1.6)
        s.rect(104.0, 31.0, 140.0, 0.4, brand.EMBER_HOT)
        fg = brand.sheet_rgba('ecx_marquee_series', s, w)
        for obj, *_ in s.items:
            bpy.data.objects.remove(obj)
        brand.save_png('T_ECX_MarqueeSeries', brand.over(img, fg))
    finally:
        brand.OUT = old


# ------------------------------------------------------------------ build, bake, review

def glow_mesh(mb, name):
    if not mb.faces:
        mb.poly([V(-0.2, 0.0, -0.05), V(-0.2, 0.02, -0.05), V(-0.22, 0.02, -0.05)], want=(0, 0, -1))
    o = mb.obj(name + '_parts')
    g = parts.glow_object([o], name)
    for s in g.material_slots:
        s.material.use_backface_culling = True
    tris = sum(len(p.vertices) - 2 for p in g.data.polygons)
    print(f'[ecx_building] {name}: {tris} triangles (glow)')
    return g


def glass_mesh():
    o = GLASS.obj('SM_ECX_Podium_Glass')
    core.assign(o, glass_plain('ecx_glass'))
    tris = sum(len(p.vertices) - 2 for p in o.data.polygons)
    print(f'[ecx_building] SM_ECX_Podium_Glass: {tris} triangles (glass)')
    return o


BUILDERS = [('SM_ECX_Entry', lambda: entry()), ('SM_ECX_Lobby', lambda: lobby()), ('SM_ECX_WingS', lambda: wing_s()), ('SM_ECX_WingN', lambda: wing_n()),
            ('SM_ECX_Canopy', lambda: canopy()), ('SM_ECX_RevolvingDoor', lambda: revolving_door()),
            ('SM_ECX_RevolvingDoor_Wings', lambda: revolving_wings()), ('SM_ECX_SwingDoor', lambda: swing_door()), ('SM_ECX_Tower', lambda: tower()),
            ('SM_ECX_Crown', lambda: crown()), ('SM_ECX_CrownCrest', lambda: crown_crest()), ('SM_ECX_Marquee', lambda: marquee()),
            ('SM_ECX_Marquee_Screen', lambda: marquee_screen())]
GLOWS = {'SM_ECX_Podium_GlowEmber': 'ember', 'SM_ECX_Podium_GlowWarm': 'warm', 'SM_ECX_Podium_GlowAsh': 'ash', 'SM_ECX_Lobby_GlowWarm': 'lobby',
         'SM_ECX_Tower_GlowEmber': 'tower', 'SM_ECX_Crown_GlowEmber': 'crown'}


def build():
    core.reset()
    reset_collectors()
    RNG.seed(2100)
    if 'SM_ECX_Marquee_Screen' in MESHES:
        marquee_textures()
    made = {}
    for name, fn in BUILDERS:
        if name in MESHES:
            made[name] = fn()
    if 'SM_ECX_Entry' in MESHES:
        entry_glass()
    for name, key in GLOWS.items():
        if name in MESHES:
            made[name] = glow_mesh(GLOW[key], name)
    if 'SM_ECX_Podium_Glass' in MESHES:
        made['SM_ECX_Podium_Glass'] = glass_mesh()
    global _BUILT
    _BUILT = [made[n] for n in MESHES if n in made]
    return _BUILT


def bake_parts(objs):
    # Glass, the screen and the glow meshes are shaded by the game: exported unbaked.
    return [o for o in objs if o.name not in UNBAKED and '_Glow' not in o.name]


def _flat_stem(img):
    return os.path.splitext(os.path.basename(img.filepath or img.name))[0]


def assemble(objs):
    """After the bake: a texture that came out one flat value (a slot's normal no bump touches, a lit slot's dark base)
    is exported as a 4x4 of that value instead of a 1-4k image of it (as facades.py does). Only normals, and the base
    color and ORM of lit slots, qualify: a dark stone's color varies by less than the test's tolerance in linear values."""
    for o in objs:
        for slot in o.material_slots:
            if slot.material is None or not slot.material.use_nodes:
                continue
            nodes = [n for n in slot.material.node_tree.nodes if n.type == 'TEX_IMAGE' and n.image is not None]
            lit = any(_flat_stem(n.image).endswith('_Emissive') for n in nodes)
            for n in nodes:
                img = n.image
                key = _flat_stem(img)
                if key.endswith('_Emissive') or not key.startswith('T_SM_ECX'):
                    continue
                if not (key.endswith('_Normal') or lit):
                    continue
                w, h = img.size
                if w * h <= 64:
                    continue
                px = np.empty(w * h * 4, dtype=np.float32)
                img.pixels.foreach_get(px)
                px = px.reshape(-1, 4)[:, :3]
                cov = px[px.sum(axis=1) > 1e-4]
                if len(cov) == 0:
                    continue
                med = np.median(cov, axis=0)
                if np.mean(np.abs(cov - med).max(axis=1) < 4.0 / 255.0) > 0.995:
                    img.scale(4, 4)
                    img.pixels.foreach_set(np.tile(np.append(cov.mean(axis=0), 1.0).astype(np.float32), 16))
                    img.save()
                    print(f'[ecx_building] {key}: flat, exported as 4x4')
    # build.py exports what assemble returns: every mesh, not just the baked ones.
    return _BUILT if _BUILT else objs


_BUILT = []


GLOW_LOOK = {'GlowEmber': (EMBER_LED, 12.0), 'GlowWarm': (WARM, 8.0), 'GlowAsh': (ASH, 10.0)}


def pose_for_review(objs):
    """Props where the game puts them (the drum 20 cm inside the glass, the four leaves at their hinges); glow meshes in
    their game colors; the marquee showing its series slide. The views: the whole front from the plaza, the crane, the
    entrance."""
    global REVIEW_VIEWS
    by = {o.name: o for o in objs}
    for n in ('SM_ECX_RevolvingDoor', 'SM_ECX_RevolvingDoor_Wings'):
        if n in by:
            by[n].location = (0.2, 0.0, 0.0)
    if 'SM_ECX_SwingDoor' in by:
        d = by['SM_ECX_SwingDoor']
        d.location = (0.0, -4.6, 0.0)
        for loc, sy in (((0.0, 2.5, 0.0), 1.0), ((0.0, -2.5, 0.0), -1.0), ((0.0, 4.6, 0.0), -1.0)):
            c = d.copy()
            core.link(c)
            c.location = loc
            c.scale.y = sy
    for o in objs:
        for key, (h, s) in GLOW_LOOK.items():
            if key in o.name:
                for slot in o.material_slots:
                    b = next(n for n in slot.material.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
                    b.inputs['Emission Color'].default_value = hx(h)
                    b.inputs['Base Color'].default_value = hx(h)
                    b.inputs['Emission Strength'].default_value = s * (2.0 if 'Crown' in o.name else 1.0)
    if 'SM_ECX_Marquee_Screen' in by:
        p = os.path.join(TEX_DIR, 'T_ECX_MarqueeSeries.png')
        if os.path.exists(p):
            m = core.Mat('review_screen')
            img = m.image(p)
            m.link(img.outputs['Color'], m.bsdf.inputs['Emission Color'])
            m.set('Emission Strength', 3.0)
            m.set('Base Color', hx(0x000000))
            core.assign(by['SM_ECX_Marquee_Screen'], m)
    REVIEW_VIEWS = [('front', -90, 7, 1.75, (0.0, -1.0, 22.0)), ('crane', -128, 15, 1.9, (10.0, 0.0, 20.0)), ('entry', -104, 4, 0.24, (-9.0, 0.0, 6.0))]
