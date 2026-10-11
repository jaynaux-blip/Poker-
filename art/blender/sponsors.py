"""The sponsors' brands in the art (next_segment_spec 3.4 and 3.10): their colours, their marks and their patches.

    blender -b -P art/blender/sponsors.py        (the patch atlas alone; assets/wear.py makes it in its own build)

The marks are the game's own (sponsorart::Mark in Plugins/ShortStackCore/Source/ShortStackCore/Private/ShortStack/UI/
SponsorArt.cpp), ported shape for shape, so a patch, a can and the laptop show the same logo. The game's radial gradients
are flat thread or ink colours here: embroidery has none. Fictional brands only.

The patch atlas (unreal/Art/Patches/T_PatchAtlas_*.png, 2048 x 2048): every brand's patch face, embroidered. The design is
drawn flat (the layout of sponsorart::Patch), split into its thread colours, and each colour is stitched: satin across the
narrow shapes (letters, rings, rims, the edge), staggered tatami rows in the wide ones, all of it raised off a twill
ground. Three maps: BaseColor (sRGB), Normal (tangent space, OpenGL: +G is up the image) and ORT (R occlusion,
G roughness, B thread: 1 embroidered, 0 twill). Cells, by brand index (sponsor::Brand::Mark, the catalog's order):
  wide  (the chest and cap patches): 512 x 344 px, 4 a row from the top;
  round (the sleeve patch):          292 x 292 px, 7 a row from y = 1376.
Each cell's top-left 12 x 12 px is the brand's thread colour (the merrowed edge's), outside the patch's outline.
patch_atlas.json beside the maps records all of this for the engine (segment_art_setup.py builds M_Patch from it).
"""
import json
import math
import os
import struct
import sys
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

import bpy  # noqa: E402
import numpy as np  # noqa: E402

import brand as ec  # noqa: E402
from artkit import core  # noqa: E402
from artkit.sheet import Sheet  # noqa: E402

OUT = os.path.join(core.ROOT, 'unreal', 'Art', 'Patches')

# ------------------------------------------------------------------ the catalog


class Brand:
    def __init__(self, index, id, short, under, color, ink, ground, thread):
        self.index = index
        self.id = id
        self.short = short
        self.under = under
        self.color = color      # sponsor::Brand::Color
        self.ink = ink          # sponsor::Brand::Ink
        self.ground = ground    # the patch's twill
        self.thread = thread    # its merrowed edge and its words


# Catalog order (sponsor::Brands(), Sponsor.cpp), which is also each brand's Mark index and atlas cell. The patch's ground
# and thread follow sponsorart::Patch, except that the thread is always the one of the brand's two colours that reads on
# the ground: the game's rule gives IronHouse dark grey and Volt Rush near-black words on a near-black patch.
BRANDS = [
    Brand(0, 'luckypenny', 'Lucky Penny', 'NORTHSIDE', 0x1f5c3a, 0xc8743a, 0x1f5c3a, 0xc8743a),
    Brand(1, 'barber', 'Fifth St. Barbers', 'NORTHSIDE', 0x1d3557, 0xe63946, 0xf4f1ea, 0x1d3557),
    Brand(2, 'ironhouse', 'IronHouse', 'NORTHSIDE', 0x2a2a2e, 0xef4444, 0x16181d, 0xef4444),
    Brand(3, 'voltrush', 'Volt Rush', 'ENERGY DRINK', 0x16181d, 0xb6ff2e, 0x16181d, 0xb6ff2e),
    Brand(4, 'overclock', 'Overclock', 'ENERGY DRINK', 0x84cc16, 0x0b0716, 0x16181d, 0x84cc16),
    Brand(5, 'embercrest', 'Team Embercrest', 'POKER ROOM', 0x1b1512, 0xe8742c, 0x1b1512, 0xe8742c),
    Brand(6, 'halo', 'Halo', 'GEAR', 0xa3e635, 0x0f1a05, 0x16181d, 0xa3e635),
    Brand(7, 'tunnelrat', 'TunnelRat', 'PRIVACY', 0x38bdf8, 0x04121c, 0x16181d, 0x38bdf8),
    Brand(8, 'stacked', 'Stacked', 'APPAREL', 0xf59e0b, 0x1a1204, 0xf59e0b, 0x1a1204),
    Brand(9, 'rangewise', 'Rangewise', 'TRAINING', 0x6366f1, 0xeef2ff, 0x16181d, 0x6366f1),
    Brand(10, 'riverline', 'Team RiverLine', 'POKER SITE', 0x27d3c3, 0x04201d, 0x27d3c3, 0x04201d),
    Brand(11, 'sitwell', 'Sitwell', 'GEAR', 0xfb923c, 0x1f0d02, 0x16181d, 0xfb923c),
    Brand(12, 'turnstone', 'Turnstone', 'POKER SITE', 0x334155, 0xd4a843, 0x334155, 0xd4a843),
]
BY_ID = {b.id: b for b in BRANDS}
# The P0 brands (spec 3.11): the ones the closet sews first.
P0 = ('luckypenny', 'barber', 'ironhouse', 'voltrush', 'overclock', 'embercrest')

# ------------------------------------------------------------------ the atlas layout

ATLAS = 2048
WIDE_PX = (512, 344)
WIDE_COLS = 4
ROUND_PX = 292
ROUND_COLS = 7
ROUND_Y0 = 4 * WIDE_PX[1]
SWATCH = 12
# The patches' faces, in millimetres: each the same shape as its cell.
WIDE_MM = (76.0, 76.0 * WIDE_PX[1] / WIDE_PX[0])      # 76 x 51.06: the chest patch (SM_Wear_Patch)
CAP_MM = (56.0, 56.0 * WIDE_PX[1] / WIDE_PX[0])       # 56 x 37.6: the cap's (SM_Wear_PatchCap), the chest's art
ROUND_MM = 64.0                                       # the sleeve's disc (SM_Wear_PatchRound)
CORNER_MM = 7.0                                       # the rectangles' corner radius


