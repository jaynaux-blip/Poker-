"""SM_Stock_*: what the Lucky Penny #212 sells, faced up on its shelves the way Benny left them at shift change.

Every product is a real-sized pack in the store's own brands: the fourteen items on its menu (Cascade water,
Fizz Cola, Volt Rush, Night Owl cold brew, Sunny Peach tea, Hilltop chips, Choco Stack, Ridgeline trail mix,
Roller Dog franks, Big Bean burritos, Oodle Cups, Deli Wedge sandwiches, Penny's drip coffee and hot cocoa),
dressed as the counter's menu draws them (the colors, the brand's initials big on the front), and the filler a
corner store carries round them: sodas, sports drinks, chips, candy, cereal, soup, motor oil, paper towels,
batteries, cigarettes, scratch-offs and phone cards.

Each product is modeled and baked once (its label printed, wrapped and baked with its plastic, glass, foil or
aluminum), then copied into ROWS: one mesh fills one shelf of one cooler door or one gondola bay, two or three
deep, faced to the front edge, a little turned and a little shopped. A row carries one material whose texture
atlas holds only its own products (BaseColor at about 1800 texels a meter; ORM and Normal at half that).

Coordinates (meters), Z up, every row's front toward -Y (the shopper), its origin ON THE SHELF TOP:
- SM_Stock_CoolerRow_*: centered on one door of SM_Cooler, origin at Y 0 = the cooler's own middle plane (the
  shelf's centerline), so a row sits at (door center X, 0, shelf top Z) in the cooler's frame. Footprint 0.70 x
  0.72 (cooler.DOOR_W - 0.06 by cooler.DEPTH - 0.18); products start just behind the price rail (Y -0.36), 2-3 deep.
- SM_Stock_ShelfRow_*: centered on one 1.2 m bay of SM_Shelf, origin at Y 0 = the gondola's spine line, the
  products on its -Y face (Y -0.295 to -0.035), 1.08 m wide (clear of the uprights). For the +Y face, the same
  row turned 180 degrees about Z at the same origin. _Bulk is the base deck's row (0.25 m clear); the rest fit
  the 0.43 m between shelves.
- SM_Stock_BackBar: the wall behind the counter: a base cabinet with cartons on it, the tobacco merchandiser's
  carcass, the scratch-off dispenser and a strip of phone and gift cards on hooks. Origin on the floor at the middle
  of its back (the wall), front toward -Y, 4.6 m wide, 0.48 deep, 2.42 tall. SM_Stock_BackBar_Tobacco (the packs in
  the merchandiser's pushers, the dividers, the price strips and the header) shares its origin: place both at once.
- SM_Stock_Counter: the impulse buys beside a register: a Choco Stack caddy, a gum rack, a lighter box and mint
  tins. Origin on the counter top at the middle of the group, front (the customer's side) toward -Y.

SHORTSTACK_STOCK=Soda,BackBar (any parts of row names) builds only those rows and the products they use.
"""
import math
import os
import random
import zlib

import bpy  # noqa: F401,I001
import bmesh
import numpy as np
from mathutils import Matrix, Vector
from mathutils import noise as mnoise

from artkit import core, parts
from artkit.shade import circle_dist, image_surface, mix_float, noise, object_coords, ramp, rrect_mask, scaled, smooth_less, vec
from artkit.sheet import Sheet
from assets import cooler, store_shelf

COOLER_ROWS = ['SM_Stock_CoolerRow_Soda', 'SM_Stock_CoolerRow_Cans', 'SM_Stock_CoolerRow_Energy', 'SM_Stock_CoolerRow_Water',
               'SM_Stock_CoolerRow_Tea', 'SM_Stock_CoolerRow_Deli', 'SM_Stock_CoolerRow_Ice']
SHELF_ROWS = ['SM_Stock_ShelfRow_Chips', 'SM_Stock_ShelfRow_Trail', 'SM_Stock_ShelfRow_Candy', 'SM_Stock_ShelfRow_Grocery',
              'SM_Stock_ShelfRow_Household', 'SM_Stock_ShelfRow_Bulk']
MESHES = COOLER_ROWS + SHELF_ROWS + ['SM_Stock_BackBar', 'SM_Stock_BackBar_Tobacco', 'SM_Stock_Counter']
DOUBLE_SIDED = False
TEXTURE_SIZE = 512
TEXTURE_SIZES = {}  # each product's own bake, sized to its surface (filled in by build)
AO_DISTANCE = 0.012
REVIEW_SCREEN_LIGHT = False
# The cooler's doors on the left, a gondola bay in front of them, the back bar and the counter set to the right.
REVIEW_VIEWS = [('cooler', -12, 5, 0.6, (-2.28, -0.3, 1.0)), ('gondola', -10, 10, 0.5, (0.6, -2.6, 0.8)),
                ('backbar', -10, 5, 0.75, (5.4, 0.4, 1.35))]

DENSITY = 1800.0   # baked texels per meter of a product's surface
PAD = 4            # texels of bake margin round each product's tile in its atlas
# The one-off pieces seen across the counter keep their full density rather than trade it for a smaller atlas.
HERO_FLOOR = {'SM_Stock_BackBar': 0.95, 'SM_Stock_BackBar_Tobacco': 0.95, 'SM_Stock_Counter': 0.95}
SHEET_PX_MM = 4.0  # label art rendered at about twice the bake's density

# The shelves it fills.
DOOR_SHELF_W = cooler.DOOR_W - 0.06
DOOR_SHELF_D = cooler.DEPTH - 0.18
COOLER_FRONT = -DOOR_SHELF_D / 2 + 0.006
COOLER_CLEAR = (cooler.SHELF_Z[1] - cooler.SHELF_Z[0]) - 0.016 - 0.01
GONDOLA_FRONT = -(store_shelf.SPINE / 2 + store_shelf.DEPTH) + 0.005
GONDOLA_BACK = -store_shelf.SPINE / 2
BAY_W = 1.08
GONDOLA_CLEAR = (store_shelf.SHELF_Z[1] - store_shelf.SHELF_Z[0]) - 0.02 - 0.01
DECK_CLEAR = store_shelf.SHELF_Z[0] - 0.02 - store_shelf.BASE - 0.01

ONLY = [s.strip() for s in os.environ.get('SHORTSTACK_STOCK', '').split(',') if s.strip()]


# ================================================================== little helpers

def sh(text):
    """A stable small hash (Python's own is salted per run, and the art must come out the same every build)."""
    return zlib.crc32(text.encode())


def mix_hex(a, b, t):
    ca = [(a >> k) & 255 for k in (16, 8, 0)]
    cb = [(b >> k) & 255 for k in (16, 8, 0)]
    c = [int(round(x + (y - x) * t)) for x, y in zip(ca, cb)]
    return (c[0] << 16) | (c[1] << 8) | c[2]


def shade(h, k):
    """The menu's Shade: toward white for k > 0, toward black for k < 0."""
    return mix_hex(h, 0xffffff, k) if k >= 0 else mix_hex(h, 0x000000, -k)


# ================================================================== label art (Sheet helpers, millimeters)

_WIDTHS = {}


def text_w(s, text, face='Black', tracking=1.0):
    """Width of text at size 1 (cached)."""
    key = (text, face, tracking)
    if key not in _WIDTHS:
        cu = bpy.data.curves.new('m', 'FONT')
        cu.body = text
        cu.font = s._font(face)
        cu.size = 1.0
        cu.space_character = tracking
        ob = bpy.data.objects.new('m', cu)
        core.link(ob)
        bpy.context.view_layer.update()
        _WIDTHS[key] = max(ob.dimensions.x, 1e-3)
        bpy.data.objects.remove(ob)
        bpy.data.curves.remove(cu)
    return _WIDTHS[key]


def fit(s, text, x, y, size, max_w, color, face='Black', align='CENTER', tracking=1.0, rough=0.4, metal=0.0, rot=0.0):
    """Text no wider than max_w (shrunk to fit), baseline at y. Returns the size used."""
    size = min(size, max_w / text_w(s, text, face, tracking))
    s.text(text, x, y, size, color, face=face, align=align, tracking=tracking, rough=rough, metal=metal, rot=rot)
    return size


def mid(s, text, x, cy, size, max_w, color, **kw):
    """Text centered on cy by its cap height."""
    size = min(size, max_w / text_w(s, text, kw.get('face', 'Black'), kw.get('tracking', 1.0)))
    return fit(s, text, x, cy - 0.355 * size, size, max_w, color, **kw)


def barcode(s, x, y, w, h, seed=1, ink=0x0b0b0b, paper=0xf7f7f3):
    s.rect(x, y, w, h, paper, rough=0.45)
    rnd = random.Random(seed)
    unit = w / 70.0
    bx = x + w * 0.08
    while bx < x + w * 0.9:
        bw = unit * rnd.choice((1, 1, 2, 3))
        s.rect(bx, y + h * 0.22, bw, h * 0.7, ink, rough=0.45)
        bx += bw + unit * rnd.choice((1, 1, 2))
    s.text(f'0 {rnd.randint(10000, 99999)} {rnd.randint(10000, 99999)} {rnd.randint(1, 9)}', x + w / 2, y + h * 0.04, h * 0.15, ink,
           face='Regular', align='CENTER', rough=0.45)


def nutrition(s, x, y, w, h, ink=0x141414, paper=0xf3f1ea):
    """A nutrition panel: real headings, the small print as rules and bars."""
    s.rect(x, y, w, h, paper, rough=0.5)
    pad = w * 0.06
    fit(s, 'Nutrition Facts', x + pad, y + h - h * 0.13, h * 0.11, w - 2 * pad, ink, face='Black', align='LEFT')
    s.rect(x + pad, y + h - h * 0.17, w - 2 * pad, h * 0.025, ink)
    yy = y + h - h * 0.24
    k = 0
    while yy > y + h * 0.06:
        bar = (w - 2 * pad) * (0.55 if k % 3 else 0.35)
        s.rect(x + pad, yy, bar, h * 0.03, mix_hex(ink, paper, 0.35))
        s.rect(x + w - pad - (w - 2 * pad) * 0.16, yy, (w - 2 * pad) * 0.16, h * 0.03, mix_hex(ink, paper, 0.35))
        s.rect(x + pad, yy - h * 0.022, w - 2 * pad, h * 0.006, ink)
        yy -= h * 0.075
        k += 1


def text_lines(s, x, y, w, h, color, rows=6, seed=3):
    """Small print as gray bars (unreadable at any distance anyway)."""
    rnd = random.Random(seed)
    step = h / rows
    for k in range(rows):
        lw = w * (rnd.uniform(0.7, 1.0) if k < rows - 1 else rnd.uniform(0.3, 0.6))
        s.rect(x, y + h - (k + 1) * step + step * 0.3, lw, step * 0.4, color)


def star(s, cx, cy, r0, r1, color, points=14, rot=0.0):
    pts = []
    for k in range(points * 2):
        a = rot + math.pi * k / points
        r = r1 if k % 2 == 0 else r0
        pts.append((cx + r * math.cos(a), cy + r * math.sin(a)))
    s.poly(pts, color, rough=0.35)


def bolt(s, cx, cy, size, color, rough=0.3, metal=0.0):
    """The menu's energy bolt."""
    S = size
    p = [(0.1, 0.48), (-0.26, -0.06), (-0.02, -0.06), (-0.1, -0.48), (0.26, 0.06), (0.02, 0.06)]
    s.poly([(cx + S * a, cy + S * b) for a, b in p], color, rough=rough, metal=metal)


def drop(s, cx, cy, size, color):
    """The menu's water drop."""
    S = size
    s.poly([(cx, cy + S * 0.46), (cx + S * 0.28, cy - S * 0.06), (cx, cy - S * 0.18), (cx - S * 0.28, cy - S * 0.06)], color, rough=0.35)
    s.circle(cx, cy - S * 0.14, S * 0.29, color, rough=0.35, n=40)


def peach(s, cx, cy, r):
    """Sunny Peach's cartoon peach: a blushing fruit with a leaf and a smile."""
    s.circle(cx, cy, r, 0xf7934c, n=64, rough=0.35)
    s.circle(cx + r * 0.25, cy + r * 0.18, r * 0.62, 0xffb36b, n=48, rough=0.35)
    s.circle(cx - r * 0.35, cy - r * 0.1, r * 0.42, 0xef6f3a, n=40, rough=0.35)
    s.poly([(cx - r * 0.05, cy + r * 0.95), (cx + r * 0.08, cy + r * 0.95), (cx + r * 0.05, cy + r * 1.25), (cx - r * 0.02, cy + r * 1.25)], 0x5b3a1a, rough=0.4)
    s.poly([(cx + r * 0.05, cy + r * 1.1), (cx + r * 0.6, cy + r * 1.45), (cx + r * 0.95, cy + r * 1.2), (cx + r * 0.5, cy + r * 0.95)], 0x4f8f3a, rough=0.4)
    s.circle(cx - r * 0.3, cy + r * 0.12, r * 0.09, 0x2b1a12, n=20)
    s.circle(cx + r * 0.25, cy + r * 0.12, r * 0.09, 0x2b1a12, n=20)
    s.ring(cx - 0.02 * r, cy - r * 0.05, r * 0.3, r * 0.38, 0x2b1a12, n=40, a0=math.pi * 1.15, a1=math.pi * 1.85)


def lemon(s, cx, cy, r):
    s.poly([(cx + r * 1.2 * math.cos(2 * math.pi * k / 48), cy + r * 0.82 * math.sin(2 * math.pi * k / 48)) for k in range(48)], 0xf2d53c, rough=0.35)
    s.circle(cx + r * 1.15, cy, r * 0.14, 0xe2b81c, n=16)
    s.poly([(cx - r * 0.1, cy + r * 0.7), (cx + r * 0.5, cy + r * 1.2), (cx + r * 0.8, cy + r * 0.95), (cx + r * 0.3, cy + r * 0.62)], 0x4f8f3a, rough=0.4)
    s.circle(cx - r * 0.35, cy + r * 0.05, r * 0.08, 0x2b1a12, n=16)
    s.circle(cx + r * 0.25, cy + r * 0.05, r * 0.08, 0x2b1a12, n=16)


def owl(s, cx, cy, size, color, eye=0x24160f):
    S = size
    s.poly([(cx - S * 0.5, cy + S * 0.25), (cx - S * 0.42, cy + S * 0.62), (cx - S * 0.18, cy + S * 0.38), (cx + S * 0.18, cy + S * 0.38),
            (cx + S * 0.42, cy + S * 0.62), (cx + S * 0.5, cy + S * 0.25), (cx + S * 0.42, cy - S * 0.3), (cx, cy - S * 0.55), (cx - S * 0.42, cy - S * 0.3)], color, rough=0.3, metal=0.6)
    for sx in (-1, 1):
        s.circle(cx + sx * S * 0.2, cy + S * 0.12, S * 0.17, eye, n=32)
        s.circle(cx + sx * S * 0.2, cy + S * 0.12, S * 0.07, color, n=24, metal=0.6, rough=0.3)
    s.poly([(cx - S * 0.06, cy - S * 0.02), (cx + S * 0.06, cy - S * 0.02), (cx, cy - S * 0.16)], eye)


def hills(s, x, y, w, h, color, seed=2, waves=3):
    rnd = random.Random(seed)
    pts = [(x, y)]
    for k in range(41):
        t = k / 40
        pts.append((x + w * t, y + h * (0.55 + 0.45 * math.sin(t * math.pi * waves + rnd.uniform(-0.1, 0.1)) * 0.5 + 0.2 * math.sin(t * 17.0))))
    pts.append((x + w, y))
    s.poly(pts, color, rough=0.4)


def peaks(s, x, y, w, h, color, snow=None):
    pts = [(x, y), (x, y + h * 0.35), (x + w * 0.22, y + h * 0.85), (x + w * 0.34, y + h * 0.6), (x + w * 0.55, y + h), (x + w * 0.72, y + h * 0.55),
           (x + w * 0.84, y + h * 0.7), (x + w, y + h * 0.3), (x + w, y)]
    s.poly(pts, color, rough=0.4)
    if snow is not None:
        s.poly([(x + w * 0.47, y + h * 0.82), (x + w * 0.55, y + h), (x + w * 0.63, y + h * 0.82), (x + w * 0.58, y + h * 0.86), (x + w * 0.52, y + h * 0.8)], snow, rough=0.4)


def chips_pile(s, cx, cy, r, shadow=0x8a6a3a, seed=5, n=16):
    """A mound of kettle chips: each one a curled, wobbly oval, golden with a darker fried rim, a few brown spots and
    a highlight, dropping a shadow on the ones under it."""
    rnd = random.Random(seed)
    chips = []
    for k in range(n):
        # More of them low and in the middle: a heap.
        u = rnd.uniform(-1.0, 1.0)
        v = rnd.uniform(0.0, 1.0) ** 1.6
        x = cx + u * r * (1.0 - 0.45 * v)
        y = cy - r * 0.55 + v * r * 1.05
        chips.append((y, x))
    for y, x in sorted(chips, reverse=True):
        rx, ry = r * rnd.uniform(0.26, 0.36), r * rnd.uniform(0.17, 0.24)
        rot = rnd.uniform(-0.8, 0.8)
        ph = rnd.uniform(0, 6.28)

        def outline(sc, dx=0.0, dy=0.0):
            pts = []
            for i in range(30):
                t = 2 * math.pi * i / 30
                q = 1.0 + 0.09 * math.sin(3 * t + ph) + 0.05 * math.sin(5 * t + 2 * ph)
                px, py = rx * sc * q * math.cos(t), ry * sc * q * math.sin(t)
                pts.append((x + dx + px * math.cos(rot) - py * math.sin(rot), y + dy + px * math.sin(rot) + py * math.cos(rot)))
            return pts
        s.poly(outline(1.0, r * 0.03, -r * 0.04), shadow, rough=0.45, alpha=0.55)
        s.poly(outline(1.0), 0xbf7a26, rough=0.45)
        s.poly(outline(0.84, -rx * 0.04, ry * 0.06), 0xe0a23e, rough=0.45)
        s.poly(outline(0.45, -rx * 0.25, ry * 0.25), 0xf2c66a, rough=0.4)
        for q in range(rnd.randint(1, 3)):
            a, d = rnd.uniform(0, 6.28), rnd.uniform(0.2, 0.7)
            s.circle(x + rx * d * math.cos(a), y + ry * d * math.sin(a), r * rnd.uniform(0.012, 0.025), 0xa8682a, n=10, rough=0.45)


def nut_window(s, x, y, w, h, seed=8):
    """Ridgeline's window: trail mix behind the film."""
    s.rect(x, y, w, h, 0xe8d7b4, rough=0.25)
    rnd = random.Random(seed)
    cols = [0x7a4a22, 0xb07a3f, 0x5a3418, 0xd9b27a, 0x8a2a2a, 0x3a2418, 0xc8955a]
    for k in range(int(w * h / 22)):
        cx, cy = x + rnd.uniform(2, w - 2), y + rnd.uniform(2, h - 2)
        r = rnd.uniform(1.6, 3.2)
        if rnd.random() < 0.5:
            s.circle(cx, cy, r, rnd.choice(cols), n=14, rough=0.4)
        else:
            a = rnd.uniform(0, math.pi)
            s.poly([(cx + r * 1.5 * math.cos(a + t) * (1 if i % 2 else 0.6), cy + r * math.sin(a + t) * (1 if i % 2 else 0.6))
                    for i, t in enumerate([2 * math.pi * j / 10 for j in range(10)])], rnd.choice(cols), rough=0.4)


def penny_logo(s, cx, cy, r, ink=0x6b3412):
    """The Lucky Penny's coin with a clover (as on its sign)."""
    s.circle(cx, cy, r, 0xd08a4e, n=64, metal=0.6, rough=0.3)
    s.ring(cx, cy, r * 0.78, r * 0.86, ink, n=64)
    for k in range(4):
        a = k * math.pi / 2 + math.pi / 4
        s.circle(cx + math.cos(a) * r * 0.22, cy + r * 0.06 + math.sin(a) * r * 0.22, r * 0.2, 0x5a2a0c, n=24)
    s.poly([(cx - r * 0.03, cy), (cx + r * 0.03, cy), (cx + r * 0.1, cy - r * 0.42), (cx + r * 0.04, cy - r * 0.42)], 0x5a2a0c)


def render(s, sid, w_mm):
    return s.render(f'stock_{sid}', int(min(4096, max(256, w_mm * SHEET_PX_MM))))


# ================================================================== shader building blocks

class Surf:
    """A finish: color (hex, rgba tuple or socket), roughness and metallic (floats or sockets)."""

    def __init__(self, color, rough=0.4, metal=0.0):
        self.color = core.hex_linear(color) if isinstance(color, int) else color
        self.rough = rough
        self.metal = metal


def over(m, mask, a, b):
    """b laid over a where mask is 1."""
    return Surf(m.mix(mask, a.color, b.color), mix_float(m, mask, a.rough, b.rough), mix_float(m, mask, a.metal, b.metal))


def band(m, z, z0, z1, soft=0.0002):
    return m.math('MULTIPLY', ramp(m, z, z0 - soft, z0 + soft), m.math('SUBTRACT', 1.0, ramp(m, z, z1 - soft, z1 + soft)))


def cyl_uv(m, sep, z0, z1):
    """Label coordinates wrapped round Z: u by angle (0.25 faces -Y; it reads left to right from outside), v up the
    band from z0 to z1."""
    ang = m.math('ARCTAN2', sep.outputs['Y'], sep.outputs['X'])
    u = m.math('ADD', m.math('DIVIDE', ang, 2 * math.pi), 0.5)
    v = m.math('DIVIDE', m.math('SUBTRACT', sep.outputs['Z'], z0), z1 - z0)
    return vec(m, u, v, 0.0)


def radius(m, sep):
    x, y = sep.outputs['X'], sep.outputs['Y']
    return m.math('SQRT', m.math('ADD', m.math('MULTIPLY', x, x), m.math('MULTIPLY', y, y)))


def part_mask(m, k, attr=None):
    """1 on faces whose 'part' attribute is k."""
    if attr is None:
        attr = m.node('ShaderNodeAttribute')
        attr.attribute_type = 'GEOMETRY'
        attr.attribute_name = 'part'
    n = m.nt.nodes.new('ShaderNodeMath')
    n.operation = 'COMPARE'
    m.link(attr.outputs['Fac'], n.inputs[0])
    n.inputs[1].default_value = float(k)
    n.inputs[2].default_value = 0.5
    return n.outputs[0]


def printed(m, paths, uv):
    ink, alpha, metal, rough = image_surface(m, paths, uv)
    return Surf(ink, rough, metal), alpha


def frost(m, tc, s, amount, mask=None):
    """Condensation on something cold: a fogged bloom, beads of water. Returns (finish, bead height)."""
    o = tc.outputs['Object']
    fog = m.math('MULTIPLY', ramp(m, noise(m, o, scale=22.0, detail=4.0), 0.48, 0.78), amount)
    vor = m.node('ShaderNodeTexVoronoi', Scale=190.0)
    m.link(o, vor.inputs['Vector'])
    beads = m.math('MULTIPLY', smooth_less(m, vor.outputs['Distance'], 0.18, 0.05),
                   m.math('MULTIPLY', ramp(m, noise(m, o, scale=11.0, detail=2.0), 0.45, 0.62), amount))
    if mask is not None:
        fog = m.math('MULTIPLY', fog, mask)
        beads = m.math('MULTIPLY', beads, mask)
    # Fog is mostly a loss of gloss: the color only lifts a touch (linear space: a little goes a long way).
    col = m.mix(m.math('MULTIPLY', fog, 0.035), s.color, (0.8, 0.84, 0.88, 1.0))
    rough = mix_float(m, m.math('MULTIPLY', fog, 0.8), s.rough, 0.3)
    rough = mix_float(m, beads, rough, 0.05)
    return Surf(col, rough, s.metal), beads


def finish_mat(m, s, height=None, strength=0.3, distance=0.0004):
    m.set('Base Color', s.color)
    m.set('Roughness', s.rough)
    m.set('Metallic', s.metal)
    if height is not None:
        bump = m.node('ShaderNodeBump', Strength=strength, Distance=distance)
        m.link(height, bump.inputs['Height'])
        m.set('Normal', bump.outputs['Normal'])
    return m


def add(m, a, b):
    if a is None:
        return b
    if b is None:
        return a
    return m.math('ADD', a, b)


def knurl(m, sep, count, mask):
    """Vertical ridges round a cap."""
    ang = m.math('ARCTAN2', sep.outputs['Y'], sep.outputs['X'])
    r = m.math('ABSOLUTE', m.math('SINE', m.math('MULTIPLY', ang, count / 2.0)))
    return m.math('MULTIPLY', r, mask)


def crinkle(m, tc, sep, amount=1.0):
    """Creased film: long folds and fine crackle."""
    folds = noise(m, scaled(m, sep, 18.0, 18.0, 6.0), detail=3.0, distortion=1.2)
    fine = noise(m, tc.outputs['Object'], scale=160.0, detail=2.0)
    return m.math('MULTIPLY', m.math('ADD', m.math('MULTIPLY', folds, 1.0), m.math('MULTIPLY', fine, 0.25)), amount)


def edge_wear(m, tc, s, amount=0.5, hex_color=0xf2efe6):
    """Scuffed corners on a printed carton: the print rubbed off toward the board."""
    from artkit.shade import convex_edges
    e = m.math('MULTIPLY', convex_edges(m, tc, distance=0.0015, samples=10, breakup_scale=180.0, gain=4.0), amount)
    e = ramp(m, e, 0.3, 0.6)
    return Surf(m.mix(e, s.color, core.hex_linear(hex_color)), mix_float(m, e, s.rough, 0.7), s.metal)


def dust(m, tc, s, amount):
    """A slow mover's shelf dust: a dry grey veil on the faces that look up, a little uneven (a fine, low-contrast
    variation: coarse patches read as stains)."""
    geo = m.node('ShaderNodeNewGeometry')
    nsep = m.node('ShaderNodeSeparateXYZ')
    m.link(geo.outputs['Normal'], nsep.inputs['Vector'])
    o = tc.outputs['Object']
    up = ramp(m, nsep.outputs['Z'], 0.55, 0.92)
    patch = m.math('ADD', 0.65, m.math('MULTIPLY', ramp(m, noise(m, o, scale=120.0, detail=4.0, roughness=0.6), 0.3, 0.75), 0.35))
    d = m.math('MULTIPLY', m.math('MULTIPLY', up, patch), amount)
    return Surf(m.mix(d, s.color, core.hex_linear(0xb9b3a8)), mix_float(m, d, s.rough, 0.9), mix_float(m, d, s.metal, 0.0))


def net_uv(m, sep, w, d, h, z0=0.0):
    """A box's six faces mapped into a net sheet (millimeter layout from net_rects), picked by the shading
    normal: front -Y, the right side +X, back +Y, the left side -X, top and bottom above them."""
    geo = m.node('ShaderNodeNewGeometry')
    nsep = m.node('ShaderNodeSeparateXYZ')
    m.link(geo.outputs['Normal'], nsep.inputs['Vector'])
    nx, ny, nz = nsep.outputs['X'], nsep.outputs['Y'], nsep.outputs['Z']
    ax, ay, az = (m.math('ABSOLUTE', c) for c in (nx, ny, nz))
    is_x = m.math('MULTIPLY', m.math('GREATER_THAN', ax, ay), m.math('GREATER_THAN', ax, az))
    is_z = m.math('MULTIPLY', m.math('GREATER_THAN', az, ax), m.math('GREATER_THAN', az, ay))
    x, y, z = sep.outputs['X'], sep.outputs['Y'], m.math('SUBTRACT', sep.outputs['Z'], z0)
    W = 2 * w + 2 * d
    H = h + d

    def lin(src, scale, off):
        return m.math('ADD', m.math('MULTIPLY', src, scale), off)
    # u (meters along the sheet) for each face; v likewise.
    u_front = lin(x, 1.0, w / 2)
    u_right = lin(y, 1.0, w + d / 2)
    u_back = lin(x, -1.0, w + d + w / 2)
    u_left = lin(y, -1.0, 2 * w + d + d / 2)
    u_top = lin(x, 1.0, w / 2)
    u_bot = lin(x, 1.0, w + d + w / 2)
    v_side = z
    v_top = lin(y, 1.0, h + d / 2)
    v_bot = lin(y, -1.0, h + d / 2)
    pos_x = m.math('GREATER_THAN', nx, 0.0)
    pos_y = m.math('GREATER_THAN', ny, 0.0)
    pos_z = m.math('GREATER_THAN', nz, 0.0)
    u_y = mix_float(m, pos_y, u_front, u_back)
    u_x = mix_float(m, pos_x, u_left, u_right)
    u_z = mix_float(m, pos_z, u_bot, u_top)
    v_z = mix_float(m, pos_z, v_bot, v_top)
    u = mix_float(m, is_z, mix_float(m, is_x, u_y, u_x), u_z)
    v = mix_float(m, is_z, v_side, v_z)
    return vec(m, m.math('DIVIDE', u, W), m.math('DIVIDE', v, H), 0.0)


def net_rects(w, d, h):
    """The faces' rectangles (x, y, width, height) in a net sheet, millimeters."""
    w, d, h = w * 1000, d * 1000, h * 1000
    return {'front': (0, 0, w, h), 'right': (w, 0, d, h), 'back': (w + d, 0, w, h), 'left': (2 * w + d, 0, d, h),
            'top': (0, h, w, d), 'bottom': (w + d, h, w, d), 'W': 2 * w + 2 * d, 'H': h + d}


def two_face_uv(m, sep, w, h, z0=0.0):
    """A bag's front (-Y) and back (+Y) into a sheet [front | back], each w by h."""
    geo = m.node('ShaderNodeNewGeometry')
    nsep = m.node('ShaderNodeSeparateXYZ')
    m.link(geo.outputs['Normal'], nsep.inputs['Vector'])
    back = m.math('GREATER_THAN', nsep.outputs['Y'], 0.0)
    x = sep.outputs['X']
    u_front = m.math('DIVIDE', m.math('ADD', x, w / 2), 2 * w)
    u_back = m.math('DIVIDE', m.math('ADD', m.math('MULTIPLY', x, -1.0), w * 1.5), 2 * w)
    u = mix_float(m, back, u_front, u_back)
    v = m.math('DIVIDE', m.math('SUBTRACT', sep.outputs['Z'], z0), h)
    return vec(m, u, v, 0.0)


def front_uv(m, sep, x0, x1, z0, z1):
    """Front elevation (X right, Z up) of a fixture."""
    u = m.math('DIVIDE', m.math('SUBTRACT', sep.outputs['X'], x0), x1 - x0)
    v = m.math('DIVIDE', m.math('SUBTRACT', sep.outputs['Z'], z0), z1 - z0)
    return vec(m, u, v, 0.0)


# ================================================================== geometry

def tag(obj, value):
    """Marks every face of a part (the 'part' attribute the materials read)."""
    a = obj.data.attributes.get('part') or obj.data.attributes.new('part', 'FLOAT', 'FACE')
    a.data.foreach_set('value', [float(value)] * len(obj.data.polygons))
    return obj


def cull_hidden(obj, backs=False):
    """Drops a wall fixture's faces nobody sees (flat on the wall behind it at Y 0, or on the floor; with backs, every
    face turned to the wall), so its bake spends no texels there."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.normal_update()
    doomed = [f for f in bm.faces if (f.normal.y > 0.9 and (backs or f.calc_center_median().y > -0.004)) or (f.normal.z < -0.9 and f.calc_center_median().z < 0.003)]
    bmesh.ops.delete(bm, geom=doomed, context='FACES_ONLY')
    bm.to_mesh(obj.data)
    bm.free()
    return obj


def cull_where(obj, test):
    """Drops the faces test(face, center) picks."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.normal_update()
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if test(f, f.calc_center_median())], context='FACES_ONLY')
    bm.to_mesh(obj.data)
    bm.free()
    return obj


def round32(v):
    return max(64, int(math.ceil(v / 32.0)) * 32)


def tile_for_area(obj, density=DENSITY):
    """A square tile giving obj about density texels a meter once its faces are box-packed."""
    area = sum(p.area for p in obj.data.polygons)
    side = math.sqrt(area / 0.62) * density
    return round32(side), round32(side)


def finish_master(obj, mat, tile, mode='box', sharp=40.0):
    """One product, ready to bake: its material, smooth shading broken at sharp edges, UVs inset in its tile."""
    core.assign(obj, mat)
    obj.data.shade_smooth()
    obj.data.set_sharp_from_angle(angle=math.radians(sharp))
    w, h = tile
    w, h = min(w, 2048), min(h, 2048)
    rect = (PAD / w, PAD / h, 1.0 - PAD / w, 1.0 - PAD / h)
    core.uv_layout(obj, [(lambda f: True, mode, None if mode == 'keep' else 10.0 / min(w, h), rect)])
    TEXTURE_SIZES[mat.m.name if hasattr(mat, 'm') else mat.name] = (w, h)
    return obj


LATHES = {}  # product id -> its profile, so a row can re-turn a lighter copy for the back ranks


def arc_length(profile):
    return sum(math.hypot(b[0] - a[0], b[1] - a[1]) for a, b in zip(profile, profile[1:]))


def lathe_master(sid, profile, segs, mat, density=DENSITY):
    obj = core.lathe(f'stock_{sid}', profile, segments=segs)
    core.orient_normals(obj)
    rmax = max(r for r, _ in profile)
    tile = (round32(2 * math.pi * rmax * density), round32(arc_length(profile) * density))
    LATHES[sid] = profile
    return finish_master(obj, mat, tile, mode='keep', sharp=50.0)


def can_profile(H):
    return [(0.0, 0.0102), (0.0115, 0.0095), (0.0195, 0.0068), (0.0238, 0.0028), (0.0256, 0.0), (0.0283, 0.0016), (0.0312, 0.0058),
            (0.0331, 0.0135), (0.0331, H - 0.0168), (0.0322, H - 0.0118), (0.0302, H - 0.0074), (0.0283, H - 0.0036), (0.0281, H - 0.0010),
            (0.0274, H + 0.0002), (0.0264, H - 0.0010), (0.0262, H - 0.0030), (0.0250, H - 0.0034), (0.0160, H - 0.0030), (0.0, H - 0.0029)]


def pet_profile(H=0.2125, R=0.0335, neck=0.0145, cap=0.016):
    """A carbonated-drink PET bottle: petaloid foot, ribbed waist, label panel, shoulder, support ring, cap."""
    s = H / 0.2125
    return [(0.0, 0.0045 * s), (0.012, 0.004 * s), (0.021, 0.0008 * s), (0.026, 0.0), (0.0305, 0.0035 * s), (R, 0.012 * s), (R, 0.052 * s),
            (R - 0.0012, 0.0565 * s), (R, 0.061 * s), (R, 0.134 * s), (R - 0.0008, 0.142 * s), (R - 0.0045, 0.155 * s), (R - 0.0105, 0.167 * s),
            (neck + 0.003, 0.178 * s), (neck, 0.1835 * s), (neck, 0.1855 * s), (neck + 0.0032, 0.1862 * s), (neck + 0.0032, 0.1882 * s),
            (neck, 0.189 * s), (neck, 0.1905 * s), (cap, 0.1908 * s), (cap, 0.2108 * s), (cap - 0.0009, H - 0.0006), (0.0, H)]


def glass_profile(H, R, mouth, cap_h=0.015, shoulder=0.04):
    """A wide-mouth glass bottle (tea, cold brew): straight body, round shoulder, a metal lug cap."""
    b = H - cap_h - 0.004
    return [(0.0, 0.004), (0.016, 0.0035), (R - 0.006, 0.0), (R - 0.0015, 0.0025), (R, 0.009), (R, b - shoulder), (R - 0.0012, b - shoulder * 0.72),
            (R - 0.0045, b - shoulder * 0.45), (mouth + 0.0045, b - shoulder * 0.16), (mouth + 0.001, b - 0.002), (mouth, b + 0.0015),
            (mouth + 0.0012, b + 0.0035), (mouth + 0.0012, H - 0.0012), (mouth + 0.0002, H), (0.0, H)]


def jar_profile(H, R, cap_r, cap_h):
    """A can or jar with a snap-on lid (coffee, soup)."""
    return [(0.0, 0.002), (R - 0.004, 0.0), (R - 0.0005, 0.0025), (R, 0.006), (R, H - cap_h - 0.004), (R - 0.0015, H - cap_h - 0.001),
            (cap_r, H - cap_h), (cap_r + 0.0006, H - cap_h + 0.002), (cap_r + 0.0006, H - 0.0015), (cap_r - 0.0012, H), (0.0, H - 0.0006)]


def pillow(name, w, h, t, seal=0.028, seed=1, nu=14, nv=22, wrinkle=0.003, pinch=None):
    """A pillow pack (chip bag, gummy bag, franks): front and back films puffed between crimped top and bottom seals.
    Base on z=0 (the bottom seal folded under: the bag rests on its lower puff), front toward -Y.

    The film doesn't stretch, so where the bag puffs its sides draw in (pinch: the fraction of the width lost at full
    puff, about a quarter of t / w): the seals stay flat and full width, the body waisted between them, the side
    folds a little wavy."""
    if pinch is None:
        pinch = min(0.12, 0.25 * t / w)
    bm = bmesh.new()
    rings = []
    ts = seal / h
    for j in range(nv + 1):
        tt = j / nv
        # Body fullness: 0 in the seals, rounding up to 1 across the middle.
        if tt <= ts or tt >= 1 - ts:
            g = 0.0
        else:
            q = (tt - ts) / (1 - 2 * ts)
            g = math.sin(math.pi * q) ** 0.55
        z = tt * h
        ring = []
        for side in (-1, 1):
            rng = range(nu + 1) if side < 0 else range(nu, -1, -1)
            for i in rng:
                if side > 0 and i in (0, nu):
                    continue
                s = -1.0 + 2.0 * i / nu
                f = max(0.0, 1.0 - abs(s) ** 2.4) ** 0.5
                th = max(t / 2 * f * g, 0.0006 * (1.0 - abs(s) ** 8))
                x = s * w / 2 * (1.0 - 0.06 * g * (1 - abs(s) ** 2)) * (1.0 - pinch * g)
                # The side folds ripple a little where the film gathers.
                x += s * abs(s) ** 6 * w * 0.012 * g * mnoise.noise(Vector((seed * 0.37, z * 40.0, s * 2.0)))
                n = (mnoise.noise(Vector((x * 28 + seed, z * 28, side * 3.0))) + 0.5 * mnoise.noise(Vector((x * 45 + seed, z * 45, side * 5.0)))) * wrinkle * g * f
                ring.append(bm.verts.new((x, side * (th + n), z)))
        rings.append(ring)
    m = len(rings[0])
    for a, b in zip(rings, rings[1:]):
        for k in range(m):
            bm.faces.new((a[k], a[(k + 1) % m], b[(k + 1) % m], b[k]))
    bm.faces.new(list(reversed(rings[0])))
    bm.faces.new(rings[-1])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    return core.mesh_object(name, bm)


def pouch(name, w, h, t, seal=0.022, nu=12, nv=16, seed=2):
    """A stand-up pouch: a gusseted oval foot it stands on, tapering to a flat sealed top."""
    bm = bmesh.new()
    rings = []
    ts = seal / h
    for j in range(nv + 1):
        tt = j / nv
        g = 0.0 if tt >= 1 - ts else math.cos(0.5 * math.pi * min(1.0, tt / (1 - ts))) ** 0.65
        z = tt * h
        ring = []
        for side in (-1, 1):
            rng = range(nu + 1) if side < 0 else range(nu, -1, -1)
            for i in rng:
                if side > 0 and i in (0, nu):
                    continue
                s = -1.0 + 2.0 * i / nu
                f = max(0.0, 1.0 - abs(s) ** 2.2) ** 0.5
                th = max(t / 2 * f * g, 0.0006 * (1.0 - abs(s) ** 8))
                n = mnoise.noise(Vector((s * 3 + seed, z * 25, side * 3.0))) * 0.0018 * g * f * (1 - g)
                ring.append(bm.verts.new((s * w / 2, side * (th + n), z)))
        rings.append(ring)
    m = len(rings[0])
    for a, b in zip(rings, rings[1:]):
        for k in range(m):
            bm.faces.new((a[k], a[(k + 1) % m], b[(k + 1) % m], b[k]))
    bm.faces.new(list(reversed(rings[0])))
    bm.faces.new(rings[-1])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    return core.mesh_object(name, bm)


def box(name, w, d, h, r=0.0015, z0=0.0, x=0.0, y=0.0, segments=None):
    """A printed carton's box, base at z0: a crisp one-segment bevel on small radii (cheap for many copies), rounder
    on molded bottles."""
    segments = segments or (1 if r < 0.003 else 3)
    return parts.rbox(name, (x, y, z0 + h / 2), (w, d, h), r, segments=segments)


def tray(name, w, d, h, front_h, t=0.0025):
    """An open-topped display carton: its floor, a low front, sides and back."""
    o = [parts.rbox('floor', (0, 0, t / 2), (w, d, t), 0.0006),
         parts.rbox('front', (0, -d / 2 + t / 2, front_h / 2), (w, t, front_h), 0.0006),
         parts.rbox('back', (0, d / 2 - t / 2, h / 2), (w, t, h), 0.0006)]
    for sx in (-1, 1):
        o.append(parts.rbox('side', (sx * (w / 2 - t / 2), 0, h / 2), (t, d - 2 * t, h), 0.0006))
    return core.join(o, name)


# ================================================================== the products

SKUS = {}   # id -> (label art function or None, master builder)


# ------------------------------------------------------------------ cans

CAN_R = 0.0331


def can_sheet(sid, H, body, accent, mono, words, sub, volume, motif=None, mono_ink=None, body_metal=0.08, window=None, word_ink=None):
    """A wrap for a can's printed body (Z 0.0135 up to H - 0.0168): the menu's look, a window band edged in the
    accent with the brand's initials big on the front; the wordmark on the back; small print and a barcode
    on the sides."""
    W = 2 * math.pi * CAN_R * 1000
    Hm = (H - 0.0168 - 0.0135) * 1000
    s = Sheet(W, Hm)
    s.rect(0, 0, W, Hm, body, metal=body_metal, rough=0.28)
    if motif:
        motif(s, W, Hm)
    wy0, wy1 = Hm * 0.3, Hm * 0.74
    s.rect(0, wy0, W, wy1 - wy0, window if window is not None else mix_hex(body, accent, 0.12), metal=body_metal, rough=0.28)
    s.rect(0, wy1, W, Hm * 0.04, accent, metal=0.2, rough=0.26)
    s.rect(0, wy0 - Hm * 0.04, W, Hm * 0.04, accent, metal=0.2, rough=0.26)
    ink = mono_ink if mono_ink is not None else accent
    fx = W * 0.25
    mid(s, mono, fx, (wy0 + wy1) / 2, (wy1 - wy0) * 0.95, W * 0.27, ink, face='Black', tracking=0.98, rough=0.25, metal=0.15)
    # Under the window on the front: the name in full, and the size.
    fit(s, words[0] if len(words) == 1 else ' '.join(words), fx, wy0 - Hm * 0.155, Hm * 0.085, W * 0.3, word_ink if word_ink is not None else ink,
        face='Black', tracking=1.12, rough=0.25)
    fit(s, volume, fx, Hm * 0.035, Hm * 0.045, W * 0.3, word_ink if word_ink is not None else ink, face='Bold', tracking=1.2, rough=0.3)
    fit(s, sub, fx, wy1 + Hm * 0.1, Hm * 0.07, W * 0.3, word_ink if word_ink is not None else ink, face='Bold', tracking=1.3, rough=0.3)
    # Back: the wordmark across the window.
    bx = W * 0.75
    n = len(words)
    lh = (wy1 - wy0) * 0.86 / n
    for k, word in enumerate(words):
        cy = wy1 - (wy1 - wy0) * 0.07 - lh * (k + 0.5)
        mid(s, word, bx, cy, lh * 0.95, W * 0.4, ink, face='Black', tracking=1.0, rough=0.25, metal=0.15)
    fit(s, sub, bx, wy1 + Hm * 0.1, Hm * 0.07, W * 0.42, word_ink if word_ink is not None else ink, face='Bold', tracking=1.3, rough=0.3)
    # Sides: the panel and the barcode.
    nutrition(s, W * 0.5 - 11, Hm * 0.03, 22, wy0 - Hm * 0.1)
    barcode(s, W * 0.985 - 17, Hm * 0.04, 15, 11, seed=sh(sid) & 255)
    s.text('PLEASE RECYCLE  CA CASH REFUND', W * 0.985 - 9.5, Hm * 0.04 + 13, 1.6, word_ink if word_ink is not None else ink, face='Bold', align='CENTER')
    return render(s, sid, W)


def can_material(sid, H, paths, frost_amt=0.7):
    m = core.Mat(f'stock_{sid}')
    tc, sep = object_coords(m)
    z = sep.outputs['Z']
    alu = Surf(0xcfd2d6, 0.2, 1.0)
    z0, z1 = 0.0135, H - 0.0168
    ink, alpha = printed(m, paths, cyl_uv(m, sep, z0, z1))
    lab = m.math('MULTIPLY', band(m, z, z0, z1), alpha)
    s = over(m, lab, alu, ink)
    # The lid: a stay-on tab over the scored mouth, a rivet.
    lid = ramp(m, z, H - 0.0045, H - 0.0035)
    r = radius(m, sep)
    inner = m.math('MULTIPLY', lid, m.math('LESS_THAN', r, 0.0255))
    tab = m.math('MULTIPLY', inner, rrect_mask(m, sep, 0.0, -0.0045, 0.0068, 0.0115, 0.0045, width=0.0002))
    hole = m.math('MULTIPLY', inner, rrect_mask(m, sep, 0.0, -0.0055, 0.0042, 0.0045, 0.0035, width=0.0002))
    tab = m.math('MULTIPLY', tab, m.math('SUBTRACT', 1.0, hole))
    mouth = rrect_mask(m, sep, 0.0, 0.0128, 0.0085, 0.0078, 0.006, width=0.0002)
    mouth_in = rrect_mask(m, sep, 0.0, 0.0128, 0.0077, 0.007, 0.0055, width=0.0002)
    score = m.math('MULTIPLY', inner, m.math('SUBTRACT', mouth, mouth_in))
    s = over(m, m.math('MULTIPLY', inner, 0.3), s, Surf(0xb9bcc1, 0.26, 1.0))
    s = over(m, tab, s, Surf(0xd9dbdf, 0.18, 1.0))
    s = over(m, hole, s, Surf(0x3a3c40, 0.5, 0.6))
    s = over(m, score, s, Surf(0x6a6d72, 0.4, 1.0))
    height = m.math('ADD', m.math('MULTIPLY', tab, 1.0), m.math('MULTIPLY', score, -0.6))
    # Cold from the cooler.
    s, beads = frost(m, tc, s, frost_amt, mask=m.math('SUBTRACT', 1.0, lid))
    height = add(m, height, beads)
    return finish_mat(m, s, height, strength=0.25, distance=0.0003)