def wide_cell(i):
    """(x, y, w, h) of brand i's wide cell, in atlas pixels from the top left."""
    return ((i % WIDE_COLS) * WIDE_PX[0], (i // WIDE_COLS) * WIDE_PX[1], WIDE_PX[0], WIDE_PX[1])


def round_cell(i):
    return ((i % ROUND_COLS) * ROUND_PX, ROUND_Y0 + (i // ROUND_COLS) * ROUND_PX, ROUND_PX, ROUND_PX)


# ------------------------------------------------------------------ colour helpers


def mix(c0, c1, f):
    return ec.mix(c0, c1, f)


def lighter(c, t):
    return mix(c, 0xffffff, t)


def darker(c, t):
    return mix(c, 0x000000, t)


def srgb01(c):
    return np.array([((c >> 16) & 255) / 255.0, ((c >> 8) & 255) / 255.0, (c & 255) / 255.0], np.float32)


def to_linear(a):
    a = np.asarray(a, np.float32)
    return np.where(a <= 0.04045, a / 12.92, ((a + 0.055) / 1.055) ** 2.4)


def to_srgb(a):
    a = np.clip(np.asarray(a, np.float32), 0.0, 1.0)
    return np.where(a <= 0.0031308, a * 12.92, 1.055 * np.power(np.maximum(a, 0.0031308), 1 / 2.4) - 0.055)


# ------------------------------------------------------------------ drawing in the game's coordinates


def clip_convex(subject, clip):
    """Sutherland-Hodgman: the part of polygon subject inside the convex polygon clip (both lists of 2D points)."""
    def inside(p, a, b):
        return (b[0] - a[0]) * (p[1] - a[1]) - (b[1] - a[1]) * (p[0] - a[0]) >= 0.0

    def cross(p, q, a, b):
        dx, dy = q[0] - p[0], q[1] - p[1]
        ex, ey = b[0] - a[0], b[1] - a[1]
        den = dx * ey - dy * ex
        if abs(den) < 1e-12:
            return q
        t = ((a[0] - p[0]) * ey - (a[1] - p[1]) * ex) / den
        return (p[0] + t * dx, p[1] + t * dy)
    # Points that (nearly) repeat make edges with no direction, whose half-plane is noise: drop them first.
    size = max(max(abs(x), abs(y)) for x, y in clip) or 1.0
    pts = []
    for q in clip:
        if not pts or math.hypot(q[0] - pts[-1][0], q[1] - pts[-1][1]) > size * 1e-7:
            pts.append(q)
    while len(pts) > 1 and math.hypot(pts[0][0] - pts[-1][0], pts[0][1] - pts[-1][1]) <= size * 1e-7:
        pts.pop()
    clip = pts
    # The clip polygon's winding decides which side is in.
    area = sum(clip[i][0] * clip[(i + 1) % len(clip)][1] - clip[(i + 1) % len(clip)][0] * clip[i][1] for i in range(len(clip)))
    if area < 0:
        clip = list(reversed(clip))
    out = list(subject)
    for i in range(len(clip)):
        a, b = clip[i], clip[(i + 1) % len(clip)]
        src, out = out, []
        if not src:
            break
        for j in range(len(src)):
            p, q = src[j - 1], src[j]
            if inside(q, a, b):
                if not inside(p, a, b):
                    out.append(cross(p, q, a, b))
                out.append(q)
            elif inside(p, a, b):
                out.append(cross(p, q, a, b))
    return out


def rrect_pts(x, y, w, h, r, steps=10):
    """A rounded rectangle's outline, (x, y) its top-left corner in canvas coordinates."""
    r = max(0.0, min(r, w / 2, h / 2))
    return core.rounded_rect(x, y, x + w, y + h, r, steps=steps)


class Pen:
    """Draws into a Sheet in the game's canvas coordinates (y down, as ui::Canvas): (x, y) lands at (x0 + x k, y0 - y k).
    Every colour used is kept, in order: the embroidery stitches each one as a thread."""

    def __init__(self, s, x0, y0, k, metal=0.0, rough=0.4):
        self.s = s
        self.x0 = x0
        self.y0 = y0
        self.k = k
        self.colors = []
        self.metal = metal  # the print's finish (a can's inks), for the sheet's surface map
        self.rough = rough

    def use(self, color):
        if color not in self.colors:
            self.colors.append(color)
        return color

    def P(self, x, y):
        return (self.x0 + x * self.k, self.y0 - y * self.k)

    def poly(self, pts, color):
        if len(pts) >= 3:
            self.s.poly([self.P(x, y) for x, y in pts], self.use(color), metal=self.metal, rough=self.rough)

    def rect(self, x, y, w, h, color):
        self.poly([(x, y), (x + w, y), (x + w, y + h), (x, y + h)], color)

    def rrect(self, x, y, w, h, r, color):
        self.poly(rrect_pts(x, y, w, h, r), color)

    def circle(self, x, y, r, color, n=96):
        self.poly(core.circle_points(x, y, r, n), color)

    def ring(self, x, y, r, width, color, a0=0.0, a1=2 * math.pi, n=128, caps=False):
        """A stroked circle (or arc, a0 to a1) of radius r, width wide."""
        r0, r1 = r - width / 2, r + width / 2
        m = max(8, int(n * abs(a1 - a0) / (2 * math.pi)))
        outer = [(x + r1 * math.cos(a0 + (a1 - a0) * i / m), y + r1 * math.sin(a0 + (a1 - a0) * i / m)) for i in range(m + 1)]
        inner = [(x + r0 * math.cos(a1 - (a1 - a0) * i / m), y + r0 * math.sin(a1 - (a1 - a0) * i / m)) for i in range(m + 1)]
        self.poly(outer + inner, color)
        if caps:
            for a in (a0, a1):
                self.circle(x + r * math.cos(a), y + r * math.sin(a), width / 2, color, 32)

    def stroke(self, pts, width, color, round_ends=True):
        """A polyline width wide, its joints and ends rounded."""
        hw = width / 2
        for (ax, ay), (bx, by) in zip(pts, pts[1:]):
            dx, dy = bx - ax, by - ay
            n = math.hypot(dx, dy) or 1.0
            ox, oy = -dy / n * hw, dx / n * hw
            self.poly([(ax + ox, ay + oy), (bx + ox, by + oy), (bx - ox, by - oy), (ax - ox, ay - oy)], color)
        if round_ends:
            for x, y in pts:
                self.circle(x, y, hw, color, 32)

    def stroke_rrect(self, x, y, w, h, r, width, color):
        """A rounded rectangle's outline, width wide, centred on its edge."""
        hw = width / 2
        outer = rrect_pts(x - hw, y - hw, w + width, h + width, r + hw)
        inner = rrect_pts(x + hw, y + hw, w - width, h - width, max(r - hw, 0.0))
        self.poly(outer + [outer[0]] + list(reversed(inner + [inner[0]])), color)

    def text(self, words, x, y, size, color, face='Black', align='LEFT', tracking=1.0):
        """Words with their baseline at canvas y."""
        X, Y = self.P(x, y)
        self.s.text(words, X, Y, size * self.k, self.use(color), face=face, align=align, tracking=tracking, metal=self.metal, rough=self.rough)


def text_width(s, words, size, face='Black', tracking=1.0):
    """The width of words set at size (sheet units)."""
    cu = bpy.data.curves.new('measure', 'FONT')
    cu.body = words
    cu.font = s._font(face)
    cu.size = size
    cu.space_character = tracking
    obj = bpy.data.objects.new('measure', cu)
    core.link(obj)
    bpy.context.view_layer.update()
    w = obj.dimensions.x
    bpy.data.objects.remove(obj)
    bpy.data.curves.remove(cu)
    return w


# ------------------------------------------------------------------ the marks (sponsorart::Mark)


class Box:
    """The square a mark is drawn in: centre (cx, cy) and side S, in canvas coordinates."""

    def __init__(self, cx, cy, S):
        self.cx, self.cy, self.S = cx, cy, S

    def at(self, u, v):
        return (self.cx + (u - 0.5) * self.S, self.cy + (v - 0.5) * self.S)

    def L(self, u):
        return u * self.S


def _square(p, B, r, color):
    p.rrect(B.cx - B.S * 0.5, B.cy - B.S * 0.5, B.S, B.S, B.S * r, color)


def penny(p, B, field, copper):
    # A copper coin with a raised rim and a four-leaf clover struck in it, on the store's green. The game's radial shine
    # is a lighter crescent here.
    _square(p, B, 0.22, field)
    ox, oy = B.at(0.5, 0.5)
    p.circle(ox, oy, B.L(0.36), darker(copper, 0.12))
    p.circle(ox + B.L(0.012), oy + B.L(0.012), B.L(0.335), copper)
    p.circle(ox - B.L(0.07), oy - B.L(0.08), B.L(0.17), lighter(copper, 0.28))
    p.ring(ox, oy, B.L(0.3), B.L(0.035), darker(copper, 0.35))
    clover = darker(copper, 0.45)
    for k in range(4):
        a = math.pi * 0.25 + k * math.pi * 0.5
        p.circle(ox + math.cos(a) * B.L(0.075), oy + math.sin(a) * B.L(0.075), B.L(0.075), clover)
    p.stroke([(ox, oy), (ox + B.L(0.05), oy + B.L(0.19))], B.L(0.03), clover)


def barber_pole(p, B, navy, red):
    # The pole: a white capsule with red and navy stripes running round it, capped top and bottom.
    x, y, w, h = B.cx - B.L(0.17), B.cy - B.L(0.36), B.L(0.34), B.L(0.72)
    capsule = rrect_pts(x, y, w, h, B.L(0.17), steps=12)
    p.poly(capsule, 0xf4f1ea)
    for k in range(-4, 6):
        sy = y + k * B.L(0.16)
        band = [(x - B.L(0.05), sy + B.L(0.12)), (x + w + B.L(0.05), sy), (x + w + B.L(0.05), sy + B.L(0.06)), (x - B.L(0.05), sy + B.L(0.18))]
        p.poly(clip_convex(band, capsule), red if k % 2 == 0 else navy)
    p.stroke_rrect(x, y, w, h, B.L(0.17), B.L(0.025), navy)
    p.rrect(B.cx - B.L(0.22), y - B.L(0.06), B.L(0.44), B.L(0.07), B.L(0.03), navy)
    p.rrect(B.cx - B.L(0.22), y + h - B.L(0.01), B.L(0.44), B.L(0.07), B.L(0.03), navy)


def dumbbell(p, B, ground, red):
    # Iron on a black plate: a dumbbell, its two plates a side in the gym's red.
    _square(p, B, 0.16, ground)
    p.rrect(B.cx - B.L(0.3), B.cy - B.L(0.035), B.L(0.6), B.L(0.07), B.L(0.03), 0xc9ccd1)
    for side in (-1.0, 1.0):
        p.rrect(B.cx + side * B.L(0.22) - B.L(0.06), B.cy - B.L(0.22), B.L(0.12), B.L(0.44), B.L(0.04), red)
        p.rrect(B.cx + side * B.L(0.33) - B.L(0.045), B.cy - B.L(0.15), B.L(0.09), B.L(0.3), B.L(0.03), darker(red, 0.25))


def bolt(p, B, ground, lime):
    # A can-black disc and a lime bolt through it.
    ox, oy = B.at(0.5, 0.5)
    p.circle(ox, oy, B.L(0.47), ground)
    p.ring(ox, oy, B.L(0.43), B.L(0.04), lime)
    p.poly([B.at(0.56, 0.12), B.at(0.3, 0.55), B.at(0.48, 0.55), B.at(0.42, 0.88), B.at(0.72, 0.42), B.at(0.54, 0.42)], lime)


def gauge(p, B, lime, ink):
    # A tachometer pinned past the red: Overclock.
    ox, oy = B.at(0.5, 0.58)
    p.circle(B.cx, B.cy, B.L(0.48), lime)
    p.ring(ox, oy, B.L(0.3), B.L(0.07), ink, math.pi * 0.85, math.pi * 2.15, caps=True)
    a = math.pi * 1.85
    p.stroke([(ox, oy), (ox + math.cos(a) * B.L(0.3), oy + math.sin(a) * B.L(0.3))], B.L(0.06), ink)
    p.circle(ox, oy, B.L(0.07), ink)


def embercrest(p, cx, cy, size, ground):
    """rlnet_detail::NetEmbercrest (brand.py's mark) in flat colours: the ember in four rings, deep at the rim and hot
    low in the disc, where the light breaks over the ridge; the faceted crest in front of it."""
    s = p.s
    X, Y = p.P(cx, cy)
    u = size * p.k / 100.0
    base = Y - 28.0 * u

    def P(x, y):
        return (X + x * u, base + y * u)
    for r, drop, color in ((34.0, 0.0, ec.EMBER_DEEP), (27.0, 3.0, ec.EMBER), (19.0, 6.0, ec.EMBER_HOT), (10.0, 9.0, ec.GOLD)):
        s.circle(X, base + (42.0 - drop) * u, r * u, p.use(color), n=128)
    s.poly([P(-51, -1.5), P(-26, 37.5), P(-15, 25.5), P(0, 64), P(13, 32.5), P(25, 43.5), P(51, -1.5)], p.use(ground))
    Lp, V1, C, V2, Rp, Rb = P(-26, 34), P(-15, 22), P(0, 60), P(13, 29), P(25, 40), P(48, 0)
    s.poly([P(-48, 0), Lp, V1, C, V2, Rp, Rb], p.use(ec.ASH))
    s.poly([Lp, V1, P(-13, 0), P(-21, 0)], p.use(ec.ICE))
    s.poly([C, V2, P(15, 0), P(3, 0)], p.use(ec.ICE))
    s.poly([Rp, Rb, P(31, 0)], p.use(ec.ICE))
    s.poly([C, P(-8, 43), P(-4.5, 46.5), P(-1.5, 42), P(1.6, 49)], p.use(0xffffff))
    s.rect(X - 48 * u, base - 6.5 * u, 45 * u, 2.4 * u, p.use(ec.EMBER_HOT))
    s.rect(X + 3 * u, base - 6.5 * u, 45 * u, 2.4 * u, p.use(ec.EMBER_HOT))


def crest(p, B, ground):
    _square(p, B, 0.18, ground)
    embercrest(p, B.cx, B.cy - B.L(0.05), B.L(0.82), ground)


def halo_ring(p, B, lime, ink):
    # A halo of light around a lens.
    p.circle(B.cx, B.cy, B.L(0.48), ink)
    p.ring(B.cx, B.cy, B.L(0.36), B.L(0.09), lime)
    p.circle(B.cx, B.cy, B.L(0.16), 0x1c2733)
    p.circle(B.cx - B.L(0.05), B.cy - B.L(0.05), B.L(0.05), lighter(lime, 0.4))


def shield(p, B, blue, ink):
    # A shield with a tunnel's mouth in it: TunnelRat.
    p.poly([B.at(0.5, 0.04), B.at(0.9, 0.18), B.at(0.86, 0.58), B.at(0.5, 0.96), B.at(0.14, 0.58), B.at(0.1, 0.18)], blue)
    ox, oy = B.at(0.5, 0.6)
    p.rect(ox - B.L(0.2), oy, B.L(0.4), B.L(0.14), ink)
    p.circle(ox, oy, B.L(0.2), ink)
    p.circle(ox, oy + B.L(0.02), B.L(0.08), lighter(blue, 0.5))


def stack(p, B, amber, ink):
    # Three bars stacked off-true, like a hoodie folded on a shelf: Stacked.
    _square(p, B, 0.2, ink)
    for k in range(3):
        y = B.cy + B.L(0.16) - k * B.L(0.17)
        x = B.cx - B.L(0.3) + (k % 2) * B.L(0.05)
        p.rrect(x, y - B.L(0.06), B.L(0.55), B.L(0.12), B.L(0.05), amber if k == 2 else darker(amber, 0.2 + 0.15 * k))


def range_chart(p, B, indigo, ink):
    # A corner of a range chart, the playable hands lit: Rangewise.
    _square(p, B, 0.14, indigo)
    cell = B.L(0.16)
    for r in range(4):
        for k in range(4):
            lit = k + r < 5 and not (k == 3 and r == 1)
            p.rrect(B.cx - B.L(0.34) + k * cell * 1.12, B.cy - B.L(0.34) + r * cell * 1.12, cell, cell, cell * 0.2, ink if lit else mix(indigo, ink, 0.18))


def waves(p, B, teal, ink):
    # Two river lines: RiverLine.
    _square(p, B, 0.24, ink)
    for k in range(2):
        line = []
        for i in range(17):
            u = i / 16.0
            line.append((B.cx - B.L(0.32) + u * B.L(0.64), B.cy - B.L(0.08) + k * B.L(0.18) + math.sin(u * math.pi * 2.0) * B.L(0.07)))
        p.stroke(line, B.L(0.08), teal if k == 0 else lighter(teal, 0.4))


def chair(p, B, orange, ink):
    # A high-backed chair in profile: Sitwell.
    p.circle(B.cx, B.cy, B.L(0.48), orange)
    p.rrect(B.cx - B.L(0.16), B.cy - B.L(0.32), B.L(0.12), B.L(0.42), B.L(0.05), ink)
    p.rrect(B.cx - B.L(0.16), B.cy + B.L(0.02), B.L(0.36), B.L(0.1), B.L(0.04), ink)
    p.rect(B.cx + B.L(0.01), B.cy + B.L(0.1), B.L(0.05), B.L(0.16), ink)
    p.rrect(B.cx - B.L(0.14), B.cy + B.L(0.24), B.L(0.34), B.L(0.05), B.L(0.025), ink)


def turnstile(p, B, slate, gold):
    # A turnstile's three arms seen end on: Turnstone.
    p.circle(B.cx, B.cy, B.L(0.48), slate)
    p.ring(B.cx, B.cy, B.L(0.42), B.L(0.03), gold)
    for k in range(3):
        a = -math.pi * 0.5 + k * math.pi * 2.0 / 3.0
        p.stroke([(B.cx, B.cy), (B.cx + math.cos(a) * B.L(0.3), B.cy + math.sin(a) * B.L(0.3))], B.L(0.08), gold)
    p.circle(B.cx, B.cy, B.L(0.09), gold)


def mark(p, b, cx, cy, S):
    """Brand b's mark in an S-sided box centred on (cx, cy), canvas coordinates: sponsorart::Mark."""
    B = Box(cx, cy, S)
    if b.index == 5:
        crest(p, B, b.color)
        return
    draw = [penny, barber_pole, dumbbell, bolt, gauge, None, halo_ring, shield, stack, range_chart, waves, chair, turnstile][b.index]
    draw(p, B, b.color, b.ink)


# ------------------------------------------------------------------ the patch layouts (sponsorart::Patch)


def _fit_lines(s, words, max_w, cap_one, cap_two, face='Black', tracking=1.02):
    """The wordmark on one line or, when that would set it small, two: (lines, size), sizes in sheet units."""
    w1 = text_width(s, words, 1.0, face, tracking)
    one = min(cap_one, max_w / max(w1, 1e-6))
    if ' ' not in words or one >= cap_two * 0.92:
        return [words], one
    parts = words.split(' ')
    best = None
    for i in range(1, len(parts)):
        a, b2 = ' '.join(parts[:i]), ' '.join(parts[i:])
        w = max(text_width(s, a, 1.0, face, tracking), text_width(s, b2, 1.0, face, tracking))
        size = min(cap_two, max_w / max(w, 1e-6))
        if best is None or size > best[1]:
            best = ([a, b2], size)
    return (best if best and best[1] > one else ([words], one))


def wide_design(s, b):
    """The chest patch's face (and the cap's): the twill in the brand's ground, a satin edge in thread under the
    merrow, a run stitch inside it, the mark at the left, the word mark and its line at the right. Canvas units are
    the atlas cell's pixels (512 x 344)."""
    W, H = WIDE_PX
    k = WIDE_MM[0] / W
    p = Pen(s, 0.0, H * k, k)
    p.rect(0, 0, W, H, b.ground)
    r = CORNER_MM / k
    p.rrect(0, 0, W, H, r, b.thread)
    p.rrect(13, 13, W - 26, H - 26, r - 13, b.ground)
    p.stroke_rrect(23, 23, W - 46, H - 46, r - 23, 4.2, mix(b.ground, b.thread, 0.62))
    mark(p, b, 143.0, H * 0.5, 190.0)
    # The words: the short name as big as fits (two lines for a long one), the category's line under it.
    x0, max_w = 258.0, 214.0
    lines, size = _fit_lines(s, b.short.upper(), max_w * k, 58.0 * k, 50.0 * k)
    size /= k
    under = min(27.0, max_w / max(text_width(s, b.under, 1.0, 'Bold', 1.22), 1e-6))
    cap, ucap, gap = 0.711 * size, 0.711 * under, 13.0
    lead = cap + 0.3 * size
    block = cap + lead * (len(lines) - 1) + gap + ucap
    y = H * 0.5 - block * 0.5 + cap
    for line in lines:
        p.text(line, x0, y, size, b.thread, face='Black', tracking=1.02)
        y += lead
    p.text(b.under, x0 + 2.0, y - lead + gap + ucap, under, mix(b.ground, b.thread, 0.82), face='Bold', tracking=1.22)
    return p


def arc_widths(s, words, face, tracking):
    """Each character's advance at size 1 (a space as a narrow letter)."""
    return [text_width(s, ch if ch != ' ' else 'n', 1.0, face) * (0.62 if ch == ' ' else 1.0) * tracking for ch in words]


def arc_text(p, words, cx, cy, radius, size, color, face, tracking, top=True, widths=None):
    """Words along a circle (sheet units, centre (cx, cy)): over the top reading clockwise with the letters standing
    outward, or under the bottom reading left to right with the letters standing in toward the centre."""
    s = p.s
    color = p.use(color)
    widths = [w * size for w in (widths or arc_widths(s, words, face, tracking))]
    total = sum(widths)
    sign = -1.0 if top else 1.0
    ang = (math.pi / 2 if top else -math.pi / 2) - sign * (total / 2) / radius
    for ch, w in zip(words, widths):
        a = ang + sign * (w / 2) / radius
        if ch != ' ':
            s.text(ch, cx + radius * math.cos(a), cy + radius * math.sin(a), size, color, face=face, align='CENTER',
                   rot=a - math.pi / 2 if top else a + math.pi / 2)
        ang += sign * w / radius
    return total / radius


# How much bigger each mark draws on the round patch than its 136 px box: a mark that is a disc or a figure on the
# patch's own ground fills the circle further than one on a square plate (whose corners would meet the lettering).
ROUND_MARK_SCALE = (1.42, 1.3, 1.12, 1.3, 1.24, 1.3, 1.24, 1.22, 1.12, 1.12, 1.12, 1.24, 1.24)


def round_design(s, b):
    """The sleeve patch's face: a disc of twill, the satin edge under the merrow and a run stitch, the mark in the
    middle, the name over the top and the category's line under the bottom, a diamond either side."""
    N = ROUND_PX
    k = ROUND_MM / N
    p = Pen(s, 0.0, N * k, k)
    c = N * 0.5
    p.rect(0, 0, N, N, b.ground)
    p.circle(c, c, c, b.thread, 160)
    p.circle(c, c, c - 9.0, b.ground, 160)
    p.ring(c, c, c - 17.0, 2.8, mix(b.ground, b.thread, 0.62), n=160)
    mark(p, b, c, c + 3.0, 136.0 * ROUND_MARK_SCALE[b.index])
    name = b.short.upper()
    w1 = arc_widths(s, name, 'Black', 1.08)
    r_name = 104.0 * k
    size = min(24.0 * k, 2.9 * r_name / max(sum(w1), 1e-6))
    arc_text(p, name, c * k, c * k, r_name, size, b.thread, 'Black', 1.08, True, w1)
    arc_text(p, b.under, c * k, c * k, 119.0 * k, 14.5 * k, mix(b.ground, b.thread, 0.82), 'Bold', 1.3, False)
    for side in (-1.0, 1.0):
        x = c + side * 113.0
        p.poly([(x, c - 5.5), (x + 4.5, c), (x, c + 5.5), (x - 4.5, c)], b.thread)
    return p


# ------------------------------------------------------------------ rendering a design


def _clear(s):
    """Takes a rendered sheet out of the scene (its letters and shapes, the camera, the materials)."""
    for obj, *_ in s.items:
        bpy.data.objects.remove(obj)
    for o in [o for o in bpy.data.objects if o.name.startswith('sheet_cam')]:
        bpy.data.objects.remove(o)
    s.items.clear()
    try:
        bpy.data.orphans_purge(do_local_ids=True, do_linked_ids=False, do_recursive=True)
    except Exception:
        pass


def render_design(draw, b, w_mm, h_mm, px_w, name):
    """A design drawn and rendered flat at px_w: (sRGB pixels, rows from the top; the colours it used, in order)."""
    s = Sheet(w_mm, h_mm)
    p = draw(s, b)
    color, _surface = s.render(name, px_w, samples=16)
    img = bpy.data.images.load(color)
    w, h = img.size
    rgb = np.array(img.pixels[:], dtype=np.float32).reshape(h, w, 4)[::-1, :, :3].copy()
    bpy.data.images.remove(img)
    _clear(s)
    return rgb, list(p.colors)


# ------------------------------------------------------------------ the embroidery

N4 = ((-1, 0), (1, 0), (0, -1), (0, 1))
N8 = N4 + ((-1, -1), (-1, 1), (1, -1), (1, 1))
# Tatami fill angles (degrees), by thread: a digitizer turns each fill so neighbours don't run the same way.
TATAMI_DEG = (35.0, -40.0, 62.0, -18.0, 80.0, 12.0, -65.0, 48.0)
H_EMB = 0.42     # mm the embroidery stands off the twill
H_TWILL = 0.05


def _shift(a, dy, dx, fill):
    """b[y, x] = a[y + dy, x + dx]; fill where that's outside."""
    H, W = a.shape[:2]
    out = np.full_like(a, fill)
    y0, y1 = max(dy, 0), H + min(dy, 0)
    x0, x1 = max(dx, 0), W + min(dx, 0)
    out[y0 - dy:y1 - dy, x0 - dx:x1 - dx] = a[y0:y1, x0:x1]
    return out


def _blur(a, r, passes=2):
    """Separable box blur of radius r pixels (edges held), passes times (two make it a tent)."""
    r = int(r)
    if r < 1:
        return a.astype(np.float32)
    out = a.astype(np.float32)
    for _ in range(passes):
        for axis in (0, 1):
            pad = [(0, 0)] * out.ndim
            pad[axis] = (r + 1, r)
            c = np.cumsum(np.pad(out, pad, mode='edge'), axis=axis, dtype=np.float64)
            n = out.shape[axis]
            out = ((np.take(c, np.arange(2 * r + 1, 2 * r + 1 + n), axis=axis) - np.take(c, np.arange(n), axis=axis)) / (2 * r + 1)).astype(np.float32)
    return out


def _majority(L, K):
    """Antialiasing leaves slivers of a third colour along edges: a pixel with fewer than 3 of its 3 x 3 sharing its
    thread takes the commonest one there."""
    best = L.copy()
    best_n = np.zeros(L.shape, np.int8)
    own = np.zeros(L.shape, np.int8)
    for k in range(K):
        m = (L == k).astype(np.int8)
        n = m.copy()
        for dy, dx in N8:
            n += _shift(m, dy, dx, 0)
        better = n > best_n
        best = np.where(better, k, best)
        best_n = np.maximum(best_n, n)
        own = np.where(L == k, n, own)
    return np.where(own >= 3, L, best).astype(L.dtype)


def _edge_distance(L, cap):
    """Pixels from each pixel to the nearest one of another thread (or the cell's edge), up to cap: erosion,
    alternating 4- and 8-neighbourhoods (an octagon, near enough to round)."""
    same = {o: _shift(L, o[0], o[1], -1) == L for o in N8}
    alive = np.ones(L.shape, bool)
    d = np.zeros(L.shape, np.float32)
    for k in range(cap):
        new = alive.copy()
        for o in (N8 if k % 2 else N4):
            new &= _shift(alive, o[0], o[1], False) & same[o]
        if not new.any():
            break
        d += new
        alive = new
    return d + 0.5, same


def _half_width(d, same, cap):
    """The widest the shape a pixel is in gets nearby (its half width): d's maximum spread through its own thread."""
    w = d.copy()
    for _ in range(cap):
        m = w
        for o in N4:
            m = np.maximum(m, np.where(same[o], _shift(w, o[0], o[1], 0.0), 0.0))
        if np.array_equal(m, w):
            break
        w = m
    return w


def _hash(*ints):
    h = np.zeros(np.broadcast(*ints).shape, np.int64)
    for i, v in enumerate(ints):
        h ^= np.asarray(v, np.int64) * np.int64((73856093, 19349663, 83492791)[i % 3])
    h = (h ^ (h >> 13)) * np.int64(1274126177)
    h = h ^ (h >> 16)
    return (h & 0xFFFF).astype(np.float32) / 65535.0


def _lic(noise, ux, uy, L, steps):
    """Line integral convolution: noise averaged along the stitch direction field (ux, uy), never across into another
    thread. Thread-sized noise comes out as threads."""
    H, W = noise.shape
    yy, xx = np.mgrid[0:H, 0:W]
    acc = noise.copy()
    cnt = np.ones((H, W), np.float32)
    for sgn in (1.0, -1.0):
        px = xx.astype(np.float32)
        py = yy.astype(np.float32)
        vx, vy = ux * sgn, uy * sgn
        alive = np.ones((H, W), bool)
        for _ in range(steps):
            px = px + vx
            py = py + vy
            ix = np.clip(np.rint(px), 0, W - 1).astype(np.int32)
            iy = np.clip(np.rint(py), 0, H - 1).astype(np.int32)
            alive &= (L[iy, ix] == L) & (px >= 0) & (px <= W - 1) & (py >= 0) & (py <= H - 1)
            acc += np.where(alive, noise[iy, ix], 0.0)
            cnt += alive
            nx, ny = ux[iy, ix], uy[iy, ix]
            flip = (nx * vx + ny * vy) < 0.0
            vx = np.where(flip, -nx, nx)
            vy = np.where(flip, -ny, ny)
    return acc / cnt


def _smoothstep(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def embroider(rgb, palette, ppmm, seed=0):
    """A flat design (sRGB, rows from the top, at twice the atlas's resolution) stitched: palette[0] is the twill ground,
    every other colour a thread. Returns (linear colour, normal, ORT) at the atlas's resolution, rows from the top."""
    H, W, _ = rgb.shape
    rng = np.random.default_rng(1009 + seed)
    pal = np.array([srgb01(c) for c in palette], np.float32)
    K = len(palette)
    L = np.argmin(((rgb[:, :, None, :] - pal[None, None, :, :]) ** 2).sum(-1), axis=-1).astype(np.int16)
    L = _majority(L, K)
    thread = L > 0
    cap = int(2.6 * ppmm) + 2
    d, same = _edge_distance(L, cap)
    w = _half_width(d, same, cap)
    sat = thread & (w < 2.0 * ppmm)
    # Satin runs across a shape (along the distance's gradient); the structure tensor keeps it steady over the middle.
    gy, gx = np.gradient(_blur(d, 1))
    r = max(2, int(0.18 * ppmm))
    jxx, jxy, jyy = _blur(gx * gx, r), _blur(gx * gy, r), _blur(gy * gy, r)
    theta = 0.5 * np.arctan2(2.0 * jxy, jxx - jyy)
    ux, uy = np.cos(theta).astype(np.float32), np.sin(theta).astype(np.float32)
    pitch = max(0.40, 4.6 / ppmm)                               # mm between threads
    noise = _blur(rng.standard_normal((H, W)).astype(np.float32), max(1, int(round(pitch * ppmm * 0.35))), 1)
    streak = _lic(noise, ux, uy, L, int(1.5 * ppmm))
    m = float(streak[sat].mean()) if sat.any() else 0.0
    sd = float(streak[sat].std()) if sat.any() else 1.0
    t = 0.5 + 0.5 * np.tanh((streak - m) / max(sd, 1e-6) * 0.9)
    # Tatami: rows of 2.6 mm stitches at the thread's angle, each row's needle points a third along from the last.
    yy, xx = np.mgrid[0:H, 0:W].astype(np.float32)
    th = np.radians(np.array(TATAMI_DEG, np.float32))[L % len(TATAMI_DEG)]
    a = (xx * np.cos(th) + yy * np.sin(th)) / ppmm
    bb = (-xx * np.sin(th) + yy * np.cos(th)) / ppmm
    row = np.floor(bb / pitch)
    fb = bb / pitch - row
    Ls = 2.6
    q = (a + np.mod(row, 3.0) * Ls / 3.0) / Ls
    stitch = np.floor(q)
    fa = q - stitch
    prof = np.sqrt(np.clip(1.0 - (2.0 * fb - 1.0) ** 2, 0.0, 1.0))
    pen = np.exp(-((np.minimum(fa, 1.0 - fa) * Ls) / 0.2) ** 2)
    tat = prof * (1.0 - 0.7 * pen) * (0.88 + 0.24 * _hash(row.astype(np.int64), stitch.astype(np.int64), L.astype(np.int64)))
    # The twill: diagonal wales, the weft's floats across them, and the yarn's own unevenness.
    al = math.radians(63.0)
    tw_pitch = max(0.6, 5.0 / ppmm)
    tw = 0.5 + 0.5 * np.sin(2.0 * math.pi * (xx * math.cos(al) - yy * math.sin(al)) / ppmm / tw_pitch)
    flo = 0.5 + 0.5 * np.sin(2.0 * math.pi * (xx * math.sin(al) + yy * math.cos(al)) / ppmm / (tw_pitch * 1.7))
    tw = tw * (0.8 + 0.2 * flo)
    yarn = _blur(rng.random((H, W)).astype(np.float32), max(1, int(0.15 * ppmm)), 1)
    yarn = (yarn - yarn.mean()) / max(float(yarn.std()), 1e-6) * 0.25 + 0.5
    # Height (mm): satin rounds over each shape like a pulled column; fills are flat-topped with a rolled edge.
    x = np.clip(d / np.maximum(w, 1.0), 0.0, 1.0)
    dome = np.where(sat, np.sqrt(np.clip(x * (2.0 - x), 0.0, 1.0)), _smoothstep(0.0, 0.5 * ppmm, d))
    micro = np.where(sat, t - 0.5, tat - 0.5)
    h = np.where(thread, H_EMB * dome + 0.09 * micro * np.sqrt(dome), H_TWILL * (tw - 0.5) + 0.02 * (yarn - 0.5)).astype(np.float32)
    # Tangent-space normal (OpenGL: +y up the image; rows run down).
    gy, gx = np.gradient(h)
    n = np.dstack([-gx * ppmm, gy * ppmm, np.ones_like(h)])
    # Colour: each thread's own, brighter on its crown, darker in the grooves; the twill's wales just show.
    hb = _blur(h, max(1, int(0.35 * ppmm)))
    cav = np.clip((hb - h) / H_EMB * 2.2, 0.0, 1.0)
    ao = 1.0 - 0.5 * cav
    shade = np.where(thread, np.where(sat, 0.78 + 0.36 * t, 0.80 + 0.30 * tat), 0.84 + 0.16 * tw + 0.08 * (yarn - 0.5))
    col = to_linear(pal)[L] * (shade * (0.55 + 0.45 * ao))[..., None]
    rough = np.where(thread, 0.36 + 0.14 * (1.0 - np.where(sat, t, tat)), 0.80 + 0.08 * (yarn - 0.5))
    ort = np.dstack([ao, rough, thread.astype(np.float32)])

    def half(img):
        return img.reshape(H // 2, 2, W // 2, 2, img.shape[2]).mean(axis=(1, 3))
    n = half(n)
    n /= np.linalg.norm(n, axis=2, keepdims=True)
    return half(col).astype(np.float32), n.astype(np.float32), half(ort).astype(np.float32)


# ------------------------------------------------------------------ the atlas


def write_png(path, rgb):
    """An 8-bit RGB PNG from floats in 0..1, rows from the top, written exactly as given."""
    h, w, _ = rgb.shape
    data = (np.clip(rgb, 0.0, 1.0) * 255.0 + 0.5).astype(np.uint8)
    raw = b''.join(b'\x00' + data[y].tobytes() for y in range(h))

    def chunk(tag, body):
        return struct.pack('>I', len(body)) + tag + body + struct.pack('>I', zlib.crc32(tag + body) & 0xffffffff)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw, 6))
                + chunk(b'IEND', b''))
    return path


def write_jpg(path, rgb):
    """A JPEG of sRGB floats (rows from the top), for review."""
    h, w, _ = rgb.shape
    img = bpy.data.images.new(os.path.basename(path), w, h, alpha=False)
    px = np.ones((h, w, 4), np.float32)
    px[:, :, :3] = np.clip(rgb[::-1], 0.0, 1.0)
    img.pixels.foreach_set(px.ravel())
    img.filepath_raw = path
    img.file_format = 'JPEG'
    img.save(quality=90)
    bpy.data.images.remove(img)
    return path


def lit(color_lin, normal, ort, light=(-0.45, 0.55, 0.70)):
    """A quick shaded look at a patch (sRGB), lit from the top left: for the review sheet only."""
    lv = np.array(light, np.float32)
    lv /= np.linalg.norm(lv)
    ndl = np.clip((normal * lv).sum(-1), 0.0, 1.0)
    hv = lv + np.array([0.0, 0.0, 1.0], np.float32)
    hv /= np.linalg.norm(hv)
    rough = np.maximum(ort[..., 1], 0.05)
    spec = np.clip((normal * hv).sum(-1), 0.0, 1.0) ** (2.0 / rough ** 4) * (1.0 - rough) * 0.5
    return to_srgb(color_lin * (0.18 + 0.95 * ndl * ort[..., 0])[..., None] + spec[..., None])


def build_atlas(out_dir=OUT, review=True):
    """Every brand's two patch faces, embroidered, into the atlas: its three maps and patch_atlas.json. Returns the maps'
    paths."""
    color = np.empty((ATLAS, ATLAS, 3), np.float32)
    color[:] = to_linear(srgb01(0x202226))
    normal = np.zeros((ATLAS, ATLAS, 3), np.float32)
    normal[..., 2] = 1.0
    ort = np.empty((ATLAS, ATLAS, 3), np.float32)
    ort[:] = (1.0, 0.8, 0.0)
    looks = {}
    for b in BRANDS:
        for shape, draw, cell, size_mm, px in (('wide', wide_design, wide_cell(b.index), WIDE_MM, WIDE_PX[0] * 2),
                                               ('round', round_design, round_cell(b.index), (ROUND_MM, ROUND_MM), ROUND_PX * 2)):
            rgb, palette = render_design(draw, b, size_mm[0], size_mm[1], px, f'patch_{b.id}_{shape}')
            c, n, o = embroider(rgb, palette, px / size_mm[0], seed=b.index * 2 + (shape == 'round'))
            x, y, w, h = cell
            if c.shape[:2] != (h, w):
                raise RuntimeError(f'{b.id} {shape}: the design came out {c.shape[1]} x {c.shape[0]}, its cell is {w} x {h}')
            color[y:y + h, x:x + w] = c
            normal[y:y + h, x:x + w] = n
            ort[y:y + h, x:x + w] = o
            # The thread's swatch, outside the outline: M_Patch colours the merrowed edge from it.
            color[y:y + SWATCH, x:x + SWATCH] = to_linear(srgb01(b.thread))
            normal[y:y + SWATCH, x:x + SWATCH] = (0.0, 0.0, 1.0)
            ort[y:y + SWATCH, x:x + SWATCH] = (1.0, 0.42, 1.0)
            looks[(b.id, shape)] = (c, n, o)
            print(f'[sponsors] {b.id} {shape}: {len(palette)} colours')
    paths = {
        'BaseColor': write_png(os.path.join(out_dir, 'T_PatchAtlas_BaseColor.png'), to_srgb(color)),
        'Normal': write_png(os.path.join(out_dir, 'T_PatchAtlas_Normal.png'), normal * 0.5 + 0.5),
        'ORT': write_png(os.path.join(out_dir, 'T_PatchAtlas_ORT.png'), ort),
    }
    meta = {
        'about': "The sponsors' patch atlas (art/blender/sponsors.py). Cells by brand index (sponsor::Brand::Mark), pixels from "
                 'the top left. Normal: tangent space, OpenGL (+G up). ORT: R occlusion, G roughness, B thread (1 embroidered).',
        'size': ATLAS,
        'wide': {'cell': list(WIDE_PX), 'cols': WIDE_COLS, 'y0': 0, 'mm': [round(v, 4) for v in WIDE_MM], 'cap_mm': [round(v, 4) for v in CAP_MM]},
        'round': {'cell': ROUND_PX, 'cols': ROUND_COLS, 'y0': ROUND_Y0, 'mm': ROUND_MM},
        'swatch': SWATCH,
        'brands': [b.id for b in BRANDS],
        'threads': {b.id: f'#{b.thread:06x}' for b in BRANDS},
        'maps': {k: os.path.basename(v) for k, v in paths.items()},
    }
    with open(os.path.join(out_dir, 'patch_atlas.json'), 'w', encoding='utf-8', newline='\n') as f:
        json.dump(meta, f, indent=2)
        f.write('\n')
    if review:
        review_atlas(color, normal, ort, looks)
    return paths


def review_atlas(color, normal, ort, looks):
    """art/review/wear_patch_atlas.jpg: the atlas's colour over the atlas lit (left); the P0 brands' faces lit at full
    size, and the Embercrest's at twice that, to judge the stitches (right)."""
    def down(img):
        return img.reshape(1024, 2, 1024, 2, 3).mean(axis=(1, 3))
    dn = down(normal)
    dn /= np.linalg.norm(dn, axis=2, keepdims=True)
    left = np.concatenate([to_srgb(down(color)), lit(down(color), dn, down(ort))], axis=0)
    panel = np.full((2048, 1628, 3), 0.05, np.float32)
    x = y = 8
    for bid in P0:
        c, n, o = looks[(bid, 'wide')]
        cr, nr, orr = looks[(bid, 'round')]
        panel[y:y + 344, x:x + 512] = lit(c, n, o)
        panel[y:y + 292, x + 520:x + 812] = lit(cr, nr, orr)
        x += 800
        if x + 812 > 1628:
            x = 8
            y += 352
    c, n, o = looks[('embercrest', 'wide')]
    big = np.repeat(np.repeat(lit(c, n, o), 2, axis=0), 2, axis=1)[:2048 - y - 16, :1544]
    panel[y + 8:y + 8 + big.shape[0], 8:8 + big.shape[1]] = big
    os.makedirs(core.REVIEW_DIR, exist_ok=True)
    write_jpg(os.path.join(core.REVIEW_DIR, 'wear_patch_atlas.jpg'), np.concatenate([left, panel], axis=1))


if __name__ == '__main__':
    core.reset()
    build_atlas()