def can_sku(sid, H, **art):
    def make(paths):
        return lathe_master(sid, can_profile(H), 24, can_material(sid, H, paths))
    SKUS[sid] = (lambda: can_sheet(sid, H, **art), make)


CAN12, CAN16 = 0.1222, 0.1570


def fizz_motif(s, W, H):
    rnd = random.Random(4)
    for k in range(70):
        x, y = rnd.uniform(0, W), rnd.uniform(0, H)
        r = rnd.uniform(0.8, 3.2)
        s.ring(x, y, r * 0.7, r, 0xe8424a, n=24)


def volt_motif(s, W, H):
    for k in range(-20, 70):
        x = k * 4.2
        s.poly([(x, 0), (x + 0.5, 0), (x + 0.5 + H * 0.55, H), (x + H * 0.55, H)], 0x22262d, metal=0.15, rough=0.3)
    for fx in (0.08, 0.42, 0.58, 0.92):
        bolt(s, W * fx, H * 0.52, H * 0.42, 0x2b3a12)


def blaze_motif(s, W, H):
    rnd = random.Random(9)
    for k in range(12):
        x = k * W / 12 + rnd.uniform(-4, 4)
        hgt = H * rnd.uniform(0.35, 0.6)
        s.poly([(x - 9, 0), (x + 9, 0), (x + 4, hgt * 0.6), (x + 7, hgt * 0.75), (x, hgt), (x - 6, hgt * 0.7), (x - 3, hgt * 0.55)], 0xff9a2e, rough=0.3)


def mill_motif(s, W, H):
    for cx in (W * 0.5, W * 0.0, W * 1.0):
        s.ring(cx, H * 0.52, H * 0.16, H * 0.2, 0x6e3c24, n=48)
        for k in range(8):
            a = k * math.pi / 4
            s.poly([(cx + math.cos(a) * H * 0.02 - math.sin(a) * 1.0, H * 0.52 + math.sin(a) * H * 0.02 + math.cos(a) * 1.0),
                    (cx + math.cos(a) * H * 0.2 - math.sin(a) * 1.0, H * 0.52 + math.sin(a) * H * 0.2 + math.cos(a) * 1.0),
                    (cx + math.cos(a) * H * 0.2 + math.sin(a) * 1.0, H * 0.52 + math.sin(a) * H * 0.2 - math.cos(a) * 1.0),
                    (cx + math.cos(a) * H * 0.02 + math.sin(a) * 1.0, H * 0.52 + math.sin(a) * H * 0.02 - math.cos(a) * 1.0)], 0x6e3c24)


can_sku('fizz_can', CAN12, body=0xd8262f, accent=0xffffff, mono='FC', words=['FIZZ', 'COLA'], sub='CLASSIC', volume='12 FL OZ (355 mL)', motif=fizz_motif)
can_sku('diet_fizz_can', CAN12, body=0xc9ccd1, accent=0xd8262f, mono='FC', words=['DIET', 'FIZZ'], sub='ZERO CALORIES', volume='12 FL OZ (355 mL)',
        body_metal=0.75, word_ink=0x1b1b1d)
can_sku('oldmill_can', CAN12, body=0x5a2e1b, accent=0xf1e3c4, mono='OM', words=['OLD', 'MILL'], sub='ROOT BEER', volume='12 FL OZ (355 mL)', motif=mill_motif)
can_sku('volt_can', CAN16, body=0x16181d, accent=0xb6ff2e, mono='VR', words=['VOLT', 'RUSH'], sub='ENERGY', volume='16 FL OZ (473 mL)', motif=volt_motif,
        word_ink=0xf2f5ee)
can_sku('volt_zero_can', CAN16, body=0xeef0ec, accent=0x16181d, mono='VR', words=['VOLT', 'RUSH'], sub='ZERO SUGAR', volume='16 FL OZ (473 mL)',
        window=0xb6ff2e, body_metal=0.1)
can_sku('blaze_can', CAN16, body=0xf26a1b, accent=0x141416, mono='BZ', words=['BLAZE'], sub='MANGO HEAT', volume='16 FL OZ (473 mL)', motif=blaze_motif,
        word_ink=0x141416)


# ------------------------------------------------------------------ bottles

def band_sheet(sid, Hb, R, bg, accent, mono, words, sub, volume, motif=None, mono_ink=None, word_ink=None, front_art=None, stripe=True,
               alpha=1.0, wrap=1.0):
    """A bottle's label band, Hb tall (meters): the menu's label, lighter than the drink, an accent stripe at its
    foot and the initials big on the front; the wordmark on the back, small print on the sides."""
    W = 2 * math.pi * R * 1000 * wrap
    H = Hb * 1000
    s = Sheet(W, H)
    s.rect(0, 0, W, H, bg, rough=0.32, alpha=alpha)
    if motif:
        motif(s, W, H)
    if stripe:
        s.rect(0, 0, W, H * 0.11, accent, rough=0.3)
        s.rect(0, H * 0.94, W, H * 0.03, accent, rough=0.3)
    ink = mono_ink if mono_ink is not None else accent
    wink = word_ink if word_ink is not None else ink
    fx = W * 0.25
    mid(s, mono, fx, H * 0.56, H * 0.62, W * 0.27, ink, face='Black', tracking=0.98, rough=0.28)
    fit(s, ' '.join(words), fx, H * 0.15, H * 0.1, W * 0.3, wink, face='Black', tracking=1.15, rough=0.3)
    if front_art:
        front_art(s, W, H)
    bx = W * 0.75
    n = len(words)
    lh = H * 0.5 / n
    for k, word in enumerate(words):
        mid(s, word, bx, H * 0.8 - lh * (k + 0.5), lh * 0.92, W * 0.4, ink, face='Black', rough=0.28)
    fit(s, sub, bx, H * 0.2, H * 0.085, W * 0.38, wink, face='Bold', tracking=1.25, rough=0.3)
    fit(s, volume, fx, H * 0.88, H * 0.06, W * 0.3, wink, face='Bold', tracking=1.2, rough=0.3)
    nutrition(s, W * 0.5 - 10, H * 0.16, 20, H * 0.7)
    barcode(s, W * 0.985 - 16, H * 0.2, 14, min(10, H * 0.4), seed=sh(sid) & 255)
    return render(s, sid, W)


def bottle_material(sid, paths, profile, label, liquid, cap, fill, liquid_rough=0.05, cap_metal=0.0, glass=False, frost_amt=0.8, cap_from=None):
    """A drink bottle: the drink seen through PET or glass (no transmission in the bake: it reads as the drink's own
    color under a gloss), the empty neck above its fill line, the label band, the cap with its knurl."""
    m = core.Mat(f'stock_{sid}')
    tc, sep = object_coords(m)
    z = sep.outputs['Z']
    H = profile[-1][1]
    cap_z = cap_from if cap_from is not None else next(zz for (rr, zz), (r2, z2) in zip(profile, profile[1:]) if z2 > H * 0.85 and r2 > rr + 0.0007)
    body = Surf(liquid, liquid_rough, 0.0)
    air = Surf(0x9aa4a8 if not glass else mix_hex(liquid, 0x202020, 0.4), 0.04, 0.0)
    s = over(m, ramp(m, z, fill - 0.001, fill + 0.001), body, air)
    # A highlight band where the liquid meets the PET wall: the meniscus.
    men = m.math('MULTIPLY', ramp(m, z, fill - 0.0025, fill - 0.0005), m.math('SUBTRACT', 1.0, ramp(m, z, fill - 0.0005, fill + 0.0005)))
    s = over(m, m.math('MULTIPLY', men, 0.6), s, Surf(mix_hex(liquid, 0xffffff, 0.5), 0.03, 0.0))
    z0, z1 = label
    ink, alpha = printed(m, paths, cyl_uv(m, sep, z0, z1))
    lab = m.math('MULTIPLY', band(m, z, z0, z1), alpha)
    s = over(m, lab, s, ink)
    capm = ramp(m, z, cap_z - 0.0003, cap_z + 0.0003)
    s = over(m, capm, s, Surf(cap, 0.32 if not cap_metal else 0.28, cap_metal))
    height = m.math('MULTIPLY', knurl(m, sep, 90 if not cap_metal else 40, m.math('MULTIPLY', capm, m.math('LESS_THAN', z, H - 0.0025))), 0.6)
    s, beads = frost(m, tc, s, frost_amt, mask=m.math('SUBTRACT', 1.0, capm))
    height = add(m, height, beads)
    if glass:
        m.set('Specular IOR Level', 0.6)
    return finish_mat(m, s, height, strength=0.25, distance=0.0003)


def bottle_sku(sid, profile, label, liquid, cap, fill, art, segs=24, **kw):
    R = max(r for r, z in profile)  # labels wrap the widest part

    def make(paths):
        return lathe_master(sid, profile, segs, bottle_material(sid, paths, profile, label, liquid, cap, fill, **kw))
    SKUS[sid] = (lambda: band_sheet(sid, label[1] - label[0], R, **art), make)


PET20 = pet_profile()
PET_LABEL = (0.073, 0.131)


def cascade_motif(s, W, H):
    for fx in (0.0, 0.5, 1.0):
        peaks(s, W * fx - 26, H * 0.11, 52, H * 0.5, 0xd7ebf7, snow=0xffffff)


def cascade_front(s, W, H):
    drop(s, W * 0.25 + 24, H * 0.62, 14, 0x8fd3ff)


def stamina_motif(s, W, H):
    for k in range(5):
        y = H * (0.2 + k * 0.14)
        s.poly([(0, y), (W, y + H * 0.2), (W, y + H * 0.24), (0, y + H * 0.04)], 0x1f5fb0, rough=0.3)


def spark_motif(s, W, H):
    rnd = random.Random(12)
    for k in range(40):
        star(s, rnd.uniform(0, W), rnd.uniform(H * 0.12, H * 0.92), 0.4, rnd.uniform(1.2, 2.6), 0x8ad65a, points=4, rot=rnd.uniform(0, 1))


def sun_motif(s, W, H):
    for fx in (0.0, 0.5, 1.0):
        for k in range(16):
            a = k * math.pi / 8
            s.poly([(W * fx, H * 0.55), (W * fx + 60 * math.cos(a), H * 0.55 + 60 * math.sin(a)), (W * fx + 60 * math.cos(a + 0.12), H * 0.55 + 60 * math.sin(a + 0.12))],
                   0xff9a3a, rough=0.3)


bottle_sku('cascade', PET20, PET_LABEL, liquid=0x7f9cad, cap=0x1d6fb8, fill=0.17,
           art=dict(bg=0xffffff, accent=0x1d6fb8, mono='C', words=['CASCADE'], sub='SPRING WATER', volume='20 FL OZ (591 mL)', motif=cascade_motif,
                    front_art=cascade_front))
bottle_sku('fizz_20', PET20, PET_LABEL, liquid=0x2a130b, cap=0xd8262f, fill=0.172,
           art=dict(bg=0xd8262f, accent=0xffffff, mono='FC', words=['FIZZ', 'COLA'], sub='CLASSIC', volume='20 FL OZ (591 mL)', motif=fizz_motif))
bottle_sku('diet_fizz_20', PET20, PET_LABEL, liquid=0x2a130b, cap=0xbfc3c8, fill=0.172,
           art=dict(bg=0xd9dce0, accent=0xd8262f, mono='FC', words=['DIET', 'FIZZ'], sub='ZERO CALORIES', volume='20 FL OZ (591 mL)', word_ink=0x1b1b1d))
bottle_sku('lime_spark_20', PET20, PET_LABEL, liquid=0x93b06a, cap=0x2fa84f, fill=0.171,
           art=dict(bg=0x2fa84f, accent=0xf4f1d0, mono='LS', words=['LIME', 'SPARK'], sub='LEMON-LIME SODA', volume='20 FL OZ (591 mL)', motif=spark_motif))
bottle_sku('sun_pop_20', PET20, PET_LABEL, liquid=0xf28a1a, cap=0xf28a1a, fill=0.171,
           art=dict(bg=0xff7a1a, accent=0xfff3d6, mono='SP', words=['SUN', 'POP'], sub='ORANGE SODA', volume='20 FL OZ (591 mL)', motif=sun_motif,
                    mono_ink=0xffffff))
bottle_sku('stamina_20', PET20, PET_LABEL, liquid=0x2f8ff0, cap=0xf2f4f6, fill=0.168,
           art=dict(bg=0x10203a, accent=0x5ad1ff, mono='S', words=['STAMINA'], sub='GLACIER FREEZE', volume='20 FL OZ (591 mL)', motif=stamina_motif,
                    word_ink=0xffffff))

TEA = glass_profile(0.19, 0.0345, 0.0205, cap_h=0.017, shoulder=0.045)
TEA_LABEL = (0.022, 0.118)


def sunny_front(s, W, H):
    peach(s, W * 0.25 - 30, H * 0.78, 7.5)


def lemon_front(s, W, H):
    lemon(s, W * 0.25 - 30, H * 0.78, 7.0)


def sunny_motif(s, W, H):
    for fx in (0.0, 0.5, 1.0):
        for k in range(12):
            a = k * math.pi / 6
            s.poly([(W * fx, H * 0.5), (W * fx + 80 * math.cos(a), H * 0.5 + 80 * math.sin(a)), (W * fx + 80 * math.cos(a + 0.18), H * 0.5 + 80 * math.sin(a + 0.18))],
                   0xffe8d2, rough=0.3)


bottle_sku('sunny_peach', TEA, TEA_LABEL, liquid=0xe8823a, cap=0xd2452b, fill=0.152, glass=True, liquid_rough=0.03,
           art=dict(bg=shade(0xffb36b, 0.55), accent=0xd2452b, mono='SP', words=['SUNNY', 'PEACH'], sub='ICED TEA', volume='18 FL OZ (532 mL)',
                    motif=sunny_motif, front_art=sunny_front))
bottle_sku('sunny_lemon', TEA, TEA_LABEL, liquid=0xd99a2a, cap=0xd2452b, fill=0.152, glass=True, liquid_rough=0.03,
           art=dict(bg=shade(0xf2d53c, 0.6), accent=0xd2452b, mono='SL', words=['SUNNY', 'LEMON'], sub='ICED TEA', volume='18 FL OZ (532 mL)',
                    motif=sunny_motif, front_art=lemon_front))

OWL = glass_profile(0.162, 0.031, 0.0195, cap_h=0.015, shoulder=0.04)
OWL_LABEL = (0.02, 0.101)


def owl_motif(s, W, H):
    for k in range(60):
        x = k * W / 60
        s.rect(x, 0, 0.35, H, 0x2e1d14)


def owl_front(s, W, H):
    owl(s, W * 0.25 + 27, H * 0.74, 13, 0xf2c14e)


bottle_sku('night_owl', OWL, OWL_LABEL, liquid=0x170b05, cap=0xf2c14e, fill=0.13, glass=True, liquid_rough=0.03, cap_metal=1.0,
           art=dict(bg=0x24160f, accent=0xf2c14e, mono='NO', words=['NIGHT', 'OWL'], sub='COLD BREW COFFEE', volume='11 FL OZ (325 mL)',
                    motif=owl_motif, front_art=owl_front))

MILK = [(0.0, 0.004), (0.02, 0.003), (0.029, 0.0), (0.0315, 0.006), (0.032, 0.02), (0.032, 0.105), (0.031, 0.118), (0.027, 0.132), (0.021, 0.143),
        (0.0165, 0.15), (0.0165, 0.153), (0.019, 0.1535), (0.019, 0.155), (0.0175, 0.1555), (0.0185, 0.156), (0.0185, 0.172), (0.0175, 0.173), (0.0, 0.1732)]
MILK_LABEL = (0.012, 0.14)


def milk_motif(s, W, H):
    rnd = random.Random(31)
    for k in range(14):
        cx, cy = rnd.uniform(0, W), rnd.uniform(H * 0.45, H * 0.95)
        r = rnd.uniform(4, 9)
        s.poly([(cx + r * (1 + 0.3 * math.sin(3 * t)) * math.cos(t), cy + r * 0.8 * (1 + 0.3 * math.cos(2 * t)) * math.sin(t)) for t in [2 * math.pi * i / 24 for i in range(24)]],
               0x3b2214, rough=0.35)


bottle_sku('hillside_choc', MILK, MILK_LABEL, liquid=0xeeeae2, cap=0x6b3a1e, fill=0.2, liquid_rough=0.35, frost_amt=0.6, cap_from=0.1555,
           art=dict(bg=0xf3eee4, accent=0x6b3a1e, mono='HC', words=['HILLSIDE'], sub='CHOCOLATE MILK', volume='14 FL OZ (414 mL)', motif=milk_motif,
                    stripe=True))


# ------------------------------------------------------------------ bags

def bag_sheet(sid, w, h, body, label, name, flavor, flavor_bg, flavor_ink, art=None, tag_line='KETTLE COOKED', weight='NET WT 8 OZ (226g)',
              name_ink=None, seal=0.028, circle=True, burst=None):
    """A chip bag's two faces (millimeters, front | back): the menu's bag, its name up top in the label color, the
    label's disc below with a heap of chips spilling over it, rays behind, a notched flavor ribbon; the back's panel
    and barcode."""
    W, H = w * 1000, h * 1000
    s = Sheet(2 * W, H)
    s.rect(0, 0, 2 * W, H, body, rough=0.28)
    seal_mm = seal * 1000
    cx = W / 2
    for k in range(18):
        a = k * math.pi / 9
        s.poly([(cx, H * 0.42), (cx + W * math.cos(a), H * 0.42 + W * math.sin(a)), (cx + W * math.cos(a + 0.17), H * 0.42 + W * math.sin(a + 0.17))],
               shade(body, 0.1), rough=0.28)
    s.rect(W, 0, W, H, body, rough=0.28)  # the rays stop at the bag's edge
    for x0 in (0, W):
        # Shaded toward the foot, the way bag printers fake depth.
        for k in range(8):
            s.rect(x0, 0, W, H * (0.42 - k * 0.05), 0x000000, rough=0.28, alpha=0.035)
    for x0 in (0, W):
        if art:
            art(s, x0, W, H)
        # Crimped seals: darker, ribbed.
        for y0 in (0, H - seal_mm):
            s.rect(x0, y0, W, seal_mm, shade(body, -0.18), rough=0.3)
            for k in range(int(seal_mm / 1.6)):
                s.rect(x0, y0 + k * 1.6, W, 0.5, shade(body, -0.3), rough=0.3)
    ink = name_ink if name_ink is not None else label
    if circle:
        s.circle(cx, H * 0.42, W * 0.31, shade(label, -0.12), n=96, rough=0.3)
        s.circle(cx, H * 0.42, W * 0.29, label, n=96, rough=0.3)
        chips_pile(s, cx, H * 0.4, W * 0.34, shadow=shade(label, -0.45), seed=sh(sid) & 63)
    fit(s, tag_line, cx, H * 0.8, H * 0.04, W * 0.6, ink, face='Bold', tracking=1.35, rough=0.3)
    mid(s, name, cx, H * 0.7, H * 0.13, W * 0.86, ink, face='Black', tracking=1.0, rough=0.25)
    ry, rh = H * 0.12, H * 0.075
    s.poly([(W * 0.03, ry), (W * 0.97, ry), (W * 0.93, ry + rh / 2), (W * 0.97, ry + rh), (W * 0.03, ry + rh), (W * 0.07, ry + rh / 2)], flavor_bg, rough=0.3)
    mid(s, flavor, cx, ry + rh / 2, H * 0.05, W * 0.78, flavor_ink, face='Black', tracking=1.1, rough=0.3)
    fit(s, weight, W * 0.1, H * 0.08, H * 0.022, W * 0.5, ink, face='Bold', align='LEFT', rough=0.3)
    if burst:
        star(s, W * 0.82, H * 0.6, W * 0.1, W * 0.125, 0xffd23f, points=16)
        mid(s, burst[0], W * 0.82, H * 0.615, W * 0.045, W * 0.16, 0xc8202c)
        mid(s, burst[1], W * 0.82, H * 0.578, W * 0.03, W * 0.16, 0xc8202c, face='Bold')
    # The back.
    bx = W
    mid(s, name, bx + W / 2, H * 0.8, H * 0.07, W * 0.6, ink, face='Black', rough=0.3)
    nutrition(s, bx + W * 0.08, H * 0.3, W * 0.38, H * 0.4)
    text_lines(s, bx + W * 0.52, H * 0.42, W * 0.4, H * 0.28, shade(ink, -0.1) if ink != label else label, rows=9, seed=sh(sid) & 15)
    barcode(s, bx + W * 0.55, H * 0.2, W * 0.3, H * 0.09, seed=sh(sid) & 255)
    # The fin seal down the middle of the back.
    s.rect(bx + W / 2 - 4, seal_mm, 8, H - 2 * seal_mm, shade(body, -0.12), rough=0.3)
    return render(s, sid, 2 * W)


def film_material(sid, paths, w, h, gloss=0.26, metal=0.08, wrinkle=1.0, seal=0.028, uvf=None, frost_amt=0.0, under=0xd9d9d9):
    """Printed film: the art over a metallized film, the crimps ribbed, creased all over."""
    m = core.Mat(f'stock_{sid}')
    tc, sep = object_coords(m)
    uv = uvf(m, sep) if uvf else two_face_uv(m, sep, w, h)
    ink, alpha = printed(m, paths, uv)
    s = over(m, alpha, Surf(under, 0.25, 0.8), Surf(ink.color, m.math('ADD', m.math('MULTIPLY', ink.rough, 0.35), gloss * 0.45), metal))
    z = sep.outputs['Z']
    seals = m.math('MAXIMUM', m.math('LESS_THAN', z, seal), m.math('GREATER_THAN', z, h - seal))
    ribs = m.math('MULTIPLY', m.math('ABSOLUTE', m.math('SINE', m.math('MULTIPLY', z, 2 * math.pi / 0.0032))), seals)
    height = add(m, crinkle(m, tc, sep, wrinkle), m.math('MULTIPLY', ribs, 1.5))
    if frost_amt:
        s, beads = frost(m, tc, s, frost_amt)
        height = add(m, height, beads)
    return finish_mat(m, s, height, strength=0.3, distance=0.0007)


def bag_sku(sid, w, h, t, seal=0.028, density=1400.0, frost_amt=0.0, gloss=0.26, **art):
    def make(paths):
        obj = pillow(f'stock_{sid}', w, h, t, seal=seal, seed=sh(sid) % 50)
        mat = film_material(sid, paths, w, h, seal=seal, frost_amt=frost_amt, gloss=gloss)
        return finish_master(obj, mat, tile_for_area(obj, density), sharp=70.0)
    SKUS[sid] = (lambda: bag_sheet(sid, w, h, seal=seal, **art), make)


def hill_art(color):
    def art(s, x0, W, H):
        hills(s, x0, H * 0.06, W, H * 0.24, color, seed=3)
        hills(s, x0, H * 0.06, W, H * 0.14, shade(color, -0.15), seed=7, waves=2)
    return art


def curls_art(s, x0, W, H):
    """A heap of cheese curls on a cream burst: each a fat, knobbly crescent, cheese-orange with a dusty light side
    and a shadow under it."""
    cx, cy = x0 + W / 2, H * 0.38
    star(s, cx, cy + H * 0.02, W * 0.28, W * 0.36, 0xffe2a8, points=18, rot=0.1)
    s.circle(cx, cy + H * 0.02, W * 0.28, 0xfff1d0, n=96, rough=0.3)
    rnd = random.Random(14 + int(x0))
    pieces = []
    for k in range(26):
        u = rnd.uniform(-1.0, 1.0)
        v = rnd.uniform(0.0, 1.0) ** 1.25
        pieces.append((cy - W * 0.2 + v * W * 0.34, cx + u * W * 0.29 * (1.0 - 0.5 * v)))
    for y, x in sorted(pieces, reverse=True):
        r = W * rnd.uniform(0.05, 0.068)
        th = r * rnd.uniform(0.62, 0.78)
        a0 = rnd.uniform(0, 2 * math.pi)
        span = rnd.uniform(2.6, 4.2)
        s.ring(x + r * 0.08, y - r * 0.12, r - th * 0.5, r + th * 0.5, 0x8a3a10, n=28, a0=a0, a1=a0 + span, alpha=0.5)
        s.ring(x, y, r - th * 0.5, r + th * 0.5, 0xe8781a, n=28, a0=a0, a1=a0 + span)
        s.ring(x - r * 0.04, y + r * 0.05, r - th * 0.15, r + th * 0.3, 0xf7a43a, n=28, a0=a0 + 0.2, a1=a0 + span - 0.3)
        for q in range(4):
            a = a0 + span * rnd.uniform(0.1, 0.9)
            s.circle(x + r * math.cos(a), y + r * math.sin(a), th * 0.12, 0xffd27a, n=8)


def pretzel_art(s, x0, W, H):
    for k in range(6):
        cx, cy = x0 + W * (0.2 + 0.3 * (k % 3)), H * (0.28 + 0.16 * (k // 3))
        for sx in (-1, 1):
            s.ring(cx + sx * 6, cy, 4.5, 7.5, 0x9a5a22, n=28)
        s.ring(cx, cy - 5, 8, 11, 0x9a5a22, n=28, a0=math.pi * 1.1, a1=math.pi * 1.9)


bag_sku('hilltop_sv', 0.22, 0.30, 0.075, burst=('2 FOR', '$4'), body=0x2e9e5b, label=0xf5f0e1, name='Hilltop', flavor='SALT & VINEGAR', flavor_bg=0xf5f0e1, flavor_ink=0x1f6e3e,
        art=hill_art(0x3fb06c))
bag_sku('hilltop_og', 0.22, 0.30, 0.075, body=0xe8b923, label=0xfff6dd, name='Hilltop', flavor='ORIGINAL SEA SALT', flavor_bg=0x2a5ba8, flavor_ink=0xffffff,
        art=hill_art(0xf2cb4a), name_ink=0x7a3a10)
bag_sku('hilltop_bbq', 0.22, 0.30, 0.075, body=0x8e2b1e, label=0xf5e6c8, name='Hilltop', flavor='SMOKY BBQ', flavor_bg=0xf2c14e, flavor_ink=0x5a1a10,
        art=hill_art(0xa83a28))
bag_sku('curls', 0.21, 0.29, 0.075, body=0xf47b20, label=0x4a2a8a, name='CRUNCH CURLS', flavor='FLAMIN\' CHEDDAR', flavor_bg=0x4a2a8a, flavor_ink=0xffd23f,
        art=curls_art, tag_line='BAKED CORN PUFFS', circle=False)
bag_sku('pretzels', 0.20, 0.28, 0.075, body=0x1f3a6e, label=0xffffff, name='TWIST & CO.', flavor='SOURDOUGH PRETZELS', flavor_bg=0xf2c14e,
        flavor_ink=0x1f3a6e, art=pretzel_art, tag_line='BAKED SINCE 1952', circle=False, weight='NET WT 10 OZ (283g)')
bag_sku('gummy', 0.13, 0.18, 0.04, seal=0.02, body=0x7a3fc0, label=0xffd23f, name='GUMMY GRINS', flavor='SOUR BEARS', flavor_bg=0xff5ba8,
        flavor_ink=0xffffff, tag_line='FRUIT SNACKS', circle=False, weight='NET WT 5 OZ (141g)',
        art=lambda s, x0, W, H: [s.circle(x0 + W * (0.2 + 0.15 * (k % 5)), H * (0.32 + 0.1 * (k // 5)), 7, [0xff4040, 0x40c060, 0xffd23f, 0xff8a1a, 0x5aa0ff][k % 5], n=24) for k in range(15)])


# ------------------------------------------------------------------ pouches

def pouch_sheet(sid, w, h, body, band_c, name, sub, weight, motif=None, name_ink=0xf5f0e1):
    """Ridgeline's pouch: the menu's band across the top with the name, the window of what's inside below."""
    W, H = w * 1000, h * 1000
    s = Sheet(2 * W, H)
    s.rect(0, 0, 2 * W, H, body, rough=0.45)
    for x0 in (0, W):
        s.rect(x0, H * 0.9, W, H * 0.1, shade(body, -0.2), rough=0.45)
        s.rect(x0, H * 0.86, W, 0.8, shade(body, -0.35), rough=0.4)  # the zipper
        if motif:
            motif(s, x0, W, H)
    s.rect(0, H * 0.58, W, H * 0.22, band_c, rough=0.4)
    mid(s, name, W / 2, H * 0.69, H * 0.1, W * 0.86, name_ink, face='Black', tracking=1.02, rough=0.35)
    fit(s, sub, W / 2, H * 0.6, H * 0.035, W * 0.8, name_ink, face='Bold', tracking=1.3, rough=0.4)
    nut_window(s, W * 0.17, H * 0.14, W * 0.66, H * 0.38, seed=sh(sid) & 31)
    fit(s, weight, W / 2, H * 0.06, H * 0.028, W * 0.6, name_ink, face='Bold', tracking=1.2, rough=0.4)
    mid(s, name, W * 1.5, H * 0.78, H * 0.06, W * 0.6, name_ink, face='Black', rough=0.4)
    nutrition(s, W * 1.08, H * 0.25, W * 0.4, H * 0.42)
    barcode(s, W * 1.55, H * 0.15, W * 0.32, H * 0.08, seed=sh(sid) & 255)
    text_lines(s, W * 1.54, H * 0.35, W * 0.38, H * 0.3, shade(body, 0.5), rows=8)
    return render(s, sid, 2 * W)


def ridge_motif(color):
    def art(s, x0, W, H):
        peaks(s, x0, H * 0.8, W, H * 0.09, color)
    return art


def pouch_sku(sid, w, h, t, **art):
    def make(paths):
        obj = pouch(f'stock_{sid}', w, h, t, seed=sh(sid) % 40)
        return finish_master(obj, film_material(sid, paths, w, h, gloss=0.5, metal=0.0, wrinkle=0.6, seal=0.0), tile_for_area(obj, 1500.0), sharp=70.0)
    SKUS[sid] = (lambda: pouch_sheet(sid, w, h, **art), make)


pouch_sku('ridgeline', 0.15, 0.22, 0.065, body=0x8a6a3a, band_c=0x2c4a2e, name='RIDGELINE', sub='TRAIL MIX · NUTS & RAISINS', weight='NET WT 6 OZ (170g)',
          motif=ridge_motif(0x6f5430))
pouch_sku('ridgeline_jerky', 0.14, 0.2, 0.04, body=0x1c1c1e, band_c=0xa8322a, name='RIDGELINE', sub='ORIGINAL BEEF JERKY', weight='NET WT 2.85 OZ (81g)',
          motif=ridge_motif(0x2e2e30))


# ------------------------------------------------------------------ cartons

def carton_material(sid, paths, w, d, h, rough_add=0.0, wear=0.4, frost_amt=0.0, z0=0.0, film=None, extras=None, emboss=None, dust_amt=0.0):
    """A printed paperboard carton (or a film-wrapped bundle when film is set): the net's art on each face, the
    print rubbed off on the corners. extras: {part tag: finish} for molded parts (caps, necks) that aren't printed.
    dust_amt: shelf dust on its tops (the slow movers)."""
    m = core.Mat(f'stock_{sid}')
    tc, sep = object_coords(m)
    ink, alpha = printed(m, paths, net_uv(m, sep, w, d, h, z0))
    s = Surf(ink.color, m.math('ADD', ink.rough, rough_add), ink.metal)
    if wear:
        s = edge_wear(m, tc, s, wear)
    height = None
    if film is not None:
        height = crinkle(m, tc, sep, film)
        s = Surf(s.color, mix_float(m, 0.6, s.rough, 0.18), s.metal)
    if emboss:
        # Paper towels' and tissue's embossed diamonds, through the film.
        across = m.math('ADD', sep.outputs['X'], sep.outputs['Y'])
        z = sep.outputs['Z']
        k = math.pi / emboss
        a = m.math('ABSOLUTE', m.math('SINE', m.math('MULTIPLY', m.math('ADD', across, z), k)))
        b = m.math('ABSOLUTE', m.math('SINE', m.math('MULTIPLY', m.math('SUBTRACT', across, z), k)))
        dots = ramp(m, m.math('MULTIPLY', a, b), 0.55, 0.85)
        s = Surf(m.mix(m.math('MULTIPLY', dots, 0.08), s.color, (0.45, 0.45, 0.48, 1.0)), s.rough, s.metal)
        height = add(m, height, m.math('MULTIPLY', dots, 0.8))
    if extras:
        attr = m.node('ShaderNodeAttribute')
        attr.attribute_type = 'GEOMETRY'
        attr.attribute_name = 'part'
        for k, f in extras.items():
            s = over(m, part_mask(m, k, attr), s, f)
    if frost_amt:
        s, beads = frost(m, tc, s, frost_amt)
        height = add(m, height, beads)
    if dust_amt:
        s = dust(m, tc, s, dust_amt)
    return finish_mat(m, s, height, strength=0.15, distance=0.0005)


def carton_sku(sid, w, d, h, art, r=0.0015, density=DENSITY, film=None, wear=0.4, shape=None, extras=None, emboss=None, dust_amt=0.0):
    """A printed box (or any shape printed like one, by its faces' directions). extras: [(part builder, finish)]."""
    def make(paths):
        obj = shape(f'stock_{sid}') if shape else box(f'stock_{sid}', w, d, h, r)
        fins = {}
        if extras:
            tag(obj, 0)
            objs = [obj]
            for k, (build_part, finish) in enumerate(extras, start=1):
                objs.append(tag(build_part(), k))
                fins[k] = finish
            obj = core.join(objs, f'stock_{sid}')
        mat = carton_material(sid, paths, w, d, h, wear=wear, film=film, extras=fins, emboss=emboss, dust_amt=dust_amt)
        return finish_master(obj, mat, tile_for_area(obj, density), sharp=40.0)

    def sheet():
        R = net_rects(w, d, h)
        s = Sheet(R['W'], R['H'])
        art(s, R)
        return render(s, sid, R['W'])
    SKUS[sid] = (sheet, make)


def rolls(name, nx, ny, r, h, bevel=0.012, n=72):
    """Rolls standing in a film wrap (paper towels, bath tissue): the outline of nx by ny circles, the film bridging
    the valleys between them a little."""
    centers = [((i - (nx - 1) / 2) * 2 * r * 0.97, (j - (ny - 1) / 2) * 2 * r * 0.97) for i in range(nx) for j in range(ny)]
    rs = []
    for k in range(n):
        a = 2 * math.pi * k / n
        dx, dy = math.cos(a), math.sin(a)
        best = 0.0
        for cx, cy in centers:
            b = dx * cx + dy * cy
            c = cx * cx + cy * cy - r * r
            disc = b * b - c
            if disc >= 0:
                best = max(best, b + math.sqrt(disc))
        rs.append(best)
    smooth = [max(rs[k], 0.5 * (rs[k - 3] + rs[(k + 3) % n]) * 0.995) for k in range(n)]
    pts = [(smooth[k] * math.cos(2 * math.pi * k / n), smooth[k] * math.sin(2 * math.pi * k / n)) for k in range(n)]
    return parts.panel(name, pts, 0.0, h, bevel, segments=2)


def fill(s, R, color, rough=0.45):
    s.rect(0, 0, R['W'], R['H'], color, rough=rough)


def panel_text(s, rect, text, fy, size_f, color, face='Black', wf=0.88, tracking=1.0):
    x, y, w, h = rect
    return mid(s, text, x + w / 2, y + h * fy, h * size_f, w * wf, color, face=face, tracking=tracking)


def cereal_art(s, R):
    fill(s, R, 0xf2c14e)
    x, y, w, h = R['front']
    s.rect(x, y + h * 0.78, w, h * 0.22, 0xd8262f, rough=0.4)
    panel_text(s, R['front'], 'CRUNCHY', 0.92, 0.07, 0xffffff)
    panel_text(s, R['front'], "O'S", 0.83, 0.1, 0xffffff)
    s.circle(x + w / 2, y + h * 0.38, w * 0.36, 0xffffff, n=64)
    s.circle(x + w / 2, y + h * 0.38, w * 0.3, 0x3a6fd8, n=64)
    rnd = random.Random(2)
    for k in range(26):
        cx, cy = x + w / 2 + rnd.uniform(-w * 0.24, w * 0.24), y + h * 0.4 + rnd.uniform(-w * 0.1, w * 0.14)
        s.ring(cx, cy, 2.2, 5.2, 0xe8a83a, n=20)
    s.rect(x + w * 0.06, y + h * 0.04, w * 0.5, h * 0.06, 0x3a6fd8)
    panel_text(s, (x + w * 0.06, y + h * 0.04, w * 0.5, h * 0.06), 'WHOLE GRAIN OATS', 0.5, 0.45, 0xffffff, face='Bold')
    for side in ('right', 'left'):
        nutrition(s, R[side][0] + 4, R[side][1] + R[side][3] * 0.2, R[side][2] - 8, R[side][3] * 0.5)
        panel_text(s, R[side], "CRUNCHY O'S", 0.85, 0.05, 0xd8262f)
    bx, by, bw, bh = R['back']
    s.rect(bx + bw * 0.1, by + bh * 0.3, bw * 0.8, bh * 0.5, 0xfff3c8)
    panel_text(s, R['back'], 'FIND THE PENNY!', 0.72, 0.06, 0xd8262f)
    text_lines(s, bx + bw * 0.16, by + bh * 0.35, bw * 0.68, bh * 0.28, 0x8a6a2a, rows=8)
    barcode(s, bx + bw * 0.6, by + bh * 0.05, bw * 0.3, bh * 0.08, seed=21)
    tx, ty, tw, th = R['top']
    panel_text(s, R['top'], "CRUNCHY O'S", 0.5, 0.4, 0xd8262f)


def crackers_art(s, R):
    fill(s, R, 0xc8202c)
    x, y, w, h = R['front']
    s.rect(x + w * 0.08, y + h * 0.15, w * 0.84, h * 0.48, 0xf6eeda)
    for k in range(6):
        cx, cy = x + w * (0.27 + 0.23 * (k % 3)), y + h * (0.28 + 0.2 * (k // 3))
        s.rect(cx - 12, cy - 12, 24, 24, 0xe8c37a)
        for hx in (-6, 0, 6):
            for hy in (-6, 0, 6):
                s.circle(cx + hx, cy + hy, 0.9, 0xb98a3a, n=8)
    panel_text(s, R['front'], 'SALTINE', 0.84, 0.1, 0xffffff)
    panel_text(s, R['front'], 'SQUARES', 0.73, 0.07, 0xf6eeda, face='Bold', tracking=1.3)
    panel_text(s, R['front'], 'NET WT 16 OZ (453g)', 0.06, 0.03, 0xffffff, face='Bold')
    for side in ('right', 'left'):
        nutrition(s, R[side][0] + 4, R[side][1] + R[side][3] * 0.2, R[side][2] - 8, R[side][3] * 0.5)
    panel_text(s, R['back'], 'SALTINE SQUARES', 0.8, 0.06, 0xffffff)
    barcode(s, R['back'][0] + R['back'][2] * 0.6, R['back'][1] + 10, R['back'][2] * 0.3, 14, seed=22)
    panel_text(s, R['top'], 'SALTINE SQUARES', 0.5, 0.4, 0xffffff)


def cocoa_art(s, R):
    fill(s, R, 0x9b2c2c)
    x, y, w, h = R['front']
    penny_logo(s, x + w * 0.2, y + h * 0.84, h * 0.08)
    fit(s, "PENNY'S", x + w * 0.35, y + h * 0.82, h * 0.09, w * 0.6, 0xf5f1e8, face='Black', align='LEFT')
    panel_text(s, R['front'], 'HOT COCOA', 0.66, 0.12, 0xf5f1e8)
    s.poly([(x + w * 0.28, y + h * 0.12), (x + w * 0.72, y + h * 0.12), (x + w * 0.76, y + h * 0.48), (x + w * 0.24, y + h * 0.48)], 0xf5f1e8)
    s.ring(x + w * 0.8, y + h * 0.3, h * 0.06, h * 0.1, 0xf5f1e8, n=32, a0=-math.pi / 2, a1=math.pi / 2)
    s.rect(x + w * 0.26, y + h * 0.43, w * 0.48, h * 0.04, 0x5b2a17)
    for k in range(3):
        s.circle(x + w * (0.38 + 0.12 * k), y + h * 0.49, h * 0.035, 0xffffff, n=24)
    panel_text(s, R['front'], '10 PACKETS · RICH MILK CHOCOLATE', 0.06, 0.035, 0xf5f1e8, face='Bold')
    for side in ('right', 'left', 'back'):
        panel_text(s, R[side], "PENNY'S HOT COCOA", 0.85, 0.06, 0xf5f1e8)
    nutrition(s, R['back'][0] + 6, R['back'][1] + 10, R['back'][2] * 0.45, R['back'][3] * 0.5)
    barcode(s, R['back'][0] + R['back'][2] * 0.6, R['back'][1] + 10, R['back'][2] * 0.32, 14, seed=23)
    panel_text(s, R['top'], "PENNY'S HOT COCOA", 0.5, 0.4, 0xf5f1e8)


def fizz12_art(s, R):
    fill(s, R, 0xd8262f)
    for face in ('front', 'back'):
        x, y, w, h = R[face]
        s.rect(x, y + h * 0.28, w, h * 0.44, mix_hex(0xd8262f, 0xffffff, 0.12))
        s.rect(x, y + h * 0.72, w, h * 0.04, 0xffffff)
        s.rect(x, y + h * 0.24, w, h * 0.04, 0xffffff)
        mid(s, 'FC', x + w * 0.2, y + h * 0.5, h * 0.36, w * 0.3, 0xffffff)
        mid(s, 'FIZZ COLA', x + w * 0.63, y + h * 0.53, h * 0.16, w * 0.6, 0xffffff)
        mid(s, '12 PACK · 12 FL OZ CANS', x + w * 0.63, y + h * 0.37, h * 0.06, w * 0.6, 0xffffff, face='Bold', tracking=1.2)
        mid(s, 'FRIDGE PACK', x + w * 0.5, y + h * 0.12, h * 0.06, w * 0.6, 0xffffff, face='Bold', tracking=1.5)
    for face in ('right', 'left'):
        x, y, w, h = R[face]
        mid(s, 'FC', x + w / 2, y + h * 0.55, h * 0.3, w * 0.7, 0xffffff)
        s.rect(x + w * 0.2, y + h * 0.15, w * 0.6, h * 0.18, 0xf6f2e8)
        mid(s, 'PULL HERE', x + w / 2, y + h * 0.24, h * 0.06, w * 0.5, 0xd8262f, face='Bold')
    tx, ty, tw, th = R['top']
    mid(s, 'FIZZ COLA', tx + tw / 2, ty + th / 2, th * 0.5, tw * 0.7, 0xffffff)
    barcode(s, R['bottom'][0] + 10, R['bottom'][1] + 10, 40, 16, seed=24)


def oil_art(s, R):
    fill(s, R, 0x141416, rough=0.35)
    x, y, w, h = R['front']
    s.rect(x, y + h * 0.18, w, h * 0.5, 0xf2c014, rough=0.35)
    s.poly([(x, y + h * 0.18), (x + w, y + h * 0.3), (x + w, y + h * 0.18)], 0x141416, rough=0.35)
    panel_text(s, R['front'], 'TORQUE', 0.58, 0.075, 0x141416)
    panel_text(s, R['front'], 'MAX', 0.47, 0.085, 0xd8262f)
    panel_text(s, R['front'], '5W-30', 0.36, 0.06, 0x141416)
    panel_text(s, R['front'], 'FULL SYNTHETIC BLEND · 1 QT', 0.1, 0.028, 0xf2c014, face='Bold')
    for side in ('right', 'left', 'back'):
        panel_text(s, R[side], 'TORQUEMAX', 0.6, 0.05, 0xf2c014)
        text_lines(s, R[side][0] + 4, R[side][1] + R[side][3] * 0.2, R[side][2] - 8, R[side][3] * 0.3, 0x6a6a6a, rows=8)


def roll_tops(s, rect, nx, ny):
    """The rolls' ends under the film: tissue wound round a brown core."""
    x, y, w, h = rect
    r = min(w / nx, h / ny) / 2
    for i in range(nx):
        for j in range(ny):
            cx, cy = x + (i + 0.5) * w / nx, y + (j + 0.5) * h / ny
            s.circle(cx, cy, r * 0.96, 0xf6f6f2, n=48, rough=0.5)
            for q in range(4):
                s.ring(cx, cy, r * (0.4 + q * 0.14), r * (0.4 + q * 0.14) + 0.4, 0xe2e2dc, n=48)
            s.circle(cx, cy, r * 0.36, 0xa9794a, n=40, rough=0.6)
            s.circle(cx, cy, r * 0.31, 0x3a2a1c, n=40, rough=0.7)


def towel_art(s, R):
    fill(s, R, 0xf4f6f8, rough=0.3)
    for face in ('front', 'back', 'right', 'left'):
        x, y, w, h = R[face]
        s.rect(x, y + h * 0.6, w, h * 0.24, 0x1e7fd0, rough=0.3)
        s.rect(x, y + h * 0.58, w, h * 0.015, 0x8fc6f0, rough=0.3)
    for face in ('front', 'back'):
        x, y, w, h = R[face]
        mid(s, 'SOAK-UP', x + w / 2, y + h * 0.73, h * 0.12, w * 0.85, 0xffffff)
        mid(s, '2 BIG ROLLS · PAPER TOWELS', x + w / 2, y + h * 0.64, h * 0.035, w * 0.85, 0xd8ecfb, face='Bold')
        s.rect(x + w * 0.62, y + h * 0.1, w * 0.3, h * 0.12, 0xffd23f, rough=0.3)
        mid(s, '= 5 REG', x + w * 0.77, y + h * 0.16, h * 0.05, w * 0.26, 0x1e5fa8)
    roll_tops(s, R['top'], 2, 1)


def tp_art(s, R):
    fill(s, R, 0xfbf7f2, rough=0.3)
    for face in ('front', 'back', 'right', 'left'):
        x, y, w, h = R[face]
        s.rect(x, y + h * 0.48, w, h * 0.4, 0x8a5fc8, rough=0.3)
        mid(s, 'CLOUD SOFT', x + w / 2, y + h * 0.7, h * 0.15, w * 0.9, 0xffffff)
        mid(s, '4 DOUBLE ROLLS', x + w / 2, y + h * 0.3, h * 0.08, w * 0.85, 0x8a5fc8, face='Bold')
    roll_tops(s, R['top'], 2, 2)


def cells_art(s, R):
    fill(s, R, 0x111214, rough=0.3)
    x, y, w, h = R['front']
    s.rect(x, y + h * 0.66, w, h * 0.2, 0xf28c28, rough=0.3)
    panel_text(s, R['front'], 'POWERCELL', 0.76, 0.09, 0x111214)
    for k in range(4):
        cx = x + w * (0.2 + 0.2 * k)
        s.rect(cx - 5, y + h * 0.18, 10, h * 0.4, 0xd9dbe0, metal=0.7, rough=0.3)
        s.rect(cx - 5, y + h * 0.48, 10, h * 0.1, 0xf28c28, rough=0.3)
        s.rect(cx - 2, y + h * 0.58, 4, h * 0.02, 0xbfc2c6, metal=0.8)
    panel_text(s, R['front'], 'AA · 4 PACK', 0.08, 0.06, 0xf28c28, face='Bold')


def soap_art(s, R):
    fill(s, R, 0x1aa6b7, rough=0.25)
    x, y, w, h = R['front']
    s.circle(x + w / 2, y + h * 0.45, w * 0.36, 0xffffff, n=64, rough=0.25)
    panel_text(s, R['front'], 'BRITE', 0.47, 0.12, 0x1aa6b7)
    panel_text(s, R['front'], 'LAUNDRY DETERGENT', 0.33, 0.035, 0x0f6a75, face='Bold')
    panel_text(s, R['front'], '50 FL OZ · 32 LOADS', 0.08, 0.035, 0xffffff, face='Bold')
    for side in ('right', 'left', 'back'):
        panel_text(s, R[side], 'BRITE', 0.6, 0.1, 0xffffff)


def case_art(s, R):
    """Cascade's 24-pack: the shrink-wrapped bottles drawn under the film's print."""
    fill(s, R, 0xbcd6e4, rough=0.15)
    for face in ('front', 'back', 'right', 'left'):
        x, y, w, h = R[face]
        n = 6 if face in ('front', 'back') else 4
        bw = w / n
        for k in range(n):
            bx = x + k * bw
            s.rect(bx + bw * 0.06, y + h * 0.02, bw * 0.88, h * 0.98, 0xa9c9d9, rough=0.1)
            s.rect(bx + bw * 0.2, y + h * 0.02, bw * 0.12, h * 0.98, 0xd9ecf5, rough=0.08)
            s.rect(bx + bw * 0.06, y + h * 0.32, bw * 0.88, h * 0.3, 0xffffff, rough=0.3)
            s.rect(bx + bw * 0.06, y + h * 0.32, bw * 0.88, h * 0.035, 0x1d6fb8)
            mid(s, 'C', bx + bw / 2, y + h * 0.48, h * 0.16, bw * 0.6, 0x1d6fb8)
        s.rect(x, y + h * 0.7, w, h * 0.13, 0x1d6fb8, rough=0.2)
        mid(s, 'CASCADE · 24 PACK · SPRING WATER', x + w / 2, y + h * 0.765, h * 0.06, w * 0.9, 0xffffff, face='Black', tracking=1.15)
    x, y, w, h = R['top']
    for i in range(6):
        for j in range(4):
            s.circle(x + w * (i + 0.5) / 6, y + h * (j + 0.5) / 4, min(w / 6, h / 4) * 0.25, 0x1d6fb8, n=32)


carton_sku('cereal', 0.19, 0.065, 0.28, cereal_art, density=1500, dust_amt=0.2)
carton_sku('crackers', 0.13, 0.11, 0.21, crackers_art, density=1500, dust_amt=0.2)
carton_sku('cocoa', 0.12, 0.065, 0.17, cocoa_art, dust_amt=0.25)
carton_sku('fizz_12pk', 0.395, 0.13, 0.245, fizz12_art, density=1200, dust_amt=0.22)
carton_sku('cells', 0.09, 0.03, 0.14, cells_art, dust_amt=0.15)
carton_sku('oil', 0.105, 0.065, 0.2, oil_art, r=0.012, dust_amt=0.4, extras=[(lambda: parts.rod('neck', (0.026, 0, 0.19), (0.026, 0, 0.215), 0.015, 20), Surf(0x141416, 0.35)),
                                                                (lambda: parts.rod('cap', (0.026, 0, 0.213), (0.026, 0, 0.236), 0.0175, 24), Surf(0xf2c014, 0.35))])
carton_sku('detergent', 0.16, 0.09, 0.22, soap_art, r=0.018, density=1300, dust_amt=0.2,
           extras=[(lambda: parts.rod('spout', (0.045, 0, 0.21), (0.045, 0, 0.226), 0.02, 24), Surf(0x1aa6b7, 0.25)),
                   (lambda: parts.rod('cap', (0.045, 0, 0.224), (0.045, 0, 0.248), 0.024, 28), Surf(0xf4f4f0, 0.35))])
carton_sku('towels', 0.24, 0.12, 0.28, towel_art, film=0.6, wear=0.0, density=1100, emboss=0.008, shape=lambda n: rolls(n, 2, 1, 0.06, 0.28))
carton_sku('tp', 0.22, 0.22, 0.115, tp_art, film=0.6, wear=0.0, density=1100, emboss=0.007, shape=lambda n: rolls(n, 2, 2, 0.055, 0.115, bevel=0.01))
carton_sku('cascade_case', 0.40, 0.26, 0.215, case_art, density=1000, wear=0.0, film=0.35, dust_amt=0.22)


# ------------------------------------------------------------------ cans and cups on the grocery shelf

def label_wrap_sheet(sid, R, Hl, bg, accent, mono, words, sub, extra=None, ink=0xffffff):
    W, H = 2 * math.pi * R * 1000, Hl * 1000
    s = Sheet(W, H)
    s.rect(0, 0, W, H, bg, rough=0.5)
    if extra:
        extra(s, W, H)
    fx = W * 0.25
    mid(s, mono, fx, H * 0.6, H * 0.42, W * 0.27, ink, face='Black')
    fit(s, words, fx, H * 0.24, H * 0.12, W * 0.3, ink, face='Black', tracking=1.05)
    fit(s, sub, fx, H * 0.1, H * 0.07, W * 0.3, accent, face='Bold', tracking=1.2)
    mid(s, words, W * 0.75, H * 0.6, H * 0.16, W * 0.42, ink, face='Black')
    fit(s, sub, W * 0.75, H * 0.35, H * 0.07, W * 0.42, accent, face='Bold', tracking=1.2)
    nutrition(s, W * 0.5 - 12, H * 0.12, 24, H * 0.76)
    barcode(s, W * 0.985 - 17, H * 0.2, 15, min(11, H * 0.3), seed=sh(sid) & 255)
    return render(s, sid, W)


def tin_material(sid, paths, profile, label, cap_z, cap, cap_metal=1.0, body_metal=0xc8ccd0, cap_rough=0.3, paper=0.5, dust_amt=0.0):
    m = core.Mat(f'stock_{sid}')
    tc, sep = object_coords(m)
    z = sep.outputs['Z']
    s = Surf(body_metal, 0.28, 1.0)
    ink, alpha = printed(m, paths, cyl_uv(m, sep, *label))
    s = over(m, m.math('MULTIPLY', band(m, z, *label), alpha), s, Surf(ink.color, m.math('ADD', m.math('MULTIPLY', ink.rough, 0.5), paper * 0.5), 0.0))
    capm = ramp(m, z, cap_z - 0.0003, cap_z + 0.0003)
    s = over(m, capm, s, Surf(cap, cap_rough, cap_metal))
    # Steel cans' beads.
    beads = m.math('MULTIPLY', m.math('ABSOLUTE', m.math('SINE', m.math('MULTIPLY', z, 2 * math.pi / 0.012))), m.math('SUBTRACT', 1.0, band(m, z, *label)))
    s = edge_wear(m, tc, s, 0.25, 0xb8bcc0)
    if dust_amt:
        s = dust(m, tc, s, dust_amt)
    return finish_mat(m, s, beads, strength=0.1)


def soup_extra(s, W, H):
    s.rect(0, 0, W, H * 0.42, 0xf6f2ea, rough=0.5)
    for fx in (0.25, 0.75):
        s.circle(W * fx, H * 0.2, H * 0.17, 0xf2d28a, n=48)
        for k in range(7):
            s.rect(W * fx - H * 0.12 + k * H * 0.04, H * 0.18, H * 0.025, H * 0.01, 0xd9b24a)


def coffee_extra(s, W, H):
    for k in range(-40, 120):
        x = k * 3.0
        s.poly([(x, 0), (x + 0.6, 0), (x + 0.6 + H * 0.4, H), (x + H * 0.4, H)], 0x5e3218, rough=0.5)
    penny_logo(s, W * 0.25, H * 0.86, H * 0.08)
    penny_logo(s, W * 0.75, H * 0.86, H * 0.08)


SOUP = jar_profile(0.102, 0.0335, 0.031, 0.004)
SKUS['soup'] = (lambda: label_wrap_sheet('soup', 0.0335, 0.084, 0xc8202c, 0xffd23f, 'HP', 'HEARTY POT', 'CHICKEN NOODLE', extra=soup_extra),
                lambda paths: lathe_master('soup', SOUP, 24, tin_material('soup', paths, SOUP, (0.009, 0.093), 0.098, 0xc8ccd0, dust_amt=0.22)))
COFFEE = jar_profile(0.15, 0.05, 0.051, 0.012)
SKUS['drip_coffee'] = (lambda: label_wrap_sheet('drip_coffee', 0.05, 0.128, 0x6b3a1e, 0xd08a4e, 'PD', "PENNY'S DRIP", 'HOUSE BLEND · GROUND', extra=coffee_extra,
                                                ink=0xf5f1e8),
                       lambda paths: lathe_master('drip_coffee', COFFEE, 32, tin_material('drip_coffee', paths, COFFEE, (0.006, 0.134), 0.137, 0x2a1a12,
                                                                                           cap_metal=0.0, cap_rough=0.4, dust_amt=0.12)))

CUP = [(0.0, 0.002), (0.028, 0.001), (0.0335, 0.0), (0.0345, 0.003), (0.046, 0.08), (0.048, 0.0815), (0.048, 0.0835), (0.0472, 0.0845), (0.0, 0.0852)]


def oodle_sheet():
    W, H = 2 * math.pi * 0.046 * 1000, 85.0
    s = Sheet(W, H)
    s.rect(0, 0, W, H, 0xffffff, rough=0.4)
    s.rect(0, H * 0.3, W, H * 0.36, 0xe0322e, rough=0.35)
    for fx in (0.25, 0.75):
        mid(s, 'OODLE', W * fx, H * 0.48, H * 0.28, W * 0.38, 0xffffff)
        fit(s, 'NOODLE CUP · CHICKEN', W * fx, H * 0.18, H * 0.075, W * 0.4, 0xe0322e, face='Black', tracking=1.1)
        for k in range(5):
            s.ring(W * fx - 30 + k * 15, H * 0.82, 3, 5, 0xf2c14e, n=20, a0=0, a1=4.5)
    fit(s, 'NET WT 2.25 OZ (64g) · ADD BOILING WATER', W * 0.5, H * 0.08, H * 0.04, W * 0.3, 0x333333, face='Bold')
    barcode(s, W * 0.5 - 8, H * 0.7, 16, 11, seed=41)
    return render(s, 'oodle', W)


def oodle_material(paths):
    m = core.Mat('stock_oodle')
    tc, sep = object_coords(m)
    z = sep.outputs['Z']
    # The cup's printed wall: its v follows the slanted side.
    ink, alpha = printed(m, paths, cyl_uv(m, sep, 0.0, 0.085))
    paper = Surf(ink.color, m.math('ADD', ink.rough, 0.1), 0.0)
    lid = ramp(m, z, 0.0838, 0.0842)
    r = radius(m, sep)
    # The foil lid: silver, its print a red ring and the flavor.
    ring_m = m.math('MULTIPLY', smooth_less(m, m.math('ABSOLUTE', m.math('SUBTRACT', r, 0.036)), 0.006, 0.0006), lid)
    foil = Surf(0xd9dce0, m.math('ADD', 0.22, m.math('MULTIPLY', noise(m, tc.outputs['Object'], scale=300.0, detail=3.0), 0.12)), 1.0)
    s = over(m, lid, paper, foil)
    s = over(m, ring_m, s, Surf(0xe0322e, 0.3, 0.3))
    mark = m.math('MULTIPLY', lid, rrect_mask(m, sep, 0.0, 0.0, 0.016, 0.006, 0.004, width=0.0003))
    s = over(m, mark, s, Surf(0xe0322e, 0.3, 0.2))
    height = m.math('MULTIPLY', noise(m, scaled(m, sep, 400.0, 400.0, 1.0), detail=2.0), m.math('MULTIPLY', lid, 0.6))
    return finish_mat(m, s, height, strength=0.15)


SKUS['oodle'] = (oodle_sheet, lambda paths: lathe_master('oodle', CUP, 32, oodle_material(paths)))


# ------------------------------------------------------------------ deli

def wedge_shape(name, w, d, h):
    """A sandwich wedge's clamshell: a right-triangle prism lying along X, the slope (the cut, through the film)
    facing front and up."""
    pts = [(-d / 2, 0.0), (d / 2, 0.0), (d / 2, h)]
    # Panel outline is in X(=Y here), Y(=Z here); extrude along the product's width.
    obj = parts.panel(name, [(y, z) for y, z in pts], -w / 2, w / 2, 0.006, segments=2)
    obj.data.transform(Matrix(((0, 0, 1, 0), (1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 0, 1))))
    core.orient_normals(obj)
    return obj


WEDGE = (0.072, 0.11, 0.125)  # a sandwich wedge's clamshell: width, the triangle's foot, its back


def wedge_sheet():
    """Two triangle halves back to back behind the clamshell's window: the cut face shows crust, bread, egg salad
    and a frill of lettuce, twice; the triangle sides show the bread's face; a card on the back, the date sticker."""
    w, d, h = WEDGE
    R = net_rects(w, d, h)
    s = Sheet(R['W'], R['H'])
    s.rect(0, 0, R['W'], R['H'], 0xe9eef2, rough=0.1)
    fx, fy, fw, fh = R['front']
    rnd = random.Random(12)
    half = fw / 2
    for k in range(2):
        x = fx + k * half
        bands = [(2.2, 0xc58a4a), (9.5, 0xf3e8d0), (11.0, 0xf2d77e), (9.5, 0xf3e8d0), (2.2, 0xc58a4a)]
        if k:
            bands = bands[::-1]
        xx = x + (half - sum(b for b, _ in bands)) / 2
        for bw, col in bands:
            s.rect(xx, fy, bw, fh, col, rough=0.1)
            if col == 0xf2d77e:
                for q in range(70):
                    s.circle(xx + rnd.uniform(0.6, bw - 0.6), fy + rnd.uniform(1, fh - 1), rnd.uniform(0.3, 0.9),
                             rnd.choice((0xf8e9a6, 0xe8c45a, 0xfaf3d6, 0x6b5a3a)), n=8, rough=0.1)
                lx = xx + (0.8 if k == 0 else bw - 3.0)
                pts = [(lx + 1.1 * math.sin(t * 0.9) + (0 if i % 2 else 0.6), fy + t) for i, t in enumerate([j * fh / 40 for j in range(41)])]
                s.poly(pts + [(p[0] + 2.2, p[1]) for p in reversed(pts)], 0x6f9a32, rough=0.1)
            xx += bw
    # The date sticker over the window's top.
    s.rect(fx + 5, fy + fh * 0.66, fw - 10, fh * 0.22, 0xffffff, rough=0.45)
    s.rect(fx + 5, fy + fh * 0.81, fw - 10, fh * 0.07, 0x5f8a2e, rough=0.45)
    mid(s, 'DELI WEDGE', fx + fw / 2, fy + fh * 0.845, fh * 0.05, fw - 16, 0xffffff)
    mid(s, 'EGG SALAD', fx + fw / 2, fy + fh * 0.765, fh * 0.055, fw - 16, 0x2b2723)
    mid(s, 'TODAY  $4.49', fx + fw / 2, fy + fh * 0.69, fh * 0.04, fw - 16, 0xc8202c, face='Black')
    # The sides: the bread's face through the film, its crust along the edges; the brand on the back half.
    for side, back_right in (('right', True), ('left', False)):
        x, y, ww, hh = R[side]
        s.rect(x, y, ww, hh, 0xc58a4a, rough=0.1)
        s.rect(x + 2.5, y + 2.5, ww - 5, hh - 5, 0xf3e8d0, rough=0.1)
        lx = x + (ww * 0.5 if back_right else 0)
        s.rect(lx, y + hh * 0.12, ww * 0.5, hh * 0.3, 0x5f8a2e, rough=0.45)
        mid(s, 'DELI', lx + ww * 0.25, y + hh * 0.32, hh * 0.08, ww * 0.42, 0xffffff)
        mid(s, 'WEDGE', lx + ww * 0.25, y + hh * 0.21, hh * 0.08, ww * 0.42, 0xffffff)
    # The card on the back.
    bx, by, bw, bh = R['back']
    s.rect(bx, by, bw, bh, 0xf6f0de, rough=0.5)
    s.rect(bx, by + bh * 0.62, bw, bh * 0.2, 0x5f8a2e, rough=0.45)
    mid(s, 'DELI WEDGE', bx + bw / 2, by + bh * 0.72, bh * 0.08, bw * 0.85, 0xffffff)
    mid(s, 'EGG SALAD ON WHITE', bx + bw / 2, by + bh * 0.52, bh * 0.05, bw * 0.85, 0x3a3a2a, face='Bold')
    nutrition(s, bx + bw * 0.12, by + bh * 0.08, bw * 0.76, bh * 0.36)
    return render(s, 'deli_wedge', R['W'])


def deli_wedge_make(paths):
    w, d, h = WEDGE
    obj = wedge_shape('stock_deli_wedge', w, d, h)
    m = core.Mat('stock_deli_wedge')
    tc, sep = object_coords(m)
    # Faces pick the net by their normals: the slope (normal -Y +Z) maps as the front, by height.
    ink, alpha = printed(m, paths, net_uv(m, sep, w, d, h))
    # The clamshell: a gloss over everything that isn't paper.
    s = Surf(ink.color, m.math('ADD', m.math('MULTIPLY', ink.rough, 0.4), 0.03), 0.0)
    s, beads = frost(m, tc, s, 0.45)
    return finish_master(obj, finish_mat(m, s, beads, strength=0.2), tile_for_area(obj), sharp=35.0)


SKUS['deli_wedge'] = (wedge_sheet, deli_wedge_make)

BURRITO = [(0.0, 0.0), (0.006, 0.001), (0.011, 0.0035), (0.0115, 0.008), (0.022, 0.012), (0.027, 0.02), (0.028, 0.03), (0.028, 0.14), (0.027, 0.15),
           (0.022, 0.158), (0.0115, 0.162), (0.011, 0.1665), (0.006, 0.169), (0.0, 0.17)]


BURRITO_L = 0.17
# The angle round the burrito's own axis (as modeled, standing) that ends up facing front and up once burrito_matrix
# lays it down in the row: the middle of its printed face, where a shopper's eye lands.
BURRITO_FACE = 0.75 * math.pi


def burrito_sheet():
    """The film's print, laid out the way it's read: the burrito lies on its side, so the name runs along its length
    (the sheet's x) and the sheet's y goes round its girth, the face a shopper sees in the middle (BURRITO_FACE); the
    barcode and the small print on the side that rests on the shelf."""
    L, G = BURRITO_L * 1000, 2 * math.pi * 0.028 * 1000
    s = Sheet(L, G)
    s.rect(0, 0, L, G, 0xe9dcc0, rough=0.25)
    s.rect(L * 0.17, 0, L * 0.66, G, 0xb5462e, rough=0.25)
    cy = G * 0.5
    for dy in (-27.0, 23.0):
        s.rect(L * 0.17, cy + dy, L * 0.66, 1.6, 0xffd23f, rough=0.25)
    mid(s, 'BIG BEAN', L / 2, cy + 6.0, 17.0, L * 0.6, 0xf6ead2)
    fit(s, 'BEAN & CHEESE', L / 2, cy - 11.0, 7.0, L * 0.56, 0xffd23f, face='Black', tracking=1.15)
    fit(s, 'MICROWAVE 90 SEC · 5 OZ', L / 2, cy - 19.5, 4.2, L * 0.56, 0xf6ead2, face='Bold', tracking=1.15)
    # Round the back: the name small, the panel and barcode.
    mid(s, 'BIG BEAN', L / 2, G * 0.06, 9.0, L * 0.5, 0xf6ead2)
    nutrition(s, L * 0.2, G * 0.82, 30, 26)
    barcode(s, L * 0.58, G * 0.84, 26, 14, seed=43)
    return render(s, 'big_bean', L)


def burrito_make(paths):
    m = core.Mat('stock_big_bean')
    tc, sep = object_coords(m)
    # u along the burrito, v round it: up the print is away from the shopper over the top (angle decreasing).
    u = m.math('DIVIDE', sep.outputs['Z'], BURRITO_L)
    ang = m.math('ARCTAN2', sep.outputs['Y'], sep.outputs['X'])
    v = m.math('FRACT', m.math('ADD', m.math('DIVIDE', m.math('SUBTRACT', BURRITO_FACE, ang), 2 * math.pi), 0.5))
    ink, alpha = printed(m, paths, vec(m, u, v, 0.0))
    s = Surf(ink.color, ink.rough, 0.15)
    z = sep.outputs['Z']
    ends = m.math('MAXIMUM', m.math('LESS_THAN', z, 0.012), m.math('GREATER_THAN', z, 0.158))
    height = add(m, crinkle(m, tc, sep, 0.7), m.math('MULTIPLY', m.math('ABSOLUTE', m.math('SINE', m.math('MULTIPLY', m.math('ARCTAN2', sep.outputs['Y'], sep.outputs['X']), 9.0))), ends))
    s, beads = frost(m, tc, s, 0.6)
    return lathe_master('big_bean', BURRITO, 20, finish_mat(m, s, add(m, height, beads), strength=0.2))


SKUS['big_bean'] = (burrito_sheet, burrito_make)


bag_sku('franks', 0.2, 0.13, 0.03, seal=0.015, body=0xc8553d, label=0xf2c14e, name='ROLLER DOG', flavor='8 ALL-BEEF FRANKS', flavor_bg=0xf2c14e,
        flavor_ink=0x8a2a1a, tag_line='THE ROLLER GRILL ORIGINAL', circle=False, weight='NET WT 15 OZ (425g)',
        art=lambda s, x0, W, H: [s.rect(x0 + W * 0.12, H * (0.32 + 0.07 * k), W * 0.76, H * 0.05, 0xa8432a) for k in range(3)])


def ice_art(s, x0, W, H):
    """Cubes of ice crowding the film."""
    rnd = random.Random(int(x0) + 3)
    for k in range(70):
        cx, cy = x0 + rnd.uniform(0, W), rnd.uniform(H * 0.06, H * 0.94)
        r = rnd.uniform(9, 16)
        s.poly(core.rounded_rect(cx - r, cy - r, cx + r, cy + r, r * 0.35, steps=3), rnd.choice((0xc9dcea, 0xd7e6f1, 0xbcd3e6)), rough=0.2)
        s.poly(core.rounded_rect(cx - r * 0.6, cy - r * 0.1, cx + r * 0.2, cy + r * 0.7, r * 0.2, steps=3), 0xf2f8fc, rough=0.15)


bag_sku('ice', 0.28, 0.42, 0.1, seal=0.03, density=900.0, frost_amt=1.0, gloss=0.2, body=0xd4e3ee, label=0x1e5fa8, name='ICE',
        flavor='NORTHSIDE ICE CO. · 7 LB', flavor_bg=0x1e5fa8, flavor_ink=0xffffff, tag_line='PURIFIED · PARTY SIZE', circle=False,
        weight='NET WT 7 LB (3.18 kg)', art=ice_art)


# ------------------------------------------------------------------ candy and the counter

def bar_sheet():
    w, d, h = 0.15, 0.06, 0.012
    R = net_rects(w, d, h)
    s = Sheet(R['W'], R['H'])
    s.rect(0, 0, R['W'], R['H'], 0x5b2a17, rough=0.22)
    for face in ('top', 'bottom'):
        x, y, ww, hh = R[face]
        s.poly([(x + ww * 0.32, y), (x + ww * 0.5, y), (x + ww * 0.38, y + hh), (x + ww * 0.2, y + hh)], 0xf2c14e, rough=0.2)
        mid(s, 'CHOCO STACK', x + ww * 0.7, y + hh * 0.55, hh * 0.32, ww * 0.5, 0xffffff)
        fit(s, 'PEANUT · CARAMEL', x + ww * 0.7, y + hh * 0.18, hh * 0.12, ww * 0.45, 0xf2c14e, face='Bold', tracking=1.2)
    return render(s, 'choco_stack', R['W'])


def bar_make(paths):
    w, d, h = 0.15, 0.06, 0.012
    obj = box('stock_choco_stack', w, d, h, 0.004)
    m = core.Mat('stock_choco_stack')
    tc, sep = object_coords(m)
    # The bar lies face up: the net's top is its front.
    ink, alpha = printed(m, paths, net_uv(m, sep, w, d, h))
    x = sep.outputs['X']
    crimp = m.math('GREATER_THAN', m.math('ABSOLUTE', x), w / 2 - 0.009)
    height = add(m, crinkle(m, tc, sep, 0.5), m.math('MULTIPLY', m.math('ABSOLUTE', m.math('SINE', m.math('MULTIPLY', sep.outputs['Y'], 2 * math.pi / 0.002))), crimp))
    return finish_master(obj, finish_mat(m, Surf(ink.color, ink.rough, 0.25), height, strength=0.2), tile_for_area(obj), sharp=40.0)


SKUS['choco_stack'] = (bar_sheet, bar_make)


def caddy_art(s, R):
    """Choco Stack's counter caddy: a carton with its lid folded back into a header."""
    fill(s, R, 0x5b2a17)
    for face in ('front', 'back', 'right', 'left'):
        x, y, w, h = R[face]
        s.rect(x, y + h * 0.15, w, h * 0.12, 0xf2c14e)
    x, y, w, h = R['front']
    mid(s, 'CHOCO STACK', x + w / 2, y + h * 0.6, h * 0.3, w * 0.9, 0xffffff)
    mid(s, '$1.49', x + w / 2, y + h * 0.21, h * 0.1, w * 0.4, 0x5b2a17)


carton_sku('caddy', 0.17, 0.2, 0.045, caddy_art, r=0.001, density=1300, shape=lambda n: tray(n, 0.17, 0.2, 0.045, 0.035))


def header_art(s, R):
    fill(s, R, 0x5b2a17)
    x, y, w, h = R['front']
    s.poly([(x, y), (x + w * 0.3, y), (x + w * 0.15, y + h), (x - w * 0.15, y + h)], 0xf2c14e)
    mid(s, 'CHOCO', x + w * 0.62, y + h * 0.66, h * 0.28, w * 0.6, 0xffffff)
    mid(s, 'STACK', x + w * 0.62, y + h * 0.3, h * 0.28, w * 0.6, 0xffffff)
    x, y, w, h = R['back']
    mid(s, 'CHOCO STACK', x + w / 2, y + h / 2, h * 0.3, w * 0.9, 0xf2c14e)


carton_sku('caddy_header', 0.17, 0.004, 0.09, header_art, r=0.001)


def gum_art(color, flavor):
    def art(s, R):
        fill(s, R, color, rough=0.25)
        x, y, w, h = R['front']
        s.rect(x, y + h * 0.1, w, h * 0.22, 0xffffff, rough=0.25)
        mid(s, 'CHEW', x + w / 2, y + h * 0.62, h * 0.34, w * 0.9, 0xffffff)
        mid(s, flavor, x + w / 2, y + h * 0.21, h * 0.12, w * 0.85, color, face='Bold')
        x, y, w, h = R['top']
        mid(s, 'CHEW', x + w / 2, y + h / 2, h * 0.5, w * 0.8, 0xffffff)
    return art


carton_sku('gum_mint', 0.075, 0.02, 0.05, gum_art(0x1fa86a, 'SPEARMINT · 15'), r=0.002)
carton_sku('gum_ice', 0.075, 0.02, 0.05, gum_art(0x2a6fd8, 'ARCTIC ICE · 15'), r=0.002)
carton_sku('gum_fruit', 0.075, 0.02, 0.05, gum_art(0xe23b8a, 'WILD BERRY · 15'), r=0.002)


def mints_art(s, R):
    fill(s, R, 0xd9dde2, rough=0.2)
    x, y, w, h = R['top']
    s.rect(x, y + h * 0.15, w, h * 0.7, 0x6fd0e8, rough=0.2)
    mid(s, 'ARCTIC', x + w / 2, y + h * 0.58, h * 0.28, w * 0.8, 0x0e3a5a)
    mid(s, 'MINTS', x + w / 2, y + h * 0.3, h * 0.18, w * 0.6, 0x0e3a5a, face='Bold', tracking=1.4)


def mints_make(paths):
    w, d, h = 0.06, 0.04, 0.016
    obj = box('stock_mints', w, d, h, 0.006, segments=3)
    m = carton_material('mints', paths, w, d, h, wear=0.6)
    m.set('Metallic', 0.85)
    return finish_master(obj, m, tile_for_area(obj), sharp=40.0)


SKUS['mints'] = (lambda: _net_sheet('mints', 0.06, 0.04, 0.016, mints_art), mints_make)


def _net_sheet(sid, w, d, h, art):
    R = net_rects(w, d, h)
    s = Sheet(R['W'], R['H'])
    art(s, R)
    return render(s, sid, R['W'])


LIGHTERS = [0xd8262f, 0x2a6fd8, 0xf2c14e, 0x1fa86a, 0x141416, 0xe23b8a, 0xf28c28, 0x8a5fc8, 0xf4f4f0]


def lighter_tray_make(paths):
    """Flick lighters standing in their display box, five by five, every color the case came with."""
    objs = []
    tray = parts.rbox('tray', (0, 0, 0.022), (0.12, 0.1, 0.044), 0.002)
    tag(tray, 0)
    objs.append(tray)
    back = parts.rbox('tray_back', (0, 0.048, 0.07), (0.12, 0.004, 0.14), 0.001)
    tag(back, 1)
    objs.append(back)
    rnd = random.Random(3)
    for i in range(5):
        for j in range(4):
            x, y = -0.044 + i * 0.022, -0.033 + j * 0.022
            body = parts.rbox('lighter', (x, y, 0.045), (0.02, 0.011, 0.07), 0.004, rot=Matrix.Rotation(rnd.uniform(-0.06, 0.06), 3, 'Y'))
            tag(body, 10 + (i * 4 + j) % len(LIGHTERS))
            hood = parts.rbox('hood', (x, y, 0.085), (0.017, 0.010, 0.012), 0.002)
            tag(hood, 2)
            objs.append(body)
            objs.append(hood)
    obj = core.join(objs, 'stock_lighters')
    m = core.Mat('stock_lighters')
    tc, sep = object_coords(m)
    attr = m.node('ShaderNodeAttribute')
    attr.attribute_type = 'GEOMETRY'
    attr.attribute_name = 'part'
    ink, alpha = printed(m, paths, front_uv(m, sep, -0.06, 0.06, 0.0, 0.14))
    s = Surf(0x141416, 0.4, 0.0)
    for k, c in enumerate(LIGHTERS):
        s = over(m, part_mask(m, 10 + k, attr), s, Surf(c, 0.25, 0.0))
    s = over(m, part_mask(m, 2, attr), s, Surf(0xc8ccd0, 0.25, 1.0))
    printed_parts = m.math('MAXIMUM', part_mask(m, 0, attr), part_mask(m, 1, attr))
    s = over(m, printed_parts, s, ink)
    return finish_master(obj, finish_mat(m, s), tile_for_area(obj, 1000.0), sharp=40.0)


def lighter_sheet():
    s = Sheet(120, 140)
    s.rect(0, 0, 120, 140, 0x141416, rough=0.35)
    s.rect(0, 0, 120, 44, 0xd8262f, rough=0.35)
    mid(s, 'FLICK', 60, 26, 22, 100, 0xffffff)
    mid(s, '$1.99 EA', 60, 9, 8, 60, 0xf2c14e, face='Bold')
    mid(s, 'FLICK', 60, 116, 20, 100, 0xd8262f)
    mid(s, 'CHILD RESISTANT · 21+', 60, 96, 6, 100, 0xffffff, face='Bold')
    return render(s, 'lighters', 120)


SKUS['lighters'] = (lighter_sheet, lighter_tray_make)


def gum_rack_make(paths):
    """A little three-step plastic rack for gum, wire-clear risers, its header card."""
    objs = []
    for k in range(3):
        step = parts.rbox('step', (0, -0.03 + k * 0.03, 0.006 + k * 0.035), (0.17, 0.03, 0.012), 0.002)
        tag(step, 0)
        objs.append(step)
        riser = parts.rbox('riser', (0, -0.016 + k * 0.03, k * 0.035 / 2 + 0.006), (0.17, 0.003, k * 0.035 + 0.012), 0.001)
        tag(riser, 0)
        objs.append(riser)
    head = parts.rbox('head', (0, 0.05, 0.125), (0.17, 0.004, 0.07), 0.002)
    tag(head, 1)
    objs.append(head)
    for sx in (-1, 1):
        post = parts.rbox('post', (sx * 0.083, 0.04, 0.06), (0.006, 0.02, 0.12), 0.002)
        tag(post, 0)
        objs.append(post)
    obj = core.join(objs, 'stock_gum_rack')
    m = core.Mat('stock_gum_rack')
    tc, sep = object_coords(m)
    attr = m.node('ShaderNodeAttribute')
    attr.attribute_type = 'GEOMETRY'
    attr.attribute_name = 'part'
    ink, alpha = printed(m, paths, front_uv(m, sep, -0.085, 0.085, 0.09, 0.16))
    s = over(m, part_mask(m, 1, attr), Surf(0xeef0f2, 0.3, 0.0), ink)
    return finish_master(obj, finish_mat(m, s), tile_for_area(obj, 1100.0), sharp=40.0)


def gum_rack_sheet():
    s = Sheet(170, 70)
    s.rect(0, 0, 170, 70, 0xffffff, rough=0.3)
    s.rect(0, 0, 170, 18, 0x1fa86a, rough=0.3)
    mid(s, 'CHEW', 85, 44, 30, 120, 0x1fa86a)
    mid(s, 'FRESH BREATH · 2 FOR $3', 85, 9, 9, 150, 0xffffff, face='Bold')
    return render(s, 'gum_rack', 170)


SKUS['gum_rack'] = (gum_rack_sheet, gum_rack_make)


# ------------------------------------------------------------------ the tobacco wall, the scratch-offs, phone cards

PACKS = [('harbor_red', 'HARBOR', 0xc8202c, 'FULL FLAVOR'), ('harbor_gold', 'HARBOR', 0xc9a24d, 'SMOOTH'), ('saxon', 'SAXON', 0x1f4fa8, 'BLUE'),
         ('saxon_menthol', 'SAXON', 0x2e8b57, 'MENTHOL'), ('pioneer', 'PIONEER', 0x2a2b2e, 'BOLD'), ('pioneer_silver', 'PIONEER', 0x9aa0a8, 'SILVER'),
         ('redline', 'REDLINE', 0xe0322e, 'KINGS'), ('cobalt', 'COBALT', 0x1b2f6e, 'ULTRA')]
PACK = (0.055, 0.022, 0.088)


def pack_art(name, color, sub):
    def art(s, R):
        fill(s, R, 0xf4f2ec, rough=0.3)
        for face in ('front', 'back'):
            x, y, w, h = R[face]
            s.rect(x, y + h * 0.48, w, h * 0.52, color, rough=0.3, metal=0.2 if color in (0xc9a24d, 0x9aa0a8) else 0.0)
            s.poly([(x, y + h * 0.48), (x + w / 2, y + h * 0.36), (x + w, y + h * 0.48)], color, rough=0.3)
            mid(s, name, x + w / 2, y + h * 0.72, h * 0.14, w * 0.88, 0xffffff)
            mid(s, sub, x + w / 2, y + h * 0.25, h * 0.07, w * 0.8, color, face='Bold', tracking=1.2)
            s.rect(x + w * 0.06, y + h * 0.03, w * 0.88, h * 0.12, 0xffffff)
            s.rect(x + w * 0.06, y + h * 0.03, w * 0.88, h * 0.12 * 0.08, 0x111111)
            text_lines(s, x + w * 0.1, y + h * 0.045, w * 0.8, h * 0.09, 0x111111, rows=2)
        x, y, w, h = R['top']
        s.rect(x, y, w, h, color, rough=0.3)
    return art


for pid, name, color, sub in PACKS:
    carton_sku(f'pack_{pid}', *PACK, pack_art(name, color, sub), r=0.0008, wear=0.2, density=2000.0)


CARDS = [('TALKNOW', 0xd7263d, '$10'), ('TALKNOW', 0xd7263d, '$25'), ('NORTHSIDE WIRELESS', 0x1e5fa8, '$30'), ('PLAYZONE', 0x2e9e5b, '$20'),
         ('PLAYZONE', 0x2e9e5b, '$50'), ('STREAMBOX', 0x8a3fd0, '$15'), ('PHONEMAX', 0xf28c28, '$10'), ('GIFT CARD', 0xd08a4e, '$25'),
         ('TALKNOW', 0xd7263d, '$40'), ('GAMEPASS', 0x141416, '$25'), ('VISTA PREPAID', 0x1aa6b7, '$50'), ('NORTHSIDE WIRELESS', 0x1e5fa8, '$50'),
         ('FOODIE', 0xe23b8a, '$15'), ('STREAMBOX', 0x8a3fd0, '$30')]
CARDS_X = (-2.2, 0.9)
CARDS_Z = (0.96, 1.26)
CARD = (0.1, 0.0015, 0.16)
CARD_TOP = CARDS_Z[1] - 0.03


def card_x():
    x0, x1 = CARDS_X
    step = (x1 - x0) / len(CARDS)
    return [x0 + step * (k + 0.5) for k in range(len(CARDS))]


def card_art(name, col, amt):
    def art(s, R):
        fill(s, R, 0xf7f7f3, rough=0.3)
        x, y, w, h = R['front']
        cx = x + w / 2
        s.circle(cx, y + h - 9, 4.5, 0x8a8a8a, n=20)
        s.rect(x + 7, y + 34, w - 14, 58, col, rough=0.22)
        s.rect(x + 7, y + 34, w - 14, 7, mix_hex(col, 0x000000, 0.3), rough=0.22)
        mid(s, name, cx, y + 76, 13, w - 20, 0xffffff)
        mid(s, amt, cx, y + 55, 20, w - 30, 0xffffff)
        mid(s, name, cx, y + 120, 11, w - 16, col)
        mid(s, 'PREPAID · ACTIVATE AT REGISTER', cx, y + 104, 4.5, w - 12, 0x333333, face='Bold')
        barcode(s, cx - 22, y + 8, 44, 20, seed=sh(name + amt))
        x, y, w, h = R['back']
        text_lines(s, x + 10, y + 40, w - 20, h - 70, 0x9a9a9a, rows=12)
    return art


for k, (name, col, amt) in enumerate(CARDS):
    carton_sku(f'card_{k}', *CARD, card_art(name, col, amt), r=0.0004, wear=0.15, density=960.0)


def carton_art(name, color, sub):
    """A carton of ten packs lying flat: the pack's livery on its top and long sides."""
    def art(s, R):
        fill(s, R, 0xf4f2ec, rough=0.3)
        for face in ('top', 'front', 'back'):
            x, y, w, h = R[face]
            s.rect(x, y + h * 0.45, w, h * 0.55, color, rough=0.3)
            mid(s, name, x + w * 0.5, y + h * 0.72, h * 0.3, w * 0.6, 0xffffff)
            mid(s, sub + ' · 10 PACKS', x + w * 0.5, y + h * 0.25, h * 0.14, w * 0.7, color, face='Bold', tracking=1.2)
    return art


for pid, name, color, sub in PACKS[:5]:
    carton_sku(f'carton_{pid}', 0.275, 0.09, 0.05, carton_art(name, color, sub), r=0.001, wear=0.3, density=700.0)


def merch_layout():
    """The tobacco merchandiser: 8 rows of pusher shelves 13 cm apart from 1.28 m, 3.26 m wide, a pack every 6.2 cm."""
    rows = 8
    pitch = 0.13
    z0 = 1.28
    x0, x1 = -2.28, 0.98
    cols = int((x1 - x0) / 0.062)
    return rows, pitch, z0, x0, x1, cols


MERCH_HEAD = 0.1


def merch_sheet():
    """The shelf edges' price strips and the lit header, as one front elevation."""
    rows, pitch, z0, x0, x1, cols = merch_layout()
    W, H = (x1 - x0) * 1000, (rows * pitch + MERCH_HEAD) * 1000
    s = Sheet(W, H)
    s.rect(0, 0, W, H, 0x16171a, rough=0.5)
    rnd = random.Random(6)
    for r in range(rows):
        y = r * pitch * 1000
        s.rect(0, y, W, 22, 0x1d1e22, rough=0.45)
        x = 4.0
        k = 0
        while x < W - 30:
            s.rect(x, y + 3, 30, 16, 0xffd23f if k % 7 == 3 else 0xf6f6f2, rough=0.35)
            s.text(f'{rnd.choice((8, 9, 10, 11))}.{rnd.choice((19, 29, 49, 79, 99))}', x + 15, y + 6.5, 9, 0x141414, face='Black', align='CENTER')
            x += 62 * rnd.choice((1, 2, 3))
            k += 1
    hy = rows * pitch * 1000
    s.rect(0, hy, W, H - hy, 0xf6f4ee, rough=0.25)
    s.rect(0, hy, W, 8, 0xd7263d, rough=0.3)
    s.rect(0, H - 6, W, 6, 0xd7263d, rough=0.3)
    for k, text in enumerate(['TOBACCO', 'WE CARD · UNDER 21 NO SALE', 'CARTON DEALS INSIDE', 'TOBACCO']):
        mid(s, text, W * (k + 0.5) / 4, hy + 8 + (H - hy - 14) * 0.5, (H - hy) * 0.5, W / 4 * 0.85, 0xd7263d if k != 1 else 0x1b1b1d,
            face='Black', tracking=1.12)
    return render(s, 'merch', W)


LOTTO = ['LUCKY 7s', 'CASH BLAST', 'GOLD RUSH', '$100 FRENZY', 'WILD CHERRY', 'MEGA MONEY', 'CROSSWORD', 'BINGO NIGHT', 'TRIPLE 777',
         'HOT SHOT', 'DIAMOND DASH', 'SET FOR LIFE', 'PIGGY BANK', 'EMBER JACKPOT', 'FAST CASH', 'MONEY TREE', 'LUCKY PENNY', 'BIG SPIN',
         'COLD HARD $', 'RIVER RICHES', 'DOUBLE DOWN', 'BLACK JACK', 'NIGHT OWL $', 'FIFTH ST.']
LOTTO_COLS = [0xe8c22a, 0x2e9e5b, 0xd7263d, 0x3a6fd8, 0x9a3fd0, 0xf28c28, 0x1aa6b7, 0xe23b8a]
LOTTO_X = (1.03, 2.27)
LOTTO_Z = (1.3, 2.42)
LOTTO_GRID = (4, 6)
LOTTO_HEAD = 0.11


def lotto_sheet():
    """The dispenser's face: its header, then a bin per game, the ticket showing, its number and price on the lip."""
    (lx0, lx1), (lz0, lz1) = LOTTO_X, LOTTO_Z
    W, H = (lx1 - lx0) * 1000, (lz1 - lz0) * 1000
    s = Sheet(W, H)
    s.rect(0, 0, W, H, 0x10224a, rough=0.35)
    head = LOTTO_HEAD * 1000
    s.rect(0, H - head, W, head, 0xf2c14e, rough=0.3)
    mid(s, 'SCRATCH-OFFS', W / 2, H - head * 0.45, head * 0.5, W * 0.9, 0x10224a)
    mid(s, 'NORTHSIDE STATE LOTTERY', W / 2, H - head * 0.86, head * 0.13, W * 0.8, 0x10224a, face='Bold', tracking=1.3)
    nc, nr = LOTTO_GRID
    cw, ch = W / nc, (H - head) / nr
    rnd = random.Random(77)
    prices = [1, 2, 2, 3, 5, 5, 5, 10, 10, 20, 20, 30]
    tops = {1: '$500', 2: '$2,500', 3: '$5,000', 5: '$50,000', 10: '$100,000', 20: '$500,000', 30: '$1,000,000'}
    for r in range(nr):
        for c in range(nc):
            k = r * nc + c
            x, y = c * cw, r * ch
            tx, ty, tw, th = x + 8, y + 24, cw - 16, ch - 30
            col = LOTTO_COLS[k % len(LOTTO_COLS)]
            price = rnd.choice(prices)
            # The ticket: a rich ground, its game's name on a darker band, the top prize, the silver to scratch.
            s.rect(tx, ty, tw, th, col, rough=0.3)
            for q in range(10):
                a = q * math.pi / 5 + k
                s.poly([(tx + tw * 0.5, ty + th * 0.55), (tx + tw * 0.5 + th * 0.44 * math.cos(a), ty + th * 0.55 + th * 0.44 * math.sin(a)),
                        (tx + tw * 0.5 + th * 0.44 * math.cos(a + 0.2), ty + th * 0.55 + th * 0.44 * math.sin(a + 0.2))], shade(col, 0.15), rough=0.3)
            s.rect(tx, ty + th * 0.68, tw, th * 0.32, shade(col, -0.35), rough=0.3)
            mid(s, LOTTO[k % len(LOTTO)], tx + tw * 0.44, ty + th * 0.84, th * 0.2, tw * 0.74, 0xffffff)
            mid(s, f'WIN UP TO {tops[price]}', tx + tw * 0.5, ty + th * 0.56, th * 0.1, tw * 0.86, 0xffe14a)
            s.rect(tx + 6, ty + 6, tw - 12, th * 0.38, 0xb9bdc2, metal=0.8, rough=0.35)
            for q in range(6):
                s.circle(tx + 18 + q * (tw - 36) / 5, ty + 6 + th * 0.19, 6.5, 0x8d9298, n=20, metal=0.8, rough=0.35)
            s.circle(tx + tw - 16, ty + th - 16, 13, 0xffffff, n=32)
            mid(s, f'${price}', tx + tw - 16, ty + th - 16, 12, 20, col, face='Black')
            s.rect(x + 8, y + 4, cw - 16, 16, 0xf6f4ee, rough=0.35)
            mid(s, f'#{k + 1}   ${price}', x + cw / 2, y + 12, 12, cw - 30, 0x10224a, face='Black')
    return render(s, 'lotto', W)


def fixture_material(sid, finishes, prints=()):
    """One fixture's finish by its parts' tags: finishes {tag: Surf}, prints [(tag, sheet, (x0, x1), (z0, z1))] seen from the front."""
    m = core.Mat(f'stock_{sid}')
    tc, sep = object_coords(m)
    attr = m.node('ShaderNodeAttribute')
    attr.attribute_type = 'GEOMETRY'
    attr.attribute_name = 'part'
    s = None
    for k, f in finishes.items():
        f = f(m, tc, sep) if callable(f) else f
        s = f if s is None else over(m, part_mask(m, k, attr), s, f)
    for k, paths, xr, zr in prints:
        ink, alpha = printed(m, paths, front_uv(m, sep, xr[0], xr[1], zr[0], zr[1]))
        s = over(m, part_mask(m, k, attr), s, ink)
    return finish_mat(m, s)


def laminate(hex_color, rough=0.45):
    """Melamine over particleboard: a faint grain, scuffed low down by shoes and mop buckets (matte, lighter streaks
    that run along the doors, not blobs)."""
    def f(m, tc, sep):
        grain = noise(m, scaled(m, sep, 3.0, 80.0, 80.0), detail=3.0)
        col = m.mix(m.math('MULTIPLY', grain, 0.35), core.hex_linear(hex_color), core.hex_linear(shade(hex_color, -0.25)))
        scuff = m.math('MULTIPLY', ramp(m, noise(m, scaled(m, sep, 5.0, 30.0, 70.0), detail=5.0), 0.62, 0.8),
                       m.math('SUBTRACT', 1.0, ramp(m, sep.outputs['Z'], 0.1, 0.45)))
        return over(m, m.math('MULTIPLY', scuff, 0.55), Surf(col, m.math('ADD', rough, m.math('MULTIPLY', grain, 0.08)), 0.0), Surf(0x3e3f43, 0.7, 0.0))
    return f


def powder(hex_color, rough=0.5):
    def f(m, tc, sep):
        peel = noise(m, tc.outputs['Object'], scale=600.0, detail=1.0)
        wear = ramp(m, noise(m, tc.outputs['Object'], scale=25.0, detail=4.0), 0.68, 0.8)
        return Surf(m.mix(m.math('MULTIPLY', wear, 0.4), core.hex_linear(hex_color), core.hex_linear(shade(hex_color, 0.25))),
                    m.math('ADD', rough, m.math('MULTIPLY', peel, 0.06)), 0.0)
    return f


def speckle(m, tc, sep):
    """A granite-look laminate counter top, as it bakes at under two hundred texels a meter: a soft cloud of warm
    greys with the fine flecks averaged into it (drawn coarse, they baked to blotches), rubbed glossier along the
    front edge where things slide on and off, and the faint rings of Benny's coffee cups."""
    o = tc.outputs['Object']
    cloud = noise(m, o, scale=7.0, detail=4.0)
    fleck = ramp(m, noise(m, o, scale=900.0, detail=1.0), 0.56, 0.64)
    col = m.mix(ramp(m, cloud, 0.3, 0.7), core.hex_linear(0x5f5c57), core.hex_linear(0x8d8982))
    col = m.mix(m.math('MULTIPLY', fleck, 0.5), col, core.hex_linear(0x34322f))
    y = sep.outputs['Y']
    worn = m.math('MULTIPLY', ramp(m, y, -0.3, -0.44), ramp(m, noise(m, o, scale=4.0, detail=3.0), 0.35, 0.65))
    rough = mix_float(m, worn, m.math('ADD', 0.4, m.math('MULTIPLY', cloud, 0.06)), 0.24)
    rings = None
    for cx, cy, r in ((-1.32, -0.27, 0.041), (-1.27, -0.2, 0.04), (0.74, -0.31, 0.038)):
        d = m.math('ABSOLUTE', m.math('SUBTRACT', circle_dist(m, sep, cx, cy), r))
        rings = add(m, rings, smooth_less(m, d, 0.003, 0.0015))
    rings = m.math('MULTIPLY', m.math('MINIMUM', rings, 1.0), ramp(m, noise(m, o, scale=60.0, detail=2.0), 0.3, 0.6))
    col = m.mix(m.math('MULTIPLY', rings, 0.55), col, core.hex_linear(0x4a3828))
    return Surf(col, mix_float(m, rings, rough, 0.5), 0.0)


def slatwall(m, tc, sep):
    """Slatwall: grooves every 7.5 cm."""
    g = smooth_less(m, m.math('ABSOLUTE', m.math('SUBTRACT', m.math('FRACT', m.math('DIVIDE', sep.outputs['Z'], 0.075)), 0.5)), 0.06, 0.01)
    return Surf(m.mix(g, core.hex_linear(0xd9d6cf), core.hex_linear(0x3a3936)), mix_float(m, g, 0.5, 0.8), 0.0)


def bb_cabinet_make(paths):
    """The back counter: a laminate carcass on a kick, eight doors with bar pulls, a speckled top."""
    D = 0.46
    o = [tag(parts.rbox('carcass', (0, -D / 2 + 0.01, 0.46), (4.6, D - 0.02, 0.84), 0.004), 0),
         tag(parts.rbox('kick', (0, -D + 0.07, 0.05), (4.58, 0.02, 0.1), 0.003), 2),
         tag(parts.rbox('top', (0, -D / 2 - 0.01, 0.9), (4.62, D + 0.02, 0.035), 0.004), 1)]
    for k in range(8):
        x = -2.3 + 0.2875 + k * 0.575
        o.append(tag(parts.rbox('door', (x, -D + 0.003, 0.5), (0.565, 0.018, 0.76), 0.003), 0))
        px = x + (0.22 if k % 2 == 0 else -0.22)
        o.append(tag(parts.rod('pull', (px, -D - 0.022, 0.64), (px, -D - 0.022, 0.84), 0.006, 10), 3))
        for z in (0.66, 0.82):
            o.append(tag(parts.rod('pull_leg', (px, -D - 0.006, z), (px, -D - 0.022, z), 0.004, 8), 3))
    obj = core.join(o, 'stock_bb_cabinet')
    mat = fixture_material('bb_cabinet', {0: laminate(0x24262c), 1: speckle, 2: Surf(0x0e0e10, 0.65, 0.0), 3: Surf(0xb8bcc0, 0.22, 1.0)})
    # Half its area was faces nobody sees (the doors' backs, the carcass's lid under the top and its floor, the top's
    # underside): gone, and their texels spent on the doors and the counter top.
    cull_hidden(obj, backs=True)
    cull_where(obj, lambda f, c: f.normal.z < -0.9 or (f.normal.z > 0.9 and c.z < 0.885 and f.calc_area() > 0.02))
    return finish_master(obj, mat, tile_for_area(obj, 170.0), sharp=40.0)


def bb_merch_frame_make(paths):
    """The merchandiser's carcass: back panel, ends, a pusher shelf per row and the header box, black powder coat."""
    rows, pitch, z0, x0, x1, cols = merch_layout()
    mw, mc = x1 - x0, (x0 + x1) / 2
    mtop = z0 + rows * pitch + MERCH_HEAD
    o = [parts.rbox('merch_back', (mc, -0.02, (z0 + mtop) / 2), (mw + 0.04, 0.04, mtop - z0), 0.004),
         parts.rbox('merch_header', (mc, -0.17, mtop - MERCH_HEAD / 2), (mw + 0.04, 0.32, MERCH_HEAD), 0.004)]
    for sx in (-1, 1):
        o.append(parts.rbox('merch_end', (mc + sx * (mw / 2 + 0.01), -0.17, (z0 + mtop) / 2), (0.02, 0.32, mtop - z0 + 0.02), 0.004))
    for r in range(rows):
        z = z0 + r * pitch
        o.append(parts.rbox('merch_shelf', (mc, -0.17, z + 0.005), (mw, 0.3, 0.01), 0.002))
    obj = core.join(o, 'stock_bb_merch_frame')
    mat = fixture_material('bb_merch_frame', {0: powder(0x18191c)})
    return finish_master(cull_hidden(obj), mat, tile_for_area(obj, 100.0), sharp=40.0)


def bb_merch_face_make(paths):
    """The merchandiser's printed fronts: a price strip on every shelf's lip, the header's lit panel."""
    rows, pitch, z0, x0, x1, cols = merch_layout()
    mw, mc = x1 - x0, (x0 + x1) / 2
    mtop = z0 + rows * pitch + MERCH_HEAD
    o = [parts.rbox('lip', (mc, -0.325, z0 + r * pitch + 0.011), (mw, 0.006, 0.022), 0.0008) for r in range(rows)]
    o.append(parts.rbox('header_face', (mc, -0.333, mtop - MERCH_HEAD / 2), (mw + 0.04, 0.008, MERCH_HEAD), 0.002))
    obj = core.join(o, 'stock_bb_merch_face')
    tag(obj, 1)
    mat = fixture_material('bb_merch_face', {1: Surf(0x16171a, 0.4)}, [(1, paths, (x0, x1), (z0, mtop))])
    return finish_master(cull_hidden(obj, backs=True), mat, tile_for_area(obj, 700.0), sharp=40.0)


def bb_lotto_make(paths):
    """The scratch-off dispenser: a deep navy cabinet, its bins boxed out in front by fins and shelves, a printed lip
    under each bin."""
    (lx0, lx1), (lz0, lz1) = LOTTO_X, LOTTO_Z
    nc, nr = LOTTO_GRID
    cw, ch = (lx1 - lx0) / nc, (lz1 - lz0 - LOTTO_HEAD) / nr
    o = [tag(parts.rbox('lotto_body', ((lx0 + lx1) / 2, -0.1, (lz0 + lz1) / 2), (lx1 - lx0, 0.2, lz1 - lz0), 0.006), 1)]
    for c in range(nc + 1):
        o.append(tag(parts.rbox('fin', (lx0 + c * cw + (0.004 if c == 0 else -0.004 if c == nc else 0.0), -0.217, lz0 + (lz1 - lz0 - LOTTO_HEAD) / 2),
                                (0.008, 0.034, lz1 - lz0 - LOTTO_HEAD), 0.002), 0))
    for r in range(nr + 1):
        o.append(tag(parts.rbox('ledge', ((lx0 + lx1) / 2, -0.217, lz0 + r * ch + 0.003), (lx1 - lx0, 0.034, 0.006), 0.002), 0))
    for r in range(nr):
        for c in range(nc):
            o.append(tag(parts.rbox('lip', (lx0 + (c + 0.5) * cw, -0.232, lz0 + r * ch + 0.013), (cw - 0.012, 0.004, 0.016), 0.001), 1))
    obj = core.join(o, 'stock_bb_lotto')
    mat = fixture_material('bb_lotto', {0: Surf(0x0c1a3a, 0.35), 1: Surf(0x10224a, 0.35)}, [(1, paths, LOTTO_X, LOTTO_Z)])
    return finish_master(cull_hidden(obj), mat, tile_for_area(obj, 500.0), sharp=40.0)


def bb_cardpanel_make(paths):
    """The card strip's slatwall and its hooks."""
    (cx0, cx1), (cz0, cz1) = CARDS_X, CARDS_Z
    o = [tag(parts.rbox('card_panel', ((cx0 + cx1) / 2, -0.008, (cz0 + cz1) / 2), (cx1 - cx0 + 0.04, 0.016, cz1 - cz0), 0.003), 0)]
    for x in card_x():
        hz = CARD_TOP + 0.004
        o.append(tag(parts.rod('hook', (x, -0.016, hz), (x, -0.135, hz + 0.01), 0.0022, 8), 1))
        o.append(tag(parts.rbox('hook_plate', (x, -0.018, hz), (0.03, 0.004, 0.02), 0.001), 1))
    obj = core.join(o, 'stock_bb_cardpanel')
    mat = fixture_material('bb_cardpanel', {0: slatwall, 1: Surf(0xc8ccd0, 0.18, 1.0)})
    return finish_master(cull_hidden(obj), mat, tile_for_area(obj, 200.0), sharp=40.0)


def bb_divider_make(paths):
    """One of the merchandiser's pusher dividers (copied between every column)."""
    obj = box('stock_bb_divider', 0.0015, 0.29, 0.03, r=0.0)
    return finish_master(obj, fixture_material('bb_divider', {0: Surf(0x2a2b2f, 0.4)}), (64, 64), sharp=40.0)


SKUS['bb_divider'] = (None, bb_divider_make)
SKUS['bb_cabinet'] = (None, bb_cabinet_make)
SKUS['bb_merch_frame'] = (None, bb_merch_frame_make)
SKUS['bb_merch_face'] = (merch_sheet, bb_merch_face_make)
SKUS['bb_lotto'] = (lotto_sheet, bb_lotto_make)
SKUS['bb_cardpanel'] = (None, bb_cardpanel_make)


# ================================================================== rows

class Placement:
    __slots__ = ('sid', 'matrix', 'lo')

    def __init__(self, sid, matrix, lo=False):
        self.sid, self.matrix, self.lo = sid, matrix, lo


def M(x, y, z, yaw=0.0, tilt=0.0):
    """At (x, y, z), turned yaw degrees about Z, then tilted about X (negative leans the top back)."""
    return Matrix.Translation((x, y, z)) @ Matrix.Rotation(math.radians(yaw), 4, 'Z') @ Matrix.Rotation(math.radians(tilt), 4, 'X')


def shift(placements, dx, dy=0.0):
    for p in placements:
        p.matrix = Matrix.Translation((dx, dy, 0.0)) @ p.matrix
    return placements


DIMS = {}  # product id -> (width X, depth Y, height Z) of its master


def dims(sid):
    return DIMS.get(sid, (0.07, 0.07, 0.1))


def faced_row(columns, front, depth_avail, width, seed, deep=3, gap=0.004, spin=10.0, shopped=0.08, lean=0.0, back_lo=True):
    """Columns of products left to right across width, each faced to the front edge and up to `deep` ranks back.
    columns: [(id, facings[, stack[, deep]])]. spin: random turn (degrees); shopped: the chance a front one's gone.
    lean: degrees the packs lean back (soft bags against the ones behind)."""
    rnd = random.Random(seed)
    out = []
    facings = []
    for col in columns:
        sid, n = col[0], col[1]
        stack = col[2] if len(col) > 2 else 1
        cdeep = col[3] if len(col) > 3 else deep
        facings += [(sid, stack, cdeep)] * n
    widths = [dims(f[0])[0] for f in facings]
    total = sum(widths) + gap * (len(facings) - 1)
    g = gap if total <= width else max(0.0, gap - (total - width) / max(1, len(facings) - 1))
    total = sum(widths) + g * (len(facings) - 1)
    x = -total / 2
    lift = math.sin(math.radians(abs(lean)))
    for (sid, stack, cdeep), w in zip(facings, widths):
        d, h = dims(sid)[1], dims(sid)[2]
        step = d / math.cos(math.radians(lean)) + 0.003
        overhang = h * math.sin(math.radians(abs(lean)))  # a leaning pack's top reaches back past its foot
        ranks = min(cdeep, max(1, int((depth_avail - 0.004 - overhang) / step)))
        for k in range(ranks):
            if k == 0 and rnd.random() < shopped:
                continue
            y = front + d / 2 + k * step + rnd.uniform(0, 0.003)
            for q in range(stack):
                yaw = rnd.uniform(-spin, spin)
                out.append(Placement(sid, M(x + w / 2 + rnd.uniform(-0.0015, 0.0015), y, q * h * 1.002 + lift * d / 2, yaw, lean), lo=back_lo and k > 0))
        x += w + g
    return out


def cooler_row(columns, seed, deep=3, spin=12.0):
    return faced_row(columns, COOLER_FRONT, DOOR_SHELF_D, DOOR_SHELF_W - 0.012, seed, deep=deep, spin=spin)


def gondola_row(columns, seed, deep=3, spin=3.0, lean=0.0):
    return faced_row(columns, GONDOLA_FRONT, GONDOLA_BACK - GONDOLA_FRONT, BAY_W, seed, deep=deep, spin=spin, lean=lean, shopped=0.1)


def burrito_matrix(x, y, z, yaw):
    """A burrito (modeled standing, 17 cm) laid on its side along X, a little flattened."""
    return (M(x, y, z + 0.024, 90 + yaw) @ Matrix.Rotation(math.radians(90), 4, 'X') @ Matrix.Translation((0, 0, -0.085))
            @ Matrix.Diagonal((1.0, 0.86, 1.0, 1.0)))


def bar_standing(x, y, z, yaw, lean):
    """A candy bar (modeled lying face up) stood on its long edge, its face to the front, leaning back."""
    return M(x, y, z, yaw, -lean) @ Matrix.Translation((0, 0, 0.03)) @ Matrix.Rotation(math.radians(90), 4, 'X')


def caddy(cx, cy, z, seed, bars=12):
    """Choco Stack's counter caddy: the open carton, its header card behind, a column of bars standing in it."""
    rnd = random.Random(seed)
    out = [Placement('caddy', M(cx, cy, z)), Placement('caddy_header', M(cx, cy + 0.099, z + 0.036, 0, -8))]
    for k in range(bars):
        out.append(Placement('choco_stack', bar_standing(cx + rnd.uniform(-0.002, 0.002), cy - 0.082 + k * 0.0135, z + 0.0025,
                                                         rnd.uniform(-1.5, 1.5), 16 + rnd.uniform(0, 5)), lo=k > 1))
    return out


def layout(name):
    rnd = random.Random(sh(name))
    if name == 'SM_Stock_CoolerRow_Soda':
        return cooler_row([('fizz_20', 4), ('diet_fizz_20', 2), ('lime_spark_20', 2), ('sun_pop_20', 2)], 11)
    if name == 'SM_Stock_CoolerRow_Cans':
        return cooler_row([('fizz_can', 4), ('diet_fizz_can', 2), ('oldmill_can', 2), ('fizz_can', 2)], 12)
    if name == 'SM_Stock_CoolerRow_Energy':
        return cooler_row([('volt_can', 4), ('volt_zero_can', 2), ('blaze_can', 2), ('night_owl', 2)], 13)
    if name == 'SM_Stock_CoolerRow_Water':
        return cooler_row([('cascade', 5), ('stamina_20', 5)], 14)
    if name == 'SM_Stock_CoolerRow_Tea':
        return cooler_row([('sunny_peach', 4), ('sunny_lemon', 2), ('hillside_choc', 3)], 15)
    if name == 'SM_Stock_CoolerRow_Deli':
        # Sandwich wedges on the left; burritos lying across in two layers; Roller Dog franks flat in a pile; milk on the right.
        out = shift(faced_row([('deli_wedge', 5)], COOLER_FRONT, 0.3, 0.42, 16, deep=2, spin=2.0, shopped=0.0), -0.13)
        for k in range(3):
            for q in range(2):
                out.append(Placement('big_bean', burrito_matrix(0.17 + rnd.uniform(-0.004, 0.004), COOLER_FRONT + 0.032 + k * 0.058, q * 0.05,
                                                                rnd.uniform(-4, 4)), lo=k > 0))
        for k in range(3):
            out.append(Placement('franks', M(0.17 + rnd.uniform(-0.006, 0.006), COOLER_FRONT + 0.28, 0.016 + k * 0.031, rnd.uniform(-6, 6), -90)
                                 @ Matrix.Translation((0, 0, -0.065))))
        out += shift(faced_row([('hillside_choc', 1)], COOLER_FRONT, 0.3, 0.07, 17, deep=3), 0.31)
        return out
    if name == 'SM_Stock_CoolerRow_Ice':
        # Seven-pound bags lying flat, two across and three high, a little slumped into each other.
        out = []
        for c in range(2):
            for q in range(3):
                out.append(Placement('ice', M((c - 0.5) * 0.31 + rnd.uniform(-0.01, 0.01), COOLER_FRONT + 0.012 + 0.21 + rnd.uniform(0, 0.02),
                                              0.05 + q * 0.088, rnd.uniform(-5, 5), -90) @ Matrix.Translation((0, 0, -0.21)), lo=q < 2))
        return out
    if name == 'SM_Stock_ShelfRow_Chips':
        return gondola_row([('hilltop_sv', 2), ('hilltop_og', 1), ('hilltop_bbq', 1), ('curls', 1)], 21, lean=-4.0)
    if name == 'SM_Stock_ShelfRow_Trail':
        return gondola_row([('ridgeline', 4), ('ridgeline_jerky', 3)], 22, deep=4, lean=-4.0)
    if name == 'SM_Stock_ShelfRow_Candy':
        out = shift(faced_row([('gummy', 3)], GONDOLA_FRONT, GONDOLA_BACK - GONDOLA_FRONT, 0.41, 23, deep=5, lean=-6.0, spin=3.0), -0.335)
        for k, cx in enumerate((0.0, 0.2, 0.4)):
            out += caddy(cx, GONDOLA_FRONT + 0.103, 0.0, 30 + k)
        return out
    if name == 'SM_Stock_ShelfRow_Grocery':
        return gondola_row([('oodle', 2, 2), ('cocoa', 1), ('drip_coffee', 2), ('soup', 3, 2), ('cereal', 1), ('crackers', 1)], 24, spin=6.0)
    if name == 'SM_Stock_ShelfRow_Household':
        return gondola_row([('towels', 1), ('tp', 1, 2), ('detergent', 2), ('cells', 3, 2, 6)], 25, deep=2)
    if name == 'SM_Stock_ShelfRow_Bulk':
        out = [Placement('cascade_case', M(-0.32, GONDOLA_FRONT + 0.13 + 0.001, 0.0, 0.8))]
        out.append(Placement('fizz_12pk', M(0.12, GONDOLA_FRONT + 0.066, 0.0, -0.6)))
        out.append(Placement('fizz_12pk', M(0.12, GONDOLA_FRONT + 0.066 + 0.133, 0.0, 0.5), lo=True))
        for k in range(2):
            for j in range(3):
                out.append(Placement('oil', M(0.373 + k * 0.111, GONDOLA_FRONT + 0.034 + j * 0.068, 0.0, rnd.uniform(-2, 2)), lo=j > 0))
        return out
    if name == 'SM_Stock_BackBar_Tobacco':
        return tobacco_layout()
    if name == 'SM_Stock_BackBar':
        return backbar_layout()
    if name == 'SM_Stock_Counter':
        return counter_layout()
    raise KeyError(name)


def backbar_layout():
    """The back bar's furniture (cabinet, the merchandiser's carcass, the scratch-off dispenser, the card strip) with its
    cards and cartons. The tobacco itself is SM_Stock_BackBar_Tobacco, at the same origin."""
    out = [Placement(sid, Matrix()) for sid in ('bb_cabinet', 'bb_merch_frame', 'bb_lotto', 'bb_cardpanel')]
    rnd = random.Random(41)
    # Cards two or three deep on each hook.
    for k, x in enumerate(card_x()):
        for q in range(2 + k % 2):
            out.append(Placement(f'card_{k}', M(x + rnd.uniform(-0.002, 0.002), -0.125 + q * 0.03, CARD_TOP - CARD[2], rnd.uniform(-2, 2))))
    # Cartons on the counter top, under the rack.
    for k, (pid, x) in enumerate((('harbor_red', -1.9), ('saxon', -1.62), ('pioneer', 0.3))):
        for q in range(2):
            out.append(Placement(f'carton_{pid}', M(x + rnd.uniform(-0.01, 0.01), -0.33, 0.9175 + q * 0.052, rnd.uniform(-4, 4))))
    return out


def tobacco_layout():
    """The merchandiser's stock: a pack faced up in every pusher (a brand a few columns wide, each row starting its run
    somewhere else, a few sold through), the dividers between them, the price strips and the header."""
    out = [Placement('bb_merch_face', Matrix())]
    rows, pitch, z0, x0, x1, cols = merch_layout()
    rnd = random.Random(40)
    run = []
    k = 0
    while len(run) < cols * 2:
        run += [PACKS[k % len(PACKS)][0]] * rnd.choice((3, 4, 5, 6))
        k += 1
    for r in range(rows):
        z = z0 + r * pitch + 0.01
        off = (r * 11) % cols
        for c in range(cols + 1):
            out.append(Placement('bb_divider', M(x0 + (x1 - x0 - 0.062 * cols) / 2 + 0.062 * c, -0.17, z)))
        for c in range(cols):
            if rnd.random() < 0.05:
                continue  # sold through to the pusher
            x = x0 + (x1 - x0 - 0.062 * cols) / 2 + 0.062 * (c + 0.5)
            out.append(Placement(f'pack_{run[off + c]}', M(x, -0.304 + PACK[1] / 2, z, rnd.uniform(-1.0, 1.0))))
    return out


def counter_layout():
    """Beside the register: the Choco Stack caddy, the lighter box, the gum rack with two packs high on each step, mints."""
    rnd = random.Random(50)
    out = caddy(-0.03, 0.0, 0.0, 51)
    out.append(Placement('lighters', M(-0.22, 0.01, 0.0, 4)))
    out.append(Placement('gum_rack', M(0.17, 0.02, 0.0, -3)))
    gums = ['gum_mint', 'gum_ice', 'gum_fruit']
    for k in range(3):
        for j in range(2):
            for q in range(2):
                out.append(Placement(gums[(j + k) % 3], M(0.17 + (j - 0.5) * 0.078, -0.01 + k * 0.03, 0.012 + k * 0.035 + q * 0.0505,
                                                          rnd.uniform(-3, 3)), lo=q > 0))
    for k in range(4):
        out.append(Placement('mints', M(-0.25 + (k % 2) * 0.066, -0.085 - (k // 2) * 0.046, 0.0, rnd.uniform(-8, 8))))
    return out


def row_products(name):
    return sorted({p.sid for p in layout(name)})


# ================================================================== build: the products, each to bake

def wanted_rows():
    if not ONLY:
        return MESHES
    return [n for n in MESHES if any(o in n for o in ONLY)]


def wanted_products():
    ids = set()
    for name in wanted_rows():
        ids |= set(row_products(name))
    return sorted(ids)


def build():
    core.reset()
    DIMS.clear()
    ids = wanted_products()
    art = {sid: (SKUS[sid][0]() if SKUS[sid][0] else None) for sid in ids}
    core.reset()
    # Extend the bake past the islands' edges (the default margin follows adjacent faces, which leaves the slivers
    # between a lathe's pole triangles black, and mip-mapping drags that black onto the caps).
    bpy.context.scene.render.bake.margin_type = 'EXTEND'
    TEXTURE_SIZES.clear()
    masters = []
    for sid in ids:
        obj = SKUS[sid][1](art[sid])
        obj.name = f'stock_{sid}'
        obj.data.name = obj.name
        masters.append(obj)
    return masters


# ================================================================== assemble: atlases and rows

def _pixels(path):
    img = bpy.data.images.load(path, check_existing=False)
    w, h = img.size
    a = np.empty(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(a)
    bpy.data.images.remove(img)
    return a.reshape(h, w, 4)


def _save(arr, path, non_color):
    h, w = arr.shape[:2]
    img = bpy.data.images.new(os.path.splitext(os.path.basename(path))[0], w, h, alpha=False)
    img.colorspace_settings.name = 'Non-Color' if non_color else 'sRGB'
    img.pixels.foreach_set(np.ascontiguousarray(arr, dtype=np.float32).ravel())
    img.filepath_raw = path
    img.file_format = 'PNG'
    img.save()
    return path


def _half(a, normal=False):
    h, w = a.shape[:2]
    b = a.reshape(h // 2, 2, w // 2, 2, 4).mean(axis=(1, 3))
    if normal:
        n = b[..., :3] * 2.0 - 1.0
        n /= np.maximum(np.linalg.norm(n, axis=2, keepdims=True), 1e-6)
        b[..., :3] = n * 0.5 + 0.5
    return b


def _pow2(v):
    p = 64
    while p < v:
        p *= 2
    return p


def _skyline(tiles, order, W):
    """Bottom-left skyline packing of tiles (in order) into width W: ({id: (x, y)}, height used), or None."""
    sky = [[0, 0, W]]  # segments (x, y, width) along the top of what's placed
    pos = {}
    for sid in order:
        w, h = tiles[sid]
        if w > W:
            return None
        best = None
        for i, (x, _, _) in enumerate(sky):
            if x + w > W:
                break
            top, span, j = 0, 0, i
            while span < w:
                top = max(top, sky[j][1])
                span += sky[j][2]
                j += 1
            if best is None or (top + h, x) < (best[0] + h, best[1]):
                best = (top, x)
        y, x = best
        pos[sid] = (x, y)
        # Raise the skyline under the new tile.
        new = []
        for sx, sy, sw in sky:
            if sx + sw <= x or sx >= x + w:
                new.append([sx, sy, sw])
                continue
            if sx < x:
                new.append([sx, sy, x - sx])
            if sx + sw > x + w:
                new.append([x + w, sy, sx + sw - x - w])
        new.append([x, y + h, w])
        new.sort()
        sky = []
        for seg in new:
            if sky and sky[-1][1] == seg[1] and sky[-1][0] + sky[-1][2] == seg[0]:
                sky[-1][2] += seg[2]
            else:
                sky.append(seg)
    return pos, max(y + tiles[s][1] for s, (x, y) in pos.items())


def _pack(tiles, max_side=2048):
    """Packs {id: (w, h)} into the smallest power-of-two atlas: (W, H, {id: (x, y)})."""
    orders = [sorted(tiles, key=lambda s: (-tiles[s][1], -tiles[s][0])), sorted(tiles, key=lambda s: (-tiles[s][0], -tiles[s][1])),
              sorted(tiles, key=lambda s: -tiles[s][0] * tiles[s][1])]
    best = None
    for W in (128, 256, 512, 1024, 2048, 4096):
        for order in orders:
            got = _skyline(tiles, order, W)
            if got is None:
                continue
            pos, used = got
            H = _pow2(used)
            key = (max(W, H) > max_side, W * H, abs(math.log2(W / H)))
            if best is None or key < best[0]:
                best = (key, W, H, pos)
    return best[1], best[2], best[3]


def _resize(a, w, h, normal=False):
    """Resamples a tile (Blender's own filter, through a float image so nothing's color-managed)."""
    H0, W0 = a.shape[:2]
    if (W0, H0) == (w, h):
        return a
    img = bpy.data.images.new('stock_resize', W0, H0, alpha=True, float_buffer=True)
    img.pixels.foreach_set(np.ascontiguousarray(a, dtype=np.float32).ravel())
    img.scale(w, h)
    b = np.empty(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(b)
    bpy.data.images.remove(img)
    b = b.reshape(h, w, 4)
    if normal:
        n = b[..., :3] * 2.0 - 1.0
        n /= np.maximum(np.linalg.norm(n, axis=2, keepdims=True), 1e-6)
        b[..., :3] = n * 0.5 + 0.5
    return b


def atlas(name, ids, masters, max_side=2048, floor=0.7):
    """Packs the row's products' baked tiles into its own atlas: returns (material, {id: uv rect}).

    A row whose products don't fit max_side at full density is scaled down just enough; one that would leave most of
    a power-of-two atlas empty trades up to 30% of its density for an atlas half or a quarter the size."""
    px = {}
    full = {}
    for sid in ids:
        mname = f'stock_{sid}'
        d = os.path.join(core.BUILD_DIR, 'textures', mname)
        px[sid] = {k: _pixels(os.path.join(d, f'T_{mname}_{k}.png')) for k in ('BaseColor', 'ORM', 'Normal')}
        h, w = px[sid]['BaseColor'].shape[:2]
        full[sid] = (w, h)
    choice = None
    for s in (1.0, 0.9, 0.8, 0.75, 0.7, 0.6, 0.5, 0.42, 0.35, 0.3, 0.25):
        tiles = {sid: (max(16, int(w * s) // 4 * 4), max(16, int(h * s) // 4 * 4)) for sid, (w, h) in full.items()}
        W, H, pos = _pack(tiles, max_side)
        if max(W, H) > max_side:
            continue
        # The first scale that fits; then a smaller atlas only for as little density as the floor allows.
        if choice is None or (s >= floor - 1e-6 and W * H < choice[2] * choice[3]):
            choice = (s, tiles, W, H, pos)
        if s <= floor + 1e-6:
            break
    s, tiles, W, H, pos = choice
    out = {k: np.zeros((H, W, 4), dtype=np.float32) for k in ('BaseColor', 'ORM', 'Normal')}
    out['ORM'][..., :] = (1.0, 0.6, 0.0, 1.0)
    out['Normal'][..., :] = (0.5, 0.5, 1.0, 1.0)
    out['BaseColor'][..., 3] = 1.0
    rects = {}
    for sid, (x, y) in pos.items():
        w, h = tiles[sid]
        for k in out:
            out[k][y:y + h, x:x + w] = _resize(px[sid][k], w, h, k == 'Normal')
        rects[sid] = (x / W, y / H, (x + w) / W, (y + h) / H)
    d = os.path.join(core.BUILD_DIR, 'textures', name)
    os.makedirs(d, exist_ok=True)
    paths = {'BaseColor': _save(out['BaseColor'], os.path.join(d, f'T_{name}_BaseColor.png'), False),
             'ORM': _save(_half(out['ORM']), os.path.join(d, f'T_{name}_ORM.png'), True),
             'Normal': _save(_half(out['Normal'], True), os.path.join(d, f'T_{name}_Normal.png'), True)}
    used = sum(w * h for w, h in tiles.values()) / (W * H)
    print(f'[stock] {name}: atlas {W} x {H} (ORM/Normal {W // 2} x {H // 2}), {len(ids)} products, {s:.0%} density, {used:.0%} used')
    mat = core.gltf_material(f'M_{name}', paths['BaseColor'], paths['ORM'], paths['Normal'])
    return mat, rects


def _remapped(master, rect, lo):
    """The product's mesh with its UVs moved into its atlas rect (a lighter re-turned lathe for back ranks)."""
    sid = master.name[len('stock_'):]
    if lo and sid in LATHES:
        segs = 12 if max(r for r, _ in LATHES[sid]) < 0.04 else 16
        tmp = core.lathe('lo', LATHES[sid], segments=segs)
        core.orient_normals(tmp)
        tmp.data.shade_smooth()
        tmp.data.set_sharp_from_angle(angle=math.radians(50.0))
        mw, mh = TEXTURE_SIZES[f'stock_{sid}']
        core.uv_layout(tmp, [(lambda f: True, 'keep', None, (PAD / mw, PAD / mh, 1.0 - PAD / mw, 1.0 - PAD / mh))])
        me = tmp.data
        bpy.data.objects.remove(tmp)
    else:
        me = master.data.copy()
    u0, v0, u1, v1 = rect
    uv = me.uv_layers.active.data
    a = np.empty(len(uv) * 2, dtype=np.float32)
    uv.foreach_get('uv', a)
    a = a.reshape(-1, 2)
    a[:, 0] = u0 + a[:, 0] * (u1 - u0)
    a[:, 1] = v0 + a[:, 1] * (v1 - v0)
    uv.foreach_set('uv', a.ravel())
    return me


def build_row(name, placements, masters, mat, rects):
    by_id = {o.name[len('stock_'):]: o for o in masters}
    meshes = {}
    bm = bmesh.new()
    for p in placements:
        key = (p.sid, p.lo)
        if key not in meshes:
            meshes[key] = _remapped(by_id[p.sid], rects[p.sid], p.lo)
        part = bmesh.new()
        part.from_mesh(meshes[key])
        part.transform(p.matrix)
        part.normal_update()
        # What rests on a shelf or on another pack never shows its underside.
        zmin = min(v.co.z for v in part.verts)
        doomed = [f for f in part.faces if f.normal.z < -0.97 and f.calc_center_median().z < zmin + 0.002]
        bmesh.ops.delete(part, geom=doomed, context='FACES_ONLY')
        tmp = bpy.data.meshes.new('tmp')
        part.to_mesh(tmp)
        part.free()
        bm.from_mesh(tmp)
        bpy.data.meshes.remove(tmp)
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    obj = core.link(bpy.data.objects.new(name, me))
    obj.data.materials.append(mat)
    for poly in me.polygons:
        poly.material_index = 0
    for m_ in meshes.values():
        bpy.data.meshes.remove(m_)
    return obj


def assemble(masters):
    for o in masters:
        DIMS[o.name[len('stock_'):]] = tuple(o.dimensions)
    rows = []
    for name in wanted_rows():
        placements = layout(name)
        ids = sorted({p.sid for p in placements})
        mat, rects = atlas(name, ids, masters, floor=HERO_FLOOR.get(name, 0.7))
        row = build_row(name, placements, masters, mat, rects)
        tris = sum(len(p.vertices) - 2 for p in row.data.polygons)
        lo, hi = core.bounds([row])
        print(f'[stock] {name}: {len(placements)} packs, {tris} triangles, X {lo.x:.3f}..{hi.x:.3f} Y {lo.y:.3f}..{hi.y:.3f} Z {lo.z:.3f}..{hi.z:.3f} m')
        clear = COOLER_CLEAR if name in COOLER_ROWS else DECK_CLEAR if name.endswith('_Bulk') else GONDOLA_CLEAR if name in SHELF_ROWS else None
        if clear is not None and hi.z > clear:
            print(f'[stock] WARNING {name} stands {hi.z:.3f} m, over its shelf\'s {clear:.3f} m clearance')
        rows.append(row)
    for o in masters:
        bpy.data.objects.remove(o)
    return rows




# ================================================================== review

_REVIEW_MATS = {}


def _stand_in(center, size, hex_color, rough=0.5, metal=0.0, emit=0.0):
    """Plain stand-in fixtures for the review (the real SM_Cooler and SM_Shelf glbs don't import back into Blender 5.2)."""
    key = (hex_color, rough, metal, emit)
    if key not in _REVIEW_MATS:
        m = core.Mat(f'review_{len(_REVIEW_MATS)}')
        m.set('Base Color', core.hex_linear(hex_color))
        m.set('Roughness', rough)
        m.set('Metallic', metal)
        if emit:
            m.set('Emission Color', core.hex_linear(hex_color))
            m.set('Emission Strength', emit)
        _REVIEW_MATS[key] = m
    o = parts.rbox('review_fixture', center, size, 0.0)
    core.assign(o, _REVIEW_MATS[key])
    return o


def _review_cooler(doors):
    """A few doors of the cooler: white interior, wire shelves, price rails, black mullions with their LED strips."""
    x0 = -cooler.DOORS * cooler.DOOR_W / 2
    x1 = x0 + doors * cooler.DOOR_W
    top = cooler.HEADER0
    _stand_in(((x0 + x1) / 2, cooler.DEPTH / 2 - 0.02, (cooler.KICK + top) / 2), (x1 - x0, 0.04, top - cooler.KICK), 0xe9ecef, 0.4)
    _stand_in(((x0 + x1) / 2, 0.02, cooler.KICK - 0.015), (x1 - x0, cooler.DEPTH - 0.06, 0.03), 0xe9ecef, 0.4)
    _stand_in(((x0 + x1) / 2, 0.02, top), (x1 - x0, cooler.DEPTH - 0.06, 0.02), 0xe9ecef, 0.4)
    _stand_in(((x0 + x1) / 2, -cooler.DEPTH / 2 + 0.04, cooler.KICK / 2), (x1 - x0, 0.02, cooler.KICK), 0x101012, 0.6)
    for i in range(doors + 1):
        x = x0 + i * cooler.DOOR_W
        _stand_in((x, -cooler.DEPTH / 2 + 0.05, (cooler.KICK + top) / 2), (0.05, 0.05, top - cooler.KICK), 0x1b1d21, 0.45)
        for side in (-1, 1):
            _stand_in((x + side * 0.027, -cooler.DEPTH / 2 + 0.07, (cooler.KICK + top) / 2), (0.006, 0.02, top - cooler.KICK - 0.1), 0xeaf4ff, 0.3, emit=8.0)
    for i in range(doors):
        x = x0 + (i + 0.5) * cooler.DOOR_W
        for z in cooler.SHELF_Z:
            _stand_in((x, 0.0, z - 0.008), (DOOR_SHELF_W, DOOR_SHELF_D, 0.016), 0x6a6d72, 0.35, metal=0.8)
            _stand_in((x, -DOOR_SHELF_D / 2 - 0.008, z - 0.004), (DOOR_SHELF_W, 0.008, 0.034), 0xf4f4f2, 0.35)
    return [x0 + (i + 0.5) * cooler.DOOR_W for i in range(doors)]


def _review_gondola(y, bays):
    """A run of gondola bays, its -Y face toward the camera."""
    L = bays * store_shelf.SECTION
    xc = (bays - 1) * store_shelf.SECTION / 2
    paint = 0xe4e1d9
    _stand_in((xc, y, store_shelf.BASE / 2), (L, 2 * store_shelf.DEPTH + store_shelf.SPINE, store_shelf.BASE), paint, 0.5)
    _stand_in((xc, y - store_shelf.DEPTH - store_shelf.SPINE / 2 + 0.01, store_shelf.BASE / 2),
              (L, 0.02, store_shelf.BASE), 0x2b2d31, 0.55)
    _stand_in((xc, y, (store_shelf.BASE + store_shelf.HEIGHT) / 2), (L, store_shelf.SPINE * 0.5, store_shelf.HEIGHT - store_shelf.BASE), paint, 0.5)
    for i in range(bays + 1):
        _stand_in((xc - L / 2 + i * store_shelf.SECTION, y, store_shelf.HEIGHT / 2), (0.05, store_shelf.SPINE, store_shelf.HEIGHT), paint, 0.48)
    for z in store_shelf.SHELF_Z:
        _stand_in((xc, y - store_shelf.SPINE / 2 - store_shelf.DEPTH / 2, z - 0.01), (L - 0.02, store_shelf.DEPTH, 0.02), paint, 0.48)
        _stand_in((xc, y - store_shelf.SPINE / 2 - store_shelf.DEPTH - 0.004, z - 0.025), (L - 0.02, 0.008, 0.05), 0xdfe6ea, 0.2)
    return [i * store_shelf.SECTION for i in range(bays)]


def pose_for_review(objs):
    """Stocks four doors of a stand-in cooler (rows at the cooler's own shelf heights), three bays of a gondola, the back
    bar against a wall and the counter set on a counter."""
    by = {o.name: o for o in objs}
    used = set()

    def put(name, loc, yaw=0.0):
        if name not in by:
            return
        o = by[name] if name not in used else _dup(by[name])
        used.add(name)
        o.location = loc
        o.rotation_euler = (0.0, 0.0, math.radians(yaw))

    doors = _review_cooler(4)
    plan = [['Soda', 'Cans', 'Soda', 'Cans'], ['Energy', 'Cans', 'Energy', 'Energy'], ['Water', 'Tea', 'Water', 'Tea'], ['Deli', 'Ice', 'Tea', 'Soda']]
    for i, col in enumerate(plan):
        for k, kind in enumerate(col):
            put(f'SM_Stock_CoolerRow_{kind}', (doors[i], 0.0, cooler.SHELF_Z[k]))
    shelf_y = -2.4
    bays = _review_gondola(shelf_y, 3)
    levels = [store_shelf.BASE] + list(store_shelf.SHELF_Z)
    plan = [['Bulk', 'Candy', 'Grocery', 'Chips'], ['Bulk', 'Household', 'Trail', 'Chips'], ['Bulk', 'Grocery', 'Candy', 'Trail']]
    for b, col in enumerate(plan):
        for k, kind in enumerate(col):
            put(f'SM_Stock_ShelfRow_{kind}', (bays[b], shelf_y, levels[k]))
    _stand_in((5.4, 0.92, 1.3), (5.2, 0.04, 2.6), 0xe9e4d8, 0.85)
    put('SM_Stock_BackBar', (5.4, 0.9, 0.0))
    put('SM_Stock_BackBar_Tobacco', (5.4, 0.9, 0.0))
    _stand_in((5.0, -1.3, 0.475), (1.2, 0.8, 0.95), 0x8e1c2a, 0.55)
    _stand_in((5.0, -1.3, 0.975), (1.24, 0.84, 0.05), 0xa0a4aa, 0.35, metal=0.9)
    put('SM_Stock_Counter', (5.0, -1.4, 1.0))


def _dup(o):
    d = o.copy()
    core.link(d)
    return d
